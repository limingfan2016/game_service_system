
/* author : limingfan
 * date : 2016.08.11
 * description : 逻辑数据处理
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
#include "CLogicHandlerTwo.h"
#include "base/Function.h"
#include "../common/CommonType.h"
#include "_HallConfigData_.h"


using namespace NProject;


namespace NPlatformService
{

// PK场金币门票期限时间排序函数
static bool pkGoldTicketTimeSort(const com_protocol::Ticket* info1, const com_protocol::Ticket* info2)
{
	return (info1->end_time() < info2->end_time());
}


// 红包奖励数据
struct RedGiftRewardData
{
	char username[32];
	unsigned int condition_type;
	unsigned int condition_value;
	unsigned int reward_type;
	unsigned int reward_count;
};


CLogicHandlerTwo::CLogicHandlerTwo()
{
	m_msgHandler = NULL;
	
	m_updateArenaConfigDataCurrentDay = 0;
}

CLogicHandlerTwo::~CLogicHandlerTwo()
{
	m_msgHandler = NULL;
}

void CLogicHandlerTwo::load(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;
	
	// 游戏内转发过来的通知消息
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonProtocol::GAME_FORWARD_MESSAGE_TO_HALL_NOTIFY, (ProtocolHandler)&CLogicHandlerTwo::gameForwardMessageNotify, this);
	
	// 红包口令
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_RED_GIFT_INFO_REQ, (ProtocolHandler)&CLogicHandlerTwo::getRedGiftInfoReq, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_RECEIVE_RED_GIFT_REQ, (ProtocolHandler)&CLogicHandlerTwo::receiveRedGiftReq, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_VIEW_RED_GIFT_REWARD_REQ, (ProtocolHandler)&CLogicHandlerTwo::viewRedGiftRewardReq, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_RECEIVE_RED_GIFT_REWARD_REQ, (ProtocolHandler)&CLogicHandlerTwo::receiveRedGiftRewardReq, this);
	
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_ADD_RED_GIFT_RSP, (ProtocolHandler)&CLogicHandlerTwo::getRedGiftInfoReply, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_RECEIVE_RED_GIFT_RSP, (ProtocolHandler)&CLogicHandlerTwo::receiveRedGiftReply, this);

	m_msgHandler->registerProtocol(LoginSrv, CUSTOM_GET_MORE_USER_DATA_FOR_VIEW_RED_GIFT_REWARD, (ProtocolHandler)&CLogicHandlerTwo::viewRedGiftRewardReply, this);
	m_msgHandler->registerProtocol(LoginSrv, CUSTOM_GET_HALL_DATA_FOR_RED_GIFT_REWARD, (ProtocolHandler)&CLogicHandlerTwo::addOfflineUserRedGiftReward, this);
	
	m_msgHandler->registerProtocol(LoginSrv, CUSTOM_GET_RED_GIFT_FRIEND_STATIC_DATA, (ProtocolHandler)&CLogicHandlerTwo::addRedGiftFriendStaticDataReply, this);
	m_msgHandler->registerProtocol(LoginSrv, CUSTOM_GET_HALL_DATA_FOR_RED_GIFT_FRIEND, (ProtocolHandler)&CLogicHandlerTwo::addRedGiftFriendLogicDataReply, this);
	
	// 背包
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_KNAPSACK_INFO_REQ, (ProtocolHandler)&CLogicHandlerTwo::getKnapsackInfoReq, this);
	m_msgHandler->registerProtocol(LoginSrv, CUSTOM_GET_KNAPSACK_INFO_FROM_COMMON_DB, (ProtocolHandler)&CLogicHandlerTwo::getKnapsackInfoReply, this);
	
	m_msgHandler->registerProtocol(ServiceType::MessageSrv, MessageSrvProtocol::BUSINESS_FORCE_PLAYER_QUIT_NOTIFY, (ProtocolHandler)&CLogicHandlerTwo::forceUserQuitNotify, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_BINDING_EXCHANGE_MALL_INFO_REQ, (ProtocolHandler)&CLogicHandlerTwo::bindExchangeMallInfoReq, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_BINDING_EXCHANGE_MALL_INFO_RSP, (ProtocolHandler)&CLogicHandlerTwo::bindExchangeMallInfoReply, this);
	
	// 获取比赛场信息
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_ARENA_INFO_REQ, (ProtocolHandler)&CLogicHandlerTwo::getArenaInfoReq, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_DETAILED_MATCH_INFO_REQ, (ProtocolHandler)&CLogicHandlerTwo::getDetailedMatchInfoReq, this);
	m_msgHandler->registerProtocol(ServiceType::MessageSrv, BUSINESS_GET_ARENA_RANKING_RSP, (ProtocolHandler)&CLogicHandlerTwo::getArenaRankingInfoReply, this);
	m_msgHandler->registerProtocol(LoginSrv, CUSTOM_GET_USERS_INFO_FOR_ARENA_RANKING, (ProtocolHandler)&CLogicHandlerTwo::getUsersInfoForArenaRanking, this);
	m_msgHandler->registerProtocol(CommonSrv, COMMON_ARENA_MATCH_CONFIG_INFO_NOTIFY, (ProtocolHandler)&CLogicHandlerTwo::updateArenaMatchConfigData, this);
	m_msgHandler->registerProtocol(CommonSrv, COMMON_GET_ARENA_RANKING_REWARD_REQ, (ProtocolHandler)&CLogicHandlerTwo::getArenaRankingRewardReq, this);
	
	m_msgHandler->registerProtocol(BuyuGameSrv, COMMON_GET_POLER_CARK_SERVICE_INFO_RSP, (ProtocolHandler)&CLogicHandlerTwo::getBuyuPokerCarkServiceInfoReply, this);
	
	// 邮箱信息
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_MAIL_MESSAGE_REQ, (ProtocolHandler)&CLogicHandlerTwo::getMailMessage, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_MAIL_MESSAGE_RSP, (ProtocolHandler)&CLogicHandlerTwo::getMailMessageReply, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_OPT_MAIL_MESSAGE_REQ, (ProtocolHandler)&CLogicHandlerTwo::optMailMessage, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_OPT_MAIL_MESSAGE_RSP, (ProtocolHandler)&CLogicHandlerTwo::optMailMessageReply, this);
}

void CLogicHandlerTwo::unLoad()
{

}

void CLogicHandlerTwo::updateConfig()  // 配置文件更新
{
	m_isNeedUpdateArenaConfigData.clear();
}

// 消息中心转发过来的通知消息
void CLogicHandlerTwo::gameForwardMessageNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::LoginForwardMessageNotify forwardMsgNotify;
	if (!m_msgHandler->parseMsgFromBuffer(forwardMsgNotify, data, len, "receive forward message notify")) return;
	
	switch (forwardMsgNotify.type())
	{
		case ELoginForwardMessageNotifyType::EPlayerUpLevel:  // 等级升级
		{
			checkRedGiftCondition(forwardMsgNotify.type(), forwardMsgNotify.player_level());
			break;
		}
		
		case ELoginForwardMessageNotifyType::EBatteryUpLevel:  // 炮台解锁升级
		{
			checkRedGiftCondition(forwardMsgNotify.type(), forwardMsgNotify.battery_rate());
			break;
		}
		
		case ELoginForwardMessageNotifyType::ERechargeValue:  // 充值操作
		{
			m_msgHandler->getActivityCenter().userRechargeNotify(m_msgHandler->getContext().userData);
			checkRedGiftCondition(forwardMsgNotify.type(), forwardMsgNotify.recharge_value());
			break;
		}
		
		case ELoginForwardMessageNotifyType::EAddRedGiftReward:  // 添加红包奖励信息
		{
			addRedGiftReward(forwardMsgNotify.data().data(), forwardMsgNotify.data().length());
			break;
		}
		
		default:
		{
			OptErrorLog("receive forward message notify, error type = %d, srcSrvId = %d", forwardMsgNotify.type(), srcSrvId);
			break;
		}
	}
}
	
	
// 红包口令相关
void CLogicHandlerTwo::sendRedGiftInfoToClient(const com_protocol::HallLogicData* hallLogicData, const char* userName)
{
	const HallConfigData::RedGiftWord& cfg = HallConfigData::config::getConfigValue().red_gift_word;
	const com_protocol::RedGift& redGift = hallLogicData->red_gfit();
	
	com_protocol::ClientGetRedGiftInfoRsp rsp;
	rsp.set_gift_id(redGift.gift_id());
	rsp.set_phone_fare_value(cfg.reward_value);
	rsp.set_is_receive_gift((redGift.has_received_gift_user() && !redGift.received_gift_user().empty()) ? 1 : 0);  // 是否已经领取过红包了，0：未领取；  1：已经领取过红包了
	
	// 红包好友个数
	unsigned int redGiftFriends = 0;
	const com_protocol::FriendData& friendData = hallLogicData->friends();
	for (int idx = 0; idx < friendData.flag_size(); ++idx)
	{
		if (friendData.flag(idx) == 1) ++redGiftFriends;
	}
	rsp.set_red_gift_friends(redGiftFriends);
	
	for (vector<HallConfigData::RedGiftRewardCondition>::const_iterator rIt = cfg.receive_reward_condition.begin(); rIt != cfg.receive_reward_condition.end(); ++rIt)
	{
		com_protocol::RedGiftRewardInfo* rewards = rsp.add_reward_list();
		rewards->set_item_type(rIt->reward_id);
		rewards->set_item_num(rIt->reward_count);
		rewards->set_condition_desc(rIt->condition_desc);
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "get red gift info reply");
	if (msgData != NULL)
	{
		if (userName == NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LoginSrvProtocol::LOGINSRV_GET_RED_GIFT_INFO_RSP);
		else m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LoginSrvProtocol::LOGINSRV_GET_RED_GIFT_INFO_RSP, userName);
	}
}

void CLogicHandlerTwo::getRedGiftInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get red gift info")) return;
	
	const com_protocol::HallLogicData* hallLogicData = m_msgHandler->getHallLogicData();
	if (!hallLogicData->has_red_gfit())
	{
		// 第一次获取红包信息
		m_msgHandler->sendMessageToCommonDbProxy(NULL, 0, CommonDBSrvProtocol::BUSINESS_ADD_RED_GIFT_REQ);
		return;
	}
	
	sendRedGiftInfoToClient(hallLogicData);
}

void CLogicHandlerTwo::getRedGiftInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::AddRedGiftRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "add red gift reply")) return;
	
	com_protocol::HallLogicData* hallLogicData = m_msgHandler->getHallLogicData(m_msgHandler->getContext().userData);
	if (hallLogicData != NULL && rsp.result() == Opt_Success)
	{
		hallLogicData->mutable_red_gfit()->set_gift_id(rsp.red_gift_id());
		sendRedGiftInfoToClient(hallLogicData, m_msgHandler->getContext().userData);
	}
	else if (rsp.result() != Opt_Success)
	{
		OptErrorLog("add red gift error, user = %s, result = %d", m_msgHandler->getContext().userData, rsp.result());
	}
}

void CLogicHandlerTwo::receiveRedGiftReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to receive red gift")) return;
	
	const com_protocol::HallLogicData* hallLogicData = m_msgHandler->getHallLogicData();
	if (!hallLogicData->has_red_gfit())
	{
		OptErrorLog("receive red gift but not found error, user = %s", m_msgHandler->getConnectUserData()->connId);
		return;
	}

	if (hallLogicData->red_gfit().has_received_gift_user())
	{
		com_protocol::ClientReceiveRedGiftRsp rsp;
		rsp.set_result(ReceiveRedGift);
		const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "received red gift reply");
		if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LoginSrvProtocol::LOGINSRV_RECEIVE_RED_GIFT_RSP);
		
		return;
	}

	com_protocol::ClientReceiveRedGiftReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "receive red gift request")) return;
	
	com_protocol::ReceiveRedGiftReq receiveReq;
	receiveReq.set_device_id(req.device_id());
	receiveReq.set_red_gift_id(req.red_gift_id());
	com_protocol::GiftInfo* giftInfo = receiveReq.mutable_gift_info();
	giftInfo->set_type(HallConfigData::config::getConfigValue().red_gift_word.reward_type);
	giftInfo->set_num(HallConfigData::config::getConfigValue().red_gift_word.reward_value);
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(receiveReq, "send receive red gift msg to db common");
	if (msgData != NULL) m_msgHandler->sendMessageToCommonDbProxy(msgData, receiveReq.ByteSize(), CommonDBSrvProtocol::BUSINESS_RECEIVE_RED_GIFT_REQ);
}

void CLogicHandlerTwo::receiveRedGiftReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ReceiveRedGiftRsp receiveRsp;
	if (!m_msgHandler->parseMsgFromBuffer(receiveRsp, data, len, "receive red gift reply")) return;
	
	ConnectUserData* cud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxy(m_msgHandler->getContext().userData));
	if (cud == NULL || cud->hallLogicData == NULL) return;  // 玩家下线了

	if (receiveRsp.has_red_gift_id() && receiveRsp.has_username())  // 领取了红包了
	{
	    com_protocol::RedGift* redGift = cud->hallLogicData->mutable_red_gfit();
		redGift->set_received_gift_id(receiveRsp.red_gift_id());
		redGift->set_received_gift_user(receiveRsp.username());
		
		if (receiveRsp.result() == Opt_Success)
		{
			makeToFriend(cud, receiveRsp.username().c_str());
			
            const HallConfigData::RedGiftWord& redGiftCfg = HallConfigData::config::getConfigValue().red_gift_word;
			m_msgHandler->getStatDataMgr().addStatGoods(EStatisticsDataItem::EReceivedRedGift, redGiftCfg.reward_type, redGiftCfg.reward_value);  // 统计数据
		}
	}

	com_protocol::ClientReceiveRedGiftRsp clientRsp;
	clientRsp.set_result(receiveRsp.result());
	if (receiveRsp.result() == Opt_Success) clientRsp.set_nickname(receiveRsp.nickname());
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(clientRsp, "received red gift reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, clientRsp.ByteSize(), LoginSrvProtocol::LOGINSRV_RECEIVE_RED_GIFT_RSP, m_msgHandler->getContext().userData);
}

void CLogicHandlerTwo::viewRedGiftRewardReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to view red gift reward")) return;
	
	const com_protocol::HallLogicData* hallLogicData = m_msgHandler->getHallLogicData();
	if (!hallLogicData->has_red_gfit())
	{
		OptErrorLog("view red gift reward but not found error, user = %s", m_msgHandler->getConnectUserData()->connId);
		return;
	}
	
	const com_protocol::RedGift& redGift = hallLogicData->red_gfit();
	com_protocol::GetMultiUserInfoReq getNickNameReq;
	for (int idx = 0; idx < redGift.not_receive_rewards_size(); ++idx)
	{
		getNickNameReq.add_username_lst(redGift.not_receive_rewards(idx).username());
	}
	
	for (int idx = 0; idx < redGift.received_rewards_size(); ++idx)
	{
		getNickNameReq.add_username_lst(redGift.received_rewards(idx).username());
	}
	
	if (getNickNameReq.username_lst_size() > 0)
	{
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(getNickNameReq, "get user nick name for view red gift reward request");
		if (msgBuffer != NULL)
		{
			ConnectUserData* ud = m_msgHandler->getConnectUserData();
			m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgBuffer, getNickNameReq.ByteSize(), CommonDBSrvProtocol::BUSINESS_GET_MULTI_USER_INFO_REQ,
			ud->connId, ud->connIdLen, 0, EServiceReplyProtocolId::CUSTOM_GET_MORE_USER_DATA_FOR_VIEW_RED_GIFT_REWARD);
		}
		
		return;
	}
	
	// 没有任何奖励数据信息
	m_msgHandler->sendMsgToProxy(NULL, 0, LoginSrvProtocol::LOGINSRV_VIEW_RED_GIFT_REWARD_RSP);
}

void CLogicHandlerTwo::viewRedGiftRewardReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const com_protocol::HallLogicData* hallLogicData = m_msgHandler->getHallLogicData(m_msgHandler->getContext().userData);
	if (hallLogicData == NULL) return;  // 用户下线了
	
	com_protocol::GetMultiUserInfoRsp getNickNameRsp;
	if (!m_msgHandler->parseMsgFromBuffer(getNickNameRsp, data, len, "get nick name for view red gift reward reply")) return;
	
	std::unordered_map<string, string> username2nickname;
	for (int idx = 0; idx < getNickNameRsp.user_simple_info_lst_size(); ++idx)
	{
		username2nickname[getNickNameRsp.user_simple_info_lst(idx).username()] = getNickNameRsp.user_simple_info_lst(idx).nickname();
	}
	
	// 条件类型  1：等级升级；  2：炮台等级解锁；  3：充值操作
	const char* type2desc[] = {"", "%s等级升到%d级", "%s解锁%d级炮台", "%s充值%d元"};
	const unsigned int typeDescCount = sizeof(type2desc) / sizeof(type2desc[0]);
	
	char dataBuffer[128] = {0};
	com_protocol::ClientViewRedGiftRewardRsp clientRsp;
	const com_protocol::RedGift& redGift = hallLogicData->red_gfit();
	
	typedef google::protobuf::RepeatedPtrField<com_protocol::RedGiftReward> RedGiftRewardList;
	const RedGiftRewardList* redGiftRewardList[] = {&redGift.not_receive_rewards(), &redGift.received_rewards()};

	for (unsigned int idx = 0; idx < sizeof(redGiftRewardList) / sizeof(redGiftRewardList[0]); ++idx)
	{
		const RedGiftRewardList* rwData = redGiftRewardList[idx];
		for (int size = rwData->size(); size > 0; --size)
		{
			const com_protocol::RedGiftReward& rw = rwData->Get(size - 1);
			std::unordered_map<string, string>::const_iterator it = username2nickname.find(rw.username());
			if (it == username2nickname.end() || rw.condition_type() >= typeDescCount)
			{
				OptErrorLog("view red gift reward error, user = %s, username = %s, condition type = %d",
				m_msgHandler->getContext().userData, rw.username().c_str(), rw.condition_type());
				continue;  // 找不到对应的信息
			}
			
			time_t finishTimeSecs = rw.time_secs() ;
			struct tm* pTm = localtime(&finishTimeSecs);
			snprintf(dataBuffer, sizeof(dataBuffer) - 1, "%d%02d%02d", (pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday);
			
			com_protocol::RedGiftRewardInfo* rewards = clientRsp.add_reward_list();
			rewards->set_time_desc(dataBuffer);       // 时间
	
			snprintf(dataBuffer, sizeof(dataBuffer) - 1, type2desc[rw.condition_type()], it->second.c_str(), rw.condition_value());
			rewards->set_condition_desc(dataBuffer);  // 条件描述
			
			const com_protocol::GiftInfo& gInfo = rw.rewards(0);
			rewards->set_item_type(gInfo.type());
			rewards->set_item_num(gInfo.num());
			rewards->set_flag(idx);  // 领奖标识，0：未领取；  1：已经领取过了
		}
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(clientRsp, "view red gift reward reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, clientRsp.ByteSize(), LoginSrvProtocol::LOGINSRV_VIEW_RED_GIFT_REWARD_RSP, m_msgHandler->getContext().userData);
}

void CLogicHandlerTwo::receiveRedGiftRewardReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to receive red gift reward")) return;
	
	const com_protocol::HallLogicData* hallLogicData = m_msgHandler->getHallLogicData();
	if (!hallLogicData->has_red_gfit())
	{
		OptErrorLog("receive red gift reward but not found error, user = %s", m_msgHandler->getConnectUserData()->connId);
		return;
	}
	
	NProject::RecordItemVector recordItemVector;
	const com_protocol::RedGift& redGift = hallLogicData->red_gfit();
	for (int index = 0; index < redGift.not_receive_rewards_size(); ++index)
	{
		bool isFind = false;
		const com_protocol::GiftInfo& gInfo = redGift.not_receive_rewards(index).rewards(0);	
		for (unsigned int idx = 0; idx < recordItemVector.size(); ++idx)
		{
			if (recordItemVector[idx].type == gInfo.type())
			{
				recordItemVector[idx].count += gInfo.num();
				isFind = true;
				break;
			}
		}
		
		if (!isFind) recordItemVector.push_back(RecordItem(gInfo.type(), gInfo.num()));
	}

    ConnectUserData* ud = m_msgHandler->getConnectUserData();
	m_msgHandler->notifyDbProxyChangeProp(ud->connId, ud->connIdLen, recordItemVector, EPropFlag::RedGiftReward, "红包口令奖励");
}

void CLogicHandlerTwo::receiveRedGiftRewardReply(const int result, ConnectProxy* conn)
{
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	if (ud != NULL)
	{
		com_protocol::ClientReceiveRedGiftRewardsRsp clientRsp;
		clientRsp.set_result(result);
		
		if (result == Opt_Success)
		{
			std::unordered_map<unsigned int, unsigned int> receiveItems;
			com_protocol::RedGift* redGift = ud->hallLogicData->mutable_red_gfit();
			for (int idx = 0; idx < redGift->not_receive_rewards_size(); ++idx)
			{
				com_protocol::RedGiftReward* rgRd = redGift->add_received_rewards();
				*rgRd = redGift->not_receive_rewards(idx);  // 加到已领取列表
				
				const com_protocol::GiftInfo& gInfo = rgRd->rewards(0);
				receiveItems[gInfo.type()] = receiveItems[gInfo.type()] + gInfo.num();
			}
			redGift->clear_not_receive_rewards();  // 未领取列表清空
			
			for (auto item = receiveItems.begin(); item != receiveItems.end(); ++item)
			{
				auto changeProp = clientRsp.add_received_gifts();
				changeProp->set_type(item->first);
				changeProp->set_num(item->second);
			}
		}

		const char* msgData = m_msgHandler->serializeMsgToBuffer(clientRsp, "receive red gift reward reply");
	    if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, clientRsp.ByteSize(), LoginSrvProtocol::LOGINSRV_RECEIVE_RED_GIFT_REWARD_RSP, conn);
	}
}

void CLogicHandlerTwo::checkRedGiftCondition(const unsigned int type, const unsigned int value)
{
	const com_protocol::HallLogicData* hallLogicData = m_msgHandler->getHallLogicData(m_msgHandler->getContext().userData);
	if (hallLogicData == NULL) return;  // 用户不在线了，异常情况不用处理了
	
	// 用户没有领取过红包则不处理
	if (!hallLogicData->has_red_gfit() || !(hallLogicData->red_gfit().has_received_gift_user()) || hallLogicData->red_gfit().received_gift_user().empty()) return;
			
	// 先看条件是否满足
	const vector<HallConfigData::RedGiftRewardCondition>& rewardList = HallConfigData::config::getConfigValue().red_gift_word.receive_reward_condition;
	for (vector<HallConfigData::RedGiftRewardCondition>::const_iterator it = rewardList.begin(); it != rewardList.end(); ++it)
	{
		if (it->condition_type == type && (it->condition_count == value || ELoginForwardMessageNotifyType::ERechargeValue == type))  // 满足条件了
		{
			// 如果是充值操作，则value的单位是RMB元，需要转换成RMB角，然后乘上配置的百分比值
			RedGiftRewardData redGiftRewardData;
			redGiftRewardData.reward_count = (ELoginForwardMessageNotifyType::ERechargeValue != type) ? it->reward_count : ((value * 10 * it->reward_count) / 100);
			if (redGiftRewardData.reward_count > 0)
			{
				redGiftRewardData.reward_type = it->reward_id;
				redGiftRewardData.condition_type = type;
				redGiftRewardData.condition_value = value;
				strncpy(redGiftRewardData.username, m_msgHandler->getContext().userData, sizeof(redGiftRewardData.username) - 1);

                // 转发消息到用户所在的大厅服务
				com_protocol::LoginForwardMessageNotify loginForwardMsgNotify;
				loginForwardMsgNotify.set_type(ELoginForwardMessageNotifyType::EAddRedGiftReward);
				loginForwardMsgNotify.set_data((const char*)&redGiftRewardData, sizeof(redGiftRewardData));
				const char* msgData = m_msgHandler->serializeMsgToBuffer(loginForwardMsgNotify, "forward message to add red gift reward");
				if (msgData != NULL)
				{
					const string& username = hallLogicData->red_gfit().received_gift_user();
					forwardMessageToLogin(username.c_str(), username.length(), msgData, loginForwardMsgNotify.ByteSize(), CommonProtocol::GAME_FORWARD_MESSAGE_TO_HALL_NOTIFY);
				}
			}
			
			break;
		}
	}
}

// 转发消息到游戏大厅
void CLogicHandlerTwo::forwardMessageToLogin(const char* username, const unsigned int uLen, const char* data, const unsigned int dLen, unsigned short dstProtocol, unsigned short srcProtocolId)
{
	// 转发充值消息到大厅
	com_protocol::ForwardMessageToServiceReq forwardMsgToHallReq;
	forwardMsgToHallReq.set_user_name(username, uLen);
	forwardMsgToHallReq.set_data(data, dLen);     // 协议数据
	forwardMsgToHallReq.set_dst_service_type(ServiceType::LoginSrv);
	forwardMsgToHallReq.set_dst_protocol(dstProtocol);
	forwardMsgToHallReq.set_src_service_id(m_msgHandler->getSrvId());
	forwardMsgToHallReq.set_src_protocol(srcProtocolId);
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(forwardMsgToHallReq, "forward message to user online hall");
	if (msgData != NULL)
	{
		m_msgHandler->sendMessageToService(ServiceType::MessageSrv, msgData, forwardMsgToHallReq.ByteSize(), MessageSrvProtocol::BUSINESS_FORWARD_MESSAGE_TO_SERVICE_REQ, username, uLen);
	}
}

// 添加红包奖励
void CLogicHandlerTwo::addRedGiftReward(com_protocol::HallLogicData* hallLogicData, const RedGiftRewardData* rgRD)
{
	com_protocol::RedGift* redGift = hallLogicData->mutable_red_gfit();
	com_protocol::RedGiftReward* noReceiveReward = redGift->add_not_receive_rewards();
	noReceiveReward->set_time_secs(time(NULL));
	noReceiveReward->set_username(rgRD->username);
	noReceiveReward->set_condition_type(rgRD->condition_type);
	noReceiveReward->set_condition_value(rgRD->condition_value);
	com_protocol::GiftInfo* gInfo = noReceiveReward->add_rewards();
	gInfo->set_type(rgRD->reward_type);
	gInfo->set_num(rgRD->reward_count);
	
	m_msgHandler->getStatDataMgr().addStatGoods(EStatisticsDataItem::ERedGiftTakeRatio, rgRD->reward_type, rgRD->reward_count);  // 统计数据
	
	// 超出最大个数则删除最早的
	const int needRemoveCount = redGift->not_receive_rewards_size() + redGift->received_rewards_size() - HallConfigData::config::getConfigValue().red_gift_word.max_reward_count;
	if (needRemoveCount > 0)
	{
		if (redGift->received_rewards_size() >= needRemoveCount)
		{
			redGift->mutable_received_rewards()->DeleteSubrange(0, needRemoveCount);  // 优先删除已经领取了的奖励
		}
        else
		{
			redGift->mutable_not_receive_rewards()->DeleteSubrange(0, needRemoveCount - redGift->received_rewards_size());
			redGift->clear_received_rewards();
		}
	}
}

void CLogicHandlerTwo::addRedGiftReward(const char* data, const unsigned int len)
{
	com_protocol::HallLogicData* hallLogicData = m_msgHandler->getHallLogicData(m_msgHandler->getContext().userData);
	if (hallLogicData == NULL)
	{
		// 获取离线玩家的数据
		m_msgHandler->getHallLogic().getUserHallLogicData(m_msgHandler->getContext().userData, 0, EServiceReplyProtocolId::CUSTOM_GET_HALL_DATA_FOR_RED_GIFT_REWARD, data, len);
		return;
	}
	
	addRedGiftReward(hallLogicData, (const RedGiftRewardData*)data);
}

void CLogicHandlerTwo::addOfflineUserRedGiftReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetGameDataRsp getHallLogicDataRsp;
	if (!m_msgHandler->parseMsgFromBuffer(getHallLogicDataRsp, data, len, "add offline user red gift reward")) return;

	if(getHallLogicDataRsp.result() != Opt_Success)
	{
		OptErrorLog("add offline user red gift reward error, result = %d", getHallLogicDataRsp.result());
		return;
	}

	com_protocol::HallLogicData hallLogicData;
	if (!m_msgHandler->parseMsgFromBuffer(hallLogicData, getHallLogicDataRsp.data().c_str(), getHallLogicDataRsp.data().length(), "get hall data to add red gift reward")) return;

    addRedGiftReward(&hallLogicData, (const RedGiftRewardData*)getHallLogicDataRsp.tc_data().data());
	
	// 向Game DB刷新用户数据
	const char* msgData = m_msgHandler->serializeMsgToBuffer(hallLogicData, "add red gift reward to logic data");
	if (msgData != NULL)
	{
		com_protocol::SetGameDataReq saveLogicData;
		saveLogicData.set_primary_key(HallLogicDataKey);
		saveLogicData.set_second_key(getHallLogicDataRsp.second_key());
		saveLogicData.set_data(msgData, hallLogicData.ByteSize());
		
		msgData = m_msgHandler->serializeMsgToBuffer(saveLogicData, "set hall red gift reward data to db");
		if (msgData != NULL) m_msgHandler->sendMessageToGameDbProxy(msgData, saveLogicData.ByteSize(), SET_GAME_DATA_REQ, "", 0);
	}
}

bool CLogicHandlerTwo::addRedGiftFriend(com_protocol::HallLogicData* hallLogicData, const char* friendName)
{
	com_protocol::FriendData* friendData = hallLogicData->mutable_friends();
	
	// 兼容新老版本数据处理
	const int needAddCount = friendData->username_size() - friendData->flag_size();
	for (int idx = 0; idx < needAddCount; ++idx) friendData->add_flag(0);  // 好友标识，0:非红包好友， 1:红包好友
	
	bool isFind = false;
	for (int idx = 0; idx < friendData->username_size(); ++idx)
	{
		if (friendData->username(idx) == friendName)
		{
			friendData->set_flag(idx, 1);  // 好友标识，0:非红包好友， 1:红包好友
			isFind = true;
        	break;
		}
	}
	
	if (!isFind)
	{
		friendData->add_username(friendName);
		friendData->add_flag(1);  // 好友标识，0:非红包好友， 1:红包好友
	}
	
	return isFind;
}

void CLogicHandlerTwo::makeToFriend(ConnectUserData* cud, const char* friendName)
{
	bool isFind = addRedGiftFriend(cud->hallLogicData, friendName);
	
	// 当前好友在线
	ConnectUserData* friendUd = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxy(friendName));
	if (friendUd != NULL)
	{
		com_protocol::ClientAddFriendRsp addFriendNotify;
		addFriendNotify.set_result(0);
		
		if (!isFind)
		{
			addFriendNotify.set_dst_username(friendUd->connId);
			addFriendNotify.set_dst_nickname(friendUd->nickname);
			addFriendNotify.set_dst_portrait_id(friendUd->portrait_id);
			
			const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(addFriendNotify, "add red gift friend notify client");
			if (msgBuffer != NULL) m_msgHandler->sendMsgToProxy(msgBuffer, addFriendNotify.ByteSize(), LOGINSRV_ADD_FRIEND_NOTIFY, m_msgHandler->getContext().userData);
		}
		
		isFind = addRedGiftFriend(friendUd->hallLogicData, m_msgHandler->getContext().userData);
		if (!isFind)
		{
			addFriendNotify.set_dst_username(cud->connId);
			addFriendNotify.set_dst_nickname(cud->nickname);
			addFriendNotify.set_dst_portrait_id(cud->portrait_id);
			
			const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(addFriendNotify, "add red gift source friend notify client");
			if (msgBuffer != NULL) m_msgHandler->sendMsgToProxy(msgBuffer, addFriendNotify.ByteSize(), LOGINSRV_ADD_FRIEND_NOTIFY, friendName);
		}
	}
	else
	{
		if (!isFind)
		{
			// 离线用户则需要获取其静态信息
			com_protocol::GetMultiUserInfoReq getFriendStaticDataReq;
			getFriendStaticDataReq.add_username_lst(friendName);
			const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(getFriendStaticDataReq, "get red gift offline friend static data");
			if (msgBuffer != NULL)
			{
				m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgBuffer, getFriendStaticDataReq.ByteSize(), CommonDBSrvProtocol::BUSINESS_GET_MULTI_USER_INFO_REQ,
				m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, 0, EServiceReplyProtocolId::CUSTOM_GET_RED_GIFT_FRIEND_STATIC_DATA);
			}
		}
		
		// 获取离线玩家的逻辑数据
		m_msgHandler->getHallLogic().getUserHallLogicData(friendName, 0, EServiceReplyProtocolId::CUSTOM_GET_HALL_DATA_FOR_RED_GIFT_FRIEND, m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
	}
}

void CLogicHandlerTwo::addRedGiftFriendStaticDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_msgHandler->getHallLogic().addFriendNotifyClient(data, len);
}

void CLogicHandlerTwo::addRedGiftFriendLogicDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetGameDataRsp getHallLogicDataRsp;
	if (!m_msgHandler->parseMsgFromBuffer(getHallLogicDataRsp, data, len, "add offline user red gift friend")) return;

	if(getHallLogicDataRsp.result() != Opt_Success)
	{
		OptErrorLog("add offline user red gift friend error, result = %d", getHallLogicDataRsp.result());
		return;
	}

	com_protocol::HallLogicData hallLogicData;
	if (!m_msgHandler->parseMsgFromBuffer(hallLogicData, getHallLogicDataRsp.data().c_str(), getHallLogicDataRsp.data().length(), "get hall data to add red gift friend")) return;

    addRedGiftFriend(&hallLogicData, getHallLogicDataRsp.tc_data().c_str());
	
	// 向Game DB刷新用户数据
	const char* msgData = m_msgHandler->serializeMsgToBuffer(hallLogicData, "add red gift friend to logic data");
	if (msgData != NULL)
	{
		com_protocol::SetGameDataReq saveLogicData;
		saveLogicData.set_primary_key(HallLogicDataKey);
		saveLogicData.set_second_key(getHallLogicDataRsp.second_key());
		saveLogicData.set_data(msgData, hallLogicData.ByteSize());
		
		msgData = m_msgHandler->serializeMsgToBuffer(saveLogicData, "set hall red gift friend data to db");
		if (msgData != NULL) m_msgHandler->sendMessageToGameDbProxy(msgData, saveLogicData.ByteSize(), SET_GAME_DATA_REQ, "", 0);
	}
}


// 背包相关
void CLogicHandlerTwo::getKnapsackInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get knapsack info")) return;
	
	com_protocol::GetUserBaseinfoReq req;
	ConnectUserData* ud = m_msgHandler->getConnectUserData();
	req.set_query_username(ud->connId, ud->connIdLen);
	req.set_data_type(ELogicDataType::BUYU_COMMON_DATA);
	
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(req, "get knapsack info request");
	if (msgBuffer != NULL)
	{
		m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgBuffer, req.ByteSize(), CommonDBSrvProtocol::BUSINESS_GET_USER_BASEINFO_REQ,
		ud->connId, ud->connIdLen, 0, EServiceReplyProtocolId::CUSTOM_GET_KNAPSACK_INFO_FROM_COMMON_DB);
	}
}

void CLogicHandlerTwo::getKnapsackInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = m_msgHandler->getConnectProxy(m_msgHandler->getContext().userData);
	if (conn == NULL) return;  // 用户下线了

	com_protocol::GetUserBaseinfoRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "get knapsack info reply")) return;

	com_protocol::ClientGetKnapsackInfoRsp clientRsp;
	clientRsp.set_result(rsp.result());
	if (clientRsp.result() == Opt_Success)
	{		
		// 基本信息
		const com_protocol::DynamicInfo& dynamicInfo = rsp.detail_info().dynamic_info();
		clientRsp.set_nickname(rsp.detail_info().static_info().nickname());
		clientRsp.set_portrait_id(rsp.detail_info().static_info().portrait_id());
		clientRsp.set_vip_level(dynamicInfo.vip_level());
		clientRsp.set_buyu_level(rsp.mutable_buyu_data()->mutable_level_info()->cur_level());
		clientRsp.set_score(dynamicInfo.score());
		clientRsp.set_rmb_gold(dynamicInfo.rmb_gold());
		clientRsp.set_game_gold(dynamicInfo.game_gold());
		
		const com_protocol::BatteryEquipment& btyEquip = rsp.commondb_data().battery_equipment();     // 炮台装备
		const google::protobuf::RepeatedPtrField<com_protocol::PropItem> hadItems = rsp.detail_info().prop_items();                // 道具列表
		const int show_zero_value = HallConfigData::config::getConfigValue().knapsack_cfg.show_zero_value;  // 当背包中物品数量为0的时候是否需要展示，值1则展示，其他值不展示
		const vector<HallConfigData::KnapsackItem> items = HallConfigData::config::getConfigValue().knapsack_cfg.knapsack_goods;   // 配置需要展示的道具及其顺序
		for (vector<HallConfigData::KnapsackItem>::const_iterator itemIt = items.begin(); itemIt != items.end(); ++itemIt)
		{
			if (itemIt->show == 1)
			{
				// 道具
				bool isFindItem = false;
				for (int idx = 0; idx < hadItems.size(); ++idx)
				{
					if (hadItems.Get(idx).type() == itemIt->type)
					{
						if (show_zero_value == 1 || hadItems.Get(idx).count() > 0)
						{
							com_protocol::PropItem* addItem = clientRsp.add_goods_list()->mutable_item();
							*addItem = hadItems.Get(idx);
						}
						isFindItem = true;
						break;
					}
				}
				if (isFindItem) continue;
	
				switch (itemIt->type)
				{
					case EPropType::PropVoucher:  // 奖券
					{
						if (show_zero_value == 1 || dynamicInfo.voucher_number() > 0)
						{
							com_protocol::PropItem* addItem = clientRsp.add_goods_list()->mutable_item();
							addItem->set_type(EPropType::PropVoucher);
							addItem->set_count(dynamicInfo.voucher_number());
						}
						isFindItem = true;
						break;
					}
					
					case EPropType::PropDiamonds:  // 钻石
					{
						if (show_zero_value == 1 || dynamicInfo.diamonds_number() > 0)
						{
							com_protocol::PropItem* addItem = clientRsp.add_goods_list()->mutable_item();
							addItem->set_type(EPropType::PropDiamonds);
							addItem->set_count(dynamicInfo.diamonds_number());
						}
						isFindItem = true;
						break;
					}
					
					case EPropType::PropPhoneFareValue:  // 话费额度
					{
						if (show_zero_value == 1 || dynamicInfo.phone_fare() > 0)
						{
							com_protocol::PropItem* addItem = clientRsp.add_goods_list()->mutable_item();
							addItem->set_type(EPropType::PropPhoneFareValue);
							addItem->set_count(dynamicInfo.phone_fare());
						}
						isFindItem = true;
						break;
					}
					
					case EPropType::PropScores:  // 积分券
					{
						if (show_zero_value == 1 || dynamicInfo.score() > 0)
						{
							com_protocol::PropItem* addItem = clientRsp.add_goods_list()->mutable_item();
							addItem->set_type(EPropType::PropScores);
							addItem->set_count(dynamicInfo.score());
						}
						isFindItem = true;
						break;
					}
					
					case EPropType::EPKDayGoldTicket:        // PK场金币对战门票ID类型
					{
						isFindItem = true;

						const com_protocol::PKTicket& pkGoldTicket = rsp.commondb_data().pk_ticket();     // 金币门票
						if (pkGoldTicket.gold_ticket_size() < 1) break;
						
						typedef vector<const com_protocol::Ticket*> GoldTicketVector;
						GoldTicketVector goldTicketSortInfoVector;
				        for (int idx = 0; idx < pkGoldTicket.gold_ticket_size(); ++idx)
						{
							if (pkGoldTicket.gold_ticket(idx).count() > 0) goldTicketSortInfoVector.push_back(&pkGoldTicket.gold_ticket(idx));
						}
				        std::sort(goldTicketSortInfoVector.begin(), goldTicketSortInfoVector.end(), pkGoldTicketTimeSort);  // 先按照门票的期限时间排序
				
						struct tm tmval;
						for (GoldTicketVector::const_iterator gtIt = goldTicketSortInfoVector.begin(); gtIt != goldTicketSortInfoVector.end(); ++gtIt)
						{
							const com_protocol::Ticket* const ticket = *gtIt;
							com_protocol::ClientKnapsackGoods* knapsackGoods = clientRsp.add_goods_list();
							com_protocol::PropItem* addItem = knapsackGoods->mutable_item();
							addItem->set_type(EPropType::EPKDayGoldTicket);  // 默认全天时间段门票
							addItem->set_count(ticket->count());
							
							time_t end_time = ticket->end_time();
							localtime_r(&end_time, &tmval);
							com_protocol::ClientPKGoldTicket* clientPKGoldTicket = knapsackGoods->mutable_pk_gold_ticket();
							clientPKGoldTicket->set_id(ticket->id());
							clientPKGoldTicket->set_year(tmval.tm_year + 1900);
							clientPKGoldTicket->set_month(tmval.tm_mon + 1);
							clientPKGoldTicket->set_day(tmval.tm_mday);
							clientPKGoldTicket->set_hour(24);
							if (ticket->has_begin_hour())
							{
								addItem->set_type(EPropType::EPKHourGoldTicket);  // 限时时间段门票
							    clientPKGoldTicket->set_hour(ticket->begin_hour());
							    clientPKGoldTicket->set_end_hour(ticket->end_hour());
							}
						}
						
						break;
					}
					
					default:
					{
						break;
					}
				}
				if (isFindItem) continue;
				
				// 炮台装备
				for (int idx = 0; idx < btyEquip.ids_size(); ++idx)
				{
					if (btyEquip.ids(idx) == itemIt->type)
					{
						com_protocol::PropItem* addItem = clientRsp.add_goods_list()->mutable_item();
						addItem->set_type(itemIt->type);
						addItem->set_count(1);
						break;
					}
				}
			}
		}
	}

	const char* msgData = m_msgHandler->serializeMsgToBuffer(clientRsp, "send user knapsack info to client");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, clientRsp.ByteSize(), LOGINSRV_GET_KNAPSACK_INFO_RSP, conn);
}

// 踢玩家下线，强制玩家下线
void CLogicHandlerTwo::forceUserQuitNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string username;
	ConnectProxy* conn = m_msgHandler->getConnectInfo("force user quit notify", username);
	if (conn == NULL) return;
	
	com_protocol::PlayerQuitNotifyReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "force user quit notify")) return;

	com_protocol::ClientUserQuitNotify quitNotify;
	quitNotify.set_info(req.info());
	quitNotify.set_code(1);

	const char* msgData = m_msgHandler->serializeMsgToBuffer(quitNotify, "force user quit notify");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, quitNotify.ByteSize(), LOGINSRV_PLAYER_QUIT_NOTIFY, conn);
	
	m_msgHandler->removeConnectProxy(username.c_str());  // 直接断开连接
}

// 绑定兑换商城玩家账号信息
void CLogicHandlerTwo::bindExchangeMallInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to bind exchange mall info")) return;
	
	m_msgHandler->sendMessageToCommonDbProxy(data, len, CommonDBSrvProtocol::BUSINESS_BINDING_EXCHANGE_MALL_INFO_REQ);
}

void CLogicHandlerTwo::bindExchangeMallInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string username;
	ConnectProxy* conn = m_msgHandler->getConnectInfo("bind exchange mall info reply", username);
	if (conn == NULL) return;
	
	m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_BINDING_EXCHANGE_MALL_INFO_RSP, conn);
}

// 获取比赛场信息
void CLogicHandlerTwo::getArenaInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get arena info")) return;

    ConnectUserData* ud = m_msgHandler->getConnectUserData();
	com_protocol::HallData hallData;
	if (m_msgHandler->getPokerCarkServiceFromRedis(ud, hallData))
	{
		// 获取捕鱼卡牌比赛场服务信息
		m_msgHandler->sendMessage(NULL, 0, ud->connId, ud->connIdLen, hallData.rooms(0).id(), CommonProtocol::COMMON_GET_POLER_CARK_SERVICE_INFO_REQ);
	}
	else
	{
		sendMatchArenaInfo();
	}
}

// 获取捕鱼卡牌服务信息
void CLogicHandlerTwo::getBuyuPokerCarkServiceInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string username;
	ConnectProxy* conn = m_msgHandler->getConnectInfo("get buyu poker cark service info reply", username);
	if (conn == NULL) return;
	
	com_protocol::BuyuPokerCarkInfo srvInfo;
	com_protocol::BuyuPokerCarkInfo* pSrvInfo = &srvInfo;
	if (!m_msgHandler->parseMsgFromBuffer(srvInfo, data, len, "get buyu poker cark service info reply")) pSrvInfo = NULL;
	
	sendMatchArenaInfo(conn, pSrvInfo);
}

// 发送比赛场信息
void CLogicHandlerTwo::sendMatchArenaInfo(ConnectProxy* conn, const com_protocol::BuyuPokerCarkInfo* pokerCarkInfo)
{
	char curDate[16] = {0};
	getCurrentDate(curDate, sizeof(curDate), "%d-%02d-%02d");
			
	string startTime;
	string finishTime;
	com_protocol::ClientGetArenaInfoRsp rsp;
	const map<int, HallConfigData::Arena>& arena_cfg_map = HallConfigData::config::getConfigValue().arena_cfg_map;
	for (map<int, HallConfigData::Arena>::const_iterator it = arena_cfg_map.begin(); it != arena_cfg_map.end(); ++it)
	{
		map<string, HallConfigData::ArenaMatchDate>::const_iterator dateIt = it->second.match_date_map.find(curDate);
		if (dateIt != it->second.match_date_map.end() && dateIt->second.match_time_map.size() > 0)
		{
			com_protocol::ClientArenaInfo* arenaInfo = rsp.add_arena_info();
			arenaInfo->set_arena_id(it->first);
			arenaInfo->set_arena_name(it->second.name);

            startTime = "24:60";
			finishTime = "00:00";
			for (map<int, HallConfigData::ArenaMatchTime>::const_iterator timeIt = dateIt->second.match_time_map.begin(); timeIt != dateIt->second.match_time_map.end(); ++timeIt)
			{
				if (timeIt->second.start < startTime) startTime = timeIt->second.start;
				if (timeIt->second.finish > finishTime) finishTime = timeIt->second.finish;
			}
			com_protocol::ClientArenaMatchTime* matchTime = arenaInfo->add_match_times();
			matchTime->set_start(startTime);
			matchTime->set_finish(finishTime);
		}
	}
	
	// 捕鱼卡牌比赛场服务信息
	if (pokerCarkInfo != NULL) *rsp.mutable_buyu_chess_info() = *pokerCarkInfo;
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "get arena info reply");
	if (msgData != NULL)
	{
		if (conn != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_GET_ARENA_INFO_RSP, conn);
		else m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_GET_ARENA_INFO_RSP);
	}
}

void CLogicHandlerTwo::getDetailedMatchInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get detailed match info")) return;
	
	com_protocol::ClientGetDetailedMatchInfoReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "get detailed match info request")) return;
	
	// 找到对应的比赛
	map<int, HallConfigData::Arena>::const_iterator it = HallConfigData::config::getConfigValue().arena_cfg_map.find(req.arena_id());
	if (it == HallConfigData::config::getConfigValue().arena_cfg_map.end())
	{
		OptErrorLog("the arena id error, user = %s, id = %d", m_msgHandler->getConnectUserData()->connId, req.arena_id());
		return;
	}
	
	// 该比赛对应的日期
	char curTimeStr[32] = {0};
	getCurrentDate(curTimeStr, sizeof(curTimeStr), "%d-%02d-%02d");
	map<string, HallConfigData::ArenaMatchDate>::const_iterator dateIt = it->second.match_date_map.find(curTimeStr);
	if (dateIt == it->second.match_date_map.end())
	{
		OptErrorLog("the arena date error, user = %s, id = %d, current date = %s", m_msgHandler->getConnectUserData()->connId, req.arena_id(), curTimeStr);
		return;
	}
	
	// 检查是否存在已经结束了的比赛
	bool hasFinishMatch = false;
	const time_t curSecs = time(NULL);
	struct tm matchTime;
	for (map<int, HallConfigData::ArenaMatchTime>::const_iterator timeIt = dateIt->second.match_time_map.begin(); timeIt != dateIt->second.match_time_map.end(); ++timeIt)
	{
		snprintf(curTimeStr, sizeof(curTimeStr) - 1, "%s %s:00", dateIt->first.c_str(), timeIt->second.finish.c_str());
		if (strptime(curTimeStr, "%Y-%m-%d %H:%M:%S", &matchTime) != NULL && curSecs > mktime(&matchTime))
		{
			hasFinishMatch = true;
			break;
		}
	}
	
	ConnectUserData* cud = m_msgHandler->getConnectUserData();
	if (hasFinishMatch)
	{
		// 先获取排名列表信息
		com_protocol::GetArenaRankingInfoReq getRankingInfoReq;
		getRankingInfoReq.set_arena_id(req.arena_id());
		const char* msgData = m_msgHandler->serializeMsgToBuffer(getRankingInfoReq, "get arena ranking request");
		if (msgData != NULL) m_msgHandler->sendMessageToService(ServiceType::MessageSrv, msgData, getRankingInfoReq.ByteSize(), BUSINESS_GET_ARENA_RANKING_REQ, cud->connId, cud->connIdLen, req.arena_id());
		
		return;
	}
	
	// 获取玩家信息
	com_protocol::GetManyUserOtherInfoReq getUserInfoReq;
	getUserInfoReq.add_query_username(cud->connId);
	getUserInfoReq.add_info_flag(EUserInfoFlag::EVipLevel);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(getUserInfoReq, "get user info for arena request");
	if (msgData != NULL)
	{
		m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgData, getUserInfoReq.ByteSize(), BUSINESS_GET_MANY_USER_OTHER_INFO_REQ,
		cud->connId, cud->connIdLen, req.arena_id(), CUSTOM_GET_USERS_INFO_FOR_ARENA_RANKING);
	}
}

void CLogicHandlerTwo::getArenaRankingInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string username;
	ConnectProxy* conn = m_msgHandler->getConnectInfo("get arena ranking info reply", username);
	if (conn == NULL) return;
	
	com_protocol::GetArenaRankingInfoRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "get arena ranking info reply")) return;
	
	com_protocol::GetManyUserOtherInfoReq getUserInfoReq;
	getUserInfoReq.add_info_flag(EUserInfoFlag::EUserNickname);
	getUserInfoReq.add_info_flag(EUserInfoFlag::EVipLevel);
	getUserInfoReq.add_query_username(username);
	
	if (rsp.has_arena_ranking())
	{
		getUserInfoReq.set_call_back_data(data, len);
		
		unordered_map<string, int> usernameList;
		const google::protobuf::RepeatedPtrField<com_protocol::ArenaMatchRankingList>& arenaRanking = rsp.arena_ranking().ranking();
		for (google::protobuf::RepeatedPtrField<com_protocol::ArenaMatchRankingList>::const_iterator arenaIt = arenaRanking.begin(); arenaIt != arenaRanking.end(); ++arenaIt)
		{
			const google::protobuf::RepeatedPtrField<com_protocol::ArenaMatchRanking>& matchRanking = arenaIt->ranking();
			for (google::protobuf::RepeatedPtrField<com_protocol::ArenaMatchRanking>::const_iterator matchIt = matchRanking.begin(); matchIt != matchRanking.end(); ++matchIt)
			{
				usernameList[matchIt->username()] = 1;
			}
		}
		
		// 排名用户的ID，需要向db common那边根据ID查找用户昵称
		for (unordered_map<string, int>::const_iterator userIt = usernameList.begin(); userIt != usernameList.end(); ++userIt) getUserInfoReq.add_query_username(userIt->first);
	}
	
	// 获取排名玩家的信息
	const char* msgData = m_msgHandler->serializeMsgToBuffer(getUserInfoReq, "get user list info for arena request");
	if (msgData != NULL)
	{
		m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgData, getUserInfoReq.ByteSize(), BUSINESS_GET_MANY_USER_OTHER_INFO_REQ,
		username.c_str(), username.length(), m_msgHandler->getContext().userFlag, CUSTOM_GET_USERS_INFO_FOR_ARENA_RANKING);
	}
}

void CLogicHandlerTwo::getUsersInfoForArenaRanking(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string username;
	ConnectProxy* conn = m_msgHandler->getConnectInfo("get arena ranking info reply", username);
	if (conn == NULL) return;
	
	ConnectUserData* cud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	com_protocol::HallData hallData;
	if (!m_msgHandler->getArenaServiceFromRedis(cud, hallData))
	{
		OptErrorLog("get arena service info error, user = %s", username.c_str());
		return;
	}

	com_protocol::GetManyUserOtherInfoRsp userInfoRsp;
	if (!m_msgHandler->parseMsgFromBuffer(userInfoRsp, data, len, "get arena ranking user info reply") || userInfoRsp.query_user_size() < 1) return;
	
	com_protocol::GetArenaRankingInfoRsp rankingInfoRsp;
	if (userInfoRsp.has_call_back_data()
	    && !m_msgHandler->parseMsgFromBuffer(rankingInfoRsp, userInfoRsp.call_back_data().c_str(), userInfoRsp.call_back_data().length(), "get arena ranking info")) return;
	
	// 找到对应的比赛
	map<int, HallConfigData::Arena>::const_iterator arenaIt = HallConfigData::config::getConfigValue().arena_cfg_map.find(m_msgHandler->getContext().userFlag);
	if (arenaIt == HallConfigData::config::getConfigValue().arena_cfg_map.end())
	{
		OptErrorLog("get arena data error, user = %s, id = %d", username.c_str(), m_msgHandler->getContext().userFlag);
		return;
	}
	
	// 该比赛对应的日期
	char curTimeStr[32] = {0};
	getCurrentDate(curTimeStr, sizeof(curTimeStr), "%d-%02d-%02d");
	map<string, HallConfigData::ArenaMatchDate>::const_iterator dateIt = arenaIt->second.match_date_map.find(curTimeStr);
	if (dateIt == arenaIt->second.match_date_map.end())
	{
		OptErrorLog("get arena date error, user = %s, id = %d, current date = %s", username.c_str(), m_msgHandler->getContext().userFlag, curTimeStr);
		return;
	}
	
	// 比赛的基本信息
	com_protocol::ClientGetDetailedMatchInfoRsp rsp;
	rsp.set_arena_id(m_msgHandler->getContext().userFlag);
	rsp.set_arena_name(arenaIt->second.name);
	rsp.set_arena_rule(HallConfigData::config::getConfigValue().arena_base_cfg.rule);
	rsp.set_is_vip_user(0);
	
	// 检查是否是VIP用户
	if (userInfoRsp.query_user(0).result() == 0)
	{
		const google::protobuf::RepeatedPtrField<com_protocol::UserOtherInfo>& otherInfo = userInfoRsp.query_user(0).other_info();
		for (google::protobuf::RepeatedPtrField<com_protocol::UserOtherInfo>::const_iterator infoIt = otherInfo.begin(); infoIt != otherInfo.end(); ++infoIt)
		{
			if (infoIt->info_flag() == EUserInfoFlag::EVipLevel)
			{
				if (infoIt->has_int_value() && infoIt->int_value() > 0) rsp.set_is_vip_user(1);
				break;
			}
		}
	}
	
	// 排名奖励
	for (vector<HallConfigData::ArenaRankingReward>::const_iterator rewardIt = arenaIt->second.ranking_rewards.begin(); rewardIt != arenaIt->second.ranking_rewards.end(); ++rewardIt)
	{
		com_protocol::ClientArenaRankingReward* reward = rsp.add_ranking_reward();
		for (map<unsigned int, unsigned int>::const_iterator itemIt = rewardIt->rewards.begin(); itemIt != rewardIt->rewards.end(); ++itemIt)
		{
			com_protocol::GiftInfo* gift = reward->add_gifts();
			gift->set_type(itemIt->first);
			gift->set_num(itemIt->second);
		}
	}
	
	// 通知各个比赛服务器更新配置信息（比较合理的方式应该是配置变更的时候即时通知即可）
	const int currentDay = getCurrentYearDay();
	if (currentDay != m_updateArenaConfigDataCurrentDay)
	{
		m_updateArenaConfigDataCurrentDay = currentDay;
		m_isNeedUpdateArenaConfigData.clear();
	}
	
	const vector<unsigned int> arenaServiceIds = HallConfigData::config::getConfigValue().arena_base_cfg.arena_service_ids;
	com_protocol::ArenaMatchDetailedInfoNotify arenaMatchDetailedInfoNotifyMsg;
	const bool isUpdateArenaConfigData = (m_isNeedUpdateArenaConfigData.find(arenaIt->first) == m_isNeedUpdateArenaConfigData.end() || m_isNeedUpdateArenaConfigData[arenaIt->first] == 0) && !arenaServiceIds.empty();
	if (isUpdateArenaConfigData)
	{
		arenaMatchDetailedInfoNotifyMsg.set_arena_id(rsp.arena_id());
		arenaMatchDetailedInfoNotifyMsg.set_arena_name(rsp.arena_name());
		google::protobuf::RepeatedPtrField<com_protocol::ClientArenaRankingReward>* arenaRankingReward = arenaMatchDetailedInfoNotifyMsg.mutable_ranking_reward();
		*arenaRankingReward = rsp.ranking_reward();
	}
	
	// 今天各个时间段的比赛
	const time_t curSecs = time(NULL);
	struct tm matchTime;
	time_t beginSecs = 0;
	time_t endSecs = 0;
	for (map<int, HallConfigData::ArenaMatchTime>::const_iterator timeIt = dateIt->second.match_time_map.begin(); timeIt != dateIt->second.match_time_map.end(); ++timeIt)
	{
		snprintf(curTimeStr, sizeof(curTimeStr) - 1, "%s %s:00", dateIt->first.c_str(), timeIt->second.start.c_str());
		if (strptime(curTimeStr, "%Y-%m-%d %H:%M:%S", &matchTime) != NULL) beginSecs = mktime(&matchTime);
		
		snprintf(curTimeStr, sizeof(curTimeStr) - 1, "%s %s:00", dateIt->first.c_str(), timeIt->second.finish.c_str());
		if (strptime(curTimeStr, "%Y-%m-%d %H:%M:%S", &matchTime) != NULL) endSecs = mktime(&matchTime);
		
		com_protocol::ClientDetailedMatchInfo* matchInfo = NULL;
		com_protocol::ClientDetailedMatchInfo* notifyMatchInfoToServer = NULL;
		if (curSecs >= endSecs)         // 已经结束了的比赛
		{
			matchInfo = rsp.add_finish_match();
			addArenaRankingInfo(userInfoRsp, rankingInfoRsp, timeIt->first, matchInfo);
		}
		else if (curSecs >= beginSecs)  // 比赛进行中
		{
			matchInfo = rsp.add_on_match();
			if (isUpdateArenaConfigData) notifyMatchInfoToServer = arenaMatchDetailedInfoNotifyMsg.add_match_info();
		}
		else                            // 还未开赛
		{
			matchInfo = rsp.add_wait_match();
			if (isUpdateArenaConfigData) notifyMatchInfoToServer = arenaMatchDetailedInfoNotifyMsg.add_match_info();
		}
		
		matchInfo->set_id(timeIt->first);
		matchInfo->set_start(timeIt->second.start);
		matchInfo->set_finish(timeIt->second.finish);
		matchInfo->set_vip(timeIt->second.vip);
		matchInfo->set_gold(timeIt->second.gold);
		matchInfo->set_bullet_rate(timeIt->second.max_bullet_rate);
		matchInfo->set_match_payment_gold(timeIt->second.match_payment_gold);
		
		if (notifyMatchInfoToServer != NULL) *notifyMatchInfoToServer = *matchInfo;
	}
	
	// 先通知各个比赛服务器更新配置信息（比较合理的方式应该是配置变更的时候即时通知即可）
	const char* msgData = NULL;
	if (isUpdateArenaConfigData && (msgData = m_msgHandler->serializeMsgToBuffer(arenaMatchDetailedInfoNotifyMsg, "arena match detailed config notify")) != NULL)
	{
		m_isNeedUpdateArenaConfigData[arenaIt->first] = 1;		
		for (vector<unsigned int>::const_iterator srvIt = arenaServiceIds.begin(); srvIt != arenaServiceIds.end(); ++srvIt)
		{
			if (m_msgHandler->sendMessage(msgData, arenaMatchDetailedInfoNotifyMsg.ByteSize(), *srvIt, COMMON_ARENA_MATCH_CONFIG_INFO_NOTIFY) != Opt_Success) m_isNeedUpdateArenaConfigData[arenaIt->first] = 0;
		}
	}
	
	// 比赛场服务信息
	com_protocol::ClientArenaServiceInfo* serviceInfo = rsp.mutable_service();
	serviceInfo->set_ip(hallData.rooms(0).ip());
	serviceInfo->set_port(hallData.rooms(0).port());
	serviceInfo->set_service_id(hallData.rooms(0).id());
	
	msgData = m_msgHandler->serializeMsgToBuffer(rsp, "get arena detailed match info reply");
	if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, rsp.ByteSize(), LOGINSRV_GET_DETAILED_MATCH_INFO_RSP, conn);
}

void CLogicHandlerTwo::addArenaRankingInfo(const com_protocol::GetManyUserOtherInfoRsp& userInfoRsp, const com_protocol::GetArenaRankingInfoRsp& rankingInfoRsp, const int time_id, com_protocol::ClientDetailedMatchInfo* matchInfo)
{
	const google::protobuf::RepeatedPtrField<com_protocol::ArenaMatchRankingList>& arenaRanking = rankingInfoRsp.arena_ranking().ranking();
	for (google::protobuf::RepeatedPtrField<com_protocol::ArenaMatchRankingList>::const_iterator rankingIt = arenaRanking.begin(); rankingIt != arenaRanking.end(); ++rankingIt)
	{
		if (rankingIt->times_id() == time_id)
		{
			const google::protobuf::RepeatedPtrField<com_protocol::ArenaMatchRanking>& matchRanking = rankingIt->ranking();
			for (google::protobuf::RepeatedPtrField<com_protocol::ArenaMatchRanking>::const_iterator matchIt = matchRanking.begin(); matchIt != matchRanking.end(); ++matchIt)
			{
				com_protocol::ClientArenaRanking* clRanking = matchInfo->add_ranking();
				bool isFind = false;
				const google::protobuf::RepeatedPtrField<com_protocol::UserOtherInfoEx>& userInfo = userInfoRsp.query_user();
				for (google::protobuf::RepeatedPtrField<com_protocol::UserOtherInfoEx>::const_iterator userInfoIt = userInfo.begin(); userInfoIt != userInfo.end(); ++userInfoIt)
			    {
					if (userInfoIt->result() == 0 && userInfoIt->query_username() == matchIt->username())
					{
						const google::protobuf::RepeatedPtrField<com_protocol::UserOtherInfo>& otherInfo = userInfoIt->other_info();
						for (google::protobuf::RepeatedPtrField<com_protocol::UserOtherInfo>::const_iterator infoIt = otherInfo.begin(); infoIt != otherInfo.end(); ++infoIt)
						{
							if (infoIt->info_flag() == EUserInfoFlag::EUserNickname)
							{
								if (infoIt->has_string_value())
								{
									isFind = true;
									clRanking->set_nick_name(infoIt->string_value());
								}
								break;
							}
						}
						
						break;
					}
				}
				if (!isFind) clRanking->set_nick_name(matchIt->username());
			}
			
			break;
		}
	}
}

void CLogicHandlerTwo::updateArenaMatchConfigData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_isNeedUpdateArenaConfigData.clear();
}

void CLogicHandlerTwo::getArenaRankingRewardReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetArenaRankingRewardReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "get arena ranking reward request")) return;
	
	// 找到对应的比赛
	map<int, HallConfigData::Arena>::const_iterator arenaIt = HallConfigData::config::getConfigValue().arena_cfg_map.find(req.arena_id());
	if (arenaIt == HallConfigData::config::getConfigValue().arena_cfg_map.end())
	{
		OptErrorLog("get arena ranking reward error, id = %d", req.arena_id());
		return;
	}
	
	// 排名奖励
	com_protocol::GetArenaRankingRewardRsp rsp;
	rsp.set_arena_id(req.arena_id());
	rsp.set_match_id(req.match_id());
	for (vector<HallConfigData::ArenaRankingReward>::const_iterator rewardIt = arenaIt->second.ranking_rewards.begin(); rewardIt != arenaIt->second.ranking_rewards.end(); ++rewardIt)
	{
		com_protocol::ClientArenaRankingReward* reward = rsp.add_ranking_reward();
		for (map<unsigned int, unsigned int>::const_iterator itemIt = rewardIt->rewards.begin(); itemIt != rewardIt->rewards.end(); ++itemIt)
		{
			com_protocol::GiftInfo* gift = reward->add_gifts();
			gift->set_type(itemIt->first);
			gift->set_num(itemIt->second);
		}
	}

	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "get arena ranking reward reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, rsp.ByteSize(), srcSrvId, COMMON_GET_ARENA_RANKING_REWARD_RSP);
}


// 获取邮件信息
void CLogicHandlerTwo::getMailMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get mail message")) return;
	
	m_msgHandler->sendMessageToCommonDbProxy(data, len, CommonDBSrvProtocol::BUSINESS_GET_MAIL_MESSAGE_REQ);
}

void CLogicHandlerTwo::getMailMessageReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string username;
	ConnectProxy* conn = m_msgHandler->getConnectInfo("get mail message reply", username);
	if (conn == NULL) return;
	
	m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_GET_MAIL_MESSAGE_RSP, conn);
}

// 操作邮件信息
void CLogicHandlerTwo::optMailMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to operate mail message")) return;
	
	m_msgHandler->sendMessageToCommonDbProxy(data, len, CommonDBSrvProtocol::BUSINESS_OPT_MAIL_MESSAGE_REQ);
}

void CLogicHandlerTwo::optMailMessageReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string username;
	ConnectProxy* conn = m_msgHandler->getConnectInfo("operate mail message reply", username);
	if (conn == NULL) return;
	
	m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_OPT_MAIL_MESSAGE_RSP, conn);
}


}
