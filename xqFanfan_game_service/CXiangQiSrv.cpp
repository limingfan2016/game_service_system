
/* author : limingfan
 * date : 2016.12.06
 * description : 象棋服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "CXiangQiSrv.h"
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
#define WriteDataLog(format, args...)     getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)


CSrvMsgHandler::CSrvMsgHandler() : m_memForUserData(MaxUserDataCount, MaxUserDataCount, sizeof(XQUserData)), m_localAsyncData(this, MaxLocalAsyncDataSize, LocalAsyncDataCount),
m_memForXQChessPieces(MaxUserDataCount, MaxUserDataCount, sizeof(XQChessPieces))
{
	m_msgBuffer[0] = '\0';
	m_playAgainCurrentId = 0;
}

CSrvMsgHandler::~CSrvMsgHandler()
{
	m_msgBuffer[0] = '\0';
	m_memForUserData.clear();
}

bool CSrvMsgHandler::checkLoginIsSuccess(const char* logInfo)
{
	// 未经过登录成功后的非法操作消息则直接关闭连接
	XQUserData* ud = (XQUserData*)getProxyUserData(getConnectProxyContext().conn);
	if (ud == NULL)
	{
		OptWarnLog("%s, can not find the user data and close connect, id = %d, ip = %s, port = %d",
		logInfo, getConnectProxyContext().conn->proxyId, CSocket::toIPStr(getConnectProxyContext().conn->peerIp), getConnectProxyContext().conn->peerPort);
		closeConnectProxy(getConnectProxyContext().conn);
		return false;
	}
	else if (ud->status != EEnterGame)
	{
		OptWarnLog("%s, close connect, user name = %s, status = %d, id = %d, ip = %s, port = %d",
		logInfo, ud->username, ud->status, getConnectProxyContext().conn->proxyId, CSocket::toIPStr(getConnectProxyContext().conn->peerIp), getConnectProxyContext().conn->peerPort);
		
		quitGame(ud->username, logInfo);
		return false;
	}
	
	return true;
}

const char* CSrvMsgHandler::serializeMsgToBuffer(const ::google::protobuf::Message& msg, const char* info)
{
	if (!msg.SerializeToArray(m_msgBuffer, MaxMsgLen))
	{
		OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), MaxMsgLen, info);
		return NULL;
	}
	return m_msgBuffer;
}

bool CSrvMsgHandler::parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info)
{
	if (!msg.ParseFromArray(buffer, len))
	{
		OptErrorLog("ParseFromArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), len, info);
		return false;
	}
	return true;
}

unsigned int CSrvMsgHandler::getCommonDbProxySrvId(const char* userName, const int len)
{
	// 要根据负载均衡选择commonDBproxy服务
	unsigned int srvId = 0;
	getDestServiceID(DBProxyCommonSrv, userName, len, srvId);
	return srvId;
}

int CSrvMsgHandler::sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userName, int uNLen)
{
	XQUserData* ud = (XQUserData*)getProxyUserData(getConnectProxyContext().conn);
	if (userName == NULL)
	{
	    userName = ud->username;
	    uNLen = ud->usernameLen;
	}
	return sendMessage(data, len, ud->username, ud->usernameLen, getCommonDbProxySrvId(userName, uNLen), protocolId);
}

int CSrvMsgHandler::sendMessageToService(NFrame::ServiceType srvType, const char* data, const unsigned int len, unsigned short protocolId, const char* userName, const int uNLen,
                                         int userFlag, unsigned short handleProtocolId)
{
	unsigned int srvId = 0;
	getDestServiceID(srvType, userName, uNLen, srvId);
	return sendMessage(data, len, userFlag, userName, uNLen, srvId, protocolId, 0, handleProtocolId);
}

CLogger& CSrvMsgHandler::getDataLogger()
{
	static CLogger dataLogger(CCfg::getValue("DataLogger", "File"), atoi(CCfg::getValue("DataLogger", "Size")), atoi(CCfg::getValue("DataLogger", "BackupCount")));
	return dataLogger;
}

CLocalAsyncData& CSrvMsgHandler::getLocalAsyncData()
{
	return m_localAsyncData;
}

CRobotManager& CSrvMsgHandler::getRobotManager()
{
	return m_robotManager;
}

void CSrvMsgHandler::updateConfig()
{

}

ConnectProxy* CSrvMsgHandler::getConnectInfo(const char* logInfo, string& userName)
{
	userName = getContext().userData;
	ConnectProxy* conn = getConnectProxy(getContext().userData);
	if (conn == NULL) OptErrorLog("%s, user data = %s", logInfo, getContext().userData);
	return conn;
}

// 通知逻辑层对应的逻辑连接已被关闭
void CSrvMsgHandler::onClosedConnect(void* userData)
{
	XQUserData* ud = (XQUserData*)userData;
	const EQuitGameMode mode = removeConnectProxy(ud->username, false) ? EQuitGameMode::EPassiveQuit : EQuitGameMode::EActivityQuit;
	quitGame(ud, mode);
}

// 退出游戏
void CSrvMsgHandler::quitGame(const char* username, const char* msg)
{
	OptInfoLog("quit game, user name = %s, msg = %s", username, msg);
	removeConnectProxy(username);
}

// 通知连接logout操作
void CSrvMsgHandler::quitGame(XQUserData* ud, EQuitGameMode mode)
{
	if (ud->status == EEnterGame)
	{
		int rc = m_redisDbOpt.delHField(NProject::GameServiceKey, NProject::GameServiceLen, ud->username, ud->usernameLen);
		if (rc < 0) OptErrorLog("delete GameServiceKey from redis error, username = %s, rc = %d", ud->username, rc);

        // 通知服务用户已经退出游戏
		com_protocol::UserQuitServiceNotify notifyMsgSrv;
		notifyMsgSrv.set_service_id(getSrvId());
		const char* msgData = serializeMsgToBuffer(notifyMsgSrv, "notify message service quit game");
		if (msgData != NULL) sendMessageToService(ServiceType::MessageSrv, msgData, notifyMsgSrv.ByteSize(), BUSINESS_USER_QUIT_SERVICE_NOTIFY, ud->username, ud->usernameLen);
		
		// 如果是正在等待的玩家则需要清空
		string& waitUser = m_waitForMatchingUsers[ud->playMode][ud->arenaId];
		if (waitUser == ud->username) waitUser.clear();
	
	    // 还没有结算就退出，则可能是逃跑或者网络掉线
	    XQChessPieces* xqCP = getXQChessPieces(ud->username);
		if (xqCP != NULL)
		{
			// 主动退出正在进行的游戏则算逃跑，非主动退出则网络掉线，否则是服务正常停止
			com_protocol::SettlementResult stResult = (mode == EQuitGameMode::EActivityQuit) ? com_protocol::SettlementResult::PlayerEscape : com_protocol::SettlementResult::PlayerDisconnect;
			if (mode == EQuitGameMode::EServiceUnload) stResult = com_protocol::SettlementResult::ServiceUnload;
			finishSettlement(xqCP, ud->sideFlag, stResult, ud);
		}
		
		destroyPlayAgainInfo(ud->playAgainId);

		ud->status = EQuitGame;
		offLine(ud);  // 下线
		
		saveUserLogicData(ud);  // 保存用户逻辑数据
	}
	
	OptInfoLog("quit game, user name = %s, status = %d, quit mode = %d", ud->username, ud->status, mode);
	
	ud->unInit();
	m_memForUserData.put((char*)ud);
}

void CSrvMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 注册协议处理函数
	// 收到的客户端请求消息
	registerProtocol(OutsideClientSrv, XIANGQISRV_CHECK_USER_REQ, (ProtocolHandler)&CSrvMsgHandler::checkUser);
	registerProtocol(OutsideClientSrv, XIANGQISRV_USER_QUIT_REQ, (ProtocolHandler)&CSrvMsgHandler::userQuitGame);
	registerProtocol(OutsideClientSrv, XIANGQISRV_GET_ARENA_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::getFFChessArenaInfo);
	registerProtocol(OutsideClientSrv, XIANGQISRV_ENTER_CHESS_ARENA_REQ, (ProtocolHandler)&CSrvMsgHandler::userEnterArena);
	registerProtocol(OutsideClientSrv, XIANGQISRV_LEAVE_CHESS_ARENA_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::userLeaveArena);
	registerProtocol(OutsideClientSrv, XIANGQISRV_ENTER_GAME_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::enterGame);
	registerProtocol(OutsideClientSrv, XIANGQISRV_READY_OPT_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::readyOptNotify);
	registerProtocol(OutsideClientSrv, XIANGQISRV_PLAY_CHESS_PIECES_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::playChessPiecesNotify);
	
	// 认输&求和
	registerProtocol(OutsideClientSrv, XIANGQISRV_PLAYER_DEFEATE_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::playerBeDefeateNotify);
	registerProtocol(OutsideClientSrv, XIANGQISRV_PLEASE_PEACE_REQ, (ProtocolHandler)&CSrvMsgHandler::playerPleasePeace);
	registerProtocol(OutsideClientSrv, XIANGQISRV_PLEASE_PEACE_CONFIRM, (ProtocolHandler)&CSrvMsgHandler::playerPleasePeaceConfirm);
	
	// 切换对手&再来一局
	registerProtocol(OutsideClientSrv, XIANGQISRV_SWITCH_OPPONENT_REQ, (ProtocolHandler)&CSrvMsgHandler::switchOpponent);
	registerProtocol(OutsideClientSrv, XIANGQISRV_PLAY_AGAIN_REQ, (ProtocolHandler)&CSrvMsgHandler::pleasePlayAgain);

	// 收到dbproxy_common消息
	registerProtocol(XiangQiSrv, EServiceReplyProtocolId::CUSTOM_GET_USER_BASE_INFO_FOR_CHECK, (ProtocolHandler)&CSrvMsgHandler::checkUserBaseInfoReply);
	
	registerProtocol(ServiceType::DBProxyCommonSrv, CommonDBSrvProtocol::BUSINESS_SET_SERVICE_LOGIC_DATA_RSP, (ProtocolHandler)&CSrvMsgHandler::saveUserLogicDataReply);
	registerProtocol(ServiceType::DBProxyCommonSrv, CommonDBSrvProtocol::BUSINESS_CHANGE_PROP_RSP, (ProtocolHandler)&CSrvMsgHandler::changePropReply);
	registerProtocol(ServiceType::DBProxyCommonSrv, CommonDBSrvProtocol::BUSINESS_CHANGE_MANY_PEOPLE_PROP_RSP, (ProtocolHandler)&CSrvMsgHandler::changeMoreUserPropReply);
	registerProtocol(ServiceType::DBProxyCommonSrv, CommonDBSrvProtocol::BUSINESS_ADD_FF_CHESS_USER_BASE_INFO_RSP, (ProtocolHandler)&CSrvMsgHandler::addUserBaseInfoReply);
	
	registerProtocol(ServiceType::MessageSrv, MessageSrvProtocol::BUSINESS_SYSTEM_MESSAGE_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::systemMessageNotify);
}

void CSrvMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!XiangQiConfig::config::getConfigValue(CCfg::getValue("Config", "BusinessXmlConfigFile")).isSetConfigValueSuccess())
	{
		OptErrorLog("set business xml config value error");
		stopService();
		return;
	}
	
	// 清理连接代理，如果服务异常退出，则启动时需要先清理连接代理
	unsigned int proxyId[1024] = {0};
	unsigned int proxyIdLen = 0;
	const NCommon::Key2Value* pServiceIds = m_srvMsgCommCfg->getList("ServiceID");
	while (pServiceIds)
	{
		if (0 != strcmp("MaxServiceId", pServiceIds->key))
		{
			unsigned int service_id = atoi(pServiceIds->value);
			unsigned int service_type = service_id / ServiceIdBaseValue;
			if (service_type == ServiceType::GatewayProxySrv) proxyId[proxyIdLen++] = service_id;
		}
		
		pServiceIds = pServiceIds->pNext;
	}
	
	if (proxyIdLen > 0) cleanUpConnectProxy(proxyId, proxyIdLen);  // 清理连接代理
	
	// 数据日志配置检查
	const unsigned int MinDataLogFileSize = 1024 * 1024 * 2;
    const unsigned int MinDataLogFileCount = 20;
	const char* dataLoggerCfg[] = {"File", "Size", "BackupCount",};
	for (unsigned int i = 0; i < (sizeof(dataLoggerCfg) / sizeof(dataLoggerCfg[0])); ++i)
	{
		const char* value = CCfg::getValue("DataLogger", dataLoggerCfg[i]);
		if (value == NULL)
		{
			OptErrorLog("data log config item error, can not find item = %s", dataLoggerCfg[i]);
			stopService();
			return;
		}
		
		if ((i == 1 && (unsigned int)atoi(value) < MinDataLogFileSize)
			|| (i == 2 && (unsigned int)atoi(value) < MinDataLogFileCount))
		{
			OptErrorLog("data log config item error, file min size = %d, count = %d", MinDataLogFileSize, MinDataLogFileCount);
			stopService();
			return;
		}
	}
	
	const char* centerRedisSrvItem = "RedisCenterService";
	const char* ip = m_srvMsgCommCfg->get(centerRedisSrvItem, "IP");
	const char* port = m_srvMsgCommCfg->get(centerRedisSrvItem, "Port");
	const char* connectTimeOut = m_srvMsgCommCfg->get(centerRedisSrvItem, "Timeout");
	if (ip == NULL || port == NULL || connectTimeOut == NULL)
	{
		OptErrorLog("redis center service config error");
		stopService();
		return;
	}
	
	if (!m_redisDbOpt.connectSvr(ip, atoi(port), atol(connectTimeOut)))
	{
		OptErrorLog("do connect redis center service failed, ip = %s, port = %s, time out = %s", ip, port, connectTimeOut);
		stopService();
		return;
	}
	
	if (!m_robotManager.load(this))
	{
		OptErrorLog("robot manager load error");
		stopService();
		return;
	}
	
	if (!m_logicHandler.load(this))
	{
		OptErrorLog("logic handler load error");
		stopService();
		return;
	}

    // 充值渔币&兑换金币
	m_rechargeMgr.init(this);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientGetRechargeFishCoinOrder, XIANGQISRV_GET_RECHARGE_FISH_COIN_ORDER_REQ, XIANGQISRV_GET_RECHARGE_FISH_COIN_ORDER_RSP);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientCancelRechargeFishCoinOrder, XIANGQISRV_CANCEL_RECHARGE_FISH_COIN_NOTIFY, 0);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientRechargeFishCoinResultNotify, 0, XIANGQISRV_RECHARGE_FISH_COIN_NOTIFY);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientExchangeGameGoods, XIANGQISRV_EXCHANGE_GOODS_REQ, XIANGQISRV_EXCHANGE_GOODS_RSP);
	
	m_rechargeMgr.addProtocolHandler(EListenerProtocolFlag::ERechargeResultNotify, (ProtocolHandler)&CSrvMsgHandler::rechargeNotifyResult, this);
	
	OptInfoLog("xiang qi fanfan service message handler onload");
}

void CSrvMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 删除服务在redis的信息
	m_redisDbOpt.delHField(NProject::HallKey, NProject::HallKeyLen, (const char*)&(srvId), sizeof(srvId));
	
	// 服务关闭，所有在线用户退出登陆
	const IDToConnectProxys& userConnects = getConnectProxy();
	for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end(); ++it)
	{
		// 这里不可以直接调用removeConnectProxy，如此会不断修改userConnects的值而导致本循环遍历it值错误
		quitGame((XQUserData*)getProxyUserData(it->second), EQuitGameMode::EServiceUnload);
	}

	stopConnectProxy();  // 停止连接代理
	
	m_redisDbOpt.disconnectSvr();
	
	OptInfoLog("xiang qi fanfan service message handler unload");
}

void CSrvMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 监控统计
	m_monitorStat.init(this);
	m_monitorStat.sendOnlineNotify();
	
	setTimer(10 * MillisecondUnit, (TimerHandler)&CSrvMsgHandler::updateRoomInfo, 0, NULL, -1);
	updateRoomInfo(0, 0, NULL, 0);
	
	OptInfoLog("xiang qi fanfan service message handler run ...");
}

// 是否可以执行该操作
const char* CSrvMsgHandler::canToDoOperation(const int opt, const char* info)
{
	if (opt == EServiceOperationType::EClientRequest && checkLoginIsSuccess(info))
	{
		XQUserData* ud = (XQUserData*)getProxyUserData(getConnectProxyContext().conn);
		return ud->username;
	}
	
	return NULL;
}

void CSrvMsgHandler::onLine(XQUserData* ud)
{
	m_logicHandler.onLine(ud);
}

void CSrvMsgHandler::offLine(XQUserData* ud)
{
	m_logicHandler.offLine(ud);
}

// 新增加用户数据
XQUserData* CSrvMsgHandler::addUserData(const char* username, ConnectProxy* conn)
{
	// 挂接用户数据
	XQUserData* ud = (XQUserData*)m_memForUserData.get();
	memset(ud, 0, sizeof(XQUserData));
	ud->init();
	
	addConnectProxy(username, conn, ud);
	
	return ud;
}

XQUserData* CSrvMsgHandler::getUserData(const char* userName)
{
	XQUserData* ud = NULL;
	if (userName == NULL)
	{
		ud = (XQUserData*)getProxyUserData(getConnectProxyContext().conn);
	}
	else if (m_robotManager.isRobot(userName))  // 机器人ID
	{
		ud = m_robotManager.getRobotData(userName);
	}
	else
	{
		ud = (XQUserData*)getProxyUserData(getConnectProxy(string(userName)));
	}
	
	return ud;
}

XQUserData* CSrvMsgHandler::createUserData()
{
	return (XQUserData*)m_memForUserData.get();
}

void CSrvMsgHandler::destroyUserData(XQUserData* xqUD)
{
	m_memForUserData.put((char*)xqUD);
}
	
// 验证用户的合法性
void CSrvMsgHandler::checkUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientXiangQiCheckReq req;
	if (!parseMsgFromBuffer(req, data, len, "check user")) return;

    com_protocol::ClientXiangQiCheckRsp rsp;
	rsp.set_result(XiangQiSessionKeyVerifyFailed);	// 默认 session_key 验证失败
	
    ConnectProxy* curConn = getConnectProxyContext().conn;
	struct NProject::SessionKeyData session_key_data;
	char session_key_str[NProject::MaxKeyLen] = {0};
	while (m_redisDbOpt.getHField(NProject::SessionKey, NProject::SessionKeyLen, req.username().c_str(), req.username().length(), (char*)&session_key_data, sizeof(session_key_data)) == sizeof(session_key_data))
	{
		NProject::getSessionKey(session_key_data, session_key_str);
		if (req.session_key() == session_key_str) // session key 验证通过，去dbproxy获取用户信息
		{
			// 添加用户连接，设置状态
			XQUserData* puser_info = getUserData(req.username().c_str());
			if (NULL != puser_info) // 处理重复连接
			{
				// 同一条连接，连续两次发起verify，则忽略
				ConnectProxy* oldConn = getConnectProxy(req.username());
				if (oldConn->peerIp.s_addr == curConn->peerIp.s_addr && oldConn->peerPort == curConn->peerPort)
				{
					OptWarnLog("same connect to do verify, username = %s, status = %d, ip = %s, port = %d", req.username().c_str(), puser_info->status, CSocket::toIPStr(oldConn->peerIp), oldConn->peerPort);
					return;
				}

				// 不同的连接，以最近一次的连接为准，关闭原有连接
				OptWarnLog("in check user close old connect, username = %s, status = %d, ip = %s, port = %d", req.username().c_str(), puser_info->status, CSocket::toIPStr(oldConn->peerIp), oldConn->peerPort);
				quitGame(req.username().c_str(), "check user close old connect");
			}
			
			// 发起查询用户信息请求
			com_protocol::GetUserBaseinfoReq db_req;
			db_req.set_query_username(req.username());
			db_req.set_data_type(ELogicDataType::FFCHESS_COMMON_DATA);
			const char* msgData = serializeMsgToBuffer(db_req, "get user base info for check");			
			if (msgData != NULL)
			{
				if (sendMessageToService(ServiceType::DBProxyCommonSrv, msgData, db_req.ByteSize(), CommonDBSrvProtocol::BUSINESS_GET_USER_BASEINFO_REQ,
										 req.username().c_str(), req.username().length(), 0, EServiceReplyProtocolId::CUSTOM_GET_USER_BASE_INFO_FOR_CHECK) != Opt_Success)
			    {
				    rsp.set_result(XiangQiSendServiceMsgError);
					break;
			    }
			}
			else
			{
				rsp.set_result(XiangQiEncodeDeCodeError);
				break;
			}
			
			// 新增用户连接
			puser_info = addUserData(req.username().c_str(), curConn);
			puser_info->status = EEnterGame;
			strncpy(puser_info->username, req.username().c_str(), sizeof(puser_info->username) - 1);
			puser_info->usernameLen = strlen(puser_info->username);
			
			// 先填写默认值
			// puser_info->playMode = com_protocol::EFFChessMode::EClassicMode;
			// puser_info->arenaId = 1;

			// 将用户进入游戏的信息写到redis
			uint32_t srv_id = getSrvId();
			int rc = m_redisDbOpt.setHField(NProject::GameServiceKey, NProject::GameServiceLen, req.username().c_str(), req.username().length(), (char*)&srv_id, sizeof(srv_id));
			if (rc != 0) OptErrorLog("set GameServiceKey to redis error, username = %s, rc = %d", req.username().c_str(), rc);

			return;
		}
		
		break;
	}

	OptErrorLog("check user error, result = %d, username = %s, request session key = %s, current session key = %s, close connect ip = %s, port = %d",
	rsp.result(), req.username().c_str(), req.session_key().c_str(), session_key_str, CSocket::toIPStr(curConn->peerIp), curConn->peerPort);
	
	sendPkgMsgToClient(rsp, XiangQiSrvProtocol::XIANGQISRV_CHECK_USER_RSP);
	closeConnectProxy(curConn);  // 验证失败则关闭连接
}

void CSrvMsgHandler::checkUserBaseInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUserBaseinfoRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "check user base info reply")) return;

    XQUserData* puser_info = getUserData(getContext().userData);
	if (puser_info == NULL || rsp.query_username() != puser_info->username) return;

	com_protocol::ClientXiangQiCheckRsp verify_rsp;
	verify_rsp.set_result(rsp.result());
	if (Opt_Success == verify_rsp.result())
	{
		// 如果还没有添加过的话则增加翻翻棋用户
		sendMessageToService(ServiceType::DBProxyCommonSrv, NULL, 0, CommonDBSrvProtocol::BUSINESS_ADD_FF_CHESS_USER_BASE_INFO_REQ, puser_info->username, puser_info->usernameLen);
		
		// 通知消息中心服务有用户进入了游戏
		com_protocol::UserEnterServiceNotify enterSrvNotify;
		enterSrvNotify.set_service_id(getSrvId());
		const char* msgData = serializeMsgToBuffer(enterSrvNotify, "notify message service enter game");
		if (msgData != NULL) sendMessageToService(ServiceType::MessageSrv, msgData, enterSrvNotify.ByteSize(), BUSINESS_USER_ENTER_SERVICE_NOTIFY, puser_info->username, puser_info->usernameLen);
							 
		// 设置用户信息
		strncpy(puser_info->nickname, rsp.detail_info().static_info().nickname().c_str(), sizeof(puser_info->nickname) - 1);
		puser_info->portrait_id = rsp.detail_info().static_info().portrait_id();
		puser_info->cur_gold = rsp.detail_info().dynamic_info().game_gold();
		if (rsp.has_ff_chess_data()) *puser_info->srvData = rsp.ff_chess_data();
		
		if (rsp.commondb_data().has_ff_chess_goods())
		{
			puser_info->chessBoardId = rsp.commondb_data().ff_chess_goods().current_board_id();
			puser_info->chessPiecesId = rsp.commondb_data().ff_chess_goods().current_pieces_id();
		}
		
		com_protocol::XiangQiUserBaseInfo* base_info = verify_rsp.mutable_base_info();
		base_info->set_nickname(puser_info->nickname);
		base_info->set_portrait_id(puser_info->portrait_id);
		base_info->set_game_gold(puser_info->cur_gold);

		google::protobuf::RepeatedPtrField<com_protocol::ClientChessArenaInfo>* list = verify_rsp.mutable_classic_arena_info();
		getChessArenaInfo(com_protocol::EFFChessMode::EClassicMode, list);
		
		onLine(puser_info);
	}

	sendPkgMsgToClient(verify_rsp, XiangQiSrvProtocol::XIANGQISRV_CHECK_USER_RSP, puser_info->username);

	if (verify_rsp.result() == Opt_Success)
	{
		OptInfoLog("check user success, username = %s, nickname = %s, current gold = %lu", puser_info->username, puser_info->nickname, puser_info->cur_gold);
		logUserInfo(rsp.detail_info(), "check user success");  // 记录用户登陆是信息
	}
	else
	{
		OptErrorLog("check user error and close connect, name = %s, result = %d", puser_info->username, verify_rsp.result());
		quitGame(puser_info->username, "check user result is error");  // 验证失败，断开连接
	}
}

// 保存用户逻辑数据应答
void CSrvMsgHandler::saveUserLogicDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetGameLogicDataRsp rsp;
	rsp.set_result(XiangQiEncodeDeCodeError);
	parseMsgFromBuffer(rsp, data, len, "save user logic data reply");
	if (rsp.result() != Opt_Success)
	{
		OptErrorLog("save user logic data reply error, name = %s, primary key = %s, second key = %s, result = %d",
		getContext().userData, rsp.primary_key().c_str(), rsp.second_key().c_str(), rsp.result());
	}
}

// 用户主动退出游戏
void CSrvMsgHandler::userQuitGame(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("user quit game")) return;

	com_protocol::ClientXiangQiQuitReq req;
	if (!parseMsgFromBuffer(req, data, len, "user quit game")) return;

	com_protocol::ClientXiangQiQuitRsp rsp;
	if (req.has_code()) rsp.set_code(req.code());
	sendPkgMsgToClient(rsp, XiangQiSrvProtocol::XIANGQISRV_USER_QUIT_RSP);
	
    XQUserData* p_user_info = getUserData();
	OptInfoLog("user quit game, name = %s", p_user_info->username);
	quitGame(p_user_info->username, "user quit game");
}

// 获取翻翻棋场次信息
void CSrvMsgHandler::getFFChessArenaInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("get FF chess arena info")) return;
	
	com_protocol::ClientGetChessArenaRsp rsp;
	google::protobuf::RepeatedPtrField<com_protocol::ClientChessArenaInfo>* list = rsp.mutable_classic_arena_info();
	getChessArenaInfo(com_protocol::EFFChessMode::EClassicMode, list);
	
	sendPkgMsgToClient(rsp, XiangQiSrvProtocol::XIANGQISRV_GET_ARENA_INFO_RSP);
}
	
// 用户进入翻翻棋场次
void CSrvMsgHandler::userEnterArena(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("user enter chess arena")) return;
	
	com_protocol::ClientEnterArenaReq req;
	if (!parseMsgFromBuffer(req, data, len, "user enter arena")) return;
	
	if (req.mode() != com_protocol::EFFChessMode::EClassicMode)
	{
		OptErrorLog("user enter chess arena error, name = %s, mode = %d, id = %d", getUserData()->username, req.mode(), req.id());
		return;
	}
	
	com_protocol::ClientEnterArenaRsp rsp;
	rsp.set_result(XiangQiInvalidFFChesssArena);
	
	XQUserData* ud = getUserData();
	map<unsigned int, XiangQiConfig::ChessArenaConfig>::const_iterator arenaCfgIt = XiangQiConfig::config::getConfigValue().classic_arena_cfg.find(req.id());
	while (arenaCfgIt != XiangQiConfig::config::getConfigValue().classic_arena_cfg.end())
	{
		// 游戏币不够
		if (ud->cur_gold < arenaCfgIt->second.min_gold || ud->cur_gold < arenaCfgIt->second.service_cost)
		{
			rsp.set_result(XiangQiGoldNotEnough);
			break;
		}
		
		// 游戏币过多，得进高倍场
		if (ud->cur_gold > arenaCfgIt->second.max_gold)
		{
			rsp.set_result(XiangQiMoreGoldError);
			break;
		}
		
		rsp.set_result(Opt_Success);
		rsp.set_base_rate(arenaCfgIt->second.base_rate);
		rsp.set_max_rate(arenaCfgIt->second.max_rate);
		rsp.set_other_rate(arenaCfgIt->second.other_rate);
		
		ud->playMode = req.mode();
		ud->arenaId = req.id();
		
		++m_onlinePlayerCount[req.mode()][req.id()];

		break;
	}
	
	sendPkgMsgToClient(rsp, XiangQiSrvProtocol::XIANGQISRV_ENTER_CHESS_ARENA_RSP);
	
	OptInfoLog("user enter chess arena, name = %s, mode = %d, id = %d, result = %d, online count = %d",
	ud->username, ud->playMode, ud->arenaId, rsp.result(), m_onlinePlayerCount[req.mode()][req.id()]);
}

// 用户退出翻翻棋场次
void CSrvMsgHandler::userLeaveArena(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("user leave arena")) return;
	
	XQUserData* ud = getUserData();
	XQChessPieces* xqCP = getXQChessPieces(ud->username);
	if (xqCP != NULL) finishSettlement(xqCP, ud->sideFlag, com_protocol::SettlementResult::PlayerEscape, ud);  // 主动退出正在进行的游戏则算逃跑
	
	string& waitUser = m_waitForMatchingUsers[ud->playMode][ud->arenaId];
	if (waitUser == ud->username) waitUser.clear();
	
	unsigned int& playerCount = m_onlinePlayerCount[ud->playMode][ud->arenaId];
	if (playerCount > 0) --playerCount;
	
	ud->playMode = 0;
	ud->arenaId = 0;
	
	destroyPlayAgainInfo(ud->playAgainId);
}
	
// 进入游戏，开始匹配对手
void CSrvMsgHandler::enterGame(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("user enter game")) return;
	
	XQUserData* userData = getUserData();
    matchingOpponentEnterGame(userData);
}

// 玩家准备操作通知
void CSrvMsgHandler::readyOptNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("ready opt")) return;
	
	com_protocol::ClientReadyOptNotify readyOptNt;
	if (!parseMsgFromBuffer(readyOptNt, data, len, "ready opt")) return;
	
	XQUserData* xqUD = getUserData();
	XQChessPieces* xqCP = getXQChessPieces(xqUD->username);
	if (xqCP == NULL)
	{
		OptErrorLog("ready opt notify, can not find the chess pieces error, username = %s, opt = %d", xqUD->username, readyOptNt.opt_value());
		return;
	}
	
	if (xqCP->curOptTimerId != 0)
	{
		killTimer(xqCP->curOptTimerId);
		xqCP->curOptTimerId = 0;
	}
	
	// 发送玩家准备操作结果，通知双方玩家
    sendReadyOptResultNotify(xqCP, xqUD->username, readyOptNt.opt_value());
	
	switch (readyOptNt.opt_value())
	{
		case com_protocol::EPlayerReadyOpt::ENotCompeteRed:  // 不抢红
		{
			if (xqCP->curOptSide == EXQSideFlag::ERedSide)
			{
				// 红方不抢红 则再询问黑方是否需要抢红争先
				getCompeteFirstBlackInquiry(xqCP);
			}
	        else
			{
			    // 黑方也不抢红 则表示双方都不抢红争先，即默认红方争先
				// 接下来询问黑方是否需要加倍加注
				getAddDoubleInquiry(xqCP, EXQSideFlag::EBlackSide); 
			}
			
			break;
		}
		
		case com_protocol::EPlayerReadyOpt::EGetCompeteRed:  // 抢红
		{
			confirmCompeteFirstRed(xqCP);
			
			// 询问未抢红的一方是否需要加倍加注
			getAddDoubleInquiry(xqCP, EXQSideFlag::EBlackSide);
			
			break;
		}
		
		case com_protocol::EPlayerReadyOpt::ENotAddDouble:     // 不加倍
		{
	        moveChessPiecesNotify(xqCP, EXQSideFlag::ERedSide);  // 红方开始下棋
			
			break;
		}
		
		case com_protocol::EPlayerReadyOpt::EGetAddDouble:  // 加倍
		{
			xqCP->firstDoubleValue *= 2;  // 翻倍
	        moveChessPiecesNotify(xqCP, EXQSideFlag::ERedSide);  // 红方开始下棋
			
			break;
		}
		
		default:
		{
			break;
		}
	}
	
	OptInfoLog("ready opt, red user = %s, black user = %s, rate = %d, first double = %d",
	xqCP->sideUserName[EXQSideFlag::ERedSide], xqCP->sideUserName[EXQSideFlag::EBlackSide], getCurrentRate(xqCP), xqCP->firstDoubleValue);
}

// 玩家下棋走子
void CSrvMsgHandler::playChessPiecesNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("play chess pieces")) return;
	
	com_protocol::ClientPlayChessPiecesNotify plCPNotify;
	if (!parseMsgFromBuffer(plCPNotify, data, len, "play chess pieces")) return;
	
	int errorCode = Opt_Success;
	XQUserData* xqUD = getUserData();
    userPlayChessPieces(xqUD, plCPNotify, errorCode);
	if (errorCode != Opt_Success)
	{
		com_protocol::ClientPlayChessPiecesResultNotify resultNotify;
		resultNotify.set_result(errorCode);
		sendPkgMsgToClient(resultNotify, XiangQiSrvProtocol::XIANGQISRV_PLAY_CHESS_RESULT_NOTIFY);
	}
}

// 玩家主动认输
// 玩家认输通知服务器，会直接触发按认输的倍率进行结算
void CSrvMsgHandler::playerBeDefeateNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("player activity defeate notify")) return;
	
	XQUserData* xqUD = getUserData();
	XQChessPieces* xqCP = getXQChessPieces(xqUD->username);
	if (xqCP != NULL && xqUD->sideFlag == xqCP->curOptSide) finishSettlement(xqCP, xqUD->sideFlag, com_protocol::SettlementResult::BeDefeate);
}

// 玩家求和
// 玩家A发起求和请求，玩家A的求和请求会被转发通知给玩家B
void CSrvMsgHandler::playerPleasePeace(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("player please peace")) return;
	
	const XQUserData* xqUD = getUserData();
	const XQChessPieces* xqCP = getXQChessPieces(xqUD->username);
	if (xqCP != NULL)
	{		
		com_protocol::ClientPlayerPleasePeaceRsp rsp;
		rsp.set_result(Opt_Success);
		const XiangQiConfig::CommConfig& common_cfg = XiangQiConfig::config::getConfigValue().common_cfg;
		if (xqCP->pleasePeace[xqUD->sideFlag] >= common_cfg.please_peace_max_times)
		{
			rsp.set_result(XiangQiPleasePeaceTimesInvalid);
		}
		else if (xqCP->moveCount[xqUD->sideFlag] < common_cfg.please_peace_move_count)
		{
			rsp.set_result(XiangQiPleasePeaceMoveChessInvalid);
			rsp.set_need_move_count(common_cfg.please_peace_move_count);
		}
		
		if (rsp.result() != Opt_Success)
		{
			rsp.set_remain_times(common_cfg.please_peace_max_times - xqCP->pleasePeace[xqUD->sideFlag]);
			sendPkgMsgToClient(rsp, XiangQiSrvProtocol::XIANGQISRV_PLEASE_PEACE_RSP);
			return;
		}
		
		// 机器人处理
		const char* otherSideId = xqCP->sideUserName[(xqUD->sideFlag + 1) % EXQSideFlag::ESideFlagCount];
		if (m_robotManager.isRobot(otherSideId))
		{
			const XiangQiConfig::RobotConfig& robotCfg = XiangQiConfig::config::getConfigValue().robot_cfg;
			setTimer(getPositiveRandNumber(robotCfg.min_refuse_peace, robotCfg.max_refuse_peace) * MillisecondUnit, (TimerHandler)&CSrvMsgHandler::robotRefusePeace,
					 atoi(otherSideId + 1), (void*)(long long)xqUD->sideFlag);
					 
		    return;
		}
		
		// 玩家A的求和请求会被转发通知给玩家B
		com_protocol::ClientPlayerPleasePeaceNotify pleasePeaceNotify;
		sendPkgMsgToClient(pleasePeaceNotify, XiangQiSrvProtocol::XIANGQISRV_PLEASE_PEACE_NOTIFY, otherSideId, "please peace notify");
	}
}

// 玩家求和对方确认
// 玩家B收到玩家A的求和请求后可以拒绝或者同意该请求，该消息发送到服务器
// 最后服务器把求和协商结果发送给玩家A
void CSrvMsgHandler::playerPleasePeaceConfirm(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("please peace confirm")) return;

	com_protocol::ClientPlayerPleasePeaceConfirmNotify confirmNotify;
	if (!parseMsgFromBuffer(confirmNotify, data, len, "please peace confirm")) return;
	
	com_protocol::ClientPlayerPleasePeaceRsp rsp;
	(confirmNotify.opt() == 1) ? rsp.set_result(Opt_Success) : rsp.set_result(XiangQiPleasePeaceOpponentRefuse);
	
	const XQUserData* xqUD = getUserData();
	XQChessPieces* xqCP = getXQChessPieces(xqUD->username);
	if (xqCP != NULL)
	{
		const unsigned short pleasePeaceSide = (xqUD->sideFlag + 1) % EXQSideFlag::ESideFlagCount;
		++xqCP->pleasePeace[pleasePeaceSide];
		rsp.set_remain_times(XiangQiConfig::config::getConfigValue().common_cfg.please_peace_max_times - xqCP->pleasePeace[pleasePeaceSide]);
		sendPkgMsgToClient(rsp, XiangQiSrvProtocol::XIANGQISRV_PLEASE_PEACE_RSP, xqCP->sideUserName[pleasePeaceSide], "please peace confirm");
		
		if (confirmNotify.opt() == 1) finishSettlement(xqCP, pleasePeaceSide, com_protocol::SettlementResult::PleasePeace);
	}
}

// 切换对手
void CSrvMsgHandler::switchOpponent(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("user switch opponent")) return;

	com_protocol::ClientSwitchOpponentRsp rsp;
	XQUserData* ud = getUserData();
	rsp.set_result(checkArenaCondition(ud));  // 检查进场合法性
	if (rsp.result() != Opt_Success)
	{
		destroyPlayAgainInfo(ud->playAgainId);
		
		sendPkgMsgToClient(rsp, XiangQiSrvProtocol::XIANGQISRV_SWITCH_OPPONENT_RSP);
		
		return;
	}

    matchingOpponentEnterGame(ud);  // 匹配对手进入游戏
}

// 再来一局
void CSrvMsgHandler::pleasePlayAgain(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("please play again")) return;

	com_protocol::ClientPlayAgainRsp rsp;
	XQUserData* ud = getUserData();
	rsp.set_result(checkArenaCondition(ud));  // 检查进场合法性
	if (rsp.result() != Opt_Success)
	{
		destroyPlayAgainInfo(ud->playAgainId);
		
		sendPkgMsgToClient(rsp, XiangQiSrvProtocol::XIANGQISRV_PLAY_AGAIN_RSP);
		
		return;
	}
	
	SPlayAgainInfo* paInfo = getPlayAgainInfo(ud->playAgainId);
	if (paInfo == NULL)
	{
		matchingOpponentEnterGame(ud);  // 匹配对手进入游戏
		return;
	}
	
	const unsigned int otherSize = (ud->sideFlag + 1) % EXQSideFlag::ESideFlagCount;
	XQUserData* otherUd = getUserData(paInfo->username[otherSize].c_str());
	if (otherUd == NULL || otherUd->playAgainId != ud->playAgainId)
	{
		matchingOpponentEnterGame(ud);  // 匹配对手进入游戏
		return;
	}
	
	if (paInfo->flag[otherSize] == 1)   // 之前另外一方已经进入等待状态
	{
		matchingOpponentEnterGame(otherUd->username, ud);		
		return;
	}
	
	paInfo->flag[ud->sideFlag] = 1;  // 进入等待状态
}

// 系统消息通知
void CSrvMsgHandler::systemMessageNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// now to do nothing
}

// 是否可以吃掉对方的棋子
bool CSrvMsgHandler::canEatOtherChessPieces(int weSide, int otherSide)
{
	if (otherSide == com_protocol::EChessPiecesValue::ENoneQiZiOpen) return true;  // 没有棋子的空位置
	
	// 调整为固定的红&黑双方
	if (weSide > com_protocol::EChessPiecesValue::EHongQiBing)
	{
		weSide -= com_protocol::EChessPiecesValue::EHeiQiJiang;
		otherSide += com_protocol::EChessPiecesValue::EHeiQiJiang;
	}
	
	switch (weSide)
	{
		case com_protocol::EChessPiecesValue::EHongQiJiang:
		{
			return otherSide != com_protocol::EChessPiecesValue::EHeiQiBing;
		}
		
		case com_protocol::EChessPiecesValue::EHongQiPao:
		{
			return true;
		}
		
		case com_protocol::EChessPiecesValue::EHongQiShi:
		{
			return otherSide != com_protocol::EChessPiecesValue::EHeiQiJiang;
		}
		
		case com_protocol::EChessPiecesValue::EHongQiXiang:
		{
			return otherSide > com_protocol::EChessPiecesValue::EHeiQiShi && otherSide < com_protocol::EChessPiecesValue::ENoneQiZiClose;
		}
		
		case com_protocol::EChessPiecesValue::EHongQiChe:
		{
			return otherSide > com_protocol::EChessPiecesValue::EHeiQiXiang && otherSide < com_protocol::EChessPiecesValue::ENoneQiZiClose;
		}
		
		case com_protocol::EChessPiecesValue::EHongQiMa:
		{
			return otherSide > com_protocol::EChessPiecesValue::EHeiQiChe && otherSide < com_protocol::EChessPiecesValue::ENoneQiZiClose;
		}
		
		case com_protocol::EChessPiecesValue::EHongQiBing:
		{
			return otherSide == com_protocol::EChessPiecesValue::EHeiQiBing || otherSide == com_protocol::EChessPiecesValue::EHeiQiJiang;
		}
		
		default:
		{
			break;
		}
	}
	
	return false;
}

// 是否可以移动到目标位置
bool CSrvMsgHandler::canMoveToPosition(const int src, const int dst)
{
	int reduceValue = src - dst;
	if (reduceValue < 0) reduceValue = -reduceValue;
	if (reduceValue != 1 && reduceValue != MaxDifferenceValue) return false;
	
	// ----------------------------
	//       棋盘位置分布图
	//       0--08--16--24
	//       |   |   |   |
	//       1--09--17--25
	//       |   |   |   |
	//       2--10--18--26
	//       |   |   |   |
	//       3--11--19--27
	//       |   |   |   |
	//       4--12--20--28
	//       |   |   |   |
	//       5--13--21--29
	//       |   |   |   |
	//       6--14--22--30
	//       |   |   |   |
	//       7--15--23--31
	// ----------------------------
	
	if (reduceValue == 1)
	{
		// 向上移动，最上一行棋子  0--08--16--24
		if (src > dst && (src % MaxDifferenceValue) == 0) return false;

		// 向下移动，最下一行棋子  7--15--23--31
		if (src < dst && ((src + 1) % MaxDifferenceValue) == 0) return false;
	}
	
	return true;
}

// 检查玩家移动的炮是否合法
bool CSrvMsgHandler::checkMovePaoIsOK(const int src_position, const int dst_position, const unsigned short curOptSide, const short* value, const char* username)
{
	// ----------------------------
	//       棋盘位置分布图
	//       0--08--16--24
	//       |   |   |   |
	//       1--09--17--25
	//       |   |   |   |
	//       2--10--18--26
	//       |   |   |   |
	//       3--11--19--27
	//       |   |   |   |
	//       4--12--20--28
	//       |   |   |   |
	//       5--13--21--29
	//       |   |   |   |
	//       6--14--22--30
	//       |   |   |   |
	//       7--15--23--31
	// ----------------------------
	
	static const unsigned short Xcoordinate[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3};  // 棋盘位置X轴坐标值
	static const unsigned short Ycoordinate[] = {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7};  // 棋盘位置Y轴坐标值
	
	if (src_position == dst_position
	    || (Xcoordinate[src_position] != Xcoordinate[dst_position] && Ycoordinate[src_position] != Ycoordinate[dst_position])) return false;  // 不在相同坐标轴上则出错
	
    const int dstChessPiecesValue = value[dst_position];
	if ((dstChessPiecesValue < ClosedChessPiecesValue && isSameSide(curOptSide, dstChessPiecesValue))  // 炮不能明打自己一方的棋子
        || dstChessPiecesValue == com_protocol::EChessPiecesValue::ENoneQiZiOpen)  // 不能走空位
	{
		if (username != NULL) OptErrorLog("check pao dst side invalid, name = %s, current side = %d, value = %d", username, curOptSide, dstChessPiecesValue);
		return false;
	}

	int maxValue = src_position;
	int reduceValue = src_position - dst_position;
	if (reduceValue < 0)
	{
		maxValue = dst_position;
		reduceValue = -reduceValue;
	}
	
	if (reduceValue == MaxDifferenceValue) return false;
	
	unsigned int diffIndexValue = (reduceValue < MaxDifferenceValue) ? 1 : MaxDifferenceValue;
	
	/*
	unsigned int diffIndexValue = 1;  // 默认上下移动
	if (reduceValue > MaxDifferenceValue)  // 左右移动
	{
		if ((reduceValue % MaxDifferenceValue) != 0)  // 必须保证差值是MaxDifferenceValue值的1倍之上
		{
			if (username != NULL) OptErrorLog("check pao position error, name = %s, src position = %d, dst position = %d", username, src_position, dst_position);
			return false;
		}
		
		diffIndexValue = MaxDifferenceValue;  // 左右移动
	}
	
	if (diffIndexValue == 1)
	{
		// 向上移动
		// 最上一行棋子  0--08--16--24
		if (src_position > dst_position)
		{
			if ((src_position % MaxDifferenceValue) == 0 || ((src_position - 1) % MaxDifferenceValue) == 0) return false;
		}
		
		// 向下移动
		// 最下一行棋子  7--15--23--31
		else
		{
			int tmpSrc = src_position + 1;
			if ((tmpSrc % MaxDifferenceValue) == 0 || ((tmpSrc + 1) % MaxDifferenceValue) == 0) return false;
		}
	}
    */
	
	// 查看源位置和目标位置之间是否只存在一个棋子
	unsigned int hasChessPiecesCount = 0;
	reduceValue -= diffIndexValue;
	while (reduceValue > 0)
	{
		if (value[maxValue - reduceValue] != com_protocol::EChessPiecesValue::ENoneQiZiOpen) ++hasChessPiecesCount;
		reduceValue -= diffIndexValue;
			
		/*
		// 炮吃子必须在相同的X坐标或者Y坐标轴上
		if (Xcoordinate[maxValue] == Xcoordinate[maxValue - reduceValue] || Ycoordinate[maxValue] == Ycoordinate[maxValue - reduceValue])
		{
			if (value[maxValue - reduceValue] != com_protocol::EChessPiecesValue::ENoneQiZiOpen) ++hasChessPiecesCount;
			reduceValue -= diffIndexValue;
		}
		else
		{
			if (username != NULL) OptErrorLog("check pao position coordinate error, name = %s, src position = %d, dst position = %d", username, src_position, dst_position);
		    return false;
		}
		*/ 
	}
	
	if (hasChessPiecesCount != 1)
	{
		if (username != NULL) OptErrorLog("check pao position error, name = %s, src position = %d, dst position = %d, chess pieces count = %u", username, src_position, dst_position, hasChessPiecesCount);
		return false;
	}
	
	return true;
}

// 是否是自己一方的棋子
bool CSrvMsgHandler::isSameSide(const unsigned int side, const unsigned int chessPiecesValue)
{
	return (side == EXQSideFlag::ERedSide && chessPiecesValue < com_protocol::EChessPiecesValue::EHeiQiJiang)
		   || (side == EXQSideFlag::EBlackSide  && chessPiecesValue > com_protocol::EChessPiecesValue::EHongQiBing && chessPiecesValue < com_protocol::EChessPiecesValue::ENoneQiZiClose);
}

// 重置连击值
void CSrvMsgHandler::resetContinueHitValue(XQChessPieces* xqCP, short side)
{
	if (xqCP->curHitValue[side] >= ContinueHitTimes)
	{
		for (unsigned int idx = 0; idx < MaxContinueHitCount; ++idx)
		{
			if (xqCP->continueHitValue[idx] == 0)
			{
				xqCP->continueHitValue[idx] = xqCP->curHitValue[side];
				break;
			}
		}
	}
	
	taskStatistics(TaskStatisticsOpt::ETSContinueHit, xqCP, side);  // 任务统计
	
	xqCP->curHitValue[side] = 0;
}

// 获取当前倍率值
unsigned int CSrvMsgHandler::getCurrentRate(const XQChessPieces* xqCP)
{
	XQUserData* ud = getUserData(xqCP->sideUserName[EXQSideFlag::ERedSide]);
	if (ud == NULL) return 0;
	
	// 连击值
	unsigned short allContinueHitValue = 0;
	for (unsigned int idx = 0; idx < MaxContinueHitCount && xqCP->continueHitValue[idx] > 0; ++idx)
	{
		allContinueHitValue += xqCP->continueHitValue[idx];  // 之前的连击值
	}
	
	// 当前的连击值
	if (xqCP->curHitValue[EXQSideFlag::ERedSide] >= ContinueHitTimes) allContinueHitValue += xqCP->curHitValue[EXQSideFlag::ERedSide];
	if (xqCP->curHitValue[EXQSideFlag::EBlackSide] >= ContinueHitTimes) allContinueHitValue += xqCP->curHitValue[EXQSideFlag::EBlackSide];
	if (allContinueHitValue == 0) allContinueHitValue = 1;
	
	// 翻开的棋子值
	const XiangQiConfig::config& cfg = XiangQiConfig::config::getConfigValue();	
	unsigned short allOpenChessPiecesValue = 0;
	for (unsigned int idx = 0; idx < OpenChessPiecesCount; ++idx)
	{
		allOpenChessPiecesValue += cfg.settlement_value.at(xqCP->openChessPiecesValue[idx]);  // 默认翻开棋子的4个位置
	}
	
	// 血量差
	int differLife = xqCP->curLife[EXQSideFlag::ERedSide] - xqCP->curLife[EXQSideFlag::EBlackSide];
	if (xqCP->curLife[EXQSideFlag::ERedSide] < xqCP->curLife[EXQSideFlag::EBlackSide]) differLife = xqCP->curLife[EXQSideFlag::EBlackSide] - xqCP->curLife[EXQSideFlag::ERedSide];
	
	// 倍率=抢先加倍*连击*（翻棋+血量差）
	unsigned int curRate = xqCP->firstDoubleValue * allContinueHitValue * (allOpenChessPiecesValue + differLife);
	
	return curRate;
}

// 结束了则结算
void CSrvMsgHandler::finishSettlement(XQChessPieces* xqCP, const short lostSide, const com_protocol::SettlementResult result, XQUserData* lostUserData)
{
	if (xqCP->curOptTimerId != 0)
	{
		killTimer(xqCP->curOptTimerId);
		xqCP->curOptTimerId = 0;
	}
	
	com_protocol::ClientPlaySettlementNotify settlementNotify;
	settlementNotify.set_result(result);
	settlementNotify.set_winner(xqCP->sideUserName[(lostSide + 1) % EXQSideFlag::ESideFlagCount]);
	settlementNotify.set_loser(xqCP->sideUserName[lostSide]);
	
	// 赢输双方数据
	XQUserData* winUserData = getUserData(settlementNotify.winner().c_str());
	if (lostUserData == NULL) lostUserData = getUserData(settlementNotify.loser().c_str());
	
	if (winUserData == NULL || lostUserData == NULL)
	{
		OptErrorLog("finish settlement, winUserData = %p, lostUserData = %p, winner = %s, loser = %s, result = %d",
	    winUserData, lostUserData, settlementNotify.winner().c_str(), settlementNotify.loser().c_str(), result);
	    return;
	}

	settlementNotify.set_first_double(xqCP->firstDoubleValue);
	settlementNotify.set_winner_life(xqCP->curLife[(lostSide + 1) % EXQSideFlag::ESideFlagCount]);
	settlementNotify.set_loser_life(xqCP->curLife[lostSide]);
	
	// 连击值
	for (unsigned int idx = 0; idx < MaxContinueHitCount && xqCP->continueHitValue[idx] > 0; ++idx)
	{
		settlementNotify.add_three_hit(xqCP->continueHitValue[idx]);  // 之前的连击值
	}
	
	// 最后的连击值
	if (xqCP->curHitValue[EXQSideFlag::ERedSide] >= ContinueHitTimes) settlementNotify.add_three_hit(xqCP->curHitValue[EXQSideFlag::ERedSide]);
	if (xqCP->curHitValue[EXQSideFlag::EBlackSide] >= ContinueHitTimes) settlementNotify.add_three_hit(xqCP->curHitValue[EXQSideFlag::EBlackSide]);
	
	// 翻开的棋子值
	const XiangQiConfig::config& cfg = XiangQiConfig::config::getConfigValue();
	const unordered_map<unsigned int, unsigned int>& settlement_rate_value = cfg.settlement_value;
	for (unsigned int idx = 0; idx < OpenChessPiecesCount; ++idx)
	{
		// 默认翻开棋子的4个位置
		com_protocol::ChessPiecesPosition* cpValue = settlementNotify.add_open_chess_pieces();
		cpValue->set_chess_pieces((com_protocol::EChessPiecesValue)xqCP->openChessPiecesValue[idx]);
		cpValue->set_rate(settlement_rate_value.at(xqCP->openChessPiecesValue[idx]));
	}
	
	// 未翻开的棋子&位置
	for (unsigned int idx = 0; idx < MaxChessPieces; ++idx)
	{
		// 还未翻开的棋子
		if (xqCP->value[idx] >= ClosedChessPiecesValue)
		{
			com_protocol::ChessPiecesPosition* cpValue = settlementNotify.add_closed_chess_pieces();
			cpValue->set_chess_pieces((com_protocol::EChessPiecesValue)(xqCP->value[idx] - ClosedChessPiecesValue));
			cpValue->set_position(idx);
		}
	}

    /*
	NormalFinish = 1;                                      // 正常结束，存在输赢
	ToDoPeace = 2;                                         // 和棋，无输赢
	PleasePeace = 3;                                       // 玩家求和，无输赢
	BeDefeate = 4;                                         // 玩家主动认输
	PlayTimeOut = 5;                                       // 玩家下棋超时
	PlayerDisconnect = 6;                                  // 玩家网络掉线
	PlayerEscape = 7;                                      // 玩家逃跑
	ServiceUnload = 8;                                     // 服务器正常停止
    */ 
	
	const XiangQiConfig::ChessArenaConfig& classic_common_cfg = cfg.classic_arena_cfg.at(winUserData->arenaId);
	unsigned int curRate = getCurrentRate(xqCP);  // 当前结算倍率值
	if (result == com_protocol::SettlementResult::BeDefeate)
	{
		if (curRate < classic_common_cfg.bedefeate_rate) curRate = classic_common_cfg.bedefeate_rate;  // 主动认输情况下的最低倍率
	}
	else if ((result == com_protocol::SettlementResult::PlayTimeOut || result == com_protocol::SettlementResult::PlayerDisconnect
	        || result == com_protocol::SettlementResult::PlayerEscape) && curRate < classic_common_cfg.other_rate)  // 超时、掉线、逃跑等其他情况下使用的倍率
	{
		curRate = classic_common_cfg.other_rate;  // 超时、掉线、逃跑等其他情况下使用的倍率
	}

    // 总值=底注*倍率
	// 倍率=抢先加倍*连击*（翻棋+血量差）
	settlementNotify.set_value(classic_common_cfg.base_rate * ((curRate <= classic_common_cfg.max_rate) ? curRate : classic_common_cfg.max_rate));  // 结算的最大倍率
	settlementNotify.set_base_rate(classic_common_cfg.base_rate);
	settlementNotify.set_cur_rate(curRate);
	
	// 和棋&服务器停止则不结算
	com_protocol::EFFChessMatchResult matchResult = com_protocol::EFFChessMatchResult::EFFChessMatchWin;
	if (result == com_protocol::SettlementResult::ToDoPeace || result == com_protocol::SettlementResult::PleasePeace || result == com_protocol::SettlementResult::ServiceUnload)
	{
		matchResult = com_protocol::EFFChessMatchResult::EFFChessMatchPeace;
		settlementNotify.set_value(0);
	}
	else
	{
		// 如果总值大于最小金币值的话则按照玩家携带的最小金币方结算
		uint64_t settlementValue = (winUserData->cur_gold < lostUserData->cur_gold) ? winUserData->cur_gold : lostUserData->cur_gold;
		if (settlementNotify.value() > settlementValue) settlementNotify.set_value(settlementValue);
		
		// 刷新双方的结算金币到DB
		MoreUserRecordItemVector userRecords;
		userRecords.push_back(MoreUserRecordItem());
		MoreUserRecordItem& recordInfo = userRecords.back();
		recordInfo.items.push_back(RecordItem(EPropType::PropGold, settlementNotify.value()));  // 结算费用
		recordInfo.src_username = winUserData->username;
		recordInfo.remark = "结算费用";
		winUserData->cur_gold += settlementNotify.value();

		userRecords.push_back(MoreUserRecordItem());
		MoreUserRecordItem& lostRecordInfo = userRecords.back();
		lostRecordInfo.items.push_back(RecordItem(EPropType::PropGold, -settlementNotify.value()));  // 结算费用
		lostRecordInfo.src_username = lostUserData->username;
		lostRecordInfo.remark = "结算费用";
		lostUserData->cur_gold -= settlementNotify.value();
		
		notifyDbProxyChangeProp(userRecords, EChangeGoodsFlag::EFinishSettlement, classic_common_cfg.base_rate, classic_common_cfg.name.c_str());
		
		// 任务统计结算
		m_logicHandler.doTaskStatistics(xqCP, lostSide, result, winUserData->username);
		m_logicHandler.doTaskStatistics(xqCP, lostSide, result, lostUserData->username);
	}
	
	if (result == com_protocol::SettlementResult::PlayerEscape || result == com_protocol::SettlementResult::PlayerDisconnect)
	{
	    sendPkgMsgToClient(settlementNotify, XiangQiSrvProtocol::XIANGQISRV_SETTLEMENT_RESULT_NOTIFY, winUserData->username, "player escape settlement result notify");
	}
	else
	{
		sendPkgMsgToPlayers(settlementNotify, XiangQiSrvProtocol::XIANGQISRV_SETTLEMENT_RESULT_NOTIFY, xqCP, "settlement result notify");
	}

	OptInfoLog("finish settlement, result = %d, winner = %s, loser = %s, value = %u, base rate = %u, current rate = %u, first double = %u, winner life = %d, loser life = %d",
	settlementNotify.result(), settlementNotify.winner().c_str(), settlementNotify.loser().c_str(), settlementNotify.value(), settlementNotify.base_rate(), settlementNotify.cur_rate(),
	settlementNotify.first_double(), settlementNotify.winner_life(), settlementNotify.loser_life());

    const bool winnerIsRobot = m_robotManager.isRobot(winUserData->username);
	const bool loserIsRobot = m_robotManager.isRobot(lostUserData->username);
	
	if (result != com_protocol::SettlementResult::ServiceUnload)
	{
		// 赢家
		const char* robotUserName = "R00001";  // 机器人信息数据入库的固定ID
		com_protocol::SetFFChessUserMatchResultReq setMatchResultReq;  // 设置对局结果信息
		com_protocol::SFFChessMatchResult* matchResultInfo = setMatchResultReq.add_match_result();
		
		matchResultInfo->set_username(winUserData->username);
		if (winnerIsRobot)
		{
			matchResultInfo->set_username(robotUserName);
			m_robotManager.finishPlayChess(winUserData, settlementNotify.value());
		}
		
		matchResultInfo->set_result(matchResult);
		(matchResult == com_protocol::EFFChessMatchResult::EFFChessMatchWin) ? matchResultInfo->set_rate(curRate) : matchResultInfo->set_rate(0);
		if (settlementNotify.value() > 0) matchResultInfo->set_win_gold(settlementNotify.value());
		
		// 输家
		matchResultInfo = setMatchResultReq.add_match_result();
		
		matchResultInfo->set_username(lostUserData->username);
		if (loserIsRobot)
		{
			matchResultInfo->set_username(robotUserName);
			m_robotManager.finishPlayChess(lostUserData, 0, settlementNotify.value());
		}
		
		matchResultInfo->set_result(com_protocol::EFFChessMatchResult::EFFChessMatchLost);
		matchResultInfo->set_rate(0);
		if (settlementNotify.value() > 0) matchResultInfo->set_lost_gold(settlementNotify.value());
		
		const char* msgData = serializeMsgToBuffer(setMatchResultReq, "set user match result request");
		if (msgData != NULL) sendMessageToService(ServiceType::DBProxyCommonSrv, msgData, setMatchResultReq.ByteSize(), CommonDBSrvProtocol::BUSINESS_SET_FF_CHESS_MATCH_RESULT_REQ,
												  setMatchResultReq.match_result(0).username().c_str(), setMatchResultReq.match_result(0).username().length());
	}

	if (!winnerIsRobot && !loserIsRobot)  // 双方都不是机器人则可以 再来一局 操作
	{
		winUserData->playAgainId = createPlayAgainInfo(cfg.common_cfg.play_again_wait_secs, xqCP->sideUserName[EXQSideFlag::ERedSide], xqCP->sideUserName[EXQSideFlag::EBlackSide]);
		if (winUserData->playAgainId != 0) lostUserData->playAgainId = winUserData->playAgainId;
	}
	else if (winnerIsRobot) m_robotManager.releaseRobotData(winUserData);  // 释放机器人资源
	else if (loserIsRobot) m_robotManager.releaseRobotData(lostUserData);  // 释放机器人资源
	
	// 释放资源
	m_onPlayGameUsers.erase(xqCP->sideUserName[EXQSideFlag::ERedSide]);
	m_onPlayGameUsers.erase(xqCP->sideUserName[EXQSideFlag::EBlackSide]);
	m_memForXQChessPieces.put((char*)xqCP);
}

// 创建再来一局信息
unsigned int CSrvMsgHandler::createPlayAgainInfo(const unsigned int waitSecs, const char* redUser, const char* blackUser)
{
	if (++m_playAgainCurrentId == 0) m_playAgainCurrentId = 1;
	
	const unsigned int playAgainTimerId = setTimer(waitSecs * MillisecondUnit, (TimerHandler)&CSrvMsgHandler::waitForPlayAgainTimeOut, m_playAgainCurrentId);
    if (playAgainTimerId != 0)
	{
		m_playAgainInfo[m_playAgainCurrentId] = SPlayAgainInfo(playAgainTimerId, redUser, blackUser);
		
		return m_playAgainCurrentId;
	}
	
	return 0;
}
	
// 清除再来一局信息
void CSrvMsgHandler::destroyPlayAgainInfo(unsigned int& playAgainId, const bool isKillTimer)
{
	if (playAgainId != 0)
	{
		SPlayAgainInfoMap::iterator it = m_playAgainInfo.find(playAgainId);
		if (it != m_playAgainInfo.end())
		{
			if (isKillTimer) killTimer(it->second.timerId);
			m_playAgainInfo.erase(it);
		}
		
		playAgainId = 0;
	}
}

SPlayAgainInfo* CSrvMsgHandler::getPlayAgainInfo(const unsigned int playAgainId)
{
	SPlayAgainInfoMap::iterator it = m_playAgainInfo.find(playAgainId);
	return (it != m_playAgainInfo.end()) ? &it->second : NULL;
}

// 等待玩家再来一局超时
void CSrvMsgHandler::waitForPlayAgainTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	destroyPlayAgainInfo((unsigned int&)userId, false);
}

// 任务统计
void CSrvMsgHandler::taskStatistics(const TaskStatisticsOpt tsOpt, XQChessPieces* xqCP, const short value)
{
    switch (tsOpt)
	{
		case TaskStatisticsOpt::ETSOpenPieces:
		{
			// 任务（对方将帅一出就被吃）统计
			if (value == com_protocol::EChessPiecesValue::EHongQiJiang)
			{
				xqCP->eatOpponentGeneral[EXQSideFlag::EBlackSide] = xqCP->moveCount[EXQSideFlag::EBlackSide];
			}
			else if (value == com_protocol::EChessPiecesValue::EHeiQiJiang)
			{
				xqCP->eatOpponentGeneral[EXQSideFlag::ERedSide] = xqCP->moveCount[EXQSideFlag::ERedSide];
			}
			
			break;
		}
		
		case TaskStatisticsOpt::ETSEatOpponentPieces:
		{
			// 任务（对方将帅一出就被吃）统计
			if (value == com_protocol::EChessPiecesValue::EHongQiJiang
			    && (xqCP->eatOpponentGeneral[EXQSideFlag::EBlackSide] == xqCP->moveCount[EXQSideFlag::EBlackSide]
				    || (xqCP->eatOpponentGeneral[EXQSideFlag::EBlackSide] + 1) == xqCP->moveCount[EXQSideFlag::EBlackSide]))
			{
				xqCP->eatOpponentGeneral[EXQSideFlag::EBlackSide] = -1;  // 表示对方将帅一出就被吃
			}
			else if (value == com_protocol::EChessPiecesValue::EHeiQiJiang
			    && (xqCP->eatOpponentGeneral[EXQSideFlag::ERedSide] == xqCP->moveCount[EXQSideFlag::ERedSide]
				    || (xqCP->eatOpponentGeneral[EXQSideFlag::ERedSide] + 1) == xqCP->moveCount[EXQSideFlag::ERedSide]))
			{
				xqCP->eatOpponentGeneral[EXQSideFlag::ERedSide] = -1;  // 表示对方将帅一出就被吃
			}
			
			break;
		}
		
		case TaskStatisticsOpt::ETSPaoEatPieces:
		{
			// 任务（一局最大炮吃子次数）统计
			++xqCP->paoEatPiecesCount[xqCP->curOptSide];
			
			break;
		}
		
		case TaskStatisticsOpt::ETSContinueHit:
		{
			// 任务（一局最大连击数）统计
			if (xqCP->curHitValue[value] > xqCP->maxContinueHitValue[value]) xqCP->maxContinueHitValue[value] = xqCP->curHitValue[value];
			
			break;
		}
		
		default:
		{
			break;
		}
	}
}

// 获取翻翻棋场次信息
void CSrvMsgHandler::getChessArenaInfo(const unsigned short mode, google::protobuf::RepeatedPtrField<com_protocol::ClientChessArenaInfo>* list)
{
	if (mode == com_protocol::EFFChessMode::EClassicMode)
	{
		const map<unsigned int, XiangQiConfig::ChessArenaConfig>& arenaCfg = XiangQiConfig::config::getConfigValue().classic_arena_cfg;
		for (map<unsigned int, XiangQiConfig::ChessArenaConfig>::const_iterator it = arenaCfg.begin(); it != arenaCfg.end(); ++it)
		{
		    com_protocol::ClientChessArenaInfo* caInfo = list->Add();
			caInfo->set_id(it->first);
			caInfo->set_name(it->second.name);
			caInfo->set_base_rate(it->second.base_rate);
			caInfo->set_min_gold(it->second.min_gold);
			caInfo->set_max_gold(it->second.max_gold);
			caInfo->set_service_cost(it->second.service_cost);
			caInfo->set_online_users(m_onlinePlayerCount[mode][it->first]);
		}
	}
}

// 检查进场合法性
int CSrvMsgHandler::checkArenaCondition(const XQUserData* ud)
{
	if (ud->playMode != com_protocol::EFFChessMode::EClassicMode)
	{
		return XiangQiInvalidFFChesssMode;
	}
	
	map<unsigned int, XiangQiConfig::ChessArenaConfig>::const_iterator arenaCfgIt = XiangQiConfig::config::getConfigValue().classic_arena_cfg.find(ud->arenaId);
	if (arenaCfgIt == XiangQiConfig::config::getConfigValue().classic_arena_cfg.end())
	{
		return XiangQiInvalidFFChesssArena;
	}	
	
	if (ud->cur_gold < arenaCfgIt->second.min_gold || ud->cur_gold < arenaCfgIt->second.service_cost) // 游戏币不够
	{
		return XiangQiGoldNotEnough;
	}
	
	if (ud->cur_gold > arenaCfgIt->second.max_gold) // 游戏币过多，得进高倍场
	{
		return XiangQiMoreGoldError;
	}
	
	return Opt_Success;
}

// 匹配对手进入游戏
bool CSrvMsgHandler::matchingOpponentEnterGame(XQUserData* userData)
{
	int rc = checkArenaCondition(userData);
	if (rc != Opt_Success)
	{
		OptErrorLog("matching opponent check arena condition error, name = %s, mode = %d, id = %d, result = %d", userData->username, userData->playMode, userData->arenaId, rc);
		return false;
	}
	
	string& waitUser = m_waitForMatchingUsers[userData->playMode][userData->arenaId];
	if (waitUser.empty())  // 没有等待的玩家
	{
		destroyPlayAgainInfo(userData->playAgainId);
		waitUser = userData->username;  // 当前的玩家进入等待区
		userData->waitTimer = setTimer(XiangQiConfig::config::getConfigValue().common_cfg.wait_matching_secs * MillisecondUnit,
									   (TimerHandler)&CSrvMsgHandler::waitMatchingTimeOut, userData->playMode, (void*)((long long)userData->arenaId));
									   
		OptWarnLog("Only Matching Test, waiting for opponent, user = %s, mode = %d, arena id = %d", userData->username, userData->playMode, userData->arenaId);
									   
		return false;
	}
	
	if (waitUser == userData->username) return false;  // 相同玩家
	
	matchingOpponentEnterGame(waitUser, userData);
	
	waitUser.clear();  // 最后记得清空等待的玩家
	
	return true;
}

// 匹配对手进入游戏
void CSrvMsgHandler::matchingOpponentEnterGame(const string& waitUser, XQUserData* userData)
{
	OptWarnLog("Only Matching Test, wait user = %s, current user = %s, mode = %d, arena id = %d", waitUser.c_str(), userData->username, userData->playMode, userData->arenaId);
	
    // 先扣除双方的服务费，刷新到DB
	const XiangQiConfig::config& cfg = XiangQiConfig::config::getConfigValue();  // 配置信息
	const XiangQiConfig::ChessArenaConfig& classic_common_cfg = cfg.classic_arena_cfg.at(userData->arenaId);
	
	MoreUserRecordItemVector userRecords;
	userRecords.push_back(MoreUserRecordItem());
	MoreUserRecordItem& recordInfo = userRecords.back();
	recordInfo.items.push_back(RecordItem(EPropType::PropGold, -classic_common_cfg.service_cost));  // 服务费
	recordInfo.src_username = userData->username;
	recordInfo.remark = "进场服务费";
	
	userRecords.push_back(MoreUserRecordItem());
	MoreUserRecordItem& waitRecordInfo = userRecords.back();
	waitRecordInfo.items.push_back(RecordItem(EPropType::PropGold, -classic_common_cfg.service_cost));  // 服务费
	waitRecordInfo.src_username = waitUser;
	waitRecordInfo.remark = "进场服务费";

	notifyDbProxyChangeProp(userRecords, EChangeGoodsFlag::EEnterGameServiceCost, classic_common_cfg.base_rate, classic_common_cfg.name.c_str());

	// 随机确定红黑双方
	destroyPlayAgainInfo(userData->playAgainId);
	userData->cur_gold -= classic_common_cfg.service_cost;
	userData->sideFlag = getRandNumber(EXQSideFlag::ERedSide, EXQSideFlag::EBlackSide);
	
	XQUserData* otherSideUserData = getUserData(waitUser.c_str());
	if (otherSideUserData->waitTimer != 0)
	{
		killTimer(otherSideUserData->waitTimer);
		otherSideUserData->waitTimer = 0;
	}
	destroyPlayAgainInfo(otherSideUserData->playAgainId);
	otherSideUserData->cur_gold -= classic_common_cfg.service_cost;
	otherSideUserData->sideFlag = (userData->sideFlag + 1) % EXQSideFlag::ESideFlagCount;

	// 匹配成功了则通知客户端双方对阵的玩家
	com_protocol::ClientMatchingNotify notify;
	com_protocol::XiangQiUserBaseInfo* firstUser = notify.add_user_info();  // 第一位玩家则为指定的抢红争先玩家
	firstUser->set_username(userData->username);
	firstUser->set_nickname(userData->nickname);
	firstUser->set_portrait_id(userData->portrait_id);
	firstUser->set_game_gold(userData->cur_gold);
	
	com_protocol::XiangQiUserBaseInfo* secondUser = notify.add_user_info();
	if (userData->sideFlag != EXQSideFlag::ERedSide)
	{
		*secondUser = *firstUser;
		firstUser->set_username(otherSideUserData->username);
		firstUser->set_nickname(otherSideUserData->nickname);
		firstUser->set_portrait_id(otherSideUserData->portrait_id);
		firstUser->set_game_gold(otherSideUserData->cur_gold);
	}
	else
	{
		secondUser->set_username(otherSideUserData->username);
		secondUser->set_nickname(otherSideUserData->nickname);
		secondUser->set_portrait_id(otherSideUserData->portrait_id);
		secondUser->set_game_gold(otherSideUserData->cur_gold);
	}
	
	// 展示的棋盘&棋子，优先选用等待着一方的信息
	notify.set_board_id(userData->chessBoardId);
	notify.set_pieces_id(userData->chessPiecesId);
	if (otherSideUserData->chessBoardId > 0) notify.set_board_id(otherSideUserData->chessBoardId);
	if (otherSideUserData->chessPiecesId > 0) notify.set_pieces_id(otherSideUserData->chessPiecesId);
	
	// 创建棋局
	XQChessPieces* xqCP = generateXQChessPieces(firstUser->username().c_str(), secondUser->username().c_str());
	xqCP->curOptSide = EXQSideFlag::ERedSide;
	xqCP->firstDoubleValue = 1;
	xqCP->curLife[EXQSideFlag::ERedSide] = classic_common_cfg.life;
	xqCP->curLife[EXQSideFlag::EBlackSide] = classic_common_cfg.life;

	notify.set_max_life(classic_common_cfg.life);
	notify.set_compete_red_secs(cfg.common_cfg.compete_red_first_secs);
	notify.set_service_cost(classic_common_cfg.service_cost);
	notify.set_cur_rate(getCurrentRate(xqCP));
	notify.set_please_peace_times(cfg.common_cfg.please_peace_max_times);
	
	const unordered_map<unsigned int, unsigned int>& settlement_rate_value = cfg.settlement_value;
	for (unsigned int idx = 0; idx < OpenChessPiecesCount; ++idx)
	{
		com_protocol::ChessPiecesPosition* cpPosition = notify.add_chess_position();
		cpPosition->set_position(OpenChessPiecesPosition[idx]);
		cpPosition->set_chess_pieces((com_protocol::EChessPiecesValue)xqCP->openChessPiecesValue[idx]);  // 默认翻开棋子的4个位置
		cpPosition->set_rate(settlement_rate_value.at(xqCP->openChessPiecesValue[idx]));
	}
	
	// 通知对阵的双方玩家
	sendPkgMsgToPlayers(notify, XiangQiSrvProtocol::XIANGQISRV_MATCHING_NOTIFY, xqCP, "matching notify");
	
	// 红方如果是机器人则随机抢红
	if (!robotCompeteFirstRed(xqCP, EXQSideFlag::ERedSide, (TimerHandler)&CSrvMsgHandler::getCompeteFirstRedTimeOut))
	{
		 // 抢红争先定时器
        xqCP->curOptTimerId = setTimer(notify.compete_red_secs() * MillisecondUnit, (TimerHandler)&CSrvMsgHandler::getCompeteFirstRedTimeOut, 0, xqCP);
	}
	
	OptInfoLog("matching opponent, red user = %s, black user = %s, rate = %d, boardId = %u, piecesId = %u",
	firstUser->username().c_str(), secondUser->username().c_str(), notify.cur_rate(), notify.board_id(), notify.pieces_id());
}

// 等待匹配对手玩家超时
void CSrvMsgHandler::waitMatchingTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	string& waitUser = m_waitForMatchingUsers[userId][(unsigned int)((long long)param)];
	if (waitUser.empty()) return;
	
	XQUserData* waitUserData = getUserData(waitUser.c_str());
	if (waitUserData != NULL && waitUserData->waitTimer == timerId)
	{
		waitUserData->waitTimer = 0;
		
		// 该玩家还在等待则触发机器人出场
		XQUserData* robotData = m_robotManager.createRobotData(waitUserData->cur_gold, waitUserData->playMode, waitUserData->arenaId);
		if (robotData == NULL) return;  // 没有机器人了
		
		matchingOpponentEnterGame(robotData->username, waitUserData);
		
		waitUser.clear();
	}
}

// 发送玩家准备操作结果，通知双方玩家
void CSrvMsgHandler::sendReadyOptResultNotify(const XQChessPieces* xqCP, const char* username, const com_protocol::EPlayerReadyOpt optResult)
{
	com_protocol::ClientReadyOptResultNotify readyOptResultNotify;
	readyOptResultNotify.set_opt_result(optResult);
	readyOptResultNotify.set_opt_username(username);
	sendPkgMsgToPlayers(readyOptResultNotify, XiangQiSrvProtocol::XIANGQISRV_READY_OPT_RESULT_NOTIFY, xqCP, "ready opt result notify");
}

// 机器人抢红
bool CSrvMsgHandler::robotCompeteFirstRed(XQChessPieces* xqCP, EXQSideFlag side, TimerHandler handler)
{
	// 如果是机器人则条件抢红、随机抢红
	if (m_robotManager.isRobot(xqCP->sideUserName[side]))
	{
		bool hasHongQiPao = false;
		bool hasHeiQiPao = false;
		for (unsigned int idx = 0; idx < OpenChessPiecesCount; ++idx)
		{
			if (xqCP->openChessPiecesValue[idx] == com_protocol::EChessPiecesValue::EHongQiPao) hasHongQiPao = true;
			else if (xqCP->openChessPiecesValue[idx] == com_protocol::EChessPiecesValue::EHeiQiPao) hasHeiQiPao = true;
		}
		
		if (hasHeiQiPao && !hasHongQiPao) return false;  // 存在黑棋炮但不存在红棋炮则不抢红
		
		const XiangQiConfig::config& cfg = XiangQiConfig::config::getConfigValue();  // 配置信息
		if ((hasHongQiPao && !hasHeiQiPao) || getRandNumber(1, PercentValue) < (int)cfg.robot_cfg.robot_ready_opt.get_red_first_rate)
		{
			unsigned int minSecs = cfg.robot_cfg.robot_ready_opt.min_red_first_secs;
			unsigned int maxSecs = cfg.robot_cfg.robot_ready_opt.max_red_first_secs;
			if (minSecs > maxSecs) minSecs = maxSecs;
			if (minSecs > cfg.common_cfg.compete_red_first_secs) minSecs = cfg.common_cfg.compete_red_first_secs;
			if (maxSecs > cfg.common_cfg.compete_red_first_secs) maxSecs = cfg.common_cfg.compete_red_first_secs;
			
			// 机器人概率抢红成功
			return (setTimer(getRandNumber(minSecs, maxSecs) * MillisecondUnit, handler, RobotReadyOptFlag, xqCP) != 0);
		}
	}
	
	return false;
}

// 当前一方确认抢红
void CSrvMsgHandler::confirmCompeteFirstRed(XQChessPieces* xqCP)
{
	xqCP->firstDoubleValue *= 2;  // 抢红翻倍
	if (xqCP->curOptSide == EXQSideFlag::EBlackSide)  // 黑方抢红
	{
		// 交换红黑双方ID
		SideUserName sideUserName = {0};
		const unsigned int sUNLen = sizeof(SideUserName) - 1;
		strncpy(sideUserName, xqCP->sideUserName[EXQSideFlag::ERedSide], sUNLen);
		strncpy(xqCP->sideUserName[EXQSideFlag::ERedSide], xqCP->sideUserName[EXQSideFlag::EBlackSide], sUNLen);
		strncpy(xqCP->sideUserName[EXQSideFlag::EBlackSide], sideUserName, sUNLen);
		
		XQUserData* readUD = getUserData(xqCP->sideUserName[EXQSideFlag::ERedSide]);
		if (readUD != NULL) readUD->sideFlag = EXQSideFlag::ERedSide;
		
		XQUserData* blackUD = getUserData(xqCP->sideUserName[EXQSideFlag::EBlackSide]);
		if (blackUD != NULL) blackUD->sideFlag = EXQSideFlag::EBlackSide;
	}
}

// 红方确认抢红争先超时
void CSrvMsgHandler::getCompeteFirstRedTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
    // 发送玩家准备操作结果，通知双方玩家
	XQChessPieces* xqCP = (XQChessPieces*)param;
    sendReadyOptResultNotify(xqCP, xqCP->sideUserName[EXQSideFlag::ERedSide],
							 (userId == RobotReadyOptFlag) ? com_protocol::EPlayerReadyOpt::EGetCompeteRed : com_protocol::EPlayerReadyOpt::ENotCompeteRed);
	
	if (userId == RobotReadyOptFlag)  // 机器人抢红了
	{
		confirmCompeteFirstRed(xqCP);
			
		// 询问未抢红的一方是否需要加倍加注
		getAddDoubleInquiry(xqCP, EXQSideFlag::EBlackSide);
	}
	else
	{
	    // 红方超时则再询问黑方是否需要抢红争先
	    getCompeteFirstBlackInquiry(xqCP);
	}
}

// 询问黑方是否抢红争先
void CSrvMsgHandler::getCompeteFirstBlackInquiry(XQChessPieces* xqCP)
{
	xqCP->curOptSide = EXQSideFlag::EBlackSide;
	com_protocol::ClientReadyOptNotify notify;
	notify.set_opt_value(com_protocol::EPlayerReadyOpt::EGetCompeteRed);
	notify.set_opt_username(xqCP->sideUserName[EXQSideFlag::EBlackSide]);
	notify.set_wait_secs(XiangQiConfig::config::getConfigValue().common_cfg.compete_red_first_secs);
	
	sendPkgMsgToPlayers(notify, XiangQiSrvProtocol::XIANGQISRV_READY_OPT_NOTIFY, xqCP, "get compete first red reply");
	
	// 黑方如果是机器人则随机抢红
	if (!robotCompeteFirstRed(xqCP, EXQSideFlag::EBlackSide, (TimerHandler)&CSrvMsgHandler::getCompeteFirstBlackTimeOut))
	{
		xqCP->curOptTimerId = setTimer(notify.wait_secs() * MillisecondUnit, (TimerHandler)&CSrvMsgHandler::getCompeteFirstBlackTimeOut, 0, xqCP);
	}
}

// 黑方确认抢红争先超时
void CSrvMsgHandler::getCompeteFirstBlackTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 发送玩家准备操作结果，通知双方玩家
	XQChessPieces* xqCP = (XQChessPieces*)param;
    sendReadyOptResultNotify(xqCP, xqCP->sideUserName[EXQSideFlag::EBlackSide],
							 (userId == RobotReadyOptFlag) ? com_protocol::EPlayerReadyOpt::EGetCompeteRed : com_protocol::EPlayerReadyOpt::ENotCompeteRed);
							 
	if (userId == RobotReadyOptFlag) confirmCompeteFirstRed(xqCP); // 机器人抢红了

	// 机器人抢红了
	// 或者黑方玩家超时则表示双方都不抢红争先，则默认红方争先
	// 接下来询问黑方是否需要加倍加注
	getAddDoubleInquiry(xqCP, EXQSideFlag::EBlackSide);
}

// 机器人加倍
bool CSrvMsgHandler::robotAddDouble(XQChessPieces* xqCP, EXQSideFlag side, TimerHandler handler)
{
	// 如果是机器人则随机加倍
	const XiangQiConfig::config& cfg = XiangQiConfig::config::getConfigValue();  // 配置信息
	if (m_robotManager.isRobot(xqCP->sideUserName[side]) && getRandNumber(1, PercentValue) < (int)cfg.robot_cfg.robot_ready_opt.add_double_rate)
	{
		unsigned int minSecs = cfg.robot_cfg.robot_ready_opt.min_add_double_secs;
		unsigned int maxSecs = cfg.robot_cfg.robot_ready_opt.max_add_double_secs;
		if (minSecs > maxSecs) minSecs = maxSecs;
		if (minSecs > cfg.common_cfg.add_double_secs) minSecs = cfg.common_cfg.add_double_secs;
		if (maxSecs > cfg.common_cfg.add_double_secs) maxSecs = cfg.common_cfg.add_double_secs;
		
		// 机器人概率加倍成功
		return (setTimer(getRandNumber(minSecs, maxSecs) * MillisecondUnit, handler, RobotReadyOptFlag, xqCP) != 0);
	}
	
	return false;
}

// 询问某一方是否加倍加注
void CSrvMsgHandler::getAddDoubleInquiry(XQChessPieces* xqCP, EXQSideFlag side)
{
	xqCP->curOptSide = side;
	
	com_protocol::ClientReadyOptNotify notify;
	notify.set_opt_value(com_protocol::EPlayerReadyOpt::EGetAddDouble);
	notify.set_opt_username(xqCP->sideUserName[side]);
	notify.set_cur_rate(getCurrentRate(xqCP));
	notify.set_wait_secs(XiangQiConfig::config::getConfigValue().common_cfg.add_double_secs);
	
	sendPkgMsgToPlayers(notify, XiangQiSrvProtocol::XIANGQISRV_READY_OPT_NOTIFY, xqCP, "get add double inquiry");
	
	if (!robotAddDouble(xqCP, side, (TimerHandler)&CSrvMsgHandler::getAddDoubleTimeOut))
	{
	    xqCP->curOptTimerId = setTimer(notify.wait_secs() * MillisecondUnit, (TimerHandler)&CSrvMsgHandler::getAddDoubleTimeOut, 0, xqCP);
	}
}

// 等待加倍操作超时
void CSrvMsgHandler::getAddDoubleTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 发送玩家准备操作结果，通知双方玩家
	XQChessPieces* xqCP = (XQChessPieces*)param;
    sendReadyOptResultNotify(xqCP, xqCP->sideUserName[xqCP->curOptSide],
							 (userId == RobotReadyOptFlag) ? com_protocol::EPlayerReadyOpt::EGetAddDouble : com_protocol::EPlayerReadyOpt::ENotAddDouble);
							
	if (userId == RobotReadyOptFlag) xqCP->firstDoubleValue *= 2;  // 翻倍
	
	// 加倍确认操作超时，则不存在加倍动作，红方开始下棋
	moveChessPiecesNotify(xqCP, EXQSideFlag::ERedSide);
}

// 机器人拒绝求和
void CSrvMsgHandler::robotRefusePeace(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	char robotId[MaxConnectIdLen] = {0};
	m_robotManager.generateRobotId(userId, robotId, MaxConnectIdLen - 1);  // 生成机器人ID

	XQChessPieces* xqCP = getXQChessPieces(robotId);
	if (xqCP != NULL)
	{
		com_protocol::ClientPlayerPleasePeaceRsp rsp;
	    rsp.set_result(XiangQiPleasePeaceOpponentRefuse);
	
		const unsigned short pleasePeaceSide = ((unsigned int)(long long)param) % EXQSideFlag::ESideFlagCount;
		++xqCP->pleasePeace[pleasePeaceSide];
		rsp.set_remain_times(XiangQiConfig::config::getConfigValue().common_cfg.please_peace_max_times - xqCP->pleasePeace[pleasePeaceSide]);
		sendPkgMsgToClient(rsp, XiangQiSrvProtocol::XIANGQISRV_PLEASE_PEACE_RSP, xqCP->sideUserName[pleasePeaceSide], "robot refuse peace confirm");
	}
}

// 玩家走棋子通知
void CSrvMsgHandler::moveChessPiecesNotify(XQChessPieces* xqCP, EXQSideFlag side)
{
	xqCP->curOptSide = side;
	com_protocol::ClientMoveChessPiecesNotify mvNotify;
	mvNotify.set_red_username(xqCP->sideUserName[EXQSideFlag::ERedSide]);
	mvNotify.set_wait_secs(XiangQiConfig::config::getConfigValue().common_cfg.play_chess_pieces_secs);
	mvNotify.set_cur_rate(getCurrentRate(xqCP));
	
	sendPkgMsgToPlayers(mvNotify, XiangQiSrvProtocol::XIANGQISRV_MOVE_CHESS_PIECES_NOTIFY, xqCP, "move chess pieces");	
	xqCP->curOptTimerId = setTimer(mvNotify.wait_secs() * MillisecondUnit, (TimerHandler)&CSrvMsgHandler::moveChessPiecesTimeOut, 0, xqCP);
	
    m_robotManager.robotPlayChessPieces(xqCP->sideUserName[EXQSideFlag::ERedSide], xqCP);  // 如果是机器人则机器人走动棋子
}

void CSrvMsgHandler::moveChessPiecesTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 走棋子超时则默认失败，直接触发结算
	XQChessPieces* xqCP = (XQChessPieces*)param;
	if (xqCP->curOptTimerId == timerId) xqCP->curOptTimerId = 0;
	
	OptWarnLog("move chess pieces time out and to do settlement, side = %d, life = %d, user = %s",
	xqCP->curOptSide, xqCP->curLife[xqCP->curOptSide], xqCP->sideUserName[xqCP->curOptSide]);
	
	finishSettlement(xqCP, xqCP->curOptSide, com_protocol::SettlementResult::PlayTimeOut);
}

// 玩家下棋走子
void CSrvMsgHandler::userPlayChessPieces(const XQUserData* xqUD, const com_protocol::ClientPlayChessPiecesNotify& plCPNotify, int& errorCode)
{
	errorCode = Opt_Success;
	
	if (plCPNotify.src_position() < MinChessPiecesPosition || plCPNotify.src_position() > MaxChessPieCesPosition
	    || plCPNotify.dst_position() < MinChessPiecesPosition || plCPNotify.dst_position() > MaxChessPieCesPosition)
	{
		OptErrorLog("play chess pieces notify the param error, name = %s, src position = %d, dst position = %d",
		xqUD->username, plCPNotify.src_position(), plCPNotify.dst_position());
		return;
	}

	XQChessPieces* xqCP = getXQChessPieces(xqUD->username);
	if (xqCP == NULL)
	{
		OptErrorLog("play chess pieces notify can not find the user info, name = %s", xqUD->username);
		return;
	}
	
	const XiangQiConfig::CommConfig& commonCfg = XiangQiConfig::config::getConfigValue().common_cfg;
	if (xqCP->restrictMoveInfo.stepCount >= commonCfg.restrict_move_step && xqCP->restrictMoveInfo.srcPosition == plCPNotify.src_position())
	{
		errorCode = XiangQiRestrictMove;
		OptWarnLog("play chess pieces notify restrict move step, name = %s, src position = %d, dst position = %d, step = %d",
		xqUD->username, xqCP->restrictMoveInfo.srcPosition, xqCP->restrictMoveInfo.dstPosition, xqCP->restrictMoveInfo.stepCount);
		return;
	}
	
	bool isFinish = false;
	com_protocol::ClientPlayChessPiecesResultNotify playChessPiecesResultNotify;
	short& srcChessPiecesValue = xqCP->value[plCPNotify.src_position()];
	short hurtSide = (xqCP->curOptSide + 1) % EXQSideFlag::ESideFlagCount;  // 被伤害的一方
	if (!plCPNotify.has_dst_position())  // 只是翻开棋子
	{
		if (srcChessPiecesValue < ClosedChessPiecesValue)
		{
			OptErrorLog("open chess pieces notify the value invalid, name = %s, position = %d, value = %d",
			xqUD->username, plCPNotify.src_position(), srcChessPiecesValue);
		    return;
		}
		
		// 机器人的金币赢率如果低于配置值则启动作弊流程调整对方正准备翻开的棋子
		// 对手下棋前作弊调整对手将要翻开的棋子
		if (!m_robotManager.isRobot(xqUD->username)
		    && m_robotManager.needCheat(xqUD, xqCP, (xqCP->curOptSide + 1) % EXQSideFlag::ESideFlagCount,
										XiangQiConfig::config::getConfigValue().robot_cfg.cheat_cfg.opponent_open_cp_cheat_base_rate))
		{
			m_robotManager.opponentOpenCPCheatAdjust(xqCP->value, xqCP->curOptSide, plCPNotify.src_position(), srcChessPiecesValue);
		}
	
		srcChessPiecesValue -= ClosedChessPiecesValue;
		playChessPiecesResultNotify.set_play_result(com_protocol::EPlayChessPiecesResult::EPlayOpen);
		
		resetContinueHitValue(xqCP, xqCP->curOptSide);
		
		taskStatistics(TaskStatisticsOpt::ETSOpenPieces, xqCP, srcChessPiecesValue);  // 任务统计
		
		xqCP->allContinueStepCount = 0;
		m_logicHandler.clearRestrictMoveInfo(xqCP->restrictMoveInfo);
	}
	else
	{
		// 源棋子未翻开或者位置不移动
		if (srcChessPiecesValue >= ClosedChessPiecesValue || plCPNotify.src_position() == plCPNotify.dst_position())
		{
			OptErrorLog("play chess pieces notify the value invalid, name = %s, value = %d, src position = %d, dst position = %d",
			xqUD->username, srcChessPiecesValue, plCPNotify.src_position(), plCPNotify.dst_position());
		    return;
		}
		
		// 不是当前玩家的棋子
		if (!isSameSide(xqCP->curOptSide, srcChessPiecesValue))
		{
			errorCode = XiangQiNotOurSide;
			
			OptErrorLog("play chess pieces notify the src side invalid, name = %s, current side = %d, value = %d, src position = %d, dst position = %d",
			xqUD->username, xqCP->curOptSide, srcChessPiecesValue, plCPNotify.src_position(), plCPNotify.dst_position());
		    return;
		}

        short& dstChessPiecesValue = xqCP->value[plCPNotify.dst_position()];
		if (srcChessPiecesValue == com_protocol::EChessPiecesValue::EHongQiPao || srcChessPiecesValue == com_protocol::EChessPiecesValue::EHeiQiPao)
		{
			if (!checkMovePaoIsOK(plCPNotify.src_position(), plCPNotify.dst_position(), xqCP->curOptSide, xqCP->value, xqUD->username))  // 炮则特殊处理
			{
				errorCode = XiangQiMovePositionInvalid;
				return;
			}

			if (dstChessPiecesValue >= ClosedChessPiecesValue)
			{
				// 机器人的金币赢率如果低于配置值则启动作弊流程调整对方明炮的目标棋子
				if (!m_robotManager.isRobot(xqUD->username)
				    && m_robotManager.needCheat(xqUD, xqCP, (xqCP->curOptSide + 1) % EXQSideFlag::ESideFlagCount,
					                            XiangQiConfig::config::getConfigValue().robot_cfg.cheat_cfg.opponent_pao_cheat_base_rate))
				{
					m_robotManager.opponentPaoHitCheatAdjust(xqCP->value, xqCP->curOptSide, dstChessPiecesValue);
				}
				
				dstChessPiecesValue -= ClosedChessPiecesValue;  // 被炮吃掉的棋子可能还没有被翻开
			}
			
			if (isSameSide(xqCP->curOptSide, dstChessPiecesValue)) hurtSide = xqCP->curOptSide;
			
			taskStatistics(TaskStatisticsOpt::ETSPaoEatPieces, xqCP);  // 任务统计
			
			xqCP->allContinueStepCount = 0;
			m_logicHandler.clearRestrictMoveInfo(xqCP->restrictMoveInfo);
		}
		else
		{
			// 棋子没有翻开，或者是同一方的棋子则不能操作
			if (dstChessPiecesValue >= ClosedChessPiecesValue || isSameSide(xqCP->curOptSide, dstChessPiecesValue))
			{
				errorCode = XiangQiMovePositionInvalid;
				// OptErrorLog("play chess pieces notify the dst side invalid, name = %s, current side = %d, value = %d",
			    // xqUD->username, xqCP->curOptSide, dstChessPiecesValue);
		        return;
			}

			if (!canMoveToPosition(plCPNotify.src_position(), plCPNotify.dst_position()))
			{
				errorCode = XiangQiMovePositionInvalid;
				// OptErrorLog("play chess pieces notify the position invalid, name = %s, src position = %d, dst position = %d",
			    // xqUD->username, plCPNotify.src_position(), plCPNotify.dst_position());
		        return;
			}
			
			if (!canEatOtherChessPieces(srcChessPiecesValue, dstChessPiecesValue))
			{
				errorCode = XiangQiCanNotEatOpponentSide;
				// OptErrorLog("play chess pieces notify the value invalid, name = %s, src value = %d, dst value = %d",
			    // xqUD->username, srcChessPiecesValue, dstChessPiecesValue);
		        return;
			}
		}
		
		playChessPiecesResultNotify.set_play_result(com_protocol::EPlayChessPiecesResult::EPlayMove);
		if (dstChessPiecesValue < com_protocol::EChessPiecesValue::ENoneQiZiClose)
		{
			playChessPiecesResultNotify.set_play_result(com_protocol::EPlayChessPiecesResult::EPlayEat);
			playChessPiecesResultNotify.set_out_chess_pieces((com_protocol::EChessPiecesValue)dstChessPiecesValue);

			xqCP->curLife[hurtSide] -= XiangQiConfig::config::getConfigValue().chess_pieces_life.at(dstChessPiecesValue);
			playChessPiecesResultNotify.set_cur_username(xqCP->sideUserName[hurtSide]);
			playChessPiecesResultNotify.set_cur_life(xqCP->curLife[hurtSide]);
			
			if (hurtSide != xqCP->curOptSide)
			{
				++xqCP->curHitValue[xqCP->curOptSide];
				if (xqCP->curHitValue[xqCP->curOptSide] >= ContinueHitTimes) playChessPiecesResultNotify.set_continue_hit(xqCP->curHitValue[xqCP->curOptSide]);
			}
			else  // 炮打到自己一方的棋子了
			{
				resetContinueHitValue(xqCP, xqCP->curOptSide);
			}
			
			playChessPiecesResultNotify.set_cur_rate(getCurrentRate(xqCP));
			
			if (xqCP->curLife[hurtSide] < 1) isFinish = true;
			
			taskStatistics(TaskStatisticsOpt::ETSEatOpponentPieces, xqCP, dstChessPiecesValue);  // 任务统计
			
			xqCP->allContinueStepCount = 0;
			m_logicHandler.clearRestrictMoveInfo(xqCP->restrictMoveInfo);
		}
		else
		{
			resetContinueHitValue(xqCP, xqCP->curOptSide);
			
			++xqCP->allContinueStepCount;
			const unsigned int moveStepCount = m_logicHandler.setRestrictMoveInfo(xqCP, plCPNotify.src_position(), plCPNotify.dst_position());
			if (moveStepCount >= commonCfg.restrict_move_prompt && moveStepCount < commonCfg.restrict_move_step)
			{
				// 禁足提示
				com_protocol::ClientRestrictMoveInfo* rstMvInfo = playChessPiecesResultNotify.mutable_restrict_move();
				rstMvInfo->set_username(xqUD->username);
				rstMvInfo->set_remain_step(commonCfg.restrict_move_step - moveStepCount);
			}
		}
			
		// 目标位置的棋子
		dstChessPiecesValue = srcChessPiecesValue;
		com_protocol::ChessPiecesPosition* dstCPInfo = playChessPiecesResultNotify.mutable_dst_position();
		dstCPInfo->set_chess_pieces((com_protocol::EChessPiecesValue)dstChessPiecesValue);
		dstCPInfo->set_position(plCPNotify.dst_position());
		
		// 源位置的棋子，此时为空位置
		srcChessPiecesValue = com_protocol::EChessPiecesValue::ENoneQiZiOpen;
	}
	
	if (xqCP->curOptTimerId != 0)
	{
		killTimer(xqCP->curOptTimerId);
		xqCP->curOptTimerId = 0;
	}
	
	// 源位置的棋子
	com_protocol::ChessPiecesPosition* srcCPInfo = playChessPiecesResultNotify.mutable_src_position();
	srcCPInfo->set_chess_pieces((com_protocol::EChessPiecesValue)srcChessPiecesValue);
	srcCPInfo->set_position(plCPNotify.src_position());
	playChessPiecesResultNotify.set_result(Opt_Success);
	
	++xqCP->moveCount[xqCP->curOptSide];
	
	// 如果还没结束的话则切换到另外一方下棋
	if (!isFinish)
	{
		xqCP->curOptSide = (xqCP->curOptSide + 1) % EXQSideFlag::ESideFlagCount;
		playChessPiecesResultNotify.set_next_opt_username(xqCP->sideUserName[xqCP->curOptSide]);
		playChessPiecesResultNotify.set_wait_secs(commonCfg.play_chess_pieces_secs);
		xqCP->curOptTimerId = setTimer(playChessPiecesResultNotify.wait_secs() * MillisecondUnit, (TimerHandler)&CSrvMsgHandler::moveChessPiecesTimeOut, 0, xqCP);
		
		if (xqCP->allContinueStepCount >= commonCfg.continue_move_prompt && xqCP->allContinueStepCount < commonCfg.continue_move_step)
		{
			playChessPiecesResultNotify.set_continue_move_remain_step(commonCfg.continue_move_step - xqCP->allContinueStepCount);  // 空走提示
		}
	}
	
	sendPkgMsgToPlayers(playChessPiecesResultNotify, XiangQiSrvProtocol::XIANGQISRV_PLAY_CHESS_RESULT_NOTIFY, xqCP, "play chess result notify");

    /*
	OptInfoLog("play chess, result = %d, src postion = %d, value = %d, dst position = %d, value = %d, out pieces = %d, hurt user = %s, life = %d, next user = %s, continue hit = %d, rate = %d",
	playChessPiecesResultNotify.play_result(), playChessPiecesResultNotify.src_position().position(), playChessPiecesResultNotify.src_position().chess_pieces(),
	playChessPiecesResultNotify.dst_position().position(), playChessPiecesResultNotify.dst_position().chess_pieces(),
	playChessPiecesResultNotify.out_chess_pieces(), playChessPiecesResultNotify.cur_username().c_str(), playChessPiecesResultNotify.cur_life(),
	playChessPiecesResultNotify.next_opt_username().c_str(), playChessPiecesResultNotify.continue_hit(), playChessPiecesResultNotify.cur_rate());
	*/
	
	if (isFinish)
	{
		finishSettlement(xqCP, hurtSide);  // 结束了则结算
	}
	else if (xqCP->allContinueStepCount >= commonCfg.continue_move_step)
	{
		finishSettlement(xqCP, (xqCP->curOptSide + 1) % EXQSideFlag::ESideFlagCount, com_protocol::SettlementResult::PleasePeace);  // 空走导致和棋了则结算
	}
	else
	{
		m_robotManager.robotPlayChessPieces(xqCP->sideUserName[xqCP->curOptSide], xqCP);  // 如果是机器人则机器人走动棋子
	}
}

// 生成棋盘棋子布局
XQChessPieces* CSrvMsgHandler::generateXQChessPieces(const char* redUserName, const char* blackUserName)
{
	/*
	// 象棋棋子值定义
	enum EChessPiecesValue
	{
		EHongQiJiang = 0;                                      // 红棋 将
		EHongQiShi = 1;                                        // 红棋 士
		EHongQiXiang = 2;                                      // 红棋 象
		EHongQiChe = 3;                                        // 红棋 车
		EHongQiMa = 4;                                         // 红棋 马
		EHongQiPao = 5;                                        // 红棋 炮
		EHongQiBing = 6;                                       // 红棋 兵
		
		EHeiQiJiang = 7;                                       // 黑棋 将
		EHeiQiShi = 8;                                         // 黑棋 士
		EHeiQiXiang = 9;                                       // 黑棋 象
		EHeiQiChe = 10;                                        // 黑棋 车
		EHeiQiMa = 11;                                         // 黑棋 马
		EHeiQiPao = 12;                                        // 黑棋 炮
		EHeiQiBing = 13;                                       // 黑棋 兵
		
		ENoneQiZiClose = 14;                                   // 存在棋子，但没有翻开
		ENoneQiZiOpen = 15;                                    // 不存在棋子，空位置
	}
	*/

	// 总共32个棋子
	static const unsigned int ChessPiecesValue[] = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 6,
													7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 13, 13, 13};
													
												
	// 32个位置索引值
	unsigned int indexs[MaxChessPieces] = {0};
	for (unsigned int idx = 0; idx < MaxChessPieces; ++idx) indexs[idx] = idx;
	
	// 随机初始化棋盘上各个位置的棋子
	XQChessPieces* xqCP = (XQChessPieces*)m_memForXQChessPieces.get();
	memset(xqCP, 0, sizeof(XQChessPieces));
	m_logicHandler.clearRestrictMoveInfo(xqCP->restrictMoveInfo);
	
	int count = MaxChessPieces - 1;
	int randIdx = 0;
	for (unsigned int valIdx = 0; valIdx < MaxChessPieces; ++valIdx)
	{
		randIdx = getRandNumber(0, count);  // 随机一个索引值
		xqCP->value[valIdx] = ChessPiecesValue[indexs[randIdx]] + ClosedChessPiecesValue;  // ClosedChessPiecesValue 初始化的棋子都是还未翻开的
		
		if (randIdx != count) indexs[randIdx] = indexs[count];  // 非最后一个位置的索引则替换
		--count;
	}
	
	// 默认翻开棋子的4个位置
	for (unsigned int idx = 0; idx < OpenChessPiecesCount; ++idx)
	{
		xqCP->openChessPiecesValue[idx] = xqCP->value[OpenChessPiecesPosition[idx]] - ClosedChessPiecesValue;
		xqCP->value[OpenChessPiecesPosition[idx]] = xqCP->openChessPiecesValue[idx];
	}
	
	strncpy(xqCP->sideUserName[EXQSideFlag::ERedSide], redUserName, sizeof(SideUserName) - 1);
	strncpy(xqCP->sideUserName[EXQSideFlag::EBlackSide], blackUserName, sizeof(SideUserName) - 1);
	
	m_onPlayGameUsers[redUserName] = xqCP;
	m_onPlayGameUsers[blackUserName] = xqCP;
	
	return xqCP;
}

XQChessPieces* CSrvMsgHandler::getXQChessPieces(const char* username)
{
	OnPlayUsers::iterator it = m_onPlayGameUsers.find(username);
	return (it != m_onPlayGameUsers.end()) ? it->second : NULL;
}
	
// 用户信息写日志
void CSrvMsgHandler::logUserInfo(const com_protocol::DetailInfo& userInfo, const char* msg)
{
	char propInfo[MaxMsgLen] = {0};
	
	/*
	unsigned int propInfoSize = 0;
	for (int idx = 0; idx < userInfo.prop_items_size(); ++idx)
	{
		const com_protocol::PropItem& propItem = userInfo.prop_items(idx);
		propInfoSize += snprintf(propInfo + propInfoSize, MaxMsgLen - propInfoSize - 1, "[%u = %u], ", propItem.type(), propItem.count());
	}
	if (propInfoSize > 2) propInfo[propInfoSize - 2] = 0;
	*/
	
	const com_protocol::StaticInfo& staticInfo = userInfo.static_info();
	const com_protocol::DynamicInfo& dynamicInfo = userInfo.dynamic_info();
	OptInfoLog("name = %s, nick = %s, vip = %u, score = %u, gold = %lu, fish coin = %u, voucher = %u, phone fare = %u, diamonds = %u, recharge = %u, login times = %u, item = %s, info = %s",
	staticInfo.username().c_str(), staticInfo.nickname().c_str(), dynamicInfo.vip_level(), dynamicInfo.score(), dynamicInfo.game_gold(), dynamicInfo.rmb_gold(), dynamicInfo.voucher_number(), dynamicInfo.phone_fare(), dynamicInfo.diamonds_number(), dynamicInfo.sum_recharge(), dynamicInfo.sum_login_times(), propInfo, msg);
}

int CSrvMsgHandler::sendMsgToProxyEx(const char* msgData, const unsigned int msgLen, unsigned short protocolId, const char* userName, unsigned int serviceId)
{
	if (m_robotManager.isRobot(userName)) return 0;  // 机器人则忽略
	
	ConnectProxy* connProxy = getConnectProxy(userName);
	if (connProxy == NULL)
	{
		OptWarnLog("send messge to connect proxy error, not find the proxy, userName = %s, protocolId = %d", (userName != NULL) ? userName : "", protocolId);
		return XiangQiNoFoundConnectProxy;
	}
	
	return sendMsgToProxy(msgData, msgLen, protocolId, connProxy, serviceId);
}

int CSrvMsgHandler::sendPkgMsgToClient(const ::google::protobuf::Message& pkg, unsigned short protocol_id, const char* username, const char* log_info)
{
	const char* msgData = serializeMsgToBuffer(pkg, log_info);
	if (msgData == NULL) return -1;
	
	if (NULL == username)
	{
		return sendMsgToProxy(msgData, pkg.ByteSize(), protocol_id);
	}
	else
	{
		return sendMsgToProxyEx(msgData, pkg.ByteSize(), protocol_id, username);
	}
}

int CSrvMsgHandler::sendPkgMsgToPlayers(const ::google::protobuf::Message& pkg, unsigned short protocol_id, const XQChessPieces* xqCP, const char* log_info)
{
	const char* msgData = serializeMsgToBuffer(pkg, log_info);
	if (msgData == NULL) return -1;
	
	int rc = sendMsgToProxyEx(msgData, pkg.ByteSize(), protocol_id, xqCP->sideUserName[EXQSideFlag::ERedSide]);
	return (rc == Opt_Success) ? sendMsgToProxyEx(msgData, pkg.ByteSize(), protocol_id, xqCP->sideUserName[EXQSideFlag::EBlackSide]) : rc;
}

void CSrvMsgHandler::changePropReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("change goods reply", userName);
	if (conn == NULL) return;
	
	com_protocol::PropChangeRsp propChangeRsp;
	if (!parseMsgFromBuffer(propChangeRsp, data, len, "change prop reply")) return;
	
	if (propChangeRsp.result() != 0) OptErrorLog("change user prop error, result = %d", propChangeRsp.result());
	
	switch (getContext().userFlag)
	{
		case EChangeGoodsFlag::EReceiveTaskReward:
		{
			break;
		}
		
		default:
		{
			break;
		}
	}

	if (propChangeRsp.result() == 0) sendGoodsChangeNotify(propChangeRsp.items(), conn);
}

void CSrvMsgHandler::changeMoreUserPropReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::MoreUsersPropChangeRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "change more user prop reply")) return;

    if (rsp.result() != 0) OptErrorLog("fan fan chess change more user prop error, result = %d", rsp.result());

	switch (getContext().userFlag)
	{
		case EChangeGoodsFlag::EEnterGameServiceCost:		
		case EChangeGoodsFlag::EFinishSettlement:
		{
			if (rsp.result() == 0)
			{
				for (google::protobuf::RepeatedPtrField<com_protocol::PropChangeRsp>::const_iterator it = rsp.prop_change_rsp().begin(); it != rsp.prop_change_rsp().end(); ++it)
				{
					sendGoodsChangeNotify(it->items(), NULL, it->src_username().c_str());
				}
			}

			break;
		}
		
		default:
		{
			break;
		}
	}
}

void CSrvMsgHandler::rechargeNotifyResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::RechargeFishCoinResultNotify resultNotify;
	if (!parseMsgFromBuffer(resultNotify, data, len, "recharge fish coin result notify")) return;
	
	if (resultNotify.result() != 0)
	{
		OptErrorLog("recharge notify result error, user = %s, result = %d", getContext().userData, resultNotify.result());
		return;
	}
	
	m_logicHandler.rechargeNotify(getContext().userData, resultNotify.money());

    /*
	// 转发充值消息到大厅
	com_protocol::LoginForwardMessageNotify loginForwardMsgNotify;
	loginForwardMsgNotify.set_type(ELoginForwardMessageNotifyType::ERechargeValue);
	loginForwardMsgNotify.set_recharge_value(resultNotify.money());
	loginForwardMsgNotify.SerializeToArray(send_data, MaxMsgLen);
	forwardMessageToLogin(userInfo->username, strlen(userInfo->username), send_data, loginForwardMsgNotify.ByteSize(), LoginBusiness::GAME_FORWARD_MESSAGE_TO_HALL_NOTIFY);
	*/ 
}

// 添加新用户基本信息
void CSrvMsgHandler::addUserBaseInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::AddFFChessUserBaseInfoRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "add user base info reply")) return;
	
	if (rsp.result() != 0) OptErrorLog("add user base info error, user = %s, result = %d", getContext().userData, rsp.result());
}

int CSrvMsgHandler::notifyDbProxyChangeProp(const char* username, unsigned int len, const RecordItemVector& recordItemVector, int flag,
											const char* remark, const int rate, const char* gameName, const char* recordId, const char* dst_username, unsigned short handleProtocolId)
{
	if (m_robotManager.isRobot(username)) return Opt_Failed;  // 机器人则忽略
	
	// 通知dbproxy变更道具数量
	com_protocol::PropChangeReq changeGoodsReq;
	for (RecordItemVector::const_iterator it = recordItemVector.begin(); it != recordItemVector.end(); ++it)
	{
		com_protocol::ItemChangeInfo* iChangeInfo = changeGoodsReq.add_items();
		iChangeInfo->set_type(it->type);
		iChangeInfo->set_count(it->count);
	}

	if (changeGoodsReq.items_size() > 0)
	{
		if (dst_username != NULL) changeGoodsReq.set_dst_username(dst_username);

		if (remark != NULL)
		{
			com_protocol::GameRecordPkg* p_game_record = changeGoodsReq.mutable_game_record();
			makeRecordPkg(*p_game_record, recordItemVector, remark, rate, gameName, recordId);
		}
		
		const char* msgData = serializeMsgToBuffer(changeGoodsReq, "notify dbproxy change goods count");
		if (msgData != NULL)
		{
			return sendMessageToService(DBProxyCommonSrv, msgData, changeGoodsReq.ByteSize(), CommonDBSrvProtocol::BUSINESS_CHANGE_PROP_REQ, username, len, flag, handleProtocolId);
		}
	}
	
	return Opt_Failed;
}

int CSrvMsgHandler::notifyDbProxyChangeProp(const MoreUserRecordItemVector& recordVector, int flag, const int rate, const char* gameName, const char* recordId, unsigned short handleProtocolId)
{
	com_protocol::MoreUsersPropChangeReq req;
	for (auto itemIt = recordVector.begin(); itemIt != recordVector.end(); ++itemIt)
	{
		if (m_robotManager.isRobot(itemIt->src_username.c_str())) continue;  // 机器人则忽略
		
		com_protocol::PropChangeReq* pPropChange = req.add_prop_change_req();
		for (RecordItemVector::const_iterator it = itemIt->items.begin(); it != itemIt->items.end(); ++it)
		{
			auto items = pPropChange->add_items();
			items->set_type(it->type);
			items->set_count(it->count);
		}
		
		pPropChange->set_src_username(itemIt->src_username);
		if (!(itemIt->dst_username.empty())) pPropChange->set_dst_username(itemIt->dst_username);

		if (!(itemIt->remark.empty()))
		{
			com_protocol::GameRecordPkg* p_game_record = pPropChange->mutable_game_record();
			makeRecordPkg(*p_game_record, itemIt->items, itemIt->remark.c_str(), rate, gameName, recordId);
		}
	}

    int rc = Opt_Failed;
    const char* msgData = serializeMsgToBuffer(req, "notify dbproxy change many users goods count");
	if (msgData != NULL)
	{
		rc = sendMessageToService(DBProxyCommonSrv, msgData, req.ByteSize(), CommonDBSrvProtocol::BUSINESS_CHANGE_MANY_PEOPLE_PROP_REQ, "", 0, flag, handleProtocolId);
	}
	
	return rc;
}

void CSrvMsgHandler::makeRecordPkg(com_protocol::GameRecordPkg &pkg, const RecordItemVector& items, const string &remark, const int rate, const char* gameName, const char* recordId)
{
	if (items.size() < 1) return;

	com_protocol::BuyuGameRecordStorageExt buyu_record;
	buyu_record.set_room_rate(rate);
	buyu_record.set_room_name(gameName);
	buyu_record.set_remark(remark);
	if (recordId != NULL) buyu_record.set_record_id(recordId);
	
	for (unsigned int idx = 0; idx < items.size(); ++idx)
	{
		com_protocol::ItemRecordStorage* itemRecord = buyu_record.add_items();
		itemRecord->set_item_type(items[idx].type);
		itemRecord->set_charge_count(items[idx].count);
	}

	const char* recordData = serializeMsgToBuffer(buyu_record, "write game record");
	if (recordData != NULL)
	{
		pkg.set_game_type(NProject::GameRecordType::BuyuExt);
	    pkg.set_game_record_bin(recordData, buyu_record.ByteSize());
	}
}

// 保存用户逻辑数据
void CSrvMsgHandler::saveUserLogicData(XQUserData* ud)
{
	const char* msgData = serializeMsgToBuffer(*ud->srvData, "pack user logic data");
	if (msgData != NULL)
	{
		com_protocol::SetGameLogicDataReq req;
		req.set_primary_key(FFLogicLogicDataKey, FFLogicLogicDataKeyLen);
		req.set_second_key(ud->username, ud->usernameLen);
		req.set_data(msgData, ud->srvData->ByteSize());
		
		msgData = serializeMsgToBuffer(req, "save user logic data");
		if (msgData != NULL) sendMessageToService(DBProxyCommonSrv, msgData, req.ByteSize(), CommonDBSrvProtocol::BUSINESS_SET_SERVICE_LOGIC_DATA_REQ, ud->username, ud->usernameLen);
	}
}

int CSrvMsgHandler::sendGoodsChangeNotify(const google::protobuf::RepeatedPtrField<com_protocol::PropItem>& items, ConnectProxy* conn, const char* userName)
{
	if (items.size() < 1) return InvalidParam;

	com_protocol::ClientPropItemChangeNotify clientGoodshangeNotify;
	for (int idx = 0; idx < items.size(); ++idx)
	{
		const com_protocol::PropItem& iInfo = items.Get(idx);
		switch (iInfo.type())
		{
			case EPropType::PropGold:
			{
				com_protocol::PropItem* notifyItem = clientGoodshangeNotify.add_prop_items();
			    *notifyItem = iInfo;
				break;
			}
			
			default:
			{
				break;
			}
		}
	}
	
	int rc = Success;
	if (clientGoodshangeNotify.prop_items_size() > 0)
	{
		const char* msgData = serializeMsgToBuffer(clientGoodshangeNotify, "goods change notify");
		if (msgData != NULL)
		{
			if (conn != NULL) rc = sendMsgToProxy(msgData, clientGoodshangeNotify.ByteSize(), XIANGQISRV_GOODS_CHANGE_NOTIFY, conn);
			else if (userName != NULL) rc = sendMsgToProxyEx(msgData, clientGoodshangeNotify.ByteSize(), XIANGQISRV_GOODS_CHANGE_NOTIFY, userName);
		}
	}
	
	return rc;
}

// 更新服务信息
void CSrvMsgHandler::updateRoomInfo(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	const XiangQiConfig::CommConfig& common_cfg = XiangQiConfig::config::getConfigValue().common_cfg;
	com_protocol::RoomData roomData;
	const unsigned int srvId = getSrvId();
	roomData.set_ip(CSocket::toIPInt(CCfg::getValue("NetConnect", "IP")));
	roomData.set_port(atoi(CCfg::getValue("NetConnect", "Port")));
	roomData.set_type(com_protocol::EClientGameType::EFFChessGame);
	roomData.set_rule(0);
	roomData.set_rate(10);
	roomData.set_current_persons(m_onPlayGameUsers.size());
	roomData.set_max_persons(common_cfg.max_online_persons);
	roomData.set_id(srvId);
	roomData.set_name(common_cfg.name);
	roomData.set_limit_game_gold(1000);
	roomData.set_limit_up_game_gold(1000000);
	roomData.set_update_timestamp(time(NULL));
	roomData.set_flag(common_cfg.service_flag);

    const char* msgData = serializeMsgToBuffer(roomData, "update service info");
	if (msgData != NULL)
	{
		int rc = m_redisDbOpt.setHField(NProject::HallKey, NProject::HallKeyLen, (const char*)&srvId, sizeof(srvId), msgData, roomData.ByteSize());
		if (rc != 0)
		{
			OptErrorLog("set xiangqi fan fan game service data to redis center service failed, rc = %d", rc);
			return;
		}
	}
}




static CSrvMsgHandler s_msgHandler;  // 消息处理模块实例

int CXiangQiSrv::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("run xiang qi fan fan service name = %s, id = %d", name, id);
	return 0;
}

void CXiangQiSrv::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("stop xiang qi fan fan service name = %s, id = %d", name, id);
}

void CXiangQiSrv::onRegister(const char* name, const unsigned int id)
{
	ReleaseInfoLog("register xiang qi fan fan module, service name = %s, id = %d", name, id);
	
	// 注册模块实例
	const unsigned short HandlerMessageModule = 0;
	m_connectNotifyToHandler = &s_msgHandler;
	registerModule(HandlerMessageModule, &s_msgHandler);
}

void CXiangQiSrv::onUpdateConfig(const char* name, const unsigned int id)
{
	const XiangQiConfig::config& cfgValue = XiangQiConfig::config::getConfigValue(CCfg::getValue("Config", "BusinessXmlConfigFile"), true);
	ReleaseInfoLog("update config value, service name = %s, id = %d, result = %d", name, id, cfgValue.isSetConfigValueSuccess());
	cfgValue.output();
	
	s_msgHandler.updateConfig();
}

// 通知逻辑层对应的逻辑连接已被关闭
void CXiangQiSrv::onClosedConnect(void* userData)
{
	m_connectNotifyToHandler->onClosedConnect(userData);
}


CXiangQiSrv::CXiangQiSrv() : IService(XiangQiSrv)
{
	m_connectNotifyToHandler = NULL;
}

CXiangQiSrv::~CXiangQiSrv()
{
	m_connectNotifyToHandler = NULL;
}

REGISTER_SERVICE(CXiangQiSrv);

}

