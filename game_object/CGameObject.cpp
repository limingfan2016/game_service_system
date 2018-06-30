
/* author : limingfan
 * date : 2018.03.28
 * description : 各游戏基础服务、接口实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "CGameObject.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace NDBOpt;
using namespace NProject;

namespace NPlatformService
{

// 数据记录日志
#define WriteDataLog(format, args...)     m_srvOpt.getDataLogger()->writeFile(NULL, 0, LogLevel::Info, format, ##args)


CGameMsgHandler::CGameMsgHandler() : m_srvOpt(sizeof(GameUserData))
{
	m_serviceHeartbeatTimerId = 0;
	m_checkHeartbeatCount = 0;
}

CGameMsgHandler::~CGameMsgHandler()
{
	m_serviceHeartbeatTimerId = 0;
	m_checkHeartbeatCount = 0;
}


void CGameMsgHandler::onUpdateConfig()
{
	m_srvOpt.updateCommonConfig(CCfg::getValue("Service", "ConfigFile"));

	if (!m_srvOpt.getServiceCommonCfg(true).isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update service xml config value error");
	}
	
	// 心跳更新
	const ServiceCommonConfig::HeartbeatCfg& heartbeatCfg = m_srvOpt.getServiceCommonCfg().heart_beat_cfg;
	stopHeartbeatCheck();
	startHeartbeatCheck(heartbeatCfg.heart_beat_interval, heartbeatCfg.heart_beat_count);

	m_gameLogicHandler.updateConfig();
	
	ReleaseInfoLog("update config business service result = %d",
	m_srvOpt.getServiceCommonCfg().isSetConfigValueSuccess());

	m_srvOpt.getServiceCommonCfg().output();
	
	onUpdate();
}

// 通知逻辑层对应的逻辑连接代理已被关闭
void CGameMsgHandler::onCloseConnectProxy(void* userData, int cbFlag)
{
	GameUserData* cud = (GameUserData*)userData;
	removeConnectProxy(string(cud->userName), 0, false);  // 有可能是被动关闭，所以这里要清除下连接信息

	exitGame(cud, cbFlag);  // 玩家退出游戏
}


void CGameMsgHandler::onInit()
{
}

void CGameMsgHandler::onUnInit()
{
}

void CGameMsgHandler::onUpdate()
{
}

int CGameMsgHandler::getSrvFlag()
{
	return 0;
}

void CGameMsgHandler::onLine(GameUserData* cud, ConnectProxy* conn, const com_protocol::DetailInfo& userDetailInfo)
{
}

void CGameMsgHandler::offLine(GameUserData* cud)
{
}


// 开始游戏
void CGameMsgHandler::onStartGame(const char* hallId, const char* roomId, SRoomData& roomData)
{	
}

void CGameMsgHandler::onHearbeatResult(const string& username, const bool isOnline)
{
	SRoomData* roomData = getRoomDataByUser(username.c_str());
	if (roomData == NULL) return;
	
	int newStatus = -1;
	if (roomData->onHearbeatStatusNotify(username, isOnline, newStatus))
	{
		// 同步玩家状态，发送消息到房间的其他玩家
		int rc = updatePlayerStatusNotify(*roomData, username.c_str(), newStatus, username.c_str(), "hear beat result");
		
		OptWarnLog("on hear beat result, username = %s, isOnline = %d, newStatus = %d, rc = %d",
		username.c_str(), isOnline, newStatus, rc);
	}
}

// 玩家离开房间
EGamePlayerStatus CGameMsgHandler::onLeaveRoom(SRoomData& roomData, const char* username)
{
	return EGamePlayerStatus::ELeaveRoom;
}

// 拒绝解散房间，恢复继续游戏
bool CGameMsgHandler::onRefuseDismissRoomContinueGame(const GameUserData* cud, SRoomData& roomData, CHandler*& instance, TimerHandler& timerHandler)
{
	instance = NULL;
	timerHandler = NULL;
	return false;
}
	
// 生成房间数据
SRoomData* CGameMsgHandler::createRoomData()
{
	SRoomData* roomData = NULL;
	NEW(roomData, SRoomData());
	return roomData;
}

// 销毁房间数据
void CGameMsgHandler::destroyRoomData(SRoomData* roomData)
{
	DELETE(roomData);
}

// 每一局房卡AA付费
float CGameMsgHandler::getAveragePayCount()
{
	return 0.00;
}
	
// 每一局房卡房主付费
float CGameMsgHandler::getCreatorPayCount()
{
	return 0.00;
}
	
bool CGameMsgHandler::checkRoomCardIsEnough(const SRoomData& roomData, const char* username, const float roomCardCount)
{
	const unsigned int payMode = roomData.roomInfo.base_info().pay_mode();
	const unsigned int gameTimes = roomData.roomInfo.base_info().game_times();

	return !((payMode == com_protocol::EHallRoomPayMode::ERoomAveragePay
		      && (roomCardCount < getAveragePayCount() * gameTimes))
		     || (payMode == com_protocol::EHallRoomPayMode::ERoomCreatorPay
		         && roomData.roomInfo.base_info().username() == username
			     && (roomCardCount < getCreatorPayCount() * gameTimes)));
}

// 是否可以开始游戏
int CGameMsgHandler::canStartGame(const SRoomData& roomData, bool isAuto)
{
	const unsigned int startPlayers = roomData.getStartPlayers();
	if (isAuto && startPlayers == 1 && roomData.playTimes < 1) return GameObjectNotAutoStartRoom;  // 需要手动开桌才能开始游戏
	if (!isAuto && startPlayers != 1) return GameObjectNotManualStartRoom;  // 只能自动开桌游戏

	unsigned int readyCount = 0;
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		if (it->second.status() == com_protocol::ERoomPlayerStatus::EPlayerReady) ++readyCount;
	}
	
	// 1、没有在坐的玩家且准备玩家个数两个以上则可以开始游戏（金币场、房卡手动场）
	// 2、满足自动开桌人数（房卡自动场）
	return (((roomData.isGoldRoom() || startPlayers == 1) && readyCount >= roomData.getMinGamePlayers())
	        || (startPlayers > 1 && readyCount >= startPlayers)) ? (int)SrvOptSuccess : GameObjectStartPlayerInsufficient;
}

// 是否可以解散房间
bool CGameMsgHandler::canDisbandGameRoom(const SRoomData& roomData)
{
	return true;
}

// 玩家是否可以离开房间
bool CGameMsgHandler::canLeaveRoom(GameUserData* cud)
{
	return true;
}
	
// 设置游戏房间数据
bool CGameMsgHandler::setGameRoomData(const string& username, const SRoomData& roomData,
									  google::protobuf::RepeatedPtrField<com_protocol::ClientRoomPlayerInfo>& roomPlayers,
									  com_protocol::ChessHallGameData& gameData)
{
	return false;
}


CServiceOperationEx& CGameMsgHandler::getSrvOpt()
{
	return m_srvOpt;
}

// 获取房间数据
SRoomData* CGameMsgHandler::getRoomDataByUser(const char* userName)
{
	GameUserData* cud = (GameUserData*)m_srvOpt.getConnectProxyUserData(userName);
	if (cud == NULL) return NULL;
	
	return getRoomDataById(cud->roomId, cud->hallId);
}

SRoomData* CGameMsgHandler::getRoomDataById(const char* roomId, const char* hallId)
{
	if (hallId == NULL)
	{
		for (HallDataMap::iterator hallIt = m_hallGameData.begin(); hallIt != m_hallGameData.end(); ++hallIt)
		{
			for (RoomDataMap::iterator ctsRoomIt = hallIt->second.gameRooms.begin(); ctsRoomIt != hallIt->second.gameRooms.end(); ++ctsRoomIt)
			{
				if (ctsRoomIt->first == roomId) return ctsRoomIt->second;
			}
		}
		
		return NULL;
	}
	
	HallDataMap::iterator hallIt = m_hallGameData.find(hallId);
	if (hallIt == m_hallGameData.end()) return NULL;
	
	RoomDataMap::iterator ctsRoomIt = hallIt->second.gameRooms.find(roomId);
	return (ctsRoomIt != hallIt->second.gameRooms.end()) ? ctsRoomIt->second : NULL;
}

SRoomData* CGameMsgHandler::getRoomData(unsigned int hallId, unsigned int roomId)
{
	char strHallId[IdMaxLen] = {0};
	snprintf(strHallId, sizeof(strHallId) - 1, "%u", hallId);
	
	char strRoomId[IdMaxLen] = {0};
	snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", roomId);
	
	return getRoomDataById(strRoomId, strHallId);
}

// 设置用户会话数据
int CGameMsgHandler::setUserSessionData(const char* username, unsigned int usernameLen, int status, unsigned int roomId)
{
	GameServiceData gameSrvData;
	gameSrvData.srvId = getSrvId();
	gameSrvData.roomId = roomId;
	gameSrvData.status = status;
	gameSrvData.timeSecs = time(NULL);

	return setUserSessionData(username, usernameLen, gameSrvData);
}

int CGameMsgHandler::setUserSessionData(const char* username, unsigned int usernameLen, const GameServiceData& gameSrvData)
{
	int rc = m_redisDbOpt.setHField(GameUserKey, GameUserKeyLen, username, usernameLen, (char*)&gameSrvData, sizeof(GameServiceData));
	if (rc != SrvOptSuccess) OptErrorLog("set user session data to redis error, username = %s, rc = %d", username, rc);
	
	return rc;
}

unsigned int CGameMsgHandler::delUserFromRoom(const StringVector& usernames)
{
	unsigned int count = 0;
	for (StringVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it)
	{
		if (delUserFromRoom(*it)) ++count;
	}
	
	return count;
}

// 解散房间，删除房间数据
bool CGameMsgHandler::delRoomData(const char* hallId, const char* roomId, bool isNotifyPlayer)
{
	HallDataMap::iterator hallIt = m_hallGameData.find(hallId);
	if (hallIt == m_hallGameData.end()) return false;
	
	RoomDataMap::iterator ctsRoomIt = hallIt->second.gameRooms.find(roomId);
	if (ctsRoomIt == hallIt->second.gameRooms.end()) return false;
	
	SRoomData* roomData = ctsRoomIt->second;
	StringVector delPlayers;
	for (RoomPlayerIdInfoMap::const_iterator it = roomData->playerInfo.begin(); it != roomData->playerInfo.end(); ++it)
	{
		delPlayers.push_back(it->first);
	}

	// 1）房间使用完毕，变更房间状态为使用完毕，同步通知到棋牌室大厅在线的玩家
	m_srvOpt.stopTimer(roomData->optTimerId);
	m_srvOpt.changeRoomDataNotify(hallId, roomId, getSrvType(), com_protocol::EHallRoomStatus::EHallRoomFinish, NULL, NULL, NULL, "room use finish");

	// 2）通知房间里的所有玩家退出房间
	if (isNotifyPlayer)
	{
	    com_protocol::ClientGameRoomFinishNotify finishNtf;
	    sendPkgMsgToRoomPlayers(*roomData, finishNtf, COMM_GAME_ROOM_FINISH_NOTIFY, "game room finish notify");
	}

	// 3）删除房间数据，防止用户离开房间导致递归调用此函数
	destroyRoomData(roomData);
	hallIt->second.gameRooms.erase(ctsRoomIt);
	if (hallIt->second.gameRooms.empty()) m_hallGameData.erase(hallIt);
	
	// 4）删除玩家心跳检测
	delHearbeatCheck(delPlayers);

	// 5）房间里的用户断开连接，离开房间
	const unsigned int count = delUserFromRoom(delPlayers);
	
	OptInfoLog("delete game room, hallId = %s, roomId = %s, remove player size = %u, count = %u",
	hallId, roomId, delPlayers.size(), count);

	return true;
}

bool CGameMsgHandler::delRoomData(unsigned int hallId, unsigned int roomId, bool isNotifyPlayer)
{
	char strHallId[IdMaxLen] = {0};
	snprintf(strHallId, sizeof(strHallId) - 1, "%u", hallId);
	
	char strRoomId[IdMaxLen] = {0};
	snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", roomId);
	
	return delRoomData(strHallId, strRoomId, isNotifyPlayer);
}

// 同步发送消息到房间的其他玩家
int CGameMsgHandler::sendPkgMsgToRoomPlayers(const SRoomData& roomData, const ::google::protobuf::Message& pkg, unsigned short protocolId,
											 const char* logInfo, const string& exceptName, int minStatus, int onlyStatus)
{
	const char* msgData = m_srvOpt.serializeMsgToBuffer(pkg, logInfo);
	if (msgData == NULL) return SerializeMessageToArrayError;

    // 房间玩家
	const bool needFixedStatus = (onlyStatus > -1);
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		if (exceptName != it->first)
		{
			if (needFixedStatus)
			{
				if (it->second.status() == onlyStatus) m_srvOpt.sendMsgToProxyEx(msgData, pkg.ByteSize(), protocolId, it->first.c_str());
			}
			else if (it->second.status() >= minStatus)
			{
				m_srvOpt.sendMsgToProxyEx(msgData, pkg.ByteSize(), protocolId, it->first.c_str());
			}
		}
	}

	return SrvOptSuccess;
}

// 同步发送消息到房间的在座玩家
int CGameMsgHandler::sendPkgMsgToRoomSitDownPlayers(const SRoomData& roomData, const ::google::protobuf::Message& pkg, unsigned short protocolId,
											        const char* logInfo, const string& exceptName)
{
	const char* msgData = m_srvOpt.serializeMsgToBuffer(pkg, logInfo);
	if (msgData == NULL) return SerializeMessageToArrayError;

    // 房间玩家
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		if (exceptName != it->first && it->second.seat_id() >= 0)
		{
			m_srvOpt.sendMsgToProxyEx(msgData, pkg.ByteSize(), protocolId, it->first.c_str());
		}
	}

	return SrvOptSuccess;
}

// 同步发送消息到房间的游戏中玩家
int CGameMsgHandler::sendPkgMsgToRoomOnPlayPlayers(const SRoomData& roomData, const ::google::protobuf::Message& pkg, unsigned short protocolId,
											        const char* logInfo, const string& exceptName)
{
	const char* msgData = m_srvOpt.serializeMsgToBuffer(pkg, logInfo);
	if (msgData == NULL) return SerializeMessageToArrayError;

    // 房间玩家
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		if (exceptName != it->first && roomData.isOnPlayer(it->first))
		{
			m_srvOpt.sendMsgToProxyEx(msgData, pkg.ByteSize(), protocolId, it->first.c_str());
		}
	}

	return SrvOptSuccess;
}

// 刷新游戏玩家状态通知
int CGameMsgHandler::updatePlayerStatusNotify(const SRoomData& roomData, const string& player, int newStatus, const string& exceptName, const char* logInfo)
{
	// 同步发送消息到房间的其他玩家
	com_protocol::UpdatePlayerStatusNotify updatePlayerStatusNtf;
	com_protocol::PlayerStatus* upPlayerStatus = updatePlayerStatusNtf.add_player_status();
	upPlayerStatus->set_username(player);
	upPlayerStatus->set_new_status(newStatus);

	return sendPkgMsgToRoomPlayers(roomData, updatePlayerStatusNtf, COMM_UPDATE_PLAYER_STATUS_NOTIFY, logInfo, exceptName);
}

int CGameMsgHandler::updatePlayerStatusNotify(const SRoomData& roomData, const StringVector& players, int newStatus, const string& exceptName, const char* logInfo)
{
	if (players.empty()) return InvalidParameter;

	// 同步发送消息到房间的其他玩家
	com_protocol::UpdatePlayerStatusNotify updatePlayerStatusNtf;
	for (StringVector::const_iterator it = players.begin(); it != players.end(); ++it)
	{
		com_protocol::PlayerStatus* upPlayerStatus = updatePlayerStatusNtf.add_player_status();
		upPlayerStatus->set_username(*it);
		upPlayerStatus->set_new_status(newStatus);
	}
	
	return sendPkgMsgToRoomPlayers(roomData, updatePlayerStatusNtf, COMM_UPDATE_PLAYER_STATUS_NOTIFY, logInfo, exceptName);
}

// 刷新准备按钮状态
int CGameMsgHandler::updatePrepareStatusNotify(const SRoomData& roomData, int status, const char* logInfo)
{
	com_protocol::ClientPrepareStatusNotify prepareStatusNtf;
	prepareStatusNtf.set_prepare_status(status);
	
	return sendPkgMsgToRoomPlayers(roomData, prepareStatusNtf, COMM_PREPARE_STATUS_NOTIFY,
                                   logInfo, "", com_protocol::ERoomPlayerStatus::EPlayerEnter);
}

void CGameMsgHandler::setOptTimeOutInfo(unsigned int hallId, unsigned int roomId, SRoomData& roomData,
                                        unsigned int waitSecs, CHandler* instance, TimerHandler timerHandler, unsigned int optType)
{
	m_srvOpt.stopTimer(roomData.optTimerId);
	
	roomData.optLastTimeSecs = waitSecs;
	roomData.optTimeSecs = time(NULL);
	roomData.optTimerId = setTimer(waitSecs * MillisecondUnit, timerHandler, hallId, (void*)(unsigned long)roomId, 1, instance);
	roomData.nextOptType = optType;
}

void CGameMsgHandler::refuseDismissRoomContinueGame(const GameUserData* cud, SRoomData& roomData, bool isCancelDismissNotify)
{
	if (roomData.dismissRoomTimerId == 0 || roomData.agreeDismissRoomPlayer.empty()
	    || std::find(roomData.agreeDismissRoomPlayer.begin(), roomData.agreeDismissRoomPlayer.end(),
		             cud->userName) != roomData.agreeDismissRoomPlayer.end()) return;

	// 恢复继续游戏
	CHandler* instance = NULL;
	TimerHandler timerHandler = NULL;
	bool isNotifyTimeOutSecs = onRefuseDismissRoomContinueGame(cud, roomData, instance, timerHandler);
	if (instance == NULL || timerHandler == NULL) return;
	
	// 停止解散房间定时器
	m_srvOpt.stopTimer(roomData.dismissRoomTimerId);
	
	// 取消解散房间通知
	if (isCancelDismissNotify)
	{
		com_protocol::ClientCancelDismissRoomNotify cdrNtf;
		sendPkgMsgToRoomSitDownPlayers(roomData, cdrNtf, COMM_PLAYER_CANCEL_DISMISS_ROOM_NOTIFY,
		                               "cancel dismiss room notify", cud->userName);
	}
	
	// 剩余倒计时时间
	if (roomData.optTimerId == 0 && roomData.optTimeSecs > 0 && roomData.optLastTimeSecs > 0)
	{
		const unsigned int pauseTimeSecs = roomData.playerDismissRoomTime[roomData.agreeDismissRoomPlayer[0]];
		const unsigned int passTimeSecs = pauseTimeSecs - roomData.optTimeSecs;
		const unsigned int remainTimeSecs = (roomData.optLastTimeSecs > passTimeSecs) ? (roomData.optLastTimeSecs - passTimeSecs) : 0;

		setOptTimeOutInfo(atoi(cud->hallId), atoi(cud->roomId), roomData, remainTimeSecs,
		                  instance, timerHandler, roomData.nextOptType);
		
		if (isNotifyTimeOutSecs && remainTimeSecs > 0)
		{
			com_protocol::ClientTimeOutSecondsNotify tosNtf;
			tosNtf.set_wait_secs(remainTimeSecs);
			sendPkgMsgToRoomSitDownPlayers(roomData, tosNtf, COMM_TIME_OUT_SECONDS_NOTIFY, "continue game time out seconds notify");
		}
	}
	
	roomData.agreeDismissRoomPlayer.clear();
}


// 设置玩家掉线重来信息
void CGameMsgHandler::setReEnterInfo(const ConstCharPointerVector& usernames, const char* hallId, const char* roomId, EGamePlayerStatus status)
{
	// 一局游戏开局，设置玩家掉线信息
	// 掉线的方式存在多种，必须在游戏开始前设置（app异常关闭、网络异常、中间路由异常、心跳异常等等）
	// 如果玩家在游戏中掉线，则依据掉线信息重连回房间
	GameServiceData gameSrvData;
	gameSrvData.srvId = getSrvId();
	gameSrvData.hallId = atoi(hallId);
	gameSrvData.roomId = atoi(roomId);
	gameSrvData.status = status;
	gameSrvData.timeSecs = time(NULL);
	
	for (ConstCharPointerVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it)
	{
		// 如果房间解散时，掉线玩家还没有重入房间，则此数据需要删除（当局游戏结束清理掉线玩家时会删除）
		setUserSessionData(*it, strlen(*it), gameSrvData);
	}
}

// 设置玩家掉线重来信息
void CGameMsgHandler::setReEnterInfo(const StringVector& usernames, const char* hallId, const char* roomId, EGamePlayerStatus status)
{
	// 一局游戏开局，设置玩家掉线信息
	// 掉线的方式存在多种，必须在游戏开始前设置（app异常关闭、网络异常、中间路由异常、心跳异常等等）
	// 如果玩家在游戏中掉线，则依据掉线信息重连回房间
	GameServiceData gameSrvData;
	gameSrvData.srvId = getSrvId();
	gameSrvData.hallId = atoi(hallId);
	gameSrvData.roomId = atoi(roomId);
	gameSrvData.status = status;
	gameSrvData.timeSecs = time(NULL);
	
	for (StringVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it)
	{
		// 如果房间解散时，掉线玩家还没有重入房间，则此数据需要删除（当局游戏结束清理掉线玩家时会删除）
		setUserSessionData(it->c_str(), it->length(), gameSrvData);
	}
}

// 多用户物品结算
int CGameMsgHandler::moreUsersGoodsSettlement(const char* hallId, const char* roomId, SRoomData& roomData,
										      GameSettlementMap& gameSettlementInfo, const char* remark, int roomFlag)
{
	// 结算变更道具数量
	RecordIDType recordId = {0};
	m_srvOpt.getRecordId(recordId);

    // 需要写数据日志信息，作为第一依据及统计使用
	com_protocol::MoreUsersItemChangeReq itemChangeReq;
	itemChangeReq.set_hall_id(hallId);

	com_protocol::ChangeRecordBaseInfo* recordInfo = itemChangeReq.mutable_base_info();
	recordInfo->set_opt_type(com_protocol::EHallPlayerOpt::EOptProfitLoss);
	recordInfo->set_record_id(recordId);
	recordInfo->set_hall_id(hallId);
	recordInfo->set_game_type(getSrvType());
	recordInfo->set_room_id(roomId);
	recordInfo->set_room_rate(roomData.roomInfo.base_info().base_rate());
    recordInfo->set_room_flag(roomFlag);
	recordInfo->set_service_id(getSrvId());
	recordInfo->set_remark(remark);

	const float serviceTaxRatio = roomData.roomInfo.base_info().service_tax_ratio();
	const bool needTaxDeduction = (serviceTaxRatio > 0.000);
	com_protocol::ItemChangeReq* itemChange = NULL;
	float taxValue = 0.00;
	for (GameSettlementMap::iterator gstInfoIt = gameSettlementInfo.begin(); gstInfoIt != gameSettlementInfo.end(); ++gstInfoIt)
	{
	    itemChange = itemChangeReq.add_item_change_req();
        itemChange->set_write_data_log(1);
		itemChange->set_hall_id(hallId);
		itemChange->set_src_username(gstInfoIt->first);
		
		roomData.writePlayerItemChangeInfo(gstInfoIt->first, itemChange);

		// 是否需要扣税扣服务费用
		if (needTaxDeduction && gstInfoIt->second.num() > 0.00)
		{
			taxValue = gstInfoIt->second.num() * serviceTaxRatio;
			gstInfoIt->second.set_num(gstInfoIt->second.num() - taxValue);
			
			// 扣税扣服务费用记录
			com_protocol::ItemChangeReq* taxDeductionItem = itemChangeReq.add_item_change_req();
			taxDeductionItem->set_write_data_log(1);
			taxDeductionItem->set_hall_id(hallId);
			taxDeductionItem->set_src_username(gstInfoIt->first);
			
			com_protocol::ItemChange* taxItemInfo = taxDeductionItem->add_items();
			taxItemInfo->set_type(gstInfoIt->second.type());
			taxItemInfo->set_num(taxValue);
			
			com_protocol::ChangeRecordBaseInfo* taxRecordInfo = taxDeductionItem->mutable_base_info();
			*taxRecordInfo = *recordInfo;
			taxRecordInfo->set_opt_type(com_protocol::EHallPlayerOpt::EOptTaxCost);
			taxRecordInfo->set_remark("settlement tax deduction");

            WriteDataLog("settlement tax deduction|%s|%u|%.3f",
			gstInfoIt->first.c_str(), taxItemInfo->type(), taxItemInfo->num());			
		}
		
		*itemChange->add_items() = gstInfoIt->second;
	}
	
	int rc = m_srvOpt.sendPkgMsgToService("", 0, ServiceType::DBProxySrv, itemChangeReq, DBPROXY_CHANGE_MORE_USERS_ITEM_REQ, "more user goods settlement");
    
    WriteDataLog("more user goods settlement|%s|%s|%s|%u|%d", hallId, roomId, recordId, itemChangeReq.item_change_req_size(), rc);

    return rc;
}

int CGameMsgHandler::moreUsersGoodsSettlement(unsigned int hallId, unsigned int roomId, SRoomData& roomData,
								              GameSettlementMap& gameSettlementInfo, const char* remark, int roomFlag)
{
	char strHallId[IdMaxLen] = {0};
	snprintf(strHallId, sizeof(strHallId) - 1, "%u", hallId);
	
	char strRoomId[IdMaxLen] = {0};
	snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", roomId);
	
	return moreUsersGoodsSettlement(strHallId, strRoomId, roomData, gameSettlementInfo, remark, roomFlag);
}

// 单用户金币变更
int CGameMsgHandler::changeUserGold(const char* hallId, const char* roomId, SRoomData& roomData,
									const string& username, float changeValue, const char* remark, int roomFlag)
{
	if (!roomData.isGoldRoom()) return -1;

	GameSettlementMap gameSettlementInfo;
	com_protocol::ItemChange& item = gameSettlementInfo[username];
	item.set_type(EGameGoodsType::EGoodsGold);
	item.set_num(changeValue);
	return moreUsersGoodsSettlement(hallId, roomId, roomData, gameSettlementInfo, remark, roomFlag);
}

int CGameMsgHandler::changeUserGold(unsigned int hallId, unsigned int roomId, SRoomData& roomData,
									const string& username, float changeValue, const char* remark, int roomFlag)
{
	if (!roomData.isGoldRoom()) return -1;

	GameSettlementMap gameSettlementInfo;
	com_protocol::ItemChange& item = gameSettlementInfo[username];
	item.set_type(EGameGoodsType::EGoodsGold);
	item.set_num(changeValue);
	return moreUsersGoodsSettlement(hallId, roomId, roomData, gameSettlementInfo, remark, roomFlag);
}

// 房卡扣费
int CGameMsgHandler::payRoomCard(const char* hallId, const char* roomId, SRoomData& roomData, int roomFlag)
{
	RecordIDType recordId = {0};
	m_srvOpt.getRecordId(recordId);

    // 需要写数据日志信息，作为第一依据及统计使用
	com_protocol::MoreUsersItemChangeReq itemChangeReq;
	itemChangeReq.set_hall_id(hallId);

	com_protocol::ChangeRecordBaseInfo* recordInfo = itemChangeReq.mutable_base_info();
	recordInfo->set_opt_type(com_protocol::EHallPlayerOpt::EOptPayRoomCard);
	recordInfo->set_record_id(recordId);
	recordInfo->set_hall_id(hallId);
	recordInfo->set_game_type(getSrvType());
	recordInfo->set_room_id(roomId);
	recordInfo->set_room_rate(roomData.roomInfo.base_info().base_rate());
    recordInfo->set_room_flag(roomFlag);
	recordInfo->set_service_id(getSrvId());
	recordInfo->set_remark("pay room card");

	const unsigned int gameTimes = roomData.roomInfo.base_info().game_times();
    if (roomData.roomInfo.base_info().pay_mode() == com_protocol::EHallRoomPayMode::ERoomAveragePay)
	{
	    roomData.payRoomCard(hallId, -(float)(getAveragePayCount() * gameTimes), itemChangeReq);
	}
	else
	{
		com_protocol::ItemChangeReq* itemChange = itemChangeReq.add_item_change_req();
		itemChange->set_write_data_log(1);
		
		itemChange->set_hall_id(hallId);
		itemChange->set_src_username(roomData.roomInfo.base_info().username());
		
		com_protocol::ItemChange* item = itemChange->add_items();
		item->set_type(EGameGoodsType::EGoodsRoomCard);
		item->set_num(-(float)(getCreatorPayCount() * gameTimes));
	}

	int rc = m_srvOpt.sendPkgMsgToService("", 0, ServiceType::DBProxySrv, itemChangeReq, DBPROXY_CHANGE_MORE_USERS_ITEM_REQ, "pay room card", 1);
    
    WriteDataLog("pay room card|%s|%s|%s|%u|%d", hallId, roomId, recordId, itemChangeReq.item_change_req_size(), rc);

    return rc;
}

// 房卡总结算
void CGameMsgHandler::cardCompleteSettlement(const char* hallId, const char* roomId, SRoomData& roomData, bool isNotify, int roomFlag)
{
	com_protocol::ClientRoomTotalSettlementNotify rtStNtf;
	if (roomData.roomGameRecord.round_win_lose_size() > 0)
	{
		// 房卡总结算记录
		com_protocol::ChangeRecordBaseInfo* recordInfo = roomData.roomGameRecord.mutable_base_info();
		recordInfo->set_opt_type(com_protocol::EHallPlayerOpt::EOptWinLoseResult);
		recordInfo->set_record_id("");
		recordInfo->set_hall_id(hallId);
		recordInfo->set_game_type(getSrvType());
		recordInfo->set_room_id(roomId);
		recordInfo->set_room_rate(roomData.roomInfo.base_info().base_rate());
		recordInfo->set_room_flag(roomFlag);
		recordInfo->set_service_id(getSrvId());
		recordInfo->set_remark("room game record");

		int rc = m_srvOpt.sendPkgMsgToService("", 0, ServiceType::DBProxySrv,
		roomData.roomGameRecord, DBPROXY_SET_ROOM_GAME_RECORD_REQ, "set room game record");

		WriteDataLog("set room game record|%s|%s|%d", hallId, roomId, rc);
		
		// 房卡总结算通知
		typedef unordered_map<string, float> PlayerSumWinLoseMap;
		PlayerSumWinLoseMap playerSumWinLose;
		for (google::protobuf::RepeatedPtrField<com_protocol::RoundWinLoseInfo>::const_iterator roundIt = roomData.roomGameRecord.round_win_lose().begin();
			 roundIt != roomData.roomGameRecord.round_win_lose().end(); ++roundIt)
		{
			for (google::protobuf::RepeatedPtrField<com_protocol::ChangeRecordPlayerInfo>::const_iterator playerIt = roundIt->player_info().begin();
				 playerIt != roundIt->player_info().end(); ++playerIt)
			{
				playerSumWinLose[playerIt->username()] += playerIt->win_lose_value();
			}
		}

		for (PlayerSumWinLoseMap::const_iterator it = playerSumWinLose.begin(); it != playerSumWinLose.end(); ++it)
		{
			com_protocol::PlayerWinLoseInfo* wlIf = rtStNtf.add_win_lose_info();
			*wlIf->mutable_static_info() = roomData.playerInfo[it->first].detail_info().static_info();
			wlIf->set_sum_win_lose(it->second);
		}
	}
	else
	{
		for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
		{
			if (it->second.seat_id() >= 0)
			{
				com_protocol::PlayerWinLoseInfo* wlIf = rtStNtf.add_win_lose_info();
				*wlIf->mutable_static_info() = it->second.detail_info().static_info();
				wlIf->set_sum_win_lose(0.00);
			}
		}
	}

	sendPkgMsgToRoomPlayers(roomData, rtStNtf, COMM_ROOM_TOTAL_SETTLEMENT_NOTIFY, "room total settlement notify");
	
	if (isNotify)
	{
		// 通知客户端结束游戏
		com_protocol::ClientFinishGameNotify finishGameNtf;
		sendPkgMsgToRoomPlayers(roomData, finishGameNtf, COMM_FINISH_GAME_NOTIFY, "finish game notify");
	}
}


bool CGameMsgHandler::startHeartbeatCheck(unsigned int checkIntervalSecs, unsigned int checkCount)
{
	if (checkIntervalSecs <= 1 || checkCount <= 1) return false;

	m_serviceHeartbeatTimerId = setTimer(checkIntervalSecs * MillisecondUnit, (TimerHandler)&CGameMsgHandler::onHeartbeatCheck, 0, NULL, -1);
	m_checkHeartbeatCount = checkCount;
	
	return (m_serviceHeartbeatTimerId != 0);
}

void CGameMsgHandler::stopHeartbeatCheck()
{
	m_srvOpt.stopTimer(m_serviceHeartbeatTimerId);
}

void CGameMsgHandler::addHearbeatCheck(const ConstCharPointerVector& usernames)
{
	for (ConstCharPointerVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it) addHearbeatCheck(*it);
}

void CGameMsgHandler::addHearbeatCheck(const StringVector& usernames)
{
	for (StringVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it) addHearbeatCheck(*it);
}

void CGameMsgHandler::addHearbeatCheck(const string& username)
{
	ConnectProxy* conn = getConnectProxy(username);
	if (conn == NULL)
	{
		onHeartbeatCheckFail(username);
	}
	else
	{
		sendMsgToProxy(NULL, 0, COMM_SERVICE_HEART_BEAT_NOTIFY, conn);  // 发送心跳消息
		
		m_heartbeatFailedTimes[username] = 1;                           // 发送了1次心跳消息
	}
}

void CGameMsgHandler::delHearbeatCheck(const ConstCharPointerVector& usernames)
{
	for (ConstCharPointerVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it) delHearbeatCheck(*it);
}

void CGameMsgHandler::delHearbeatCheck(const StringVector& usernames)
{
	for (StringVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it) delHearbeatCheck(*it);
}

void CGameMsgHandler::delHearbeatCheck(const string& username)
{
	m_heartbeatFailedTimes.erase(username);
}

void CGameMsgHandler::clearHearbeatCheck()
{
	m_heartbeatFailedTimes.clear();
}

void CGameMsgHandler::addWaitForDisbandRoom(const unsigned int hallId, const unsigned int roomId)
{
	unsigned long long hallRoomId = hallId;
	*((unsigned int*)&hallRoomId + 1) = roomId;
	
	// 空房间解散时间点
	m_waitForDisbandRoomInfo[hallRoomId] = time(NULL) + m_srvOpt.getServiceCommonCfg().game_cfg.disband_room_wait_secs;
}

void CGameMsgHandler::delWaitForDisbandRoom(const unsigned int hallId, const unsigned int roomId)
{
	unsigned long long hallRoomId = hallId;
	*((unsigned int*)&hallRoomId + 1) = roomId;

	m_waitForDisbandRoomInfo.erase(hallRoomId);
}

// 解散所有等待的空房间
void CGameMsgHandler::disbandAllWaitForRoom()
{
	char strHallId[IdMaxLen] = {0};
	char strRoomId[IdMaxLen] = {0};
	for (WaitForDisbandRoomInfoMap::iterator it = m_waitForDisbandRoomInfo.begin(); it != m_waitForDisbandRoomInfo.end(); ++it)
	{
		snprintf(strHallId, sizeof(strHallId) - 1, "%u", (unsigned int)it->first);
		snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", *((unsigned int*)&(it->first) + 1));
		
		delRoomData(strHallId, strRoomId);
	}
}

void CGameMsgHandler::startCheckDisbandRoom()
{
	unsigned int checkDisbandRoomIntervalSecs = m_srvOpt.getServiceCommonCfg().game_cfg.check_disband_room_interval;
	if (checkDisbandRoomIntervalSecs < 5) checkDisbandRoomIntervalSecs = 5;
	setTimer(checkDisbandRoomIntervalSecs * MillisecondUnit, (TimerHandler)&CGameMsgHandler::onCheckDisbandRoom, 0, NULL, (unsigned int)-1);
}

// 检测待解散的空房间，超时时间到则解散房间
void CGameMsgHandler::onCheckDisbandRoom(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	const unsigned int curSecs = time(NULL);
	char strHallId[IdMaxLen] = {0};
	char strRoomId[IdMaxLen] = {0};
	SRoomData* roomData = NULL;
	for (WaitForDisbandRoomInfoMap::iterator it = m_waitForDisbandRoomInfo.begin(); it != m_waitForDisbandRoomInfo.end();)
	{
		snprintf(strHallId, sizeof(strHallId) - 1, "%u", (unsigned int)it->first);
		snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", *((unsigned int*)&(it->first) + 1));
		
		roomData = getRoomDataById(strRoomId, strHallId);
		if (roomData == NULL || (curSecs > it->second && canDisbandGameRoom(*roomData)))
		{
			delRoomData(strHallId, strRoomId);

			m_waitForDisbandRoomInfo.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

// 定时保存数据到redis服务
void CGameMsgHandler::saveDataToRedis(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	m_serviceInfo.set_update_timestamp(time(NULL));
	const char* gameSrvInfo = m_srvOpt.serializeMsgToBuffer(m_serviceInfo, "save cattles service data to redis");
	if (gameSrvInfo != NULL)
	{
		// 设置服务存活数据
		static const unsigned int srvId = getSrvId();
	    int rc = m_redisDbOpt.setHField(GameServiceListKey, GameServiceListLen, (const char*)&srvId, sizeof(srvId), gameSrvInfo, m_serviceInfo.ByteSize());
	    if (rc != 0) OptErrorLog("set game service data to redis center service failed, rc = %d", rc);
	}
}


void CGameMsgHandler::heartbeatCheck(const string& username)
{
	HeartbeatFailedTimesMap::iterator it = m_heartbeatFailedTimes.find(username);
	if (it != m_heartbeatFailedTimes.end())
	{
		if (it->second == 0) onHeartbeatCheckSuccess(username);  // 回调过心跳失败函数后重新收到消息
		
		it->second = 1;
	}
}

void CGameMsgHandler::onHeartbeatCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	ConnectProxy* conn = NULL;
	for (HeartbeatFailedTimesMap::iterator it = m_heartbeatFailedTimes.begin(); it != m_heartbeatFailedTimes.end();)
	{
		conn = getConnectProxy(it->first);
		if (conn == NULL)
		{
			onHeartbeatCheckFail(it->first);

			m_heartbeatFailedTimes.erase(it++);
		}
		else
		{
			if (it->second > 0 && ++it->second > m_checkHeartbeatCount)  // 心跳检测失败了
			{
				onHeartbeatCheckFail(it->first);

				it->second = 0;  // 表示心跳检测失败，回调了心跳失败函数
			}

			sendMsgToProxy(NULL, 0, COMM_SERVICE_HEART_BEAT_NOTIFY, conn);

			++it;
		}
	}
}

void CGameMsgHandler::onHeartbeatCheckFail(const string& username)
{
	onHearbeatResult(username, false);
}

void CGameMsgHandler::onHeartbeatCheckSuccess(const string& username)
{
	onHearbeatResult(username, true);
}

// 是否是棋牌室审核通过的正式玩家
bool CGameMsgHandler::isHallPlayer(const GameUserData* cud)
{
	// 棋牌室管理员审核通过才能成为棋牌室正式玩家，才可以参与游戏
	return (cud->inHallStatus == com_protocol::EHallPlayerStatus::ECheckAgreedPlayer);
}

com_protocol::ClientRoomPlayerInfo* CGameMsgHandler::getPlayerInfoBySeatId(RoomPlayerIdInfoMap& playerInfo, const int seatId)
{
	for (RoomPlayerIdInfoMap::iterator it = playerInfo.begin(); it != playerInfo.end(); ++it)
	{
		if (it->second.seat_id() == seatId) return &it->second;
	}
	
	return NULL;
}

int CGameMsgHandler::getFreeSeatId(GameUserData* cud, SRoomData* roomData, int& seatId)
{
	const int maxPlayerCount = roomData->roomInfo.base_info().player_count();
	if (cud->seatId >= 0)
	{
		// 玩家选定座位
		if (cud->seatId >= maxPlayerCount) return GameObjectInvalidSeatID;

        const com_protocol::ClientRoomPlayerInfo* plInfo = getPlayerInfoBySeatId(roomData->playerInfo, cud->seatId);
		if (plInfo != NULL
		    && plInfo->detail_info().static_info().username() != cud->userName) return GameObjectRoomSeatInUse;
		
		seatId = cud->seatId;
	}
	else
	{
		if ((int)roomData->getSeatedPlayers() >= maxPlayerCount) return GameObjectRoomSeatFull;

		// 服务器自动分配座位
		seatId = 0;
		for (; seatId < maxPlayerCount; ++seatId)
		{
			if (getPlayerInfoBySeatId(roomData->playerInfo, seatId) == NULL) break;
		}
		
		if (seatId >= maxPlayerCount) return GameObjectRoomSeatFull;
	}
	
	return SrvOptSuccess;
}

int CGameMsgHandler::userSitDown(GameUserData* cud, SRoomData* roomData)
{
	int rc = SrvOptFailed;
	RoomPlayerIdInfoMap::iterator userInfoIt = roomData->playerInfo.find(cud->userName);
	do
	{
		if (userInfoIt == roomData->playerInfo.end())
		{
			rc = GameObjectNotRoomUserInfo;
			break;
		}
		
		// 必须是审核通过的棋牌室玩家才可以参与游戏
		if (!isHallPlayer(cud))
		{
			rc = GameObjectNoHallCheckOKUser;
			break;
		}

		if (roomData->isGoldRoom())
		{
			// 最小下注额
			if (userInfoIt->second.detail_info().dynamic_info().game_gold() < roomData->getPlayerNeedMinGold())
			{
				rc = GameObjectGameGoldInsufficient;
				break;
			}
		}
		else if (!checkRoomCardIsEnough(*roomData, cud->userName, userInfoIt->second.detail_info().dynamic_info().room_card()))
		{
			rc = GameObjectRoomCardInsufficient;
			break;
		}

		int seatId = -1;
		rc = getFreeSeatId(cud, roomData, seatId);
		if (rc != SrvOptSuccess) break;

		// 玩家坐下，占用座位号
		cud->seatId = seatId;
		userInfoIt->second.set_seat_id(cud->seatId);
		userInfoIt->second.set_status(com_protocol::ERoomPlayerStatus::EPlayerSitDown);  // 玩家状态为坐下
		
		// 玩家可能重入空房间，因此这里删除空房间超时记录
		delWaitForDisbandRoom(atoi(cud->hallId), atoi(cud->roomId));

	} while (false);
	
	if (rc != SrvOptSuccess)
	{
		cud->seatId = -1;  // 占座失败

		return rc;
	}

	// 1）有玩家坐下，先变更玩家的座位号（登陆大厅的玩家才能看到该玩家坐下）
    m_srvOpt.changeRoomPlayerSeat(cud->roomId, cud->userName, cud->seatId, "change player seat");

	if (roomData->roomStatus == 0)
	{
		// 2）第一次有玩家坐下，变更房间状态为游戏准备中（登陆大厅的玩家才能获取到该房间信息），同时通知到棋牌室大厅在线玩家
		roomData->roomStatus = com_protocol::EHallRoomStatus::EHallRoomReady;
		m_srvOpt.changeRoomDataNotify(cud->hallId, cud->roomId, getSrvType(), com_protocol::EHallRoomStatus::EHallRoomReady,
		                              &roomData->roomInfo.base_info(), &userInfoIt->second, NULL, "room ready");
	}
	else
	{
		// 2）有玩家坐下，通知到棋牌室大厅在线玩家
        m_srvOpt.updateRoomDataNotifyHall(cud->hallId, cud->roomId, getSrvType(), com_protocol::EHallRoomStatus::EHallRoomReady,
		                                  NULL, &userInfoIt->second, NULL, "player sit down notify hall");
	}

	return SrvOptSuccess;
}

// 进入房间session key检查
int CGameMsgHandler::checkSessionKey(const string& username, const string& sessionKey, SessionKeyData& sessionKeyData)
{
	// 验证会话数据是否正确
	if (m_redisDbOpt.getHField(NProject::SessionKey, NProject::SessionKeyLen, username.c_str(), username.length(),
		(char*)&sessionKeyData, sizeof(sessionKeyData)) != sizeof(sessionKeyData)) return GameObjectGetSessionKeyError;
	
	char sessionKeyStr[NProject::IdMaxLen] = {0};
	NProject::getSessionKey(sessionKeyData, sessionKeyStr);
	if (sessionKey != sessionKeyStr) return GameObjectCheckSessionKeyFailed;
	
	return SrvOptSuccess;
}

// 删除用户会话数据
void CGameMsgHandler::delUserSessionData(const char* username, unsigned int usernameLen)
{
	int rc = m_redisDbOpt.delHField(GameUserKey, GameUserKeyLen, username, usernameLen);
	if (rc < 0) OptErrorLog("delete user session data from redis error, username = %s, rc = %d", username, rc);
}

// 玩家退出游戏
void CGameMsgHandler::exitGame(void* cb, int quitStatus)
{
	GameUserData* ud = (GameUserData*)cb;
	if (ud == NULL)
	{
		OptErrorLog("logout notify but user data is null, quitStatus = %d", quitStatus);
		return;
	}

	EGamePlayerStatus playerStatus = EGamePlayerStatus::ELeaveRoom;
	unsigned int onlineTimeSecs = 0;

	if (m_srvOpt.isCheckSuccessConnectProxy(ud))
	{
		// 玩家下线
		offLine(ud);
		onlineTimeSecs = time(NULL) - ud->timeSecs;  // 在线时长
		if (m_serviceInfo.current_persons() > 0) m_serviceInfo.set_current_persons(m_serviceInfo.current_persons() - 1);

		// 玩家离开游戏房间
		playerStatus = userLeaveRoom(ud, quitStatus);

		// 如果是座位上的玩家离开房间则需要通知大厅里在线的玩家
		SRoomData* roomData = getRoomDataById(ud->roomId, ud->hallId);
		if (roomData != NULL)
		{
			if (ud->seatId >= 0)  //  && roomData != NULL && roomData->hasSeatPlayer())
			{
				// 有玩家从座位上离开还没有解散的房间，则同步消息到大厅
				m_srvOpt.updateRoomDataNotifyHall(ud->hallId, ud->roomId, getSrvType(), (com_protocol::EHallRoomStatus)roomData->roomStatus,
												  NULL, NULL, ud->userName, "player leave room notify hall");
			}
			
			// 所有在座的玩家离开房间，导致房间可以解散
			// 但需要保留一段时间，玩家可能会重入房间
			if (playerStatus == EGamePlayerStatus::ELeaveRoom && canDisbandGameRoom(*roomData))
			{
				addWaitForDisbandRoom(atoi(ud->hallId), atoi(ud->roomId));
			}
		}

		if (playerStatus != EGamePlayerStatus::EDisconnectRoom)
		{
			delUserSessionData(ud->userName, ud->userNameLen);  // 不是游戏玩家掉线，则删除用户会话数据
		}
		
		// 最后才通知服务用户已经logout（离线则会自动变更座位号为无效）
		// 玩家离开可能导致房间解散，则修改房间状态（使用完毕）在先，玩家离线状态在后
		// 否则大厅里登录的玩家可能会获取到没有在线玩家的空房间错误
		m_srvOpt.sendUserOfflineNotify(ud->userName, getSrvId(), ud->hallId, ud->roomId, quitStatus, "", onlineTimeSecs);
	}
	
	OptInfoLog("game user logout, name = %s, connect status = %d, online time = %u, quit status = %d, player status = %d",
	ud->userName, ud->status, onlineTimeSecs, quitStatus, playerStatus);
	
	m_srvOpt.destroyConnectProxyUserData(ud);
}


int CGameMsgHandler::addRoomData(const com_protocol::ChessHallRoomInfo& roomInfo)
{		
	SHallData& hallCattlesData = m_hallGameData[roomInfo.base_info().hall_id()];
	SRoomData* roomData = createRoomData();
	hallCattlesData.gameRooms[roomInfo.base_info().room_id()] = roomData;

	// 游戏状态为准备状态
	roomData->roomInfo = roomInfo;
	roomData->gameStatus = com_protocol::EGameStatus::EGameReady;
	
	OptInfoLog("add game room, hallId = %s, roomId = %s, type = %d, rate = %u, player count = %u",
	roomInfo.base_info().hall_id().c_str(), roomInfo.base_info().room_id().c_str(),
	roomInfo.base_info().room_type(), roomInfo.base_info().base_rate(), roomInfo.base_info().player_count());
	
	return SrvOptSuccess;
}

void CGameMsgHandler::addUserToRoom(const GameUserData* cud, SRoomData* roomData, const com_protocol::DetailInfo& userDetailInfo)
{
	com_protocol::ClientRoomPlayerInfo& newPlayerInfo = roomData->playerInfo[cud->userName];
	*newPlayerInfo.mutable_detail_info() = userDetailInfo;
	newPlayerInfo.set_seat_id(-1);  // 此时没有座位号
	newPlayerInfo.set_status(com_protocol::ERoomPlayerStatus::EPlayerEnter);  // 玩家状态为进入房间
	
	if (roomData->roomStatus == 0 && roomData->roomInfo.base_info().username() == cud->userName)
	{
		// 房主最开始第一次进入房间
		// 则默认变更房间状态为游戏准备中（登陆大厅的玩家才能获取到该房间信息），同时通知到棋牌室大厅在线玩家
		roomData->roomStatus = com_protocol::EHallRoomStatus::EHallRoomReady;
		m_srvOpt.changeRoomDataNotify(cud->hallId, cud->roomId, getSrvType(), com_protocol::EHallRoomStatus::EHallRoomReady,
		                              &roomData->roomInfo.base_info(), &newPlayerInfo, NULL, "default room ready");
	}
}

bool CGameMsgHandler::delUserFromRoom(const string& userName)
{
	// 已经掉线了的玩家，在真正离开房间前没有重新连入房间，则之前的会话数据需要清除
	bool isOK = removeConnectProxy(userName, com_protocol::EUserQuitStatus::EUserOffline);  // 删除与客户端的连接代理，不会关闭连接
	if (!isOK) delUserSessionData(userName.c_str(), userName.length());

	return isOK;
}

void CGameMsgHandler::userEnterRoom(ConnectProxy* conn, const com_protocol::DetailInfo& detailInfo)
{
	GameUserData* cud = (GameUserData*)getProxyUserData(conn);
	const string& userName = detailInfo.static_info().username();
	
	// 关闭和之前游戏服务的连接，该用户所在的之前的游戏服务必须退出
	int rc = SrvOptFailed;
	GameServiceData gameSrvData;
	const int gameLen = m_redisDbOpt.getHField(GameUserKey, GameUserKeyLen, userName.c_str(), userName.length(), (char*)&gameSrvData, sizeof(gameSrvData));
	if (gameLen > 0)
	{
		// 关闭和之前游戏服务的连接
		if (gameSrvData.srvId != getSrvId())
		{
			rc = sendMessage(userName.c_str(), userName.length(), gameSrvData.srvId, SERVICE_CLOSE_REPEATE_CONNECT_NOTIFY);
			
			OptWarnLog("enter room check user reply and close game service repeate connect, user = %s, service id = %u, rc = %d",
		               userName.c_str(), gameSrvData.srvId, rc);
		}
		else
		{
			// 存在重复进入房间就直接关闭之前的连接
			m_srvOpt.closeRepeatConnectProxy(userName.c_str(), userName.length(), getSrvId(), 0, 0);
		}
	}

	com_protocol::ClientEnterRoomRsp enterRoomRsp;
	SRoomData* roomData = NULL;
	do
	{
		roomData = getRoomDataById(cud->roomId, cud->hallId);
		if (roomData == NULL)
		{
			OptErrorLog("get room data error, hallId = %s, roomId = %s", cud->hallId, cud->roomId);
	
			enterRoomRsp.set_result(GameObjectNotFoundRoomData);
			break;
		}
		
		// 默认坐下
		if (cud->seatId >= -1)
		{
			if (roomData->isGoldRoom())
			{
				// 如果是默认坐下，需要判断金币是否足够，只有房间的创建者才可以默认坐下，则金币必须足够
				if (detailInfo.dynamic_info().game_gold() < roomData->getEnterNeedGold())
				{
					enterRoomRsp.set_need_gold(roomData->getEnterNeedGold());
					enterRoomRsp.set_result(GameObjectGameGoldInsufficient);
					break;
				}
			}
			else if (!checkRoomCardIsEnough(*roomData, userName.c_str(), detailInfo.dynamic_info().room_card()))
			{
				enterRoomRsp.set_result(GameObjectRoomCardInsufficient);
				break;
			}
		}
		
		// 设置用户会话数据
        rc = setUserSessionData(userName.c_str(), userName.length(), EGamePlayerStatus::EInRoom);
		if (rc != SrvOptSuccess)
		{
			enterRoomRsp.set_result(GameObjectSetUserSessionDataFailed);
			break;
		}
		
		// 成功则替换连接代理关联信息，变更连接ID为用户名
		m_srvOpt.checkConnectProxySuccess(userName, conn);

		// 玩家加入房间
		addUserToRoom(cud, roomData, detailInfo);
		
		m_serviceInfo.set_current_persons(m_serviceInfo.current_persons() + 1);

		if (cud->seatId >= -1)  // 默认坐下
		{
		    rc = userSitDown(cud, roomData);
			if (rc != SrvOptSuccess)
			{
			    enterRoomRsp.set_result(rc);
			    break;
			}
		}
		
		enterRoomRsp.set_result(SrvOptSuccess);
		enterRoomRsp.set_allocated_room_info(&roomData->roomInfo);

		// 设置游戏房间数据
        if (!setGameRoomData(userName, *roomData, *enterRoomRsp.mutable_user_info(), *enterRoomRsp.mutable_game_info()))
		{
			enterRoomRsp.clear_game_info();
		}

		if (roomData->playerInfo.size() > 1)
		{
			// 同步发送消息到房间的其他玩家
			com_protocol::ClientEnterRoomNotify enterRoomNotify;
			enterRoomNotify.set_allocated_user_info(&roomData->playerInfo[userName]);
	        sendPkgMsgToRoomPlayers(*roomData, enterRoomNotify, COMM_PLAYER_ENTER_ROOM_NOTIFY, "player enter room notify", userName);

			enterRoomNotify.release_user_info();
		}

	} while (false);

	rc = m_srvOpt.sendMsgPkgToProxy(enterRoomRsp, COMM_PLAYER_ENTER_ROOM_RSP, conn, "enter room reply");
	
	OptInfoLog("send enter room result to client, user name = %s, result = %d, rc = %d, online persons = %u, connect id = %u, flag = %d, ip = %s, port = %d",
	userName.c_str(), enterRoomRsp.result(), rc, m_serviceInfo.current_persons(), conn->proxyId, conn->proxyFlag, CSocket::toIPStr(conn->peerIp), conn->peerPort);

    if (enterRoomRsp.result() == SrvOptSuccess)
	{
		enterRoomRsp.release_room_info();
		
		// 玩家上线通知
		const int seatId = (cud->seatId >= 0) ? cud->seatId : -1;
		m_srvOpt.sendUserOnlineNotify(cud->userName, getSrvId(), CSocket::toIPStr(conn->peerIp),
								      cud->hallId, cud->roomId, roomData->roomInfo.base_info().base_rate(), seatId);

		// 最后才回调上线通知
	    onLine(cud, conn, detailInfo);
	}
	else
	{
		// 失败则关闭连接
	    // removeConnectProxy(cud->userName);
	}
}

void CGameMsgHandler::userReEnterRoom(const string& userName, ConnectProxy* conn, GameUserData* cud, SRoomData& roomData,
									  const com_protocol::ClientRoomPlayerInfo& userInfo)
{
	// 成功则替换连接代理关联信息，变更连接ID为用户名
	m_srvOpt.checkConnectProxySuccess(userName, conn);
	roomData.onReEnter(userName);

	m_serviceInfo.set_current_persons(m_serviceInfo.current_persons() + 1);
	
	com_protocol::ClientReEnterRoomRsp reEnterRoomRsp;
	reEnterRoomRsp.set_result(SrvOptSuccess);
	reEnterRoomRsp.set_allocated_room_info(&roomData.roomInfo);

	// 设置游戏房间数据
	if (!setGameRoomData(userName, roomData, *reEnterRoomRsp.mutable_user_info(), *reEnterRoomRsp.mutable_game_info()))
	{
		reEnterRoomRsp.clear_game_info();
	}
	
	// 取消解散房间，如果存在该操作的话
	refuseDismissRoomContinueGame(cud, roomData, true);
	
	if (roomData.playerInfo.size() > 1)
	{
		// 玩家掉线后重连上线，同步发送消息到房间的其他玩家
        updatePlayerStatusNotify(roomData, userName, userInfo.status(), cud->userName, "player re enter notify");
    }

	int rc = m_srvOpt.sendMsgPkgToProxy(reEnterRoomRsp, COMM_PLAYER_REENTER_ROOM_RSP, conn, "re enter room reply");
	
	reEnterRoomRsp.release_room_info();
	
	m_srvOpt.sendUserOnlineNotify(cud->userName, getSrvId(), CSocket::toIPStr(conn->peerIp),
								  cud->hallId, cud->roomId, roomData.roomInfo.base_info().base_rate(), cud->seatId);

    // 最后才回调上线通知
	onLine(cud, conn, userInfo.detail_info());

	OptInfoLog("send re enter room result to client, hall id = %s, room id = %s, user name = %s, result = %d, rc = %d, online persons = %u, connect id = %u, flag = %d, ip = %s, port = %d",
	cud->hallId, cud->roomId, userName.c_str(), reEnterRoomRsp.result(), rc, m_serviceInfo.current_persons(), conn->proxyId, conn->proxyFlag, CSocket::toIPStr(conn->peerIp), conn->peerPort);
}

EGamePlayerStatus CGameMsgHandler::userLeaveRoom(GameUserData* cud, int quitStatus)
{
	SRoomData* roomData = getRoomDataById(cud->roomId, cud->hallId);
	if (roomData == NULL) return EGamePlayerStatus::ERoomDisband; // 找不到房间即已经解散

	// 服务停止
	if (com_protocol::EUserQuitStatus::EServiceStop == quitStatus)
	{
		delRoomData(cud->hallId, cud->roomId);  // 服务停止则解散删除房间
		
		return EGamePlayerStatus::ERoomDisband;
	}

	const EGamePlayerStatus playerStatus = onLeaveRoom(*roomData, cud->userName);
	if (playerStatus == EGamePlayerStatus::ERoomDisband)
	{
		delRoomData(cud->hallId, cud->roomId);  // 解散删除房间
	}
	else if (playerStatus == EGamePlayerStatus::ELeaveRoom)
	{
		// 玩家离开房间，同步发送消息到房间的其他玩家
		com_protocol::ClientLeaveRoomNotify lvRoomNotify;
		lvRoomNotify.set_username(cud->userName);
		sendPkgMsgToRoomPlayers(*roomData, lvRoomNotify, COMM_PLAYER_LEAVE_ROOM_NOTIFY, "player leave room notify", cud->userName);
	}
	else if (playerStatus == EGamePlayerStatus::EDisconnectRoom)
	{
		// 玩家掉线，同步玩家状态，发送消息到房间的其他玩家
		updatePlayerStatusNotify(*roomData, cud->userName, com_protocol::ERoomPlayerStatus::EPlayerOffline, cud->userName, "player disconnect notify");
	}
	
	// 玩家离开了，是否还存在新庄家
	if (playerStatus != EGamePlayerStatus::ERoomDisband && !roomData->hasNextNewBanker())
	{
		/*
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
		*/

		// 通知客户端没有庄家了（客户端准备按钮设置为灰色）
		// 玩家A，B，C，如果A离开了找不到新庄家，则发消息给B，C
		// 接着D玩家（不满足当庄条件）进入房间然后B玩家又离开了，此时要发消息给C，D玩家
		// 因此只要不存在庄家了则每次都发送该消息
		roomData->currentHasBanker = false;
		updatePrepareStatusNotify(*roomData, com_protocol::ETrueFalseType::EFalseType, "player leave no banker");
	}

	return playerStatus;
}

void CGameMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!m_srvOpt.initEx(this, CCfg::getValue("Service", "ConfigFile")))
	{
		OptErrorLog("init service operation instance error");
		stopService();
		return;
	}

    m_srvOpt.setCloseRepeateNotifyProtocol(SERVICE_CLOSE_REPEATE_CONNECT_NOTIFY, COMM_CLOSE_REPEATE_CONNECT_NOTIFY);

	m_srvOpt.cleanUpConnectProxy(m_srvMsgCommCfg);  // 清理连接代理，如果服务异常退出，则启动时需要先清理连接代理

	// 创建数据日志配置
	if (!m_srvOpt.createDataLogger("DataLogger", "File", "Size", "BackupCount"))
	{
		OptErrorLog("create data log config error");
		stopService();
		return;
	}

	const DBConfig::config& dbCfg = m_srvOpt.getDBCfg();
	if (!dbCfg.isSetConfigValueSuccess())
	{
		OptErrorLog("get db xml config value error");
		stopService();
		return;
	}

	if (!m_redisDbOpt.connectSvr(dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port,
	    dbCfg.redis_db_cfg.center_db_timeout * MillisecondUnit))
	{
		OptErrorLog("game service connect center redis service failed, ip = %s, port = %u, time out = %u",
		dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port, dbCfg.redis_db_cfg.center_db_timeout);
		
		stopService();
		return;
	}
	
    if (m_gameLogicHandler.init(this) != SrvOptSuccess)
	{
		OptErrorLog("init game logic handler instance error");
		
		stopService();
		return;
	}

	// 定时保存服务存活数据到redis
	const char* secsTimeInterval = CCfg::getValue("Service", "SaveDataToDBInterval");
	const unsigned int saveIntervalSecs = (secsTimeInterval != NULL) ? atoi(secsTimeInterval) : 10;
	setTimer(saveIntervalSecs * MillisecondUnit, (TimerHandler)&CGameMsgHandler::saveDataToRedis, 0, NULL, (unsigned int)-1);

	// 启动心跳检测
	const ServiceCommonConfig::HeartbeatCfg& heartbeatCfg = m_srvOpt.getServiceCommonCfg().heart_beat_cfg;
	startHeartbeatCheck(heartbeatCfg.heart_beat_interval, heartbeatCfg.heart_beat_count);
	
	// 启动定时检测待解散的空房间，超时时间到则解散房间
	startCheckDisbandRoom();

    // 输出配置值
	m_srvOpt.getServiceCommonCfg().output();
	m_srvOpt.getDBCfg().output();
	
	onInit();
	
	// 保存服务存活数据到redis
	m_serviceInfo.set_id(getSrvId());
	m_serviceInfo.set_type(getSrvType());
	m_serviceInfo.set_name(getSrvName());
	m_serviceInfo.set_flag(getSrvFlag());
	m_serviceInfo.set_current_persons(0);
	saveDataToRedis(0, 0, NULL, 0);
}

void CGameMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	onUnInit();

	m_gameLogicHandler.unInit();
	
	// 服务下线通知
	m_srvOpt.sendServiceStatusNotify(srvId, com_protocol::EServiceStatus::ESrvOffline, getSrvType(), srvName);

	// 服务停止，玩家退出服务
	com_protocol::ClientLogoutNotify srvQuitNtf;
	srvQuitNtf.set_info("game service quit");
	const char* quitNtfMsgData = m_srvOpt.serializeMsgToBuffer(srvQuitNtf, "game service quit notify");

	// 服务关闭，所有在线用户退出登陆
	const IDToConnectProxys& userConnects = getConnectProxy();
	for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end();)
	{
		if (quitNtfMsgData != NULL) sendMsgToProxy(quitNtfMsgData, srvQuitNtf.ByteSize(), COMM_FORCE_PLAYER_QUIT_NOTIFY, it->second);

		// 这里不可以直接++it遍历调用removeConnectProxy，如此会不断修改userConnects的值而导致本循环遍历it值错误
		removeConnectProxy(it->first, com_protocol::EUserQuitStatus::EServiceStop);
		it = userConnects.begin();
	}

	disbandAllWaitForRoom();    // 解散所有等待的空房间

	stopConnectProxy();  // 停止连接代理
	
	// 删除服务存活数据
	int rc = m_redisDbOpt.delHField(GameServiceListKey, GameServiceListLen, (const char*)&srvId, sizeof(srvId));
	if (rc < 0) OptErrorLog("delete game service data from redis center service failed, rc = %d", rc);
	
	m_redisDbOpt.disconnectSvr();
}

void CGameMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("register protocol handler, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
	
	// 注册协议处理函数
	registerProtocol(getSrvType(), SERVICE_COMMON_GET_USER_INFO_FOR_ENTER_ROOM, (ProtocolHandler)&CGameMsgHandler::getUserInfoForEnterReply);
	registerProtocol(getSrvType(), SERVICE_COMMON_GET_ROOM_INFO_FOR_ENTER_ROOM, (ProtocolHandler)&CGameMsgHandler::getRoomInfoForEnterReply);
	
	// 收到的客户端请求消息
	registerProtocol(OutsideClientSrv, COMM_PLAYER_ENTER_ROOM_REQ, (ProtocolHandler)&CGameMsgHandler::enter);
	// registerProtocol(OutsideClientSrv, COMM_PLAYER_SIT_DOWN_REQ, (ProtocolHandler)&CGameMsgHandler::sitDown);
	registerProtocol(OutsideClientSrv, COMM_PLAYER_PREPARE_GAME_REQ, (ProtocolHandler)&CGameMsgHandler::prepare);
	registerProtocol(OutsideClientSrv, COMM_PLAYER_START_GAME_REQ, (ProtocolHandler)&CGameMsgHandler::start);
	registerProtocol(OutsideClientSrv, COMM_PLAYER_LEAVE_ROOM_REQ, (ProtocolHandler)&CGameMsgHandler::leave);
	registerProtocol(OutsideClientSrv, COMM_PLAYER_CHANGE_ROOM_REQ, (ProtocolHandler)&CGameMsgHandler::change);
	registerProtocol(OutsideClientSrv, COMM_PLAYER_REENTER_ROOM_REQ, (ProtocolHandler)&CGameMsgHandler::reEnter);
	
	registerProtocol(OutsideClientSrv, COMM_SERVICE_HEART_BEAT_NOTIFY, (ProtocolHandler)&CGameMsgHandler::heartbeatMessage);

	registerProtocol(DBProxySrv, DBPROXY_CHANGE_MORE_USERS_ITEM_RSP, (ProtocolHandler)&CGameMsgHandler::changeMoreUsersGoodsReply);
	registerProtocol(DBProxySrv, DBPROXY_SET_ROOM_GAME_RECORD_RSP, (ProtocolHandler)&CGameMsgHandler::setRoomGameRecordReply);
}

void CGameMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 服务上线通知
	int rc = m_srvOpt.sendServiceStatusNotify(srvId, com_protocol::EServiceStatus::ESrvOnline, getSrvType(), srvName);
	ReleaseInfoLog("run cattles game message handler service name = %s, id = %d, module = %d, rc = %d", srvName, srvId, moduleId, rc);
}

bool CGameMsgHandler::onPreHandleMessage(const char* msgData, const unsigned int msgLen, const ConnectProxyContext& connProxyContext)
{
	if (connProxyContext.protocolId == COMM_PLAYER_ENTER_ROOM_REQ || connProxyContext.protocolId == COMM_PLAYER_REENTER_ROOM_REQ)
	{
		return !m_srvOpt.isRepeateCheckConnectProxy(COMM_FORCE_PLAYER_QUIT_NOTIFY);  // 检查是否存在重复登陆
	}

	// 检查发送消息的连接是否已成功通过验证，防止玩家验证成功之前胡乱发消息
    if (m_srvOpt.checkConnectProxyIsSuccess(COMM_FORCE_PLAYER_QUIT_NOTIFY))
	{
		heartbeatCheck(((const GameUserData*)getProxyUserData(connProxyContext.conn))->userName);

		return true;
	}
	
	OptErrorLog("game service receive not check success message, protocolId = %d", connProxyContext.protocolId);
	
	return false;
}

// 是否可以执行该操作
const char* CGameMsgHandler::canToDoOperation(const int opt)
{
	if (opt == EServiceOperationType::EClientRequest && m_srvOpt.checkConnectProxyIsSuccess(COMM_FORCE_PLAYER_QUIT_NOTIFY))
	{
		GameUserData* ud = (GameUserData*)getProxyUserData(getConnectProxyContext().conn);
		return ud->userName;
	}
	
	return NULL;
}

void CGameMsgHandler::enter(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 先初始化连接
	GameUserData* curCud = (GameUserData*)getProxyUserData(getConnectProxyContext().conn);
	if (curCud == NULL) curCud = (GameUserData*)m_srvOpt.createConnectProxyUserData(getConnectProxyContext().conn);
	
	com_protocol::ClientEnterRoomReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "user enter room")) return;
	
	com_protocol::ClientEnterRoomRsp rsp;
	int rc = SrvOptFailed;
	do
	{
		// 先验证会话数据是否正确
		SessionKeyData sessionKeyData;
        rsp.set_result(checkSessionKey(req.username(), req.session_key(), sessionKeyData));
		if (rsp.result() != SrvOptSuccess) break;
        
        if (sessionKeyData.hallId != (unsigned int)atoi(req.hall_id().c_str()))
        {
            rsp.set_result(GameObjectInvalidChessHallId);
            break;
        }

        // 保存登陆数据
		strncpy(curCud->hallId, req.hall_id().c_str(), IdMaxLen - 1);
		strncpy(curCud->roomId, req.room_id().c_str(), IdMaxLen - 1);
		
		// 座位号大于等于 0 表示玩家自行选座
		// 座位号为 -1 表示由服务器分配座位号
		// 所有小于 -1 的值表示玩家仅仅是进入房间
		curCud->seatId = req.has_seat_id() ? req.seat_id() : -2;
		
		// 玩家在棋牌室的状态
		curCud->inHallStatus = sessionKeyData.status;

        unsigned int dbProxySrvId = 0;
		NProject::getDestServiceID(ServiceType::DBProxySrv, req.username().c_str(), req.username().length(), dbProxySrvId);

		SRoomData* clsRoomData = getRoomDataById(req.room_id().c_str(), req.hall_id().c_str());
		if (clsRoomData != NULL)
		{
			const com_protocol::ChessHallRoomBaseInfo& rmBaseInfo = clsRoomData->roomInfo.base_info();
			if ((rmBaseInfo.room_type() == com_protocol::EHallRoomType::EGoldId
			     || rmBaseInfo.room_type() == com_protocol::EHallRoomType::ECardId)  // 私人房间
			    && req.is_be_invited() != 1 && req.username() != rmBaseInfo.username())
			{
				// 私人房间除了房主，其他玩家不可以直接进入，需要被邀请才能进入
				rsp.set_result(GameObjectPrivateRoomCanNotBeEnter);
				break;
			}
			
			if (rmBaseInfo.is_can_view() != 1  // 该房间不可以旁观
			    && (!isHallPlayer(curCud)                             // 是否是棋牌室审核通过的正式玩家
				    || clsRoomData->getRoomPlayerCount() >= rmBaseInfo.player_count())  // 房间满人了
			   )
			{
				rsp.set_result(GameObjectRoomCanNotBeView);
				break;
			}

			// 已经存在房间数据，则获取用户信息即可
			com_protocol::GetUserDataReq getUserDataReq;
			getUserDataReq.set_username(req.username());
			getUserDataReq.set_info_type(com_protocol::EUserInfoType::EUserStaticDynamic);
			getUserDataReq.set_hall_id(req.hall_id());

			rc = m_srvOpt.sendPkgMsgToService(curCud->userName, curCud->userNameLen, getUserDataReq, dbProxySrvId,
											  DBPROXY_GET_USER_DATA_REQ, "enter room get user data request", 0,
											  SERVICE_COMMON_GET_USER_INFO_FOR_ENTER_ROOM);
		}
		else
		{
			// 没有房间数据，则第一次进入房间，需要获取房间数据
			com_protocol::GetHallGameRoomInfoReq getRoomDataReq;
			getRoomDataReq.set_room_id(req.room_id());
			getRoomDataReq.set_username(req.username());  // 同时获取该用户的信息
			getRoomDataReq.set_hall_id(req.hall_id());
			
			rc = m_srvOpt.sendPkgMsgToService(curCud->userName, curCud->userNameLen, getRoomDataReq, dbProxySrvId,
											  DBPROXY_GET_GAME_ROOM_INFO_REQ, "enter room get room data request",
                                              req.is_be_invited(), SERVICE_COMMON_GET_ROOM_INFO_FOR_ENTER_ROOM);
		}

		OptInfoLog("receive message to enter, ip = %s, port = %d, username = %s, hall id = %s, room id = %s, is invited = %d, room data = %p, rc = %d",
		CSocket::toIPStr(getConnectProxyContext().conn->peerIp), getConnectProxyContext().conn->peerPort, req.username().c_str(),
		req.hall_id().c_str(), req.room_id().c_str(), req.is_be_invited(), clsRoomData, rc);
		
		return;
		
	} while (false);

	rc = m_srvOpt.sendMsgPkgToProxy(rsp, COMM_PLAYER_ENTER_ROOM_RSP, "user enter room check error");

	OptErrorLog("receive message to enter but check error, ip = %s, port = %d, user name = %s, result = %d, rc = %d",
	CSocket::toIPStr(getConnectProxyContext().conn->peerIp), getConnectProxyContext().conn->peerPort,
	req.username().c_str(), rsp.result(), rc);
	
	// 验证失败可以考虑直接关闭该连接了
}

void CGameMsgHandler::getUserInfoForEnterReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(getContext().userData);
	if (conn == NULL)
	{
		OptErrorLog("enter room get user info reply can not find the connect data, user data = %s", getContext().userData);
		return;
	}
	
	com_protocol::GetUserDataRsp rsp;
	if (!m_srvOpt.parseMsgFromBuffer(rsp, data, len, "enter room get user info reply")) return;
	
	if (rsp.result() != SrvOptSuccess)
	{
		com_protocol::ClientEnterRoomRsp enterRoomRsp;
		enterRoomRsp.set_result(rsp.result());
		m_srvOpt.sendMsgPkgToProxy(enterRoomRsp, COMM_PLAYER_ENTER_ROOM_RSP, conn, "enter room get user info reply error");

        // 验证失败可以考虑直接关闭该连接了
		return;
	}
	
	userEnterRoom(conn, rsp.detail_info());
}

void CGameMsgHandler::getRoomInfoForEnterReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(getContext().userData);
	if (conn == NULL)
	{
		OptErrorLog("enter room get room info reply can not find the connect data, user data = %s", getContext().userData);
		return;
	}
	
	com_protocol::GetHallGameRoomInfoRsp rsp;
	if (!m_srvOpt.parseMsgFromBuffer(rsp, data, len, "enter room get room info reply")) return;
	
	com_protocol::ClientEnterRoomRsp enterRoomRsp;
	do
	{
		if (rsp.result() != SrvOptSuccess)
		{
			enterRoomRsp.set_result(rsp.result());
			break;
		}
		
		const com_protocol::ChessHallRoomBaseInfo& rmBaseInfo = rsp.room_info().base_info();
		if ((rmBaseInfo.room_type() == com_protocol::EHallRoomType::EGoldId
		     || rmBaseInfo.room_type() == com_protocol::EHallRoomType::ECardId)  // 私人房间
			&& getContext().userFlag != 1
			&& rsp.user_detail_info().static_info().username() != rmBaseInfo.username())
		{
			// 私人房间除了房主，其他玩家不可以直接进入，需要被邀请才能进入
			enterRoomRsp.set_result(GameObjectPrivateRoomCanNotBeEnter);
			break;
		}
		
		// 新增棋牌室房间信息
		int rc = addRoomData(rsp.room_info());
		if (rc != SrvOptSuccess)
		{
			enterRoomRsp.set_result(rc);
			break;
		}
		
		userEnterRoom(conn, rsp.user_detail_info());
		
		return;

	} while (false);
	
	// 验证失败可以考虑直接关闭该连接了
	m_srvOpt.sendMsgPkgToProxy(enterRoomRsp, COMM_PLAYER_ENTER_ROOM_RSP, conn, "enter room get room info reply error");
}

/*
void CGameMsgHandler::sitDown(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientPlayerSitDownReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "sit down request")) return;

	com_protocol::ClientPlayerSitDownRsp rsp;	
	GameUserData* cud = (GameUserData*)m_srvOpt.getConnectProxyUserData();
	SCattlesRoomData* roomData = NULL;
	do
	{
	    roomData = getRoomDataById(cud->roomId, cud->hallId);
		if (roomData == NULL)
		{
			rsp.set_result(GameObjectNotFoundRoomData);
			break;
		}
		
		if (roomData->gameStatus != com_protocol::ECattlesGameStatus::ECattlesGameReady)
		{
			rsp.set_result(GameObjectGameStatusInvalidOpt);
			break;
		}

		if (req.has_seat_id()) cud->seatId = req.seat_id();
		rsp.set_result(userSitDown(cud, roomData));
		if (rsp.result() != SrvOptSuccess) break;
		
		rsp.set_seat_id(cud->seatId);

		// 同步发送消息到房间的其他玩家
		com_protocol::ClientPlayerSitDownNotify stNtf;
		stNtf.set_username(cud->userName, cud->userNameLen);
		stNtf.set_seat_id(cud->seatId);
		sendPkgMsgToRoomPlayers(*roomData, stNtf, COMM_PLAYER_SIT_DOWN_NOTIFY, "player sit down notify", cud->userName);
		
		// 是否存在新庄家
		if (!roomData->currentHasBanker && hasNextNewBanker(*roomData))
		{
			// 通知客户端有庄家了（客户端准备按钮设置可点击）
			roomData->currentHasBanker = true;
			updatePrepareStatusNotify(*roomData, com_protocol::ETrueFalseType::ETrueType, "player sit down has banker");
		}

	} while (false);
	
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, COMM_PLAYER_SIT_DOWN_RSP, "player sit down reply");
	OptInfoLog("player sit down reply, username = %s, hallId = %s, roomId = %s, seatId = %d, result = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rsp.result(), rc);
	
	if (roomData != NULL && !roomData->currentHasBanker)
	{
		// 通知该玩家没有庄家（客户端准备按钮设置为灰色）
		com_protocol::CattlesPrepareStatusNotify prepareStatusNtf;
		prepareStatusNtf.set_prepare_status(com_protocol::ETrueFalseType::EFalseType);
		m_srvOpt.sendMsgPkgToProxy(prepareStatusNtf, CATTLES_PREPARE_STATUS_NOTIFY, "current no banker notify");
	}
}
*/

void CGameMsgHandler::prepare(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientPlayerPrepareReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "player prepare request")) return;

	com_protocol::ClientPlayerPrepareRsp rsp;
	GameUserData* cud = (GameUserData*)m_srvOpt.getConnectProxyUserData();
	SRoomData* roomData = NULL;
	do
	{
	    roomData = getRoomDataById(cud->roomId, cud->hallId);
		if (roomData == NULL)
		{
			rsp.set_result(GameObjectNotFoundRoomData);
			break;
		}
		
		if (roomData->gameStatus != com_protocol::EGameStatus::EGameReady)
		{
			rsp.set_result(GameObjectGameStatusInvalidOpt);
			break;
		}
		
		// 是否存在庄家
		if (!roomData->currentHasBanker)
		{
			rsp.set_result(GameObjectCurrentNoBanker);
			break;
		}
		
		RoomPlayerIdInfoMap::iterator userInfoIt = roomData->playerInfo.find(cud->userName);
		if (userInfoIt == roomData->playerInfo.end())
		{
			rsp.set_result(GameObjectNotRoomUserInfo);
			break;
		}
		
		// 坐下占住位置
		if (req.has_seat_id()) cud->seatId = req.seat_id();
		rsp.set_result(userSitDown(cud, roomData));
		if (rsp.result() != SrvOptSuccess) break;
		
		rsp.set_seat_id(cud->seatId);
		userInfoIt->second.set_status(com_protocol::ERoomPlayerStatus::EPlayerReady);  // 玩家状态为准备

		// 同步发送消息到房间的其他玩家
		com_protocol::ClientPlayerPrepareNotify preNtf;
		preNtf.set_username(cud->userName, cud->userNameLen);
		preNtf.set_seat_id(cud->seatId);
		sendPkgMsgToRoomPlayers(*roomData, preNtf, COMM_PLAYER_PREPARE_GAME_NOTIFY, "player prepare notify", cud->userName);

	} while (false);
	
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, COMM_PLAYER_PREPARE_GAME_RSP, "player prepare game");
	OptInfoLog("player prepare game reply, username = %s, hallId = %s, roomId = %s, seatId = %d, result = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rsp.result(), rc);

	if (rsp.result() == SrvOptSuccess)
	{
		// 是否可以手动开始游戏
		if (roomData->playTimes < 1 && canStartGame(*roomData, false) == SrvOptSuccess)
		{
			const char* ntfUsername = NULL;
			const string& roomOwner = roomData->roomInfo.base_info().username();
			RoomPlayerIdInfoMap::const_iterator ownerIt = roomData->playerInfo.find(roomOwner);
			if (ownerIt != roomData->playerInfo.end())
			{
				if (ownerIt->second.status() == com_protocol::ERoomPlayerStatus::EPlayerReady) ntfUsername = roomOwner.c_str();
			}
			else
			{
				ntfUsername = cud->userName;
			}

			if (ntfUsername != NULL) m_srvOpt.sendMsgToProxyEx(NULL, 0, COMM_MANUAL_START_GAME_NOTIFY, ntfUsername);
		}
	
	    // 是否可以自动开始游戏
		if (canStartGame(*roomData) == SrvOptSuccess) onStartGame(cud->hallId, cud->roomId, *roomData); // 开始游戏
	}
}

// 手动开始游戏
void CGameMsgHandler::start(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientPlayerStartGameRsp rsp;
	GameUserData* cud = (GameUserData*)getProxyUserData(getConnectProxyContext().conn);
	SRoomData* roomData = NULL;
	do
	{
	    roomData = getRoomDataById(cud->roomId, cud->hallId);
		if (roomData == NULL)
		{
			rsp.set_result(GameObjectNotFoundRoomData);
			break;
		}
		
		if (roomData->gameStatus != com_protocol::EGameStatus::EGameReady)
		{
			rsp.set_result(GameObjectGameStatusInvalidOpt);
			break;
		}
		
		// 房主才能开始游戏
		if (roomData->roomInfo.base_info().username() != cud->userName)
		{
			rsp.set_result(GameObjectNotRoomCreator);
			break;
		}
		
		RoomPlayerIdInfoMap::iterator userInfoIt = roomData->playerInfo.find(cud->userName);
		if (userInfoIt == roomData->playerInfo.end())
		{
			rsp.set_result(GameObjectNotRoomUserInfo);
			break;
		}
		
		if (userInfoIt->second.status() != com_protocol::ERoomPlayerStatus::EPlayerReady)
		{
			rsp.set_result(GameObjectPlayerStatusInvalidOpt);
			break;
		}

		if (!checkRoomCardIsEnough(*roomData, cud->userName, userInfoIt->second.detail_info().dynamic_info().room_card()))
		{
			rsp.set_result(GameObjectRoomCardInsufficient);
			break;
		}
		
		rsp.set_result(canStartGame(*roomData, false));

	} while (false);
	
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, COMM_PLAYER_START_GAME_RSP, "player start game");
	OptInfoLog("player start game reply, username = %s, hallId = %s, roomId = %s, seatId = %d, result = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rsp.result(), rc);
	
	if (rsp.result() == SrvOptSuccess) onStartGame(cud->hallId, cud->roomId, *roomData); // 开始游戏
}

void CGameMsgHandler::leave(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	GameUserData* cud = (GameUserData*)getProxyUserData(getConnectProxyContext().conn);

	com_protocol::ClientLeaveRoomRsp rsp;
	canLeaveRoom(cud) ? rsp.set_result(SrvOptSuccess) : rsp.set_result(GameObjectOnPlayCanNotLeaveRoom);
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, COMM_PLAYER_LEAVE_ROOM_RSP, "player leave room");

	OptInfoLog("player leave room, username = %s, hallId = %s, roomId = %s, seatId = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rc);
	
	if (rsp.result() == SrvOptSuccess) removeConnectProxy(string(cud->userName), com_protocol::EUserQuitStatus::EUserOffline);  // 删除与客户端的连接代理，不会关闭连接
}

void CGameMsgHandler::change(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	GameUserData* cud = (GameUserData*)getProxyUserData(getConnectProxyContext().conn);
	
	com_protocol::ClientChangeRoomRsp rsp;
	canLeaveRoom(cud) ? rsp.set_result(SrvOptSuccess) : rsp.set_result(GameObjectOnPlayCanNotChangeRoom);
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, COMM_PLAYER_CHANGE_ROOM_RSP, "player change room");
	
	OptInfoLog("player change room, username = %s, hallId = %s, roomId = %s, seatId = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rc);
	
	if (rsp.result() == SrvOptSuccess) removeConnectProxy(string(cud->userName), com_protocol::EUserQuitStatus::EUserChangeRoom);
}

// 掉线重连，重入房间
void CGameMsgHandler::reEnter(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 先初始化连接
	GameUserData* curCud = (GameUserData*)getProxyUserData(getConnectProxyContext().conn);
	if (curCud == NULL) curCud = (GameUserData*)m_srvOpt.createConnectProxyUserData(getConnectProxyContext().conn);
	
	com_protocol::ClientReEnterRoomReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "user re enter room")) return;
	
	com_protocol::ClientReEnterRoomRsp rsp;
	do
	{
		// 先验证会话数据是否正确
		SessionKeyData sessionKeyData;
        rsp.set_result(checkSessionKey(req.username(), req.session_key(), sessionKeyData));
		if (rsp.result() != SrvOptSuccess) break;
        
        if (sessionKeyData.hallId != (unsigned int)atoi(req.hall_id().c_str()))
        {
            rsp.set_result(GameObjectInvalidChessHallId);
            break;
        }
		
		SRoomData* roomData = getRoomDataById(req.room_id().c_str(), req.hall_id().c_str());
		if (roomData == NULL)
		{
			rsp.set_result(GameObjectNotFoundRoomData);
			break;
		}

		if (!roomData->isOnPlay())
		{
			rsp.set_result(GameObjectGameAlreadyFinish);  // 当局游戏已经结束
			break;
		}
		
		// 验证玩家数据
		RoomPlayerIdInfoMap::const_iterator userInfoIt = roomData->playerInfo.find(req.username());
	    if (userInfoIt == roomData->playerInfo.end())
		{
			rsp.set_result(GameObjectNotRoomUserInfo);
			break;
		}
		
		if (userInfoIt->second.seat_id() < 0)
		{
			rsp.set_result(GameObjectPlayerNoSeat);
			break;
		}
		
		// 保存掉线重连登陆信息
		strncpy(curCud->hallId, req.hall_id().c_str(), IdMaxLen - 1);
	    strncpy(curCud->roomId, req.room_id().c_str(), IdMaxLen - 1);
		curCud->seatId = userInfoIt->second.seat_id();

		// 玩家在棋牌室的状态
		curCud->inHallStatus = sessionKeyData.status;

		userReEnterRoom(req.username(), getConnectProxyContext().conn, curCud, *roomData, userInfoIt->second);
		
		return;
		
	} while (false);

	// 验证失败则删除用户会话数据，不可以再通过掉线重连进入房间
	delUserSessionData(req.username().c_str(), req.username().length());

	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, COMM_PLAYER_REENTER_ROOM_RSP, "user re enter room check error");

	OptErrorLog("receive message to re enter but check error, ip = %s, port = %d, user name = %s, result = %d, rc = %d",
	CSocket::toIPStr(getConnectProxyContext().conn->peerIp), getConnectProxyContext().conn->peerPort,
	req.username().c_str(), rsp.result(), rc);
	
	// 验证失败可以考虑直接关闭该连接了
}

// 心跳应答消息
void CGameMsgHandler::heartbeatMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// now do nothing
}

void CGameMsgHandler::changeMoreUsersGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::MoreUsersItemChangeRsp rsp;
	if (!m_srvOpt.parseMsgFromBuffer(rsp, data, len, "change more users goods reply")) return;

	if (rsp.failed_count() > 0)
	{
		for (google::protobuf::RepeatedPtrField<com_protocol::ItemChangeRsp>::const_iterator it = rsp.item_change_rsp().begin();
		     it != rsp.item_change_rsp().end(); ++it)
	    {
		    OptErrorLog("change more users goods error, username = %s, result = %d", it->src_username().c_str(), it->result());
	    }
	}
	
	OptInfoLog("change more users goods reply, flag = %d, failed count = %d",
	getContext().userFlag, rsp.failed_count());
}

void CGameMsgHandler::setRoomGameRecordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetRoomGameRecordRsp rsp;
	if (!m_srvOpt.parseMsgFromBuffer(rsp, data, len, "set room game record reply")) return;

	if (rsp.result() != SrvOptSuccess) OptErrorLog("set room game record reply error = %d", rsp.result());
}

}

