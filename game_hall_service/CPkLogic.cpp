
/* author : zhangyang
 * date : 2016.05.26
 * description : PK场逻辑服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "CPkLogic.h"
#include "CLoginSrv.h"
#include "base/ErrorCode.h"
#include "connect/CSocket.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace HallConfigData;

const uint32_t exchange_rate_min = 60;
const uint32_t exchange_rate_hour = exchange_rate_min * 60;
const uint32_t table_player_max = 4;


//场次引导状态
enum EFieldGuideStatus
{
	EFiledClose = 0,		//场次关闭
	EFiledNotPeople = 1,	//场次没人
	EFiledPeople = 2,		//场次有人
};

namespace NPlatformService
{
	CPkLogic::CPkLogic()
		: m_pMsgHandler(NULL)
		, m_tableID(0)
	{
	}

	CPkLogic::~CPkLogic()
	{
	}
	
	void CPkLogic::load(CSrvMsgHandler* msgHandler)
	{
		m_pMsgHandler = msgHandler;

		m_pMsgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_PK_INFO_REQ, (ProtocolHandler)&CPkLogic::handleClientGetPkPlayInfoReq, this);
		m_pMsgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_GET_PK_RULE_INFO_REQ, (ProtocolHandler)&CPkLogic::handleClientGetPkPlayRuleInfoReq, this);
		m_pMsgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_PK_PLAY_SIGN_UP_REQ, (ProtocolHandler)&CPkLogic::handleClientSignUpReq, this);
		m_pMsgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_PK_PLAY_CANCEL_SIGN_UP_REQ, (ProtocolHandler)&CPkLogic::handleClientCancelSignUpReq, this);
		m_pMsgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_PK_PLAY_READY_REQ, (ProtocolHandler)&CPkLogic::handleClientReadyReq, this);
		m_pMsgHandler->registerProtocol(OutsideClientSrv, LOGINSRV_PK_PLAY_RECONNECTION_REQ, (ProtocolHandler)&CPkLogic::handleClientlReconnectionReq, this);
		m_pMsgHandler->registerProtocol(GameSrvDBProxy, CUSTOM_GET_GAME_DATA_FOR_SIGN_UP, (ProtocolHandler)&CPkLogic::getGameDataForSignUp, this);
		m_pMsgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_USER_PROP_AND_PREVENTION_CHEAT_RSP, (ProtocolHandler)&CPkLogic::getUserPropAndPreventionCheat, this);

		m_pMsgHandler->registerProtocol(ServiceType::CommonSrv, CommonProtocol::LOGINSRV_PK_PLAY_END_NOTIFY, (ProtocolHandler)&CPkLogic::handlePkPlayEndUserQuit, this);
		
		m_pMsgHandler->registerProtocol(ServiceType::BuyuGameSrv, PKServiceProtocol::SERVICE_PLAYER_START_PK_RSP, (ProtocolHandler)&CPkLogic::playerStartPKReply, this);
		m_pMsgHandler->registerProtocol(ServiceType::BuyuGameSrv, PKServiceProtocol::SERVICE_PLAYER_WAIVER_RSP, (ProtocolHandler)&CPkLogic::playerWaiverRsp, this);

		m_pMsgHandler->registerProtocol(ServiceType::BuyuGameSrv, PKServiceProtocol::SERVICE_PLAYER_END_PK_NOTIFY, (ProtocolHandler)&CPkLogic::playEndPkNotify, this);

		//
		m_pMsgHandler->registerProtocol(ServiceType::MessageSrv, MessageSrvProtocol::BUSINESS_GET_PK_PLAY_RANKING_CLEAR_NUM_RSP, (ProtocolHandler)&CPkLogic::getPKPlayRankingClearNumRsp, this);

		m_pMsgHandler->registerProtocol(GameSrvDBProxy, CUSTOM_GET_MORE_HALL_DATA_FOR_UP_RANKING_INFO, (ProtocolHandler)&CPkLogic::getOnLineUserHallDataUpPKRangkingInfo, this);

		//初始化PK场
		initPkPlay();
	}
	
	void CPkLogic::unLoad(CSrvMsgHandler* msgHandler)
	{
	}
	
	void CPkLogic::onLine(ConnectUserData* ud, ConnectProxy* conn)
	{
		auto curTime = time(NULL);

		do 
		{
			auto player = m_playRecord.find(ud->connId);
			if (player == m_playRecord.end())
				break;

			if (curTime >= player->second.playEndTime)
			{
				m_playRecord.erase(player);
				break; 
			}

			com_protocol::ClientPKPlayReconnectionNotify notify;
			//notify.set_ip(player->second.ip);
			//notify.set_port(player->second.port);
			//notify.set_srv_id(player->second.srv_id);
			notify.set_play_id(player->second.play_id);

			//提示断线重连
			const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(notify, "pk play reconnection");
			if (msgBuffer != NULL)
				m_pMsgHandler->sendMsgToProxy(msgBuffer, notify.ByteSize(), LOGINSRV_PK_PLAY_RECONNECTION_NOTIFY, conn);

		} while (false);
	}
	
	void CPkLogic::offLine(ConnectUserData* ud)
	{
		//如果在临时报名列表中
		auto player = m_tempSignUpPlayer.find(ud->connId);
		if (player != m_tempSignUpPlayer.end())
		{
			m_tempSignUpPlayer.erase(player);
			return;
		}

		//快速玩家报名队列
		player = m_quickStartList.find(ud->connId);
		if (player != m_quickStartList.end())
		{
			m_quickStartList.erase(player);
		}

		//查询是否在桌子中
		std::vector<FieldInfo>::iterator field;
		vector<TableInfo>::iterator table;
		if (getTableAndField(ud->connId, table, field))
		{
			//玩家退出(但目前玩家已经开始游戏, 加入到断线重连)
			if (table->nStatus == ETableStatus::StatusPlaying)
				return;

			player = table->playerList.find(ud->connId);
			if (player == table->playerList.end())
				return;

			//删除掉该玩家
			table->playerList.erase(player);

			if(table->playerList.size() == 0)
				field->tableList.erase(table);

			return;
		}		
	}

	void CPkLogic::updateConfig()
	{
	}

	void CPkLogic::handleClientGetPkPlayInfoRsp(const com_protocol::GetUserOtherInfoRsp &userOtherInfoRsp, const std::string &userName)
	{
		com_protocol::ClientGetPkPlayInfoRsp rsp;
		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		int serviceCharge = 0;

		//场次信息
		for (auto it = cfg.pk_play_info.PkPlayInfoList.begin(); it != cfg.pk_play_info.PkPlayInfoList.end(); ++it)
		{
			auto pkPlayInfo = rsp.add_pk_play_info();
			pkPlayInfo->set_play_id(it->play_id);
			pkPlayInfo->set_user_lv(it->user_lv);		//入场需要的用户等级

			FieldInfo* field = getField((EPKFieldID)it->play_id);
			if (field == NULL)
				continue;

			pkPlayInfo->set_open(field->bOpen);
			pkPlayInfo->set_start_time(it->start_time_hour * exchange_rate_hour + it->start_time_min * exchange_rate_min + it->start_time_sec);		//开始时间
			pkPlayInfo->set_end_time(it->end_time_hour * exchange_rate_hour + it->end_time_min * exchange_rate_min + it->end_time_sec);				//结束时间

			pkPlayInfo->set_reward_type(it->reward_type);
			pkPlayInfo->set_reward_min(it->reward_num_min);
			pkPlayInfo->set_reward_max(it->reward_num_max);
			
			for (auto conditionIt = it->condition_list.begin(); conditionIt != it->condition_list.end(); ++conditionIt)
			{
				auto condition = pkPlayInfo->add_condition();
				condition->set_type(conditionIt->first);
				condition->set_num(conditionIt->second);
			}

			for (auto entranceIt = it->entrance_fee.begin(); entranceIt != it->entrance_fee.end(); ++entranceIt)
			{
				auto entrance = pkPlayInfo->add_entrance_fee();
				entrance->set_type(entranceIt->first);
				entrance->set_num(entranceIt->second);

				if (entranceIt->first == PropGold)
				{
					serviceCharge = entrance->num();
					entrance->set_num(entrance->num() + cfg.pk_play_info.gold_max);
				}
			}
			

			uint32_t num = 0;
			for (auto carryPropIt = it->carry_prop.begin(); carryPropIt != it->carry_prop.end(); ++carryPropIt)
			{
				auto propInfo = rsp.add_prop_info();
				auto userPropInfo = propInfo->mutable_have_item();
				userPropInfo->set_type(carryPropIt->first);

				if (!getPropNum<const com_protocol::GetUserOtherInfoRsp>(carryPropIt->first, num, userOtherInfoRsp))
					OptErrorLog("CPkLogic handleClientGetPkPlayInfoRsp, get prop num, prop id:%d", carryPropIt->first);

				userPropInfo->set_num(num);
				propInfo->set_item_num_max(carryPropIt->second);
			}
		}

		rsp.set_service_charge(serviceCharge);

		const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(rsp, "get pk play info");
		if (msgBuffer != NULL)
		{
			m_pMsgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_GET_PK_INFO_RSP, m_pMsgHandler->getConnectProxy(userName));
		}
	}

	void CPkLogic::handleClientSignUpRsp(com_protocol::GetUserPropAndPreventionCheatRsp &db_rsp)
	{
		std::string userName = db_rsp.query_username();
		auto player = m_tempSignUpPlayer.find(userName);
		if (player == m_tempSignUpPlayer.end())
		{
			OptErrorLog("CPkLogic handleClientSignUpRsp, not find sign up user:%s", userName.c_str());
			return;
		}
	
		com_protocol::ClientPKPlaySignUpRsp rsp;
		switch (player->second.nSignUp)
		{
		case SignUpType::SignUp:
			rsp.set_result(signUp(player->second.nQuickStartFieldID, player->second.nSignUp, player->second.lv, userName, db_rsp));
			break;

		case SignUpType::QuickStart:
			rsp.set_result(quickStart(player->second.nQuickStartFieldID, player->second.nSignUp, player->second.lv, userName, db_rsp));
			break;

		default:
			OptErrorLog("CPkLogic handleClientSignUpRsp, sign type:%d", player->second.nSignUp);
			m_tempSignUpPlayer.erase(player);
			return ;
		}
		
		const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(rsp, "handle client sign up rsp");
		if (msgBuffer != NULL)
		{
			m_pMsgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_PK_PLAY_SIGN_UP_RSP, m_pMsgHandler->getConnectProxy(userName));
		}

		m_tempSignUpPlayer.erase(player);
		
		// 玩家操作记录
		char remarks[32] = {0};
		snprintf(remarks, sizeof(remarks) - 1, "%s-%d", (player->second.nSignUp == SignUpType::QuickStart) ? "快速报名" : "入场报名", rsp.result());
		writePlayerOptRecord(userName.c_str(), userName.length(), (rsp.result() == 0) ? EPKPlayerOpt::EOnMatching : EPKPlayerOpt::ESignUpFailed, remarks);
	}

	void CPkLogic::onStartPlay(com_protocol::GetManyUserOtherInfoRsp &rsp)
	{
		if (rsp.query_user_size() <= 0)
		{
			OptErrorLog("CPkLogic onStartPlay, query user size:%d", rsp.query_user_size());
			return;
		}

		std::vector<FieldInfo>::iterator field;
		vector<TableInfo>::iterator table;
		if (!getTableAndField(rsp.query_user(0).query_username(), table, field))
		{
			OptErrorLog("CPkLogic onStartPlay, user:%s, table == null", rsp.query_user(0).query_username().c_str());
			return;
		}


		bool bIsUserExist = false;
		
		uint32_t tableID = ((m_pMsgHandler->getSrvId() & 0xFFFF) << 16) + (++m_tableID & 0xFFFF);

		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		auto pkPlayCfg = getPkPlayCfg(cfg, field->nFieldID);

		com_protocol::ClientPKPlayMatchingFinishNotify notify;

		com_protocol::PlayerPKStartReq  pkStartReq;
		pkStartReq.set_pk_type(field->nFieldID);		

		//Pk场索引
		auto pkItem = notify.add_table_info();
		pkItem->set_key(PkIndexKey);
		pkItem->set_int_value(tableID);
		
		//Pk场类型
		pkItem = notify.add_table_info();
		pkItem->set_key(PkTypeKey);
		pkItem->set_int_value(field->nFieldID);
		bool bOnLine = false;

		for (auto playIt = table->playerList.begin(); playIt != table->playerList.end(); ++playIt)
		{
			bIsUserExist = false;
			for (auto queryIt = rsp.query_user().begin(); queryIt != rsp.query_user().end(); ++queryIt)
			{
				if (playIt->first == queryIt->query_username())
				{
					//取用户信息
					bIsUserExist = true;
					auto player = notify.add_player();
					player->set_nickname(queryIt->other_info(0).string_value());
					player->set_sex(queryIt->other_info(1).int_value());
					player->set_head_portrait(queryIt->other_info(2).int_value());		
					player->set_vip_lv(queryIt->other_info(3).int_value());
					player->set_lv(playIt->second.lv);
					player->set_user_name(playIt->first);

					//Pk场用户
					pkItem = notify.add_table_info();
					pkItem->set_key(PkPlayerKey);
					pkItem->set_str_value(playIt->first);

					//pkStartReq.add_username(playIt->first);

					auto pkPlayer = pkStartReq.add_play_list();
					pkPlayer->set_username(playIt->first);
					pkPlayer->set_nickname(queryIt->other_info(0).string_value());
					pkPlayer->set_game_gold(queryIt->other_info(4).int_value());
					pkPlayer->set_portrait_id(queryIt->other_info(2).int_value());
					pkPlayer->set_vip_lv(queryIt->other_info(3).int_value());	//VIP等级

					auto ticket = playIt->second.ticket.find(field->nFieldID);
					if (ticket == playIt->second.ticket.end())
						pkPlayer->set_ticket(0);
					else
						pkPlayer->set_ticket(ticket->second);

					//取pk场服务信息
					while (!bOnLine)
					{
						ConnectProxy* conn = m_pMsgHandler->getConnectProxy(playIt->first);
						if (conn == NULL)
							break;

						ConnectUserData* ud = (ConnectUserData*)m_pMsgHandler->getProxyUserData(conn);
						com_protocol::HallData hallData;
						if (!m_pMsgHandler->getPKServiceFromRedis(ud, hallData))
							break;

						notify.set_ip(hallData.rooms(0).ip());
						notify.set_port(hallData.rooms(0).port());
						notify.set_srv_id(hallData.rooms(0).id());				//服务ID
						
						bOnLine = true;
						break;
					}
					break;
				}
			}

			//表示数据有错误(在桌子上查不到该用户)
			if (!bIsUserExist)
			{
				OptErrorLog("CPkLogic onStartPlay, data error, user not exist table, user:%s", playIt->first.c_str());
				return;
			}
		}			
		
		//表示没有一个玩家在线
		if (!bOnLine)
		{
			field->tableList.erase(table);
			return;
		}

		//手续费
		NProject::RecordItemVector recordItemVector;
		for (auto it = pkPlayCfg->entrance_fee.begin(); it != pkPlayCfg->entrance_fee.end(); ++it)
		{
			NProject::RecordItem addItem(it->first, -it->second);
			recordItemVector.push_back(addItem);
		}		

		//扣除所需的PK物品
		for (auto it = pkPlayCfg->condition_list.begin(); it != pkPlayCfg->condition_list.end(); ++it)
		{
			NProject::RecordItem addItem(it->first, -it->second);
			recordItemVector.push_back(addItem);
		}
		
		char msgBuffer[NFrame::MaxMsgLen] = {0};
		if (!notify.SerializeToArray(msgBuffer, MaxMsgLen))
		{
			OptErrorLog("CPkLogic onStartPlay, msg byte len = %d, buff len = %d", notify.ByteSize(), MaxMsgLen);
			return ;
		}
	
	    // 生成PK游戏记录ID
	    char pkRecordId[32] = {0};
	    generatePKRecordId(pkRecordId, sizeof(pkRecordId));
		pkStartReq.set_game_record_id(pkRecordId);

		const char* pBuffer = m_pMsgHandler->serializeMsgToBuffer(pkStartReq, "pk play start");
		m_pMsgHandler->sendMessage(pBuffer, pkStartReq.ByteSize(), notify.srv_id(), PKServiceProtocol::SERVICE_PLAYER_START_PK_REQ);
		OptInfoLog("CPkLogic onStartPlay, size:%d, play num:%d", pkStartReq.ByteSize(), rsp.query_user().size());
		NProject::RecordItemVector vecTicket;

		for (auto queryIt = rsp.query_user().begin(); queryIt != rsp.query_user().end(); ++queryIt)
		{
			do 
			{
				vecTicket.clear();
				auto player = table->playerList.find(queryIt->query_username());
				if (player == table->playerList.end())
					break;

				auto fieldTicket = player->second.ticket.find(field->nFieldID);
				if (fieldTicket == player->second.ticket.end())
					break;

				if (fieldTicket->second > 0)
				{
					RecordItem item(fieldTicket->second, -1);
					OptInfoLog("onStartPlay, username:%s, ticket id:%d", queryIt->query_username().c_str(), fieldTicket->second);
					vecTicket.push_back(item);
				}
			} while (false);

			if (vecTicket.size() > 0)
			{
				//扣除门票
				m_pMsgHandler->notifyDbProxyChangeProp(queryIt->query_username().c_str(), (unsigned int)queryIt->query_username().size(),
					vecTicket, EPropFlag::DeductPkPlayEntranceFee, "PK场入场费", NULL, 0, pkRecordId);
			}
			else
			{
				//扣除入场费
				m_pMsgHandler->notifyDbProxyChangeProp(queryIt->query_username().c_str(), (unsigned int)queryIt->query_username().size(),
					recordItemVector, EPropFlag::DeductPkPlayEntranceFee, "PK场入场费", NULL, 0, pkRecordId);
			}	

			//通知给客户端
			m_pMsgHandler->sendMsgToProxy(msgBuffer, notify.ByteSize(), LOGINSRV_PK_PLAY_MATCHING_FINISH_NOTIFY,
				 						  m_pMsgHandler->getConnectProxy(queryIt->query_username()));
		}
		
		//记录游戏开始
		writeRecord(&(*table), notify.ip(), notify.port(), notify.srv_id(), pkPlayCfg->play_time, field->nFieldID);
	
		//清空该桌子
		field->tableList.erase(table);
		
		// 玩家操作记录
		writePlayerOptRecord(rsp.query_user(0).query_username().c_str(), rsp.query_user(0).query_username().length(), EPKPlayerOpt::EBeginPlay, pkRecordId);
	}

	void CPkLogic::onActiveGuide(unsigned int timerId, int userId, void* param, unsigned int remainCount)
	{
		int pkPlay[FieldIDMax] = { EFiledClose };
		
		//筛选需要引导的PK
		for (auto fieldIt = m_field.begin(); fieldIt != m_field.end(); ++fieldIt)
		{
			if(!fieldIt->bOpen)
				continue;
			pkPlay[fieldIt->nFieldID] = (fieldIt->tableList.size() > 0 ? EFiledPeople : EFiledNotPeople);
		}

		int i = 1;
		for (; i < FieldIDMax; ++i)
		{
			if (pkPlay[i - 1] != pkPlay[i])
				break;
		}

		do 
		{
			//表示所有的场次在一个状态
			if (i == FieldIDMax)
			{
				//所有场次都是关闭状态
				if (pkPlay[0] == EFiledClose)
				{
					OptInfoLog("all pk play close!!");
					return;
				}

				//表示所有场次都有人在等待
				if (pkPlay[0] == EFiledPeople)
					break;
			}
			
			//向快速匹配的玩家对列查找需要引导的玩家场次
			for (auto quickStartIt = m_quickStartList.begin(); quickStartIt != m_quickStartList.end(); ++quickStartIt)
			{
				//筛选优先的匹配场次
				if (pkPlay[quickStartIt->second.nQuickStartFieldID] == EFiledNotPeople)
				{
					pkPlay[quickStartIt->second.nQuickStartFieldID] = EFiledPeople;

					//判断是否已经筛选完毕
					if(isFilterFinish(pkPlay, FieldIDMax))
						break;
				}
			}

		} while (false);		
		
		com_protocol::LoginToGameActiveGuideNotify notify;
		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();

		for (int n = 0; n < FieldIDMax; ++n)
		{
			if (pkPlay[n] != EFiledPeople)
				continue;

			const HallConfigData::PkPlay *playCfg = getPkPlayCfg(cfg, n);
			if (playCfg == NULL)
				continue;
						
			auto playCfgInfo = notify.add_cfg();
			playCfgInfo->set_id(n);
			playCfgInfo->set_user_lv(playCfg->user_lv);
			playCfgInfo->set_ticket(playCfg->ticket);

			//要求的物品
			for (auto it = playCfg->condition_list.begin(); it != playCfg->condition_list.end(); ++it)
				m_pMsgHandler->addItemCfg(playCfgInfo, it->first, it->second);
			
			//服务费
			for (auto it = playCfg->entrance_fee.begin(); it != playCfg->entrance_fee.end(); ++it)
				m_pMsgHandler->addItemCfg(playCfgInfo, it->first, it->second);

			//玩家可携带的金币数量
			m_pMsgHandler->addItemCfg(playCfgInfo, EPropType::PropGold, cfg.pk_play_info.gold_max);
		}

		//表示所有场次没有人
		if (notify.cfg_size() <= 0)
			return;

		//发送消息给游戏服通知筛选对应场次的玩家
		vector<int> vecSrv;
		m_pMsgHandler->getAllGameServerID(ServiceType::BuyuGameSrv, vecSrv);
		char msgData[MaxMsgLen] = { 0 };
		notify.SerializeToArray(msgData, MaxMsgLen);
		const bool hasNoGuideService = (cfg.pk_play_info.no_guide_service.size() > 0);
		for (auto srvIt = vecSrv.begin(); srvIt != vecSrv.end(); ++srvIt)
		{
			// 非过滤的服务才会被推送引导消息
			if (!hasNoGuideService || std::find(cfg.pk_play_info.no_guide_service.begin(), cfg.pk_play_info.no_guide_service.end(), *srvIt) == cfg.pk_play_info.no_guide_service.end())
			{
				m_pMsgHandler->sendMessage(msgData, notify.ByteSize(), (*srvIt), LOGINSRV_PK_PLAY_ACTIVE_GUIDE_NOTIFY);
				OptInfoLog("CPkLogic onActiveGuide, send srv id:%d", (*srvIt));
			}
		}
	}

	bool CPkLogic::isFilterFinish(int *pkPlay, int nLen)
	{
		if (pkPlay == NULL)
		{
			OptErrorLog("CPkLogic::isFilterFinish, pkPlay == NULL");
			return true;
		}
		
		for (int i = 0; i < nLen; ++i)
		{
			if (pkPlay[i] == EFiledNotPeople)
				return false;
		}

		return true;
	}

	void CPkLogic::handlePkPlayEndUserQuit(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		//删除游戏记录
		auto player = m_playRecord.find(m_pMsgHandler->getContext().userData);
		if (player != m_playRecord.end())
			m_playRecord.erase(player);

		com_protocol::PkPlayEndUserQuitReq req;
		if (!m_pMsgHandler->parseMsgFromBuffer(req, data, len, "handle pk play end user quit"))
		{
			OptErrorLog("CPkLogic handlePkPlayEndUserQuit, pk play end user quit req");
			return;
		}
	
		for (auto userNameIt = req.username_lst().begin(); userNameIt != req.username_lst().end(); ++userNameIt)
		{
			player = m_playRecord.find(*userNameIt);
			if (player != m_playRecord.end())
				m_playRecord.erase(player);
		}
	}

	void CPkLogic::getUserPropAndPreventionCheat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		com_protocol::GetUserPropAndPreventionCheatRsp db_rsp;

		if (!m_pMsgHandler->parseMsgFromBuffer(db_rsp, data, len, "get user prop and prevention cheat") || db_rsp.result() != 0)
		{
			OptErrorLog("CPkLogic getUserPropAndPreventionCheat, get user prop and prevention cheat result:%d", db_rsp.result());
			return;
		}

		handleClientSignUpRsp(db_rsp);
	}

	void CPkLogic::initPkPlay()
	{
		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		FieldInfo field;
		uint32_t timeCur = getCurTimeSec();		
		uint32_t timeStart = 0;					//场次开启时间
		uint32_t timeEnd = 0;					//场次结束时间
		uint32_t nTime = 0;

		for (auto matchingLvCfg = cfg.pk_play_info.matchingLvList.begin(); matchingLvCfg != cfg.pk_play_info.matchingLvList.end(); ++matchingLvCfg)
		{
			const HallConfigData::PkPlay* pkPlayCfg = getPkPlayCfg(cfg, matchingLvCfg->play_id);
			if (pkPlayCfg == NULL)
				continue;

			timeStart = pkPlayCfg->start_time_hour * exchange_rate_hour + pkPlayCfg->start_time_min * exchange_rate_min + pkPlayCfg->start_time_sec;
			timeEnd = pkPlayCfg->end_time_hour * exchange_rate_hour + pkPlayCfg->end_time_min * exchange_rate_min + pkPlayCfg->end_time_sec;

			if (timeStart > timeEnd)
			{
				OptErrorLog("CPkLogic initPkPlay, pk play config time!! timeStart:%d, timeEnd:%d", timeStart, timeEnd);
				continue;
			}

			//场次已经开始
			if (timeStart <= timeCur && timeCur < timeEnd)
			{
				field.bOpen = true;
				nTime = timeEnd - timeCur;				//计算关闭时间
				m_pMsgHandler->setTimer(nTime * 1000, (TimerHandler)&CPkLogic::onClosePKPlay, matchingLvCfg->play_id, NULL, 1, this);
			}
			else
			{
				field.bOpen = false;

				//计算打开时间
				if (timeCur < timeStart)
					nTime = timeStart - timeCur;				
				else
					nTime = 24 * exchange_rate_hour - timeEnd + timeStart;
				
				m_pMsgHandler->setTimer(nTime * 1000, (TimerHandler)&CPkLogic::onOpenPKPlay, matchingLvCfg->play_id, NULL, 1, this);
			}

			field.nFieldID = (EPKFieldID)matchingLvCfg->play_id;
			m_field.push_back(field);
		}

		//开启匹配定时器
		m_pMsgHandler->setTimer(cfg.pk_play_info.matching_time * 1000, (TimerHandler)&CPkLogic::matchingPlayer, 0, NULL, 1, this);

		//开启定时引导
		m_pMsgHandler->setTimer(cfg.pk_play_info.guide_time * 1000, (TimerHandler)&CPkLogic::onActiveGuide, 0, NULL, -1, this);

	}

	void CPkLogic::onOpenPKPlay(unsigned int timerId, int userId, void* param, unsigned int remainCount)
	{
		auto field = getField((EPKFieldID)userId);
		if (field == NULL)
		{
			OptErrorLog("CPkLogic onOpenPKPlay, field == NULL, PK Field ID:%d", userId);
			return;
		}
		
		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		auto pkPlayCfg = getPkPlayCfg(cfg, field->nFieldID);
		if (pkPlayCfg == NULL)
		{
			OptErrorLog("CPkLogic onOpenPKPlay, pkPlayCfg == NULL, PK Field ID:%d", field->nFieldID);
			return;
		}
			

		//结束时间
		uint32_t timeEnd = pkPlayCfg->end_time_hour * exchange_rate_hour + pkPlayCfg->end_time_min * exchange_rate_min + pkPlayCfg->end_time_sec;
				
		//定时关闭
		uint32_t nTime = timeEnd - getCurTimeSec();
		m_pMsgHandler->setTimer(nTime * 1000, (TimerHandler)&CPkLogic::onClosePKPlay, field->nFieldID, NULL, 1, this);

		//打开场次
		field->bOpen = true;
		field->tableList.clear();

		//通知所有玩家PK场打开
		notifyClientPKPlayStatus(field->nFieldID, field->bOpen);
	}

	void CPkLogic::onClosePKPlay(unsigned int timerId, int userId, void* param, unsigned int remainCount)
	{
		auto field = getField((EPKFieldID)userId);
		if (field == NULL)
		{
			OptErrorLog("CPkLogic onClosePKPlay, field == NULL, PK Field ID:%d", userId);
			return;
		}

		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		auto pkPlayCfg = getPkPlayCfg(cfg, field->nFieldID);
		if (pkPlayCfg == NULL)
		{
			OptErrorLog("CPkLogic onClosePKPlay, pkPlayCfg == NULL, PK Field ID:%d", field->nFieldID);
			return;
		}

		uint32_t timeStart = pkPlayCfg->start_time_hour * exchange_rate_hour + pkPlayCfg->start_time_min * exchange_rate_min + pkPlayCfg->start_time_sec;
		uint32_t timeEnd = pkPlayCfg->end_time_hour * exchange_rate_hour + pkPlayCfg->end_time_min * exchange_rate_min + pkPlayCfg->end_time_sec;
		uint32_t nTime = 24 * exchange_rate_hour - timeEnd + timeStart;

		//定时开启
		m_pMsgHandler->setTimer(nTime * 1000, (TimerHandler)&CPkLogic::onOpenPKPlay, field->nFieldID, NULL, 1, this);

		//场次关闭
		field->bOpen = false;
		field->tableList.clear();

		//通知所有玩家PK场关闭
		notifyClientPKPlayStatus(field->nFieldID, field->bOpen);		
	}

	void CPkLogic::handleClientGetPkPlayInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, 
												unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		if (!m_pMsgHandler->checkLoginIsSuccess("receive not login successs message to get pk play info ")) return;

		ConnectUserData* ud = (ConnectUserData*)m_pMsgHandler->getProxyUserData(m_pMsgHandler->getConnectProxyContext().conn);
		if (ud == NULL)
		{
			OptErrorLog("CPkLogic handleClientGetPkPlayInfoReq, ud == NULL");
			return;
		}

		//查询用户所有的道具数量
		com_protocol::GetUserOtherInfoReq db_req;
		char send_data[MaxMsgLen] = { 0 };
		db_req.set_query_username(ud->connId);	

		for (int i = 1; i < PropMax; ++i)
			db_req.add_info_flag(i);
						
		db_req.SerializeToArray(send_data, MaxMsgLen);
		m_pMsgHandler->sendMessageToService(NFrame::ServiceType::DBProxyCommonSrv, send_data, db_req.ByteSize(), BUSINESS_GET_USER_OTHER_INFO_REQ, 
										   ud->connId, ud->connIdLen, GetUserInfoFlag::GetUserAllPropToPKPlayInfoReq);
	}

	void CPkLogic::handleClientGetPkPlayRuleInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, 
													unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		if (!m_pMsgHandler->checkLoginIsSuccess("receive not login successs message to get pk play rule info req ")) return;

		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		
		com_protocol::ClientGetPKPlayRuleInfoRsp rsp;
		rsp.set_rule(cfg.pk_play_info.play_rule);

		const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(rsp, "get pk play rule info req");
		if (msgBuffer != NULL)
			m_pMsgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_GET_PK_RULE_INFO_RSP);
	}

	void CPkLogic::handleClientSignUpReq(const char* data, const unsigned int len, unsigned int srcSrvId, 
										 unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		if (!m_pMsgHandler->checkLoginIsSuccess("receive not login successs message to sign up req")) return;

		com_protocol::ClientPKPlaySignUpReq req;
		if (!m_pMsgHandler->parseMsgFromBuffer(req, data, len, "handle sign up req")) return;
		
		ConnectUserData* ud = (ConnectUserData*)m_pMsgHandler->getProxyUserData(m_pMsgHandler->getConnectProxyContext().conn);
		if (ud == NULL)
		{
			OptErrorLog("CPkLogic handleClientSignUpReq, ud == NULL");
			return;
		}
		
		com_protocol::ClientPKPlaySignUpRsp rsp;

		do 
		{
			//判断是否之前已报名过
			if (isUserSignUp(ud->connId))
			{
				rsp.set_result(ProtocolReturnCode::UserAlreadySignUp);
				break;
			}

			//是否在游戏中
			auto playRecordIt = m_playRecord.find(ud->connId);
			if (playRecordIt != m_playRecord.end())
			{
				if (time(NULL) < playRecordIt->second.playEndTime)
				{
					rsp.set_result(ProtocolReturnCode::UserPlaying);
					break;
				}
				
				m_playRecord.erase(playRecordIt);
			}

			//报名类型
			if (req.sign_up() != SignUpType::SignUp && req.sign_up() != SignUpType::QuickStart)
			{
				rsp.set_result(ProtocolReturnCode::SignUpTypeError);
				break;
			}
					
			if (req.sign_up() == SignUpType::SignUp)
			{
				auto signUpField = getField((EPKFieldID)req.play_id());
				if (signUpField == NULL)
				{
					OptErrorLog("CPkLogic signUp, signUpField == NULL, play id:%d", req.play_id());
					rsp.set_result(ProtocolReturnCode::NotFindSignUpPKPlay);
					break;
				}

				if (!signUpField->bOpen)
				{
					rsp.set_result(ProtocolReturnCode::PKPlayAlreadyCloser);
					break;
				}
			}

			if (req.sign_up() == SignUpType::QuickStart)
			{
				bool bFlag = false;	//如果有一个场次开 该标志为true
				for (auto it = m_field.begin(); it != m_field.end(); ++it)
				{
					if (it->bOpen)
					{
						bFlag = true;
						break;
					}
				}

				if (!bFlag)
				{
					rsp.set_result(ProtocolReturnCode::PKPlayAlreadyCloser);
					break;
				}
			}

			//向game_db查询数据
			com_protocol::GetGameDataReq gameLogicReq;
			gameLogicReq.set_primary_key(NProject::BuyuLogicDataKey);
			gameLogicReq.set_second_key(ud->connId, ud->connIdLen);
			const char* msgData = m_pMsgHandler->serializeMsgToBuffer(gameLogicReq, "handle client sign up req get game logic data request");
			if (msgData == NULL || Success != m_pMsgHandler->sendMessageToGameDbProxy(msgData, gameLogicReq.ByteSize(), GET_GAME_DATA_REQ, ud->connId, ud->connIdLen, EServiceReplyProtocolId::CUSTOM_GET_GAME_DATA_FOR_SIGN_UP))
			{
				OptErrorLog("CHallLogic handleQuitRetainReq, send to game db");
			}
				
			//临时存储报名选手信息
			EPlayeStatus status = (req.sign_up() == SignUpType::QuickStart ? EPlayeStatus::StatusNotReady : EPlayeStatus::StatusReady);
			
			//如果没有优先场次，则默认为配置文件中优先匹配场次
			const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
			EPKFieldID firstPKPlayId = cfg.pk_play_info.matchingLvList.size() <= 0 ? NProject::FieldIDPhoneFare : static_cast<EPKFieldID>(cfg.pk_play_info.matchingLvList.begin()->play_id);
			EPKFieldID nFieldID = req.has_play_id() ? static_cast<EPKFieldID>(req.play_id()) : firstPKPlayId;
			
			PlayerInfo player((SignUpType)req.sign_up(), nFieldID, status, 0);
			m_tempSignUpPlayer.insert(PLAYER_VALUE(ud->connId, player));
			
			// 玩家操作记录
			char remarks[32] = {0};
			snprintf(remarks, sizeof(remarks) - 1, "%d", nFieldID);
		    writePlayerOptRecord(ud->connId, ud->connIdLen, (req.sign_up() == SignUpType::QuickStart) ? EPKPlayerOpt::EFastSignUp : EPKPlayerOpt::EEntranceSignUp, remarks);

			return;

		} while (false);				
						
		const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(rsp, "sign up req");
		if (msgBuffer != NULL)
			m_pMsgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_PK_PLAY_SIGN_UP_RSP);
	}

	void CPkLogic::handleClientCancelSignUpReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, 
											   unsigned short srcProtocolId)
	{
		com_protocol::ClientPKPlayCancelSignUpReq req;
		if (!m_pMsgHandler->parseMsgFromBuffer(req, data, len, "handle client cancel sign up req")) return;

		ConnectUserData* ud = (ConnectUserData*)m_pMsgHandler->getProxyUserData(m_pMsgHandler->getConnectProxyContext().conn);

		com_protocol::ClientPKPlayCancelSignUpRsp rsp;
		rsp.set_result(ProtocolReturnCode::Opt_Success);
		ProtocolReturnCode code = ProtocolReturnCode::NotFindCancelSignUpPKPlay;

		do 
		{
			switch (req.sign_up())
			{
			case SignUpType::SignUp:
			{
				do 
				{
					auto field = getField((EPKFieldID)req.play_id());
					if (field == NULL)
					{
						auto playRecord = m_playRecord.find(ud->connId);
						if (playRecord == m_playRecord.end())
						{
							rsp.set_result(ProtocolReturnCode::NotFindCancelSignUpPKPlay);
							break;
						}
						
						rsp.set_result(UserPlayingNotCancel);
						break;
					}

					for (auto tableIt = field->tableList.begin(); tableIt != field->tableList.end(); ++tableIt)
					{
						if (cancelTablePlayerSignUp(*tableIt, ud->connId, code))
						{
							OptInfoLog("Cancel Sign Up Req, field id:%d, player size:%d", field->nFieldID, tableIt->playerList.size());
							if (tableIt->playerList.size() == 0)	field->tableList.erase(tableIt);
							break;
						}
					}

					rsp.set_result(code);
				} while (false);
			}
				break;

			case SignUpType::QuickStart:
			{
				do 
				{
					auto playerIt = m_quickStartList.find(ud->connId);
					if (playerIt != m_quickStartList.end())
					{
						m_quickStartList.erase(playerIt);
						break;
					}

					auto playRecord = m_playRecord.find(ud->connId);
					if (playRecord != m_playRecord.end())
					{
						rsp.set_result(UserPlayingNotCancel);
						break;
					}
					
					//查询场次
					for (auto fieldIt = m_field.begin(); fieldIt != m_field.end(); ++fieldIt)
					{
						if (!fieldIt->bOpen)
							continue;

						//查询桌子
						for (auto tableIt = fieldIt->tableList.begin(); tableIt != fieldIt->tableList.end(); ++tableIt)
						{
							if (cancelTablePlayerSignUp(*tableIt, ud->connId, code))
							{
								if (tableIt->playerList.size() == 0)	fieldIt->tableList.erase(tableIt);
								break;
							}
						}

						rsp.set_result(code);
					}
				} while (false);					
			}
				break;
							
			default:
				rsp.set_result(ProtocolReturnCode::SignUpTypeError);
			}

		} while (false);
		
		const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(rsp, "cancel sign up rsp");
		if (msgBuffer != NULL)
			m_pMsgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_PK_PLAY_CANCEL_SIGN_UP_RSP);
			
		// 玩家操作记录
		char remarks[32] = {0};
		snprintf(remarks, sizeof(remarks) - 1, "%s", (req.sign_up() == SignUpType::QuickStart) ? "快速报名" : "入场报名");
		writePlayerOptRecord(ud->connId, ud->connIdLen, EPKPlayerOpt::ECancelSignUp, remarks);
	}

	void CPkLogic::handleClientReadyReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		com_protocol::ClientPKPlayReadyReq req;
		if (!m_pMsgHandler->parseMsgFromBuffer(req, data, len, "handle client ready req")) return;

		com_protocol::ClientPKPlayReadyRsp rsp;
		rsp.set_result(ProtocolReturnCode::Opt_Success);

		ConnectUserData* ud = (ConnectUserData*)m_pMsgHandler->getProxyUserData(m_pMsgHandler->getConnectProxyContext().conn);
		
		vector<TableInfo>::iterator table;
		std::vector<FieldInfo>::iterator field;

		if (!getTableAndField(ud->connId, table, field))
		{
			rsp.set_result(ProtocolReturnCode::NotFindMatchingTable);
			const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(rsp, "client ready req");
			if (msgBuffer != NULL)
				m_pMsgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_PK_PLAY_READY_RSP);
			return;
		}

		do 
		{
			
			auto playerIt = table->playerList.find(ud->connId);
			if (playerIt == table->playerList.end())
			{
				rsp.set_result(ProtocolReturnCode::NotFindMatchingTable);
				break;
			}			
			
			if (table->nStatus == StatusPlaying)
			{
				rsp.set_result(ProtocolReturnCode::UserPlaying);
				break;
			}

			if (req.ready() == 0)
				playerIt->second.nPlayeStatus = EPlayeStatus::StatusReady;
			else
			{
				table->playerList.erase(playerIt);
			}			

		} while (false);
				
		const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(rsp, "client ready req");
		if (msgBuffer != NULL)
			m_pMsgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_PK_PLAY_READY_RSP);	

		
		//删除快速开始的选手并通知其结果
		if (onBefStartPlay(*table) == EStarPlayCode::EPlayInsufficientNumOfPeople)
		{
			quickStarPlayerEscTable(*table, true);
			if (table->playerList.size() == 0)
				field->tableList.erase(table);
		}
	} 

	void CPkLogic::handleClientlReconnectionReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		if (!m_pMsgHandler->checkLoginIsSuccess("receive not login successs message to handle client cancel reconnection req")) return;

		com_protocol::ClientPKPlayReconnectionReq reReq;
		if (!m_pMsgHandler->parseMsgFromBuffer(reReq, data, len, "handle clientl reconnection req")) return;

		ConnectUserData* ud = (ConnectUserData*)m_pMsgHandler->getProxyUserData(m_pMsgHandler->getConnectProxyContext().conn);
		auto playRecord = m_playRecord.find(ud->connId);
		if (playRecord == m_playRecord.end())
		{
			OptInfoLog("CPkLogic handleClientlReconnectionReq, user name:%s", ud->connId);
			return;
		}

		com_protocol::ClientPKPlayReconnectionRsp rsp;
		rsp.set_result(Opt_Success);
		rsp.set_reconnection(reReq.reconnection());
		do 
		{
			if (reReq.reconnection() == 0)
			{
				//超过游戏时间
				auto curTime = time(NULL);
				if (curTime >= playRecord->second.playEndTime)
				{
					rsp.set_result(PkPlayEnd);
					m_playRecord.erase(playRecord);
					break;
				}
			}

			//断线重连
			com_protocol::PlayerPKWaiverReq req;
			req.set_username(ud->connId);
			req.set_reconnection(reReq.reconnection());
			const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(req, "player pk waiver req");
			m_pMsgHandler->sendMessage(msgBuffer, req.ByteSize(), playRecord->second.srv_id, PKServiceProtocol::SERVICE_PLAYER_WAIVER_REQ);

			if (reReq.reconnection() == 0)
			{
				rsp.set_ip(playRecord->second.ip);
				rsp.set_port(playRecord->second.port);
				rsp.set_srv_id(playRecord->second.srv_id);
			}
			else
			{
				m_playRecord.erase(playRecord);
			}

		} while (false);

		const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(rsp, "handle clientl reconnection req");
		if (msgBuffer != NULL)
			m_pMsgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_PK_PLAY_RECONNECTION_RSP);
	}

	void CPkLogic::getGameDataForSignUp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		string userName = m_pMsgHandler->getContext().userData;
		auto player = m_tempSignUpPlayer.find(userName);
		if (player == m_tempSignUpPlayer.end())
		{
			OptErrorLog("CPkLogic getGameDataForSignUp, not find sign up user:%s", userName.c_str());
			return;
		}

		com_protocol::GetGameDataRsp gameLogicDataRsp;
		if (!m_pMsgHandler->parseMsgFromBuffer(gameLogicDataRsp, data, len, "get game data for is sign up") || gameLogicDataRsp.result() != 0)
		{
			OptErrorLog("CPkLogic getGameDataForSignUp, get game data for is sign up = %d", gameLogicDataRsp.result());
			m_tempSignUpPlayer.erase(player);
			return;
		}

		com_protocol::BuyuLogicData game_logic_data;
		if ((gameLogicDataRsp.has_data() && gameLogicDataRsp.data().length() > 0) && !m_pMsgHandler->parseMsgFromBuffer(game_logic_data,
			gameLogicDataRsp.data().c_str(), gameLogicDataRsp.data().length(), "get game data for is sign up"))
		{
			m_tempSignUpPlayer.erase(player);
			return;
		}
				

		com_protocol::ClientPKPlaySignUpRsp rsp;
		do 
		{
			const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
			const HallConfigData::PkPlay* fieldCfg = getPkPlayCfg(cfg, player->second.nQuickStartFieldID);
			if (fieldCfg == NULL)
			{
				rsp.set_result(ProtocolReturnCode::NotFindSignUpPKPlay);
				break;
			}
									
			player->second.lv = game_logic_data.level_info().cur_level();

			// 向 db_common 取用户道具信息 已经防作弊列表
			com_protocol::GetUserPropAndPreventionCheatReq db_req;
			char send_data[MaxMsgLen] = { 0 };
			db_req.set_query_username(userName);
			for (int i = 1; i < PropMax; ++i)
				db_req.add_info_flag(i);

			db_req.add_info_flag(EPKDayGoldTicket);
			db_req.SerializeToArray(send_data, MaxMsgLen);
			m_pMsgHandler->sendMessageToService(NFrame::ServiceType::DBProxyCommonSrv, send_data, db_req.ByteSize(), BUSINESS_GET_USER_PROP_AND_PREVENTION_CHEAT_REQ,
												userName.c_str(), userName.size());
			return;

		} while (false);
		

		const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(rsp, "sign up req");
		if (msgBuffer != NULL)
			m_pMsgHandler->sendMsgToProxy(msgBuffer, rsp.ByteSize(), LOGINSRV_PK_PLAY_SIGN_UP_RSP, m_pMsgHandler->getConnectProxy(userName.c_str()));

		m_tempSignUpPlayer.erase(player);
	}

	void CPkLogic::matchingPlayer(unsigned int timerId, int userId, void* param, unsigned int remainCount)
	{
		for (auto fieldIt = m_field.begin(); fieldIt != m_field.end(); ++fieldIt)
		{
			if (!fieldIt->bOpen)
				continue;
			//匹配 快速匹配选手
			onMatchingQuick(*fieldIt, true);
		}

		//剩余的快速匹配选手进入备选场次
		for (auto fieldIt = m_field.begin(); fieldIt != m_field.end(); ++fieldIt)
		{
			if (!fieldIt->bOpen)
				continue;				

			onMatchingQuick(*fieldIt, false);
		}

		//剩余的快速匹配选手进行匹配
		surplusQuickPlayerMatching();

		//匹配完毕
		matchingFinish();

		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		m_pMsgHandler->setTimer(cfg.pk_play_info.matching_time * 1000, (TimerHandler)&CPkLogic::matchingPlayer, 0, NULL, 1, this);
	}

	void CPkLogic::onMatchingQuick(FieldInfo &field, bool bQuick)
	{
		//把快速开始的选手(优先该场次)匹配到该场次
		bool bInTo = false;
		//if (!bQuick)
		//	OptInfoLog("onMatchingQuick quick start list size:%d", m_quickStartList.size());
		for (auto quickPlayer = m_quickStartList.begin(); quickPlayer != m_quickStartList.end();)
		{
			bInTo = false;
			do
			{
				//匹配优先场次
				if (bQuick && quickPlayer->second.nQuickStartFieldID != field.nFieldID)
					break;

				//匹配备选场次
				if (!bQuick && !isAlternativeField(quickPlayer->second, field.nFieldID))
					break;									
				
				//判断能否入场
				if (!intoField(&field, quickPlayer->first, quickPlayer->second, true))
					break;

				bInTo = true;
			} while (false);
			
			if (bInTo)		m_quickStartList.erase(quickPlayer++);
			else			++quickPlayer;
		}
	}

	void CPkLogic::surplusQuickPlayerMatching()
	{
		std::map<std::string, PlayerInfo> result;

		// 将剩余快速开始选手中同一优先场次的选手放到一起
		for (auto fieldIt = m_field.begin(); fieldIt != m_field.end(); ++fieldIt)
		{
			if (!fieldIt->bOpen)
				continue;
						
			result.clear();

			//扫描优先该场次的选手
			scanFirstField(fieldIt->nFieldID, m_quickStartList, result);

			//将优先该场次的选手进入场次
			quickStartPlayerIntoField(*fieldIt, result, m_quickStartList);
		}

		// 将剩余快速开始选手放到一起
		for (auto fieldIt = m_field.begin(); fieldIt != m_field.end(); ++fieldIt)
		{
			if (!fieldIt->bOpen)
				continue;		
			
			result.clear();

			//扫描该场次的选手
			scanField(fieldIt->nFieldID, m_quickStartList, result);

			//将优先该场次的选手进入场次
			quickStartPlayerIntoField(*fieldIt, result, m_quickStartList);
		}
	}
	
	void CPkLogic::matchingFinish()
	{
		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();

		auto timerID = m_pMsgHandler->setTimer((cfg.pk_play_info.wait_ready_time) * 1000, (TimerHandler)&CPkLogic::waitEndPlayerReady, 0, NULL, 1, this);

		std::map<std::string, PlayerInfo> notReadyPlayer;

		for (auto fieldIt = m_field.begin(); fieldIt != m_field.end(); ++fieldIt)
		{
			if (!fieldIt->bOpen)
				continue;

			for (auto tableIt = fieldIt->tableList.begin(); tableIt != fieldIt->tableList.end();)
			{				
				if (tableIt->nStatus != ETableStatus::StatusIdle)
				{
					++tableIt;
					continue;
				}					

				switch (onBefStartPlay(*tableIt))
				{

				//扫描桌上没有准备的选手
				case EStarPlayCode::EPlayerNotReady:
					if (tableIt->playerList.size() < cfg.pk_play_info.play_num_min)
						break;
					else
					{						
						notReadyPlayer.clear();
						scanTablePlayerNotReady(*tableIt, notReadyPlayer);

						//向客户端通知用户准备
						com_protocol::ClientPKPlayReadyNotify notify;
						notify.set_time(cfg.pk_play_info.wait_ready_time);
						notify.set_play_id(fieldIt->nFieldID);
						const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(notify, "client pk play ready notify");

						for (auto it = notReadyPlayer.begin(); it != notReadyPlayer.end(); ++it)
							m_pMsgHandler->sendMsgToProxy(msgBuffer, notify.ByteSize(), LOGINSRV_PK_PLAY_READY_NOTIFY, m_pMsgHandler->getConnectProxy(it->first));

						tableIt->nStatus = StatusMatching;
						tableIt->timerId = timerID;
					}
					break;

				case EStarPlayCode::EPlayInsufficientNumOfPeople:
					quickStarPlayerEscTable(*tableIt);
					tableIt->nStatus = StatusIdle;
					break;

				default:
					break;
				}

				if (tableIt->playerList.size() == 0)
				{
					tableIt = fieldIt->tableList.erase(tableIt);
				}
				else
				{
					++tableIt;
				}
			}
		}
	}

	EStarPlayCode CPkLogic::onBefStartPlay(TableInfo &table)
	{
		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();

		if (table.nStatus == ETableStatus::StatusPlaying)
			return EStarPlayCode::EPlaying;

		if (table.playerList.size() < cfg.pk_play_info.play_num_min)
			return EStarPlayCode::EPlayInsufficientNumOfPeople;

		com_protocol::GetManyUserOtherInfoReq db_req;
		db_req.add_info_flag(EUserInfoFlag::EUserNickname);
		db_req.add_info_flag(EUserInfoFlag::EUserSex);
		db_req.add_info_flag(EUserInfoFlag::EUserPortraitId);
		db_req.add_info_flag(EUserInfoFlag::EVipLevel);		
		db_req.add_info_flag(EPropType::PropGold);

		for (auto playerIt = table.playerList.begin(); playerIt != table.playerList.end(); ++playerIt)
		{
			if (playerIt->second.nPlayeStatus == EPlayeStatus::StatusNotReady)
				return EStarPlayCode::EPlayerNotReady;

			db_req.add_query_username(playerIt->first.c_str(), playerIt->first.size());
		}	 		

		//向DB查询桌子上用户信息
		const char* data = m_pMsgHandler->serializeMsgToBuffer(db_req, "get many user info req");
		m_pMsgHandler->sendMessageToService(NFrame::ServiceType::DBProxyCommonSrv, data, db_req.ByteSize(), CommonDBSrvProtocol::BUSINESS_GET_MANY_USER_OTHER_INFO_REQ, "", 0, GetManyUserInfoFlag::GetManyUserInfoStartPkPlay);

		table.nStatus = ETableStatus::StatusPlaying;
		return EStarPlayCode::EStarPlaySuccess;
	}

	void CPkLogic::notifyClientPKPlayStatus(EPKFieldID nFieldID, bool bStatus)
	{
		com_protocol::ClientPKPlayStatusNotify notify;
		notify.set_play_id(nFieldID);
		notify.set_play_status(bStatus);
		const char* msgBuffer = m_pMsgHandler->serializeMsgToBuffer(notify, "notify client pk play status");

		const IDToConnectProxys& userConnects = m_pMsgHandler->getConnectProxy();
		for (IDToConnectProxys::const_iterator it = userConnects.begin(); it != userConnects.end(); ++it)
		{
			m_pMsgHandler->sendMsgToProxy(msgBuffer, notify.ByteSize(), LOGINSRV_PK_PLAY_STATUS_NOTIFY, m_pMsgHandler->getConnectProxy(it->first));
		}
	}

	void CPkLogic::waitEndPlayerReady(unsigned int timerId, int userId, void* param, unsigned int remainCount)
	{
		//扫描桌上没有准备的选手
		std::map<std::string, PlayerInfo> notReadyPlayer;

		for (auto fieldIt = m_field.begin(); fieldIt != m_field.end(); ++fieldIt)
		{
			if (!fieldIt->bOpen)
				continue;

			for (auto tableIt = fieldIt->tableList.begin(); tableIt != fieldIt->tableList.end();)
			{
				if (tableIt->timerId != timerId)
				{
					++tableIt;
					continue;
				}

				if (onBefStartPlay(*tableIt) != EStarPlayCode::EStarPlaySuccess)
				{
					quickStarPlayerEscTable(*tableIt, true);
					tableIt->nStatus = ETableStatus::StatusIdle;
				}

				tableIt->timerId = ETimerId;

				if (tableIt->playerList.size() == 0)	
					tableIt = fieldIt->tableList.erase(tableIt);
				else									
					++tableIt;
			}
		}
	}

	/*bool CPkLogic::getPropNum(uint32_t propType, uint32_t &num, const com_protocol::GetUserPropAndPreventionCheatRsp &db_rsp)
	{
		for (int i = 0; i < db_rsp.other_info_size(); ++i)
		{
			if (db_rsp.other_info(i).info_flag() == propType)
			{
				num = rsp.other_info(i).int_value();
				return true;
			}		
		}
		
		OptErrorLog("CPkLogic getPropNum, prop type:%d", propType);
		num = 0;
		return false;
	}
	*/
	const HallConfigData::PkPlay* CPkLogic::getPkPlayCfg(const HallConfigData::config& cfg, int nFieldID)
	{
		for (auto pkPlayCfg = cfg.pk_play_info.PkPlayInfoList.begin(); pkPlayCfg != cfg.pk_play_info.PkPlayInfoList.end(); ++pkPlayCfg)
		{
			if (pkPlayCfg->play_id == nFieldID)
				return &(*pkPlayCfg);
		}

		OptErrorLog("CPkLogic getPkPlayCfg, get pk play cfg, pk play id:%d", nFieldID);
		return NULL;
	}

	uint32_t CPkLogic::getCurTimeSec()
	{
		struct tm t = { 0 };
		time_t tt;
		time(&tt);
		localtime_r(&tt, &t);
		return t.tm_hour * exchange_rate_hour + t.tm_min * exchange_rate_min + t.tm_sec;
	}

	FieldInfo* CPkLogic::getField(EPKFieldID nFieldID)
	{
		for (auto it = m_field.begin(); it != m_field.end(); ++it)
		{
			if (it->nFieldID == nFieldID)
				return &(*it);
		}

		OptErrorLog("CPkLogic getField, not find field, field id:%d", nFieldID);
		return NULL;
	}

	bool CPkLogic::getTableAndField(const std::string &user, vector<TableInfo>::iterator &table, std::vector<FieldInfo>::iterator &field)
	{
		for (auto it = m_field.begin(); it != m_field.end(); ++it)
		{
			if (!it->bOpen)
				continue;

			for (auto tableIt = it->tableList.begin(); tableIt != it->tableList.end(); ++tableIt)
			{
				auto playerIt = tableIt->playerList.find(user);
				if (playerIt == tableIt->playerList.end())
					continue;

				field = it;
				table = tableIt;
				return true;
			}
		}
		return false;
	}

	bool CPkLogic::isUserSignUp(const std::string &user)
	{
		for (auto it = m_field.begin(); it != m_field.end(); ++it)
		{
			if(!it->bOpen)
				continue;

			for (auto tableIt = it->tableList.begin(); tableIt != it->tableList.end(); ++tableIt)
			{
				if (tableIt->playerList.find(user) != tableIt->playerList.end())
				{
					OptInfoLog("is user sign up, field id:%d, table status:%d, user:%s, size:%d", it->nFieldID, tableIt->nStatus, user.c_str(), tableIt->playerList.size());
					return true;
				}
			}
		}
	
		if (m_quickStartList.find(user) != m_quickStartList.end())
			return true;

		return m_tempSignUpPlayer.find(user) != m_tempSignUpPlayer.end();
	}

	ProtocolReturnCode CPkLogic::signUp(EPKFieldID nFieldID, SignUpType nSignUpType, uint32_t lv, const std::string &user,
										const com_protocol::GetUserPropAndPreventionCheatRsp &db_rsp)
	{
		if (nSignUpType != SignUpType::SignUp)
		{
			OptErrorLog("CPkLogic signUp, Sign Up Type: %d", nSignUpType);
			return ProtocolReturnCode::SignUpTypeError;
		}

		auto signUpField = getField(nFieldID);
		if (signUpField == NULL)
		{
			OptErrorLog("CPkLogic signUp, signUpField == NULL, play id:%d", nFieldID);
			return ProtocolReturnCode::NotFindSignUpPKPlay;
		}

		//报名场次是否开启
		if (!signUpField->bOpen)
			return ProtocolReturnCode::PKPlayAlreadyCloser;

		//判断是否满足入场条件
		int nTicket = 0;		//true表示是使用门票
		auto code = isIntoField(nFieldID, lv, db_rsp, nTicket);
		if (code != ProtocolReturnCode::Opt_Success)
			return code;
	
		//选手信息
		PlayerInfo player(nSignUpType, nFieldID, StatusReady, lv);
		player.ticket.insert(std::map<int, int>::value_type(nFieldID, nTicket));

		for (auto preventionCheatIt = db_rsp.prevention_cheat_user().begin(); preventionCheatIt != db_rsp.prevention_cheat_user().end(); ++preventionCheatIt)
			player.cheatList.push_back(*preventionCheatIt);

		//进入该场次(如果没进入 则新加一张桌进入该场次)
		if (!intoField(signUpField, user, player))
		{		
			TableInfo table;
			//OptInfoLog("CPkLogic signUp, new table");
			intoTable(user, player, table);
			signUpField->tableList.push_back(table);
		}

		return ProtocolReturnCode::Opt_Success;
	}

	ProtocolReturnCode CPkLogic::quickStart(EPKFieldID nFieldID, SignUpType nSignUpType, uint32_t lv, const std::string &user,
											const com_protocol::GetUserPropAndPreventionCheatRsp &db_rsp)
	{
		if (nSignUpType != SignUpType::QuickStart)
		{
			OptErrorLog("CPkLogic quickStart, Sign Up Type: %d", nSignUpType);
			return ProtocolReturnCode::SignUpTypeError;
		}

		bool bFlag = false;				//false 表示没有一个场开启
		PlayerInfo player;
		player.lv = lv;
		player.nPlayeStatus = StatusNotReady;
		player.nSignUp = SignUpType::QuickStart;
		ProtocolReturnCode code;
		int nTicket = 0;			//true表示使用门票

		//选手防作弊信息
		for (auto preventionCheatIt = db_rsp.prevention_cheat_user().begin(); preventionCheatIt != db_rsp.prevention_cheat_user().end(); ++preventionCheatIt)
			player.cheatList.push_back(*preventionCheatIt);

		for (int i = FieldIDPhoneFare; i < FieldIDMax; ++i)
		{
			auto signUpField = getField((EPKFieldID)i);
			if (signUpField == NULL)
			{
				OptErrorLog("CPkLogic quickStart, signUpField == NULL, play id:%d", i);
				return ProtocolReturnCode::NotFindSignUpPKPlay;
			}

			if (!signUpField->bOpen)
				continue;

			
			code = isIntoField((EPKFieldID)i, player.lv, db_rsp, nTicket);
			if (code != ProtocolReturnCode::Opt_Success)
				continue;
			
			if ((EPKFieldID)i == nFieldID)
				player.nQuickStartFieldID = nFieldID;
			else
				player.alternativeFieldID.push_back((EPKFieldID)i);

			player.ticket.insert(std::map<int, int>::value_type(i, nTicket));
			bFlag = true;
		}
		
		if (!bFlag)
			return ProtocolReturnCode::NotFindSignUpPKPlay;

		//从备选场次选择一个当优先场次
		if (player.nQuickStartFieldID == NProject::FieldIDNone)
		{
			if (player.alternativeFieldID.size() <= 0)
				return ProtocolReturnCode::NotFindSignUpPKPlay;

			auto fieldIt = player.alternativeFieldID.begin();
			player.nQuickStartFieldID = *fieldIt;
			player.alternativeFieldID.erase(fieldIt);
		}

		
		m_quickStartList.insert(PLAYER_VALUE(user, player));

		//OptInfoLog("quick start, nFieldID:%d, user name:%s, quick start field id:%d", nFieldID, user.c_str(), player.nQuickStartFieldID);
		//for (auto it = player.alternativeFieldID.begin(); it != player.alternativeFieldID.end(); ++it)
		//	OptInfoLog("bei xuan: %d", *it);

		return ProtocolReturnCode::Opt_Success;
	}

	ProtocolReturnCode CPkLogic::isIntoField(EPKFieldID nFieldID, uint32_t lv, const com_protocol::GetUserPropAndPreventionCheatRsp &db_rsp, int &nTicket)
	{
		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		nTicket = 0;

		auto fieldCfg = getPkPlayCfg(cfg, nFieldID);
		if (fieldCfg == NULL)
		{
			OptErrorLog("CPkLogic signUp, fieldCfg == NULL, play id:%d", nFieldID);
			return ProtocolReturnCode::NotFindSignUpPKPlay;
		}

		if (lv < (uint32_t)fieldCfg->user_lv)
			return ProtocolReturnCode::SignUpLvNotEnough;

		//如果有门票则直接进入
		uint32_t num = 0;
		if (fieldCfg->ticket != 0 && getPropNum<const com_protocol::GetUserPropAndPreventionCheatRsp>(fieldCfg->ticket, num, db_rsp))
		{
			if (num != 0)
			{
				nTicket = num;
				return ProtocolReturnCode::Opt_Success;
			}
		}

		//统计入场所需的道具
		map<int, uint32_t> condition;
		for (auto it = fieldCfg->condition_list.begin(); it != fieldCfg->condition_list.end(); ++it)
			condition[it->first] += it->second;
		for (auto it = fieldCfg->entrance_fee.begin(); it != fieldCfg->entrance_fee.end(); ++it)
			condition[it->first] += it->second;

		//可携带最大金币数
		condition[PropGold] += cfg.pk_play_info.gold_max;
		for (auto it = condition.begin(); it != condition.end(); ++it)
		{
			if (!getPropNum<const com_protocol::GetUserPropAndPreventionCheatRsp>(it->first, num, db_rsp))
				return ProtocolReturnCode::SignUpConditionNotSatisfied;

			if (num < it->second)
			{
				switch (it->first)
				{
				case PropGold:
					return ProtocolReturnCode::GoldInsufficient;

				case PropFishCoin:
					return ProtocolReturnCode::FishCoinInsufficient;

				case PropPhoneFareValue:
					return ProtocolReturnCode::PhoneFareValueInsufficient;

				case PropScores:
					return ProtocolReturnCode::ScoresInsufficient;

				case PropDiamonds:
					return ProtocolReturnCode::DiamondsInsufficient;

				case PropVoucher:
					return ProtocolReturnCode::VoucherInsufficient;

				default:
					return ProtocolReturnCode::SignUpConditionNotSatisfied;
				}
			}
		}
		return ProtocolReturnCode::Opt_Success;
	}

	bool CPkLogic::intoField(FieldInfo* signUpField, const std::string &user, const PlayerInfo &player, bool bQuickStart)
	{
		if (signUpField == NULL)
		{
			OptErrorLog("CPkLogic intoField, signUpField == NULL");
			return false;
		}			
		
		for (auto tableIt = signUpField->tableList.begin(); tableIt != signUpField->tableList.end(); ++tableIt)
		{
			//快速玩家进入桌子前(该桌子必须要有玩家)
			if (bQuickStart && tableIt->playerList.size() <= 0)
				continue;

			if (!intoTable(user, player, *tableIt))
				continue;

			//OptInfoLog("CPkLogic intoField, into table, user:%s, field id:%d", user.c_str(), signUpField->nFieldID);
			return true;
		}
		return false;
	}

	bool CPkLogic::intoTable(const std::string &user, const PlayerInfo &player, TableInfo &tableInfo)
	{
		if (!isIntoTable(user, tableInfo))
			return false;
				
		if (!tableInfo.playerList.insert(std::map<std::string, PlayerInfo>::value_type(user, player)).second)
		{
			//表示该桌子已经有该玩家了(插入相同的用户)
			OptErrorLog("CPkLogic::intoTable, into table user:%s", user.c_str());
			return true;
		}

		//OptInfoLog("into table, tableInfo id:%d,  table user size:%d, user:%s", &tableInfo, tableInfo.playerList.size(), user.c_str());
		return true;
	}

	bool CPkLogic::isIntoTable(const std::string &user, const TableInfo &tableInfo)
	{
		if (tableInfo.playerList.size() >= table_player_max)
			return false;

		if (tableInfo.nStatus != StatusIdle)
			return false;

		//防作弊
		for (auto tableIt = tableInfo.playerList.begin(); tableIt != tableInfo.playerList.end(); ++tableIt)
		{
			for (auto cheatIt = tableIt->second.cheatList.begin(); cheatIt != tableIt->second.cheatList.end(); ++cheatIt)
			{
				if (*cheatIt == user)
				return false;
			}				
		}

		return true;
	}

	bool CPkLogic::isAlternativeField(const PlayerInfo &player, EPKFieldID nFieldID)
	{
		for (auto alternativeIt = player.alternativeFieldID.begin(); 
			 alternativeIt != player.alternativeFieldID.end(); 
			 ++alternativeIt)
		{
			if (*alternativeIt == nFieldID)
				return true;
		}
		return false;
	}

	void CPkLogic::quickStarPlayerEscTable(TableInfo &table, bool bNotice)
	{
		com_protocol::ClientPKPlayReadyResultNotify notify;
		notify.set_result(ProtocolReturnCode::PlayerQuitGameReMatch);
		auto msgBuffer = m_pMsgHandler->serializeMsgToBuffer(notify, "client pk play ready result notify");

		for (auto player = table.playerList.begin(); player != table.playerList.end();)
		{
			if (player->second.nSignUp != SignUpType::QuickStart)
				++player;
			else
			{
				if (bNotice && player->second.nPlayeStatus == EPlayeStatus::StatusReady)
				{
					m_pMsgHandler->sendMsgToProxy(msgBuffer, notify.ByteSize(), LOGINSRV_PK_PLAY_READY_RESULT_NOTIFY,
												  m_pMsgHandler->getConnectProxy(player->first.c_str()));
				}			
				//m_quickStartList.insert(*player);
				table.playerList.erase(player++);
			}		
		}
	}

	void CPkLogic::scanTablePlayerNotReady(TableInfo &table, std::map<std::string, PlayerInfo> &result)
	{
		for (auto player = table.playerList.begin(); player != table.playerList.end(); ++player)
		{
			if (player->second.nPlayeStatus == StatusNotReady)
				result.insert(*player);
		}
	}

	bool CPkLogic::cancelTablePlayerSignUp(TableInfo &table, const std::string &user, ProtocolReturnCode &code)
	{
		for (auto playerIt = table.playerList.begin(); playerIt != table.playerList.end(); ++playerIt)
		{
			if(playerIt->first == user)
			{
				if (table.nStatus == StatusPlaying)
				{
					code = ProtocolReturnCode::UserPlayingNotCancel;
					return true;
				}

				OptInfoLog("cancel table player sign up, user name:%s, table status:%d, size:%d", playerIt->first.c_str(), table.nStatus, table.playerList.size());
				table.playerList.erase(playerIt);
				code = ProtocolReturnCode::Opt_Success;		
				return true;
			}
		}
		code = ProtocolReturnCode:: NotFindCancelSignUpPKPlay;
		return false;
	}

	void CPkLogic::scanFirstField(EPKFieldID nFieldID, const std::map<std::string, PlayerInfo> &player, std::map<std::string, PlayerInfo> &result)
	{
		for (auto playerIt = player.begin(); playerIt != player.end(); ++playerIt)
		{
			if (playerIt->second.nQuickStartFieldID == nFieldID)
				result.insert(*playerIt);
		}
	}

	void CPkLogic::scanField(EPKFieldID nFieldID, const std::map<std::string, PlayerInfo> &player, std::map<std::string, PlayerInfo> &result)
	{
		for (auto playerIt = player.begin(); playerIt != player.end(); ++playerIt)
		{

			if (playerIt->second.nQuickStartFieldID == nFieldID)
			{
				result.insert(PLAYER_VALUE(playerIt->first, playerIt->second));
				continue;
			}

			for (auto alternativeIt = playerIt->second.alternativeFieldID.begin(); alternativeIt != playerIt->second.alternativeFieldID.end(); 
				  ++alternativeIt)
			{
				if (*alternativeIt == nFieldID)
				{
					result.insert(PLAYER_VALUE(playerIt->first, playerIt->second));
					break;
				}
			}	
		}
	}

	void CPkLogic::quickStartPlayerIntoField(FieldInfo &field, std::map<std::string, PlayerInfo> &intoPlayer, std::map<std::string, PlayerInfo> &quickStartList)
	{
		if (!field.bOpen)
			return;

		const HallConfigData::config& cfg = HallConfigData::config::getConfigValue();
		if (intoPlayer.size() < cfg.pk_play_info.play_num_min)
			return;

		bool bFlag = false;
		vector<TableInfo>	tableList;
		
		for (auto playerIt = intoPlayer.begin(); playerIt != intoPlayer.end(); ++playerIt)
		{
			if (tableList.size() == 0)
			{
				TableInfo table;
				table.nStatus = StatusIdle;
				tableList.push_back(table);
			}
			
			bFlag = false;
			for (auto tableIt = tableList.begin(); tableIt != tableList.end(); ++tableIt)
			{				
				if (intoTable(playerIt->first, playerIt->second, *tableIt))
				{
					bFlag = true;
					//OptInfoLog("CPkLogic::quickStartPlayerIntoField, into table, user:%s, bFlag:%d", playerIt->first.c_str(), bFlag);
					break;
				}					
			}

			//没进去则新建一个桌子
			if (!bFlag)
			{
				TableInfo table;
				table.nStatus = StatusIdle;
				intoTable(playerIt->first, playerIt->second, table);
				//OptInfoLog("CPkLogic::quickStartPlayerIntoField, into table, user:%s, bFlag:%d", playerIt->first.c_str(), bFlag);
				tableList.push_back(table);
			}			
		}

		for (auto tableIt = tableList.begin(); tableIt != tableList.end(); ++tableIt)
		{
			if(tableIt->playerList.size() < cfg.pk_play_info.play_num_min)
				continue;

			//将桌上玩家加入到对应的场次当中
			field.tableList.push_back(*tableIt);

			//从快速玩家队列删除
			for (auto playerIt = tableIt->playerList.begin(); playerIt != tableIt->playerList.end(); ++playerIt)
			{
				auto player = quickStartList.find(playerIt->first);
				if (player == quickStartList.end())
					continue;
				
				quickStartList.erase(player);
			}
		}		
	}

	void CPkLogic::clearTable(TableInfo &table)
	{
		table.nStatus = ETableStatus::StatusIdle;
		table.timerId = 0;
		table.playerList.clear();
	}

	bool CPkLogic::writeRecord(const TableInfo* pTable, uint32_t ip, uint32_t port, uint32_t srvId, uint32_t playTime, EPKFieldID nPkFieldId)
	{
		if (pTable == NULL)
			return false;

		PlayRecord playRecord;
		playRecord.ip = ip;
		playRecord.port = port;
		playRecord.srv_id = srvId;
		playRecord.playEndTime = time(NULL) + playTime;
		playRecord.play_id = nPkFieldId;
		
		for (auto it = pTable->playerList.begin(); it != pTable->playerList.end(); ++it)
			m_playRecord.insert(std::map<std::string, PlayRecord>::value_type(it->first, playRecord));
		
		return true;
	}
	
	// 生成PK场游戏记录ID
	unsigned int CPkLogic::generatePKRecordId(char* recordId, unsigned int len)
	{
		// 生成PK记录ID
		static unsigned int PKRecordIndex = 0;           // 避免同一时间同一服务产生相同的记录ID
		const unsigned int MaxPKRecordIndex = 100000;
		return snprintf(recordId, len - 1, "%uS%uI05%u", (unsigned int)time(NULL), m_pMsgHandler->getSrvId(), (PKRecordIndex++ % MaxPKRecordIndex));
	}
	
	//PK开始消息应答
	void CPkLogic::playerStartPKReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
	}

	//PK弃权应答
	void CPkLogic::playerWaiverRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
	}

	//PK场游戏结束
	void CPkLogic::playEndPkNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		com_protocol::PlayerPKEndNotify notify;
		if (!m_pMsgHandler->parseMsgFromBuffer(notify, data, len, "play end pk notify"))
		{
			OptErrorLog("CPkLogic playEndPkNotify, play end pk notify parse msg from buffer");
			return;
		}

		com_protocol::GetPkPlayRankingClearNumReq req;		
		for (int i = 0; i < notify.username_size(); ++i)
		{
			//通知客户端刷新数据
			m_pMsgHandler->sendMsgToProxy("", 0, LOGINSRV_USER_ATTRIBUTE_UPDATE_NOTIFY, m_pMsgHandler->getConnectProxy(notify.username(i).c_str()));
						
			if (notify.score(i) >= notify.min_score())
			{
				req.add_username(notify.username(i));
				req.add_pk_score(notify.score(i));
				
				m_pMsgHandler->getActivityCenter().addPKJoinMatchTimes(notify.username(i).c_str());  // 新增PK任务参赛次数
			}
		}

		if (req.username_size() > 0)
		{
			//向消息中心取 PK排行榜的更新次数
			char msgBuffer[NFrame::MaxMsgLen] = { 0 };
			if (!req.SerializeToArray(msgBuffer, MaxMsgLen))
			{
				OptErrorLog("CPkLogic playEndPkNotify, msg byte len = %d, buff len = %d", req.ByteSize(), NFrame::MaxMsgLen);
				return;
			}

			//OptInfoLog("1. on pk play end, to msg srv find user clear num!");
			m_pMsgHandler->sendMessageToService(ServiceType::MessageSrv, msgBuffer, req.ByteSize(), MessageSrvProtocol::BUSINESS_GET_PK_PLAY_RANKING_CLEAR_NUM_REQ, "", 0);
		}
		
		for (int idx = 0; idx < notify.winners_size(); ++idx)
		{
			m_pMsgHandler->getActivityCenter().addPKWinMatchTimes(notify.winners(idx).c_str());  // 新增PK任务赢局次数
		}
	}

	void CPkLogic::getPKPlayRankingClearNumRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		com_protocol::GetPkPlayRankingClearNumRsp rsp;	
		if (!m_pMsgHandler->parseMsgFromBuffer(rsp, data, len, "get PK Play Ranking Clear Num Rsp")) return;

		//判断是否在有效排行时间内
		if (!rsp.open_ranking())
			return;

		//向消息中心更新PK排行榜
		com_protocol::PKPlayRankingListUpdateReq req;

		//查询离线玩家数据
		com_protocol::GetMoreKeyGameDataReq getHallLogicDataReq;
		getHallLogicDataReq.set_primary_key(HallLogicDataKey, HallLogicDataKeyLen);

		uint32_t maxSCore = 0;
		for (int32_t i = 0; i < rsp.username_size(); ++i)
		{		
			//更新用户的最佳得分(更新失败是因为用户离线了)
			if (!updateUserMaxPkScore(rsp.username(i), rsp.pk_score(i), rsp.clear_num(), maxSCore))
			{				
				//向game db 查询用户的大厅数据
				getHallLogicDataReq.add_second_keys(rsp.username(i));
				continue;
			}
			auto updateInfo = req.add_update_info();
			updateInfo->set_username(rsp.username(i));
			updateInfo->set_score(maxSCore);
		}

		char msgBuffer[NFrame::MaxMsgLen] = { 0 };
		if (req.update_info_size() > 0)
		{
			
			if (!req.SerializeToArray(msgBuffer, MaxMsgLen))
			{
				OptErrorLog("CPkLogic getPKPlayRankingClearNumRsp, msg byte len = %d, buff len = %d", req.ByteSize(), NFrame::MaxMsgLen);
				return;
			}

			//OptInfoLog("2. update user pk play ranking!");
			m_pMsgHandler->sendMessageToService(ServiceType::MessageSrv, msgBuffer, req.ByteSize(), MessageSrvProtocol::BUSINESS_UPDATE_PK_PLAY_RANKING_LIST_REQ, "", 0);
		}	

		if (getHallLogicDataReq.second_keys_size() > 0)
		{
			getHallLogicDataReq.set_tc_data(data, len);
			if (!getHallLogicDataReq.SerializeToArray(msgBuffer, MaxMsgLen))
			{
				OptErrorLog("CPkLogic getPKPlayRankingClearNumRsp, msg byte len = %d, buff len = %d", getHallLogicDataReq.ByteSize(), NFrame::MaxMsgLen);
				return;
			}
			
			OptInfoLog("Updat Hall data, off line user!");
			m_pMsgHandler->sendMessageToGameDbProxy(msgBuffer, getHallLogicDataReq.ByteSize(), GameServiceDBProtocol::GET_MORE_KEY_GAME_DATA_REQ, "", 0, CUSTOM_GET_MORE_HALL_DATA_FOR_UP_RANKING_INFO);
		}
	}

	void CPkLogic::getOnLineUserHallDataUpPKRangkingInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		com_protocol::GetMoreKeyGameDataRsp getHallLogicDataRsp;
		if (!m_pMsgHandler->parseMsgFromBuffer(getHallLogicDataRsp, data, len, "get on line user hall data up pk rangking info")) return;

		com_protocol::GetPkPlayRankingClearNumRsp rsp;
		if (!m_pMsgHandler->parseMsgFromBuffer(rsp, getHallLogicDataRsp.tc_data().c_str(), getHallLogicDataRsp.tc_data().length(), "parse msg from buffer to get PK play ranking"))
		{
			return;
		}

		//向消息中心更新PK排行榜
		com_protocol::PKPlayRankingListUpdateReq req;

		//大厅数据
		com_protocol::HallLogicData hallLogicData;

		//保存数据
		com_protocol::SetGameDataReq saveLogicData;
		saveLogicData.set_primary_key(HallLogicDataKey);

		char hallLogicDataBuff[MaxMsgLen] = { 0 };
		char msgBuffer[MaxMsgLen] = { 0 };
		uint32_t maxScore = 0;
		for (int i = 0; i < getHallLogicDataRsp.second_key_data_size(); ++i)
		{
			if (!m_pMsgHandler->parseMsgFromBuffer(hallLogicData, getHallLogicDataRsp.second_key_data(i).data().c_str(), getHallLogicDataRsp.second_key_data(i).data().length(), "get hall user logic data"))
			{
			//	OptErrorLog("get user logic data from redis error, user name = %s", getHallLogicDataRsp.second_key_data(i).key().c_str());
				continue;
			}

			for (int n = 0; n < rsp.username_size(); ++n)
			{
				if (rsp.username(n) == getHallLogicDataRsp.second_key_data(i).key())
				{
					if (!updateUserMaxPkScore(&hallLogicData, rsp.pk_score(n), rsp.clear_num(), maxScore))
						continue;

					//向排行榜更新的数据
					auto updateInfo = req.add_update_info();
					updateInfo->set_username(rsp.username(n));
					updateInfo->set_score(maxScore);

					//向Game db					
					saveLogicData.set_second_key(getHallLogicDataRsp.second_key_data(i).key());
					if (m_pMsgHandler->serializeMsgToBuffer(hallLogicData, hallLogicDataBuff, MaxMsgLen, "set hall logic data to redis"))
					{
						saveLogicData.set_data(hallLogicDataBuff, hallLogicData.ByteSize());
						if (m_pMsgHandler->serializeMsgToBuffer(saveLogicData, msgBuffer, MaxMsgLen, "set hall logic data to redis"))
							m_pMsgHandler->sendMessageToGameDbProxy(msgBuffer, saveLogicData.ByteSize(), SET_GAME_DATA_REQ, "", 0);
					}
				}
			}
		}

	
		if (req.update_info_size() > 0)
		{
			if (!req.SerializeToArray(msgBuffer, MaxMsgLen))
			{
				OptErrorLog("CPkLogic getOnLineUserHallDataUpPKRangkingInfo, msg byte len = %d, buff len = %d", req.ByteSize(), NFrame::MaxMsgLen);
				return;
			}

			OptInfoLog("3. update user pk play ranking!");
			m_pMsgHandler->sendMessageToService(ServiceType::MessageSrv, msgBuffer, req.ByteSize(), MessageSrvProtocol::BUSINESS_UPDATE_PK_PLAY_RANKING_LIST_REQ, "", 0);
		}
		
	}

	bool CPkLogic::updateUserMaxPkScore(const string &username, uint32_t pkScore, uint64_t nClearNum, uint32_t &maxSCore)
	{
		com_protocol::HallLogicData* hallLogicData = m_pMsgHandler->getHallLogicData(username.c_str());
		return updateUserMaxPkScore(hallLogicData, pkScore, nClearNum, maxSCore);	
	}

	bool CPkLogic::updateUserMaxPkScore(com_protocol::HallLogicData* hallLogicData, uint32_t pkScore, uint64_t nClearNum, uint32_t &maxSCore)
	{
		if (hallLogicData == NULL)
			return false;

		com_protocol::PkPlayRankingInfo* rankingInfo = NULL;
		if (!hallLogicData->has_ranking_info())
		{
			rankingInfo = hallLogicData->mutable_ranking_info();
			rankingInfo->set_last_pk_score(0);					//默认, 分数为0	
			rankingInfo->set_last_ranking(NoneRanking);			//默认, 未上榜
			rankingInfo->set_receive_reward(EPKPlayRankingRewardStatus::ENotAward);						//默认, 不可以领奖
			rankingInfo->set_last_effective_time(nClearNum);	//清空次数
			rankingInfo->set_score_max(pkScore);
			maxSCore = pkScore;
			return true;
		}

		rankingInfo = hallLogicData->mutable_ranking_info();

		if (rankingInfo->last_effective_time() != nClearNum)
		{
			if (nClearNum - rankingInfo->last_effective_time() == 1)
			{
				rankingInfo->set_last_pk_score(rankingInfo->score_max());
				rankingInfo->set_score_max(0);
			}
			else
			{
				rankingInfo->set_last_pk_score(0);					//默认, 分数为0	
			}

			rankingInfo->set_last_ranking(NoneRanking);			//默认, 未上榜
			rankingInfo->set_receive_reward(EPKPlayRankingRewardStatus::ENotAward);						//默认, 不可以领奖
			rankingInfo->set_last_effective_time(nClearNum);	//清空次数
		}

		if (pkScore > rankingInfo->score_max())
			rankingInfo->set_score_max(pkScore);

		maxSCore = rankingInfo->score_max();
		return true;
	}
	
	// 玩家操作记录
	void CPkLogic::writePlayerOptRecord(const char* username, const unsigned int uLen, unsigned int optFlag, const char* remarks)
	{
		com_protocol::PKPlayerMatchingNotify optNotify;
		optNotify.set_opt_flag(optFlag);
		optNotify.set_remarks(remarks);
		const char* msgData = m_pMsgHandler->serializeMsgToBuffer(optNotify, "write player PK opt record");
		if (msgData != NULL) m_pMsgHandler->sendMessageToService(ServiceType::DBProxyCommonSrv, msgData, optNotify.ByteSize(), BUSINESS_PLAYER_MATCHING_OPT_NOTIFY, username, uLen);
	}
}

 