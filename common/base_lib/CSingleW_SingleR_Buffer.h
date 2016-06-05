
/* author : limingfan
 * date : 2014.11.27
 * description : 单线程写，单线程读字节码流缓冲区，读写免锁
 */
 
#ifndef CSINGLEW_SINGLER_BUFFER_H
#define CSINGLEW_SINGLER_BUFFER_H

#include <string.h>
#include "MacroDefine.h"


namespace NCommon
{

// 单线程写，单线程读字节码流缓冲区，读写免锁 
// 读写数据缓冲区，注意字节对齐
struct CSingleW_SingleR_Buffer
{
	CSingleW_SingleR_Buffer* next;  // 下一块可读写的buff
    char* buff;                     // 缓冲区开始地址
	unsigned int buffSize;          // 缓冲区buff的大小
	
	// 读写索引
	volatile unsigned int readIdx;
	volatile unsigned int writeIdx;
	
	// 统计数据
	// unsigned int msgCount;
	// unsigned int msgSize;
	
	// 直接获取写缓冲区
	// 写数据时必须先写第一块数据区，写完了才能继续写第二块数据区，否则会直接导致错误
	inline bool beginWriteBuff(char*& firstBuff, unsigned int& firstLen, char*& secondBuff, unsigned int& secondLen) const
	{
		// 一次性拷贝读写索引，防止读写线程并发修改发生变化而导致错误
		unsigned int rdIdx = readIdx;
		unsigned int wtIdx = writeIdx;

        // 注意：rdIdx + 1 == wtIdx 则队列满，因此需要留出一个字节的空位
		// 否则存在当读线程读数据的时候，如果写线程正好把数据写满导致 rdIdx == wtIdx，会使得读线程误判断为空队列错误
		secondBuff = NULL;
		secondLen = 0;
		if (wtIdx < rdIdx)
		{
			firstBuff = buff + wtIdx;
			firstLen = rdIdx - wtIdx - 1;  // 留出一个字节的空位
		}
		else
		{
			// rdIdx == wtIdx 情况则队列空
			firstBuff = buff + wtIdx;
			firstLen = buffSize - wtIdx;
			if (firstLen < 1 && rdIdx > 0)
			{
				firstBuff = buff;  // 调整到队列头部
				firstLen = rdIdx - 1;  // 留出一个字节的空位
			}
			else if (rdIdx > 0)
			{
				secondBuff = buff;
				secondLen = rdIdx - 1;  // 留出一个字节的空位
			}
		}

		return (firstLen > 0);  // 有可能队列数据满了		
	}
	
	// 结束写入数据，必须配对beginWriteBuff调用
	inline void endWriteBuff(char* firstBuff, unsigned int firstLen, char* secondBuff, unsigned int secondLen)
	{
		if (secondLen > 0)
		{
			writeIdx = secondLen;
		}
		else if (firstLen > 0)
		{
			writeIdx = firstBuff - buff + firstLen;
		}
	}
	
	
	// 直接获取读缓冲区
	// 读数据时必须先读第一块数据区，读完了才能继续读第二块数据区，否则会直接导致错误
	inline bool beginReadBuff(char*& firstBuff, unsigned int& firstLen, char*& secondBuff, unsigned int& secondLen) const
	{
		// 一次性拷贝读写索引，防止读写线程并发修改发生变化而导致错误
		unsigned int rdIdx = readIdx;
		unsigned int wtIdx = writeIdx;
		
		firstBuff = NULL;
		firstLen = 0;
		secondBuff = NULL;
		secondLen = 0;		
		if (rdIdx < wtIdx)
		{
			firstBuff = buff + rdIdx;
			firstLen = wtIdx - rdIdx;			
		}
		else if (rdIdx > wtIdx)
		{		    
			firstBuff = buff + rdIdx;
			firstLen = buffSize - rdIdx;
			if (firstLen < 1 && wtIdx > 0)
			{
				firstBuff = buff;  // 调整到队列头部
				firstLen = wtIdx;
			}
			else if (wtIdx > 0)
			{
				secondBuff = buff;
				secondLen = wtIdx;
			}
		}  // rdIdx == wtIdx 情况则队列空
		
		return (firstLen > 0);  // 有可能队列数据是空的
	}
	
	// 结束读取数据，必须配对beginReadBuff调用
	inline void endReadBuff(char* firstBuff, unsigned int firstLen, char* secondBuff, unsigned int secondLen)
	{
		if (secondLen > 0)
		{
			readIdx = secondLen;
		}
		else if (firstLen > 0)
		{
			readIdx = firstBuff - buff + firstLen;
		}
	}
	
	// 写入缓冲区，返回实际写入的数据长度
	// incompleteWrite : 缓冲区不够写时，是否强制写入，即不完全拷贝数据到写缓冲区
	inline unsigned int write(const char* data, const unsigned int len, bool incompleteWrite = true)
	{
		unsigned int freeSize = getFreeSize();
		if (freeSize == 0 || (!incompleteWrite && freeSize < len))
		{
			return 0;
		}
		
		char* firstBuff = NULL;
		unsigned int firstLen = 0;
		char* secondBuff = NULL;
		unsigned int secondLen = 0;
		if (!beginWriteBuff(firstBuff, firstLen, secondBuff, secondLen))
		{
			return 0;
		}
		
		unsigned int cpLen = len < firstLen ? len : firstLen;
		memcpy(firstBuff, data, cpLen);
		endWriteBuff(firstBuff, cpLen, NULL, 0);
		
		if (cpLen < len && secondLen > 0)
		{
			unsigned int secondCpLen = len - cpLen;
			secondCpLen = secondCpLen < secondLen ? secondCpLen : secondLen;
			memcpy(secondBuff, data + cpLen, secondCpLen);
		    endWriteBuff(NULL, 0, secondBuff, secondCpLen);
			cpLen += secondCpLen;
		}
		
		return cpLen;
	}
	
	// 从缓冲区读，返回实际读出的数据长度
	// isCopy : 为false时不会拷贝数据到调用者空间，相当于直接丢弃数据
	// incompleteRead : 缓冲区数据不够读时，是否强制读出，即不完全拷贝读出数据到用户空间
	inline unsigned int read(char* data, const unsigned int len, bool isCopy = true, bool incompleteRead = true)
	{
		unsigned int dataSize = getDataSize();
		if (dataSize == 0 || (!incompleteRead && dataSize < len))
		{
			return 0;
		}
		
		char* firstBuff = NULL;
		unsigned int firstLen = 0;
		char* secondBuff = NULL;
		unsigned int secondLen = 0;
		if (!beginReadBuff(firstBuff, firstLen, secondBuff, secondLen))
		{
			return 0;
		}
		
		unsigned int cpLen = len < firstLen ? len : firstLen;
		if (isCopy) memcpy(data, firstBuff, cpLen);
		endReadBuff(firstBuff, cpLen, NULL, 0);

		if (cpLen < len && secondLen > 0)
		{
			unsigned int secondCpLen = len - cpLen;
			secondCpLen = secondCpLen < secondLen ? secondCpLen : secondLen;
			if (isCopy) memcpy(data + cpLen, secondBuff, secondCpLen);
		    endReadBuff(NULL, 0, secondBuff, secondCpLen);
			cpLen += secondCpLen;
		}
		
		return cpLen;
	}
	
	inline unsigned int getDataSize() const
	{
		// 一次性拷贝读写索引，防止读写线程并发修改发生变化而导致错误
		unsigned int rdIdx = readIdx;
		unsigned int wtIdx = writeIdx;
        return (rdIdx <= wtIdx) ? (wtIdx - rdIdx) : (buffSize - rdIdx + wtIdx);
	}
	
	inline unsigned int getFreeSize() const
	{
		// 一次性拷贝读写索引，防止读写线程并发修改发生变化而导致错误
		unsigned int rdIdx = readIdx;
		unsigned int wtIdx = writeIdx;
		
		// 注意：buffSize == wtIdx && rdIdx 为 0 的情况
		unsigned int freeSize = (wtIdx < rdIdx) ? (rdIdx - wtIdx) : (buffSize - wtIdx + rdIdx);
		return (freeSize > 0) ? (freeSize - 1) : 0;  // 留出一个字节的空位，用于判断队列是否满了
	}
	
	inline bool isEmpty() const
	{
		// 一次性拷贝读写索引，防止读写线程并发修改发生变化而导致错误
		unsigned int rdIdx = readIdx;
		unsigned int wtIdx = writeIdx;
		return (rdIdx == wtIdx) || (wtIdx == 0 && rdIdx == buffSize);
	}
	
	inline bool isFull() const
	{
		return getFreeSize() == 0;
	}
};

}

#endif // CSINGLEW_SINGLER_BUFFER_H
