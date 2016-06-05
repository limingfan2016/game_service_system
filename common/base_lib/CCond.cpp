
/* author : limingfan
 * date : 2014.10.21
 * description : 线程同步通知API封装
 */
 
#include "CCond.h"
#include "CMutex.h"
#include "MacroDefine.h"


namespace NCommon
{

CCond::CCond()
{
	int rc = pthread_condattr_init(&m_condAttr);
	if (rc != 0)
	{
		ReleaseErrorLog("call pthread_condattr_init error = %d", rc);  // 运行日志
	}
	
	rc = pthread_cond_init(&m_cond, &m_condAttr);
	if (rc != 0)
	{
		ReleaseErrorLog("call pthread_cond_init error = %d", rc);  // 运行日志
	}
}

CCond::~CCond()
{
	pthread_cond_destroy(&m_cond);
	pthread_condattr_destroy(&m_condAttr);
}

int CCond::wait(CMutex& mutex)
{
	return pthread_cond_wait(&m_cond, mutex.get_mutex());
}

int CCond::timedWait(CMutex& mutex, const struct timespec* time)
{
	return pthread_cond_timedwait(&m_cond, mutex.get_mutex(), time);
}

int CCond::signal()
{
	return pthread_cond_signal(&m_cond);
}

int CCond::broadCast()
{
	return pthread_cond_broadcast(&m_cond);
}
	
}

