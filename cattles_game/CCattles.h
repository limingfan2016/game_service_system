
/* author : limingfan
 * date : 2017.11.15
 * description : 牛牛游戏服务实现
 */

#ifndef _CCATTLES_GAME_SRV_H_
#define _CCATTLES_GAME_SRV_H_


#include "SrvFrame/IService.h"
#include "SrvFrame/FrameType.h"
#include "SrvFrame/CLogicHandler.h"
#include "SrvFrame/CDataContent.h"
#include "db/CRedis.h"

#include "BaseDefine.h"
#include "CGameProcess.h"
#include "CGameLogicHandler.h"


using namespace NFrame;
using namespace NProject;


namespace NPlatformService
{

// 消息协议处理对象
class CCattlesMsgHandler : public NFrame::CLogicHandler
{
public:
	CCattlesMsgHandler();
	~CCattlesMsgHandler();

public:
	// 获取房间数据
	SCattlesRoomData* getRoomDataByUser(const char* userName = NULL);
    SCattlesRoomData* getRoomDataById(const char* roomId, const char* hallId = NULL);
	SCattlesRoomData* getRoomData(unsigned int hallId, unsigned int roomId);
	
	// 设置用户会话数据
    int setUserSessionData(const char* username, unsigned int usernameLen, int status = 0, unsigned int roomId = 0);
	int setUserSessionData(const char* username, unsigned int usernameLen, const GameServiceData& gameSrvData);
	
	// 从房间删除用户
	unsigned int delUserFromRoom(const StringVector& usernames);
	
	// 解散房间，删除房间数据
	bool delRoomData(const char* hallId, const char* roomId, bool isNotifyPlayer = true);
	
	// 获取对应状态的玩家数量
	unsigned int getPlayerCount(const SCattlesRoomData& roomData, const int status);

    // 同步发送消息到房间的其他玩家
	int sendPkgMsgToRoomPlayers(const SCattlesRoomData& roomData, const ::google::protobuf::Message& pkg, unsigned short protocolId,
	                            const char* logInfo, const string& exceptName, int minStatus = -1);
	
	// 刷新游戏玩家状态通知
	int updatePlayerStatusNotify(const SCattlesRoomData& roomData, const string& player, int newStatus, const string& exceptName, const char* logInfo = "");
    int updatePlayerStatusNotify(const SCattlesRoomData& roomData, const StringVector& players, int newStatus, const string& exceptName, const char* logInfo = "");
	
	// 刷新准备按钮状态
	int updatePrepareStatusNotify(const SCattlesRoomData& roomData, com_protocol::ETrueFalseType status, const char* logInfo = "");
	
public:
    bool startHeartbeatCheck(unsigned int checkIntervalSecs, unsigned int checkCount);
	void stopHeartbeatCheck();
	
	void addHearbeatCheck(const ConstCharPointerVector& usernames);
	void addHearbeatCheck(const StringVector& usernames);
	void addHearbeatCheck(const string& username);
	void delHearbeatCheck(const ConstCharPointerVector& usernames);
	void delHearbeatCheck(const StringVector& usernames);
	void delHearbeatCheck(const string& username);
	void clearHearbeatCheck();

public:
	// 房间的最小下注额
    unsigned int getMinBetValue(const SCattlesRoomData& roomData);
	
	// 房间的最大下注额
    unsigned int getMaxBetValue(const SCattlesRoomData& roomData);

	// 闲家需要的最低金币数量
	unsigned int getMinNeedPlayerGold(const SCattlesRoomData& roomData);

	// 庄家需要的最低金币数量
	unsigned int getMinNeedBankerGold(const SCattlesRoomData& roomData);
	
	// 检查是否存在构成牛牛的三张牌
    bool getCattlesCardIndex(const CattlesHandCard poker, unsigned int& idx1, unsigned int& idx2, unsigned int& idx3);

public:
	CServiceOperationEx& getSrvOpt();
	
	CGameProcess& getGameProcess();
	
public:
	void updateConfig();  // 配置文件更新
	
	void onCloseConnectProxy(void* userData, int cbFlag);   // 通知逻辑层对应的逻辑连接代理已被关闭
	
public:
    const NCattlesConfigData::CattlesConfig* m_pCfg;  // 本服务配置数据，外部可直接访问


private:    
	void addWaitForDisbandRoom(const unsigned int hallId, const unsigned int roomId);
	void delWaitForDisbandRoom(const unsigned int hallId, const unsigned int roomId);

    // 解散所有等待的空房间
    void disbandAllWaitForRoom();

	// 检测待解散的空房间，超时时间到则解散房间
	void startCheckDisbandRoom();
	void onCheckDisbandRoom(unsigned int timerId, int userId, void* param, unsigned int remainCount);

private:
    // 定时保存服务信息到redis DB
	void saveDataToRedis(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
private:
	void heartbeatCheck(const string& username);
	virtual void onHeartbeatCheckFail(const string& username);
	virtual void onHeartbeatCheckSuccess(const string& username);
	
	void onHeartbeatCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
private:
    com_protocol::ClientRoomPlayerInfo* getPlayerInfoBySeatId(RoomPlayerIdInfoMap& playerInfo, const int seatId);
	int getFreeSeatId(GameUserData* cud, SCattlesRoomData* roomData, int& seatId);
	int userSitDown(GameUserData* cud, SCattlesRoomData* roomData);
	
	// 检查房卡数量是否足够
	bool checkRoomCardIsEnough(const char* username, const float roomCardCount, const SCattlesRoomData& roomData);
	
	// 是否是棋牌室审核通过的正式玩家
	bool isHallPlayer(const GameUserData* cud);
	
	// 是否可以开始游戏
	int canStartGame(const SCattlesRoomData& roomData, bool isAuto = true);
	
	// 房间最大倍率
	unsigned int getCardMaxRate(const SCattlesRoomData& roomData);
	
	// 设置游戏房间数据，返回是否设置了游戏数据
    bool setGameRoomData(const string& username, const SCattlesRoomData& roomData,
						 google::protobuf::RepeatedPtrField<com_protocol::ClientRoomPlayerInfo>& roomPlayers,
                         com_protocol::ChessHallGameData& gameData);

	// 设置游戏玩家信息
	void setGamePlayerInfo(const SGamePlayerData& playerData, com_protocol::CattlesGamePlayerInfo& gamePlayerInfo);

	// 进入房间session key检查
    int checkSessionKey(const string& username, const string& sessionKey, SessionKeyData& sessionKeyData);
	
	// 删除用户会话数据
    void delUserSessionData(const char* username, unsigned int usernameLen);
	
	// 退出游戏
	void logoutNotify(void* cb, int quitStatus);

private:	
	int addRoomData(const com_protocol::ChessHallRoomInfo& roomInfo);
	
	void addUserToRoom(const char* userName, SCattlesRoomData* roomData, const com_protocol::DetailInfo& userDetailInfo);
	bool delUserFromRoom(const string& username);

    void userEnterRoom(ConnectProxy* conn, const com_protocol::DetailInfo& detailInfo);
	void userReEnterRoom(const string& userName, ConnectProxy* conn, GameUserData* cud, SCattlesRoomData& roomData,
                         const com_protocol::ClientRoomPlayerInfo& userInfo, SGamePlayerData& playerData);
	EGamePlayerStatus userLeaveRoom(GameUserData* cud, int quitStatus);

	void onLine(GameUserData* cud, ConnectProxy* conn, const com_protocol::DetailInfo& userDetailInfo);
	void offLine(GameUserData* cud);
	
private:
    virtual void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	virtual bool onPreHandleMessage(const char* msgData, const unsigned int msgLen, const ConnectProxyContext& connProxyContext);
	
	const char* canToDoOperation(const int opt);  // 是否可以执行该操作

private:
    void giveGoodsNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	// 进入房间
	void enter(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getUserInfoForEnterReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void getRoomInfoForEnterReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 坐下位置&点击准备
    void sitDown(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void prepare(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    
	// 手动开始游戏
    void start(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 离开房间
    void leave(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 更换房间
	void change(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 掉线重连，重入房间
	void reEnter(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 心跳应答消息
	void heartbeatMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	void playerChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	CServiceOperationEx m_srvOpt;  // 服务基本操作

	NDBOpt::CRedis m_redisDbOpt;

	CGameProcess m_gameProcess;
	
	CGameLogicHandler m_gameLogicHandler;

private:
	com_protocol::GameServiceInfo m_serviceInfo;
	
	HallCattlesMap m_hallCattlesData;
	
	WaitForDisbandRoomInfoMap m_waitForDisbandRoomInfo;
	
private:
	HeartbeatFailedTimesMap m_heartbeatFailedTimes;
	unsigned int m_serviceHeartbeatTimerId;
	unsigned int m_checkHeartbeatCount;


DISABLE_COPY_ASSIGN(CCattlesMsgHandler);
};


// 牛牛游戏服务
class CCattlesSrv : public NFrame::IService
{
public:
	CCattlesSrv();
	~CCattlesSrv();

private:
	virtual int onInit(const char* name, const unsigned int id);            // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);         // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);       // 服务启动后被调用，服务需在此注册本服务的各模块信息
	
	virtual void onUpdateConfig(const char* name, const unsigned int id);   // 服务配置更新
	
private:
	virtual void onCloseConnectProxy(void* userData, int cbFlag);           // 通知逻辑层对应的逻辑连接代理已被关闭


DISABLE_COPY_ASSIGN(CCattlesSrv);
};

}

#endif // _CCATTLES_GAME_SRV_H_
