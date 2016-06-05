
/* author : limingfan
 * date : 2015.06.30
 * description : 纯网络数据处理驱动
 */

#include <string.h>
#include "CNetDataHandler.h"
#include "CService.h"
#include "base/ErrorCode.h"


using namespace NCommon;
using namespace NErrorCode;

namespace NFrame
{

CNetDataHandler::CNetDataHandler()
{
}

CNetDataHandler::~CNetDataHandler()
{
}

// 收到网络客户端的数据
int CNetDataHandler::onClientMessage(const char* data, const unsigned int len, const unsigned int msgId,
								     unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn)
{
	return onClientData(conn, data, len);
}

int CNetDataHandler::onClientData(Connect* conn, const char* data, const unsigned int len)
{
	return NoFoundProtocolHandler;
}

// 本地端主动建立的连接
void CNetDataHandler::doCreateActiveConnect(ActiveConnectData* activeConnectData)
{
	(this->*(activeConnectData->cbFunc))(activeConnectData->conn, activeConnectData->data.peerIp, activeConnectData->data.peerPort, activeConnectData->userCb, activeConnectData->data.userId);
}

// 业务发起主动连接调用
// 本地端准备发起主动连接
// peerIp&peerPort&userId 唯一标识一个主动连接，三者都相同则表示是同一个连接
int CNetDataHandler::doActiveConnect(ActiveConnectHandler cbFunction, const strIP_t peerIp, unsigned short peerPort, unsigned int timeOut, void* userCb, int userId)
{
	int rc = m_service->doActiveConnect(this, cbFunction, peerIp, peerPort, timeOut, userCb, userId);
	if (rc != Success) ReleaseErrorLog("do active connect error, rc = %d", rc);
	return rc;
}

// 向目标网络客户端发送数据
int CNetDataHandler::sendDataToClient(Connect* conn, const char* data, const unsigned int len)
{
	int rc = m_service->sendDataToClient(conn, data, len);
	if (rc != Success) ReleaseErrorLog("send client data by connect error, rc = %d, len = %d", rc, len);
	return rc;
}

int CNetDataHandler::sendDataToClient(const char* connId, const char* data, const unsigned int len)
{
    int rc = (connId != NULL) ? m_service->sendDataToClient(getConnect(connId), data, len) : InvalidParam;
	if (rc != Success) ReleaseErrorLog("send client data by connId error, rc = %d, len = %d, connId = %s", rc, len, (connId != NULL) ? connId : "");
	return rc;
}

// 应用上层如果不使用框架提供的连接容器，则以string key类型发送消息时需要重写该接口
Connect* CNetDataHandler::getConnect(const string& connId)
{
	return m_connectMgr->getConnect(connId);
}

void CNetDataHandler::addConnect(const string& connId, Connect* conn, void* userData)
{
	m_connectMgr->addConnect(connId, conn, userData);
}

const IDToConnects& CNetDataHandler::getConnect()
{
	return m_connectMgr->getConnect();
}

void* CNetDataHandler::resetUserData(Connect* conn, void* userData)
{
	void* oldData = NULL;
	if (conn != NULL)
	{
		oldData = conn->userCb;
		conn->userCb = userData;
	}
	return oldData;
}

bool CNetDataHandler::resetUserData(const string& connId, void* userData)
{
	Connect* conn = m_connectMgr->getConnect(connId);
	resetUserData(conn, userData);
	return (conn != NULL);
}

void* CNetDataHandler::getUserData(Connect* conn)
{
	return m_connectMgr->getUserData(conn);
}

bool CNetDataHandler::removeConnect(const string& connId, bool doClose)
{
	return m_connectMgr->removeConnect(connId, doClose);
}

bool CNetDataHandler::haveConnect(const string& connId)
{
	return m_connectMgr->haveConnect(connId);
}

void CNetDataHandler::closeConnect(const string& connId)
{
	m_connectMgr->closeConnect(connId);
}

void CNetDataHandler::closeConnect(Connect* conn)
{
	m_connectMgr->closeConnect(conn);
}

void CNetDataHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
}

}

