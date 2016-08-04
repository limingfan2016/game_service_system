
/* author : limingfan
 * date : 2015.01.15
 * description : 各服务间消息通信中间件
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>

#include "CSrvMsgComm.h"
#include "CNetMsgHandler.h"
#include "base/CCfg.h"
#include "base/ErrorCode.h"
#include "base/Constant.h"
#include "base/CShm.h"
#include "connect/CConnectManager.h"
#include "messageComm/CMsgComm.h"
#include "base/CProcess.h"


using namespace NConnect;
using namespace NErrorCode;

namespace NMsgComm
{

static const int NetConnectCount = 1024;                    // 网络连接长度
static const int WaitEventTimeOut = 1;                      // 监听网络连接事件，等待时间，单位毫秒

static const unsigned int MinShmSize = 1024 * 1024 * 2;     // 共享内存块的最小值2M
static const int MinMsgQueueSize = 1024;                    // 网络连接消息队列的最小长度


// 外部发信号正常退出
static CConnectManager* ST_ConnectMgr = NULL;
static void ExitProcess(int sigNum, siginfo_t* sigInfo, void* context)
{
	ReleaseWarnLog("receive signal = %d, and exit process", sigNum);
	if (ST_ConnectMgr != NULL) ST_ConnectMgr->stop();
}

// 动态刷新配置项
static CSrvMsgComm* ST_SrvMsgComm = NULL;
static void UpdateConfig(void* cb)
{
	if (ST_SrvMsgComm != NULL) ST_SrvMsgComm->updateConfig();
}


CSrvMsgComm::CSrvMsgComm() : m_shmData(NULL), m_keyFileFd(-1)
{
	memset(&m_cfgData, 0, sizeof(m_cfgData));
}

CSrvMsgComm::~CSrvMsgComm()
{
	memset(&m_cfgData, 0, sizeof(m_cfgData));
	m_shmData = NULL;
	m_keyFileFd = -1;
}
	
int CSrvMsgComm::createShm(const char* cfgFile)
{
	// 初始化配置项
	int rc = initCfgFile(cfgFile);
	if (rc != Success) return rc;
	
	// 从key文件节点数据获取共享内存key值
    char shmKeyFile[MaxFullLen] = {0};
	int shmKey = CMsgComm::getFileKey(m_cfgData.Name, m_cfgData.Name, shmKeyFile, MaxFullLen);
	if(shmKey == 0) return CreateKeyFileFailed;
	
	// 给key文件加锁，防止被重复打开多次
	rc = CSharedMemory::lockFile(shmKeyFile, 0, m_keyFileFd);
    if (rc != Success) return rc;
	
	// 创建共享内存
	// 申请的共享内存总大小
    int shmSize = sizeof(ShmData) + sizeof(MsgHandlerList) * m_cfgData.ShmCount;   // 共享内存头部、网络节点
	shmSize += ((m_cfgData.ShmSize + sizeof(MsgHandlerList) + sizeof(MsgQueueList)) * m_cfgData.ShmCount);
	int flag = 0666 | IPC_CREAT;  // 创建共享内存标识
	int isNewCreate = 0;
	int shmId = -1;
	char* pShm = NULL;
	rc = CSharedMemory::create(shmKey, shmSize, flag, isNewCreate, shmId, pShm);
	if (rc != Success) return rc;

    // 初始化共享内存数据
    m_shmData = (ShmData*)pShm;
	if (isNewCreate)
	{
        rc = initShm();  // 共享内存新创建则初始化头部信息
		if (rc != Success) return rc;
		
		ReleaseInfoLog("create new shared memory success, size = %u, config size = %u",
		m_shmData->msgQueueSize, m_cfgData.ShmSize * m_cfgData.ShmCount);
	}
    else
	{
		initNetHandler();
        ReleaseWarnLog("get already existent shared memory success, size = %u, config size = %u",
		m_shmData->msgQueueSize, m_cfgData.ShmSize * m_cfgData.ShmCount);
	}

	ReleaseInfoLog("open shared memory, pid = %lu, size = %d, key file = %s, key = 0x%x, shmId = %d\n",
	                getpid(), shmSize, shmKeyFile, shmKey, shmId);

	return Success;
}
	
void CSrvMsgComm::run()
{
	if (m_shmData == NULL)
	{
		ReleaseErrorLog("can not open shared memory");
		return;
	}
	
	if (m_shmData->msgQueueSize != (m_cfgData.ShmSize * m_cfgData.ShmCount))
	{
		ReleaseErrorLog("create shared memory size = %d not equal the config size = %d, block size = %d, block count = %d",
		m_shmData->msgQueueSize, m_cfgData.ShmSize * m_cfgData.ShmCount, m_cfgData.ShmSize, m_cfgData.ShmCount);
		return;
	}
	
	CNetMsgHandler netMsgHandler(m_shmData, &m_cfgData);
	CConnectManager connectManager(m_cfgData.IP, m_cfgData.Port, &netMsgHandler);

	ServerCfgData cfgData;
	cfgData.listenMemCache = NetConnectCount;    // 连接内存池内存块个数
	cfgData.listenNum = NetConnectCount;         // 监听连接的队列长度
	cfgData.count = atoi(CCfg::getValue("NetConnect", "ConnectCount"));                // 读写缓冲区内存池内存块个数
	cfgData.step = cfgData.count;                 // 自动分配的内存块个数
	cfgData.size = atoi(CCfg::getValue("NetConnect", "MsgQueueSize"));                 // 每个读写数据区内存块的大小
	cfgData.activeInterval = atoi(CCfg::getValue("NetConnect", "ActiveTimeInterval"));       // 连接活跃检测间隔时间，单位秒，超过此间隔时间无数据则关闭连接
	cfgData.checkTimes = atoi(CCfg::getValue("NetConnect", "CheckNoDataTimes"));             // 检查最大socket无数据的次数，超过此最大次数则连接移出消息队列，避免遍历一堆无数据的空连接
	cfgData.hbInterval = atoi(CCfg::getValue("NetConnect", "HBInterval"));           // 心跳检测间隔时间，单位秒
	cfgData.hbFailedTimes = atoi(CCfg::getValue("NetConnect", "HBFailTimes"));        // 心跳检测连续失败hbFailedTimes次后关闭连接
	int rc = connectManager.start(cfgData);
	if (rc != Success)
	{
		ReleaseErrorLog("start net connect manager failed, rc = %d", rc);
		return;
	}

    // 启动网络连接管理服务
	ST_ConnectMgr = &connectManager;
	CProcess::installSignal(SignalNumber::StopProcess, NMsgComm::ExitProcess);  // 进程正常退出信号
	CConfigManager::addUpdateNotify(NMsgComm::UpdateConfig);
	
	connectManager.run(NetConnectCount, WaitEventTimeOut);
}

int CSrvMsgComm::initShm()
{
	memset(m_shmData, 0, sizeof(ShmData));
	int rc = initSharedMutex();
	if (rc != Success) return rc;
	
	// 各个不同类型数据块在共享内存中的分布
	// |1个ShmData |连续N个当前节点的 MsgHandlerList |连续N个网络节点的 MsgHandlerList |连续N个 MsgQueueList| 
	HandlerIndex msgHandlerList = sizeof(ShmData);
	HandlerIndex netMsgHandlerList = msgHandlerList + sizeof(MsgHandlerList) * m_cfgData.ShmCount;
	QueueIndex msgQueueList = netMsgHandlerList + sizeof(MsgHandlerList) * m_cfgData.ShmCount;
	
	// 链接串成队列
	MsgHandlerList* msgHandler = NULL;
	m_shmData->msgHandlerList = msgHandlerList;
	
	MsgHandlerList* netMsgHandler = NULL;
	m_shmData->netMsgHanders = netMsgHandlerList;
	
	MsgQueueList* msgQueue = NULL;
	m_shmData->msgQueueList = msgQueueList;
	for (unsigned int i = 0; i < m_cfgData.ShmCount; ++i)
	{
		msgHandler = (MsgHandlerList*)((char*)m_shmData + msgHandlerList);
		memset(msgHandler, 0, sizeof(MsgHandlerList));
		
		netMsgHandler = (MsgHandlerList*)((char*)m_shmData + netMsgHandlerList);
		memset(netMsgHandler, 0, sizeof(MsgHandlerList));
		
		if (i < m_cfgData.ShmCount - 1)
		{
			// 下一个MsgHandlerList的开始地址
			msgHandler->next = msgHandlerList + sizeof(MsgHandlerList);
		    msgHandlerList = msgHandler->next;
			
			netMsgHandler->next = netMsgHandlerList + sizeof(MsgHandlerList);
		    netMsgHandlerList = netMsgHandler->next;
		}
		else
		{
			msgHandler->next = 0;
			netMsgHandler->next = 0;
		}
		
		msgQueue = (MsgQueueList*)((char*)m_shmData + msgQueueList);
		memset(msgQueue, 0, sizeof(MsgQueueList));
		msgQueue->header.size = m_cfgData.ShmSize;
		msgQueue->queue = msgQueueList + sizeof(MsgQueueList);
		if (i < m_cfgData.ShmCount - 1)
		{
			// 下一个消息队列List的开始地址
			msgQueue->next = msgQueue->queue + m_cfgData.ShmSize;
			msgQueueList = msgQueue->next;
		}
		else
		{
			msgQueue->next = 0;
		}
	}
	
	// 初始化外部网络节点信息
	const Key2Value* nodeIPs = CCfg::getValueList("NodeIP");
	const char* localIp = CCfg::getValue("SrvMsgComm", "IP");
	netMsgHandlerList = m_shmData->netMsgHanders;
	while (nodeIPs != NULL)
	{
		if (strcmp(nodeIPs->value, "0") != 0 && strcmp(nodeIPs->value, localIp) != 0)  // 非本地IP
		{
			netMsgHandler = (MsgHandlerList*)((char*)m_shmData + netMsgHandlerList);
			netMsgHandler->readerNode.ipPort.ip = CSocket::toIPInt(nodeIPs->value);
			netMsgHandler->readerNode.ipPort.port = atoi(CCfg::getValue("NodePort", nodeIPs->key));
			netMsgHandlerList = netMsgHandler->next;
		}	
		nodeIPs = nodeIPs->pNext;
	}
	
	// 标识共享内存初始化成功，此时共享内存可用
	m_shmData->msgQueueSize = m_cfgData.ShmSize * m_cfgData.ShmCount;
	
	return Success;
}

void CSrvMsgComm::initNetHandler()
{
	const char* localIp = CCfg::getValue("SrvMsgComm", "IP");
	const Key2Value* nodeIPs = CCfg::getValueList("NodeIP");
	const Key2Value* findIP = NULL;
	unsigned int nodePort = 0;
	
	MsgHandlerList* msgHandler = NULL;
	HandlerIndex netHandlerIdx = m_shmData->netMsgHanders;
	while (netHandlerIdx != 0)
	{
		msgHandler = (MsgHandlerList*)((char*)m_shmData + netHandlerIdx);
		netHandlerIdx = msgHandler->next;
		msgHandler->status = InitConnect;
		msgHandler->connId = 0;
		
		if (msgHandler->readerNode.id == 0) break;
		
		findIP = nodeIPs;
		while (findIP != NULL)
		{
			if (strcmp(findIP->value, "0") != 0 && strcmp(findIP->value, localIp) != 0  // 非本地IP
			    && msgHandler->readerNode.ipPort.ip == CSocket::toIPInt(findIP->value))
			{
				nodePort = atoi(CCfg::getValue("NodePort", findIP->key));
				if (msgHandler->readerNode.ipPort.port != nodePort)  // 变更端口号了
				{
					msgHandler->readerNode.ipPort.port = nodePort;
					msgHandler->readMsg = 0;
				}
				
				break;
			}	
			findIP = findIP->pNext;
		}
		if (findIP == NULL) msgHandler->readMsg = 0;  // 没找到说明被删除了
	}
}

int CSrvMsgComm::initSharedMutex()
{
	int rc = pthread_mutexattr_init(&m_shmData->sharedMutex.mutexAttr);
	do
	{
		if (rc != 0)
		{
			ReleaseErrorLog("init shared mutex attribute error = %d, info = %s", rc, strerror(rc));
			break;
		}
		
		// 设置为进程间共享
		rc = pthread_mutexattr_setpshared(&m_shmData->sharedMutex.mutexAttr, PTHREAD_PROCESS_SHARED);
		if (rc != 0)
		{
			ReleaseErrorLog("set shared mutex attribute error = %d, info = %s", rc, strerror(rc));
			break;
		}
		
		rc = pthread_mutex_init(&m_shmData->sharedMutex.mutex, &m_shmData->sharedMutex.mutexAttr);
		if (rc != 0)
		{
			ReleaseErrorLog("init shared mutex error = %d, info = %s", rc, strerror(rc));
			break;
		}
		
		return Success;
		
	} while (0);
	
	return InitSharedMutexError;
}

void CSrvMsgComm::unInitSharedMutex()
{
	if (m_shmData != NULL)
	{
		pthread_mutex_destroy(&m_shmData->sharedMutex.mutex);
	    pthread_mutexattr_destroy(&m_shmData->sharedMutex.mutexAttr);
	}
}

int CSrvMsgComm::getNetNodes(unsigned int& nodeCount)
{
	nodeCount = 0;
	struct in_addr ipInAddr;
	const Key2Value* nodeIPs = CCfg::getValueList("NodeIP");
	const char* localIp = CCfg::getValue("SrvMsgComm", "IP");
	const char* nodePort = NULL;
	while (nodeIPs != NULL)
	{
		if (strcmp(nodeIPs->value, "0") != 0 && strcmp(nodeIPs->value, localIp) != 0)  // 非本地IP
		{
			if (CSocket::toIPInAddr(nodeIPs->value, ipInAddr) != Success) return CfgInvalidIPError;
			nodePort = CCfg::getValue("NodePort", nodeIPs->key);
			if (nodePort == NULL || atoi(nodePort) < MinConnectPort) return NodePortCfgError;
			++nodeCount;
		}	
		nodeIPs = nodeIPs->pNext;
	}
	
	return Success;
}

int CSrvMsgComm::initCfgFile(const char* cfgFile)
{
	// 默认配置！
	const char* cfgFileFullName = (cfgFile != NULL) ? cfgFile : "./config/message_communication.cfg";   // 默认的配置文件全路径名
	const char* key2value[] = {
		"/* 配置文件说明 */\n\n",
		"\n// 日志配置项\n",
        "[Logger]  // 日志配置项\n",
		"DebugLogFile = ./logs/debug/debug\n",
		"ReleaseLogFile = ./logs/release/release\n",
		"OptLogFile = ./logs/opt/opt\n",
		"LogFileSize = 8388608    // 每个日志文件8M大小\n",
		"LogBakFileCount = 20     // 备份文件个数\n",
		"WriteDebugLog = 1        // 非0值打开调试日志，0值关闭调试日志\n\n",
		
		"\n// 服务消息通信中间件配置项\n",
        "[SrvMsgComm]\n",
		"Name = SrvMsgCommInstance      // 消息通信中间件实例名\n",
		"IP = 192.168.1.2               // 本节点IP\n",
		"Port = 60036                   // 本节点通信端口号，最小值不能小于2000，防止和系统端口号冲突\n",
		"ShmSize = 16777216             // 每块共享内存16M大小，最小必须大于2M\n",
		"ShmCount = 32                  // 共享内存块个数，最小必须大于1\n",
		"ActiveConnectTimeOut = 5       // 源端主动建立连接，最长等待时间，单位：秒\n",
		"RevcMsgCount = 100             // 服务收消息策略，即服务每次从单一共享内存通道读取消息的最大个数，最小必须大于0\n\n",
		           
		"\n// 网络连接配置项，各项值必须大于1，否则配置错误\n",
        "[NetConnect]\n",
		"ActiveTimeInterval = 604800    // 连接活跃检测间隔时间，单位秒，超过此间隔时间无数据则关闭连接\n",
		"CheckNoDataTimes = 600         // 检查最大socket无数据的次数（每秒检测一次），超过此最大次数则连接移出消息队列，避免遍历一堆无数据的空连接",
		"HBInterval = 3600              // 心跳检测间隔时间，单位秒\n",
		"HBFailTimes = 10               // 心跳检测连续失败HBFailTimes次后关闭连接\n",
		"MsgQueueSize = 2097152         // 连接消息队列的大小，最小值必须大于等于1024\n",
		"ConnectCount = 16              // 本节点连接个数\n\n",
		
		"\n// 机器节点IP配置\n",
        "[NodeIP]\n",
		"Node0 = 0                // 0 值表示为本节点\n",
		"Node1 = 192.168.1.2\n",
		"Node2 = 192.168.1.4      // 非本节点配置\n\n",
		
		"\n// 非本节点端口号配置\n",
        "[NodePort]\n",
		"Node1 = 60036\n",
		"Node2 = 60036\n\n",
		
		"\n// 各个通信服务实例ID配置，服务实例名到服务ID的映射，ID值必须大于0并且小于MaxServiceId的配置值，否则配置错误\n",
        "[ServiceID]\n",
		"MaxServiceId = 100000     // 该值最大为100000，配置大于100000则配置错误\n",
		"SrvName1 = 1\n",
		"SrvName2 = 2\n",
		"SrvName3 = 3\n\n",
		
		"\n// 服务实例所在的节点配置\n",
        "[ServiceInNode]\n",
		"SrvName1 = Node0\n",
		"SrvName2 = Node1\n",
		"SrvName3 = Node2\n\n",
	};
	
	CCfg::initCfgFile(cfgFileFullName, key2value, (int)(sizeof(key2value) / sizeof(key2value[0])));
	
	// 初始化配置项值
	memset(&m_cfgData, 0, sizeof(m_cfgData));

	// 检查服务ID配置项的值是否正确
	const char* maxSrvId = CCfg::getValue("ServiceID", "MaxServiceId");
	const Key2Value* serviceIdList = CCfg::getValueList("ServiceID");
	if (maxSrvId == NULL || serviceIdList == NULL) return ServiceIDCfgError;

	m_cfgData.MaxServiceId = atoi(maxSrvId);
	if (m_cfgData.MaxServiceId > MaxServiceId || m_cfgData.MaxServiceId <= 1) return ServiceIDCfgError;
	
	unsigned int serviceIdValue = 0;
	while (serviceIdList != NULL)
	{
		serviceIdValue = atoi(serviceIdList->value);
		if (serviceIdValue < 1 || serviceIdValue > m_cfgData.MaxServiceId) return ServiceIDCfgError;
		serviceIdList = serviceIdList->pNext;
	}
	
	// 网络外部节点配置检查
	m_cfgData.NetNodeCount = 0;
	int rc = getNetNodes(m_cfgData.NetNodeCount);
	if (rc != Success) return rc;
	
	// NetConnect 配置项值检查
	const char* netConnectCfg[] = {"MsgQueueSize", "ActiveTimeInterval", "CheckNoDataTimes", "HBInterval", "HBFailTimes", "ConnectCount",};
	for (int i = 0; i < (int)(sizeof(netConnectCfg) / sizeof(netConnectCfg[0])); ++i)
	{
		const char* value = CCfg::getValue("NetConnect", netConnectCfg[i]);
		if (value == NULL || atoi(value) <= 1) return NetConnectCfgError;
		if (i == 0 && atoi(value) < MinMsgQueueSize) return NetConnectCfgError;
	}
	
	// SrvMsgComm 配置项值
	const char* srvMsgCommCfg[] = {"Name", "IP", "Port", "ShmSize", "ShmCount", "ActiveConnectTimeOut", "RevcMsgCount",};
	for (int i = 0; i < (int)(sizeof(srvMsgCommCfg) / sizeof(srvMsgCommCfg[0])); ++i)
	{
		const char* value = CCfg::getValue("SrvMsgComm", srvMsgCommCfg[i]);
		if (value == NULL) return SrvMsgCommCfgError;
		
		switch (i)
		{
			case 0:
			if (strlen(value) <= 1) return SrvMsgCommCfgError;
			strcpy(m_cfgData.Name, value);
			break;
			
			case 1:
			struct in_addr ipInAddr;
			if (CSocket::toIPInAddr(value, ipInAddr) != Success) return CfgInvalidIPError;
			strcpy(m_cfgData.IP, value);
			break;
			
			case 2:
			m_cfgData.Port = (unsigned short)atoi(value);
			if (m_cfgData.Port < MinConnectPort) return SrvMsgCommCfgError;
			break;
			
			case 3:
			m_cfgData.ShmSize = atoi(value);
			if (m_cfgData.ShmSize < MinShmSize) return SrvMsgCommCfgError;
			break;
			
			case 4:
			m_cfgData.ShmCount = atoi(value);
			if (m_cfgData.ShmCount <= 1 || m_cfgData.NetNodeCount > m_cfgData.ShmCount) return SrvMsgCommCfgError;
			break;
			
			case 5:
			m_cfgData.ActiveConnectTimeOut = atoi(value);
			if (m_cfgData.ActiveConnectTimeOut < 1) return SrvMsgCommCfgError;
			break;
			
			case 6:
			m_cfgData.RevcMsgCount = atoi(value);
			if (m_cfgData.RevcMsgCount < 1) return SrvMsgCommCfgError;
			break;
			
			default:
			break;
		}
	}
	
	ReleaseInfoLog("init config file success, MsgCommName = %s, MaxServiceId = %u, ShmSize = %u, ShmCount = %u, RevcMsgCount = %u",
	                m_cfgData.Name, m_cfgData.MaxServiceId, m_cfgData.ShmSize, m_cfgData.ShmCount, m_cfgData.RevcMsgCount);
	
	return Success;
}

// 动态刷新配置数据
void CSrvMsgComm::updateConfig()
{
	ReleaseInfoLog("receive update config notify");
	
	CCfg::reLoadCfg();  // 重新加载配置数据
	unsigned int netNodeCount = 0;
	int rc = getNetNodes(netNodeCount);
	if (rc != Success || netNodeCount > m_cfgData.ShmCount)
	{
		ReleaseErrorLog("net node config error, new node count = %d, shm count = %d, rc = %d",
		netNodeCount, m_cfgData.ShmCount, rc);
		return;
	}
	
	if (m_cfgData.NetNodeCount >= netNodeCount)
	{
		ReleaseWarnLog("now only support add net node, current node count = %d, new config node count = %d", m_cfgData.NetNodeCount, netNodeCount);
		return;
	}
	
	m_cfgData.NetNodeCount = netNodeCount;  // 更新网络节点数
	const char* localIp = CCfg::getValue("SrvMsgComm", "IP");
	const Key2Value* nodeIPs = CCfg::getValueList("NodeIP");
	unsigned int nodeIp = 0;
	unsigned int nodePort = 0;
	
	MsgHandlerList* msgHandler = NULL;
	HandlerIndex netHandlerIdx = 0;
	while (nodeIPs != NULL)
	{
		if (strcmp(nodeIPs->value, "0") != 0 && strcmp(nodeIPs->value, localIp) != 0)  // 非本地IP
		{
			nodeIp = CSocket::toIPInt(nodeIPs->value);
			nodePort = atoi(CCfg::getValue("NodePort", nodeIPs->key));
					
			netHandlerIdx = m_shmData->netMsgHanders;
			while (netHandlerIdx != 0)
			{
				msgHandler = (MsgHandlerList*)((char*)m_shmData + netHandlerIdx);
				netHandlerIdx = msgHandler->next;
				
				if (msgHandler->readerNode.id == 0)  // 之前不存在该节点则新增加入队列
				{
					msgHandler->readerNode.ipPort.ip = nodeIp;
					msgHandler->readerNode.ipPort.port = nodePort;
					msgHandler->status = InitConnect;
		            msgHandler->connId = 0;
					msgHandler->readMsg = 0;
					
					ReleaseInfoLog("add new net node success, ip = %s, port = %d", nodeIPs->value, nodePort);
					break;
				}

				if (msgHandler->readerNode.ipPort.ip == nodeIp && msgHandler->readerNode.ipPort.port == nodePort) break;
			}
		}	
		nodeIPs = nodeIPs->pNext;
	}
}
	
}


int main(int argc, char* argv[])
{
	if (argc <= 1)
	{
		printf("Need input config file as param!\nUsage example : exefile ../config/srvMsgComm/srvMsgComm.cfg\n");
		return -1;
	}

	NMsgComm::CSrvMsgComm srvMsgCommunication;
	int rc = srvMsgCommunication.createShm(argv[1]);
	if (rc == 0)
	{
		NMsgComm::ST_SrvMsgComm = &srvMsgCommunication;
		srvMsgCommunication.run();
	}
	else
	{
		ReleaseErrorLog("init and open shared memory error, rc = %d", rc);
	}
	
	return 0;
}

