
/* author : limingfan
 * date : 2015.01.22
 * description : 共享内存服务间消息通信
 */

#ifndef CMSGCOMM_H
#define CMSGCOMM_H

#include "MsgCommType.h"
#include "ISrvMsgComm.h"
#include "base/MacroDefine.h"
#include "base/Constant.h"
#include "connect/NetType.h"


using namespace NConnect;

namespace NMsgComm
{

static const unsigned int MaxServiceId = 100000;    // 服务实例的ID值必须大于0并且小于 MaxServiceId 的值


// 自动加解共享锁Helper
class CSharedLockGuard
{
public:
	CSharedLockGuard(SharedMutex& sharedMutex) : rSharedMutex(sharedMutex)
	{
		Success = pthread_mutex_lock(&rSharedMutex.mutex);
	}
	
	~CSharedLockGuard()
	{
		if (Success == 0) pthread_mutex_unlock(&rSharedMutex.mutex);
	}
	
	int Success;
	
private:
	SharedMutex& rSharedMutex;
	
DISABLE_CONSTRUCTION_ASSIGN(CSharedLockGuard);
};


// 服务间网络消息头部数据
struct SrvMsgHeader
{
	NetPkgHeader netPkgHeader;  // 网络层数据包头部
	NetMsgHeader netMsgHeader;  // 服务间通信网络消息头部
};


class CMsgComm : public ISrvMsgComm
{
public:
    // 服务间通信公共操作api
	static int getFileKey(const char* pathName, const char* srvName, char* fileName, int len);
	static int addHandlerToNode(ShmData* shmData, NodeMsgHandler*& msgHandler);            // 申请共享内存块并挂接到当前节点队列
	static MsgQueueList* addQueueToHandler(ShmData* shmData, MsgHandlerList* msgHandler);  // 申请共享内存消息队列挂接到消息处理者


public:
	CMsgComm(const char* srvMsgCommCfgFile, const char* srvName);
	virtual ~CMsgComm();
	
public:
    // 必须先初始化操作成功才能收发消息
	virtual int init();
	virtual void unInit();
	virtual CCfg* getCfg();
	virtual void reLoadCfg();
	
public:
    // 获取本服务名、各服务名对应的ID
	virtual const char* getSrvName();
	virtual unsigned int getSrvId(const char* srvName);
	
public:
    // 收发消息
	virtual int send(const unsigned int srvId, const char* data, const unsigned int len);
	virtual int recv(char* data, unsigned int& len);
	
public:
    // 直接从缓冲区空间收消息，避免拷贝数据，效率较高
	virtual int beginRecv(char*& data, int& len, void*& cb);
	virtual void endRecv(const char* data, const int len, void* cb);
	
private:
	MsgQueueList* getSrvMsgQueue();
	MsgQueueList* getReceiverMsgQueue(const unsigned int readerId);
	
private:
	unsigned int m_maxServiceId;
	unsigned int m_srvId;
	char m_srvName[CfgKeyValueLen];
	char* m_shmAddr;
	ShmData* m_shmData;
	MsgQueueList** m_msgQueue;
	MsgHandlerList* m_msgHandler;
	QueueIndex m_curQueueIdx;
	CCfg* m_cfg;
	const Key2Value* m_srvNameId;
	SrvMsgHeader m_srvMsgHeader;

DISABLE_CONSTRUCTION_ASSIGN(CMsgComm);
};

}

#endif // CMSGCOMM_H
