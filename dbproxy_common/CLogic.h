
/* author : liuxu
* date : 2015.03.10
* description : 消息处理辅助类
*/
 
#ifndef __CLOGIC_H__
#define __CLOGIC_H__

#include <string>
#include "SrvFrame/CModule.h"
#include "db/CMySql.h"
#include "db/CMemcache.h"
#include "app_server_dbproxy_common.pb.h"
#include "base/CCfg.h"
#include "base/CLogger.h"
#include "MessageDefine.h"
#include "_DbproxyCommonConfig_.h"
#include "_MallConfigData_.h"


class CSrvMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace std;
using namespace NDBOpt;
using namespace NErrorCode;

// typedef vector<MallConfigData::gift_item> PayGiftVector;

class SItemInfo
{
public:
	uint32_t item_num;
	double buy_price;
	// PayGiftVector* giftVector;

	// SItemInfo(){item_num = 0; buy_price = 0.0; giftVector = NULL;}
};

class SExchangeInfo
{
public:
	uint32_t exchange_count;
	uint32_t exchange_get;
	char exchange_desc[256];
};

class CLogic : public NFrame::CHandler
{
public:
	CLogic();
	~CLogic();

	int init(CSrvMsgHandler *psrvMsgHandle);

	//消息处理
	int procRegisterUserReq(const com_protocol::RegisterUserReq &req, const uint32_t platform_type);
	int procModifyPasswordReq(const com_protocol::ModifyPasswordReq &req, const string &username);
	int procModifyBaseinfoReq(const com_protocol::ModifyBaseinfoReq &req, const string &username);
	void procModifyBaseinfoReq(const com_protocol::ModifyBaseinfoReq &req, const string &username, com_protocol::ModifyBaseinfoRsp &rsp);
	int procVeriyfyPasswordReq(const com_protocol::VerifyPasswordReq &req);
	int procGetUserBaseinfoReq(const com_protocol::GetUserBaseinfoReq &req, com_protocol::DetailInfo &detail_info);
	int procRoundEndDataChargeReq(const com_protocol::RoundEndDataChargeReq &req, uint64_t& gameGold);
	int procLoginNotifyReq(const com_protocol::LoginNotifyReq &req);
	int procLogoutNotifyReq(const com_protocol::LogoutNotifyReq &req);
	int procGetMultiUserInfoReq(const com_protocol::GetMultiUserInfoReq &req, com_protocol::GetMultiUserInfoRsp &rsp);

public:
    void updateConfig();
	
	CLogger& getDataLogger();

private:
	int getRegisterSql(const com_protocol::RegisterUserReq &req, string &sql);
	uint32_t addItemCount(CUserBaseinfo& user_baseinfo, unsigned int type, unsigned int amount);
	// uint32_t recharge(com_protocol::UserRechargeRsp &rsp, PayGiftVector* payGifts, char* giftInfo, unsigned int len);
	
	uint32_t rechargeFishCoin(const com_protocol::UserRechargeRsp& rsp, const com_protocol::FishCoinInfo* fcInfo, char* giftInfo, unsigned int len);
	
private:
    unsigned int getItemFlag(const unsigned int id);
    void getMallConfigReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);  // 获取商城配置
	
	void handldMsgUserRechargeReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void handldMsgExchangePhoneCardReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void handldMsgModifyExchangeGoodsStatusReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	// void handldMsgExchangeGoodsReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 直接重置密码
	void resetPasswordReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 查询用户ID
	void queryUserNameReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 查询用户相关信息
	void queryUserInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	int getRankingItemInfo(const com_protocol::GetRankingItemReq& req, com_protocol::GetRankingItemRsp& rsp);

private:
	CSrvMsgHandler *m_psrvMsgHandler;

	unordered_map<uint32_t, SItemInfo> m_mitem_info;	//RMB充值
	unordered_map<uint32_t, SExchangeInfo> m_mechange_info;	//兑换
	unordered_map<uint32_t, uint32_t> m_exchangeCount2Get;	// 话费兑换消费多少额度或者张数，得到面值多少额度的手机话费

	com_protocol::GetMallConfigRsp m_get_mall_config_rsp;
	unsigned int m_firstRechargeItemId;
};

#endif // __CLOGIC_H__
