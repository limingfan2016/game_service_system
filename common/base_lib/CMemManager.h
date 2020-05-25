
/* author : admin
 * date : 2014.11.16
 * description : 自动内存块内存池管理，快速高效的获取、释放内存，防止内存碎片
 */
 
#ifndef CMEMMANAGER_H
#define CMEMMANAGER_H

namespace NCommon
{

struct MemPoolContainer;

// 定长内存池管理
class CMemManager
{
public:
    // 第一次分配count块长度为size的内存，用完则自动分配step块内存
	// 注意：count&step 的取值至少大于1，否则不会创建内存池
	CMemManager(const unsigned int count, const unsigned int step, const unsigned int size);
	~CMemManager();
	
public:
    // 获取内存&释放内存
	char* get();
	void put(char* p);
	
public:
	void free();   // 释放空闲的内存池
	void clear();  // 释放所有内存池
	
public:
    unsigned int getBuffSize() const;
    unsigned int getFreeCount() const;
	unsigned int getMaxCount() const;

	// 注意：step 的取值至少大于1，否则不会创建内存池
	void setStep(unsigned int step);
	unsigned int getStep() const;
	
private:
	bool createPool(const unsigned int count, const unsigned int size, MemPoolContainer*& pMemPoolContainer);
	void destroyPool(MemPoolContainer* pMemPoolContainer);
	
private:
    // 禁止拷贝、赋值
	CMemManager();
	CMemManager(const CMemManager&);
	CMemManager& operator =(const CMemManager&);


private:
	const unsigned int m_buffSize;
	unsigned int m_step;
	MemPoolContainer* m_pHeader;
	
	
	// only for test code
public:
	void output();
};

}

#endif // CMEMMANAGER_H
