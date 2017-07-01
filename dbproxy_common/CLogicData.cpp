
/* author : limingfan
 * date : 2016.03.30
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

#include "CSrvMsgHandler.h"
#include "CLogicData.h"
#include "base/Function.h"
#include "../common/CommonType.h"


using namespace NProject;


CLogicData::CLogicData()
{
	m_msgHandler = NULL;
}

CLogicData::~CLogicData()
{
	m_msgHandler = NULL;
}

int CLogicData::init(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;
	
    // 设置服务逻辑数据
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_SET_SERVICE_LOGIC_DATA_REQ, (ProtocolHandler)&CLogicData::setServiceLogicData, this);

	setTaskTimer(0, 0, NULL, 0); // 启动定时器执行定时任务
	
	return 0;
}

void CLogicData::unInit()
{
	saveLogicData();  // 保存逻辑数据到redis
}


void CLogicData::setServiceLogicData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetGameLogicDataRsp setLogicDataRsp;
	com_protocol::SetGameLogicDataReq setLogicDataReq;
	setLogicDataRsp.set_result(ServiceSuccess);
	
	int rc = ServiceFailed;
	if (m_msgHandler->parseMsgFromBuffer(setLogicDataReq, data, len, "set service logic data"))
	{
		rc = m_msgHandler->getLogicRedisService().setHField(setLogicDataReq.primary_key().c_str(), setLogicDataReq.primary_key().length(),
														    setLogicDataReq.second_key().c_str(), setLogicDataReq.second_key().length(),
															setLogicDataReq.data().c_str(), setLogicDataReq.data().length());
	    if (rc != 0)
		{
			setLogicDataRsp.set_result(ServiceSetLogicDataError);
			OptErrorLog("set service logic data to redis error, primary key = %s, second key = %s, data len = %u, rc = %d",
			setLogicDataReq.primary_key().c_str(), setLogicDataReq.second_key().c_str(), len, rc);
		}
	}
	else
	{
		setLogicDataRsp.set_result(ServiceParseFromArrayError);
	}
	
	setLogicDataRsp.set_primary_key(setLogicDataReq.primary_key());
	setLogicDataRsp.set_second_key(setLogicDataReq.second_key());

	rc = ServiceFailed;
	char send_data[MaxMsgLen] = {0};
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(setLogicDataRsp, send_data, send_data_len, "set service logic data");
	if (msgData != NULL)
	{
		if (srcProtocolId == 0) srcProtocolId = BUSINESS_SET_SERVICE_LOGIC_DATA_RSP;
		rc = m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, srcProtocolId);
	}
	
	if (setLogicDataRsp.result() != 0 || rc != 0)
	{
		OptErrorLog("set service logic data error, srcSrvId = %u, srcProtocolId = %d, msg byte len = %d, buff len = %d, result = %d, rc = %d",
		srcSrvId, srcProtocolId, setLogicDataRsp.ByteSize(), MaxMsgLen, setLogicDataRsp.result(), rc);
	}
}

const ServiceLogicData& CLogicData::getLogicData(const char* userName, unsigned int len)
{
	UserToLogicData::iterator it = m_user2LogicData.find(userName);
	if (it != m_user2LogicData.end()) return it->second;
	
	m_user2LogicData[userName] = ServiceLogicData();
	ServiceLogicData& srvData = m_user2LogicData[userName];
	srvData.isUpdate = 0;
	
	char msgBuffer[MaxMsgLen] = {0};
	int gameDataLen = m_msgHandler->getLogicRedisService().getHField(DBCommonLogicDataKey, DBCommonLogicDataKeyLen, userName, len, msgBuffer, MaxMsgLen);
	if (gameDataLen > 0)
	{
		if (!srvData.logicData.ParseFromArray(msgBuffer, gameDataLen))
		{
			// 出错了只能初始化清空数据重新再来
			OptErrorLog("getLogicData, ParseFromArray message error, user = %s, msg byte len = %d, buff len = %d", userName, srvData.logicData.ByteSize(), gameDataLen);
			srvData.logicData.Clear();
			srvData.isUpdate = 1;
		}
	}
	
	return srvData;
}

ServiceLogicData& CLogicData::setLogicData(const char* userName, unsigned int len)
{
	ServiceLogicData& srvData = (ServiceLogicData&)getLogicData(userName, len);
	srvData.isUpdate = 1;  // update logic data
	return srvData;
}

const com_protocol::FFChessGoodsData& CLogicData::getFFChessGoods(const char* userName, unsigned int len)
{
	return getLogicData(userName, len).logicData.ff_chess_goods();
}

void CLogicData::saveLogicData()
{
	char msgBuffer[MaxMsgLen] = {0};
	unsigned int msgLen = 0;
	for (UserToLogicData::iterator it = m_user2LogicData.begin(); it != m_user2LogicData.end(); ++it)
	{
		if (it->second.isUpdate == 1)
		{
			msgLen = MaxMsgLen;
			const char* msgData = m_msgHandler->serializeMsgToBuffer(it->second.logicData, msgBuffer, msgLen, "save user logic data to redis");
			if (msgData != NULL)
			{
				int rc = m_msgHandler->getLogicRedisService().setHField(DBCommonLogicDataKey, DBCommonLogicDataKeyLen, it->first.c_str(), it->first.length(), msgData, msgLen);
				if (rc != 0) OptErrorLog("save user logic data to redis error, user = %s, data len = %u, rc = %d", it->first.c_str(), msgLen, rc);
			}
		}
	}
	
	m_user2LogicData.clear();
}

// 任务定时器
void CLogicData::setTaskTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	OptInfoLog("task timer begin, current timer id = %u, remain count = %u", timerId, remainCount);
	
	// 执行定时任务
	saveLogicData();

	// 重新设置定时器
	struct tm tmval;
	time_t curSecs = time(NULL);
	unsigned int intervalSecs = 60 * 60 * 24;
	if (localtime_r(&curSecs, &tmval) != NULL)
	{
		tmval.tm_sec = 0;
		tmval.tm_min = 0;
		tmval.tm_hour = atoi(CCfg::getValue("Service", "TaskTimerHour"));
		++tmval.tm_mday;
		time_t nextSecs = mktime(&tmval);
		if (nextSecs != (time_t)-1)
		{
			intervalSecs = nextSecs - curSecs;
		}
		else
		{
			OptErrorLog("setTaskTimer, get next time error");
		}
	}
	else
	{
		OptErrorLog("setTaskTimer, get local time error");
	}

	unsigned int tId = m_msgHandler->setTimer(intervalSecs * 1000, (TimerHandler)&CLogicData::setTaskTimer, 0, NULL, 1, this);	
	OptInfoLog("task timer end, next timer id = %u, interval = %u, year = %d, month = %d, day = %d, hour = %d, min = %d, sec = %d",
	tId, intervalSecs, tmval.tm_year + 1900, tmval.tm_mon + 1, tmval.tm_mday, tmval.tm_hour, tmval.tm_min, tmval.tm_sec);
}

void CLogicData::rechargeFishCoin(const com_protocol::UserRechargeReq& req)
{
	if (req.item_flag() == MallItemFlag::First)
	{
		com_protocol::DBCommonSrvLogicData& logicData = setLogicData(req.username().c_str(), req.username().length()).logicData;
		com_protocol::RechargeInfo* rcInfo = logicData.mutable_recharge();
		rcInfo->add_recharged_id(req.item_id());
	}
}

void CLogicData::exchangeBatteryEquipment(const char* userName, unsigned int len, const unsigned int id)
{
	com_protocol::DBCommonSrvLogicData& logicData = setLogicData(userName, len).logicData;
	com_protocol::BatteryEquipment* btEquip = logicData.mutable_battery_equipment();
	btEquip->add_ids(id);
}

bool CLogicData::fishCoinIsRecharged(const char* userName, const unsigned int len, const unsigned int id)
{
	const com_protocol::DBCommonSrvLogicData& logicData = getLogicData(userName, len).logicData;
	if (logicData.has_recharge() && logicData.recharge().recharged_id_size() > 0)
	{
		for (int idx = 0; idx < logicData.recharge().recharged_id_size(); ++idx)
		{
			if (logicData.recharge().recharged_id(idx) == id) return true;  // 首次充值的物品标识
		}
	}
	
	return false;
}

void CLogicData::addFFChessGoods(const unsigned int chessItemId, const com_protocol::FFChessExchangeType type, const unsigned int useDay, com_protocol::FFChessGoodsData* ffChessData)
{
	time_t validTime = 0;  // 永久有效
	if (useDay > 0)
	{
		struct tm tmval;
	    validTime = time(NULL);
	    localtime_r(&validTime, &tmval);
		tmval.tm_hour = 0;
		tmval.tm_min = 0;
		tmval.tm_sec = 0;
		tmval.tm_mday += (useDay + 1);
		validTime = mktime(&tmval);  // 截止有效时间
	}
	
	com_protocol::FFChessGoodsInfo* chessGoodsInfo = (type == com_protocol::FFChessExchangeType::EFFChessBoard) ? ffChessData->add_chess_board() : ffChessData->add_chess_pieces();
	chessGoodsInfo->set_id(chessItemId);
	chessGoodsInfo->set_valid_time(validTime);
	chessGoodsInfo->set_use_day(useDay);
	
	(type == com_protocol::FFChessExchangeType::EFFChessBoard) ? ffChessData->set_current_board_id(chessItemId) : ffChessData->set_current_pieces_id(chessItemId);
}

void CLogicData::exchangeFFChessGoods(const char* userName, unsigned int len, const unsigned int useDay, const com_protocol::ExchangeFFChessItemRsp& info)
{
	com_protocol::FFChessGoodsData* ffChessData = setLogicData(userName, len).logicData.mutable_ff_chess_goods();
	if (info.has_ff_chess_item())
	{
		addFFChessGoods(info.ff_chess_item().id(), info.type(), useDay, ffChessData);
	}
	else if (info.has_gift_package())
	{
		const com_protocol::FFChessGiftPackage& ffChessGiftPackage = info.gift_package();
		if (ffChessGiftPackage.gift_chess_board_size() > 0) addFFChessGoods(ffChessGiftPackage.gift_chess_board(0).id(), com_protocol::FFChessExchangeType::EFFChessBoard, useDay, ffChessData);
		if (ffChessGiftPackage.gift_chess_pieces_size() > 0) addFFChessGoods(ffChessGiftPackage.gift_chess_pieces(0).id(), com_protocol::FFChessExchangeType::EFFChessPieces, useDay, ffChessData);
	}
}

// 删除过期的翻翻棋道具
const ServiceLogicData& CLogicData::updateFFChessGoods(const char* userName, unsigned int len)
{
	vector<unsigned int> rmBoardIds;
	vector<unsigned int> rmPiecesIds;
	
	const ServiceLogicData& currentLogicData = getLogicData(userName, len);
	
	
	// 这里目前先清空老数据，做兼容处理，此处代码等新数据覆盖后需要删除掉
	if (currentLogicData.logicData.has_ff_chess_goods())
	{
		bool isClearFFChessGoods = false;
		const google::protobuf::RepeatedPtrField<com_protocol::FFChessGoodsInfo>& cbInfo = currentLogicData.logicData.ff_chess_goods().chess_board();
		for (google::protobuf::RepeatedPtrField<com_protocol::FFChessGoodsInfo>::const_iterator ffcGoodsInfo = cbInfo.begin(); ffcGoodsInfo != cbInfo.end(); ++ffcGoodsInfo)
		{
			if (!ffcGoodsInfo->has_use_day())  // 老数据则清空
			{
				isClearFFChessGoods = true;
				setLogicData(userName, len).logicData.clear_ff_chess_goods();
				break;
			}
		}
		
		if (!isClearFFChessGoods)
		{
			const google::protobuf::RepeatedPtrField<com_protocol::FFChessGoodsInfo>& cpInfo = currentLogicData.logicData.ff_chess_goods().chess_pieces();
			for (google::protobuf::RepeatedPtrField<com_protocol::FFChessGoodsInfo>::const_iterator ffcGoodsInfo = cpInfo.begin(); ffcGoodsInfo != cpInfo.end(); ++ffcGoodsInfo)
			{
				if (!ffcGoodsInfo->has_use_day())  // 老数据则清空
				{
					setLogicData(userName, len).logicData.clear_ff_chess_goods();
					break;
				}
			}
		}
	}
	
	
	if (currentLogicData.logicData.has_ff_chess_goods())
	{
		const unsigned int curSecs = time(NULL);
		unsigned int validTime = 0;
		const com_protocol::FFChessGoodsData& ffChessGoodsData = currentLogicData.logicData.ff_chess_goods();
		for (int idx = 0; idx < ffChessGoodsData.chess_board_size(); ++idx)  // 棋盘
		{
			validTime = ffChessGoodsData.chess_board(idx).valid_time();
			if (validTime > 0 && validTime < curSecs)
			{
				rmBoardIds.push_back(ffChessGoodsData.chess_board(idx).id());
			}
		}
		
		if (!rmBoardIds.empty())
		{
			// 删除过期的棋盘
			com_protocol::DBCommonSrvLogicData& logicData = setLogicData(userName, len).logicData;
			com_protocol::FFChessGoodsData* rmFFChessGoodsData = logicData.mutable_ff_chess_goods();
			for (vector<unsigned int>::const_iterator it = rmBoardIds.begin(); it != rmBoardIds.end(); ++it)
			{
				for (int idx = 0; idx < rmFFChessGoodsData->chess_board_size(); ++idx)  // 棋盘
				{
					if (*it == rmFFChessGoodsData->chess_board(idx).id())
					{
						rmFFChessGoodsData->mutable_chess_board()->DeleteSubrange(idx, 1);
						break;
					}
				}
			}
		}
		
		for (int idx = 0; idx < ffChessGoodsData.chess_pieces_size(); ++idx)  // 棋子
		{
			validTime = ffChessGoodsData.chess_pieces(idx).valid_time();
			if (validTime > 0 && validTime < curSecs)
			{
				rmPiecesIds.push_back(ffChessGoodsData.chess_pieces(idx).id());
			}
		}
		
		if (!rmPiecesIds.empty())
		{
			// 删除过期的棋子
			com_protocol::DBCommonSrvLogicData& logicData = setLogicData(userName, len).logicData;
			com_protocol::FFChessGoodsData* rmFFChessGoodsData = logicData.mutable_ff_chess_goods();
			for (vector<unsigned int>::const_iterator it = rmPiecesIds.begin(); it != rmPiecesIds.end(); ++it)
			{
				for (int idx = 0; idx < rmFFChessGoodsData->chess_pieces_size(); ++idx)  // 棋子
				{
					if (*it == rmFFChessGoodsData->chess_pieces(idx).id())
					{
						rmFFChessGoodsData->mutable_chess_pieces()->DeleteSubrange(idx, 1);
						break;
					}
				}
			}
		}
	}
	else
	{
		// 第一次则新增加默认道具
		// 默认的棋盘&棋子ID值
		const unsigned int defaultChessBoardId = 11001;
		const unsigned int defaultChessPiecesId = 12001;
		com_protocol::FFChessGoodsData* addDefaultFFChessData = setLogicData(userName, len).logicData.mutable_ff_chess_goods();
		addFFChessGoods(defaultChessBoardId, com_protocol::FFChessExchangeType::EFFChessBoard, 0, addDefaultFFChessData);
		addFFChessGoods(defaultChessPiecesId, com_protocol::FFChessExchangeType::EFFChessPieces, 0, addDefaultFFChessData);
	}
	
	return currentLogicData;
}


