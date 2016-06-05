
/* author : limingfan
 * date : 2015.01.15
 * description : 消息数据结构类型
 */

#ifndef MSG_COMM_TYPE_H
#define MSG_COMM_TYPE_H

#include <pthread.h>
#include "base/Type.h"
#include "base/Constant.h"


using namespace NCommon;

namespace NMsgComm
{

// 相对共享内存头部的偏移量
typedef unsigned int BuffIndex;
typedef unsigned int QueueIndex;
typedef unsigned int HandlerIndex;


// 节点状态
enum NodeStatus
{
	InitConnect = 0,
	NormalConnect = 1,
};

// 共享内存队列信息，每块共享内存对应一个MsgQueueList
struct MsgQueueList
{
	unsigned int readerId;       // 队列接收方
	unsigned int writerId;       // 队列发送方，0为外部节点的网络数据，非0为本节点数据
	
	PkgQueue header;             // 数据队列头信息
	BuffIndex queue;             // 队列数据开始位置
	
	QueueIndex next;             // 串成队列链表
};

// 每个读写消息对象
struct MsgHandlerList
{
	unsigned int readerId;       // 队列接收方
	QueueIndex readMsg;          // 从此队列读消息
	
	unsigned int status;         // 节点状态
	uuid_type connId;            // 网络节点的连接ID
	NodeID readerNode;           // 接收方所在节点，0为当前节点，非0为其他外部节点

	HandlerIndex next;           // 串成对象链表
};

typedef MsgHandlerList NodeMsgHandler;   // 当前节点的消息数据
typedef MsgHandlerList NetMsgHandler;    // 非本节点，网络消息数据

// 进程间共享锁
struct SharedMutex
{
	pthread_mutexattr_t mutexAttr;
	pthread_mutex_t mutex;
};

// 整个共享内存数据
struct ShmData
{
	HandlerIndex nodeMsgHandlers;    // 本节点已用的共享内存块，本节点进程间通信非网络消息
	HandlerIndex netMsgHanders;      // 非本节点已用的共享内存块，本节点（主动连接）的网络消息
	
	QueueIndex msgQueueList;         // 剩余可用的共享内存块
	HandlerIndex msgHandlerList;     // 剩余可用的消息处理者对应的内存块
	
	unsigned int msgQueueSize;       // 用户配置的总共享内存大小，其值和配置大小一致则表示共享内存已经初始化可用，否则共享内存不可用
	SharedMutex sharedMutex;         // 进程间共享锁，进程申请共享内存块的时候需要加锁
};


// 网络消息头部数据
struct NetMsgHeader
{
	unsigned int readerId;           // 队列接收方
	unsigned int writerId;           // 队列发送方
};


// 消息通信中间件配置信息
struct CfgData
{
	char Name[CfgKeyValueLen];           // 通信中间件实例名
	strIP_t IP;                          // 本节点IP
	unsigned short Port;                 // 本节点通信端口号
	unsigned int ShmSize;                // 每块共享内存大小
	unsigned int ShmCount;               // 共享内存块个数
	unsigned int ActiveConnectTimeOut;   // 源端主动建立连接，最长等待时间，单位：秒
	unsigned int MaxServiceId;           // 服务实例ID的最大值
	unsigned int NetNodeCount;           // 配置了的网络节点个数
	unsigned int RevcMsgCount;           // 服务收消息策略，即服务单次从单一共享内存通道读取消息的最大个数
};

}

#endif // MSG_COMM_TYPE_H
