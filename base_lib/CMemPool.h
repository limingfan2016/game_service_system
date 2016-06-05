
/* author : limingfan
 * date : 2014.10.20
 * description : 内存块内存池快速高效的获取访问，防止内存频繁操作导致内存碎片
 */
 
#ifndef CMEM_POOL_H
#define CMEM_POOL_H

namespace NCommon
{
// 内存池实现
class CMemPool
{
public:
    // 创建count块内存，每块内存size大小的字节
	CMemPool(const unsigned int count, const unsigned int size);
	bool init(void* data = 0);
	
	~CMemPool();

public:
    // 获取内存&释放内存
	char* get();
	bool put(char* p);

public:
	bool empty() const;
	bool full() const;
	unsigned int getFreeCount() const;
	unsigned int getMaxCount() const;
	unsigned int getBuffSize() const;
	
private:
    // 禁止拷贝、赋值
	CMemPool();
	CMemPool(const CMemPool&);
	CMemPool& operator =(const CMemPool&);

private:
	const unsigned int m_maxCount;
	const unsigned int m_buffSize;

private:
	char* m_pBuff;
	char* m_pHead;
	char* m_pLast;
	unsigned int m_freeCount;
	
	
	// only for test code
public:
	void output();
};

}

#endif  // CMEM_POOL_H

