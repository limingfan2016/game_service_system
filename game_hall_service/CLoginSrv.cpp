
/* author : limingfan
 * date : 2015.03.09
 * description : Login 服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "CLoginSrv.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"
#include "_HallConfigData_.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace NDBOpt;
using namespace NProject;

namespace NPlatformService
{

// 数据记录日志
#define WriteDataLog(format, args...)     getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)

// 统计数据日志
#define StatisticsDataLog(format, args...)     getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)


// 客户端版本标识
enum ClientVersionFlag
{
	normal = 0,               // 正常的当前版本
	check = 1,                // 审核版本
};

// 连接状态
enum ConnectStatus
{
	buildSuccess = 0,
	registerSuccess = 1,
	verifySuccess = 2,
	loginSuccess = 3,
	connectClosed = 4,
};

// 游戏房间排序函数
bool sortGameRoom(const string& a, const string& b)
{
	com_protocol::RoomData roomDataA;
	com_protocol::RoomData roomDataB;

	auto aID = roomDataA.ParseFromArray(a.c_str(), a.length()) ? roomDataA.id() : ((const RedisRoomData*)a.data())->id;
	auto bID = roomDataB.ParseFromArray(b.c_str(), b.length()) ? roomDataB.id() : ((const RedisRoomData*)b.data())->id;
	return aID < bID;
}


CSrvMsgHandler::CSrvMsgHandler() : m_memForUserData(MaxUserDataCount, MaxUserDataCount, sizeof(ConnectUserData)), m_onlyForTestAsyncData(this, 1024, 8)
{
	m_msgBuffer[0] = '\0';
	m_hallLogicDataBuff[0] = '\0';
	memset(&m_loginSrvData, 0, sizeof(m_loginSrvData));
	
	m_getHallDataLastTimeSecs = 0;
	m_getHallDataIntervalSecs = 1;
	m_gameIntervalSecs = 1;
	
	m_index = 0;
}

CSrvMsgHandler::~CSrvMsgHandler()
{
	m_msgBuffer[0] = '\0';
	m_hallLogicDataBuff[0] = '\0';
	memset(&m_loginSrvData, 0, sizeof(m_loginSrvData));
	m_memForUserData.clear();
	
	m_getHallDataLastTimeSecs = 0;
	m_getHallDataIntervalSecs = 1;
	m_gameIntervalSecs = 1;
	
	m_index = 0;
}

bool CSrvMsgHandler::checkLoginIsSuccess(const char* logInfo)
{
	// 未经过登录成功后的非法操作消息则直接关闭连接
	ConnectUserData* ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
	if (ud == NULL)
	{
		closeConnectProxy(getConnectProxyContext().conn);
		OptWarnLog("%s, close connect, id = %d, ip = %s, port = %d",
		logInfo, getConnectProxyContext().conn->proxyId, CSocket::toIPStr(getConnectProxyContext().conn->peerIp), getConnectProxyContext().conn->peerPort);
		return false;
	}
	else if (ud->status != loginSuccess)
	{
		OptWarnLog("%s, close connect, user name = %s, id = %d, ip = %s, port = %d",
		logInfo, ud->connId, getConnectProxyContext().conn->proxyId, CSocket::toIPStr(getConnectProxyContext().conn->peerIp), getConnectProxyContext().conn->peerPort);
		
		removeConnectProxy(string(ud->connId));
		return false;
	}
	
	return true;
}

bool CSrvMsgHandler::serializeMsgToBuffer(const ::google::protobuf::Message& msg, char* buffer, const unsigned int len, const char* info)
{
	if (!msg.SerializeToArray(buffer, len))
	{
		OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), len, info);
		return false;
	}
	return true;
}

const char* CSrvMsgHandler::serializeMsgToBuffer(const ::google::protobuf::Message& msg, const char* info)
{
	if (!msg.SerializeToArray(m_msgBuffer, MaxMsgLen))
	{
		OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), MaxMsgLen, info);
		return NULL;
	}
	return m_msgBuffer;
}

bool CSrvMsgHandler::parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info)
{
	if (!msg.ParseFromArray(buffer, len))
	{
		OptErrorLog("ParseFromArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), len, info);
		return false;
	}
	return true;
}

unsigned int CSrvMsgHandler::getCommonDbProxySrvId(const char* userName, const int len)
{
	// 要根据负载均衡选择commonDBproxy服务
	unsigned int srvId = 0;
	getDestServiceID(DBProxyCommonSrv, userName, len, srvId);
	return srvId;
}

int CSrvMsgHandler::sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userName, int uNLen)
{
	ConnectUserData* ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
	if (userName == NULL)
	{
	    userName = ud->connId;
	    uNLen = ud->connIdLen;
	}
	return sendMessage(data, len, ud->connId, ud->connIdLen, getCommonDbProxySrvId(userName, uNLen), protocolId);
}

int CSrvMsgHandler::sendMessageToGameDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userName, const int uNLen,
                                             unsigned short handleProtocolId, int userFlag)
{
	unsigned int srvId = 0;
	getDestServiceID(GameSrvDBProxy, userName, uNLen, srvId);
	return sendMessage(data, len, userFlag, userName, uNLen, srvId, protocolId, 0, handleProtocolId);
}

int CSrvMsgHandler::sendMessageToService(NFrame::ServiceType srvType, const char* data, const unsigned int len, unsigned short protocolId, const char* userName, const int uNLen,
                                         int userFlag, unsigned short handleProtocolId)
{
	unsigned int srvId = 0;
	getDestServiceID(srvType, userName, uNLen, srvId);
	return sendMessage(data, len, userFlag, userName, uNLen, srvId, protocolId, 0, handleProtocolId);
}

// from client request msg
void CSrvMsgHandler::registerUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (getProxyUserData(getConnectProxyContext().conn) == NULL) initConnect(getConnectProxyContext().conn);  // 先初始化连接

	com_protocol::RegisterUserReq regReq;
	if (!parseMsgFromBuffer(regReq, data, len, "user register")) return;
	
	regReq.set_client_ip(CSocket::toIPStr(getConnectProxyContext().conn->peerIp));
	const char* msgData = serializeMsgToBuffer(regReq, "user register");
	if (msgData != NULL)
	{
		sendMessageToCommonDbProxy(msgData, regReq.ByteSize(), BUSINESS_REGISTER_USER_REQ, regReq.username().c_str(), regReq.username().length());
		
		OptInfoLog("receive message to register, user name = %s, ip = %s", regReq.username().c_str(), regReq.client_ip().c_str());
	}
}

void CSrvMsgHandler::getUserNameByIMEI(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (getProxyUserData(getConnectProxyContext().conn) == NULL) initConnect(getConnectProxyContext().conn);  // 先初始化连接
	
	com_protocol::GetUsernameByIMEIReq getUserNameReq;
	if (!parseMsgFromBuffer(getUserNameReq, data, len, "get user name by IMEI")) return;
	
	getUserNameReq.set_client_ip(CSocket::toIPStr(getConnectProxyContext().conn->peerIp));
	const char* msgData = serializeMsgToBuffer(getUserNameReq, "get user name by IMEI");
	if (msgData != NULL)
	{
		sendMessageToCommonDbProxy(msgData, getUserNameReq.ByteSize(), BUSINESS_GET_USERNAME_BY_IMEI_REQ, getUserNameReq.imei().c_str(), getUserNameReq.imei().length());
		
		OptInfoLog("receive message to get user name, imei = %s, os type = %d, ip = %s", getUserNameReq.imei().c_str(), getUserNameReq.os_type(), getUserNameReq.client_ip().c_str());
	}
}

void CSrvMsgHandler::handleGetPkPlayRankingInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("receive not login successs message to get pk play ranking info req")) return;

	ConnectUserData* ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
	sendMessageToService(ServiceType::MessageSrv, data, len, MessageSrvProtocol::BUSINESS_VIEW_PK_PLAY_RANKING_LIST_REQ, ud->connId, ud->connIdLen);
}

void CSrvMsgHandler::handleGetPkPlayRankingInfoRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in viewGoldRankingReply can not find the connect data", userName);
	if (conn == NULL) return;

	com_protocol::ClientGetPkPlayRankingInfoRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "handle Get Pk Play Ranking Info Rsp")) return;

	com_protocol::HallLogicData* hallLogicData = getHallLogicData(userName.c_str());
	if (hallLogicData == NULL)
	{
		OptErrorLog("CSrvMsgHandler handleGetPkPlayRankingInfoRsp, hallLogicData == NULL, user name:%s", userName.c_str());
		return;
	}

	//更新用户排行榜数据
	updateUserPkPlayRankingData(hallLogicData, rsp.update_num());

	auto rankingInfo = hallLogicData->ranking_info();
	rsp.set_last_pk_score(rankingInfo.last_pk_score());					//上周PK分数
	rsp.set_last_ranking(rankingInfo.last_ranking());					//上周排名 -1.未上榜  0~n.表示排名
	rsp.set_receive_reward(rankingInfo.receive_reward());				//0.表示可以领奖、1表示已领奖 2.不可以领取

	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();

	if (rankingInfo.last_ranking() != NoneRanking)
	{
		auto rewardItemIt = cfg.pk_play_ranking.find(rankingInfo.last_ranking());
		if (rewardItemIt != cfg.pk_play_ranking.end())
		{
			rsp.set_already_reward_item_id(rewardItemIt->second.reward.begin()->first);			//已奖励ID
			rsp.set_already_reward_item_num(rewardItemIt->second.reward.begin()->second);		//已奖励数量
		}
	}

	auto myRanking = rsp.mutable_user_ranking();
	auto itemIt = cfg.pk_play_ranking.find(myRanking->ranking());
	if (itemIt != cfg.pk_play_ranking.end())
	{
		myRanking->set_reward_item_id(itemIt->second.reward.begin()->first);
		myRanking->set_reward_item_num(itemIt->second.reward.begin()->second);
	}

	for (int i = 0; i < rsp.ranking_size(); ++i)
	{
		auto rankingInfo = rsp.ranking(i);
		auto itemIt = cfg.pk_play_ranking.find(rankingInfo.ranking());
		if (itemIt != cfg.pk_play_ranking.end())
		{
			rsp.mutable_ranking(i)->set_reward_item_id(itemIt->second.reward.begin()->first);
			rsp.mutable_ranking(i)->set_reward_item_num(itemIt->second.reward.begin()->second);
		}
	}	
	
	if (serializeMsgToBuffer(rsp, m_msgBuffer, MaxMsgLen, "handle get pk play ranking info rsp"))
		sendMsgToProxy(m_msgBuffer, rsp.ByteSize(), LoginSrvProtocol::LOGINSRV_GET_PK_PLAY_RANKING_INFO_RSP, conn);
	else
		OptErrorLog("CSrvMsgHandler handleGetPkPlayRankingInfoRsp, serialize msg to buffer");
}

void CSrvMsgHandler::handleGetPkPlayRankingRuleReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("receive not login successs message to get pk play ranking rule req")) return;

	ConnectUserData* ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
	sendMessageToService(ServiceType::MessageSrv, data, len, MessageSrvProtocol::BUSINESS_GET_PK_PLAY_RANKING_RULE_REQ, ud->connId, ud->connIdLen);
}

void CSrvMsgHandler::handleGetPkPlayRankingRuleRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in handleGetPkPlayRankingRuleRsp can not find the connect data", userName);
	if (conn == NULL) return;

	sendMsgToProxy(data, len, LoginSrvProtocol::LOGINSRV_GET_PK_PLAY_RANKING_RULE_RSP, conn);
}

void CSrvMsgHandler::handleGetPkPlayCelebrityReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("receive not login successs message to get pk play Celebrity req")) return;

	ConnectUserData* ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
	sendMessageToService(ServiceType::MessageSrv, data, len, MessageSrvProtocol::BUSINESS_GET_PK_PLAY_CELEBRITY_INFO_REQ, ud->connId, ud->connIdLen);
}

void CSrvMsgHandler::handleGetPkPlayCelebrityRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in handleGetPkPlayCelebrityRsp can not find the connect data", userName);
	if (conn == NULL) return;

	com_protocol::ClientPkPlayCelebrityInfoRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "handle Get Pk Play Ranking Celebrity Rsp")) return;

	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	auto rewardItemIt = cfg.pk_play_ranking.find(0);
	if (rewardItemIt != cfg.pk_play_ranking.end())
	{
		for (int i = 0; i < rsp.celebritys_size(); ++i)
		{
			rsp.mutable_celebritys(i)->set_reward_item_id(rewardItemIt->second.reward.begin()->first);
			rsp.mutable_celebritys(i)->set_reward_item_num(rewardItemIt->second.reward.begin()->second);
		}
	}
		
	if (serializeMsgToBuffer(rsp, m_msgBuffer, MaxMsgLen, "handle get pk play ranking Celebrity rsp"))
		sendMsgToProxy(m_msgBuffer, rsp.ByteSize(), LoginSrvProtocol::LOGINSRV_GET_PK_PLAY_CELEBRITY_INFO_RSP, conn);
}

void CSrvMsgHandler::handleGetPkPlayRewarReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("receive not login successs message to handle get pk play rewar req")) return;

	ConnectUserData* ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
	sendMessageToService(ServiceType::MessageSrv, data, len, MessageSrvProtocol::BUSINESS_GET_PK_PLAY_RANKING_REWAR_REQ, ud->connId, ud->connIdLen);
}

void CSrvMsgHandler::handleGetPkPlayRewarRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in handleGetPkPlayCelebrityRsp can not find the connect data", userName);
	if (conn == NULL) return;

	com_protocol::PkPlayRankingRewardRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "handle get pk play rewar rsp")) return;

	com_protocol::HallLogicData* hallLogicData = getHallLogicData(userName.c_str());
	if (hallLogicData == NULL)
	{
		OptErrorLog("CSrvMsgHandler handleGetPkPlayRewarRsp, hallLogicData == NULL, user name:%s", userName.c_str());
		return;
	}

	updateUserPkPlayRankingData(hallLogicData, rsp.clear_num());

	com_protocol::ClientPkPlayRankingRewardRsp rewardRsp;
	char explain[255] = {0};	

	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	rewardRsp.set_result(ProtocolReturnCode::PKPlayRankingListNoneReward);
	if (hallLogicData->ranking_info().receive_reward() == EPKPlayRankingRewardStatus::EAlreadyAward 
	 || hallLogicData->ranking_info().receive_reward() == EPKPlayRankingRewardStatus::ECanAward)
	{
		auto rewardItemIt = cfg.pk_play_ranking.find(hallLogicData->ranking_info().last_ranking());
		if (rewardItemIt != cfg.pk_play_ranking.end())
		{
			rewardRsp.set_reward_item_id(rewardItemIt->second.reward.begin()->first);
			rewardRsp.set_reward_item_num(rewardItemIt->second.reward.begin()->second);
			rewardRsp.set_last_ranking(hallLogicData->ranking_info().last_ranking());

			//领取奖励
			if (hallLogicData->ranking_info().receive_reward() == EPKPlayRankingRewardStatus::ECanAward)
			{
				//向DB更新数据
				NProject::RecordItemVector recordItemVector;
				NProject::RecordItem addItem(rewardItemIt->second.reward.begin()->first, rewardItemIt->second.reward.begin()->second);
				recordItemVector.push_back(addItem);

				snprintf(explain, 254, "%s%d%s%ld", "PK排行榜奖励-", hallLogicData->ranking_info().last_ranking() + 1, "-", rsp.clear_num());
				notifyDbProxyChangeProp(userName.c_str(), (unsigned int)userName.length(), recordItemVector, EPropFlag::PKPlayRankingListReward, explain, NULL);
				rewardRsp.set_result(Opt_Success);
			}
		}
	}
	if (serializeMsgToBuffer(rewardRsp, m_msgBuffer, MaxMsgLen, "handle get pk play rewar rsp"))
		sendMsgToProxy(m_msgBuffer, rewardRsp.ByteSize(), LOGINSRV_GET_PK_PLAY_RANKING_REWAR_RSP, conn);
}

void CSrvMsgHandler::handleUpdateOnLineUserPKRewarReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::PKPlayRankingGiveOutRewardReq req;
	if (!parseMsgFromBuffer(req, data, len, "handle Update On Line User PK Rewar Req")) return;

	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	auto rewardItemIt = cfg.pk_play_ranking.find(req.user_info(0).ranking());
	// const string userName(getContext().userData, getContext().userDataLen);

	//生成PK排行榜奖励记录
	if (rewardItemIt != cfg.pk_play_ranking.end() && !rewardItemIt->second.reward.empty())
	{
		// 仅仅是生成排名记录，不会发放奖品
		char remark[256] = { 0 };
		snprintf(remark, sizeof(remark) - 1, "PK排行榜奖励-%d-%ld", req.user_info(0).ranking() + 1, req.clear_num());
		NProject::RecordItemVector recordItemVector;
		NProject::RecordItem addItem(rewardItemIt->second.reward.begin()->first, 0);
		recordItemVector.push_back(addItem);
		notifyDbProxyChangeProp(getContext().userData, getContext().userDataLen, recordItemVector, EPropFlag::PKRankingListRecord, remark, NULL);
		
		// 获奖信息发放到邮箱
		snprintf(remark, sizeof(remark) - 1, "恭喜您在PK场比赛中获得第%d名！", req.user_info(0).ranking() + 1);
		postMessageToMail(getContext().userData, getContext().userDataLen, "PK排行榜奖励", remark, rewardItemIt->second.reward);
	}

	com_protocol::HallLogicData* hallLogicData = getHallLogicData(getContext().userData);
	if(hallLogicData == NULL)
	{
		//向GAME DB更新用户的奖励	
		com_protocol::GetGameDataReq getHallLogicDataReq;
		getHallLogicDataReq.set_primary_key(HallLogicDataKey);
		getHallLogicDataReq.set_second_key(req.user_info(0).username());
		getHallLogicDataReq.set_tc_data(data, len);

		const char* msgData = serializeMsgToBuffer(getHallLogicDataReq, "get hall logic data request");
		if (msgData != NULL)
			sendMessageToGameDbProxy(msgData, getHallLogicDataReq.ByteSize(), GET_GAME_DATA_REQ, req.user_info(0).username().c_str(), req.user_info(0).username().size(), CUSTOM_GET_HALL_DATA_FOR_PK_RANKING_REWARD);
		return ;
	}	
	
	auto rankingInfo = hallLogicData->mutable_ranking_info();
	rankingInfo->set_last_pk_score(req.user_info(0).score());											//
	rankingInfo->set_last_ranking(req.user_info(0).ranking());									//
	rankingInfo->set_receive_reward((rewardItemIt != cfg.pk_play_ranking.end() ? EPKPlayRankingRewardStatus::ECanAward : EPKPlayRankingRewardStatus::ENotAward));		//
	rankingInfo->set_score_max(0);
	rankingInfo->set_last_effective_time(req.clear_num());
	OptInfoLog("CSrvMsgHandler handleUpdateOnLineUserPKRewarReq, user name:%s", getContext().userData);
}

void CSrvMsgHandler::getHallLogicDataForReplyPKRankingReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetGameDataRsp getHallLogicDataRsp;
	if (!parseMsgFromBuffer(getHallLogicDataRsp, data, len, "get hall logic data for reply pk ranking reward")) return;

	if(getHallLogicDataRsp.result() != 0)
	{
		OptErrorLog("CSrvMsgHandler getHallLogicDataForReplyPKRankingReward, result:%d", getHallLogicDataRsp.result());
		return;
	}

	com_protocol::HallLogicData hallLogicData;
	if (!parseMsgFromBuffer(hallLogicData, getHallLogicDataRsp.data().c_str(), getHallLogicDataRsp.data().length(), "get hall user logic data"))
	{
		OptErrorLog("CSrvMsgHandler getHallLogicDataForReplyPKRankingReward, parse msg from buffer");
		return;
	}


	com_protocol::PKPlayRankingGiveOutRewardReq req;
	if (!parseMsgFromBuffer(req, getHallLogicDataRsp.tc_data().c_str(), getHallLogicDataRsp.tc_data().length(), "handle Update On Line User PK Rewar Req")) return;

	//
	OptInfoLog("2 .update last ranking info, off line user name:%s", req.user_info(0).username().c_str());
	updateLastRankingInfo(hallLogicData, req.user_info(0).ranking(), req.user_info(0).score(), req.clear_num());
	
	//向Game DB刷新用户数据
	com_protocol::SetGameDataReq saveLogicData;
	saveLogicData.set_primary_key(HallLogicDataKey);
	saveLogicData.set_second_key(req.user_info(0).username());
	char hallLogicDataBuff[MaxMsgLen] = { 0 };
	if (serializeMsgToBuffer(hallLogicData, hallLogicDataBuff, MaxMsgLen, "set hall logic data to redis"))
	{
		saveLogicData.set_data(hallLogicDataBuff, hallLogicData.ByteSize());
		if (serializeMsgToBuffer(saveLogicData, m_msgBuffer, MaxMsgLen, "set hall logic data to redis"))
			sendMessageToGameDbProxy(m_msgBuffer, saveLogicData.ByteSize(), SET_GAME_DATA_REQ, "", 0);
	}

}

void CSrvMsgHandler::handleUpdateOffLineUserPKRewarReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::PKPlayRankingGiveOutRewardReq req;
	if (!parseMsgFromBuffer(req, data, len, "handle Update On Line User PK Rewar Req")) return;
	
	com_protocol::GetMoreKeyGameDataReq getHallLogicDataReq;
	getHallLogicDataReq.set_primary_key(HallLogicDataKey, HallLogicDataKeyLen);
	getHallLogicDataReq.set_tc_data(data, len);

	for (int i = 0; i < req.user_info_size(); ++i)
		getHallLogicDataReq.add_second_keys(req.user_info(i).username());
	

	char msgBuffer[MaxMsgLen] = { 0 };
	if (serializeMsgToBuffer(getHallLogicDataReq, msgBuffer, MaxMsgLen, "get more hall data"))
	{
		auto rc = sendMessageToGameDbProxy(msgBuffer, getHallLogicDataReq.ByteSize(), GameServiceDBProtocol::GET_MORE_KEY_GAME_DATA_REQ, "", 0, CUSTOM_GET_MORE_HALL_DATA_FOR_PK_RANKING_REWARD);
		if (OptSuccess != rc)
		{
			OptErrorLog("get no handle fish coin order data error, rc = %d", rc);
			return;
		}

		OptInfoLog("CSrvMsgHandler handleUpdateOffLineUserPKRewarReq, find more hall data....");
	}
}

void CSrvMsgHandler::getOffLineUserHallDataUpPKRewar(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetMoreKeyGameDataRsp getHallLogicDataRsp;
	if (!parseMsgFromBuffer(getHallLogicDataRsp, data, len, "get Un Handle Order Data Reply")) return;

	if (getHallLogicDataRsp.result() != 0)
	{
		OptErrorLog("CSrvMsgHandler getOnLineUserHallDataUpPKRewar, result = %d", getHallLogicDataRsp.result());
		return;
	}

	com_protocol::PKPlayRankingGiveOutRewardReq req;
	if (!parseMsgFromBuffer(req, getHallLogicDataRsp.tc_data().c_str(), getHallLogicDataRsp.tc_data().length(), "parse msg from buffer reward"))
	{
		OptErrorLog("CSrvMsgHandler getOnLineUserHallDataUpPKRewar, parse msg from buffer, PKPlayRankingGiveOutRewardReq");
		return;
	}

	OptInfoLog("get all off line user data.....");
	com_protocol::SetGameDataReq saveLogicData;
	com_protocol::HallLogicData hallLogicData;
	char hallLogicDataBuff[MaxMsgLen] = { 0 };
	for (int i = 0; i < getHallLogicDataRsp.second_key_data_size(); ++i)
	{
		if (!parseMsgFromBuffer(hallLogicData, getHallLogicDataRsp.second_key_data(i).data().c_str(), getHallLogicDataRsp.second_key_data(i).data().length(), "get hall user logic data"))
		{
			OptErrorLog("get user logic data from redis error, user name = %s", getHallLogicDataRsp.second_key_data(i).key().c_str());
			continue;
		}

		for (int n = 0; n < req.user_info_size(); ++n)
		{
			if (req.user_info(n).username() == getHallLogicDataRsp.second_key_data(i).key())
			{
				OptInfoLog("1. update last ranking info, off line user name:%s", req.user_info(n).username().c_str());

				updateLastRankingInfo(hallLogicData, req.user_info(n).ranking(), req.user_info(n).score(), req.clear_num());

				//向Game DB刷新用户数据
				saveLogicData.set_primary_key(HallLogicDataKey);
				saveLogicData.set_second_key(getHallLogicDataRsp.second_key_data(i).key());
				if (serializeMsgToBuffer(hallLogicData, hallLogicDataBuff, MaxMsgLen, "set hall logic data to redis"))
				{
					saveLogicData.set_data(hallLogicDataBuff, hallLogicData.ByteSize());
					if (serializeMsgToBuffer(saveLogicData, m_msgBuffer, MaxMsgLen, "set hall logic data to redis"))
						sendMessageToGameDbProxy(m_msgBuffer, saveLogicData.ByteSize(), SET_GAME_DATA_REQ, "", 0);
				}				
				break;
			}
		}
	}
}

void CSrvMsgHandler::updateLastRankingInfo(com_protocol::HallLogicData &hallLogicData, uint32_t ranking, uint32_t score, uint64_t nRankingClearNum)
{
	auto rankingInfo = hallLogicData.mutable_ranking_info();
	rankingInfo->set_last_pk_score(score);
	rankingInfo->set_last_ranking(ranking);	
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	rankingInfo->set_receive_reward((cfg.pk_play_ranking.find(ranking) != cfg.pk_play_ranking.end()) ? EPKPlayRankingRewardStatus::ECanAward : EPKPlayRankingRewardStatus::ENotAward);
	rankingInfo->set_score_max(0);
	rankingInfo->set_last_effective_time(nRankingClearNum);
}

void CSrvMsgHandler::updateUserPkPlayRankingData(com_protocol::HallLogicData* hallLogicData, uint64_t nRankingClearNum)
{
	if (hallLogicData == NULL)
		return;

	com_protocol::PkPlayRankingInfo* rankingInfo = NULL;

	if (!hallLogicData->has_ranking_info())
	{
		rankingInfo = hallLogicData->mutable_ranking_info();
		rankingInfo->set_last_pk_score(0);											//默认, 分数为0	
		rankingInfo->set_last_ranking(NoneRanking);									//默认, 未上榜
		rankingInfo->set_receive_reward(EPKPlayRankingRewardStatus::ENotAward);		//默认, 不可以领奖
		rankingInfo->set_score_max(0);
		rankingInfo->set_last_effective_time(nRankingClearNum);
		return;
	}

	rankingInfo = hallLogicData->mutable_ranking_info();

	if (rankingInfo->last_effective_time() != nRankingClearNum)
	{
		if (nRankingClearNum - rankingInfo->last_effective_time() == 1)
		{
			rankingInfo->set_last_pk_score(rankingInfo->score_max());
			rankingInfo->set_score_max(0);
		}
		else
		{
			rankingInfo->set_last_pk_score(0);					//默认, 分数为0	
			rankingInfo->set_score_max(0);
		}

		rankingInfo->set_last_ranking(NoneRanking);										//未上榜
		rankingInfo->set_receive_reward(EPKPlayRankingRewardStatus::ENotAward);			//不可以领奖
		rankingInfo->set_last_effective_time(nRankingClearNum);							//更新用户的更新次数
	}
}

void CSrvMsgHandler::login(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 先检查是否存在重复登陆
	ConnectUserData* curCud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
	if (curCud == NULL)
	{
		initConnect(getConnectProxyContext().conn);  // 先初始化连接
	}
	else if (curCud->status == loginSuccess || curCud->loginTimes > 0)
	{
		// 重复登陆的一律关闭连接
		OptErrorLog("user login again, close connect, user name = %s, id = %u, ip = %s, port = %d, status = %u, times = %u, platform type = %u, interval = %u",
		curCud->connId, getConnectProxyContext().conn->proxyId, CSocket::toIPStr(getConnectProxyContext().conn->peerIp), getConnectProxyContext().conn->peerPort,
		curCud->status, curCud->loginTimes, curCud->platformType, time(NULL) - curCud->loginSecs);
		
		if (!removeConnectProxy(curCud->connId)) closeConnectProxy(getConnectProxyContext().conn);  // 作弊行为则强制关闭
		
		return;
	}
	
	com_protocol::ClientLoginReq loginReq;
	if (!parseMsgFromBuffer(loginReq, data, len, "user login")) return;
	
	// 检查版本号是否可用
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	const unsigned int deviceType = loginReq.device();
	const map<int, HallConfigData::client_version>* client_version_list = NULL;
	if (deviceType == ClientVersionType::AndroidSeparate)
		client_version_list = &cfg.android_version_list;
	else if (deviceType == ClientVersionType::AppleMerge)
		client_version_list = &cfg.apple_version_list;
	else if (deviceType == ClientVersionType::AndroidMerge)
		client_version_list = &cfg.android_merge_version;

    bool isCheckVersionOK = false;
	if (client_version_list != NULL)
	{
		const unsigned int platformType = loginReq.platform_type();
		map<int, HallConfigData::client_version>::const_iterator verIt = client_version_list->find(platformType);
		if (verIt != client_version_list->end())
		{
			const HallConfigData::client_version& cVer = verIt->second;
			if ((loginReq.version() >= cVer.valid && loginReq.version() <= cVer.version)  // 正式版本号
				|| (cVer.check_flag == 1 && loginReq.version() == cVer.check_version))    // 校验测试版本号
			{
				isCheckVersionOK = true;
				
				ConnectUserData* ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
				ud->platformType = platformType;
				ud->loginTimes = 1;
				if (loginReq.version() > cVer.version) ud->versionFlag = ClientVersionFlag::check;
				
				// 校验账号&密码
				com_protocol::VerifyPasswordReq checkPaswReq;
				checkPaswReq.set_username(loginReq.username());
				checkPaswReq.set_passwd(loginReq.passwd());
				checkPaswReq.set_login_id(loginReq.login_id());
				checkPaswReq.set_login_ip(CSocket::toIPStr(getConnectProxyContext().conn->peerIp));
				
				int rc = -1;
				const char* msgData = serializeMsgToBuffer(checkPaswReq, "check user password request");
				if (msgData != NULL) rc = sendMessageToCommonDbProxy(msgData, checkPaswReq.ByteSize(), BUSINESS_VERIYFY_PASSWORD_REQ, loginReq.username().c_str(), loginReq.username().length());

				OptInfoLog("receive message to login, ip = %s, port = %d, user name = %s, platform type = %u, version = %s, login id = %s, rc = %d, connId = %s, status = %d, time = %u",
				checkPaswReq.login_ip().c_str(), getConnectProxyContext().conn->peerPort, loginReq.username().c_str(), platformType, loginReq.version().c_str(),
				loginReq.login_id().c_str(), rc, ud->connId, ud->status, ud->loginSecs);
			}
		}
	}
	
	if (!isCheckVersionOK)
	{
		com_protocol::ClientLoginRsp loginRsp;
	    loginRsp.set_result(ClientVersionInvalid);
		if (serializeMsgToBuffer(loginRsp, m_msgBuffer, MaxMsgLen, "user login but client version invalid")) sendMsgToProxy(m_msgBuffer, loginRsp.ByteSize(), LOGINSRV_LOGIN_RSP);
	
		OptWarnLog("receive message to login but client version invalid, ip = %s, user name = %s, device = %d, version = %s",
		CSocket::toIPStr(getConnectProxyContext().conn->peerIp), loginReq.username().c_str(), loginReq.device(), loginReq.version().c_str());
	}
}

void CSrvMsgHandler::logout(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("receive not login successs message to logout")) return;

    ConnectUserData* ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
	removeConnectProxy(string(ud->connId));  // 删除关闭与客户端的连接
}

void CSrvMsgHandler::modifyPassword(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("receive not login successs message to modify password")) return;
	
	com_protocol::ClientModifyPasswordReq modifyPasswordReq;
	if (!parseMsgFromBuffer(modifyPasswordReq, data, len, "user modify password")) return;
	
	com_protocol::ModifyPasswordReq toModifyPwReq;
	toModifyPwReq.set_is_reset(0);
	toModifyPwReq.set_old_passwd(modifyPasswordReq.old_passwd());
	toModifyPwReq.set_new_passwd(modifyPasswordReq.new_passwd());
	if (serializeMsgToBuffer(toModifyPwReq, m_msgBuffer, MaxMsgLen, "user modify password")) sendMessageToCommonDbProxy(m_msgBuffer, toModifyPwReq.ByteSize(), BUSINESS_MODIFY_PASSWORD_REQ);
}

void CSrvMsgHandler::modifyUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("receive not login successs message to modify user informaiton")) return;
	
	sendMessageToCommonDbProxy(data, len, BUSINESS_MODIFY_BASEINFO_REQ);  // 直接消息转发即可
}

void CSrvMsgHandler::getUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("receive not login successs message to get user informaiton")) return;
	
	sendMessageToCommonDbProxy(data, len, BUSINESS_GET_USER_BASEINFO_REQ);  // 直接消息转发即可
}

// 获取游戏大厅数据
void CSrvMsgHandler::getHallInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!checkLoginIsSuccess("receive not login successs message to get game hall informaiton")) return;

	com_protocol::HallData hallData;
	com_protocol::ClientGetHallRsp getHallRsp;
	getBuyuDataFromRedis((ConnectUserData*)getProxyUserData(getConnectProxyContext().conn), hallData);  // 游戏大厅数据
	getHallRsp.set_allocated_hall_info(&hallData);
	getHallRsp.set_result(0);

	if (serializeMsgToBuffer(getHallRsp, m_msgBuffer, MaxMsgLen, "get hall data")) sendMsgToProxy(m_msgBuffer, getHallRsp.ByteSize(), LOGINSRV_GET_HALL_INFO_RSP);
	getHallRsp.release_hall_info();
}

// from common db service reply msg
void CSrvMsgHandler::registerUserReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(string(getContext().userData));
	if (conn == NULL)
	{
		OptErrorLog("in registerUserReply can not find the connect data, user data = %s", getContext().userData);
		return;
	}

	int rc = sendMsgToProxy(data, len, LOGINSRV_REGISTER_USER_RSP, conn);
	
	com_protocol::RegisterUserRsp registerUserRsp;
	parseMsgFromBuffer(registerUserRsp, data, len, "user register reply");
	OptInfoLog("send register result to client, user name = %s, result = %d, send msg rc = %d", registerUserRsp.username().c_str(), registerUserRsp.result(), rc);
}

void CSrvMsgHandler::getUserNameByIMEIReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(string(getContext().userData));
	if (conn == NULL)
	{
		OptErrorLog("in getUserNameByIMEIReply can not find the connect data, user data = %s", getContext().userData);
		return;
	}

	int rc = sendMsgToProxy(data, len, LOGINSRV_GET_USERNAME_BY_IMEI_RSP, conn);
	
	com_protocol::GetUsernameByIMEIRsp getUserNameRsp;
	parseMsgFromBuffer(getUserNameRsp, data, len, "get user name reply");
	const char* userName = getUserNameRsp.has_username() ? getUserNameRsp.username().c_str() : "";
	OptInfoLog("send get user name result to client, user name = %s, result = %d, send msg rc = %d", userName, getUserNameRsp.result(), rc);
}

void CSrvMsgHandler::modifyPasswordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	sendMsgToProxy(data, len, LOGINSRV_MODIFY_PASSWORD_RSP, getContext().userData);
}

void CSrvMsgHandler::modifyUserInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ModifyBaseinfoRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "modify user info reply")) return;

	if (rsp.result() == 0)
	{
		m_redPacketActivity.modifyUserNickname(getContext().userData, rsp);
		
		ConnectProxy* conn = getConnectProxy(getContext().userData);
		if (conn == NULL)
		{
			OptErrorLog("modify user info can not find the connect data, user data = %s", getContext().userData);
			return;
		}

		// 提取用户逻辑数据信息
		ConnectUserData* ud = (ConnectUserData*)getProxyUserData(conn);
		if (ud != NULL && rsp.has_base_info_rsp())
		{
			if (rsp.base_info_rsp().has_nickname()) strncpy(ud->nickname, rsp.base_info_rsp().nickname().c_str(), MaxConnectIdLen - 1);
			if (rsp.base_info_rsp().has_portrait_id()) ud->portrait_id = rsp.base_info_rsp().portrait_id();
		}
	}

	sendMsgToProxy(data, len, LOGINSRV_MODIFY_BASEINFO_RSP, getContext().userData);
}

void CSrvMsgHandler::verifyPasswordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	ConnectProxy* conn = getConnectProxy(string(getContext().userData));
	if (conn == NULL)
	{
		OptErrorLog("in verifyPasswordReply can not find the connect data, user data = %s", getContext().userData);
		return;
	}
	
	com_protocol::VerifyPasswordRsp vpResult;
	if (!parseMsgFromBuffer(vpResult, data, len, "verify password reply")) return;
	if (vpResult.result() != 0)
	{
		com_protocol::ClientLoginRsp failedResult;
		failedResult.set_result(vpResult.result());
		if (serializeMsgToBuffer(failedResult, m_msgBuffer, MaxMsgLen, "verify user password")) sendMsgToProxy(m_msgBuffer, failedResult.ByteSize(), LOGINSRV_LOGIN_RSP, conn);
		return;
	}

	// 检查是否存在重复登录
	SessionKeyData sessionKeyData;
	const int loginSKDLen = m_redisDbOpt.getHField(SessionKey, SessionKeyLen, vpResult.username().c_str(), vpResult.username().length(), (char*)&sessionKeyData, sizeof(sessionKeyData));
	if (loginSKDLen > 0)
	{
		// 关闭和登录服务器的连接
		ConnectProxy* vpConn = NULL;
		if (sessionKeyData.loginSrvId != getSrvId())
		{
			sendMessage(vpResult.username().c_str(), vpResult.username().length(), sessionKeyData.loginSrvId, LOGINSRV_CLOSE_CONNECT_REQ);
		}
		else
		{
			vpConn = getConnectProxy(vpResult.username());
			if (vpConn == conn)  // 同一连接的重复登陆消息直接忽略掉
			{
				OptWarnLog("ignore repeat user login message, user name = %s, id = %d, ip = %s, port = %d",
				vpResult.username().c_str(), conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort);
				
				return;
			}
			
			closeRepeatConnect(vpResult.username().c_str(), vpResult.username().length(), getSrvId(), 0, 0);  // 存在重复的登录连接就直接关闭
		}
		
		// 关闭和游戏的连接
		GameServiceData gameSrvData;
		const int gameLen = m_redisDbOpt.getHField(GameServiceKey, GameServiceLen, vpResult.username().c_str(), vpResult.username().length(), (char*)&gameSrvData, sizeof(gameSrvData));
		if (gameLen > 0)
		{
			sendMessage(NULL, 0, vpResult.username().c_str(), vpResult.username().length(), gameSrvData.id, GAME_SERVICE_LOGOUT_NOTIFY_REQ);
		}
		
		struct in_addr sdkAddrIp;
		sdkAddrIp.s_addr = sessionKeyData.ip;
		OptInfoLog("get session key info, vpUser = %s, skd ip = %s, port = %d, time = %u, id = %u, service id = %u, game skd len = %d, id = %u, userData = %s, conn id = %u, ip = %s, port = %d, conn = %p, vpConn = %p",
		vpResult.username().c_str(), CSocket::toIPStr(sdkAddrIp), sessionKeyData.port, sessionKeyData.timeSecs, sessionKeyData.loginSrvId, getSrvId(), gameLen, gameSrvData.id,
		getContext().userData, conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort, conn, vpConn);
	}
	else
	{
		OptInfoLog("get session key empty, vpUser = %s, skd len = %d, userData = %s, id = %u, ip = %s, port = %d",
		vpResult.username().c_str(), loginSKDLen, getContext().userData, conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort);
	}
	
	// 删除初始化ID的连接
	ConnectUserData* ud = (ConnectUserData*)getProxyUserData(conn);
	removeConnectProxy(string(ud->connId), false);
	
	// 校验成功之后，变更连接ID为用户名
	strncpy(ud->connId, vpResult.username().c_str(), MaxConnectIdLen - 1);
	ud->connIdLen = vpResult.username().length();
	ud->status = verifySuccess;
	ud->score = 0;
	ud->phoneBill = 0;
	ud->propVoucher = 0;
	ud->shareId = 0;
	addConnectProxy(vpResult.username(), conn, ud);
	
	// 获取大厅逻辑数据
	com_protocol::GetGameDataReq hallLogicData;
	hallLogicData.set_primary_key(HallLogicDataKey);
	hallLogicData.set_second_key(vpResult.username().c_str());
	if (serializeMsgToBuffer(hallLogicData, m_msgBuffer, MaxMsgLen, "get hall logic data"))
	{
		sendMessageToGameDbProxy(m_msgBuffer, hallLogicData.ByteSize(), GET_GAME_DATA_REQ, ud->connId, ud->connIdLen);
	}
}

void CSrvMsgHandler::getHallLogicDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetGameDataRsp hallLogicDataRsp;
	if (!parseMsgFromBuffer(hallLogicDataRsp, data, len, "get hall logic data reply")) return;
	
	if (hallLogicDataRsp.result() != 0)
	{
		OptErrorLog("get hall logic data reply error, user data = %s, rc = %d", getContext().userData, hallLogicDataRsp.result());
		return;
	}
	
	const string userName = getContext().userData;
	ConnectProxy* conn = getConnectProxy(userName);
	if (conn == NULL)
	{
		OptErrorLog("in getHallLogicDataReply can not find the connect data, user data = %s", getContext().userData);
		return;
	}

	// 提取用户逻辑数据信息
	ConnectUserData* ud = (ConnectUserData*)getProxyUserData(conn);
	if (ud->hallLogicData != NULL)  // 内存覆盖错误，一律关闭连接
	{
		OptErrorLog("user login and reset hall logic data, close connect, user name = %s, id = %u, ip = %s, port = %d, status = %u, times = %u, platform type = %u, interval = %u",
		ud->connId, conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort, ud->status, ud->loginTimes, ud->platformType, time(NULL) - ud->loginSecs);
		
		if (!removeConnectProxy(ud->connId)) closeConnectProxy(conn);  // 作弊行为则强制关闭
		
		return;
	}

	NEW(ud->hallLogicData, com_protocol::HallLogicData());
	if (hallLogicDataRsp.has_data() && !parseMsgFromBuffer(*(ud->hallLogicData), hallLogicDataRsp.data().c_str(), hallLogicDataRsp.data().length(), "get hall user logic data"))
	{
		OptErrorLog("get user logic data from redis error, user name = %s", getContext().userData);
		return;
	}
	
	// 新增加数据信息
	time_t curSecs = time(NULL);
	const unsigned int theDay = localtime(&curSecs)->tm_yday;
	if (!ud->hallLogicData->has_pk_ticket() || theDay != ud->hallLogicData->pk_ticket().today())
	{
		// 第一次则初始化PK门票信息或者跨天了则重置数据
		ud->hallLogicData->mutable_pk_ticket()->set_today(theDay);
		ud->hallLogicData->mutable_pk_ticket()->set_remind(0);
	}
	
	// 获取用户基本信息数据
	com_protocol::GetUserBaseinfoReq userInfoReq;
	userInfoReq.set_query_username(userName);
	if (ud->hallLogicData->pk_ticket().remind() == 0) userInfoReq.set_data_type(ELogicDataType::DBCOMMON_LOGIC_DATA);

	if (serializeMsgToBuffer(userInfoReq, m_msgBuffer, MaxMsgLen, "get user base info"))
	{
		sendMessage(m_msgBuffer, userInfoReq.ByteSize(), ud->connId, ud->connIdLen, getCommonDbProxySrvId(ud->connId, ud->connIdLen), BUSINESS_GET_USER_BASEINFO_REQ);
	}
}

void CSrvMsgHandler::getUserInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const string userName = getContext().userData;
	ConnectProxy* conn = getConnectProxy(userName);
	if (conn == NULL)
	{
		OptErrorLog("in getUserInfoReply can not find the connect data, user data = %s", getContext().userData);
		return;
	}

	m_idx2UserBaseInfo[++m_index] = com_protocol::GetUserBaseinfoRsp();
	com_protocol::GetUserBaseinfoRsp& userBaseInfoRsp = m_idx2UserBaseInfo[m_index];
	if (!parseMsgFromBuffer(userBaseInfoRsp, data, len, "get user info reply") || userBaseInfoRsp.result() != 0)
	{
		OptErrorLog("in getUserInfoReply parse msg error, result = %d", userBaseInfoRsp.result());
		m_idx2UserBaseInfo.erase(m_index);
	    return;
	}
	
	// 发起查询用户等级信息请求
	com_protocol::GetGameDataReq gameLogicReq;
	gameLogicReq.set_primary_key(NProject::BuyuLogicDataKey);
	gameLogicReq.set_second_key(userBaseInfoRsp.query_username().c_str(), userBaseInfoRsp.query_username().length());
	const char* msgData = serializeMsgToBuffer(gameLogicReq, "get game logic data request");
	if (msgData == NULL
	    || Success != sendMessageToGameDbProxy(msgData, gameLogicReq.ByteSize(), GET_GAME_DATA_REQ, getContext().userData, getContext().userDataLen, LOGINSRV_GET_GAME_LOGIC_DATA_RSP, m_index))
	{
		OptErrorLog("send msg to game db proxy error");
		m_idx2UserBaseInfo.erase(m_index);
	}
}

void CSrvMsgHandler::getUserGameLogicDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	IndexToUserBaseInfo::iterator it = m_idx2UserBaseInfo.find(getContext().userFlag);
	if (it == m_idx2UserBaseInfo.end())
	{
		OptErrorLog("in getUserGameLogicDataReply can not find the user base info, flag = %d", getContext().userFlag);
		return;
	}
	
	com_protocol::GetUserBaseinfoRsp& userBaseInfoRsp = it->second;
	const string userName = getContext().userData;
	ConnectProxy* conn = getConnectProxy(userName);
	ConnectUserData* ud = (ConnectUserData*)getProxyUserData(conn);
	
	bool isOptOk = false;
	do
	{
		if (conn == NULL)
		{
			OptErrorLog("in getUserGameLogicDataReply can not find the connect data, user data = %s", getContext().userData);
			break;
		}
		
		com_protocol::GetGameDataRsp gameLogicDataRsp;
		if (!parseMsgFromBuffer(gameLogicDataRsp, data, len, "get game logic data reply") || gameLogicDataRsp.result() != 0)
		{
			OptErrorLog("get game logic data reply error, user data = %s, rc = %d", getContext().userData, gameLogicDataRsp.result());
			break;
		}
		
		com_protocol::BuyuLogicData game_logic_data;
		if ((gameLogicDataRsp.has_data() && gameLogicDataRsp.data().length() > 0) && !parseMsgFromBuffer(game_logic_data, gameLogicDataRsp.data().c_str(), gameLogicDataRsp.data().length(), "get user game logic data"))
		{
			OptErrorLog("get user game logic data from redis error, user name = %s, query name = %s", getContext().userData, userBaseInfoRsp.query_username().c_str());
			break;
		}
		
		com_protocol::BuyuLevelInfo* gameInfo = userBaseInfoRsp.mutable_detail_info()->mutable_buyu_logic_info();
		gameInfo->set_cur_level(game_logic_data.level_info().cur_level());
		gameInfo->set_cur_level_exp(game_logic_data.level_info().cur_level_exp());
		gameInfo->set_sum_exp(game_logic_data.level_info().sum_exp());
		gameInfo->set_level_need_exp(getBuyuLevelExp(game_logic_data.level_info().cur_level()));

		if (ud->status == loginSuccess)
		{
			// 登录成功之后的查询消息则直接转发回客户端
			const char* msgData = serializeMsgToBuffer(userBaseInfoRsp, "send user base info to client");
			if (msgData != NULL) sendMsgToProxy(msgData, userBaseInfoRsp.ByteSize(), LOGINSRV_GET_USER_INFO_RSP, conn);
			m_idx2UserBaseInfo.erase(it);
			
			return;
		}
		
		isOptOk = true;
	} while (0);
	
	if (!isOptOk)
	{
		m_idx2UserBaseInfo.erase(it);
		return;
	}

	com_protocol::HallData hallData;
	com_protocol::ClientLoginRsp loginRsp;
	loginRsp.set_result(userBaseInfoRsp.result());
	while (loginRsp.result() == 0)
	{
		// 填写session key
		SessionKeyData sessionKeyData;
		sessionKeyData.ip = conn->peerIp.s_addr;
		sessionKeyData.port = conn->peerPort;
		sessionKeyData.timeSecs = time(NULL);
		sessionKeyData.loginSrvId = getSrvId();
		sessionKeyData.userVersion = isNewVersionUser(ud) ? 1 : 0;
		sessionKeyData.reserve = 0;
		
		// 存储到redis服务
		int rc = m_redisDbOpt.setHField(SessionKey, SessionKeyLen, getContext().userData, getContext().userDataLen, (const char*)&sessionKeyData, sizeof(sessionKeyData));
        if (rc != 0)
		{
			OptErrorLog("set session key to redis error, rc = %d, user name = %s", rc, getContext().userData);
			loginRsp.set_result(SetSessionKeyFailed);
			break;
		}
		
		char sessionKeyValue[MaxKeyLen] = {0};
		getSessionKey(sessionKeyData, sessionKeyValue);
		loginRsp.set_session_key(sessionKeyValue);
		loginRsp.set_allocated_detail_info((com_protocol::DetailInfo*)&userBaseInfoRsp.detail_info());
		
		// 游戏大厅数据
		getBuyuDataFromRedis(ud, hallData);
		loginRsp.set_allocated_hall_info(&hallData);
		
		// 是否放开小游戏服务入口，值1为放开，其他值则关闭
		loginRsp.set_open_game_entrance(0);
		const HallConfigData::OtherGameSwitch& otherGameCfg = HallConfigData::config::getConfigValue().other_game_cfg;
		if (otherGameCfg.open_game == 1)
		{
		    unordered_map<unsigned int, unsigned int>::const_iterator flagIt = otherGameCfg.open_platform.find(ud->platformType);
	        if (flagIt != otherGameCfg.open_platform.end() && flagIt->second == 1) loginRsp.set_open_game_entrance(1);  // 打开小游戏入口
		}

		break;
	}
	
	if (serializeMsgToBuffer(loginRsp, m_msgBuffer, MaxMsgLen, "user login"))
	{
		int rc = sendMsgToProxy(m_msgBuffer, loginRsp.ByteSize(), LOGINSRV_LOGIN_RSP, conn);
		if (rc == Success && loginRsp.result() == Success)
		{
			// 记录登录信息
			ud->status = loginSuccess;
			ud->loginSecs = time(NULL);

			const com_protocol::StaticInfo& userStaticInfo = loginRsp.detail_info().static_info();
			strncpy(ud->nickname, userStaticInfo.nickname().c_str(), MaxConnectIdLen - 1);
			ud->portrait_id = userStaticInfo.portrait_id();

			com_protocol::LoginNotifyReq notifyLogin;
			notifyLogin.set_login_timestamp(ud->loginSecs);

            // 兼容处理，老用户的账号第一次登陆新版本服务器时，老用户的奖券转换成积分券，加到积分券数量中去
			com_protocol::DynamicInfo* dynamicInfo = loginRsp.mutable_detail_info()->mutable_dynamic_info();
			if (isNewVersionUser(ud) && dynamicInfo->voucher_number() > 0)
			{
				notifyLogin.set_lottery_to_score(1);

                // 老的奖券换算成积分券，1:2500 的比例
				// 目前，应该奖券是先转换为积分券，然后由于积分券时间过期，会被直接清零，这里把比例调整为 1:1 ，防止出现大额转换
                // const unsigned int allCurrentScore = dynamicInfo->score() + dynamicInfo->voucher_number() * 2500;
				const unsigned int allCurrentScore = dynamicInfo->score() + dynamicInfo->voucher_number() * 1;
                WriteDataLog("Change Lottery To Score|%s|%u|%u|%u", userName.c_str(), dynamicInfo->score(), dynamicInfo->voucher_number(), allCurrentScore);
				
				dynamicInfo->set_score(allCurrentScore);
			}

			const uint32_t sum_recharge = dynamicInfo->sum_recharge();
			const uint32_t vipLv = m_hallLogic.getVIPLv(sum_recharge);
			if (dynamicInfo->vip_level() != vipLv)
			{
				OptErrorLog("check VIP level error, user = %s, sum recharge = %u, new level = %u, current level = %u", userName.c_str(), sum_recharge, vipLv, dynamicInfo->vip_level());
				WriteDataLog("Check VIP Level|%s|%u|%u|%u", userName.c_str(), sum_recharge, vipLv, dynamicInfo->vip_level());
				
				// 存在用户充值成功后，游戏异常退出或者网络异常等情况导致没有更新VIP等级，则用户下次上线的时候重新刷新VIP等级
				getHallLogic().toDBUpdateVIPLv(userName.c_str(), vipLv - dynamicInfo->vip_level(), EPropFlag::VIPUpLv);
				dynamicInfo->set_vip_level(vipLv);
			}
			
			/*
			// 兼容处理，老用户的账号第一次登陆新版本服务器时，老用户VIP等级为0且有累计充值，则设置VIP等级
			if (isNewVersionUser(ud) && dynamicInfo->vip_level() == 0 && sum_recharge > 0)
			{
				dynamicInfo->set_vip_level(vipLv);
				if (vipLv > 0)	ud->hallLogicData->mutable_vip_info()->set_receive(VIPStatus::VIPReceiveReward);
			}
			*/ 

			if (serializeMsgToBuffer(notifyLogin, m_msgBuffer, MaxMsgLen, "notify dbcommon service login"))
			{
				sendMessage(m_msgBuffer, notifyLogin.ByteSize(), userName.c_str(), userName.length(), getCommonDbProxySrvId(userName.c_str(), userName.length()), BUSINESS_LOGIN_NOTIFY_REQ);
			}
			
			// 最后才回调玩家上线接口
			++m_loginSrvData.current_persons;
			onLine(ud, conn, userBaseInfoRsp);
			
			com_protocol::UserEnterServiceNotify notifyMsgSrv;
			notifyMsgSrv.set_service_id(getSrvId());
			if (serializeMsgToBuffer(notifyMsgSrv, m_msgBuffer, MaxMsgLen, "notify message service login"))
			{
				sendMessageToService(ServiceType::MessageSrv, m_msgBuffer, notifyMsgSrv.ByteSize(), BUSINESS_USER_ENTER_SERVICE_NOTIFY, userName.c_str(), userName.length());
			}
		}
		
		OptInfoLog("send login result to client, user name = %s, result = %d, rc = %d, online persons = %d, connect id = %u, flag = %d, ip = %s, port = %d",
		userName.c_str(), loginRsp.result(), rc, m_loginSrvData.current_persons, conn->proxyId, conn->proxyFlag, CSocket::toIPStr(conn->peerIp), conn->peerPort);
	}
	
	if (loginRsp.result() == 0)
	{
		loginRsp.release_detail_info();
		loginRsp.release_hall_info();
	}
	
	m_idx2UserBaseInfo.erase(it);
}

void CSrvMsgHandler::loginReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// now do nothing
}

void CSrvMsgHandler::logoutReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// now do nothing
}

void CSrvMsgHandler::setHallLogicDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// now do nothing
}

// 定时保存数据到redis服务
void CSrvMsgHandler::saveDataToDb(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	static const unsigned int loginSrvId = getSrvId();
	m_loginSrvData.curTimeSecs = time(NULL);
	int rc = m_redisDbOpt.setHField(LoginListKey, LoginListLen, (const char*)&loginSrvId, sizeof(loginSrvId), (const char*)&m_loginSrvData, sizeof(m_loginSrvData));
	if (rc != 0) OptErrorLog("in saveDataToDb set login service data to redis center service failed, rc = %d", rc);
}

// 初始化连接数据
void CSrvMsgHandler::initConnect(ConnectProxy* conn)
{
	// 挂接用户数据
	ConnectUserData* ud = (ConnectUserData*)m_memForUserData.get();
	memset(ud, 0, sizeof(ConnectUserData));
	ud->status = buildSuccess;
	ud->connIdLen = snprintf(ud->connId, MaxConnectIdLen - 1, "^%s:%d~?", CSocket::toIPStr(conn->peerIp), conn->peerPort);
	ud->score = 0;
	ud->phoneBill = 0;
	ud->propVoucher = 0;
	ud->shareId = 0;
	addConnectProxy(string(ud->connId), conn, ud);
}

CRechargeMgr& CSrvMsgHandler::getRechargeInstance()
{
	return m_rechargeMgr;
}

CLogger& CSrvMsgHandler::getDataLogger()
{
	static CLogger dataLogger(CCfg::getValue("DataLogger", "File"), atoi(CCfg::getValue("DataLogger", "Size")), atoi(CCfg::getValue("DataLogger", "BackupCount")));
	return dataLogger;
}

void CSrvMsgHandler::updateConfig()
{
	m_hallLogic.updateConfig();
	m_activityCenter.updateConfig();
	m_pkLogic.updateConfig();
	m_logicHandlerTwo.updateConfig();
}

ConnectProxy* CSrvMsgHandler::getConnectInfo(const char* logInfo, string& userName)
{
	userName = getContext().userData;
	ConnectProxy* conn = getConnectProxy(getContext().userData);
	if (conn == NULL) OptErrorLog("%s, user data = %s", logInfo, getContext().userData);
	return conn;
}

CPkLogic& CSrvMsgHandler::getPkLogic()
{
	return m_pkLogic;
}

// 关闭重复的连接
void CSrvMsgHandler::closeRepeatConnect(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const string userName(data, len);
	ConnectProxy* conn = getConnectProxy(userName);
	if (conn != NULL)
	{
		com_protocol::ClientRepeatLoginNotify repeateLoginNotify;
		repeateLoginNotify.set_username(userName);
		if (serializeMsgToBuffer(repeateLoginNotify, m_msgBuffer, MaxMsgLen, "notify close repeate login")) sendMsgToProxy(m_msgBuffer, repeateLoginNotify.ByteSize(), LOGINSRV_REPEAT_LOGIN_NOTIFY, conn);
		
		OptWarnLog("close repeate user connect, id = %ld, ip = %s, port = %d, user name = %s", conn->proxyId, CSocket::toIPStr(conn->peerIp), conn->peerPort, userName.c_str());
		
		removeConnectProxy(userName);
	}
}

// 游戏用户重复连接退出通知
void CSrvMsgHandler::gameUserLogout(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const string userName(getContext().userData, getContext().userDataLen);
	OptWarnLog("receive game service logout notify, user name = %s", userName.c_str());
}

void CSrvMsgHandler::handlePlayGenerateShareIDReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// const string userName(getContext().userData, getContext().userDataLen);
	if (getContext().userDataLen > 0) getHallLogic().generateShareID(getContext().userData);

	com_protocol::playGenerateShareIDReq req;
	if (!parseMsgFromBuffer(req, data, len, "play generate share id req"))
	{
		OptErrorLog("CSrvMsgHandler handlePlayGenerateShareIDReq");
		return;
	}

	for (auto it = req.username_lst().begin(); it != req.username_lst().end(); ++it)
		getHallLogic().generateShareID((*it).c_str());
}

void CSrvMsgHandler::handlePlayShareReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const string userName(getContext().userData, getContext().userDataLen);

	//获取分享奖励
	com_protocol::ClientHallShareRsp rsp;
	getHallLogic().getShareReward(userName.c_str(), rsp);

	char msgData[MaxMsgLen] = { 0 };
	rsp.SerializeToArray(msgData, MaxMsgLen);
	sendMessage(msgData, rsp.ByteSize(), srcSrvId, LOGINSRV_PLAY_SHARE_RSP);
}

void CSrvMsgHandler::handleGetPKPlayCfgReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::LoginForwardMsgGetPkPlayCfgRsp rsp;
		
	//场次信息
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	for (auto it = cfg.pk_play_info.PkPlayInfoList.begin(); it != cfg.pk_play_info.PkPlayInfoList.end(); ++it)
	{
		auto playCfg = rsp.add_cfg();
		playCfg->set_id(it->play_id);
		playCfg->set_user_lv(it->user_lv);		//入场需要的用户等级
		playCfg->set_ticket(it->ticket);

		//入场所需物品
		for (auto conditionIt = it->condition_list.begin(); conditionIt != it->condition_list.end(); ++conditionIt)
			addItemCfg(playCfg, conditionIt->first, conditionIt->second);
		
		//手续费
		for (auto entranceIt = it->entrance_fee.begin(); entranceIt != it->entrance_fee.end(); ++entranceIt)
			addItemCfg(playCfg, entranceIt->first, entranceIt->second);

		//入场可携带的金币
		addItemCfg(playCfg, EPropType::PropGold, cfg.pk_play_info.gold_max);
	}
	
	char msgData[MaxMsgLen] = { 0 };
	rsp.SerializeToArray(msgData, MaxMsgLen);
	sendMessage(msgData, rsp.ByteSize(), srcSrvId, GET_PK_PLAY_CFG_RSP);
}

void CSrvMsgHandler::addItemCfg(com_protocol::PKPlayCfg* pCfg, int itemID, int itemNum)
{
	for (int i = 0; i < pCfg->require_id_size(); ++i)
	{
		if (pCfg->require_id(i) != itemID)
			continue;

		pCfg->set_require_num(i, pCfg->require_num(i) + itemNum);
		return;
	}

	pCfg->add_require_id(itemID);
	pCfg->add_require_num(itemNum);
}

// 通知逻辑层对应的逻辑连接已被关闭
void CSrvMsgHandler::onClosedConnect(void* userData)
{
	ConnectUserData* ud = (ConnectUserData*)userData;
	int flag = removeConnectProxy(string(ud->connId), false) ? 2 : 1;  // 1:主动关闭连接；2:被动关闭的连接；
	notifyLogout(ud, flag);
	
	DELETE(ud->hallLogicData);
    m_memForUserData.put((char*)userData);
}

CHallLogic& CSrvMsgHandler::getHallLogic()
{
	return m_hallLogic;
}

CActivityCenter& CSrvMsgHandler::getActivityCenter()
{
	return m_activityCenter;
}

CLogicHandlerTwo& CSrvMsgHandler::getLogicHanderTwo()
{
	return m_logicHandlerTwo;
}

// 统计数据
CStatisticsDataManager& CSrvMsgHandler::getStatDataMgr()
{
	return m_statDataMgr;
}

// 通知连接logout操作
void CSrvMsgHandler::notifyLogout(void* cb, int flag)
{
	ConnectUserData* ud = (ConnectUserData*)cb;
	if (ud->status == loginSuccess)
	{
		ud->status = connectClosed;
		if (m_loginSrvData.current_persons > 0) --m_loginSrvData.current_persons;

		int rc = m_redisDbOpt.delHField(SessionKey, SessionKeyLen, ud->connId, ud->connIdLen);  // 删除session key
		if (rc < 0) OptErrorLog("remove session key failed, user name = %s, rc = %d", ud->connId, rc);
		
		offLine(ud);  // 下线

		int result = -1;
		if (serializeMsgToBuffer(*(ud->hallLogicData), m_hallLogicDataBuff, LogicDataMaxLen, "save hall logic data"))
		{
			com_protocol::SetGameDataReq saveLogicData;
			saveLogicData.set_primary_key(HallLogicDataKey);
			saveLogicData.set_second_key(ud->connId);
			saveLogicData.set_data(m_hallLogicDataBuff, ud->hallLogicData->ByteSize());
			if (serializeMsgToBuffer(saveLogicData, m_msgBuffer, MaxMsgLen, "set hall logic data to redis"))
			{
				result = sendMessageToGameDbProxy(m_msgBuffer, saveLogicData.ByteSize(), SET_GAME_DATA_REQ, ud->connId, ud->connIdLen);
			}
		}
		
        // 通知服务用户已经logout
		com_protocol::UserQuitServiceNotify notifyMsgSrv;
		notifyMsgSrv.set_service_id(getSrvId());
		if (serializeMsgToBuffer(notifyMsgSrv, m_msgBuffer, MaxMsgLen, "notify message service logout"))
		{
			sendMessageToService(ServiceType::MessageSrv, m_msgBuffer, notifyMsgSrv.ByteSize(), BUSINESS_USER_QUIT_SERVICE_NOTIFY, ud->connId, ud->connIdLen);
		}
			
		unsigned int logoutTimeSecs = time(NULL);
		com_protocol::LogoutNotifyReq logoutNotify;
		logoutNotify.set_logout_mode(flag);
		logoutNotify.set_online_time(logoutTimeSecs - ud->loginSecs);
		logoutNotify.set_logout_timestamp(logoutTimeSecs);
		if (serializeMsgToBuffer(logoutNotify, m_msgBuffer, MaxMsgLen, "notify logout"))
		{
			sendMessage(m_msgBuffer, logoutNotify.ByteSize(), ud->connId, ud->connIdLen, getCommonDbProxySrvId(ud->connId, ud->connIdLen), BUSINESS_LOGOUT_NOTIFY_REQ);
		}
		
		OptInfoLog("user logout, name = %s, result = %d, rc = %d", ud->connId, result, rc);
	}
}

// 从redis服务获取象棋服务游戏数据
bool CSrvMsgHandler::getXiangQiServiceFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData)
{
	const HallConfigData::GameRoomCfg& gameRoomCfg = HallConfigData::config::getConfigValue().game_room_cfg;
	if (gameRoomCfg.open_chess_entrance != 1) return false;
	
	const HallConfigData::OtherGameSwitch& otherGameCfg = HallConfigData::config::getConfigValue().other_game_cfg;
	unordered_map<unsigned int, unsigned int>::const_iterator flagIt = otherGameCfg.open_chess.find(ud->platformType);
	if (flagIt == otherGameCfg.open_chess.end() || flagIt->second != 1) return false;

	return getServicesFromRedis(ud, hallData, ServiceType::XiangQiSrv, gameRoomCfg.current_chess_flag, gameRoomCfg.new_chess_flag);
}

// 从redis服务获取捕鱼卡牌服务游戏数据
bool CSrvMsgHandler::getPokerCarkServiceFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData)
{
	const HallConfigData::GameRoomCfg& gameRoomCfg = HallConfigData::config::getConfigValue().game_room_cfg;
	if (gameRoomCfg.open_poker_cark_entrance != 1) return false;

	const HallConfigData::OtherGameSwitch& otherGameCfg = HallConfigData::config::getConfigValue().other_game_cfg;
	unordered_map<unsigned int, unsigned int>::const_iterator flagIt = otherGameCfg.open_poker_cark.find(ud->platformType);
	if (flagIt == otherGameCfg.open_poker_cark.end() || flagIt->second != 1) return false;
	
	return getServicesFromRedis(ud, hallData, ServiceType::BuyuGameSrv, gameRoomCfg.current_poker_cark_flag, gameRoomCfg.new_poker_cark_flag);
}

// 从redis服务获取比赛场游戏数据
bool CSrvMsgHandler::getArenaServiceFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData)
{
	const HallConfigData::GameRoomCfg& gameRoomCfg = HallConfigData::config::getConfigValue().game_room_cfg;
	return getServicesFromRedis(ud, hallData, ServiceType::BuyuGameSrv, gameRoomCfg.current_arena_flag, gameRoomCfg.new_arena_flag);
}

// 从redis服务获取PK场游戏数据
bool CSrvMsgHandler::getPKServiceFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData)
{
	const HallConfigData::GameRoomCfg& gameRoomCfg = HallConfigData::config::getConfigValue().game_room_cfg;
	return getServicesFromRedis(ud, hallData, ServiceType::BuyuGameSrv, gameRoomCfg.current_pk_flag, gameRoomCfg.new_pk_flag);
}

bool CSrvMsgHandler::getServicesFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData, const unsigned int srvType, const unsigned int currentFlag, const unsigned int newFlag)
{
	// 刷新游戏数据
	const unsigned int currentSecs = updateServiceData();
	hallData.set_id(0);
	const bool isNewVersionUserValue = isNewVersionUser(ud);  // 审核版本或者是强制更新版本
	com_protocol::RoomData roomData;
	for (unsigned int i = 0; i < m_strHallData.size(); ++i)
	{
		if (parseMsgFromBuffer(roomData, m_strHallData[i].data(), m_strHallData[i].length(), "get game service room data")
		    && (currentSecs - roomData.update_timestamp()) < m_gameIntervalSecs  // 有效时间内
			&& (roomData.id() / ServiceIdBaseValue) == srvType)
		{
			if ((isNewVersionUserValue && roomData.flag() == newFlag)
				|| (!isNewVersionUserValue && roomData.flag() == currentFlag))
			{
				com_protocol::RoomData* proRoom = hallData.add_rooms();
				*proRoom = roomData;
			}
		}
	}

	return hallData.rooms_size() > 0;
}

bool CSrvMsgHandler::getAllGameServerID(const unsigned int srvType, std::vector<int> &vecSrvID)
{
	const unsigned int currentSecs = updateServiceData();
	com_protocol::RoomData roomData;
	for (unsigned int i = 0; i < m_strHallData.size(); ++i)
	{
		if (parseMsgFromBuffer(roomData, m_strHallData[i].data(), m_strHallData[i].length(), "get room data"))
		{
			if ((currentSecs - roomData.update_timestamp()) < m_gameIntervalSecs && (roomData.id() / ServiceIdBaseValue) == srvType)
			{
				vecSrvID.push_back(roomData.id());
			}
		}
	}

	return true;
}

// 从redis服务获取游戏大厅数据
bool CSrvMsgHandler::getBuyuDataFromRedis(const ConnectUserData* ud, com_protocol::HallData& hallData)
{
	const unsigned int currentSecs = updateServiceData();
	hallData.set_id(0);
	const HallConfigData::GameRoomCfg& gameRoomCfg = HallConfigData::config::getConfigValue().game_room_cfg;
	const bool isNewVersionUserValue = isNewVersionUser(ud);  // 审核版本或者是强制更新版本
	com_protocol::RoomData roomData;
	for (unsigned int i = 0; i < m_strHallData.size(); ++i)
	{
		if (parseMsgFromBuffer(roomData, m_strHallData[i].data(), m_strHallData[i].length(), "get room data"))
		{
			if ((currentSecs - roomData.update_timestamp()) < m_gameIntervalSecs && (roomData.id() / ServiceIdBaseValue) == ServiceType::BuyuGameSrv)
			{
				if (isNewVersionUserValue)
				{
					if (roomData.id() < gameRoomCfg.new_service_id) continue;
				}
				else if (roomData.id() < gameRoomCfg.current_service_id)
				{
					continue;
				}

				com_protocol::RoomData* proRoom = hallData.add_rooms();
				*proRoom = roomData;
				
				if (hallData.rooms_size() >= (int)gameRoomCfg.buyu_max_count) break;
			}
			
			// OptInfoLog("get hall info from redis new = %d, room id = %d, new_service_id = %d, condition = %d, condition2 = %d",
			// isNewVersionUserValue, roomData.id(), buyuRoomCfg.new_service_id, (currentSecs - roomData.update_timestamp()) < m_gameIntervalSecs);
		}
		
		/*
		else
		{
			const RedisRoomData* redisRoomData = (const RedisRoomData*)m_strHallData[i].data();  // 只提取value数据
			if ((currentSecs - redisRoomData->update_timestamp) < m_gameIntervalSecs)
			{
				if (isNewVersionUserValue)  // 新版本用户
				{
					if (redisRoomData->id < buyuRoomCfg.new_service_id) continue;  // 新用户，但非新版本服务
				}
				else if (redisRoomData->id < buyuRoomCfg.current_service_id)
				{
					continue;  // 老用户，但非老版本服务
				}

				com_protocol::RoomData* proRoom = hallData.add_rooms();
				proRoom->set_ip(redisRoomData->ip);
				proRoom->set_port(redisRoomData->port);
				proRoom->set_type(redisRoomData->type);
				proRoom->set_rule(redisRoomData->rule);
				proRoom->set_rate(redisRoomData->rate);
				proRoom->set_current_persons(redisRoomData->current_persons);
				proRoom->set_max_persons(redisRoomData->max_persons);
				proRoom->set_id(redisRoomData->id);
				proRoom->set_name(redisRoomData->name);
				proRoom->set_limit_game_gold(redisRoomData->limit_game_gold);
				proRoom->set_limit_up_game_gold(redisRoomData->limit_up_game_gold);
			
				if (hallData.rooms_size() >= (int)buyuRoomCfg.max_count) break;
			}
			
			// OptInfoLog("get hall info from redis new = %d, room id = %d, new_service_id = %d, current service id = %d, condition = %d",
			// isNewVersionUserValue, redisRoomData->id, buyuRoomCfg.new_service_id, buyuRoomCfg.current_service_id, (currentSecs - redisRoomData->update_timestamp) < m_gameIntervalSecs);
		}
	    */
		
		/*
		struct in_addr addrIp;
		addrIp.s_addr = redisRoomData->ip;
		OptInfoLog("call getBuyuDataFromRedis, ip = %s, port = %d, type = %d, rule = %d, cur per = %d, id = %d, name = %s",
		CSocket::toIPStr(addrIp), redisRoomData->port, redisRoomData->type, redisRoomData->rule, redisRoomData->current_persons, redisRoomData->id, redisRoomData->name);
		*/ 
	}

	return m_strHallData.size() > 0;
}

unsigned int CSrvMsgHandler::updateServiceData()
{
	// 刷新游戏数据
	const unsigned int currentSecs = time(NULL);
	if ((currentSecs - m_getHallDataLastTimeSecs) >= m_getHallDataIntervalSecs)
	{
		m_getHallDataLastTimeSecs = currentSecs;
		std::vector<std::string> strHallData;
		int rc = m_redisDbOpt.getHAll(HallKey, HallKeyLen, strHallData);
		if (rc == 0)
		{
			m_strHallData.clear();
			for (unsigned int i = 1; i < strHallData.size(); i += 2) m_strHallData.push_back(strHallData[i]);  // 只提取value数据
			std::sort(m_strHallData.begin(), m_strHallData.end(), sortGameRoom);  // 房间排序
		}
		else
		{
			OptErrorLog("get hall info from redis error, rc = %d", rc);
		}
	}
	
	return currentSecs;
}

bool CSrvMsgHandler::isNewVersionUser(const ConnectUserData* ud)
{
	if (ud->versionFlag == ClientVersionFlag::check) return true;  // 审核版本

	const HallConfigData::GameRoomCfg& gameRoomCfg = HallConfigData::config::getConfigValue().game_room_cfg;
	unordered_map<unsigned int, unsigned int>::const_iterator flagIt = gameRoomCfg.platform_update_flag.find(ud->platformType);
	return (flagIt != gameRoomCfg.platform_update_flag.end() && flagIt->second == 1);  // 强制更新版本
}


void CSrvMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("register protocol handler, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
	
	// 注册协议处理函数
	// 收到的客户端请求消息
	registerProtocol(OutsideClientSrv, LOGINSRV_REGISTER_USER_REQ, (ProtocolHandler)&CSrvMsgHandler::registerUser);
	registerProtocol(OutsideClientSrv, LOGINSRV_GET_USERNAME_BY_IMEI_REQ, (ProtocolHandler)&CSrvMsgHandler::getUserNameByIMEI);
	registerProtocol(OutsideClientSrv, LOGINSRV_MODIFY_PASSWORD_REQ, (ProtocolHandler)&CSrvMsgHandler::modifyPassword);
	registerProtocol(OutsideClientSrv, LOGINSRV_MODIFY_BASEINFO_REQ, (ProtocolHandler)&CSrvMsgHandler::modifyUserInfo);
	registerProtocol(OutsideClientSrv, LOGINSRV_LOGIN_REQ, (ProtocolHandler)&CSrvMsgHandler::login);
	registerProtocol(OutsideClientSrv, LOGINSRV_LOGOUT_REQ, (ProtocolHandler)&CSrvMsgHandler::logout);
	registerProtocol(OutsideClientSrv, LOGINSRV_GET_USER_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::getUserInfo);
	registerProtocol(OutsideClientSrv, LOGINSRV_GET_HALL_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::getHallInfo);

	//PK场排行榜
	registerProtocol(OutsideClientSrv, LOGINSRV_GET_PK_PLAY_RANKING_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::handleGetPkPlayRankingInfoReq);
	registerProtocol(OutsideClientSrv, LOGINSRV_GET_PK_PLAY_RANKING_REWAR_REQ, (ProtocolHandler)&CSrvMsgHandler::handleGetPkPlayRewarReq);
	registerProtocol(OutsideClientSrv, LOGINSRV_GET_PK_PLAY_RANKING_RULE_REQ, (ProtocolHandler)&CSrvMsgHandler::handleGetPkPlayRankingRuleReq);
	registerProtocol(OutsideClientSrv, LOGINSRV_GET_PK_PLAY_CELEBRITY_INFO_REQ, (ProtocolHandler)&CSrvMsgHandler::handleGetPkPlayCelebrityReq);
	

	

	registerProtocol(MessageSrv, MessageSrvProtocol::BUSINESS_VIEW_PK_PLAY_RANKING_LIST_RSP, (ProtocolHandler)&CSrvMsgHandler::handleGetPkPlayRankingInfoRsp, this);
	registerProtocol(MessageSrv, MessageSrvProtocol::BUSINESS_GET_PK_PLAY_RANKING_RULE_RSP, (ProtocolHandler)&CSrvMsgHandler::handleGetPkPlayRankingRuleRsp, this);
	registerProtocol(MessageSrv, MessageSrvProtocol::BUSINESS_GET_PK_PLAY_CELEBRITY_INFO_RSP, (ProtocolHandler)&CSrvMsgHandler::handleGetPkPlayCelebrityRsp, this);
	registerProtocol(MessageSrv, MessageSrvProtocol::BUSINESS_GET_PK_PLAY_RANKING_REWAR_RSP, (ProtocolHandler)&CSrvMsgHandler::handleGetPkPlayRewarRsp, this);
	
	registerProtocol(MessageSrv, BUSINESS_ON_LINE_PK_RANKING_REWARD_REQ, (ProtocolHandler)&CSrvMsgHandler::handleUpdateOnLineUserPKRewarReq, this);
	registerProtocol(MessageSrv, BUSINESS_OFF_LINE_PK_RANKING_REWARD_REQ, (ProtocolHandler)&CSrvMsgHandler::handleUpdateOffLineUserPKRewarReq, this);

	registerProtocol(GameSrvDBProxy, CUSTOM_GET_MORE_HALL_DATA_FOR_PK_RANKING_REWARD, (ProtocolHandler)&CSrvMsgHandler::getOffLineUserHallDataUpPKRewar, this);


		
	// 收到的CommonDB代理服务应答消息
	registerProtocol(DBProxyCommonSrv, BUSINESS_REGISTER_USER_RSP, (ProtocolHandler)&CSrvMsgHandler::registerUserReply);
	registerProtocol(DBProxyCommonSrv, BUSINESS_GET_USERNAME_BY_IMEI_RSP, (ProtocolHandler)&CSrvMsgHandler::getUserNameByIMEIReply);
	registerProtocol(DBProxyCommonSrv, BUSINESS_MODIFY_PASSWORD_RSP, (ProtocolHandler)&CSrvMsgHandler::modifyPasswordReply);
	registerProtocol(DBProxyCommonSrv, BUSINESS_MODIFY_BASEINFO_RSP, (ProtocolHandler)&CSrvMsgHandler::modifyUserInfoReply);
	registerProtocol(DBProxyCommonSrv, BUSINESS_VERIYFY_PASSWORD_RSP, (ProtocolHandler)&CSrvMsgHandler::verifyPasswordReply);
	registerProtocol(DBProxyCommonSrv, BUSINESS_GET_USER_BASEINFO_RSP, (ProtocolHandler)&CSrvMsgHandler::getUserInfoReply);
	registerProtocol(DBProxyCommonSrv, BUSINESS_LOGIN_NOTIFY_RSP, (ProtocolHandler)&CSrvMsgHandler::loginReply);
	registerProtocol(DBProxyCommonSrv, BUSINESS_LOGOUT_NOTIFY_RSP, (ProtocolHandler)&CSrvMsgHandler::logoutReply);
	
	// 收到的GameDB代理服务应答消息
	registerProtocol(GameSrvDBProxy, GameServiceDBProtocol::GET_GAME_DATA_RSP, (ProtocolHandler)&CSrvMsgHandler::getHallLogicDataReply);
	registerProtocol(GameSrvDBProxy, GameServiceDBProtocol::SET_GAME_DATA_RSP, (ProtocolHandler)&CSrvMsgHandler::setHallLogicDataReply);
	
	registerProtocol(GameSrvDBProxy, LOGINSRV_GET_GAME_LOGIC_DATA_RSP, (ProtocolHandler)&CSrvMsgHandler::getUserGameLogicDataReply);

	registerProtocol(GameSrvDBProxy, CUSTOM_GET_HALL_DATA_FOR_PK_RANKING_REWARD, (ProtocolHandler)&CSrvMsgHandler::getHallLogicDataForReplyPKRankingReward);

	
	
	// 内部服务消息
	registerProtocol(LoginSrv, LOGINSRV_CLOSE_CONNECT_REQ, (ProtocolHandler)&CSrvMsgHandler::closeRepeatConnect);
	registerProtocol(CommonSrv, GAME_SERVICE_LOGOUT_NOTIFY_RSP, (ProtocolHandler)&CSrvMsgHandler::gameUserLogout);
	
	registerProtocol(CommonSrv, LOGINSRV_PLAY_GENERATE_SHARE_ID_REQ, (ProtocolHandler)&CSrvMsgHandler::handlePlayGenerateShareIDReq, this);
	registerProtocol(CommonSrv, LOGINSRV_PLAY_SHARE_REQ, (ProtocolHandler)&CSrvMsgHandler::handlePlayShareReq, this);

	//获取PK场配置信息
	registerProtocol(CommonSrv, GET_PK_PLAY_CFG_REQ, (ProtocolHandler)&CSrvMsgHandler::handleGetPKPlayCfgReq, this);


	//监控统计
	m_monitorStat.init(this);
	m_monitorStat.sendOnlineNotify();
}

void CSrvMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!HallConfigData::config::getConfigValue(CCfg::getValue("LoginService", "BusinessXmlConfigFile")).isSetConfigValueSuccess())
	{
		OptErrorLog("set business xml config value error");
		stopService();
		return;
	}
	
	// 清理连接代理，如果服务异常退出，则启动是需要先清理连接代理
	unsigned int proxyId[1024] = {0};
	unsigned int proxyIdLen = 0;
	const NCommon::Key2Value* pServiceIds = m_srvMsgCommCfg->getList("ServiceID");
	while (pServiceIds)
	{
		if (0 != strcmp("MaxServiceId", pServiceIds->key))
		{
			unsigned int service_id = atoi(pServiceIds->value);
			unsigned int service_type = service_id / ServiceIdBaseValue;
			if (service_type == ServiceType::GatewayProxySrv) proxyId[proxyIdLen++] = service_id;
		}
		
		pServiceIds = pServiceIds->pNext;
	}
	
	if (proxyIdLen > 0) cleanUpConnectProxy(proxyId, proxyIdLen);  // 清理连接代理
	
	// 数据日志配置检查
	const unsigned int MinDataLogFileSize = 1024 * 1024 * 2;
    const unsigned int MinDataLogFileCount = 20;
	const char* dataLoggerCfg[] = {"File", "Size", "BackupCount",};
	for (unsigned int i = 0; i < (sizeof(dataLoggerCfg) / sizeof(dataLoggerCfg[0])); ++i)
	{
		const char* value = CCfg::getValue("DataLogger", dataLoggerCfg[i]);
		if (value == NULL)
		{
			OptErrorLog("data log config item error, can not find item = %s", dataLoggerCfg[i]);
			stopService();
			return;
		}
		
		if ((i == 1 && (unsigned int)atoi(value) < MinDataLogFileSize)
			|| (i == 2 && (unsigned int)atoi(value) < MinDataLogFileCount))
		{
			OptErrorLog("data log config item error, file min size = %d, count = %d", MinDataLogFileSize, MinDataLogFileCount);
			stopService();
			return;
		}
	}
	
	const char* centerRedisSrvItem = "RedisCenterService";
	const char* ip = m_srvMsgCommCfg->get(centerRedisSrvItem, "IP");
	const char* port = m_srvMsgCommCfg->get(centerRedisSrvItem, "Port");
	const char* connectTimeOut = m_srvMsgCommCfg->get(centerRedisSrvItem, "Timeout");
	if (ip == NULL || port == NULL || connectTimeOut == NULL)
	{
		OptErrorLog("redis center service config error");
		stopService();
		return;
	}
	
	if (!m_redisDbOpt.connectSvr(ip, atoi(port), atol(connectTimeOut)))
	{
		OptErrorLog("do connect redis center service failed, ip = %s, port = %s, time out = %s", ip, port, connectTimeOut);
		stopService();
		return;
	}
	
	// 把大厅的数据存储到redis
	unsigned int loginSrvId = getSrvId();
	m_loginSrvData.ip = CSocket::toIPInt(CCfg::getValue("NetConnect", "IP"));
	m_loginSrvData.port = atoi(CCfg::getValue("NetConnect", "Port"));
	m_loginSrvData.curTimeSecs = time(NULL);
	int rc = m_redisDbOpt.setHField(LoginListKey, LoginListLen, (const char*)&loginSrvId, sizeof(loginSrvId), (const char*)&m_loginSrvData, sizeof(m_loginSrvData));
	if (rc != 0)
	{
		OptErrorLog("set login service data to redis center service failed, rc = %d", rc);
		stopService();
		return;
	}
	
	// 定时保存数据到redis
	setTimer(atoi(CCfg::getValue("LoginService", "SaveDataToDBInterval")), (TimerHandler)&CSrvMsgHandler::saveDataToDb, 0, NULL, (unsigned int)-1);
	
	// 向游戏大厅获取数据的时间间隔，单位秒
	const char* secsTimeInterval = CCfg::getValue("LoginService", "GetHallDataInterval");
	if (secsTimeInterval != NULL) m_getHallDataIntervalSecs = atoi(secsTimeInterval);
	
	// 游戏服务检测的时间间隔，单位秒
	secsTimeInterval = CCfg::getValue("LoginService", "GameDataInterval");
	if (secsTimeInterval != NULL) m_gameIntervalSecs = atoi(secsTimeInterval);
	else m_gameIntervalSecs = m_getHallDataIntervalSecs * 2;
	
	// 充值管理
	m_rechargeMgr.init(this);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientGameMall, LOGINSRV_GET_GAME_MALL_CONFIG_REQ, LOGINSRV_GET_GAME_MALL_CONFIG_RSP);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientGetRechargeFishCoinOrder, LOGINSRV_GET_RECHARGE_FISH_COIN_ORDER_REQ, LOGINSRV_GET_RECHARGE_FISH_COIN_ORDER_RSP);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientCancelRechargeFishCoinOrder, LOGINSRV_CANCEL_RECHARGE_FISH_COIN_NOTIFY, 0);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientRechargeFishCoinResultNotify, 0, LOGINSRV_RECHARGE_FISH_COIN_NOTIFY);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientExchangeGameGoods, LOGINSRV_EXCHANGE_GAME_GOODS_REQ, LOGINSRV_EXCHANGE_GAME_GOODS_RSP);
	// m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientGetSuperValuePackageInfo, LOGINSRV_GET_SUPER_VALUE_PACKAGE_INFO_REQ, LOGINSRV_GET_SUPER_VALUE_PACKAGE_INFO_RSP);
	// m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientReceiveSuperValuePackage, LOGINSRV_RECEIVE_SUPER_VALUE_PACKAGE_GIFT_REQ, LOGINSRV_RECEIVE_SUPER_VALUE_PACKAGE_GIFT_RSP);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientGetFirstPackageInfo, LOGINSRV_GET_FIRST_PACKAGE_INFO_REQ, LOGINSRV_GET_FIRST_PACKAGE_INFO_RSP);
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientGetExchangeGoodsConfig, LOGINSRV_GET_EXCHANGE_CONFIG_REQ, LOGINSRV_GET_EXCHANGE_CONFIG_RSP);
	//m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientExchangePhoneFare, LOGINSRV_EXCHANGE_PHONE_FARE_REQ, LOGINSRV_EXCHANGE_PHONE_FARE_RSP);
	
	m_rechargeMgr.setClientMessageHandler(EMessageHandlerType::EClientCheckAppleRechargeResult, LOGINSRV_APPLE_RECHARGE_RESULT_NOTIFY, 0);

	m_rechargeMgr.addProtocolHandler(EListenerProtocolFlag::ERechargeResultNotify, (ProtocolHandler)&CSrvMsgHandler::rechargeNotifyResult, this);
	
	m_hallLogic.load(this);
	m_activityCenter.load(this);
	m_pkLogic.load(this);
	m_logicHandlerTwo.load(this);
	m_redPacketActivity.load(this);
	m_catchGiftActivity.load(this);
}

void CSrvMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 服务关闭，所有在线用户退出登陆
	const IDToConnectProxys& userConnects = getConnectProxy();
	for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end(); ++it)
	{
		// 这里不可以调用removeConnectProxy，如此会不断修改userConnects的值而导致本循环遍历it值错误
		notifyLogout(getProxyUserData(it->second), 1);
	}
	
	m_hallLogic.unLoad(this);
	m_activityCenter.unLoad(this);
	m_pkLogic.unLoad(this);
	m_logicHandlerTwo.unLoad();
	m_redPacketActivity.unLoad();
	m_catchGiftActivity.unLoad();

	stopConnectProxy();  // 停止连接代理
	
	m_redisDbOpt.disconnectSvr();
}

void CSrvMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 总量&小时&天 统计数据
	getStatDataMgr().addStatData(EStatisticsDataItem::EAllStatisticsData);
	getStatDataMgr().addStatData(EStatisticsDataItem::EHourStatisticsData);
	getStatDataMgr().addStatData(EStatisticsDataItem::EDayStatisticsData);
	
	// 设置统计时间间隔
	unsigned int interval = HallConfigData::config::getConfigValue().common_cfg.statistics_interval_secs;
	if (interval < 1) interval = 1;
	setTimer(1000 * interval, (TimerHandler)&CSrvMsgHandler::writeStatisticsData, 0, NULL, -1);
	
	
	// only for test，测试异步消息本地数据存储
	testAsyncLocalData();
}

// 是否可以执行该操作
const char* CSrvMsgHandler::canToDoOperation(const int opt, const char* info)
{
	if (opt == EServiceOperationType::EClientRequest && checkLoginIsSuccess(info))
	{
		ConnectUserData* ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
		return ud->connId;
	}
	
	return NULL;
}

void CSrvMsgHandler::onLine(ConnectUserData* ud, ConnectProxy* conn, const com_protocol::GetUserBaseinfoRsp& baseInfo)
{
	m_hallLogic.onLine(ud, conn);
	m_activityCenter.onLine(ud, conn);
	m_pkLogic.onLine(ud, conn);
	m_catchGiftActivity.onLine(ud, conn);
	
	remindPlayerUsePKGoldTicket(ud, baseInfo);  // 快到期门票发消息提醒
}

void CSrvMsgHandler::offLine(ConnectUserData* ud)
{
	m_hallLogic.offLine(ud->hallLogicData);
	m_activityCenter.offLine(ud->hallLogicData);
	m_pkLogic.offLine(ud);
	m_catchGiftActivity.offLine(ud);
}

// 提醒玩家尽快使用快过期的金币对战门票
void CSrvMsgHandler::remindPlayerUsePKGoldTicket(ConnectUserData* ud, const com_protocol::GetUserBaseinfoRsp& baseInfo)
{
	// 快到期门票发消息提醒
	if (ud->hallLogicData->pk_ticket().remind() == 0 && baseInfo.has_commondb_data())
	{
		ud->hallLogicData->mutable_pk_ticket()->set_remind(1);
		
		const com_protocol::PKTicket& pkTicket = baseInfo.commondb_data().pk_ticket();
		if (pkTicket.gold_ticket_size() > 0)
		{
			time_t endTime = pkTicket.gold_ticket(0).end_time();
			for (int idx = 1; idx < pkTicket.gold_ticket_size(); ++idx)
			{
				if (pkTicket.gold_ticket(idx).end_time() < endTime) endTime = pkTicket.gold_ticket(idx).end_time();
			}

			const unsigned int theDay = localtime(&endTime)->tm_yday;
			if (theDay == ud->hallLogicData->pk_ticket().today())
			{
				// 今天到期的门票发消息通知玩家尽快使用，否则会过期
				com_protocol::MessageNotifyReq msgNotifyReq;
				msgNotifyReq.add_username(ud->connId);
				msgNotifyReq.set_content("您有即将到期的对战门票，请及时使用。");
				msgNotifyReq.set_mode(ESystemMessageMode::ShowDialogBox);
				const char* msgData = serializeMsgToBuffer(msgNotifyReq, "remind user use PK gold ticket notify");
				if (msgData != NULL)
				{
					sendMessageToService(ServiceType::MessageSrv, msgData, msgNotifyReq.ByteSize(), MessageSrvProtocol::BUSINESS_SYSTEM_MESSAGE_NOTIFY,
					ud->connId, ud->connIdLen);
				}
			}
		}
	}
}

com_protocol::HallLogicData* CSrvMsgHandler::getHallLogicData(const char* userName)
{
	ConnectUserData* ud = NULL;
	if (userName == NULL) ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);	
	else ud = (ConnectUserData*)getProxyUserData(getConnectProxy(string(userName)));
	
	return (ud != NULL) ? ud->hallLogicData : NULL;
}

ConnectUserData* CSrvMsgHandler::getConnectUserData(const char* userName)
{
	ConnectUserData* ud = NULL;
	if (userName == NULL) ud = (ConnectUserData*)getProxyUserData(getConnectProxyContext().conn);
	else ud = (ConnectUserData*)getProxyUserData(getConnectProxy(string(userName)));
	
	return ud;
}

void CSrvMsgHandler::makeRecordPkg(com_protocol::GameRecordPkg &pkg, const RecordItemVector& items, const string &remark, const char* recordId)
{
	if (items.size() < 1) return;

	com_protocol::BuyuGameRecordStorageExt buyu_record;
	buyu_record.set_room_rate(0);
	buyu_record.set_room_name("大厅");
	buyu_record.set_remark(remark);
	if (recordId != NULL) buyu_record.set_record_id(recordId);
	
	for (unsigned int idx = 0; idx < items.size(); ++idx)
	{
		com_protocol::ItemRecordStorage* itemRecord = buyu_record.add_items();
		itemRecord->set_item_type(items[idx].type);
		itemRecord->set_charge_count(items[idx].count);
	}

	const char* recordData = serializeMsgToBuffer(buyu_record, "write game record");
	if (recordData != NULL)
	{
		pkg.set_game_type(NProject::GameRecordType::BuyuExt);
	    pkg.set_game_record_bin(recordData, buyu_record.ByteSize());
	}
}

void CSrvMsgHandler::rechargeNotifyResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::RechargeFishCoinResultNotify resultNotify;
	if (!parseMsgFromBuffer(resultNotify, data, len, "recharge fish coin result notify to client")) return;

	if (resultNotify.result() != 0)
	{
		OptErrorLog("CSrvMsgHandler rechargeNotifyResult, recharge notify result:%d", resultNotify.result());
		return;
	}

	const string userName = getContext().userData;

	//查询VIP是否可以升级
	com_protocol::GetUserOtherInfoReq db_req;
	char send_data[MaxMsgLen] = { 0 };
	db_req.set_query_username(userName);
	db_req.add_info_flag(EUserInfoFlag::EVipLevel);
	db_req.add_info_flag(EUserInfoFlag::ERechargeAmount);
	db_req.SerializeToArray(send_data, MaxMsgLen);
	sendMessageToService(NFrame::ServiceType::DBProxyCommonSrv, send_data, db_req.ByteSize(), BUSINESS_GET_USER_OTHER_INFO_REQ, userName.c_str(), userName.length(), GetUserInfoFlag::GetUserVIPLvForVIPUp);
	
	m_activityCenter.userRechargeNotify(getContext().userData);
	m_logicHandlerTwo.checkRedGiftCondition(ELoginForwardMessageNotifyType::ERechargeValue, resultNotify.money());
	
	// OptInfoLog("CSrvMsgHandler rechargeNotifyResult, recharge notify result:%d, userName:%s", resultNotify.result(), userName.c_str());
}

void CSrvMsgHandler::notifyDbProxyChangeProp(const char* username, unsigned int len, const RecordItemVector& recordItemVector,
											 int flag, const char* remark, const char* dst_username, unsigned short handleProtocolId, const char* recordId)
{
	// 通知dbproxy变更道具数量
	com_protocol::PropChangeReq preq;
	for (RecordItemVector::const_iterator it = recordItemVector.begin(); it != recordItemVector.end(); ++it)
	{
		com_protocol::ItemChangeInfo* iChangeInfo = preq.add_items();
		iChangeInfo->set_type(it->type);
		iChangeInfo->set_count(it->count);
	}

	//统计道具修改数
	statisticsChangePropNum(username, len, recordItemVector);

	if (preq.items_size() > 0)
	{
		if (dst_username != NULL) preq.set_dst_username(dst_username);

		if (remark != NULL)
		{
			com_protocol::GameRecordPkg* p_game_record = preq.mutable_game_record();
			makeRecordPkg(*p_game_record, recordItemVector, remark, recordId);
		}
		
		const char* msgData = serializeMsgToBuffer(preq, "notify dbproxy change goods count");
		if (msgData != NULL)
		{
			sendMessageToService(DBProxyCommonSrv, msgData, preq.ByteSize(), CommonDBSrvProtocol::BUSINESS_CHANGE_PROP_REQ, username, len, flag, handleProtocolId);
		}
	}
}

// 信息发往邮箱
void CSrvMsgHandler::postMessageToMail(const char* username, unsigned int len, const char* title, const char* content, const map<unsigned int, unsigned int>& rewards)
{
	com_protocol::AddMailRewardMessageReq req;
	req.set_title(title);
	req.set_content(content);
	for (map<unsigned int, unsigned int>::const_iterator it = rewards.begin(); it != rewards.end(); ++it)
	{
		com_protocol::GiftInfo* gift = req.add_gifts();
		gift->set_type(it->first);
		gift->set_num(it->second);
	}
	
	const char* msgData = serializeMsgToBuffer(req, "post message to mail request");
	if (msgData != NULL) sendMessageToService(ServiceType::DBProxyCommonSrv, msgData, req.ByteSize(), BUSINESS_ADD_MAIL_MESSAGE_REQ, username, len);
}

void CSrvMsgHandler::statisticsChangePropNum(const char* username, unsigned int len, const RecordItemVector& recordItemVector)
{
	ConnectProxy* conn = getConnectProxy(username);
	ConnectUserData* ud = (ConnectUserData*)getProxyUserData(conn);

	for (auto changeProp = recordItemVector.begin(); changeProp != recordItemVector.end(); ++changeProp)
	{
		switch (changeProp->type)
		{
		case EPropType::PropVoucher:
			if (changeProp->count > 0)
				ud->propVoucher += changeProp->count;
			break;

		case EPropType::PropPhoneFareValue:
			if (changeProp->count > 0)
				ud->phoneBill += changeProp->count;
			break;

		case EPropType::PropScores:
			if (changeProp->count > 0)
				ud->score += changeProp->count;
			break;

		default:
			break;
		}
	}
}

bool CSrvMsgHandler::isProxyMsg()
{
	return (m_msgType == MessageType::ConnectProxyMsg);
}

void CSrvMsgHandler::writeStatisticsData(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
    // 写统计日志
	// 检测是否过天
	time_t curSecs = time(NULL);
	struct tm tmVal;
	localtime_r(&curSecs, &tmVal);
	
	// 统计日志，每小时&天写一次
	static int statHour = -1;
	static int statDay = -1;
	
	if (statHour != tmVal.tm_hour)
	{
		statHour = tmVal.tm_hour;

		CStatisticsData& hourStatData = getStatDataMgr().getStatData(EStatisticsDataItem::EHourStatisticsData);
		const StatisticsGoods& lotteryDraw = hourStatData.getStatGoods(EStatisticsDataItem::ERechargeLotteryDraw);
		const StatisticsGoods& pkTask = hourStatData.getStatGoods(EStatisticsDataItem::EPKTaskStat);
		const StatisticsGoods& receivedRedGift = hourStatData.getStatGoods(EStatisticsDataItem::EReceivedRedGift);
		const StatisticsGoods& redGiftTakeRatio = hourStatData.getStatGoods(EStatisticsDataItem::ERedGiftTakeRatio);
		
		StatisticsDataLog("PhonefareLottery Statistics Hour|%d|LotteryDraw|%u|%u|PKTask|%u|%u|ReceivedRedGift|%u|%u|RedGiftTakeRatio|%u|%u", statHour,
		lotteryDraw.phoneFare, lotteryDraw.scoreVoucher, pkTask.phoneFare, pkTask.scoreVoucher,
		receivedRedGift.phoneFare, receivedRedGift.scoreVoucher, redGiftTakeRatio.phoneFare, redGiftTakeRatio.scoreVoucher);
		
		hourStatData.resetAllStatGoods();
	}
	
	if (statDay != tmVal.tm_yday)	//跨天了
	{
		statDay = tmVal.tm_yday;

		CStatisticsData& statTotalData = getStatDataMgr().getStatData(EStatisticsDataItem::EAllStatisticsData);
		const StatisticsGoods& allLotteryDraw = statTotalData.getStatGoods(EStatisticsDataItem::ERechargeLotteryDraw);
		const StatisticsGoods& allPkTask = statTotalData.getStatGoods(EStatisticsDataItem::EPKTaskStat);
		const StatisticsGoods& allReceivedRedGift = statTotalData.getStatGoods(EStatisticsDataItem::EReceivedRedGift);
		const StatisticsGoods& allRedGiftTakeRatio = statTotalData.getStatGoods(EStatisticsDataItem::ERedGiftTakeRatio);
		
		CStatisticsData& dayStatData = getStatDataMgr().getStatData(EStatisticsDataItem::EDayStatisticsData);
		const StatisticsGoods& dayLotteryDraw = dayStatData.getStatGoods(EStatisticsDataItem::ERechargeLotteryDraw);
		const StatisticsGoods& dayPkTask = dayStatData.getStatGoods(EStatisticsDataItem::EPKTaskStat);
		const StatisticsGoods& dayReceivedRedGift = dayStatData.getStatGoods(EStatisticsDataItem::EReceivedRedGift);
		const StatisticsGoods& dayRedGiftTakeRatio = dayStatData.getStatGoods(EStatisticsDataItem::ERedGiftTakeRatio);
		
		
		StatisticsDataLog("PhonefareLottery Statistics Day|%d|LotteryDraw|%u|%u|%u|%u|PKTask|%u|%u|%u|%u|ReceivedRedGift|%u|%u|%u|%u|RedGiftTakeRatio|%u|%u|%u|%u", statDay,
		allLotteryDraw.phoneFare, allLotteryDraw.scoreVoucher, dayLotteryDraw.phoneFare, dayLotteryDraw.scoreVoucher,
		allPkTask.phoneFare, allPkTask.scoreVoucher, dayPkTask.phoneFare, dayPkTask.scoreVoucher,
		allReceivedRedGift.phoneFare, allReceivedRedGift.scoreVoucher, dayReceivedRedGift.phoneFare, dayReceivedRedGift.scoreVoucher,
		allRedGiftTakeRatio.phoneFare, allRedGiftTakeRatio.scoreVoucher, dayRedGiftTakeRatio.phoneFare, dayRedGiftTakeRatio.scoreVoucher);
		
		dayStatData.resetAllStatGoods();
	}
}


// only for test，测试异步消息本地数据存储
struct STestAsyncLocalData
{
	int iValue;
	short sValue;
	bool bValue;
	char cValue;
	char strValue[100];
};

void CSrvMsgHandler::testAsyncLocalData()
{
	/*
	registerProtocol(CommonSrv, ETestProtocolId::ETEST_ASYNC_MSG_LOCAL_DATA_RSP, (ProtocolHandler)&CSrvMsgHandler::testAsyncLocalDataReply, this);
	
	com_protocol::ServiceTestReq req;
	req.set_name("test_name_");
	req.set_id("test async local data ID_");
	req.set_age(100);
	req.set_gold(100000000);
	req.set_desc("only for test async local data read and write!");
	
	const char* msgData = serializeMsgToBuffer(req, "only for test async local data read and write!");
	if (msgData != NULL)
	{
		// 先获取本地数据存储空间
		STestAsyncLocalData* localData = (STestAsyncLocalData*)m_onlyForTestAsyncData.createData();  // 这种方式高效，直接返回存储空间
		if (localData != NULL)
		{
			// 本地数据必须在发送消息之前存储
			// 直接存储本地数据
			localData->iValue = 30012003;
			localData->sValue = 5601;
			localData->bValue = true;
			localData->cValue = 'T';
			strncpy(localData->strValue, req.desc().c_str(), sizeof(localData->strValue) - 1);

			// m_onlyForTestAsyncData.createData((const char*)&localData, sizeof(localData));  // 这种方式效率低下，多了一次数据拷贝
			
			int rc1 = sendMessageToService(ServiceType::DBProxyCommonSrv, msgData, req.ByteSize(), ETestProtocolId::ETEST_ASYNC_MSG_LOCAL_DATA_REQ, req.name().c_str(), req.name().length(), ServiceType::DBProxyCommonSrv);
			int rc2 = sendMessageToService(ServiceType::ManageSrv, msgData, req.ByteSize(), ETestProtocolId::ETEST_ASYNC_MSG_LOCAL_DATA_REQ, req.name().c_str(), req.name().length(), ServiceType::ManageSrv);
			
			OptInfoLog("Test Async Local Data Request, localData = %p, rc1 = %d, rc2 = %d", localData, rc1, rc2);
		}
	}
	*/ 
}

void CSrvMsgHandler::testAsyncLocalDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ServiceTestRsp rsp;
	if (!parseMsgFromBuffer(rsp, data, len, "test async local data reply")) return;
	
	OptInfoLog("Test Async Local Data Reply, userData = %s, userFlag = %d, result = %d, desc = %s",
	getContext().userData, getContext().userFlag, rsp.result(), rsp.desc().c_str());
	
	const STestAsyncLocalData* localData = (const STestAsyncLocalData*)m_onlyForTestAsyncData.getData();  // 获取本地存储数据
	if (localData != NULL)
	{
		OptInfoLog("Test Async Local Data Reply, iValue = %d, sValue = %d, bValue = %d, cValue = %c, strValue = %s",
	    localData->iValue, localData->sValue, localData->bValue, localData->cValue, localData->strValue);
		
		if (ServiceType::ManageSrv == getContext().userFlag)
		{
			m_onlyForTestAsyncData.destroyData((const char*)localData);
			OptInfoLog("Test Async Local Data Remove, localData = %p", localData);
		}
	}
	
	OptInfoLog("Test Async Local Data Get Again, localData = %p", m_onlyForTestAsyncData.getData());
}





static CSrvMsgHandler s_msgHandler;  // 消息处理模块实例

int CLoginSrv::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("run login service name = %s, id = %d", name, id);
	return 0;
}

void CLoginSrv::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("stop login service name = %s, id = %d", name, id);
}

void CLoginSrv::onRegister(const char* name, const unsigned int id)
{
	ReleaseInfoLog("register login module, service name = %s, id = %d", name, id);
	
	// 注册模块实例
	const unsigned short HandlerMessageModule = 0;
	m_connectNotifyToHandler = &s_msgHandler;
	registerModule(HandlerMessageModule, &s_msgHandler);
}

void CLoginSrv::onUpdateConfig(const char* name, const unsigned int id)
{
	const HallConfigData::config& cfgValue = HallConfigData::config::getConfigValue(CCfg::getValue("LoginService", "BusinessXmlConfigFile"), true);
	ReleaseInfoLog("update config value, service name = %s, id = %d, result = %d", name, id, cfgValue.isSetConfigValueSuccess());
	cfgValue.output();
	
	 s_msgHandler.updateConfig();
}

// 通知逻辑层对应的逻辑连接已被关闭
void CLoginSrv::onClosedConnect(void* userData)
{
	m_connectNotifyToHandler->onClosedConnect(userData);
}


CLoginSrv::CLoginSrv() : IService(LoginSrv)
{
	m_connectNotifyToHandler = NULL;
}

CLoginSrv::~CLoginSrv()
{
	m_connectNotifyToHandler = NULL;
}

REGISTER_SERVICE(CLoginSrv);

}

