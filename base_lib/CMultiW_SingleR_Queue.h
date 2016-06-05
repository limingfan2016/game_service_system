
/* author : limingfan
 * date : 2014.11.07
 * description : 多线程写，单线程读队列，读免锁
 */
 
#ifndef CMULTIW_SINGLER_QUEUE_H
#define CMULTIW_SINGLER_QUEUE_H

#include "CMutex.h"
#include "CCond.h"

namespace NCommon
{

class CMultiW_SingleR_Queue
{
public:
	CMultiW_SingleR_Queue(int size);
	~CMultiW_SingleR_Queue();

public:
    int put(char* pData);
	char* get(int ms = 0);  // 队列空时读线程阻塞等待的时间，单位毫秒，默认值0表示永久等待直到被消息入队列唤醒
	
private:
	int waitNotify(int ms);
	void notify();
	
private:
    // 禁止拷贝、赋值
	CMultiW_SingleR_Queue();
	CMultiW_SingleR_Queue(const CMultiW_SingleR_Queue&);
	CMultiW_SingleR_Queue& operator =(const CMultiW_SingleR_Queue&);
		
private:
	CMutex m_writeMutex;    // 写锁
	CMutex m_notifyMutex;   // 队列空、满等待锁
	CCond m_notifyCond;     // 队列空、满等待条件
	volatile char m_isWaitNotify;    // 读线程是否在等待数据
	
private:
	char** m_pBuff;         // 消息缓冲区
	int m_size;             // 队列容量

private:
    // 读写队列索引
	volatile int m_write;
	volatile int m_read;
};

}

#endif // CMULTIW_SINGLER_QUEUE_H



