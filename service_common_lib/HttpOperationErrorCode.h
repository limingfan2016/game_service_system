
/* author : limingfan
 * date : 2017.09.20
 * description : HTTP操作服务错误码定义
 */
 
#ifndef __HTTP_OPT_ERROR_CODE_H__
#define __HTTP_OPT_ERROR_CODE_H__


// HTTP操作服务错误码 6001 - 6999
enum EHttpOperationErrorCode
{
	HOPTWeiXinLoginInvalidParameter = 6001,   // 微信登录无效参数（调用端需重传有效的code）
	HOPTNoFoundItemInfo = 6002,               // 找不到物品信息
	HOPTGetOrderIdFailed = 6003,              // 获取订单ID失败
	HOPTSaveOrderDataError = 6004,            // 保持订单数据错误
	HOPTRechargeFailed = 6005,                // 充值失败
	
	HOPTGetWeiXinUserInfoError = 6006,        // 获取微信用户基本信息失败
	HOPTGetWeiXinRefreshTokenError = 6007,    // 获取微信刷新token失败
	HOPTWeiXinRefreshTokenTimeOut = 6008,     // 微信刷新token已超时（调用端需重传有效的code）
	HOPTUpdateWeiXinAccessTokenError = 6009,  // 刷新微信access token失败
	HOPTGetWeiXinTokenInfoError = 6010,       // 获取微信token信息失败
	HOPTGetWeiXinTokenInvalidCode = 6011,     // 无效的code获取微信token信息（调用端需重传有效的code）
	HOPTGetWeiXinTokenReplyError = 6012,      // 获取微信token信息http返回无效信息（调用端需重传有效的code）
	HOPTGetWeiXinUserInfoOpenIdError = 6013,  // 无效的openid获取用户信息（调用端需重传有效的code）
    
    HOPTSendCheckPhoneNumberInfoError = 6014, // 发送手机验证码信息失败
    HOPTCheckPhoneNumberReplyError = 6015,    // 发送手机验证码信息返回错误
	
	HOPTSendPhoneMessageError = 6016,         // 发送手机短信息通知失败
	HOPTSendPhoneMessageReplyError = 6017,    // 发送手机短信息通知返回错误
};


#endif // __HTTP_OPT_ERROR_CODE_H__

