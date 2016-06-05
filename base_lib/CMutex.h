
/* author : limingfan
 * date : 2014.10.19
 * description : 互斥锁API封装
 */
 
#ifndef CMUTEX_H
#define CMUTEX_H

#include <pthread.h>


namespace NCommon
{

// 锁类型
enum MutexType
{
	defaultMutexType = PTHREAD_MUTEX_DEFAULT,              // 默认属性
	recursiveMutexType = PTHREAD_MUTEX_RECURSIVE,          // 独占递归锁
	recursiveNPMutexType = PTHREAD_MUTEX_RECURSIVE_NP,     // 抢占递归锁
	sharedMutexType = PTHREAD_PROCESS_SHARED,              // 进程间共享锁
};

class CMutex
{
public:
	CMutex(MutexType mutexType = defaultMutexType);
	~CMutex();
	
public:
	int lock();
	int tryLock();
	int unLock();
	pthread_mutex_t* get_mutex();
	
private:
    // 禁止拷贝、赋值
    CMutex(const CMutex&);
    CMutex& operator =(const CMutex&);

private:
	pthread_mutexattr_t m_mutexAttr;  // 为了支持以后好扩展使用
	pthread_mutex_t m_mutex;
};

}

#endif // CMUTEX_H


