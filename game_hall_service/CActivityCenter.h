
/* author : limingfan
 * date : 2016.04.20
 * description : 活动中心
 */

#ifndef CACTIVITY_CENTER_H
#define CACTIVITY_CENTER_H

#include "base/MacroDefine.h"
#include "SrvFrame/CModule.h"
#include "TypeDefine.h"
#include "_HallConfigData_.h"


using namespace NFrame;

namespace NPlatformService
{

class CSrvMsgHandler;


// 新手礼包活动数据
struct NewbieGiftActivityInfo
{
	unsigned int startTimeSecs;
	unsigned int finishTimeSecs;
	
	// 统计数据信息
	int curRegisterUserDay;
	int curReceiveGiftDay;
	unsigned int registerUsers;            // 每天新增加的用户量
	unsigned int receiveGifts;             // 每天领取的新手礼包
};

// 充值抽奖活动数据
struct RechargeAwardActivityInfo
{
	unsigned int startTimeSecs;
	unsigned int finishTimeSecs;
	
	unsigned int lastLotteryTimeSecs;      // 抽奖结束时间点
	unsigned int phoneFare;                // 已经送出去的话费额度
	
	// 统计数据信息
	int curActivityUserDay;                // 参加活动
	unsigned int activityUsers;            // 每天参加活动的用户量
};


// 活动中心逻辑处理
class CActivityCenter : public NFrame::CHandler
{
public:
	CActivityCenter();
	~CActivityCenter();
	
public:
	void load(CSrvMsgHandler* msgHandler);
	void unLoad(CSrvMsgHandler* msgHandler);
	
	void onLine(ConnectUserData* ud, ConnectProxy* conn);
	void offLine(com_protocol::HallLogicData* hallLogicData);
	
	void updateConfig();
	
public:
	void userRechargeNotify(const char* userName);
	
	void receivePKTaskRewardReply(const unsigned int flag);  // 领取了PK任务奖励
	void addPKJoinMatchTimes(const char* userName);          // 新增PK任务参赛次数
    void addPKWinMatchTimes(const char* userName);           // 新增PK任务赢局次数
	
private:
    // 活动中心列表
    void getActivityList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getUserRegisterTimeReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void receiveActivityRewardReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void receiveNewbieGift(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void rechargeLotteryWinGift(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    void receiveNewbieReward(ConnectProxy* conn);
	void receiveRechargeLotteryGift(const string& userName, ConnectProxy* conn, const vector<HallConfigData::RechargeReward>& actvityRewardCfg, const unsigned int rewardIdx);

private:
	ConnectProxy* getConnectInfo(const char* logInfo, string& userName);
	
	void getActivityData(unsigned int activityType, com_protocol::HallLogicData* hallData, const char* userName = NULL);
	int addNewbieActivity(com_protocol::HallLogicData* hallData, unsigned int curSecs, unsigned int activityType, com_protocol::GetActivityCenterRsp& rsp);
	void addRechargeLotteryActivity(com_protocol::HallLogicData* hallData, unsigned int curSecs, com_protocol::GetActivityCenterRsp& rsp);
	
	
private:
    // PK场任务活动
    void getPKTaskInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void receivePKTaskReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	void resetPKTask(ConnectUserData* ud, unsigned int theDay);
	int getPKOnlineTaskIndex(const unsigned int hour, const unsigned int min, int& nextIdx);

    void getPKJoinTaskInfo(const com_protocol::LoginPKJoinMatch& join_match, com_protocol::ClientPKMatchTask* matchTask);  // 对局任务信息
    void getPKWinTaskInfo(const com_protocol::LoginPKWinMatch& win_match, com_protocol::ClientPKMatchTask* matchTask);  // 赢局任务信息
	void getPKOnlineTaskInfo(const com_protocol::LoginPKOnlineTask& onlineTask, com_protocol::ClientPKOnlineTask* pkOnlineTask);  // PK在线任务信息
	
private:
	CSrvMsgHandler* m_msgHandler;
	
	NewbieGiftActivityInfo m_newbieGift;
	RechargeAwardActivityInfo m_rechargeAward;


DISABLE_COPY_ASSIGN(CActivityCenter);
};

}

#endif // CACTIVITY_CENTER_H
