
/* author : limingfan
 * date : 2016.12.06
 * description : 象棋服务实现
 */

#ifndef CXIANGQI_SRV_H
#define CXIANGQI_SRV_H

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
#include "db/CRedis.h"
#include "TypeDefine.h"
#include "CRobotManager.h"
#include "CFFQILogicHandler.h"


using namespace std;
using namespace NProject;


namespace NPlatformService
{

// 消息协议处理对象
class CSrvMsgHandler : public NFrame::CLogicHandler
{
public:
	CSrvMsgHandler();
	~CSrvMsgHandler();
	
public:
	unsigned int getCommonDbProxySrvId(const char* userName, const int len);
	int sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userName = NULL, int uNLen = 0);

	int sendMessageToService(NFrame::ServiceType srvType, const char* data, const unsigned int len, unsigned short protocolId, const char* userName, const int uNLen,
							 int userFlag = 0, unsigned short handleProtocolId = 0);
							 
	bool checkLoginIsSuccess(const char* logInfo);

	const char* serializeMsgToBuffer(const ::google::protobuf::Message& msg, const char* info);
	bool parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info);
	
public:
    void updateConfig();  // 配置文件更新
	
	void onClosedConnect(void* userData);   // 通知逻辑层对应的逻辑连接已被关闭
	
public:
	XQUserData* getUserData(const char* userName = NULL);
	
	XQUserData* createUserData();
	void destroyUserData(XQUserData* xqUD);
	
	NFrame::ConnectProxy* getConnectInfo(const char* logInfo, string& userName);
	
	XQChessPieces* getXQChessPieces(const char* username);
	
public:
	// 玩家下棋走子
    void userPlayChessPieces(const XQUserData* xqUD, const com_protocol::ClientPlayChessPiecesNotify& plCPNotify, int& errorCode);
	
	// 是否可以吃掉对方的棋子
	bool canEatOtherChessPieces(int weSide, int otherSide);
	
	// 是否可以移动到目标位置
	bool canMoveToPosition(const int src, const int dst);
	
	// 是否是自己一方的棋子
	bool isSameSide(const unsigned int side, const unsigned int chessPiecesValue);
	
	// 检查玩家走动的炮是否合法
    bool checkMovePaoIsOK(const int src_position, const int dst_position, const unsigned short curOptSide, const short* value, const char* username = NULL);
	
public:
	CLogger& getDataLogger();
	
	NFrame::CLocalAsyncData& getLocalAsyncData();
	
	CRobotManager& getRobotManager();
	
public:
    int sendMsgToProxyEx(const char* msgData, const unsigned int msgLen, unsigned short protocolId, const char* userName = NULL, unsigned int serviceId = 0);
    int sendPkgMsgToClient(const ::google::protobuf::Message& pkg, unsigned short protocol_id, const char* username = NULL, const char* log_info = "");
	int sendPkgMsgToPlayers(const ::google::protobuf::Message& pkg, unsigned short protocol_id, const XQChessPieces* xqCP, const char* log_info = "");
	
	int notifyDbProxyChangeProp(const char* username, unsigned int len, const RecordItemVector& recordItemVector, int flag,
								const char* remark = NULL, const int rate = 0, const char* gameName = "象棋翻翻", const char* recordId = NULL, const char* dst_username = NULL, unsigned short handleProtocolId = 0);
    
    int notifyDbProxyChangeProp(const MoreUserRecordItemVector& recordVector, int flag, const int rate = 0, const char* gameName = "象棋翻翻", const char* recordId = NULL, unsigned short handleProtocolId = 0);
	
	int sendGoodsChangeNotify(const google::protobuf::RepeatedPtrField<com_protocol::PropItem>& items, NFrame::ConnectProxy* conn, const char* userName = NULL);

private:
    // 验证用户的合法性
	void checkUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 用户主动退出游戏
	void userQuitGame(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取翻翻棋场次信息
	void getFFChessArenaInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 用户进入翻翻棋场次
	void userEnterArena(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 用户退出翻翻棋场次
	void userLeaveArena(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 进入游戏，开始匹配对手
	void enterGame(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 玩家准备操作通知
	void readyOptNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 玩家下棋走子
	void playChessPiecesNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	// 玩家主动认输
	// 玩家认输通知服务器，会直接触发按认输的倍率进行结算
	void playerBeDefeateNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 玩家求和
	// 玩家A发起求和请求，玩家A的求和请求会被转发通知给玩家B
	void playerPleasePeace(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 玩家求和对方确认
	// 玩家B收到玩家A的求和请求后可以拒绝或者同意该请求，该消息发送到服务器
	// 最后服务器把求和协商结果发送给玩家A
	void playerPleasePeaceConfirm(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 切换对手
	void switchOpponent(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 再来一局
	void pleasePlayAgain(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 系统消息通知
	void systemMessageNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 获取翻翻棋场次信息
	void getChessArenaInfo(const unsigned short mode, google::protobuf::RepeatedPtrField<com_protocol::ClientChessArenaInfo>* list);
	
	// 检查进场合法性
    int checkArenaCondition(const XQUserData* ud);
	
	// 匹配对手进入游戏
    bool matchingOpponentEnterGame(XQUserData* userData);
	void matchingOpponentEnterGame(const string& waitUser, XQUserData* userData);
	void waitMatchingTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);             // 等待匹配对手玩家超时

    // 发送玩家准备操作结果，通知双方玩家
    void sendReadyOptResultNotify(const XQChessPieces* xqCP, const char* username, const com_protocol::EPlayerReadyOpt optResult);
	
	// 抢红争先操作
    bool robotCompeteFirstRed(XQChessPieces* xqCP, EXQSideFlag side, TimerHandler handler);                        // 机器人抢红
    void confirmCompeteFirstRed(XQChessPieces* xqCP);                                                              // 当前一方确认抢红
    void getCompeteFirstRedTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);       // 红方确认 抢红争先超时
	void getCompeteFirstBlackInquiry(XQChessPieces* xqCP);                                                         // 询问黑方是否抢红争先
	void getCompeteFirstBlackTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);     // 黑方确认 抢红争先超时
	
    bool robotAddDouble(XQChessPieces* xqCP, EXQSideFlag side, TimerHandler handler);                              // 机器人加倍
	void getAddDoubleInquiry(XQChessPieces* xqCP, EXQSideFlag side);                                               // 询问某一方是否加倍加注
	void getAddDoubleTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);             // 等待加倍操作超时
	
	void robotRefusePeace(unsigned int timerId, int userId, void* param, unsigned int remainCount);                // 机器人拒绝求和
	
	// 玩家走棋子通知
	void moveChessPiecesNotify(XQChessPieces* xqCP, EXQSideFlag side);
	void moveChessPiecesTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);          // 等待玩家走动棋子超时
	
	// 重置连击值
	void resetContinueHitValue(XQChessPieces* xqCP, short side);
	
	// 获取当前倍率值
	unsigned int getCurrentRate(const XQChessPieces* xqCP);
	
	// 结束了则结算
	void finishSettlement(XQChessPieces* xqCP, const short lostSide, const com_protocol::SettlementResult result = com_protocol::SettlementResult::NormalFinish, XQUserData* lostUserData = NULL);
	
	// 创建&清除再来一局信息
	unsigned int createPlayAgainInfo(const unsigned int waitSecs, const char* redUser, const char* blackUser);
	void destroyPlayAgainInfo(unsigned int& playAgainId, const bool isKillTimer = true);
	SPlayAgainInfo* getPlayAgainInfo(const unsigned int playAgainId);
	void waitForPlayAgainTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);  // 等待玩家再来一局超时
	
	// 任务统计
	void taskStatistics(const TaskStatisticsOpt tsOpt, XQChessPieces* xqCP, const short value = 0);

private:
	// 收到的CommonDB代理服务应答消息
	void checkUserBaseInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 保存用户逻辑数据应答
	void saveUserLogicDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void changePropReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void changeMoreUserPropReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void rechargeNotifyResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 添加新用户基本信息
	void addUserBaseInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    XQUserData* addUserData(const char* username, NFrame::ConnectProxy* conn);  // 新增加用户数据
	
    void quitGame(const char* username, const char* msg);
	void quitGame(XQUserData* ud, EQuitGameMode mode);

private:
    void logUserInfo(const com_protocol::DetailInfo& userInfo, const char* msg);  // 用户信息写日志
	
	void makeRecordPkg(com_protocol::GameRecordPkg &pkg, const RecordItemVector& items, const string &remark, const int rate, const char* gameName, const char* recordId);
	
	// 保存用户逻辑数据
	void saveUserLogicData(XQUserData* ud);
	
private:
	XQChessPieces* generateXQChessPieces(const char* redUserName, const char* blackUserName);  // 生成棋盘棋子布局

private:
    void onLine(XQUserData* ud);
	void offLine(XQUserData* ud);
	
	void updateRoomInfo(unsigned int timerId, int userId, void* param, unsigned int remainCount);  // 更新服务信息

private:
    virtual void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	virtual const char* canToDoOperation(const int opt, const char* info = "");  // 是否可以执行该操作

private:
	char m_msgBuffer[NFrame::MaxMsgLen];
	NCommon::CMemManager m_memForUserData;
	
	NFrame::CLocalAsyncData m_localAsyncData;
	
	NDBOpt::CRedis m_redisDbOpt;
	NProject::CMonitorStat m_monitorStat;
	
private:
    NCommon::CMemManager m_memForXQChessPieces;
	ChessModeWaitPlayer m_waitForMatchingUsers;         // 理论上只有一个玩家在等待
	ChessModeOnlinePlayerCount m_onlinePlayerCount;     // 各个场次的在线人数
	OnPlayUsers m_onPlayGameUsers;                      // 正在玩游戏的玩家&棋局信息
	
	unsigned int m_playAgainCurrentId;                  // 再来一局全局唯一ID
	SPlayAgainInfoMap m_playAgainInfo;                  // 再来一局玩家信息
	
	CRobotManager m_robotManager;
	CFFQILogicHandler m_logicHandler;
	
	CRechargeMgr m_rechargeMgr;
	
	
DISABLE_COPY_ASSIGN(CSrvMsgHandler);
};


// 象棋服务
class CXiangQiSrv : public NFrame::IService
{
public:
	CXiangQiSrv();
	~CXiangQiSrv();

private:
	virtual int onInit(const char* name, const unsigned int id);            // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);         // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);       // 服务启动后被调用，服务需在此注册本服务的各模块信息
	virtual void onUpdateConfig(const char* name, const unsigned int id);   // 服务配置更新
	
private:
	virtual void onClosedConnect(void* userData);                           // 通知逻辑层对应的逻辑连接已被关闭


private:
    CSrvMsgHandler* m_connectNotifyToHandler;
	
	
DISABLE_COPY_ASSIGN(CXiangQiSrv);
};

}

#endif // CXIANGQI_SRV_H
