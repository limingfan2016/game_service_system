
/* author : limingfan
 * date : 2016.01.12
 * description : 第三方平台接口API操作实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <algorithm>
#include <math.h>
#include <openssl/evp.h>


#include "CThirdInterface.h"
#include "CThirdPlatformOpt.h"
#include "CHttpSrv.h"
#include "CHttpData.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"
#include "common/CommonType.h"



using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;


// 充值日志文件
#define ThirdRechargeLog(platformId, format, args...)     m_platformatOpt->getThirdRechargeLogger(platformId).writeFile(NULL, 0, LogLevel::Info, format, ##args)

namespace NPlatformService
{

struct VivoOrderInfo : public UserCbData
{
	char orderId[MaxRechargeTransactionLen];
};

// Facebook用户信息校验
struct FacebookUserInfo : public UserCbData
{
	char userInfo[MaxRequestDataLen];
	unsigned int userInfoLen;
};


// 注册各个平台http消息处理函数
void CThirdInterface::registerHttpMsgHandler()
{
	/*
	{
		// 注册校验第三方用户请求处理函数
        m_msgHandler->registerCheckUserRequest(ThirdPlatformType::M4399, (CheckUserRequest)&CThirdInterface::checkM4399User, this);
		m_msgHandler->registerCheckUserRequest(ThirdPlatformType::MeiZu, (CheckUserRequest)&CThirdInterface::checkMeiZuUser, this);
       
        
		// 注册校验第三方用户应答处理函数
        m_msgHandler->registerCheckUserHandler(ThirdPlatformType::M4399, (CheckUserHandler)&CThirdInterface::checkM4399UserReply, this);
		m_msgHandler->registerCheckUserHandler(ThirdPlatformType::MeiZu, (CheckUserHandler)&CThirdInterface::checkMeiZuUserReply, this);

	}
	*/ 
	
	
	// 注册用户付费GET协议充值处理函数
    m_msgHandler->registerUserPayGetHandler(ThirdPlatformType::DownJoy, (UserPayGetOptHandler)&CThirdInterface::downJoyPaymentMsg, this);
	m_msgHandler->registerUserPayGetHandler(ThirdPlatformType::M4399, (UserPayGetOptHandler)&CThirdInterface::handleM4399PaymentMsg, this);

	
	// 注册用户付费POST协议充值处理函数
	m_msgHandler->registerUserPayPostHandler(ThirdPlatformType::MeiZu, (UserPayPostOptHandler)&CThirdInterface::handleMeiZuPaymentMsg, this);
	
	// 获取VIVO平台订单信息
	m_msgHandler->registerHttpReplyHandler(HttpReplyHandlerID::RIDGetVivoOrderInfo, (HttpReplyHandler)&CThirdInterface::getVivoOrderInfoReply, this);
	
	// 苹果版本充值结果校验
	m_msgHandler->registerHttpReplyHandler(HttpReplyHandlerID::CheckAppleIOSPay, (HttpReplyHandler)&CThirdInterface::checkAppleRechargeResultReply, this);
	
	// 韩国平台 优惠券奖励处理
	m_msgHandler->registerUserPayPostHandler(ThirdPlatformType::HanYou, (UserPayPostOptHandler)&CThirdInterface::handleHanYouCouponRewardMsg, this);
	
	// Facebook用户信息
	m_msgHandler->registerHttpReplyHandler(HttpReplyHandlerID::CheckFacebookUserInfo, (HttpReplyHandler)&CThirdInterface::checkFacebookUserReply, this);
}


CThirdInterface::CThirdInterface()
{
	m_msgHandler = NULL;
	m_platformatOpt = NULL;
	
	memset(m_orderIndex, 0, sizeof(m_orderIndex));
	
	m_baiduOrderIndex = 0;
}

CThirdInterface::~CThirdInterface()
{
	m_msgHandler = NULL;
	m_platformatOpt = NULL;
	
	memset(m_orderIndex, 0, sizeof(m_orderIndex));
	
	m_baiduOrderIndex = 0;
}

bool CThirdInterface::load(CSrvMsgHandler* msgHandler, CThirdPlatformOpt* platformatOpt)
{
	m_msgHandler = msgHandler;
	m_platformatOpt = platformatOpt;
	
	registerHttpMsgHandler();
	
	m_msgHandler->registerProtocol(CommonSrv, GET_FISH_COIN_RECHARGE_ORDER_REQ, (ProtocolHandler)&CThirdInterface::getFishCoinRechargeOrder, this);
	m_msgHandler->registerProtocol(CommonSrv, BUSINESS_GET_MALL_FISH_COIN_INFO_RSP, (ProtocolHandler)&CThirdInterface::getFishCoinInfoReply, this);
	m_msgHandler->registerProtocol(CommonSrv, CANCEL_FISH_COIN_RECHARGE_NOTIFY, (ProtocolHandler)&CThirdInterface::cancelFishCoinRechargeOrder, this);
	
	m_msgHandler->registerProtocol(CommonSrv, CHECK_APPLE_RECHARGE_RESULT, (ProtocolHandler)&CThirdInterface::checkAppleRechargeResult, this);
	m_msgHandler->registerProtocol(CommonSrv, CHECK_FACEBOOK_USER_REQ, (ProtocolHandler)&CThirdInterface::checkFacebookUser, this);
	
	m_msgHandler->registerProtocol(GameSrvDBProxy, SET_GAME_DATA_RSP, (ProtocolHandler)&CThirdInterface::saveOrderDataReply, this);
	m_msgHandler->registerProtocol(GameSrvDBProxy, DEL_GAME_DATA_RSP, (ProtocolHandler)&CThirdInterface::removeOrderDataReply, this);
	m_msgHandler->registerProtocol(GameSrvDBProxy, GET_MORE_KEY_GAME_DATA_RSP, (ProtocolHandler)&CThirdInterface::getUnHandleOrderDataReply, this);
	
    m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_ADD_HAN_YOU_COUPON_REWARD_RSP, (ProtocolHandler)&CThirdInterface::addHanYouCouponRewardReply, this);
	
	if (!parseUnHandleOrderData())
	{
		ReleaseErrorLog("parse fish coin unhandle transaction error");
		m_msgHandler->stopService();
		return false;
	}
	
	// 百度订单号索引，百度规定订单号长度不能超过11位
	// 向后延长1个小时的秒数，防止服务异常退出时，存在重复的订单号索引值
	m_baiduOrderIndex = time(NULL) + 3600;
	
	return true;
}

void CThirdInterface::unload()
{
	
}


bool CThirdInterface::handleM4399PaymentMsg(Connect* conn, const RequestHeader& reqHeaderData)
{
    bool isOK = handleM4399PaymentMsgImpl(reqHeaderData);
	CHttpResponse rsp;
	isOK ? rsp.setContent("{\"status\":2, \"code\":null, \"money\":, \"gamemoney\":\"10\", \"msg\":\"充值成功\"}") 
         : rsp.setContent("{\"status\":3, \"code\":null, \"money\":\"0\", \"gamemoney\":\"0\", \"msg\":\"充值失败\"}");
	
	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to 4399 http, rc = %d, data = %s", rc, rspMsg);
	return isOK;
}

bool CThirdInterface::handleM4399PaymentMsgImpl(const RequestHeader& reqHeaderData)
{
    // 签名结果
    ParamKey2Value::const_iterator signatureIt = reqHeaderData.paramKey2Value.find("sign");
	if (signatureIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleM4399PaymentMsgImpl, can not find the check data sign return");
		return false;
	}
    
    //外部订单号
    ParamKey2Value::const_iterator orderidIt = reqHeaderData.paramKey2Value.find("orderid");
	if (orderidIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleM4399PaymentMsgImpl, can not find the check data orderid");
		return false;
	}
    
    //用户uid
    ParamKey2Value::const_iterator uidIt = reqHeaderData.paramKey2Value.find("uid");
	if (uidIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleM4399PaymentMsgImpl, can not find the check data uid");
		return false;
	}
    
    //float	用户充值的人民币金额，单位：元
    ParamKey2Value::const_iterator moneyIt = reqHeaderData.paramKey2Value.find("money");
	if (moneyIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleM4399PaymentMsgImpl, can not find the check data money");
		return false;
	}
    
    //gamemoney	是	int	兑换的游戏币数量
    ParamKey2Value::const_iterator gamemoneyIt = reqHeaderData.paramKey2Value.find("gamemoney");
	if (gamemoneyIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleM4399PaymentMsgImpl, can not find the check data gamemoney");
		return false;
	}
    
    //serverid	否	int	要充值的服务区号(最多不超过8位)
    ParamKey2Value::const_iterator serveridIt = reqHeaderData.paramKey2Value.find("serverid");
	if (serveridIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleM4399PaymentMsgImpl, can not find the check data serverid");
		return false;
	}
    
    //内部订单
    ParamKey2Value::const_iterator markIt = reqHeaderData.paramKey2Value.find("mark");
	if (markIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleM4399PaymentMsgImpl, can not find the check data mark");
		return false;
	}
    
    // 内部订单号信息
    FishCoinOrderIdToInfo::iterator thirdInfoIt;
    if(!getTranscationData(markIt->second.c_str(), thirdInfoIt))
    {
        OptErrorLog("handleM4399PaymentMsgImpl, can not find transaction id = %s", markIt->second.c_str());
		return false;
    }
    
    //要充值的游戏角色id
    ParamKey2Value::const_iterator roleidIt = reqHeaderData.paramKey2Value.find("roleid");
	if (roleidIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleM4399PaymentMsgImpl, can not find the check data roleid");
		return false;
	}
    
    //发起请求时的时间戳
    ParamKey2Value::const_iterator timeIt = reqHeaderData.paramKey2Value.find("time");
	if (timeIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleM4399PaymentMsgImpl, can not find the check data time");
		return false;
	}
    
    //签名计算为：$sign = md5($orderid . $uid . $money . $gamemoney . $serverid . $secrect . $mark . $roleid.$time); 
    //当参数$serverid,$mark ,$roleid为空时，不参与签名计算。详见[签名说明]。
    
    //需要签名的数据
    string signatureData = orderidIt->second + uidIt->second + moneyIt->second + gamemoneyIt->second + serveridIt->second + 
    m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::M4399)->paymentKey + markIt->second + roleidIt->second + timeIt->second;
    
     //MD5加密
    char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(signatureData.c_str(), (unsigned int)signatureData.size(), md5Buff);
    
    //校验
    if (strcmp(md5Buff, signatureIt->second.c_str()) != 0)
    {
        OptErrorLog("handleM4399PaymentMsgImpl, check md5 error! send md5 data: %s, md5Encrypt:%s", signatureIt->second.c_str(), md5Buff);
        return false;
    }
    
    // 充值订单信息记录
    const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
    //format = name|id|amount|money|rechargeNo|srcSrvId|reply|uid|result|order|payFee|time
	ThirdRechargeLog(ThirdPlatformType::M4399, "%s|%u|%u|%.2f|%s|%u|reply|%s|%d|%s|%s|%s", thirdInfo.user, 
    thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, thirdInfoIt->first.c_str(), 
    thirdInfo.srcSrvId, uidIt->second.c_str(), 0, orderidIt->second.c_str(), moneyIt->second.c_str(), timeIt->second.c_str());

    // 支付成功才会接收到该信息
	provideRechargeItem(OptSuccess, thirdInfoIt, ThirdPlatformType::M4399, uidIt->second.c_str(), orderidIt->second.c_str());

	return true;
}

//  魅族 Http消息处理
bool CThirdInterface::handleMeiZuPaymentMsg(Connect* conn, const HttpMsgBody& msgBody)
{
	int nCode = handleMeiZuPaymentMsgImpl(msgBody);
	CHttpResponse rsp;
	    
    char buffData[MaxNetBuffSize] = {0};
	snprintf(buffData, MaxNetBuffSize - 1, "{\"code\":%d,\"message\":\"\",\"value\":\"\",\"redirect\":\"\"}", nCode);
    rsp.setContent(buffData);

	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to Mei Zu http, rc = %d, data = %s", rc, rspMsg);
	
	return (nCode != 900000);
}

int CThirdInterface::handleMeiZuPaymentMsgImpl(const HttpMsgBody& msgBody)
{
    int nAbnormalError = 900000;        //异常错误
    int nNotShipped = 120013;           //尚未发货
    int nShippedError = 120014;         //发货失败
    int nOK = 200;                      //发货成功
    
     //交易状态
    ParamKey2Value::const_iterator tradeStatusIt = msgBody.paramKey2Value.find("trade_status");
	if (tradeStatusIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMeiZuPaymentMsgImpl, can not find the check data pay trade status");
		return nAbnormalError;
	}
    
    //签名结果
	ParamKey2Value::const_iterator signIt = msgBody.paramKey2Value.find("sign");
	if (signIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMeiZuPaymentMsgImpl, can not find the check data sign");
		return nAbnormalError;
	}

    //外部订单号
    ParamKey2Value::const_iterator orderIdIt = msgBody.paramKey2Value.find("order_id");
	if (orderIdIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMeiZuPaymentMsgImpl, can not find the check data order id");
		return nAbnormalError;
	}

    //内部订单号
	ParamKey2Value::const_iterator cpOrderIdIt = msgBody.paramKey2Value.find("cp_order_id");
	if (cpOrderIdIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMeiZuPaymentMsgImpl, can not find the check data cp order id");
		return nAbnormalError;
	}

    // 内部订单号信息
    FishCoinOrderIdToInfo::iterator thirdInfoIt;
    if(!getTranscationData(cpOrderIdIt->second.c_str(), thirdInfoIt))
    {
        OptErrorLog("handleMeiZuPaymentMsgImpl, can not find transaction id = %s", cpOrderIdIt->second.c_str());
		return nAbnormalError;
    }
    
    //用户ID
    ParamKey2Value::const_iterator uidIt = msgBody.paramKey2Value.find("uid");
	if (uidIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMeiZuPaymentMsgImpl, can not find the check data uid");
		return nAbnormalError;
	}

    //价格
    ParamKey2Value::const_iterator priceIt = msgBody.paramKey2Value.find("total_price");
	if (priceIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMeiZuPaymentMsgImpl, can not find the check data price");
		return nAbnormalError;
	}
    
    //支付时间
    ParamKey2Value::const_iterator payTimeIt = msgBody.paramKey2Value.find("pay_time");
	if (payTimeIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMeiZuPaymentMsgImpl, can not find the check data pay time");
		return nAbnormalError;
	}

    //appSecret
    auto appSecret = m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::MeiZu)->paymentKey;
    
	map<string, string> signMap;
	for (auto it = msgBody.paramKey2Value.begin(); it != msgBody.paramKey2Value.end(); ++it)
	{
		if(!(it->first == "sign" || it->first == "sign_type") && it->second.size() > 0)
		{
			signMap.insert(map<string, string>::value_type(it->first, it->second));
		}
	}

	string signData;
	for (auto it = signMap.begin(); it != signMap.end(); )
	{
		signData += (it->first + "=" + it->second);
		
		if(++it != signMap.end())
			signData += "&";
		else
		{
			signData += ":";
			signData += appSecret;
		}
	}

    //MD5
    char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(signData.c_str(), (unsigned int)signData.size(), md5Buff);

    //校验
    if (signIt->second != md5Buff)
    {
        OptErrorLog("handleMeiZuPaymentMsgImpl, check md5 error! send md5 data: %s, md5Encrypt:%s", signIt->second.c_str(), md5Buff);
        return nAbnormalError;
    }
    
    //待支付 || 支付中
    if(tradeStatusIt->second == "1" || tradeStatusIt->second == "2")
    {
        OptInfoLog("handleMeiZuPaymentMsgImpl, Wait for payment, order id:%s, trade Status:%s", orderIdIt->second.c_str(), tradeStatusIt->second.c_str());
        return nNotShipped;
    }
    
    // 充值订单信息记录
    const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
    //format = name|id|amount|money|rechargeNo|srcSrvId|reply|uid|result|order|payFee|time
	ThirdRechargeLog(ThirdPlatformType::MeiZu, "%s|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s|%s", thirdInfo.user, 
    thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, thirdInfoIt->first.c_str(), 
    thirdInfo.srcSrvId, uidIt->second.c_str(), tradeStatusIt->second.c_str(), orderIdIt->second.c_str(), priceIt->second.c_str(), payTimeIt->second.c_str());

    // 支付成功才会接收到该信息
	int result = (tradeStatusIt->second == "3" ? 0 : 1);
	provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::MeiZu, uidIt->second.c_str(), orderIdIt->second.c_str());
	if (result != 0)
    {
        removeTransactionData(thirdInfoIt);
        return nShippedError;
    }
    
	return nOK;
}

void CThirdInterface::getThirdRechargeTransactionMeiZu(com_protocol::GetRechargeFishCoinOrderRsp &thirdRsp, const com_protocol::FishCoinInfo* fcInfo, const char *puid)
{
    map<string, string> data;       //map默认排序 签名时需要排序
    
   //app_id
    data.insert(map<string, string>::value_type("app_id", m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::MeiZu)->appId));
    
    //cp_order_id
    data.insert(map<string, string>::value_type("cp_order_id", thirdRsp.order_id()));
    
    //uid
    data.insert(map<string, string>::value_type("uid", puid));
    
    //product_id
    char tmp[MaxNetBuffSize] = {0};
    snprintf(tmp, MaxNetBuffSize - 1, "%d", fcInfo->id());
    data.insert(map<string, string>::value_type("product_id", tmp));
    
    //product_subject
    string product_subject = "购买渔币";
    data.insert(map<string, string>::value_type("product_subject", product_subject));
    
    //product_body
    data.insert(map<string, string>::value_type("product_body", ""));
    
    //product_unit
    data.insert(map<string, string>::value_type("product_unit", ""));
    
    //buy_amount
    data.insert(map<string, string>::value_type("buy_amount", "1"));
    
    //product_per_price
    snprintf(tmp, MaxNetBuffSize - 1, "%.2f", fcInfo->buy_price());
    data.insert(map<string, string>::value_type("product_per_price", tmp));
    
    //total_price
    data.insert(map<string, string>::value_type("total_price", tmp));

    //create_time
	snprintf(tmp, MaxNetBuffSize - 1, "%u", (unsigned int)time(NULL));
    data.insert(map<string, string>::value_type("create_time", tmp));
    
    //pay_type
    data.insert(map<string, string>::value_type("pay_type", "0"));
    
    //user_info
    data.insert(map<string, string>::value_type("user_info", ""));
     
    //签名数据
    string signData;
    for(auto it = data.begin(); it != data.end();)
    {
        signData += (it->first + "=" + it->second);
		
		if(++it != data.end())
			signData += "&";
		else
		{
			signData += ":";
			signData += m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::MeiZu)->paymentKey;
		}
    }
    
     //MD5
    char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(signData.c_str(), (unsigned int)signData.size(), md5Buff);
    
    //sign
    data.insert(map<string, string>::value_type("sign", md5Buff));
    
    //sign_type
    data.insert(map<string, string>::value_type("sign_type", "md5"));
    
    
    for(auto it = data.begin(); it != data.end(); ++it)
    {
        com_protocol::ThirdOrderKeyValue* value = thirdRsp.add_key_value();
        value->set_key(it->first);
        value->set_value(it->second);        
    }
}

// 当乐充值验证消息
bool CThirdInterface::downJoyPaymentMsg(Connect* conn, const RequestHeader& reqHeaderData)
{
	// GET /downjoyRecharge/ChallengeFishing1?
	// order=3735912519735icbaif2l&money=0.01&mid=2469593221172&time=20150720102525&result=1&
	// ext=1.20150621153831.123&signature=d9f1f30fa8b7aa2bcc831312731d1471
	
	/*
	ParamKey2Value::const_iterator extIt = reqHeaderData.paramKey2Value.find("ext");
	if (extIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("receive error down joy http header data, can not find the ext data");
		return false;
	}
	
	FishCoinOrderIdToInfo::iterator fishCoinOrderIt;
	if (!getTranscationData(extIt->second.c_str(), fishCoinOrderIt))
	{
		// 老版本的订单信息
		return m_msgHandler->handleDownJoyPaymentMsg(conn, ud, reqHeaderData);
	}
	*/
	
    ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getUserData(conn);
	bool isOK = downJoyPaymentMsgImpl(ud, reqHeaderData);
	CHttpResponse rsp;
	unsigned int msgLen = 0;
	isOK ? rsp.setContent("success") : rsp.setContent("failure");
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);

	OptInfoLog("send recharge reply message to down joy http, rc = %d, data = %s", rc, rspMsg);
	
	return isOK;
}

bool CThirdInterface::downJoyPaymentMsgImpl(const ConnectUserData* ud, const RequestHeader& reqHeaderData)
{
	// GET /downjoyRecharge/ChallengeFishing1?
	// order=3735912519735icbaif2l&money=0.01&mid=2469593221172&time=20150720102525&result=1&
	// ext=1.20150621153831.123&signature=d9f1f30fa8b7aa2bcc831312731d1471
	
	// 参数校验
	const char* paramList[] = {"ext", "signature", "money", "result", "mid", "order", "time"};
	for (unsigned int idx = 0; idx < (sizeof(paramList) / sizeof(paramList[0])); ++idx)
	{
		if (reqHeaderData.paramKey2Value.find(paramList[idx]) == reqHeaderData.paramKey2Value.end())
		{
			OptErrorLog("receive error down joy http header data, can not find %s data", paramList[idx]);
			return false;
		}
	}
	
    // 内部订单号
	ParamKey2Value::const_iterator extIt = reqHeaderData.paramKey2Value.find("ext");
	FishCoinOrderIdToInfo::iterator fishCoinOrderIt;
	if (!getTranscationData(extIt->second.c_str(), fishCoinOrderIt))
	{
		OptErrorLog("receive error down joy http header data, can not find the fish coin order id, ext = %s", extIt->second.c_str());
		return false;
	}

    // 校验充值订单信息
	const char* param = strchr(ud->reqData, '?');
	const char* signature = strstr(ud->reqData, "&signature=");
	if (param == NULL || signature == NULL || param > signature)
	{
		OptErrorLog("receive error down joy http header, data = %s, len = %d", ud->reqData, ud->reqDataLen);
		return false;
	}

	// 当乐平台MD5加密校验参数
	// order=ok123456&money=5.21&mid=123456&time=20141212105433&result=1&ext=1234567890&key=NIhmYdfPe05f
	char msgBuffer[NFrame::MaxMsgLen];
	unsigned int paramLen = signature - param;
	memcpy(msgBuffer, param + 1, paramLen);  // 待校验参数
	
	// 附加上当乐平台提供的MD5校验KEY
	unsigned int keyLen = snprintf(msgBuffer + paramLen, NFrame::MaxMsgLen - paramLen - 1, "key=%s", m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::DownJoy)->paymentKey);
	char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(msgBuffer, paramLen + keyLen, md5Buff);
	
	ParamKey2Value::const_iterator signatureIt = reqHeaderData.paramKey2Value.find("signature");
	if (strcmp(md5Buff, signatureIt->second.c_str()) != 0)
	{
		OptErrorLog("receive error down joy http header data, the signature md5 is error, signature = %s", msgBuffer);
		return false;
	}

    // 充值订单信息记录
    // format = name|id|amount|money|rechargeNo|srcSrvId|reply|mid|result|order|time
	const ThirdOrderInfo& thirdInfo = fishCoinOrderIt->second;
	ThirdRechargeLog(ThirdPlatformType::DownJoy, "%s|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s", thirdInfo.user, thirdInfo.itemId, thirdInfo.itemAmount,
	thirdInfo.money, fishCoinOrderIt->first.c_str(), thirdInfo.srcSrvId, reqHeaderData.paramKey2Value.at("mid").c_str(), reqHeaderData.paramKey2Value.at("result").c_str(),
	reqHeaderData.paramKey2Value.at("order").c_str(), reqHeaderData.paramKey2Value.at("time").c_str());

    // 充值结果
    int result = (atoi(reqHeaderData.paramKey2Value.at("result").c_str()) == 1) ? OptSuccess : 1;
	provideRechargeItem(result, fishCoinOrderIt, ThirdPlatformType::DownJoy, reqHeaderData.paramKey2Value.at("mid").c_str(), reqHeaderData.paramKey2Value.at("order").c_str());
	
	return true;
}


// 渔币充值处理操作流程
void CThirdInterface::getFishCoinRechargeOrder(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetRechargeFishCoinOrderReq thirdReq;
	if (!m_msgHandler->parseMsgFromBuffer(thirdReq, data, len, "get fish coin recharge order")) return;

    com_protocol::GetMallFishCoinInfoReq getMallFishCoinInfoReq;
	getMallFishCoinInfoReq.set_id(thirdReq.fish_coin_id());
	getMallFishCoinInfoReq.set_cb_flag(srcSrvId);
	getMallFishCoinInfoReq.set_cb_data(data, len);
	getMallFishCoinInfoReq.set_platform_id(thirdReq.platform_id());
	const char* msgData = m_msgHandler->serializeMsgToBuffer(getMallFishCoinInfoReq, "get mall fish coin information request");
	if (msgData != NULL)
	{							
		m_msgHandler->sendMessageToCommonDbProxy(msgData, getMallFishCoinInfoReq.ByteSize(), BUSINESS_GET_MALL_FISH_COIN_INFO_REQ, m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
	}
}

// 渔币充值处理操作流程
void CThirdInterface::getFishCoinInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetMallFishCoinInfoRsp getMallFishCoinInfoRsp;
	if (!m_msgHandler->parseMsgFromBuffer(getMallFishCoinInfoRsp, data, len, "get mall fish coin information reply")) return;
	
	if (getMallFishCoinInfoRsp.result() != OptSuccess)
	{
		OptErrorLog("get mall fish coin information error, user = %s, result = %d", m_msgHandler->getContext().userData, getMallFishCoinInfoRsp.result());
		return;
	}
	
	com_protocol::GetRechargeFishCoinOrderReq thirdReq;
	if (!m_msgHandler->parseMsgFromBuffer(thirdReq, getMallFishCoinInfoRsp.cb_data().c_str(), getMallFishCoinInfoRsp.cb_data().length(), "get fish coin recharge order")) return;
    
	// 获取订单信息
	srcSrvId = getMallFishCoinInfoRsp.cb_flag();
	char rechargeTranscation[MaxRechargeTransactionLen] = {'\0'};
	com_protocol::GetRechargeFishCoinOrderRsp thirdRsp;
	thirdRsp.set_result(SrvRetCode::ParamValueError);
	if (getThirdOrderId(thirdReq.platform_id(), thirdReq.os_type(), srcSrvId, rechargeTranscation, MaxRechargeTransactionLen))
	{
		const com_protocol::FishCoinInfo* fcInfo = &(getMallFishCoinInfoRsp.fish_info());  // 获取物品信息
		unsigned int amount = fcInfo->num();
		if (getMallFishCoinInfoRsp.status() == 1)  // 已经购买过了
		{
			amount += fcInfo->gift();
		}
		else
		{
			amount = fcInfo->first_amount();
		}
		
		thirdRsp.set_result(OptSuccess);
		thirdRsp.set_order_id(rechargeTranscation);
		thirdRsp.set_fish_coin_id(thirdReq.fish_coin_id());
		thirdRsp.set_fish_coin_amount(amount);
		thirdRsp.set_money(fcInfo->buy_price());
		
		// 存储账单数据
		const ThirdOrderInfo* thirdOrderInfo = saveTransactionData(m_msgHandler->getContext().userData, srcSrvId, thirdReq.platform_id(), rechargeTranscation, thirdRsp);
		if (thirdOrderInfo == NULL)
		{
			thirdRsp.set_result(SrvRetCode::SaveTransactionError);
		}
		
		else if (thirdReq.platform_id() == ThirdPlatformType::QiHu360   // 360平台额外数据
				 || thirdReq.platform_id() == ThirdPlatformType::UCSDK 	// UC平台
				 || thirdReq.platform_id() == ThirdPlatformType::Oppo)
		{
			// 添加支付的回调地址
			com_protocol::ThirdOrderKeyValue* value = thirdRsp.add_key_value();
			value->set_key("url");
			
			char strUrl[MaxNetBuffSize] = {0};
			snprintf(strUrl, MaxNetBuffSize - 1, "http://%s:%s%s", CCfg::getValue("NetConnect", "IP"), CCfg::getValue("NetConnect", "Port"),
			m_platformatOpt->getThirdPlatformCfg(thirdReq.platform_id())->paymentUrl);
			value->set_value(strUrl);
		}
		
		else if(thirdReq.platform_id() == ThirdPlatformType::MeiZu)
		{
			getThirdRechargeTransactionMeiZu(thirdRsp, fcInfo, m_msgHandler->getContext().userData);
		}
		
		else if(thirdReq.platform_id() == ThirdPlatformType::TXQQ 
			 || thirdReq.platform_id() == ThirdPlatformType::TXWX)
		{
			string openid, openkey, pf, pfkey;
			for (int i = 0; i < thirdReq.key_value_size(); ++i)
			{
				const com_protocol::ThirdOrderKeyValue &kv = thirdReq.key_value(i);
				if(kv.key() == "openid")    openid = kv.value();
				if(kv.key() == "openkey")   openkey = kv.value();
				if(kv.key() == "pf")        pf = kv.value();
				if(kv.key() == "pfkey")     pfkey = kv.value();
			}
			
			// 查询腾讯充值前托管的游戏币数量，保存在订单信息里，用于充值后数量变化比较，以便确认充值是否成功
			if (m_platformatOpt->updateTXServerUserCurrencyData((ThirdPlatformType)thirdReq.platform_id(), openid.c_str(), openkey.c_str(), pf.c_str(), 
			pfkey.c_str(), srcSrvId, thirdRsp.order_id().c_str())) return;
			
			// 查询腾讯托管的渔币操作错误
			removeTransactionData(rechargeTranscation);
			thirdRsp.Clear();
			thirdRsp.set_result(SrvRetCode::UpdateTxServerDataError);
		}
		
		else if (thirdReq.platform_id() == ThirdPlatformType::Vivo)
		{
			if (getVivoOrderInfo(srcSrvId, thirdRsp.order_id().c_str(), thirdRsp.money())) return;
			
			removeTransactionData(rechargeTranscation);
			thirdRsp.Clear();
			thirdRsp.set_result(SrvRetCode::GetVivoOrderError);
		}
		
		else if (thirdReq.platform_id() == ThirdPlatformType::MiGu)
		{
			com_protocol::ThirdOrderKeyValue* value = thirdRsp.add_key_value();
		    value->set_key("mgChargingId");
			value->set_value(fcInfo->order_charging_id());
		    // value->set_value(fcInfo->mg_charging_id());
		}
		
		else if (thirdReq.platform_id() == ThirdPlatformType::WoGame)
		{
			com_protocol::ThirdOrderKeyValue* value = thirdRsp.add_key_value();
		    value->set_key("woGameChargingId");
			value->set_value(fcInfo->order_charging_id());
		    // value->set_value(fcInfo->wogame_charging_id());
		}
		
		if (fcInfo->has_order_charging_id())
		{
			com_protocol::ThirdOrderKeyValue* value = thirdRsp.add_key_value();
		    value->set_key("orderChargingId");
			value->set_value(fcInfo->order_charging_id());
		}
	}
	else
	{
		OptErrorLog("get fish coin order id error, user = %s, platform id = %u, os type = %u", m_msgHandler->getContext().userData, thirdReq.platform_id(), thirdReq.os_type());
	} 

    int rc = -1;
    const char* msgData = m_msgHandler->serializeMsgToBuffer(thirdRsp, "send fish coin recharge order to client");
	if (msgData != NULL)
	{							
		rc = m_msgHandler->sendMessage(msgData, thirdRsp.ByteSize(), srcSrvId, GET_FISH_COIN_RECHARGE_ORDER_RSP);
		if (rc == Success && thirdRsp.result() == OptSuccess)
		{
			// 充值订单信息记录
			// format = name|id|amount|money|rechargeNo|request|platformType|srcSrvId
			ThirdRechargeLog(thirdReq.platform_id(), "%s|%u|%u|%.2f|%s|request|%u|%u", m_msgHandler->getContext().userData, thirdRsp.fish_coin_id(), thirdRsp.fish_coin_amount(), thirdRsp.money(),
			thirdRsp.order_id().c_str(), thirdReq.platform_id(), srcSrvId);
		}
	}
	
	// 发送失败则删除订单数据
	if (rc != Success && thirdRsp.result() == OptSuccess) removeTransactionData(rechargeTranscation);
	
	OptInfoLog("get fish coin recharge order record, srcSrvId = %u, user = %s, money = %.2f, order id = %s, platform id = %u, ostype = %u, fish coin id = %u, result = %d, rc = %d",
		       srcSrvId, m_msgHandler->getContext().userData, thirdRsp.money(), thirdRsp.order_id().c_str(),
			   thirdReq.platform_id(), thirdReq.os_type(), thirdReq.fish_coin_id(), thirdRsp.result(), rc);
}

void CThirdInterface::cancelFishCoinRechargeOrder(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CancelRechargeFishCoinNotify cancelThirdNotify;
	if (!m_msgHandler->parseMsgFromBuffer(cancelThirdNotify, data, len, "cancel fish coin recharge order notify")) return;
	
	removeTransactionData(cancelThirdNotify.order_id().c_str());  // 删除订单数据
	
	// 充值订单信息记录
	// format = name|rechargeNo|cancel|platformType|srcSrvId|reason|info
	ThirdRechargeLog(cancelThirdNotify.platform_id(), "%s|%s|cancel|%u|%u|%d|%s",
	m_msgHandler->getContext().userData, cancelThirdNotify.order_id().c_str(), cancelThirdNotify.platform_id(), srcSrvId, cancelThirdNotify.reason(), cancelThirdNotify.info().c_str());
	
	OptInfoLog("cancel fish coin recharge order, srcSrvId = %u, user = %s, order id = %s, platform id = %d, reason = %d, info = %s",
	srcSrvId, m_msgHandler->getContext().userData, cancelThirdNotify.order_id().c_str(), cancelThirdNotify.platform_id(), cancelThirdNotify.reason(), cancelThirdNotify.info().c_str());
}

// 渔币充值发放物品
void CThirdInterface::provideRechargeItem(int result, FishCoinOrderIdToInfo::iterator thirdInfoIt, ThirdPlatformType pType, const char* uid, const char* thirdOtherData)
{
    const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
	if (result == OptSuccess)
	{
		// 充值成功，发放物品，充值信息入库
		com_protocol::UserRechargeReq userRechargeReq;
		userRechargeReq.set_bills_id(thirdInfoIt->first.c_str());
		userRechargeReq.set_username(thirdInfo.user);
		userRechargeReq.set_item_type(EGameGoodsType::EFishCoin);
		userRechargeReq.set_item_id(thirdInfo.itemId);
		userRechargeReq.set_item_count(1);
		userRechargeReq.set_recharge_rmb(thirdInfo.money);
		userRechargeReq.set_third_type(pType);
		userRechargeReq.set_third_account(uid);
		userRechargeReq.set_third_other_data(thirdOtherData);
	    userRechargeReq.set_item_flag(0);
		
	    // 是否首充标志
		// unsigned int flag = m_msgHandler->getMallMgr().fishCoinIsRecharged(userRechargeReq.username().c_str(), userRechargeReq.username().length(), thirdInfo.itemId) ? 0 : MallItemFlag::First;
		
		const char* msgData = m_msgHandler->serializeMsgToBuffer(userRechargeReq, "send third platform fish coin recharge result to db proxy");
		if (msgData != NULL)
		{							
			int rc = m_msgHandler->sendMessageToCommonDbProxy(msgData, userRechargeReq.ByteSize(), BUSINESS_USER_RECHARGE_REQ, userRechargeReq.username().c_str(), userRechargeReq.username().length());
			OptInfoLog("send third platform fish coin payment result to db proxy, rc = %d, user name = %s, bill id = %s", rc, userRechargeReq.username().c_str(), userRechargeReq.bills_id().c_str());
		}
	}
	else
	{
		// 充值失败则不会发放物品，直接通知客户端
		com_protocol::RechargeFishCoinResultNotify fishCoinRechargeResultNotify;
		fishCoinRechargeResultNotify.set_result(1);
		fishCoinRechargeResultNotify.set_order_id(thirdInfoIt->first.c_str());
		fishCoinRechargeResultNotify.set_fish_coin_id(thirdInfo.itemId);
		fishCoinRechargeResultNotify.set_fish_coin_amount(thirdInfo.itemAmount);
		fishCoinRechargeResultNotify.set_money(thirdInfo.money);
		fishCoinRechargeResultNotify.set_give_count(0);

		const char* msgData = m_msgHandler->serializeMsgToBuffer(fishCoinRechargeResultNotify, "send third platform fish coin recharge result to client");
		if (msgData != NULL)
		{							
			int rc = m_msgHandler->sendMessage(msgData, fishCoinRechargeResultNotify.ByteSize(), thirdInfo.user, strlen(thirdInfo.user), thirdInfo.srcSrvId, FISH_COIN_RECHARGE_NOTIFY);
			OptWarnLog("send third platform fish coin payment result to client, rc = %d, user name = %s, result = %d, bill id = %s",
			rc, thirdInfo.user, result, thirdInfoIt->first.c_str());
		}
		
		// removeTransactionData(thirdInfoIt);  // 先不要删除，可能存在第三方通知错误，重新发起消息即可，否则订单超时会自动删除
	}
}

// 第三方平台渔币充值结果处理
bool CThirdInterface::fishCoinRechargeItemReply(const com_protocol::UserRechargeRsp& userRechargeRsp)
{
	const com_protocol::UserRechargeReq& userRechargeInfo = userRechargeRsp.info();
	FishCoinOrderIdToInfo::iterator thirdInfoIt;
	if (!getTranscationData(userRechargeInfo.bills_id().c_str(), thirdInfoIt))
	{
		OptErrorLog("can not find the fish coin recharge info, user = %s, bill id = %s, money = %.2f, result = %d",
		m_msgHandler->getContext().userData, userRechargeInfo.bills_id().c_str(), userRechargeInfo.recharge_rmb(), userRechargeRsp.result());

		return false;
	}

    const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
    com_protocol::RechargeFishCoinResultNotify fishCoinRechargeResultNotify;
	fishCoinRechargeResultNotify.set_result(userRechargeRsp.result());
	fishCoinRechargeResultNotify.set_order_id(thirdInfoIt->first.c_str());
	fishCoinRechargeResultNotify.set_fish_coin_id(thirdInfo.itemId);
	fishCoinRechargeResultNotify.set_fish_coin_amount(userRechargeRsp.item_amount());
	fishCoinRechargeResultNotify.set_money(thirdInfo.money);
	
	unsigned int giftCount = (userRechargeRsp.gift_array_size() > 0) ? userRechargeRsp.gift_array(0).num() : 0;
	fishCoinRechargeResultNotify.set_give_count(giftCount);

	int rc = -1;
	const char* msgBuff = m_msgHandler->serializeMsgToBuffer(fishCoinRechargeResultNotify, "send fish coin recharge result to client");
	if (msgBuff != NULL) rc = m_msgHandler->sendMessage(msgBuff, fishCoinRechargeResultNotify.ByteSize(), thirdInfo.srcSrvId, FISH_COIN_RECHARGE_NOTIFY);

    // 充值订单信息记录最终结果
    // format = name|id|amount|flag|money|rechargeNo|srcSrvId|finish|umid|result|order|giftCount
	ThirdRechargeLog(userRechargeInfo.third_type(), "%s|%u|%u|%u|%.2f|%s|%u|finish|%s|%d|%s|%u", thirdInfo.user, thirdInfo.itemId, userRechargeRsp.item_amount(),
	userRechargeInfo.item_flag(), userRechargeInfo.recharge_rmb(), thirdInfoIt->first.c_str(), thirdInfo.srcSrvId, userRechargeInfo.third_account().c_str(),
	userRechargeRsp.result(), userRechargeInfo.third_other_data().c_str(), giftCount);

	OptInfoLog("send fish coin recharge result to client, rc = %d, srcSrvId = %u, user name = %s, result = %d, bill id = %s, money = %.2f, uid = %s, trade = %s",
	rc, thirdInfo.srcSrvId, userRechargeInfo.username().c_str(), userRechargeRsp.result(), userRechargeInfo.bills_id().c_str(), userRechargeInfo.recharge_rmb(),
	userRechargeInfo.third_account().c_str(), userRechargeInfo.third_other_data().c_str());
	
	removeTransactionData(thirdInfoIt);  // 充值操作处理完毕，删除该充值信息
	return true;
}

void CThirdInterface::checkUnHandleOrderData()
{
	// 未处理订单最大时间间隔，超过配置的间隔时间还未处理的订单数据则直接删除
	unsigned int deleteSecs = time(NULL) - atoi(CCfg::getValue("HttpService", "UnHandleTransactionMaxIntervalSeconds"));
	for (FishCoinOrderIdToInfo::iterator it = m_fishCoinOrder2Info.begin(); it != m_fishCoinOrder2Info.end();)
	{
		if (it->second.timeSecs < deleteSecs)
		{
			// 充值订单信息记录，超时取消
			// format = name|id|amount|money|rechargeNo|srcSrvId|timeout
			const ThirdOrderInfo& thirdInfo = it->second;
			ThirdRechargeLog(thirdInfo.platformId, "%s|%u|%u|%.2f|%s|%u|timeout",
			thirdInfo.user, thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, it->first.c_str(), thirdInfo.srcSrvId);

			time_t createTimeSecs = it->second.timeSecs;
	        struct tm* tmval = localtime(&createTimeSecs);
			OptWarnLog("remove time out unhandle order data, date = %d-%02d-%02d %02d:%02d:%02d, user = %s, order id = %s, fish coin id = %u, amount = %u, money = %.2f, srcSrvId = %u",
			tmval->tm_year + 1900, tmval->tm_mon + 1, tmval->tm_mday, tmval->tm_hour, tmval->tm_min, tmval->tm_sec,
			thirdInfo.user, it->first.c_str(), thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, thirdInfo.srcSrvId);
			
			removeTransactionData(it++);
		}
		else
		{
			++it;
		}
	}
}

const ThirdOrderInfo* CThirdInterface::saveTransactionData(const char* userName, const unsigned int srcSrvId, const unsigned int platformId,
													       const char* transactionId, const com_protocol::GetRechargeFishCoinOrderRsp& thirdRsp)
{
	if (thirdRsp.result() != OptSuccess || thirdRsp.ByteSize() > (int)MaxRechargeTransactionLen) return NULL;

	// 订单数据格式 : id.amount.money-srv.user-platformId.time
	// 4003.600.0.98-1001.10000001
	char rechargeData[MaxRechargeTransactionLen] = {'\0'};
	const unsigned int currentTimeSecs = time(NULL);
	unsigned int dataLen = snprintf(rechargeData, MaxRechargeTransactionLen - 1, "%u.%u.%.2f-%u.%s-%u.%u", thirdRsp.fish_coin_id(), thirdRsp.fish_coin_amount(), thirdRsp.money(), srcSrvId, userName, platformId, currentTimeSecs);
	
	// 订单信息临时保存到数据库，防止服务异常退出导致订单数据丢失
	com_protocol::SetGameDataReq saveOrderReq;
	saveOrderReq.set_primary_key(FishCoinRechargeKey, FishCoinRechargeKeyLen);
	saveOrderReq.set_second_key(transactionId);
	saveOrderReq.set_data(rechargeData, dataLen);
	int rc = m_msgHandler->sendMessageToService(ServiceType::GameSrvDBProxy, saveOrderReq, GameServiceDBProtocol::SET_GAME_DATA_REQ, "", 0);
    if (rc != OptSuccess)
	{
		OptErrorLog("save fish coin order data to redis logic service error, rc = %d", rc);
		return NULL;
	}
	
	// 内存存储
	m_fishCoinOrder2Info[transactionId] = ThirdOrderInfo();
	ThirdOrderInfo& info = m_fishCoinOrder2Info[transactionId];
	strncpy(info.user, userName, MaxUserNameLen - 1);
	info.timeSecs = currentTimeSecs;
	info.srcSrvId = srcSrvId;
	info.platformId = platformId;
	info.itemId = thirdRsp.fish_coin_id();
	info.itemAmount = thirdRsp.fish_coin_amount();
	info.money = thirdRsp.money();
	info.userCbFlag = 0;
	
	return &info;
}

// 获取第三方平台订单ID
bool CThirdInterface::getThirdOrderId(const unsigned int platformType, const unsigned int osType, const unsigned int srcSrvId, char* transactionId, unsigned int len)
{
	const unsigned int MaxThirdPlatformTransactionLen = 32;
	if (len < MaxThirdPlatformTransactionLen) return false;

    if (platformType >= ThirdPlatformType::MaxPlatformType)
	{
		OptErrorLog("get third order id, invalid platform type = %u", platformType);
		return false;
	}
	
    unsigned int osPlatformType = platformType;
	if (osType == ClientVersionType::AppleMerge)  // 苹果版本
	{
		osPlatformType = platformType * ThirdPlatformType::ClientDeviceBase + platformType;  // 转换为苹果版本值
	}
	else if (osType != ClientVersionType::AndroidMerge)  // 非安卓合并版本
	{
		OptErrorLog("get third order id, invalid os type = %u", osType);
		return false;
	}

	// 订单号格式 : platformTtimeIindexSsrcSrvId
	// 3T1458259523I23S4001
	const unsigned int MaxOrderIdIndex = 10000;
	const unsigned int curSecs = time(NULL);
	unsigned int idLen = snprintf(transactionId, len - 1, "%uT%uI%uS%u", osPlatformType, curSecs, ++m_orderIndex[platformType] % MaxOrderIdIndex, srcSrvId);
	if (ThirdPlatformType::WoGame == platformType)      // 联通沃商店
	{
		const unsigned int WOGAMEORDERLEN = 24;  // 订单号必须为24位
		if (idLen < WOGAMEORDERLEN)
		{
			strncat(transactionId, "00000000000000000000", WOGAMEORDERLEN - idLen);
			idLen = WOGAMEORDERLEN;
		}
	}
	
	else if (ThirdPlatformType::Baidu == platformType)  // 百度
	{
		// 百度特殊处理
		// 百度订单号索引，百度规定订单号长度不能超过11位
		if (curSecs > m_baiduOrderIndex) m_baiduOrderIndex = curSecs;
		else ++m_baiduOrderIndex;
		
		idLen = snprintf(transactionId, len - 1, "%u", m_baiduOrderIndex);
	}
	
	else if (ThirdPlatformType::MiGu == platformType)   // 咪咕
	{
		// 该参数订单号ID 非空时必须16个字节，仅支持字母大小写、数字和9个特殊字符 ! # * / = + - , . 其中不能含有空格
		static unsigned int MiGuOrderIndex = 0;
		const unsigned int MiGuMaxIndex = 100000;
		idLen = snprintf(transactionId, len - 1, "%uM%05u", curSecs, ++MiGuOrderIndex % MiGuMaxIndex);
	}

	transactionId[idLen] = 0;

	return true;
}

bool CThirdInterface::getTranscationData(const char* orderId, FishCoinOrderIdToInfo::iterator& it)
{
    it = m_fishCoinOrder2Info.find(orderId);
    return it != m_fishCoinOrder2Info.end();
}

void CThirdInterface::removeTransactionData(const char* transactionId)
{
	removeTransactionData(m_fishCoinOrder2Info.find(transactionId));
}

void CThirdInterface::removeTransactionData(FishCoinOrderIdToInfo::iterator it)
{
	if (it != m_fishCoinOrder2Info.end())
	{
		com_protocol::DelGameDataReq delOrderReq;
		delOrderReq.set_primary_key(FishCoinRechargeKey, FishCoinRechargeKeyLen);
		delOrderReq.add_second_keys(it->first.c_str(), it->first.length());
		int rc = m_msgHandler->sendMessageToService(ServiceType::GameSrvDBProxy, delOrderReq, GameServiceDBProtocol::DEL_GAME_DATA_REQ, "", 0);	
		OptInfoLog("remove fish coin order data, id = %s, result = %d", it->first.c_str(), rc);

		m_fishCoinOrder2Info.erase(it);
	}
}

void CThirdInterface::saveOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetGameDataRsp setRsp;
	if (!m_msgHandler->parseMsgFromBuffer(setRsp, data, len, "save fish coin order data reply")) return;
	
	if (setRsp.result() != 0) OptErrorLog("save fish coin order data error, key = %s, result = %d", setRsp.second_key().c_str(), setRsp.result());
}

void CThirdInterface::removeOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::DelGameDataRsp delRsp;
	if (!m_msgHandler->parseMsgFromBuffer(delRsp, data, len, "remove fish coin order data reply")) return;
	
	if (delRsp.result() != 0)
	{
		const char* key = (delRsp.second_keys_size() > 0) ? delRsp.second_keys(0).c_str() : "";
		OptErrorLog("remove fish coin order data error, key = %s, result = %d", key, delRsp.result());
	}
}

bool CThirdInterface::parseUnHandleOrderData()
{	
	// 获取之前没有处理完毕的订单数据，一般该数据为空
	// 当且仅当用户充值流程没走完并且此种情况下服务异常退出时才会存在未处理完毕的订单数据
	// 注意：用户请求订单，但一直没有取消订单的情况下，该订单会一直存在直到超时被删除
	com_protocol::GetMoreKeyGameDataReq getFishCoinRechargeDataReq;
	getFishCoinRechargeDataReq.set_primary_key(FishCoinRechargeKey, FishCoinRechargeKeyLen);
	int rc = m_msgHandler->sendMessageToService(ServiceType::GameSrvDBProxy, getFishCoinRechargeDataReq, GameServiceDBProtocol::GET_MORE_KEY_GAME_DATA_REQ, "", 0);
    if (rc != OptSuccess)
	{
		OptErrorLog("get no handle fish coin order data error, rc = %d", rc);
		return false;
	}
	
	return true;
}
	
void CThirdInterface::getUnHandleOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 获取之前没有处理完毕的订单数据，一般该数据为空
	// 当且仅当用户充值流程没走完并且此种情况下服务异常退出时才会存在未处理完毕的订单数据
	// 注意：用户请求订单，但一直没有取消订单的情况下，该订单会一直存在直到超时被删除
    com_protocol::GetMoreKeyGameDataRsp getFishCoinRechargeDataRsp;
	if (!m_msgHandler->parseMsgFromBuffer(getFishCoinRechargeDataRsp, data, len, "get unhandle fish coin order data reply")) return;
	
	if (getFishCoinRechargeDataRsp.result() != 0)
	{
		OptErrorLog("get unhandle fish coin order data reply error, result = %d", getFishCoinRechargeDataRsp.result());
		return;
	}
	
	OptInfoLog("get unhandle fish coin order data reply, unhandle count = %d", getFishCoinRechargeDataRsp.second_key_data_size());
	
	for (int idx = 0; idx < getFishCoinRechargeDataRsp.second_key_data_size(); ++idx)
	{
		const com_protocol::SecondKeyGameData& skGameData = getFishCoinRechargeDataRsp.second_key_data(idx);
		const char* transactionId = skGameData.key().c_str();  // 订单ID值
		const char* transactionData = skGameData.data().c_str();    // 订单数据内容
		m_fishCoinOrder2Info[transactionId] = ThirdOrderInfo();
	    ThirdOrderInfo& info = m_fishCoinOrder2Info[transactionId];
		if (!parseOrderData(transactionId, transactionData, info))
		{
			m_fishCoinOrder2Info.erase(transactionId);
			
			OptErrorLog("have unhandle fish coin order data error, id = %s, data = %s", transactionId, transactionData);
		}
	}
	
	checkUnHandleOrderData();
}

bool CThirdInterface::parseOrderData(const char* transactionId, const char* transactionData, ThirdOrderInfo& transcationInfo)
{
	// 订单号格式 : platformTtimeIindexSsrcSrvId
	// 3T1466042162I123S4001
	char transactionInfo[MaxRechargeTransactionLen] = {'\0'};
	strncpy(transactionInfo, transactionId, MaxRechargeTransactionLen - 1);
	
	unsigned int platformId = ThirdPlatformType::MaxPlatformType;
	unsigned int timeSecs = 0;
	char* platformInfo = (char*)strchr(transactionInfo, 'T');  // 平台类型ID值
	if (platformInfo != NULL)  // 老订单号格式
	{
		*platformInfo = '\0';
		platformId = atoi(transactionInfo);
		
		char* idTimeInfo = (char*)strchr(++platformInfo, 'I');  // 订单号时间戳
		if (idTimeInfo == NULL)
		{
			OptErrorLog("parse fish coin order id error, can not find the time info, id = %s", transactionId);
			return false;
		}
		*idTimeInfo = '\0';
		timeSecs = atoi(platformInfo);
    }
	
	// 订单数据格式 : id.amount.money-srv.user-platformId.time
	// 4003.600.0.98-1001.10000001-26.1466042162
	enum EFishCoinInfo
	{
		Eid = 0,
		Eamount = 1,
		Esize = 2,
	};
	unsigned int itemValue[EFishCoinInfo::Esize];
	strncpy(transactionInfo, transactionData, MaxRechargeTransactionLen - 1);
	const char* itemInfo = transactionInfo;
	for (int i = 0; i < EFishCoinInfo::Esize; ++i)
	{
		char* valueEnd = (char*)strchr(itemInfo, '.');
		if (*itemInfo == '-' || *itemInfo == '\0' || valueEnd == NULL)
		{
			OptErrorLog("parse fish coin order data error, can not find the goods info, data = %s", transactionData);
			return false;
		}
		
		*valueEnd = '\0';
		itemValue[i] = atoi(itemInfo);
		
		itemInfo = ++valueEnd;
	}
	
	// 充值价格
    char* moneyInfo = (char*)strchr(itemInfo, '-');
	if (moneyInfo == NULL)
	{
		OptErrorLog("parse fish coin order data error, can not find the money info, data = %s", transactionData);
		return false;
	}
	*moneyInfo = '\0';
	float money = atof(itemInfo);
	
	// 用户信息
	char* uName = (char*)strchr(++moneyInfo, '.');
	if (uName == NULL)
	{
		OptErrorLog("parse fish coin order data error, can not find the user name info, data = %s", transactionData);
		return false;
	}
	*uName = '\0';
	unsigned int srcSrvId = atoi(moneyInfo);
	++uName;
	
	// 新的订单号数据格式
	// srv.user-platformId.time
	// 平台类型ID值
	platformInfo = (char*)strchr(uName, '-');
	if (platformInfo != NULL)
	{
		*platformInfo = '\0';  // uName user 结束符，结束点
		char* dataTimeInfo = (char*)strchr(++platformInfo, '.');  // 订单数据时间戳
		if (dataTimeInfo == NULL)
		{
			OptErrorLog("parse fish coin order data error, can not find the time info, data = %s", transactionData);
			return false;
		}
		*dataTimeInfo = '\0';  // platformId 结束符，结束点
		platformId = atoi(platformInfo);
		timeSecs = atoi(++dataTimeInfo);
	}
	
	if (platformId >= ThirdPlatformType::MaxPlatformType || timeSecs < 1)
	{
		OptErrorLog("parse fish coin order data error, can not find the platform or time info, platform = %d, time = %u, order id = %s, data = %s",
		platformId, timeSecs, transactionId, transactionData);
		return false;
	}
	
	strncpy(transcationInfo.user, uName, MaxUserNameLen - 1);
	transcationInfo.timeSecs = timeSecs;
	transcationInfo.srcSrvId = srcSrvId;
	transcationInfo.platformId = platformId;
	transcationInfo.itemId = itemValue[EFishCoinInfo::Eid];
	transcationInfo.itemAmount = itemValue[EFishCoinInfo::Eamount];
	transcationInfo.money = money;
	transcationInfo.userCbFlag = 0;

	time_t createTimeSecs = timeSecs;
	struct tm* tmval = localtime(&createTimeSecs);
	OptInfoLog("parse fish coin order data, date = %d-%02d-%02d %02d:%02d:%02d, user name = %s, platform = %u, time = %u, id = %u, amount = %u, money = %.2f, srcSrvId = %u, order id = %s, data = %s", tmval->tm_year + 1900, tmval->tm_mon + 1, tmval->tm_mday, tmval->tm_hour, tmval->tm_min, tmval->tm_sec, uName, platformId, timeSecs,
	itemValue[EFishCoinInfo::Eid], itemValue[EFishCoinInfo::Eamount], money, srcSrvId, transactionId, transactionData);
	
	return true;
}


// 获取vivo订单信息
bool CThirdInterface::getVivoOrderInfo(const unsigned int srcSrvId, const char* orderId, const float price)
{
    const ThirdPlatformConfig* pThird = m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::Vivo);
    if(pThird == NULL)
    {
        OptErrorLog("get vivo platform config error, type = %d, orderId = %s", ThirdPlatformType::Vivo, orderId);
        return false;
    }

    // 请求参数
    map<string, string> parameter;
    parameter.insert(pair<string, string>("version", "1.0.0"));
    parameter.insert(pair<string, string>("storeId", pThird->appKey));
    parameter.insert(pair<string, string>("appId", pThird->appId));
    parameter.insert(pair<string, string>("storeOrder", orderId));
    parameter.insert(pair<string, string>("notifyUrl", "http://120.25.13.106/api/pay/vivo/Vivo.php"));
	
	parameter.insert(pair<string, string>("orderTitle", "YuBi"));
	parameter.insert(pair<string, string>("orderDesc", "AoRongBuYu"));
	
	char dataBuffer[MaxNetBuffSize] = {0};
	snprintf(dataBuffer, sizeof(dataBuffer) - 1, "%.2f", price);
	parameter.insert(pair<string, string>("orderAmount", dataBuffer));
	
	const time_t curSecs = time(NULL);
	struct tm curTime;
	localtime_r(&curSecs, &curTime);
	snprintf(dataBuffer, sizeof(dataBuffer) - 1, "%d%02d%02d%02d%02d%02d", curTime.tm_year + 1900, curTime.tm_mon + 1, curTime.tm_mday, curTime.tm_hour, curTime.tm_min, curTime.tm_sec);
    parameter.insert(pair<string, string>("orderTime", dataBuffer));
    
    // 校验参数
    string reqParameter;
    for(auto it = parameter.begin(); it != parameter.end(); ++it)
    {
        reqParameter += (it->first + "=" + it->second + "&");
    }
	
	md5Encrypt(pThird->paymentKey, strlen(pThird->paymentKey), dataBuffer);
	reqParameter += dataBuffer;
	
	// signature=md5_hex(key1=value1&key2=value2&...&keyn=valuen&to_lower_case(md5_hex(App-key)))
	md5Encrypt(reqParameter.c_str(), reqParameter.length(), dataBuffer);
	
	parameter.insert(pair<string, string>("signMethod", "MD5"));
	parameter.insert(pair<string, string>("signature", dataBuffer));
	
	OptInfoLog("vivo param value = %s, md5 = %s, orderId = %s", reqParameter.c_str(), dataBuffer, orderId);

	// 请求参数
    reqParameter.clear();
	unsigned int encodeBufferLen = 0;
    for(auto it = parameter.begin(); it != parameter.end(); ++it)
    {
		encodeBufferLen = MaxNetBuffSize - 1;
		if(!urlEncode(it->second.c_str(), it->second.length(), dataBuffer, encodeBufferLen))
		{
			OptErrorLog("get vivo order info, encode param error, param = %s, orderId = %s", it->second.c_str(), orderId);
			
            return false;
		}
        reqParameter += (it->first + "=" + dataBuffer + "&");
    }

    // 向vivo服务器获取订单号ID
    CHttpRequest httpRequest(HttpMsgType::POST);
	httpRequest.setParam(reqParameter.c_str(), reqParameter.length() - 1); // 注意！需要去掉最后一个 & 字符
	
	VivoOrderInfo vivoOrderInfo;
	vivoOrderInfo.flag = 0;
	strncpy(vivoOrderInfo.orderId, orderId, MaxRechargeTransactionLen - 1);
	
	if (!m_msgHandler->doHttpConnect(ThirdPlatformType::Vivo, srcSrvId, httpRequest, HttpReplyHandlerID::RIDGetVivoOrderInfo, pThird->url, &vivoOrderInfo, sizeof(vivoOrderInfo)))
	{
		OptErrorLog("get vivo order info do http connect error, orderId = %s", orderId);
		
		return false;
	}
	
	return true;
}

bool CThirdInterface::getVivoOrderInfoReply(ConnectData* cd)
{
	const VivoOrderInfo* vivoOrderInfo = (const VivoOrderInfo*)cd->cbData;
	com_protocol::GetRechargeFishCoinOrderRsp thirdRsp;
	ParamKey2Value paramKey2Value;
	ParamKey2Value::const_iterator vivoOrder;
	
    while (true)
	{
	    CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);

        thirdRsp.set_result(SrvRetCode::GetVivoOrderError);
		ParamKey2Value::const_iterator respCode = paramKey2Value.find("respCode");
		if (respCode == paramKey2Value.end())
		{
			OptErrorLog("get vivo order info reply but can not find the respCode, orderId = %s", vivoOrderInfo->orderId);
			break;
		}
		
		ParamKey2Value::const_iterator respMsg = paramKey2Value.find("respMsg");
		if (respMsg == paramKey2Value.end())
		{
			OptErrorLog("get vivo order info reply but can not find the respMsg, orderId = %s", vivoOrderInfo->orderId);
			break;
		}
		
		// 失败则通知客户端请求订单失败
		if(respCode->second != "200")
		{
			OptErrorLog("get vivo order info reply error, respMsg = %s, respCode = %s, orderId = %s", respMsg->second.c_str(), respCode->second.c_str(), vivoOrderInfo->orderId);
			break;
		}
		
		vivoOrder = paramKey2Value.find("vivoOrder");
		if (vivoOrder == paramKey2Value.end())
		{
			OptErrorLog("get vivo order info reply error, can not find the vivoOrder, orderId = %s", vivoOrderInfo->orderId);
			break;
		}
		
		ParamKey2Value::const_iterator signature = paramKey2Value.find("signature");
		if (signature == paramKey2Value.end())  
		{
			OptErrorLog("get vivo order info reply error, can not find the signature, orderId = %s", vivoOrderInfo->orderId);
			break;
		}
		
		// 验证参数有效性
        map<string, string> parameter;
		const char* vivoParams[] = {"orderAmount", "vivoSignature"};
		for (unsigned int idx = 0; idx < sizeof(vivoParams) / sizeof(vivoParams[0]); ++idx)
		{
			ParamKey2Value::const_iterator paramIt = paramKey2Value.find(vivoParams[idx]);
			if (paramIt == paramKey2Value.end())
			{
				parameter.clear();
				
				OptErrorLog("get vivo order info reply error, can not find the %s, orderId = %s", vivoParams[idx], vivoOrderInfo->orderId);
				break;
			}
			
			parameter.insert(pair<string, string>(vivoParams[idx], paramIt->second));
		}
		if (parameter.empty()) break;
		
		parameter.insert(pair<string, string>(respCode->first, respCode->second));
		parameter.insert(pair<string, string>(respMsg->first, respMsg->second));
		parameter.insert(pair<string, string>(vivoOrder->first, vivoOrder->second));
		
		string reqParameter;
		for(auto it = parameter.begin(); it != parameter.end(); ++it)
		{
			reqParameter += (it->first + "=" + it->second + "&");
		}
		
		char md5Result[Md5BuffSize] = {0};
		const char* payKey = m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::Vivo)->paymentKey;
		md5Encrypt(payKey, strlen(payKey), md5Result);
		reqParameter += md5Result;
		
		// signature=md5_hex(key1=value1&key2=value2&...&keyn=valuen&to_lower_case(md5_hex(App-key)))
		md5Encrypt(reqParameter.c_str(), reqParameter.length(), md5Result);
		if (signature->second != md5Result)
		{
			OptErrorLog("get vivo order info reply error, md5 result = %s, signature = %s, orderId = %s", md5Result, signature->second.c_str(), vivoOrderInfo->orderId);
			break;
		}
		
		// 获取内部订单信息
		FishCoinOrderIdToInfo::iterator serviceOrderIt;
		if(!getTranscationData(vivoOrderInfo->orderId, serviceOrderIt))
		{
			OptErrorLog("get vivo order info reply error, can not find the service roderId = %s", vivoOrderInfo->orderId);
			break;
		}
		
		thirdRsp.set_result(OptSuccess);
		thirdRsp.set_order_id(vivoOrderInfo->orderId);
		thirdRsp.set_fish_coin_id(serviceOrderIt->second.itemId);
		thirdRsp.set_fish_coin_amount(serviceOrderIt->second.itemAmount);
		thirdRsp.set_money(serviceOrderIt->second.money);
		
		com_protocol::ThirdOrderKeyValue* value = thirdRsp.add_key_value();
		value->set_key("vivoSignature");
		value->set_value(paramKey2Value.at("vivoSignature"));
		
		value = thirdRsp.add_key_value();
		value->set_key("vivoOrder");
		value->set_value(vivoOrder->second);
		
		break;
	}
    
    int rc = -1;
    const char* msgData = m_msgHandler->serializeMsgToBuffer(thirdRsp, "send vivo fish coin order to client");
    if (msgData != NULL)
	{
        rc = m_msgHandler->sendMessage(msgData, thirdRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), cd->keyData.srvId, GET_FISH_COIN_RECHARGE_ORDER_RSP);
		if (rc == OptSuccess && thirdRsp.result() == OptSuccess)
		{
			// 充值订单信息记录
			// format = name|id|amount|money|rechargeNo|request|platformType|srcSrvId|vivoOrder
			ThirdRechargeLog(ThirdPlatformType::Vivo, "%s|%u|%u|%.2f|%s|request|%u|%u|%s", cd->keyData.key, thirdRsp.fish_coin_id(), thirdRsp.fish_coin_amount(),
			thirdRsp.money(), thirdRsp.order_id().c_str(), ThirdPlatformType::Vivo, cd->keyData.srvId, vivoOrder->second.c_str());
		}
    }
	
	OptInfoLog("get vivo order info reply, srcSrvId = %u, user = %s, fish coin id = %u, money = %.2f, order id = %s, platform id = %u, result = %d, rc = %d",
		       cd->keyData.srvId, cd->keyData.key, thirdRsp.fish_coin_id(), thirdRsp.money(), vivoOrderInfo->orderId, ThirdPlatformType::Vivo, thirdRsp.result(), rc);
			   
    // 失败则删除订单数据
	const bool isOK = thirdRsp.result() == OptSuccess && rc == OptSuccess;
	if (!isOK) removeTransactionData(vivoOrderInfo->orderId);
	
	return isOK;
}

// 苹果版本充值验证
void CThirdInterface::checkAppleRechargeResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CheckAppleRechargeResultNotify checkNotify;
	if (!m_msgHandler->parseMsgFromBuffer(checkNotify, data, len, "check apple recharge result") || checkNotify.receipt_data().empty()) return;
	
	// 内部订单号信息
    FishCoinOrderIdToInfo::iterator thirdInfoIt;
    if(!getTranscationData(checkNotify.order_id().c_str(), thirdInfoIt))
    {
        OptErrorLog("check apple recharge result, can not find order id = %s", checkNotify.order_id().c_str());
		return;
    }
	
	// 支付收据数据Base64编码。 
    char base64Buff[MaxNetBuffSize] = {0};
    unsigned int base64BuffLen = MaxNetBuffSize - 1;
    if(!base64Encode(checkNotify.receipt_data().c_str(), checkNotify.receipt_data().length(), base64Buff, base64BuffLen))
    {
        OptErrorLog("check apple recharge result, receipt data base64 encode error");
        return;
    }
	
	const ThirdPlatformConfig* pAppleThird = m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::AppleIOS);
	if (pAppleThird == NULL)
	{
		OptErrorLog("check apple recharge result, get apple config error");
        return;
	}

    // 日志信息
	ThirdRechargeLog(ThirdPlatformType::AppleIOS, "%s|%u|%u|%.2f|%s|check|%u|%u|%s", thirdInfoIt->second.user, thirdInfoIt->second.itemId, thirdInfoIt->second.itemAmount,
	                 thirdInfoIt->second.money, checkNotify.order_id().c_str(), ThirdPlatformType::AppleIOS, srcSrvId, checkNotify.receipt_data().c_str());
	
	// https://sandbox.itunes.apple.com/verifyReceipt   // 沙箱测试环境
	// https://buy.itunes.apple.com/verifyReceipt       // 正式生产环境
	// {"receipt-data": receipt} POST 内容
	ConnectData* cd = m_msgHandler->getConnectData();
	memset(cd, 0, sizeof(ConnectData));
	
	cd->timeOutSecs = HttpConnectTimeOut;
	strncpy(cd->keyData.key, checkNotify.order_id().c_str(), MaxKeySize - 1);
	cd->keyData.flag = HttpReplyHandlerID::CheckAppleIOSPay;
	cd->keyData.srvId = srcSrvId;
	cd->keyData.protocolId = 0;
	
	CHttpRequest httpRequest(HttpMsgType::POST);
	httpRequest.setHeaderKeyValue("Content-Type", "application/json");
	
	string postContent = string("{\"receipt-data\": ") + string(base64Buff, base64BuffLen) + "}";
	httpRequest.setContent(postContent.c_str(), postContent.length());
	cd->sendDataLen = MaxNetBuffSize;

	const char* msgData = httpRequest.getMessage(cd->sendData, cd->sendDataLen, pAppleThird->host, pAppleThird->url);
    if (msgData == NULL)
	{
		m_msgHandler->putConnectData(cd);
		OptErrorLog("check apple recharge result, get http request message error");
		return;
	}

	bool result = m_msgHandler->doHttpConnect(pAppleThird->ip, pAppleThird->port, cd);
	if (!result)
	{
		m_msgHandler->putConnectData(cd);
		OptErrorLog("do http connect to check apple recharge result error, ip = %s, port = %d", pAppleThird->ip, pAppleThird->port);
	}
	
	OptInfoLog("do http connect to check apple recharge, result = %d, user = %s, order = %s, receipt data = %s, message len = %u, content = %s",
	result, thirdInfoIt->second.user, checkNotify.order_id().c_str(), checkNotify.receipt_data().c_str(), cd->sendDataLen, cd->sendData);
}

bool CThirdInterface::checkAppleRechargeResultReply(ConnectData* cd)
{
	// 内部订单号信息
    FishCoinOrderIdToInfo::iterator thirdInfoIt;
    if(!getTranscationData(cd->keyData.key, thirdInfoIt))
    {
		ThirdRechargeLog(ThirdPlatformType::AppleIOS, "%s|%u|%u|replyError", cd->keyData.key, ThirdPlatformType::AppleIOS, cd->keyData.srvId);
						 
        OptErrorLog("check apple recharge reply error, can not find order id = %s", cd->keyData.key);
		return false;
    }

    /* apple reply message content
    {
	"status" : 0, // if 0 successful or failure
	"receipt" : {…..}
	}
	*/
	
	ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
	ParamKey2Value::iterator receiptIt = paramKey2Value.find("receipt");
	if (receiptIt == paramKey2Value.end())
	{
		// 日志信息
	    ThirdRechargeLog(ThirdPlatformType::AppleIOS, "%s|%u|%u|%.2f|%s|replyError|%u|%u", thirdInfoIt->second.user, thirdInfoIt->second.itemId, thirdInfoIt->second.itemAmount,
		                 thirdInfoIt->second.money, cd->keyData.key, ThirdPlatformType::AppleIOS, cd->keyData.srvId);
						 
		OptErrorLog("check apple recharge reply error, can not find receipt data, order id = %s", cd->keyData.key);
		return false;
	}
	
	ParamKey2Value::iterator statusIt = paramKey2Value.find("status");
	if (statusIt == paramKey2Value.end())
	{
		// 日志信息
	    ThirdRechargeLog(ThirdPlatformType::AppleIOS, "%s|%u|%u|%.2f|%s|replyError|%u|%u|%s", thirdInfoIt->second.user, thirdInfoIt->second.itemId, thirdInfoIt->second.itemAmount,
		                 thirdInfoIt->second.money, cd->keyData.key, ThirdPlatformType::AppleIOS, cd->keyData.srvId, receiptIt->second.c_str());
						 
		OptErrorLog("check apple recharge reply error, can not find status data, order id = %s", cd->keyData.key);
		return false;
	}
	
	// 日志信息
	ThirdRechargeLog(ThirdPlatformType::AppleIOS, "%s|%u|%u|%.2f|%s|reply|%u|%u|%s|%s", thirdInfoIt->second.user, thirdInfoIt->second.itemId, thirdInfoIt->second.itemAmount,
	                 thirdInfoIt->second.money, cd->keyData.key, ThirdPlatformType::AppleIOS, cd->keyData.srvId, statusIt->second.c_str(), receiptIt->second.c_str());
					 
	// 充值结果
    int result = atoi(statusIt->second.c_str());
	provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::AppleIOS, "", receiptIt->second.c_str());
	
	return true;
}

// Facebook 用户信息
void CThirdInterface::checkFacebookUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUsernameByPlatformReq checkNotify;
	if (!m_msgHandler->parseMsgFromBuffer(checkNotify, data, len, "check facebook user") || checkNotify.user_token().empty()) return;

    // 构造https消息请求头部数据
	// https://graph.facebook.com/me?access_token=%s&fields=%s
	// fields = "id,gender,name";
	// GET /me?access_token=xxx&fields=id,gender,name HTTP/1.1\r\nHost: graph.facebook.com\r\n\r\n
	char urlParam[MaxNetBuffSize] = {0};
	unsigned int urlParamLen = snprintf(urlParam, MaxNetBuffSize - 1, "access_token=%s&fields=id,name,gender", checkNotify.user_token().c_str());
	CHttpRequest httpRequest(HttpMsgType::GET);
	httpRequest.setHeaderKeyValue("Accept-Charset", "utf-8");
	httpRequest.setParam(urlParam, urlParamLen);

	FacebookUserInfo fUserInfo;
	fUserInfo.flag = 0;
	fUserInfo.userInfoLen = len;
	memcpy(fUserInfo.userInfo, data, len);
	if (!m_msgHandler->doHttpConnect(ThirdPlatformType::HanYouFacebook, srcSrvId, httpRequest, NULL, HttpReplyHandlerID::CheckFacebookUserInfo, &fUserInfo, sizeof(fUserInfo)))
	{
		OptErrorLog("do facebook https connect error");
	}
	
	OptInfoLog("do http connect to check facebook user = %s, id = %s, token = %s", m_msgHandler->getContext().userData, checkNotify.platform_id().c_str(), checkNotify.user_token().c_str());
}

bool CThirdInterface::checkFacebookUserReply(ConnectData* cd)
{
	const FacebookUserInfo* fUserInfo = (const FacebookUserInfo*)cd->cbData;
	com_protocol::CheckFacebookUserNotify notify;
	com_protocol::GetUsernameByPlatformReq* reqInfo = notify.mutable_user_info();
	if (!m_msgHandler->parseMsgFromBuffer(*reqInfo, fUserInfo->userInfo, fUserInfo->userInfoLen, "check facebook user reply")) return false;
	
	while (true)
	{
		ParamKey2Value paramKey2Value;
		CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
		ParamKey2Value::iterator idIt = paramKey2Value.find("id");
		if (idIt == paramKey2Value.end())
		{
			OptErrorLog("check facebook user reply error, can not find id, user = %s, id = %s", cd->keyData.key, reqInfo->platform_id().c_str());
			
			notify.set_result(-1);
			break;
		}
		
		if (idIt->second != reqInfo->platform_id())
		{
			OptErrorLog("check facebook user reply, the id is error, user = %s, id = %s, reply id = %s", cd->keyData.key, reqInfo->platform_id().c_str(), idIt->second.c_str());
			
			notify.set_result(-1);
			break;
		}
		
		ParamKey2Value::iterator nameIt = paramKey2Value.find("name");
		if (nameIt == paramKey2Value.end())
		{
			OptErrorLog("check facebook user reply error, can not find name, user = %s, id = %s", cd->keyData.key, reqInfo->platform_id().c_str());
			
			notify.set_result(-1);
			break;
		}
		
		ParamKey2Value::iterator genderIt = paramKey2Value.find("gender");
		if (genderIt == paramKey2Value.end())
		{
			OptErrorLog("check facebook user reply error, can not find gender, user = %s, id = %s", cd->keyData.key, reqInfo->platform_id().c_str());
			
			notify.set_result(-1);
			break;
		}
		
		notify.set_result(0);
		reqInfo->set_nickname(nameIt->second);
		reqInfo->set_gender(0);

	    break;
	}
	
	int rc = -1;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(notify, "send check facebook user result to client");
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, notify.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), cd->keyData.srvId, CHECK_FACEBOOK_USER_RSP);

	OptInfoLog("send check facebook user reply message, rc = %d, result = %d, id = %s", rc, notify.result(), reqInfo->platform_id().c_str());
	
	return notify.result() == 0;
}

bool CThirdInterface::handleHanYouCouponRewardMsg(Connect* conn, const HttpMsgBody& msgBody)
{
	string content;
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getUserData(conn);
	bool isOK = handleHanYouCouponRewardMsgImpl(ud->connId, msgBody, content);
	if (!isOK)
	{
		CHttpResponse rsp;
		rsp.setContent(content);

		unsigned int msgLen = 0;
		const char* rspMsg = rsp.getMessage(msgLen);
		int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
		OptErrorLog("send reply message to Han You, error msg = %s, rc = %d", content.c_str(), rc);
	}
	
	return isOK;
}

bool CThirdInterface::handleHanYouCouponRewardMsgImpl(const char* connId, const HttpMsgBody& msgBody, string& content)
{
	/*
	<各情况下 各情况下 Json Json Json 应答格式 应答格式 >
	- 安全 检查失败 : {"Result":false,"ResultCode":1100,"ResultMsg":"invalid signed value"}
	- 重复 奖励 :     {"Result":false,"ResultCode":3100,"ResultMsg":"duplicate transaction"}
	- 用户 检查失败 : {"Result":false,"ResultCode":3200,"ResultMsg":"invalid user"}
	- 发送例外事件 :  {"Result":false,"ResultCode":4000,"ResultMsg":"custom error message"}
	- 奖励支付成功 :  {"Result":true,"ResultCode":1,"ResultMsg":"success"}
	*/
	
	ParamKey2Value::const_iterator usnIt = msgBody.paramKey2Value.find("username");
    if(usnIt == msgBody.paramKey2Value.end())
    {
		content = "{\"Result\":false,\"ResultCode\":4000,\"ResultMsg\":\"can not find the user name\"}";
        OptErrorLog("handle han you reward, can not find the user name");
        return false;
    }
	
	ParamKey2Value::const_iterator rwKeyIt = msgBody.paramKey2Value.find("reward_key");
    if(rwKeyIt == msgBody.paramKey2Value.end())
    {
		content = "{\"Result\":false,\"ResultCode\":4000,\"ResultMsg\":\"can not find the reward key\"}";
        OptErrorLog("handle han you reward, can not find the reward key");
        return false;
    }
	
	ParamKey2Value::const_iterator quantityIt = msgBody.paramKey2Value.find("quantity");
    if(quantityIt == msgBody.paramKey2Value.end())
    {
		content = "{\"Result\":false,\"ResultCode\":4000,\"ResultMsg\":\"can not find the reward quantity\"}";
        OptErrorLog("handle han you reward, can not find the reward quantity");
        return false;
    }
	
	ParamKey2Value::const_iterator signedValIt = msgBody.paramKey2Value.find("signed_value");
    if(signedValIt == msgBody.paramKey2Value.end())
    {
		content = "{\"Result\":false,\"ResultCode\":4000,\"ResultMsg\":\"can not find the signed value\"}";
        OptErrorLog("handle han you reward, can not find the signed value");
        return false;
    }
	
	// 哈希运算消息认证
	unsigned char hmacMD5Buff[MaxNetBuffSize] = {0};
    unsigned int hmacMD5BuffLen = MaxNetBuffSize - 1;
	string checkData = usnIt->second + rwKeyIt->second + quantityIt->second;
	const char* paymentKey = m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::HanYou)->paymentKey;
    const char* hmacMD5Result = Compute_HMAC(EVP_md5(), paymentKey, strlen(paymentKey), (const unsigned char*)checkData.c_str(), checkData.length(), hmacMD5Buff, hmacMD5BuffLen);
	
	// 认证结果转换为16进制比较
	char signatureHex[MaxNetBuffSize] = {0};
	int signatureHexLen = b2str(hmacMD5Result, hmacMD5BuffLen, signatureHex, MaxNetBuffSize - 1, false);
	if (signatureHexLen != (int)signedValIt->second.length() || memcmp(signatureHex, signedValIt->second.c_str(), signatureHexLen) != 0)
	{
		content = "{\"Result\":false,\"ResultCode\":1100,\"ResultMsg\":\"invalid signed value\"}";
		OptErrorLog("handle han you reward, check the signature error, signed value = %s, check result = %s, data = %s, key = %s",
		signedValIt->second.c_str(), signatureHex, checkData.c_str(), paymentKey);
		return false;
	}
	
	// 获取优惠券奖励
	com_protocol::UseHanYouCouponReq useCouponReq;
	useCouponReq.set_cb_data(connId);
	useCouponReq.set_coupon_id(rwKeyIt->second);
	com_protocol::GiftInfo* gift = useCouponReq.mutable_coupon_reward();
	gift->set_type(EPropType::PropGold);
	gift->set_num(atoi(quantityIt->second.c_str()));
	
	int rc = SrvRetCode::OptFailed;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(useCouponReq, "use coupon request");
	if (msgData != NULL)
	{							
		rc = m_msgHandler->sendMessageToCommonDbProxy(msgData, useCouponReq.ByteSize(), BUSINESS_ADD_HAN_YOU_COUPON_REWARD_REQ, usnIt->second.c_str(), usnIt->second.length());
	}
	
	if (rc != SrvRetCode::OptSuccess)
	{
		content = "{\"Result\":false,\"ResultCode\":4000,\"ResultMsg\":\"send add reward message failed\"}";
		OptErrorLog("handle han you reward, send add reward message error, reward key = %s, quantity = %s, rc = %d",
		rwKeyIt->second.c_str(), quantityIt->second.c_str(), rc);
		return false;
	}
	
	OptInfoLog("receive Han You add coupon reward request, user = %s, coupon id = %s, quantity = %s, connId = %s",
	usnIt->second.c_str(), rwKeyIt->second.c_str(), quantityIt->second.c_str(), connId);

    return true;
}

void CThirdInterface::addHanYouCouponRewardReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/*
	<各情况下 各情况下 Json Json Json 应答格式 应答格式 >
	- 安全 检查失败 : {"Result":false,"ResultCode":1100,"ResultMsg":"invalid signed value"}
	- 重复 奖励 :     {"Result":false,"ResultCode":3100,"ResultMsg":"duplicate transaction"}
	- 用户 检查失败 : {"Result":false,"ResultCode":3200,"ResultMsg":"invalid user"}
	- 发送例外事件 :  {"Result":false,"ResultCode":4000,"ResultMsg":"custom error message"}
	- 奖励支付成功 :  {"Result":true,"ResultCode":1,"ResultMsg":"success"}
	*/
	
	com_protocol::UseHanYouCouponRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "add coupon reward reply")) return;
	
	Connect* conn = m_msgHandler->getConnect(rsp.cb_data());
	if (conn == NULL)
	{
		OptErrorLog("add han you coupon reward, get connect error, user = %s, id = %s", m_msgHandler->getContext().userData, rsp.cb_data().c_str());
		return;
	}
	
	string content = "{\"Result\":true,\"ResultCode\":1,\"ResultMsg\":\"success\"}";
	if (rsp.result() != 0) content = "{\"Result\":false,\"ResultCode\":4000,\"ResultMsg\":\"add coupon reward error\"}";
	
	CHttpResponse httpRsp;
	httpRsp.setContent(content);

	unsigned int msgLen = 0;
	const char* rspMsg = httpRsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send add coupon reward reply message to Han You, user = %s, result = %d, rc = %d, coupon id = %s, msg = %s",
	m_msgHandler->getContext().userData, rsp.result(), rc, rsp.coupon_id().c_str(), content.c_str());
	
	if (rsp.result() == 0)
	{
		// 韩文版HanYou平台使用优惠券获得的奖励通知
	    const unsigned short COMMON_ADD_HAN_YOU_COUPON_REWARD_NOTIFY = 116;
		com_protocol::ClientReceivedHanYouCouponRewardNotify couponRewardNotify;
		couponRewardNotify.set_allocated_coupon_reward(rsp.release_coupon_reward());
	    m_msgHandler->sendMessageToService(ServiceType::LoginSrv, couponRewardNotify, COMMON_ADD_HAN_YOU_COUPON_REWARD_NOTIFY,
		                                   m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
	}
}


/*
// M4399用户校验
void CThirdInterface::checkM4399User(const ParamKey2Value& key2value, unsigned int srcSrvId)
{
    //uid
    ParamKey2Value::const_iterator uidIt = key2value.find("uid");
	if (uidIt == key2value.end())
	{
		OptErrorLog("checkM4399User, can not find the uid");
		return;
	}
    
    //token
    ParamKey2Value::const_iterator tokenIt = key2value.find("token");
	if (tokenIt == key2value.end())
	{
		OptErrorLog("checkM4399User, can not find the token");
		return;
	}
    
    //GET http://m.4399api.com/openapi/oauth-check.html
    
     // 构造http消息请求头部数据
	char urlParam[MaxNetBuffSize] = {0};
	unsigned int urlParamLen = snprintf(urlParam, MaxNetBuffSize - 1, "state=%s&uid=%s", 
                                        tokenIt->second.c_str(), uidIt->second.c_str());
                                        
	CHttpRequest httpRequest(HttpMsgType::GET);
	httpRequest.setParam(urlParam, urlParamLen);
	if (!m_msgHandler->doHttpConnect(ThirdPlatformType::M4399, srcSrvId, httpRequest))
	{
		OptErrorLog("do 4399 http connect error");
	}
}

bool CThirdInterface::checkM4399UserReply(ConnectData* cd)
{
    ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value);
    
    ParamKey2Value::iterator codeIt = paramKey2Value.find("code");
    if (codeIt == paramKey2Value.end())
    {
        OptErrorLog("checkM4399UserReply can not find the code");
        return false;
    }
    
    ParamKey2Value::iterator messageIt = paramKey2Value.find("message");
    if (messageIt == paramKey2Value.end())
    {
        OptErrorLog("checkM4399UserReply can not find the message");
        return false;
    }
    
    com_protocol::ThirdPlatformUserCheckRsp checkM4399Rsp; 
    checkM4399Rsp.set_third_code(0);  
    checkM4399Rsp.set_result((codeIt->second == "100" ? 0 : 1));

    int rc = -1;  
    const char* msgData = m_msgHandler->serializeMsgToBuffer(checkM4399Rsp, "send check 4399 user result to client");
	if (msgData != NULL) 
    {
        rc = m_msgHandler->sendMessage(msgData, checkM4399Rsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), 
                                       cd->keyData.srvId, cd->keyData.protocolId);
    }
    
    OptInfoLog("send check 4399 user reply message, rc=%d, data=%s", rc, cd->receiveData);
    return true;
}

  //  魅族用户校验
void CThirdInterface::checkMeiZuUser(const ParamKey2Value& key2value, unsigned int srcSrvId)
{
	//uid
    ParamKey2Value::const_iterator uidIt = key2value.find("uid");
	if (uidIt == key2value.end())
	{
		OptErrorLog("checkMeiZuUser, can not find the uid");
		return;
	}
    
    //session
    ParamKey2Value::const_iterator sessionIt = key2value.find("session");
	if (sessionIt == key2value.end())
	{
		OptErrorLog("checkMeiZuUser, can not find the session");
		return;
	}
    
	//按Key排序
	map<string, string> signMap;
	signMap.insert(map<string, string>::value_type("app_id", m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::MeiZu)->appId));
	signMap.insert(map<string, string>::value_type("session_id", sessionIt->second));
	signMap.insert(map<string, string>::value_type("uid", uidIt->second));
	
	char ts[MaxNetBuffSize] = {0};
	snprintf(ts, MaxNetBuffSize - 1, "%u", (unsigned int)time(NULL));
	signMap.insert(map<string, string>::value_type("ts", ts));
	
	
	//需要发送的数据
	string urlData;
	for(auto it = signMap.begin(); it != signMap.end(); )
	{
		urlData += it->first + "=" + it->second;

		if(++it != signMap.end())
			urlData += "&";
	}
	
	//签名
	string sign = urlData + ":";
    sign += m_platformatOpt->getThirdPlatformCfg(ThirdPlatformType::MeiZu)->paymentKey;

	//MD5
    char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(sign.c_str(), (unsigned int)sign.size(), md5Buff);


    // 构造http消息请求头部数据
	urlData += "sign_type=md5&sign=";
	urlData += md5Buff;
	
	CHttpRequest httpRequest(HttpMsgType::POST);
	httpRequest.setParam(urlData.c_str(), urlData.size());
	if (!m_msgHandler->doHttpConnect(ThirdPlatformType::MeiZu, srcSrvId, httpRequest))
	{
		OptErrorLog("do mei zu http connect error");
	}
    
    return ;

}

bool CThirdInterface::checkMeiZuUserReply(ConnectData* cd)
{
	 ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value);
    
    ParamKey2Value::iterator codeIt = paramKey2Value.find("code");
    if (codeIt == paramKey2Value.end())
    {
        OptErrorLog("checkMeiZuUserReply can not find the code");
        return false;
    }
    
    ParamKey2Value::iterator messageIt = paramKey2Value.find("message");
    if (messageIt == paramKey2Value.end())
    {
        OptErrorLog("checkMeiZuUserReply can not find the message");
        return false;
    }
    
    com_protocol::ThirdPlatformUserCheckRsp checkMeiZuRsp; 
    checkMeiZuRsp.set_third_code(0);  
    checkMeiZuRsp.set_result((codeIt->second == "200" ? 0 : 1));

    int rc = -1;  
    const char* msgData = m_msgHandler->serializeMsgToBuffer(checkMeiZuRsp, "send check mei zu user result to client");
	if (msgData != NULL) 
    {
        rc = m_msgHandler->sendMessage(msgData, checkMeiZuRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), 
                                       cd->keyData.srvId, cd->keyData.protocolId);
    }
    
    OptInfoLog("send check Mei Zu user reply message, rc=%d, data=%s", rc, cd->receiveData);
    return true;
}
*/

}

