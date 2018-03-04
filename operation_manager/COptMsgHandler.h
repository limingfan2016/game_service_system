
/* author : limingfan
 * date : 2017.09.15
 * description : 消息处理类
 */
 
#ifndef __COPT_MSG_HANDLER_H__
#define __COPT_MSG_HANDLER_H__

#include "SrvFrame/CLogicHandler.h"
#include "SrvFrame/CDataContent.h"
#include "db/CMySql.h"
#include "db/CRedis.h"
#include "base/CCfg.h"
#include "base/CMemManager.h"
#include "../http_base/CHttpSrv.h"
#include "../common/CClientVersionManager.h"
#include "../common/CIP2City.h"
#include "../common/CRandomNumber.h"

#include "BaseDefine.h"
#include "CHttpMessageHandler.h"
#include "COptLogicHandler.h"


using namespace NCommon;
using namespace NFrame;
using namespace NDBOpt;
using namespace NHttp;


// 客户端版本管理
class CBSAppVersionManager : public CClientVersionManager
{
private:
	virtual int getVersionInfo(const unsigned int deviceType, const unsigned int platformType, const string& curVersion, unsigned int& flag, string& newVersion, string& newFileURL);
};


class COptMsgHandler : public NFrame::CLogicHandler
{
public:
	COptMsgHandler();
	~COptMsgHandler();

public:
    CHttpMessageHandler& getHttpMsgHandler();

	CServiceOperationEx& getSrvOpt();
	
	CDBOpertaion* getOptDBOpt();
	
public:	
	// 获取棋牌室不重复的随机ID
	unsigned int getChessHallNoRepeatRandID();
	
	// 获取棋牌室数据
	SChessHallData* getChessHallData(const char* hall_id, int& errorCode, bool isNeedUpdate = false);
	
	// 获取棋牌室游戏信息
    bool getHallGameInfo(const char* hall_id, GameTypeToInfoMap& hallGameInfo);

public:	
	// 从DB中查找用户信息
    int queryPlayerInfo(const char* sql, const unsigned int sqlLen, StringKeyValueMap& keyValue);
	
	// 查找棋牌室用户信息
    int getChessHallPlayerInfo(const char* hallId, const char* username, StringKeyValueMap& keyValue);

	// 从逻辑DB查找用户静态信息
	int getPlayerStaticInfo(const char* username, StringKeyValueMap& keyValue);
	
	// 获取玩家在线的服务ID
	bool getOnlineServiceId(const char* hallId, const char* username, unsigned int& hallSrvId, unsigned int& gameSrvId);

public:
	// 回滚&提交数据
	bool dbRollback();
	bool dbCommit();
	
public:
	void loadConfig(bool isReset);  // 配置更新
	
	void onCloseConnectProxy(void* userData, int cbFlag);   // 通知逻辑层对应的逻辑连接代理已被关闭

private:
	void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);

	bool onPreHandleMessage(const char* msgData, const unsigned int msgLen, const ConnectProxyContext& connProxyContext);

private:
	bool initChessHallIdRandomNumber();

	// 检查客户端版本号是否可用
	int checkClientVersion(unsigned int deviceType, unsigned int platformType, const string& curVersion, unsigned char& versionFlag);
	
	// 登录成功
    void loginSuccess(unsigned char platformType, ConnectProxy* conn, OptMgrUserData* cud, const com_protocol::BSLoginInfo& info);
	
	// 通知连接logout操作
    void logoutNotify(void* cb, int flag);
    
    // 执行任务
    void doTask();
	
private:
	// 新注册管理员
	int newRegisterManager(const char* peerIp, com_protocol::RegisterManagerReq& req);

	// 生成注册用户sql语句
	int getRegisterSql(const char* creator, unsigned int registerTimeSecs, unsigned int userId, const char* ip, unsigned int level,
                       const com_protocol::RegisterManagerReq& req, string& sql);
	
	// 检查平台信息是否已经注册过了
	int checkRegisterPlatformInfo(unsigned int platformType, const string& platformId);
					   
	// 管理员登录
	int login(ConnectProxy* conn, OptMgrUserData* curCud, unsigned int platformType, const string& platformId,
              string& name, string& password, com_protocol::BSLoginInfo& info, bool& needReleaseChessHallInfo);

private:
	// 获取棋牌室信息
	int getChessHallInfo(const char* hall_id, com_protocol::BSChessHallInfo& chessHallInfo);
	
	// 获取棋牌室基本信息
    int getChessHallBaseInfo(const char* hall_id, com_protocol::BSChessHallInfo& chessHallInfo);
	
	// 获取对应时间点对应操作的总数额
    double getTimeOptSumValue(const char* hall_id, com_protocol::EHallPlayerOpt opt, time_t timeSecs = 0, unsigned int gameType = 0);
	
	// 获取对应状态的玩家人数
	unsigned int getPlayerCount(const char* hall_id, com_protocol::EHallPlayerStatus status, unsigned int gameType = 0);
	
	// 刷新在线&离线玩家人数
	void updatePlayerCount(unsigned int serviceId, const char* hall_id, com_protocol::EHallPlayerStatus status, unsigned int count = 1);
								 
	// 获取管理员所在的棋牌室信息
	bool getManagerChessHallInfo(const char* manager, com_protocol::EManagerLevel level,
	                             google::protobuf::RepeatedPtrField<com_protocol::BSChessHallInfo>& chessHallInfo);

private:
    void recordUserEnterService(const com_protocol::UserEnterGameNotify& nf, int userType);
	void recordUserLeaveService(const com_protocol::UserLeaveGameNotify& nf, int userType);
	
private:
    void msgRecordToDB(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	void updateServiceData(unsigned int timerId, int userId, void* param, unsigned int remainCount);
    
    // 执行定时任务
    void onTaskTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
private:
    void registerManager(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void managerLogin(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void managerWxLoginReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void managerLogout(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    void userEnterServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void userLeaveServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 服务状态通知
	void serviceStatusNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 消息记录
	void msgRecordNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

public:
    const NOptMgrConfig::config* m_pCfg;                        // 公共配置数据，外部可直接访问
	
private:
	CServiceOperationEx m_srvOpt;
	
	// CBSAppVersionManager m_bsAppVerManager;
	
	CHttpMessageHandler m_httpMsgHandler;
	
	CDBOpertaion* m_pDBOpt;
	
	NDBOpt::CRedis m_redisDbOpt;
	
	CRandomNumber m_chessHallIdRandomNumbers;  // 棋牌室ID随机数
	
	CIP2City m_ip2city;
	
	COptLogicHandler m_logicHandler;
	
private:
	uint32_t m_msgRecordToDbTimer;
	string m_msgRecord;             // 用户消息记录信息
	
	uint32_t m_serviceValidIntervalSecs;
	
	ChessHallIdToDataMap m_hallId2Data;
};


// 可收发http消息
class COperationManager : public CHttpSrv
{
public:
	COperationManager();
	~COperationManager();
	
private:
	virtual void onRegister(const char* name, const unsigned int id);       // 服务启动后被调用，服务需在此注册本服务的各模块信息
	
	virtual void onCloseConnectProxy(void* userData, int cbFlag);           // 通知逻辑层对应的逻辑连接代理已被关闭
};


#endif // __COPT_MSG_HANDLER_H__
