
/* author : limingfan
 * date : 2016.04.20
 * description : 活动中心
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <string>

#include "CActivityCenter.h"
#include "CLoginSrv.h"
#include "base/ErrorCode.h"
#include "common/CommonType.h"


using namespace std;
using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace HallConfigData;
using namespace NProject;


namespace NPlatformService
{

// 活动中心统计日志
#define ActivityCenterStatisticsLog(format, args...)     // m_msgHandler->getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)


struct IdxToLotteryReward
{
	unsigned int idx;
	const HallConfigData::RechargeReward* reward;
	
	IdxToLotteryReward() {};
	IdxToLotteryReward(unsigned int _idx, const HallConfigData::RechargeReward* _reward) : idx(_idx), reward(_reward) {};
};
typedef vector<IdxToLotteryReward> IdxToLotteryRewardVector;
		
		
CActivityCenter::CActivityCenter()
{
	m_msgHandler = NULL;
	
	memset(&m_newbieGift, 0, sizeof(m_newbieGift));
	memset(&m_rechargeAward, 0, sizeof(m_rechargeAward));
}

CActivityCenter::~CActivityCenter()
{
	m_msgHandler = NULL;
	
	memset(&m_newbieGift, 0, sizeof(m_newbieGift));
	memset(&m_rechargeAward, 0, sizeof(m_rechargeAward));
}


void CActivityCenter::onLine(ConnectUserData* ud, ConnectProxy* conn)
{
	com_protocol::HallLogicData* hallLogicData = ud->hallLogicData;
	if (!hallLogicData->has_new_user_gift_activity())
	{
		hallLogicData->mutable_new_user_gift_activity()->set_status(NewbieGiftStatus::InitNewbie);
	}
	
	if (!hallLogicData->has_recharge_reward_activity())
	{
		hallLogicData->mutable_recharge_reward_activity()->set_lottery_count(0);
	}
	
	time_t curSecs = time(NULL);
	unsigned int theDay = localtime(&curSecs)->tm_yday;
	resetPKTask(ud, theDay);
}

void CActivityCenter::offLine(com_protocol::HallLogicData* hallLogicData)
{
}

void CActivityCenter::updateConfig()
{
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	
	// 更新活动时间
	struct ActivityConfig
	{
		const char* strTime;
		unsigned int* intTime;
	};
	
	ActivityConfig actCfg[] = {{cfg.newbie_gift_activity.start_time.c_str(), &m_newbieGift.startTimeSecs}, {cfg.newbie_gift_activity.finish_time.c_str(), &m_newbieGift.finishTimeSecs},
	{cfg.recharge_lottery_activity.start_time.c_str(), &m_rechargeAward.startTimeSecs}, {cfg.recharge_lottery_activity.finish_time.c_str(), &m_rechargeAward.finishTimeSecs},
	{cfg.recharge_lottery_activity.lottery_finish_time.c_str(), &m_rechargeAward.lastLotteryTimeSecs},};
	
	struct tm activityTime;
	for (unsigned int idx = 0; idx < (sizeof(actCfg) / sizeof(actCfg[0])); ++idx)
	{
		*(actCfg[idx].intTime) = -1;
		if (strptime(actCfg[idx].strTime, "%Y-%m-%d %H:%M:%S", &activityTime) != NULL) *(actCfg[idx].intTime) = mktime(&activityTime);
		if (*(actCfg[idx].intTime) == (unsigned int)-1) *(actCfg[idx].intTime) = time(NULL);
	}

	m_newbieGift.curRegisterUserDay = getCurrentYearDay();
	m_newbieGift.curReceiveGiftDay = m_newbieGift.curRegisterUserDay;
	m_rechargeAward.curActivityUserDay = m_newbieGift.curRegisterUserDay;
	
	OptInfoLog("newbie activity time, start = [%s, %u], finish = [%s, %u], recharge lottery activity time, start = [%s, %u], finish = [%s, %u], lottery = [%s, %u], year day = %d",
	cfg.newbie_gift_activity.start_time.c_str(), m_newbieGift.startTimeSecs, cfg.newbie_gift_activity.finish_time.c_str(), m_newbieGift.finishTimeSecs,
	cfg.recharge_lottery_activity.start_time.c_str(), m_rechargeAward.startTimeSecs, cfg.recharge_lottery_activity.finish_time.c_str(), m_rechargeAward.finishTimeSecs,
	cfg.recharge_lottery_activity.lottery_finish_time.c_str(), m_rechargeAward.lastLotteryTimeSecs, m_rechargeAward.curActivityUserDay);
}

void CActivityCenter::load(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;

	// 活动中心列表
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_ACTIVITY_CENTER_REQ, (ProtocolHandler)&CActivityCenter::getActivityList, this);
	m_msgHandler->registerProtocol(LoginSrv, CUSTOM_GET_USER_REGISTER_TIME_FOR_NEWBIE_ACTIVITY, (ProtocolHandler)&CActivityCenter::getUserRegisterTimeReply, this);

	m_msgHandler->registerProtocol(LoginSrv, CUSTOM_RECEIVE_ACTIVITY_REWARD, (ProtocolHandler)&CActivityCenter::receiveActivityRewardReply, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_RECEIVE_NEWBIE_GIFT_REQ, (ProtocolHandler)&CActivityCenter::receiveNewbieGift, this);
    m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_RECHARGE_WIN_GIFT_REQ, (ProtocolHandler)&CActivityCenter::rechargeLotteryWinGift, this);
	
	// PK场任务活动
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_PK_TASK_INFO_REQ, (ProtocolHandler)&CActivityCenter::getPKTaskInfo, this);
    m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_RECEIVE_PK_TASK_REWARD_REQ, (ProtocolHandler)&CActivityCenter::receivePKTaskReward, this);
	
	updateConfig();
}

void CActivityCenter::unLoad(CSrvMsgHandler* msgHandler)
{
	
}

ConnectProxy* CActivityCenter::getConnectInfo(const char* logInfo, string& userName)
{
	userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL) OptErrorLog("%s, user data = %s", logInfo, m_msgHandler->getContext().userData);
    return conn;
}


// 活动中心列表
void CActivityCenter::getActivityList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("get activity list request")) return;
	
	com_protocol::GetActivityCenterReq getActivityReq;
	if (!m_msgHandler->parseMsgFromBuffer(getActivityReq, data, len, "get activity center data request")) return;
	
	com_protocol::HallLogicData* hallData = m_msgHandler->getHallLogicData();
	if (hallData == NULL)
	{
		OptErrorLog("get user hall logic data for activity center data error");
		return;
	}
	
	getActivityData(getActivityReq.activity_type(), hallData);
}

void CActivityCenter::getUserRegisterTimeReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUserBaseinfoRsp registerTimeRsp;
	if (!m_msgHandler->parseMsgFromBuffer(registerTimeRsp, data, len, "get user register time reply")) return;
	
	if (registerTimeRsp.result() != Opt_Success)
	{
		OptErrorLog("get user register time error, user = %s, result = %d", registerTimeRsp.query_username().c_str(), registerTimeRsp.result());
		return;
	}
	
	com_protocol::HallLogicData* hallData = m_msgHandler->getHallLogicData(registerTimeRsp.query_username().c_str());
	if (hallData == NULL)
	{
		OptErrorLog("get user hall logic data to set newbie activity status error, user = %s", registerTimeRsp.query_username().c_str());
		return;
	}
	
	// 用户注册时间
	struct tm registerTm;
	unsigned int registerSeces = 0;
	if (strptime(registerTimeRsp.detail_info().static_info().register_time().c_str(), "%Y-%m-%d %H:%M:%S", &registerTm) != NULL) registerSeces = mktime(&registerTm);
	if (registerSeces >= m_newbieGift.startTimeSecs && registerSeces < m_newbieGift.finishTimeSecs)  // 在新手礼包活动时间范围内注册的用户才能获得新手礼包
	{
		hallData->mutable_new_user_gift_activity()->set_status(NewbieGiftStatus::HasNewbieGift);
		
		// 统计日志
		++m_newbieGift.registerUsers;
		int curYearDay = getCurrentYearDay();
		if (m_newbieGift.curRegisterUserDay != curYearDay)
		{
			m_newbieGift.curRegisterUserDay = curYearDay;
			m_newbieGift.registerUsers = 1;
		}
		
		ActivityCenterStatisticsLog("ActivityCenter Statistics, newbie activity register users|%s|%d|%d",
		registerTimeRsp.query_username().c_str(), m_newbieGift.curRegisterUserDay, m_newbieGift.registerUsers);
	}
	else
	{
		hallData->mutable_new_user_gift_activity()->set_status(NewbieGiftStatus::WithoutNewbieGifg);
	}

	getActivityData(m_msgHandler->getContext().userFlag, hallData, registerTimeRsp.query_username().c_str());
}

void CActivityCenter::receiveActivityRewardReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::PropChangeRsp propChangeRsp;
	if (!m_msgHandler->parseMsgFromBuffer(propChangeRsp, data, len, "receive activity reward reply")) return;
	
	string userName;
	ConnectProxy* conn = getConnectInfo("receive activity reward reply", userName);
	if (conn == NULL) return;
	
	if (propChangeRsp.result() != Opt_Success)
	{
		OptErrorLog("receive activity reward reply error, user = %s, flag = %d, result = %d", userName.c_str(), m_msgHandler->getContext().userFlag, propChangeRsp.result());
		return;
	}
	
	switch (m_msgHandler->getContext().userFlag)
	{
		case EPropFlag::ReceiveNewbieGift:  // 新手礼包活动
		{
			receiveNewbieReward(conn);
			break;
		}
		
		default:
		{
			const unsigned int userFlag = ((unsigned int)m_msgHandler->getContext().userFlag & 0xFFFF0000) >> 16;
			const unsigned int rewardIdx = (unsigned int)m_msgHandler->getContext().userFlag & 0xFFFF;
			const vector<HallConfigData::RechargeReward>& actvityRewardCfg = HallConfigData::config::getConfigValue().recharge_lottery_activity.recharge_reward_list;
			if (userFlag == EPropFlag::RechargeLotteryGift && rewardIdx < actvityRewardCfg.size())  // 充值抽奖活动
			{
				receiveRechargeLotteryGift(userName, conn, actvityRewardCfg, rewardIdx);
			}
			else
			{
				OptErrorLog("receive activity reward reply error, user = %s, flag = %d, parse flag = %u, idx = %u, reward size = %u",
				userName.c_str(), m_msgHandler->getContext().userFlag, userFlag, rewardIdx, actvityRewardCfg.size());
			}
			
			break;
		}
	}
}

void CActivityCenter::receiveNewbieReward(ConnectProxy* conn)
{
	ConnectUserData* cud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	if (cud != NULL && cud->hallLogicData != NULL) cud->hallLogicData->mutable_new_user_gift_activity()->set_status(NewbieGiftStatus::ReceivedNewbieGift);

	com_protocol::ReceiveNewbieGiftRsp rsp;
	rsp.set_result(Opt_Success);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "send receive newbie gift reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_RECEIVE_NEWBIE_GIFT_RSP, conn);

	// 统计日志
	++m_newbieGift.receiveGifts;
	int curYearDay = getCurrentYearDay();
	if (m_newbieGift.curReceiveGiftDay != curYearDay)
	{
		m_newbieGift.curReceiveGiftDay = curYearDay;
		m_newbieGift.receiveGifts = 1;
	}
	
	ActivityCenterStatisticsLog("ActivityCenter Statistics, newbie activity receive gifts|%s|%d|%d", cud->connId, m_newbieGift.curReceiveGiftDay, m_newbieGift.receiveGifts);
}

void CActivityCenter::receiveRechargeLotteryGift(const string& userName, ConnectProxy* conn, const vector<HallConfigData::RechargeReward>& actvityRewardCfg, const unsigned int rewardIdx)
{
	ConnectUserData* cud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	if (cud == NULL || cud->hallLogicData == NULL)
	{
		OptErrorLog("get user connect data to receive recharge lottery gift error, pointer = %p", cud);
		return;
	}
	
	const HallConfigData::RechargeReward& rechargeGift = actvityRewardCfg[rewardIdx];
	com_protocol::RechargeRewardActivity* rechargeLotteryAct = cud->hallLogicData->mutable_recharge_reward_activity();
	// if (rechargeLotteryAct->lottery_count() > 0) rechargeLotteryAct->set_lottery_count(rechargeLotteryAct->lottery_count() - 1);
	if (rechargeLotteryAct->reward_record_size() >= (int)HallConfigData::config::getConfigValue().recharge_lottery_activity.lottery_record_count
		&& rechargeLotteryAct->reward_record_size() > 0) rechargeLotteryAct->mutable_reward_record()->DeleteSubrange(0, 1);
	
	// 新增抽中奖纪录
	com_protocol::RechargeLotteryRecord* lotteryRecord = rechargeLotteryAct->add_reward_record();
	lotteryRecord->set_time_secs(time(NULL));
	lotteryRecord->set_type(rechargeGift.type);
	lotteryRecord->set_count(rechargeGift.count);

	// 获奖记录时间点
	char strTime[128] = {0};
	struct tm recordTime;
	time_t recordTimeSecs = lotteryRecord->time_secs();
	localtime_r(&recordTimeSecs, &recordTime);
	strftime(strTime, sizeof(strTime) - 1, "%Y-%m-%d %H:%M", &recordTime);

	com_protocol::RechargeWinGiftRsp rsp;
	rsp.set_result(Opt_Success);
	rsp.set_gift_idx(rewardIdx);
	rsp.set_time(strTime);
	com_protocol::GiftInfo* giftInfo = rsp.mutable_gift_info();
	giftInfo->set_type(rechargeGift.type);
	giftInfo->set_num(rechargeGift.count);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "send recharge lottery gift reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_RECHARGE_WIN_GIFT_RSP, conn);
	
	// 是否需要发送走马灯信息
	if (rechargeGift.flag == 1)
	{
		// 【系统】恭喜玩家XXXXX参与充值大抽奖活动获得XXXX！
		const char* goodsName = getGameGoodsChineseName(rechargeGift.type);
		if (goodsName != NULL)
		{
			float giftCount = rechargeGift.count;
			const char* msgFormat = "恭喜玩家 %s 参与充值大抽奖活动获得%.0f%s";
			if (rechargeGift.type == EPropType::PropPhoneFareValue)
			{
				giftCount /= 10.0;
				msgFormat = "恭喜玩家 %s 参与充值大抽奖活动获得%.1f元%s";
			}
			
			char content[1024] = {0};
			unsigned int contentLen = snprintf(content, sizeof(content) - 1, msgFormat, userName.c_str(), giftCount, goodsName);
			com_protocol::MessageNotifyReq msgNotifyReq;
			msgNotifyReq.set_content(content, contentLen);
			msgNotifyReq.set_mode(ESystemMessageMode::ShowRoll);
			const char* msgData = m_msgHandler->serializeMsgToBuffer(msgNotifyReq, "send user lottery reward gift notify");
			if (msgData != NULL)
			{
				m_msgHandler->sendMessageToService(ServiceType::MessageSrv, msgData, msgNotifyReq.ByteSize(), MessageSrvProtocol::BUSINESS_SYSTEM_MESSAGE_NOTIFY,
				userName.c_str(), userName.length());
			}
		}
	}
}

void CActivityCenter::receiveNewbieGift(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive newbie gift request")) return;
	
	ConnectUserData* cud = m_msgHandler->getConnectUserData();
	if (cud == NULL || cud->hallLogicData == NULL)
	{
		OptErrorLog("get user connect data to receive newbie gift error, pointer = %p", cud);
		return;
	}
	
	com_protocol::HallLogicData* hallData = cud->hallLogicData;
	if (hallData->new_user_gift_activity().status() == NewbieGiftStatus::HasNewbieGift)
	{
		NProject::RecordItemVector recordItemVector;
		const HallConfigData::NewbieGiftActivity& actvityCfg = HallConfigData::config::getConfigValue().newbie_gift_activity;
		for (unsigned int idx = 0; idx < actvityCfg.newbie_reward_list.size(); ++idx)
		{
			recordItemVector.push_back(NProject::RecordItem(actvityCfg.newbie_reward_list[idx].type, actvityCfg.newbie_reward_list[idx].count));
		}

		m_msgHandler->notifyDbProxyChangeProp(cud->connId, cud->connIdLen, recordItemVector,
		EPropFlag::ReceiveNewbieGift, "新手礼包", NULL, EServiceReplyProtocolId::CUSTOM_RECEIVE_ACTIVITY_REWARD);
		return;
	}
	
	com_protocol::ReceiveNewbieGiftRsp rsp;
	(hallData->new_user_gift_activity().status() == NewbieGiftStatus::ReceivedNewbieGift) ? rsp.set_result(AlreadyReceivedNewbieGift) : rsp.set_result(NoNewbieGift);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "send receive newbie gift reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_RECEIVE_NEWBIE_GIFT_RSP);
}
	
void CActivityCenter::userRechargeNotify(const char* userName)
{
	unsigned int curSecs = time(NULL);
	if (curSecs < m_rechargeAward.startTimeSecs || curSecs > m_rechargeAward.finishTimeSecs) return;

	com_protocol::HallLogicData* hallData = m_msgHandler->getHallLogicData(userName);
	if (hallData == NULL)
	{
		OptErrorLog("get user hall logic data for add recharge lottery times error, user = %s", userName);
		return;
	}
	
	hallData->mutable_recharge_reward_activity()->set_lottery_count(hallData->recharge_reward_activity().lottery_count() + 1);
}

void CActivityCenter::rechargeLotteryWinGift(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("recharge lottery request")) return;
	
	ConnectUserData* cud = m_msgHandler->getConnectUserData();
	if (cud == NULL || cud->hallLogicData == NULL)
	{
		OptErrorLog("get user connect data to recharge lottery error, pointer = %p", cud);
		return;
	}
	
	int result = Opt_Success;
	while (true)
	{
		if (m_rechargeAward.lastLotteryTimeSecs < (unsigned int)time(NULL))
		{
			result = RechargeLotteryActivityFinish;
			break;
		}
		
		com_protocol::RechargeRewardActivity* rechargeLotteryAct = cud->hallLogicData->mutable_recharge_reward_activity();
		if (rechargeLotteryAct->lottery_count() < 1)
		{
			result = NoRechargeLotteryGift;
			break;
		}
		
		unsigned int allRate = 0;
		IdxToLotteryRewardVector idx2rewardVector;
		const HallConfigData::RechargeLotteryActivity& actvityCfg = HallConfigData::config::getConfigValue().recharge_lottery_activity;
		for (unsigned int idx = 0; idx < actvityCfg.recharge_reward_list.size(); ++idx)
		{
			// 查看话费额度是否还充足
			if (actvityCfg.recharge_reward_list[idx].type == EPropType::PropPhoneFareValue
				&& actvityCfg.max_phone_fare - m_rechargeAward.phoneFare < actvityCfg.recharge_reward_list[idx].count) continue;

			allRate += actvityCfg.recharge_reward_list[idx].rate;
			idx2rewardVector.push_back(IdxToLotteryReward(idx, &(actvityCfg.recharge_reward_list[idx])));
		}
		
		// 计算抽奖概率
		NProject::RecordItemVector recordItemVector;
		const unsigned int percent = getPositiveRandNumber(1, allRate);
		unsigned int lotteryFlagIdx = EPropFlag::RechargeLotteryGift;
		unsigned int currentRate = 0;
		for (unsigned int idx = 0; idx < idx2rewardVector.size(); ++idx)
		{
			currentRate += idx2rewardVector[idx].reward->rate;
			if (percent <= currentRate)
			{
				lotteryFlagIdx = ((lotteryFlagIdx & 0xFFFF) << 16) + (idx2rewardVector[idx].idx & 0xFFFF);  // 保存抽奖标志&奖品索引值
				recordItemVector.push_back(NProject::RecordItem(idx2rewardVector[idx].reward->type, idx2rewardVector[idx].reward->count));
				if (idx2rewardVector[idx].reward->type == EPropType::PropPhoneFareValue) m_rechargeAward.phoneFare += idx2rewardVector[idx].reward->count;

				// 统计日志
				++m_rechargeAward.activityUsers;
				int curYearDay = getCurrentYearDay();
				if (m_rechargeAward.curActivityUserDay != curYearDay)
				{
					m_rechargeAward.curActivityUserDay = curYearDay;
					m_rechargeAward.activityUsers = 1;
				}
				
				m_msgHandler->getStatDataMgr().addStatGoods(EStatisticsDataItem::ERechargeLotteryDraw, idx2rewardVector[idx].reward->type, idx2rewardVector[idx].reward->count);
				
				ActivityCenterStatisticsLog("ActivityCenter Statistics, recharge lottery|%s|%d|%d|%u|%u", cud->connId, m_rechargeAward.curActivityUserDay, m_rechargeAward.activityUsers,
				idx2rewardVector[idx].reward->type, idx2rewardVector[idx].reward->count);
		
				break;
			}
		}
		
		// 防止外挂在消息没回来之前疯狂的发送充值抽奖消息
		// 因此必须提前扣除抽奖次数
	    rechargeLotteryAct->set_lottery_count(rechargeLotteryAct->lottery_count() - 1);
		
		if (!recordItemVector.empty())
		{
		    m_msgHandler->notifyDbProxyChangeProp(cud->connId, cud->connIdLen, recordItemVector, lotteryFlagIdx, "充值抽奖", NULL, EServiceReplyProtocolId::CUSTOM_RECEIVE_ACTIVITY_REWARD);
			OptInfoLog("recharge lottery win gift, user = %s, remain count = %u, all rate = %u, percent = %u, reward size = %u, type = %u, count = %u",
			cud->connId, rechargeLotteryAct->lottery_count(), allRate, percent, idx2rewardVector.size(), recordItemVector[0].type, recordItemVector[0].count);
		}
		
		return;
	}
	
	com_protocol::RechargeWinGiftRsp rsp;
	rsp.set_result(result);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "send recharge lottery reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_RECHARGE_WIN_GIFT_RSP);
}

void CActivityCenter::getActivityData(unsigned int activityType, com_protocol::HallLogicData* hallData, const char* userName)
{
	com_protocol::GetActivityCenterRsp rsp;
	unsigned int curSecs = time(NULL);  // 当前时间点
	switch (activityType)
	{
		case EActivityCenterType::NewbieGift:
		{
			if (addNewbieActivity(hallData, curSecs, activityType, rsp) == NewbieGiftStatus::InitNewbie) return;  // 新手礼包活动
			break;
		}
		
		case EActivityCenterType::RechargeLottery:
		{
			addRechargeLotteryActivity(hallData, curSecs, rsp);  // 充值抽奖获得
			break;
		}
		
		case EActivityCenterType::ActivityList:
		{
			if (addNewbieActivity(hallData, curSecs, activityType, rsp) == NewbieGiftStatus::InitNewbie) return;  // 新手礼包活动
			addRechargeLotteryActivity(hallData, curSecs, rsp);  // 充值抽奖获得
			break;
		}
		
		default:
		{
			OptErrorLog("get activity data, the type invalid, type = %u", activityType);
			return;
		}
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "send activity center data reply");
	if (msgData != NULL)
	{
		if (userName == NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_GET_ACTIVITY_CENTER_RSP);
		else m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_GET_ACTIVITY_CENTER_RSP, userName);
	}
}

int CActivityCenter::addNewbieActivity(com_protocol::HallLogicData* hallData, unsigned int curSecs, unsigned int activityType, com_protocol::GetActivityCenterRsp& rsp)
{
	int newbieStatus = -1;
	while (curSecs >= m_newbieGift.startTimeSecs && curSecs < m_newbieGift.finishTimeSecs)  // 还在新手礼包活动时间范围内
	{
		newbieStatus = hallData->new_user_gift_activity().status();
		if (newbieStatus == NewbieGiftStatus::WithoutNewbieGifg) break;  // 没有新手礼包
		
		if (newbieStatus == NewbieGiftStatus::InitNewbie)  // 需要查询用户注册时间点
		{
			// 查询该用户的注册时间
			ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
			com_protocol::GetUserBaseinfoReq getUserRegisterTime;
			getUserRegisterTime.set_query_username(ud->connId, ud->connIdLen);
	        const char* msgData = m_msgHandler->serializeMsgToBuffer(getUserRegisterTime, "get user register time");
			if (msgData != NULL)
			{
				m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgData, getUserRegisterTime.ByteSize(), CommonDBSrvProtocol::BUSINESS_GET_USER_BASEINFO_REQ,
				ud->connId, ud->connIdLen, activityType, EServiceReplyProtocolId::CUSTOM_GET_USER_REGISTER_TIME_FOR_NEWBIE_ACTIVITY);
			}
			
			break;
		}
		
		rsp.add_activity_types(EActivityCenterType::NewbieGift);
		com_protocol::NewbieGiftPackageActivity* newbieActivity = rsp.mutable_newbie_gift_activity();
		com_protocol::ActivityBaseInfo* baseInfo = newbieActivity->mutable_base_info();
		const HallConfigData::NewbieGiftActivity& actvityCfg = HallConfigData::config::getConfigValue().newbie_gift_activity;
		baseInfo->set_title(actvityCfg.title);
		// baseInfo->set_desc(actvityCfg.desc);
		baseInfo->set_rule(actvityCfg.rule);
		baseInfo->set_time(actvityCfg.time);
		
		for (unsigned int idx = 0; idx < actvityCfg.newbie_reward_list.size(); ++idx)
		{
			com_protocol::GiftInfo* gift = newbieActivity->add_gift_info();
			gift->set_type(actvityCfg.newbie_reward_list[idx].type);
			gift->set_num(actvityCfg.newbie_reward_list[idx].count);
		}
		
		newbieActivity->set_status(newbieStatus);
		
		break;
	}
	
	return newbieStatus;
}

void CActivityCenter::addRechargeLotteryActivity(com_protocol::HallLogicData* hallData, unsigned int curSecs, com_protocol::GetActivityCenterRsp& rsp)
{
	if (curSecs >= m_rechargeAward.startTimeSecs && curSecs < m_rechargeAward.finishTimeSecs)  // 还在活动时间范围内
	{
		rsp.add_activity_types(EActivityCenterType::RechargeLottery);
		com_protocol::RechargeWinGiftActivity* rechargeLotteryActivity = rsp.mutable_recharge_lottery_activity();
		com_protocol::ActivityBaseInfo* baseInfo = rechargeLotteryActivity->mutable_base_info();
		const HallConfigData::RechargeLotteryActivity& actvityCfg = HallConfigData::config::getConfigValue().recharge_lottery_activity;
		baseInfo->set_title(actvityCfg.title);
		// baseInfo->set_desc(actvityCfg.desc);
		baseInfo->set_rule(actvityCfg.rule);
		baseInfo->set_time(actvityCfg.time);
		
		for (unsigned int idx = 0; idx < actvityCfg.recharge_reward_list.size(); ++idx)
		{
			com_protocol::GiftInfo* gift = rechargeLotteryActivity->add_lottery_gift_list();
			gift->set_type(actvityCfg.recharge_reward_list[idx].type);
			gift->set_num(actvityCfg.recharge_reward_list[idx].count);
		}
		
		char strTime[128] = {0};
		struct tm recordTime;
		time_t recordTimeSecs = 0;
		rechargeLotteryActivity->set_lottery_count(hallData->recharge_reward_activity().lottery_count());
		for (int idx = 0; idx < hallData->recharge_reward_activity().reward_record_size(); ++idx)
		{
			const com_protocol::RechargeLotteryRecord& rewardRecord = hallData->recharge_reward_activity().reward_record(idx);
			com_protocol::RechargeAwardRecord* lotteryRecord = rechargeLotteryActivity->add_reward_record();
			lotteryRecord->set_type(rewardRecord.type());
			lotteryRecord->set_count(rewardRecord.count());
			
			// 获奖记录时间点
			recordTimeSecs = rewardRecord.time_secs();
			localtime_r(&recordTimeSecs, &recordTime);
			strftime(strTime, sizeof(strTime) - 1, "%Y-%m-%d %H:%M", &recordTime);
			lotteryRecord->set_time(strTime);
		}
	}
}


// PK场任务活动
void CActivityCenter::getPKTaskInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("get PK tast info request")) return;
	
	const com_protocol::LoginPKTask& pkTaskData = m_msgHandler->getHallLogicData()->pk_task();
	com_protocol::ClientGetPKTaskRsp rsp;
	
	getPKJoinTaskInfo(pkTaskData.join_match(), rsp.mutable_join_match());  // 对局任务信息
    getPKWinTaskInfo(pkTaskData.win_match(), rsp.mutable_win_match());  // 赢局任务信息
	getPKOnlineTaskInfo(pkTaskData.online_task(), rsp.mutable_online_task());  // PK在线任务信息

	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "send PK task data reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_GET_PK_TASK_INFO_RSP);
}

// 对局任务信息
void CActivityCenter::getPKJoinTaskInfo(const com_protocol::LoginPKJoinMatch& join_match, com_protocol::ClientPKMatchTask* matchTask)
{
	// 对局任务
	const vector<HallConfigData::CfgJoinMatch>& joinMatchCfg = HallConfigData::config::getConfigValue().pk_join_match_list;
	const HallConfigData::CfgJoinMatch* jmCfg = NULL;
	if ((join_match.match_idx() >= joinMatchCfg.size()) // 配置的任务数减少了则认为是完成了所有任务了
	   || (join_match.match_idx() == (joinMatchCfg.size() - 1) && join_match.reward_idx() > -1)) // 最后一个任务并且已经领奖了
	{
		matchTask->set_finish_times(0);
		matchTask->set_need_times(0);
		matchTask->set_is_finish(1);
		
		if (joinMatchCfg.size() > 0) jmCfg = &(joinMatchCfg[joinMatchCfg.size() - 1]);
	}
	else
	{
		matchTask->set_finish_times(join_match.join_times());
		matchTask->set_need_times(joinMatchCfg[join_match.match_idx()].times);
		if (matchTask->finish_times() > matchTask->need_times()) matchTask->set_finish_times(matchTask->need_times());
		matchTask->set_is_finish(0);
		
		jmCfg = &(joinMatchCfg[join_match.match_idx()]);
	}

    if (jmCfg != NULL)
	{
		matchTask->set_name(jmCfg->name);
		matchTask->set_desc(jmCfg->desc);
		for (map<unsigned int, unsigned int>::const_iterator rwIt = jmCfg->rewards.begin(); rwIt != jmCfg->rewards.end(); ++rwIt)
		{
			com_protocol::GiftInfo* gf = matchTask->add_gift_info();
			gf->set_type(rwIt->first);
			gf->set_num(rwIt->second);
		}
	}
}

// 赢局任务信息
void CActivityCenter::getPKWinTaskInfo(const com_protocol::LoginPKWinMatch& win_match, com_protocol::ClientPKMatchTask* matchTask)
{
	// 赢局任务
	const vector<HallConfigData::CfgWinMatch>& winMatchCfg = HallConfigData::config::getConfigValue().pk_win_match_list;
	const HallConfigData::CfgWinMatch* wmCfg = NULL;
	if ((win_match.match_idx() >= winMatchCfg.size()) // 配置的任务数减少了则认为是完成了所有任务了
	   || (win_match.match_idx() == (winMatchCfg.size() - 1) && win_match.reward_idx() > -1)) // 最后一个任务并且已经领奖了
	{
		matchTask->set_finish_times(0);
		matchTask->set_need_times(0);
		matchTask->set_is_finish(1);
		
		wmCfg = &(winMatchCfg[winMatchCfg.size() - 1]);
	}
	else
	{
		matchTask->set_finish_times(win_match.win_times());
		matchTask->set_need_times(winMatchCfg[win_match.match_idx()].times);
		if (matchTask->finish_times() > matchTask->need_times()) matchTask->set_finish_times(matchTask->need_times());
		matchTask->set_is_finish(0);
		
		wmCfg = &(winMatchCfg[win_match.match_idx()]);
	}
	
	if (wmCfg != NULL)
	{
		matchTask->set_name(wmCfg->name);
		matchTask->set_desc(wmCfg->desc);
		for (map<unsigned int, unsigned int>::const_iterator rwIt = wmCfg->rewards.begin(); rwIt != wmCfg->rewards.end(); ++rwIt)
		{
			com_protocol::GiftInfo* gf = matchTask->add_gift_info();
			gf->set_type(rwIt->first);
			gf->set_num(rwIt->second);
		}
	}
}

// PK在线任务信息
void CActivityCenter::getPKOnlineTaskInfo(const com_protocol::LoginPKOnlineTask& onlineTask, com_protocol::ClientPKOnlineTask* pkOnlineTask)
{
	// PK在线任务
	struct tm curTime;
	const time_t curSecs = time(NULL);
	localtime_r(&curSecs, &curTime);
	
	const vector<HallConfigData::CfgOnlineTask>& onlineTaskCfg = HallConfigData::config::getConfigValue().pk_online_task;
	int nextIdx = -1;
	int curTaskIdx = getPKOnlineTaskIndex(curTime.tm_hour, curTime.tm_min, nextIdx);
	pkOnlineTask->set_has_reward(0);
	pkOnlineTask->set_is_finish(1);
	if (curTaskIdx < 0)
	{
		if (nextIdx > -1)
		{
			curTaskIdx = nextIdx;
			pkOnlineTask->set_is_finish(0);  // 存在下一个任务
		}
		else
		{
			curTaskIdx = onlineTaskCfg.size() - 1;
		}
	}
	else if (onlineTask.reward_idx() != curTaskIdx)  // 存在奖励没领取
	{
		pkOnlineTask->set_has_reward(1);
		pkOnlineTask->set_is_finish(0);
	}
	else if (nextIdx > -1)  // 当前已经领取了奖励，存在下一个任务
	{
		curTaskIdx = nextIdx;
		pkOnlineTask->set_is_finish(0);
	}
	
	if (curTaskIdx > -1)
	{
		const HallConfigData::CfgOnlineTask& oTCfg = onlineTaskCfg[curTaskIdx];
		pkOnlineTask->set_name(oTCfg.name);
		pkOnlineTask->set_desc(oTCfg.desc);
		pkOnlineTask->set_begin_hour(oTCfg.beginHour);
		pkOnlineTask->set_begin_min(oTCfg.beginMin);
		pkOnlineTask->set_end_hour(oTCfg.endHour);
		pkOnlineTask->set_end_min(oTCfg.endMin);
		
		for (map<unsigned int, unsigned int>::const_iterator rwIt = oTCfg.rewards.begin(); rwIt != oTCfg.rewards.end(); ++rwIt)
		{
			com_protocol::GiftInfo* gf = pkOnlineTask->add_gift_info();
			gf->set_type(rwIt->first);
			gf->set_num(rwIt->second);
		}
	}
}

void CActivityCenter::receivePKTaskReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive PK task reward request")) return;
	
	com_protocol::ClientReceivePKTaskRewardReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "receive PK task reward request")) return;
	
	int flag = 0;
	const map<unsigned int, unsigned int>* matchRewards = NULL;
	string remark;
	const com_protocol::LoginPKTask& pkTaskData = m_msgHandler->getHallLogicData()->pk_task();
	switch (req.task_type())
	{
		case EPKTaskType::EPKJoinType:
		{
			const vector<HallConfigData::CfgJoinMatch>& joinMatchCfg = HallConfigData::config::getConfigValue().pk_join_match_list;
	        const com_protocol::LoginPKJoinMatch& join_match = pkTaskData.join_match();
			if (join_match.reward_idx() < 0 && join_match.match_idx() < joinMatchCfg.size() && join_match.join_times() >= joinMatchCfg[join_match.match_idx()].times)
			{
				flag = EPropFlag::JoinMatchReward;
				matchRewards = &(joinMatchCfg[join_match.match_idx()].rewards);
				remark = "PK对局任务奖励";
			}
			
			break;
		}
		
		case EPKTaskType::EPKWinType:
		{
			const vector<HallConfigData::CfgWinMatch>& winMatchCfg = HallConfigData::config::getConfigValue().pk_win_match_list;
	        const com_protocol::LoginPKWinMatch& win_match = pkTaskData.win_match();
			if (win_match.reward_idx() < 0 && win_match.match_idx() < winMatchCfg.size() && win_match.win_times() >= winMatchCfg[win_match.match_idx()].times)
			{
				flag = EPropFlag::WinMatchReward;
				matchRewards = &(winMatchCfg[win_match.match_idx()].rewards);
				remark = "PK赢局任务奖励";
			}
			
			break;
		}
		
		case EPKTaskType::EPKOnlineType:
		{
			// PK在线任务
			struct tm curTime;
			const time_t curSecs = time(NULL);
			localtime_r(&curSecs, &curTime);
			
			int nextIdx = -1;
			int curTaskIdx = getPKOnlineTaskIndex(curTime.tm_hour, curTime.tm_min, nextIdx);
			if (curTaskIdx > -1 && pkTaskData.online_task().reward_idx() != curTaskIdx)
			{
				flag = EPropFlag::OnlineMatchReward;
				matchRewards = &(HallConfigData::config::getConfigValue().pk_online_task[curTaskIdx].rewards);
				remark = "PK在线任务奖励";
			}
	
			break;
		}
		
		default:
		{
			OptErrorLog("receive PK task reward, the type invalid, type = %u", req.task_type());
			return;
		}
	}
	
	if (matchRewards != NULL)
	{
		// 发送消息到DB common领取奖励物品
		NProject::RecordItemVector recordItemVector;
		for (map<unsigned int, unsigned int>::const_iterator rwIt = matchRewards->begin(); rwIt != matchRewards->end(); ++rwIt)
		{
			recordItemVector.push_back(RecordItem(rwIt->first, rwIt->second));

			m_msgHandler->getStatDataMgr().addStatGoods(EStatisticsDataItem::EPKTaskStat, rwIt->first, rwIt->second);  // 统计数据
		}
		
		ConnectUserData* ud = m_msgHandler->getConnectUserData();
		m_msgHandler->notifyDbProxyChangeProp(ud->connId, ud->connIdLen, recordItemVector, flag, remark.c_str());
	}
}

void CActivityCenter::resetPKTask(ConnectUserData* ud, unsigned int theDay)
{
	if (!ud->hallLogicData->has_pk_task() || ud->hallLogicData->pk_task().today() != theDay)
	{
		com_protocol::LoginPKTask* pkTask = ud->hallLogicData->mutable_pk_task();
		pkTask->set_today(theDay);
		
		com_protocol::LoginPKJoinMatch* joinMatch = pkTask->mutable_join_match();
		joinMatch->set_join_times(0);
		joinMatch->set_match_idx(0);
		joinMatch->set_reward_idx(-1);
		
		com_protocol::LoginPKWinMatch* winMatch = pkTask->mutable_win_match();
		winMatch->set_win_times(0);
		winMatch->set_match_idx(0);
		winMatch->set_reward_idx(-1);
		
		com_protocol::LoginPKOnlineTask* onlineTask = pkTask->mutable_online_task();
		onlineTask->set_reward_idx(-1);
	}
}

int CActivityCenter::getPKOnlineTaskIndex(const unsigned int hour, const unsigned int min, int& nextIdx)
{
	nextIdx = -1;
	const vector<HallConfigData::CfgOnlineTask>& onlineTaskCfg = HallConfigData::config::getConfigValue().pk_online_task;
	for (unsigned int idx = 0; idx < onlineTaskCfg.size(); ++idx)
	{
		const HallConfigData::CfgOnlineTask& cfgOT = onlineTaskCfg[idx];
		if ((hour > cfgOT.beginHour || (hour == cfgOT.beginHour && min >= cfgOT.beginMin))  // 当前时间大于配置的开始时间
		    && (hour < cfgOT.endHour || (hour == cfgOT.endHour && min < cfgOT.endMin)))    // 当前时间小于配置的结束时间
		{
			if ((idx + 1) < onlineTaskCfg.size()) nextIdx = idx + 1;
			return idx;
		}
		
		if (hour < cfgOT.beginHour || (hour == cfgOT.beginHour && min < cfgOT.beginMin))
		{
			nextIdx = idx;
			break;
		}
	}
	
	return -1;
}

// 领取了PK任务奖励
void CActivityCenter::receivePKTaskRewardReply(const unsigned int flag)
{
	com_protocol::HallLogicData* hLD = m_msgHandler->getHallLogicData(m_msgHandler->getContext().userData);
	if (hLD == NULL) return;  // 玩家下线了
	
	const map<unsigned int, unsigned int>* matchRewards = NULL;
	com_protocol::ClientReceivePKTaskRewardRsp rsp;
	rsp.set_result(0);
	
	com_protocol::LoginPKTask* pkTaskData = hLD->mutable_pk_task();
	switch (flag)
	{
		case EPropFlag::JoinMatchReward:
		{
			const vector<HallConfigData::CfgJoinMatch>& joinMatchCfg = HallConfigData::config::getConfigValue().pk_join_match_list;
	        com_protocol::LoginPKJoinMatch* join_match = pkTaskData->mutable_join_match();
			join_match->set_reward_idx(join_match->match_idx());
			matchRewards = &(joinMatchCfg[join_match->match_idx()].rewards);
			
			if ((int)join_match->match_idx() < ((int)joinMatchCfg.size() - 1))  // 还存在任务没有完成
			{
				join_match->set_reward_idx(-1);
				join_match->set_join_times(0);
				join_match->set_match_idx(join_match->match_idx() + 1);
			}

			rsp.set_task_type(EPKTaskType::EPKJoinType);
			getPKJoinTaskInfo(pkTaskData->join_match(), rsp.mutable_new_task());  // 对局任务信息
			
			break;
		}
		
		case EPropFlag::WinMatchReward:
		{
			const vector<HallConfigData::CfgWinMatch>& winMatchCfg = HallConfigData::config::getConfigValue().pk_win_match_list;
	        com_protocol::LoginPKWinMatch* win_match = pkTaskData->mutable_win_match();
			win_match->set_reward_idx(win_match->match_idx());
			matchRewards = &(winMatchCfg[win_match->match_idx()].rewards);
			
			if ((int)win_match->match_idx() < ((int)winMatchCfg.size() - 1))  // 还存在任务没有完成
			{
				win_match->set_reward_idx(-1);
				win_match->set_win_times(0);
				win_match->set_match_idx(win_match->match_idx() + 1);
			}
			
			rsp.set_task_type(EPKTaskType::EPKWinType);
			getPKWinTaskInfo(pkTaskData->win_match(), rsp.mutable_new_task());  // 赢局任务信息
			
			break;
		}
		
		case EPropFlag::OnlineMatchReward:
		{
			// PK在线任务
			struct tm curTime;
			const time_t curSecs = time(NULL);
			localtime_r(&curSecs, &curTime);
			
			int nextIdx = -1;
			int curTaskIdx = getPKOnlineTaskIndex(curTime.tm_hour, curTime.tm_min, nextIdx);
			pkTaskData->mutable_online_task()->set_reward_idx(curTaskIdx);
			
			if (curTaskIdx > -1) matchRewards = &(HallConfigData::config::getConfigValue().pk_online_task[curTaskIdx].rewards);
			
			rsp.set_task_type(EPKTaskType::EPKOnlineType);
			getPKOnlineTaskInfo(pkTaskData->online_task(), rsp.mutable_online_task());  // PK在线任务信息

			break;
		}
		
		default:
		{
			OptErrorLog("receive PK task reward reply, the type invalid, type = %u", flag);
			return;
		}
	}
	
	if (matchRewards != NULL)
	{
		for (map<unsigned int, unsigned int>::const_iterator rwIt = matchRewards->begin(); rwIt != matchRewards->end(); ++rwIt)
		{
			com_protocol::GiftInfo* gI = rsp.add_received_gift();
			gI->set_type(rwIt->first);
			gI->set_num(rwIt->second);
		}
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "receive PK reward reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_RECEIVE_PK_TASK_REWARD_RSP, m_msgHandler->getContext().userData);
}

// 新增PK任务参赛次数
void CActivityCenter::addPKJoinMatchTimes(const char* userName)
{
	com_protocol::HallLogicData* hLD = m_msgHandler->getHallLogicData(userName);
	if (hLD == NULL) return;  // 玩家下线了
	
	com_protocol::LoginPKJoinMatch* joinMatch = hLD->mutable_pk_task()->mutable_join_match();
	const vector<HallConfigData::CfgJoinMatch>& joinMatchCfg = HallConfigData::config::getConfigValue().pk_join_match_list;
	if (joinMatch->match_idx() < joinMatchCfg.size() && joinMatch->join_times() < joinMatchCfg[joinMatch->match_idx()].times)
	{
		joinMatch->set_join_times(joinMatch->join_times() + 1);
	}
}

// 新增PK任务赢局次数
void CActivityCenter::addPKWinMatchTimes(const char* userName)
{
	com_protocol::HallLogicData* hLD = m_msgHandler->getHallLogicData(userName);
	if (hLD == NULL) return;  // 玩家下线了
	
	com_protocol::LoginPKWinMatch* winMatch = hLD->mutable_pk_task()->mutable_win_match();
	const vector<HallConfigData::CfgWinMatch>& winMatchCfg = HallConfigData::config::getConfigValue().pk_win_match_list;
	if (winMatch->match_idx() < winMatchCfg.size() && winMatch->win_times() < winMatchCfg[winMatch->match_idx()].times)
	{
		winMatch->set_win_times(winMatch->win_times() + 1);
	}
}

}

