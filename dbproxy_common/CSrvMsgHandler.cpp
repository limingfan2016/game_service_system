
/* author : liuxu
* date : 2015.03.03
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
#include <algorithm>

#include "CLogic.h"
#include "CGameRecord.h"
#include "MessageDefine.h"
#include "_MallConfigData_.h"
#include "CSrvMsgHandler.h"
#include "SrvFrame/CModule.h"
#include "base/Function.h"
#include "../common/CommonType.h"


using namespace NProject;


// 数据记录日志文件
#define DataRecordLog(format, args...)     m_pLogic->getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)


// PK场金币门票期限时间排序函数
static bool pkGoldTicketTimeSort(const PKGoldTicketSortInfo& info1, const PKGoldTicketSortInfo& info2)
{
	return (info1.endSecs < info2.endSecs);
}


CSrvMsgHandler::CSrvMsgHandler() : m_onlyForTestAsyncData(this, 1024, 8)
{
	NEW(m_pMManageUserModifyStat, CMemManager(1024, 1024, sizeof(CUserModifyStat)));
	m_pLogic = new CLogic();
	m_p_game_record = new CGameRecord();

	m_pDBOpt = NULL;
	m_check_timer = (uint32_t)-1;
	m_db_connect_check_timer = (uint32_t)-1;
	m_cur_connect_failed_count = 0;
	m_pMySqlOptDb = NULL;
}

CSrvMsgHandler::~CSrvMsgHandler()
{
	DELETE(m_pMManageUserModifyStat);
	DELETE(m_pLogic);
	DELETE(m_p_game_record);

	if (m_pDBOpt)
	{
		CMySql::destroyDBOpt(m_pDBOpt);
		m_pDBOpt = NULL;
	}
	
	if (m_pMySqlOptDb != NULL)
	{
		CMySql::destroyDBOpt(m_pMySqlOptDb);
		m_pMySqlOptDb = NULL;
	}
}

void CSrvMsgHandler::onTimerMsg(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	ReleaseInfoLog("--- onTimerMsg ---, timerId = %ld, userId = %d, param = %p, remainCount = %d\n",
	timerId, userId, param, remainCount);
}


void CSrvMsgHandler::loadConfig()
{
	ReleaseInfoLog("load config.");
	m_ip2city.init(CCfg::getValue("Config", "city_id_map_file_path"), CCfg::getValue("Config", "ip_city_map_file_path"));
	NXmlConfig::CXmlConfig::setConfigValue(CCfg::getValue("Config", "BusinessXmlConfigFile"), m_config);

	m_senitiveWordFilter.unInit();
	m_senitiveWordFilter.init(CCfg::getValue("Config", "key_words_file_path"));

	if (!m_config.isSetConfigValueSuccess())
	{
		ReleaseErrorLog("set BusinessXmlConfigFile xml config value error");
		stopService();
		return;
	}
	m_config.output();
	
	const MallConfigData::MallData& cfgValue = MallConfigData::MallData::getConfigValue(CCfg::getValue("Config", "mall"), true);
	if (!cfgValue.isSetConfigValueSuccess())
	{
		ReleaseErrorLog("set mall xml config value error");
		stopService();
		return;
	}
	cfgValue.output();

	m_pLogic->updateConfig();
	m_logicHandler.updateConfig();
	m_logicHandlerTwo.updateConfig();
	m_logicHandlerThree.updateConfig();
	m_ffChessLogicHandler.updateConfig();
	
	m_p_game_record->setParam(this);
	m_p_game_record->loadConfig();

	//更新timer
	if ((uint32_t)-1 != m_check_timer)
	{
		killTimer(m_check_timer);
		m_check_timer = setTimer(1000 * m_config.server_config.check_time_gap, (TimerHandler)&CSrvMsgHandler::checkToCommit, 0, NULL, -1);
	}
	
	if ((uint32_t)-1 != m_db_connect_check_timer)
	{
		killTimer(m_db_connect_check_timer);
		m_db_connect_check_timer = setTimer(1000 * m_config.server_config.db_connect_check_time_gap, (TimerHandler)&CSrvMsgHandler::dbConnectCheck, 0, NULL, -1);
	}
}
	
void CSrvMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onLoad ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
	
	loadConfig();

	if (0 != CMySql::createDBOpt(m_pMySqlOptDb, m_config.mysql_opt_db_cfg.mysql_ip.c_str(),
								 m_config.mysql_opt_db_cfg.mysql_username.c_str(),
								 m_config.mysql_opt_db_cfg.mysql_password.c_str(),
								 m_config.mysql_opt_db_cfg.mysql_dbname.c_str(),
								 m_config.mysql_opt_db_cfg.mysql_port))
	{
		ReleaseErrorLog("CMySql::createDBOpt failed. ip:%s username:%s passwd:%s dbname:%s port:%d", m_config.mysql_opt_db_cfg.mysql_ip.c_str(),
						 m_config.mysql_opt_db_cfg.mysql_username.c_str(), m_config.mysql_opt_db_cfg.mysql_password.c_str(),
						 m_config.mysql_opt_db_cfg.mysql_dbname.c_str(), m_config.mysql_opt_db_cfg.mysql_port);
		stopService();
		return;
	}
}

void CSrvMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onUnLoad ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);

    m_logicData.unInit();
	
	unsigned long long start;
	unsigned long long end;
	struct timeval startTime;
	struct timeval endTime;
	gettimeofday(&startTime, NULL);
	start = startTime.tv_sec * 1000000 + startTime.tv_usec;
	time_t now_timestamp = time(NULL);

	int commit_count = m_umapCommitToDB.size();
	int modify_count = m_umapHasModify.size();
	for (unordered_map<string, CUserModifyStat*>::iterator ite = m_umapCommitToDB.begin();
		ite != m_umapCommitToDB.end();)
	{
		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!getUserBaseinfo(ite->first, user_baseinfo, true))
		{
			OptErrorLog("onUnLoad:commitToMysql error.|%s|%u|%u|%u", ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
			m_pMManageUserModifyStat->put((char*)(ite->second));
			m_umapCommitToDB.erase(ite++);
			continue;
		}

		bool ret = updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info);
		updateUserPropInfoToMysql(user_baseinfo.prop_info);
		OptInfoLog("onUnLoad:commitToMysql|%u|%s|%u|%u|%u", ret, ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
		m_pMManageUserModifyStat->put((char*)(ite->second));
		m_umapCommitToDB.erase(ite++);
	}

	for (unordered_map<string, CUserModifyStat*>::iterator ite = m_umapHasModify.begin();
		ite != m_umapHasModify.end();)
	{
		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!getUserBaseinfo(ite->first, user_baseinfo, true))
		{
			OptErrorLog("onUnLoad:commitToMysql error.|%s|%u|%u|%u", ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
			m_pMManageUserModifyStat->put((char*)(ite->second));
			m_umapHasModify.erase(ite++);
			continue;
		}

		bool ret = updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info);
		updateUserPropInfoToMysql(user_baseinfo.prop_info);
		OptInfoLog("onUnLoad:commitToMysql|%u|%s|%u|%u|%u", ret, ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
		m_pMManageUserModifyStat->put((char*)(ite->second));
		m_umapHasModify.erase(ite++);
	}
	mysqlCommit();

	gettimeofday(&endTime, NULL);
	end = endTime.tv_sec * 1000000 + endTime.tv_usec;
	unsigned long long timeuse = end - start;
	int exe_count = commit_count + modify_count;
	ReleaseInfoLog("onUnLoad:commitToMysql time stat(us)|%llu|%u|%u|%u|%llu", timeuse, exe_count, commit_count, modify_count, timeuse / (exe_count ? exe_count : 1));

	bool bret = m_p_game_record->commitAllRecord();
	ReleaseInfoLog("onUnLoad:all game record commitToMysql|%d", bret);

	m_logicRedisDbOpt.disconnectSvr();
	
	ReleaseInfoLog("--- onUnLoad --- all success.");
}
	
void CSrvMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onRegister ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
		
	// 注册协议处理函数
	registerProtocol(ServiceType::CommonSrv,CommonSrvBusiness::BUSINESS_REGISTER_USER_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgRegisterUserReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_MODIFY_PASSWORD_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgModifyPasswordReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_MODIFY_BASEINFO_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgModifyBaseinfoReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_VERIYFY_PASSWORD_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgVeriyfyPasswordReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_USER_BASEINFO_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgGetUserBaseinfoReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_ROUND_END_DATA_CHARGE_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgRoundEndDataChargeReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_LOGIN_NOTIFY_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgLoginNotifyReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_LOGOUT_NOTIFY_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgLogoutNotifyReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_MULTI_USER_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgGetMultiUserInfoReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_PROP_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgGetPropInfoReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_PROP_CHARGE_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgPropChargeReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_USERNAME_BY_IMEI_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgGetUsernameByIMEIReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_USERNAME_BY_PLATFROM_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgGetUsernameByPlatformReq);
	
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_CHANGE_PROP_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgChangeUserPropertyValueReq);

	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_CHANGE_MANY_PEOPLE_PROP_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgChangeManyPeoplePropertyValueReq);

	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GAME_RECORD_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::handldMsgGameRecord);


	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_USER_OTHER_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::getUserOtherInfo);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_RESET_USER_OTHER_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::resetUserOtherInfo);

	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_MANY_USER_OTHER_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::getManyUserOtherInfo);

	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_USER_PROP_AND_PREVENTION_CHEAT_REQ, (ProtocolHandler)&CSrvMsgHandler::getUserPropAndPreventionCheatReq);

	//
	//管理用户
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::MANAGE_MODIFY_PERSON_STATUS_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgMModifyPersonStatusReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::MANAGE_MODIFY_PASSWORD_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgMModifyPasswordReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::MANAGE_MODIFY_BASEINFO_REQ, (ProtocolHandler)&CSrvMsgHandler::handldMsgMModifyBaseinfoReq);

	//
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_PK_PLAY_RANKING_LIST_REQ, (ProtocolHandler)&CSrvMsgHandler::handldPkPlayRankingListReq);
	registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_PK_PLAY_CELEBRITY_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::handldGetPKPlayCelebrityReq);

	//监控统计
	m_monitor_stat.init(this);
}
	
void CSrvMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onRun ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);

	m_memcache.addServer(m_config.server_config.memcached_ip, m_config.server_config.memcached_port);
	if (!m_memcache.set(string("test"), "test", 4, 1, 0))
	{
		ReleaseErrorLog("memcache connect failed. ip:%s port:%d", m_config.server_config.memcached_ip.c_str(), m_config.server_config.memcached_port);
		stopService();
		return ;
	}

	if (0 != CMySql::createDBOpt(m_pDBOpt, m_config.server_config.mysql_ip.c_str(), m_config.server_config.mysql_username.c_str(),
		m_config.server_config.mysql_password.c_str(), m_config.server_config.mysql_dbname.c_str(), m_config.server_config.mysql_port))
	{
		ReleaseErrorLog("CMySql::createDBOpt failed. ip:%s username:%s passwd:%s dbname:%s port:%d", m_config.server_config.mysql_ip.c_str(),
			m_config.server_config.mysql_username.c_str(), m_config.server_config.mysql_password.c_str(), m_config.server_config.mysql_dbname.c_str(), m_config.server_config.mysql_port);
		stopService();
		return;
	}
	m_pDBOpt->autoCommit(0); //关闭自动commit
	//设置mysql字符集（默认utf8，已不需要设置）
	
	const char* logicRedisSrvItem = "RedisLogicService";
	const char* ip = m_srvMsgCommCfg->get(logicRedisSrvItem, "IP");
	const char* port = m_srvMsgCommCfg->get(logicRedisSrvItem, "Port");
	const char* connectTimeOut = m_srvMsgCommCfg->get(logicRedisSrvItem, "Timeout");
	if (ip == NULL || port == NULL || connectTimeOut == NULL)
	{
		ReleaseErrorLog("logic redis service config error");
		stopService();
		return;
	}
	
	if (!m_logicRedisDbOpt.connectSvr(ip, atoi(port), atol(connectTimeOut)))
	{
		ReleaseErrorLog("do connect logic redis service failed, ip = %s, port = %s, time out = %s", ip, port, connectTimeOut);
		stopService();
		return;
	}
	
	if (0 != m_logicData.init(this))
	{
		ReleaseErrorLog("init logic data failed.");
		stopService();
		return;
	}
	
    if (0 != m_pLogic->init(this))
	{
		ReleaseErrorLog("m_pLogic->init failed.");
		stopService();
		return;
	}
	
	int rc = m_logicHandler.init(this);
	if (0 != rc)
	{
		ReleaseErrorLog("init logic handler failed, rc = %d", rc);
		stopService();
		return;
	}
	
	rc = m_logicHandlerTwo.init(this);
	if (0 != rc)
	{
		ReleaseErrorLog("init logic handler two failed, rc = %d", rc);
		stopService();
		return;
	}
	
	rc = m_logicHandlerThree.init(this);
	if (0 != rc)
	{
		ReleaseErrorLog("init logic handler three failed, rc = %d", rc);
		stopService();
		return;
	}
	
	rc = m_ffChessLogicHandler.init(this);
	if (0 != rc)
	{
		ReleaseErrorLog("init ff chess logic handler failed, rc = %d", rc);
		stopService();
		return;
	}
	
	if (!m_p_game_record->init())
	{
		ReleaseErrorLog("m_p_game_record->init failed.");
		stopService();
		return;
	}

	//0.1秒做一次提交Mysql操作（增加频度，减少服务延迟）
	setTimer(100, (TimerHandler)&CSrvMsgHandler::commitToMysql, 0, NULL, -1);
	m_check_timer = setTimer(1000 * m_config.server_config.check_time_gap, (TimerHandler)&CSrvMsgHandler::checkToCommit, 0, NULL, -1);
	m_db_connect_check_timer = setTimer(1000 * m_config.server_config.db_connect_check_time_gap, (TimerHandler)&CSrvMsgHandler::dbConnectCheck, 0, NULL, -1);
	
	
	// only for test，测试异步消息本地数据存储
	testAsyncLocalData();
	
	// 小时定时器，自服务启动开始算，每一小时触发一次
	// const unsigned int hourSeces = 60 * 60;
	// setTimer(1000 * hourSeces, (TimerHandler)&CSrvMsgHandler::hourOnTimer, 0, NULL, -1);

	//批量注册用户
	//batchRegisterUser();
}

CDBOpertaion* CSrvMsgHandler::getMySqlOptDB()
{
	return m_pMySqlOptDb;
}

void CSrvMsgHandler::batchRegisterUser()
{
	/*
	com_protocol::RegisterUserReq req;
	req.set_passwd("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	char username_pre[] = "player";
	char username[64] = { 0 };
	for (int i = 0; i < 100000; i++)
	{
		snprintf(username, sizeof(username), "%s%05d", username_pre, i);
		req.set_username(username);
		req.set_nickname(username);
		int ret = m_pLogic->procRegisterUserReq(req);
		if (ret != Success)
		{
			ReleaseErrorLog("batchRegisterUser ERROR|%s", username);
		}
	}
	*/ 
}
	
void CSrvMsgHandler::handldMsgRegisterUserReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/*
	com_protocol::RegisterUserReq req;
	if (!req.ParseFromArray(data, len))
	{		
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgRegisterUserReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	if (req.has_client_ip())
	{
		string city_name("");
		uint32_t city_id = 0;
		m_ip2city.getCityInfoByIP(req.client_ip().c_str(), city_name, city_id);
		req.set_city_id(city_id);
	}

	int rel = m_pLogic->procRegisterUserReq(req);
	com_protocol::RegisterUserRsp rsp;
	rsp.set_username(req.username());
	rsp.set_result(rel);

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgRegisterUserReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return ;
	}
	
	OptInfoLog("RegisterUser|%u|%d|%s|%s|%s|%s", srcProtocolId, rsp.result(), getContext().userData, req.username().c_str(), req.passwd().c_str(), req.nickname().c_str());
	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_REGISTER_USER_RSP);

	return;
	*/ 
}

void CSrvMsgHandler::handldMsgModifyPasswordReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ModifyPasswordReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgModifyPasswordReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	int rel = m_pLogic->procModifyPasswordReq(req, string(getContext().userData, getContext().userDataLen));
	com_protocol::ModifyPasswordRsp rsp;
	rsp.set_result(rel);

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgModifyPasswordReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return;
	}

	OptInfoLog("ModifyPassword|%u|%d|%s|%d|%s|%s", srcProtocolId, rsp.result(), getContext().userData, req.is_reset(), req.old_passwd().c_str(), req.new_passwd().c_str());
	
	if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_MODIFY_PASSWORD_RSP;
	sendMessage(send_data, send_data_len, srcSrvId, srcProtocolId);

	return;
}

void CSrvMsgHandler::handldMsgModifyBaseinfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ModifyBaseinfoReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgModifyBaseinfoReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::ModifyBaseinfoRsp rsp;
	m_pLogic->procModifyBaseinfoReq(req, string(getContext().userData, getContext().userDataLen), rsp);

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgModifyBaseinfoReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return;
	}

	OptInfoLog("ModifyBaseinfo|%u|%d|%s", srcProtocolId, rsp.result(), getContext().userData);
	
	if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_MODIFY_BASEINFO_RSP;
	sendMessage(send_data, send_data_len, srcSrvId, srcProtocolId);

	return;
}

void CSrvMsgHandler::handldMsgVeriyfyPasswordReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::VerifyPasswordReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgVeriyfyPasswordReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	int rel = m_pLogic->procVeriyfyPasswordReq(req);
	com_protocol::VerifyPasswordRsp rsp;
	rsp.set_result(rel);
	rsp.set_username(req.username());

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgVeriyfyPasswordReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return;
	}

	OptInfoLog("VerifyPassword|%u|%d|%s|%s|%s", srcProtocolId, rsp.result(), getContext().userData, req.passwd().c_str(),req.username().c_str());
	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_VERIYFY_PASSWORD_RSP);

	return;
}

void CSrvMsgHandler::handldMsgGetUserBaseinfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUserBaseinfoReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgGetUserBaseinfoReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::GetUserBaseinfoRsp rsp;
	rsp.set_query_username(req.query_username());
	com_protocol::DetailInfo *pdetail_info = rsp.mutable_detail_info();
	int rel = m_pLogic->procGetUserBaseinfoReq(req, *pdetail_info);
	rsp.set_result(rel);
	if (0 != rel)
	{
		rsp.clear_detail_info();
	}
	else if (req.has_data_type())
	{
		getLogicHander().getLogicData(req, rsp);  // 从redis获取逻辑数据
	}

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgGetUserBaseinfoReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return;
	}

	// OptInfoLog("GetUserBaseinfo|%u|%d|%s|%s", srcProtocolId, rsp.result(), getContext().userData, req.query_username().c_str());
	if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_GET_USER_BASEINFO_RSP;
	sendMessage(send_data, send_data_len, srcSrvId, srcProtocolId); // CommonSrvBusiness::BUSINESS_GET_USER_BASEINFO_RSP, 0, srcProtocolId);
	
	/*
	OptWarnLog("get base info from message, request user = %s, id = %s, name = %s, game_gold = %lld, rmb_gold = %u, diamonds_number = %u, phone_fare = %u, voucher_number = %u, sum_recharge = %u, rc = %d",
	req.query_username().c_str(), pdetail_info->static_info().username().c_str(), pdetail_info->static_info().nickname().c_str(), pdetail_info->dynamic_info().game_gold(),
	pdetail_info->dynamic_info().rmb_gold(), pdetail_info->dynamic_info().diamonds_number(), pdetail_info->dynamic_info().phone_fare(), pdetail_info->dynamic_info().voucher_number(),
	pdetail_info->dynamic_info().sum_recharge(), rc);
    */
	
	return;
}

void CSrvMsgHandler::handldMsgRoundEndDataChargeReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::RoundEndDataChargeReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgRoundEndDataChargeReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

    uint64_t gameGold = 0;
	int rel = m_pLogic->procRoundEndDataChargeReq(req, gameGold);
	com_protocol::RoundEndDataChargeRsp rsp;
	rsp.set_result(rel);

	char send_data[MaxMsgLen] = { 0 };
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgRoundEndDataChargeReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return;
	}
	
    if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_ROUND_END_DATA_CHARGE_RSP;
	rel = sendMessage(send_data, rsp.ByteSize(), srcSrvId, srcProtocolId);
	
	OptInfoLog("RoundEndDataCharge|%u|%d|%d|%d|%s|%u|%d|%d|%d|%d|%d|%d|%ld|%lu", srcSrvId, srcProtocolId, rsp.result(), rel, getContext().userData, req.game_id(), req.delta_exp(),
		req.delta_vip_exp(), req.delta_score(), req.delta_rmb_gold(), req.delta_voucher_number(), req.delta_phone_card_number(), req.delta_game_gold(), gameGold);
	return;
}

void CSrvMsgHandler::handldMsgLoginNotifyReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::LoginNotifyReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgLoginNotifyReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	int rel = m_pLogic->procLoginNotifyReq(req);
	com_protocol::LoginNotifyRsp rsp;
	rsp.set_result(rel);

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgLoginNotifyReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return;
	}

	OptInfoLog("LoginNotify|%u|%d|%s|%u", srcProtocolId, rsp.result(), getContext().userData, req.login_timestamp());
	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_LOGIN_NOTIFY_RSP);

	return;
}

void CSrvMsgHandler::handldMsgLogoutNotifyReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::LogoutNotifyReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgLogoutNotifyReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	int rel = m_pLogic->procLogoutNotifyReq(req);
	com_protocol::LogoutNotifyRsp rsp;
	rsp.set_result(rel);

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgLogoutNotifyReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return;
	}

	OptInfoLog("LogoutNotify|%u|%d|%s|%d|%u|%u", srcProtocolId, rsp.result(), getContext().userData, req.logout_mode(), req.online_time(), req.logout_timestamp());
	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_LOGOUT_NOTIFY_RSP);
	return;
}

void CSrvMsgHandler::handldMsgGetMultiUserInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetMultiUserInfoReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgGetMultiUserInfoReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::GetMultiUserInfoRsp rsp;
	if (req.has_flag()) rsp.set_flag(req.flag());
	m_pLogic->procGetMultiUserInfoReq(req, rsp);

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgGetMultiUserInfoReq--- pack failed.| userData:%s", getContext().userData);
		return;
	}

	// OptInfoLog("GetMultiUserInfo|%u|%s", srcProtocolId,  getContext().userData);
	if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_GET_MULTI_USER_INFO_RSP;
	sendMessage(send_data, send_data_len, srcSrvId, srcProtocolId);
	return;
}


void CSrvMsgHandler::handldMsgGetPropInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetPropInfoReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgGetPropInfoReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::GetPropInfoRsp rsp;
	rsp.set_query_username(req.query_username());
	CUserBaseinfo user_baseinfo;
	if (!getUserBaseinfo(req.query_username(), user_baseinfo))
	{
		rsp.set_result(ServiceGetUserinfoFailed);
	}
	else
	{
		rsp.set_result(0);
		com_protocol::PropInfo* pprop_info = rsp.mutable_prop_info();
		pprop_info->set_light_cannon_count(user_baseinfo.prop_info.light_cannon_count);
		pprop_info->set_mute_bullet_count(user_baseinfo.prop_info.mute_bullet_count);
		pprop_info->set_flower_count(user_baseinfo.prop_info.flower_count);
		pprop_info->set_slipper_count(user_baseinfo.prop_info.slipper_count);
		pprop_info->set_suona_count(user_baseinfo.prop_info.suona_count);
	}

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	rsp.SerializeToArray(send_data, MaxMsgLen);

	OptInfoLog("GetPropInfo|%u|%s|%s", srcProtocolId, getContext().userData, req.query_username().c_str());
	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_GET_PROP_INFO_RSP);
	return;
}

void CSrvMsgHandler::handldMsgPropChargeReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::PropChargeReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgPropChargeReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::PropChargeRsp rsp;
	CUserBaseinfo user_baseinfo;
	if (req.has_oper_type()) rsp.set_oper_type(req.oper_type());
	if (req.has_dst_username()) rsp.set_dst_username(req.dst_username());

	(getUserBaseinfo(string(getContext().userData, getContext().userDataLen), user_baseinfo)) ? rsp.set_result(0) : rsp.set_result(ServiceGetUserinfoFailed);
	
	while (rsp.result() == 0)
	{
		int64_t tmp = 0;
		if (req.has_delta_light_cannon_count())
		{
			tmp = (int64_t)user_baseinfo.prop_info.light_cannon_count + req.delta_light_cannon_count();
			if (tmp < 0)
			{
				rsp.set_result(ServiceLightCannonNotEnought);
				break;
			}
			user_baseinfo.prop_info.light_cannon_count = (uint32_t)tmp;
			
			com_protocol::PropItem* pItem = rsp.add_items();
			pItem->set_type(EPropType::PropLightCannon);
			pItem->set_count(user_baseinfo.prop_info.light_cannon_count);
		}

		if (req.has_delta_mute_bullet_count())
		{
			tmp = (int64_t)user_baseinfo.prop_info.mute_bullet_count + req.delta_mute_bullet_count();
			if (tmp < 0)
			{
				rsp.set_result(ServiceMuteBulletNotEnought);
				break;
			}
			user_baseinfo.prop_info.mute_bullet_count = (uint32_t)tmp;
			
			com_protocol::PropItem* pItem = rsp.add_items();
			pItem->set_type(EPropType::PropMuteBullet);
			pItem->set_count(user_baseinfo.prop_info.mute_bullet_count);
		}

		if (req.has_delta_flower_count())
		{
			tmp = (int64_t)user_baseinfo.prop_info.flower_count + req.delta_flower_count();
			if (tmp < 0)
			{
				rsp.set_result(ServiceFlowerNotEnought);
				break;
			}
			user_baseinfo.prop_info.flower_count = (uint32_t)tmp;
			
			com_protocol::PropItem* pItem = rsp.add_items();
			pItem->set_type(EPropType::PropFlower);
			pItem->set_count(user_baseinfo.prop_info.flower_count);
		}

		if (req.has_delta_slipper_count())
		{
			tmp = (int64_t)user_baseinfo.prop_info.slipper_count + req.delta_slipper_count();
			if (tmp < 0)
			{
				rsp.set_result(ServiceSlipperNotEnought);
				break;
			}
			user_baseinfo.prop_info.slipper_count = (uint32_t)tmp;
			
			com_protocol::PropItem* pItem = rsp.add_items();
			pItem->set_type(EPropType::PropSlipper);
			pItem->set_count(user_baseinfo.prop_info.slipper_count);
		}

		if (req.has_delta_suona_count())
		{
			tmp = (int64_t)user_baseinfo.prop_info.suona_count + req.delta_suona_count();
			if (tmp < 0)
			{
				rsp.set_result(ServiceSuonaNotEnought);
				break;
			}
			user_baseinfo.prop_info.suona_count = (uint32_t)tmp;
			
			com_protocol::PropItem* pItem = rsp.add_items();
			pItem->set_type(EPropType::PropSuona);
			pItem->set_count(user_baseinfo.prop_info.suona_count);
		}
		
		//将数据同步到memcached
		if (setUserBaseinfoToMemcached(user_baseinfo))
		{
			//有修改，更新状态
			updateOptStatus(string(getContext().userData, getContext().userDataLen), UpdateOpt::Modify);
			
			if (req.has_game_record())
			{
				m_p_game_record->procGameRecord(req.game_record(), &user_baseinfo);
			}
		}
		else
		{
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
		}

		break;
	}

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	rsp.SerializeToArray(send_data, MaxMsgLen);
	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_PROP_CHARGE_RSP);
	
	if (rsp.result() != 0) OptErrorLog("PropCharge|%s|%d", getContext().userData, rsp.result());
	//OptInfoLog("PropCharge|%u|%s|%d", srcProtocolId, getContext().userData, rsp.result());
	
	return;
}

void CSrvMsgHandler::handldMsgGetUsernameByIMEIReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUsernameByIMEIReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgGetUsernameByIMEIReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::GetUsernameByIMEIRsp rsp;
	rsp.set_result(ServiceSuccess);
	do 
	{
		//验证imei合法性
		if (req.imei().length() < 10 || req.imei().length() > 64)
		{
			rsp.set_result(ServiceIMEIUnlegal);
			break;
		}

		//先从memcached查看是否存在
		vector<char> ret_val;
		uint32_t flags = 0;
		if (m_memcache.get(string("00") + req.imei(), ret_val, flags))
		{
			rsp.set_username(&ret_val[0], ret_val.size());
			break;
		}

		//再找找DB
		char sql_tmp[2048] = { 0 };
		CQueryResult *p_result = NULL;
		snprintf(sql_tmp, sizeof(sql_tmp), "select username from tb_user_platform_map_info where platform_id=\'%s\' and platform_type=0;", req.imei().c_str());
		if (Success == m_pDBOpt->queryTableAllResult(sql_tmp, p_result)
			&& 1 == p_result->getRowCount())
		{
			rsp.set_username(p_result->getNextRow()[0]);
			m_pDBOpt->releaseQueryResult(p_result);
			//缓存到memcached
			m_memcache.set(string("00") + req.imei(), rsp.username().c_str(), rsp.username().length(), 0, flags);
			break;
		}
		m_pDBOpt->releaseQueryResult(p_result);
		
		//都没有，注册一个
		string username("");
		string city_name("");
		uint32_t city_id = 0;
		if (req.has_client_ip())
		{
			m_ip2city.getCityInfoByIP(req.client_ip().c_str(), city_name, city_id);
		}

		int ret = autoRegister(username, req.imei(), 0, req.os_type(), username, city_id);
		if (0 == ret)
		{
			rsp.set_username(username);
			break;
		}
		rsp.set_result(ret);
	} while (false);
	
	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	rsp.SerializeToArray(send_data, MaxMsgLen);

	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_GET_USERNAME_BY_IMEI_RSP);
}

bool CSrvMsgHandler::handleTXQQPlatformatId(com_protocol::GetUsernameByPlatformReq& req, string& username)
{
	// 应用宝QQ登录，兼容新老版本用户，解决老版本使用qq昵称作为平台ID问题
	if (req.platform_type() != ThirdPlatformType::TXQQ || !req.has_other_id() || req.other_id().empty()) return false;

    bool isOldVersionId = false;
	const string* tXQQPlatformatId[] = {&(req.other_id()), &(req.platform_id())};  // 必须第一优先查找other id
	for (unsigned int idx = 0; idx < sizeof(tXQQPlatformatId) / sizeof(tXQQPlatformatId[0]); ++idx)
	{
		const string& platformatId = *(tXQQPlatformatId[idx]);
		
		// 先从memcached查看是否存在
		vector<char> ret_val;
		uint32_t flags = 0;
		char pre_str[16] = { 0 };
		snprintf(pre_str, sizeof(pre_str)-1, "%02u", req.platform_type());
		const string userNameKey = string(pre_str) + platformatId;
		if (m_memcache.get(userNameKey, ret_val, flags))
		//char* ret_val = NULL;
		//unsigned int valueLen = 0;
		//if (m_userNameCache.get(userNameKey.c_str(), userNameKey.length(), ret_val, valueLen))
		{
			username = string(&ret_val[0], ret_val.size());
			// username = string(ret_val, valueLen);

			OptInfoLog("TXQQ platformat, get username from memcache, pre_str = %s, platform id = %s, type = %u, key = %s, username = %s",
			pre_str, platformatId.c_str(), req.platform_type(), userNameKey.c_str(), username.c_str());
			if (idx == 0) return true;  // 已经是正确的ID了
			
			isOldVersionId = true;
			break;  // 老版本的ID，QQ昵称
		}
			
		// 再看下db是否已经保存正确的ID
		char sql_tmp[2048] = { 0 };
		CQueryResult *p_result = NULL;
		snprintf(sql_tmp, sizeof(sql_tmp), "select username from tb_user_platform_map_info where platform_id=\'%s\' and platform_type=%u;", platformatId.c_str(), req.platform_type());
		if (Success == m_pDBOpt->queryTableAllResult(sql_tmp, p_result)
			&& 1 == p_result->getRowCount())
		{
			username = p_result->getNextRow()[0];
			m_pDBOpt->releaseQueryResult(p_result);
			
			// 缓存到memcached
			bool isOK = m_memcache.set(userNameKey, username.c_str(), username.length(), 0, flags);
			// bool isOK = m_userNameCache.set(userNameKey.c_str(), userNameKey.length(), username.c_str(), username.length());
			
			OptInfoLog("TXQQ platformat, get username from mysql table tb_user_platform_map_info, pre_str = %s, platform id = %s, type = %u, key = %s, username = %s, set to memcache = %d",
			pre_str, platformatId.c_str(), req.platform_type(), userNameKey.c_str(), username.c_str(), isOK);
			if (idx == 0) return true;  // 已经是正确的ID了
			
			isOldVersionId = true;
			break;  // 老版本的ID，QQ昵称
		}
		m_pDBOpt->releaseQueryResult(p_result);
	}
	
	if (isOldVersionId)
	{
		// 更新老版本的ID为新的other id值
		// update tb_user_platform_map_info set platform_id='NewOtherID' where username='10000169' and platform_type=14;
		char updateSql[2048] = {0};
		unsigned int sqlLen = snprintf(updateSql, sizeof(updateSql) - 1, "update tb_user_platform_map_info set platform_id=\'%s\' where username=\'%s\' and platform_type=%u;",
		req.other_id().c_str(), username.c_str(), req.platform_type());
		int result = m_pDBOpt->executeSQL(updateSql, sqlLen);
		
		OptWarnLog("TXQQ platformat, update platform id = %s, type = %u, username = %s, old platform id = %s, result = %d",
		req.other_id().c_str(), req.platform_type(), username.c_str(), req.platform_id().c_str(), result);
		
		return true;
	}
	
	req.set_platform_id(req.other_id());  // 新注册的QQ号，则使用other id作为平台ID
	return false;
}

void CSrvMsgHandler::handldMsgGetUsernameByPlatformReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUsernameByPlatformReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgGetUsernameByPlatformReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::GetUsernameByPlatformRsp rsp;
	rsp.set_result(ServiceSuccess);
	do
	{
		string txQQUserName;
		if (handleTXQQPlatformatId(req, txQQUserName))
		{
			rsp.set_username(txQQUserName);
			break;
		}
		
		//验证platform_id合法性
		if (req.platform_id().length() > 64)
		{
			rsp.set_result(ServicePlatformIdUnlegal);
			break;
		}

		//先从memcached查看是否存在
		vector<char> ret_val;
		uint32_t flags = 0;
		char pre_str[16] = { 0 };
		snprintf(pre_str, sizeof(pre_str)-1, "%02u", req.platform_type());
		if (m_memcache.get(string(pre_str) + req.platform_id(), ret_val, flags))
		{
			rsp.set_username(&ret_val[0], ret_val.size());
			break;
		}

		//再找找DB
		char sql_tmp[2048] = { 0 };
		CQueryResult *p_result = NULL;
		snprintf(sql_tmp, sizeof(sql_tmp), "select username from tb_user_platform_map_info where platform_id=\'%s\' and platform_type=%u;", req.platform_id().c_str(), req.platform_type());
		if (Success == m_pDBOpt->queryTableAllResult(sql_tmp, p_result)
			&& 1 == p_result->getRowCount())
		{
			rsp.set_username(p_result->getNextRow()[0]);
			m_pDBOpt->releaseQueryResult(p_result);
			//缓存到memcached
			m_memcache.set(string(pre_str) + req.platform_id(), rsp.username().c_str(), rsp.username().length(), 0, flags);
			break;
		}
		m_pDBOpt->releaseQueryResult(p_result);

		//都没有，注册一个
		string username("");
		string nickname("");
		if (req.has_nickname())
		{
			nickname = utf8SubStr(req.nickname().c_str(), 0, m_config.common_cfg.nickname_length);
		}
		string city_name("");
		uint32_t city_id = 0;
		if (req.has_client_ip())
		{
			m_ip2city.getCityInfoByIP(req.client_ip().c_str(), city_name, city_id);
		}

		int ret = autoRegister(username, req.platform_id(), req.platform_type(), req.os_type(), nickname, city_id, req.has_gender() ? req.gender() : 0);
		if (0 == ret)
		{
			rsp.set_username(username);
			
			OptInfoLog("auto register username, pre_str = %s, platform id = %s, type = %u, username = %s",
			pre_str, req.platform_id().c_str(), req.platform_type(), rsp.username().c_str());
			
			break;
		}
		rsp.set_result(ret);
		
		OptErrorLog("auto register username error, ret = %d", ret);
		
	} while (false);

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	rsp.SerializeToArray(send_data, MaxMsgLen);

	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_GET_USERNAME_BY_PLATFROM_RSP);
}

std::string CSrvMsgHandler::utf8SubStr(const char* str, int start, int num)
{
	if ((int)strlen(str) <= (start + num)){
		return std::string(str, start, strlen(str));
	}

	int realNum = num;
	char _last = str[start + realNum - 1];

	while ((_last >> 7) == 1)
	{
		if (--realNum <= 0)
			break;

		_last = str[start + realNum - 1];
	}


	return std::string(str, start, start + realNum);
}

void CSrvMsgHandler::handldMsgMModifyPersonStatusReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ModifyPersonStatusReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgMModifyPersonStatusReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::ModifyPersonStatusRsp rsp;
	rsp.set_result(0);
	rsp.set_modify_username(req.modify_username());

	do 
	{
		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!getUserBaseinfo(req.modify_username(), user_baseinfo))
		{
			rsp.set_result(ServiceGetUserinfoFailed); //获取DB中的信息失败
			break;
		}

		user_baseinfo.static_info.status = req.status();

		//将数据写入DB
		if (updateUserStaticBaseinfoToMysql(user_baseinfo.static_info))
		{
			if (setUserBaseinfoToMemcached(user_baseinfo))
			{
				mysqlCommit();
			}
			else
			{
				mysqlRollback();
				rsp.set_result(ServiceUpdateDataToMemcachedFailed);
				break;
			}
		}
		else
		{
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}
	} while (false);

	//返回
	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgMModifyPersonStatusReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return;
	}

	OptInfoLog("MModifyPersonStatus|%u|%s|%s|%u", rsp.result(), getContext().userData, req.modify_username().c_str(), req.status());
	
	if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::MANAGE_MODIFY_PERSON_STATUS_RSP;
	sendMessage(send_data, send_data_len, srcSrvId, srcProtocolId);
}

void CSrvMsgHandler::handldMsgMModifyPasswordReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ManageModifyPasswordReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgMModifyPasswordReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	int rel = m_pLogic->procModifyPasswordReq(req.mreq(), req.username());
	com_protocol::ManageModifyBaseinfoRsp rsp;
	rsp.set_result(rel);

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgMModifyPasswordReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return;
	}

	OptInfoLog("ManageModifyPassword|%u|%d|%s|%d|%s|%s", srcProtocolId, rsp.result(), getContext().userData, req.mreq().is_reset(), req.mreq().old_passwd().c_str(), req.mreq().new_passwd().c_str());
	sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::MANAGE_MODIFY_PASSWORD_RSP);
}

void CSrvMsgHandler::handldMsgMModifyBaseinfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ManageModifyBaseinfoReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgModifyBaseinfoReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	int rel = m_pLogic->procModifyBaseinfoReq(req.mreq(), req.username());
	com_protocol::ManageModifyBaseinfoRsp rsp;
	rsp.set_result(rel);

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- handldMsgMModifyBaseinfoReq--- pack failed.| userData:%s, result:%d", getContext().userData, rsp.result());
		return;
	}

	OptInfoLog("ManageModifyBaseinfo|%u|%d|%s", srcProtocolId, rsp.result(), getContext().userData);
	
	if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::MANAGE_MODIFY_BASEINFO_RSP;
	sendMessage(send_data, send_data_len, srcSrvId, srcProtocolId);

	return;
}

int CSrvMsgHandler::autoRegister(string &username, const string& platform_id, uint32_t platform_type, uint32_t os_type, const string &nickname, uint32_t city_id, int gender)
{
	//先找出最大ID
	unsigned int cur_id = 0;
	char sql_tmp[2048] = { 0 };
	CQueryResult *p_result = NULL;
	snprintf(sql_tmp, sizeof(sql_tmp), "select max(id) from tb_user_platform_map_info;");
	if (Success != m_pDBOpt->queryTableAllResult(sql_tmp, p_result)
		|| 1 != p_result->getRowCount())
	{
		m_pDBOpt->releaseQueryResult(p_result);
		return ServiceGetMaxIdFailed;
	}
	RowDataPtr rowData = p_result->getNextRow();
	cur_id = atoi(rowData[0] ? rowData[0] : "0");
	m_pDBOpt->releaseQueryResult(p_result);
	cur_id++;

	//注册用户
	int ret = 0;
	for (int i = 0; i < 3; i++)
	{
		com_protocol::RegisterUserReq rreq;
		snprintf(sql_tmp, sizeof(sql_tmp), "10%06u", cur_id);
		rreq.set_username(sql_tmp);
		if (nickname.length() > 0)
		{
			rreq.set_nickname(nickname);
		}
		else
		{
			rreq.set_nickname(sql_tmp);
		}
		rreq.set_passwd("E10ADC3949BA59ABBE56E057F20F883E");

		if (0 != city_id)
		{
			rreq.set_city_id(city_id);
		}
		
		rreq.set_sex(gender);
		
		ret = m_pLogic->procRegisterUserReq(rreq, platform_type);
		
		//注册成功
		if (ServiceSuccess == ret)
		{
			//插入第三方ID映射信息
			username = sql_tmp;
			snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_user_platform_map_info(platform_id,platform_type,username,id,create_time,os_type) values(\'%s\',%u,\'%s\',%u,now(),%u);", 
				platform_id.c_str(), platform_type, username.c_str(), cur_id, os_type);
			if (Success == m_pDBOpt->modifyTable(sql_tmp))
			{
				mysqlCommit();
				OptInfoLog("auto register user = %s, nickname = %s, platform id = %s, type = %d",
				rreq.username().c_str(), rreq.nickname().c_str(), platform_id.c_str(), platform_type);
			}
			else
			{
				mysqlRollback();
				OptErrorLog("exeSql failed|%s|%s", username.c_str(), sql_tmp);
				return ServiceInsertFailed;
			}
			break;
		}
		cur_id++;
	}

	return ret;
}

void CSrvMsgHandler::updateOptStatus(const string& username, UpdateOpt upate_opt)
{
	unordered_map<string, CUserModifyStat*>::iterator ite = m_umapCommitToDB.find(username);
	if (ite != m_umapCommitToDB.end())
	{
		ite->second->modify_count++;
		if(upate_opt == Logout)
		{
			ite->second->is_logout = true;
		}
		return;
	}

	ite = m_umapHasModify.find(username);
	if (ite != m_umapHasModify.end())
	{
		ite->second->modify_count++;
		if (upate_opt == Logout)
		{
			ite->second->is_logout = true;
			m_umapCommitToDB[username] = ite->second;
			m_umapHasModify.erase(ite);
		}
		else if (ite->second->modify_count >= (uint32_t)m_config.server_config.commit_need_counts)
		{
			m_umapCommitToDB[username] = ite->second;
			m_umapHasModify.erase(ite);
		}
		return;
	}

	CUserModifyStat *puser_modify_stat = (CUserModifyStat *)m_pMManageUserModifyStat->get();
	puser_modify_stat->first_modify_timestamp = time(NULL);
	if (upate_opt == Logout)
	{
		puser_modify_stat->is_logout = true;
		puser_modify_stat->modify_count = 0;
		m_umapCommitToDB[username] = puser_modify_stat;
	}
	else
	{
		puser_modify_stat->modify_count = 1;
		puser_modify_stat->is_logout = false;
		m_umapHasModify[username] = puser_modify_stat;
	}

	return;
}

void CSrvMsgHandler::commitToMysql(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	if (0 == m_umapCommitToDB.size())
	{
		return;
	}

	unsigned long long start;
	unsigned long long end;
	struct timeval startTime;
	struct timeval endTime;
	gettimeofday(&startTime, NULL);
	start = startTime.tv_sec * 1000000 + startTime.tv_usec;

	time_t now_timestamp = time(NULL);
	int count = (int)m_config.server_config.max_commit_count_per_second;
	for (unordered_map<string, CUserModifyStat*>::iterator ite = m_umapCommitToDB.begin();
		count > 0 && ite != m_umapCommitToDB.end();
		count--)
	{
		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!getUserBaseinfo(ite->first, user_baseinfo, true))
		{
			OptErrorLog("commitToMysql error.|%s|%u|%u|%u", ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
			m_pMManageUserModifyStat->put((char*)(ite->second));
			m_umapCommitToDB.erase(ite++);
			continue;
		}

		bool ret = updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info);
		updateUserPropInfoToMysql(user_baseinfo.prop_info);
		OptInfoLog("commitToMysql|%u|%s|%u|%u|%u", ret, ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
		m_pMManageUserModifyStat->put((char*)(ite->second));
		m_umapCommitToDB.erase(ite++);
	}

	if (count < (int)m_config.server_config.max_commit_count_per_second)
	{
		mysqlCommit();

		gettimeofday(&endTime, NULL);
		end = endTime.tv_sec * 1000000 + endTime.tv_usec;
		unsigned long long timeuse = end - start;
		int exe_count = m_config.server_config.max_commit_count_per_second - count;
		ReleaseInfoLog("commitToMysql time stat(us)|%llu|%u|%llu", timeuse, exe_count, timeuse / exe_count);
	}
	
	return;
}

void CSrvMsgHandler::checkToCommit(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	unsigned long long start;
	unsigned long long end;
	struct timeval startTime;
	struct timeval endTime;
	gettimeofday(&startTime, NULL);
	start = startTime.tv_sec * 1000000 + startTime.tv_usec;

	unsigned int count = m_umapHasModify.size();
	unsigned int cur_timestamp = time(NULL);
	for (unordered_map<string, CUserModifyStat*>::iterator ite = m_umapHasModify.begin();
		ite != m_umapHasModify.end();)
	{
		if (cur_timestamp - ite->second->first_modify_timestamp >= (uint32_t)m_config.server_config.commit_need_time)
		{
			m_umapCommitToDB[ite->first] = ite->second;
			m_umapHasModify.erase(ite++);
		}
		else
		{
			ite++;
		}
	}
	m_p_game_record->checkCommitRecord();

	gettimeofday(&endTime, NULL);
	end = endTime.tv_sec * 1000000 + endTime.tv_usec;
	unsigned long long timeuse = end - start;
	ReleaseInfoLog("checkToCommit && checkCommitRecord time stat(us)|%llu|%u|%u|%u", timeuse, count, m_umapHasModify.size(), m_umapCommitToDB.size());

	return;
}

void CSrvMsgHandler::dbConnectCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	if (!dbIsConnect())
	{
		m_cur_connect_failed_count++;
		setTimer(1000, (TimerHandler)&CSrvMsgHandler::dbHasDisconnectCheck, 0, NULL, 1);
	}
	else
	{
		m_cur_connect_failed_count = 0;
	}
}

void CSrvMsgHandler::dbHasDisconnectCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	if (!dbIsConnect())
	{
		m_cur_connect_failed_count++;
		if (m_cur_connect_failed_count >= m_config.server_config.db_connect_check_times)
		{
			unsigned int srvId = 0;
			NProject::getDestServiceID(MessageSrv, "0", 1, srvId);
			sendMessage(NULL, 0, srvId, MessageBusiness::BUSINESS_STOP_GAME_SERVICE_NOTIFY);

			ReleaseErrorLog("db connect failed times:%u", m_cur_connect_failed_count);
			stopService();
		}
		setTimer(1000, (TimerHandler)&CSrvMsgHandler::dbHasDisconnectCheck, 0, NULL, 1);
	}
	else
	{
		m_cur_connect_failed_count = 0;
	}
}

// 小时定时器，自服务启动开始算，每一小时触发一次
void CSrvMsgHandler::hourOnTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 删除3天后没刷新的数据内存块，这个值要配置出去
	// const unsigned int dataCacheExpiredInterval = 60 * 60 * 24 * 3;
	// m_userNameCache.removeExpiredCache(dataCacheExpiredInterval);
	// m_memcache.removeExpiredCache(dataCacheExpiredInterval);
	
	// OptInfoLog("hour timer on finish");
}


bool CSrvMsgHandler::getUserBaseinfo(const string &username, CUserBaseinfo &user_baseinfo, bool just_memcached)
{
	//先从memcached中获取数据
	vector<char> ret_val;
	uint32_t flags = 0;
	bool getMemDataOk = m_memcache.get(username, ret_val, flags);
	if (getMemDataOk && sizeof(user_baseinfo) == ret_val.size())
	{
		memcpy((void*)&user_baseinfo, (void*)&ret_val[0], ret_val.size());
		
		/*
		OptWarnLog("get base info from memcache success, request user = %s, id = %s, name = %s, game_gold = %lld, rmb_gold = %u, diamonds_number = %u, phone_fare = %u, voucher_number = %u, sum_recharge = %u",
	username.c_str(), user_baseinfo.static_info.username, user_baseinfo.static_info.nickname, user_baseinfo.dynamic_info.game_gold,
	user_baseinfo.dynamic_info.rmb_gold, user_baseinfo.dynamic_info.diamonds_number, user_baseinfo.dynamic_info.phone_fare, user_baseinfo.dynamic_info.voucher_number,
	user_baseinfo.dynamic_info.sum_recharge);
		*/
		
		return true;
	}

	if (just_memcached)
	{
		std::string errorMsg;
		m_memcache.error(errorMsg);
		OptWarnLog("get user data from memcache failed, user = %s, result = %d, info size = %u, value size = %u, flags = %u, errorMsg = %s",
		username.c_str(), getMemDataOk, sizeof(user_baseinfo), ret_val.size(), flags, errorMsg.c_str());
		
		return false;
	}

	//从mysql中查找
	//先查找static_baseinfo
	char sql[2048] = { 0 };
	CQueryResult *p_result = NULL;
	snprintf(sql, sizeof(sql), "select id,register_time,username,password,nickname,person_sign,realname,portrait_id,qq_num,address,birthday,sex,home_phone,\
							   mobile_phone,email,age,idc,other_contact,city_id,status from tb_user_static_baseinfo where username=\'%s\';", username.c_str());
	if (Success != m_pDBOpt->queryTableAllResult(sql, p_result) || 1 != p_result->getRowCount())
	{
		OptErrorLog("exec sql error:%s", sql);
		m_pDBOpt->releaseQueryResult(p_result);
		return false;
	}
	RowDataPtr rowData = p_result->getNextRow();
	user_baseinfo.static_info.id = atoi(rowData[0]);
	strcpy(user_baseinfo.static_info.register_time, rowData[1]);
	strcpy(user_baseinfo.static_info.username, rowData[2]);
	strcpy(user_baseinfo.static_info.password, rowData[3]);
	strcpy(user_baseinfo.static_info.nickname, rowData[4]);
	strcpy(user_baseinfo.static_info.person_sign, rowData[5]);
	strcpy(user_baseinfo.static_info.realname, rowData[6]);
	user_baseinfo.static_info.portrait_id = atoi(rowData[7]);
	strcpy(user_baseinfo.static_info.qq_num, rowData[8]);
	strcpy(user_baseinfo.static_info.address, rowData[9]);
	strcpy(user_baseinfo.static_info.birthday, rowData[10]);
	user_baseinfo.static_info.sex = atoi(rowData[11]);
	strcpy(user_baseinfo.static_info.home_phone, rowData[12]);
	strcpy(user_baseinfo.static_info.mobile_phone, rowData[13]);
	strcpy(user_baseinfo.static_info.email, rowData[14]);
	user_baseinfo.static_info.age = atoi(rowData[15]);
	strcpy(user_baseinfo.static_info.idc, rowData[16]);
	strcpy(user_baseinfo.static_info.other_contact, rowData[17]);
	user_baseinfo.static_info.city_id = atoi(rowData[18]);
	user_baseinfo.static_info.status = atoi(rowData[19]);
	m_pDBOpt->releaseQueryResult(p_result);

	//再查找dynamic_baseinfo
	p_result = NULL;
	snprintf(sql, sizeof(sql), "select id,username,level,cur_level_exp,vip_level,vip_cur_level_exp,vip_time_limit,score,game_gold,\
							   	rmb_gold,sum_recharge,sum_recharge_count,last_login_time,last_logout_time,sum_online_time,sum_login_times,voucher_number,phone_card_number,\
								diamonds_number,phone_fare,mobile_phone_number from tb_user_dynamic_baseinfo where username=\'%s\';", username.c_str());
	if (Success != m_pDBOpt->queryTableAllResult(sql, p_result) || 1 != p_result->getRowCount() || p_result->getFieldCount() != 21)
	{
		OptErrorLog("exec sql error:%s", sql);
		m_pDBOpt->releaseQueryResult(p_result);
		return false;
	}
	rowData = p_result->getNextRow();
	user_baseinfo.dynamic_info.id = atoi(rowData[0]);
	strcpy(user_baseinfo.dynamic_info.username, rowData[1]);
	user_baseinfo.dynamic_info.level = atoi(rowData[2]);
	user_baseinfo.dynamic_info.cur_level_exp = atoi(rowData[3]);
	user_baseinfo.dynamic_info.vip_level = atoi(rowData[4]);
	user_baseinfo.dynamic_info.vip_cur_level_exp = atoi(rowData[5]);
	strcpy(user_baseinfo.dynamic_info.vip_time_limit, rowData[6]);
	user_baseinfo.dynamic_info.score = atoi(rowData[7]);
	user_baseinfo.dynamic_info.game_gold = atoll(rowData[8]);
	user_baseinfo.dynamic_info.rmb_gold = atoi(rowData[9]);
	user_baseinfo.dynamic_info.sum_recharge = atoi(rowData[10]);
	user_baseinfo.dynamic_info.sum_recharge_count = atoi(rowData[11]);
	strcpy(user_baseinfo.dynamic_info.last_login_time, rowData[12]);
	strcpy(user_baseinfo.dynamic_info.last_logout_time, rowData[13]);
	user_baseinfo.dynamic_info.sum_online_time = atoi(rowData[14]);
	user_baseinfo.dynamic_info.sum_login_times = atoi(rowData[15]);
	user_baseinfo.dynamic_info.voucher_number = atoi(rowData[16]);
	user_baseinfo.dynamic_info.phone_card_number = atoi(rowData[17]);
	user_baseinfo.dynamic_info.diamonds_number = atoi(rowData[18]);
	user_baseinfo.dynamic_info.phone_fare = atoi(rowData[19]);
	strcpy(user_baseinfo.dynamic_info.mobile_phone_number, rowData[20]);
	m_pDBOpt->releaseQueryResult(p_result);

	//再查找prop_info
	p_result = NULL;
	snprintf(sql, sizeof(sql), "select id,username,light_cannon_count,mute_bullet_count,flower_count,slipper_count,suona_count,auto_bullet_count,lock_bullet_count,rampage_count,dud_shield_count from tb_user_prop_info where username=\'%s\';", username.c_str());
	if (Success != m_pDBOpt->queryTableAllResult(sql, p_result) || 1 != p_result->getRowCount() || p_result->getFieldCount() != 11)
	{
		OptErrorLog("exec sql error:%s", sql);
		m_pDBOpt->releaseQueryResult(p_result);
		return false;
	}
	rowData = p_result->getNextRow();
	user_baseinfo.prop_info.id = atoi(rowData[0]);
	strcpy(user_baseinfo.prop_info.username, rowData[1]);
	user_baseinfo.prop_info.light_cannon_count = atoi(rowData[2]);
	user_baseinfo.prop_info.mute_bullet_count = atoi(rowData[3]);
	user_baseinfo.prop_info.flower_count = atoi(rowData[4]);
	user_baseinfo.prop_info.slipper_count = atoi(rowData[5]);
	user_baseinfo.prop_info.suona_count = atoi(rowData[6]);
	user_baseinfo.prop_info.auto_bullet_count = atoi(rowData[7]);
	user_baseinfo.prop_info.lock_bullet_count = atoi(rowData[8]);
	user_baseinfo.prop_info.rampage_count = atoi(rowData[9]);
    user_baseinfo.prop_info.dud_shield_count = atoi(rowData[10]);

	m_pDBOpt->releaseQueryResult(p_result);

    /*
    OptWarnLog("get base info from mysql success, request user = %s, id = %s, name = %s, game_gold = %lld, rmb_gold = %u, diamonds_number = %u, phone_fare = %u, voucher_number = %u, sum_recharge = %u",
	username.c_str(), user_baseinfo.static_info.username, user_baseinfo.static_info.nickname, user_baseinfo.dynamic_info.game_gold,
	user_baseinfo.dynamic_info.rmb_gold, user_baseinfo.dynamic_info.diamonds_number, user_baseinfo.dynamic_info.phone_fare, user_baseinfo.dynamic_info.voucher_number,
	user_baseinfo.dynamic_info.sum_recharge);
	*/
	
	return true;
}

bool CSrvMsgHandler::setUserBaseinfoToMemcached(const CUserBaseinfo &user_baseinfo)
{
	uint32_t flags = 0;
	bool ret = m_memcache.set(user_baseinfo.static_info.username, (char*)&user_baseinfo, sizeof(user_baseinfo), 0, flags);
	if (!ret)
	{
	    std::string errorMsg;
		m_memcache.error(errorMsg);
		
		OptErrorLog("set base info to memcache error, id = %s, name = %s, game_gold = %lld, rmb_gold = %u, diamonds_number = %u, phone_fare = %u, voucher_number = %u, sum_recharge = %u, errorMsg = %s",
	    user_baseinfo.static_info.username, user_baseinfo.static_info.nickname, user_baseinfo.dynamic_info.game_gold,
	    user_baseinfo.dynamic_info.rmb_gold, user_baseinfo.dynamic_info.diamonds_number, user_baseinfo.dynamic_info.phone_fare, user_baseinfo.dynamic_info.voucher_number,
	    user_baseinfo.dynamic_info.sum_recharge, errorMsg.c_str());
	}

	return ret;
}

bool CSrvMsgHandler::updateUserStaticBaseinfoToMysql(const CUserStaticBaseinfo &user_static_baseinfo)
{
	char sql[2048] = { 0 };
	snprintf(sql, sizeof(sql), "update tb_user_static_baseinfo set password=\'%s\',nickname=\'%s\',person_sign=\'%s\',realname=\'%s\',\
							   portrait_id=%u,qq_num=\'%s\',address=\'%s\',birthday=\'%s\',sex=%d,home_phone=\'%s\', mobile_phone=\'%s\',email=\'%s\',\
							   age=%u,idc=\'%s\',other_contact=\'%s\',city_id=%u,status=%u where username=\'%s\';", user_static_baseinfo.password,
							   user_static_baseinfo.nickname, user_static_baseinfo.person_sign, user_static_baseinfo.realname, user_static_baseinfo.portrait_id,
							   user_static_baseinfo.qq_num, user_static_baseinfo.address, user_static_baseinfo.birthday, user_static_baseinfo.sex,
							   user_static_baseinfo.home_phone, user_static_baseinfo.mobile_phone, user_static_baseinfo.email, user_static_baseinfo.age,
							   user_static_baseinfo.idc, user_static_baseinfo.other_contact, user_static_baseinfo.city_id, user_static_baseinfo.status, user_static_baseinfo.username);
	if (Success == m_pDBOpt->modifyTable(sql))
	{
		//OptInfoLog("exeSql success|%s|%s", user_static_baseinfo.username, sql);
	}
	else
	{
		OptErrorLog("exeSql failed|%s|%s", user_static_baseinfo.username, sql);
		return false;
	}

	return true;
}

bool CSrvMsgHandler::updateUserDynamicBaseinfoToMysql(const CUserDynamicBaseinfo &user_dynamic_baseinfo)
{
	char sql[2048] = { 0 };
	snprintf(sql, sizeof(sql), "update tb_user_dynamic_baseinfo set level=%u,cur_level_exp=%u,vip_level=%u,vip_cur_level_exp=%u,\
							   vip_time_limit=\'%s\',score=%u,game_gold=%lu,rmb_gold=%u,sum_recharge=%u,sum_recharge_count=%u,last_login_time=\'%s\',\
							   last_logout_time=\'%s\',sum_online_time=%u,sum_login_times=%u,voucher_number=%u,phone_card_number=%u,diamonds_number=%u,\
							   phone_fare=%u, mobile_phone_number=\'%s\' where username=\'%s\';", 
							   user_dynamic_baseinfo.level, user_dynamic_baseinfo.cur_level_exp, user_dynamic_baseinfo.vip_level, user_dynamic_baseinfo.vip_cur_level_exp,
							   user_dynamic_baseinfo.vip_time_limit, user_dynamic_baseinfo.score, user_dynamic_baseinfo.game_gold, user_dynamic_baseinfo.rmb_gold,
							   user_dynamic_baseinfo.sum_recharge, user_dynamic_baseinfo.sum_recharge_count, user_dynamic_baseinfo.last_login_time,
							   user_dynamic_baseinfo.last_logout_time, user_dynamic_baseinfo.sum_online_time, user_dynamic_baseinfo.sum_login_times,
							   user_dynamic_baseinfo.voucher_number, user_dynamic_baseinfo.phone_card_number, user_dynamic_baseinfo.diamonds_number,
							   user_dynamic_baseinfo.phone_fare, user_dynamic_baseinfo.mobile_phone_number, user_dynamic_baseinfo.username);
	if (Success == m_pDBOpt->modifyTable(sql))
	{
		//OptInfoLog("exeSql success|%s|%s", user_dynamic_baseinfo.username, sql);
	}
	else
	{
		OptErrorLog("exeSql failed|%s|%s", user_dynamic_baseinfo.username, sql);
		return false;
	}

	return true;
}

bool CSrvMsgHandler::updateUserPropInfoToMysql(const CUserPropInfo &user_prop_info)
{
	char sql[2048] = { 0 };
	snprintf(sql, sizeof(sql), "update tb_user_prop_info set light_cannon_count=%u, mute_bullet_count=%u, flower_count=%u, slipper_count=%u, suona_count=%u, auto_bullet_count=%u, lock_bullet_count=%u, rampage_count=%u, dud_shield_count=%u where username=\'%s\';",
		user_prop_info.light_cannon_count, user_prop_info.mute_bullet_count, user_prop_info.flower_count, user_prop_info.slipper_count, user_prop_info.suona_count, user_prop_info.auto_bullet_count, user_prop_info.lock_bullet_count, user_prop_info.rampage_count, user_prop_info.dud_shield_count, user_prop_info.username);
	if (Success == m_pDBOpt->modifyTable(sql))
	{
		//OptInfoLog("exeSql success|%s|%s", user_dynamic_baseinfo.username, sql);
	}
	else
	{
		OptErrorLog("exeSql failed|%s|%s", user_prop_info.username, sql);
		return false;
	}

	return true;
}

bool CSrvMsgHandler::mysqlRollback()
{
	bool ret = m_pDBOpt->rollback();
	if (ret)
	{
		OptInfoLog("exe rollback success");
	}
	else
	{
		OptErrorLog("exe rollback failed");
	}

	return ret;
}

bool CSrvMsgHandler::mysqlCommit()
{
	if (!m_pDBOpt)
	{
		return false;
	}

	bool ret = m_pDBOpt->commit();
	if (!ret)
	{
		const char* msg = m_pDBOpt->errorInfo();
		if (msg == NULL) msg = "";
		OptErrorLog("exe mysql commit failed, error = %u, msg = %s", m_pDBOpt->errorNum(), msg);
	}
	
	return ret;
}

bool CSrvMsgHandler::dbIsConnect()
{
	//检测memcached 连接
	if (!m_memcache.set(string("test"), "test", 4, 1, 0))
	{
		ReleaseErrorLog("TIME CHECK|memcache connect failed. ip:%s port:%d", m_config.server_config.memcached_ip.c_str(), m_config.server_config.memcached_port);
		return false;
	}

	//检测Mysql 连接
	char sql[1024] = { 0 };
	CQueryResult *p_result = NULL;
	snprintf(sql, sizeof(sql), "select now();");
	if (Success != m_pDBOpt->queryTableAllResult(sql, p_result))
	{
		ReleaseErrorLog("TIME CHECK|exec sql error:%s", sql);
		m_pDBOpt->releaseQueryResult(p_result);
		return false;
	}
	m_pDBOpt->releaseQueryResult(p_result);

	return true;
}

void CSrvMsgHandler::onUpdateConfig()
{
	loadConfig();	
}


CQueryResult* CSrvMsgHandler::queryUserInfo(const char* sql, const unsigned int len)
{
	CQueryResult* p_result = NULL;
	if (Success != m_pDBOpt->queryTableAllResult(sql, len, p_result) || p_result->getRowCount() < 1)
	{
		m_pDBOpt->releaseQueryResult(p_result);
		p_result = NULL;
	}
	
	return p_result;
}

void CSrvMsgHandler::releaseQueryResult(CQueryResult* rst)
{
	if (rst != NULL) m_pDBOpt->releaseQueryResult(rst);
}

CLogicHandler& CSrvMsgHandler::getLogicHander()
{
	return m_logicHandler;
}

CLogic& CSrvMsgHandler::getLogic()
{
	return *m_pLogic;
}

CLogicData& CSrvMsgHandler::getLogicDataInstance()
{
	return m_logicData;
}

CRedis& CSrvMsgHandler::getLogicRedisService()
{
	return m_logicRedisDbOpt;
}

CSensitiveWordFilter& CSrvMsgHandler::getSensitiveWordFilter()
{
	return m_senitiveWordFilter;
}

// 金币&道具&玩家属性等数量变更修改
int CSrvMsgHandler::changeUserPropertyValue(const char* userData, const unsigned int userDataLen, const google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo>& goodsList, const com_protocol::BuyuGameRecordStorageExt& recordData)
{
	com_protocol::GameRecordPkg record;
	char dataBuffer[MaxMsgLen] = {0};
	unsigned int dataBufferLen = sizeof(dataBuffer);
	const char* msgData = serializeMsgToBuffer(recordData, dataBuffer, dataBufferLen, "change user property value record");
	if (msgData != NULL)
	{
		record.set_game_type(NProject::GameRecordType::BuyuExt);
		record.set_game_record_bin(msgData, dataBufferLen);
	}

	return changeUserPropertyValue(userData, userDataLen, goodsList, &record);
}
	
// 金币&道具&玩家属性等数量变更修改
int CSrvMsgHandler::changeUserPropertyValue(const char* userData, const unsigned int userDataLen, 
											const google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo>& goodsList, 
											const com_protocol::GameRecordPkg* record, 
											google::protobuf::RepeatedPtrField<com_protocol::PropItem>* amountValueList, 
											google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>* changeValueList)
{
	CUserBaseinfo user_baseinfo;
	int rc = changeUserPropertyValue(userData, userDataLen, goodsList, user_baseinfo, amountValueList, changeValueList);
	if (rc != ServiceSuccess) return rc;

	if (!setUserBaseinfoToMemcached(user_baseinfo)) return ServiceUpdateDataToMemcachedFailed;  // 将数据同步到memcached
	updateOptStatus(string(userData, userDataLen), UpdateOpt::Modify);	// 有修改，更新状态
	
	if (record != NULL) m_p_game_record->procGameRecord(*record, &user_baseinfo);  // 写游戏记录
	
	return ServiceSuccess;
}

// 金币&道具&玩家属性等数量变更修改
int CSrvMsgHandler::changeUserPropertyValue(const char* userData, const unsigned int userDataLen,
											const google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo>& goodsList,
											CUserBaseinfo& user_baseinfo,
											google::protobuf::RepeatedPtrField<com_protocol::PropItem>* amountValueList,
											google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>* changeValueList)
{
	if (!getUserBaseinfo(string(userData, userDataLen), user_baseinfo)) return ServiceGetUserinfoFailed;

	/*  类型定义值
	PropGold = 1,				//金币
	PropFishCoin = 2,			//渔币
	PropTelephoneFare = 3,		//话费卡
	PropSuona = 4,				//小喇叭
	PropLightCannon = 5,		//激光炮
	PropFlower = 6,				//鲜花
	PropMuteBullet = 7,			//哑弹
	PropSlipper = 8,			//拖鞋
	PropVoucher = 9,			//奖券
	PropAutoBullet = 10,		//自动炮子弹
	PropLockBullet = 11,		//锁定炮子弹
	PropDiamonds = 12,			//钻石
	PropPhoneFareValue = 13, 	//话费额度
	PropScore = 14,				//积分
	PropRampage = 15,			//狂暴
	PropDudShield = 16,			//哑弹防护
	PropMax,
	*/
	uint32_t* type2value[] =
	{
		NULL,                                               //保留
		NULL,                                               //金币
		&user_baseinfo.dynamic_info.rmb_gold,               //渔币
		&user_baseinfo.dynamic_info.phone_card_number,      //话费卡
		&user_baseinfo.prop_info.suona_count,				//小喇叭
		&user_baseinfo.prop_info.light_cannon_count,		//激光炮
		&user_baseinfo.prop_info.flower_count,				//鲜花
		&user_baseinfo.prop_info.mute_bullet_count,			//哑弹
		&user_baseinfo.prop_info.slipper_count,			    //拖鞋
		&user_baseinfo.dynamic_info.voucher_number,	        //奖券
		&user_baseinfo.prop_info.auto_bullet_count,		    //自动炮子弹
		&user_baseinfo.prop_info.lock_bullet_count,		    //锁定炮子弹
		&user_baseinfo.dynamic_info.diamonds_number,        //钻石
		&user_baseinfo.dynamic_info.phone_fare,             //话费额度
		&user_baseinfo.dynamic_info.score,					//积分
		&user_baseinfo.prop_info.rampage_count,				//狂暴
		&user_baseinfo.prop_info.dud_shield_count,		    //哑弹防护
	};
	const unsigned int typeCount = sizeof(type2value) / sizeof(type2value[0]);
	
	// PK场金币门票处理
	unsigned int goldTicketPrefix = 0;
	PKGoldTicket pkGoldTicket;
	PKGoldTicketVector pkGoldTicketVector;
	
	int64_t tmp = 0;
	const bool hasAmountValueList = (amountValueList != NULL);
	const bool hasChangeValueList = (changeValueList != NULL);
	for (int idx = 0; idx < goodsList.size(); ++idx)
	{
		const com_protocol::ItemChangeInfo& icInfo = goodsList.Get(idx);
        if (icInfo.type() < typeCount)  // 基本的道具类型
		{
			uint32_t* value = type2value[icInfo.type()];
			if (value == NULL)
			{
				switch (icInfo.type())
				{
					case EPropType::PropGold :
					{
						tmp = user_baseinfo.dynamic_info.game_gold + icInfo.count();
						user_baseinfo.dynamic_info.game_gold = tmp;  // 金币
						break;
					}

					default :
					{
						return ServicePropTypeInvalid;
						break;
					}
				}

				if (tmp < 0) return ServicePropNotEnought;
			}
			else
			{
				tmp = (int64_t)(*value) + icInfo.count();
				if (tmp < 0) return ServicePropNotEnought;

				*value = (uint32_t)tmp;
			}
		}
		
		// 其他道具类型
		else
		{
			switch (icInfo.type())
			{
				case EUserInfoFlag::EVipLevel : 
				{
					tmp = user_baseinfo.dynamic_info.vip_level + icInfo.count();
					user_baseinfo.dynamic_info.vip_level = (uint32_t)tmp;  // VIP等级
					break;
				}
				
				case EUserInfoFlag::ERechargeAmount : 
				{
					tmp = user_baseinfo.dynamic_info.sum_recharge + icInfo.count();
					user_baseinfo.dynamic_info.sum_recharge = (uint32_t)tmp;  // 充值额度
					++user_baseinfo.dynamic_info.sum_recharge_count;
					break;
				}
				
				default :
				{
					if (!checkPKGoldTicket(icInfo.type(), pkGoldTicket.id, goldTicketPrefix, pkGoldTicket.isFullDay,
					pkGoldTicket.day, pkGoldTicket.beginHour, pkGoldTicket.beginMin, pkGoldTicket.endHour, pkGoldTicket.endMin)) return ServicePropTypeInvalid;
					
					pkGoldTicket.count = icInfo.count();
					pkGoldTicketVector.push_back(pkGoldTicket);
					continue;
					
					break;
				}
			}

			if (tmp < 0) return ServicePropNotEnought;
		}

        if (hasAmountValueList)
		{
			com_protocol::PropItem* propItem = amountValueList->Add();
			propItem->set_type(icInfo.type());
			propItem->set_count((uint32_t)tmp);
		}
		
		if (hasChangeValueList)
		{
			com_protocol::GiftInfo* gfInfo = changeValueList->Add();
			gfInfo->set_type(icInfo.type());
			gfInfo->set_num(icInfo.count());
		}
	}
	
	int rc = ServiceSuccess;
	if (pkGoldTicketVector.size() > 0) rc = changePKGoldTicketValue(userData, userDataLen, pkGoldTicketVector);
	
	return rc;
}

// 修改PK场金币门票数量
int CSrvMsgHandler::changePKGoldTicketValue(const char* userName, unsigned int len, const PKGoldTicketVector& pkGoldTicketVector)
{
	const unsigned int maxTicketIndex = 100000;  // 金币门票的最大索引ID值
	const unsigned int dayGoldTicketPrefix = EPropType::EPKDayGoldTicket * maxTicketIndex;
	const unsigned int hourGoldTicketPrefix = EPropType::EPKHourGoldTicket * maxTicketIndex;
				
	// 取得当前时间
	struct tm tmval;
	time_t curSecs = time(NULL);
	localtime_r(&curSecs, &tmval);
	
    com_protocol::PKTicket* pkTicket = removeOverdueGoldTicket(userName, len);  // 先删除过期的门票
	for (PKGoldTicketVector::const_iterator it = pkGoldTicketVector.begin(); it != pkGoldTicketVector.end(); ++it)
	{
		bool isFind = false;
		if (it->id == 0 && it->count > 0)  // 新增加金币门票
		{
			// 计算门票的有效使用时间段
			struct tm ticketTime = tmval;
			ticketTime.tm_sec = 0;
			ticketTime.tm_min = 0;
			ticketTime.tm_hour = 0;
			ticketTime.tm_mday += it->day;
			const unsigned int endTime = mktime(&ticketTime);                // 结束时间点
			
			// 查找是否和现有的门票相同，相同则数量增加，否则增加新的门票
			unsigned int ticketId = 0;
			for (int idx = 0; idx < pkTicket->gold_ticket_size(); ++idx)
			{
				const com_protocol::Ticket& ticket = pkTicket->gold_ticket(idx);
				if (ticket.end_time() == endTime)
				{
					if ((it->isFullDay && !ticket.has_begin_hour())  // 全天有效的门票
						|| (it->beginHour == ticket.begin_hour() && it->beginMin == ticket.begin_min() && it->endHour == ticket.end_hour() && it->endMin == ticket.end_min()))
					{
						isFind = true;
						pkTicket->mutable_gold_ticket(idx)->set_count(ticket.count() + it->count);  // 相同的门票增加数量就ok
						break;
					}
				}
				
				ticketId = ticket.has_begin_hour() ? (ticket.id() % hourGoldTicketPrefix) : (ticket.id() % dayGoldTicketPrefix);
			}
			
			if (!isFind)
			{
				// 添加新门票
				com_protocol::Ticket* newTicket = pkTicket->add_gold_ticket();
				unsigned int newTicketId = it->isFullDay ? dayGoldTicketPrefix : hourGoldTicketPrefix;
				newTicketId += (++ticketId % maxTicketIndex);
				newTicket->set_id(newTicketId);
				newTicket->set_count(it->count);
				newTicket->set_end_time(endTime);
				if (!it->isFullDay)
				{
					newTicket->set_begin_hour(it->beginHour);
					newTicket->set_begin_min(it->beginMin);
					newTicket->set_end_hour(it->endHour);
					newTicket->set_end_min(it->endMin);
				}
			}
		}
		
		else  // 使用&增加 金币门票
		{
			// 自动使用门票规则：使用最早到期的符合条件的门票
			// 或者根据门票ID使用指定的门票
			unsigned int useTicketIndex = 0;
			const unsigned int ticketId = (it->id == EPropType::EPKDayGoldTicket || it->id == EPropType::EPKHourGoldTicket) ? 0 : it->id;
			isFind = findPKGoldTicket(pkTicket, useTicketIndex, ticketId, it->count);  // 找到满足条件的PK场金币门票
			if (!isFind)
			{
				if (it->id != EPropType::EPKDayGoldTicket && it->id != EPropType::EPKHourGoldTicket)
				{
					OptErrorLog("can not find the PK gold ticket, user = %s, id = %u", userName, it->id);
					return ServiceNoFindPKGoldTicketId;
				}
				
				return ServicePropNotEnought;
			}
			
			int newCount = (int)(pkTicket->gold_ticket(useTicketIndex).count()) + it->count;
			if (newCount < 0) return ServicePropNotEnought;
			
			pkTicket->mutable_gold_ticket(useTicketIndex)->set_count(newCount);
		}
	}
	
	return ServiceSuccess;
}

// 删除过期的金币门票
com_protocol::PKTicket* CSrvMsgHandler::removeOverdueGoldTicket(const char* userName, unsigned int len)
{
	const unsigned int curSecs = time(NULL);  // 当前时间点
	ServiceLogicData& srvLogicData = getLogicDataInstance().setLogicData(userName, len);
	com_protocol::PKTicket* pkTicket = srvLogicData.logicData.mutable_pk_ticket();
	for (int idx = 0; idx < pkTicket->gold_ticket_size();)
	{
		if (pkTicket->gold_ticket(idx).end_time() < curSecs)  // 检查是否已经过期了
		{
			pkTicket->mutable_gold_ticket()->DeleteSubrange(idx, 1);  // 删除过期的门票
			continue;
		}
		
		++idx;
	}
	
	return pkTicket;
}

// 找到满足条件的PK场金币门票
bool CSrvMsgHandler::findPKGoldTicket(const com_protocol::PKTicket* pkTicket, unsigned int& ticketIndex, const unsigned int ticketId, const int count)
{
	// 查找指定ID的门票
	if (ticketId > 0)
	{
		for (int idx = 0; idx < pkTicket->gold_ticket_size(); ++idx)
		{
			if (pkTicket->gold_ticket(idx).id() == ticketId && ((int)pkTicket->gold_ticket(idx).count() + count) >= 0)  // ID匹配并且数量足够
			{
				ticketIndex = idx;
				return true;
			}
		}
		
		return false;
	}
	
	// 取得当前时间
	struct tm tmval;
	time_t curSecs = time(NULL);
	localtime_r(&curSecs, &tmval);
	
	// 自动使用门票规则：使用最早到期的符合条件的门票
	PKGoldTicketSortInfoVector goldTicketSortInfoVector;
	for (int idx = 0; idx < pkTicket->gold_ticket_size(); ++idx)
	{
		if (pkTicket->gold_ticket(idx).count() > 0) goldTicketSortInfoVector.push_back(PKGoldTicketSortInfo(idx, pkTicket->gold_ticket(idx).end_time()));
	}
	std::sort(goldTicketSortInfoVector.begin(), goldTicketSortInfoVector.end(), pkGoldTicketTimeSort);  // 先按照门票的期限时间排序

	for (PKGoldTicketSortInfoVector::const_iterator sortIt = goldTicketSortInfoVector.begin(); sortIt != goldTicketSortInfoVector.end(); ++sortIt)
	{
		const com_protocol::Ticket& ticket = pkTicket->gold_ticket(sortIt->index);
		if (!ticket.has_begin_hour())
		{
			ticketIndex = sortIt->index;
			return true;
		}
		
		struct tm goldTicketTime = tmval;
		goldTicketTime.tm_sec = 0;
		goldTicketTime.tm_min = ticket.begin_min();
		goldTicketTime.tm_hour = ticket.begin_hour();
		unsigned int timeSecs = mktime(&goldTicketTime);  // 开始时间点
		if (curSecs >= timeSecs)
		{
			goldTicketTime.tm_sec = 0;
			goldTicketTime.tm_min = ticket.end_min();
			goldTicketTime.tm_hour = ticket.end_hour();
			timeSecs = mktime(&goldTicketTime);  // 结束时间点
			if (curSecs < timeSecs)
			{
				ticketIndex = sortIt->index;
				return true;
			}
		}
	}
	
	return false;
}

void CSrvMsgHandler::handldMsgChangeUserPropertyValueReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::PropChangeReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgChangeUserPropertyValueReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::PropChangeRsp rsp;
	(req.items_size() > 0) ? rsp.set_result(0) : rsp.set_result(ServicePropTypeInvalid);
	rsp.set_src_username(req.src_username());
	if (req.has_dst_username()) rsp.set_dst_username(req.dst_username());

	if (rsp.result() == 0)
	{
		const com_protocol::GameRecordPkg* record = req.has_game_record() ? &(req.game_record()) : NULL;
		int result = changeUserPropertyValue(getContext().userData, getContext().userDataLen, req.items(), record, rsp.mutable_items());
		rsp.set_result(result);
	}
	
	char dataBuffer[MaxMsgLen] = { 0 };
	if (rsp.result() != 0)
	{
		unsigned int itemLogSize = 0; // only for log
		rsp.clear_items();
		for (int idx = 0; idx < req.items_size(); ++idx)
		{
			const com_protocol::ItemChangeInfo& icInfo = req.items(idx);
			com_protocol::PropItem* propItem = rsp.add_items();
			propItem->set_type(icInfo.type());
			propItem->set_count(icInfo.count());
			
			itemLogSize += snprintf(dataBuffer + itemLogSize, MaxMsgLen - itemLogSize - 1, "[%u = %d] ", icInfo.type(), icInfo.count()); // only for log
		}

		OptErrorLog("ChangeProp srcSrvId = %u, srcProtocolId = %d, user = %s, result = %d, dst username = %s, item = %s",
		srcSrvId, srcProtocolId, getContext().userData, rsp.result(), req.dst_username().c_str(), dataBuffer);
	}

	rsp.SerializeToArray(dataBuffer, MaxMsgLen);
	if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_CHANGE_PROP_RSP;
	sendMessage(dataBuffer, rsp.ByteSize(), srcSrvId, srcProtocolId);

	return;
}

void CSrvMsgHandler::handldMsgChangeManyPeoplePropertyValueReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::MoreUsersPropChangeReq req;
	if (!req.ParseFromArray(data, len))
	{
		OptErrorLog("change more user property value error, unpack failed len = %u", len);
		return;
	}
	
	if (req.prop_change_req_size() < 1)
	{
		OptErrorLog("change more user property value error, the size = %d", req.prop_change_req_size());
		return;
	}
	
	com_protocol::MoreUsersPropChangeRsp rsp;
	int result = ServiceSuccess;
	UserBaseInfoRecordVector userBaseInfoRecordVector;
	for (int idx = 0; idx < req.prop_change_req_size(); ++idx)
	{
		const com_protocol::PropChangeReq& propChangeReq = req.prop_change_req(idx);
		com_protocol::PropChangeRsp& propChangeRsp = *(rsp.add_prop_change_rsp());
		
		(propChangeReq.items_size() > 0) ? propChangeRsp.set_result(ServiceSuccess) : propChangeRsp.set_result(ServicePropTypeInvalid);
	    if (propChangeReq.has_dst_username()) propChangeRsp.set_dst_username(propChangeReq.dst_username());
		if (propChangeReq.has_src_username()) propChangeRsp.set_src_username(propChangeReq.src_username());
			
		if (propChangeRsp.result() == ServiceSuccess)
		{
			userBaseInfoRecordVector.push_back(UserBaseInfoRecord());
		    UserBaseInfoRecord& userBaseInfoRecord = userBaseInfoRecordVector.back();
			userBaseInfoRecord.record = propChangeReq.has_game_record() ? &(propChangeReq.game_record()) : NULL;
			int rc = changeUserPropertyValue(propChangeReq.src_username().c_str(), propChangeReq.src_username().length(), propChangeReq.items(), userBaseInfoRecord.baseInfo, propChangeRsp.mutable_items());
			propChangeRsp.set_result(rc);
		}
		
		if (propChangeRsp.result() != ServiceSuccess)
		{
			result = propChangeRsp.result();
			break;
		}
	}
	rsp.set_result(result);
	
	if (result == ServiceSuccess)
	{
		for (UserBaseInfoRecordVector::iterator it = userBaseInfoRecordVector.begin(); it != userBaseInfoRecordVector.end(); ++it)
		{
			if (setUserBaseinfoToMemcached(it->baseInfo))  // 将数据同步到memcached
			{
				updateOptStatus(it->baseInfo.static_info.username, UpdateOpt::Modify);	// 有修改，更新状态
				
				if (it->record != NULL) m_p_game_record->procGameRecord(*(it->record), &(it->baseInfo));  // 写游戏记录
			}
		}
	}
	else if (result != ServicePropNotEnought)
	{
		OptErrorLog("more user change prop error, srcSrvId = %u, srcProtocolId = %d, result = %d", srcSrvId, srcProtocolId, result);
	}

    char dataBuffer[MaxMsgLen] = {0};
	rsp.SerializeToArray(dataBuffer, MaxMsgLen);
	if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_CHANGE_MANY_PEOPLE_PROP_RSP;
	sendMessage(dataBuffer, rsp.ByteSize(), srcSrvId, srcProtocolId);
}

void CSrvMsgHandler::handldMsgGameRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::MoreUserGameRecordPkg moreUserGameRecor;
	if (!parseMsgFromBuffer(moreUserGameRecor, data, len, "handld msg game record req")) return;

	CUserBaseinfo user_baseinfo;

	for (int i = 0; i <	moreUserGameRecor.user_record_size(); ++i)
	{
		if (!getUserBaseinfo(moreUserGameRecor.user_record(i).user_name(), user_baseinfo))
		{
			OptErrorLog("CSrvMsgHandler handldMsgGameRecord, user name:%s", moreUserGameRecor.user_record(i).user_name().c_str());
			continue;
		}

		m_p_game_record->procGameRecord(moreUserGameRecor.user_record(i).record(), &user_baseinfo, (bool)moreUserGameRecor.user_record(i).record_prop());
	}
}

void CSrvMsgHandler::handldPkPlayRankingListReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetPkPlayRankingInfoRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "handld pk play ranking list req")) return;

	CUserBaseinfo user_baseinfo;

	for (int i = 0; i < rsp.ranking_size(); ++i)
	{
		auto userRankingInfo = rsp.mutable_ranking(i);
		if (!getUserBaseinfo(userRankingInfo->user_name(), user_baseinfo))
		{
			OptErrorLog("CSrvMsgHandler handldPkPlayRankingListReq, get user base info, user name:%s", userRankingInfo->user_name().c_str());
			continue;
		}
		
		userRankingInfo->set_head(user_baseinfo.static_info.portrait_id);		// 头像
		userRankingInfo->set_nickname(user_baseinfo.static_info.nickname);		// 昵称
		userRankingInfo->set_sex(user_baseinfo.static_info.sex);				// 性别
		userRankingInfo->set_gold(user_baseinfo.dynamic_info.game_gold);		// 金币
	}

	auto myRanking = rsp.mutable_user_ranking();
	if (getUserBaseinfo(myRanking->user_name(), user_baseinfo))
	{		
		myRanking->set_head(user_baseinfo.static_info.portrait_id);			// 头像
		myRanking->set_nickname(user_baseinfo.static_info.nickname);		// 昵称
		myRanking->set_sex(user_baseinfo.static_info.sex);					// 性别
		myRanking->set_gold(user_baseinfo.dynamic_info.game_gold);			// 金币
	}
	else
	{
		OptErrorLog("CSrvMsgHandler handldPkPlayRankingListReq, get user base info, user name:%s", myRanking->user_name().c_str());
	}

	char dataBuffer[MaxMsgLen] = { 0 };
	if (rsp.SerializeToArray(dataBuffer, MaxMsgLen))
	{
		sendMessage(dataBuffer, rsp.ByteSize(), srcSrvId, CommonSrvBusiness::BUSINESS_GET_PK_PLAY_RANKING_LIST_RSP);
	}
}

void CSrvMsgHandler::handldGetPKPlayCelebrityReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientPkPlayCelebrityInfoRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "Get PK Play Celebrity rsp")) return;

	CUserBaseinfo user_baseinfo;

	for (int32_t i = 0; i < rsp.celebritys_size(); ++i)
	{
		com_protocol::CelebrityInfo* p = rsp.mutable_celebritys(i);
		if (!getUserBaseinfo(p->username(), user_baseinfo))
		{
			OptErrorLog("CSrvMsgHandler handldPkPlayRankingListReq, get user base info, user name:%s", rsp.celebritys(i).username().c_str());
			continue;
		}
			
		p->set_head(user_baseinfo.static_info.portrait_id);				//头像
		p->set_nickname(user_baseinfo.static_info.nickname);			//昵称
		p->set_sex(user_baseinfo.static_info.sex);						//性别
		p->set_gold(user_baseinfo.dynamic_info.game_gold);				//金币
	}

	char dataBuffer[MaxMsgLen] = { 0 };
	rsp.SerializeToArray(dataBuffer, MaxMsgLen);
	sendMessage(dataBuffer, rsp.ByteSize(), srcSrvId, CommonSrvBusiness::BUSINESS_PK_PLAY_CELEBRITY_INFO_RSP);
}

void CSrvMsgHandler::getUserOtherInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUserOtherInfoReq req;
	if (!parseMsgFromBuffer(req, data, len, "get user other information")) return;
	
	com_protocol::GetUserOtherInfoRsp rsp;
	rsp.set_query_username(req.query_username());
	rsp.set_result(ServiceGetUserinfoFailed);
	if (req.has_call_back_data()) rsp.set_call_back_data(req.call_back_data());
	
	// 获取DB中的用户信息	
	CUserBaseinfo user_baseinfo;
	unsigned int int_value = 0;
	string str_value;
	if (getUserBaseinfo(req.query_username(), user_baseinfo))
	{
		rsp.set_result(ServiceSuccess);
		str_value.clear();
		for (int idx = 0; idx < req.info_flag_size(); ++idx)
		{
			switch (req.info_flag(idx))
			{
			case EPropType::PropGold:				//金币
				int_value = user_baseinfo.dynamic_info.game_gold;
				break;
			case EPropType::PropFishCoin :		    //渔币
				int_value = user_baseinfo.dynamic_info.rmb_gold;
				break;
			case EPropType::PropTelephoneFare :		//话费卡
				int_value = user_baseinfo.dynamic_info.phone_card_number;
				break;
			case EPropType::PropSuona:				//小喇叭
				int_value = user_baseinfo.prop_info.suona_count;
				break;
			case EPropType::PropLightCannon:		//激光炮
				int_value = user_baseinfo.prop_info.light_cannon_count;
				break;
			case EPropType::PropFlower:				//鲜花
				int_value = user_baseinfo.prop_info.flower_count;
				break;
			case EPropType::PropMuteBullet:			//哑弹
				int_value = user_baseinfo.prop_info.mute_bullet_count;
				break;
			case EPropType::PropSlipper:			//拖鞋
				int_value = user_baseinfo.prop_info.slipper_count;
				break;
			case EPropType::PropVoucher:			//奖券
				int_value = user_baseinfo.dynamic_info.voucher_number;
				break;
			case EPropType::PropAutoBullet:			//自动炮子弹
				int_value = user_baseinfo.prop_info.auto_bullet_count;
				break;
			case EPropType::PropLockBullet:			//锁定炮子弹
				int_value = user_baseinfo.prop_info.lock_bullet_count;
				break;
			case EPropType::PropDiamonds :			//钻石
				int_value = user_baseinfo.dynamic_info.diamonds_number;
				break;
			case EPropType::PropPhoneFareValue :	//话费额度
				int_value = user_baseinfo.dynamic_info.phone_fare;
				break;

			case EPropType::PropRampage:			//狂暴
				int_value = user_baseinfo.prop_info.rampage_count;
				break;
				
			case EPropType::PropDudShield:			//哑弹防护
				int_value = user_baseinfo.prop_info.dud_shield_count;
				break;

			case EPropType::PropScores :
				int_value = user_baseinfo.dynamic_info.score;
				break;
				
			case EUserInfoFlag::EVipLevel :			//VIP等级
				int_value = user_baseinfo.dynamic_info.vip_level;
				break;
			case EUserInfoFlag::ERechargeAmount :	//充值话费总额度
				int_value = user_baseinfo.dynamic_info.sum_recharge;
				break;

			case EUserInfoFlag::EUserNickname:		// 用户昵称
				str_value = user_baseinfo.static_info.nickname;
				break;

			case EUserInfoFlag::EUserPortraitId :		// 用户头像
				int_value = user_baseinfo.static_info.portrait_id;
				break;

			default:
				OptErrorLog("get user other information, the info flag invalid, user = %s, flag = %u", getContext().userData, req.info_flag(idx));
				rsp.set_result(ServiceUnknownItemID);
				break;
			}
			
			if (rsp.result() != ServiceSuccess) break;
			
			
			com_protocol::UserOtherInfo* otherInfo = rsp.add_other_info();
			otherInfo->set_info_flag(req.info_flag(idx));
			otherInfo->set_int_value(int_value);
			
			if (!str_value.empty())	otherInfo->set_string_value(str_value);
			
		}
	}

    if (rsp.result() != ServiceSuccess) rsp.clear_other_info();
	
	char buffer[MaxMsgLen] = { 0 };
	unsigned int msgLen = MaxMsgLen;
	const char* msgData = serializeMsgToBuffer(rsp, buffer, msgLen, "get user other information reply");
	if (msgData != NULL)
	{
		if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_GET_USER_OTHER_INFO_RSP;
	    sendMessage(msgData, msgLen, srcSrvId, srcProtocolId);
	}
}

void CSrvMsgHandler::getManyUserOtherInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetManyUserOtherInfoReq req;
	if (!parseMsgFromBuffer(req, data, len, "get many user other information")) return;


	com_protocol::GetManyUserOtherInfoRsp rsp;
	if (req.has_call_back_data()) rsp.set_call_back_data(req.call_back_data());

	// 获取DB中的用户信息	
	CUserBaseinfo user_baseinfo;
	//uint32_t int_value;

	for (auto userIt = req.query_username().begin(); userIt != req.query_username().end(); ++userIt)
	{
		auto queryUser = rsp.add_query_user();
		queryUser->set_result(ServiceSuccess);
		if (!getUserBaseinfo(*userIt, user_baseinfo))
		{
			queryUser->set_result(ServiceUserDataNotExist);
			continue;
		}

		queryUser->set_query_username(*userIt);			
		for (int i = 0; i < req.info_flag_size(); ++i)
		{
			com_protocol::UserOtherInfo *otherInfo = queryUser->add_other_info();
			otherInfo->set_info_flag(req.info_flag(i));

			switch (req.info_flag(i))
			{
			case EPropType::PropGold:
				otherInfo->set_int_value(user_baseinfo.dynamic_info.game_gold);
				break;
				
			case EPropType::PropDudShield:				// 哑弹护盾
				otherInfo->set_int_value(user_baseinfo.prop_info.dud_shield_count);
				break;

			case EUserInfoFlag::EVipLevel:				// VIP等级
				otherInfo->set_int_value(user_baseinfo.dynamic_info.vip_level);
				break;

			case EUserInfoFlag::EUserNickname:			// 用户昵称
				otherInfo->set_string_value(user_baseinfo.static_info.nickname);
				break;

			case EUserInfoFlag::EUserPortraitId:		// 用户头像
				otherInfo->set_int_value(user_baseinfo.static_info.portrait_id);
				break;

			case EUserInfoFlag::EUserSex:				// 用户性别
				otherInfo->set_int_value(user_baseinfo.static_info.sex);
				break;

			case EPropType::PropVoucher:		//奖券
				otherInfo->set_int_value(user_baseinfo.dynamic_info.voucher_number);
				break;

			case EPropType::PropDiamonds:			//钻石
				otherInfo->set_int_value(user_baseinfo.dynamic_info.diamonds_number);
				break;

			case EPropType::PropPhoneFareValue:		//话费额度
				otherInfo->set_int_value(user_baseinfo.dynamic_info.phone_fare);
				break;
			
			// PK场金币对战门票ID类型			
			case EPropType::EPKDayGoldTicket:              
			case EPropType::EPKHourGoldTicket:
			{
				unsigned int ticketIndex = 0;
				com_protocol::PKTicket* pkTicket = removeOverdueGoldTicket(userIt->c_str(), userIt->length());  // 先删除过期的金币门票

				// 找到满足条件的PK场金币门票
				const unsigned int id = findPKGoldTicket(pkTicket, ticketIndex) ? pkTicket->gold_ticket(ticketIndex).id() : 0;                                    
				otherInfo->set_int_value(id);
				break;
			}
			
			default:
				break;
			}				
		}
	}
	
	char buffer[MaxMsgLen] = { 0 };
	unsigned int msgLen = MaxMsgLen;
	const char* msgData = serializeMsgToBuffer(rsp, buffer, msgLen, "get many user other info");
	if (msgData != NULL)
	{
		if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_GET_MANY_USER_OTHER_INFO_RSP;
		sendMessage(msgData, msgLen, srcSrvId, srcProtocolId);
	}
}

void CSrvMsgHandler::getUserPropAndPreventionCheatReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, 
													   unsigned short srcProtocolId)
{
	com_protocol::GetUserPropAndPreventionCheatReq req;
	if (!parseMsgFromBuffer(req, data, len, "reset user other information")) return;

	com_protocol::GetUserPropAndPreventionCheatRsp rsp;
	rsp.set_result(Success);
	rsp.set_query_username(req.query_username());


	do 
	{
		//获取防作弊列表
		char sql_tmp[2048] = { 0 };
		unsigned int sqlLen = snprintf(sql_tmp, sizeof(sql_tmp), "select target_user from tb_user_prevention_cheat where source_user=\'%s\'", req.query_username().c_str());
		CQueryResult* p_result = NULL;

		auto code = m_pMySqlOptDb->queryTableAllResult(sql_tmp, sqlLen, p_result);
		if (Success != code)
		{
			rsp.set_result(code);
			OptErrorLog("CSrvMsgHandler getUserPropAndPreventionCheatReq, exeSql failed|%s", sql_tmp);
			break;
		}

		RowDataPtr rowData = NULL;
		while ((rowData = p_result->getNextRow()) != NULL)
		{
			rsp.add_prevention_cheat_user(rowData[0]);
		}
		m_pMySqlOptDb->releaseQueryResult(p_result);


		//获取用户道具信息
		CUserBaseinfo user_baseinfo;
		if (!getUserBaseinfo(req.query_username(), user_baseinfo))
		{
			rsp.set_result(ServiceUserDataNotExist);
			break;
		}

		for (int i = 0; i < req.info_flag_size(); ++i)
		{
			com_protocol::UserOtherInfo *otherInfo = rsp.add_other_info();
			otherInfo->set_info_flag(req.info_flag(i));

			switch (req.info_flag(i))
			{
			case EPropType::PropGold:
				otherInfo->set_int_value(user_baseinfo.dynamic_info.game_gold);
				break;

			case EPropType::PropFishCoin:			//鱼币 
				otherInfo->set_int_value(user_baseinfo.dynamic_info.rmb_gold);
				break;

			case EPropType::PropTelephoneFare:		//话费卡
				otherInfo->set_int_value(user_baseinfo.dynamic_info.phone_card_number);
				break;

			case EPropType::PropSuona:				//小喇叭
				otherInfo->set_int_value(user_baseinfo.prop_info.suona_count);
				break;

			case EPropType::PropLightCannon:		//激光炮
				otherInfo->set_int_value(user_baseinfo.prop_info.light_cannon_count);
				break;

			case EPropType::PropFlower:				//鲜花
				otherInfo->set_int_value(user_baseinfo.prop_info.flower_count);
				break;

			case EPropType::PropMuteBullet:			//哑弹
				otherInfo->set_int_value(user_baseinfo.prop_info.mute_bullet_count);
				break;

			case EPropType::PropSlipper:			//拖鞋
				otherInfo->set_int_value(user_baseinfo.prop_info.slipper_count);
				break;

			case EPropType::PropVoucher:			//奖券
				otherInfo->set_int_value(user_baseinfo.dynamic_info.voucher_number);
				break;

			case EPropType::PropAutoBullet:		//自动炮子弹
				otherInfo->set_int_value(user_baseinfo.prop_info.auto_bullet_count);
				break;

			case EPropType::PropLockBullet:		//锁定炮子弹
				otherInfo->set_int_value(user_baseinfo.prop_info.lock_bullet_count);
				break;

			case EPropType::PropDiamonds:			//钻石
				otherInfo->set_int_value(user_baseinfo.dynamic_info.diamonds_number);
				break;

			case EPropType::PropPhoneFareValue:		//话费额度
				otherInfo->set_int_value(user_baseinfo.dynamic_info.phone_fare);
				break;

			case EPropType::PropScores:			//积分
				otherInfo->set_int_value(user_baseinfo.dynamic_info.score);
				break;

			case EPropType::PropRampage:			//狂暴
				otherInfo->set_int_value(user_baseinfo.prop_info.rampage_count);
				break;
				
			case EPropType::PropDudShield:			//哑弹防护
			    otherInfo->set_int_value(user_baseinfo.prop_info.dud_shield_count);
				break;

			case EPropType::EPKDayGoldTicket:
			case EPropType::EPKHourGoldTicket:
			{
				unsigned int ticketIndex = 0;
				com_protocol::PKTicket* pkTicket = removeOverdueGoldTicket(req.query_username().c_str(), req.query_username().size());  // 先删除过期的金币门票
				const unsigned int id = findPKGoldTicket(pkTicket, ticketIndex) ? pkTicket->gold_ticket(ticketIndex).id() : 0;	 // 找到满足条件的PK场金币门票
				otherInfo->set_int_value(id);
				break;
			}

			default:
				break;
			}
		}

	} while (false);

	char buffer[MaxMsgLen] = { 0 };
	unsigned int msgLen = MaxMsgLen;
	const char* msgData = serializeMsgToBuffer(rsp, buffer, msgLen, "get user prop and prevention cheat req");
	if (msgData != NULL)
	{
		if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_GET_USER_PROP_AND_PREVENTION_CHEAT_RSP;
		sendMessage(msgData, msgLen, srcSrvId, srcProtocolId);
	}

}

void CSrvMsgHandler::resetUserOtherInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ResetUserOtherInfoReq req;
	if (!parseMsgFromBuffer(req, data, len, "reset user other information")) return;
	
	com_protocol::ResetUserOtherInfoRsp rsp;
	rsp.set_query_username(req.query_username());
	rsp.set_result(ServiceGetUserinfoFailed);
	if (req.has_call_back_data()) rsp.set_call_back_data(req.call_back_data());
	
	// 获取DB中的用户信息	
	CUserBaseinfo user_baseinfo;
	resetUserBaseInfo(req, rsp, user_baseinfo);

	char buffer[MaxMsgLen] = { 0 };
	unsigned int msgLen = MaxMsgLen;
	const char* msgData = serializeMsgToBuffer(rsp, buffer, msgLen, "reset user other information reply");
	if (msgData != NULL)
	{
		if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_RESET_USER_OTHER_INFO_RSP;
	    sendMessage(msgData, msgLen, srcSrvId, srcProtocolId);
	}
}

void CSrvMsgHandler::resetUserBaseInfo(const com_protocol::ResetUserOtherInfoReq& req, com_protocol::ResetUserOtherInfoRsp& rsp, CUserBaseinfo& user_baseinfo)
{
	rsp.set_result(ServiceGetUserinfoFailed);
	if (getUserBaseinfo(req.query_username(), user_baseinfo))
	{
		unsigned int int_value = 0;
		unsigned int info_flag = 0;
		rsp.set_result(ServiceSuccess);
		for (int idx = 0; idx < req.other_info_size(); ++idx)
		{
			int_value = req.other_info(idx).int_value();
			info_flag = req.other_info(idx).info_flag();
			switch (info_flag)
			{
			case EPropType::PropGold:				//金币
				user_baseinfo.dynamic_info.game_gold = int_value;
				break;
			case EPropType::PropFlower:				//鲜花
				user_baseinfo.prop_info.flower_count = int_value;
				break;
			case EPropType::PropMuteBullet:			//哑弹
				user_baseinfo.prop_info.mute_bullet_count = int_value;
				break;
			case EPropType::PropSlipper:			//拖鞋
				user_baseinfo.prop_info.slipper_count = int_value;
				break;
			case EPropType::PropLightCannon:		//激光炮
				user_baseinfo.prop_info.light_cannon_count = int_value;
				break;

			case EPropType::PropScores:				//积分
			{
				DataRecordLog("reset user score|%s|%u|%u", req.query_username().c_str(), user_baseinfo.dynamic_info.score, int_value);
				OptWarnLog("reset user score, user = %s, score = %u, set = %u", req.query_username().c_str(), user_baseinfo.dynamic_info.score, int_value);

				user_baseinfo.dynamic_info.score = int_value;
				break;
			}

			case EUserInfoFlag::EVipLevel :			//VIP等级
				user_baseinfo.dynamic_info.vip_level = int_value;
				break;
	
			/*	
			case EPropType::PropFishCoin :		    //渔币
				int_value = user_baseinfo.dynamic_info.rmb_gold;
				break;
			case EPropType::PropSuona:				//小喇叭
				int_value = user_baseinfo.prop_info.suona_count;
				break;
			case EPropType::PropVoucher:			//奖券
				int_value = user_baseinfo.dynamic_info.voucher_number;
				break;
			case EPropType::PropAutoBullet:			//自动炮子弹
				int_value = user_baseinfo.prop_info.auto_bullet_count;
				break;
			case EPropType::PropLockBullet:			//锁定炮子弹
				int_value = user_baseinfo.prop_info.lock_bullet_count;
				break;
			case EPropType::PropDiamonds :			//钻石
				int_value = user_baseinfo.dynamic_info.diamonds_number;
				break;
			case EPropType::PropPhoneFareValue :	//话费额度
				int_value = user_baseinfo.dynamic_info.phone_fare;
				break;
			case EUserInfoFlag::ERechargeAmount :	//充值话费总额度
				int_value = user_baseinfo.dynamic_info.sum_recharge;
				break;
			*/
 
			default:
				OptErrorLog("reset user other information, the info flag invalid, user = %s, flag = %u", req.query_username().c_str(), info_flag);
				rsp.set_result(ServiceUnknownItemID);
				break;
			}
			
			if (rsp.result() != ServiceSuccess) break;
			
			com_protocol::UserOtherInfo* otherInfo = rsp.add_other_info();
			otherInfo->set_info_flag(info_flag);
			otherInfo->set_int_value(int_value);
		}
		
		if (rsp.result() == ServiceSuccess)
		{
			if (setUserBaseinfoToMemcached(user_baseinfo))  // 将数据同步到memcached
			{
				updateOptStatus(req.query_username(), UpdateOpt::Modify);	// 有修改，更新状态
			}
			else
			{
				rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			}
		}
	}

    if (rsp.result() != ServiceSuccess) rsp.clear_other_info();
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

const char* CSrvMsgHandler::serializeMsgToBuffer(const ::google::protobuf::Message& msg, char* buffer, unsigned int& len, const char* info)
{
	if (msg.SerializeToArray(buffer, len))
	{
		len = msg.ByteSize();
	    return buffer;
	}
	
	OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), len, info);
	return NULL;
}




// only for test，测试异步消息本地数据存储
void CSrvMsgHandler::testAsyncLocalData()
{
	registerProtocol(ServiceType::CommonSrv, ETestProtocolId::ETEST_ASYNC_MSG_LOCAL_DATA_REQ, (ProtocolHandler)&CSrvMsgHandler::testAsyncLocalDataRequest);
}

void CSrvMsgHandler::testAsyncLocalDataRequest(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ServiceTestReq req;
	if (!parseMsgFromBuffer(req, data, len, "test async local data request")) return;
	
	const char* testFlag = CCfg::getValue("Test", "AsyncLocalData");
	if (testFlag != NULL)
	{
		// 先获取本地数据存储空间
		char* localData = m_onlyForTestAsyncData.createData();
		if (localData != NULL) strncpy(localData, "In DB Proxy CSrvMsgHandler::testAsyncLocalDataRequest", m_onlyForTestAsyncData.getBufferSize() - 1);
	}
	
	OptInfoLog("Test Async Local Data Request, name = %s, id = %s, age = %d, gold = %d, desc = %s, testFlag = %p, localData = %p",
	req.name().c_str(), req.id().c_str(), req.age(), req.gold(), req.desc().c_str(), testFlag, m_onlyForTestAsyncData.getData());
	
	com_protocol::ServiceTestRsp rsp;
	rsp.set_result(time(NULL));
	rsp.set_desc(req.name() + req.id() + req.desc());
	
	char buffer[1024] = {0};
	unsigned int msgLen = sizeof(buffer);
	const char* msgData = serializeMsgToBuffer(rsp, buffer, msgLen, "test async local data reply");
	if (msgData != NULL) sendMessage(msgData, msgLen, srcSrvId, ETestProtocolId::ETEST_ASYNC_MSG_LOCAL_DATA_RSP);
	
	m_onlyForTestAsyncData.destroyData();
	
	OptInfoLog("Test Async Local Data Get Again, testFlag = %p, localData = %p", testFlag, m_onlyForTestAsyncData.getData());
}

