
/* author : limingfan
 * date : 2017.01.03
 * description : 翻翻棋逻辑处理
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "CFFQILogicHandler.h"
#include "CXiangQiSrv.h"
#include "base/ErrorCode.h"
#include "base/Function.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;


namespace NPlatformService
{

CFFQILogicHandler::CFFQILogicHandler()
{
	m_msgHander = NULL;
}

CFFQILogicHandler::~CFFQILogicHandler()
{
	m_msgHander = NULL;
}

bool CFFQILogicHandler::load(CSrvMsgHandler* msgHandler)
{
	m_msgHander = msgHandler;
	
	m_msgHander->registerProtocol(OutsideClientSrv, XIANGQISRV_GET_TASK_REQ, (ProtocolHandler)&CFFQILogicHandler::getTaskInfo, this);
	m_msgHander->registerProtocol(OutsideClientSrv, XIANGQISRV_RECEIVE_TASK_REWARD_REQ, (ProtocolHandler)&CFFQILogicHandler::receiveTaskReward, this);
	m_msgHander->registerProtocol(XiangQiSrv, EServiceReplyProtocolId::CUSTOM_RECEIVE_TASK_REWARD_REPLY, (ProtocolHandler)&CFFQILogicHandler::receiveTaskRewardReply, this);
	
	m_msgHander->registerProtocol(OutsideClientSrv, XIANGQISRV_GET_FF_CHESS_MALL_CONFIG_REQ, (ProtocolHandler)&CFFQILogicHandler::getGoodsMallCfg, this);
	m_msgHander->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_GAME_MALL_CONFIG_RSP, (ProtocolHandler)&CFFQILogicHandler::getGoodsMallCfgReply, this);
	
	m_msgHander->registerProtocol(OutsideClientSrv, XIANGQISRV_GET_FF_CHESS_GOODS_MALL_REQ, (ProtocolHandler)&CFFQILogicHandler::getFFChessGoodsMall, this);
	m_msgHander->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_FF_CHESS_MALL_RSP, (ProtocolHandler)&CFFQILogicHandler::getFFChessGoodsMallReply, this);
	
	m_msgHander->registerProtocol(OutsideClientSrv, XIANGQISRV_EXCHANGE_FF_CHESS_GOODS_REQ, (ProtocolHandler)&CFFQILogicHandler::exchangeFFChessGoods, this);
	m_msgHander->registerProtocol(DBProxyCommonSrv, BUSINESS_EXCHANGE_FF_CHESS_GOODS_RSP, (ProtocolHandler)&CFFQILogicHandler::exchangeFFChessGoodsReply, this);
	
	m_msgHander->registerProtocol(OutsideClientSrv, XIANGQISRV_USE_FF_CHESS_GOODS_REQ, (ProtocolHandler)&CFFQILogicHandler::useFFChessGoods, this);
	m_msgHander->registerProtocol(DBProxyCommonSrv, BUSINESS_USE_FF_CHESS_GOODS_RSP, (ProtocolHandler)&CFFQILogicHandler::useFFChessGoodsReply, this);
	
	m_msgHander->registerProtocol(OutsideClientSrv, XIANGQISRV_GET_FF_CHESS_USER_BASE_INFO_REQ, (ProtocolHandler)&CFFQILogicHandler::getUserBaseInfo, this);
	m_msgHander->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_FF_CHESS_USER_BASE_INFO_RSP, (ProtocolHandler)&CFFQILogicHandler::getUserBaseInfoReply, this);
	
	m_msgHander->registerProtocol(DBProxyCommonSrv, BUSINESS_SET_FF_CHESS_MATCH_RESULT_RSP, (ProtocolHandler)&CFFQILogicHandler::setUserMatchResultReply, this);

	m_msgHander->registerProtocol(OutsideClientSrv, XIANGQISRV_GET_FF_CHESS_RULE_INFO_REQ, (ProtocolHandler)&CFFQILogicHandler::getRuleInfo, this);
	
	m_msgHander->registerProtocol(OutsideClientSrv, XIANGQISRV_USER_CHAT_REQ, (ProtocolHandler)&CFFQILogicHandler::userChat, this);
	m_msgHander->registerProtocol(ServiceType::MessageSrv, MessageSrvProtocol::BUSINESS_CHAT_FILTER_RSP, (ProtocolHandler)&CFFQILogicHandler::userChatNotify, this);
	
	return true;
}

void CFFQILogicHandler::unLoad()
{
	
}

void CFFQILogicHandler::updateConfig()
{
	
}

void CFFQILogicHandler::onLine(XQUserData* ud)
{
	struct tm curDateTime;
	time_t curSecs = time(NULL);
	localtime_r(&curSecs, &curDateTime);
	
	com_protocol::XQSRVTaskData* taskData = ud->srvData->mutable_task_data();
	if (taskData->today() != (unsigned int)curDateTime.tm_yday)
	{
		taskData->clear_data();
		taskData->set_today(curDateTime.tm_yday);
	}
}

void CFFQILogicHandler::offLine(XQUserData* ud)
{
	
}


// 任务统计
void CFFQILogicHandler::doTaskStatistics(const XQChessPieces* xqCP, const short lostSide, const com_protocol::SettlementResult result, const char* username)
{
	if (xqCP == NULL || result == com_protocol::SettlementResult::ServiceUnload
	    || username == NULL || m_msgHander->getRobotManager().isRobot(username)) return;
	
	XQUserData* userData = m_msgHander->getUserData(username);
	if (userData == NULL || userData->srvData == NULL) return;

    com_protocol::XQSRVTask* srvTask = NULL;
	com_protocol::XQSRVTaskData* taskData = userData->srvData->mutable_task_data();
	const map<unsigned int, XiangQiConfig::ChessTaskConfig>& taskCfg = XiangQiConfig::config::getConfigValue().task_cfg;
	for (map<unsigned int, XiangQiConfig::ChessTaskConfig>::const_iterator taskCfgIt = taskCfg.begin(); taskCfgIt != taskCfg.end(); ++taskCfgIt)
	{
		srvTask = NULL;
		for (int idx = 0; idx < taskData->data_size(); ++idx)
		{
			if (taskData->data(idx).id() == taskCfgIt->first)
			{
				srvTask = taskData->mutable_data(idx);				
				break;
			}
		}
		
		if (srvTask == NULL)
		{
			srvTask = taskData->add_data();
			srvTask->set_id(taskCfgIt->first);
			srvTask->set_count(0);
			srvTask->set_status(0);
		}
		
		switch (taskCfgIt->second.type)
		{
			case EFFQiTaskType::ETaskWin:
			{
				// 非和棋则算输赢
				if ((short)userData->sideFlag != lostSide && result != com_protocol::SettlementResult::ToDoPeace && result != com_protocol::SettlementResult::PleasePeace)
				{
					srvTask->set_count(srvTask->count() + 1);
				}
				
				break;
			}
			
			case EFFQiTaskType::ETaskJoin:
			{
				srvTask->set_count(srvTask->count() + 1);
				
				break;
			}
			
			case EFFQiTaskType::ETaskContinueWin:
			{
				// 非和棋则算输赢
				if ((short)userData->sideFlag != lostSide && result != com_protocol::SettlementResult::ToDoPeace && result != com_protocol::SettlementResult::PleasePeace)
				{
					srvTask->set_count(srvTask->count() + 1);
				}
				else
				{
					srvTask->set_count(0);
				}
				
				break;
			}
			
			case EFFQiTaskType::ETaskEatGeneral:
			{
				if (xqCP->eatOpponentGeneral[userData->sideFlag] == (unsigned short)-1)
				{
					srvTask->set_count(srvTask->count() + 1);
				}
				
				break;
			}
			
			case EFFQiTaskType::ETaskOneContinueHit:
			{
				if (xqCP->maxContinueHitValue[userData->sideFlag] >= (short)taskCfgIt->second.value)
				{
					srvTask->set_count(srvTask->count() + 1);
				}
				
				break;
			}
			
			case EFFQiTaskType::ETaskOnePaoEatPieces:
			{
				if (xqCP->paoEatPiecesCount[userData->sideFlag] >= (unsigned short)taskCfgIt->second.value)
				{
					srvTask->set_count(srvTask->count() + 1);
				}
				
				break;
			}
			
			default:
			{
				break;
			}
		}
	}
}

// 充值成功通知
void CFFQILogicHandler::rechargeNotify(const char* username, const float money)
{
	if (username == NULL || m_msgHander->getRobotManager().isRobot(username)) return;
	
	XQUserData* userData = m_msgHander->getUserData(username);
	if (userData == NULL || userData->srvData == NULL) return;

    const unsigned int value = money;
    com_protocol::XQSRVTask* srvTask = NULL;
	com_protocol::XQSRVTaskData* taskData = userData->srvData->mutable_task_data();
	const map<unsigned int, XiangQiConfig::ChessTaskConfig>& taskCfg = XiangQiConfig::config::getConfigValue().task_cfg;
	for (map<unsigned int, XiangQiConfig::ChessTaskConfig>::const_iterator taskCfgIt = taskCfg.begin(); taskCfgIt != taskCfg.end(); ++taskCfgIt)
	{
		if (taskCfgIt->second.type != EFFQiTaskType::ETaskRecharge) continue;  // 忽略非充值类型
		
		srvTask = NULL;
		for (int idx = 0; idx < taskData->data_size(); ++idx)
		{
			if (taskData->data(idx).id() == taskCfgIt->first)
			{
				srvTask = taskData->mutable_data(idx);				
				break;
			}
		}
		
		if (srvTask == NULL)
		{
			srvTask = taskData->add_data();
			srvTask->set_id(taskCfgIt->first);
			srvTask->set_count(0);
			srvTask->set_status(0);
		}
		
		if (value >= taskCfgIt->second.value) srvTask->set_count(srvTask->count() + 1);
	}
}

// 设置禁足信息
unsigned int CFFQILogicHandler::setRestrictMoveInfo(XQChessPieces* xqCP, const short moveSrcPosition, const short moveDstPosition)
{
	if (xqCP == NULL) return 0;
	
	SRestrictMoveInfo& rstMvInfo = xqCP->restrictMoveInfo;
	if (rstMvInfo.srcPosition == moveSrcPosition)
	{
		++rstMvInfo.stepCount;
		rstMvInfo.srcPosition = moveDstPosition;
		
		return rstMvInfo.stepCount;
	}
	else if (rstMvInfo.dstPosition == moveSrcPosition)
	{
		rstMvInfo.dstPosition = moveDstPosition;
	}
	else
	{
		// 其他情况则重新开始运算
		clearRestrictMoveInfo(rstMvInfo);
		
		// 炮不算
		if (xqCP->value[moveSrcPosition] == com_protocol::EChessPiecesValue::EHeiQiPao
		    || xqCP->value[moveSrcPosition] == com_protocol::EChessPiecesValue::EHongQiPao) return 0;
		
		// 1）左边，2）右边，3）向上（最上一行棋子  0--08--16--24），4）向下（最下一行棋子  7--15--23--31）
		const short positionIndex[] = {-MaxDifferenceValue, MaxDifferenceValue, -1, 1};
		short moveIndex = 0;
		short cpValue = 0;
		short beEatPosition = -1;
		for (unsigned int idx = 0; idx < (sizeof(positionIndex) / sizeof(positionIndex[0])); ++idx)
		{
			moveIndex = moveDstPosition + positionIndex[idx];
			if (moveIndex >= MinChessPiecesPosition && moveIndex <= MaxChessPieCesPosition)
			{
				cpValue = xqCP->value[moveIndex];
				if (cpValue < com_protocol::EChessPiecesValue::ENoneQiZiClose
				    && !m_msgHander->isSameSide(xqCP->curOptSide, cpValue) && m_msgHander->canEatOtherChessPieces(xqCP->value[moveSrcPosition], cpValue))
				{
					if (beEatPosition != -1) return 0;  // 可以同时吃掉多个则不算
					
					beEatPosition = moveIndex;
				}
			}
		}
		
		// 移动后可以吃对方的一个棋子则记录信息
		if (beEatPosition != -1)
		{
			rstMvInfo.srcPosition = moveDstPosition;
			rstMvInfo.dstPosition = beEatPosition;
			rstMvInfo.stepCount = 1;
			rstMvInfo.robotChaseStep = getPositiveRandNumber(XiangQiConfig::config::getConfigValue().robot_cfg.min_chase_step, XiangQiConfig::config::getConfigValue().robot_cfg.max_chase_step);
		}
	}
	
	return 0;
}

// 清空禁足信息
void CFFQILogicHandler::clearRestrictMoveInfo(SRestrictMoveInfo& rstMvInfo)
{
	rstMvInfo.srcPosition = -1;
	rstMvInfo.dstPosition = -1;
	rstMvInfo.stepCount = 0;
	rstMvInfo.robotChaseStep = 1;
}
	
// 获取任务列表
void CFFQILogicHandler::getTaskInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHander->checkLoginIsSuccess("get task info")) return;
	 
	com_protocol::ClientXiangQiGetTaskRsp rsp;
	XQUserData* ud = m_msgHander->getUserData();
	const com_protocol::XQSRVTaskData& taskData = ud->srvData->task_data();
	
	unsigned int count = 0;
	unsigned int status = 0;
	const map<unsigned int, XiangQiConfig::ChessTaskConfig>& taskCfg = XiangQiConfig::config::getConfigValue().task_cfg;
	for (map<unsigned int, XiangQiConfig::ChessTaskConfig>::const_iterator taskCfgIt = taskCfg.begin(); taskCfgIt != taskCfg.end(); ++taskCfgIt)
	{
		count = 0;
	    status = 0;
		for (int idx = 0; idx < taskData.data_size(); ++idx)
		{
			if (taskData.data(idx).id() == taskCfgIt->first)
			{
				count = taskData.data(idx).count();
				status = taskData.data(idx).status();
				
				break;
			}
		}
		
		// if (status == 1) continue;  // 已经完成并且已经领取了奖励的任务不再显示
		
		com_protocol::ClientFFQITaskData* clientTaskData = rsp.add_client_task_data();
		clientTaskData->set_id(taskCfgIt->first);
		clientTaskData->set_picture(taskCfgIt->second.picture);
		clientTaskData->set_title(taskCfgIt->second.title);
		clientTaskData->set_amount(taskCfgIt->second.amount);
		clientTaskData->set_count(count);
        
		if (status == 1)  // 已经完成的任务
		{
			clientTaskData->set_status(com_protocol::EClientFFQITaskStatus::EAlreadyReceived);
		}
		else if (clientTaskData->count() >= clientTaskData->amount())
		{
			clientTaskData->set_status(com_protocol::EClientFFQITaskStatus::EToReceive);
		}
		else if (taskCfgIt->second.type != EFFQiTaskType::ETaskRecharge)
		{
			clientTaskData->set_status(com_protocol::EClientFFQITaskStatus::EToFinish);
		}
		else
		{
			clientTaskData->set_status(com_protocol::EClientFFQITaskStatus::EToRecharge);
		}
		
		for (map<unsigned int, unsigned int>::const_iterator taskGiftsCfgIt = taskCfgIt->second.gifts.begin(); taskGiftsCfgIt != taskCfgIt->second.gifts.end(); ++taskGiftsCfgIt)
		{
		    com_protocol::GiftInfo* gifts = clientTaskData->add_gift();
			gifts->set_type(taskGiftsCfgIt->first);
			gifts->set_num(taskGiftsCfgIt->second);
		}
	}
	
	m_msgHander->sendPkgMsgToClient(rsp, XIANGQISRV_GET_TASK_RSP, NULL, "get task info");
}

// 领取任务奖励
void CFFQILogicHandler::receiveTaskReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHander->checkLoginIsSuccess("get task info")) return;
	
	com_protocol::ClientXiangQiReceiveTaskRewardReq req;
	if (!m_msgHander->parseMsgFromBuffer(req, data, len, "receive task reward")) return;
	
	const bool isReceiveOneReward = req.has_id();  // 让编译器高效优化
	const map<unsigned int, XiangQiConfig::ChessTaskConfig>& taskCfg = XiangQiConfig::config::getConfigValue().task_cfg;
	map<unsigned int, XiangQiConfig::ChessTaskConfig>::const_iterator taskCfgIt;
	XQUserData* ud = m_msgHander->getUserData();
	com_protocol::XQSRVTaskData* taskData = ud->srvData->mutable_task_data();
	SReceiveTaskRewardInfo* rTskRwInfo = NULL;
	unsigned int tskRwInfoIndex = 0;
	map<unsigned int, unsigned int> rewardType2Count;
	for (int idx = 0; idx < taskData->data_size(); ++idx)
	{
		taskCfgIt = taskCfg.find(taskData->data(idx).id());
		if (taskData->data(idx).status() == 0 && taskCfgIt != taskCfg.end() && taskData->data(idx).count() >= taskCfgIt->second.amount)
		{
			if (isReceiveOneReward && req.id() != taskCfgIt->first) continue;
			
			taskData->mutable_data(idx)->set_status(1);

			// 可以领取的奖励
			for (map<unsigned int, unsigned int>::const_iterator taskGiftsCfgIt = taskCfgIt->second.gifts.begin(); taskGiftsCfgIt != taskCfgIt->second.gifts.end(); ++taskGiftsCfgIt)
			{
				if (taskGiftsCfgIt->second > 0) rewardType2Count[taskGiftsCfgIt->first] += taskGiftsCfgIt->second;
			}
			
			// 保存领取奖励的任务ID，异步应答消息回来时提取使用
			if (rTskRwInfo == NULL)
			{
				rTskRwInfo = (SReceiveTaskRewardInfo*)m_msgHander->getLocalAsyncData().createData();
				memset(rTskRwInfo, 0, sizeof(SReceiveTaskRewardInfo));
			}
			rTskRwInfo->recievedTaskId[tskRwInfoIndex++] = taskCfgIt->first;
			
			if (isReceiveOneReward) break;  // 只领取其中一个任务奖励的话则退出，否则继续遍历查找可以领取的奖励
		}
	}
	
	if (rTskRwInfo != NULL)
	{
		RecordItemVector recordItemVector;
		for (map<unsigned int, unsigned int>::const_iterator rewardType2CountIt = rewardType2Count.begin(); rewardType2CountIt != rewardType2Count.end(); ++rewardType2CountIt)
		{
			recordItemVector.push_back(RecordItem(rewardType2CountIt->first, rewardType2CountIt->second));
		}
		
		int rc = Opt_Failed;
		if (!recordItemVector.empty())
		{
			// 领取任务奖励
			rc = m_msgHander->notifyDbProxyChangeProp(ud->username, ud->usernameLen, recordItemVector, 0, "领取翻翻棋任务奖励", 0, "象棋翻翻", NULL, NULL, CUSTOM_RECEIVE_TASK_REWARD_REPLY);
		}
		
		if (rc != Opt_Success) m_msgHander->getLocalAsyncData().destroyData((const char*)rTskRwInfo);
	}
}

void CFFQILogicHandler::receiveTaskRewardReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	SReceiveTaskRewardInfo* rTskRwInfo = (SReceiveTaskRewardInfo*)m_msgHander->getLocalAsyncData().getData();
	if (rTskRwInfo == NULL)
	{
		OptErrorLog("receive task reward reply error, can not find the async data");
		return;
	}
	
	string userName;
	ConnectProxy* conn = m_msgHander->getConnectInfo("receive task reward reply", userName);
	if (conn == NULL)
	{
		m_msgHander->getLocalAsyncData().destroyData((const char*)rTskRwInfo);
		return;
	}
	
	com_protocol::PropChangeRsp propChangeRsp;
	if (!m_msgHander->parseMsgFromBuffer(propChangeRsp, data, len, "receive task reward reply"))
	{
		m_msgHander->getLocalAsyncData().destroyData((const char*)rTskRwInfo);
		return;
	}

	com_protocol::ClientXiangQiReceiveTaskRewardRsp receiveTaskRewardRsp;
	receiveTaskRewardRsp.set_result(propChangeRsp.result());
	
	if (propChangeRsp.result() == 0)
	{
		for (unsigned int idx = 0; idx < sizeof(rTskRwInfo->recievedTaskId) / sizeof(unsigned short); ++idx)
		{
			if (rTskRwInfo->recievedTaskId[idx] < 1) break;

			map<unsigned int, XiangQiConfig::ChessTaskConfig>::const_iterator taskCfgIt = XiangQiConfig::config::getConfigValue().task_cfg.find(rTskRwInfo->recievedTaskId[idx]);
			if (taskCfgIt != XiangQiConfig::config::getConfigValue().task_cfg.end())
			{
				receiveTaskRewardRsp.add_ids(taskCfgIt->first);
				for (map<unsigned int, unsigned int>::const_iterator taskGiftsCfgIt = taskCfgIt->second.gifts.begin(); taskGiftsCfgIt != taskCfgIt->second.gifts.end(); ++taskGiftsCfgIt)
				{
					com_protocol::GiftInfo* gift = receiveTaskRewardRsp.add_gift();
					gift->set_type(taskGiftsCfgIt->first);
					gift->set_num(taskGiftsCfgIt->second);
				}
			}
		}
		
		m_msgHander->sendGoodsChangeNotify(propChangeRsp.items(), conn);
	}
	else
	{
		OptErrorLog("receive task reward error, result = %d", propChangeRsp.result());
	}
	
	const char* msgData = m_msgHander->serializeMsgToBuffer(receiveTaskRewardRsp, "receive task reward, goods change notify");
	if (msgData != NULL) m_msgHander->sendMsgToProxy(msgData, receiveTaskRewardRsp.ByteSize(), XIANGQISRV_RECEIVE_TASK_REWARD_RSP, conn);
	
	m_msgHander->getLocalAsyncData().destroyData((const char*)rTskRwInfo);
}

void CFFQILogicHandler::getGoodsMallCfg(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHander->checkLoginIsSuccess("get goods mall config")) return;
	
    m_msgHander->sendMessageToCommonDbProxy(data, len, CommonDBSrvProtocol::BUSINESS_GET_GAME_MALL_CONFIG_REQ);
}

void CFFQILogicHandler::getGoodsMallCfgReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = m_msgHander->getConnectInfo("get goods mall config reply", userName);
	if (conn == NULL) return;
	
	m_msgHander->sendMsgToProxy(data, len, XIANGQISRV_GET_FF_CHESS_MALL_CONFIG_RSP, conn);
}

// 游戏道具商城配置
void CFFQILogicHandler::getFFChessGoodsMall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHander->checkLoginIsSuccess("get ff chess goods mall")) return;
	
    m_msgHander->sendMessageToCommonDbProxy(data, len, CommonDBSrvProtocol::BUSINESS_GET_FF_CHESS_MALL_REQ);
}

void CFFQILogicHandler::getFFChessGoodsMallReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = m_msgHander->getConnectInfo("get ff chess goods mall reply", userName);
	if (conn == NULL) return;
	
	m_msgHander->sendMsgToProxy(data, len, XIANGQISRV_GET_FF_CHESS_GOODS_MALL_RSP, conn);
}

void CFFQILogicHandler::exchangeFFChessGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHander->checkLoginIsSuccess("exchange ff chess goods")) return;
	
    m_msgHander->sendMessageToCommonDbProxy(data, len, CommonDBSrvProtocol::BUSINESS_EXCHANGE_FF_CHESS_GOODS_REQ);
}

void CFFQILogicHandler::exchangeFFChessGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = m_msgHander->getConnectInfo("exchange ff chess goods reply", userName);
	if (conn == NULL) return;
	
	com_protocol::ExchangeFFChessItemRsp rsp;
	if (!m_msgHander->parseMsgFromBuffer(rsp, data, len, "exchange ff chess goods reply")) return;
	
	if (rsp.result() == Opt_Success)
	{
	    XQUserData* ud = m_msgHander->getUserData(userName.c_str());
		if (rsp.type() == com_protocol::FFChessExchangeType::EFFChessBoard) ud->chessBoardId = rsp.ff_chess_item().id();
		else if (rsp.type() == com_protocol::FFChessExchangeType::EFFChessPieces) ud->chessPiecesId = rsp.ff_chess_item().id();
	}
	
	m_msgHander->sendMsgToProxy(data, len, XIANGQISRV_EXCHANGE_FF_CHESS_GOODS_RSP, conn);
}

void CFFQILogicHandler::useFFChessGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHander->checkLoginIsSuccess("use ff chess goods")) return;
	
    m_msgHander->sendMessageToCommonDbProxy(data, len, CommonDBSrvProtocol::BUSINESS_USE_FF_CHESS_GOODS_REQ);
}

void CFFQILogicHandler::useFFChessGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = m_msgHander->getConnectInfo("use ff chess goods reply", userName);
	if (conn == NULL) return;
	
	com_protocol::SelectUseFFChessItemRsp rsp;
	if (!m_msgHander->parseMsgFromBuffer(rsp, data, len, "use ff chess goods reply")) return;
	
	if (rsp.result() == Opt_Success)
	{
	    XQUserData* ud = m_msgHander->getUserData(userName.c_str());
		if (rsp.type() == com_protocol::FFChessExchangeType::EFFChessBoard) ud->chessBoardId = rsp.id();
		else if (rsp.type() == com_protocol::FFChessExchangeType::EFFChessPieces) ud->chessPiecesId = rsp.id();
	}
	
	m_msgHander->sendMsgToProxy(data, len, XIANGQISRV_USE_FF_CHESS_GOODS_RSP, conn);
}

void CFFQILogicHandler::getUserBaseInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHander->checkLoginIsSuccess("get user base info")) return;
	
	m_msgHander->sendMessageToCommonDbProxy(data, len, CommonDBSrvProtocol::BUSINESS_GET_FF_CHESS_USER_BASE_INFO_REQ);
}

void CFFQILogicHandler::getUserBaseInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	string userName;
	ConnectProxy* conn = m_msgHander->getConnectInfo("get user base info reply", userName);
	if (conn == NULL) return;
	
	m_msgHander->sendMsgToProxy(data, len, XIANGQISRV_GET_FF_CHESS_USER_BASE_INFO_RSP, conn);
}

void CFFQILogicHandler::setUserMatchResultReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetFFChessUserMatchResultRsp rsp;
	if (!m_msgHander->parseMsgFromBuffer(rsp, data, len, "set user match result reply")) return;
	
	for (int idx = 0; idx < rsp.opt_result_size(); ++idx)
	{
		if (rsp.opt_result(idx).result() != 0) OptErrorLog("set user match result reply error, user = %s, result = %d", rsp.opt_result(idx).username().c_str(), rsp.opt_result(idx).result());
	}
}

// 游戏规则&说明信息
void CFFQILogicHandler::getRuleInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHander->checkLoginIsSuccess("get rule info request")) return;
	
	const XiangQiConfig::RuleInfo& ruleInfo = XiangQiConfig::config::getConfigValue().rule_info;
	com_protocol::ClientGetFFChessRuleInfoRsp rsp;
	rsp.set_title(ruleInfo.title);
	rsp.set_content(ruleInfo.content);
	m_msgHander->sendPkgMsgToClient(rsp, XiangQiSrvProtocol::XIANGQISRV_GET_FF_CHESS_RULE_INFO_RSP);
}

// 游戏内同桌聊天
void CFFQILogicHandler::userChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHander->checkLoginIsSuccess("user chat request")) return;
	
	XQUserData* xqUD = m_msgHander->getUserData();
	m_msgHander->sendMessageToService(ServiceType::MessageSrv, data, len, MessageSrvProtocol::BUSINESS_CHAT_FILTER_REQ, xqUD->username, xqUD->usernameLen);
}

void CFFQILogicHandler::userChatNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	XQChessPieces* xqCP = m_msgHander->getXQChessPieces(m_msgHander->getContext().userData);
	if (xqCP == NULL) return;  // 棋局已经结束
	
	com_protocol::ChatFilterRsp rsp;
	if (!m_msgHander->parseMsgFromBuffer(rsp, data, len, "user chat reply")) return;

	com_protocol::ClientFFChessUserChatNotify chatNotify;
	chatNotify.set_chat_content(rsp.filter_chat_content());
	chatNotify.set_src_username(m_msgHander->getContext().userData);
	if (rsp.has_message_id()) chatNotify.set_message_id(rsp.message_id());
	
	m_msgHander->sendPkgMsgToPlayers(chatNotify, XIANGQISRV_USER_CHAT_NOTIFY, xqCP, "user chat reply");
}

}
