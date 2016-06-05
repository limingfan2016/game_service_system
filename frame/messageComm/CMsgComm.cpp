
/* author : limingfan
 * date : 2015.01.22
 * description : 共享内存服务间消息通信
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "CMsgComm.h"
#include "base/ErrorCode.h"
#include "base/Constant.h"
#include "base/CShm.h"
#include "base/CMessageQueue.h"
#include "connect/CSocket.h"


using namespace NErrorCode;

namespace NMsgComm
{

// 服务间通信公共操作api
int CMsgComm::getFileKey(const char* pathName, const char* srvName, char* fileName, int len)
{
	if (pathName == NULL || *pathName == '\0'
	    || srvName == NULL || *srvName == '\0'
		|| fileName == NULL || len <= 1) return 0;

    int fLen = snprintf(fileName, len - 1, "%s/.%s/%s.keyfile", getenv("HOME"), pathName, srvName);
    fileName[fLen] = '\0';
	return CSharedMemory::getKey(fileName);
}

int CMsgComm::addHandlerToNode(ShmData* shmData, NodeMsgHandler*& msgHandler)
{
	msgHandler = NULL;
	
	{
		// 加锁申请共享内存块
		CSharedLockGuard lockGuard(shmData->sharedMutex);
		if (lockGuard.Success != 0)
		{
			ReleaseErrorLog("lock to get shared memory block for handler failed, error = %d, info = %s",
			lockGuard.Success, strerror(lockGuard.Success));
			return GetSharedMemoryBlockError;
		}

		HandlerIndex handlerIdx = shmData->msgHandlerList;
		if (handlerIdx != 0)
		{
			msgHandler = (NodeMsgHandler*)((char*)shmData + handlerIdx);
			shmData->msgHandlerList = msgHandler->next;
			msgHandler->next = shmData->nodeMsgHandlers;
			shmData->nodeMsgHandlers = handlerIdx;
		}
	}
	
	if (msgHandler == NULL)
	{
		ReleaseErrorLog("no have free shared memory block for handler");
		return GetSharedMemoryBlockError;
	}
	
	return Success;
}

MsgQueueList* CMsgComm::addQueueToHandler(ShmData* shmData, MsgHandlerList* msgHandler)
{
	MsgQueueList* msgQueue = NULL;

	{
		// 加锁申请共享内存块
		CSharedLockGuard lockGuard(shmData->sharedMutex);
		if (lockGuard.Success != 0)
		{
			ReleaseErrorLog("lock to get shared memory block for message queue failed, error = %d, info = %s",
			lockGuard.Success, strerror(lockGuard.Success));
			return NULL;
		}

		QueueIndex queueIdx = shmData->msgQueueList;
		if (queueIdx != 0)
		{
			// 把共享内存队列挂接到目标处理者队列链表
			msgQueue = (MsgQueueList*)((char*)shmData + queueIdx);
			shmData->msgQueueList = msgQueue->next;
			msgQueue->next = msgHandler->readMsg;
			msgHandler->readMsg = queueIdx;
		}
	}
		
	if (msgQueue == NULL) ReleaseErrorLog("no have free shared memory block for message queue");  // 没有共享内存可用了
	
	return msgQueue;
}


CMsgComm::CMsgComm(const char* srvMsgCommCfgFile, const char* srvName) : m_maxServiceId(0), m_srvId(0), m_shmAddr(NULL), m_shmData(NULL),
m_msgQueue(NULL), m_msgHandler(NULL), m_curQueueIdx(0), m_cfg(NULL)
{
	m_srvName[0] = '\0';
	
	if (srvMsgCommCfgFile == NULL || *srvMsgCommCfgFile == '\0'
	    || srvName == NULL || *srvName == '\0') return;

    NEW(m_cfg, CCfg(srvMsgCommCfgFile, NULL, 0));
	if (m_cfg == NULL) return;
	
	const char* maxServiceId = m_cfg->get("ServiceID", "MaxServiceId");
	if (maxServiceId == NULL) return;
	m_maxServiceId = atoi(maxServiceId);
	if (m_maxServiceId > MaxServiceId || m_maxServiceId <= 1) return;

	const char* srvId = m_cfg->get("ServiceID", srvName);
	if (srvId == NULL) return;
	m_srvId = atoi(srvId);
	if (m_srvId < 1 || m_srvId > m_maxServiceId) return;
	strcpy(m_srvName, srvName);
	
	m_srvNameId = m_cfg->getList("ServiceID");
	
	NEW_ARRAY(m_msgQueue, MsgQueueList*[m_maxServiceId]);
	memset(m_msgQueue, 0, sizeof(MsgQueueList*) * m_maxServiceId);
	
	memset(&m_srvMsgHeader, 0, sizeof(m_srvMsgHeader));
	m_srvMsgHeader.netPkgHeader.type = MSG;
	m_srvMsgHeader.netMsgHeader.writerId = htonl(m_srvId);
}

CMsgComm::~CMsgComm()
{
	m_maxServiceId = 0;
	m_srvId = 0;
	m_srvName[0] = '\0';
	m_shmAddr = NULL;
	m_shmData = NULL;
	m_srvNameId = NULL;
	DELETE_ARRAY(m_msgQueue);
	m_msgHandler = NULL;
	m_curQueueIdx = 0;
	DELETE(m_cfg);
	memset(&m_srvMsgHeader, 0, sizeof(m_srvMsgHeader));
}

int CMsgComm::init()
{
	if (m_srvId < 1 || m_srvId > m_maxServiceId) return ServiceIDCfgError;

    // 锁定服务对应的关键文件
    // 同一块共享内存只能挂接一个服务实例，防止重复启动服务实例
    int srvFd = -1;
    char keyFile[MaxFullLen] = {0};
	const char* pathName = m_cfg->get("SrvMsgComm", "Name");
	if (CMsgComm::getFileKey(pathName, getSrvName(), keyFile, MaxFullLen) == 0) return GetSrvFileKeyError;
	int rc = CSharedMemory::lockFile(keyFile, 0, srvFd);  
    if (rc != Success)
	{
		ReleaseErrorLog("lock service instance key file failed, name = %s, file = %s, error = %d",
		getSrvName(), keyFile, rc);
	    return rc;	
	}
	
	// 获取共享内存key值
    int shmKey = CMsgComm::getFileKey(pathName, pathName, keyFile, MaxFullLen);
    if(shmKey == 0) return CreateKeyFileFailed;
	
    // 挂接共享内存
	int shmId = -1;
	char* pShm = NULL;
	rc = CSharedMemory::atShm(shmKey, shmId, pShm);
	if (rc != Success) return rc;

    // 初始化共享内存数据
    m_shmData = (ShmData*)pShm;
	m_shmAddr = pShm;
	const unsigned int shmCfgSize = atoi(m_cfg->get("SrvMsgComm", "ShmSize")) * atoi(m_cfg->get("SrvMsgComm", "ShmCount"));
	if (m_shmData->msgQueueSize != shmCfgSize)
	{
		ReleaseWarnLog("wait for service message communication instance init shared memory");
		sleep(1);  // 等待消息通信中间件服务初始化共享内存完毕
		if (m_shmData->msgQueueSize != shmCfgSize) return SharedMemoryDataError;
	}
	
	// 必须要遍历查找，在进程退出（之前已经存在该共享内存消息队列了）重启的情况下，必须使用之前已经建立好的共享内存消息队列！
	HandlerIndex handlerIdx = m_shmData->nodeMsgHandlers;
	NodeMsgHandler* msgHandler = NULL;
	while (handlerIdx != 0)
	{
		msgHandler = (NodeMsgHandler*)(m_shmAddr + handlerIdx);
		if (msgHandler->readerId == m_srvId) break;
		handlerIdx = msgHandler->next;
	}
	
	if (handlerIdx == 0)
	{
	    rc = CMsgComm::addHandlerToNode(m_shmData, msgHandler);  // 不存在则申请内存块挂接到队列头部
		if (rc != Success)
		{
			ReleaseErrorLog("add handler to shared memory node failed, service name = %s", getSrvName());
			return rc;	
		}
	}
	
	msgHandler->readerId = m_srvId;
	m_msgHandler = msgHandler;
	
	ReleaseInfoLog("open shared memory, pid = %lu, config size = %d, key file = %s, key = 0x%x, shmId = %d\n",
	                getpid(), shmCfgSize, keyFile, shmKey, shmId);

	return Success;
}

void CMsgComm::unInit()
{
	// now do nothing
}

CCfg* CMsgComm::getCfg()
{
	return m_cfg;
}

void CMsgComm::reLoadCfg()
{
	if (m_cfg != NULL)
	{
		m_cfg->reLoad();
		m_srvNameId = m_cfg->getList("ServiceID");
	}
}

const char* CMsgComm::getSrvName()
{
	return m_srvName;
}

unsigned int CMsgComm::getSrvId(const char* srvName)
{
	if (srvName == NULL || *srvName == '\0') return 0;

	const Key2Value* pFind = m_srvNameId;
	while (pFind != NULL)
	{
		if (strcmp(pFind->key, srvName) == 0)
		{
			unsigned int srvId = atoi(pFind->value);
	        return (srvId < 1 || srvId > m_maxServiceId) ? 0 : srvId;
		}
		pFind = pFind->pNext;
	}	
	
	return 0;	
}


int CMsgComm::send(const unsigned int srvId, const char* data, const unsigned int len)
{
	MsgQueueList* msgQueue = getReceiverMsgQueue(srvId);
	if (msgQueue != NULL)
	{
		// 本地节点的消息直接写到目标共享内存队列
		if (msgQueue->readerId != 0) return CMessageQueue::write(m_shmAddr + msgQueue->queue, msgQueue->header, data, len);
		
		// 网络外部节点的消息则添加网络数据包头部、服务消息头部等信息
		static const unsigned int SrvMsgHeaderLen = sizeof(m_srvMsgHeader);
		const int allLen = SrvMsgHeaderLen + len;
		char* dataBuff = NULL;
		int rc = CMessageQueue::beginWriteBuff(m_shmAddr + msgQueue->queue, msgQueue->header, dataBuff, allLen);
		if (rc != Success) return rc;
		
		// 为了高效率，减少数据拷贝次数，这里直接写网络层数据包头部数据
		m_srvMsgHeader.netPkgHeader.len = htonl(allLen);
	    m_srvMsgHeader.netMsgHeader.readerId = htonl(srvId);
		memcpy(dataBuff, &m_srvMsgHeader, SrvMsgHeaderLen);  // 消息头部数据
		memcpy(dataBuff + SrvMsgHeaderLen, data, len);       // 用户数据
		CMessageQueue::endWriteBuff(m_shmAddr + msgQueue->queue, msgQueue->header, dataBuff, allLen);  // 提交数据到共享内存
		return Success;
	}
	
	return NotFoundService;
}

int CMsgComm::recv(char* data, unsigned int& len)
{
	MsgQueueList* msgQueue = getSrvMsgQueue();
	if (msgQueue != NULL)
	{
		m_curQueueIdx = msgQueue->next;  // 切换到下一个队列
		return CMessageQueue::read(m_shmAddr + msgQueue->queue, msgQueue->header, data, (int&)len);
	}
	
	return NotServiceMsg;
}

int CMsgComm::beginRecv(char*& data, int& len, void*& cb)
{
	MsgQueueList* msgQueue = getSrvMsgQueue();
	if (msgQueue != NULL)
	{
		cb = msgQueue;
	    return CMessageQueue::beginReadBuff(m_shmAddr + msgQueue->queue, msgQueue->header, data, len);
	}
	
	return NotServiceMsg;
}

void CMsgComm::endRecv(const char* data, const int len, void* cb)
{
	MsgQueueList* msgQueue = (MsgQueueList*)cb;
	m_curQueueIdx = msgQueue->next;  // 切换到下一个队列
	CMessageQueue::endReadBuff(m_shmAddr + msgQueue->queue, msgQueue->header, data, len);
}


MsgQueueList* CMsgComm::getSrvMsgQueue()
{
	if (m_curQueueIdx == 0)
	{
		m_curQueueIdx = m_msgHandler->readMsg;
		if (m_curQueueIdx == 0) return NULL;
	}
	
	QueueIndex curIdx = m_curQueueIdx;
	MsgQueueList* msgQueue = (MsgQueueList*)(m_shmAddr + m_curQueueIdx);
	while (CMessageQueue::readyRead(m_shmAddr + msgQueue->queue, msgQueue->header) != Success)
	{
		m_curQueueIdx = (msgQueue->next != 0) ? msgQueue->next : m_msgHandler->readMsg;  // 切换到下一个队列
		if (m_curQueueIdx == curIdx) return NULL;  // 回到循环起点了则退出循环，当前没有消息可处理
		msgQueue = (MsgQueueList*)(m_shmAddr + m_curQueueIdx);
	}

	return msgQueue;
}

MsgQueueList* CMsgComm::getReceiverMsgQueue(const unsigned int readerId)
{
	if (readerId > m_maxServiceId || readerId < 1)
	{
		ReleaseErrorLog("readerId = %d is invalid, the value must be less than or equal to %d and bigger than 0!", readerId, m_maxServiceId);
		return NULL;
	}
	
	MsgQueueList* readMsg = m_msgQueue[readerId - 1];
	if (readMsg != NULL) return readMsg;
	
	// 开始查找消息的属主，目标队列
	const char* srvName = m_cfg->getKeyByValue("ServiceID", readerId);
	if (srvName == NULL)
	{
		ReleaseErrorLog("can not find the service name by id = %d", readerId);
		return NULL;
	}
	
	const char* srvNode = m_cfg->get("ServiceInNode", srvName);
	if (srvNode == NULL)
	{
		ReleaseErrorLog("can not find the service node by name = %s", srvName);
		return NULL;
	}
	
	const char* nodeIp = m_cfg->get("NodeIP", srvNode);
	if (nodeIp == NULL)
	{
		ReleaseErrorLog("can not find the node ip by node = %s", srvNode);
		return NULL;
	}
	
	// 找到该消息的属主
	MsgHandlerList* msgHandler = NULL;
	HandlerIndex handlerIdx = 0;
	bool isSrvInCurrentNode = true;
	if (strcmp(nodeIp, "0") == 0 || strcmp(nodeIp, m_cfg->get("SrvMsgComm", "IP")) == 0)
	{
		// 本地节点的服务
	    handlerIdx = m_shmData->nodeMsgHandlers;
		while (handlerIdx != 0)
		{
			msgHandler = (MsgHandlerList*)(m_shmAddr + handlerIdx);
			if (msgHandler->readerId == readerId) break;  // 找到消息属主了
			handlerIdx = msgHandler->next;
		}
	}
	else
	{
		// 网络外部节点的服务
		isSrvInCurrentNode = false;
		const char* nodePort = m_cfg->get("NodePort", srvNode);
		if (nodePort == NULL)
		{
			ReleaseErrorLog("can not find the node port by node = %s", srvNode);
			return NULL;
		}
		
		const unsigned int peerIp = CSocket::toIPInt(nodeIp);
		const unsigned short peerPort = atoi(nodePort);
		handlerIdx = m_shmData->netMsgHanders;
		while (handlerIdx != 0)
		{
			msgHandler = (MsgHandlerList*)(m_shmAddr + handlerIdx);
			if (peerIp == msgHandler->readerNode.ipPort.ip && peerPort == msgHandler->readerNode.ipPort.port) break;  // 找到消息属主了
			handlerIdx = msgHandler->next;
		}
	}
	
	if (handlerIdx == 0)
	{
		ReleaseErrorLog("can not find the msg handler by readerId = %d", readerId);
		return NULL;  // 找不到消息属主
	}
	
	// 必须要遍历查找，在进程退出（之前已经存在该共享内存消息队列了）重启的情况下，必须使用之前已经建立好的共享内存消息队列！
	QueueIndex queueIdx = msgHandler->readMsg;
	while (queueIdx != 0)
	{
		readMsg = (MsgQueueList*)(m_shmAddr + queueIdx);
		if (readMsg->writerId == m_srvId) break;  // 消息源匹配
		queueIdx = readMsg->next;
	}
	
	if (queueIdx == 0)
	{
		readMsg = CMsgComm::addQueueToHandler(m_shmData, msgHandler);
		if (readMsg == NULL)
		{
			ReleaseErrorLog("add message queue to handler failed, readerId = %d", readerId);
			return NULL;
		}

		readMsg->writerId = m_srvId;
		readMsg->readerId = isSrvInCurrentNode ? readerId : 0;  // 0值表示发往网络的消息
	}

	m_msgQueue[readerId - 1] = readMsg;
	return readMsg;
}


// 创建&销毁ISrvMsgComm对象实例
// srvMsgCommCfgFile : 消息通信配置文件
// srvName : 通信的本服务名称，在消息通信配置文件中配置
ISrvMsgComm* createSrvMsgComm(const char* srvMsgCommCfgFile, const char* srvName)
{
	if (srvMsgCommCfgFile == NULL || *srvMsgCommCfgFile == '\0'
	    || srvName == NULL || *srvName == '\0') return NULL;
		
	ISrvMsgComm* instance = NULL;
	NEW(instance, CMsgComm(srvMsgCommCfgFile, srvName));
	return instance;
}

void destroySrvMsgComm(ISrvMsgComm*& instance)
{
	DELETE(instance);
}

}

