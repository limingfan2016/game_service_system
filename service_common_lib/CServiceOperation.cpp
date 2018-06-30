
/* author : limingfan
 * date : 2017.09.11
 * description : 各服务公共接口操作API
 */

#include "connect/CSocket.h"
#include "SrvFrame/UserType.h"
#include "CServiceOperation.h"
#include "CommonType.h"
#include "MessageCenterProtocolId.h"
#include "DBProxyProtocolId.h"


using namespace NCommon;
using namespace NFrame;
using namespace NConnect;


namespace NProject
{

static const unsigned int UserDataCount = 256;  // 用户数据块增长步长数量


// 连接代理状态
enum EConnectProxyStatus
{
	EBuildSuccess = 1,                          // 连接建立
	ECheckSuccess = 2,                          // 连接验证成功
	EConnectClosed = 3,                         // 连接关闭
};


CServiceOperation::CServiceOperation()
{
	m_msgHandler = NULL;
	m_dataLogger = NULL;
}

CServiceOperation::~CServiceOperation()
{
	m_msgHandler = NULL;
	
	destroyDataLogger();
	m_dataLogger = NULL;
}

bool CServiceOperation::init(NFrame::CModule* msgHander, const char* xmlCommonCfgFile)
{
	m_msgHandler = msgHander;

	if (xmlCommonCfgFile != NULL)
	{
		const NCommonConfig::CommonCfg& cfg = NCommonConfig::CommonCfg::getConfigValue(xmlCommonCfgFile);
		if (!cfg.isSetConfigValueSuccess())
		{
			ReleaseErrorLog("set common xml config value error, file = %s", xmlCommonCfgFile);
			return false;
		}
		
		cfg.output();
	}
	
	return true;
}

// 刷新配置文件
bool CServiceOperation::updateCommonConfig(const char* xmlCommonCfgFile)
{
	bool updateResult = true;
	if (xmlCommonCfgFile != NULL)
	{
		const NCommonConfig::CommonCfg& cfg = NCommonConfig::CommonCfg::getConfigValue(xmlCommonCfgFile, true);
		if (!cfg.isSetConfigValueSuccess())
		{
			updateResult = false;
			ReleaseErrorLog("update common xml config value error, file = %s", xmlCommonCfgFile);
		}
		
		ReleaseInfoLog("update common config result = %d", cfg.isSetConfigValueSuccess());
		
		cfg.output();
	}
	
	return updateResult;
}

const NCommonConfig::CommonCfg& CServiceOperation::getCommonCfg()
{
	return NCommonConfig::CommonCfg::getConfigValue();
}

const DBConfig::config& CServiceOperation::getDBCfg(bool isReset)
{
	return DBConfig::config::getConfigValue(NCommonConfig::CommonCfg::getConfigValue().config_file.db_xml_cfg.c_str(), isReset);
}

const ServiceCommonConfig::SrvCommonCfg& CServiceOperation::getServiceCommonCfg(bool isReset)
{
	return ServiceCommonConfig::SrvCommonCfg::getConfigValue(NCommonConfig::CommonCfg::getConfigValue().config_file.service_xml_cfg.c_str(), isReset);
}

const MallConfigData::MallData& CServiceOperation::getMallCfg(bool isReset)
{
	return MallConfigData::MallData::getConfigValue(NCommonConfig::CommonCfg::getConfigValue().config_file.mall_xml_cfg.c_str(), isReset);
}

const NCattlesBaseConfig::CattlesBaseConfig& CServiceOperation::getCattlesBaseCfg(bool isReset)
{
	return NCattlesBaseConfig::CattlesBaseConfig::getConfigValue(NCommonConfig::CommonCfg::getConfigValue().config_file.cattles_base_cfg.c_str(), isReset);
}

const NGoldenFraudBaseConfig::GoldenFraudBaseConfig& CServiceOperation::getGoldenFraudBaseCfg(bool isReset)
{
	return NGoldenFraudBaseConfig::GoldenFraudBaseConfig::getConfigValue(NCommonConfig::CommonCfg::getConfigValue().config_file.golden_fraud_base_cfg.c_str(), isReset);
}

const NFightLandlordBaseConfig::FightLandlordBaseConfig& CServiceOperation::getFightLandlordBaseCfg(bool isReset)
{
	return NFightLandlordBaseConfig::FightLandlordBaseConfig::getConfigValue(NCommonConfig::CommonCfg::getConfigValue().config_file.fight_landlord_base_cfg.c_str(), isReset);
}
	
bool CServiceOperation::createDataLogger(const char* cfgName, const char* fileItem, const char* sizeItem, const char* backupCountItem)
{
	if (m_dataLogger == NULL)
	{
		// 数据日志配置检查
		const unsigned int MinDataLogFileSize = 1024 * 1024 * 2;
		const unsigned int MinDataLogFileCount = 10;
		const char* dataLoggerCfg[] = {fileItem, sizeItem, backupCountItem,};
		for (unsigned int i = 0; i < (sizeof(dataLoggerCfg) / sizeof(dataLoggerCfg[0])); ++i)
		{
			const char* value = CCfg::getValue(cfgName, dataLoggerCfg[i]);
			if (value == NULL)
			{
				OptErrorLog("data log config item error, can not find item = %s", dataLoggerCfg[i]);
				return false;
			}
			
			if ((i == 1 && (unsigned int)atoi(value) < MinDataLogFileSize)
				|| (i == 2 && (unsigned int)atoi(value) < MinDataLogFileCount))
			{
				OptErrorLog("data log config item error, file min size = %d, count = %d", MinDataLogFileSize, MinDataLogFileCount);
				return false;
			}
		}

        // 创建日志对象
		NEW(m_dataLogger, CLogger(CCfg::getValue(cfgName, fileItem), atoi(CCfg::getValue(cfgName, sizeItem)), atoi(CCfg::getValue(cfgName, backupCountItem))));
	}

	return true;
}

void CServiceOperation::destroyDataLogger()
{
	DELETE(m_dataLogger);
}

// 获取数据日志对象
NCommon::CLogger* CServiceOperation::getDataLogger()
{
	return m_dataLogger;
}

void CServiceOperation::stopTimer(unsigned int& timerId)
{
	if (timerId != 0)
	{
		m_msgHandler->killTimer(timerId);
		timerId = 0;
	}
}

// 获取唯一记录ID
unsigned int CServiceOperation::getRecordId(RecordIDType recordId)
{
    static unsigned int SRecordIndex = 0;
	const unsigned int MaxRecordIndex = 1000000;

	return snprintf(recordId, sizeof(RecordIDType) - 1, "%uS%uI%u", (unsigned int)time(NULL), m_msgHandler->getSrvId(), ++SRecordIndex % MaxRecordIndex);
}

bool CServiceOperation::parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info)
{
	if (msg.ParseFromArray(buffer, len)) return true;

	OptErrorLog("ParseFromArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), len, info);
	return false;
}

const char* CServiceOperation::serializeMsgToBuffer(const ::google::protobuf::Message& msg, const char* info)
{
	static char msgBuffer[MaxMsgLen] = {0};
	if (msg.SerializeToArray(msgBuffer, MaxMsgLen)) return msgBuffer;
	
	OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), MaxMsgLen, info);
	return NULL;
}

const char* CServiceOperation::serializeMsgToBuffer(const ::google::protobuf::Message& msg, char* msgBuffer, unsigned int& msgBufferLen, const char* info)
{
	if (msgBuffer == NULL || msgBufferLen < 1) return NULL;

	if (msg.SerializeToArray(msgBuffer, msgBufferLen))
	{
		msgBufferLen = msg.ByteSize();
		return msgBuffer;
	}
	
	OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), msgBufferLen, info);
	return NULL;
}

// 向服务发送消息包
int CServiceOperation::sendPkgMsgToService(const char* username, unsigned int usernameLen, ServiceType srvType, const ::google::protobuf::Message& msg,
						                   const unsigned short protocolId, const char* info, int userFlag, unsigned short handleProtocolId)
{
	int rc = SrvOptFailed;
	const char* msgData = serializeMsgToBuffer(msg, info);
	if (msgData != NULL)
	{
		unsigned int srvId = 0;
		NProject::getDestServiceID(srvType, username, usernameLen, srvId);
		rc = m_msgHandler->sendMessage(msgData, msg.ByteSize(), userFlag, username, usernameLen, srvId, protocolId, 0, handleProtocolId);
	}
	
	return rc;
}
						
int CServiceOperation::sendPkgMsgToService(const ::google::protobuf::Message& msg, const unsigned int srvId, const unsigned short protocolId,
						                   const char* info, unsigned short handleProtocolId)
{
	int rc = SrvOptFailed;
	const char* msgData = serializeMsgToBuffer(msg, info);
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, msg.ByteSize(), srvId, protocolId, 0, handleProtocolId);
	
	return rc;
}

int CServiceOperation::sendPkgMsgToService(const char* userData, unsigned int userDataLen, const ::google::protobuf::Message& msg, const unsigned int srvId,
										   const unsigned short protocolId, const char* info, int userFlag, unsigned short handleProtocolId)
{
	int rc = SrvOptFailed;
	const char* msgData = serializeMsgToBuffer(msg, info);
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, msg.ByteSize(), userFlag, userData, userDataLen, srvId, protocolId, 0, handleProtocolId);

	return rc;
}

int CServiceOperation::sendPkgMsgToDbProxy(const ::google::protobuf::Message& msg, unsigned short protocolId, const char* username, const unsigned int usernameLen,
						                   const char* info, int userFlag, unsigned short handleProtocolId)
{
	return sendPkgMsgToService(username, usernameLen, ServiceType::DBProxySrv, msg, protocolId, info, userFlag, handleProtocolId);
}


int CServiceOperation::sendMsgToService(const char* username, unsigned int usernameLen, ServiceType srvType,
                                        const char* data, const unsigned int len,
										const unsigned short protocolId, int userFlag, unsigned short handleProtocolId)
{
	unsigned int srvId = 0;
	NProject::getDestServiceID(srvType, username, usernameLen, srvId);
	return m_msgHandler->sendMessage(data, len, userFlag, username, usernameLen, srvId, protocolId, 0, handleProtocolId);
}

int CServiceOperation::sendMsgToService(const char* data, const unsigned int len, const unsigned int srvId, const unsigned short protocolId,
										unsigned short handleProtocolId)
{
	return m_msgHandler->sendMessage(data, len, srvId, protocolId, 0, handleProtocolId);
}

int CServiceOperation::sendMsgToService(const char* userData, unsigned int userDataLen, const char* data, const unsigned int len, const unsigned int srvId,
										const unsigned short protocolId, int userFlag, unsigned short handleProtocolId)
{
	return m_msgHandler->sendMessage(data, len, userFlag, userData, userDataLen, srvId, protocolId, 0, handleProtocolId);
}


int CServiceOperation::sendServiceStatusNotify(unsigned int serviceId, com_protocol::EServiceStatus serviceStatus,
	                                           unsigned int serviceType, const char* serviceName)
{
	com_protocol::ServiceStatusNotify nf;
	nf.set_service_id(serviceId);
	nf.set_service_status(serviceStatus);
	nf.set_service_type(serviceType);
	nf.set_service_name(serviceName);
	
	return sendPkgMsgToService("", 0, ServiceType::MessageCenterSrv, nf, MSGCENTER_SERVICE_STATUS_NOTIFY, "service status notify");
}

int CServiceOperation::sendUserOnlineNotify(const char* username, unsigned int serviceId, const char* onlineIP,
                                            const char* hallId, const char* roomId, unsigned int roomRate, int seatId)
{
	com_protocol::UserEnterGameNotify nf;
	nf.set_username(username);
	nf.set_service_id(serviceId);
	
	if (*onlineIP != '\0') nf.set_online_ip(onlineIP);
	if (*hallId != '\0') nf.set_hall_id(hallId);
	if (*roomId != '\0') nf.set_room_id(roomId);
	
	nf.set_room_rate(roomRate);
	nf.set_seat_id(seatId);
	
	return sendPkgMsgToService("", 0, ServiceType::MessageCenterSrv, nf, MSGCENTER_USER_ONLINE_NOTIFY, "user online notify");
}

int CServiceOperation::sendUserOfflineNotify(const char* username, unsigned int serviceId, const char* hallId, const char* roomId,
                                             int code, const char* info, unsigned int onlineTime, int seatId)
{
    com_protocol::UserLeaveGameNotify nf;
	nf.set_username(username);
	nf.set_service_id(serviceId);
	
	if (*hallId != '\0') nf.set_hall_id(hallId);
	if (*roomId != '\0') nf.set_room_id(roomId);
	if (*info != '\0') nf.set_info(info);
	
	nf.set_code(code);
	nf.set_online_time(onlineTime);
	nf.set_seat_id(seatId);
	
	return sendPkgMsgToService("", 0, ServiceType::MessageCenterSrv, nf, MSGCENTER_USER_OFFLINE_NOTIFY, "user offline notify");
}

// 转发消息到消息中心
int CServiceOperation::forwardMessage(const char* username, const char* data, const unsigned int len, unsigned int dstSrvType,
									  unsigned short dstProtocolId, unsigned int srcSrvId, unsigned short srcProtocol, const char* info)
{
	com_protocol::ForwardMessageToServiceReq forwardMsgReq;
	forwardMsgReq.set_username(username);
	forwardMsgReq.set_data(data, len);
	forwardMsgReq.set_dst_service_type(dstSrvType);
	forwardMsgReq.set_dst_protocol(dstProtocolId);
	forwardMsgReq.set_src_service_id(srcSrvId);
	forwardMsgReq.set_src_protocol(srcProtocol);
	
	return sendPkgMsgToService(forwardMsgReq.username().c_str(), forwardMsgReq.username().length(),
							   ServiceType::MessageCenterSrv, forwardMsgReq, MSGCENTER_FORWARD_MESSAGE_REQ, info);
}

// 修改房间数据同步通知
int CServiceOperation::changeRoomDataNotify(const char* hallId, const char* roomId, int gameType, com_protocol::EHallRoomStatus status,
						                    const com_protocol::ChessHallRoomBaseInfo* addRoomInfo,
                                            const com_protocol::ClientRoomPlayerInfo* addPlayerInfo,
						                    const char* delUsername, const char* info)
{
    // 先修改房间的使用状态
    int rc = changeHallRoomStatus(roomId, status, info);
	if (rc != SrvOptSuccess) return rc;
	
	// 成功则同步消息到大厅
	return updateRoomDataNotifyHall(hallId, roomId, gameType, status, addRoomInfo, addPlayerInfo, delUsername, info);
}

// 修改房间的使用状态
int CServiceOperation::changeHallRoomStatus(const char* roomId, com_protocol::EHallRoomStatus status, const char* info)
{
	com_protocol::ChangeHallGameRoomStatusReq req;
	req.set_room_id(roomId);
	req.set_new_status(status);
	return sendPkgMsgToService("", 0, ServiceType::DBProxySrv, req, DBPROXY_CHANGE_GAME_ROOM_STATUS_REQ, info);
}

// 房间数据变更同步通知到大厅
int CServiceOperation::updateRoomDataNotifyHall(const char* hallId, const char* roomId, int gameType, com_protocol::EHallRoomStatus status,
												const com_protocol::ChessHallRoomBaseInfo* addRoomInfo,
												const com_protocol::ClientRoomPlayerInfo* addPlayerInfo,
										     	const char* delUsername, const char* info)
{
	// 房间数据变更，通知到棋牌室大厅
	com_protocol::ClientUpdateHallRoomNotify updateHallRoomNtf;
	updateHallRoomNtf.set_game_type(gameType);
	if (delUsername != NULL) updateHallRoomNtf.add_del_usernames(delUsername);

	com_protocol::ClientHallRoomInfo* hallRoomInfo = updateHallRoomNtf.mutable_room_info();
	hallRoomInfo->set_room_id(roomId);
	hallRoomInfo->set_status(status);

	if (addRoomInfo != NULL)
	{
		// 新增房间
		hallRoomInfo->set_type(addRoomInfo->room_type());
		hallRoomInfo->set_players(addRoomInfo->player_count());
		hallRoomInfo->set_base_rate(addRoomInfo->base_rate());
		hallRoomInfo->set_is_can_view(addRoomInfo->is_can_view());
		
		if (addRoomInfo->has_portrait_id()) hallRoomInfo->set_portrait_id(addRoomInfo->portrait_id());
	}

    const char* srcUsername = delUsername;
	if (addPlayerInfo != NULL)
	{
		// 新增玩家
		const com_protocol::StaticInfo& userStaticInfo = addPlayerInfo->detail_info().static_info();
		com_protocol::ClientRoomUserInfo* roomUserInfo = hallRoomInfo->add_user_info();
		roomUserInfo->set_username(userStaticInfo.username());
		roomUserInfo->set_nickname(userStaticInfo.nickname());
		roomUserInfo->set_portrait_id(userStaticInfo.portrait_id());
		roomUserInfo->set_gender(userStaticInfo.gender());
		roomUserInfo->set_game_gold(addPlayerInfo->detail_info().dynamic_info().game_gold());
		roomUserInfo->set_seat_id(addPlayerInfo->seat_id());
		
		srcUsername = roomUserInfo->username().c_str();
	}
	
	const char* ntfMsgData = serializeMsgToBuffer(updateHallRoomNtf, info);
	if (ntfMsgData != NULL)
	{
		// 刷新数据通知
		com_protocol::ServiceUpdateDataNotify srvUpdateDataNtf;
		if (hallId != NULL) srvUpdateDataNtf.set_hall_id(hallId);
		if (srcUsername != NULL) srvUpdateDataNtf.set_src_username(srcUsername);
		srvUpdateDataNtf.set_type(com_protocol::EUpdateDataType::EUpdateHallRoom);
		srvUpdateDataNtf.set_data(ntfMsgData, updateHallRoomNtf.ByteSize());

		return sendPkgMsgToService("", 0, ServiceType::MessageCenterSrv, srvUpdateDataNtf, MSGCENTER_UPDATE_SERVICE_DATA_NOTIFY, info);
	}
	
	return SrvOptFailed;
}

// 变更游戏房间里玩家的座位号
int CServiceOperation::changeRoomPlayerSeat(const char* roomId, const char* username, int seatId, const char* info)
{
	com_protocol::ChangeGameRoomPlayerSeatReq changeSeatReq;
	changeSeatReq.set_room_id(roomId);
	changeSeatReq.set_username(username);
	changeSeatReq.set_seat_id(seatId);
	return sendPkgMsgToService("", 0, ServiceType::DBProxySrv, changeSeatReq, DBPROXY_CHANGE_GAME_ROOM_PLAYER_SEAT_REQ, info);
}

// 设置最近一起游戏的玩家信息
int CServiceOperation::setLastPlayersNotify(const char* hallId, const ConstCharPointerVector& usernames, const char* info)
{
	if (usernames.size() <= 1) return InvalidParameter;
	
	com_protocol::SetLastPlayersNotify ntf;
	ntf.set_hall_id(hallId);
	for (ConstCharPointerVector::const_iterator it = usernames.begin(); it != usernames.end(); ++it) ntf.add_username(*it);

	return sendPkgMsgToService(usernames[0], strlen(usernames[0]), ServiceType::DBProxySrv, ntf, DBPROXY_ADD_LAST_PLAYERS_REQ, info);
}


CServiceOperationEx::CServiceOperationEx(unsigned int userDataSize) : m_memForUserData(UserDataCount, UserDataCount, userDataSize)
{
	m_msgHandler = NULL;
	m_closeRepeateNotifyProtocolId = 0;
}

CServiceOperationEx::~CServiceOperationEx()
{
	m_memForUserData.clear();

	m_msgHandler = NULL;
	m_closeRepeateNotifyProtocolId = 0;
}

bool CServiceOperationEx::initEx(NFrame::CLogicHandler* msgHander, const char* xmlCommonCfgFile)
{
	m_msgHandler = msgHander;

	return init(msgHander, xmlCommonCfgFile);
}

void CServiceOperationEx::setCloseRepeateNotifyProtocol(int listenProtocolId, unsigned int clientNotifyProtocolId)
{
	if (listenProtocolId > 0)
	{
		m_msgHandler->registerProtocol(CommonSrv, listenProtocolId, (ProtocolHandler)&CServiceOperationEx::closeRepeatConnectProxy, this);
	}
	
	m_closeRepeateNotifyProtocolId = clientNotifyProtocolId;
}

// 创建初始化连接代理用户数据
ConnectProxyUserData* CServiceOperationEx::createConnectProxyUserData(ConnectProxy* conn)
{
	const unsigned int userDataSize = m_memForUserData.getBuffSize();
	if (userDataSize < sizeof(ConnectProxyUserData)) return NULL;

	// 挂接用户数据
	ConnectProxyUserData* cud = (ConnectProxyUserData*)m_memForUserData.get();
	memset(cud, 0, userDataSize);
	cud->timeSecs = time(NULL);
	cud->status = EBuildSuccess;
	cud->repeatTimes = 0;
	
	// 初始化连接ID
	cud->userNameLen = snprintf(cud->userName, IdMaxLen - 1, "initID:%s:%d", CSocket::toIPStr(conn->peerIp), conn->peerPort);
	
	m_msgHandler->addConnectProxy(cud->userName, conn, cud);
	
	return cud;
}

// 销毁连接代理用户数据
void CServiceOperationEx::destroyConnectProxyUserData(ConnectProxyUserData* cud)
{
	cud->status = EConnectClosed;  // 标识一下，防止上层使用错误时可查看数据信息
	m_memForUserData.put((char*)cud);
}

// 连接代理验证成功
ConnectProxyUserData* CServiceOperationEx::checkConnectProxySuccess(const string& usreName, ConnectProxy* conn)
{
	ConnectProxyUserData* cud = (ConnectProxyUserData*)m_msgHandler->getProxyUserData(conn);
	if (cud != NULL)
	{
		// 先删除初始化连接ID
		m_msgHandler->removeConnectProxy(cud->userName, 0, false);

		cud->status = ECheckSuccess;
		strncpy(cud->userName, usreName.c_str(), IdMaxLen - 1);
		cud->userNameLen = usreName.length();

		// 用户ID关联连接代理
		m_msgHandler->addConnectProxy(cud->userName, conn, cud);
	}
	
	return cud;
}

// 是否是验证成功的连接代理
bool CServiceOperationEx::isCheckSuccessConnectProxy(const ConnectProxyUserData* cud)
{
	return (cud != NULL && cud->status == ECheckSuccess);
}

// 检查同一条连接代理是否重复验证
bool CServiceOperationEx::isRepeateCheckConnectProxy(unsigned int forceQuitProtocol)
{
	ConnectProxy* conn = m_msgHandler->getConnectProxyContext().conn;
	if (conn == NULL) return false;
	
	const unsigned short cMaxRepeatTimes = 2;
	ConnectProxyUserData* cud = (ConnectProxyUserData*)m_msgHandler->getProxyUserData(conn);
	if (cud != NULL && (cud->status == ECheckSuccess || ++cud->repeatTimes > cMaxRepeatTimes))
	{
		// 同一条连接，重复登陆的一律关闭连接
		OptErrorLog("close repeate check connect proxy, userName = %s, id = %u, ip = %s, port = %d, status = %u, times = %d, interval = %u",
		cud->userName, conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort,
		cud->status, cud->repeatTimes, time(NULL) - cud->timeSecs);
		
		// 强制玩家退出服务
		com_protocol::ClientLogoutNotify forceQuitNtf;
		forceQuitNtf.set_info("repeate check operation");
		sendMsgPkgToProxy(forceQuitNtf, forceQuitProtocol, conn, "repeate check operation force quit notify");
		
		if (!m_msgHandler->removeConnectProxy(cud->userName, com_protocol::EUserQuitStatus::EUserRepeateLogin))
        {
            m_msgHandler->closeConnectProxy(conn, com_protocol::EUserQuitStatus::EUserRepeateLogin);  // 作弊行为则强制关闭
        }
		
		return true;
	}
	
	return false;
}

// 检查发送消息的连接是否已成功通过验证，防止玩家验证成功之前胡乱发消息
bool CServiceOperationEx::checkConnectProxyIsSuccess(unsigned int forceQuitProtocol)
{
	ConnectProxy* conn = m_msgHandler->getConnectProxyContext().conn;
	if (conn == NULL) return false;

	ConnectProxyUserData* cud = (ConnectProxyUserData*)m_msgHandler->getProxyUserData(conn);
	if (cud != NULL && cud->status == ECheckSuccess) return true;
	
	// 未经过登录成功后的非法操作消息则直接关闭连接
	if (cud == NULL)
	{
		OptErrorLog("close connect, id = %d, ip = %s, port = %d",
		conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort);
	}
	else if (cud->status != ECheckSuccess)
	{
		OptErrorLog("close connect, time secs = %u, status = %u, userName = %s, id = %d, ip = %s, port = %d,",
		cud->timeSecs, cud->status, cud->userName, conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort);
	}
	
	// 强制玩家退出服务
	com_protocol::ClientLogoutNotify forceQuitNtf;
	forceQuitNtf.set_info("invalid operation");
	sendMsgPkgToProxy(forceQuitNtf, forceQuitProtocol, conn, "invalid operation force quit notify");

	const bool isClosed = (cud != NULL) ? m_msgHandler->removeConnectProxy(cud->userName, com_protocol::EUserQuitStatus::EUserInvalidMessage) : false;
    if (!isClosed) m_msgHandler->closeConnectProxy(conn, com_protocol::EUserQuitStatus::EUserInvalidMessage);
	
	return false;
}

// 获取连接代理上的用户数据，userName为NULL则默认获取当前上下文连接代理，否则获取userName对应的连接代理
ConnectProxyUserData* CServiceOperationEx::getConnectProxyUserData(const char* userName)
{
	ConnectProxyUserData* cud = NULL;
	if (userName == NULL) cud = (ConnectProxyUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	else cud = (ConnectProxyUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxy(userName));
	
	return cud;
}

// 用户是否在线，获取当前上下文中的userName，根据userName获取连接代理
ConnectProxy* CServiceOperationEx::getConnectProxyInfo(const char*& userName, const char* info)
{
	userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL) OptErrorLog("%s, get connect proxy error, user data = %s", info, userName);
	
	return conn;
}

// 关闭相同用户ID对应的重复连接代理
void CServiceOperationEx::closeRepeatConnectProxy(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const string userName(data, len);
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn != NULL)
	{
		com_protocol::ClientCloseRepeatLoginNotify closeRepeateNotify;
		closeRepeateNotify.set_username(userName);
		sendMsgPkgToProxy(closeRepeateNotify, m_closeRepeateNotifyProtocolId, conn, "close repeate connect proxy notify");
		
		OptWarnLog("close repeate connect proxy notify, userName = %s, id = %u, ip = %s, port = %d",
		userName.c_str(), conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort);
		
		m_msgHandler->removeConnectProxy(userName, com_protocol::EUserQuitStatus::EUserRepeateConnect);
	}
}

// 服务启动时清理连接代理
void CServiceOperationEx::cleanUpConnectProxy(CCfg* srvMsgCommCfg)
{
	if (srvMsgCommCfg == NULL) return;

	// 清理连接代理，如果服务异常退出，则启动时需要先清理连接代理
	unsigned int proxyId[1024] = {0};
	unsigned int proxyIdLen = 0;
	const NCommon::Key2Value* pServiceIds = srvMsgCommCfg->getList("ServiceID");
	while (pServiceIds)
	{
		if (0 != strcmp("MaxServiceId", pServiceIds->key))
		{
			unsigned int service_id = atoi(pServiceIds->value);
			unsigned int service_type = service_id / ServiceIdBaseValue;
			if (service_type == ServiceType::GatewaySrv) proxyId[proxyIdLen++] = service_id;
		}
		
		pServiceIds = pServiceIds->pNext;
	}
	
	if (proxyIdLen > 0) m_msgHandler->cleanUpConnectProxy(proxyId, proxyIdLen);  // 清理连接代理
}

int CServiceOperationEx::sendMsgToProxyEx(const char* data, const unsigned int len, unsigned short protocolId, const char* userName)
{
	ConnectProxy* connProxy = (userName != NULL) ? m_msgHandler->getConnectProxy(userName) : NULL;
	if (connProxy == NULL)
	{
		OptWarnLog("send messge to connect proxy error, not find the proxy, userName = %s, protocolId = %d",
		(userName != NULL) ? userName : "", protocolId);
		
		return ServiceNoFoundConnectProxy;
	}
	
	return m_msgHandler->sendMsgToProxy(data, len, protocolId, connProxy);
}

int CServiceOperationEx::sendMsgPkgToProxy(const ::google::protobuf::Message& pkg, unsigned short protocolId, const char* info, const char* username)
{
	const char* msgData = serializeMsgToBuffer(pkg, info);
	if (msgData == NULL) return SerializeMessageToArrayError;
	
	if (NULL == username)
	{
		return m_msgHandler->sendMsgToProxy(msgData, pkg.ByteSize(), protocolId);
	}
	else
	{
		return sendMsgToProxyEx(msgData, pkg.ByteSize(), protocolId, username);
	}
}

int CServiceOperationEx::sendMsgPkgToProxy(const ::google::protobuf::Message& pkg, unsigned short protocolId, ConnectProxy* conn, const char* info)
{
	const char* msgData = serializeMsgToBuffer(pkg, info);
	if (msgData == NULL) return SerializeMessageToArrayError;
	
	return m_msgHandler->sendMsgToProxy(msgData, pkg.ByteSize(), protocolId, conn);
}


}

