
/* author : limingfan
 * date : 2015.04.23
 * description : 类型定义
 */

#ifndef TYPE_DEFINE_H
#define TYPE_DEFINE_H

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <string>
#include <unordered_map>
#include "base/Type.h"
#include "common/CommonType.h"
#include "app_http_service.pb.h"


using namespace std;

namespace NPlatformService
{

// 常量定义
static const unsigned int MaxReceiveNetBuffSize = 10240;
static const unsigned int MaxNetBuffSize = 2048;
static const unsigned int MaxKeySize = 36;
static const unsigned int MaxDataQueueSize = 8192;
static const unsigned int MaxConnectIdLen = 36;
static const unsigned int MaxUserNameLen = 36;
static const unsigned int MaxUserDataCount = 128;
static const unsigned int MaxConnectDataCount = 128;
static const unsigned int MaxRequestDataLen = 2048;
static const unsigned int MaxRechargeTransactionLen = 256;
static const unsigned int MinRechargeLogFileSize = 1024 * 1024 * 8;
static const unsigned int MinRechargeLogFileCount = 20;
static const unsigned int Md5BuffSize = 36;
static const unsigned int Md5ResultLen = 32;

static const unsigned int HttpConnectTimeOut = 10;  // 连接超时时间，单位秒
static const unsigned short HttpsPort = 443;

static const char* RequestHeaderFlag = "\r\n\r\n";
static const unsigned int RequestHeaderFlagLen = strlen(RequestHeaderFlag);

static const char* ProtocolGetMethod = "GET";
static const unsigned int ProtocolGetMethodLen = strlen(ProtocolGetMethod);
static const char* ProtocolPostMethod = "POST";
static const unsigned int ProtocolPostMethodLen = strlen(ProtocolPostMethod);


// 服务返回码
enum SrvRetCode
{
	OptFailed = -1,
	OptSuccess = 0,

    HeaderDataError = 30001,
	RechargeTranscationFormatError = 30002,               // 充值订单号格式错误
	NoFoundGoogleRechargeTranscation = 30003,             // 没有找到Google SDK对应的账单ID
	GoogleSignVerityError = 30004,                        // Google SDK签名校验失败
	ParamValueError = 30005,                              // 参数错误
	SaveTransactionError = 30006,                         // 存储订单号失败
	BodyDataError = 30007,                                // http消息体错误
	IncompleteBodyData = 30008,                           // 不完整的http消息体
	CheckXiaoMiUserError = 30009,                         // 校验小米用户返回错误
	CheckWanDouJiaUserError = 30010,                      // 校验豌豆荚用户返回错误
	GetPriceFromMallDataError = 30011,                    // 从商城数据获取物品价格错误
    CheckQiHu360UserError = 30012,                        // 校验360用户返回错误
    UpdateTxServerDataError = 30013,                      // 同步腾讯服务器上数据失败
    FindOrderError = 30014,                               // 查询内部订单失败
	GoogleCheckPayFailed = 30015,                         // Google SDK付费校验失败
	GetFishCoinDataError = 30016,                         // 获取渔币信息错误
	NonExistentFirstPackage = 30017,                      // 已经充值了，没有首冲礼包了
	GetUserOtherInfoError = 30018,                        // 获取用户信息失败
	DeductTXFishCoinError = 30019,                        // 扣除腾讯托管的渔币失败
	TXRechargePriceNotMatching = 30020,                   // 腾讯充值后查询充值的价格与实际符合
	SerializeToArrayError = 30021,                        // 消息编码失败
	GetVivoOrderError = 30022,                            // 获取VIVO充值订单失败
};

// http服务协议ID值
enum HttpServiceProtocol
{
	// 当乐用户登录校验
	CHECK_DOWNJOY_USER_REQ = 1,
	CHECK_DOWNJOY_USER_RSP = 2,
	
	// 当乐用户充值
	GET_DOWNJOY_RECHARGE_TRANSACTION_REQ = 3,
	GET_DOWNJOY_RECHARGE_TRANSACTION_RSP = 4,
	DOWNJOY_USER_RECHARGE_NOTIFY = 5,
	CANCEL_DOWNJOY_RECHARGE_NOTIFY = 6,
	
	// Google SDK 充值操作
	GET_GOOGLE_RECHARGE_TRANSACTION_REQ = 7,
	GET_GOOGLE_RECHARGE_TRANSACTION_RSP = 8,
	CHECK_GOOGLE_RECHARGE_RESULT_REQ = 9,
	CHECK_GOOGLE_RECHARGE_RESULT_RSP = 10,
	
	// 第三方平台用户充值
	GET_THIRD_RECHARGE_TRANSACTION_REQ = 11,
	GET_THIRD_RECHARGE_TRANSACTION_RSP = 12,
	THIRD_USER_RECHARGE_NOTIFY = 13,
	CANCEL_THIRD_RECHARGE_NOTIFY = 14,
	
	// 小米用户登录校验
	CHECK_XIAOMI_USER_REQ = 15,
	CHECK_XIAOMI_USER_RSP = 16,
	
    // 第三方平台用户校验
	CHECK_THIRD_USER_REQ = 17,
	CHECK_THIRD_USER_RSP = 18,
	
	// 商城配置
	GET_MALL_CONFIG_REQ = 19,
	GET_MALL_CONFIG_RSP = 20,
	
	//查询腾讯充值是否成功
	FIN_RECHARGE_SUCCESS_REQ = 21,
	FIN_RECHARGE_SUCCESS_RSP = 22,
	
	// 新版本游戏商城配置
	GET_GAME_MALL_CONFIG_REQ = 23,
	GET_GAME_MALL_CONFIG_RSP = 24,
	
	// 渔币充值
	GET_FISH_COIN_RECHARGE_ORDER_REQ = 25,
	GET_FISH_COIN_RECHARGE_ORDER_RSP = 26,
	FISH_COIN_RECHARGE_NOTIFY = 27,
	CANCEL_FISH_COIN_RECHARGE_NOTIFY = 28,
	
	// 苹果版本充值结果校验
	CHECK_APPLE_RECHARGE_RESULT = 29,
	
	// 韩友 Facebook 用户登录验证
	CHECK_FACEBOOK_USER_REQ = 30,
	CHECK_FACEBOOK_USER_RSP = 31,
	
	/*
	// 兑换游戏商品
	EXCHANGE_GAME_GOODS_REQ = 29,
	EXCHANGE_GAME_GOODS_RSP = 30,
	
	// 获取超值礼包信息
	GET_SUPER_VALUE_PACKAGE_REQ = 31,
	GET_SUPER_VALUE_PACKAGE_RSP = 32,
	
	// 领取超值礼包物品
	RECEIVE_SUPER_VALUE_PACKAGE_GIFT_REQ = 33,
	RECEIVE_SUPER_VALUE_PACKAGE_GIFT_RSP = 34,
	
	// 获取首冲礼包信息
	GET_FIRST_RECHARGE_PACKAGE_REQ = 35,
	GET_FIRST_RECHARGE_PACKAGE_RSP = 36,
	
	// 获取公共配置信息
	GET_GAME_COMMON_CONFIG_REQ = 37,
	GET_GAME_COMMON_CONFIG_RSP = 38,
	*/ 
};

// commonDB服务定义的协议ID值
enum CommonDBSrvProtocol
{
	BUSINESS_GET_MALL_CONFIG_REQ = 19,
	BUSINESS_GET_MALL_CONFIG_RSP = 20,

    BUSINESS_USER_RECHARGE_REQ = 35,
	BUSINESS_USER_RECHARGE_RSP = 36,
	
	// 获取游戏商城配置
	BUSINESS_GET_GAME_MALL_CONFIG_REQ = 51,
	BUSINESS_GET_GAME_MALL_CONFIG_RSP = 52,
	
	// 兑换游戏商品
	BUSINESS_EXCHANGE_GAME_GOODS_REQ = 53,
	BUSINESS_EXCHANGE_GAME_GOODS_RSP = 54,
	
	// 领取超值礼包物品
	BUSINESS_RECEIVE_SUPER_VALUE_PACKAGE_REQ = 55,
	BUSINESS_RECEIVE_SUPER_VALUE_PACKAGE_RSP = 56,
	
	// 获取目标用户的任意信息
	BUSINESS_GET_USER_OTHER_INFO_REQ = 57,
	BUSINESS_GET_USER_OTHER_INFO_RSP = 58,
	
	// 获取公共配置信息
	BUSINESS_GET_GAME_COMMON_CONFIG_REQ = 65,
	BUSINESS_GET_GAME_COMMON_CONFIG_RSP = 66,
	
	// 获取商城渔币信息
	BUSINESS_GET_MALL_FISH_COIN_INFO_REQ = 69,
	BUSINESS_GET_MALL_FISH_COIN_INFO_RSP = 70,
	
	// 韩文版HanYou平台获取优惠券奖励
	BUSINESS_ADD_HAN_YOU_COUPON_REWARD_REQ = 125,
	BUSINESS_ADD_HAN_YOU_COUPON_REWARD_RSP = 126,
};

// game db proxy 服务定义的协议ID值
enum GameServiceDBProtocol
{
	SET_GAME_DATA_REQ = 1,
	SET_GAME_DATA_RSP = 2,
	
	GET_GAME_DATA_REQ = 3,
	GET_GAME_DATA_RSP = 4,
	
    SET_GAME_RECORD_REQ = 5,
	SET_GAME_RECORD_RSP = 6,
	
	GET_GAME_RECORD_REQ = 7,
	GET_GAME_RECORD_RSP = 8,
	
	DEL_GAME_DATA_REQ = 9,
	DEL_GAME_DATA_RSP = 10,
	
	GET_MORE_KEY_GAME_DATA_REQ = 11,
	GET_MORE_KEY_GAME_DATA_RSP = 12,
};


typedef unordered_map<string, string> ParamKey2Value;

// http 请求头部数据
struct RequestHeader
{
	const char* method;
	unsigned int methodLen;
	
	const char* url;
	unsigned int urlLen;
	
	ParamKey2Value paramKey2Value;
};

// http 消息体数据
struct HttpMsgBody
{
	unsigned int len;
	
	ParamKey2Value paramKey2Value;
};


// 当乐平台参数配置
struct DownJoyConfig
{
	const char* host;
	const char* url;
	const char* innerMD5Key;
	
	// 安卓版本配置
	const char* appId;
	const char* appKey;
	const char* paymentKey;
	const char* paymentUrl;
	
	// 苹果版本配置
	const char* IOSappId;
	const char* IOSappKey;
	const char* IOSpaymentKey;
	const char* IOSpaymentUrl;
	
	const char* rechargeLogFileName;
	const char* rechargeLogFileSize;
	const char* rechargeLogFileCount;
	
	strIP_t ip;
	unsigned short port;
};

// 第三方平台参数配置
struct ThirdPlatformConfig
{
	const char* host;
	const char* url;

	const char* appId;
	const char* appKey;
	const char* paymentKey;
	const char* paymentUrl;

	strIP_t ip;
	unsigned short port;
};


// 挂接在连接上的相关数据
struct ConnectUserData
{
	unsigned int reqDataLen;
	char reqData[MaxRequestDataLen];  // 收到的请求数据
	
	unsigned int connIdLen;
	char connId[MaxConnectIdLen];     // 连接ID标识
};

// 连接服务状态
enum SrvStatus
{
	Stop = 0,
	Run = 1,
	Stopping = 2,
};

// 连接状态
enum ConnectStatus
{
	Normal = 0,
	Connecting = 1,
	TimeOut = 2,
	AlreadyWrite = 3,
	CanRead = 4,
	ConnectError = 5,
	SSLConnecting = 6,
	SSLConnectError = 7,
	DataError = 8,
};

// 用户挂接数据
struct UserKeyData
{
	unsigned int flag;
	char key[MaxKeySize];
	unsigned int srvId;
	unsigned short protocolId;
};

// 用户挂接的回调数据
struct UserCbData
{
	unsigned int flag;  // 0：默认内部管理内存； 1：外部挂接的内存由外部管理（创建&释放）
};

struct ConnectData
{
	// 用户挂接和连接相关的数据
	UserKeyData keyData;
	
	// 需要发送的数据
	char sendData[MaxNetBuffSize];
	unsigned int sendDataLen;
	unsigned int timeOutSecs;
	
	// 收到的数据
	char receiveData[MaxReceiveNetBuffSize];
	unsigned int receiveDataLen;
	unsigned int receiveDataIdx;

	int fd;
	unsigned int lastTimeSecs;
	int status;         // 连接状态
	
	SSL* sslConnect;    // https 连接操作对象
	
	const UserCbData* cbData;   // 用户挂接的数据
};


// http请求应答标志ID值
enum HttpReplyHandlerID
{
	// 安卓设备类型
	IdBeginValue = NProject::ThirdPlatformType::ClientDeviceBase,
    
    IdUpdateTXServerUserCurrencyData,       //更新腾讯服务器上的货币数据
    IdFindRechargeSuccess,                  //查询腾讯充值是否成功
    IddeductTXServerCurrency,               //扣除腾讯服务器上货币
    IdSendGift,								//赠送玩家礼物
    
	GetGoogleSDKAccessToken,                // 获取google访问api的token信息
	UpdateGoogleSDKAccessToken,             // 刷新google访问api的token信息
	CheckGoogleSDKPay,                      // 检查google付费操作
	
	RIDGetVivoOrderInfo,                    // 获取VIVO平台订单信息
	
	CheckAppleIOSPay,                       // 检查苹果版本付费操作
	CheckFacebookUserInfo,                  // 检查Facebook用户信息

	IdEndValue,
};


enum ItemDataFlag
{
	IType = 0,
	IId = 1,
	ICount = 2,
	IAmount = 3,
	ISize = 4,
};

// 充值标识
enum RechargeFlag
{
	First = 1,            // 首次充值
};


// 渔币充值第三方订单信息
struct ThirdOrderInfo
{
	unsigned int timeSecs;                   // 创建订单时间点
	
	char user[MaxUserNameLen];               // 用户名
	unsigned int srcSrvId;                   // 服务ID
	unsigned int platformId;                 // 平台ID

	unsigned int itemId;                     // 渔币ID
	unsigned int itemAmount;                 // 渔币总个数
	float money;                             // 支付总额，单位：元
	
	int userCbFlag;
};

typedef unordered_map<string, ThirdOrderInfo> FishCoinOrderIdToInfo;  // 渔币订单信息

}

#endif // TYPE_DEFINE_H
