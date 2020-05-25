
/* author : admin
 * date : 2014.11.17
 * description : 共享内存操作，共享内存管理进程间通信实现
 */
 
#ifndef CSHM_H
#define CSHM_H

#include "MacroDefine.h"
#include "Type.h"


namespace NCommon
{

// 共享内存操作，公共API
class CSharedMemory
{
public:
    static int getKey(const char* keyFile);                                                  // 利用文件node id做为共享内存key，保证key唯一性
	static int lockFile(const char* keyFile, int lockPos, int& fd);                          // 加锁共享内存对应的key文件，防止进程重复初始化共享内存
	static int unLockFile(int fd, int lockPos);                                              // 解锁共享内存key对应的文件
	
public:
	static int create(int key, ulong64_t size, int flag, int& isCreate, int& shmId, char*& pShm);  // 进程向操作系统申请共享内存空间
	static int atShm(int key, int& shmId, char*& pShm);                                      // 挂接共享内存
	static int release(void*& shm);                                                          // 进程脱离共享内存
	
    DISABLE_CLASS_BASE_FUNC(CSharedMemory);  // 禁止实例化对象
};



static const int SIDE_COUNT = 2;  // 通信两端

// 共享内存头
struct ShmHeader
{
	unsigned int processLogicId[SIDE_COUNT];      // 通信两端的进程逻辑ID
	PkgQueue queue[SIDE_COUNT];                   // 收发数据队列
};

// 本进程内共享内存信息
struct ShmInfo
{
	int id;                      // 共享内存id值
	int key;                     // 共享内存key值
	ShmHeader* shmHeader;        // 共享内存头部地址
	char* queue[SIDE_COUNT];     // 共享内存中收发数据队列开始地址
	int writeQueue;              // 发数据队列索引
	int readQueue;               // 收数据队列索引 
};

// 两两进程间共享内存通信
class CShm
{
public:
	CShm();
	~CShm();
	
public:
    // 只读打开共享内存，以便跟踪共享内存统计数据信息
    int trace(const unsigned int srcId, const unsigned int dstId, const ulong64_t size);
	
    // 打开共享内存，不存在则创建
	// srcId为本进程逻辑ID，dstId为通信对端进程逻辑ID
	// size为共享内存大小，实际会分配2倍size大小的共享内存，分别为读写数据2条队列
	// 每个进程只能调用open一次，多次操作将返回失败
	// 在进程1打开一个和进程2通信的共享内存：open(1, 2, 10240 * 1024)
	int open(const unsigned int srcId, const unsigned int dstId, const ulong64_t size);
	
	// 此处不会调用shmctl删除共享内存，否则其他进程将映射不到该共享内存空间
	// 共享内存为双方通信进程共用，因此需要外部确认而手动删除
	int close();
	
	// 输出共享内存信息到运行日志
	void output();

	// 共享内存双队列，通信双方都是单读单写模式，因此读写都不加锁
	// 但理论上是需要加共享锁的，除非对int类型的读写操作是原子的
public:
    // 从共享内存区输入&输出数据，会产生一次数据拷贝过程，必须配对使用
	int write(const char* pData, const int len);
	int read(char* pData, int& rLen);
	
	// 直接读写共享内存区，无数据拷贝过程，必须配对使用
public:
	// 获取共享内存区buff，用于直接写入数据到该buff中
	int beginWriteBuff(char*& pBuff, const int len);        // len：申请buff的长度
	int endWriteBuff(const char* pBuff, const int len);     // len：实际写入的数据长度
    
	// 获取共享内存区buff，用于直接读取该buff中的数据
	int beginReadBuff(char*& pBuff, int& len);              // len：buff中数据包长度
	int endReadBuff(const char* pBuff, const int len);      // len：buff数据长度
	
private:
    // 打开共享内存
	// srcId为本进程逻辑ID，dstId为通信对端进程逻辑ID
	// size为共享内存大小，实际会分配2倍size大小的共享内存，分别为2读写数据队列
	// isCreate如果为true则不存在就创建共享内存
	// 每个进程只能调用open一次，多次操作将返回失败
	// 在进程1打开一个和进程2通信的共享内存：open(1, 2, 10240 * 1024)
	int open(const unsigned int srcId, const unsigned int dstId, const ulong64_t size, const int isCreate);
	
private:
    // 禁止拷贝、赋值
    CShm(const CShm&);
	CShm& operator =(const CShm&);
	
private:
	ShmInfo m_shmInfo;
};

}

#endif // CSHM_H
