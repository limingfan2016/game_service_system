
/* author : limingfan
 * date : 2015.03.09
 * description : Login 服务实现
 */

#ifndef CLOGIN_SRV_H
#define CLOGIN_SRV_H

#include <string>
#include <unordered_map>
#include <map>
#include <vector>

#include "SrvFrame/IService.h"
#include "SrvFrame/CLogicHandler.h"
#include "SrvFrame/CDataContent.h"
#include "base/CMemManager.h"
#include "base/CLogger.h"
#include "common/CommonType.h"
#include "common/CMonitorStat.h"
#include "common/CRechargeMgr.h"
#include "common/CStatisticsData.h"
#include "db/CRedis.h"
#include "TypeDefine.h"
#include "CHallLogic.h"
#include "CActivityCenter.h"
#include "CPkLogic.h"
#include "CLogicHandlerTwo.h"
#include "CRedPacketActivity.h"
#include "CCatchGiftActivity.h"


using namespace std;
using namespace NProject;


namespace NPlatformService
{

typedef unordered_map<int, com_protocol::GetUserBaseinfoRsp> IndexToUserBaseInfo;


// 消息协议处理对象
class CSrvMsgHandler : public NFrame::CLogicHandler
{
public:
	CSrvMsgHandler();
	~CSrvMsgHandler();
	
public:
	unsigned int getCommonDbProxySrvId(const char* userName, const int len);
	int sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userName = NULL, int uNLen = 0);

	int sendMessageToGameDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userName, const int uNLen, unsigned short handleProtocolId = 0, int userFlag = 0);
	int sendMessageToService(NFrame::ServiceType srvType, const char* data, const unsigned int len, unsigned short protocolId, const char* userName, const int uNLen,
							 int userFlag = 0, unsigned short handleProtocolId = 0);
	bool checkLoginIsSuccess(const char* logInfo);
	bool serializeMsgToBuffer(const ::google::protobuf::Message& msg, char* buffer, const unsigned int len, const char* info);
	const char* serializeMsgToBuffer(const ::google::protobuf::Message& msg, const char* info);
	bool parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info);
	
	com_protocol::HallLogicData* getHallLogicData(const char* userName = NULL);
	ConnectUserData* getConnectUserData(const char* userName = NULL);

	void notifyDbProxyChangeProp(const char* username, unsigned int len, const RecordItemVector& recordItemVector, int flag,
	                             const char* remark = NULL, const char* dst_username = NULL, unsigned short handleProtocolId = 0, const char* recordId = NULL);

    // 信息发往邮箱
    void postMessageToMail(const char* username, unsigned int len, const char* title, const char* content, const map<unsigned int, unsigned int>& rewards);

	//统计改变的道具数量
	void statisticsChangePropNum(const char* username, unsigned int len, const RecordItemVector& recordItemVector);

	bool isProxyMsg();
	void initConnect(ConnectProxy* conn);
	
	bool getPKServiceFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData);
    bool getArenaServiceFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData);  // 从redis服务获取比赛场游戏数据
	bool getXiangQiServiceFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData);  // 从redis服务获取象棋服务游戏数据
	bool getPokerCarkServiceFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData);  // 从redis服务获取捕鱼卡牌服务游戏数据
    bool getServicesFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData, const unsigned int srvType, const unsigned int currentFlag, const unsigned int newFlag);
	
	bool getAllGameServerID(const unsigned int srvType, std::vector<int> &vecSrvID);  // 获取所有的游戏服务ID
	
	void addItemCfg(com_protocol::PKPlayCfg* pCfg, int itemID, int itemNum);
	
	//配置文件更新
	void updateConfig();

	ConnectProxy* getConnectInfo(const char* logInfo, string& userName);
	
	void onClosedConnect(void* userData);   // 通知逻辑层对应的逻辑连接已被关闭

public:
    NProject::CRechargeMgr& getRechargeInstance();
	CLogger& getDataLogger();
	CPkLogic& getPkLogic();
	CHallLogic& getHallLogic();
	CActivityCenter& getActivityCenter();
	CLogicHandlerTwo& getLogicHanderTwo();
	CStatisticsDataManager& getStatDataMgr();  // 统计数据
	
private:
    // 收到的客户端请求消息
	void registerUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void login(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void logout(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void modifyPassword(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void modifyUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getHallInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getUserNameByIMEI(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//获取PK场排名信息
	void handleGetPkPlayRankingInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handleGetPkPlayRankingInfoRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//获取PK场详细规则
	void handleGetPkPlayRankingRuleReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handleGetPkPlayRankingRuleRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//获取PK场名人堂信息
	void handleGetPkPlayCelebrityReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handleGetPkPlayCelebrityRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//获取PK场排行榜奖励
	void handleGetPkPlayRewarReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handleGetPkPlayRewarRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//发放PK场排行榜奖励(离线玩家)
	void handleUpdateOffLineUserPKRewarReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//获取离线玩家大厅数据（用于更新该用户的排行奖励）
	void getOffLineUserHallDataUpPKRewar(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//更新用户的上周排名信息
	void updateLastRankingInfo(com_protocol::HallLogicData &hallLogicData, uint32_t ranking, uint32_t score, uint64_t nRankingClearNum);
		
	//发放PK场排行榜奖励(在线玩家)
	void handleUpdateOnLineUserPKRewarReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void getHallLogicDataForReplyPKRankingReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//更新用户排行榜数据
	void updateUserPkPlayRankingData(com_protocol::HallLogicData* hallLogicData, uint64_t nRankingClearNum);

private:
	// 收到的CommonDB代理服务应答消息
	void registerUserReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void modifyPasswordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void modifyUserInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void verifyPasswordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getUserInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getUserGameLogicDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void loginReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void logoutReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getUserNameByIMEIReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 收到的GameDB代理服务应答消息
	void getHallLogicDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void setHallLogicDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    void saveDataToDb(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void notifyLogout(void* cb, int flag);
	
	void writeStatisticsData(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	unsigned int updateServiceData();
	bool getBuyuDataFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData);

	bool isNewVersionUser(const ConnectUserData* ud);
	
	void makeRecordPkg(com_protocol::GameRecordPkg &pkg, const RecordItemVector& items, const string &remark, const char* recordId = NULL);
	
	// 用户充值结果响应
	void rechargeNotifyResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    void onLine(ConnectUserData* ud, ConnectProxy* conn, const com_protocol::GetUserBaseinfoRsp& baseInfo);
	void offLine(ConnectUserData* ud);
	
	// 提醒玩家尽快使用快过期的金币对战门票
    void remindPlayerUsePKGoldTicket(ConnectUserData* ud, const com_protocol::GetUserBaseinfoRsp& baseInfo);
	
private:
    // only for test，测试异步消息本地数据存储
	void testAsyncLocalData();
	void testAsyncLocalDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	
	// 内部消息处理
private:
	void closeRepeatConnect(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void gameUserLogout(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handlePlayGenerateShareIDReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handlePlayShareReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handleGetPKPlayCfgReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    virtual void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	virtual const char* canToDoOperation(const int opt, const char* info = "");  // 是否可以执行该操作

private:
	NDBOpt::CRedis m_redisDbOpt;
	char m_msgBuffer[NFrame::MaxMsgLen];
	char m_hallLogicDataBuff[LogicDataMaxLen];
	NProject::LoginServiceData m_loginSrvData;
	NCommon::CMemManager m_memForUserData;
	
	unsigned int m_getHallDataLastTimeSecs;
	unsigned int m_getHallDataIntervalSecs;
	unsigned int m_gameIntervalSecs;
	std::vector<std::string> m_strHallData;
	
	CHallLogic m_hallLogic;
	CPkLogic	m_pkLogic;
	NProject::CMonitorStat m_monitorStat;
	
	int m_index;
	IndexToUserBaseInfo m_idx2UserBaseInfo;
	
	NProject::CRechargeMgr m_rechargeMgr;
	
	CActivityCenter m_activityCenter;
	
	CLogicHandlerTwo m_logicHandlerTwo;
	
	CStatisticsDataManager m_statDataMgr;
	
	CLocalAsyncData m_onlyForTestAsyncData;  // only for test，测试异步消息本地数据存储
	
	CRedPacketActivity m_redPacketActivity;
	
	CCatchGiftActivity m_catchGiftActivity;
	
	
DISABLE_COPY_ASSIGN(CSrvMsgHandler);
};


// Login 服务
class CLoginSrv : public NFrame::IService
{
public:
	CLoginSrv();
	~CLoginSrv();

private:
	virtual int onInit(const char* name, const unsigned int id);            // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);         // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);       // 服务启动后被调用，服务需在此注册本服务的各模块信息
	virtual void onUpdateConfig(const char* name, const unsigned int id);   // 服务配置更新
	
private:
	virtual void onClosedConnect(void* userData);   // 通知逻辑层对应的逻辑连接已被关闭


private:
    CSrvMsgHandler* m_connectNotifyToHandler;
	
	
DISABLE_COPY_ASSIGN(CLoginSrv);
};

}

#endif // CLOGIN_SRV_H
