
/* author : limingfan
 * date : 2014.11.17
 * description : 共享内存操作，共享内存管理进程间通信实现
 */
 
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>

#include "CShm.h"
#include "ErrorCode.h"
#include "Constant.h"
#include "CMessageQueue.h"


using namespace NErrorCode;

namespace NCommon
{

CShm::CShm()
{
	memset(&m_shmInfo, 0, sizeof(m_shmInfo));
}

CShm::~CShm()
{
	close();
}

// 只读打开共享内存，以便跟踪共享内存统计数据信息
int CShm::trace(const unsigned int srcId, const unsigned int dstId, const int size)
{
	memset(&m_shmInfo, 0, sizeof(m_shmInfo));
	return open(srcId, dstId, size, 0);
}
	
// 打开共享内存，不存在则创建
int CShm::open(const unsigned int srcId, const unsigned int dstId, const int size)
{
	memset(&m_shmInfo, 0, sizeof(m_shmInfo));
	return open(srcId, dstId, size, 1);
}

// 关闭共享内存
int CShm::close()
{
	void* shm = m_shmInfo.shmHeader;
	int rc = CSharedMemory::release(shm);
	memset(&m_shmInfo, 0, sizeof(m_shmInfo));
	return rc;
}

// 输出共享内存信息到运行日志
void CShm::output()
{
	ReleaseInfoLog("--- begin output shm information ---");
	ReleaseInfoLog("shm information, pid = %d, key = 0x%x, shmId = %d, writeQueue = %d, readQueue = %d.",
	                getpid(), m_shmInfo.key, m_shmInfo.id, m_shmInfo.writeQueue, m_shmInfo.readQueue);
					
	// 拷贝出来再输出才准确
	const unsigned int processLogicId[] = {m_shmInfo.shmHeader->processLogicId[0], m_shmInfo.shmHeader->processLogicId[1]};
	ReleaseInfoLog("process logic id1 = %d, id2 = %d.", processLogicId[0], processLogicId[1]);
	
	for (int i = 0; i < SIDE_COUNT; i++)
	{
		const PkgQueue queue = m_shmInfo.shmHeader->queue[i];  // 拷贝输出
		ReleaseInfoLog("data package queue%d size = %d, write = %d, read = %d, writeCount = %d, writeSize = %d, readCount = %d, readSize = %d.",
	                   i, queue.size, queue.write, queue.read,
					   queue.record.writeCount, queue.record.writeSize, queue.record.readCount, queue.record.readSize);
	}
	ReleaseInfoLog("--- end output shm information ---\n");
}


// 打开共享内存
// srcId为本进程逻辑ID，dstId为通信对端进程逻辑ID
// size为共享内存大小，实际会分配2倍size大小的共享内存，分别为2读写数据队列
// isCreate默认如果不存在则创建共享内存
// 每个进程只能调用 open&close 一次，多次操作将返回失败
// 在进程1打开一个和进程2通信的共享内存：open(1, 2, 10240 * 1024)
int CShm::open(const unsigned int srcId, const unsigned int dstId, const int size, const int isCreate)
{
    if (srcId == dstId || size <= 0)  // 参数校验
    {
        ReleaseErrorLog("open shm logic process id equal or size invalid, srcId = %d, dstId = %d, size = %d", srcId, dstId, size);
        return ShmProcIdEqual;
    }
	
    // 共享内存分成收发两队列
    // 根据进程逻辑ID确定对应的收发数据队列
	// 进程逻辑ID按大小升序排列
	const int idx1 = 0;
	const int idx2 = 1;
	int lockPos = 0;
	unsigned int procId[] = {srcId, dstId};      // 通信两端的进程逻辑ID
    if (srcId > dstId)
	{
		lockPos = 1;
		procId[idx1] = dstId;
		procId[idx2] = srcId;
	}
    
    // 从key文件节点数据获取共享内存key值
	const int keyFileFullLen = MaxFullLen * SIDE_COUNT;
    char shmKeyFile[keyFileFullLen] = {0};
    int fLen = snprintf(shmKeyFile, keyFileFullLen - 1, "%s/.shm_key_path/%d_%d.shm", getenv("HOME"), procId[idx1], procId[idx2]);
    shmKeyFile[fLen] = '\0';
    int shmKey = CSharedMemory::getKey(shmKeyFile);
    if(shmKey == 0)
    {
        return CreateKeyFileFailed;
    }
	
	int flag = 0222;   // 只读共享内存
    if (isCreate)
    {
		// 给key文件加锁，防止被重复打开多次以及其他非通信进程打开
        flag = 0666 | IPC_CREAT;  // 创建共享内存标识
		int fd = -1;
		if (CSharedMemory::lockFile(shmKeyFile, lockPos, fd) != Success)
        {
            return LockKeyFileFailed;
        }
    }
	
	// 创建共享内存
	int shmSize = sizeof(ShmHeader) + size * SIDE_COUNT;  // 申请的共享内存总大小
	int isNewCreate = 0;
	int shmId = -1;
	char* pShm = NULL;
	int rc = CSharedMemory::create(shmKey, shmSize, flag, isNewCreate, shmId, pShm);
	if (rc != Success)
	{
		return rc;
	}

    // 初始化共享内存数据
	m_shmInfo.id = shmId;
	m_shmInfo.key = shmKey;
    m_shmInfo.shmHeader = (ShmHeader*)pShm;
	if (isNewCreate)
	{
        // 共享内存新创建则初始化头部信息
		memset(m_shmInfo.shmHeader, 0, sizeof(ShmHeader));
		for (int i = 0; i < SIDE_COUNT; i++)
		{
			m_shmInfo.shmHeader->processLogicId[i] = procId[i];
			m_shmInfo.shmHeader->queue[i].size = size;
		}
		
		m_shmInfo.queue[idx1] = pShm + sizeof(ShmHeader);         // 队列1开始地址
	    m_shmInfo.queue[idx2] = m_shmInfo.queue[idx1] + size;     // 队列2开始地址
		ReleaseInfoLog("create shm size = %d", size);
	}
    else
	{
        // 共享已存在，判断本进程是否为该共享内存通信的其中一端
		// 这里等待1秒，等待共享内存的创建进程初始化共享内存头部信息完毕
		// sleep并不能解决并发冲突的问题，通常情况下需要按顺序启动通信双方的进程以避免冲突
		sleep(1);
        if (m_shmInfo.shmHeader->processLogicId[idx1] != procId[idx1]
		    || m_shmInfo.shmHeader->processLogicId[idx2] != procId[idx2])
		{
			ReleaseErrorLog("the shm not mate process logic id, srcId = %d, dstId = %d, procId1 = %d, procId2",
			                srcId, dstId, m_shmInfo.shmHeader->processLogicId[idx1], m_shmInfo.shmHeader->processLogicId[idx2]);
			return ShmNotMateId;
		}
		
		m_shmInfo.queue[idx1] = pShm + sizeof(ShmHeader);         // 队列1开始地址
	    m_shmInfo.queue[idx2] = m_shmInfo.queue[idx1] + m_shmInfo.shmHeader->queue[idx1].size;  // 队列2开始地址
		ReleaseInfoLog("get shm size = %d", size);
	}

    // 读写端的队列索引
    if (srcId == m_shmInfo.shmHeader->processLogicId[idx1])
	{
		m_shmInfo.writeQueue = idx1;
		m_shmInfo.readQueue = idx2;
	}
	else
	{
		m_shmInfo.writeQueue = idx2;
		m_shmInfo.readQueue = idx1;
	}
	
	ReleaseInfoLog("open shm, pid = %d, create = %d, size = %d, srcProcessId = %d, dstProcessId = %d, key file = %s, key = 0x%x, shmId = %d.",
	                getpid(), isCreate, size, srcId, dstId, shmKeyFile, shmKey, shmId);
	output();

	return Success;
}

// 从共享内存区输入&输出数据，会产生一次数据拷贝过程
int CShm::write(const char* pData, const int len)
{
	PkgQueue& queue = m_shmInfo.shmHeader->queue[m_shmInfo.writeQueue];
	return CMessageQueue::write(m_shmInfo.queue[m_shmInfo.writeQueue], queue, pData, len);
}

int CShm::read(char* pData, int& rLen)
{
	PkgQueue& queue = m_shmInfo.shmHeader->queue[m_shmInfo.readQueue];
	return CMessageQueue::read(m_shmInfo.queue[m_shmInfo.readQueue], queue, pData, rLen);
}

// 直接读写共享内存区，无数据拷贝过程
// 获取共享内存区buff，用于直接写入数据到该buff中
int CShm::beginWriteBuff(char*& pBuff, const int len)
{
	PkgQueue& queue = m_shmInfo.shmHeader->queue[m_shmInfo.writeQueue];
	return CMessageQueue::beginWriteBuff(m_shmInfo.queue[m_shmInfo.writeQueue], queue, pBuff, len);
}

int CShm::endWriteBuff(const char* pBuff, const int len)
{
	PkgQueue& queue = m_shmInfo.shmHeader->queue[m_shmInfo.writeQueue];
	return CMessageQueue::endWriteBuff(m_shmInfo.queue[m_shmInfo.writeQueue], queue, pBuff, len);
}

// 直接读写共享内存区，无数据拷贝过程
// 获取共享内存区buff，用于直接读取该buff中的数据
int CShm::beginReadBuff(char*& pBuff, int& len)
{
	PkgQueue& queue = m_shmInfo.shmHeader->queue[m_shmInfo.readQueue];
	return CMessageQueue::beginReadBuff(m_shmInfo.queue[m_shmInfo.readQueue], queue, pBuff, len);
}

int CShm::endReadBuff(const char* pBuff, const int len)
{
	PkgQueue& queue = m_shmInfo.shmHeader->queue[m_shmInfo.readQueue];
	return CMessageQueue::endReadBuff(m_shmInfo.queue[m_shmInfo.readQueue], queue, pBuff, len);
}



// 根据文件路径取共享内存key值
// 直接使用文件节点号作为共享内存的key值
int CSharedMemory::getKey(const char* keyFile)
{
	struct stat fInfo;
    int key = 0;
	if (stat(keyFile, &fInfo) != 0)
	{
		if (ENOENT == errno)  // 不存在则创建
		{
			char cmd[MaxFullLen * SIDE_COUNT] = {0};
			snprintf(cmd, sizeof(cmd), "mkdir -p %s", keyFile);
			*strrchr(cmd, '/') = 0;
			system(cmd);   // 创建目录
			snprintf(cmd, sizeof(cmd), "touch %s", keyFile);   
			system(cmd);   // 创建空白文件
			if (stat(keyFile, &fInfo) == 0)
			{
				key = fInfo.st_ino;
			}
		}
	}
	else
	{
		key = fInfo.st_ino;
	}

    if (key == 0)
	{
		ReleaseErrorLog("create key file = %s error = %d, info = %s", keyFile, errno, strerror(errno));
	}
	
    return key;
}

// 锁定key文件，防止重复多次打开共享内存
// 进程退出后文件自动解锁，不需要显示解锁key文件动作
// 解锁动作：1）进程退出；2）close文件描述符；3）主动调用api解锁
int CSharedMemory::lockFile(const char* keyFile, int lockPos, int& fd)
{
	fd = ::open(keyFile, O_RDWR);  // 进程运行期间不关闭文件，进程退出自动关闭
    if (fd < 0)
    {
		ReleaseErrorLog("open key file = %s to do lock shm error = %d, info = %s", keyFile, errno, strerror(errno));
        return OpenKeyFileFailed;
    }
	
    struct flock fLock;
    fLock.l_type = F_WRLCK;
    fLock.l_whence = SEEK_SET;
    fLock.l_start = lockPos;
    fLock.l_len = 1;
    if (fcntl(fd, F_SETLK, &fLock) < 0)
    {
		ReleaseErrorLog("lock shm key file = %s error = %d, info = %s", keyFile, errno, strerror(errno));
        return LockKeyFileFailed;
    }
	
	return Success;
}

int CSharedMemory::unLockFile(int fd, int lockPos)
{
    struct flock fLock;
    fLock.l_type = F_UNLCK;
    fLock.l_whence = SEEK_SET;
    fLock.l_start = lockPos;
    fLock.l_len = 1;
    if (fcntl(fd, F_SETLK, &fLock) < 0)
    {
		ReleaseErrorLog("unlock shm key file = %d error = %d, info = %s", fd, errno, strerror(errno));
        return UnLockKeyFileFailed;
    }
	
	::close(fd);
	
	return Success;
}

int CSharedMemory::create(int key, int size, int flag, int& isCreate, int& shmId, char*& pShm)
{
	isCreate = 0;
	shmId = -1;
	pShm = NULL;
	if (flag & IPC_CREAT)
	{
		int createFlag = flag | IPC_EXCL;  // 互斥创建
		shmId = ::shmget(key, size, createFlag);
		if (shmId >= 0)
		{
			isCreate = 1;  // 第一次新创建的共享内存			
		}
	}
	if (shmId < 0)
	{
		shmId = ::shmget(key, 0, flag);  // key对应的共享内存已经存在
		if (shmId < 0)
		{
			ReleaseErrorLog("get shm failed, key = %d, size = %d, flag = %d, erron = %d, info = %s",
			                key, size, flag, errno, strerror(errno));
			return CreateShmFailed;
		}
	}
	
	void* pUponShm = ::shmat(shmId, NULL, 0);
	if (pUponShm == (void*)-1)
	{
		ReleaseErrorLog("at shm failed, key = %d, shmId = %d, erron = %d, info = %s",
						key, shmId, errno, strerror(errno));
		return UponShmFailed;
	}
	pShm = (char*)pUponShm;
	
	struct shmid_ds shmStat ;
	if (::shmctl(shmId, IPC_STAT, &shmStat) == 0)
	{
		ReleaseInfoLog("get shm size = %d, and user input size = %d", (int)shmStat.shm_segsz, size);
	}
	
	return Success;
}

// 挂接共享内存
int CSharedMemory::atShm(int key, int& shmId, char*& pShm)
{
	int isNewCreate = 0;
	return create(key, 0, 0666, isNewCreate, shmId, pShm);
}

int CSharedMemory::release(void*& shm)
{
	if (shm != NULL)
	{
		// 脱离共享内存
		// 此处不要调用shmctl删除共享内存，否则其他进程将映射不到该共享内存
		// 共享内存为双方进程共用，因此需要手动删除
	    if (::shmdt(shm) != 0)
		{
			ReleaseErrorLog("release shm failed, shm pointer = %p, erron = %d, info = %s", shm, errno, strerror(errno));
			return OffShmFailed;
		}
		shm = NULL;
	}
	
	return Success;
}

}

