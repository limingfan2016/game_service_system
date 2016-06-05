
/* author : limingfan
 * date : 2014.11.20
 * description : TCP连接相关实现
 */
 
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "CTcpConnect.h"
#include "base/MacroDefine.h"
#include "base/ErrorCode.h"


using namespace NCommon;
using namespace NErrorCode;


namespace NConnect
{

CTcpConnect::CTcpConnect(const char* ip, const int port) : CSocket(ip, port)
{
	m_isNormal = false;
}

CTcpConnect::~CTcpConnect()
{
	destroy();
}

int CTcpConnect::create(int listenNum)
{
	int rc = open(SOCK_STREAM);  // tcp连接socket
	if (rc != Success)
	{
		return rc;
	}
	
	rc = setReuseAddr(1);
	if (rc != Success)
	{
		return rc;
	}
	
	rc = setNagle(1);
	if (rc != Success)
	{
		return rc;
	}
	
	rc = setNoBlock();
	if (rc != Success)
	{
		return rc;
	}
	
	rc = bind();
	if (rc != Success)
	{
		return rc;
	}
	
	rc = listen(listenNum);
	m_isNormal = (rc == Success);

	return rc;
}

void CTcpConnect::destroy()
{
	m_isNormal = false;
	close();
}

bool CTcpConnect::isNormal()
{
	return (m_fd > 0) && m_isNormal;
}
	
int CTcpConnect::accept(int& fd, in_addr& peerIp, unsigned short& peerPort, int& errorCode)
{
	errorCode = Success;
	struct sockaddr_in peerAddr;
	socklen_t peerSize = sizeof(peerAddr);
	memset(&peerAddr, 0, peerSize);
	fd = ::accept(m_fd, (struct sockaddr*)&peerAddr, &peerSize);
	if (fd == -1)
	{
		errorCode = errno;
		if (errorCode != EAGAIN && errorCode != EINTR)
		{
		    ReleaseErrorLog("accept connect server ip = %s, port = %d, error = %d, info = %s", m_ip, m_port, errno, strerror(errno));
		}
		return AcceptConnectFailed;
	}

    memcpy(&peerIp, &peerAddr.sin_addr, sizeof(peerIp));
	peerPort = ntohs(peerAddr.sin_port);

	return Success;
}

}

