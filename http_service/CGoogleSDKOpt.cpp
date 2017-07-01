
/* author : limingfan
 * date : 2015.10.28
 * description : Google SDK 操作实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "CGoogleSDKOpt.h"
#include "CHttpSrv.h"
#include "CHttpData.h"
#include "CThirdInterface.h"
#include "CThirdPlatformOpt.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"
#include "common/CommonType.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace NProject;


// 充值日志文件
// #define GoogleRechargeLog(format, args...)     getRechargeLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)
#define GoogleRechargeLog(format, args...)     m_platformatOpt->getThirdRechargeLogger(ThirdPlatformType::GoogleSDK).writeFile(NULL, 0, LogLevel::Info, format, ##args)


namespace NPlatformService
{

/*
// Google SDK 签名验证的公钥
static const char* GooglePublicKey = "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA69yfeBfCBHCdlniCJmxJ\nl2/pQz98yALZ4BoYoaIEyCTtzcGl2UJNQ0FInzauJlX9S3OFjjasxjZy2awwhTeg\nZtpIxOIvMxUyA1UxhUNqn3P2l5g/woUZAD99916NkukJhkWUl5nPldMHlfIKfjm8\nIiG1UckdhSmIPlVO+wUeYJIVeHx3SE5as+eW4glw87KkQrpFNw7e8tBF/IOAM5qK\nyvf16SRn4r6SOhBdOPl2oouQYNQv+4SSCp6f/FXO17XXnB33jjtOkCfBKZerJ6Gs\ncqTqKug0BbzDtNs86kEqkIwVkguc5Vvzg5aJCq4A97CW2IIQqf464LF0xifOfo8J\nBQIDAQAB\n-----END PUBLIC KEY-----\n";
static const unsigned int GooglePublicKeyLen = strlen(GooglePublicKey);
*/

static const char* GoogleCfgItem = "Google";


CGoogleSDKOpt::CGoogleSDKOpt()
{
	m_msgHandler = NULL;
	m_platformatOpt = NULL;
	m_tokenInfo.lastSecs = 0;
	m_rechargeTransactionIdx = 0;
}

CGoogleSDKOpt::~CGoogleSDKOpt()
{
	m_msgHandler = NULL;
	m_platformatOpt = NULL;
	m_tokenInfo.lastSecs = 0;
	m_rechargeTransactionIdx = 0;
}

/*
CLogger& CGoogleSDKOpt::getRechargeLogger()
{
	static CLogger rechargeLogger(CCfg::getValue(GoogleCfgItem, "RechargeLogFileName"), atoi(CCfg::getValue(GoogleCfgItem, "RechargeLogFileSize")), atoi(CCfg::getValue(GoogleCfgItem, "RechargeLogFileCount")));
	return rechargeLogger;
}
*/

bool CGoogleSDKOpt::load(CSrvMsgHandler* msgHandler, CThirdPlatformOpt* platformatOpt)
{
	m_msgHandler = msgHandler;
	m_platformatOpt = platformatOpt;

	bool isOk = true;
	const char* runValue = CCfg::getValue(GoogleCfgItem, "CheckHost");  // 检查是否使用启动Google SDK处理流程
	while (runValue != NULL && atoi(runValue) == 1)
	{
		// Google SDK 配置项值
		const char* googleCfg[] = {"RechargeLogFileSize", "RechargeLogFileCount", "RechargeLogFileName", "Host", "Url", "Protocol",
		"ClientId", "ClientKey", "RefreshToken", "TokenHost", "TokenUrl",};
		for (int i = 0; i < (int)(sizeof(googleCfg) / sizeof(googleCfg[0])); ++i)
		{
			const char* value = CCfg::getValue(GoogleCfgItem, googleCfg[i]);
			if (value == NULL)
			{
				OptErrorLog("google config item error, not find key = %s", googleCfg[i]);
				return false;
			}
			
			if ((i == 0 && (unsigned int)atoi(value) < MinRechargeLogFileSize)
				|| (i == 1 && (unsigned int)atoi(value) < MinRechargeLogFileCount))
			{
				OptErrorLog("google config item error, recharge log file min size = %d, count = %d", MinRechargeLogFileSize, MinRechargeLogFileCount);
				return false;
			}
		}
	
		isOk = m_msgHandler->getHostInfo(CCfg::getValue(GoogleCfgItem, "Host"), m_tokenInfo.checkPayIp, m_tokenInfo.checkPayPort, CCfg::getValue(GoogleCfgItem, "Protocol"));
		if (!isOk) break;
		
		isOk = m_msgHandler->getHostInfo(CCfg::getValue(GoogleCfgItem, "TokenHost"), m_tokenInfo.tokenIp, m_tokenInfo.tokenPort, "https");
		if (!isOk) break;
		
		m_msgHandler->registerProtocol(CommonSrv, GET_GOOGLE_RECHARGE_TRANSACTION_REQ, (ProtocolHandler)&CGoogleSDKOpt::getRechargeTransaction, this);
		m_msgHandler->registerProtocol(CommonSrv, CHECK_GOOGLE_RECHARGE_RESULT_REQ, (ProtocolHandler)&CGoogleSDKOpt::checkRechargeResult, this);
		
		m_msgHandler->registerHttpReplyHandler(HttpReplyHandlerID::GetGoogleSDKAccessToken, (HttpReplyHandler)&CGoogleSDKOpt::getAccessTokenReply, this);
		m_msgHandler->registerHttpReplyHandler(HttpReplyHandlerID::UpdateGoogleSDKAccessToken, (HttpReplyHandler)&CGoogleSDKOpt::updateAccessTokenReply, this);
		m_msgHandler->registerHttpReplyHandler(HttpReplyHandlerID::CheckGoogleSDKPay, (HttpReplyHandler)&CGoogleSDKOpt::checkRechargeResultExReply, this);

        isOk = getAccessToken(HttpReplyHandlerID::GetGoogleSDKAccessToken);
		break;
	}
	
	return isOk;
}

void CGoogleSDKOpt::unload()
{
	
}


void CGoogleSDKOpt::getRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetOverseasPaymentNoReq msg;
	if (!m_msgHandler->parseMsgFromBuffer(msg, data, len, "get google recharge transaction")) return;
	
	unsigned int platformType = ThirdPlatformType::GoogleSDK;  // 第三方平台类型
	if (msg.os_type() == ClientVersionType::AppleMerge)  // 苹果版本
	{
		platformType = platformType * ThirdPlatformType::ClientDeviceBase + platformType;  // 转换为苹果版本值
	}
	else if (msg.os_type() != ClientVersionType::AndroidMerge)  // 非安卓合并版本
	{
		OptErrorLog("getRechargeTransaction, the google SDK os type invalid, type = %d", msg.os_type());
		return;
	}
	
	char rechargeTranscation[MaxRechargeTransactionLen] = {'\0'};
	const int maxRandNumber = 1000000;
	time_t curSecs = time(NULL);
	struct tm* tmval = localtime(&curSecs);
	
	// 订单号格式 : rand.time.platform.order-type.id.count.amount-srv.user
	// 10869.20150621153831.1.1-1.1003.1.1000-1001.10000003
	snprintf(rechargeTranscation, MaxRechargeTransactionLen - 1, "%d.%d%02d%02d%02d%02d%02d.%d.%d-%d.%d.%d.%d-%d.%s",
	getRandNumber(1, maxRandNumber), tmval->tm_year + 1900, tmval->tm_mon + 1, tmval->tm_mday, tmval->tm_hour, tmval->tm_min, tmval->tm_sec,
	platformType, ++m_rechargeTransactionIdx, msg.item_type(), msg.item_id(), msg.item_count(), msg.item_amount(), srcSrvId, m_msgHandler->getContext().userData);
	
	// 存储订单信息
	BuyInfo buyInfo;
	buyInfo.itemType = msg.item_type();
	buyInfo.itemId = msg.item_id();
	buyInfo.itemCount = msg.item_count();
	buyInfo.itemAmount = msg.item_amount();
	buyInfo.srcSrvId = 0;
	
	com_protocol::GetOverseasRechargeTransactionRsp getOverseasRechargeTransactionRsp;
	getOverseasRechargeTransactionRsp.set_result(OptSuccess);
	getOverseasRechargeTransactionRsp.set_recharge_transaction(rechargeTranscation);
	getOverseasRechargeTransactionRsp.set_item_type(buyInfo.itemType);
	getOverseasRechargeTransactionRsp.set_item_id(buyInfo.itemId);
	getOverseasRechargeTransactionRsp.set_item_count(buyInfo.itemCount);
	getOverseasRechargeTransactionRsp.set_item_amount(buyInfo.itemAmount);
	
	int rc = -1;
	const char* msgBuff = m_msgHandler->serializeMsgToBuffer(getOverseasRechargeTransactionRsp, "send google recharge transaction");
	if (msgBuff != NULL)
	{
		rc = m_msgHandler->sendMessage(msgBuff, getOverseasRechargeTransactionRsp.ByteSize(), srcSrvId, GET_GOOGLE_RECHARGE_TRANSACTION_RSP);
		if (rc == Success)
		{
			m_recharge2BuyInfo[rechargeTranscation] = buyInfo;  // 存储订单信息

			// 充值订单信息记录
			// format = name|type|id|count|amount|money|rechargeNo|request|platformType
			GoogleRechargeLog("%s|%d|%d|%d|%d|%.2f|%s|request|%d", m_msgHandler->getContext().userData, buyInfo.itemType, buyInfo.itemId, buyInfo.itemCount, buyInfo.itemAmount, 0.00,
			rechargeTranscation, platformType);
		}
	}
	
	OptInfoLog("get google SDK recharge transaction, money = %.2f, user = %s, bill id = %s, ostype = %d, rc = %d", 
			   0.00, m_msgHandler->getContext().userData, rechargeTranscation, msg.os_type(), rc);
}

void CGoogleSDKOpt::checkRechargeResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::OverseasRechargeResultCheckReq msg;
	if (!m_msgHandler->parseMsgFromBuffer(msg, data, len, "check google recharge transaction")) return;
	
	// 兼容老版本，检查是否是新版本的新校验方式
	if (msg.has_order_id() && msg.has_package_name() && msg.has_product_id() && msg.has_purchase_token()) return checkRechargeResultEx(msg, srcSrvId);
	
	// only for log
	OptErrorLog("checkRechargeResult message, user = %s, result = %d, recharge transaction = %s, money = %.2f, sign data = %s, result = %s",
				m_msgHandler->getContext().userData, msg.result(), msg.recharge_transaction().c_str(), msg.money(), msg.sign_data().c_str(), msg.sign_result().c_str());
	
	
	/*
	RechargeTranscationToBuyInfo::iterator it = m_recharge2BuyInfo.find(msg.recharge_transaction());
	if (msg.result() != OptSuccess)
	{
		if (it != m_recharge2BuyInfo.end()) m_recharge2BuyInfo.erase(it);

		// 充值订单信息记录
		// format = name|rechargeNo|cancel|result
		GoogleRechargeLog("%s|%s|cancel|%d", m_msgHandler->getContext().userData, msg.recharge_transaction().c_str(), msg.result());
	
		OptWarnLog("check google SDK recharge transaction cancel, user = %s, bill id = %s, result = %d",
		m_msgHandler->getContext().userData, msg.recharge_transaction().c_str(), msg.result());
		
		return;
	}
	
	const char* msgBuff = NULL;
	com_protocol::OverseasRechargeResultCheckRsp rsp;
	if (it == m_recharge2BuyInfo.end())
	{
		// format = name|rechargeNo|money|error|result
		GoogleRechargeLog("%s|%s|%.2f|error|%d", m_msgHandler->getContext().userData, msg.recharge_transaction().c_str(), msg.money(), msg.result());
		
		OptErrorLog("check google SDK recharge transaction error, user = %s, bill id = %s, money = %.2f, result = %d",
		m_msgHandler->getContext().userData, msg.recharge_transaction().c_str(), msg.money(), msg.result());
		
		rsp.set_result(NoFoundGoogleRechargeTranscation);
		msgBuff = m_msgHandler->serializeMsgToBuffer(rsp, "send check google recharge transaction error result");
		if (msgBuff != NULL) m_msgHandler->sendMessage(msgBuff, rsp.ByteSize(), srcSrvId, CHECK_GOOGLE_RECHARGE_RESULT_RSP);
		
		return;
	}
	
	rsp.set_result(OptSuccess);
	//  目前搞不清楚Google的安全认证方式，所有先屏蔽不做安全校验
	// 先base64解码签名内容
	rsp.set_result(GoogleSignVerityError);
	const unsigned int SignResultMaxLen = 1024;
	char signResult[SignResultMaxLen] = {0};
	int signResultLen = base64Decode(msg.sign_result().c_str(), msg.sign_result().length(), signResult, SignResultMaxLen);
	if (signResultLen > 1)
	{
		// SHA1摘要做RSA验证
		bool verifyOk = sha1RSAVerify(GooglePublicKey, GooglePublicKeyLen, (const unsigned char*)msg.sign_data().c_str(), msg.sign_data().length(),
									  (const unsigned char*)signResult, signResultLen);
	    if (verifyOk) rsp.set_result(OptSuccess);
	}
	// 
	
	int rc = -1;
	if (rsp.result() == OptSuccess)
	{
		com_protocol::ItemInfo itemInfo;
		itemInfo.set_id(it->second.itemId);
		// if (m_msgHandler->getMallMgr().getItemInfo(it->second.itemType, itemInfo))
		{
			msg.set_money(itemInfo.buy_price() * it->second.itemCount);
		}
		
		// 校验成功则发放物品
		com_protocol::UserRechargeReq userRechargeReq;
		userRechargeReq.set_bills_id(msg.recharge_transaction());
		userRechargeReq.set_username(m_msgHandler->getContext().userData);
		userRechargeReq.set_item_type(it->second.itemType);
		userRechargeReq.set_item_id(it->second.itemId);
		userRechargeReq.set_item_count(it->second.itemCount);
		userRechargeReq.set_item_flag(0); // m_msgHandler->getMallMgr().getItemFlag(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, it->second.itemId));
		userRechargeReq.set_recharge_rmb(msg.money());
		userRechargeReq.set_third_account(msg.sign_result());
		userRechargeReq.set_third_type(ThirdPlatformType::GoogleSDK);
		userRechargeReq.set_third_other_data(msg.sign_result());
		msgBuff = m_msgHandler->serializeMsgToBuffer(userRechargeReq, "send check google recharge result to db proxy");
		if (msgBuff != NULL)
		{
			rc = m_msgHandler->sendMessageToCommonDbProxy(msgBuff, userRechargeReq.ByteSize(), BUSINESS_USER_RECHARGE_REQ, m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
		}
	}
	else
	{
		// 失败则不会发放物品，直接通知客户端
		msgBuff = m_msgHandler->serializeMsgToBuffer(rsp, "send check google recharge transaction error result to client");
		if (msgBuff != NULL) m_msgHandler->sendMessage(msgBuff, rsp.ByteSize(), srcSrvId, CHECK_GOOGLE_RECHARGE_RESULT_RSP);
	}
	
	// 充值订单信息记录
    // format = name|type|id|count|amount|money|rechargeNo|reply|result|sign|sign result
	GoogleRechargeLog("%s|%d|%d|%d|%d|%.2f|%s|reply|%d|%s|%s", m_msgHandler->getContext().userData, it->second.itemType, it->second.itemId, it->second.itemCount, it->second.itemAmount,
	msg.money(), msg.recharge_transaction().c_str(), rsp.result(), msg.sign_data().c_str(), msg.sign_result().c_str());
	
	OptInfoLog("check google SDK recharge transaction, rc = %d, user = %s, bill id = %s, result = %d",
	rc, m_msgHandler->getContext().userData, msg.recharge_transaction().c_str(), rsp.result());
	
	if (rc == OptSuccess) it->second.srcSrvId = srcSrvId;
	else m_recharge2BuyInfo.erase(it);
	*/ 
}

void CGoogleSDKOpt::checkRechargeResultEx(const com_protocol::OverseasRechargeResultCheckReq& msg, unsigned int srcSrvId)
{
    // only for log
	OptWarnLog("checkRechargeResultEx message, user = %s, result = %d, recharge transaction = %s, money = %.2f, order id = %s, package name = %s, product id = %s, purchase token = %s",
			   m_msgHandler->getContext().userData, msg.result(), msg.recharge_transaction().c_str(), msg.money(), msg.order_id().c_str(), msg.package_name().c_str(), msg.product_id().c_str(), msg.purchase_token().c_str());
	
	// 老版本订单，兼容处理
	RechargeTranscationToBuyInfo::iterator it = m_recharge2BuyInfo.find(msg.recharge_transaction());
	
	// 新版本订单，查询渔币充值内部订单
	FishCoinOrderIdToInfo::iterator thirdInfoIt;
	const bool isFishCoinOrder = m_platformatOpt->getThirdInstance().getTranscationData(msg.recharge_transaction().c_str(), thirdInfoIt);
	
	if (msg.result() != OptSuccess)
	{
		// 充值订单信息记录
		// format = name|rechargeNo|cancel|order|result
		GoogleRechargeLog("%s|%s|cancel|%s|%d", m_msgHandler->getContext().userData, msg.recharge_transaction().c_str(), msg.order_id().c_str(), msg.result());
			
		if (it != m_recharge2BuyInfo.end())
		{
			m_recharge2BuyInfo.erase(it);
		
			OptWarnLog("check google SDK recharge transaction cancel, user = %s, bill id = %s, order id = %s, result = %d",
			m_msgHandler->getContext().userData, msg.recharge_transaction().c_str(), msg.order_id().c_str(), msg.result());
		}
		else
		{
			if (isFishCoinOrder) m_platformatOpt->getThirdInstance().removeTransactionData(thirdInfoIt);

			OptWarnLog("cancel google SDK fish coin recharge order, user = %s, bill id = %s, order id = %s, result = %d",
			m_msgHandler->getContext().userData, msg.recharge_transaction().c_str(), msg.order_id().c_str(), msg.result());
		}
		
		return;
	}
	
	if (it == m_recharge2BuyInfo.end() && !isFishCoinOrder)
	{
		// format = name|rechargeNo|money|error|order|result
		GoogleRechargeLog("%s|%s|%.2f|error|%s|%d", m_msgHandler->getContext().userData, msg.recharge_transaction().c_str(), msg.money(), msg.order_id().c_str(), msg.result());
		
		OptErrorLog("check google SDK recharge order error, user = %s, bill id = %s, order id = %s, money = %.2f, result = %d",
		m_msgHandler->getContext().userData, msg.recharge_transaction().c_str(), msg.order_id().c_str(), msg.money(), msg.result());
		
		com_protocol::OverseasRechargeResultCheckRsp rsp;
		rsp.set_result(NoFoundGoogleRechargeTranscation);
		const char* msgBuff = m_msgHandler->serializeMsgToBuffer(rsp, "send check google recharge transaction error result");
		if (msgBuff != NULL) m_msgHandler->sendMessage(msgBuff, rsp.ByteSize(), srcSrvId, CHECK_GOOGLE_RECHARGE_RESULT_RSP);
		
		return;
	}
	
	if (time(NULL) < m_tokenInfo.lastSecs)  // token 是否在有效期内
	{
		checkRechargeResultExImpl(m_msgHandler->getContext().userData, srcSrvId, msg.recharge_transaction().c_str(), msg.order_id().c_str(), msg.package_name().c_str(), msg.product_id().c_str(), msg.purchase_token().c_str());
	}
	else
	{
		// 先更新访问token
		UpdateAccessTokenInfo upTkInfo;
		strncpy(upTkInfo.rechargeTransaction, msg.recharge_transaction().c_str(), CbDataMaxSize - 1);
		strncpy(upTkInfo.googleOrderId, msg.order_id().c_str(), CbDataMaxSize - 1);
		strncpy(upTkInfo.packageName, msg.package_name().c_str(), CbDataMaxSize - 1);
		strncpy(upTkInfo.productId, msg.product_id().c_str(), CbDataMaxSize - 1);
		strncpy(upTkInfo.purchaseToken, msg.purchase_token().c_str(), CbDataMaxSize - 1);
		getAccessToken(HttpReplyHandlerID::UpdateGoogleSDKAccessToken, srcSrvId, m_msgHandler->getContext().userData, &upTkInfo, sizeof(upTkInfo));
	}
}

void CGoogleSDKOpt::checkRechargeResultExImpl(const char* keyData, unsigned int srvId, const char* rechargeTransaction, const char* googleOrderId, const char* packageName, const char* productId, const char* purchaseToken)
{
	// https://www.googleapis.com/androidpublisher/v2/applications/{packageName}/purchases/products/{productId}/tokens/{purchaseToken}?access_token={access_token}
	ConnectData* cd = m_msgHandler->getConnectData();
	memset(cd, 0, sizeof(ConnectData));
	
	cd->timeOutSecs = HttpConnectTimeOut;
	strncpy(cd->keyData.key, keyData, MaxKeySize - 1);
	cd->keyData.flag = HttpReplyHandlerID::CheckGoogleSDKPay;
	cd->keyData.srvId = srvId;
	cd->keyData.protocolId = 0;
	
	char url[MaxNetBuffSize] = {0};
	snprintf(url, sizeof(url) - 1, "%s/%s/purchases/products/%s/tokens/%s",
	CCfg::getValue(GoogleCfgItem, "Url"), packageName, productId, purchaseToken);
	
	CHttpRequest httpRequest(HttpMsgType::GET);
	httpRequest.setParamValue("access_token", m_tokenInfo.accessToken);
	cd->sendDataLen = MaxNetBuffSize;
	
	bool result = true;
	while (result)
	{
		const char* msgData = httpRequest.getMessage(cd->sendData, cd->sendDataLen, CCfg::getValue(GoogleCfgItem, "Host"), url);
		if (msgData == NULL)
		{
			m_msgHandler->putConnectData(cd);
			OptErrorLog("get http request message to check google recharge error, user = %s, bill = %s, google order = %s", keyData, rechargeTransaction, googleOrderId);
			result = false;
			break;
		}

		result = m_msgHandler->doHttpConnect(m_tokenInfo.checkPayIp, m_tokenInfo.checkPayPort, cd);
		if (result)
		{
			unsigned int buffLen = 0;
			CheckPayInfo* ckpInfo = (CheckPayInfo*)m_msgHandler->getDataBuffer(buffLen);  // 可以直接使用这里内存管理的内存块
			ckpInfo->flag = 0;  // 由框架底层释放内存
			strncpy(ckpInfo->rechargeTransaction, rechargeTransaction, CbDataMaxSize - 1);
			strncpy(ckpInfo->googleOrderId, googleOrderId, CbDataMaxSize - 1);
			cd->cbData = ckpInfo;
		}
		else
		{
			m_msgHandler->putConnectData(cd);
			OptErrorLog("do http connect to check pay error, ip = %s, port = %d, user = %s, bill = %s, google order = %s",
			m_tokenInfo.checkPayIp, m_tokenInfo.checkPayPort, keyData, rechargeTransaction, googleOrderId);
			break;
		}
		
		OptInfoLog("do http connect to check pay, user = %s, bill = %s, google order = %s, flag = %u, message len = %u, content = %s",
		cd->keyData.key, rechargeTransaction, googleOrderId, cd->keyData.flag, cd->sendDataLen, cd->sendData);
		
		break;
	}
	
	if (!result)
	{
		// format = name|rechargeNo|failed|order
		GoogleRechargeLog("%s|%s|failed|%s", keyData, rechargeTransaction, googleOrderId);
		
		OptErrorLog("do http to check google recharge error, user = %s, bill id = %s, order id = %s", keyData, rechargeTransaction, googleOrderId);
		
		/*
		// 新版本订单，查询渔币充值内部订单
		FishCoinOrderIdToInfo::iterator thirdInfoIt;
		if (m_platformatOpt->getThirdInstance().getTranscationData(rechargeTransaction, thirdInfoIt))
		{
			m_platformatOpt->getThirdInstance().provideRechargeItem(1, thirdInfoIt, ThirdPlatformType::GoogleSDK, googleOrderId, productId);
		}
		*/ 
	}
}

bool CGoogleSDKOpt::checkFishCoinRechargeResultReply(ConnectData* cd)
{
	// 新版本订单，查询渔币充值内部订单
	const CheckPayInfo* ckpInfo = (const CheckPayInfo*)cd->cbData;
	FishCoinOrderIdToInfo::iterator thirdInfoIt;
	if (!m_platformatOpt->getThirdInstance().getTranscationData(ckpInfo->rechargeTransaction, thirdInfoIt))
	{
		// format = name|rechargeNo|error|order
		GoogleRechargeLog("%s|%s|error|%s|order", cd->keyData.key, ckpInfo->rechargeTransaction, ckpInfo->googleOrderId);
		
		OptErrorLog("check google SDK fish coin recharge order error, can not find the recharge order, user = %s, bill id = %s, order id = %s",
		cd->keyData.key, ckpInfo->rechargeTransaction, ckpInfo->googleOrderId);

		return false;
	}
	
	/* 正确的应答消息内容格式
	{
	  "kind": "androidpublisher#productPurchase",
	  "purchaseTimeMillis": long,
	  "purchaseState": integer,
	  "consumptionState": integer,
	  "developerPayload": string
	}
	 
	consumptionState	integer	The consumption state of the inapp product. Possible values are:
	0 : Yet to be consumed
	1 : Consumed
	developerPayload	string	A developer-specified string that contains supplemental information about an order.	
	kind	string	This kind represents an inappPurchase object in the androidpublisher service.	
	purchaseState	integer	The purchase state of the order. Possible values are:
	0 : Purchased
	1 : Cancelled
	purchaseTimeMillis	long	The time the product was purchased, in milliseconds since the epoch (Jan 1, 1970).
	*/

	int result = GoogleSignVerityError;
	ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
	if (paramKey2Value.size() > 0)
	{
		result = OptSuccess;
		const char* checkPayItem[] = {"purchaseState", "kind", "purchaseTimeMillis", "consumptionState", "developerPayload",};
		for (int i = 0; i < (int)(sizeof(checkPayItem) / sizeof(checkPayItem[0])); ++i)
		{
			ParamKey2Value::iterator checkIt = paramKey2Value.find(checkPayItem[i]);
			if (checkIt == paramKey2Value.end())
			{
				result = GoogleSignVerityError;
				OptErrorLog("check google SDK fish coin recharge order error, can not find the item = %s", checkPayItem[i]);
				break;
			}
			
			if (i == 0 && checkIt->second != "0")
			{
				result = GoogleCheckPayFailed;
				OptErrorLog("check google SDK fish coin recharge order error, the purchase state = %s", checkIt->second.c_str());
				break;
			}
		}
	}
	
	if (result != OptSuccess)
	{
		OptErrorLog("check google SDK fish coin recharge order failed, user = %s, bill id = %s, order id = %s, result = %d",
		cd->keyData.key, ckpInfo->rechargeTransaction, ckpInfo->googleOrderId, result);
	}
	
	// 充值订单信息记录
    // format = name|id|amount|money|rechargeNo|srcSrvId|reply|result|order|consumptionState|time
	const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
	GoogleRechargeLog("%s|%u|%u|%.2f|%s|%u|reply|%d|%s|%s|%s", cd->keyData.key, thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, ckpInfo->rechargeTransaction, thirdInfo.srcSrvId,
	result, ckpInfo->googleOrderId, paramKey2Value.at("consumptionState").c_str(), paramKey2Value.at("purchaseTimeMillis").c_str());
	
	m_platformatOpt->getThirdInstance().provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::GoogleSDK, ckpInfo->googleOrderId, paramKey2Value.at("consumptionState").c_str());
	
	return true;
}


bool CGoogleSDKOpt::checkRechargeResultExReply(ConnectData* cd)
{
	com_protocol::OverseasRechargeResultCheckRsp rsp;
	const char* msgBuff = NULL;
	const CheckPayInfo* ckpInfo = (const CheckPayInfo*)cd->cbData;
	
	// 新版本订单，查询渔币充值内部订单
	FishCoinOrderIdToInfo::iterator thirdInfoIt;
	if (m_platformatOpt->getThirdInstance().getTranscationData(ckpInfo->rechargeTransaction, thirdInfoIt)) return checkFishCoinRechargeResultReply(cd);  // 新版本渔币充值
	
	RechargeTranscationToBuyInfo::iterator buyInfoIt = m_recharge2BuyInfo.find(ckpInfo->rechargeTransaction);
	if (buyInfoIt == m_recharge2BuyInfo.end())
	{
		// format = name|rechargeNo|error|order
		GoogleRechargeLog("%s|%s|error|%s|transaction", cd->keyData.key, ckpInfo->rechargeTransaction, ckpInfo->googleOrderId);
		
		OptErrorLog("check google SDK recharge transaction error, can not find the recharge transaction, user = %s, bill id = %s, order id = %s",
		cd->keyData.key, ckpInfo->rechargeTransaction, ckpInfo->googleOrderId);
		
		rsp.set_result(NoFoundGoogleRechargeTranscation);
		msgBuff = m_msgHandler->serializeMsgToBuffer(rsp, "send check google recharge transaction error result");
		if (msgBuff != NULL) m_msgHandler->sendMessage(msgBuff, rsp.ByteSize(), cd->keyData.srvId, CHECK_GOOGLE_RECHARGE_RESULT_RSP);
		
		return checkFishCoinRechargeResultReply(cd);  // 新版本渔币充值
	}
	
	/* 正确的应答消息内容格式
	{
	  "kind": "androidpublisher#productPurchase",
	  "purchaseTimeMillis": long,
	  "purchaseState": integer,
	  "consumptionState": integer,
	  "developerPayload": string
	}
	 
	consumptionState	integer	The consumption state of the inapp product. Possible values are:
	0 : Yet to be consumed
	1 : Consumed
	developerPayload	string	A developer-specified string that contains supplemental information about an order.	
	kind	string	This kind represents an inappPurchase object in the androidpublisher service.	
	purchaseState	integer	The purchase state of the order. Possible values are:
	0 : Purchased
	1 : Cancelled
	purchaseTimeMillis	long	The time the product was purchased, in milliseconds since the epoch (Jan 1, 1970).
	*/
	
	rsp.set_result(GoogleSignVerityError);
	ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
	if (paramKey2Value.size() > 0)
	{
		rsp.set_result(OptSuccess);
		const char* checkPayItem[] = {"purchaseState", "kind", "purchaseTimeMillis", "consumptionState", "developerPayload",};
		for (int i = 0; i < (int)(sizeof(checkPayItem) / sizeof(checkPayItem[0])); ++i)
		{
			ParamKey2Value::iterator checkIt = paramKey2Value.find(checkPayItem[i]);
			if (checkIt == paramKey2Value.end())
			{
				rsp.set_result(GoogleSignVerityError);
				OptErrorLog("check google SDK recharge transaction error, can not find the item = %s", checkPayItem[i]);
				break;
			}
			
			if (i == 0 && checkIt->second != "0")
			{
				rsp.set_result(GoogleCheckPayFailed);
				OptErrorLog("check google SDK recharge transaction error, the purchase state = %s", checkIt->second.c_str());
				break;
			}
		}
	}
	
	if (rsp.result() != OptSuccess)
	{
		// format = name|rechargeNo|error|order|result|response
		GoogleRechargeLog("%s|%s|error|%s|%d|response", cd->keyData.key, ckpInfo->rechargeTransaction, ckpInfo->googleOrderId, rsp.result());
		
		OptErrorLog("check google SDK recharge transaction failed, user = %s, bill id = %s, order id = %s", cd->keyData.key, ckpInfo->rechargeTransaction, ckpInfo->googleOrderId);

		msgBuff = m_msgHandler->serializeMsgToBuffer(rsp, "send check google recharge transaction error result");
		if (msgBuff != NULL) m_msgHandler->sendMessage(msgBuff, rsp.ByteSize(), cd->keyData.srvId, CHECK_GOOGLE_RECHARGE_RESULT_RSP);
		
		return false;
	}
	
	int rc = -1;
	float money = 0.00;
	com_protocol::ItemInfo itemInfo;
	itemInfo.set_id(buyInfoIt->second.itemId);
	if (m_msgHandler->getMallMgr().getItemInfo(buyInfoIt->second.itemType, itemInfo))
	{
		money = itemInfo.buy_price() * buyInfoIt->second.itemCount;
	}
	
	// 校验成功则发放物品
	com_protocol::UserRechargeReq userRechargeReq;
	userRechargeReq.set_bills_id(ckpInfo->rechargeTransaction);
	userRechargeReq.set_username(cd->keyData.key);
	userRechargeReq.set_item_type(buyInfoIt->second.itemType);
	userRechargeReq.set_item_id(buyInfoIt->second.itemId);
	userRechargeReq.set_item_count(buyInfoIt->second.itemCount);
	userRechargeReq.set_item_flag(0); // m_msgHandler->getMallMgr().getItemFlag(cd->keyData.key, strlen(cd->keyData.key), buyInfoIt->second.itemId));
	userRechargeReq.set_recharge_rmb(money);
	userRechargeReq.set_third_account(ckpInfo->googleOrderId);
	userRechargeReq.set_third_type(ThirdPlatformType::GoogleSDK);
	userRechargeReq.set_third_other_data(paramKey2Value.at("consumptionState"));
	msgBuff = m_msgHandler->serializeMsgToBuffer(userRechargeReq, "send check google recharge result to db proxy");
	if (msgBuff != NULL)
	{
		rc = m_msgHandler->sendMessageToCommonDbProxy(msgBuff, userRechargeReq.ByteSize(), BUSINESS_USER_RECHARGE_REQ, cd->keyData.key, strlen(cd->keyData.key));
	}
	
	// 充值订单信息记录
    // format = name|type|id|count|amount|money|rechargeNo|reply|result|order|consumptionState|time
	GoogleRechargeLog("%s|%d|%d|%d|%d|%.2f|%s|reply|%d|%s|%s|%s", cd->keyData.key, buyInfoIt->second.itemType, buyInfoIt->second.itemId, buyInfoIt->second.itemCount, buyInfoIt->second.itemAmount,
	money, ckpInfo->rechargeTransaction, rsp.result(), ckpInfo->googleOrderId, paramKey2Value.at("consumptionState").c_str(), paramKey2Value.at("purchaseTimeMillis").c_str());
	
	OptInfoLog("check google SDK recharge transaction, rc = %d, user = %s, bill id = %s, order id = %s, result = %d",
	rc, cd->keyData.key, ckpInfo->rechargeTransaction, ckpInfo->googleOrderId, rsp.result());
	
	if (rc == OptSuccess) buyInfoIt->second.srcSrvId = cd->keyData.srvId;
	else m_recharge2BuyInfo.erase(buyInfoIt);
	
	return true;
}

bool CGoogleSDKOpt::googleRechargeItemReply(const com_protocol::UserRechargeRsp& userRechargeRsp)
{
	const com_protocol::UserRechargeReq& userRechargeInfo = userRechargeRsp.info();
	RechargeTranscationToBuyInfo::iterator it = m_recharge2BuyInfo.find(userRechargeInfo.bills_id());
	if (it == m_recharge2BuyInfo.end())
	{
		// 充值订单信息记录最终结果
		// format = name|type|id|count|amount|flag|money|rechargeNo|no_update_finish|umid|result|other data
		GoogleRechargeLog("%s|%u|%u|%u|%u|%u|%.2f|%s|finish|%s|%d|%s|%s", userRechargeInfo.username().c_str(), userRechargeInfo.item_type(), userRechargeInfo.item_id(), userRechargeInfo.item_count(), userRechargeRsp.item_amount(), userRechargeInfo.item_flag(), userRechargeInfo.recharge_rmb(), userRechargeInfo.bills_id().c_str(), userRechargeInfo.third_account().c_str(), userRechargeRsp.result(), userRechargeInfo.third_other_data().c_str());

		OptErrorLog("can not find the recharge info, user = %s, bill id = %s, order id = %s, money = %.2f, result = %d",
		m_msgHandler->getContext().userData, userRechargeInfo.bills_id().c_str(), userRechargeInfo.third_account().c_str(), userRechargeInfo.recharge_rmb(), userRechargeRsp.result());

		return false;
	}

	com_protocol::OverseasRechargeResultCheckRsp rsp;
	rsp.set_result(userRechargeRsp.result());
	rsp.set_item_type(it->second.itemType);
	rsp.set_item_id(it->second.itemId);
	rsp.set_item_count(it->second.itemCount);
	rsp.set_item_amount(userRechargeRsp.item_amount());
	
	char giftData[MaxNetBuffSize] = {0};
	m_msgHandler->getGiftInfo(userRechargeRsp, rsp.mutable_gift_array(), giftData, MaxNetBuffSize);

	int rc = -1;
	const char* msgBuff = m_msgHandler->serializeMsgToBuffer(rsp, "send check google recharge transaction result to client");
	if (msgBuff != NULL) rc = m_msgHandler->sendMessage(msgBuff, rsp.ByteSize(), it->second.srcSrvId, CHECK_GOOGLE_RECHARGE_RESULT_RSP);

    m_recharge2BuyInfo.erase(it);  // 充值操作处理完毕，删除该充值信息

    // 充值订单信息记录最终结果
    // format = name|type|id|count|amount|flag|money|rechargeNo|finish|umid|result|other data|giftdata
	GoogleRechargeLog("%s|%u|%u|%u|%u|%u|%.2f|%s|finish|%s|%d|%s|%s", userRechargeInfo.username().c_str(), userRechargeInfo.item_type(), userRechargeInfo.item_id(), userRechargeInfo.item_count(), userRechargeRsp.item_amount(), userRechargeInfo.item_flag(), userRechargeInfo.recharge_rmb(), userRechargeInfo.bills_id().c_str(), userRechargeInfo.third_account().c_str(), userRechargeRsp.result(), userRechargeInfo.third_other_data().c_str(), giftData);
	
	OptInfoLog("send google payment result to client, rc = %d, user name = %s, result = %d, bill id = %s, order id = %s, money = %.2f",
	rc, userRechargeInfo.username().c_str(), userRechargeRsp.result(), userRechargeInfo.bills_id().c_str(), userRechargeInfo.third_account().c_str(), userRechargeInfo.recharge_rmb());
	return true;
}

bool CGoogleSDKOpt::getAccessToken(unsigned int flag, unsigned int srvId, const char* keyData, const UserCbData* cbData, unsigned int len)
{
	if (cbData != NULL && len > sizeof(ConnectUserData))
	{
		OptErrorLog("the cb data len too large = %u, max size = %d", len, sizeof(ConnectUserData));
		return false;
	}
	
	// https://accounts.google.com/o/oauth2/token?grant_type=refresh_token&client_id={CLIENT_ID}&client_secret={CLIENT_SECRET}&refresh_token={REFRESH_TOKEN}
	ConnectData* cd = m_msgHandler->getConnectData();
	memset(cd, 0, sizeof(ConnectData));
	
	cd->timeOutSecs = HttpConnectTimeOut;
	if (keyData == NULL) keyData = "";
	strncpy(cd->keyData.key, keyData, MaxKeySize - 1);
	cd->keyData.flag = flag;
	cd->keyData.srvId = srvId;
	cd->keyData.protocolId = 0;
	
	char content[MaxNetBuffSize] = {0};
	unsigned int contentLen = snprintf(content, sizeof(content) - 1, "grant_type=refresh_token&client_id=%s&client_secret=%s&refresh_token=%s",
	CCfg::getValue(GoogleCfgItem, "ClientId"), CCfg::getValue(GoogleCfgItem, "ClientKey"), CCfg::getValue(GoogleCfgItem, "RefreshToken"));
	
	CHttpRequest httpRequest(HttpMsgType::POST);
	httpRequest.setHeaderKeyValue("Content-Type", "application/x-www-form-urlencoded");
	httpRequest.setContent(content, contentLen);
	cd->sendDataLen = MaxNetBuffSize;
	const char* msgData = httpRequest.getMessage(cd->sendData, cd->sendDataLen, CCfg::getValue(GoogleCfgItem, "TokenHost"), CCfg::getValue(GoogleCfgItem, "TokenUrl"));
    if (msgData == NULL)
	{
		m_msgHandler->putConnectData(cd);
		OptErrorLog("CGoogleSDKOpt::getAccessToken, get http request message error");
		return false;
	}

	bool result = m_msgHandler->doHttpConnect(m_tokenInfo.tokenIp, m_tokenInfo.tokenPort, cd);
	if (result)
	{
		if (cbData != NULL && len > 0)
		{
			unsigned int buffLen = 0;
			UserCbData* cbd = (UserCbData*)m_msgHandler->getDataBuffer(buffLen);  // 可以直接使用这里内存管理的内存块
			memcpy(cbd, cbData, len);
			cbd->flag = 0;  // 自动内存管理，框架层释放该内存块
			cd->cbData = cbd;
		}
	}
	else
	{
		m_msgHandler->putConnectData(cd);
		OptErrorLog("do http connect to get access token error, ip = %s, port = %d", m_tokenInfo.tokenIp, m_tokenInfo.tokenPort);
	}
	
	OptInfoLog("do http connect to get access token, result = %d, user = %s, flag = %u, message len = %u, content = %s",
	result, cd->keyData.key, flag, cd->sendDataLen, cd->sendData);
	
	return result;
}

bool CGoogleSDKOpt::getAccessTokenReply(ConnectData* cd)
{
	bool result = updateAccessTokenInfo(strchr(cd->receiveData, '{'));
    if (!result)
    {
        OptErrorLog("get access token error and stop service");
		m_msgHandler->stopService();
    }
	
	return result;
}

bool CGoogleSDKOpt::updateAccessTokenReply(ConnectData* cd)
{
	bool result = updateAccessTokenInfo(strchr(cd->receiveData, '{'));
    if (result)
	{
		const UpdateAccessTokenInfo* upTkInfo = (const UpdateAccessTokenInfo*)cd->cbData;
		checkRechargeResultExImpl(cd->keyData.key, cd->keyData.srvId, upTkInfo->rechargeTransaction, upTkInfo->googleOrderId, upTkInfo->packageName, upTkInfo->productId, upTkInfo->purchaseToken);
	}
	else
	{
		OptErrorLog("update access token error");
	}
	
	return result;
}

bool CGoogleSDKOpt::updateAccessTokenInfo(const char* info)
{
    /* 正确的应答消息内容格式
	{
	    "access_token" : "ya29.AHES3ZQ_MbZCwac9TBWIbjW5ilJkXvLTeSl530Na2",
	    "token_type" : "Bearer",
	    "expires_in" : 3600,
	}
	*/

	ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(info, paramKey2Value);
	
	ParamKey2Value::iterator accessTokenIt = paramKey2Value.find("access_token");
    ParamKey2Value::iterator expiresInIt = paramKey2Value.find("expires_in");
    if (accessTokenIt == paramKey2Value.end() || accessTokenIt->second.length() < 1
	    || expiresInIt == paramKey2Value.end() || expiresInIt->second.length() < 1) return false;

	const unsigned int advanceSecs = 60 * 10;  // 提取advanceSecs秒钟获取token
	unsigned int newLastSecs = time(NULL) + atoi(expiresInIt->second.c_str()) - advanceSecs;
    OptInfoLog("update access token success, old token = %s, last time = %u, new token = %s, last time = %u, expires in time = %d",
	m_tokenInfo.accessToken.c_str(), m_tokenInfo.lastSecs, accessTokenIt->second.c_str(), newLastSecs, atoi(expiresInIt->second.c_str()));

	m_tokenInfo.lastSecs = newLastSecs;
	m_tokenInfo.accessToken = accessTokenIt->second;
	
	return true;
}

}

