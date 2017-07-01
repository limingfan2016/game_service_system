
/* author : liuxu
* date : 2015.07.02
* description : 消息处理实现
*/
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "MessageDefine.h"
#include "CSrvMsgHandler.h"
#include "SrvFrame/CModule.h"
#include "base/Function.h"

using namespace NProject;


CSrvMsgHandler::CSrvMsgHandler() : m_onlyForTestAsyncData(this, 1024, 8)
{
	m_cur_max_reply_id = 0;
	m_pDBOpt = NULL;
	NEW(m_pMManageCSReplyInfo, CMemManager(1024, 1024, sizeof(CCSReplyInfo)));
	m_serviceIntervalSecs = 1;
	m_optId = 0;
}

CSrvMsgHandler::~CSrvMsgHandler()
{
	if (m_pDBOpt)
	{
		CMySql::destroyDBOpt(m_pDBOpt);
		m_pDBOpt = NULL;
	}

	DELETE(m_pMManageCSReplyInfo);
	m_serviceIntervalSecs = 1;
	m_optId = 0;
}

void CSrvMsgHandler::onTimerMsg(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	//ReleaseInfoLog("--- onTimerMsg ---, timerId = %ld, userId = %d, param = %p, remainCount = %d\n",
	//timerId, userId, param, remainCount);
	if (userId >= (int)m_config.monitor_info_list.size())
	{
		return;
	}

	com_protocol::GetMonitorStatDataReq req;
	req.set_stat_type(m_config.monitor_info_list[userId].stat_type);
	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = 0;
	send_data_len = req.ByteSize();
	req.SerializeToArray(send_data, MaxMsgLen);

	for (int i = 0; i < (int)m_vservice_id.size(); i++)
	{
		if (0 == m_config.monitor_info_list[userId].server_type
			|| m_vservice_id[i] / 1000 == m_config.monitor_info_list[userId].server_type)
		{
			sendMessage(send_data, send_data_len, m_vservice_id[i], GET_MONITOR_STAT_DATA_REQ);
		}
	}

	//自己也写个心跳监控
	if (0 == m_config.monitor_info_list[userId].server_type
		&& 1 == m_config.monitor_info_list[userId].stat_type)
	{
		com_protocol::GetMonitorStatDataRsp rsp;
		rsp.set_stat_type(1);

		send_data_len = rsp.ByteSize();
		rsp.SerializeToArray(send_data, MaxMsgLen);
		handldMsgGetMonitorStatDataRsp(send_data, send_data_len, getSrvId(), 0, 0);
	}
	
}

void CSrvMsgHandler::checkInfo(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	//检测超时连接
	m_manage_user.timeCheck();

	//获取最新客服答复
	getCSReply();

	//向Message查找用户所在游戏
	queryUserGameServer();
	
	// 超时则清空数据
	uint32_t now_timestamp = time(NULL);
	for (OptIdToConnectUser::iterator ite = m_opt2ConnectUser.begin(); ite != m_opt2ConnectUser.end();)
	{
		if (ite->second.curTime + 600 >= now_timestamp)
		{
			m_opt2ConnectUser.erase(ite++);
		}
		else
		{
			ite++;
		}
	}
}


void CSrvMsgHandler::loadConfig()
{
	ReleaseInfoLog("load config.");
	NXmlConfig::CXmlConfig::setConfigValue(CCfg::getValue("Config", "BusinessXmlConfigFile"), m_config);
	m_config.output();

	//清理老的定时器
	for (vector<uint32_t>::iterator ite = m_vstat_timer_id.begin();
		ite != m_vstat_timer_id.end();
		ite++)
	{
		killTimer(*ite);
	}
	m_vstat_timer_id.clear();
	m_vstat_timer_id.resize(m_config.monitor_info_list.size());

	//设置新的定时器
	for (int i = 0; i < (int)m_config.monitor_info_list.size(); i++)
	{
		uint32_t timer_id = setTimer(1000 * m_config.monitor_info_list[i].time_gap, (TimerHandler)&CSrvMsgHandler::onTimerMsg, i, NULL, -1);
		m_vstat_timer_id.push_back(timer_id);
	}

}
	
void CSrvMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onLoad ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
	
	const char* centerRedisSrvItem = "RedisCenterService";
	const char* ip = m_srvMsgCommCfg->get(centerRedisSrvItem, "IP");
	const char* port = m_srvMsgCommCfg->get(centerRedisSrvItem, "Port");
	const char* connectTimeOut = m_srvMsgCommCfg->get(centerRedisSrvItem, "Timeout");
	if (ip == NULL || port == NULL || connectTimeOut == NULL)
	{
		ReleaseErrorLog("redis center service config error");
		stopService();
		return;
	}
	
	if (!m_redisDbOpt.connectSvr(ip, atoi(port), atol(connectTimeOut)))
	{
		ReleaseErrorLog("do connect redis center service failed, ip = %s, port = %s, time out = %s", ip, port, connectTimeOut);
		stopService();
		return;
	}
	
	loadConfig();

	//获取所有需要监控的服务id
	const NCommon::Key2Value *pKey2Value = m_srvMsgCommCfg->getList("ServiceID");
	while (pKey2Value)
	{
		if (0 == strcmp("MaxServiceId", pKey2Value->key))
		{
			pKey2Value = pKey2Value->pNext;
			continue;
		}

		int service_id = atoi(pKey2Value->value);
		int service_type = service_id / 1000;
		if (service_type != ServiceType::ManageSrv)
		{
			m_vservice_id.push_back(service_id);
		}

		pKey2Value = pKey2Value->pNext;
	}

	m_manage_user.load(this);
	
	// 定时向redis获取服务数据，单位秒
	unsigned int getServiceDataIntervalSecs = 60 * 10;
	const char* secsTimeInterval = CCfg::getValue("ServiceData", "GetServiceDataInterval");
	if (secsTimeInterval != NULL) getServiceDataIntervalSecs = atoi(secsTimeInterval);
	setTimer(getServiceDataIntervalSecs * 1000, (TimerHandler)&CSrvMsgHandler::getServiceData, 0, NULL, (unsigned int)-1);
	
	// 游戏服务检测的时间间隔，单位秒
	secsTimeInterval = CCfg::getValue("ServiceData", "GameDataInterval");
	if (secsTimeInterval != NULL) m_serviceIntervalSecs = atoi(secsTimeInterval);
	else m_serviceIntervalSecs = getServiceDataIntervalSecs * 2;
}

void CSrvMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onUnLoad ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
    
	m_redisDbOpt.disconnectSvr();
	
	m_manage_user.unLoad(this);
	ReleaseInfoLog("--- onUnLoad --- all success.");
}
	
void CSrvMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onRegister ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
		
	// 注册协议处理函数
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::GET_MONITOR_STAT_DATA_RSP, (ProtocolHandler)&CSrvMsgHandler::handldMsgGetMonitorStatDataRsp);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::GET_HISTORY_OPINION_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgGetHistoryOpinionReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::UPLOAD_NEW_OPINION_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgUploadNewOpinionReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::USER_ADD_OPINION_ASK_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgUserAddOpinionReq);
	
	registerProtocol(ServiceType::MessageSrv, MessageSrvBusiness::BUSINESS_USER_ENTER_SERVICE_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::handldMsgUserEnterServiceNotify);
	registerProtocol(ServiceType::MessageSrv, MessageSrvBusiness::BUSINESS_USER_QUIT_SERVICE_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::handldMsgUserQuitServiceNotify);
	registerProtocol(ServiceType::MessageSrv, MessageSrvBusiness::BUSINESS_SERVICE_ONLINE_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::serviceOnlineNotify);

	//message srv
	registerProtocol(ServiceType::MessageSrv, MessageSrvBusiness::BUSINESS_GET_USER_SERVICE_ID_RSP, (ProtocolHandler)&CSrvMsgHandler::handldMsgGetUserServiceIDRsp);

	setTimer(1000 * m_config.cs_reply_check_gap, (TimerHandler)&CSrvMsgHandler::checkInfo, 0, NULL, -1);
}
	
void CSrvMsgHandler::onClosedConnect(void* userData)
{
	if (NULL != userData)
	{
		
	}
}

void CSrvMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onRun ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
	//初始化
	if (0 != CMySql::createDBOpt(m_pDBOpt, m_config.mysql_config.mysql_ip.c_str(), m_config.mysql_config.mysql_username.c_str(),
		m_config.mysql_config.mysql_password.c_str(), m_config.mysql_config.mysql_dbname.c_str(), m_config.mysql_config.mysql_port))
	{
		ReleaseErrorLog("CMySql::createDBOpt failed. ip:%s username:%s passwd:%s dbname:%s port:%d", m_config.mysql_config.mysql_ip.c_str(), m_config.mysql_config.mysql_username.c_str(),
			m_config.mysql_config.mysql_password.c_str(), m_config.mysql_config.mysql_dbname.c_str(), m_config.mysql_config.mysql_port);
		stopService();
		return;
	}

	checkInfo(0, 0, NULL, 0);
	
	
	// only for test，测试异步消息本地数据存储
	testAsyncLocalData();
}

void CSrvMsgHandler::handldMsgGetMonitorStatDataRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetMonitorStatDataRsp rsp;
	if (!rsp.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgGetMonitorStatDataRsp--- unpack failed.| len:%u, data:%s", len, data);
		return;
	}

	uint32_t stat_type = rsp.stat_type();
	uint32_t service_id = rsp.has_service_id() ? rsp.service_id() : srcSrvId;
	int64_t value_int64_1 = rsp.has_value_int64_1() ? rsp.value_int64_1() : 0;
	int64_t value_int64_2 = rsp.has_value_int64_2() ? rsp.value_int64_2() : 0;
	int64_t value_int64_3 = rsp.has_value_int64_3() ? rsp.value_int64_3() : 0;
	int64_t value_int64_4 = rsp.has_value_int64_4() ? rsp.value_int64_4() : 0;
	double value_double_1 = rsp.has_value_double_1() ? rsp.value_double_1() : 0.0;
	double value_double_2 = rsp.has_value_double_2() ? rsp.value_double_2() : 0.0;
	string value_string_1 = rsp.has_value_string_1() ? rsp.value_string_1() : "";

	char sql_tmp[2048] = { 0 };
	snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_monitor_run_data(server_id,stat_type,value_int64_1,value_int64_2,value_int64_3,value_int64_4,value_double_1,value_double_2,value_string_1) \
			values(%u,%u,%ld,%ld,%ld,%ld,%lf,%lf,\'%s\');", service_id, stat_type, value_int64_1, value_int64_2,
			value_int64_3, value_int64_4, value_double_1, value_double_2, value_string_1.c_str());
	if (Success == m_pDBOpt->modifyTable(sql_tmp))
	{
		//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql_tmp);
	}
	else
	{
		OptErrorLog("exeSql failed|%u|%u|%s", stat_type, service_id, sql_tmp);
	}
}

void CSrvMsgHandler::handldMsgGetHistoryOpinionReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetHistoryOpinionReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgGetHistoryOpinionReq--- unpack failed.| len:%u, data:%s", len, data);
		return;
	}

	com_protocol::GetHistoryOpinionRsp rsp;
	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = 0;

	char sql_tmp[2048] = { 0 };
	snprintf(sql_tmp, sizeof(sql_tmp), "select id,opinion_type,create_time,opinion_content,add_ask,cs_reply,award_count,award_type from tb_user_opinion where id>=%u and game_type=%u and username=\"%s\" order by id limit %u;",
		req.begin_id(), req.game_type(), getContext().userData, req.max_opinion_count() + 1);
	CQueryResult *p_result = NULL;
	if (Success != m_pDBOpt->queryTableAllResult(sql_tmp, p_result))
	{
		OptErrorLog("exec sql error:%s", sql_tmp);
		m_pDBOpt->releaseQueryResult(p_result);
		rsp.set_result(ServiceSelectOpinionError);
	}
	else
	{
		rsp.set_result(ServiceSuccess);
		if (p_result->getRowCount() > req.max_opinion_count())
		{
			rsp.set_is_get_last(0);
		}
		else
		{
			rsp.set_is_get_last(1);
		}

		RowDataPtr rowData = NULL;
		int cur = 0;
		//填入历史反溃信息
		while ((rowData = p_result->getNextRow()) && ++cur <= (int)(req.max_opinion_count()))
		{
			unsigned long* lengths = p_result->getFieldLengthsOfCurrentRow();
			com_protocol::OneCompleteOpinion* p_one_opinion = rsp.add_complete_opinion_list();
			p_one_opinion->set_opinion_id(atoi(rowData[0]));
			p_one_opinion->set_opinion_type(atoi(rowData[1]));
			//将用户反溃的标题内容添加
			com_protocol::OpinionAskAndReply* p_ask_and_reply = p_one_opinion->add_opinion_list();
			p_ask_and_reply->set_user_type(0);
			struct tm tm_time;
			strptime(rowData[2], "%Y-%m-%d %H:%M:%S", &tm_time);
			p_ask_and_reply->set_timestamp(mktime(&tm_time));
			p_ask_and_reply->set_content(rowData[3]);
			//再将用户追问及客服答复添加
			addAskAndReply(p_one_opinion, rowData[4], lengths[4], rowData[5], lengths[5]);
			p_one_opinion->set_award_count(atoi(rowData[6]));
			p_one_opinion->set_award_type(atoi(rowData[7]));
		}
		m_pDBOpt->releaseQueryResult(p_result);
	}
	
	rsp.SerializeToArray(send_data, MaxMsgLen);
	send_data_len = rsp.ByteSize();
	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::GET_HISTORY_OPINION_RSP);

	return;
}

void CSrvMsgHandler::handldMsgUploadNewOpinionReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UploadNewOpinionReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgUploadNewOpinionReq--- unpack failed.| len:%u, data:%s", len, data);
		return;
	}

	if (!req.has_game_type())
	{
		return;
	}

	com_protocol::UploadNewOpinionRsp rsp;
	rsp.set_result(ServiceSuccess);
	rsp.set_opinion_type(req.opinion_type());
	rsp.set_opinion_content(req.opinion_content());

	//将反溃意见插入表中
	char sql_tmp[2048] = { 0 };
	snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_user_opinion(username,game_type,create_time,deal_status,os_type,phone_model,os_version,game_version,opinion_type,opinion_content,add_ask,cs_reply) \
		values(\'%s\',%u,now(),%u,%u,\'%s\',\'%s\',\'%s\',%u,\'%s\', \'\',\'\');", getContext().userData,req.game_type(), 0, req.os_type(),
		req.has_phone_model() ? req.phone_model().c_str() : "", req.has_os_version() ? req.os_version().c_str() : "", req.game_version().c_str(),
		req.opinion_type(), req.opinion_content().c_str());
	if (Success == m_pDBOpt->modifyTable(sql_tmp))
	{
		//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql_tmp);
	}
	else
	{
		OptErrorLog("exeSql failed|%s|%s", getContext().userData, sql_tmp);
		rsp.set_result(ServiceInsertOpinionFailed);
	}

	//获取刚插入数据的id
	snprintf(sql_tmp, sizeof(sql_tmp), "select last_insert_id(),last_modify_timestamp from tb_user_opinion where id = last_insert_id();");
	CQueryResult *p_result = NULL;
	if (Success != m_pDBOpt->queryTableAllResult(sql_tmp, p_result) || p_result->getRowCount() < 1)
	{
		OptErrorLog("exec sql error:%s", sql_tmp);
		m_pDBOpt->releaseQueryResult(p_result);
		rsp.set_result(ServiceSelectOpinionError);
	}
	else
	{
		RowDataPtr rowData = p_result->getNextRow();
		rsp.set_opinion_id(atoi(rowData[0]));
		struct tm tminfo;
		strptime(rowData[1], "%Y-%m-%d %H:%M:%S", &tminfo);
		rsp.set_timestamp(mktime(&tminfo));
		m_pDBOpt->releaseQueryResult(p_result);
	}

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = 0;
	send_data_len = rsp.ByteSize();
	rsp.SerializeToArray(send_data, MaxMsgLen);
	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::UPLOAD_NEW_OPINION_RSP);

	return;
}

void CSrvMsgHandler::handldMsgUserAddOpinionReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserAddOpinionAskReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgUserAddOpinionReq--- unpack failed.| len:%u, data:%s", len, data);
		return;
	}

	com_protocol::UserAddOpinionAskRsp rsp;
	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = 0;
	rsp.set_result(ServiceSuccess);
	rsp.set_opinion_id(req.opinion_id());
	rsp.set_add_ask(req.add_ask());
	rsp.set_timestamp(time(NULL));

	//先找到该反溃记录
	do 
	{
		com_protocol::AllOpinionAskAndReply ask;
		unsigned int db_time = 0;
		char sql_tmp[2048] = { 0 };
		snprintf(sql_tmp, sizeof(sql_tmp), "select add_ask,now() from tb_user_opinion where id=%u;", req.opinion_id());
		CQueryResult *p_result = NULL;
		if (Success != m_pDBOpt->queryTableAllResult(sql_tmp, p_result) || p_result->getRowCount() < 1)
		{
			OptErrorLog("exec sql error:%s", sql_tmp);
			m_pDBOpt->releaseQueryResult(p_result);
			rsp.set_result(ServiceSelectOpinionError);
			break;
		}
		else
		{
			RowDataPtr rowData = p_result->getNextRow();
			unsigned long* lengths = p_result->getFieldLengthsOfCurrentRow();
			if (!ask.ParseFromArray(rowData[0], lengths[0]))
			{
				m_pDBOpt->releaseQueryResult(p_result);
				break;
			}
			//获取DB当前时间戳
			struct tm tm_time;
			strptime(rowData[1], "%Y-%m-%d %H:%M:%S", &tm_time);
			db_time = mktime(&tm_time);
			rsp.set_timestamp(db_time);
			m_pDBOpt->releaseQueryResult(p_result);
		}

		//添加并Insert到db
		com_protocol::OpinionAskAndReply* po = ask.add_opinion_list();
		po->set_user_type(0);
		po->set_timestamp(db_time);
		po->set_content(req.add_ask());
		send_data_len = ask.ByteSize();
		ask.SerializeToArray(send_data, MaxMsgLen);
		char change_data[MaxMsgLen] = { 0 };
		m_pDBOpt->realEscapeString(change_data, send_data, send_data_len);
		snprintf(sql_tmp, sizeof(sql_tmp), "update tb_user_opinion set add_ask=\"%s\" where id=%u;", change_data, req.opinion_id());
		if (Success == m_pDBOpt->modifyTable(sql_tmp))
		{
			//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql_tmp);
		}
		else
		{
			OptErrorLog("exeSql failed|%s|%s", getContext().userData, sql_tmp);
			rsp.set_result(ServiceInsertOpinionFailed);
		}

	} while (false);

	send_data_len = rsp.ByteSize();
	rsp.SerializeToArray(send_data, MaxMsgLen);
	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::USER_ADD_OPINION_ASK_RSP);

	return;
}

void CSrvMsgHandler::handldMsgGetUserServiceIDRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUserServiceIDRsp rsp;
	if (!rsp.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgGetUserServiceIDRsp--- unpack failed.| len:%u, data:%s", len, data);
		return;
	}

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = 0;
	unordered_map<uint32_t, CCSReplyInfo*>::iterator ite = m_umap_cs_reply.begin();
	for (int i = 0; 
		i < rsp.user_service_lst_size() || ite != m_umap_cs_reply.end(); 
		i++)
	{
		if (!(rsp.user_service_lst(i).username() == ite->second->username))
		{
			break;
		}

		CCSReplyInfo *pcs_reply_info = ite->second;
		switch (pcs_reply_info->game_type)
		{
		case NProject::GameType::BUYU:
			if (rsp.user_service_lst(i).game_service_id() / 1000 == ServiceType::BuyuGameSrv)
			{
				com_protocol::CsReplyOpinionNotify nf;
				nf.set_opinion_id(ite->second->id);
				nf.set_award_count(ite->second->award_count);
				nf.set_award_type(ite->second->award_type);
				com_protocol::OpinionAskAndReply* preply = nf.mutable_reply();
				preply->set_user_type(1);
				preply->set_timestamp(pcs_reply_info->create_time);
				preply->set_content(pcs_reply_info->cs_reply);

				send_data_len = nf.ByteSize();
				nf.SerializeToArray(send_data, MaxMsgLen);
				sendMessage(send_data, send_data_len, rsp.user_service_lst(i).username().c_str(), rsp.user_service_lst(i).username().length(),
					rsp.user_service_lst(i).game_service_id(), CommonSrvBusiness::CS_REPLY_OPINION_NOTIFY);

				m_pMManageCSReplyInfo->put((char*)pcs_reply_info);
				//删除DB记录
				char sql_tmp[2048] = { 0 };
				snprintf(sql_tmp, sizeof(sql_tmp), "delete from tb_opinion_reply where idx=%u;", ite->first);
				if (Success != m_pDBOpt->executeSQL(sql_tmp, strlen(sql_tmp)))
				{
					OptErrorLog("exec sql error:%s", sql_tmp);
				}
				m_umap_cs_reply.erase(ite++);
			}
			else
			{
				ite++;
			}
			break;
		default:
			ite++;
			break;
		}
	}
}

int CSrvMsgHandler::sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* username, const int username_len,
	unsigned short dstModuleId, unsigned short handleProtocolId)
{
	unsigned int srvId = 0;
	NProject::getDestServiceID(DBProxyCommonSrv, username, username_len, srvId);
	return sendMessage(data, len, username, username_len, srvId, protocolId, dstModuleId, handleProtocolId);
}

int CSrvMsgHandler::sendMessageToSrv(ServiceType srvType, const char* data, const unsigned int len, unsigned short protocolId, const char* username, const int username_len,
	int userFlag, unsigned short handleProtocolId)
{
	unsigned int srvId = 0;
	NProject::getDestServiceID(srvType, username, username_len, srvId);
	return sendMessage(data, len, userFlag, username, username_len, srvId, protocolId, 0, handleProtocolId);
}

uint32_t CSrvMsgHandler::getCommonDbProxySrvId(const char *username, const int username_len)
{
	uint32_t srvId = 0;
	NProject::getDestServiceID(DBProxyCommonSrv, username, username_len, srvId);

	return srvId;
}

void CSrvMsgHandler::addAskAndReply(com_protocol::OneCompleteOpinion* p_one, const char* ask_bin, uint32_t ask_len, const char* reply_bin, uint32_t reply_len)
{
	com_protocol::AllOpinionAskAndReply ask, reply;
	if (!ask.ParseFromArray(ask_bin, ask_len) || !reply.ParseFromArray(reply_bin, reply_len))
	{
		OptErrorLog("addAskAndReply unpack error.");
		return;
	}

	int ask_i = 0;
	int reply_i = 0;
	//用时间戳做归并排序
	for ( ;ask_i < ask.opinion_list_size() && reply_i < reply.opinion_list_size();)
	{
		if (ask.opinion_list(ask_i).timestamp() < reply.opinion_list(reply_i).timestamp())
		{
			com_protocol::OpinionAskAndReply* oask = p_one->add_opinion_list();
			oask->set_user_type(0);
			oask->set_timestamp(ask.opinion_list(ask_i).timestamp());
			oask->set_content(ask.opinion_list(ask_i).content());
			ask_i++;
		}
		else
		{
			com_protocol::OpinionAskAndReply* oreply = p_one->add_opinion_list();
			oreply->set_user_type(1);
			oreply->set_timestamp(reply.opinion_list(reply_i).timestamp());
			oreply->set_content(reply.opinion_list(reply_i).content());
			reply_i++;
		}
	}

	//把剩余的加到列表
	for (; ask_i < ask.opinion_list_size(); ask_i++)
	{
		com_protocol::OpinionAskAndReply* oask = p_one->add_opinion_list();
		oask->set_user_type(0);
		oask->set_timestamp(ask.opinion_list(ask_i).timestamp());
		oask->set_content(ask.opinion_list(ask_i).content());
	}

	for (; reply_i < reply.opinion_list_size(); reply_i++)
	{
		com_protocol::OpinionAskAndReply* oreply = p_one->add_opinion_list();
		oreply->set_user_type(1);
		oreply->set_timestamp(reply.opinion_list(reply_i).timestamp());
		oreply->set_content(reply.opinion_list(reply_i).content());
	}
}

void CSrvMsgHandler::getCSReply()
{
	char sql_tmp[2048] = { 0 };
	snprintf(sql_tmp, sizeof(sql_tmp), "select idx,id,username,game_type,UNIX_TIMESTAMP(create_time),cs_name,cs_reply,award_count,award_type from tb_opinion_reply where idx>%u order by idx;", m_cur_max_reply_id);
	CQueryResult *p_result = NULL;
	if (Success != m_pDBOpt->queryTableAllResult(sql_tmp, p_result))
	{
		OptErrorLog("exec sql error:%s", sql_tmp);
		m_pDBOpt->releaseQueryResult(p_result);
	}
	else
	{
		RowDataPtr rowData = NULL;
		//将信息插入map
		while ((rowData = p_result->getNextRow()) != NULL)
		{
			CCSReplyInfo *pcs_reply_info = (CCSReplyInfo *)m_pMManageCSReplyInfo->get();
			pcs_reply_info->idx = atoi(rowData[0]);
			pcs_reply_info->id = atoi(rowData[1]);
			strncpy(pcs_reply_info->username, rowData[2], sizeof(pcs_reply_info->username) - 1);
			pcs_reply_info->game_type = atoi(rowData[3]);
			pcs_reply_info->create_time = atoi(rowData[4]);
			strncpy(pcs_reply_info->cs_name, rowData[5], sizeof(pcs_reply_info->cs_name) - 1);
			strncpy(pcs_reply_info->cs_reply, rowData[6], sizeof(pcs_reply_info->cs_reply) - 1);
			pcs_reply_info->award_count = atoi(rowData[7]);
			pcs_reply_info->award_type = atoi(rowData[8]);

			m_umap_cs_reply[pcs_reply_info->idx] = pcs_reply_info;
			m_cur_max_reply_id = pcs_reply_info->idx;
		}
		m_pDBOpt->releaseQueryResult(p_result);
	}
}

void CSrvMsgHandler::queryUserGameServer()
{
	if (m_umap_cs_reply.size() <= 0)
	{
		return;
	}

	com_protocol::GetUserServiceIDReq req;
	for (unordered_map<uint32_t, CCSReplyInfo*>::iterator ite = m_umap_cs_reply.begin();
		ite != m_umap_cs_reply.end();
		ite++)
	{
		req.add_username_lst(ite->second->username);
	}

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = 0;
	send_data_len = req.ByteSize();
	req.SerializeToArray(send_data, MaxMsgLen);

	sendMessageToSrv(ServiceType::MessageSrv, send_data, send_data_len, MessageSrvBusiness::BUSINESS_GET_USER_SERVICE_ID_REQ, m_umap_cs_reply.begin()->second->username, 
		strlen(m_umap_cs_reply.begin()->second->username));
}


void CSrvMsgHandler::handldMsgUserEnterServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserEnterServiceNotify nf;
	if (!nf.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgUserEnterServiceNotify--- unpack failed.| len:%u, data:%s", len, data);
		return;
	}

	char sql_tmp[2048] = { 0 };
	snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_online_user_data(user_id,server_id,online_time) \
			 values(\'%s\',%u,now());", getContext().userData, nf.service_id());
	if (Success == m_pDBOpt->modifyTable(sql_tmp))
	{
		//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql_tmp);
	}
	else
	{
		OptErrorLog("exeSql failed|%s|%s", getContext().userData, sql_tmp);
	}

	OptInfoLog("UserEnterServiceNotify|%s|%u", getContext().userData, nf.service_id());
}

void CSrvMsgHandler::handldMsgUserQuitServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserQuitServiceNotify nf;
	if (!nf.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgUserQuitServiceNotify--- unpack failed.| len:%u, data:%s", len, data);
		return;
	}

	char sql_tmp[2048] = { 0 };
	snprintf(sql_tmp, sizeof(sql_tmp), "delete from tb_online_user_data where user_id=\'%s\' and server_id=%u;", getContext().userData,nf.service_id());
	if (Success == m_pDBOpt->modifyTable(sql_tmp))
	{
		//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql_tmp);
	}
	else
	{
		OptErrorLog("exeSql failed|%s|%s", getContext().userData, sql_tmp);
	}

	OptInfoLog("UserQuitServiceNotify|%s|%u", getContext().userData, nf.service_id());
}

// 服务启动上线通知
void CSrvMsgHandler::serviceOnlineNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	unsigned int dstSrvId = ntohl(*((unsigned int*)data));
	char sql_tmp[2048] = {0};
	snprintf(sql_tmp, sizeof(sql_tmp) - 1, "update tb_online_service_data set current_online_persons=0 where server_id=\'%d\';delete from tb_online_user_data where server_id=\'%d\';",
			 dstSrvId, dstSrvId);
	
	CQueryResult* qResult = NULL;
	int rc = m_pDBOpt->executeMultiSql(sql_tmp, qResult);
	if (rc == Success)
	{
		m_pDBOpt->releaseQueryResult(qResult);
	}
	else
	{
		OptErrorLog("exeSql failed|%s|%u", sql_tmp, rc);
	}

	OptInfoLog("serviceOnlineNotify|%s|%u", sql_tmp, rc);
}

void CSrvMsgHandler::getServiceData(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	std::vector<std::string> strHallData;
	int rc = m_redisDbOpt.getHAll(HallKey, HallKeyLen, strHallData);
	if (rc != 0)
	{
	    ReleaseErrorLog("get game info from redis error, rc = %d", rc);
		return;
	}
	
	static const unsigned int maxBuffSize = 1024 * 512;
	char sql_tmp[maxBuffSize] = "delete from tb_online_service_data;";
	static const unsigned int deleteSqlLen = strlen(sql_tmp);
	
	const unsigned int serviceTypeBase = 1000;
	char cmd[2048] = {0};
	unsigned int cmdLen = 0;
	const unsigned int currentSecs = time(NULL);
	com_protocol::RoomData roomData;
	for (unsigned int i = 1; i < strHallData.size(); i += 2)
	{
		if (roomData.ParseFromArray(strHallData[i].data(), strHallData[i].length()))
		{
			if ((currentSecs - roomData.update_timestamp()) < m_serviceIntervalSecs)
			{
				cmdLen += snprintf(cmd, sizeof(cmd), "insert into tb_online_service_data(server_id,server_type,server_name,current_online_persons,max_online_persons) values(%u,%u,\'%s\',%u,%u);",
					roomData.id(), roomData.id() / serviceTypeBase, roomData.name().c_str(), roomData.current_persons(), roomData.max_persons());

				if (cmdLen < (maxBuffSize - deleteSqlLen))
				{
					strcat(sql_tmp, cmd);
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			const RedisRoomData* redisRoomData = (const RedisRoomData*)strHallData[i].data();  // 只提取value数据
			if ((currentSecs - redisRoomData->update_timestamp) < m_serviceIntervalSecs)
			{
				cmdLen += snprintf(cmd, sizeof(cmd), "insert into tb_online_service_data(server_id,server_type,server_name,current_online_persons,max_online_persons) values(%u,%u,\'%s\',%u,%u);",
			                   redisRoomData->id, redisRoomData->id / serviceTypeBase, redisRoomData->name, redisRoomData->current_persons, redisRoomData->max_persons);
				if (cmdLen < (maxBuffSize - deleteSqlLen))
				{
					strcat(sql_tmp, cmd);
				}
				else
				{
					break;
				}
			}
		}

		
	}
	
	strHallData.clear();
	rc = m_redisDbOpt.getHAll(LoginListKey, LoginListLen, strHallData);
	if (rc == 0)
	{
		for (unsigned int i = 1; i < strHallData.size(); i += 2)
		{
			unsigned int id = *((unsigned int*)(strHallData[i - 1].data()));
			const LoginServiceData* loginData = (const LoginServiceData*)strHallData[i].data();
			cmdLen += snprintf(cmd, sizeof(cmd), "insert into tb_online_service_data(server_id,server_type,server_name,current_online_persons,max_online_persons) values(%u,%u,\'%s\',%u,%u);",
			                   id, id / serviceTypeBase, "游戏大厅", loginData->current_persons, 0);
			if (cmdLen < (maxBuffSize - deleteSqlLen))
			{
				strcat(sql_tmp, cmd);
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		ReleaseErrorLog("get login info from redis error, rc = %d", rc);
	}
	
	CQueryResult* qResult = NULL;
	rc = m_pDBOpt->executeMultiSql(sql_tmp, qResult);
	if (rc == Success)
	{
		m_pDBOpt->releaseQueryResult(qResult);
	}
	else
	{
		OptErrorLog("exeSql failed|%s|%u", sql_tmp, rc);
	}

	OptInfoLog("getServiceData|%s|%u", sql_tmp, rc);
}

int CSrvMsgHandler::addOptUser(const string& userName)
{
	m_opt2ConnectUser[++m_optId] = ConnectUserInfo(userName, time(NULL));
	return m_optId;
}

int CSrvMsgHandler::addOptUser(const string& userName, const string& data)
{
	m_opt2ConnectUser[++m_optId] = ConnectUserInfo(userName, time(NULL), data);
	return m_optId;
}

string CSrvMsgHandler::getOptUser(int optId, bool isRemove)
{
	string name;
	OptIdToConnectUser::iterator it = m_opt2ConnectUser.find(optId);
	if (it != m_opt2ConnectUser.end())
	{
		name = it->second.name;
		if (isRemove) m_opt2ConnectUser.erase(it);
	}
	
	return name;
}

string CSrvMsgHandler::getOptUser(int optId, string& data, bool isRemove)
{
	string name;
	OptIdToConnectUser::iterator it = m_opt2ConnectUser.find(optId);
	if (it != m_opt2ConnectUser.end())
	{
		name = it->second.name;
		data = it->second.data;
		if (isRemove) m_opt2ConnectUser.erase(it);
	}
	
	return name;
}




// only for test，测试异步消息本地数据存储
union USrvIdFlag
{
	unsigned int value;
	struct SSrvIdFlag
	{
		unsigned short srvId;
		unsigned short flag;
	} srvFlag;
};

// only for test，测试异步消息本地数据存储
struct STestAsyncLocalData
{
	char strValue[100];
	char cValue;
	int iValue;
	short sValue;
	bool bValue;
};

void CSrvMsgHandler::testAsyncLocalData()
{
	registerProtocol(ServiceType::CommonSrv, ETestProtocolId::ETEST_ASYNC_MSG_LOCAL_DATA_REQ, (ProtocolHandler)&CSrvMsgHandler::testAsyncLocalDataRequest);
	registerProtocol(ServiceType::CommonSrv, ETestProtocolId::ETEST_ASYNC_MSG_LOCAL_DATA_RSP, (ProtocolHandler)&CSrvMsgHandler::testAsyncLocalDataReply);
}

void CSrvMsgHandler::testAsyncLocalDataRequest(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (CCfg::getValue("Test", "AsyncLocalData") != NULL)
	{
		// 本地数据必须在发送消息之前存储
		// 直接存储本地数据
		STestAsyncLocalData localData;
		localData.iValue = 1;
		localData.sValue = 2;
		localData.bValue = false;
		localData.cValue = 'M';
		strncpy(localData.strValue, "In CSrvMsgHandler::testAsyncLocalDataRequest", sizeof(localData.strValue) - 1);
			
		m_onlyForTestAsyncData.createData((const char*)&localData, sizeof(localData));  // 这种方式低效，多了一次数据拷贝
	}
	
	USrvIdFlag srvFlagValue;
	srvFlagValue.srvFlag.srvId = srcSrvId;
	srvFlagValue.srvFlag.flag = getContext().userFlag;
	
    sendMessageToSrv(ServiceType::DBProxyCommonSrv, data, len, ETestProtocolId::ETEST_ASYNC_MSG_LOCAL_DATA_REQ, getContext().userData, getContext().userDataLen, srvFlagValue.value);
}

void CSrvMsgHandler::testAsyncLocalDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* testFlag = CCfg::getValue("Test", "AsyncLocalData");
	if (testFlag != NULL)
	{
		const STestAsyncLocalData* localData = (const STestAsyncLocalData*)m_onlyForTestAsyncData.getData();  // 获取本地存储数据
		if (localData != NULL)
		{
			OptInfoLog("Test Async Local Data Reply, iValue = %d, sValue = %d, bValue = %d, cValue = %c, strValue = %s",
			localData->iValue, localData->sValue, localData->bValue, localData->cValue, localData->strValue);
			
			if (*testFlag == '1')
			{
				m_onlyForTestAsyncData.destroyData((const char*)localData);
				OptInfoLog("Test Async Local Data Remove, localData = %p", localData);
			}
		}
		
		OptInfoLog("Test Async Local Data Get Again, localData = %p", m_onlyForTestAsyncData.getData());
	}
	
	USrvIdFlag srvFlagValue;
	srvFlagValue.value = getContext().userFlag;
	
	sendMessage(data, len, srvFlagValue.srvFlag.flag, getContext().userData, getContext().userDataLen, srvFlagValue.srvFlag.srvId, ETestProtocolId::ETEST_ASYNC_MSG_LOCAL_DATA_RSP);
}

