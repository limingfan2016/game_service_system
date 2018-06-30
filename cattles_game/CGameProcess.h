
/* author : limingfan
 * date : 2017.11.21
 * description : 牛牛游戏过程
 */
 
#ifndef __CGAME_PROCESS_H__
#define __CGAME_PROCESS_H__

#include "SrvFrame/CModule.h"
#include "BaseDefine.h"


using namespace NCommon;
using namespace NFrame;
using namespace NDBOpt;
using namespace NErrorCode;

namespace NPlatformService
{

class CCattlesMsgHandler;

class CGameProcess : public NFrame::CHandler
{
public:
	CGameProcess();
	~CGameProcess();

public:
	int init(CCattlesMsgHandler* pMsgHandler);
	void unInit();
    void updateConfig();
	
public:
	// 开始游戏
	void startGame(const char* hallId, const char* roomId, SCattlesRoomData& roomData);
	
	// 玩家离开房间
	EGamePlayerStatus playerLeaveRoom(GameUserData* cud, SCattlesRoomData& roomData, int quitStatus);
	
	// 是否可以解散房间
	bool canDisbandGameRoom(const SCattlesRoomData& roomData);
	
	// 是否存在下一局新庄家
	bool hasNextNewBanker(SCattlesRoomData& roomData);
	
	// 玩家是否可以离开房间
    bool canLeaveRoom(GameUserData* cud);
	
	// 心跳检测结果
	void onHearbeatResult(const string& username, const bool isOnline);
	
	// 房卡总结算
    void cardCompleteSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData, bool isNotify = true);
	
	// 拒绝解散房间，恢复继续游戏
	void refuseDismissRoomContinueGame(const GameUserData* cud, SCattlesRoomData& roomData, bool isCancelDismissNotify = false);

private:
	// 设置操作超时信息
	void setOptTimeOutInfo(unsigned int hallId, unsigned int roomId, SCattlesRoomData& roomData, unsigned int waitSecs, TimerHandler timerHandler, unsigned int optType);

    // 随机&计算玩家手牌
    bool randomCalculatePlayerCard(SCattlesRoomData& roomData);
	
	// 玩家抢庄
	bool competeBanker(unsigned int hallId, unsigned int roomId, SCattlesRoomData& roomData);

	// 确定庄家
	bool confirmBanker(unsigned int hallId, unsigned int roomId, SCattlesRoomData& roomData);
	
	// 通知玩家开始下注
    void startChoiceBet(unsigned int hallId, unsigned int roomId, SCattlesRoomData& roomData);
	
	// 发牌给玩家
	void pushCardToPlayer(unsigned int hallId, unsigned int roomId, SCattlesRoomData& roomData);
	
	// 一局结算
    bool doSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData, com_protocol::CattlesSettlementNotify& settlementNtf);
    void finishSettlement(SCattlesRoomData& roomData); // 完成结算
	void roundSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData);

    // 金币场结算
    bool doGoldSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData);
	void goldSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData);
	
	// 房卡场结算
    bool doCardSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData);
	void cardSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData);
	
	// 发送赢家公告
    void sendWinnerNotice(const char* hallId, const string& username, const StringVector& matchingWords);

	// 准备下一局游戏
	void prepareNextTime(const char* hallId, const char* roomId, SCattlesRoomData& roomData);
	
	// 玩家离开房间
    EGamePlayerStatus playerLeaveRoom(const string& username, SCattlesRoomData& roomData);
    
    // 玩家抢庄是否已经完毕
	bool isFinishChoiceBanker(SCattlesRoomData& roomData);
	
	// 对应的状态是否已经完毕
	bool isFinishStatus(SCattlesRoomData& roomData, const int status);

	// 对应状态的玩家个数
	unsigned int getStatusCount(SCattlesRoomData& roomData, const int status);
	
	// 设置亮牌通知消息
    void setOpenCardNotifyMsg(SCattlesRoomData& roomData, const string& username,
	                          com_protocol::CattlesPlayerOpenCardNotify& openCardNtf, bool isNeedCard = true);

	// 发送亮牌消息，通知其他玩家
	void sendOpenCardNotify(SCattlesRoomData& roomData, const string& username, const char* logInfo,
	                        const string& exceptName, bool isNeedCard = true);
							
private:
    bool getCattlesFixedNextNewBanker(SCattlesRoomData& roomData, string& banker);          // 获取牛牛模式或者固定模式的下一局庄家
    bool setFreeOpenCardNextNewBanker(SCattlesRoomData& roomData);                          // 设置自由模式或者明牌模式的下一局庄家
	
    bool cattlesBanker(SCattlesRoomData& roomData, StringVector* bankers = NULL);           // 牛牛上庄
	bool fixedBanker(SCattlesRoomData& roomData, StringVector* bankers = NULL);             // 固定庄家
	bool freeBanker(SCattlesRoomData& roomData, StringVector* bankers = NULL);              // 自由抢庄
	bool openCardBanker(SCattlesRoomData& roomData, StringVector* bankers = NULL);          // 明牌抢庄
	
private:
	// 结算牌型
    unsigned int getCattlesCardType(const CattlesHandCard poker,
	                                const ::google::protobuf::RepeatedField<int>& selectSpecialType,
									::google::protobuf::RepeatedField<::google::protobuf::uint32>* cattlesCards = NULL);
    // 特殊牌型
    unsigned int getSpecialCardType(const CattlesHandCard poker, const ::google::protobuf::RepeatedField<int>& selectSpecialType);
	bool isSequenceCattle(const CattlesHandCard poker);     // 顺子牛
	bool isFiveColourCattle(const CattlesHandCard poker);   // 五花牛
	bool isSameColourCattle(const CattlesHandCard poker);   // 同花牛
	bool isGourdCattle(const CattlesHandCard poker);        // 葫芦牛
	bool isBombCattle(const CattlesHandCard poker);         // 炸弹牛
	bool isFiveSmallCattle(const CattlesHandCard poker);    // 五小牛
	
	void descendSort(CattlesHandCard poker);                // 降序排序
	
	// 返回最大值的牌
	unsigned char getMaxValueCard(const CattlesHandCard poker);
	
	// 比较双方的牌，返回是否是 pd1 赢
	bool getSettlementCardType(const SGamePlayerData& pd1, const SGamePlayerData& pd2);
	
	// 多用户物品结算
	int moreUsersGoodsSettlement(const char* hallId, const char* roomId, SCattlesRoomData& roomData,
								 google::protobuf::RepeatedPtrField<com_protocol::CattlesSettlementInfo>& clsStInfo);
								 
	// 房卡扣费
	int payRoomCard(const char* hallId, const char* roomId, SCattlesRoomData& roomData);

private:
    // 等待玩家操作超时
	void prepareTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void competeBankerTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void confirmBankerTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void choiceBetTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	void openCardTimeOut(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	void doStartGame(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
private:
    void playerChoiceBanker(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void performFinish(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void playerChoiceBet(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void playerOpenCard(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	void changeMoreUsersGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void setRoomGameRecordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	CCattlesMsgHandler* m_msgHandler;
};

}


#endif // __CGAME_PROCESS_H__
