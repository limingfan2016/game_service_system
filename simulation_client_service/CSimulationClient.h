
/* author : limingfan
 * date : 2015.09.21
 * description : 仿真客户端服务实现
 */

#ifndef CSIMULATION_CLIENT_H
#define CSIMULATION_CLIENT_H

#include "SrvFrame/IService.h"
#include "SrvFrame/CGameModule.h"
#include "base/CMemManager.h"
#include "common/CommonType.h"
#include "common/CMonitorStat.h"
#include "TypeDefine.h"


using namespace NFrame;

namespace NPlatformService
{

// 消息协议处理对象
class CSrvMsgHandler : public NFrame::CGameModule
{
public:
	CSrvMsgHandler();
	~CSrvMsgHandler();
	
	// 公共API
private:
	int sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userName, int uNLen,
	                               int userFlag = 0, unsigned short handleProtocolId = 0);
	int sendMessageToService(NFrame::ServiceType srvType, const char* data, const unsigned int len, unsigned short protocolId, const char* userName, const int uNLen,
							 int userFlag = 0, unsigned short handleProtocolId = 0);
	bool serializeMsgToBuffer(::google::protobuf::Message& msg, char* buffer, const unsigned int len, const char* info);
	const char* serializeMsgToBuffer(::google::protobuf::Message& msg, const char* info);
	bool parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info);

public:
	void onClosedConnect(void* userData);   // 通知逻辑层对应的逻辑连接已被关闭	

private:
	void registerRobot(const unsigned int robotType, const unsigned int num);
	void robotActiveLoginService(const char* service_ip, unsigned int service_port, unsigned int service_id, unsigned int robot_type, unsigned int table_id, unsigned int seat_id, unsigned int min_gold, unsigned int max_gold);
	void releaseRobot(RobotData* rData, bool isCloseConnect = true);
	void robotActiveLoginServiceCallBack(Connect* conn, const char* peerIp, const unsigned short peerPort, void* userCb, int userId);
	
	void changeBulletRate(unsigned int timerId, int userId, void* param, unsigned int remainCount);  // 定时变更子弹倍率
	void useProp(unsigned int timerId, int userId, void* param, unsigned int remainCount);  // 定时使用道具
    void doChat(unsigned int timerId, int userId, void* param, unsigned int remainCount);  // 机器人定时发言

private:
    // 收到的服务请求消息
	void robotLoginNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void robotLogoutNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	// 收到的CommonDB代理服务应答消息
    void queryUsernameReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void registerRobotReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// void clearRobotGoldReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void resetRobotInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	// 收到的游戏服务应答消息
    void enterServiceRoomReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void enterServiceTableReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void enterTableNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void leaveTableNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 直接忽略的消息，不需要做任何处理
	void dummyMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);


private:
    virtual void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);


    // 测试代码，测试接口
private:
    void testRobotLoginNotify(unsigned int table_id, unsigned int seat_id);
	void testRobotLogoutNotify(const char* username);


private:
	char m_msgBuffer[NFrame::MaxMsgLen];
	NProject::CMonitorStat m_monitorStat;
	
	RobotUserNameVector m_normalFreeRobots;  // 普通机器人
	RobotUserNameVector m_vipFreeRobots;     // VIP机器人
	unsigned int m_robotIdIdx;
	
	RobotLoginNotifyDataVector m_waitLoginData;
	NCommon::CMemManager m_memForRobot;       // 机器人内存块数据

	
DISABLE_COPY_ASSIGN(CSrvMsgHandler);
};


// 仿真客户端服务
class CSimulationClient : public NFrame::IService
{
public:
	CSimulationClient();
	~CSimulationClient();

private:
	virtual int onInit(const char* name, const unsigned int id);            // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);         // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);       // 服务启动后被调用，服务需在此注册本服务的各模块信息
	virtual void onUpdateConfig(const char* name, const unsigned int id);   // 服务配置更新

private:
	virtual void onClosedConnect(void* userData);   // 通知逻辑层对应的逻辑连接已被关闭

private:
    CSrvMsgHandler* m_connectNotifyToHandler;
	

DISABLE_COPY_ASSIGN(CSimulationClient);
};

}

#endif // CSIMULATION_CLIENT_H
