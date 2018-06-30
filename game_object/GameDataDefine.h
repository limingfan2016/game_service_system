
/* author : limingfan
 * date : 2018.03.28
 * description : 游戏数据类型定义
 */

#ifndef _GAME_DATA_DEFINE_H_
#define _GAME_DATA_DEFINE_H_

#include "common/CommonType.h"
#include "common/CServiceOperation.h"
#include "common/DBProxyProtocolId.h"
#include "protocol/game_object.pb.h"

#include "GameObjectErrorCode.h"


using namespace NProject;

namespace NPlatformService
{

// 挂接在连接上的相关数据
struct GameUserData : public ConnectProxyUserData
{
	// 玩家登录的棋牌室信息
	// 玩家可以旁观未通过审核的棋牌室
	char hallId[IdMaxLen];                   // 玩家登录的棋牌室ID
	char roomId[IdMaxLen];                   // 玩家登录的棋牌室房间ID
	short seatId;                            // 玩家登录的棋牌室房间座位ID
	short inHallStatus;                      // 玩家在棋牌室的状态
};


// key ：玩家ID
// value ：
// 1）值大于0则表示发送了心跳消息的次数
// 2）值等于0则表示心跳检测失败，回调了心跳失败函数
typedef unordered_map<string, unsigned int> HeartbeatFailedTimesMap;  // 玩家心跳消息失败次数

// 房间里的玩家数据（旁观者、游戏玩家等），玩家ID到玩家信息映射
typedef unordered_map<string, com_protocol::ClientRoomPlayerInfo> RoomPlayerIdInfoMap;

// 等待超时的空房间，超时后则解散
typedef unordered_map<unsigned long long, unsigned int> WaitForDisbandRoomInfoMap;

// 玩家发起解散房间请求时间点
typedef unordered_map<string, unsigned int> PlayerDismissRoomTimeMap;


// 正在玩游戏中的玩家数据
struct SGamePlayerData
{
	bool isOnline;                             // 是否在线
	unsigned int betValue;                     // 下注额度
	int cardValue;                             // 牌型结果值，不同游戏不同类型牌型
	
	int winLoseValue;                          // 当前局输赢值
	int sumWinLoseValue;                       // 总输赢值

	bool isLastBanker;                         // 是否是上一局庄家
	bool isNewBanker;                          // 是否是当前新庄家

	inline void reset()
	{
		isOnline = true;
		betValue = 0;
		cardValue = 0;
		
		winLoseValue = 0;
		// sumWinLoseValue = 0;  // do not reset to 0
		
		isLastBanker = false;
		isNewBanker = false;
	}

	SGamePlayerData() : isOnline(true), betValue(0), cardValue(0), winLoseValue(0), sumWinLoseValue(0),
	                    isLastBanker(false), isNewBanker(false) {};
	virtual ~SGamePlayerData() {};
};
typedef unordered_map<string, SGamePlayerData> GamePlayerIdDataMap;  // 玩家ID映射数据


// 棋牌室单个房间数据
struct SRoomData
{
    unsigned int playTimes;                             // 游戏局数
	int roomStatus;                                     // 房间状态（准备中、游戏中）
	int gameStatus;                                     // 游戏状态
	
    unsigned int optLastTimeSecs;                       // 操作倒计时持续的时间长度（游戏玩家掉线重连回房间、拒绝解散房间等需要）
	unsigned int optTimeSecs;                           // 操作开始时间点
	unsigned int optTimerId;                            // 操作定时器ID
	unsigned int nextOptType;                           // 超时后下一个操作类型（解散房间被玩家拒绝之后恢复继续游戏使用）

	com_protocol::ChessHallRoomInfo roomInfo;           // 房间基本信息

    bool currentHasBanker;                              // 房间当前是否存在满足当庄的玩家
	RoomPlayerIdInfoMap playerInfo;                     // 房间内所有玩家的数据（包括观看玩家）
	
	com_protocol::SetRoomGameRecordReq roomGameRecord;  // 房间游戏结算记录
	
	PlayerDismissRoomTimeMap playerDismissRoomTime;     // 玩家发起解散房间请求时间点
	StringVector agreeDismissRoomPlayer;                // 同意解散房间的玩家
	unsigned int dismissRoomTimerId;                    // 解散房间定时器ID

	UserChatInfoMap chatInfo;                           // 房间内玩家的聊天限制信息
	

    // 房间的最小下注额
    virtual unsigned int getMinBetValue() const
	{
		return 0;
	}
	
	// 房间的最大下注额
    virtual unsigned int getMaxBetValue() const
	{
		return 0;
	}
	
	// 闲家需要的最低金币数量
	virtual unsigned int getPlayerNeedMinGold() const
	{
		return 0;
	}

	// 庄家需要的最低金币数量
	virtual unsigned int getBankerNeedMinGold() const
	{
		return 0;
	}
	
	// 进入房间需要的金币数量
	virtual unsigned int getEnterNeedGold() const
	{
		return 0;
	}
	
	// 最小游戏玩家人数
	virtual unsigned int getMinGamePlayers() const
	{
		return 2;
	}
	
	// 开桌人数
	virtual unsigned int getStartPlayers() const
	{
		return 0;
	}
	
	// 是否存在下一局新庄家
	virtual bool hasNextNewBanker() const
	{
		return true;
	}

    // 是否是正在游戏中的玩家
	virtual bool isOnPlayer(const string& username) const
	{
		return false;
	}
	
	// 心跳通知
	virtual bool onHearbeatStatusNotify(const string& username, const bool isOnline, int& newStatus)
	{
		return false;
	}
	
	// 重入房间
	virtual void onReEnter(const string& username)
	{
	}
	
	// 游戏记录信息
	virtual void writePlayerItemChangeInfo(const string& username, com_protocol::ItemChangeReq* itemChange)
	{
	}
	
	// 房卡付费
	virtual void payRoomCard(const char* hallId, const float payCount, com_protocol::MoreUsersItemChangeReq& itemChangeReq)
	{
	}


    // 是否正在解散房间中
    inline bool isOnDismissRoom() const
	{
		return (dismissRoomTimerId != 0 && !agreeDismissRoomPlayer.empty());
	}
	
    // 是否是游戏中
    inline bool isOnPlay() const
	{
		const unsigned int roomType = roomInfo.base_info().room_type();
		if (roomType == com_protocol::EHallRoomType::EGoldId || roomType == com_protocol::EHallRoomType::EGoldEnter)
		{
			return gameStatus > com_protocol::EGameStatus::EGameReady;  // 金币场
		}
		
		return playTimes > 0;  // 房卡场
	}
	
    // 是否是金币场房间
    inline bool isGoldRoom() const
	{
		const unsigned int roomType = roomInfo.base_info().room_type();
		return (roomType == com_protocol::EHallRoomType::EGoldId || roomType == com_protocol::EHallRoomType::EGoldEnter);
	}

	// 获取玩家信息
	inline const com_protocol::ClientRoomPlayerInfo* getPlayerInfo(const char* username) const
	{
		RoomPlayerIdInfoMap::const_iterator it = playerInfo.find(username);
		return (it != playerInfo.end()) ? &it->second : NULL;
	}
	
	// 获取玩家状态
	inline int getPlayerStatus(const char* username) const
	{
		RoomPlayerIdInfoMap::const_iterator it = playerInfo.find(username);
		return (it != playerInfo.end()) ? it->second.status() : -1;
	}
	
	// 获取状态玩家数量
	unsigned int getPlayerCount(const int status)
	{
		unsigned int count = 0;
		for (RoomPlayerIdInfoMap::const_iterator it = playerInfo.begin(); it != playerInfo.end(); ++it)
		{
			if (it->second.status() == status) ++count;
		}
		
		return count;
	}

	// 房间里座位上的玩家个数
	inline unsigned int getSeatedPlayers() const
	{
		unsigned int count = 0;
		for (RoomPlayerIdInfoMap::const_iterator it = playerInfo.begin(); it != playerInfo.end(); ++it)
		{
			if (it->second.seat_id() >= 0) ++count;
		}
		
		return count;
	}
	
	// 房间里座位上是否还有玩家
	inline bool hasSeatPlayer() const
	{
		for (RoomPlayerIdInfoMap::const_iterator it = playerInfo.begin(); it != playerInfo.end(); ++it)
		{
			if (it->second.seat_id() >= 0) return true;
		}
		
		return false;
	}
	
	// 获取房间里所有玩家的数量（包括旁观者）
	inline unsigned int getRoomPlayerCount() const
	{
		return playerInfo.size();
	}
	

	SRoomData() : playTimes(0), roomStatus(0), gameStatus(0),
	              optLastTimeSecs(0), optTimeSecs(0), optTimerId(0), nextOptType(0),
                  currentHasBanker(true), dismissRoomTimerId(0) {};
	virtual ~SRoomData() {};
};

typedef unordered_map<string, SRoomData*> RoomDataMap;  // 房间ID到房间数据

// 棋牌室房间数据
struct SHallData
{
	RoomDataMap gameRooms;
};
typedef unordered_map<string, SHallData> HallDataMap;  // 棋牌室ID到棋牌室数据

}

typedef unordered_map<string, com_protocol::ItemChange> GameSettlementMap;  // 游戏结算信息


#endif // _GAME_DATA_DEFINE_H_

