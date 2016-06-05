
/* author : limingfan
 * date : 2014.12.17
 * description : 网络连接管理socket读写数据线程
 */
 
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "CDataHandler.h"
#include "CConnectManager.h"
#include "base/ErrorCode.h"


using namespace NCommon;
using namespace NErrorCode;

namespace NConnect
{

static const unsigned int netPkgHeaderLen = sizeof(NetPkgHeader);

enum DataHandlerStatus
{
	stopted = 0,
	running = 1,
	stopping = 2,
};

CDataHandler::CDataHandler(CConnectManager* connMgr, ILogicHandler* logicHandler) : m_connMgr(connMgr), m_logicHandler(logicHandler)
{
	m_curConn = NULL;
	m_status = stopted;
}

CDataHandler::~CDataHandler()
{
	m_curConn = NULL;
	m_connMgr = NULL;
	m_logicHandler = NULL;
	m_status = stopted;
}

int CDataHandler::startHandle()
{
	if (m_connMgr == NULL || m_logicHandler == NULL) return InvalidParam;
	
	m_status = running;
	int rc = start();
	if (rc != Success) m_status = stopted;
	
	return rc;
}

void CDataHandler::finishHandle()
{
	m_status = stopping;  // 等待数据处理线程销毁资源退出
}

bool CDataHandler::isRunning()
{
	return (m_status != stopted);
}


unsigned int CDataHandler::getCanReadDataSize(Connect* conn)
{
	CSingleW_SingleR_Buffer* curReadIdx = conn->curReadIdx;
	unsigned int size = curReadIdx->getDataSize();
	if (curReadIdx->next != curReadIdx) size += curReadIdx->next->getDataSize();  // 注意这里需要继续尝试下一块数据区
	return size;
}

unsigned int CDataHandler::getCanWriteDataSize(Connect* conn)
{
	CSingleW_SingleR_Buffer* curWriteIdx = conn->curWriteIdx;
	if (curWriteIdx == NULL) return 0;
	
	unsigned int size = curWriteIdx->getDataSize();
	if (curWriteIdx->next != curWriteIdx) size += curWriteIdx->next->getDataSize();  // 注意这里需要继续尝试下一块数据区
	return size;
}

void CDataHandler::releaseWrtBuff(CConnectManager* connMgr, Connect* conn)
{
	CSingleW_SingleR_Buffer* first = conn->writeBuff;
	if (first != NULL)
	{
		CSingleW_SingleR_Buffer* current = first->next;
		CSingleW_SingleR_Buffer* release = NULL;
		while (current != first)  // 环型链表
		{
			release = current;
			current = current->next;
			connMgr->releaseWriteBuffer(release);  // 释放读写缓冲区buff块
		}
		connMgr->releaseWriteBuffer(first);  // 释放读写缓冲区buff块
		conn->writeBuff = NULL;
		conn->curWriteIdx = NULL;
	}
}

bool CDataHandler::writeToSkt(CConnectManager* connMgr, Connect* conn, const char* data, const unsigned int len, int& wtLen)
{
	wtLen = 0;
	int nWrite = ::write(conn->fd, data, len);
	if (nWrite > 0)
	{
		connMgr->setConnectNormal(conn);  // 写数据成功则连接一般情况下都正常
		wtLen = nWrite;
	}
	
	if (nWrite < 0)
	{
		if (errno == EINTR) return writeToSkt(connMgr, conn, data, len, wtLen);  // 被信号中断则继续读写数据
		if (errno == EAGAIN) return true;  // 没有socket缓冲区可写了
		
		ReleaseWarnLog("direct write to socket buffer error, peer ip = %s, port = %d, id = %lld, nWrite = %d, errno = %d, info = %s",
	    CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id, nWrite, errno, strerror(errno));

		conn->connStatus = closed;  // 通知调用者关闭此连接
		return false;
	}
	
	return true;
}

// 把data数据写入网络连接
bool CDataHandler::write(CConnectManager* connMgr, Connect* conn, const char* data, int len)
{
	int wtLen = 0;
	if (getCanWriteDataSize(conn) == 0)
	{
		// 先直接写到socket缓冲区
		if (!writeToSkt(connMgr, conn, data, len, wtLen)) return false;
		if (len == wtLen) return true;
		
		// 剩余未写完的数据写往临时缓冲区
		data += wtLen;
		len -= wtLen;
	}
	
	CSingleW_SingleR_Buffer* writeBuff = conn->writeBuff;
	if (writeBuff == NULL)
	{
	    // 第一次创建新内存块并挂接到链表上
		writeBuff = connMgr->getWriteBuffer();
		if (writeBuff == NULL)
		{
			ReleaseErrorLog("first time get no memory for write logic data to connect, id = %lld, fd = %d", conn->id, conn->fd);
			return false;  // 好吧，内存没了这种不可思议的事情居然出现了，估计哪里有内存泄露了，太疯狂了。。。
		}
		
		writeBuff->next = writeBuff;    // 整成环型链表读写循环
		conn->writeBuff = writeBuff;
		conn->curWriteIdx = writeBuff;
	}
		
	wtLen = writeBuff->write(data, len);
	if (wtLen < len)
	{
		// 下一块buff必须为空才能往里写数据，否则会导致读线程读的数据不连续而出现错误
		CSingleW_SingleR_Buffer* next = writeBuff->next;
		if (next != writeBuff && next->isEmpty())
		{
			char* readFlag = (char*)next;			
			if (*(--readFlag) == 0)  // 非当前正在读的buff才能执行写操作
			{
                conn->writeBuff = next;  // 条件满足则切换到下一块内存
			    return write(connMgr, conn, data + wtLen, len - wtLen);    
			}
		}

		// 创建新内存块并挂接到链表上
		CSingleW_SingleR_Buffer* newBuff = connMgr->getWriteBuffer();
		if (newBuff == NULL)
		{
			ReleaseErrorLog("no memory for write logic new data to connect, id = %lld, fd = %d", conn->id, conn->fd);
			return false;  // 好吧，内存没了这种不可思议的事情居然出现了，估计哪里有内存泄露了，太疯狂了。。。
		}
			
		newBuff->next = next;
		writeBuff->next = newBuff;
		conn->writeBuff = newBuff;  // 切换到下一块内存
		return write(connMgr, conn, data + wtLen, len - wtLen);
	}
	
	return true;
}

// 把网络数据写到data空间
// isRead : 为false时从连接读出的数据会被直接丢弃，不会拷贝到data调用者空间
unsigned int CDataHandler::read(Connect* conn, char* data, unsigned int len, bool isRead)
{
	CSingleW_SingleR_Buffer* curReadIdx = conn->curReadIdx;
	unsigned int rdLen = curReadIdx->read(data, len, isRead);
	if (rdLen < len)  // 存在rdLen == 0，可能当前buff正好是一个完整的消息，因此需要继续再赏试下一块buff
	{
		CSingleW_SingleR_Buffer* next = curReadIdx->next;
		if (next != curReadIdx && !next->isEmpty())
		{
			char* readFlag = (char*)next;
			*(--readFlag) = 1;  // 设置读标志位
			readFlag = (char*)curReadIdx;
			*(--readFlag) = 0;  // 清除读标志位

			conn->curReadIdx = next;  // 条件满足则切换到下一块内存
			return rdLen + read(conn, data + rdLen, len - rdLen, isRead);
		}
	}
	
	return rdLen;
}



void CDataHandler::releaseWriteBuffer(Connect* conn)
{
	CDataHandler::releaseWrtBuff(m_connMgr, conn);
}

// 把data数据写入网络连接
bool CDataHandler::writeData(Connect* conn, const char* data, int len)
{
	return CDataHandler::write(m_connMgr, conn, data, len);
}

// 把网络数据写到data空间
// isRead : 为false时从连接读出的数据会被直接丢弃，不会拷贝到data调用者空间
unsigned int CDataHandler::readData(Connect* conn, char* data, unsigned int len, bool isRead)
{
	return CDataHandler::read(conn, data, len, isRead);
}

void CDataHandler::writeHbRequestPkg(Connect* curConn, unsigned int curSecs)
{
	if (curConn->connStatus == normal && curConn->logicStatus == normal && (int)(curSecs - curConn->heartBeatSecs) >= m_connMgr->getHbInterval())
	{
		curConn->heartBeatSecs = time(NULL);  // 当前发送心跳消息时间点
		++curConn->hbResult;                  // 心跳失败次数
		
		// 发送心跳请求消息检测连接是否正常
		static const NetPkgHeader hbRequestPkgHeader(htonl(netPkgHeaderLen), CTL, HB_RQ);
		writeData(curConn, (const char*)&hbRequestPkgHeader, netPkgHeaderLen);
	}
}

bool CDataHandler::readPkgHeader(Connect* curConn, unsigned int& dataSize)
{
	char netPkgHeader[netPkgHeaderLen] = {0};
	
	if (dataSize < netPkgHeaderLen) return false;
		
	if (readData(curConn, netPkgHeader, netPkgHeaderLen) != netPkgHeaderLen)
	{
		curConn->logicStatus = exception;  // 逻辑数据异常了
		ReleaseErrorLog("read net pkg header error, id = %lld, fd = %d, pkg header size = %d, data size = %d",
		curConn->id, curConn->fd, netPkgHeaderLen, dataSize);
		return false;
	}
	
	dataSize -= netPkgHeaderLen;  // 剩余数据量
	NetPkgHeader* pkgHeader = (NetPkgHeader*)netPkgHeader;
	if (pkgHeader->type == MSG)  // 用户数据包
	{
		pkgHeader->len = ntohl(pkgHeader->len);  // 注意长度需要转成主机字节序
		if (pkgHeader->len <= netPkgHeaderLen)
		{
			curConn->logicStatus = exception;  // 逻辑数据异常了
			ReleaseErrorLog("read user net pkg header error, id = %lld, fd = %d, pkg len = %d", curConn->id, curConn->fd, pkgHeader->len);
			return false;
		}

		curConn->needReadSize = pkgHeader->len - netPkgHeaderLen;  // 用户数据包长度
		return true;
	}
	else if (pkgHeader->type == CTL)  // 控制类型数据包
	{
		if (pkgHeader->cmd == HB_RP || pkgHeader->cmd == HB_RQ)  // 心跳应答消息，心跳请求包
		{
			curConn->heartBeatSecs = time(NULL);
			curConn->hbResult = 0;  // 连接正常
			
			static const NetPkgHeader hbReplyPkgHeader(htonl(netPkgHeaderLen), CTL, HB_RP);
			if (pkgHeader->cmd == HB_RQ) writeData(curConn, (const char*)&hbReplyPkgHeader, netPkgHeaderLen);  // 发送心跳应答消息包
			
			return (dataSize >= netPkgHeaderLen) ? readPkgHeader(curConn, dataSize) : false;  // 继续尝试读取数据包头部
		}
	}
	
	curConn->logicStatus = exception;  // 逻辑数据异常了
	ReleaseErrorLog("read net pkg header error, id = %lld, fd = %d, pkg len = %d, type = %d, cmd = %d", curConn->id, curConn->fd, pkgHeader->len, pkgHeader->type, pkgHeader->cmd);
	return false;
}

bool CDataHandler::readPkgBody(Connect* curConn, unsigned int dataSize, unsigned int& pkgLen, char* pkgData)
{
	if (curConn->needReadSize > 0 && dataSize >= (unsigned int)curConn->needReadSize)
	{
		m_connMgr->setConnectNormal(curConn);  // 有数据则填写活跃时间点
		
		// 直接拷贝数据到缓冲区空间
		if (pkgData != NULL)
		{
			if ((unsigned int)curConn->needReadSize > pkgLen)
			{
				ReleaseErrorLog("read net pkg failed, need read size = %d, pkg buff size = %d", curConn->needReadSize, pkgLen);
				return false;
			}
			
			if (readData(curConn, pkgData, curConn->needReadSize) == (unsigned int)curConn->needReadSize)  // 从连接读出数据写入逻辑层buff
			{
				pkgLen = curConn->needReadSize;
				curConn->needReadSize = 0;  // 下一次读取数据包头部
				return true;
			}
			
			curConn->logicStatus = exception;  // 逻辑数据异常了
			ReleaseErrorLog("read net pkg data to buff error, id = %ld, fd = %d, need read size = %d, data size = %d",
			curConn->id, curConn->fd, curConn->needReadSize, dataSize);
			return false;
		}
				

		// 从逻辑层申请空间写数据
		BufferHeader* buffHeader = m_logicHandler->getBufferHeader();
		if (buffHeader != NULL)
		{
			if (readData(curConn, buffHeader->header, buffHeader->len) != buffHeader->len)  // 先读头部数据
			{
				curConn->logicStatus = exception;  // 逻辑数据异常了
				ReleaseErrorLog("read net pkg user header error, id = %ld, fd = %d, need read size = %d, data size = %d",
				curConn->id, curConn->fd, buffHeader->len, dataSize);
				return false;
			}
			curConn->needReadSize -= buffHeader->len;
		}
		
		void* logicCb = NULL;
		char* logicWriteBuff = m_logicHandler->getWriteBuffer(curConn->needReadSize, curConn->id, buffHeader, logicCb);
		if (curConn->needReadSize == 0) return true;  // 存在只有用户数据包头的情况
		
		if (logicWriteBuff != NULL)
		{
			bool isRead = (logicWriteBuff != (char*)-1);  // 是否直接丢弃数据，false则丢弃数据
			if (readData(curConn, logicWriteBuff, curConn->needReadSize, isRead) == (unsigned int)curConn->needReadSize)  // 从连接读出数据写入逻辑层buff
			{
				if (isRead) m_logicHandler->submitWriteBuffer(logicWriteBuff, curConn->needReadSize, curConn->id, logicCb);  // 提交数据到逻辑层
				curConn->needReadSize = 0;  // 下一次读取数据包头部
				return true;
			}
			
			curConn->logicStatus = exception;  // 逻辑数据异常了
			ReleaseErrorLog("read net pkg body error, id = %ld, fd = %d, need read size = %d, data size = %d",
			curConn->id, curConn->fd, curConn->needReadSize, dataSize);
		}
	}
	
	return false;
}

bool CDataHandler::handleData(Connect* curConn, unsigned int dataSize, unsigned int curSecs, unsigned int& pkgLen, char* pkgData)
{
	// 根据标志读取头部数据或者是用户数据包体
	bool isOK = false;
	if (curConn->needReadSize == 0)
	{
		if (readPkgHeader(curConn, dataSize)) isOK = readPkgBody(curConn, dataSize, pkgLen, pkgData);
	}
	else
	{
		isOK = readPkgBody(curConn, dataSize, pkgLen, pkgData);
	}

	if (!isOK)
	{
		if (curConn->logicStatus == exception)
		{
			releaseWriteBuffer(curConn);          // 异常则断开连接，释放资源
			curConn->logicStatus = deleted;       // 标志此连接为无效连接，可删除了
		}
		else
		{
            writeHbRequestPkg(curConn, curSecs);  // 发送心跳请求包检查网络连接
		}
	}
	
    return isOK;
}

int CDataHandler::handleData(Connect* msgConnList, NetPkgHeader& userPkgHeader)
{
	unsigned int pkgLen = 0;
	int handledMsgCount = 0;
	unsigned int curSecs = time(NULL);
	unsigned int dataSize = 0;
	Connect* curConn = NULL;
	while (msgConnList != NULL && isRunning())
	{
		if (readFromLogic(userPkgHeader)) ++handledMsgCount;  // 从逻辑层读数据写入对应的连接
		
		curConn = msgConnList;
		msgConnList = msgConnList->pNext;
		
		if (curConn->logicStatus == deleted) continue;  // 忽略无效的连接
		
		if (curConn->connStatus == closed)
		{
			releaseWriteBuffer(curConn);
			curConn->logicStatus = deleted;             // 标志此连接为无效连接，可删除了
			continue;
		}
		
		// 这里等待一下，等待连接处理线程处理完
		// 连接线程删除无效连接，因此等待成功则必须重新取下一个连接，否则如果当前msgConnList为无效连接的话会出错
		if (m_connMgr->waitConnecter()) msgConnList = curConn->pNext;
		
		// 从连接读数据写入逻辑层
		dataSize = getCanReadDataSize(curConn);  // 当前可以从连接读取的数据量
		if (dataSize == 0)
		{
			writeHbRequestPkg(curConn, curSecs);  // 发送心跳请求包检查网络连接
		}
		else if (handleData(curConn, dataSize, curSecs, pkgLen))  // 存在数据则读取，写入逻辑层
		{
			++handledMsgCount;
		}
	}
	
	return handledMsgCount;
}

// 从逻辑层读数据写入对应的连接
bool CDataHandler::readFromLogic(NetPkgHeader& userPkgHeader)
{
	int readLogicLen = 0;
	uuid_type connLogicId = 0;
	bool isNeedWriteMsgHeader = true;
	void* logicCb = NULL;
	char* logicReadBuff = m_logicHandler->getReadBuffer(readLogicLen, connLogicId, isNeedWriteMsgHeader, logicCb);  // 尝试从逻辑层读数据
	if (logicReadBuff != NULL)
	{
		Connect* writeToConn = m_connMgr->getMsgConnect(connLogicId);
		if (writeToConn == NULL || writeToConn->connStatus != normal || writeToConn->logicStatus != normal)  // 无效连接的数据，则通知逻辑层connLogicId对应的连接无效了
		{
			void* cb = (writeToConn != NULL) ? writeToConn->userCb : NULL;
			m_logicHandler->onInvalidConnect(connLogicId, cb);
		}
		else
		{
			if (isNeedWriteMsgHeader)
			{
				userPkgHeader.len = htonl(netPkgHeaderLen + readLogicLen);
				writeData(writeToConn, (const char*)&userPkgHeader, netPkgHeaderLen);  // 1) 先写数据包头部信息
			}
			
			if (writeData(writeToConn, logicReadBuff, readLogicLen))                   // 2) 再把逻辑层的数据写到连接里
			{
				m_logicHandler->submitReadBuffer(logicReadBuff, readLogicLen, connLogicId, logicCb);  // 成功则提交数据
				return true;
			}
		}
	}
	
	return false;
}

void CDataHandler::run()
{
	detach();  // 分离自己，线程结束则自动释放资源
	
	const int waitMillisecond = 1000 * 1;  // 无数据处理时等待的时间，1毫秒
	NetPkgHeader userPkgHeader(0, MSG, 0);  // 用户数据包头
	Connect* msgConnList = NULL;
	while (m_status == running)
	{
		m_connMgr->waitConnecter();  // 等待连接处理线程处理完
		msgConnList = m_connMgr->getMsgConnectList();
		if (msgConnList != NULL) // 处理连接数据
		{
			if (handleData(msgConnList, userPkgHeader) < 1) usleep(waitMillisecond);  // 无数据处理则等待让出cpu
		}
		else if (!readFromLogic(userPkgHeader))
		{
			 usleep(waitMillisecond);  // 无数据处理则等待让出cpu
		}
	}

	// 释放所有连接的写buffer内存块
	msgConnList = m_connMgr->getMsgConnectList();
	while (msgConnList != NULL)
	{
	    releaseWriteBuffer(msgConnList);
		msgConnList = msgConnList->pNext;
	}
	
	// 终止线程运行
	m_status = stopted;
	stop();
}

const char* CDataHandler::getName()
{
	return "NetDataHandler";
}


// 由逻辑层主动收发消息
ReturnValue CDataHandler::recv(Connect*& conn, char* data, unsigned int& len)
{
	// 1）先过滤掉无效的连接
	while (m_curConn != NULL && m_curConn->logicStatus == ConnectStatus::deleted) m_curConn = m_curConn->pNext;
	Connect* nextConn = (m_curConn != NULL) ? m_curConn->pNext : NULL;  // 必须在wait之前获取，wait之后如果m_curConn被移出队列则next已经为空了
	
	// 2）接着等待连接处理线程处理完
	m_connMgr->waitConnecter();
	if (m_curConn == NULL || m_curConn->readSocketTimes == 0)  // wait之后存在m_curConn可能被连接线程移出队列了
	{
		m_curConn = (nextConn != NULL) ? nextConn : m_connMgr->getMsgConnectList();  // 必须确保连接在队列中，否则会错误、无限循环
		if (m_curConn == NULL) return NotNetData;
	}

	unsigned int curSecs = time(NULL);
	unsigned int dataSize = 0;
	Connect* const beginConn = m_curConn;
	while (true)
	{
		if (m_curConn->connStatus == ConnectStatus::normal && m_curConn->logicStatus == ConnectStatus::normal)
		{
			dataSize = getCanReadDataSize(m_curConn);  // 当前可以从连接读取的数据量
			if (dataSize == 0)
			{
				writeHbRequestPkg(m_curConn, curSecs);  // 发送心跳请求包检查网络连接
			}
			else if (handleData(m_curConn, dataSize, curSecs, len, data))  // 存在数据则读取，写入逻辑层
			{
				conn = m_curConn;
				m_curConn = m_curConn->pNext;  // 切换到下一个连接，以便均衡遍历所有连接
				return OptSuccess;
			}
		}
		
		// 删除销毁已经关闭的连接
		else if (m_curConn->connStatus == ConnectStatus::closed && m_curConn->logicStatus != ConnectStatus::deleted)
		{
			releaseWriteBuffer(m_curConn);
			m_curConn->logicStatus = ConnectStatus::deleted;  // 标志此连接为无效连接，可删除了
		}
		
		m_curConn = (m_curConn->pNext != NULL) ? m_curConn->pNext : m_connMgr->getMsgConnectList();
		if (m_curConn == beginConn) break;  // 回到循环起点了，无可处理的消息
	}
	
	return NotNetData;
}

ReturnValue CDataHandler::send(Connect* conn, const char* data, const unsigned int len, bool isNeedWriteMsgHeader)
{
	if (conn->connStatus != ConnectStatus::normal || conn->logicStatus != ConnectStatus::normal)  // 无效连接的数据
	{
		close(conn);
		return ConnectException;
	}
	
	if (isNeedWriteMsgHeader)
	{
		NetPkgHeader userPkgHeader(htonl(netPkgHeaderLen + len), MSG, 0);  // 用户数据包头
		writeData(conn, (const char*)&userPkgHeader, netPkgHeaderLen);     // 1) 先写数据包头部信息
	}
	
	return writeData(conn, data, len) ? OptSuccess : SndDataFailed;        // 2) 再把逻辑层的数据写到连接里
}

void CDataHandler::close(Connect* conn)
{
	if (conn == m_curConn) m_curConn = m_curConn->pNext;
	
	if (getCanWriteDataSize(conn) > 0)
	{
		conn->logicStatus = ConnectStatus::sendClose;  // 连接里的数据发送完毕后关闭连接
	}
	else
	{
    	releaseWriteBuffer(conn);
		conn->logicStatus = ConnectStatus::deleted;    // 标志此连接为无效连接，可删除了
	}
}




// 逻辑层使用该接口从连接里收发数据
CLogicConnectMgr::CLogicConnectMgr(CDataHandler* dataHandler) : m_dataHandler(dataHandler)
{
	
}

CLogicConnectMgr::~CLogicConnectMgr()
{
	m_dataHandler = NULL;
}

ReturnValue CLogicConnectMgr::recv(Connect*& conn, char* data, unsigned int& len)
{
	return m_dataHandler->recv(conn, data, len);
}

ReturnValue CLogicConnectMgr::send(Connect* conn, const char* data, const unsigned int len, bool isNeedWriteMsgHeader)
{
	return m_dataHandler->send(conn, data, len, isNeedWriteMsgHeader);
}
	
void CLogicConnectMgr::close(Connect* conn)
{
	m_dataHandler->close(conn);
}


	
// 防止操作异常情况下线程不解锁
SynWaitNotify::SynWaitNotify(CConnectManager* connMgr) : m_connMgr(connMgr)
{
	m_connMgr->waitDataHandler();       // 等待数据处理线程处理完
}

SynWaitNotify::~SynWaitNotify()
{
	m_connMgr->notifyDataHandler();     // 通知数据处理线程执行处理操作
	m_connMgr = NULL;
}

}

