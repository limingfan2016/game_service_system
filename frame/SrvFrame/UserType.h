
/* author : limingfan
 * date : 2015.01.30
 * description : 服务开发用户数据类型
 */

#ifndef USER_TYPE_H
#define USER_TYPE_H


namespace NFrame
{

// 一个服务最多支持 MaxModuleIDCount 个模块
// 注册的模块ID不能大于等于 MaxModuleIDCount 的值
static const unsigned int MaxModuleIDCount = 8;

static const unsigned int MaxUserDataLen = 32;                    // 支持的用户数据最大长度不能超过 MaxUserDataLen
static const unsigned int MaxLocalAsyncDataFlagLen = 128;         // 支持的本地异步数据标识最大长度不能超过 MaxLocalAsyncDataFlagLen
static const unsigned int MaxMsgLen = 1024 * 512;                 // 支持的消息最大长度不能超过 MaxMsgLen
static const unsigned int ServiceIdBaseValue = 1000;              // 服务ID的倍数基值，即服务 ServiceType = ServiceID / ServiceIdBaseValue


// 服务类型
enum ServiceType
{
	CommonSrv = 0,                 // 公共类型
	LoginSrv = 1,
	LoginListSrv = 2,
	DBProxyCommonSrv = 3,
	BuyuGameSrv = 4,			   // 捕鱼游戏
	GameSrvDBProxy = 5,            // 游戏服务逻辑数据DB代理服务
	MessageSrv = 6,				   // 消息服务
	ManageSrv = 7,				   // 管理服务
	LandlordsGameSrv = 8,		   //斗地主
	GatewayProxySrv = 28,	       // 网关代理服务
	SimulationClientSrv = 29,      // 仿真客户端服务
	HttpSrv = 30,                  // Http服务
	OutsideClientSrv = 31,         // 外部客户端服务类型
	MaxServiceType = 32,           // 最大服务类型，不能超过此值
};


// 消息类型
enum MessageType
{
	InnerServiceMsg = 0,           // 内部服务器间消息
	NetClientMsg = 1,              // 网络客户端过来的消息
	ConnectProxyMsg = 2,           // 连接代理数据
};

}

#endif // USER_TYPE_H
