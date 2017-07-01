
/* author : limingfan
 * date : 2017.01.23
 * description : 翻翻棋相关逻辑处理
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "CLogic.h"
#include "CSrvMsgHandler.h"
#include "CFFChessLogicHandler.h"
#include "base/Function.h"
#include "../common/CommonType.h"


using namespace NProject;


// 兑换日志文件
#define ExchangeLog(format, args...)     m_msgHandler->getLogic().getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)


CFFChessLogicHandler::CFFChessLogicHandler()
{
	m_msgHandler = NULL;
}

CFFChessLogicHandler::~CFFChessLogicHandler()
{
	m_msgHandler = NULL;
}

int CFFChessLogicHandler::init(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;
	
	m_msgHandler->registerProtocol(ServiceType::XiangQiSrv, CommonSrvBusiness::BUSINESS_ADD_FF_CHESS_USER_BASE_INFO_REQ, (ProtocolHandler)&CFFChessLogicHandler::addUserBaseInfo, this);
	m_msgHandler->registerProtocol(ServiceType::XiangQiSrv, CommonSrvBusiness::BUSINESS_GET_FF_CHESS_USER_BASE_INFO_REQ, (ProtocolHandler)&CFFChessLogicHandler::getUserBaseInfo, this);
	m_msgHandler->registerProtocol(ServiceType::XiangQiSrv, CommonSrvBusiness::BUSINESS_SET_FF_CHESS_MATCH_RESULT_REQ, (ProtocolHandler)&CFFChessLogicHandler::setUserMatchResult, this);

	return 0;
}

void CFFChessLogicHandler::unInit()
{

}

void CFFChessLogicHandler::updateConfig()
{
}

int CFFChessLogicHandler::getFFChessUserBaseInfo(const char* username, SFFChessUserBaseinfo* baseInfo)
{
	// 目前看人数不多，所以先直接读写DB，如果在线人数多了会影响性能则改为内存数据库读写&同步
	char sql[2048] = {0};
	CQueryResult* p_result = NULL;
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "select max_rate,max_continue_win,current_continue_win,all_times,win_times,win_gold,lost_gold from tb_ff_chess_user_base_info where username=\'%s\';", username);
	int rc = m_msgHandler->m_pDBOpt->queryTableAllResult(sql, sqlLen, p_result);
	if (Success != rc)
	{
		OptErrorLog("select ff chess user base info from db error, sql = %s, rc = %d", sql, rc);
		return ServiceGetUserinfoFailed;
	}
	
	if (p_result == NULL || p_result->getRowCount() != 1)
	{
		OptWarnLog("select ff chess user base info from db but not find user, user = %s, count = %d",
		username, (p_result == NULL) ? 0 : p_result->getRowCount());
		
		m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
		return FFChessUserNotExistence;
	}

    if (baseInfo != NULL)
	{
		RowDataPtr rowData = p_result->getNextRow();
		baseInfo->max_rate = atoi(rowData[0]);
		baseInfo->max_continue_win = atoi(rowData[1]);
		baseInfo->current_continue_win = atoi(rowData[2]);
		baseInfo->all_times = atoi(rowData[3]);
		baseInfo->win_times = atoi(rowData[4]);
		baseInfo->win_gold = atoll(rowData[5]);
		baseInfo->lost_gold = atoll(rowData[6]);
		strncpy(baseInfo->username, username, sizeof(baseInfo->username) - 1);
	}

	m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
	
	return ServiceSuccess;
}

// 增加翻翻棋用户数据到DB，调用该函数成功后必须调用DB接口mysqlCommit()执行提交修改确认
int CFFChessLogicHandler::addFFChessUserBaseInfo(const char* username, const SFFChessUserBaseinfo& baseInfo, const bool needCommit)
{
	if (getFFChessUserBaseInfo(username, NULL) == ServiceSuccess) return ServiceSuccess;

	// 目前看人数不多，所以先直接读写DB，如果在线人数多了会影响性能则改为内存数据库读写&同步
	char sql[2048] = {0};
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "insert into tb_ff_chess_user_base_info(username,insert_time,max_rate,max_continue_win,current_continue_win,all_times,win_times) \
	                                     values(\'%s\',now(),%u,%u,%u,%u,%u);",
										 username, baseInfo.max_rate, baseInfo.max_continue_win, baseInfo.current_continue_win, baseInfo.all_times, baseInfo.win_times);
	int rc = m_msgHandler->m_pDBOpt->modifyTable(sql, sqlLen);
	if (Success != rc)
	{
		OptErrorLog("insert ff chess user base info to db error, sql = %s, rc = %d", sql, rc);
		return InsertFFChessUserInfoError;
	}
	
	if (needCommit) m_msgHandler->mysqlCommit();  // 提交修改

	return ServiceSuccess;
}

// 刷新翻翻棋用户数据到DB，调用该函数成功后必须调用DB接口mysqlCommit()执行提交修改确认
int CFFChessLogicHandler::updateFFChessUserBaseInfo(const char* username, const SFFChessUserBaseinfo& baseInfo, const bool needCommit)
{
	// 目前看人数不多，所以先直接读写DB，如果在线人数多了会影响性能则改为内存数据库读写&同步
	char sql[2048] = {0};
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "update tb_ff_chess_user_base_info set max_rate=%u,max_continue_win=%u,current_continue_win=%u,all_times=%u,win_times=%u,win_gold=%lu,lost_gold=%lu where username=\'%s\';",
	                                     baseInfo.max_rate, baseInfo.max_continue_win, baseInfo.current_continue_win, baseInfo.all_times, baseInfo.win_times, baseInfo.win_gold, baseInfo.lost_gold, username);
	int rc = m_msgHandler->m_pDBOpt->modifyTable(sql, sqlLen);
	if (Success != rc)
	{
		OptErrorLog("update ff chess user base info to db error, sql = %s, rc = %d", sql, rc);
		return UpdateFFChessUserInfoError;
	}
	
	if (needCommit) m_msgHandler->mysqlCommit();  // 提交修改

	return ServiceSuccess;
}

// 发送消息包
int CFFChessLogicHandler::sendPkgMsgToService(const ::google::protobuf::Message& msg, const unsigned int srcSrvId, const unsigned short protocolId, const char* info)
{
	int rc = ServiceFailed;
	char send_data[MaxMsgLen] = {0};
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(msg, send_data, send_data_len, info);
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, protocolId);
	
	return rc;
}


// 添加新用户基本数据
void CFFChessLogicHandler::addUserBaseInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::AddFFChessUserBaseInfoRsp rsp;
	SFFChessUserBaseinfo baseInfo;
	memset(&baseInfo, 0, sizeof(baseInfo));
	rsp.set_result(addFFChessUserBaseInfo(m_msgHandler->getContext().userData, baseInfo, true));
	
	sendPkgMsgToService(rsp, srcSrvId, CommonSrvBusiness::BUSINESS_ADD_FF_CHESS_USER_BASE_INFO_RSP, "add ff chess user base info reply");
}

// 获取用户数据
void CFFChessLogicHandler::getUserBaseInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetFFChessUserBaseInfoReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "get ff chess user base info request")) return;
	
	com_protocol::ClientGetFFChessUserBaseInfoRsp rsp;
	SFFChessUserBaseinfo baseInfo;
	rsp.set_result(getFFChessUserBaseInfo(req.username().c_str(), &baseInfo));
	if (rsp.result() == ServiceSuccess)
	{
		com_protocol::ClientFFChessUserBaseInfo* clientBaseInfo = rsp.mutable_base_info();
		clientBaseInfo->set_username(req.username());
		clientBaseInfo->set_max_rate(baseInfo.max_rate);
		clientBaseInfo->set_max_continue_win(baseInfo.max_continue_win);
		clientBaseInfo->set_current_continue_win(baseInfo.current_continue_win);
		clientBaseInfo->set_all_times(baseInfo.all_times);
		clientBaseInfo->set_win_times(baseInfo.win_times);
	}
	
	sendPkgMsgToService(rsp, srcSrvId, CommonSrvBusiness::BUSINESS_GET_FF_CHESS_USER_BASE_INFO_RSP, "get ff chess user base info reply");
}

// 设置翻翻棋用户棋局比赛结果
void CFFChessLogicHandler::setUserMatchResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetFFChessUserMatchResultReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "set ff chess user match result request")) return;
	
	com_protocol::SetFFChessUserMatchResultRsp rsp;	
	int rc = ServiceFailed;
	SFFChessUserBaseinfo baseInfo;
	for (int idx = 0; idx < req.match_result_size(); ++idx)
	{
		const com_protocol::SFFChessMatchResult& matchResult = req.match_result(idx);
		rc = getFFChessUserBaseInfo(matchResult.username().c_str(), &baseInfo);
		if (rc == ServiceSuccess)
		{
			++baseInfo.all_times;
			if (matchResult.rate() > baseInfo.max_rate) baseInfo.max_rate = matchResult.rate();
			if (matchResult.result() == com_protocol::EFFChessMatchResult::EFFChessMatchWin)
			{
				++baseInfo.current_continue_win;
				++baseInfo.win_times;
				
				if (baseInfo.current_continue_win > baseInfo.max_continue_win) baseInfo.max_continue_win = baseInfo.current_continue_win;
			}
			else
			{
				baseInfo.current_continue_win = 0;
			}
			
			if (matchResult.has_win_gold()) baseInfo.win_gold += matchResult.win_gold();
			else if (matchResult.has_lost_gold()) baseInfo.lost_gold += matchResult.lost_gold();
			
			rc = updateFFChessUserBaseInfo(matchResult.username().c_str(), baseInfo, true);
		}
		
		com_protocol::SMultiUserOptResult* optResult = rsp.add_opt_result();
		optResult->set_username(matchResult.username());
		optResult->set_result(rc);
	}
	
	sendPkgMsgToService(rsp, srcSrvId, CommonSrvBusiness::BUSINESS_SET_FF_CHESS_MATCH_RESULT_RSP, "set ff chess user match result reply");
}

