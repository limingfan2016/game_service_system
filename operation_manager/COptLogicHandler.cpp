
/* author : limingfan
 * date : 2017.10.20
 * description : 消息处理分模块分流
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

#include "base/Function.h"
#include "COptMsgHandler.h"
#include "COptLogicHandler.h"


// 数据记录日志
#define WriteDataLog(format, args...)     m_msgHandler->getSrvOpt().getDataLogger()->writeFile(NULL, 0, LogLevel::Info, format, ##args)


COptLogicHandler::COptLogicHandler()
{
	m_msgHandler = NULL;
}

COptLogicHandler::~COptLogicHandler()
{
	m_msgHandler = NULL;
}

bool COptLogicHandler::init(COptMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_CHECK_PHONE_NUMBER_REQ, (ProtocolHandler)&COptLogicHandler::checkManagerPhoneNumber, this);
    m_msgHandler->registerProtocol(ServiceType::HttpOperationSrv, HTTPOPT_CHECK_PHONE_NUMBER_RSP, (ProtocolHandler)&COptLogicHandler::checkManagerPhoneNumberReply, this);

    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_CREATE_CHESS_HALL_REQ, (ProtocolHandler)&COptLogicHandler::createChessHall, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_CREATE_HALL_RECHARGE_INFO_RSP, (ProtocolHandler)&COptLogicHandler::createChessHallRechargeInfoReply, this);

	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_GET_HALL_INFO_REQ, (ProtocolHandler)&COptLogicHandler::getChessHallInfo, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_GET_PLAYER_INFO_REQ, (ProtocolHandler)&COptLogicHandler::getPlayerInfo, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_GET_HALL_GAME_INFO_REQ, (ProtocolHandler)&COptLogicHandler::getGameInfo, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_ADD_HALL_NEW_GAME_REQ, (ProtocolHandler)&COptLogicHandler::optHallGame, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_SET_HALL_GAME_TAX_RATIO_REQ, (ProtocolHandler)&COptLogicHandler::setHallGameTaxRatio, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_OPT_HALL_PLAYER_REQ, (ProtocolHandler)&COptLogicHandler::optPlayer, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_MANAGER_OPT_PLAYER_INFO_RSP, (ProtocolHandler)&COptLogicHandler::optPlayerReply, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_ADD_MANAGER_REQ, (ProtocolHandler)&COptLogicHandler::addManager, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_PLAYER_QUIT_CHESS_HALL_NOTIFY, (ProtocolHandler)&COptLogicHandler::notifyPlayerQuitChessHall, this);

    // 商城操作
    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_GET_MALL_DATA_REQ, (ProtocolHandler)&COptLogicHandler::getMallData, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_GET_MALL_BUY_INFO_REQ, (ProtocolHandler)&COptLogicHandler::getMallBuyInfo, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_GET_MALL_GIVE_INFO_REQ, (ProtocolHandler)&COptLogicHandler::getMallGiveInfo, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_SET_PLAYER_INIT_GOODS_REQ, (ProtocolHandler)&COptLogicHandler::setPlayerInitGoods, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_CONFIRM_BUY_INFO_REQ, (ProtocolHandler)&COptLogicHandler::confirmBuyInfo, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_CONFIRM_MALL_BUY_INFO_RSP, (ProtocolHandler)&COptLogicHandler::confirmBuyInfoReply, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_GIVE_GOODS_REQ, (ProtocolHandler)&COptLogicHandler::giveGoods, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_GIVE_MALL_GOODS_RSP, (ProtocolHandler)&COptLogicHandler::giveGoodsReply, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, OPTMGR_PLAYER_BUY_MALL_GOODS_NOTIFY, (ProtocolHandler)&COptLogicHandler::buyMallGoodsNotify, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, BSC_GET_GAME_PLAYER_DATA_REQ, (ProtocolHandler)&COptLogicHandler::getUserInfo, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, OPTMGR_PLAYER_APPLY_JOIN_HALL_NOTIFY, (ProtocolHandler)&COptLogicHandler::applyJoinChessHallNotify, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, OPTMGR_GET_APPLY_JOIN_PLAYERS_REQ, (ProtocolHandler)&COptLogicHandler::getApplyJoinPlayers, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, OPTMGR_OPT_HALL_PLAYER_REQ, (ProtocolHandler)&COptLogicHandler::managerOptHallPlayer, this);
	m_msgHandler->registerProtocol(ServiceType::OperationManageSrv, OPTMGR_CUSTOM_OPT_HALL_PLAYER_RESULT, (ProtocolHandler)&COptLogicHandler::managerOptHallPlayerReply, this);

    return true;
}

void COptLogicHandler::unInit()
{
}

void COptLogicHandler::updateConfig()
{
}

void COptLogicHandler::online(OptMgrUserData* cud)
{
	
}

void COptLogicHandler::offline(OptMgrUserData* cud)
{
	m_managerPhoneNumberData.erase(cud->userName);
}

void COptLogicHandler::getSelectHallPlayerInfoSQL(const com_protocol::GetBSHallPlayerInfoReq& req, char* sql, unsigned int& sqlLen)
{
	// select H.idx,U.username,U.nickname,U.portrait_id,H.create_time,H.user_remark,H.recharge,H.game_gold,H.profit_loss,H.tax_cost,H.exchange,
	// H.online_hall_service_id,H.online_game_service_id,H.offline_hall_secs from tb_chess_hall_user_info as H left join youju_logic_db.tb_user_static_baseinfo as U 
	// on H.username=U.username where H.user_status=x and H.hall_id='xx' and H.idx > y limit z;
	const char* logicDbName = m_msgHandler->getSrvOpt().getDBCfg().logic_db_cfg.dbname.c_str();
	unsigned int tmpSqlLen = snprintf(sql, sqlLen - 1,
	"select H.idx,U.username,U.nickname,U.portrait_id,UNIX_TIMESTAMP(H.create_time),H.user_remark,H.recharge,H.game_gold,H.profit_loss,H.tax_cost,"
	"H.exchange,H.online_hall_service_id,H.online_game_service_id,H.offline_hall_secs from %s.tb_chess_hall_user_info as H left join "
	"%s.tb_user_static_baseinfo as U on H.username=U.username where H.hall_id=\'%s\' and H.idx>%u",
	logicDbName, logicDbName, req.hall_id().c_str(), req.current_idx());

	const unsigned int status = req.status();
	if (status == com_protocol::EHallPlayerStatus::EOnlinePlayer)            // 在线玩家
	{
		tmpSqlLen += snprintf(sql + tmpSqlLen, sqlLen - tmpSqlLen - 1, " and H.user_status=%u and H.online_hall_service_id>%u",
		com_protocol::EHallPlayerStatus::ECheckAgreedPlayer, ServiceIdBaseValue);
	}
	else if (status == com_protocol::EHallPlayerStatus::ECheckAgreedPlayer)  // 离线玩家
	{
		tmpSqlLen += snprintf(sql + tmpSqlLen, sqlLen - tmpSqlLen - 1, " and H.user_status=%u and H.online_hall_service_id=0", status);
	}
	else
	{
		tmpSqlLen += snprintf(sql + tmpSqlLen, sqlLen - tmpSqlLen - 1, " and H.user_status=%u", status);
	}
	
	tmpSqlLen += snprintf(sql + tmpSqlLen, sqlLen - tmpSqlLen - 1, "  limit %u;", m_msgHandler->m_pCfg->common_cfg.get_player_count);
	
	sqlLen = tmpSqlLen;
}

void COptLogicHandler::getSelectHallNeedCheckUserInfoSQL(const com_protocol::GetBSHallPlayerInfoReq& req, char* sql, unsigned int& sqlLen)
{
	// select H.idx,U.username,U.nickname,U.portrait_id,H.create_time,H.explain_msg from tb_chess_hall_user_info as H left join 
	// tb_user_static_baseinfo as U on H.username=U.username where H.user_status=x and H.hall_id='xx' and H.idx > y limit z;
	const char* logicDbName = m_msgHandler->getSrvOpt().getDBCfg().logic_db_cfg.dbname.c_str();
	sqlLen = snprintf(sql, sqlLen - 1,
	"select H.idx,U.username,U.nickname,U.portrait_id,UNIX_TIMESTAMP(H.create_time),H.explain_msg from %s.tb_chess_hall_user_info as H left join "
	"%s.tb_user_static_baseinfo as U on H.username=U.username where H.user_status=%u and H.hall_id=\'%s\' and H.idx > %u limit %u;",
	logicDbName, logicDbName, req.status(), req.hall_id().c_str(),
	req.current_idx(), m_msgHandler->m_pCfg->common_cfg.get_player_count);
}

int COptLogicHandler::addNewHallGame(OptMgrUserData* cud, SChessHallData* hallData, unsigned int gameType)
{
	GameTypeToInfoMap::const_iterator hallGameIt = hallData->gameInfo.find(gameType);
	if (hallGameIt != hallData->gameInfo.end()) return SrvOptSuccess;

	const map<unsigned int, ServiceCommonConfig::GameInfo>& gameInfoCfg = m_msgHandler->getSrvOpt().getServiceCommonCfg().game_info_map;
	map<unsigned int, ServiceCommonConfig::GameInfo>::const_iterator gameCfgIt = gameInfoCfg.find(gameType);
	if (gameCfgIt != gameInfoCfg.end())
	{
		char tmpSql[256] = {0};
		const unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"insert into tb_hall_game_info(hall_id,create_time,game_type,service_tax_ratio,game_status) value(\'%s\',now(),%u,%.3f,%u);",
		cud->chessHallId, gameType, gameCfgIt->second.service_tax_ratio, com_protocol::EHallGameStatus::ENormalGame);
		
		if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->executeSQL(tmpSql, tmpSqlLen))
		{
			OptErrorLog("insert chess hall game info error, sql = %s", tmpSql);
			return BSAddNewGameError;
		}
		else if (1 != m_msgHandler->getOptDBOpt()->getAffectedRows())
		{
			OptErrorLog("insert chess hall game info failed, sql = %s", tmpSql);
			return BSAddNewGameError;
		}
		else if (!m_msgHandler->getHallGameInfo(cud->chessHallId, hallData->gameInfo))
		{
			OptErrorLog("add chess hall game info failed, manager = %s, hall id = %s, type = %d",
			cud->userName, cud->chessHallId, gameType);
			return BSAddNewGameError;
		}
	}
	else
	{
		OptErrorLog("add chess hall game type error, manager = %s, hall id = %s, type = %d",
		cud->userName, cud->chessHallId, gameType);
		return BSNoFoundGameInfo;
	}
	
	return SrvOptSuccess;
}

int COptLogicHandler::updateHallGameStatus(OptMgrUserData* cud, SChessHallData* hallData, unsigned int gameType, unsigned int status)
{
	char tmpSql[256] = {0};
	const unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"update tb_hall_game_info set game_status=%u where hall_id=\'%s\' and game_type=%u;", status, cud->chessHallId, gameType);
	
	if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->executeSQL(tmpSql, tmpSqlLen))
	{
		OptErrorLog("update game status error, sql = %s", tmpSql);
		return BSUpdateGameStatusError;
	}
	else if (1 != m_msgHandler->getOptDBOpt()->getAffectedRows())
	{
		OptErrorLog("update game status failed, sql = %s", tmpSql);
		return BSUpdateGameStatusError;
	}
	else if (!m_msgHandler->getHallGameInfo(cud->chessHallId, hallData->gameInfo))
	{
		OptErrorLog("update game status failed, manager = %s, hall id = %s, type = %d, status = %d",
		cud->userName, cud->chessHallId, gameType, status);
		return BSUpdateGameStatusError;
	}
	
	return SrvOptSuccess;
}

int COptLogicHandler::updateHallGameTaxRatio(OptMgrUserData* cud, SChessHallData* hallData, unsigned int gameType, float taxRatio)
{
	char tmpSql[256] = {0};
	const unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"update tb_hall_game_info set service_tax_ratio=%.3f where hall_id=\'%s\' and game_type=%u;", taxRatio, cud->chessHallId, gameType);
	
	if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->executeSQL(tmpSql, tmpSqlLen))
	{
		OptErrorLog("update game tax ratio error, sql = %s", tmpSql);
		return BSUpdateGameTaxRatioError;
	}
	else if (1 != m_msgHandler->getOptDBOpt()->getAffectedRows())
	{
		OptErrorLog("update game tax ratio failed, sql = %s", tmpSql);
		return BSUpdateGameTaxRatioError;
	}
	else if (!m_msgHandler->getHallGameInfo(cud->chessHallId, hallData->gameInfo))
	{
		OptErrorLog("update game tax ratio failed, manager = %s, hall id = %s, type = %d, tax ratio = %.3f",
		cud->userName, cud->chessHallId, gameType, taxRatio);
		return BSUpdateGameTaxRatioError;
	}
	
	return SrvOptSuccess;
}

// 通知C端玩家用户，在棋牌室的状态变更
void COptLogicHandler::notifyChessHallPlayerStatus(const char* hallId, const char* username, com_protocol::EHallPlayerStatus status)
{
	unsigned int dstServiceType = ServiceType::GameHallSrv;
	if (status != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer)
	{
		// 非审核通过的其他操作，通知的玩家必须在线
		unsigned int hallSrvId = 0;
		unsigned int gameSrvId = 0;
		if (!m_msgHandler->getOnlineServiceId(hallId, username, hallSrvId, gameSrvId) || hallSrvId <= ServiceIdBaseValue) return;

		if (gameSrvId > ServiceIdBaseValue) dstServiceType = gameSrvId / ServiceIdBaseValue;
	}
	
	com_protocol::ClientChessHallPlayerStatusNotify playerStatusNotify;
	playerStatusNotify.set_hall_id(hallId);
	playerStatusNotify.set_status(status);
	const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(playerStatusNotify, "player status notify");
	if (msgData != NULL)
	{
		m_msgHandler->getSrvOpt().forwardMessage(username, msgData, playerStatusNotify.ByteSize(), dstServiceType,
		OPTMGR_CHESS_HALL_PLAYER_STATUS_NOTIFY, m_msgHandler->getSrvId(), 0, "forward player status notify message");
	}
}

// 获取对应时间点商城操作的总数额
int COptLogicHandler::getTimeMallOptSumValue(const char* hall_id, int opt, time_t timeSecs, double& sumValue)
{
	if (hall_id == NULL || *hall_id == '\0') return BSChessHallIdParamError;

	// select sum(pay_count) from tb_mall_record_info where hall_id='xx' and opt_type=yy and create_time like '2017-10-18%';
	char tmpSql[512] = {0};
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 2,
	"select sum(pay_count) from %s.tb_mall_record_info where hall_id=\'%s\' and opt_type=%u",
	m_msgHandler->getSrvOpt().getDBCfg().logic_db_cfg.dbname.c_str(), hall_id, opt);
	
	if (timeSecs > 0)
	{
		struct tm* pTm = localtime(&timeSecs);
		tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 2,
	    " and create_time like \'%d-%02d-%02d%%\'", (pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday);
	}
	
	tmpSql[tmpSqlLen] = ';';
	tmpSql[tmpSqlLen + 1] = '\0';
	
	sumValue = 0.00;
	int rc = SrvOptSuccess;
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess == m_msgHandler->getOptDBOpt()->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
	{
		if (pResult != NULL && pResult->getRowCount() == 1)
		{
			RowDataPtr rowData = pResult->getNextRow();
			if (rowData[0] != NULL) sumValue = atof(rowData[0]);
		}

		m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);
	}
	else
	{
		rc = BSQueryMallOptSumValueError;
		OptErrorLog("query mall opt sum value error, sql = %s", tmpSql);
	}
	
	return rc;
}

// 获取商城操作信息
int COptLogicHandler::getMallOptInfo(const char* hallId, int optType, int optStatus1, int optStatus2, unsigned int afterIdx, unsigned int limitCount,
									 google::protobuf::RepeatedPtrField<com_protocol::MallOptInfoRecord>& optInfoList, int& currentIdx)
{
	if (hallId == NULL || *hallId == '\0') return BSChessHallIdParamError;

	// select M.idx,U.username,U.nickname,U.portrait_id,M.record_id,UNIX_TIMESTAMP(M.create_time),M.pay_count 
	// from tb_mall_record_info as M left join youju_logic_db.tb_user_static_baseinfo as U 
	// on M.username=U.username where M.opt_type=x and M.opt_status=x and M.hall_id='xx' and M.idx > y limit z;
	const char* logicDbName = m_msgHandler->getSrvOpt().getDBCfg().logic_db_cfg.dbname.c_str();
	char sql[1024] = {0};
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1,
	"select M.idx,U.username,U.nickname,U.portrait_id,M.record_id,UNIX_TIMESTAMP(M.create_time),M.pay_count "
	"from %s.tb_mall_record_info as M left join %s.tb_user_static_baseinfo as U "
	"on M.username=U.username where M.opt_type=%d and (M.opt_status=%d or M.opt_status=%d) and M.hall_id=\'%s\' and M.idx > %u limit %u;",
	logicDbName, logicDbName, optType, optStatus1, optStatus2, hallId, afterIdx, limitCount);

	CQueryResult* pResult = NULL;
	if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->queryTableAllResult(sql, sqlLen, pResult))
	{
		OptErrorLog("query mall info error, sql = %s", sql);

		return BSQueryMallInfoError;
	}
	
	currentIdx = -1;
	if (pResult == NULL || pResult->getRowCount() < 1)  // 没有数据
	{
		m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);
		
		return SrvOptSuccess;
	}
	
	enum ETableFieldName
	{
		EIdxField = 0,
		EUIdField = 1,
		EUNameField = 2,
		EUPortraitField = 3,
		EMRecordIdField = 4,
		EMCreateTimeField = 5,
		EMPayCountField = 6,
	};

	// select M.idx,U.username,U.nickname,U.portrait_id,M.record_id,UNIX_TIMESTAMP(M.create_time),M.pay_count
	RowDataPtr rowData = NULL;
	while ((rowData = pResult->getNextRow()) != NULL)
	{
		com_protocol::MallOptInfoRecord* optInfoRecord = optInfoList.Add();
		com_protocol::MallOptInfo* optInfo = optInfoRecord->mutable_opt_info();
		optInfo->set_record_id(rowData[EMRecordIdField]);
		optInfo->set_time_secs(atoi(rowData[EMCreateTimeField]));

		com_protocol::Goods* payGoods = optInfo->mutable_goods()->mutable_pay_goods();
		payGoods->set_price(1.00);
		payGoods->set_count(atof(rowData[EMPayCountField]));

		if (atoi(rowData[EIdxField]) > currentIdx) currentIdx = atoi(rowData[EIdxField]);

		if (rowData[EUIdField] != NULL)
		{
			com_protocol::StaticInfo* uStaticInfo = optInfoRecord->mutable_username_info();
			uStaticInfo->set_username(rowData[EUIdField]);
			uStaticInfo->set_nickname(rowData[EUNameField]);
			uStaticInfo->set_portrait_id(rowData[EUPortraitField]);
		}
		else
		{
			OptErrorLog("query mall info can not find the record username, record id = %s", rowData[EMRecordIdField]);
		}
	}
	
	m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);
	
	return SrvOptSuccess;
}

// 发送消息给当前所有在线的管理员
void COptLogicHandler::sendMessageToAllManager(const string& hallId, const google::protobuf::Message& msg, unsigned short protocolId, const char* logInfo)
{
	bool isSerializeMsg = false;
    char msgBuffer[256] = {0};
	unsigned int msgBufferLen = sizeof(msgBuffer);
	
	// 可能存在多个在线管理员，通知全部
	const IDToConnectProxys& userConnects = m_msgHandler->getConnectProxy();
	for (IDToConnectProxys::const_iterator onlineMgrIt = userConnects.begin(); onlineMgrIt != userConnects.end(); ++onlineMgrIt)
	{
		// 先找到在线的棋牌室管理员（效率）
		OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getProxyUserData(onlineMgrIt->second);
		if (cud != NULL && hallId == cud->chessHallId)
		{
			if (!isSerializeMsg)
			{
				if (NULL == m_msgHandler->getSrvOpt().serializeMsgToBuffer(msg, msgBuffer, msgBufferLen, logInfo)) return;
				
				isSerializeMsg = true;
			}
			
			int rc = m_msgHandler->sendMsgToProxy(msgBuffer, msgBufferLen, protocolId, onlineMgrIt->second);
			OptInfoLog("send message to mananger = %s, hallId = %s, protocolId = %d, rc = %d",
			cud->userName, cud->chessHallId, protocolId, rc);
		}
	}
}


void COptLogicHandler::checkManagerPhoneNumber(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();

    com_protocol::CheckBSPhoneNumberReq req;
    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "check manager phone number request")) return;
    
    // 收信人手机号，如1xxxxxxxxxx，格式必须为11位
    const unsigned int phoneNumberLen = 11;
    if (req.phone_number().length() != phoneNumberLen || *req.phone_number().c_str() != '1'
        || !NCommon::strIsDigit(req.phone_number().c_str(), req.phone_number().length()))
	{
		com_protocol::CheckBSPhoneNumberRsp rsp;
		rsp.set_result(BSInvalidMobilePhoneNumber);

		m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_CHECK_PHONE_NUMBER_RSP, "check manager phone number error reply");
	}

	// 此数据在用户创建棋牌室之后或者退出游戏时删除
	const unsigned int verificationCode = CRandomNumber::getUIntNumber(100001, 999999);  // 随机6位数字
	m_managerPhoneNumberData[cud->userName] = SPhoneNumberData(time(NULL), req.phone_number(), verificationCode);

    // 每个手机号码每分钟只能接收 1 条短信
	// 给用户手机发送验证码
	req.set_verification_code(verificationCode);
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::HttpOperationSrv, req,
												  HTTPOPT_CHECK_PHONE_NUMBER_REQ, "check manager phone number request");
}

void COptLogicHandler::checkManagerPhoneNumberReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const char* userName = NULL;
	ConnectProxy* conn = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "check manager phone number reply");
	if (conn != NULL) m_msgHandler->sendMsgToProxy(data, len, BSC_CHECK_PHONE_NUMBER_RSP, conn); 
}

void COptLogicHandler::createChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CreateBSChessHallReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "create BS chess hall request")) return;
	
	com_protocol::CreateBSChessHallRsp rsp;
	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	int rc = SrvOptFailed;
	do
	{
		if (cud->level != com_protocol::EManagerLevel::ESuperLevel)
		{
			rsp.set_result(BSCreateChessHallManagerLevelLimit);
			break;
		}
		
		if (cud->chessHallId[0] != '\0')
		{
			OptErrorLog("already exist chess hall, manager = %s, id = %s", cud->userName, cud->chessHallId);
			rsp.set_result(BSAlreadyExistChessHall);			
			break;
		}
		
		// 输入合法性判定
		if (req.name().length() >= 32 || req.name().length() < 3)
		{
			rsp.set_result(BSChessHallNameInputUnlegal);
			break;
		}

		if (req.has_desc() && (req.desc().length() >= 128 || req.desc().length() < 3))
		{
			rsp.set_result(BSChessHallDescInputUnlegal);
			break;
		}
		
		ManagerToPhoneNumberDataMap::iterator phoneNumberIt = m_managerPhoneNumberData.find(cud->userName);
		if (phoneNumberIt == m_managerPhoneNumberData.end() || phoneNumberIt->second.code != req.phone_verification_code())
		{
			rsp.set_result(BSInvalidPhoneVerificationCode);
			break;
		}

		// 获取棋牌室不重复的随机ID
	    const unsigned int chessHallID = m_msgHandler->getChessHallNoRepeatRandID();
		char tmpSql[1024] = {0};
	    unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"insert into tb_chess_card_hall(hall_id,hall_name,create_time,creator,contacts_name,contacts_mobile_phone,description,status) "
		"value(\'%u\',\'%s\',now(),\'%s\',\'%s\',\'%s\',\'%s\',%u);",
		chessHallID, req.name().c_str(), cud->userName, req.manager_name().c_str(), phoneNumberIt->second.number.c_str(), req.desc().c_str(),
		com_protocol::EChessHallStatus::EOpenHall);
			 
		// 插入表数据
		if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->executeSQL(tmpSql, tmpSqlLen))
		{
			OptErrorLog("add chess hall error, exec sql = %s", tmpSql);
			rsp.set_result(BSAddChessHallError);
			break;
		}
		
		// 添加棋牌室ID到管理员记录
	    tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"insert into tb_chess_hall_manager_dynamic_info(create_time,hall_id,name,level,status) value(now(),\'%u\',\'%s\',%u,%u);",
		chessHallID, cud->userName, com_protocol::EManagerLevel::ESuperLevel, com_protocol::EManagerStatus::EMgrNormal);
		if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->executeSQL(tmpSql, tmpSqlLen)
		    || (unsigned long)-1 == m_msgHandler->getOptDBOpt()->getAffectedRows())
		{
			OptErrorLog("add manager dynamic info error, affected rows = %lu, sql = %s",
			m_msgHandler->getOptDBOpt()->getAffectedRows(), tmpSql);
			rsp.set_result(BSUpdateChessHallIDError);
			
			// 失败了则删除新添加的棋牌室信息
			tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1, "delete from tb_chess_card_hall where hall_id=\'%u\';", chessHallID);
			if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->executeSQL(tmpSql, tmpSqlLen))
			{
				OptErrorLog("delete chess hall from update id failed error, exec sql = %s", tmpSql);
			}
			
			break;
		}
		
		// 填写棋牌室ID
		snprintf(cud->chessHallId, sizeof(cud->chessHallId) - 1, "%u", chessHallID);

		// 创建棋牌室充值信息
		com_protocol::CreateHallRechargeInfoReq createHallRechargeInfoReq;
		createHallRechargeInfoReq.set_hall_id(cud->chessHallId);
		createHallRechargeInfoReq.set_current_gold(0.00);
		createHallRechargeInfoReq.set_current_room_card(0.00);

        rc = m_msgHandler->getSrvOpt().sendPkgMsgToDbProxy(createHallRechargeInfoReq, DBPROXY_CREATE_HALL_RECHARGE_INFO_REQ, cud->userName, cud->userNameLen, "create chess hall recharge info request");
		
		OptInfoLog("send create chess hall recharge info request to db proxy, rc = %d, username = %s, hall id = %s, name = %s",
		rc, cud->userName, cud->chessHallId, req.name().c_str());
		
		return;

	} while (false);
	
	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_CREATE_CHESS_HALL_RSP, "create BS chess hall error reply");

	OptErrorLog("create chess hall error, manager = %s, hall id = %s, name = %s, result = %d, send msg rc = %d",
	cud->userName, cud->chessHallId, req.name().c_str(), rsp.result(), rc);
}

void COptLogicHandler::createChessHallRechargeInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CreateHallRechargeInfoRsp createHallRechargeInfoRsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(createHallRechargeInfoRsp, data, len, "create chess hall recharge info reply")) return;

	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "create chess hall recharge info reply");
	if (connProxy == NULL) return;  // 离线了
	
	int rc = SrvOptFailed;
	com_protocol::CreateBSChessHallRsp rsp;
	rsp.set_result(createHallRechargeInfoRsp.result());
	
	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getProxyUserData(connProxy);
	if (rsp.result() == SrvOptSuccess)
	{
		SChessHallData* hallData = m_msgHandler->getChessHallData(cud->chessHallId, rc, true);
		if (hallData != NULL) rsp.set_allocated_chess_hall(&hallData->baseInfo);
		else rsp.set_result(rc);
	}
	else
	{
		// 失败了则删除新添加的棋牌室信息
		char tmpSql[1024] = {0};
		const unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"delete from tb_chess_card_hall where hall_id=\'%s\';"
		"delete from tb_chess_hall_manager_dynamic_info where hall_id=\'%s\' and name=\'%s\';",
		createHallRechargeInfoRsp.hall_id().c_str(), createHallRechargeInfoRsp.hall_id().c_str(), userName);

		CQueryResult* qResult = NULL;
		if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->executeMultiSql(tmpSql, tmpSqlLen, qResult))
		{
			OptErrorLog("delete chess hall from create recharge info failed error, sql = %s", tmpSql);
		}
		if (qResult != NULL) m_msgHandler->getOptDBOpt()->releaseQueryResult(qResult);

		*cud->chessHallId = '\0';
	}
	
	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_CREATE_CHESS_HALL_RSP, connProxy, "create BS chess hall reply");
	
	WriteDataLog("create chess hall|%s|%s|%s|%d|%d", cud->userName, cud->chessHallId, rsp.chess_hall().name().c_str(),
	rsp.result(), rc);
	
	OptInfoLog("create chess hall, manager = %s, chess hall id = %s, name = %s, result = %d, send msg rc = %d",
	cud->userName, cud->chessHallId, rsp.chess_hall().name().c_str(), rsp.result(), rc);
	
	if (rsp.result() == SrvOptSuccess) rsp.release_chess_hall();
}

void COptLogicHandler::getChessHallInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetBSChessHallInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get BS chess hall info request")) return;
	
	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	com_protocol::GetBSChessHallInfoRsp rsp;
	int rc = SrvOptFailed;
	SChessHallData* hallData = m_msgHandler->getChessHallData(cud->chessHallId, rc);
	if (hallData != NULL)
	{
		rsp.set_result(SrvOptSuccess);
		rsp.set_allocated_chess_hall(&hallData->baseInfo);
	}
	else
	{
		rsp.set_result(rc);
	}
	
	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_GET_HALL_INFO_RSP, "get BS chess hall info reply");
	
	if (rsp.result() == SrvOptSuccess) rsp.release_chess_hall();
	
	OptInfoLog("get chess hall info, manager = %s, id = %s, result = %d, send msg rc = %d",
	cud->userName, cud->chessHallId, rsp.result(), rc);
}

void COptLogicHandler::getPlayerInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetBSHallPlayerInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get BS player info request")) return;
	
	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (req.hall_id() != cud->chessHallId)
	{
		OptErrorLog("get player info, hall id error, id = %s, param = %s", cud->chessHallId, req.hall_id().c_str());
		return;
	}

    com_protocol::GetBSHallPlayerInfoRsp rsp;
	rsp.set_result(SrvOptSuccess);
	int rc = SrvOptFailed;
	do
	{
		SChessHallData* hallData = m_msgHandler->getChessHallData(cud->chessHallId, rc);
		if (hallData == NULL)
		{
			rsp.set_result(rc);
			break;
		}

		rsp.set_status(req.status());
		rsp.set_current_idx(req.current_idx());
		
		com_protocol::BSHallPlayerNumber* playerNumber = rsp.mutable_player_number();
		playerNumber->set_online_players(hallData->baseInfo.current_onlines());
		playerNumber->set_offilne_players(hallData->offilnePlayers);
		playerNumber->set_forbid_players(hallData->forbidPlayers);
		playerNumber->set_apply_players(hallData->applyPlayers);
		
		char tmpSql[1024] = {0};
	    unsigned int tmpSqlLen = sizeof(tmpSql);
		if (req.status() != com_protocol::EHallPlayerStatus::EWaitForCheck)
		{
			getSelectHallPlayerInfoSQL(req, tmpSql, tmpSqlLen);
		}
		else
		{
			getSelectHallNeedCheckUserInfoSQL(req, tmpSql, tmpSqlLen);
		}
		
		CQueryResult* pResult = NULL;
		if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
		{
			OptErrorLog("query chess hall player info error, sql = %s", tmpSql);
			rsp.set_result(BSGetHallPlayerInfoError);
			break;
		}
		
		if (pResult == NULL || pResult->getRowCount() < 1)
		{
			rsp.set_current_idx(-1);  // 没有数据了
			m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);
			break;
		}

		enum ETableFieldName
		{
			EIdxField = 0,
			EUIdField = 1,
			EUNameField = 2,
			EUPortraitField = 3,
			EHTimeField = 4,
			EHRemarkField = 5,
			EHExplainMsgField = 5,
			EHRechargeField = 6,
			EHBalanceField = 7,
			EHProfitField = 8,
			EHTaxField = 9,
			EHExchangeField = 10,
			EHHallSrvIdField = 11,
			EHGameSrvIdField = 12,
			EHOfflineField = 13,
		};

        const unsigned int curSecs = time(NULL);
		int currentIdx = -1;
		RowDataPtr rowData = NULL;
		if (req.status() != com_protocol::EHallPlayerStatus::EWaitForCheck)
		{
			// select H.idx,U.username,U.nickname,U.portrait_id,H.create_time,H.user_remark,H.recharge,
			// H.game_gold,H.profit_loss,H.tax_cost,H.exchange,H.online_game_service_id,H.offline_hall_secs
			const map<unsigned int, ServiceCommonConfig::GameInfo>& gameInfo = m_msgHandler->getSrvOpt().getServiceCommonCfg().game_info_map;
			unsigned int onlineGameId = 0;
			while ((rowData = pResult->getNextRow()) != NULL)
			{
				if (rowData[EUIdField] != NULL)
				{
					com_protocol::BSHallPlayerInfo* hpInfo = rsp.add_player_info();
					hpInfo->set_user_id(rowData[EUIdField]);
					hpInfo->set_nick_name(rowData[EUNameField]);
					hpInfo->set_portrait_id(rowData[EUPortraitField]);
					hpInfo->set_user_remark(rowData[EHRemarkField]);
					hpInfo->set_add_time(atoi(rowData[EHTimeField]));
					hpInfo->set_recharge(atof(rowData[EHRechargeField]));
					hpInfo->set_balance(atof(rowData[EHBalanceField]));
					hpInfo->set_profit_loss(atof(rowData[EHProfitField]));
					hpInfo->set_tax_cost(atof(rowData[EHTaxField]));
					hpInfo->set_exchange(atof(rowData[EHExchangeField]));
					
					onlineGameId = atoi(rowData[EHGameSrvIdField]);
					if (onlineGameId > 0)
					{
						map<unsigned int, ServiceCommonConfig::GameInfo>::const_iterator gIfIt = gameInfo.find(onlineGameId / ServiceIdBaseValue);
						if (gIfIt != gameInfo.end()) hpInfo->set_online_game_name(gIfIt->second.name);
					}
					else if (atoi(rowData[EHHallSrvIdField]) > 0)
					{
						hpInfo->set_online_game_name("大厅");
					}
					else
					{
					    hpInfo->set_offline_secs(curSecs - atoi(rowData[EHOfflineField]));
					}
					
					if (atoi(rowData[EIdxField]) > currentIdx) currentIdx = atoi(rowData[EIdxField]);
				}
			}
			
			if ((unsigned int)rsp.player_info_size() < m_msgHandler->m_pCfg->common_cfg.get_player_count) currentIdx = -1;
		}
		else
		{
			// select H.idx,U.username,U.nickname,U.portrait_id,H.create_time,H.explain_msg
			while ((rowData = pResult->getNextRow()) != NULL)
			{
				if (rowData[EUIdField] != NULL)
				{
					com_protocol::BSHallNeedCheckUserInfo* ncUserInfo = rsp.add_check_user();
					ncUserInfo->set_user_id(rowData[EUIdField]);
					ncUserInfo->set_nick_name(rowData[EUNameField]);
					ncUserInfo->set_portrait_id(rowData[EUPortraitField]);
					ncUserInfo->set_apply_time(atoi(rowData[EHTimeField]));
					ncUserInfo->set_explain_msg(rowData[EHExplainMsgField]);
					
					if (atoi(rowData[EIdxField]) > currentIdx) currentIdx = atoi(rowData[EIdxField]);
				}
			}
			
			if ((unsigned int)rsp.check_user_size() < m_msgHandler->m_pCfg->common_cfg.get_player_count) currentIdx = -1;
		}
		
		m_msgHandler->getOptDBOpt()->releaseQueryResult(pResult);
		
		rsp.set_current_idx(currentIdx);
		
	} while (false);
	
	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_GET_PLAYER_INFO_RSP, "get BS player info reply");
	
	OptInfoLog("get player info, manager = %s, hall id = %s, result = %d, status = %d, index = %d, send msg rc = %d",
	cud->userName, cud->chessHallId, rsp.result(), rsp.status(), rsp.current_idx(), rc);
}

void COptLogicHandler::getGameInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetBSHallGameInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get game info request")) return;

	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (req.hall_id() != cud->chessHallId)
	{
		OptErrorLog("get game info, hall id error, id = %s, param = %s", cud->chessHallId, req.hall_id().c_str());
		return;
	}

	int rc = SrvOptFailed;
    com_protocol::GetBSHallGameInfoRsp rsp;
	SChessHallData* hallData = m_msgHandler->getChessHallData(cud->chessHallId, rc);
	if (hallData != NULL)
	{
		rsp.set_result(SrvOptSuccess);
		const map<unsigned int, ServiceCommonConfig::GameInfo>& gameInfo = m_msgHandler->getSrvOpt().getServiceCommonCfg().game_info_map;
		for (map<unsigned int, ServiceCommonConfig::GameInfo>::const_iterator gIfIt = gameInfo.begin(); gIfIt != gameInfo.end(); ++gIfIt)
		{
			GameTypeToInfoMap::const_iterator hallGameIt = hallData->gameInfo.find(gIfIt->first);
			if (hallGameIt != hallData->gameInfo.end())
			{
				*rsp.add_hall_games() = hallGameIt->second;
			}
			else
			{
				com_protocol::BSHallGameInfo* newGameInfo = rsp.add_new_games();
				newGameInfo->set_type(gIfIt->first);
				newGameInfo->set_name(gIfIt->second.name);
				newGameInfo->set_desc(gIfIt->second.desc);
				newGameInfo->set_icon(gIfIt->second.icon);
			}
		}
	}
	else
	{
		rsp.set_result(rc);
	}
	
	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_GET_HALL_GAME_INFO_RSP, "get BS game info reply");
	
	OptInfoLog("get game info, manager = %s, id = %s, result = %d, send msg rc = %d",
	cud->userName, cud->chessHallId, rsp.result(), rc);
}

void COptLogicHandler::optHallGame(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::OptBSHallGameReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "opt hall game request")) return;

	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	int rc = SrvOptFailed;
    com_protocol::OptBSHallGameRsp rsp;
	SChessHallData* hallData = m_msgHandler->getChessHallData(cud->chessHallId, rc);
	if (hallData != NULL)
	{
		rsp.set_game_type(req.game_type());
		rsp.set_status(req.status());
		rsp.set_result(SrvOptSuccess);

        bool isAddNewGame = false;
		GameTypeToInfoMap::const_iterator hallGameIt;
		if (req.status() == com_protocol::EHallGameStatus::ENormalGame)
		{
			hallGameIt = hallData->gameInfo.find(req.game_type());
			if (hallGameIt == hallData->gameInfo.end())
			{
				// 新添加游戏
				isAddNewGame = true;
				rsp.set_result(addNewHallGame(cud, hallData, req.game_type()));
			}
		}
		
		if (!isAddNewGame)
		{
			rsp.set_result(updateHallGameStatus(cud, hallData, req.game_type(), req.status()));  // 刷新游戏状态
		}
		
		hallGameIt = hallData->gameInfo.find(req.game_type());
		if (hallGameIt != hallData->gameInfo.end())
		{
			*rsp.mutable_game_info() = hallGameIt->second;
		}
	}
	else
	{
		rsp.set_result(rc);
	}
	
	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_ADD_HALL_NEW_GAME_RSP, "add BS opt hall reply");
	
	OptInfoLog("opt game, manager = %s, id = %s, result = %d, game type = %d, status = %d, send msg rc = %d",
	cud->userName, cud->chessHallId, rsp.result(), req.game_type(), req.status(), rc);
}

void COptLogicHandler::setHallGameTaxRatio(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetBSHallGameTaxRatioReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "set hall game tax ratio request")) return;

	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	int rc = SrvOptFailed;
    com_protocol::SetBSHallGameTaxRatioRsp rsp;
	
	do
	{
		SChessHallData* hallData = m_msgHandler->getChessHallData(cud->chessHallId, rc);
		if (hallData == NULL)
		{
			rsp.set_result(rc);
			break;
		}
		
		GameTypeToInfoMap::const_iterator hallGameIt = hallData->gameInfo.find(req.game_type());
		if (hallGameIt == hallData->gameInfo.end())
		{
			rsp.set_result(BSNoFoundGameData);
			break;
		}
		
		const float taxRatio = req.service_tax_ratio();
		if (taxRatio < 0.001 || taxRatio > 1.000)
		{
			rsp.set_result(BSInvalidGameTaxRatio);
			break;
		}
		
		// 刷新游戏税收比例
		rsp.set_result(updateHallGameTaxRatio(cud, hallData, req.game_type(), taxRatio));

		rsp.set_game_type(req.game_type());
		rsp.set_service_tax_ratio(taxRatio);
	
	} while (false);
	
	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_SET_HALL_GAME_TAX_RATIO_RSP, "set hall game tax ratio reply");
	
	OptInfoLog("set hall game tax ratio, manager = %s, chess hall id = %s, result = %d, game type = %d, tax ratio = %.3f, send msg rc = %d",
	cud->userName, cud->chessHallId, rsp.result(), req.game_type(), req.service_tax_ratio(), rc);
}

void COptLogicHandler::optPlayer(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::OptBSHallPlayerReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "operation BS player request")) return;

	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (*cud->chessHallId == '\0')
	{
		OptErrorLog("manager opt player error, manager = %s, chess hall id = %s", cud->userName, cud->chessHallId);
		return;
	}
	
	com_protocol::ClientManagerOptHallPlayerReq mgrOptPlayerReq;
	mgrOptPlayerReq.set_allocated_username(req.release_username());
	mgrOptPlayerReq.set_status(req.status());
	mgrOptPlayerReq.set_hall_id(cud->chessHallId);
	
	int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, mgrOptPlayerReq,
												           DBPROXY_MANAGER_OPT_PLAYER_INFO_REQ, "operation BS player request");
											
	WriteDataLog("operation player status|%s|%s|%s|%d|%d|request",
	cud->userName, cud->chessHallId, mgrOptPlayerReq.username().c_str(), mgrOptPlayerReq.status(), rc);
}

void COptLogicHandler::optPlayerReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientManagerOptHallPlayerRsp mgrOptPlayerRsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(mgrOptPlayerRsp, data, len, "operation BS player reply")) return;
	
	if (mgrOptPlayerRsp.result() == SrvOptSuccess)
	{
	    // 状态结果即时通知在线玩家
	    notifyChessHallPlayerStatus(mgrOptPlayerRsp.hall_id().c_str(), mgrOptPlayerRsp.username().c_str(), mgrOptPlayerRsp.status());
	}

    int rc = SrvOptFailed;
	const char* managerName = NULL;
	ConnectProxy* conn = m_msgHandler->getSrvOpt().getConnectProxyInfo(managerName, "operation BS player reply");
	if (conn != NULL) rc = m_msgHandler->sendMsgToProxy(data, len, BSC_OPT_HALL_PLAYER_RSP, conn);

    WriteDataLog("operation player status|%s|%s|%s|%d|%d|reply",
	(managerName != NULL) ? managerName : "", mgrOptPlayerRsp.hall_id().c_str(), mgrOptPlayerRsp.username().c_str(),
	mgrOptPlayerRsp.result(), rc);
	
	// OptInfoLog("opt player, manager = %s, hall id = %s, result = %d, player name = %s, status = %d, send msg rc = %d",
	// (managerName != NULL) ? managerName : "", mgrOptPlayerRsp.hall_id().c_str(), mgrOptPlayerRsp.result(),
	// mgrOptPlayerRsp.username().c_str(), mgrOptPlayerRsp.status(), rc);
}

void COptLogicHandler::addManager(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
}

// 服务器（棋牌室管理员）主动通知玩家退出棋牌室（从棋牌室踢出玩家）
void COptLogicHandler::notifyPlayerQuitChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientPlayerQuitChessHallNotify quitNotify;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(quitNotify, data, len, "notify player quit chess hall")) return;

    OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	
	unsigned int hallSrvId = 0;
	unsigned int gameSrvId = 0;
	if (!m_msgHandler->getOnlineServiceId(cud->chessHallId, quitNotify.username().c_str(), hallSrvId, gameSrvId)
	    || hallSrvId <= ServiceIdBaseValue) return;

	if (gameSrvId == 0) gameSrvId = hallSrvId;

	m_msgHandler->getSrvOpt().forwardMessage(quitNotify.username().c_str(), data, len, gameSrvId / ServiceIdBaseValue,
	OPTMGR_PLAYER_QUIT_CHESS_HALL_NOTIFY, m_msgHandler->getSrvId(), 0, "forward player quit chess hall notify message");
}

void COptLogicHandler::getMallData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BSGetMallInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get mall info request")) return;
	
	com_protocol::BSGetMallInfoRsp rsp;
	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	int rc = SrvOptFailed;
	do
	{
		if (req.hall_id().empty() || req.hall_id() != cud->chessHallId)
		{
			rsp.set_result(BSChessHallIdParamError);
			break;
		}
		
		// 获取对应时间点商城操作的总数额
		double todayPayGold = 0.00;
        rc = getTimeMallOptSumValue(cud->chessHallId, com_protocol::EHallPlayerOpt::EOptBuyGoods, time(NULL), todayPayGold);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		com_protocol::BSMallInfo* mallInfo = rsp.mutable_mall_info();
		mallInfo->set_today_pay_gold(todayPayGold);
		
		// 获取商城操作信息
		const unsigned int getMallRecordCount = m_msgHandler->m_pCfg->common_cfg.manager_get_mall_record_count;
		int currentIdx = -1;
		rc = getMallOptInfo(cud->chessHallId, com_protocol::EHallPlayerOpt::EOptBuyGoods, com_protocol::EMallOptStatus::EMallOptUnconfirm,
		                    com_protocol::EMallOptStatus::EMallOptUnconfirm, 0, getMallRecordCount,
					        *mallInfo->mutable_unconfirm_info(), currentIdx);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		// 一开始默认为0，列表的最后索引值，大于0表示还存在记录，小于0则表示记录已经遍历完毕
		if ((unsigned int)rsp.mall_info().unconfirm_info_size() < getMallRecordCount) currentIdx = -1;
		
		rsp.set_result(SrvOptSuccess);
		mallInfo->set_current_idx(currentIdx);
			
	} while (false);
	
	if (rsp.result() != SrvOptSuccess) rsp.clear_mall_info();
	
	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_GET_MALL_DATA_RSP, "get mall info reply");
	
	OptInfoLog("get mall info, manager = %s, hall id = %s, result = %d, request hall id = %s, send msg rc = %d",
	cud->userName, cud->chessHallId, rsp.result(), req.hall_id().c_str(), rc);
}

void COptLogicHandler::getMallBuyInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BSGetMallBuyInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get mall buy info request")) return;
	
	com_protocol::BSGetMallBuyInfoRsp rsp;
	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();

	// 获取商城操作信息
	const unsigned int getMallRecordCount = m_msgHandler->m_pCfg->common_cfg.manager_get_mall_record_count;
	int currentIdx = -1;
	int rc = getMallOptInfo(cud->chessHallId, com_protocol::EHallPlayerOpt::EOptBuyGoods, req.status(), req.status(), req.current_idx(),
	                        getMallRecordCount, *rsp.mutable_buy_info(), currentIdx);
	
	rsp.set_result(rc);
	if (rc == SrvOptSuccess)
	{
		rsp.set_status(req.status());
		
		// 一开始默认为0，列表的最后索引值，大于0表示还存在记录，小于0则表示记录已经遍历完毕
	    if ((unsigned int)rsp.buy_info_size() < getMallRecordCount) currentIdx = -1;
		rsp.set_current_idx(currentIdx);
	}

	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_GET_MALL_BUY_INFO_RSP, "get mall buy info reply");
	
	OptInfoLog("get mall buy info, manager = %s, hall id = %s, result = %d, new idx = %d, request status = %d, idx = %d, send msg rc = %d",
	cud->userName, cud->chessHallId, rsp.result(), currentIdx, req.status(), req.current_idx(), rc);
}

void COptLogicHandler::getMallGiveInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BSGetMallGiveInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get mall give info request")) return;
	
	com_protocol::BSGetMallGiveInfoRsp rsp;
	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();

	// 获取商城操作信息
	const unsigned int getMallRecordCount = m_msgHandler->m_pCfg->common_cfg.manager_get_mall_record_count;
	int currentIdx = -1;
	int rc = getMallOptInfo(cud->chessHallId, com_protocol::EHallPlayerOpt::EOptGiveGift,
	                        com_protocol::EMallOptStatus::EMallOptUnconfirm, com_protocol::EMallOptStatus::EMallOptConfirm,
	                        req.current_idx(), getMallRecordCount, *rsp.mutable_give_info(), currentIdx);
	
	rsp.set_result(rc);
	if (rc == SrvOptSuccess)
	{
		// 一开始默认为0，列表的最后索引值，大于0表示还存在记录，小于0则表示记录已经遍历完毕
	    if ((unsigned int)rsp.give_info_size() < getMallRecordCount) currentIdx = -1;
		rsp.set_current_idx(currentIdx);
	}

	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_GET_MALL_GIVE_INFO_RSP, "get mall give info reply");
	
	OptInfoLog("get mall give info, manager = %s, hall id = %s, result = %d, new idx = %d, request idx = %d, send msg rc = %d",
	cud->userName, cud->chessHallId, rsp.result(), currentIdx, req.current_idx(), rc);
}

void COptLogicHandler::confirmBuyInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BSConfirmBuyInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "confirm buy info request")) return;

	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (*cud->chessHallId == '\0')
	{
		com_protocol::BSConfirmBuyInfoRsp rsp;
		rsp.set_result(BSChessHallIdParamError);

		m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_CONFIRM_BUY_INFO_RSP, "confirm buy info error reply");
		
		return;
	}
	
	req.set_manager(cud->userName);
	req.set_hall_id(cud->chessHallId);
	
	int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
												           DBPROXY_CONFIRM_MALL_BUY_INFO_REQ, "confirm buy info request");
														   
	WriteDataLog("confirm buy info|%s|%s|%s|%d|request", cud->userName, cud->chessHallId, req.record_id().c_str(), rc);
}

void COptLogicHandler::confirmBuyInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	int rc = SrvOptFailed;
	const char* managerName = NULL;
	ConnectProxy* conn = m_msgHandler->getSrvOpt().getConnectProxyInfo(managerName, "confirm buy info reply");
	if (conn != NULL) rc = m_msgHandler->sendMsgToProxy(data, len, BSC_CONFIRM_BUY_INFO_RSP, conn);

    WriteDataLog("confirm buy info|%s|%d|reply", (managerName != NULL) ? managerName : "", rc);

	// OptInfoLog("confirm buy info, manager = %s, send msg rc = %d", (managerName != NULL) ? managerName : "", rc);
}

void COptLogicHandler::setPlayerInitGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BSSetPlayerInitGoodsReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "set player init gold request")) return;

    int rc = SrvOptFailed;
	float playerInitGold = 0.00;
	OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	com_protocol::BSSetPlayerInitGoodsRsp rsp;
	do
	{
		if (req.player_init_goods_size() < 1 || req.player_init_goods(0).count() < 0.00)
		{
			OptErrorLog("set player init gold param error, manager = %s, hall id = %s, goods size = %d",
			cud->userName, cud->chessHallId, req.player_init_goods_size());

			rsp.set_result(BSPlayerInitGoldParamError);
			break;
		}

		SChessHallData* hallData = m_msgHandler->getChessHallData(cud->chessHallId, rc);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		playerInitGold = req.player_init_goods(0).count();
		char tmpSql[256] = {0};
		const unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"update tb_chess_card_hall set player_init_gold=%.2f where hall_id=\'%s\';", playerInitGold, cud->chessHallId);
		
		if (SrvOptSuccess != m_msgHandler->getOptDBOpt()->executeSQL(tmpSql, tmpSqlLen))
		{
			OptErrorLog("update player init gold error, sql = %s", tmpSql);
			rsp.set_result(BSUpdatePlayerInitGoldError);
			break;
		}

		if (1 != m_msgHandler->getOptDBOpt()->getAffectedRows())
		{
			OptErrorLog("update player init gold failed, sql = %s", tmpSql);
			rsp.set_result(BSUpdatePlayerInitGoldError);
			break;
		}
		
		hallData->baseInfo.set_player_init_gold(playerInitGold);

		rsp.set_result(SrvOptSuccess);
		*rsp.mutable_player_init_goods() = req.player_init_goods();

	} while (false);
	
	rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_SET_PLAYER_INIT_GOODS_RSP, "set player init gold reply");
	
	WriteDataLog("set player init goods|%s|%s|%d|%.2f|%d", cud->userName, cud->chessHallId, rsp.result(), playerInitGold, rc);
	
	// OptInfoLog("set player init gold, manager = %s, hall id = %s, result = %d, player init gold = %.2f send msg rc = %d",
	// cud->userName, cud->chessHallId, rsp.result(), playerInitGold, rc);
}

void COptLogicHandler::giveGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BSGiveGoodsReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "give goods request")) return;

	com_protocol::BSGiveGoodsRsp rsp;
	do
	{
		// 棋牌室信息验证
		const OptMgrUserData* cud = (OptMgrUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
		int rc = SrvOptFailed;
		SChessHallData* hallData = m_msgHandler->getChessHallData(cud->chessHallId, rc);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}

		// 验证玩家用户是否在棋牌室中
		StringKeyValueMap keyValue;
		keyValue["username"] = "";
		rc = m_msgHandler->getChessHallPlayerInfo(cud->chessHallId, req.username().c_str(), keyValue);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		// 物品数量验证
		const float giveGoodsCount = req.change_goods().num();
		
		/*
		if (giveGoodsCount < 0.001)
		{
			rsp.set_result(BSGiveGoodsParamInvalid);
			break;
		}
		*/

		float remainGoodsCount = 0.00;
		const unsigned int goodsType = req.change_goods().type();
		switch (goodsType)
		{
			case EGameGoodsType::EGoodsGold:
			{
				remainGoodsCount = hallData->baseInfo.current_gold() - giveGoodsCount;
				break;
			}
			
			case EGameGoodsType::EGoodsRoomCard:
			{
				remainGoodsCount = hallData->baseInfo.current_room_card() - giveGoodsCount;
				break;
			}
			
			default:
			{
				rc = BSGiveGoodsTypeInvalid;
				break;
			}
		}
		
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		if (remainGoodsCount < 0.001)
		{
			rsp.set_result(BSChessHallGoodsInsufficient);
			break;
		}
		
		req.set_hall_id(cud->chessHallId);

		unsigned int dbProxySrvId = 0;
		NProject::getDestServiceID(ServiceType::DBProxySrv, req.username().c_str(), req.username().length(), dbProxySrvId);
		rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, req, dbProxySrvId,
														   DBPROXY_GIVE_MALL_GOODS_REQ, "manager give gold to player");

        WriteDataLog("give goods|%s|%s|%s|%u|%.2f|%.2f|%d|request",
		cud->userName, cud->chessHallId, req.username().c_str(),
		goodsType, giveGoodsCount, remainGoodsCount, rc);
		
		// OptInfoLog("manager give gold to player, manager = %s, hall id = %s, username = %s, count = %.2f, remain = %.2f",
		// cud->userName, cud->chessHallId, req.username().c_str(), giveGoldCount, hallData->baseInfo.current_gold() - giveGoldCount);

		return;

	} while (false);
	
	m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_GIVE_GOODS_RSP, "give goods check error reply");
}

void COptLogicHandler::giveGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BSGiveGoodsRsp rsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(rsp, data, len, "manager give goods reply"))
	{
		OptErrorLog("manager give goods reply error, userdata = %s", m_msgHandler->getContext().userData);

		return;
	}
	
	int rc = SrvOptFailed;
	com_protocol::ItemChange& giveItem = *rsp.mutable_change_goods();
	const float playerItemAmount = giveItem.amount();
	SChessHallData* hallData = NULL;
	if (rsp.result() == SrvOptSuccess)
	{
		// 同步通知C端在线玩家
		com_protocol::ClientGiveGoodsToPlayerNotify cggNtf;
		cggNtf.set_record_id(rsp.record_id());
		*cggNtf.mutable_give_goods() = giveItem;
		const char* cggNtfData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(cggNtf, "give gold notify player");
		if (cggNtfData != NULL)
		{
		    com_protocol::ForwardMessageToServiceNotify ggMsgNtf;
			ggMsgNtf.set_username(rsp.username());
			ggMsgNtf.set_data(cggNtfData, cggNtf.ByteSize());

			com_protocol::TargetServiceInfo* srvInfo = ggMsgNtf.add_srv_info();
			srvInfo->set_type(ServiceType::GameHallSrv);  // 大厅
			srvInfo->set_protocol(SERVICE_MANAGER_GIVE_GOODS_NOTIFY);
			
			srvInfo = ggMsgNtf.add_srv_info();
			srvInfo->set_type(0);  // 游戏服务
			srvInfo->set_protocol(SERVICE_MANAGER_GIVE_GOODS_NOTIFY);
			
			m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp.username().c_str(), rsp.username().length(),
							                              ServiceType::MessageCenterSrv, ggMsgNtf, MSGCENTER_FORWARD_SERVICE_MESSAGE_NOTIFY,
														  "give goods notify player");
		}
		
		// 更新棋牌室物品数量
		hallData = m_msgHandler->getChessHallData(rsp.hall_id().c_str(), rc, true);
		if (hallData != NULL)
		{
			if (giveItem.type() == EGameGoodsType::EGoodsGold)
			{
				giveItem.set_amount(hallData->baseInfo.current_gold());
			}
			else if (giveItem.type() == EGameGoodsType::EGoodsRoomCard)
			{
				giveItem.set_amount(hallData->baseInfo.current_room_card());
			}
			else
			{
				rsp.set_result(BSGiveGoodsTypeInvalid);
				
				OptErrorLog("manager give goods to player reply but the goods type error, type = %u, hallId = %s, username = %s, recordId = %s",
	            giveItem.type(), rsp.hall_id().c_str(), rsp.username().c_str(), rsp.record_id().c_str());
			}
		}
		else
		{
			rsp.set_result(rc);
		}
	}

    rc = SrvOptFailed;
    const char* managerName = NULL;
	ConnectProxy* conn = m_msgHandler->getSrvOpt().getConnectProxyInfo(managerName, "give goods reply");
	if (conn != NULL) rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_GIVE_GOODS_RSP, conn, "give goods reply");
	
	WriteDataLog("give goods|%s|%s|%s|%u|%.2f|%.2f|%.2f|%d|%d|reply",
	(managerName != NULL) ? managerName : "", rsp.hall_id().c_str(), rsp.username().c_str(),
	giveItem.type(), giveItem.num(), playerItemAmount, giveItem.amount(), rsp.result(), rc);
		
	// OptInfoLog("manager give goods reply, manager = %s, record id = %s, hall id = %s, username = %s, give good type = %u, count = %.2f, remain = %.2f, result = %d, rc = %d",
	// (managerName != NULL) ? managerName : "", rsp.record_id().c_str(), rsp.hall_id().c_str(), rsp.username().c_str(),
	// giveItem.type(), giveItem.num(), (hallData != NULL) ? hallData->baseInfo.current_gold() : 0.00, rsp.result(), rc);
}

// 管理员获取玩家信息
void COptLogicHandler::getUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetUserDataReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "manager get user info request")) return;

	// 从逻辑DB查找用户静态信息
	StringKeyValueMap keyValue;
	keyValue["username"] = "";
	keyValue["nickname"] = "";
	keyValue["portrait_id"] = "";
	keyValue["gender"] = "";
	
	com_protocol::ClientGetUserDataRsp rsp;
	rsp.set_result(m_msgHandler->getPlayerStaticInfo(req.username().c_str(), keyValue));
    if (rsp.result() == SrvOptSuccess)
	{
		com_protocol::StaticInfo* userStaticInfo = rsp.mutable_detail_info()->mutable_static_info();
		userStaticInfo->set_username(keyValue["username"]);
		userStaticInfo->set_nickname(keyValue["nickname"]);
		userStaticInfo->set_portrait_id(keyValue["portrait_id"]);
		userStaticInfo->set_gender(atoi(keyValue["gender"].c_str()));
	}
	
	m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_GET_GAME_PLAYER_DATA_RSP, "manager get user info reply");
	
	/* 向dbproxy获取玩家数据
	com_protocol::GetUserDataReq getUserDataReq;
	getUserDataReq.set_username(req.username());
	getUserDataReq.set_info_type(com_protocol::EUserInfoType::EUserStaticDynamic);

    OptMgrUserData* cud = m_msgHandler->getUserData();
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->connIdLen, ServiceType::DBProxySrv, getUserDataReq,
												  DBPROXY_GET_USER_DATA_REQ, "get user info request from manager");
	
	 
	// 应答消息回来
	const char* userName = NULL;
	ConnectProxy* conn = m_msgHandler->getConnectInfo(userName, "get user info from manager reply");
	if (conn == NULL) return;

	com_protocol::GetUserDataRsp getUserDataRsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(getUserDataRsp, data, len, "get user info from manager reply")) return;
	
	com_protocol::ClientGetUserDataRsp rsp;
	rsp.set_result(getUserDataRsp.result());
	if (rsp.result() == SrvOptSuccess) rsp.set_allocated_detail_info(getUserDataRsp.release_detail_info());
	
	m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, BSC_GET_GAME_PLAYER_DATA_RSP, conn, "get user info from manager reply");
	*/ 
}

// 玩家购买商城物品通知
void COptLogicHandler::buyMallGoodsNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BSPlayerBuyMallGoodsNotify ntf;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(ntf, data, len, "player buy mall goods notify"))
	{
		OptErrorLog("player buy mall goods notify, userdata = %s", m_msgHandler->getContext().userData);

		return;
	}

    int rc = SrvOptFailed;
	SChessHallData* hallData = m_msgHandler->getChessHallData(ntf.hall_id().c_str(), rc, true);
	// if (hallData != NULL) hallData->baseInfo.set_current_gold(hallData->baseInfo.current_gold() + (-ntf.gold_info.num()));
	
	WriteDataLog("buy goods|%s|%s|%u|%.2f|%u|%.2f|%.2f|%.2f|%d|notify",
	m_msgHandler->getContext().userData, ntf.hall_id().c_str(), ntf.buy_goods().type(), ntf.buy_goods().count(),
	ntf.gold_info().type(), ntf.gold_info().num(), ntf.gold_info().amount(), (hallData != NULL) ? hallData->baseInfo.current_gold() : 0.00, rc);
}

// 玩家申请加入棋牌室通知管理员
void COptLogicHandler::applyJoinChessHallNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::PlayerApplyJoinChessHallNotify notify;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(notify, data, len, "player apply join chess hall notify")) return;

	// 玩家申请加入棋牌室，通知B端在线的管理员
	sendMessageToAllManager(notify.hall_id(), notify, BSC_PLAYER_APPLY_JOIN_HALL_NOTIFY, "player apply join hall notify");
}

// 管理员在C端获取申请待审核的玩家列表
void COptLogicHandler::getApplyJoinPlayers(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientManagerGetApplyJoinPlayersReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get apply join players request")) return;

    com_protocol::ClientManagerGetApplyJoinPlayersRsp rsp;
    do
	{

		const char* logicDbName = m_msgHandler->getSrvOpt().getDBCfg().logic_db_cfg.dbname.c_str();

		// 先检查该玩家是否是合法的管理员
		char tmpSql[1024] = {0};
		unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"select HS.name from %s.tb_user_platform_map_info as PT left join tb_chess_hall_manager_static_info as HS on "
		"PT.platform_id=HS.platform_id where HS.platform_type=PT.platform_type+1 and PT.username=\'%s\';",
		logicDbName, m_msgHandler->getContext().userData);
		
		CDBOpertaion* optDb = m_msgHandler->getOptDBOpt();
		CQueryResult* pResult = NULL;
		if (SrvOptSuccess != optDb->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
		{
			OptErrorLog("query manager name error, sql = %s", tmpSql);

            rsp.set_result(BSQueryManagerIdError);
			break;
		}
		
		if (pResult == NULL || pResult->getRowCount() != 1)
		{
			OptErrorLog("query manager name failed, sql = %s, row = %u", tmpSql, (pResult != NULL) ? pResult->getRowCount() : 0);
			optDb->releaseQueryResult(pResult);
			
			rsp.set_result(BSGetManagerIdError);
			break;
		}
		
		tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"select 1 from tb_chess_hall_manager_dynamic_info where hall_id=\'%s\' and status=0 and name=\'%s\';",
		req.hall_id().c_str(), pResult->getNextRow()[0]);
		
		optDb->releaseQueryResult(pResult);
		
		// 检查是否是管理员
		pResult = NULL;
		if (SrvOptSuccess != optDb->queryTableAllResult(tmpSql, tmpSqlLen, pResult)
		    || pResult == NULL || pResult->getRowCount() != 1)
		{
			OptErrorLog("check manager info error, sql = %s, row = %u", tmpSql, (pResult != NULL) ? pResult->getRowCount() : 0);
			optDb->releaseQueryResult(pResult);

            rsp.set_result(BSCheckManagerInfoError);
			break;
		}
		optDb->releaseQueryResult(pResult);

        // 获取待审核的玩家信息
		tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
		"select U.username,U.nickname,U.portrait_id,U.gender,UNIX_TIMESTAMP(H.create_time),H.explain_msg from %s.tb_chess_hall_user_info as H left join "
		"%s.tb_user_static_baseinfo as U on H.username=U.username where H.user_status=%u and H.hall_id=\'%s\';",
		logicDbName, logicDbName, com_protocol::EHallPlayerStatus::EWaitForCheck, req.hall_id().c_str());
		
		pResult = NULL;
		if (SrvOptSuccess != optDb->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
		{
			OptErrorLog("query apply join player info error, sql = %s", tmpSql);
			rsp.set_result(BSQueryApplyJoinPlayerInfoError);
			break;
		}

		rsp.set_result(SrvOptSuccess);

        if (pResult != NULL && pResult->getRowCount() > 0)
		{
			RowDataPtr rowData = NULL;
			while ((rowData = pResult->getNextRow()) != NULL)
			{
				if (rowData[0] != NULL)
				{
					com_protocol::ApplyJoinPlayerInfo* playerInfo = rsp.add_players();
					com_protocol::StaticInfo* staticInfo = playerInfo->mutable_static_info();
					staticInfo->set_username(rowData[0]);
					staticInfo->set_nickname(rowData[1]);
					staticInfo->set_portrait_id(rowData[2]);
					staticInfo->set_gender(atoi(rowData[3]));

					playerInfo->set_apply_time(atoi(rowData[4]));
					playerInfo->set_explain_msg(rowData[5]);
				}
			}
		}
		optDb->releaseQueryResult(pResult);
	
	} while (false);
	
	if (srcProtocolId == 0) srcProtocolId = OPTMGR_GET_APPLY_JOIN_PLAYERS_RSP;
	m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get apply join players reply");
}

void COptLogicHandler::managerOptHallPlayer(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientManagerOptHallPlayerReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "manager operation player request")) return;

	int rc = m_msgHandler->getSrvOpt().sendMsgToService(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen,
	ServiceType::DBProxySrv, data, len, DBPROXY_MANAGER_OPT_PLAYER_INFO_REQ, srcSrvId, OPTMGR_CUSTOM_OPT_HALL_PLAYER_RESULT);

	WriteDataLog("operation player status|%s|%s|%s|%d|%d|request",
	m_msgHandler->getContext().userData, req.hall_id().c_str(), req.username().c_str(), req.status(), rc);
}

void COptLogicHandler::managerOptHallPlayerReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientManagerOptHallPlayerRsp mgrOptPlayerRsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(mgrOptPlayerRsp, data, len, "manager operation player reply")) return;
	
	// 先回应答给源服务
	int rc = m_msgHandler->getSrvOpt().sendMsgToService(data, len, m_msgHandler->getContext().userFlag, OPTMGR_OPT_HALL_PLAYER_RSP);

    WriteDataLog("operation player status|%s|%s|%s|%d|%d|reply",
	m_msgHandler->getContext().userData, mgrOptPlayerRsp.hall_id().c_str(), mgrOptPlayerRsp.username().c_str(),
	mgrOptPlayerRsp.result(), rc);

    // 接着通知目标玩家
	if (mgrOptPlayerRsp.result() == SrvOptSuccess)
	{
	    // 状态结果即时通知在线玩家
	    notifyChessHallPlayerStatus(mgrOptPlayerRsp.hall_id().c_str(), mgrOptPlayerRsp.username().c_str(), mgrOptPlayerRsp.status());
		
		// 状态结果即时通知在线的B端管理员
		com_protocol::ClientManagerOptHallPlayerNotify mgrOptPlayerNotify;
		mgrOptPlayerNotify.set_allocated_username(mgrOptPlayerRsp.release_username());
		mgrOptPlayerNotify.set_status(mgrOptPlayerRsp.status());
	    sendMessageToAllManager(mgrOptPlayerRsp.hall_id(), mgrOptPlayerNotify, BSC_MANAGER_OPT_HALL_PLAYER_NOTIFY, "manager operation player notify");
	}
}
