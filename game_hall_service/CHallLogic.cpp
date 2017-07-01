
/* author : limingfan
 * date : 2015.04.23
 * description : 大厅逻辑服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "CHallLogic.h"
#include "CLoginSrv.h"
#include "base/ErrorCode.h"
#include "connect/CSocket.h"
#include "common/CommonType.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace NProject;
using namespace HallConfigData;

#define VIP_ADDITIONAL_NUM	0			//VIP奖励领取后，破产补助奖励的次数
#define DAY_SEC				(24*60*60)		//一天的秒数

namespace NPlatformService
{

// 数据记录日志
#define WriteDataLog(format, args...)     m_msgHandler->getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)


// 登陆奖励状态
enum LoginRewardStatus
{
	NoReward = 0,
	CanGetReward = 1,
	AlreadyGeted = 2,
};

// 好友状态
enum FriendStatus
{
	OffLine = 0,
	InHall = 1,
	InGame = 2,
};


//公告阅读标识
enum ReadNoticeFlag
{
	notRead = 0,		//未阅读
	read = 1,
};

enum NoticeShowType
{
	notEject = 0,
	eject = 1,
};

//退出挽留奖励
enum EQuitRetainRewardType
{
	EQuitRetainRewardNot = 0,	//没有奖励
	EQuitRetainRewardVIP,		//VIP奖励
	EQuitRetainRewardSign,		//签到奖励
	EQuitRetainRewardLuckDraw,  //抽奖
};


int CVersionManager::getVersionInfo(const unsigned int deviceType, const unsigned int platformType, const string& curVersion, unsigned int& flag, string& newVersion, string& newFileURL)
{
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	const map<int, HallConfigData::client_version>* client_version_list = NULL;
	if (deviceType == ClientVersionType::AndroidSeparate)
		client_version_list = &cfg.android_version_list;
	else if (deviceType == ClientVersionType::AppleMerge)
		client_version_list = &cfg.apple_version_list;
	else if (deviceType == ClientVersionType::AndroidMerge)
		client_version_list = &cfg.android_merge_version;
	else
		return ClientVersionInvalid;
	
	map<int, HallConfigData::client_version>::const_iterator verIt = client_version_list->find(platformType);
	if (verIt == client_version_list->end()) return ClientVersionConfigError;
	
	const HallConfigData::client_version& cVer = verIt->second;
	newVersion = cVer.version;
	newFileURL = cVer.url;
	flag = ((curVersion < cVer.valid) || (curVersion > newVersion)) ? 1 : 0;  // 正式版本号
	if (flag == 1)
	{
		flag = (cVer.check_flag != 1 || curVersion != cVer.check_version) ? 1 : 0;  // 校验测试版本号
		if (flag == 0) newVersion = cVer.check_version;                             // 修改为审核版本号
	}

	OptInfoLog("client version data, ip = %s, port = %d, device type = %d, platform type = %d, current version = %s, valid version = %s, flag = %d, new version = %s, file url = %s, check flag = %d, version = %s", CSocket::toIPStr(m_msgHandler->getConnectProxyContext().conn->peerIp), m_msgHandler->getConnectProxyContext().conn->peerPort, deviceType, platformType, curVersion.c_str(), cVer.valid.c_str(), flag, newVersion.c_str(), newFileURL.c_str(), cVer.check_flag, cVer.check_version.c_str());
	
	return Success;
}


CHallLogic::CHallLogic() : m_msgHandler(NULL), m_dataContent(DataContentSize, DataContentCount)
{
	m_friendDonateGoldIndex = HallDataFriendOpt::DonateGoldToFriend;
	m_noticeVersion = 0;
	
	m_clearScoresNum = 0;
	m_clearScoresTime = 0;
	m_clearScoreTimerId = 0;
}

CHallLogic::~CHallLogic()
{
	m_msgHandler = NULL;
	m_friendDonateGoldIndex = 0;
}

// 大厅逻辑处理
void CHallLogic::getLoginRewardList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get login reward list")) return;

    com_protocol::ClientGetLoginRewardListRsp loginRewardList;
	com_protocol::LoginReward* loginReward = m_msgHandler->getHallLogicData()->mutable_login_reward();
	loginRewardList.set_day(loginReward->status_size());
	
	const bool isSpecial = m_msgHandler->getConnectUserData()->platformType == ThirdPlatformType::AoRong;  // 奥榕平台配置特殊值
	const vector<login_reward>& login_reward_list = HallConfigData::config::getConfigValue().login_reward_list;
	for (unsigned int i = 0; i < login_reward_list.size(); ++i)
	{
		com_protocol::RewardItem* rwItem = loginRewardList.add_reward_item();
		if ((int)i < loginReward->status_size()) rwItem->set_status(loginReward->status(i));
		else  rwItem->set_status(LoginRewardStatus::NoReward);
		
		rwItem->set_type(login_reward_list[i].type);
		rwItem->set_num(isSpecial ? login_reward_list[i].special : login_reward_list[i].num);
	}
	
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(loginRewardList, "get login reward list");
	if (msgBuffer != NULL)
	{
		m_msgHandler->sendMsgToProxy(msgBuffer, loginRewardList.ByteSize(), LOGINSRV_GET_LOGIN_REWARD_LIST_RSP);
	}
}

void CHallLogic::receiveLoginReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to receive login reward")) return;
	
	com_protocol::ClientReceiveLoginRewardReq receiveLoginRewardReq;
	if (!m_msgHandler->parseMsgFromBuffer(receiveLoginRewardReq, data, len, "receive login reward")) return;

	int result = 0;
	com_protocol::LoginReward* loginReward = m_msgHandler->getHallLogicData()->mutable_login_reward();
	int the_day = receiveLoginRewardReq.the_day();
	if (the_day != loginReward->status_size())
	{
		result = ProtocolReturnCode::NoLoginRewardReceive;
	}
	else if (loginReward->status(the_day - 1) != LoginRewardStatus::CanGetReward)
	{
		result = ProtocolReturnCode::ReceivedLoginReward;
	}
	
	if (result != 0)
	{
		com_protocol::ClientReceiveLoginRewardRsp loginRewardRsp;
		loginRewardRsp.set_result(result);
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(loginRewardRsp, "receive login reward param error");
		if (msgBuffer != NULL)
		{
			m_msgHandler->sendMsgToProxy(msgBuffer, loginRewardRsp.ByteSize(), LOGINSRV_RECEIVE_LOGIN_REWARD_RSP);
		}
		return;
	}
	
	// 先提前设置领取状态，防止变更金币，异步消息回来之前被疯狂领取多次
	loginReward->set_status(loginReward->status_size() - 1, LoginRewardStatus::AlreadyGeted);
	
	com_protocol::RoundEndDataChargeReq addLoginRewardReq;
	addLoginRewardReq.set_game_id(GameType::ALL_GAME);
	
	// 奥榕平台配置特殊值
	const vector<login_reward>& login_reward_list = HallConfigData::config::getConfigValue().login_reward_list;
	const unsigned int loginGold = (m_msgHandler->getConnectUserData()->platformType == ThirdPlatformType::AoRong) ? login_reward_list[the_day - 1].special : login_reward_list[the_day - 1].num;
	addLoginRewardReq.set_delta_game_gold(loginGold);
	
	com_protocol::GameRecordPkg* gRecord = addLoginRewardReq.mutable_game_record();
	gRecord->set_game_type(0);
	
	com_protocol::BuyuGameRecordStorage recordStore;
	recordStore.set_room_rate(0);
	recordStore.set_room_name("大厅");
	recordStore.set_item_type(EPropType::PropGold);
	recordStore.set_charge_count(loginGold);
	recordStore.set_remark("登陆获得奖励");
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(recordStore, "user receive login reward record");
	if (msgBuffer != NULL) gRecord->set_game_record_bin(msgBuffer, recordStore.ByteSize());
		
	msgBuffer = m_msgHandler->serializeMsgToBuffer(addLoginRewardReq, "add login reward");
	if (msgBuffer != NULL)
	{
		m_msgHandler->sendMessageToCommonDbProxy(msgBuffer, addLoginRewardReq.ByteSize(), BUSINESS_ROUND_END_DATA_CHARGE_REQ);
	}
}

void CHallLogic::addGoldReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::RoundEndDataChargeRsp addGoldRsp;
	if (!m_msgHandler->parseMsgFromBuffer(addGoldRsp, data, len, "add gold reply")) return;
	
	if (m_msgHandler->getContext().userFlag >= HallDataFriendOpt::DonateGoldToFriend)
	{
	    return donateGoldToFriendReply(addGoldRsp, m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, m_msgHandler->getContext().userFlag);
	}

	ConnectProxy* conn = m_msgHandler->getConnectProxy(m_msgHandler->getContext().userData);
	if (conn == NULL)
	{
		OptErrorLog("in addGoldReply can not find the connect data, user data = %s", m_msgHandler->getContext().userData);
		return;
	}
	
	com_protocol::ClientReceiveLoginRewardRsp clientReceiveLoginRewardRsp;
	clientReceiveLoginRewardRsp.set_result(addGoldRsp.result());
	if (addGoldRsp.result() == 0)
	{
		ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
		com_protocol::LoginReward* loginReward = ud->hallLogicData->mutable_login_reward();
		// loginReward->set_status(loginReward->status_size() - 1, LoginRewardStatus::AlreadyGeted);
		
		const vector<login_reward>& login_reward_list = HallConfigData::config::getConfigValue().login_reward_list;
		ConnectUserData* cud = m_msgHandler->getConnectUserData(m_msgHandler->getContext().userData);
		const bool isSpecial = (cud != NULL && cud->platformType == ThirdPlatformType::AoRong);  // 奥榕平台配置特殊值
		 clientReceiveLoginRewardRsp.set_gold(isSpecial ? login_reward_list[loginReward->status_size() - 1].special : login_reward_list[loginReward->status_size() - 1].num);
	}

    const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(clientReceiveLoginRewardRsp, "add login reward reply to client");
	if (msgBuffer != NULL) m_msgHandler->sendMsgToProxy(msgBuffer, clientReceiveLoginRewardRsp.ByteSize(), LOGINSRV_RECEIVE_LOGIN_REWARD_RSP, conn);
}

bool CHallLogic::getUserHallLogicData(const char* username, int opt, unsigned short srcProtocolId, const char* cbData, const unsigned int cbDataLen)
{
	// 获取非在线用户大厅逻辑数据
	com_protocol::GetGameDataReq hallLogicData;
	hallLogicData.set_primary_key(HallLogicDataKey);
	hallLogicData.set_second_key(username);
	if (cbData != NULL && cbDataLen > 0) hallLogicData.set_tc_data(cbData, cbDataLen);
	
	if (srcProtocolId == 0) srcProtocolId = LOGINSRV_GET_OFFLINE_USER_HALL_DATA_RSP;
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(hallLogicData, "get offline user hall data");
	return (msgBuffer != NULL && m_msgHandler->sendMessageToGameDbProxy(msgBuffer, hallLogicData.ByteSize(), GET_GAME_DATA_REQ, 
												                        hallLogicData.second_key().c_str(), hallLogicData.second_key().length(), srcProtocolId, opt) == Success);
}

bool CHallLogic::addFriendOpt(com_protocol::HallLogicData& hallLogicData, com_protocol::AddFriendReq& addFriendReq, unsigned int srcSrvId)
{
	com_protocol::AddFriendRsp addFriendRsp;
	addFriendRsp.set_allocated_friend_(&addFriendReq);
	addFriendRsp.set_result(Opt_Success);
	
	const ::std::string& dst_username = addFriendReq.dst_username();
	com_protocol::FriendData* friendData = hallLogicData.mutable_friends();
	bool isFind = false;
	for (int idx = 0; idx < friendData->username_size(); ++idx)
	{
		if (friendData->username(idx) == dst_username)
		{
			isFind = true;
        	break;
		}
	}
	if (!isFind) friendData->add_username(dst_username);
	
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(addFriendRsp, "add friend request");
	if (msgBuffer != NULL) m_msgHandler->sendMessage(msgBuffer, addFriendRsp.ByteSize(), srcSrvId, ADD_FRIEND_RSP);
	
	addFriendRsp.release_friend_();
	
	return !isFind;
}

void CHallLogic::addFriendNotifyClient(const char* data, const unsigned int len)
{
	com_protocol::GetMultiUserInfoRsp getFriendStaticDataRsp;
	if (!m_msgHandler->parseMsgFromBuffer(getFriendStaticDataRsp, data, len, "get offline friend static data reply")
		|| getFriendStaticDataRsp.user_simple_info_lst(0).result() != 0) return;
	
	const string userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL)
	{
		OptErrorLog("in addFriendNotifyClient can not find the connect data, user data = %s", m_msgHandler->getContext().userData);
		return;
	}
	
	const com_protocol::UserSimpleInfo& staticData = getFriendStaticDataRsp.user_simple_info_lst(0);
	com_protocol::ClientAddFriendRsp addFriendNotify;
	addFriendNotify.set_result(0);
	addFriendNotify.set_dst_username(staticData.username());
	addFriendNotify.set_dst_nickname(staticData.nickname());
	addFriendNotify.set_dst_portrait_id(staticData.portrait_id());
	
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(addFriendNotify, "add friend notify client");
	if (msgBuffer != NULL) m_msgHandler->sendMsgToProxy(msgBuffer, addFriendNotify.ByteSize(), LOGINSRV_ADD_FRIEND_NOTIFY, conn);
}

void CHallLogic::addFriend(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::AddFriendReq addFriendReq;
	if (!m_msgHandler->parseMsgFromBuffer(addFriendReq, data, len, "add friend request")) return;
	
	if (addFriendReq.src_username() == addFriendReq.dst_username())
	{
		OptWarnLog("in addFriend friend name same as oneself = %s", addFriendReq.src_username().c_str());
		return;
	}
	
	ConnectProxy* conn = m_msgHandler->getConnectProxy(addFriendReq.src_username());
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	if (ud != NULL)
	{
		if (addFriendOpt(*(ud->hallLogicData), addFriendReq, srcSrvId))
		{
			ConnectUserData* dstUd = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxy(addFriendReq.dst_username()));
			if (dstUd != NULL)
			{
				// 对方目前用户在线
				com_protocol::ClientAddFriendRsp addFriendNotify;
				addFriendNotify.set_result(0);
				addFriendNotify.set_dst_username(dstUd->connId);
				addFriendNotify.set_dst_nickname(dstUd->nickname);
				addFriendNotify.set_dst_portrait_id(dstUd->portrait_id);
				
				const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(addFriendNotify, "add friend notify client");
				if (msgBuffer != NULL) m_msgHandler->sendMsgToProxy(msgBuffer, addFriendNotify.ByteSize(), LOGINSRV_ADD_FRIEND_NOTIFY, conn);
			}
			else
			{
				// 离线用户则需要获取其静态信息
				com_protocol::GetMultiUserInfoReq getFriendStaticDataReq;
				getFriendStaticDataReq.add_username_lst(addFriendReq.dst_username());
				const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(getFriendStaticDataReq, "get offline friend static data");
				if (msgBuffer != NULL)
				{
					unsigned int commonDbProxyId = m_msgHandler->getCommonDbProxySrvId(addFriendReq.src_username().c_str(), addFriendReq.src_username().length());
					m_msgHandler->sendMessage(msgBuffer, getFriendStaticDataReq.ByteSize(), HallDataFriendOpt::AddFriendNotifyClient,
											  addFriendReq.src_username().c_str(), addFriendReq.src_username().length(), commonDbProxyId, BUSINESS_GET_MULTI_USER_INFO_REQ);
				}
			}
		}
	}
	else if (getUserHallLogicData(addFriendReq.src_username().c_str(), HallDataFriendOpt::AddFriend))  // 获取非在线用户大厅逻辑数据
	{
		m_addFriendOptContext[addFriendReq.src_username()] = FriendOpt(addFriendReq.dst_username(), srcSrvId);
	}
}

void CHallLogic::removeFriendByName(com_protocol::HallLogicData& hallLogicData, const ::std::string& dst_username)
{
	com_protocol::FriendData* friendData = hallLogicData.mutable_friends();
	for (int idx = 0; idx < friendData->username_size(); ++idx)
	{
		if (friendData->username(idx) == dst_username)
		{
			friendData->mutable_username()->DeleteSubrange(idx, 1);
			break;
		}
	}
}

void CHallLogic::removeFriendOpt(com_protocol::HallLogicData& hallLogicData, com_protocol::RemoveFriendReq& removeFriendReq, unsigned int srcSrvId)
{
	com_protocol::RemoveFriendRsp removeFriendRsp;
	removeFriendRsp.set_allocated_friend_(&removeFriendReq);
	removeFriendRsp.set_result(Opt_Success);
	removeFriendByName(hallLogicData, removeFriendReq.dst_username());
	
	if (srcSrvId > 0)
	{
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(removeFriendRsp, "remove friend request");
		if (msgBuffer != NULL) m_msgHandler->sendMessage(msgBuffer, removeFriendRsp.ByteSize(), srcSrvId, REMOVE_FRIEND_RSP);
	}
	
	removeFriendRsp.release_friend_();
}

void CHallLogic::removeFriend(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::RemoveFriendReq removeFriendReq;
	if (!m_msgHandler->parseMsgFromBuffer(removeFriendReq, data, len, "remove friend request")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxy(removeFriendReq.src_username()));
	if (ud != NULL)
	{
		removeFriendOpt(*(ud->hallLogicData), removeFriendReq, srcSrvId);
	}
	else if (getUserHallLogicData(removeFriendReq.src_username().c_str(), HallDataFriendOpt::RemoveFriend))  // 获取非在线用户大厅逻辑数据
	{
		m_removeFriendOptContext[removeFriendReq.src_username()] = FriendOpt(removeFriendReq.dst_username(), srcSrvId);
	}
}

void CHallLogic::getOfflineUserHallDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	FriendOpt friendOptData;
	GetFriendDynamicDataContext::iterator getFriendDynamicDataContextIt;
	
	const string userName = m_msgHandler->getContext().userData;
	const int userFlag = m_msgHandler->getContext().userFlag;
	if (userFlag == HallDataFriendOpt::GetFriendDynamicData)
	{
		getFriendDynamicDataContextIt = m_getFriendDynamicData.find(userName);
		if (getFriendDynamicDataContextIt == m_getFriendDynamicData.end())
		{
			OptErrorLog("can not find the operate record, user name = %s, flag = %d", m_msgHandler->getContext().userData, userFlag);
			return;
		}
	}
	else if (userFlag >= HallDataFriendOpt::DonateGoldToFriend)
	{
		// now to do nothing
	}
	else
	{
		FriendOptContext& rFriendOptCtx = (userFlag == HallDataFriendOpt::AddFriend) ? m_addFriendOptContext : m_removeFriendOptContext;
		FriendOptContext::iterator it = rFriendOptCtx.find(userName);
		if (it == rFriendOptCtx.end())
		{
			OptErrorLog("can not find the operate record, user name = %s, flag = %d", m_msgHandler->getContext().userData, userFlag);
			return;
		}
		
		friendOptData = it->second;
		rFriendOptCtx.erase(it);
	}
	
	com_protocol::GetGameDataRsp hallLogicDataRsp;
	if (!m_msgHandler->parseMsgFromBuffer(hallLogicDataRsp, data, len, "get offline user hall logic data reply")) return;
	
	if (hallLogicDataRsp.result() != 0)
	{
		OptErrorLog("get offline user hall logic data reply error, user data = %s, flag = %d, rc = %d", 
		m_msgHandler->getContext().userData, userFlag, hallLogicDataRsp.result());
		return;
	}
	
	if (!hallLogicDataRsp.has_data()) return;

	// 提取用户逻辑数据信息
	com_protocol::HallLogicData hallLogicData;
	if (!m_msgHandler->parseMsgFromBuffer(hallLogicData, hallLogicDataRsp.data().c_str(), hallLogicDataRsp.data().length(), "get offline user hall logic data"))
	{
		OptErrorLog("get offline user logic data from redis error, user name = %s, flag = %d", m_msgHandler->getContext().userData, userFlag);
		return;
	}

    // 获取好友动态信息
    if (userFlag == HallDataFriendOpt::GetFriendDynamicData)
	{
		com_protocol::FriendData* friendData = hallLogicData.mutable_friends();
		if (friendData->username_size() < 1)
		{
			com_protocol::ClientGetFriendDynamicDataRsp rsp;
			const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "get friend dynamic data");
			if (msgBuffer != NULL) m_msgHandler->sendMessage(msgBuffer, rsp.ByteSize(), getFriendDynamicDataContextIt->second, LOGINSRV_GET_FRIEND_DYNAMIC_DATA_RSP);
			
			m_getFriendDynamicData.erase(getFriendDynamicDataContextIt);
			return;
		}
	
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(*friendData, "get friend dynamic data");
		if (msgBuffer != NULL) m_msgHandler->sendMessageToService(ServiceType::MessageSrv, msgBuffer, friendData->ByteSize(), BUSINESS_GET_USER_SERVICE_ID_REQ, userName.c_str(), userName.length());
	
		return;
	}
	
	if (userFlag == HallDataFriendOpt::AddFriend)
	{
		// 加好友
		com_protocol::AddFriendReq addFriendReq;
		addFriendReq.set_src_username(userName);
	    addFriendReq.set_dst_username(friendOptData.username);
		if (!addFriendOpt(hallLogicData, addFriendReq, friendOptData.srvId)) return;
	}
	else if (userFlag >= HallDataFriendOpt::DonateGoldToFriend)
	{
		if (!setDonateGoldToOfflineUser(hallLogicData, userFlag)) return;
	}
	else
	{
		// 删除好友
		com_protocol::RemoveFriendReq removeFriendReq;
	    removeFriendReq.set_src_username(userName);
	    removeFriendReq.set_dst_username(friendOptData.username);
    	removeFriendOpt(hallLogicData, removeFriendReq, friendOptData.srvId);
	}
	
	// 刷新数据到DB
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(hallLogicData, "save offline user hall logic data");
	if (msgBuffer != NULL)
	{
		com_protocol::SetGameDataReq saveLogicData;
		saveLogicData.set_primary_key(HallLogicDataKey);
		saveLogicData.set_second_key(userName);
		saveLogicData.set_data(msgBuffer, hallLogicData.ByteSize());
		msgBuffer = m_msgHandler->serializeMsgToBuffer(saveLogicData, "set offline user hall logic data to redis");
		if (msgBuffer != NULL) m_msgHandler->sendMessageToGameDbProxy(msgBuffer, saveLogicData.ByteSize(), SET_GAME_DATA_REQ, userName.c_str(), userName.length());
	}
}

void CHallLogic::removeFriendByClient(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to remove friend")) return;
	
	com_protocol::ClientRemoveFriendReq removeFriendReq;
	if (!m_msgHandler->parseMsgFromBuffer(removeFriendReq, data, len, "client remove friend request")) return;
	
	const ::std::string& dst_username = removeFriendReq.dst_username();
	com_protocol::ClientRemoveFriendRsp removeFriendRsp;
	removeFriendRsp.set_dst_username(dst_username);
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	removeFriendRsp.set_result(Opt_Success);
	removeFriendByName(*(ud->hallLogicData), dst_username);

	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(removeFriendRsp, "client remove friend request");
	if (msgBuffer != NULL) m_msgHandler->sendMsgToProxy(msgBuffer, removeFriendRsp.ByteSize(), LOGINSRV_REMOVE_FRIEND_RSP);
	
	// 对方好友列表相互删除
	ConnectUserData* dstUd = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxy(dst_username));
	if (dstUd != NULL)
	{
		removeFriendByName(*(dstUd->hallLogicData), ud->connId);
	}
	else
	{
		com_protocol::FriendData friendData;
		friendData.add_username(dst_username);
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(friendData, "get friend dynamic data for remove friend");
		if (msgBuffer != NULL)
		{
			m_msgHandler->sendMessageToService(ServiceType::MessageSrv, msgBuffer, friendData.ByteSize(), BUSINESS_GET_USER_SERVICE_ID_REQ,
			                                   ud->connId, ud->connIdLen, HallDataFriendOpt::GetDynamicDataForRemoveOther);
		}
	}
}

void CHallLogic::removeOtherFriend(const com_protocol::GetUserServiceIDRsp& getUserSrvIdRsp)
{
	const com_protocol::UserSerivceInfo& userSrvInfo = getUserSrvIdRsp.user_service_lst(0);
	const string& username = userSrvInfo.username();
	if (userSrvInfo.login_service_id() != 0)
	{
		com_protocol::RemoveFriendReq removeFriendReq;
		removeFriendReq.set_src_username(username);
		removeFriendReq.set_dst_username(m_msgHandler->getContext().userData);
		
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(removeFriendReq, "remove other friend");
		if (msgBuffer != NULL)
		{
			m_msgHandler->sendMessage(msgBuffer, removeFriendReq.ByteSize(), username.c_str(), username.length(), userSrvInfo.login_service_id(), REMOVE_FRIEND_REQ);
		}
	}
	else if (getUserHallLogicData(username.c_str(), HallDataFriendOpt::RemoveFriend))  // 获取非在线用户大厅逻辑数据
	{
		m_removeFriendOptContext[username] = FriendOpt(m_msgHandler->getContext().userData, 0);
	}
	
	// OptWarnLog("remove other friend, friend name = %s, login id = %d", username.c_str(), userSrvInfo.login_service_id());
}

void CHallLogic::removeFriendReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// now do nothing
}

void CHallLogic::donateGoldToFriend(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientFriendDonateGoldReq clientFriendDonateGoldReq;
	if (!m_msgHandler->parseMsgFromBuffer(clientFriendDonateGoldReq, data, len, "donate gold to friend")) return;
	
	ConnectUserData* ud = NULL;
	bool isProxyMsg = m_msgHandler->isProxyMsg();
	if (isProxyMsg)
    {
		if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get friend dynamic data")) return;
		ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	}
	else
	{
		ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxy(string(m_msgHandler->getContext().userData)));
	}
	
	if (ud == NULL)
	{
		OptErrorLog("receive msg to donate gold to friend, can not find user data, net flag = %d", isProxyMsg);
		return;
	}

    // 总是返回成功
    com_protocol::ClientFriendDonateGoldRsp clientFriendDonateGoldRsp;
	clientFriendDonateGoldRsp.set_result(0);
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(clientFriendDonateGoldRsp, "donate gold reply to client");
	if (msgBuffer != NULL)
	{
	    if (isProxyMsg) m_msgHandler->sendMsgToProxy(msgBuffer, clientFriendDonateGoldRsp.ByteSize(), LOGINSRV_FRIEND_DONATE_GOLD_RSP);
		else m_msgHandler->sendMessage(msgBuffer, clientFriendDonateGoldRsp.ByteSize(), srcSrvId, LOGINSRV_FRIEND_DONATE_GOLD_RSP);
	}

    bool alreadyDonate = false;
	com_protocol::FriendDonateGoldData* donateGold = ud->hallLogicData->mutable_donate_gold();
	for (int i = 0; i < donateGold->username_size(); ++i)
	{
		if (donateGold->username(i) == clientFriendDonateGoldReq.friend_username())
		{
			alreadyDonate = true;
			break;
		}
	}

	if (!alreadyDonate)
	{
		// 先直接处理，给对方好友加金币
		const HallConfigData::FriendDonateGold& friend_donate_gold = HallConfigData::config::getConfigValue().friend_donate_gold;
		int donateGoldNumber = getRandNumber(friend_donate_gold.min, friend_donate_gold.max);
	
		com_protocol::RoundEndDataChargeReq addFriendDonateGoldReq;
		addFriendDonateGoldReq.set_game_id(GameType::ALL_GAME);
		addFriendDonateGoldReq.set_delta_game_gold(donateGoldNumber);
		
		com_protocol::GameRecordPkg* gRecord = addFriendDonateGoldReq.mutable_game_record();
		gRecord->set_game_type(0);
	
		com_protocol::BuyuGameRecordStorage recordStore;
		recordStore.set_room_rate(0);
		recordStore.set_room_name("大厅");
		recordStore.set_item_type(EPropType::PropGold);
		recordStore.set_charge_count(donateGoldNumber);
		recordStore.set_remark("好友赠送的奖励");
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(recordStore, "user receive login reward record");
		if (msgBuffer != NULL) gRecord->set_game_record_bin(msgBuffer, recordStore.ByteSize());
	
        msgBuffer = m_msgHandler->serializeMsgToBuffer(addFriendDonateGoldReq, "add friend dontate gold");
		if (msgBuffer != NULL)
		{
			if (++m_friendDonateGoldIndex < HallDataFriendOpt::DonateGoldToFriend) m_friendDonateGoldIndex = HallDataFriendOpt::DonateGoldToFriend;
			int rc = m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgBuffer, addFriendDonateGoldReq.ByteSize(), BUSINESS_ROUND_END_DATA_CHARGE_REQ,
														clientFriendDonateGoldReq.friend_username().c_str(), clientFriendDonateGoldReq.friend_username().length(), m_friendDonateGoldIndex);
			if (rc == Success)
			{
				// 记录捐送金币信息
				FriendDonateGoldData* fdData = (FriendDonateGoldData*)m_dataContent.getBuffer();
				memcpy(fdData->userName, ud->connId, ud->connIdLen + 1);
				fdData->gold = donateGoldNumber;
				fdData->notifySrvId = isProxyMsg ? 0 : srcSrvId;
				
				strInt_t idxKey = {0};
				m_dataContent.set(OperationType::FriendDonateGold, intValueToChars(m_friendDonateGoldIndex, idxKey), (char*)fdData);
				
				donateGold->add_username(clientFriendDonateGoldReq.friend_username());
			}
			else
			{
				--m_friendDonateGoldIndex;
				OptErrorLog("send message to db common to add friend donate gold error = %d, friend = %s", rc, clientFriendDonateGoldReq.friend_username().c_str());
			}
		}
	}
}

int CHallLogic::sendDonateGoldToFriendNotify(FriendDonateGoldData* fdData, ConnectProxy* conn)
{
	// 即时通知在线用户好友
	com_protocol::ClientFriendDonateGoldNotify donateGoldNotify;
	com_protocol::DonateGold* dg = donateGoldNotify.add_donatelist();
	dg->set_username(fdData->userName);
	dg->set_gold(fdData->gold);
	time_t curSecs = time(NULL);
	struct tm* tmval = localtime(&curSecs);
	dg->set_month(tmval->tm_mon + 1);
	dg->set_day(tmval->tm_mday);

    int rc = Opt_Failed;
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(donateGoldNotify, "donate gold to friend notify");
	if (msgBuffer != NULL)
	{
		if (conn != NULL) rc = m_msgHandler->sendMsgToProxy(msgBuffer, donateGoldNotify.ByteSize(), LOGINSRV_FRIEND_DONATE_GOLD_NOTIFY, conn);
		else if (fdData->notifySrvId > 0) rc = m_msgHandler->sendMessage(msgBuffer, donateGoldNotify.ByteSize(), fdData->notifySrvId, LOGINSRV_FRIEND_DONATE_GOLD_NOTIFY);
	}
	
	return rc;
}

void CHallLogic::donateGoldToFriendReply(const com_protocol::RoundEndDataChargeRsp& addGoldRsp, const char* friendName, const unsigned int friendNameLen, const int indexKey)
{
	strInt_t idxKey = {0};
	FriendDonateGoldData* fdData = (FriendDonateGoldData*)m_dataContent.get(OperationType::FriendDonateGold, intValueToChars(indexKey, idxKey));
	if (fdData == NULL)
	{
		OptErrorLog("donate gold to friend reply, can not find the data, index = %d, friend = %s", indexKey, friendName);
		return;
	}

    if (addGoldRsp.result() == Success)
	{
		// 查找目标朋友用户所在的服务
		com_protocol::GetUserServiceIDReq getSrvIdReq;
		getSrvIdReq.add_username_lst(friendName);
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(getSrvIdReq, "get service id to donate gold to friend");
		if (msgBuffer != NULL && Success == m_msgHandler->sendMessageToService(ServiceType::MessageSrv, msgBuffer, getSrvIdReq.ByteSize(), BUSINESS_GET_USER_SERVICE_ID_REQ,
																			   friendName, friendNameLen, indexKey))
		{
			m_dataContent.set(OperationType::FriendDonateGold, idxKey, (char*)fdData);
			return;
		}
	}
	else
	{
		OptErrorLog("donate gold to friend reply error, result = %d, friend = %s", addGoldRsp.result(), friendName);
	}
	
	m_dataContent.putBuffer((char*)fdData);
}

void CHallLogic::donateGoldToFriendNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("donate gold to friend notify", userName);
	if (conn != NULL) m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_FRIEND_DONATE_GOLD_NOTIFY, conn);
}

void CHallLogic::getSrvIdForDonateGold(const com_protocol::GetUserServiceIDRsp& getUserSrvIdRsp, const int indexKey)
{
	const com_protocol::UserSerivceInfo& userSrvInfo = getUserSrvIdRsp.user_service_lst(0);
	if (userSrvInfo.login_service_id() == 0 && userSrvInfo.game_service_id() == 0)
	{
		if (!getUserHallLogicData(userSrvInfo.username().c_str(), indexKey))  // 获取非在线用户大厅逻辑数据
		{
			strInt_t idxKey = {0};
			m_dataContent.remove(OperationType::FriendDonateGold, intValueToChars(indexKey, idxKey));
		}
	}
	else
	{
		strInt_t idxKey = {0};
		FriendDonateGoldData* fdData = (FriendDonateGoldData*)m_dataContent.get(OperationType::FriendDonateGold, intValueToChars(indexKey, idxKey));
		if (fdData == NULL)
		{
			OptErrorLog("get friend service id for donate gold, can not find the data, index = %d, user = %s, friend = %s",
			indexKey, m_msgHandler->getContext().userData, userSrvInfo.username().c_str());
			return;
		}
	
		// 即时通知在线用户好友
		ConnectProxy* conn = NULL;
		fdData->notifySrvId = userSrvInfo.game_service_id() > 0 ? userSrvInfo.game_service_id() : userSrvInfo.login_service_id();
		if (fdData->notifySrvId == m_msgHandler->getSrvId())
		{
			// 目标朋友在本大厅服务
			conn = m_msgHandler->getConnectProxy(userSrvInfo.username().c_str());
			fdData->notifySrvId = 0;
		}

		sendDonateGoldToFriendNotify(fdData, conn);		 
		m_dataContent.putBuffer((char*)fdData);
	}
}

bool CHallLogic::setDonateGoldToOfflineUser(com_protocol::HallLogicData& hallLogicData, const int indexKey)
{
	strInt_t idxKey = {0};
	FriendDonateGoldData* fdData = (FriendDonateGoldData*)m_dataContent.get(OperationType::FriendDonateGold, intValueToChars(indexKey, idxKey));
	if (fdData == NULL)
	{
		OptErrorLog("get offline friend info to donate gold, can not find the data, index = %d, user = %s", indexKey, m_msgHandler->getContext().userData);
		return false;
	}
	
	com_protocol::FriendDonateGoldData* donateGold = hallLogicData.mutable_donate_gold();
	if (donateGold->donatelist_size() >= HallConfigData::config::getConfigValue().friend_donate_gold.show_count) donateGold->mutable_donatelist()->DeleteSubrange(0, 1);
	
	time_t curSecs = time(NULL);
	struct tm* tmval = localtime(&curSecs);
	com_protocol::DonateGold* donateInfo = donateGold->add_donatelist();
	donateInfo->set_username(fdData->userName);
	donateInfo->set_gold(fdData->gold);
	donateInfo->set_month(tmval->tm_mon + 1);
	donateInfo->set_day(tmval->tm_mday);
	
	m_dataContent.putBuffer((char*)fdData);
	return true;
}

void CHallLogic::getFriendDynamicData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::FriendData* friendData = NULL;
	const char* userName = NULL;
	unsigned int userNameLen = 0;
	unsigned int dstSrvId = 0;
	bool isProxyMsg = m_msgHandler->isProxyMsg();
	if (isProxyMsg)
	{
		if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get friend dynamic data")) return;
		
		ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
		userName = ud->connId;
		userNameLen = ud->connIdLen;
		friendData = m_msgHandler->getHallLogicData()->mutable_friends();
	}
	else
	{
		dstSrvId = srcSrvId;
		userName = m_msgHandler->getContext().userData;
	    userNameLen = m_msgHandler->getContext().userDataLen;
		com_protocol::HallLogicData* hallData = m_msgHandler->getHallLogicData(userName);
		if (hallData == NULL)
		{
			if (getUserHallLogicData(userName, HallDataFriendOpt::GetFriendDynamicData)) m_getFriendDynamicData[userName] = dstSrvId;
			return;
		}
		
		friendData = hallData->mutable_friends();
	}

	if (friendData->username_size() < 1)
	{
		com_protocol::ClientGetFriendDynamicDataRsp rsp;
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "get friend dynamic data");
		if (msgBuffer != NULL)
		{
			if (isProxyMsg) m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_GET_FRIEND_DYNAMIC_DATA_RSP);
			else m_msgHandler->sendMessage(msgBuffer, rsp.ByteSize(), srcSrvId, LOGINSRV_GET_FRIEND_DYNAMIC_DATA_RSP);
		}
		return;
	}
	
	m_getFriendDynamicData[userName] = dstSrvId;
	
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(*friendData, "get friend dynamic data");
	if (msgBuffer != NULL)
	{
	    m_msgHandler->sendMessageToService(ServiceType::MessageSrv, msgBuffer, friendData->ByteSize(), BUSINESS_GET_USER_SERVICE_ID_REQ, userName, userNameLen);
	}
}

void CHallLogic::getUserSrvIdReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUserServiceIDRsp getUserSrvIdRsp;
	if (!m_msgHandler->parseMsgFromBuffer(getUserSrvIdRsp, data, len, "get user service id reply")) return;
	
	const int userFlag = m_msgHandler->getContext().userFlag;
	if (userFlag == HallDataFriendOpt::GetDynamicDataForRemoveOther)
	{
	    return removeOtherFriend(getUserSrvIdRsp);
	}
	else if (userFlag >= HallDataFriendOpt::DonateGoldToFriend)
	{
		return getSrvIdForDonateGold(getUserSrvIdRsp, userFlag);
	}
	
	const string userName = m_msgHandler->getContext().userData;
	GetFriendDynamicDataContext::iterator it = m_getFriendDynamicData.find(userName);
    if (it == m_getFriendDynamicData.end())
	{
		OptErrorLog("can not find the operate record, user name = %s", m_msgHandler->getContext().userData);
		return;
	}
	
	unsigned int dstSrvId = it->second;
	m_getFriendDynamicData.erase(it);

	ConnectProxy* conn = NULL;
	if (dstSrvId == 0)
	{
		conn = m_msgHandler->getConnectProxy(userName);
		if (conn == NULL)
		{
			OptErrorLog("in getUserSrvIdReply can not find the connect data, user data = %s", m_msgHandler->getContext().userData);
			return;
		}
	}
	
	// OptWarnLog("friend data, user name = %s", userName.c_str());
	com_protocol::HallLogicData* hallData = m_msgHandler->getHallLogicData(userName.c_str());
	const com_protocol::FriendDonateGoldData* donateGoldData = (hallData != NULL) ? hallData->mutable_donate_gold() : NULL;
	const com_protocol::FriendData* friendData = (hallData != NULL) ? hallData->mutable_friends() : NULL;
	com_protocol::ClientGetFriendDynamicDataRsp getFriendDynamicDataRsp;
	for (int i = 0; i < getUserSrvIdRsp.user_service_lst_size(); ++i)
	{
		com_protocol::ClientFriendDynamicData* friendDynamicData = getFriendDynamicDataRsp.add_friends();
		const com_protocol::UserSerivceInfo& userSrvInfo = getUserSrvIdRsp.user_service_lst(i);
		friendDynamicData->set_username(userSrvInfo.username());
		if (userSrvInfo.game_service_id() != 0) friendDynamicData->set_status(FriendStatus::InGame);
		else if (userSrvInfo.login_service_id() != 0) friendDynamicData->set_status(FriendStatus::InHall);
		else friendDynamicData->set_status(FriendStatus::OffLine);
		
		int flag = 0;
		for (int j = 0; (donateGoldData != NULL) && j < donateGoldData->username_size(); ++j)
		{
			if (donateGoldData->username(j) == userSrvInfo.username())
			{
				flag = 1;
				break;
			}
		}
		friendDynamicData->set_flag(flag);
		
		// 是否是红包好友
		flag = 0;
		for (int idx = 0; (friendData != NULL) && idx < friendData->username_size(); ++idx)
		{
			if (friendData->username(idx) == userSrvInfo.username())
			{
				if (friendData->flag_size() > idx && friendData->flag(idx) == 1) flag = 1;
				break;
			}
		}
		friendDynamicData->set_red_gift_flag(flag);
		
		// OptWarnLog("friend dynamic data, count = %d, friend name = %s, status = %d, flag = %d", i, userSrvInfo.username().c_str(), friendDynamicData->status(), friendDynamicData->flag());
	}

    const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(getFriendDynamicDataRsp, "get user service id reply");
	if (msgBuffer != NULL)
	{
		if (dstSrvId == 0) m_msgHandler->sendMsgToProxy(msgBuffer, getFriendDynamicDataRsp.ByteSize(), LOGINSRV_GET_FRIEND_DYNAMIC_DATA_RSP, conn);
		else m_msgHandler->sendMessage(msgBuffer, getFriendDynamicDataRsp.ByteSize(), dstSrvId, LOGINSRV_GET_FRIEND_DYNAMIC_DATA_RSP);
	}
}

void CHallLogic::getFriendStaticData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get friend static data")) return;
	
	com_protocol::FriendData* friendData = m_msgHandler->getHallLogicData()->mutable_friends();
	if (friendData->username_size() < 1)
	{
		com_protocol::GetMultiUserInfoRsp rsp;
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "get friend static data");
		if (msgBuffer != NULL) m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_GET_FRIEND_STATIC_DATA_RSP);
		return;
	}
	
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(*friendData, "get friend static data");
	if (msgBuffer != NULL)
	{
		m_msgHandler->sendMessageToCommonDbProxy(msgBuffer, friendData->ByteSize(), BUSINESS_GET_MULTI_USER_INFO_REQ);
	}
}

void CHallLogic::transmitMsgToClient(const char* data, const unsigned int len, unsigned short protocolId, const char* info)
{
	const string userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL)
	{
		OptErrorLog("in %s can not find the connect data, user data = %s", info, m_msgHandler->getContext().userData);
		return;
	}
	
	m_msgHandler->sendMsgToProxy(data, len, protocolId, conn);
}

void CHallLogic::getFriendStaticDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (m_msgHandler->getContext().userFlag != HallDataFriendOpt::AddFriendNotifyClient)
	{
	    transmitMsgToClient(data, len, LOGINSRV_GET_FRIEND_STATIC_DATA_RSP, "getFriendStaticDataReply");
	}
	else
	{
		addFriendNotifyClient(data, len);
	}
}

void CHallLogic::getFriendStaticDataById(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_msgHandler->sendMessageToCommonDbProxy(data, len, BUSINESS_GET_MULTI_USER_INFO_REQ);
}

void CHallLogic::privateChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to do private chat")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->sendMessageToService(ServiceType::MessageSrv, data, len, BUSINESS_PRIVATE_CHAT_REQ, ud->connId, ud->connIdLen);
}

void CHallLogic::privateChatReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	transmitMsgToClient(data, len, LOGINSRV_PRIVATE_CHAT_RSP, "privateChatReply");
}

void CHallLogic::notifyPrivateChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	transmitMsgToClient(data, len, LOGINSRV_PRIVATE_CHAT_NOTIFY, "notifyPrivateChat");
}

void CHallLogic::changePropReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in changePropReply", userName);
	if (conn == NULL) return;
	
	com_protocol::PropChangeRsp propChangeRsp;
	if (!m_msgHandler->parseMsgFromBuffer(propChangeRsp, data, len, "get change prop reply")) return;

	if (propChangeRsp.result() == Success)
	{
		com_protocol::HallChangePropNotify notify;
		for (auto item = propChangeRsp.items().begin(); item != propChangeRsp.items().end(); ++item)
		{
			auto changeProp = notify.add_change_prop();
			changeProp->set_type(item->type());
			changeProp->set_num(item->count());
		}

		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(notify, "change prop reply");
		m_msgHandler->sendMsgToProxy(msgBuffer, notify.ByteSize(), LOGINSRV_CHANGE_PROP_NOTIFY, conn);

		switch (m_msgHandler->getContext().userFlag)
		{
			case 0:
			{
				ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
				com_protocol::WorldChatReq worldChatReq;
				worldChatReq.set_src_nickname(ud->nickname);
				worldChatReq.set_src_portrait_id(ud->portrait_id);
				worldChatReq.set_chat_content(propChangeRsp.dst_username());
				worldChatReq.set_chat_mode(1);

				const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(worldChatReq, "world chat");
				if (msgBuffer != NULL)
				{
					m_msgHandler->sendMessageToService(ServiceType::MessageSrv, msgBuffer, worldChatReq.ByteSize(), BUSINESS_WORLD_CHAT_REQ, ud->connId, ud->connIdLen);
				}
			}
			break;

			case EPropFlag::ShareReward:
			{
				ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
				if (ud != NULL)
				{
					auto nRewardCount = ud->hallLogicData->share_info().reward_count();
					ud->hallLogicData->mutable_share_info()->set_reward_count(++nRewardCount);
				}			
			}
			break;
			
			case EPropFlag::VIPUpLv:
			{
				ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
				if (ud->hallLogicData->vip_info().receive() == VIPStatus::VIPNoReward)
				{
				   ud->hallLogicData->mutable_vip_info()->set_receive(VIPStatus::VIPReceiveReward);
				   m_msgHandler->sendMsgToProxy(NULL, 0, LOGINSRV_FIND_VIP_REWARD_NOTIFY, conn);
				}

				//通知VIP等级变化			
				com_protocol::PersonPropertyNotify notify;
				auto property = notify.add_property();
				property->set_attribute_id(EClientPropNumType::enPropNumType_vip);
				property->set_change_count(propChangeRsp.items(0).count());
				const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(notify, "vip lv update");
				m_msgHandler->sendMsgToProxy(msgBuffer, notify.ByteSize(), LOGINSRV_PERSON_PROPERTY_NOTIFY, conn);
			}
			break;

			case EPropFlag::PKPlayRankingListReward:
			{
				ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
				if (ud != NULL)	ud->hallLogicData->mutable_ranking_info()->set_receive_reward(EPKPlayRankingRewardStatus::EAlreadyAward);
			}
			break;
			
			// PK场任务奖励
			case EPropFlag::JoinMatchReward:
			case EPropFlag::WinMatchReward:
			case EPropFlag::OnlineMatchReward:
			{
				m_msgHandler->getActivityCenter().receivePKTaskRewardReply(m_msgHandler->getContext().userFlag);
				break;
			}

			default:
			    break;
		}
	}
	else
	{
		OptErrorLog("CHallLogic changePropReply, change prop reply fail, code result:%d, user flag:%d,", propChangeRsp.result(), m_msgHandler->getContext().userFlag);

		if (m_msgHandler->getContext().userFlag == 0)
		{
			com_protocol::ClientWorldChatRsp wcatRsp;
			wcatRsp.set_result(propChangeRsp.result());
			const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(wcatRsp, "world chat change prop failed");
			if (msgBuffer != NULL)
			{
				m_msgHandler->sendMsgToProxy(msgBuffer, wcatRsp.ByteSize(), LOGINSRV_WORLD_CHAT_RSP, conn);
			}
		}
	}
	
	// 领取了红包奖励处理
	if (m_msgHandler->getContext().userFlag == EPropFlag::RedGiftReward) m_msgHandler->getLogicHanderTwo().receiveRedGiftRewardReply(propChangeRsp.result(), conn);
}

void CHallLogic::getUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUserOtherInfoRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "get User Info")) return;

	if (rsp.result() != 0)
	{
		OptErrorLog("CHallLogic getUserInfo, get user info error, result:%d", rsp.result());
		return;
	}

	string userName = m_msgHandler->getContext().userData;

	switch (m_msgHandler->getContext().userFlag)
	{
	case GetUserInfoFlag::GetUserVIPInfo:
		{
			int nVIPLv = rsp.other_info(0).int_value();
			int nRechargeAmount = rsp.other_info(1).int_value();
			//OptInfoLog("CHallLogic getUserInfo, vip lv:%d, Recharge Amount:%d, original Amount:%d", nVIPLv, nRechargeAmount, rsp.other_info(1).int_value());
			noticeUserVIPInfo(userName, nVIPLv, nRechargeAmount);
		}
		break;

		case GetUserInfoFlag::GetUserVIPLvForReward:
		{
			const HallConfigData::config& vipCfg = HallConfigData::config::getConfigValue();
			int nVIPLv = rsp.other_info(0).int_value();

			//非VIP用户
			if (nVIPLv == 0)
			{
				com_protocol::ClientGetVIPRewardRsp rsp;
				rsp.set_result(ProtocolReturnCode::NoVIPUser);
				const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "get vip reward result");
				m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_GET_VIP_REWARD_RSP, m_msgHandler->getConnectProxy(userName));
				return;
			}

			com_protocol::HallLogicData* hallData = m_msgHandler->getHallLogicData(userName.c_str());
			if (hallData == NULL)
			{
				OptErrorLog("CHallLogic getUserInfo, hallData == NULL, user name:%s", userName.c_str());
				return;
			}
			
			//已领取
			if ((VIPStatus)hallData->vip_info().receive() == VIPStatus::VIPAlreadyGeted)
			{
				com_protocol::ClientGetVIPRewardRsp rsp;
				rsp.set_result(ProtocolReturnCode::AlreadyGetedVIPReward);
				const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "get vip reward result");
				m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_GET_VIP_REWARD_RSP, m_msgHandler->getConnectProxy(userName));
				return;
			}

			//领取VIP奖励
			receiveVIPReward(userName, nVIPLv, vipCfg, hallData);
		}
			break;

		case GetUserInfoFlag::GetUserVIPLvForVIPUp:
		{
			uint32_t nVIPLv = rsp.other_info(0).int_value();
			uint32_t nRechargeAmount = rsp.other_info(1).int_value();
			uint32_t curLV = getVIPLv(nRechargeAmount);

			if (curLV > nVIPLv)
			{
				toDBUpdateVIPLv(userName.c_str(), curLV - nVIPLv, EPropFlag::VIPUpLv);
			}
		}
		break;

		case GetUserInfoFlag::GetUserAllPropToPKPlayInfoReq:
			m_msgHandler->getPkLogic().handleClientGetPkPlayInfoRsp(rsp, userName);
			break;

		case GetUserInfoFlag::GetUserInfoToPKPlaySignUp:
			//m_msgHandler->getPkLogic().handleClientSignUpRsp(rsp, userName);
			break;

		default:
		break;
	}	
}

void CHallLogic::getManyUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	
	com_protocol::GetManyUserOtherInfoRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "get many user info")) return;

	switch (m_msgHandler->getContext().userFlag)
	{
	case GetManyUserInfoFlag::GetManyUserInfoStartPkPlay:
		m_msgHandler->getPkLogic().onStartPlay(rsp);
		break;

	default:
		break;
	}
}

void CHallLogic::noticeUserVIPInfo(string &userName, uint32_t nVIPLv, uint32_t nRechargeAmount)
{
	const HallConfigData::config& vipCfg = HallConfigData::config::getConfigValue();
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL)
	{
		OptErrorLog("CHallLogic noticeUserVIPInfo, conn == NULL, user name:%s", userName.c_str());
		return;
	}

	com_protocol::HallLogicData* hallData = m_msgHandler->getHallLogicData(userName.c_str());
	if (hallData == NULL)
	{
		OptErrorLog("CHallLogic noticeUserVIPInfo, hallData == NULL, userName:%s", userName.c_str());
		return;
	}

	auto curLv = getVIPLv(nRechargeAmount);

	if (curLv != nVIPLv)
	{
		/*
		if (curLv < nVIPLv)
		{
			OptErrorLog("CHallLogic noticeUserVIPInfo, vip cur lv:%d, Recharge lv:%d", nVIPLv, curLv);
			return;
		}
		*/
		
		toDBUpdateVIPLv(userName.c_str(), curLv - nVIPLv, EPropFlag::VIPUpLv);
		nVIPLv = curLv;
	}

	//OptInfoLog("CHallLogic noticeUserVIPInfo, nVIPLv:%d, receive:%d", nVIPLv, hallData->vip_info().receive());

	const char* msgBuffer = NULL;
	if (nVIPLv == 0 || hallData->vip_info().receive() == VIPStatus::VIPAlreadyGeted)
	{
		com_protocol::ClientVIPInfoRsp rsp;
		rsp.set_rmb(nRechargeAmount);
		setVIPCfgInfo(rsp);
		msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "");
		m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_VIP_INFO_RSP, conn);
	}
	else
	{
		auto cfg = vipCfg.vip_info_list.find(nVIPLv);
		if (cfg == vipCfg.vip_info_list.end())
		{
			OptErrorLog("CHallLogic get VIP Cfg Error! VIP Lv:%d", nVIPLv);
			return ;
		}
		
		com_protocol::ClientGetUserVIPInfoRsp rsp;
		
		for (auto it = cfg->second.reward.begin(); it != cfg->second.reward.end(); ++it)
		{
			if (it->first == EPropType::PropGold)
			{
				if (cfg->second.reward_day > hallData->vip_info().receive_godl_num())
				{
					auto reward = rsp.add_reward_list();
					reward->set_num(it->second);
					reward->set_type(it->first);
				}
			}
			else
			{
				auto reward = rsp.add_reward_list();
				reward->set_num(it->second);
				reward->set_type(it->first);
			}
		}
		
		rsp.set_bankruptcy_subsidy(cfg->second.subsidy);		
		msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "");
		m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_GET_USER_VIP_INFO_RSP, conn);
	}	
}

void CHallLogic::setVIPCfgInfo(com_protocol::ClientVIPInfoRsp &rsp)
{
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();

	for (auto vipIt = cfg.vip_info_list.begin(); vipIt != cfg.vip_info_list.end(); ++vipIt)
	{
		com_protocol::VipInfo *pItem = rsp.add_vip_info();
		pItem->set_lv(vipIt->first);
		pItem->set_rmb(vipIt->second.condition);
		pItem->set_bankruptcy_subsidy(vipIt->second.subsidy);
		pItem->set_gold_receive_num(vipIt->second.reward_day);
	
		for (auto rewardCfg = vipIt->second.reward.begin(); rewardCfg != vipIt->second.reward.end(); ++rewardCfg)
		{
			auto reward = pItem->add_reward_list();
			reward->set_type(rewardCfg->first);
			reward->set_num(rewardCfg->second);
		}
	}	
}

void CHallLogic::receiveVIPReward(const string &userName, uint32_t nVIPLv, const HallConfigData::config& vipCfg, com_protocol::HallLogicData* hallData)
{
	auto cfg = vipCfg.vip_info_list.find(nVIPLv);
	if (cfg == vipCfg.vip_info_list.end())
	{
		OptErrorLog("CHallLogic getUserInfo, read config error! user vip lv:%d,", nVIPLv);
		return;
	}


	NProject::RecordItemVector recordItemVector;

	com_protocol::ClientGetVIPRewardRsp rsp;
	rsp.set_result(ProtocolReturnCode::Opt_Success);
	bool bReward = false;
	for (auto cfgIt = cfg->second.reward.begin(); cfgIt != cfg->second.reward.end(); ++cfgIt)
	{
		bReward = false;
		if (cfgIt->first == EPropType::PropGold)
		{
			//控制领取金币的天数
			if (cfg->second.reward_day > hallData->vip_info().receive_godl_num())
			{
				int n = hallData->vip_info().receive_godl_num();
				hallData->mutable_vip_info()->set_receive_godl_num(++n);
				bReward = true;
			}
		}
		else
		{
			bReward = true;		//非金币可以领取奖励
		}

		if (bReward)
		{
			auto addReward = rsp.add_reward_list();
			addReward->set_type(cfgIt->first);
			addReward->set_num(cfgIt->second);

			NProject::RecordItem addItem(cfgIt->first, cfgIt->second);
			recordItemVector.push_back(addItem);
		}
	}	
	
	m_msgHandler->notifyDbProxyChangeProp(userName.c_str(), (unsigned int)userName.length(), recordItemVector, EPropFlag::VIPPropReward, "VIP奖励", NULL);

	//增加破产补助次数
	hallData->mutable_vip_info()->set_subsidies_num_max(cfg->second.subsidy);
	hallData->mutable_vip_info()->set_subsidies_num_cur(VIP_ADDITIONAL_NUM);

	//领取状态
	hallData->mutable_vip_info()->set_receive(VIPStatus::VIPAlreadyGeted);

	//设置哪天领取的
	time_t curSecs = time(NULL);
	hallData->mutable_vip_info()->set_today(localtime(&curSecs)->tm_yday);

	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "get vip reward result");
	m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_GET_VIP_REWARD_RSP, m_msgHandler->getConnectProxy(userName));
}


uint32_t CHallLogic::getVIPLv(uint32_t rechargeMoney)
{
	uint32_t vipLv = 0;
	const HallConfigData::config& vipCfg = HallConfigData::config::getConfigValue();

	for (auto cfg = vipCfg.vip_info_list.begin(); cfg != vipCfg.vip_info_list.end(); ++cfg)
	{
		if (rechargeMoney < cfg->second.condition)
		{
			break;
		}
		else if (rechargeMoney == cfg->second.condition)
		{
			vipLv = cfg->first;
			break;
		}

		vipLv = cfg->first;
	}
		
	return vipLv;
}

void CHallLogic::worldChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to do world chat")) return;
	
	com_protocol::ClientWorldChatReq clientWorldChatReq;;
	if (!m_msgHandler->parseMsgFromBuffer(clientWorldChatReq, data, len, "get user service id reply")) return;

    // 扣除聊天消耗的道具
	com_protocol::PropChangeReq propChangeReq;
	com_protocol::ItemChangeInfo* itemChange = propChangeReq.add_items();
	itemChange->set_type(EPropType::PropSuona);
	itemChange->set_count(-1);
	propChangeReq.set_dst_username(clientWorldChatReq.chat_content());
	com_protocol::GameRecordPkg* gRecord = propChangeReq.mutable_game_record();
	gRecord->set_game_type(GameRecordType::Game);
    
	com_protocol::BuyuGameRecordStorage recordStore;
	recordStore.set_room_rate(0);
	recordStore.set_room_name("大厅");
	recordStore.set_item_type(EPropType::PropSuona);
	recordStore.set_charge_count(-1);
	recordStore.set_remark("使用喇叭广播消息");
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(recordStore, "world chat record");
	if (msgBuffer != NULL) gRecord->set_game_record_bin(msgBuffer, recordStore.ByteSize());
	
	msgBuffer = m_msgHandler->serializeMsgToBuffer(propChangeReq, "decrease the world chat item");
	if (msgBuffer != NULL)
	{
		m_msgHandler->sendMessageToCommonDbProxy(msgBuffer, propChangeReq.ByteSize(), BUSINESS_CHANGE_PROP_REQ);
	}
}

void CHallLogic::worldChatReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	transmitMsgToClient(data, len, LOGINSRV_WORLD_CHAT_RSP, "worldChatReply");
}

void CHallLogic::notifyWorldChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const IDToConnectProxys& userConnects = m_msgHandler->getConnectProxy();
	for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end(); ++it)
	{
		m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_WORLD_CHAT_NOTIFY, it->second);
	}
}

// 系统消息通知
void CHallLogic::systemMessageNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 系统消息
	com_protocol::SystemMessageNotify sysNotify;
	if (!m_msgHandler->parseMsgFromBuffer(sysNotify, data, len, "system message notify")) return;
	
	if (sysNotify.type() == ESystemMessageType::Group)
	{
		const IDToConnectProxys& userConnects = m_msgHandler->getConnectProxy();
		for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end(); ++it)
		{
			m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_SYSTEM_MESSAGE_NOTIFY, it->second);
		}
	}
	else
	{
		string userName;
		ConnectProxy* conn = getConnectInfo("in systemMessageNotify", userName);
		if (conn != NULL) m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_SYSTEM_MESSAGE_NOTIFY, conn);
	}
}

void CHallLogic::getMallConfig(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get mall config data")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->getRechargeInstance().getMallData(data, len, ud->connId, ud->connIdLen);
}

void CHallLogic::getMallConfigReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in getMallConfigReply", userName);
	if (conn == NULL) return;
	
	m_msgHandler->getRechargeInstance().setMallData(data, len);
	
	m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_GET_MALL_CONFIG_RSP, conn);
}

int CHallLogic::sendMessageToHttpService(const char* data, const unsigned int len, unsigned short protocolId)
{
	// 要根据负载均衡选择http服务
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	unsigned int srvId = 0;
	getDestServiceID(HttpSrv, ud->connId, ud->connIdLen, srvId);
	return m_msgHandler->sendMessage(data, len, ud->connId, ud->connIdLen, srvId, protocolId);
}

void CHallLogic::checkDownJoyUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::CheckDownJoyUserReq checkDownJoyUserReq;
	if (!m_msgHandler->parseMsgFromBuffer(checkDownJoyUserReq, data, len, "check down joy user")) return;
	
	// 先初始化连接
	if (m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn) == NULL) m_msgHandler->initConnect(m_msgHandler->getConnectProxyContext().conn);
	
	int rc = sendMessageToHttpService(data, len, CHECK_DOWNJOY_USER_REQ);
	if (rc == Success)
	{
		ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	    m_connId2DownJoyInfo[string(ud->connId, ud->connIdLen)] = DownJoyInfo(checkDownJoyUserReq.nickname(), checkDownJoyUserReq.umid(), checkDownJoyUserReq.ostype());
	}

	OptInfoLog("receive message to check down joy user, rc = %d", rc);
}

void CHallLogic::checkDownJoyUserReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const string userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL)
	{
		OptErrorLog("in checkDownJoyUserReply can not find the connect data, user data = %s", m_msgHandler->getContext().userData);
		return;
	}
	
	ConnectId2DownJoyInfo::iterator it = m_connId2DownJoyInfo.find(userName);
	if (it == m_connId2DownJoyInfo.end())
	{
		OptErrorLog("can not find the check down joy user record, user data = %s", m_msgHandler->getContext().userData);
		return;
	}

	com_protocol::CheckDownJoyUserRsp checkDownJoyUserRsp;
	if (m_msgHandler->parseMsgFromBuffer(checkDownJoyUserRsp, data, len, "check down joy user reply"))
	{
		OptInfoLog("receive http check down joy user reply message, result = %d, valid = %d", checkDownJoyUserRsp.result(), checkDownJoyUserRsp.valid());
		if (checkDownJoyUserRsp.result() == DownJoyCode::DownJoySuccess && checkDownJoyUserRsp.valid() != DownJoyCode::DownJoyInvalid)
		{
			com_protocol::GetUsernameByPlatformReq getUserNameReq;
			getUserNameReq.set_platform_id(it->second.umid);
			getUserNameReq.set_platform_type(UserAccountNumber::DownJoyPlatform);
			getUserNameReq.set_os_type(it->second.osType);
			getUserNameReq.set_nickname(it->second.name);
			getUserNameReq.set_client_ip(CSocket::toIPStr(conn->peerIp));
			const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(getUserNameReq, "get user name by down joy platform");
			if (msgBuffer != NULL && m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgBuffer, getUserNameReq.ByteSize(), BUSINESS_GET_USERNAME_BY_PLATFROM_REQ,
												                        userName.c_str(), userName.length(), UserAccountNumber::DownJoyPlatform) == Success)
			{
				m_connId2DownJoyCheck[userName] = DownJoyCheck(checkDownJoyUserRsp.result(), checkDownJoyUserRsp.valid());
			}
		}
		else
		{
			m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_CHECK_DOWNJOY_USER_RSP, conn);
		}
	}
	
	m_connId2DownJoyInfo.erase(it);
}

void CHallLogic::checkFacebookUser(const char* data, const unsigned int len, const com_protocol::GetUsernameByPlatformReq& req)
{
	int rc = Opt_Failed;
	if (req.has_user_token()) rc = sendMessageToHttpService(data, len, CHECK_FACEBOOK_USER_REQ);
	OptInfoLog("receive message to check facebook user, id = %s, token = %s, rc = %d", req.platform_id().c_str(), req.user_token().c_str(), rc);
}

void CHallLogic::checkFacebookUserReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const string userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL)
	{
		OptErrorLog("in checkFacebookUserReply can not find the connect data, user data = %s", m_msgHandler->getContext().userData);
		return;
	}

	com_protocol::CheckFacebookUserNotify notify;
	if (m_msgHandler->parseMsgFromBuffer(notify, data, len, "check facebook user reply"))
	{
		OptInfoLog("receive http check facebook user reply message, result = %d", notify.result());
		if (notify.result() == 0)
		{
			notify.mutable_user_info()->set_client_ip(CSocket::toIPStr(conn->peerIp));
			const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(notify.user_info(), "get user name by facebook platform");
			if (msgBuffer != NULL)
			{
				m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgBuffer, notify.user_info().ByteSize(), BUSINESS_GET_USERNAME_BY_PLATFROM_REQ,
				userName.c_str(), userName.length());
			}
		}
		else
		{
			com_protocol::GetUsernameByPlatformRsp getUserNameRsp;
			getUserNameRsp.set_result(CheckFacebookUserError);
			const char* msgData = m_msgHandler->serializeMsgToBuffer(getUserNameRsp, "check facebook user error reply");
			if (msgData != NULL) m_msgHandler->sendMsgToProxy(msgData, getUserNameRsp.ByteSize(), LOGINSRV_GET_USERNAME_BY_PLATFROM_RSP, conn);
		}
	}
}

void CHallLogic::getUserNameByPlatform(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUsernameByPlatformReq getUsernameByPlatformReq;
	if (!m_msgHandler->parseMsgFromBuffer(getUsernameByPlatformReq, data, len, "get user name by platform type")) return;

    getUsernameByPlatformReq.set_client_ip(CSocket::toIPStr(m_msgHandler->getConnectProxyContext().conn->peerIp));  // 填写IP地址区分地区
    const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(getUsernameByPlatformReq, "get user name by platform type");
	if (msgBuffer == NULL) return;
	
	if (m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn) == NULL) m_msgHandler->initConnect(m_msgHandler->getConnectProxyContext().conn);  // 先初始化连接

	if (getUsernameByPlatformReq.platform_type() != ThirdPlatformType::HanYouFacebook)
	{
		ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	    m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgBuffer, getUsernameByPlatformReq.ByteSize(), BUSINESS_GET_USERNAME_BY_PLATFROM_REQ, ud->connId, ud->connIdLen);
	}
	else
	{
		checkFacebookUser(data, len, getUsernameByPlatformReq);
	}
}

void CHallLogic::getUserNameByPlatformReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const string userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL)
	{
		OptErrorLog("in getUserNameByPlatformReply can not find the connect data, user data = %s", m_msgHandler->getContext().userData);
		return;
	}
	
	com_protocol::GetUsernameByPlatformRsp getUserNameRsp;
	if (!m_msgHandler->parseMsgFromBuffer(getUserNameRsp, data, len, "get user name by platform type reply"))
	{
		m_connId2DownJoyCheck.erase(userName);
		return;
	}
	
	int rc = -1;
	if (m_msgHandler->getContext().userFlag == 0)
	{
		rc = m_msgHandler->sendMsgToProxy(data, len, LOGINSRV_GET_USERNAME_BY_PLATFROM_RSP, conn);
		OptInfoLog("send user info by platform type to client, name = %s, result = %d, rc = %d", getUserNameRsp.username().c_str(), getUserNameRsp.result(), rc);
	}
	
	// 当乐平台处理
	else if (m_msgHandler->getContext().userFlag == UserAccountNumber::DownJoyPlatform)
	{
		ConnectId2DownJoyCheck::iterator it = m_connId2DownJoyCheck.find(userName);
		if (it == m_connId2DownJoyCheck.end())
		{
			OptErrorLog("can not find the get down joy user record, user data = %s", m_msgHandler->getContext().userData);
			return;
		}
		
		com_protocol::CheckDownJoyUserRsp checkDownJoyUserRsp;
		checkDownJoyUserRsp.set_result(it->second.result);
		checkDownJoyUserRsp.set_valid(it->second.valid);
		if (getUserNameRsp.result() == Success) checkDownJoyUserRsp.set_innerusername(getUserNameRsp.username());
		else checkDownJoyUserRsp.set_result(getUserNameRsp.result());
		
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(checkDownJoyUserRsp, "send down joy user info to client");
		if (msgBuffer != NULL)
		{
			rc = m_msgHandler->sendMsgToProxy(msgBuffer, checkDownJoyUserRsp.ByteSize(), LOGINSRV_CHECK_DOWNJOY_USER_RSP, conn);
			OptInfoLog("send down joy user info to client, rc = %d, result = %d, valid = %d, name = %s",
			rc, checkDownJoyUserRsp.result(), checkDownJoyUserRsp.valid(), checkDownJoyUserRsp.has_innerusername() ? checkDownJoyUserRsp.innerusername().c_str() : "");
		}
				
		m_connId2DownJoyCheck.erase(it);
	}
}

ConnectProxy* CHallLogic::getConnectInfo(const char* logInfo, string& userName)
{
	userName = m_msgHandler->getContext().userData;
	ConnectProxy* conn = m_msgHandler->getConnectProxy(userName);
	if (conn == NULL) OptErrorLog("%s, user data = %s", logInfo, m_msgHandler->getContext().userData);
    return conn;
}

void CHallLogic::getDownJoyRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get down joy recharge transaction")) return;
	
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->getRechargeInstance().getDownJoyRechargeTransaction(data, len, ud->connId, ud->connIdLen, LOGINSRV_GET_DOWNJOY_RECHARGE_TRANSACTION_RSP, m_msgHandler->getConnectProxyContext().conn);
}

void CHallLogic::getDownJoyRechargeTransactionReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in getDownJoyRechargeTransactionReply can not find the connect data", userName);
	if (conn == NULL) return;
	
	m_msgHandler->getRechargeInstance().getDownJoyRechargeTransactionReply(data, len, LOGINSRV_GET_DOWNJOY_RECHARGE_TRANSACTION_RSP, conn);
}

void CHallLogic::downJoyRechargeTransactionNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
ConnectProxy* conn = getConnectInfo("in downJoyRechargeTransactionNotify can not find the connect data", userName);
if (conn == NULL) return;

m_msgHandler->getRechargeInstance().downJoyRechargeTransactionNotify(data, len, LOGINSRV_DOWNJOY_RECHARGE_TRANSACTION_NOTIFY, conn);
}

void CHallLogic::cancelDownJoyRechargeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to cancel down joy recharge notify")) return;

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	m_msgHandler->getRechargeInstance().cancelDownJoyRechargeNotify(data, len, ud->connId, ud->connIdLen);
}


void CHallLogic::onLine(ConnectUserData* ud, ConnectProxy* conn)
{
	com_protocol::HallLogicData* hallLogicData = ud->hallLogicData;
	time_t curSecs = time(NULL);
	int theDay = localtime(&curSecs)->tm_yday;

	com_protocol::LoginReward* loginReward = hallLogicData->mutable_login_reward();
	if (loginReward->day() != theDay)
	{
		unsigned int rwCount = loginReward->status_size();
		if (rwCount > 0)
		{
			// 非连续登陆，非新年第一天，非之前的奖励已经领取
			if ((((theDay - loginReward->day()) != 1) && theDay != 0)
				|| (loginReward->status(rwCount - 1) != LoginRewardStatus::AlreadyGeted)
				|| (rwCount >= HallConfigData::config::getConfigValue().login_reward_list.size()))  // 超过连续登陆的天数了
			{
				loginReward->clear_status();
			}

		}

		loginReward->set_day(theDay);
		loginReward->add_status(LoginRewardStatus::CanGetReward);
	}
	
	//设置VIP状态
	if (!hallLogicData->has_vip_info())
	{
		auto pVIPInfo = hallLogicData->mutable_vip_info();
		pVIPInfo->set_receive(VIPStatus::VIPNoReward);
		pVIPInfo->set_today(0);
		pVIPInfo->set_subsidies_num_max(VIP_ADDITIONAL_NUM);
		pVIPInfo->set_subsidies_num_cur(VIP_ADDITIONAL_NUM);
	}
	else
	{
		//隔天清空VIP状态
		com_protocol::VIPInfo* pVIPInfo = hallLogicData->mutable_vip_info();
		if (pVIPInfo->today() != (uint32_t)(theDay) && pVIPInfo->receive() != VIPStatus::VIPNoReward)
		{
			pVIPInfo->set_today(theDay);
			pVIPInfo->set_receive(VIPStatus::VIPReceiveReward);
			pVIPInfo->set_subsidies_num_max(VIP_ADDITIONAL_NUM);
			pVIPInfo->set_subsidies_num_cur(VIP_ADDITIONAL_NUM);
		}
	}	

	//通知客户端可以领取VIP奖励
	if (hallLogicData->vip_info().receive() == VIPStatus::VIPReceiveReward)
	{
		m_msgHandler->sendMsgToProxy(NULL, 0, LOGINSRV_FIND_VIP_REWARD_NOTIFY, conn);
	}
		
	com_protocol::FriendDonateGoldData* friendDonateGold = hallLogicData->mutable_donate_gold();
	if (friendDonateGold->today() != (unsigned int)theDay)
	{
		friendDonateGold->set_today(theDay);
		friendDonateGold->clear_username();
	}

	if (friendDonateGold->donatelist_size() > 0)
	{
		com_protocol::ClientFriendDonateGoldNotify donateGoldNotify;
		for (int i = 0; i < friendDonateGold->donatelist_size(); ++i)
		{
			com_protocol::DonateGold* dg = donateGoldNotify.add_donatelist();
			*dg = friendDonateGold->donatelist(i);
		}
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(donateGoldNotify, "send friend donate gold data notify to client");
		if (msgBuffer != NULL) m_msgHandler->sendMsgToProxy(msgBuffer, donateGoldNotify.ByteSize(), LOGINSRV_FRIEND_DONATE_GOLD_NOTIFY, conn);

		friendDonateGold->clear_donatelist();
	}

	
	setUserNoticeInfo(hallLogicData, theDay, conn);		//设置用户公告信息 
	setUserShareInfo(hallLogicData, theDay);			//设置用户分享信息
	setUserScoreInfo(hallLogicData, conn);				//设置用户积分信息

	m_hallLogicHandler.onLine(ud, conn);
}

void CHallLogic::offLine(com_protocol::HallLogicData* hallLogicData)
{
	m_hallLogicHandler.offLine(hallLogicData);
}

void CHallLogic::updateConfig()
{
	openClearScoresTimer();
	
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();

	if (m_noticeVersion != cfg.hall_notice.notice_version)
	{
		time_t curSecs = time(NULL);
		int theDay = localtime(&curSecs)->tm_yday;
		m_noticeVersion = cfg.hall_notice.notice_version;

		const IDToConnectProxys& userConnects = m_msgHandler->getConnectProxy();
		for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end(); ++it)
		{
			com_protocol::HallLogicData* hallLogicData = m_msgHandler->getHallLogicData(it->first.c_str());
			if (hallLogicData != NULL)
			{
				setUserNoticeInfo(hallLogicData, theDay, it->second);
			}
		}
	}
}

void CHallLogic::getUserVIPInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get user vip info")) return;

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);

	//获取用户当前VIP等级
	com_protocol::GetUserOtherInfoReq db_req;
	char send_data[MaxMsgLen] = { 0 };
	db_req.set_query_username(ud->connId);
	db_req.add_info_flag(EUserInfoFlag::EVipLevel);
	db_req.add_info_flag(EUserInfoFlag::ERechargeAmount);
	db_req.SerializeToArray(send_data, MaxMsgLen);
	m_msgHandler->sendMessageToService(NFrame::ServiceType::DBProxyCommonSrv, send_data, db_req.ByteSize(), BUSINESS_GET_USER_OTHER_INFO_REQ, ud->connId, ud->connIdLen, GetUserInfoFlag::GetUserVIPInfo);
}

void CHallLogic::getVIPReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{

	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get vip reward list"))	return;
	
	if (!m_msgHandler->getHallLogicData()->has_vip_info())
	{
		OptErrorLog("CHallLogic getVIPReward, vip info empty!");
		return;
	}

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);

	//已领取
	if ((VIPStatus)m_msgHandler->getHallLogicData()->vip_info().receive() == VIPStatus::VIPAlreadyGeted)
	{
		com_protocol::ClientGetVIPRewardRsp rsp;
		rsp.set_result(ProtocolReturnCode::AlreadyGetedVIPReward);
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "get vip reward result");
		m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_GET_VIP_REWARD_RSP, m_msgHandler->getConnectProxyContext().conn);
		return;
	}

	//查询用户VIP等级
	com_protocol::GetUserOtherInfoReq db_req;
	char send_data[MaxMsgLen] = { 0 };
	db_req.set_query_username(ud->connId);
	db_req.add_info_flag(EUserInfoFlag::EVipLevel);
	db_req.SerializeToArray(send_data, MaxMsgLen);
	m_msgHandler->sendMessageToService(NFrame::ServiceType::DBProxyCommonSrv, send_data, db_req.ByteSize(), BUSINESS_GET_USER_OTHER_INFO_REQ, ud->connId, ud->connIdLen, GetUserInfoFlag::GetUserVIPLvForReward);
}

void CHallLogic::toDBUpdateVIPLv(const char *userName, int addVIPLv, int userFlag)
{
	NProject::RecordItemVector recordItemVector;
	RecordItem item(EUserInfoFlag::EVipLevel, addVIPLv);
	recordItemVector.push_back(item);
	m_msgHandler->notifyDbProxyChangeProp(userName, strlen(userName), recordItemVector, userFlag, "VIP等级变更", NULL);
}


void CHallLogic::getNoticeReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get notice req"))	return;
	time_t curSecs = time(NULL);
	int theDay = localtime(&curSecs)->tm_yday;
	sendNoticeHint(theDay, m_msgHandler->getConnectProxyContext().conn, m_msgHandler->getHallLogicData());
}

void CHallLogic::setUserNoticeInfo(com_protocol::HallLogicData* hallLogicData, int theDay, ConnectProxy* conn)
{
	if (hallLogicData == NULL)
	{
		OptErrorLog("CHallLogic setUserNoticeInfo, hallLogicData == NULL");
		return;
	}

	if(!hallLogicData->has_notice_info())
	{
		auto noticeInfo = hallLogicData->mutable_notice_info();
		noticeInfo->set_read_flag(ReadNoticeFlag::notRead);
		noticeInfo->set_version(0);
		noticeInfo->set_today(theDay);
	}
	else
	{
		//隔天清空公告阅读状态
		/*if (hallLogicData->notice_info().today() != (uint32_t)(theDay))
		{
			hallLogicData->mutable_notice_info()->set_read_flag(ReadNoticeFlag::notRead);
		}*/
	}

	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();

	//如果公告版本号有更新
	if (hallLogicData->notice_info().version() != cfg.hall_notice.notice_version)
	{
		hallLogicData->mutable_notice_info()->set_version(cfg.hall_notice.notice_version);
		hallLogicData->mutable_notice_info()->set_read_flag(ReadNoticeFlag::notRead);
	}

	//如果用户未阅读, 且为主动弹出公告
	if (hallLogicData->notice_info().read_flag() == ReadNoticeFlag::notRead && cfg.hall_notice.eject == NoticeShowType::eject)
	{
		sendNoticeHint(theDay, conn, hallLogicData);
	}

	//如果用户未阅读，不主动弹出则提示用户有公告
	if (hallLogicData->notice_info().read_flag() == ReadNoticeFlag::notRead && cfg.hall_notice.eject == NoticeShowType::notEject)
	{
		m_msgHandler->sendMsgToProxy(NULL, 0, LOGINSRV_NEW_NOTICE_NOTIFY, conn);
	}
}

void CHallLogic::sendNoticeHint(int theDay, ConnectProxy* conn, com_protocol::HallLogicData* hallLogicData)
{
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	com_protocol::ClientHallNoticeRsp rsp;

	for (auto noticeItem = cfg.hall_notice.notice_list.begin(); noticeItem != cfg.hall_notice.notice_list.end(); ++noticeItem)
	{
		auto notice = rsp.add_notice_list();
		notice->set_title(noticeItem->title);
		notice->set_content(noticeItem->content);
		notice->set_picture_path(noticeItem->picture_path);
	}

	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "get notice req");
	if (msgBuffer != NULL)
	{	
		//设置为已读
		hallLogicData->mutable_notice_info()->set_read_flag(ReadNoticeFlag::read);
		hallLogicData->mutable_notice_info()->set_today(theDay);

		m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_NOTICE_RSP, conn);	
	}
}

void CHallLogic::handleShareReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to handle Share Req")) return;

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	if (ud == NULL)
	{
		OptErrorLog("get connect user data, ud == null");
		return;
	}

	com_protocol::ClientHallShareRsp rsp;
	getShareReward(ud->connId, rsp);
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "handle share req");
	if (msgBuffer != NULL)
		m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_SHARE_RSP, m_msgHandler->getConnectProxyContext().conn);
}

void CHallLogic::getShareReward(const char *pUserName, com_protocol::ClientHallShareRsp &rsp)
{
	rsp.set_result(Opt_Success);

    auto userData = m_msgHandler->getConnectUserData(pUserName);
	if (pUserName == NULL || userData == NULL)
	{
		OptErrorLog("CHallLogic getShareReward, pUserName = %p, userData = %p", pUserName, userData);
		rsp.set_result(NotFindUserInfo);
		rsp.set_share_title("");
		rsp.set_share_content("");
		return;
	}

	if (userData->shareId == 0)
	{
		rsp.set_result(AlreadyShare);
		rsp.set_share_title("");
		rsp.set_share_content("");
		return;
	}

	userData->shareId = 0;
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	rsp.set_share_content(cfg.hall_share.content);
	rsp.set_share_title(cfg.hall_share.title);

	auto pHallLogicData = m_msgHandler->getHallLogicData(pUserName);
	time_t curSecs = time(NULL);
	int theDay = localtime(&curSecs)->tm_yday;

	//处理如果用户一直在线跨天
	if (!pHallLogicData->share_info().has_today())
	{
		pHallLogicData->mutable_share_info()->set_today(theDay);
	}

	//隔天则重新累计领取次数
	else if (pHallLogicData->share_info().today() != (uint32_t)theDay)
	{
		pHallLogicData->mutable_share_info()->set_today(theDay);
		pHallLogicData->mutable_share_info()->set_reward_count(0);
	}

	NProject::RecordItemVector recordItemVector;

	auto nRewardCount = pHallLogicData->share_info().reward_count();
	if (nRewardCount >= 0 && nRewardCount < cfg.hall_share.share_reward_list.size())
	{
		for (auto rewardCfg = cfg.hall_share.share_reward_list[nRewardCount].reward.begin();
			rewardCfg != cfg.hall_share.share_reward_list[nRewardCount].reward.end();
			++rewardCfg)
		{
			auto shareReward = rsp.add_share_reward();
			shareReward->set_id(rewardCfg->first);
			shareReward->set_num(rewardCfg->second);

			RecordItem item(rewardCfg->first, rewardCfg->second);
			recordItemVector.push_back(item);
		}
	}

	m_msgHandler->notifyDbProxyChangeProp(pUserName, strlen(pUserName), recordItemVector, EPropFlag::ShareReward, "分享奖励", NULL);
}

void CHallLogic::generateShareID(const char *pUserName)
{
	if (pUserName == NULL)
	{
		OptWarnLog("CHallLogic generateShareID, user name == NULL");
		return;
	}
	
	auto userData = m_msgHandler->getConnectUserData(pUserName);
	if (userData == NULL)
	{
		OptWarnLog("CHallLogic generateShareID, get connect user data, user:%s", pUserName);
		return;
	}

	userData->shareId = (unsigned int)time(NULL);
}

void CHallLogic::handleGetScoresItemReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get scores item info")) return;
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);

	m_msgHandler->sendMessageToService(NFrame::ServiceType::DBProxyCommonSrv, NULL, 0, CommonDBSrvProtocol::BUSINESS_GET_MALL_SCORES_INFO_REQ, 
									   ud->connId, ud->connIdLen);
}

void CHallLogic::handleGetScoresItemRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = getConnectInfo("in handleGetScoresItemRsp", userName);
	if (conn == NULL) return;

	com_protocol::ClientGetScoresItemRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "handle get scores item rsp")) return;
	rsp.set_clear_time((uint32_t)(m_clearScoresTime));

	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "handle get scores item rsp");
	m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_GET_SCORES_ITEM_RSP, conn);
}

void CHallLogic::handleExchangeScoresItemReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to exchange scores item")) return;
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);

	m_msgHandler->sendMessageToService(NFrame::ServiceType::DBProxyCommonSrv, data, len, CommonDBSrvProtocol::BUSINESS_EXCHANGE_SCORES_ITEM_REQ,
									   ud->connId, ud->connIdLen);
}

void CHallLogic::handleExchangeScoresItemRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientExchangeScoresItemRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "handle exchange scores item rsp")) return;
	
	string userName;
	ConnectProxy* conn = getConnectInfo("in handleExchangeScoresItemRsp", userName);
	if (conn == NULL) return;
		
	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	com_protocol::HallLogicData* hallLogicData = ud->hallLogicData;

	//防止用户在线跨天
	time_t curSecs = time(NULL);
	int theDay = localtime(&curSecs)->tm_yday;
	setUserShareInfo(hallLogicData, theDay);

	//获取分享信息
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	auto nRewardCount = hallLogicData->share_info().reward_count();
	if (nRewardCount >= 0 && nRewardCount < cfg.hall_share.share_reward_list.size())
	{
		for (auto rewardCfg = cfg.hall_share.share_reward_list[nRewardCount].reward.begin();
			rewardCfg != cfg.hall_share.share_reward_list[nRewardCount].reward.end();
			++rewardCfg)
		{
			auto shareReward = rsp.add_share_reward();
			shareReward->set_item_id(rewardCfg->first);
			shareReward->set_item_num(rewardCfg->second);
		}
	}

	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "handle exchange scores item rsp");
	m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_EXCHANGE_SCORES_ITEM_RSP, conn);
}

void CHallLogic::handleQuitRetainReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->checkLoginIsSuccess("receive not login successs message to get login reward list")) return;

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
	com_protocol::HallLogicData* hallData = ud->hallLogicData;
	if (hallData == NULL)
	{
		OptErrorLog("CHallLogic handleQuitRetainReq, hallData == NULL");
		return;
	}

	com_protocol::ClientHallQuitRetainRewardRsp rsp;
	rsp.set_reward_id(EQuitRetainRewardType::EQuitRetainRewardNot);

	do
	{
		//VIP奖励
		if (hallData->vip_info().receive() == VIPStatus::VIPReceiveReward)
		{
			rsp.set_reward_id(EQuitRetainRewardType::EQuitRetainRewardVIP);
			break;
		}

		//签到奖励
		auto loginReward = hallData->login_reward();
		const vector<login_reward>& login_reward_list = HallConfigData::config::getConfigValue().login_reward_list;
		uint32_t rewardIndex = loginReward.status_size() - 1;
		if (rewardIndex >= 0 && rewardIndex < login_reward_list.size() && (loginReward.status(rewardIndex) == LoginRewardStatus::CanGetReward)
		    && login_reward_list[rewardIndex].type == EPropType::PropGold)
		{
			rsp.set_reward_id(EQuitRetainRewardType::EQuitRetainRewardSign);
			rsp.set_reward_type(EPropType::PropGold);
			
			// 奥榕平台配置特殊值
			rsp.set_reward_num((m_msgHandler->getConnectUserData()->platformType == ThirdPlatformType::AoRong) ? login_reward_list[rewardIndex].special : login_reward_list[rewardIndex].num);
			break;
		}
				
		//发起查询局内抽奖信息请求 
		com_protocol::GetGameDataReq gameLogicReq;
		gameLogicReq.set_primary_key(NProject::BuyuLogicDataKey);
		ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(m_msgHandler->getConnectProxyContext().conn);
		gameLogicReq.set_second_key(ud->connId, ud->connIdLen);

		const char* msgData = m_msgHandler->serializeMsgToBuffer(gameLogicReq, "get game logic data request");
		if (msgData == NULL
		|| Success != m_msgHandler->sendMessageToGameDbProxy(msgData, gameLogicReq.ByteSize(), GET_GAME_DATA_REQ, ud->connId, ud->connIdLen, EServiceReplyProtocolId::CUSTOM_GET_GAME_DATA_FOR_TASK_REWARD))
		{
			OptErrorLog("CHallLogic handleQuitRetainReq, send to game db");
		}
		return;
	
	} while (0);
		
	if (rsp.reward_id() != EQuitRetainRewardType::EQuitRetainRewardNot)
	{
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "quit retain reward rsp");
		if (msgBuffer != NULL)
			m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_QUIT_RETAIN_RSP);
	}
}

void CHallLogic::getGameDataForTaskReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetGameDataRsp gameLogicDataRsp;

	if (!m_msgHandler->parseMsgFromBuffer(gameLogicDataRsp, data, len, "get game data for task reward") || gameLogicDataRsp.result() != 0)
	{
		OptErrorLog("CHallLogic getGameDataForTaskReward, get game data for task reward rc = %d", gameLogicDataRsp.result());
		return;
	}

	com_protocol::BuyuLogicData game_logic_data;
	if ((gameLogicDataRsp.has_data() && gameLogicDataRsp.data().length() > 0) && !m_msgHandler->parseMsgFromBuffer(game_logic_data,
		 gameLogicDataRsp.data().c_str(), gameLogicDataRsp.data().length(), "get game data for task reward"))
	{
		return;
	}

	string userName;
	ConnectProxy* conn = getConnectInfo("in getGameDataForTaskReward", userName);
	if (conn == NULL) return;
	
	//任务奖励没有领取
	if (game_logic_data.has_task_info() && game_logic_data.task_info().catch_fish_tasks(0).no_receive_item_size() > 0)
	{
		com_protocol::ClientHallQuitRetainRewardRsp rsp;
		rsp.set_reward_id(EQuitRetainRewardType::EQuitRetainRewardLuckDraw);
		const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "quit retain reward rsp");
		if (msgBuffer != NULL)
			m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_QUIT_RETAIN_RSP, conn);
		return;
	}

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	com_protocol::ClientHallQuitRetainRsp retainRsp;
	retainRsp.set_phone_bill(ud->phoneBill);
	retainRsp.set_prop_voucher(ud->propVoucher);
	retainRsp.set_score(ud->score);
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(retainRsp, "quit retain reward rsp");
	if (msgBuffer != NULL)
	{
		m_msgHandler->sendMsgToProxy(msgBuffer, retainRsp.ByteSize(), LOGINSRV_QUIT_RETAIN_NO_REWARD_RSP, conn);
	}
	
}

void CHallLogic::handleGameAddPropReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ForwardMessageChangePropNotify notify;
	if (!m_msgHandler->parseMsgFromBuffer(notify, data, len, "handle game add prop req")) return;

	string userName;
	ConnectProxy* conn = getConnectInfo("in handleGameAddPropReq", userName);
	if (conn == NULL) return;

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);

	for (auto i = 0; i != notify.change_prop_size(); ++i)
	{
		switch (notify.change_prop(i).type())
		{
		case EPropType::PropVoucher:
			ud->propVoucher += notify.change_prop(i).num();
			break;

		case EPropType::PropPhoneFareValue:
			ud->phoneBill += notify.change_prop(i).num();
			break;

		case EPropType::PropScores:
			ud->score += notify.change_prop(i).num();
			break;
		}
	}
}

void CHallLogic::setUserShareInfo(com_protocol::HallLogicData* hallLogicData, int theDay)
{
	if (hallLogicData == NULL)
	{
		OptErrorLog("CHallLogic setUserShareInfo, hallLogicData == NULL");
		return;
	}

	if (!hallLogicData->has_share_info())
	{
		hallLogicData->mutable_share_info()->set_reward_count(0);
		hallLogicData->mutable_share_info()->set_today(theDay);
	}
	else
	{
		if (hallLogicData->share_info().today() != (uint32_t)theDay)
		{
			hallLogicData->mutable_share_info()->set_reward_count(0);
			hallLogicData->mutable_share_info()->set_today(theDay);
		}
	}	
}

void CHallLogic::setUserScoreInfo(com_protocol::HallLogicData* hallLogicData, ConnectProxy* conn)
{
	if (hallLogicData == NULL)
	{
		OptErrorLog("CHallLogic setUserScoreInfo, hallLogicData == NULL");
		return;
	}

	if (conn == NULL)
	{
		OptErrorLog("CHallLogic setUserScoreInfo, conn == NULL");
		return;
	}

	if(!hallLogicData->has_scores_info())
	{
		auto scoresInfo = hallLogicData->mutable_scores_info();
		scoresInfo->set_scores_clear_num(m_clearScoresNum);
	}

	ConnectUserData* ud = (ConnectUserData*)m_msgHandler->getProxyUserData(conn);
	if (ud != NULL)
	{
		clearUserScores(hallLogicData, ud->connId, ud->connIdLen);
	}
}

void CHallLogic::openClearScoresTimer()
{
	const HallConfigData::CommonCfg& cfg = HallConfigData::config::getConfigValue().common_cfg;

	struct tm startTime;
	if (strptime(cfg.scores_clear_start_time.c_str(), "%Y-%m-%d-%H:%M:%S", &startTime) == NULL)
	{
		OptErrorLog("open clear score timer, scores shop cfg error, scores clear start time:%s", cfg.scores_clear_start_time.c_str());
		return;
	}

	if (cfg.scores_clear_septum_day <= 0)
	{
		OptErrorLog("open clear score timer, scores clear septum day error! septum day: %d", cfg.scores_clear_septum_day);
		return;
	}

    uint32_t clearScoresNum = 0;
	const auto curTime = time(NULL);
	const auto septumDay = cfg.scores_clear_septum_day * DAY_SEC;
	time_t clearScoresTime = mktime(&startTime) + septumDay;
	if (clearScoresTime < curTime)
	{
		clearScoresNum = ((curTime - clearScoresTime) / septumDay) + 1;
		clearScoresTime += (clearScoresNum * septumDay);
	}
	
	if (m_clearScoreTimerId != 0)  // 存在定时器了
	{
		if (clearScoresNum == m_clearScoresNum && clearScoresTime == m_clearScoresTime) return;  // 时间上没变化则直接返回
		
		m_msgHandler->killTimer(m_clearScoreTimerId);
		m_clearScoreTimerId = 0;
	}
	
	m_clearScoresNum = clearScoresNum;      // 积分清空次数
	m_clearScoresTime = clearScoresTime;    // 清空积分的时间

	struct tm nextClearTime;
    localtime_r(&m_clearScoresTime, &nextClearTime);

	m_clearScoreTimerId = m_msgHandler->setTimer((m_clearScoresTime - curTime) * 1000, (TimerHandler)&CHallLogic::clearAllOnLineUserScores, 0, NULL, 1, this);
	OptInfoLog("set clear all user score, timer id = %u, config start time = %s, step day = %u, step count = %u, time second = %u, interval = %u, clear time = %d-%02d-%02d %02d:%02d:%02d",
	m_clearScoreTimerId, cfg.scores_clear_start_time.c_str(), cfg.scores_clear_septum_day, m_clearScoresNum, (unsigned int)m_clearScoresTime, m_clearScoresTime - curTime,
	nextClearTime.tm_year + 1900, nextClearTime.tm_mon + 1, nextClearTime.tm_mday, nextClearTime.tm_hour, nextClearTime.tm_min, nextClearTime.tm_sec);
	
	WriteDataLog("Set Clear User Score Timer|%s|%u|%u|%u|%u|%u|%d-%02d-%02d %02d:%02d:%02d",
	cfg.scores_clear_start_time.c_str(), cfg.scores_clear_septum_day, m_clearScoresNum, (unsigned int)m_clearScoresTime, m_clearScoreTimerId, m_clearScoresTime - curTime,
	nextClearTime.tm_year + 1900, nextClearTime.tm_mon + 1, nextClearTime.tm_mday, nextClearTime.tm_hour, nextClearTime.tm_min, nextClearTime.tm_sec);
}

void CHallLogic::clearAllOnLineUserScores(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	const HallConfigData::CommonCfg& cfg = HallConfigData::config::getConfigValue().common_cfg;
	time_t curTime = time(NULL);
	const unsigned int mistakeSeconds = 60;              // 允许误差范围 mistakeSeconds 秒
	if ((curTime + mistakeSeconds) < m_clearScoresTime)  // 还不到触发时间点
	{
		if (m_clearScoreTimerId != timerId)
		{
			OptErrorLog("on clear all user score error, current timer id = %u, score timer id = %u", timerId, m_clearScoreTimerId);
			m_msgHandler->killTimer(m_clearScoreTimerId);
		}
	
		// 时间还没到，则重新设置定时器
		m_clearScoreTimerId = m_msgHandler->setTimer((m_clearScoresTime - curTime) * 1000, (TimerHandler)&CHallLogic::clearAllOnLineUserScores, 0, NULL, 1, this);
		
		OptErrorLog("on clear all user score error, current timer id = %u, new timer id = %u, config start time = %s, step day = %u, step count = %u, time second = %u, current second = %u",
		timerId, m_clearScoreTimerId, cfg.scores_clear_start_time.c_str(), cfg.scores_clear_septum_day, m_clearScoresNum, (unsigned int)m_clearScoresTime, (unsigned int)curTime);
		
		WriteDataLog("Clear All Online User Score Error|%s|%u|%u|%u|%u|%u|%u",
		cfg.scores_clear_start_time.c_str(), cfg.scores_clear_septum_day, m_clearScoresNum, (unsigned int)m_clearScoresTime, m_clearScoreTimerId, timerId, (unsigned int)curTime);

		return;
	}
	
	++m_clearScoresNum;
	
	m_clearScoreTimerId = 0;
	if (cfg.scores_clear_septum_day > 0)
	{
		curTime += (cfg.scores_clear_septum_day * DAY_SEC);
	    m_clearScoreTimerId = m_msgHandler->setTimer(cfg.scores_clear_septum_day * DAY_SEC * 1000, (TimerHandler)&CHallLogic::clearAllOnLineUserScores, 0, NULL, 1, this);
	}
	m_clearScoresTime = curTime;
	
	struct tm nextClearTime;
    localtime_r(&m_clearScoresTime, &nextClearTime);
	
	OptInfoLog("on clear all user score, current timer id = %u, new timer id = %u, config start time = %s, step day = %u, step count = %u, time second = %u, clear time = %d-%02d-%02d %02d:%02d:%02d",
	timerId, m_clearScoreTimerId, cfg.scores_clear_start_time.c_str(), cfg.scores_clear_septum_day, m_clearScoresNum, (unsigned int)m_clearScoresTime,
	nextClearTime.tm_year + 1900, nextClearTime.tm_mon + 1, nextClearTime.tm_mday, nextClearTime.tm_hour, nextClearTime.tm_min, nextClearTime.tm_sec);
	
	WriteDataLog("Clear All Online User Score|%s|%u|%u|%u|%u|%u|%d-%02d-%02d %02d:%02d:%02d",
	cfg.scores_clear_start_time.c_str(), cfg.scores_clear_septum_day, m_clearScoresNum, (unsigned int)m_clearScoresTime, m_clearScoreTimerId, timerId,
	nextClearTime.tm_year + 1900, nextClearTime.tm_mon + 1, nextClearTime.tm_mday, nextClearTime.tm_hour, nextClearTime.tm_min, nextClearTime.tm_sec);

	const IDToConnectProxys& userConnects = m_msgHandler->getConnectProxy();
	for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end(); ++it)
	{
		clearUserScores(m_msgHandler->getHallLogicData(it->first.c_str()), it->first.c_str(), it->first.size());
	}
}

void CHallLogic::clearUserScores(com_protocol::HallLogicData* hallLogicData, const char* userName, int uNLen)
{
	if (hallLogicData == NULL)
	{
		OptErrorLog("clear user score, hallLogicData == NULL");
		return;
	}

	//清空积分
	if(hallLogicData->scores_info().scores_clear_num() != m_clearScoresNum)
	{
		com_protocol::ResetUserOtherInfoReq req;
		req.set_query_username(userName);
		auto otherInfo = req.add_other_info();
		otherInfo->set_info_flag(EPropType::PropScores);
		otherInfo->set_int_value(0);

		auto msgBuffer = m_msgHandler->serializeMsgToBuffer(req, "clear user scores");
		m_msgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgBuffer, req.ByteSize(), CommonDBSrvProtocol::BUSINESS_RESET_USER_OTHER_INFO_REQ, userName, uNLen);

		WriteDataLog("Clear User Score|%s|%u|%u", userName, hallLogicData->scores_info().scores_clear_num(), m_clearScoresNum);
		OptWarnLog("clear user score, name = %s, user num = %u, current num = %u", userName, hallLogicData->scores_info().scores_clear_num(), m_clearScoresNum);
		
		hallLogicData->mutable_scores_info()->set_scores_clear_num(m_clearScoresNum);
	}
}

void CHallLogic::resetUserOtherInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// to do nothing now
}


void CHallLogic::load(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;
	
	// 收到的CommonDB代理服务应答消息
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_ROUND_END_DATA_CHARGE_RSP, (ProtocolHandler)&CHallLogic::addGoldReply, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_MULTI_USER_INFO_RSP, (ProtocolHandler)&CHallLogic::getFriendStaticDataReply, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_USERNAME_BY_PLATFROM_RSP, (ProtocolHandler)&CHallLogic::getUserNameByPlatformReply, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_CHANGE_PROP_RSP, (ProtocolHandler)&CHallLogic::changePropReply, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_USER_OTHER_INFO_RSP, (ProtocolHandler)&CHallLogic::getUserInfo, this);

	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_MANY_USER_OTHER_INFO_RSP, (ProtocolHandler)&CHallLogic::getManyUserInfo, this);
	
	m_msgHandler->registerProtocol(GameSrvDBProxy, LOGINSRV_GET_OFFLINE_USER_HALL_DATA_RSP, (ProtocolHandler)&CHallLogic::getOfflineUserHallDataReply, this);
	
	// 登陆签到奖励
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_LOGIN_REWARD_LIST_REQ, (ProtocolHandler)&CHallLogic::getLoginRewardList, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_RECEIVE_LOGIN_REWARD_REQ, (ProtocolHandler)&CHallLogic::receiveLoginReward, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_USERNAME_BY_PLATFROM_REQ, (ProtocolHandler)&CHallLogic::getUserNameByPlatform, this);
	
	// 好友操作
	m_msgHandler->registerProtocol(CommonSrv, ADD_FRIEND_REQ, (ProtocolHandler)&CHallLogic::addFriend, this);
	m_msgHandler->registerProtocol(CommonSrv, REMOVE_FRIEND_REQ, (ProtocolHandler)&CHallLogic::removeFriend, this);
	m_msgHandler->registerProtocol(CommonSrv, LOGINSRV_GET_FRIEND_DYNAMIC_DATA_REQ, (ProtocolHandler)&CHallLogic::getFriendDynamicData, this);
	m_msgHandler->registerProtocol(CommonSrv, LOGINSRV_FRIEND_DONATE_GOLD_REQ, (ProtocolHandler)&CHallLogic::donateGoldToFriend, this);
	m_msgHandler->registerProtocol(LoginSrv, REMOVE_FRIEND_RSP, (ProtocolHandler)&CHallLogic::removeFriendReply, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_FRIEND_DYNAMIC_DATA_REQ, (ProtocolHandler)&CHallLogic::getFriendDynamicData, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_FRIEND_STATIC_DATA_REQ, (ProtocolHandler)&CHallLogic::getFriendStaticData, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_FRIEND_STATIC_DATA_BYID_REQ, (ProtocolHandler)&CHallLogic::getFriendStaticDataById, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_REMOVE_FRIEND_REQ, (ProtocolHandler)&CHallLogic::removeFriendByClient, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_FRIEND_DONATE_GOLD_REQ, (ProtocolHandler)&CHallLogic::donateGoldToFriend, this);
	m_msgHandler->registerProtocol(LoginSrv, LOGINSRV_FRIEND_DONATE_GOLD_NOTIFY, (ProtocolHandler)&CHallLogic::donateGoldToFriendNotify, this);
	
	m_msgHandler->registerProtocol(MessageSrv, BUSINESS_GET_USER_SERVICE_ID_RSP, (ProtocolHandler)&CHallLogic::getUserSrvIdReply, this);
	
	// 聊天&消息通知
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_PRIVATE_CHAT_REQ, (ProtocolHandler)&CHallLogic::privateChat, this);
	m_msgHandler->registerProtocol(MessageSrv, BUSINESS_PRIVATE_CHAT_RSP, (ProtocolHandler)&CHallLogic::privateChatReply, this);
	m_msgHandler->registerProtocol(MessageSrv, BUSINESS_PRIVATE_CHAT_NOTIFY, (ProtocolHandler)&CHallLogic::notifyPrivateChat, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_WORLD_CHAT_REQ, (ProtocolHandler)&CHallLogic::worldChat, this);
	m_msgHandler->registerProtocol(MessageSrv, BUSINESS_WORLD_CHAT_RSP, (ProtocolHandler)&CHallLogic::worldChatReply, this);
	m_msgHandler->registerProtocol(MessageSrv, BUSINESS_WORLD_CHAT_NOTIFY, (ProtocolHandler)&CHallLogic::notifyWorldChat, this);
	
	m_msgHandler->registerProtocol(MessageSrv, BUSINESS_SYSTEM_MESSAGE_NOTIFY, (ProtocolHandler)&CHallLogic::systemMessageNotify, this);

	// 商城配置
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_MALL_CONFIG_REQ, (ProtocolHandler)&CHallLogic::getMallConfig, this);
	m_msgHandler->registerProtocol(HttpSrv, HttpServiceProtocol::GET_MALL_CONFIG_RSP, (ProtocolHandler)&CHallLogic::getMallConfigReply, this);
	
	// 当乐平台接口
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_CHECK_DOWNJOY_USER_REQ, (ProtocolHandler)&CHallLogic::checkDownJoyUser, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_DOWNJOY_RECHARGE_TRANSACTION_REQ, (ProtocolHandler)&CHallLogic::getDownJoyRechargeTransaction, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_CANCEL_DOWNJOY_RECHARGE_NOTIFY, (ProtocolHandler)&CHallLogic::cancelDownJoyRechargeNotify, this);
	m_msgHandler->registerProtocol(HttpSrv, CHECK_DOWNJOY_USER_RSP, (ProtocolHandler)&CHallLogic::checkDownJoyUserReply, this);
	m_msgHandler->registerProtocol(HttpSrv, GET_DOWNJOY_RECHARGE_TRANSACTION_RSP, (ProtocolHandler)&CHallLogic::getDownJoyRechargeTransactionReply, this);
	m_msgHandler->registerProtocol(HttpSrv, DOWNJOY_USER_RECHARGE_NOTIFY, (ProtocolHandler)&CHallLogic::downJoyRechargeTransactionNotify, this);
	m_msgHandler->registerProtocol(HttpSrv, CHECK_FACEBOOK_USER_RSP, (ProtocolHandler)&CHallLogic::checkFacebookUserReply, this);

	//VIP
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_USER_VIP_INFO_REQ, (ProtocolHandler)&CHallLogic::getUserVIPInfo, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_VIP_REWARD_REQ, (ProtocolHandler)&CHallLogic::getVIPReward, this);

	//公告
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_NOTICE_REQ, (ProtocolHandler)&CHallLogic::getNoticeReq, this);

	//分享
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_SHARE_REQ, (ProtocolHandler)&CHallLogic::handleShareReq, this);
	
	//积分商城
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_SCORES_ITEM_REQ, (ProtocolHandler)&CHallLogic::handleGetScoresItemReq, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_MALL_SCORES_INFO_RSP, (ProtocolHandler)&CHallLogic::handleGetScoresItemRsp, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_EXCHANGE_SCORES_ITEM_REQ, (ProtocolHandler)&CHallLogic::handleExchangeScoresItemReq, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_EXCHANGE_SCORES_ITEM_RSP, (ProtocolHandler)&CHallLogic::handleExchangeScoresItemRsp, this);
	
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_RESET_USER_OTHER_INFO_RSP, (ProtocolHandler)&CHallLogic::resetUserOtherInfoReply, this);

	//退出挽留
	m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_QUIT_RETAIN_REQ, (ProtocolHandler)&CHallLogic::handleQuitRetainReq, this);

	m_msgHandler->registerProtocol(GameSrvDBProxy, CUSTOM_GET_GAME_DATA_FOR_TASK_REWARD, (ProtocolHandler)&CHallLogic::getGameDataForTaskReward, this);
	
	//增加道具
	m_msgHandler->registerProtocol(CommonSrv, GAME_ADD_PROP_NOTIFY, (ProtocolHandler)&CHallLogic::handleGameAddPropReq, this);

	//新版本的登逻辑处理
    m_msgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_VERIFICATION_REQ, (ProtocolHandler)&CHallLogic::handleLoginVerf, this);

	// 客户端版本管理
	m_versionManager.init(m_msgHandler, LOGINSRV_CHECK_CLIENT_VERSION_REQ, LOGINSRV_CHECK_CLIENT_VERSION_RSP);
	
	// 其他逻辑处理
	m_hallLogicHandler.load(m_msgHandler);
	
	const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
	m_noticeVersion = cfg.hall_notice.notice_version;

	//开始定时清空用户积分
	openClearScoresTimer();
}

void CHallLogic::unLoad(CSrvMsgHandler* msgHandler)
{
	m_hallLogicHandler.unLoad(msgHandler);
}


void CHallLogic::handleLoginVerf(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::LoginMsgReq loginReq;
	if (!m_msgHandler->parseMsgFromBuffer(loginReq, data, len, "login verify user request")) return;
	const string& open_id = loginReq.open_id();
	const string& sid     = loginReq.sid();
	const string& sign    = loginReq.sign();
	// const string& ext     = loginReq.ext();

	const string appkey="df90fcedr4d903w2";
	const string appscrete="mop34d4f6tgyugfl";

	//	进行验证
	map<string, string>  map1;
	map1.insert(pair<string,string>("open_id",open_id));
	map1.insert(pair<string, string>("sid",sid));
	map1.insert(pair<string, string>("appkey",appkey));
	map1.insert(pair<string, string>("game_id","by"));

	int count = 0 ;
	int size  = map1.size();
	string str ;

	map<string, string>::iterator it;
	for (it = map1.begin(); it != map1.end(); ++it)
	{
		string key    = (*it).first.c_str();
		string value  = (*it).second.c_str();
		++count;
		if(count==size)
		{
			str = str + key+"="+value;
		}
		else
		{
			str = str + key+"="+value+"&";
		}
		
	}

	str=str+appscrete;
	char md5Buff[32] = {0};
	//OptInfoLog("str:%s",str.c_str());
	//OptInfoLog("str_len:%d",str.length());
	NProject::md5Encrypt(str.c_str(),str.length(),md5Buff, false);
	string md5str(md5Buff);
	//OptInfoLog("sign:%s",sign.c_str());
	//OptInfoLog("chansheng:%s",md5str.c_str());
	int result = md5str.compare(sign) ; 
	com_protocol::ClientLoginRsp  rsp;

	if(0==result)
	{
		OptInfoLog("user login verify is ok");
		//返回code=0 client
		rsp.set_result(0);
	}
	else
	{
		OptErrorLog("user login verify is error, open id = %s, sid = %s, sign = %s, md5 result = %s", open_id.c_str(), sid.c_str(), sign.c_str(), md5str.c_str());
		//返回code=1 client
		rsp.set_result(1);
	}
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, "login verify user reply");
	if (msgBuffer != NULL)
	{
		m_msgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_VERIFICATION_RSP);
	}
} 

}

