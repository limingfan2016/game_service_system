
/* author : limingfan
 * date : 2015.11.05
 * description : 第三方平台操作实现
 */

#ifndef CTHIRD_PLATFORM_OPT_H
#define CTHIRD_PLATFORM_OPT_H

#include "SrvFrame/CModule.h"
#include "TypeDefine.h"
#include "base/CLogger.h"
#include "CThirdInterface.h"


using namespace NFrame;

namespace NPlatformService
{

struct ThirdTranscationInfo : public ThirdOrderInfo
{
	unsigned int itemType;                   // 物品类型：0金币；1道具
	unsigned int itemCount;                  // 购买的物品份数
};

typedef unordered_map<string, ThirdTranscationInfo> ThirdTranscationIdToInfo;  // 订单信息

// 腾讯充值信息
struct TXRetInfo : public UserCbData
{
    int platformID;         //平台ID
    char openid[128];
    char openkey[256]; 
    char pf[128]; 
    char pfkey[128];
    char billno[64];          //订单ID
    char userData[36];          //用户
    unsigned int srcSrvId;
    int nPayState;              //用户购买状态 0表示已发货  -1表示已经支付，但没有发货(需继续查询)
    int timerId;                //定时器ID
    int remainCount;            //剩余查询次数
};

// 腾讯充值后查询流程状态
enum ETXRechargeStatus
{
	EFinish = 0,              // 完成
	EContinueQuery = 1,       // 继续查询
	EDeductTXCoin = 2,        // 扣除腾讯托管的游戏币
};


class CSrvMsgHandler;


// 消息协议处理对象
class CThirdPlatformOpt : public NFrame::CHandler
{
public:
	CThirdPlatformOpt();
	~CThirdPlatformOpt();
	
public:
    bool load(CSrvMsgHandler* msgHandler);
	void unload();
	
public:
	const ThirdPlatformConfig* getThirdPlatformCfg(unsigned int type);
	NCommon::CLogger& getThirdRechargeLogger(const unsigned int platformId);
	CThirdInterface& getThirdInstance();
	
public:
	bool thirdPlatformRechargeItemReply(const com_protocol::UserRechargeRsp& userRechargeRsp);
	void checkUnHandleTransactionData();
	
	void getFishCoinInfoReply(const com_protocol::GetMallFishCoinInfoRsp& getMallFishCoinInfoRsp);


    // 腾讯充值查询操作
public:
	//1)更新腾讯服务器上游戏币
    bool updateTXServerUserCurrencyData(ThirdPlatformType platformID, const char* openid, const char *openkey, const char *pf, 
                                        const char *pfkey, unsigned int srcSrvId, const char *orderID);

private:

    //2)查询 腾讯服务器用户游戏币数据
    bool findTXServerUserCurrencyData(unsigned int srcSrvId, HttpReplyHandlerID replyHandlerID, TXRetInfo *pRetData);
	
    //3)处理 更新腾讯服务器上游戏币
    bool handleUpdateTXServerUserCurrencyData(ConnectData* cd);
	
	//4)应答第三充值交易(腾讯)
    void answerThirdRechargeTransaction(int nCode, const TXRetInfo* txRet, const int txBalance = 0);
	
	
	// 充值成功后的操作流程
    //5)查询充值成功请求
    void findRechargeSuccessReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	//6)处理  腾讯充值是否成功
    bool handleFindRechargeSuccess(ConnectData* cd);
	
	//7)充值(腾讯)
    int txRecharge(ConnectData* cd, int& deductTxBalance);

    //8)定时查询腾讯充值是否成功
    void onFindRechargeSuccess(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	 //9)扣除腾讯服务器上的游戏币
    bool deductTXServerCurrency(const HttpReplyHandlerID replyHandlerID, const unsigned int deductValue, const TXRetInfo* pRetData);

    //10)扣除腾讯托管的渔币
    void onDeductTXServerCurrency(unsigned int timerId, int userId, void* param, unsigned int remainCount);
 
    //11)处理 扣除腾讯服务器上的游戏币
	bool handleDeductTXServerCurrencyMsg(ConnectData* cd);
	
    //生成腾讯的签名
    string produceTXSig(const char *pUri, const map<string, string> &parameter, const char *pAppKey);

private:	
    // 小米平台Http消息处理
	bool handleXiaoMiPaymentMsg(Connect* conn, const RequestHeader& reqHeaderData);
	bool handleXiaoMiPaymentMsgImpl(const RequestHeader& reqHeaderData);
	
    // 豌豆荚平台Http消息处理
	bool handleWanDouJiaPaymentMsg(Connect* conn, const HttpMsgBody& msgBody);
	bool handleWanDouJiaPaymentMsgImpl(const HttpMsgBody& msgBody);
	
private:
    //  奇虎360平台http消息处理
 	bool handleQiHu360PaymentMsg(Connect* conn, const RequestHeader& reqHeaderData);
	bool handleQiHu360PaymentMsgImpl(const RequestHeader& reqHeaderData); 

	// UC平台http消息处理
	bool handleUCPaymentMsg(Connect* conn, const HttpMsgBody& msgBody);
	bool handleUCPaymentMsgImpl(const HttpMsgBody& msgBody);
    
    // OPPO平台http消息处理
	bool handleOppoPaymentMsg(Connect* conn, const HttpMsgBody& msgBody);
	bool handleOppoPaymentMsgImpl(const HttpMsgBody& msgBody);
    
    // 木蚂蚁平台http消息处理
    bool handleMuMaYiPaymentMsg(Connect* conn, const HttpMsgBody& msgBody);
	bool handleMuMaYiPaymentMsgImpl(const HttpMsgBody& msgBody);
    
    // 安智
    bool handleAnZhiPaymentMsg(Connect* conn, const HttpMsgBody& msgBody);
	bool handleAnZhiPaymentMsgImpl(const HttpMsgBody& msgBody);
	
private:
    // 客户端请求消息
	void getThirdRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void cancelThirdRechargeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 第三方用户校验
    void checkThirdUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
    // 豌豆荚用户校验
	void checkWanDouJiaUser(const ParamKey2Value& key2value, unsigned int srcSrvId);
	bool checkWanDouJiaUserReply(ConnectData* cd);
	
	// 小米用户校验
	void checkXiaoMiUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	bool checkXiaoMiUserReply(ConnectData* cd);
	
private:
    bool saveTransactionData(const char* userName, const unsigned int srcSrvId, const unsigned int platformId,
							 const char* transactionId, const com_protocol::GetThirdPlatformRechargeTransactionRsp& thirdRsp);

	bool parseTransactionData(const char* transactionId, const char* transactionData, ThirdTranscationInfo& transcationInfo);
	bool parseUnHandleTransactionData();
	
	bool getThirdTransactionId(unsigned int platformType, unsigned int osType, char* transactionId, unsigned int len);  // 获取第三方平台订单ID
    bool getTranscationIdToInfo(const string &TranscationId, ThirdTranscationIdToInfo::iterator &it);

    void removeTransactionData(const char* transactionId);
	void removeTransactionData(ThirdTranscationIdToInfo::iterator it);
	
	void registerHttpMsgHandler();
	
	// 充值发放物品
	void provideRechargeItem(int result, ThirdTranscationIdToInfo::iterator thirdInfoIt, ThirdPlatformType pType = MaxPlatformType, const char* uid = "", const char* thirdOtherData = "");

	
private:
    CSrvMsgHandler* m_msgHandler;
	
	ThirdPlatformConfig m_thirdPlatformCfg[ThirdPlatformType::MaxPlatformType];
	unsigned int m_transactionIndex[ThirdPlatformType::MaxPlatformType];
	ThirdTranscationIdToInfo m_thirdTransaction2Info;
	
	const char* m_thirdName[ThirdPlatformType::MaxPlatformType];
	NCommon::CLogger* m_thirdLogger[ThirdPlatformType::MaxPlatformType];
	
	CThirdInterface m_thirdInterface;
	
	
	/*
private:    
    // 360 用户校验
    void checkQiHu360User(const ParamKey2Value& key2value, unsigned int srcSrvId);
    bool checkQiHu360UserReply(ConnectData* cd);
	
    //  Oppo用户校验
    bool checkOppoUserReply(ConnectData* cd);
    void checkOppoUser(const ParamKey2Value& key2value, unsigned int srcSrvId);
    
    //  木蚂蚁用户校验
    bool checkMuMaYiUserReply(ConnectData* cd);
    void checkMuMaYiUser(const ParamKey2Value& key2value, unsigned int srcSrvId);

    // 搜狗用户校验
    bool checkSoGouUserReply(ConnectData* cd);
    void checkSoGouUser(const ParamKey2Value& key2value, unsigned int srcSrvId);

    // 应用宝QQ用户校验
    bool checkTXQQUserReply(ConnectData* cd);
    void checkTXQQUser(const ParamKey2Value& key2value, unsigned int srcSrvId);

    // 应用宝W微信用户校验
    bool checkTXWXUserReply(ConnectData* cd);
    void checkTXWXUser(const ParamKey2Value& key2value, unsigned int srcSrvId);
    
    //  安智用户校验
    bool checkAnZhiUserReply(ConnectData* cd);
    void checkAnZhiUser(const ParamKey2Value& key2value, unsigned int srcSrvId);
	
private:
	// 有玩平台充值消息处理
	bool handleYouWanPaymentMsg(Connect* conn, const HttpMsgBody& msgBody);
	bool handleYouWanPaymentMsgImpl(const HttpMsgBody& msgBody);
	bool getYouWanTransactionId(unsigned int osType, char* transactionId, unsigned int len);  // 获取有玩平台订单ID
	
	// 劲牛平台充值消息处理
	bool handleJinNiuPaymentMsg(Connect* conn, const HttpMsgBody& msgBody);
	bool handleJinNiuPaymentMsgImpl(const HttpMsgBody& msgBody);
	
	// 应用宝
	bool handleTXPaymentMsg(Connect* conn, const HttpMsgBody& msgBody);
	bool handleTXPaymentMsgImpl(const HttpMsgBody& msgBody);
    */
	

DISABLE_COPY_ASSIGN(CThirdPlatformOpt);
};

}

#endif // CTHIRD_PLATFORM_OPT_H
