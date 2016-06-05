
/* author : limingfan
 * date : 2014.10.19
 * description : 互斥锁API封装
 */

#include <string.h>

#include "CMutex.h"
#include "MacroDefine.h"


namespace NCommon
{

CMutex::CMutex(MutexType mutexType)
{
	int rc = pthread_mutexattr_init(&m_mutexAttr);
	if (rc != 0)
	{
		ReleaseErrorLog("call pthread_mutexattr_init error = %d, info = %s", rc, strerror(rc));  // 运行日志
	}
	
	else if (mutexType != defaultMutexType)
	{
		rc = pthread_mutexattr_setpshared(&m_mutexAttr, mutexType);
		if (rc != 0)
		{
			ReleaseErrorLog("call pthread_mutexattr_setpshared error = %d, info = %s, mutex type = %d", rc, strerror(rc), mutexType);
		}
	}
	
	rc = pthread_mutex_init(&m_mutex, &m_mutexAttr);
	if (rc != 0)
	{
		ReleaseErrorLog("call pthread_mutex_init error = %d, info = %s", rc, strerror(rc));  // 运行日志
	}
}

CMutex::~CMutex()
{
	pthread_mutex_destroy(&m_mutex);
	pthread_mutexattr_destroy(&m_mutexAttr);
}

int CMutex::lock()
{
	return pthread_mutex_lock(&m_mutex);
}

int CMutex::tryLock()
{
	return pthread_mutex_trylock(&m_mutex);
}

int CMutex::unLock()
{
	return pthread_mutex_unlock(&m_mutex);
}

pthread_mutex_t* CMutex::get_mutex()
{
	return &m_mutex;
}

}


