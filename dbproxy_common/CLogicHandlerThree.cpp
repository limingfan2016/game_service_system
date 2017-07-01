
/* author : limingfan
* date : 2017.05.18
* description : 消息处理辅助类
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

#include "MessageDefine.h"
#include "CSrvMsgHandler.h"
#include "CLogic.h"
#include "SrvFrame/CModule.h"
#include "CLogicHandlerThree.h"
#include "base/Function.h"
#include "db/CMySql.h"
#include "base/CMD5.h"
#include "CGameRecord.h"
#include "../common/CommonType.h"


using namespace NProject;

// 数据日志文件
#define WriteDataLog(format, args...)     m_msgHandler->getLogic().getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)


CLogicHandlerThree::CLogicHandlerThree()
{
	m_msgHandler = NULL;
}

CLogicHandlerThree::~CLogicHandlerThree()
{
	m_msgHandler = NULL;
}

int CLogicHandlerThree::init(CSrvMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

    // 游戏内虚拟物品兑换
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_USE_HAN_YOU_COUPON_NOTIFY, (ProtocolHandler)&CLogicHandlerThree::useHanYouCouponNotify, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_ADD_HAN_YOU_COUPON_REWARD_REQ, (ProtocolHandler)&CLogicHandlerThree::addHanYouCouponReward, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_AUTO_SCORE_EXCHANGE_ITEM_REQ, (ProtocolHandler)&CLogicHandlerThree::autoScoreExchangeItem, this);
	
	return 0;
}

void CLogicHandlerThree::updateConfig()
{
}

// 韩国平台 玩家使用优惠券通知
void CLogicHandlerThree::useHanYouCouponNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientUseHanYouCouponNotify useCouponNotify;
	if (!m_msgHandler->parseMsgFromBuffer(useCouponNotify, data, len, "use coupon notify")) return;
	
	ServiceLogicData& srvData = m_msgHandler->getLogicDataInstance().setLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
	com_protocol::HanYouCouponInfo* couponInfo = srvData.logicData.mutable_han_you_data()->add_coupon_info();
	couponInfo->set_coupon_id(useCouponNotify.coupon_id());
	couponInfo->set_reward_count(useCouponNotify.reward_count());
	
	const int CouponMaxSize = 5;
	if (srvData.logicData.han_you_data().coupon_info_size() > CouponMaxSize)
	{
		google::protobuf::RepeatedPtrField<com_protocol::HanYouCouponInfo>* couponInfoList = srvData.logicData.mutable_han_you_data()->mutable_coupon_info();
	    couponInfoList->DeleteSubrange(0, 1);
	}
	
	WriteDataLog("use HanYou coupon|%s|%s|%u", m_msgHandler->getContext().userData, useCouponNotify.coupon_id().c_str(), useCouponNotify.reward_count());
}

// 韩国平台 玩家使用优惠券获得奖励
void CLogicHandlerThree::addHanYouCouponReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UseHanYouCouponReq addCouponRewardReq;
	if (!m_msgHandler->parseMsgFromBuffer(addCouponRewardReq, data, len, "add coupon reward request")) return;

	com_protocol::UseHanYouCouponRsp rsp;
	rsp.set_result(ServiceGetUserinfoFailed);
	
	CUserBaseinfo userBaseinfo;
	if (m_msgHandler->getUserBaseinfo(m_msgHandler->getContext().userData, userBaseinfo))
	{
		int couponIndex = -1;
		rsp.set_result(HanYouCouponNotFound);
		const ServiceLogicData& srvData = m_msgHandler->getLogicDataInstance().getLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
		const google::protobuf::RepeatedPtrField<com_protocol::HanYouCouponInfo>& couponInfoList = srvData.logicData.han_you_data().coupon_info();
		for (int idx = 0; idx < couponInfoList.size(); ++idx)
		{
			const com_protocol::HanYouCouponInfo& couponInfo = couponInfoList.Get(idx);
			if (couponInfo.coupon_id() == addCouponRewardReq.coupon_id())  // && couponInfo.reward_count() == addCouponRewardReq.coupon_reward().num())
			{
				couponIndex = idx;
				rsp.set_result(ServiceSuccess);
				break;
			}
		}
	
		if (rsp.result() == ServiceSuccess)
		{
			google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo> goodsList;  // 领取的物品
			com_protocol::ItemChangeInfo* itemInfo = goodsList.Add();
			itemInfo->set_type(addCouponRewardReq.coupon_reward().type());
			itemInfo->set_count(addCouponRewardReq.coupon_reward().num());
			
			// 写游戏记录
			com_protocol::BuyuGameRecordStorageExt recordData;
			com_protocol::ItemRecordStorage* itemRecord = recordData.add_items();
			itemRecord->set_item_type(itemInfo->type());
			itemRecord->set_charge_count(itemInfo->count());
			recordData.set_room_rate(0);
			recordData.set_room_name("大厅");
			recordData.set_remark("HanYou优惠券奖励");
			
			// 金币&道具&玩家属性等数量变更修改
			int result = m_msgHandler->changeUserPropertyValue(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, goodsList, recordData);
			rsp.set_result(result);
			
			if (rsp.result() == ServiceSuccess)
			{
				ServiceLogicData& setSrvData = m_msgHandler->getLogicDataInstance().setLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
				setSrvData.logicData.mutable_han_you_data()->mutable_coupon_info()->DeleteSubrange(couponIndex, 1);
			}
		}
	}
	
	WriteDataLog("add HanYou coupon reward|%s|%s|%u|%u|%d", m_msgHandler->getContext().userData, addCouponRewardReq.coupon_id().c_str(),
	addCouponRewardReq.coupon_reward().type(), addCouponRewardReq.coupon_reward().num(), rsp.result());
	
	rsp.set_allocated_coupon_id(addCouponRewardReq.release_coupon_id());
	rsp.set_allocated_coupon_reward(addCouponRewardReq.release_coupon_reward());
	if (addCouponRewardReq.has_cb_data()) rsp.set_allocated_cb_data(addCouponRewardReq.release_cb_data());
	
	char dataBuffer[MaxMsgLen] = {0};
	unsigned int msgBufferLen = sizeof(dataBuffer);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, dataBuffer, msgBufferLen, "add coupon reward reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, msgBufferLen, srcSrvId, BUSINESS_ADD_HAN_YOU_COUPON_REWARD_RSP);
}

void CLogicHandlerThree::autoScoreExchangeItem(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientAutoExchangeScoresItemReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "auto score exchange item")) return;

	com_protocol::ClientAutoExchangeScoresItemRsp rsp;
	rsp.set_result(ServiceSuccess);

    unsigned int payCount = 0;
	unsigned int getCount = 0;
	
	do 
	{
		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_msgHandler->getUserBaseinfo(string(m_msgHandler->getContext().userData), user_baseinfo))
		{
			rsp.set_result(ServiceGetUserinfoFailed);//获取DB中的信息失败
			break;
		}
		
		auto itemCfg = m_msgHandler->getLogicHander().getExchangeItemCfg(req.item_id());
		if (itemCfg == NULL)
		{
			rsp.set_result(ServiceNotFindExchangeItem);
			break;
		}
		
		//判断积分是否足够
		if (user_baseinfo.dynamic_info.score < itemCfg->exchange_scores)
		{
			rsp.set_result(ServiceScoresNotEnough);
			break;
		}
		
		uint32_t* type2value[] =
		{
			NULL,                                               //保留
			NULL,                                               //金币
			NULL,                                               //渔币
			NULL,                                               //话费卡
			&user_baseinfo.prop_info.suona_count,				//小喇叭
			&user_baseinfo.prop_info.light_cannon_count,		//激光炮
			&user_baseinfo.prop_info.flower_count,				//鲜花
			&user_baseinfo.prop_info.mute_bullet_count,			//哑弹
			&user_baseinfo.prop_info.slipper_count,			    //拖鞋
			NULL,                                   	        //奖券
			&user_baseinfo.prop_info.auto_bullet_count,		    //自动炮子弹
			&user_baseinfo.prop_info.lock_bullet_count,		    //锁定炮子弹
			&user_baseinfo.dynamic_info.diamonds_number,        //钻石
			NULL,                                               //话费额度
			NULL,                            					//积分
			&user_baseinfo.prop_info.rampage_count,				//狂暴
			&user_baseinfo.prop_info.dud_shield_count,		    //哑弹防护
		};
		const unsigned int typeCount = sizeof(type2value) / sizeof(type2value[0]);
		
		if (req.item_id() >= typeCount)
		{
			rsp.set_result(ServicePropTypeInvalid);
			break;
		}

        // 增加目标物品数量
        int64_t tmp = 0;
		uint32_t* value = type2value[req.item_id()];
		if (value == NULL)
		{
			if (req.item_id() != EPropType::PropGold)
			{
				rsp.set_result(ServicePropTypeInvalid);
			    break;
			}
			
			tmp = user_baseinfo.dynamic_info.game_gold + itemCfg->get_goods_count;
			user_baseinfo.dynamic_info.game_gold = tmp;  // 金币

			if (tmp < 0)
			{
				rsp.set_result(ServicePropNotEnought);
			    break;
			}
		}
		else
		{
			tmp = (int64_t)(*value) + itemCfg->get_goods_count;
			if (tmp < 0)
			{
				rsp.set_result(ServicePropNotEnought);
			    break;
			}

			*value = (uint32_t)tmp;
		}

		//开始兑换 减少积分
		user_baseinfo.dynamic_info.score -= itemCfg->exchange_scores;
		
		// 将数据同步到memcached
		if (!m_msgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			break;
		}
		m_msgHandler->updateOptStatus(m_msgHandler->getContext().userData, CSrvMsgHandler::UpdateOpt::Modify);	// 有修改，更新状态
		
		// 写游戏记录
		com_protocol::BuyuGameRecordStorageExt scoreExchangeRd;
		scoreExchangeRd.set_room_rate(0);
		scoreExchangeRd.set_room_name("大厅");
		scoreExchangeRd.set_remark("自动积分兑换物品");
		com_protocol::ItemRecordStorage* itemRecord = scoreExchangeRd.add_items();
		itemRecord->set_item_type(EPropType::PropScores);
		itemRecord->set_charge_count(-itemCfg->exchange_scores);
		itemRecord = scoreExchangeRd.add_items();
		itemRecord->set_item_type(req.item_id());
		itemRecord->set_charge_count(itemCfg->get_goods_count);
			
		com_protocol::GameRecordPkg record;
		char dataBuffer[MaxMsgLen] = {0};
		unsigned int dataBufferLen = sizeof(dataBuffer);
		const char* msgData = m_msgHandler->serializeMsgToBuffer(scoreExchangeRd, dataBuffer, dataBufferLen, "auto score exchange item record");
		if (msgData != NULL)
		{
			record.set_game_type(NProject::GameRecordType::BuyuExt);
			record.set_game_record_bin(msgData, dataBufferLen);
			m_msgHandler->m_p_game_record->procGameRecord(record, &user_baseinfo);  // 写游戏记录
		}

		rsp.set_scores(user_baseinfo.dynamic_info.score);
		com_protocol::GiftInfo* giftItem = rsp.mutable_gift();
		giftItem->set_type(req.item_id());
		giftItem->set_num(itemCfg->get_goods_count);

		payCount = itemCfg->exchange_scores;
	    getCount = itemCfg->get_goods_count;

	} while (0);
			
	char send_data[MaxMsgLen] = { 0 };
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, send_data, send_data_len, "send auto exchange scores item");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_AUTO_SCORE_EXCHANGE_ITEM_RSP);
	
	WriteDataLog("auto score exchange gift|%s|%d|%u|%u|%u", m_msgHandler->getContext().userData, rsp.result(), payCount, req.item_id(), getCount);
}

