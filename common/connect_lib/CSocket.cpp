
/* author : admin
 * date : 2014.11.15
 * description : 网络连接封装实现
 */
 
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "CSocket.h"
#include "base/MacroDefine.h"
#include "base/ErrorCode.h"


using namespace NCommon;
using namespace NErrorCode;


namespace NConnect
{

CSocket::CSocket()
{
	init(NULL, 0);
}
	
CSocket::CSocket(const char* ip, const unsigned short port)
{
	init(ip, port);
}

CSocket::~CSocket()
{
	unInit();
}


const char* CSocket::toIPStr(const struct in_addr& ip)
{
	return ::inet_ntoa(ip);
}

unsigned int CSocket::toIPInt(const char* ip)
{
	if (ip == NULL || *ip == '\0') return 0;
	
	return ::inet_addr(ip);
}

int CSocket::toIPInAddr(const char* ipStr, struct in_addr& ipInAddr)
{
	if (ipStr == NULL || *ipStr == '\0' || inet_aton(ipStr, &ipInAddr) == 0) return InValidIp;
	return Success;
}

int CSocket::getOpt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
{
	if (::getsockopt(sockfd, level, optname, optval, optlen) == -1)
	{
		ReleaseErrorLog("get socket option failed, error = %d, info = %s", errno, strerror(errno));
		return SystemCallErr;
	}
	return Success;
}
	

int CSocket::init(const char* ip, const unsigned short port)
{
	unInit();
	
	if (ip == NULL || *ip == '\0' || port < 1) return InvalidParam;
	
	strncpy(m_ip, ip, StrIPLen - 1);
	m_port = port;
	return Success;
}

void CSocket::unInit()
{
	m_ip[0] = '\0';
	m_port = 0;
	m_fd = -1;
}


int CSocket::open(int type)
{
	int fd = socket(AF_INET, type, 0);
	if (fd == -1)
	{
		ReleaseErrorLog("create socket type = %d, error = %d, info = %s", type, errno, strerror(errno));
		return CreateSocketFailed;
	}
	
	m_fd = fd;
	return Success;
}

int CSocket::open(int family, int type, int protocol)
{
	int fd = socket(family, type, protocol);
	if (fd == -1)
	{
		ReleaseErrorLog("create socket family = %d, type = %d, protocol = %d, error = %d, info = %s", family, type, protocol, errno, strerror(errno));
		return CreateSocketFailed;
	}
	
	m_fd = fd;
	return Success;
}

int CSocket::close()
{
	if (m_fd != -1)
	{
		if (::close(m_fd) != 0)
		{
			ReleaseErrorLog("close socket error = %d, info = %s", errno, strerror(errno));
			return CloseSocketFailed;
		}
		m_fd = -1;
	}
	return Success;
}

int CSocket::bind()
{
	if (*m_ip == '\0' || m_port < 1)
	{
		ReleaseErrorLog("bind param invalid, ip = %s, port = %d", m_ip, m_port);
		return InValidIp;
	}

	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(m_port);
	if (inet_aton(m_ip, &servAddr.sin_addr) == 0)
	{
		ReleaseErrorLog("bind convert ip = %s failed", m_ip);
		return InValidIp;
	}
	
	if (::bind(m_fd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
	{
		ReleaseErrorLog("bind ip = %s, port = %d, error = %d, info = %s", m_ip, m_port, errno, strerror(errno));
		return BindIpPortFailed;
	}
	
	return Success;
}

int CSocket::listen(int num)
{
	if (num < 1)
	{
		return InvalidParam;
	}
	
	if (::listen(m_fd, num) == -1)
	{
		ReleaseErrorLog("listen ip = %s, port = %d, queue num = %d, error = %d, info = %s", m_ip, m_port, num, errno, strerror(errno));
		return ListenConnectFailed;
	}
	
	return Success;
}

int CSocket::connect(int& errorCode)
{
	errorCode = Success;
	struct sockaddr_in clientAddr;
	memset(&clientAddr, 0, sizeof(clientAddr));
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(m_port);
	if (inet_aton(m_ip, &clientAddr.sin_addr) == 0)
	{
		ReleaseErrorLog("connect convert ip = %s failed", m_ip);
		return InValidIp;
	}

	if (::connect(m_fd, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) == -1)
	{
		errorCode = errno;
		if (errorCode != EINPROGRESS)  // 非阻塞式调用连接
		{
			ReleaseErrorLog("connect ip = %s, port = %d, error = %d, info = %s", m_ip, m_port, errno, strerror(errno));
			return ConnectIpPortFailed;
		}
	}
	
	return Success;
}


int CSocket::setLinger(int onOff, int val)
{
	struct linger lger;
	lger.l_onoff = onOff;
	lger.l_linger = val;
	if (setsockopt(m_fd, SOL_SOCKET, SO_LINGER, &lger, sizeof(lger)) == -1)
	{
		ReleaseErrorLog("set linger onOff = %d, value = %d, error = %d, info = %s", onOff, val, errno, strerror(errno));
		return SetLingerFailed;
	}
	
	return Success;
}

int CSocket::setRcvBuff(int val)
{
	if (setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val)) == -1)
	{
		ReleaseErrorLog("set receive buff, value = %d, error = %d, info = %s", val, errno, strerror(errno));
		return SetRcvBuffFailed;
	}
	
	return Success;
}

int CSocket::setSndBuff(int val)
{
	if (setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, &val, sizeof(val)) == -1)
	{
		ReleaseErrorLog("set send buff, value = %d, error = %d, info = %s", val, errno, strerror(errno));
		return SetSndBuffFailed;
	}
	
	return Success;
}

int CSocket::setReuseAddr(int val)
{
	if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1)
	{
		ReleaseErrorLog("set reuse ip, value = %d, error = %d, info = %s", val, errno, strerror(errno));
		return SetReuseAddrFailed;
	}
	
	return Success;
}

int CSocket::setNagle(int val)
{
	if (setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1)
	{
		ReleaseErrorLog("set nagle, value = %d, error = %d, info = %s", val, errno, strerror(errno));
		return SetNagleFailed;
	}
	
	return Success;
}

int CSocket::setNoBlock()
{
	return setNoBlock(m_fd);
}

int CSocket::setNoBlock(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1)
	{
		ReleaseErrorLog("get flag for set not block, error = %d, info = %s", errno, strerror(errno));
		return SetNonBlockFailed;
	}
	
	if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)
	{
		ReleaseErrorLog("set not block, error = %d, info = %s", errno, strerror(errno));
		return SetNonBlockFailed;
	}
	
	return Success;
}

int CSocket::getFd() const
{
	return m_fd;
}

const char* CSocket::getIp() const
{
	return m_ip;
}

unsigned short CSocket::getPort() const
{
	return m_port;
}

}

