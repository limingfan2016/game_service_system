
/* author : limingfan
 * date : 2018.03.28
 * description : 各游戏基础服务、接口实现
 */

#ifndef _CGAME_OBJECT_H_
#define _CGAME_OBJECT_H_


#include "SrvFrame/IService.h"
#include "SrvFrame/FrameType.h"
#include "SrvFrame/CLogicHandler.h"
#include "SrvFrame/CDataContent.h"
#include "db/CRedis.h"

#include "GameDataDefine.h"
#include "CGameLogicHandler.h"


using namespace NFrame;
using namespace NProject;


namespace NPlatformService
{

// 消息协议处理对象
class CGameMsgHandler : public NFrame::CLogicHandler
{
public:
	CGameMsgHandler();
	~CGameMsgHandler();


public:
    // 具体的游戏服务必须回调这两API
	void onUpdateConfig();                                  // 服务配置文件更新
	void onCloseConnectProxy(void* userData, int cbFlag);   // 通知逻辑层对应的逻辑连接代理已被关闭
	

public:
	// 服务启动&退出
	virtual void onInit();
	virtual void onUnInit();
	
	// 服务配置数据刷新
	virtual void onUpdate();
	
	// 服务标识
	virtual int getSrvFlag();
	
public:	
	// 玩家上线&下线
	virtual void onLine(GameUserData* cud, ConnectProxy* conn, const com_protocol::DetailInfo& userDetailInfo);
	virtual void offLine(GameUserData* cud);

public:
	// 开始游戏
	virtual void onStartGame(const char* hallId, const char* roomId, SRoomData& roomData);
	
	// 心跳失败&成功
	virtual void onHearbeatResult(const string& username, const bool isOnline);
	
	// 玩家离开房间
	virtual EGamePlayerStatus onLeaveRoom(SRoomData& roomData, const char* username);
	
	// 拒绝解散房间，恢复继续游戏
	virtual bool onRefuseDismissRoomContinueGame(const GameUserData* cud, SRoomData& roomData, CHandler*& instance, TimerHandler& timerHandler);
	
public:
    // 创建&销毁房间数据
    virtual SRoomData* createRoomData();
	virtual void destroyRoomData(SRoomData* roomData);
	
public:
	// 每一局房卡AA付费
	virtual float getAveragePayCount();
	
	// 每一局房卡房主付费
	virtual float getCreatorPayCount();

	// 检查房卡数量是否足够
	virtual bool checkRoomCardIsEnough(const SRoomData& roomData, const char* username, const float roomCardCount);
	
	// 是否可以开始游戏
	virtual int canStartGame(const SRoomData& roomData, bool isAuto = true);
	
	// 是否可以解散房间
    virtual bool canDisbandGameRoom(const SRoomData& roomData);

    // 玩家是否可以离开房间
    virtual bool canLeaveRoom(GameUserData* cud);
	
	// 设置游戏房间数据，返回是否设置了游戏数据
    virtual bool setGameRoomData(const string& username, const SRoomData& roomData,
						         google::protobuf::RepeatedPtrField<com_protocol::ClientRoomPlayerInfo>& roomPlayers,
                                 com_protocol::ChessHallGameData& gameData);

public:
	CServiceOperationEx& getSrvOpt();

public:
	// 获取房间数据
	SRoomData* getRoomDataByUser(const char* userName = NULL);
    SRoomData* getRoomDataById(const char* roomId, const char* hallId = NULL);
	SRoomData* getRoomData(unsigned int hallId, unsigned int roomId);
	
	// 设置用户会话数据
    int setUserSessionData(const char* username, unsigned int usernameLen, int status = 0, unsigned int roomId = 0);
	int setUserSessionData(const char* username, unsigned int usernameLen, const GameServiceData& gameSrvData);
	
	// 从房间删除用户
	unsigned int delUserFromRoom(const StringVector& usernames);
	
	// 解散房间，删除房间数据
	bool delRoomData(const char* hallId, const char* roomId, bool isNotifyPlayer = true);
	bool delRoomData(unsigned int hallId, unsigned int roomId, bool isNotifyPlayer = true);

    // 同步发送消息到房间的其他玩家
	int sendPkgMsgToRoomPlayers(const SRoomData& roomData, const ::google::protobuf::Message& pkg, unsigned short protocolId,
	                            const char* logInfo, const string& exceptName = "", int minStatus = -1, int onlyStatus = -1);
	
	// 同步发送消息到房间的在座玩家
    int sendPkgMsgToRoomSitDownPlayers(const SRoomData& roomData, const ::google::protobuf::Message& pkg, unsigned short protocolId,
									   const char* logInfo, const string& exceptName = "");
									   
	// 同步发送消息到房间的游戏中玩家
    int sendPkgMsgToRoomOnPlayPlayers(const SRoomData& roomData, const ::google::protobuf::Message& pkg, unsigned short protocolId,
									  const char* logInfo, const string& exceptName = "");
	
	// 刷新游戏玩家状态通知
	int updatePlayerStatusNotify(const SRoomData& roomData, const string& player, int newStatus, const string& exceptName, const char* logInfo = "");
    int updatePlayerStatusNotify(const SRoomData& roomData, const StringVector& players, int newStatus, const string& exceptName, const char* logInfo = "");
	
	// 刷新准备按钮状态
	int updatePrepareStatusNotify(const SRoomData& roomData, int status, const char* logInfo = "");
	
	// 设置操作超时信息
	void setOptTimeOutInfo(unsigned int hallId, unsigned int roomId, SRoomData& roomData,
	                       unsigned int waitSecs, CHandler* instance, TimerHandler timerHandler, unsigned int optType);
						   
	// 拒绝解散房间，恢复继续游戏
	void refuseDismissRoomContinueGame(const GameUserData* cud, SRoomData& roomData, bool isCancelDismissNotify = false);


public:
	// 设置玩家掉线重来信息
    void setReEnterInfo(const ConstCharPointerVector& usernames, const char* hallId = "", const char* roomId = "", EGamePlayerStatus status = EGamePlayerStatus::EInRoom);
	void setReEnterInfo(const StringVector& usernames, const char* hallId = "", const char* roomId = "", EGamePlayerStatus status = EGamePlayerStatus::EInRoom);

public:
    // 多用户物品结算
    int moreUsersGoodsSettlement(const char* hallId, const char* roomId, SRoomData& roomData,
								 GameSettlementMap& gameSettlementInfo, const char* remark = "game settlement", int roomFlag = 0);
	int moreUsersGoodsSettlement(unsigned int hallId, unsigned int roomId, SRoomData& roomData,
								 GameSettlementMap& gameSettlementInfo, const char* remark = "game settlement", int roomFlag = 0);
								 
	// 单用户金币变更
    int changeUserGold(const char* hallId, const char* roomId, SRoomData& roomData,
					   const string& username, float changeValue, const char* remark = "game settlement", int roomFlag = 0);
	int changeUserGold(unsigned int hallId, unsigned int roomId, SRoomData& roomData,
					   const string& username, float changeValue, const char* remark = "game settlement", int roomFlag = 0);

	// 房卡扣费
    int payRoomCard(const char* hallId, const char* roomId, SRoomData& roomData, int roomFlag = 0);

    // 房卡总结算
    void cardCompleteSettlement(const char* hallId, const char* roomId, SRoomData& roomData, bool isNotify = true, int roomFlag = 0);
	

	// 玩家心跳检测机制（服务级别）
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

private:
    // 增加&删除 待解散的房间
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
    // 触发心跳检测
	void heartbeatCheck(const string& username);
	void onHeartbeatCheck(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	void onHeartbeatCheckFail(const string& username);
	void onHeartbeatCheckSuccess(const string& username);
	
private:
    // 是否是棋牌室审核通过的正式玩家
	bool isHallPlayer(const GameUserData* cud);

	// 玩家坐下，占用桌子位置
    com_protocol::ClientRoomPlayerInfo* getPlayerInfoBySeatId(RoomPlayerIdInfoMap& playerInfo, const int seatId);
	int getFreeSeatId(GameUserData* cud, SRoomData* roomData, int& seatId);
	int userSitDown(GameUserData* cud, SRoomData* roomData);

	// 进入房间session key检查
    int checkSessionKey(const string& username, const string& sessionKey, SessionKeyData& sessionKeyData);
	
	// 删除用户会话数据
    void delUserSessionData(const char* username, unsigned int usernameLen);
	
	// 玩家退出游戏
	void exitGame(void* cb, int quitStatus);

private:
	int addRoomData(const com_protocol::ChessHallRoomInfo& roomInfo);
	
	void addUserToRoom(const GameUserData* cud, SRoomData* roomData, const com_protocol::DetailInfo& userDetailInfo);
	bool delUserFromRoom(const string& username);

    void userEnterRoom(ConnectProxy* conn, const com_protocol::DetailInfo& detailInfo);
	void userReEnterRoom(const string& userName, ConnectProxy* conn, GameUserData* cud, SRoomData& roomData,
                         const com_protocol::ClientRoomPlayerInfo& userInfo);
	EGamePlayerStatus userLeaveRoom(GameUserData* cud, int quitStatus);
	
private:
    void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	bool onPreHandleMessage(const char* msgData, const unsigned int msgLen, const ConnectProxyContext& connProxyContext);
	const char* canToDoOperation(const int opt);  // 是否可以执行该操作

private:
	// 进入房间
	void enter(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getUserInfoForEnterReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void getRoomInfoForEnterReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 坐下位置&点击准备
    // void sitDown(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void prepare(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    
	// 手动开始游戏
    void start(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 离开房间
    void leave(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 更换房间
	void change(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 掉线重连，重入房间
	void reEnter(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:	
	// 心跳应答消息
	void heartbeatMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    void changeMoreUsersGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void setRoomGameRecordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

protected:
	CServiceOperationEx m_srvOpt;  // 服务基本操作

	NDBOpt::CRedis m_redisDbOpt;
	
	CGameLogicHandler m_gameLogicHandler;

protected:
	com_protocol::GameServiceInfo m_serviceInfo;
	
	HallDataMap m_hallGameData;
	
	WaitForDisbandRoomInfoMap m_waitForDisbandRoomInfo;
	
private:
	HeartbeatFailedTimesMap m_heartbeatFailedTimes;
	unsigned int m_serviceHeartbeatTimerId;
	unsigned int m_checkHeartbeatCount;


DISABLE_COPY_ASSIGN(CGameMsgHandler);
};

}

#endif // _CGAME_OBJECT_H_

