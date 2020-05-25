
/* author : admin
 * date : 2014.11.16
 * description : 自动内存块内存池管理，快速高效的获取、释放内存，防止内存碎片
 */
 
#include "CMemManager.h"
#include "CMemPool.h"
#include "MacroDefine.h"


namespace NCommon
{

static const int AddrSize = sizeof(CMemPool*);     // 指针占用内存的长度

struct MemPoolContainer
{
	CMemPool memPool;
	MemPoolContainer* pNext;

	MemPoolContainer(const unsigned int count, const unsigned int size) : memPool(count, size), pNext(NULL) {}
};


// 第一次分配count块长度为size的内存，用完则自动分配step块内存
// 注意：count&step 的取值至少大于1，否则不会创建内存池
CMemManager::CMemManager(const unsigned int count, const unsigned int step, const unsigned int size) : m_buffSize(size), m_step(0), m_pHeader(NULL)
{
	if (count <= 1 || step <= 1 || size == 0)
	{
		return;
	}
	
	m_step = step;
	createPool(count, size + AddrSize, m_pHeader);
}

CMemManager::~CMemManager()
{
	m_step = 0;
	clear();
}

char* CMemManager::get()
{
	char* pRet = NULL;
	
	MemPoolContainer* pPre = NULL;
	MemPoolContainer* pCur = m_pHeader;
	while (pCur != NULL)
	{
		pRet = pCur->memPool.get();
		if (pRet != NULL)
		{
			pRet += AddrSize;  // 用户buff块的起始地址
			if (pPre != NULL)
			{
				// 把非空内存池调整到链表头部
				pPre->pNext = pCur->pNext;
				pCur->pNext = m_pHeader;
				m_pHeader = pCur;
			}
			break;
		}
		
		pPre = pCur;
		pCur = pCur->pNext;
	}
	
	if (pRet == NULL && m_step > 1)   // 内存池没内存块了，则创建新的内存池
	{
	    MemPoolContainer* pMemPoolContainer = NULL;
	    if (createPool(m_step, m_buffSize + AddrSize, pMemPoolContainer))
		{
			pMemPoolContainer->pNext = m_pHeader;
			m_pHeader = pMemPoolContainer;
			pRet = pMemPoolContainer->memPool.get();
			pRet += AddrSize;  // 用户buff块的起始地址
		}
	}
	
	return pRet;
}

void CMemManager::put(char* p)
{
	if (p != NULL)
	{
		p -= AddrSize;
		CMemPool* mp = (CMemPool*)(*(unsigned long*)p);
		mp->put(p);
	}
}

// 释放空闲的内存池
void CMemManager::free()
{
	MemPoolContainer* pDelete = NULL;
	MemPoolContainer* pPre = NULL;
	MemPoolContainer* pCur = m_pHeader;
	while (pCur != NULL)
	{
		if (pCur->memPool.full())
		{
		    pDelete = pCur;
			if (pPre != NULL)
			{
				pPre->pNext = pCur->pNext;
			}
			else
			{
				m_pHeader = pCur->pNext;
			}
			pCur = pCur->pNext;
		    destroyPool(pDelete);
			continue;
		}
		
		pPre = pCur;
		pCur = pCur->pNext;
	}
}

// 释放所有内存池
void CMemManager::clear()
{
	MemPoolContainer* pDelete = NULL;
	MemPoolContainer* pCur = m_pHeader;
	m_pHeader = NULL;
	while (pCur != NULL)
	{
		pDelete = pCur;
		pCur = pCur->pNext;
		destroyPool(pDelete);
	}
}

unsigned int CMemManager::getBuffSize() const
{
	return m_buffSize;
}

unsigned int CMemManager::getFreeCount() const
{
	unsigned int count = 0;
	MemPoolContainer* pCur = m_pHeader;
	while (pCur != NULL)
	{
		count += pCur->memPool.getFreeCount();
		pCur = pCur->pNext;
	}
	return count;
}

unsigned int CMemManager::getMaxCount() const
{
	unsigned int count = 0;
	MemPoolContainer* pCur = m_pHeader;
	while (pCur != NULL)
	{
		count += pCur->memPool.getMaxCount();
		pCur = pCur->pNext;
	}
	return count;
}

unsigned int CMemManager::getStep() const
{
	return m_step;
}

// 注意：step 的取值至少大于1，否则不会创建内存池
void CMemManager::setStep(unsigned int step)
{
	if (step > 1)
	{
		m_step = step;
	}
}

bool CMemManager::createPool(const unsigned int count, const unsigned int size, MemPoolContainer*& pMemPoolContainer)
{
	bool ok = false;
	NEW(pMemPoolContainer, MemPoolContainer(count, size));
	if (pMemPoolContainer != NULL)
	{
		ok = pMemPoolContainer->memPool.init(&pMemPoolContainer->memPool);
		if (!ok)
		{
			DELETE(pMemPoolContainer);
		}
        else
        {
            CMemMonitor::getInstance().newMemPoolInfo(pMemPoolContainer, count, size + AddrSize);
        }
	}
	return ok;
}

void CMemManager::destroyPool(MemPoolContainer* pMemPoolContainer)
{
    CMemMonitor::getInstance().deleteMemPoolInfo(pMemPoolContainer);

    DELETE(pMemPoolContainer);
}


// only for test code
void CMemManager::output()
{
	MemPoolContainer* pCur = m_pHeader;
	while (pCur != NULL)
	{
		pCur->memPool.output();
		pCur = pCur->pNext;
	}
}

}

