
/* author : liuxu
* date : 2015.03.03
* description : 消息处理类
*/
 
#ifndef __CSVR_MSG_HANDLER_H__
#define __CSVR_MSG_HANDLER_H__

#include <string.h>
#include "SrvFrame/CModule.h"
#include "SrvFrame/CDataContent.h"
#include "../common/CMonitorStat.h"
#include "../common/CSensitiveWordFilter.h"
#include "db/CMySql.h"
#include "db/CMemcache.h"
#include "base/CCfg.h"
#include "db/CMemcache.h"
#include "db/CRedis.h"
#include <unordered_map>
#include <string>
#include "base/CMemManager.h"
#include "_DbproxyCommonConfig_.h"
#include "app_server_dbproxy_common.pb.h"
#include "CIP2City.h"
#include "CLogicHandler.h"
#include "CLogicHandlerTwo.h"
#include "CLogicHandlerThree.h"
#include "CFFChessLogicHandler.h"
#include "CLogicData.h"
#include "MessageDefine.h"


using namespace NCommon;
using namespace NFrame;
using namespace std;
using namespace NDBOpt;
using namespace NErrorCode;

class CLogic;
class CGameRecord;

class CSrvMsgHandler : public NFrame::CModule
{
public:
	CSrvMsgHandler();
	~CSrvMsgHandler();

	void loadConfig();

	void onTimerMsg(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	CDBOpertaion* getMySqlOptDB();

	//消息处理
	void handldMsgRegisterUserReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgModifyPasswordReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgModifyBaseinfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgVeriyfyPasswordReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgGetUserBaseinfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgRoundEndDataChargeReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgLoginNotifyReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgLogoutNotifyReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgGetMultiUserInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgGetPropInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgPropChargeReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgGetUsernameByIMEIReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgMModifyPersonStatusReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgMModifyPasswordReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgMModifyBaseinfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgGetUsernameByPlatformReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void handldMsgChangeUserPropertyValueReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void handldMsgChangeManyPeoplePropertyValueReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	//游戏记录
	void handldMsgGameRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void handldPkPlayRankingListReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//获取PK场名人堂信息
	void handldGetPKPlayCelebrityReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//同步memcached数据到mysql的相关操作
	enum UpdateOpt
	{
		Login = 0,
		Logout,
		Modify,
	};
	void updateOptStatus(const string& username, UpdateOpt upate_opt);
	void commitToMysql(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void checkToCommit(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void dbConnectCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void dbHasDisconnectCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	int autoRegister(string &username, const string& platform_id, uint32_t platform_type, uint32_t os_type, const string &nickname = "", uint32_t city_id = 0, int gender = 0);

	//常用DB操作
	bool getUserBaseinfo(const string &username, CUserBaseinfo &user_baseinfo, bool just_memcached = false);
	bool setUserBaseinfoToMemcached(const CUserBaseinfo &user_baseinfo);
	bool updateUserStaticBaseinfoToMysql(const CUserStaticBaseinfo &user_static_baseinfo); //调用成功后，需执行mysqlCommit()提交修改;
	bool updateUserDynamicBaseinfoToMysql(const CUserDynamicBaseinfo &user_dynamic_baseinfo); //调用成功后，需执行mysqlCommit()提交修改;
	bool updateUserPropInfoToMysql(const CUserPropInfo &user_prop_info); //调用成功后，需执行mysqlCommit()提交修改;
	bool mysqlRollback();
	bool mysqlCommit();
	bool dbIsConnect();
	
	void batchRegisterUser();
	
public:
	bool parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info);
	const char* serializeMsgToBuffer(const ::google::protobuf::Message& msg, char* buffer, unsigned int& len, const char* info);
	
    CQueryResult* queryUserInfo(const char* sql, const unsigned int len);
	void releaseQueryResult(CQueryResult* rst);
	
	CLogicHandler& getLogicHander();
	CLogic& getLogic();
	
	CLogicData& getLogicDataInstance();
	CRedis& getLogicRedisService();
	CSensitiveWordFilter& getSensitiveWordFilter();

public:
    void onUpdateConfig();
	
	std::string utf8SubStr(const char* str, int start, int num);
	
	void resetUserBaseInfo(const com_protocol::ResetUserOtherInfoReq& req, com_protocol::ResetUserOtherInfoRsp& rsp, CUserBaseinfo& user_baseinfo);

private:
	void getUserOtherInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void getManyUserOtherInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void getUserPropAndPreventionCheatReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 重置用户信息，目前只给内部机器人使用
	void resetUserOtherInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 应用宝QQ登录，兼容新老版本用户，解决老版本使用qq昵称作为平台ID问题
	bool handleTXQQPlatformatId(com_protocol::GetUsernameByPlatformReq& req, string& username);
	
	// 小时定时器，自服务启动开始算，每一小时触发一次
	void hourOnTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	// 找到满足条件的PK场金币门票
    bool findPKGoldTicket(const com_protocol::PKTicket* pkTicket, unsigned int& ticketIndex, const unsigned int ticketId = 0, const int count = 0);

public:
    // 金币&道具&玩家属性等数量变更修改
    int changeUserPropertyValue(const char* userData, const unsigned int userDataLen, const google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo>& goodsList, const com_protocol::BuyuGameRecordStorageExt& recordData);
	
    // 金币&道具&玩家属性等数量变更修改
    int changeUserPropertyValue(const char* userData, const unsigned int userDataLen, const google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo>& goodsList, const com_protocol::GameRecordPkg* record = NULL, google::protobuf::RepeatedPtrField<com_protocol::PropItem>* amountValueList = NULL, google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>* changeValueList = NULL);
	
	// 金币&道具&玩家属性等数量变更修改
    int changeUserPropertyValue(const char* userData, const unsigned int userDataLen, const google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo>& goodsList, CUserBaseinfo& user_baseinfo, google::protobuf::RepeatedPtrField<com_protocol::PropItem>* amountValueList = NULL, google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>* changeValueList = NULL);
	
	// 修改PK场金币门票数量
	int changePKGoldTicketValue(const char* userName, unsigned int len, const PKGoldTicketVector& pkGoldTicketVector);
	
	// 删除过期的金币门票
	com_protocol::PKTicket* removeOverdueGoldTicket(const char* userName, unsigned int len);
	
private:
    // only for test，测试异步消息本地数据存储
	void testAsyncLocalData();
	void testAsyncLocalDataRequest(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	

public:
	CDBOpertaion *m_pDBOpt;			// 游戏逻辑数据库
	DbproxyCommonConfig::config m_config;
	CMemManager *m_pMManageUserModifyStat;
	unordered_map<string, CUserModifyStat*> m_umapHasModify;
	unordered_map<string, CUserModifyStat*> m_umapCommitToDB;
	CGameRecord *m_p_game_record;
	CIP2City m_ip2city;

private:
    CMemcache m_memcache;
	
	CLogic *m_pLogic;
	CLogicHandler m_logicHandler;
	CLogicData m_logicData;

	NProject::CMonitorStat m_monitor_stat;
	CSensitiveWordFilter m_senitiveWordFilter;

	uint32_t m_check_timer;
	uint32_t m_db_connect_check_timer;
	uint32_t m_cur_connect_failed_count;
	
	CRedis m_logicRedisDbOpt;
	CDBOpertaion *m_pMySqlOptDb;		// 游戏操作数据库
	
	CLogicHandlerTwo m_logicHandlerTwo;
	
	CLogicHandlerThree m_logicHandlerThree;
	
	CLocalAsyncData m_onlyForTestAsyncData;  // only for test，测试异步消息本地数据存储
	
	CFFChessLogicHandler m_ffChessLogicHandler;
};

#endif // __CSVR_MSG_HANDLER_H__
