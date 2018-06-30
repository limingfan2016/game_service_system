
/* author : limingfan
 * date : 2015.01.30
 * description : 服务开发模块API定义实现
 */

#include <string.h>
#include "CModule.h"
#include "CService.h"
#include "base/ErrorCode.h"


using namespace NCommon;
using namespace NErrorCode;

namespace NFrame
{

extern CService& getService();  // 获取服务实例


CHandler::CHandler()
{
}

CHandler::~CHandler()
{
}

// 定时器消息
void CHandler::onTimeOut(TimerHandler timerHandler, unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	(this->*timerHandler)(timerId, userId, param, remainCount);
}


CModule::CModule()
{
	m_moduleId = (unsigned short)-1;
    m_service = &getService();
	memset(&m_context, 0, sizeof(m_context));
	
	m_srvMsgCommCfg = NULL;
	m_connectMgr = NULL;

	m_msgType = MessageType::InnerServiceMsg;
}

CModule::~CModule()
{
	m_moduleId = (unsigned short)-1;
    m_service = NULL;
	memset(&m_context, 0, sizeof(m_context));
	
	m_srvMsgCommCfg = NULL;
	m_connectMgr = NULL;

	m_msgType = MessageType::InnerServiceMsg;
}

// 加载模块			   
void CModule::doLoad(unsigned short moduleId, const char* srvName, const unsigned int srvId, CConnectMgr* connectMgr, CCfg* srvMsgCommCfg)
{
	m_moduleId = moduleId;
	m_connectMgr = connectMgr;
	m_srvMsgCommCfg = srvMsgCommCfg;
	m_context.dstSrvId = getSrvId();
	m_context.dstModuleId = m_moduleId;
	
	onLoad(srvName, srvId, m_moduleId);
	onRegister(srvName, srvId, m_moduleId);  // 在此注册本模块处理的协议函数，绑定协议ID到实现函数
}

// 服务模块收到消息				
int CModule::onServiceMessage(const char* msgData, const unsigned int msgLen, const char* userData, const unsigned int userDataLen,
							  unsigned short dstProtocolId, unsigned int srcSrvId, unsigned short srcSrvType,
							  unsigned short srcModuleId, unsigned short srcProtocolId, int userFlag, unsigned int msgId, const char* srvAsyncDataFlag, unsigned int srvAsyncDataFlagLen)
{
	if (userDataLen > MaxUserDataLen)
	{
		ReleaseErrorLog("receive msg error, srcServiceId = %d, srcServiceType = %d, dstProtocolId = %d, userDataLen = %d, srcProtocolId = %d",
		                srcSrvId, srcSrvType, dstProtocolId, userDataLen, srcProtocolId);
		return InvalidParam;
	}
	
	const ProtocolIdToHandler* protocolHandler = NULL;
	ProtocolIdToHandler::const_iterator handlerIt;
	if (dstProtocolId < MaxProtocolIDCount)
	{
		protocolHandler = &m_protocolHanders[srcSrvType];
		handlerIt = protocolHandler->find(dstProtocolId);
		if (handlerIt == protocolHandler->end() || handlerIt->second.handler == NULL)
		{
			protocolHandler = &m_protocolHanders[CommonServiceType];  // 默认取公共协议
			handlerIt = protocolHandler->find(dstProtocolId);
		}
	}
	else
	{
		protocolHandler = &m_serviceProtocolHandler;
		handlerIt = protocolHandler->find(dstProtocolId);
	}
	
	if (handlerIt == protocolHandler->end() || handlerIt->second.instance == NULL || handlerIt->second.handler == NULL)
	{
		ReleaseErrorLog("can not find the protocol handler, srcServiceId = %d, srcServiceType = %d, dstProtocolId = %d", srcSrvId, srcSrvType, dstProtocolId);
		return NoFoundProtocolHandler;
	}
	
	// 填写消息上下文信息
	m_context.srcSrvId = srcSrvId;
	m_context.srcSrvType = srcSrvType;
	m_context.srcModuleId = srcModuleId;
	m_context.srcProtocolId = srcProtocolId;
	m_context.dstProtocolId = dstProtocolId;
	
	if (userDataLen > 0) memcpy(m_context.userData, userData, userDataLen);
	m_context.userData[userDataLen] = '\0';
	m_context.userDataLen = userDataLen;
	m_context.userFlag = userFlag;
	
	// 异步数据标识
	if (srvAsyncDataFlagLen > 0) memcpy(m_context.srvAsyncDataFlag, srvAsyncDataFlag, srvAsyncDataFlagLen);
	m_context.srvAsyncDataFlag[srvAsyncDataFlagLen] = '\0';
	m_context.srvAsyncDataFlagLen = srvAsyncDataFlagLen;
	
	// 业务逻辑处理消息
	m_msgType = MessageType::InnerServiceMsg;
	(handlerIt->second.instance->*(handlerIt->second.handler))(msgData, msgLen, srcSrvId, srcModuleId, srcProtocolId);
	
	m_context.dstProtocolId = (unsigned short)-1;
	m_context.userData[0] = '\0';
	m_context.userDataLen = 0;
	m_context.userFlag = 0;

	m_context.srvAsyncDataFlag[0] = '\0';
	m_context.srvAsyncDataFlagLen = 0;
	
	return Success;
}

// 获取服务ID，主要用于发送服务消息
unsigned int CModule::getServiceId(const char* serviceName)
{
	return m_service->getServiceId(serviceName);
}

const char* CModule::getSrvName()
{
	return m_service->getName();
}

unsigned short CModule::getSrvType()
{
	return m_service->getType();
}

unsigned int CModule::getSrvId()
{
	return m_service->getId();
}

// 服务注册的本模块ID
unsigned short CModule::getModuleId()
{
	return m_moduleId;
}

void CModule::stopService()
{
	return m_service->stop();
}
	
// 当前消息上下文内容
const Context& CModule::getContext()
{
	return m_context;
}

// 注册处理协议的函数
int CModule::registerProtocol(unsigned int srcSrvType, unsigned short protocolId, ProtocolHandler handler, CHandler* instance)
{
	if (protocolId == MaxProtocolIDCount || handler == NULL) return InvalidParam;
	
	if (instance == NULL) instance = this;
	MsgHandler* msgHandler = NULL;
	if (protocolId < MaxProtocolIDCount)
	{
		m_protocolHanders[srcSrvType][protocolId] = MsgHandler();
		msgHandler = &(m_protocolHanders[srcSrvType][protocolId]);
	}
	else
	{
		m_serviceProtocolHandler[protocolId] = MsgHandler();
		msgHandler = &(m_serviceProtocolHandler[protocolId]);
	}
	
	msgHandler->instance = instance;
	msgHandler->handler = handler;
	
	return Success;
}
					
// 向目标服务发送请求消息
// handleProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话
int CModule::sendMessage(const char* msgData, const unsigned int msgLen, const char* userData, const unsigned int userDataLen,
				         unsigned int dstServiceId, unsigned short dstProtocolId, unsigned short dstModuleId, unsigned short handleProtocolId)
{
	int rc = m_service->sendMessage(msgData, msgLen, userData, userDataLen, m_context.srvAsyncDataFlag, m_context.srvAsyncDataFlagLen, dstServiceId, dstProtocolId, dstModuleId, m_moduleId, handleProtocolId, m_context.userFlag);
	if (rc != Success) ReleaseErrorLog("send service message error1, msgLen = %u, dstServiceId = %d, dstProtocolId = %d, rc = %d", msgLen, dstServiceId, dstProtocolId, rc);
	return rc;
}

// 向目标服务发送请求消息
// handleProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话
int CModule::sendMessage(const char* msgData, const unsigned int msgLen, int userFlag, const char* userData, const unsigned int userDataLen,
				         unsigned int dstServiceId, unsigned short dstProtocolId, unsigned short dstModuleId, unsigned short handleProtocolId, unsigned int msgId)
{
	int rc = m_service->sendMessage(msgData, msgLen, userData, userDataLen, m_context.srvAsyncDataFlag, m_context.srvAsyncDataFlagLen, dstServiceId, dstProtocolId, dstModuleId, m_moduleId, handleProtocolId, userFlag, msgId);
	if (rc != Success) ReleaseErrorLog("send service message error2, msgLen = %u, dstServiceId = %d, dstProtocolId = %d, rc = %d", msgLen, dstServiceId, dstProtocolId, rc);
	return rc;
}

// 向目标服务发送请求消息，默认自动发送上下文中存在的用户数据
// handleProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话
int CModule::sendMessage(const char* msgData, const unsigned int msgLen, unsigned int dstServiceId, unsigned short dstProtocolId,
                         unsigned short dstModuleId, unsigned short handleProtocolId)
{
	int rc = m_service->sendMessage(msgData, msgLen, m_context.userData, m_context.userDataLen, m_context.srvAsyncDataFlag, m_context.srvAsyncDataFlagLen, dstServiceId, dstProtocolId, dstModuleId, m_moduleId, handleProtocolId, m_context.userFlag);
	if (rc != Success) ReleaseErrorLog("send service message error3, msgLen = %u, dstServiceId = %d, dstProtocolId = %d, rc = %d", msgLen, dstServiceId, dstProtocolId, rc);
	return rc;
}

// 向目标服务发送请求消息，默认自动发送上下文中存在的用户数据
// srcServiceProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话
int CModule::sendMessage(const char* msgData, const unsigned int msgLen, unsigned short srcServiceType, unsigned int srcServiceId, unsigned short srcServiceProtocolId,
	                     unsigned int dstServiceId, unsigned short dstProtocolId, unsigned short dstModuleId, unsigned int msgId)
{
	int rc = m_service->sendMessage(srcServiceType, srcServiceId, msgData, msgLen, m_context.userData, m_context.userDataLen, m_context.srvAsyncDataFlag, m_context.srvAsyncDataFlagLen, dstServiceId, dstProtocolId, dstModuleId, 0, srcServiceProtocolId, m_context.userFlag, msgId);
	if (rc != Success) ReleaseErrorLog("send service message error4, msgLen = %u, dstServiceId = %d, dstProtocolId = %d, rc = %d", msgLen, dstServiceId, dstProtocolId, rc);
	return rc;
}

// 定时器设置，返回定时器ID，返回 0 表示设置定时器失败
unsigned int CModule::setTimer(unsigned int interval, TimerHandler timerHandler, int userId, void* param, unsigned int count, CHandler* instance)
{
	if (instance == NULL) instance = this;
	return m_service->setTimer(instance, interval, timerHandler, userId, param, count);
}

void CModule::killTimer(unsigned int timerId)
{
	m_service->killTimer(timerId);
}

// 服务启动，做初始化工作
void CModule::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
}

// 服务停止运行，做去初始化工作
void CModule::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{         
}

// 模块开始运行
void CModule::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
}

// 收到网络客户端的消息				  
int CModule::onClientMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
							 unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn)
{
	return NoFoundProtocolHandler;
}

// 收到连接代理的数据				  
int CModule::onProxyMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
							unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, ConnectProxy* conn)
{
	return NoFoundProtocolHandler;
}

}

