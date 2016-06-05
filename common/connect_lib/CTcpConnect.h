
/* author : limingfan
 * date : 2014.11.20
 * description : TCP连接相关实现
 */
 
#ifndef CTCPCONNECT_H
#define CTCPCONNECT_H

#include "CSocket.h"


struct in_addr;

namespace NConnect
{

class CTcpConnect : public CSocket
{
public:
	CTcpConnect(const char* ip, const int port);
	~CTcpConnect();
	
public:
    int create(int listenNum);
    void destroy();
	bool isNormal();
	
public:
	int accept(int& fd, in_addr& peerIp, unsigned short& peerPort, int& errorCode);
	
private:
    bool m_isNormal;
};

}

#endif // CTCPCONNECT_H
