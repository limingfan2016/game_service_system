
/* author : limingfan
 * date : 2014.12.05
 * description : 网络连接管理
 */
 
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <vector>

#include "CConnectManager.h"
#include "CDataHandler.h"
#include "CDataTransmit.h"
#include "ILogicHandler.h"
#include "base/Constant.h"
#include "base/ErrorCode.h"
#include "base/CProcess.h"


using namespace NCommon;
using namespace NErrorCode;

namespace NConnect
{

// 错误的主动连接下次可以执行建立连接的时间间隔，单位秒，错误的连接不能总是拼命的循环建立、然后错误、接着再建立。。。
static const unsigned int MinErrorActiveConnectIntervalSecs = 6;
static const unsigned int MaxErrorActiveConnectIntervalSecs = 60;

// 检查最大socket无数据的次数，超过此最大次数则连接移出消息队列，避免遍历一堆无数据的空连接
static const unsigned short MaxCheckTimesOfNoSocketData = 60;


// 忽略管道SIGPIPE信号
static void SignalHandler(int sigNum, siginfo_t* sigInfo, void* context)
{
	ReleaseWarnLog("receive signal = %d", sigNum);
}


CConnectManager::CConnectManager(const char* ip, const int port, ILogicHandler* logicHandler) : m_tcpListener(ip, port), m_logicHandler(logicHandler)
{
	m_loginConnectList = NULL;
	m_msgConnectList = NULL;
	m_lastCheckTime = 0;
	
	m_memForRead = NULL;
	m_memForWrite = NULL;
	m_connMemory = NULL;
	m_connId = 0;
	
	m_listenNum = 0;
	m_hbInterval = 0;           // 心跳检测间隔时间，单位秒
	m_hbFailedTimes = 0;        // 心跳检测连续失败hbFailedTimes次后关闭连接
	m_activeInterval = 0;       // 活跃检测间隔时间，单位秒
	
	m_status = 0;
	m_synNotify.status = parallel;
}

CConnectManager::~CConnectManager()
{
	clear();
}


int CConnectManager::start(const ServerCfgData& cfgData)
{
	int rc = doStart(cfgData);
	if (rc != Success)
	{
		DELETE(m_memForWrite);
		DELETE(m_connMemory);
		DELETE(m_memForRead);
		ReleaseErrorLog("server start failed, error = %d\n", rc);
	}
	return rc;
}

int CConnectManager::doStart(const ServerCfgData& cfgData)
{
	const unsigned int minSize = 1024;  // 缓冲区最小长度
	
	ReleaseInfoLog("server start memory pool count = %d, step = %d, size = %d, minSize = %d, listenNum = %d, hbInterval = %d, hbFailedTimes = %d, activeInterval = %d, connect object cache = %d.",
	cfgData.count, cfgData.step, cfgData.size, minSize, cfgData.listenNum,
    cfgData.hbInterval, cfgData.hbFailedTimes, cfgData.activeInterval, cfgData.listenMemCache);
	
	if (m_logicHandler == NULL || cfgData.listenMemCache <= 1 || cfgData.count <= 1 || cfgData.step <= 1 || cfgData.size < minSize
	    || cfgData.listenNum <= 1 || cfgData.hbInterval <= 1 || cfgData.hbFailedTimes < 1 || cfgData.activeInterval <= 1) return InvalidParam;
	
    NEW(m_connMemory, CMemManager(cfgData.listenMemCache, cfgData.listenMemCache, sizeof(Connect)));  // 创建连接使用的内存池
	if (m_connMemory == NULL) return CreateMemPoolFailed;
	
	NEW(m_memForWrite, CMemManager(cfgData.count, cfgData.step, cfgData.size));  // 创建写缓冲区使用的内存池
	if (m_memForWrite == NULL) return CreateMemPoolFailed;
	
	NEW(m_memForRead, CMemManager(cfgData.count, cfgData.step, cfgData.size));  // 创建读缓冲区使用的内存池
	if (m_memForRead == NULL) return CreateMemPoolFailed;
	
	// 创建tcp监听的连接
	int rc = m_tcpListener.create(cfgData.listenNum);
	if (rc != Success)
	{
		m_tcpListener.destroy();
		return rc;
	}

	m_listenNum = cfgData.listenNum;
	m_hbInterval = cfgData.hbInterval;
	m_hbFailedTimes = cfgData.hbFailedTimes;
	m_activeInterval = cfgData.activeInterval;
	ReleaseInfoLog("server start success.");
	
	return Success;
}

// 启动连接管理服务，waitTimeout：IO监听数据等待超时时间，单位毫秒
// isActive：是否主动收发网络数据
int CConnectManager::run(const int connectCount, const int waitTimeout, DataHandlerType dataHandlerType)
{
	ReleaseInfoLog("server run param connectCount = %d, waitTimeout = %d, dataHandlerType = %d", connectCount, waitTimeout, dataHandlerType);
	if (connectCount < 1
	   || dataHandlerType < DataHandlerType::ThreadHandle
	   || dataHandlerType > DataHandlerType::DataTransmit) return InvalidParam;
	
	if (!m_tcpListener.isNormal()) return ListenConnectException;

	// epoll 模型监听连接
    int rc = m_epoll.create(connectCount);
    if (rc != Success) return rc;
	
	// 启动数据读写处理线程
	CDataHandler dataHandler(this, m_logicHandler);
	CDataTransmit dataTransmit(this, m_logicHandler);
	if (dataHandlerType == DataHandlerType::ThreadHandle)
	{
		rc = dataHandler.startHandle();
		if (rc != Success)
		{
			ReleaseErrorLog("start data handler thread error = %d\n", rc);
			return rc;
		}
	}
	else if (dataHandlerType == DataHandlerType::NetDataParseActive)
	{
		m_logicHandler->setConnMgrInstance(&dataHandler);
	}
	else if (dataHandlerType == DataHandlerType::DataTransmit)
	{
		m_logicHandler->setConnMgrInstance(&dataTransmit);
	}
	
	
	// 注册监听连接事件到epoll模型
	Connect listenConn;
	memset(&listenConn, 0, sizeof(Connect));
	listenConn.connStatus = normal;
	listenConn.fd = m_tcpListener.getFd();
	struct epoll_event optEv;
	optEv.data.ptr = &listenConn;
    optEv.events = EPOLLIN | EPOLLET;  // 监听读事件
	
	rc = m_epoll.addListener(listenConn.fd, optEv);
    if (rc != Success)
    {
		m_epoll.destroy();
        return rc;
    }
	
	// 忽略管道SIGPIPE信号
	CProcess::installSignal(SIGPIPE, NConnect::SignalHandler);
	
	ReleaseInfoLog("Tcp epoll server [ip = %s, port = %d, pid = %d] running ...\n", m_tcpListener.getIp(), m_tcpListener.getPort(), getpid());
	
	m_lastCheckTime = time(NULL);                // 检测连接的时间点
	const int maxEvents = 8192;                  // 最大监听事件个数
    struct epoll_event waitEvents[maxEvents];    // 监听epoll事件数组
	int fdCount = -1;
	Connect* conn = NULL;
	m_status = 1;  // 连接服务运行状态
	while(m_status == 1)
	{
		// 等待epoll事件的发生，一次无法输出全部事件，下次会继续输出，因此事件不会丢失
        fdCount = m_epoll.listen(waitEvents, maxEvents, waitTimeout);

		if (m_status == 0) break;  // 连接服务退出
		
		for (int i = 0; i < fdCount; ++i)
		{
			conn = (Connect*)(waitEvents[i].data.ptr);			
			if (conn->fd == listenConn.fd)
			{
				acceptConnect(waitEvents[i].events, conn);        // 建立新连接
			}
			else if (conn->id != 0)
			{
				handleConnect(waitEvents[i].events, conn);        // 从已建立的连接读写业务逻辑数据
			}
			else
			{
				onActiveConnect(waitEvents[i].events, conn);      // 主动建立的连接需要初始化才能读写业务逻辑数据
			}
		}
		
		doActiveConnect();  // 处理本地端主动发起的连接

		handleConnect();    // 处理所有连接（心跳检测，活跃检测，删除异常连接等等）
	}
	
	m_epoll.removeListener(listenConn.fd);  // 删除监听连接
	m_epoll.destroy();  // 关闭epoll模型
	
	if (dataHandlerType == DataHandlerType::ThreadHandle)
	{
		// 等待数据处理线程销毁资源，优雅的退出
		dataHandler.finishHandle();
		while (dataHandler.isRunning()) usleep(1000);
	}
	
	clear();  // 最后清理资源
	
	ReleaseInfoLog("Tcp epoll server exit, pid = %d, dataHandlerType = %d\n", getpid(), dataHandlerType);
	return Success;
}

// 停止连接管理服务
void CConnectManager::stop()
{
	m_status = 0;
}


bool CConnectManager::isRunning()
{
	return (m_status == 1);
}

// 服务退出时清理所有资源
void CConnectManager::clear()
{
	m_listenNum = 0;
	m_hbInterval = 0;         // 心跳检测间隔时间，单位秒
	m_hbFailedTimes = 0;      // 心跳检测连续失败hbFailedTimes次后关闭连接
	m_activeInterval = 0;     // 活跃检测间隔时间，单位秒
	m_status = 0;
	m_synNotify.status = parallel;
	
	m_tcpListener.destroy();  // 关闭监听连接
	clearAllConnect();        // 清理关闭所有连接
	m_epoll.destroy();        // 关闭epoll模型
	
	DELETE(m_memForRead);     // 删除读写缓冲区内存池
	DELETE(m_memForWrite);    // 删除读写缓冲区内存池
	DELETE(m_connMemory);     // 删除连接数据对象使用的内存池
	m_connId = 0;
	
	m_logicHandler = NULL;
	m_errorConns.clear();
}

// 关闭清理所有连接
void CConnectManager::clearAllConnect()
{
	Connect* pDestroy = NULL;
	Connect* pCurrent = m_loginConnectList;
	while (pCurrent != NULL)
	{
		pDestroy = pCurrent;
		pCurrent = pCurrent->pNext;
		destroyConnect(pDestroy);
	}
	
	for (IdToConnect::iterator it = m_connectMap.begin(); it != m_connectMap.end(); ++it)
	{
		destroyConnect(it->second);
	}
	
	m_connectMap.clear();	
	m_loginConnectList = NULL;
	m_msgConnectList = NULL;
	m_lastCheckTime = 0;
}

// 重新建立监听
void CConnectManager::reListen(Connect* listenConn)
{
	m_epoll.removeListener(listenConn->fd);  // 删除之前异常的监听连接
	m_tcpListener.destroy();  // 关闭监听连接
	
	int rc = m_tcpListener.create(m_listenNum);  // 重新启动监听连接
	if (rc == Success)
	{
		struct epoll_event optEv;
	    optEv.data.ptr = listenConn;
        optEv.events = EPOLLIN | EPOLLET;  // 监听读事件
		listenConn->connStatus = normal;
		listenConn->fd = m_tcpListener.getFd();
		if (m_epoll.addListener(listenConn->fd, optEv) != Success)
		{
			m_tcpListener.destroy();  // 关闭监听连接
			ReleaseErrorLog("reCreate tcp listener failed, add to epoll error");
		}
		else
		{
			ReleaseInfoLog("reCreate tcp listener success");
		}
	}
	else
	{
		ReleaseErrorLog("reCreate tcp listener error = %d, info = %s", errno, strerror(errno));
		m_tcpListener.destroy();  // 关闭监听连接
	}
}

// 建立新连接
void CConnectManager::acceptConnect(uint32_t eventVal, Connect* listenConn)
{
	if (eventVal & (EPOLLERR | EPOLLHUP)) return reListen(listenConn);  // 连接异常，需要关闭并重启监听连接
	
	if (eventVal & EPOLLIN)           // 存在新连接到来
	{
		Connect* newConn = NULL;
		int fd = -1;
		struct in_addr peerIp;
		unsigned short peerPort = 0;
		int errorCode = Success;
		while (isRunning())
		{
			if (m_tcpListener.accept(fd, peerIp, peerPort, errorCode) != Success)
			{
				if (errorCode == EINTR)  continue; // 被信号中断则继续
				break;  // 其他情况则退出
			}

            if (m_tcpListener.setNoBlock(fd) != Success) break;  // 设置非阻塞
			
			newConn = createConnect(fd, peerIp, peerPort);  // 创建被动连接信息数据
			if (newConn == NULL)
			{
				if (::close(fd) != 0) ReleaseWarnLog("close new accept connect fd = %d, error = %d, info = %s", fd, errno, strerror(errno));  // 失败则关闭连接
				ReleaseErrorLog("create new accept connect failed, fd = %d, ip = %s, port = %d", fd, CSocket::toIPStr(peerIp), peerPort);
				break;
			}

			// 回调通知逻辑层接收创建新连接成功
			addInitedConnect(newConn);
	        const ReturnValue retVal = m_logicHandler->onCreateConnect(newConn, CSocket::toIPStr(newConn->peerIp), newConn->peerPort, newConn->userCb);
			if (retVal == ReturnValue::CloseConnect) newConn->logicStatus = ConnectStatus::deleted;                    // 逻辑层要求直接关闭连接
			else if (retVal == ReturnValue::SendDataCloseConnect) newConn->logicStatus = ConnectStatus::sendClose;     // 逻辑层要求发送完数据之后关闭连接
			if (retVal != ReturnValue::OptSuccess) ReleaseWarnLog("create passive connect, logic return result = %d, ip = %s, port = %d, logicStatus = %d",
			retVal, CSocket::toIPStr(newConn->peerIp), newConn->peerPort, newConn->logicStatus);
		}
		
		if (errorCode != Success && errorCode != EAGAIN && errorCode != EINTR) reListen(listenConn);  // 监听连接异常了
	}
}

// 处理本地端主动发起的连接
void CConnectManager::doActiveConnect()
{
	ActiveConnect* activeConns = m_logicHandler->getConnectData();
    if (activeConns == NULL) return;

	Connect* newConn = NULL;
	struct in_addr peerAddrIp;
	int fd = -1;
	int errorCode = Success;
	ReturnValue activeConnState = ActiveConnError;
	while (activeConns != NULL)
	{
		activeConnState = ActiveConnError;
		
		if (activeConns->peerPort < MinConnectPort
		    || CSocket::toIPInAddr(activeConns->peerIp, peerAddrIp) != Success) activeConnState = ActiveConnIpPortErr;  // 无效IP或者端口号

		if (activeConnState == ActiveConnError && checkActiveConnect(peerAddrIp, activeConns->peerPort, activeConns->userId))
		{
			activeConns = activeConns->next;
			continue;  // 连接已经在建立中了。。。
		}
		
		if (checkErrorActiveConnect(peerAddrIp.s_addr, activeConns->peerPort))
		{
			activeConns = activeConns->next;
			continue;  // 上次错误的连接，时间间隔还没到
		}

        fd = -1;
		errorCode = Success;
		if (activeConnState == ActiveConnError && doActiveConnect(activeConns->peerIp, activeConns->peerPort, fd, errorCode) == Success)  // 建立主动连接
		{
			newConn = createConnect(fd, peerAddrIp, activeConns->peerPort);  // 创建主动连接信息数据
			if (newConn == NULL)
			{
				if (::close(fd) != 0) ReleaseWarnLog("close new active connect fd = %d, error = %d, info = %s", fd, errno, strerror(errno));  // 失败则关闭连接
				ReleaseErrorLog("create new active connect failed, fd = %d, ip = %s, port = %d", fd, activeConns->peerIp, activeConns->peerPort);
				break;
			}

            newConn->userCb = activeConns->userCb;  // 挂接保存用户数据
			newConn->userId = activeConns->userId;
			activeConnState = OptSuccess;
			if (errorCode != Success)
			{
				// 连接建立中，得等待连接建立完毕成功。。。
				newConn->heartBeatSecs = time(NULL) + activeConns->timeOut;  // 建立连接超时时间点
				activeConnState = ActiveConnecting;
				
				// 正在建立中的连接加入login队列
				if (m_loginConnectList != NULL)
				{
					newConn->pNext = m_loginConnectList;
					m_loginConnectList->pPre = newConn;
					m_loginConnectList = newConn;
					m_loginConnectList->pPre = NULL;
				}
				else
				{
					m_loginConnectList = newConn;
				}
			}
		}
		
		// 回调通知逻辑层主动连接建立的状态
		if (activeConnState == OptSuccess)
		{
			addInitedConnect(newConn);
			const ReturnValue retVal = m_logicHandler->doCreateConnect(newConn, activeConns->peerIp, activeConns->peerPort, activeConnState, newConn->userCb);  // 直接成功了
			if (retVal == ReturnValue::CloseConnect) newConn->logicStatus = ConnectStatus::deleted;                    // 逻辑层要求直接关闭连接
			else if (retVal == ReturnValue::SendDataCloseConnect) newConn->logicStatus = ConnectStatus::sendClose;     // 逻辑层要求发送完数据之后关闭连接
			if (retVal != ReturnValue::OptSuccess) ReleaseWarnLog("create active connect direct successful, logic return result = %d, ip = %s, port = %d, logicStatus = %d",
			retVal, activeConns->peerIp, activeConns->peerPort, newConn->logicStatus);
		}
		else if (activeConnState != ActiveConnecting)
		{
			ReleaseErrorLog("create active connect failed, ip = %s, port = %d", activeConns->peerIp, activeConns->peerPort);

		    addErrorActiveConnect(peerAddrIp.s_addr, activeConns->peerPort);
			m_logicHandler->doCreateConnect(NULL, activeConns->peerIp, activeConns->peerPort, activeConnState, activeConns->userCb);  // 连接建立直接失败了
		}

		activeConns = activeConns->next;
	}
}

// 检查是否对应的主动连接已经在建立中了
bool CConnectManager::checkActiveConnect(const struct in_addr& peerIp, const unsigned short peerPort, const int userId)
{
	Connect* conn = m_loginConnectList;
	while (conn != NULL)
	{
		if (peerIp.s_addr == conn->peerIp.s_addr && peerPort == conn->peerPort && userId == conn->userId) return true;
		conn = conn->pNext;
	}
	return false;
}

void CConnectManager::addErrorActiveConnect(unsigned int ip, unsigned short port)
{
	NodeID nodeIdVal;
	nodeIdVal.ipPort.ip = ip;
	nodeIdVal.ipPort.port = port;
	NodeIdToTimeSecs::iterator it = m_errorConns.find(nodeIdVal.id);
    if (it != m_errorConns.end())
	{
		it->second = time(NULL) + MaxErrorActiveConnectIntervalSecs;  // 下次可以执行建立连接的时间点
	}
	else
	{
		m_errorConns[nodeIdVal.id] = time(NULL) + MinErrorActiveConnectIntervalSecs;  // 下次可以执行建立连接的时间点
	}
}

void CConnectManager::removeErrorActiveConnect(unsigned int ip, unsigned short port)
{
	if (m_errorConns.size() > 0)
	{
		NodeID nodeIdVal;
		nodeIdVal.ipPort.ip = ip;
		nodeIdVal.ipPort.port = port;
		m_errorConns.erase(nodeIdVal.id);
	}
}

bool CConnectManager::checkErrorActiveConnect(unsigned int ip, unsigned short port)
{
	if (m_errorConns.size() > 0)
	{
		NodeID nodeIdVal;
		nodeIdVal.ipPort.ip = ip;
		nodeIdVal.ipPort.port = port;
		NodeIdToTimeSecs::iterator it = m_errorConns.find(nodeIdVal.id);
		return (it != m_errorConns.end() && it->second > time(NULL));
	}
	return false;
}

// 建立本地端发起的主动连接
int CConnectManager::doActiveConnect(const char* peerIp, const unsigned short peerPort, int& fd, int& errorCode)
{
	int rc = m_activeConnect.init(peerIp, peerPort);	
	do
	{
		if (rc != Success) break;
		
		rc = m_activeConnect.open(SOCK_STREAM);   // tcp socket
		if (rc != Success) break;
		
		rc = m_activeConnect.setReuseAddr(1);
		if (rc != Success) break;
		
		rc = m_activeConnect.setNagle(1);
		if (rc != Success) break;
		
		rc = m_activeConnect.setNoBlock();        // 设置为非阻塞
		if (rc != Success) break;
		
		rc = m_activeConnect.connect(errorCode);  // 非阻塞式，建立连接
		if (rc != Success) break;
		
		fd = m_activeConnect.getFd();             // 返回tcp socket文件描述符
	}
	while (0);
	
	if (rc != Success)
	{
		ReleaseErrorLog("do active connect failed, ip = %s, port = %d", peerIp, peerPort);
		m_activeConnect.close();
	}
	m_activeConnect.unInit();
	
	return rc;
}

void CConnectManager::onActiveConnect(uint32_t eventVal, Connect* conn)
{
    // 检查本地端发起的主动连接
	int err = 0;
	socklen_t len = sizeof(err); 
	int rc = CSocket::getOpt(conn->fd, SOL_SOCKET, SO_ERROR, (void*)&err, &len);
	
	// 连接异常则直接关闭删除
	if ((eventVal & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
		|| (rc != Success) || (err != EINPROGRESS && err != 0))
	{
		ReleaseErrorLog("create active connect error, ip = %s, port = %d, error = %d, info = %s",
		CSocket::toIPStr(conn->peerIp), conn->peerPort, err, strerror(err));
	
		addErrorActiveConnect(conn->peerIp.s_addr, conn->peerPort);
		m_logicHandler->doCreateConnect(NULL, CSocket::toIPStr(conn->peerIp), conn->peerPort, ActiveConnError, conn->userCb);
	    return destroyLoginConnect(conn);  // 错误则从login队列删除
	}

	if (err == EINPROGRESS)
	{
		if (time(NULL) < conn->heartBeatSecs) return;  // 没超时则继续等待
		
		addErrorActiveConnect(conn->peerIp.s_addr, conn->peerPort);
		m_logicHandler->doCreateConnect(NULL, CSocket::toIPStr(conn->peerIp), conn->peerPort, ActiveConnTimeOut, conn->userCb);
		return destroyLoginConnect(conn);  // 超时了则从login队列删除
	}

	// 连接建立成功了
	removeConnect(m_loginConnectList, conn);   // 先从队列移除
	removeErrorActiveConnect(conn->peerIp.s_addr, conn->peerPort);
	addInitedConnect(conn);
	
	const ReturnValue retVal = m_logicHandler->doCreateConnect(conn, CSocket::toIPStr(conn->peerIp), conn->peerPort, OptSuccess, conn->userCb);
	if (retVal == ReturnValue::CloseConnect) conn->logicStatus = ConnectStatus::deleted;                    // 逻辑层要求直接关闭连接
	else if (retVal == ReturnValue::SendDataCloseConnect) conn->logicStatus = ConnectStatus::sendClose;     // 逻辑层要求发送完数据之后关闭连接
	if (retVal != ReturnValue::OptSuccess) ReleaseWarnLog("create active connect successful, logic return result = %d, ip = %s, port = %d, logicStatus = %d",
	retVal, CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->logicStatus);
	
	if (eventVal & EPOLLOUT) conn->canWrite = 1;  // 可写数据

	if (eventVal & EPOLLIN) readFromConnect(conn); // 有数据可读
}

void CConnectManager::addInitedConnect(Connect* conn)
{
	// 初始化连接数据
    conn->id = ++m_connId;                    // 连接id递增
	conn->activeSecs = time(NULL);            // 当前活跃时间点
	conn->heartBeatSecs = conn->activeSecs;   // 当前心跳时间点
	conn->connStatus = normal;
	conn->logicStatus = normal;	
	conn->hbResult = 0;
	
	m_connectMap[conn->id] = conn;
	
	ReleaseInfoLog("init message connect, ip = %s, port = %d, id = %lld\n", CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id);
}

void CConnectManager::addToMsgQueue(Connect* conn)
{
	if (conn->readSocketTimes > 0)
	{
		conn->readSocketTimes = 1;  // 已经在队列中了，则重新开始计算即可
		return;
	}
	
	// 加入消息队列
	conn->readSocketTimes = 1;
	if (m_msgConnectList != NULL)
	{
		conn->pNext = m_msgConnectList;
		m_msgConnectList->pPre = conn;
		m_msgConnectList = conn;
		m_msgConnectList->pPre = NULL;
	}
	else
	{
		conn->pNext = NULL;
		conn->pPre = NULL;
		m_msgConnectList = conn;
	}
}

void CConnectManager::removeFromMsgQueue(Connect* conn)
{
	if (conn->readSocketTimes > 0)
	{
		conn->readSocketTimes = 0;
		removeConnect(m_msgConnectList, conn);  // 从队列删除
	}
}

// 申请内存资源创建连接
Connect* CConnectManager::createConnect(const int fd, const struct in_addr& peerIp, const unsigned short peerPort)
{
	// 连接管理线程只创建读socket缓冲区，写socket缓冲区由数据处理线程创建
	CSingleW_SingleR_Buffer* readBuff = getReadBuffer();
	if (readBuff == NULL)
	{
		ReleaseErrorLog("get memory for read buffer of new connect failed");
		return NULL;
	}
	readBuff->next = readBuff;    // 整成环型链表读写循环
	
	Connect* newConn = (Connect*)m_connMemory->get();
	if (newConn == NULL)
	{
		releaseReadBuffer(readBuff);
		ReleaseErrorLog("get memory for create connect failed");
		return NULL;
	}

    // 初始化连接数据
	memset(newConn, 0, sizeof(Connect));
	newConn->fd = fd;
	newConn->peerIp = peerIp;
	newConn->peerPort = peerPort;
	newConn->readBuff = readBuff;
	newConn->curReadIdx = readBuff;
	
	// 新连接加入epoll监听模型
	struct epoll_event optEv;
	optEv.data.ptr = newConn;
	optEv.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;  // 同时监听读写事件，边缘触发模式
	int rc = m_epoll.addListener(newConn->fd, optEv);
    if (rc != Success)
	{
		releaseReadBuffer(readBuff);
		m_connMemory->put((char*)newConn);
		return NULL;
	}
	
	ReleaseInfoLog("create connect, ip = %s, port = %d, fd = %d", CSocket::toIPStr(newConn->peerIp), newConn->peerPort, newConn->fd);

	return newConn;
}

// 处理监听的连接（读写数据）
void CConnectManager::handleConnect(uint32_t eventVal, Connect* conn)
{
	if (eventVal & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))   // 连接异常，需要关闭此类连接
	{
		ReleaseWarnLog("ready close error connect, peer ip = %s, port = %d, id = %lld", CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id);
		
		if (eventVal & EPOLLIN) readFromConnect(conn);   // 存在连接有数据可读的情况则先读最后的数据
		conn->connStatus = closed;  // 关闭该连接
	}
	else
	{
		if (eventVal & EPOLLIN)  readFromConnect(conn);  // 有数据可读
		if (eventVal & EPOLLOUT) writeToConnect(conn);   // 可写数据
	}
	
	if (conn->connStatus != normal || conn->logicStatus != normal) closeConnect(conn);  // 关闭异常的连接
}

// 处理所有连接（心跳检测，活跃检测，删除异常连接等等）
void CConnectManager::handleConnect()
{
	unsigned int curSecs = time(NULL);       // 系统当前时间
	if (curSecs == m_lastCheckTime) return;  // 默认每秒检测一次
	
	// 检测所有连接	
	m_lastCheckTime = curSecs;
	bool isEraseConnect = false;
	Connect* msgConn = NULL;
	for (IdToConnect::iterator it = m_connectMap.begin(); it != m_connectMap.end();)
	{
		isEraseConnect = false;
		msgConn = it->second;
		
		// 关闭异常的连接
		if (msgConn->logicStatus != normal)
		{
			closeConnect(msgConn);
			if (msgConn->logicStatus == deleted && msgConn->writeBuff == NULL)
			{
				ReleaseWarnLog("destory message connect, peer ip = %s, port = %d, id = %lld, connStatus = %d, logicStatus = %d\n",
				               CSocket::toIPStr(msgConn->peerIp), msgConn->peerPort, msgConn->id, msgConn->connStatus, msgConn->logicStatus);
					   
				// 等待数据处理线程处理完
				// 连接线程处理完毕会自动通知数据处理线程执行处理操作
				SynWaitNotify sysWaitNotifyHepler(this);
				isEraseConnect = true;
				m_connectMap.erase(it++);
		        destroyMsgConnect(msgConn);  // 删除异常的连接	
			}
		}
		
		else if (msgConn->connStatus == normal)
		{
			if (CDataHandler::getCanWriteDataSize(msgConn) > 0 && msgConn->canWrite) writeToConnect(msgConn);  // 存在数据可写往socket
			
			// 心跳检测，活跃检测，检测失败或者超时则关闭连接
			if ((msgConn->hbResult >= m_hbFailedTimes) || ((int)(curSecs - msgConn->activeSecs) > m_activeInterval))
			{
				unsigned short hbResult = msgConn->hbResult;
				unsigned short cfgHbTimes = m_hbFailedTimes;
				ReleaseWarnLog("close not active connect, ip = %s, port = %d, logicId = %lld, hbFailedTimes = %d, config hb times = %d, no active time = %d, config active interval = %d\n",
				CSocket::toIPStr(msgConn->peerIp), msgConn->peerPort, msgConn->id, hbResult, cfgHbTimes, curSecs - msgConn->activeSecs, m_activeInterval);
				closeConnect(msgConn);
			}
			
			// 检查最大socket无数据的次数，超过此最大次数则连接移出消息队列，避免遍历一堆无数据的空连接
			else if (msgConn->readSocketTimes > MaxCheckTimesOfNoSocketData)
			{
				SynWaitNotify sysWaitNotifyHepler(this);
				removeFromMsgQueue(msgConn);  // 从消息队列删除
			}
			else if (msgConn->readSocketTimes > 0 && CDataHandler::getCanReadDataSize(msgConn) == 0)
			{
				++msgConn->readSocketTimes;
			}
		}
		
		if (!isEraseConnect) ++it;
	}
}

void CConnectManager::closeConnect(Connect* conn)
{
	if (conn->fd > 0)
	{
		// 发送完数据再关闭连接
		if (conn->logicStatus == sendClose && conn->connStatus == normal && conn->canWrite) writeToConnect(conn);
		
		m_epoll.removeListener(conn->fd);  // 从epoll模型删除监听事件

        ReleaseWarnLog("close connect, peer ip = %s, port = %d, id = %lld, connStatus = %d, logicStatus = %d",
		CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id, conn->connStatus, conn->logicStatus);
		
        // 关闭连接
		if (::close(conn->fd) != 0) ReleaseWarnLog("close connect fd = %d, error = %d, info = %s", conn->fd, errno, strerror(errno));
		
		conn->fd = -1;
		conn->canWrite = 0;
		conn->connStatus = closed;
		
		// 成功建立了的连接才会回调关闭
		if (conn->id != 0) m_logicHandler->onClosedConnect(conn, conn->userCb);  // 通知逻辑层connLogicId对应的连接已被关闭
		
		if (conn->writeBuff != NULL) addToMsgQueue(conn);  // 有资源没释放要加回消息队列等待数据处理线程释放资源，之后才能销毁连接
		else if (conn->readSocketTimes == 0) conn->logicStatus = deleted;
	}
}

void CConnectManager::removeConnect(Connect*& listHeader, Connect* conn)
{
	if (conn != listHeader)        // 非头结点
	{
		conn->pPre->pNext = conn->pNext;
		if (conn->pNext != NULL)           // 非尾结点
		{
			conn->pNext->pPre = conn->pPre;
		}
	}
	else
	{
		listHeader = conn->pNext;  // 头结点
		if (listHeader != NULL)    // 非尾结点
		{
			listHeader->pPre = NULL;
		}
	}
	
	conn->pNext = NULL;
	conn->pPre = NULL;
}

void CConnectManager::destroyConnect(Connect* conn)
{
	// 1)关闭连接&删除监听事件
	closeConnect(conn);
		
    // 2)回收内存块，把内存块放回内存池，回收读socket缓冲区内存块，写socket内存块由数据处理线程回收管理
	CSingleW_SingleR_Buffer* first = conn->readBuff;
	if (first != NULL)
	{
		CSingleW_SingleR_Buffer* current = first->next;
		CSingleW_SingleR_Buffer* release = NULL;
		while (current != first)  // 环型链表
		{
			release = current;
			current = current->next;
			releaseReadBuffer(release);  // 释放读写缓冲区buff块
		}
		releaseReadBuffer(first);  // 释放读写缓冲区buff块
		conn->readBuff = NULL;
		conn->curReadIdx = NULL;
	}

	// 3)最后释放连接数据对象内存块
	m_connMemory->put((char*)conn);
}

void CConnectManager::destroyMsgConnect(Connect* conn)
{
	removeFromMsgQueue(conn);  // 从消息队列删除
	destroyConnect(conn);
}

void CConnectManager::destroyLoginConnect(Connect* conn)
{
	removeConnect(m_loginConnectList, conn);  // 从队列删除
	destroyConnect(conn);
}

bool CConnectManager::readFromConnect(Connect* conn)
{
	if (!isRunning()) return false;
	
	// 写数据时必须先写第一块数据区，写完了才能继续写第二块数据区，否则会直接导致错误
	char* firstBuff = NULL;
	unsigned int firstLen = 0;
	char* secondBuff = NULL;
	unsigned int secondLen = 0;
	CSingleW_SingleR_Buffer* readBuff = conn->readBuff;
	if (!readBuff->beginWriteBuff(firstBuff, firstLen, secondBuff, secondLen))  // 获取buff失败则队列已经满了
	{
		// 下一块buff必须为空才能往里写数据，否则会导致读线程读的数据不连续而出现错误
		CSingleW_SingleR_Buffer* next = readBuff->next;
		if (next != readBuff && next->isEmpty())
		{
			char* flag = (char*)next;			
			if (*(--flag) == 0)  // 非当前正在读的buff才能执行写操作
			{
                conn->readBuff = next;  // 条件满足则切换到下一块内存执行写操作
				readBuff = conn->readBuff;
				readBuff->beginWriteBuff(firstBuff, firstLen, secondBuff, secondLen);   
			}
		}

        if (firstLen == 0)
		{
			// 创建新内存块并挂接到链表上，循环链表向同一方向挂接，一写一读线程下不会有问题
			CSingleW_SingleR_Buffer* newBuff = getReadBuffer();
			if (newBuff == NULL)
			{
				// 没有内存了，理论上不会出现。。。不可思议
				ReleaseErrorLog("no memory for write connect data to logic, id = %I64d, fd = %d", conn->id, conn->fd);
				return false;
			}
			
			newBuff->next = next;
			readBuff->next = newBuff;
			conn->readBuff = newBuff;  // 切换到新内存块
			readBuff = conn->readBuff;
			readBuff->beginWriteBuff(firstBuff, firstLen, secondBuff, secondLen);
        }
	}
	
	int nRead = ::read(conn->fd, firstBuff, firstLen);  // 读数据到第一块内存
	if (nRead > 0)
	{
		addToMsgQueue(conn);        // 有数据则加入消息处理队列
		setConnectNormal(conn);     // 读数据成功则连接一般情况下都正常
		
		readBuff->endWriteBuff(firstBuff, nRead, NULL, 0);  // 成功则提交数据
		if ((unsigned int)nRead < firstLen) return true;  // 数据已经读完
		
		if (secondBuff != NULL)  // 存在第二块内存则继续写入数据
		{
			nRead = ::read(conn->fd, secondBuff, secondLen);  // 继续读数据到第二块内存
			if (nRead > 0)
			{
				readBuff->endWriteBuff(NULL, 0, secondBuff, nRead);  // 成功则提交数据
				if ((unsigned int)nRead < secondLen)  return true;  // 数据已经读完
				
				return readFromConnect(conn);  // 数据可能没读完但内存块用完了，则继续读数据
			}
		}
		else
		{
		    return readFromConnect(conn);  // 数据可能没读完但内存块用完了，则继续读数据
		}
	}
	
	// nRead < 0 的情况
	if (errno == EINTR)
	{
		errno = 0;
		return readFromConnect(conn);  // 被信号中断则继续读数据
	}
	
	if (errno == EAGAIN) return true;  // 没有数据可读了
	
	ReleaseWarnLog("read from connect, peer ip = %s, port = %d, id = %lld, nRead = %d, errno = %d, info = %s",
	CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id, nRead, errno, strerror(errno));
		
	// 其他 nRead < 0 的情况，连接异常则关闭连接
	// nRead == 0 的情况对端关闭连接则本端也关闭连接
	conn->connStatus = closed;  // 通知调用者关闭此连接
	return false;
}

bool CConnectManager::writeToConnect(Connect* conn)
{
	if (!isRunning()) return false;
	
	// 读数据时必须先读第一块数据区，读完了才能继续读第二块数据区，否则会直接导致错误
	conn->canWrite = 1;  // 标志连接可写
	CSingleW_SingleR_Buffer* curWriteIdx = conn->curWriteIdx;
	if (curWriteIdx == NULL) return true;  // 目前无数据可写
	
	char* firstBuff = NULL;
	unsigned int firstLen = 0;
	char* secondBuff = NULL;
	unsigned int secondLen = 0;
	if (!curWriteIdx->beginReadBuff(firstBuff, firstLen, secondBuff, secondLen))  // 获取buff失败则队列已经空了
	{
		// 存在当前buff为空，则可能当前buff正好是一个已经读出来的完整消息，因此需要继续再尝试下一块buff
		CSingleW_SingleR_Buffer* next = curWriteIdx->next;
		if (next != curWriteIdx && !next->isEmpty())
		{
			char* flag = (char*)next;
			*(--flag) = 1;  // 设置读标志位
			flag = (char*)curWriteIdx;
			*(--flag) = 0;  // 清除读标志位

            conn->curWriteIdx = next;  // 条件满足则切换到下一块内存
			curWriteIdx = conn->curWriteIdx;
			curWriteIdx->beginReadBuff(firstBuff, firstLen, secondBuff, secondLen);
		}
	}
	
	if (firstLen == 0) return true;  // 目前无数据可写
	
	int nWrite = ::write(conn->fd, firstBuff, firstLen);
	if (nWrite > 0)
	{
		setConnectNormal(conn);  // 写数据成功则连接一般情况下都正常
		
		curWriteIdx->endReadBuff(firstBuff, nWrite, NULL, 0);  // 成功则提交数据
		if ((unsigned int)nWrite < firstLen)
		{
			conn->canWrite = 0;  // 标志连接不可写
			return true;
		}
		
		if (secondBuff != NULL)  // 存在第二块内存则继续读写数据
		{
			nWrite = ::write(conn->fd, secondBuff, secondLen);  // 继续写数据到socket
			if (nWrite > 0)
			{
				curWriteIdx->endReadBuff(NULL, 0, secondBuff, nWrite);  // 成功则提交数据
				if ((unsigned int)nWrite < secondLen)
				{
					conn->canWrite = 0;  // 标志连接不可写
					return true;
				}
				
				return writeToConnect(conn);  // 继续读写数据
			}
		}
		else
		{
			return writeToConnect(conn);  // 继续读写数据
		}
	}
	
	if (nWrite < 0)
	{
		// nWrite < 0 的情况
		if (errno == EINTR) return writeToConnect(conn);  // 被信号中断则继续读写数据
		
		conn->canWrite = 0;   // 标志连接不可写
		if (errno == EAGAIN) return true;  // 没有socket缓冲区可写了
		
		ReleaseWarnLog("write to connect, peer ip = %s, port = %d, id = %lld, nWrite = %d, errno = %d, info = %s",
	    CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id, nWrite, errno, strerror(errno));
		
		// 其他 nWrite < 0 的情况，连接异常则关闭连接
		conn->connStatus = closed;  // 通知调用者关闭此连接
		return false;
	}
	
	return true;
}

void CConnectManager::setConnectNormal(Connect* conn)
{
	conn->activeSecs = time(NULL);
	conn->heartBeatSecs = conn->activeSecs;
	conn->hbResult = 0;
}

CSingleW_SingleR_Buffer* CConnectManager::getBuffer(CMemManager* memMgr)
{
	CSingleW_SingleR_Buffer* buffContainer = NULL;
	char* buff = memMgr->get();
	if (buff != NULL)
	{
		const unsigned int bufferHeaderSize = sizeof(CSingleW_SingleR_Buffer);
		memset(buff, 0, bufferHeaderSize + 1);
		
		*buff = 1;  // 读线程占用标志位，此时写线程不能对该buff执行写操作
	    ++buff;     // 第一个字节存储标志位
	
		buffContainer = (CSingleW_SingleR_Buffer*)buff;
		buffContainer->buff = buff + bufferHeaderSize;               // 数据区开始地址
		buffContainer->buffSize = memMgr->getBuffSize() - bufferHeaderSize - 1;    // 数据缓冲区长度，扣掉标志位1字节
	}
	return buffContainer;
}

void CConnectManager::releaseBuffer(CMemManager* memMgr, CSingleW_SingleR_Buffer*& pBuff)
{
	if (pBuff != NULL)
	{
		char* pBegin = (char*)pBuff;
		memMgr->put(--pBegin);
		pBuff = NULL;
	}
}

CSingleW_SingleR_Buffer* CConnectManager::getReadBuffer()
{
	return getBuffer(m_memForRead);
}

void CConnectManager::releaseReadBuffer(CSingleW_SingleR_Buffer*& pBuff)
{
	releaseBuffer(m_memForRead, pBuff);
}


CSingleW_SingleR_Buffer* CConnectManager::getWriteBuffer()
{
	return getBuffer(m_memForWrite);
}

void CConnectManager::releaseWriteBuffer(CSingleW_SingleR_Buffer*& pBuff)
{
	releaseBuffer(m_memForWrite, pBuff);
}

unsigned short CConnectManager::getHbInterval() const
{
	return m_hbInterval;
}

Connect* CConnectManager::getMsgConnectList()
{
	return m_msgConnectList;
}

Connect* CConnectManager::getMsgConnect(const uuid_type connId)
{
	IdToConnect::iterator it = m_connectMap.find(connId);
	return (it != m_connectMap.end()) ? it->second : NULL;
}

// 等待连接处理线程处理完
bool CConnectManager::waitConnecter()
{
	if (m_synNotify.status == waitDH)  // 连接线程正在等待中。。。
	{
		// 1) 先通知唤醒等待的连接线程
		m_synNotify.waitDataHandlerMutex.lock();      // 加锁，防止唤醒事件丢失
		m_synNotify.status = waitConn;                // 挂起自己，等待连接线程处理完毕
		m_synNotify.waitDataHandlerMutex.unLock();
		m_synNotify.waitDataHandlerCond.signal();     // 唤醒连接线程
		
		// 2) 接着挂起自己， 等待连接线程处理完毕
		m_synNotify.waitConnecterMutex.lock();
		while (m_synNotify.status == waitConn) m_synNotify.waitConnecterCond.wait(m_synNotify.waitConnecterMutex);
		m_synNotify.waitConnecterMutex.unLock();
		
		return true;
	}
	
	return false;
}

// 等待数据处理线程处理完
void CConnectManager::waitDataHandler()
{
	// 挂起连接线程，等待数据处理线程
	m_synNotify.waitDataHandlerMutex.lock();
	m_synNotify.status = waitDH;
	while (m_synNotify.status == waitDH) m_synNotify.waitDataHandlerCond.wait(m_synNotify.waitDataHandlerMutex);  // 等待数据处理线程，挂起自己
	m_synNotify.waitDataHandlerMutex.unLock();
}

// 通知数据处理线程执行处理操作
void CConnectManager::notifyDataHandler()
{
	m_synNotify.waitConnecterMutex.lock();
	m_synNotify.status = parallel;             // 恢复到双线程并行状态
	m_synNotify.waitConnecterMutex.unLock();
	m_synNotify.waitConnecterCond.signal();    // 唤醒数据处理线程
}
	
}

