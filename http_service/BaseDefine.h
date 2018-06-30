
/* author : limingfan
 * date : 2015.04.23
 * description : 类型定义
 */

#ifndef __BASE_DEFINE_H__
#define __BASE_DEFINE_H__

#include <string>
#include <unordered_map>
#include "base/Type.h"
#include "base/CLogger.h"
#include "common/CommonType.h"
#include "common/HttpOperationProtocolId.h"
#include "common/DBProxyProtocolId.h"
#include "common/HttpOperationErrorCode.h"
#include "common/CServiceOperation.h"
#include "http_base/HttpBaseDefine.h"

#include "_NHttpOperationConfig_.h"
#include "protocol/appsrv_http_operation.pb.h"


using namespace std;
using namespace NProject;
using namespace NHttp;

namespace NPlatformService
{

// 常量定义
static const unsigned int MaxRechargeOrderLen = 256;
static const unsigned int MinRechargeLogFileSize = 1024 * 1024 * 8;
static const unsigned int MinRechargeLogFileCount = 10;

static const unsigned int Md5BuffSize = 36;
static const unsigned int Md5ResultLen = 32;

static const unsigned int MaxCbDataLen = 128;


// http请求应答标志ID值
enum EHttpReplyHandlerID
{
	ERequestIdBeginValue = 10000,

    EGetWeiXinTokenInfoId,                      // 获取微信登录token等信息
	EUpdateWeiXinTokenInfoId,                   // 刷新微信登录token信息
	EGetWeiXinUserInfoId,                       // 获取微信用户信息
    
    ESendMobileVerificationCodeId,              // 发送手机验证码
	ESendMobilePhoneMessageId,                  // 发送手机通知短信息
	
	ERequestIdEndValue,
};


// 平台ID对应的数据日志
typedef unordered_map<unsigned int, NCommon::CLogger*> IdToDataLogger;

// 平台ID对应的订单号当前索引值
typedef unordered_map<unsigned int, unsigned int> IdToOrderIndex;

// 第三方订单信息
struct ThirdOrderInfo
{
	unsigned int timeSecs;                   // 创建订单时间点
	
	char user[IdMaxLen];                     // 请求订单的玩家用户名
	char hallId[IdMaxLen];                   // 请求订单的棋牌室ID
	unsigned int srcSrvId;                   // 请求订单的服务ID
	unsigned int platformId;                 // 该订单所属的第三方平台ID

	unsigned int itemId;                     // 订单购买的物品ID
	unsigned int itemAmount;                 // 订单购买的物品个数
	double money;                            // 订单需要支付的总金额，单位：元
};

// 订单ID对应的订单信息
typedef unordered_map<string, ThirdOrderInfo> OrderIdToInfo;


/*
// 微信token数据，存储在中心redis DB（现阶段先不存储在DB，只存储在内存中，后续优化可存储在redis DB）
// key 为 platformtype.unionid
struct SWXTokenData
{
	unsigned int timeOutSecs;                // 超时时间点
	
	unsigned int refreshTokenLen;
	char refreshToken[MaxCbDataLen];
};
*/


// https请求调用微信接口，本地存储回调数据
struct SWXReqestCbData : public UserCbData
{
	unsigned int platformType;

	unsigned int cbDataLen;
	char cbData[MaxCbDataLen];
	
	unsigned int userIdLen;
	char userId[MaxCbDataLen];
};


// 微信登录用户信息
struct SWXLoginInfo
{
	unsigned int refreshTokenTimeOutSecs;  // refresh token 超时时间点
	unsigned int accessTokenTimeOutSecs;   // access token 超时时间点

	string refreshToken;
	string accessToken;
	string openid;
};

// 微信用户唯一 ID unionid 对应的用户登录信息
// key 为 platformtype.unionid
typedef unordered_map<string, SWXLoginInfo> WXUIDToLoginInfoMap;

}


#endif // __BASE_DEFINE_H__
