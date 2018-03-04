
/* author : limingfan
 * date : 2017.09.15
 * description : 消息处理类
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

#include "connect/CSocket.h"
#include "base/CMD5.h"
#include "base/Function.h"
#include "COptMsgHandler.h"


using namespace NConnect;


// md5加密结果最大长度字符串
static const unsigned int MD5PSWMaxLen = 128;
typedef char MD5ResultStr[MD5PSWMaxLen];

// 获取用户内部自动密码加密结果
static void getInnerAutoPsw(const string& username, const unsigned int timeSecs, MD5ResultStr pswResult)
{
	// 用户注册默认密码MD5加密结果
	// 秘钥明文 manager@0123456789mm
	// ！注意，秘钥明文不可更改，否则会导致之前使用该秘钥明文注册的账号校验失败！
	static const char* UserDefaultSecretKey = "23A246E5B4B0EBBC4FFDA8346FE5AA62";  // 秘钥明文 MD5(manager@0123456789mm) 结果

    MD5ResultStr mmStr = {0};
	const unsigned int mmLen = snprintf(mmStr, sizeof(MD5ResultStr) - 1, "%s%u%s", username.c_str(), timeSecs, UserDefaultSecretKey);
	NProject::md5Encrypt(mmStr, mmLen, pswResult, false);
}


// 数据记录日志
#define WriteDataLog(format, args...)     m_srvOpt.getDataLogger()->writeFile(NULL, 0, LogLevel::Info, format, ##args)


// 商务客户端版本标识
enum EBSAppVersionFlag
{
	ENormalVersion = 0,               // 正常的当前版本
	ECheckVersion = 1,                // 审核版本
};


// 版本号信息验证
int CBSAppVersionManager::getVersionInfo(const unsigned int deviceType, const unsigned int platformType, const string& curVersion, unsigned int& flag, string& newVersion, string& newFileURL)
{
	const NOptMgrConfig::config& cfg = NOptMgrConfig::config::getConfigValue();
	const map<int, NOptMgrConfig::client_version>* client_version_list = NULL;
	if (deviceType == com_protocol::EMobileOSType::EAndroid)
		client_version_list = &cfg.android_version_list;
	else if (deviceType == com_protocol::EMobileOSType::EAppleIOS)
		client_version_list = &cfg.apple_version_list;
	else
		return BSVersionMobileOSTypeError;
	
	map<int, NOptMgrConfig::client_version>::const_iterator verIt = client_version_list->find(platformType);
	if (verIt == client_version_list->end()) return BSVersionPlatformTypeError;
	
	// flag 版本号标识结果
	//	enum ECheckVersionResult
	//	{
	//		EValidVersion = 1;        // 可用版本
	//		EInValidVersion = 0;      // 无效的不可用版本，必须下载最新版本才能进入游戏
	//  }

	const NOptMgrConfig::client_version& cVer = verIt->second;
	newVersion = cVer.version;
	newFileURL = cVer.url;
	flag = ((curVersion < cVer.valid) || (curVersion > newVersion)) ? 0 : 1;        // 正式版本号
	if (flag == 0)
	{
		flag = (cVer.check_flag != 1 || curVersion != cVer.check_version) ? 0 : 1;  // 验证是否是审核版本号
		if (flag == 1) newVersion = cVer.check_version;                             // 最新版本号修改为审核版本号
	}

	OptInfoLog("business app version data, ip = %s, port = %d, device type = %d, platform type = %d, current version = %s, valid version = %s, flag = %d, new version = %s, file url = %s, check flag = %d, version = %s",
	CSocket::toIPStr(m_msgHandler->getConnectProxyContext().conn->peerIp), m_msgHandler->getConnectProxyContext().conn->peerPort,
	deviceType, platformType, curVersion.c_str(), cVer.valid.c_str(), flag, newVersion.c_str(), newFileURL.c_str(), cVer.check_flag, cVer.check_version.c_str());
	
	return SrvOptSuccess;
}


COptMsgHandler::COptMsgHandler() : m_srvOpt(sizeof(OptMgrUserData))
{
	m_pDBOpt = NULL;
	m_msgRecordToDbTimer = 0;
	m_serviceValidIntervalSecs = 0;
	m_pCfg = NULL;
}

COptMsgHandler::~COptMsgHandler()
{
	if (m_pDBOpt)
	{
		CMySql::destroyDBOpt(m_pDBOpt);
		m_pDBOpt = NULL;
	}
	
	m_msgRecordToDbTimer = 0;
	m_serviceValidIntervalSecs = 0;
	m_pCfg = NULL;
}

CHttpMessageHandler& COptMsgHandler::getHttpMsgHandler()
{
	return m_httpMsgHandler;
}

CServiceOperationEx& COptMsgHandler::getSrvOpt()
{
	return m_srvOpt;
}

CDBOpertaion* COptMsgHandler::getOptDBOpt()
{
	return m_pDBOpt;
}


void COptMsgHandler::loadConfig(bool isReset)
{
	if (isReset) m_srvOpt.updateCommonConfig(CCfg::getValue("ServiceConfig", "ConfigFile"));

	m_pCfg = &NOptMgrConfig::config::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str(), isReset);
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
	
	if (!m_srvOpt.getMallCfg(isReset).isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update mall xml config value error");
		
		stopService();
		return;
	}

	m_ip2city.init(m_srvOpt.getCommonCfg().data_file.city_id_mapping.c_str(), m_srvOpt.getCommonCfg().data_file.ip_city_mapping.c_str());

	if (0 != m_msgRecordToDbTimer)
	{
		killTimer(m_msgRecordToDbTimer);
	    m_msgRecordToDbTimer = setTimer(MillisecondUnit * m_pCfg->common_cfg.chat_record_to_db_interval, (TimerHandler)&COptMsgHandler::msgRecordToDB, 0, NULL, -1);
	}

	m_logicHandler.updateConfig();

	m_srvOpt.getDBCfg().output();
	m_srvOpt.getServiceCommonCfg().output();
	m_srvOpt.getMallCfg().output();
	m_pCfg->output();
	
	ReleaseInfoLog("update config finish");
}

void COptMsgHandler::onCloseConnectProxy(void* userData, int cbFlag)
{
	OptMgrUserData* cud = (OptMgrUserData*)userData;
	removeConnectProxy(string(cud->userName), 0, false);  // 存在连接被动关闭的情况，因此需要清理连接信息

	logoutNotify(cud, cbFlag);
}

void COptMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!m_srvOpt.initEx(this, CCfg::getValue("ServiceConfig", "ConfigFile")))
	{
		OptErrorLog("init service operation instance error");

        stopService();
		return;
	}
	
	m_srvOpt.cleanUpConnectProxy(m_srvMsgCommCfg);  // 清理连接代理，如果服务异常退出，则启动时需要先清理连接代理
	
	m_srvOpt.setCloseRepeateNotifyProtocol(0, BSC_CLOSE_REPEATE_CONNECT_NOTIFY);
	
	// 创建数据日志
	if (!m_srvOpt.createDataLogger("DataLogger", "File", "Size", "BackupCount"))
	{
		OptErrorLog("create data log error");
		stopService();

		return;
	}
	
	loadConfig(false);
	
	m_httpMsgHandler.init(this);

	// 游戏运营管理数据库
	const DBConfig::config& dbCfg = m_srvOpt.getDBCfg();
	if (0 != CMySql::createDBOpt(m_pDBOpt, dbCfg.opt_db_cfg.ip.c_str(),
								 dbCfg.opt_db_cfg.username.c_str(),
								 dbCfg.opt_db_cfg.password.c_str(),
								 dbCfg.opt_db_cfg.dbname.c_str(),
								 dbCfg.opt_db_cfg.port))
	{
		OptErrorLog("operation service, mysql create operation db failed. ip:%s username:%s passwd:%s dbname:%s port:%d",
					dbCfg.opt_db_cfg.ip.c_str(),
					dbCfg.opt_db_cfg.username.c_str(), dbCfg.opt_db_cfg.password.c_str(),
					dbCfg.opt_db_cfg.dbname.c_str(), dbCfg.opt_db_cfg.port);
		stopService();
		
		return;
	}

	// 可能存在服务异常退出的情况，则清理在线用户数据（棋牌室管理员在线数据）
	char tmpSql[256] = {0};
	const unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	                                        "update tb_online_user_data set status=%u,offline_time=now() where status=%u and server_id=%u;",
			                                com_protocol::EUserQuitStatus::EServiceStart, com_protocol::EUserQuitStatus::EUserOnline, getSrvId());
	int rc = m_pDBOpt->executeSQL(tmpSql, tmpSqlLen);
	if (rc != SrvOptSuccess)
	{
		OptErrorLog("update online manager user data error, rc = %d, sql = %s", rc, tmpSql);
		stopService();
		
		return;
	}
	
	if (!initChessHallIdRandomNumber())
	{
		OptErrorLog("init chess hall id random number error");
		stopService();
		
		return;
	}

	if (!m_logicHandler.init(this))
	{
		OptErrorLog("init logic handler error");
		stopService();
		
		return;
	}

    m_msgRecordToDbTimer = setTimer(MillisecondUnit * m_pCfg->common_cfg.chat_record_to_db_interval, (TimerHandler)&COptMsgHandler::msgRecordToDB, 0, NULL, -1);

    // 中心内存redis数据库
	if (!m_redisDbOpt.connectSvr(dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port,
	    dbCfg.redis_db_cfg.center_db_timeout * MillisecondUnit))
	{
		OptErrorLog("operation manager connect center redis service failed, ip = %s, port = %u, time out = %u",
		dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port, dbCfg.redis_db_cfg.center_db_timeout);
		
		stopService();
		return;
	}

	// 定时向redis获取服务数据，单位秒
	unsigned int getServiceDataIntervalSecs = 60 * 10;
	const char* secsTimeInterval = CCfg::getValue("ServiceConfig", "GetServiceDataInterval");
	if (secsTimeInterval != NULL) getServiceDataIntervalSecs = atoi(secsTimeInterval);
	setTimer(getServiceDataIntervalSecs * MillisecondUnit, (TimerHandler)&COptMsgHandler::updateServiceData, 0, NULL, (unsigned int)-1);
	
	// 游戏服务检测的有效时间间隔，单位秒
	secsTimeInterval = CCfg::getValue("ServiceConfig", "GameServiceValidInterval");
	if (secsTimeInterval != NULL) m_serviceValidIntervalSecs = atoi(secsTimeInterval);
	else m_serviceValidIntervalSecs = getServiceDataIntervalSecs * 2;
	
	// 启动时获取下服务数据信息
	updateServiceData(0, 0, NULL, 0);
    
    // 启动定时器执行定时任务
	onTaskTimer(0, 0, NULL, 0);

	ReleaseInfoLog("--- onLoad ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}

void COptMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onUnLoad ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
	
	m_logicHandler.unInit();

	// 服务关闭，所有在线用户退出登陆
	const IDToConnectProxys& userConnects = getConnectProxy();
	for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end(); ++it)
	{
		// 这里不可以调用removeConnectProxy，如此会不断修改userConnects的值而导致本循环遍历it值错误
		logoutNotify(getProxyUserData(it->second), com_protocol::EUserQuitStatus::EServiceStop);
	}

	stopConnectProxy();  // 停止连接代理

	m_redisDbOpt.disconnectSvr();

	ReleaseInfoLog("--- onUnLoad --- all success.");
}

void COptMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 注册协议处理函数
	
	registerProtocol(ServiceType::OutsideClientSrv, BSC_MANAGER_REGISTER_REQ, (ProtocolHandler)&COptMsgHandler::registerManager);
	registerProtocol(ServiceType::OutsideClientSrv, BSC_MANAGER_LOGIN_REQ, (ProtocolHandler)&COptMsgHandler::managerLogin);
	registerProtocol(ServiceType::HttpOperationSrv, HTTPOPT_WX_LOGIN_RSP, (ProtocolHandler)&COptMsgHandler::managerWxLoginReply);
	registerProtocol(ServiceType::OutsideClientSrv, BSC_MANAGER_LOGOUT_NOTIFY, (ProtocolHandler)&COptMsgHandler::managerLogout);
	
	registerProtocol(ServiceType::CommonSrv, OPTMGR_SERVICE_STATUS_NOTIFY, (ProtocolHandler)&COptMsgHandler::serviceStatusNotify);
	
	registerProtocol(ServiceType::CommonSrv, OPTMGR_USER_ONLINE_NOTIFY, (ProtocolHandler)&COptMsgHandler::userEnterServiceNotify);
	registerProtocol(ServiceType::CommonSrv, OPTMGR_USER_OFFLINE_NOTIFY, (ProtocolHandler)&COptMsgHandler::userLeaveServiceNotify);
	
	registerProtocol(ServiceType::CommonSrv, OPTMGR_MSG_RECORD_NOTIFY, (ProtocolHandler)&COptMsgHandler::msgRecordNotify);
	
	// 商务客户端APP版本管理
	// m_bsAppVerManager.init(this, BSC_VERSION_CHECK_REQ, BSC_VERSION_CHECK_RSP);
	
	ReleaseInfoLog("--- onRegister ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}

void COptMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onRun ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}

bool COptMsgHandler::onPreHandleMessage(const char* msgData, const unsigned int msgLen, const ConnectProxyContext& connProxyContext)
{
	if (connProxyContext.protocolId == BSC_MANAGER_REGISTER_REQ || connProxyContext.protocolId == BSC_MANAGER_LOGIN_REQ)
	{
		return !m_srvOpt.isRepeateCheckConnectProxy(BSC_FORCE_USER_QUIT_NOTIFY);  // 检查是否存在重复登陆
	}

	// 检查发送消息的连接是否已成功通过验证，防止玩家验证成功之前胡乱发消息
    if (m_srvOpt.checkConnectProxyIsSuccess(BSC_FORCE_USER_QUIT_NOTIFY)) return true;
	
	OptErrorLog("operation manager service receive not check success message, protocolId = %d", connProxyContext.protocolId);
	
	return false;
}


bool COptMsgHandler::initChessHallIdRandomNumber()
{
	// 查找目前所有的棋牌室ID
	const char* findHallIdSql = "select hall_id from tb_chess_card_hall;";
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess != m_pDBOpt->queryTableAllResult(findHallIdSql, pResult))
	{
		OptErrorLog("find all chess hall id error, sql = %s", findHallIdSql);
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
	m_pDBOpt->releaseQueryResult(pResult);
	
	const unsigned int minChessHallId = 100001;
	const unsigned int maxChessHallId = 999999;
	m_chessHallIdRandomNumbers.initNoRepeatUIntNumber(minChessHallId, maxChessHallId, &needFilterNumbers);
	
	return true;
}

// 检查客户端版本号是否可用
int COptMsgHandler::checkClientVersion(unsigned int deviceType, unsigned int platformType, const string& curVersion, unsigned char& versionFlag)
{
	const map<int, NOptMgrConfig::client_version>* client_version_list = NULL;
	if (deviceType == com_protocol::EMobileOSType::EAndroid)
		client_version_list = &m_pCfg->android_version_list;
	else if (deviceType == com_protocol::EMobileOSType::EAppleIOS)
		client_version_list = &m_pCfg->apple_version_list;
	else
	{
		return BSVersionMobileOSTypeError;
	}

	map<int, NOptMgrConfig::client_version>::const_iterator verIt = client_version_list->find(platformType);
	if (verIt == client_version_list->end())
	{
		return BSVersionPlatformTypeError;
	}
	
	const NOptMgrConfig::client_version& cVer = verIt->second;
	if ((curVersion >= cVer.valid && curVersion <= cVer.version)       // 正式版本号
		|| (cVer.check_flag == 1 && curVersion == cVer.check_version))    // 校验测试版本号
	{
		versionFlag = (curVersion > cVer.version) ? EBSAppVersionFlag::ECheckVersion : EBSAppVersionFlag::ENormalVersion;
	}
	else
	{
		return BSClientVersionInvalid;
	}
	
	return SrvOptSuccess;
}

// 登录成功
void COptMsgHandler::loginSuccess(unsigned char platformType, ConnectProxy* conn, OptMgrUserData* cud, const com_protocol::BSLoginInfo& info)
{
	// 成功则替换连接代理关联信息，变更连接ID为用户名
	m_srvOpt.checkConnectProxySuccess(info.name(), conn);

	cud->platformType = platformType;
	cud->level = info.level();
	strncpy(cud->nickname, info.nickname().c_str(), IdMaxLen - 1);
	// strncpy(cud->portraitId, info.portrait_id().c_str(), PortraitIdMaxLen - 1);
	
	if (info.chess_hall_size() > 0) strncpy(cud->chessHallId, info.chess_hall(0).id().c_str(), IdMaxLen - 1);
}
	
// 通知连接logout操作
void COptMsgHandler::logoutNotify(void* cb, int flag)
{
	unsigned int onlineTimeSecs = 0;
	OptMgrUserData* cud = (OptMgrUserData*)cb;
	if (m_srvOpt.isCheckSuccessConnectProxy(cud))
	{
		m_logicHandler.offline(cud);

		onlineTimeSecs = time(NULL) - cud->timeSecs;

		com_protocol::UserLeaveGameNotify leaveNf;
		leaveNf.set_username(cud->userName);
		leaveNf.set_service_id(getSrvId());
		leaveNf.set_code(flag);
	    recordUserLeaveService(leaveNf, cud->level);
	}
	
	OptInfoLog("business user logout, name = %s, status = %d, login times = %u, online time = %u, flag = %d",
	cud->userName, cud->status, cud->repeatTimes, onlineTimeSecs, flag);
	
	m_srvOpt.destroyConnectProxyUserData(cud);
}

// 执行任务
void COptMsgHandler::doTask()
{
    unsigned int deleteTimeSecs = time(NULL) - (m_pCfg->timing_task.save_user_login_info_day * OneDaySeconds);
    char sql[256] = {0};
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1,
	                                     "delete from tb_online_user_data where UNIX_TIMESTAMP(offline_time) < %u;", deleteTimeSecs);

	int rc = m_pDBOpt->executeSQL(sql, sqlLen);
	
    OptInfoLog("delete user login info, config day = %u, timeSecs = %u, count = %u, rc = %d",
    m_pCfg->timing_task.save_user_login_info_day, deleteTimeSecs, m_pDBOpt->getAffectedRows(), rc);
}

int COptMsgHandler::newRegisterManager(const char* peerIp, com_protocol::RegisterManagerReq& req)
{
	// 先查询该平台type&id对应的用户ID是否已经存在是否已经被注册过了
	int rc = checkRegisterPlatformInfo(req.platform_type(), req.platform_id());
	if (rc != SrvOptSuccess) return rc;

	// 找出当前最大用户ID
	CQueryResult* pResult = NULL;
	char sqlTmp[256] = {0};
	const unsigned int sqlLen = snprintf(sqlTmp, sizeof(sqlTmp) - 1, "select max(user_id) from tb_chess_hall_manager_static_info;");
	if (SrvOptSuccess != m_pDBOpt->queryTableAllResult(sqlTmp, sqlLen, pResult)
		|| pResult == NULL || 1 != pResult->getRowCount())
	{
		m_pDBOpt->releaseQueryResult(pResult);

		return BSGetUserMaxIdFailed;
	}

	RowDataPtr rowData = pResult->getNextRow();
	unsigned int curMaxUserId = (rowData[0] != NULL) ? atoi(rowData[0]) : 0;
	m_pDBOpt->releaseQueryResult(pResult);

    // 生成内部账号ID
	++curMaxUserId;
	snprintf(sqlTmp, sizeof(sqlTmp) - 1, "3%04u", curMaxUserId);
	req.set_name(sqlTmp);
	
	// 密码处理
	const unsigned int curSecs = time(NULL);
	if (req.password().length() < 1)
	{
		// 用户没有输入密码则默认生成内部自动密码
		MD5ResultStr pswResult = {0};
		getInnerAutoPsw(req.name(), curSecs, pswResult);
		req.set_password(pswResult);
	}

	// 生成注册用户sql语句
	string registerSql;
	rc = getRegisterSql(req.name().c_str(), curSecs, curMaxUserId, peerIp, com_protocol::EManagerLevel::ESuperLevel, req, registerSql);
	if (rc != SrvOptSuccess) return rc;
	
	// 插入表数据
	if (SrvOptSuccess != m_pDBOpt->executeSQL(registerSql.c_str(), registerSql.length()))
	{
		OptErrorLog("register manager error, sql = %s", registerSql.c_str());

		return BSRegisterManagerError;
	}
	
	OptInfoLog("register manager success, platform id = %s, type = %u, manager name = %s, nick = %s, gender = %d, portrait = %s",
	req.platform_id().c_str(), req.platform_type(), req.name().c_str(), req.nickname().c_str(), req.gender(), req.portrait_id().c_str());

	return SrvOptSuccess;
}

int COptMsgHandler::checkRegisterPlatformInfo(unsigned int platformType, const string& platformId)
{
	// 查询该平台type&id对应的用户ID是否已经存在是否已经被注册过了
	CQueryResult* pResult = NULL;
	char sqlTmp[256] = {0};
	unsigned int sqlLen = snprintf(sqlTmp, sizeof(sqlTmp) - 1,
	"select 1 from tb_chess_hall_manager_static_info where platform_type=%u and platform_id=\'%s\' limit 1;",
	platformType, platformId.c_str());
	if (SrvOptSuccess != m_pDBOpt->queryTableAllResult(sqlTmp, sqlLen, pResult))
	{
		OptErrorLog("get register manager platform info error, sql = %s", sqlTmp);

		return BSQueryPlatformInfoError;
	}
	
	if (pResult != NULL)
	{
		const bool hasSameName = (pResult->getRowCount() > 0);

		m_pDBOpt->releaseQueryResult(pResult);

		if (hasSameName)
		{
		    OptErrorLog("register already exist platform type id, platform type = %u, id = %s", platformType, platformId.c_str());

			return BSExistPlatformInfoError;
		}
	}
	
	return SrvOptSuccess;
}

int COptMsgHandler::getRegisterSql(const char* creator, unsigned int registerTimeSecs, unsigned int userId, const char* ip, unsigned int level,
                                   const com_protocol::RegisterManagerReq& req, string& sql)
{
	// 输入合法性判定
	if (req.name().length() >= 32 || req.name().length() < 3 || !strIsAlnum(req.name().c_str()))
	{
		return BSUsernameInputUnlegal;
	}

	if (req.nickname().length() >= 32)
	{
		return BSNicknameInputUnlegal;
	}
	
	if (req.portrait_id().length() >= 512)
	{
		return BSPortraitIdInputUnlegal;
	}

	if (req.password().length() != 32 || !strIsUpperHex(req.password().c_str()))
	{
		return BSPasswdInputUnlegal;
	}
	
	// password = md5(name + md5(password))
	CMD5 md5;
	string password = req.name() + req.password();
	md5.GenerateMD5((const unsigned char*)password.c_str(), password.length());
	password = md5.ToString();
	
	const string& address = req.has_address() ? req.address() : "";
	if (address.length() >= 64)
	{
		return BSAddrInputUnlegal;
	}
	
	int32_t gender = req.has_gender() ? req.gender() : 1;
	
	const string& mobile_phone = req.has_mobile_phone() ? req.mobile_phone() : "";
	if (mobile_phone.length() >= 16 || !strIsAlnum(mobile_phone.c_str()))
	{
		return BSMobilephoneInputUnlegal;
	}
	
	const string& email = req.has_email() ? req.email() : "";
	if (email.length() >= 36)
	{
		return BSEmailInputUnlegal;
	}
	
	string city_name;
	uint32_t city_id = 0;
	m_ip2city.getCityInfoByIP(ip, city_name, city_id);

	char str_tmp[10240] = {0};
	snprintf(str_tmp, sizeof(str_tmp) - 1,
	         "insert into tb_chess_hall_manager_static_info(register_time,user_id,creator,platform_id,platform_type,name,nickname,password,portrait_id, \
             gender,email,mobile_phone,address,city_id,remarks) value(%u,%u,\'%s\',\'%s\',%u,\'%s\',\'%s\',\'%s\',\'%s\',\
			 %u,\'%s\',\'%s\',\'%s\',%u, 'regiter');",
			 registerTimeSecs, userId, creator, req.platform_id().c_str(), req.platform_type(), req.name().c_str(), req.nickname().c_str(), password.c_str(), req.portrait_id().c_str(),
			 gender, email.c_str(), mobile_phone.c_str(), address.c_str(), city_id);
			 
	sql = str_tmp;

	return SrvOptSuccess;
}

int COptMsgHandler::login(ConnectProxy* conn, OptMgrUserData* curCud, unsigned int platformType, const string& platformId,
                          string& name, string& password, com_protocol::BSLoginInfo& info, bool& needReleaseChessHallInfo)
{
	needReleaseChessHallInfo = false;
	int rc = SrvOptSuccess;
	CQueryResult* pResult = NULL;

	do
	{
		if (name.empty() && platformId.empty())
		{
			rc = BSInvalidParameter;
			break;
		}

		// 直接查找用户数据
		char sqlBuffer[512] = {0};
		unsigned int sqlLen = 0;
		if (name.length() > 0)
		{
			// 优先名字ID查找
			sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
			"select S.name,S.register_time,S.password,S.status,S.nickname,S.portrait_id,S.gender,D.hall_id,D.level,D.status from tb_chess_hall_manager_static_info as S "
			"left join tb_chess_hall_manager_dynamic_info as D on S.name=D.name where S.name=\'%s\';", name.c_str());
		}
		else
		{
			// 名字ID没有则平台信息查找
			sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1,
			"select S.name,S.register_time,S.password,S.status,S.nickname,S.portrait_id,S.gender,D.hall_id,D.level,D.status from tb_chess_hall_manager_static_info as S "
			"left join tb_chess_hall_manager_dynamic_info as D on S.name=D.name where S.platform_type=%u and S.platform_id=\'%s\';",
			platformType, platformId.c_str());
		}
	
		if (SrvOptSuccess != m_pDBOpt->queryTableAllResult(sqlBuffer, sqlLen, pResult) || pResult == NULL)
		{
			OptErrorLog("query manager info error, result = %p, sql = %s", pResult, sqlBuffer);
			
			rc = BSQueryManagerInfoError;
			break;
		}
		
		if (pResult->getRowCount() < 1)
		{
			OptErrorLog("query manager info failed, row count = %u, sql = %s", pResult->getRowCount(), sqlBuffer);
			
			rc = BSNonExistManagerNameError;
			break;
		}
		
		// 用户名&密码处理
		// 输入用户名就必须同时输入密码，否则非法处理
		RowDataPtr rowData = pResult->getNextRow();
		if (name.length() < 1)
		{
			name = rowData[0];
			
			if (password.length() < 1)
			{
				// 用户没有输入密码则默认生成内部自动密码
				MD5ResultStr pswResult = {0};
				getInnerAutoPsw(name, atoi(rowData[1]), pswResult);
				password = pswResult;
			}
		}

		// 密码校验
		CMD5 md5;
		sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "%s%s", name.c_str(), password.c_str());
		md5.GenerateMD5((const unsigned char*)sqlBuffer, sqlLen);

		if (md5.ToString() != rowData[2])
		{
			rc = BSPasswordVerifyFailed;  // 密码校验失败
			break;
        }

		// 账号状态检查
		if (atoi(rowData[3]) != com_protocol::EManagerStatus::EMgrNormal)
		{
			rc = BSUserLimitLogin;  // 账号限制登录
			break;
		}

		// 存在重复的登录连接就直接关闭
		m_srvOpt.closeRepeatConnectProxy(name.c_str(), name.length(), getSrvId(), 0, 0);

		// 管理员所在的棋牌室信息
		info.set_level(com_protocol::EManagerLevel::ESuperLevel);

		// select S.name,S.register_time,S.password,S.status,S.nickname,S.portrait_id,S.gender,D.hall_id,D.level,D.status
		if (rowData[7] != NULL && rowData[7][0] != '\0')
		{
			SChessHallData* chData = getChessHallData(rowData[7], rc);
			if (chData == NULL) break;

			needReleaseChessHallInfo = true;
			info.mutable_chess_hall()->AddAllocated(&chData->baseInfo);

			info.set_level((com_protocol::EManagerLevel)atoi(rowData[8]));
		}
		
		info.set_name(name);
		info.set_nickname(rowData[4]);
		info.set_portrait_id(rowData[5]);
		info.set_gender(atoi(rowData[6]));

		// 登录成功了
		loginSuccess(platformType, conn, curCud, info);
		
		// 记录登录信息
		com_protocol::UserEnterGameNotify managerLoginNf;
		managerLoginNf.set_username(name);
		managerLoginNf.set_service_id(getSrvId());
		managerLoginNf.set_online_ip(CSocket::toIPStr(conn->peerIp));
		recordUserEnterService(managerLoginNf, info.level());
	
	} while (false);
	
	if (pResult != NULL) m_pDBOpt->releaseQueryResult(pResult);
	
	OptInfoLog("manager login, result = %d, platform id = %s, type = %u, manager name = %s nick = %s gender = %d, portrait = %s",
	rc, platformId.c_str(), platformType, name.c_str(), info.nickname().c_str(), info.gender(), info.portrait_id().c_str());

	return rc;
}

// 获取棋牌室不重复的随机ID
unsigned int COptMsgHandler::getChessHallNoRepeatRandID()
{
	return m_chessHallIdRandomNumbers.getNoRepeatUIntNumber();
}

// 获取棋牌室数据
SChessHallData* COptMsgHandler::getChessHallData(const char* hall_id, int& errorCode, bool isNeedUpdate)
{
	if (hall_id == NULL || *hall_id == '\0')
	{
		errorCode = BSChessHallIdParamError;
		return NULL;
	}
	
	SChessHallData* chessHallData = NULL;
	ChessHallIdToDataMap::iterator hallDataIt = m_hallId2Data.find(hall_id);
	if (hallDataIt != m_hallId2Data.end())
	{
		chessHallData = &hallDataIt->second;
	}
	else
	{
		m_hallId2Data[hall_id] = SChessHallData();
		chessHallData = &m_hallId2Data[hall_id];
	}
	
	chessHallData->offilnePlayers = getPlayerCount(hall_id, com_protocol::EHallPlayerStatus::ECheckAgreedPlayer);
	chessHallData->forbidPlayers = getPlayerCount(hall_id, com_protocol::EHallPlayerStatus::EForbidPlayer);
	chessHallData->applyPlayers = getPlayerCount(hall_id, com_protocol::EHallPlayerStatus::EWaitForCheck);
	
	// 当前在线人数
	chessHallData->baseInfo.set_current_onlines(getPlayerCount(hall_id, com_protocol::EHallPlayerStatus::EOnlinePlayer));
	
	// 今日同时最高在线人数
	if (chessHallData->baseInfo.current_onlines() > chessHallData->baseInfo.max_onlines())
	{
		chessHallData->baseInfo.set_max_onlines(chessHallData->baseInfo.current_onlines());
	}

	// 各个游戏类型在线人数
	for (GameTypeToInfoMap::iterator gameInfoIt = chessHallData->gameInfo.begin(); gameInfoIt != chessHallData->gameInfo.end(); ++gameInfoIt)
	{
		gameInfoIt->second.set_online_players(getPlayerCount(hall_id, com_protocol::EHallPlayerStatus::EOnlinePlayer, gameInfoIt->first));
	}
	
	const int currentDay = getCurrentYearDay();
	if (isNeedUpdate || chessHallData->yearDay != currentDay || chessHallData->needUpdateChessHallInfo)
	{
		chessHallData->baseInfo.Clear();
		errorCode = getChessHallInfo(hall_id, chessHallData->baseInfo);
		if (errorCode != SrvOptSuccess)
		{
			m_hallId2Data.erase(hall_id);

			return NULL;
		}

        getHallGameInfo(hall_id, chessHallData->gameInfo);  // 获取棋牌室游戏信息
		
		chessHallData->yearDay = currentDay;
		chessHallData->needUpdatePlayer = false;
		chessHallData->needUpdateChessHallInfo = false;
	}
	
	else if (chessHallData->needUpdatePlayer)
	{
		/*
		// 当前在线人数
		chessHallData->baseInfo.set_current_onlines(getPlayerCount(hall_id, com_protocol::EHallPlayerStatus::EOnlinePlayer));
		
		// 今日同时最高在线人数
		if (!chessHallData->baseInfo.has_max_onlines()
		    || chessHallData->baseInfo.current_onlines() > chessHallData->baseInfo.max_onlines())
		{
			chessHallData->baseInfo.set_max_onlines(chessHallData->baseInfo.current_onlines());
		}
		
		// 当前离线人数
	    chessHallData->offilnePlayers = getPlayerCount(hall_id, com_protocol::EHallPlayerStatus::ECheckAgreedPlayer);
		
		// 各个游戏类型在线人数
		for (GameTypeToInfoMap::iterator gameInfoIt = chessHallData->gameInfo.begin(); gameInfoIt != chessHallData->gameInfo.end(); ++gameInfoIt)
		{
			gameInfoIt->second.set_online_players(getPlayerCount(hall_id, com_protocol::EHallPlayerStatus::EOnlinePlayer, gameInfoIt->first));
		}

		chessHallData->needUpdatePlayer = false;
		*/ 
	}
	
	errorCode = SrvOptSuccess;
	return chessHallData;
}

// 获取棋牌室游戏信息
bool COptMsgHandler::getHallGameInfo(const char* hall_id, GameTypeToInfoMap& hallGameInfo)
{
	// 当前棋牌室里的游戏
	// select create_time,game_type,game_status,service_tax_ratio from tb_hall_game_info where hall_id='xx';
	char tmpSql[256] = {0};
	const unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"select UNIX_TIMESTAMP(create_time),game_type,game_status,service_tax_ratio from tb_hall_game_info where hall_id=\'%s\';", hall_id);

    hallGameInfo.clear();
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess != m_pDBOpt->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
	{
		OptErrorLog("query chess hall game error, sql = %s", tmpSql);
		return false;
	}
	
	if (pResult != NULL && pResult->getRowCount() > 0)
	{
		const map<unsigned int, ServiceCommonConfig::GameInfo>& gameCfgInfo = m_srvOpt.getServiceCommonCfg().game_info_map;
		const time_t todaySecs = time(NULL);
		RowDataPtr rowData = NULL;
		while ((rowData = pResult->getNextRow()) != NULL)
		{
			const unsigned int gameType = atoi(rowData[1]);
			hallGameInfo[gameType] = com_protocol::BSHallGameInfo();
			
			com_protocol::BSHallGameInfo& gameInfo = hallGameInfo[gameType];
			gameInfo.set_type(gameType);
			gameInfo.set_add_time(atoi(rowData[0]));
			gameInfo.set_status((com_protocol::EHallGameStatus)atoi(rowData[2]));
			gameInfo.set_service_tax_ratio(atof(rowData[3]));

			map<unsigned int, ServiceCommonConfig::GameInfo>::const_iterator gIfIt = gameCfgInfo.find(gameType);
			if (gIfIt != gameCfgInfo.end())
			{
			    gameInfo.set_name(gIfIt->second.name);
			    gameInfo.set_desc(gIfIt->second.desc);
				gameInfo.set_icon(gIfIt->second.icon);
			}
			
			gameInfo.set_total_revenue(getTimeOptSumValue(hall_id, com_protocol::EHallPlayerOpt::EOptTaxCost, 0, gameType));
			gameInfo.set_today_revenue(getTimeOptSumValue(hall_id, com_protocol::EHallPlayerOpt::EOptTaxCost, todaySecs, gameType));
			gameInfo.set_online_players(getPlayerCount(hall_id, com_protocol::EHallPlayerStatus::EOnlinePlayer, gameType));
		}
	}
	
	m_pDBOpt->releaseQueryResult(pResult);

	return true;
}

// 从DB中查找用户信息
int COptMsgHandler::queryPlayerInfo(const char* sql, const unsigned int sqlLen, StringKeyValueMap& keyValue)
{
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess != getOptDBOpt()->queryTableAllResult(sql, sqlLen, pResult))
	{
		OptErrorLog("query player info error, sql = %s", sql);

		return BSQueryPlayerInfoError;
	}
	
	if (pResult == NULL || pResult->getRowCount() != 1)
	{
		OptErrorLog("query player info failed, sql = %s, row = %u", sql, (pResult != NULL) ? pResult->getRowCount() : 0);
		getOptDBOpt()->releaseQueryResult(pResult);
		
		return BSDBNoFoundPlayerInfoError;
	}

    // sql语句查找、遍历的顺序要一致，由 keyValue 值保证
	unsigned int index = 0;
	RowDataPtr rowData = pResult->getNextRow();
	for (StringKeyValueMap::iterator kvIt = keyValue.begin(); kvIt != keyValue.end(); ++kvIt)
	{
		if (rowData[index] != NULL) kvIt->second = rowData[index];
		
		++index;
	}
	
	getOptDBOpt()->releaseQueryResult(pResult);
	
	return SrvOptSuccess;
}

// 查找棋牌室用户信息
int COptMsgHandler::getChessHallPlayerInfo(const char* hallId, const char* username, StringKeyValueMap& keyValue)
{
	if (hallId == NULL || *hallId == '\0'
	    || username == NULL || *username == '\0' || keyValue.size() < 1) return BSInvalidParameter;

	// select xx,yy,zz from tb_chess_hall_user_info where hall_id='xx' and username='yy';
	char tmpSql[1024] = {0};
	StringKeyValueMap::const_iterator kvIt = keyValue.begin();
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1, "select %s", kvIt->first.c_str());

	while (++kvIt != keyValue.end())
	{
		tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 1, ",%s", kvIt->first.c_str());
	}

	tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 1,
	" from %s.tb_chess_hall_user_info where hall_id=\'%s\' and username=\'%s\';",
	m_srvOpt.getDBCfg().logic_db_cfg.dbname.c_str(), hallId, username);

	// 从DB中查找用户信息
    return queryPlayerInfo(tmpSql, tmpSqlLen, keyValue);
}

// 从逻辑DB查找用户静态信息
int COptMsgHandler::getPlayerStaticInfo(const char* username, StringKeyValueMap& keyValue)
{
	if (username == NULL || *username == '\0' || keyValue.size() < 1) return BSInvalidParameter;

	// select xx,yy,zz from tb_user_static_baseinfo where username=xx;
	char tmpSql[1024] = {0};
	StringKeyValueMap::const_iterator kvIt = keyValue.begin();
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1, "select %s", kvIt->first.c_str());

	while (++kvIt != keyValue.end())
	{
		tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 1, ",%s", kvIt->first.c_str());
	}

	tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 1,
	" from %s.tb_user_static_baseinfo where username=\'%s\';",
	m_srvOpt.getDBCfg().logic_db_cfg.dbname.c_str(), username);

	// 从DB中查找用户信息
    return queryPlayerInfo(tmpSql, tmpSqlLen, keyValue);
}

bool COptMsgHandler::getOnlineServiceId(const char* hallId, const char* username, unsigned int& hallSrvId, unsigned int& gameSrvId)
{
	hallSrvId = 0;
	gameSrvId = 0;
	StringKeyValueMap keyValue;
	keyValue["online_hall_service_id"] = "";
	keyValue["online_game_service_id"] = "";
	if (getChessHallPlayerInfo(hallId, username, keyValue) == SrvOptSuccess)
	{
		hallSrvId = atoi(keyValue["online_hall_service_id"].c_str());
		gameSrvId = atoi(keyValue["online_game_service_id"].c_str());
	}
	
	return (hallSrvId > ServiceIdBaseValue || gameSrvId > ServiceIdBaseValue);
}

bool COptMsgHandler::dbRollback()
{
	bool ret = m_pDBOpt->rollback();
	if (ret)
	{
		OptInfoLog("db rollback success");
	}
	else
	{
		OptErrorLog("db rollback failed");
	}

	return ret;
}

bool COptMsgHandler::dbCommit()
{
	bool ret = m_pDBOpt->commit();
	if (!ret)
	{
		const char* msg = m_pDBOpt->errorInfo();
		if (msg == NULL) msg = "";
		OptErrorLog("db commit failed, error = %u, msg = %s", m_pDBOpt->errorNum(), msg);
	}
	
	return ret;
}

// 获取棋牌室信息
int COptMsgHandler::getChessHallInfo(const char* hall_id, com_protocol::BSChessHallInfo& chessHallInfo)
{
	int rc = getChessHallBaseInfo(hall_id, chessHallInfo);
	if (rc != SrvOptSuccess) return rc;
	
	// 今日&昨日利润
	const time_t todaySecs = time(NULL);
	const time_t yesterdaySecs = todaySecs - 60 * 60 * 24;
	chessHallInfo.set_today_profit(getTimeOptSumValue(hall_id, com_protocol::EHallPlayerOpt::EOptTaxCost, todaySecs));
	chessHallInfo.set_yesterday_profit(getTimeOptSumValue(hall_id, com_protocol::EHallPlayerOpt::EOptTaxCost, yesterdaySecs));
	
	// 今日&昨日充值
	chessHallInfo.set_today_recharge(getTimeOptSumValue(hall_id, com_protocol::EHallPlayerOpt::EOptGiveGift, todaySecs));
	chessHallInfo.set_yesterday_recharge(getTimeOptSumValue(hall_id, com_protocol::EHallPlayerOpt::EOptGiveGift, yesterdaySecs));
	
	// 当前在线人数
	chessHallInfo.set_current_onlines(getPlayerCount(hall_id, com_protocol::EHallPlayerStatus::EOnlinePlayer));
	
	// 今日同时最高在线人数
	if (!chessHallInfo.has_max_onlines() || chessHallInfo.current_onlines() > chessHallInfo.max_onlines())
	{
		chessHallInfo.set_max_onlines(chessHallInfo.current_onlines());
	}
	
	return SrvOptSuccess;
}

// 获取棋牌室基本信息
int COptMsgHandler::getChessHallBaseInfo(const char* hall_id, com_protocol::BSChessHallInfo& chessHallInfo)
{
	// 查找棋牌室信息
	char tmpSql[1024] = {0};
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"select H.hall_name,H.hall_logo,UNIX_TIMESTAMP(H.create_time),H.check_flag,H.description,H.rule,H.status,H.contacts_name,H.contacts_mobile_phone,"
	"H.contacts_qq,H.contacts_wx,H.player_init_gold,R.current_gold,R.current_room_card from tb_chess_card_hall as H left join %s.tb_chess_hall_recharge_info as R on "
	"H.hall_id=R.hall_id where H.hall_id=\'%s\' and H.status!=%u;",
	m_srvOpt.getDBCfg().logic_db_cfg.dbname.c_str(), hall_id, com_protocol::EChessHallStatus::EDisbandHall);
	
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess != m_pDBOpt->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
	{
		OptErrorLog("query chess hall info error, sql = %s", tmpSql);
		return BSGetHallInfoError;
	}
	
	if (pResult == NULL || pResult->getRowCount() < 1)
	{
		// 没有数据
		m_pDBOpt->releaseQueryResult(pResult);
		return BSNoFoundHallInfo;
	}
	
	// 棋牌室基本信息
	RowDataPtr rowData = pResult->getNextRow();
	chessHallInfo.set_id(hall_id);
	chessHallInfo.set_name(rowData[0]);
	if (rowData[1] != NULL) chessHallInfo.set_logo(rowData[1]);
	chessHallInfo.set_create_time(atoi(rowData[2]));
	chessHallInfo.set_check_flag((com_protocol::ECheckUserFlag)atoi(rowData[3]));
	chessHallInfo.set_desc(rowData[4]);
	if (rowData[5] != NULL) chessHallInfo.set_rule(rowData[5]);
	chessHallInfo.set_status((com_protocol::EChessHallStatus)atoi(rowData[6]));
	
	if (rowData[7] != NULL) chessHallInfo.set_contacts_name(rowData[7]);
	if (rowData[8] != NULL) chessHallInfo.set_contacts_mobile_phone(rowData[8]);
	if (rowData[9] != NULL) chessHallInfo.set_contacts_qq(rowData[9]);
	if (rowData[10] != NULL) chessHallInfo.set_contacts_wx(rowData[10]);
	if (rowData[11] != NULL) chessHallInfo.set_player_init_gold(atof(rowData[11]));
	if (rowData[12] != NULL) chessHallInfo.set_current_gold(atof(rowData[12]));
	if (rowData[13] != NULL) chessHallInfo.set_current_room_card(atof(rowData[13]));

	m_pDBOpt->releaseQueryResult(pResult);
	
	return SrvOptSuccess;
}

// 获取对应时间点对应操作的总数额
double COptMsgHandler::getTimeOptSumValue(const char* hall_id, com_protocol::EHallPlayerOpt opt, time_t timeSecs, unsigned int gameType)
{
	// select sum(item_count) from tb_user_pay_income_record where hall_id='xx' and opt_type=yy and create_time like '2017-10-18%';
	char tmpSql[512] = {0};
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 2,
	"select sum(item_count) from %s.tb_user_pay_income_record where hall_id=\'%s\' and opt_type=%u",
	m_srvOpt.getDBCfg().logic_db_cfg.dbname.c_str(), hall_id, opt);
	
	if (timeSecs > 0)
	{
		struct tm* pTm = localtime(&timeSecs);
		tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 2,
	    " and create_time like \'%d-%02d-%02d%%\'", (pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday);
	}
	
	if (gameType > 0)
	{
		tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 2, " and game_type=%u", gameType);
	}
	
	tmpSql[tmpSqlLen] = ';';
	tmpSql[tmpSqlLen + 1] = '\0';
	
	double sumValue = 0.00;
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess == m_pDBOpt->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
	{
		if (pResult != NULL && pResult->getRowCount() == 1)
		{
			RowDataPtr rowData = pResult->getNextRow();
			if (rowData[0] != NULL) sumValue = atof(rowData[0]);
		}

		m_pDBOpt->releaseQueryResult(pResult);
	}
	else
	{
		OptErrorLog("query chess hall opt sum value error, sql = %s", tmpSql);
	}
	
	return sumValue;
}

// 获取对应状态的玩家人数
unsigned int COptMsgHandler::getPlayerCount(const char* hall_id, com_protocol::EHallPlayerStatus status, unsigned int gameType)
{
	// 当前对应状态的玩家人数
	// select count(user_status) from tb_chess_hall_user_info where hall_id='xx' and user_status=yy;
	char tmpSql[512] = {0};
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 2,
	"select count(user_status) from %s.tb_chess_hall_user_info where hall_id=\'%s\'",
	m_srvOpt.getDBCfg().logic_db_cfg.dbname.c_str(), hall_id);
	
	if (status == com_protocol::EHallPlayerStatus::EOnlinePlayer)            // 在线玩家
	{
		tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 2, " and user_status=%u and online_hall_service_id>%u",
		com_protocol::EHallPlayerStatus::ECheckAgreedPlayer, ServiceIdBaseValue);
	}
	else if (status == com_protocol::EHallPlayerStatus::ECheckAgreedPlayer)  // 离线玩家
	{
		tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 2, " and user_status=%u and online_hall_service_id=0", status);
	}
	else
	{
		tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 2, " and user_status=%u", status);
	}
	
	if (gameType > 0)
	{
		tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 2, " and (online_game_service_id/%u=%u)",
		ServiceIdBaseValue, gameType);
	}
	
	tmpSql[tmpSqlLen] = ';';
	tmpSql[tmpSqlLen + 1] = '\0';
	
	unsigned int playerCount = 0;
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess == m_pDBOpt->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
	{
		if (pResult != NULL && pResult->getRowCount() == 1)
		{
			RowDataPtr rowData = pResult->getNextRow();
			if (rowData[0] != NULL) playerCount = atoi(rowData[0]);
		}
		
		m_pDBOpt->releaseQueryResult(pResult);
	}
	else
	{
		OptErrorLog("query chess hall player error, sql = %s", tmpSql);
	}
	
	return playerCount;
}

// 刷新在线&离线玩家人数
void COptMsgHandler::updatePlayerCount(unsigned int serviceId, const char* hall_id, com_protocol::EHallPlayerStatus status, unsigned int count)
{
	ChessHallIdToDataMap::iterator hallDataIt = m_hallId2Data.find(hall_id);
	if (hallDataIt != m_hallId2Data.end())
	{
		SChessHallData& chessHallData = hallDataIt->second;
		GameTypeToInfoMap::iterator gameInfoIt = chessHallData.gameInfo.find(serviceId / ServiceIdBaseValue);
		if (status == com_protocol::EHallPlayerStatus::EOnlinePlayer)            // 在线玩家
		{
			const unsigned int hallNewOnlines = chessHallData.baseInfo.current_onlines() + count;
			chessHallData.baseInfo.set_current_onlines(hallNewOnlines);
			chessHallData.offilnePlayers = (chessHallData.offilnePlayers < count) ? 0 : (chessHallData.offilnePlayers - count);
			
			// 今日同时最高在线人数
			if (hallNewOnlines > chessHallData.baseInfo.max_onlines())
			{
				chessHallData.baseInfo.set_max_onlines(hallNewOnlines);
			}

			if (gameInfoIt != chessHallData.gameInfo.end())
			{
				gameInfoIt->second.set_online_players(gameInfoIt->second.online_players() + count);
			}
		}
		else if (status == com_protocol::EHallPlayerStatus::ECheckAgreedPlayer)  // 离线玩家
		{
			unsigned int currentOnlines = chessHallData.baseInfo.current_onlines();
			chessHallData.baseInfo.set_current_onlines((currentOnlines < count) ? 0 : (currentOnlines - count));
			chessHallData.offilnePlayers += count;
			
			if (gameInfoIt != chessHallData.gameInfo.end())
			{
				currentOnlines = gameInfoIt->second.online_players();
				gameInfoIt->second.set_online_players((currentOnlines < count) ? 0 : (currentOnlines - count));
			}
		}
	}
}

// 获取管理员所在的棋牌室信息
bool COptMsgHandler::getManagerChessHallInfo(const char* manager, com_protocol::EManagerLevel level,
                                             google::protobuf::RepeatedPtrField<com_protocol::BSChessHallInfo>& chessHallInfo)
{
	/*
	// 查找管理员所在的棋牌室信息
	// select H.hall_id,H.hall_name,H.create_time,H.check_flag,H.description,H.rule,H.status,G.create_time,G.game_type,
	// G.game_status from tb_chess_card_hall as H left join tb_hall_game_info as G on H.hall_id=G.hall_id where H.status!=3 and 
	// H.hall_id in (select H.hall_id from tb_chess_hall_user_info where username='mName');

    // 联合查找管理员所在的棋牌室信息
	char tmpSql[10240] = {0};
	unsigned int tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"select H.hall_id,H.hall_name,H.create_time,H.check_flag,H.description,H.rule,H.status,G.create_time,G.game_type,G.game_status "
	"from tb_chess_card_hall as H left join tb_hall_game_info as G on H.hall_id=G.hall_id where H.status!=%u and "
	"H.hall_id in (select H.hall_id from tb_chess_hall_user_info where username=\'%s\');",
	com_protocol::EChessHallStatus::EDisbandHall, manager, level);
	
	CQueryResult* pResult = NULL;
	if (SrvOptSuccess != m_pDBOpt->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
	{
		OptErrorLog("query chess hall info error, sql = %s", tmpSql);
		return false;
	}
	
	if (pResult == NULL || pResult->getRowCount() < 1)
	{
		// 没有数据
		m_pDBOpt->releaseQueryResult(pResult);
		return false;
	}

    // 组合查找棋牌室管理员和待审核用户sql
	// select H.hall_id,H.username,U.nickname,U.portrait_id,H.user_remark,H.create_time,H.user_status,H.explain_msg from
	// tb_chess_hall_user_info as H left join youju_logic_db.tb_user_static_baseinfo as U on H.username=U.username 
	// where H.user_status=0 and H.hall_id='xx' or H.hall_id='yy';
	tmpSqlLen = snprintf(tmpSql, sizeof(tmpSql) - 1,
	"select H.hall_id,H.username,U.nickname,U.portrait_id,H.user_remark,H.create_time,H.user_status,H.explain_msg "
	"from tb_chess_hall_user_info as H left join %s.tb_user_static_baseinfo as U on H.username=U.username "
	"where H.user_status=%u and ", m_pDBCfg->logic_db_cfg.dbname.c_str(), com_protocol::EHallPlayerStatus::EWaitForCheck);

    typedef google::protobuf::RepeatedPtrField<com_protocol::BSChessHallInfo> ProtCHInfoVector;

	// 棋牌室信息
	RowDataPtr rowData = NULL;
	while ((rowData = pResult->getNextRow()) != NULL)
	{
		com_protocol::BSChessHallInfo* chHlInfo = NULL;
		for (ProtCHInfoVector::iterator chInfoIt = chessHallInfo.begin(); chInfoIt != chessHallInfo.end(); ++chInfoIt)
		{
			if (chInfoIt->id() == rowData[0])
			{
				chHlInfo = &(*chInfoIt);
				break;
			}
		}
		
		if (chHlInfo == NULL)
		{
			// 基本信息
			chHlInfo = chessHallInfo.Add();
		    chHlInfo->set_id(rowData[0]);
			chHlInfo->set_name(rowData[1]);
			chHlInfo->set_create_time(rowData[2]);
			chHlInfo->set_check_flag((com_protocol::ECheckUserFlag)atoi(rowData[3]));
			chHlInfo->set_desc(rowData[4]);
			chHlInfo->set_rule(rowData[5]);
			chHlInfo->set_status((com_protocol::EChessHallStatus)atoi(rowData[6]));
			
			// 组合查找棋牌室用户sql语句
		    tmpSqlLen += snprintf(tmpSql + tmpSqlLen, sizeof(tmpSql) - tmpSqlLen - 1, "H.hall_id=\'%s\' or ", rowData[0]);
		}
		
		if (rowData[9] != NULL)  // 游戏状态
		{
			// 游戏信息
			com_protocol::BSHallGameInfo* hgInfo = (com_protocol::EHallGameStatus::ENormalGame == atoi(rowData[9])) ? chHlInfo->add_enable_games() : chHlInfo->add_shield_games();
			hgInfo->set_add_time(rowData[7]);
			hgInfo->set_type(atoi(rowData[8]));
			
			map<unsigned int, ServiceCommonConfig::GameInfo>::const_iterator gIfIt = m_pServiceCfg->game_info_map.find(hgInfo->type());
			if (gIfIt != m_pServiceCfg->game_info_map.end())
			{
			    hgInfo->set_name(gIfIt->second.name);
			    hgInfo->set_desc(gIfIt->second.desc);
			}
		}
	}
	m_pDBOpt->releaseQueryResult(pResult);

	tmpSqlLen -= 4; // 注意去掉最后多余的" or "
	tmpSql[tmpSqlLen] = ';';
	tmpSql[tmpSqlLen + 1] = '\0';
	
	pResult = NULL;
	if (SrvOptSuccess != m_pDBOpt->queryTableAllResult(tmpSql, tmpSqlLen, pResult))
	{
		OptErrorLog("query chess hall user error, sql = %s", tmpSql);
		return false;
	}
	
	if (pResult == NULL || pResult->getRowCount() < 1)
	{
		// 没有数据
		// 至少存在一个管理员，没有则出错了
		OptErrorLog("query chess hall user error, row count = %d, sql = %s", (pResult != NULL) ? pResult->getRowCount() : 0, tmpSql);
		m_pDBOpt->releaseQueryResult(pResult);
		return false;
	}
	
	// 棋牌室用户信息
	while ((rowData = pResult->getNextRow()) != NULL)
	{
		com_protocol::BSChessHallInfo* chHlInfo = NULL;
		for (ProtCHInfoVector::iterator chInfoIt = chessHallInfo.begin(); chInfoIt != chessHallInfo.end(); ++chInfoIt)
		{
			if (chInfoIt->id() == rowData[0])
			{
				chHlInfo = &(*chInfoIt);
				break;
			}
		}
		
		if (chHlInfo != NULL)
		{
			// 用户（管理员&待审核玩家）信息
			// select H.hall_id,H.username,U.nickname,U.portrait_id,H.user_remark,H.create_time,H.user_status,H.explain_msg
			const com_protocol::EHallUserType userType = (com_protocol::EHallUserType)atoi(rowData[1]);
			com_protocol::BSHallUserInfo* huInfo = (com_protocol::EHallUserType::EPlayUser == userType) ? chHlInfo->add_ckeck_players() : chHlInfo->add_managers();
			huInfo->set_user_id(rowData[2]);
			if (rowData[3] != NULL) huInfo->set_nick_name(rowData[3]);
			if (rowData[4] != NULL) huInfo->set_portrait_id(rowData[4]);
			if (rowData[5] != NULL) huInfo->set_user_remark(rowData[5]);
			if (com_protocol::EHallUserType::EPlayUser == userType) huInfo->set_explain_msg(rowData[8]);
			huInfo->set_add_time(rowData[6]);
			huInfo->set_user_type(userType);
		}
	}
	m_pDBOpt->releaseQueryResult(pResult);
	
	// 可以添加的游戏信息
	typedef google::protobuf::RepeatedPtrField<com_protocol::BSHallGameInfo> ProtCHGameInfoVector;
	for (ProtCHInfoVector::iterator chInfoIt = chessHallInfo.begin(); chInfoIt != chessHallInfo.end(); ++chInfoIt)
	{
		for (map<unsigned int, ServiceCommonConfig::GameInfo>::const_iterator gIfIt = m_pServiceCfg->game_info_map.begin();
		     gIfIt != m_pServiceCfg->game_info_map.end(); ++gIfIt)
		{
			bool isFind = false;
			for (ProtCHGameInfoVector::const_iterator protGInfoIt = chInfoIt->enable_games().begin();
			     protGInfoIt != chInfoIt->enable_games().end(); ++protGInfoIt)
		    {
				if (protGInfoIt->type() == gIfIt->first)
				{
					isFind = true;
					break;
				}
			}
			
			if (!isFind)
			{	
				for (ProtCHGameInfoVector::const_iterator protGInfoIt = chInfoIt->shield_games().begin();
					 protGInfoIt != chInfoIt->shield_games().end(); ++protGInfoIt)
				{
					if (protGInfoIt->type() == gIfIt->first)
					{
						isFind = true;
						break;
					}
				}
				
				if (!isFind)
				{
					// 都没有就添加游戏
					com_protocol::BSHallGameInfo* hgInfo = chInfoIt->add_can_add_games();
					hgInfo->set_type(gIfIt->first);
					hgInfo->set_name(gIfIt->second.name);
					hgInfo->set_desc(gIfIt->second.desc);
				}
			}
		}
	}
	*/
	
	return false;
}

void COptMsgHandler::recordUserEnterService(const com_protocol::UserEnterGameNotify& nf, int userType)
{
	char sql[1024] = {0};
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1,
	                                     "insert into tb_online_user_data(username,user_type,hall_id,game_type,room_id,room_rate,server_id,online_ip,online_time,status)"
                                         " values(\'%s\',%u,\'%s\',%u,\'%s\',%u,%u,\'%s\',now(),%u);",
										 nf.username().c_str(), userType, nf.hall_id().c_str(), nf.service_id() / ServiceIdBaseValue, nf.room_id().c_str(), nf.room_rate(),
										 nf.service_id(), nf.online_ip().c_str(), com_protocol::EUserQuitStatus::EUserOnline);

	if (SrvOptSuccess != m_pDBOpt->executeSQL(sql, sqlLen))
	{
		OptErrorLog("user enter service exeSql failed|%s|%s", nf.username().c_str(), sql);
	}
	
	// if (userType == com_protocol::EHallUserType::EPlayUser) updatePlayerCount(nf.service_id(), nf.hall_id().c_str(), com_protocol::EHallPlayerStatus::EOnlinePlayer);
	
	// OptErrorLog("test user enter service, name = %s, hallId = %s, roomId = %s, serviceId = %u, rate = %d",
	// nf.username().c_str(), nf.hall_id().c_str(), nf.room_id().c_str(), nf.service_id(), nf.room_rate());
}

void COptMsgHandler::recordUserLeaveService(const com_protocol::UserLeaveGameNotify& nf, int userType)
{
	char sql[1024] = {0};
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1,
	                                     "update tb_online_user_data set status=%d,offline_time=now() where username=\'%s\' and status=%u and server_id=%u and user_type=%u;",
										 nf.code(), nf.username().c_str(), com_protocol::EUserQuitStatus::EUserOnline, nf.service_id(), userType);

	if (SrvOptSuccess != m_pDBOpt->executeSQL(sql, sqlLen))
	{
		OptErrorLog("user leave service exeSql failed|%s|%s", nf.username().c_str(), sql);
	}
	
	// if (userType == com_protocol::EHallUserType::EPlayUser) updatePlayerCount(nf.service_id(), nf.hall_id().c_str(), com_protocol::EHallPlayerStatus::ECheckAgreedPlayer);
	
	// OptErrorLog("test user leave service, name = %s, serviceId = %u, code = %d",
	// nf.username().c_str(), nf.service_id(), nf.code());
}


void COptMsgHandler::msgRecordToDB(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{	
	if (m_msgRecord.empty()) return;

	CQueryResult* qResult = NULL;
	const int rc = m_pDBOpt->executeMultiSql(m_msgRecord.c_str(), m_msgRecord.length(), qResult);
	if (qResult != NULL) m_pDBOpt->releaseQueryResult(qResult);
	
	if (SrvOptSuccess != rc) OptErrorLog("write chat record to db error, rc = %d, sql = %s", rc, m_msgRecord.c_str());
	
	// 成功刷新到DB了则清空，失败了也必须清空，否则后面会一直失败
	m_msgRecord.clear();
}

void COptMsgHandler::updateServiceData(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	std::vector<std::string> strHallData;
	int rc = m_redisDbOpt.getHAll(GameServiceListKey, GameServiceListLen, strHallData);
	if (rc != 0)
	{
	    OptErrorLog("get game service info from redis error, rc = %d", rc);
		return;
	}
	
	static const unsigned int maxBuffSize = 1024 * 512;
	char sql_tmp[maxBuffSize] = "delete from tb_online_service_data;";
	static const unsigned int deleteSqlLen = strlen(sql_tmp);

    // 游戏服务信息
	char cmd[2048] = {0};
	unsigned int cmdLen = 0;
	const unsigned int currentSecs = time(NULL);
	com_protocol::GameServiceInfo gmSrvData;
	for (unsigned int i = 1; i < strHallData.size(); i += 2)
	{
		if (gmSrvData.ParseFromArray(strHallData[i].data(), strHallData[i].length()))
		{
			if ((currentSecs - gmSrvData.update_timestamp()) < m_serviceValidIntervalSecs)
			{
				cmdLen += snprintf(cmd, sizeof(cmd) - 1, "insert into tb_online_service_data(update_time,server_id,server_type,server_name,current_online_persons,max_online_persons) values(now(),%u,%u,\'%s\',%u,%u);",
					               gmSrvData.id(), gmSrvData.type(), gmSrvData.name().c_str(), gmSrvData.current_persons(), gmSrvData.max_persons());

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
	
	// 大厅信息
	strHallData.clear();
	rc = m_redisDbOpt.getHAll(GameHallListKey, GameHallListLen, strHallData);
	if (rc == 0)
	{
		for (unsigned int i = 1; i < strHallData.size(); i += 2)
		{
			const GameHallServiceData* hallData = (const GameHallServiceData*)strHallData[i].data();
			if ((currentSecs - hallData->curTimeSecs) < m_serviceValidIntervalSecs)
			{
				unsigned int id = *((unsigned int*)(strHallData[i - 1].data()));
				cmdLen += snprintf(cmd, sizeof(cmd) - 1, "insert into tb_online_service_data(update_time,server_id,server_type,server_name,current_online_persons,max_online_persons) values(now(),%u,%u,\'%s\',%u,%u);",
								   id, id / ServiceIdBaseValue, "GameHall", hallData->currentPersons, 0);
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
	else
	{
		OptErrorLog("get game hall info from redis error, rc = %d", rc);
	}
	
	CQueryResult* qResult = NULL;
	rc = m_pDBOpt->executeMultiSql(sql_tmp, qResult);
	if (rc == SrvOptSuccess)
	{
		m_pDBOpt->releaseQueryResult(qResult);
	}
	else
	{
		OptErrorLog("update service data exe sql failed, rc = %d, sql = %s", rc, sql_tmp);
	}
}

// 执行定时任务
void COptMsgHandler::onTaskTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	OptInfoLog("do task time begin, current timer id = %u, remain count = %u", timerId, remainCount);
    
    // 执行任务
    doTask();

	// 重新设置定时器
	struct tm tmval;
	time_t curSecs = time(NULL);
	unsigned int intervalSecs = OneDaySeconds;
	if (localtime_r(&curSecs, &tmval) != NULL)
	{
		tmval.tm_sec = 0;
		tmval.tm_min = 0;
		tmval.tm_hour = m_pCfg->timing_task.day_hour;
		++tmval.tm_mday;
		time_t nextSecs = mktime(&tmval);
		if (nextSecs != (time_t)-1)
		{
			intervalSecs = nextSecs - curSecs;
		}
		else
		{
			OptErrorLog("do task timer, get next time error");
		}
	}
	else
	{
		OptErrorLog("do task timer, get local time error");
	}

	unsigned int tId = setTimer(intervalSecs * MillisecondUnit, (TimerHandler)&COptMsgHandler::onTaskTimer);
	OptInfoLog("do task time end, next timer id = %u, interval = %u, date = %d-%02d-%02d %02d:%02d:%02d",
	tId, intervalSecs, tmval.tm_year + 1900, tmval.tm_mon + 1, tmval.tm_mday, tmval.tm_hour, tmval.tm_min, tmval.tm_sec);
}


void COptMsgHandler::registerManager(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 先初始化连接
	ConnectProxy* conn = getConnectProxyContext().conn;
	OptMgrUserData* curCud = (OptMgrUserData*)getProxyUserData(conn);
	if (curCud == NULL) curCud = (OptMgrUserData*)m_srvOpt.createConnectProxyUserData(conn);

	com_protocol::RegisterManagerReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "register manager request")) return;

    int rc = SrvOptFailed;
	bool needReleaseChessHallInfo = false;
    com_protocol::RegisterManagerRsp rsp;

	do
	{
		/*
		// 检查客户端版本号是否可用
		int rc = checkClientVersion(req.os_type(), req.platform_type(), req.version(), curCud->versionFlag);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		*/
		
		// 新注册管理员
		rc = newRegisterManager(CSocket::toIPStr(conn->peerIp), req);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		// 管理员登录
		com_protocol::BSLoginInfo* info = rsp.mutable_info();
	    
		rc = login(conn, curCud, req.platform_type(), req.platform_id(), *req.mutable_name(), *req.mutable_password(), *info, needReleaseChessHallInfo);
		if (rc != SrvOptSuccess)
		{
			rsp.clear_info();
			rsp.set_result(rc);
			break;
		}

        rsp.set_result(SrvOptSuccess);
		rsp.set_platform_id(req.platform_id());
		rsp.set_name(req.name());
		rsp.set_password(req.password());

	} while (false);
	
	rc = m_srvOpt.sendMsgPkgToProxy(rsp, BSC_MANAGER_REGISTER_RSP, conn, "register manager reply");
	
	if (needReleaseChessHallInfo) rsp.mutable_info()->mutable_chess_hall()->ReleaseLast();
	
	OptInfoLog("send register business manager result to client, platform type = %u, id = %s, name = %s, ip = %s, result = %d, send msg rc = %d",
	req.platform_type(), req.platform_id().c_str(), req.name().c_str(), CSocket::toIPStr(conn->peerIp), rsp.result(), rc);
	
	// 失败可以考虑直接关闭该连接了
}

void COptMsgHandler::managerLogin(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 先初始化连接
	ConnectProxy* conn = getConnectProxyContext().conn;
	OptMgrUserData* curCud = (OptMgrUserData*)getProxyUserData(conn);
	if (curCud == NULL) curCud = (OptMgrUserData*)m_srvOpt.createConnectProxyUserData(conn);
	
	com_protocol::ManagerLoginReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "manager login request")) return;
	
	// 第三方登录信息验证
	int rc = SrvOptFailed;
	if (req.platform_type() == EBusinessManagerWeiXin)  // 微信登录&验证
	{
		com_protocol::WXLoginReq wxLoginReq;
		wxLoginReq.set_platform_type(EBusinessManagerWeiXin);
		wxLoginReq.set_cb_data(data, len);
		
		if (req.has_name()) wxLoginReq.set_username(req.name());
		if (req.has_platform_id()) wxLoginReq.set_platform_id(req.platform_id());
		if (req.third_check_data().kv_size() > 0) wxLoginReq.set_code(req.third_check_data().kv(0).value());
		
		rc = m_srvOpt.sendPkgMsgToService(curCud->userName, curCud->userNameLen, ServiceType::HttpOperationSrv, wxLoginReq, HTTPOPT_WX_LOGIN_REQ, "manager wx login request");
		
		OptInfoLog("receive message to wx login, name = %s, platform type = %u, version = %s, userName = %s, status = %d, time = %u, rc = %d",
		req.name().c_str(), req.platform_type(), req.version().c_str(),
		curCud->userName, curCud->status, curCud->timeSecs, rc);
		
		return;
	}

    bool needReleaseChessHallInfo = false;
    com_protocol::ManagerLoginRsp rsp;

	do
	{
		// 管理员所在的棋牌室信息
		com_protocol::BSLoginInfo* info = rsp.mutable_info();
		rc = login(conn, curCud, req.platform_type(), req.platform_id(), *req.mutable_name(), *req.mutable_password(), *info, needReleaseChessHallInfo);
		if (rc != SrvOptSuccess)
		{
			rsp.clear_info();
			rsp.set_result(rc);
			break;
		}

        // 登录成功了
		rsp.set_result(SrvOptSuccess);
		rsp.set_platform_id(req.platform_id());
		rsp.set_name(req.name());
		rsp.set_password(req.password());

	} while (false);
	
	rc = m_srvOpt.sendMsgPkgToProxy(rsp, BSC_MANAGER_LOGIN_RSP, conn, "manager login reply");
	
	if (needReleaseChessHallInfo) rsp.mutable_info()->mutable_chess_hall()->ReleaseLast();
	
	OptInfoLog("manager login platform type = %u, id = %s, name = %s, ip = %s, result = %d, send msg rc = %d",
	req.platform_type(), req.platform_id().c_str(), req.name().c_str(), CSocket::toIPStr(conn->peerIp), rsp.result(), rc);
	
	// 失败可以考虑直接关闭该连接了
}

void COptMsgHandler::managerWxLoginReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(string(getContext().userData));
	if (conn == NULL)
	{
		OptErrorLog("manager wei xin login reply can not find the connect data, user data = %s", getContext().userData);
		return;
	}
	
	com_protocol::WXLoginRsp wxLoginRsp;
	if (!m_srvOpt.parseMsgFromBuffer(wxLoginRsp, data, len, "manager wei xin login reply")) return;

    int rc = SrvOptFailed;
    OptMgrUserData* curCud = (OptMgrUserData*)getProxyUserData(conn);
    com_protocol::ManagerLoginReq loginReq;

    bool needReleaseChessHallInfo = false;
    com_protocol::ManagerLoginRsp rsp;
	rsp.set_result(wxLoginRsp.result());

	while (rsp.result() == SrvOptSuccess)
	{
	    if (!m_srvOpt.parseMsgFromBuffer(loginReq, wxLoginRsp.cb_data().c_str(), wxLoginRsp.cb_data().length(), "wei xin reply cb data")) return;

        loginReq.set_platform_id(wxLoginRsp.platform_id());
	    if (!loginReq.has_name())
		{
			// 检查平台信息是否已经注册过了
	        rc = checkRegisterPlatformInfo(loginReq.platform_type(), loginReq.platform_id());
			if (rc == BSQueryPlatformInfoError)
			{
				rsp.set_result(rc);
				break;
			}
			
			if (rc == SrvOptSuccess)
			{
				// 没有则自动注册新账号
				com_protocol::RegisterManagerReq req;
				req.set_platform_type(loginReq.platform_type());
				req.set_platform_id(loginReq.platform_id());
				req.set_os_type(loginReq.os_type());
				if (loginReq.has_password()) req.set_password(loginReq.password());
				
				req.set_nickname(wxLoginRsp.user_static_info().nickname());
				req.set_portrait_id(wxLoginRsp.user_static_info().portrait_id());
				req.set_gender(wxLoginRsp.user_static_info().gender());
				
				// 新注册管理员
				rc = newRegisterManager(CSocket::toIPStr(conn->peerIp), req);
				if (rc != SrvOptSuccess)
				{
					rsp.set_result(rc);
					break;
				}
			}
		}
		
		// 管理员所在的棋牌室信息
		com_protocol::BSLoginInfo* info = rsp.mutable_info();
		rc = login(conn, curCud, loginReq.platform_type(), loginReq.platform_id(), *loginReq.mutable_name(), *loginReq.mutable_password(), *info, needReleaseChessHallInfo);
		if (rc != SrvOptSuccess)
		{
			rsp.clear_info();
			rsp.set_result(rc);
			break;
		}

        // 登录成功了
		rsp.set_platform_id(loginReq.platform_id());
		rsp.set_name(loginReq.name());
		rsp.set_password(loginReq.password());

		break;
	}
	
	rc = m_srvOpt.sendMsgPkgToProxy(rsp, BSC_MANAGER_LOGIN_RSP, conn, "manager wei xin login reply");
	
	if (needReleaseChessHallInfo) rsp.mutable_info()->mutable_chess_hall()->ReleaseLast();
	
	OptInfoLog("manager wei xin login platform type = %u, id = %s, name = %s, ip = %s, result = %d, send msg rc = %d",
	loginReq.platform_type(), loginReq.platform_id().c_str(), loginReq.name().c_str(), CSocket::toIPStr(conn->peerIp), rsp.result(), rc);

	// 验证失败可以考虑直接关闭该连接了
}

void COptMsgHandler::managerLogout(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    OptMgrUserData* ud = (OptMgrUserData*)getProxyUserData(getConnectProxyContext().conn);
	
	OptInfoLog("user logout name = %s, status = %u", ud->userName, ud->status);
	
	removeConnectProxy(string(ud->userName), com_protocol::EUserQuitStatus::EUserOffline);  // 删除关闭与客户端的连接
}


void COptMsgHandler::userEnterServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserEnterGameNotify nf;
	if (!m_srvOpt.parseMsgFromBuffer(nf, data, len, "user enter game notify")) return;

	recordUserEnterService(nf, com_protocol::EHallUserType::EPlayUser);
}

void COptMsgHandler::userLeaveServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserLeaveGameNotify nf;
	if (!m_srvOpt.parseMsgFromBuffer(nf, data, len, "user leave game notify")) return;

	recordUserLeaveService(nf, com_protocol::EHallUserType::EPlayUser);
}

// 服务状态通知
void COptMsgHandler::serviceStatusNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ServiceStatusNotify nf;
	if (!m_srvOpt.parseMsgFromBuffer(nf, data, len, "service status notify")) return;

	char sql[512] = {0};
	const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "update tb_online_service_data set update_time=now(),current_online_persons=0 where server_id=%u;"
	                                     "update tb_online_user_data set status=%u,offline_time=now() where status=%u and server_id=%u;",
			                             nf.service_id(), nf.service_status(), com_protocol::EUserQuitStatus::EUserOnline, nf.service_id());
	
	CQueryResult* qResult = NULL;
	int rc = m_pDBOpt->executeMultiSql(sql, sqlLen, qResult);
	if (rc == SrvOptSuccess)
	{
		m_pDBOpt->releaseQueryResult(qResult);
	}
	else
	{
		OptErrorLog("service status exeSql failed|%s", sql);
	}
	
	for (ChessHallIdToDataMap::iterator hallDataIt = m_hallId2Data.begin(); hallDataIt != m_hallId2Data.end(); ++hallDataIt)
	{
		hallDataIt->second.needUpdatePlayer = true;
	}

	OptWarnLog("service status notify id = %u, status = %d, type = %u, name = %s, rc = %d",
	nf.service_id(), nf.service_status(), nf.service_type(), nf.service_name().c_str(), rc);
}

void COptMsgHandler::msgRecordNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::MessageRecordNotify msgNotify;
	if (!m_srvOpt.parseMsgFromBuffer(msgNotify, data, len, "chat record notify")) return;

	if (msgNotify.chat_context().length() > m_pCfg->common_cfg.max_chat_context_size)
	{
		OptErrorLog("the chat context is large error, size = %u, limit = %u",
		msgNotify.chat_context().length(), m_pCfg->common_cfg.max_chat_context_size);

		return;
	}
	
	char buff[2048] = {0};
	unsigned long sqlLen = snprintf(buff, sizeof(buff) - 1, "insert into tb_user_chat_record(username,chat_time,hall_id,game_type,room_rate,chat_type,chat_context) values(\'%s\',now(),\'%s\',%u,%u,%u,\'%s\');",
	msgNotify.username().c_str(), msgNotify.hall_id().c_str(), msgNotify.game_type(), msgNotify.room_rate(), msgNotify.chat_type(), msgNotify.chat_context().c_str());
	
	char sql[2048] = {0};
	if (m_pDBOpt->realEscapeString(sql, buff, sqlLen))
	{
		sql[sqlLen] = '\0';
		m_msgRecord += sql;

		if (m_msgRecord.length() > m_pCfg->common_cfg.max_chat_record_size_to_db) msgRecordToDB(0, 0, NULL, 0);
	}
	else
	{
		OptErrorLog("chat record real escape string error, sql = %s", buff);
	}
}



static COptMsgHandler s_srvMsgHandler;  // 消息处理实例

void COperationManager::onRegister(const char* name, const unsigned int id)
{
	ReleaseInfoLog("register operation mananger module, service name = %s, id = %d", name, id);
	
	// 注册模块实例
	const unsigned short srvModuleId = 0;
	registerModule(srvModuleId, &s_srvMsgHandler);              // 网关代理服务&内部服务消息处理模块

	registerHttpHandler(&s_srvMsgHandler.getHttpMsgHandler());  // HTTP消息处理模块
}

// 通知逻辑层对应的逻辑连接代理已被关闭
void COperationManager::onCloseConnectProxy(void* userData, int cbFlag)
{
	s_srvMsgHandler.onCloseConnectProxy(userData, cbFlag);
}

COperationManager::COperationManager() : CHttpSrv(ServiceType::OperationManageSrv)
{
}

COperationManager::~COperationManager()
{
}


REGISTER_SERVICE(COperationManager);

