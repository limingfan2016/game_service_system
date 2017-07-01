
/* author : limingfan
 * date : 2017.02.15
 * description : 红包活动
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "CLoginSrv.h"
#include "CRedPacketActivity.h"
#include "base/Function.h"
#include "../common/CommonType.h"
#include "_HallConfigData_.h"


using namespace NProject;


namespace NPlatformService
{

CRedPacketActivity::CRedPacketActivity()
{
	m_msgHandler = NULL;
}

CRedPacketActivity::~CRedPacketActivity()
{
	m_msgHandler = NULL;
}

void CRedPacketActivity::load(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;

    m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_RED_PACKET_ACTIVITY_TIME_REQ, (ProtocolHandler)&CRedPacketActivity::getRedPacketActivityTimeInfo, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_OPEN_RED_PACKET_ACTIVITY_REQ, (ProtocolHandler)&CRedPacketActivity::openRedPacketActivity, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_CLOSE_RED_PACKET_ACTIVITY_NOTIFY, (ProtocolHandler)&CRedPacketActivity::closeRedPacketActivity, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_USER_CLICK_RED_PACKET_REQ, (ProtocolHandler)&CRedPacketActivity::userClickRedPacket, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_VIEW_RED_PACKET_HISTORY_LIST_REQ, (ProtocolHandler)&CRedPacketActivity::viewRedPacketHistoryList, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_VIEW_RED_PACKET_DETAILED_HISTORY_REQ, (ProtocolHandler)&CRedPacketActivity::viewRedPacketDetailedHistory, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_VIEW_PERSONAL_RED_PACKET_HISTORY_REQ, (ProtocolHandler)&CRedPacketActivity::viewPersonalRedPacketHistory, this);
	
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_ACTIVITY_RED_PACKET_RSP, (ProtocolHandler)&CRedPacketActivity::getActivityRedPacketReply, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_ADD_ACTIVITY_RED_PACKET_RSP, (ProtocolHandler)&CRedPacketActivity::userClickRedPacketReply, this);
	
	// 获取红包活动数据
	m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, NULL, 0, BUSINESS_GET_ACTIVITY_RED_PACKET_REQ, "", 0);
}

void CRedPacketActivity::unLoad()
{

}


void CRedPacketActivity::modifyUserNickname(const char* username, const com_protocol::ModifyBaseinfoRsp& mdRsp)
{
	if (username == NULL || mdRsp.result() != Opt_Success || !mdRsp.has_base_info_rsp()) return;
	
	UserIdToInfoMap::iterator userIt = m_redPacketActivityData.winRedPacketUserId2Infos.find(username);
	if (userIt == m_redPacketActivityData.winRedPacketUserId2Infos.end()) return;
	
	if (mdRsp.base_info_rsp().has_nickname()) userIt->second.nickname = mdRsp.base_info_rsp().nickname();  // 更新用户昵称
	if (mdRsp.base_info_rsp().has_portrait_id()) userIt->second.portraitId = mdRsp.base_info_rsp().portrait_id();  // 更新用户头像ID
}

// 获取活动红包数据信息
void CRedPacketActivity::getActivityRedPacketReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->parseMsgFromBuffer(m_redPacketActivityData.redPacketActivityData, data, len, "get red packet activity data reply"))
	{
		m_redPacketActivityData.redPacketActivityData.Clear();
		return;
	}

    // 计算每个活动发放的总物品数据
	for (int activityIdx = 0; activityIdx < m_redPacketActivityData.redPacketActivityData.activitys_size(); ++activityIdx)
	{
		com_protocol::RedPacketActivityInfo* redPacketActivityInfo = m_redPacketActivityData.redPacketActivityData.mutable_activitys(activityIdx);
		const unsigned int activityId = redPacketActivityInfo->activity_id();
		for (int recordIdx = 0; recordIdx < m_redPacketActivityData.redPacketActivityData.win_records_size(); ++recordIdx)
		{
			const com_protocol::WinRedPacketActivityRecord& redPacketRecord = m_redPacketActivityData.redPacketActivityData.win_records(recordIdx);
			if (activityId == redPacketRecord.activity_id())
			{
				com_protocol::GiftInfo* giftAmount = NULL;
				const unsigned int giftType = redPacketRecord.gift().type();
				for (int idx = 0; idx < redPacketActivityInfo->gift_amount_size(); ++idx)
				{
					if (redPacketActivityInfo->gift_amount(idx).type() == giftType)
					{
						giftAmount = redPacketActivityInfo->mutable_gift_amount(idx);
						break;
					}
				}
				
				if (giftAmount == NULL)
				{
					giftAmount = redPacketActivityInfo->add_gift_amount();
					giftAmount->set_type(giftType);
					giftAmount->set_num(0);
				}
				giftAmount->set_num(giftAmount->num() + redPacketRecord.gift().num());
			}
		}
	}
	
	// 参与抢红包活动的玩家
	for (int idx = 0; idx < m_redPacketActivityData.redPacketActivityData.user_infos_size(); ++idx)
	{
		const com_protocol::WinRedPacketUserInfo& userInfo = m_redPacketActivityData.redPacketActivityData.user_infos(idx);
		m_redPacketActivityData.winRedPacketUserId2Infos[userInfo.username()] = SRedPacketUserInfo(userInfo.nickname(), userInfo.portrait_id());
	}
	m_redPacketActivityData.redPacketActivityData.clear_user_infos();
}

// 获取抢红包活动时间信息
void CRedPacketActivity::getRedPacketActivityTimeInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get red packet activity time info")) return;
	
	const time_t curSecs = time(NULL);
	m_redPacketActivityData.update(curSecs);  // 先刷新数据
	
	com_protocol::ClientOpenWinRedPacketActivityRsp rsp;
	unsigned int beginSecs = 0;
	unsigned int endSecs = 0;
	const HallConfigData::RedPacketActivityInfo* currentRedPacketInfo = getActivityData(curSecs, beginSecs, endSecs);
	if (currentRedPacketInfo != NULL)
	{
		rsp.set_id(beginSecs);
	    rsp.set_name(currentRedPacketInfo->activity_name);
		
		(curSecs < beginSecs) ? rsp.set_remain_secs(beginSecs - curSecs) : rsp.set_remain_secs(0);
		rsp.set_begin_secs(beginSecs);
		rsp.set_end_secs(endSecs);
	}
	else
	{
		rsp.set_remain_secs(-1);
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "get win red packet activity time info reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_GET_RED_PACKET_ACTIVITY_TIME_RSP);
}

// 打开抢红包活动UI界面
void CRedPacketActivity::openRedPacketActivity(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to open red packet activity")) return;
	
	const time_t curSecs = time(NULL);
	m_redPacketActivityData.update(curSecs);  // 先刷新数据
	
	com_protocol::ClientOpenWinRedPacketActivityRsp rsp;
	rsp.set_rule(HallConfigData::config::getConfigValue().win_red_packet_base_cfg.rule);
	
	unsigned int beginSecs = 0;
	unsigned int endSecs = 0;
	const HallConfigData::RedPacketActivityInfo* currentRedPacketInfo = getActivityData(curSecs, beginSecs, endSecs);
	if (currentRedPacketInfo != NULL)
	{
		m_redPacketActivityData.activityBeginTimeSecs = beginSecs;
	    m_redPacketActivityData.activityEndTimeSecs = endSecs;
	
		rsp.set_id(m_redPacketActivityData.activityBeginTimeSecs);
	    rsp.set_name(currentRedPacketInfo->activity_name);
		(curSecs < m_redPacketActivityData.activityBeginTimeSecs) ? rsp.set_remain_secs(m_redPacketActivityData.activityBeginTimeSecs - curSecs) : rsp.set_remain_secs(0);
		rsp.set_begin_secs(beginSecs);
		rsp.set_end_secs(endSecs);

		if (currentRedPacketInfo->payment_type > 0 && currentRedPacketInfo->payment_count > 0)
		{
		    com_protocol::GiftInfo* payGoods = rsp.mutable_payment_goods();
		    payGoods->set_type(currentRedPacketInfo->payment_type);
		    payGoods->set_num(currentRedPacketInfo->payment_count);
		}
	}
	else
	{
		rsp.set_remain_secs(-1);
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "open win red packet activity reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_OPEN_RED_PACKET_ACTIVITY_RSP);
}

// 调用了打开抢红包活动UI界面之后
// 玩家关闭UI页面是必须调用关闭活动消息接口通知服务器
void CRedPacketActivity::closeRedPacketActivity(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// now not use, to do nothing
}

// 用户点击开抢红包
void CRedPacketActivity::userClickRedPacket(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to click red packet")) return;
	
	com_protocol::ClientUserClickRedPacketReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "user click win red packet")) return;
	
	com_protocol::ClientUserClickRedPacketRsp rsp;
	rsp.set_result(Opt_Success);
	const time_t curSecs = time(NULL);
	m_redPacketActivityData.update(curSecs);  // 先刷新数据
	
	const unsigned int activityId = req.activity_id();
	ConnectUserData* cud = m_msgHandler->getConnectUserData();
	const HallConfigData::RedPacketActivityInfo* rpactCfgInfo = NULL;
	
	if (m_redPacketActivityData.onReceiveRedPacketUsers.find(cud->connId) != m_redPacketActivityData.onReceiveRedPacketUsers.end())
	{
		rsp.set_result(WinRedPacketActivityOnReceive);
	}
	else if (curSecs < m_redPacketActivityData.activityBeginTimeSecs)  // 活动还没开始
	{
		rsp.set_result(WinRedPacketActivityNotBegin);
		rsp.set_remain_secs(m_redPacketActivityData.activityBeginTimeSecs - curSecs);
	}
	else if (curSecs >= m_redPacketActivityData.activityEndTimeSecs)  // 活动已经结束了
	{
		rsp.set_result(WinRedPacketActivityAlreadyEnd);
	}
	else if (activityId != m_redPacketActivityData.activityBeginTimeSecs)
	{
		rsp.set_result(NotFindWinRedPacketActivity);  // ID不匹配
		for (int idx = m_redPacketActivityData.redPacketActivityData.activitys_size() - 1; idx >= 0; --idx)
		{
			if (activityId == m_redPacketActivityData.redPacketActivityData.activitys(idx).activity_id())
			{
				rsp.set_result(WinRedPacketActivityFinish);  // 本次活动的红包被抢没了
				break;
			}
		}
	}
	else if (getActivityStatus(activityId) == ERedPacketActivityStatus::ERedPacketActivityOff)  // 本次活动的红包被抢没了
	{
		rsp.set_result(WinRedPacketActivityFinish);
	}
	else
	{
		// 校验配置信息
		char curTimeStr[32] = {0};
	    getCurrentDate(curTimeStr, sizeof(curTimeStr), "%d-%02d-%02d");  // 当前日期
		const map<string, HallConfigData::RedPacketActivityDate>& redPacketActivityDate = HallConfigData::config::getConfigValue().win_red_packet_activity.red_packet_activity_date_map;
		map<string, HallConfigData::RedPacketActivityDate>::const_iterator actDateIt = redPacketActivityDate.find(curTimeStr);
		if (actDateIt != redPacketActivityDate.end())
		{
			struct tm activityTime;
			for (vector<HallConfigData::RedPacketActivityInfo>::const_iterator timeIt = actDateIt->second.red_packet_activity_time_array.begin(); timeIt != actDateIt->second.red_packet_activity_time_array.end(); ++timeIt)
			{
				snprintf(curTimeStr, sizeof(curTimeStr) - 1, "%s %s", actDateIt->first.c_str(), timeIt->start.c_str());
				if (strptime(curTimeStr, "%Y-%m-%d %H:%M:%S", &activityTime) != NULL && (unsigned int)mktime(&activityTime) == activityId)
				{
					rpactCfgInfo = &(*timeIt);
					break;
				}
			}
		}
		
		if (rpactCfgInfo == NULL) rsp.set_result(NotFindWinRedPacketActivity);  // ID不匹配
	}
	
	// 如果是第一个点击抢红包，则生成红包信息
	if (rsp.result() == Opt_Success && !generateRedPackets(activityId, rpactCfgInfo, m_redPacketActivityData))
	{
		rsp.set_result(WinRedPacketActivityCfgError);
	}
	
	// 成功则开抢红包
	const string* redPacketId = NULL;
	if (rsp.result() == Opt_Success)
	{
		redPacketId = winRedPacket(m_redPacketActivityData.redPackets);
		if (redPacketId == NULL) rsp.set_result(WinRedPacketActivityFinish);
	}

    int rc = Opt_Failed;
	if (rsp.result() != Opt_Success)
	{
		// 检查看是不是活动已经结束了
		unsigned int beginSecs = 0;
		unsigned int endSecs = 0;
		const HallConfigData::RedPacketActivityInfo* redPacketActivityCfgInfo = NULL;
		if ((redPacketActivityCfgInfo = getActivityData(curSecs, beginSecs, endSecs)) != NULL)
		{
			if (curSecs < beginSecs)  // 活动还没开始
			{
				m_redPacketActivityData.activityBeginTimeSecs = beginSecs;
				m_redPacketActivityData.activityEndTimeSecs = endSecs;
			
				rsp.set_remain_secs(beginSecs - curSecs);
				rsp.set_next_remain_secs(beginSecs - curSecs);
				rsp.set_next_activity_id(beginSecs);
				rsp.set_next_activity_name(redPacketActivityCfgInfo->activity_name);
				rsp.set_next_begin_secs(beginSecs);
			}
			else if (curSecs <= endSecs && (redPacketActivityCfgInfo = getActivityData(endSecs + 1, beginSecs, endSecs)) != NULL)  // 活动时间还没结束则取下一次的活动时间
			{
				rsp.set_next_remain_secs(beginSecs - curSecs);
				rsp.set_next_activity_id(beginSecs);
				rsp.set_next_activity_name(redPacketActivityCfgInfo->activity_name);
				rsp.set_next_begin_secs(beginSecs);
			}
		}
	
		if (rsp.result() == WinRedPacketActivityFinish && !getWinRedPacketInfo(activityId, *rsp.mutable_record_info())) rsp.clear_record_info();
		
		const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "user click win red packet direct reply");
	    if (msgData != NULL) rc = m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_USER_CLICK_RED_PACKET_RSP);
	}
	else
	{
		// 抢红包消息
		com_protocol::AddActivityRedPacketReq addRedPacketReq;
		int activityIndex = getWinRedPacketInfoIndex(activityId);
		if (activityIndex < 0)
		{
			com_protocol::RedPacketActivityInfo* rpaInfo = addRedPacketReq.mutable_activity_info();
			rpaInfo->set_activity_id(activityId);
			rpaInfo->set_activity_name(rpactCfgInfo->activity_name);
			rpaInfo->set_red_packet_name(rpactCfgInfo->red_packet_name);
			rpaInfo->set_blessing_words(rpactCfgInfo->blessing_words);
			rpaInfo->set_red_packet_amount(m_redPacketActivityData.redPackets.size());
			
			if (rpactCfgInfo->payment_type > 0 && rpactCfgInfo->payment_count > 0)
			{
				com_protocol::GiftInfo* payGoods = rpaInfo->mutable_payment_goods();
				payGoods->set_type(rpactCfgInfo->payment_type);
				payGoods->set_num(rpactCfgInfo->payment_count);
			}
		}
		
		// 抢到的红包信息
		SRedPacketInfo& redPacketInfo = m_redPacketActivityData.redPackets[*redPacketId];
		com_protocol::WinRedPacketActivityRecord* wrpaRecord = addRedPacketReq.mutable_record_info();
		wrpaRecord->set_activity_id(activityId);
		wrpaRecord->set_red_packet_id(*redPacketId);
		wrpaRecord->set_receive_username(cud->connId);
		wrpaRecord->set_receive_time(time(NULL));
		
		com_protocol::GiftInfo* redPacketGift = wrpaRecord->mutable_gift();
		redPacketGift->set_type(redPacketInfo.type);
		redPacketGift->set_num(redPacketInfo.count);
		
		if (redPacketInfo.status == ERedPacketStatus::EOnReceiveRedPacket)  // 同时被抢的红包
		{
			char activityIdStr[32] = {0};
			snprintf(activityIdStr, sizeof(activityIdStr) - 1, "%u-%u", activityId, ++m_redPacketActivityData.winRedPacketIndex);
			wrpaRecord->set_red_packet_id_ex(activityIdStr);
		}
		
		if (rpactCfgInfo->payment_type > 0 && rpactCfgInfo->payment_count > 0)
		{
			com_protocol::GiftInfo* payGoods = wrpaRecord->mutable_payment_goods();
			payGoods->set_type(rpactCfgInfo->payment_type);
			payGoods->set_num(rpactCfgInfo->payment_count);
		}
		
		unsigned int redPacketGoldCount = redPacketInfo.count;
		if (redPacketInfo.type == EPropType::PropPhoneFareValue)
		{
			redPacketGoldCount *= HallConfigData::config::getConfigValue().win_red_packet_base_cfg.one_phone_fare_to_gold;
		}
		else if (redPacketInfo.type == EPropType::PropScores)
		{
			redPacketGoldCount /= HallConfigData::config::getConfigValue().win_red_packet_base_cfg.many_scores_to_one_gold;
		}
		
		if (redPacketGoldCount > m_redPacketActivityData.bestGoldCount)
		{
			m_redPacketActivityData.bestGoldCount = redPacketGoldCount;
			wrpaRecord->set_is_best(1);
		}

		const char* msgData = m_msgHandler->serializeMsgToBuffer(addRedPacketReq, "add red packet request");
		if (msgData != NULL) rc = m_msgHandler->sendMessageToCommonDbProxy(msgData, addRedPacketReq.ByteSize(), BUSINESS_ADD_ACTIVITY_RED_PACKET_REQ);
		if (rc == Opt_Success)
		{
			m_redPacketActivityData.onReceiveRedPacketUsers[cud->connId] = 0;
			redPacketInfo.status = ERedPacketStatus::EOnReceiveRedPacket;  // 标志为正在领取中
		}
	}
	
	OptInfoLog("user click win red packet request, user = %s, activity id = %u, red packet id = %s, result = %d, rc = %d",
	cud->connId, activityId, (redPacketId != NULL) ? redPacketId->c_str() : "", rsp.result(), rc);
}

void CRedPacketActivity::userClickRedPacketReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::AddActivityRedPacketRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "user click win red packet reply")) return;
	
	com_protocol::ClientUserClickRedPacketRsp clientRsp;
	clientRsp.set_result(rsp.result());
	
	const com_protocol::AddActivityRedPacketReq& req = rsp.red_packet_info();
	if (rsp.result() == 0)  // 成功
	{
		// 还没有活动信息则新创建
		if (req.has_activity_info() && getWinRedPacketInfoIndex(req.activity_info().activity_id()) < 0)
		{
			com_protocol::RedPacketActivityInfo* rpactInfo = m_redPacketActivityData.redPacketActivityData.add_activitys();
			*rpactInfo = req.activity_info();
		}
		
		const int activityIndex = getWinRedPacketInfoIndex(req.record_info().activity_id());
		if (activityIndex < 0)
		{
			OptErrorLog("user click win red packet reply error, can not find the activity data, user = %s, activity id = %u, red packet id = %s",
	        m_msgHandler->getContext().userData, req.record_info().activity_id(), req.record_info().red_packet_id().c_str());
			
			return;
		}
		
		// 添加活动总物品信息
		com_protocol::GiftInfo* giftAmount = NULL;
		com_protocol::RedPacketActivityInfo* rpactInfo = m_redPacketActivityData.redPacketActivityData.mutable_activitys(activityIndex);
		const unsigned int giftType = req.record_info().gift().type();
		for (int idx = 0; idx < rpactInfo->gift_amount_size(); ++idx)
		{
			if (rpactInfo->gift_amount(idx).type() == giftType)
			{
				giftAmount = rpactInfo->mutable_gift_amount(idx);
				break;
			}
		}
		
		if (giftAmount == NULL)
		{
			giftAmount = rpactInfo->add_gift_amount();
			giftAmount->set_type(giftType);
			giftAmount->set_num(0);
		}
		giftAmount->set_num(giftAmount->num() + req.record_info().gift().num());
		
		// 最佳手气
		if (req.record_info().is_best() == 1) rpactInfo->set_best_username(req.record_info().receive_username());
		
		// 新增加用户抢红包成功记录
		com_protocol::WinRedPacketActivityRecord* wrpaRecord = m_redPacketActivityData.redPacketActivityData.add_win_records();
		*wrpaRecord = req.record_info();
		
		// 刷新用户信息
		SRedPacketUserInfo& redPacketUserInfo = m_redPacketActivityData.winRedPacketUserId2Infos[req.record_info().receive_username()];
		redPacketUserInfo.nickname = req.record_info().receive_nickname();
		redPacketUserInfo.portraitId = req.record_info().portrait_id();
		
		// 该红包标志为已经被领取了
		RedPacketIdToInfoMap::iterator redPacketIt = m_redPacketActivityData.redPackets.find(req.record_info().red_packet_id());
		if (redPacketIt != m_redPacketActivityData.redPackets.end()) redPacketIt->second.status = ERedPacketStatus::ESuccessReceivedRedPacket;

		// 结束了则重置活动数据
		if (getActivityStatus(req.record_info().activity_id()) == ERedPacketActivityStatus::ERedPacketActivityOff)
		{
			m_redPacketActivityData.reset(req.record_info().activity_id());
		}
		
		// 用户成功抢到的红包物品
		com_protocol::GiftInfo* receivedGift = clientRsp.mutable_received_red_packet();
		*receivedGift = req.record_info().gift();
	}
	
	else if (rsp.result() == 1)  // 表示红包被抢完了
	{
		clientRsp.set_result(WinRedPacketActivityFinish);
	}

    int rc = Opt_Failed;
	string userName;
	ConnectProxy* conn = m_msgHandler->getConnectInfo("user click win red packet reply", userName);
	if (conn != NULL)
	{
	    if (clientRsp.result() == 0 || clientRsp.result() == WinRedPacketActivityFinish)
		{
		    // 检查看是不是活动已经结束了
			const time_t curSecs = time(NULL);
	        m_redPacketActivityData.update(curSecs);  // 先刷新数据
			unsigned int beginSecs = 0;
			unsigned int endSecs = 0;
			const HallConfigData::RedPacketActivityInfo* redPacketActivityCfgInfo = NULL;
			if ((redPacketActivityCfgInfo = getActivityData(curSecs, beginSecs, endSecs)) != NULL)
			{
				if (curSecs < beginSecs)  // 活动还没开始
				{
					m_redPacketActivityData.activityBeginTimeSecs = beginSecs;
					m_redPacketActivityData.activityEndTimeSecs = endSecs;
				
					clientRsp.set_remain_secs(beginSecs - curSecs);
					clientRsp.set_next_remain_secs(beginSecs - curSecs);
					clientRsp.set_next_activity_id(beginSecs);
					clientRsp.set_next_activity_name(redPacketActivityCfgInfo->activity_name);
					clientRsp.set_next_begin_secs(beginSecs);
				}
				else if (curSecs <= endSecs && (redPacketActivityCfgInfo = getActivityData(endSecs + 1, beginSecs, endSecs)) != NULL)  // 活动时间还没结束则取下一次的活动时间
				{
					clientRsp.set_next_remain_secs(beginSecs - curSecs);
					clientRsp.set_next_activity_id(beginSecs);
					clientRsp.set_next_activity_name(redPacketActivityCfgInfo->activity_name);
					clientRsp.set_next_begin_secs(beginSecs);
				}
			}
			
			if (!getWinRedPacketInfo(req.record_info().activity_id(), *clientRsp.mutable_record_info())) clientRsp.clear_record_info();
			
			if (req.record_info().has_payment_goods()) *(clientRsp.mutable_payment_goods()) = req.record_info().payment_goods();
		}
		
		const char* msgData = m_msgHandler->serializeMsgToBuffer(clientRsp, "user click win red packet reply");
	    if (msgData != NULL) rc = m_msgHandler->sendMsgToProxy(msgData, clientRsp.ByteSize(), LOGINSRV_USER_CLICK_RED_PACKET_RSP, conn);
	}
	
	OptInfoLog("user click win red packet reply, user = %s, activity id = %u, red packet id = %s, result = %d, rc = %d",
	m_msgHandler->getContext().userData, req.record_info().activity_id(), req.record_info().red_packet_id().c_str(), clientRsp.result(), rc);
}

// 查看红包历史记录列表
void CRedPacketActivity::viewRedPacketHistoryList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to view red packet history list")) return;
	
	com_protocol::ClientViewRedPacketHistoryListRsp rsp;
	const google::protobuf::RepeatedPtrField<com_protocol::RedPacketActivityInfo>& activityList = m_redPacketActivityData.redPacketActivityData.activitys();
	for (google::protobuf::RepeatedPtrField<com_protocol::RedPacketActivityInfo>::const_iterator it = activityList.begin(); it != activityList.end(); ++it)
	{
		com_protocol::ClientRedPacketHistory* cltRPHistory = rsp.add_history_list();
		cltRPHistory->set_activity_id(it->activity_id());
		cltRPHistory->set_red_packet_name(it->red_packet_name());
		cltRPHistory->set_time_secs(it->activity_id());
		
		google::protobuf::RepeatedPtrField<com_protocol::GiftInfo >* receivedGift = cltRPHistory->mutable_gifts();
		*receivedGift = it->gift_amount();
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "view red packet history list reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_VIEW_RED_PACKET_HISTORY_LIST_RSP);
}

// 查看单个红包详细的历史记录信息
void CRedPacketActivity::viewRedPacketDetailedHistory(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to view red packet detailed history")) return;
	
	com_protocol::ClientViewRedPacketDetailedHistoryReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "view red packet detailed history request")) return;
	
	com_protocol::ClientViewRedPacketDetailedHistoryRsp rsp;
	rsp.set_result(Opt_Success);
	if (!getWinRedPacketInfo(req.activity_id(), *rsp.mutable_record_info()))
	{
		rsp.set_result(NotFindWinRedPacketActivity);
		rsp.clear_record_info();
	}

	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "view red packet detailed history reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_VIEW_RED_PACKET_DETAILED_HISTORY_RSP);
}

// 查看个人红包历史记录
void CRedPacketActivity::viewPersonalRedPacketHistory(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to view personal red packet history")) return;
	
	com_protocol::ClientViewPersonalRedPacketHistoryRsp rsp;
	rsp.set_result(Opt_Success);
	
	const string username = m_msgHandler->getConnectUserData()->connId;
	unsigned int redPacketAmount = 0;
	const google::protobuf::RepeatedPtrField<com_protocol::WinRedPacketActivityRecord>& redPacketRecords = m_redPacketActivityData.redPacketActivityData.win_records();
	for (google::protobuf::RepeatedPtrField<com_protocol::WinRedPacketActivityRecord>::const_iterator it = redPacketRecords.begin(); it != redPacketRecords.end(); ++it)
	{
		if (it->receive_username() != username) continue;
		
		const com_protocol::RedPacketActivityInfo* rpaInfo = getWinRedPacketActivity(it->activity_id());
		if (rpaInfo == NULL) continue;
		
		com_protocol::ClientPersonalRedPacketHistory* personalHistory = rsp.add_history();
		personalHistory->set_red_packet_name(rpaInfo->red_packet_name());
		personalHistory->set_blessing_words(rpaInfo->blessing_words());
		personalHistory->set_time_secs(it->receive_time());
		
		com_protocol::GiftInfo* receivedGift = personalHistory->mutable_received_gifts();
		*receivedGift = it->gift();
		
		if (rpaInfo->best_username() == username) personalHistory->set_is_best(1);

        // 物品总数量
		com_protocol::GiftInfo* giftAmount = NULL;
		const unsigned int giftType = receivedGift->type();
		for (int idx = 0; idx < rsp.amount_gifts_size(); ++idx)
		{
			if (rsp.amount_gifts(idx).type() == giftType)
			{
				giftAmount = rsp.mutable_amount_gifts(idx);
				break;
			}
		}
		
		if (giftAmount == NULL)
		{
			giftAmount = rsp.add_amount_gifts();
			giftAmount->set_type(giftType);
			giftAmount->set_num(0);
		}
		giftAmount->set_num(giftAmount->num() + receivedGift->num());

		++redPacketAmount;
	}
	
	rsp.set_red_packet_amount(redPacketAmount);
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "view personal red packet history reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_VIEW_PERSONAL_RED_PACKET_HISTORY_RSP);
}

// 获取当前活动或者下一次活动信息
const HallConfigData::RedPacketActivityInfo* CRedPacketActivity::getActivityData(const unsigned int curSecs, unsigned int& beginSecs, unsigned int& endSecs)
{
	// 各个时间段的活动
	char curTimeStr[32] = {0};
	getCurrentDate(curTimeStr, sizeof(curTimeStr), "%d-%02d-%02d");  // 当前日期
	struct tm activityTime;
	beginSecs = 0;
	endSecs = 0;

	const map<string, HallConfigData::RedPacketActivityDate>& redPacketActivityDate = HallConfigData::config::getConfigValue().win_red_packet_activity.red_packet_activity_date_map;
	for (map<string, HallConfigData::RedPacketActivityDate>::const_iterator dateIt = redPacketActivityDate.begin(); dateIt != redPacketActivityDate.end(); ++dateIt)
	{
		if (dateIt->first < curTimeStr) continue;  // 已经过期了
		
		if (dateIt->first == curTimeStr)  // 当天的活动
		{
			for (vector<HallConfigData::RedPacketActivityInfo>::const_iterator timeIt = dateIt->second.red_packet_activity_time_array.begin(); timeIt != dateIt->second.red_packet_activity_time_array.end(); ++timeIt)
			{
				snprintf(curTimeStr, sizeof(curTimeStr) - 1, "%s %s", dateIt->first.c_str(), timeIt->finish.c_str());
				if (strptime(curTimeStr, "%Y-%m-%d %H:%M:%S", &activityTime) != NULL) endSecs = mktime(&activityTime);
				
				if (curSecs >= endSecs) continue; // 已经结束了的活动
				
				snprintf(curTimeStr, sizeof(curTimeStr) - 1, "%s %s", dateIt->first.c_str(), timeIt->start.c_str());
				if (strptime(curTimeStr, "%Y-%m-%d %H:%M:%S", &activityTime) != NULL) beginSecs = mktime(&activityTime);

				if ((curSecs >= beginSecs && getActivityStatus(beginSecs) != ERedPacketActivityStatus::ERedPacketActivityOff)  // 活动进行中
					|| (curSecs < beginSecs && endSecs > beginSecs))  // 还未开始活动
				{
					return &(*timeIt);  // 找到合适的活动信息了
				}
			}
		}
		
	    else if (dateIt->second.red_packet_activity_time_array.begin() != dateIt->second.red_packet_activity_time_array.end())  // 下一次的活动，可能是相隔了N天的
		{
			vector<HallConfigData::RedPacketActivityInfo>::const_iterator timeIt = dateIt->second.red_packet_activity_time_array.begin();	
			snprintf(curTimeStr, sizeof(curTimeStr) - 1, "%s %s", dateIt->first.c_str(), timeIt->start.c_str());
			if (strptime(curTimeStr, "%Y-%m-%d %H:%M:%S", &activityTime) != NULL) beginSecs = mktime(&activityTime);
			
			snprintf(curTimeStr, sizeof(curTimeStr) - 1, "%s %s", dateIt->first.c_str(), timeIt->finish.c_str());
			if (strptime(curTimeStr, "%Y-%m-%d %H:%M:%S", &activityTime) != NULL) endSecs = mktime(&activityTime);
			
			if (curSecs < beginSecs && endSecs > beginSecs) return &(*timeIt);  // 确认活动还没开始，找到合适的活动信息了
		}
	}
	
	return NULL;
}

bool CRedPacketActivity::getWinRedPacketInfo(const unsigned int activityId, com_protocol::ClientWinRedPacketInfo& info)
{
	for (int activityIdx = m_redPacketActivityData.redPacketActivityData.activitys_size() - 1; activityIdx >= 0; --activityIdx)
	{
		const com_protocol::RedPacketActivityInfo& redPacketActivityInfo = m_redPacketActivityData.redPacketActivityData.activitys(activityIdx);
		if (activityId == redPacketActivityInfo.activity_id())
		{
			info.set_red_packet_name(redPacketActivityInfo.red_packet_name());
			info.set_blessing_words(redPacketActivityInfo.blessing_words());
			info.set_red_packet_amount(redPacketActivityInfo.red_packet_amount());
			*(info.mutable_received_gifts()) = redPacketActivityInfo.gift_amount();
			
			unsigned int receivedCount = 0;
			info.set_best_index(0);
			for (int recordIdx = 0; recordIdx < m_redPacketActivityData.redPacketActivityData.win_records_size(); ++recordIdx)
			{
				const com_protocol::WinRedPacketActivityRecord& redPacketRecord = m_redPacketActivityData.redPacketActivityData.win_records(recordIdx);
				if (activityId == redPacketRecord.activity_id())
				{
					++receivedCount;
					
					com_protocol::ClientWinRedPacketRecord* recordInfo = info.add_record();
					UserIdToInfoMap::const_iterator userInfoIt = m_redPacketActivityData.winRedPacketUserId2Infos.find(redPacketRecord.receive_username());
					if (userInfoIt != m_redPacketActivityData.winRedPacketUserId2Infos.end())
					{
						recordInfo->set_nickname(userInfoIt->second.nickname);
						recordInfo->set_portrait_id(userInfoIt->second.portraitId);
					}
					else
					{
						recordInfo->set_nickname(redPacketRecord.receive_username());
						recordInfo->set_portrait_id(0);
					}
					
					recordInfo->set_time_secs(redPacketRecord.receive_time());
					*(recordInfo->mutable_gift()) = redPacketRecord.gift();
					
					if (redPacketActivityInfo.best_username() == redPacketRecord.receive_username()) info.set_best_index(info.record_size() - 1);
				}
			}
			
			info.set_received_count(receivedCount);
			
			return true;
		}
	}
	
	return false;
}

int CRedPacketActivity::getWinRedPacketInfoIndex(const unsigned int activityId)
{
	for (int activityIdx = m_redPacketActivityData.redPacketActivityData.activitys_size() - 1; activityIdx >= 0; --activityIdx)
	{
		if (activityId == m_redPacketActivityData.redPacketActivityData.activitys(activityIdx).activity_id()) return activityIdx;
	}
	
	return -1;
}

const com_protocol::RedPacketActivityInfo* CRedPacketActivity::getWinRedPacketActivity(const unsigned int activityId)
{
	for (int activityIdx = m_redPacketActivityData.redPacketActivityData.activitys_size() - 1; activityIdx >= 0; --activityIdx)
	{
		const com_protocol::RedPacketActivityInfo& rpaInfo = m_redPacketActivityData.redPacketActivityData.activitys(activityIdx);
		if (activityId == rpaInfo.activity_id()) return &rpaInfo;
	}
	
	return NULL;
}

bool CRedPacketActivity::generateRedPackets(const unsigned int activityId, const HallConfigData::RedPacketActivityInfo* rpactCfgInfo, SRedPacketActivityData& rpaData)
{
	if (!rpaData.redPackets.empty()) return true;

	char activityIdStr[32] = {0};
	unsigned int index = 0;
	for (vector<HallConfigData::RedPacketInfo>::const_iterator rpInfoIt = rpactCfgInfo->red_packet_list.begin(); rpInfoIt != rpactCfgInfo->red_packet_list.end(); ++rpInfoIt)
	{
		// 配置错误
		if (rpInfoIt->gift_type < EPropType::PropGold || rpInfoIt->gift_type >= EPropType::PropMax || rpInfoIt->gift_amount < 1 || rpInfoIt->red_packet_count < 1
		    || rpInfoIt->max_gift_count < rpInfoIt->min_gift_count || rpInfoIt->max_gift_count > rpInfoIt->gift_amount
			|| (rpInfoIt->min_gift_count * rpInfoIt->red_packet_count) > rpInfoIt->gift_amount)
		{
			rpaData.redPackets.clear();
			return false;
		}
		
		const unsigned int sameRedPacketTryTimes = rpInfoIt->red_packet_count * 2;
		unsigned int tryTimes = 0;
		unsigned int giftAmount = rpInfoIt->gift_amount;
		unsigned int minGiftAmount = 0;
		unsigned int giftCount = 0;
		for (unsigned int idx = 0; idx < rpInfoIt->red_packet_count; ++idx)
		{
			if (giftAmount < rpInfoIt->min_gift_count)
			{
				rpaData.redPackets.clear();
				return false;
			}
			
			minGiftAmount = rpInfoIt->min_gift_count * (rpInfoIt->red_packet_count - idx - 1);
			tryTimes = 0;
			do
			{
				if ((idx + 1) == rpInfoIt->red_packet_count)  // 最后一个红包
				{
					giftCount = giftAmount;
					if (giftCount > rpInfoIt->max_gift_count) giftCount = getPositiveRandNumber(rpInfoIt->min_gift_count, rpInfoIt->max_gift_count);
				}
				else
				{
					giftCount = getPositiveRandNumber(rpInfoIt->min_gift_count, rpInfoIt->max_gift_count);
					while ((giftAmount - giftCount) < minGiftAmount)
					{
						if (--giftCount < rpInfoIt->min_gift_count) break;
					}
					
					if (giftCount < rpInfoIt->min_gift_count)
					{
						rpaData.redPackets.clear();
						return false;
					}
				}
				
			} while (existRedPacket(rpaData.redPackets, rpInfoIt->gift_type, giftCount) && ++tryTimes < sameRedPacketTryTimes);
			
			giftAmount -= giftCount;
			
			snprintf(activityIdStr, sizeof(activityIdStr) - 1, "%u-%u", activityId, ++index);
			rpaData.redPackets[activityIdStr] = SRedPacketInfo(rpInfoIt->gift_type, giftCount, ERedPacketStatus::ENotReceiveRedPacket);
		}
	}
	
	if (!rpaData.redPackets.empty())
	{
		rpaData.onReceiveRedPacketUsers.clear();
		
		rpaData.bestGoldCount = 0;
		rpaData.winRedPacketIndex = rpaData.redPackets.size();
		
		return true;
	}
	
	return false;
}

bool CRedPacketActivity::existRedPacket(const RedPacketIdToInfoMap& rpInfoMap, const unsigned int type, const unsigned int count)
{
	for (RedPacketIdToInfoMap::const_iterator it = rpInfoMap.begin(); it != rpInfoMap.end(); ++it)
	{
		if (it->second.type == type && it->second.count == count) return true;
	}
	
	return false;
}

const std::string* CRedPacketActivity::winRedPacket(const RedPacketIdToInfoMap& rpInfoMap)
{
	std::vector<const std::string*> notReceiveRedPacketIds;
	std::vector<const std::string*> onReceiveRedPacketIds;
	for (RedPacketIdToInfoMap::const_iterator it = rpInfoMap.begin(); it != rpInfoMap.end(); ++it)
	{
		if (it->second.status == ERedPacketStatus::ENotReceiveRedPacket) notReceiveRedPacketIds.push_back(&it->first);
		else if (it->second.status == ERedPacketStatus::EOnReceiveRedPacket) onReceiveRedPacketIds.push_back(&it->first);
	}
	
	if (!notReceiveRedPacketIds.empty()) return notReceiveRedPacketIds[getPositiveRandNumber(0, notReceiveRedPacketIds.size() - 1)];
	if (!onReceiveRedPacketIds.empty()) return onReceiveRedPacketIds[getPositiveRandNumber(0, onReceiveRedPacketIds.size() - 1)];
	
	return NULL;
}

ERedPacketActivityStatus CRedPacketActivity::getActivityStatus(const unsigned int activityId)
{
	if (activityId == m_redPacketActivityData.lastFinishActivityId) return ERedPacketActivityStatus::ERedPacketActivityOff;
	
	if (m_redPacketActivityData.redPackets.empty()) return ERedPacketActivityStatus::ERedPacketActivityWait;
	
	for (RedPacketIdToInfoMap::const_iterator it = m_redPacketActivityData.redPackets.begin(); it != m_redPacketActivityData.redPackets.end(); ++it)
	{
		if (it->second.status != ERedPacketStatus::ESuccessReceivedRedPacket) return ERedPacketActivityStatus::ERedPacketActivityOn;
	}
	
	return ERedPacketActivityStatus::ERedPacketActivityOff;
}

}
