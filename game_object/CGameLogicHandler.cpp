
/* author : limingfan
 * date : 2018.01.05
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

#include "CGameObject.h"
#include "CGameLogicHandler.h"


using namespace NProject;

namespace NPlatformService
{

CGameLogicHandler::CGameLogicHandler()
{
	m_msgHandler = NULL;
}

CGameLogicHandler::~CGameLogicHandler()
{
	m_msgHandler = NULL;
}

int CGameLogicHandler::init(CGameMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

    m_msgHandler->registerProtocol(OutsideClientSrv, COMM_GET_INVITATION_LIST_REQ, (ProtocolHandler)&CGameLogicHandler::getInvitationList, this);
	m_msgHandler->registerProtocol(DBProxySrv, DBPROXY_GET_INVITATION_LIST_RSP, (ProtocolHandler)&CGameLogicHandler::getInvitationListReply, this);
	
	m_msgHandler->registerProtocol(OutsideClientSrv, COMM_GET_LAST_PLAYER_LIST_REQ, (ProtocolHandler)&CGameLogicHandler::getLastPlayerList, this);
	m_msgHandler->registerProtocol(DBProxySrv, DBPROXY_GET_LAST_PLAYER_LIST_RSP, (ProtocolHandler)&CGameLogicHandler::getLastPlayerListReply, this);

    m_msgHandler->registerProtocol(OutsideClientSrv, COMM_INVITE_PLAYER_PLAY_NOTIFY, (ProtocolHandler)&CGameLogicHandler::playerInvitePlayerPlay, this);
	m_msgHandler->registerProtocol(CommonSrv, SERVICE_PLAYER_INVITATION_NOTIFY, (ProtocolHandler)&CGameLogicHandler::playerReceiveInvitationNotify, this);

    m_msgHandler->registerProtocol(OutsideClientSrv, COMM_PLAYER_REFUSE_INVITE_NOTIFY, (ProtocolHandler)&CGameLogicHandler::refusePlayerInvitationNotify, this);
	m_msgHandler->registerProtocol(CommonSrv, SERVICE_REFUSE_INVITATION_NOTIFY, (ProtocolHandler)&CGameLogicHandler::playerReceiveRefuseInvitationNotify, this);

    m_msgHandler->registerProtocol(OutsideClientSrv, COMM_PLAYER_DISMISS_ROOM_REQ, (ProtocolHandler)&CGameLogicHandler::playerAskDismissRoom, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, COMM_PLAYER_DISMISS_ROOM_ANSWER_NOTIFY, (ProtocolHandler)&CGameLogicHandler::playerAnswerDismissRoom, this);
	
	m_msgHandler->registerProtocol(CommonSrv, SERVICE_MANAGER_GIVE_GOODS_NOTIFY, (ProtocolHandler)&CGameLogicHandler::giveGoodsNotify, this);
	m_msgHandler->registerProtocol(OutsideClientSrv, COMM_SEND_MSG_INFO_NOTIFY, (ProtocolHandler)&CGameLogicHandler::playerChat, this);

	return SrvOptSuccess;
}

void CGameLogicHandler::unInit()
{
}

void CGameLogicHandler::updateConfig()
{
}

// 发送赢家公告
void CGameLogicHandler::sendWinnerNotice(const char* hallId, const string& username, const string& noticeMsg,
	                                     const StringVector& matchingWords, const vector<unsigned int>& objColour)
{
	com_protocol::ClientSndMsgInfoNotify winMsgNtf;
	com_protocol::MessageInfo* msgInfo = winMsgNtf.mutable_msg_info();
	msgInfo->set_msg_type(com_protocol::EMsgType::EWordsMsg);
	msgInfo->set_msg_content(noticeMsg);
	msgInfo->set_msg_owner(com_protocol::EMsgOwner::ESystemMsg);
	msgInfo->set_hall_id(hallId);

	const unsigned int objColourSize = objColour.size();
	unsigned int colourIdx = 0;
    for (StringVector::const_iterator it = matchingWords.begin(); it != matchingWords.end(); ++it)
	{
		com_protocol::ColourMsgInfo* matchingInfo = msgInfo->add_matching_info();
		matchingInfo->set_words(*it);
		
		if (colourIdx < objColourSize)
		{
			matchingInfo->set_colour((com_protocol::EMsgColour)objColour[colourIdx]);
			++colourIdx;
		}
	}

	const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(winMsgNtf, "winner notice");
	if (msgData != NULL)
	{
		m_msgHandler->getSrvOpt().forwardMessage(username.c_str(), msgData, winMsgNtf.ByteSize(), ServiceType::GameHallSrv,
		SERVICE_GAME_MSG_NOTICE_NOTIFY, m_msgHandler->getSrvId(), 0, "forward winner notice message");
	}
}


void CGameLogicHandler::setInvitationStatus(const SRoomData& roomData, google::protobuf::RepeatedPtrField<com_protocol::InvitationInfo>* invitationInfoList)
{
	const unsigned int minNeedGold = roomData.getPlayerNeedMinGold();
	com_protocol::EPlayerGameStatus status = com_protocol::EPlayerGameStatus::EOfflineGame;
	for (google::protobuf::RepeatedPtrField<com_protocol::InvitationInfo>::iterator it = invitationInfoList->begin();
		 it != invitationInfoList->end(); ++it)
	{
		status = it->status();
		if ((!roomData.isGoldRoom() || it->dynamic_info().game_gold() >= minNeedGold)  // 金币满足
			&& (status == com_protocol::EPlayerGameStatus::EOnlineGame
				|| status == com_protocol::EPlayerGameStatus::EViewGame
				|| status == com_protocol::EPlayerGameStatus::EPrepareGame))  // 状态满足
		{
			it->set_invitation_flag(com_protocol::EInvitationFlag::ECanBeInvite);
		}
	}
}

void CGameLogicHandler::onPlayerDismissRoom(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	char strHallId[IdMaxLen] = {0};
	snprintf(strHallId, sizeof(strHallId) - 1, "%u", userId);
		
	char strRoomId[IdMaxLen] = {0};
	snprintf(strRoomId, sizeof(strRoomId) - 1, "%u", (unsigned int)(unsigned long)param);
		
	SRoomData* roomData = m_msgHandler->getRoomDataById(strRoomId, strHallId);
	if (roomData != NULL)
	{
		OptInfoLog("player dismiss room, timerId = %u, hallId = %s, roomId = %s", timerId, strHallId, strRoomId);
		
		// 结算记录
		m_msgHandler->cardCompleteSettlement(strHallId, strRoomId, *roomData, false);

		com_protocol::ClientConfirmDismissRoomNotify cdrNtf;
		m_msgHandler->sendPkgMsgToRoomPlayers(*roomData, cdrNtf, COMM_PLAYER_CONFIRM_DISMISS_ROOM_NOTIFY, "confirm dismiss room");
											  
		m_msgHandler->delRoomData(strHallId, strRoomId, false);  // 删除解散房间
	}
}

void CGameLogicHandler::getInvitationList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	SRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData == NULL) return;

	com_protocol::ClientGetInvitationListReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get invitation list request")) return;
	req.set_hall_id(cud->hallId);
	
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req, DBPROXY_GET_INVITATION_LIST_REQ, "get invitation list request");
}

void CGameLogicHandler::getInvitationListReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* conn = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "get invitation list reply");
	if (conn == NULL) return;      // 玩家离开了
	
	GameUserData* cud = (GameUserData*)m_msgHandler->getProxyUserData(conn);
	SRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData == NULL) return;  // 玩家不在房间了
	
	com_protocol::ClientGetInvitationListRsp rsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(rsp, data, len, "get invitation list reply")) return;
	
	if (rsp.result() == SrvOptSuccess && rsp.invitation_info_size() > 0)
	{
		setInvitationStatus(*roomData, rsp.mutable_invitation_info());
	}
	
	int rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, COMM_GET_INVITATION_LIST_RSP, conn, "get invitation list reply");
	OptInfoLog("get invitation list reply, usrename = %s, hall id = %s, room id = %s, result = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, rsp.result(), rc);
}

void CGameLogicHandler::getLastPlayerList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	SRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData == NULL) return;

	com_protocol::ClientGetLastPlayerListReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get last player list request")) return;
	
	req.set_username(cud->userName);
	req.set_hall_id(cud->hallId);

	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req, DBPROXY_GET_LAST_PLAYER_LIST_REQ, "get last player list request");
}

void CGameLogicHandler::getLastPlayerListReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* conn = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "get last player list reply");
	if (conn == NULL) return;      // 玩家离开了
	
	GameUserData* cud = (GameUserData*)m_msgHandler->getProxyUserData(conn);
	SRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData == NULL) return;  // 玩家不在房间了
	
	com_protocol::ClientGetLastPlayerListRsp rsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(rsp, data, len, "get last player list reply")) return;
	
	if (rsp.result() == SrvOptSuccess && rsp.last_players_size() > 0)
	{
		setInvitationStatus(*roomData, rsp.mutable_last_players());
	}

	int rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, COMM_GET_LAST_PLAYER_LIST_RSP, conn, "get last player list reply");
	OptInfoLog("get last player list reply, usrename = %s, hall id = %s, room id = %s, result = %d, rc = %d",
	cud->userName, cud->hallId, cud->roomId, rsp.result(), rc);
}

// 玩家邀请玩家一起游戏
void CGameLogicHandler::playerInvitePlayerPlay(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientInvitePlayerPlayNotify ntf;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(ntf, data, len, "player invite player")) return;
	
	GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	SRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData == NULL) return;  // 不在房间了

	// 验证条件是否满足
	const com_protocol::InvitationInfo& invInfo = ntf.dst_invitation_info();
	if (!invInfo.has_invitation_flag() || invInfo.invitation_flag() != com_protocol::EInvitationFlag::ECanBeInvite) return;
	
	unsigned int dstSrvType = 0;
	if (invInfo.status() == com_protocol::EPlayerGameStatus::EOnlineGame) dstSrvType = GameHallSrv;
	else if (invInfo.status() != com_protocol::EPlayerGameStatus::EViewGame
			 && invInfo.status() != com_protocol::EPlayerGameStatus::EPrepareGame) return;
	
	// 填写邀请数据
	ntf.set_allocated_src_detail_info(roomData->playerInfo[cud->userName].mutable_detail_info());
	ntf.set_wait_secs(m_msgHandler->getSrvOpt().getServiceCommonCfg().game_cfg.invitation_wait_secs);
	ntf.set_service_id(m_msgHandler->getSrvId());
	ntf.set_hall_id(cud->hallId);
	ntf.set_room_id(cud->roomId);
	
	// 被邀请的目标玩家
    int rc = SrvOptFailed;
	const string& dstUsername = invInfo.static_info().username();
	ConnectProxy* conn = m_msgHandler->getConnectProxy(dstUsername);
	if (conn != NULL)
	{
		// 目标玩家在同一服务器则直接通知
		rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(ntf, COMM_INVITE_PLAYER_PLAY_NOTIFY, conn, "player invite player");
	}
	else
	{
		// 经由消息中心转发消息到目标玩家所在的服务器
		const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(ntf, "player invite player");
		if (msgData != NULL)
		{
			rc = m_msgHandler->getSrvOpt().forwardMessage(dstUsername.c_str(), msgData, ntf.ByteSize(), dstSrvType,
			                                              SERVICE_PLAYER_INVITATION_NOTIFY, m_msgHandler->getSrvId(),
													      0, "player invite player");
		}
	}
	
	ntf.release_src_detail_info();
    
    OptInfoLog("player invite player, usrename = %s, hall id = %s, room id = %s, dst username = %s, rc = %d",
	cud->userName, cud->hallId, cud->roomId, dstUsername.c_str(), rc);
}

// 玩家收到邀请一起游戏的通知
void CGameLogicHandler::playerReceiveInvitationNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* conn = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "receive invitation notify");
	if (conn == NULL) return;      // 被邀请的玩家离线了

    int rc = m_msgHandler->sendMsgToProxy(data, len, COMM_INVITE_PLAYER_PLAY_NOTIFY, conn);
	OptInfoLog("receive invitation notify, usrename = %s, rc = %d", userName, rc);
}

// 拒绝玩家邀请通知
void CGameLogicHandler::refusePlayerInvitationNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientPlayerRefuseInviteNotify ntf;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(ntf, data, len, "refuse player invitation notify")) return;

    // 拒绝玩家的信息
	const GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	const SRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	const com_protocol::ClientRoomPlayerInfo* playerInfo = (roomData != NULL) ? roomData->getPlayerInfo(cud->userName) : NULL;
	if (playerInfo != NULL) *ntf.mutable_static_info() = playerInfo->detail_info().static_info();

	ConnectProxy* conn = m_msgHandler->getConnectProxy(ntf.username());
	if (conn != NULL)
	{
		// 目标玩家在同一服务器则直接通知
		m_msgHandler->getSrvOpt().sendMsgPkgToProxy(ntf, COMM_PLAYER_REFUSE_INVITE_NOTIFY, conn, "refuse player invitation notify");
	}
	else
	{
		m_msgHandler->getSrvOpt().forwardMessage(ntf.username().c_str(), data, len, ntf.service_id() / ServiceIdBaseValue,
												 SERVICE_REFUSE_INVITATION_NOTIFY, m_msgHandler->getSrvId(),
												 0, "refuse player invitation notify");
	}
}

// 玩家收到拒绝邀请通知
void CGameLogicHandler::playerReceiveRefuseInvitationNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* conn = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "receive refuse invitation notify");
	if (conn == NULL) return;      // 被拒绝的玩家离线了

    int rc = m_msgHandler->sendMsgToProxy(data, len, COMM_PLAYER_REFUSE_INVITE_NOTIFY, conn);
	OptInfoLog("receive refuse invitation notify, usrename = %s, rc = %d", userName, rc);
}

void CGameLogicHandler::playerAskDismissRoom(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	SRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);

	const com_protocol::ClientRoomPlayerInfo* playerInfo = (roomData != NULL) ? roomData->getPlayerInfo(cud->userName) : NULL;
	
	OptInfoLog("player ask dismiss room, usrename = %s, hallId = %s, roomId = %s, info = %p, status = %d, seat = %d, is gold = %d, timerId = %u",
	cud->userName, cud->hallId, cud->roomId, playerInfo, (playerInfo != NULL) ? playerInfo->status() : -1,
	(playerInfo != NULL) ? playerInfo->seat_id() : -1, roomData->isGoldRoom(), roomData->dismissRoomTimerId);
	
	if (playerInfo == NULL
	    || playerInfo->status() < com_protocol::ERoomPlayerStatus::EPlayerEnter
		|| playerInfo->seat_id() < 0
		|| roomData->isGoldRoom() || roomData->dismissRoomTimerId != 0) return;
	
	com_protocol::ClientDismissRoomRsp rsp;
	const ServiceCommonConfig::GameConfig& gameCfg = m_msgHandler->getSrvOpt().getServiceCommonCfg().game_cfg;
	const unsigned int curSecs = time(NULL);
	unsigned int& lastTimeSecs = roomData->playerDismissRoomTime[cud->userName];
	if ((curSecs - lastTimeSecs) > gameCfg.ask_dismiss_room_interval)
	{
		rsp.set_result(SrvOptSuccess);
		lastTimeSecs = curSecs;
	}
	else
	{
		rsp.set_result(GameObjectAskDismissRoomFrequently);
	}
	
	m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, COMM_PLAYER_DISMISS_ROOM_RSP, "player ask dismiss room");
	if (rsp.result() != SrvOptSuccess) return;

	if (!roomData->isOnPlay())
	{
		OptInfoLog("player direct dismiss room, usrename = %s, hallId = %s, roomId = %s",
		cud->userName, cud->hallId, cud->roomId);

		m_msgHandler->delRoomData(cud->hallId, cud->roomId);  // 还没有开始游戏的房间可直接解散
		return;
	}
	
	// 同步发送消息到房间的在座玩家
	com_protocol::ClientDismissRoomNotify dismissRoomNtf;
	dismissRoomNtf.set_wait_secs(gameCfg.dismiss_room_secs);
	dismissRoomNtf.set_ask_username(cud->userName);
	m_msgHandler->sendPkgMsgToRoomSitDownPlayers(*roomData, dismissRoomNtf, COMM_PLAYER_DISMISS_ROOM_ASK_NOTIFY, "player ask dismiss room");

	// 定时解散房间
	m_msgHandler->getSrvOpt().stopTimer(roomData->optTimerId);  // 停止当前定时器，暂停游戏
	roomData->agreeDismissRoomPlayer.clear();
	roomData->agreeDismissRoomPlayer.push_back(cud->userName);
    roomData->dismissRoomTimerId = m_msgHandler->setTimer(gameCfg.dismiss_room_secs * MillisecondUnit,
	                                                      (TimerHandler)&CGameLogicHandler::onPlayerDismissRoom,
														  atoi(cud->hallId), (void*)(unsigned long)atoi(cud->roomId), 1, this);
}

void CGameLogicHandler::playerAnswerDismissRoom(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	SRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData == NULL || roomData->dismissRoomTimerId == 0
	    || std::find(roomData->agreeDismissRoomPlayer.begin(), roomData->agreeDismissRoomPlayer.end(),
		             cud->userName) != roomData->agreeDismissRoomPlayer.end()) return;
	
	com_protocol::ClientDismissRoomNotify ntf;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(ntf, data, len, "answer dismiss room notify")) return;

	ntf.set_answer_username(cud->userName);
	m_msgHandler->sendPkgMsgToRoomSitDownPlayers(*roomData, ntf, COMM_PLAYER_DISMISS_ROOM_ANSWER_NOTIFY, "player answer dismiss room");
	
	if (ntf.is_agree() == 1)
	{
		roomData->agreeDismissRoomPlayer.push_back(cud->userName);
		if (roomData->agreeDismissRoomPlayer.size() >= roomData->getSeatedPlayers())
		{
			// 全部人都同意了则直接解散房间
			m_msgHandler->getSrvOpt().stopTimer(roomData->dismissRoomTimerId);
			onPlayerDismissRoom(0, atoi(cud->hallId), (void*)(unsigned long)atoi(cud->roomId), 0);
		}
	}
	else
	{
		// 任意一个玩家不同意则不可以解散房间
		OptInfoLog("player disagree dismiss room, usrename = %s, hallId = %s, roomId = %s",
		cud->userName, cud->hallId, cud->roomId);

        m_msgHandler->refuseDismissRoomContinueGame(cud, *roomData);
	}
}

// 管理员赠送物品通知
void CGameLogicHandler::giveGoodsNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	SRoomData* roomData = m_msgHandler->getRoomDataByUser(m_msgHandler->getContext().userData);
	if (roomData != NULL)
	{
		com_protocol::ClientGiveGoodsToPlayerNotify ggNtf;
		if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(ggNtf, data, len, "give goods notify")) return;

		RoomPlayerIdInfoMap::iterator it = roomData->playerInfo.find(m_msgHandler->getContext().userData);
		if (it != roomData->playerInfo.end())
		{
			if (ggNtf.give_goods().type() == EGameGoodsType::EGoodsGold)
			{
			    it->second.mutable_detail_info()->mutable_dynamic_info()->set_game_gold(ggNtf.give_goods().amount());
			}
			else if (ggNtf.give_goods().type() == EGameGoodsType::EGoodsRoomCard)
			{
				it->second.mutable_detail_info()->mutable_dynamic_info()->set_room_card(ggNtf.give_goods().amount());
			}
		}
		
		OptInfoLog("receive give goods notify, username = %s, type = %d, amount = %.2f",
		m_msgHandler->getContext().userData, ggNtf.give_goods().type(), ggNtf.give_goods().amount());
	}
}

void CGameLogicHandler::playerChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	GameUserData* cud = (GameUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	SRoomData* roomData = m_msgHandler->getRoomDataById(cud->roomId, cud->hallId);
	if (roomData != NULL)
	{
		RoomPlayerIdInfoMap::iterator userInfoIt = roomData->playerInfo.find(cud->userName);
		if (userInfoIt == roomData->playerInfo.end()
		    || userInfoIt->second.status() < com_protocol::ERoomPlayerStatus::EPlayerEnter
		    || userInfoIt->second.seat_id() < 0)
		{
			OptErrorLog("player send chat message but not sit down, username = %s", cud->userName);
			return;
		}
		
		const ServiceCommonConfig::PlayerChatCfg& chatCfg = m_msgHandler->getSrvOpt().getServiceCommonCfg().player_chat_cfg;
	    SUserChatInfo& plChatInfo = roomData->chatInfo[cud->userName];
		const unsigned int curSecs = time(NULL);
		if ((curSecs - plChatInfo.chatSecs) > chatCfg.interval_secs)
		{
			plChatInfo.chatSecs = curSecs;
			plChatInfo.chatCount = 0;
		}
		
		if (plChatInfo.chatCount >= chatCfg.chat_count)
		{
			// OptWarnLog("player send chat message frequently, username = %s, config interval = %u, count = %u, current count = %u",
			// cud->userName, chatCfg.interval_secs, chatCfg.chat_count, plChatInfo.chatCount);

			return;
		}

        com_protocol::ClientSndMsgInfoNotify smiNtf;
	    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(smiNtf, data, len, "room chat notify")) return;
		
		if (smiNtf.msg_info().msg_content().length() > chatCfg.chat_content_length) return;
		
		++plChatInfo.chatCount;

		*smiNtf.mutable_msg_info()->mutable_player_info() = userInfoIt->second.detail_info().static_info();
		m_msgHandler->sendPkgMsgToRoomSitDownPlayers(*roomData, smiNtf, COMM_SEND_MSG_INFO_NOTIFY, "player room chat notify");
	}
}

}


