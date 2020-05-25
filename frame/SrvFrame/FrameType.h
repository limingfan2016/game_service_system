
/* author : admin
 * date : 2015.01.30
 * description : 服务开发框架各个类型定义
 */

#ifndef FRAMETYPE_H
#define FRAMETYPE_H

#include "connect/ILogicHandler.h"


namespace NFrame
{
	/* SERVICE_LOGIN_GAME.proto 文件
	// 用户在线&离线状态
	enum EUserQuitStatus
    {
        EUserStatusInvalid = 0;           // 无效值
        
        EUserOnline = 1;                  // 正常在线

        EUserPassiveOffline = 2;          // 被动关闭连接，下线

        EUserChangeRoom = 3;              // 玩家更换房间，下线
        EUserBeForceOffline = 4;          // 玩家被强制下线
        EUserRepeateLogin = 5;            // 重复登录被关闭连接，下线
        EUserInvalidMessage = 6;          // 非法消息被关闭连接，下线
        EUserRepeateConnect = 7;          // 重复连接被关闭，下线
        EUserPromptLeaveRoom = 8;         // 房间提示玩家离开，连接被关闭，下线
        EUserBeKicked = 9;                // 玩家被后台踢下线

        EUserInvalidService = 10;         // 网关收到非目标服务（比如内部公共服务）的非法消息，关闭连接
        EUserOversizedMessage = 11;       // 网关收到非法超大消息包，关闭连接
        EUserFrequentMessage = 12;        // 网关频繁过度收到大量客户端消息，关闭连接
        EServiceFrequentMessage = 13;     // 网关频繁过度收到大量服务端消息，关闭连接
        EGatewayServiceStop = 14;         // 网关服务停止，关闭连接
        
        EServiceStart = 100;              // 服务启动（EServiceStatus::ESrvOnline），下线
        EServiceStop = 101;               // 服务停止（EServiceStatus::ESrvOffline），下线
        EServiceFault = 102;              // 服务发生故障（EServiceStatus::ESrvFaultStop），要求停止相关服务
        
        ELoginErrorBase = 10000;          // 大厅登录失败
        
        EUserOffline = 1000000;           // 正常离线（排序原因，因此定义为最大值）
    }
	*/

	enum ConnectProxyOperation
	{
        ProxyException = -1,            // 代理服务异常
        
		// 其值定义和业务上层保持一致，不能冲突
		// 参考 proto 协议 EUserQuitStatus 定义（用户在线&离线状态）
		ActiveClosed = 1,               // 服务端主动要求关闭代理的连接
		PassiveClosed = 2,              // 代理的连接已经被动关闭了
		StopProxy = 3,                  // 服务已经停止了，代理该服务的所有连接都将被关闭
		
		InvalidService = 10,            // 网关收到非目标服务（比如内部公共服务）的非法消息，关闭连接
	    OversizedMessage = 11,          // 网关收到非法超大消息包，关闭连接
	    ClientFrequentMessage = 12,           // 网关频繁过度收到大量客户端消息，关闭连接
        ServiceFrequentMessage = 13,     // 网关频繁过度收到大量服务端消息，关闭连接
		GatewayServiceStop = 14,        // 网关服务停止，关闭连接
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
        unsigned int checksum;          // 校验这个字段之后数据的完整性

        unsigned int sequence;          // 序列号，防止重复包攻击
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
        unsigned int paramLen;          // 用户设置的数据长度，存在长度时定时器将创建内存空间存储用户数据
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
