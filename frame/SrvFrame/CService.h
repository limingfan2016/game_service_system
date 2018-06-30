
/* author : limingfan
 * date : 2015.01.30
 * description : 服务开发API定义实现
 */

#ifndef CSERVICE_H
#define CSERVICE_H

#include <unordered_map>
#include "IService.h"
#include "FrameType.h"
#include "CNetMsgComm.h"
#include "CConnectMgr.h"
#include "base/MacroDefine.h"
#include "base/CMemManager.h"
#include "base/CMessageQueue.h"
#include "base/CTimer.h"
#include "messageComm/ISrvMsgComm.h"


namespace NFrame
{

class CHandler;
class CModule;
struct ServiceMsgHeader;

typedef std::unordered_map<unsigned int, void*> TimerIdToMsg;

// 定时器触发
class CTimerCallBack : public NCommon::CTimerI
{
public:
	virtual bool OnTimer(unsigned int timerId, void* pParam, unsigned int remainCount);
	
public:
	NCommon::CAddressQueue* pTimerMsgQueue;
};


// 服务框架基础功能
class CService
{
public:
	CService();
	~CService();
	
public:
    // 启动服务，初始化服务数据
    // cfgFile : 本服务的配置文件路径名
	// srvMsgCommCfgFile : 服务通信消息中间件配置文件路径名
	// 本服务名称
    int start(const char* cfgFile, const char* srvMsgCommCfgFile, const char* srvName);
	
	// 运行服务，开始收发处理消息
    void run();
	
	// 停止服务
	void stop();
	
	// 通知配置更新
	void notifyUpdateConfig();

public:
    // 向目标服务发送请求消息
	// handleProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话
    int sendMessage(const char* msgData, const unsigned int msgLen, const char* userData, const unsigned int userDataLen, const char* srvAsyncDataFlag, const unsigned int srvAsyncDataFlagLen,
	                unsigned int dstServiceId, unsigned short dstProtocolId, unsigned short dstModuleId = 0, unsigned short srcModuleId = 0,
					unsigned short handleProtocolId = 0, int userFlag = 0, unsigned int msgId = 0);
					
	// handleProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话（用于转发消息，替换源消息的服务类型及服务ID，使得目标服务直接回消息给源端而不是中转服务）
    int sendMessage(const unsigned short srcServiceType, const unsigned int srcServiceId, const char* msgData, const unsigned int msgLen, const char* userData, const unsigned int userDataLen,
					const char* srvAsyncDataFlag, const unsigned int srvAsyncDataFlagLen, unsigned int dstServiceId, unsigned short dstProtocolId, unsigned short dstModuleId = 0,
					unsigned short srcModuleId = 0, unsigned short handleProtocolId = 0, int userFlag = 0, unsigned int msgId = 0);
					
	// 向目标网络客户端发送消息
	int sendMsgToClient(const char* msgData, const unsigned int msgLen, unsigned int msgId, unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn);
						
	// 向目标网络客户端发送任意数据
	int sendDataToClient(Connect* conn, const char* data, const unsigned int len);
	
	// 向目标连接代理发送消息
	int sendMsgToProxy(const char* msgData, const unsigned int msgLen, unsigned int msgId, unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, ConnectProxy* conn);
	
public:
    // 主动关闭、停止连接代理
	void closeProxy(ConnectProxy* conn, bool isActive, int cbFlag = 0);                  // 服务关闭用户连接时调用
	void closeProxy(const uuid_type id, bool isActive, int cbFlag = 0);                  // 服务关闭用户连接时调用

	uuid_type getProxyId(ConnectProxy* connProxy);
	ConnectProxy* getProxy(const uuid_type id);
	
	void stopProxy();                                                           // 服务停止时调用
	void cleanUpProxy(const unsigned int proxyId[], const unsigned int len);    // 服务启动时调用

public:
    const char* getName();
	unsigned short getType();
	unsigned int getId();
	unsigned int getServiceId(const char* serviceName);
	
	int registerModule(unsigned short moduleId, CModule* pInstance);
	int registerNetModule(CNetDataHandler* pInstance);
	
	void setServiceType(unsigned int srvType);
    void setConnectClient();
	
public:	
    // 定时器设置，返回定时器ID，返回 0 表示设置定时器失败
	unsigned int setTimer(CHandler* handler, unsigned int interval, TimerHandler cbFunc, int userId = 0, void* param = NULL, unsigned int count = 1);
	void killTimer(unsigned int timerId);
	
public:
    // 本地端准备发起主动连接
    int doActiveConnect(CNetDataHandler* instance, ActiveConnectHandler cbFunction, const strIP_t peerIp, unsigned short peerPort, unsigned int timeOut, void* userCb = NULL, int userId = 0);

public:
	void createdClientConnect(Connect* conn, const char* peerIp, const unsigned short peerPort);
	ActiveConnect* getActiveConnectData();
	void doCreateServiceConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, ReturnValue rtVal, void*& cb);
	void closedClientConnect(Connect* conn);
	
private:
    void handleServiceMessage(ServiceMsgHeader* msgHeader, unsigned int msgLen);
	void handleClientMessage(const char* data, Connect* conn, unsigned int msgLen);
	void handleClientMessage(ClientMsgHeader* msgHeader, Connect* conn, unsigned int msgLen);
	bool handleTimerMessage();
	void clear();
	

private:
    bool m_isRunning;
	bool m_isNotifyUpdateConfig;
    const char* m_srvName;
	unsigned int m_srvId;
	unsigned short m_srvType;                                // 服务类型
	NMsgComm::ISrvMsgComm* m_srvMsgComm;
	char m_recvMsg[MaxMsgLen];
	char m_sndMsg[MaxMsgLen];
    CModule* m_moduleInstance[MaxModuleIDCount];             // 服务注册的模块实例
	CNetMsgComm* m_netMsgComm;                               // 外部客户端通信
	CConnectMgr m_connectMgr;                                // 连接管理对象
	
	unsigned int m_timerMsgMaxCount;                         // 支持设置的定时器最大个数，0则无限制
	CTimerCallBack m_timerMsgCb;                             // 定时器触发回调接口
	TimerIdToMsg m_timerIdToMsg;                             // kill定时器时删除定时器消息
	NCommon::CMemManager m_memForTimerMsg;                   // 定时器消息产生
	NCommon::CAddressQueue m_timerMsgQueue;                  // 定时器消息队列
	NCommon::CAddressQueue m_closedClientConnectQueue;       // 关闭网络客户端连接操作队列
	NCommon::CAddressQueue m_activeConnectDataQueue;         // 本地端主动发起建立连接的队列
	NCommon::CAddressQueue m_activeConnectResultQueue;       // 本地端主动发起建立连接的结果
	NCommon::CMemManager m_memForActiveConnect;              // 主动连接数据
	NCommon::CTimer m_timer;                                 // 定时器各种操作
	
DISABLE_COPY_ASSIGN(CService);
};

}

#endif // CSERVICE_H
