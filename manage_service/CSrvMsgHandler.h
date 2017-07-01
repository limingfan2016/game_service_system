
/* author : liuxu
* date : 2015.07.02
* description : 消息处理类
*/
 
#ifndef __CSVR_MSG_HANDLER_H__
#define __CSVR_MSG_HANDLER_H__

#include "../common/CommonType.h"
#include "SrvFrame/CModule.h"
#include "SrvFrame/CGameModule.h"
#include "SrvFrame/CDataContent.h"
#include "db/CMySql.h"
#include "db/CMemcache.h"
#include "db/CRedis.h"
#include "base/CCfg.h"
#include "db/CMemcache.h"
#include <unordered_map>
#include <string>
#include "base/CMemManager.h"
#include "_Config_.h"
#include "app_server_manage_service.pb.h"
#include "ManageUser.h"

using namespace NCommon;
using namespace NFrame;
using namespace std;
using namespace NDBOpt;
using namespace NErrorCode;


struct ConnectUserInfo
{
	ConnectUserInfo() : curTime(0) {};
	ConnectUserInfo(const string& _name, uint32_t _curTime) : name(_name), curTime(_curTime) {};
	ConnectUserInfo(const string& _name, uint32_t _curTime, const string& _data) : name(_name), curTime(_curTime), data(_data) {};
	
	string name;
	uint32_t curTime;
	
	string data;  // 额外存储的任意二进制数据
};

typedef unordered_map<int, ConnectUserInfo> OptIdToConnectUser;


class CCSReplyInfo
{
public:
	uint32_t idx;
	uint32_t id;
	char username[33];
	uint32_t game_type;
	uint32_t create_time;
	char cs_name[64];
	char cs_reply[1024];
	uint32_t award_count;
	uint32_t award_type;
};

class CSrvMsgHandler : public NFrame::CGameModule
{
public:
	CSrvMsgHandler();
	~CSrvMsgHandler();

	void loadConfig();

	void onTimerMsg(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void checkInfo(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);

	//客户端关闭连接通知
	void onClosedConnect(void* userData);   // 通知逻辑层对应的逻辑连接已被关闭

	//处理消息
	void handldMsgGetMonitorStatDataRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgGetHistoryOpinionReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgUploadNewOpinionReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgUserAddOpinionReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//MESSAGE SRV
	void handldMsgGetUserServiceIDRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);


	//功能函数
	uint32_t getCommonDbProxySrvId(const char *username, const int username_len);
	int sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* username, const int username_len,
		unsigned short dstModuleId = 0, unsigned short handleProtocolId = 0);

	int sendMessageToSrv(ServiceType srvType, const char* data, const unsigned int len, unsigned short protocolId, const char* username, const int username_len,
		int userFlag = 0, unsigned short handleProtocolId = 0);

	//辅助函数
	void addAskAndReply(com_protocol::OneCompleteOpinion* p_one, const char* ask_bin, uint32_t ask_len, const char* reply_bin, uint32_t reply_len);
	void getCSReply();
	void queryUserGameServer();
	
public:
	int addOptUser(const string& userName);
	int addOptUser(const string& userName, const string& data);
	string getOptUser(int optId, bool isRemove = true);
	string getOptUser(int optId, string& data, bool isRemove = true);
	
private:
    void handldMsgUserEnterServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handldMsgUserQuitServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 服务启动上线通知
	void serviceOnlineNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getServiceData(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
private:
    // only for test，测试异步消息本地数据存储
	void testAsyncLocalData();
	void testAsyncLocalDataRequest(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void testAsyncLocalDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	

public:
	CDBOpertaion *m_pDBOpt;
	Config::config  m_config;
	vector<uint32_t> m_vstat_timer_id;
	vector<uint32_t> m_vservice_id;

	CManageUser m_manage_user;

	CMemManager *m_pMManageCSReplyInfo;
	uint32_t m_cur_max_reply_id;
	unordered_map<uint32_t, CCSReplyInfo*> m_umap_cs_reply;
	
	NDBOpt::CRedis m_redisDbOpt;
	unsigned int m_serviceIntervalSecs;
	
private:
	int m_optId;
	OptIdToConnectUser m_opt2ConnectUser;
	
	
	CLocalAsyncData m_onlyForTestAsyncData;  // only for test，测试异步消息本地数据存储

};

#endif // __CSVR_MSG_HANDLER_H__
