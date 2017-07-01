
/* author : limingfan
 * date : 2015.11.05
 * description : 第三方平台操作实现
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


#include "CThirdPlatformOpt.h"
#include "CHttpSrv.h"
#include "CHttpData.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"
#include "common/CommonType.h"
#include "channel/nduo/NduoOpt.h"
#include "channel/uuc/UucOpt.h"
#include "channel/huawei/HuaweiOpt.h"

#include <iostream>


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace NProject;


// 充值日志文件
#define ThirdRechargeLog(platformId, format, args...)     getThirdRechargeLogger(platformId).writeFile(NULL, 0, LogLevel::Info, format, ##args)

namespace NPlatformService
{

// 注册各个平台http消息处理函数
void CThirdPlatformOpt::registerHttpMsgHandler()
{
	// 第三方平台配置项
	const char* thirdName[] = {"AoRong", "DownJoy", "Google", "YouWan", "WanDouJia", "XiaoMi", "JinNiu", "QiHu360", "UCSDK", "OPPO", "MuMaYi", "SoGou", "AnZhi", "MeiZu", "TXQQ", "TXWX",
							   "M4399", "WoGame", "YdMM", "LoveGame", "Nduo", "Uucun", "Huawei", "Zhuoyi", "ZhuoyiTest", "TT", "Baidu", "YunZhongKu", "Vivo", "MiGu", "MoGuWan", "YiJie",
							   "SanXing", "HaiMaWan", "AppleIOS", "HanYou", "ManTuo", "HanYouFacebook"};
	for (unsigned int idx = 0; idx < (sizeof(thirdName) / sizeof(thirdName[0])) && idx < ThirdPlatformType::MaxPlatformType; ++idx) m_thirdName[idx] = thirdName[idx];
	
	
	/*
	{
		// 注册校验第三方用户请求处理函数
		m_msgHandler->registerCheckUserRequest(ThirdPlatformType::QiHu360, (CheckUserRequest)&CThirdPlatformOpt::checkQiHu360User, this);
		m_msgHandler->registerCheckUserRequest(ThirdPlatformType::Oppo, (CheckUserRequest)&CThirdPlatformOpt::checkOppoUser, this);
		m_msgHandler->registerCheckUserRequest(ThirdPlatformType::MuMaYi, (CheckUserRequest)&CThirdPlatformOpt::checkMuMaYiUser, this);
		m_msgHandler->registerCheckUserRequest(ThirdPlatformType::SoGou, (CheckUserRequest)&CThirdPlatformOpt::checkSoGouUser, this);
		m_msgHandler->registerCheckUserRequest(ThirdPlatformType::AnZhi, (CheckUserRequest)&CThirdPlatformOpt::checkAnZhiUser, this);
		m_msgHandler->registerCheckUserRequest(ThirdPlatformType::TXQQ, (CheckUserRequest)&CThirdPlatformOpt::checkTXQQUser, this);
		m_msgHandler->registerCheckUserRequest(ThirdPlatformType::TXWX, (CheckUserRequest)&CThirdPlatformOpt::checkTXWXUser, this);
        
		// 注册校验第三方用户应答处理函数
		m_msgHandler->registerCheckUserHandler(ThirdPlatformType::QiHu360, (CheckUserHandler)&CThirdPlatformOpt::checkQiHu360UserReply, this);
		m_msgHandler->registerCheckUserHandler(ThirdPlatformType::Oppo, (CheckUserHandler)&CThirdPlatformOpt::checkOppoUserReply, this);
		m_msgHandler->registerCheckUserHandler(ThirdPlatformType::MuMaYi, (CheckUserHandler)&CThirdPlatformOpt::checkMuMaYiUserReply, this);
       	m_msgHandler->registerCheckUserHandler(ThirdPlatformType::SoGou, (CheckUserHandler)&CThirdPlatformOpt::checkSoGouUserReply, this);
		m_msgHandler->registerCheckUserHandler(ThirdPlatformType::AnZhi, (CheckUserHandler)&CThirdPlatformOpt::checkAnZhiUserReply, this);
		m_msgHandler->registerCheckUserHandler(ThirdPlatformType::TXQQ, (CheckUserHandler)&CThirdPlatformOpt::checkTXQQUserReply, this);
		m_msgHandler->registerCheckUserHandler(ThirdPlatformType::TXWX, (CheckUserHandler)&CThirdPlatformOpt::checkTXWXUserReply, this);
	}
	*/
	
	// 注册校验第三方用户请求处理函数&应答处理函数
	m_msgHandler->registerCheckUserRequest(ThirdPlatformType::WanDouJia, (CheckUserRequest)&CThirdPlatformOpt::checkWanDouJiaUser, this);
	m_msgHandler->registerCheckUserHandler(ThirdPlatformType::WanDouJia, (CheckUserHandler)&CThirdPlatformOpt::checkWanDouJiaUserReply, this);
	m_msgHandler->registerCheckUserHandler(ThirdPlatformType::XiaoMi, (CheckUserHandler)&CThirdPlatformOpt::checkXiaoMiUserReply, this);
	

	// 注册用户付费GET协议充值处理函数
	m_msgHandler->registerUserPayGetHandler(ThirdPlatformType::XiaoMi, (UserPayGetOptHandler)&CThirdPlatformOpt::handleXiaoMiPaymentMsg, this);
	m_msgHandler->registerUserPayGetHandler(ThirdPlatformType::QiHu360, (UserPayGetOptHandler)&CThirdPlatformOpt::handleQiHu360PaymentMsg, this);


	// 注册用户付费POST协议充值处理函数
	// m_msgHandler->registerUserPayPostHandler(ThirdPlatformType::YouWan, (UserPayPostOptHandler)&CThirdPlatformOpt::handleYouWanPaymentMsg, this);
	// m_msgHandler->registerUserPayPostHandler(ThirdPlatformType::JinNiu, (UserPayPostOptHandler)&CThirdPlatformOpt::handleJinNiuPaymentMsg, this);
	// m_msgHandler->registerUserPayPostHandler(ThirdPlatformType::TXQQ, (UserPayPostOptHandler)&CThirdPlatformOpt::handleTXPaymentMsg, this);
	m_msgHandler->registerUserPayPostHandler(ThirdPlatformType::WanDouJia, (UserPayPostOptHandler)&CThirdPlatformOpt::handleWanDouJiaPaymentMsg, this);
	m_msgHandler->registerUserPayPostHandler(ThirdPlatformType::UCSDK, (UserPayPostOptHandler)&CThirdPlatformOpt::handleUCPaymentMsg, this);
	m_msgHandler->registerUserPayPostHandler(ThirdPlatformType::Oppo, (UserPayPostOptHandler)&CThirdPlatformOpt::handleOppoPaymentMsg, this);
	m_msgHandler->registerUserPayPostHandler(ThirdPlatformType::MuMaYi, (UserPayPostOptHandler)&CThirdPlatformOpt::handleMuMaYiPaymentMsg, this);
	m_msgHandler->registerUserPayPostHandler(ThirdPlatformType::AnZhi, (UserPayPostOptHandler)&CThirdPlatformOpt::handleAnZhiPaymentMsg, this);

    // 处理 更新腾讯游戏币
	m_msgHandler->registerHttpReplyHandler(HttpReplyHandlerID::IdUpdateTXServerUserCurrencyData, (HttpReplyHandler)&CThirdPlatformOpt::handleUpdateTXServerUserCurrencyData, this);
	
	// 扣除多余的TX托管的渔币数量
    m_msgHandler->registerHttpReplyHandler(HttpReplyHandlerID::IddeductTXServerCurrency, (HttpReplyHandler)&CThirdPlatformOpt::handleDeductTXServerCurrencyMsg, this);
                                            
    // 处理腾讯充值是否成功 
    m_msgHandler->registerHttpReplyHandler(HttpReplyHandlerID::IdFindRechargeSuccess, (HttpReplyHandler)&CThirdPlatformOpt::handleFindRechargeSuccess, this);	
}


CThirdPlatformOpt::CThirdPlatformOpt()
{
	m_msgHandler = NULL;
	memset(m_thirdPlatformCfg, 0, sizeof(m_thirdPlatformCfg));
	memset(m_transactionIndex, 0, sizeof(m_transactionIndex));

	memset(m_thirdName, 0, sizeof(m_thirdName));
	memset(m_thirdLogger, 0, sizeof(m_thirdLogger));
}

CThirdPlatformOpt::~CThirdPlatformOpt()
{
	m_msgHandler = NULL;
	memset(m_thirdPlatformCfg, 0, sizeof(m_thirdPlatformCfg));
	memset(m_transactionIndex, 0, sizeof(m_transactionIndex));

	memset(m_thirdName, 0, sizeof(m_thirdName));
	
	for (unsigned int idx = ThirdPlatformType::DownJoy; idx < ThirdPlatformType::MaxPlatformType; ++idx) DELETE(m_thirdLogger[idx]);
	memset(m_thirdLogger, 0, sizeof(m_thirdLogger));
}

CLogger& CThirdPlatformOpt::getThirdRechargeLogger(const unsigned int platformId)
{
	if (platformId < ThirdPlatformType::MaxPlatformType)
	{
	    CLogger* thirdLogger = m_thirdLogger[platformId];
		if (thirdLogger != NULL) return *thirdLogger;

		const char* name = m_thirdName[platformId];
		if (name != NULL)
		{
		    NEW(thirdLogger, CLogger(CCfg::getValue(name, "RechargeLogFileName"), atoi(CCfg::getValue(name, "RechargeLogFileSize")), atoi(CCfg::getValue(name, "RechargeLogFileCount"))));
			if (thirdLogger != NULL)
			{
				m_thirdLogger[platformId] = thirdLogger;
				return *thirdLogger;
			}
		}
	}
	
	return CLogger::getOptLogger();
}

CThirdInterface& CThirdPlatformOpt::getThirdInstance()
{
	return m_thirdInterface;
}
 
bool CThirdPlatformOpt::getTranscationIdToInfo(const string &TranscationId, ThirdTranscationIdToInfo::iterator &it)
{
    it = m_thirdTransaction2Info.find(TranscationId);
    return it != m_thirdTransaction2Info.end();
}

bool CThirdPlatformOpt::load(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;
	
	registerHttpMsgHandler();
	
	// 配置项值检查
	const char* thirdLoggerCfg[] = {"RechargeLogFileName", "RechargeLogFileSize", "RechargeLogFileCount",};
	const char* thirdAppCfg[] = {"Host", "Url", "AppId", "AppKey", "PaymentKey", "PaymentUrl",};
	for (unsigned int i = 0; i < (sizeof(m_thirdName) / sizeof(m_thirdName[0])); ++i)
	{
		if (m_thirdName[i] == NULL) continue;

		// 日志配置检查
		for (unsigned int j = 0; j < (sizeof(thirdLoggerCfg) / sizeof(thirdLoggerCfg[0])); ++j)
		{
			const char* value = CCfg::getValue(m_thirdName[i], thirdLoggerCfg[j]);
			if (value == NULL)
			{
				ReleaseErrorLog("third platform %s logger config item error, can not find the item = %s", m_thirdName[i], thirdLoggerCfg[j]);
				m_msgHandler->stopService();
				return false;
			}
			
			if ((j == 1 && (unsigned int)atoi(value) < MinRechargeLogFileSize)
				|| (j == 2 && (unsigned int)atoi(value) < MinRechargeLogFileCount))
			{
				ReleaseErrorLog("third platform %s config item error, recharge log file min size = %d, count = %d", m_thirdName[i], MinRechargeLogFileSize, MinRechargeLogFileCount);
				m_msgHandler->stopService();
				return false;
			}
		}
		
		// 应用配置检查
		ThirdPlatformConfig& thirdPlatformCfg = m_thirdPlatformCfg[i];
		const char** thirdAppCfgValue[] = {&(thirdPlatformCfg.host), &(thirdPlatformCfg.url), &(thirdPlatformCfg.appId), &(thirdPlatformCfg.appKey), &(thirdPlatformCfg.paymentKey), &(thirdPlatformCfg.paymentUrl),};
		for (unsigned int j = 0; j < (sizeof(thirdAppCfg) / sizeof(thirdAppCfg[0])); ++j)
		{
			const char* value = CCfg::getValue(m_thirdName[i], thirdAppCfg[j]);
			if (value == NULL)
			{
				ReleaseErrorLog("third platform %s app config item error, can not find the item = %s", m_thirdName[i], thirdAppCfg[j]);
				m_msgHandler->stopService();
				return false;
			}
			
			*thirdAppCfgValue[j] = value;
		}
		
		// 登陆连接IP检查
		const char* runValue = CCfg::getValue(m_thirdName[i], "CheckHost");  // 检查是否使用启动登陆校验处理流程
		const char* protocalValue = CCfg::getValue(m_thirdName[i], "Protocol");
		if (protocalValue == NULL) protocalValue = HttpService;
		if (runValue != NULL && atoi(runValue) == 1 && !m_msgHandler->getHostInfo(thirdPlatformCfg.host, thirdPlatformCfg.ip, thirdPlatformCfg.port, protocalValue))
		{
			ReleaseErrorLog("get %s host info error, host = %s", m_thirdName[i], thirdPlatformCfg.host);
			m_msgHandler->stopService();
			return false;
		}
	}

    /*
	if (!parseUnHandleTransactionData())
	{
		ReleaseErrorLog("parse unhandle transaction error");
		m_msgHandler->stopService();
		return false;
	}
	
	m_msgHandler->registerProtocol(CommonSrv, GET_THIRD_RECHARGE_TRANSACTION_REQ, (ProtocolHandler)&CThirdPlatformOpt::getThirdRechargeTransaction, this);
    m_msgHandler->registerProtocol(CommonSrv, CANCEL_THIRD_RECHARGE_NOTIFY, (ProtocolHandler)&CThirdPlatformOpt::cancelThirdRechargeNotify, this);
	*/
	
	m_msgHandler->registerProtocol(CommonSrv, FIN_RECHARGE_SUCCESS_REQ, (ProtocolHandler)&CThirdPlatformOpt::findRechargeSuccessReq, this);
	
	m_msgHandler->registerProtocol(LoginSrv, CHECK_THIRD_USER_REQ, (ProtocolHandler)&CThirdPlatformOpt::checkThirdUser, this);
	m_msgHandler->registerProtocol(LoginSrv, CHECK_XIAOMI_USER_REQ, (ProtocolHandler)&CThirdPlatformOpt::checkXiaoMiUser, this);
	
	return m_thirdInterface.load(m_msgHandler, this) 
		   && NduoOpt::getInstance()->load(m_msgHandler, this) 
		   && UucOpt::getInstance()->load(m_msgHandler, this) 
		   && HuaweiOpt::getInstance()->load(m_msgHandler,this) ;
}

void CThirdPlatformOpt::unload()
{
	m_thirdInterface.unload();
	NduoOpt::getInstance()->unload();
	UucOpt::getInstance()->unload();
	HuaweiOpt::getInstance()->unload();
}


const ThirdPlatformConfig* CThirdPlatformOpt::getThirdPlatformCfg(unsigned int type)
{
	return (type < MaxPlatformType) ? &(m_thirdPlatformCfg[type]) : NULL;
}

bool CThirdPlatformOpt::parseUnHandleTransactionData()
{
	/*
	// 获取之前没有处理完毕的订单数据，一般该数据为空
	// 当且仅当用户充值流程没走完并且此种情况下服务异常退出时才会存在未处理完毕的订单数据
	vector<string> unHandleTransactionData;
	int ret = m_msgHandler->getRedisService().getHAll(RechargeDataKey, RechargeDataKeyLen, unHandleTransactionData);
	if (0 != ret)
	{
		ReleaseErrorLog("get no handle transaction data error, rc = %d", ret);
		return false;
	}

	for (unsigned int i = 1; i < unHandleTransactionData.size(); i += 2)
	{
		const char* transactionId = unHandleTransactionData[i - 1].c_str();  // 订单ID值
		const char* transactionData = unHandleTransactionData[i].c_str();    // 订单数据内容
		m_thirdTransaction2Info[transactionId] = ThirdTranscationInfo();
	    ThirdTranscationInfo& info = m_thirdTransaction2Info[transactionId];
		if (!parseTransactionData(transactionId, transactionData, info))
		{
			m_thirdTransaction2Info.erase(transactionId);
			
			OptErrorLog("have unhandle transaction data error, id = %s, data = %s", transactionId, transactionData);
		}
	}
	*/
	
	return true;
}

bool CThirdPlatformOpt::updateTXServerUserCurrencyData(ThirdPlatformType platformID, const char* openid, const char *openkey, const char *pf, 
                                                       const char *pfkey, unsigned int srcSrvId, const char *orderID)
{
	// 回传数据
	TXRetInfo txRetData;
	strncpy(txRetData.openid, openid, 128 - 1);
	strncpy(txRetData.pf, pf, 128 - 1);
	strncpy(txRetData.pfkey, pfkey, 128 - 1);
	strncpy(txRetData.billno, orderID, 64 - 1);
	strncpy(txRetData.openkey, openkey, 256 - 1);
	strncpy(txRetData.userData, m_msgHandler->getContext().userData, 36 - 1);
	txRetData.platformID = platformID;
	txRetData.flag = 0;  // 底层自动管理内存
	txRetData.srcSrvId = srcSrvId;
	txRetData.timerId = -1;
	txRetData.remainCount = 0;
	txRetData.nPayState = 0;

	//查询腾讯服务器上数据
	return findTXServerUserCurrencyData(srcSrvId, IdUpdateTXServerUserCurrencyData, &txRetData);
}

void CThirdPlatformOpt::findRechargeSuccessReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    com_protocol::FindRechargeSuccessReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "find recharge success req error!!")) return;
	
	FishCoinOrderIdToInfo::iterator orderIt;
	if (!m_thirdInterface.getTranscationData(req.recharge_transaction().c_str(), orderIt))
	{
		OptErrorLog("findRechargeSuccessReq, can not find the order = %s", req.recharge_transaction().c_str());
        return;
	}
    
    unsigned int buffLen = 0;
    TXRetInfo *txRetData = (TXRetInfo*)m_msgHandler->getDataBuffer(buffLen);
    if(buffLen < sizeof(TXRetInfo))
    {
        OptErrorLog("findRechargeSuccessReq, get data buffer error, buffLen=%d, txRetDataLen=%d, order = %s", buffLen, sizeof(TXRetInfo), req.recharge_transaction().c_str());
        return;
    }      
   
    for (int i = 0; i < req.key_value_size(); ++i)
    {
        const com_protocol::ThirdOrderKeyValue &kv = req.key_value(i);
        if(kv.key() == "openid")    strncpy(txRetData->openid, kv.value().c_str(), 128 - 1);
        if(kv.key() == "openkey")   strncpy(txRetData->openkey, kv.value().c_str(), 256 - 1);
        if(kv.key() == "pf")        strncpy(txRetData->pf, kv.value().c_str(), 128 - 1);
        if(kv.key() == "pfkey")     strncpy(txRetData->pfkey, kv.value().c_str(), 128 - 1);
        if(kv.key() == "payState")  txRetData->nPayState = atoi(kv.value().c_str());
    }
    
    strncpy(txRetData->billno, req.recharge_transaction().c_str(), 64 - 1);
    txRetData->platformID = (int)req.platform_id();
    strncpy(txRetData->userData, orderIt->second.user, 36 - 1);
    txRetData->flag = 1;                    //由外部管理内存释放
    txRetData->timerId = -1;                //定时器ID
    txRetData->remainCount = 0;             //剩余查询次数
    txRetData->srcSrvId = srcSrvId;
   
    //查询充值是否成功
    findTXServerUserCurrencyData(srcSrvId, IdFindRechargeSuccess, txRetData);
    
    OptInfoLog("find TX recharge result, user = %s, order id = %s, pay state = %d", txRetData->userData, txRetData->billno, txRetData->nPayState);
}

void CThirdPlatformOpt::onFindRechargeSuccess(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
    OptInfoLog("onFindRechargeSuccess, remainCount:%d", remainCount);
    
    //查询是否充值成功
    TXRetInfo* txRetData = (TXRetInfo*)param;
    txRetData->remainCount = remainCount;
    findTXServerUserCurrencyData(txRetData->srcSrvId, IdFindRechargeSuccess, txRetData);
}

void CThirdPlatformOpt::onDeductTXServerCurrency(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	const TXRetInfo* txRet = (TXRetInfo*)param;
	bool result = deductTXServerCurrency(HttpReplyHandlerID::IddeductTXServerCurrency, txRet->nPayState, txRet);
	
	OptInfoLog("onDeductTXServerCurrency, user = %s, order = %s, tx balance = %d, result = %d", txRet->userData, txRet->billno, txRet->nPayState, result);
    
    if (!result) m_msgHandler->putDataBuffer((char*)txRet);  // 释放内存
}

bool CThirdPlatformOpt::findTXServerUserCurrencyData(unsigned int srcSrvId, HttpReplyHandlerID replyHandlerID, TXRetInfo *pRetData)
{    
    if(pRetData == NULL)
    {
        OptErrorLog("findTXServerUserCurrencyData, pRetData == NULL");
        return false;
    }
    
    if(!(pRetData->platformID == ThirdPlatformType::TXQQ || pRetData->platformID == ThirdPlatformType::TXWX))
    {
        OptErrorLog("findTXServerUserCurrencyData, platform ID not TXQQ or TXWX!, platform id = %d", pRetData->platformID);
        return false;
    }
    
    //第三方平台
    //微信登录态和手Q登录态使用的支付接口相同，支付ID相同； 服务端使用的appid和appkey都使用手Qappid和appkey
    const ThirdPlatformConfig* pThird = getThirdPlatformCfg(ThirdPlatformType::TXQQ);
    if(pThird == NULL)
    {
        OptErrorLog("findTXServerUserCurrencyData platform id error, platformID:%d", pRetData->platformID);
        return false;
    }
    
    //请求参数
    map<string, string> parameter;
    parameter.insert(pair<string, string>("openid", pRetData->openid));
    parameter.insert(pair<string, string>("openkey", pRetData->openkey));
    parameter.insert(pair<string, string>("appid", pThird->appId));
    parameter.insert(pair<string, string>("pf", pRetData->pf));
    parameter.insert(pair<string, string>("pfkey", pRetData->pfkey));
    parameter.insert(pair<string, string>("zoneid", "1"));

	OptInfoLog("find TX Server User Currency Data, openid:%s, openkey:%s, appId:%s, pf:%s, pfkey:%s, zoneid:1", pRetData->openid, pRetData->openkey, pThird->appId, pRetData->pf, pRetData->pfkey);
       
    char sTime[MaxNetBuffSize] = {0};
    snprintf(sTime, MaxNetBuffSize - 1, "%u", (unsigned int)time(NULL));
    parameter.insert(pair<string, string>("ts", sTime));
    
    //生成签名
    const char *url = CCfg::getValue("TXQQ", "UrlFind");        //这里QQ 和微信没有区别
    string sig = produceTXSig(url, parameter, pThird->appKey);
    
    //签名进行url编码
    char urlSig[MaxNetBuffSize] = {0};
    unsigned int urlSigLen = MaxNetBuffSize - 1;
    if(!urlEncode(sig.c_str(), sig.size(), urlSig, urlSigLen))
    {
        OptErrorLog("findTXServerUserCurrencyData, sig url encode error, sig:%s", sig.c_str());
        return false;
    }    
    sig = "sig=";
    sig += urlSig;
    
    //请求参数
    string reqParameter;
    for(auto it = parameter.begin(); it != parameter.end(); ++it)
    {
        reqParameter += (it->first + "=" + it->second + "&");
    }
    
    reqParameter += sig;
    OptInfoLog("findTXServerUserCurrencyData reqParameter:%s", reqParameter.c_str());    
        
        
    //Cookie数据
    char cookieData[MaxNetBuffSize] = {0};
    const char* session_id = (pRetData->platformID == ThirdPlatformType::TXQQ ? "openid" : "hy_gameid");
    const char* session_type = (pRetData->platformID == ThirdPlatformType::TXQQ ? "kp_actoken" : "wc_actoken");
    
    //org_loc urlencode
    char org_loc[MaxNetBuffSize] = {0};
    unsigned int orglocLen = MaxNetBuffSize - 1;
    if(!urlEncode("/mpay/get_balance_m", strlen("/mpay/get_balance_m"), org_loc, orglocLen))
    {
         OptErrorLog("findTXServerUserCurrencyData, url encode error!!");
         return false;
    }
    snprintf(cookieData, MaxNetBuffSize - 1, "session_id=%s;session_type=%s;org_loc=%s", session_id, session_type, org_loc);
    
    //向腾讯服务器查询当前用户当前的数据
    CHttpRequest httpRequest(HttpMsgType::GET);
	httpRequest.setParam(reqParameter);
	httpRequest.setHeaderKeyValue("Cookie", cookieData);
   
	if (!m_msgHandler->doHttpConnect(pRetData->platformID, srcSrvId, httpRequest, replyHandlerID, url, (UserCbData*)pRetData, 
                                     sizeof(TXRetInfo)))
	{
		OptErrorLog("findTXServerUserCurrencyData do http connect error");
		return false;
	}
	
	return true;
}

bool CThirdPlatformOpt::deductTXServerCurrency(const HttpReplyHandlerID replyHandlerID, const unsigned int deductValue, const TXRetInfo* pRetData)
{
    if(pRetData == NULL)
    {
		OptErrorLog("deductTXServerCurrency, pRetData == NULL!");
        return false;
    }
    
    if(!(pRetData->platformID == ThirdPlatformType::TXQQ || pRetData->platformID == ThirdPlatformType::TXWX))
    {
        OptErrorLog("deductTXServerCurrency, platform ID not TXQQ or TXWX, platformID:%d", pRetData->platformID);
        return false;
    }
    
    //第三方平台
    const ThirdPlatformConfig* pThird = getThirdPlatformCfg(ThirdPlatformType::TXQQ);
    if(pThird == NULL)
    {
        OptErrorLog("deductTXServerCurrency platform id error, platformID:%d", pRetData->platformID);
        return false;
    }
   
    //请求参数
	strInt_t strDeductValue = {0};
	snprintf(strDeductValue, StrIntLen - 1, "%u", deductValue);
    map<string, string> parameter;
    parameter.insert(pair<string, string>("openid", pRetData->openid));
    parameter.insert(pair<string, string>("openkey", pRetData->openkey));
    parameter.insert(pair<string, string>("appid", pThird->appId));
    parameter.insert(pair<string, string>("pf", pRetData->pf));
    parameter.insert(pair<string, string>("pfkey", pRetData->pfkey));
    parameter.insert(pair<string, string>("zoneid", "1"));
    parameter.insert(pair<string, string>("amt", strDeductValue));
    parameter.insert(pair<string, string>("billno", pRetData->billno));
    
    char sTime[MaxNetBuffSize] = {0};
    snprintf(sTime, MaxNetBuffSize - 1, "%u", (unsigned int)time(NULL));
    parameter.insert(pair<string, string>("ts", sTime));
     
    //签名
    auto url = CCfg::getValue("TXQQ", "UrlDeduct");
    string sig = produceTXSig(url, parameter, pThird->appKey);
    
     //签名进行url编码
    char urlSig[MaxNetBuffSize] = {0};
    unsigned int urlSigLen = MaxNetBuffSize - 1;
    if(!urlEncode(sig.c_str(), sig.size(), urlSig, urlSigLen))
    {
        OptErrorLog("deductTXServerCurrency, sig url encode error, sig:%s", sig.c_str());
        return false;
    }    
           
     //请求参数
    string reqParameter;
    for(auto it = parameter.begin(); it != parameter.end(); ++it)
    {
        reqParameter += (it->first + "=" + it->second + "&");
    }
    reqParameter += "sig=";
    reqParameter += urlSig;
    
    //OptInfoLog("deductTXServerCurrency reqParameter:%s", reqParameter.c_str());    
    
    //Cookie数据
    //org_loc urlencode
    char org_loc[MaxNetBuffSize] = {0};
    unsigned int orglocLen = MaxNetBuffSize - 1;
    if(!urlEncode("/mpay/pay_m", strlen("/mpay/pay_m"), org_loc, orglocLen))
    {
         OptErrorLog("deductTXServerCurrency, url encode error!!");
         return false;
    }
    
    char cookieData[MaxNetBuffSize] = {0};
    const char* session_id = (pRetData->platformID == ThirdPlatformType::TXQQ) ? "openid" : "hy_gameid";
    const char* session_type = (pRetData->platformID == ThirdPlatformType::TXQQ) ? "kp_actoken" : "wc_actoken";
    snprintf(cookieData, MaxNetBuffSize - 1, "session_id=%s;session_type=%s;org_loc=%s", session_id, session_type, org_loc);
    
    //扣除游戏币接口
    CHttpRequest httpRequest(HttpMsgType::GET);
    httpRequest.setParam(reqParameter);
	httpRequest.setHeaderKeyValue("Cookie", cookieData);
     
	if (!m_msgHandler->doHttpConnect(pRetData->platformID, pRetData->srcSrvId, httpRequest, replyHandlerID, url, (const UserCbData*)pRetData, sizeof(TXRetInfo)))
	{
		OptErrorLog("findTXServerUserCurrencyData do http connect error");
		return false;
	}
	
	return true;
}

string CThirdPlatformOpt::produceTXSig(const char *pUri, const map<string, string> &parameter, const char *pAppKey)
{
   //第1步：将请求的URI路径进行URL编码（URI不含host，URI示例：/v3/user/get_info）。
   char urlEncodeBuff[MaxNetBuffSize] = {0};
   unsigned int urlEncodeBuffLen = MaxNetBuffSize - 1; 
   if(!urlEncode(pUri, strlen(pUri), urlEncodeBuff, urlEncodeBuffLen))
   {
       OptErrorLog("produceTXSig, uri url Encode error, Uri:%s", pUri);
       return "";
   }
   
   //第2步：将除“sig”外的所有参数按key进行字典升序排列。
   //排序后的参数(key=value)用&拼接起来，并进行URL编码。
   //注：除非OpenAPI文档中特别标注了某参数不参与签名，否则除sig外的所有参数都要参与签名。
   string temp;
   for(auto it = parameter.begin(); it != parameter.end(); )
   {
      temp += (it->first + "=" + it->second);
      //用 &进行分隔
      if(++it != parameter.end())
          temp += "&";
   }
   
   //url编码
   char urlParameter[MaxNetBuffSize] = {0};
   unsigned int urlParameterLen = MaxNetBuffSize - 1;
   if(!urlEncode(temp.c_str(), temp.size(), urlParameter, urlParameterLen))
   {
       OptErrorLog("produceTXSig, Parameter url Encode error, Parameter:%s", temp.c_str());
       return "";
   }
   
   //第4步：将HTTP请求方式（GET或者POST）以及第1步和第3步中的字符串用&拼接起来。
   string source("GET&");
   source += urlEncodeBuff;
   source += "&";
   source += urlParameter;
   
   //OptInfoLog("produceTXSig, str:%s", source.c_str());
   
    //Step 2. 构造密钥
    //得到密钥的方式：在应用的appkey末尾加上一个字节的“&”，即appkey&，例如：
    string appkey(pAppKey);
    appkey += "&";
    
    //OptInfoLog("produceTXSig, key:%s", appkey.c_str());
    
    //使用HMAC-SHA1加密算法，使用Step2中得到的密钥对Step1中得到的源串加密。
    unsigned char hmacSha1Buff[MaxNetBuffSize] = {0};
    unsigned int hmacSha1BuffLen = MaxNetBuffSize - 1;
    if(HMAC_SHA1(appkey.c_str(), appkey.size(), (const unsigned char*)source.c_str(), source.size(), hmacSha1Buff, hmacSha1BuffLen) == NULL)
    {
        OptErrorLog("produceTXSig HMAC_SHA1 error!!, key:%s, data:%s", appkey.c_str(), source.c_str());
        return "";
    }
    
    //OptInfoLog("produceTXSig, hmac_sha1 sig:%s", hmacSha1Buff);
    
    //然后将加密后的字符串经过Base64编码。 
    char base64Buff[MaxNetBuffSize] = {0};
    unsigned int base64BuffLen = MaxNetBuffSize - 1;
    if(!base64Encode((char*)hmacSha1Buff, hmacSha1BuffLen, base64Buff, base64BuffLen))
    {
        OptErrorLog("produceTXSig, base64 Encode error!!");
        return "";
    }
    //OptInfoLog("produceTXSig, base64 sig:%s", base64Buff);
    
    return base64Buff;
}

void CThirdPlatformOpt::answerThirdRechargeTransaction(int nCode, const TXRetInfo* txRet, const int txBalance)
{
    com_protocol::GetRechargeFishCoinOrderRsp thirdRsp;
    
    //获取内部订单信息
    FishCoinOrderIdToInfo::iterator it;
    if(!m_thirdInterface.getTranscationData(txRet->billno, it))
    {
        nCode = SrvRetCode::FindOrderError;
        OptErrorLog("answerThirdRechargeTransaction, find order error, order id:%s", txRet->billno);
    }
    else if (nCode == OptSuccess)
	{
		thirdRsp.set_order_id(txRet->billno);
		thirdRsp.set_fish_coin_id(it->second.itemId);
		thirdRsp.set_fish_coin_amount(it->second.itemAmount);
		thirdRsp.set_money(it->second.money);
    }
    
    thirdRsp.set_result(nCode);
    
    int rc = -1;
    const char* msgData = m_msgHandler->serializeMsgToBuffer(thirdRsp, "send tx fish coin order to client");
    if (msgData != NULL)
	{
        rc = m_msgHandler->sendMessage(msgData, thirdRsp.ByteSize(), txRet->userData, strlen(txRet->userData), txRet->srcSrvId, GET_FISH_COIN_RECHARGE_ORDER_RSP);
		if (rc == OptSuccess && thirdRsp.result() == OptSuccess)
		{
			// 充值订单信息记录
			// format = name|id|amount|money|rechargeNo|request|platformType|srcSrvId|txBalance
			ThirdRechargeLog(txRet->platformID, "%s|%u|%u|%.2f|%s|request|%u|%u|%u", txRet->userData, thirdRsp.fish_coin_id(), thirdRsp.fish_coin_amount(),
			thirdRsp.money(), thirdRsp.order_id().c_str(), txRet->platformID, txRet->srcSrvId, txBalance);
			
			// 这里保存腾讯服务器上的游戏币数量，充值成功后查询比较是否新的总额大于该当前的总额
			it->second.userCbFlag = txBalance;
		}
    }
	
	OptInfoLog("answerThirdRechargeTransaction, srcSrvId = %u, user = %s, tx balance = %d, fish coin id = %u, money = %.2f, order id = %s, platform id = %u, result = %d, rc = %d",
		       txRet->srcSrvId, txRet->userData, txBalance, thirdRsp.fish_coin_id(), thirdRsp.money(), thirdRsp.order_id().c_str(), txRet->platformID, thirdRsp.result(), rc);
			   
    // 失败则删除订单数据
	if (thirdRsp.result() != OptSuccess || rc != OptSuccess) m_thirdInterface.removeTransactionData(it);
}

bool CThirdPlatformOpt::handleUpdateTXServerUserCurrencyData(ConnectData* cd)
{
    ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
    
    //结果
	const TXRetInfo* txRet = (const TXRetInfo*)cd->cbData;
    ParamKey2Value::const_iterator ret = paramKey2Value.find("ret");
    if (ret == paramKey2Value.end())
	{
        answerThirdRechargeTransaction(SrvRetCode::UpdateTxServerDataError, txRet);
		OptErrorLog("handleUpdateTXServerUserCurrencyData, can not find the check data ret");
		return false;
	}
    
    //更新失败 则通知客户端请求订单失败
    if(ret->second != "0")
    {
        //返回客户端消息
        string errorMsg;
        ParamKey2Value::const_iterator msg = paramKey2Value.find("msg");
        if(msg != paramKey2Value.end())
            errorMsg = msg->second;
         
        answerThirdRechargeTransaction(SrvRetCode::UpdateTxServerDataError, txRet);
        OptErrorLog("handleUpdateTXServerUserCurrencyData, update tx server data error: %s, msg:%s, order = %s", ret->second.c_str(), errorMsg.c_str(), txRet->billno);
        return false;
    }
    
    //游戏币
    ParamKey2Value::const_iterator balance = paramKey2Value.find("balance");
	if (balance == paramKey2Value.end())
	{
        answerThirdRechargeTransaction(SrvRetCode::UpdateTxServerDataError, txRet);
		OptErrorLog("handleUpdateTXServerUserCurrencyData, can not find the check data balance");
		return false;
	}

	answerThirdRechargeTransaction(SrvRetCode::OptSuccess, txRet, atoi(balance->second.c_str()));  // 返回客户端消息
	return true;
}

bool CThirdPlatformOpt::handleFindRechargeSuccess(ConnectData* cd)
{
    TXRetInfo *txRet = (TXRetInfo*)cd->cbData;        // 用户挂接的数据
	int nTxBalance = 0;
    switch(txRecharge(cd, nTxBalance))
    {
        case ETXRechargeStatus::EFinish :     //操作完成
        {
            if(txRet->timerId != -1)
            {
                m_msgHandler->killTimer(txRet->timerId);      //取消定时器
            }
            
            //释放内存
            m_msgHandler->putDataBuffer((char*)cd->cbData);
            OptInfoLog("handleFindRechargeSuccess OK!");
			break;
        }
  
        case ETXRechargeStatus::EContinueQuery:     //操作未完成
        {    
            //已发货状态 只要查询一次
            if(txRet->nPayState == 0)
            {
                m_msgHandler->putDataBuffer((char*)cd->cbData);
                OptErrorLog("handleFindRechargeSuccess find end, Recharge error 1!!!");
                return true;
            }
            
            //判断是否需要开启定时器查询
            else if(txRet->timerId == -1 && txRet->nPayState == -1)         
            {
                //开启定时查询
                txRet->remainCount = 15;
                txRet->timerId = m_msgHandler->setTimer(10 * 1000, (TimerHandler)&CThirdPlatformOpt::onFindRechargeSuccess, 0, (void*)txRet, txRet->remainCount, this);
                OptInfoLog("handleFindRechargeSuccess star timing find!!!");
                return true;
            }
            
            else if(txRet->timerId != -1 && txRet->nPayState == -1 && txRet->remainCount == 0)
            {
                m_msgHandler->killTimer(txRet->timerId);                                //取消定时器
                m_msgHandler->putDataBuffer((char*)cd->cbData);           //释放内存
                
                OptErrorLog("handleFindRechargeSuccess find end, Recharge error 2!!!");
                return true;
            }
  
            break;
		}
		
		case ETXRechargeStatus::EDeductTXCoin:     //扣除腾讯托管的游戏币
        {
            if(txRet->timerId != -1)
            {
                m_msgHandler->killTimer(txRet->timerId);      //取消定时器
            }
			
			// 充值成功后则直接扣除腾讯托管的游戏币
			// 扣除腾讯服务器上的游戏币
			OptInfoLog("deduct TX service coin, user = %s, balance = %d", txRet->userData, nTxBalance);
			txRet->remainCount = 1;           // 失败的话最多再尝试一次扣款
			txRet->nPayState = nTxBalance;
			if (!deductTXServerCurrency(HttpReplyHandlerID::IddeductTXServerCurrency, nTxBalance, txRet))
			{
				txRet->remainCount = 0;
				m_msgHandler->setTimer(1 * 1000, (TimerHandler)&CThirdPlatformOpt::onDeductTXServerCurrency, 0, (void*)txRet, 1, this);
			}

			break;
        }
    }
    
    return true;
}

bool CThirdPlatformOpt::handleDeductTXServerCurrencyMsg(ConnectData* cd)
{
    OptInfoLog("handleDeductTXServerCurrencyMsg data: %s", cd->receiveData);
    
    ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
    
	// 结果
	bool isFinish = true;
	TXRetInfo* txRet = (TXRetInfo*)cd->cbData;
    ParamKey2Value::const_iterator ret = paramKey2Value.find("ret");
	const int result = (ret == paramKey2Value.end() || ret->second != "0") ? DeductTXFishCoinError : OptSuccess;
    if (result != OptSuccess && txRet->remainCount > 0)
	{
		--txRet->remainCount;
		if (deductTXServerCurrency(HttpReplyHandlerID::IddeductTXServerCurrency, txRet->nPayState, txRet)) isFinish = false;
	}
	
	if (isFinish)
	{
		OptInfoLog("deduct TX game gold finish, srcSrvId = %u, user = %s, order = %s, result = %d", txRet->srcSrvId, txRet->userData, txRet->billno, result);
	    m_msgHandler->putDataBuffer((char*)cd->cbData);  // 释放内存
	}
	
    return true;
}

int CThirdPlatformOpt::txRecharge(ConnectData* cd, int& deductTxBalance)
{
	deductTxBalance = 0;
    ParamKey2Value paramKey2Value;
    CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
    
    TXRetInfo *txRet = (TXRetInfo*)cd->cbData;
    bool bErrorSendMsg = false;
    
    //判断失败是否要通知客户端
    if(txRet->nPayState == 0)
    {
        bErrorSendMsg = true;
    }
    else if(txRet->nPayState == -1 && txRet->remainCount == 0 && txRet->timerId != -1)
    {
        bErrorSendMsg = true;
    }
    
    OptInfoLog("txRecharge PayState:%d, remainCount:%d, timerId:%d, order = %s", txRet->nPayState, txRet->remainCount, txRet->timerId, txRet->billno);
        
    //结果
    ParamKey2Value::const_iterator ret = paramKey2Value.find("ret");
    if (ret == paramKey2Value.end())
	{
		OptErrorLog("txRecharge, can not find the check data ret");
		return ETXRechargeStatus::EContinueQuery;
	}    

    //更新失败 则通知客户端请求订单失败
	int nTxBalance = 0;     //当前腾讯服务器上的游戏币数量
    if(ret->second != "0")
    {
        //返回客户端消息
        string errorMsg;
        ParamKey2Value::const_iterator msg = paramKey2Value.find("msg");
        if(msg != paramKey2Value.end())
            errorMsg = msg->second;
        
        OptErrorLog("txRecharge, update tx server data error: %s, msg:%s, order id = %s", ret->second.c_str(), errorMsg.c_str(), txRet->billno); 
    }
    else
    {
         //腾讯的游戏币
        ParamKey2Value::const_iterator balance = paramKey2Value.find("balance");
        if (balance == paramKey2Value.end())
        {
            OptErrorLog("txRecharge, can not find the check data balance");
            return ETXRechargeStatus::EContinueQuery;
        }
        
        nTxBalance = atoi(balance->second.c_str());     //腾讯服务器上的游戏币数量
    }   
    
    //查询内部订单
    FishCoinOrderIdToInfo::iterator thirdInfoIt;
    if(!m_thirdInterface.getTranscationData(txRet->billno, thirdInfoIt))
    {
		// 充值订单信息记录
		//format = name|rechargeNo|srcSrvId|reply|return|balance|payState|remainCount
		ThirdRechargeLog(txRet->platformID, "%s|%s|%u|reply|%s|%d|%d|%d",
		txRet->userData, txRet->billno, txRet->srcSrvId, ret->second.c_str(), nTxBalance, txRet->nPayState, txRet->remainCount);
		
        OptErrorLog("handleFindRechargeSuccess, can not find order! order_id = %s", txRet->billno);
        return ETXRechargeStatus::EFinish;
    }
	
	int result = -1;
	const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
	const int beforeRechargeTxBalance = thirdInfo.userCbFlag;
    if (nTxBalance > beforeRechargeTxBalance)  // 腾讯当前的余额大于充值前的余额则表示充值成功
	{
		// 检查充值额度是否匹配，最多保留小数点后两位
		float rechargeRmbYuan = (nTxBalance - beforeRechargeTxBalance) / 10.0;
		int rechargeRmbFen = (int)(rechargeRmbYuan * 100);               // 玩家充值价格，换算成RMB分单位
		int realRmbFen = (int)(thirdInfo.money * 100);                   // 实际价格，换算成RMB分单位
		if (rechargeRmbFen < 1 || realRmbFen != rechargeRmbFen)
		{
			result = TXRechargePriceNotMatching;
			OptErrorLog("TX recharge fish coin the price not matching, user = %s, real rmb = %.2f, value = %d, pay rmb = %.2f, value = %d, order = %s",
			thirdInfo.user, thirdInfo.money, realRmbFen, rechargeRmbYuan, rechargeRmbFen, txRet->billno);
		}
		else
		{
			deductTxBalance = nTxBalance - beforeRechargeTxBalance;  // 扣除本次充值的渔币数量
		    result = 0;
		}
	}
	else
    {
        if(!bErrorSendMsg)     
        {
            OptInfoLog("not find recharge result!");
            return ETXRechargeStatus::EContinueQuery;              //继续查询
        }
    }
	
	/*
	thirdInfo.itemAmount = 0;
	const com_protocol::FishCoinInfo* fcInfo = m_msgHandler->getMallMgr().getFishCoinInfo(thirdInfo.itemId);  // 获取物品信息
	if (fcInfo != NULL)
	{
		thirdInfo.itemAmount = fcInfo->num();
		if (m_msgHandler->getMallMgr().fishCoinIsRecharged(thirdInfo.user, strlen(thirdInfo.user), thirdInfo.itemId))
		{
			thirdInfo.itemAmount += fcInfo->gift();
		}
		else
		{
			thirdInfo.itemAmount = fcInfo->first_amount();
		}
	}
	*/ 
    
    // 充值订单信息记录
    //format = name|id|amount|money|rechargeNo|srcSrvId|reply|result|payState|beforeBalance|newBalance
	ThirdRechargeLog(txRet->platformID, "%s|%u|%u|%.2f|%s|%u|reply|%d|%d|%d|%d", thirdInfo.user, thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, thirdInfoIt->first.c_str(), 
    thirdInfo.srcSrvId, result, txRet->nPayState, beforeRechargeTxBalance, nTxBalance);
    
	OptInfoLog("TX recharge, user = %s, order = %s, result = %d, tx before balance = %d, new balance = %d, pay state = %d, fish coin id = %u, amount = %d",
	thirdInfo.user, thirdInfoIt->first.c_str(), result, beforeRechargeTxBalance, nTxBalance, txRet->nPayState, thirdInfo.itemId, thirdInfo.itemAmount);
	
	char uidData[1024] = {0};
	snprintf(uidData, sizeof(uidData) - 1, "%d-%d-%d", txRet->nPayState, beforeRechargeTxBalance, nTxBalance); 
	m_thirdInterface.provideRechargeItem(result, thirdInfoIt, (ThirdPlatformType)thirdInfo.platformId, uidData, ret->second.c_str());

	return (0 == result) ? ETXRechargeStatus::EDeductTXCoin : ETXRechargeStatus::EFinish;  // 充值成功后则直接扣除腾讯托管的游戏币
}


void CThirdPlatformOpt::checkUnHandleTransactionData()
{
	m_thirdInterface.checkUnHandleOrderData();
	
	/*
	// 未处理订单最大时间间隔，超过配置的间隔时间还未处理的订单数据则直接删除
	unsigned int deleteSecs = time(NULL) - atoi(CCfg::getValue("HttpService", "UnHandleTransactionMaxIntervalSeconds"));
	for (ThirdTranscationIdToInfo::iterator it = m_thirdTransaction2Info.begin(); it != m_thirdTransaction2Info.end();)
	{
		if (it->second.timeSecs < deleteSecs)
		{
			// 充值订单信息记录，超时取消
			// format = name|type|id|count|amount|money|rechargeNo|srcSrvId|timeout
			const ThirdTranscationInfo& thirdInfo = it->second;
			ThirdRechargeLog(thirdInfo.platformId, "%s|%u|%u|%u|%u|%.2f|%s|%u|timeout",
			thirdInfo.user, thirdInfo.itemType, thirdInfo.itemId, thirdInfo.itemCount, thirdInfo.itemAmount, thirdInfo.money, it->first.c_str(), thirdInfo.srcSrvId);

			OptWarnLog("remove time out unhandle transaction data , user = %s, id = %s, type = %u, id = %u, count = %u, amount = %u, money = %.2f",
			thirdInfo.user, it->first.c_str(), thirdInfo.itemType, thirdInfo.itemId, thirdInfo.itemCount, thirdInfo.itemAmount, thirdInfo.money);
			
			removeTransactionData(it++);
		}
		else
		{
			++it;
		}
	}
	*/
}

bool CThirdPlatformOpt::saveTransactionData(const char* userName, const unsigned int srcSrvId, const unsigned int platformId,
                                            const char* transactionId, const com_protocol::GetThirdPlatformRechargeTransactionRsp& thirdRsp)
{
	return false;
	
	/*
	if (thirdRsp.result() != OptSuccess || thirdRsp.ByteSize() > (int)MaxRechargeTransactionLen) return false;

	// 订单数据格式 : type.id.count.amount.money-srv.user
	// 0.1003.1.600.0.98-1001.10000001
	char rechargeData[MaxRechargeTransactionLen] = {'\0'};
	unsigned int dataLen = snprintf(rechargeData, MaxRechargeTransactionLen - 1, "%u.%u.%u.%u.%.2f-%u.%s",
	thirdRsp.item_type(), thirdRsp.item_id(), thirdRsp.item_count(), thirdRsp.item_amount(), thirdRsp.money(), srcSrvId, userName);
	
	// 订单信息临时保存到数据库，防止服务异常退出导致订单数据丢失
	// 阻塞式同步调用
	int rc = m_msgHandler->getRedisService().setHField(RechargeDataKey, RechargeDataKeyLen, transactionId, strlen(transactionId), rechargeData, dataLen);
	if (rc != 0)
	{
		OptErrorLog("set transaction data to redis logic service error, rc = %d", rc);
		return false;
	}
	
	// 内存存储
	m_thirdTransaction2Info[transactionId] = ThirdTranscationInfo();
	ThirdTranscationInfo& info = m_thirdTransaction2Info[transactionId];
	strncpy(info.user, userName, MaxUserNameLen - 1);
	info.timeSecs = time(NULL);
	info.srcSrvId = srcSrvId;
	info.platformId = platformId;
	info.itemType = thirdRsp.item_type();
	info.itemId = thirdRsp.item_id();
	info.itemCount = thirdRsp.item_count();
	info.itemAmount = thirdRsp.item_amount();
	info.money = thirdRsp.money();
	
	return true;
	*/ 
}

void CThirdPlatformOpt::removeTransactionData(const char* transactionId)
{
	// removeTransactionData(m_thirdTransaction2Info.find(transactionId));
}

void CThirdPlatformOpt::removeTransactionData(ThirdTranscationIdToInfo::iterator it)
{
	/*
	if (it != m_thirdTransaction2Info.end())
	{
		int rc = m_msgHandler->getRedisService().delHField(RechargeDataKey, RechargeDataKeyLen, it->first.c_str(), it->first.length());
		if (rc < 0) OptErrorLog("remove transaction data error, id = %s, rc = %d", it->first.c_str(), rc);

		m_thirdTransaction2Info.erase(it);
	}
	*/ 
}

bool CThirdPlatformOpt::parseTransactionData(const char* transactionId, const char* transactionData, ThirdTranscationInfo& transcationInfo)
{
	return false;
	
	/*
	// 订单号格式 : platform.time.index
	// 3.20150621153831.123
	char transactionInfo[MaxRechargeTransactionLen] = {'\0'};
	strncpy(transactionInfo, transactionId, MaxRechargeTransactionLen - 1);
	char* platformInfo = (char*)strchr(transactionInfo, '.');
	if (platformInfo == NULL)
	{
		OptErrorLog("parse transaction id error, can not find the platform info, data = %s", transactionId);
		return false;
	}
	*platformInfo = '\0';
	unsigned int platformId = atoi(transactionInfo);
	
	char* timeInfo = (char*)strchr(++platformInfo, '.');
	if (timeInfo == NULL)
	{
		OptErrorLog("parse transaction id error, can not find the time info, data = %s", transactionId);
		return false;
	}
	*timeInfo = '\0';
	unsigned int timeSecs = atoi(platformInfo);
	
	// 订单数据格式 : type.id.count.amount.money-srv.user
	// 0.1003.1.600.0.98-1001.10000001
	unsigned int itemValue[ItemDataFlag::ISize];
	strncpy(transactionInfo, transactionData, MaxRechargeTransactionLen - 1);
	const char* itemInfo = transactionInfo;
	for (int i = 0; i < ItemDataFlag::ISize; ++i)
	{
		char* valueEnd = (char*)strchr(itemInfo, '.');
		if (*itemInfo == '-' || *itemInfo == '\0' || valueEnd == NULL)
		{
			OptErrorLog("parse transaction data error, can not find the item info, data = %s", transactionData);
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
		OptErrorLog("parse transaction data error, can not find the money info, data = %s", transactionData);
		return false;
	}
	*moneyInfo = '\0';
	float money = atof(itemInfo);
	
	// 用户信息
	char* uName = (char*)strchr(++moneyInfo, '.');
	if (uName == NULL)
	{
		OptErrorLog("parse transaction data error, can not find the user name info, data = %s", transactionData);
		return false;
	}
	*uName = '\0';
	unsigned int srcSrvId = atoi(moneyInfo);
	++uName;
	
	strncpy(transcationInfo.user, uName, MaxUserNameLen - 1);
	transcationInfo.timeSecs = timeSecs;
	transcationInfo.srcSrvId = srcSrvId;
	transcationInfo.platformId = platformId;
	transcationInfo.itemType = itemValue[ItemDataFlag::IType];
	transcationInfo.itemId = itemValue[ItemDataFlag::IId];
	transcationInfo.itemCount = itemValue[ItemDataFlag::ICount];
	transcationInfo.itemAmount = itemValue[ItemDataFlag::IAmount];
	transcationInfo.money = money;

	time_t createTimeSecs = timeSecs;
	struct tm* tmval = localtime(&createTimeSecs);
	OptInfoLog("parse transaction data, date = %d-%02d-%02d %02d:%02d:%02d, platform = %u, time = %u, item type = %u, id = %u, count = %u, amount = %u, money = %.2f, srcSrvId = %u, name = %s, transaction id = %s, data = %s",
	tmval->tm_year + 1900, tmval->tm_mon + 1, tmval->tm_mday, tmval->tm_hour, tmval->tm_min, tmval->tm_sec,
	platformId, timeSecs, itemValue[ItemDataFlag::IType], itemValue[ItemDataFlag::IId], itemValue[ItemDataFlag::ICount], itemValue[ItemDataFlag::IAmount], money,
	srcSrvId, uName, transactionId, transactionData);
	
	return true;
	*/ 
}

void CThirdPlatformOpt::getThirdRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/* 4.0版本之后，充值只能是渔币，老的充值方式不再使用，废弃
	com_protocol::GetThirdPlatformRechargeTransactionReq thirdReq;
	if (!m_msgHandler->parseMsgFromBuffer(thirdReq, data, len, "get third recharge transaction")) return;

    com_protocol::GetMallFishCoinInfoReq getMallFishCoinInfoReq;
	getMallFishCoinInfoReq.set_type(thirdReq.item_type());
	getMallFishCoinInfoReq.set_id(thirdReq.item_id());
	getMallFishCoinInfoReq.set_cb_flag(srcSrvId);
	getMallFishCoinInfoReq.set_cb_data(data, len);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(getMallFishCoinInfoReq, "get mall item data request");
	if (msgData != NULL)
	{							
		m_msgHandler->sendMessageToCommonDbProxy(msgData, getMallFishCoinInfoReq.ByteSize(), BUSINESS_GET_MALL_FISH_COIN_INFO_REQ, m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, 1);
	}
	*/
	
	
	/*
	com_protocol::GetThirdPlatformRechargeTransactionReq thirdReq;
	if (!m_msgHandler->parseMsgFromBuffer(thirdReq, data, len, "get third recharge transaction")) return;
	
	char rechargeTranscation[MaxRechargeTransactionLen] = {'\0'};
	com_protocol::GetThirdPlatformRechargeTransactionRsp thirdRsp;
	
	com_protocol::ItemInfo itemInfo;
	itemInfo.set_id(thirdReq.item_id());
	if (m_msgHandler->getMallMgr().getItemInfo(thirdReq.item_type(), itemInfo))  // 获取物品信息
	{
		// 获取订单信息
		bool isOK = false;
		thirdRsp.set_result(SrvRetCode::ParamValueError);
		if (thirdReq.platform_id() > ThirdPlatformType::AoRong && thirdReq.platform_id() < ThirdPlatformType::JinNiu)  // 只兼容老版本平台
		{
			isOK = getThirdTransactionId(thirdReq.platform_id(), thirdReq.os_type(), rechargeTranscation, MaxRechargeTransactionLen);
		}
		
		if (isOK)
		{
			thirdRsp.set_result(OptSuccess);
			thirdRsp.set_recharge_transaction(rechargeTranscation);
			thirdRsp.set_item_type(thirdReq.item_type());
			thirdRsp.set_item_id(thirdReq.item_id());
			thirdRsp.set_item_count(thirdReq.item_count());
			thirdRsp.set_item_amount(itemInfo.num() * thirdReq.item_count());
			thirdRsp.set_money(itemInfo.buy_price() * thirdReq.item_count());
			
			// 存储账单数据
			if (!saveTransactionData(m_msgHandler->getContext().userData, srcSrvId, thirdReq.platform_id(), rechargeTranscation, thirdRsp))
			{
				thirdRsp.set_result(SrvRetCode::SaveTransactionError);
			}
		}
		else
		{
			OptErrorLog("getThirdRechargeTransaction, get transaction id error, user = %s, platform id = %u, os type = %u",
			m_msgHandler->getContext().userData, thirdReq.platform_id(), thirdReq.os_type());
		}
	}
	else
	{
		thirdRsp.set_result(SrvRetCode::GetPriceFromMallDataError);
		
		OptErrorLog("getThirdRechargeTransaction, get item price for third recharge from mall error, user = %s, platform id = %d, item type = %d, id = %d, count = %d",
		m_msgHandler->getContext().userData, thirdReq.platform_id(), thirdReq.item_type(), thirdReq.item_id(), thirdReq.item_count());
	}

    int rc = -1;
    const char* msgData = m_msgHandler->serializeMsgToBuffer(thirdRsp, "send third recharge transcation to client");
	if (msgData != NULL)
	{							
		rc = m_msgHandler->sendMessage(msgData, thirdRsp.ByteSize(), srcSrvId, GET_THIRD_RECHARGE_TRANSACTION_RSP);
		if (rc == Success && thirdRsp.result() == OptSuccess)
		{
			// 充值订单信息记录
			// format = name|type|id|count|amount|money|rechargeNo|request|platformType|srcSrvId
			ThirdRechargeLog(thirdReq.platform_id(), "%s|%u|%u|%u|%u|%.2f|%s|request|%u|%u", m_msgHandler->getContext().userData, thirdRsp.item_type(), thirdRsp.item_id(),
			thirdRsp.item_count(), thirdRsp.item_amount(), thirdRsp.money(), thirdRsp.recharge_transaction().c_str(), thirdReq.platform_id(), srcSrvId);
		}
	}
	
	// 发送失败则删除订单数据
	if (rc != Success && thirdRsp.result() == OptSuccess) removeTransactionData(rechargeTranscation);
	
	OptInfoLog("third recharge transaction record, srcSrvId = %u, user = %s, money = %.2f, transaction id = %s, platform id = %u, ostype = %u, result = %d, rc = %d",
		       srcSrvId, m_msgHandler->getContext().userData, thirdRsp.money(), thirdRsp.recharge_transaction().c_str(),
			   thirdReq.platform_id(), thirdReq.os_type(), thirdRsp.result(), rc);
	*/ 
}

void CThirdPlatformOpt::getFishCoinInfoReply(const com_protocol::GetMallFishCoinInfoRsp& getMallFishCoinInfoRsp)
{
	/*
	com_protocol::GetThirdPlatformRechargeTransactionReq thirdReq;
	if (!m_msgHandler->parseMsgFromBuffer(thirdReq, getMallFishCoinInfoRsp.cb_data().c_str(), getMallFishCoinInfoRsp.cb_data().length(), "get third recharge transaction")) return;
	// if (!m_msgHandler->parseMsgFromBuffer(thirdReq, data, len, "get third recharge transaction")) return;
	
	// 获取订单信息
	char rechargeTranscation[MaxRechargeTransactionLen] = {'\0'};
	com_protocol::GetThirdPlatformRechargeTransactionRsp thirdRsp;
	
	unsigned int srcSrvId = getMallFishCoinInfoRsp.cb_flag();
	const com_protocol::FishCoinInfo& itemInfo = getMallFishCoinInfoRsp.fish_info();  // 获取物品信息
	bool isOK = false;
	thirdRsp.set_result(SrvRetCode::ParamValueError);
	if (thirdReq.platform_id() > ThirdPlatformType::AoRong && thirdReq.platform_id() < ThirdPlatformType::JinNiu)  // 只兼容老版本平台
	{
		isOK = getThirdTransactionId(thirdReq.platform_id(), thirdReq.os_type(), rechargeTranscation, MaxRechargeTransactionLen);
	}
	
	if (isOK)
	{
		thirdRsp.set_result(OptSuccess);
		thirdRsp.set_recharge_transaction(rechargeTranscation);
		thirdRsp.set_item_type(thirdReq.item_type());
		thirdRsp.set_item_id(thirdReq.item_id());
		thirdRsp.set_item_count(thirdReq.item_count());
		thirdRsp.set_item_amount(itemInfo.num() * thirdReq.item_count());
		thirdRsp.set_money(itemInfo.buy_price() * thirdReq.item_count());
		
		// 存储账单数据
		if (!saveTransactionData(m_msgHandler->getContext().userData, srcSrvId, thirdReq.platform_id(), rechargeTranscation, thirdRsp))
		{
			thirdRsp.set_result(SrvRetCode::SaveTransactionError);
		}
	}
	else
	{
		OptErrorLog("get third transaction id error, user = %s, platform id = %u, os type = %u",
		m_msgHandler->getContext().userData, thirdReq.platform_id(), thirdReq.os_type());
	}
	
    int rc = -1;
    const char* msgData = m_msgHandler->serializeMsgToBuffer(thirdRsp, "send third recharge transcation to client");
	if (msgData != NULL)
	{							
		rc = m_msgHandler->sendMessage(msgData, thirdRsp.ByteSize(), srcSrvId, GET_THIRD_RECHARGE_TRANSACTION_RSP);
		if (rc == Success && thirdRsp.result() == OptSuccess)
		{
			// 充值订单信息记录
			// format = name|type|id|count|amount|money|rechargeNo|request|platformType|srcSrvId
			ThirdRechargeLog(thirdReq.platform_id(), "%s|%u|%u|%u|%u|%.2f|%s|request|%u|%u", m_msgHandler->getContext().userData, thirdRsp.item_type(), thirdRsp.item_id(),
			thirdRsp.item_count(), thirdRsp.item_amount(), thirdRsp.money(), thirdRsp.recharge_transaction().c_str(), thirdReq.platform_id(), srcSrvId);
		}
	}
	
	// 发送失败则删除订单数据
	if (rc != Success && thirdRsp.result() == OptSuccess) removeTransactionData(rechargeTranscation);
	
	OptInfoLog("third recharge transaction record, srcSrvId = %u, user = %s, money = %.2f, transaction id = %s, platform id = %u, ostype = %u, result = %d, rc = %d",
		       srcSrvId, m_msgHandler->getContext().userData, thirdRsp.money(), thirdRsp.recharge_transaction().c_str(),
			   thirdReq.platform_id(), thirdReq.os_type(), thirdRsp.result(), rc);
	*/ 
}

void CThirdPlatformOpt::cancelThirdRechargeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/*
	com_protocol::CancelThirdPlatformRechargeNotify cancelThirdNotify;
	if (!m_msgHandler->parseMsgFromBuffer(cancelThirdNotify, data, len, "cancel third recharge notify")) return;
	
	removeTransactionData(cancelThirdNotify.recharge_transaction().c_str());  // 删除订单数据
	
	// 充值订单信息记录
	// format = name|rechargeNo|cancel|platformType|srcSrvId|reason|info
	ThirdRechargeLog(cancelThirdNotify.platform_id(), "%s|%s|cancel|%u|%u|%d|%s",
	m_msgHandler->getContext().userData, cancelThirdNotify.recharge_transaction().c_str(), cancelThirdNotify.platform_id(), srcSrvId, cancelThirdNotify.reason(), cancelThirdNotify.info().c_str());
	
	OptInfoLog("cancel third recharge, srcSrvId = %u, user = %s, transaction id = %s, platform id = %d, reason = %d, info = %s",
	srcSrvId, m_msgHandler->getContext().userData, cancelThirdNotify.recharge_transaction().c_str(), cancelThirdNotify.platform_id(), cancelThirdNotify.reason(), cancelThirdNotify.info().c_str());
	*/ 
}

// 获取第三方平台订单ID
bool CThirdPlatformOpt::getThirdTransactionId(unsigned int platformType, unsigned int osType, char* transactionId, unsigned int len)
{
	return false;
	
	/*
	const unsigned int MaxThirdPlatformTransactionLen = 32;
	if (len < MaxThirdPlatformTransactionLen) return false;

    if (platformType >= ThirdPlatformType::MaxPlatformType)
	{
		OptErrorLog("getThirdTransactionId, invalid platform type = %u", platformType);
		return false;
	}

	if (osType == ClientVersionType::AppleMerge)  // 苹果版本
	{
		platformType = platformType * ThirdPlatformType::ClientDeviceBase + platformType;  // 转换为苹果版本值
	}
	else if (osType != ClientVersionType::AndroidMerge)  // 非安卓合并版本
	{
		OptErrorLog("getThirdTransactionId, invalid os type = %u", osType);
		return false;
	}

	// 订单号格式 : platform.time.index
	// 3.20150621153831.123
	unsigned int idLen = snprintf(transactionId, len - 1, "%u.%u.%u", platformType, (unsigned int)time(NULL), ++m_transactionIndex[platformType]);
	transactionId[idLen] = 0;

	return true;
	*/ 
}

// 充值发放物品
void CThirdPlatformOpt::provideRechargeItem(int result, ThirdTranscationIdToInfo::iterator thirdInfoIt, ThirdPlatformType pType, const char* uid, const char* thirdOtherData)
{
	/*
    const ThirdTranscationInfo& thirdInfo = thirdInfoIt->second;
	if (result == OptSuccess)
	{
		// 充值成功，发放物品，充值信息入库
		com_protocol::UserRechargeReq userRechargeReq;
		userRechargeReq.set_bills_id(thirdInfoIt->first.c_str());
		userRechargeReq.set_username(thirdInfo.user);
		userRechargeReq.set_item_type(thirdInfo.itemType);
		userRechargeReq.set_item_id(thirdInfo.itemId);
		userRechargeReq.set_item_count(thirdInfo.itemCount);
		userRechargeReq.set_item_flag(0); // m_msgHandler->getMallMgr().getItemFlag(userRechargeReq.username().c_str(), userRechargeReq.username().length(), thirdInfo.itemId));
		userRechargeReq.set_recharge_rmb(thirdInfo.money);
		userRechargeReq.set_third_type(pType);
		userRechargeReq.set_third_account(uid);
		userRechargeReq.set_third_other_data(thirdOtherData);
		
		const char* msgData = m_msgHandler->serializeMsgToBuffer(userRechargeReq, "provideRechargeItem, send third platform recharge result to db proxy");
		if (msgData != NULL)
		{							
			int rc = m_msgHandler->sendMessageToCommonDbProxy(msgData, userRechargeReq.ByteSize(), BUSINESS_USER_RECHARGE_REQ, userRechargeReq.username().c_str(), userRechargeReq.username().length());
			OptInfoLog("send third platform payment result to db proxy, rc = %d, user name = %s, bill id = %s", rc, userRechargeReq.username().c_str(), userRechargeReq.bills_id().c_str());
		}
	}
	else
	{
		// 充值失败则不会发放物品，直接通知客户端
		com_protocol::ThirdPlatformRechargeResultNotify thirdPlatformRechargeResultNotify;
		thirdPlatformRechargeResultNotify.set_result(1);
		thirdPlatformRechargeResultNotify.set_recharge_transaction(thirdInfoIt->first.c_str());
		thirdPlatformRechargeResultNotify.set_item_type(thirdInfo.itemType);
		thirdPlatformRechargeResultNotify.set_item_id(thirdInfo.itemId);
		thirdPlatformRechargeResultNotify.set_item_count(thirdInfo.itemCount);
		thirdPlatformRechargeResultNotify.set_item_amount(thirdInfo.itemAmount);
		thirdPlatformRechargeResultNotify.set_money(thirdInfo.money);
		
		const char* msgData = m_msgHandler->serializeMsgToBuffer(thirdPlatformRechargeResultNotify, "send third platform recharge result to client");
		if (msgData != NULL)
		{							
			int rc = m_msgHandler->sendMessage(msgData, thirdPlatformRechargeResultNotify.ByteSize(), thirdInfo.user, strlen(thirdInfo.user), thirdInfo.srcSrvId, THIRD_USER_RECHARGE_NOTIFY);
			OptWarnLog("send third platform payment result to client, rc = %d, user name = %s, result = %d, bill id = %s",
			rc, thirdInfo.user, result, thirdInfoIt->first.c_str());
		}
	}
	*/ 
}

// 第三方平台充值结果处理
bool CThirdPlatformOpt::thirdPlatformRechargeItemReply(const com_protocol::UserRechargeRsp& userRechargeRsp)
{
	return m_thirdInterface.fishCoinRechargeItemReply(userRechargeRsp);  // 新版本渔币充值
	
	/*
	const com_protocol::UserRechargeReq& userRechargeInfo = userRechargeRsp.info();
	if (userRechargeInfo.item_type() == EGameGoodsType::EFishCoin) return m_thirdInterface.fishCoinRechargeItemReply(userRechargeRsp);  // 新版本渔币充值

	ThirdTranscationIdToInfo::iterator thirdInfoIt = m_thirdTransaction2Info.find(userRechargeInfo.bills_id());
	if (thirdInfoIt == m_thirdTransaction2Info.end())
	{
		OptErrorLog("can not find the recharge info, user = %s, bill id = %s, money = %.2f, result = %d",
		m_msgHandler->getContext().userData, userRechargeInfo.bills_id().c_str(), userRechargeInfo.recharge_rmb(), userRechargeRsp.result());

		return false;
	}

    const ThirdTranscationInfo& thirdInfo = thirdInfoIt->second;
    com_protocol::ThirdPlatformRechargeResultNotify thirdPlatformRechargeResultNotify;
	thirdPlatformRechargeResultNotify.set_result(userRechargeRsp.result());
	thirdPlatformRechargeResultNotify.set_recharge_transaction(userRechargeInfo.bills_id());
	thirdPlatformRechargeResultNotify.set_item_type(thirdInfo.itemType);
	thirdPlatformRechargeResultNotify.set_item_id(thirdInfo.itemId);
	thirdPlatformRechargeResultNotify.set_item_count(thirdInfo.itemCount);
	thirdPlatformRechargeResultNotify.set_item_amount(userRechargeRsp.item_amount());
	thirdPlatformRechargeResultNotify.set_money(thirdInfo.money);
	
	char giftData[MaxNetBuffSize] = {0};
	m_msgHandler->getGiftInfo(userRechargeRsp, thirdPlatformRechargeResultNotify.mutable_gift_array(), giftData, MaxNetBuffSize);

	int rc = -1;
	const char* msgBuff = m_msgHandler->serializeMsgToBuffer(thirdPlatformRechargeResultNotify, "thirdPlatformRechargeItemReply, send third platform recharge result to client");
	if (msgBuff != NULL) rc = m_msgHandler->sendMessage(msgBuff, thirdPlatformRechargeResultNotify.ByteSize(), thirdInfo.srcSrvId, THIRD_USER_RECHARGE_NOTIFY);

    // 充值订单信息记录最终结果
    // format = name|type|id|count|amount|flag|money|rechargeNo|srcSrvId|finish|umid|result|order|giftdata
	ThirdRechargeLog(userRechargeInfo.third_type(), "%s|%u|%u|%u|%u|%u|%.2f|%s|%u|finish|%s|%d|%s|%s", thirdInfo.user, thirdInfo.itemType, thirdInfo.itemId, thirdInfo.itemCount, userRechargeRsp.item_amount(),
	userRechargeInfo.item_flag(), thirdInfo.money, thirdInfoIt->first.c_str(), thirdInfo.srcSrvId, userRechargeInfo.third_account().c_str(), userRechargeRsp.result(), userRechargeInfo.third_other_data().c_str(), giftData);

	OptInfoLog("send third platform recharge result to client, rc = %d, srcSrvId = %u, user name = %s, result = %d, bill id = %s, money = %.2f, uid = %s, trade = %s",
	rc, thirdInfo.srcSrvId, userRechargeInfo.username().c_str(), userRechargeRsp.result(), userRechargeInfo.bills_id().c_str(), userRechargeInfo.recharge_rmb(),
	userRechargeInfo.third_account().c_str(), userRechargeInfo.third_other_data().c_str());
	
	removeTransactionData(thirdInfoIt);  // 充值操作处理完毕，删除该充值信息
	return true;
	*/ 
}

// 小米平台充值消息处理
bool CThirdPlatformOpt::handleXiaoMiPaymentMsg(Connect* conn, const RequestHeader& reqHeaderData)
{
	/* 应答消息数据，例子
	参数名称	重要性	说明
	errcode	必须	状态码,
	200 成功;
	1506 cpOrderId 错误
	1515 appId 错误
	1516 uid 错误
	1525 signature 错误
	3515 订单信息不一致，用于和 CP 的订单校验

	errMsg	可选	错误信息
	注意:对于同一个订单号的多次通知,开发商要自己保证只处理一次发货。

	例如：{“errcode”:200}
	*/

	bool isOK = handleXiaoMiPaymentMsgImpl(reqHeaderData);
	CHttpResponse rsp;
	isOK ? rsp.setContent("{\"errcode\":200}") : rsp.setContent("{\"errcode\":3515}");
	rsp.setHeaderKeyValue("Content-Type", "application/json");
	
	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to xiao mi http, rc = %d, data = %s", rc, rspMsg);
	
	return isOK;
}

// 小米平台充值消息处理
bool CThirdPlatformOpt::handleXiaoMiPaymentMsgImpl(const RequestHeader& reqHeaderData)
{
	// 签名结果
    ParamKey2Value::const_iterator signatureIt = reqHeaderData.paramKey2Value.find("signature");
	if (signatureIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleXiaoMiPaymentMsg, can not find the check data signature");
		return false;
	}
	
	// 内部订单号
	ParamKey2Value::const_iterator cpOrderIdIt = reqHeaderData.paramKey2Value.find("cpOrderId");
	if (cpOrderIdIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleXiaoMiPaymentMsg, can not find the check data cpOrderId");
		return false;
	}
	
	// 新老版本兼容处理（购买金币、道具、渔币等）
	// 内部订单号信息
	const ThirdOrderInfo* thirdInfo = NULL;
	FishCoinOrderIdToInfo::iterator newFishCoinOrderTt;
	if (!m_thirdInterface.getTranscationData(cpOrderIdIt->second.c_str(), newFishCoinOrderTt))
	{
		OptErrorLog("handleXiaoMiPaymentMsg, can not find transaction id = %s", cpOrderIdIt->second.c_str());
		return false;
	}
	
	thirdInfo = &(newFishCoinOrderTt->second);
		
	/*
	ThirdTranscationIdToInfo::iterator thirdInfoIt = m_thirdTransaction2Info.find(cpOrderIdIt->second.c_str());
	if (thirdInfoIt == m_thirdTransaction2Info.end())
	{
		// OptErrorLog("handleXiaoMiPaymentMsg, can not find transaction id = %s", cpOrderIdIt->second.c_str());
		// return false;
		
		if (!m_thirdInterface.getTranscationData(cpOrderIdIt->second.c_str(), newFishCoinOrderTt))
		{
			OptErrorLog("handleXiaoMiPaymentMsg, can not find transaction id = %s", cpOrderIdIt->second.c_str());
		    return false;
		}
		
		thirdInfo = &(newFishCoinOrderTt->second);
	}
	else
	{
		thirdInfo = &(thirdInfoIt->second);
	}
	*/ 
	
	/* 签名数据格式，例子
	appId=2882303761517239138&cpOrderId=9786bffc-996d-4553-aa33-f7e92c0b29d5&orderConsumeType=10&orderId=21140990160359583390&orderStatus=TRADE_SUCCESS
	&payFee=1&payTime=2014-09-05 15:20:27&productCode=com.demo_1&productCount=1&productName=银子1两&uid=100010
	*/ 
	// key排序之后，数据做签名消息认证
	std::vector<string> keyVector;
	for (ParamKey2Value::const_iterator it = reqHeaderData.paramKey2Value.begin(); it != reqHeaderData.paramKey2Value.end(); ++it)
	{
		if (strcmp("signature", it->first.c_str()) != 0 && it->second.length() > 0) keyVector.push_back(it->first);
	}
	
	char bufferData[MaxNetBuffSize] = {0};
	unsigned int bufferDataLen = 0;
	char signatureData[MaxNetBuffSize] = {0};
	unsigned int signatureDataLen = 0;
	std::sort(keyVector.begin(), keyVector.end());
	for (vector<string>::const_iterator it = keyVector.begin(); it != keyVector.end(); ++it)
	{
		bufferDataLen = MaxNetBuffSize;
		const string& value = reqHeaderData.paramKey2Value.at(*it);
		if (!urlDecode(value.c_str(), value.length(), bufferData, bufferDataLen))
		{
			OptErrorLog("handleXiaoMiPaymentMsg, url decode error, value = %s", value.c_str());
			return false;
		}
		
		signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, "%s=%s&", it->c_str(), bufferData);
	}
	signatureData[signatureDataLen - 1] = 0;  // 去掉最后的&符号
	
	// 哈希1运算 消息认证码
	bufferDataLen = MaxNetBuffSize;
	const char* hmacSha1Key = getThirdPlatformCfg(ThirdPlatformType::XiaoMi)->paymentKey;
    const char* signatureResult = HMAC_SHA1(hmacSha1Key, (unsigned int)strlen(hmacSha1Key), (const unsigned char*)signatureData, signatureDataLen - 1,
											(unsigned char*)bufferData, bufferDataLen);
    if (signatureResult == NULL)
	{
		OptErrorLog("handleXiaoMiPaymentMsg, HMAC SHA1 error, data = %s, key = %s", signatureData, hmacSha1Key);
		return false;
	}
	
	// 认证结果转换为16进制比较
	char signatureHex[MaxNetBuffSize] = {0};
	int signatureHexLen = b2str(signatureResult, bufferDataLen, signatureHex, MaxNetBuffSize - 1, false);
	if (signatureHexLen != (int)signatureIt->second.length() || memcmp(signatureHex, signatureIt->second.c_str(), signatureHexLen) != 0)
	{
		OptErrorLog("handleXiaoMiPaymentMsg, check the signature error, signature = %s, result = %s, data = %s, key = %s",
		signatureIt->second.c_str(), signatureHex, signatureData, hmacSha1Key);
		return false;
	}
	
	// 充值订单信息记录
    // format = name|id|amount|money|rechargeNo|srcSrvId|reply|uid|result|order|payFee|time
	bufferDataLen = MaxNetBuffSize;
	const string& payTime = reqHeaderData.paramKey2Value.at("payTime");
	urlDecode(payTime.c_str(), payTime.length(), bufferData, bufferDataLen);

	ThirdRechargeLog(ThirdPlatformType::XiaoMi, "%s|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s|%s", thirdInfo->user, thirdInfo->itemId, thirdInfo->itemAmount, thirdInfo->money,
	cpOrderIdIt->second.c_str(), thirdInfo->srcSrvId, reqHeaderData.paramKey2Value.at("uid").c_str(), reqHeaderData.paramKey2Value.at("orderStatus").c_str(),
	reqHeaderData.paramKey2Value.at("orderId").c_str(), reqHeaderData.paramKey2Value.at("payFee").c_str(), bufferData);
    
	// orderStatus	必须	订单状态，TRADE_SUCCESS 代表成功
	int result = strcmp(reqHeaderData.paramKey2Value.at("orderStatus").c_str(), "TRADE_SUCCESS");
	m_thirdInterface.provideRechargeItem(result, newFishCoinOrderTt, ThirdPlatformType::XiaoMi, reqHeaderData.paramKey2Value.at("uid").c_str(), reqHeaderData.paramKey2Value.at("orderId").c_str());
	
	/*
	// 新老版本兼容处理（购买金币、道具、渔币等）
	if (thirdInfoIt != m_thirdTransaction2Info.end())
	{
		provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::XiaoMi, reqHeaderData.paramKey2Value.at("uid").c_str(), reqHeaderData.paramKey2Value.at("orderId").c_str());
		if (result != 0) removeTransactionData(thirdInfoIt);
	}
	else
	{
		m_thirdInterface.provideRechargeItem(result, newFishCoinOrderTt, ThirdPlatformType::XiaoMi, reqHeaderData.paramKey2Value.at("uid").c_str(), reqHeaderData.paramKey2Value.at("orderId").c_str());
	}
	*/ 

	return true;
}


// 豌豆荚平台充值消息处理
bool CThirdPlatformOpt::handleWanDouJiaPaymentMsg(Connect* conn, const HttpMsgBody& msgBody)
{
	bool isOK = handleWanDouJiaPaymentMsgImpl(msgBody);
	CHttpResponse rsp;
	isOK ? rsp.setContent("success") : rsp.setContent("fail");

	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to wan dou jia http, rc = %d, data = %s", rc, rspMsg);
	
	return isOK;
}

// 豌豆荚平台充值消息处理
bool CThirdPlatformOpt::handleWanDouJiaPaymentMsgImpl(const HttpMsgBody& msgBody)
{
	/* POST 返回的数据内容形式
sign=gDMyrGGsH%2BgiPAnfq5fzfwHPg0LC0zZpFwKte4f2m7PC%2BS2DG%2BhTZZsNkCQhqlF9BJsHZ9dI3xXU9dDIiPxzMzCc3%2FON0qNAs7Ccu4BVpNMAxt1qDD8JO3mX7uWUgZjsOJj7hPj%2FWhSDtwppulqLZwmEI%2BnflroZMmbmFuVcknY%3D&content=%7B%22orderId%22%3A223500293%2C%22appKeyId%22%3A100034529%2C%22buyerId%22%3A187090218%2C%22cardNo%22%3Anull%2C%22money%22%3A1%2C%22chargeType%22%3A%22BALANCEPAY%22%2C%22timeStamp%22%3A1448003120991%2C%22out_trade_no%22%3A%224.1447990904.10%22%7D
	&signType=RSA
	*/
	// 付费订单信息
	ParamKey2Value::const_iterator contentIt = msgBody.paramKey2Value.find("content");
	if (contentIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleWanDouJiaPaymentMsgImpl, can not find the check data content");
		return false;
	}
	
	// 内容URL解码
	char bufferData[MaxNetBuffSize] = {0};
	unsigned int bufferDataLen = MaxNetBuffSize;
	if (!urlDecode(contentIt->second.c_str(), contentIt->second.length(), bufferData, bufferDataLen))
	{
		OptErrorLog("handleWanDouJiaPaymentMsgImpl, content url decode error, value = %s", contentIt->second.c_str());
		return false;
	}
	
	// 内部订单号
	ParamKey2Value orderInfo;
	CHttpDataParser::parseJSONContent(bufferData, orderInfo);
	ParamKey2Value::const_iterator orderIdIt = orderInfo.find("out_trade_no");
	if (orderIdIt == orderInfo.end())
	{
		OptErrorLog("handleWanDouJiaPaymentMsgImpl, can not find the check data out_trade_no");
		return false;
	}

    // 新老版本兼容处理（购买金币、道具、渔币等）
	// 内部订单号信息
	const ThirdOrderInfo* thirdInfo = NULL;
	FishCoinOrderIdToInfo::iterator newFishCoinOrderTt;
	if (!m_thirdInterface.getTranscationData(orderIdIt->second.c_str(), newFishCoinOrderTt))
	{
		OptErrorLog("handleWanDouJiaPaymentMsgImpl, can not find transaction id = %s", orderIdIt->second.c_str());
		return false;
	}
	
	thirdInfo = &(newFishCoinOrderTt->second);
		
	/*
	ThirdTranscationIdToInfo::iterator thirdInfoIt = m_thirdTransaction2Info.find(orderIdIt->second.c_str());
	if (thirdInfoIt == m_thirdTransaction2Info.end())
	{
		if (!m_thirdInterface.getTranscationData(orderIdIt->second.c_str(), newFishCoinOrderTt))
		{
			OptErrorLog("handleWanDouJiaPaymentMsgImpl, can not find transaction id = %s", orderIdIt->second.c_str());
		    return false;
		}
		
		thirdInfo = &(newFishCoinOrderTt->second);
	}
	else
	{
		thirdInfo = &(thirdInfoIt->second);
	}
	*/ 
	
	// 签名结果
    ParamKey2Value::const_iterator signatureIt = msgBody.paramKey2Value.find("sign");
	if (signatureIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleWanDouJiaPaymentMsgImpl, can not find the check data sign");
		return false;
	}
	
	// 签名URL解码
	char signatureData[MaxNetBuffSize] = {0};
	unsigned int signatureDataLen = MaxNetBuffSize;
	if (!urlDecode(signatureIt->second.c_str(), signatureIt->second.length(), signatureData, signatureDataLen))
	{
		OptErrorLog("handleWanDouJiaPaymentMsgImpl, sign url decode error, value = %s", signatureIt->second.c_str());
		return false;
	}
	
	// 签名结果base64解码
	char base64DecodeBuff[MaxNetBuffSize] = {0};
	int base64DeCodeLen = base64Decode(signatureData, signatureDataLen, base64DecodeBuff, MaxNetBuffSize);
	if (base64DeCodeLen < 1)
	{
		OptErrorLog("handleWanDouJiaPaymentMsgImpl, sign url base64 decode error, value = %s", signatureData);
		return false;
	}
	
	// SHA1摘要做RSA验证
	static const char* wanDouJiaPublicKey = "-----BEGIN PUBLIC KEY-----\nMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCd95FnJFhPinpNiE/h4VA6bU1rzRa5+a2\n5BxsnFX8TzquWxqDCoe4xG6QKXMXuKvV57tTRpzRo2jeto40eHKClzEgjx9lTYVb2RFHHFW\nio/YGTfnqIPTVpi7d7uHY+0FZ0lYL5LlW4E2+CQMxFOPRwfqGzMjs1SDlH7lVrLEVy6QIDA\nQAB\n-----END PUBLIC KEY-----\n";
	static const unsigned int wanDouJiaPublicKeyLen = strlen(wanDouJiaPublicKey);
	bool verifyOk = sha1RSAVerify(wanDouJiaPublicKey, wanDouJiaPublicKeyLen, (const unsigned char*)bufferData, bufferDataLen, (const unsigned char*)base64DecodeBuff, base64DeCodeLen);
	if (!verifyOk)
	{
		OptErrorLog("handleWanDouJiaPaymentMsgImpl, check the signature error, data = %s, result = %s, key = %s", bufferData, signatureData, wanDouJiaPublicKey);
		return false;
	}

	// 充值订单信息记录
    // format = name|id|amount|money|rechargeNo|srcSrvId|reply|uid|order|chargeType|money|time
	ThirdRechargeLog(ThirdPlatformType::WanDouJia, "%s|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s|%s", thirdInfo->user, thirdInfo->itemId, thirdInfo->itemAmount,
	thirdInfo->money, orderIdIt->second.c_str(), thirdInfo->srcSrvId, orderInfo.at("buyerId").c_str(), orderInfo.at("orderId").c_str(),
	orderInfo.at("chargeType").c_str(), orderInfo.at("money").c_str(), orderInfo.at("timeStamp").c_str());

    /*
	// 豌豆荚的回调必定成功
	// 新老版本兼容处理（购买金币、道具、渔币等）
	if (thirdInfoIt != m_thirdTransaction2Info.end())
	{
		provideRechargeItem(Success, thirdInfoIt, ThirdPlatformType::WanDouJia, orderInfo.at("buyerId").c_str(), orderInfo.at("orderId").c_str());
	}
	else
	{
		m_thirdInterface.provideRechargeItem(Success, newFishCoinOrderTt, ThirdPlatformType::WanDouJia, orderInfo.at("buyerId").c_str(), orderInfo.at("orderId").c_str());
	}
    */
	
	m_thirdInterface.provideRechargeItem(Success, newFishCoinOrderTt, ThirdPlatformType::WanDouJia, orderInfo.at("buyerId").c_str(), orderInfo.at("orderId").c_str());
	return true;
}

bool CThirdPlatformOpt::handleQiHu360PaymentMsg(Connect* conn, const RequestHeader& reqHeaderData)
{
    bool isOK = handleQiHu360PaymentMsgImpl(reqHeaderData);
	CHttpResponse rsp;
	isOK ? rsp.setContent("ok") : rsp.setContent("failure");
	
	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to qi hu 360 http, rc = %d, data = %s", rc, rspMsg);
	return isOK;
}

bool CThirdPlatformOpt::handleQiHu360PaymentMsgImpl(const RequestHeader& reqHeaderData)
{
    // 签名结果
    ParamKey2Value::const_iterator signatureIt = reqHeaderData.paramKey2Value.find("sign");
	if (signatureIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleQiHu360PaymentMsg, can not find the check data sign return");
		return false;
	}
	
	// 内部订单号
	ParamKey2Value::const_iterator cpOrderIdIt = reqHeaderData.paramKey2Value.find("app_order_id");
    if (cpOrderIdIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("handleQiHu360PaymentMsg, can not find the check data app order id");
		return false;
	}
	
	// 内部订单号信息
	FishCoinOrderIdToInfo::iterator thirdInfoIt;
	if (!m_thirdInterface.getTranscationData(cpOrderIdIt->second.c_str(), thirdInfoIt))
	{
		OptErrorLog("handleQiHu360PaymentMsg, can not find transaction id = %s", cpOrderIdIt->second.c_str());
		return false;
	}
    
    //  内部订单价格与第三方价格是否匹配
    ParamKey2Value::const_iterator amountIt = reqHeaderData.paramKey2Value.find("amount");
    if(amountIt == reqHeaderData.paramKey2Value.end())
    {
        OptErrorLog("handleQiHu360PaymentMsg, can not find amount");
        return false;
    }
	
    float fAmount = atof(amountIt->second.c_str()) / 100.0;      //  单位为分所以换算成元
    if(fabs(thirdInfoIt->second.money - fAmount) > 0.01)
    {
  		OptErrorLog("handleQiHu360PaymentMsg, Price is not equal. money=%f, amount=%f", thirdInfoIt->second.money, fAmount);
        return false;
    }
    
    //剔除掉不需要参与签名的参数
    //注意 Map中的元素是自动按key升序排序
    map<string, string> signMap;
    for(ParamKey2Value::const_iterator it = reqHeaderData.paramKey2Value.begin(); 
        it != reqHeaderData.paramKey2Value.end(); ++it)
    {
        if (!(strcmp("sign_return", it->first.c_str()) == 0 || strcmp("sign", it->first.c_str()) == 0) 
            && it->second.length() > 0) 
        {
            signMap.insert(pair<string, string>(it->first, it->second));
        }
    }
    
    //格式化
	unsigned int signatureDataLen = 0;
    char signatureData[MaxNetBuffSize] = {0};
    //for(auto & it : signMap)
    for(map<string, string>::iterator it = signMap.begin(); it != signMap.end(); ++it)
    {        
        signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                    "%s#", (*it).second.c_str());
    }
     
    //appsecret
    signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                 "%s", getThirdPlatformCfg(ThirdPlatformType::QiHu360)->paymentKey);
    //MD5加密
    char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(signatureData, signatureDataLen, md5Buff);
    
    // OptInfoLog("handleQiHu360PaymentMsg signatureData: %s", signatureData);
    
    if (strcmp(md5Buff, signatureIt->second.c_str()) != 0)
	{
		OptErrorLog("handleQiHu360PaymentMsg, check md5 error, data = %s, sign = %s, result = %s", 
                    signatureData, signatureIt->second.c_str(), md5Buff);
		return false;
	}
    
 	// 充值订单信息记录
    //format = name|id|amount|money|rechargeNo|srcSrvId|reply|uid|result|order|payFee|time
	const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
	ThirdRechargeLog(ThirdPlatformType::QiHu360, "%s|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s", thirdInfo.user, 
    thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, thirdInfoIt->first.c_str(), 
    thirdInfo.srcSrvId, reqHeaderData.paramKey2Value.at("user_id").c_str(), reqHeaderData.paramKey2Value.at("gateway_flag").c_str(),
    reqHeaderData.paramKey2Value.at("order_id").c_str(), reqHeaderData.paramKey2Value.at("amount").c_str());
    
	// orderStatus	必须	订单状态，success 代表成功
	int result = strcmp(reqHeaderData.paramKey2Value.at("gateway_flag").c_str(), "success");
	m_thirdInterface.provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::QiHu360,
	reqHeaderData.paramKey2Value.at("user_id").c_str(), reqHeaderData.paramKey2Value.at("order_id").c_str());
    
    return true;
}


bool CThirdPlatformOpt::handleUCPaymentMsg(Connect* conn, const HttpMsgBody& msgBody)
{
	bool isOK = handleUCPaymentMsgImpl(msgBody);
	CHttpResponse rsp;
	isOK ? rsp.setContent("success") : rsp.setContent("failure");
	
	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to UC http, rc = %d, data = %s", rc, rspMsg);
	return isOK;
}

bool CThirdPlatformOpt::handleUCPaymentMsgImpl(const HttpMsgBody& msgBody)
{
	// MD5校验结果
    ParamKey2Value::const_iterator signIt = msgBody.paramKey2Value.find("sign");
	if (signIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleUCPaymentMsg, can not find the check data sign");
		return false;
	}

	// 内部订单号
	ParamKey2Value::const_iterator cpOrderIdIt = msgBody.paramKey2Value.find("orderId");
    if (cpOrderIdIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleUCPaymentMsg, can not find the check data app order id");
		return false;
	}
	
	// 内部订单号信息
	FishCoinOrderIdToInfo::iterator thirdInfoIt;
	if (!m_thirdInterface.getTranscationData(cpOrderIdIt->second.c_str(), thirdInfoIt))
	{
		OptErrorLog("handleUCPaymentMsg, can not find transaction id = %s", cpOrderIdIt->second.c_str());
		return false;
	}

	const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
	char dataBuff[MaxNetBuffSize] = {0};
	snprintf(dataBuff, MaxNetBuffSize - 1, "%.2f", thirdInfo.money);
	if (strcmp(dataBuff, msgBody.paramKey2Value.at("amount").c_str()) != 0)
	{
		OptErrorLog("handleUCPaymentMsg, the pay money invalid, pay = %s, record = %.2f", msgBody.paramKey2Value.at("amount").c_str(), thirdInfo.money);
		return false;
	}

	//剔除掉不需要参与签名的参数
    //注意 Map中的元素是自动按key升序排序
	ParamKey2Value::const_iterator dataIt = msgBody.paramKey2Value.find("data");
    map<string, string> signMap;
    for(ParamKey2Value::const_iterator it = msgBody.paramKey2Value.begin(); 
        it != msgBody.paramKey2Value.end(); ++it)
    {
        if (!(strcmp("ver", it->first.c_str()) == 0 || strcmp("sign", it->first.c_str()) == 0)) 
        {
        	if (strcmp("data", it->first.c_str()) == 0)
        	{
				signMap.insert(pair<string, string>("failedDesc", ""));
        	}
        	else
        	{
            	signMap.insert(pair<string, string>(it->first, it->second));        		
        	}
        }
    }

	//格式化
	unsigned int signatureDataLen = 0;
    char signatureData[MaxNetBuffSize] = {0};
    //for(auto & it : signMap)
    for(map<string, string>::iterator it = signMap.begin(); it != signMap.end(); ++it)
    {        
        signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                    "%s=%s", (*it).first.c_str() , (*it).second.c_str());
    }
     
    //apiKey
    signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                 "%s", getThirdPlatformCfg(ThirdPlatformType::UCSDK)->paymentKey);

	//MD5校验
	char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(signatureData, signatureDataLen, md5Buff);    
    if (strcmp(md5Buff, signIt->second.c_str()) != 0)
	{
		OptErrorLog("handleUCPaymentMsg, check md5 error, data = %s, sign = %s, result = %s", 
                    signatureData, signIt->second.c_str(), md5Buff);
		return false;
	}

	// 充值订单信息记录
    //format = name|id|amount|money|rechargeNo|srcSrvId|reply|uid|result|order|payFee|time
	ThirdRechargeLog(ThirdPlatformType::UCSDK, "%s|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s|%s", thirdInfo.user, 
    thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, thirdInfoIt->first.c_str(), 
    thirdInfo.srcSrvId, msgBody.paramKey2Value.at("gameId").c_str(), msgBody.paramKey2Value.at("orderStatus").c_str(),
    msgBody.paramKey2Value.at("tradeId").c_str(), msgBody.paramKey2Value.at("amount").c_str(), msgBody.paramKey2Value.at("tradeTime").c_str());

    // orderStatus	必须	订单状态，"S" 代表成功 "F" 代表失败
	int result = strcmp(msgBody.paramKey2Value.at("orderStatus").c_str(), "S");
	m_thirdInterface.provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::UCSDK,
	msgBody.paramKey2Value.at("gameId").c_str(), msgBody.paramKey2Value.at("tradeId").c_str());

	return true;
}

bool CThirdPlatformOpt::handleOppoPaymentMsg(Connect* conn, const HttpMsgBody& msgBody)
{
    bool isOK = handleOppoPaymentMsgImpl(msgBody);
	CHttpResponse rsp;
	isOK ? rsp.setContent("{result:OK&resultMsg:OK}") : rsp.setContent("{result:FAIL&resultMsg:}");

	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to Oppo http, rc = %d, data = %s", rc, rspMsg);
	return isOK;
}

bool CThirdPlatformOpt::handleOppoPaymentMsgImpl(const HttpMsgBody& msgBody)
{
    // 签名结果
    ParamKey2Value::const_iterator signIt = msgBody.paramKey2Value.find("sign");
	if (signIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleOppoPaymentMsg, can not find the check data sign");
		return false;
	}

	// 内部订单号
	ParamKey2Value::const_iterator orderIdIt = msgBody.paramKey2Value.find("partnerOrder");
    if (orderIdIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleOppoPaymentMsg, can not find the check data app order id");
		return false;
	}
	
	// 内部订单号信息
	FishCoinOrderIdToInfo::iterator thirdInfoIt;
	if (!m_thirdInterface.getTranscationData(orderIdIt->second.c_str(), thirdInfoIt))
	{
		OptErrorLog("handleOppoPaymentMsg, can not find transaction id = %s", orderIdIt->second.c_str());
		return false;
	}

    //生成签名数据
    unsigned int signatureDataLen = 0;
    char signatureData[MaxNetBuffSize] = {0};
    
    //Oppo 订单号
    ParamKey2Value::const_iterator notifyIdIt = msgBody.paramKey2Value.find("notifyId");
    if(notifyIdIt == msgBody.paramKey2Value.end())
    {
        OptErrorLog("handleOppoPaymentMsg, can not find the check data notify Id");
		return false;
    }
    signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                 "notifyId=%s&" , (notifyIdIt->second.c_str()));
                                 
    //内部订单号
    signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                 "partnerOrder=%s&", orderIdIt->second.c_str());    
 
    //商品名称
    ParamKey2Value::const_iterator productNameIt = msgBody.paramKey2Value.find("productName");
    if(productNameIt == msgBody.paramKey2Value.end())
    {
        OptErrorLog("handleOppoPaymentMsg, can not find the check data product Name");
		return false;
    }
    signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                 "productName=%s&", productNameIt->second.c_str());    

    //商品描述
    ParamKey2Value::const_iterator productDescIt = msgBody.paramKey2Value.find("productDesc");
    if(productDescIt == msgBody.paramKey2Value.end())
    {
        OptErrorLog("handleOppoPaymentMsg, can not find the check data product Desc");
		return false;
    }
    signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                 "productDesc=%s&", productDescIt->second.c_str());  

    //商品价格
    ParamKey2Value::const_iterator priceIt = msgBody.paramKey2Value.find("price");
    if(priceIt == msgBody.paramKey2Value.end())
    {
        OptErrorLog("handleOppoPaymentMsg, can not find the check data price");
		return false;
    }
    signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                 "price=%s&", priceIt->second.c_str());  
                      
    //商品数量
    ParamKey2Value::const_iterator countIt = msgBody.paramKey2Value.find("count");
    if(countIt == msgBody.paramKey2Value.end())
    {
        OptErrorLog("handleOppoPaymentMsg, can not find the check data count");
		return false;
    }
    signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                 "count=%s&", countIt->second.c_str());  
                                 
    //attach
    ParamKey2Value::const_iterator attachIt = msgBody.paramKey2Value.find("attach");
    if(attachIt == msgBody.paramKey2Value.end())
    {
        OptErrorLog("handleOppoPaymentMsg, can not find the check data attach");
		return false;
    }
    signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, 
                                 "attach=%s", attachIt->second.c_str());  
    
   /* //进行 base64编码
    unsigned int base64buffDataLen = 0;
    char base64buffData[MaxNetBuffSize] = {0};
    if(!base64Encode(signatureData, signatureDataLen, base64buffData, base64buffDataLen))
    {
        OptErrorLog("handleOppoPaymentMsg base64Encode Error. signatureData=%s, signatureDataLen=%d", signatureData, signatureDataLen);
        return false;
    }

    //RSA
    unsigned int rsabuffDataLen = 0;
    char rsabuffData[MaxNetBuffSize] = {0};
    auto paymentKey = getThirdPlatformCfg(ThirdPlatformType::Oppo)->paymentKey;
    if(!sha1RSASign(paymentKey, strlen(paymentKey), (const unsigned char*)base64buffData, base64buffDataLen, 
      (unsigned char*)rsabuffData, rsabuffDataLen, false))

    {
        OptErrorLog("handleOppoPaymentMsg sha1RSASign Error. paymentKey=%s, paymentKeylen=%d, base64buffData=%s, base64buffDatalen=%d", 
        paymentKey, strlen(paymentKey), base64buffData, base64buffDataLen);
        return false;
    }
     
    //校验
    if (strcmp(rsabuffData, signIt->second.c_str()) != 0)
	{
		OptErrorLog("handleOppoPaymentMsg, check md5 error, data = %s, sign = %s, result = %s", 
                    signatureData, signIt->second.c_str(), rsabuffData);
		return false;
	}

	// 充值订单信息记录
    //format = name|id|amount|money|rechargeNo|srcSrvId|reply|uid|result|order|payFee|time
    const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
	ThirdRechargeLog(ThirdPlatformType::Oppo, "%s|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s", thirdInfo.user, 
    thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, thirdInfoIt->first.c_str(), 
    thirdInfo.srcSrvId, attachIt->second.c_str(), "0", notifyIdIt->second.c_str(), priceIt->second.c_str());

    // 
	int result = 0;
	provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::Oppo, attachIt->second.c_str(), notifyIdIt->second.c_str());
	*/
    return false;
}

bool CThirdPlatformOpt::handleMuMaYiPaymentMsg(Connect* conn, const HttpMsgBody& msgBody)
{
    bool isOK = handleMuMaYiPaymentMsgImpl(msgBody);
	CHttpResponse rsp;
	isOK ? rsp.setContent("success") : rsp.setContent("fail");

	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to mu ma yi http, rc = %d, data = %s", rc, rspMsg);
	return isOK;
}

bool CThirdPlatformOpt::handleMuMaYiPaymentMsgImpl(const HttpMsgBody& msgBody)
{
     // 签名
    ParamKey2Value::const_iterator signIt = msgBody.paramKey2Value.find("tradeSign");
    if(signIt == msgBody.paramKey2Value.end())
    {
        OptErrorLog("handleMuMaYiPaymentMsg, can not find the sign");
		return false;
    }
    
     // 用户ID
	ParamKey2Value::const_iterator userIdIt = msgBody.paramKey2Value.find("uid");
    if (userIdIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMuMaYiPaymentMsg, can not find the user id");
		return false;
	}
	
    // 结果
	ParamKey2Value::const_iterator tradeStateIt = msgBody.paramKey2Value.find("tradeState");
    if (tradeStateIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMuMaYiPaymentMsg, can not find the trade State");
		return false;
	}
    
    //外部订单号
    ParamKey2Value::const_iterator thirdPartyorderIt = msgBody.paramKey2Value.find("orderID");
    if (thirdPartyorderIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMuMaYiPaymentMsg, can not find the orderID");
		return false;
	}
    
    //价格
    ParamKey2Value::const_iterator priceIt = msgBody.paramKey2Value.find("productPrice");
    if (priceIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMuMaYiPaymentMsg, can not find the price");
		return false;
	}
    
    // 内部订单号
	ParamKey2Value::const_iterator orderIdIt = msgBody.paramKey2Value.find("productDesc");
    if (orderIdIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleMuMaYiPaymentMsg, can not find the check data app order id");
		return false;
	}
    
	// 内部订单号信息
	FishCoinOrderIdToInfo::iterator thirdInfoIt;
	if (!m_thirdInterface.getTranscationData(orderIdIt->second.c_str(), thirdInfoIt))
	{
		OptErrorLog("handleMuMaYiPaymentMsg, can not find transaction id = %s", orderIdIt->second.c_str());
		return false;
	}
    
    //签名长度
    if(signIt->second.size() < 14)
    {
        OptErrorLog("handleMuMaYiPaymentMsg, sign size less than 14", signIt->second.size());
        return false;
    }
          
    auto data = signIt->second;
    
    auto verity_str = data.substr(0, 8);
    
    data = data.substr(8);
    
    OptInfoLog("data = %s", data.c_str());
    
    //MD5 加密
    char md5Buff[MaxNetBuffSize] = {0};
    md5Encrypt(data.c_str(), data.size(), md5Buff);
    string temp = md5Buff;
    
    if(verity_str != temp.substr(0, 8))
    {
        OptErrorLog("handleMuMaYiPaymentMsg verity_str != temp.substr(0, 8), verity_str = %s, temp = %s", verity_str.c_str(), temp.substr(0, 8).c_str());
        return false;
    }
        
    auto key_b = data.substr(0, 6);
    auto rand_key = key_b + getThirdPlatformCfg(ThirdPlatformType::MuMaYi)->appKey;
    
    md5Encrypt(rand_key.c_str(), rand_key.size(), md5Buff);
    rand_key = md5Buff;
    
    char base64DecodeBuff[MaxNetBuffSize] = {0};
    data = data.substr(6);
    int base64DeCodeLen = base64Decode(data.c_str(), data.size(), base64DecodeBuff, MaxNetBuffSize);
    if (base64DeCodeLen < 1)
	{
		OptErrorLog("handleMuMaYiPaymentMsg, base64 decode error, value = %s", data.c_str());
		return false;
	}
    
    OptInfoLog("data=%s, base64DecodeBuff=%s, base64DeCodeLen=%d", data.c_str(), base64DecodeBuff, base64DeCodeLen);
    
    string verfic;
    for(int i = 0; i < base64DeCodeLen; i++)
    {
       verfic += char(base64DecodeBuff[i] ^ rand_key[i % 32]);
    }
    
    //验证失败
    if(verfic != thirdPartyorderIt->second)
    {
        OptErrorLog("handleMuMaYiPaymentMsg, verification fail verfic = %s, send_verfic = %s", verfic.c_str(), thirdPartyorderIt->second.c_str());
        return false;
    }
        
    // 充值订单信息记录
    //format = name|id|amount|money|rechargeNo|srcSrvId|reply|uid|result|order|payFee|time
    const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
	ThirdRechargeLog(ThirdPlatformType::MuMaYi, "%s|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s", thirdInfo.user, 
    thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, thirdInfoIt->first.c_str(), 
    thirdInfo.srcSrvId, userIdIt->second.c_str(), tradeStateIt->second.c_str(), thirdPartyorderIt->second.c_str(), priceIt->second.c_str());

	int result = strcmp(tradeStateIt->second.c_str(), "success");
	m_thirdInterface.provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::MuMaYi, userIdIt->second.c_str(), thirdPartyorderIt->second.c_str());
    return true;
}
    
bool CThirdPlatformOpt::handleAnZhiPaymentMsg(Connect* conn, const HttpMsgBody& msgBody)
{
    bool isOK = handleAnZhiPaymentMsgImpl(msgBody);
	CHttpResponse rsp;
	isOK ? rsp.setContent("success") : rsp.setContent("fail");

	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to an zhi http, rc = %d, data = %s", rc, rspMsg);
	return isOK;
}

bool CThirdPlatformOpt::handleAnZhiPaymentMsgImpl(const HttpMsgBody& msgBody)
{
    ParamKey2Value::const_iterator dataIt = msgBody.paramKey2Value.find("data");
    if(dataIt == msgBody.paramKey2Value.end())
    {
        OptErrorLog("handleAnZhiPaymentMsgImpl,  can not find the data");
        return false;
    }

    //Url解码
	char urlData[MaxNetBuffSize] = { 0 };
    unsigned int nUrlDataLen = MaxNetBuffSize;
	if (!urlDecode(dataIt->second.c_str(), (unsigned int)dataIt->second.size(), urlData, nUrlDataLen))
	{
       // OptErrorLog("handleAnZhiPaymentMsgImpl Url Decode Error, data=%s, len=%d", dataIt->second.c_str(), dataIt->second.szie());
		return false;
	}

	//Base64解码
	char base64Data[MaxNetBuffSize] = {0};
    unsigned int nBase64DataLen = MaxNetBuffSize;
    nBase64DataLen = base64Decode(urlData, nUrlDataLen, base64Data, nBase64DataLen);
	if (nBase64DataLen < 1)
	{
        OptErrorLog("handleAnZhiPaymentMsgImpl Base64 Decode Error, data=%s, len=%d", urlData, nUrlDataLen);
		return false;
	}
	
    // DES3解码
    string strBase64Data(base64Data, nBase64DataLen);
    string decodeResult = dESEcb3Encrypt(strBase64Data, getThirdPlatformCfg(ThirdPlatformType::AnZhi)->appId);
    if(decodeResult.empty())
    {
        OptErrorLog("handleAnZhiPaymentMsgImpl DESEcb3Encrypt Error, data=%s, len=%d", base64Data, nBase64DataLen);
        return false;
    }
    
    OptInfoLog("handleAnZhiPaymentMsgImpl dESEcb3Encrypt: %s", decodeResult.c_str());
    
    
    ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(decodeResult.c_str(), '{'), paramKey2Value);

    ParamKey2Value::iterator cpInfoIt = paramKey2Value.find("cpInfo");  
    if(cpInfoIt == paramKey2Value.end())
    {
        OptErrorLog("handleAnZhiPaymentMsgImpl can not find the cpInfo");
        return false;
    }
 
    // 内部订单号信息
	FishCoinOrderIdToInfo::iterator thirdInfoIt;
	if (!m_thirdInterface.getTranscationData(cpInfoIt->second.c_str(), thirdInfoIt))
	{
		OptErrorLog("handleAnZhiPaymentMsgImpl, can not find transaction id = %s", cpInfoIt->second.c_str());
		return false;
	}
    const ThirdOrderInfo& thirdInfo = thirdInfoIt->second;
    
    // 价格
    ParamKey2Value::iterator payAmountIt = paramKey2Value.find("payAmount");  
    if(payAmountIt == paramKey2Value.end())
    {
        OptErrorLog("handleAnZhiPaymentMsgImpl can not find the pay Amount");
        return false;
    }
    
    // 价格比较
    float fAmount = atof(payAmountIt->second.c_str()) / 100.0;      //  单位为分所以换算成元
    if(fabs(thirdInfo.money - fAmount) > 0.01)
    {
        OptErrorLog("handleAnZhiPaymentMsgImpl, money error!, third money=%f, my money=%f", fAmount, thirdInfo.money);
        return false;
    }
    
    // Uid
    ParamKey2Value::iterator uidIt = paramKey2Value.find("uid");  
    if(uidIt == paramKey2Value.end())
    {
        OptErrorLog("handleAnZhiPaymentMsgImpl can not find the uid");
        return false;
    }
    
    // 外部订单号
    ParamKey2Value::iterator orderIdIt = paramKey2Value.find("orderId");  
    if(orderIdIt == paramKey2Value.end())
    {
        OptErrorLog("handleAnZhiPaymentMsgImpl can not find the order Id");
        return false;
    }
        
    // 订单状态
    ParamKey2Value::iterator codeIt = paramKey2Value.find("code");  
    if(codeIt == paramKey2Value.end())
    {
        OptErrorLog("handleAnZhiPaymentMsgImpl can not find the code");
        return false;
    }
            
    // 充值订单信息记录
    //format = name|id|amount|money|rechargeNo|srcSrvId|reply|uid|result|order|payFee|time
	ThirdRechargeLog(ThirdPlatformType::AnZhi, "%s|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s", thirdInfo.user, 
    thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, thirdInfoIt->first.c_str(), 
    thirdInfo.srcSrvId, uidIt->second.c_str(), codeIt->second.c_str(), orderIdIt->second.c_str(), payAmountIt->second.c_str());

	int result = (codeIt->second == "1" ? 0 : 1);
	m_thirdInterface.provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::AnZhi, uidIt->second.c_str(), orderIdIt->second.c_str());
    
    return true;
}


// 第三方用户校验
void CThirdPlatformOpt::checkThirdUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ThirdPlatformUserCheckReq thirdPlatformUserCheckReq;
	if (!m_msgHandler->parseMsgFromBuffer(thirdPlatformUserCheckReq, data, len, "check third user")) return;
	
	unsigned int platform_id = thirdPlatformUserCheckReq.platform_id();
	if (platform_id < ThirdPlatformType::WanDouJia || platform_id >= ThirdPlatformType::MaxPlatformType)
	{
		OptErrorLog("checkThirdUser, the platform id is invalid, id = %d", platform_id);
		return;
	}
	
	ParamKey2Value key2value;
	for (int i = 0; i < thirdPlatformUserCheckReq.key_value_size(); ++i)
	{
		const com_protocol::ThirdCheckKeyValue& kv = thirdPlatformUserCheckReq.key_value(i);
		key2value[kv.key()] = kv.value();
	}
	
	m_msgHandler->handleCheckUserRequest(platform_id, key2value, srcSrvId);
}

void CThirdPlatformOpt::checkWanDouJiaUser(const ParamKey2Value& key2value, unsigned int srcSrvId)
{
	ParamKey2Value::const_iterator uidIt = key2value.find("uid");
	if (uidIt == key2value.end())
	{
		OptErrorLog("checkWanDouJiaUser, can not find the uid");
		return;
	}
	
	ParamKey2Value::const_iterator tokenIt = key2value.find("token");
	if (tokenIt == key2value.end())
	{
		OptErrorLog("checkWanDouJiaUser, can not find the token");
		return;
	}
	
	char encodeData[MaxNetBuffSize] = {0};
	unsigned int encodeDataLen = MaxNetBuffSize;
	if (!urlEncode(tokenIt->second.c_str(), tokenIt->second.length(), encodeData, encodeDataLen))
	{
		OptErrorLog("checkWanDouJiaUser, encode the token = %s error", tokenIt->second.c_str());
		return;
	}
	
	// 构造http消息请求头部数据
	// GET /api/uid/check?uid=8139480&token=1234567890&appkey_id=100668669 HTTP/1.1\r\nHost: pay.wandoujia.com\r\n\r\n
	char urlParam[MaxNetBuffSize] = {0};
	unsigned int urlParamLen = snprintf(urlParam, MaxNetBuffSize - 1, "uid=%s&token=%s&appkey_id=%s", uidIt->second.c_str(), encodeData, getThirdPlatformCfg(ThirdPlatformType::WanDouJia)->appId);
	CHttpRequest httpRequest(HttpMsgType::GET);
	httpRequest.setParam(urlParam, urlParamLen);
	if (!m_msgHandler->doHttpConnect(ThirdPlatformType::WanDouJia, srcSrvId, httpRequest))
	{
		OptErrorLog("do wan dou jia http connect error");
	}
}

bool CThirdPlatformOpt::checkWanDouJiaUserReply(ConnectData* cd)
{
    // 解析豌豆荚平台接口数据
	// data : true or false
	const char* flag = strstr(cd->receiveData, RequestHeaderFlag);  // 是否是完整的http头部数据	
	com_protocol::ThirdPlatformUserCheckRsp thirdPlatformUserCheckRsp;
	(flag != NULL && strcmp("true", flag + RequestHeaderFlagLen) == 0) ? thirdPlatformUserCheckRsp.set_result(OptSuccess) : thirdPlatformUserCheckRsp.set_result(CheckWanDouJiaUserError);

	int rc = -1;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(thirdPlatformUserCheckRsp, "send check wan dou jia user result to client");
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, thirdPlatformUserCheckRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), cd->keyData.srvId, cd->keyData.protocolId);

	OptInfoLog("send check wan dou jia user reply message, rc = %d, result = %d", rc, thirdPlatformUserCheckRsp.result());
	
	return true;
}

// 校验小米平台账号合法性
void CThirdPlatformOpt::checkXiaoMiUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CheckXiaoMiUserReq checkXiaoMiUserReq;
	if (!m_msgHandler->parseMsgFromBuffer(checkXiaoMiUserReq, data, len, "check xiao mi user")) return;
	
	//	请求示例：
	//	http://mis.migc.xiaomi.com/api/biz/service/verifySession.do?appId=2882303761517239138&session=1nlfxuAGmZk9IR2L&uid=100010&signature=b560b14efb18ee2eb8f85e51c5f7c11f697abcfc
	//	http message : GET /api/biz/service/verifySession.do?appId=2882303761517239138&session=1nlfxuAGmZk9IR2L&uid=100010&signature=b560b14efb18ee2eb8f85e51c5f7c11f697abcfc HTTP/1.1
	
	// 构造http消息请求头部数据
	static const char* signatureFlag = "&signature=";
	static const unsigned int signatureFlagLen = strlen(signatureFlag);

	const ThirdPlatformConfig* xiaoMiCfg = getThirdPlatformCfg(ThirdPlatformType::XiaoMi);
	ConnectData* cd = m_msgHandler->getConnectData();
	memset(cd, 0, sizeof(ConnectData));
	cd->sendDataLen = snprintf(cd->sendData, MaxNetBuffSize - 1, "GET %s?", xiaoMiCfg->url);

	// 哈希1运算，消息认证码
	char signatureData[MaxRequestDataLen] = {0};
	unsigned int signatureDataLen = MaxRequestDataLen;
	unsigned int signatureLen = snprintf(cd->sendData + cd->sendDataLen, MaxNetBuffSize - cd->sendDataLen - 1, "appId=%s&session=%s&uid=%s%s",
										 xiaoMiCfg->appId, checkXiaoMiUserReq.session_id().c_str(), checkXiaoMiUserReq.u_id().c_str(), signatureFlag);
	const char* signatureResult = HMAC_SHA1(xiaoMiCfg->appKey, strlen(xiaoMiCfg->appKey), (const unsigned char*)(cd->sendData + cd->sendDataLen), signatureLen - signatureFlagLen,
	                                        (unsigned char*)signatureData, signatureDataLen);
	if (signatureResult == NULL)
	{
		m_msgHandler->putConnectData(cd);

		OptErrorLog("signature data error, appId = %s, appKey = %s, session = %s, uid = %s, data = %s",
		xiaoMiCfg->appId, xiaoMiCfg->appKey, checkXiaoMiUserReq.session_id().c_str(), checkXiaoMiUserReq.u_id().c_str(), (const char*)(cd->sendData + cd->sendDataLen));
		return;
	}
	
	cd->sendDataLen += signatureLen;
	cd->sendDataLen += b2str(signatureResult, signatureDataLen, cd->sendData + cd->sendDataLen, MaxNetBuffSize - cd->sendDataLen - 1, false);
	cd->sendDataLen += snprintf(cd->sendData + cd->sendDataLen, MaxNetBuffSize - cd->sendDataLen - 1, " HTTP/1.1\r\nHost: %s%s", xiaoMiCfg->host, RequestHeaderFlag);

	cd->timeOutSecs = HttpConnectTimeOut;
	strcpy(cd->keyData.key, m_msgHandler->getContext().userData);
	cd->keyData.flag = ThirdPlatformType::XiaoMi;
	cd->keyData.srvId = srcSrvId;
	cd->keyData.protocolId = CHECK_XIAOMI_USER_RSP;

    OptInfoLog("do xiao mi http connect, user = %s, xiao mi uid = %s, session = %s, http request data len = %d, content = %s",
		       cd->keyData.key, checkXiaoMiUserReq.u_id().c_str(), checkXiaoMiUserReq.session_id().c_str(), cd->sendDataLen, cd->sendData);
			   
	if (!m_msgHandler->doHttpConnect(xiaoMiCfg->ip, xiaoMiCfg->port, cd))
	{
		OptErrorLog("do xiao mi http connect error");
		m_msgHandler->putConnectData(cd);
		return;
	}
}

bool CThirdPlatformOpt::checkXiaoMiUserReply(ConnectData* cd)
{
	// 解析小米平台接口数据
	// data : {"errcode": 200}
	// errcode	必须	状态码 200验证正确 1515appId错误 1516uid错误 1520session错误 1525signature错误
    // errMsg	可选	错误信息
	ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value);

	com_protocol::CheckXiaoMiUserRsp checkXiaoMiUserRsp;
	checkXiaoMiUserRsp.set_result(CheckXiaoMiUserError);
	checkXiaoMiUserRsp.set_mcode(OptSuccess);
	
	const int SuccessXiaoMiCode = 200;
	ParamKey2Value::iterator it = paramKey2Value.find("errcode");
	if (it != paramKey2Value.end())
	{
		int mCode = atoi(it->second.c_str());
		if (mCode == SuccessXiaoMiCode) checkXiaoMiUserRsp.set_result(OptSuccess);
		checkXiaoMiUserRsp.set_mcode(mCode);
	}
	
	it = paramKey2Value.find("errMsg");
	if (it != paramKey2Value.end()) checkXiaoMiUserRsp.set_m_errmsg(it->second);
	
	int rc = -1;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(checkXiaoMiUserRsp, "send check xiao mi user result to client");
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, checkXiaoMiUserRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), cd->keyData.srvId, cd->keyData.protocolId);

	OptInfoLog("send check xiao mi user reply message, rc = %d, result = %d, mcode = %d", rc, checkXiaoMiUserRsp.result(), checkXiaoMiUserRsp.mcode());
	
	return true;
}



/*
void CThirdPlatformOpt::checkQiHu360User(const ParamKey2Value& key2value, unsigned int srcSrvId)
{
    ParamKey2Value::const_iterator access_token = key2value.find("access_token");
	if (access_token == key2value.end())
	{
		OptErrorLog("checkQiHu360User, can not find the access token");
		return;
	}
	
    //构造http消息请求头部数据
    //GET /user/me.json?access_token=12345678983b38aabcdef387453ac8133ac3263987654321&fields=id,name,avatar,sex,area HTTP/1.1\r\nHost: %s%s
    const ThirdPlatformConfig* qiHu360 = getThirdPlatformCfg(ThirdPlatformType::QiHu360);
    ConnectData* cd = m_msgHandler->getConnectData();
	memset(cd, 0, sizeof(ConnectData));
	cd->sendDataLen = snprintf(cd->sendData, MaxNetBuffSize - 1, "GET %s?", qiHu360->url);
	cd->sendDataLen += snprintf(cd->sendData + cd->sendDataLen, MaxNetBuffSize - cd->sendDataLen - 1, 
    "access_token=%s&fields=id,name,avatar,sex,area HTTP/1.1\r\nHost: %s%s",
	access_token->second.c_str(), qiHu360->host, RequestHeaderFlag);
    
    cd->timeOutSecs = HttpConnectTimeOut;
	strcpy(cd->keyData.key, m_msgHandler->getContext().userData);
	cd->keyData.flag = ThirdPlatformType::QiHu360;
	cd->keyData.srvId = srcSrvId;
	cd->keyData.protocolId = CHECK_THIRD_USER_RSP;
    
    OptInfoLog("do qi hu 360 https connect, user = %s, token = %s, http request data len = %d, content = %s",
		       cd->keyData.key, access_token->second.c_str(), cd->sendDataLen, cd->sendData);

	if (!m_msgHandler->doHttpConnect(qiHu360->ip, qiHu360->port, cd))
	{
		OptErrorLog("do qi hu 360 http connect error");
		m_msgHandler->putConnectData(cd);
		return;
	}
}

bool CThirdPlatformOpt::checkQiHu360UserReply(ConnectData* cd)
{
    // 解析奇虎360平台接口数据
    //HTTP/1.1 200 OK
    //Content-Type: application/json
    //Cache-Control: no-store
    //{
    //   "id": "201459001",
    //    "name": "360U201459001",
    //    "avatar": "http://u1.qhimg.com/qhimg/quc/...ed6e9c53543903b",
    //    "sex": "未知"
    //    "area": ""
    //}
	
	ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value);

	com_protocol::ThirdPlatformUserCheckRsp checkQiHu360UserRsp;
	checkQiHu360UserRsp.set_result(0);  
    checkQiHu360UserRsp.set_third_code(0);  
   
    //检测失败( {"error_code":"4010101","error":"oauth_token不可用（OAuth1.0a）"} )
    ParamKey2Value::iterator it = paramKey2Value.find("error_code");
    if (it != paramKey2Value.end())
    {
        checkQiHu360UserRsp.set_result(1);
        checkQiHu360UserRsp.set_third_code(atoi(it->second.c_str()));
        
        //错误信息
        it = paramKey2Value.find("error");
        if(it != paramKey2Value.end())
        {
          checkQiHu360UserRsp.set_third_msg(it->second.c_str());
        }     
        
        int rc = -1;
        const char* msgData = m_msgHandler->serializeMsgToBuffer(checkQiHu360UserRsp, "send check qi hu 360 user result to client");
        if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, checkQiHu360UserRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), cd->keyData.srvId, cd->keyData.protocolId);

        OptInfoLog("send check qihu 360 user reply message, error code:%d, error msg:%s",
                    checkQiHu360UserRsp.third_code(), checkQiHu360UserRsp.third_msg().c_str());
        return true;
    }
    
    ParamKey2Value::iterator idIt = paramKey2Value.find("id");
    if (idIt != paramKey2Value.end())
    {
        com_protocol::ThirdCheckKeyValue* value = checkQiHu360UserRsp.add_key_value();
        value->set_key("id");
        value->set_value(idIt->second.c_str());
    }
    
    ParamKey2Value::iterator namtIt = paramKey2Value.find("name");
    if (namtIt != paramKey2Value.end())
    {
        com_protocol::ThirdCheckKeyValue* value = checkQiHu360UserRsp.add_key_value();
        value->set_key("name");
        value->set_value(namtIt->second.c_str());
    }
    
    int rc = -1;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(checkQiHu360UserRsp, "send check qi hu 360 user result to client");
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, checkQiHu360UserRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), cd->keyData.srvId, cd->keyData.protocolId);

	OptInfoLog("send check qihu 360 user reply message, id:%s, name:%s", idIt->second.c_str(), namtIt->second.c_str());
					
	return true;
}

bool CThirdPlatformOpt::checkSoGouUserReply(ConnectData* cd)
{
	// {
	//   result: true //有效session key  正常返回
	// }

	// {
	//   result: false //无效session key
	// }
	// {
	//   error: {
	//     code: -1,
	//     msg: "Internal server error"
	//   }
	// }

	ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value);

	com_protocol::ThirdPlatformUserCheckRsp checkSoGouUserRsp;
	checkSoGouUserRsp.set_result(0);  
    checkSoGouUserRsp.set_third_code(0);

    ParamKey2Value::iterator errIt = paramKey2Value.find("error");
    ParamKey2Value::iterator resultIt = paramKey2Value.find("result");
    if (errIt != paramKey2Value.end()||strcmp("false", resultIt->second.c_str()) == 0)
    {
        checkSoGouUserRsp.set_result(1);          
                
    }

    int rc = -1;
        const char* msgData = m_msgHandler->serializeMsgToBuffer(checkSoGouUserRsp, "send check sogou user result to client");
        if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, checkSoGouUserRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), cd->keyData.srvId, cd->keyData.protocolId);

        OptInfoLog("send check sogou user reply message, rc = %d, result = %d", rc, checkSoGouUserRsp.result());
        return true;
}

void CThirdPlatformOpt::checkSoGouUser(const ParamKey2Value& key2value, unsigned int srcSrvId)
{
	//POST gid=62&session_key=a1e912a708b9f9a669eca53a4b1180822d8fee58e01d63552b0178e3da84b614&user_id=8411626&auth=31474948ac1926d155bc837885d6a296
	ParamKey2Value::const_iterator sessionkeyIt = key2value.find("session_key");
	if (sessionkeyIt == key2value.end())
	{
		OptErrorLog("checkSoGouUser, can not find the session_key");
		return;
	}

	ParamKey2Value::const_iterator useridIt = key2value.find("user_id");
    
     const ThirdPlatformConfig* soGou = getThirdPlatformCfg(ThirdPlatformType::SoGou);
     
    //签名    md5 (gid=1&session_key=1&user_id=1&appsecret);
    char auth[MaxNetBuffSize] = {0};
    unsigned int authLen = snprintf(auth, MaxNetBuffSize - 1, "gid=%s&session_key=%s&user_id=%s&%s", soGou->appId, sessionkeyIt->second.c_str(), useridIt->second.c_str(), soGou->paymentKey);
    
    char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(auth, authLen, md5Buff);
        
    // 构造http消息请求头部数据
	char urlParam[MaxNetBuffSize] = {0};
	unsigned int urlParamLen = snprintf(urlParam, MaxNetBuffSize - 1, "gid=%s&session_key=%s&user_id=%s&auth=%s", 
                                        soGou->appId, sessionkeyIt->second.c_str(), useridIt->second.c_str(), md5Buff);
                                        
	CHttpRequest httpRequest(HttpMsgType::POST);
	httpRequest.setParam(urlParam, urlParamLen);
	if (!m_msgHandler->doHttpConnect(ThirdPlatformType::SoGou, srcSrvId, httpRequest))
	{
		OptErrorLog("do sogou http connect error");
	}
}

bool CThirdPlatformOpt::checkTXQQUserReply(ConnectData* cd)
{
	// 解析应用宝平台接口数据
	//{"ret":0,"msg":"user is logged in"}
    
  	ParamKey2Value paramKey2Value;
   	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value);

	ParamKey2Value::iterator retIt = paramKey2Value.find("ret");
    if(retIt == paramKey2Value.end())
    {
        OptErrorLog("checkTXQQUserReply,  can not find the ret");
        return false;
    }
        
    
    ParamKey2Value::iterator msgIt = paramKey2Value.find("msg"); 
    if(msgIt == paramKey2Value.end())
    {
        OptErrorLog("checkTXQQUserReply,  can not find the msg");
        return false;
    }

	com_protocol::ThirdPlatformUserCheckRsp checkTXQQUserRsp; 
    if (atoi(retIt->second.c_str()) == 0)
    {    	
    	checkTXQQUserRsp.set_result(0);  
    	checkTXQQUserRsp.set_third_msg(msgIt->second.c_str()); 

    	OptInfoLog("checkTXQQUserReply success! ret = %d, msg = %s", atoi(retIt->second.c_str()), msgIt->second.c_str());
    }
    else
    {
		checkTXQQUserRsp.set_result(1);  
    	checkTXQQUserRsp.set_third_msg(msgIt->second.c_str()); 

    	OptInfoLog("checkTXQQUserReply failed! ret = %d, msg = %s", atoi(retIt->second.c_str()), msgIt->second.c_str());
    }

    int rc = -1;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(checkTXQQUserRsp, "send check TXQQ user result to client");
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, checkTXQQUserRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), cd->keyData.srvId, cd->keyData.protocolId);

	OptInfoLog("send check TXQQ user reply message, rc = %d, result = %d", rc, checkTXQQUserRsp.result());
	
	return true;
	
}

void CThirdPlatformOpt::checkTXQQUser(const ParamKey2Value& key2value, unsigned int srcSrvId)
{
	// 构造http消息请求头部数据
	/POST 
	/auth/verify_login/?timestamp=*&appid=**&sig=***&openid=**&encode=1 
	//	HTTP/1.0
	//	Host:$domain
	//	Content-Type: application/x-www-form-urlencoded
	//	Content-Length: 198
	//	{
	//	    "appid": 100703379,
	//	    "openid": "A3284A812ECA15269F85AE1C2D94EB37",
	//	    "openkey": "933FE8C9AB9C585D7EABD04373B7155F",
	//	    "userip": "192.168.1.32"
	//	}

	//sig 加密规则：appKey.ts
	//md5 32位小写,例如”111111”的md5是” 96e79218965eb72c92a549dd5a330112”;

	const ThirdPlatformConfig* TXQQ = getThirdPlatformCfg(ThirdPlatformType::TXQQ);

	ParamKey2Value::const_iterator openID = key2value.find("openid");
	if (openID == key2value.end())
	{
		OptErrorLog("chechTXQQUser, can not find the openid");
		return;
	}

	ParamKey2Value::const_iterator openKey = key2value.find("openkey");
	if (openKey == key2value.end())
	{
		OptErrorLog("chechTXQQUser, can not find the openkey");
	}
    
    ParamKey2Value::const_iterator useripIt = key2value.find("ip");
	if (useripIt == key2value.end())
	{
		OptErrorLog("chechTXQQUser, can not find the ip");
	}
    
	char infoBuff[MaxNetBuffSize] = {0};
	unsigned int infoLen = snprintf(infoBuff,MaxNetBuffSize-1,"{\"appid\":%s,\"openid\":\"%s\",\"openkey\":\"%s\",\"userip\":\"%s\"}",TXQQ->appId,openID->second.c_str(),openKey->second.c_str(), useripIt->second.c_str());

	OptInfoLog("checkTXQQUser,infoLen = %u", infoLen);

	unsigned int ts = time(NULL);
	// MD5加密
	char signData[MaxNetBuffSize] = {0};
	char md5Buff[Md5BuffSize] = {0};
	unsigned int md5Len = snprintf(signData, MaxNetBuffSize - 1, "%s%u", TXQQ->appKey, ts);
	md5Encrypt(signData, md5Len, md5Buff);

	// 构造http消息请求头部数据
	char urlParam[MaxNetBuffSize] = {0};
	unsigned int urlParamLen = snprintf(urlParam, MaxNetBuffSize - 1, "timestamp=%u&appid=%s&sig=%s&openid=%s&encode=1", 
                                        ts, TXQQ->appId, md5Buff, openID->second.c_str());
                                        
	CHttpRequest httpRequest(HttpMsgType::POST);
	httpRequest.setParam(urlParam, urlParamLen);
	httpRequest.setHeaderKeyValue("Content-Type","application/x-www-form-urlencoded");
	httpRequest.setContent(infoBuff);

	if (!m_msgHandler->doHttpConnect(ThirdPlatformType::TXQQ, srcSrvId, httpRequest))
	{
		OptErrorLog("do TX http connect error");
		return;
	}
}


// 获取有玩平台订单ID
bool CThirdPlatformOpt::getYouWanTransactionId(unsigned int osType, char* transactionId, unsigned int len)
{
	// 有玩平台对订单号做了最大长度为20的限制，订单号除了平台ID，时间之外，只剩下4个字符的空间了
	const unsigned int MaxYouWanTransactionLen = 20;
	const unsigned int MaxYouWanTransactionIdx = 9999;
	if (len < MaxYouWanTransactionLen) return false;

	if (++m_transactionIndex[ThirdPlatformType::YouWan] > MaxYouWanTransactionIdx) m_transactionIndex[ThirdPlatformType::YouWan] = 1;
	
	unsigned int platformType = ThirdPlatformType::YouWan;
	if (osType == ClientVersionType::AppleMerge)  // 苹果版本
	{
		platformType = platformType * ThirdPlatformType::ClientDeviceBase + platformType;  // 转换为苹果版本值
	}
	else if (osType != ClientVersionType::AndroidMerge)  // 非安卓合并版本
	{
		OptErrorLog("getYouWanTransactionId, invalid os type = %d", osType);
		return false;
	}

	// 订单号格式 : platform.time.index.additional
	// 3.20150621153831.123
	unsigned int idLen = snprintf(transactionId, len - 1, "%u.%u.%u", platformType, (unsigned int)time(NULL), m_transactionIndex[ThirdPlatformType::YouWan]);
	if (idLen < MaxYouWanTransactionLen) strncat(transactionId, "000000000000000000000", MaxYouWanTransactionLen - idLen);

	return true;
}

// 有玩平台充值消息处理
bool CThirdPlatformOpt::handleYouWanPaymentMsg(Connect* conn, const HttpMsgBody& msgBody)
{
	bool isOK = handleYouWanPaymentMsgImpl(msgBody);
	CHttpResponse rsp;
	unsigned int msgLen = 0;
	isOK ? rsp.setContent("success") : rsp.setContent("failure");
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to you wan http, rc = %d, data = %s", rc, rspMsg);
	
	return isOK;
}

// 有玩平台充值消息处理
bool CThirdPlatformOpt::handleYouWanPaymentMsgImpl(const HttpMsgBody& msgBody)
{
    // 汉字编码格式问题，时间来不及处理了，先这么干着，后面有时间在修改编码问题
	static unsigned int GoodsNameIndex = 0;
	typedef unordered_map<string, string> UnicodeToChineseCharacter;
	static UnicodeToChineseCharacter unicode2CChar;
	if (GoodsNameIndex == 0)
	{
		// 初始化数据
		GoodsNameIndex = 5;
		const char* unicode[] = {"\\u91d1\\u5e01", "\\u9c9c\\u82b1", "\\u5c0f\\u5587\\u53ed", "\\u54d1\\u5f39", "\\u62d6\\u978b", "\\u81ea\\u52a8\\u70ae", "\\u9501\\u5b9a\\u70ae"};
	    const char* chineseCharacter[] = {"金币", "鲜花", "小喇叭", "哑弹", "拖鞋", "自动炮", "锁定炮"};
		for (unsigned int i = 0; i < sizeof(unicode) / sizeof(unicode[0]); ++i)
		{
			unicode2CChar[unicode[i]] = chineseCharacter[i];  // unicode对应的汉字
		}
	}
	
    ParamKey2Value::const_iterator keyIt = msgBody.paramKey2Value.find("key");
	if (keyIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleYouWanPaymentMsg, can not find the check data key");
		return false;
	}
	
	string keyForMD5;
	const char* checkInfo[] = {"status", "trade_no", "uid", "app_id", "app_trade_no", "goods_name", "goods_price",};
	for (unsigned int i = 0; i < sizeof(checkInfo) / sizeof(checkInfo[0]); ++i)
	{
		ParamKey2Value::const_iterator it = msgBody.paramKey2Value.find(checkInfo[i]);
		if (it == msgBody.paramKey2Value.end())
		{
			OptErrorLog("handleYouWanPaymentMsg, can not find the check data = %s", checkInfo[i]);
	        return false;
		}
        
		// 汉字编码格式问题，时间来不及处理了，先这么干着，后面有时间在修改编码问题
        if (i == GoodsNameIndex) keyForMD5 += unicode2CChar[it->second.c_str()];
		else keyForMD5 += it->second.c_str();
	}
	
	ThirdTranscationIdToInfo::iterator thirdInfoIt = m_thirdTransaction2Info.find(msgBody.paramKey2Value.at("app_trade_no"));
	if (thirdInfoIt == m_thirdTransaction2Info.end())
	{
		OptErrorLog("handleYouWanPaymentMsg, can not find transaction id = %s", msgBody.paramKey2Value.at("app_trade_no").c_str());
		return false;
	}
	
	// md5(status+trade_no+uid+app_id+app_trade_no+goods_name+goods_price+app_secret);
	char md5Buff[Md5BuffSize] = {0};
	keyForMD5 += getThirdPlatformCfg(ThirdPlatformType::YouWan)->paymentKey;
	md5Encrypt(keyForMD5.c_str(), keyForMD5.length(), md5Buff);
	if (strcmp(md5Buff, keyIt->second.c_str()) != 0)
	{
		OptErrorLog("handleYouWanPaymentMsg, md5 check key error, check data = %s, key = %s, md5 = %s", keyForMD5.c_str(), keyIt->second.c_str(), md5Buff);
		return false;
	}
	
	// 充值订单信息记录
    // format = name|type|id|count|amount|money|rechargeNo|srcSrvId|reply|umid|result|order|time
	const ThirdTranscationInfo& thirdInfo = thirdInfoIt->second;
	const char* youwanNoticeTime = "0";
	ParamKey2Value::const_iterator noticeTime = msgBody.paramKey2Value.find("notice_time");
	if (noticeTime != msgBody.paramKey2Value.end()) youwanNoticeTime = noticeTime->second.c_str();
	ThirdRechargeLog(ThirdPlatformType::YouWan, "%s|%u|%u|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s", thirdInfo.user, thirdInfo.itemType, thirdInfo.itemId, thirdInfo.itemCount, thirdInfo.itemAmount,
	thirdInfo.money, thirdInfoIt->first.c_str(), thirdInfo.srcSrvId, msgBody.paramKey2Value.at("uid").c_str(), msgBody.paramKey2Value.at("status").c_str(), msgBody.paramKey2Value.at("trade_no").c_str(), youwanNoticeTime);
    
	// status	int	1	必选	支付结果0:成功  1：失败
	int result = strcmp(msgBody.paramKey2Value.at("status").c_str(), "0");
	provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::YouWan, msgBody.paramKey2Value.at("uid").c_str(), msgBody.paramKey2Value.at("trade_no").c_str());
	if (result != 0) removeTransactionData(thirdInfoIt);

	return true;
}


// 劲牛平台充值消息处理
bool CThirdPlatformOpt::handleJinNiuPaymentMsg(Connect* conn, const HttpMsgBody& msgBody)
{
	// {"resultCode":"0","resultMsg":"success"}
	bool isOK = handleJinNiuPaymentMsgImpl(msgBody);
	CHttpResponse rsp;
	unsigned int msgLen = 0;
	isOK ? rsp.setContent("{\"resultCode\":\"0\",\"resultMsg\":\"success\"}") : rsp.setContent("{\"resultCode\":\"1\",\"resultMsg\":\"failure\"}");
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to jin niu http, rc = %d, data = %s", rc, rspMsg);
	
	return isOK;
}

// 劲牛平台充值消息处理
bool CThirdPlatformOpt::handleJinNiuPaymentMsgImpl(const HttpMsgBody& msgBody)
{
	// MD5校验结果
    ParamKey2Value::const_iterator signIt = msgBody.paramKey2Value.find("sign");
	if (signIt == msgBody.paramKey2Value.end())
	{
		OptErrorLog("handleJinNiuPaymentMsg, can not find the check data sign");
		return false;
	}
	
	const char* checkInfo[] = {"paymentType", "serialNumber", "billFee", "result", "indexId", "chargeTime",};
	for (unsigned int i = 0; i < sizeof(checkInfo) / sizeof(checkInfo[0]); ++i)
	{
		ParamKey2Value::const_iterator it = msgBody.paramKey2Value.find(checkInfo[i]);
		if (it == msgBody.paramKey2Value.end())
		{
			OptErrorLog("handleJinNiuPaymentMsg, can not find the check data = %s", checkInfo[i]);
	        return false;
		}
	}
	
	ThirdTranscationIdToInfo::iterator thirdInfoIt = m_thirdTransaction2Info.find(msgBody.paramKey2Value.at("serialNumber"));
	if (thirdInfoIt == m_thirdTransaction2Info.end())
	{
		OptErrorLog("handleJinNiuPaymentMsg, can not find transaction id = %s", msgBody.paramKey2Value.at("serialNumber").c_str());
		return false;
	}

	// MD5校验数据格式，例子
	//sign =md5(参数1=参数值&参数2=参数值&参数n=参数值……+key)

	// key排序之后，数据做MD5校验
	std::vector<string> keyVector;
	for (ParamKey2Value::const_iterator it = msgBody.paramKey2Value.begin(); it != msgBody.paramKey2Value.end(); ++it)
	{
		if (strcmp("sign", it->first.c_str()) != 0) keyVector.push_back(it->first);
	}

	char signatureData[MaxNetBuffSize] = {0};
	unsigned int signatureDataLen = 0;
	std::sort(keyVector.begin(), keyVector.end());
	for (vector<string>::const_iterator it = keyVector.begin(); it != keyVector.end(); ++it)
	{
		const string& value = msgBody.paramKey2Value.at(*it);
		signatureDataLen += snprintf(signatureData + signatureDataLen, MaxNetBuffSize - signatureDataLen - 1, "%s=%s&", it->c_str(), value.c_str());
	}
	
	signatureDataLen += snprintf(signatureData + signatureDataLen - 1, MaxNetBuffSize - signatureDataLen - 1, "%s", getThirdPlatformCfg(ThirdPlatformType::JinNiu)->paymentKey);  // 附加上劲牛平台提供的MD5校验KEY
	char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(signatureData, signatureDataLen - 1, md5Buff);
	if (strcmp(md5Buff, signIt->second.c_str()) != 0)
	{
		OptErrorLog("handleJinNiuPaymentMsg, check md5 error, data = %s, sign = %s, result = %s", signatureData, signIt->second.c_str(), md5Buff);
		return false;
	}

	const ThirdTranscationInfo& thirdInfo = thirdInfoIt->second;
	char dataBuff[MaxNetBuffSize] = {0};
	snprintf(dataBuff, MaxNetBuffSize - 1, "%u", (unsigned int)(thirdInfo.money * 100));
	if (strcmp(dataBuff, msgBody.paramKey2Value.at("billFee").c_str()) != 0)
	{
		OptErrorLog("handleJinNiuPaymentMsg, the pay money invalid, pay = %s, record = %.2f", msgBody.paramKey2Value.at("billFee").c_str(), thirdInfo.money);
		return false;
	}

	// 充值订单信息记录
    // format = name|type|id|count|amount|money|rechargeNo|srcSrvId|reply|result|indexId|paymentType|chargeTime
	ThirdRechargeLog(ThirdPlatformType::JinNiu, "%s|%u|%u|%u|%u|%.2f|%s|%u|reply|%s|%s|%s|%s", thirdInfo.user, thirdInfo.itemType, thirdInfo.itemId, thirdInfo.itemCount, thirdInfo.itemAmount,
	thirdInfo.money, thirdInfoIt->first.c_str(), thirdInfo.srcSrvId, msgBody.paramKey2Value.at("result").c_str(),
	msgBody.paramKey2Value.at("indexId").c_str(), msgBody.paramKey2Value.at("paymentType").c_str(), msgBody.paramKey2Value.at("chargeTime").c_str());
    
	// result	计费结果	String	计费结果 0：成功 1：失败
	int result = strcmp(msgBody.paramKey2Value.at("result").c_str(), "0");
	snprintf(dataBuff, MaxNetBuffSize - 1, "%s-%s", msgBody.paramKey2Value.at("paymentType").c_str(), msgBody.paramKey2Value.at("billFee").c_str());
	provideRechargeItem(result, thirdInfoIt, ThirdPlatformType::JinNiu, dataBuff, msgBody.paramKey2Value.at("indexId").c_str());
	if (result != 0) removeTransactionData(thirdInfoIt);

	return true;
}

bool CThirdPlatformOpt::checkOppoUserReply(ConnectData* cd)
{
    //解析Oppo数据
	//    {"resultCode":"200","resultMsg":"正常","loginToken":xxx,"ssoid":123456,"appKey":null,
	//	 "userName":abc,"email":null,"mobileNumber":null,"createTime":null,"userStatus":null
	//	}
    
    int rc = -1;
    ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value);

	com_protocol::ThirdPlatformUserCheckRsp checkOppoRsp;
	checkOppoRsp.set_result(0);  
    checkOppoRsp.set_third_code(0);  
    
    //检测是否成功
    ParamKey2Value::iterator reCodeIt = paramKey2Value.find("resultCode");
    if (reCodeIt == paramKey2Value.end())
    {
        OptInfoLog("Error checkOppoUserReply Not found resultCode.");
        return false;
    }
    
    //失败
    if(strcmp(reCodeIt->second.c_str(), "200") != 0)
    {
        checkOppoRsp.set_result(1);
        
        int nErrorCode = atoi(reCodeIt->second.c_str());
        checkOppoRsp.set_third_code(nErrorCode);
        
        switch(nErrorCode)
        {
            case 1001:
            checkOppoRsp.set_third_msg("param error");
            break;
            
            case 1002:
            checkOppoRsp.set_third_msg("signerr error");
            break;
        }
             
        const char* msgData = m_msgHandler->serializeMsgToBuffer(checkOppoRsp, "send check oppo user result to client");
        if (msgData != NULL) 
        {
            rc = m_msgHandler->sendMessage(msgData, checkOppoRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key),
                                               cd->keyData.srvId, cd->keyData.protocolId);
        }
                                                            
        OptInfoLog("send check oppo user reply message, rc=%d, error_code=%s, error_msg=%s", rc, 
                    reCodeIt->second.c_str(), checkOppoRsp.third_msg().c_str());
        return true;
    }
   
   //用户名
    ParamKey2Value::iterator userNameIt = paramKey2Value.find("userName");
    if (userNameIt != paramKey2Value.end())
    {
        com_protocol::ThirdCheckKeyValue* value = checkOppoRsp.add_key_value();
        value->set_key("userName");
        value->set_value(userNameIt->second.c_str());
    }
     
    //用户ID 
    ParamKey2Value::iterator ssoidIt = paramKey2Value.find("ssoid");
    if (ssoidIt != paramKey2Value.end())
    {
        com_protocol::ThirdCheckKeyValue* value = checkOppoRsp.add_key_value();
        value->set_key("ssoid");
        value->set_value(ssoidIt->second.c_str());
    }
    
    const char* msgData = m_msgHandler->serializeMsgToBuffer(checkOppoRsp, "send check oppo user result to client");
	if (msgData != NULL) 
    {
        rc = m_msgHandler->sendMessage(msgData, checkOppoRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), 
                                           cd->keyData.srvId, cd->keyData.protocolId);
    }
    
    OptInfoLog("send check oppo user reply message, rc=%d, ssoid=%s, name=%s", rc, ssoidIt->second.c_str(), userNameIt->second.c_str());
    return true;
}

void CThirdPlatformOpt::checkOppoUser(const ParamKey2Value& key2value, unsigned int srcSrvId)
{
    ParamKey2Value::const_iterator tokenIt = key2value.find("token");
	if (tokenIt == key2value.end())
	{
		OptErrorLog("checkOppoUser, can not find the token");
		return;
	}
    char token[MaxNetBuffSize] = {0};
    unsigned int tokenDataLen = MaxNetBuffSize - 1;
    urlEncode(tokenIt->second.c_str(), tokenIt->second.size(), token, tokenDataLen);
    
    ParamKey2Value::const_iterator ssoidIt = key2value.find("ssoid");
	if (ssoidIt == key2value.end())
	{
		OptErrorLog("checkOppoUser, can not find the ssoid");
		return;
	}
    char ssoid[MaxNetBuffSize] = {0};
    unsigned int ssoidDataLen = MaxNetBuffSize - 1;
    urlEncode(ssoidIt->second.c_str(), ssoidIt->second.size(), ssoid, ssoidDataLen);
    	
    //构造http消息请求头部数据
    const ThirdPlatformConfig* oppo = getThirdPlatformCfg(ThirdPlatformType::Oppo);
    char param[MaxNetBuffSize] = {0};
    unsigned int paramDataLen = 0;
    paramDataLen = snprintf(param, MaxNetBuffSize - 1, "oauthConsumerKey=%s&oauthToken=%s&oauthSignatureMethod=HMAC-SHA1&oauthTimestamp=%u&oauthNonce=%d&oauthVersion=1.0&",
                            oppo->appId, token, (unsigned int)time(NULL), getRandNumber(0, 255));
        
    //oauthSignature:  appSecret + & 为Key, 对param进行HMAC-SHA1加密
    char sigKey[MaxNetBuffSize] = {0};
    unsigned int sigKeyDataLen = 0;
    sigKeyDataLen = snprintf(sigKey, MaxNetBuffSize - 1, "%s&", oppo->appKey);    
    
    OptInfoLog("sigKey=%s", sigKey);
    
    char oauthSignature[MaxNetBuffSize] = {0};
    unsigned int oauthSignatureDataLen = MaxNetBuffSize - 1;
	
    //HMAC_SHA1
    HMAC_SHA1(sigKey, sigKeyDataLen, (const unsigned char*)param, paramDataLen, 
             (unsigned char*)oauthSignature, oauthSignatureDataLen);
        
    //base64     
    char buffer[MaxNetBuffSize] = {0};
    unsigned int bufferLen = MaxNetBuffSize - 1;
    if(!base64Encode(oauthSignature, oauthSignatureDataLen, buffer, bufferLen))
    {
        OptErrorLog("checkOppoUser base64 Error!!");
        return;
    }    
    
    //URL编码
    memset(oauthSignature, 0, MaxNetBuffSize * sizeof(char));
    oauthSignatureDataLen = MaxNetBuffSize - 1;
    if(!urlEncode(buffer, bufferLen, oauthSignature, oauthSignatureDataLen))
    {
        OptErrorLog("checkOppoUser urlEncode Error!!");
        return;
    }
   
    ConnectData* cd = m_msgHandler->getConnectData();
	memset(cd, 0, sizeof(ConnectData));
	cd->sendDataLen = snprintf(cd->sendData, MaxNetBuffSize - 1, "GET %s?", oppo->url);
    cd->sendDataLen += snprintf(cd->sendData + cd->sendDataLen, MaxNetBuffSize - cd->sendDataLen - 1,
    "fileId=%s&token=%s HTTP/1.1\r\nHost: %s\r\nparam:%s\r\noauthSignature:%s%s", ssoid, token, oppo->host, 
    param, oauthSignature, RequestHeaderFlag);
   
    cd->timeOutSecs = HttpConnectTimeOut;
	strcpy(cd->keyData.key, m_msgHandler->getContext().userData);
	cd->keyData.flag = ThirdPlatformType::Oppo;
	cd->keyData.srvId = srcSrvId;
	cd->keyData.protocolId = CHECK_THIRD_USER_RSP;
    
    OptInfoLog("do oppo https connect, user = %s, token = %s, ssoid = %s, http request data len = %d, content = %s",
		       cd->keyData.key, token, ssoid, cd->sendDataLen, cd->sendData);

	if (!m_msgHandler->doHttpConnect(oppo->ip, oppo->port, cd))
	{
		OptErrorLog("do oppo http connect error");
		m_msgHandler->putConnectData(cd);
		return;
	}
}

bool CThirdPlatformOpt::checkMuMaYiUserReply(ConnectData* cd)
{
    int rc = -1;
    const char *pHeadEnd = strstr(cd->receiveData, RequestHeaderFlag);
    if(pHeadEnd == NULL)
    {
        OptErrorLog("checkMuMaYiUserReply can not find the RequestHeaderFlag.");
        return false;
    }
    
    com_protocol::ThirdPlatformUserCheckRsp checkMuMaYiRsp;
	checkMuMaYiRsp.set_result(1);               //默认失败
     checkMuMaYiRsp.set_third_code(0); 
     
    if(strstr(pHeadEnd, "success") != NULL)
    {
        checkMuMaYiRsp.set_result(0);  
    }
    
    const char* msgData = m_msgHandler->serializeMsgToBuffer(checkMuMaYiRsp, "send check mu ma yi user result to client");
	if (msgData != NULL) 
    {
        rc = m_msgHandler->sendMessage(msgData, checkMuMaYiRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), 
                                       cd->keyData.srvId, cd->keyData.protocolId);
    }
    
    OptInfoLog("send check mu ma yi user reply message, rc=%d, data=%s", rc, cd->receiveData);
    return true;
}

void CThirdPlatformOpt::checkMuMaYiUser(const ParamKey2Value& key2value, unsigned int srcSrvId)
{
    ParamKey2Value::const_iterator tokenIt = key2value.find("token");
	if (tokenIt == key2value.end())
	{
		OptErrorLog("checkMuMaYiUser, can not find the token");
		return;
	}
     
    ParamKey2Value::const_iterator uidIt = key2value.find("uid");
	if (uidIt == key2value.end())
	{
		OptErrorLog("checkMuMaYiUser, can not find the uid");
		return;
	}
    
    //POST 请求
    const ThirdPlatformConfig* muMaYi = getThirdPlatformCfg(ThirdPlatformType::MuMaYi);
    ConnectData* cd = m_msgHandler->getConnectData();
	memset(cd, 0, sizeof(ConnectData));
	cd->sendDataLen = snprintf(cd->sendData, MaxNetBuffSize - 1, "POST %s?", muMaYi->url);
	cd->sendDataLen += snprintf(cd->sendData + cd->sendDataLen, MaxNetBuffSize - cd->sendDataLen - 1, 
    "token=%s&uid=%s HTTP/1.1\r\nHost: %s%s", tokenIt->second.c_str(), uidIt->second.c_str(), muMaYi->host, RequestHeaderFlag);
    
    cd->timeOutSecs = HttpConnectTimeOut;
	strcpy(cd->keyData.key, m_msgHandler->getContext().userData);
	cd->keyData.flag = ThirdPlatformType::MuMaYi;
	cd->keyData.srvId = srcSrvId;
	cd->keyData.protocolId = CHECK_THIRD_USER_RSP;
    
    OptInfoLog("do mu ma yi https connect, user = %s, uid = %s, token = %s, http request data len = %d, content = %s",
		       cd->keyData.key, uidIt->second.c_str(), tokenIt->second.c_str(), cd->sendDataLen, cd->sendData);

	if (!m_msgHandler->doHttpConnect(muMaYi->ip, muMaYi->port, cd))
	{
		OptErrorLog("do mu ma yi http connect error");
		m_msgHandler->putConnectData(cd);
		return;
	}
}


bool CThirdPlatformOpt::checkTXWXUserReply(ConnectData* cd)
{
	// 解析应用宝平台接口数据
	//{"ret":0,"msg":"ok"}
    
  	ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value);

	ParamKey2Value::iterator retIt = paramKey2Value.find("ret");
    ParamKey2Value::iterator msgIt = paramKey2Value.find("msg");   

	com_protocol::ThirdPlatformUserCheckRsp checkTXWXUserRsp; 

    if (atoi(retIt->second.c_str()) == 0)
    {    	
    	checkTXWXUserRsp.set_result(0);  
    	checkTXWXUserRsp.set_third_msg(msgIt->second.c_str()); 

    	OptInfoLog("checkTXWXUserReply success! ret = %d, msg = %s", atoi(retIt->second.c_str()), msgIt->second.c_str());
    }
    else
    {
		checkTXWXUserRsp.set_result(1);  
    	checkTXWXUserRsp.set_third_msg(msgIt->second.c_str()); 

    	OptInfoLog("checkTXWXUserReply failed! ret = %d, msg = %s", atoi(retIt->second.c_str()), msgIt->second.c_str());
    }

    int rc = -1;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(checkTXWXUserRsp, "send check TXWX user result to client");
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, checkTXWXUserRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), cd->keyData.srvId, cd->keyData.protocolId);

	OptInfoLog("send check TXWX user reply message, rc = %d, result = %d", rc, checkTXWXUserRsp.result());
	
	return true;
}

void CThirdPlatformOpt::checkTXWXUser(const ParamKey2Value& key2value, unsigned int srcSrvId)
{
	// 构造http消息请求头部数据
	//	POST 
	//	/auth/check_token/?timestamp=*&appid=**&sig=***&openid=**&encode=1 
	//	HTTP/1.0
	//	Host:$domain
	//	Content-Type: application/x-www-form-urlencoded
	//	Content-Length: 198
	//	{
	//	    "accessToken": "OezXcEiiBSKSxW0eoylIeLl3C6OgXeyrDnhDI73sCBJPLafWudG-idTVMbKesBkhBO_ZxFWN4zlXCpCHpYcrXNG6Vs-cocorhdT5Czj_23QF6D1qH8MCldg0BSMdEUnsaWcFH083zgWJcl_goeBUSQ",
	//    	"openid": "oGRTijiaT-XrbyXKozckdNHFgPyc"
	//	}

	//sig 加密规则：appKey.ts
	//md5 32位小写,例如”111111”的md5是” 96e79218965eb72c92a549dd5a330112”;

	const ThirdPlatformConfig* TXWX = getThirdPlatformCfg(ThirdPlatformType::TXWX);

	ParamKey2Value::const_iterator openID = key2value.find("openid");
	if (openID == key2value.end())
	{
		OptErrorLog("chechTXWXUser, can not find the openid");
		return;
	}

	ParamKey2Value::const_iterator openKey = key2value.find("openkey");
	if (openKey == key2value.end())
	{
		OptErrorLog("chechTXWXUser, can not find the openkey");
	}

	char infoBuff[MaxNetBuffSize] = {0};
	unsigned int infoLen = snprintf(infoBuff,MaxNetBuffSize-1,"{\"accessToken\":\"%s\",\"openid\":\"%s\"}",openKey->second.c_str(),openID->second.c_str());

	OptInfoLog("checkTXWXUser,infoLen = %u", infoLen);

	unsigned int ts = time(NULL);
	// MD5加密
	char signData[MaxNetBuffSize] = {0};
	char md5Buff[Md5BuffSize] = {0};
	unsigned int md5Len = snprintf(signData, MaxNetBuffSize - 1, "%s%u", TXWX->appKey, ts);
	md5Encrypt(signData, md5Len, md5Buff);

	// 构造http消息请求头部数据
	char urlParam[MaxNetBuffSize] = {0};
	unsigned int urlParamLen = snprintf(urlParam, MaxNetBuffSize - 1, "timestamp=%u&appid=%s&sig=%s&openid=%s&encode=1", 
                                        ts, TXWX->appId, md5Buff, openID->second.c_str());
                                        
	CHttpRequest httpRequest(HttpMsgType::POST);
	httpRequest.setParam(urlParam, urlParamLen);
	httpRequest.setHeaderKeyValue("Content-Type","application/x-www-form-urlencoded");
	httpRequest.setContent(infoBuff);

	if (!m_msgHandler->doHttpConnect(ThirdPlatformType::TXWX, srcSrvId, httpRequest))
	{
		OptErrorLog("do TX http connect error");
		return;
	}
}

bool CThirdPlatformOpt::checkAnZhiUserReply(ConnectData* cd)
{
    ParamKey2Value paramKey2Value;
	CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '\'');

	com_protocol::ThirdPlatformUserCheckRsp checkAnZhiRsp;
	checkAnZhiRsp.set_result(0);  
    checkAnZhiRsp.set_third_code(0);  
       
    ParamKey2Value::iterator scIt = paramKey2Value.find("sc");
    if (scIt == paramKey2Value.end())
    {
        OptErrorLog("CThirdPlatformOpt, can not find the sc");
        return false;
    }
    
    ParamKey2Value::iterator stIt = paramKey2Value.find("st");
    if (stIt == paramKey2Value.end())
    {
        OptErrorLog("CThirdPlatformOpt, can not find the st");
        return false;
    }
    
     //检测是否成功
    if(atoi(scIt->second.c_str()) != 1)
    {
        checkAnZhiRsp.set_result(1);
        checkAnZhiRsp.set_third_msg(stIt->second.c_str());
    }
      
    int rc = -1;  
    const char* msgData = m_msgHandler->serializeMsgToBuffer(checkAnZhiRsp, "send check an zhi user result to client");
	if (msgData != NULL) 
    {
        rc = m_msgHandler->sendMessage(msgData, checkAnZhiRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), 
                                       cd->keyData.srvId, cd->keyData.protocolId);
    }
    
    OptInfoLog("send check an zhi user reply message, rc=%d, sc=%s, data=%s", rc, scIt->second.c_str(), cd->receiveData);
    return true;
}

void CThirdPlatformOpt::checkAnZhiUser(const ParamKey2Value& key2value, unsigned int srcSrvId)
{
    ParamKey2Value::const_iterator sidIt = key2value.find("sid");
	if (sidIt == key2value.end())
	{
		OptErrorLog("checkAnZhiUser, can not find the sid");
		return;
	}
    
     const ThirdPlatformConfig* anZhi = getThirdPlatformCfg(ThirdPlatformType::AnZhi);
     
    //签名    Base64.encodeToString (appkey+sid+appsecret);
    char sign[MaxNetBuffSize] = {0};
    unsigned int signLen = snprintf(sign, MaxNetBuffSize - 1, "%s%s%s", anZhi->appKey, sidIt->second.c_str(), anZhi->appId);
    
    char base64Data[MaxNetBuffSize] = {0};
    unsigned int base64DataLen = MaxNetBuffSize - 1;
    if(!base64Encode(sign, signLen, base64Data, base64DataLen))
    {
        OptErrorLog("checkAnZhiUser, Base64 Encode Error!");
        return ;
    }
        
    // 构造http消息请求头部数据
	char urlParam[MaxNetBuffSize] = {0};
	unsigned int urlParamLen = snprintf(urlParam, MaxNetBuffSize - 1, "time=%u&appkey=%s&sid=%s&sign=%s", 
                                        (unsigned int)time(NULL), anZhi->appKey, sidIt->second.c_str(), base64Data);
                                        
	CHttpRequest httpRequest(HttpMsgType::POST);
	httpRequest.setParam(urlParam, urlParamLen);
	if (!m_msgHandler->doHttpConnect(ThirdPlatformType::AnZhi, srcSrvId, httpRequest))
	{
		OptErrorLog("do an zhi http connect error");
	}
}

bool CThirdPlatformOpt::handleTXPaymentMsg(Connect* conn, const HttpMsgBody& msgBody)
{
	bool isOK = handleTXPaymentMsgImpl(msgBody);
	CHttpResponse rsp;
	isOK ? rsp.setContent("{\"ret\":0,\"msg\":\"OK\"}") : rsp.setContent("{\"ret\":1,\"msg\":\"系统繁忙\"}");

	unsigned int msgLen = 0;
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = m_msgHandler->sendDataToClient(conn, rspMsg, msgLen);
	OptInfoLog("send reply message to TX http, rc = %d, data = %s", rc, rspMsg);
	
	return isOK;
}

bool CThirdPlatformOpt::handleTXPaymentMsgImpl(const HttpMsgBody& msgBody)
{
	return true;
}
*/

}

