
/* author : limingfan
 * date : 2015.03.27
 * description : 游戏逻辑开发框架API
 */

#include <string.h>
#include "CGameModule.h"
#include "CService.h"
#include "base/ErrorCode.h"


using namespace NCommon;
using namespace NErrorCode;

namespace NFrame
{

CGameModule::CGameModule()
{
	memset(&m_netClientContext, 0, sizeof(m_netClientContext));
	m_clientProtocolHanders = &m_protocolHanders[OutsideClientServiceType];
}

CGameModule::~CGameModule()
{
	memset(&m_netClientContext, 0, sizeof(m_netClientContext));
	m_clientProtocolHanders = NULL;
}

// 收到网络客户端的消息				  
int CGameModule::onClientMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
								 unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn)
{
    if (protocolId >= MaxProtocolIDCount)
	{
		ReleaseErrorLog("receive net client msg error, protocolId = %d", protocolId);
		return InvalidParam;
	}

	ProtocolIdToHandler::const_iterator handlerIt = m_clientProtocolHanders->find(protocolId);
	if (handlerIt == m_clientProtocolHanders->end() || handlerIt->second.handler == NULL)
	{
		ReleaseErrorLog("not find the protocol handler for net client msg, protocolId = %d", protocolId);
		return NoFoundProtocolHandler;
	}
	
	// 填写消息上下文信息
	m_netClientContext.serviceId = serviceId;
	m_netClientContext.moduleId = moduleId;
	m_netClientContext.protocolId = protocolId;
	m_netClientContext.msgId = msgId;
	m_netClientContext.conn = conn;
	
	// 业务逻辑处理消息
	m_msgType = MessageType::NetClientMsg;
	(handlerIt->second.instance->*(handlerIt->second.handler))(msgData, msgLen, serviceId, moduleId, protocolId);

	m_netClientContext.protocolId = (unsigned short)-1;
	m_netClientContext.msgId = (unsigned int)-1;
	m_netClientContext.conn = NULL;
	
	return Success;
}

// 当前网络客户端消息上下文内容
const NetClientContext& CGameModule::getNetClientContext()
{
	return m_netClientContext;
}

// 设置新的上下文连接对象，返回之前的连接对象
Connect* CGameModule::setContextConnect(Connect* newConn)
{
	Connect* curConn = m_netClientContext.conn;
	m_netClientContext.conn = newConn;
	return curConn;
}

// 向目标网络客户端发送消息
int CGameModule::sendMsgToClient(const char* msgData, const unsigned int msgLen, unsigned short protocolId, unsigned int serviceId, unsigned short moduleId)
{
	int rc = m_service->sendMsgToClient(msgData, msgLen, m_netClientContext.msgId, serviceId, moduleId, protocolId, m_netClientContext.conn);
	if (rc != Success) ReleaseErrorLog("send client message error1, msgLen = %u, serviceId = %d, moduleId = %d, protocolId = %d, rc = %d", msgLen, serviceId, moduleId, protocolId, rc);
	return rc;
}

int CGameModule::sendMsgToClient(const char* msgData, const unsigned int msgLen, unsigned short protocolId,
					             const char* userName, unsigned int serviceId, unsigned short moduleId, unsigned int msgId)
{	
    int rc = (userName != NULL) ? m_service->sendMsgToClient(msgData, msgLen, msgId, serviceId, moduleId, protocolId, getConnect(userName)) : InvalidParam;
	if (rc != Success) ReleaseErrorLog("send client message error2, msgLen = %u, serviceId = %d, moduleId = %d, protocolId = %d, rc = %d, userName = %s",
	msgLen, serviceId, moduleId, protocolId, rc, (userName != NULL) ? userName : "");
	return rc;
}

int CGameModule::sendMsgToClient(const char* msgData, const unsigned int msgLen, unsigned short protocolId,
	                             Connect* conn, unsigned int serviceId, unsigned short moduleId, unsigned int msgId)
{
    int rc = m_service->sendMsgToClient(msgData, msgLen, msgId, serviceId, moduleId, protocolId, conn);
	if (rc != Success) ReleaseErrorLog("send client message error3, msgLen = %u, serviceId = %d, moduleId = %d, protocolId = %d, rc = %d", msgLen, serviceId, moduleId, protocolId, rc);
	return rc;
}

}

