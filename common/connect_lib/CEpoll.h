
/* author : admin
 * date : 2014.11.30
 * description : epoll模型API封装实现
 */
 
#ifndef CEPOLL_H
#define CEPOLL_H


struct epoll_event;

namespace NConnect
{

class CEpoll
{
public:
	CEpoll();
	~CEpoll();
	
public:
	int create(int listenCount);
	void destroy();
	int listen(struct epoll_event* waitEvents, const int maxEvents, const int waitTimeout);
	int addListener(int fd, struct epoll_event& optEv);
	int modifyListenEvent(int fd, struct epoll_event& optEv);
	int removeListener(int fd);

private:
    // 禁止拷贝、赋值
	CEpoll(const CEpoll&);
	CEpoll& operator =(const CEpoll&);
	
private:
    int m_epfd;

};

}

#endif // CEPOLL_H
