
/* author : limingfan
 * date : 2017.11.01
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

#include "base/Function.h"
#include "common/CRandomNumber.h"

#include "CGameHall.h"
#include "CHallLogicHandler.h"


using namespace NProject;


namespace NPlatformService
{

CHallLogicHandler::CHallLogicHandler()
{
	m_msgHandler = NULL;
}

CHallLogicHandler::~CHallLogicHandler()
{
	m_msgHandler = NULL;
}

int CHallLogicHandler::init(CSrvMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_APPLY_JOIN_CHESS_HALL_REQ, (ProtocolHandler)&CHallLogicHandler::applyJoinChessHall, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_APPLY_JOIN_CHESS_HALL_RSP, (ProtocolHandler)&CHallLogicHandler::applyJoinChessHallReply, this);

	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_CANCEL_APPLY_CHESS_HALL_REQ, (ProtocolHandler)&CHallLogicHandler::cancelApplyChessHall, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_CANCEL_APPLY_CHESS_HALL_RSP, (ProtocolHandler)&CHallLogicHandler::cancelApplyChessHallReply, this);

    m_msgHandler->registerProtocol(ServiceType::OperationManageSrv, OPTMGR_CHESS_HALL_PLAYER_STATUS_NOTIFY, (ProtocolHandler)&CHallLogicHandler::chessHallPlayerStatusNotify, this);
	m_msgHandler->registerProtocol(ServiceType::OperationManageSrv, OPTMGR_PLAYER_QUIT_CHESS_HALL_NOTIFY, (ProtocolHandler)&CHallLogicHandler::playerQuitChessHallNotify, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_CREATE_HALL_GAME_ROOM_REQ, (ProtocolHandler)&CHallLogicHandler::createHallGameRoom, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_CREATE_HALL_GAME_ROOM_RSP, (ProtocolHandler)&CHallLogicHandler::createHallGameRoomReply, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_GET_MALL_DATA_REQ, (ProtocolHandler)&CHallLogicHandler::getMallInfo, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_BUY_MALL_GOODS_REQ, (ProtocolHandler)&CHallLogicHandler::buyMallGoods, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_PLAYER_BUY_MALL_GOODS_RSP, (ProtocolHandler)&CHallLogicHandler::buyMallGoodsReply, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_GET_MALL_BUY_RECORD_REQ, (ProtocolHandler)&CHallLogicHandler::getMallBuyRecord, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_PLAYER_GET_MALL_BUY_RECORD_RSP, (ProtocolHandler)&CHallLogicHandler::getMallBuyRecordReply, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_PLAYER_SEND_CHAT_MSG_NOTIFY, (ProtocolHandler)&CHallLogicHandler::playerChat, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, SERVICE_GAME_MSG_NOTICE_NOTIFY, (ProtocolHandler)&CHallLogicHandler::gameMsgNoticeNotify, this);
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_GET_HALL_MESSAGE_REQ, (ProtocolHandler)&CHallLogicHandler::getHallMsgInfo, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_MODIFY_USER_DATA_REQ, (ProtocolHandler)&CHallLogicHandler::modifyUserInfo, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_MODIFY_USER_DATA_RSP, (ProtocolHandler)&CHallLogicHandler::modifyUserInfoReply, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, SERVICE_PLAYER_INVITATION_NOTIFY, (ProtocolHandler)&CHallLogicHandler::playerReceiveInvitationNotify, this);
    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_PLAYER_REFUSE_INVITE_NOTIFY, (ProtocolHandler)&CHallLogicHandler::refusePlayerInvitationNotify, this);

    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_GET_GAME_RECORD_INFO_REQ, (ProtocolHandler)&CHallLogicHandler::getGameRecord, this);
	m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_GET_GAME_RECORD_INFO_RSP, (ProtocolHandler)&CHallLogicHandler::getGameRecordReply, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_GET_DETAILED_GAME_RECORD_REQ, (ProtocolHandler)&CHallLogicHandler::getDetailedGameRecord, this);
    m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_GET_DETAILED_GAME_RECORD_RSP, (ProtocolHandler)&CHallLogicHandler::getDetailedGameRecordReply, this);

    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_CHECK_USER_PHONE_NUMBER_REQ, (ProtocolHandler)&CHallLogicHandler::checkUserPhoneNumber, this);
    m_msgHandler->registerProtocol(ServiceType::GameHallSrv, SGH_CUSTOM_GET_USER_PHONE_NUMBER, (ProtocolHandler)&CHallLogicHandler::getUserPhoneNumberReply, this);
    m_msgHandler->registerProtocol(ServiceType::HttpOperationSrv, HTTPOPT_CHECK_PHONE_NUMBER_RSP, (ProtocolHandler)&CHallLogicHandler::checkUserPhoneNumberReply, this);

    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_CHECK_MOBILE_CODE_REQ, (ProtocolHandler)&CHallLogicHandler::checkMobileVerificationCode, this);
    m_msgHandler->registerProtocol(ServiceType::DBProxySrv, SGH_CUSTOM_SET_USER_PHONE_NUMBER, (ProtocolHandler)&CHallLogicHandler::setUserPhoneNumberReply, this);

    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_MANAGER_GET_APPLY_JOIN_PLAYERS_REQ, (ProtocolHandler)&CHallLogicHandler::getApplyJoinPlayers, this);
    m_msgHandler->registerProtocol(ServiceType::OperationManageSrv, OPTMGR_GET_APPLY_JOIN_PLAYERS_RSP, (ProtocolHandler)&CHallLogicHandler::getApplyJoinPlayersReply, this);
	
	m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_MANAGER_OPT_HALL_PLAYER_REQ, (ProtocolHandler)&CHallLogicHandler::managerOptHallPlayer, this);
    m_msgHandler->registerProtocol(ServiceType::OperationManageSrv, OPTMGR_OPT_HALL_PLAYER_RSP, (ProtocolHandler)&CHallLogicHandler::managerOptHallPlayerReply, this);

    m_msgHandler->registerProtocol(ServiceType::OutsideClientSrv, CGH_GET_HALL_PLAYER_INFO_REQ, (ProtocolHandler)&CHallLogicHandler::getHallPlayerList, this);
    m_msgHandler->registerProtocol(ServiceType::DBProxySrv, DBPROXY_GET_HALL_PLAYER_LIST_RSP, (ProtocolHandler)&CHallLogicHandler::getHallPlayerListReply, this);

	return SrvOptSuccess;
}

void CHallLogicHandler::unInit()
{
}

void CHallLogicHandler::updateConfig()
{
}

void CHallLogicHandler::onLine(HallUserData* cud, ConnectProxy* conn, const com_protocol::DetailInfo& userDetailInfo)
{
}

void CHallLogicHandler::offLine(HallUserData* cud)
{
    m_setPhoneNumberInfoMap.erase(cud->userName);
}

// 获取房间最大倍率
unsigned int CHallLogicHandler::getCardMaxRate(const com_protocol::CattleRoomInfo& cattleRoomInfo)
{
	const NCattlesBaseConfig::CattlesBaseConfig& ctlBaseCfg = m_msgHandler->getSrvOpt().getCattlesBaseCfg();
	unsigned int specialMaxRate = 1;
	const ::google::protobuf::RepeatedField<int>& selectSpecialType = cattleRoomInfo.special_card_type();
	if (selectSpecialType.size() > 0)
	{
		int maxCardType = 0;
		for (::google::protobuf::RepeatedField<int>::const_iterator it = selectSpecialType.begin(); it != selectSpecialType.end(); ++it)
		{
			if (*it > maxCardType) maxCardType = *it;
		}
		
		map<int, unsigned int>::const_iterator spRtIt = ctlBaseCfg.special_card_rate.find(maxCardType);
		if (spRtIt != ctlBaseCfg.special_card_rate.end()) specialMaxRate = spRtIt->second;
	}

	// 基础倍率的最大倍率为牛牛
	unsigned int baseMaxRate = 1;
	const unsigned int rateIndex = cattleRoomInfo.rate_rule_index();
	const map<int, unsigned int>& baseRate = ctlBaseCfg.rate_rule_cfg[rateIndex].cattles_value_rate;
	map<int, unsigned int>::const_iterator baseRtIt = baseRate.find(com_protocol::ECattleBaseCardType::ECattleCattle);
	if (baseRtIt != baseRate.end()) baseMaxRate = baseRtIt->second;
	
	return (specialMaxRate > baseMaxRate) ? specialMaxRate : baseMaxRate;
}

unsigned int CHallLogicHandler::getEnterRoomMinGold(const com_protocol::CattleRoomInfo& cattleRoomInfo)
{
	// 准入金币=最大底分*游戏人数*牌型最大倍率*1
	// 最大底分是A/B中的B，游戏人数是2或者5
	// 牌型最大倍率是翻倍规则和特殊牌型中最大的翻倍数值，常数暂定为1（预留日后修改）
	const NCattlesBaseConfig::CattlesBaseConfig& ctlBaseCfg = m_msgHandler->getSrvOpt().getCattlesBaseCfg();
	const vector<unsigned int>& betRate = ctlBaseCfg.base_number_cfg.bet_rate;
	const unsigned int maxBetRate = !betRate.empty() ? betRate[betRate.size() - 1] : 1;  // 最大底注倍率
	const unsigned int maxBetValue = maxBetRate * cattleRoomInfo.number_index();

	const unsigned int playerCount = ctlBaseCfg.player_count_cfg[cattleRoomInfo.player_count_index()] - 1;
	const unsigned int maxRate = getCardMaxRate(cattleRoomInfo);
	const double multiplier = ctlBaseCfg.common_cfg.create_room_need_gold_multiplier;  // 乘数因子常量
	
	return (maxBetValue * playerCount * maxRate * multiplier);
}

int CHallLogicHandler::setRoomBaseInfo(const HallUserData* cud, com_protocol::ChessHallRoomInfo& roomInfo)
{
	switch (roomInfo.base_info().game_type())
	{
		case ServiceType::CattlesGame:
		{
			// 参数校验
			const NCattlesBaseConfig::CattlesBaseConfig& ctlBaseCfg = m_msgHandler->getSrvOpt().getCattlesBaseCfg();
			com_protocol::CattleRoomInfo* cattleRoomInfo = roomInfo.mutable_cattle_room();
			if (cattleRoomInfo->number_index() < 1
			    || cattleRoomInfo->number_index() > ctlBaseCfg.base_number_cfg.max_base_bet_value
			    || cattleRoomInfo->rate_rule_index() >= ctlBaseCfg.rate_rule_cfg.size()
				|| cattleRoomInfo->player_count_index() >= ctlBaseCfg.player_count_cfg.size()) return InvalidParameter;
			
			// 金币校验
			const unsigned int enterRoomMinGold = getEnterRoomMinGold(*cattleRoomInfo);
			
			/*
			if (cud->gameGold < enterRoomMinGold || cud->gameGold < cattleRoomInfo->need_gold())
			{
				OptErrorLog("set room info game gold insufficient error, user gold = %.2f, need min gold = %u, input gold = %u",
				cud->gameGold, enterRoomMinGold, cattleRoomInfo->need_gold());

				return GameHallCreateRoomGoldInsufficient;
			}
			*/

			if (cattleRoomInfo->need_gold() < enterRoomMinGold) cattleRoomInfo->set_need_gold(enterRoomMinGold);

			roomInfo.mutable_base_info()->set_player_count(ctlBaseCfg.player_count_cfg[cattleRoomInfo->player_count_index()]);
			roomInfo.mutable_base_info()->set_base_rate(cattleRoomInfo->number_index());
			
			break;
		}
		
		default:
		{
			return GameHallNoFoundGameServiceType;
			break;
		}
	}
	
	return SrvOptSuccess;
}

void CHallLogicHandler::applyJoinChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	m_msgHandler->getSrvOpt().sendMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv,
                                               data, len, DBPROXY_APPLY_JOIN_CHESS_HALL_REQ);
}

void CHallLogicHandler::applyJoinChessHallReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "player apply join chess hall reply");
	if (connProxy == NULL) return; // 玩家已经离线了

	m_msgHandler->sendMsgToProxy(data, len, CGH_APPLY_JOIN_CHESS_HALL_RSP, connProxy);
	
	// 成功则同步通知在线的棋牌室管理员
	com_protocol::ClientApplyJoinChessHallRsp rsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(rsp, data, len, "player apply join chess hall notify")
		|| rsp.result() != SrvOptSuccess) return;

	// 申请加入棋牌室成功则同步通知在线的B端、C端棋牌室管理员
	const HallUserData* cud = (HallUserData*)m_msgHandler->getProxyUserData(connProxy);
	com_protocol::PlayerApplyJoinChessHallNotify notify;
	com_protocol::StaticInfo* staticInfo = notify.mutable_static_info();
	staticInfo->set_username(cud->userName);
	staticInfo->set_nickname(cud->nickname);
	staticInfo->set_portrait_id(cud->portraitId);
	staticInfo->set_gender(cud->gender);

	notify.set_allocated_hall_id(rsp.release_hall_id());
	if (rsp.has_explain_msg()) notify.set_allocated_explain_msg(rsp.release_explain_msg());
	
	m_msgHandler->sendApplyJoinHallNotify(cud, rsp.managers(), notify);
}

void CHallLogicHandler::cancelApplyChessHall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	m_msgHandler->getSrvOpt().sendMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv,
                                               data, len, DBPROXY_CANCEL_APPLY_CHESS_HALL_REQ);
}

void CHallLogicHandler::cancelApplyChessHallReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "player cancel apply chess hall reply");
	if (connProxy != NULL) m_msgHandler->sendMsgToProxy(data, len, CGH_CANCEL_APPLY_CHESS_HALL_RSP, connProxy);
}

// 通知C端玩家用户，在棋牌室的状态变更
void CHallLogicHandler::chessHallPlayerStatusNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "chess hall player status notify");
	if (connProxy != NULL)
	{
		com_protocol::ClientChessHallPlayerStatusNotify playerStatusNotify;
	    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(playerStatusNotify, data, len, "chess hall player status notify")) return;
	
        int rc = SrvOptFailed;
		HallUserData* cud = (HallUserData*)m_msgHandler->getProxyUserData(connProxy);
        if (strcmp(cud->hallId, playerStatusNotify.hall_id().c_str()) == 0)
        {
            cud->playerStatus = playerStatusNotify.status();

            // 更新会话数据
            SessionKeyData sessionKeyData;
            rc = m_msgHandler->setSessionKeyData(connProxy, cud, cud->hallId, cud->playerStatus, sessionKeyData, cud->userName);
            
            // com_protocol::EHallPlayerStatus::ECheckAgreedPlayer
            // 如果是审核通过的玩家则同步玩家的状态到消息中心，以便玩家可以接收到其他玩家创建房间等操作更新
        }

        m_msgHandler->sendMsgToProxy(data, len, CGH_CHESS_HALL_PLAYER_STATUS_NOTIFY, connProxy);
		OptInfoLog("chess hall player status notify, username = %s, hallId = %s, notify hallId = %s, status = %d, rc = %d",
		userName, cud->hallId, playerStatusNotify.hall_id().c_str(), playerStatusNotify.status(), rc);
		
		if (cud->playerStatus == com_protocol::EHallPlayerStatus::EForbidPlayer)
		{
			m_msgHandler->removeConnectProxy(cud->userName, com_protocol::EUserQuitStatus::EUserBeForceOffline);
		}
	}
}

// 服务器（棋牌室管理员）主动通知玩家退出棋牌室（从棋牌室踢出玩家）
void CHallLogicHandler::playerQuitChessHallNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "player quit chess hall notify");
	if (connProxy != NULL)
	{
		m_msgHandler->sendMsgToProxy(data, len, CGH_PLAYER_QUIT_CHESS_HALL_NOTIFY, connProxy);

		OptWarnLog("manager let player quit chess hall notify, username = %s", userName);
		
	    m_msgHandler->removeConnectProxy(userName, com_protocol::EUserQuitStatus::EUserBeForceOffline);  // 删除关闭与客户端的连接
	}
}

// 玩家创建棋牌室游戏房间
void CHallLogicHandler::createHallGameRoom(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientCreateHallGameRoomReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "create hall game room request")) return;
	
	const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	com_protocol::ClientCreateHallGameRoomRsp rsp;
	do
	{
		// 参数校验
		if (cud->playerStatus != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer)
		{
			rsp.set_result(GameHallCreateRoomNoCheckAgreedPlayer);
			break;
		}
        
        if (req.room_info().base_info().hall_id() != cud->hallId || !req.room_info().base_info().has_room_type())
		{
			OptErrorLog("create hall game room param error, username = %s, hallId = %s, status = %d, pass hall id = %s, has room type = %d",
			cud->userName, cud->hallId, cud->playerStatus, req.room_info().base_info().hall_id().c_str(), req.room_info().base_info().has_room_type());
			
			rsp.set_result(GameHallCreateGameRoomParamInvalid);
			break;
		}

		if (m_msgHandler->getSrvOpt().getServiceCommonCfg().game_info_map.find(req.room_info().base_info().game_type()) == m_msgHandler->getSrvOpt().getServiceCommonCfg().game_info_map.end())
		{
			rsp.set_result(GameHallNoFoundGameInfo);
			break;
		}
		
		rsp.set_result(setRoomBaseInfo(cud, *req.mutable_room_info()));

	} while (false);

    int rc = SrvOptFailed;
    if (rsp.result() == SrvOptSuccess)
	{
        rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
		                                                   DBPROXY_CREATE_HALL_GAME_ROOM_REQ, "create hall game room request");
	}
	else
	{
		rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, CGH_CREATE_HALL_GAME_ROOM_RSP, "create hall game room error reply");
	}
	
	const com_protocol::ChessHallRoomBaseInfo& logBaseInfo = req.room_info().base_info();
	OptInfoLog("player create hall game room, username = %s, hall id = %s, game type = %u, room type = %u, base rate = %u, player count = %u, result = %d, send msg rc = %d",
	cud->userName, logBaseInfo.hall_id().c_str(), logBaseInfo.game_type(), logBaseInfo.room_type(), logBaseInfo.base_rate(), logBaseInfo.player_count(), rsp.result(), rc);
}

void CHallLogicHandler::createHallGameRoomReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "player create hall game room reply");
	if (connProxy != NULL) m_msgHandler->sendMsgToProxy(data, len, CGH_CREATE_HALL_GAME_ROOM_RSP, connProxy);
}

// 获取商城信息
void CHallLogicHandler::getMallInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	com_protocol::ClientGetMallInfoRsp rsp;
	rsp.set_result(SrvOptSuccess);
	
	const vector<MallConfigData::MallGoodsCfg>& mallGoodCfg = m_msgHandler->getSrvOpt().getMallCfg().mall_good_array;
	for (vector<MallConfigData::MallGoodsCfg>::const_iterator it = mallGoodCfg.begin(); it != mallGoodCfg.end(); ++it)
	{
		com_protocol::Goods* buyGoods = rsp.add_mall_info()->mutable_buy_goods();
		buyGoods->set_type(it->type);
		buyGoods->set_price(it->price);
	}
	
	int rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, CGH_GET_MALL_DATA_RSP, "get hall info reply");
	
	OptInfoLog("get hall info reply, username = %s, result = %d, send msg rc = %d", cud->userName, rsp.result(), rc);
}

// 玩家购买商城物品
void CHallLogicHandler::buyMallGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (cud->playerStatus != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer) return;
	
    com_protocol::ClientBuyMallGoodsReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "buy mall goods request")) return;

    req.set_hall_id(cud->hallId);
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
	                                              DBPROXY_PLAYER_BUY_MALL_GOODS_REQ, "buy mall goods request");
}

void CHallLogicHandler::buyMallGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientBuyMallGoodsRsp rsp;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(rsp, data, len, "buy mall goods reply")) return;
	
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "buy mall goods reply");
	if (connProxy != NULL) m_msgHandler->sendMsgToProxy(data, len, CGH_BUY_MALL_GOODS_RSP, connProxy);
	
	if (rsp.result() == SrvOptSuccess)
	{
	    // 成功则通知在线的棋牌室管理员
		com_protocol::BSPlayerBuyMallGoodsNotify plBuyGoodsNtf;
		plBuyGoodsNtf.set_allocated_hall_id(rsp.release_hall_id());
		plBuyGoodsNtf.set_allocated_buy_goods(rsp.release_buy_goods());
		plBuyGoodsNtf.set_allocated_gold_info(rsp.release_gold_info());

		m_msgHandler->getSrvOpt().sendPkgMsgToService(userName, m_msgHandler->getContext().userDataLen, ServiceType::OperationManageSrv,
		plBuyGoodsNtf, OPTMGR_PLAYER_BUY_MALL_GOODS_NOTIFY, "buy mall goods notify");
	}
}

// 玩家获取商城购买记录
void CHallLogicHandler::getMallBuyRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (cud->playerStatus != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer) return;
	
    com_protocol::ClientGetBuyMallGoodsRecordReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get mall buy record request")) return;

    req.set_hall_id(cud->hallId);
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
	                                              DBPROXY_PLAYER_GET_MALL_BUY_RECORD_REQ, "get mall buy record request");
}

void CHallLogicHandler::getMallBuyRecordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "player get mall buy record");
	if (connProxy != NULL) m_msgHandler->sendMsgToProxy(data, len, CGH_GET_MALL_BUY_RECORD_RSP, connProxy);
}


void CHallLogicHandler::playerChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (*cud->hallId == '\0') return;

    const ServiceCommonConfig::PlayerChatCfg& chatCfg = m_msgHandler->getSrvOpt().getServiceCommonCfg().player_chat_cfg;
    const unsigned int curSecs = time(NULL);
	SUserChatInfo& userChatInfo = m_msgHandler->getUserChatInfo(cud->userName);
	if ((curSecs - userChatInfo.chatSecs) > chatCfg.interval_secs)
	{
		userChatInfo.chatSecs = curSecs;
		userChatInfo.chatCount = 0;
	}
	
	if (userChatInfo.chatCount >= chatCfg.chat_count)
	{
		// OptWarnLog("chess hall player send chat message frequently, username = %s, config interval = %u, count = %u, current count = %u",
		// cud->userName, chatCfg.interval_secs, chatCfg.chat_count, userChatInfo.chatCount);

		return;
	}

	com_protocol::ClientSndMsgInfoNotify smiNtf;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(smiNtf, data, len, "hall chat notify")) return;
	
	if (smiNtf.msg_info().msg_content().length() > chatCfg.chat_content_length) return;
	
	++userChatInfo.chatCount;

	com_protocol::StaticInfo* usrStaticInfo = smiNtf.mutable_msg_info()->mutable_player_info();
	usrStaticInfo->set_username(cud->userName);
	usrStaticInfo->set_nickname(cud->nickname);
	usrStaticInfo->set_portrait_id(cud->portraitId);
	usrStaticInfo->set_gender(cud->gender);

    // 转发消息给棋牌室所有在线用户
	const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(smiNtf, "hall player chat notify");
	if (msgData != NULL) m_msgHandler->sendMsgDataToAllHallUser(msgData, smiNtf.ByteSize(), cud->hallId, CGH_PLAYER_SEND_CHAT_MSG_NOTIFY);
	
	// 缓存消息
	MessageInfoList& msgInfoList = m_hallMsgInfoMap[cud->hallId];
	msgInfoList.push_back(smiNtf.msg_info());
	if (msgInfoList.size() > m_msgHandler->m_pCfg->common_cfg.cache_message_count) msgInfoList.pop_front();
}

void CHallLogicHandler::gameMsgNoticeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientSndMsgInfoNotify smiNtf;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(smiNtf, data, len, "game notice notify")) return;

    m_msgHandler->sendMsgDataToAllHallUser(data, len, smiNtf.msg_info().hall_id().c_str(), CGH_SERVICE_PUSH_MSG_NOTIFY);
	
	// 缓存消息
	MessageInfoList& msgInfoList = m_hallMsgInfoMap[smiNtf.msg_info().hall_id()];
	msgInfoList.push_back(smiNtf.msg_info());
	if (msgInfoList.size() > m_msgHandler->m_pCfg->common_cfg.cache_message_count) msgInfoList.pop_front();
}

void CHallLogicHandler::getHallMsgInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (*cud->hallId == '\0') return;
	
	const MessageInfoList& msgInfoList = m_hallMsgInfoMap[cud->hallId];
	com_protocol::ClientGetMsgInfoRsp rsp;
	for (MessageInfoList::const_iterator it = msgInfoList.begin(); it != msgInfoList.end(); ++it)
	{
		*rsp.add_msg_infos() = *it;
	}
	
	int rc = m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, CGH_GET_HALL_MESSAGE_RSP, "get hall msg info reply");
	
	OptInfoLog("get hall msg info reply, username = %s, hall id = %s, msg count = %u, send msg rc = %d",
	cud->userName, cud->hallId, (unsigned int)msgInfoList.size(), rc);
}

void CHallLogicHandler::modifyUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (cud->playerStatus != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer) return;

	com_protocol::ClientModifyUserDataReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "modify user info request")
	    || req.static_info().username() != cud->userName) return;  // 只能修改自己的信息

	req.set_hall_id(cud->hallId);
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
								                  DBPROXY_MODIFY_USER_DATA_REQ, "modify user info request");
}

void CHallLogicHandler::modifyUserInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "modify user info reply");
	if (connProxy != NULL)
	{
		com_protocol::ClientModifyUserDataRsp rsp;
	    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(rsp, data, len, "modify user info reply")) return;
		
		m_msgHandler->sendMsgToProxy(data, len, CGH_MODIFY_USER_DATA_RSP, connProxy);
	
		if (rsp.result() == SrvOptSuccess)
		{
			HallUserData* cud = (HallUserData*)m_msgHandler->getProxyUserData(connProxy);
		    const com_protocol::StaticInfo& staticInfo = rsp.static_info();
			if (staticInfo.has_nickname()) strncpy(cud->nickname, staticInfo.nickname().c_str(), IdMaxLen - 1);
	        if (staticInfo.has_gender()) cud->gender = staticInfo.gender();
		}
	}
}


// 玩家收到邀请一起游戏的通知
void CHallLogicHandler::playerReceiveInvitationNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* conn = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "receive invitation notify");
	if (conn == NULL) return;      // 被邀请的玩家离线了

    int rc = m_msgHandler->sendMsgToProxy(data, len, CGH_INVITE_PLAYER_PLAY_NOTIFY, conn);
	OptInfoLog("receive invitation notify, usrename = %s, rc = %d", userName, rc);
}

// 拒绝玩家邀请通知
void CHallLogicHandler::refusePlayerInvitationNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientPlayerRefuseInviteNotify ntf;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(ntf, data, len, "refuse player invitation notify")) return;

	HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	com_protocol::StaticInfo* staticInfo = ntf.mutable_static_info();
	staticInfo->set_username(cud->userName);
	staticInfo->set_nickname(cud->nickname);
	staticInfo->set_portrait_id(cud->portraitId);
	staticInfo->set_gender(cud->gender);

	m_msgHandler->getSrvOpt().forwardMessage(ntf.username().c_str(), data, len, ntf.service_id() / ServiceIdBaseValue,
											 SERVICE_REFUSE_INVITATION_NOTIFY, m_msgHandler->getSrvId(),
											 0, "refuse player invitation notify");
}

void CHallLogicHandler::getGameRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (cud->playerStatus != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer) return;
	
    com_protocol::ClientGetGameRecordReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get game record request")) return;

    req.set_hall_id(cud->hallId);
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
	                                              DBPROXY_GET_GAME_RECORD_INFO_REQ, "get game record request");
}

void CHallLogicHandler::getGameRecordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "get game record reply");
	if (connProxy != NULL) m_msgHandler->sendMsgToProxy(data, len, CGH_GET_GAME_RECORD_INFO_RSP, connProxy); 
}

void CHallLogicHandler::getDetailedGameRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (cud->playerStatus != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer) return;
	
    com_protocol::ClientGetDetailedGameRecordReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get detailed game record request")) return;

    req.set_hall_id(cud->hallId);
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
	                                              DBPROXY_GET_DETAILED_GAME_RECORD_REQ, "get detailed game record request");
}

void CHallLogicHandler::getDetailedGameRecordReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "get detailed game record reply");
	if (connProxy != NULL) m_msgHandler->sendMsgToProxy(data, len, CGH_GET_DETAILED_GAME_RECORD_RSP, connProxy); 
}


void CHallLogicHandler::checkUserPhoneNumber(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (cud->playerStatus != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer) return;
    
    com_protocol::ClientCheckPhoneNumberReq req;
    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "check user phone number request")) return;
    
    // 收信人手机号，如1xxxxxxxxxx，格式必须为11位
    const unsigned int phoneNumberLen = 11;
    if (req.phone_number().length() != phoneNumberLen || *req.phone_number().c_str() != '1'
        || !NCommon::strIsDigit(req.phone_number().c_str(), req.phone_number().length())) return;

    UserIdToPhoneNumberDataMap::iterator it = m_setPhoneNumberInfoMap.find(cud->userName);
    if (it != m_setPhoneNumberInfoMap.end())
    {
        // 每个手机号码每分钟只能接收 1 条短信
        const unsigned int setInterval = m_msgHandler->m_pCfg->common_cfg.set_phone_number_interval;
        const unsigned int minuteSecs = (setInterval < 1) ? MinuteSecondUnit : (setInterval * MinuteSecondUnit);  // 秒数
        if ((time(NULL) - it->second.timeSecs) < minuteSecs) return;
        
        it->second.timeSecs = time(NULL);
        it->second.number = req.phone_number();
    }
    else
    {
        // 此数据在用户退出游戏时才删除，得一直保留作为时间间隔判断
        m_setPhoneNumberInfoMap[cud->userName] = SSetPhoneNumberData(time(NULL), req.phone_number(), 0);
    }

    // 先获取玩家当前的手机号码
    com_protocol::GetUserAttributeValueReq getPhoneNumberReq;
    getPhoneNumberReq.set_hall_id(cud->hallId);
    getPhoneNumberReq.set_username(cud->userName);
    com_protocol::UserAttributeInfo* attributeInfo = getPhoneNumberReq.mutable_attribute_info();
    attributeInfo->add_attributes()->set_type(com_protocol::UserAttributeType::EUAPhoneNumber);

	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, getPhoneNumberReq,
								                  DBPROXY_GET_USER_ATTRIBUTE_VALUE_REQ, "get user phone number request", 0,
                                                  SGH_CUSTOM_GET_USER_PHONE_NUMBER);
}

void CHallLogicHandler::getUserPhoneNumberReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    com_protocol::GetUserAttributeValueRsp getPhoneNumberRsp;
    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(getPhoneNumberRsp, data, len, "get user phone number reply")) return;
    
    const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "get user phone number reply");
	if (connProxy == NULL) return;  // 玩家离线了

    com_protocol::ClientCheckPhoneNumberRsp rsp;
    do
    {
        const HallUserData* cud = (HallUserData*)m_msgHandler->getProxyUserData(connProxy);
        UserIdToPhoneNumberDataMap::iterator it = m_setPhoneNumberInfoMap.find(cud->userName);
    
        if (it == m_setPhoneNumberInfoMap.end())
        {
            rsp.set_result(GameHallNoFoundPhoneNumberInfo);
            break;
        }
        
        if (getPhoneNumberRsp.result() != SrvOptSuccess)
        {
            rsp.set_result(getPhoneNumberRsp.result());
            break;
        }
        
        if (getPhoneNumberRsp.attribute_info().attributes(0).string_value() == it->second.number)
        {
            rsp.set_phone_number(it->second.number);
            rsp.set_result(GameHallAlreadySetPhoneNumber);
            break;
        }
        
        it->second.code = CRandomNumber::getUIntNumber(100001, 999999);  // 随机6位数字
        
        // 给用户手机发送验证码
        com_protocol::ClientCheckPhoneNumberReq ckPhoneNumberReq;
        ckPhoneNumberReq.set_phone_number(it->second.number);
        ckPhoneNumberReq.set_verification_code(it->second.code);
	    m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::HttpOperationSrv, ckPhoneNumberReq,
								                      HTTPOPT_CHECK_PHONE_NUMBER_REQ, "check user phone number request");
        return;
        
    } while (false);
    
    m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, CGH_CHECK_USER_PHONE_NUMBER_RSP, connProxy, "check user phone number error reply");
}

void CHallLogicHandler::checkUserPhoneNumberReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "check user phone number reply");
	if (connProxy != NULL) m_msgHandler->sendMsgToProxy(data, len, CGH_CHECK_USER_PHONE_NUMBER_RSP, connProxy); 
}

void CHallLogicHandler::checkMobileVerificationCode(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (cud->playerStatus != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer) return;
    
    com_protocol::ClientCheckMobileCodeReq req;
    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "check mobile code request")) return;

    com_protocol::ClientCheckMobileCodeRsp rsp;
    do
    {
        UserIdToPhoneNumberDataMap::iterator it = m_setPhoneNumberInfoMap.find(cud->userName);
        if (it == m_setPhoneNumberInfoMap.end())
        {
            rsp.set_result(GameHallNoFoundPhoneNumberInfo);
            break;
        }
        
        if (it->second.code != req.verification_code())
        {
            rsp.set_result(GameHallInvalidPhoneVerificationCode);
            break;
        }
        
        // 向DB修改手机号码
        com_protocol::ClientModifyUserDataReq setPhoneNumberReq;
        setPhoneNumberReq.set_hall_id(cud->hallId);
        com_protocol::StaticInfo* staticInfo = setPhoneNumberReq.mutable_static_info();
        staticInfo->set_username(cud->userName);
        staticInfo->set_mobile_phone(it->second.number);
	
        m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, setPhoneNumberReq,
                                                      DBPROXY_MODIFY_USER_DATA_REQ, "modify user phone number request", 0,
                                                      SGH_CUSTOM_SET_USER_PHONE_NUMBER);
        
        return;

    } while (false);
    
    m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, CGH_CHECK_MOBILE_CODE_RSP, "check mobile code error reply");
}

void CHallLogicHandler::setUserPhoneNumberReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "modify user phone number reply");
	if (connProxy == NULL) return;

    com_protocol::ClientModifyUserDataRsp setPhoneNumberRsp;
    if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(setPhoneNumberRsp, data, len, "modify user phone number reply")) return;

    com_protocol::ClientCheckMobileCodeRsp rsp;
    rsp.set_result(setPhoneNumberRsp.result());
    if (rsp.result() == SrvOptSuccess)
    {
        rsp.set_phone_number(setPhoneNumberRsp.static_info().mobile_phone());

        HallUserData* cud = (HallUserData*)m_msgHandler->getProxyUserData(connProxy);
        UserIdToPhoneNumberDataMap::iterator it = m_setPhoneNumberInfoMap.find(cud->userName);
        if (it != m_setPhoneNumberInfoMap.end()) it->second.code = 0;  // 成功则初始化验证码
    }
    
    m_msgHandler->getSrvOpt().sendMsgPkgToProxy(rsp, CGH_CHECK_MOBILE_CODE_RSP, connProxy, "modify user phone number reply");
}

// 管理员在C端获取申请待审核的玩家列表
void CHallLogicHandler::getApplyJoinPlayers(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (cud->playerStatus != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer) return;
	
    com_protocol::ClientManagerGetApplyJoinPlayersReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get apply join players request")) return;

    req.set_hall_id(cud->hallId);
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::OperationManageSrv, req,
	                                              OPTMGR_GET_APPLY_JOIN_PLAYERS_REQ, "get apply join players request");
}

void CHallLogicHandler::getApplyJoinPlayersReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "get apply join players reply");
	if (connProxy != NULL) m_msgHandler->sendMsgToProxy(data, len, CGH_MANAGER_GET_APPLY_JOIN_PLAYERS_RSP, connProxy);
}

// 管理员在C端操作棋牌室玩家
void CHallLogicHandler::managerOptHallPlayer(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
	if (cud->playerStatus != com_protocol::EHallPlayerStatus::ECheckAgreedPlayer) return;
	
    com_protocol::ClientManagerOptHallPlayerReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "manager opt hall player request")) return;

    req.set_hall_id(cud->hallId);
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::OperationManageSrv, req,
	                                              OPTMGR_OPT_HALL_PLAYER_REQ, "manager opt hall player request");
}

void CHallLogicHandler::managerOptHallPlayerReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "manager opt hall player reply");
	if (connProxy != NULL) m_msgHandler->sendMsgToProxy(data, len, CGH_MANAGER_OPT_HALL_PLAYER_RSP, connProxy);
}

void CHallLogicHandler::getHallPlayerList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    com_protocol::ClientGetHallPlayerInfoReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get hall player list request")) return;

	const HallUserData* cud = (HallUserData*)m_msgHandler->getSrvOpt().getConnectProxyUserData();
    req.set_hall_id(cud->hallId);
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cud->userName, cud->userNameLen, ServiceType::DBProxySrv, req,
	                                              DBPROXY_GET_HALL_PLAYER_LIST_REQ, "get hall player list request");
}

void CHallLogicHandler::getHallPlayerListReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const char* userName = NULL;
	ConnectProxy* connProxy = m_msgHandler->getSrvOpt().getConnectProxyInfo(userName, "get hall player list reply");
	if (connProxy != NULL) m_msgHandler->sendMsgToProxy(data, len, CGH_GET_HALL_PLAYER_INFO_RSP, connProxy);
}

}
