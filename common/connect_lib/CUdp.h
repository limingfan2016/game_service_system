
/* author : admin
 * date : 2014.11.23
 * description : UDP相关实现
 */
 
#ifndef CUDP_H
#define CUDP_H

#include "CSocket.h"

namespace NConnect
{

class CUdp : public CSocket
{
public:
	CUdp(const char* ip, const int port);
	~CUdp();

public:
	int create(int listenNum);
	void destroy();
	size_t recvFrom(void *buf, size_t len, struct sockaddr *src_addr, socklen_t *addrlen);
	size_t sendTo(const void *buf, size_t len, const struct sockaddr *dest_addr, socklen_t addrlen);

};

}

#endif // CUDP_H
