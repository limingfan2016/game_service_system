
/* author : liuxu
 * modify : limingfan
 * date : 2015.03.03
 * description : 消息处理实现
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>

#include "base/CMD5.h"
#include "CMsgHandler.h"
#include "base/Function.h"


using namespace NProject;


// md5加密结果最大长度字符串
static const unsigned int MD5PSWMaxLen = 128;
typedef char MD5ResultStr[MD5PSWMaxLen];

// 获取用户内部自动密码加密结果
static void getInnerAutoPsw(const string& username, const unsigned int timeSecs, MD5ResultStr pswResult)
{
	// 用户注册默认密码MD5加密结果
	// 秘钥明文 PSW0123456789mm
	// ！注意，秘钥明文不可更改，否则会导致之前使用该秘钥明文注册的账号校验失败！
	static const char* UserDefaultSecretKey = "C71CD5478C009DB8B749F3FF81C032B0";  // 秘钥明文 MD5(PSW0123456789mm) 结果

    MD5ResultStr mmStr = {0};
	const unsigned int mmLen = snprintf(mmStr, sizeof(MD5ResultStr) - 1, "%s%u%s", username.c_str(), timeSecs, UserDefaultSecretKey);
	NProject::md5Encrypt(mmStr, mmLen, pswResult, false);
}


// 数据记录日志文件
#define DBDataRecordLog(format, args...)   m_srvOpt.getDataLogger()->writeFile(NULL, 0, LogLevel::Info, format, ##args)


CMsgHandler::CMsgHandler()
{
	static const unsigned int MaxUserModifyStatCount = 1024;   // 数据修改统计内存块最大个数
	NEW(m_pMManageUserModifyStat, CMemManager(MaxUserModifyStatCount, MaxUserModifyStatCount, sizeof(CUserModifyStat)));

	m_pCfg = NULL;
	
	m_pLogicDBOpt = NULL;
	m_pOperationDbOpt = NULL;
	
	m_check_commit_timer = 0;
	m_db_connect_check_timer = 0;
	m_cur_connect_failed_count = 0;
}

CMsgHandler::~CMsgHandler()
{
	DELETE(m_pMManageUserModifyStat);

    m_pCfg = NULL;
	
	if (m_pLogicDBOpt != NULL)
	{
		CMySql::destroyDBOpt(m_pLogicDBOpt);
		m_pLogicDBOpt = NULL;
	}
	
	if (m_pOperationDbOpt != NULL)
	{
		CMySql::destroyDBOpt(m_pOperationDbOpt);
		m_pOperationDbOpt = NULL;
	}
	
	m_check_commit_timer = 0;
	m_db_connect_check_timer = 0;
	m_cur_connect_failed_count = 0;
}


void CMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!m_srvOpt.init(this, CCfg::getValue("Service", "ConfigFile")))
	{
		OptErrorLog("init service operation instance error");
		stopService();
		return;
	}

	// 创建数据日志配置
	if (!m_srvOpt.createDataLogger("DBDataLogger", "LogFileName", "LogFileSize", "LogFileCount"))
	{
		OptErrorLog("create data log error");
		stopService();
		return;
	}
	
	loadConfig(false);
	
	ReleaseInfoLog("--- onLoad ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}

void CMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 注册协议处理函数
	// registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_USERNAME_REQ, (ProtocolHandler)&CMsgHandler::getUsernameByPlatform);
	registerProtocol(ServiceType::GameHallSrv, DBPROXY_CHECK_USER_REQ, (ProtocolHandler)&CMsgHandler::checkUser);
	registerProtocol(ServiceType::GameHallSrv, DBPROXY_ENTER_CHESS_HALL_REQ, (ProtocolHandler)&CMsgHandler::enterChessHall);

	registerProtocol(ServiceType::CommonSrv, DBPROXY_USER_LOGIN_NOTIFY, (ProtocolHandler)&CMsgHandler::userLoginNotify);
	registerProtocol(ServiceType::CommonSrv, DBPROXY_USER_LOGOUT_NOTIFY, (ProtocolHandler)&CMsgHandler::userLogoutNotify);
	registerProtocol(ServiceType::CommonSrv, DBPROXY_SERVICE_STATUS_NOTIFY, (ProtocolHandler)&CMsgHandler::serviceStatusNotify);
	
	ReleaseInfoLog("--- onRegister ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}
	
void CMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	const DBConfig::config& dbCfg = m_srvOpt.getDBCfg();
	if (!dbCfg.isSetConfigValueSuccess())
	{
		OptErrorLog("get db xml config value error");
		stopService();

		return;
	}
	
	// memcache DB
	if (!m_memcache.addServer(dbCfg.logic_db_cfg.memcached_ip, dbCfg.logic_db_cfg.memcached_port))
	{
		OptErrorLog("memcache connect failed. ip:%s port:%d", dbCfg.logic_db_cfg.memcached_ip.c_str(), dbCfg.logic_db_cfg.memcached_port);
		stopService();
		
		return ;
	}

	// 游戏逻辑数据库
    // 设置mysql字符集（默认utf8，已不需要设置）
	if (0 != CMySql::createDBOpt(m_pLogicDBOpt, dbCfg.logic_db_cfg.ip.c_str(), dbCfg.logic_db_cfg.username.c_str(),
		dbCfg.logic_db_cfg.password.c_str(), dbCfg.logic_db_cfg.dbname.c_str(), dbCfg.logic_db_cfg.port))
	{
		OptErrorLog("mysql create logic db failed. ip:%s username:%s passwd:%s dbname:%s port:%d", dbCfg.logic_db_cfg.ip.c_str(),
		dbCfg.logic_db_cfg.username.c_str(), dbCfg.logic_db_cfg.password.c_str(), dbCfg.logic_db_cfg.dbname.c_str(), dbCfg.logic_db_cfg.port);
		stopService();
		
		return;
	}
	
	// 关闭自动commit，所有该DB的修改需要手动调用mysqlCommit()提交完成
	m_pLogicDBOpt->autoCommit(0);
	
	// 游戏运营管理数据库
	if (0 != CMySql::createDBOpt(m_pOperationDbOpt, dbCfg.opt_db_cfg.ip.c_str(),
								 dbCfg.opt_db_cfg.username.c_str(),
								 dbCfg.opt_db_cfg.password.c_str(),
								 dbCfg.opt_db_cfg.dbname.c_str(),
								 dbCfg.opt_db_cfg.port))
	{
		OptErrorLog("mysql create operation db failed. ip:%s username:%s passwd:%s dbname:%s port:%d", dbCfg.opt_db_cfg.ip.c_str(),
					dbCfg.opt_db_cfg.username.c_str(), dbCfg.opt_db_cfg.password.c_str(),
					dbCfg.opt_db_cfg.dbname.c_str(), dbCfg.opt_db_cfg.port);
		stopService();
		
		return;
	}

    // redis logic DB
	if (!m_logicRedisDbOpt.connectSvr(dbCfg.redis_db_cfg.logic_db_ip.c_str(), dbCfg.redis_db_cfg.logic_db_port,
	    dbCfg.redis_db_cfg.logic_db_timeout * MillisecondUnit))
	{
		OptErrorLog("do connect logic redis service failed, ip = %s, port = %u, time out = %u",
		dbCfg.redis_db_cfg.logic_db_ip.c_str(), dbCfg.redis_db_cfg.logic_db_port, dbCfg.redis_db_cfg.logic_db_timeout);
		stopService();
		return;
	}
	
	if (SrvOptSuccess != m_logicData.init(this))
	{
		OptErrorLog("m_logicData.init failed.");
		stopService();
		return;
	}
	
    if (SrvOptSuccess != m_logicHandler.init(this))
	{
		OptErrorLog("m_logicHandler.init failed.");
		stopService();
		return;
	}
	
	if (SrvOptSuccess != m_chessHallOpt.init(this))
	{
		OptErrorLog("m_chessHallOpt.init failed.");
		stopService();
		return;
	}
	
	if (!m_gameRecord.init(this))
	{
		OptErrorLog("m_gameRecord.init failed.");
		stopService();
		return;
	}

	// 0.1秒做一次提交Mysql操作（增加频度，减少服务延迟）
	setTimer(100, (TimerHandler)&CMsgHandler::commitToMysql, 0, NULL, -1);
	
	// 定时提交检查&DB断连检查
	m_check_commit_timer = setTimer(MillisecondUnit * dbCfg.logic_db_cfg.check_time_gap, (TimerHandler)&CMsgHandler::checkToCommit, 0, NULL, -1);
	m_db_connect_check_timer = setTimer(MillisecondUnit * dbCfg.logic_db_cfg.db_connect_check_time_gap, (TimerHandler)&CMsgHandler::dbConnectCheck, 0, NULL, -1);
	
	ReleaseInfoLog("--- onRun ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}

void CMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onUnLoad ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
	
	m_logicData.unInit();
    m_gameRecord.unInit();
	m_logicHandler.unInit();
	m_chessHallOpt.unInit();
	
	// 记录开始&结束时间
	unsigned long long start;
	unsigned long long end;
	struct timeval startTime;
	struct timeval endTime;
	gettimeofday(&startTime, NULL);
	start = startTime.tv_sec * 1000000 + startTime.tv_usec;
	time_t now_timestamp = time(NULL);

    // 可以直接提交数据的队列
	const int commit_count = m_umapCommitToDB.size();
	CUserBaseinfo user_baseinfo;
	for (unordered_map<string, CUserModifyStat*>::iterator ite = m_umapCommitToDB.begin(); ite != m_umapCommitToDB.end();)
	{
		//获取DB中的用户信息
		if (getUserBaseinfo(ite->first, "", user_baseinfo, true) != SrvOptSuccess)  // 只取memcached内存数据
		{
			OptErrorLog("onUnLoad commitToMysql error.|%s|%u|%u|%u", ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
			
			m_pMManageUserModifyStat->put((char*)(ite->second));
			m_umapCommitToDB.erase(ite++);
			
			continue;
		}

		bool ret = updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info);
		OptInfoLog("onUnLoad commitToMysql|%u|%s|%u|%u|%u", ret, ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
		
		m_pMManageUserModifyStat->put((char*)(ite->second));
		m_umapCommitToDB.erase(ite++);
	}

    // 存在数据修改的队列
    const int modify_count = m_umapHasModify.size();
	for (unordered_map<string, CUserModifyStat*>::iterator ite = m_umapHasModify.begin(); ite != m_umapHasModify.end();)
	{
		//获取DB中的用户信息
		if (getUserBaseinfo(ite->first, "", user_baseinfo, true) != SrvOptSuccess)  // 只取memcached内存数据
		{
			OptErrorLog("onUnLoad commitModifyToMysql error.|%s|%u|%u|%u", ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
			
			m_pMManageUserModifyStat->put((char*)(ite->second));
			m_umapHasModify.erase(ite++);
			
			continue;
		}

		bool ret = updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info);
		OptInfoLog("onUnLoad commitModifyToMysql|%u|%s|%u|%u|%u", ret, ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
		
		m_pMManageUserModifyStat->put((char*)(ite->second));
		m_umapHasModify.erase(ite++);
	}
	mysqlCommit();  // 提交到DB

    // 结束时间
	gettimeofday(&endTime, NULL);
	end = endTime.tv_sec * 1000000 + endTime.tv_usec;
	const unsigned long long timeuse = end - start;
	const unsigned int exe_count = commit_count + modify_count;
	ReleaseInfoLog("onUnLoad:commitToMysql time stat(us)|%llu|%u|%u|%u|%llu", timeuse, exe_count, commit_count, modify_count, timeuse / (exe_count ? exe_count : 1));

	const bool bret = m_gameRecord.commitAllGameRecord();
	ReleaseInfoLog("onUnLoad:all game record commitToMysql|%d", bret);

	m_logicRedisDbOpt.disconnectSvr();
	
	ReleaseInfoLog("--- onUnLoad --- all success.");
}

void CMsgHandler::loadConfig(bool isReset)
{
	if (isReset) m_srvOpt.updateCommonConfig(CCfg::getValue("Service", "ConfigFile"));

	m_pCfg = &DbProxyConfig::config::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str(), isReset);
	if (!m_pCfg->isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update business xml config value error");
		
		stopService();
		return;
	}
	
	if (!m_srvOpt.getDBCfg(isReset).isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update db xml config value error");
		
		stopService();
		return;
	}

	if (!m_srvOpt.getServiceCommonCfg(isReset).isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update service xml config value error");
		
		stopService();
		return;
	}

	m_ip2city.init(m_srvOpt.getCommonCfg().data_file.city_id_mapping.c_str(), m_srvOpt.getCommonCfg().data_file.ip_city_mapping.c_str());
	
	m_senitiveWordFilter.unInit();
	m_senitiveWordFilter.init(m_srvOpt.getCommonCfg().data_file.sensitive_words_file.c_str());
	
	m_logicHandler.updateConfig();
	m_gameRecord.loadConfig();
	m_chessHallOpt.updateConfig();

	// 更新timer
	const DBConfig::config& dbCfg = m_srvOpt.getDBCfg();
	if (0 != m_check_commit_timer)
	{
		killTimer(m_check_commit_timer);
		m_check_commit_timer = setTimer(MillisecondUnit * dbCfg.logic_db_cfg.check_time_gap, (TimerHandler)&CMsgHandler::checkToCommit, 0, NULL, -1);
	}
	
	if (0 != m_db_connect_check_timer)
	{
		killTimer(m_db_connect_check_timer);
		m_db_connect_check_timer = setTimer(MillisecondUnit * dbCfg.logic_db_cfg.db_connect_check_time_gap, (TimerHandler)&CMsgHandler::dbConnectCheck, 0, NULL, -1);
	}
	
	ReleaseInfoLog("update config finish");
	
	dbCfg.output();
	m_srvOpt.getServiceCommonCfg().output();
	m_pCfg->output();
}

int CMsgHandler::autoRegisterUser(string& username, string& passwd, const string& platform_id, uint32_t platform_type, uint32_t os_type,
                                  const string& nickname, const string& portrait_id, uint32_t city_id, int gender)
{
	//先找出最大ID
	char sql_tmp[2048] = {0};
	CQueryResult* p_result = NULL;
	unsigned int sqlLen = snprintf(sql_tmp, sizeof(sql_tmp) - 1, "select max(user_id) from tb_user_platform_map_info;");
	if (SrvOptSuccess != m_pLogicDBOpt->queryTableAllResult(sql_tmp, sqlLen, p_result)
	    || p_result == NULL || 1 != p_result->getRowCount())
	{
		m_pLogicDBOpt->releaseQueryResult(p_result);
		return DBProxyGetMaxIdFailed;
	}
	RowDataPtr rowData = p_result->getNextRow();
	unsigned int cur_id = (rowData[0] != NULL) ? atoi(rowData[0]) : 0;
	m_pLogicDBOpt->releaseQueryResult(p_result);
	++cur_id;

	//注册用户
	int ret = SrvOptSuccess;
	for (int i = 0; i < 3; ++i)
	{
		com_protocol::RegisterUserReq registerReq;
		snprintf(sql_tmp, sizeof(sql_tmp) - 1, "1%06u", cur_id);  // 生成内部账号ID
		registerReq.set_username(sql_tmp);
		
		(nickname.length() > 0) ? registerReq.set_nickname(nickname) : registerReq.set_nickname(registerReq.username());
		if (portrait_id.length() > 0) registerReq.set_portrait_id(portrait_id);
        if (passwd.length() > 0) registerReq.set_passwd(passwd);
		if (0 != city_id) registerReq.set_city_id(city_id);
		
		registerReq.set_gender(gender);
		
		ret = registerUser(registerReq);
		
		//注册成功
		if (SrvOptSuccess == ret)
		{
			//插入第三方ID映射信息
			sqlLen = snprintf(sql_tmp, sizeof(sql_tmp) - 1, "insert into tb_user_platform_map_info(platform_id,platform_type,username,user_id,create_time,os_type) values(\'%s\',%u,\'%s\',%u,now(),%u);", 
				              platform_id.c_str(), platform_type, registerReq.username().c_str(), cur_id, os_type);
			if (SrvOptSuccess == m_pLogicDBOpt->modifyTable(sql_tmp, sqlLen))
			{
				username = registerReq.username();
				passwd = registerReq.passwd();

				mysqlCommit();  // 成功则提交数据到DB
				
				OptInfoLog("auto register user = %s, nickname = %s, platform id = %s, type = %d, os type = %d",
				registerReq.username().c_str(), registerReq.nickname().c_str(), platform_id.c_str(), platform_type, os_type);
			}
			else
			{
				mysqlRollback();  // 失败则回滚数据
				
				OptErrorLog("register user platform info exeSql failed|%s|%s", registerReq.username().c_str(), sql_tmp);
				
				ret = DBProxyRegisterPlatformInfoFailed;
			}
			
			break;
		}
		
		++cur_id;
	}

	return ret;
}

int CMsgHandler::registerUser(com_protocol::RegisterUserReq& req)
{
	string sql("");
	int ret = getUserStaticBaseInfoSql(req, sql);
	if (SrvOptSuccess != ret) return ret;

	//插入用户静态表数据
	if (SrvOptSuccess != m_pLogicDBOpt->modifyTable(sql.c_str(), sql.length()))
	{
		OptErrorLog("register user static info exeSql failed|%s|%s", req.username().c_str(), sql.c_str());
		
		return DBProxyRegisterStaticInfoFailed;
	}

    /*
	//插入用户动态表信息
	char sql_tmp[128] = {0};
	const unsigned int sqlLen = snprintf(sql_tmp, sizeof(sql_tmp) - 1,
	                                     "insert into tb_user_dynamic_baseinfo(username, game_gold, room_card) value(\'%s\', %.2f, %.2f);",
	                                     req.username().c_str(), m_pCfg->register_init.game_gold, m_pCfg->register_init.room_card);
	if (SrvOptSuccess != m_pLogicDBOpt->modifyTable(sql_tmp, sqlLen))
	{
		mysqlRollback();
		
		OptErrorLog("register user dynamic info exeSql failed|%s|%s", req.username().c_str(), sql_tmp);
		
		return DBProxyRegisterDynamicInfoFailed;
	}
    */

	return SrvOptSuccess;
}

int CMsgHandler::getUserStaticBaseInfoSql(com_protocol::RegisterUserReq& req, string& sql)
{
	//输入合法性判定
	if (req.username().length() >= 32 || req.username().length() < 5 || !strIsAlnum(req.username().c_str()))
	{
		return DBProxyUsernameInputUnlegal;
	}
	
	if (req.nickname().length() >= 32)
	{
		return DBProxyNicknameInputUnlegal;
	}

	const string& person_sign = req.has_person_sign() ? req.person_sign() : "";
	if (person_sign.length() >= 64)
	{
		return DBProxyPersonSignInputUnlegal;
	}
	
	const string& realname = req.has_realname() ? req.realname() : "";
	if (realname.length() >= 16)
	{
		return DBProxyRealnameInputUnlegal;
	}
	
	const string& portrait_id = req.has_portrait_id() ? req.portrait_id() : "";
	if (portrait_id.length() >= 512)
	{
		return DBProxyPortraitIdInputUnlegal;
	}
	
	const string& qq_num = req.has_qq_num() ? req.qq_num() : "";
	if (qq_num != "" && (!strIsDigit(qq_num.c_str()) || qq_num.length() >= 16))
	{
		return DBProxyQQnumInputUnlegal;
	}
	
	const string& address = req.has_address() ? req.address() : "";
	if (address.length() >= 64)
	{
		return DBProxyAddrInputUnlegal;
	}
	
	const string& birthday = req.has_birthday() ? req.birthday() : "1000-01-01";
	if (!strIsDate(birthday.c_str()))
	{
		return DBProxyBirthdayInputUnlegal;
	}
	
	const int32_t gender = req.has_gender() ? req.gender() : 1;
	
	const string& mobile_phone = req.has_mobile_phone() ? req.mobile_phone() : "";
	if (mobile_phone.length() >= 16 || !strIsDigit(mobile_phone.c_str(), mobile_phone.length()))
	{
		return DBProxyMobilephoneInputUnlegal;
	}
	
	const string& email = req.has_email() ? req.email() : "";
	if (email.length() >= 36)
	{
		return DBProxyEmailInputUnlegal;
	}
	
	const uint32_t age = req.has_age() ? req.age() : 0;

	const string& idc = req.has_idc() ? req.idc() : "";
	if (idc.length() >= 24)
	{
		return DBProxyIDCInputUnlegal;
	}
	
	const string& other_contact = req.has_other_contact() ? req.other_contact() : "";
	if (other_contact.length() >= 64)
	{
		return DBProxyOtherContactUnlegal;
	}
	
	const uint32_t city_id = req.has_city_id() ? req.city_id() : 0;
	
	// 密码处理
	const unsigned int curSecs = time(NULL);
	if (req.passwd().length() < 1)
	{
		// 用户没有输入密码则默认生成内部自动密码
		MD5ResultStr pswResult = {0};
		getInnerAutoPsw(req.username(), curSecs, pswResult);
		req.set_passwd(pswResult);
	}
	
	if (req.passwd().length() != 32 || !strIsUpperHex(req.passwd().c_str()))
	{
		return DBProxyPasswdInputUnlegal;
	}
	
	// 存入db的密码
	// passwd = md5(username + md5(passwd))
	CMD5 md5;
	string password = req.username() + req.passwd();
	md5.GenerateMD5((const unsigned char*)password.c_str(), password.length());
	password = md5.ToString();

	char str_tmp[2048] = {0};
	snprintf(str_tmp, sizeof(str_tmp) - 1,
	         "insert into tb_user_static_baseinfo(register_time, username, password, nickname, person_sign, realname, portrait_id, \
			 qq_num, address, birthday, gender, mobile_phone, email, age, idc, other_contact, city_id) value(%u,\'%s\',\
			 \'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',\'%s\',%d,\'%s\',\'%s\',%u,\'%s\',\'%s\',%u);",
			 curSecs, req.username().c_str(), password.c_str(), req.nickname().c_str(), person_sign.c_str(), realname.c_str(), portrait_id.c_str(),
			 qq_num.c_str(), address.c_str(), birthday.c_str(), gender, mobile_phone.c_str(), email.c_str(), age, idc.c_str(),
			 other_contact.c_str(), city_id);
			 
	sql = str_tmp;

	return SrvOptSuccess;
}

// 根据第三方平台账号获取内部账号ID
// 如没有则新注册一个对应该第三方平台账号的内部账户ID
bool CMsgHandler::getAndRegisterUserByPlatform(const com_protocol::GetUsernameByPlatformReq& req, com_protocol::GetUsernameByPlatformRsp& rsp)
{
	bool isNewRegisterUser = false;
	rsp.set_result(SrvOptSuccess);

	do
	{
		//验证platform_id合法性
		if (req.platform_id().length() > 64)
		{
			rsp.set_result(DBProxyPlatformIdUnlegal);
			break;
		}

		//先从memcached查看是否存在
		vector<char> ret_val;
		uint32_t flags = 0;
		char mcIdValue[128] = { 0 };
		snprintf(mcIdValue, sizeof(mcIdValue) - 1, "%03u-%s", req.platform_type(), req.platform_id().c_str());
		if (m_memcache.get(mcIdValue, ret_val, flags))
		{
			rsp.set_username(&ret_val[0], ret_val.size());
			break;
		}

		//再找DB
		char sql_tmp[2048] = { 0 };
		CQueryResult* p_result = NULL;
		const unsigned int sqlLen = snprintf(sql_tmp, sizeof(sql_tmp) - 1, "select username from tb_user_platform_map_info where platform_id=\'%s\' and platform_type=%u;", req.platform_id().c_str(), req.platform_type());
		if (SrvOptSuccess == m_pLogicDBOpt->queryTableAllResult(sql_tmp, sqlLen, p_result)
		    && p_result != NULL && 1 == p_result->getRowCount())
		{
			rsp.set_username(p_result->getNextRow()[0]);
			m_pLogicDBOpt->releaseQueryResult(p_result);
			
			//缓存到memcached
			m_memcache.set(mcIdValue, rsp.username().c_str(), rsp.username().length(), 0, flags);
			break;
		}
		m_pLogicDBOpt->releaseQueryResult(p_result);

		//都没有，注册一个
		string nickname("");
		if (req.has_nickname())
		{
			//敏感字符过滤
			if (getSensitiveWordFilter().isExistSensitiveStr((const unsigned char*)req.nickname().c_str(), req.nickname().length()))
			{
				rsp.set_result(DBProxyNicknameSensitiveStr);
			    break;
			}

			// nickname长度限制
			nickname = utf8SubStr(req.nickname().c_str(), req.nickname().length(), 0, m_pCfg->common_cfg.nickname_length);
		}
		
		string city_name("");
		uint32_t city_id = 0;
		if (req.has_login_ip()) m_ip2city.getCityInfoByIP(req.login_ip().c_str(), city_name, city_id);

        // 注册新用户
        string username("");
		string password = (req.passwd().length() > 0) ? req.passwd() : "";
		int ret = autoRegisterUser(username, password, req.platform_id(), req.platform_type(), req.os_type(), nickname, req.portrait_id(), city_id, req.has_gender() ? req.gender() : 1);
		if (SrvOptSuccess != ret)
		{
			OptErrorLog("auto register user error, ret = %d", ret);
			rsp.set_result(ret);
			break;
		}

		// 第一次新注册的账户则传回密码
		isNewRegisterUser = true;
		rsp.set_username(username);
		rsp.set_passwd(password);
		
		OptInfoLog("auto register user, mcIdValue = %s, platform id = %s, type = %u, username = %s",
		mcIdValue, req.platform_id().c_str(), req.platform_type(), rsp.username().c_str());

	} while (false);
	
	return isNewRegisterUser;
}

// 验证用户账号信息
int CMsgHandler::checkUserAccount(com_protocol::ClientLoginReq& req, CUserBaseinfo& user_baseinfo)
{
	// 获取DB中的用户信息，这里只取玩家的静态数据（如果内存DB中没有的话），hall_id 参数值为空
	int rc = getUserBaseinfo(req.username(), "", user_baseinfo);
	if (rc != SrvOptSuccess) return rc;  // 获取DB中的信息失败
	
	// 先查看帐号状态是否已经被锁定
	if (0 != user_baseinfo.static_info.status) return DBProxyUserLimitLogin;
	
	// 检查该账号是否被锁定了
	char sqlBuffer[2048] = {0};
	unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "select username,login_id,login_ip from tb_locked_account where username=\'%s\' or login_ip=\'%s\';",
	req.username().c_str(), req.login_ip().c_str());
	
	// 兼容处理，存在login id
	if (req.login_id().length() > 0)
	{
		--sqlLen;  // 去掉语句最后的分号，重新组装新的sql语句添加 login id 条件信息
		sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, " or login_id=\'%s\';", req.login_id().c_str());
	}
	
	// 检查该账号是否被锁定了
	CQueryResult* p_result = NULL;
	if (SrvOptSuccess == getOptDBOpt()->queryTableAllResult(sqlBuffer, sqlLen, p_result)
	    && p_result != NULL && p_result->getRowCount() > 0)
	{
		OptErrorLog("the user account has be locked, name = %s, login id = %s, login ip = %s, count = %d",
		req.username().c_str(), req.login_id().c_str(), req.login_ip().c_str(), p_result->getRowCount());
		
		int result = DBProxyUserLimitLogin;
		RowDataPtr rowData = NULL;
		while ((rowData = p_result->getNextRow()) != NULL)
		{
			if (strcmp(rowData[2], req.login_ip().c_str()) == 0)      // IP
			{
				result = DBProxyIPLimitLogin;
				break;
			}
			else if (req.login_id().length() > 0 && strcmp(rowData[1], req.login_id().c_str()) == 0)           // 设备ID
			{
				result = DBProxyDeviceLimitLogin;
				break;
			}
		}
		
		getOptDBOpt()->releaseQueryResult(p_result);

		return result;
	}
	getOptDBOpt()->releaseQueryResult(p_result);

    // 密码处理
    if (req.passwd().length() < 1)
	{
		// 用户没有输入密码则使用默认自动生成的内部密码
		MD5ResultStr pswResult = {0};
		getInnerAutoPsw(req.username(), user_baseinfo.static_info.register_time, pswResult);
		req.set_passwd(pswResult);
	}

	// 密码校验
	CMD5 md5;
	sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "%s%s", req.username().c_str(), req.passwd().c_str());
	md5.GenerateMD5((const unsigned char*)sqlBuffer, sqlLen);

	if (md5.ToString() != user_baseinfo.static_info.password)
    {
        OptErrorLog("check password error, username = %s, passwd = %s, md5 = %s, db = %s",
		req.username().c_str(), req.passwd().c_str(), md5.ToString().c_str(), user_baseinfo.static_info.password);
            
        return DBProxyPasswordVerifyFailed;  // 密码校验失败
    }
	
	// 更新登陆ID&IP
	sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "update tb_user_platform_map_info set login_id=\'%s\',login_ip=\'%s\' where username=\'%s\';",
	req.login_id().c_str(), req.login_ip().c_str(), req.username().c_str());
	if (getLogicDBOpt()->executeSQL(sqlBuffer, sqlLen) == SrvOptSuccess) mysqlCommit();
	else mysqlRollback();

	// 是否需要更新用户静态数据，微信登录每次都检查玩家的基本信息，
	// 玩家可能修改了微信的昵称、头像、性别等信息则需要更新
	bool isUpdateStaticInfo = false;
	if (req.has_user_input_nickname()
	    && req.user_input_nickname() != user_baseinfo.static_info.nickname
		&& strcmp(user_baseinfo.static_info.nickname, user_baseinfo.static_info.username) == 0)
	{
		isUpdateStaticInfo = true;
		strncpy(user_baseinfo.static_info.nickname, req.user_input_nickname().c_str(), sizeof(user_baseinfo.static_info.nickname) - 1);
	}
	
	if (req.has_user_input_portrait_id() && req.user_input_portrait_id() != user_baseinfo.static_info.portrait_id)
	{
		isUpdateStaticInfo = true;
		strncpy(user_baseinfo.static_info.portrait_id, req.user_input_portrait_id().c_str(), sizeof(user_baseinfo.static_info.portrait_id) - 1);
	}
	
	if (req.has_user_input_gender() && req.user_input_gender() != user_baseinfo.static_info.gender)
	{
		isUpdateStaticInfo = true;
		user_baseinfo.static_info.gender = req.user_input_gender();
	}
	
	// 这里如果有修改就实时刷新到DB
	if (isUpdateStaticInfo && updateUserStaticBaseinfoToMysql(user_baseinfo.static_info))
	{
		if (setUserBaseinfoToMemcached(user_baseinfo))
		{
			mysqlCommit();
		}
		else
		{
			mysqlRollback();

			OptErrorLog("update user static info error, username = %s, nickname = %s",
			user_baseinfo.static_info.username, user_baseinfo.static_info.nickname);
		}
	}

	return SrvOptSuccess;
}

int CMsgHandler::setUserLoginInfo(const CUserBaseinfo& user_baseinfo, com_protocol::ClientLoginRsp& rsp)
{
	setUserDetailInfo(user_baseinfo, *rsp.mutable_detail_info());

	// 用户所在棋牌室信息
	return m_chessHallOpt.getUserChessHallBaseInfo(user_baseinfo.static_info.username, *rsp.mutable_hall_base_info());
}


void CMsgHandler::commitToMysql(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	if (m_umapCommitToDB.empty()) return;

	unsigned long long start;
	unsigned long long end;
	struct timeval startTime;
	struct timeval endTime;
	gettimeofday(&startTime, NULL);
	start = startTime.tv_sec * 1000000 + startTime.tv_usec;

    const DBConfig::config& dbCfg = m_srvOpt.getDBCfg();
	time_t now_timestamp = time(NULL);
	int count = (int)dbCfg.logic_db_cfg.max_commit_count;
	for (unordered_map<string, CUserModifyStat*>::iterator ite = m_umapCommitToDB.begin();
	     count > 0 && ite != m_umapCommitToDB.end(); --count)
	{
		// 获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (getUserBaseinfo(ite->first, "", user_baseinfo, true) != SrvOptSuccess)  // 只取memcached内存数据
		{
			OptErrorLog("commitToMysql error.|%s|%u|%u|%u", ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
			m_pMManageUserModifyStat->put((char*)(ite->second));
			m_umapCommitToDB.erase(ite++);
			
			continue;
		}

		bool ret = updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info);
		OptInfoLog("commitToMysql|%u|%s|%u|%u|%u", ret, ite->first.c_str(), ite->second->modify_count, now_timestamp - ite->second->first_modify_timestamp, ite->second->is_logout);
		m_pMManageUserModifyStat->put((char*)(ite->second));
		m_umapCommitToDB.erase(ite++);
	}

	if (count < (int)dbCfg.logic_db_cfg.max_commit_count)
	{
		mysqlCommit();

		gettimeofday(&endTime, NULL);
		end = endTime.tv_sec * 1000000 + endTime.tv_usec;
		unsigned long long timeuse = end - start;
		int exe_count = dbCfg.logic_db_cfg.max_commit_count - count;
		ReleaseInfoLog("commitToMysql time stat(us)|%llu|%u|%llu", timeuse, exe_count, timeuse / exe_count);
	}
	
	return;
}

void CMsgHandler::checkToCommit(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	unsigned long long start;
	unsigned long long end;
	struct timeval startTime;
	struct timeval endTime;
	gettimeofday(&startTime, NULL);
	start = startTime.tv_sec * 1000000 + startTime.tv_usec;

	unsigned int count = m_umapHasModify.size();
	unsigned int cur_timestamp = time(NULL);
	for (unordered_map<string, CUserModifyStat*>::iterator ite = m_umapHasModify.begin(); ite != m_umapHasModify.end();)
	{
		if (cur_timestamp - ite->second->first_modify_timestamp >= (uint32_t)m_srvOpt.getDBCfg().logic_db_cfg.commit_need_time)
		{
			m_umapCommitToDB[ite->first] = ite->second;
			m_umapHasModify.erase(ite++);
		}
		else
		{
			ite++;
		}
	}
	
	m_gameRecord.checkCommitGameRecord();

	gettimeofday(&endTime, NULL);
	end = endTime.tv_sec * 1000000 + endTime.tv_usec;
	unsigned long long timeuse = end - start;
	ReleaseInfoLog("checkToCommit && checkCommitRecord time stat(us)|%llu|%u|%u|%u", timeuse, count, m_umapHasModify.size(), m_umapCommitToDB.size());

	return;
}

bool CMsgHandler::dbIsConnect()
{
	static const string scTestConnectNormal = "t-TCN-";  // 测试连接数据
	
	// 检测memcached 连接
	const DBConfig::config& dbCfg = m_srvOpt.getDBCfg();
	if (!m_memcache.set(scTestConnectNormal, scTestConnectNormal.c_str(), scTestConnectNormal.length(), 1, 0))
	{
		ReleaseErrorLog("TIME CHECK|memcache connect failed, ip:%s port:%d", dbCfg.logic_db_cfg.memcached_ip.c_str(), dbCfg.logic_db_cfg.memcached_port);
		return false;
	}

	// 检测Mysql 连接
	char sql[32] = { 0 };
	CQueryResult* p_result = NULL;
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "select now();");
	if (Success != m_pLogicDBOpt->queryTableAllResult(sql, sqlLen, p_result))
	{
		ReleaseErrorLog("TIME CHECK|logic db exec sql error:%s", sql);
		m_pLogicDBOpt->releaseQueryResult(p_result);
		return false;
	}
	m_pLogicDBOpt->releaseQueryResult(p_result);
	
	p_result = NULL;
	if (Success != m_pOperationDbOpt->queryTableAllResult(sql, sqlLen, p_result))
	{
		ReleaseErrorLog("TIME CHECK|operation db exec sql error:%s", sql);
		m_pOperationDbOpt->releaseQueryResult(p_result);
		return false;
	}
	m_pOperationDbOpt->releaseQueryResult(p_result);
	
	// 检测redis连接
	if (0 != m_logicRedisDbOpt.setKey(scTestConnectNormal.c_str(), scTestConnectNormal.length(),
	    scTestConnectNormal.c_str(), scTestConnectNormal.length()))
	{
		ReleaseErrorLog("TIME CHECK|redis connect failed, ip:%s port:%d",
		dbCfg.redis_db_cfg.logic_db_ip.c_str(), dbCfg.redis_db_cfg.logic_db_port);
		return false;
	}

	return true;
}

void CMsgHandler::dbConnectCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	if (!dbIsConnect())
	{
		m_cur_connect_failed_count++;
		setTimer(MillisecondUnit * 1, (TimerHandler)&CMsgHandler::dbHasDisconnectCheck, 0, NULL, 1);
	}
	else
	{
		m_cur_connect_failed_count = 0;
	}
}

void CMsgHandler::dbHasDisconnectCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	if (!dbIsConnect())
	{
		m_cur_connect_failed_count++;
		if (m_cur_connect_failed_count >= m_srvOpt.getDBCfg().logic_db_cfg.db_connect_check_times)
		{
			com_protocol::ServiceStatusNotify srvStatusNf;
			srvStatusNf.set_service_id(getSrvId());
			srvStatusNf.set_service_status(com_protocol::EServiceStatus::ESrvFaultStop);
			srvStatusNf.set_service_type(DBProxySrv);
			srvStatusNf.set_service_name("DBProxy");
			m_srvOpt.sendPkgMsgToService("", 0, MessageCenterSrv, srvStatusNf, MSGCENTER_SERVICE_STATUS_NOTIFY, "db proxy service stop notify");

			ReleaseErrorLog("db connect failed and stop service, times:%u", m_cur_connect_failed_count);
			stopService();
			
			return;
		}
		
		setTimer(MillisecondUnit * 1, (TimerHandler)&CMsgHandler::dbHasDisconnectCheck, 0, NULL, 1);
	}
	else
	{
		m_cur_connect_failed_count = 0;
	}
}


// 获取服务的逻辑数据
int CMsgHandler::getServiceData(const char* firstKey, const unsigned int firstKeyLen, const char* secondKey, const unsigned int secondKeyLen, ::google::protobuf::Message& msg)
{
	int result = SrvOptFailed;
	char msgBuffer[MaxMsgLen] = {0};
	int gameDataLen = getLogicRedisService().getHField(firstKey, firstKeyLen, secondKey, secondKeyLen, msgBuffer, MaxMsgLen);
	if (gameDataLen > 0)
	{
		result = msg.ParseFromArray(msgBuffer, gameDataLen) ? 0 : DBProxyParseFromArrayError;
	}
	else
	{
		result = gameDataLen;
	}
	
	if (result != 0) OptErrorLog("get service data error, key first = %s, second = %s, result = %d", firstKey, secondKey, result);
	
	return result;
}

string CMsgHandler::utf8SubStr(const char* str, unsigned int strLen, unsigned int start, unsigned int num)
{
	if (strLen <= (start + num)) return string(str, start, strLen);

	int realNum = num;
	char last = str[start + realNum - 1];
	while ((last >> 7) == 1)
	{
		if (--realNum <= 0) break;

		last = str[start + realNum - 1];
	}

	return string(str, start, realNum);
}

// 获取商城物品配置信息
const MallConfigData::MallGoodsCfg* CMsgHandler::getMallGoodsCfg(unsigned int type)
{
	const vector<MallConfigData::MallGoodsCfg>& mallGoodCfg = m_srvOpt.getMallCfg().mall_good_array;
	for (vector<MallConfigData::MallGoodsCfg>::const_iterator it = mallGoodCfg.begin(); it != mallGoodCfg.end(); ++it)
	{
		if (it->type == type) return &(*it);
	}
	
	return NULL;
}

int CMsgHandler::getUserBaseinfo(const string& username, const string& hall_id, CUserBaseinfo& user_baseinfo, bool just_memcached, bool just_db)
{
	if (!just_db)
	{
		// 先从memcached中获取数据
		vector<char> ret_val;
		uint32_t flags = 0;
		bool getMemDataOk = m_memcache.get(username, ret_val, flags);
		if (getMemDataOk && sizeof(user_baseinfo) == ret_val.size())
		{
			memcpy((void*)&user_baseinfo, (void*)&ret_val[0], ret_val.size());
			
			if (!hall_id.empty() && hall_id != user_baseinfo.dynamic_info.hall_id)
			{
				return getUserDynamicInfoFromMysql(username, hall_id, user_baseinfo.dynamic_info);
			}
			
			return SrvOptSuccess;
		}

		if (just_memcached)
		{
			std::string errorMsg;
			m_memcache.error(errorMsg);
			OptWarnLog("get user data from memcache failed, user = %s, result = %d, info size = %u, value size = %u, flags = %u, errorMsg = %s",
			username.c_str(), getMemDataOk, sizeof(user_baseinfo), ret_val.size(), flags, errorMsg.c_str());
			
			return DBProxyGetUserInfoFromMemcachedFailed;
		}
	}

	// 从mysql中查找
	int rc = getUserStaticInfoFromMysql(username, user_baseinfo.static_info);
	if (rc != SrvOptSuccess) return rc;

	if (hall_id.empty())
	{
		memset(&user_baseinfo.dynamic_info, 0, sizeof(user_baseinfo.dynamic_info));

		return SrvOptSuccess;
	}
	
	return getUserDynamicInfoFromMysql(username, hall_id, user_baseinfo.dynamic_info);
}

int CMsgHandler::getUserStaticInfoFromMysql(const string& username, CUserStaticBaseinfo& static_info)
{
	// 从mysql中查找
	char sql[2048] = {0};
	CQueryResult* p_result = NULL;
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "select register_time,username,password,nickname,portrait_id,person_sign,mobile_phone,"
								         "email,gender,city_id,status from tb_user_static_baseinfo where username=\'%s\';", username.c_str());
	if (SrvOptSuccess != m_pLogicDBOpt->queryTableAllResult(sql, sqlLen, p_result)
	    || p_result == NULL || 1 != p_result->getRowCount())
	{
		OptErrorLog("get user static info exec sql error:%s", sql);
		m_pLogicDBOpt->releaseQueryResult(p_result);
		
		return DBProxyGetUserStaticInfoFailed;
	}
	
	RowDataPtr rowData = p_result->getNextRow();
	static_info.register_time = atoi(rowData[0]);
	strncpy(static_info.username, rowData[1], sizeof(static_info.username) - 1);
	strncpy(static_info.password, rowData[2], sizeof(static_info.password) - 1);
	strncpy(static_info.nickname, rowData[3], sizeof(static_info.nickname) - 1);
	strncpy(static_info.portrait_id, rowData[4], sizeof(static_info.portrait_id) - 1);
	strncpy(static_info.person_sign, rowData[5], sizeof(static_info.person_sign) - 1);
	strncpy(static_info.mobile_phone, rowData[6], sizeof(static_info.mobile_phone) - 1);
	strncpy(static_info.email, rowData[7], sizeof(static_info.email) - 1);
	static_info.gender = atoi(rowData[8]);
	static_info.city_id = atoi(rowData[9]);
	static_info.status = atoi(rowData[10]);

	m_pLogicDBOpt->releaseQueryResult(p_result);
	
	return SrvOptSuccess;
}

int CMsgHandler::getUserDynamicInfoFromMysql(const string& username, const string& hall_id, CUserDynamicBaseinfo& dynamic_info)
{
 	// 从mysql中查找
	char sql[2048] = {0};
	CQueryResult* p_result = NULL;
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "select UNIX_TIMESTAMP(create_time),hall_id,username,user_status,rmb_gold,game_gold,room_card,profit_loss,tax_cost,\
					                     exchange,recharge,recharge_times,sum_online_secs,sum_login_times,login_ip \
					                     from tb_chess_hall_user_info where username=\'%s\' and hall_id=\'%s\';", username.c_str(), hall_id.c_str());
	if (SrvOptSuccess != m_pLogicDBOpt->queryTableAllResult(sql, sqlLen, p_result)
        || p_result == NULL || 1 != p_result->getRowCount())
	{
		OptErrorLog("get chess hall user dynamic info exec sql error:%s", sql);
		m_pLogicDBOpt->releaseQueryResult(p_result);

		return DBProxyGetUserDynamicInfoFailed;
	}
	
	RowDataPtr rowData = p_result->getNextRow();
	dynamic_info.create_time = atoi(rowData[0]);
	strncpy(dynamic_info.hall_id, rowData[1], sizeof(dynamic_info.hall_id) - 1);
	strncpy(dynamic_info.username, rowData[2], sizeof(dynamic_info.username) - 1);
	dynamic_info.user_status = atoi(rowData[3]);
	dynamic_info.rmb_gold = atof(rowData[4]);
	dynamic_info.game_gold = atof(rowData[5]);
	dynamic_info.room_card = atof(rowData[6]);
	dynamic_info.profit_loss = atof(rowData[7]);
	dynamic_info.tax_cost = atof(rowData[8]);
	dynamic_info.exchange = atof(rowData[9]);
	dynamic_info.recharge = atof(rowData[10]);
	dynamic_info.recharge_times = atoi(rowData[11]);
	dynamic_info.sum_online_secs = atoi(rowData[12]);
	dynamic_info.sum_login_times = atoi(rowData[13]);
	strncpy(dynamic_info.login_ip, rowData[14], sizeof(dynamic_info.login_ip) - 1);
	
	m_pLogicDBOpt->releaseQueryResult(p_result);

	return SrvOptSuccess;
}

void CMsgHandler::setUserDetailInfo(const CUserBaseinfo& user_baseinfo, com_protocol::DetailInfo& detailInfo)
{
	setUserDetailInfo(user_baseinfo, detailInfo.mutable_static_info(), detailInfo.mutable_dynamic_info());
}

void CMsgHandler::setUserDetailInfo(const CUserBaseinfo& user_baseinfo, com_protocol::StaticInfo* staticInfo, com_protocol::DynamicInfo* dynamicInfo)
{
	// 用户静态信息
	if (staticInfo != NULL)
	{
		staticInfo->set_username(user_baseinfo.static_info.username);
		staticInfo->set_nickname(user_baseinfo.static_info.nickname);
		staticInfo->set_portrait_id(user_baseinfo.static_info.portrait_id);
		staticInfo->set_gender(user_baseinfo.static_info.gender);
		staticInfo->set_city_id(user_baseinfo.static_info.city_id);
		if (*user_baseinfo.static_info.mobile_phone != '\0') staticInfo->set_mobile_phone(user_baseinfo.static_info.mobile_phone);
	}
	
	// 用户动态信息
	if (dynamicInfo != NULL)
	{
	    dynamicInfo->set_game_gold(user_baseinfo.dynamic_info.game_gold);
	    if (*user_baseinfo.dynamic_info.login_ip != '\0') dynamicInfo->set_login_ip(user_baseinfo.dynamic_info.login_ip);
	}
}

CDBOpertaion* CMsgHandler::getLogicDBOpt()
{
	return m_pLogicDBOpt;
}

CDBOpertaion* CMsgHandler::getOptDBOpt()
{
	return m_pOperationDbOpt;
}

bool CMsgHandler::mysqlRollback()
{
	bool ret = m_pLogicDBOpt->rollback();
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

bool CMsgHandler::mysqlCommit()
{
	if (!m_pLogicDBOpt)
	{
		return false;
	}

	bool ret = m_pLogicDBOpt->commit();
	if (!ret)
	{
		const char* msg = m_pLogicDBOpt->errorInfo();
		if (msg == NULL) msg = "";
		OptErrorLog("exe mysql commit failed, error = %u, msg = %s", m_pLogicDBOpt->errorNum(), msg);
	}
	
	return ret;
}

bool CMsgHandler::setUserBaseinfoToMemcached(const CUserBaseinfo& user_baseinfo)
{
	uint32_t flags = 0;
	bool ret = m_memcache.set(user_baseinfo.static_info.username, (char*)&user_baseinfo, sizeof(user_baseinfo), 0, flags);
	if (!ret)
	{
	    std::string errorMsg;
		m_memcache.error(errorMsg);
		
		OptErrorLog("set base info to memcache error, username = %s, nickname = %s, game_gold = %.2f, errorMsg = %s",
	    user_baseinfo.static_info.username, user_baseinfo.static_info.nickname, user_baseinfo.dynamic_info.game_gold, errorMsg.c_str());
	}

	return ret;
}

bool CMsgHandler::updateUserStaticBaseinfoToMysql(const CUserStaticBaseinfo& user_static_baseinfo)
{
	char sql[2048] = {0};
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "update tb_user_static_baseinfo set password=\'%s\',nickname=\'%s\',portrait_id=\'%s\',person_sign=\'%s\',\
							   mobile_phone=\'%s\',email=\'%s\',gender=%d,city_id=%u,status=%u where username=\'%s\';", user_static_baseinfo.password,
							   user_static_baseinfo.nickname, user_static_baseinfo.portrait_id, user_static_baseinfo.person_sign, user_static_baseinfo.mobile_phone,
							   user_static_baseinfo.email, user_static_baseinfo.gender, user_static_baseinfo.city_id, user_static_baseinfo.status, user_static_baseinfo.username);
	if (SrvOptSuccess != m_pLogicDBOpt->modifyTable(sql, sqlLen))
	{
		OptErrorLog("update user static info exeSql failed|%s|%s", user_static_baseinfo.username, sql);
		return false;
	}

	return true;
}

bool CMsgHandler::updateUserDynamicBaseinfoToMysql(const CUserDynamicBaseinfo& user_dynamic_baseinfo)
{
	if (*user_dynamic_baseinfo.username == '\0' || *user_dynamic_baseinfo.hall_id == '\0') return false;

	char sql[2048] = {0};
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "update tb_chess_hall_user_info set user_status=%u,rmb_gold=%.2f,\
							   game_gold=%.2f,room_card=%.2f,profit_loss=%.2f,tax_cost=%.2f,exchange=%.2f,recharge=%.2f,recharge_times=%u,\
							   sum_online_secs=%u,sum_login_times=%u,login_ip=\'%s\' where username=\'%s\' and hall_id=\'%s\';", 
							   user_dynamic_baseinfo.user_status, user_dynamic_baseinfo.rmb_gold, user_dynamic_baseinfo.game_gold,
							   user_dynamic_baseinfo.room_card, user_dynamic_baseinfo.profit_loss, user_dynamic_baseinfo.tax_cost,
							   user_dynamic_baseinfo.exchange, user_dynamic_baseinfo.recharge, user_dynamic_baseinfo.recharge_times,
							   user_dynamic_baseinfo.sum_online_secs, user_dynamic_baseinfo.sum_login_times,user_dynamic_baseinfo.login_ip,
							   user_dynamic_baseinfo.username, user_dynamic_baseinfo.hall_id);
	if (SrvOptSuccess != m_pLogicDBOpt->modifyTable(sql, sqlLen))
	{
		OptErrorLog("update chess hall user dynamic info exeSql failed|%s|%s|%s",
		user_dynamic_baseinfo.username, user_dynamic_baseinfo.hall_id, sql);
		return false;
	}

	return true;
}

void CMsgHandler::updateOptStatus(const string& username, EUpdateDBOpt upate_opt)
{
	// 先检查是否已经在提交的队列里了
	unordered_map<string, CUserModifyStat*>::iterator ite = m_umapCommitToDB.find(username);
	if (ite != m_umapCommitToDB.end())
	{
		ite->second->modify_count++;
		if(upate_opt == ELogoutOpt) ite->second->is_logout = true;
		
		return;
	}

    // 再检查是否已经在修改的队列里了
	ite = m_umapHasModify.find(username);
	if (ite != m_umapHasModify.end())
	{
		ite->second->modify_count++;
		if (upate_opt == ELogoutOpt)
		{
			// 移到直接提交的队列
			ite->second->is_logout = true;
			m_umapCommitToDB[username] = ite->second;
			m_umapHasModify.erase(ite);
		}
		else if (ite->second->modify_count >= (uint32_t)m_srvOpt.getDBCfg().logic_db_cfg.commit_need_count)
		{
			// 移到直接提交的队列
			m_umapCommitToDB[username] = ite->second;
			m_umapHasModify.erase(ite);
		}
		
		return;
	}
     
	// 都没有就新建统计数据放入修改队列
	CUserModifyStat* puser_modify_stat = (CUserModifyStat*)m_pMManageUserModifyStat->get();
	puser_modify_stat->first_modify_timestamp = time(NULL);
	if (upate_opt == ELogoutOpt)
	{
		puser_modify_stat->is_logout = true;
		puser_modify_stat->modify_count = 0;
		m_umapCommitToDB[username] = puser_modify_stat;
	}
	else
	{
		puser_modify_stat->is_logout = false;
		puser_modify_stat->modify_count = 1;
		m_umapHasModify[username] = puser_modify_stat;
	}

	return;
}


CServiceOperation& CMsgHandler::getSrvOpt()
{
	return m_srvOpt;
}

CDBLogicHandler& CMsgHandler::getLogicHandler()
{
	return m_logicHandler;
}

CRedis& CMsgHandler::getLogicRedisService()
{
	return m_logicRedisDbOpt;
}

CSensitiveWordFilter& CMsgHandler::getSensitiveWordFilter()
{
	return m_senitiveWordFilter;
}

CServiceLogicData& CMsgHandler::getSrvLogicData()
{
	return m_logicData;
}

CGameRecord& CMsgHandler::getGameRecord()
{
	return m_gameRecord;
}

void CMsgHandler::onUpdateConfig()
{
	loadConfig(true);	
}


// 根据第三方平台账号获取内部账号ID
// 如没有则新注册一个对应该第三方平台账号的内部账户ID
void CMsgHandler::getUsernameByPlatform(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUsernameByPlatformReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "get username by platform request")) return;

	com_protocol::GetUsernameByPlatformRsp rsp;
	getAndRegisterUserByPlatform(req, rsp);
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_USERNAME_RSP;
    m_srvOpt.sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get user name reply");
}

// 验证用户，成功则返回用户信息
void CMsgHandler::checkUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientLoginReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "check user request")) return;
	
	com_protocol::ClientLoginRsp rsp;
	while (true)
	{
		// 没有用户名则先查找&注册
		if (!req.has_username())
		{
			com_protocol::GetUsernameByPlatformReq getUserReq;
			getUserReq.set_platform_type(req.platform_type());
			getUserReq.set_platform_id(req.platform_id());
			getUserReq.set_os_type(req.os_type());
			if (req.has_login_ip()) getUserReq.set_login_ip(req.login_ip());
			
			if (req.has_user_input_passwd()) getUserReq.set_passwd(req.user_input_passwd());
			if (req.has_user_input_nickname()) getUserReq.set_nickname(req.user_input_nickname());
			if (req.has_user_input_portrait_id()) getUserReq.set_portrait_id(req.user_input_portrait_id());
			if (req.has_user_input_gender()) getUserReq.set_gender(req.user_input_gender());
			
			com_protocol::GetUsernameByPlatformRsp getUserRsp;
			bool isNewRegisterUser = getAndRegisterUserByPlatform(getUserReq, getUserRsp);
			if (getUserRsp.result() != SrvOptSuccess)
			{
				rsp.set_result(getUserRsp.result());
				break;
			}
			
			req.set_username(getUserRsp.username());
			
			// 密码来源：1）用户输入  2）用户没有输入则内部自动生成
			if (isNewRegisterUser) req.set_passwd(getUserRsp.passwd());
		}

		else if (req.passwd().length() < 1)
		{
			// 存在用户名则必须同时存在密码，否则非法登陆
			rsp.set_result(DBProxyPasswdInputUnlegal);
			break;
		}

		CUserBaseinfo user_baseinfo;
		rsp.set_result(checkUserAccount(req, user_baseinfo));
		if (rsp.result() != SrvOptSuccess) break;

        // 用户信息
		rsp.set_result(setUserLoginInfo(user_baseinfo, rsp));
		if (rsp.result() != SrvOptSuccess) break;
		
		rsp.set_platform_id(req.platform_id());
		rsp.set_username(req.username());
		rsp.set_passwd(req.passwd());

		break;
	}
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_CHECK_USER_RSP;
    m_srvOpt.sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "check user reply");
}

// 玩家进入棋牌室大厅
void CMsgHandler::enterChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientEnterHallReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "user enter chess hall request")) return;
	
	int rc = SrvOptFailed;
	com_protocol::ClientEnterHallRsp rsp;
	do
	{
		if (req.hall_id().empty())
		{
			rsp.set_result(DBProxyChessHallIdParamError);
			break;
		}
		
		if (req.is_apply() == 1)
		{
			rc = m_chessHallOpt.applyJoinChessHall(getContext().userData, req.hall_id(), req.explain_msg(), srcSrvId);
			if (rc != SrvOptSuccess && rc != DBProxyPlayerAlreadyInHallError)
			{
				rsp.set_result(rc);
				break;
			}
			
			// 正常的申请则获取棋牌室管理员ID，通知C端在线的管理员
			if (rc == SrvOptSuccess)
			{
				rc = m_chessHallOpt.getManagerIdList(req.hall_id().c_str(), *rsp.mutable_managers());
				if (rc != SrvOptSuccess)
				{
					rsp.set_result(rc);
					break;
				}
			}
		}
		
		// 获取玩家在该棋牌室的信息
		CUserBaseinfo user_baseinfo;
		rc = getUserBaseinfo(getContext().userData, req.hall_id(), user_baseinfo);
	    if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		// 设置玩家在该棋牌室的信息
		setUserDetailInfo(user_baseinfo, *rsp.mutable_detail_info());
		
		// 获取用户所在棋牌室的管理员级别
        // 当前玩家用户也有可能是该棋牌室的管理员
		int manageLevel = 0;
        rc = m_chessHallOpt.getUserManageLevel(getContext().userData, req.hall_id().c_str(), manageLevel);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		if (manageLevel > 0) rsp.set_manager_level((com_protocol::EManagerLevel)manageLevel);
		
		// 获取用户所在的棋牌室当前所有信息
	    rc = m_chessHallOpt.getUserChessHallInfo(getContext().userData, req.hall_id().c_str(), req.max_online_rooms(),
												 *rsp.mutable_hall_info(), rsp.mutable_give_gold_info(), user_baseinfo.dynamic_info.game_gold);
		if (rc != SrvOptSuccess)
		{
			rsp.clear_hall_info();
			rsp.set_result(rc);
			break;
		}
		
		rsp.set_result(SrvOptSuccess);
		if (req.has_is_apply()) rsp.set_is_apply(req.is_apply());
		if (req.has_explain_msg()) rsp.set_allocated_explain_msg(req.release_explain_msg());

	} while (false);
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_ENTER_CHESS_HALL_RSP;
    m_srvOpt.sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "user enter hall reply");
}
	
// 用户登录游戏通知
void CMsgHandler::userLoginNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserEnterGameNotify notify;
	if (!m_srvOpt.parseMsgFromBuffer(notify, data, len, "user enter game notify")) return;
	
	char sql[1024] = {0};
	unsigned int sqlLen = 0;
	if ((notify.service_id() / ServiceIdBaseValue) == GameHallSrv)
	{
		// 游戏玩家必须进入指定的棋牌室后才算玩家上线大厅
		CUserBaseinfo user_baseinfo;
	    if (getUserBaseinfo(notify.username(), notify.hall_id(), user_baseinfo) == SrvOptSuccess)
		{
			// 玩家在该棋牌室的信息刷新到memcached内存db
			++user_baseinfo.dynamic_info.sum_login_times;
			strncpy(user_baseinfo.dynamic_info.login_ip, notify.online_ip().c_str(), sizeof(user_baseinfo.dynamic_info.login_ip) - 1);
			if (!setUserBaseinfoToMemcached(user_baseinfo))
			{
				OptErrorLog("user enter hall service set base info error, hall id = %s, username = %s, service id = %u, ip = %s",
				notify.hall_id().c_str(), notify.username().c_str(), notify.service_id(), user_baseinfo.dynamic_info.login_ip);
			}
		}
		else
		{
			OptErrorLog("user enter hall service get base info error, hall id = %s, username = %s, service id = %u",
			notify.hall_id().c_str(), notify.username().c_str(), notify.service_id());
		}

	    // 登陆上线信息需要实时刷新到db，B端管理员会获取棋牌室里上线的玩家信息
		sqlLen = snprintf(sql, sizeof(sql) - 1,
		"update tb_chess_hall_user_info set online_hall_service_id=%u,online_hall_secs=%u,login_ip=\'%s\' where hall_id=\'%s\' and username=\'%s\';",
		notify.service_id(), (unsigned int)time(NULL), notify.online_ip().c_str(), notify.hall_id().c_str(), notify.username().c_str());
	}
    else
	{
		sqlLen = snprintf(sql, sizeof(sql) - 1,
		"update tb_chess_hall_user_info set online_game_service_id=%u,online_room_id='\%s\',online_seat_id=%d where hall_id=\'%s\' and username=\'%s\';",
		notify.service_id(), notify.room_id().c_str(), notify.seat_id(), notify.hall_id().c_str(), notify.username().c_str());
	}
	
	if (SrvOptSuccess == m_pLogicDBOpt->executeSQL(sql, sqlLen))
	{
		mysqlCommit();

		OptInfoLog("user enter service, username = %s, service id = %u, hall id = %s, room id = %s, seat id = %d",
		notify.username().c_str(), notify.service_id(), notify.hall_id().c_str(), notify.room_id().c_str(), notify.seat_id());
	}
	else
	{
		OptErrorLog("user enter service error, username = %s, sql = %s", notify.username().c_str(), sql);
	}
}

// 用户登出游戏通知
void CMsgHandler::userLogoutNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserLeaveGameNotify notify;
	if (!m_srvOpt.parseMsgFromBuffer(notify, data, len, "user leave game notify")) return;
	
	char sql[1024] = {0};
	unsigned int sqlLen = 0;
	if ((notify.service_id() / ServiceIdBaseValue) == GameHallSrv)
	{
		// 获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (getUserBaseinfo(notify.username(), notify.hall_id(), user_baseinfo) == SrvOptSuccess)
		{
			// 将数据同步到memcached
			user_baseinfo.dynamic_info.sum_online_secs += notify.online_time();
			if (setUserBaseinfoToMemcached(user_baseinfo)) updateOptStatus(notify.username(), EUpdateDBOpt::ELogoutOpt);
		}
        else
		{
			OptErrorLog("user leave hall service get base info error, hall id = %s, username = %s, service id = %u",
			notify.hall_id().c_str(), notify.username().c_str(), notify.service_id());
		}

        // 刷新大厅数据
		sqlLen = snprintf(sql, sizeof(sql) - 1,
		"update tb_chess_hall_user_info set offline_hall_secs=%u,online_hall_service_id=0 where hall_id=\'%s\' and username=\'%s\';",
		(unsigned int)time(NULL), notify.hall_id().c_str(), notify.username().c_str());
	}
	else
	{
		// 刷新服务房间数据
		sqlLen = snprintf(sql, sizeof(sql) - 1,
		"update tb_chess_hall_user_info set offline_game_secs=%u,online_game_service_id=0,online_seat_id=%d where hall_id=\'%s\' and username=\'%s\';",
		(unsigned int)time(NULL), notify.seat_id(), notify.hall_id().c_str(), notify.username().c_str());
	}

	if (SrvOptSuccess == m_pLogicDBOpt->executeSQL(sql, sqlLen))
	{
		mysqlCommit();

		OptInfoLog("user leave service, username = %s, service id = %u, hall id = %s, room id = %s, seat id = %d, online time = %u, code = %d",
		notify.username().c_str(), notify.service_id(), notify.hall_id().c_str(), notify.room_id().c_str(), notify.seat_id(), notify.online_time(), notify.code());
	}
	else
	{
		OptErrorLog("user leave service error, username = %s, sql = %s", notify.username().c_str(), sql);
	}
}

// 服务状态通知
void CMsgHandler::serviceStatusNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ServiceStatusNotify notify;
	if (!m_srvOpt.parseMsgFromBuffer(notify, data, len, "service status notify")) return;

	char sql[1024] = {0};
	unsigned int sqlLen = 0;
	if ((notify.service_id() / ServiceIdBaseValue) == GameHallSrv)
	{
        // 刷新大厅数据
		sqlLen = snprintf(sql, sizeof(sql) - 1,
		"update tb_chess_hall_user_info set offline_hall_secs=%u,online_hall_service_id=0 where online_hall_service_id=%u;",
		(unsigned int)time(NULL), notify.service_id());
	}
	else
	{
		// 刷新服务房间数据
		sqlLen = snprintf(sql, sizeof(sql) - 1,
		"update tb_chess_hall_user_info set offline_game_secs=%u,online_game_service_id=0 where online_game_service_id=%u;",
		(unsigned int)time(NULL), notify.service_id());
	}

    const int rc = m_pLogicDBOpt->executeSQL(sql, sqlLen);
	if (SrvOptSuccess == rc) mysqlCommit();
	else OptErrorLog("service status notify error, sql = %s", sql);

	OptWarnLog("service status notify id = %u, status = %d, type = %u, name = %s, rc = %d",
	notify.service_id(), notify.service_status(), notify.service_type(), notify.service_name().c_str(), rc);
}

