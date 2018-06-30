
/* author : limingfan
 * date : 2018.03.29
 * description : 炸金花游戏服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "CGoldenFraud.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace NDBOpt;
using namespace NProject;

namespace NGameService
{

// 数据记录日志
#define WriteDataLog(format, args...)     m_srvOpt.getDataLogger()->writeFile(NULL, 0, LogLevel::Info, format, ##args)


CGoldenFraudMsgHandler::CGoldenFraudMsgHandler()
{
	m_pCfg = NULL;
}

CGoldenFraudMsgHandler::~CGoldenFraudMsgHandler()
{
	m_pCfg = NULL;
}

void CGoldenFraudMsgHandler::onInit()
{
	m_pCfg = &NGoldenFraudConfigData::GoldenFraudConfig::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str());
	if (!m_pCfg->isSetConfigValueSuccess())
	{
		OptErrorLog("set business xml config value error");
		stopService();
		return;
	}
	
	registerProtocol(OutsideClientSrv, GOLDENFRAUD_GIVE_UP_REQ, (ProtocolHandler)&CGoldenFraudMsgHandler::giveUp);
	registerProtocol(OutsideClientSrv, GOLDENFRAUD_VIEW_CARD_REQ, (ProtocolHandler)&CGoldenFraudMsgHandler::viewCard);
	registerProtocol(OutsideClientSrv, GOLDENFRAUD_DO_BET_REQ, (ProtocolHandler)&CGoldenFraudMsgHandler::doBet);
	registerProtocol(OutsideClientSrv, GOLDENFRAUD_COMPARE_CARD_REQ, (ProtocolHandler)&CGoldenFraudMsgHandler::compareCard);

    m_srvOpt.getGoldenFraudBaseCfg().output();
	m_pCfg->output();
}

void CGoldenFraudMsgHandler::onUnInit()
{
	
}
	
// 服务配置数据刷新
void CGoldenFraudMsgHandler::onUpdate()
{
	m_pCfg = &NGoldenFraudConfigData::GoldenFraudConfig::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str(), true);
	if (!m_pCfg->isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update business xml config value error");
	}

	ReleaseInfoLog("update config business result = %d", m_pCfg->isSetConfigValueSuccess());

    m_srvOpt.getGoldenFraudBaseCfg(true).output();
	m_pCfg->output();
}

// 服务标识
int CGoldenFraudMsgHandler::getSrvFlag()
{
	return m_pCfg->common_cfg.flag;
}

// 玩家上线&下线
void CGoldenFraudMsgHandler::onLine(GameUserData* cud, ConnectProxy* conn, const com_protocol::DetailInfo& userDetailInfo)
{
}

void CGoldenFraudMsgHandler::offLine(GameUserData* cud)
{
}

	
// 开始游戏
void CGoldenFraudMsgHandler::onStartGame(const char* hallId, const char* roomId, SRoomData& roomData)
{
	// 初始化座位为无人
	SGoldenFraudRoom& goldenFraudRoom = (SGoldenFraudRoom&)roomData;
	goldenFraudRoom.currentAllBet = 0;
	goldenFraudRoom.compareCardResult.clear();
	for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
	{
		goldenFraudRoom.players[idx].playerFlag = ENobody;
		goldenFraudRoom.players[idx].username.clear();
	}
	
	// 重置当前局游戏玩家信息并设置玩家的掉线重连会话数据
	const unsigned int betValue = goldenFraudRoom.roomInfo.base_info().base_rate();
	int seatId = -1;
	ConstCharPointerVector playerUsernames;  // 一起游戏的玩家
	for (RoomPlayerIdInfoMap::iterator it = goldenFraudRoom.playerInfo.begin(); it != goldenFraudRoom.playerInfo.end(); ++it)
	{
		seatId = it->second.seat_id();
		if (seatId < 0 || seatId >= (int)PlayerMaxCount) continue;

		SGoldenFraudData& playerData = goldenFraudRoom.players[seatId];
		playerData.reset(it->first);
		
		if (goldenFraudRoom.payBet(betValue, playerData) != SrvOptSuccess)
		{
			OptErrorLog("start game but pay bet error, hallId = %s, roomId = %s, username = %s, bet = %u",
			hallId, roomId, playerData.username.c_str(), betValue);
			return;  // 出错了，这里可以考虑直接解散房间
		}

		it->second.set_status(com_protocol::EGoldenFraudPlayerStatus::EGoldenFraudPlayOn);  // 变更状态为游戏中

		playerUsernames.push_back(it->first.c_str());
	}

	// 随机&计算玩家手牌
    if (!randomCalculatePlayerCard(goldenFraudRoom))
	{
		OptErrorLog("start game but random calculate player card error, hallId = %s, roomId = %s",
		hallId, roomId);
		return;  // 出错了，这里可以考虑直接解散房间
	}
	
	// 游戏开始了，房间状态变更为游戏中
	m_srvOpt.stopTimer(goldenFraudRoom.optTimerId);
	goldenFraudRoom.roomStatus = com_protocol::EHallRoomStatus::EHallRoomPlay;
	m_srvOpt.changeRoomDataNotify(hallId, roomId, GoldenFraudGame, com_protocol::EHallRoomStatus::EHallRoomPlay,
								  NULL, NULL, NULL, "room play game");
					
	m_srvOpt.setLastPlayersNotify(hallId, playerUsernames);  // 一起游戏的玩家通知DB缓存
	
	setReEnterInfo(playerUsernames, hallId, roomId, EGamePlayerStatus::EPlayRoom);  // 如果玩家在游戏中掉线，则依据掉线信息重连回房间
	
	addHearbeatCheck(playerUsernames);  // 启动游戏玩家心跳检测

    // 初始化房间数据
	++goldenFraudRoom.playTimes;
	goldenFraudRoom.gameStatus = com_protocol::EGameStatus::EGameChoiceBet;
	goldenFraudRoom.currentRoundTimes = 0;
	goldenFraudRoom.currentFollowBet = betValue;
	
	// 随机庄家
	unsigned int bankerIdx = CRandomNumber::getUIntNumber(0, PlayerMaxCount - 1);
	for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
	{
		if (goldenFraudRoom.players[bankerIdx].playerFlag == EOnPlay)
		{
			goldenFraudRoom.players[bankerIdx].isNewBanker = true;
			break;
		}
		
		++bankerIdx %= PlayerMaxCount;
	}
	
	goldenFraudRoom.bankerIdx = bankerIdx;
	goldenFraudRoom.currentPlayerIdx = bankerIdx;
	if (!goldenFraudRoom.nextOptPlayerIndex())
	{
		OptErrorLog("get next opt player error, hallId = %s, roomId = %s",
		hallId, roomId);
		
		return;
	}

	// 开始游戏，发牌给玩家
	const NGoldenFraudBaseConfig::GoldenFraudBaseConfig& cfg = m_srvOpt.getGoldenFraudBaseCfg();
	com_protocol::GoldenFraudPushCardToPlayerNotify pushCardNtf;
	pushCardNtf.set_wait_secs(cfg.common_cfg.play_game_secs);
	pushCardNtf.set_banker_name(goldenFraudRoom.players[bankerIdx].username);
	pushCardNtf.set_next_username(goldenFraudRoom.players[goldenFraudRoom.currentPlayerIdx].username);
	pushCardNtf.set_next_follow_bet(goldenFraudRoom.currentFollowBet);
	pushCardNtf.set_play_times(goldenFraudRoom.playTimes);
	
	// 加注比例列表
	const vector<unsigned int>& betRate = goldenFraudRoom.isGoldRoom() ? cfg.gold_bet_rate : cfg.card_bet_rate;
	for (vector<unsigned int>::const_iterator it = betRate.begin();
	     it != betRate.end(); ++it) pushCardNtf.add_increase_bet_value(*it * betValue);
	
	sendPkgMsgToRoomPlayers(goldenFraudRoom, pushCardNtf, GOLDENFRAUD_PUSH_CARD_NOTIFY, "push card notify");
	
	// 超时操作
	setOptTimeOutInfo(atoi(hallId), atoi(roomId), goldenFraudRoom, pushCardNtf.wait_secs(),
	                  this, (TimerHandler)&CGoldenFraudMsgHandler::playerOptTimeOut, EOptType::EPlayerOpt);

    OptInfoLog("start game push card to player, hallId = %s, roomId = %s, times = %u, banker = %s, players = %u",
	hallId, roomId, goldenFraudRoom.playTimes, pushCardNtf.banker_name().c_str(), playerUsernames.size());
}

// 玩家离开房间
EGamePlayerStatus CGoldenFraudMsgHandler::onLeaveRoom(SRoomData& roomData, const char* username)
{
	do
	{
		RoomPlayerIdInfoMap::iterator plIt = roomData.playerInfo.find(username);
		if (plIt == roomData.playerInfo.end()) break;
		
		SGoldenFraudRoom& goldenFraudRoom = (SGoldenFraudRoom&)roomData;
		SGoldenFraudData* playerData = goldenFraudRoom.getPlayerData(username);
		if (playerData == NULL)
		{
			roomData.playerInfo.erase(plIt);  // 非游戏玩家（旁观玩家、或者玩家坐下了但没开始游戏）直接删除
			break;
		}
		
		// 非游戏中状态，任意玩家都可以离开房间
		if (!roomData.isOnPlay())
		{
			roomData.playerInfo.erase(plIt);
			playerData->playerFlag = ENobody;
			playerData->username.clear();
			break;
		}
		
		// 金币场玩家弃牌、比牌输了都可以提前离开
		if (roomData.isGoldRoom() && playerData->playerFlag != EOnPlay)
		{
			roomData.playerInfo.erase(plIt);
			break;
		}
		
		// 游戏过程中，玩家掉线了
		playerData->isOnline = false;

		return EGamePlayerStatus::EDisconnectRoom;

	} while (false);
	
	return (canDisbandGameRoom(roomData) && roomData.roomStatus == 0) ? EGamePlayerStatus::ERoomDisband : EGamePlayerStatus::ELeaveRoom;
}

// 拒绝解散房间，恢复继续游戏
bool CGoldenFraudMsgHandler::onRefuseDismissRoomContinueGame(const GameUserData* cud, SRoomData& roomData, CHandler*& instance, TimerHandler& timerHandler)
{
	instance = this;

	if (roomData.nextOptType == EOptType::EPlayerOpt) timerHandler = (TimerHandler)&CGoldenFraudMsgHandler::playerOptTimeOut;
	else if (roomData.nextOptType == EOptType::EPrepareOpt) timerHandler = (TimerHandler)&CGoldenFraudMsgHandler::prepareTimeOut;
	else timerHandler = NULL;
	
	return true;
}

// 创建&销毁房间数据
SRoomData* CGoldenFraudMsgHandler::createRoomData()
{
	SRoomData* roomData = NULL;
	NEW(roomData, SGoldenFraudRoom());
	return roomData;
}

void CGoldenFraudMsgHandler::destroyRoomData(SRoomData* roomData)
{
	DELETE(roomData);
}

// 每一局房卡AA付费
float CGoldenFraudMsgHandler::getAveragePayCount()
{
	return m_srvOpt.getGoldenFraudBaseCfg().room_card_cfg.average_pay_count;
}

// 每一局房卡房主付费
float CGoldenFraudMsgHandler::getCreatorPayCount()
{
	return m_srvOpt.getGoldenFraudBaseCfg().room_card_cfg.creator_pay_count;
}

// 是否可以解散房间
bool CGoldenFraudMsgHandler::canDisbandGameRoom(const SRoomData& roomData)
{	
	return roomData.playerInfo.empty();  // 空房间没有人才可以解散
}

// 玩家是否可以离开房间
bool CGoldenFraudMsgHandler::canLeaveRoom(GameUserData* cud)
{
	SGoldenFraudRoom* roomData = (SGoldenFraudRoom*)getRoomDataById(cud->roomId, cud->hallId);
	if (roomData == NULL) return true;
	
	// 非游戏中状态，任意玩家都可以离开房间
	if (!roomData->isOnPlay()) return true;
	
	// 金币场非游戏中的玩家可以离开
	SGoldenFraudData* playerData = roomData->getPlayerData(cud->userName);
	return (playerData == NULL || (roomData->isGoldRoom() && playerData->playerFlag != EOnPlay));
}

// 设置游戏房间数据，返回是否设置了游戏数据
bool CGoldenFraudMsgHandler::setGameRoomData(const string& username, const SRoomData& roomData,
							                 google::protobuf::RepeatedPtrField<com_protocol::ClientRoomPlayerInfo>& roomPlayers,
							                 com_protocol::ChessHallGameData& gameData)
{
	// 房间里玩家的信息
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		*roomPlayers.Add() = it->second;
		
		OptInfoLog("test add user info, caller = %s, player = %s, static name = %s, status = %d, seat = %d",
		username.c_str(), it->first.c_str(), it->second.detail_info().static_info().username().c_str(),
		it->second.status(), it->second.seat_id());
	}

	if (!roomData.isOnPlay()) return false;

    // 正在游戏中则回传游戏数据
	com_protocol::ChessHallGameBaseData* gameBaseData = gameData.mutable_base_data();
	gameBaseData->set_game_type(GoldenFraudGame);
	gameBaseData->set_game_status(roomData.gameStatus);
	gameBaseData->set_current_round(roomData.playTimes);

	SGoldenFraudRoom& goldenFraudRoom = (SGoldenFraudRoom&)roomData;
	gameBaseData->set_current_bet(goldenFraudRoom.currentAllBet);
	
	com_protocol::GoldenFraudGameInfo* goldenFraudInfo = gameData.mutable_golden_fraud_game();
	goldenFraudInfo->set_banker_id(goldenFraudRoom.players[goldenFraudRoom.bankerIdx].username);
	goldenFraudInfo->set_current_round_times(goldenFraudRoom.currentRoundTimes);
	goldenFraudInfo->set_current_opt_username(goldenFraudRoom.players[goldenFraudRoom.currentPlayerIdx].username);
	
	const NGoldenFraudBaseConfig::GoldenFraudBaseConfig& goldenFraudCfg = m_srvOpt.getGoldenFraudBaseCfg();
	goldenFraudInfo->set_normal_wait_secs(goldenFraudCfg.common_cfg.play_game_secs);

	// 剩余倒计时时间
	if (roomData.optTimerId > 0 && roomData.optTimeSecs > 0 && roomData.optLastTimeSecs > 0)
	{
		const unsigned int passTimeSecs = time(NULL) - roomData.optTimeSecs;
		if (roomData.optLastTimeSecs > passTimeSecs) goldenFraudInfo->set_wait_secs(roomData.optLastTimeSecs - passTimeSecs);
	}
	
	// 加注比例列表
	const unsigned int betValue = goldenFraudRoom.roomInfo.base_info().base_rate();
	const vector<unsigned int>& betRate = goldenFraudRoom.isGoldRoom() ? goldenFraudCfg.gold_bet_rate : goldenFraudCfg.card_bet_rate;
	for (vector<unsigned int>::const_iterator it = betRate.begin();
	     it != betRate.end(); ++it) goldenFraudInfo->add_increase_bet_value(*it * betValue);

	for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
	{
		SGoldenFraudData& playerData = goldenFraudRoom.players[idx];
		if (playerData.playerFlag == ENobody) continue;
		
		com_protocol::GoldenFraudGamePlayerInfo* gamePlayer = goldenFraudInfo->add_player_info();
		gamePlayer->set_username(playerData.username);
		gamePlayer->set_status(goldenFraudRoom.playerInfo[playerData.username].status());
		gamePlayer->set_bet_value(playerData.betValue);
		
		if (playerData.viewCardFlag != ENotViewCard)
		{
			gamePlayer->set_is_view_card(1);
			if (username == playerData.username)
			{
				for (unsigned int cardIdx = 0; cardIdx < CardCount; ++cardIdx) gamePlayer->add_pokers(playerData.handCard[cardIdx]);
				gamePlayer->set_result_value(playerData.cardValue);
			}
		}
		
		if (username == playerData.username) gamePlayer->set_follow_bet(goldenFraudRoom.getFollowBet(playerData.viewCardFlag));
		
		// 房卡场数据
		if (!goldenFraudRoom.isGoldRoom()) gamePlayer->set_sum_win_lose(playerData.sumWinLoseValue);
		
		OptInfoLog("test only bet, username = %s, curBet = %u, allBet = %u, sumValue = %d",
        playerData.username.c_str(), playerData.betValue, goldenFraudRoom.currentAllBet, playerData.sumWinLoseValue);
	}
	
	OptInfoLog("test set game room data, username = %s, game status = %u, bankerId = %s",
    username.c_str(), goldenFraudRoom.gameStatus, goldenFraudInfo->banker_id().c_str());

	return true;
}

// 结算牌型
unsigned int CGoldenFraudMsgHandler::getCardType(const HandCard poker)
{
	if (isThreeSameCard(poker)) return com_protocol::EGoldenFraudCardType::EThreeSameCard;            // 豹子
	
	if (isSameColourSequence(poker)) return com_protocol::EGoldenFraudCardType::ESameColourSequence;  // 同花顺
	
	if (isSameColour(poker)) return com_protocol::EGoldenFraudCardType::ESameColourCard;              // 同花
	
	if (isSequence(poker)) return com_protocol::EGoldenFraudCardType::ESequenceCard;                  // 顺子
	
	if (isCoupleCard(poker)) return com_protocol::EGoldenFraudCardType::ECoupleCard;                  // 对子
	
	if (isSpecialCard(poker)) return com_protocol::EGoldenFraudCardType::ESpecialCard;                // 特殊牌
	
	return com_protocol::EGoldenFraudCardType::EScatteredCard;                                        // 散牌
}

// 特殊牌
bool CGoldenFraudMsgHandler::isSpecialCard(const HandCard poker)
{
	if (isSameColour(poker)) return false;

	// 特殊牌型：235
	const unsigned int specialValue[] = {2, 3, 5};
	for (unsigned int idx = 0; idx < (sizeof(specialValue) / sizeof(unsigned int)); ++idx)
	{
		if (poker[0] % PokerCardBaseValue == specialValue[idx]
		    || poker[1] % PokerCardBaseValue == specialValue[idx]
			|| poker[2] % PokerCardBaseValue == specialValue[idx]) continue;
			
	    return false;
	}

	return true;
}

// 对子
bool CGoldenFraudMsgHandler::isCoupleCard(const HandCard poker)
{
	const unsigned int value0 = poker[0] % PokerCardBaseValue;
	return value0 == (poker[1] % PokerCardBaseValue)
	       || value0 == (poker[2] % PokerCardBaseValue)
		   || (poker[1] % PokerCardBaseValue) == (poker[2] % PokerCardBaseValue);
}

// 顺子
bool CGoldenFraudMsgHandler::isSequence(const HandCard poker)
{
	// 降序排序
	HandCard sortPoker = {0};
	memcpy(sortPoker, poker, sizeof(HandCard));
	
    descendSort(sortPoker);
	
	unsigned int value0 = sortPoker[0] % PokerCardBaseValue;
	if (value0 == 1) value0 += GoldenFraudCardA;
	
	return ((value0 == ((sortPoker[1] % PokerCardBaseValue) + 1))
	        && (value0 == ((sortPoker[2] % PokerCardBaseValue) + 2)));
}

// 同花
bool CGoldenFraudMsgHandler::isSameColour(const HandCard poker)
{
	const unsigned int Colour = poker[0] / PokerCardBaseValue;
	return Colour == (poker[1] / PokerCardBaseValue) && Colour == (poker[2] / PokerCardBaseValue);
}

// 同花顺
bool CGoldenFraudMsgHandler::isSameColourSequence(const HandCard poker)
{
	return isSameColour(poker) && isSequence(poker);
}

// 豹子
bool CGoldenFraudMsgHandler::isThreeSameCard(const HandCard poker)
{
	return (poker[0] % PokerCardBaseValue == poker[1] % PokerCardBaseValue
	        && poker[0] % PokerCardBaseValue == poker[2] % PokerCardBaseValue);
}

// 降序排序
void CGoldenFraudMsgHandler::descendSort(HandCard poker)
{
	// 只有3张牌，简单交换排序
	unsigned char tmp = '\0';
	unsigned int value1 = 0;
	unsigned int value2 = 0;
	for (unsigned int idx1 = 0; idx1 < CardCount - 1; ++idx1)
	{
		for (unsigned int idx2 = (idx1 + 1); idx2 < CardCount; ++idx2)
		{
			value1 = poker[idx1] % PokerCardBaseValue;
			value2 = poker[idx2] % PokerCardBaseValue;
			
			if (value1 == 1) value1 += GoldenFraudCardA;
			if (value2 == 1) value2 += GoldenFraudCardA;
			
			// 先比较牌值，牌值相同则比较花色
			if ((value2 > value1)
				|| (value2 == value1 && (poker[idx2] / PokerCardBaseValue) > (poker[idx1] / PokerCardBaseValue)))
			{
				tmp = poker[idx1];
				poker[idx1] = poker[idx2];
				poker[idx2] = tmp;
			}
		}
	}
}
	
// 随机&计算玩家手牌
bool CGoldenFraudMsgHandler::randomCalculatePlayerCard(SGoldenFraudRoom& roomData)
{
	unsigned char pokerCard[BasePokerCardCount] = {0};
	memcpy(pokerCard, AllPokerCard, BasePokerCardCount * sizeof(unsigned char));

	unsigned int pokerCardCount = BasePokerCardCount - 1;
	unsigned int randomIdx = 0;
	for (unsigned int seatId = 0; seatId < PlayerMaxCount; ++seatId)
	{
		SGoldenFraudData& playerData = roomData.players[seatId];
		if (playerData.playerFlag == ENobody) continue;
		
		// 游戏玩家
		for (unsigned int idx = 0; idx < CardCount; ++idx)
		{
			// 随机玩家手牌
			randomIdx = CRandomNumber::getUIntNumber(0, pokerCardCount);
			playerData.handCard[idx] = pokerCard[randomIdx];

			pokerCard[randomIdx] = pokerCard[pokerCardCount];

			if (--pokerCardCount < 1)
			{
				OptErrorLog("random calculate player card error, need card count = %u, all card count = %u",
	            CardCount, BasePokerCardCount);
				
				return false;
            }
		}
		
		// 算出牌型
		playerData.cardValue = getCardType(playerData.handCard);
	}
	
	return true;
}

int CGoldenFraudMsgHandler::getOptData(const GameUserData* cud, SGoldenFraudRoom*& roomData, SGoldenFraudData*& playerData)
{
	roomData = (SGoldenFraudRoom*)getRoomDataById(cud->roomId, cud->hallId);
	if (roomData == NULL) return GameObjectNotFoundRoomData;

	if (roomData->gameStatus != com_protocol::EGameStatus::EGameChoiceBet
	    && roomData->gameStatus != com_protocol::EGameStatus::EGameAllIn) return GameObjectGameStatusInvalidOpt;
	
	playerData = roomData->getPlayerData(cud->userName);
	if (playerData == NULL) return GoldenFraudNotFoundPlayer;
	
	const com_protocol::ClientRoomPlayerInfo* pInfo = roomData->getPlayerInfo(cud->userName);
	if (pInfo == NULL
	    || pInfo->status() != com_protocol::EGoldenFraudPlayerStatus::EGoldenFraudPlayOn) return GoldenFraudPlayerStatusInvalid;

	return SrvOptSuccess;
}

EOptResult CGoldenFraudMsgHandler::doNextOpt(SGoldenFraudRoom& roomData, unsigned int hallId, unsigned int roomId, unsigned int addWaitSecs)
{
	
	if (roomData.nextOptPlayerIndex())
	{
		// 超时操作
		setOptTimeOutInfo(hallId, roomId, roomData,
						  m_srvOpt.getGoldenFraudBaseCfg().common_cfg.play_game_secs + addWaitSecs,
						  this, (TimerHandler)&CGoldenFraudMsgHandler::playerOptTimeOut, EOptType::EPlayerOpt);
		return EHasNextOpt;
	}
	
	// 游戏结束了则结算
	return EAlreadyFinish;
}

void CGoldenFraudMsgHandler::doNextOptAndNotify(SGoldenFraudRoom& roomData, const GameUserData* cud, int opt,
                                                unsigned int bet, const string& compareUsername)
{
	doNextOptAndNotify(roomData, cud->userName, atoi(cud->hallId), atoi(cud->roomId), opt, bet, compareUsername);
}

void CGoldenFraudMsgHandler::doNextOptAndNotify(SGoldenFraudRoom& roomData, const string& userName, unsigned int hallId,
                                                unsigned int roomId, int opt, unsigned int bet, const string& compareUsername)
{
	com_protocol::GoldenFraudPlayerOptNotify optNtf;
	optNtf.set_opt_type((com_protocol::EGoldenFraudOptType)opt);
	optNtf.set_opt_username(userName);

	SGoldenFraudData& currentPlayerData = roomData.players[roomData.currentPlayerIdx];
    EOptResult optResult = ECurrentOpt;
	switch (opt)
	{
		case com_protocol::EGoldenFraudOptType::EDiscardCard:
		{
			if (roomData.getWinnerData() != NULL) optResult = EAlreadyFinish;  // 剩下最后的两个人，任意一方弃牌都会导致结算
			else if (currentPlayerData.username == userName) optResult = doNextOpt(roomData, hallId, roomId);  // 当前操作的玩家弃牌了，则轮到下一个玩家操作

			break;
		}
		
		case com_protocol::EGoldenFraudOptType::EViewCard:
		{
			if (currentPlayerData.username == userName)
			{
				optNtf.set_next_follow_bet(roomData.getCurrentPlayerFollowBet());
			}
		
			break;
		}
		
		case com_protocol::EGoldenFraudOptType::EFollowBet:
		case com_protocol::EGoldenFraudOptType::EIncreaseBet:
		case com_protocol::EGoldenFraudOptType::ECompareCard:
		case com_protocol::EGoldenFraudOptType::EAllInBet:
		{
			optNtf.set_opt_bet_value(bet);
			optNtf.set_current_bet_value(currentPlayerData.betValue);
			if (roomData.isGoldRoom()) optNtf.set_current_gold_value(roomData.playerInfo[userName].detail_info().dynamic_info().game_gold());

			unsigned int addWaitSecs = 0;
			if (opt == com_protocol::EGoldenFraudOptType::ECompareCard)
			{
				optResult = compareCardOpt(roomData, currentPlayerData, hallId, roomId, compareUsername, optNtf);
				if (optResult == EOptError) return;
				else if (optResult == EAlreadyFinish) break;
				
				addWaitSecs = m_srvOpt.getGoldenFraudBaseCfg().common_cfg.compare_card_secs * optNtf.compare_player_size();
			}

			optResult = doNextOpt(roomData, hallId, roomId, addWaitSecs);
			
			break;
		}

		default:
		{
			OptErrorLog("send opt notify the type is invalid, username = %s, hallId = %u, roomId = %u, opt = %d",
	                    userName.c_str(), hallId, roomId, opt);
		    return;
			break;
		}
	}
	
	optNtf.set_all_bet_value(roomData.currentAllBet);
	optNtf.set_current_round(roomData.currentRoundTimes);
	
	if (optResult == EHasNextOpt)
	{
		optNtf.set_next_username(roomData.players[roomData.currentPlayerIdx].username);
		optNtf.set_next_follow_bet(roomData.getCurrentPlayerFollowBet());
	}
	else if (optResult == ECurrentOpt)
	{
		optNtf.set_next_username(currentPlayerData.username);
	}
	
	int rc = sendPkgMsgToRoomPlayers(roomData, optNtf, GOLDENFRAUD_PLAYER_OPT_NOTIFY, "player opt notify");

	OptInfoLog("do opt hallId = %u, roomId = %u, opt = %d, bet = %u, result = %d, current username = %s, next username = %s, rc = %d",
			   hallId, roomId, opt, bet, optResult, userName.c_str(), optNtf.next_username().c_str(), rc);
	
	if (optResult == EAlreadyFinish) roundSettlement(roomData, userName, hallId, roomId, opt);
}

void CGoldenFraudMsgHandler::doCompareCard(SGoldenFraudRoom& roomData, SGoldenFraudData& active, SGoldenFraudData& passive)
{
	// 比牌结果
	if (active.cardValue == com_protocol::EGoldenFraudCardType::EThreeSameCard
		&& passive.cardValue == com_protocol::EGoldenFraudCardType::ESpecialCard)
	{
		active.playerFlag = ECompareLose;
	}
	else if (active.cardValue == com_protocol::EGoldenFraudCardType::ESpecialCard
		     && passive.cardValue == com_protocol::EGoldenFraudCardType::EThreeSameCard)
	{
		passive.playerFlag = ECompareLose;
	}
	else if (active.cardValue > passive.cardValue)
	{
		passive.playerFlag = ECompareLose;
	}
	else if (passive.cardValue > active.cardValue)
	{
		active.playerFlag = ECompareLose;
	}
	else
	{
		// 降序排序
		HandCard actCd = {0};
		memcpy(actCd, active.handCard, sizeof(HandCard));
		descendSort(actCd);
		
		HandCard pasCd = {0};
		memcpy(pasCd, passive.handCard, sizeof(HandCard));
		descendSort(pasCd);
		
		for (unsigned int idx = 0; idx < CardCount; ++idx)
		{
			unsigned int actCdVal = actCd[idx] % PokerCardBaseValue;
			unsigned int pasCdVal = pasCd[idx] % PokerCardBaseValue;
			
			if (actCdVal == 1) actCdVal += GoldenFraudCardA;
			if (pasCdVal == 1) pasCdVal += GoldenFraudCardA;
			
			if (actCdVal > pasCdVal)
			{
				passive.playerFlag = ECompareLose;
				break;
			}
			else if (pasCdVal > actCdVal)
			{
				active.playerFlag = ECompareLose;
				break;
			}
		}
		
		if (passive.playerFlag != ECompareLose) active.playerFlag = ECompareLose;
	}

    // 比牌记录
	roomData.compareCardResult[active.username].push_back(SCompareCardResult(passive.username, (passive.playerFlag == ECompareLose) ? 0 : 1));
	roomData.compareCardResult[passive.username].push_back(SCompareCardResult(active.username, (active.playerFlag == ECompareLose) ? 0 : 1));
}

void CGoldenFraudMsgHandler::compareCardOpt(const char* hallId, const char* roomId, SGoldenFraudRoom& roomData,
                                            SGoldenFraudData& active, SGoldenFraudData& passive, const char* info)
{
	doCompareCard(roomData, active, passive);
		
	const SGoldenFraudData& loser = (active.playerFlag == ECompareLose) ? active : passive;
	roomData.playerInfo[loser.username].set_status(com_protocol::EGoldenFraudPlayerStatus::EGoldenFraudLose);
	
	// 比牌输家，结算扣除金币
	changeUserGold(hallId, roomId, roomData, loser.username, -(int)loser.betValue, info);
}

EOptResult CGoldenFraudMsgHandler::compareCardOpt(SGoldenFraudRoom& roomData, SGoldenFraudData& currentPlayerData,
                                                  unsigned int hallId, unsigned int roomId, const string& compareUsername,
												  com_protocol::GoldenFraudPlayerOptNotify& optNtf)
{
	com_protocol::GoldenFraudCompareCardPlayer* comparePlayer = optNtf.add_compare_player();
	if (currentPlayerData.playerFlag == ECompareLose)
	{
		comparePlayer->set_lose_username(currentPlayerData.username);
		comparePlayer->set_win_username(compareUsername);
	}
	else
	{
		comparePlayer->set_lose_username(compareUsername);
		comparePlayer->set_win_username(currentPlayerData.username);
	}
	
	if (roomData.getWinnerData() != NULL) return EAlreadyFinish;  // 剩下最后的两个人，比牌会导致结算
	
	if (!roomData.isGoldRoom() || currentPlayerData.playerFlag == ECompareLose
	    || roomData.playerInfo[currentPlayerData.username].detail_info().dynamic_info().game_gold() > 0.001) return ECurrentOpt;
	
	// 金币场，当前玩家比牌赢了，但该玩家已经没有金币了则强制该玩家和在场的其他玩家比牌
	const int compareIdx = roomData.getPlayerIndex(compareUsername);
	if (compareIdx < 0)
	{
		OptErrorLog("get compare player index error, current username = %s, compare username = %s, hallId = %u, roomId = %u",
					currentPlayerData.username.c_str(), compareUsername.c_str(), hallId, roomId);
		
		return EOptError;
	}

    char strHallId[IdMaxLen] = {0};
	snprintf(strHallId, sizeof(strHallId) - 1, "%u", hallId);
	
	char strRoomId[IdMaxLen] = {0};
	snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", roomId);

	unsigned int currentIdx = compareIdx;
	for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
	{
		++currentIdx %= PlayerMaxCount;
		if (currentIdx == (unsigned int)compareIdx) break;
		
		SGoldenFraudData& cmpPlayerData = roomData.players[currentIdx];
		if (cmpPlayerData.playerFlag == EOnPlay && cmpPlayerData.username != currentPlayerData.username)
		{
			compareCardOpt(strHallId, strRoomId, roomData, currentPlayerData, cmpPlayerData, "no gold compare card");
			
			comparePlayer = optNtf.add_compare_player();
			if (currentPlayerData.playerFlag == ECompareLose)
			{
				comparePlayer->set_lose_username(currentPlayerData.username);
				comparePlayer->set_win_username(cmpPlayerData.username);
				
				break;
			}
			else
			{
				comparePlayer->set_lose_username(cmpPlayerData.username);
				comparePlayer->set_win_username(currentPlayerData.username);
			}
		}
	}
	
	return (roomData.getWinnerData() != NULL) ? EAlreadyFinish : ECurrentOpt;  // 剩下最后的两个人，比牌会导致结算
}

void CGoldenFraudMsgHandler::roundSettlement(SGoldenFraudRoom& roomData, const string& userName, unsigned int hallId, unsigned int roomId, int opt)
{
	m_srvOpt.stopTimer(roomData.optTimerId);

	if (roomData.isGoldRoom()) goldSettlement(roomData, userName, hallId, roomId, opt);
	else cardSettlement(roomData, userName, hallId, roomId, opt);
}

bool CGoldenFraudMsgHandler::doSettlement(SGoldenFraudRoom& roomData, const string& userName, unsigned int hallId, unsigned int roomId, int opt)
{
	GameSettlementMap gameSettlementInfo;
	const bool isGoldRoom = roomData.isGoldRoom();
	if (roomData.isFinishRound())  // 轮次完毕导致的结算
	{
		const int curIdx = roomData.getPlayerIndex(userName);
		if (curIdx < 0)
		{
			OptErrorLog("settlement get current player index error, username = %s, hallId = %u, roomId = %u, opt = %d",
						userName.c_str(), hallId, roomId, opt);
			return false;
		}
		
		com_protocol::GoldenFraudCompareCardResultNotify cmpCdResultNtf;
		SGoldenFraudData* cmpData = NULL;
		int idx = curIdx + 1;
		while (true)
		{
			idx %= PlayerMaxCount;
			if (roomData.players[idx].playerFlag == EOnPlay)
			{
				if (cmpData != NULL)
				{
					// 比牌
					com_protocol::GoldenFraudCompareCardPlayer* cmpCdPlayer = cmpCdResultNtf.add_compare_player();
					doCompareCard(roomData, *cmpData, roomData.players[idx]);
					
					if (cmpData->playerFlag == ECompareLose)
					{
						cmpCdPlayer->set_lose_username(cmpData->username);
						cmpData = &roomData.players[idx];
					}
					else
					{
						cmpCdPlayer->set_lose_username(roomData.players[idx].username);
					}
					cmpCdPlayer->set_win_username(cmpData->username);
					
					const SGoldenFraudData* loser = roomData.getPlayerData(cmpCdPlayer->lose_username());
					roomData.playerInfo[loser->username].set_status(com_protocol::EGoldenFraudPlayerStatus::EGoldenFraudLose);
					
					if (isGoldRoom)
					{
						com_protocol::ItemChange& item = gameSettlementInfo[loser->username];
						item.set_type(EGameGoodsType::EGoodsGold);
						item.set_num(-(int)loser->betValue);
					}
				}
				else
				{
					cmpData = &roomData.players[idx];
				}
			}
			
			if (idx == curIdx) break;
			
			++idx;
		}
		
		if (cmpCdResultNtf.compare_player_size() > 0)
		{
			sendPkgMsgToRoomPlayers(roomData, cmpCdResultNtf, GOLDENFRAUD_COMPARE_CARD_RESULT_NOTIFY, "compare result notify");
		}
	}

	// 结算消息
	SGoldenFraudData* winner = roomData.getWinnerData();
	if (winner == NULL)
	{
		OptErrorLog("settlement get winner error, username = %s, hallId = %u, roomId = %u, opt = %d",
					userName.c_str(), hallId, roomId, opt);
		return false;
	}
	
	unsigned int winValue = roomData.currentAllBet - winner->betValue;  // 本次玩家盈利赢了多少
	if (isGoldRoom)
	{
		com_protocol::ItemChange& item = gameSettlementInfo[winner->username];
		item.set_type(EGameGoodsType::EGoodsGold);
		item.set_num(winValue);
		moreUsersGoodsSettlement(hallId, roomId, roomData, gameSettlementInfo);
		
		// moreUsersGoodsSettlement 调用会根据比例对赢家金币扣税
		// item.num() 是扣税后实际盈利赢的金币
		winValue = item.num();
	}

	com_protocol::GoldenFraudSettlementNotify settlementNtf;
	com_protocol::GoldenFraudWinner* stlWinner = settlementNtf.mutable_winner();
	stlWinner->set_username(winner->username);
	stlWinner->set_win_value(winValue);
	
	roomData.winBet(winValue + winner->betValue, *winner);
	if (isGoldRoom) stlWinner->set_current_gold(roomData.playerInfo[winner->username].detail_info().dynamic_info().game_gold());

	// 结算消息发送给旁观的玩家
	sendPkgMsgToRoomPlayers(roomData, settlementNtf, GOLDENFRAUD_SETTLEMENT_NOTIFY, "settlement notify", "", -1,
							com_protocol::ERoomPlayerStatus::EPlayerEnter);
	
	// 游戏玩家
	for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
	{
		const SGoldenFraudData& playerData = roomData.players[idx];
		ConnectProxy* conn = getConnectProxy(playerData.username);
		if (conn != NULL)
		{
			settlementNtf.clear_compare_card_info();
			settlementNtf.clear_card_value();
			settlementNtf.clear_card_type();

			CompareCardResultMap::const_iterator ccrIt = roomData.compareCardResult.find(playerData.username);
			if (ccrIt != roomData.compareCardResult.end())
			{
				SGoldenFraudData* cmpPlData = NULL;
				for (CompareCardResultVector::const_iterator it = ccrIt->second.begin(); it != ccrIt->second.end(); ++it)
				{
					cmpPlData = roomData.getPlayerData(it->username);
					if (cmpPlData != NULL)
					{
						com_protocol::GoldenFraudPlayerCard* playerCard = settlementNtf.add_compare_card_info();
						playerCard->set_username(it->username);
						for (unsigned int cdIdx = 0; cdIdx < CardCount; ++cdIdx) playerCard->add_card_value(cmpPlData->handCard[cdIdx]);
						playerCard->set_card_type((com_protocol::EGoldenFraudCardType)cmpPlData->cardValue);
						playerCard->set_is_winner(it->result);
					}
				}
				
				if (playerData.viewCardFlag == ENotViewCard)
				{
				    for (unsigned int cdIdx = 0; cdIdx < CardCount; ++cdIdx) settlementNtf.add_card_value(playerData.handCard[cdIdx]);
				    settlementNtf.set_card_type((com_protocol::EGoldenFraudCardType)playerData.cardValue);
				}
			}

			m_srvOpt.sendMsgPkgToProxy(settlementNtf, GOLDENFRAUD_SETTLEMENT_NOTIFY, conn, "settlement notify");
		}
	}
	
	OptInfoLog("settlement notify, room type = %d, play times = %u, round = %u, all bet = %u, win player = %s, bet = %u, win value = %u",
    roomData.roomInfo.base_info().room_type(), roomData.playTimes, roomData.currentRoundTimes, roomData.currentAllBet,
	winner->username.c_str(), winner->betValue, winValue);
	
	return true;
}

// 一局金币场结算
void CGoldenFraudMsgHandler::goldSettlement(SGoldenFraudRoom& roomData, const string& userName, unsigned int hallId, unsigned int roomId, int opt)
{
	// 如果结算失败就解散房间
	if (!doSettlement(roomData, userName, hallId, roomId, opt))
	{
		delRoomData(hallId, roomId);

		return;
	}
	
	ConstCharPointerVector usernames;
	roomData.getCurrentPlayer(usernames);
	setReEnterInfo(usernames);    // 重置重连信息
	delHearbeatCheck(usernames);  // 删除玩家心跳检测
	
	// 接着开始准备下一局游戏
	prepareNextTime(hallId, roomId, roomData);
}

// 一局房卡场结算
void CGoldenFraudMsgHandler::cardSettlement(SGoldenFraudRoom& roomData, const string& userName, unsigned int hallId, unsigned int roomId, int opt)
{
	// 如果结算失败就解散房间
	if (!doSettlement(roomData, userName, hallId, roomId, opt))
	{
		delRoomData(hallId, roomId);

		return;
	}

    // 保存本局结果
	RecordIDType recordId = {0};
	m_srvOpt.getRecordId(recordId);
	com_protocol::RoundWinLoseInfo* roundWinLoseInfo = roomData.roomGameRecord.add_round_win_lose();
	roundWinLoseInfo->set_record_id(recordId);
    for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
	{
		SGoldenFraudData& playerData = roomData.players[idx];
		if (playerData.playerFlag == ENobody) continue;

        com_protocol::ChangeRecordPlayerInfo* playerInfo = roundWinLoseInfo->add_player_info();
        playerInfo->set_card_result(playerData.cardValue);
        playerInfo->set_card_rate(0);
        playerInfo->set_card_info((const char*)playerData.handCard, CardCount);
        playerInfo->set_player_flag(playerData.isNewBanker ? 1 : 0);
		playerInfo->set_bet_value(playerData.betValue);
		playerInfo->set_win_lose_value(int((playerData.playerFlag == EOnPlay) ? (roomData.currentAllBet - playerData.betValue) : -playerData.betValue));
		playerInfo->set_username(playerData.username);
		
		OptInfoLog("test only record, username = %s, betValue = %u, allBet = %u, sumValue = %d, win lose = %.2f",
        playerData.username.c_str(), playerData.betValue, roomData.currentAllBet, playerData.sumWinLoseValue,
		playerInfo->win_lose_value());
	}
	
	char strHallId[IdMaxLen] = {0};
	snprintf(strHallId, sizeof(strHallId) - 1, "%u", hallId);
	
	char strRoomId[IdMaxLen] = {0};
	snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", roomId);

	if (roomData.playTimes >= roomData.roomInfo.base_info().game_times())
	{
		ConstCharPointerVector usernames;
		roomData.getCurrentPlayer(usernames);
		setReEnterInfo(usernames);    // 重置重连信息
		delHearbeatCheck(usernames);  // 删除玩家心跳检测
		
		// 房卡总结算记录
		cardCompleteSettlement(strHallId, strRoomId, roomData);

		// 结束解散房间
		delRoomData(hallId, roomId, false);
		
		return;
	}
	
	// 如果是第一局则扣除房卡付费
	if (roomData.playTimes == 1) payRoomCard(strHallId, strRoomId, roomData);  // 房卡扣费
	
	// 准备下一局游戏
	// 重新设置游戏状态为准备
	roomData.gameStatus = com_protocol::EGameStatus::EGameReady;
	roomData.optTimeSecs = 0;
	roomData.optTimerId = 0;
	roomData.optLastTimeSecs = 0;
	roomData.nextOptType = 0;

    // 玩家复位为进入房间状态
	for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
	{
		SGoldenFraudData& playerData = roomData.players[idx];
		if (playerData.playerFlag == ENobody) continue;

        RoomPlayerIdInfoMap::iterator it = roomData.playerInfo.find(playerData.username);
		if (it != roomData.playerInfo.end()) it->second.set_status(com_protocol::ERoomPlayerStatus::EPlayerEnter);
	}

	// 重新变更房间的状态为游戏准备中
	roomData.roomStatus = com_protocol::EHallRoomStatus::EHallRoomReady;
	m_srvOpt.changeRoomDataNotify(strHallId, strRoomId, GoldenFraudGame, com_protocol::EHallRoomStatus::EHallRoomReady,
								  NULL, NULL, NULL, "room ready again");

    // 设置准备超时时间点
	com_protocol::ClientPrepareGameNotify preGmNtf;
	preGmNtf.set_wait_secs(m_srvOpt.getGoldenFraudBaseCfg().common_cfg.prepare_game_secs);
	setOptTimeOutInfo(hallId, roomId, roomData, preGmNtf.wait_secs(),
					  this, (TimerHandler)&CGoldenFraudMsgHandler::prepareTimeOut, EOptType::EPrepareOpt);
	
	sendPkgMsgToRoomPlayers(roomData, preGmNtf, COMM_PREPARE_GAME_NOTIFY, "prepare next game");

	OptInfoLog("card prepare next time, hallId = %u, roomId = %u, game status = %d, times = %u, user count = %u, address = %p",
    hallId, roomId, roomData.gameStatus, roomData.playTimes, (unsigned int)roomData.playerInfo.size(), &roomData);
}

// 准备下一局金币场游戏
void CGoldenFraudMsgHandler::prepareNextTime(unsigned int hallId, unsigned int roomId, SGoldenFraudRoom& roomData)
{
	// 1）重新设置游戏状态为准备
	roomData.gameStatus = com_protocol::EGameStatus::EGameReady;
	roomData.optTimeSecs = 0;
	roomData.optTimerId = 0;
	roomData.optLastTimeSecs = 0;
	roomData.nextOptType = 0;

	// 2）准备下一局游戏之前，先删除掉线了的玩家，可能会导致房间解散（如果房间里的玩家全是掉线玩家）
	// 因此清理掉线用户之前必须先检查房间是否可以解散，如果可以解散则直接解散删除房间
	StringVector offlinePlayers;
	com_protocol::ClientLeaveRoomNotify lvRoomNotify;
	for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
	{
		SGoldenFraudData& playerData = roomData.players[idx];
		if (playerData.playerFlag == ENobody || playerData.isOnline) continue;

		// 同步离线玩家消息
		lvRoomNotify.set_username(playerData.username);
		sendPkgMsgToRoomPlayers(roomData, lvRoomNotify, COMM_PLAYER_LEAVE_ROOM_NOTIFY, "player leave room notify", playerData.username);
		
		// 需要删除的掉线玩家
		// 1）真正离线的玩家（已经走完离线流程，只剩下当局玩家数据和会话数据没有删除，即掉线后到目前为止没有重连进入房间的玩家）
		// 2）心跳离线的玩家（没有走过离线流程，所有数据都还没有删除）
		// 因此这里必须直接删除离线玩家的游戏数据
		offlinePlayers.push_back(playerData.username);
		roomData.playerInfo.erase(playerData.username);
		playerData.playerFlag = ENobody;
		playerData.username.clear();
	}
	
	// 3）没有在线玩家了（全部离线）则解散删除房间
	if (!roomData.hasOnlineUser())
	{
		delRoomData(hallId, roomId);

		return;
	}

    // 4）剩余的在线玩家复位为进入房间状态
	for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
	{
		SGoldenFraudData& playerData = roomData.players[idx];
		if (playerData.playerFlag == ENobody) continue;

        RoomPlayerIdInfoMap::iterator it = roomData.playerInfo.find(playerData.username);
		if (it != roomData.playerInfo.end()) it->second.set_status(com_protocol::ERoomPlayerStatus::EPlayerEnter);
	}

	// 5）重新变更房间的状态为游戏准备中
	roomData.roomStatus = com_protocol::EHallRoomStatus::EHallRoomReady;
	char strHallId[IdMaxLen] = {0};
	snprintf(strHallId, sizeof(strHallId) - 1, "%u", hallId);
	
	char strRoomId[IdMaxLen] = {0};
	snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", roomId);
	m_srvOpt.changeRoomDataNotify(strHallId, strRoomId, GoldenFraudGame, com_protocol::EHallRoomStatus::EHallRoomReady,
								  NULL, NULL, NULL, "room ready again");

	// 6）最后才清理离线玩家
	if (!offlinePlayers.empty())
	{
		// 如果是已经走完离线流程的玩家那只剩下会话数据待删除
		const unsigned int count = delUserFromRoom(offlinePlayers);
		OptWarnLog("game prepare and clear offline player, size = %u, count = %u", offlinePlayers.size(), count);
	}
	
	OptInfoLog("prepare next time, hallId = %u, roomId = %u, game status = %d, times = %u, user count = %u, address = %p",
    hallId, roomId, roomData.gameStatus, roomData.playTimes, (unsigned int)roomData.playerInfo.size(), &roomData);
}


void CGoldenFraudMsgHandler::prepareTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 也可使用timerId遍历找到房间数据，但效率较低
	char strHallId[IdMaxLen] = {0};
	snprintf(strHallId, sizeof(strHallId) - 1, "%u", userId);
	
	char strRoomId[IdMaxLen] = {0};
	snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", (unsigned int)(unsigned long)param);

	SGoldenFraudRoom* roomData = (SGoldenFraudRoom*)getRoomDataById(strRoomId, strHallId);

	OptInfoLog("prepare time out, hallId = %u, roomId = %u, roomData = %p, timerId = %u",
	           userId, (unsigned int)(unsigned long)param, roomData, timerId);
	
	if (roomData == NULL) return;

	for (unsigned int idx = 0; idx < PlayerMaxCount; ++idx)
	{
		SGoldenFraudData& playerData = roomData->players[idx];
		if (playerData.playerFlag == ENobody) continue;

        RoomPlayerIdInfoMap::iterator it = roomData->playerInfo.find(playerData.username);
		if (it != roomData->playerInfo.end() && it->second.seat_id() >= 0) it->second.set_status(com_protocol::ERoomPlayerStatus::EPlayerReady);
	}

    roomData->optTimerId = 0;
	onStartGame(strHallId, strRoomId, *roomData); // 直接开始游戏
}

void CGoldenFraudMsgHandler::playerOptTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 也可使用timerId遍历找到房间数据，但效率较低
	SGoldenFraudRoom* roomData = (SGoldenFraudRoom*)getRoomData(userId, (unsigned int)(unsigned long)param);
	
	OptInfoLog("player opt time out, hallId = %u, roomId = %u, roomData = %p, timerId = %u",
	           userId, (unsigned int)(unsigned long)param, roomData, timerId);
	
	if (roomData != NULL)
	{
		roomData->optTimerId = 0;

		// 超时则默认弃牌
		SGoldenFraudData& playerData = roomData->players[roomData->currentPlayerIdx];
		playerData.playerFlag = EGiveUpCard;
		roomData->playerInfo[playerData.username].set_status(com_protocol::EGoldenFraudPlayerStatus::EGoldenFraudGiveUp);

        // 弃牌结算扣除金币
        changeUserGold(userId, (unsigned int)(unsigned long)param, *roomData,
					   playerData.username, -(int)playerData.betValue, "time out give up card");
		
		// 发送玩家操作通知
		doNextOptAndNotify(*roomData, playerData.username, userId, (unsigned int)(unsigned long)param,
		                   com_protocol::EGoldenFraudOptType::EDiscardCard);
	}
}


void CGoldenFraudMsgHandler::giveUp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	GameUserData* cud = (GameUserData*)m_srvOpt.getConnectProxyUserData();
	SGoldenFraudRoom* roomData = NULL;
	SGoldenFraudData* playerData = NULL;
	
	com_protocol::GoldenFraudPlayerGiveUpRsp rsp;
	rsp.set_result(getOptData(cud, roomData, playerData));
	
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, GOLDENFRAUD_GIVE_UP_RSP, "player give up card");
	OptInfoLog("player give up card reply, username = %s, hallId = %s, roomId = %s, seatId = %d, result = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rsp.result(), rc);

	if (rsp.result() == SrvOptSuccess)
	{
		// 弃牌结算扣除金币
		changeUserGold(cud->hallId, cud->roomId, *roomData,
					   cud->userName, -(int)playerData->betValue, "give up card");

		// 变更状态为弃牌
		playerData->playerFlag = EGiveUpCard;
		roomData->playerInfo[cud->userName].set_status(com_protocol::EGoldenFraudPlayerStatus::EGoldenFraudGiveUp);

		// 发送玩家操作通知
	    doNextOptAndNotify(*roomData, cud, com_protocol::EGoldenFraudOptType::EDiscardCard);
	}
}

void CGoldenFraudMsgHandler::viewCard(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GoldenFraudPlayerViewCardRsp rsp;
	GameUserData* cud = (GameUserData*)m_srvOpt.getConnectProxyUserData();
	SGoldenFraudRoom* roomData = NULL;
	SGoldenFraudData* playerData = NULL;
	do
	{
	    rsp.set_result(getOptData(cud, roomData, playerData));
		if (rsp.result() != SrvOptSuccess) break;
		
		if (playerData->viewCardFlag != ENotViewCard)
		{
			rsp.set_result(GoldenFraudAlreadyViewCard);
			break;
		}
		
		if (!roomData->canViewCard())
		{
			rsp.set_result(GoldenFraudRoundCanNotViewCard);
			break;
		}
		
		playerData->viewCardFlag = EAlreadyViewCard;
		
		for (unsigned int idx = 0; idx < CardCount; ++idx) rsp.add_card_value(playerData->handCard[idx]);
		rsp.set_card_type((com_protocol::EGoldenFraudCardType)playerData->cardValue);
		rsp.set_next_follow_bet(roomData->getFollowBet(EAlreadyViewCard));

	} while (false);
	
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, GOLDENFRAUD_VIEW_CARD_RSP, "player view card");
	OptInfoLog("player view card reply, username = %s, hallId = %s, roomId = %s, seatId = %d, result = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rsp.result(), rc);

    // 成功则发送玩家操作通知
	if (rsp.result() == SrvOptSuccess) doNextOptAndNotify(*roomData, cud, com_protocol::EGoldenFraudOptType::EViewCard);
}

void CGoldenFraudMsgHandler::doBet(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GoldenFraudPlayerDoBetReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "player do bet request")) return;
	
	com_protocol::GoldenFraudPlayerDoBetRsp rsp;
	GameUserData* cud = (GameUserData*)m_srvOpt.getConnectProxyUserData();
	SGoldenFraudRoom* roomData = NULL;
	SGoldenFraudData* playerData = NULL;
    unsigned int betValue = 0;
	string compareUsername;

	do
	{
		rsp.set_result(getOptData(cud, roomData, playerData));
		if (rsp.result() != SrvOptSuccess) break;
		
		if (roomData->gameStatus == com_protocol::EGameStatus::EGameAllIn
		    && req.opt_type() != com_protocol::EGoldenFraudOptType::EAllInBet)
		{
			rsp.set_result(GameObjectGameStatusInvalidOpt);
			break;
		}
		
		if (roomData->players[roomData->currentPlayerIdx].username != cud->userName)
		{
			rsp.set_result(GoldenFraudNotCurrentOptPlayer);
			break;
		}

        SGoldenFraudData* activeAllInPlayer = NULL;
        unsigned int currentFollowBet = roomData->currentFollowBet;
        betValue = roomData->getCurrentPlayerFollowBet();
        if (req.opt_type() == com_protocol::EGoldenFraudOptType::EIncreaseBet)
		{
			// 加注比例列表
			const NGoldenFraudBaseConfig::GoldenFraudBaseConfig& goldenFraudCfg = m_srvOpt.getGoldenFraudBaseCfg();
			const vector<unsigned int>& betRate = roomData->isGoldRoom() ? goldenFraudCfg.gold_bet_rate : goldenFraudCfg.card_bet_rate;
			if (std::find(betRate.begin(), betRate.end(),
			              req.increase_bet_value() / roomData->roomInfo.base_info().base_rate()) == betRate.end())
			{
				rsp.set_result(GoldenFraudNotExistIncreaseBet);
				break;
			}
			
			if (req.increase_bet_value() < betValue)
			{
				rsp.set_result(GoldenFraudIncreaseBetInvalid);
				break;
			}

			currentFollowBet = req.increase_bet_value();
			betValue = (playerData->viewCardFlag == ENotViewCard) ? currentFollowBet : currentFollowBet * 2;
		}
		else if (req.opt_type() == com_protocol::EGoldenFraudOptType::EAllInBet)
		{
			if (roomData->gameStatus != com_protocol::EGameStatus::EGameAllIn)
			{
				if (!roomData->isGoldRoom())
				{
					rsp.set_result(GoldenFraudNotGoldTypeAllIn);
					break;
				}
				
				if (!roomData->canAllIn())
				{
					rsp.set_result(GoldenFraudAllInLimit);
					break;
				}
				
				float gameGold = roomData->playerInfo[cud->userName].detail_info().dynamic_info().game_gold();
				if (gameGold < betValue)
				{
					rsp.set_result(GoldenFraudGoldInsufficient);
					break;
				}
				betValue = gameGold;

				SGoldenFraudData* allInOther = roomData->getAllInOtherPlayer(cud->userName);
				gameGold = roomData->playerInfo[allInOther->username].detail_info().dynamic_info().game_gold();
				if (gameGold < betValue) betValue = gameGold;
				
				currentFollowBet = betValue;
				roomData->gameStatus = com_protocol::EGameStatus::EGameAllIn;
			}
			else if ((activeAllInPlayer = roomData->getAllInOtherPlayer(cud->userName)) == NULL)
			{
				rsp.set_result(GoldenFraudAllInLimit);
				break;
			}
		}
		
		rsp.set_result(roomData->payBet(betValue, *playerData));
		if (rsp.result() != SrvOptSuccess) break;
		
		roomData->currentFollowBet = currentFollowBet;
		if (activeAllInPlayer != NULL)
		{
			compareCardOpt(cud->hallId, cud->roomId, *roomData, *activeAllInPlayer, *playerData, "all in compare card lose");

		    req.set_opt_type(com_protocol::EGoldenFraudOptType::ECompareCard);
			compareUsername = activeAllInPlayer->username;
		}

	} while (false);
	
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, GOLDENFRAUD_DO_BET_RSP, "player do bet reply");
	OptInfoLog("player do bet reply, username = %s, hallId = %s, roomId = %s, seatId = %d, result = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rsp.result(), rc);

    // 成功则发送玩家操作通知
	if (rsp.result() == SrvOptSuccess) doNextOptAndNotify(*roomData, cud, req.opt_type(), betValue, compareUsername);
}

void CGoldenFraudMsgHandler::compareCard(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GoldenFraudPlayerCompareCardReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "player compare card request")) return;
	
	com_protocol::GoldenFraudPlayerCompareCardRsp rsp;
	GameUserData* cud = (GameUserData*)m_srvOpt.getConnectProxyUserData();
	SGoldenFraudRoom* roomData = NULL;
	SGoldenFraudData* playerData = NULL;
	unsigned int betValue = 0;

	do
	{
		rsp.set_result(getOptData(cud, roomData, playerData));
		if (rsp.result() != SrvOptSuccess) break;
		
		if (roomData->players[roomData->currentPlayerIdx].username != cud->userName)
		{
			rsp.set_result(GoldenFraudNotCurrentOptPlayer);
			break;
		}
		
		if (!roomData->canCompareCard())
		{
			rsp.set_result(GoldenFraudRoundCanNotCompareCard);
			break;
		}
		
		SGoldenFraudData* cmpPlDt = roomData->getPlayerData(req.compare_username());
	    if (cmpPlDt == NULL || cmpPlDt->playerFlag != EOnPlay)
		{
			rsp.set_result(GoldenFraudNotFoundPlayer);
			break;
		}

        // 比牌要付出双倍下注额
		betValue = roomData->getCurrentPlayerFollowBet() * 2;
		if (roomData->isGoldRoom())
		{
			const com_protocol::DynamicInfo& dynamicInfo = roomData->playerInfo[cud->userName].detail_info().dynamic_info();
			if (dynamicInfo.game_gold() < betValue) betValue = dynamicInfo.game_gold();
		}
		
		rsp.set_result(roomData->payBet(betValue, *playerData));
		if (rsp.result() != SrvOptSuccess) break;
		
		compareCardOpt(cud->hallId, cud->roomId, *roomData, *playerData, *cmpPlDt, "compare card lose");

	} while (false);
	
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, GOLDENFRAUD_COMPARE_CARD_RSP, "player compare card reply");
	OptInfoLog("player compare card reply, username = %s, hallId = %s, roomId = %s, seatId = %d, result = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rsp.result(), rc);

    // 成功则发送玩家操作通知
	if (rsp.result() == SrvOptSuccess)
	{
		doNextOptAndNotify(*roomData, cud, com_protocol::EGoldenFraudOptType::ECompareCard, betValue, req.compare_username());
	}
}





static CGoldenFraudMsgHandler s_msgHandler;  // 消息处理模块实例

int CGoldenFraudSrv::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("run game service name = %s, id = %d", name, id);
	return 0;
}

void CGoldenFraudSrv::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("stop game service name = %s, id = %d", name, id);
}

void CGoldenFraudSrv::onRegister(const char* name, const unsigned int id)
{
	// 注册模块实例
	const unsigned short HandlerMessageModuleId = 0;
	registerModule(HandlerMessageModuleId, &s_msgHandler);
	
	ReleaseInfoLog("register game module, service name = %s, id = %d", name, id);
}

void CGoldenFraudSrv::onUpdateConfig(const char* name, const unsigned int id)
{
	s_msgHandler.onUpdateConfig();
}

// 通知逻辑层对应的逻辑连接已被关闭
void CGoldenFraudSrv::onCloseConnectProxy(void* userData, int cbFlag)
{
	s_msgHandler.onCloseConnectProxy(userData, cbFlag);
}


CGoldenFraudSrv::CGoldenFraudSrv() : IService(GoldenFraudGame)
{
}

CGoldenFraudSrv::~CGoldenFraudSrv()
{
}

REGISTER_SERVICE(CGoldenFraudSrv);

}

