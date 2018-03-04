
/* author : liuxu
 * modify : limingfan
 * date : 2015.03.03
 * description : 消息处理类
 */
 
#ifndef __CSVR_MSG_HANDLER_H__
#define __CSVR_MSG_HANDLER_H__

#include <string.h>
#include <unordered_map>

#include "base/CCfg.h"
#include "base/CMemManager.h"
#include "db/CMySql.h"
#include "db/CMemcache.h"
#include "db/CRedis.h"
#include "SrvFrame/CModule.h"
#include "SrvFrame/CDataContent.h"
#include "../common/CIP2City.h"
#include "../common/CSensitiveWordFilter.h"

#include "BaseDefine.h"
#include "CGameRecord.h"
#include "CLogicHandler.h"
#include "CServiceLogicData.h"
#include "CChessHallOperation.h"


// 刷新DB操作
enum EUpdateDBOpt
{
	ELoginOpt = 1,
	ELogoutOpt,
	EModifyOpt,
};


using namespace std;
using namespace NCommon;
using namespace NFrame;
using namespace NDBOpt;
using namespace NErrorCode;


class CMsgHandler : public NFrame::CModule
{
public:
	CMsgHandler();
	~CMsgHandler();

public:
	// 获取服务数据
    int getServiceData(const char* firstKey, const unsigned int firstKeyLen, const char* secondKey, const unsigned int secondKeyLen, ::google::protobuf::Message& msg);
	
	// utf8字符截断
	string utf8SubStr(const char* str, unsigned int strLen, unsigned int start, unsigned int num);
	
	// 获取商城物品配置信息
	const MallConfigData::MallGoodsCfg* getMallGoodsCfg(unsigned int type);

    // 常用DB操作
public:
	// 获取玩家数据
	int getUserBaseinfo(const string& username, const string& hall_id, CUserBaseinfo& user_baseinfo, bool just_memcached = false, bool just_db = false);
	
	// 从mysql db中获取静态&动态数据
	int getUserStaticInfoFromMysql(const string& username, CUserStaticBaseinfo& static_info);
	int getUserDynamicInfoFromMysql(const string& username, const string& hall_id, CUserDynamicBaseinfo& dynamic_info);
	
	// 用户详细信息
	void setUserDetailInfo(const CUserBaseinfo& user_baseinfo, com_protocol::DetailInfo& detailInfo);
	void setUserDetailInfo(const CUserBaseinfo& user_baseinfo, com_protocol::StaticInfo* staticInfo, com_protocol::DynamicInfo* dynamicInfo);
	
	CDBOpertaion* getLogicDBOpt();
	CDBOpertaion* getOptDBOpt();
	
	// 回滚&提交数据
	bool mysqlRollback();
	bool mysqlCommit();
	
	// 数据同步到内存DB
	bool setUserBaseinfoToMemcached(const CUserBaseinfo& user_baseinfo);
	
	// 刷新用户静态&动态信息，调用成功后，需执行 mysqlCommit() 提交修改更新到mysql DB;
	bool updateUserStaticBaseinfoToMysql(const CUserStaticBaseinfo& user_static_baseinfo);
	bool updateUserDynamicBaseinfoToMysql(const CUserDynamicBaseinfo& user_dynamic_baseinfo);
	
	// 修改内存DB后需记录操作，定时同步到mysql DB
	void updateOptStatus(const string& username, const EUpdateDBOpt upate_opt);
	
	// 各辅助类对象
public:
	CServiceOperation& getSrvOpt();
	
	CDBLogicHandler& getLogicHandler();
	
	CRedis& getLogicRedisService();
	
	CSensitiveWordFilter& getSensitiveWordFilter();
	
	CServiceLogicData& getSrvLogicData();

	CGameRecord& getGameRecord();
	
public:
	// 配置文件更新回调
	void onUpdateConfig();

private:
	// 同步memcached数据到mysql DB
	void commitToMysql(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void checkToCommit(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	// DB断连检测
	bool dbIsConnect();
	void dbConnectCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void dbHasDisconnectCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
private:
    // 服务生命周期过程
	void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	void loadConfig(bool isReset);
	
private:
	int autoRegisterUser(string& username, string& passwd, const string& platform_id, uint32_t platform_type, uint32_t os_type,
					     const string& nickname, const string& portrait_id, uint32_t city_id, int gender);
	int registerUser(com_protocol::RegisterUserReq& req);
	int getUserStaticBaseInfoSql(com_protocol::RegisterUserReq& req, string& sql);
	
	// 根据第三方平台账号获取内部账号ID
    // 如没有则新注册一个对应该第三方平台账号的内部账户ID
    bool getAndRegisterUserByPlatform(const com_protocol::GetUsernameByPlatformReq& req, com_protocol::GetUsernameByPlatformRsp& rsp);
	
	int checkUserAccount(com_protocol::ClientLoginReq& req, CUserBaseinfo& user_baseinfo);
	int setUserLoginInfo(const CUserBaseinfo& user_baseinfo, com_protocol::ClientLoginRsp& rsp);

    // 各消息协议处理
private:
    // 根据第三方平台账号获取对应的内部账号ID
	// 如没有则新注册一个对应该第三方平台账号的内部账户ID
	void getUsernameByPlatform(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
    // 验证用户，成功则返回用户信息
	// 根据第三方平台账号获取对应的内部账号ID
	// 如没有则新注册一个对应该第三方平台账号的内部账户ID
	void checkUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 玩家进入棋牌室大厅
	void enterChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 用户登录游戏通知
	void userLoginNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 用户登出游戏通知
	void userLogoutNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 服务状态通知
    void serviceStatusNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

public:
    const DbProxyConfig::config* m_pCfg;                        // 公共配置数据，外部可直接访问

private:
	CDBOpertaion* m_pLogicDBOpt;         // 游戏逻辑数据库
	CDBOpertaion* m_pOperationDbOpt;     // 游戏操作数据库
	
	CMemcache m_memcache;
	CRedis m_logicRedisDbOpt;
	
private:
	CMemManager* m_pMManageUserModifyStat;
	unordered_map<string, CUserModifyStat*> m_umapHasModify;   // 修改了的用户数据
	unordered_map<string, CUserModifyStat*> m_umapCommitToDB;  // 需要提交的用户数据

private:
	uint32_t m_check_commit_timer;
	
	uint32_t m_db_connect_check_timer;
	uint32_t m_cur_connect_failed_count;

private:	
	CServiceOperation m_srvOpt;
	
	CIP2City m_ip2city;
	
	CSensitiveWordFilter m_senitiveWordFilter;
	
    CServiceLogicData m_logicData;
	
	CGameRecord m_gameRecord;
	
	CDBLogicHandler m_logicHandler;
	
	CChessHallOperation m_chessHallOpt;
};


#endif // __CSVR_MSG_HANDLER_H__
