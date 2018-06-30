
/* author : limingfan
 * date : 2015.01.30
 * description : 服务开发模块API定义实现
 */

#ifndef CMODULE_H
#define CMODULE_H

#include <unordered_map>

#include "IService.h"
#include "CConnectMgr.h"
#include "base/MacroDefine.h"
#include "base/CCfg.h"


namespace NFrame
{

// 一个模块最多支持处理 MaxProtocolIDCount 个协议
// 注册的协议ID值不能大于等于 MaxProtocolIDCount
static const unsigned short MaxProtocolIDCount = 8192;

// 协议消息处理者
class CHandler;
typedef void (CHandler::*ProtocolHandler)(const char* msgData, const unsigned int msgLen, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
struct MsgHandler
{
	CHandler* instance;
	ProtocolHandler handler;
};

typedef std::unordered_map<unsigned int, MsgHandler> ProtocolIdToHandler;

typedef std::unordered_map<unsigned int, ProtocolIdToHandler> ServiceTypeToProtocolHandler;


// 定时器消息处理者函数
typedef void (CHandler::*TimerHandler)(unsigned int timerId, int userId, void* param, unsigned int remainCount);

class CHandler
{
public:
	CHandler();
	virtual ~CHandler();
	
private:
	// 定时器消息
	void onTimeOut(TimerHandler timerHandler, unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	friend class CService;
	
DISABLE_COPY_ASSIGN(CHandler);
};


// 服务消息上下文内容
struct Context
{
	// 消息源端
	unsigned int srcSrvId;
	unsigned short srcSrvType;
	unsigned short srcModuleId;
	unsigned short srcProtocolId;
	
	// 消息目标端
	unsigned int dstSrvId;
	unsigned short dstModuleId;
	unsigned short dstProtocolId;
	
	// 用户数据
	char userData[MaxUserDataLen + 1];
	unsigned int userDataLen;
	int userFlag;
	
	// 异步数据标识
	char srvAsyncDataFlag[MaxLocalAsyncDataFlagLen + 1];
	unsigned int srvAsyncDataFlagLen;
};


class CService;

// 模块开发者接口
class CModule : public CHandler
{
public:
	CModule();
	virtual ~CModule();
	
public:
    // 本服务信息
	const char* getSrvName();
	unsigned short getSrvType();
	unsigned int getSrvId();
	unsigned short getModuleId();
	
	// 停止退出服务
    void stopService();
	
    // 获取服务ID，主要用于发送服务消息
    unsigned int getServiceId(const char* serviceName);

	// 当前消息上下文内容
	const Context& getContext();
	
	// 注册处理协议的函数
    int registerProtocol(unsigned int srcSrvType, unsigned short protocolId, ProtocolHandler handler, CHandler* instance = NULL);
	
	// 向目标服务发送请求消息
	// handleProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话
    int sendMessage(const char* msgData, const unsigned int msgLen, const char* userData, const unsigned int userDataLen,
	                unsigned int dstServiceId, unsigned short dstProtocolId, unsigned short dstModuleId = 0, unsigned short handleProtocolId = 0);
					
	// 向目标服务发送请求消息
	// handleProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话
    int sendMessage(const char* msgData, const unsigned int msgLen, int userFlag, const char* userData, const unsigned int userDataLen,
	                unsigned int dstServiceId, unsigned short dstProtocolId,
					unsigned short dstModuleId = 0, unsigned short handleProtocolId = 0, unsigned int msgId = 0);
					
	// 向目标服务发送请求消息，默认自动发送上下文中存在的用户数据
	// handleProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话
    int sendMessage(const char* msgData, const unsigned int msgLen, unsigned int dstServiceId, unsigned short dstProtocolId,
					unsigned short dstModuleId = 0, unsigned short handleProtocolId = 0);
					
    // 向目标服务发送请求消息，默认自动发送上下文中存在的用户数据
	// srcServiceProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话
    int sendMessage(const char* msgData, const unsigned int msgLen, unsigned short srcServiceType, unsigned int srcServiceId, unsigned short srcServiceProtocolId,
	                unsigned int dstServiceId, unsigned short dstProtocolId, unsigned short dstModuleId = 0, unsigned int msgId = 0);

public:
    // 定时器设置，返回定时器ID，返回 0 表示设置定时器失败
	unsigned int setTimer(unsigned int interval, TimerHandler timerHandler, int userId = 0, void* param = NULL, unsigned int count = 1, CHandler* instance = NULL);
	void killTimer(unsigned int timerId);

    // 服务开发者实现的接口
private:
    virtual void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);             // 服务启动，做初始化工作
	virtual void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);           // 服务停止运行，做去初始化工作
	virtual void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId) = 0;     // 在此注册本模块处理的协议函数，绑定协议ID到实现函数
	virtual void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);              // 模块开始运行

	
private:
    // 加载模块			   
	void doLoad(unsigned short moduleId, const char* srvName, const unsigned int srvId, CConnectMgr* connectMgr, NCommon::CCfg* srvMsgCommCfg);
	
    // 服务模块收到消息				
	virtual int onServiceMessage(const char* msgData, const unsigned int msgLen, const char* userData, const unsigned int userDataLen,
						         unsigned short dstProtocolId, unsigned int srcSrvId, unsigned short srcSrvType, unsigned short srcModuleId,
						         unsigned short srcProtocolId, int userFlag, unsigned int msgId, const char* srvAsyncDataFlag, unsigned int srvAsyncDataFlagLen);
	
	// 收到网络客户端的消息				  
	virtual int onClientMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
								unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn);
								
    // 收到连接代理的数据
	virtual int onProxyMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
							   unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, ConnectProxy* conn);


protected:
    ServiceTypeToProtocolHandler m_protocolHanders;
	
	CService* m_service;
	CConnectMgr* m_connectMgr;
	MessageType m_msgType;

	CCfg* m_srvMsgCommCfg;  // 服务间消息通信配置信息

private:
    ProtocolIdToHandler m_serviceProtocolHandler;
	
    unsigned short m_moduleId;
	Context m_context;


    friend class CService;

DISABLE_COPY_ASSIGN(CModule);
};

}

#endif // CMODULE_H
