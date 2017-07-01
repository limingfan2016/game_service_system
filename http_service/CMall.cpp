
/* author : limingfan
 * date : 2015.12.09
 * description : 商城管理
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "CMall.h"
#include "CHttpSrv.h"
#include "base/ErrorCode.h"
#include "common/CommonType.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NProject;

namespace NPlatformService
{

CMall::CMall()
{
	m_msgHandler = NULL;
	// m_firstRechargeItemId = 0;
}

CMall::~CMall()
{
	m_msgHandler = NULL;
	// m_firstRechargeItemId = 0;
}

bool CMall::load(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;
	
	// 老版本，商城配置
	m_msgHandler->registerProtocol(CommonSrv, GET_MALL_CONFIG_REQ, (ProtocolHandler)&CMall::getMallConfig, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_MALL_CONFIG_RSP, (ProtocolHandler)&CMall::getMallConfigReply, this);
	
	/*
	m_msgHandler->registerProtocol(CommonSrv, GET_GAME_MALL_CONFIG_REQ, (ProtocolHandler)&CMall::getGameMallConfig, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_GAME_MALL_CONFIG_RSP, (ProtocolHandler)&CMall::getGameMallConfigReply, this);
	
	// 首冲礼包
	m_msgHandler->registerProtocol(CommonSrv, GET_FIRST_RECHARGE_PACKAGE_REQ, (ProtocolHandler)&CMall::getFirstRechargeInfo, this);
	
	m_msgHandler->registerProtocol(CommonSrv, EXCHANGE_GAME_GOODS_REQ, (ProtocolHandler)&CMall::exchangeGameGoods, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_EXCHANGE_GAME_GOODS_RSP, (ProtocolHandler)&CMall::exchangeGameGoodsReply, this);
	
	m_msgHandler->registerProtocol(CommonSrv, GET_GAME_COMMON_CONFIG_REQ, (ProtocolHandler)&CMall::getGameCommonConfig, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_GET_GAME_COMMON_CONFIG_RSP, (ProtocolHandler)&CMall::getGameCommonConfigReply, this);
	
	
	// 超值礼包
	m_msgHandler->registerProtocol(CommonSrv, GET_SUPER_VALUE_PACKAGE_REQ, (ProtocolHandler)&CMall::getSuperValuePackageInfo, this);
	m_msgHandler->registerProtocol(CommonSrv, RECEIVE_SUPER_VALUE_PACKAGE_GIFT_REQ, (ProtocolHandler)&CMall::receiveSuperValuePackageGift, this);
	m_msgHandler->registerProtocol(DBProxyCommonSrv, BUSINESS_RECEIVE_SUPER_VALUE_PACKAGE_RSP, (ProtocolHandler)&CMall::receiveSuperValuePackageGiftReply, this);
    */
	
	
	return true;
}


/*
void CMall::unload()
{
	
}

unsigned int CMall::getItemFlag(const char* userName, const unsigned int userNameLen, const unsigned int id)
{
	unsigned int flag = MallItemFlag::NoFlag;
	if (id == m_firstRechargeItemId)  // 首次充值的物品标识
	{
		const com_protocol::HttpSrvLogicData& logicData = m_msgHandler->getLogicData(userName, userNameLen).logicData;
		if (!logicData.has_recharge() || logicData.recharge().flag() != RechargeFlag::First) flag = MallItemFlag::First;
	}
	return flag;
}
*/


bool CMall::getItemInfo(const int type, com_protocol::ItemInfo& itemInfo)
{
	// 充值物品大类型
	enum RechargeItemType
	{
		Gold = 0,      // 金币
		Item = 1,      // 道具
	};
		
	if (type == RechargeItemType::Gold)
	{
		for (int i = 0; i < m_mallCfg.gold_list_size(); ++i)
		{
			const com_protocol::ItemInfo& iInfo = m_mallCfg.gold_list(i).gold();
			if (iInfo.id() == itemInfo.id())
			{
				itemInfo.set_buy_price(iInfo.buy_price());
				itemInfo.set_num(iInfo.num());
				itemInfo.set_flag(iInfo.flag());
				return true;
			}
		}
	}
	else if (type == RechargeItemType::Item)
	{
		for (int i = 0; i < m_mallCfg.item_list_size(); ++i)
		{
			const com_protocol::ItemInfo& iInfo = m_mallCfg.item_list(i).item();
			if (iInfo.id() == itemInfo.id())
			{
				itemInfo.set_buy_price(iInfo.buy_price());
				itemInfo.set_num(iInfo.num());
				itemInfo.set_flag(iInfo.flag());
				return true;
			}
		}
	}
	
	OptWarnLog("can not find the item info, type = %d, id = %d", type, itemInfo.id());
	
	return false;
}

void CMall::getMallConfig(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	int rc = m_msgHandler->sendMessageToCommonDbProxy(data, len, BUSINESS_GET_MALL_CONFIG_REQ, m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, srcSrvId);
	OptWarnLog("get old version mall data request, user = %s, srcSrvId = %u, rc = %d", m_msgHandler->getContext().userData, srcSrvId, rc);
}

void CMall::getMallConfigReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->parseMsgFromBuffer(m_mallCfg, data, len, "get mall config reply")) return;

	unsigned int dataLen = len;
	for (int i = 0; i < m_mallCfg.gold_list_size(); ++i)
	{
		com_protocol::ItemInfo& iInfo = (com_protocol::ItemInfo&)m_mallCfg.gold_list(i).gold();
		if (iInfo.flag() >= MallItemFlag::FirstNoFlag)  // 首次充值的物品标识
		{
			/*
			m_firstRechargeItemId = iInfo.id();
			
			const com_protocol::HttpSrvLogicData& logicData = m_msgHandler->getLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen).logicData;
			unsigned int flag = (logicData.has_recharge() && logicData.recharge().flag() == RechargeFlag::First) ? (iInfo.flag() - MallItemFlag::FirstNoFlag) : MallItemFlag::First;
			*/
 
			iInfo.set_flag(MallItemFlag::First);
			
			data = m_msgHandler->serializeMsgToBuffer(m_mallCfg, "get mall config reply");
			dataLen = m_mallCfg.ByteSize();
			
			break;
		}
	}
	
	int rc = -1;
	if (data != NULL) rc = m_msgHandler->sendMessage(data, dataLen, m_msgHandler->getContext().userFlag, GET_MALL_CONFIG_RSP);
	OptWarnLog("get old version mall data reply, user = %s, srcSrvId = %u, rc = %d", m_msgHandler->getContext().userData, m_msgHandler->getContext().userFlag, rc);
}


/*
const com_protocol::FishCoinInfo* CMall::getFishCoinInfo(const unsigned int id)
{
	for (int i = 0; i < m_gameMallCfg.fish_coin_list_size(); ++i)
	{
		const com_protocol::FishCoinInfo& fcInfo = m_gameMallCfg.fish_coin_list(i);
		if (fcInfo.id() == id) return &fcInfo;
	}
	
	return (m_gameMallCfg.first_recharge_pkg().id() == id) ? &(m_gameMallCfg.first_recharge_pkg()) : NULL;
}

bool CMall::fishCoinIsRecharged(const char* userName, const unsigned int len, const unsigned int id)
{
	const com_protocol::HttpSrvLogicData& logicData = m_msgHandler->getLogicData(userName, len).logicData;
	if (logicData.has_recharge() && logicData.recharge().recharged_id_size() > 0)
	{
		for (int idx = 0; idx < logicData.recharge().recharged_id_size(); ++idx)
		{
			if (logicData.recharge().recharged_id(idx) == id) return true;  // 首次充值的物品标识
		}
	}
	
	return false;
}

void CMall::getGameMallConfig(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_msgHandler->sendMessageToCommonDbProxy(data, len, BUSINESS_GET_GAME_MALL_CONFIG_REQ, m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, srcSrvId);
}

void CMall::getGameMallConfigReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (!m_msgHandler->parseMsgFromBuffer(m_gameMallCfg, data, len, "get game mall config reply")) return;
	
	const com_protocol::HttpSrvLogicData& logicData = m_msgHandler->getLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen).logicData;
	const bool hasRechargeData = (logicData.has_recharge() && logicData.recharge().recharged_id_size() > 0);
	for (int i = 0; i < m_gameMallCfg.fish_coin_list_size(); ++i)
	{
		com_protocol::FishCoinInfo& fcInfo = (com_protocol::FishCoinInfo&)m_gameMallCfg.fish_coin_list(i);
		if (fcInfo.flag() >= MallItemFlag::FirstNoFlag)
		{
			fcInfo.set_flag(MallItemFlag::First);
			if (hasRechargeData)
			{
				for (int idx = 0; idx < logicData.recharge().recharged_id_size(); ++idx)
				{
					if (logicData.recharge().recharged_id(idx) == fcInfo.id())
					{
						fcInfo.set_flag(fcInfo.flag() - MallItemFlag::FirstNoFlag);
						break;
					}
				}
			}
		}
	}
	
	if (logicData.has_battery_equipment() && logicData.battery_equipment().ids_size() > 0)
	{
		const com_protocol::BatteryEquipment& btyEquip = logicData.battery_equipment();
		for (int i = 0; i < btyEquip.ids_size(); ++i)
		{
			for (int idx = 0; idx < m_gameMallCfg.battery_equipments_size(); ++idx)
			{
				if (m_gameMallCfg.battery_equipments(idx).id() == btyEquip.ids(i))
				{
					m_gameMallCfg.mutable_battery_equipments()->DeleteSubrange(idx, 1);
					break;
				}
			}
		}
	}
	
    data = m_msgHandler->serializeMsgToBuffer(m_gameMallCfg, "get fish coin mall config reply");
	if (data != NULL) m_msgHandler->sendMessage(data, m_gameMallCfg.ByteSize(), m_msgHandler->getContext().userFlag, GET_GAME_MALL_CONFIG_RSP);
}

// 获取首冲礼包信息
void CMall::getFirstRechargeInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetFirstPackageInfoRsp rsp;
	rsp.set_result(NonExistentFirstPackage);
	
	const com_protocol::FishCoinInfo& firstRechargePkg = m_gameMallCfg.first_recharge_pkg();
	if (!fishCoinIsRecharged(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, firstRechargePkg.id()))
	{
		rsp.set_result(OptSuccess);
		com_protocol::FirstRechargePackage* firstPkg = rsp.mutable_pkg_info();
		firstPkg->set_id(firstRechargePkg.id());
		firstPkg->set_num(firstRechargePkg.num());
		firstPkg->set_buy_price(firstRechargePkg.buy_price());
		firstPkg->set_times(firstRechargePkg.flag());
	}

	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, "get first package info");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, rsp.ByteSize(), srcSrvId, GET_FIRST_RECHARGE_PACKAGE_RSP);
}

// 兑换商城物品
void CMall::exchangeGameGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_msgHandler->sendMessageToCommonDbProxy(data, len, BUSINESS_EXCHANGE_GAME_GOODS_REQ, m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, srcSrvId);
}

void CMall::exchangeGameGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ExchangeGameGoogsRsp exchangeGameGoodsRsp;
	if (!m_msgHandler->parseMsgFromBuffer(exchangeGameGoodsRsp, data, len, "exchange game goods reply")) return;
	
	// 是否成功兑换炮台装备
	while (exchangeGameGoodsRsp.result() == OptSuccess && exchangeGameGoodsRsp.type() == ExchangeType::EBatteryEquip)
	{
		com_protocol::HttpSrvLogicData& logicData = m_msgHandler->setLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen).logicData;
        com_protocol::BatteryEquipment* btEquip = logicData.mutable_battery_equipment();
		btEquip->add_ids(exchangeGameGoodsRsp.id());
		break;
	}

	m_msgHandler->sendMessage(data, len, m_msgHandler->getContext().userFlag, EXCHANGE_GAME_GOODS_RSP);
}

// 获取公共配置
void CMall::getGameCommonConfig(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	m_msgHandler->sendMessageToCommonDbProxy(data, len, BUSINESS_GET_GAME_COMMON_CONFIG_REQ, m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, srcSrvId);
}

void CMall::getGameCommonConfigReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetGameCommonConfigRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "get game common config reply")) return;
	
	// 是否是炮台装备
	unsigned int msgLen = len;
	while (rsp.result() == OptSuccess && rsp.type() == ECommonConfigType::EBatteryEquipCfg)
	{
		const com_protocol::HttpSrvLogicData& logicData = m_msgHandler->getLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen).logicData;		
		if (logicData.has_battery_equipment() && logicData.battery_equipment().ids_size() > 0)
		{
			const com_protocol::BatteryEquipment& btyEquip = logicData.battery_equipment();
			for (int idx = 0; idx < rsp.battery_equipment_cfg_size(); ++idx)
			{
				com_protocol::BatteryEquipmentCfg* btyEquipCfg = rsp.mutable_battery_equipment_cfg(idx);
				for (int i = 0; i < btyEquip.ids_size(); ++i)
				{
					if (btyEquipCfg->info().id() == btyEquip.ids(i))
					{
						btyEquipCfg->set_status(1);
						msgLen = 0;
						break;
					}
				}
			}
		}
		break;
	}

	if (msgLen == 0)
	{
	    data = m_msgHandler->serializeMsgToBuffer(rsp, "get game common config reply");
		msgLen = rsp.ByteSize();
	}
	
	if (data != NULL) m_msgHandler->sendMessage(data, msgLen, m_msgHandler->getContext().userFlag, GET_GAME_COMMON_CONFIG_RSP);
}



// 该功能模块被暂时废掉了
// 获取超值礼包信息
void CMall::getSuperValuePackageInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetSuperValuePackageInfoRsp spValuePkgRsp;
	spValuePkgRsp.set_gift_status(0);
	com_protocol::SuperValuePackage* spValuePkg = spValuePkgRsp.mutable_info();
	*spValuePkg = m_gameMallCfg.super_value_pkg();
	
	const com_protocol::HttpSrvLogicData& logicData = m_msgHandler->getLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen).logicData;
	while (logicData.has_recharge() && logicData.recharge().has_super_value_pkg_last_time())
	{
		// 当前时间
		time_t curSecs = time(NULL);
		if (logicData.recharge().super_value_pkg_last_time() < curSecs) break;
		
		const unsigned int daySecs = 60 * 60 * 24;  // 一天的秒数
		unsigned int remainDays = (logicData.recharge().super_value_pkg_last_time() - curSecs) / daySecs + 1;
		spValuePkg->set_remain_day(remainDays);
		
		struct tm tmval;
		if (localtime_r(&curSecs, &tmval) == NULL)
		{
			OptErrorLog("get super value package, get local time error, user = %s", m_msgHandler->getContext().userData);
			break;
		}
		
		if (!logicData.recharge().has_receive_super_value_pkg_day() || (int)logicData.recharge().receive_super_value_pkg_day() != tmval.tm_yday)
		{
			spValuePkgRsp.set_gift_status(1);
		}
		
		break;
	}
	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(spValuePkgRsp, "get super value package info");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, spValuePkgRsp.ByteSize(), srcSrvId, GET_SUPER_VALUE_PACKAGE_RSP);
}

// 领取超值礼包物品
void CMall::receiveSuperValuePackageGift(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ReceiveSuperValuePackageReq receiveSuperValuePkgReq;
	if (!m_msgHandler->parseMsgFromBuffer(receiveSuperValuePkgReq, data, len, "receive super value package request")) return;
	
	if (receiveSuperValuePkgReq.id() != m_gameMallCfg.super_value_pkg().id())
	{
		OptErrorLog("to receive super package, the id is error, user = %s, invalid id = %u, package id = %u",
		m_msgHandler->getContext().userData, receiveSuperValuePkgReq.id(), m_gameMallCfg.super_value_pkg().id());
		return;
	}
	
	// 当前时间
	time_t curSecs = time(NULL);
	const com_protocol::HttpSrvLogicData& logicData = m_msgHandler->getLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen).logicData;
	if (!logicData.has_recharge() || !logicData.recharge().has_super_value_pkg_last_time() || logicData.recharge().super_value_pkg_last_time() < curSecs)
	{
		OptErrorLog("to receive super package, can not find the package data, user = %s", m_msgHandler->getContext().userData);
		return;
		
		struct tm tmval;
		if (localtime_r(&curSecs, &tmval) == NULL)
		{
			OptErrorLog("to receive super package, get local time error, user = %s", m_msgHandler->getContext().userData);
			return;
		}
		
		if (logicData.recharge().has_receive_super_value_pkg_day() && (int)logicData.recharge().receive_super_value_pkg_day() == tmval.tm_yday)
		{
			OptErrorLog("to receive super package, today already receive the gift package, user = %s", m_msgHandler->getContext().userData);
			return;
		}
	}
	
	m_msgHandler->sendMessageToCommonDbProxy(data, len, BUSINESS_RECEIVE_SUPER_VALUE_PACKAGE_REQ, m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, srcSrvId);
}

void CMall::receiveSuperValuePackageGiftReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ReceiveSuperValuePackageRsp rsp;
	if (!m_msgHandler->parseMsgFromBuffer(rsp, data, len, "receive super value package reply")) return;
	
	while (rsp.result() == OptSuccess)
	{
		com_protocol::HttpSrvLogicData& logicData = m_msgHandler->setLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen).logicData;
		if (!logicData.has_recharge())
		{
			OptErrorLog("receive super value package success, but can not find the recharge record, user = %s", m_msgHandler->getContext().userData);
			break;
		}
		
		time_t curSecs = time(NULL);
		struct tm tmval;
		if (localtime_r(&curSecs, &tmval) == NULL)
		{
			OptErrorLog("receive super package reply, get local time error, user = %s", m_msgHandler->getContext().userData);
			break;
		}
		
		logicData.mutable_recharge()->set_receive_super_value_pkg_day(tmval.tm_yday);
		break;
	}
	
	m_msgHandler->sendMessage(data, len, m_msgHandler->getContext().userFlag, RECEIVE_SUPER_VALUE_PACKAGE_GIFT_RSP);
}
*/

}

