
/* author : limingfan
 * date : 2015.12.09
 * description : 商城管理
 */

#ifndef CMALL_H
#define CMALL_H

#include "SrvFrame/CModule.h"
#include "base/MacroDefine.h"
#include "TypeDefine.h"


namespace NPlatformService
{

class CSrvMsgHandler;

/*
// 充值物品大类型
enum RechargeItemType
{
	Gold = 0,      // 金币
	Item = 1,      // 道具
};
*/


// 消息协议响应对象数据
class CMall : public NFrame::CHandler
{

public:
	CMall();
	~CMall();
	
public:
    bool load(CSrvMsgHandler* msgHandler);
	void unload();
	
private:
    void getMallConfig(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void getMallConfigReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

public:
    bool getItemInfo(const int type, com_protocol::ItemInfo& itemInfo);
	
/*
	unsigned int getItemFlag(const char* userName, const unsigned int userNameLen, const unsigned int id);

public:
    const com_protocol::FishCoinInfo* getFishCoinInfo(const unsigned int id);
	bool fishCoinIsRecharged(const char* userName, const unsigned int len, const unsigned int id);
	
private:
    // 新版本商城配置信息
    void getGameMallConfig(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void getGameMallConfigReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 获取首冲礼包信息
	void getFirstRechargeInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 兑换商城物品
	void exchangeGameGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void exchangeGameGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    
	// 获取公共配置
	void getGameCommonConfig(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getGameCommonConfigReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取超值礼包信息
	void getSuperValuePackageInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 领取超值礼包物品
	void receiveSuperValuePackageGift(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void receiveSuperValuePackageGiftReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
*/	


private:
    CSrvMsgHandler* m_msgHandler;
	com_protocol::GetMallConfigRsp m_mallCfg;
	
	// unsigned int m_firstRechargeItemId;
	// com_protocol::GetGameMallCfgRsp m_gameMallCfg;
    

DISABLE_COPY_ASSIGN(CMall);
};


}

#endif // CMALL_H
