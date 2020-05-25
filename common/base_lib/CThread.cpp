
/* author : admin
 * date : 2014.10.21
 * description : 线程管理API封装
 */
 
#include <string.h>
#include "CThread.h"
#include "MacroDefine.h"


namespace NCommon
{

void* runThreadFunc(void* pObj)
{
	CThread* pThread = (CThread*)pObj;
	pThread->run();
	return NULL;
}


CThread::CThread() : m_pId(0)
{
}

CThread::~CThread()
{
	m_pId = 0;
}


int CThread::detach(pthread_t pid)
{
	int rc = pthread_detach(pid);
	if (rc != 0)
	{
		ReleaseErrorLog("call pthread_detach pid = %ld, error = %d, info = %s", pid, rc, strerror(rc));  // 运行日志
	}
	return rc;
}

int CThread::join(pthread_t pid, void** retVal)
{
	int rc = pthread_join(pid, retVal);
	if (rc != 0)
	{
		ReleaseErrorLog("call pthread_join pid = %ld, error = %d, info = %s", pid, rc, strerror(rc));  // 运行日志
	}
	return rc;
}

int CThread::cancel(pthread_t pid)
{
	int rc = pthread_cancel(pid);
	if (rc != 0)
	{
		ReleaseErrorLog("call pthread_cancel pid = %ld, error = %d, info = %s", pid, rc, strerror(rc));  // 运行日志
	}
	return rc;
}


int CThread::start()
{
	int rc = pthread_create(&m_pId, NULL, runThreadFunc, (void*)this);
	if (rc != 0)
	{
		ReleaseErrorLog("call pthread_create error = %d, info = %s", rc, strerror(rc));  // 运行日志
	}
	return rc;
}

int CThread::yield()
{
	return pthread_yield();
}

void CThread::stop()
{
	pthread_exit(NULL);
}

// 线程主动分离自己，必须在run函数调用才能生效
int CThread::detach()
{
	int rc = pthread_detach(pthread_self());
	if (rc != 0)
	{
		ReleaseErrorLog("call pthread_detach error = %d, info = %s", rc, strerror(rc));  // 运行日志
	}
	return rc;
}
	
pthread_t CThread::getId()
{
	return m_pId;
}

const char* CThread::getName()
{
	return "";
}

void CThread::run()
{
}
	
}

