
/* author : limingfan
 * date : 2017.09.21
 * description : 游戏大厅服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "CGameHall.h"
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


// 客户端版本标识
enum EClientVersionFlag
{
	ENormalVersion = 0,               // 正常的当前版本
	ECheckVersion = 1,                // 审核版本
};


// 游戏服务排序函数
bool sortGameService(const string& a, const string& b)
{
	com_protocol::GameServiceInfo srvInfo;
	const unsigned int aID = srvInfo.ParseFromArray(a.c_str(), a.length()) ? srvInfo.id() : 0;
	const unsigned int bID = srvInfo.ParseFromArray(b.c_str(), b.length()) ? srvInfo.id() : 0;

	return aID < bID;
}


// 版本号信息验证
int CVersionManager::getVersionInfo(const unsigned int deviceType, const unsigned int platformType, const string& curVersion, unsigned int& flag, string& newVersion, string& newFileURL)
{
	const HallConfigData::HallConfig& cfg = HallConfigData::HallConfig::getConfigValue();
	const map<int, HallConfigData::client_version>* client_version_list = NULL;
	if (deviceType == com_protocol::EMobileOSType::EAndroid)
		client_version_list = &cfg.android_version_list;
	else if (deviceType == com_protocol::EMobileOSType::EAppleIOS)
		client_version_list = &cfg.apple_version_list;
	else
		return GameHallVersionMobileOSTypeError;
	
	map<int, HallConfigData::client_version>::const_iterator verIt = client_version_list->find(platformType);
	if (verIt == client_version_list->end()) return GameHallVersionPlatformTypeError;
	
	// flag 版本号标识结果
	//	enum ECheckVersionResult
	//	{
	//		EValidVersion = 1;        // 可用版本
	//		EInValidVersion = 0;      // 无效的不可用版本，必须下载最新版本才能进入游戏
	//  }

	const HallConfigData::client_version& cVer = verIt->second;
	newVersion = cVer.version;
	newFileURL = cVer.url;
	flag = ((curVersion < cVer.valid) || (curVersion > newVersion)) ? 0 : 1;        // 正式版本号
	if (flag == 0)
	{
		flag = (cVer.check_flag != 1 || curVersion != cVer.check_version) ? 0 : 1;  // 验证是否是审核版本号
		if (flag == 1) newVersion = cVer.check_version;                             // 最新版本号修改为审核版本号
	}

	OptInfoLog("client version data, ip = %s, port = %d, device type = %d, platform type = %d, current version = %s, valid version = %s, flag = %d, new version = %s, file url = %s, check flag = %d, version = %s",
	CSocket::toIPStr(m_msgHandler->getConnectProxyContext().conn->peerIp), m_msgHandler->getConnectProxyContext().conn->peerPort,
	deviceType, platformType, curVersion.c_str(), cVer.valid.c_str(), flag, newVersion.c_str(), newFileURL.c_str(), cVer.check_flag, cVer.check_version.c_str());
	
	return Success;
}


CSrvMsgHandler::CSrvMsgHandler() : m_srvOpt(sizeof(HallUserData))
{
	memset(&m_hallSrvData, 0, sizeof(m_hallSrvData));
	
	m_getGameDataLastTimeSecs = 0;
	m_getGameDataIntervalSecs = 30;
	m_gameServiceValidIntervalSecs = 180;
	
	m_pCfg = NULL;
}

CSrvMsgHandler::~CSrvMsgHandler()
{
	memset(&m_hallSrvData, 0, sizeof(m_hallSrvData));

	m_getGameDataLastTimeSecs = 0;
	m_getGameDataIntervalSecs = 0;
	m_gameServiceValidIntervalSecs = 0;
	
	m_pCfg = NULL;
}


void CSrvMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("register protocol handler, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
	
	// 注册协议处理函数
	// 收到的客户端请求消息
	registerProtocol(OutsideClientSrv, CGH_USER_LOGIN_REQ, (ProtocolHandler)&CSrvMsgHandler::login);
	registerProtocol(DBProxySrv, DBPROXY_CHECK_USER_RSP, (ProtocolHandler)&CSrvMsgHandler::loginReply);
	registerProtocol(HttpOperationSrv, HTTPOPT_WX_LOGIN_RSP, (ProtocolHandler)&CSrvMsgHandler::wxLoginReply);
	registerProtocol(OutsideClientSrv, CGH_USER_LOGOUT_REQ, (ProtocolHandler)&CSrvMsgHandler::logout);
	
	registerProtocol(OutsideClientSrv, CGH_USER_ENTER_HALL_REQ, (ProtocolHandler)&CSrvMsgHandler::enterHall);
	registerProtocol(DBProxySrv, DBPROXY_ENTER_CHESS_HALL_RSP, (ProtocolHandler)&CSrvMsgHandler::enterHallReply);
	registerProtocol(OutsideClientSrv, CGH_USER_LEAVE_HALL_REQ, (ProtocolHandler)&CSrvMsgHandler::leaveHall);

	registerProtocol(CommonSrv, SERVICE_MANAGER_GIVE_GOODS_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::giveGoodsNotify);
	registerProtocol(OutsideClientSrv, CGH_PLAYER_CONFIRM_GIVE_GOODS_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::playerConfirmGiveGoods);

    registerProtocol(OutsideClientSrv, CGH_GET_HALL_BASE_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::getHallBaseInfo);
	registerProtocol(DBProxySrv, DBPROXY_GET_HALL_BASE_INFO_RSP, (ProtocolHandler)&CSrvMsgHandler::getHallBaseInfoReply);

	registerProtocol(OutsideClientSrv, CGH_GET_CHESS_HALL_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::getHallInfo);
	registerProtocol(DBProxySrv, DBPROXY_GET_CHESS_HALL_INFO_RSP, (ProtocolHandler)&CSrvMsgHandler::getHallInfoReply);

	registerProtocol(OutsideClientSrv, CGH_GET_GAME_ROOM_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::getHallRoomList);
	registerProtocol(DBProxySrv, DBPROXY_GET_HALL_ROOM_LIST_RSP, (ProtocolHandler)&CSrvMsgHandler::getHallRoomListReply);
    
    registerProtocol(OutsideClientSrv, CGH_GET_HALL_RANKING_REQ, (ProtocolHandler)&CSrvMsgHandler::getHallRanking);
	registerProtocol(DBProxySrv, DBPROXY_GET_HALL_RANKING_RSP, (ProtocolHandler)&CSrvMsgHandler::getHallRankingReply);
	
	registerProtocol(OutsideClientSrv, CGH_GET_USER_DATA_REQ, (ProtocolHandler)&CSrvMsgHandler::getUserInfo);
	registerProtocol(GameHallSrv, SGH_CUSTOM_GET_USER_INFO_ID, (ProtocolHandler)&CSrvMsgHandler::getUserInfoReply);
	
	registerProtocol(MessageCenterSrv, MSGCENTER_UPDATE_SERVICE_DATA_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::updateServiceDataNotify);
}

void CSrvMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!m_srvOpt.initEx(this, CCfg::getValue("GameHall", "ConfigFile")))
	{
		OptErrorLog("init service operation instance error");
		stopService();
		return;
	}

    m_srvOpt.setCloseRepeateNotifyProtocol(SERVICE_CLOSE_REPEATE_CONNECT_NOTIFY, CGH_CLOSE_REPEATE_CONNECT_NOTIFY);

	m_srvOpt.cleanUpConnectProxy(m_srvMsgCommCfg);  // 清理连接代理，如果服务异常退出，则启动时需要先清理连接代理
	
	m_pCfg = &HallConfigData::HallConfig::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str());
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
		OptErrorLog("hall service connect center redis service failed, ip = %s, port = %u, time out = %u",
		dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port, dbCfg.redis_db_cfg.center_db_timeout);
		
		stopService();
		return;
	}
	
	int rc = m_hallLogicHandler.init(this);
	if (rc != SrvOptSuccess)
	{
		OptErrorLog("init hall logic handler error, rc = %d", rc);
		
		stopService();
		return;
	}
	
	// 把大厅的数据存储到redis
	unsigned int gameHallSrvId = getSrvId();
	m_hallSrvData.curTimeSecs = time(NULL);
	m_hallSrvData.currentPersons = 0;
	
	rc = m_redisDbOpt.setHField(GameHallListKey, GameHallListLen, (const char*)&gameHallSrvId, sizeof(gameHallSrvId), (const char*)&m_hallSrvData, sizeof(m_hallSrvData));
	if (rc != 0)
	{
		OptErrorLog("set hall service data to redis center service failed, rc = %d", rc);
		stopService();
		return;
	}
	
	// 定时保存数据到redis
	const char* secsTimeInterval = CCfg::getValue("GameHall", "SaveDataToDBInterval");
	const unsigned int saveIntervalSecs = (secsTimeInterval != NULL) ? atoi(secsTimeInterval) : 60;
	setTimer(saveIntervalSecs * MillisecondUnit, (TimerHandler)&CSrvMsgHandler::saveDataToRedis, 0, NULL, (unsigned int)-1);
	
	// 向redis获取游戏服务数据的时间间隔，单位秒
	secsTimeInterval = CCfg::getValue("GameHall", "GetGameDataInterval");
	if (secsTimeInterval != NULL) m_getGameDataIntervalSecs = atoi(secsTimeInterval);
	
	// 游戏服务检测的时间间隔，单位秒
	secsTimeInterval = CCfg::getValue("GameHall", "GameDataValidInterval");
	if (secsTimeInterval != NULL) m_gameServiceValidIntervalSecs = atoi(secsTimeInterval);
	else m_gameServiceValidIntervalSecs = m_getGameDataIntervalSecs * 2;

	// 输出配置值
	m_srvOpt.getServiceCommonCfg().output();
	m_srvOpt.getCattlesBaseCfg().output();
	m_pCfg->output();
}

void CSrvMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	m_hallLogicHandler.unInit();

    // 服务下线通知
	m_srvOpt.sendServiceStatusNotify(srvId, com_protocol::EServiceStatus::ESrvOffline, GameHallSrv, "GameHall");
	
	// 服务停止，玩家退出服务
	com_protocol::ClientLogoutNotify srvQuitNtf;
	srvQuitNtf.set_info("hall service quit");
	const char* quitNtfMsgData = m_srvOpt.serializeMsgToBuffer(srvQuitNtf, "hall service quit notify");

	// 服务关闭，所有在线用户退出登陆
	const IDToConnectProxys& userConnects = getConnectProxy();
	for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end(); ++it)
	{
		if (quitNtfMsgData != NULL) sendMsgToProxy(quitNtfMsgData, srvQuitNtf.ByteSize(), CGH_FORCE_PLAYER_QUIT_NOTIFY, it->second);

		// 这里不可以调用removeConnectProxy，如此会不断修改userConnects的值而导致本循环遍历it值错误
		userLogout(getProxyUserData(it->second), com_protocol::EUserQuitStatus::EServiceStop);
	}

	stopConnectProxy();  // 停止连接代理
	
	int rc = m_redisDbOpt.delHField(GameHallListKey, GameHallListLen, (const char*)&srvId, sizeof(srvId));
	if (rc < 0) OptErrorLog("delete game hall service data from redis center service failed, rc = %d", rc);

	m_redisDbOpt.disconnectSvr();
}

void CSrvMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 服务上线通知
	int rc = m_srvOpt.sendServiceStatusNotify(srvId, com_protocol::EServiceStatus::ESrvOnline, GameHallSrv, "GameHall");
	ReleaseInfoLog("run hall message handler service name = %s, id = %d, module = %d, rc = %d", srvName, srvId, moduleId, rc);
}

bool CSrvMsgHandler::onPreHandleMessage(const char* msgData, const unsigned int msgLen, const ConnectProxyContext& connProxyContext)
{
	if (connProxyContext.protocolId == CGH_USER_LOGIN_REQ)
	{
		return !m_srvOpt.isRepeateCheckConnectProxy(CGH_FORCE_PLAYER_QUIT_NOTIFY);  // 检查是否存在重复登陆
	}

	// 检查发送消息的连接是否已成功通过验证，防止玩家验证成功之前胡乱发消息
    if (m_srvOpt.checkConnectProxyIsSuccess(CGH_FORCE_PLAYER_QUIT_NOTIFY)) return true;
	
	OptErrorLog("hall service receive not check success message, protocolId = %d", connProxyContext.protocolId);
	
	return false;
}

// 是否可以执行该操作
const char* CSrvMsgHandler::canToDoOperation(const int opt)
{
	if (opt == EServiceOperationType::EClientRequest && m_srvOpt.checkConnectProxyIsSuccess(CGH_FORCE_PLAYER_QUIT_NOTIFY))
	{
		HallUserData* ud = (HallUserData*)getProxyUserData(getConnectProxyContext().conn);
		return ud->userName;
	}
	
	return NULL;
}

// 获取玩家的聊天限制信息
SUserChatInfo& CSrvMsgHandler::getUserChatInfo(const string& username)
{
	return m_chatInfo[username];
}

void CSrvMsgHandler::sendApplyJoinHallNotify(const HallUserData* cud, const google::protobuf::RepeatedPtrField<string>& managers,
											 const com_protocol::PlayerApplyJoinChessHallNotify& notify)
{
	// 通知在线的B端、C端棋牌室管理员
	char msgBuffer[256] = {0};
	unsigned int msgBufferLen = sizeof(msgBuffer);
	if (NULL != m_srvOpt.serializeMsgToBuffer(notify, msgBuffer, msgBufferLen, "player apply join hall notify"))
	{
		// 通知B端管理员
		m_srvOpt.sendMsgToService(cud->userName, cud->userNameLen, ServiceType::OperationManageSrv,
								  msgBuffer, msgBufferLen, OPTMGR_PLAYER_APPLY_JOIN_HALL_NOTIFY);
								  
		// 通知C端在线的管理员
		for (google::protobuf::RepeatedPtrField<string>::const_iterator mgrIdIt = managers.begin();
			 mgrIdIt != managers.end(); ++mgrIdIt)
		{
			// C端的管理员是否在线
			ConnectProxy* mgrConn = getConnectProxy(*mgrIdIt);
			HallUserData* mgrCud = (HallUserData*)getProxyUserData(mgrConn);
			if (mgrCud != NULL && notify.hall_id() == mgrCud->hallId)
			{
				int rc = sendMsgToProxy(msgBuffer, msgBufferLen, CGH_PLAYER_APPLY_JOIN_HALL_NOTIFY, mgrConn);
				OptInfoLog("player apply join hall notify, mananger = %s, hallId = %s, rc = %d",
				mgrCud->userName, mgrCud->hallId, rc);
			}
		}
	}
}

bool CSrvMsgHandler::isProxyMsg()
{
	return (m_msgType == MessageType::ConnectProxyMsg);
}

CServiceOperationEx& CSrvMsgHandler::getSrvOpt()
{
	return m_srvOpt;
}

void CSrvMsgHandler::updateConfig()
{
	m_srvOpt.updateCommonConfig(CCfg::getValue("GameHall", "ConfigFile"));

	m_pCfg = &HallConfigData::HallConfig::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str(), true);
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

	m_hallLogicHandler.updateConfig();

	ReleaseInfoLog("update config business result = %d, service result = %d, cattles result = %d",
	m_pCfg->isSetConfigValueSuccess(), m_srvOpt.getServiceCommonCfg().isSetConfigValueSuccess(), m_srvOpt.getCattlesBaseCfg().isSetConfigValueSuccess());

	m_srvOpt.getServiceCommonCfg().output();
	m_srvOpt.getCattlesBaseCfg().output();
	m_pCfg->output();
}

// 通知逻辑层对应的逻辑连接代理已被关闭
void CSrvMsgHandler::onCloseConnectProxy(void* userData, int cbFlag)
{
	HallUserData* cud = (HallUserData*)userData;
	removeConnectProxy(string(cud->userName), 0, false);  // 可能存在连接被动关闭，所以这里要清除连接信息
    
	userLogout(cud, cbFlag);
}


// 定时保存数据到redis服务
void CSrvMsgHandler::saveDataToRedis(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	static const unsigned int srvId = getSrvId();
	m_hallSrvData.curTimeSecs = time(NULL);
	int rc = m_redisDbOpt.setHField(GameHallListKey, GameHallListLen, (const char*)&srvId, sizeof(srvId), (const char*)&m_hallSrvData, sizeof(m_hallSrvData));
	if (rc != 0) OptErrorLog("set game hall service data to redis center service failed, rc = %d", rc);
}

unsigned int CSrvMsgHandler::updateServiceData()
{
	// 刷新游戏数据
	const unsigned int currentSecs = time(NULL);
	if ((currentSecs - m_getGameDataLastTimeSecs) >= m_getGameDataIntervalSecs)
	{
		m_getGameDataLastTimeSecs = currentSecs;
		std::vector<std::string> strGameData;
		int rc = m_redisDbOpt.getHAll(GameServiceListKey, GameServiceListLen, strGameData);
		if (rc == 0)
		{
			m_strGameData.clear();
			for (unsigned int i = 1; i < strGameData.size(); i += 2) m_strGameData.push_back(strGameData[i]);  // 只提取value数据
			std::sort(m_strGameData.begin(), m_strGameData.end(), sortGameService);  // 游戏服务排序
		}
		else
		{
			OptErrorLog("get game service list from redis error, rc = %d", rc);
		}
	}
	
	return currentSecs;
}

bool CSrvMsgHandler::isNewVersionUser(const HallUserData* ud)
{
	if (ud->versionFlag == EClientVersionFlag::ECheckVersion) return true;  // 审核版本

	unordered_map<unsigned int, unsigned int>::const_iterator flagIt = m_pCfg->game_service_cfg.platform_update_flag.find(ud->platformType);
	return (flagIt != m_pCfg->game_service_cfg.platform_update_flag.end() && flagIt->second == 1);  // 强制更新版本
}

// 设置棋牌室信息
void CSrvMsgHandler::setChessHallInfo(const HallUserData* cud, com_protocol::ClientHallInfo& hallInfo)
{
    const unsigned int idIndex = NCommon::strToHashValue(hallInfo.base_info().id().c_str(), hallInfo.base_info().id().length());
    com_protocol::GameServiceInfoList gameSrvList;
    for (int idx = 0; idx < hallInfo.game_info_size();)
	{
		// 设置棋牌室游戏信息
		gameSrvList.clear_game_service();
		com_protocol::ClientHallGameInfo* gameInfo = hallInfo.mutable_game_info(idx);
        if (setHallGameInfo(cud, gameSrvList, idIndex, *gameInfo))
		{
			++idx;
		}
		else
		{
			hallInfo.mutable_game_info()->DeleteSubrange(idx, 1);  // 没有找到对应的游戏服务则删除
		}
	}
}

// 设置棋牌室游戏信息
bool CSrvMsgHandler::setHallGameInfo(const HallUserData* cud, com_protocol::GameServiceInfoList& gameSrvList,
                                     unsigned int idIndex, com_protocol::ClientHallGameInfo& gameInfo)
{
	if (getServicesFromRedis(cud, gameSrvList, gameInfo.game_type()))  // 根据类型查找服务
	{
		// 填写游戏信息
		map<unsigned int, ServiceCommonConfig::GameInfo>::const_iterator gCfgIt = m_srvOpt.getServiceCommonCfg().game_info_map.find(gameInfo.game_type());
		if (gCfgIt != m_srvOpt.getServiceCommonCfg().game_info_map.end())
		{
			gameInfo.set_game_name(gCfgIt->second.name);
			gameInfo.set_room_max_player(gCfgIt->second.room_max_player);
		}
		
		// 填写服务信息
		const com_protocol::GameServiceInfo& gSrvInfo = gameSrvList.game_service(idIndex % gameSrvList.game_service_size());
		gameInfo.set_service_ip(gSrvInfo.ip());
		gameInfo.set_service_port(gSrvInfo.port());
		gameInfo.set_service_id(gSrvInfo.id());
		
		// 房间列表的最后索引值，大于0表示还存在该类房间，小于0则表示该类房间已经遍历完毕
		if (gameInfo.room_info_size() < (int)m_pCfg->common_cfg.get_online_room_count) gameInfo.set_current_idx(-1);
		
		return true;
	}
	
	return false;
}

void CSrvMsgHandler::userLogin(ConnectProxy* conn, com_protocol::ClientLoginRsp& loginRsp)
{
	// 关闭和之前游戏服务的连接，该用户所在的之前的游戏服务必须退出
	int rc = SrvOptFailed;
	char reEnterId[IdMaxLen] = {0};
	GameServiceData gameSrvData;
	const int gameLen = m_redisDbOpt.getHField(GameUserKey, GameUserKeyLen, loginRsp.username().c_str(), loginRsp.username().length(), (char*)&gameSrvData, sizeof(gameSrvData));
	if (gameLen > 0)
	{
		if (gameSrvData.status == EGamePlayerStatus::EPlayRoom && gameSrvData.hallId > 0 && gameSrvData.roomId > 0)
		{
			// 掉线重连，游戏服务ID
			loginRsp.set_enter_service_id(gameSrvData.srvId);
			
			// 先房间ID
			snprintf(reEnterId, sizeof(reEnterId) - 1, "%u", gameSrvData.roomId);
			loginRsp.set_enter_room_id(reEnterId);

            // 后棋牌室ID
			snprintf(reEnterId, sizeof(reEnterId) - 1, "%u", gameSrvData.hallId);
			loginRsp.set_enter_hall_id(reEnterId);
			
			OptWarnLog("login check user reply and set reconnect info, user = %s, service id = %u, hall id = %u, room id = %u",
			loginRsp.username().c_str(), gameSrvData.srvId, gameSrvData.hallId, gameSrvData.roomId);
		}
		else
		{
			// 关闭之前的游戏服务连接
			rc = sendMessage(loginRsp.username().c_str(), loginRsp.username().length(), gameSrvData.srvId, SERVICE_CLOSE_REPEATE_CONNECT_NOTIFY);
			
			OptWarnLog("login check user reply and close game service repeate connect, user = %s, service id = %u, rc = %d",
			loginRsp.username().c_str(), gameSrvData.srvId, rc);
		}
	}
		
	// 检查是否存在重复登录
	SessionKeyData sessionKeyData;
	const int loginSKDLen = m_redisDbOpt.getHField(SessionKey, SessionKeyLen, loginRsp.username().c_str(), loginRsp.username().length(), (char*)&sessionKeyData, sizeof(sessionKeyData));
	if (loginSKDLen > 0)
	{
		// 关闭和登录服务器的连接
		ConnectProxy* vpConn = NULL;
		if (sessionKeyData.srvId != getSrvId())
		{
			rc = sendMessage(loginRsp.username().c_str(), loginRsp.username().length(), sessionKeyData.srvId, SERVICE_CLOSE_REPEATE_CONNECT_NOTIFY);
			
			OptWarnLog("login check user reply and close hall service repeate connect, user = %s, service id = %u, rc = %d",
		    loginRsp.username().c_str(), sessionKeyData.srvId, rc);
		}
		else
		{
			vpConn = getConnectProxy(loginRsp.username());
			if (vpConn == conn)  // 同一连接的重复登陆消息直接忽略掉
			{
				OptWarnLog("ignore repeat user login message, user name = %s, id = %d, ip = %s, port = %d",
				loginRsp.username().c_str(), conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort);
				
				return;
			}
			
			// 存在重复的登录连接就直接关闭
			m_srvOpt.closeRepeatConnectProxy(loginRsp.username().c_str(), loginRsp.username().length(), getSrvId(), 0, 0);
		}

		struct in_addr sdkAddrIp;
		sdkAddrIp.s_addr = sessionKeyData.ip;
		OptInfoLog("get session key info, vpUser = %s, skd ip = %s, port = %d, time = %u, id = %u, service id = %u, game skd len = %d, id = %u, userData = %s, conn id = %u, ip = %s, port = %d, conn = %p, vpConn = %p",
		loginRsp.username().c_str(), CSocket::toIPStr(sdkAddrIp), sessionKeyData.port, sessionKeyData.timeSecs, sessionKeyData.srvId, getSrvId(), gameLen, gameSrvData.srvId,
		getContext().userData, conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort, conn, vpConn);
	}
	else
	{
		OptInfoLog("get session key empty, vpUser = %s, skd len = %d, userData = %s, id = %u, ip = %s, port = %d",
		loginRsp.username().c_str(), loginSKDLen, getContext().userData, conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort);
	}

	// 成功则替换连接代理关联信息，变更连接ID为用户名
	HallUserData* cud = (HallUserData*)m_srvOpt.checkConnectProxySuccess(loginRsp.username(), conn);
	strncpy(cud->nickname, loginRsp.detail_info().static_info().nickname().c_str(), IdMaxLen - 1);
	strncpy(cud->portraitId, loginRsp.detail_info().static_info().portrait_id().c_str(), PortraitIdMaxLen - 1);
	cud->gender = loginRsp.detail_info().static_info().gender();
	
	*cud->hallId = '\0';
	cud->playerStatus = com_protocol::EHallPlayerStatus::EOutHallPlayer;

	if (*reEnterId != '\0')
	{
		// 掉线重连，玩家重入游戏房间
		rc = userEnterHall(conn, cud, reEnterId, com_protocol::EHallPlayerStatus::ECheckAgreedPlayer, NULL, loginRsp.detail_info());
		if (rc != SrvOptSuccess)
		{
			loginRsp.Clear();
			loginRsp.set_result(rc);
		}
	}
	
	if (loginRsp.result() == SrvOptSuccess)
	{
		// 设置会话ID
		char sessionKeyValue[IdMaxLen] = {0};
		sessionKeyData.ip = conn->peerIp.s_addr;
		sessionKeyData.port = conn->peerPort;
		sessionKeyData.srvId = getSrvId();
		getSessionKey(sessionKeyData, sessionKeyValue);
		loginRsp.set_session_key(sessionKeyValue);

		++m_hallSrvData.currentPersons;
	}

	rc = m_srvOpt.sendMsgPkgToProxy(loginRsp, CGH_USER_LOGIN_RSP, conn, "user login reply");

    OptInfoLog("send login result to client, username = %s, platfrom id = %s, result = %d, rc = %d, online persons = %d, connect id = %u, flag = %d, ip = %s, port = %d",
	cud->userName, loginRsp.platform_id().c_str(), loginRsp.result(), rc, m_hallSrvData.currentPersons, conn->proxyId, conn->proxyFlag, CSocket::toIPStr(conn->peerIp), conn->peerPort);
}

// 通知连接logout操作
void CSrvMsgHandler::userLogout(void* cb, int flag)
{
	HallUserData* cud = (HallUserData*)cb;
	if (cud == NULL)
	{
		OptErrorLog("logout notify but user data is null, flag = %d", flag);
		return;
	}

	if (m_srvOpt.isCheckSuccessConnectProxy(cud))
	{
		userLeaveHall(cud);

        if (m_hallSrvData.currentPersons > 0) --m_hallSrvData.currentPersons;

		m_chatInfo.erase(cud->userName);
	}
	
	OptInfoLog("user logout, name = %s, status = %d, flag = %d, online time = %u, current persons = %u",
    cud->userName, cud->status, flag, time(NULL) - cud->timeSecs, m_hallSrvData.currentPersons);
	
	m_srvOpt.destroyConnectProxyUserData(cud);
}

int CSrvMsgHandler::userEnterHall(ConnectProxy* conn, HallUserData* cud, const char* hallId, int playerStatus,
								  com_protocol::ClientHallInfo* hallInfo, const com_protocol::DetailInfo& userDetailInfo)
{
	// 设置玩家的会话数据
	SessionKeyData sessionKeyData;
	if (setSessionKeyData(conn, cud, hallId, playerStatus, sessionKeyData, cud->userName) != SrvOptSuccess) return GameHallSetSessionKeyFailed;

    cud->playerStatus = playerStatus;
	if (hallId != NULL) strncpy(cud->hallId, hallId, IdMaxLen - 1);
	if (hallInfo != NULL) setChessHallInfo(cud, *hallInfo); // 设置棋牌室信息

    // 进入棋牌室，上线通知
	m_srvOpt.sendUserOnlineNotify(cud->userName, getSrvId(), CSocket::toIPStr(conn->peerIp), cud->hallId);
	
	// 最后才回调玩家上线通知
	onLine(cud, conn, userDetailInfo);
	
	return SrvOptSuccess;
}

int CSrvMsgHandler::userLeaveHall(HallUserData* cud)
{
	// 删除玩家的会话数据
	int rc = m_redisDbOpt.delHField(SessionKey, SessionKeyLen, cud->userName, cud->userNameLen);
	if (rc < 0) OptErrorLog("user leave hall remove session key error, username = %s, rc = %d", cud->userName, rc);
	
	if (*cud->hallId == '\0') return SrvOptSuccess;
	
	// 回调玩家下线通知
	offLine(cud);

    // 离开棋牌室，下线通知
	m_srvOpt.sendUserOfflineNotify(cud->userName, getSrvId(), cud->hallId, "",
	                               com_protocol::EUserQuitStatus::EUserOffline, "", time(NULL) - cud->timeSecs);
								   
	OptInfoLog("user leave hall, username = %s, hallId = %s, player status = %d",
	cud->userName, cud->hallId, cud->playerStatus);

    *cud->hallId = '\0';
    cud->playerStatus = com_protocol::EHallPlayerStatus::EOutHallPlayer;
	
	return rc;
}

void CSrvMsgHandler::onLine(HallUserData* cud, ConnectProxy* conn, const com_protocol::DetailInfo& userDetailInfo)
{
    m_hallLogicHandler.onLine(cud, conn, userDetailInfo);
}

void CSrvMsgHandler::offLine(HallUserData* cud)
{
    m_hallLogicHandler.offLine(cud);
}

bool CSrvMsgHandler::getServicesFromRedis(const HallUserData* ud, com_protocol::GameServiceInfoList& gameSrvList, const unsigned int srvType)
{
	const unsigned int originalSrvFlag = 0;  // 当前运行的原服务
	const unsigned int newSrvFlag = 1;       // 现在升级的新服务

	const unsigned int currentSecs = updateServiceData();     // 刷新游戏数据
	const bool isNewVersionUserValue = isNewVersionUser(ud);  // 审核版本或者是强制更新版本
	com_protocol::GameServiceInfo srvData;
	for (unsigned int i = 0; i < m_strGameData.size(); ++i)
	{
		if (m_srvOpt.parseMsgFromBuffer(srvData, m_strGameData[i].data(), m_strGameData[i].length(), "get game service data")
		    && (currentSecs - srvData.update_timestamp()) < m_gameServiceValidIntervalSecs  // 有效时间内
			&& (srvData.id() / ServiceIdBaseValue) == srvType)
		{
			if ((!isNewVersionUserValue && srvData.flag() == originalSrvFlag)
				|| (isNewVersionUserValue && srvData.flag() == newSrvFlag))
			{
				com_protocol::GameServiceInfo* pSrvData = gameSrvList.add_game_service();
				*pSrvData = srvData;
			}
		}
	}

	return gameSrvList.game_service_size() > 0;
}

// 设置玩家的会话数据
int CSrvMsgHandler::setSessionKeyData(const ConnectProxy* conn, const HallUserData* cud, const char* hallId, int status,
                                      SessionKeyData& sessionKeyData, const string& saveUserName)
{
	sessionKeyData.ip = conn->peerIp.s_addr;
	sessionKeyData.port = conn->peerPort;
	sessionKeyData.srvId = getSrvId();
	sessionKeyData.timeSecs = cud->timeSecs;
	sessionKeyData.userVersion = isNewVersionUser(cud) ? 1 : 0;
	sessionKeyData.hallId = (hallId != NULL) ? atoi(hallId) : 0;
	sessionKeyData.status = status;
	
	int rc = SrvOptSuccess;
	if (!saveUserName.empty())
	{
		// 存储到redis服务
		rc = m_redisDbOpt.setHField(SessionKey, SessionKeyLen, saveUserName.c_str(), saveUserName.length(), (const char*)&sessionKeyData, sizeof(sessionKeyData));
		if (rc != SrvOptSuccess)
		{
			OptErrorLog("set session key to redis error, rc = %d, username = %s", rc, saveUserName.c_str());
			rc = GameHallSetSessionKeyFailed;
		}
	}
	
	return rc;
}

// 发送消息给棋牌室所有在线用户
void CSrvMsgHandler::sendMsgDataToAllHallUser(const char* data, const unsigned int len, const char* hallId, unsigned short protocolId)
{
	if (hallId == NULL || *hallId == '\0') return;

	const HallUserData* cud = NULL;
	const IDToConnectProxys& userConnects = getConnectProxy();  // 所有当前在线玩家
	for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end(); ++it)
	{
		cud = (const HallUserData*)getProxyUserData(it->second);
		if (m_srvOpt.isCheckSuccessConnectProxy(cud) && strcmp(cud->hallId, hallId) == 0)
		{
			sendMsgToProxy(data, len, protocolId, it->second);
		}
	}
}

// 管理员赠送物品通知
void CSrvMsgHandler::giveGoodsNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* conn = m_srvOpt.getConnectProxyInfo(userName, "give goods notify");
	if (conn != NULL) sendMsgToProxy(data, len, CGH_MANAGER_GIVE_GOODS_NOTIFY, conn);
	
	OptInfoLog("receive give goods notify, username = %s, online = %d", userName, (conn != NULL));
}

// 管理员赠送物品，玩家确认收货
void CSrvMsgHandler::playerConfirmGiveGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    HallUserData* cud = (HallUserData*)m_srvOpt.getConnectProxyUserData();
	m_srvOpt.sendMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, data, len, DBPROXY_PLAYER_CONFIRM_GIVE_GOODS_NOTIFY);
}

void CSrvMsgHandler::login(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 先初始化连接
	HallUserData* curCud = (HallUserData*)getProxyUserData(getConnectProxyContext().conn);
	if (curCud == NULL) curCud = (HallUserData*)m_srvOpt.createConnectProxyUserData(getConnectProxyContext().conn);

	com_protocol::ClientLoginReq loginReq;
	if (!m_srvOpt.parseMsgFromBuffer(loginReq, data, len, "user login")) return;
	
	com_protocol::ClientLoginRsp loginRsp;
	loginRsp.set_result(SrvOptSuccess);
	
	// 客户端版本号验证
	while (loginReq.has_version())
	{
		// 检查版本号是否可用
		const unsigned int deviceType = loginReq.os_type();
		const map<int, HallConfigData::client_version>* client_version_list = NULL;
		if (deviceType == com_protocol::EMobileOSType::EAndroid)
			client_version_list = &m_pCfg->android_version_list;
		else if (deviceType == com_protocol::EMobileOSType::EAppleIOS)
			client_version_list = &m_pCfg->apple_version_list;
		else
		{
			loginRsp.set_result(GameHallVersionMobileOSTypeError);
			break;
		}

		map<int, HallConfigData::client_version>::const_iterator verIt = client_version_list->find(loginReq.platform_type());
		if (verIt == client_version_list->end())
		{
			loginRsp.set_result(GameHallVersionPlatformTypeError);
			break;
		}
		
		const HallConfigData::client_version& cVer = verIt->second;
		if ((loginReq.version() >= cVer.valid && loginReq.version() <= cVer.version)  // 正式版本号
			|| (cVer.check_flag == 1 && loginReq.version() == cVer.check_version))    // 校验测试版本号
		{
			if (loginReq.version() > cVer.version) curCud->versionFlag = EClientVersionFlag::ECheckVersion;
		}
		else
		{
			loginRsp.set_result(GameHallClientVersionInvalid);
			break;
		}
		
		break;
	}
	
	// 第三方登录信息验证
	if (loginRsp.result() == SrvOptSuccess)
	{
		if (loginReq.platform_type() == EClientGameWeiXin)  // 微信登录&验证
		{
			com_protocol::WXLoginReq wxLoginReq;
			wxLoginReq.set_platform_type(EClientGameWeiXin);
			wxLoginReq.set_cb_data(data, len);
			
			if (loginReq.has_username()) wxLoginReq.set_username(loginReq.username());
			if (loginReq.has_platform_id()) wxLoginReq.set_platform_id(loginReq.platform_id());
			if (loginReq.third_check_data().kv_size() > 0) wxLoginReq.set_code(loginReq.third_check_data().kv(0).value());
			
			int rc = m_srvOpt.sendPkgMsgToService(curCud->userName, curCud->userNameLen, ServiceType::HttpOperationSrv, wxLoginReq, HTTPOPT_WX_LOGIN_REQ, "wx login request");
			OptInfoLog("receive message to wx login, username = %s, platform type = %u, version = %s, login id = %s, userName = %s, status = %d, time = %u, rc = %d",
			loginReq.username().c_str(), loginReq.platform_type(), loginReq.version().c_str(),
			loginReq.login_id().c_str(), curCud->userName, curCud->status, curCud->timeSecs, rc);
			
			return;
		}
	}
	
	if (loginRsp.result() == SrvOptSuccess)
	{
		curCud->platformType = loginReq.platform_type();

		loginReq.set_login_ip(CSocket::toIPStr(getConnectProxyContext().conn->peerIp));  // 登录IP

		unsigned int dbProxySrvId = 0;
		NProject::getDestServiceID(ServiceType::DBProxySrv, loginReq.username().c_str(), loginReq.username().length(), dbProxySrvId);
		int rc = m_srvOpt.sendPkgMsgToService(curCud->userName, curCud->userNameLen, loginReq, dbProxySrvId, DBPROXY_CHECK_USER_REQ, "user login request");

		OptInfoLog("receive message to login, ip = %s, port = %d, username = %s, platform type = %u, version = %s, login id = %s, userName = %s, status = %d, time = %u, rc = %d",
		loginReq.login_ip().c_str(), getConnectProxyContext().conn->peerPort, loginReq.username().c_str(), loginReq.platform_type(), loginReq.version().c_str(),
		loginReq.login_id().c_str(), curCud->userName, curCud->status, curCud->timeSecs, rc);
	}
	
	else
	{
		int rc = m_srvOpt.sendMsgPkgToProxy(loginRsp, CGH_USER_LOGIN_RSP, "user login check error");

		OptErrorLog("receive message to login but check error, ip = %s, user name = %s, platform type = %u, device = %d, version = %s, result = %d, rc = %d",
		CSocket::toIPStr(getConnectProxyContext().conn->peerIp), loginReq.username().c_str(), loginReq.platform_type(), loginReq.os_type(), loginReq.version().c_str(), loginRsp.result(), rc);
		
		// 验证失败可以考虑直接关闭该连接了
	}
}

void CSrvMsgHandler::loginReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(string(getContext().userData));
	if (conn == NULL)
	{
		OptErrorLog("login check user reply can not find the connect data, user data = %s", getContext().userData);
		return;
	}

	com_protocol::ClientLoginRsp loginRsp;
	if (!m_srvOpt.parseMsgFromBuffer(loginRsp, data, len, "login check user reply")) return;
	
	if (loginRsp.result() != SrvOptSuccess)
	{
		sendMsgToProxy(data, len, CGH_USER_LOGIN_RSP, conn);
		return;
	}
	
	userLogin(conn, loginRsp);
}

void CSrvMsgHandler::wxLoginReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(string(getContext().userData));
	if (conn == NULL)
	{
		OptErrorLog("wei xin login reply can not find the connect data, user data = %s", getContext().userData);
		return;
	}
	
	com_protocol::WXLoginRsp wxLoginRsp;
	if (!m_srvOpt.parseMsgFromBuffer(wxLoginRsp, data, len, "wei xin login reply")) return;

	if (wxLoginRsp.result() == SrvOptSuccess)
	{
		com_protocol::ClientLoginReq loginReq;
	    if (!m_srvOpt.parseMsgFromBuffer(loginReq, wxLoginRsp.cb_data().c_str(), wxLoginRsp.cb_data().length(), "wei xin reply cb data")) return;
	
	    HallUserData* curCud = (HallUserData*)getProxyUserData(conn);
		curCud->platformType = loginReq.platform_type();

		loginReq.set_platform_id(wxLoginRsp.platform_id());
		loginReq.set_user_input_nickname(wxLoginRsp.user_static_info().nickname());
		loginReq.set_user_input_portrait_id(wxLoginRsp.user_static_info().portrait_id());
		loginReq.set_user_input_gender(wxLoginRsp.user_static_info().gender());
		
		loginReq.set_login_ip(CSocket::toIPStr(conn->peerIp));  // 登录IP

		unsigned int dbProxySrvId = 0;
		NProject::getDestServiceID(ServiceType::DBProxySrv, loginReq.username().c_str(), loginReq.username().length(), dbProxySrvId);
		int rc = m_srvOpt.sendPkgMsgToService(curCud->userName, curCud->userNameLen, loginReq, dbProxySrvId, DBPROXY_CHECK_USER_REQ, "user login request");

		OptInfoLog("wei xin login reply, ip = %s, port = %d, username = %s, platform type = %u, id = %s, version = %s, login id = %s, userName = %s, status = %d, time = %u, rc = %d",
		loginReq.login_ip().c_str(), conn->peerPort, loginReq.username().c_str(), loginReq.platform_type(), loginReq.platform_id().c_str(), loginReq.version().c_str(),
		loginReq.login_id().c_str(), curCud->userName, curCud->status, curCud->timeSecs, rc);
		
		return;
	}
	
	com_protocol::ClientLoginRsp loginRsp;
	loginRsp.set_result(wxLoginRsp.result());
	int rc = m_srvOpt.sendMsgPkgToProxy(loginRsp, CGH_USER_LOGIN_RSP, conn, "wei xin user login check error");

	OptErrorLog("receive message to do wei xin login but check error, ip = %s, port = %d, result = %d, rc = %d",
	CSocket::toIPStr(conn->peerIp), conn->peerPort, loginRsp.result(), rc);
	
	// 验证失败可以考虑直接关闭该连接了
}

void CSrvMsgHandler::logout(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	sendMsgToProxy(data, len, CGH_USER_LOGOUT_RSP);

    HallUserData* ud = (HallUserData*)getProxyUserData(getConnectProxyContext().conn);
	removeConnectProxy(string(ud->userName), com_protocol::EUserQuitStatus::EUserOffline);  // 删除关闭与客户端的连接
}

void CSrvMsgHandler::enterHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientEnterHallReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "enter hall request")) return;
	
	ConnectProxy* conn = getConnectProxyContext().conn;
	req.set_login_ip(CSocket::toIPStr(conn->peerIp));  // 登录IP
	req.set_max_online_rooms(m_pCfg->common_cfg.get_online_room_count);         // 获取的最大在线房间数量
	
	HallUserData* curCud = (HallUserData*)getProxyUserData(conn);
	if (*curCud->hallId != '\0') userLeaveHall(curCud);  // 玩家没有正常退出之前的棋牌室则这里主动退出
	
	int rc = m_srvOpt.sendPkgMsgToService(curCud->userName, curCud->userNameLen, ServiceType::DBProxySrv, req, DBPROXY_ENTER_CHESS_HALL_REQ, "user enter hall request");

	OptInfoLog("receive message to enter hall, username = %s, rc = %d", curCud->userName, rc);
}

void CSrvMsgHandler::enterHallReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(getContext().userData);
	if (conn == NULL)
	{
		OptErrorLog("user enter hall reply can not find the connect data, user data = %s", getContext().userData);
		return;
	}
	
	com_protocol::ClientEnterHallRsp rsp;
	if (!m_srvOpt.parseMsgFromBuffer(rsp, data, len, "enter hall reply")) return;
	
	int rc = SrvOptFailed;
	const char* hallId = NULL;
	int playerStatus = com_protocol::EHallPlayerStatus::EOutHallPlayer;
	HallUserData* cud = (HallUserData*)getProxyUserData(conn);
	if (rsp.result() == SrvOptSuccess)
	{
		// 玩家相关的棋牌室ID&状态
		if (rsp.has_hall_info())
		{
			hallId = rsp.hall_info().base_info().id().c_str();
			playerStatus = rsp.hall_info().base_info().player_status();
		}

        rc = userEnterHall(conn, cud, hallId, playerStatus, (hallId != NULL) ? rsp.mutable_hall_info() : NULL, rsp.detail_info());
		if (rc != SrvOptSuccess)
		{
			rsp.Clear();
			rsp.set_result(rc);
		}
	}

	rc = m_srvOpt.sendMsgPkgToProxy(rsp, CGH_USER_ENTER_HALL_RSP, conn, "enter hall reply");
	
	if (rsp.result() == SrvOptSuccess && rsp.is_apply() == 1
	    && playerStatus == com_protocol::EHallPlayerStatus::EWaitForCheck)
	{
		// 申请加入棋牌室成功则同步通知在线的B端、C端棋牌室管理员
		com_protocol::PlayerApplyJoinChessHallNotify notify;
		notify.set_allocated_static_info(rsp.mutable_detail_info()->release_static_info());
		notify.set_hall_id(hallId);
		if (rsp.has_explain_msg()) notify.set_allocated_explain_msg(rsp.release_explain_msg());
		
		sendApplyJoinHallNotify(cud, rsp.managers(), notify);
	}

	OptInfoLog("user enter hall reply, result = %d, hall id = %s, username = %s, status = %d, is apply = %d, rc = %d",
	rsp.result(), rsp.hall_info().base_info().id().c_str(), getContext().userData, playerStatus, rsp.is_apply(), rc);
}

void CSrvMsgHandler::leaveHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientLeaveHallRsp rsp;
	rsp.set_result(SrvOptSuccess);
	m_srvOpt.sendMsgPkgToProxy(rsp, CGH_USER_LEAVE_HALL_RSP, "user leave hall reply");
	
	userLeaveHall((HallUserData*)m_srvOpt.getConnectProxyUserData());
}

void CSrvMsgHandler::getHallBaseInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    // 获取棋牌室基本信息
    HallUserData* cud = (HallUserData*)m_srvOpt.getConnectProxyUserData();
	int rc = m_srvOpt.sendMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, data, len, DBPROXY_GET_HALL_BASE_INFO_REQ);
	OptInfoLog("get hall base info request, username = %s, rc = %d", cud->userName, rc);
}

void CSrvMsgHandler::getHallBaseInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(getContext().userData);
	if (conn == NULL)
	{
		OptErrorLog("get hall base info reply can not find the connect data, user data = %s", getContext().userData);
		return;
	}

    int rc = sendMsgToProxy(data, len, CGH_GET_HALL_BASE_INFO_RSP, conn);
	OptInfoLog("get hall base info reply, username = %s, rc = %d", getContext().userData, rc);
}

void CSrvMsgHandler::getHallInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetHallInfoReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "get hall info request")) return;

    // 获取棋牌室信息
    HallUserData* cud = (HallUserData*)m_srvOpt.getConnectProxyUserData();
	req.set_max_online_rooms(m_pCfg->common_cfg.get_online_room_count);
	req.set_username(cud->userName);
	
	int rc = m_srvOpt.sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
										  DBPROXY_GET_CHESS_HALL_INFO_REQ, "get hall info request");

	OptInfoLog("get chess hall info request, id = %s, username = %s, rc = %d", req.hall_id().c_str(), cud->userName, rc);
}

void CSrvMsgHandler::getHallInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(getContext().userData);
	if (conn == NULL)
	{
		OptErrorLog("get hall info reply can not find the connect data, user data = %s", getContext().userData);
		return;
	}
	
	com_protocol::ClientGetHallInfoRsp rsp;
	if (!m_srvOpt.parseMsgFromBuffer(rsp, data, len, "get hall info reply")) return;
	
	if (rsp.result() == SrvOptSuccess)
	{
		// 设置棋牌室信息
		setChessHallInfo((HallUserData*)m_srvOpt.getConnectProxyUserData(getContext().userData), *rsp.mutable_hall_info());
	}
	else
	{
		OptErrorLog("get hall info error, result = %d, user name = %s", rsp.result(), getContext().userData);
	}

	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, CGH_GET_CHESS_HALL_INFO_RSP, conn, "get hall info reply");

	OptInfoLog("get chess hall info reply, result = %d, id = %s, username = %s, rc = %d",
	rsp.result(), rsp.hall_info().base_info().id().c_str(), getContext().userData, rc);
}

void CSrvMsgHandler::getHallRoomList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetHallGameRoomReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "get hall room list request")) return;
	
	// 获取棋牌室游戏房间信息
	req.set_max_online_rooms(m_pCfg->common_cfg.get_online_room_count);
	HallUserData* cud = (HallUserData*)m_srvOpt.getConnectProxyUserData();
	int rc = m_srvOpt.sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
										  DBPROXY_GET_HALL_ROOM_LIST_REQ, "get hall room list request");

	OptInfoLog("get chess hall room list request, hall id = %s, username = %s, game type = %u, idx = %d, rc = %d",
	req.hall_id().c_str(), cud->userName, req.game_type(), req.current_idx(), rc);
}

void CSrvMsgHandler::getHallRoomListReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* conn = m_srvOpt.getConnectProxyInfo(userName, "get hall room list reply");
	if (conn == NULL) return;
	
	com_protocol::ClientGetHallGameRoomRsp rsp;
	if (!m_srvOpt.parseMsgFromBuffer(rsp, data, len, "get hall room list reply")) return;
	
	if (rsp.result() == SrvOptSuccess)
	{
		const unsigned int idIndex = NCommon::strToHashValue(rsp.hall_id().c_str(), rsp.hall_id().length());
		com_protocol::GameServiceInfoList gameSrvList;
		if (!setHallGameInfo((HallUserData*)getProxyUserData(conn), gameSrvList, idIndex, *rsp.mutable_game_room_info()))
		{
			rsp.set_result(GameHallNoFoundOnlineGameService);
			
			OptErrorLog("get hall room list reply can not find online service error, hall id = %s, game type = %d, username = %s",
	                    rsp.hall_id().c_str(), rsp.game_room_info().game_type(), userName);

			rsp.clear_game_room_info();
		}
	}
	
	int rc = m_srvOpt.sendMsgPkgToProxy(rsp, CGH_GET_GAME_ROOM_INFO_RSP, conn, "get hall room list reply");

	OptInfoLog("get hall room list reply, result = %d, hall id = %s, game type = %d, idx = %d, username = %s, rc = %d",
	rsp.result(), rsp.hall_id().c_str(), rsp.game_room_info().game_type(), rsp.game_room_info().current_idx(), userName, rc);
}

void CSrvMsgHandler::getHallRanking(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    HallUserData* cud = (HallUserData*)m_srvOpt.getConnectProxyUserData();
    if (*cud->hallId == '\0') return;

	com_protocol::ClientGetHallRankingReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "get hall ranking request")) return;
	
	req.set_hall_id(cud->hallId);
	m_srvOpt.sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
                                 DBPROXY_GET_HALL_RANKING_REQ, "get hall ranking request");
}

void CSrvMsgHandler::getHallRankingReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* conn = m_srvOpt.getConnectProxyInfo(userName, "get hall ranking reply");
	if (conn != NULL) sendMsgToProxy(data, len, CGH_GET_HALL_RANKING_RSP, conn);
}

void CSrvMsgHandler::getUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	HallUserData* cud = (HallUserData*)m_srvOpt.getConnectProxyUserData();
    if (*cud->hallId == '\0') return;

	com_protocol::ClientGetUserDataReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "get user info request")) return;
	
	com_protocol::GetUserDataReq getUserDataReq;
	getUserDataReq.set_username(req.username());
	getUserDataReq.set_info_type(com_protocol::EUserInfoType::EUserStaticDynamic);
	getUserDataReq.set_hall_id(cud->hallId);

	unsigned int dbProxySrvId = NProject::getDestServiceID(ServiceType::DBProxySrv, req.username().c_str(), req.username().length());
	m_srvOpt.sendPkgMsgToService(cud->userName, cud->userNameLen, getUserDataReq, dbProxySrvId,
								 DBPROXY_GET_USER_DATA_REQ, "get user info request", 0, SGH_CUSTOM_GET_USER_INFO_ID);
}

void CSrvMsgHandler::getUserInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* conn = m_srvOpt.getConnectProxyInfo(userName, "get user info reply");
	if (conn == NULL) return;

	com_protocol::GetUserDataRsp getUserDataRsp;
	if (!m_srvOpt.parseMsgFromBuffer(getUserDataRsp, data, len, "get user info reply")) return;
	
	com_protocol::ClientGetUserDataRsp rsp;
	rsp.set_result(getUserDataRsp.result());
	if (rsp.result() == SrvOptSuccess) rsp.set_allocated_detail_info(getUserDataRsp.release_detail_info());
	
	m_srvOpt.sendMsgPkgToProxy(rsp, CGH_GET_USER_DATA_RSP, conn, "get user info reply");
}

// 刷新服务数据通知
void CSrvMsgHandler::updateServiceDataNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ServiceUpdateDataNotify srvUpdateDataNtf;
	if (!m_srvOpt.parseMsgFromBuffer(srvUpdateDataNtf, data, len, "update service data notify")) return;

    // 目前只有房间数据刷新
	int rc = SrvOptFailed;
	ConnectProxy* conn = NULL;
	for (google::protobuf::RepeatedPtrField<string>::const_iterator it = srvUpdateDataNtf.notify_usernames().begin();
	     it != srvUpdateDataNtf.notify_usernames().end(); ++it)
    {
		conn = getConnectProxy(*it);
		if (conn != NULL) rc = sendMsgToProxy(srvUpdateDataNtf.data().c_str(), srvUpdateDataNtf.data().length(), CGH_UPDATE_HALL_GAME_ROOM_NOTIFY, conn);
    }
	
	OptInfoLog("update service data notify, hall id = %s, username = %s, notify count = %d, send msg rc = %d",
	srvUpdateDataNtf.hall_id().c_str(), srvUpdateDataNtf.src_username().c_str(), srvUpdateDataNtf.notify_usernames_size(), rc);
}





static CSrvMsgHandler s_msgHandler;  // 消息处理模块实例

int CGameHallSrv::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("run game hall service name = %s, id = %d", name, id);
	return 0;
}

void CGameHallSrv::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("stop game hall service name = %s, id = %d", name, id);
}

void CGameHallSrv::onRegister(const char* name, const unsigned int id)
{
	// 注册模块实例
	const unsigned short HandlerMessageModuleId = 0;
	registerModule(HandlerMessageModuleId, &s_msgHandler);
	
	ReleaseInfoLog("register game hall module, service name = %s, id = %d", name, id);
}

void CGameHallSrv::onUpdateConfig(const char* name, const unsigned int id)
{
	s_msgHandler.updateConfig();
}

// 通知逻辑层对应的逻辑连接已被关闭
void CGameHallSrv::onCloseConnectProxy(void* userData, int cbFlag)
{
	s_msgHandler.onCloseConnectProxy(userData, cbFlag);
}


CGameHallSrv::CGameHallSrv() : IService(GameHallSrv)
{
}

CGameHallSrv::~CGameHallSrv()
{
}

REGISTER_SERVICE(CGameHallSrv);

}

