
/* author : limingfan
 * date : 2017.11.01
 * description : 消息处理辅助类
 */
 
#ifndef __CHALL_LOGIC_HANDLER_H__
#define __CHALL_LOGIC_HANDLER_H__

#include "SrvFrame/CModule.h"
#include "TypeDefine.h"


using namespace NCommon;
using namespace NFrame;
using namespace NDBOpt;
using namespace NErrorCode;

namespace NPlatformService
{

class CSrvMsgHandler;

class CHallLogicHandler : public NFrame::CHandler
{
public:
	CHallLogicHandler();
	~CHallLogicHandler();
	
public:
	int init(CSrvMsgHandler* pMsgHandler);
	void unInit();

    void updateConfig();
    
    void onLine(HallUserData* cud, ConnectProxy* conn, const com_protocol::DetailInfo& userDetailInfo);
	void offLine(HallUserData* cud);
	
private:
    unsigned int getCardMaxRate(const com_protocol::CattleRoomInfo& cattleRoomInfo);        // 获取房间最大倍率
	unsigned int getEnterRoomMinGold(const com_protocol::CattleRoomInfo& cattleRoomInfo);   // 最小准入金币
	int setRoomBaseInfo(const HallUserData* cud, com_protocol::ChessHallRoomInfo& roomInfo);

private:
	void applyJoinChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void applyJoinChessHallReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void cancelApplyChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void cancelApplyChessHallReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 通知C端玩家用户，在棋牌室的状态变更
	void chessHallPlayerStatusNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 服务器（棋牌室管理员）主动通知玩家退出棋牌室（从棋牌室踢出玩家）
	void playerQuitChessHallNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    // 玩家创建棋牌室游戏房间
	void createHallGameRoom(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void createHallGameRoomReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    // 获取商城信息
	void getMallInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 玩家购买商城物品
	void buyMallGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void buyMallGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 玩家获取商城购买记录
	void getMallBuyRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getMallBuyRecordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	void playerChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void gameMsgNoticeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getHallMsgInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    void modifyUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void modifyUserInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	void playerReceiveInvitationNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void refusePlayerInvitationNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    
private:
    void getGameRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void getGameRecordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
    void getDetailedGameRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getDetailedGameRecordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    void checkUserPhoneNumber(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void getUserPhoneNumberReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void checkUserPhoneNumberReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
    void checkMobileVerificationCode(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void setUserPhoneNumberReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 管理员在C端获取申请待审核的玩家列表
	void getApplyJoinPlayers(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getApplyJoinPlayersReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 管理员在C端操作棋牌室玩家
	void managerOptHallPlayer(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void managerOptHallPlayerReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    void getHallPlayerList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getHallPlayerListReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	CSrvMsgHandler* m_msgHandler;
	
	HallMessageInfoMap m_hallMsgInfoMap;
    
    UserIdToPhoneNumberDataMap m_setPhoneNumberInfoMap;
};

}

#endif // __CHALL_LOGIC_HANDLER_H__
