
/* author : limingfan
 * date : 2015.10.12
 * description : 游戏逻辑处理驱动
 */

#include <string.h>
#include "CLogicHandler.h"
#include "CService.h"
#include "base/ErrorCode.h"


using namespace NCommon;
using namespace NErrorCode;

namespace NFrame
{

CLogicHandler::CLogicHandler()
{
	memset(&m_connectProxyContext, 0, sizeof(m_connectProxyContext));
	m_proxyProtocolHanders = &m_protocolHanders[OutsideClientServiceType];
}

CLogicHandler::~CLogicHandler()
{
	memset(&m_connectProxyContext, 0, sizeof(m_connectProxyContext));
	m_proxyProtocolHanders = NULL;
}

// 收到网络客户端的数据
int CLogicHandler::onProxyMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
								  unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, ConnectProxy* conn)
{
	if (protocolId >= MaxProtocolIDCount)
	{
		ReleaseErrorLog("receive proxy msg error, protocolId = %d", protocolId);
		return InvalidParam;
	}

	ProtocolIdToHandler::const_iterator handlerIt = m_proxyProtocolHanders->find(protocolId);
	if (handlerIt == m_proxyProtocolHanders->end() || handlerIt->second.handler == NULL)
	{
		ReleaseErrorLog("not find the protocol handler for proxy msg, protocolId = %d", protocolId);
		return NoFoundProtocolHandler;
	}
	
	// 填写消息上下文信息
	m_connectProxyContext.serviceId = serviceId;
	m_connectProxyContext.moduleId = moduleId;
	m_connectProxyContext.protocolId = protocolId;
	m_connectProxyContext.msgId = msgId;
	m_connectProxyContext.conn = conn;
	
	if (onPreHandleMessage(msgData, msgLen, m_connectProxyContext))
	{
		// 业务逻辑处理消息
		m_msgType = MessageType::ConnectProxyMsg;
		(handlerIt->second.instance->*(handlerIt->second.handler))(msgData, msgLen, serviceId, moduleId, protocolId);
	}

	m_connectProxyContext.protocolId = (unsigned short)-1;
	m_connectProxyContext.msgId = (unsigned int)-1;
	m_connectProxyContext.conn = NULL;
	
	return Success;
}

bool CLogicHandler::onPreHandleMessage(const char* msgData, const unsigned int msgLen, const ConnectProxyContext& connProxyContext)
{
	return true;
}


// 当前网络客户端消息上下文内容
const ConnectProxyContext& CLogicHandler::getConnectProxyContext()
{
	return m_connectProxyContext;
}

// 设置新的上下文连接对象，返回之前的连接对象
ConnectProxy* CLogicHandler::resetContextConnectProxy(ConnectProxy* newConn)
{
	ConnectProxy* curConn = m_connectProxyContext.conn;
	m_connectProxyContext.conn = newConn;
	return curConn;
}

// 停止连接代理，服务停止时调用
void CLogicHandler::stopConnectProxy()
{
	m_connectMgr->clearConnectProxy();
	m_service->stopProxy();

	ReleaseWarnLog("stop connect proxy, service id = %u, name = %s", getSrvId(), getSrvName());
}

// 清理停止连接代理，服务启动时调用，比如服务异常退出，则启动前调用该函数，关闭之前的代理连接
void CLogicHandler::cleanUpConnectProxy(const unsigned int proxyId[], const unsigned int len)
{
	m_connectMgr->clearConnectProxy();
	m_service->cleanUpProxy(proxyId, len);

	ReleaseWarnLog("clean up connect proxy, service id = %u, name = %s", getSrvId(), getSrvName());
}


// 获取代理信息
uuid_type CLogicHandler::getProxyId(ConnectProxy* connProxy)
{
	return m_service->getProxyId(connProxy);
}

ConnectProxy* CLogicHandler::getProxy(const uuid_type id)
{
	return m_service->getProxy(id);
}

// 服务关闭用户连接时调用
void CLogicHandler::closeProxy(const uuid_type id, int cbFlag)
{
	m_service->closeProxy(id, true, cbFlag);
}

void* CLogicHandler::resetProxyUserData(ConnectProxy* conn, void* userData)
{
	void* oldData = NULL;
	if (conn != NULL)
	{
		oldData = conn->userCb;
		conn->userCb = userData;
	}
	return oldData;
}

void* CLogicHandler::getProxyUserData(ConnectProxy* conn)
{
	return (conn != NULL) ? conn->userCb : NULL;
}


// 向目标网络代理发送消息
int CLogicHandler::sendMsgToProxy(const char* msgData, const unsigned int msgLen, unsigned short protocolId, unsigned int serviceId, unsigned short moduleId)
{
	int rc = m_service->sendMsgToProxy(msgData, msgLen, m_connectProxyContext.msgId, serviceId, moduleId, protocolId, m_connectProxyContext.conn);
	if (rc != Success) ReleaseErrorLog("send proxy message error1, msgLen = %u, serviceId = %d, moduleId = %d, protocolId = %d, rc = %d", msgLen, serviceId, moduleId, protocolId, rc);
	return rc;
}

int CLogicHandler::sendMsgToProxy(const char* msgData, const unsigned int msgLen, unsigned short protocolId,
					              const char* userName, unsigned int serviceId, unsigned short moduleId, unsigned int msgId)
{
    int rc = (userName != NULL) ? m_service->sendMsgToProxy(msgData, msgLen, msgId, serviceId, moduleId, protocolId, getConnectProxy(userName)) : InvalidParam;
	if (rc != Success) ReleaseErrorLog("send proxy message error2, msgLen = %u, serviceId = %d, moduleId = %d, protocolId = %d, rc = %d, userName = %s",
	msgLen, serviceId, moduleId, protocolId, rc, (userName != NULL) ? userName : "");
	return rc;
}

int CLogicHandler::sendMsgToProxy(const char* msgData, const unsigned int msgLen, unsigned short protocolId,
	                              ConnectProxy* conn, unsigned int serviceId, unsigned short moduleId, unsigned int msgId)
{
    int rc = m_service->sendMsgToProxy(msgData, msgLen, msgId, serviceId, moduleId, protocolId, conn);
	if (rc != Success) ReleaseErrorLog("send proxy message error3, msgLen = %u, serviceId = %d, moduleId = %d, protocolId = %d, rc = %d", msgLen, serviceId, moduleId, protocolId, rc);
	return rc;
}

// 应用上层如果不使用框架提供的连接代理容器，则以string key类型发送消息时需要重写该接口
ConnectProxy* CLogicHandler::getConnectProxy(const string& connId)
{
	return m_connectMgr->getConnectProxy(connId);
}

void CLogicHandler::addConnectProxy(const string& connId, ConnectProxy* conn, void* userData)
{
	m_connectMgr->addConnectProxy(connId, conn, userData);
}

const IDToConnectProxys& CLogicHandler::getConnectProxy()
{
	return m_connectMgr->getConnectProxy();
}

bool CLogicHandler::resetProxyUserData(const string& connId, void* userData)
{
	ConnectProxy* conn = m_connectMgr->getConnectProxy(connId);
	resetProxyUserData(conn, userData);
	return (conn != NULL);
}

bool CLogicHandler::removeConnectProxy(const string& connId, int cbFlag, bool doClose)
{
	return m_connectMgr->removeConnectProxy(connId, doClose, cbFlag);
}

bool CLogicHandler::haveConnectProxy(const string& connId)
{
	return m_connectMgr->haveConnectProxy(connId);
}

bool CLogicHandler::closeConnectProxy(const string& connId, int cbFlag)
{
	return m_connectMgr->closeConnectProxy(connId, cbFlag);
}

void CLogicHandler::closeConnectProxy(ConnectProxy* conn, int cbFlag)
{
	m_connectMgr->closeConnectProxy(conn, cbFlag);
}

void CLogicHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
}

}

