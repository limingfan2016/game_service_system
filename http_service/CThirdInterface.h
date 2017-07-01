
/* author : limingfan
 * date : 2016.01.12
 * description : 第三方平台接口API操作实现
 */

#ifndef CTHIRD_INTERFACE_H
#define CTHIRD_INTERFACE_H

#include "SrvFrame/CModule.h"
#include "TypeDefine.h"
#include "base/CLogger.h"

using namespace NFrame;
using namespace NProject;

namespace NPlatformService
{

class CSrvMsgHandler;
class CThirdPlatformOpt;


// 消息协议处理对象
class CThirdInterface : public NFrame::CHandler
{
public:
	CThirdInterface();
	~CThirdInterface();
	
public:
    bool load(CSrvMsgHandler* msgHandler, CThirdPlatformOpt* platformatOpt);
	void unload();
	
public:
    // 渔币充值处理操作流程
    bool getTranscationData(const char* orderId, FishCoinOrderIdToInfo::iterator& it);
	void removeTransactionData(FishCoinOrderIdToInfo::iterator it);

	// 渔币充值，物品发放
	void provideRechargeItem(int result, FishCoinOrderIdToInfo::iterator thirdInfoIt, ThirdPlatformType pType = MaxPlatformType, const char* uid = "", const char* thirdOtherData = "");
	bool fishCoinRechargeItemReply(const com_protocol::UserRechargeRsp& userRechargeRsp);
	
	void checkUnHandleOrderData();

private:
    // M4399 Http消息处理
    bool handleM4399PaymentMsg(Connect* conn, const RequestHeader& reqHeaderData);
    bool handleM4399PaymentMsgImpl(const RequestHeader& reqHeaderData);
    
    // 魅族 Http消息处理
	// 返回值: 成功(200) 尚未发货(120013) 发货失败(120014) 未知异常(900000)
    bool handleMeiZuPaymentMsg(Connect* conn, const HttpMsgBody& msgBody);
    int  handleMeiZuPaymentMsgImpl(const HttpMsgBody& msgBody);
	
	// 当乐充值验证消息
    bool downJoyPaymentMsg(Connect* conn, const RequestHeader& reqHeaderData);
	bool downJoyPaymentMsgImpl(const ConnectUserData* ud, const RequestHeader& reqHeaderData);
    
private:
    //  处理魅族的订单请求
    void getThirdRechargeTransactionMeiZu(com_protocol::GetRechargeFishCoinOrderRsp &thirdRsp, const com_protocol::FishCoinInfo* fcInfo, const char *puid);
	
	// 获取vivo订单信息
	bool getVivoOrderInfo(const unsigned int srcSrvId, const char* orderId, const float price);
	bool getVivoOrderInfoReply(ConnectData* cd);
	
private:
	void checkAppleRechargeResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);  // 苹果版本充值验证
	bool checkAppleRechargeResultReply(ConnectData* cd);
	
	// 韩国平台 优惠券奖励处理
	bool handleHanYouCouponRewardMsg(Connect* conn, const HttpMsgBody& msgBody);
	bool handleHanYouCouponRewardMsgImpl(const char* connId, const HttpMsgBody& msgBody, string& content);
	void addHanYouCouponRewardReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// Facebook 用户信息
	void checkFacebookUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	bool checkFacebookUserReply(ConnectData* cd);

private:
	void getFishCoinRechargeOrder(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getFishCoinInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void cancelFishCoinRechargeOrder(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    void saveOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void removeOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    bool getThirdOrderId(const unsigned int platformType, const unsigned int osType, const unsigned int srcSrvId, char* transactionId, unsigned int len);  // 获取第三方平台订单ID
    const ThirdOrderInfo* saveTransactionData(const char* userName, const unsigned int srcSrvId, const unsigned int platformId,
							                  const char* transactionId, const com_protocol::GetRechargeFishCoinOrderRsp& thirdRsp);
    void removeTransactionData(const char* transactionId);

    bool parseUnHandleOrderData();
	void getUnHandleOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	bool parseOrderData(const char* transactionId, const char* transactionData, ThirdOrderInfo& transcationInfo);
	
	void registerHttpMsgHandler();

private:
    CSrvMsgHandler* m_msgHandler;
	CThirdPlatformOpt* m_platformatOpt;
	
	unsigned int m_orderIndex[ThirdPlatformType::MaxPlatformType];
	FishCoinOrderIdToInfo m_fishCoinOrder2Info;
	
	unsigned int m_baiduOrderIndex;  // 百度订单号索引，百度规定订单号长度不能超过11位
	
	
	/*
private:
    // M4399用户校验
    void checkM4399User(const ParamKey2Value& key2value, unsigned int srcSrvId);
    bool checkM4399UserReply(ConnectData* cd);
    
    //  魅族用户校验
    void checkMeiZuUser(const ParamKey2Value& key2value, unsigned int srcSrvId);
    bool checkMeiZuUserReply(ConnectData* cd);
    */
	
	
DISABLE_COPY_ASSIGN(CThirdInterface);
};

}

#endif // CTHIRD_INTERFACE_H
