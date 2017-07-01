
/* author : limingfan
 * date : 2015.07.01
 * description : Http 服务简单实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/md5.h>

#include "CHttpSrv.h"
#include "CHttpData.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace NProject;


// 充值日志文件
#define DownJoyRechargeLog(format, args...)     getDownJoyRechargeLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)

namespace NPlatformService
{

CSrvMsgHandler::CSrvMsgHandler() : m_memForUserData(MaxUserDataCount, MaxUserDataCount, sizeof(ConnectUserData)),
m_memForConnectData(MaxConnectDataCount, MaxConnectDataCount, sizeof(ConnectData))
{
    m_msgBuffer[0] = '\0';
	memset(&m_downJoyCfg, 0, sizeof(m_downJoyCfg));
	m_rechargeTransactionId = 0;
	
	memset(m_checkUserHandlers, 0, sizeof(m_checkUserHandlers));
	memset(m_userPayGetOptHandlers, 0, sizeof(m_userPayGetOptHandlers));
	memset(m_userPayPostOptHandlers, 0, sizeof(m_userPayPostOptHandlers));
	memset(m_checkUserRequests, 0, sizeof(m_checkUserRequests));
}

CSrvMsgHandler::~CSrvMsgHandler()
{
    m_memForUserData.clear();
	m_memForConnectData.clear();
	m_msgBuffer[0] = '\0';
	memset(&m_downJoyCfg, 0, sizeof(m_downJoyCfg));
	m_rechargeTransactionId = 0;
	
	memset(m_checkUserHandlers, 0, sizeof(m_checkUserHandlers));
	memset(m_userPayGetOptHandlers, 0, sizeof(m_userPayGetOptHandlers));
	memset(m_userPayPostOptHandlers, 0, sizeof(m_userPayPostOptHandlers));
	memset(m_checkUserRequests, 0, sizeof(m_checkUserRequests));
}


bool CSrvMsgHandler::serializeMsgToBuffer(::google::protobuf::Message& msg, char* buffer, const unsigned int len, const char* info)
{
	if (!msg.SerializeToArray(buffer, len))
	{
		OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), len, info);
		return false;
	}
	return true;
}

const char* CSrvMsgHandler::serializeMsgToBuffer(::google::protobuf::Message& msg, const char* info)
{
	if (!msg.SerializeToArray(m_msgBuffer, NFrame::MaxMsgLen))
	{
		OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), NFrame::MaxMsgLen, info);
		return NULL;
	}
	return m_msgBuffer;
}

bool CSrvMsgHandler::parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info)
{
	if (!msg.ParseFromArray(buffer, len))
	{
		OptErrorLog("ParseFromArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), len, info);
		return false;
	}
	return true;
}

int CSrvMsgHandler::sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userName, int uNLen, int userFlag)
{
	// 要根据负载均衡选择commonDBproxy服务
	unsigned int srvId = 0;
	getDestServiceID(DBProxyCommonSrv, userName, uNLen, srvId);
	return sendMessage(data, len, userFlag, userName, uNLen, srvId, protocolId);
}

int CSrvMsgHandler::sendMessageToService(NFrame::ServiceType srvType, const ::google::protobuf::Message& msg, unsigned short protocolId, const char* userName, const int uNLen,
                                         int userFlag, unsigned short handleProtocolId)
{
	if (!msg.SerializeToArray(m_msgBuffer, NFrame::MaxMsgLen))
	{
		OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, srvType = %d, protocolId = %d, userFlag = %d, handleProtocolId = %d",
		msg.ByteSize(), NFrame::MaxMsgLen, srvType, protocolId, userFlag, handleProtocolId);
		return SerializeToArrayError;
	}
	
	unsigned int srvId = 0;
	getDestServiceID(srvType, userName, uNLen, srvId);
	return sendMessage(m_msgBuffer, msg.ByteSize(), userFlag, userName, uNLen, srvId, protocolId, 0, handleProtocolId);
}

/*
CRedis& CSrvMsgHandler::getRedisService()
{
	return m_redisDbOpt;
}
*/

bool CSrvMsgHandler::getHostInfo(const char* host, strIP_t ip, unsigned short& port, const char* service)
{
	struct addrinfo hints;
	struct addrinfo* result = NULL;
	struct addrinfo* rp = NULL;
	
	/* Obtain address(es) matching host/port */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	int rc = getaddrinfo(host, service, &hints, &result);
	if (rc != 0)
	{
	   OptErrorLog("call getaddrinfo for http connect error, host = %s, service = %s, rc = %d, info = %s", host, service, rc, strerror(rc));
	   return false;
	}
	
	/* getaddrinfo() returns a list of address structures.
	Try each address until we successfully connect(2). If socket(2) (or connect(2)) fails, we (close the socket and) try the next address. */
	CSocket activeConnect;
	int errorCode = -1;
	struct sockaddr_in* peerAddr = NULL;
	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		peerAddr = (struct sockaddr_in*)rp->ai_addr;
		rc = activeConnect.init(CSocket::toIPStr(peerAddr->sin_addr), ntohs(peerAddr->sin_port));
		do
		{
			if (rc != Success) break;
			
			rc = activeConnect.open(SOCK_STREAM);   // tcp socket
			if (rc != Success) break;
			
			rc = activeConnect.setReuseAddr(1);
			if (rc != Success) break;
			
			rc = activeConnect.setNagle(1);
			if (rc != Success) break;
			
			rc = activeConnect.connect(errorCode);  // 尝试建立连接，阻塞式
			if (rc != Success) break;
		}
		while (0);
		
		OptInfoLog("connect %s host = %s, ip = %s, port = %d, result = %d", service, host, CSocket::toIPStr(peerAddr->sin_addr), ntohs(peerAddr->sin_port), rc);
		activeConnect.close();
		if (rc == Success)
		{
	        strcpy(ip, activeConnect.getIp());
			port = activeConnect.getPort();
	        break;
		}
	}
	activeConnect.unInit();
	
	freeaddrinfo(result);
	
	if (rp == NULL)
	{
	    OptErrorLog("connect %s error, host = %s", service, host);
		return false;
	}

    return true;
}

ConnectData* CSrvMsgHandler::getConnectData()
{
	return (ConnectData*)m_memForConnectData.get();
}

void CSrvMsgHandler::putConnectData(ConnectData* connData)
{
	if (connData->cbData != NULL && connData->cbData->flag == 0) putDataBuffer((const char*)connData->cbData);
	
	m_memForConnectData.put((char*)connData);
}

char* CSrvMsgHandler::getDataBuffer(unsigned int& len)
{
	len = sizeof(ConnectUserData);
	return m_memForUserData.get();
}

void CSrvMsgHandler::putDataBuffer(const char* dataBuff)
{
	if (dataBuff != NULL) m_memForUserData.put((char*)dataBuff);
}

bool CSrvMsgHandler::doHttpConnect(unsigned int platformType, unsigned int srcSrvId, const CHttpRequest& httpRequest, const char* url, unsigned int flag, const UserCbData* cbData, unsigned int len, const char* userData, unsigned short protocolId)
{
	const ThirdPlatformConfig* thirdCfg = m_thirdPlatformOpt.getThirdPlatformCfg(platformType);
	if (thirdCfg == NULL || srcSrvId < 1 || len >= sizeof(ConnectUserData))
	{
		OptErrorLog("to do http connect, the param is error, platform type = %u, srcSrvId = %u, cb data len = %u", platformType, srcSrvId, len);
		return false;
	}

    ConnectData* cd = getConnectData();
	memset(cd, 0, sizeof(ConnectData));
	
	cd->timeOutSecs = HttpConnectTimeOut;
	if (userData == NULL) userData = getContext().userData;
	strncpy(cd->keyData.key, userData, MaxKeySize - 1);
	cd->keyData.flag = (flag > HttpReplyHandlerID::IdBeginValue) ? flag : platformType;
	cd->keyData.srvId = srcSrvId;
	cd->keyData.protocolId = protocolId;
	
	cd->sendDataLen = MaxNetBuffSize;
	if (url == NULL) url = thirdCfg->url;
	const char* msgData = httpRequest.getMessage(cd->sendData, cd->sendDataLen, thirdCfg->host, url);
    if (msgData == NULL)
	{
		putConnectData(cd);
		OptErrorLog("get http request message error, platform type = %u", platformType);
		return false;
	}

    bool isOK = m_httpConnectMgr.doHttpConnect(thirdCfg->ip, thirdCfg->port, cd);
	OptInfoLog("do http connect, result = %d, user = %s, platform type = %u, flag = %u, message len = %u, content = %s", isOK, cd->keyData.key, platformType, flag, cd->sendDataLen, cd->sendData);
	
	if (isOK)
	{
		if (cbData != NULL && len > 0)
		{
			if (cbData->flag == 0)
			{
				unsigned int buffLen = 0;
				UserCbData* userCbData = (UserCbData*)getDataBuffer(buffLen);  // 可以直接使用这里内存管理的内存块
				memcpy(userCbData, cbData, len);
				cbData = userCbData;
			}
			cd->cbData = cbData;
		}
	}
	else
	{
		putConnectData(cd);
	}
	
	return isOK;
}

bool CSrvMsgHandler::doHttpConnect(unsigned int platformType, unsigned int srcSrvId, const CHttpRequest& httpRequest, unsigned int id, const char* url, const UserCbData* cbData, unsigned int len, const char* userData, unsigned short protocolId)
{
	return doHttpConnect(platformType, srcSrvId, httpRequest, url, id, cbData, len, userData, protocolId);
}

bool CSrvMsgHandler::doHttpConnect(const char* ip, const unsigned short port, ConnectData* connData)
{
	return m_httpConnectMgr.doHttpConnect(ip, port, connData);
}

// 新增充值赠送物品
void CSrvMsgHandler::getGiftInfo(const com_protocol::UserRechargeRsp& userRechargeRsp, google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>* giftInfo, char* logInfo, unsigned int logInfoLen)
{
	static const char* GiftName = "gift";
	static const unsigned int GiftNameLen = strlen(GiftName);
	
	logInfo[0] = '\0';
	if (userRechargeRsp.gift_array_size() > 0 && logInfoLen > GiftNameLen)
	{
		strcpy(logInfo, GiftName);
		unsigned int giftDataLen = GiftNameLen;
		for (int idx = 0; idx < userRechargeRsp.gift_array_size(); ++idx)
		{
			const com_protocol::GiftInfo& rGiftInfo = userRechargeRsp.gift_array(idx);
			com_protocol::GiftInfo* gift = giftInfo->Add();
			gift->set_num(rGiftInfo.num());
			gift->set_type(rGiftInfo.type());
			
			giftDataLen += snprintf(logInfo + giftDataLen, logInfoLen - giftDataLen - 1, "&%u=%u", rGiftInfo.type(), rGiftInfo.num());
		}
	}
}

/*
const ServiceLogicData& CSrvMsgHandler::getLogicData(const char* userName, unsigned int len)
{
	UserToLogicData::iterator it = m_user2LogicData.find(userName);
	if (it != m_user2LogicData.end()) return it->second;
	
	m_user2LogicData[userName] = ServiceLogicData();
	ServiceLogicData& srvData = m_user2LogicData[userName];
	srvData.isUpdate = 0;
	
	int gameDataLen = getRedisService().getHField(HttpLogicDataKey, HttpLogicDataKeyLen, userName, len, m_msgBuffer, MaxMsgLen);
	if (gameDataLen > 0)
	{
		if (!srvData.logicData.ParseFromArray(m_msgBuffer, gameDataLen))
		{
			// 出错了只能初始化清空数据重新再来
			OptErrorLog("getLogicData, ParseFromArray message error, user = %s, msg byte len = %d, buff len = %d", userName, srvData.logicData.ByteSize(), gameDataLen);
			srvData.logicData.Clear();
			srvData.isUpdate = 1;
		}
	}
	
	return srvData;
}

ServiceLogicData& CSrvMsgHandler::setLogicData(const char* userName, unsigned int len)
{
	ServiceLogicData& srvData = (ServiceLogicData&)getLogicData(userName, len);
	srvData.isUpdate = 1;  // update logic data
	return srvData;
}
*/


CMall& CSrvMsgHandler::getMallMgr()
{
	return m_mallMgr;
}

void CSrvMsgHandler::rechargeItemReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserRechargeRsp userRechargeRsp;
	if (!parseMsgFromBuffer(userRechargeRsp, data, len, "user recharge reply")) return;
	
	bool isOK = false;
	const com_protocol::UserRechargeReq& userRechargeInfo = userRechargeRsp.info();
	const unsigned int platformType = userRechargeInfo.third_type();
	
	if (platformType == ThirdPlatformType::GoogleSDK && userRechargeInfo.item_type() != EGameGoodsType::EFishCoin)
	{
		isOK = m_googleSDKOpt.googleRechargeItemReply(userRechargeRsp);  // 老版本，Google SDK 处理
	}
	
	/*
	else if (platformType == ThirdPlatformType::DownJoy && userRechargeInfo.item_type() != EGameGoodsType::EFishCoin)
	{
	 	isOK = downJoyRechargeItemReply(userRechargeRsp);  // 老版本，当乐平台处理
	}
	*/
 
	else if (platformType < ThirdPlatformType::MaxPlatformType)
	{
		isOK = m_thirdPlatformOpt.thirdPlatformRechargeItemReply(userRechargeRsp);  // 第三方平台充值处理
	}
	
	OptInfoLog("receive recharge reply, user = %s, platform type = %u, ok = %d, result = %d, order id = %s",
	userRechargeInfo.username().c_str(), platformType, isOK, userRechargeRsp.result(), userRechargeInfo.bills_id().c_str());
	
	/*
	// 是否首充值成功
	if (userRechargeInfo.item_flag() == MallItemFlag::First && isOK && userRechargeRsp.result() == OptSuccess)
	{
		com_protocol::HttpSrvLogicData& logicData = setLogicData(userRechargeInfo.username().c_str(), userRechargeInfo.username().length()).logicData;
		com_protocol::RechargeInfo* rcInfo = logicData.mutable_recharge();
	    (userRechargeInfo.item_type() == EGameGoodsType::EFishCoin) ? rcInfo->add_recharged_id(userRechargeInfo.item_id()) : rcInfo->set_flag(RechargeFlag::First);
	}
	*/ 
}

int CSrvMsgHandler::onHandle()
{
	ConnectData* cd = m_httpConnectMgr.getConnectData();
	if (cd != NULL)
	{
		// only log
		// OptInfoLog("receive connect data, flag = %d, status = %d, fd = %d, data = %s", cd->keyData.flag, cd->status, cd->fd, cd->receiveData);
		
		bool isSuccess = false;
		if (cd->status == ConnectStatus::CanRead)
		{
			if (cd->keyData.flag < ThirdPlatformType::MaxPlatformType)
			{
				CheckUserHandlerObject& handlerObj = m_checkUserHandlers[cd->keyData.flag];
				if (handlerObj.handler != NULL && handlerObj.instance != NULL)
				{
					isSuccess = (handlerObj.instance->*handlerObj.handler)(cd);
				}
				else
				{
					OptErrorLog("the key data flag is invalid, can not find the handler, flag = %d", cd->keyData.flag);
				}
			}
			else
			{
				HttpReplyHandlerInfo::iterator it = m_httpReplyHandlerInfo.find(cd->keyData.flag);
				if (it != m_httpReplyHandlerInfo.end() && it->second.handler != NULL && it->second.instance != NULL)
				{
					isSuccess = (it->second.instance->*it->second.handler)(cd);
				}
			}
		}
		
		if (!isSuccess) OptWarnLog("receive connect data and operation failed, key = %s, service id = %d, protocol id = %d, flag = %d, status = %d, fd = %d, data = %s",
		cd->keyData.key, cd->keyData.srvId, cd->keyData.protocolId, cd->keyData.flag, cd->status, cd->fd, cd->receiveData);
		
		m_httpConnectMgr.closeHttpConnect(cd);
		putConnectData(cd);
		
		if (m_httpConnectMgr.haveConnectData()) return Success;
	}
	
	return NoLogicHandle;
}

int CSrvMsgHandler::handleRequest(Connect* conn, ConnectUserData* ud)
{
	const char* flag = strstr(ud->reqData, RequestHeaderFlag);  // 是否是完整的http请求头部数据
	if (flag == NULL)
	{
		if (ud->reqDataLen >= MaxRequestDataLen)
		{
			OptErrorLog("receive buffer is full but can not find end flag, http header data = %s, len = %d", ud->reqData, ud->reqDataLen);
			
			// 关闭该连接
		    removeConnect(string(ud->connId));
	        return HeaderDataError;
		}
		
		return Success;
	}

    // 解析http头部数据信息
    RequestHeader reqHeaderData;
	int rc = CHttpDataParser::parseHeader(ud->reqData, reqHeaderData);
	if (rc != Success)
	{
		OptErrorLog("receive error http header data = %s, len = %d, parse result = %d", ud->reqData, ud->reqDataLen, rc);
		
		// 关闭该连接
		removeConnect(string(ud->connId));
		return rc;
	}
	
	// only log, to do nothing
	OptInfoLog("receive http payment data = %s, len = %d", ud->reqData, ud->reqDataLen);
	
	// 各个平台消息判断
	bool isOK = false;
	if (ProtocolGetMethodLen == reqHeaderData.methodLen && memcmp(reqHeaderData.method, ProtocolGetMethod, reqHeaderData.methodLen) == 0)  // GET 协议
	{
		/*
		// 当乐平台付费应答消息
		const char* djPaymentKey = NULL;
		if (strlen(m_downJoyCfg.paymentUrl) == reqHeaderData.urlLen && memcmp(reqHeaderData.url, m_downJoyCfg.paymentUrl, reqHeaderData.urlLen) == 0)
		{
			djPaymentKey = m_downJoyCfg.paymentKey;  // 当乐安卓版本
		}
		else if (strlen(m_downJoyCfg.IOSpaymentUrl) == reqHeaderData.urlLen && memcmp(reqHeaderData.url, m_downJoyCfg.IOSpaymentUrl, reqHeaderData.urlLen) == 0)
		{
			djPaymentKey = m_downJoyCfg.IOSpaymentKey;  // 当乐苹果版本
		}
		
		if (djPaymentKey != NULL)  // 当乐平台付费应答消息
		{
			isOK = handleDownJoyPaymentMsg(conn, ud, reqHeaderData, djPaymentKey);
		}
        else
		*/
 
		{
			// 其他注册的第三方平台付费通知消息
			const ThirdPlatformConfig* thirdCfg = NULL;
			unsigned int idx = ThirdPlatformType::DownJoy;
			for (; idx < ThirdPlatformType::MaxPlatformType; ++idx)
			{
				UserPayGetOptHandlerObject& handlerObj = m_userPayGetOptHandlers[idx];
				if (handlerObj.handler != NULL && handlerObj.instance != NULL)
				{
					thirdCfg = m_thirdPlatformOpt.getThirdPlatformCfg(idx);
					if (thirdCfg != NULL && thirdCfg->paymentUrl != NULL && strlen(thirdCfg->paymentUrl) == reqHeaderData.urlLen && memcmp(reqHeaderData.url, thirdCfg->paymentUrl, reqHeaderData.urlLen) == 0)
					{
						isOK = (handlerObj.instance->*handlerObj.handler)(conn, reqHeaderData);
						break;
					}
				}
			}
			
			if (idx >= ThirdPlatformType::MaxPlatformType)
			{
				string httpUrl(reqHeaderData.url, reqHeaderData.urlLen);
	            OptErrorLog("receive http header get data but can not find the platform register, GET protocol url is error, request url = %s, len = %d", httpUrl.c_str(), reqHeaderData.urlLen);
				
				removeConnect(string(ud->connId));
		        return HeaderDataError;
			}
		}
		
		// 一个完整的http头部数据之后，剩余的其他数据量
		ud->reqDataLen -= (flag + RequestHeaderFlagLen - ud->reqData);
		if (ud->reqDataLen > 0)
		{
			if (ud->reqDataLen < MaxRequestDataLen)
			{
				memcpy(ud->reqData, flag + RequestHeaderFlagLen, ud->reqDataLen);
			}
			else
			{
				OptErrorLog("after handle get operation, the request data is very big so close connect, data len = %u, header len = %u",
				ud->reqDataLen, (flag + RequestHeaderFlagLen - ud->reqData));
				
				// 错误的消息则关闭该连接
				removeConnect(string(ud->connId));
			}
		}
		
		return isOK ? (int)Success : (int)HeaderDataError;
	}
	
	else if (ProtocolPostMethodLen == reqHeaderData.methodLen && memcmp(reqHeaderData.method, ProtocolPostMethod, reqHeaderData.methodLen) == 0)  // POST 协议
	{
		HttpMsgBody msgBody;
		msgBody.len = MaxRequestDataLen;
		rc = CHttpDataParser::parseBody(ud->reqData, flag + RequestHeaderFlagLen, msgBody);
		if (rc == BodyDataError)
		{
			OptErrorLog("receive error http body post data = %s, len = %d", ud->reqData, ud->reqDataLen);
			
			// 错误的消息体则关闭该连接
			removeConnect(string(ud->connId));
			
			return Success;
		}
		
		if (rc == IncompleteBodyData) return Success;  // 继续等待接收完毕完整的消息体
		
		// 其他注册的第三方平台付费通知消息
		const ThirdPlatformConfig* thirdCfg = NULL;
		unsigned int idx = ThirdPlatformType::YouWan;
		for (; idx < ThirdPlatformType::MaxPlatformType; ++idx)
		{
			UserPayPostOptHandlerObject& handlerObj = m_userPayPostOptHandlers[idx];
			if (handlerObj.handler != NULL && handlerObj.instance != NULL)
			{
				thirdCfg = m_thirdPlatformOpt.getThirdPlatformCfg(idx);
				if (thirdCfg != NULL && thirdCfg->paymentUrl != NULL && strlen(thirdCfg->paymentUrl) == reqHeaderData.urlLen && memcmp(reqHeaderData.url, thirdCfg->paymentUrl, reqHeaderData.urlLen) == 0)
				{
					isOK = (handlerObj.instance->*handlerObj.handler)(conn, msgBody);
					break;
				}
			}
		}
		
		if (idx >= ThirdPlatformType::MaxPlatformType)
		{
			string httpUrl(reqHeaderData.url, reqHeaderData.urlLen);
			OptErrorLog("receive http body post data but can not find the platform register, POST protocol url is error, request url = %s, len = %d", httpUrl.c_str(), reqHeaderData.urlLen);
			
			removeConnect(string(ud->connId));
			return HeaderDataError;
		}

		// 一个完整的http头部数据&消息体之后，剩余的其他数据量
		ud->reqDataLen -= (flag + RequestHeaderFlagLen - ud->reqData);
		ud->reqDataLen -= msgBody.len;
		if (ud->reqDataLen > 0)
		{
			if (ud->reqDataLen < MaxRequestDataLen)
			{
				memcpy(ud->reqData, flag + RequestHeaderFlagLen + msgBody.len, ud->reqDataLen);
			}
			else
			{
				OptErrorLog("after handle post operation, the request data is very big so close connect, data len = %u, header len = %u, body len = %u",
				ud->reqDataLen, (flag + RequestHeaderFlagLen - ud->reqData), msgBody.len);
				
				// 错误的消息体则关闭该连接
				removeConnect(string(ud->connId));
			}
		}

		return isOK ? (int)Success : (int)HeaderDataError;
	}
	
	return HeaderDataError;
}

/*
void CSrvMsgHandler::saveLogicData()
{
	for (UserToLogicData::iterator it = m_user2LogicData.begin(); it != m_user2LogicData.end(); ++it)
	{
		if (it->second.isUpdate == 1)
		{
			const char* msgData = serializeMsgToBuffer(it->second.logicData, "save user logic data to redis");
			if (msgData != NULL)
			{
				int rc = getRedisService().setHField(HttpLogicDataKey, HttpLogicDataKeyLen, it->first.c_str(), it->first.length(), msgData, it->second.logicData.ByteSize());
				if (rc != 0) OptErrorLog("save user logic data to redis error, user = %s, rc = %d", it->first.c_str(), rc);
				
				OptErrorLog("only test, save user logic data to redis test, user = %s, rc = %d, len = %d", it->first.c_str(), rc, it->second.logicData.ByteSize());
			}
		}
	}
	
	m_user2LogicData.clear();
}
*/

void CSrvMsgHandler::setCheckLogicDataTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	OptInfoLog("check logic data time begin, current timer id = %u, remain count = %u", timerId, remainCount);
	
	// 执行定时任务
	// saveLogicData();
	m_thirdPlatformOpt.checkUnHandleTransactionData();

	// 重新设置定时器
	struct tm tmval;
	time_t curSecs = time(NULL);
	unsigned int intervalSecs = 60 * 60 * 24;
	if (localtime_r(&curSecs, &tmval) != NULL)
	{
		tmval.tm_sec = 0;
		tmval.tm_min = 0;
		tmval.tm_hour = atoi(CCfg::getValue("HttpService", "CheckLogicDataTime"));
		++tmval.tm_mday;
		time_t nextSecs = mktime(&tmval);
		if (nextSecs != (time_t)-1)
		{
			intervalSecs = nextSecs - curSecs;
		}
		else
		{
			OptErrorLog("setCheckLogicDataTimer, get next time error");
		}
	}
	else
	{
		OptErrorLog("setCheckLogicDataTimer, get local time error");
	}

	unsigned int tId = setTimer(intervalSecs * 1000, (TimerHandler)&CSrvMsgHandler::setCheckLogicDataTimer);
	OptInfoLog("check logic data time end, next timer id = %u, interval = %u, year = %d, month = %d, day = %d, hour = %d, min = %d, sec = %d",
	tId, intervalSecs, tmval.tm_year + 1900, tmval.tm_mon + 1, tmval.tm_mday, tmval.tm_hour, tmval.tm_min, tmval.tm_sec);
}

int CSrvMsgHandler::onClientData(Connect* conn, const char* data, const unsigned int len)
{
	// 挂接用户数据
	ConnectUserData* ud = (ConnectUserData*)getUserData(conn);
	if (ud == NULL)
	{
		unsigned int buffLen = 0;
		ud = (ConnectUserData*)getDataBuffer(buffLen);
		memset(ud, 0, sizeof(ConnectUserData));
		ud->connIdLen = snprintf(ud->connId, MaxConnectIdLen - 1, "^%s:%d~?", CSocket::toIPStr(conn->peerIp), conn->peerPort);
		addConnect(string(ud->connId), conn, ud);
	}
	
	// 保存请求数据
	unsigned int maxCpLen = MaxRequestDataLen - ud->reqDataLen;
	if (len < maxCpLen) maxCpLen = len;
	memcpy(ud->reqData + ud->reqDataLen, data, maxCpLen);
	ud->reqDataLen += maxCpLen;

	return handleRequest(conn, ud);
}

void CSrvMsgHandler::registerCheckUserHandler(unsigned int platformType, CheckUserHandler handler, CHandler* instance)
{
	if (platformType < ThirdPlatformType::MaxPlatformType)
	{
		CheckUserHandlerObject& handlerObj = m_checkUserHandlers[platformType];
		handlerObj.instance = instance;
		handlerObj.handler = handler;
	}
}

void CSrvMsgHandler::registerUserPayGetHandler(unsigned int platformType, UserPayGetOptHandler handler, CHandler* instance)
{
	if (platformType < ThirdPlatformType::MaxPlatformType)
	{
		UserPayGetOptHandlerObject& handlerObj = m_userPayGetOptHandlers[platformType];
		handlerObj.instance = instance;
		handlerObj.handler = handler;
	}
}

void CSrvMsgHandler::registerUserPayPostHandler(unsigned int platformType, UserPayPostOptHandler handler, CHandler* instance)
{
	if (platformType < ThirdPlatformType::MaxPlatformType)
	{
		UserPayPostOptHandlerObject& handlerObj = m_userPayPostOptHandlers[platformType];
		handlerObj.instance = instance;
		handlerObj.handler = handler;
	}
}

void CSrvMsgHandler::registerCheckUserRequest(unsigned int platformType, CheckUserRequest handler, CHandler* instance)
{
	if (platformType < ThirdPlatformType::MaxPlatformType)
	{
		CheckUserRequestObject& handlerObj = m_checkUserRequests[platformType];
		handlerObj.instance = instance;
		handlerObj.handler = handler;
	}
}

void CSrvMsgHandler::handleCheckUserRequest(unsigned int platformType, const ParamKey2Value& key2value, unsigned int srcSrvId)
{
	if (platformType < ThirdPlatformType::MaxPlatformType)
	{
		CheckUserRequestObject& handlerObj = m_checkUserRequests[platformType];
		if (handlerObj.handler != NULL && handlerObj.instance != NULL)
		{
			return (handlerObj.instance->*handlerObj.handler)(key2value, srcSrvId);
		}
	}
	
	OptErrorLog("the platform type is invalid, can not find the handler, type = %u", platformType);
}

void CSrvMsgHandler::registerHttpReplyHandler(unsigned int id, HttpReplyHandler handler, CHandler* instance)
{
	if (id > HttpReplyHandlerID::IdBeginValue && id < HttpReplyHandlerID::IdEndValue)
	{
		m_httpReplyHandlerInfo[id] = HttpReplyHandlerObject(instance, handler);
	}
}


void CSrvMsgHandler::checkDownJoyUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CheckDownJoyUserReq checkDownJoyUserReq;
	if (!parseMsgFromBuffer(checkDownJoyUserReq, data, len, "check down joy user")) return;

    const char* appId = NULL;
	const char* appKey = NULL;
	if (checkDownJoyUserReq.ostype() == ClientVersionType::AndroidMerge)  // 安卓合并版本
	{
		appId = m_downJoyCfg.appId;
		appKey = m_downJoyCfg.appKey;
	}
	else if (checkDownJoyUserReq.ostype() == ClientVersionType::AppleMerge)  // 苹果版本
	{
		appId = m_downJoyCfg.IOSappId;
		appKey = m_downJoyCfg.IOSappKey;
	}
	else
	{
		OptErrorLog("checkDownJoyUser, the os type invalid, type = %d", checkDownJoyUserReq.ostype());
		return;
	}
	
	/*
	请求示例：
	http://ngsdk.d.cn/api/cp/checkToken?appid=195&umid=36223535814&token=4C18A0AEAB1
	B4C9BBFD49E21E202025C&sig=9405aec7d7785d4cbfa6126004635406
	sig 加密规则：appId|appKey|token|umid
	明文：
	195|j5VEvxhc|4C18A0AEAB1B4C9BBFD49E21E202025C|36223535814
	sig MD5 密文：
	9405aec7d7785d4cbfa6126004635406
	http message : GET /api/cp/checkToken?appid=195&umid=36223535814&token=4C18A0AEAB1B4C9BBFD49E21E202025C&sig=9405aec7d7785d4cbfa6126004635406 HTTP/1.1
	*/ 
	
	// MD5加密
	char md5Buff[Md5BuffSize] = {0};
	unsigned int md5Len = snprintf(m_msgBuffer, NFrame::MaxMsgLen - 1, "%s|%s|%s|%s", appId, appKey, checkDownJoyUserReq.token().c_str(), checkDownJoyUserReq.umid().c_str());
	md5Encrypt(m_msgBuffer, md5Len, md5Buff);

	// 构造http消息请求头部数据
	ConnectData* cd = getConnectData();
	memset(cd, 0, sizeof(ConnectData));
	
	/*
	strcpy(cd->sendData, "GET ");
	strcat(cd->sendData, m_downJoyCfg.url);
	strcat(cd->sendData, "?appid=");
	strcat(cd->sendData, appId);
	strcat(cd->sendData, "&umid=");
	strcat(cd->sendData, checkDownJoyUserReq.umid().c_str());
	strcat(cd->sendData, "&token=");
	strcat(cd->sendData, checkDownJoyUserReq.token().c_str());
	strcat(cd->sendData, "&sig=");
	strcat(cd->sendData, md5Buff);
	strcat(cd->sendData, " HTTP/1.1\r\nHost: ");
	strcat(cd->sendData, m_downJoyCfg.host);
	strcat(cd->sendData, RequestHeaderFlag);
	cd->sendDataLen = strlen(cd->sendData);
	*/
	
	cd->sendDataLen = snprintf(cd->sendData, MaxNetBuffSize - 1, "GET %s?appid=%s&umid=%s&token=%s&sig=%s HTTP/1.1\r\nHost: %s%s",
	m_downJoyCfg.url, appId, checkDownJoyUserReq.umid().c_str(), checkDownJoyUserReq.token().c_str(), md5Buff, m_downJoyCfg.host, RequestHeaderFlag);

	cd->timeOutSecs = HttpConnectTimeOut;
	strcpy(cd->keyData.key, getContext().userData);
	cd->keyData.flag = ThirdPlatformType::DownJoy;
	cd->keyData.srvId = srcSrvId;
	cd->keyData.protocolId = CHECK_DOWNJOY_USER_RSP;

    OptInfoLog("do down joy http connect, userId = %s, os type = %d, down joy umid = %s, username = %s, nickname = %s, token = %s, http request data len = %d, content = %s",
		       cd->keyData.key, checkDownJoyUserReq.ostype(), checkDownJoyUserReq.umid().c_str(), checkDownJoyUserReq.username().c_str(), checkDownJoyUserReq.nickname().c_str(),
			   checkDownJoyUserReq.token().c_str(), cd->sendDataLen, cd->sendData);
			   
	if (!m_httpConnectMgr.doHttpConnect(m_downJoyCfg.ip, m_downJoyCfg.port, cd))
	{
		OptErrorLog("do down joy http connect error");
		putConnectData(cd);
		return;
	}
}

void CSrvMsgHandler::getDownJoyRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/*
	com_protocol::GetDownJoyPaymentNoReq getDownJoyPaymentNoReq;
	if (!parseMsgFromBuffer(getDownJoyPaymentNoReq, data, len, "get down joy recharge transaction")) return;
	
	char rechargeTranscation[MaxRechargeTransactionLen] = {'\0'};
	com_protocol::GetDownJoyRechargeTransactionRsp getDownJoyPaymentNoRsp;
	unsigned int platformType = 0;
	if (getRechargeTransaction(srcSrvId, ThirdPlatformType::DownJoy, getDownJoyPaymentNoReq, rechargeTranscation, MaxRechargeTransactionLen, platformType))
	{
		getDownJoyPaymentNoRsp.set_result(OptSuccess);
		getDownJoyPaymentNoRsp.set_money(getDownJoyPaymentNoReq.money());
		getDownJoyPaymentNoRsp.set_servername(CCfg::getValue("ServiceName", "DownJoy"));
		getDownJoyPaymentNoRsp.set_rechargetransactionno(rechargeTranscation);
		getDownJoyPaymentNoRsp.set_itemtype(getDownJoyPaymentNoReq.itemtype());
		getDownJoyPaymentNoRsp.set_itemid(getDownJoyPaymentNoReq.itemid());
		getDownJoyPaymentNoRsp.set_itemcount(getDownJoyPaymentNoReq.itemcount());
		getDownJoyPaymentNoRsp.set_itemamount(getDownJoyPaymentNoReq.itemamount());
	}
	else
	{
		getDownJoyPaymentNoRsp.set_result(RechargeTranscationFormatError);
	}

	if (serializeMsgToBuffer(getDownJoyPaymentNoRsp, m_msgBuffer, NFrame::MaxMsgLen, "send down joy recharge transcation to client"))
	{
		int rc = sendMessage(m_msgBuffer, getDownJoyPaymentNoRsp.ByteSize(), srcSrvId, GET_DOWNJOY_RECHARGE_TRANSACTION_RSP);
		if (rc == Success && getDownJoyPaymentNoRsp.result() == OptSuccess)
		{
			// 充值订单信息记录
			// format = name|type|id|count|amount|money|rechargeNo|request|umid|playername|platformType
			DownJoyRechargeLog("%s|%d|%d|%d|%d|%.2f|%s|request|%s|%s|%d", getDownJoyPaymentNoReq.username().c_str(), getDownJoyPaymentNoRsp.itemtype(), getDownJoyPaymentNoRsp.itemid(),
			getDownJoyPaymentNoRsp.itemcount(), getDownJoyPaymentNoReq.itemamount(), getDownJoyPaymentNoRsp.money(), getDownJoyPaymentNoRsp.rechargetransactionno().c_str(),
			getDownJoyPaymentNoReq.umid().c_str(), getDownJoyPaymentNoReq.playername().c_str(), platformType);
		}
		
		OptInfoLog("down joy recharge transaction record, down joy umid = %s, player name = %s, money = %.2f, user = %s, bill id = %s, result = %d, ostype = %d, rc = %d",
		getDownJoyPaymentNoReq.umid().c_str(), getDownJoyPaymentNoReq.playername().c_str(), getDownJoyPaymentNoReq.money(),
		getDownJoyPaymentNoReq.username().c_str(), getDownJoyPaymentNoRsp.rechargetransactionno().c_str(), getDownJoyPaymentNoRsp.result(), getDownJoyPaymentNoReq.os_type(), rc);
	}
	*/ 
}

void CSrvMsgHandler::cancelDownJoyRechargeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/*
	com_protocol::CancelDownJoyRechargeNotify cancelDownJoyRechargeNotifyMsg;
	if (!parseMsgFromBuffer(cancelDownJoyRechargeNotifyMsg, data, len, "cancel down joy recharge notify")) return;
	
	// 充值订单信息记录
	// format = name|rechargeNo|cancel|umid|playername|info
	DownJoyRechargeLog("%s|%s|cancel|%s|%s|%s", getContext().userData, cancelDownJoyRechargeNotifyMsg.rechargetransactionno().c_str(), cancelDownJoyRechargeNotifyMsg.umid().c_str(),
    cancelDownJoyRechargeNotifyMsg.playername().c_str(), cancelDownJoyRechargeNotifyMsg.info().c_str());
	
	OptInfoLog("cancel down joy recharge, down joy umid = %s, player name = %s, info = %s, user = %s, bill id = %s", cancelDownJoyRechargeNotifyMsg.umid().c_str(),
    cancelDownJoyRechargeNotifyMsg.playername().c_str(), cancelDownJoyRechargeNotifyMsg.info().c_str(), getContext().userData, cancelDownJoyRechargeNotifyMsg.rechargetransactionno().c_str());
	*/ 
}

bool CSrvMsgHandler::checkDownJoyCfg()
{
	// 当乐配置项值
	const char* downJoyCfg[] = {"Host", "Url", "AppId", "AppKey", "PaymentKey", "PaymentUrl", "RechargeOrderKey", "RechargeLogFileName", "RechargeLogFileSize", "RechargeLogFileCount",
	                            "IOSAppId", "IOSAppKey", "IOSPaymentKey", "IOSPaymentUrl",};
	const char** downJoyCfgValue[] = {&(m_downJoyCfg.host), &(m_downJoyCfg.url), &(m_downJoyCfg.appId), &(m_downJoyCfg.appKey), &(m_downJoyCfg.paymentKey),
	&(m_downJoyCfg.paymentUrl), &(m_downJoyCfg.innerMD5Key), &(m_downJoyCfg.rechargeLogFileName), &(m_downJoyCfg.rechargeLogFileSize), &(m_downJoyCfg.rechargeLogFileCount),
	&(m_downJoyCfg.IOSappId), &(m_downJoyCfg.IOSappKey), &(m_downJoyCfg.IOSpaymentKey), &(m_downJoyCfg.IOSpaymentUrl), };
	for (int i = 0; i < (int)(sizeof(downJoyCfg) / sizeof(downJoyCfg[0])); ++i)
	{
		const char* value = CCfg::getValue("DownJoy", downJoyCfg[i]);
		if (value == NULL)
		{
            ReleaseErrorLog("downjoy config item error");
			return false;
		}
		
		*downJoyCfgValue[i] = value;
	}
	
	if ((unsigned int)atoi(m_downJoyCfg.rechargeLogFileSize) < MinRechargeLogFileSize || (unsigned int)atoi(m_downJoyCfg.rechargeLogFileCount) < MinRechargeLogFileCount)
	{
		ReleaseErrorLog("downjoy config item error, recharge log file min size = %d, count = %d", MinRechargeLogFileSize, MinRechargeLogFileCount);
		return false;
	}
	
	const char* checkHost = CCfg::getValue("DownJoy", "CheckHost");  // 检查是否使用启动当乐处理流程
	if (checkHost != NULL && atoi(checkHost) == 1 && !getHostInfo(m_downJoyCfg.host, m_downJoyCfg.ip, m_downJoyCfg.port))
	{
		ReleaseErrorLog("get downjoy host info error, host = %s", m_downJoyCfg.host);
		return false;
	}

	return true;
}

bool CSrvMsgHandler::checkDownJoyUserReply(ConnectData* cd)
{
	// 解析当乐平台接口数据
	// data : {"valid":"1","roll":true,"interval":60,"times":4,"msg_code":2000,"msg_desc":"成功"}
	char* begin = strchr(cd->receiveData, '{');
    char* end = strchr(cd->receiveData, '}');
	if (begin == NULL || end == NULL) return false;
	*++end = '\0';

	// 返回码
	const char* msgCodeFlag = "\"msg_code\":";
	char* msgCode = strstr(begin + 1, msgCodeFlag);
	if (msgCode == NULL) return false;
	msgCode += strlen(msgCodeFlag);
	char* msgCodeEnd = strchr(msgCode, ',');
	if (msgCodeEnd == NULL) return false;
	
	// 有效标识
	const char* validFlag = "\"valid\":\"";
	char* valid = strstr(begin + 1, validFlag);
	char* validEnd = NULL;
	if (valid != NULL)
	{
	    valid += strlen(validFlag);
	    validEnd = strchr(valid, '"');
	}
	
	// 信息描述
	const char* msgDescFlag = "\"msg_desc\":\"";
	char* msgDesc = strstr(begin + 1, msgDescFlag);
	char* msgDescEnd = NULL;
	if (msgDesc != NULL)
	{
		msgDesc += strlen(msgDescFlag);
		msgDescEnd = strchr(msgDesc, '"');
	}

    // 全部查找完毕后才能在结束位置修改为 '\0
	*msgCodeEnd = '\0';
	int validValue = 2;
	if (validEnd != NULL)
	{
		*validEnd = '\0';
		validValue = atoi(valid);
	}
	
    const char* desc = "";
	if (msgDescEnd != NULL)
	{
		desc = msgDesc;
		*msgDescEnd = '\0';
	}

	com_protocol::CheckDownJoyUserRsp checkDownJoyUserRsp;
	checkDownJoyUserRsp.set_result(atoi(msgCode));
	checkDownJoyUserRsp.set_valid(validValue);
	if (serializeMsgToBuffer(checkDownJoyUserRsp, m_msgBuffer, NFrame::MaxMsgLen, "send check down joy user result to client"))
	{
		int rc = sendMessage(m_msgBuffer, checkDownJoyUserRsp.ByteSize(), cd->keyData.key, strlen(cd->keyData.key), cd->keyData.srvId, cd->keyData.protocolId);
		OptInfoLog("send check down joy user reply message, rc = %d, result = %d, valid = %d, desc = %s", rc, checkDownJoyUserRsp.result(), checkDownJoyUserRsp.valid(), desc);
	}
	
	return true;
}

bool CSrvMsgHandler::getRechargeTransaction(unsigned int srcSrvId, const ThirdPlatformType type, const com_protocol::GetDownJoyPaymentNoReq& msg,
                                            char* rechargeTranscation, unsigned int len, unsigned int& platformType)
{
	int orderBuffLen = len - Md5ResultLen - 1;
	if (orderBuffLen <= 1) return false;
	
	platformType = type;  // 第三方平台类型
	if (msg.os_type() == ClientVersionType::AppleMerge)  // 苹果版本
	{
		platformType = platformType * ThirdPlatformType::ClientDeviceBase + platformType;  // 转换为苹果版本值
	}
	else if (msg.os_type() != ClientVersionType::AndroidMerge)  // 非安卓合并版本
	{
		OptErrorLog("getRechargeTransaction, the os type invalid, type = %d", msg.os_type());
		return false;
	}
	
	++m_rechargeTransactionId;
	time_t curSecs = time(NULL);
	struct tm* tmval = localtime(&curSecs);

	// 订单号格式 : MD5.time.platform.order-type.id.count.money-srv.user
	// 874CC49B0685FFE0284718E2AA1D1D46.20150621153831.1.1-0.1003.1.0.01-1001.10000001
	unsigned int size = snprintf(rechargeTranscation + Md5ResultLen, orderBuffLen, ".%d%02d%02d%02d%02d%02d.%d.%d-%d.%d.%d.%.2f-%d.%s|%s",
	tmval->tm_year + 1900, tmval->tm_mon + 1, tmval->tm_mday, tmval->tm_hour, tmval->tm_min, tmval->tm_sec,
	platformType, m_rechargeTransactionId, msg.itemtype(), msg.itemid(), msg.itemcount(), msg.money(), srcSrvId, msg.username().c_str(), m_downJoyCfg.innerMD5Key);
	
	if (size >= (unsigned int)orderBuffLen)
	{
		--m_rechargeTransactionId;
	    return false;
	}
	
	// MD5加密订单信息
	char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(rechargeTranscation + Md5ResultLen, size, md5Buff, false);
	memcpy(rechargeTranscation, md5Buff, Md5ResultLen);
	
	len = Md5ResultLen + size - strlen(m_downJoyCfg.innerMD5Key) - 1;
	rechargeTranscation[len] = '\0';
	
	// OptInfoLog("get recharge transaction, type = %d, id = %s, len = %d", type, rechargeTranscation, len);
	
	return true;
}

bool CSrvMsgHandler::handleDownJoyPaymentMsg(Connect* conn, ConnectUserData* ud, const RequestHeader& reqHeaderData)
{
	return false;

	/*
	// GET /downjoyRecharge/ChallengeFishing1?
	// order=3735912519735icbaif2l&money=0.01&mid=2469593221172&time=20150720102525&result=1&
	// ext=C4CA4238A0B923820DCC509A6F75849B.20150620180429.1.100-0.1001.10.0.78-1001.10000001&
	// signature=d9f1f30fa8b7aa2bcc831312731d1471
	const char* param = strchr(ud->reqData, '?');
	const char* signature = strstr(ud->reqData, "&signature=");
	if (param == NULL || signature == NULL || param > signature)
	{
		OptErrorLog("receive error down joy http header data[param] = %s, len = %d", ud->reqData, ud->reqDataLen);
		return false;
	}

	bool isOK = handleDownJoyPaymentMsg(reqHeaderData, param, signature);
	CHttpResponse rsp;
	unsigned int msgLen = 0;
	isOK ? rsp.setContent("success") : rsp.setContent("failure");
	const char* rspMsg = rsp.getMessage(msgLen);
	int rc = sendDataToClient(conn, rspMsg, msgLen);

	OptInfoLog("send reply message to down joy http, rc = %d, data = %s", rc, rspMsg);
	
	return isOK;
	*/ 
}

bool CSrvMsgHandler::handleDownJoyPaymentMsg(const RequestHeader& reqHeaderData, const char* param, const char* signature)
{
	// GET /downjoyRecharge/ChallengeFishing1?
	// order=3735912519735icbaif2l&money=0.01&mid=2469593221172&time=20150720102525&result=1&
	// ext=C4CA4238A0B923820DCC509A6F75849B.20150620180429.1.100-0.1001.10.0.78-1001.10000001&
	// signature=d9f1f30fa8b7aa2bcc831312731d1471

    ParamKey2Value::const_iterator signatureIt = reqHeaderData.paramKey2Value.find("signature");
	if (signatureIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("receive error http header data, not find signature data");
		return false;
	}
	
	// 当乐平台MD5加密校验参数
	// order=ok123456&money=5.21&mid=123456&time=20141212105433&result=1&ext=1234567890&key=NIhmYdfPe05f
	unsigned int paramLen = signature - param;
	memcpy(m_msgBuffer, param + 1, paramLen);  // 待校验参数
	unsigned int keyLen = snprintf(m_msgBuffer + paramLen, NFrame::MaxMsgLen - paramLen - 1, "key=%s", m_downJoyCfg.paymentKey);  // 附加上当乐平台提供的MD5校验KEY
	char md5Buff[Md5BuffSize] = {0};
	md5Encrypt(m_msgBuffer, paramLen + keyLen, md5Buff);
	if (strcmp(md5Buff, signatureIt->second.c_str()) != 0)
	{
		OptErrorLog("receive error http header data, the signature is error, signature = %s", m_msgBuffer);
		return false;
	}

    // 参数读取校验
    ParamKey2Value::const_iterator moneyIt = reqHeaderData.paramKey2Value.find("money");
	if (moneyIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("receive error http header data, not find money data");
		return false;
	}

	ParamKey2Value::const_iterator resultIt = reqHeaderData.paramKey2Value.find("result");
	if (resultIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("receive error http header data, not find result data");
		return false;
	}
	
	ParamKey2Value::const_iterator midIt = reqHeaderData.paramKey2Value.find("mid");
	if (midIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("receive error http header data, not find mid data");
		return false;
	}
	
	ParamKey2Value::const_iterator orderIt = reqHeaderData.paramKey2Value.find("order");
	if (orderIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("receive error http header data, not find order data");
		return false;
	}
	
	ParamKey2Value::const_iterator timeIt = reqHeaderData.paramKey2Value.find("time");
	if (timeIt == reqHeaderData.paramKey2Value.end())
	{
		OptErrorLog("receive error http header data, not find time data");
		return false;
	}
	
	// 内部MD5加密校验内部订单信息
	// ext=C4CA4238A0B923820DCC509A6F75849B.20150620180429.1.100-0.1001.10.0.78-1001.10000001
	ParamKey2Value::const_iterator extIt = reqHeaderData.paramKey2Value.find("ext");
	if (extIt == reqHeaderData.paramKey2Value.end() || extIt->second.length() <= Md5ResultLen)
	{
		OptErrorLog("receive error http header data, ext data error");
		return false;
	}
	const char* rechargeTranscationNo = extIt->second.c_str() + Md5ResultLen;
	keyLen = snprintf(m_msgBuffer, NFrame::MaxMsgLen - 1, "%s|%s", rechargeTranscationNo, m_downJoyCfg.innerMD5Key);  // 附加上内部MD5校验KEY
	md5Encrypt(m_msgBuffer, keyLen, md5Buff, false);
	if (memcmp(md5Buff, extIt->second.c_str(), Md5ResultLen) != 0)
	{
		OptErrorLog("receive error http header data, the inner recharge transaction is error, data = %s", m_msgBuffer);
		return false;
	}

    // 物品信息
	m_msgBuffer[extIt->second.length() - Md5ResultLen] = '\0';
    const char* itemInfo = strchr(m_msgBuffer, '-');
	if (itemInfo == NULL)
	{
		OptErrorLog("receive error http header data, can not find the item info");
		return false;
	}
	
	unsigned int itemValue[ItemDataFlag::ISize - 1];
	for (int i = 0; i < ItemDataFlag::ISize - 1; ++i)
	{
		char* valueEnd = (char*)strchr(++itemInfo, '.');
		if (*itemInfo == '-' || *itemInfo == '\0' || valueEnd == NULL)
		{
			OptErrorLog("receive error http header data, item info error");
			return false;
		}
		
		*valueEnd = '\0';
		itemValue[i] = atoi(itemInfo);
		
		itemInfo = valueEnd;
	}
	
	// 用户信息
    char* userInfo = (char*)strchr(++itemInfo, '-');
	if (userInfo == NULL)
	{
		OptErrorLog("receive error http header data, can not find the user info");
		return false;
	}
	
	// 付费价格比较
	*userInfo = '\0';
	if (strcmp(itemInfo, moneyIt->second.c_str()) != 0)
	{
		OptErrorLog("receive error http header data, the payment money error, pay money = %s, recharge money = %s", moneyIt->second.c_str(), itemInfo);
		return false;
	}
	
	char* uName = (char*)strchr(++userInfo, '.');
	if (uName == NULL)
	{
		OptErrorLog("receive error http header data, can not find the service info");
		return false;
	}
	*uName = '\0';
	string userName = ++uName;
	unsigned int srcSrvId = atoi(userInfo);

    // 充值订单信息记录
    // format = name|type|id|count|money|rechargeNo|reply|umid|result|order|time
	DownJoyRechargeLog("%s|%d|%d|%d|%s|%s|reply|%s|%s|%s|%s", userName.c_str(), itemValue[ItemDataFlag::IType], itemValue[ItemDataFlag::IId], itemValue[ItemDataFlag::ICount],
	moneyIt->second.c_str(), extIt->second.c_str(), midIt->second.c_str(), resultIt->second.c_str(), orderIt->second.c_str(), timeIt->second.c_str());
    
	if (atoi(resultIt->second.c_str()) == 1)
	{
		// 充值成功，发放物品，充值信息入库
		snprintf(m_msgBuffer, NFrame::MaxMsgLen - 1, "%s|%s", timeIt->second.c_str(), orderIt->second.c_str());
		com_protocol::UserRechargeReq userRechargeReq;
		userRechargeReq.set_bills_id(extIt->second.c_str());
		userRechargeReq.set_username(userName);
		userRechargeReq.set_item_type(itemValue[ItemDataFlag::IType]);
		userRechargeReq.set_item_id(itemValue[ItemDataFlag::IId]);
		userRechargeReq.set_item_count(itemValue[ItemDataFlag::ICount]);
		userRechargeReq.set_item_flag(0); // getMallMgr().getItemFlag(userName.c_str(), userName.length(), itemValue[ItemDataFlag::IId]));
		userRechargeReq.set_recharge_rmb(atof(moneyIt->second.c_str()));
		userRechargeReq.set_third_account(midIt->second.c_str());
		userRechargeReq.set_third_type(ThirdPlatformType::DownJoy);
		userRechargeReq.set_third_other_data(m_msgBuffer);
		if (serializeMsgToBuffer(userRechargeReq, m_msgBuffer, NFrame::MaxMsgLen, "send down joy recharge result to db proxy"))
		{
			int rc = sendMessageToCommonDbProxy(m_msgBuffer, userRechargeReq.ByteSize(), BUSINESS_USER_RECHARGE_REQ, userName.c_str(), userName.length());
			OptInfoLog("send down joy payment result to db proxy, rc = %d, user name = %s, bill id = %s", rc, userName.c_str(), userRechargeReq.bills_id().c_str());
		}
	}
	else
	{
		// 充值失败则不会发放物品，直接通知客户端
		com_protocol::DownJoyRechargeTransactionNotify downJoyRechargeTransactionNotify;
		downJoyRechargeTransactionNotify.set_result(atoi(resultIt->second.c_str()));
		downJoyRechargeTransactionNotify.set_money(atof(moneyIt->second.c_str()));
		downJoyRechargeTransactionNotify.set_rechargetransactionno(extIt->second.c_str());
		downJoyRechargeTransactionNotify.set_itemtype(itemValue[ItemDataFlag::IType]);
		downJoyRechargeTransactionNotify.set_itemid(itemValue[ItemDataFlag::IId]);
		downJoyRechargeTransactionNotify.set_itemcount(itemValue[ItemDataFlag::ICount]);
		downJoyRechargeTransactionNotify.set_itemamount(itemValue[ItemDataFlag::ICount]);
		if (serializeMsgToBuffer(downJoyRechargeTransactionNotify, m_msgBuffer, NFrame::MaxMsgLen, "send down joy recharge notify to client"))
		{
			int rc = sendMessage(m_msgBuffer, downJoyRechargeTransactionNotify.ByteSize(), userName.c_str(), userName.length(), srcSrvId, DOWNJOY_USER_RECHARGE_NOTIFY);
			OptWarnLog("send down joy payment result to client, rc = %d, user name = %s, result = %d, bill id = %s",
			rc, userName.c_str(), downJoyRechargeTransactionNotify.result(), extIt->second.c_str());
		}
	}

	return true;
}

bool CSrvMsgHandler::downJoyRechargeItemReply(const com_protocol::UserRechargeRsp& userRechargeRsp)
{
	// 查找目标服务id值
	const int result = (userRechargeRsp.result() == 0) ? 1 : userRechargeRsp.result();
	const com_protocol::UserRechargeReq& userRechargeInfo = userRechargeRsp.info();
	const char* srvBegin = strrchr(userRechargeInfo.bills_id().c_str(), '-');
	const char* srvEnd = (srvBegin != NULL) ? strchr(srvBegin, '.') : NULL;
	if (srvBegin == NULL || srvEnd == NULL)
	{
		OptErrorLog("receive down joy payment result but not find service id info, result = %d, user name = %s, bill id = %s",
		userRechargeRsp.result(), userRechargeInfo.username().c_str(), userRechargeInfo.bills_id().c_str());
		return false;
	}
	
	// 提取消息的目标服务id值
	const unsigned int srvLen = srvEnd - srvBegin - 1;
	memcpy(m_msgBuffer, srvBegin + 1, srvLen);
	m_msgBuffer[srvLen] = '\0';
	const unsigned int srvId = atoi(m_msgBuffer);
	if (srvId < 1)
	{
		OptErrorLog("receive down joy payment result but the service id error, result = %d, user name = %s, bill id = %s",
		userRechargeRsp.result(), userRechargeInfo.username().c_str(), userRechargeInfo.bills_id().c_str());
		return false;
	}

    int rc = -1;
	com_protocol::DownJoyRechargeTransactionNotify downJoyRechargeTransactionNotify;
	downJoyRechargeTransactionNotify.set_result(result);
	downJoyRechargeTransactionNotify.set_money(userRechargeInfo.recharge_rmb());
	downJoyRechargeTransactionNotify.set_rechargetransactionno(userRechargeInfo.bills_id());
	downJoyRechargeTransactionNotify.set_itemtype(userRechargeInfo.item_type());
	downJoyRechargeTransactionNotify.set_itemid(userRechargeInfo.item_id());
	downJoyRechargeTransactionNotify.set_itemcount(userRechargeInfo.item_count());
	downJoyRechargeTransactionNotify.set_itemamount(userRechargeRsp.item_amount());
	
	char giftData[MaxNetBuffSize] = {0};
	getGiftInfo(userRechargeRsp, downJoyRechargeTransactionNotify.mutable_gift_array(), giftData, MaxNetBuffSize);
	
	if (serializeMsgToBuffer(downJoyRechargeTransactionNotify, m_msgBuffer, NFrame::MaxMsgLen, "send down joy recharge notify to client"))
	{
		rc = sendMessage(m_msgBuffer, downJoyRechargeTransactionNotify.ByteSize(), userRechargeInfo.username().c_str(), userRechargeInfo.username().length(), srvId, DOWNJOY_USER_RECHARGE_NOTIFY);
	}
	
	// 充值订单信息记录最终结果
    // format = name|type|id|count|amount|flag|money|rechargeNo|finish|umid|result|order|giftdata
	DownJoyRechargeLog("%s|%u|%u|%u|%u|%u|%.2f|%s|finish|%s|%d|%s|%s", userRechargeInfo.username().c_str(), userRechargeInfo.item_type(), userRechargeInfo.item_id(), userRechargeInfo.item_count(), userRechargeRsp.item_amount(),
	userRechargeInfo.item_flag(), userRechargeInfo.recharge_rmb(), userRechargeInfo.bills_id().c_str(), userRechargeInfo.third_account().c_str(), userRechargeRsp.result(), userRechargeInfo.third_other_data().c_str(), giftData);
	
	OptInfoLog("send down joy payment result to client, rc = %d, user name = %s, result = %d, bill id = %s", rc, userRechargeInfo.username().c_str(), userRechargeRsp.result(), userRechargeInfo.bills_id().c_str());
	return true;
}

CLogger& CSrvMsgHandler::getDownJoyRechargeLogger()
{
	static CLogger rechargeLogger(m_downJoyCfg.rechargeLogFileName, atoi(m_downJoyCfg.rechargeLogFileSize), atoi(m_downJoyCfg.rechargeLogFileCount));
	return rechargeLogger;
}



void CSrvMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	int rc = m_httpConnectMgr.starSrv();
	if (rc != Success)
	{
	    ReleaseErrorLog("start http connect manager error, rc = %d", rc);
		stopService();
		return;
	}
	
	// 服务配置项值检查
	const char* serviceCfg[] = {"CheckLogicDataTime", "UnHandleTransactionMaxIntervalSeconds",};
	for (unsigned int i = 0; i < (sizeof(serviceCfg) / sizeof(serviceCfg[0])); ++i)
	{
		const char* cfgValue = CCfg::getValue("HttpService", serviceCfg[i]);
		if (cfgValue == NULL || atoi(cfgValue) < 1
		    || (i == 0 && atoi(cfgValue) > 23))
		{
			ReleaseErrorLog("HttpService config error, item = %s", serviceCfg[i]);
			stopService();
			return;
		}
	}
	
	if (!checkDownJoyCfg()) // 当乐配置项值
	{
		stopService();
		return;
	}
	
	/*
	const char* logicRedisSrvItem = "RedisLogicService";
	const char* ip = m_srvMsgCommCfg->get(logicRedisSrvItem, "IP");
	const char* port = m_srvMsgCommCfg->get(logicRedisSrvItem, "Port");
	const char* connectTimeOut = m_srvMsgCommCfg->get(logicRedisSrvItem, "Timeout");
	if (ip == NULL || port == NULL || connectTimeOut == NULL)
	{
		ReleaseErrorLog("redis service config error");
		stopService();
		return;
	}
	
	if (!m_redisDbOpt.connectSvr(ip, atoi(port), atol(connectTimeOut)))
	{
		ReleaseErrorLog("do connect redis service failed, ip = %s, port = %s, time out = %s", ip, port, connectTimeOut);
		stopService();
		return;
	}
	*/
	
	if (!m_thirdPlatformOpt.load(this) || !m_googleSDKOpt.load(this, &m_thirdPlatformOpt) || !m_mallMgr.load(this))
	{
		ReleaseErrorLog("load operation failed");
		stopService();
	    return;
	}
	
	registerCheckUserHandler(ThirdPlatformType::DownJoy, (CheckUserHandler)&CSrvMsgHandler::checkDownJoyUserReply, this);

	// 启动定时器执行定时任务
	setCheckLogicDataTimer(0, 0, NULL, 0);
}

void CSrvMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	m_googleSDKOpt.unload();
	m_thirdPlatformOpt.unload();
	// m_mallMgr.unload();
	
	// 保存逻辑数据到redis
	// saveLogicData();
	
	// 等待线程销毁资源，优雅的退出
	m_httpConnectMgr.stopSrv();
	while (m_httpConnectMgr.isRunning()) usleep(1000);
	
	// m_redisDbOpt.disconnectSvr();
}

void CSrvMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("http service data handler run, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId); 
}

void CSrvMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("http service register protocol handler, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
	
	registerProtocol(CommonSrv, CHECK_DOWNJOY_USER_REQ, (ProtocolHandler)&CSrvMsgHandler::checkDownJoyUser);
	
	// registerProtocol(CommonSrv, GET_DOWNJOY_RECHARGE_TRANSACTION_REQ, (ProtocolHandler)&CSrvMsgHandler::getDownJoyRechargeTransaction);
	// registerProtocol(CommonSrv, CANCEL_DOWNJOY_RECHARGE_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::cancelDownJoyRechargeNotify);
	
	registerProtocol(DBProxyCommonSrv, BUSINESS_USER_RECHARGE_RSP, (ProtocolHandler)&CSrvMsgHandler::rechargeItemReply);
	
	//监控统计
	m_monitorStat.init(this);
}

// 通知逻辑层对应的逻辑连接已被关闭
void CSrvMsgHandler::onClosedConnect(void* userData)
{
	ConnectUserData* ud = (ConnectUserData*)userData;
	removeConnect(string(ud->connId), false);  // 可能存在被动关闭的连接
	putDataBuffer((char*)userData);
}



int CHttpSrv::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("run http service name = %s, id = %d", name, id);
	return 0;
}

void CHttpSrv::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("stop http service name = %s, id = %d", name, id);
}

void CHttpSrv::onRegister(const char* name, const unsigned int id)
{
	ReleaseInfoLog("register http module, service name = %s, id = %d", name, id);
	
	// 注册模块实例
	const unsigned short HandlerMessageModule = 0;
	static CSrvMsgHandler msgHandler;
	m_connectNotifyToHandler = &msgHandler;
	registerModule(HandlerMessageModule, &msgHandler);
}

void CHttpSrv::onUpdateConfig(const char* name, const unsigned int id)
{
}

// 通知逻辑层对应的逻辑连接已被关闭
void CHttpSrv::onClosedConnect(void* userData)
{
	m_connectNotifyToHandler->onClosedConnect(userData);
}

// 通知逻辑层处理逻辑
int CHttpSrv::onHandle()
{
	return m_connectNotifyToHandler->onHandle();
}


CHttpSrv::CHttpSrv() : IService(HttpSrv, true)
{
	m_connectNotifyToHandler = NULL;
}

CHttpSrv::~CHttpSrv()
{
	m_connectNotifyToHandler = NULL;
}

REGISTER_SERVICE(CHttpSrv);

}

