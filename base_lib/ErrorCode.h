
/* author : limingfan
 * date : 2014.10.15
 * description : 各个模块，各种函数，返回码、错误码定义，统一管理
 */
 
#ifndef ERROR_CODE_H
#define ERROR_CODE_H


namespace NErrorCode
{

// redis 错误码范围[-3 -- -20]
enum ERedis
{
	NormalError = -3,		//redis常规错误，说明用的不对
	NetError = -4,			//网络连接错误
	SpaceNotEnough = -5,	//存储空间不够
};

// 公共模块 错误码范围[0---100]
enum ECommon {
	Success = 0,          // 调用成功
	InvalidParam = 1,     // 无效参数
	NoMemory = 2,         // new 失败，没内存了
	
	QueueFull = 3,        // 消息队列满了
	SystemCallErr = 4,    // 系统调用失败
};


// 共享内存模块 错误码范围[101---120]
enum EShm {
	OpenKeyFileFailed = 101,          // 打开共享内存key文件失败
	LockKeyFileFailed = 102,          // 锁定共享内存key文件失败
	CreateKeyFileFailed = 103,        // 创建共享内存key文件失败
	ShmProcIdEqual = 104,             // 共享内存通信两端逻辑进程号相同
	CreateShmFailed = 105,            // 创建共享内存失败
	UponShmFailed = 106,              // 映射共享内存失败
	ShmNotMateId = 107,               // 共享内存不匹配通信进程逻辑ID
	OffShmFailed = 108,               // 关闭共享内存失败
	PkgQueueFull = 109,               // 数据包队列满了
	PkgQueueEmpty = 110,              // 数据包队列空了
	PkgSuperLarge = 111,              // 用户数据包过大
	ReadPkgConflict = 112,            // 读取数据包数据发生冲突了
	RWInvalidParam = 113,             // 读写数据包传递的参数无效
	ReadPkgBuffSmall = 114,           // 读写数据包的用户空间不足
	UnLockKeyFileFailed = 115,        // 解锁共享内存key文件失败
};


// 网络连接模块 错误码范围[121---150]
enum EConnect {
	CreateSocketFailed = 121,         // 创建socket失败
	CloseSocketFailed = 122,          // 关闭socket失败
	InValidIp = 123,                  // 无效ip地址
	BindIpPortFailed = 124,           // 绑定ip、端口失败
	ListenConnectFailed = 125,        // 启动监听连接失败
	ConnectIpPortFailed = 126,        // 连接失败
	SetLingerFailed = 127,            // 选项设置失败
	SetRcvBuffFailed = 128,           // 选项设置失败 
	SetSndBuffFailed = 129,           // 选项设置失败
	SetReuseAddrFailed = 130,         // 选项设置失败
	SetNagleFailed = 131,             // 选项设置失败
	SetNonBlockFailed = 132,          // 选项设置失败
	AcceptConnectFailed = 133,        // 建立连接失败
};


// 连接管理服务 错误码范围[151---170]
enum EConnServer {
    CreateMemPoolFailed = 151,        // 创建内存池失败
	CreateEPollFailed = 152,          // 创建epoll模型失败
	AddEPollListenerFailed = 153,     // 增加监听者失败
	RemoveEPollListenerFailed = 154,  // 删除监听者失败
	ModifyEPollListenerFailed = 155,  // 修改监听事件失败
	ListenConnectException = 156,     // 监听连接异常
};


// 服务消息通信中间件 错误码范围[171---180]
enum ESrvMsgComm {
	InitSharedMutexError = 171,       // 初始化共享内存共享锁失败
    SrvMsgCommCfgError = 172,         // SrvMsgComm 段配置错误
	ServiceIDCfgError = 173,          // ServiceID 段配置错误
	NetConnectCfgError = 174,         // NetConnect 段配置错误
	NotFoundService = 175,            // 没有找到对应的服务
	SharedMemoryDataError = 176,      // 共享内存数据错误
	GetSharedMemoryBlockError = 177,  // 获取共享内存块错误
	CfgInvalidIPError = 178,          // 配置了无效的IP
	NodePortCfgError = 179,           // NodePort 段配置错误
	GetSrvFileKeyError = 180,         // 获取服务对应的关键文件失败
	NotServiceMsg = 181,              // 服务当前没有收到消息
};


// 服务框架 错误码范围[191---210]
enum ESrvFrame {
	NotRegisterService = 191,         // 未注册服务
	LargeMessage = 192,               // 超大消息包不支持
	LargeUserData = 193,              // 超大用户数据不支持
	ServiceIPError = 194,             // 配置了无效的IP
	ServicePortError = 195,           // Port配置错误
	SrvConnectCfgError = 196,         // 服务连接配置项错误
	NoSupportConnectClient = 197,     // 不支持客户端连接
	NoFoundClientConnect = 198,       // 找不到用户名对应的连接
	NoFoundConnectProxy = 199,        // 找不到用户名对应的连接代理
	NetClientContextError = 200,      // 网络连接客户端上下文错误
	NoFoundProtocolHandler = 201,     // 找不到对应的协议处理函数
	NoLogicHandle = 202,              // 无处理逻辑处理
	LargeAsyncDataFlag = 203,         // 超大异步数据标识不支持
};


// MySql操作 错误码范围[211---230]
enum EMySqlOpt {
	InitMySqlLibError = 211,          // 初始化MySql库失败错误
	ConnectMySqlError = 212,          // 连接MySql错误
	DoSqlCmmError = 213,              // 执行sql命令出错
	GetSqlAllResultError = 214,       // 一次性获取所有查询结果集出错
	GetSqlResultError = 215,          // 获取单个查询结果集出错
	DoPreStmtCmmError = 216,          // 执行预处理sql命令出错
};


};


#endif // ERROR_CODE_H
