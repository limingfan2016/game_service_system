
/* author : admin
 * date : 2014.11.19
 * description : 网络管理模块各种类型定义
 */
 
#ifndef NET_TYPE_H
#define NET_TYPE_H

#include <netinet/in.h>

#include "base/Type.h"
#include "base/CSingleW_SingleR_Buffer.h"
#include "base/CMutex.h"
#include "base/CCond.h"


using namespace NCommon;

namespace NConnect
{

enum DataHandlerType
{
	ThreadHandle = 0,               // 由网络层内部创建线程主动收发、解析处理数据
	NetDataParseActive = 1,         // 网络层内部解析处理数据，外部主动调用收发数据消息包
	DataTransmit = 2,               // 网络层内部不解析数据，外部主动调用收发、解析纯网络数据
};

// 连接&逻辑处理状态
enum ConnectStatus
{
	normal = 0,                  // 连接正常，可读写数据 [连接线程] 填写
	closed = 1,                  // 连接已关闭，通知读写线程连接关闭，等待读写数据线程取走缓冲区中的数据 [连接线程] 填写
	connecting = 2,              // 本地端发起的主动连接处于正在建立的过程中，还没有完全建立成功完毕 [连接线程] 填写
	deleted = 3,                 // 读写线程已取走缓冲区数据，直到此时连接管理线程才可以真正删除连接 [数据处理线程] 填写
	exception = 4,               // 连接对应的逻辑处理发生异常，需要直接断开并删除此连接 [数据处理线程] 填写
	sendClose = 5,               // 连接里的数据发送完毕后直接关闭连接 [数据处理线程] 填写
};

// 连接关联的数据，注意字节对齐
struct Connect
{
	Connect* pPre;                            // 连接链表中的前一个连接
	Connect* pNext;                           // 连接链表中的下一个连接
	
	CSingleW_SingleR_Buffer* readBuff;        // 从socket连接里读数据放入此缓冲区，由 [连接线程] 创建&修改&释放
	CSingleW_SingleR_Buffer* curReadIdx;      // 当前读buff索引，由 [数据处理线程] 使用不断修改
	
	CSingleW_SingleR_Buffer* writeBuff;       // 把此缓冲区的数据写入socekt连接，由 [数据处理线程] 创建&修改&释放
	CSingleW_SingleR_Buffer* curWriteIdx;     // 当前写buff索引，由 [连接线程] 使用不断修改

	unsigned int heartBeatSecs;               // 上次心跳时间，即上次发送心跳的时间点，单位秒，由 [数据处理线程] 发送心跳消息时填写
	unsigned int activeSecs;                  // 上次活跃时间，即上次收到用户数据的时间点，单位秒，由 [数据处理线程] 读出socket数据时填写
	int needReadSize;                         // 需要从readBuff中读取的数据长度，为0时表示读取数据包头部大小，大于0时表示读取数据包体，由 [数据处理线程] 读出socket数据时填写
	
	void* userCb;                             // 用户挂接的回调数据
	int userId;                               // 用户挂接的ID
	uuid_type id;                             // 连接ID
	int fd;                                   // 连接socket描述符
	unsigned short checkSocketTimes;           // 按一定时间间隔检查socket，没有数据的次数
	unsigned short peerPort;                  // 连接对端的端口号
	struct in_addr peerIp;                    // 连接对端的ip
	
	unsigned char hbResult;                   // 心跳检查结果：0正常，大于0则为心跳检测的连续失败次数，由 [数据处理线程] 填写
	unsigned char logicStatus;                // 该连接对应的逻辑处理状态：0正常，2可删除，3逻辑异常，5发送数据完毕后关闭连接；由 [数据处理线程] 填写
	unsigned char connStatus;                 // 连接的状态：0正常，1关闭，2连接处在建立过程；由 [连接线程] 填写
	unsigned char canWrite;                   // 连接是否可写：0不可写，1连接可写；由 [连接线程] 填写
};


// 线程处理者状态
enum HandlerStatus
{
	parallel = 0,       // 连接线程&数据处理线程，并发执行
	waitDH = 1,         // 连接线程等待数据处理线程
	waitConn = 2,       // 数据处理线程等待连接线程
};

// 连接管理线程和数据处理线程同步条件，注意字节对齐
struct SynNotify
{
	CMutex waitDataHandlerMutex;    // 连接线程等待数据读写线程
	CCond waitDataHandlerCond;      // 条件变量通知
	
	CMutex waitConnecterMutex;      // 数据读写线程等待连接线程
	CCond waitConnecterCond;        // 条件变量通知
	
	volatile int status;                     // parallel 0:并发执行； waitDH 1:连接线程等待数据处理线程； waitConn 2:数据处理线程等待连接线程
};


// 网络数据包类型
enum NetPkgType
{
	MSG = 0,          // 用户消息包
	CTL = 1,          // 控制类型包
};

// 控制标示符
enum CtlFlag
{
	HB_RQ = 0,        // 心跳请求包，发往对端
	HB_RP = 1,        // 心跳应答包，来自对端

	BLACK_LIMIT = 2,  // 黑名单限制包，发往对端后服务器关闭连接
	WHITE_LIMIT = 3,  // 白名单限制包，发往对端后服务器关闭连接
};

// 网络数据包头部，注意字节对齐
// 1）len字段必填写，并且需要转成网络字节序
// 2）如果是用户协议消息，其他字段为0；如果是控制消息则根据类型填写
#pragma pack(1)
struct NetPkgHeader
{
	uint32_t len;    // 数据包总长度，包括包头+包体
	uchar8_t type;   // 数据包类型 NetPkgType ，用户消息包则值为 0
	uchar8_t cmd;    // 控制信息 CtlFlag 
	uchar8_t ver;    // 版本号，目前为 0
	uchar8_t res;    // 保留字段，目前为 0
	
	NetPkgHeader() {}
	NetPkgHeader(uint32_t _len, uchar8_t _type, uchar8_t _cmd) : len(_len), type(_type), cmd(_cmd), ver(0), res(0) {}
	~NetPkgHeader() {}
};
#pragma pack()

}


#endif  // NET_TYPE_H
