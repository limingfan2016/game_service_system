
/* author : limingfan
 * date : 2015.01.30
 * description : 服务开发框架各个类型定义
 */

#ifndef FRAMETYPE_H
#define FRAMETYPE_H

#include "connect/ILogicHandler.h"


namespace NFrame
{
	enum ConnectProxyOperation
	{
        ProxyException = -1,            // 代理服务异常
        
		ActiveClosed = 1,               // 服务端主动要求关闭代理的连接
		PassiveClosed = 2,              // 代理的连接已经被动关闭了
		StopProxy = 3,                  // 服务已经停止了，代理该服务的所有连接都将被关闭
	};

	
	// 客户端连接的地址
	struct ConnectAddress
	{
		struct in_addr peerIp;          // 客户端连接对端的ip
		unsigned short peerPort;        // 客户端连接对端的端口号
	};
	
	// 连接代理数据
	struct ConnectProxy
	{
		int proxyFlag;                  // 连接代理标识符
		unsigned int proxyId;           // 代理服务的ID

		void* userCb;                   // 用户挂接的回调数据
		int userId;                     // 用户挂接的ID
		
		struct in_addr peerIp;          // 客户端连接对端的ip
		unsigned short peerPort;        // 客户端连接对端的端口号
	};

	// 定时器消息处理者函数
	class CHandler;
    typedef void (CHandler::*TimerHandler)(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	class CNetDataHandler;
    typedef void (CNetDataHandler::*ActiveConnectHandler)(NConnect::Connect* conn, const char* peerIp, const unsigned short peerPort, void* userCb, int userId);
	

    // 外部客户端消息头部数据，注意字节对齐
	#pragma pack(1)
	struct ClientMsgHeader
	{
		unsigned int serviceId;         // 服务ID
		unsigned short moduleId;        // 服务下的模块ID
		unsigned short protocolId;      // 模块下的协议ID
		unsigned int msgId;             // 消息ID
		unsigned int msgLen;            // 消息码流长度，不包含消息头长度
	};
	#pragma pack()
	
	
	// 服务消息处理者标识符
	struct MsgHandlerID
	{
		unsigned int serviceId;         // 服务ID
		unsigned short serviceType;     // 服务类型
		unsigned short moduleId;        // 服务下的模块ID
		unsigned short protocolId;      // 模块下的协议ID
	};
	
	// 注意字节对齐
	#pragma pack(1)
	struct ServiceMsgHeader
	{
		MsgHandlerID srcService;        // 消息源服务
		MsgHandlerID dstService;        // 消息目标服务
		unsigned int msgId;             // 消息ID
		int userFlag;                   // 用户携带的标志数据
		unsigned int userDataLen;       // 用户数据长度
		unsigned int msgLen;            // 消息码流长度，不包含消息头长度
		unsigned int asyncDataFlagLen;  // 异步数据标识长度
	};
	#pragma pack()
	
	// 定时器消息
	struct TimerMessage
	{
		unsigned int timerId;           // 定时器ID
		unsigned int count;             // 剩余触发次数
		CHandler* handler;              // 定时器消息处理者
		TimerHandler cbFunc;            // 回调函数
		void* param;                    // 用户设置的数据
		unsigned int userId;            // 用户设置的ID
		unsigned int deleteRef;         // 删除消息的引用计数
	};
	
	// 主动连接数据
	struct ActiveConnectData
	{
		NConnect::ActiveConnect data;   // 用户传递的连接主机的参数值
		void* userCb;                   // 用户传递的回调数据
        ActiveConnectHandler cbFunc;    // 用户传递的回调函数
		CNetDataHandler* instance;      // 目标处理对象
		NConnect::Connect* conn;        // 新建立的连接数据
	};
}

#endif // FRAMETYPE_H
