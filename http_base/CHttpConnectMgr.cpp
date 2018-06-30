
/* author : limingfan
 * date : 2015.07.01
 * description : 连接管理
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include "CHttpConnectMgr.h"
#include "CHttpData.h"
#include "base/ErrorCode.h"
#include "base/CProcess.h"
#include "connect/CSocket.h"


using namespace NErrorCode;

namespace NHttp
{

// 忽略管道SIGPIPE信号
static void SignalHandler(int sigNum, siginfo_t* sigInfo, void* context)
{
	ReleaseWarnLog("in http service, receive signal = %d", sigNum);
}


CHttpConnectMgr::CHttpConnectMgr() : m_dataQueue(MaxDataQueueSize)
{
}

CHttpConnectMgr::~CHttpConnectMgr()
{
}

int CHttpConnectMgr::starSrv()
{
	// epoll 模型监听连接
    int rc = m_epoll.create(1);
    if (rc != Success) return rc;
	
	rc = start();
	if (rc != Success) return rc;
	
	return Success;
}

void CHttpConnectMgr::stopSrv()
{
	if (m_status != SrvStatus::Stop) m_status = Stopping;
}

bool CHttpConnectMgr::isRunning()
{
	return (m_status != SrvStatus::Stop);
}

bool CHttpConnectMgr::doHttpConnect(const char* ip, const unsigned short port, ConnectData* connData)
{
	if (ip == NULL || *ip == '\0' || connData == NULL) return false;

    connData->fd = -1;
	connData->status = ConnectStatus::ConnectError;

	int errorCode = -1;
    int rc = m_activeConnect.init(ip, port);
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

		connData->fd = m_activeConnect.getFd();   // 返回tcp socket文件描述符
		connData->receiveData[0] = '\0';
		connData->receiveDataIdx = 0;
		connData->receiveDataLen = MaxReceiveNetBuffSize;
		connData->sslConnect = (HttpsPort == port) ? CSSLObject::getInstance().createSSLInstance() : NULL;  // 可能是 https 消息
		
		connData->lastTimeSecs = time(NULL) + connData->timeOutSecs;  // 建立连接超时时间点
		if (errorCode != Success)
		{
			// 连接建立中，得等待连接建立完毕成功。。。
			connData->status = ConnectStatus::Connecting;
		}
		else
		{
			connData->status = ConnectStatus::Normal;  // 连接建立直接成功了
			if (connData->sslConnect != NULL) doSSLConnect(connData);  // 存在 https 调用
		}
		
		// 新连接加入epoll监听模型
		struct epoll_event optEv;
		optEv.data.ptr = connData;
		optEv.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;  // 同时监听读写事件，边缘触发模式
		rc = m_epoll.addListener(connData->fd, optEv);
	}
	while (0);
	
	if (rc != Success)
	{
		OptErrorLog("do connect http service error, ip = %s, port = %d, rc = %d", ip, port, rc);
		m_activeConnect.close();
	}
	m_activeConnect.unInit();

    return (rc == Success);
}

ConnectData* CHttpConnectMgr::receiveConnectData()
{
	return (ConnectData*)m_dataQueue.get();
}

bool CHttpConnectMgr::haveConnectData()
{
	return m_dataQueue.have();
}

void CHttpConnectMgr::sendConnectData(ConnectData* connData)
{
	if (connData != NULL)
	{
		// http短连接，应答消息回来后直接关闭连接
		// 必须先关闭连接，防止epoll触发其他事件（比如对端先关连接事件）导致重复处理错误
		closeHttpConnect(connData);

	    m_dataQueue.put(connData);
	}
}

void CHttpConnectMgr::closeHttpConnect(ConnectData* connData)
{
	if (connData->fd > 0)
	{
		m_epoll.removeListener(connData->fd);  // 删除监听连接
	    ::close(connData->fd);
		connData->fd = -1;
	}
	
	if (connData->sslConnect != NULL)
	{
		CSSLObject::getInstance().destroySSLInstance(connData->sslConnect);
		connData->sslConnect = NULL;
	}
}


void CHttpConnectMgr::run()
{
	detach();  // 分离自己，线程结束则自动释放资源

	// 忽略管道SIGPIPE信号
	CProcess::installSignal(SIGPIPE, NHttp::SignalHandler);
	
	ReleaseInfoLog("start to do epoll listen for http connect");

    int fdCount = -1;
	ConnectData* connData = NULL;
	const int maxEvents = 8192;                  // 最大监听事件个数
    struct epoll_event waitEvents[maxEvents];    // 监听epoll事件数组
	
	m_status = SrvStatus::Run;  // 连接服务运行状态
	while(m_status == SrvStatus::Run)
	{
		// 等待epoll事件的发生，一次无法输出全部事件，下次会继续输出，因此事件不会丢失
        fdCount = m_epoll.listen(waitEvents, maxEvents, 1);

		if (m_status != SrvStatus::Run) break;  // 连接服务退出
		
		for (int i = 0; i < fdCount; ++i)
		{
			connData = (ConnectData*)(waitEvents[i].data.ptr);
			if (connData->status == ConnectStatus::Connecting)  // http 连接建立中
			{
				onActiveConnect(waitEvents[i].events, connData);
			}
			else if (connData->status == ConnectStatus::SSLConnecting)  // https SSL 连接建立中
			{
				doSSLConnect(connData);
			}
			else
			{
				handleConnect(waitEvents[i].events, connData);
			}
		}
	}
	
	ReleaseInfoLog("finish to do epoll listen for http connect");
	m_status = SrvStatus::Stop;
}

void CHttpConnectMgr::onActiveConnect(uint32_t eventVal, ConnectData* conn)
{
    // 检查本地端发起的主动连接
	int err = 0;
	socklen_t len = sizeof(err); 
	int rc = CSocket::getOpt(conn->fd, SOL_SOCKET, SO_ERROR, (void*)&err, &len);
	
	// 连接异常则直接关闭删除
	if ((eventVal & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
		|| (rc != Success) || (err != EINPROGRESS && err != 0))
	{
		OptErrorLog("create http connect error, fd = %d, event = %d, rc = %d, error = %d, info = %s",
	    conn->fd, (eventVal & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)), rc, err, strerror(err));
		
		conn->status = ConnectStatus::ConnectError;
		sendConnectData(conn);
		return;
	}

	if (err == EINPROGRESS)
	{
		if (time(NULL) < conn->lastTimeSecs) return;  // 没超时则继续等待
		
		OptWarnLog("connect http host time out, requestId = %d, key = %s", conn->keyData.requestId, conn->keyData.key);
		
		conn->status = ConnectStatus::TimeOut;
		sendConnectData(conn);
		return;
	}

	conn->status = ConnectStatus::Normal;  // 连接建立成功了
	if (conn->sslConnect != NULL) return doSSLConnect(conn);  // 存在 https 调用

	if (eventVal & EPOLLOUT) writeToConnect(conn);  // 可写数据
	if (eventVal & EPOLLIN) readFromConnect(conn);  // 有数据可读
}

// 处理监听的连接（读写数据）
void CHttpConnectMgr::handleConnect(uint32_t eventVal, ConnectData* conn)
{
	if (eventVal & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))   // 连接异常，需要关闭此类连接
	{
		OptWarnLog("ready close error http connect, have data = %d", (eventVal & EPOLLIN));
		
		if (eventVal & EPOLLIN)
		{
	        readFromConnect(conn);   // 存在连接有数据可读的情况则先读最后的数据
		}
		else
		{
			conn->status = ConnectStatus::ConnectError;
			sendConnectData(conn);
		}
	}
	else
	{
		if (eventVal & EPOLLOUT) writeToConnect(conn);   // 可写数据
		if (eventVal & EPOLLIN)  readFromConnect(conn);  // 有数据可读
	}
}

void CHttpConnectMgr::writeToConnect(ConnectData* conn)
{
	if (!isRunning() || conn->sendDataLen < 1) return;

    if (conn->sslConnect != NULL) return writeToSSLConnect(conn);
	
	int nWrite = ::write(conn->fd, conn->sendData, conn->sendDataLen);
	if (nWrite > 0)
	{
		conn->sendDataLen -= nWrite;
		if (conn->sendDataLen > 0) memmove(conn->sendData, conn->sendData + nWrite, conn->sendDataLen);
		else conn->status = ConnectStatus::AlreadyWrite;
		
		// OptInfoLog("write data to http connect, remain len = %d, status = %d fd = %d", conn->sendDataLen, conn->status, conn->fd);
		
		return;
	}
	
	if (nWrite < 0)
	{
		// nWrite < 0 的情况
		if (errno == EINTR) return writeToConnect(conn);  // 被信号中断则继续读写数据
		if (errno == EAGAIN) return;  // 没有socket缓冲区可写了
		
		OptWarnLog("write to http connect error, nWrite = %d, errno = %d, info = %s", nWrite, errno, strerror(errno));
		conn->status = ConnectStatus::ConnectError;
		sendConnectData(conn);
	}
}

void CHttpConnectMgr::readFromConnect(ConnectData* conn)
{
	if (!isRunning()) return;
	
	if (conn->sslConnect != NULL) return readFromSSLConnect(conn);

	int nRead = ::read(conn->fd, conn->receiveData + conn->receiveDataIdx, conn->receiveDataLen);
	if (nRead > 0)
	{
		const int status = parseReadData(nRead, conn);
		if (status == ConnectStatus::DataError || status == ConnectStatus::CanRead) sendConnectData(conn);
		return;
	}
	else if (nRead == 0)
	{
		conn->status = ConnectStatus::CanRead;
		sendConnectData(conn);
		return;
	}
	
	// nRead < 0 的情况
	if (errno == EINTR)
	{
		errno = 0;
		return readFromConnect(conn);  // 被信号中断则继续读数据
	}
	
	if (errno == EAGAIN) return;  // 没有数据可读了
	
	OptWarnLog("read from http connect, nRead = %d, errno = %d, info = %s", nRead, errno, strerror(errno));
	conn->status = ConnectStatus::ConnectError;
	sendConnectData(conn);
}

int CHttpConnectMgr::parseReadData(int nRead, ConnectData* conn)
{
	if (nRead > 0)
	{
		conn->receiveDataIdx += nRead;
		conn->receiveDataLen -= nRead;
		conn->receiveData[conn->receiveDataIdx] = '\0';
		const char* flag = strstr(conn->receiveData, RequestHeaderFlag);  // 是否是完整的http请求头部数据
		if (conn->receiveDataLen == 0)  // 数据量过大，直接错误
		{
			conn->status = ConnectStatus::DataError;
		}
		else if (flag != NULL)  // 是否是完整的http请求头部数据
		{
			unsigned int bodyLen = MaxReceiveNetBuffSize;
		    int rc = Success;

			// 是否是完整的数据体数据
			if (strstr(flag + RequestHeaderFlagLen, RequestHeaderFlag) != NULL
			    || (rc = CHttpDataParser::getHttpBodyLen(conn->receiveData, flag + RequestHeaderFlagLen, bodyLen)) == Success)
			{
				conn->status = ConnectStatus::CanRead;
			}
			else if (rc == BodyDataError)
			{
				// conn->status = ConnectStatus::DataError;
		        // sendConnectData(conn);
			}
			
			// OptWarnLog("read data from connect, data = %s, status = %d, fd = %d, key = %s, len = %d, rc = %d",
		    // conn->receiveData, conn->status, conn->fd, conn->keyData.key, bodyLen, rc);
		}
	}
	
	return conn->status;
}


// https 消息使用
void CHttpConnectMgr::doSSLConnect(ConnectData* connData)
{
	if (connData->sslConnect != NULL)
	{
		do
		{
			if (connData->status == ConnectStatus::Normal)
			{
				connData->status = ConnectStatus::SSLConnecting;  // 建立ssl连接中。。。
				if (SSL_set_fd(connData->sslConnect, connData->fd) == 0)
				{
					connData->status = ConnectStatus::SSLConnectError;
					ERR_print_errors_fp(stderr);
					OptErrorLog("call SSL_set_fd error");
					break;
				}
				
				SSL_set_connect_state(connData->sslConnect);
			}

			int code = SSL_do_handshake(connData->sslConnect);  // 非阻塞ssl握手动作
			if (code == 1)
			{
				// 成功了则直接写数据
				connData->status = ConnectStatus::Normal;
				return writeToSSLConnect(connData);
			}
			
			int err = SSL_get_error(connData->sslConnect, code);
			if (err != SSL_ERROR_WANT_WRITE && err != SSL_ERROR_WANT_READ)
			{
				connData->status = ConnectStatus::SSLConnectError;
			    ERR_print_errors_fp(stderr);
				OptErrorLog("call SSL_do_handshake error, code = %d, err = %d", code, err);
				break;
			}

			if (time(NULL) > connData->lastTimeSecs)  // 超时则退出
			{
				connData->status = ConnectStatus::TimeOut;
				OptWarnLog("connect https to do SSL operation host time out, requestId = %d, key = %s", connData->keyData.requestId, connData->keyData.key);
			}

		} while (0);
		
		if (connData->status != ConnectStatus::SSLConnecting) sendConnectData(connData);
	}
}

void CHttpConnectMgr::writeToSSLConnect(ConnectData* connData)
{
	if (connData->sendDataLen > 0)
	{
		int nWrite = SSL_write(connData->sslConnect, connData->sendData, connData->sendDataLen);
		if (nWrite > 0)
		{
	        connData->sendDataLen -= nWrite;
			if (connData->sendDataLen > 0) memmove(connData->sendData, connData->sendData + nWrite, connData->sendDataLen);
		    else connData->status = ConnectStatus::AlreadyWrite;

			return;
		}
		else if (SSL_get_error(connData->sslConnect, nWrite) != SSL_ERROR_WANT_WRITE)  // 如果是 SSL_ERROR_WANT_WRITE 则继续等待下一次写事件
		{
			OptWarnLog("write to https connect error, nWrite = %d, err = %d", nWrite, SSL_get_error(connData->sslConnect, nWrite));
			connData->status = ConnectStatus::SSLConnectError;
			sendConnectData(connData);
		}
	}
}

void CHttpConnectMgr::readFromSSLConnect(ConnectData* connData)
{
	int nRead = SSL_read(connData->sslConnect, connData->receiveData + connData->receiveDataIdx, connData->receiveDataLen);
	if (nRead > 0)
	{
		const int status = parseReadData(nRead, connData);
		if (status == ConnectStatus::DataError || status == ConnectStatus::CanRead) sendConnectData(connData);

		return;
	}

    int err = SSL_get_error(connData->sslConnect, nRead);
	if (err == SSL_ERROR_ZERO_RETURN)  // 已经结束了
	{
		connData->status = ConnectStatus::CanRead;
		sendConnectData(connData);
	}
	else if (err != SSL_ERROR_WANT_READ)  // 如果是 SSL_ERROR_WANT_READ 则继续等待下一次读事件
	{
		OptWarnLog("read from https connect error, nRead = %d, err = %d", nRead, err);
		
		// 0 The read operation was not successful. 
		// The reason may either be a clean shutdown due to a "close notify" alert sent by the peer 
		// (in which case the SSL_RECEIVED_SHUTDOWN
		connData->status = (nRead == 0 || err == SSL_RECEIVED_SHUTDOWN) ? ConnectStatus::CanRead : ConnectStatus::SSLConnectError;
		sendConnectData(connData);
	}
}

}

