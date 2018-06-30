
/* author : limingfan
 * date : 2017.11.15
 * description : 牛牛游戏服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "CCattles.h"
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


CCattlesMsgHandler::CCattlesMsgHandler() : m_srvOpt(sizeof(GameUserData))
{
	m_pCfg = NULL;
	
	m_serviceHeartbeatTimerId = 0;
	m_checkHeartbeatCount = 0;
}

CCattlesMsgHandler::~CCattlesMsgHandler()
{
	m_pCfg = NULL;
	
	m_serviceHeartbeatTimerId = 0;
	m_checkHeartbeatCount = 0;
}


void CCattlesMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("register protocol handler, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
	
	// 注册协议处理函数
	registerProtocol(CommonSrv, SERVICE_MANAGER_GIVE_GOODS_NOTIFY, (ProtocolHandler)&CCattlesMsgHandler::giveGoodsNotify);
	
	registerProtocol(CattlesGame, CATTLES_GET_USER_INFO_FOR_ENTER_ROOM, (ProtocolHandler)&CCattlesMsgHandler::getUserInfoForEnterReply);
	registerProtocol(CattlesGame, CATTLES_GET_ROOM_INFO_FOR_ENTER_ROOM, (ProtocolHandler)&CCattlesMsgHandler::getRoomInfoForEnterReply);
	
	// 收到的客户端请求消息
	registerProtocol(OutsideClientSrv, COMM_PLAYER_ENTER_ROOM_REQ, (ProtocolHandler)&CCattlesMsgHandler::enter);
	registerProtocol(OutsideClientSrv, COMM_PLAYER_SIT_DOWN_REQ, (ProtocolHandler)&CCattlesMsgHandler::sitDown);
	registerProtocol(OutsideClientSrv, COMM_PLAYER_PREPARE_GAME_REQ, (ProtocolHandler)&CCattlesMsgHandler::prepare);
	registerProtocol(OutsideClientSrv, COMM_PLAYER_START_GAME_REQ, (ProtocolHandler)&CCattlesMsgHandler::start);
	registerProtocol(OutsideClientSrv, COMM_PLAYER_LEAVE_ROOM_REQ, (ProtocolHandler)&CCattlesMsgHandler::leave);
	registerProtocol(OutsideClientSrv, COMM_PLAYER_CHANGE_ROOM_REQ, (ProtocolHandler)&CCattlesMsgHandler::change);
	
	registerProtocol(OutsideClientSrv, COMM_PLAYER_REENTER_ROOM_REQ, (ProtocolHandler)&CCattlesMsgHandler::reEnter);
	
	registerProtocol(OutsideClientSrv, COMM_SEND_MSG_INFO_NOTIFY, (ProtocolHandler)&CCattlesMsgHandler::playerChat);
	
	registerProtocol(OutsideClientSrv, COMM_SERVICE_HEART_BEAT_NOTIFY, (ProtocolHandler)&CCattlesMsgHandler::heartbeatMessage);
}

void CCattlesMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!m_srvOpt.initEx(this, CCfg::getValue("Service", "ConfigFile")))
	{
		OptErrorLog("init service operation instance error");
		stopService();
		return;
	}

    m_srvOpt.setCloseRepeateNotifyProtocol(SERVICE_CLOSE_REPEATE_CONNECT_NOTIFY, COMM_CLOSE_REPEATE_CONNECT_NOTIFY);

	m_srvOpt.cleanUpConnectProxy(m_srvMsgCommCfg);  // 清理连接代理，如果服务异常退出，则启动时需要先清理连接代理
	
	m_pCfg = &NCattlesConfigData::CattlesConfig::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str());
	if (!m_pCfg->isSetConfigValueSuccess())
	{
		OptErrorLog("set business xml config value error");
		stopService();
		return;
	}
	
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
	
	if (m_gameProcess.init(this) != SrvOptSuccess)
	{
		OptErrorLog("init game process instance error");
		
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
	setTimer(saveIntervalSecs * MillisecondUnit, (TimerHandler)&CCattlesMsgHandler::saveDataToRedis, 0, NULL, (unsigned int)-1);
	
	// 保存服务存活数据到redis
	m_serviceInfo.set_id(getSrvId());
	m_serviceInfo.set_type(ServiceType::CattlesGame);
	m_serviceInfo.set_name("CattlesGame");
	m_serviceInfo.set_flag(m_pCfg->common_cfg.flag);
	m_serviceInfo.set_current_persons(0);
	saveDataToRedis(0, 0, NULL, 0);
	
	// 启动心跳检测
	const ServiceCommonConfig::HeartbeatCfg& heartbeatCfg = m_srvOpt.getServiceCommonCfg().heart_beat_cfg;
	startHeartbeatCheck(heartbeatCfg.heart_beat_interval, heartbeatCfg.heart_beat_count);
	
	// 启动定时检测待解散的空房间，超时时间到则解散房间
	startCheckDisbandRoom();

    // 输出配置值
	m_srvOpt.getServiceCommonCfg().output();
	m_srvOpt.getCattlesBaseCfg().output();
	m_pCfg->output();
}

void CCattlesMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	m_gameProcess.unInit();
	m_gameLogicHandler.unInit();
	
	// 服务下线通知
	m_srvOpt.sendServiceStatusNotify(srvId, com_protocol::EServiceStatus::ESrvOffline, CattlesGame, "CattlesGame");

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
		// logoutNotify(getProxyUserData(it->second), com_protocol::EUserQuitStatus::EServiceStop);
	}

	disbandAllWaitForRoom();    // 解散所有等待的空房间

	stopConnectProxy();  // 停止连接代理
	
	// 删除服务存活数据
	int rc = m_redisDbOpt.delHField(GameServiceListKey, GameServiceListLen, (const char*)&srvId, sizeof(srvId));
	if (rc < 0) OptErrorLog("delete game service data from redis center service failed, rc = %d", rc);
	
	m_redisDbOpt.disconnectSvr();
}

void CCattlesMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 服务上线通知
	int rc = m_srvOpt.sendServiceStatusNotify(srvId, com_protocol::EServiceStatus::ESrvOnline, CattlesGame, "CattlesGame");
	ReleaseInfoLog("run cattles game message handler service name = %s, id = %d, module = %d, rc = %d", srvName, srvId, moduleId, rc);
}

bool CCattlesMsgHandler::onPreHandleMessage(const char* msgData, const unsigned int msgLen, const ConnectProxyContext& connProxyContext)
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
const char* CCattlesMsgHandler::canToDoOperation(const int opt)
{
	if (opt == EServiceOperationType::EClientRequest && m_srvOpt.checkConnectProxyIsSuccess(COMM_FORCE_PLAYER_QUIT_NOTIFY))
	{
		GameUserData* ud = (GameUserData*)getProxyUserData(getConnectProxyContext().conn);
		return ud->userName;
	}
	
	return NULL;
}

// 获取房间数据
SCattlesRoomData* CCattlesMsgHandler::getRoomDataByUser(const char* userName)
{
	GameUserData* cud = (GameUserData*)m_srvOpt.getConnectProxyUserData(userName);
	if (cud == NULL) return NULL;
	
	return getRoomDataById(cud->roomId, cud->hallId);
}

SCattlesRoomData* CCattlesMsgHandler::getRoomDataById(const char* roomId, const char* hallId)
{
	if (hallId == NULL)
	{
		for (HallCattlesMap::iterator hallIt = m_hallCattlesData.begin(); hallIt != m_hallCattlesData.end(); ++hallIt)
		{
			for (CattlesRoomMap::iterator ctsRoomIt = hallIt->second.cattlesRooms.begin(); ctsRoomIt != hallIt->second.cattlesRooms.end(); ++ctsRoomIt)
			{
				if (ctsRoomIt->first == roomId) return &ctsRoomIt->second;
			}
		}
		
		return NULL;
	}
	
	HallCattlesMap::iterator hallIt = m_hallCattlesData.find(hallId);
	if (hallIt == m_hallCattlesData.end()) return NULL;
	
	CattlesRoomMap::iterator ctsRoomIt = hallIt->second.cattlesRooms.find(roomId);
	return (ctsRoomIt != hallIt->second.cattlesRooms.end()) ? &ctsRoomIt->second : NULL;
}

SCattlesRoomData* CCattlesMsgHandler::getRoomData(unsigned int hallId, unsigned int roomId)
{
	char strHallId[IdMaxLen] = {0};
	snprintf(strHallId, sizeof(strHallId) - 1, "%u", hallId);
	
	char strRoomId[IdMaxLen] = {0};
	snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", roomId);
	
	return getRoomDataById(strRoomId, strHallId);
}

// 设置用户会话数据
int CCattlesMsgHandler::setUserSessionData(const char* username, unsigned int usernameLen, int status, unsigned int roomId)
{
	GameServiceData gameSrvData;
	gameSrvData.srvId = getSrvId();
	gameSrvData.roomId = roomId;
	gameSrvData.status = status;
	gameSrvData.timeSecs = time(NULL);

	return setUserSessionData(username, usernameLen, gameSrvData);
}

int CCattlesMsgHandler::setUserSessionData(const char* username, unsigned int usernameLen, const GameServiceData& gameSrvData)
{
	int rc = m_redisDbOpt.setHField(GameUserKey, GameUserKeyLen, username, usernameLen, (char*)&gameSrvData, sizeof(GameServiceData));
	if (rc != SrvOptSuccess) OptErrorLog("set user session data to redis error, username = %s, rc = %d", username, rc);
	
	return rc;
}
	
unsigned int CCattlesMsgHandler::delUserFromRoom(const StringVector& usernames)
{
	unsigned int count = 0;
	for (StringVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it)
	{
		if (delUserFromRoom(*it)) ++count;
	}
	
	return count;
}

// 获取状态玩家数量
unsigned int CCattlesMsgHandler::getPlayerCount(const SCattlesRoomData& roomData, const int status)
{
	unsigned int count = 0;
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		if (it->second.status() == status) ++count;
	}
	
	return count;
}

// 同步发送消息到房间的其他玩家
int CCattlesMsgHandler::sendPkgMsgToRoomPlayers(const SCattlesRoomData& roomData, const ::google::protobuf::Message& pkg, unsigned short protocolId,
                                                const char* logInfo, const string& exceptName, int minStatus)
{
	const char* msgData = m_srvOpt.serializeMsgToBuffer(pkg, logInfo);
	if (msgData == NULL) return SerializeMessageToArrayError;

    // 房间玩家
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		if (exceptName != it->first && it->second.status() >= minStatus) m_srvOpt.sendMsgToProxyEx(msgData, pkg.ByteSize(), protocolId, it->first.c_str());
	}
	
	return SrvOptSuccess;
}

// 刷新游戏玩家状态通知
int CCattlesMsgHandler::updatePlayerStatusNotify(const SCattlesRoomData& roomData, const string& player, int newStatus, const string& exceptName, const char* logInfo)
{
	// 同步发送消息到房间的其他玩家
	com_protocol::UpdatePlayerStatusNotify updatePlayerStatusNtf;
	com_protocol::PlayerStatus* upPlayerStatus = updatePlayerStatusNtf.add_player_status();
	upPlayerStatus->set_username(player);
	upPlayerStatus->set_new_status(newStatus);

	return sendPkgMsgToRoomPlayers(roomData, updatePlayerStatusNtf, COMM_UPDATE_PLAYER_STATUS_NOTIFY, logInfo, exceptName);
}

int CCattlesMsgHandler::updatePlayerStatusNotify(const SCattlesRoomData& roomData, const StringVector& players, int newStatus, const string& exceptName, const char* logInfo)
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
int CCattlesMsgHandler::updatePrepareStatusNotify(const SCattlesRoomData& roomData, com_protocol::ETrueFalseType status, const char* logInfo)
{
	com_protocol::CattlesPrepareStatusNotify prepareStatusNtf;
	prepareStatusNtf.set_prepare_status(status);
	
	return sendPkgMsgToRoomPlayers(roomData, prepareStatusNtf, CATTLES_PREPARE_STATUS_NOTIFY,
                                   logInfo, "", com_protocol::ERoomPlayerStatus::EPlayerSitDown);
}


bool CCattlesMsgHandler::startHeartbeatCheck(unsigned int checkIntervalSecs, unsigned int checkCount)
{
	if (checkIntervalSecs <= 1 || checkCount <= 1) return false;

	m_serviceHeartbeatTimerId = setTimer(checkIntervalSecs * MillisecondUnit, (TimerHandler)&CCattlesMsgHandler::onHeartbeatCheck, 0, NULL, -1);
	m_checkHeartbeatCount = checkCount;
	
	return (m_serviceHeartbeatTimerId != 0);
}

void CCattlesMsgHandler::stopHeartbeatCheck()
{
	m_srvOpt.stopTimer(m_serviceHeartbeatTimerId);
}

void CCattlesMsgHandler::addHearbeatCheck(const ConstCharPointerVector& usernames)
{
	for (ConstCharPointerVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it) addHearbeatCheck(*it);
}

void CCattlesMsgHandler::addHearbeatCheck(const StringVector& usernames)
{
	for (StringVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it) addHearbeatCheck(*it);
}

void CCattlesMsgHandler::addHearbeatCheck(const string& username)
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

void CCattlesMsgHandler::delHearbeatCheck(const ConstCharPointerVector& usernames)
{
	for (ConstCharPointerVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it) delHearbeatCheck(*it);
}

void CCattlesMsgHandler::delHearbeatCheck(const StringVector& usernames)
{
	for (StringVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it) delHearbeatCheck(*it);
}

void CCattlesMsgHandler::delHearbeatCheck(const string& username)
{
	m_heartbeatFailedTimes.erase(username);
}

void CCattlesMsgHandler::clearHearbeatCheck()
{
	m_heartbeatFailedTimes.clear();
}

void CCattlesMsgHandler::heartbeatCheck(const string& username)
{
	HeartbeatFailedTimesMap::iterator it = m_heartbeatFailedTimes.find(username);
	if (it != m_heartbeatFailedTimes.end())
	{
		if (it->second == 0) onHeartbeatCheckSuccess(username);  // 回调过心跳失败函数后重新收到消息
		
		it->second = 1;
	}
}

void CCattlesMsgHandler::onHeartbeatCheckFail(const string& username)
{
	m_gameProcess.onHearbeatResult(username, false);
}

void CCattlesMsgHandler::onHeartbeatCheckSuccess(const string& username)
{
	m_gameProcess.onHearbeatResult(username, true);
}

void CCattlesMsgHandler::onHeartbeatCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount)
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


// 房间的最小下注额
unsigned int CCattlesMsgHandler::getMinBetValue(const SCattlesRoomData& roomData)
{
	const vector<unsigned int>& betRate = roomData.isGoldRoom() ?
	m_srvOpt.getCattlesBaseCfg().gold_base_number_cfg.bet_rate : m_srvOpt.getCattlesBaseCfg().card_base_number_cfg.bet_rate;
	const unsigned int minBetRate = !betRate.empty() ? betRate[0] : 1;  // 最小底注倍率
	
	return (minBetRate * roomData.roomInfo.cattle_room().number_index());
}

// 房间的最大下注额
unsigned int CCattlesMsgHandler::getMaxBetValue(const SCattlesRoomData& roomData)
{
	const vector<unsigned int>& betRate = roomData.isGoldRoom() ?
	m_srvOpt.getCattlesBaseCfg().gold_base_number_cfg.bet_rate : m_srvOpt.getCattlesBaseCfg().card_base_number_cfg.bet_rate;
	const unsigned int maxBetRate = !betRate.empty() ? betRate[betRate.size() - 1] : 1;  // 最大底注倍率
	
	return (maxBetRate * roomData.roomInfo.cattle_room().number_index());
}

// 闲家需要的最低金币数量
unsigned int CCattlesMsgHandler::getMinNeedPlayerGold(const SCattlesRoomData& roomData)
{
	// 玩家的金币 >= 最小底注*牌型最大倍率
	return (getMinBetValue(roomData) * roomData.maxRate);
}

// 庄家需要的最低金币数量
unsigned int CCattlesMsgHandler::getMinNeedBankerGold(const SCattlesRoomData& roomData)
{
	if (!roomData.isGoldRoom()) return 0;  // 非金币场无需金币

	// 庄家的金币 >= 最大底注*闲家人数*牌型最大倍率
	const unsigned int playerCount = roomData.roomInfo.base_info().player_count() - 1;  // 闲家人数
	
	return (getMaxBetValue(roomData) * playerCount * roomData.maxRate);
}

// 检查是否存在构成牛牛的三张牌
bool CCattlesMsgHandler::getCattlesCardIndex(const CattlesHandCard poker, unsigned int& idx1, unsigned int& idx2, unsigned int& idx3)
{
	unsigned int cattlesValue = 0;
	for (idx1 = 0; idx1 < CattlesCardCount - 2; ++idx1)
	{
		for (idx2 = (idx1 + 1); idx2 < CattlesCardCount - 1; ++idx2)
		{
			for (idx3 = (idx2 + 1); idx3 < CattlesCardCount; ++idx3)
			{
				// 任意3张牌的值
				cattlesValue = CardValue[poker[idx1] % PokerCardBaseValue] + CardValue[poker[idx2] % PokerCardBaseValue] + CardValue[poker[idx3] % PokerCardBaseValue];
				if ((cattlesValue % BaseCattlesValue) == 0) return true; // 存在牛X
			}
		}
	}
	
	return false;
}

CServiceOperationEx& CCattlesMsgHandler::getSrvOpt()
{
	return m_srvOpt;
}

CGameProcess& CCattlesMsgHandler::getGameProcess()
{
	return m_gameProcess;
}


void CCattlesMsgHandler::updateConfig()
{
	m_srvOpt.updateCommonConfig(CCfg::getValue("Service", "ConfigFile"));
	
	m_pCfg = &NCattlesConfigData::CattlesConfig::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str(), true);
	if (!m_pCfg->isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update business xml config value error");
	}

	if (!m_srvOpt.getServiceCommonCfg(true).isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update service xml config value error");
	}

	if (!m_srvOpt.getCattlesBaseCfg(true).isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update cattles base xml config value error");
	}
	
	// 心跳更新
	const ServiceCommonConfig::HeartbeatCfg& heartbeatCfg = m_srvOpt.getServiceCommonCfg().heart_beat_cfg;
	stopHeartbeatCheck();
	startHeartbeatCheck(heartbeatCfg.heart_beat_interval, heartbeatCfg.heart_beat_count);

	m_gameProcess.updateConfig();
	m_gameLogicHandler.updateConfig();
	
	ReleaseInfoLog("update config business result = %d, service result = %d, cattles result = %d",
	m_pCfg->isSetConfigValueSuccess(), m_srvOpt.getServiceCommonCfg().isSetConfigValueSuccess(), m_srvOpt.getCattlesBaseCfg().isSetConfigValueSuccess());

	m_srvOpt.getServiceCommonCfg().output();
	m_srvOpt.getCattlesBaseCfg().output();
	m_pCfg->output();
}

// 通知逻辑层对应的逻辑连接代理已被关闭
void CCattlesMsgHandler::onCloseConnectProxy(void* userData, int cbFlag)
{
	GameUserData* cud = (GameUserData*)userData;
	removeConnectProxy(string(cud->userName), 0, false);  // 有可能是被动关闭，所以这里要清除下连接信息

	logoutNotify(cud, cbFlag);
}


void CCattlesMsgHandler::addWaitForDisbandRoom(const unsigned int hallId, const unsigned int roomId)
{
	unsigned long long hallRoomId = hallId;
	*((unsigned int*)&hallRoomId + 1) = roomId;
	
	// 空房间解散时间点
	m_waitForDisbandRoomInfo[hallRoomId] = time(NULL) + m_srvOpt.getServiceCommonCfg().game_cfg.disband_room_wait_secs;
}

void CCattlesMsgHandler::delWaitForDisbandRoom(const unsigned int hallId, const unsigned int roomId)
{
	unsigned long long hallRoomId = hallId;
	*((unsigned int*)&hallRoomId + 1) = roomId;

	m_waitForDisbandRoomInfo.erase(hallRoomId);
}

// 解散所有等待的空房间
void CCattlesMsgHandler::disbandAllWaitForRoom()
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

void CCattlesMsgHandler::startCheckDisbandRoom()
{
	unsigned int checkDisbandRoomIntervalSecs = m_srvOpt.getServiceCommonCfg().game_cfg.check_disband_room_interval;
	if (checkDisbandRoomIntervalSecs < 5) checkDisbandRoomIntervalSecs = 5;
	setTimer(checkDisbandRoomIntervalSecs * MillisecondUnit, (TimerHandler)&CCattlesMsgHandler::onCheckDisbandRoom, 0, NULL, (unsigned int)-1);
}

// 检测待解散的空房间，超时时间到则解散房间
void CCattlesMsgHandler::onCheckDisbandRoom(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	const unsigned int curSecs = time(NULL);
	char strHallId[IdMaxLen] = {0};
	char strRoomId[IdMaxLen] = {0};
	SCattlesRoomData* roomData = NULL;
	for (WaitForDisbandRoomInfoMap::iterator it = m_waitForDisbandRoomInfo.begin(); it != m_waitForDisbandRoomInfo.end();)
	{
		snprintf(strHallId, sizeof(strHallId) - 1, "%u", (unsigned int)it->first);
		snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", *((unsigned int*)&(it->first) + 1));
		
		roomData = getRoomDataById(strRoomId, strHallId);
		if (roomData == NULL || (curSecs > it->second && m_gameProcess.canDisbandGameRoom(*roomData)))
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
void CCattlesMsgHandler::saveDataToRedis(unsigned int timerId, int userId, void* param, unsigned int remainCount)
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

int CCattlesMsgHandler::addRoomData(const com_protocol::ChessHallRoomInfo& roomInfo)
{
	const bool isRoomCardType = (roomInfo.base_info().room_type() == com_protocol::EHallRoomType::ECardId
			                     || roomInfo.base_info().room_type() == com_protocol::EHallRoomType::ECardEnter);
    const NCattlesBaseConfig::CattlesBaseConfig& ctlBaseCfg = m_srvOpt.getCattlesBaseCfg();
	const com_protocol::CattleRoomInfo& cattleRoomInfo = roomInfo.cattle_room();
	if (cattleRoomInfo.rate_rule_index() >= ctlBaseCfg.rate_rule_cfg.size()
	    || cattleRoomInfo.number_index() < 1
		|| (!isRoomCardType && cattleRoomInfo.number_index() > ctlBaseCfg.gold_base_number_cfg.max_base_bet_value)
		|| (isRoomCardType && cattleRoomInfo.number_index() > ctlBaseCfg.card_base_number_cfg.max_base_bet_value))
	{
		return CattlesBaseConfigError;
	}
				
	SHallCattlesData& hallCattlesData = m_hallCattlesData[roomInfo.base_info().hall_id()];
	SCattlesRoomData& roomData = hallCattlesData.cattlesRooms[roomInfo.base_info().room_id()];

	// 游戏状态为准备状态
	roomData.roomInfo = roomInfo;
	roomData.gameStatus = com_protocol::ECattlesGameStatus::ECattlesGameReady;
	roomData.maxRate = getCardMaxRate(roomData);
	
	OptInfoLog("add game room, hallId = %s, roomId = %s, base rate = %u, player count = %u",
	roomInfo.base_info().hall_id().c_str(), roomInfo.base_info().room_id().c_str(),
	cattleRoomInfo.number_index(), roomInfo.base_info().player_count());
	
	return SrvOptSuccess;
}

// 解散房间，删除房间数据
bool CCattlesMsgHandler::delRoomData(const char* hallId, const char* roomId, bool isNotifyPlayer)
{
	HallCattlesMap::iterator hallIt = m_hallCattlesData.find(hallId);
	if (hallIt == m_hallCattlesData.end()) return false;
	
	CattlesRoomMap::iterator ctsRoomIt = hallIt->second.cattlesRooms.find(roomId);
	if (ctsRoomIt == hallIt->second.cattlesRooms.end()) return false;
	
	StringVector delPlayers;
	for (RoomPlayerIdInfoMap::const_iterator it = ctsRoomIt->second.playerInfo.begin(); it != ctsRoomIt->second.playerInfo.end(); ++it)
	{
		delPlayers.push_back(it->first);
	}

	// 1）房间使用完毕，变更房间状态为使用完毕，同步通知到棋牌室大厅在线的玩家
	m_srvOpt.stopTimer(ctsRoomIt->second.optTimerId);
	m_srvOpt.changeRoomDataNotify(hallId, roomId, CattlesGame, com_protocol::EHallRoomStatus::EHallRoomFinish, NULL, NULL, NULL, "room use finish");

	// 2）通知房间里的所有玩家退出房间
	if (isNotifyPlayer)
	{
	    com_protocol::ClientGameRoomFinishNotify finishNtf;
	    sendPkgMsgToRoomPlayers(ctsRoomIt->second, finishNtf, COMM_GAME_ROOM_FINISH_NOTIFY, "game room finish notify", "");
	}

	// 3）删除房间数据，防止用户离开房间导致递归调用此函数
	hallIt->second.cattlesRooms.erase(ctsRoomIt);
	if (hallIt->second.cattlesRooms.empty()) m_hallCattlesData.erase(hallIt);
	
	// 4）删除玩家心跳检测
	delHearbeatCheck(delPlayers);

	// 5）房间里的用户断开连接，离开房间
	const unsigned int count = delUserFromRoom(delPlayers);
	
	OptInfoLog("delete game room, hallId = %s, roomId = %s, remove player size = %u, count = %u",
	hallId, roomId, delPlayers.size(), count);

	return true;
}

void CCattlesMsgHandler::addUserToRoom(const char* userName, SCattlesRoomData* roomData, const com_protocol::DetailInfo& userDetailInfo)
{
	com_protocol::ClientRoomPlayerInfo& newPlayerInfo = roomData->playerInfo[userName];
	*newPlayerInfo.mutable_detail_info() = userDetailInfo;
	newPlayerInfo.set_seat_id(-1);  // 此时没有座位号
	newPlayerInfo.set_status(com_protocol::ERoomPlayerStatus::EPlayerEnter);  // 玩家状态为进入房间
}

bool CCattlesMsgHandler::delUserFromRoom(const string& userName)
{
	// 已经掉线了的玩家，在真正离开房间前没有重新连入房间，则之前的会话数据需要清除
	bool isOK = removeConnectProxy(userName, com_protocol::EUserQuitStatus::EUserOffline);  // 删除与客户端的连接代理，不会关闭连接
	if (!isOK) delUserSessionData(userName.c_str(), userName.length());

	return isOK;
}

com_protocol::ClientRoomPlayerInfo* CCattlesMsgHandler::getPlayerInfoBySeatId(RoomPlayerIdInfoMap& playerInfo, const int seatId)
{
	for (RoomPlayerIdInfoMap::iterator it = playerInfo.begin(); it != playerInfo.end(); ++it)
	{
		if (it->second.seat_id() == seatId) return &it->second;
	}
	
	return NULL;
}

int CCattlesMsgHandler::getFreeSeatId(GameUserData* cud, SCattlesRoomData* roomData, int& seatId)
{
	const int maxPlayerCount = roomData->roomInfo.base_info().player_count();

	if ((int)getPlayerCount(*roomData, com_protocol::ERoomPlayerStatus::EPlayerSitDown) >= maxPlayerCount) return CattlesRoomSeatFull;

	if (cud->seatId >= 0)
	{
		// 玩家选定座位
		if (cud->seatId >= maxPlayerCount) return CattlesInvalidSeatID;

		if (getPlayerInfoBySeatId(roomData->playerInfo, cud->seatId) != NULL) return CattlesRoomSeatInUse;
		
		seatId = cud->seatId;
	}
	else
	{
		// 服务器自动分配座位
		seatId = 0;
		for (; seatId < maxPlayerCount; ++seatId)
		{
			if (getPlayerInfoBySeatId(roomData->playerInfo, seatId) == NULL) break;
		}
		
		if (seatId >= maxPlayerCount) return CattlesRoomSeatFull;
	}
	
	return SrvOptSuccess;
}

int CCattlesMsgHandler::userSitDown(GameUserData* cud, SCattlesRoomData* roomData)
{
	int rc = SrvOptFailed;
	RoomPlayerIdInfoMap::iterator userInfoIt = roomData->playerInfo.find(cud->userName);
	do
	{
		if (userInfoIt == roomData->playerInfo.end())
		{
			rc = CattlesNotRoomUserInfo;
			break;
		}
		
		// 必须是审核通过的棋牌室玩家才可以参与游戏
		if (!isHallPlayer(cud))
		{
			rc = CattlesNoHallCheckOKUser;
			break;
		}

		if (roomData->isGoldRoom())
		{
			// 最小下注额
			if (userInfoIt->second.detail_info().dynamic_info().game_gold() < getMinNeedPlayerGold(*roomData))
			{
				rc = CattlesGameGoldInsufficient;
				break;
			}
		}
		else if (!checkRoomCardIsEnough(cud->userName, userInfoIt->second.detail_info().dynamic_info().room_card(), *roomData))
		{
			rc = CattlesRoomCardInsufficient;
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
		m_srvOpt.changeRoomDataNotify(cud->hallId, cud->roomId, CattlesGame, com_protocol::EHallRoomStatus::EHallRoomReady,
		                              &roomData->roomInfo.base_info(), &userInfoIt->second, NULL, "room ready");
	}
	else
	{
		// 2）有玩家坐下，通知到棋牌室大厅在线玩家
        m_srvOpt.updateRoomDataNotifyHall(cud->hallId, cud->roomId, CattlesGame, com_protocol::EHallRoomStatus::EHallRoomReady,
		                                  NULL, &userInfoIt->second, NULL, "player sit down notify hall");
	}

	return SrvOptSuccess;
}

bool CCattlesMsgHandler::checkRoomCardIsEnough(const char* username, const float roomCardCount, const SCattlesRoomData& roomData)
{
	const NCattlesBaseConfig::RoomCardCfg& roomCardCfg = m_srvOpt.getCattlesBaseCfg().room_card_cfg;
	const unsigned int payMode = roomData.roomInfo.base_info().pay_mode();
	const unsigned int gameTimes = roomData.roomInfo.base_info().game_times();

	return !((payMode == com_protocol::EHallRoomPayMode::ERoomAveragePay
		      && (roomCardCount < roomCardCfg.average_pay_count * gameTimes))
		     || (payMode == com_protocol::EHallRoomPayMode::ERoomCreatorPay
		         && roomData.roomInfo.base_info().username() == username
			     && (roomCardCount < roomCardCfg.creator_pay_count * gameTimes)));
}

// 是否是棋牌室审核通过的正式玩家
bool CCattlesMsgHandler::isHallPlayer(const GameUserData* cud)
{
	// 棋牌室管理员审核通过才能成为棋牌室正式玩家，才可以参与游戏
	return (cud->inHallStatus == com_protocol::EHallPlayerStatus::ECheckAgreedPlayer);
}

// 是否可以开始游戏
int CCattlesMsgHandler::canStartGame(const SCattlesRoomData& roomData, bool isAuto)
{
	const unsigned int startPlayers = roomData.roomInfo.cattle_room().start_players();
	if (isAuto && startPlayers == 1 && roomData.playTimes < 1) return CattlesNotAutoStartRoom;  // 需要手动开桌才能开始游戏
	if (!isAuto && startPlayers != 1) return CattlesNotManualStartRoom;  // 只能自动开桌游戏

	unsigned int readyCount = 0;
	int playerStatus = 0;
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		playerStatus = it->second.status();
		if (playerStatus == com_protocol::ERoomPlayerStatus::EPlayerSitDown) return CattlesHasSitDownPlayer;
		
		if (playerStatus == com_protocol::ERoomPlayerStatus::EPlayerReady) ++readyCount;
	}
	
	// 1、没有在坐的玩家且准备玩家个数两个以上则可以开始游戏（金币场、房卡手动场）
	// 2、满足自动开桌人数（房卡自动场）
	return (((roomData.isGoldRoom() || startPlayers == 1) && readyCount > 1)
	        || (startPlayers > 1 && readyCount >= startPlayers)) ? (int)SrvOptSuccess : CattlesStartPlayerInsufficient;
}

// 获取房间最大倍率
unsigned int CCattlesMsgHandler::getCardMaxRate(const SCattlesRoomData& roomData)
{
	unsigned int specialMaxRate = 1;
	const ::google::protobuf::RepeatedField<int>& selectSpecialType = roomData.roomInfo.cattle_room().special_card_type();
	if (selectSpecialType.size() > 0)
	{
		int maxCardType = 0;
		for (::google::protobuf::RepeatedField<int>::const_iterator it = selectSpecialType.begin(); it != selectSpecialType.end(); ++it)
		{
			if (*it > maxCardType) maxCardType = *it;
		}
		
		map<int, unsigned int>::const_iterator spRtIt = m_srvOpt.getCattlesBaseCfg().special_card_rate.find(maxCardType);
		if (spRtIt != m_srvOpt.getCattlesBaseCfg().special_card_rate.end()) specialMaxRate = spRtIt->second;
	}

	// 基础倍率的最大倍率为牛牛
	unsigned int baseMaxRate = 1;
	const unsigned int rateIndex = roomData.roomInfo.cattle_room().rate_rule_index();
	const map<int, unsigned int>& baseRate = m_srvOpt.getCattlesBaseCfg().rate_rule_cfg[rateIndex].cattles_value_rate;
	map<int, unsigned int>::const_iterator baseRtIt = baseRate.find(com_protocol::ECattleBaseCardType::ECattleCattle);
	if (baseRtIt != baseRate.end()) baseMaxRate = baseRtIt->second;
	
	return (specialMaxRate > baseMaxRate) ? specialMaxRate : baseMaxRate;
}

// 设置游戏房间数据
bool CCattlesMsgHandler::setGameRoomData(const string& username, const SCattlesRoomData& roomData,
                                         google::protobuf::RepeatedPtrField<com_protocol::ClientRoomPlayerInfo>& roomPlayers,
                                         com_protocol::ChessHallGameData& gameData)
{
	OptInfoLog("test set game room data, username = %s", username.c_str());

	// 房间里玩家的信息
	for (RoomPlayerIdInfoMap::const_iterator it = roomData.playerInfo.begin(); it != roomData.playerInfo.end(); ++it)
	{
		OptInfoLog("test set game player info, username = %s, status = %u, seat = %d",
        it->first.c_str(), it->second.status(), it->second.seat_id());
			
		*roomPlayers.Add() = it->second;
	}

	if (!roomData.isOnPlay()) return false;

    // 正在游戏中则回传游戏数据
	com_protocol::ChessHallGameBaseData* gameBaseData = gameData.mutable_base_data();
	gameBaseData->set_game_type(CattlesGame);
	gameBaseData->set_game_status(roomData.gameStatus);
	gameBaseData->set_current_round(roomData.playTimes);
	
	com_protocol::CattleGameInfo* cattlesGameInfo = gameData.mutable_cattle_game();
	const char* currentBanker = roomData.getCurrentBanker();
	if (currentBanker != NULL) cattlesGameInfo->set_banker_id(currentBanker);
	
	// 剩余倒计时时间
	if (roomData.optTimerId > 0 && roomData.optTimeSecs > 0 && roomData.optLastTimeSecs > 0)
	{
		const unsigned int passTimeSecs = time(NULL) - roomData.optTimeSecs;
		if (roomData.optLastTimeSecs > passTimeSecs) cattlesGameInfo->set_wait_secs(roomData.optLastTimeSecs - passTimeSecs);
	}
	
	const unsigned int minBankerGold = getMinNeedBankerGold(roomData); // 庄家需要的最低金币数量
	for (GamePlayerIdDataMap::const_iterator it = roomData.gamePlayerData.begin(); it != roomData.gamePlayerData.end(); ++it)
	{
		// 游戏玩家数据
		RoomPlayerIdInfoMap::const_iterator plIt = roomData.playerInfo.find(it->first);
		if (plIt != roomData.playerInfo.end())
		{
			com_protocol::CattlesGamePlayerInfo* gamePlayer = cattlesGameInfo->add_player_info();
			gamePlayer->set_username(it->first);
			gamePlayer->set_status(plIt->second.status());
			
			if (plIt->second.detail_info().dynamic_info().game_gold() >= minBankerGold)
			{
				gamePlayer->set_is_can_compete(com_protocol::ETrueFalseType::ETrueType);
			}
			else
			{
				gamePlayer->set_is_can_compete(com_protocol::ETrueFalseType::EFalseType);
			}

            gamePlayer->set_choice_banker_result((com_protocol::ETrueFalseType)it->second.choiceBankerResult);
			
			// 非庄家下注额
			if (!it->second.isNewBanker && it->second.betValue > 0) gamePlayer->set_bet_value(it->second.betValue);
			
			// 房卡场数据
			if (!roomData.isGoldRoom()) gamePlayer->set_sum_win_lose(it->second.sumWinLoseValue);
			
			OptInfoLog("test set game player data, username = %s, status = %u, can compete = %d, choice banker result = %d, is banker = %d, bet value = %u",
            gamePlayer->username().c_str(), gamePlayer->status(), gamePlayer->is_can_compete(), gamePlayer->choice_banker_result(),
			it->second.isNewBanker, it->second.betValue);

            // 开牌状态则只有玩家本人和闲家的亮牌会传回
			if (roomData.gameStatus == com_protocol::ECattlesGameStatus::ECattlesGamePushCard
				&& (it->first == username
				    || (!it->second.isNewBanker && gamePlayer->status() == com_protocol::ECattlesPlayerStatus::ECattlesOpenCard)))
			{
				setGamePlayerInfo(it->second, *gamePlayer);
			}
			else if (it->first == username
			         && roomData.roomInfo.cattle_room().type() == com_protocol::ECattleBankerType::EOpenCardBanker)
			{
				// 明牌抢庄的4张手牌
				for (unsigned int idx = 0; idx < CattlesCardCount - 1; ++idx) gamePlayer->add_pokers(it->second.poker[idx]);
			}
		}
	}
	
	OptInfoLog("test set game room data, username = %s, game status = %u, bankerId = %s, player count = %u, user count = %u, address = %p",
    username.c_str(), roomData.gameStatus, cattlesGameInfo->banker_id().c_str(), (unsigned int)roomData.gamePlayerData.size(), (unsigned int)roomData.playerInfo.size(), &roomData);

	return true;
}

void CCattlesMsgHandler::setGamePlayerInfo(const SGamePlayerData& playerData, com_protocol::CattlesGamePlayerInfo& gamePlayerInfo)
{
	gamePlayerInfo.set_settlement_rate(playerData.settlementRate);
	gamePlayerInfo.set_result_value(playerData.resultValue);

	for (unsigned int idx = 0; idx < CattlesCardCount; ++idx) gamePlayerInfo.add_pokers(playerData.poker[idx]);
	
	// 检查是否存在构成牛牛的三张牌
	unsigned int idx1 = 0;
	unsigned int idx2 = 0;
	unsigned int idx3 = 0;
	if (playerData.resultValue > 0 && playerData.resultValue <= com_protocol::ECattleBaseCardType::ECattleCattle
		&& getCattlesCardIndex(playerData.poker, idx1, idx2, idx3))
	{
		gamePlayerInfo.add_cattles_cards(playerData.poker[idx1]);
		gamePlayerInfo.add_cattles_cards(playerData.poker[idx2]);
		gamePlayerInfo.add_cattles_cards(playerData.poker[idx3]);
	}
}

// 进入房间session key检查
int CCattlesMsgHandler::checkSessionKey(const string& username, const string& sessionKey, SessionKeyData& sessionKeyData)
{
	// 验证会话数据是否正确
	if (m_redisDbOpt.getHField(NProject::SessionKey, NProject::SessionKeyLen, username.c_str(), username.length(),
		(char*)&sessionKeyData, sizeof(sessionKeyData)) != sizeof(sessionKeyData)) return CattlesGetSessionKeyError;
	
	char sessionKeyStr[NProject::IdMaxLen] = {0};
	NProject::getSessionKey(sessionKeyData, sessionKeyStr);
	if (sessionKey != sessionKeyStr) return CattlesCheckSessionKeyFailed;
	
	return SrvOptSuccess;
}

// 删除用户会话数据
void CCattlesMsgHandler::delUserSessionData(const char* username, unsigned int usernameLen)
{
	int rc = m_redisDbOpt.delHField(GameUserKey, GameUserKeyLen, username, usernameLen);
	if (rc < 0) OptErrorLog("delete user session data from redis error, username = %s, rc = %d", username, rc);
}

// 通知连接logout操作
void CCattlesMsgHandler::logoutNotify(void* cb, int quitStatus)
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
		SCattlesRoomData* roomData = getRoomDataById(ud->roomId, ud->hallId);
		if (roomData != NULL)
		{
			if (ud->seatId >= 0)  //  && roomData != NULL && roomData->hasSeatPlayer())
			{
				// 有玩家从座位上离开还没有解散的房间，则同步消息到大厅
				m_srvOpt.updateRoomDataNotifyHall(ud->hallId, ud->roomId, CattlesGame, (com_protocol::EHallRoomStatus)roomData->roomStatus,
												  NULL, NULL, ud->userName, "player leave room notify hall");
			}
			
			// 所有在座的玩家离开房间，导致房间可以解散
			// 但需要保留一段时间，玩家可能会重入房间
			if (playerStatus == EGamePlayerStatus::ELeaveRoom && m_gameProcess.canDisbandGameRoom(*roomData))
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
	
	OptInfoLog("cattles game user logout, name = %s, connect status = %d, online time = %u, quit status = %d, player status = %d",
	ud->userName, ud->status, onlineTimeSecs, quitStatus, playerStatus);
	
	m_srvOpt.destroyConnectProxyUserData(ud);
}

void CCattlesMsgHandler::userEnterRoom(ConnectProxy* conn, const com_protocol::DetailInfo& detailInfo)
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
	SCattlesRoomData* roomData = NULL;
	do
	{
		roomData = getRoomDataById(cud->roomId, cud->hallId);
		if (roomData == NULL)
		{
			OptErrorLog("get room data error, hallId = %s, roomId = %s", cud->hallId, cud->roomId);
	
			enterRoomRsp.set_result(CattlesNotFoundRoomData);
			break;
		}
		
		// 默认坐下
		if (cud->seatId >= -1)
		{
			if (roomData->isGoldRoom())
			{
				// 如果是默认坐下，需要判断金币是否足够，只有房间的创建者才可以默认坐下，则金币必须足够
				if (detailInfo.dynamic_info().game_gold() < roomData->roomInfo.cattle_room().need_gold())
				{
					enterRoomRsp.set_need_gold(roomData->roomInfo.cattle_room().need_gold());
					enterRoomRsp.set_result(CattlesGameGoldInsufficient);
					break;
				}
			}
			else if (!checkRoomCardIsEnough(userName.c_str(), detailInfo.dynamic_info().room_card(), *roomData))
			{
				enterRoomRsp.set_result(CattlesRoomCardInsufficient);
				break;
			}
		}
		
		// 设置用户会话数据
        rc = setUserSessionData(userName.c_str(), userName.length(), EGamePlayerStatus::EInRoom);
		if (rc != SrvOptSuccess)
		{
			enterRoomRsp.set_result(CattlesSetUserSessionDataFailed);
			break;
		}
		
		// 成功则替换连接代理关联信息，变更连接ID为用户名
		m_srvOpt.checkConnectProxySuccess(userName, conn);

		// 玩家加入房间
		addUserToRoom(userName.c_str(), roomData, detailInfo);
		
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

void CCattlesMsgHandler::userReEnterRoom(const string& userName, ConnectProxy* conn, GameUserData* cud, SCattlesRoomData& roomData,
                                         const com_protocol::ClientRoomPlayerInfo& userInfo, SGamePlayerData& playerData)
{
	// 成功则替换连接代理关联信息，变更连接ID为用户名
	m_srvOpt.checkConnectProxySuccess(userName, conn);
	playerData.isOnline = true;

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
	m_gameProcess.refuseDismissRoomContinueGame(cud, roomData, true);
	
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

EGamePlayerStatus CCattlesMsgHandler::userLeaveRoom(GameUserData* cud, int quitStatus)
{
	EGamePlayerStatus playerStatus = EGamePlayerStatus::ERoomDisband; // 如果找不到房间即已经解散
	SCattlesRoomData* roomData = getRoomDataById(cud->roomId, cud->hallId);
	if (roomData != NULL) playerStatus = m_gameProcess.playerLeaveRoom(cud, *roomData, quitStatus);  // 房间里的玩家

	return playerStatus;
}

void CCattlesMsgHandler::onLine(GameUserData* cud, ConnectProxy* conn, const com_protocol::DetailInfo& userDetailInfo)
{
}

void CCattlesMsgHandler::offLine(GameUserData* cud)
{
}

// 管理员赠送物品通知
void CCattlesMsgHandler::giveGoodsNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	SCattlesRoomData* roomData = getRoomDataByUser(getContext().userData);
	if (roomData != NULL)
	{
		com_protocol::ClientGiveGoodsToPlayerNotify ggNtf;
		if (!m_srvOpt.parseMsgFromBuffer(ggNtf, data, len, "give goods notify")) return;

		RoomPlayerIdInfoMap::iterator it = roomData->playerInfo.find(getContext().userData);
		if (it != roomData->playerInfo.end())
		{
			if (ggNtf.give_goods().type() == EGameGoodsType::EGoodsGold)
			{
			    it->second.mutable_detail_info()->mutable_dynamic_info()->set_game_gold(ggNtf.give_goods().amount());
			}
			else if (ggNtf.give_goods().type() == EGameGoodsType::EGoodsRoomCard)
			{
				it->second.mutable_detail_info()->mutable_dynamic_info()->set_room_card(ggNtf.give_goods().amount());
			}
		}
		
		OptInfoLog("receive give goods notify, username = %s, amount = %.2f", getContext().userData, ggNtf.give_goods().amount());
	}
}

void CCattlesMsgHandler::enter(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
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
            rsp.set_result(CattlesInvalidChessHallId);
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

		SCattlesRoomData* clsRoomData = getRoomDataById(req.room_id().c_str(), req.hall_id().c_str());
		if (clsRoomData != NULL)
		{
			const com_protocol::ChessHallRoomBaseInfo& rmBaseInfo = clsRoomData->roomInfo.base_info();
			if ((rmBaseInfo.room_type() == com_protocol::EHallRoomType::EGoldId
			     || rmBaseInfo.room_type() == com_protocol::EHallRoomType::ECardId)  // 私人房间
			    && req.is_be_invited() != 1 && req.username() != rmBaseInfo.username())
			{
				// 私人房间除了房主，其他玩家不可以直接进入，需要被邀请才能进入
				rsp.set_result(CattlesPrivateRoomCanNotBeEnter);
				break;
			}
			
			if (rmBaseInfo.is_can_view() != 1  // 该房间不可以旁观
			    && (!isHallPlayer(curCud)                             // 是否是棋牌室审核通过的正式玩家
				    || clsRoomData->getRoomPlayerCount() >= rmBaseInfo.player_count())  // 房间满人了
			   )
			{
				rsp.set_result(CattlesRoomCanNotBeView);
				break;
			}

			// 已经存在房间数据，则获取用户信息即可
			com_protocol::GetUserDataReq getUserDataReq;
			getUserDataReq.set_username(req.username());
			getUserDataReq.set_info_type(com_protocol::EUserInfoType::EUserStaticDynamic);
			getUserDataReq.set_hall_id(req.hall_id());

			rc = m_srvOpt.sendPkgMsgToService(curCud->userName, curCud->userNameLen, getUserDataReq, dbProxySrvId,
											  DBPROXY_GET_USER_DATA_REQ, "enter room get user data request", 0,
											  CATTLES_GET_USER_INFO_FOR_ENTER_ROOM);
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
                                              req.is_be_invited(), CATTLES_GET_ROOM_INFO_FOR_ENTER_ROOM);
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

void CCattlesMsgHandler::getUserInfoForEnterReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
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

void CCattlesMsgHandler::getRoomInfoForEnterReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
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
			enterRoomRsp.set_result(CattlesPrivateRoomCanNotBeEnter);
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

void CCattlesMsgHandler::sitDown(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
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
			rsp.set_result(CattlesNotFoundRoomData);
			break;
		}
		
		if (roomData->gameStatus != com_protocol::ECattlesGameStatus::ECattlesGameReady)
		{
			rsp.set_result(CattlesGameStatusInvalidOpt);
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
		if (!roomData->currentHasBanker && m_gameProcess.hasNextNewBanker(*roomData))
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

void CCattlesMsgHandler::prepare(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientPlayerPrepareRsp rsp;
	GameUserData* cud = (GameUserData*)m_srvOpt.getConnectProxyUserData();
	SCattlesRoomData* roomData = NULL;
	do
	{
	    roomData = getRoomDataById(cud->roomId, cud->hallId);
		if (roomData == NULL)
		{
			rsp.set_result(CattlesNotFoundRoomData);
			break;
		}
		
		if (roomData->gameStatus != com_protocol::ECattlesGameStatus::ECattlesGameReady)
		{
			rsp.set_result(CattlesGameStatusInvalidOpt);
			break;
		}
		
		// 是否存在庄家
		if (!roomData->currentHasBanker)
		{
			rsp.set_result(CattlesCurrentNoBanker);
			break;
		}
		
		RoomPlayerIdInfoMap::iterator userInfoIt = roomData->playerInfo.find(cud->userName);
		if (userInfoIt == roomData->playerInfo.end())
		{
			rsp.set_result(CattlesNotRoomUserInfo);
			break;
		}
		
		if (userInfoIt->second.status() != com_protocol::ERoomPlayerStatus::EPlayerSitDown || userInfoIt->second.seat_id() < 0)
		{
			rsp.set_result(CattlesPlayerStatusInvalidOpt);
			break;
		}

		if (roomData->isGoldRoom() && userInfoIt->second.detail_info().dynamic_info().game_gold() < getMinNeedPlayerGold(*roomData))
		{
			rsp.set_result(CattlesGameGoldInsufficient);
			break;
		}
		
		rsp.set_result(SrvOptSuccess);
		userInfoIt->second.set_status(com_protocol::ERoomPlayerStatus::EPlayerReady);  // 玩家状态为准备

		// 同步发送消息到房间的其他玩家
		com_protocol::ClientPlayerPrepareNotify preNtf;
		preNtf.set_username(cud->userName, cud->userNameLen);
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

			if (ntfUsername != NULL) m_srvOpt.sendMsgToProxyEx(NULL, 0, CATTLES_MANUAL_START_GAME_NOTIFY, ntfUsername);
		}
	
	    // 是否可以自动开始游戏
		if (canStartGame(*roomData) == SrvOptSuccess) m_gameProcess.startGame(cud->hallId, cud->roomId, *roomData); // 开始游戏
	}
}

// 手动开始游戏
void CCattlesMsgHandler::start(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientPlayerStartGameRsp rsp;
	GameUserData* cud = (GameUserData*)getProxyUserData(getConnectProxyContext().conn);
	SCattlesRoomData* roomData = NULL;
	do
	{
	    roomData = getRoomDataById(cud->roomId, cud->hallId);
		if (roomData == NULL)
		{
			rsp.set_result(CattlesNotFoundRoomData);
			break;
		}
		
		if (roomData->gameStatus != com_protocol::ECattlesGameStatus::ECattlesGameReady)
		{
			rsp.set_result(CattlesGameStatusInvalidOpt);
			break;
		}
		
		// 房主才能开始游戏
		if (roomData->roomInfo.base_info().username() != cud->userName)
		{
			rsp.set_result(CattlesNotRoomCreator);
			break;
		}
		
		// 是否存在庄家
		if (!roomData->currentHasBanker)
		{
			rsp.set_result(CattlesCurrentNoBanker);
			break;
		}
		
		RoomPlayerIdInfoMap::iterator userInfoIt = roomData->playerInfo.find(cud->userName);
		if (userInfoIt == roomData->playerInfo.end())
		{
			rsp.set_result(CattlesNotRoomUserInfo);
			break;
		}
		
		if (userInfoIt->second.status() != com_protocol::ERoomPlayerStatus::EPlayerReady)
		{
			rsp.set_result(CattlesPlayerStatusInvalidOpt);
			break;
		}

		if (!checkRoomCardIsEnough(cud->userName, userInfoIt->second.detail_info().dynamic_info().room_card(), *roomData))
		{
			rsp.set_result(CattlesRoomCardInsufficient);
			break;
		}
		
		rsp.set_result(canStartGame(*roomData, false));

	} while (false);
	
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, COMM_PLAYER_START_GAME_RSP, "player start game");
	OptInfoLog("player start game reply, username = %s, hallId = %s, roomId = %s, seatId = %d, result = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rsp.result(), rc);
	
	if (rsp.result() == SrvOptSuccess)
	{
		m_gameProcess.startGame(cud->hallId, cud->roomId, *roomData); // 开始游戏
	}
}

void CCattlesMsgHandler::leave(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	GameUserData* cud = (GameUserData*)getProxyUserData(getConnectProxyContext().conn);

	com_protocol::ClientLeaveRoomRsp rsp;
	m_gameProcess.canLeaveRoom(cud) ? rsp.set_result(SrvOptSuccess) : rsp.set_result(CattlesOnPlayCanNotLeaveRoom);
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, COMM_PLAYER_LEAVE_ROOM_RSP, "player leave room");

	OptInfoLog("player leave room, username = %s, hallId = %s, roomId = %s, seatId = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rc);
	
	if (rsp.result() == SrvOptSuccess) removeConnectProxy(string(cud->userName), com_protocol::EUserQuitStatus::EUserOffline);  // 删除与客户端的连接代理，不会关闭连接
}

void CCattlesMsgHandler::change(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	GameUserData* cud = (GameUserData*)getProxyUserData(getConnectProxyContext().conn);
	
	com_protocol::ClientChangeRoomRsp rsp;
	m_gameProcess.canLeaveRoom(cud) ? rsp.set_result(SrvOptSuccess) : rsp.set_result(CattlesOnPlayCanNotChangeRoom);
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, COMM_PLAYER_CHANGE_ROOM_RSP, "player change room");
	
	OptInfoLog("player change room, username = %s, hallId = %s, roomId = %s, seatId = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, cud->seatId, rc);
	
	if (rsp.result() == SrvOptSuccess) removeConnectProxy(string(cud->userName), com_protocol::EUserQuitStatus::EUserChangeRoom);
}

// 掉线重连，重入房间
void CCattlesMsgHandler::reEnter(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
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
            rsp.set_result(CattlesInvalidChessHallId);
            break;
        }
		
		SCattlesRoomData* roomData = getRoomDataById(req.room_id().c_str(), req.hall_id().c_str());
		if (roomData == NULL)
		{
			rsp.set_result(CattlesNotFoundRoomData);
			break;
		}

		if (!roomData->isOnPlay())
		{
			rsp.set_result(CattlesGameAlreadyFinish);  // 当局游戏已经结束
			break;
		}
		
		// 是否是游戏玩家
		GamePlayerIdDataMap::iterator playerIt = roomData->gamePlayerData.find(req.username());
	    if (playerIt == roomData->gamePlayerData.end())
		{
			rsp.set_result(CattlesNotInPlay);
			break;
		}
		
		// 验证玩家数据
		RoomPlayerIdInfoMap::const_iterator userInfoIt = roomData->playerInfo.find(req.username());
	    if (userInfoIt == roomData->playerInfo.end())
		{
			rsp.set_result(CattlesNotRoomUserInfo);
			break;
		}
		
		if (userInfoIt->second.seat_id() < 0)
		{
			rsp.set_result(CattlesPlayerNoSeat);
			break;
		}
		
		if (playerIt->second.isOnline)
		{
			OptWarnLog("player re enter but it is online, username = %s, hallId = %s, roomId = %s",
			req.username().c_str(), req.hall_id().c_str(), req.room_id().c_str());
		}
		
		// 保存掉线重连登陆信息
		strncpy(curCud->hallId, req.hall_id().c_str(), IdMaxLen - 1);
	    strncpy(curCud->roomId, req.room_id().c_str(), IdMaxLen - 1);
		curCud->seatId = userInfoIt->second.seat_id();

		// 玩家在棋牌室的状态
		curCud->inHallStatus = sessionKeyData.status;

		userReEnterRoom(req.username(), getConnectProxyContext().conn, curCud, *roomData, userInfoIt->second, playerIt->second);
		
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
void CCattlesMsgHandler::heartbeatMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// now do nothing
}

void CCattlesMsgHandler::playerChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	GameUserData* cud = (GameUserData*)m_srvOpt.getConnectProxyUserData();
	SCattlesRoomData* roomData = getRoomDataById(cud->roomId, cud->hallId);
	if (roomData != NULL)
	{
		RoomPlayerIdInfoMap::iterator userInfoIt = roomData->playerInfo.find(cud->userName);
		if (userInfoIt == roomData->playerInfo.end()
		    || userInfoIt->second.status() < com_protocol::ERoomPlayerStatus::EPlayerSitDown
			|| userInfoIt->second.seat_id() < 0)
		{
			OptErrorLog("player send chat message but not sit down, username = %s", cud->userName);
			return;
		}
		
		const ServiceCommonConfig::PlayerChatCfg& chatCfg = m_srvOpt.getServiceCommonCfg().player_chat_cfg;
	    SUserChatInfo& plChatInfo = roomData->chatInfo[cud->userName];
		const unsigned int curSecs = time(NULL);
		if ((curSecs - plChatInfo.chatSecs) > chatCfg.interval_secs)
		{
			plChatInfo.chatSecs = curSecs;
			plChatInfo.chatCount = 0;
		}
		
		if (plChatInfo.chatCount >= chatCfg.chat_count)
		{
			// OptWarnLog("player send chat message frequently, username = %s, config interval = %u, count = %u, current count = %u",
			// cud->userName, chatCfg.interval_secs, chatCfg.chat_count, plChatInfo.chatCount);

			return;
		}

        com_protocol::ClientSndMsgInfoNotify smiNtf;
	    if (!m_srvOpt.parseMsgFromBuffer(smiNtf, data, len, "room chat notify")) return;
		
		if (smiNtf.msg_info().msg_content().length() > chatCfg.chat_content_length) return;
		
		++plChatInfo.chatCount;

		*smiNtf.mutable_msg_info()->mutable_player_info() = userInfoIt->second.detail_info().static_info();
		sendPkgMsgToRoomPlayers(*roomData, smiNtf, COMM_SEND_MSG_INFO_NOTIFY,
		                        "player room chat notify", "", com_protocol::ERoomPlayerStatus::EPlayerSitDown);
	}
}






static CCattlesMsgHandler s_msgHandler;  // 消息处理模块实例

int CCattlesSrv::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("run game service name = %s, id = %d", name, id);
	return 0;
}

void CCattlesSrv::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("stop game service name = %s, id = %d", name, id);
}

void CCattlesSrv::onRegister(const char* name, const unsigned int id)
{
	// 注册模块实例
	const unsigned short HandlerMessageModuleId = 0;
	registerModule(HandlerMessageModuleId, &s_msgHandler);
	
	ReleaseInfoLog("register game module, service name = %s, id = %d", name, id);
}

void CCattlesSrv::onUpdateConfig(const char* name, const unsigned int id)
{
	s_msgHandler.updateConfig();
}

// 通知逻辑层对应的逻辑连接已被关闭
void CCattlesSrv::onCloseConnectProxy(void* userData, int cbFlag)
{
	s_msgHandler.onCloseConnectProxy(userData, cbFlag);
}


CCattlesSrv::CCattlesSrv() : IService(CattlesGame)
{
}

CCattlesSrv::~CCattlesSrv()
{
}

REGISTER_SERVICE(CCattlesSrv);

}

