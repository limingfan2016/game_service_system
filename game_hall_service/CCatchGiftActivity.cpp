
/* author : limingfan
 * date : 2017.03.15
 * description : 福袋活动
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>

#include "CLoginSrv.h"
#include "CCatchGiftActivity.h"
#include "base/Function.h"
#include "../common/CommonType.h"


using namespace NProject;


namespace NPlatformService
{

// 阶段时间值调整
struct SStageTime
{
	unsigned int index;
	unsigned int timeSecs;
	
	SStageTime() : index(0), timeSecs(0) {};
	SStageTime(unsigned int _index, unsigned int _timeSecs) : index(_index), timeSecs(_timeSecs) {};
	~SStageTime() {};
};

static bool StageTimeSort(const SStageTime& v1, const SStageTime& v2)
{
	return v1.timeSecs < v2.timeSecs;
}
	
	
CCatchGiftActivity::CCatchGiftActivity()
{
	m_msgHandler = NULL;
}

CCatchGiftActivity::~CCatchGiftActivity()
{
	m_msgHandler = NULL;
}

void CCatchGiftActivity::load(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;

    m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_ENTER_CATCH_GIFT_ACTIVITY_REQ, (ProtocolHandler)&CCatchGiftActivity::enterCatchGiftActivity, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_CATCH_GIFT_ACTIVITY_TIME_REQ, (ProtocolHandler)&CCatchGiftActivity::getCatchGiftActivityTime, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_HIT_CATCH_GIFT_ACTIVITY_GOODS_REQ, (ProtocolHandler)&CCatchGiftActivity::hitCatchGiftActivityDropGoods, this);

    m_msgHandler->registerProtocol(LoginSrv, CUSTOM_GET_USER_INFO_FOR_CATCH_GIFT_ACTIVITY, (ProtocolHandler)&CCatchGiftActivity::enterCatchGiftActivityReply, this);
	m_msgHandler->registerProtocol(LoginSrv, CUSTOM_GET_USER_VIP_FOR_CATCH_GIFT_ACTIVITY, (ProtocolHandler)&CCatchGiftActivity::getCatchGiftActivityTimeReply, this);
}

void CCatchGiftActivity::unLoad()
{

}

void CCatchGiftActivity::onLine(ConnectUserData* ud, ConnectProxy* conn)
{
	update(ud->hallLogicData);
}

void CCatchGiftActivity::offLine(ConnectUserData* ud)
{
	clearUserData(ud->connId);  // 清空可能存在的遗留数据
}

void CCatchGiftActivity::update(com_protocol::HallLogicData* hallLogicData)
{
	const unsigned int today = getCurrentYearDay();
	if (today != hallLogicData->catch_gift_activity().today())
	{
		com_protocol::CatchGiftActivity* cgaInfo = hallLogicData->mutable_catch_gift_activity();
		cgaInfo->set_today(today);
		cgaInfo->set_times(0);
		cgaInfo->set_finish_time_secs(0);
	}
}

// 进入福袋活动
void CCatchGiftActivity::enterCatchGiftActivity(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to enter catch gift activity")) return;
	
	ConnectUserData* cud = m_msgHandler->getConnectUserData();
	clearUserData(cud->connId);  // 清空可能存在的遗留数据

	// 查询金币&VIP等级
	com_protocol::GetUserOtherInfoReq getGoldVIPMsg;
	getGoldVIPMsg.set_query_username(cud->connId);
	getGoldVIPMsg.add_info_flag(EPropType::PropGold);
	getGoldVIPMsg.add_info_flag(EUserInfoFlag::EVipLevel);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(getGoldVIPMsg, "get user gold and vip for enter catch gift activity");
	if (msgData != NULL) m_msgHandler->sendMessageToService(NFrame::ServiceType::DBProxyCommonSrv, msgData, getGoldVIPMsg.ByteSize(), BUSINESS_GET_USER_OTHER_INFO_REQ, cud->connId, cud->connIdLen, 0, CUSTOM_GET_USER_INFO_FOR_CATCH_GIFT_ACTIVITY);
}

void CCatchGiftActivity::enterCatchGiftActivityReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string username;
	ConnectProxy* conn = m_msgHandler->getConnectInfo("get user info for enter catch gift activity reply", username);
	if (conn == NULL) return;
	
	com_protocol::GetUserOtherInfoRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "get user info for enter catch gift activity reply")) return;
	
	if (rsp.result() != 0)
	{
		OptErrorLog("get user info for enter catch gift activity reply error, result = %d", rsp.result());
		return;
	}
	
	ConnectUserData* cud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	update(cud->hallLogicData);  // 先刷新时间&数据
	
	const unsigned int goldCount = rsp.other_info(0).int_value();
	const unsigned int vipLevel = rsp.other_info(1).int_value();
	const HallConfigData::CatchGiftActivityCfg& cgaCfg = HallConfigData::config::getConfigValue().catch_gift_activity_cfg;
	const com_protocol::CatchGiftActivity& cgaInfo = cud->hallLogicData->catch_gift_activity();
	
	com_protocol::ClientEnterCatchGiftActivityRsp clientRsp;
	clientRsp.set_result(Opt_Success);
	NProject::RecordItemVector recordItemVector;
	if (cgaInfo.times() >= cgaCfg.day_free_times)
	{
		const unsigned int intervalSecs = (vipLevel >= cgaCfg.min_vip_level) ? cgaCfg.vip_user_time_interval : cgaCfg.normal_user_time_interval;
		const unsigned int currentSecs = time(NULL);
		const unsigned int nextTimeSecs = cgaInfo.finish_time_secs() + intervalSecs;
		if (currentSecs < nextTimeSecs)
		{
		    clientRsp.set_result(InvalidCatchGiftActivityTime);
		    clientRsp.set_next_remain_secs(nextTimeSecs - currentSecs);
			clientRsp.set_vip_level(cgaCfg.min_vip_level);
			clientRsp.set_activity_interval_secs(intervalSecs);
		}
		else if (goldCount < cgaCfg.need_pay_gold)
		{
			clientRsp.set_result(CatchGiftActivityGoldInsufficient);
			clientRsp.set_vip_level(cgaCfg.min_vip_level);
			clientRsp.set_need_pay_gold(cgaCfg.need_pay_gold);
		}
		else
		{
			recordItemVector.push_back(RecordItem(EPropType::PropGold, -cgaCfg.need_pay_gold));  // 扣除入场费
		}
	}
	
	if (clientRsp.result() == Opt_Success)
	{
		// 创建玩家活动数据
		m_catchGiftActivityData.userData[cud->connId] = SCGAUserData();
	    SCGAUserData& cgaUserData = m_catchGiftActivityData.userData[cud->connId];
		
		int rc = createCGAUserData(cud->connId, vipLevel, cgaCfg, cgaUserData);       // 创建玩家活动数据
		if (rc == Opt_Success) rc = startActivity(cud->connId, cgaCfg, cgaUserData);  // 开启活动，启动掉落物品定时器
		
		if (rc == Opt_Success)
		{	
			// 扣除入场费
			if (!recordItemVector.empty()) m_msgHandler->notifyDbProxyChangeProp(cud->connId, cud->connIdLen, recordItemVector, EPropFlag::EnterCatchGiftActivity, "参加福袋活动入场费");
			
			// 刷新活动DB数据
			com_protocol::CatchGiftActivity* updateCGAInfo = cud->hallLogicData->mutable_catch_gift_activity();
			updateCGAInfo->set_times(updateCGAInfo->times() + 1);
			updateCGAInfo->set_finish_time_secs(cgaUserData.finishTimeSecs);

			clientRsp.set_activity_time_secs(cgaCfg.activity_time_secs);
		}
		else
		{
			clientRsp.set_result(rc);
			clearUserData(cud->connId);
		}
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(clientRsp, "enter catch gift activity reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, clientRsp.ByteSize(), LOGINSRV_ENTER_CATCH_GIFT_ACTIVITY_RSP, conn);
	
	OptInfoLog("enter catch gift activity, user = %s, result = %d, times = %u", cud->connId, clientRsp.result(), cgaInfo.times());
}

// 获取福袋活动时间
void CCatchGiftActivity::getCatchGiftActivityTime(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get catch gift activity time")) return;
	
	ConnectUserData* cud = m_msgHandler->getConnectUserData();
	update(cud->hallLogicData);  // 先刷新时间&数据

	if (cud->hallLogicData->catch_gift_activity().times() < HallConfigData::config::getConfigValue().catch_gift_activity_cfg.day_free_times)
	{
		// 可直接参加活动
		com_protocol::ClientGetCatchGiftActivityTimeRsp clientRsp;
		clientRsp.set_remain_secs(0);	
		const char* msgData = m_msgHandler->serializeMsgToBuffer(clientRsp, "get catch gift activity time direct reply");
		if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, clientRsp.ByteSize(), LOGINSRV_GET_CATCH_GIFT_ACTIVITY_TIME_RSP);
		
		return;
	}
	
	// 查询金币&VIP等级
	com_protocol::GetUserOtherInfoReq getVIPMsg;
	getVIPMsg.set_query_username(cud->connId);
	getVIPMsg.add_info_flag(EUserInfoFlag::EVipLevel);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(getVIPMsg, "get user vip for view catch gift activity time");
	if (msgData != NULL) m_msgHandler->sendMessageToService(NFrame::ServiceType::DBProxyCommonSrv, msgData, getVIPMsg.ByteSize(), BUSINESS_GET_USER_OTHER_INFO_REQ, cud->connId, cud->connIdLen, 0, CUSTOM_GET_USER_VIP_FOR_CATCH_GIFT_ACTIVITY);
}

void CCatchGiftActivity::getCatchGiftActivityTimeReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string username;
	ConnectProxy* conn = m_msgHandler->getConnectInfo("get user vip for view catch gift activity time reply", username);
	if (conn == NULL) return;
	
	com_protocol::GetUserOtherInfoRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "get user vip for view catch gift activity time reply")) return;
	
	if (rsp.result() != 0)
	{
		OptErrorLog("get user vip for view catch gift activity time reply error, result = %d", rsp.result());
		return;
	}
	
	ConnectUserData* cud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	const unsigned int vipLevel = rsp.other_info(0).int_value();
	const HallConfigData::CatchGiftActivityCfg& cgaCfg = HallConfigData::config::getConfigValue().catch_gift_activity_cfg;
	const com_protocol::CatchGiftActivity& cgaInfo = cud->hallLogicData->catch_gift_activity();
	
	com_protocol::ClientGetCatchGiftActivityTimeRsp clientRsp;
	clientRsp.set_remain_secs(0);
	if (cgaInfo.times() >= cgaCfg.day_free_times)
	{
		const unsigned int intervalSecs = (vipLevel >= cgaCfg.min_vip_level) ? cgaCfg.vip_user_time_interval : cgaCfg.normal_user_time_interval;
		const unsigned int currentSecs = time(NULL);
		const unsigned int nextTimeSecs = cgaInfo.finish_time_secs() + intervalSecs;
		if (currentSecs < nextTimeSecs) clientRsp.set_remain_secs(nextTimeSecs - currentSecs);
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(clientRsp, "get catch gift activity time reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, clientRsp.ByteSize(), LOGINSRV_GET_CATCH_GIFT_ACTIVITY_TIME_RSP, conn);
}

// 碰撞命中福袋活动掉落的物品
void CCatchGiftActivity::hitCatchGiftActivityDropGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to hit catch gift activity drop goods")) return;
	
	ConnectUserData* cud = m_msgHandler->getConnectUserData();
	SCGAUserData* actData = getUserActivityData(cud->connId);
	if (actData == NULL) return;

    com_protocol::ClientHitCatchGiftActivityGoodsReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "hit catch gift activity drop goods request")) return;
	
	SCGADropGoodsMap::iterator goodsIt = actData->amountDropGoods.find(req.goods().id());
	if (goodsIt == actData->amountDropGoods.end()) return;  // 物品不存在，可能过期被删除了

    const unsigned int curSecs = time(NULL);
	const HallConfigData::CatchGiftActivityCfg& cgaCfg = HallConfigData::config::getConfigValue().catch_gift_activity_cfg;
	if ((goodsIt->second.time + cgaCfg.goods_drop_secs) < curSecs)  // 防止玩家作弊行为，时间已经过期
	{
		actData->amountDropGoods.erase(goodsIt);
		
		return;
	}

	// 防止玩家作弊行为，瞬间发一堆碰撞命中消息
	if (curSecs > actData->intervalTimeSecs)
	{
		actData->intervalTimeSecs = curSecs + cgaCfg.catch_use_secs;
		actData->hitCount = 0;
	}
	
	if (++actData->hitCount > cgaCfg.catch_max_goods) return;  // 防止玩家作弊行为，瞬间发一堆碰撞命中消息
	
	if (goodsIt->second.type != (unsigned int)req.goods().type()) return;  // 类型不匹配
	
	if (goodsIt->second.stage >= cgaCfg.activity_stage_time_range_array.size()) return;  // 配置变更

    const unsigned int goodsCfgType = (goodsIt->second.type > com_protocol::ClientCatchGiftActivityGoodsType::ECGA_RedPacket) ? com_protocol::ClientCatchGiftActivityGoodsType::ECGA_GoldBomb : goodsIt->second.type;
	const HallConfigData::ActivityStageTimeRange& stageTimeRange = cgaCfg.activity_stage_time_range_array[goodsIt->second.stage];
	map<unsigned int, HallConfigData::CGA_DropGoods>::const_iterator goodsDropIt = stageTimeRange.drop_goods_map.find(goodsCfgType);
	if (goodsDropIt == stageTimeRange.drop_goods_map.end()) return;  // 配置变更
	
	actData->amountDropGoods.erase(goodsIt);  // 命中了则删除该物品信息

	com_protocol::ClientHitCatchGiftActivityGoodsRsp rsp;
	unsigned int randCount = getPositiveRandNumber(goodsDropIt->second.get_min_number, goodsDropIt->second.get_max_number);
	switch (req.goods().type())
	{
		case com_protocol::ClientCatchGiftActivityGoodsType::ECGA_Gold:
		case com_protocol::ClientCatchGiftActivityGoodsType::ECGA_RedPacket:
		{
			if (actData->amountGold < cgaCfg.activity_gold_amount)
			{
			    actData->amountGold += randCount;
			    rsp.set_amount_gold(actData->amountGold);
			}
			else
			{
				OptErrorLog("gold is already enough error, user = %s, amount gold = %u, config gold = %u", cud->connId, actData->amountGold, cgaCfg.activity_gold_amount);
			}
			
			break;
		}
		
		case com_protocol::ClientCatchGiftActivityGoodsType::ECGA_Score:
		{
			if (actData->amountScore < cgaCfg.activity_score_amount)
			{
				actData->amountScore += randCount;
				rsp.set_amount_score(actData->amountScore);
			}
			else
			{
				OptErrorLog("score is already enough error, user = %s, amount score = %u, config score = %u", cud->connId, actData->amountScore, cgaCfg.activity_score_amount);
			}
			
			break;
		}
		
		// 金币炸弹&清屏
		case com_protocol::ClientCatchGiftActivityGoodsType::ECGA_GoldBomb:
		{
			actData->amountGold = (actData->amountGold > randCount) ? (actData->amountGold - randCount) : 0;
			rsp.set_amount_gold(actData->amountGold);
			rsp.set_reduce_gold_count(randCount);
			
			actData->amountDropGoods.clear();
			break;
		}
		
		// 时间炸弹，减少活动时间
		case com_protocol::ClientCatchGiftActivityGoodsType::ECGA_TimeBomb:
		{
			map<unsigned int, HallConfigData::CGA_RangeValue>::const_iterator rangeIt = stageTimeRange.stage_range_cfg.find(ECGARangeFlag::ECGA_StageReduceTime);
			if (rangeIt == stageTimeRange.stage_range_cfg.end()) return;
		    
			randCount = getPositiveRandNumber(rangeIt->second.min_value, rangeIt->second.max_value);
			actData->finishTimeSecs -= randCount;
			
			rsp.set_reduce_time_secs(randCount);
			
			break;
		}
		
		// 冷冻炸弹，冷冻时间
		case com_protocol::ClientCatchGiftActivityGoodsType::ECGA_FreezeBomb:
		{
			map<unsigned int, HallConfigData::CGA_RangeValue>::const_iterator rangeIt = stageTimeRange.stage_range_cfg.find(ECGARangeFlag::ECGA_StageFreezeTime);
			if (rangeIt == stageTimeRange.stage_range_cfg.end()) return;
		    
			randCount = getPositiveRandNumber(rangeIt->second.min_value, rangeIt->second.max_value);
			
			actData->freezeFinishTimeSecs = curSecs + randCount;  // 冷冻炸弹冷却结束时间点，冷却期间不掉落物品
			
			// 被冷冻了则调整掉落物品的时间点
			for (SCGADropGoodsMap::iterator gdsIt = actData->amountDropGoods.begin(); gdsIt != actData->amountDropGoods.end();)
			{
				if ((gdsIt->second.time + cgaCfg.goods_drop_secs) < curSecs)
				{
					actData->amountDropGoods.erase(gdsIt++);  // 时间已经过期则直接删除
				}
				else
				{
					gdsIt->second.time += randCount;  // 延长时间点
					++gdsIt;
				}
			}
			
			rsp.set_freeze_time_secs(randCount);
			
			break;
		}
		
		default:
		{
			return;
			break;
		}
	}
	
	rsp.set_result(Opt_Success);
	rsp.set_allocated_goods(req.release_goods());
	(actData->finishTimeSecs > curSecs) ? rsp.set_remain_secs(actData->finishTimeSecs - curSecs) : rsp.set_remain_secs(0);
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "hit catch gift activity drop goods reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_HIT_CATCH_GIFT_ACTIVITY_GOODS_RSP);

	if (rsp.remain_secs() == 0) finishActivity(cud->connId, cud->connIdLen, "hit drop goods"); // 活动结束结算
}

int CCatchGiftActivity::createCGAUserData(const char* username, const unsigned int vipLevel, const HallConfigData::CatchGiftActivityCfg& cgaCfg, SCGAUserData& cgaUserData)
{
	if (cgaCfg.activity_stage_time_range_array.empty() || cgaCfg.activity_time_secs <= 1
	    || cgaCfg.min_stage_time_secs <= 1 || cgaCfg.max_stage_time_secs > cgaCfg.activity_time_secs)
	{
		OptErrorLog("create catch gift activity data config error, username = %s", username);
		
		return CatchGiftActivityConfigError;
	}

    // 阶段时间值调整
	unsigned int dropGoldCount = 0;
	unsigned int dropGoldValue = 0;
	unsigned int dropGoldTimes = 0;
	vector<SStageTime> stageTimes;
	unsigned int stageTimeAllSecs = 0;
	cgaUserData.stageValue.clear();
	cgaUserData.goldNumbers.clear();
	for (vector<HallConfigData::ActivityStageTimeRange>::const_iterator stageIt = cgaCfg.activity_stage_time_range_array.begin(); stageIt != cgaCfg.activity_stage_time_range_array.end(); ++stageIt)
	{
		// 范围金币数量
		map<unsigned int, HallConfigData::CGA_RangeValue>::const_iterator rangeIt = stageIt->stage_range_cfg.find(ECGARangeFlag::ECGA_StageGoldCount);
		if (rangeIt == stageIt->stage_range_cfg.end())
		{
			OptErrorLog("create catch gift activity data config stage gold error, username = %s", username);

			return CatchGiftActivityConfigError;
		}
		
		const unsigned int goldRangeValue = getPositiveRandNumber(rangeIt->second.min_value, rangeIt->second.max_value);
		if (goldRangeValue == 0)
		{
			OptErrorLog("create catch gift activity data config stage gold range error, username = %s", username);
			
			return CatchGiftActivityConfigError;
		}
		
		// 本阶段发送金币掉落的次数&每次的数量
		map<unsigned int, HallConfigData::CGA_DropGoods>::const_iterator goldDropIt = stageIt->drop_goods_map.find(com_protocol::ClientCatchGiftActivityGoodsType::ECGA_Gold);
		if (goldDropIt == stageIt->drop_goods_map.end())
		{
			OptErrorLog("create catch gift activity data config drop gold error, username = %s", username);

			return CatchGiftActivityConfigError;
		}
		
		dropGoldCount = 0;
	    dropGoldValue = 0;
	    dropGoldTimes = 0;
		while (dropGoldCount < goldRangeValue)
		{
			dropGoldValue = getPositiveRandNumber(goldDropIt->second.min_count, goldDropIt->second.max_count);
			if (dropGoldValue == 0)
			{
				OptErrorLog("create catch gift activity data config drop gold count error, username = %s", username);
				
				return CatchGiftActivityConfigError;
			}
			
			dropGoldCount += dropGoldValue;
			if (dropGoldCount > goldRangeValue) dropGoldValue -= (dropGoldCount - goldRangeValue);
			
			++dropGoldTimes;
			cgaUserData.goldNumbers.push_back(dropGoldValue);
		}

	    // 范围时间长度
		rangeIt = stageIt->stage_range_cfg.find(ECGARangeFlag::ECGA_StageTime);
		if (rangeIt == stageIt->stage_range_cfg.end())
		{
			OptErrorLog("create catch gift activity data config stage time error, username = %s", username);
			
			return CatchGiftActivityConfigError;
		}
		
		const unsigned int timeRangeValue = getPositiveRandNumber(rangeIt->second.min_value, rangeIt->second.max_value);
		if (timeRangeValue == 0)
		{
			OptErrorLog("create catch gift activity data config stage time range error, username = %s", username);
			
			return CatchGiftActivityConfigError;
		}
		
		stageTimeAllSecs += timeRangeValue;
		stageTimes.push_back(SStageTime(stageTimes.size(), timeRangeValue));
		
		cgaUserData.stageValue.push_back(SCGAStageRangeValue(dropGoldTimes, 0));  // 本阶段掉落的次数
	}
	
	// 阶段时间长度误差调整
	std::sort(stageTimes.begin(), stageTimes.end(), StageTimeSort);  // 先升序排序
	unsigned int adjustTimes = (cgaCfg.activity_time_secs > stageTimeAllSecs) ? (cgaCfg.activity_time_secs - stageTimeAllSecs) : (stageTimeAllSecs - cgaCfg.activity_time_secs);
	adjustTimes *= stageTimes.size();
	unsigned int index = 0;
	while (adjustTimes > 0 && stageTimeAllSecs < cgaCfg.activity_time_secs)
	{
		--adjustTimes;
		SStageTime& stInfo = stageTimes[index++ % stageTimes.size()];  // 从最小值开始增加
		if (stInfo.timeSecs < cgaCfg.max_stage_time_secs)
		{
			++stInfo.timeSecs;
			++stageTimeAllSecs;
		}
	}
	
	index = stageTimes.size();
	while (adjustTimes > 0 && stageTimeAllSecs > cgaCfg.activity_time_secs)
	{
		--adjustTimes;
		SStageTime& stInfo = stageTimes[--index];  // 从最大值开始减少
		if (stInfo.timeSecs > cgaCfg.min_stage_time_secs)
		{
			--stInfo.timeSecs;
			--stageTimeAllSecs;
		}

		if (index == 0) index = stageTimes.size();
	}

    // 每个阶段的时间长度
	stageTimeAllSecs = 0;
	for (vector<SStageTime>::const_iterator stIt = stageTimes.begin(); stIt != stageTimes.end(); ++stIt)
	{
		cgaUserData.stageValue[stIt->index].timeSecs = stIt->timeSecs;  // 每个阶段的秒数
		stageTimeAllSecs += stIt->timeSecs;
	}
	
	if (stageTimeAllSecs != cgaCfg.activity_time_secs)
	{
		OptErrorLog("create catch gift activity data config activity time error, username = %s, stage time = %d, activity time = %d",
		username, stageTimeAllSecs, cgaCfg.activity_time_secs);

		return CatchGiftActivityConfigError;
	}
	
	// 初始化各个数值
	cgaUserData.vipLevel = vipLevel;
	cgaUserData.stageIndex = 0;
	cgaUserData.goldNumberIndex = 0;

	cgaUserData.finishTimeSecs = time(NULL) + cgaCfg.activity_time_secs;  // 活动结束时间点
	
	cgaUserData.timerId = 0;                         // 定时发送掉落物品的定时器ID
	cgaUserData.timerInterval = 0;                   // 定时器时间间隔，单位毫秒
	
	cgaUserData.dropGoldCount = 0;                   // 当前阶段已经送出去掉落的金币个数
	cgaUserData.nextDropScore = 0;                   // 下一次掉落积分券需要的金币数量点（已经掉落的金币个数满足的数量条件）
	cgaUserData.nextDropRedpacket = 0;               // 下一次掉落红包需要的金币数量点（已经掉落的金币个数满足的数量条件）
	cgaUserData.nextDropBomb = 0;                    // 下一次掉落炸弹需要的金币数量点（已经掉落的金币个数满足的数量条件）
	
	cgaUserData.amountGold = 0;                      // 获得的总金币数量
	cgaUserData.amountScore = 0;                     // 获得的总积分券数量

    cgaUserData.dropGoodsIdIndex = 0;                // 掉落物品ID的索引值
	cgaUserData.amountDropGoods.clear();             // 所有已经送出去掉落的物品列表
	
	cgaUserData.freezeFinishTimeSecs = 0;            // 冷冻炸弹冷却结束时间点，冷却期间不掉落物品
	
	// 防玩家外挂作弊行为
	cgaUserData.intervalTimeSecs = 0;                // 时间间隔点
	cgaUserData.hitCount = 0;                        // 时间间隔段内已经命中的物品个数
	
	return Opt_Success;
}

int CCatchGiftActivity::startActivity(const char* username, const HallConfigData::CatchGiftActivityCfg& cgaCfg, SCGAUserData& cgaUserData)
{
	++m_catchGiftActivityData.userIdIndex;
	int rc = initStageValue(m_catchGiftActivityData.userIdIndex, cgaCfg, cgaUserData);
	
    if (rc == Opt_Success) m_catchGiftActivityData.userIds[m_catchGiftActivityData.userIdIndex] = username;
	
	return rc;
}

int CCatchGiftActivity::initStageValue(const unsigned int userId, const HallConfigData::CatchGiftActivityCfg& cgaCfg, SCGAUserData& cgaUserData)
{
	if (cgaUserData.stageIndex >= cgaCfg.activity_stage_time_range_array.size()) return EnterCatchGiftActivityError;
	
	const map<unsigned int, HallConfigData::CGA_RangeValue>& stageRangeMap = cgaCfg.activity_stage_time_range_array.at(cgaUserData.stageIndex).stage_range_cfg;
	
	// 当前阶段已经送出去掉落的金币个数
	cgaUserData.dropGoldCount = 0;
	
	// 下一次掉落积分券需要的金币数量点（已经掉落的金币个数满足的数量条件）
	map<unsigned int, HallConfigData::CGA_RangeValue>::const_iterator rangeIt = stageRangeMap.find(ECGARangeFlag::ECGA_StageScore);
	if (rangeIt != stageRangeMap.end()) cgaUserData.nextDropScore = getPositiveRandNumber(rangeIt->second.min_value, rangeIt->second.max_value);
	
	// 下一次掉落红包需要的金币数量点（已经掉落的金币个数满足的数量条件）
	rangeIt = stageRangeMap.find(ECGARangeFlag::ECGA_StageRedPacket);
	if (rangeIt != stageRangeMap.end()) cgaUserData.nextDropRedpacket = getPositiveRandNumber(rangeIt->second.min_value, rangeIt->second.max_value);
	
	// 下一次掉落炸弹需要的金币数量点（已经掉落的金币个数满足的数量条件）
	rangeIt = stageRangeMap.find(ECGARangeFlag::ECGA_StageBomb);
	if (rangeIt != stageRangeMap.end()) cgaUserData.nextDropBomb = getPositiveRandNumber(rangeIt->second.min_value, rangeIt->second.max_value);
	
	// 启动掉落定时器
	const SCGAStageRangeValue& stageRangeValue = cgaUserData.stageValue[cgaUserData.stageIndex];
	cgaUserData.timerInterval = (stageRangeValue.timeSecs * 1000) / stageRangeValue.dropGoldTimes;  // 转换为毫秒
	cgaUserData.timerId = m_msgHandler->setTimer(cgaUserData.timerInterval, (TimerHandler)&CCatchGiftActivity::dropGoodsTimer, userId, NULL, stageRangeValue.dropGoldTimes, this);
	
	return (cgaUserData.timerId != 0) ? Opt_Success : EnterCatchGiftActivityError;
}

void CCatchGiftActivity::dropGoodsTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	if (!dropGoods(userId))
	{
		finishActivity(userId, "drop goods failed");  // 结束活动
		return;
	}

	SCGAUserData* actUserData = getUserActivityData(userId);
	const unsigned int lastStageIndex = actUserData->stageValue.size() - 1;
	if (actUserData->stageIndex >= lastStageIndex && remainCount < 1)  // 最后阶段最后一次触发
	{
		actUserData->timerId = 0;
		finishActivity(userId, "last stage finish");  // 结束活动
		return;
	}
	
	const unsigned int curSecs = time(NULL);
	if (curSecs >= actUserData->finishTimeSecs)
	{
		finishActivity(userId, "time end");  // 结束活动
		return;
	}
	
	// 剩余时间间隔不足则重新调整定时器
	const unsigned int remainMillisecond = (actUserData->finishTimeSecs - curSecs) * 1000;  // 转换为毫秒
	if (remainMillisecond < actUserData->timerInterval)
	{
		actUserData->timerInterval = remainMillisecond;
		actUserData->timerId = m_msgHandler->setTimer(actUserData->timerInterval, (TimerHandler)&CCatchGiftActivity::dropGoodsTimer, userId, NULL, 1, this);
		
		if (remainCount > 0) m_msgHandler->killTimer(timerId);
	}

	// 切换到下一阶段
	if (actUserData->stageIndex < lastStageIndex && remainCount < 1)
	{
		++actUserData->stageIndex;
		initStageValue(userId, HallConfigData::config::getConfigValue().catch_gift_activity_cfg, *actUserData);
	}
}

bool CCatchGiftActivity::dropGoods(const unsigned int userId)
{
	SCGAUserIdMap::const_iterator userIdIt = m_catchGiftActivityData.userIds.find(userId);
	if (userIdIt == m_catchGiftActivityData.userIds.end()) return false;
	
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userIdIt->second);
	if (conn == NULL)
	{
		OptErrorLog("drop goods can not find user info, user = %s", userIdIt->second.c_str());
		
		return false;
	}
	
	SCGAUserDataMap::iterator userDataIt = m_catchGiftActivityData.userData.find(userIdIt->second);
	if (userDataIt == m_catchGiftActivityData.userData.end()) return false;
	
	SCGAUserData& cgaUserData = userDataIt->second;
	const HallConfigData::CatchGiftActivityCfg& cgaCfg = HallConfigData::config::getConfigValue().catch_gift_activity_cfg;
	if (cgaUserData.stageIndex >= cgaCfg.activity_stage_time_range_array.size()) return false;
	
	const unsigned int curSecs = time(NULL);
	const unsigned int dropGoldCount = cgaUserData.goldNumbers[cgaUserData.goldNumberIndex++ % cgaUserData.goldNumbers.size()];  // 先索引递增
	if (curSecs < cgaUserData.freezeFinishTimeSecs) return true;  // 冷冻炸弹冷却结束时间点，冷却期间不掉落物品
	
	const map<unsigned int, HallConfigData::CGA_RangeValue>& stageRangeMap = cgaCfg.activity_stage_time_range_array.at(cgaUserData.stageIndex).stage_range_cfg;
	const map<unsigned int, HallConfigData::CGA_DropGoods>& dropGoodsMap = cgaCfg.activity_stage_time_range_array.at(cgaUserData.stageIndex).drop_goods_map;
	const HallConfigData::CGA_DropGoods& goldInfo = dropGoodsMap.at(com_protocol::ClientCatchGiftActivityGoodsType::ECGA_Gold);
	
	const bool isNeedGold = cgaUserData.amountGold < cgaCfg.activity_gold_amount;
	const bool isNeedScore = cgaUserData.amountScore < cgaCfg.activity_score_amount;
	if (!isNeedGold || !isNeedScore)
	{
		OptErrorLog("gold or score is already enough error, user = %s, amount gold = %u, config gold = %u, amount score = %u, config score = %u",
		userIdIt->second.c_str(), cgaUserData.amountGold, cgaCfg.activity_gold_amount, cgaUserData.amountScore, cgaCfg.activity_score_amount);
	}
	
	com_protocol::ClientCatchGiftActivityAddGoodsDropNotify dropGoodsNotify;
	com_protocol::ClientCatchGiftActivityGoods* dgInfo = NULL;
	for (unsigned int index = 0; index < dropGoldCount; ++index)
	{
		if (isNeedGold)
		{
			// 掉落金币
			dgInfo = dropGoodsNotify.add_drop_goods();
			dgInfo->set_id(++cgaUserData.dropGoodsIdIndex);
			dgInfo->set_type(com_protocol::ClientCatchGiftActivityGoodsType::ECGA_Gold);
			dgInfo->set_speed(getPositiveRandNumber(goldInfo.min_speed, goldInfo.max_speed));  // 掉落速度
			
			cgaUserData.amountDropGoods[cgaUserData.dropGoodsIdIndex] = SCGADropGoods(cgaUserData.stageIndex, com_protocol::ClientCatchGiftActivityGoodsType::ECGA_Gold, curSecs);
		}
		
		++cgaUserData.dropGoldCount;
	
		// 掉落积分券
		if (cgaUserData.dropGoldCount == cgaUserData.nextDropScore && isNeedScore)
		{
			const HallConfigData::CGA_RangeValue& stageInfo = stageRangeMap.at(ECGARangeFlag::ECGA_StageScore);
			cgaUserData.nextDropScore += getPositiveRandNumber(stageInfo.min_value, stageInfo.max_value);  // 下一次掉落需达到的金币数量
			
			const HallConfigData::CGA_DropGoods& scoreInfo = dropGoodsMap.at(com_protocol::ClientCatchGiftActivityGoodsType::ECGA_Score);
			const unsigned int dropCount = getPositiveRandNumber(scoreInfo.min_count, scoreInfo.max_count);
			for (unsigned int idx = 0; idx < dropCount; ++idx)
			{
				dgInfo = dropGoodsNotify.add_drop_goods();
				dgInfo->set_id(++cgaUserData.dropGoodsIdIndex);
				dgInfo->set_type(com_protocol::ClientCatchGiftActivityGoodsType::ECGA_Score);
				dgInfo->set_speed(getPositiveRandNumber(scoreInfo.min_speed, scoreInfo.min_speed));  // 掉落速度
				
				cgaUserData.amountDropGoods[cgaUserData.dropGoodsIdIndex] = SCGADropGoods(cgaUserData.stageIndex, com_protocol::ClientCatchGiftActivityGoodsType::ECGA_Score, curSecs);
			}
		}
		
		// 掉落红包
		if (cgaUserData.dropGoldCount == cgaUserData.nextDropRedpacket && isNeedGold)
		{
			const HallConfigData::CGA_RangeValue& stageInfo = stageRangeMap.at(ECGARangeFlag::ECGA_StageRedPacket);
			cgaUserData.nextDropRedpacket += getPositiveRandNumber(stageInfo.min_value, stageInfo.max_value);  // 下一次掉落需达到的金币数量
			
			const HallConfigData::CGA_DropGoods& redPacketInfo = dropGoodsMap.at(com_protocol::ClientCatchGiftActivityGoodsType::ECGA_RedPacket);
			const unsigned int dropCount = getPositiveRandNumber(redPacketInfo.min_count, redPacketInfo.max_count);
			for (unsigned int idx = 0; idx < dropCount; ++idx)
			{
				dgInfo = dropGoodsNotify.add_drop_goods();
				dgInfo->set_id(++cgaUserData.dropGoodsIdIndex);
				dgInfo->set_type(com_protocol::ClientCatchGiftActivityGoodsType::ECGA_RedPacket);
				dgInfo->set_speed(getPositiveRandNumber(redPacketInfo.min_speed, redPacketInfo.min_speed));  // 掉落速度
				
				cgaUserData.amountDropGoods[cgaUserData.dropGoodsIdIndex] = SCGADropGoods(cgaUserData.stageIndex, com_protocol::ClientCatchGiftActivityGoodsType::ECGA_RedPacket, curSecs);
			}
		}
		
		// 掉落炸弹
		if (cgaUserData.dropGoldCount == cgaUserData.nextDropBomb)
		{
			unsigned int allRate = 0;
			const map<unsigned int, unsigned int>& bombTypeRate = cgaCfg.activity_stage_time_range_array.at(cgaUserData.stageIndex).drop_bomb_type_rate;
			for (map<unsigned int, unsigned int>::const_iterator btrIt = bombTypeRate.begin(); btrIt != bombTypeRate.end(); ++btrIt) allRate += btrIt->second;
			
			if (allRate > 1)
			{
				const HallConfigData::CGA_RangeValue& stageInfo = stageRangeMap.at(ECGARangeFlag::ECGA_StageBomb);
			    cgaUserData.nextDropBomb += getPositiveRandNumber(stageInfo.min_value, stageInfo.max_value);  // 下一次掉落需达到的金币数量
			
				const HallConfigData::CGA_DropGoods& bombInfo = dropGoodsMap.at(com_protocol::ClientCatchGiftActivityGoodsType::ECGA_GoldBomb);
				const unsigned int dropCount = getPositiveRandNumber(bombInfo.min_count, bombInfo.max_count);
				for (unsigned int idx = 0; idx < dropCount; ++idx)
				{
					// 根据概率确定掉落的炸弹类型
					unsigned int percentValue = 0;
					const unsigned int rateValue = getPositiveRandNumber(1, allRate);
					for (map<unsigned int, unsigned int>::const_iterator btrIt = bombTypeRate.begin(); btrIt != bombTypeRate.end(); ++btrIt)
					{
						percentValue += btrIt->second;
						if (rateValue <= percentValue)
						{
							dgInfo = dropGoodsNotify.add_drop_goods();
							dgInfo->set_id(++cgaUserData.dropGoodsIdIndex);
							dgInfo->set_type((com_protocol::ClientCatchGiftActivityGoodsType)btrIt->first);
							dgInfo->set_speed(getPositiveRandNumber(bombInfo.min_speed, bombInfo.min_speed));  // 掉落速度
							
					        cgaUserData.amountDropGoods[cgaUserData.dropGoodsIdIndex] = SCGADropGoods(cgaUserData.stageIndex, btrIt->first, curSecs);
							
							break;
						}
					}
				}
			}
		}
	}
	
	if (dropGoodsNotify.drop_goods_size() > 0)
	{
	    const char* msgData = m_msgHandler->serializeMsgToBuffer(dropGoodsNotify, "catch gift activity drop goods notify");
	    if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, dropGoodsNotify.ByteSize(), LOGINSRV_CATCH_GIFT_ACTIVITY_ADD_GOODS_DROP_NOTIFY, conn);
	}
	
	return true;
}

void CCatchGiftActivity::finishActivity(unsigned int userId, const char* info)
{
	SCGAUserIdMap::iterator idIt = m_catchGiftActivityData.userIds.find(userId);
	if (idIt != m_catchGiftActivityData.userIds.end())
	{
		doFinishActivity(idIt->second.c_str(), idIt->second.length(), info);
	    clearUserData(userId);
	}
}

void CCatchGiftActivity::finishActivity(const char* username, const unsigned int uLen, const char* info)
{
	doFinishActivity(username, uLen, info);
	clearUserData(username);
}

void CCatchGiftActivity::doFinishActivity(const char* username, const unsigned int uLen, const char* info)
{
	SCGAUserData* userActData = getUserActivityData(username);
	if (userActData == NULL) return;
	
	ConnectProxy* conn = m_msgHandler->getConnectProxy(username);
	if (conn == NULL)
	{
		OptErrorLog("finish activity can not find user data, user = %s, info = %s", username, info);
		
		return;
	}

	const HallConfigData::CatchGiftActivityCfg& cgaCfg = HallConfigData::config::getConfigValue().catch_gift_activity_cfg;
	const char* promptMsg = "";
	for (map<unsigned int, string>::const_iterator promptIt = cgaCfg.settlement_prompt_msg.begin(); promptIt != cgaCfg.settlement_prompt_msg.end(); ++promptIt)
	{
		promptMsg = promptIt->second.c_str();
		if (userActData->amountGold <= promptIt->first) break;
	}
	
	// 活动结束通知
	com_protocol::ClientFinishCatchGiftActivityNotify finishNotify;
	finishNotify.set_amount_gold(userActData->amountGold);
	finishNotify.set_amount_score(userActData->amountScore);
	finishNotify.set_info(promptMsg);
	finishNotify.set_vip_level(cgaCfg.min_vip_level);
	
	unsigned int intervalSecs = cgaCfg.normal_user_time_interval;
	if (userActData->vipLevel >= cgaCfg.min_vip_level)
	{
		intervalSecs = cgaCfg.vip_user_time_interval;
		finishNotify.set_double_gift(1);
		
		// vip 翻倍
		userActData->amountGold *= 2;
		userActData->amountScore *= 2;
	}
	
	finishNotify.set_next_remain_secs(intervalSecs);
	
	// 活动结算获得的金币&积分券
	NProject::RecordItemVector recordItemVector;
	if (userActData->amountGold > 0) recordItemVector.push_back(RecordItem(EPropType::PropGold, userActData->amountGold));
	if (userActData->amountScore > 0) recordItemVector.push_back(RecordItem(EPropType::PropScores, userActData->amountScore));
	if (!recordItemVector.empty()) m_msgHandler->notifyDbProxyChangeProp(username, uLen, recordItemVector, EPropFlag::CatchGiftActivitySettlement, "福袋活动获得奖励");
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(finishNotify, "finish catch gift activity notify");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, finishNotify.ByteSize(), LOGINSRV_FINISH_CATCH_GIFT_ACTIVITY_NOTIFY, conn);
	
	OptInfoLog("finish catch gift activity, info = %s, user = %s, vip = %u, gold = %u, score = %u",
	info, username, userActData->vipLevel, userActData->amountGold, userActData->amountScore);
}

SCGAUserData* CCatchGiftActivity::getUserActivityData(const char* username)
{
	SCGAUserDataMap::iterator userDataIt = m_catchGiftActivityData.userData.find(username);
	return (userDataIt != m_catchGiftActivityData.userData.end()) ? &userDataIt->second : NULL;
}

SCGAUserData* CCatchGiftActivity::getUserActivityData(const unsigned int userId)
{
	SCGAUserIdMap::const_iterator userIdIt = m_catchGiftActivityData.userIds.find(userId);
	if (userIdIt == m_catchGiftActivityData.userIds.end()) return NULL;
	
	SCGAUserDataMap::iterator userDataIt = m_catchGiftActivityData.userData.find(userIdIt->second);
	return (userDataIt != m_catchGiftActivityData.userData.end()) ? &userDataIt->second : NULL;
}

// 清空数据
void CCatchGiftActivity::clearUserData(const char* username)
{
	SCGAUserDataMap::iterator userDataIt = m_catchGiftActivityData.userData.find(username);
	if (userDataIt != m_catchGiftActivityData.userData.end())
	{
		if (userDataIt->second.timerId != 0) m_msgHandler->killTimer(userDataIt->second.timerId);
		m_catchGiftActivityData.userData.erase(userDataIt);
		
		for (SCGAUserIdMap::iterator idIt = m_catchGiftActivityData.userIds.begin(); idIt != m_catchGiftActivityData.userIds.end(); ++idIt)
		{
			if (idIt->second == username)
			{
				m_catchGiftActivityData.userIds.erase(idIt);
				break;
			}
		}
	}
}

// 清空数据
void CCatchGiftActivity::clearUserData(const unsigned int userId)
{
	SCGAUserIdMap::iterator idIt = m_catchGiftActivityData.userIds.find(userId);
	if (idIt != m_catchGiftActivityData.userIds.end())
	{
		SCGAUserDataMap::iterator userDataIt = m_catchGiftActivityData.userData.find(idIt->second);
		if (userDataIt != m_catchGiftActivityData.userData.end())
		{
			if (userDataIt->second.timerId != 0) m_msgHandler->killTimer(userDataIt->second.timerId);
			m_catchGiftActivityData.userData.erase(userDataIt);
		}
		
		m_catchGiftActivityData.userIds.erase(idIt);
	}
}

}

