
/* author : limingfan
 * date : 2017.11.15
 * description : 类型定义
 */

#ifndef BASE_DEFINE_H
#define BASE_DEFINE_H

#include "common/CommonType.h"
#include "common/CServiceOperation.h"
#include "common/CRandomNumber.h"

#include "common/CattlesErrorCode.h"
#include "common/CattlesProtocolId.h"

#include "common/GameHallProtocolId.h"
#include "common/DBProxyProtocolId.h"

#include "protocol/cattles_game.pb.h"
#include "_NCattlesConfigData_.h"


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


// 操作类型
enum EOptType
{
	EPrepareGame = 1,
	EStartGame,
	ECompeteBanker,
	EConfirmBanker,
	EChoiceBet,
	EOpenCard,
};


// key ：玩家ID
// value ：
// 1）值大于0则表示发送了心跳消息的次数
// 2）值等于0则表示心跳检测失败，回调了心跳失败函数
typedef unordered_map<string, unsigned int> HeartbeatFailedTimesMap;  // 玩家心跳消息失败次数


// 牛牛扑克牌对应的值
static const unsigned int CardValue[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10};
static const unsigned int BaseCattlesValue = 10;

// 牛牛5张手牌
static const unsigned int CattlesCardCount = 5;
typedef unsigned char CattlesHandCard[CattlesCardCount];

// 正在玩游戏中的玩家数据
struct SGamePlayerData
{
	bool isOnline;                             // 是否在线

	unsigned int betValue;                     // 下注额度
	CattlesHandCard poker;                     // 5张手牌
	
	unsigned int resultValue;                  // 牌型结果值，牛几或者特殊牌型
	unsigned int settlementRate;               // 结算倍率
	
	int winLoseValue;                          // 当前局输赢值
	int sumWinLoseValue;                       // 总输赢值

	bool isLastCattlesWinner;                  // 是否是上一局闲家牛牛牌型以上的赢家（计算下一局新庄家使用）
	bool isLastBanker;                         // 是否是上一局庄家
	bool isNewBanker;                          // 是否是当前新庄家

    int choiceBankerResult;                    // 玩家抢庄结果
	

	inline void reset()
	{
		isOnline = true;
		betValue = 0;
		resultValue = 0;
		settlementRate = 0;
		winLoseValue = 0;
		isLastCattlesWinner = false;
		isLastBanker = false;
		isNewBanker = false;
		choiceBankerResult = 0;
		poker[0] = 0;
		
		// sumWinLoseValue = 0;  // do not reset to 0
	}

	SGamePlayerData() : isOnline(true), betValue(0), resultValue(0), settlementRate(0), winLoseValue(0), sumWinLoseValue(0),
	                    isLastCattlesWinner(false), isLastBanker(false), isNewBanker(false), choiceBankerResult(0) {poker[0] = 0;};
	~SGamePlayerData() {};
};
typedef unordered_map<string, SGamePlayerData> GamePlayerIdDataMap;  // 玩家ID映射数据


// 房间里的玩家数据（旁观者、游戏玩家等），玩家ID到玩家信息映射
typedef unordered_map<string, com_protocol::ClientRoomPlayerInfo> RoomPlayerIdInfoMap;

// 玩家发起解散房间请求时间点
typedef unordered_map<string, unsigned int> PlayerDismissRoomTimeMap;


// 棋牌室单个房间数据
struct SCattlesRoomData
{
    unsigned int playTimes;                             // 游戏局数
	int roomStatus;                                     // 房间状态（准备中、游戏中）
	
	int gameStatus;                                     // 游戏状态
	unsigned int playerConfirmCount;                    // 前端确认庄家时播放动画，需要所有在座的玩家都确认完毕才能开始游戏

    unsigned int optLastTimeSecs;                       // 操作倒计时持续的时间长度（游戏玩家掉线重连回房间、拒绝解散房间等需要）
	unsigned int optTimeSecs;                           // 操作开始时间点
	unsigned int optTimerId;                            // 操作定时器ID
	unsigned int nextOptType;                           // 超时后下一个操作类型（解散房间被玩家拒绝之后恢复继续游戏使用）
	
	unsigned int maxRate;                               // 房间牌型的最大倍率（计算庄家）
	com_protocol::ChessHallRoomInfo roomInfo;           // 房间基本信息

    bool currentHasBanker;                              // 房间当前是否存在满足当庄的玩家
	RoomPlayerIdInfoMap playerInfo;                     // 房间内所有玩家的数据（包括观看玩家）
	GamePlayerIdDataMap gamePlayerData;                 // 正在游戏中玩家的数据
	
	com_protocol::SetRoomGameRecordReq roomGameRecord;  // 房间游戏结算记录
	
	PlayerDismissRoomTimeMap playerDismissRoomTime;     // 玩家发起解散房间请求时间点
	StringVector agreeDismissRoomPlayer;                // 同意解散房间的玩家
	unsigned int dismissRoomTimerId;                    // 解散房间定时器ID

	UserChatInfoMap chatInfo;                           // 房间内玩家的聊天限制信息
	

    // 是否正在解散房间中
    inline const bool isOnDismissRoom() const
	{
		return (dismissRoomTimerId != 0 && !agreeDismissRoomPlayer.empty());
	}
	
    // 是否是游戏中
    inline const bool isOnPlay() const
	{
		const unsigned int roomType = roomInfo.base_info().room_type();
		if (roomType == com_protocol::EHallRoomType::EGoldId || roomType == com_protocol::EHallRoomType::EGoldEnter)
		{
			return gameStatus > com_protocol::ECattlesGameStatus::ECattlesGameReady;  // 金币场
		}
		
		return playTimes > 0;  // 房卡场
	}
	
    // 是否是金币场房间
    inline const bool isGoldRoom() const
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
	
	// 获取游戏玩家数量
	inline unsigned int getGamePlayerCount() const
	{
		return gamePlayerData.size();
	}
	
	// 获取房间里所有玩家的数量（包括旁观者）
	inline unsigned int getRoomPlayerCount() const
	{
		return playerInfo.size();
	}
	
	// 上一局庄家ID
	inline const char* getLastBanker(string* needSave = NULL) const
	{
		for (GamePlayerIdDataMap::const_iterator it = gamePlayerData.begin(); it != gamePlayerData.end(); ++it)
		{
			if (it->second.isLastBanker)
			{
				if (needSave != NULL)
				{
					*needSave = it->first;
					return needSave->c_str();
				}
				
				return it->first.c_str();
			}
		}
		
		return NULL;
	}
	
	// 当前局庄家ID
	inline const char* getCurrentBanker(string* needSave = NULL) const
	{
		for (GamePlayerIdDataMap::const_iterator it = gamePlayerData.begin(); it != gamePlayerData.end(); ++it)
		{
			if (it->second.isNewBanker)
			{
				if (needSave != NULL)
				{
					*needSave = it->first;
					return needSave->c_str();
				}
				
				return it->first.c_str();
			}
		}
		
		return NULL;
	}
	
	// 当局结算，牌型为牛牛以上的赢家
	inline bool getLastCattlesWinners(ConstCharPointerVector& players) const
	{
		for (GamePlayerIdDataMap::const_iterator it = gamePlayerData.begin(); it != gamePlayerData.end(); ++it)
		{
			if (it->second.isLastCattlesWinner) players.push_back(it->first.c_str()); // 上一局牌型为牛牛以上的赢家
		}
		
		return !players.empty();
	}
	
	// 当局结算，金币足够当庄的玩家
	inline bool getCanToBeNewBankers(const unsigned int needGold, StringVector& players) const
	{
		RoomPlayerIdInfoMap::const_iterator plIt;
		for (GamePlayerIdDataMap::const_iterator it = gamePlayerData.begin(); it != gamePlayerData.end(); ++it)
		{
			plIt = playerInfo.find(it->first);
			if (plIt != playerInfo.end() && plIt->second.detail_info().dynamic_info().game_gold() >= needGold)
			{
				players.push_back(it->first);
			}
		}
		
		return !players.empty();
	}
	
	SCattlesRoomData() : playTimes(0), roomStatus(0), gameStatus(0), playerConfirmCount(0),
	                     optLastTimeSecs(0), optTimeSecs(0), optTimerId(0), nextOptType(0),
                         maxRate(0), currentHasBanker(true), dismissRoomTimerId(0) {};
	~SCattlesRoomData() {};
};

typedef unordered_map<string, SCattlesRoomData> CattlesRoomMap;  // 房间ID到房间数据

// 棋牌室房间数据
struct SHallCattlesData
{
	CattlesRoomMap cattlesRooms;
};
typedef unordered_map<string, SHallCattlesData> HallCattlesMap;  // 棋牌室ID到棋牌室数据


// 等待超时的空房间，超时后则解散
typedef unordered_map<unsigned long long, unsigned int> WaitForDisbandRoomInfoMap;

}


#endif // BASE_DEFINE_H

