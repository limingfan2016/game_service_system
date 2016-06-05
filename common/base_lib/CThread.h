
/* author : limingfan
 * date : 2014.10.21
 * description : 线程管理API封装
 */
 
#ifndef CTHREAD_H
#define CTHREAD_H

#include <pthread.h>


namespace NCommon
{

class CThread
{
public:
	CThread();
	virtual ~CThread();

public:
    // 线程管理api
	static int detach(pthread_t pid);  // 分离线程
	static int join(pthread_t pid, void** retVal);    // 等待线程结束
	static int cancel(pthread_t pid);    // 取消停止线程
	

public:
	int start();         // 创建线程并执行run函数调用
	
	// 以下函数只能在线程运行函数run内调用
	int yield();         // 挂起线程，只能在线程内主动调用
	int detach();        // 线程主动分离自己，必须在run函数调用才能生效
	void stop();         // 停止线程，只能在线程内主动调用
	
public:
	pthread_t getId();   // 线程id
	virtual const char* getName();  // 线程名称

private:
    // 禁止拷贝、赋值
    CThread(const CThread&);
    CThread& operator =(const CThread&);

private:
	virtual void run();  // 线程实现者重写run
	
private:
    pthread_t m_pId;


    friend void* runThreadFunc(void* pObj);
};

}

#endif // CTHREAD_H



