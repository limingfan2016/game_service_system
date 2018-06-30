
/* author : limingfan
 * date : 2018.03.28
 * description : 类型定义
 */

#ifndef BASE_DEFINE_H
#define BASE_DEFINE_H

#include "common/CommonType.h"
#include "common/CServiceOperation.h"
#include "common/CRandomNumber.h"

#include "common/GoldenFraudErrorCode.h"
#include "common/GoldenFraudProtocolId.h"

#include "common/GameHallProtocolId.h"
#include "common/DBProxyProtocolId.h"

#include "game_object/GameDataDefine.h"

#include "protocol/golden_fraud_game.pb.h"
#include "_NGoldenFraudConfigData_.h"


using namespace NProject;
using namespace NPlatformService;

namespace NGameService
{

// 玩家标识
enum EPlayerFlag
{
	EOnPlay = 1,                                  // 玩家游戏中
	EGiveUpCard = 2,                              // 玩家已经弃牌
	ECompareLose = 3,                             // 玩家比牌输了
	ENobody = 4,                                  // 座位没人（没有玩家）
};

// 看牌标识
enum EViewCardFlag
{
	ENotViewCard = 1,                             // 还没有看牌
	EAlreadyViewCard = 2,                         // 已经看牌了
};

// 操作结果
enum EOptResult
{
	ECurrentOpt = 1,                              // 正在当前操作
	EHasNextOpt = 2,                              // 已经执行到下一个操作
	EAlreadyFinish = 3,                           // 操作已经结束了
	EOptError = 4,                                // 操作错误
};

// 操作类型
enum EOptType
{
	EPlayerOpt = 1,                               // 玩家操作
	EPrepareOpt = 2,                              // 准备下一局操作
};


static const unsigned int PlayerMaxCount = 5;     // 最大玩家个数
static const unsigned int GoldenFraudCardA = 13;  // 炸金花牌A基础值，A > K(13)
static const unsigned int CardCount = 3;
typedef unsigned char HandCard[CardCount];        // 炸金花3张手牌

struct SGoldenFraudData : public SGamePlayerData
{
	string username;                              // 玩家ID
	int playerFlag;                               // 玩家标识
	
	HandCard handCard;                            // 玩家手牌
    int viewCardFlag;                             // 看牌标识

    inline void reset(const string& usrName)
	{
		SGamePlayerData::reset();
		
		username = usrName;
		playerFlag = EOnPlay;

		handCard[0] = {0};
		viewCardFlag = ENotViewCard;
	}
	
	SGoldenFraudData() : playerFlag(ENobody), viewCardFlag(ENotViewCard) {handCard[0] = {0};};
	~SGoldenFraudData() {};
};
typedef SGoldenFraudData GoldenFraudPlayers[PlayerMaxCount];    // 玩家数据


// 比牌结果
struct SCompareCardResult
{
	string username;                              // 玩家ID
	int result;                                   // 是否是比牌赢家，值1是赢家，其他值不是
	
	SCompareCardResult() : result(0) {};
	SCompareCardResult(const string& usrn, int rst) : username(usrn), result(rst) {};
	~SCompareCardResult() {};
};
typedef vector<SCompareCardResult> CompareCardResultVector;
typedef unordered_map<string, CompareCardResultVector> CompareCardResultMap;


struct SGoldenFraudRoom : public SRoomData
{
	unsigned int currentRoundTimes;               // 当前轮次
	unsigned int currentFollowBet;                // 当前需要的跟注值
	unsigned int currentAllBet;                   // 当前总下注值
	
	GoldenFraudPlayers players;                   // 游戏玩家
	unsigned int bankerIdx;                       // 庄家索引值
	unsigned int currentPlayerIdx;                // 当前操作玩家的索引值
	
	CompareCardResultMap compareCardResult;       // 比牌信息
	
	
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
		if (!isGoldRoom()) return 0;

		const NGoldenFraudBaseConfig::GoldenFraudBaseConfig& cfg = NGoldenFraudBaseConfig::GoldenFraudBaseConfig::getConfigValue(NCommonConfig::CommonCfg::getConfigValue().config_file.golden_fraud_base_cfg.c_str(), false);

		// 最大加注比例 * 底注 * 最大参与人数
		return cfg.gold_bet_rate[cfg.gold_bet_rate.size() - 1] * roomInfo.base_info().base_rate() * roomInfo.base_info().player_count();
	}

	// 庄家需要的最低金币数量
	virtual unsigned int getBankerNeedMinGold() const
	{
		return 0;
	}
	
	// 进入房间需要的金币数量
	virtual unsigned int getEnterNeedGold() const
	{
		return getPlayerNeedMinGold();
	}
	
	// 开桌人数
	virtual unsigned int getStartPlayers() const
	{
		return roomInfo.golden_fraud_room().start_players();
	}
	
	// 心跳通知
	virtual bool onHearbeatStatusNotify(const string& username, const bool isOnline, int& newStatus)
	{
	    if (!isOnPlay()) return false;

		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			SGoldenFraudData& playerData = players[idx];
			if (username == playerData.username)
			{
				if (playerData.isOnline != isOnline)
				{
					playerData.isOnline = isOnline;
					newStatus = !isOnline ? com_protocol::ERoomPlayerStatus::EPlayerOffline : playerInfo[username].status();
					
					return true;
				}
				
				return false;
			}
		}
		
		return false;
	}
	
	// 重入房间
	virtual void onReEnter(const string& username)
	{
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			SGoldenFraudData& playerData = players[idx];
			if (username == playerData.username)
			{
				playerData.isOnline = true;
				break;
			}
		}
	}
	
	// 游戏记录信息
	virtual void writePlayerItemChangeInfo(const string& username, com_protocol::ItemChangeReq* itemChange)
	{
		const SGoldenFraudData* playerData = getPlayerData(username);
		if (playerData == NULL) return;

        com_protocol::ChangeRecordPlayerInfo* playerInfo = itemChange->mutable_player_info();
        playerInfo->set_card_result(playerData->cardValue);
        playerInfo->set_card_rate(0);
        playerInfo->set_card_info((const char*)playerData->handCard, CardCount);
        playerInfo->set_player_flag(playerData->isNewBanker ? 1 : 0);
		playerInfo->set_bet_value(playerData->betValue);
	}
	
	// 房卡付费
	virtual void payRoomCard(const char* hallId, const float payCount, com_protocol::MoreUsersItemChangeReq& itemChangeReq)
	{
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			SGoldenFraudData& playerData = players[idx];
			if (playerData.playerFlag == ENobody) continue;
			
			com_protocol::ItemChangeReq* itemChange = itemChangeReq.add_item_change_req();
            itemChange->set_write_data_log(1);
			
			itemChange->set_hall_id(hallId);
		    itemChange->set_src_username(playerData.username);
			
			com_protocol::ItemChange* item = itemChange->add_items();
			item->set_type(EGameGoodsType::EGoodsRoomCard);
			item->set_num(payCount);
		}
	}
	
	// 获取下一个操作玩家索引值
	bool nextOptPlayerIndex()
	{
		if (players[currentPlayerIdx].playerFlag == ENobody) return false;

		const unsigned int curIdx = currentPlayerIdx;
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			if (currentPlayerIdx == bankerIdx)
			{
				if (++currentRoundTimes > roomInfo.golden_fraud_room().max_round()) return false;
			}

			++currentPlayerIdx %= PlayerMaxCount;
			if (currentPlayerIdx == curIdx) return false;
			
			if (players[currentPlayerIdx].playerFlag == EOnPlay) return true;
		}
		
		return false;
	}
	
	inline bool isFinishRound() const
	{
		return currentRoundTimes > roomInfo.golden_fraud_room().max_round();
	}
	
	inline bool canViewCard() const
	{
		return (currentRoundTimes >= roomInfo.golden_fraud_room().read_card_round()
		        && gameStatus == com_protocol::EGameStatus::EGameChoiceBet);
	}
	
	inline bool canCompareCard() const
	{
		return (currentRoundTimes >= roomInfo.golden_fraud_room().compare_card_round()
		        && gameStatus == com_protocol::EGameStatus::EGameChoiceBet);
	}
	
	inline bool canAllIn() const
	{
		if (currentRoundTimes < roomInfo.golden_fraud_room().compare_card_round()) return false;
		
		unsigned int onPlayerCount = 0;
		int playerViewCard = -1;
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			const SGoldenFraudData& playerData = players[idx];
			if (playerData.playerFlag != EOnPlay) continue;
			
			if (++onPlayerCount > 2) return false;  // 全押必须只有2个玩家
			
			if (playerViewCard == -1) playerViewCard = playerData.viewCardFlag;
			else if (playerViewCard != playerData.viewCardFlag) return false;  // 全押玩家的看牌标识必须一致
		}
		
		return onPlayerCount > 1;
	}

	inline bool hasOnlinePlayer() const
	{
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			if (players[idx].playerFlag == EOnPlay && players[idx].isOnline) return true;
		}
		
		return false;
	}
	
	inline bool hasOnlineUser() const
	{
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			if (players[idx].playerFlag != ENobody && players[idx].isOnline) return true;
		}
		
		return false;
	}
	
	SGoldenFraudData* getPlayerData(const string& username)
	{
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			if (username == players[idx].username) return &players[idx];
		}
		
		return NULL;
	}
	
	int getPlayerIndex(const string& username)
	{
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			if (username == players[idx].username) return idx;
		}
		
		return -1;
	}
	
	SGoldenFraudData* getAllInOtherPlayer(const string& currentPlayer)
	{
		SGoldenFraudData* other = NULL;
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			if (players[idx].playerFlag == EOnPlay && players[idx].username != currentPlayer)
			{
				if (other != NULL) return NULL;  // 全押的对方必须唯一

				other = &players[idx];
			}
		}
		
		return other;
	}
	
	SGoldenFraudData* getWinnerData()
	{
		SGoldenFraudData* winner = NULL;
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			if (players[idx].playerFlag == EOnPlay)
			{
				if (winner != NULL) return NULL;  // 最后的赢家唯一

				winner = &players[idx];
			}
		}
		
		return winner;
	}
	
	inline unsigned int getFollowBet(int viewCardFlag) const
	{
		return (viewCardFlag == ENotViewCard
			    || gameStatus == com_protocol::EGameStatus::EGameAllIn) ? currentFollowBet : currentFollowBet * 2;
	}
	
	inline unsigned int getCurrentPlayerFollowBet() const
	{
		return getFollowBet(players[currentPlayerIdx].viewCardFlag);
	}

	// 付费下注额
	inline int payBet(unsigned int betValue, SGoldenFraudData& playerData)
	{
		if (isGoldRoom())
		{
			com_protocol::DynamicInfo* dynamicInfo = playerInfo[playerData.username].mutable_detail_info()->mutable_dynamic_info();
			if (dynamicInfo->game_gold() < betValue) return GoldenFraudGoldInsufficient;

		    dynamicInfo->set_game_gold(dynamicInfo->game_gold() - betValue);
		}
		else
		{
			playerData.sumWinLoseValue -= betValue;
		}

		playerData.betValue += betValue;
		currentAllBet += betValue;
		
		OptInfoLog("test only pay bet, username = %s, betValue = %u, curBet = %u, allBet = %u, sumValue = %d",
        playerData.username.c_str(), betValue, playerData.betValue, currentAllBet, playerData.sumWinLoseValue);
		
		return SrvOptSuccess;
	}
	
	// 赢得下注额
	inline void winBet(unsigned int betValue, SGoldenFraudData& playerData)
	{
		if (isGoldRoom())
		{
			com_protocol::DynamicInfo* dynamicInfo = playerInfo[playerData.username].mutable_detail_info()->mutable_dynamic_info();
		    dynamicInfo->set_game_gold(dynamicInfo->game_gold() + betValue);
		}
		else
		{
			playerData.sumWinLoseValue += betValue;
		}
		
		OptInfoLog("test only win bet, username = %s, betValue = %u, curBet = %u, allBet = %u, sumValue = %d",
        playerData.username.c_str(), betValue, playerData.betValue, currentAllBet, playerData.sumWinLoseValue);
	}
	
	void getCurrentPlayer(ConstCharPointerVector& usernames) const
	{
		for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
		{
			if (players[idx].playerFlag != ENobody) usernames.push_back(players[idx].username.c_str());
		}
	}

	SGoldenFraudRoom() : currentRoundTimes(0), currentFollowBet(0), currentAllBet(0), bankerIdx(0), currentPlayerIdx(0) {};
	~SGoldenFraudRoom() {};
};

}


#endif // BASE_DEFINE_H

