
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

#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"
#include "CHttpSrv.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;


namespace NHttp
{

CHttpDataHandler::CHttpDataHandler() : m_memForRequestBuffer(MaxRequestBufferCount, MaxRequestBufferCount, sizeof(HttpRequestBuffer)),
m_memForConnectData(MaxConnectDataCount, MaxConnectDataCount, sizeof(ConnectData))
{
}

CHttpDataHandler::~CHttpDataHandler()
{
    m_memForRequestBuffer.clear();
	m_memForConnectData.clear();
}

// 根据host获取ip&port
bool CHttpDataHandler::getHostInfo(const char* host, strIP_t ip, unsigned short& port, const char* service)
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

ConnectData* CHttpDataHandler::getConnectData()
{
	return (ConnectData*)m_memForConnectData.get();
}

void CHttpDataHandler::putConnectData(ConnectData* connData)
{
	if (connData->cbData != NULL && connData->cbData->flag == 0) putDataBuffer((const char*)connData->cbData);

	m_memForConnectData.put((char*)connData);
}

char* CHttpDataHandler::getDataBuffer(unsigned int& len)
{
	len = m_memForRequestBuffer.getBuffSize();
	return m_memForRequestBuffer.get();
}

void CHttpDataHandler::putDataBuffer(const char* dataBuff)
{
	if (dataBuff != NULL) m_memForRequestBuffer.put((char*)dataBuff);
}

// 建立http连接，同时发送http请求数据
bool CHttpDataHandler::doHttpConnect(const char* ip, unsigned int port, const char* host, const char* url, const CHttpRequest& httpRequest,
									 const unsigned int requestId, const char* userData, const unsigned int srcSrvId,
                                     const UserCbData* cbData, const unsigned int cbDataLen, const unsigned short protocolId)
{
	if (ip == NULL || host == NULL || url == NULL || cbDataLen >= sizeof(HttpRequestBuffer))
	{
		OptErrorLog("to do http connect, the param is error, ip = %p, host = %p, url = %p, cb data len = %u",
		ip, host, url, cbDataLen);
		return false;
	}

    ConnectData* cd = getConnectData();
	memset(cd, 0, sizeof(ConnectData));
	
	cd->timeOutSecs = HttpConnectTimeOut;
	if (userData != NULL) strncpy(cd->keyData.key, userData, MaxKeySize - 1);
	cd->keyData.requestId = requestId;
	cd->keyData.srvId = srcSrvId;
	cd->keyData.protocolId = protocolId;
	
	cd->sendDataLen = MaxNetBuffSize;
	const char* msgData = httpRequest.getMessage(cd->sendData, cd->sendDataLen, host, url);
    if (msgData == NULL)
	{
		putConnectData(cd);
		OptErrorLog("get http request message error, request id = %u", requestId);
		return false;
	}

    bool isOK = m_httpConnectMgr.doHttpConnect(ip, port, cd);
	OptInfoLog("do http connect, cd = %p, result = %d, fd = %d, userData = %s, request id = %u, message len = %u, content = %s",
	cd, isOK, cd->fd, cd->keyData.key, requestId, cd->sendDataLen, cd->sendData);
	
	if (isOK)
	{
		if (cbData != NULL && cbDataLen > 0)
		{
			if (cbData->flag == 0)
			{
				// 可以直接使用这里内存管理的内存块
				unsigned int buffLen = 0;
				UserCbData* userCbData = (UserCbData*)getDataBuffer(buffLen);
				memcpy(userCbData, cbData, cbDataLen);
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

bool CHttpDataHandler::doHttpConnect(const char* ip, const unsigned short port, ConnectData* connData)
{
	return m_httpConnectMgr.doHttpConnect(ip, port, connData);
}

// 主动发送的http请求收到应答消息
int CHttpDataHandler::onHandle()
{
	ConnectData* cd = m_httpConnectMgr.receiveConnectData();
	if (cd != NULL)
	{
		// only log
		// OptInfoLog("receive connect data, requestId = %d, status = %d, fd = %d, data = %s", cd->keyData.requestId, cd->status, cd->fd, cd->receiveData);
		
		bool isSuccess = false;
		if (cd->status == ConnectStatus::CanRead)
		{
			HttpReplyHandlerInfo::iterator replyIt = m_httpReplyHandlerInfo.find(cd->keyData.requestId);
			if (replyIt != m_httpReplyHandlerInfo.end() && replyIt->second.handler != NULL && replyIt->second.instance != NULL)
			{
				// 回调上层处理函数处理消息
				isSuccess = (replyIt->second.instance->*replyIt->second.handler)(cd);
			}
		}

		if (isSuccess) OptInfoLog("receive connect data and operation finish, cd = %p, key = %s, service id = %d, protocol id = %d, requestId = %d, status = %d, fd = %d",
		cd, cd->keyData.key, cd->keyData.srvId, cd->keyData.protocolId, cd->keyData.requestId, cd->status, cd->fd);
		
		else OptWarnLog("receive connect data and operation failed, cd = %p, key = %s, service id = %d, protocol id = %d, requestId = %d, status = %d, fd = %d, data = %s",
		cd, cd->keyData.key, cd->keyData.srvId, cd->keyData.protocolId, cd->keyData.requestId, cd->status, cd->fd, cd->receiveData);
		
		// http短连接，应答消息回来后底层已经直接关闭连接了
		// 这里直接回收数据内存块即可
		putConnectData(cd);
		
		if (m_httpConnectMgr.haveConnectData()) return Success;
	}
	
	return NoLogicHandle;
}

// 被动收到外部的http请求消息
int CHttpDataHandler::handleRequest(Connect* conn, HttpRequestBuffer* ud)
{
	// 是否是完整的http请求头部数据
	const char* flag = strstr(ud->reqData, RequestHeaderFlag);
	if (flag == NULL)
	{
		if (ud->reqDataLen >= (MaxRequestDataLen - 1))
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
	OptInfoLog("receive http data = %s, len = %d", ud->reqData, ud->reqDataLen);
	
	// GET 协议消息
	bool isOK = false;
	if (ProtocolGetMethodLen == reqHeaderData.methodLen && memcmp(reqHeaderData.method, ProtocolGetMethod, reqHeaderData.methodLen) == 0)
	{
		const string httpUrl(reqHeaderData.url, reqHeaderData.urlLen);
		HttpGetOptHandlerInfo::iterator getOptIt = m_httpGetOptHandlerInfo.find(httpUrl);
		if (getOptIt != m_httpGetOptHandlerInfo.end() && getOptIt->second.handler != NULL && getOptIt->second.instance != NULL)
		{
			// 回调上层处理函数处理消息
			isOK = (getOptIt->second.instance->*getOptIt->second.handler)(ud->connId, conn, reqHeaderData);
		}
		else
		{
			OptErrorLog("receive http header get data but can not find the register, GET protocol url is error, request url = %s, len = %d",
			httpUrl.c_str(), reqHeaderData.urlLen);
			
			removeConnect(string(ud->connId));
			return HeaderDataError;
		}

		// 一个完整的http头部数据之后，剩余的其他数据量
		ud->reqDataLen -= (flag + RequestHeaderFlagLen - ud->reqData);
		if (ud->reqDataLen > 0)
		{
			if (ud->reqDataLen < MaxRequestDataLen)
			{
				memmove(ud->reqData, flag + RequestHeaderFlagLen, ud->reqDataLen);
				ud->reqData[ud->reqDataLen] = '\0';
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
	
	// POST 协议消息
	else if (ProtocolPostMethodLen == reqHeaderData.methodLen && memcmp(reqHeaderData.method, ProtocolPostMethod, reqHeaderData.methodLen) == 0)
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
		
		const string httpUrl(reqHeaderData.url, reqHeaderData.urlLen);
		HttpPostOptHandlerInfo::iterator postOptIt = m_httpPostOptHandlerInfo.find(httpUrl);
		if (postOptIt != m_httpPostOptHandlerInfo.end() && postOptIt->second.handler != NULL && postOptIt->second.instance != NULL)
		{
			// 回调上层处理函数处理消息
			isOK = (postOptIt->second.instance->*postOptIt->second.handler)(ud->connId, conn, reqHeaderData, msgBody);
		}
		else
		{
			OptErrorLog("receive http body post data but can not find the platform register, POST protocol url is error, request url = %s, len = %d",
			httpUrl.c_str(), reqHeaderData.urlLen);
			
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
				memmove(ud->reqData, flag + RequestHeaderFlagLen + msgBody.len, ud->reqDataLen);
				ud->reqData[ud->reqDataLen] = '\0';			
			}
			else
			{
				OptErrorLog("after handle post operation, the request data is very big so close connect, data len = %u, header len = %u, body len = %u, all data = %s, header data = %s",
				ud->reqDataLen, (flag + RequestHeaderFlagLen - ud->reqData), msgBody.len, ud->reqData, flag + RequestHeaderFlagLen);
				
				// 错误的消息体则关闭该连接
				removeConnect(string(ud->connId));
			}
		}

		return isOK ? (int)Success : (int)HeaderDataError;
	}
	
	return HeaderDataError;
}

// 被动收到外部的http请求消息
int CHttpDataHandler::onClientData(Connect* conn, const char* data, const unsigned int len)
{
	// 挂接用户数据
	HttpRequestBuffer* ud = (HttpRequestBuffer*)getUserData(conn);
	if (ud == NULL)
	{
		unsigned int buffLen = 0;
		ud = (HttpRequestBuffer*)getDataBuffer(buffLen);
		memset(ud, 0, sizeof(HttpRequestBuffer));
		ud->connIdLen = snprintf(ud->connId, MaxConnectIdLen - 1, "%s:%d", CSocket::toIPStr(conn->peerIp), conn->peerPort);
		addConnect(string(ud->connId), conn, ud);
		
		OptInfoLog("create passive connect id = %s", ud->connId);
	}
	
	// 保存请求数据
	unsigned int maxCpLen = MaxRequestDataLen - ud->reqDataLen - 1;
	if (len < maxCpLen) maxCpLen = len;
	memcpy(ud->reqData + ud->reqDataLen, data, maxCpLen);  // 继续追加收到的数据
	ud->reqDataLen += maxCpLen;
	ud->reqData[ud->reqDataLen] = '\0';  // 加上字符串结束符

	return handleRequest(conn, ud);
}

void CHttpDataHandler::registerGetOptHandler(const string& url, HttpGetOptHandler handler, CHandler* instance)
{
	m_httpGetOptHandlerInfo[url] = HttpGetOptHandlerObject(instance, handler);
}

void CHttpDataHandler::registerPostOptHandler(const string& url, HttpPostOptHandler handler, CHandler* instance)
{
	m_httpPostOptHandlerInfo[url] = HttpPostOptHandlerObject(instance, handler);
}

void CHttpDataHandler::registerHttpReplyHandler(unsigned int requestId, HttpReplyHandler handler, CHandler* instance)
{
	m_httpReplyHandlerInfo[requestId] = HttpReplyHandlerObject(instance, handler);
}

// 通知逻辑层对应的逻辑连接已被关闭
void CHttpDataHandler::onClosedConnect(void* userData)
{
	HttpRequestBuffer* ud = (HttpRequestBuffer*)userData;
	bool isPassive = removeConnect(string(ud->connId), false);  // 可能存在被动关闭的连接
	OptInfoLog("close connect id = %s, is passive = %d", ud->connId, isPassive);
	
	putDataBuffer((char*)userData);
}

void CHttpDataHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	int rc = m_httpConnectMgr.starSrv();
	if (rc != Success)
	{
	    ReleaseErrorLog("start http connect manager error, rc = %d", rc);
		stopService();
		return;
	}
	
	rc = onInit(srvName, srvId, moduleId);
	if (rc != Success)
	{
	    ReleaseErrorLog("on init error, rc = %d", rc);
		stopService();
		return;
	}
}

void CHttpDataHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	onUnInit(srvName, srvId, moduleId);
	
	// 等待线程销毁资源，优雅的退出
	m_httpConnectMgr.stopSrv();
	while (m_httpConnectMgr.isRunning()) usleep(1000);
}

int CHttpDataHandler::onInit(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	return Success;
}

void CHttpDataHandler::onUnInit(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
}

void CHttpDataHandler::onUpdateConfig()
{
}


int CHttpSrv::registerHttpHandler(CHttpDataHandler* pInstance)
{
	m_httpDataHandler = pInstance;
	return registerNetModule(m_httpDataHandler);
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

void CHttpSrv::onUpdateConfig(const char* name, const unsigned int id)
{
	m_httpDataHandler->onUpdateConfig();
}

// 通知逻辑层对应的逻辑连接已被关闭
void CHttpSrv::onClosedConnect(void* userData)
{
	m_httpDataHandler->onClosedConnect(userData);
}

// 通知逻辑层处理逻辑
int CHttpSrv::onHandle()
{
	return m_httpDataHandler->onHandle();
}


CHttpSrv::CHttpSrv(unsigned int srvType) : IService(srvType, true)
{
	m_httpDataHandler = NULL;
}

CHttpSrv::~CHttpSrv()
{
	m_httpDataHandler = NULL;
}

}

