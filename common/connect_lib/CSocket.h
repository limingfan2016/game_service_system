
/* author : admin
 * date : 2014.11.15
 * description : 网络连接封装实现
 */
 
#ifndef CSOCKET_H
#define CSOCKET_H

#include <netinet/in.h>
#include "base/Type.h"


namespace NConnect
{

class CSocket
{
public:
    CSocket();
	CSocket(const char* ip, const unsigned short port);
	virtual ~CSocket();
	
public:
	static const char* toIPStr(const struct in_addr& ip);
	static unsigned int toIPInt(const char* ip);
	static int toIPInAddr(const char* ipStr, struct in_addr& ipInAddr);
	static int getOpt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
	
public:
	int init(const char* ip, const unsigned short port);
	void unInit();
	
public:
	int open(int type);  // type：tcp或者udp；SOCK_STREAM&SOCK_DGRAM
	int open(int family, int type, int protocol);
	int close();
	
public:
	int bind();
	int listen(int num);
	
public:
	int connect(int& errorCode);
	
public:
    int setLinger(int onOff, int val);  // 慎用该选项
    int setRcvBuff(int val);            // 接受缓冲区大小，在connect&listen操作之前设置
	int setSndBuff(int val);            // 发生缓冲区大小，在connect&listen操作之前设置
	int setReuseAddr(int val);          // 地址重用，bind之前调用
	int setNagle(int val);              // Nagle算法
	int setNoBlock();                   // 设置为非主塞方式
	int setNoBlock(int fd);             // 设置为非主塞方式
	
public:
	int getFd() const;
	const char* getIp() const;
	unsigned short getPort() const;


private:
    // 禁止拷贝、赋值
	CSocket(const CSocket&);
	CSocket& operator =(const CSocket&);
	
protected:
    strIP_t m_ip;
    unsigned short m_port;
	int m_fd;
};

}

#endif // CSOCKET_H
