
/* author : limingfan
 * date : 2014.11.17
 * description : 消息队列操作，单线程读&单线程写免锁
 */
 
#ifndef CMESSAGEQUEUE_H
#define CMESSAGEQUEUE_H

#include "MacroDefine.h"
#include "Type.h"


namespace NCommon
{

// 码流消息队列操作 ，公共API
class CMessageQueue
{
public:
    // 输入&输出数据，会产生一次数据拷贝过程
	static int write(char* pShmWrite, PkgQueue& shmQueue, const char* pData, const int len);
	static int read(char* pShmRead, PkgQueue& shmQueue, char* pData, int& rLen);
	
public:
	// 获取buff，用于直接写入数据到该buff中
	static int beginWriteBuff(char* pShmWrite, const PkgQueue& shmQueue, char*& pBuff, const int len);        // len：申请buff的长度
	static int endWriteBuff(char* pShmWrite, PkgQueue& shmQueue, const char* pBuff, const int len);           // len：实际写入的数据长度
    
	// 获取buff，用于直接读取该buff中的数据
	static int beginReadBuff(char* pShmRead, const PkgQueue& shmQueue, char*& pBuff, int& len);               // len：buff中数据包长度
	static int endReadBuff(char* pShmRead, PkgQueue& shmQueue, const char* pBuff, const int len);             // len：buff数据长度
	
public:
    // 查看消息队列是否可读写消息
    static int readyWrite(char* pShmWrite, const PkgQueue& shmQueue, const int len);
	static int readyRead(char* pShmRead, const PkgQueue& shmQueue);
	
private:
    // 调整数据队列读写点索引
	static int adjustWriteIdx(const PkgQueue& queue, const int needLen, char* pWrite, int& curWrite);
	static int adjustReadIdx(const PkgQueue& queue, const char* pRead, int& curRead);
	
    DISABLE_CLASS_BASE_FUNC(CMessageQueue);  // 禁止实例化对象
};



// 指针消息队列，公共API
class CAddressQueue
{
public:
    CAddressQueue();
	CAddressQueue(unsigned int size);
	~CAddressQueue();

public:
    void resetSize(unsigned int size);
	unsigned int getSize();
	
public:
    bool put(void* pData);
	void* get();
	bool have();

public:
    void clear();                          // 清空消息
	unsigned int check(void* pCheckData);  // 检查当前消息队列中存在此消息的个数
	
private:
	void** m_pBuff;               // 消息缓冲区
	unsigned int m_size;          // 队列容量

private:
    // 读写队列索引
	volatile unsigned int m_write;
	volatile unsigned int m_read;
	
DISABLE_COPY_ASSIGN(CAddressQueue);
};

}

#endif // CMESSAGEQUEUE_H
