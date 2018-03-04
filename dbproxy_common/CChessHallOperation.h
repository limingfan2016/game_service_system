
/* author : limingfan
 * date : 2017.09.15
 * description : 棋牌室操作实现
 */
 
#ifndef __CCHESS_HALL_OPERATION_H__
#define __CCHESS_HALL_OPERATION_H__

#include "SrvFrame/CModule.h"
#include "BaseDefine.h"
#include "../common/CRandomNumber.h"


class CMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace NDBOpt;
using namespace NErrorCode;


class CChessHallOperation : public NFrame::CHandler
{
public:
	CChessHallOperation();
	~CChessHallOperation();
	
public:
	int init(CMsgHandler* pMsgHandler);
	void unInit();
    void updateConfig();

public:
    // 获取用户所有棋牌室基本信息
	int getUserChessHallBaseInfo(const char* username, ::google::protobuf::RepeatedPtrField<com_protocol::ClientHallBaseInfo>& baseInfos);
	
	// 获取用户所在的棋牌室当前所有信息
	int getUserChessHallInfo(const char* username, const char* hallId, unsigned int max_online_rooms, com_protocol::ClientHallInfo& hallInfo,
							 ::google::protobuf::RepeatedPtrField<com_protocol::BSGiveGoodsInfo>* giveGoodsInfo = NULL, double gameGold = 0.00);
	
	// 获取用户金币赠送信息
	int getUserGiveGoodsInfo(const char* username, const char* hallId, ::google::protobuf::RepeatedPtrField<com_protocol::BSGiveGoodsInfo>& giveGoodsInfo, double gameGold);
	
	// 玩家申请加入棋牌室
	int applyJoinChessHall(const char* username, const string& hall_id, const string& explain_msg, unsigned int serviceId);
	
	// 获取棋牌室基本信息
	int getChessHallBaseInfo(const char* hall_id, com_protocol::ClientHallBaseInfo& baseInfo, const char* username);
	
	// 获取棋牌室游戏信息
    int getChessHallGameInfo(const char* hall_id, google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>& gameInfoList);
	
	// 获取棋牌室当前在线房间信息
    int getChessHallOnlineRoomInfo(const char* hall_id, com_protocol::ClientHallGameInfo& gameInfo,
	                               unsigned int afterIdx, unsigned int count, StringVector& roomIds);
	int getChessHallOnlineRoomInfo(const char* hall_id, google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>& gameInfoList,
                                   unsigned int afterIdx, unsigned int count, StringVector& roomIds);
	
	// 获取在线游戏房间的用户信息
	int getChessHallOnlineUserInfo(const char* hall_id, const StringVector& roomIds, OnlineUserMap& onlineUsers);
    int getChessHallOnlineUserInfo(const char* hall_id, const StringVector& roomIds,
                                   google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>& gameInfoList);
	
	// 获取棋牌室里的玩家人数
    unsigned int getPlayerCount(const char* hall_id);
	
	// 获取棋牌室用户状态信息
    com_protocol::EHallPlayerStatus getChessHallPlayerStatus(const char* hallId, const char* username);
	
	// 获取当前时间点棋牌室排行
    int getCurrentHallRanking(const char* hall_id, SHallRankingVector& hallRanking);
	int getCurrentHallRanking(const char* hall_id, google::protobuf::RepeatedPtrField<com_protocol::ClientHallRanking>& hallRanking);
	
	// 获取玩家所在的房间
	com_protocol::ClientHallRoomInfo* getPlayerRoom(com_protocol::ClientHallGameInfo& gameInfo, const char* roomId);
    com_protocol::ClientHallRoomInfo* getPlayerRoom(google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>& gameInfoList, unsigned int gameType, const char* roomId);
	
	// 获取用户所在棋牌室的管理员级别
	// 当前玩家用户也有可能是该棋牌室的管理员
	int getUserManageLevel(const char* username, const char* hallId, int& manageLevel);
	
	// 获取棋牌室管理员ID
	int getManagerIdList(const char* hallId, google::protobuf::RepeatedPtrField<string>& managerIds);

private:
    bool initHallGameRoomIdRandomNumber();
	
	// 获取棋牌室不重复的随机游戏房间ID
    unsigned int getHallGameRoomNoRepeatRandID();
	

	// B端棋牌室管理操作
private:
    // 创建棋牌室充值信息
    void createHallRechargeInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 棋牌室管理员操作玩家信息
    void managerOptPlayer(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 管理员确认商场购买信息
	void confirmMallBuyInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 管理员赠送商城物品给玩家
	void giveMallGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 棋牌室管理员充值
	void chessHallManagerRecharge(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	

    // C端玩家用户操作
private:	
	void applyIntoChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void cancelApplyChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 创建棋牌室房间
	void createHallGameRoom(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 修改棋牌室游戏房间状态
	void changeHallGameRoomStatus(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    
	// 修改棋牌室游戏房间玩家座位号
	void changeGameRoomPlayerSeat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	// 玩家购买商城物品
	void buyMallGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 玩家获取商城购买记录
	void getMallBuyRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 管理员赠送物品之后，玩家确认收货
	void confirmGiveGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);


private:
    // 获取棋牌室基本信息
	void getHallBaseInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取棋牌室当前所有信息
	void getChessHallInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取棋牌室房间列表
	void getHallRoomList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
    // 获取棋牌室当前排名列表
	void getHallRanking(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 获取棋牌室用户状态信息
	void getHallPlayerStatus(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 获取棋牌室游戏房间信息
	void getHallGameRoomInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	CMsgHandler* m_msgHandler;
	
	CRandomNumber m_hallGameRoomIdRandomNumbers;  // 棋牌室游戏房间ID随机数
};


#endif // __CCHESS_HALL_OPERATION_H__
