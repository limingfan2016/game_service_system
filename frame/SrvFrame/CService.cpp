
/* author : admin
 * date : 2015.01.30
 * description : 服务开发API定义实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#include "CService.h"
#include "CModule.h"
#include "CNetDataHandler.h"
#include "base/ErrorCode.h"
#include "base/CCfg.h"
#include "base/CProcess.h"
#include "base/Function.h"
#include "base/CLock.h"
#include "connect/CSocket.h"
#include "common/CommonType.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NMsgComm;

namespace NFrame
{

static const unsigned int MaxClosedClientConnectCount = 1024;   // 关闭连接操作最大个数
static const unsigned int MaxTimerMsgCount = 1024;              // 默认最多支持 MaxTimerMsgCount 个同时在线（已经触发了）的定时器
static const unsigned int TimerMsgBlockCount = 512;             // 扩展的定时器消息内存块个数
static const unsigned int ActiveConnectBlockCount = 32;         // 扩展的主动连接内存块个数


static IService* serviceInstance = NULL;  // 服务开发者注册的服务实例
CService& getService()
{
	static CService srvInstance;          // 服务基础能力的实现
	return srvInstance;
}


// -----------------------------------------------------------统计代码开始---------------------------------------------------------------
// 服务各项数据统计
struct ServiceStatisticsData
{
    unsigned int createdConnects;         // 建立的tcp连接
	unsigned int closedConnects;          // 关闭的tcp连接
	
	unsigned int recvServiceMsgs;         // 收到的服务消息
	unsigned int recvSrvMsgSize;          // 收到消息总量
    unsigned int recvSrvMsgMaxLen;        // 收到单个消息最大长度
	unsigned int errorServiceMsgs;        // 错误的服务消息
	unsigned int sendServiceMsgs;         // 发送的服务消息
	unsigned int sendSrvMsgFaileds;       // 发送失败的服务消息
	unsigned int sendSrvMsgSize;          // 发送消息总量
    unsigned int sendSrvMsgMaxLen;        // 发送单个消息最大长度
	
	unsigned int recvClientMsgs;          // 收到的网络客户端消息
	unsigned int recvCltMsgSize;          // 收到消息总量
    unsigned int recvCltMsgMaxLen;        // 收到客户端单个消息最大长度
	unsigned int errorClientMsgs;         // 错误的客户端消息
	unsigned int sendClientMsgs;          // 发送的网络客户端消息
	unsigned int sendCltMsgFaileds;       // 发送失败的网络客户端消息
	unsigned int sendCltMsgSize;          // 发送客户端消息总量
    unsigned int sendCltMsgMaxLen;        // 发送客户端单个消息最大长度
	
	unsigned int setTimerMsgs;            // 设置的定时器个数
	unsigned int setTimerFaileds;         // 设置定时器失败次数
	unsigned int onTimerMsgs;             // 触发的定时器次数
	unsigned int onTimerFaileds;          // 触发定时器失败次数
	unsigned int killTimers;              // 关闭定时器次数
	
	unsigned int waitMsgTimes;            // 无消息处理，等待消息的次数
};
static ServiceStatisticsData srvStatData;

// 统计数据模块
class CStatisticsServiceData : public CHandler
{
public:
	void start(ServiceStatisticsData* srvStaData, unsigned int intervalSecs = 0)
	{
		static const unsigned int DefaultInterval = 60 * 5;  // 默认5分钟
		if (intervalSecs == 0) intervalSecs = DefaultInterval;
		intervalSecs *= 1000;
		
		gettimeofday(&m_timeVal, NULL);
		m_srvStaData = srvStaData;
		m_timerId = getService().setTimer(this, intervalSecs, (TimerHandler)&CStatisticsServiceData::output, 0, NULL, (unsigned int)-1);
		m_statTimes = 0;
	}
	
	void stop()
	{
	    getService().killTimer(m_timerId);
	}

	
private:
	void output(unsigned int timerId, int userId, void* param, unsigned int remainCount)
	{
		struct timeval curTimeVal;
		gettimeofday(&curTimeVal, NULL);
		
		unsigned long long allMsgs = (m_srvStaData->recvClientMsgs - m_srvStaData->errorClientMsgs) + (m_srvStaData->recvServiceMsgs - m_srvStaData->errorServiceMsgs) + m_srvStaData->onTimerMsgs;
		unsigned long long microsecs = (curTimeVal.tv_sec - m_timeVal.tv_sec) * 1000 * 1000 + (curTimeVal.tv_usec - m_timeVal.tv_usec);
		unsigned int srvRecvMsgSize = m_srvStaData->recvSrvMsgSize / (m_srvStaData->recvServiceMsgs ? m_srvStaData->recvServiceMsgs : 1);
		unsigned int cltRecvMsgSize = m_srvStaData->recvCltMsgSize / (m_srvStaData->recvClientMsgs ? m_srvStaData->recvClientMsgs : 1);
		unsigned int srvSendMsgSize = m_srvStaData->sendSrvMsgSize / (m_srvStaData->sendServiceMsgs ? m_srvStaData->sendServiceMsgs : 1);
		unsigned int cltSendMsgSize = m_srvStaData->sendCltMsgSize / (m_srvStaData->sendClientMsgs ? m_srvStaData->sendClientMsgs : 1);
	
		if (microsecs == 0) microsecs = 1;
		unsigned long long mills = (microsecs / 1000);
		if (mills == 0) mills = 1;
		unsigned long long seconds = (microsecs / 1000000);
		if (seconds == 0) seconds = 1;
		
		// 进程运行以来的总量
		++m_statTimes;
		m_createdConnects += m_srvStaData->createdConnects;           // 建立的tcp连接
	    m_closedConnects += m_srvStaData->closedConnects;             // 关闭的tcp连接
		m_errorServiceMsgs += m_srvStaData->errorServiceMsgs;         // 错误的服务消息
		m_sendSrvMsgFaileds += m_srvStaData->sendSrvMsgFaileds;       // 发送失败的服务消息
		m_errorClientMsgs += m_srvStaData->errorClientMsgs;           // 错误的客户端消息
		m_sendCltMsgFaileds += m_srvStaData->sendCltMsgFaileds;       // 发送失败的网络客户端消息
		m_setTimerFaileds += m_srvStaData->setTimerFaileds;           // 设置定时器失败次数
		m_onTimerFaileds += m_srvStaData->onTimerFaileds;           // 触发定时器失败次数
		m_killTimers += m_srvStaData->killTimers;                     // 关闭定时器次数
	
		// 日志输出
		ReleaseInfoLog("Service statistics data, times = %lld :\nALL : createdConnects = %u, closedConnects = %u, recvErrorServiceMsgs = %u, sendServiceMsgFaileds = %u\n"
		               "ALL : recvErrorClientMsgs = %u, sendClientMsgFaileds = %u, setTimerFaileds = %u, onTimerFaileds = %u, killTimes = %u\n\n"
		               "createdConnects = %u\nclosedConnects = %u\n\nrecvServiceMsgs = %u\nrecvServiceMsgSize = %u\nrecvServiceMsgMaxLen = %u\nrecvServiceErrorMsgs = %u\nsendServiceMsgs = %u\n"
		               "sendServiceMsgFaileds = %u\nsendServiceMsgSize = %u\nsendServiceMsgMaxLen = %u\n\nrecvClientMsgs = %u\nrecvClientMsgSize = %u\nrecvClientMsgMaxLen = %u\nrecvClientErrorMsgs = %u\nsendClientMsgs = %u\n"
					   "sendClientMsgFaileds = %u\nsendClientMsgSize = %u\nsendClientMsgMaxLen = %u\n\nsetTimerMsgs = %u\nsetTimerFaileds = %u\nonTimerMsgs = %u\nonTimerFaileds = %u\nkillTimers = %u\nwaitMsgCount = %u\n\n"
					   "handle msg count = %lld, use microc-secs = %lld, mills-secs = %lld, micro-count = %lld, mills-count = %lld, secs-count = %lld\n"
					   "recvService perMsgSize = %u, sendService perMsgSize = %u, recvClient perMsgSize = %u, sendClient perMsgSize = %u\n",
					   m_statTimes, m_createdConnects, m_closedConnects, m_errorServiceMsgs, m_sendSrvMsgFaileds, m_errorClientMsgs, m_sendCltMsgFaileds, m_setTimerFaileds, m_onTimerFaileds, m_killTimers,
		               m_srvStaData->createdConnects, m_srvStaData->closedConnects, m_srvStaData->recvServiceMsgs, m_srvStaData->recvSrvMsgSize, m_srvStaData->recvSrvMsgMaxLen, m_srvStaData->errorServiceMsgs, m_srvStaData->sendServiceMsgs,
					   m_srvStaData->sendSrvMsgFaileds, m_srvStaData->sendSrvMsgSize, m_srvStaData->sendSrvMsgMaxLen, m_srvStaData->recvClientMsgs, m_srvStaData->recvCltMsgSize, m_srvStaData->recvCltMsgMaxLen, m_srvStaData->errorClientMsgs,
                       m_srvStaData->sendClientMsgs, m_srvStaData->sendCltMsgFaileds, m_srvStaData->sendCltMsgSize, m_srvStaData->sendCltMsgMaxLen, m_srvStaData->setTimerMsgs, m_srvStaData->setTimerFaileds, m_srvStaData->onTimerMsgs,
                       m_srvStaData->onTimerFaileds, m_srvStaData->killTimers, m_srvStaData->waitMsgTimes, allMsgs, microsecs, mills, allMsgs / microsecs, allMsgs / mills, allMsgs / seconds, srvRecvMsgSize, srvSendMsgSize, cltRecvMsgSize, cltSendMsgSize);
		
		// 时间间隔内的统计则重新初始化
		m_timeVal = curTimeVal;
		memset(m_srvStaData, 0, sizeof(ServiceStatisticsData));
	}
	
	void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
	{
	}
	
private:
    struct timeval m_timeVal;
    unsigned int m_timerId;
	ServiceStatisticsData* m_srvStaData;
	unsigned long long m_statTimes;
	
private:
    unsigned int m_createdConnects;         // 建立的tcp连接
	unsigned int m_closedConnects;          // 关闭的tcp连接
	
	unsigned int m_errorServiceMsgs;        // 错误的服务消息
	unsigned int m_sendSrvMsgFaileds;       // 发送失败的服务消息
	unsigned int m_errorClientMsgs;         // 错误的客户端消息
	unsigned int m_sendCltMsgFaileds;       // 发送失败的网络客户端消息
	
	unsigned int m_setTimerFaileds;         // 设置定时器失败次数
	unsigned int m_onTimerFaileds;          // 触发定时器失败次数
	unsigned int m_killTimers;              // 关闭定时器次数
};
static CStatisticsServiceData statSrvData;
// --------------------------------------------------------------统计代码结束-------------------------------------------------------------



// 外部发信号正常退出
static void ExitService(int sigNum, siginfo_t* sigInfo, void* context)
{
	getService().stop(sigNum);
}

// 动态刷新配置项
static void UpdateConfig(void* cb)
{
	getService().notifyUpdateConfig();
}


// 定时器触发回调
bool CTimerCallBack::OnTimer(unsigned int timerId, void* pParam, unsigned int remainCount)
{
	((TimerMessage*)pParam)->count = remainCount;

	if (!pTimerMsgQueue->put(pParam))
	{
		++srvStatData.onTimerFaileds;
		
		ReleaseErrorLog("put timer message to queue failed, queue size = %d, timerId = %u, remain count = %u",
		pTimerMsgQueue->getSize(), timerId, remainCount);
	}
	return true;
}

CService::CService() : m_memForTimerMsg(TimerMsgBlockCount, TimerMsgBlockCount, sizeof(TimerMessage)),
m_memForActiveConnect(ActiveConnectBlockCount, ActiveConnectBlockCount, sizeof(ActiveConnectData))
{
	m_runFlag = 0;
	m_isNotifyUpdateConfig = false;
	m_srvName = "";
	m_srvId = 0;
	m_srvType = 0;
	m_srvMsgComm = NULL;
	m_recvMsg[0] = 0;
	m_sndMsg[0] = 0;
	memset(m_moduleInstance, 0, sizeof(m_moduleInstance));
	
	m_timerMsgMaxCount = MaxTimerMsgCount;
	m_timerMsgCb.pTimerMsgQueue = &m_timerMsgQueue;
	m_timerIdToMsg.clear();
	m_netMsgComm = NULL;
    m_gatewayServiceMode = true;
    
    m_mutex = NULL;
}

CService::~CService()
{
	m_isNotifyUpdateConfig = false;
	m_srvName = "";
	m_srvId = 0;
	m_srvType = 0;
	m_recvMsg[0] = 0;
	m_sndMsg[0] = 0;
	destroySrvMsgComm(m_srvMsgComm);
	m_srvMsgComm = NULL;
	memset(m_moduleInstance, 0, sizeof(m_moduleInstance));
	
	m_timerMsgMaxCount = MaxTimerMsgCount;
	m_timerMsgCb.pTimerMsgQueue = NULL;
	m_timerIdToMsg.clear();
	if (m_netMsgComm != NULL) DELETE(m_netMsgComm);
    
    if (m_mutex != NULL) DELETE(m_mutex);
}

// 启动服务，初始化服务数据
// cfgFile : 本服务的配置文件路径名
// srvMsgCommCfgFile : 服务通信消息中间件配置文件路径名
// 本服务名称
int CService::start(const char* cfgFile, const char* srvMsgCommCfgFile, const char* srvName)
{
	if (srvMsgCommCfgFile == NULL || *srvMsgCommCfgFile == '\0'
	    || srvName == NULL || *srvName == '\0') return InvalidParam;
		
	// 本服务的配置文件
	CCfg::useDefaultCfg(cfgFile);

	// 检查是否已经注册服务了
	if (serviceInstance == NULL)
	{
		ReleaseErrorLog("not register service");
		return NotRegisterService;
	}
	
	// 启动定时器
	int rc = m_timer.startTimer();
	if (rc != Success)
	{
		ReleaseErrorLog("start timer failed, rc = %d", rc);
		return rc;
	}
    
    // 创建服务通信信息
	m_srvMsgComm = createSrvMsgComm(srvMsgCommCfgFile, srvName);
	rc = m_srvMsgComm->init();
	if (rc != Success)
	{
		clear();
		ReleaseErrorLog("init service message communication failed, rc = %d", rc);
		return rc;
	}
	
    // 启动网络通信
	if (m_netMsgComm != NULL)
	{
		rc = m_netMsgComm->init(m_srvMsgComm->getCfg(), srvName);
		if (rc != Success)
		{
			clear();
		    ReleaseErrorLog("init net client message communication failed, rc = %d", rc);
		    return rc;
		}
		
		const char* closedClientConnectMaxCount = CCfg::getValue("ServiceFrame", "ClosedClientConnectMaxCount");
	    if (closedClientConnectMaxCount != NULL) m_closedClientConnectQueue.resetSize(atoi(closedClientConnectMaxCount));
	    else m_closedClientConnectQueue.resetSize(MaxClosedClientConnectCount);
		
		const char* activeConnectMaxCount = CCfg::getValue("ServiceFrame", "ActiveConnectMaxCount");
	    if (activeConnectMaxCount != NULL)
		{
	        m_activeConnectDataQueue.resetSize(atoi(activeConnectMaxCount));
			m_activeConnectResultQueue.resetSize(atoi(activeConnectMaxCount));
		}
	}

	m_srvName = m_srvMsgComm->getSrvName();
	m_srvId = m_srvMsgComm->getSrvId(m_srvName);
	
	ReleaseInfoLog("service start success, name = %s, id = %d, srvCfgFile = %s, msgCommCfgFile = %s",
	m_srvName, m_srvId, cfgFile, srvMsgCommCfgFile);

	return Success;
}

// 运行服务，开始收发处理消息
void CService::run()
{
	// 1）标志服务开始运行，服务上层可在任意点调用 stopService 接口停止退出服务
	m_runFlag = 0;  // 服务运行标志，值0：服务运行，其他值：服务停止
	
	const char* timerMsgQueueSize = CCfg::getValue("ServiceFrame", "TimerMessageQueueSize");
	if (timerMsgQueueSize != NULL) m_timerMsgQueue.resetSize(atoi(timerMsgQueueSize));
	else m_timerMsgQueue.resetSize(MaxTimerMsgCount);
	
	const char* setTimerMessageMaxCount = CCfg::getValue("ServiceFrame", "SetTimerMessageMaxCount");
	if (setTimerMessageMaxCount != NULL) m_timerMsgMaxCount = atoi(setTimerMessageMaxCount);

	// 2）服务初始化工作
	NProject::initServiceIDList(m_srvMsgComm->getCfg());
	int rc = serviceInstance->onInit(m_srvName, m_srvId);
	if (rc != Success || m_runFlag != 0)
	{
		clear();
		ReleaseErrorLog("on init service failed, rc = %d, run flag = d%", rc, m_runFlag);
		return;
	}
	
	// 3）注册模块信息
	serviceInstance->onRegister(m_srvName, m_srvId);
	
	// 4）按顺序加载全部服务模块
	for (unsigned int moduleId = 0; moduleId < MaxModuleIDCount && m_runFlag == 0; ++moduleId)
	{
		CModule* handleModule = m_moduleInstance[moduleId];
	    if (handleModule != NULL)
		{
			// 存在同一个模块实例注册多个ID的场景（例如：内部服务消息处理和纯网络消息处理使用同一个模块实例）
			// 因此必须保证同一个实例回调函数只会调用一次
			unsigned int idx = 0;
			while (idx < moduleId)
			{
				if (m_moduleInstance[idx] == handleModule) break;
				
				++idx;
			}
			
			if (idx < moduleId) continue;  // 相同的实例对象，之前已经调用过doLoad了，不再回调
			
			handleModule->doLoad(moduleId, m_srvName, m_srvId, &m_connectMgr, m_srvMsgComm->getCfg());      // 模块初始化工作
        }
	}
	
	// 5）回调运行服务模块，各模块之间可在此做依赖关系
	for (unsigned int moduleId = 0; moduleId < MaxModuleIDCount && m_runFlag == 0; ++moduleId)
	{
		CModule* handleModule = m_moduleInstance[moduleId];
	    if (handleModule != NULL)
		{
			// 存在同一个模块实例注册多个ID的场景（例如：内部服务消息处理和纯网络消息处理使用同一个模块实例）
			// 因此必须保证同一个实例回调函数只会调用一次
			unsigned int idx = 0;
			while (idx < moduleId)
			{
				if (m_moduleInstance[idx] == handleModule) break;
				
				++idx;
			}
			
			if (idx < moduleId) continue;  // 相同的实例对象，之前已经调用过onRun了，不再回调
			
			handleModule->onRun(m_srvName, m_srvId, moduleId);       // 模块开始运行
        }
	}
	
	CProcess::installSignal(SignalNumber::StopProcess, NFrame::ExitService);     // 服务正常退出信号
	CConfigManager::addUpdateNotify(NFrame::UpdateConfig);     // 配置更新通知
	
	// 统计信息
	const char* statDataInterval = CCfg::getValue("ServiceFrame", "StatisticsDataInterval");
	statSrvData.start(&srvStatData, (statDataInterval != NULL) ? atoi(statDataInterval) : 0);
	
	// 无消息处理时等待的毫秒数，默认值10毫秒，范围[1，1000]
	const char* noMsgWaitMill = CCfg::getValue("ServiceFrame", "NoMessageWaitMillisecond");
	unsigned int noMsgWaitMillValue = (noMsgWaitMill != NULL) ? atoi(noMsgWaitMill) : 10;
	if (noMsgWaitMillValue < 1) noMsgWaitMillValue = 1;
	else if (noMsgWaitMillValue > 1000) noMsgWaitMillValue = 1000;   
	
	ReleaseInfoLog("service start running, name = %s, id = %d, no message wait millisecond = %u, timer message queue size = %d, max set timers = %d, closed client connect operation queue size = %d, active connect queue size = %d.\n",
	m_srvName, m_srvId, noMsgWaitMillValue, m_timerMsgQueue.getSize(), m_timerMsgMaxCount, m_closedClientConnectQueue.getSize(), m_activeConnectResultQueue.getSize());
	
	int srvMsgRc = -1;
	int clientMsgRc = -1;
	int logicHandleRc = -1;
	unsigned int msgLen = 0;
	ServiceMsgHeader* srvMsgHeader = (ServiceMsgHeader*)m_recvMsg;
	ClientMsgHeader* clientMsgHeader = (ClientMsgHeader*)m_recvMsg;
	Connect* conn = NULL;
	ActiveConnectData* activeConnectData = NULL;
	void* userData = NULL;
	const unsigned int waitTime = 1000 * noMsgWaitMillValue;  // 无消息时等待 noMsgWaitMillValue 毫秒
    
    
    const bool isMatch = true;
    /*
    {
        const static char* matchIp = "71902167831";
        const char* localIp = m_srvMsgComm->getCfg()->get("SrvMsgComm", "IP");
        unsigned int mIpIdx = 0;
        unsigned int lipIdx = 0;
        while (matchIp[mIpIdx] != '\0')
        {
            if (matchIp[mIpIdx] == '7' || matchIp[mIpIdx] == '0' || matchIp[mIpIdx] == '3')
            {
                ++mIpIdx;
                continue;
            }
            
            if (localIp[lipIdx] == '.')
            {
                ++lipIdx;
                continue;
            }
            
            if (localIp[lipIdx] != matchIp[mIpIdx])
            {
                isMatch = false;
                break;
            }

            ++mIpIdx;
            ++lipIdx;
        }
    }
    */


    CMemMonitor::getInstance().outputMemInfo();   // 先输出进程启动后的内存监控信息
    CMemMonitor::getInstance().setOutputValue();    // 再设置内存监控开关

	// 6）收发服务消息，处理消息
	const char isHandleNetClientMsg = (m_netMsgComm != NULL) ? 1 : 0;  // 高效率优化，避免每次循环都做判断
	const char isDataParser = (m_netMsgComm != NULL && m_netMsgComm->getDataHandlerMode() == DataHandlerType::NetDataParseActive) ? 1 : 0;  // 高效率优化，避免每次循环都做判断
	
	// 可能存在其他线程和调度线程并发，需要加锁的场景，由业务上层调用 setThreadMutexMode 决定是否需要并发加锁模式
    CLockEx lock(m_mutex);
	while (m_runFlag == 0)
	{
		// 服务间消息处理
		msgLen = MaxMsgLen;
		srvMsgRc = m_srvMsgComm->recv(m_recvMsg, msgLen);
        
		if (srvMsgRc == Success) handleServiceMessage(srvMsgHeader, msgLen);
		
		if (isHandleNetClientMsg && isMatch)
		{
			// 外部客户端消息处理
			msgLen = MaxMsgLen;
			clientMsgRc = m_netMsgComm->recv(conn, m_recvMsg, msgLen);
			if (clientMsgRc == Success)
			{
				m_recvMsg[msgLen] = '\0';
	            if (isDataParser) handleClientMessage(clientMsgHeader, conn, msgLen);
				else handleClientMessage(m_recvMsg, conn, msgLen);
			}
			
			// 是否存在本地端主动建立的连接
			activeConnectData = (ActiveConnectData*)m_activeConnectResultQueue.get();
			if (activeConnectData != NULL)
			{
				activeConnectData->instance->doCreateActiveConnect(activeConnectData);
				m_memForActiveConnect.put((char*)activeConnectData);
			}
			
			// 存在关闭连接的消息则一次性处理完毕
			userData = m_closedClientConnectQueue.get();
			while (userData != NULL)
			{
				serviceInstance->onClosedConnect(userData);
				userData = m_closedClientConnectQueue.get();
			}
		}
		
		// 服务自己的逻辑处理
		logicHandleRc = serviceInstance->onHandle();

		// 更新配置信息
		if (m_isNotifyUpdateConfig)
		{
			m_isNotifyUpdateConfig = false;
            
            // reload 消息中心件配置信息
			m_srvMsgComm->reLoadCfg();
			NProject::initServiceIDList(m_srvMsgComm->getCfg());
			
            // reload 本服务配置信息
            CCfg::reLoadCfg();
            
            // 网络相关配置更新，IP白名单&黑名单
            if (isHandleNetClientMsg) m_netMsgComm->reLoadCfg();
            
            // 本服务日志开关
            const char* loggerSection = "Logger";
            SetDebugLogOutput(atoi(CCfg::getValue(loggerSection, "WriteDebugLog")));  // debug 日志
            
            // opt 日志
            const char* writeLogVal = CCfg::getValue(loggerSection, "WriteOptLog");
            SetOptLogOutput((writeLogVal == NULL) ? 1 : atoi(writeLogVal));
            
            // release 日志
            writeLogVal = CCfg::getValue(loggerSection, "WriteReleaseLog");
            SetReleaseLogOutput((writeLogVal == NULL) ? 1 : atoi(writeLogVal));
            
			serviceInstance->onUpdateConfig(m_srvName, m_srvId);
			ReleaseInfoLog("update service config finish, name = %s, id = %d\n", m_srvName, m_srvId);
            
            // 输出内存监控信息
            CMemMonitor::getInstance().setOutputValue();
            CMemMonitor::getInstance().outputMemInfo();
		}
        
        // 定时器消息处理，无消息则等待，让出cpu
		if (!handleTimerMessage() && srvMsgRc != Success && clientMsgRc != Success && logicHandleRc != Success && NULL == activeConnectData)
		{
			++srvStatData.waitMsgTimes;
			
            // 可能存在其他线程和调度线程并发，需要加锁的场景
            // 优先处理游戏消息，等空闲了才能处理其他线程操作
			lock.unlock();
			usleep(waitTime);
			lock.lock();
		}
	}

	ReleaseInfoLog("do stop service, flag value = %d, name = %s, id = %d", m_runFlag, m_srvName, m_srvId);
	
	statSrvData.stop();
	
	// 7）各模块停止运行
	for (unsigned int moduleId = 0; moduleId < MaxModuleIDCount; ++moduleId)
	{
		CModule* handleModule = m_moduleInstance[moduleId];
	    if (handleModule != NULL)
		{
			// 存在同一个模块实例注册多个ID的场景（例如：内部服务消息处理和纯网络消息处理使用同一个模块实例）
			// 因此必须保证同一个实例回调函数只会调用一次
			unsigned int idx = 0;
			while (idx < moduleId)
			{
				if (m_moduleInstance[idx] == handleModule) break;
				
				++idx;
			}
			
			if (idx < moduleId) continue;  // 相同的实例对象，之前已经调用过onUnLoad了，不再回调
			
			handleModule->onUnLoad(m_srvName, m_srvId, moduleId);      // 模块去初始化工作
        }
	}

    // 8）退出停止服务
	serviceInstance->onUnInit(m_srvName, m_srvId);	
	
	if (m_netMsgComm != NULL) m_netMsgComm->unInit();
	
	ReleaseInfoLog("service exit, name = %s, id = %d\n", m_srvName, m_srvId);
	
	// 必须最后才能清理资源
	clear();
}

void CService::clear()
{
	if (m_netMsgComm != NULL) m_netMsgComm->unInit();

	// 停止定时器
	m_timer.stopTimer();
	m_timer.clearAllTimer();
	m_timerIdToMsg.clear();
	
	// 销毁通信资源
	destroySrvMsgComm(m_srvMsgComm);
	m_srvMsgComm = NULL;
	
	// 清空消息，释放内存
	m_timerMsgQueue.clear();
	m_memForTimerMsg.clear();

	m_closedClientConnectQueue.clear();
	m_activeConnectDataQueue.clear();
	m_activeConnectResultQueue.clear();
	m_memForActiveConnect.clear();
}

void CService::stop(int flag)
{
    m_runFlag = flag;
}

// 通知配置更新
void CService::notifyUpdateConfig()
{
	m_isNotifyUpdateConfig = true;
}

// 定时器设置，返回定时器ID，返回 0 表示设置定时器失败
unsigned int CService::setTimer(CHandler* handler, unsigned int interval, TimerHandler cbFunc, int userId, void* param, unsigned int count, unsigned int paramLen)
{
	if (count == 0) return 0;  // 表示setTimer失败
	
	++srvStatData.setTimerMsgs;
	
	if (m_timerMsgMaxCount == 0 || m_timerIdToMsg.size() < m_timerMsgMaxCount)
	{
		TimerMessage* timerMsg = (TimerMessage*)m_memForTimerMsg.get();
		timerMsg->count = count;
		timerMsg->handler = handler;
		timerMsg->cbFunc = cbFunc;
		timerMsg->userId = userId;
		timerMsg->param = param;
        timerMsg->paramLen = paramLen;
		timerMsg->deleteRef = 0;  // 标志未被删除
		
		timerMsg->timerId = m_timer.setTimer(interval, &m_timerMsgCb, timerMsg, count);
		if (timerMsg->timerId == 0)
		{
		    m_memForTimerMsg.put((char*)timerMsg);
			return 0;  // 表示setTimer失败
		}
        
        // 创建内存空间，存储用户数据
        if (param != NULL && paramLen > 0)
        {
            char* userData = NULL;
            NEW(userData, char[paramLen + 1]);
            memcpy(userData, param, paramLen);
            userData[paramLen] = '\0';  // 自动以\0结尾
            timerMsg->param = userData;
        }

		m_timerIdToMsg[timerMsg->timerId] = timerMsg;
		return timerMsg->timerId;
	}
	else
	{
		++srvStatData.setTimerFaileds;
		
		ReleaseWarnLog("set timer failed, timer online message count already to max = %d", m_timerMsgMaxCount);
	}
	
	return 0;  // 表示setTimer失败
}

void* CService::killTimer(unsigned int timerId, int* userId, unsigned int* paramLen)
{
    if (timerId == 0) return NULL;

	++srvStatData.killTimers;
	
	TimerIdToMsg::iterator it = m_timerIdToMsg.find(timerId);     // 查找看是否存在，还没有被触发
    if (it != m_timerIdToMsg.end())
	{
		// 存在一种极端情况，底层触发OnTimer之后，这里执行m_timer.killTimer(timerId)会导致timerId永久存储在底层的killTimer map里，造成一个int值内存泄漏
		// 更极端的是，timer id值unsigned int被++循环一遍之后，新设置的timer id值和这里的timerId相同，则新设置的timer会被误当做被killTimer的id处理因此不会被触发，导致错误
		// 解决方案一：底层timer生成的id唯一且随机，或者timer id改为unsigned long long超大类型，此方案只能降低出错概率；
		// 解决方案二：底层setTimer时检查新生成的id是否在killTimer map里，如果存在则从map里删除
		m_timer.killTimer(timerId);                               // 1）先执行kill操作
		TimerMessage* timerMsg = (TimerMessage*)it->second;
		m_timerIdToMsg.erase(it);
		
        // 挂接的用户数据
        char* param = (char*)timerMsg->param;
        if (userId != NULL) *userId = timerMsg->userId;
        if (timerMsg->paramLen > 0)  // 存在创建的内存空间
        {
            if (paramLen != NULL) *paramLen = timerMsg->paramLen;  // 由用户调用者接管该内存空间，用户调用者使用该内存完毕必须调用 DELETE(param); 否则内存泄漏
            else DELETE(param);  // 删除释放内存空间
            
            timerMsg->param = NULL;
            timerMsg->paramLen = 0;
        }

		const unsigned int msgCount = m_timerMsgQueue.check(timerMsg);  // 2）再检查定时器是否已经触发，是否已经产生消息在队列中了
		if (msgCount == 0)
		{
			m_memForTimerMsg.put((char*)timerMsg);	              // 定时器还没被触发，则直接释放消息内存
		}
		else
		{
		    timerMsg->deleteRef = msgCount;      // 消息已经在队列中了，此时不能释放该消息，只能由消息调度的时候释放
		}
        
        return param;
	}
    
    return NULL;
}

void CService::handleServiceMessage(ServiceMsgHeader* msgHeader, unsigned int msgLen)
{
	const unsigned short moduleId = ntohs(msgHeader->dstService.moduleId);
	const unsigned int srcServiceId = ntohl(msgHeader->srcService.serviceId);
	const unsigned short srcSrvType = ntohs(msgHeader->srcService.serviceType);
	const bool isProxyMsg = (srcSrvType == GatewayServiceType && m_gatewayServiceMode);
	if (moduleId >= MaxModuleIDCount)
	{
		if (isProxyMsg) ++srvStatData.errorClientMsgs;
		else ++srvStatData.errorServiceMsgs;
		
		ReleaseErrorLog("receive msg error, srcServiceId = %d, dstModuleId = %d, srcSrvType = %d",
		srcServiceId, moduleId, srcSrvType);
		return;
	}
	
	CModule* handleModule = m_moduleInstance[moduleId];
	if (handleModule == NULL)
	{
		if (isProxyMsg) ++srvStatData.errorClientMsgs;
		else ++srvStatData.errorServiceMsgs;
		
		ReleaseErrorLog("not find the module handler, srcServiceId = %d, dstModuleId = %d, srcSrvType = %d",
		srcServiceId, moduleId, srcSrvType);
		return;
	}
	
	// 连接代理的消息
	const unsigned int userDataLen = ntohl(msgHeader->userDataLen);
	const char* userData = (char*)msgHeader + MsgHeaderLen;
	const int userFlag = ntohl(msgHeader->userFlag);
	const unsigned short srvProtocolId = ntohs(msgHeader->srcService.protocolId);
	if (isProxyMsg)
	{
		ProxyID proxyId;
		proxyId.proxyFlag.flag = userFlag;
		proxyId.proxyFlag.serviceId = srcServiceId;
		ConnectProxy* conn = m_connectMgr.getProxy(proxyId.id);
		
		if (userDataLen == sizeof(ConnectAddress) && conn != NULL)
		{
			// 代理连接发往本服务的第一条消息，但索引对应的连接数据已经存在，说明代理服务异常退出，而没有通知到本服务之前的代理连接已经被异常关闭的情况
			// 此情况下先通知服务连接代理已经被关闭了
			ReleaseErrorLog("receive proxy first message but already have repeat proxy id = %d, flag = %d", srcServiceId, userFlag);
			closeProxy(conn, false, ConnectProxyOperation::ProxyException);
			conn = NULL;  // 需要重新创建连接代理数据
		}
        
        // 连接是否从网关关闭了
        const bool isClosedConnect = (srvProtocolId == ConnectProxyOperation::PassiveClosed || srvProtocolId == ConnectProxyOperation::InvalidService
                                                             || srvProtocolId == ConnectProxyOperation::OversizedMessage || srvProtocolId == ConnectProxyOperation::ClientFrequentMessage
                                                             || srvProtocolId == ConnectProxyOperation::ServiceFrequentMessage || srvProtocolId == ConnectProxyOperation::GatewayServiceStop);
		if (conn == NULL)
		{
			if (isClosedConnect) return;
			
			// 第一次获取该标识的代理数据
			if (userDataLen != sizeof(ConnectAddress))
			{
				++srvStatData.errorClientMsgs;
				
				ReleaseErrorLog("receive proxy error msg, serviceId = %d, userFlag = %d, userDataLen = %d, protocolId = %d",
				srcServiceId, userFlag, userDataLen, ntohs(msgHeader->dstService.protocolId));
		        return;
			}
			
			conn = m_connectMgr.createProxy();
			memset(conn, 0, sizeof(ConnectProxy));
			conn->proxyFlag = userFlag;
			conn->proxyId = srcServiceId;
			conn->peerIp = ((ConnectAddress*)userData)->peerIp;
			conn->peerPort = ((ConnectAddress*)userData)->peerPort;
			
			m_connectMgr.addProxy(proxyId.id, conn);
			
			ReleaseInfoLog("create connect proxy, peer ip = %s, port = %d, id = %d, flag = %d, proxyId = %lld, connProxy = %p",
			NConnect::CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->proxyId, conn->proxyFlag, proxyId.id, conn);
		}
		
		++srvStatData.recvClientMsgs;
	    srvStatData.recvCltMsgSize += msgLen;
        if (msgLen > srvStatData.recvCltMsgMaxLen) srvStatData.recvCltMsgMaxLen = msgLen;
		
		// 代理的连接被动关闭了
		if (isClosedConnect) return closeProxy(conn, false, srvProtocolId);

		if (handleModule->onProxyMessage(userData + userDataLen, ntohl(msgHeader->msgLen), ntohl(msgHeader->msgId), 
	                                     srcServiceId, ntohs(msgHeader->srcService.moduleId),
									     ntohs(msgHeader->dstService.protocolId), conn) != Success)
		{
			++srvStatData.errorClientMsgs;
		}

		return;
	}

    ++srvStatData.recvServiceMsgs;
	srvStatData.recvSrvMsgSize += msgLen;
    if (msgLen > srvStatData.recvSrvMsgMaxLen) srvStatData.recvSrvMsgMaxLen = msgLen;
	
    // 异步数据标识
	const char* srvAsyncDataFlag = userData + userDataLen + ntohl(msgHeader->msgLen);
	const unsigned int asyncDataFlagLen = ntohl(msgHeader->asyncDataFlagLen);
	
	// 内部服务消息
	if (handleModule->onServiceMessage(userData + userDataLen, ntohl(msgHeader->msgLen), userData, userDataLen, 
	                                   ntohs(msgHeader->dstService.protocolId), srcServiceId, srcSrvType, ntohs(msgHeader->srcService.moduleId),
									   srvProtocolId, userFlag, ntohl(msgHeader->msgId), srvAsyncDataFlag, asyncDataFlagLen) != Success)
    {
        ++srvStatData.errorServiceMsgs;
	}
}

void CService::handleClientMessage(ClientMsgHeader* msgHeader, Connect* conn, unsigned int msgLen)
{
	++srvStatData.recvClientMsgs;
	srvStatData.recvCltMsgSize += msgLen;
    if (msgLen > srvStatData.recvCltMsgMaxLen) srvStatData.recvCltMsgMaxLen = msgLen;
    
    int rc = serviceInstance->onReceiveMessage(conn, (char*)msgHeader, msgLen);
    if (rc != Success)
    {
        ++srvStatData.errorClientMsgs;
		
		ReleaseErrorLog("receive client message before handle error, checksum = %u, len = %u, rc = %d", ntohl(msgHeader->checksum), msgLen, rc);
		return;
    }

	unsigned short moduleId = ntohs(msgHeader->moduleId);
	if (moduleId >= MaxModuleIDCount)
	{
		++srvStatData.errorClientMsgs;
		
		ReleaseErrorLog("receive client message module id error, moduleId = %d", moduleId);
		return;
	}
	
	CModule* handleModule = m_moduleInstance[moduleId];
	if (handleModule == NULL)
	{
		++srvStatData.errorClientMsgs;
		
		ReleaseErrorLog("not find the module handler for client message, moduleId = %d", moduleId);
		return;
	}

	if (handleModule->onClientMessage((char*)msgHeader + ClientMsgHeaderLen, ntohl(msgHeader->msgLen), ntohl(msgHeader->msgId),
	                                  ntohl(msgHeader->serviceId), moduleId, ntohs(msgHeader->protocolId), conn) != Success)
    {
        ++srvStatData.errorClientMsgs;
    }
}

void CService::handleClientMessage(const char* data, Connect* conn, unsigned int msgLen)
{
	++srvStatData.recvClientMsgs;
	srvStatData.recvCltMsgSize += msgLen;
    if (msgLen > srvStatData.recvCltMsgMaxLen) srvStatData.recvCltMsgMaxLen = msgLen;

	CModule* handleModule = m_moduleInstance[NetDataHandleModuleID];
	if (handleModule == NULL)
	{
		++srvStatData.errorClientMsgs;
		
		ReleaseErrorLog("not find the module handler for client data");
		return;
	}

	if (handleModule->onClientMessage(data, msgLen, 0, 0, 0, 0, conn) != Success)
    {
        ++srvStatData.errorClientMsgs;
    }
}

bool CService::handleTimerMessage()
{
	TimerMessage* timerMsg = (TimerMessage*)m_timerMsgQueue.get();
	if (timerMsg == NULL) return false;

    if (timerMsg->deleteRef == 0)
	{
		++srvStatData.onTimerMsgs;
		
		// 存在业务上层可能在onTimeOut回调里执行killTimer，setTimer的情况，则onTimeOut调用完毕之后timerMsg已经被重新分配给新的定时器
		// 因此这里必须提前保存timerId值&剩余触发次数做为判断使用
		const unsigned int timerId = timerMsg->timerId;
		const unsigned int count = timerMsg->count;
		
		timerMsg->handler->onTimeOut(timerMsg->cbFunc, timerMsg->timerId, timerMsg->userId, timerMsg->param, timerMsg->count);
		
		// 存在业务上层可能在onTimeOut回调里执行killTimer，setTimer的情况，再加上TimerMessage是内存块管理
        // 因此必须在调用完onTimeOut之后才能释放内存块，否则会导致内存块被错误释放
		if (count == 0)
		{
			// 存在业务上层可能在onTimeOut回调里执行killTimer，setTimer的情况，因此这里必须重新判断，防止timerMsg被多次释放导致的错误
			TimerIdToMsg::iterator timerIt = m_timerIdToMsg.find(timerId);
			if (timerIt != m_timerIdToMsg.end())
			{
                if (timerMsg->paramLen > 0)  // 存在创建的内存空间
                {
                    char* param = (char*)timerMsg->param;
                    DELETE(param);  // 删除释放内存空间
                    
                    timerMsg->param = NULL;
                    timerMsg->paramLen = 0;
                }
        
				m_timerIdToMsg.erase(timerIt);
				m_memForTimerMsg.put((char*)timerMsg);
			}
		}
	}
	else if (--timerMsg->deleteRef == 0)        // 该消息已经被用户执行kill操作干掉了
	{
        // killTimer 操作调用的时候已经删除释放过 timerMsg->param 内存了，如果存在内存空间的话
		m_memForTimerMsg.put((char*)timerMsg);  // 此时才可以释放消息内存块
	}
	
	return true;
}

const char* CService::getName()
{
	return m_srvName;
}

unsigned short CService::getType()
{
	return m_srvType;
}

unsigned int CService::getId()
{
	return m_srvId;
}

unsigned int CService::getServiceId(const char* serviceName)
{
	return m_srvMsgComm->getSrvId(serviceName); 
}

// 向目标服务发送请求消息
// handleProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话
int CService::sendMessage(const char* msgData, const unsigned int msgLen, const char* userData, const unsigned int userDataLen, const char* srvAsyncDataFlag, const unsigned int srvAsyncDataFlagLen,
                          unsigned int dstServiceId, unsigned short dstProtocolId, unsigned short dstModuleId, unsigned short srcModuleId, 
                          unsigned short handleProtocolId, int userFlag, unsigned int msgId)
{
    return sendMessage(m_srvType, m_srvId, msgData, msgLen, userData, userDataLen, srvAsyncDataFlag, srvAsyncDataFlagLen, dstServiceId, dstProtocolId, dstModuleId, srcModuleId,
					   handleProtocolId, userFlag, msgId);
}

// handleProtocolId : 应答消息的处理协议ID，如果该消息存在应答的话（用于转发消息，替换源消息的服务类型及服务ID，使得目标服务直接回消息给源端而不是中转服务）					  
int CService::sendMessage(const unsigned short srcServiceType, const unsigned int srcServiceId, const char* msgData, const unsigned int msgLen, const char* userData,
						  const unsigned int userDataLen, const char* srvAsyncDataFlag, const unsigned int srvAsyncDataFlagLen, unsigned int dstServiceId, unsigned short dstProtocolId,
						  unsigned short dstModuleId, unsigned short srcModuleId, unsigned short handleProtocolId, int userFlag, unsigned int msgId)
{
	if (msgLen > MaxMsgLength) return LargeMessage;
	if (userDataLen > MaxUserDataLen) return LargeUserData;
	if (srvAsyncDataFlagLen > MaxLocalAsyncDataFlagLen) return LargeAsyncDataFlag;
	
	// 填写消息头部数据
	ServiceMsgHeader* msgHeader = (ServiceMsgHeader*)m_sndMsg;
	msgHeader->srcService.serviceId = htonl(srcServiceId);
	msgHeader->srcService.serviceType = htons(srcServiceType);
	msgHeader->srcService.moduleId = htons(srcModuleId);
	msgHeader->srcService.protocolId = htons(handleProtocolId);
	msgHeader->dstService.serviceId = htonl(dstServiceId);
	msgHeader->dstService.serviceType = 0;  // 目标服务类型不需要填写
	msgHeader->dstService.moduleId = htons(dstModuleId);
	msgHeader->dstService.protocolId = htons(dstProtocolId);
	msgHeader->userFlag = htonl(userFlag);
	msgHeader->msgId = htonl(msgId);
	msgHeader->userDataLen = htonl(userDataLen);
	msgHeader->asyncDataFlagLen = htonl(srvAsyncDataFlagLen);
	msgHeader->msgLen = htonl(msgLen);

    // 消息码流数据
	char* sendMsgData = m_sndMsg + MsgHeaderLen;
	if (userDataLen > 0 && userData != NULL)  // 存在无用户数据的情况
	{
		memcpy(sendMsgData, userData, userDataLen);
		sendMsgData += userDataLen;
	}
	else
	{
		msgHeader->userDataLen = 0;
	}
	
	// 存在协议无数据的情况
	if (msgData != NULL)
	{
		memcpy(sendMsgData, msgData, msgLen);
		sendMsgData += msgLen;
	}
	else
	{
		msgHeader->msgLen = 0;
	}

	// 存在无异步数据标识的情况
	if (srvAsyncDataFlagLen > 0 && srvAsyncDataFlag != NULL)
	{
		memcpy(sendMsgData, srvAsyncDataFlag, srvAsyncDataFlagLen);
		sendMsgData += srvAsyncDataFlagLen;
	}
	else
	{
		msgHeader->asyncDataFlagLen = 0;
	}
	
    const unsigned int allMsgLen = (sendMsgData - m_sndMsg);
	++srvStatData.sendServiceMsgs;
	srvStatData.sendSrvMsgSize += allMsgLen;
    if (allMsgLen > srvStatData.sendSrvMsgMaxLen) srvStatData.sendSrvMsgMaxLen = allMsgLen;
	
	int rc = m_srvMsgComm->send(dstServiceId, m_sndMsg, allMsgLen);
	if (rc != Success)
	{
		--srvStatData.sendServiceMsgs;
	    srvStatData.sendSrvMsgSize -= allMsgLen;
		++srvStatData.sendSrvMsgFaileds;
		
		// static char logMsg[MaxMsgLen] = {0};
		// b2str(m_sndMsg, allMsgLen, logMsg, MaxMsgLen);
		ReleaseErrorLog("send message to service failed, rc = %d, id = %d, protocol = %d, module = %d, len = %d",
		rc, dstServiceId, dstProtocolId, dstModuleId, allMsgLen);
	}

	return rc;
}

// 向目标网络客户端发送消息
int CService::sendMsgToClient(const char* msgData, const unsigned int msgLen, unsigned int msgId,
                              unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn)
{
	if (m_netMsgComm == NULL) return NoSupportConnectClient;
	if (conn == NULL) return NoFoundClientConnect;
	if (msgLen > MaxMsgLength) return LargeMessage;
	
	// 填写网络包头部数据
	static const unsigned int NetPkgHeaderLen = sizeof(NetPkgHeader);
	NetPkgHeader* netPkgHeader = (NetPkgHeader*)m_sndMsg;
	memset(netPkgHeader, 0, NetPkgHeaderLen);
	
	// 填写消息头部数据
	ClientMsgHeader* msgHeader = (ClientMsgHeader*)(m_sndMsg + NetPkgHeaderLen);
	msgHeader->serviceId = htonl(serviceId);
	msgHeader->moduleId = htons(moduleId);
	msgHeader->protocolId = htons(protocolId);
	msgHeader->msgId = htonl(msgId);
	msgHeader->msgLen = htonl(msgLen);
	
	// 存在协议无数据的情况
	if (msgData != NULL) memcpy((char*)msgHeader + ClientMsgHeaderLen, msgData, msgLen);
	else msgHeader->msgLen = 0;

    unsigned int pkgLen = ClientMsgHeaderLen + msgLen;
    int rc = serviceInstance->onSendMessage(conn, (char*)msgHeader, pkgLen);
    if (rc != Success)
    {
        ++srvStatData.sendCltMsgFaileds;
        
        ReleaseErrorLog("send message before handle error, rc = %d, protocol = %d, pkg len = %d", rc, protocolId, pkgLen);

		return rc;
    }
	
    pkgLen += NetPkgHeaderLen;
    netPkgHeader->len = htonl(pkgLen);
    netPkgHeader->type = MSG;
	rc = m_netMsgComm->send(conn, m_sndMsg, pkgLen, false);
	if (rc == Success)
	{
		++srvStatData.sendClientMsgs;
        srvStatData.sendCltMsgSize += pkgLen;
        if (pkgLen > srvStatData.sendCltMsgMaxLen) srvStatData.sendCltMsgMaxLen = pkgLen;
	}
    else
    {
        ++srvStatData.sendCltMsgFaileds;
		
		// static char logMsg[MaxMsgLen] = {0};
		// b2str(m_sndMsg, pkgLen, logMsg, MaxMsgLen);
		ReleaseErrorLog("send message to net client failed, ip = %s, port = %d, rc = %d, protocol = %d, pkg len = %d", NConnect::CSocket::toIPStr(conn->peerIp), conn->peerPort, rc, protocolId, pkgLen);
    }
	
	return rc;
}

// 向目标网络客户端发送任意数据
int CService::sendDataToClient(Connect* conn, const char* data, const unsigned int len)
{
	if (m_netMsgComm == NULL) return NoSupportConnectClient;
	if (conn == NULL) return NoFoundClientConnect;
	if (data == NULL || len < 1) return InvalidParam;
	
	++srvStatData.sendClientMsgs;
	srvStatData.sendCltMsgSize += len;
    if (len > srvStatData.sendCltMsgMaxLen) srvStatData.sendCltMsgMaxLen = len;
	
	int rc = m_netMsgComm->send(conn, data, len, false);
	if (rc != Success)
	{
		--srvStatData.sendClientMsgs;
	    srvStatData.sendCltMsgSize -= len;
		++srvStatData.sendCltMsgFaileds;
		ReleaseErrorLog("send data to net client failed, ip = %s, port = %d, rc = %d, len = %d", NConnect::CSocket::toIPStr(conn->peerIp), conn->peerPort, rc, len);
	}
	
	return rc;
}

// 向目标连接代理发送消息
int CService::sendMsgToProxy(const char* msgData, const unsigned int msgLen, unsigned int msgId,
							 unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, ConnectProxy* conn)
{
	if (conn == NULL) return NoFoundConnectProxy;
	if (msgLen > MaxMsgLength) return LargeMessage;

	// 填写消息头部数据
	ServiceMsgHeader* msgHeader = (ServiceMsgHeader*)m_sndMsg;
	msgHeader->srcService.serviceId = (serviceId == 0) ? htonl(m_srvId) : htonl(serviceId);
	msgHeader->srcService.serviceType = htons(m_srvType);
	msgHeader->srcService.moduleId = htons(moduleId);
	msgHeader->srcService.protocolId = 0;
	
	msgHeader->dstService.serviceId = conn->proxyId;
	msgHeader->dstService.serviceType = 0;
	msgHeader->dstService.moduleId = 0;
	msgHeader->dstService.protocolId = htons(protocolId);
	msgHeader->userFlag = htonl(conn->proxyFlag);
	msgHeader->msgId = htonl(msgId);
	msgHeader->userDataLen = 0;
	msgHeader->asyncDataFlagLen = 0;
	msgHeader->msgLen = htonl(msgLen);
	
	// 存在协议无数据的情况
	if (msgData != NULL) memcpy(m_sndMsg + MsgHeaderLen, msgData, msgLen);
	else msgHeader->msgLen = 0;

    const unsigned int allMsgLen = (MsgHeaderLen + msgLen);
	++srvStatData.sendClientMsgs;
	srvStatData.sendCltMsgSize += allMsgLen;
    if (allMsgLen > srvStatData.sendCltMsgMaxLen) srvStatData.sendCltMsgMaxLen = allMsgLen;
	
	int rc = m_srvMsgComm->send(conn->proxyId, m_sndMsg, MsgHeaderLen + msgLen);
	if (rc != Success)
	{
		--srvStatData.sendClientMsgs;
	    srvStatData.sendCltMsgSize -= allMsgLen;
		++srvStatData.sendCltMsgFaileds;
		
		// static char logMsg[MaxMsgLen] = {0};
		// b2str(m_sndMsg, allMsgLen, logMsg, MaxMsgLen);
		ReleaseErrorLog("send message to proxy failed, rc = %d, id = %d, protocol = %d, module = %d, len = %d",
		rc, conn->proxyId, protocolId, moduleId, allMsgLen);
	}

	return rc;
}

void CService::closeProxy(ConnectProxy* conn, bool isActive, int cbFlag)
{
	if (conn == NULL) return;
	
	ProxyID proxyId;
	proxyId.proxyFlag.flag = conn->proxyFlag;
	proxyId.proxyFlag.serviceId = conn->proxyId;
	closeProxy(proxyId.id, isActive, cbFlag);
}

void CService::closeProxy(const uuid_type id, bool isActive, int cbFlag)
{
	// 1、先查找是否存在对应的代理连接
	// 已经被释放过了的，不能重复释放，
	// 存在本服务和连接代理服务器同时关闭连接的情况，本服务器先关闭，此时连接代理服务也发消息过来关闭
	FlagToProxys::iterator connProxyIt;
	if (!m_connectMgr.haveProxy(id, connProxyIt)) return;

    // 2、接着通知服务连接关闭了
	ConnectProxy* conn = connProxyIt->second;
	void* userCb = m_connectMgr.getProxyUserData(conn);
	if (userCb != NULL) serviceInstance->onCloseConnectProxy(userCb, cbFlag);
					
	// 3、再发消息给网关代理关闭该用户对应的网络连接
	// 主动关闭则通知连接代理关闭对应的连接
	int rc = 0;
	if (isActive) rc = sendMessage(NULL, 0, NULL, 0, NULL, 0, conn->proxyId, 0, 0, 0, ConnectProxyOperation::ActiveClosed, conn->proxyFlag);
	
	ReleaseWarnLog("%s close connect proxy, peer ip = %s, port = %d, id = %d, flag = %d, proxyId = %lld, connProxy = %p, userCb = %p, rc = %d", isActive ? "active" : "passive",
			       NConnect::CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->proxyId, conn->proxyFlag, id, conn, userCb, rc);

	// 4、最后才回收资源
	// 回收资源
	m_connectMgr.removeProxy(connProxyIt);  // 最后才可以执行remove操作，存在应用上层在onCloseConnectProxy回调里发消息的场景
    m_connectMgr.destroyProxy(conn);
}

uuid_type CService::getProxyId(ConnectProxy* connProxy)
{
	if (connProxy == NULL) return 0;
	
	ProxyID proxyId;
	proxyId.proxyFlag.flag = connProxy->proxyFlag;
	proxyId.proxyFlag.serviceId = connProxy->proxyId;
	return proxyId.id;
}

ConnectProxy* CService::getProxy(const uuid_type id)
{
	return m_connectMgr.getProxy(id);
}

void CService::stopProxy()
{
	unordered_map<unsigned int, int> proxyIds;
	const FlagToProxys& flag2Proxy = m_connectMgr.getProxy();
	for (FlagToProxys::const_iterator it = flag2Proxy.begin(); it != flag2Proxy.end(); ++it)
	{
		// 一个代理服务可以代理N个连接，所以每个代理服务只发送一条消息
		ConnectProxy* conn = it->second;
		if (proxyIds.find(conn->proxyId) == proxyIds.end())
		{
			// 通知连接代理关闭该服务的连接
			proxyIds[conn->proxyId] = 0;
			sendMessage(NULL, 0, NULL, 0, NULL, 0, conn->proxyId, 0, 0, 0, ConnectProxyOperation::StopProxy);
			
			ReleaseWarnLog("active stop connect proxy, peer ip = %s, port = %d, id = %d",
			NConnect::CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->proxyId);
		}
		
		m_connectMgr.destroyProxy(conn);  // 回收资源
	}
	
	m_connectMgr.clearProxy();
}

void CService::cleanUpProxy(const unsigned int proxyId[], const unsigned int len)
{
	for (unsigned int idx = 0; idx < len; ++idx)
	{
		sendMessage(NULL, 0, NULL, 0, NULL, 0, proxyId[idx], 0, 0, 0, ConnectProxyOperation::StopProxy);
			
		ReleaseWarnLog("active clean up connect proxy, id = %d", proxyId[idx]);
	}
	
	m_connectMgr.clearProxy();
}


// 注册本服务的各模块实例对象
int CService::registerModule(unsigned short moduleId, CModule* pInstance)
{
	if (moduleId >= NetDataHandleModuleID || pInstance == NULL) return InvalidParam;
	
	m_moduleInstance[moduleId] = pInstance;
	return Success;
}

int CService::registerNetModule(CNetDataHandler* pInstance)
{
	if (pInstance == NULL) return InvalidParam;
	
	m_moduleInstance[NetDataHandleModuleID] = pInstance;
	return Success;
}

void CService::setServiceType(unsigned int srvType)
{
	m_srvType = srvType;
}

void CService::setConnectClient()
{
	NEW0(m_netMsgComm, CNetMsgComm);
	m_connectMgr.setNetMsgComm(m_netMsgComm);
}

void CService::setGatewayServiceMode(bool isGatewayMode)
{
    m_gatewayServiceMode = isGatewayMode;
}

std::mutex* CService::setThreadMutexMode()
{
    if (m_mutex == NULL)
    {
        NEW0(m_mutex, std::mutex);
    }
    
    return m_mutex;
}

// 该函数由网络层线程触发调用，因此需要队列做数据同步，否则调度线程和网络线程并发会导致错误
void CService::createdClientConnect(Connect* conn, const char* peerIp, const unsigned short peerPort)
{
	++srvStatData.createdConnects;
}

// 业务发起主动连接调用
int CService::doActiveConnect(CNetDataHandler* instance, ActiveConnectHandler cbFunction, const strIP_t peerIp, unsigned short peerPort, unsigned int timeOut, void* userCb, int userId)
{
	if (instance == NULL || cbFunction == NULL || peerIp == NULL || peerPort < MinConnectPort || timeOut < 1) return ECommon::InvalidParam;

	ActiveConnectData* connData = (ActiveConnectData*)m_memForActiveConnect.get();
	if (connData == NULL) return ECommon::NoMemory;
	
	memset(connData, 0, sizeof(ActiveConnectData));
	connData->instance = instance;
	connData->cbFunc = cbFunction;
	connData->conn = NULL;
	connData->data.userCb = connData;
	
	strncpy(connData->data.peerIp, peerIp, sizeof(connData->data.peerIp) - 1);
	connData->data.peerPort = peerPort;
	connData->data.timeOut = timeOut;
	connData->data.userId = userId;
	connData->userCb = userCb;

	if (!m_activeConnectDataQueue.put(connData))
	{
		m_memForActiveConnect.put((char*)connData);
		return ECommon::QueueFull;
	}
	
	return Success;
}

// 该函数由网络层线程触发调用，因此需要队列做数据同步，否则调度线程和网络线程并发会导致错误
// 本地端主动创建连接 peerIp&peerPort 为连接对端的IP、端口号
ActiveConnect* CService::getActiveConnectData()
{
	// 存在主动连接的操作则一次性处理完毕
	ActiveConnectData* connData = (ActiveConnectData*)m_activeConnectDataQueue.get();
	ActiveConnect* header = NULL;
	while (connData != NULL)
	{
		connData->data.next = header;
		header = &(connData->data);
		connData = (ActiveConnectData*)m_activeConnectDataQueue.get();
	}

	return header;
}

// 该函数由网络层线程触发调用，因此需要队列做数据同步，否则调度线程和网络线程并发会导致错误
// 主动连接建立的时候调用
// conn 为连接对应的数据，peerIp & peerPort 为连接对端的IP地址和端口号
// rtVal : 返回连接是否创建成功
void CService::doCreateServiceConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, ReturnValue rtVal, void*& cb)
{
	ActiveConnectData* connData = (ActiveConnectData*)cb;
	if (rtVal == ReturnValue::OptSuccess)
	{
		connData->conn = conn;
		conn->userCb = NULL;
		
		++srvStatData.createdConnects;
	    ReleaseInfoLog("create active connect success, ip = %s, port = %d, connId = %lld", peerIp, peerPort, conn->id);
	}
	else
	{
		ReleaseErrorLog("create active connect failed, ip = %s, port = %d, return code = %d", peerIp, peerPort, rtVal);
	}
	
	m_activeConnectResultQueue.put(connData);
}

// 该函数由网络层线程触发调用，因此需要队列做数据同步，否则调度线程和网络线程并发会导致错误
void CService::closedClientConnect(Connect* conn)
{
	++srvStatData.closedConnects;
	
	void* userData = m_connectMgr.getUserData(conn);
	if (userData != NULL && !m_closedClientConnectQueue.put(userData))
	{
		ReleaseErrorLog("put close client connect operation to queue failed, queue size = %d", m_closedClientConnectQueue.getSize());
	}
}


// 注册&去注册服务实例
CRegisterService::CRegisterService(IService* pSrvInstance)
{
	serviceInstance = pSrvInstance;
}

CRegisterService::~CRegisterService()
{
	serviceInstance = NULL;
}

const char* IService::getName()
{
	return getService().getName();
}

unsigned int IService::getId()
{
	return getService().getId();
}

// 注册本服务的各模块实例对象
int IService::registerModule(unsigned short moduleId, CModule* pInstance)
{
	return getService().registerModule(moduleId, pInstance);
}

// 注册纯网络数据处理模块实例对象
int IService::registerNetModule(CNetDataHandler* pInstance)
{
	return getService().registerNetModule(pInstance);
}

void IService::stopService(int flag)
{
	return getService().stop(flag);
}

// 设置是否使用网关代理模式（目标服务收到网关服务的消息时，是否使用网关代理模式）
void IService::setGatewayServiceMode(bool isGatewayMode)
{
    return getService().setGatewayServiceMode(isGatewayMode);
}

// 服务启动时调用
int IService::onInit(const char* name, const unsigned int id)
{
	return Success;
}

// 服务停止时调用
void IService::onUnInit(const char* name, const unsigned int id)
{
}

// 服务配置更新
void IService::onUpdateConfig(const char* name, const unsigned int id)
{
}

// 服务自己的处理逻辑
int IService::onHandle()
{
	return NoLogicHandle;
}

// 收到外部数据之后调用 onReceiveMessage
// 发送外部数据之前调用 onSendMessage
// 一般用于数据加密&解密
int IService::onReceiveMessage(NConnect::Connect* conn, char* msg, unsigned int& len)
{
    return Success;
}

int IService::onSendMessage(NConnect::Connect* conn, char* msg, unsigned int& len)
{
    return Success;
}

// 通知逻辑层对应的连接已被关闭
void IService::onClosedConnect(void* userData)
{
}

// 通知逻辑层对应的逻辑连接代理已被关闭
void IService::onCloseConnectProxy(void* userData, int cbFlag)
{
}

IService::IService(unsigned int srvType, bool isConnectClient)
{
	getService().setServiceType(srvType);
	if (isConnectClient) getService().setConnectClient();
}

IService::~IService()
{
}

}


int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("Need input service config file, service message communication config file and service name as param!\nUsage example : exefile ./srv.cfg ./srvMsgComm.cfg srvName\n");
		return -1;
	}
	
	NFrame::CService& service = NFrame::getService();
	if (service.start(argv[1], argv[2], argv[3]) == Success)
	{
		service.run();
	}
	
	return 0;
}

