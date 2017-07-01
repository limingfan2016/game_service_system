
/* author : limingfan
 * date : 2015.08.6
 * description : 大厅逻辑服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <string>

#include "CLogicHandler.h"
#include "CLoginSrv.h"
#include "base/ErrorCode.h"
#include "common/CommonType.h"
#include "connect/CSocket.h"
#include "_HallConfigData_.h"


using namespace std;
using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace HallConfigData;
using namespace NProject;


namespace NPlatformService
{

CHallLogicHandler::CHallLogicHandler() : m_msgHandler(NULL)
{
}

CHallLogicHandler::~CHallLogicHandler()
{
	m_msgHandler = NULL;
}


void CHallLogicHandler::onLine(ConnectUserData* ud, ConnectProxy* conn)
{
	com_protocol::HallLogicData* hallLogicData = ud->hallLogicData;
	time_t curSecs = time(NULL);
	unsigned int theDay = localtime(&curSecs)->tm_yday;

	com_protocol::NoGoldFreeGive* noGoldFreeGive = hallLogicData->mutable_nogold_free_give();
	if (noGoldFreeGive->today() != theDay)
	{
		noGoldFreeGive->set_today(theDay);
		noGoldFreeGive->set_times(0);
		noGoldFreeGive->set_seconds(0);
	}
	else
	{
		getNoGoldFreeGiveInfo(conn, 0, true);
	}
	
	// 第一次上线则设置当前时间
	if (!hallLogicData->has_overseas_notify_info())
	{
		hallLogicData->mutable_overseas_notify_info()->set_payment_secs(curSecs);
	}
	
	com_protocol::HallData hallData;
	if (m_msgHandler->getArenaServiceFromRedis(ud, hallData))
	{
		// 获取该用户的比赛信息
		m_msgHandler->sendMessage(NULL, 0, ud->connId, ud->connIdLen, hallData.rooms(0).id(), CommonProtocol::COMMON_GET_ARENA_MATCH_USER_INFO_REQ);
	}
}

void CHallLogicHandler::offLine(com_protocol::HallLogicData* hallLogicData)
{
}

void CHallLogicHandler::load(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;

	// 物品兑换
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_EXCHANGE_PHONE_CARD_REQ, (ProtocolHandler)&CHallLogicHandler::exchangePhoneCard, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_EXCHANGE_GOODS_REQ, (ProtocolHandler)&CHallLogicHandler::exchangeGoods, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_OVERSEAS_NOTIFY_REQ, (ProtocolHandler)&CHallLogicHandler::popOverseasNotify, this);

	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_EXCHANGE_PHONE_FARE_REQ, (ProtocolHandler)&CHallLogicHandler::exchangePhoneFareReq, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_EXCHANGE_PHONE_FARE_RSP, (ProtocolHandler)&CHallLogicHandler::exchangePhoneFareRsp, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_NO_GOLD_FREE_GIVE_REQ, (ProtocolHandler)&CHallLogicHandler::getNoGoldFreeGive, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_NO_GOLD_FREE_GIVE_REQ, (ProtocolHandler)&CHallLogicHandler::receiveNoGoldFreeGive, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, LOGINSRV_ADD_ALLOWANCE_GOLD_RSP, (ProtocolHandler)&CHallLogicHandler::addAllowanceGoldReply, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_ADD_MAIL_BOX_REQ, (ProtocolHandler)&CHallLogicHandler::addEmailBox, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, LOGINSRV_ADD_MAIL_BOX_RSP, (ProtocolHandler)&CHallLogicHandler::addEmailBoxReply, this);
	
	// 破产补助免费赠送金币
	m_msgHandler->registerProtocol(CommonSrv, NO_GOLD_FREE_GIVE_REQ, (ProtocolHandler)&CHallLogicHandler::getNoGoldFreeGive, this);
	m_msgHandler->registerProtocol(CommonSrv, GET_NO_GOLD_FREE_GIVE_REQ, (ProtocolHandler)&CHallLogicHandler::receiveNoGoldFreeGive, this);

	m_msgHandler->registerProtocol(CommonSrv, GAME_USER_VIP_LV_UPDATE_NOTIFY, (ProtocolHandler)&CHallLogicHandler::gameUserVIPLvUpdateNotify, this);
	
	// 收到的CommonDB代理服务应答消息
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_EXCHANGE_PHONE_CARD_RSP, (ProtocolHandler)&CHallLogicHandler::exchangePhoneCardReply, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_EXCHANGE_GOODS_RSP, (ProtocolHandler)&CHallLogicHandler::exchangeGoodsReply, this);
	
	// 用户个人财务变更通知，如：金币变化、道具变化、奖券变化等
	m_msgHandler->registerProtocol(ManageSrv, ManagerSrvProtocol::PERSON_PROPERTY_NOTIFY, (ProtocolHandler)&CHallLogicHandler::personPropertyNotify, this);
	
	// 停止游戏服务通知
	m_msgHandler->registerProtocol(MessageSrv, MessageSrvProtocol::BUSINESS_STOP_GAME_SERVICE_NOTIFY, (ProtocolHandler)&CHallLogicHandler::stopGameServiceNotify, this);
	
	// Google SDK 操作
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_GOOGLE_RECHARGE_REQ, (ProtocolHandler)&CHallLogicHandler::getRechargeTransaction, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_CHECK_GOOGLE_RECHARGE_REQ, (ProtocolHandler)&CHallLogicHandler::checkRechargeResult, this);
	m_msgHandler->registerProtocol(HttpSrv, HttpServiceProtocol::GET_GOOGLE_RECHARGE_TRANSACTION_RSP, (ProtocolHandler)&CHallLogicHandler::getRechargeTransactionReply, this);
	m_msgHandler->registerProtocol(HttpSrv, HttpServiceProtocol::CHECK_GOOGLE_RECHARGE_RESULT_RSP, (ProtocolHandler)&CHallLogicHandler::checkRechargeResultReply, this);
	m_msgHandler->registerProtocol(CommonSrv, LoginSrvProtocol::LOGINSRV_OVERSEAS_RECHARGE_NOTIFY, (ProtocolHandler)&CHallLogicHandler::overseasRechargeNotify, this);
	
	// 第三方 SDK 操作
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_THIRD_RECHARGE_TRANSACTION_REQ, (ProtocolHandler)&CHallLogicHandler::getThirdPlatformRechargeTransaction, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_CANCEL_THIRD_RECHARGE_NOTIFY, (ProtocolHandler)&CHallLogicHandler::cancelThirdPlatformRechargeNotify, this);
	m_msgHandler->registerProtocol(HttpSrv, HttpServiceProtocol::GET_THIRD_RECHARGE_TRANSACTION_RSP, (ProtocolHandler)&CHallLogicHandler::getThirdPlatformRechargeTransactionReply, this);
	m_msgHandler->registerProtocol(HttpSrv, HttpServiceProtocol::THIRD_USER_RECHARGE_NOTIFY, (ProtocolHandler)&CHallLogicHandler::thirdPlatformRechargeTransactionNotify, this);
	
	// 第三方平台账号检查
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_CHECK_THIRD_USER_REQ, (ProtocolHandler)&CHallLogicHandler::checkThirdUser, this);
	m_msgHandler->registerProtocol(HttpSrv, HttpServiceProtocol::CHECK_THIRD_USER_RSP, (ProtocolHandler)&CHallLogicHandler::checkThirdUserReply, this);
	
	// 小米平台账号检查
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_CHECK_XIAOMI_USER_REQ, (ProtocolHandler)&CHallLogicHandler::checkXiaoMiUser, this);
	m_msgHandler->registerProtocol(HttpSrv, HttpServiceProtocol::CHECK_XIAOMI_USER_RSP, (ProtocolHandler)&CHallLogicHandler::checkXiaoMiUserReply, this);

	//	查询腾讯充值是否成功
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_FIND_RECHARGE_SUCCESS_REQ, (ProtocolHandler)&CHallLogicHandler::findRechargeSuccess, this);
	m_msgHandler->registerProtocol(HttpSrv, HttpServiceProtocol::FIN_RECHARGE_SUCCESS_RSP, (ProtocolHandler)&CHallLogicHandler::findRechargeSuccessReply, this);

	
	// 排行榜
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_VIEW_RANKING_LIST_REQ, (ProtocolHandler)&CHallLogicHandler::viewRankingList, this);
	m_msgHandler->registerProtocol(MessageSrv, MessageSrvProtocol::BUSINESS_GOLD_RANKING_LIST_RSP, (ProtocolHandler)&CHallLogicHandler::viewGoldRankingReply, this);
	m_msgHandler->registerProtocol(MessageSrv, MessageSrvProtocol::BUSINESS_PHONE_RANKING_LIST_RSP, (ProtocolHandler)&CHallLogicHandler::viewPhoneRankingReply, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_GET_FISH_RATE_SCORE_REQ, (ProtocolHandler)&CHallLogicHandler::getFishRateScore, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_VERSION_HOT_UPDATE_REQ, (ProtocolHandler)&CHallLogicHandler::checkVersionHotUpdate, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_READ_SYSTEM_MSG_NOTIFY, (ProtocolHandler)&CHallLogicHandler::readSystemMsgNotify, this);
	
	m_msgHandler->registerProtocol(BuyuGameSrv, CommonProtocol::COMMON_GET_ARENA_MATCH_USER_INFO_RSP, (ProtocolHandler)&CHallLogicHandler::arenaMatchOnNotify, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_CANCEL_ARENA_MATCH_NOTIFY, (ProtocolHandler)&CHallLogicHandler::cancelArenaMatchNotify, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_GET_XIANGQI_SERVICE_INFO_REQ, (ProtocolHandler)&CHallLogicHandler::getXiangQiServiceInfo, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_GET_GAME_SERVICE_INFO_REQ, (ProtocolHandler)&CHallLogicHandler::getGameServiceInfo, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_GET_NEW_PLAYER_GUIDE_REQ, (ProtocolHandler)&CHallLogicHandler::getNewPlayerGuideInfo, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_FINISH_NEW_PLAYER_GUIDE_NOTIFY, (ProtocolHandler)&CHallLogicHandler::finishNewPlayerGuideNotify, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_USE_HAN_YOU_COUPON_NOTIFY, (ProtocolHandler)&CHallLogicHandler::useHanYouCouponNotify, this);
	m_msgHandler->registerProtocol(HttpSrv, CommonProtocol::COMMON_ADD_HAN_YOU_COUPON_REWARD_NOTIFY, (ProtocolHandler)&CHallLogicHandler::addHanYouCouponRewardNotify, this);
	
	// 自动积分兑换物品
	m_msgHandler->registerProtocol(OutsideClientSrv, LoginSrvProtocol::LOGINSRV_AUTO_SCORE_EXCHANGE_ITEM_REQ, (ProtocolHandler)&CHallLogicHandler::autoScoreExchangeItem, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_AUTO_SCORE_EXCHANGE_ITEM_RSP, (ProtocolHandler)&CHallLogicHandler::autoScoreExchangeItemReply, this);
}

void CHallLogicHandler::unLoad(CSrvMsgHandler* msgHandler)
{
	
}


// 大厅逻辑处理
void CHallLogicHandler::exchangePhoneCard(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    if (!m_msgHandler->checkLoginIsSuccess("exchange phone card request")) return;
	
	m_msgHandler->sendMessageToCommonDbProxy(data, len, BUSINESS_EXCHANGE_PHONE_CARD_REQ);
}

void CHallLogicHandler::exchangePhoneCardReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
    ConnectProxy* conn = getConnectInfo("exchange phone card reply", userName);
	if (conn == NULL) return;
	
	m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_EXCHANGE_PHONE_CARD_RSP, conn);
}

void CHallLogicHandler::exchangeGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("exchange goods request")) return;
	
	m_msgHandler->sendMessageToCommonDbProxy(data, len, BUSINESS_EXCHANGE_GOODS_REQ);
}

void CHallLogicHandler::exchangeGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
    ConnectProxy* conn = getConnectInfo("exchange phone card reply", userName);
	if (conn == NULL) return;
	
	//分享奖励
	com_protocol::ExchangeGoodsRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "exchange goods reply")) return;
	
	if (rsp.result() == 0)
	{
		com_protocol::HallLogicData* hallData = m_msgHandler->getHallLogicData(userName.c_str());
		if (hallData == NULL)
		{
			OptErrorLog("CHallLogicHandler exchangeGoodsReply, hallData == NULL, user name:%s", userName.c_str());
			return;
		}

		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		unsigned int index = getShareRewardIndex(hallData);
		if (index < cfg.hall_share.share_reward_list.size())
		{
			for (auto rewardCfg = cfg.hall_share.share_reward_list[index].reward.begin(); rewardCfg != cfg.hall_share.share_reward_list[index].reward.end();
				++rewardCfg)
			{
				auto shareReward = rsp.add_share_reward();
				shareReward->set_id(rewardCfg->first);
				shareReward->set_num(rewardCfg->second);
			}
		}
	}	

	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "exchange goods reply");
	m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_EXCHANGE_GOODS_RSP, conn);
}

void CHallLogicHandler::exchangePhoneFareReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("exchange phone fare req")) return;
	m_msgHandler->sendMessageToCommonDbProxy(data, len, CommonDBSrvProtocol::BUSINESS_EXCHANGE_PHONE_FARE_REQ);
}

void CHallLogicHandler::exchangePhoneFareRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("exchange phone card reply", userName);
	if (conn == NULL) return;

	com_protocol::ExchangePhoneFareRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "exchange phone fare rsp")) return;

	if (rsp.result() == 0)
	{
		com_protocol::HallLogicData* hallData = m_msgHandler->getHallLogicData(userName.c_str());
		if (hallData == NULL)
		{
			OptErrorLog("CHallLogicHandler exchangePhoneFareRsp, hallData == NULL, user name:%s", userName.c_str());
			return;
		}

		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		unsigned int index = getShareRewardIndex(hallData);
		if (index < cfg.hall_share.share_reward_list.size())
		{
			for (auto rewardCfg = cfg.hall_share.share_reward_list[index].reward.begin(); rewardCfg != cfg.hall_share.share_reward_list[index].reward.end();
				++rewardCfg)
			{
				auto shareReward = rsp.add_share_reward();
				shareReward->set_id(rewardCfg->first);
				shareReward->set_num(rewardCfg->second);
			}
		}
	}
	
	//生成分享ID
	m_msgHandler->getHallLogic().generateShareID(userName.c_str());
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "exchange phone fare rsp");
	m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_EXCHANGE_PHONE_FARE_RSP, conn);
}

void CHallLogicHandler::getNoGoldFreeGiveInfo(ConnectProxy* conn, unsigned int srcSrvId, bool isActiveNotify)
{
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	com_protocol::NoGoldFreeGive* noGoldFreeGive = ud->hallLogicData->mutable_nogold_free_give();

	com_protocol::VIPInfo* userVIPInfo = ud->hallLogicData->mutable_vip_info();
	if (userVIPInfo == NULL)
	{
		OptErrorLog("CHallLogicHandler getNoGoldFreeGiveInfo, userVIPInfo == NULL");
		return;
	}
	
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	com_protocol::NoGoldFreeGiveRsp noGoldFreeGiveRsp;
	if (noGoldFreeGive->seconds() > 0)
	{
		// 之前已经触发了破产补助金币
		unordered_map<int, NoGoldFreeGive>::const_iterator it = cfg.nogold_free_give.find(noGoldFreeGive->times());

		if (it != cfg.nogold_free_give.end())
		{
			noGoldFreeGiveRsp.set_gold(it->second.gold);
		}
		else
		{
			it = cfg.vip_nogold_free_give.find(userVIPInfo->subsidies_num_cur());
			if (it != cfg.vip_nogold_free_give.end())
			{
				noGoldFreeGiveRsp.set_gold(it->second.gold);
			}
			else
			{
				OptErrorLog("CHallLogicHandler getNoGoldFreeGiveInfo, can not find the no vip gold free give config data, times = %d, user = %s", userVIPInfo->subsidies_num_cur(), ud->connId);
				return;
			}
		}

		noGoldFreeGiveRsp.set_result(Opt_Success);
		unsigned int sumAmount = cfg.nogold_free_give.size() + userVIPInfo->subsidies_num_max();
		noGoldFreeGiveRsp.set_amount(sumAmount);
		noGoldFreeGiveRsp.set_times(noGoldFreeGive->times());
		unsigned int remainSecs = time(NULL);
		remainSecs = (remainSecs < noGoldFreeGive->seconds()) ? (noGoldFreeGive->seconds() - remainSecs) : 0;
		noGoldFreeGiveRsp.set_seconds(remainSecs);
	}
	else if ((cfg.nogold_free_give.size() + userVIPInfo->subsidies_num_max())> noGoldFreeGive->times() && !isActiveNotify)
	{
		// 新触发的破产补助金币信息
		noGoldFreeGiveRsp.set_times(noGoldFreeGive->times() + 1);
		unordered_map<int, NoGoldFreeGive>::const_iterator it = cfg.nogold_free_give.find(noGoldFreeGiveRsp.times());
		if (it != cfg.nogold_free_give.end())
		{
			noGoldFreeGive->set_times(noGoldFreeGive->times() + 1);
			noGoldFreeGive->set_seconds(it->second.seconds + time(NULL));
			
			noGoldFreeGiveRsp.set_result(Opt_Success);
			unsigned int sumAmount = cfg.nogold_free_give.size() + userVIPInfo->subsidies_num_max();
			noGoldFreeGiveRsp.set_amount(sumAmount);
			noGoldFreeGiveRsp.set_gold(it->second.gold);
			noGoldFreeGiveRsp.set_seconds(it->second.seconds);
		}
		else
		{
			//VIP 额外破产次数
			if (userVIPInfo->subsidies_num_cur() < userVIPInfo->subsidies_num_max())
			{
				uint32_t nCurNum = userVIPInfo->subsidies_num_cur();
				unordered_map<int, NoGoldFreeGive>::const_iterator it = cfg.vip_nogold_free_give.find(nCurNum);
				if (it == cfg.vip_nogold_free_give.end())
				{
					OptErrorLog("in getNoGoldFreeGiveInfo, can not find vip gold free give config data, vip cur num, user:%d", nCurNum, ud->connId);
					return;
				}

				noGoldFreeGive->set_times(noGoldFreeGive->times() + 1);
				noGoldFreeGive->set_seconds(it->second.seconds + time(NULL));

				noGoldFreeGiveRsp.set_result(Opt_Success);
				noGoldFreeGiveRsp.set_amount(cfg.nogold_free_give.size() + userVIPInfo->subsidies_num_max());
				noGoldFreeGiveRsp.set_gold(it->second.gold);
				noGoldFreeGiveRsp.set_seconds(it->second.seconds);
			}
			else
			{
				OptErrorLog("in getNoGoldFreeGiveInfo, can not find the no gold free give config data, times = %d, user = %s", noGoldFreeGiveRsp.times(), ud->connId);
				return;
			}
		}
	}
	else
	{
		if (isActiveNotify) return;
		
		noGoldFreeGiveRsp.set_result(NotNoGoldFreeGive);
	}
	
	// OptWarnLog("in getNoGoldFreeGiveInfo, times = %d, seconds = %d, current secs = %d, user = %s, srcSrvId = %d", noGoldFreeGive->times(), noGoldFreeGive->seconds(), time(NULL), ud->connId, srcSrvId);
	
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(noGoldFreeGiveRsp, "get no gold free give request");
	if (msgBuffer != NULL)
	{
		if (srcSrvId > 0) m_msgHandler->sendMessage(msgBuffer, noGoldFreeGiveRsp.ByteSize(), srcSrvId, NO_GOLD_FREE_GIVE_RSP);
		else m_msgHandler->sendMsgToProxy(msgBuffer, noGoldFreeGiveRsp.ByteSize(), LOGINSRV_NO_GOLD_FREE_GIVE_RSP, conn);
	}
}

void CHallLogicHandler::getNoGoldFreeGive(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::NoGoldFreeGiveReq noGoldFreeGiveReq;
	if (!m_msgHandler->parseMsgFromBuffer(noGoldFreeGiveReq, data, len, "get no gold free give request")) return;
	
	ConnectProxy* conn = NULL;
	unsigned int dstSrvId = 0;
	if (m_msgHandler->isProxyMsg())
	{
		if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get no gold free give")) return;
		conn = m_msgHandler->getConnectProxyContext().conn;
	}
	else
	{
		dstSrvId = srcSrvId;
		string userName;
        conn = getConnectInfo("get no gold free give request", userName);
	}
	if (conn == NULL) return;

    bool isActiveNotify = noGoldFreeGiveReq.has_flag() ? (noGoldFreeGiveReq.flag() == 1) : false;
	getNoGoldFreeGiveInfo(conn, dstSrvId, isActiveNotify);
}

void CHallLogicHandler::receiveNoGoldFreeGiveHandle(ConnectProxy* conn, unsigned int srcSrvId)
{
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	com_protocol::NoGoldFreeGive* noGoldFreeGive = ud->hallLogicData->mutable_nogold_free_give();

	com_protocol::ReceiveNoGoldFreeGiveRsp receiveNoGoldFreeGiveRsp;
	
	// OptWarnLog("in receiveNoGoldFreeGiveHandle, times = %d, seconds = %d, current secs = %d, user = %s, srcSrvId = %d", noGoldFreeGive->times(), noGoldFreeGive->seconds(), time(NULL) + 5, ud->connId, srcSrvId);
	
	if (noGoldFreeGive->seconds() > 0)
	{
		const unsigned int deviationSecs = 5;  // 前后台误差时间值
		unsigned int remainSecs = time(NULL) + deviationSecs;
		if (remainSecs >= noGoldFreeGive->seconds())
		{
			const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
			unordered_map<int, NoGoldFreeGive>::const_iterator it = cfg.nogold_free_give.find(noGoldFreeGive->times());
			int gold = 0;
			
			if (it != cfg.nogold_free_give.end())
			{
				gold = it->second.gold;
			}
			else 
			{
				//是否是VIP额外次数奖励
				com_protocol::VIPInfo* userVIPInfo = ud->hallLogicData->mutable_vip_info();
				if (userVIPInfo == NULL)
				{
					OptErrorLog("CHallLogicHandler receiveNoGoldFreeGiveHandle, userVIPInfo == NULL");
					return;
				}

				it = cfg.vip_nogold_free_give.find(userVIPInfo->subsidies_num_cur());
				if (it != cfg.vip_nogold_free_give.end())
				{
					gold = it->second.gold;	
					userVIPInfo->set_subsidies_num_cur(userVIPInfo->subsidies_num_cur() + 1);
				}
			}
			
			
			if (gold > 0)
			{
				// 先提前设置领取清零，防止变更金币，异步消息回来之前被疯狂领取多次
				noGoldFreeGive->set_seconds(0);
				
				com_protocol::RoundEndDataChargeReq addLoginRewardReq;
				addLoginRewardReq.set_game_id(GameType::ALL_GAME);
				addLoginRewardReq.set_delta_game_gold(gold);
				com_protocol::GameRecordPkg* gRecord = addLoginRewardReq.mutable_game_record();
				gRecord->set_game_type(0);
				
				com_protocol::BuyuGameRecordStorage recordStore;
				recordStore.set_room_rate(0);
				recordStore.set_room_name("大厅");
				recordStore.set_item_type(EPropType::PropGold);
				recordStore.set_charge_count(gold);
				recordStore.set_remark("破产补助");
				const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(recordStore, "add gold to user when no gold record");
				if (msgBuffer != NULL) gRecord->set_game_record_bin(msgBuffer, recordStore.ByteSize());
		
				msgBuffer = m_msgHandler->serializeMsgToBuffer(addLoginRewardReq, "add allowance gold request");
				if (msgBuffer != NULL)
				{
				    m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgBuffer, addLoginRewardReq.ByteSize(), BUSINESS_ROUND_END_DATA_CHARGE_REQ,
				                                       ud->connId, ud->connIdLen, srcSrvId, LOGINSRV_ADD_ALLOWANCE_GOLD_RSP);
				}
			}
			else
			{
				OptErrorLog("in receiveNoGoldFreeGiveHandle, can not find the no gold free give config data, times = %d, user = %s", noGoldFreeGive->times(), ud->connId);
			}
			
			return;
		}
		else
		{
			receiveNoGoldFreeGiveRsp.set_result(NoGoldFreeGiveNoReceiveTime);
		}
	}
	else
	{
		receiveNoGoldFreeGiveRsp.set_result(NotNoGoldFreeGive);
	}
	
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(receiveNoGoldFreeGiveRsp, "receive no gold free give");
	if (msgBuffer != NULL)
	{
		if (srcSrvId > 0) m_msgHandler->sendMessage(msgBuffer, receiveNoGoldFreeGiveRsp.ByteSize(), srcSrvId, GET_NO_GOLD_FREE_GIVE_RSP);
		else m_msgHandler->sendMsgToProxy(msgBuffer, receiveNoGoldFreeGiveRsp.ByteSize(), LOGINSRV_GET_NO_GOLD_FREE_GIVE_RSP, conn);
	}
}

void CHallLogicHandler::addAllowanceGoldReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::RoundEndDataChargeRsp addGoldRsp;
	if (!m_msgHandler->parseMsgFromBuffer(addGoldRsp, data, len, "add allowance gold reply")) return;

	const string userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL)
	{
		OptErrorLog("in addAllowanceGoldReply can not find the connect data, user data = %s", m_msgHandler->getContext().userData);
		return;
	}
	
	srcSrvId = m_msgHandler->getContext().userFlag;
	com_protocol::ReceiveNoGoldFreeGiveRsp receiveNoGoldFreeGiveRsp;
	if (addGoldRsp.result() == 0)
	{
		ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
		com_protocol::NoGoldFreeGive* noGoldFreeGive = ud->hallLogicData->mutable_nogold_free_give();
		
		// OptWarnLog("in addAllowanceGoldReply, times = %d, seconds = %d, current secs = %d, user = %s, srcSrvId = %d", noGoldFreeGive->times(), noGoldFreeGive->seconds(), time(NULL) + 5, ud->connId, srcSrvId);
	
		// noGoldFreeGive->set_seconds(0);

		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		unordered_map<int, NoGoldFreeGive>::const_iterator it = cfg.nogold_free_give.find(noGoldFreeGive->times());
		if (it != cfg.nogold_free_give.end())
		{
			receiveNoGoldFreeGiveRsp.set_result(Opt_Success);
			receiveNoGoldFreeGiveRsp.set_gold(it->second.gold);
		}
		else
		{
			com_protocol::VIPInfo* userVIPInfo = ud->hallLogicData->mutable_vip_info();
			it = cfg.vip_nogold_free_give.find(userVIPInfo->subsidies_num_cur());
			if (it != cfg.vip_nogold_free_give.end())
			{
				receiveNoGoldFreeGiveRsp.set_result(Opt_Success);
				receiveNoGoldFreeGiveRsp.set_gold(it->second.gold);
			}
			else
			{
				OptErrorLog("in addAllowanceGoldReply, can not find the no gold free give config data, times = %d, user = %s", noGoldFreeGive->times(), ud->connId);
				return;
			}
		}
	}
	else
	{
		receiveNoGoldFreeGiveRsp.set_result(addGoldRsp.result());
	}

	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(receiveNoGoldFreeGiveRsp, "add allowance gold reply");
	if (msgBuffer != NULL)
	{
		if (srcSrvId > 0) m_msgHandler->sendMessage(msgBuffer, receiveNoGoldFreeGiveRsp.ByteSize(), srcSrvId, GET_NO_GOLD_FREE_GIVE_RSP);
		else m_msgHandler->sendMsgToProxy(msgBuffer, receiveNoGoldFreeGiveRsp.ByteSize(), LOGINSRV_GET_NO_GOLD_FREE_GIVE_RSP, conn);
	}
}

void CHallLogicHandler::gameUserVIPLvUpdateNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const string userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL)
	{
		OptErrorLog("CHallLogicHandler gameUserVIPLvUpdateNotify, can not find the connect data, user data = %s", m_msgHandler->getContext().userData);
		return;
	}

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	if (ud == NULL)
	{
		OptErrorLog("CHallLogicHandler gameUserVIPLvUpdateNotify, can not find the connect user data:%s", m_msgHandler->getContext().userData);
		return;
	}

	if (ud->hallLogicData->vip_info().receive() == VIPStatus::VIPNoReward)
	{
		ud->hallLogicData->mutable_vip_info()->set_receive(VIPStatus::VIPReceiveReward);
		m_msgHandler->sendMsgToProxy(NULL, 0, LOGINSRV_FIND_VIP_REWARD_NOTIFY, conn);
	}	
}

void CHallLogicHandler::receiveNoGoldFreeGive(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = NULL;
	unsigned int dstSrvId = 0;
	if (m_msgHandler->isProxyMsg())
	{
		if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to receive no gold free give")) return;
		conn = m_msgHandler->getConnectProxyContext().conn;
	}
	else
	{
		dstSrvId = srcSrvId;
		string userName;
        conn = getConnectInfo("receive no gold free give request", userName);
	}
	if (conn == NULL) return;

	receiveNoGoldFreeGiveHandle(conn, dstSrvId);
}

// 绑定邮箱
void CHallLogicHandler::addEmailBox(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/*
	if (!m_msgHandler->checkLoginIsSuccess("user add email box")) return;
	
	com_protocol::AddMailBoxReq addMailBoxReq;
	if (!m_msgHandler->parseMsgFromBuffer(addMailBoxReq, data, len, "user add email box")) return;
	
	com_protocol::ModifyBaseinfoReq modifyBaseinfoReq;
	modifyBaseinfoReq.set_email(addMailBoxReq.mail_box_name());
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(modifyBaseinfoReq, "user add email box");
	if (msgBuffer != NULL)
	{
		ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
		m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgBuffer, modifyBaseinfoReq.ByteSize(), BUSINESS_MODIFY_BASEINFO_REQ,
										   ud->connId, ud->connIdLen, 0, LOGINSRV_ADD_MAIL_BOX_RSP);
	}
	*/ 
}

void CHallLogicHandler::addEmailBoxReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/*
	string userName;
	ConnectProxy* conn = getConnectInfo("user add email box reply", userName);
	if (conn == NULL) return;
	
	m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_ADD_MAIL_BOX_RSP, conn);
	*/ 
}

void CHallLogicHandler::personPropertyNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("person property notify", userName);
	if (conn == NULL) return;
	
	int rc = m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_PERSON_PROPERTY_NOTIFY, conn);
	OptInfoLog("person property change notify, user = %s, rc = %d", userName.c_str(), rc);
}

// 停止游戏服务通知
void CHallLogicHandler::stopGameServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	OptErrorLog("stopGameServiceNotify, receive message to stop game service, srcSrvId = %d", srcSrvId);
	m_msgHandler->stopService();
}

// 弹出海外广告请求
void CHallLogicHandler::popOverseasNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("pop overseas notify")) return;
	
	com_protocol::GetOverseasNotifyReq getOverseasNotifyReq;
	if (!m_msgHandler->parseMsgFromBuffer(getOverseasNotifyReq, data, len, "pop overseas notify request")) return;
	
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	const unordered_map<unsigned int, unsigned int>& notifyCfg = (getOverseasNotifyReq.type() == 1) ? cfg.over_seas_screen_notify : cfg.over_seas_banner_notify;
	unordered_map<unsigned int, unsigned int>::const_iterator it = notifyCfg.find(getOverseasNotifyReq.click_point());
	if (it == notifyCfg.end())
	{
		OptErrorLog("popOverseasNotify, can not find the overseas notify config, type = %d, click point = %d",
		getOverseasNotifyReq.type(), getOverseasNotifyReq.click_point());
		return;
	}
    
	unsigned int popSecs = m_msgHandler->getHallLogicData()->mutable_overseas_notify_info()->payment_secs() + it->second * 3600;
    com_protocol::GetOverseasNotifyRsp getOverseasNotifyRsp;
	getOverseasNotifyRsp.set_result((time(NULL) >= popSecs) ? 0 : 1);
	if (getOverseasNotifyRsp.result() == 0)
	{
		getOverseasNotifyRsp.set_type(getOverseasNotifyReq.type());
		getOverseasNotifyRsp.set_click_point(getOverseasNotifyReq.click_point());
	}
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(getOverseasNotifyRsp, "pop overseas notify reply");
	if (msgBuffer != NULL) m_msgHandler->sendMsgToProxy(msgBuffer, getOverseasNotifyRsp.ByteSize(), LOGINSRV_GET_OVERSEAS_NOTIFY_RSP);
}

void CHallLogicHandler::viewRankingList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to view ranking list")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->sendMessageToService(ServiceType::MessageSrv, data, len, MessageSrvProtocol::BUSINESS_VIEW_RANKING_LIST_REQ, ud->connId, ud->connIdLen);
}

void CHallLogicHandler::viewGoldRankingReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in viewGoldRankingReply can not find the connect data", userName);
	if (conn == NULL) return;
	
	m_msgHandler->sendMsgToProxy(data, len, LoginSrvProtocol::LOGINSRV_GOLD_RANKING_LIST_RSP, conn);
}

void CHallLogicHandler::viewPhoneRankingReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in viewPhoneRankingReply can not find the connect data", userName);
	if (conn == NULL) return;
	
	m_msgHandler->sendMsgToProxy(data, len, LoginSrvProtocol::LOGINSRV_PHONE_RANKING_LIST_RSP, conn);
}

void CHallLogicHandler::getFishRateScore(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("get fish rate score")) return;
	
	com_protocol::ClientBuyuGetFishRateRsp rsp;
	const map<unsigned int, unsigned int>& fishReate = HallConfigData::config::getConfigValue().show_fish_rate;
	for (map<unsigned int, unsigned int>::const_iterator it = fishReate.begin(); it != fishReate.end(); ++it)
	{
		rsp.add_fish_rate(it->second);
	}
	
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "get fish rate score");
	if (msgBuffer != NULL) m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LoginSrvProtocol::LOGINSRV_GET_FISH_RATE_SCORE_RSP);
}

void CHallLogicHandler::getRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("get google recharge transaction")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->getRechargeInstance().getGoogleRechargeTransaction(data, len, ud->connId, ud->connIdLen, LOGINSRV_GET_GOOGLE_RECHARGE_RSP, m_msgHandler->getConnectProxyContext().conn);
}

void CHallLogicHandler::getRechargeTransactionReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in getRechargeTransactionReply can not find the connect data", userName);
	if (conn == NULL) return;
	
	m_msgHandler->getRechargeInstance().getGoogleRechargeTransactionReply(data, len, LOGINSRV_GET_GOOGLE_RECHARGE_RSP, conn);
}

void CHallLogicHandler::checkRechargeResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("check google recharge transaction")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->getRechargeInstance().checkGoogleRechargeResult(data, len, ud->connId, ud->connIdLen);
}

void CHallLogicHandler::checkRechargeResultReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in checkRechargeResultReply can not find the connect data", userName);
	if (conn == NULL) return;
	
	int result = -1;
	m_msgHandler->getRechargeInstance().checkGoogleRechargeResultReply(data, len, LOGINSRV_CHECK_GOOGLE_RECHARGE_RSP, conn, result);
	if (result == Success)
	{
		ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
		ud->hallLogicData->mutable_overseas_notify_info()->set_payment_secs(time(NULL));  // 更新充值时间点
	}
}

void CHallLogicHandler::overseasRechargeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in overseasRechargeNotify can not find the connect data", userName);
	if (conn == NULL) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	ud->hallLogicData->mutable_overseas_notify_info()->set_payment_secs(time(NULL));  // 更新充值时间点
}


ConnectProxy* CHallLogicHandler::getConnectInfo(const char* logInfo, string& userName)
{
	userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL) OptErrorLog("%s, user data = %s", logInfo, m_msgHandler->getContext().userData);
    return conn;
}

unsigned int CHallLogicHandler::getShareRewardIndex(com_protocol::HallLogicData* hallData)
{
	time_t curSecs = time(NULL);
	int theDay = localtime(&curSecs)->tm_yday;

	if (!hallData->share_info().has_today())
	{
		hallData->mutable_share_info()->set_today(theDay);
		hallData->mutable_share_info()->set_reward_count(0);
	}

	//隔天则重新累计领取次数
	else if (hallData->share_info().today() != (uint32_t)theDay)
	{
		hallData->mutable_share_info()->set_today(theDay);
		hallData->mutable_share_info()->set_reward_count(0);
	}

	return hallData->share_info().reward_count();
}

void CHallLogicHandler::getThirdPlatformRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get third platform recharge transaction")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->getRechargeInstance().getThirdRechargeTransaction(data, len, ud->connId, ud->connIdLen);
}

void CHallLogicHandler::getThirdPlatformRechargeTransactionReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in getThirdPlatformRechargeTransactionReply can not find the connect data", userName);
	if (conn == NULL) return;
	
	m_msgHandler->getRechargeInstance().getThirdRechargeTransactionReply(data, len, userName.c_str(), LOGINSRV_THIRD_RECHARGE_TRANSACTION_RSP, conn);
}

void CHallLogicHandler::thirdPlatformRechargeTransactionNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in thirdPlatformRechargeTransactionNotify can not find the connect data", userName);
	if (conn == NULL) return;
	
	int result = -1;
	m_msgHandler->getRechargeInstance().thirdRechargeTransactionNotify(data, len, userName.c_str(), LOGINSRV_THIRD_RECHARGE_TRANSACTION_NOTIFY, conn, result);
}

void CHallLogicHandler::cancelThirdPlatformRechargeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to cancel third platform recharge notify")) return;

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->getRechargeInstance().cancelThirdRechargeNotify(data, len, ud->connId, ud->connIdLen);
}

// 第三方账号检查
void CHallLogicHandler::checkThirdUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 先初始化连接
	if (m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn) == NULL) m_msgHandler->initConnect(m_msgHandler->getConnectProxyContext().conn);

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	int rc = m_msgHandler->sendMessageToService(ServiceType::HttpSrv, data, len, HttpServiceProtocol::CHECK_THIRD_USER_REQ, ud->connId, ud->connIdLen);
	OptInfoLog("receive message to check third user, rc = %d", rc);
}

void CHallLogicHandler::checkThirdUserReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in checkThirdUserReply can not find the connect data", userName);
	if (conn == NULL) return;
	
	int rc = m_msgHandler->sendMsgToProxy(data, len, LoginSrvProtocol::LOGINSRV_CHECK_THIRD_USER_RSP, conn);
	OptInfoLog("check third user result reply, rc = %d", rc);
}


// 小米账号检查
void CHallLogicHandler::checkXiaoMiUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 先初始化连接
	if (m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn) == NULL) m_msgHandler->initConnect(m_msgHandler->getConnectProxyContext().conn);

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	int rc = m_msgHandler->sendMessageToService(ServiceType::HttpSrv, data, len, HttpServiceProtocol::CHECK_XIAOMI_USER_REQ, ud->connId, ud->connIdLen);
	OptInfoLog("receive message to check xiao mi user, rc = %d", rc);
}

void CHallLogicHandler::checkXiaoMiUserReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in checkXiaoMiUserReply can not find the connect data", userName);
	if (conn == NULL) return;
	
	int rc = m_msgHandler->sendMsgToProxy(data, len, LoginSrvProtocol::LOGINSRV_CHECK_XIAOMI_USER_RSP, conn);
	OptInfoLog("check xiao mi user result reply, rc = %d", rc);
}

//查询腾讯充值是否成功
void CHallLogicHandler::findRechargeSuccess(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("in findRechargeSuccess can not find the connect data")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->getRechargeInstance().findTxRechargeSuccess(data, len, ud->connId, ud->connIdLen);
}

void CHallLogicHandler::findRechargeSuccessReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in findRechargeSuccessReply can not find the connect data", userName);
	if (conn == NULL) return;
	
	int rc = m_msgHandler->sendMsgToProxy(data, len, LoginSrvProtocol::LOGINSRV_FIND_RECHARGE_SUCCESS_RSP, conn);
	OptInfoLog("find recharge success reply, rc = %d", rc);
}


// 版本热更新
void CHallLogicHandler::checkVersionHotUpdate(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// if (!m_msgHandler->checkLoginIsSuccess("check version hot update")) return;
	
	enum
	{
		ENotHotUpdate = 0,         // 0：没有热更新（正常）；
		EHasHotUpdate = 1,         // 1：存在热更新（正常）； 
		EForceHotUpdate = 2,       // 2：当前版本不在配置范围之内，应该执行强制热更新（正常）；
	    EDeviceTypeError = 3,      // 3：设备类型错误； 
		EPlatformTypeError = 4,    // 4：平台类型错误； 
		EVersionURLError = 5,      // 5：当前版本号对应的URL配置错误
		EParseMsgError = 6,        // 6：消息解码错误
	};
	
	int result = Opt_Success;
	com_protocol::ClientVersionHotUpdateReq checkVersionHotUpdateReq;
	if (!m_msgHandler->parseMsgFromBuffer(checkVersionHotUpdateReq, data, len, "check version hot update request")) result = EParseMsgError;

	const string& curVersion = checkVersionHotUpdateReq.cur_version();
	com_protocol::ClientVersionHotUpdateRsp checkVersionHotUpdateRsq;
	checkVersionHotUpdateRsq.set_flag(ENotHotUpdate);
	checkVersionHotUpdateRsq.set_new_version("");
	checkVersionHotUpdateRsq.set_new_file_url("");

	string old_version;
	string new_version;
	int check_flag;
	string check_version;
	while (result == Opt_Success)
	{
		if (checkVersionHotUpdateReq.device_type() != ClientVersionType::AndroidMerge)
		{
			checkVersionHotUpdateRsq.set_flag(EDeviceTypeError);
			result = ClientHotUpdateVersionInvalid;
			break;
		}
		
		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		map<int, HallConfigData::HotUpdateVersion>::const_iterator hotUpdateCfgIt = cfg.android_merge_hot_update.find(checkVersionHotUpdateReq.platform_type());
		if (hotUpdateCfgIt == cfg.android_merge_hot_update.end())
		{
			checkVersionHotUpdateRsq.set_flag(EPlatformTypeError);
			result = ClientHotUpdateCfgError;
			break;
		}

		old_version = hotUpdateCfgIt->second.old_version;
		new_version = hotUpdateCfgIt->second.new_version;
		check_flag = hotUpdateCfgIt->second.check_flag;
		check_version = hotUpdateCfgIt->second.check_version;
		
		map<string, string>::const_iterator versionUrlIt = hotUpdateCfgIt->second.version_pkg_url.find(curVersion);
		if (versionUrlIt == hotUpdateCfgIt->second.version_pkg_url.end())
		{
			checkVersionHotUpdateRsq.set_flag(EVersionURLError);
			result = ClientHotUpdateVersionInvalid;
			break;
		}
		
		checkVersionHotUpdateRsq.set_new_version(new_version);
		checkVersionHotUpdateRsq.set_new_file_url(versionUrlIt->second);
		
		int flag = EHasHotUpdate;
		if (curVersion == new_version || (check_flag == 1 && curVersion == check_version)) flag = ENotHotUpdate;  // 已经是最新版本号，不存在热更新
		else if (curVersion < old_version || curVersion > new_version) flag = EForceHotUpdate;                    // 不在版本配置范围之内，需要强制热更新
		
		checkVersionHotUpdateRsq.set_flag(flag);
		if (flag == ENotHotUpdate)  checkVersionHotUpdateRsq.set_new_version(curVersion);

		break;
	}
	
	if (result != Opt_Success)
	{
		// OptErrorLog("check version hot update error, device type = %d, platform type = %d, current version = %s, result = %d, flag = %d",
		// checkVersionHotUpdateReq.device_type(), checkVersionHotUpdateReq.platform_type(), curVersion.c_str(), result, checkVersionHotUpdateRsq.flag());
		
		checkVersionHotUpdateRsq.set_flag(ENotHotUpdate);  // 错误则一律不能执行热更新
	}

    result = Opt_Failed;
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(checkVersionHotUpdateRsq, "check version hot update reply");
	if (msgBuffer != NULL) result = m_msgHandler->sendMsgToProxy(msgBuffer, checkVersionHotUpdateRsq.ByteSize(), LOGINSRV_VERSION_HOT_UPDATE_RSP);
	
	// OptInfoLog("check version hot update, ip = %s, device type = %d, platform type = %d, current version = %s, flag = %d, old version = %s, new version = %s, file url = %s, check flag = %d, version = %s, result = %d", CSocket::toIPStr(m_msgHandler->getConnectProxyContext().conn->peerIp), checkVersionHotUpdateReq.device_type(), checkVersionHotUpdateReq.platform_type(), curVersion.c_str(), checkVersionHotUpdateRsq.flag(), old_version.c_str(), new_version.c_str(), checkVersionHotUpdateRsq.new_file_url().c_str(), check_flag, check_version.c_str(), result);
}

// 系统消息已读通知
void CHallLogicHandler::readSystemMsgNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("read system message notify, can not find the connect data")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->sendMessageToService(ServiceType::MessageSrv, data, len, MessageSrvProtocol::BUSINESS_READ_SYSTEM_MSG_NOTIFY, ud->connId, ud->connIdLen);
}

// 比赛场正在进行中通知，掉线重连，重入游戏等通知
void CHallLogicHandler::arenaMatchOnNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("arena match on notify", userName);
	if (conn == NULL) return;

	com_protocol::GetArenaMatchPlayerInfoRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "arena match on notify")) return;
	
	const map<int, HallConfigData::Arena>& arena_cfg_map = HallConfigData::config::getConfigValue().arena_cfg_map;
	map<int, HallConfigData::Arena>::const_iterator it = arena_cfg_map.find(rsp.arena_id());  // 比赛类型
	if (it == arena_cfg_map.end()) return;
	
	char curDate[16] = {0};
	getCurrentDate(curDate, sizeof(curDate), "%d-%02d-%02d");
	map<string, HallConfigData::ArenaMatchDate>::const_iterator dateIt = it->second.match_date_map.find(curDate);
	if (dateIt == it->second.match_date_map.end()) return;  // 比赛日期
	
	map<int, HallConfigData::ArenaMatchTime>::const_iterator timeIt = dateIt->second.match_time_map.find(rsp.match_id());
	if (timeIt == dateIt->second.match_time_map.end()) return;  // 比赛时间点
	
	com_protocol::ClientArenaMatchOnNotify onMatchNotify;
	com_protocol::ClientArenaMatchBaseInfo* baseInfo = onMatchNotify.mutable_info();
	baseInfo->set_arena_id(rsp.arena_id());
	baseInfo->set_arena_name(it->second.name);
	baseInfo->set_match_id(rsp.match_id());
	baseInfo->set_start_time(timeIt->second.start);
	baseInfo->set_finish_time(timeIt->second.finish);
	baseInfo->set_vip(timeIt->second.vip);
	baseInfo->set_gold(timeIt->second.gold);
	baseInfo->set_bullet_rate(timeIt->second.max_bullet_rate);
	
	// 比赛场服务信息
	com_protocol::ClientArenaServiceInfo* serviceInfo = onMatchNotify.mutable_service();
	serviceInfo->set_ip(rsp.ip());
	serviceInfo->set_port(rsp.port());
	serviceInfo->set_service_id(srcSrvId);
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(onMatchNotify, "arena match on notify");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, onMatchNotify.ByteSize(), LoginSrvProtocol::LOGINSRV_ARENA_MATCH_ON_NOTIFY, conn);
}

// 用户断线重连，重新登录游戏后取消比赛通知
void CHallLogicHandler::cancelArenaMatchNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("cancel arena match notify")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	com_protocol::HallData hallData;
	if (m_msgHandler->getArenaServiceFromRedis(ud, hallData))
	{
		// 通知比赛服务用户主动取消比赛了
		m_msgHandler->sendMessage(data, len, ud->connId, ud->connIdLen, hallData.rooms(0).id(), CommonProtocol::COMMON_CANCEL_ARENA_MATCH_NOTIFY);
	}
}

// 获取象棋服务信息
void CHallLogicHandler::getXiangQiServiceInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get xiangqi game service informaiton")) return;

	com_protocol::ClientGetHallRsp getHallRsp;
	getHallRsp.set_result(0);
	com_protocol::HallData* hallData = getHallRsp.mutable_hall_info();
	if (m_msgHandler->getXiangQiServiceFromRedis((ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn), *hallData))
	{
		const char* msgData = m_msgHandler->serializeMsgToBuffer(getHallRsp, "get xiangqi game service data");
        if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, getHallRsp.ByteSize(), LOGINSRV_GET_XIANGQI_SERVICE_INFO_RSP);
	}
}

// 获取新手指引信息
void CHallLogicHandler::getNewPlayerGuideInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get new player guide informaiton")) return;

	com_protocol::ClientGetNewPlayerGuideRsp rsp;
	com_protocol::HallLogicData* hlData = m_msgHandler->getHallLogicData();
	if (!hlData->has_new_player_guide() || hlData->new_player_guide().flag() != 1) rsp.set_flag(1);
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "get new player guide data");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_GET_NEW_PLAYER_GUIDE_RSP);
}
	
// 新手引导流程已经完毕则通知服务器
void CHallLogicHandler::finishNewPlayerGuideNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to notify finish new player guide")) return;
	
	com_protocol::HallLogicData* hlData = m_msgHandler->getHallLogicData();
	hlData->mutable_new_player_guide()->set_flag(1);
}

// 获取小游戏服务信息
void CHallLogicHandler::getGameServiceInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get game service informaiton")) return;

	com_protocol::ClientGetServiceInfoRsp getGameServiceRsp;
	com_protocol::HallData hallData;
	
	ConnectUserData* cud = m_msgHandler->getConnectUserData();
	if (m_msgHandler->getXiangQiServiceFromRedis(cud, hallData))             // 象棋翻翻
	{
		*getGameServiceRsp.add_service_info() = hallData.rooms(0);
		hallData.Clear();
	}
	
	if (m_msgHandler->getPokerCarkServiceFromRedis(cud, hallData))           // 扑鱼卡牌
	{
		*getGameServiceRsp.add_service_info() = hallData.rooms(0);
		hallData.Clear();
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(getGameServiceRsp, "get game service data");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, getGameServiceRsp.ByteSize(), LOGINSRV_GET_GAME_SERVICE_INFO_RSP);
}

// 韩文版HanYou平台使用优惠券通知
void CHallLogicHandler::useHanYouCouponNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login success message to use Han You coupon notify")) return;
	
	m_msgHandler->sendMessageToCommonDbProxy(data, len, BUSINESS_USE_HAN_YOU_COUPON_NOTIFY);
}

// 韩文版HanYou平台使用优惠券获得的奖励通知
void CHallLogicHandler::addHanYouCouponRewardNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("add Han You coupon reward notify", userName);
	if (conn == NULL) return;

	m_msgHandler->sendMsgToProxy(data, len, LoginSrvProtocol::LOGINSRV_ADD_HAN_YOU_COUPON_REWARD_NOTIFY, conn);
}

// 自动积分兑换物品
void CHallLogicHandler::autoScoreExchangeItem(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login success message to auto score exchange item")) return;
	
	m_msgHandler->sendMessageToCommonDbProxy(data, len, BUSINESS_AUTO_SCORE_EXCHANGE_ITEM_REQ);
}

void CHallLogicHandler::autoScoreExchangeItemReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("auto score exchange item reply", userName);
	if (conn == NULL) return;

	m_msgHandler->sendMsgToProxy(data, len, LoginSrvProtocol::LOGINSRV_AUTO_SCORE_EXCHANGE_ITEM_RSP, conn);
}

}


