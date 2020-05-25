
/* author : admin
 * date : 2015.01.30
 * description : 服务开发用户数据类型
 */

#ifndef USER_TYPE_H
#define USER_TYPE_H

#include "FrameType.h"


namespace NFrame
{

// 一个服务最多支持 MaxModuleIDCount 个模块
// 注册的模块ID不能大于等于 MaxModuleIDCount 的值
static const unsigned int MaxModuleIDCount = 8;


// 纯网络数据处理者注册的对应模块ID
static const unsigned int NetDataHandleModuleID = MaxModuleIDCount - 1;


static const unsigned int MaxUserDataLen = 32;                              // 支持的用户数据最大长度不能超过 MaxUserDataLen
static const unsigned int MaxLocalAsyncDataFlagLen = 128;         // 支持的本地异步数据标识最大长度不能超过 MaxLocalAsyncDataFlagLen
static const unsigned int MaxMsgLen = 1024.00 * 1024.00 * 1.20;                     // 支持的消息最大长度不能超过 MaxMsgLen


static const unsigned int MsgHeaderLen = sizeof(ServiceMsgHeader);
static const unsigned int ClientMsgHeaderLen = sizeof(ClientMsgHeader);
static const unsigned int MaxMsgLength = MaxMsgLen - MsgHeaderLen - MaxUserDataLen - MaxLocalAsyncDataFlagLen;


// 框架内置的服务类型
static const unsigned int CommonServiceType = 0;                         // 公共服务类型
static const unsigned int GatewayServiceType = 1;                          // 网关服务类型
static const unsigned int OutsideClientServiceType = 2;                 // 外部客户端类型


// 消息类型
enum MessageType
{
	InnerServiceMsg = 0,              // 内部服务器间消息
	NetClientMsg = 1,                  // 网络客户端过来的消息
	ConnectProxyMsg = 2,           // 连接代理数据
};

}

#endif // USER_TYPE_H
