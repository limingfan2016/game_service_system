
/* author : limingfan
 * date : 2014.11.17
 * description : 消息队列操作，单线程读&单线程写免锁
 */
 
#include <string.h>

#include "CMessageQueue.h"
#include "ErrorCode.h"


using namespace NErrorCode;

namespace NCommon
{

static const int PkgHeaderSize = sizeof(PkgHeader);  // 数据包头长度


// 输入&输出数据，会产生一次数据拷贝过程
int CMessageQueue::write(char* pShmWrite, PkgQueue& shmQueue, const char* pData, const int len)
{
	// 参数校验
	if (pData == NULL || len <= 0)
	{
		return RWInvalidParam;
	}
	
	int curWrite = 0;  // buff当前写点	
	int needLen = PkgHeaderSize + len;
	int rc = adjustWriteIdx(shmQueue, needLen, pShmWrite, curWrite);
	if (rc != Success)
	{
		return rc;
	}
	
	// 拷贝数据入队列
	pShmWrite += curWrite;
	((PkgHeader*)pShmWrite)->flag = USER_DATA;
	((PkgHeader*)pShmWrite)->len = len;             // 用户数据包长度
	memcpy(pShmWrite + PkgHeaderSize, pData, len);  // 用户数据包数据流
	shmQueue.write = curWrite + needLen;            // 调整队列写点索引
	shmQueue.record.writeCount++;
	shmQueue.record.writeSize += len;
	
	return Success;
}

int CMessageQueue::read(char* pShmRead, PkgQueue& shmQueue, char* pData, int& rLen)
{
	// 参数校验
	if (pData == NULL || rLen <= 0)
	{
		return RWInvalidParam;
	}
	
	int curRead = 0;  // buff当前读点
	int rc = adjustReadIdx(shmQueue, pShmRead, curRead);
	if (rc != Success)
	{
		return rc;
	}
	
	// 拷贝数据到用户缓冲区中
	pShmRead += curRead;
	int needLen = ((PkgHeader*)pShmRead)->len;
	if (needLen > rLen)
	{
		return ReadPkgBuffSmall;
	}
	memcpy(pData, pShmRead + PkgHeaderSize, needLen);  // 用户数据包数据流
	rLen = needLen;
	shmQueue.read = curRead + PkgHeaderSize + needLen;            // 调整队列写点索引
	shmQueue.record.readCount++;
	shmQueue.record.readSize += needLen;
	
	return Success;
}

// 查看消息队列是否可写消息
int CMessageQueue::readyWrite(char* pShmWrite, const PkgQueue& shmQueue, const int len)
{
	if (len < 1) return RWInvalidParam;

	int curWrite = 0;
	return adjustWriteIdx(shmQueue, PkgHeaderSize + len, pShmWrite, curWrite);
}

// 查看消息队列是否可读消息
int CMessageQueue::readyRead(char* pShmRead, const PkgQueue& shmQueue)
{
	int curRead = 0;
	return adjustReadIdx(shmQueue, pShmRead, curRead);
}


// 直接读写buff，无数据拷贝过程
// 获取buff，用于直接写入数据到该buff中
int CMessageQueue::beginWriteBuff(char* pShmWrite, const PkgQueue& shmQueue, char*& pBuff, const int len)
{
	// 参数校验
	if (len <= 0)
	{
		return RWInvalidParam;
	}

	int curWrite = 0;  // buff当前写点	
	int needLen = PkgHeaderSize + len;
	int rc = adjustWriteIdx(shmQueue, needLen, pShmWrite, curWrite);
	if (rc != Success)
	{
		return rc;
	}
	pBuff = pShmWrite + curWrite + PkgHeaderSize;   // 数据写缓冲区开始地址
	
	return Success;
}

int CMessageQueue::endWriteBuff(char* pShmWrite, PkgQueue& shmQueue, const char* pBuff, const int len)
{
	// 参数校验
	if (pBuff == NULL || len <= 0)
	{
		return RWInvalidParam;
	}
	
	PkgHeader* pkgHeader = (PkgHeader*)(pBuff - PkgHeaderSize);
	pkgHeader->flag = USER_DATA;
	pkgHeader->len = len;             // 用户数据包长度
	shmQueue.write = pBuff - pShmWrite + len;            // 调整队列写点索引
	shmQueue.record.writeCount++;
	shmQueue.record.writeSize += len;
	
	return Success;
}

// 直接读写buff，无数据拷贝过程
// 获取buff，用于直接读取该buff中的数据
int CMessageQueue::beginReadBuff(char* pShmRead, const PkgQueue& shmQueue, char*& pBuff, int& len)
{
	int curRead = 0;  // buff当前读点
	int rc = adjustReadIdx(shmQueue, pShmRead, curRead);
	if (rc != Success)
	{
		return rc;
	}
	
	pBuff = pShmRead + curRead + PkgHeaderSize;      // 数据包开始地址
	len = ((PkgHeader*)(pShmRead + curRead))->len;   // 数据包长度
	
	return Success;
}

int CMessageQueue::endReadBuff(char* pShmRead, PkgQueue& shmQueue, const char* pBuff, const int len)
{
	// 参数校验
	if (pBuff == NULL || len <= 0)
	{
		return RWInvalidParam;
	}

	shmQueue.read = pBuff - pShmRead + len;
	shmQueue.record.readCount++;
	shmQueue.record.readSize += len;
	
	return Success;
}

// 调整队列写点索引
int CMessageQueue::adjustWriteIdx(const PkgQueue& queue, const int needLen, char* pWrite, int& curWrite)
{
	// 读写点拷贝出来，防止冲突
	curWrite = queue.write;  // buff当前写点
	const int curRead = queue.read;    // buff当前读点	
	
	// 注意：curRead + 1 == curWrite 则队列满，因此需要留出一个字节的空位
	// 否则存在当读线程读数据的时候，如果写线程正好把数据写满导致 curRead == curWrite，会使得读线程误判断为空队列错误
	int remainLen = 0;  // buff剩余容量
	if (curWrite < curRead)                  // queue: begin----->write----->read-----end
	{
		remainLen = curRead - curWrite - 1;  // 留出一个字节的空位
		if (remainLen < needLen)             // queue: begin----->write->read---------end  // 中间处剩余空间不足
		{
			return PkgQueueFull;
		}
	}
	else
	{
		// 1) queue: begin----->read----->write-----end
		// 2) queue: begin->write->read-------------end  读写点相同则为空队列
		remainLen = queue.size - curWrite;
		if (remainLen < needLen)             // queue: begin------->read------>write->end  队列终止处剩余空间不足
		{
			// 注意留出一个字节的空位
			if ((curRead - 1) < needLen)    // queue: begin->read------------>write->end  队列开始处剩余空间不足
			{
				return PkgQueueFull;
			}
			
			if (remainLen >= PkgHeaderSize)  // queue: begin->read---------->write--->end  队列终止处存放控制包，标记数据结束
			{
				((PkgHeader*)(pWrite + curWrite))->flag = BUFF_END;
			}
			curWrite = 0;  // buff最后剩余空间不足，这个时候调整写点，从头开始
		}
	}
	
	return Success;
}

// 调整队列读点索引
int CMessageQueue::adjustReadIdx(const PkgQueue& queue, const char* pRead, int& curRead)
{
	// 读写点拷贝出来，防止冲突
	curRead = queue.read;  // buff当前读点
	const int curWrite = queue.write;    // buff当前写点
	
	int remainLen = 0;  // 剩余需要读取的数据量
	if (curWrite < curRead)                  // queue: begin----->write----->read-----end
	{
		remainLen = queue.size - curRead;
		if (remainLen < PkgHeaderSize)       // 剩余数据不足一个数据包头部
		{
			if (curWrite < PkgHeaderSize)    // queue: begin->write------------>read->end  队列开始处还没有数据
			{
				return PkgQueueEmpty;
			}
			curRead = 0;  // 此时调整读索引到队列最开始处
		}
        else                                 // 剩余数据至少存有一个数据包头部
		{
			const PkgHeader* pkgHeader = (PkgHeader*)(pRead + curRead); // queue: begin->write----------->read-->end  队列的数据包头部
			if (pkgHeader->flag == BUFF_END)  // 队列数据终止了
			{
				if (curWrite < PkgHeaderSize)  // queue: begin->write------------>read->end  队列开始处还没有数据
				{
					return PkgQueueEmpty;
				}
				curRead = 0;  // 此时调整读索引到队列最开始处
			}
		}
	}
	else if (curWrite > curRead)             // queue: begin----->read----->write-----end
	{
		remainLen = curWrite - curRead;
		if (remainLen < PkgHeaderSize)       // 准备读取的数据不足一个数据包头部，数据冲突错误了。。。
		{
			return ReadPkgConflict;
		}
	}
	else                                     // queue: begin->write->read-------------end  读写点相同则为空队列
	{
		return PkgQueueEmpty;
	}
	
	return Success;
}




// 指针消息队列，公共API
CAddressQueue::CAddressQueue() : m_pBuff(NULL)
{
	resetSize(0);
}

CAddressQueue::CAddressQueue(unsigned int size) : m_pBuff(NULL)
{
	resetSize(size);
}

CAddressQueue::~CAddressQueue()
{
	DELETE_ARRAY(m_pBuff);
	m_size = 0;
    m_write = 0;
	m_read = 0;
}

void CAddressQueue::resetSize(unsigned int size)
{
	if (size < 1) size = 1;
	
	m_size = size;
	DELETE_ARRAY(m_pBuff);
	NEW_ARRAY(m_pBuff, void*[m_size]);
	memset(m_pBuff, 0, sizeof(void*) * m_size);
	m_write = 0;
	m_read = 0;
}

unsigned int CAddressQueue::getSize()
{
	return m_size;
}
	
bool CAddressQueue::put(void* pData)
{
	if (pData != NULL && m_pBuff[m_write] == NULL)  // 读点在并发中不断变化，因此判断数据，不能依据读写点判断是否为满队列
	{
		m_pBuff[m_write] = pData;
		m_write = (m_write + 1) % m_size;    // 下一个写点
		return true;
	}
	
	return false;
}

void* CAddressQueue::get()
{
	// 写点在并发中不断变化，因此判断数据
	// 由于并发可能存在put()函数中pBuff[m_write] = pData;语句没有完全赋值完，就被这里读到的情况
	void* pData = m_pBuff[m_read];
	if (pData != NULL)  
	{
		m_pBuff[m_read] = NULL;
	    m_read = (m_read + 1) % m_size;
	}
	
	return pData;
}

bool CAddressQueue::have()
{
	return (m_pBuff[m_read] != NULL);
}

// 清空消息
void CAddressQueue::clear()
{
	memset(m_pBuff, 0, sizeof(void*) * m_size);
    m_write = 0;
	m_read = 0;
}

// 检查当前消息队列中存在此消息的个数
unsigned int CAddressQueue::check(void* pCheckData)
{
	unsigned int count = 0;
	unsigned int curRead = m_read;
	void* pData = m_pBuff[curRead];
	while (pData != NULL)
	{
		if (pData == pCheckData) ++count;
		
		curRead = (curRead + 1) % m_size;
		if (curRead == m_read) break;  // 防止队列爆满的情况
		
		pData = m_pBuff[curRead];
	}
	
	return count;
}

}

