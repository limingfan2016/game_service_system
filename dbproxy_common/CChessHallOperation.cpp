
/* author : limingfan
 * date : 2017.09.15
 * description : 棋牌室操作实现
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>

#include "CMsgHandler.h"
#include "CChessHallOperation.h"


using namespace NProject;

// 数据日志文件
#define DBDataRecordLog(format, args...)     m_msgHandler->getSrvOpt().getDataLogger()->writeFile(NULL, 0, LogLevel::Info, format, ##args)


CChessHallOperation::CChessHallOperation()
{
	m_msgHandler = NULL;
}

CChessHallOperation::~CChessHallOperation()
{
	m_msgHandler = NULL;
}

int CChessHallOperation::init(CMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

    // 棋牌室管理员操作
	m_msgHandler->registerProtocol(ServiceType::OperationManageSrv, DBPROXY_MANAGER_OPT_PLAYER_INFO_REQ, (ProtocolHandler)&CChessHallOperation::managerOptPlayer, this);
	m_msgHandler->registerProtocol(ServiceType::OperationManageSrv, DBPROXY_CONFIRM_MALL_BUY_INFO_REQ, (ProtocolHandler)&CChessHallOperation::confirmMallBuyInfo, this);
	m_msgHandler->registerProtocol(ServiceType::OperationManageSrv, DBPROXY_GIVE_MALL_GOODS_REQ, (ProtocolHandler)&CChessHallOperation::giveMallGoods, this);

    // 获取棋牌室信息	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_HALL_BASE_INFO_REQ, (ProtocolHandler)&CChessHallOperation::getHallBaseInfo, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_CHESS_HALL_INFO_REQ, (ProtocolHandler)&CChessHallOperation::getChessHallInfo, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_HALL_ROOM_LIST_REQ, (ProtocolHandler)&CChessHallOperation::getHallRoomList, this);
    m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_HALL_RANKING_REQ, (ProtocolHandler)&CChessHallOperation::getHallRanking, this);

	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_HALL_PLAYER_STATUS_REQ, (ProtocolHandler)&CChessHallOperation::getHallPlayerStatus, this);
    m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_GAME_ROOM_INFO_REQ, (ProtocolHandler)&CChessHallOperation::getHallGameRoomInfo, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_APPLY_JOIN_CHESS_HALL_REQ, (ProtocolHandler)&CChessHallOperation::applyIntoChessHall, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_CANCEL_APPLY_CHESS_HALL_REQ, (ProtocolHandler)&CChessHallOperation::cancelApplyChessHall, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_CREATE_HALL_GAME_ROOM_REQ, (ProtocolHandler)&CChessHallOperation::createHallGameRoom, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_CHANGE_GAME_ROOM_STATUS_REQ, (ProtocolHandler)&CChessHallOperation::changeHallGameRoomStatus, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_CHANGE_GAME_ROOM_PLAYER_SEAT_REQ, (ProtocolHandler)&CChessHallOperation::changeGameRoomPlayerSeat, this);

    m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_PLAYER_BUY_MALL_GOODS_REQ, (ProtocolHandler)&CChessHallOperation::buyMallGoods, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_PLAYER_GET_MALL_BUY_RECORD_REQ, (ProtocolHandler)&CChessHallOperation::getMallBuyRecord, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_PLAYER_CONFIRM_GIVE_GOODS_NOTIFY, (ProtocolHandler)&CChessHallOperation::confirmGiveGoods, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_CREATE_HALL_RECHARGE_INFO_REQ, (ProtocolHandler)&CChessHallOperation::createHallRechargeInfo, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_MANAGER_RECHARGE_REQ, (ProtocolHandler)&CChessHallOperation::chessHallManagerRecharge, this);

    return initHallGameRoomIdRandomNumber() ? SrvOptSuccess : SrvOptFailed;
}

void CChessHallOperation::unInit()
{
}

void CChessHallOperation::updateConfig()
{
}


int CChessHallOperation::getUserChessHallBaseInfo(const char* username, ::google::protobuf::RepeatedPtrField<com_protocol::ClientHallBaseInfo>& baseInfos)
{
	// 用户所在棋牌室信息
	char sqlBuffer[2048] = {0};
	const unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select user.user_status,hall.hall_id,hall.hall_name,hall.hall_logo,UNIX_TIMESTAMP(hall.create_time),hall.description,hall.level,hall.status from %s.tb_chess_hall_user_info as user "
	"inner join tb_chess_card_hall as hall on user.hall_id=hall.hall_id where user.username=\'%s\' and hall.status!=%u and (user.user_status=%u or user.user_status=%u) "
	"order by user.user_status asc,user.online_hall_secs desc;",
	m_msgHandler->getSrvOpt().getDBCfg().logic_db_cfg.dbname.c_str(), username, com_protocol::EChessHallStatus::EDisbandHall,
	com_protocol::EHallPlayerStatus::EWaitForCheck, com_protocol::EHallPlayerStatus::ECheckAgreedPlayer);
	
	// 查表找出用户所在的棋牌室信息
	CQueryResult* p_result = NULL;
	if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->queryTableAllResult(sqlBuffer, sqlLen, p_result))
	{
		OptErrorLog("query user hall base info error, sql = %s", sqlBuffer);
		return DBProxyQueryUserHallInfoError;
	}
	
    if (p_result != NULL && p_result->getRowCount() > 0)
	{
		RowDataPtr rowData = NULL;
		while ((rowData = p_result->getNextRow()) != NULL)
		{
			if (rowData[1] != NULL)
			{
				// 只查找待审核或者已经审核通过的棋牌室信息
				com_protocol::ClientHallBaseInfo* hallBaseInfo = baseInfos.Add();
				hallBaseInfo->set_id(rowData[1]);
				hallBaseInfo->set_name(rowData[2]);
				hallBaseInfo->set_logo(rowData[3]);
				hallBaseInfo->set_create_time(atoi(rowData[4]));
				hallBaseInfo->set_desc(rowData[5]);
				hallBaseInfo->set_level(atoi(rowData[6]));
				hallBaseInfo->set_hall_status((com_protocol::EChessHallStatus)atoi(rowData[7]));
				hallBaseInfo->set_player_count(getPlayerCount(rowData[1]));
				hallBaseInfo->set_player_status((com_protocol::EHallPlayerStatus)atoi(rowData[0]));
			}
		}
	}
	m_msgHandler->getOptDBOpt()->releaseQueryResult(p_result);
	
	return SrvOptSuccess;
}

int CChessHallOperation::getUserChessHallInfo(const char* username, const char* hallId, unsigned int max_online_rooms,
									          com_protocol::ClientHallInfo& hallInfo, ::google::protobuf::RepeatedPtrField<com_protocol::BSGiveGoodsInfo>* giveGoodsInfo, double gameGold)
{
	// 用户所在棋牌室信息
	char sqlBuffer[2048] = {0};
	unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select user.user_status,hall.hall_id,hall.hall_name,hall.hall_logo,UNIX_TIMESTAMP(hall.create_time),hall.description,hall.level,hall.status from %s.tb_chess_hall_user_info as user "
	"inner join tb_chess_card_hall as hall on user.hall_id=hall.hall_id where user.username=\'%s\' and user.hall_id=\'%s\' and "
	"hall.status=%u and (user.user_status=%u or user.user_status=%u);",
	m_msgHandler->getSrvOpt().getDBCfg().logic_db_cfg.dbname.c_str(), username, hallId, com_protocol::EChessHallStatus::EOpenHall,
	com_protocol::EHallPlayerStatus::EWaitForCheck, com_protocol::EHallPlayerStatus::ECheckAgreedPlayer);
	
	// 查表找出用户所在的棋牌室信息
	CQueryResult* p_result = NULL;
	if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->queryTableAllResult(sqlBuffer, sqlLen, p_result))
	{
		OptErrorLog("query user hall info error, sql = %s", sqlBuffer);
		return DBProxyQueryUserHallInfoError;
	}
	
	com_protocol::ClientHallBaseInfo* hallBaseInfo = NULL;
    if (p_result != NULL && p_result->getRowCount() == 1)
	{
		// 棋牌室信息
		RowDataPtr rowData = p_result->getNextRow();
		hallBaseInfo = hallInfo.mutable_base_info();
		hallBaseInfo->set_id(rowData[1]);
		hallBaseInfo->set_name(rowData[2]);
		hallBaseInfo->set_logo(rowData[3]);
		hallBaseInfo->set_create_time(atoi(rowData[4]));
		hallBaseInfo->set_desc(rowData[5]);
		hallBaseInfo->set_level(atoi(rowData[6]));
		hallBaseInfo->set_hall_status((com_protocol::EChessHallStatus)atoi(rowData[7]));
		hallBaseInfo->set_player_status((com_protocol::EHallPlayerStatus)atoi(rowData[0]));
	}
	m_msgHandler->getOptDBOpt()->releaseQueryResult(p_result);
	
	if (hallBaseInfo == NULL) return DBProxyGetUserHallInfoError;
	
    int rc = SrvOptSuccess;
	do
	{
		hallBaseInfo->set_player_count(getPlayerCount(hallId)); // 棋牌室玩家数量

		// 获取棋牌室游戏信息
		google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>& gameInfoList = *hallInfo.mutable_game_info();
		rc = getChessHallGameInfo(hallId, gameInfoList);
		if (SrvOptSuccess != rc) break;

		// 获取棋牌室当前在线房间信息
		StringVector roomIds;
		rc = getChessHallOnlineRoomInfo(hallId, gameInfoList, 0, max_online_rooms, roomIds);
		if (SrvOptSuccess != rc) break;
		
		// 获取在线游戏房间的用户信息
		rc = !roomIds.empty() ? getChessHallOnlineUserInfo(hallId, roomIds, gameInfoList) : SrvOptSuccess;
		if (rc == DBProxyNotFoundOnlinePlayerError)
		{
			// 处理错误的在线房间
			OptErrorLog("check user and get game room online player error, username = %s, hallId = %s, room count = %u",
			username, hallId, roomIds.size());
		}
		else if (SrvOptSuccess != rc)
		{
			break;
		}
		
		// 清理无在线玩家的房间
		
		// 获取棋牌室排行数据
		rc = getCurrentHallRanking(hallId, *hallInfo.mutable_ranking());
		if (rc != SrvOptSuccess) break;

		// 赠送金币玩家上线通知
		if (giveGoodsInfo != NULL) rc = getUserGiveGoodsInfo(username, hallId, *giveGoodsInfo, gameGold);

	} while (false);
	
	return rc;
}

int CChessHallOperation::getUserGiveGoodsInfo(const char* username, const char* hallId, ::google::protobuf::RepeatedPtrField<com_protocol::BSGiveGoodsInfo>& giveGoodsInfo, double gameGold)
{	
	// 赠送金币玩家上线通知
	// select record_id,pay_type,pay_count from tb_mall_record_info where hall_id=xx and username=yy and opt_type=zz and opt_status=123
	char sqlBuffer[512] = {0};
	const unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select record_id,pay_type,pay_count from tb_mall_record_info where hall_id=\'%s\' and username=\'%s\' and opt_type=%d and opt_status=%d;",
	hallId, username, com_protocol::EHallPlayerOpt::EOptGiveGift, com_protocol::EMallOptStatus::EMallOptUnconfirm);

	CQueryResult* p_result = NULL;
	if (SrvOptSuccess != m_msgHandler->getLogicDBOpt()->queryTableAllResult(sqlBuffer, sqlLen, p_result))
	{
		OptErrorLog("query mall give goods info error, sql = %s", sqlBuffer);

		return DBProxyQueryMallGiveGoodsInfoError;
	}
	
	// select record_id,pay_type,pay_count from tb_mall_record_info
	if (p_result != NULL && p_result->getRowCount() > 0)
	{
		RowDataPtr rowData = NULL;
		while ((rowData = p_result->getNextRow()) != NULL)
		{
			com_protocol::BSGiveGoodsInfo* giveGoods = giveGoodsInfo.Add();
			giveGoods->set_record_id(rowData[0]);

			com_protocol::ItemChange* changeGold = giveGoods->mutable_give_goods();
			changeGold->set_type(atoi(rowData[1]));
			changeGold->set_num(atof(rowData[2]));
			changeGold->set_amount(gameGold);
		}
	}
	
	m_msgHandler->getLogicDBOpt()->releaseQueryResult(p_result);

	return SrvOptSuccess;
}

int CChessHallOperation::applyJoinChessHall(const char* username, const string& hall_id, const string& explain_msg, unsigned int serviceId)
{
	if (hall_id.empty()) return DBProxyChessHallIdParamError;
	
	if (explain_msg.length() >= 32) return DBProxyApplyJoinMsgLargeError;
	
	// 查询该棋牌室ID是否存在
	// select 1 from tb_chess_card_hall where hall_id='xx' and status=yy;
	char tmpSql[512] = {0};
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"select 1 from tb_chess_card_hall where hall_id=\'%s\' and status=%u limit 1;",
	hall_id.c_str(), com_protocol::EChessHallStatus::EOpenHall);
	
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
	{
		OptErrorLog("query chess hall id error, user = %s, sql = %s", username, tmpSql);
		return DBProxyQueryChessHallIdError;
	}
	
	const bool hasChessHallId = (pResult != NULL) ? (pResult->getRowCount() > 0) : false;
	m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);

	if (!hasChessHallId)
	{
		OptErrorLog("apply join chess hall error, the hall id is not found, user = %s, hall id = %s", username, hall_id.c_str());
		return DBProxyChessHallIdParamError;
	}

	// select user_status from tb_chess_hall_user_info where hall_id='xx' and username='yy';
	tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"select user_status from tb_chess_hall_user_info where hall_id=\'%s\' and username=\'%s\';",
	hall_id.c_str(), username);

	pResult = NULL;
	if (SrvOptSuccess != m_msgHandler->getLogicDBOpt()->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
	{
		OptErrorLog("query player status error, sql = %s", tmpSql);
		return DBProxyGetHallPlayerStatusError;
	}
	
	com_protocol::EHallPlayerStatus playerStatus = com_protocol::EHallPlayerStatus::EOutHallPlayer;
	if (pResult != NULL && pResult->getRowCount() == 1)
	{
		RowDataPtr rowData = pResult->getNextRow();
		playerStatus = (com_protocol::EHallPlayerStatus)atoi(rowData[0]);
	}
	m_msgHandler->getLogicDBOpt()->releaseQueryResult(pResult);
		
	if (playerStatus == com_protocol::EHallPlayerStatus::EForbidPlayer) return DBProxyForbidPlayerError;
	if (playerStatus == com_protocol::EHallPlayerStatus::ECheckAgreedPlayer) return DBProxyPlayerAlreadyInHallError;
		
	// 设置玩家在棋牌室中的状态
	if (playerStatus == com_protocol::EHallPlayerStatus::EOutHallPlayer)
	{ 
		tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"insert into tb_chess_hall_user_info(hall_id,username,create_time,user_status,online_hall_service_id,online_hall_secs,explain_msg,game_gold,room_card) "
		"value(\'%s\',\'%s\',now(),%u,%u,%u,\'%s\',%.2f,%.2f);",
		hall_id.c_str(), username, com_protocol::EHallPlayerStatus::EWaitForCheck, serviceId, (unsigned int)time(NULL), explain_msg.c_str(),
		m_msgHandler->m_pCfg->register_init.game_gold, m_msgHandler->m_pCfg->register_init.room_card);
	}
	else
	{
		// update tb_chess_hall_user_info set user_status=x where hall_id='x' and username='y';
		tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"update tb_chess_hall_user_info set user_status=%u,explain_msg=\'%s\' where hall_id=\'%s\' and username=\'%s\';",
		com_protocol::EHallPlayerStatus::EWaitForCheck, explain_msg.c_str(), hall_id.c_str(), username);
	}

	if (SrvOptSuccess == m_msgHandler->getLogicDBOpt()->executeSQL(tmpSql, tmpSqlLen))
	{
		m_msgHandler->mysqlCommit();
	}
	else
	{
		OptErrorLog("set hall player status error, sql = %s", tmpSql);
		return DBProxySetHallPlayerStatusError;
	}
	
	return SrvOptSuccess;
}

// 获取棋牌室基本信息
int CChessHallOperation::getChessHallBaseInfo(const char* hall_id, com_protocol::ClientHallBaseInfo& baseInfo, const char* username)
{
	// 棋牌室基本信息
	char sqlBuffer[1024] = {0};
	unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select hall_name,hall_logo,UNIX_TIMESTAMP(create_time),description,level,status from tb_chess_card_hall where hall_id=\'%s\' and status!=%u;",
	hall_id, com_protocol::EChessHallStatus::EDisbandHall);

	CQueryResult* p_result = NULL;
	CDBOpertaion* pOptDB = m_msgHandler->getOptDBOpt();
	if (SrvOptSuccess != pOptDB->queryTableAllResult(sqlBuffer, sqlLen, p_result) || p_result == NULL || p_result->getRowCount() != 1)
	{
		pOptDB->releaseQueryResult(p_result);
		
		return DBProxyNotFoundChessHallError;
	}
	
	RowDataPtr rowData = p_result->getNextRow();
	baseInfo.set_id(hall_id);
	baseInfo.set_name(rowData[0]);
	baseInfo.set_logo(rowData[1]);
	baseInfo.set_create_time(atoi(rowData[2]));
	baseInfo.set_desc(rowData[3]);
	baseInfo.set_level(atoi(rowData[4]));
	baseInfo.set_hall_status((com_protocol::EChessHallStatus)atoi(rowData[5]));
	baseInfo.set_player_count(getPlayerCount(hall_id));
	baseInfo.set_player_status(com_protocol::EHallPlayerStatus::EOutHallPlayer);
	
	pOptDB->releaseQueryResult(p_result);
	
	if (username != NULL) baseInfo.set_player_status(getChessHallPlayerStatus(hall_id, username));

	return SrvOptSuccess;
}

// 获取棋牌室游戏信息
int CChessHallOperation::getChessHallGameInfo(const char* hall_id, google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>& gameInfoList)
{
	// 棋牌室游戏信息
	char sqlBuffer[1024] = {0};
	const unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select game_type,service_tax_ratio from tb_hall_game_info where hall_id=\'%s\' and game_status=%u;",
	hall_id, com_protocol::EHallGameStatus::ENormalGame);

    CQueryResult* p_result = NULL;
    CDBOpertaion* pOptDB = m_msgHandler->getOptDBOpt();
	if (SrvOptSuccess != pOptDB->queryTableAllResult(sqlBuffer, sqlLen, p_result))
	{
		return DBProxyQueryHallGameError;
	}
	
	if (p_result == NULL || p_result->getRowCount() < 1)
	{
		// 该棋牌室还没有添加游戏
		pOptDB->releaseQueryResult(p_result);
		return SrvOptSuccess;
	}
	
	RowDataPtr rowData = NULL;
	while ((rowData = p_result->getNextRow()) != NULL)
	{
		com_protocol::ClientHallGameInfo* gameInfo = gameInfoList.Add();
		gameInfo->set_game_type(atoi(rowData[0]));          // 游戏类型
		gameInfo->set_service_tax_ratio(atof(rowData[1]));  // 游戏服务税收比例

		gameInfo->set_room_max_player(0);                   // 由业务层配置决定
		gameInfo->set_current_idx(-1);                      // 房间列表的最后索引值，大于0表示还存在该类房间，小于0则表示该类房间已经遍历完毕
	}
	pOptDB->releaseQueryResult(p_result);
	
	return SrvOptSuccess;
}

// 获取棋牌室当前在线房间信息
int CChessHallOperation::getChessHallOnlineRoomInfo(const char* hall_id, com_protocol::ClientHallGameInfo& gameInfo,
													unsigned int afterIdx, unsigned int count, StringVector& roomIds)
{
	if (count < 1) return SrvOptSuccess;

	// 在逻辑数据库中查询在线的游戏房间数据
	char sqlBuffer[1024] = {0};
	const unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select RM.room_id,RM.room_type,RM.username,ST.portrait_id,RM.room_status,RM.total_game_times,RM.use_game_times,RM.player_count,RM.base_rate,RM.view_flag,RM.idx "
	"from tb_user_room_info as RM left join tb_user_static_baseinfo as ST on RM.username=ST.username "
	"where RM.hall_id=\'%s\' and RM.game_type=%u and (RM.room_status=%u or RM.room_status=%u) and RM.idx > %u limit %u;",
	hall_id, gameInfo.game_type(), com_protocol::EHallRoomStatus::EHallRoomReady, com_protocol::EHallRoomStatus::EHallRoomPlay,
	afterIdx, count);

	CQueryResult* p_result = NULL;
    CDBOpertaion* pLogicDB = m_msgHandler->getLogicDBOpt();
	if (SrvOptSuccess != pLogicDB->queryTableAllResult(sqlBuffer, sqlLen, p_result)) return DBProxyQueryHallOnlineRoomError;
	
	if (p_result == NULL || p_result->getRowCount() < 1)
	{
		// 目前没有在线的游戏房间
		pLogicDB->releaseQueryResult(p_result);
		return SrvOptSuccess;
	}

    int newIdx = -1; // 房间列表的最后索引值，大于0表示还存在该类房间，小于0则表示该类房间已经遍历完毕
	RowDataPtr rowData = NULL;
	while ((rowData = p_result->getNextRow()) != NULL)
	{
		// 游戏房间数据
		// select room_id,room_type,username,room_status,total_game_times,use_game_times,player_count,base_rate,view_flag,idx from tb_user_room_info
		com_protocol::ClientHallRoomInfo* roomInfo = gameInfo.add_room_info();
		roomInfo->set_room_id(rowData[0]);
		roomInfo->set_type((com_protocol::EHallRoomType)atoi(rowData[1]));
		roomInfo->set_username(rowData[2]);
		if (rowData[3] != NULL) roomInfo->set_portrait_id(rowData[3]);
		roomInfo->set_status((com_protocol::EHallRoomStatus)atoi(rowData[4]));
		roomInfo->set_game_times(atoi(rowData[5]));
		roomInfo->set_use_game_times(atoi(rowData[6]));
		roomInfo->set_players(atoi(rowData[7]));
		roomInfo->set_base_rate(atoi(rowData[8]));
		roomInfo->set_is_can_view(atoi(rowData[9]));
		
		newIdx = atoi(rowData[10]);
		roomIds.push_back(rowData[0]);
		
		// OptErrorLog("test get online game type = %u, room = %s, username = %s, new idx = %u",
		// gameInfo.game_type(), rowData[0], rowData[2], newIdx);
	}
	pLogicDB->releaseQueryResult(p_result);
	
	gameInfo.set_current_idx(newIdx);
	
	return SrvOptSuccess;
}

int CChessHallOperation::getChessHallOnlineRoomInfo(const char* hall_id, google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>& gameInfoList,
                                                    unsigned int afterIdx, unsigned int count, StringVector& roomIds)
{
	int rc = SrvOptSuccess;
	for (google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>::iterator it = gameInfoList.begin();
		 it != gameInfoList.end(); ++it)
	{
		rc = getChessHallOnlineRoomInfo(hall_id, *it, afterIdx, count, roomIds);
		if (rc != SrvOptSuccess) return rc;
	}
	
	return SrvOptSuccess;
}

// 获取在线游戏房间的用户信息
int CChessHallOperation::getChessHallOnlineUserInfo(const char* hall_id, const StringVector& roomIds, OnlineUserMap& onlineUsers)
{
	// 查找在线游戏房间的用户信息
	char sqlBuffer[1024 * 1024] = {0};
	unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select username,online_game_service_id,online_room_id,online_seat_id from tb_chess_hall_user_info where hall_id=\'%s\' and "
	"online_game_service_id>0 and online_seat_id>=0",
	hall_id);
	
	// 查询指定房间ID的在线用户
	// "select xxx from table_name where hall=xxx and (roomId=yy or roomId=zzz or roomId=123);"
	StringVector::const_iterator roomIdIt = roomIds.begin();
	if (roomIdIt != roomIds.end())
	{
		sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, " and (online_room_id=\'%s\'", roomIdIt->c_str());
		while (++roomIdIt != roomIds.end())
		{
			sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, " or online_room_id=\'%s\'", roomIdIt->c_str());
		}
		sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, ")");
	}
	sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, ";");
	
	CQueryResult* p_result = NULL;
    CDBOpertaion* pLogicDB = m_msgHandler->getLogicDBOpt();
	if (SrvOptSuccess != pLogicDB->queryTableAllResult(sqlBuffer, sqlLen, p_result))
	{
		return DBProxyQueryOnlinePlayerError;
	}
	
	if (p_result == NULL || p_result->getRowCount() < 1)
	{
		pLogicDB->releaseQueryResult(p_result);
		return DBProxyNotFoundOnlinePlayerError;  // 找不到在线玩家
	}

	// 在线房间的玩家信息
	RowDataPtr rowData = NULL;
	while ((rowData = p_result->getNextRow()) != NULL)
	{
		// select username,online_game_service_id,online_room_id,online_seat_id from tb_chess_hall_user_info
		onlineUsers[rowData[0]] = OnlineUser(atoi(rowData[1]), rowData[2], atoi(rowData[3]));
	}
	pLogicDB->releaseQueryResult(p_result);
	
	return SrvOptSuccess;
}

// 获取在线游戏房间的用户信息
int CChessHallOperation::getChessHallOnlineUserInfo(const char* hall_id, const StringVector& roomIds,
                                                    google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>& gameInfoList)
{
	OnlineUserMap onlineUsers;
	int rc = getChessHallOnlineUserInfo(hall_id, roomIds, onlineUsers);
	if (rc != SrvOptSuccess) return rc;

	// 在线房间的玩家信息
	CUserBaseinfo userBaseinfo;
	for (OnlineUserMap::const_iterator it = onlineUsers.begin(); it != onlineUsers.end(); ++it)
	{
		com_protocol::ClientHallRoomInfo* usrRoomInfo = getPlayerRoom(gameInfoList,
																	  it->second.serviceId / ServiceIdBaseValue,
																	  it->second.roomId.c_str());
		if (usrRoomInfo != NULL && m_msgHandler->getUserBaseinfo(it->first, hall_id, userBaseinfo) == SrvOptSuccess)
		{
			com_protocol::ClientRoomUserInfo* userInfo = usrRoomInfo->add_user_info();
			userInfo->set_username(it->first);
			userInfo->set_nickname(userBaseinfo.static_info.nickname);
			userInfo->set_portrait_id(userBaseinfo.static_info.portrait_id);
			userInfo->set_gender(userBaseinfo.static_info.gender);
			userInfo->set_game_gold(userBaseinfo.dynamic_info.game_gold);
			userInfo->set_seat_id(it->second.seatId);
			
			// OptErrorLog("test get online username = %s, gold = %.2f, service id = %u, room id = %s",
			// it->first.c_str(), userBaseinfo.dynamic_info.game_gold, it->second.serviceId, it->second.roomId.c_str());
		}
		else
		{
			OptErrorLog("get game online room player error, username = %s, service id = %s, room id = %s",
			it->first.c_str(), it->second.serviceId, it->second.roomId.c_str());
		}
	}
	
	return SrvOptSuccess;
}

// 获取棋牌室里的玩家人数
unsigned int CChessHallOperation::getPlayerCount(const char* hall_id)
{
	// select count(user_status) from tb_chess_hall_user_info where hall_id='xx' and (user_status=zz or user_status=xyz);
	char tmpSql[512] = {0};
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 2,
	"select count(user_status) from tb_chess_hall_user_info where hall_id=\'%s\' and user_status=%u;",
	hall_id, com_protocol::EHallPlayerStatus::ECheckAgreedPlayer);
	
	unsigned int playerCount = 0;
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess == m_msgHandler->getLogicDBOpt()->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
	{
		if (pResult != NULL && pResult->getRowCount() == 1)
		{
			RowDataPtr rowData = pResult->getNextRow();
			if (rowData[0] != NULL) playerCount = atoi(rowData[0]);
		}
		
		m_msgHandler->getLogicDBOpt()->releaseQueryResult(pResult);
	}
	else
	{
		OptErrorLog("query chess hall player count error, sql = %s", tmpSql);
	}
	
	return playerCount;
}

// 获取棋牌室用户状态信息
com_protocol::EHallPlayerStatus CChessHallOperation::getChessHallPlayerStatus(const char* hallId, const char* username)
{
    com_protocol::EHallPlayerStatus playerStatus = com_protocol::EHallPlayerStatus::EOutHallPlayer;
	if (hallId == NULL || username == NULL) return playerStatus;
	
	char sqlBuffer[256] = {0};
	const unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select user_status from tb_chess_hall_user_info where hall_id=\'%s\' and username=\'%s\';", hallId, username);

	CQueryResult* p_result = NULL;
	if (SrvOptSuccess == m_msgHandler->getLogicDBOpt()->queryTableAllResult(sqlBuffer, sqlLen, p_result) && p_result != NULL && p_result->getRowCount() == 1)
	{
		RowDataPtr rowData = p_result->getNextRow();
		playerStatus = (com_protocol::EHallPlayerStatus)atoi(rowData[0]);
	}
	
	m_msgHandler->getLogicDBOpt()->releaseQueryResult(p_result);
	
	return playerStatus;
}

// 获取当前时间点棋牌室排行
int CChessHallOperation::getCurrentHallRanking(const char* hall_id, SHallRankingVector& hallRanking)
{
	// 当日玩家排名数据
	const time_t timeSecs = time(NULL);
	struct tm* pTm = localtime(&timeSecs);

	// select PI.username,PI.item_count,U.online_hall_service_id,U.online_game_service_id 
	// from tb_user_pay_income_record as PI left join tb_chess_hall_user_info as U on PI.username=U.username 
	// where PI.hall_id='511319' and PI.opt_type=3 and PI.item_type=2 and PI.item_count>0.001 and 
	// PI.create_time like '2017-12-18%' and U.hall_id='511319';
	char sqlBuffer[1024] = {0};
	const unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select PI.username,PI.item_count,U.online_hall_service_id,U.online_game_service_id "
	"from tb_user_pay_income_record as PI left join tb_chess_hall_user_info as U on PI.username=U.username "
	"where PI.hall_id=\'%s\' and PI.opt_type=%u and PI.item_type=%u and PI.item_count>0.001 and "
	"PI.create_time like \'%d-%02d-%02d%%\' and U.hall_id=\'%s\';",
	hall_id, com_protocol::EHallPlayerOpt::EOptProfitLoss, EGameGoodsType::EGoodsGold,
	(pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday, hall_id);
	
	hallRanking.clear();
	CDBOpertaion* pLogicDB = m_msgHandler->getLogicDBOpt();
	CQueryResult* p_result = NULL;
	if (SrvOptSuccess != pLogicDB->queryTableAllResult(sqlBuffer, sqlLen, p_result))
	{
		return DBProxyGetHallRankingDataError;
	}

	if (p_result == NULL || p_result->getRowCount() < 1)
	{
		pLogicDB->releaseQueryResult(p_result);

		return SrvOptSuccess; // 没有数据
	}
	
	// select PI.username,PI.item_count,U.online_hall_service_id,U.online_game_service_id
	// 玩家输赢数据排名
	RowDataPtr rowData = NULL;
	unsigned int onlineServiceId = 0;
	while ((rowData = p_result->getNextRow()) != NULL)
	{
		SHallRankingVector::iterator rankingIt = std::find(hallRanking.begin(), hallRanking.end(), rowData[0]);
		if (rankingIt != hallRanking.end())
		{
			rankingIt->value += atof(rowData[1]);
		}
		else
		{
			onlineServiceId = atoi(rowData[3]);                                // 游戏服务ID
			if (onlineServiceId == 0) onlineServiceId = atoi(rowData[2]);      // 大厅服务ID
			hallRanking.push_back(SHallRanking(rowData[0], atof(rowData[1]), onlineServiceId / ServiceIdBaseValue));
		}
	}
	pLogicDB->releaseQueryResult(p_result);

	std::sort(hallRanking.begin(), hallRanking.end());
	
	return SrvOptSuccess;
}

int CChessHallOperation::getCurrentHallRanking(const char* hall_id, google::protobuf::RepeatedPtrField<com_protocol::ClientHallRanking>& hallRanking)
{
	// 获取棋牌室排行数据
	CUserBaseinfo userBaseinfo;
	SHallRankingVector rankingInfo;
	
	int rc = getCurrentHallRanking(hall_id, rankingInfo);
	if (rc != SrvOptSuccess) return rc;

    const int maxRanking = m_msgHandler->getSrvOpt().getServiceCommonCfg().chess_hall_cfg.max_ranking;
	for (SHallRankingVector::const_iterator hRankingIt = rankingInfo.begin(); hRankingIt != rankingInfo.end(); ++hRankingIt)
	{
		if (hallRanking.size() >= maxRanking) break;
		
		if (m_msgHandler->getUserBaseinfo(hRankingIt->username, hall_id, userBaseinfo) == SrvOptSuccess)
		{
			com_protocol::ClientHallRanking* hRankingInfo = hallRanking.Add();
			hRankingInfo->set_username(hRankingIt->username);
			hRankingInfo->set_nickname(userBaseinfo.static_info.nickname);
			hRankingInfo->set_portrait_id(userBaseinfo.static_info.portrait_id);
			hRankingInfo->set_gender(userBaseinfo.static_info.gender);
			hRankingInfo->set_game_gold(hRankingIt->value);
			if (hRankingIt->gameType > 0) hRankingInfo->set_game_type(hRankingIt->gameType);
		}
	}
	
	return SrvOptSuccess;
}

// 获取玩家所在的房间
com_protocol::ClientHallRoomInfo* CChessHallOperation::getPlayerRoom(com_protocol::ClientHallGameInfo& gameInfo, const char* roomId)
{
	google::protobuf::RepeatedPtrField<com_protocol::ClientHallRoomInfo>::iterator roomEndIt = gameInfo.mutable_room_info()->end();
	for (google::protobuf::RepeatedPtrField<com_protocol::ClientHallRoomInfo>::iterator roomIt = gameInfo.mutable_room_info()->begin();
		 roomIt != roomEndIt; ++roomIt)
	{
		if (roomIt->room_id() == roomId) return &(*roomIt);
	}
	
	return NULL;
}

com_protocol::ClientHallRoomInfo* CChessHallOperation::getPlayerRoom(google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>& gameInfoList, unsigned int gameType, const char* roomId)
{
	if (gameType < 1 || roomId == NULL || *roomId == '\0') return NULL;

	for (google::protobuf::RepeatedPtrField<com_protocol::ClientHallGameInfo>::iterator gameIt = gameInfoList.begin();
		 gameIt != gameInfoList.end(); ++gameIt)
	{
		if (gameIt->game_type() == gameType) return getPlayerRoom(*gameIt, roomId);
	}
	
	return NULL;
}

// 获取用户所在棋牌室的管理员级别
// 当前玩家用户也有可能是该棋牌室的管理员
int CChessHallOperation::getUserManageLevel(const char* username, const char* hallId, int& manageLevel)
{
	manageLevel = 0;

	char sqlBuffer[1024] = {0};
	unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select platform_id,platform_type from tb_user_platform_map_info where username=\'%s\';", username);

	CQueryResult* pResult = NULL;
	if (SrvOptSuccess != m_msgHandler->getLogicDBOpt()->queryTableAllResult(sqlBuffer, sqlLen, pResult))
	{
		return DBProxyQueryUserPlatformInfoError;
	}

	if (pResult == NULL || pResult->getRowCount() != 1)
	{
		m_msgHandler->getLogicDBOpt()->releaseQueryResult(pResult);

		return DBProxyGetUserPlatformInfoError;
	}
	
	RowDataPtr rowData = pResult->getNextRow();
	sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select D.level from tb_chess_hall_manager_static_info as S left join tb_chess_hall_manager_dynamic_info as D "
	"on S.name=D.name where S.platform_id=\'%s\' and S.platform_type=%d and S.status=0 and D.hall_id=\'%s\' and D.status=0;",
	rowData[0], atoi(rowData[1]) + 1, hallId);
	
	m_msgHandler->getLogicDBOpt()->releaseQueryResult(pResult);
	
	pResult = NULL;
	if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->queryTableAllResult(sqlBuffer, sqlLen, pResult))
	{
		return DBProxyQueryUserManageLevelError;
	}

	if (pResult != NULL && pResult->getRowCount() == 1) manageLevel = atoi(pResult->getNextRow()[0]);
	
	m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);
	
	return SrvOptSuccess;
}

// 获取棋牌室管理员ID
int CChessHallOperation::getManagerIdList(const char* hallId, google::protobuf::RepeatedPtrField<string>& managerIds)
{
	char sqlBuffer[102400] = {0};
	unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select S.platform_id,S.platform_type from tb_chess_hall_manager_dynamic_info as D left join tb_chess_hall_manager_static_info as S "
	"on D.name=S.name where D.hall_id=\'%s\'and D.status=0;", hallId);

	CQueryResult* pResult = NULL;
	if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->queryTableAllResult(sqlBuffer, sqlLen, pResult))
	{
		return DBProxyQueryManagerPlatformInfoError;
	}
	
	sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select username from tb_user_platform_map_info where (platform_id='0' and platform_type=0) ");

	if (pResult != NULL && pResult->getRowCount() > 0)
	{
		RowDataPtr rowData = NULL;
		while ((rowData = pResult->getNextRow()) != NULL)
		{
			sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1,
			                   "or (platform_id=\'%s\' and platform_type=%d)", rowData[0], atoi(rowData[1]) - 1);
		}
	}
	m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);
	
	sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, ";");
	pResult = NULL;
	if (SrvOptSuccess != m_msgHandler->getLogicDBOpt()->queryTableAllResult(sqlBuffer, sqlLen, pResult))
	{
		return DBProxyQueryManagerIdError;
	}
	
	if (pResult != NULL && pResult->getRowCount() > 0)
	{
		RowDataPtr rowData = NULL;
		while ((rowData = pResult->getNextRow()) != NULL)
		{
			managerIds.Add()->assign(rowData[0]);
		}
	}
	m_msgHandler->getLogicDBOpt()->releaseQueryResult(pResult);

	return SrvOptSuccess;
}


bool CChessHallOperation::initHallGameRoomIdRandomNumber()
{
	// 查找目前所有的棋牌室ID
	const char* findRoomIdSql = "select room_id from tb_user_room_info;";
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess != m_msgHandler->getLogicDBOpt()->queryTableAllResult(findRoomIdSql, pResult))
	{
		OptErrorLog("find all chess hall id error, sql = %s", findRoomIdSql);
		return false;
	}

    NumberVector needFilterNumbers;
	if (pResult != NULL && pResult->getRowCount() > 0)
	{
		RowDataPtr rowData = NULL;
		while ((rowData = pResult->getNextRow()) != NULL)
		{
			needFilterNumbers.push_back(atoi(rowData[0]));
		}
	}
	m_msgHandler->getLogicDBOpt()->releaseQueryResult(pResult);
	
	const unsigned int minHallRoomId = 100001;
	const unsigned int maxHallRoomId = 999999;
	const unsigned int minNumberCount = 1000;

	return m_hallGameRoomIdRandomNumbers.initNoRepeatUIntNumber(minHallRoomId, maxHallRoomId, &needFilterNumbers, minNumberCount);
}

// 获取棋牌室不重复的随机游戏房间ID
unsigned int CChessHallOperation::getHallGameRoomNoRepeatRandID()
{
	return m_hallGameRoomIdRandomNumbers.getNoRepeatUIntNumber();
}


// 创建棋牌室充值信息
void CChessHallOperation::createHallRechargeInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CreateHallRechargeInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "create hall recharge info request")) return;

	com_protocol::CreateHallRechargeInfoRsp rsp;
	
	do
	{
		char tmpSql[512] = {0};
		const unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"insert into tb_chess_hall_recharge_info(hall_id,create_time,current_gold,current_room_card) value(\'%s\',now(),%.2f,%.2f);",
		req.hall_id().c_str(), req.current_gold(), req.current_room_card());
		
		CDBOpertaion* logicDbOpt = m_msgHandler->getLogicDBOpt();
		if (SrvOptSuccess != logicDbOpt->executeSQL(tmpSql, tmpSqlLen))
		{
			OptErrorLog("create hall recharge info error, sql = %s", tmpSql);
			rsp.set_result(DBProxyCreateHallRechargeInfoError);
			break;
		}
		else if (1 != logicDbOpt->getAffectedRows())
		{
			m_msgHandler->mysqlRollback();

			OptErrorLog("create hall recharge info failed, sql = %s", tmpSql);
			rsp.set_result(DBProxyCreateHallRechargeInfoError);
			break;
		}
		
		// 提交数据
		m_msgHandler->mysqlCommit();
		
		// 写日志记录
		DBDataRecordLog("CREATE HALL RECHARGE INFO|%s|%s|%.2f|%.2f",
		req.hall_id().c_str(), m_msgHandler->getContext().userData, req.current_gold(), req.current_room_card());

		rsp.set_result(SrvOptSuccess);

	} while (false);
	
	rsp.set_allocated_hall_id(req.release_hall_id());
	rsp.set_current_gold(req.current_gold());
	rsp.set_current_room_card(req.current_room_card());

	if (srcProtocolId == 0) srcProtocolId = DBPROXY_CREATE_HALL_RECHARGE_INFO_RSP;
    int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "create hall recharge info reply");

	OptInfoLog("create hall recharge info reply, result = %d, hall id = %s, username = %s, current gold = %.2f, room card = %.2f, rc = %d",
	rsp.result(), rsp.hall_id().c_str(), m_msgHandler->getContext().userData, rsp.current_gold(), rsp.current_room_card(), rc);
}

void CChessHallOperation::managerOptPlayer(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientManagerOptHallPlayerReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "manager operation hall player request")) return;

	com_protocol::ClientManagerOptHallPlayerRsp rsp;
    do
    {
        CUserBaseinfo userBaseinfo;
        int rc = m_msgHandler->getUserBaseinfo(req.username(), req.hall_id(), userBaseinfo);
        if (rc != SrvOptSuccess)
        {
            rsp.set_result(rc);
            break;
        }
        
        userBaseinfo.dynamic_info.user_status = req.status();
        
        // update tb_chess_hall_user_info set user_status=x where hall_id='x' and username='y';
        char tmpSql[256] = {0};
        unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
        "update tb_chess_hall_user_info set user_status=%u where hall_id=\'%s\' and username=\'%s\';",
        req.status(), req.hall_id().c_str(), req.username().c_str());
            
        CDBOpertaion* dbOpt = m_msgHandler->getLogicDBOpt();
        rc = dbOpt->executeSQL(tmpSql, tmpSqlLen);
        if (SrvOptSuccess != rc)
        {
            OptErrorLog("update chess hall player info error, sql = %s", tmpSql);

            m_msgHandler->mysqlRollback();
            
            rsp.set_result(DBProxyUpdateHallPlayerInfoError);
            break;
        }
		
		if (1 != dbOpt->getAffectedRows())
        {
            OptWarnLog("update chess hall player info warn, affected rows = %u, sql = %s",
            (unsigned int)dbOpt->getAffectedRows(), tmpSql);
        }
        
        if (!m_msgHandler->setUserBaseinfoToMemcached(userBaseinfo))  // 将数据同步到memcached
        {
            m_msgHandler->mysqlRollback();

            rsp.set_result(DBProxySetDataToMemcachedFailed);
            break;
        }
        
        m_msgHandler->mysqlCommit();
        
        rsp.set_status(req.status());
        rsp.set_allocated_username(req.release_username());
        rsp.set_allocated_hall_id(req.release_hall_id());
        rsp.set_result(SrvOptSuccess);

    } while (false);
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_MANAGER_OPT_PLAYER_INFO_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "manager operation hall player reply");
}

void CChessHallOperation::confirmMallBuyInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BSConfirmBuyInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "manager confirm mall buy info request")) return;

	char tmpSql[256] = {0};
	const unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"update tb_mall_record_info set opt_status=%d,manager=\'%s\' where hall_id=\'%s\' and record_id=\'%s\';",
	com_protocol::EMallOptStatus::EMallOptConfirm, req.manager().c_str(), req.hall_id().c_str(), req.record_id().c_str());
	
	com_protocol::BSConfirmBuyInfoRsp rsp;
	if (SrvOptSuccess == m_msgHandler->getLogicDBOpt()->executeSQL(tmpSql, tmpSqlLen))
	{
		m_msgHandler->mysqlCommit();

		rsp.set_result(SrvOptSuccess);
		rsp.set_allocated_record_id(req.release_record_id());
	}
	else
	{
		OptErrorLog("update mall buy info status error, sql = %s", tmpSql);
		rsp.set_result(DBProxyUpdateMallOptStatusError);
	}
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_CONFIRM_MALL_BUY_INFO_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "manager confirm mall buy info reply");
}

void CChessHallOperation::giveMallGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BSGiveGoodsReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "manager give mall goods request")) return;

    int rc = SrvOptFailed;
    com_protocol::BSGiveGoodsRsp rsp;
	do
	{
		// 刷新棋牌室物品和商城记录到DB
		const com_protocol::ItemChange& giveItem = req.change_goods();
		char tmpSql[1024] = {0};
		unsigned int tmpSqlLen = 0;		
		if (giveItem.type() == EGameGoodsType::EGoodsGold)
		{
			tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		    "update tb_chess_hall_recharge_info set current_gold=current_gold - %.2f where hall_id=\'%s\' and current_gold >= %.2f;",
		    giveItem.num(), req.hall_id().c_str(), giveItem.num());
		}
		else if (giveItem.type() == EGameGoodsType::EGoodsRoomCard)
		{
			tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		    "update tb_chess_hall_recharge_info set current_room_card=current_room_card - %.2f where hall_id=\'%s\' and current_room_card >= %.2f;",
		    giveItem.num(), req.hall_id().c_str(), giveItem.num());
		}
		else
		{
			rsp.set_result(DBProxyGoodsTypeInvalid);
			break;
		}

        // 扣掉棋牌室物品数量
        CDBOpertaion* logicDbOpt = m_msgHandler->getLogicDBOpt();
		if (SrvOptSuccess != logicDbOpt->executeSQL(tmpSql, tmpSqlLen))
		{
			OptErrorLog("manager give goods update chess hall goods error, sql = %s", tmpSql);
			rsp.set_result(DBProxyUpdateChessHallGoodsError);
			break;
		}
		else if (1 != logicDbOpt->getAffectedRows())
		{
			m_msgHandler->mysqlRollback();

			OptErrorLog("manager give goods update chess hall goods failed, sql = %s", tmpSql);
			rsp.set_result(DBProxyUpdateChessHallGoodsError);
			break;
		}
		
		// 记录ID
		RecordIDType recordId = {0};
		m_msgHandler->getSrvOpt().getRecordId(recordId);

		// 赠送或者扣除物品数量
		int optType = com_protocol::EHallPlayerOpt::EOptGiveGift;
		float optNum = giveItem.num();
		const char* optMsg = "MANAGER GIVE GOODS";
		if (optNum < 0.00)
		{
			optType = com_protocol::EHallPlayerOpt::EOptBuyGoods;
			optNum = -optNum;
			optMsg = "MANAGER DEDUCTION GOODS";
		}
		
		// 写商城记录
		tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"insert into tb_mall_record_info(record_id,create_time,hall_id,manager,username,opt_type,opt_status,pay_type,pay_count,get_type,get_count,remarks) "
		"value(\'%s\',now(),\'%s\',\'%s\',\'%s\',%d,%d,%u,%.2f,%u,%.2f,\'%s\');",
		recordId, req.hall_id().c_str(), m_msgHandler->getContext().userData, req.username().c_str(),
		optType, com_protocol::EMallOptStatus::EMallOptUnconfirm,
		giveItem.type(), optNum, giveItem.type(), optNum, optMsg);
		
		if (SrvOptSuccess != logicDbOpt->executeSQL(tmpSql, tmpSqlLen))
		{
			m_msgHandler->mysqlRollback();
			
			OptErrorLog("write manager give goods record error, sql = %s", tmpSql);
			rsp.set_result(DBProxyWriteOptMallGoodsRecordError);
			break;
		}

		// 设置玩家道具变更记录基本信息
		com_protocol::ChangeRecordBaseInfo recordInfo;
        m_msgHandler->getLogicHandler().setRecordBaseInfo(recordInfo, srcSrvId, req.hall_id(), "", 0, optType,
		recordId, "manager give goods to player");
		
		// 增加玩家道具数量
		google::protobuf::RepeatedPtrField<com_protocol::ItemChange> items;
		*items.Add() = giveItem;

		CUserBaseinfo userBaseinfo;
        com_protocol::ChangeRecordPlayerInfo playerInfo;
		rc = m_msgHandler->getLogicHandler().changeUserGoodsValue(req.hall_id(), req.username(), items, userBaseinfo, recordInfo, playerInfo);
		
		// 不管是否成功都需要写数据记录日志
		m_msgHandler->getLogicHandler().writeItemDataLog(items, req.username(), m_msgHandler->getContext().userData, optType, rc, optMsg);
		
		// 提交数据
		if (rc == SrvOptSuccess) m_msgHandler->mysqlCommit();
		else m_msgHandler->mysqlRollback();
		
		rsp.set_result(rc);
		*rsp.mutable_change_goods() = items.Get(0);
		rsp.set_allocated_username(req.release_username());
		rsp.set_allocated_hall_id(req.release_hall_id());
		rsp.set_allocated_record_id(recordInfo.release_record_id());

	} while (false);

	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GIVE_MALL_GOODS_RSP;
    rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "manager give mall goods reply");
	
	OptInfoLog("manager give goods reply, result = %d, hall id = %s, username = %s, give good type = %u, count = %.2f, record id = %s, rc = %d",
	rsp.result(), rsp.hall_id().c_str(), rsp.username().c_str(), rsp.change_goods().type(), rsp.change_goods().num(),
	rsp.record_id().c_str(), rc);
}

// 棋牌室管理员充值
void CChessHallOperation::chessHallManagerRecharge(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserRechargeReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "manager recharge request")) return;

    com_protocol::UserRechargeRsp rsp;

	do
	{
		// 先验证棋牌室信息
		char tmpSql[1024] = {0};
		unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"select 1 from tb_chess_hall_manager_dynamic_info where name=\'%s\' and hall_id=\'%s\';",
		req.username().c_str(), req.hall_id().c_str());
		
		CQueryResult* pResult = NULL;
		if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->queryTableAllResult(tmpSql, tmpSqlLen, pResult)
		    || pResult == NULL || pResult->getRowCount() != 1)
		{
			m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);
			
			rsp.set_result(DBProxyNotFoundChessHallError);
			break;
		}
		m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);

		// 刷新棋牌室物品和充值记录到DB
		if (req.item_type() == EGameGoodsType::EGoodsGold)
		{
			tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		    "update tb_chess_hall_recharge_info set rmb_gold=rmb_gold+%.2f,recharge_gold=recharge_gold+%.2f,recharge_gold_times=recharge_gold_times+1,current_gold=current_gold+%.2f where hall_id=\'%s\';",
		    req.recharge_rmb(), req.item_count(), req.item_count(), req.hall_id().c_str());
		}
		
		/*
		else if (giveItem.type() == EGameGoodsType::EGoodsRoomCard)
		{
			tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		    "update tb_chess_hall_recharge_info set current_room_card=current_room_card - %.2f where hall_id=\'%s\' and current_room_card >= %.2f;",
		    giveItem.num(), req.hall_id().c_str(), giveItem.num());
		}
		*/
 
		else
		{
			rsp.set_result(DBProxyGoodsTypeInvalid);
			break;
		}

        // 增加棋牌室物品数量
        CDBOpertaion* logicDbOpt = m_msgHandler->getLogicDBOpt();
		if (SrvOptSuccess != logicDbOpt->executeSQL(tmpSql, tmpSqlLen))
		{
			OptErrorLog("manager recharge update chess hall goods error, sql = %s", tmpSql);
			rsp.set_result(DBProxyManagerRechargeGoodsError);
			break;
		}
		else if (1 != logicDbOpt->getAffectedRows())
		{
			m_msgHandler->mysqlRollback();

			OptErrorLog("manager recharge update chess hall goods failed, sql = %s", tmpSql);
			rsp.set_result(DBProxyManagerRechargeGoodsError);
			break;
		}

		// 写充值记录
		tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"insert into tb_hall_manager_recharge_record(bills_id,hall_id,username,create_time,item_type,item_id,item_count,item_amount,recharge_rmb,third_account,third_type,third_others_data) "
		"value(\'%s\',\'%s\',\'%s\',now(),%u,%u,%.2f,%.2f,%.2f,\'%s\',%d,\'%s\');",
		req.order_id().c_str(), req.hall_id().c_str(), req.username().c_str(), req.item_type(), req.item_id(), req.item_count(), req.item_amount(),
		req.recharge_rmb(), req.third_account().c_str(), req.third_type(), req.third_other_data().c_str());
		
		if (SrvOptSuccess != logicDbOpt->executeSQL(tmpSql, tmpSqlLen))
		{
			m_msgHandler->mysqlRollback();
			
			OptErrorLog("write manager recharge record error, sql = %s", tmpSql);
			rsp.set_result(DBProxyWriteManagerRechargeRecordError);
			break;
		}
		
		// 提交数据
		m_msgHandler->mysqlCommit();
		
		// 写充值日志记录
		DBDataRecordLog("MANAGER RECHARGE|%s|%s|%u|%u|%.2f|%.2f|%.2f|%s|%s|%d|%s",
		req.hall_id().c_str(), req.username().c_str(),
		req.item_type(), req.item_id(), req.item_count(), req.item_amount(), req.recharge_rmb(),
		req.order_id().c_str(), req.third_account().c_str(), req.third_type(), req.third_other_data().c_str());

		rsp.set_result(SrvOptSuccess);

	} while (false);
	
	rsp.set_item_amount(req.item_amount());
	rsp.set_allocated_info(&req);

	if (srcProtocolId == 0) srcProtocolId = DBPROXY_MANAGER_RECHARGE_RSP;
    int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "manager recharge goods reply");
	
	rsp.release_info();
	
	OptInfoLog("manager recharge goods reply, result = %d, hall id = %s, username = %s, good type = %u, count = %.2f, rmb = %.2f, record id = %s, rc = %d",
	rsp.result(), req.hall_id().c_str(), req.username().c_str(), req.item_type(), req.item_count(), req.recharge_rmb(), req.order_id().c_str(), rc);
}
	
	
void CChessHallOperation::applyIntoChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientApplyJoinChessHallReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "player apply into chess hall request")) return;

	com_protocol::ClientApplyJoinChessHallRsp rsp;
	rsp.set_result(applyJoinChessHall(m_msgHandler->getContext().userData, req.hall_id(), req.explain_msg(), srcSrvId));
	rsp.set_allocated_hall_id(req.release_hall_id());
	if (req.has_explain_msg()) rsp.set_allocated_explain_msg(req.release_explain_msg());
	
	// 获取棋牌室管理员ID
	if (rsp.result() == SrvOptSuccess) rsp.set_result(getManagerIdList(rsp.hall_id().c_str(), *rsp.mutable_managers()));

	if (srcProtocolId == 0) srcProtocolId = DBPROXY_APPLY_JOIN_CHESS_HALL_RSP;
	int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "player apply into chess hall reply");
	
	OptInfoLog("player apply into chess hall, id = %s, username = %s, result = %d, rc = %d",
	rsp.hall_id().c_str(), m_msgHandler->getContext().userData, rsp.result(), rc);
}

void CChessHallOperation::cancelApplyChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientCancelApplyChessHallReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "player cancel apply chess hall request")) return;

	// update tb_chess_hall_user_info set user_status=x where hall_id='x' and username='y' and user_status=xxx;
	char tmpSql[512] = {0};
	const unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"update tb_chess_hall_user_info set user_status=%u where hall_id=\'%s\' and username=\'%s\' and user_status=%u;",
	com_protocol::EHallPlayerStatus::ECancelApply, req.hall_id().c_str(), m_msgHandler->getContext().userData,
	com_protocol::EHallPlayerStatus::EWaitForCheck);
	
	com_protocol::ClientCancelApplyChessHallRsp rsp;
	rsp.set_result(SrvOptSuccess);

	if (SrvOptSuccess == m_msgHandler->getOptDBOpt()->executeSQL(tmpSql, tmpSqlLen))
	{
		m_msgHandler->mysqlCommit();
		
	}
	else
	{
		OptErrorLog("cancel apply chess hall error, sql = %s", tmpSql);
		rsp.set_result(DBProxyCancelApplyChessHallError);
	}
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_CANCEL_APPLY_CHESS_HALL_RSP;
	int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "player cancel apply chess hall reply");
	
	OptInfoLog("player cancel apply chess hall, id = %s, username = %s, result = %d, rc = %d",
	req.hall_id().c_str(), m_msgHandler->getContext().userData, rsp.result(), rc);
}

// 创建棋牌室房间
void CChessHallOperation::createHallGameRoom(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientCreateHallGameRoomReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "create hall game room request")) return;
	
	int rc = SrvOptFailed;
	com_protocol::ClientCreateHallGameRoomRsp rsp;
	do
	{
        // select count(*) from tb_user_room_info where username='xx' and create_time like '2017-10-18%';
        const time_t timeSecs = time(NULL);
        const struct tm* pTm = localtime(&timeSecs);
        char tmpSql[10240] = {0};
        unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
        "select count(*) from tb_user_room_info where username=\'%s\' and room_status!=%u and create_time like \'%d-%02d-%02d%%\';",
        m_msgHandler->getContext().userData, com_protocol::EHallRoomStatus::EHallRoomFinish,
        (pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday);

        unsigned int createNoUseRoomCount = 0;
        CQueryResult* pResult = NULL;
        if (SrvOptSuccess == m_msgHandler->getLogicDBOpt()->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
        {
            if (pResult != NULL && pResult->getRowCount() == 1)
            {
                RowDataPtr rowData = pResult->getNextRow();
                if (rowData[0] != NULL) createNoUseRoomCount = atoi(rowData[0]);
            }

            m_msgHandler->getLogicDBOpt()->releaseQueryResult(pResult);
        }
        else
        {
            rsp.set_result(DBProxyQueryUserRoomInfoError);
            break;
        }
        
        if (createNoUseRoomCount > m_msgHandler->getSrvOpt().getServiceCommonCfg().chess_hall_cfg.max_no_finish_room_count)
        {
            rsp.set_result(DBProxyUserCreateMoreNoFinishRoom);
            break;
        }
        
        // 该棋牌室是否存在该游戏信息
        // select 1 from tb_hall_game_info where hall_id='xx' and game_type=xxx and game_status=xx limit 1
        tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
        "select 1 from tb_hall_game_info where hall_id=\'%s\' and game_type=%u and game_status=%d limit 1;",
        req.room_info().base_info().hall_id().c_str(), req.room_info().base_info().game_type(), com_protocol::EHallGameStatus::ENormalGame);

        bool hallHasGame = false;
        pResult = NULL;
        if (SrvOptSuccess == m_msgHandler->getOptDBOpt()->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
        {
            hallHasGame = (pResult != NULL && pResult->getRowCount() > 0);

            m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);
        }
        else
        {
            rsp.set_result(DBProxyQueryHallGameInfoError);
            break;
        }
        
        if (!hallHasGame)
        {
            rsp.set_result(DBProxyNoFoundHallGameInfoError);
            break;
        }

		rsp.set_result(SrvOptSuccess);
		rsp.set_allocated_room_info(req.release_room_info());
	
		const com_protocol::ChessHallRoomInfo& roomInfo = rsp.room_info();
		const com_protocol::ChessHallRoomBaseInfo baseInfo = roomInfo.base_info();  // 先拷贝一份
		
		// 棋牌室ID合法性判定
		const string& hallId = baseInfo.hall_id();
		if (hallId.length() >= 32 || hallId.length() < 3)
		{
			rsp.set_result(DBProxyCreateRoomInvalidParam);
			break;
		}
		
		CUserBaseinfo user_baseinfo;
		rc = m_msgHandler->getUserBaseinfo(m_msgHandler->getContext().userData, hallId, user_baseinfo);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		// 牛牛游戏创建房间，金币检查
		if (roomInfo.has_cattle_room() && roomInfo.cattle_room().need_gold() > user_baseinfo.dynamic_info.game_gold)
		{
			rsp.set_result(DBProxyCreateRoomGoldInsufficient);
			break;
		}

		// insert blob data type
		// 基本数据只存储游戏类型作为索引映射对应的游戏协议数据，其他基本信息从mysql table读写（管理后台需要这些数据做统计查询等）
		rsp.mutable_room_info()->clear_base_info();  // 清空基本数据，只保存游戏类型字段
		com_protocol::ChessHallRoomBaseInfo* newBaseInfo = rsp.mutable_room_info()->mutable_base_info();
		newBaseInfo->set_game_type(baseInfo.game_type());
		
		const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(roomInfo, "save room info to db talbe");
		if (msgData == NULL)
		{
			rsp.set_result(DBProxySerializeRoomInfoError);
			break;
		}
		
		char roomData[10240] = {0};
		unsigned long roomDataLen = roomInfo.ByteSize();
		CDBOpertaion* optDB = m_msgHandler->getLogicDBOpt();
		if (!optDB->realEscapeString(roomData, msgData, roomDataLen))
		{
			rsp.set_result(DBProxyFormatRoomInfoError);
			break;
		}

		// 获取房间不重复的随机ID
	    const unsigned int roomId = getHallGameRoomNoRepeatRandID();
	    tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"insert into tb_user_room_info(hall_id,room_id,username,create_time,game_type,room_type,view_flag,base_rate,player_count,room_status,room_info) "
		"value(\'%s\',\'%u\',\'%s\',now(),%u,%u,%d,%u,%u,%u,\'%s\');",
		hallId.c_str(), roomId, m_msgHandler->getContext().userData, baseInfo.game_type(), baseInfo.room_type(), baseInfo.is_can_view(), baseInfo.base_rate(),
		baseInfo.player_count(), com_protocol::EHallRoomStatus::EHallRoomCreated, roomData);
			 
		// 插入表数据
		if (SrvOptSuccess != optDB->executeSQL(tmpSql, tmpSqlLen))
		{
			OptErrorLog("add hall game room error, exec sql = %s", tmpSql);
			rsp.set_result(DBProxyAddHallGameRoomError);
			break;
		}
		
		m_msgHandler->mysqlCommit();

		// 填写棋牌室游戏房间ID
		snprintf(roomData, sizeof(roomData) - 1, "%u", roomId);
		
		*newBaseInfo = baseInfo;
		newBaseInfo->set_room_id(roomData);

	} while (false);
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_CREATE_HALL_GAME_ROOM_RSP;
	rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "create hall game room reply");

    const com_protocol::ChessHallRoomBaseInfo& logBaseInfo = rsp.room_info().base_info();
	OptInfoLog("create hall game room, username = %s, hall id = %s, room id = %s, game type = %u, room type = %u, result = %d, send msg rc = %d",
	m_msgHandler->getContext().userData, logBaseInfo.hall_id().c_str(), logBaseInfo.room_id().c_str(),
	logBaseInfo.game_type(), logBaseInfo.room_type(), rsp.result(), rc);
}

// 修改棋牌室游戏房间状态
void CChessHallOperation::changeHallGameRoomStatus(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ChangeHallGameRoomStatusReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "change hall game room status request")) return;

	char tmpSql[256] = {0};
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1, "update tb_user_room_info set room_status=%u where room_id=\'%s\';",
									  req.new_status(), req.room_id().c_str());

	if (SrvOptSuccess == m_msgHandler->getLogicDBOpt()->executeSQL(tmpSql, tmpSqlLen))
	{
		m_msgHandler->mysqlCommit();
	}
	else
	{
		OptErrorLog("update hall game room status error, exec sql = %s", tmpSql);
	}
	
	OptInfoLog("update hall game room status, username = %s, roomId = %s, new status = %d",
	m_msgHandler->getContext().userData, req.room_id().c_str(), req.new_status());
}

// 修改棋牌室游戏房间玩家座位号
void CChessHallOperation::changeGameRoomPlayerSeat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ChangeGameRoomPlayerSeatReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "change game room player seat request")) return;

	char tmpSql[256] = {0};
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"update tb_chess_hall_user_info set online_seat_id=%d where username=\'%s\' and online_room_id=\'%s\';",
	req.seat_id(), req.username().c_str(), req.room_id().c_str());

	if (SrvOptSuccess == m_msgHandler->getLogicDBOpt()->executeSQL(tmpSql, tmpSqlLen))
	{
		m_msgHandler->mysqlCommit();
	}
	else
	{
		OptErrorLog("update game room player seat error, exec sql = %s", tmpSql);
	}
	
	OptInfoLog("update game room player seat, username = %s, roomId = %s, new seat = %d",
	req.username().c_str(), req.room_id().c_str(), req.seat_id());
}


// 玩家购买商城物品
void CChessHallOperation::buyMallGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/*
	com_protocol::ClientBuyMallGoodsReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "player buy mall goods request")) return;
	
	int rc = SrvOptFailed;
	const char* username = m_msgHandler->getContext().userData;
	com_protocol::ClientBuyMallGoodsRsp rsp;
	do
	{
		// 玩家信息验证
		CUserBaseinfo userBaseinfo;
		rc = m_msgHandler->getUserBaseinfo(username, req.hall_id().c_str(), userBaseinfo);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		const MallConfigData::MallGoodsCfg* mallGoodsCfg = m_msgHandler->getMallGoodsCfg(req.buy_goods().type());
		if (mallGoodsCfg == NULL)
		{
			rsp.set_result(DBProxyGetMallGoodsInfoError);
			break;
		}
		
		// 金币验证
		const double payCount = mallGoodsCfg->price * req.buy_goods().count();
		if (payCount > userBaseinfo.dynamic_info.game_gold)
		{
			rsp.set_result(DBProxyGoodsNotEnought);
			break;
		}

		// 刷新棋牌室金币和商城记录到DB
		char tmpSql[1024] = {0};
		unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"update tb_chess_hall_recharge_info set current_gold=current_gold + %.2f where hall_id=\'%s\';",
		payCount, req.hall_id().c_str());

        // 增加棋牌室金币
        CDBOpertaion* logicDbOpt = m_msgHandler->getLogicDBOpt();
		if (SrvOptSuccess != logicDbOpt->executeSQL(tmpSql, tmpSqlLen))
		{
			OptErrorLog("player buy goods update chess hall goods error, sql = %s", tmpSql);
			rsp.set_result(DBProxyUpdateChessHallGoodsError);
			break;
		}
		else if (1 != logicDbOpt->getAffectedRows())
		{
			m_msgHandler->mysqlRollback();

			OptErrorLog("player buy goods update chess hall goods failed, sql = %s", tmpSql);
			rsp.set_result(DBProxyUpdateChessHallGoodsError);
			break;
		}
		
		// 记录ID
		RecordIDType recordId = {0};
		m_msgHandler->getSrvOpt().getRecordId(recordId);

		// 写商城记录
		const int optType = com_protocol::EHallPlayerOpt::EOptBuyGoods;		
		tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"insert into tb_mall_record_info(record_id,create_time,hall_id,username,opt_type,opt_status,pay_type,pay_count,get_type,get_count,remarks) "
		"value(\'%s\',now(),\'%s\',\'%s\',%d,%d,%u,%.2f,%u,%.2f,\'%s\');",
		recordId, req.hall_id().c_str(), username, optType, com_protocol::EMallOptStatus::EMallOptUnconfirm,
		EGameGoodsType::EGoodsGold, payCount, req.buy_goods().type(), req.buy_goods().count(), "player buy mall goods");
		
		if (SrvOptSuccess != logicDbOpt->executeSQL(tmpSql, tmpSqlLen))
		{
			m_msgHandler->mysqlRollback();
			
			OptErrorLog("write player buy goods record error, sql = %s", tmpSql);
			rsp.set_result(DBProxyWriteOptMallGoodsRecordError);
			break;
		}
		
		// 设置玩家道具变更记录基本信息
		com_protocol::ChangeRecordBaseInfo recordInfo;
        m_msgHandler->getLogicHandler().setRecordBaseInfo(recordInfo, srcSrvId, req.hall_id(), "", 0, optType,
		recordId, "player buy goods from mall");
		
		// 扣掉玩家道具数量
		google::protobuf::RepeatedPtrField<com_protocol::ItemChange> items;
		com_protocol::ItemChange* playerPayItem = items.Add();
		playerPayItem->set_type(EGameGoodsType::EGoodsGold);
		playerPayItem->set_num(-payCount);

        com_protocol::ChangeRecordPlayerInfo playerInfo;
		rc = m_msgHandler->getLogicHandler().changeUserGoodsValue(req.hall_id(), username, items, userBaseinfo, recordInfo, playerInfo);
		
		// 不管是否成功都需要写数据记录日志
		m_msgHandler->getLogicHandler().writeItemDataLog(items, username, "mall", optType, rc, "PLAYER BUY ITEM");
		
		// 提交数据
		if (rc == SrvOptSuccess) m_msgHandler->mysqlCommit();
		else m_msgHandler->mysqlRollback();
		
		rsp.set_result(rc);
		rsp.set_allocated_hall_id(req.release_hall_id());
		rsp.set_allocated_buy_goods(req.release_buy_goods());
		*rsp.mutable_gold_info() = items.Get(0);

	} while (false);
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_PLAYER_BUY_MALL_GOODS_RSP;
	rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "player buy mall goods reply");
	
	OptInfoLog("player buy mall goods reply, result = %d, hall id = %s, username = %s, buy good type = %u, count = %.2f, pay type = %u, count = %.2f, remain = %.2f, rc = %d",
	rsp.result(), rsp.hall_id().c_str(), username, rsp.buy_goods().type(), rsp.buy_goods().count(),
	rsp.gold_info().type(), rsp.gold_info().num(), rsp.gold_info().amount(), rc);
	*/ 
}

// 玩家获取商城购买记录
void CChessHallOperation::getMallBuyRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetBuyMallGoodsRecordReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get mall buy goods record")) return;

	// select record_id,UNIX_TIMESTAMP(create_time),pay_type,pay_count,get_type,get_count,idx
	// from tb_mall_record_info where opt_type=x and hall_id = 'xx' and username='xx' and idx > y limit z;
	const unsigned int getMallRecordMaxCount = m_msgHandler->getSrvOpt().getServiceCommonCfg().chess_hall_cfg.player_get_mall_record_count;
	char sql[1024] = {0};
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1,
	"select record_id,UNIX_TIMESTAMP(create_time),pay_type,pay_count,get_type,get_count,idx "
	"from tb_mall_record_info where opt_type=%d and hall_id=\'%s\' and username=\'%s\' and idx > %u limit %u;",
	com_protocol::EHallPlayerOpt::EOptBuyGoods, req.hall_id().c_str(), m_msgHandler->getContext().userData,
	req.current_idx(), getMallRecordMaxCount);

    com_protocol::ClientGetBuyMallGoodsRecordRsp rsp;
	CQueryResult* pResult = NULL;
	do
	{
		if (SrvOptSuccess != m_msgHandler->getLogicDBOpt()->queryTableAllResult(sql, sqlLen, pResult))
		{
			OptErrorLog("query player mall buy record error, sql = %s", sql);

			rsp.set_result(DBProxyQueryMallPlayerBuyInfoError);
			break;
		}

		int currentIdx = -1;
		if (pResult == NULL || pResult->getRowCount() < 1)  // 没有数据
		{
			rsp.set_result(SrvOptSuccess);
			rsp.set_current_idx(currentIdx);
			break;
		}

		// select record_id,UNIX_TIMESTAMP(create_time),pay_type,pay_count,get_type,get_count,idx
		const MallConfigData::MallGoodsCfg* mallGoodsCfg = NULL;
		RowDataPtr rowData = NULL;
		while ((rowData = pResult->getNextRow()) != NULL)
		{
			com_protocol::MallOptInfo* optInfo = rsp.add_buy_info();
			optInfo->set_record_id(rowData[0]);
			optInfo->set_time_secs(atoi(rowData[1]));

			com_protocol::MallGoods* mallGoods = optInfo->mutable_goods();
			com_protocol::Goods* payGoods = mallGoods->mutable_pay_goods();
			payGoods->set_type(atoi(rowData[2]));
			payGoods->set_count(atof(rowData[3]));
			payGoods->set_price(1.00);
			
			com_protocol::Goods* buyGoods = mallGoods->mutable_buy_goods();
			
			mallGoodsCfg = m_msgHandler->getMallGoodsCfg(atoi(rowData[4]));
			buyGoods->set_type(atoi(rowData[4]));
			buyGoods->set_count(atof(rowData[5]));
			(mallGoodsCfg != NULL) ? buyGoods->set_price(mallGoodsCfg->price) : buyGoods->set_price(1.00);

			if (atoi(rowData[6]) > currentIdx) currentIdx = atoi(rowData[6]);
		}

		if ((unsigned int)rsp.buy_info_size() < getMallRecordMaxCount) currentIdx = -1;
		
		rsp.set_result(SrvOptSuccess);
		rsp.set_current_idx(currentIdx);
	
	} while (false);
	
	m_msgHandler->getLogicDBOpt()->releaseQueryResult(pResult);
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_PLAYER_GET_MALL_BUY_RECORD_RSP;
	int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get mall buy goods record reply");

	OptInfoLog("get mall buy goods record, username = %s, hall id = %s, idx = %d, result = %d, new idx = %d, send msg rc = %d",
	m_msgHandler->getContext().userData, req.hall_id().c_str(), req.current_idx(), rsp.result(), rsp.current_idx(), rc);
}

// 管理员赠送物品之后，玩家确认收货
void CChessHallOperation::confirmGiveGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGiveGoodsPlayerConfirmNotify cfmNtf;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(cfmNtf, data, len, "player confirm give goods notify")
	    || cfmNtf.record_id_size() < 1) return;

	google::protobuf::RepeatedPtrField<string>::const_iterator it = cfmNtf.record_id().begin();
	char tmpSql[10240] = {0};
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"update tb_mall_record_info set opt_status=%d,username=\'%s\' where record_id=\'%s\'",
	com_protocol::EMallOptStatus::EMallOptConfirm, m_msgHandler->getContext().userData, it->c_str());
	
	while (++it != cfmNtf.record_id().end())
	{
		tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 1, " or record_id=\'%s\'", it->c_str());
	}
	tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 1, ";");

	if (SrvOptSuccess != m_msgHandler->getLogicDBOpt()->executeSQL(tmpSql, tmpSqlLen))
	{
		OptErrorLog("update mall give goods status error, sql = %s", tmpSql);
		
		return;
	}
	
	if ((unsigned int)cfmNtf.record_id_size() != m_msgHandler->getLogicDBOpt()->getAffectedRows())
	{
		OptErrorLog("update mall give goods status failed, record size = %d, affected rows = %lu, sql = %s",
		cfmNtf.record_id_size(), m_msgHandler->getLogicDBOpt()->getAffectedRows(), tmpSql);
	}
	
	m_msgHandler->mysqlCommit();
	
	OptInfoLog("player confirm give goods, username = %s, record size = %d, first id = %s",
	m_msgHandler->getContext().userData, cfmNtf.record_id_size(), cfmNtf.record_id().begin()->c_str());
}


// 获取棋牌室基本信息
void CChessHallOperation::getHallBaseInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetHallBaseInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get hall base info request")) return;
	
	int rc = SrvOptFailed;
	com_protocol::ClientGetHallBaseInfoRsp rsp;
	if (req.has_hall_id())
	{
	    // 指定棋牌室的基本信息
		rsp.set_allocated_hall_id(req.release_hall_id());
	    rc = getChessHallBaseInfo(rsp.hall_id().c_str(), *rsp.add_hall_base_info(), m_msgHandler->getContext().userData);
	}
    else
	{
		// 用户所在棋牌室信息
		rc = getUserChessHallBaseInfo(m_msgHandler->getContext().userData, *rsp.mutable_hall_base_info());
	}
	
	rsp.set_result(rc);
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_HALL_BASE_INFO_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get hall base info reply");
}
	
// 获取棋牌室当前所有信息
void CChessHallOperation::getChessHallInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetHallInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get chess hall info request")) return;

    com_protocol::ClientGetHallInfoRsp rsp;
	rsp.set_result(getUserChessHallInfo(req.username().c_str(), req.hall_id().c_str(), req.max_online_rooms(), *rsp.mutable_hall_info()));

	if (rsp.result() != SrvOptSuccess) rsp.clear_hall_info();
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_CHESS_HALL_INFO_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get chess hall info reply");
}

// 获取棋牌室房间列表
void CChessHallOperation::getHallRoomList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetHallGameRoomReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get hall room list request")) return;

    com_protocol::ClientGetHallGameRoomRsp rsp;
	rsp.set_hall_id(req.hall_id());
	
	do
	{
		// 获取棋牌室当前在线房间信息
		com_protocol::ClientHallGameInfo* gameInfo = rsp.mutable_game_room_info();
		gameInfo->set_game_type(req.game_type());
		gameInfo->set_room_max_player(0);  // 由业务层配置决定
		
		StringVector roomIds;
		int rc = getChessHallOnlineRoomInfo(req.hall_id().c_str(), *gameInfo, req.current_idx(), req.max_online_rooms(), roomIds);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		if (!roomIds.empty())
		{
			OnlineUserMap onlineUsers;
			rc = getChessHallOnlineUserInfo(req.hall_id().c_str(), roomIds, onlineUsers);
			if (rc == DBProxyNotFoundOnlinePlayerError)
			{
				// 处理错误的在线房间
				OptErrorLog("get hall room list online player error, username = %s, hallId = %s, room count = %u",
				m_msgHandler->getContext().userData, req.hall_id().c_str(), roomIds.size());
			}
			else if (SrvOptSuccess != rc)
			{
				rsp.set_result(rc);
				break;
			}
			
			// 清理无在线玩家的房间
			

			// 在线房间的玩家信息
			CUserBaseinfo userBaseinfo;
			for (OnlineUserMap::const_iterator it = onlineUsers.begin(); it != onlineUsers.end(); ++it)
			{
				com_protocol::ClientHallRoomInfo* usrRoomInfo = getPlayerRoom(*gameInfo, it->second.roomId.c_str());
				if (usrRoomInfo != NULL && m_msgHandler->getUserBaseinfo(it->first, req.hall_id(), userBaseinfo) == SrvOptSuccess)
				{
					com_protocol::ClientRoomUserInfo* userInfo = usrRoomInfo->add_user_info();
					userInfo->set_username(it->first);
					userInfo->set_nickname(userBaseinfo.static_info.nickname);
					userInfo->set_portrait_id(userBaseinfo.static_info.portrait_id);
					userInfo->set_gender(userBaseinfo.static_info.gender);
					userInfo->set_game_gold(userBaseinfo.dynamic_info.game_gold);
					userInfo->set_seat_id(it->second.seatId);
					
					// OptErrorLog("test get room list online username = %s, gold = %.2f, service id = %u, room id = %s",
					// it->first.c_str(), userBaseinfo.dynamic_info.game_gold, it->second.serviceId, it->second.roomId.c_str());
				}
				else
				{
					OptErrorLog("get game online room list player error, username = %s, service id = %s, room id = %s",
					it->first.c_str(), it->second.serviceId, it->second.roomId.c_str());
				}
			}
		}
		
		rsp.set_result(SrvOptSuccess);

	} while (false);
	
	if (rsp.result() != SrvOptSuccess) rsp.clear_game_room_info();

	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_HALL_ROOM_LIST_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get hall room list reply");
}

// 获取棋牌室当前排名列表
void CChessHallOperation::getHallRanking(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    com_protocol::ClientGetHallRankingReq req;
    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get hall ranking request")) return;
    
    com_protocol::ClientGetHallRankingRsp rsp;
    rsp.set_result(getCurrentHallRanking(req.hall_id().c_str(), *rsp.mutable_ranking()));
    
    if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_HALL_RANKING_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get hall ranking reply");
}

// 获取棋牌室用户状态信息
void CChessHallOperation::getHallPlayerStatus(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetHallPlayerStatusReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get hall player status request")) return;

    com_protocol::GetHallPlayerStatusRsp rsp;
	rsp.set_result(SrvOptSuccess);
	rsp.set_hall_id(req.hall_id());
    rsp.set_status(getChessHallPlayerStatus(req.hall_id().c_str(), m_msgHandler->getContext().userData));
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_HALL_PLAYER_STATUS_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get hall player status reply");
}

// 获取棋牌室游戏房间信息
void CChessHallOperation::getHallGameRoomInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetHallGameRoomInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get hall game room info request")) return;

    int rc = SrvOptFailed;
    com_protocol::GetHallGameRoomInfoRsp rsp;
    CQueryResult* pResult = NULL;

    do
	{
		CUserBaseinfo userBaseinfo;
		if (req.has_username())
		{
			// 获取DB中的用户信息
			rc = m_msgHandler->getUserBaseinfo(req.username(), req.hall_id(), userBaseinfo);
			if (rc != SrvOptSuccess)
			{
				rsp.set_result(rc);
				break;
			}
			
			m_msgHandler->setUserDetailInfo(userBaseinfo, *rsp.mutable_user_detail_info());
		}
		
		// select hall_id,username,room_type,base_rate,player_count,view_flag,service_tax_ratio,room_info from tb_user_room_info where room_id='xx';
		char sqlBuffer[512] = {0};
		const unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
		"select R.hall_id,R.username,R.room_type,R.base_rate,R.player_count,R.view_flag,G.service_tax_ratio,R.room_info "
		"from tb_user_room_info as R left join %s.tb_hall_game_info as G on R.game_type=G.game_type "
		"where R.room_id=\'%s\' and G.hall_id=\'%s\';",
		m_msgHandler->getSrvOpt().getDBCfg().opt_db_cfg.dbname.c_str(), req.room_id().c_str(), req.hall_id().c_str());
	
		if (SrvOptSuccess != m_msgHandler->getLogicDBOpt()->queryTableAllResult(sqlBuffer, sqlLen, pResult))
		{
			OptErrorLog("query hall game room info error, username = %s, sql = %s", m_msgHandler->getContext().userData, sqlBuffer);
		    rsp.set_result(DBProxyQueryHallGameRoomInfoError);
			break;
		}
		
		if (pResult == NULL || pResult->getRowCount() != 1)
		{
			rsp.set_result(DBProxyNoFoundGameRoomInfo);
			break;
		}

		// select blob data type, select hall_id,username,room_type,base_rate,player_count,view_flag,service_tax_ratio,room_info from tb_user_room_info
		RowDataPtr rowData = pResult->getNextRow();
		unsigned long* rowLen = pResult->getFieldLengthsOfCurrentRow();
		
		// service_tax_ratio field
		if (rowData[6] == NULL)
		{
			rsp.set_result(DBProxyNoFoundRoomGameInfo);
			break;
		}

		com_protocol::ChessHallRoomInfo* roomInfo = rsp.mutable_room_info();
		if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(*roomInfo, rowData[7], rowLen[7], "get hall game room info from db"))
		{
			rsp.clear_room_info();
			rsp.set_result(DBProxyParseRoomInfoError);
			break;
		}

        rsp.set_result(SrvOptSuccess);
		com_protocol::ChessHallRoomBaseInfo* baseInfo = roomInfo->mutable_base_info();
		baseInfo->set_room_id(req.room_id());
		baseInfo->set_hall_id(rowData[0]);
		baseInfo->set_username(rowData[1]);
		baseInfo->set_room_type((com_protocol::EHallRoomType)atoi(rowData[2]));
		baseInfo->set_base_rate(atoi(rowData[3]));
		baseInfo->set_player_count(atoi(rowData[4]));
		baseInfo->set_is_can_view(atoi(rowData[5]));
		baseInfo->set_service_tax_ratio(atof(rowData[6]));

		if (req.username() != baseInfo->username()) m_msgHandler->getUserBaseinfo(baseInfo->username(), baseInfo->hall_id(), userBaseinfo);
		baseInfo->set_portrait_id(userBaseinfo.static_info.portrait_id);

	} while (false);
	
	m_msgHandler->getLogicDBOpt()->releaseQueryResult(pResult);

	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_GAME_ROOM_INFO_RSP;
	rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get hall game room info reply");
	
	const com_protocol::ChessHallRoomBaseInfo& logBaseInfo = rsp.room_info().base_info();
	OptInfoLog("get hall game room info, username = %s, hall id = %s, room id = %s, creator = %s, game type = %u, room type = %u, result = %d, send msg rc = %d",
	m_msgHandler->getContext().userData, logBaseInfo.hall_id().c_str(), logBaseInfo.room_id().c_str(), logBaseInfo.username().c_str(),
	logBaseInfo.game_type(), logBaseInfo.room_type(), rsp.result(), rc);
}

