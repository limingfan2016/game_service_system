
/* author : limingfan
 * date : 2017.11.21
 * description : 牛牛游戏过程
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>

#include "CCattles.h"
#include "CGameProcess.h"


using namespace NProject;

namespace NPlatformService
{

// 数据记录日志
#define WriteDataLog(format, args...)     m_msgHandler->getSrvOpt().getDataLogger()->writeFile(NULL, 0, LogLevel::Info, format, ##args)


CGameProcess::CGameProcess()
{
	m_msgHandler = NULL;
}

CGameProcess::~CGameProcess()
{
	m_msgHandler = NULL;
}

int CGameProcess::init(CCattlesMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CATTLES_PLAYER_CHOICE_BANKER_REQ, (ProtocolHandler)&CGameProcess::playerChoiceBanker, this);
    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CATTLES_CLIENT_PERFORM_FINISH_NOTIFY, (ProtocolHandler)&CGameProcess::performFinish, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CATTLES_PLAYER_CHOICE_BET_REQ, (ProtocolHandler)&CGameProcess::playerChoiceBet, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CATTLES_PLAYER_OPEN_CARD_REQ, (ProtocolHandler)&CGameProcess::playerOpenCard, this);
	
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_CHANGE_MORE_USERS_ITEM_RSP, (ProtocolHandler)&CGameProcess::changeMoreUsersGoodsReply, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_SET_ROOM_GAME_RECORD_RSP, (ProtocolHandler)&CGameProcess::setRoomGameRecordReply, this);

	return SrvOptSuccess;
}

void CGameProcess::unInit()
{
}

void CGameProcess::updateConfig()
{

}

// 开始游戏
void CGameProcess::startGame(const char* hallId, const char* roomId, SCattlesRoomData& roomData)
{
	const com_protocol::ECattleBankerType bankerType = roomData.roomInfo.cattle_room().type();
    if (!hasNextNewBanker(roomData))  // 是否存在下一局新庄家
	{
		OptErrorLog("start game but can not find next new banker error, hallId = %s, roomId = %s, banker type = %d",
		hallId, roomId, bankerType);
		return;  // 出错了，这里可以考虑直接解散房间
	}
	
	// 1）必须在清空上一局玩家信息之前，先获取上一局庄家信息及当前局庄家信息（牛牛上庄、固定庄家）
	string lastBanker;
	roomData.getLastBanker(&lastBanker);

    // 2）如果是牛牛上庄、固定庄家，则在清空上一局玩家信息之前先计算出庄家
    string newBanker;
	if (bankerType == com_protocol::ECattleBankerType::ECattleBanker || bankerType == com_protocol::ECattleBankerType::EFixedBanker)
	{
        if (!getCattlesFixedNextNewBanker(roomData, newBanker))
        {
            OptErrorLog("start game but get next new banker error, hallId = %s, roomId = %s, banker type = %d",
			hallId, roomId, bankerType);
		    return;  // 出错了，这里可以考虑直接解散房间
        }
	}

	// 一局游戏开局，设置玩家掉线信息
	// 掉线的方式存在多种，必须在游戏开始前设置（app异常关闭、网络异常、中间路由异常、心跳异常等等）
	// 如果玩家在游戏中掉线，则依据掉线信息重连回房间
	GameServiceData gameSrvData;
	gameSrvData.srvId = m_msgHandler->getSrvId();
	gameSrvData.hallId = atoi(hallId);
	gameSrvData.roomId = atoi(roomId);
	gameSrvData.status = EGamePlayerStatus::EPlayRoom;
	gameSrvData.timeSecs = time(NULL);
	
	// 3）重置当前局游戏玩家信息并设置玩家的掉线重连会话数据
	ConstCharPointerVector playerUsernames;  // 一起游戏的玩家
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		GamePlayerIdDataMap::iterator gamePlayerIt = roomData.gamePlayerData.find(it->first);
		if (it->second.status() == com_protocol::ERoomPlayerStatus::EPlayerReady)  // 本局游戏玩家
		{
			if (gamePlayerIt != roomData.gamePlayerData.end()) gamePlayerIt->second.reset();
			else roomData.gamePlayerData[it->first] = SGamePlayerData();

            // 如果房间解散时，掉线玩家还没有重入房间，则此数据需要删除（当局游戏结束清理掉线玩家时会删除）
			m_msgHandler->setUserSessionData(it->first.c_str(), it->first.length(), gameSrvData);

            playerUsernames.push_back(it->first.c_str());
		}
		else if (gamePlayerIt != roomData.gamePlayerData.end())
		{
			roomData.gamePlayerData.erase(gamePlayerIt);  // 删除非本局游戏玩家
		}
	}

	// 4）随机&计算玩家手牌
    if (!randomCalculatePlayerCard(roomData))
	{
		OptErrorLog("start game but random calculate player card error, hallId = %s, roomId = %s, banker type = %d",
		hallId, roomId, bankerType);
		return;  // 出错了，这里可以考虑直接解散房间
	}
	
	// 5）游戏开始了，房间状态变更为游戏中
	m_msgHandler->getSrvOpt().stopTimer(roomData.optTimerId);
	roomData.roomStatus = com_protocol::EHallRoomStatus::EHallRoomPlay;
	m_msgHandler->getSrvOpt().changeRoomDataNotify(hallId, roomId, CattlesGame, com_protocol::EHallRoomStatus::EHallRoomPlay,
												   NULL, NULL, NULL, "room play game");
	
    if (!lastBanker.empty()) roomData.gamePlayerData[lastBanker].isLastBanker = true;  // 上一局庄家

	m_msgHandler->getSrvOpt().setLastPlayersNotify(hallId, playerUsernames);  // 一起游戏的玩家通知DB缓存
	
	m_msgHandler->addHearbeatCheck(playerUsernames);  // 启动游戏玩家心跳检测

	// 等待客户端特效动画表现完毕后才开始游戏
	setOptTimeOutInfo(atoi(hallId), atoi(roomId), roomData, m_msgHandler->getSrvOpt().getCattlesBaseCfg().common_cfg.start_game_secs,
	(TimerHandler)&CGameProcess::doStartGame, EOptType::EStartGame);
	
	// 通知客户端开始游戏
	++roomData.playTimes;
	com_protocol::ClientStartGameNotify startGameNtf;
	startGameNtf.set_current_round(roomData.playTimes);
	m_msgHandler->sendPkgMsgToRoomPlayers(roomData, startGameNtf, COMM_START_GAME_NOTIFY, "start game notify", "");

    // 抢庄类型
	if (bankerType == com_protocol::ECattleBankerType::EFreeBanker
	    || bankerType == com_protocol::ECattleBankerType::EOpenCardBanker)
	{
		roomData.gameStatus = com_protocol::ECattlesGameStatus::ECattlesCompeteBanker;
		
		return;
	}

	// 直接确定庄家（牛牛上庄、固定庄家类型）
    roomData.gamePlayerData[newBanker].isNewBanker = true;
	roomData.gameStatus = com_protocol::ECattlesGameStatus::ECattlesConfirmBanker;
}

// 玩家离开房间
EGamePlayerStatus CGameProcess::playerLeaveRoom(GameUserData* cud, SCattlesRoomData& roomData, int quitStatus)
{
	// 服务停止或者房间可解散
	const EGamePlayerStatus playerStatus = playerLeaveRoom(cud->userName, roomData);
	if (com_protocol::EUserQuitStatus::EServiceStop == quitStatus)  // || canDisbandGameRoom(roomData))
	{
		m_msgHandler->delRoomData(cud->hallId, cud->roomId);  // 服务停止则解散删除房间
		
		return EGamePlayerStatus::ERoomDisband;
	}

	if (playerStatus == EGamePlayerStatus::ELeaveRoom)
	{
		// 玩家离开房间，同步发送消息到房间的其他玩家
		com_protocol::ClientLeaveRoomNotify lvRoomNotify;
		lvRoomNotify.set_username(cud->userName);
		m_msgHandler->sendPkgMsgToRoomPlayers(roomData, lvRoomNotify, COMM_PLAYER_LEAVE_ROOM_NOTIFY, "player leave room notify", cud->userName);
	}
	else if (playerStatus == EGamePlayerStatus::EDisconnectRoom)
	{
		// 玩家掉线，同步玩家状态，发送消息到房间的其他玩家
        m_msgHandler->updatePlayerStatusNotify(roomData, cud->userName, com_protocol::ERoomPlayerStatus::EPlayerOffline, cud->userName, "player disconnect notify");
	}
	
	// 玩家离开了，是否还存在新庄家
	if (!hasNextNewBanker(roomData))
	{
		// 庄家离开了且没有找到新庄家，则取消已经点击准备了的玩家状态，改为坐下状态
		StringVector players;
		for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
		{
			com_protocol::ClientRoomPlayerInfo& plInfo = roomData.playerInfo[it->first];
			if (plInfo.status() == com_protocol::ERoomPlayerStatus::EPlayerReady)
			{
				plInfo.set_status(com_protocol::ERoomPlayerStatus::EPlayerSitDown);
				
				players.push_back(it->first);
			}
		}
		
		// 同步玩家状态到房间的其他玩家
		if (!players.empty()) m_msgHandler->updatePlayerStatusNotify(roomData, players, com_protocol::ERoomPlayerStatus::EPlayerSitDown, cud->userName, "player change status to sit down notify");
		
		// 通知客户端没有庄家了（客户端准备按钮设置为灰色）
		// 玩家A，B，C，如果A离开了找不到新庄家，则发消息给B，C
		// 接着D玩家（不满足当庄条件）进入房间然后B玩家又离开了，此时要发消息给C，D玩家
		// 因此只要不存在庄家了则每次都发送该消息
		roomData.currentHasBanker = false;
		m_msgHandler->updatePrepareStatusNotify(roomData, com_protocol::ETrueFalseType::EFalseType, "player leave no banker");
	}
	
	return playerStatus;
}

// 是否可以解散房间
bool CGameProcess::canDisbandGameRoom(const SCattlesRoomData& roomData)
{
	if (!roomData.gamePlayerData.empty())
	{
		// 游戏中或者一局游戏刚结束，全部游戏玩家掉线则解散房间
		for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
		{
			if (it->second.isOnline) return false;  // 存在在线玩家则不能解散
		}
		
		return true;
	}
	
	int playerStatus = 0;
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		// 存在玩家坐下或者准备，不能解散游戏
		playerStatus = it->second.status();
		if (playerStatus == com_protocol::ERoomPlayerStatus::EPlayerSitDown
		    || playerStatus == com_protocol::ERoomPlayerStatus::EPlayerReady) return false;
	}
	
	return true;
}


void CGameProcess::setOptTimeOutInfo(unsigned int hallId, unsigned int roomId, SCattlesRoomData& roomData, unsigned int waitSecs, TimerHandler timerHandler, unsigned int optType)
{
	roomData.optLastTimeSecs = waitSecs;
	roomData.optTimeSecs = time(NULL);
	roomData.optTimerId = m_msgHandler->setTimer(waitSecs * MillisecondUnit, timerHandler, hallId, (void*)(unsigned long)roomId, 1, this);
	roomData.nextOptType = optType;
}

// 随机&计算玩家手牌
bool CGameProcess::randomCalculatePlayerCard(SCattlesRoomData& roomData)
{
	unsigned char pokerCard[BasePokerCardCount] = {0};
	memcpy(pokerCard, AllPokerCard, BasePokerCardCount * sizeof(unsigned char));
	
	// 牌型倍率配置
	const com_protocol::CattleRoomInfo& cattleRoomInfo = roomData.roomInfo.cattle_room(); // 房间信息
	const map<int, unsigned int>& baseRate = m_msgHandler->getSrvOpt().getCattlesBaseCfg().rate_rule_cfg[cattleRoomInfo.rate_rule_index()].cattles_value_rate;
	const map<int, unsigned int>& specialRate = m_msgHandler->getSrvOpt().getCattlesBaseCfg().special_card_rate;

	unsigned int pokerCardCount = BasePokerCardCount - 1;
	unsigned int randomIdx = 0;
	for (GamePlayerIdDataMap::iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		// 游戏玩家
		for (unsigned int idx = 0; idx < CattlesCardCount; ++idx)
		{
			// 随机玩家手牌
			randomIdx = CRandomNumber::getUIntNumber(0, pokerCardCount);
			it->second.poker[idx] = pokerCard[randomIdx];

			pokerCard[randomIdx] = pokerCard[pokerCardCount];

			if (--pokerCardCount < 1)
			{
				OptErrorLog("random calculate player card error, player count = %u, need card count = %u, all card count = %u",
	            roomData.gamePlayerData.size(), CattlesCardCount, BasePokerCardCount);
				
				return false;
            }
		}
		
		// 算出牌型、倍率
		it->second.settlementRate = 1;
		it->second.resultValue = getCattlesCardType(it->second.poker, cattleRoomInfo.special_card_type(), NULL);
		
		if (it->second.resultValue > com_protocol::ECattleBaseCardType::ECattleCattle)
		{
			map<int, unsigned int>::const_iterator spRateIt = specialRate.find(it->second.resultValue);
			if (spRateIt != specialRate.end()) it->second.settlementRate = spRateIt->second;
		}
		else
		{
			map<int, unsigned int>::const_iterator baseRateIt = baseRate.find(it->second.resultValue);
			if (baseRateIt != baseRate.end()) it->second.settlementRate = baseRateIt->second;
		}
	}
	
	return true;
}

// 玩家抢庄
bool CGameProcess::competeBanker(unsigned int hallId, unsigned int roomId, SCattlesRoomData& roomData)
{
	// 通知玩家抢庄
	com_protocol::CattlesCompeteBankerNotify cptBkNtf;
	cptBkNtf.set_wait_secs(m_msgHandler->getSrvOpt().getCattlesBaseCfg().common_cfg.compete_banker_secs);

    const com_protocol::ECattleBankerType bankerType = roomData.roomInfo.cattle_room().type();
    const unsigned int minBankerGold = m_msgHandler->getMinNeedBankerGold(roomData); // 庄家需要的最低金币数量
	for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		if (roomData.playerInfo[it->first].detail_info().dynamic_info().game_gold() >= minBankerGold)
		{
			cptBkNtf.set_is_can_compete(com_protocol::ETrueFalseType::ETrueType);
		}
		else
		{
			cptBkNtf.set_is_can_compete(com_protocol::ETrueFalseType::EFalseType);
		}
		
		cptBkNtf.clear_open_cards();
		if (bankerType == com_protocol::ECattleBankerType::EOpenCardBanker)
		{
			for (unsigned int idx = 0; idx < CattlesCardCount - 1; ++idx)
			{
				cptBkNtf.add_open_cards(it->second.poker[idx]);  // 明牌抢庄的4张手牌
			}
		}
		
		m_msgHandler->getSrvOpt().sendMsgPkgToProxy(cptBkNtf, CATTLES_COMPETE_BANKER_NOTIFY,
													"notify player choice banker", it->first.c_str());
	}
	
	// 通知非游戏玩家
	cptBkNtf.set_is_can_compete(com_protocol::ETrueFalseType::EFalseType);
	cptBkNtf.clear_open_cards();
	const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(cptBkNtf, "player choice banker notify");
	if (msgData != NULL)
	{	
		for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
		{
			if (it->second.status() < com_protocol::ERoomPlayerStatus::EPlayerReady)
			{
				m_msgHandler->getSrvOpt().sendMsgToProxyEx(msgData, cptBkNtf.ByteSize(), CATTLES_COMPETE_BANKER_NOTIFY, it->first.c_str());
			}
		}
	}

    // 游戏状态为开始抢庄，下一步等待玩家抢庄
	roomData.gameStatus = com_protocol::ECattlesGameStatus::ECattlesCompeteBanker;
	setOptTimeOutInfo(hallId, roomId, roomData, cptBkNtf.wait_secs(), (TimerHandler)&CGameProcess::competeBankerTimeOut, EOptType::ECompeteBanker);

    OptInfoLog("start choice banker, hallId = %u, roomId = %u, wait secs = %u, timerId = %u",
	           hallId, roomId, cptBkNtf.wait_secs(), roomData.optTimerId);

	return true;
}

// 确定庄家
bool CGameProcess::confirmBanker(unsigned int hallId, unsigned int roomId, SCattlesRoomData& roomData)
{
	m_msgHandler->getSrvOpt().stopTimer(roomData.optTimerId);

    const char* currentBanker = roomData.getCurrentBanker();
    if (currentBanker == NULL)
	{
		OptErrorLog("confirm banker but can not find next new banker error, hallId = %u, roomId = %u, banker type = %d",
		hallId, roomId, roomData.roomInfo.cattle_room().type());
		return false;  // 出错了，这里可以考虑直接解散房间
	}
    
	// 通知玩家已确认庄家，等待前端播放动画效果完毕后玩家才可以开始下注
	com_protocol::CattlesConfirmBankerNotify cfBkNtf;
	cfBkNtf.set_banker_id(currentBanker);
	cfBkNtf.set_wait_secs(m_msgHandler->getSrvOpt().getCattlesBaseCfg().common_cfg.confirm_banker_secs);
    
    const char* lastBanker = roomData.getLastBanker();
	if (lastBanker != NULL) cfBkNtf.set_last_time_banker(lastBanker);
	
	// 默认设置庄家的下注额及状态（防止下注超时等）
	roomData.gamePlayerData[cfBkNtf.banker_id()].betValue = 1;  // 庄家的下注额实际不会使用到
	roomData.playerInfo[cfBkNtf.banker_id()].set_status(com_protocol::ECattlesPlayerStatus::ECattlesChoiceBet);

	// 游戏状态为确认庄家
	roomData.gameStatus = com_protocol::ECattlesGameStatus::ECattlesConfirmBanker;
	roomData.playerConfirmCount = 0;
	
	// 设置玩家操作超时信息
	setOptTimeOutInfo(hallId, roomId, roomData, cfBkNtf.wait_secs(), (TimerHandler)&CGameProcess::confirmBankerTimeOut, EOptType::EConfirmBanker);

    int rc = m_msgHandler->sendPkgMsgToRoomPlayers(roomData, cfBkNtf, CATTLES_CONFIRM_BANKER_NOTIFY, "confirm banker notify", "");

    OptInfoLog("confirm banker, type = %d, hallId = %u, roomId = %u, banker = %s, wait secs = %u, timerId = %u, rc = %d",
	           roomData.roomInfo.cattle_room().type(), hallId, roomId, cfBkNtf.banker_id().c_str(), cfBkNtf.wait_secs(),
			   roomData.optTimerId, rc);
			   
	return true;
}

// 通知玩家开始下注
void CGameProcess::startChoiceBet(unsigned int hallId, unsigned int roomId, SCattlesRoomData& roomData)
{
	m_msgHandler->getSrvOpt().stopTimer(roomData.optTimerId);

	// 通知玩家开始下注
	com_protocol::CattlesNotifyPlayerChoiceBet ntfChoiceBet;
	ntfChoiceBet.set_wait_secs(m_msgHandler->getSrvOpt().getCattlesBaseCfg().common_cfg.choice_bet_secs);
	
	// 玩家最大输赢值
	const double maxBetValue = m_msgHandler->getMaxBetValue(roomData) * roomData.maxRate;
	for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		if (!it->second.isNewBanker)
		{
			if (roomData.playerInfo[it->first].detail_info().dynamic_info().game_gold() < maxBetValue)
			{
				ntfChoiceBet.set_can_choice_max(com_protocol::ETrueFalseType::EFalseType);
			}
			else
			{
				ntfChoiceBet.set_can_choice_max(com_protocol::ETrueFalseType::ETrueType);
			}
			
			m_msgHandler->getSrvOpt().sendMsgPkgToProxy(ntfChoiceBet, CATTLES_NOTIFY_PLAYER_CHOICE_BET,
			                                            "notify player choice bet", it->first.c_str());
		}
	}
	
	// 非游戏玩家&庄家（庄家默认状态为下注状态）
	ntfChoiceBet.clear_can_choice_max();
	const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(ntfChoiceBet, "player choice bet notify");
	if (msgData != NULL)
	{
		int playerStatus = 0;
		for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
		{
			playerStatus = it->second.status();
			if (playerStatus != com_protocol::ERoomPlayerStatus::EPlayerReady
			    && playerStatus != com_protocol::ECattlesPlayerStatus::ECattlesChoiceBankerOpt)
			{
				m_msgHandler->getSrvOpt().sendMsgToProxyEx(msgData, ntfChoiceBet.ByteSize(), CATTLES_NOTIFY_PLAYER_CHOICE_BET, it->first.c_str());
			}
		}
	}

    // 游戏状态为开始下注，下一步等待玩家下注
	roomData.gameStatus = com_protocol::ECattlesGameStatus::ECattlesGameChoiceBet;
	setOptTimeOutInfo(hallId, roomId, roomData, ntfChoiceBet.wait_secs(), (TimerHandler)&CGameProcess::choiceBetTimeOut, EOptType::EChoiceBet);

    OptInfoLog("start choice bet, hallId = %u, roomId = %u, wait secs = %u, timerId = %u",
	           hallId, roomId, ntfChoiceBet.wait_secs(), roomData.optTimerId);
}

// 发牌给玩家
void CGameProcess::pushCardToPlayer(unsigned int hallId, unsigned int roomId, SCattlesRoomData& roomData)
{
	m_msgHandler->getSrvOpt().stopTimer(roomData.optTimerId);

    // 检查是否存在构成牛牛的三张牌
    unsigned int idx1 = 0;
    unsigned int idx2 = 0;
    unsigned int idx3 = 0;

	com_protocol::CattlesPushCardToPlayerNotify pushCardNtf;
    pushCardNtf.set_wait_secs(m_msgHandler->getSrvOpt().getCattlesBaseCfg().common_cfg.open_card_secs);
	for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
        // 游戏玩家
		pushCardNtf.clear_pokers();
		for (unsigned int idx = 0; idx < CattlesCardCount; ++idx) pushCardNtf.add_pokers(it->second.poker[idx]);

		pushCardNtf.set_result_value(it->second.resultValue);
		pushCardNtf.set_settlement_rate(it->second.settlementRate);
        
        // 检查是否存在构成牛牛的三张牌
        pushCardNtf.clear_cattles_cards();
		if (it->second.resultValue > 0 && it->second.resultValue <= com_protocol::ECattleBaseCardType::ECattleCattle
		    && m_msgHandler->getCattlesCardIndex(it->second.poker, idx1, idx2, idx3))
		{
			pushCardNtf.add_cattles_cards(it->second.poker[idx1]);
			pushCardNtf.add_cattles_cards(it->second.poker[idx2]);
			pushCardNtf.add_cattles_cards(it->second.poker[idx3]);
		}

		m_msgHandler->getSrvOpt().sendMsgPkgToProxy(pushCardNtf, CATTLES_PUSH_CARD_TO_PLAYER_NOTIFY, "push card notify", it->first.c_str());
	}
	
	// 非游戏玩家
	pushCardNtf.Clear();
	pushCardNtf.set_wait_secs(m_msgHandler->getSrvOpt().getCattlesBaseCfg().common_cfg.open_card_secs);
	const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(pushCardNtf, "push card notify other player");
	if (msgData != NULL)
	{	
		for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
		{
			if (it->second.status() != com_protocol::ECattlesPlayerStatus::ECattlesChoiceBet)
			{
				m_msgHandler->getSrvOpt().sendMsgToProxyEx(msgData, pushCardNtf.ByteSize(), CATTLES_PUSH_CARD_TO_PLAYER_NOTIFY, it->first.c_str());
			}
		}
	}
	
	// 下一步，等待玩家亮牌
	roomData.gameStatus = com_protocol::ECattlesGameStatus::ECattlesGamePushCard;
	setOptTimeOutInfo(hallId, roomId, roomData, pushCardNtf.wait_secs(), (TimerHandler)&CGameProcess::openCardTimeOut, EOptType::EOpenCard);

    OptInfoLog("push card to player, hallId = %u, roomId = %u, wait secs = %u, timerId = %u",
	           hallId, roomId, pushCardNtf.wait_secs(), roomData.optTimerId);
}

// 一局结算
bool CGameProcess::doSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData, com_protocol::CattlesSettlementNotify& settlementNtf)
{
	m_msgHandler->getSrvOpt().stopTimer(roomData.optTimerId);

	const char* bankerId = roomData.getCurrentBanker();
	if (bankerId == NULL)
	{
		OptErrorLog("settlement get banker error, hallId = %s, roomId = %s", hallId, roomId);
		return false;
	}
	
	// 结算之前，同步庄家的亮牌给其他玩家
	sendOpenCardNotify(roomData, bankerId, "banker open card notify", bankerId);

    // 庄家信息
	SGamePlayerData& bankerData = roomData.gamePlayerData[bankerId];
	const double bankerGold = roomData.playerInfo[bankerId].detail_info().dynamic_info().game_gold();
	const bool isGoldRoom = roomData.isGoldRoom();
	int bankerWinLoseValue = 0;      // 庄家本局最终输赢金币
	bool isFirstWin = true;          // 双方比较，是否是庄家赢
	int winLostValue = 0;            // 双方比较，输赢值

	for (GamePlayerIdDataMap::iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		if (!it->second.isNewBanker)  // 非庄家
		{
			// 计算输赢值（倍率*押注值）
			isFirstWin = getSettlementCardType(bankerData, it->second);
			if (isFirstWin)
			{
				// 庄家赢了
				winLostValue = bankerData.settlementRate * it->second.betValue;
			    bankerWinLoseValue += winLostValue;
				
				// 检查玩家金币是否足够
				if (isGoldRoom && winLostValue > roomData.playerInfo[it->first].detail_info().dynamic_info().game_gold())
				{
					OptErrorLog("settlement player error, lose value = %d, all value = %.2f",
					-winLostValue, roomData.playerInfo[it->first].detail_info().dynamic_info().game_gold());
					return false;
				}
				
				winLostValue = -winLostValue;  // 玩家输了
			}
			else
			{
				// 玩家赢了
				winLostValue = it->second.settlementRate * it->second.betValue;
				bankerWinLoseValue -= winLostValue;

				// 检查庄家金币是否足够
				if (isGoldRoom && bankerWinLoseValue < 0 && (unsigned int)-bankerWinLoseValue > bankerGold)
				{
					OptErrorLog("settlement banker error, lose value = %d, all value = %.2f", bankerWinLoseValue, bankerGold);
					return false;
				}
				
				// 牌型为牛牛以上的赢家
				if (it->second.resultValue >= com_protocol::ECattleBaseCardType::ECattleCattle)
				{
					it->second.isLastCattlesWinner = true;
					
					const map<int, string>::const_iterator cardTypeIt = m_msgHandler->getSrvOpt().getCattlesBaseCfg().card_type_name.find(it->second.resultValue);
					if (cardTypeIt != m_msgHandler->getSrvOpt().getCattlesBaseCfg().card_type_name.end())
					{
						// 发送赢家公告
						StringVector matchingWords;
						matchingWords.push_back(roomData.playerInfo[it->first].detail_info().static_info().nickname());  // 玩家昵称
						matchingWords.push_back(cardTypeIt->second);  // 牌型中文名称
						
						NumberStr winValueStr = {0};
						snprintf(winValueStr, sizeof(NumberStr) - 1, "%d", winLostValue);
						matchingWords.push_back(winValueStr);         // 赢值

						sendWinnerNotice(hallId, it->first, matchingWords);
					}
				}
			}
			
			it->second.winLoseValue = winLostValue;
			it->second.sumWinLoseValue += winLostValue;
			
			com_protocol::CattlesSettlementInfo* clsSettlementInfo = settlementNtf.add_info();
			clsSettlementInfo->set_username(it->first);
			clsSettlementInfo->set_can_join_arena(com_protocol::ETrueFalseType::ETrueType);
			com_protocol::ItemChange* itemChange = clsSettlementInfo->mutable_item_change();
			itemChange->set_type(EGameGoodsType::EGoodsGold);
			itemChange->set_num(winLostValue);
			itemChange->set_amount(it->second.sumWinLoseValue);
		}
	}
	
	// 庄家最后的输赢值
	bankerData.winLoseValue = bankerWinLoseValue;
	bankerData.sumWinLoseValue += bankerWinLoseValue;

	com_protocol::CattlesSettlementInfo* clsSettlementInfo = settlementNtf.add_info();
	clsSettlementInfo->set_username(bankerId);
	clsSettlementInfo->set_can_join_arena(com_protocol::ETrueFalseType::ETrueType);
	com_protocol::ItemChange* itemChange = clsSettlementInfo->mutable_item_change();
	itemChange->set_type(EGameGoodsType::EGoodsGold);
	itemChange->set_num(bankerWinLoseValue);
	itemChange->set_amount(bankerData.sumWinLoseValue);

	return true;
}

// 完成结算
void CGameProcess::finishSettlement(SCattlesRoomData& roomData)
{	
	// 一局游戏结束，恢复设置玩家正常信息
	GameServiceData gameSrvData;
	gameSrvData.srvId = m_msgHandler->getSrvId();
	gameSrvData.hallId = 0;
	gameSrvData.roomId = 0;
	gameSrvData.status = EGamePlayerStatus::EInRoom;
	gameSrvData.timeSecs = time(NULL);
			
	for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		m_msgHandler->setUserSessionData(it->first.c_str(), it->first.length(), gameSrvData);

	    m_msgHandler->delHearbeatCheck(it->first);  // 删除玩家心跳检测
	}
}

void CGameProcess::roundSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData)
{
	if (roomData.isGoldRoom()) goldSettlement(hallId, roomId, roomData);
	else cardSettlement(hallId, roomId, roomData);
}

// 金币场结算
bool CGameProcess::doGoldSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData)
{
	com_protocol::CattlesSettlementNotify settlementNtf;
	if (!doSettlement(hallId, roomId, roomData, settlementNtf)) return false;
	
	// 多用户物品结算
	int rc = moreUsersGoodsSettlement(hallId, roomId, roomData, *settlementNtf.mutable_info());
	if (rc != SrvOptSuccess)
	{
		OptErrorLog("gold settlement change more usres goods error, rc = %d", rc);
		return false;
	}
	
	// 变更道具数量
    const unsigned int minPlayerGold = m_msgHandler->getMinNeedPlayerGold(roomData); // 闲家需要的最低金币数量
    const unsigned int minBankerGold = m_msgHandler->getMinNeedBankerGold(roomData); // 庄家需要的最低金币数量
	com_protocol::ETrueFalseType hasNewBanker = com_protocol::ETrueFalseType::EFalseType;
	for (google::protobuf::RepeatedPtrField<com_protocol::CattlesSettlementInfo>::iterator clsStlIfIt = settlementNtf.mutable_info()->begin();
	     clsStlIfIt != settlementNtf.mutable_info()->end(); ++clsStlIfIt)
	{
		// 金币数量变更
		com_protocol::DynamicInfo* dynamicInfo = roomData.playerInfo[clsStlIfIt->username()].mutable_detail_info()->mutable_dynamic_info();
		dynamicInfo->set_game_gold(dynamicInfo->game_gold() + clsStlIfIt->item_change().num());
		clsStlIfIt->mutable_item_change()->set_amount(dynamicInfo->game_gold());
		
		// 如果玩家的金币＜最小底注*牌型最大倍率，那么文字提示【您的金币不足】，【准备】按钮是灰色，点击无效。
		if (dynamicInfo->game_gold() < minPlayerGold)
		{
			clsStlIfIt->set_can_join_arena(com_protocol::ETrueFalseType::EFalseType);
		}
		else if (hasNewBanker == com_protocol::ETrueFalseType::EFalseType && dynamicInfo->game_gold() >= minBankerGold)
		{
			// 如果所有的玩家的金币＜最大底注*闲家人数*牌型最大倍率，那么文字提示【没有符合上庄条件的玩家】，【准备】按钮是灰色，点击无效。
			hasNewBanker = com_protocol::ETrueFalseType::ETrueType;  // 金币足够当庄
		}
	}

    // 一局结算之后是否存在可以当庄的玩家
    settlementNtf.set_has_banker(hasNewBanker);
	rc = m_msgHandler->sendPkgMsgToRoomPlayers(roomData, settlementNtf, CATTLES_SETTLEMENT_NOTIFY, "settlement notify", "");

	OptInfoLog("gold settlement notify, play times = %u, has banker = %d, type = %d, rc = %d",
    roomData.playTimes, settlementNtf.has_banker(), roomData.roomInfo.cattle_room().type(), rc);
	
	return true;
}

// 一局金币场结算
void CGameProcess::goldSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData)
{
	// 1）是否可以解散房间，如果结算失败或者玩家全部掉线就解散房间
	if (!doGoldSettlement(hallId, roomId, roomData) || canDisbandGameRoom(roomData))
	{
		m_msgHandler->delRoomData(hallId, roomId);

		return;
	}
	
	// 2）一局游戏结束，恢复设置玩家正常信息
	finishSettlement(roomData);
	
	// 3）接着开始准备下一局游戏
	prepareNextTime(hallId, roomId, roomData);
}

// 房卡场结算
bool CGameProcess::doCardSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData)
{
	com_protocol::CattlesSettlementNotify settlementNtf;
	if (!doSettlement(hallId, roomId, roomData, settlementNtf)) return false;

    // 保存本局结果
	RecordIDType recordId = {0};
	m_msgHandler->getSrvOpt().getRecordId(recordId);
	com_protocol::RoundWinLoseInfo* roundWinLoseInfo = roomData.roomGameRecord.add_round_win_lose();
	roundWinLoseInfo->set_record_id(recordId);

	for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		com_protocol::ChangeRecordPlayerInfo* playerInfo = roundWinLoseInfo->add_player_info();
        playerInfo->set_card_result(it->second.resultValue);
        playerInfo->set_card_rate(it->second.settlementRate);
        playerInfo->set_card_info((const char*)it->second.poker, CattlesCardCount);
        playerInfo->set_player_flag(it->second.isNewBanker ? 1 : 0);
		playerInfo->set_bet_value(it->second.isNewBanker ? 0 : it->second.betValue);
		playerInfo->set_win_lose_value(it->second.winLoseValue);
		playerInfo->set_username(it->first);		
	}

    settlementNtf.set_has_banker(com_protocol::ETrueFalseType::ETrueType);
	int rc = m_msgHandler->sendPkgMsgToRoomPlayers(roomData, settlementNtf, CATTLES_SETTLEMENT_NOTIFY, "settlement notify", "");

	OptInfoLog("card settlement notify, play times = %u, type = %d, rc = %d",
    roomData.playTimes, roomData.roomInfo.cattle_room().type(), rc);
	
	return true;
}

// 房卡结算
void CGameProcess::cardSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData)
{
	// 1）如果结算失败就解散房间
	if (!doCardSettlement(hallId, roomId, roomData))
	{
		m_msgHandler->delRoomData(hallId, roomId);

		return;
	}
	
	if (roomData.playTimes >= roomData.roomInfo.base_info().game_times())
	{
		// 游戏结束，恢复设置玩家正常信息
		finishSettlement(roomData);
		
		// 房卡总结算记录
		cardCompleteSettlement(hallId, roomId, roomData);

		// 结束解散房间
		m_msgHandler->delRoomData(hallId, roomId, false);
		
		return;
	}
	
	// 如果是第一局则扣除房卡付费
	if (roomData.playTimes == 1) payRoomCard(hallId, roomId, roomData);  // 房卡扣费
	
	// 准备下一局游戏
	// 1）重新设置游戏状态为准备
	roomData.gameStatus = com_protocol::ECattlesGameStatus::ECattlesGameReady;
	roomData.playerConfirmCount = 0;
	roomData.optTimeSecs = 0;
	roomData.optTimerId = 0;
	roomData.optLastTimeSecs = 0;
	roomData.nextOptType = 0;
	
	// 2）当前庄家设置为上一局庄家
	const char* bankerId = roomData.getCurrentBanker();
	if (bankerId != NULL)
	{
		SGamePlayerData& bankerData = roomData.gamePlayerData[bankerId];
		bankerData.isLastBanker = true;
		bankerData.isNewBanker = false;
	}

    // 3）玩家复位为坐下状态
	for (GamePlayerIdDataMap::iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		roomData.playerInfo[it->first].set_status(com_protocol::ERoomPlayerStatus::EPlayerSitDown);  // 玩家复位为坐下状态
	}
	
	// 4）重新变更房间的状态为游戏准备中
	roomData.roomStatus = com_protocol::EHallRoomStatus::EHallRoomReady;
	m_msgHandler->getSrvOpt().changeRoomDataNotify(hallId, roomId, CattlesGame, com_protocol::EHallRoomStatus::EHallRoomReady,
												   NULL, NULL, NULL, "room ready again");

    // 5）设置准备超时时间点
	com_protocol::ClientPrepareGameNotify preGmNtf;
	preGmNtf.set_wait_secs(m_msgHandler->getSrvOpt().getCattlesBaseCfg().common_cfg.prepare_game_secs);
	setOptTimeOutInfo(atoi(hallId), atoi(roomId), roomData, preGmNtf.wait_secs(), (TimerHandler)&CGameProcess::prepareTimeOut, EOptType::EPrepareGame);
	
	m_msgHandler->sendPkgMsgToRoomPlayers(roomData, preGmNtf, COMM_PREPARE_GAME_NOTIFY, "prepare next game", "");

	OptInfoLog("card prepare next time, hallId = %s, roomId = %s, game status = %d, player count = %u, user count = %u, address = %p",
    hallId, roomId, roomData.gameStatus, (unsigned int)roomData.gamePlayerData.size(), (unsigned int)roomData.playerInfo.size(), &roomData);
}

// 发送赢家公告
void CGameProcess::sendWinnerNotice(const char* hallId, const string& username, const StringVector& matchingWords)
{
	com_protocol::ClientSndMsgInfoNotify winMsgNtf;
	com_protocol::MessageInfo* msgInfo = winMsgNtf.mutable_msg_info();
	msgInfo->set_msg_type(com_protocol::EMsgType::EWordsMsg);
	msgInfo->set_msg_content(m_msgHandler->getSrvOpt().getCattlesBaseCfg().winner_notice.notice_msg);
	msgInfo->set_msg_owner(com_protocol::EMsgOwner::ESystemMsg);
	msgInfo->set_hall_id(hallId);

    const vector<unsigned int>& objColour = m_msgHandler->getSrvOpt().getCattlesBaseCfg().winner_notice.object_colour;
	const unsigned int objColourSize = objColour.size();
	unsigned int colourIdx = 0;
    for (StringVector::const_iterator it = matchingWords.begin(); it != matchingWords.end(); ++it)
	{
		com_protocol::ColourMsgInfo* matchingInfo = msgInfo->add_matching_info();
		matchingInfo->set_words(*it);
		
		if (colourIdx < objColourSize)
		{
			matchingInfo->set_colour((com_protocol::EMsgColour)objColour[colourIdx]);
			++colourIdx;
		}
	}

	const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(winMsgNtf, "winner notice");
	if (msgData != NULL)
	{
		m_msgHandler->getSrvOpt().forwardMessage(username.c_str(), msgData, winMsgNtf.ByteSize(), ServiceType::GameHallSrv,
		SERVICE_GAME_MSG_NOTICE_NOTIFY, m_msgHandler->getSrvId(), 0, "forward winner notice message");
	}
}

// 准备下一局游戏
void CGameProcess::prepareNextTime(const char* hallId, const char* roomId, SCattlesRoomData& roomData)
{
	// 1）重新设置游戏状态为准备
	roomData.gameStatus = com_protocol::ECattlesGameStatus::ECattlesGameReady;
	roomData.playerConfirmCount = 0;
	roomData.optTimeSecs = 0;
	roomData.optTimerId = 0;
	roomData.optLastTimeSecs = 0;
	roomData.nextOptType = 0;

	// 2）准备下一局游戏之前，先删除掉线了的玩家，可能会导致房间解散（如果房间里的玩家全是掉线玩家）
	// 因此清理掉线用户之前必须先检查房间是否可以解散，如果可以解散则直接解散删除房间
	StringVector offlinePlayers;
	com_protocol::ClientLeaveRoomNotify lvRoomNotify;
	for (GamePlayerIdDataMap::iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end();)
	{
		if (!it->second.isOnline)
		{
			// 同步离线玩家消息
			lvRoomNotify.set_username(it->first);
			m_msgHandler->sendPkgMsgToRoomPlayers(roomData, lvRoomNotify, COMM_PLAYER_LEAVE_ROOM_NOTIFY, "player leave room notify", it->first);
			
			// 需要删除的掉线玩家
			// 1）真正离线的玩家（已经走完离线流程，只剩下当局玩家数据和会话数据没有删除，即掉线后到目前为止没有重连进入房间的玩家）
			// 2）心跳离线的玩家（没有走过离线流程，所有数据都还没有删除）
			// 因此这里必须直接删除离线玩家的游戏数据
			offlinePlayers.push_back(it->first);
			roomData.playerInfo.erase(it->first);
			roomData.gamePlayerData.erase(it++);
		}
		else
		{
			++it;
		}
	}
	
	// 3）没有在线玩家了（全部离线）则解散删除房间
	if (roomData.gamePlayerData.empty())
	{
		m_msgHandler->delRoomData(hallId, roomId);

		return;
	}
	
	// 4）如果当前庄家没离线的话，当前庄家设置为上一局庄家
	const char* bankerId = roomData.getCurrentBanker();
	if (bankerId != NULL)
	{
		SGamePlayerData& bankerData = roomData.gamePlayerData[bankerId];
		bankerData.isLastBanker = true;
		bankerData.isNewBanker = false;
	}

    // 5）剩余的在线玩家复位为坐下状态
	for (GamePlayerIdDataMap::iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		roomData.playerInfo[it->first].set_status(com_protocol::ERoomPlayerStatus::EPlayerSitDown);  // 玩家复位为坐下状态
	}
	
	// 6）是否还存在新庄家
	roomData.currentHasBanker = hasNextNewBanker(roomData);
    if (!roomData.currentHasBanker)
	{
		// 通知客户端没有庄家了（客户端准备按钮设置为灰色）
		m_msgHandler->updatePrepareStatusNotify(roomData, com_protocol::ETrueFalseType::EFalseType, "prepre next time no banker");
	}
	
	// 7）重新变更房间的状态为游戏准备中
	roomData.roomStatus = com_protocol::EHallRoomStatus::EHallRoomReady;
	m_msgHandler->getSrvOpt().changeRoomDataNotify(hallId, roomId, CattlesGame, com_protocol::EHallRoomStatus::EHallRoomReady,
												   NULL, NULL, NULL, "room ready again");

	// 8）最后才清理离线玩家
	if (!offlinePlayers.empty())
	{
		// 如果是已经走完离线流程的玩家那只剩下会话数据待删除
		const unsigned int count = m_msgHandler->delUserFromRoom(offlinePlayers);
		OptWarnLog("game prepare and clear offline player, size = %u, count = %u", offlinePlayers.size(), count);
	}
	
	OptInfoLog("prepare next time, hallId = %s, roomId = %s, game status = %d, player count = %u, user count = %u, address = %p",
    hallId, roomId, roomData.gameStatus, (unsigned int)roomData.gamePlayerData.size(), (unsigned int)roomData.playerInfo.size(), &roomData);
}

// 玩家离开房间
EGamePlayerStatus CGameProcess::playerLeaveRoom(const string& username, SCattlesRoomData& roomData)
{
	RoomPlayerIdInfoMap::iterator plIt = roomData.playerInfo.find(username);
	if (plIt == roomData.playerInfo.end()) return EGamePlayerStatus::ELeaveRoom;
	
	GamePlayerIdDataMap::iterator gmIt = roomData.gamePlayerData.find(username);
	if (gmIt == roomData.gamePlayerData.end())
	{
		roomData.playerInfo.erase(plIt);  // 非游戏玩家（旁观玩家、或者玩家坐下了但没开始游戏）直接删除

		return EGamePlayerStatus::ELeaveRoom;
	}
	
	// 非游戏中状态，任意玩家都可以离开房间
	if (!roomData.isOnPlay())
	{
		roomData.playerInfo.erase(plIt);
		roomData.gamePlayerData.erase(gmIt);

		return EGamePlayerStatus::ELeaveRoom;
	}
	
	// 游戏过程中，玩家掉线了
	gmIt->second.isOnline = false;

	return EGamePlayerStatus::EDisconnectRoom;
}

// 玩家抢庄是否已经完毕
bool CGameProcess::isFinishChoiceBanker(SCattlesRoomData& roomData)
{
    const unsigned int minBankerGold = m_msgHandler->getMinNeedBankerGold(roomData); // 庄家需要的最低金币数量
    for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		if (roomData.playerInfo[it->first].detail_info().dynamic_info().game_gold() >= minBankerGold
            && roomData.playerInfo[it->first].status() != com_protocol::ECattlesPlayerStatus::ECattlesChoiceBankerOpt) return false;
	}
    
    return true;
}

// 对应的状态是否已经完毕
bool CGameProcess::isFinishStatus(SCattlesRoomData& roomData, const int status)
{
	for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		if (roomData.playerInfo[it->first].status() != status) return false;
	}
	
	return true;
}

// 对应状态的个数
unsigned int CGameProcess::getStatusCount(SCattlesRoomData& roomData, const int status)
{
	unsigned int count = 0;
	for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		if (roomData.playerInfo[it->first].status() == status) ++count;
	}
	
	return count;
}

// 设置亮牌通知消息
void CGameProcess::setOpenCardNotifyMsg(SCattlesRoomData& roomData, const string& username,
                                        com_protocol::CattlesPlayerOpenCardNotify& openCardNtf, bool isNeedCard)
{
	openCardNtf.set_username(username);
	
	const SGamePlayerData& playerData = roomData.gamePlayerData[username];
	openCardNtf.set_settlement_rate(playerData.settlementRate);
	openCardNtf.set_result_value(playerData.resultValue);
	
	if (isNeedCard)
	{
		for (unsigned int idx = 0; idx < CattlesCardCount; ++idx) openCardNtf.add_pokers(playerData.poker[idx]);
		
		// 检查是否存在构成牛牛的三张牌
		unsigned int idx1 = 0;
		unsigned int idx2 = 0;
		unsigned int idx3 = 0;
		if (playerData.resultValue > 0 && playerData.resultValue <= com_protocol::ECattleBaseCardType::ECattleCattle
		    && m_msgHandler->getCattlesCardIndex(playerData.poker, idx1, idx2, idx3))
		{
			openCardNtf.add_cattles_cards(playerData.poker[idx1]);
			openCardNtf.add_cattles_cards(playerData.poker[idx2]);
			openCardNtf.add_cattles_cards(playerData.poker[idx3]);
		}
	}
}

// 发送亮牌消息，通知其他玩家
void CGameProcess::sendOpenCardNotify(SCattlesRoomData& roomData, const string& username, const char* logInfo,
                                      const string& exceptName, bool isNeedCard)
{
	// 亮牌同步通知
	com_protocol::CattlesPlayerOpenCardNotify openCardNtf;
	setOpenCardNotifyMsg(roomData, username, openCardNtf, isNeedCard);

	m_msgHandler->sendPkgMsgToRoomPlayers(roomData, openCardNtf, CATTLES_PLAYER_OPEN_CARD_NOTIFY, logInfo, exceptName);
}

// 是否存在下一局新庄家
bool CGameProcess::hasNextNewBanker(SCattlesRoomData& roomData)
{
    bool isOK = false;
	switch (roomData.roomInfo.cattle_room().type())
	{
		case com_protocol::ECattleBankerType::ECattleBanker:
		{
            isOK = cattlesBanker(roomData);
			break;
		}
		
		case com_protocol::ECattleBankerType::EFixedBanker:
		{
            isOK = fixedBanker(roomData);
			break;
		}
		
		case com_protocol::ECattleBankerType::EFreeBanker:
		{
			isOK = freeBanker(roomData);
			break;
		}
		
		case com_protocol::ECattleBankerType::EOpenCardBanker:
		{
			isOK = openCardBanker(roomData);
			break;
		}
		
		default:
		{
			break;
		}
	}

	return isOK;
}

// 玩家是否可以离开房间
bool CGameProcess::canLeaveRoom(GameUserData* cud)
{
	SCattlesRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData == NULL) return true;
	
	// 非游戏中状态，任意玩家都可以离开房间
	if (!roomData->isOnPlay()) return true;
	
	// 非游戏中的玩家可以离开
	return (roomData->gamePlayerData.find(cud->userName) == roomData->gamePlayerData.end());
}

void CGameProcess::onHearbeatResult(const string& username, const bool isOnline)
{
	SCattlesRoomData* roomData = m_msgHandler->getRoomDataByUser(username.c_str());
	if (roomData == NULL || !roomData->isOnPlay()) return;
	
	GamePlayerIdDataMap::iterator gmIt = roomData->gamePlayerData.find(username);
	if (gmIt == roomData->gamePlayerData.end()) return;

    if (gmIt->second.isOnline != isOnline)
	{
		gmIt->second.isOnline = isOnline;

		// 同步玩家状态，发送消息到房间的其他玩家
		const int newStatus = !isOnline ? com_protocol::ERoomPlayerStatus::EPlayerOffline : roomData->playerInfo[username].status();
		int rc = m_msgHandler->updatePlayerStatusNotify(*roomData, username.c_str(), newStatus, username.c_str(), "hear beat result");
		
		OptWarnLog("on hear beat result, username = %s, isOnline = %d, newStatus = %d, rc = %d",
		username.c_str(), isOnline, newStatus, rc);
	}
}

// 房卡总结算
void CGameProcess::cardCompleteSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData, bool isNotify)
{
	if (roomData.roomGameRecord.round_win_lose_size() > 0)
	{
		// 房卡总结算记录
		com_protocol::ChangeRecordBaseInfo* recordInfo = roomData.roomGameRecord.mutable_base_info();
		recordInfo->set_opt_type(com_protocol::EHallPlayerOpt::EOptWinLoseResult);
		recordInfo->set_record_id("");
		recordInfo->set_hall_id(hallId);
		recordInfo->set_game_type(CattlesGame);
		recordInfo->set_room_id(roomId);
		recordInfo->set_room_rate(roomData.roomInfo.base_info().base_rate());
		recordInfo->set_room_flag(roomData.roomInfo.cattle_room().type());
		recordInfo->set_service_id(m_msgHandler->getSrvId());
		recordInfo->set_remark("room game record");

		int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService("", 0, ServiceType::DBProxySrv,
		roomData.roomGameRecord, DBPROXY_SET_ROOM_GAME_RECORD_REQ, "set room game record");

		WriteDataLog("set room game record|%s|%s|%d", hallId, roomId, rc);
	}
	
	if (isNotify)
	{
		// 通知客户端结束游戏
		com_protocol::ClientFinishGameNotify finishGameNtf;
		m_msgHandler->sendPkgMsgToRoomPlayers(roomData, finishGameNtf, COMM_FINISH_GAME_NOTIFY, "finish game notify", "");
	}
}

void CGameProcess::refuseDismissRoomContinueGame(const GameUserData* cud, SCattlesRoomData& roomData, bool isCancelDismissNotify)
{
	if (roomData.dismissRoomTimerId == 0 || roomData.agreeDismissRoomPlayer.empty()
	    || std::find(roomData.agreeDismissRoomPlayer.begin(), roomData.agreeDismissRoomPlayer.end(),
		             cud->userName) != roomData.agreeDismissRoomPlayer.end()) return;

	// 恢复继续游戏
	TimerHandler timerHandler = NULL;
	bool isNotifyTimeOutSecs = true;
	switch (roomData.nextOptType)
	{
		case EOptType::EPrepareGame:
		{
			timerHandler = (TimerHandler)&CGameProcess::prepareTimeOut;
			break;
		}
		
		case EOptType::EStartGame:
		{
			timerHandler = (TimerHandler)&CGameProcess::doStartGame;
			isNotifyTimeOutSecs = false;
			break;
		}
		
		case EOptType::ECompeteBanker:
		{
			timerHandler = (TimerHandler)&CGameProcess::competeBankerTimeOut;
			break;
		}
		
		case EOptType::EConfirmBanker:
		{
			timerHandler = (TimerHandler)&CGameProcess::confirmBankerTimeOut;
			isNotifyTimeOutSecs = false;
			break;
		}
		
		case EOptType::EChoiceBet:
		{
			timerHandler = (TimerHandler)&CGameProcess::choiceBetTimeOut;
			break;
		}
		
		case EOptType::EOpenCard:
		{
			timerHandler = (TimerHandler)&CGameProcess::openCardTimeOut;
			break;
		}
		
		default:
		{
			OptErrorLog("refuse dismiss room continue game error, username = %s, hallId = %s, roomId = %s, opt type = %u",
			cud->userName, cud->hallId, cud->roomId, roomData.nextOptType);

			break;
		}
	}
	
	if (timerHandler == NULL) return;
	
	// 停止解散房间定时器
	m_msgHandler->getSrvOpt().stopTimer(roomData.dismissRoomTimerId);
	
	// 取消房间通知
	if (isCancelDismissNotify)
	{
		com_protocol::ClientCancelDismissRoomNotify cdrNtf;
		m_msgHandler->sendPkgMsgToRoomPlayers(roomData, cdrNtf, COMM_PLAYER_CANCEL_DISMISS_ROOM_NOTIFY,
		"cancel dismiss room notify", cud->userName, com_protocol::ERoomPlayerStatus::EPlayerSitDown);
	}
	
	// 剩余倒计时时间
	if (roomData.optTimerId == 0 && roomData.optTimeSecs > 0 && roomData.optLastTimeSecs > 0)
	{
		const unsigned int pauseTimeSecs = roomData.playerDismissRoomTime[roomData.agreeDismissRoomPlayer[0]];
		const unsigned int passTimeSecs = pauseTimeSecs - roomData.optTimeSecs;
		const unsigned int remainTimeSecs = (roomData.optLastTimeSecs > passTimeSecs) ? (roomData.optLastTimeSecs - passTimeSecs) : 0;
		
		m_msgHandler->getGameProcess().setOptTimeOutInfo(atoi(cud->hallId), atoi(cud->roomId),
		roomData, remainTimeSecs, timerHandler, roomData.nextOptType);
		
		if (isNotifyTimeOutSecs && remainTimeSecs > 0)
		{
			com_protocol::ClientTimeOutSecondsNotify tosNtf;
			tosNtf.set_wait_secs(remainTimeSecs);
			m_msgHandler->sendPkgMsgToRoomPlayers(roomData, tosNtf, COMM_TIME_OUT_SECONDS_NOTIFY,
			"continue game time out seconds notify", "", com_protocol::ERoomPlayerStatus::EPlayerSitDown);
		}
	}
	
	roomData.agreeDismissRoomPlayer.clear();
}

// 获取牛牛模式或者固定模式的下一局庄家
bool CGameProcess::getCattlesFixedNextNewBanker(SCattlesRoomData& roomData, string& banker)
{
    if (roomData.playTimes < 1)
    {
        // 第一局房主为庄家
        RoomPlayerIdInfoMap::iterator bankerIt = roomData.playerInfo.find(roomData.roomInfo.base_info().username());
        if (bankerIt != roomData.playerInfo.end() && bankerIt->second.status() > com_protocol::ERoomPlayerStatus::EPlayerEnter)
        {
            banker = bankerIt->first;

            return true;
        }
    }
        
    StringVector bankers;
    bool isOK = false;
	switch (roomData.roomInfo.cattle_room().type())
	{
		case com_protocol::ECattleBankerType::ECattleBanker:
		{
            isOK = cattlesBanker(roomData, &bankers);
			break;
		}
		
		case com_protocol::ECattleBankerType::EFixedBanker:
		{
            isOK = fixedBanker(roomData, &bankers);
			break;
		}
		
		default:
		{
			break;
		}
	}
	
    if (isOK) banker = bankers[CRandomNumber::getUIntNumber(0, bankers.size() - 1)];

	return isOK;
}

// 设置自由模式或者明牌模式的下一局庄家
bool CGameProcess::setFreeOpenCardNextNewBanker(SCattlesRoomData& roomData)
{
    const unsigned int minBankerGold = m_msgHandler->getMinNeedBankerGold(roomData); // 庄家需要的最低金币数量
    StringVector choiceBankers;  // 玩家抢庄
    StringVector autoBankers;    // 没有玩家抢庄
    for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		if (it->second.choiceBankerResult == com_protocol::ETrueFalseType::ETrueType)
        {
            choiceBankers.push_back(it->first);
        }
        
        else if (choiceBankers.empty() && roomData.playerInfo[it->first].detail_info().dynamic_info().game_gold() >= minBankerGold)
        {
            autoBankers.push_back(it->first);
        }
	}
    
    StringVector* bankers = !choiceBankers.empty() ? &choiceBankers : &autoBankers;
    if (!bankers->empty()) roomData.gamePlayerData[(*bankers)[CRandomNumber::getUIntNumber(0, bankers->size() - 1)]].isNewBanker = true;
    
    return !bankers->empty();
}

// 牛牛上庄
bool CGameProcess::cattlesBanker(SCattlesRoomData& roomData, StringVector* bankers)
{
	const unsigned int minBankerGold = m_msgHandler->getMinNeedBankerGold(roomData); // 庄家需要的最低金币数量
	StringVector newBankers;
	if (!roomData.gamePlayerData.empty())
	{
		if (roomData.getCanToBeNewBankers(minBankerGold, newBankers))  // 存在可以当庄的玩家
		{
			// 满足当庄的条件：1、金币满足；2、牛牛以上牌型（玩家）；3、赢家
			ConstCharPointerVector cattlesWinner;
			if (roomData.getLastCattlesWinners(cattlesWinner))
			{
				// 找到最大牌型的赢家
				bool isFirstWin = true;
				unsigned int maxCardTypeIdx = 0;
				for (unsigned int idx = 1; idx < cattlesWinner.size(); ++idx)
				{
					isFirstWin = getSettlementCardType(roomData.gamePlayerData[cattlesWinner[maxCardTypeIdx]],
													   roomData.gamePlayerData[cattlesWinner[idx]]);
					if (!isFirstWin) maxCardTypeIdx = idx;
				}
				
				// 金币是否足够
				if (std::find(newBankers.begin(), newBankers.end(), cattlesWinner[maxCardTypeIdx]) != newBankers.end())
				{
					if (bankers != NULL) bankers->push_back(cattlesWinner[maxCardTypeIdx]);
					
					return true;
				}
			}

			// 没有赢家或者赢家金币不足够当庄，那如果上一局的庄家金币足够则庄家连庄
			const char* lastBanker = roomData.getLastBanker();
			if (lastBanker != NULL && std::find(newBankers.begin(), newBankers.end(), lastBanker) != newBankers.end())
			{
				if (bankers != NULL) bankers->push_back(lastBanker); // 庄家连庄

				return true;
			}
			
			// 庄家金币不足，则随机一个赢家
			if (bankers != NULL) bankers->push_back(newBankers[CRandomNumber::getUIntNumber(0, newBankers.size() - 1)]); 

			return true;
		}
	}
	
	// 上一局玩家没有符合当庄的玩家，再从座位玩家查找
	return freeBanker(roomData, bankers);
}

// 固定庄家
bool CGameProcess::fixedBanker(SCattlesRoomData& roomData, StringVector* bankers)
{
	const unsigned int minBankerGold = m_msgHandler->getMinNeedBankerGold(roomData); // 庄家需要的最低金币数量
	const char* lastBanker = roomData.getLastBanker();
	if (lastBanker != NULL && roomData.playerInfo[lastBanker].detail_info().dynamic_info().game_gold() >= minBankerGold)
	{
		if (bankers != NULL) bankers->push_back(lastBanker); // 庄家连庄

		return true;
	}
	
	return freeBanker(roomData, bankers);
}

// 自由抢庄
bool CGameProcess::freeBanker(SCattlesRoomData& roomData, StringVector* bankers)
{
	const unsigned int minBankerGold = m_msgHandler->getMinNeedBankerGold(roomData); // 庄家需要的最低金币数量
	int playerStatus = com_protocol::ERoomPlayerStatus::EPlayerEnter;
	for (RoomPlayerIdInfoMap::const_iterator plIt = roomData.playerInfo.begin(); plIt != roomData.playerInfo.end(); ++plIt)
	{
		// 状态&金币满足即可
		playerStatus = plIt->second.status();
		if ((playerStatus == com_protocol::ERoomPlayerStatus::EPlayerSitDown
		     || playerStatus == com_protocol::ERoomPlayerStatus::EPlayerReady
			 || playerStatus == com_protocol::ECattlesPlayerStatus::ECattlesChoiceBankerOpt)
			&& (plIt->second.detail_info().dynamic_info().game_gold() >= minBankerGold))
		{
			if (bankers == NULL) return true;
			
			bankers->push_back(plIt->first);
		}
	}
	
	return (bankers != NULL && !bankers->empty());
}

// 明牌抢庄
bool CGameProcess::openCardBanker(SCattlesRoomData& roomData, StringVector* bankers)
{
	return freeBanker(roomData, bankers);
}

// 结算牌型
unsigned int CGameProcess::getCattlesCardType(const CattlesHandCard poker,
                                              const ::google::protobuf::RepeatedField<int>& selectSpecialType,
											  ::google::protobuf::RepeatedField<::google::protobuf::uint32>* cattlesCards)
{
	// 先查看是否存在特殊牌型
	unsigned int cattlesValue = getSpecialCardType(poker, selectSpecialType);
	if (cattlesValue > com_protocol::ECattleBaseCardType::ECattleCattle) return cattlesValue;

	// 检查是否存在构成牛牛的三张牌
	unsigned int idx1 = 0;
	unsigned int idx2 = 0;
	unsigned int idx3 = 0;
	if (!m_msgHandler->getCattlesCardIndex(poker, idx1, idx2, idx3)) return com_protocol::ECattleBaseCardType::ENoCattle;  // 无牛
	
	// 剩余其他2张牌的值
	cattlesValue = 0;
	unsigned int cardCount = 0;
	for (unsigned int cattlesIdx = 0; cattlesIdx < CattlesCardCount; ++cattlesIdx)
	{
		if (cattlesIdx != idx1 && cattlesIdx != idx2 && cattlesIdx != idx3)
		{
			cattlesValue += CardValue[poker[cattlesIdx] % PokerCardBaseValue];
			if (++cardCount > 1) break;
		}
	}
	
	if (cattlesCards != NULL)
	{
		cattlesCards->Add(poker[idx1]);
		cattlesCards->Add(poker[idx2]);
		cattlesCards->Add(poker[idx3]);
	}
	
	// 计算牌型为牛几
	cattlesValue %= BaseCattlesValue;
	return (cattlesValue != 0) ? cattlesValue : com_protocol::ECattleBaseCardType::ECattleCattle;
}

// 特殊牌型
unsigned int CGameProcess::getSpecialCardType(const CattlesHandCard poker, const ::google::protobuf::RepeatedField<int>& selectSpecialType)
{
	typedef bool (CGameProcess::*GetCattlesSpecialTypeFunc)(const CattlesHandCard poker);
    static const GetCattlesSpecialTypeFunc getSpecialType[] =
	{
		&CGameProcess::isFiveSmallCattle,
		&CGameProcess::isBombCattle,
		&CGameProcess::isGourdCattle,
		&CGameProcess::isSameColourCattle,
		&CGameProcess::isFiveColourCattle,
		&CGameProcess::isSequenceCattle,
	};

	if (selectSpecialType.size() > 0)
	{
		vector<unsigned int> sortSpecialType;
		for (::google::protobuf::RepeatedField<int>::const_iterator it = selectSpecialType.begin(); it != selectSpecialType.end(); ++it)
		{
			if (*it >= com_protocol::ECattleSpecialCardType::ESequenceCattle
			    && *it <= com_protocol::ECattleSpecialCardType::EFiveSmallCattle)
			{	
				// std::sort 默认为升序排序，最大的牌型需要最先得到判断执行，因此调整最大的牌型排在最前面
				sortSpecialType.push_back(com_protocol::ECattleSpecialCardType::EFiveSmallCattle - *it);
			}
		}
		
		std::sort(sortSpecialType.begin(), sortSpecialType.end());
		for (vector<unsigned int>::const_iterator sptIt = sortSpecialType.begin(); sptIt != sortSpecialType.end(); ++sptIt)
		{
			if ((this->*(getSpecialType[*sptIt]))(poker)) return (com_protocol::ECattleSpecialCardType::EFiveSmallCattle - *sptIt);
		}
	}
	
	return 0;  // 无特殊牌型
}
	
// 顺子牛
bool CGameProcess::isSequenceCattle(const CattlesHandCard poker)
{
	// 降序排序
	CattlesHandCard sortPoker = {0};
	memcpy(sortPoker, poker, sizeof(CattlesHandCard));
	
    descendSort(sortPoker);
	for (unsigned int idx = 0; idx < CattlesCardCount - 1; ++idx)
	{
		if ((sortPoker[idx] % PokerCardBaseValue) != ((sortPoker[idx + 1] % PokerCardBaseValue) + 1)) return false;
	}
	
	return true;
}   

// 五花牛
bool CGameProcess::isFiveColourCattle(const CattlesHandCard poker)
{
	const unsigned int BaseCattlesColour = 11;
	for (unsigned int idx = 0; idx < CattlesCardCount; ++idx)
	{
		if ((poker[idx] % PokerCardBaseValue) < BaseCattlesColour) return false;
	}
	
	return true;
}

// 同花牛
bool CGameProcess::isSameColourCattle(const CattlesHandCard poker)
{
	const unsigned int Colour = poker[0] / PokerCardBaseValue;
	for (unsigned int idx = 1; idx < CattlesCardCount; ++idx)
	{
		if ((poker[idx] / PokerCardBaseValue) != Colour) return false;
	}
	
	return true;
}

// 葫芦牛
bool CGameProcess::isGourdCattle(const CattlesHandCard poker)
{
	// 降序排序
	CattlesHandCard sortPoker = {0};
	memcpy(sortPoker, poker, sizeof(CattlesHandCard));
	
    descendSort(sortPoker);
	
	// 检查是否是葫芦，牌型例子：33222 或者 33322
	return (sortPoker[0] % PokerCardBaseValue == sortPoker[1] % PokerCardBaseValue
	        && sortPoker[3] % PokerCardBaseValue == sortPoker[4] % PokerCardBaseValue
	        && (sortPoker[1] % PokerCardBaseValue == sortPoker[2] % PokerCardBaseValue
			    || sortPoker[2] % PokerCardBaseValue == sortPoker[3] % PokerCardBaseValue));
}

// 炸弹牛
bool CGameProcess::isBombCattle(const CattlesHandCard poker)
{
	// 降序排序
	CattlesHandCard sortPoker = {0};
	memcpy(sortPoker, poker, sizeof(CattlesHandCard));
	
    descendSort(sortPoker);

	// 检查是否是四条，牌型例子：33332 或者 23333
	return (sortPoker[1] % PokerCardBaseValue == sortPoker[2] % PokerCardBaseValue
	        && sortPoker[1] % PokerCardBaseValue == sortPoker[3] % PokerCardBaseValue
	        && (sortPoker[1] % PokerCardBaseValue == sortPoker[0] % PokerCardBaseValue
		        || sortPoker[1] % PokerCardBaseValue == sortPoker[4] % PokerCardBaseValue));
}  

// 五小牛
bool CGameProcess::isFiveSmallCattle(const CattlesHandCard poker)
{
	unsigned int smlValue = 0;
	for (unsigned int idx = 0; idx < CattlesCardCount; ++idx)
	{
		smlValue += (poker[idx] % PokerCardBaseValue);
	}
	
	const unsigned int SmallCattlesValue = 10;
	return smlValue <= SmallCattlesValue;
}

// 降序排序
void CGameProcess::descendSort(CattlesHandCard poker)
{
	// 只有5张牌，简单交换排序
	unsigned char tmp = '\0';
	unsigned int value1 = 0;
	unsigned int value2 = 0;
	for (unsigned int idx1 = 0; idx1 < CattlesCardCount - 1; ++idx1)
	{
		for (unsigned int idx2 = (idx1 + 1); idx2 < CattlesCardCount; ++idx2)
		{
			value1 = poker[idx1] % PokerCardBaseValue;
			value2 = poker[idx2] % PokerCardBaseValue;
			
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

// 返回最大值的牌
unsigned char CGameProcess::getMaxValueCard(const CattlesHandCard poker)
{
	unsigned char maxValueCard = poker[0];
	unsigned int value1 = 0;
	unsigned int value2 = 0;
	for (unsigned int idx = 1; idx < CattlesCardCount; ++idx)
	{
		value1 = maxValueCard % PokerCardBaseValue;
		value2 = poker[idx] % PokerCardBaseValue;
		
		// 先比较牌值，牌值相同则比较花色
		if ((value2 > value1)
			|| (value2 == value1 && (poker[idx] / PokerCardBaseValue) > (maxValueCard / PokerCardBaseValue)))
		{
			maxValueCard = poker[idx];
		}
	}
	
	return maxValueCard;
}

// 比较双方的牌，返回是否是 pd1 赢
bool CGameProcess::getSettlementCardType(const SGamePlayerData& pd1, const SGamePlayerData& pd2)
{
	// 直接牌型比较
	if (pd1.resultValue > pd2.resultValue) return true;
	if (pd1.resultValue < pd2.resultValue) return false;
	
	// 牌型相同则比较最大的一张牌
	const unsigned char pd1MaxCard = getMaxValueCard(pd1.poker);
	const unsigned char pd2MaxCard = getMaxValueCard(pd2.poker);
	const unsigned int value1 = pd1MaxCard % PokerCardBaseValue;
	const unsigned int value2 = pd2MaxCard % PokerCardBaseValue;
	
	// 牌值大小比较
	if (value1 > value2) return true;
	if (value1 < value2) return false;
	
	// 牌花色比较
	return ((pd1MaxCard / PokerCardBaseValue) > (pd2MaxCard / PokerCardBaseValue));
}

// 多用户物品结算
int CGameProcess::moreUsersGoodsSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData,
										   google::protobuf::RepeatedPtrField<com_protocol::CattlesSettlementInfo>& clsStInfo)
{
	// 结算变更道具数量
	RecordIDType recordId = {0};
	m_msgHandler->getSrvOpt().getRecordId(recordId);

    // 需要写数据日志信息，作为第一依据及统计使用
	com_protocol::MoreUsersItemChangeReq itemChangeReq;
	itemChangeReq.set_hall_id(hallId);

	com_protocol::ChangeRecordBaseInfo* recordInfo = itemChangeReq.mutable_base_info();
	recordInfo->set_opt_type(com_protocol::EHallPlayerOpt::EOptProfitLoss);
	recordInfo->set_record_id(recordId);
	recordInfo->set_hall_id(hallId);
	recordInfo->set_game_type(CattlesGame);
	recordInfo->set_room_id(roomId);
	recordInfo->set_room_rate(roomData.roomInfo.base_info().base_rate());
    recordInfo->set_room_flag(roomData.roomInfo.cattle_room().type());
	recordInfo->set_service_id(m_msgHandler->getSrvId());
	recordInfo->set_remark("cattles settlement");

	const float serviceTaxRatio = roomData.roomInfo.base_info().service_tax_ratio();
	const bool needTaxDeduction = (serviceTaxRatio > 0.000);
	com_protocol::ItemChangeReq* itemChange = NULL;
	float taxValue = 0.00;
	for (google::protobuf::RepeatedPtrField<com_protocol::CattlesSettlementInfo>::iterator clsStlIfIt = clsStInfo.begin();
	     clsStlIfIt != clsStInfo.end(); ++clsStlIfIt)
	{
	    itemChange = itemChangeReq.add_item_change_req();
        itemChange->set_write_data_log(1);

        const SGamePlayerData& playerData = roomData.gamePlayerData[clsStlIfIt->username()];
        com_protocol::ChangeRecordPlayerInfo* playerInfo = itemChange->mutable_player_info();
        playerInfo->set_card_result(playerData.resultValue);
        playerInfo->set_card_rate(playerData.settlementRate);
        playerInfo->set_card_info((const char*)playerData.poker, CattlesCardCount);
        playerInfo->set_player_flag(playerData.isNewBanker ? 1 : 0);
		playerInfo->set_bet_value(playerData.betValue);

		itemChange->set_hall_id(hallId);
		itemChange->set_src_username(clsStlIfIt->username());
        
        WriteDataLog("user goods settlement|%s|%u|%u|%d.%d.%d.%d.%d|%d|%u|%.2f", clsStlIfIt->username().c_str(), playerData.resultValue, playerData.settlementRate,
        playerData.poker[0], playerData.poker[1], playerData.poker[2], playerData.poker[3], playerData.poker[4],
		playerInfo->player_flag(), clsStlIfIt->item_change().type(), clsStlIfIt->item_change().num());
		
		// 是否需要扣税扣服务费用
		if (needTaxDeduction && clsStlIfIt->item_change().num() > 0.00)
		{
			taxValue = clsStlIfIt->item_change().num() * serviceTaxRatio;
			clsStlIfIt->mutable_item_change()->set_num(clsStlIfIt->item_change().num() - taxValue);
			
			// 扣税扣服务费用记录
			com_protocol::ItemChangeReq* taxDeductionItem = itemChangeReq.add_item_change_req();
			taxDeductionItem->set_write_data_log(1);
			taxDeductionItem->set_hall_id(hallId);
			taxDeductionItem->set_src_username(clsStlIfIt->username());
			
			com_protocol::ItemChange* taxItemInfo = taxDeductionItem->add_items();
			taxItemInfo->set_type(clsStlIfIt->item_change().type());
			taxItemInfo->set_num(taxValue);
			
			com_protocol::ChangeRecordBaseInfo* taxRecordInfo = taxDeductionItem->mutable_base_info();
			*taxRecordInfo = *recordInfo;
			taxRecordInfo->set_opt_type(com_protocol::EHallPlayerOpt::EOptTaxCost);
			taxRecordInfo->set_remark("cattles settlement tax deduction");

            WriteDataLog("settlement tax deduction|%s|%u|%.3f",
			clsStlIfIt->username().c_str(), taxItemInfo->type(), taxItemInfo->num());			
		}
		
		*itemChange->add_items() = clsStlIfIt->item_change();
	}
	
	int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService("", 0, ServiceType::DBProxySrv, itemChangeReq, DBPROXY_CHANGE_MORE_USERS_ITEM_REQ, "more user goods settlement");
    
    WriteDataLog("more user goods settlement|%s|%s|%s|%u|%d", hallId, roomId, recordId, itemChangeReq.item_change_req_size(), rc);

    return rc;
}

// 房卡扣费
int CGameProcess::payRoomCard(const char* hallId, const char* roomId, SCattlesRoomData& roomData)
{
	RecordIDType recordId = {0};
	m_msgHandler->getSrvOpt().getRecordId(recordId);

    // 需要写数据日志信息，作为第一依据及统计使用
	com_protocol::MoreUsersItemChangeReq itemChangeReq;
	itemChangeReq.set_hall_id(hallId);

	com_protocol::ChangeRecordBaseInfo* recordInfo = itemChangeReq.mutable_base_info();
	recordInfo->set_opt_type(com_protocol::EHallPlayerOpt::EOptPayRoomCard);
	recordInfo->set_record_id(recordId);
	recordInfo->set_hall_id(hallId);
	recordInfo->set_game_type(CattlesGame);
	recordInfo->set_room_id(roomId);
	recordInfo->set_room_rate(roomData.roomInfo.base_info().base_rate());
    recordInfo->set_room_flag(roomData.roomInfo.cattle_room().type());
	recordInfo->set_service_id(m_msgHandler->getSrvId());
	recordInfo->set_remark("cattles pay room card");

	const NCattlesBaseConfig::RoomCardCfg& roomCardCfg = m_msgHandler->getSrvOpt().getCattlesBaseCfg().room_card_cfg;
	const unsigned int payMode = roomData.roomInfo.base_info().pay_mode();
	const unsigned int gameTimes = roomData.roomInfo.base_info().game_times();

    if (payMode == com_protocol::EHallRoomPayMode::ERoomAveragePay)
	{
		const float payCount = roomCardCfg.average_pay_count * gameTimes;
		for (GamePlayerIdDataMap::iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
		{
			com_protocol::ItemChangeReq* itemChange = itemChangeReq.add_item_change_req();
            itemChange->set_write_data_log(1);
			
			itemChange->set_hall_id(hallId);
		    itemChange->set_src_username(it->first);
			
			com_protocol::ItemChange* item = itemChange->add_items();
			item->set_type(EGameGoodsType::EGoodsRoomCard);
			item->set_num(payCount);
		}
	}
	else
	{
		com_protocol::ItemChangeReq* itemChange = itemChangeReq.add_item_change_req();
		itemChange->set_write_data_log(1);
		
		itemChange->set_hall_id(hallId);
		itemChange->set_src_username(roomData.roomInfo.base_info().username());
		
		com_protocol::ItemChange* item = itemChange->add_items();
		item->set_type(EGameGoodsType::EGoodsRoomCard);
		item->set_num(roomCardCfg.creator_pay_count * gameTimes);
	}

	int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService("", 0, ServiceType::DBProxySrv, itemChangeReq, DBPROXY_CHANGE_MORE_USERS_ITEM_REQ, "pay room card", 1);
    
    WriteDataLog("pay room card|%s|%s|%s|%u|%d", hallId, roomId, recordId, itemChangeReq.item_change_req_size(), rc);

    return rc;
}

void CGameProcess::prepareTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 也可使用timerId遍历找到房间数据，但效率较低
	char strHallId[IdMaxLen] = {0};
	snprintf(strHallId, sizeof(strHallId) - 1, "%u", userId);
	
	char strRoomId[IdMaxLen] = {0};
	snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", (unsigned int)(unsigned long)param);
	
	SCattlesRoomData* roomData = m_msgHandler->getRoomDataById(strRoomId, strHallId);
	OptInfoLog("prepare time out, hallId = %u, roomId = %u, roomData = %p, timerId = %u",
	           userId, (unsigned int)(unsigned long)param, roomData, timerId);
	
	if (roomData == NULL) return;

	for (RoomPlayerIdInfoMap::iterator it = roomData->playerInfo.begin(); it != roomData->playerInfo.end(); ++it)
	{
		if (it->second.status() == com_protocol::ERoomPlayerStatus::EPlayerSitDown && it->second.seat_id() >= 0)
		{
			it->second.set_status(com_protocol::ERoomPlayerStatus::EPlayerReady);  // 玩家状态为准备
		}
	}

    roomData->optTimerId = 0;
	startGame(strHallId, strRoomId, *roomData); // 直接开始游戏
}

void CGameProcess::competeBankerTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 也可使用timerId遍历找到房间数据，但效率较低
	SCattlesRoomData* roomData = m_msgHandler->getRoomData(userId, (unsigned int)(unsigned long)param);
	
	OptInfoLog("compete banker time out, hallId = %u, roomId = %u, roomData = %p, timerId = %u",
	           userId, (unsigned int)(unsigned long)param, roomData, timerId);
	
	if (roomData != NULL)
	{
		roomData->optTimerId = 0;

		// 确定庄家
        setFreeOpenCardNextNewBanker(*roomData);
        confirmBanker(userId, (unsigned int)(unsigned long)param, *roomData);
	}
}

void CGameProcess::confirmBankerTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 也可使用timerId遍历找到房间数据，但效率较低
	SCattlesRoomData* roomData = m_msgHandler->getRoomData(userId, (unsigned int)(unsigned long)param);
	
	OptInfoLog("confirm banker time out, hallId = %u, roomId = %u, roomData = %p, timerId = %u",
	           userId, (unsigned int)(unsigned long)param, roomData, timerId);
	
	if (roomData != NULL)
	{
		roomData->optTimerId = 0;

		// 通知玩家开始下注
        startChoiceBet(userId, (unsigned int)(unsigned long)param, *roomData);
	}
}

void CGameProcess::choiceBetTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 也可使用timerId遍历找到房间数据，但效率较低
	SCattlesRoomData* roomData = m_msgHandler->getRoomData(userId, (unsigned int)(unsigned long)param);
	
	OptInfoLog("player choice bet time out, hallId = %u, roomId = %u, roomData = %p, timerId = %u",
	           userId, (unsigned int)(unsigned long)param, roomData, timerId);
	
	if (roomData != NULL)
	{
		roomData->optTimerId = 0;

		// 所有未选择下注的玩家默认押注最低值
		com_protocol::CattlesPlayerChoiceBetNotify choiceBetNtf;
		const unsigned int minBetValue = m_msgHandler->getMinBetValue(*roomData);
		for (GamePlayerIdDataMap::iterator it = roomData->gamePlayerData.begin(); it != roomData->gamePlayerData.end(); ++it)
		{
			if (it->second.betValue == 0)
			{
				// 玩家状态为下注
				roomData->playerInfo[it->first].set_status(com_protocol::ECattlesPlayerStatus::ECattlesChoiceBet);
				it->second.betValue = minBetValue;
				
				// 同步下注消息给其他玩家
				choiceBetNtf.set_username(it->first);
				choiceBetNtf.set_bet_value(minBetValue);
				m_msgHandler->sendPkgMsgToRoomPlayers(*roomData, choiceBetNtf, CATTLES_PLAYER_CHOICE_BET_TIME_OUT_NOTIFY, "choice bet time out notify", "");
			}
		}

        // 下一步，发牌给玩家
	    pushCardToPlayer(userId, (unsigned int)(unsigned long)param, *roomData);
	}
}

void CGameProcess::openCardTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 也可使用timerId遍历找到房间数据，但效率较低
    SCattlesRoomData* roomData = m_msgHandler->getRoomData(userId, (unsigned int)(unsigned long)param);
	
	OptInfoLog("open card time out, hallId = %u, roomId = %u, roomData = %p, timerId = %u",
	           userId, (unsigned int)(unsigned long)param, roomData, timerId);
	
	if (roomData != NULL)
	{
		roomData->optTimerId = 0;

		// 所有未亮牌的玩家亮牌
		for (GamePlayerIdDataMap::iterator it = roomData->gamePlayerData.begin(); it != roomData->gamePlayerData.end(); ++it)
		{
			com_protocol::ClientRoomPlayerInfo& playerInfo = roomData->playerInfo[it->first];
			if (playerInfo.status() != com_protocol::ECattlesPlayerStatus::ECattlesOpenCard)  // 还没有亮牌的玩家
			{
				playerInfo.set_status(com_protocol::ECattlesPlayerStatus::ECattlesOpenCard);  // 玩家状态为亮牌
				
				if (it->second.isOnline)
				{
					// 在线玩家超时没亮牌则重置为无牛
					it->second.resultValue = 0;
					it->second.settlementRate = 1;
				}
				
				if (!it->second.isNewBanker)
				{
					sendOpenCardNotify(*roomData, it->first, "player time out open card notify", "");  // 玩家亮牌超时
				}
				else
				{
					// 庄家亮牌超时，庄家的亮牌发送给庄家
					com_protocol::CattlesPlayerOpenCardNotify openCardNtf;
					setOpenCardNotifyMsg(*roomData, it->first, openCardNtf);
					m_msgHandler->getSrvOpt().sendMsgPkgToProxy(openCardNtf, CATTLES_PLAYER_OPEN_CARD_NOTIFY,
																"banker time out open card notify", it->first.c_str());
				}
			}
		}
		
		char strHallId[IdMaxLen] = {0};
		snprintf(strHallId, sizeof(strHallId) - 1, "%u", userId);
		
		char strRoomId[IdMaxLen] = {0};
		snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", (unsigned int)(unsigned long)param);
		roundSettlement(strHallId, strRoomId, *roomData);  // 结算
	}
}

void CGameProcess::doStartGame(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	SCattlesRoomData* roomData = m_msgHandler->getRoomData(userId, (unsigned int)(unsigned long)param);
	if (roomData != NULL)
	{
		roomData->optTimerId = 0;

		if (roomData->gameStatus == com_protocol::ECattlesGameStatus::ECattlesCompeteBanker)
		{
			competeBanker(userId, (unsigned int)(unsigned long)param, *roomData);  // 玩家抢庄（自由抢庄、明牌抢庄类型）
		}
		else
		{
			confirmBanker(userId, (unsigned int)(unsigned long)param, *roomData);  // 直接确定庄家（牛牛上庄、固定庄家类型）
		}
	}
}

void CGameProcess::playerChoiceBanker(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CattlesPlayerChoiceBankerReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "choice banker request")) return;

    GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	SCattlesRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData != NULL && roomData->gameStatus == com_protocol::ECattlesGameStatus::ECattlesCompeteBanker)
	{
		GamePlayerIdDataMap::iterator playerDataIt = roomData->gamePlayerData.find(cud->userName);
        if (playerDataIt != roomData->gamePlayerData.end()
            && roomData->playerInfo[playerDataIt->first].status() != com_protocol::ECattlesPlayerStatus::ECattlesChoiceBankerOpt)
		{
			com_protocol::CattlesPlayerChoiceBankerRsp rsp;
			do
			{
				// 金币判断
                const unsigned int minBankerGold = m_msgHandler->getMinNeedBankerGold(*roomData); // 庄家需要的最低金币数量
				if (roomData->playerInfo[playerDataIt->first].detail_info().dynamic_info().game_gold() < minBankerGold)
				{
					rsp.set_result(CattlesChoiceBankerGoldError);
					break;
				}

                // 设置抢庄结果及状态
				playerDataIt->second.choiceBankerResult = req.choice_result();
				roomData->playerInfo[playerDataIt->first].set_status(com_protocol::ECattlesPlayerStatus::ECattlesChoiceBankerOpt);
				
				rsp.set_result(SrvOptSuccess);
				rsp.set_choice_result(req.choice_result());
				
				// 同步抢庄消息给其他玩家
                com_protocol::CattlesPlayerChoiceBankerNotify choiceBankerNtf;
				choiceBankerNtf.set_username(playerDataIt->first);
				choiceBankerNtf.set_choice_result(req.choice_result());
				m_msgHandler->sendPkgMsgToRoomPlayers(*roomData, choiceBankerNtf, CATTLES_PLAYER_CHOICE_BANKER_NOTIFY, "choice banker notify", playerDataIt->first);

			} while (false);
			
			m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, CATTLES_PLAYER_CHOICE_BANKER_RSP, "choice banker reply");
			
			OptInfoLog("player choice banker reply, usrename = %s, count = %u",
			cud->userName, getStatusCount(*roomData, com_protocol::ECattlesPlayerStatus::ECattlesChoiceBankerOpt));

			if (rsp.result() == SrvOptSuccess && isFinishChoiceBanker(*roomData))
			{
                setFreeOpenCardNextNewBanker(*roomData);
				confirmBanker(atoi(cud->hallId), atoi(cud->roomId), *roomData);  // 抢庄完毕了就确认庄家
			}
		}
	}
	
	OptInfoLog("player choice banker, usrename = %s", cud->userName);
}

void CGameProcess::performFinish(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{	
	GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	SCattlesRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData != NULL && roomData->gameStatus == com_protocol::ECattlesGameStatus::ECattlesConfirmBanker
	    && roomData->gamePlayerData.find(cud->userName) != roomData->gamePlayerData.end()
		&& ++roomData->playerConfirmCount >= roomData->gamePlayerData.size())
	{
        startChoiceBet(atoi(cud->hallId), atoi(cud->roomId), *roomData);  // 通知玩家开始下注
	}
}

void CGameProcess::playerChoiceBet(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CattlesPlayerChoiceBetReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "choice bet request")) return;

    GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	SCattlesRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData != NULL && roomData->gameStatus == com_protocol::ECattlesGameStatus::ECattlesGameChoiceBet)
	{
		GamePlayerIdDataMap::iterator playerDataIt = roomData->gamePlayerData.find(cud->userName);
        if (playerDataIt != roomData->gamePlayerData.end()
            && roomData->playerInfo[playerDataIt->first].status() != com_protocol::ECattlesPlayerStatus::ECattlesChoiceBet)
		{
			com_protocol::CattlesPlayerChoiceBetRsp rsp;
			do
			{
				bool isValidBetValue = false;
				const unsigned int baseBetValue = roomData->roomInfo.cattle_room().number_index();
				const vector<unsigned int>& betRate = roomData->isGoldRoom() ?
				m_msgHandler->getSrvOpt().getCattlesBaseCfg().gold_base_number_cfg.bet_rate : m_msgHandler->getSrvOpt().getCattlesBaseCfg().card_base_number_cfg.bet_rate;
				for (vector<unsigned int>::const_iterator betRateIt = betRate.begin(); betRateIt != betRate.end(); ++betRateIt)
				{
					if (req.bet_value() == baseBetValue * *betRateIt)
					{
						isValidBetValue = true;
						break;
					}
				}
				
				if (!isValidBetValue)
				{
					rsp.set_result(CattlesInvalidChoiceBetValue);
					break;
				}
				
				// 下注额判断
				if (roomData->isGoldRoom() && roomData->playerInfo[playerDataIt->first].detail_info().dynamic_info().game_gold() < (req.bet_value() * roomData->maxRate))
				{
					rsp.set_result(CattlesGameGoldInsufficient);
					break;
				}

                // 设置下注额及状态
				playerDataIt->second.betValue = req.bet_value();
				roomData->playerInfo[playerDataIt->first].set_status(com_protocol::ECattlesPlayerStatus::ECattlesChoiceBet);
				
				rsp.set_result(SrvOptSuccess);
				rsp.set_bet_value(req.bet_value());
				
				// 同步下注消息给其他玩家
                com_protocol::CattlesPlayerChoiceBetNotify choiceBetNtf;
				choiceBetNtf.set_username(playerDataIt->first);
				choiceBetNtf.set_bet_value(req.bet_value());
				m_msgHandler->sendPkgMsgToRoomPlayers(*roomData, choiceBetNtf, CATTLES_PLAYER_CHOICE_BET_NOTIFY, "choice bet notify", playerDataIt->first);

			} while (false);
			
			m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, CATTLES_PLAYER_CHOICE_BET_RSP, "choice bet reply");
			
			OptInfoLog("player choice bet reply, usrename = %s, count = %u",
			cud->userName, getStatusCount(*roomData, com_protocol::ECattlesPlayerStatus::ECattlesChoiceBet));

			if (rsp.result() == SrvOptSuccess && isFinishStatus(*roomData, com_protocol::ECattlesPlayerStatus::ECattlesChoiceBet))
			{
				pushCardToPlayer(atoi(cud->hallId), atoi(cud->roomId), *roomData);  // 下注完毕了就发牌给玩家
			}
		}
	}
	
	OptInfoLog("player choice bet, usrename = %s", cud->userName);
}

void CGameProcess::playerOpenCard(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CattlesPlayerOpenCardReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "open card request")) return;

	GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	SCattlesRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData != NULL && roomData->gameStatus == com_protocol::ECattlesGameStatus::ECattlesGamePushCard)
	{
		GamePlayerIdDataMap::iterator playerDataIt = roomData->gamePlayerData.find(cud->userName);
        if (playerDataIt != roomData->gamePlayerData.end()
            && roomData->playerInfo[playerDataIt->first].status() != com_protocol::ECattlesPlayerStatus::ECattlesOpenCard)
		{
			roomData->playerInfo[playerDataIt->first].set_status(com_protocol::ECattlesPlayerStatus::ECattlesOpenCard);
			
			if (req.result_value() != playerDataIt->second.resultValue)
			{
				// 5张牌计算出的牛X为唯一结果，只要玩家计算与服务器运算结果不一致则错误或作弊，一律设置为无牛
				playerDataIt->second.resultValue = 0;
				playerDataIt->second.settlementRate = 1;
			}
			
			com_protocol::CattlesPlayerOpenCardRsp rsp;
			rsp.set_result(SrvOptSuccess);

			m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, CATTLES_PLAYER_OPEN_CARD_RSP, "open card reply");
			
			OptInfoLog("player open card reply, usrename = %s", cud->userName);
			
			// 同步亮牌消息给其他玩家
			sendOpenCardNotify(*roomData, playerDataIt->first, "player open card notify",
			                   playerDataIt->first, !playerDataIt->second.isNewBanker);
			
			if (isFinishStatus(*roomData, com_protocol::ECattlesPlayerStatus::ECattlesOpenCard))
			{
	            roundSettlement(cud->hallId, cud->roomId, *roomData);  // 结算
			}
		}
	}
}

void CGameProcess::changeMoreUsersGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::MoreUsersItemChangeRsp rsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(rsp, data, len, "change more users goods reply")) return;

	if (rsp.failed_count() > 0)
	{
		for (google::protobuf::RepeatedPtrField<com_protocol::ItemChangeRsp>::const_iterator it = rsp.item_change_rsp().begin();
		     it != rsp.item_change_rsp().end(); ++it)
	    {
		    OptErrorLog("change more users goods error, username = %s, result = %d", it->src_username().c_str(), it->result());
	    }
	}
	
	OptInfoLog("change more users goods reply, flag = %d, failed count = %d",
	m_msgHandler->getContext().userFlag, rsp.failed_count());
}

void CGameProcess::setRoomGameRecordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetRoomGameRecordRsp rsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(rsp, data, len, "set room game record reply")) return;

	if (rsp.result() != SrvOptSuccess) OptErrorLog("set room game record reply error = %d", rsp.result());
}

}

