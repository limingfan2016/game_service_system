
/* author : limingfan
 * date : 2015.08.03
 * description : 充值管理
 */

#include "CommonType.h"
#include "CRechargeMgr.h"
#include "base/ErrorCode.h"


using namespace NCommon;
using namespace NFrame;
using namespace NErrorCode;

namespace NProject
{

// http服务协议ID值
enum HttpServiceProtocol
{
	// 当乐平台
	GET_DOWNJOY_RECHARGE_TRANSACTION_REQ = 3,
	CANCEL_DOWNJOY_RECHARGE_NOTIFY = 6,
	
	// Google SDK
	GET_GOOGLE_RECHARGE_TRANSACTION_REQ = 7,
	CHECK_GOOGLE_RECHARGE_RESULT_REQ = 9,
	
	// 第三方平台用户充值
	GET_THIRD_RECHARGE_TRANSACTION_REQ = 11,
	CANCEL_THIRD_RECHARGE_NOTIFY = 14,
	
	// 老版本商城配置
	GET_MALL_CONFIG_REQ = 19,
	GET_MALL_CONFIG_RSP = 20,

	//查询腾讯充值
	FIN_RECHARGE_SUCCESS_REQ = 21,
	FIN_RECHARGE_SUCCESS_RSP = 22,
	
	// 新版本游戏商城配置
	// GET_GAME_MALL_CONFIG_REQ = 23,
	// GET_GAME_MALL_CONFIG_RSP = 24,
	
	// 渔币充值
	GET_FISH_COIN_RECHARGE_ORDER_REQ = 25,
	GET_FISH_COIN_RECHARGE_ORDER_RSP = 26,
	FISH_COIN_RECHARGE_NOTIFY = 27,
	CANCEL_FISH_COIN_RECHARGE_NOTIFY = 28,
	
	// 苹果版本充值结果校验
	CHECK_APPLE_RECHARGE_RESULT = 29,
	
	/*
	// 兑换游戏商品
	EXCHANGE_GAME_GOODS_REQ = 29,
	EXCHANGE_GAME_GOODS_RSP = 30,
	
	// 获取超值礼包信息
	GET_SUPER_VALUE_PACKAGE_REQ = 31,
	GET_SUPER_VALUE_PACKAGE_RSP = 32,
	
	// 领取超值礼包物品
	RECEIVE_SUPER_VALUE_PACKAGE_GIFT_REQ = 33,
	RECEIVE_SUPER_VALUE_PACKAGE_GIFT_RSP = 34,
	
	// 获取首冲礼包信息
	GET_FIRST_RECHARGE_PACKAGE_REQ = 35,
	GET_FIRST_RECHARGE_PACKAGE_RSP = 36,
	*/ 
};

// dbproxy服务协议ID值
enum DbProxyServiceProtocol
{
	// 老版本商城配置
	// BUSINESS_GET_MALL_CONFIG_REQ = 19,
	// BUSINESS_GET_MALL_CONFIG_RSP = 20,
	
	// 新版本，获取游戏商城配置
	BUSINESS_GET_GAME_MALL_CONFIG_REQ = 51,
	BUSINESS_GET_GAME_MALL_CONFIG_RSP = 52,
	
	// 兑换游戏商品
	BUSINESS_EXCHANGE_GAME_GOODS_REQ = 53,
	BUSINESS_EXCHANGE_GAME_GOODS_RSP = 54,
	
	// 获取实物兑换配置信息
	BUSINESS_GET_GOODS_EXCHANGE_CONFIG_REQ = 59,
	BUSINESS_GET_GOODS_EXCHANGE_CONFIG_RSP = 60,
	
	// 兑换话费额度
	BUSINESS_EXCHANGE_PHONE_FARE_REQ = 61,
	BUSINESS_EXCHANGE_PHONE_FARE_RSP = 62,
	
	// 获取公共配置信息
	BUSINESS_GET_GAME_COMMON_CONFIG_REQ = 65,
	BUSINESS_GET_GAME_COMMON_CONFIG_RSP = 66,
	
	// 获取首冲礼包信息
	BUSINESS_GET_FIRST_RECHARGE_PACKAGE_REQ = 67,
	BUSINESS_GET_FIRST_RECHARGE_PACKAGE_RSP = 68,
};


CRechargeMgr::CRechargeMgr()
{
	m_msgBuffer[0] = '\0';
	m_msgHandler = NULL;
}

CRechargeMgr::~CRechargeMgr()
{
	m_msgBuffer[0] = '\0';
	m_msgHandler = NULL;
}

bool CRechargeMgr::init(NFrame::CLogicHandler* msgHandler)
{
	m_msgHandler = msgHandler;
	return (m_msgHandler != NULL);
}

bool CRechargeMgr::setClientMessageHandler(const unsigned short msgHandlerType, const unsigned short reqId, const unsigned short rspId)
{
	switch (msgHandlerType)
	{
		case EMessageHandlerType::EClientGameMall :
		{
			m_request2replyId[BUSINESS_GET_GAME_MALL_CONFIG_RSP] = rspId;
			m_msgHandler->registerProtocol(OutsideClientSrv, reqId, (ProtocolHandler)&CRechargeMgr::getGameGoodsMallCfg, this);
			m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_GAME_MALL_CONFIG_RSP, (ProtocolHandler)&CRechargeMgr::getGameGoodsMallCfgReply, this);
			
			return true;
		}
		
		case EMessageHandlerType::EClientGetRechargeFishCoinOrder :
		{
			m_request2replyId[GET_FISH_COIN_RECHARGE_ORDER_RSP] = rspId;
			m_msgHandler->registerProtocol(OutsideClientSrv, reqId, (ProtocolHandler)&CRechargeMgr::rechargeFishCoin, this);
			m_msgHandler->registerProtocol(HttpSrv, GET_FISH_COIN_RECHARGE_ORDER_RSP, (ProtocolHandler)&CRechargeMgr::rechargeFishCoinReply, this);
			
			return true;
		}
		
		case EMessageHandlerType::EClientCancelRechargeFishCoinOrder :
		{
			m_msgHandler->registerProtocol(OutsideClientSrv, reqId, (ProtocolHandler)&CRechargeMgr::cancelRechargeFishCoin, this);
			
			return true;
		}
		
		case EMessageHandlerType::EClientRechargeFishCoinResultNotify :
		{
			m_request2replyId[FISH_COIN_RECHARGE_NOTIFY] = rspId;
			m_msgHandler->registerProtocol(HttpSrv, FISH_COIN_RECHARGE_NOTIFY, (ProtocolHandler)&CRechargeMgr::rechargeFishCoinResultNotify, this);
			
			return true;
		}
	
		case EMessageHandlerType::EClientExchangeGameGoods :
		{
			m_msgHandler->registerProtocol(OutsideClientSrv, reqId, (ProtocolHandler)&CRechargeMgr::exchangeGameGoods, this);
			if (rspId > 0)
			{
				m_request2replyId[BUSINESS_EXCHANGE_GAME_GOODS_RSP] = rspId;
			    m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_EXCHANGE_GAME_GOODS_RSP, (ProtocolHandler)&CRechargeMgr::exchangeGameGoodsReply, this);
			}
			
			return true;
		}
		
		/*
		case EMessageHandlerType::EClientGetSuperValuePackageInfo :
		{
			m_msgHandler->registerProtocol(OutsideClientSrv, reqId, (ProtocolHandler)&CRechargeMgr::getSuperValuePackageInfo, this);
			m_request2replyId[GET_SUPER_VALUE_PACKAGE_RSP] = rspId;
			m_msgHandler->registerProtocol(HttpSrv, GET_SUPER_VALUE_PACKAGE_RSP, (ProtocolHandler)&CRechargeMgr::getSuperValuePackageInfoReply, this);
			
			return true;
		}
		
		case EMessageHandlerType::EClientReceiveSuperValuePackage :
		{
			m_msgHandler->registerProtocol(OutsideClientSrv, reqId, (ProtocolHandler)&CRechargeMgr::receiveSuperValuePackageGift, this);
			if (rspId > 0)
			{
				m_request2replyId[RECEIVE_SUPER_VALUE_PACKAGE_GIFT_RSP] = rspId;
			    m_msgHandler->registerProtocol(HttpSrv, RECEIVE_SUPER_VALUE_PACKAGE_GIFT_RSP, (ProtocolHandler)&CRechargeMgr::receiveSuperValuePackageGiftReply, this);
			}
			
			return true;
		}
		*/ 
		
		case EMessageHandlerType::EClientGetFirstPackageInfo :
		{
			m_msgHandler->registerProtocol(OutsideClientSrv, reqId, (ProtocolHandler)&CRechargeMgr::getFirstRechargeInfo, this);
			m_request2replyId[BUSINESS_GET_FIRST_RECHARGE_PACKAGE_RSP] = rspId;
			m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_FIRST_RECHARGE_PACKAGE_RSP, (ProtocolHandler)&CRechargeMgr::getFirstRechargeInfoReply, this);
			
			return true;
		}
	
		case EMessageHandlerType::EClientGetExchangeGoodsConfig :
		{
			m_request2replyId[BUSINESS_GET_GOODS_EXCHANGE_CONFIG_RSP] = rspId;
			m_msgHandler->registerProtocol(OutsideClientSrv, reqId, (ProtocolHandler)&CRechargeMgr::getExchangeMallCfg, this);
			m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_GOODS_EXCHANGE_CONFIG_RSP, (ProtocolHandler)&CRechargeMgr::getExchangeMallCfgReply, this);
			
			return true;
		}
		
		case EMessageHandlerType::EClientExchangePhoneFare :
		{
			m_request2replyId[BUSINESS_EXCHANGE_PHONE_FARE_RSP] = rspId;
			m_msgHandler->registerProtocol(OutsideClientSrv, reqId, (ProtocolHandler)&CRechargeMgr::exchangePhoneFare, this);
			m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_EXCHANGE_PHONE_FARE_RSP, (ProtocolHandler)&CRechargeMgr::exchangePhoneFareReply, this);
			
			return true;
		}
		
		case EMessageHandlerType::EClientCheckAppleRechargeResult :
		{
			m_msgHandler->registerProtocol(OutsideClientSrv, reqId, (ProtocolHandler)&CRechargeMgr::checkAppleRecharge, this);
			
			return true;
		}
		
		default :
		{
			OptErrorLog("set client message handler, the type is invalid, type = %d", msgHandlerType);
		}
	}
	
	return false;
}

// 注册处理协议的函数
int CRechargeMgr::addProtocolHandler(const unsigned short protocolId, ProtocolHandler handler, CHandler* instance)
{
	if (handler == NULL || instance == NULL) return InvalidParam;

    m_protocolId2Handler[protocolId] = MsgHandler();
	MsgHandler& msgHandler = m_protocolId2Handler[protocolId];
	msgHandler.instance = instance;
	msgHandler.handler = handler;
	return Success;
}

bool CRechargeMgr::setMallData(const char* data, const unsigned int len)
{
	return m_mallMgr.setMallData(data, len);
}

int CRechargeMgr::getMallData(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen)
{
	return sendMessageToHttpService(data, len, GET_MALL_CONFIG_REQ, userData, userDataLen, 0);
}


// 新版本游戏商城配置
void CRechargeMgr::getGameGoodsMallCfg(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userData = m_msgHandler->canToDoOperation(EServiceOperationType::EClientRequest, "get game mall config request");
	if (userData == NULL) return;

    sendMessageToCommonDbProxy(data, len, BUSINESS_GET_GAME_MALL_CONFIG_REQ, userData, strlen(userData));
}

void CRechargeMgr::getGameGoodsMallCfgReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_msgHandler->sendMsgToProxy(data, len, m_request2replyId[BUSINESS_GET_GAME_MALL_CONFIG_RSP], m_msgHandler->getContext().userData);
}

// 兑换商城物品
void CRechargeMgr::exchangeGameGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userData = m_msgHandler->canToDoOperation(EServiceOperationType::EClientRequest, "exchange game goods request");
	if (userData == NULL) return;

    com_protocol::ExchangeGameGoogsReq exchangeGameGoodsReq;
	if (!parseMsgFromBuffer(exchangeGameGoodsReq, data, len, "exchange game goods request")) return;

    int rc = sendMessageToCommonDbProxy(data, len, BUSINESS_EXCHANGE_GAME_GOODS_REQ, userData, strlen(userData));
	OptInfoLog("receive message to exchange game goods, user = %s, type = %u, id = %u, count = %u, rc = %d",
	userData, exchangeGameGoodsReq.type(), exchangeGameGoodsReq.id(), exchangeGameGoodsReq.count(), rc);
}

void CRechargeMgr::exchangeGameGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ExchangeGameGoogsRsp exchangeGameGoodsRsp;
	if (!parseMsgFromBuffer(exchangeGameGoodsRsp, data, len, "exchange game goods reply")) return;
	
	int rc = m_msgHandler->sendMsgToProxy(data, len, m_request2replyId[BUSINESS_EXCHANGE_GAME_GOODS_RSP], m_msgHandler->getContext().userData);
	OptInfoLog("exchange game goods reply, user = %s, result = %d, type = %u, id = %u, rc = %d",
	m_msgHandler->getContext().userData, exchangeGameGoodsRsp.result(), exchangeGameGoodsRsp.type(), exchangeGameGoodsRsp.id(), rc);

	/*
	// 是否注册监听函数
	ProtocolIdToHandler::iterator handlerIt = m_protocolId2Handler.find();
	if (handlerIt != m_protocolId2Handler.end() && handlerIt->second.instance != NULL && handlerIt->second.handler != NULL)
	{
		(handlerIt->second.instance->*(handlerIt->second.handler))(data, len, srcSrvId, srcModuleId, srcProtocolId);
	}
	*/ 
}

// 获取首冲礼包信息
void CRechargeMgr::getFirstRechargeInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userData = m_msgHandler->canToDoOperation(EServiceOperationType::EClientRequest, "get first package info request");
	if (userData == NULL) return;
	
	com_protocol::GetFirstPackageInfoReq req;
	if (!parseMsgFromBuffer(req, data, len, "get first package info request")) return;

    sendMessageToCommonDbProxy(data, len, BUSINESS_GET_FIRST_RECHARGE_PACKAGE_REQ, userData, strlen(userData));
}

void CRechargeMgr::getFirstRechargeInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_msgHandler->sendMsgToProxy(data, len, m_request2replyId[BUSINESS_GET_FIRST_RECHARGE_PACKAGE_RSP], m_msgHandler->getContext().userData);
}


/*
// 获取超值礼包信息
void CRechargeMgr::getSuperValuePackageInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userData = m_msgHandler->canToDoOperation(EServiceOperationType::EClientRequest, "get super value package info request");
	if (userData == NULL) return;
	
	com_protocol::GetSuperValuePackageInfoReq getSuperValuePkgReq;
	if (!parseMsgFromBuffer(getSuperValuePkgReq, data, len, "get super value package info request")) return;

	sendMessageToHttpService(data, len, GET_SUPER_VALUE_PACKAGE_REQ, userData, strlen(userData), getSuperValuePkgReq.platform_id());
}

void CRechargeMgr::getSuperValuePackageInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_msgHandler->sendMsgToProxy(data, len, m_request2replyId[GET_SUPER_VALUE_PACKAGE_RSP], m_msgHandler->getContext().userData);
}

// 领取超值礼包物品
void CRechargeMgr::receiveSuperValuePackageGift(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userData = m_msgHandler->canToDoOperation(EServiceOperationType::EClientRequest, "receive super value package request");
	if (userData == NULL) return;
	
	com_protocol::ReceiveSuperValuePackageReq req;
	if (!parseMsgFromBuffer(req, data, len, "receive super value package request")) return;

	sendMessageToHttpService(data, len, RECEIVE_SUPER_VALUE_PACKAGE_GIFT_REQ, userData, strlen(userData), req.platform_id());
}

void CRechargeMgr::receiveSuperValuePackageGiftReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_msgHandler->sendMsgToProxy(data, len, m_request2replyId[RECEIVE_SUPER_VALUE_PACKAGE_GIFT_RSP], m_msgHandler->getContext().userData);
}
*/


// 新版本实物兑换信息配置
void CRechargeMgr::getExchangeMallCfg(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userData = m_msgHandler->canToDoOperation(EServiceOperationType::EClientRequest, "get exchange mall config request");
	if (userData == NULL) return;
	
	sendMessageToCommonDbProxy(data, len, BUSINESS_GET_GOODS_EXCHANGE_CONFIG_REQ, userData, strlen(userData));
}

void CRechargeMgr::getExchangeMallCfgReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_msgHandler->sendMsgToProxy(data, len, m_request2replyId[BUSINESS_GET_GOODS_EXCHANGE_CONFIG_RSP], m_msgHandler->getContext().userData);
}

// 兑换手机话费额度
void CRechargeMgr::exchangePhoneFare(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userData = m_msgHandler->canToDoOperation(EServiceOperationType::EClientRequest, "exchange phone fare request");
	if (userData == NULL) return;

	int rc = sendMessageToCommonDbProxy(data, len, BUSINESS_EXCHANGE_PHONE_FARE_REQ, userData, strlen(userData));
	OptInfoLog("receive message to exchange phone fare, user = %s, rc = %d", userData, rc);
}

void CRechargeMgr::exchangePhoneFareReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ExchangePhoneFareRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "exchange phone fare reply")) return;
	
	int rc = m_msgHandler->sendMsgToProxy(data, len, m_request2replyId[BUSINESS_EXCHANGE_PHONE_FARE_RSP], m_msgHandler->getContext().userData);
	OptInfoLog("exchange phone fare reply, user = %s, result = %d, rc = %d", m_msgHandler->getContext().userData, rsp.result(), rc);
}


int CRechargeMgr::sendMessageToHttpService(const char* data, const unsigned int len, unsigned short protocolId, const char* userData, const unsigned int userDataLen, const unsigned int platformType)
{
	return m_msgHandler->sendMessage(data, len, userData, userDataLen, getHttpServiceId(platformType), protocolId);
}

int CRechargeMgr::sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userData, const unsigned int userDataLen)
{
	// 要根据负载均衡选择commonDBproxy服务
	unsigned int srvId = 0;
	getDestServiceID(DBProxyCommonSrv, userData, userDataLen, srvId);
	return m_msgHandler->sendMessage(data, len, userData, userDataLen, srvId, protocolId);
}

bool CRechargeMgr::parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info)
{
	if (!msg.ParseFromArray(buffer, len))
	{
		OptErrorLog("ParseFromArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), len, info);
		return false;
	}
	return true;
}

const char* CRechargeMgr::serializeMsgToBuffer(::google::protobuf::Message& msg, const char* info)
{
	if (!msg.SerializeToArray(m_msgBuffer, MaxMsgLen))
	{
		OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), MaxMsgLen, info);
		return NULL;
	}
	return m_msgBuffer;
}


// 新版本渔币充值
void CRechargeMgr::rechargeFishCoin(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userData = m_msgHandler->canToDoOperation(EServiceOperationType::EClientRequest, "get recharge fish coin order request");
	if (userData == NULL) return;

    com_protocol::GetRechargeFishCoinOrderReq getFishCoinOrderReq;
	if (!parseMsgFromBuffer(getFishCoinOrderReq, data, len, "get recharge fish coin order request")) return;
	
	int rc = sendMessageToHttpService(data, len, GET_FISH_COIN_RECHARGE_ORDER_REQ, userData, strlen(userData), getFishCoinOrderReq.platform_id());
	
	OptInfoLog("receive message to get fish coin recharge order, user = %s, platform id = %d, os type = %d, fish coin id = %d, rc = %d",
	userData, getFishCoinOrderReq.platform_id(), getFishCoinOrderReq.os_type(), getFishCoinOrderReq.fish_coin_id(), rc);
}

void CRechargeMgr::rechargeFishCoinReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetRechargeFishCoinOrderRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "get fish coin recharge order reply")) return;
	
	int rc = m_msgHandler->sendMsgToProxy(data, len, m_request2replyId[GET_FISH_COIN_RECHARGE_ORDER_RSP], m_msgHandler->getContext().userData);
	
	if (rsp.result() == Success)
	{
	    OptInfoLog("get fish coin recharge order, user = %s, money = %.2f, order id = %s, fish coin id = %d, amount = %d, rc = %d",
		m_msgHandler->getContext().userData, rsp.money(), rsp.order_id().c_str(), rsp.fish_coin_id(), rsp.fish_coin_amount(), rc);
	}
	else
	{
		OptWarnLog("get fish coin recharge order error, user = %s, result = %d, rc = %d", m_msgHandler->getContext().userData, rsp.result(), rc);
	}
}

void CRechargeMgr::cancelRechargeFishCoin(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userData = m_msgHandler->canToDoOperation(EServiceOperationType::EClientRequest, "cancel fish coin order request");
	if (userData == NULL) return;

    com_protocol::CancelRechargeFishCoinNotify cancelFishCoinOrder;
	if (!parseMsgFromBuffer(cancelFishCoinOrder, data, len, "cancel fish coin order request")) return;
	
	int rc = sendMessageToHttpService(data, len, CANCEL_FISH_COIN_RECHARGE_NOTIFY, userData, strlen(userData), cancelFishCoinOrder.platform_id());
	
	OptInfoLog("receive message to cancel fish coin recharge, user = %s, order = %s, platform id = %d, reason = %d, info = %s, rc = %d",
	userData, cancelFishCoinOrder.order_id().c_str(), cancelFishCoinOrder.platform_id(), cancelFishCoinOrder.reason(), cancelFishCoinOrder.info().c_str(), rc);
}

void CRechargeMgr::rechargeFishCoinResultNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::RechargeFishCoinResultNotify resultNotify;
	if (!parseMsgFromBuffer(resultNotify, data, len, "recharge fish coin result notify to client")) return;

    int rc = m_msgHandler->sendMsgToProxy(data, len, m_request2replyId[FISH_COIN_RECHARGE_NOTIFY], m_msgHandler->getContext().userData);
	
	OptInfoLog("fish coin recharge notify, user = %s, result = %d, money = %.2f, order = %s, fish coin id = %d, amount = %d, give count = %d, rc = %d",
	m_msgHandler->getContext().userData, resultNotify.result(), resultNotify.money(), resultNotify.order_id().c_str(), resultNotify.fish_coin_id(), resultNotify.fish_coin_amount(), resultNotify.give_count(), rc);
	
	// 是否注册监听函数
	ProtocolIdToHandler::iterator handlerIt = m_protocolId2Handler.find(EListenerProtocolFlag::ERechargeResultNotify);
	if (handlerIt != m_protocolId2Handler.end() && handlerIt->second.instance != NULL && handlerIt->second.handler != NULL)
	{
		(handlerIt->second.instance->*(handlerIt->second.handler))(data, len, srcSrvId, srcModuleId, srcProtocolId);
	}
}

// 苹果版本充值验证
void CRechargeMgr::checkAppleRecharge(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userData = m_msgHandler->canToDoOperation(EServiceOperationType::EClientRequest, "check apple recharge result");
	if (userData == NULL) return;

    com_protocol::CheckAppleRechargeResultNotify checkAppleRechargeNotify;
	if (!parseMsgFromBuffer(checkAppleRechargeNotify, data, len, "check apple recharge result request")) return;
	
	int rc = sendMessageToHttpService(data, len, CHECK_APPLE_RECHARGE_RESULT, userData, strlen(userData), ThirdPlatformType::AppleIOS);
	
	OptInfoLog("receive message to check apple recharge result, user = %s, order id = %s, receipt data = %s, rc = %d",
	userData, checkAppleRechargeNotify.order_id().c_str(), checkAppleRechargeNotify.receipt_data().c_str(), rc);
}
	


bool CRechargeMgr::getThirdRechargeTransaction(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen)
{
	com_protocol::GetThirdPlatformRechargeTransactionReq thirdReq;
	if (!parseMsgFromBuffer(thirdReq, data, len, "get third recharge transaction request")) return false;

	int rc = sendMessageToHttpService(data, len, GET_THIRD_RECHARGE_TRANSACTION_REQ, userData, userDataLen, thirdReq.platform_id());
	OptInfoLog("receive message to get third recharge transaction, user = %s, platform id = %d, os type = %d, item type = %d, id = %d, count = %d, rc = %d",
	userData, thirdReq.platform_id(), thirdReq.os_type(), thirdReq.item_type(), thirdReq.item_id(), thirdReq.item_count(), rc);
	
	return (rc == Success);
}

bool CRechargeMgr::getThirdRechargeTransactionReply(const char* data, const unsigned int len, const char* userData, unsigned short protocolId, ConnectProxy* conn)
{
	com_protocol::GetThirdPlatformRechargeTransactionRsp getThirdRsp;
	if (!parseMsgFromBuffer(getThirdRsp, data, len, "get third recharge transaction reply")) return false;
	
	if (getThirdRsp.result() == Success)
	{
	    OptInfoLog("get third recharge transaction, user = %s, money = %.2f, transaction = %s, type = %d, id = %d, count = %d, amount = %d",
		userData, getThirdRsp.money(), getThirdRsp.recharge_transaction().c_str(), getThirdRsp.item_type(), getThirdRsp.item_id(), getThirdRsp.item_count(), getThirdRsp.item_amount());
	}
	else
	{
		OptWarnLog("get third recharge transaction error, user = %s, result = %d", userData, getThirdRsp.result());
	}

	return (Success == m_msgHandler->sendMsgToProxy(data, len, protocolId, conn));
}

bool CRechargeMgr::thirdRechargeTransactionNotify(const char* data, const unsigned int len, const char* userData, unsigned short protocolId, ConnectProxy* conn, int& result)
{
	com_protocol::ThirdPlatformRechargeResultNotify thirdResultNotify;
	if (!parseMsgFromBuffer(thirdResultNotify, data, len, "third recharge notify to client")) return false;

    result = thirdResultNotify.result();
    int rc = m_msgHandler->sendMsgToProxy(data, len, protocolId, conn);

    const char* giftData = "gift";
	unsigned int giftDataLen = 0;
	for (int idx = 0; idx < thirdResultNotify.gift_array_size(); ++idx)
	{
		giftDataLen += snprintf(m_msgBuffer + giftDataLen, NFrame::MaxMsgLen - giftDataLen - 1, "%u=%u&", thirdResultNotify.gift_array(idx).type(), thirdResultNotify.gift_array(idx).num());
	}
	if (giftDataLen > 0)
	{
		m_msgBuffer[giftDataLen - 1] = '\0';
		giftData = m_msgBuffer;
	}
	
	OptInfoLog("third recharge notify, user = %s, result = %d, money = %.2f, transaction = %s, item type = %d, id = %d, count = %d, amount = %d, rc = %d, gift = %s",
				userData, result, thirdResultNotify.money(), thirdResultNotify.recharge_transaction().c_str(),
				thirdResultNotify.item_type(), thirdResultNotify.item_id(), thirdResultNotify.item_count(), thirdResultNotify.item_amount(), rc, giftData);
				
	return (rc == Success);
}

bool CRechargeMgr::cancelThirdRechargeNotify(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen)
{
	com_protocol::CancelThirdPlatformRechargeNotify cancelThirdNotify;
	if (!parseMsgFromBuffer(cancelThirdNotify, data, len, "cancel third recharge notify")) return false;

	int rc = sendMessageToHttpService(data, len, CANCEL_THIRD_RECHARGE_NOTIFY, userData, userDataLen, cancelThirdNotify.platform_id());
	OptInfoLog("receive message to cancel third recharge, user = %s, transaction = %s, platform id = %d, reason = %d, info = %s, rc = %d",
	userData, cancelThirdNotify.recharge_transaction().c_str(), cancelThirdNotify.platform_id(), cancelThirdNotify.reason(), cancelThirdNotify.info().c_str(), rc);
	
	return (rc == Success);
}



bool CRechargeMgr::getDownJoyRechargeTransaction(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen, unsigned short protocolId, ConnectProxy* conn)
{
	com_protocol::GetDownJoyRechargeTransactionReq getDownJoyRechargeTransactionReq;
	if (!parseMsgFromBuffer(getDownJoyRechargeTransactionReq, data, len, "get down joy recharge transaction request")) return false;
	
	bool isOk = false;
	float money = 0;
	unsigned int itemAmount = 0;
	com_protocol::ItemInfo itemInfo;
	itemInfo.set_id(getDownJoyRechargeTransactionReq.itemid());
	if (m_mallMgr.getMallItemInfo(getDownJoyRechargeTransactionReq.itemtype(), itemInfo))
	{
		money = itemInfo.buy_price() * getDownJoyRechargeTransactionReq.itemcount();
		itemAmount = itemInfo.num() * getDownJoyRechargeTransactionReq.itemcount();
		com_protocol::GetDownJoyPaymentNoReq getDownJoyPaymentNoReq;
		getDownJoyPaymentNoReq.set_money(money);
		getDownJoyPaymentNoReq.set_username(userData);
		getDownJoyPaymentNoReq.set_umid(getDownJoyRechargeTransactionReq.umid());
		getDownJoyPaymentNoReq.set_playername(getDownJoyRechargeTransactionReq.playername());
		getDownJoyPaymentNoReq.set_itemtype(getDownJoyRechargeTransactionReq.itemtype());
		getDownJoyPaymentNoReq.set_itemid(getDownJoyRechargeTransactionReq.itemid());
		getDownJoyPaymentNoReq.set_itemcount(getDownJoyRechargeTransactionReq.itemcount());
		getDownJoyPaymentNoReq.set_itemamount(itemAmount);
		
		unsigned int osType = getDownJoyRechargeTransactionReq.has_os_type() ? getDownJoyRechargeTransactionReq.os_type() : ClientVersionType::AndroidMerge;
		getDownJoyPaymentNoReq.set_os_type(osType);

        const char* msgBuffer = serializeMsgToBuffer(getDownJoyPaymentNoReq, "get down joy payment transaction");
	    isOk = (msgBuffer != NULL && (Success == sendMessageToHttpService(msgBuffer, getDownJoyPaymentNoReq.ByteSize(), GET_DOWNJOY_RECHARGE_TRANSACTION_REQ, userData, userDataLen)));
	}
	else
	{
		OptErrorLog("get item price for down joy recharge from mall error, umid = %s, player name = %s, item type = %d, id = %d, count = %d",
		getDownJoyRechargeTransactionReq.umid().c_str(), getDownJoyRechargeTransactionReq.playername().c_str(),
		getDownJoyRechargeTransactionReq.itemtype(), getDownJoyRechargeTransactionReq.itemid(), getDownJoyRechargeTransactionReq.itemcount());
		
		// 直接返回失败消息给客户端
		com_protocol::GetDownJoyRechargeTransactionRsp getDownJoyRechargeTransactionRsp;
		getDownJoyRechargeTransactionRsp.set_result(ProtocolReturnCode::GetPriceFromMallDataError);
		const char* msgBuffer = serializeMsgToBuffer(getDownJoyRechargeTransactionRsp, "get item price for recharge transaction error");
	    isOk = (msgBuffer != NULL && (Success == m_msgHandler->sendMsgToProxy(msgBuffer, getDownJoyRechargeTransactionRsp.ByteSize(), protocolId, conn)));
	}
	
	OptInfoLog("receive message to get down joy recharge transaction, item type = %d, id = %d, count = %d, amount = %d, money = %.2f",
	getDownJoyRechargeTransactionReq.itemtype(), getDownJoyRechargeTransactionReq.itemid(), getDownJoyRechargeTransactionReq.itemcount(), itemAmount, money);
	
	return isOk;
}

bool CRechargeMgr::getDownJoyRechargeTransactionReply(const char* data, const unsigned int len, unsigned short protocolId, ConnectProxy* conn)
{
	com_protocol::GetDownJoyRechargeTransactionRsp getDownJoyRechargeTransactionRsp;
	if (!parseMsgFromBuffer(getDownJoyRechargeTransactionRsp, data, len, "get down joy recharge transaction reply")) return false;
	
	if (getDownJoyRechargeTransactionRsp.result() == Success)
	{
	    OptInfoLog("get down joy recharge transaction, money = %.2f, service name = %s, transaction = %s, type = %d, id = %d, count = %d, amount = %d",
			       getDownJoyRechargeTransactionRsp.money(), getDownJoyRechargeTransactionRsp.servername().c_str(), getDownJoyRechargeTransactionRsp.rechargetransactionno().c_str(),
			       getDownJoyRechargeTransactionRsp.itemtype(), getDownJoyRechargeTransactionRsp.itemid(), getDownJoyRechargeTransactionRsp.itemcount(), getDownJoyRechargeTransactionRsp.itemamount());
	}
	else
	{
		OptWarnLog("get down joy recharge transaction error, result = %d", getDownJoyRechargeTransactionRsp.result());
	}

	return (Success == m_msgHandler->sendMsgToProxy(data, len, protocolId, conn));
}

bool CRechargeMgr::downJoyRechargeTransactionNotify(const char* data, const unsigned int len, unsigned short protocolId, ConnectProxy* conn)
{
	com_protocol::DownJoyRechargeTransactionNotify downJoyRechargeTransactionNotify;
	if (!parseMsgFromBuffer(downJoyRechargeTransactionNotify, data, len, "down joy recharge notify to client")) return false;

    int rc = m_msgHandler->sendMsgToProxy(data, len, protocolId, conn);
	
	const char* giftData = "gift";
	unsigned int giftDataLen = 0;
	for (int idx = 0; idx < downJoyRechargeTransactionNotify.gift_array_size(); ++idx)
	{
		giftDataLen += snprintf(m_msgBuffer + giftDataLen, NFrame::MaxMsgLen - giftDataLen - 1, "%u=%u&",
		downJoyRechargeTransactionNotify.gift_array(idx).type(), downJoyRechargeTransactionNotify.gift_array(idx).num());
	}
	if (giftDataLen > 0)
	{
		m_msgBuffer[giftDataLen - 1] = '\0';
		giftData = m_msgBuffer;
	}
	
	OptInfoLog("down joy recharge notify, result = %d, money = %.2f, transaction = %s, item type = %d, id = %d, count = %d, amount = %d, rc = %d, gift = %s",
				downJoyRechargeTransactionNotify.result(), downJoyRechargeTransactionNotify.money(), downJoyRechargeTransactionNotify.rechargetransactionno().c_str(),
				downJoyRechargeTransactionNotify.itemtype(), downJoyRechargeTransactionNotify.itemid(),
				downJoyRechargeTransactionNotify.itemcount(), downJoyRechargeTransactionNotify.itemamount(), rc, giftData);
				
	return (rc == Success);
}

bool CRechargeMgr::cancelDownJoyRechargeNotify(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen)
{
	com_protocol::CancelDownJoyRechargeNotify cancelDownJoyRechargeNotifyMsg;
	if (!parseMsgFromBuffer(cancelDownJoyRechargeNotifyMsg, data, len, "cancel down joy recharge notify")) return false;
	
	int rc = sendMessageToHttpService(data, len, CANCEL_DOWNJOY_RECHARGE_NOTIFY, userData, userDataLen);
	OptInfoLog("receive message to cancel down joy recharge, user = %s, umid = %s, playername = %s, id = %s, info = %s, rc = %d", userData,
	cancelDownJoyRechargeNotifyMsg.umid().c_str(), cancelDownJoyRechargeNotifyMsg.playername().c_str(), cancelDownJoyRechargeNotifyMsg.rechargetransactionno().c_str(),
	cancelDownJoyRechargeNotifyMsg.info().c_str(), rc);
	
	return (rc == Success);
}



bool CRechargeMgr::findTxRechargeSuccess(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen)
{
	com_protocol::FindRechargeSuccessReq thirdReq;
	if (!parseMsgFromBuffer(thirdReq, data, len, "get tx recharge transaction find request")) return false;

	int rc = sendMessageToHttpService(data, len, FIN_RECHARGE_SUCCESS_REQ, userData, userDataLen, thirdReq.platform_id());
	
	OptInfoLog("receive message to find Tx recharge success, user = %s, platform id = %d, recharge transaction = %s, rc = %d",
	userData, thirdReq.platform_id(), thirdReq.recharge_transaction().c_str(), rc);
	
	return (rc == Success);
}


bool CRechargeMgr::getGoogleRechargeTransaction(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen, unsigned short protocolId, ConnectProxy* conn)
{
	com_protocol::GetOverseasRechargeTransactionReq getOverseasRechargeTransactionReq;
	if (!parseMsgFromBuffer(getOverseasRechargeTransactionReq, data, len, "get google recharge transaction request")) return false;

	bool isOk = false;
	float money = 0;
	unsigned int itemAmount = 0;
	com_protocol::ItemInfo itemInfo;
	itemInfo.set_id(getOverseasRechargeTransactionReq.item_id());
	if (m_mallMgr.getMallItemInfo(getOverseasRechargeTransactionReq.item_type(), itemInfo))
	{
		money = itemInfo.buy_price() * getOverseasRechargeTransactionReq.item_count();
		itemAmount = itemInfo.num() * getOverseasRechargeTransactionReq.item_count();
		com_protocol::GetOverseasPaymentNoReq getOverseasPaymentNoReq;
		getOverseasPaymentNoReq.set_item_type(getOverseasRechargeTransactionReq.item_type());
		getOverseasPaymentNoReq.set_item_id(getOverseasRechargeTransactionReq.item_id());
		getOverseasPaymentNoReq.set_item_count(getOverseasRechargeTransactionReq.item_count());
		getOverseasPaymentNoReq.set_item_amount(itemAmount);
		getOverseasPaymentNoReq.set_os_type(getOverseasRechargeTransactionReq.os_type());

        const char* msgBuffer = serializeMsgToBuffer(getOverseasPaymentNoReq, "get google payment transaction");
	    isOk = (msgBuffer != NULL && (Success == sendMessageToHttpService(msgBuffer, getOverseasPaymentNoReq.ByteSize(), GET_GOOGLE_RECHARGE_TRANSACTION_REQ, userData, userDataLen)));
	}
	else
	{
		OptErrorLog("get item price for google payment from mall error, user data = %s, item type = %d, id = %d, count = %d",
		userData, getOverseasRechargeTransactionReq.item_type(), getOverseasRechargeTransactionReq.item_id(), getOverseasRechargeTransactionReq.item_count());
		
		// 直接返回失败消息给客户端
		com_protocol::GetOverseasRechargeTransactionRsp getOverseasRechargeTransactionRsp;
		getOverseasRechargeTransactionRsp.set_result(ProtocolReturnCode::GetPriceFromMallDataError);
		const char* msgBuffer = serializeMsgToBuffer(getOverseasRechargeTransactionRsp, "get item price for google recharge transaction error");
	    isOk = (msgBuffer != NULL && (Success == m_msgHandler->sendMsgToProxy(msgBuffer, getOverseasRechargeTransactionRsp.ByteSize(), protocolId, conn)));
	}
	
	OptInfoLog("receive message to get google recharge transaction, user data = %s, item type = %d, id = %d, count = %d, amount = %d, money = %.2f",
	userData, getOverseasRechargeTransactionReq.item_type(), getOverseasRechargeTransactionReq.item_id(), getOverseasRechargeTransactionReq.item_count(), itemAmount, money);
	
	return isOk;
}

bool CRechargeMgr::getGoogleRechargeTransactionReply(const char* data, const unsigned int len, unsigned short protocolId, ConnectProxy* conn)
{
	com_protocol::GetOverseasRechargeTransactionRsp getOverseasRechargeTransactionRsp;
	if (!parseMsgFromBuffer(getOverseasRechargeTransactionRsp, data, len, "get google recharge transaction reply")) return false;
	
	if (getOverseasRechargeTransactionRsp.result() == Success)
	{
	    OptInfoLog("get google recharge transaction, transaction = %s, type = %d, id = %d, count = %d, amount = %d", getOverseasRechargeTransactionRsp.recharge_transaction().c_str(),
			       getOverseasRechargeTransactionRsp.item_type(), getOverseasRechargeTransactionRsp.item_id(), getOverseasRechargeTransactionRsp.item_count(), getOverseasRechargeTransactionRsp.item_amount());
	}
	else
	{
		OptWarnLog("get google recharge transaction error, result = %d", getOverseasRechargeTransactionRsp.result());
	}

	return (Success == m_msgHandler->sendMsgToProxy(data, len, protocolId, conn));
}

bool CRechargeMgr::checkGoogleRechargeResult(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen)
{
	com_protocol::OverseasRechargeResultCheckReq overseasRechargeResultCheckReq;
	if (!parseMsgFromBuffer(overseasRechargeResultCheckReq, data, len, "check google recharge result request")) return false;

    int rc = sendMessageToHttpService(data, len, CHECK_GOOGLE_RECHARGE_RESULT_REQ, userData, userDataLen);
	OptInfoLog("check google recharge, result = %d, money = %.2f, transaction = %s, rc = %d",
				overseasRechargeResultCheckReq.result(), overseasRechargeResultCheckReq.money(), overseasRechargeResultCheckReq.recharge_transaction().c_str(), rc);

	return (rc == Success);
}

bool CRechargeMgr::checkGoogleRechargeResultReply(const char* data, const unsigned int len, unsigned short protocolId, ConnectProxy* conn, int& result)
{
	com_protocol::OverseasRechargeResultCheckRsp overseasRechargeResultCheckRsp;
	if (!parseMsgFromBuffer(overseasRechargeResultCheckRsp, data, len, "check google recharge result reply")) return false;
	
	result = overseasRechargeResultCheckRsp.result();
	int rc = m_msgHandler->sendMsgToProxy(data, len, protocolId, conn);
	
	const char* giftData = "gift";
	unsigned int giftDataLen = 0;
	for (int idx = 0; idx < overseasRechargeResultCheckRsp.gift_array_size(); ++idx)
	{
		giftDataLen += snprintf(m_msgBuffer + giftDataLen, NFrame::MaxMsgLen - giftDataLen - 1, "%u=%u&",
		overseasRechargeResultCheckRsp.gift_array(idx).type(), overseasRechargeResultCheckRsp.gift_array(idx).num());
	}
	if (giftDataLen > 0)
	{
		m_msgBuffer[giftDataLen - 1] = '\0';
		giftData = m_msgBuffer;
	}
	
	OptInfoLog("check google recharge, result = %d, type = %d, id = %d, count = %d, amount = %d, rc = %d, gift = %s", overseasRechargeResultCheckRsp.result(),
			   overseasRechargeResultCheckRsp.item_type(), overseasRechargeResultCheckRsp.item_id(), overseasRechargeResultCheckRsp.item_count(),
			   overseasRechargeResultCheckRsp.item_amount(), rc, giftData);
	
	return (rc == Success);
}

}

