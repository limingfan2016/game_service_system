
/* author : limingfan
 * date : 2017.11.26
 * description : 消息处理辅助类
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
#include "CLogicHandler.h"


using namespace NProject;

// 数据记录日志文件
#define DBDataRecordLog(format, args...)   m_msgHandler->getSrvOpt().getDataLogger()->writeFile(NULL, 0, LogLevel::Info, format, ##args)


CDBLogicHandler::CDBLogicHandler()
{
	m_msgHandler = NULL;
}

CDBLogicHandler::~CDBLogicHandler()
{
	m_msgHandler = NULL;
}

int CDBLogicHandler::init(CMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_CHANGE_ITEM_REQ, (ProtocolHandler)&CDBLogicHandler::changeItem, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_CHANGE_MORE_USERS_ITEM_REQ, (ProtocolHandler)&CDBLogicHandler::changeMoreUsersItem, this);

    m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_INVITATION_LIST_REQ, (ProtocolHandler)&CDBLogicHandler::getInvitationList, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_ADD_LAST_PLAYERS_REQ, (ProtocolHandler)&CDBLogicHandler::setLastPlayers, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_LAST_PLAYER_LIST_REQ, (ProtocolHandler)&CDBLogicHandler::getLastPlayerList, this);
    m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_HALL_PLAYER_LIST_REQ, (ProtocolHandler)&CDBLogicHandler::getHallPlayerList, this);

    m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_GAME_RECORD_INFO_REQ, (ProtocolHandler)&CDBLogicHandler::getGameRecord, this);
    m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_DETAILED_GAME_RECORD_REQ, (ProtocolHandler)&CDBLogicHandler::getDetailedGameRecord, this);

	return SrvOptSuccess;
}

void CDBLogicHandler::unInit()
{
}

void CDBLogicHandler::updateConfig()
{
}

// 设置记录基本信息
void CDBLogicHandler::setRecordBaseInfo(com_protocol::ChangeRecordBaseInfo& recordInfo, unsigned int srcSrvId,
								        const string& hallId, const string& roomId, int roomRate, int optType,
										const RecordIDType recordId, const char* remark)
{
	// 玩家道具变更记录
	recordInfo.set_opt_type(optType);
	recordInfo.set_record_id(recordId);
	recordInfo.set_hall_id(hallId);
	recordInfo.set_game_type(srcSrvId / ServiceIdBaseValue);
	recordInfo.set_room_id(roomId);
	recordInfo.set_room_rate(roomRate);
	recordInfo.set_service_id(srcSrvId);
	recordInfo.set_remark(remark);
}

// 道具变更记录
int CDBLogicHandler::recordItemChange(const string& username, const google::protobuf::RepeatedPtrField<com_protocol::ItemChange>& goodsList,
									  const com_protocol::ChangeRecordBaseInfo& recordData, const com_protocol::ChangeRecordPlayerInfo& playerInfo)
{
	if (goodsList.size() < 1) return DBProxyNoGoodsForChangeError;

    char cardInfo[32] = {0};
	unsigned long cardInfoLen = 0;

	char sql[1024] = {0};
	unsigned int sqlLen = 0;
	CDBOpertaion* optDB = m_msgHandler->getLogicDBOpt();
	for (google::protobuf::RepeatedPtrField<com_protocol::ItemChange>::const_iterator itemIt = goodsList.begin();
	     itemIt != goodsList.end(); ++itemIt)
	{
		cardInfo[0] = '\0';
		cardInfoLen = playerInfo.card_info().length();
		if (cardInfoLen > 0 && !optDB->realEscapeString(cardInfo, playerInfo.card_info().c_str(), cardInfoLen))
		{
			m_msgHandler->mysqlRollback();  // 失败则回滚之前的insert数据记录
			
			OptErrorLog("add goods change format record info error, username = %s, hallId = %s, roomId = %s, recordId = %s,"
			"opt_type = %d, item_type = %u, item_count = %.2f, cur_count = %.2f", username.c_str(), recordData.hall_id().c_str(),
			recordData.room_id().c_str(), recordData.record_id().c_str(), recordData.opt_type(),
			itemIt->type(), itemIt->num(), itemIt->amount());

			return DBProxyAddGoodsFormatRecordInfoError;
		}
		
		sqlLen = snprintf(sql, sizeof(sql) - 1,
		"insert into tb_user_pay_income_record(username,record_id,create_time,hall_id,game_type,room_id,room_rate,room_flag,"
		"card_result,card_rate,card_info,player_flag,service_id,opt_type,item_type,item_count,cur_count,remark) "
        "values(\'%s\',\'%s\',now(),\'%s\',%u,\'%s\',%u,%u,%u,%u,\'%s\',%u,%u,%u,%u,%.2f,%.2f,\'%s\');", 
		username.c_str(), recordData.record_id().c_str(), recordData.hall_id().c_str(), recordData.game_type(),
		recordData.room_id().c_str(), recordData.room_rate(), recordData.room_flag(), playerInfo.card_result(), playerInfo.card_rate(),
        cardInfo, playerInfo.player_flag(), recordData.service_id(), recordData.opt_type(), itemIt->type(),
		itemIt->num(), itemIt->amount(), recordData.remark().c_str());

		if (SrvOptSuccess != optDB->executeSQL(sql, sqlLen))
		{
			m_msgHandler->mysqlRollback();  // 失败则回滚之前的insert数据记录
			
			OptErrorLog("insert item change record error, username = %s, sql = %s", username.c_str(), sql);
			
			return DBProxyAddGoodsChangeRecordError;
		}
	}
	
	return SrvOptSuccess;
}

void CDBLogicHandler::writeItemDataLog(const google::protobuf::RepeatedPtrField<com_protocol::ItemChange>& items,
                                       const string& srcName, const string& dstName,
									   int optType, int result, const char* info)
{
	for (google::protobuf::RepeatedPtrField<com_protocol::ItemChange>::const_iterator itemIt = items.begin(); itemIt != items.end(); ++itemIt)
	{
		DBDataRecordLog("%s|%s|%u|%.2f|%.2f|%d|%d|%s",
		info, srcName.c_str(), itemIt->type(), itemIt->num(), itemIt->amount(),
		optType, result, dstName.c_str());
	}
}

// 金币&道具&玩家属性等数量变更修改
int CDBLogicHandler::changeUserGoodsValue(const string& hall_id, const string& username, google::protobuf::RepeatedPtrField<com_protocol::ItemChange>& goodsList, CUserBaseinfo& userBaseinfo,
                                          const com_protocol::ChangeRecordBaseInfo& recordData, const com_protocol::ChangeRecordPlayerInfo& playerInfo)
{
	if (goodsList.size() < 1) return DBProxyNoGoodsForChangeError;
	
	int rc = m_msgHandler->getUserBaseinfo(username, hall_id, userBaseinfo);
	if (rc != SrvOptSuccess) return rc;

	/*  物品类型定义值
	EGoodsRMB = 1,				// 人民币
	EGoodsGold = 2,			    // 游戏金币
	EGoodsRoomCard = 3,		    // 游戏房卡
	EMaxGameGoodsType,
	*/

	double* type2value[] =
	{
		NULL,                                               // 保留
		NULL,                                               // 人民币
		&userBaseinfo.dynamic_info.game_gold,               // 金币
		&userBaseinfo.dynamic_info.room_card,               // 房卡
	};
	const unsigned int typeCount = sizeof(type2value) / sizeof(type2value[0]);

	double tmp = 0.00;
	for (google::protobuf::RepeatedPtrField<com_protocol::ItemChange>::iterator itemIt = goodsList.begin();
	     itemIt != goodsList.end(); ++itemIt)
	{
        if (itemIt->type() < typeCount)  // 基本的道具类型
		{
			double* value = type2value[itemIt->type()];
			if (value != NULL)
			{
				// 如果是扣税扣服务费用则仅仅是写流水记录
				if (recordData.opt_type() == com_protocol::EHallPlayerOpt::EOptTaxCost)
				{
					itemIt->set_amount(*value);
					continue;
				}

				*value = *value + itemIt->num();
				tmp = *value;
			}
			else
			{
				switch (itemIt->type())
				{
					/*
					case EGameGoodsType::EGoodsGold :
					{
						tmp = userBaseinfo.dynamic_info.game_gold + itemIt->num();
						userBaseinfo.dynamic_info.game_gold = tmp;  // 金币余额
						break;
					}
                    */

					default :
					{
						return DBProxyGoodsTypeInvalid;
						break;
					}
				}
			}
			
			if (tmp < 0.001) return DBProxyGoodsNotEnought;
		}
		
		// 其他道具类型
		else
		{
			return DBProxyGoodsTypeInvalid;
		}

        itemIt->set_amount(tmp);
	}

    const bool needWriteRecord = recordData.has_opt_type() && recordData.has_record_id() && recordData.has_hall_id();
	rc = needWriteRecord ? recordItemChange(username, goodsList, recordData, playerInfo) : SrvOptSuccess;
	if (rc != SrvOptSuccess) return rc;

    // 如果是扣税扣服务费用则仅仅是写流水记录
	if (recordData.opt_type() != com_protocol::EHallPlayerOpt::EOptTaxCost)
	{
		if (!m_msgHandler->setUserBaseinfoToMemcached(userBaseinfo))  // 将数据同步到memcached
		{
			if (needWriteRecord) m_msgHandler->mysqlRollback();

			return DBProxySetDataToMemcachedFailed;
		}
		
		m_msgHandler->updateOptStatus(username, EUpdateDBOpt::EModifyOpt);	// 有修改，更新状态
    }
	
	if (needWriteRecord) m_msgHandler->mysqlCommit();                // 提交变更记录

	return SrvOptSuccess;
}

int CDBLogicHandler::getInvitationPlayerInfo(const char* hallId, const char* username, const char* sql, unsigned int sqlLen, const unsigned int invitationCount,
                                             google::protobuf::RepeatedPtrField<com_protocol::InvitationInfo>& invitations, int& currentIdx)
{
	// sql 语句必须为：
	// select idx,username,online_hall_service_id,online_game_service_id,online_room_id,online_seat_id from tb_chess_hall_user_info
	CQueryResult* p_result = NULL;
	CDBOpertaion* pLogicDB = m_msgHandler->getLogicDBOpt();
	if (SrvOptSuccess != pLogicDB->queryTableAllResult(sql, sqlLen, p_result))
	{
		OptErrorLog("query invitation player info error, sql = %s", sql);

		return DBProxyQueryInvitationPlayerError;
	}

	currentIdx = -1; // 值为 -1 表示列表已经结束
	if (p_result == NULL || p_result->getRowCount() < 1)
	{
		pLogicDB->releaseQueryResult(p_result);

		return SrvOptSuccess;
	}
	
	// 游戏房间状态（准备中、游戏中）
	typedef vector<com_protocol::InvitationInfo*> InvitationInfoVector;
	typedef unordered_map<string, InvitationInfoVector> RoomIdToInvitationInfo;

	char sqlBuffer[102400] = {0};
	sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select room_id,room_status from tb_user_room_info where hall_id=\'%s\' and (room_id='0'", hallId);

	int tmpIdx = -1;
	const bool isGetAllInfo = (p_result->getRowCount() < invitationCount);
	
	RowDataPtr rowData = NULL;
	CUserBaseinfo userBaseinfo;
	RoomIdToInvitationInfo roomId2InvitationInfo;
	unsigned int gameSrvId = 0;
	while ((rowData = p_result->getNextRow()) != NULL)
	{
		// 获取玩家数据
		if (strcmp(username, rowData[1]) == 0  // 过滤掉玩家自己
		    || m_msgHandler->getUserBaseinfo(rowData[1], hallId, userBaseinfo) != SrvOptSuccess) continue;

		// select idx,username,online_hall_service_id,online_game_service_id,online_room_id,online_seat_id
		com_protocol::InvitationInfo* invitationInfo = invitations.Add();
		m_msgHandler->setUserDetailInfo(userBaseinfo, invitationInfo->mutable_static_info(), invitationInfo->mutable_dynamic_info());
		
		gameSrvId = atoi(rowData[3]);  // 游戏服务ID
		if (gameSrvId > 0)
		{
			invitationInfo->set_game_type(gameSrvId / ServiceIdBaseValue);
			
			if (atoi(rowData[5]) >= 0)  // 座位ID
			{
				invitationInfo->set_status(com_protocol::EPlayerGameStatus::EPrepareGame);  // 准备游戏中
				
				if (rowData[4][0] != '\0')  // 房间ID
				{
					// 需要查询玩家在房间的状态是否是游戏中
					roomId2InvitationInfo[rowData[4]].push_back(invitationInfo);
					sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, " or room_id=\'%s\'", rowData[4]);
				}
			}
			else
			{
				invitationInfo->set_status(com_protocol::EPlayerGameStatus::EViewGame);     // 观看游戏中
			}
		}
		else if (atoi(rowData[2]) > 0)
		{
			invitationInfo->set_status(com_protocol::EPlayerGameStatus::EOnlineGame);       // 在线
		}
		else
		{
			invitationInfo->set_status(com_protocol::EPlayerGameStatus::EOfflineGame);      // 离线
		}
		
		if (!isGetAllInfo)
		{
			tmpIdx = atoi(rowData[0]);
			if (tmpIdx > currentIdx) currentIdx = tmpIdx;  // 记录根据多个字段排序了，因此这里必须比较大小
		}
	}
	pLogicDB->releaseQueryResult(p_result);

	if (!roomId2InvitationInfo.empty())
	{
		// 查询玩家在房间的状态是否是游戏中
		sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, ");");
		p_result = NULL;
		if (SrvOptSuccess != pLogicDB->queryTableAllResult(sqlBuffer, sqlLen, p_result))
		{
			OptErrorLog("query invitation player room error, sql = %s", sqlBuffer);

			return DBProxyQueryInvitationRoomInfoError;
		}

		if (p_result != NULL && p_result->getRowCount() > 0)
		{
			while ((rowData = p_result->getNextRow()) != NULL)
			{
				// room_id,room_status
				if (atoi(rowData[1]) == com_protocol::EHallRoomStatus::EHallRoomPlay)
				{
					const InvitationInfoVector& invInfo = roomId2InvitationInfo[rowData[0]];
					for (InvitationInfoVector::const_iterator it = invInfo.begin(); it != invInfo.end(); ++it)
					{
						(*it)->set_status(com_protocol::EPlayerGameStatus::EPlayGame);  // 正在游戏中
					}
				}
			}
		}
		pLogicDB->releaseQueryResult(p_result);
	}

	return SrvOptSuccess;
}


void CDBLogicHandler::changeItem(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ItemChangeReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "change item request")) return;
	
	CUserBaseinfo userBaseinfo;
	int rc = changeUserGoodsValue(req.hall_id(), req.src_username(), *req.mutable_items(), userBaseinfo, req.base_info(), req.player_info());
	
	// 写数据日志
	if (req.write_data_log() == 1)
	{
		writeItemDataLog(req.items(), req.src_username(), req.dst_username(), req.base_info().opt_type(), rc, "CHANGE ITEM");
	}
	
	com_protocol::ItemChangeRsp rsp;
	rsp.set_result(rc);
	if (req.has_base_info()) rsp.set_allocated_base_info(req.release_base_info());
    if (req.has_player_info()) rsp.set_allocated_player_info(req.release_player_info());
	if (req.has_src_username()) rsp.set_allocated_src_username(req.release_src_username());
	if (req.has_dst_username()) rsp.set_allocated_dst_username(req.release_dst_username());
	if (req.items_size() > 0) *rsp.mutable_items() = req.items();
	if (req.has_cb_data()) rsp.set_allocated_cb_data(req.release_cb_data());
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_CHANGE_ITEM_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "change item reply");
}

void CDBLogicHandler::changeMoreUsersItem(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::MoreUsersItemChangeReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "change item request")) return;
	
	unsigned int failedCount = 0;
	com_protocol::MoreUsersItemChangeRsp rsp;
	CUserBaseinfo userBaseinfo;
	int rc = SrvOptFailed;
	for (google::protobuf::RepeatedPtrField<com_protocol::ItemChangeReq>::iterator changeReqIt = req.mutable_item_change_req()->begin();
	     changeReqIt != req.mutable_item_change_req()->end(); ++changeReqIt)
	{
		const com_protocol::ChangeRecordBaseInfo& recordInfo = changeReqIt->has_base_info() ? changeReqIt->base_info() : req.base_info();
	    rc = changeUserGoodsValue(req.hall_id(), changeReqIt->src_username(), *changeReqIt->mutable_items(), userBaseinfo, recordInfo, changeReqIt->player_info());
		
		// 写数据日志
		if (changeReqIt->write_data_log() == 1)
		{
			writeItemDataLog(changeReqIt->items(), changeReqIt->src_username(), changeReqIt->dst_username(), recordInfo.opt_type(), rc, "CHANGE MORE ITEM");
		}

		com_protocol::ItemChangeRsp* itemChangeRsp = rsp.add_item_change_rsp();
		itemChangeRsp->set_result(rc);
        if (changeReqIt->has_base_info()) itemChangeRsp->set_allocated_base_info(changeReqIt->release_base_info());
        if (changeReqIt->has_player_info()) itemChangeRsp->set_allocated_player_info(changeReqIt->release_player_info());
		if (changeReqIt->has_src_username()) itemChangeRsp->set_allocated_src_username(changeReqIt->release_src_username());
		if (changeReqIt->has_dst_username()) itemChangeRsp->set_allocated_dst_username(changeReqIt->release_dst_username());
		if (changeReqIt->items_size() > 0) *itemChangeRsp->mutable_items() = changeReqIt->items();
		
		if (rc != SrvOptSuccess) ++failedCount;
	}
	
	rsp.set_failed_count(failedCount);
	if (req.has_base_info()) rsp.set_allocated_base_info(req.release_base_info());
	if (req.has_cb_data()) rsp.set_allocated_cb_data(req.release_cb_data());

	if (srcProtocolId == 0) srcProtocolId = DBPROXY_CHANGE_MORE_USERS_ITEM_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "change more users item reply");
}


void CDBLogicHandler::getInvitationList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetInvitationListReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get invitation list request")) return;

	const unsigned int invitationCount = m_msgHandler->getSrvOpt().getServiceCommonCfg().game_cfg.get_invitation_count;
	const char* hallId = req.hall_id().c_str();
	const unsigned int currentIdx = (req.has_cursor_info()) ? req.cursor_info().current_idx() : 0;
	
	char sqlBuffer[10240] = {0};
	unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select idx,username,online_hall_service_id,online_game_service_id,online_room_id,online_seat_id "
	"from tb_chess_hall_user_info where hall_id=\'%s\' and user_status=%u and idx > %u "
	"order by online_hall_service_id desc,online_game_service_id asc,online_seat_id asc,game_gold desc limit %u;",
	hallId, com_protocol::EHallPlayerStatus::ECheckAgreedPlayer, currentIdx, invitationCount);
	
	com_protocol::ClientGetInvitationListRsp rsp;
	int newIdx = -1;
	int rc = getInvitationPlayerInfo(hallId, m_msgHandler->getContext().userData, sqlBuffer, sqlLen, invitationCount, *rsp.mutable_invitation_info(), newIdx);
	rsp.set_result(rc);
	if (rc == SrvOptSuccess)
	{
		rsp.set_allocated_cursor_info(req.release_cursor_info());
		rsp.mutable_cursor_info()->set_current_idx(newIdx);
	}
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_INVITATION_LIST_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get invitation list reply");
}

void CDBLogicHandler::setLastPlayers(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetLastPlayersNotify ntf;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(ntf, data, len, "set last player notify")
	    || ntf.username_size() <= 1) return;
	
	const unsigned int maxLastPlayerCount = m_msgHandler->getSrvOpt().getServiceCommonCfg().game_cfg.last_player_count;
	LastPlayerList& hallPlayerList = m_hallLastPlayers[ntf.hall_id()];
	const google::protobuf::RepeatedPtrField<string>& usernames = ntf.username();
	for (google::protobuf::RepeatedPtrField<string>::const_iterator usrIt = usernames.begin(); usrIt != usernames.end(); ++usrIt)
	{
		StringInList& playerList = hallPlayerList[*usrIt];
		for (google::protobuf::RepeatedPtrField<string>::const_iterator playerIt = usernames.begin(); playerIt != usernames.end(); ++playerIt)
		{
			if (*usrIt == *playerIt) continue;
			
			StringInList::iterator fdPlIt = std::find(playerList.begin(), playerList.end(), *playerIt);
			if (fdPlIt != playerList.end()) playerList.erase(fdPlIt);
            playerList.push_front(*playerIt);  // 最近的排在最前面
			
			if (playerList.size() > maxLastPlayerCount) playerList.pop_back();
		}
	}
}

void CDBLogicHandler::getLastPlayerList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetLastPlayerListReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get last player list request")) return;

    com_protocol::ClientGetLastPlayerListRsp rsp;
	rsp.set_result(SrvOptSuccess);

	const StringInList& playerList = m_hallLastPlayers[req.hall_id()][req.username()];
	if (!playerList.empty())
	{
		const char* hallId = req.hall_id().c_str();
		StringInList::const_iterator plIt = playerList.begin();
		char sqlBuffer[102400] = {0};
		unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
		"select idx,username,online_hall_service_id,online_game_service_id,online_room_id,online_seat_id "
		"from tb_chess_hall_user_info where hall_id=\'%s\' and (username=\'%s\'",
		hallId, plIt->c_str());
		
		while (++plIt != playerList.end())
		{
			sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, " or username=\'%s\'", plIt->c_str());
		}
		sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, ");");

		int newIdx = -1;
		int rc = getInvitationPlayerInfo(hallId, "", sqlBuffer, sqlLen,
										 (m_msgHandler->getSrvOpt().getServiceCommonCfg().game_cfg.last_player_count + 1),
										 *rsp.mutable_last_players(), newIdx);
		rsp.set_result(rc);
	}

	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_LAST_PLAYER_LIST_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get last player list reply");
}

void CDBLogicHandler::getHallPlayerList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetHallPlayerInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get hall player list request")) return;

	const unsigned int playerCount = m_msgHandler->getSrvOpt().getServiceCommonCfg().chess_hall_cfg.get_hall_player_count;
	const char* hallId = req.hall_id().c_str();
	const unsigned int currentIdx = (req.has_current_idx()) ? req.current_idx() : 0;
	
	char sqlBuffer[10240] = {0};
	unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
	"select idx,username,online_hall_service_id,online_game_service_id,online_room_id,online_seat_id "
	"from tb_chess_hall_user_info where hall_id=\'%s\' and user_status=%u and idx > %u "
	"order by online_seat_id desc,online_game_service_id desc,online_hall_service_id desc limit %u;",
	hallId, com_protocol::EHallPlayerStatus::ECheckAgreedPlayer, currentIdx, playerCount);
	
	int newIdx = -1;
	google::protobuf::RepeatedPtrField<com_protocol::InvitationInfo> playerList;
	int rc = getInvitationPlayerInfo(hallId, "", sqlBuffer, sqlLen, playerCount, playerList, newIdx);
	
	com_protocol::ClientGetHallPlayerInfoRsp rsp;
	rsp.set_result(rc);
	if (rc == SrvOptSuccess)
	{
		rsp.set_current_idx(newIdx);
		
		for (google::protobuf::RepeatedPtrField<com_protocol::InvitationInfo>::iterator it = playerList.begin();
		     it != playerList.end(); ++it)
	    {
		    com_protocol::HallPlayerInfo* playerInfo = rsp.add_hall_player_info();
			playerInfo->set_allocated_static_info(it->release_static_info());
			playerInfo->set_allocated_dynamic_info(it->release_dynamic_info());
			playerInfo->set_status(it->status());
			playerInfo->set_game_type(it->game_type());
	    }
	}
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_HALL_PLAYER_LIST_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get hall player list reply");
}

void CDBLogicHandler::getGameRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    com_protocol::ClientGetGameRecordReq req;
    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get game record request")) return;

    com_protocol::ClientGetGameRecordRsp rsp;
    rsp.set_game_type(req.game_type());

    CUserBaseinfo userBaseinfo;
    int rc = m_msgHandler->getUserBaseinfo(m_msgHandler->getContext().userData, req.hall_id(), userBaseinfo);
    if (rc == SrvOptSuccess)
    {
        char sqlBuffer[1024] = {0};
        unsigned int sqlLen = 0;
        CDBOpertaion* pLogicDB = m_msgHandler->getLogicDBOpt();
        CQueryResult* pResult = NULL;
        RowDataPtr rowData = NULL;
        for (unsigned int idx = com_protocol::ECattleBankerType::ECattleBanker; idx <= com_protocol::ECattleBankerType::EOpenCardBanker; ++idx)
        {
            sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
            "select count(*),sum(item_count),max(UNIX_TIMESTAMP(create_time)) from tb_user_pay_income_record "
            "where hall_id=\'%s\' and username=\'%s\' and game_type=%u and opt_type=%d and room_flag=%u;",
            req.hall_id().c_str(), m_msgHandler->getContext().userData, req.game_type(), com_protocol::EHallPlayerOpt::EOptProfitLoss, idx);
            
            pResult = NULL;
            if (SrvOptSuccess != pLogicDB->queryTableAllResult(sqlBuffer, sqlLen, pResult))
            {
                OptErrorLog("query game record info error, sql = %s", sqlBuffer);

                rsp.set_result(DBProxyQueryGameRecordInfoError);
                rsp.clear_banker_type_record();

                break;
            }
            
            if (pResult != NULL)
            {
                rowData = pResult->getNextRow();
                if (rowData != NULL && rowData[0] != NULL && atoi(rowData[0]) > 0)
                {
                    com_protocol::BankerTypeRecordInfo* recordInfo = rsp.add_banker_type_record();
                    recordInfo->set_game_times(atoi(rowData[0]));
                    recordInfo->set_sum_win_lose(atof(rowData[1]));
                    recordInfo->set_time_secs(atoi(rowData[2]));
                    recordInfo->set_banker_type(idx);
                }
                pLogicDB->releaseQueryResult(pResult);
            }
        }
        
        rsp.set_result(SrvOptSuccess);
        m_msgHandler->setUserDetailInfo(userBaseinfo, *rsp.mutable_detail_info());
    }
    else
    {
        rsp.set_result(rc);
    }
    
    if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_GAME_RECORD_INFO_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get game record reply");
}

void CDBLogicHandler::getDetailedGameRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    com_protocol::ClientGetDetailedGameRecordReq req;
    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get detailed game record request")) return;
    
    com_protocol::ClientGetDetailedGameRecordRsp rsp;
    do
    {
        const char* username = m_msgHandler->getContext().userData;
        CUserBaseinfo userBaseinfo;
        int rc = m_msgHandler->getUserBaseinfo(username, req.hall_id(), userBaseinfo);
        if (rc != SrvOptSuccess)
        {
            rsp.set_result(rc);
            break;
        }
    
        const int getRecordCount = m_msgHandler->getSrvOpt().getServiceCommonCfg().chess_hall_cfg.get_game_record_count;
        unsigned int currentIdx = (req.current_idx() < 1) ? 100000000 : req.current_idx();
        char sqlBuffer[102400] = {0};
        unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
        "select idx,record_id,UNIX_TIMESTAMP(create_time),cur_count,item_count,card_info,card_result,card_rate,player_flag from tb_user_pay_income_record "
        "where hall_id=\'%s\' and username=\'%s\' and game_type=%u and opt_type=%d and room_flag=%u and idx < %u order by idx desc limit %u;",
        req.hall_id().c_str(), username, req.game_type(), com_protocol::EHallPlayerOpt::EOptProfitLoss, req.banker_type(), currentIdx, getRecordCount);
    
        CDBOpertaion* pLogicDB = m_msgHandler->getLogicDBOpt();
        CQueryResult* pResult = NULL;
        if (SrvOptSuccess != pLogicDB->queryTableAllResult(sqlBuffer, sqlLen, pResult))
        {
            OptErrorLog("query detailed game record error, sql = %s", sqlBuffer);

            rsp.set_result(DBProxyQueryDetailedGameRecordError);
            break;
        }
        
        if (pResult == NULL || pResult->getRowCount() < 1)
        {
            pLogicDB->releaseQueryResult(pResult);
            
            // 没有记录
            rsp.set_current_idx(-1);
            rsp.set_result(SrvOptSuccess);

            break;
        }
        
        typedef unordered_map<string, com_protocol::GameRecordInfo*> RecordInfoMap;
        
        int rowCount = pResult->getRowCount();
        const bool isGetAllInfo = (rowCount < getRecordCount);
        if (isGetAllInfo) currentIdx = -1;

        RowDataPtr rowData = NULL;
        RecordInfoMap recordId2Info;
        while ((rowData = pResult->getNextRow()) != NULL)
	    {
            // select idx,record_id,UNIX_TIMESTAMP(create_time),cur_count,item_count,card_info,card_result,card_rate,player_flag
            com_protocol::GameRecordInfo* recordInfo = rsp.add_game_record();
            recordInfo->set_time_secs(atoi(rowData[2]));
            recordInfo->set_current_gold(atof(rowData[3]));
            
            // 玩家信息
            com_protocol::GameRecordPlayerInfo* playerInfo = recordInfo->add_record_info();
            // playerInfo->set_username(username);
            playerInfo->set_nickname(userBaseinfo.static_info.nickname);
            playerInfo->set_win_lose(atof(rowData[4]));
            playerInfo->set_pokers(rowData[5]);
            playerInfo->set_card_result(atoi(rowData[6]));
            playerInfo->set_card_rate(atoi(rowData[7]));
            playerInfo->set_player_flag(atoi(rowData[8]));
            
            recordId2Info[rowData[1]] = recordInfo;
            
            if (!isGetAllInfo && --rowCount < 1) currentIdx = atoi(rowData[0]);  // 最后一条记录了
        }
        pLogicDB->releaseQueryResult(pResult);
        
        if (!recordId2Info.empty())
        {
            RecordInfoMap::const_iterator it = recordId2Info.begin();
            sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
            "select USB.nickname,UPIR.username,UPIR.record_id,UPIR.item_count,UPIR.card_info,UPIR.card_result,UPIR.card_rate,UPIR.player_flag "
            "from tb_user_pay_income_record as UPIR left join tb_user_static_baseinfo as USB on UPIR.username=USB.username where "
            "hall_id=\'%s\' and UPIR.username!=\'%s\' and game_type=%u and opt_type=%d and room_flag=%u and (UPIR.record_id=\'%s\'",
            req.hall_id().c_str(), username, req.game_type(), com_protocol::EHallPlayerOpt::EOptProfitLoss, req.banker_type(), it->first.c_str());
            
            while (++it != recordId2Info.end())
            {
                sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, " or UPIR.record_id=\'%s\'", it->first.c_str());
            }
            sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, ");");
            
            pResult = NULL;
            if (SrvOptSuccess != pLogicDB->queryTableAllResult(sqlBuffer, sqlLen, pResult))
            {
                OptErrorLog("query detailed game record player info error, sql = %s", sqlBuffer);

                rsp.set_result(DBProxyQueryGameRecordPlayerInfoError);
                break;
            }
            
            if (pResult == NULL || pResult->getRowCount() < 1)
            {
                pLogicDB->releaseQueryResult(pResult);
                rsp.set_result(DBProxyGetGameRecordPlayerInfoError);
                break;
            }
            
            while ((rowData = pResult->getNextRow()) != NULL)
            {
                // select USB.nickname,UPIR.username,UPIR.record_id,UPIR.item_count,UPIR.card_info,UPIR.card_result,UPIR.card_rate,UPIR.player_flag
                if (rowData[0] != NULL)
                {
                    RecordInfoMap::const_iterator fdIt = recordId2Info.find(rowData[2]);
                    if (fdIt != recordId2Info.end())
                    {
                        // 玩家信息
                        com_protocol::GameRecordPlayerInfo* playerInfo = fdIt->second->add_record_info();
                        playerInfo->set_nickname(rowData[0]);
                        playerInfo->set_username(rowData[1]);
                        playerInfo->set_win_lose(atof(rowData[3]));
                        playerInfo->set_pokers(rowData[4]);
                        playerInfo->set_card_result(atoi(rowData[5]));
                        playerInfo->set_card_rate(atoi(rowData[6]));
                        playerInfo->set_player_flag(atoi(rowData[7]));
                    }
                }
            }
            pLogicDB->releaseQueryResult(pResult);
        }
        
        rsp.set_current_idx(currentIdx);
        rsp.set_result(SrvOptSuccess);

    } while (false);
    
    if (rsp.result() == SrvOptSuccess)
    {
        rsp.set_game_type(req.game_type());
        rsp.set_banker_type(req.banker_type());
    }
        
    if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_DETAILED_GAME_RECORD_RSP;
    m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get detailed game record reply");
}
