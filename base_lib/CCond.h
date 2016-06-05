
/* author : limingfan
 * date : 2014.10.21
 * description : 线程同步通知API封装
 */
 
#ifndef CCOND_H
#define CCOND_H

#include <pthread.h>


namespace NCommon
{
class CMutex;

class CCond
{
public:
	CCond();
	~CCond();
	
public:
	int wait(CMutex& mutex);
	int timedWait(CMutex& mutex, const struct timespec* time);
	int signal();
	int broadCast();

private:
    // 禁止拷贝、赋值
    CCond(const CCond&);
    CCond& operator =(const CCond&);

private:
    pthread_condattr_t m_condAttr;  // 为了支持以后好扩展使用
	pthread_cond_t m_cond;
};

}

#endif // CCOND_H


