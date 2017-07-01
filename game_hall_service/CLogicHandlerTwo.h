
/* author : limingfan
 * date : 2016.08.11
 * description : 逻辑数据处理
 */
 
#ifndef _CLOGIC_LOGIC_HANDLER_TWO_H_
#define _CLOGIC_LOGIC_HANDLER_TWO_H_

#include "base/MacroDefine.h"
#include "SrvFrame/CModule.h"
#include "TypeDefine.h"


using namespace NCommon;
using namespace NFrame;

namespace NPlatformService
{

class CSrvMsgHandler;
struct RedGiftRewardData;


class CLogicHandlerTwo : public NFrame::CHandler
{
public:
	CLogicHandlerTwo();
	~CLogicHandlerTwo();

public:
	void load(CSrvMsgHandler* msgHandler);
	void unLoad();

	void updateConfig();  // 配置文件更新
	
public:
    void checkRedGiftCondition(const unsigned int type, const unsigned int value);
	
	void receiveRedGiftRewardReply(const int result, ConnectProxy* conn);
	
private:
    // 消息中心转发过来的通知消息
    void gameForwardMessageNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 红包口令相关
	void getRedGiftInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getRedGiftInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void receiveRedGiftReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void receiveRedGiftReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void viewRedGiftRewardReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void viewRedGiftRewardReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void receiveRedGiftRewardReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void addOfflineUserRedGiftReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void addRedGiftFriendStaticDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void addRedGiftFriendLogicDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    // 背包相关
	void getKnapsackInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getKnapsackInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 踢玩家下线，强制玩家下线
	void forceUserQuitNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 绑定兑换商城玩家账号信息
	void bindExchangeMallInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void bindExchangeMallInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	

	// 获取邮件信息
	void getMailMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getMailMessageReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 操作邮件信息
	void optMailMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void optMailMessageReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	// 获取比赛场信息
	void getArenaInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getDetailedMatchInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getArenaRankingInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getUsersInfoForArenaRanking(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void updateArenaMatchConfigData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getArenaRankingRewardReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 获取捕鱼卡牌服务信息
    void getBuyuPokerCarkServiceInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void sendMatchArenaInfo(ConnectProxy* conn = NULL, const com_protocol::BuyuPokerCarkInfo* pokerCarkInfo = NULL); // 发送比赛场信息
	
private:
    void sendRedGiftInfoToClient(const com_protocol::HallLogicData* hallLogicData, const char* userName = NULL);
	
	void addRedGiftReward(com_protocol::HallLogicData* hallLogicData, const RedGiftRewardData* rgRD);
	void addRedGiftReward(const char* data, const unsigned int len);
	
	bool addRedGiftFriend(com_protocol::HallLogicData* hallLogicData, const char* friendName);
	void makeToFriend(ConnectUserData* cud, const char* friendName);
	
	void addArenaRankingInfo(const com_protocol::GetManyUserOtherInfoRsp& userInfoRsp, const com_protocol::GetArenaRankingInfoRsp& rankingInfoRsp, const int time_id, com_protocol::ClientDetailedMatchInfo* matchInfo);
	
	// 转发消息到游戏大厅
    void forwardMessageToLogin(const char* username, const unsigned int uLen, const char* data, const unsigned int dLen, unsigned short dstProtocol, unsigned short srcProtocolId = 0);
	
private:
	CSrvMsgHandler* m_msgHandler;
	
	int m_updateArenaConfigDataCurrentDay;                   // 当前天数
	unordered_map<int, int> m_isNeedUpdateArenaConfigData;   // 是否需要更新比赛场配置数据
	

DISABLE_COPY_ASSIGN(CLogicHandlerTwo);
};

}

#endif // _CLOGIC_LOGIC_HANDLER_TWO_H_
