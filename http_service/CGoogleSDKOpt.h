
/* author : limingfan
 * date : 2015.10.28
 * description : Google SDK 操作实现
 */

#ifndef CGOOGLE_SDK_OPT_H
#define CGOOGLE_SDK_OPT_H

#include "SrvFrame/CModule.h"
#include "TypeDefine.h"
#include "base/CLogger.h"


using namespace NFrame;

namespace NPlatformService
{

class CSrvMsgHandler;
class CThirdPlatformOpt;


// token信息
struct TokenInfo
{
	strIP_t checkPayIp;
	unsigned short checkPayPort;

	strIP_t tokenIp;
	unsigned short tokenPort;
	unsigned int lastSecs;
	string accessToken;
};

// 查询购买挂接的数据信息
static const unsigned int CbDataMaxSize = 256;
struct CheckPayInfo : public UserCbData
{
	char rechargeTransaction[CbDataMaxSize];
	char googleOrderId[CbDataMaxSize];
};

// 更新访问token挂接的数据信息
struct UpdateAccessTokenInfo : public UserCbData
{
	char rechargeTransaction[CbDataMaxSize];
	char googleOrderId[CbDataMaxSize];
	char packageName[CbDataMaxSize];
	char productId[CbDataMaxSize];
	char purchaseToken[CbDataMaxSize];
};

// 购买的物品信息
struct BuyInfo
{
	unsigned int itemType;
	unsigned int itemId;
	unsigned int itemCount;
	unsigned int itemAmount;
	
	unsigned int srcSrvId;
};

typedef unordered_map<string, BuyInfo> RechargeTranscationToBuyInfo;


// 消息协议处理对象
class CGoogleSDKOpt : public NFrame::CHandler
{
public:
	CGoogleSDKOpt();
	~CGoogleSDKOpt();
	
public:
    bool load(CSrvMsgHandler* msgHandler, CThirdPlatformOpt* platformatOpt);
	void unload();
	
public:
    bool googleRechargeItemReply(const com_protocol::UserRechargeRsp& userRechargeRsp);

private:
    void getRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void checkRechargeResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);  // 老版本校验方式
	
	// 新版本校验方式
	void checkRechargeResultEx(const com_protocol::OverseasRechargeResultCheckReq& msg, unsigned int srcSrvId);
	void checkRechargeResultExImpl(const char* keyData, unsigned int srvId, const char* rechargeTransaction, const char* googleOrderId, const char* packageName, const char* productId, const char* purchaseToken);
	bool checkRechargeResultExReply(ConnectData* cd);
	
	bool checkFishCoinRechargeResultReply(ConnectData* cd);
	
private:
	bool getAccessToken(unsigned int flag, unsigned int srvId = 0, const char* keyData = NULL, const UserCbData* cbData = NULL, unsigned int len = 0);
	bool getAccessTokenReply(ConnectData* cd);
	
	bool updateAccessTokenReply(ConnectData* cd);
	bool updateAccessTokenInfo(const char* info);
	
private:
	// NCommon::CLogger& getRechargeLogger();


private:
    CSrvMsgHandler* m_msgHandler;
	CThirdPlatformOpt* m_platformatOpt;
	
	TokenInfo m_tokenInfo;
	
	RechargeTranscationToBuyInfo m_recharge2BuyInfo;
	unsigned int m_rechargeTransactionIdx;
	

DISABLE_COPY_ASSIGN(CGoogleSDKOpt);
};

}

#endif // CGOOGLE_SDK_OPT_H
