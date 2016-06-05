
/* author : limingfan
 * date : 2014.10.20
 * description : 内存块内存池快速高效的获取访问，防止内存频繁操作导致内存碎片
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CMemPool.h"
#include "MacroDefine.h"


namespace NCommon
{
typedef unsigned long pointer_t;               // 32&64 位系统指针占用内存的长度都和 unsigned long 类型占用的长度相等
static const int AddrSize = sizeof(char*);     // 指针占用内存的长度

// 内存池实现
CMemPool::CMemPool(const unsigned int count, const unsigned int size) : m_maxCount(count), m_buffSize(size), m_freeCount(0)
{
	m_pBuff = NULL;
	m_pHead = NULL;
	m_pLast = NULL;
	if (count <= 1 || size == 0)
	{
		return;
	}
	
	NEW_ARRAY(m_pBuff, char[(size + AddrSize) * count]);  // 块buff的实际大小 (size + AddrSize) * count = 总内存大小
}

CMemPool::~CMemPool()
{
	m_freeCount = 0;
	m_pHead = NULL;
	m_pLast = NULL;
	DELETE_ARRAY(m_pBuff);
}

bool CMemPool::init(void* data)
{
	// 切片串成链表
	if (m_pBuff != NULL)
	{
		const unsigned int count = m_maxCount;
	    const unsigned int len = m_buffSize + AddrSize;  // 块buff的实际大小
	    const unsigned int max = len * count;            // 总内存大小
	
		memset(m_pBuff, 0, sizeof(char) * max);
		m_pLast = m_pBuff + len * (count - 1);

        char* pCur = NULL;
		char* pNext = NULL;		
		for (unsigned int i = 0; i < count - 1; i++)
		{
			pCur = m_pBuff + len * i;              // 当前buff块
			pNext = m_pBuff + len * (i + 1);       // 下一块buff的起始地址
			*(pointer_t*)pCur = (pointer_t)pNext;  // 存储在前一块buff的头部
			*(pointer_t*)(pCur + AddrSize) = (pointer_t)data;  // 在头部存储用户数据
		}
		*(pointer_t*)(pNext + AddrSize) = (pointer_t)data;     // 在头部存储用户数据，最后一个内存块

		m_pHead = m_pBuff;
		m_freeCount = count;
		return true;
	}
	
	return false;
}

char* CMemPool::get()
{
	char* pRet = NULL;
	if (!empty())
	{
		m_freeCount--;
		pRet = (m_pHead + AddrSize);       // 用户buff的地址
		m_pHead = (char*)(*(pointer_t*)m_pHead);  // 下一块buff的地址
	}
	return pRet;
}

bool CMemPool::put(char* p)
{
	if (p != NULL)
	{
		p -= AddrSize;
		if (p < m_pBuff || p > m_pLast)  // 地址范围判断
		{
			return false;
		}

		if (m_pHead != NULL)
		{
			*(pointer_t*)p = (pointer_t)m_pHead;
		}
		else
		{
			*(pointer_t*)p = 0;
		}
		m_pHead = p;
		m_freeCount++;
	}
	
	return true;
}

bool CMemPool::empty() const
{
	return m_freeCount == 0;
}

bool CMemPool::full() const
{
	return m_freeCount == m_maxCount;
}

unsigned int CMemPool::getFreeCount() const
{
	return m_freeCount;
}

unsigned int CMemPool::getMaxCount() const
{
	return m_maxCount;
}

unsigned int CMemPool::getBuffSize() const
{
	return m_buffSize;
}


// only for test code
void CMemPool::output()
{
	char* p = m_pHead;
	while (p != NULL)
	{
		printf("--> %ld", (pointer_t)p);
		p = (char*)(*(pointer_t*)p);
	}
	printf("\n");
}

}



