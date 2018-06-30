
/* author : limingfan
 * date : 2018.03.29
 * description : 炸金花游戏服务实现
 */

#ifndef _GOLDEN_FRAUD_GAME_SRV_H_
#define _GOLDEN_FRAUD_GAME_SRV_H_


#include "SrvFrame/IService.h"
#include "SrvFrame/FrameType.h"
#include "SrvFrame/CLogicHandler.h"
#include "SrvFrame/CDataContent.h"
#include "db/CRedis.h"
#include "game_object/CGameObject.h"

#include "BaseDefine.h"

using namespace NProject;


namespace NGameService
{

// 消息协议处理对象
class CGoldenFraudMsgHandler : public NPlatformService::CGameMsgHandler
{
public:
	CGoldenFraudMsgHandler();
	~CGoldenFraudMsgHandler();


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

	// 是否可以解散房间
    virtual bool canDisbandGameRoom(const SRoomData& roomData);

    // 玩家是否可以离开房间
    virtual bool canLeaveRoom(GameUserData* cud);
	
	// 设置游戏房间数据，返回是否设置了游戏数据
    virtual bool setGameRoomData(const string& username, const SRoomData& roomData,
						         google::protobuf::RepeatedPtrField<com_protocol::ClientRoomPlayerInfo>& roomPlayers,
                                 com_protocol::ChessHallGameData& gameData);
								 
public:
    const NGoldenFraudConfigData::GoldenFraudConfig* m_pCfg;  // 本服务配置数据，外部可直接访问

private:
	// 结算牌型
    unsigned int getCardType(const HandCard poker);

    bool isSpecialCard(const HandCard poker);               // 特殊牌
    bool isCoupleCard(const HandCard poker);                // 对子
	bool isSequence(const HandCard poker);                  // 顺子
	bool isSameColour(const HandCard poker);                // 同花
	bool isSameColourSequence(const HandCard poker);        // 同花顺
	bool isThreeSameCard(const HandCard poker);             // 豹子
	
	void descendSort(HandCard poker);                       // 降序排序

private:
    // 随机&计算玩家手牌
    bool randomCalculatePlayerCard(SGoldenFraudRoom& roomData);
	
	// 获取操作数据
	int getOptData(const GameUserData* cud, SGoldenFraudRoom*& roomData, SGoldenFraudData*& playerData);
	
	// 下一个操作
	EOptResult doNextOpt(SGoldenFraudRoom& roomData, unsigned int hallId, unsigned int roomId, unsigned int addWaitSecs = 0);

	// 发送玩家操作通知
	void doNextOptAndNotify(SGoldenFraudRoom& roomData, const GameUserData* cud, int opt,
	                        unsigned int bet = 0, const string& compareUsername = "");
	void doNextOptAndNotify(SGoldenFraudRoom& roomData, const string& userName, unsigned int hallId,
	                        unsigned int roomId, int opt, unsigned int bet = 0, const string& compareUsername = "");
	
	// 玩家比牌
	void doCompareCard(SGoldenFraudRoom& roomData, SGoldenFraudData& active, SGoldenFraudData& passive);
	void compareCardOpt(const char* hallId, const char* roomId, SGoldenFraudRoom& roomData,
                        SGoldenFraudData& active, SGoldenFraudData& passive, const char* info = "");
	EOptResult compareCardOpt(SGoldenFraudRoom& roomData, SGoldenFraudData& currentPlayerData,
                              unsigned int hallId, unsigned int roomId, const string& compareUsername,
							  com_protocol::GoldenFraudPlayerOptNotify& optNtf);
	
	// 回合结算
	void roundSettlement(SGoldenFraudRoom& roomData, const string& userName, unsigned int hallId, unsigned int roomId, int opt);
	bool doSettlement(SGoldenFraudRoom& roomData, const string& userName, unsigned int hallId, unsigned int roomId, int opt);
	
	// 一局金币场&房卡场结算
	void goldSettlement(SGoldenFraudRoom& roomData, const string& userName, unsigned int hallId, unsigned int roomId, int opt);
	void cardSettlement(SGoldenFraudRoom& roomData, const string& userName, unsigned int hallId, unsigned int roomId, int opt);
	
	// 准备下一局金币场游戏
	void prepareNextTime(unsigned int hallId, unsigned int roomId, SGoldenFraudRoom& roomData);

private:
    // 等待玩家操作超时
	void prepareTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void playerOptTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
private:
    void giveUp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void viewCard(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void doBet(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void compareCard(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);


DISABLE_COPY_ASSIGN(CGoldenFraudMsgHandler);
};


// 炸金花游戏服务
class CGoldenFraudSrv : public NFrame::IService
{
public:
	CGoldenFraudSrv();
	~CGoldenFraudSrv();

private:
	virtual int onInit(const char* name, const unsigned int id);            // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);         // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);       // 服务启动后被调用，服务需在此注册本服务的各模块信息
	
	virtual void onUpdateConfig(const char* name, const unsigned int id);   // 服务配置更新
	
private:
	virtual void onCloseConnectProxy(void* userData, int cbFlag);           // 通知逻辑层对应的逻辑连接代理已被关闭


DISABLE_COPY_ASSIGN(CGoldenFraudSrv);
};

}

#endif // _GOLDEN_FRAUD_GAME_SRV_H_
