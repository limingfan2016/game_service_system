
/* author : limingfan
 * date : 2017.10.20
 * description : 消息处理分模块分流
 */
 
#ifndef __COPT_LOGIC_HANDLER_H__
#define __COPT_LOGIC_HANDLER_H__

#include "SrvFrame/CModule.h"
#include "BaseDefine.h"


class COptMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace NDBOpt;
using namespace NErrorCode;


class COptLogicHandler : public NFrame::CHandler
{
public:
	COptLogicHandler();
	~COptLogicHandler();

public:
	bool init(COptMsgHandler* pMsgHandler);
	void unInit();
	
    void updateConfig();
	
	void online(OptMgrUserData* cud);
	void offline(OptMgrUserData* cud);
	
private:
	void getSelectHallPlayerInfoSQL(const com_protocol::GetBSHallPlayerInfoReq& req, char* sql, unsigned int& sqlLen);
	void getSelectHallNeedCheckUserInfoSQL(const com_protocol::GetBSHallPlayerInfoReq& req, char* sql, unsigned int& sqlLen);
	
	int addNewHallGame(OptMgrUserData* cud, SChessHallData* hallData, unsigned int gameType);
	int updateHallGameStatus(OptMgrUserData* cud, SChessHallData* hallData, unsigned int gameType, unsigned int status);
	int updateHallGameTaxRatio(OptMgrUserData* cud, SChessHallData* hallData, unsigned int gameType, float taxRatio);

    // 通知C端玩家用户，在棋牌室的状态变更
	void notifyChessHallPlayerStatus(const char* hallId, const char* username, com_protocol::EHallPlayerStatus status);
	
	// 获取对应时间点商城操作的总数额
    int getTimeMallOptSumValue(const char* hall_id, int opt, time_t timeSecs, double& sumValue);
	
	// 获取商城操作信息
    int getMallOptInfo(const char* hallId, int optType, int optStatus1, int optStatus2, unsigned int afterIdx, unsigned int limitCount,
					   google::protobuf::RepeatedPtrField<com_protocol::MallOptInfoRecord>& optInfoList, int& currentIdx);

	// 发送消息给当前所有在线的管理员
	void sendMessageToAllManager(const string& hallId, const google::protobuf::Message& msg, unsigned short protocolId, const char* logInfo = "");

    // B端APP管理员操作
private:	
	void checkManagerPhoneNumber(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void checkManagerPhoneNumberReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void createChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void createChessHallRechargeInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getChessHallInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getPlayerInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getGameInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void optHallGame(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void setHallGameTaxRatio(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void optPlayer(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void optPlayerReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void addManager(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 服务器（棋牌室管理员）主动通知玩家退出棋牌室（从棋牌室踢出玩家）
	void notifyPlayerQuitChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 商城操作
    void getMallData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getMallBuyInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getMallGiveInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void confirmBuyInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void confirmBuyInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void setPlayerInitGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void giveGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void giveGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 管理员获取玩家信息
	void getUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);


	// C端APP游戏玩家操作
private:
	// 玩家购买商城物品通知
    void buyMallGoodsNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 玩家申请加入棋牌室通知管理员
	void applyJoinChessHallNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 管理员在C端获取申请待审核的玩家列表
	void getApplyJoinPlayers(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 管理员在C端操作棋牌室玩家
	void managerOptHallPlayer(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void managerOptHallPlayerReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	COptMsgHandler* m_msgHandler;
	
	ManagerToPhoneNumberDataMap m_managerPhoneNumberData;
};


#endif // __COPT_LOGIC_HANDLER_H__
