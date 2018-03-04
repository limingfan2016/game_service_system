
/* author : limingfan
 * date : 2017.09.21
 * description : 游戏大厅服务实现
 */

#ifndef _CGAME_HALL_SRV_H_
#define _CGAME_HALL_SRV_H_

#include "SrvFrame/IService.h"
#include "SrvFrame/FrameType.h"
#include "SrvFrame/CLogicHandler.h"
#include "SrvFrame/CDataContent.h"
#include "common/CClientVersionManager.h"
#include "db/CRedis.h"

#include "TypeDefine.h"
#include "CHallLogicHandler.h"


using namespace NFrame;
using namespace NProject;


namespace NPlatformService
{

// 客户端版本管理
class CVersionManager : public CClientVersionManager
{
private:
	virtual int getVersionInfo(const unsigned int deviceType, const unsigned int platformType, const string& curVersion, unsigned int& flag, string& newVersion, string& newFileURL);
};


// 消息协议处理对象
class CSrvMsgHandler : public NFrame::CLogicHandler
{
public:
	CSrvMsgHandler();
	~CSrvMsgHandler();

public:
	// 获取玩家的聊天限制信息
	SUserChatInfo& getUserChatInfo(const string& username);
	
	// 玩家申请加入棋牌室，通知在线管理员
	void sendApplyJoinHallNotify(const HallUserData* cud, const google::protobuf::RepeatedPtrField<string>& managers,
								 const com_protocol::PlayerApplyJoinChessHallNotify& notify);

	// 是否是网关代理的消息
	bool isProxyMsg();
	
public:
	// 获取各服务数据
	bool getServicesFromRedis(const HallUserData* ud, com_protocol::GameServiceInfoList& gameSrvList, const unsigned int srvType);
	
	// 设置玩家的会话数据
	int setSessionKeyData(const ConnectProxy* conn, const HallUserData* cud, const char* hallId, int status,
	                      SessionKeyData& sessionKeyData, const string& saveUserName = "");
	
	// 发送消息给棋牌室所有在线用户
	void sendMsgDataToAllHallUser(const char* data, const unsigned int len, const char* hallId, unsigned short protocolId);

public:
	CServiceOperationEx& getSrvOpt();
	
public:
	void updateConfig();  // 配置文件更新
	
	void onCloseConnectProxy(void* userData, int cbFlag);  // 通知逻辑层对应的逻辑连接代理已被关闭
	
public:
    const HallConfigData::HallConfig* m_pCfg;  // 配置数据，外部可直接访问
	
private:
    // 定时保存服务信息到redis DB
	void saveDataToRedis(unsigned int timerId, int userId, void* param, unsigned int remainCount);

    // 刷新游戏数据，返回当前时间点
	unsigned int updateServiceData();

    // 是否是新版本用户
	bool isNewVersionUser(const HallUserData* ud);
	
	// 设置棋牌室信息
	void setChessHallInfo(const HallUserData* cud, com_protocol::ClientHallInfo& hallInfo);
	
	// 设置棋牌室游戏信息
    bool setHallGameInfo(const HallUserData* cud, com_protocol::GameServiceInfoList& gameSrvList,
                         unsigned int idIndex, com_protocol::ClientHallGameInfo& gameInfo);

private:
	void userLogin(ConnectProxy* conn, com_protocol::ClientLoginRsp& loginRsp);
	void userLogout(void* cb, int flag);

    int userEnterHall(ConnectProxy* conn, HallUserData* cud, const char* hallId, int playerStatus,
					  com_protocol::ClientHallInfo* hallInfo, const com_protocol::DetailInfo& userDetailInfo);
	int userLeaveHall(HallUserData* cud);

	void onLine(HallUserData* cud, ConnectProxy* conn, const com_protocol::DetailInfo& userDetailInfo);
	void offLine(HallUserData* cud);
	
private:
    virtual void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	virtual bool onPreHandleMessage(const char* msgData, const unsigned int msgLen, const ConnectProxyContext& connProxyContext);
	
	virtual const char* canToDoOperation(const int opt);  // 是否可以执行该操作

private:
	void giveGoodsNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void playerConfirmGiveGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	void login(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void loginReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void wxLoginReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void logout(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    void enterHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void enterHallReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void leaveHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    void getHallBaseInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getHallBaseInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getHallInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getHallInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    void getHallRoomList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getHallRoomListReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
    void getHallRanking(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getHallRankingReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void getUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getUserInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 刷新服务数据通知
	void updateServiceDataNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	CServiceOperationEx m_srvOpt;  // 服务基本操作
	
	NDBOpt::CRedis m_redisDbOpt;
	
	NProject::GameHallServiceData m_hallSrvData;

	CHallLogicHandler m_hallLogicHandler;

private:
	std::vector<std::string> m_strGameData;         // 游戏服务数据
	unsigned int m_getGameDataLastTimeSecs;         // 更新游戏服务数据时间点
	unsigned int m_getGameDataIntervalSecs;         // 更新游戏服务数据时间间隔
	unsigned int m_gameServiceValidIntervalSecs;    // 游戏服务数据有效时间间隔
	
private:
	UserChatInfoMap m_chatInfo;                     // 棋牌室玩家的聊天限制信息
	
	
DISABLE_COPY_ASSIGN(CSrvMsgHandler);
};


// 游戏大厅服务
class CGameHallSrv : public NFrame::IService
{
public:
	CGameHallSrv();
	~CGameHallSrv();

private:
	virtual int onInit(const char* name, const unsigned int id);            // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);         // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);       // 服务启动后被调用，服务需在此注册本服务的各模块信息
	
	virtual void onUpdateConfig(const char* name, const unsigned int id);   // 服务配置更新
	
private:
	virtual void onCloseConnectProxy(void* userData, int cbFlag);                       // 通知逻辑层对应的逻辑连接代理已被关闭


DISABLE_COPY_ASSIGN(CGameHallSrv);
};

}

#endif // _CGAME_HALL_SRV_H_
