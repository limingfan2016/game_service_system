
/* author : limingfan
 * date : 2014.11.07
 * description : 多线程写，单线程读队列，读免锁
 */
 
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "CMultiW_SingleR_Queue.h"
#include "MacroDefine.h"
#include "ErrorCode.h"
#include "CLock.h"


using namespace NErrorCode;

namespace NCommon
{

CMultiW_SingleR_Queue::CMultiW_SingleR_Queue(int size)
{
	m_size = size;
	NEW_ARRAY(m_pBuff, char*[size]);
	memset(m_pBuff, 0, sizeof(char*) * size);
    m_write = 1;  // 开始写点
	m_read = 0;   // 开始读点
	m_isWaitNotify = 0;
}

CMultiW_SingleR_Queue::~CMultiW_SingleR_Queue()
{
	m_size = 0;
	DELETE_ARRAY(m_pBuff);
	m_pBuff = NULL;
    m_write = 1;
	m_read = 0;
	m_isWaitNotify = 0;
}

int CMultiW_SingleR_Queue::waitNotify(int ms)
{
	int rc = 0;
	if (ms <= 0)
	{
        rc = m_notifyCond.wait(m_notifyMutex);	
	}
	else
	{
		struct timespec abstime;
		struct timeval now;
		(void)gettimeofday(&now, NULL);
		long nsec = now.tv_usec * 1000 + (ms % 1000) * 1000000;  // 转换为纳秒
        abstime.tv_nsec = nsec % 1000000000;  // 剩余纳秒
        abstime.tv_sec = now.tv_sec + (nsec / 1000000000) + (ms / 1000);  // 转换为当前秒
		rc = m_notifyCond.timedWait(m_notifyMutex, &abstime);
	}
	
	return rc;
}

void CMultiW_SingleR_Queue::notify()
{
	// !!!这里必须先加锁，否则存在唤醒事件丢失的情况（不加锁则可能唤醒事件提前触发，导致等待线程死等）
	m_notifyMutex.lock();
    m_notifyCond.signal();
	m_notifyMutex.unLock();
}

int CMultiW_SingleR_Queue::put(char* pData)
{
	int rc = Success;
	if (pData != NULL)
	{
		m_writeMutex.lock();
		if (m_pBuff[m_write] == NULL)  // 读点在并发中不断变化，因此只能判断数据，不能依据读写点判断是否为满队列
		{
			// 理论上不保证读取m_read的值是原子操作，尤其是在多cpu多核的机器上
			// 下一个读点没有数据，则之前队列可能为空，读线程有可能是在等待
			// !!!放入数据之前做判断
			bool isWaitNotify = (m_pBuff[(m_read + 1) % m_size] == NULL);
			
			m_pBuff[m_write] = pData;
			m_write = (m_write + 1) % m_size;    // 下一个写点
			m_writeMutex.unLock();
			
            if (m_isWaitNotify || isWaitNotify)  // 双重判断，更保险一些
		    {
			    notify();
		    }
		}
		else
		{
			m_writeMutex.unLock();
			rc = QueueFull;  // 队列满
		}
	}
	
	return rc;
}

// 队列空时读线程阻塞等待ms毫秒的时间，单位毫秒，默认值0表示永久等待直到被消息入队列唤醒
char* CMultiW_SingleR_Queue::get(int ms)
{
	int nextRead = (m_read + 1) % m_size;
	
	// 写点在并发中不断变化，因此只能判断数据，依据读写点判断是否为空队列不准确，但可以考虑和数据一起做判断
	// 由于并发可能存在put()函数中pBuff[m_write] = pData;语句没有完全赋值完，就被这里读到的情况
	// 可以考虑增加读写点一起做判断
	char* pData = m_pBuff[nextRead];
	if (pData == NULL)  
	{
		m_notifyMutex.lock();     // 锁定等待数据入队列
		m_isWaitNotify = 1;       // 需要消息唤醒通知
		pData = m_pBuff[nextRead];  // !!!这里必须重新再取一次，有可能通知事件已经提前触发了，因此数据已经存在了，否则这种情况下将导致线程死等
	    while (pData == NULL)
		{
		    int rc = waitNotify(ms);
			pData = m_pBuff[nextRead];
			if (ETIMEDOUT == rc)      // 等待超时则退出
			{
				break;
			}
		}
		m_isWaitNotify = 0;
		m_notifyMutex.unLock();
		
		if (pData == NULL)  // 等待超时无数据
		{
			return NULL;
		}
	}
	m_pBuff[nextRead] = NULL;
	m_read = nextRead;
	
	return pData;
}

}

