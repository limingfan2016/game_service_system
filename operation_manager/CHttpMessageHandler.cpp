
/* author : limingfan
 * date : 2017.09.27
 * description : Http 操作实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "base/Function.h"
#include "connect/CSocket.h"

#include "CHttpMessageHandler.h"
#include "COptMsgHandler.h"


using namespace NCommon;
using namespace NFrame;
using namespace NHttp;


// 数据记录日志
#define WriteDataLog(format, args...)     m_msgHandler->getSrvOpt().getDataLogger()->writeFile(NULL, 0, LogLevel::Info, format, ##args)


CHttpMessageHandler::CHttpMessageHandler()
{
}

CHttpMessageHandler::~CHttpMessageHandler()
{
}

void CHttpMessageHandler::init(COptMsgHandler* optMsgHandler)
{
	m_msgHandler = optMsgHandler;
	
	// 注册http POST消息处理函数
	registerPostOptHandler(m_msgHandler->m_pCfg->recharge_cfg.post_url, (HttpPostOptHandler)&CHttpMessageHandler::chessHallManagerRecharge, this);
	
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_MANAGER_RECHARGE_RSP, (ProtocolHandler)&CHttpMessageHandler::managerRechargeReply, this);
}

int CHttpMessageHandler::onInit(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	return SrvOptSuccess;
}

void CHttpMessageHandler::onUnInit(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
}

void CHttpMessageHandler::onUpdateConfig()
{
	m_msgHandler->loadConfig(true);
}


void CHttpMessageHandler::getRechargeRecordId(RecordIDType recordId)
{
	// 订单号格式 : timeDindexSsrcSrvId
	// 1458259523D00001S4001
	static unsigned int sRecordIdIndex = 0;
	const unsigned int MaxRecordIdIndex = 100000;
	snprintf(recordId, sizeof(RecordIDType) - 1, "%uD%05uS%u", (unsigned int)time(NULL), ++sRecordIdIndex % MaxRecordIdIndex, m_msgHandler->getSrvId());
}

bool CHttpMessageHandler::chessHallManagerRecharge(const char* connId, Connect* conn, const RequestHeader& reqHeaderData, const HttpMsgBody& msgBody)
{
	/*
	key=xxxxxxxxxx
    check context=userId=1000003&hallId=357780&type=2&count=1000&price=123.00&{key}
    userId=1000003&hallId=357780&type=2&count=1000.00&price=123.00&signature=md5(context)
	*/ 
	
	int rc = SrvOptFailed;
	string msg = "";

	do
	{
		ParamKey2Value::const_iterator typeIt = msgBody.paramKey2Value.find("type");
		if (typeIt == msgBody.paramKey2Value.end())
		{
			msg = "can not find the item type";
			break;
		}
		
		if (atoi(typeIt->second.c_str()) != EGameGoodsType::EGoodsGold)
		{
			msg = "the input type is error, " + typeIt->second;
			break;
		}
		
		ParamKey2Value::const_iterator priceIt = msgBody.paramKey2Value.find("price");
		if (priceIt == msgBody.paramKey2Value.end())
		{
			msg = "can not find the item price";
			break;
		}
		
		/*
		map<string, string>::const_iterator cfgCount = m_msgHandler->m_pCfg->recharge_cfg.gold_price_count.find(priceIt->second);
		if (cfgCount == m_msgHandler->m_pCfg->recharge_cfg.gold_price_count.end())
		{
			msg = "the input price is error, " + priceIt->second;
			break;
		}
		*/
		
		ParamKey2Value::const_iterator countIt = msgBody.paramKey2Value.find("count");
		if (countIt == msgBody.paramKey2Value.end())
		{
			msg = "can not find the item count";
			break;
		}
		
		if (priceIt->second != countIt->second)  // if (cfgCount->second != countIt->second)
		{
			msg = "the input count is error, " + countIt->second + " != " + priceIt->second;
			break;
		}
		
		ParamKey2Value::const_iterator userIdIt = msgBody.paramKey2Value.find("userId");
		if (userIdIt == msgBody.paramKey2Value.end())
		{
			msg = "can not find the user id";
			break;
		}
		
		ParamKey2Value::const_iterator hallIdIt = msgBody.paramKey2Value.find("hallId");
		if (hallIdIt == msgBody.paramKey2Value.end())
		{
			msg = "can not find the hall id";
			break;
		}

		SChessHallData* hallData = m_msgHandler->getChessHallData(hallIdIt->second.c_str(), rc, true);
		if (rc != SrvOptSuccess)
		{
			msg = "get chess hall info error, id = " + hallIdIt->second;
			break;
		}
		
		ParamKey2Value::const_iterator signatureIt = msgBody.paramKey2Value.find("signature");
		if (signatureIt == msgBody.paramKey2Value.end())
		{
			msg = "can not find the signature result";
			break;
		}
		
		char signatureContent[1024] = {0};
		const unsigned int signatureContentLen = snprintf(signatureContent, sizeof(signatureContent) - 1,
		"userId=%s&hallId=%s&type=%s&count=%s&price=%s&%s",
		userIdIt->second.c_str(), hallIdIt->second.c_str(), typeIt->second.c_str(), countIt->second.c_str(),
		priceIt->second.c_str(), m_msgHandler->m_pCfg->recharge_cfg.sign_key.c_str());
		
		// MD5加密
		char signatureResult[128] = {0};
        md5Encrypt(signatureContent, signatureContentLen, signatureResult, false);
		if (signatureIt->second != signatureResult)
		{
			msg = "the input signature is error, " + signatureIt->second + " != " + signatureResult;
			break;
		}
		
		// 订单记录ID
		RecordIDType recordId = {0};
		getRechargeRecordId(recordId);
		
		// 是否是扣除物品数量
		float itemCount = atof(countIt->second.c_str());
		if (itemCount < 0.00 && -itemCount > hallData->baseInfo.current_gold()) itemCount = -hallData->baseInfo.current_gold();

        // 初步验证成功，发放物品，充值信息入库
		com_protocol::UserRechargeReq rechargeReq;
		rechargeReq.set_order_id(recordId);
		rechargeReq.set_username(userIdIt->second);
		rechargeReq.set_hall_id(hallIdIt->second);
		rechargeReq.set_item_type(atoi(typeIt->second.c_str()));
		rechargeReq.set_item_id(0);
		rechargeReq.set_item_count(itemCount);
		rechargeReq.set_item_amount(hallData->baseInfo.current_gold() + rechargeReq.item_count());
		rechargeReq.set_recharge_rmb(itemCount);
		rechargeReq.set_third_type(EThirdPlatformType::EBusinessManagerWeiXin);
		rechargeReq.set_third_account(userIdIt->second);
		rechargeReq.set_third_other_data("");
		
		// 数据日志
		WriteDataLog("recharge|%s|%s|%s|%s|%.2f|%s|%s|request", userIdIt->second.c_str(), hallIdIt->second.c_str(),
		typeIt->second.c_str(), countIt->second.c_str(), rechargeReq.item_amount(), priceIt->second.c_str(), recordId);

        rc = m_msgHandler->getSrvOpt().sendPkgMsgToDbProxy(rechargeReq, DBPROXY_MANAGER_RECHARGE_REQ, connId, strlen(connId), "manager recharge request");
		
		OptInfoLog("send manager recharge request to db proxy, rc = %d, username = %s, hall id = %s, bill id = %s",
		rc, userIdIt->second.c_str(), hallIdIt->second.c_str(), recordId);
		
		return true;
		
	} while (false);
	
	// 验证错误则直接给http端回应答
	char rspData[MaxRequestDataLen] = {0};
	const unsigned int rspDataLen = snprintf(rspData, sizeof(rspData) - 1,
	"{\"code\":-1,\"message\":\"%s\"}", msg.c_str());
    
	CHttpResponse rsp;
	rsp.setContent(rspData, rspDataLen);

	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	rc = sendDataToClient(conn, rspMsg, msgLen);

	OptErrorLog("send manager recharge reply message, rc = %d, data = %s", rc, rspData);

	return false;
}

void CHttpMessageHandler::managerRechargeReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserRechargeRsp rechargeRsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(rechargeRsp, data, len, "manager recharge reply")) return;
	
	const com_protocol::UserRechargeReq& rechargeInfo = rechargeRsp.info();
	float itemAmount = rechargeInfo.item_amount();
	int rc = SrvOptFailed;

	if (rechargeRsp.result() == SrvOptSuccess)
	{
		SChessHallData* hallData = m_msgHandler->getChessHallData(rechargeInfo.hall_id().c_str(), rc, true);
		if (rc != SrvOptSuccess)
		{
			OptErrorLog("manager recharge reply but get hall info error, hall id = %s, rc = %d",
			rechargeInfo.hall_id().c_str(), rc);
		}
		
		// 如果B端管理员在线则同步通知
		ConnectProxy* connProxy = m_msgHandler->getConnectProxy(rechargeInfo.username());
		if (connProxy != NULL && hallData != NULL)
		{
		    com_protocol::BSRechargeResultNotify rechargeNotify;
			rechargeNotify.set_result(SrvOptSuccess);
			rechargeNotify.set_order_id(rechargeInfo.order_id());
			rechargeNotify.set_pay_money(rechargeInfo.recharge_rmb());

			com_protocol::ItemChange* rechargeItem = rechargeNotify.mutable_change_item();
			rechargeItem->set_type(rechargeInfo.item_type());
			rechargeItem->set_num(rechargeInfo.item_count());
			rechargeItem->set_amount(hallData->baseInfo.current_gold());
			
			itemAmount = rechargeItem->amount();
			
			rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rechargeNotify, BSC_MANAGER_RECHARGE_NOTIFY, connProxy, "manager recharge notify");
			
			OptInfoLog("manager recharge notify, username = %s, hall id = %s, goods type = %u, count = %.2f, amount = %.2f, rc = %d",
			rechargeInfo.username().c_str(), rechargeInfo.hall_id().c_str(),
			rechargeItem->type(), rechargeItem->num(), rechargeItem->amount(), rc);
		}
	}
	
	// 数据日志
	WriteDataLog("recharge|%s|%s|%u|%.2f|%.2f|%.2f|%s|%d|reply", rechargeInfo.username().c_str(), rechargeInfo.hall_id().c_str(),
	rechargeInfo.item_type(), rechargeInfo.item_count(), itemAmount, rechargeInfo.recharge_rmb(),
	rechargeInfo.order_id().c_str(), rechargeRsp.result());
	
	char rspData[MaxRequestDataLen] = {0};
	Connect* conn = getConnect(m_msgHandler->getContext().userData);
	if (conn != NULL)
	{
		// 给http端回应答
		const unsigned int rspDataLen = snprintf(rspData, sizeof(rspData) - 1, "{\"code\":%d,\"message\":\"%s\",\"total\":%.2f}",
		rechargeRsp.result(), (rechargeRsp.result() == SrvOptSuccess) ? "success" : "recharge error", itemAmount);
		
		CHttpResponse rsp;
		rsp.setContent(rspData, rspDataLen);

		unsigned int msgLen = 0;
		const char* rspMsg = rsp.getMessage(msgLen);
		rc = sendDataToClient(conn, rspMsg, msgLen);
	}

	OptInfoLog("send manager recharge reply message, rc = %d, conn id = %s, address = %p, data = %s",
	rc, m_msgHandler->getContext().userData, conn, rspData);
}
