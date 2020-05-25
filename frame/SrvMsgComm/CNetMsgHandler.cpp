
/* author : admin
 * date : 2015.01.19
 * description : 网络消息收发处理
 */

#include <string.h>
#include <arpa/inet.h>

#include "CNetMsgHandler.h"
#include "base/ErrorCode.h"
#include "base/CCfg.h"
#include "base/CShm.h"
#include "base/CMessageQueue.h"
#include "connect/CSocket.h"
#include "messageComm/CMsgComm.h"


using namespace NErrorCode;

namespace NMsgComm
{

static const unsigned int ActiveConnectBlockCount = 32;


CNetMsgHandler::CNetMsgHandler(ShmData* shmData, CfgData* cfgData) : m_shm(shmData), m_cfg(cfgData)
{
	m_shmAddr = (char*)m_shm;
	
	m_buffHeader.header = (char*)&m_netMsgHeader;
	m_buffHeader.len = sizeof(m_netMsgHeader);
	memset(m_buffHeader.header, 0, m_buffHeader.len);

	NEW_ARRAY(m_nodeMsgQueueList, MsgQueueList*[m_cfg->MaxServiceId]);
	memset(m_nodeMsgQueueList, 0, sizeof(MsgQueueList*) * m_cfg->MaxServiceId);

	m_curNetHandlerIdx = 0;
	m_curNetQueueIdx = 0;
	
	NEW_ARRAY(m_netMsgHandlers, MsgHandlerList*[m_cfg->ShmCount]);
	memset(m_netMsgHandlers, 0, sizeof(MsgHandlerList*) * m_cfg->ShmCount);
	
	NEW(m_memForActiveConn, CMemManager(ActiveConnectBlockCount, ActiveConnectBlockCount, sizeof(ActiveConnect)));
	m_activeConnects = NULL;
}

CNetMsgHandler::~CNetMsgHandler()
{
	m_shmAddr = NULL;
	m_shm = NULL;
	m_cfg = NULL;
	
	memset(m_buffHeader.header, 0, m_buffHeader.len);
	memset(&m_buffHeader, 0, sizeof(m_buffHeader));
	
	DELETE_ARRAY(m_nodeMsgQueueList);
	
	m_curNetHandlerIdx = 0;
	m_curNetQueueIdx = 0;
	DELETE_ARRAY(m_netMsgHandlers);
	
	DELETE(m_memForActiveConn);
	m_activeConnects = NULL;
}


// 获取当前节点对应消息处理者的目标队列，写入网络消息
MsgQueueList* CNetMsgHandler::getNodeMsgQueue(const unsigned int readerId)
{
	if (readerId > m_cfg->MaxServiceId || readerId < 1)
	{
		ReleaseErrorLog("readerId = %d is invalid, the value must be less than or equal to %d and bigger than 0!", readerId, m_cfg->MaxServiceId);
		return NULL;
	}
	
	MsgQueueList* readMsg = m_nodeMsgQueueList[readerId - 1];
	if (readMsg != NULL) return readMsg;
	
	// 找到该消息的属主
	NodeMsgHandler* nodeMsgHandlers = NULL;
	HandlerIndex handlerIdx = m_shm->nodeMsgHandlers;
	while (handlerIdx != 0)
	{
		nodeMsgHandlers = (NodeMsgHandler*)(m_shmAddr + handlerIdx);
		if (nodeMsgHandlers->readerId == readerId) break;
		handlerIdx = nodeMsgHandlers->next;
	}
	
	if (handlerIdx == 0)
	{
		ReleaseErrorLog("can not find the msg readerId = %d", readerId);
		return NULL;  // 找不到消息属主
	}
	
	// 找到该消息的所属网络消息队列
	// 必须要遍历查找，在进程退出（之前已经存在该共享内存消息队列了）重启的情况下，必须使用之前已经建立好的共享内存消息队列！
	QueueIndex queueIdx = nodeMsgHandlers->readMsg;
	while (queueIdx != 0)
	{
		readMsg = (MsgQueueList*)(m_shmAddr + queueIdx);
		if (readMsg->writerId == 0) break;
		queueIdx = readMsg->next;
	}
	
	if (queueIdx == 0)
	{
		readMsg = CMsgComm::addQueueToHandler(m_shm, nodeMsgHandlers);
		if (readMsg == NULL)
		{
			ReleaseErrorLog("add message queue to handler failed, readerId = %d", readerId);
			return NULL;
		}

		readMsg->readerId = readerId;
		readMsg->writerId = 0;  // 0值表示来自网络的消息
	}
	
	m_nodeMsgQueueList[readerId - 1] = readMsg;
	return readMsg;
}

// 获取当前节点消息处理者发往外部网络节点的消息队列
MsgQueueList* CNetMsgHandler::getNetHandlerMsgQueue(uuid_type& connId)
{
	MsgQueueList* msgQueue = NULL;
    MsgHandlerList* msgHandler = NULL;
	while (m_curNetQueueIdx != 0)
	{
		msgQueue = (MsgQueueList*)(m_shmAddr + m_curNetQueueIdx);
		if (CMessageQueue::readyRead(m_shmAddr + msgQueue->queue, msgQueue->header) == Success)
		{
            msgHandler = m_netMsgHandlers[m_curNetHandlerIdx];
            if (msgHandler != NULL)
            {
                connId = msgHandler->connId;
                return msgQueue;
            }
		}
		m_curNetQueueIdx = msgQueue->next;
	}
	
	return NULL;
}

// 获取当前节点消息处理者发往外部网络节点的消息队列
MsgQueueList* CNetMsgHandler::getNetMsgQueue(uuid_type& connId)
{
	MsgQueueList* msgQueue = getNetHandlerMsgQueue(connId);
    if (msgQueue != NULL) return msgQueue;
	
	MsgHandlerList* msgHandler = NULL;
	for (unsigned int count = 0; count < m_cfg->NetNodeCount; ++count)
	{
		m_curNetHandlerIdx = (m_curNetHandlerIdx + 1) % m_cfg->NetNodeCount;  // 循环遍历所有正常的节点，直到回到起点
		msgHandler = m_netMsgHandlers[m_curNetHandlerIdx];
		if (msgHandler != NULL)
		{
			m_curNetQueueIdx = msgHandler->readMsg;
			msgQueue = getNetHandlerMsgQueue(connId);
            if (msgQueue != NULL) return msgQueue;
		}
	}
	
	return NULL;
}


// 返回NULL值则不会读取数据；返回非NULL值则先从连接缓冲区读取len的数据长度，做为getWriteBuffer接口的bufferHeader输入参数
// 该接口一般做为先读取消息头部，用于解析消息头部使用
BufferHeader* CNetMsgHandler::getBufferHeader()
{
	return &m_buffHeader;  // 让网络层先填写消息头部数据
}

// 从逻辑层获取数据缓冲区，把从连接读到的数据写到该缓冲区	
// 返回值为 (char*)-1 则表示直接丢弃该数据；返回值为非NULL则读取数据；返回值为NULL则不读取数据
char* CNetMsgHandler::getWriteBuffer(const int len, const uuid_type connId, BufferHeader* bufferHeader, void*& cb)
{
    /*
    {
        const static char* matchIp = "1921681";
        const char* localIp = m_cfg->IP;
        unsigned int mIpIdx = 0;
        unsigned int lipIdx = 0;
        while (matchIp[mIpIdx] != '\0')
        {
            if (localIp[lipIdx] == '.')
            {
                ++lipIdx;
                continue;
            }
            
            if (localIp[lipIdx] != matchIp[mIpIdx]) return NULL;
            
            ++mIpIdx;
            ++lipIdx;
        }
    }
    */
 
    
	MsgQueueList* readMsg = getNodeMsgQueue(ntohl(m_netMsgHeader.readerId));
	if (readMsg == NULL)
	{
		ReleaseErrorLog("get msg queue failed, readerId = %u, discard msg len = %d, connect id = %lld, writerId = %u",
		ntohl(m_netMsgHeader.readerId), len, connId, ntohl(m_netMsgHeader.writerId));
		return (char*)-1;  // 获取该消息的属主队列失败，直接丢弃该消息
	}

    char* pShmBuff = NULL;		
	int rc = CMessageQueue::beginWriteBuff(m_shmAddr + readMsg->queue, readMsg->header, pShmBuff, len);
    if (rc != Success)
	{
	    ReleaseErrorLog("begin write data to shared buff failed = %d, dicard msg len = %d, readerId = %u, writerId = %u",
		rc, len, ntohl(m_netMsgHeader.readerId), ntohl(m_netMsgHeader.writerId));
		return (char*)-1;  // 一般情况下是队列满了，失败则必须丢消息，因为消息头已经读出来了，否则会导致数据解析错乱
	}
	
	cb = (void*)readMsg;
	return pShmBuff;
}

void CNetMsgHandler::submitWriteBuffer(char* buff, const int len, const uuid_type connId, void* cb)
{
	MsgQueueList* readMsg = (MsgQueueList*)cb;
	CMessageQueue::endWriteBuff(m_shmAddr + readMsg->queue, readMsg->header, buff, len);  // 提交到共享内存
}


// 从逻辑层获取数据缓冲区，把该数据写入connLogicId对应的连接里
// isNeedWriteMsgHeader 是否需要写入网络数据消息头部信息，默认会写消息头信息
// isNeedWriteMsgHeader 如果逻辑层调用者填值为false，则调用者必须自己写入网络消息头部数据，即返回的可读buff中已经包含了网络消息头数据
char* CNetMsgHandler::getReadBuffer(int& len, uuid_type& connId, bool& isNeedWriteMsgHeader, void*& cb)
{
    /*
    {
        const static char* matchIp = "1921681";
        const char* localIp = m_cfg->IP;
        unsigned int mIpIdx = 0;
        unsigned int lipIdx = 0;
        while (matchIp[mIpIdx] != '\0')
        {
            if (localIp[lipIdx] == '.')
            {
                ++lipIdx;
                continue;
            }
            
            if (localIp[lipIdx] != matchIp[mIpIdx]) return NULL;
            
            ++mIpIdx;
            ++lipIdx;
        }
    }
    */
    
    
	MsgQueueList* netMsgQueue = getNetMsgQueue(connId);
	if (netMsgQueue == NULL) return NULL;

	isNeedWriteMsgHeader = false;  // 为了高效率，减少数据拷贝次数，上层调用端已经写了头部数据了
	cb = (void*)netMsgQueue;
	
    char* pShmBuff = NULL;
    CMessageQueue::beginReadBuff(m_shmAddr + netMsgQueue->queue, netMsgQueue->header, pShmBuff, len);
	return pShmBuff;
}

void CNetMsgHandler::submitReadBuffer(char* buff, const int len, const uuid_type connId, void* cb)
{
	MsgQueueList* netMsgQueue = (MsgQueueList*)cb;
	CMessageQueue::endReadBuff(m_shmAddr + netMsgQueue->queue, netMsgQueue->header, buff, len);
	m_curNetQueueIdx = netMsgQueue->next;  // 切换到下一条消息队列读取消息
}


// 本地端主动创建连接 peerIp&peerPort 为连接对端的IP、端口号
// peerPort : 必须大于2000，防止和系统端口号冲突
// timeOut : 最长timeOut秒后连接还没有建立成功则关闭，并回调返回创建连接超时；单位秒
ActiveConnect* CNetMsgHandler::getConnectData()
{
    // 1）先处理无效了的连接
	ActiveConnect* activeConnect = m_activeConnects;
	ActiveConnect* pDelete = NULL;
	ActiveConnect* pPre = NULL;
	while (activeConnect != NULL)
	{
		if (activeConnect->peerIp[0] == '\0' && activeConnect->peerPort == 0)  // 删除无效连接信息
		{
			pDelete = activeConnect;
			if (pPre != NULL) pPre->next = activeConnect->next;
			else m_activeConnects = activeConnect->next;
			activeConnect = activeConnect->next;
		    m_memForActiveConn->put((char*)pDelete);
			continue;
		}
		
		pPre = activeConnect;
		activeConnect = activeConnect->next;
	}
	
	// 2）再查看是否存在等待建立连接的消息
	HandlerIndex handlerIdx = m_shm->netMsgHanders;
	NetMsgHandler* netMsgHandler = NULL;
	while (handlerIdx != 0)
	{
		netMsgHandler = (NetMsgHandler*)(m_shmAddr + handlerIdx);
		handlerIdx = netMsgHandler->next;
		
		if (netMsgHandler->readerNode.id == 0) break;  // 无节点数据了
		
		if (netMsgHandler->readMsg != 0 && netMsgHandler->status != NormalConnect)  // 存在消息发送并且未连接
		{
			activeConnect = m_activeConnects;
			while (activeConnect != NULL)
			{
				if (CSocket::toIPInt(activeConnect->peerIp) == netMsgHandler->readerNode.ipPort.ip) break;  // 已经存在了，避免重复建立相同连接
				activeConnect = activeConnect->next;
			}
			
			if (activeConnect == NULL)  // 没找到则建立主动连接
			{
				activeConnect = (ActiveConnect*)m_memForActiveConn->get();
				if (activeConnect == NULL) break;  // 居然没内存了。。。不可思议
				
				memset(activeConnect, 0, sizeof(ActiveConnect));
				struct in_addr addrIp;
				addrIp.s_addr = netMsgHandler->readerNode.ipPort.ip;
				strcpy(activeConnect->peerIp, CSocket::toIPStr(addrIp));    // IP
				activeConnect->peerPort = netMsgHandler->readerNode.ipPort.port;   // port
				activeConnect->timeOut = m_cfg->ActiveConnectTimeOut;          // 最长等待时间
				activeConnect->userCb = NULL;
				activeConnect->userId = 0;
				activeConnect->next = m_activeConnects;
				m_activeConnects = activeConnect;
				
				ReleaseInfoLog("do create active connect, ip = %s, port = %d", activeConnect->peerIp, activeConnect->peerPort);
			}
		}
	}
	
	return m_activeConnects;
}

// 主动连接建立的时候调用
// conn 为连接对应的数据，peerIp&peerPort 为连接对端的IP地址和端口号
// rtVal : 返回连接是否创建成功
// 网络管理层根据逻辑层返回码判断是否需要关闭该连接，返回码为 [CloseConnect, SendDataCloseConnect] 则关闭该连接
ReturnValue CNetMsgHandler::doCreateConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, ReturnValue rtVal, void*& cb)
{
	// 无论连接建立成功与否，及时清除连接信息，避免重复发起相同的连接
	ActiveConnect* activeConnect = m_activeConnects;
	while (activeConnect != NULL)
	{
		if (strcmp(peerIp, activeConnect->peerIp) == 0 && peerPort == activeConnect->peerPort)
		{
			activeConnect->peerIp[0] = '\0';
			activeConnect->peerPort = 0;
			break;
		}
		activeConnect = activeConnect->next;
	}
	
	if (rtVal == OptSuccess)
	{
		// 连接建立成功了
		const unsigned int ip = CSocket::toIPInt(peerIp);
        HandlerIndex handlerIdx = m_shm->netMsgHanders;	
		NetMsgHandler* netMsgHandler = NULL;
		while (handlerIdx != 0)
		{
			netMsgHandler = (NetMsgHandler*)(m_shmAddr + handlerIdx);
		    handlerIdx = netMsgHandler->next;
			if(ip == netMsgHandler->readerNode.ipPort.ip && peerPort == netMsgHandler->readerNode.ipPort.port)
			{
				// 同时挂接到处理者队列
				netMsgHandler->status = NormalConnect;
				netMsgHandler->connId = conn->id;
				for (unsigned int i = 0; i < m_cfg->NetNodeCount; ++i)
				{
					if (m_netMsgHandlers[i] == NULL)  // 找到空闲位置
					{
						m_netMsgHandlers[i] = netMsgHandler;
						cb = (void*)(unsigned long)(i + 1);  // 保证指针值不为0
						break;
					}
				}
				
				ReleaseInfoLog("create active connect success, ip = %s, port = %d, connId = %lld", peerIp, peerPort, netMsgHandler->connId);
				return OptSuccess;
			}
		}
		
		ReleaseErrorLog("connect success, but not find the msg handler, ip = %s, port = %d", peerIp, peerPort);  // 居然找不到属主队列
		return CloseConnect;  // 直接关闭连接
	}
	else
	{
		ReleaseErrorLog("connect failed, ip = %s, port = %d, return code = %d", peerIp, peerPort, rtVal);
	}
	
	return OptSuccess;
}
	
// 连接建立的时候调用
// conn 为连接对应的数据，peerIp & peerPort 为连接对端的IP地址和端口号
// 网络管理层根据逻辑层返回码判断是否需要关闭该连接，返回码为 [CloseConnect, SendDataCloseConnect] 则关闭该连接
ReturnValue CNetMsgHandler::onCreateConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, void*& cb)
{
	ReleaseInfoLog("on create connect, ip = %s, port = %d, connId = %lld", peerIp, peerPort, conn->id);
	
	const char* localIp = CCfg::getValue("SrvMsgComm", "IP");
	const Key2Value* findIP = CCfg::getValueList("NodeIP");
	while (findIP != NULL)
	{
		if (strcmp(findIP->value, peerIp) == 0 || strcmp(localIp, peerIp) == 0) return ReturnValue::OptSuccess;
		
		findIP = findIP->pNext;
	}
	
	ReleaseErrorLog("create passive connect but can not find the config node ip, so close the connect, ip = %s, port = %d, connId = %lld, fd = %d",
	peerIp, peerPort, conn->id, conn->fd);
	return ReturnValue::CloseConnect;
}

void CNetMsgHandler::onRemoveConnect(void* cb)
{
	if (cb == NULL) return;
	
	int idx = (unsigned long)cb - 1;
	if (idx < 0 || idx >= (int)m_cfg->NetNodeCount)
	{
		ReleaseWarnLog("the call back idx value is error = %d", idx);
		return;
	}
	
	NetMsgHandler* netMsgHandler = m_netMsgHandlers[idx];
	m_netMsgHandlers[idx] = NULL;
	if (netMsgHandler != NULL)
	{
		netMsgHandler->status = InitConnect;  // 连接被关闭了
		netMsgHandler->connId = 0;
	}
}

// 通知逻辑层对应的逻辑连接已被关闭
void CNetMsgHandler::onClosedConnect(Connect* conn, void* cb)
{
	onRemoveConnect(cb);
	ReleaseWarnLog("closed net connect, id = %lld, cb = %d", conn->id, (unsigned long)cb - 1);
}

// 通知逻辑层connId对应的逻辑连接无效了，可能找不到，或者异常被关闭了，不能读写数据等等
void CNetMsgHandler::onInvalidConnect(const uuid_type connId, void* cb)
{
	onRemoveConnect(cb);
	ReleaseWarnLog("invalid net connect, id = %lld, cb = %d", connId, (unsigned long)cb - 1);
}


// 直接从从逻辑层读写消息数据，多了一次中间数据拷贝，性能较低，暂不实现
ReturnValue CNetMsgHandler::readData(char* data, int& len, uuid_type& connId)
{
	return OptFailed;
}

ReturnValue CNetMsgHandler::writeData(const char* data, const int len, const uuid_type connId)
{
	return OptFailed;
}

// 设置连接收发数据接口对象，暂不实现
void CNetMsgHandler::setConnMgrInstance(IConnectMgr* instance)
{
	// do nothing
}

}



