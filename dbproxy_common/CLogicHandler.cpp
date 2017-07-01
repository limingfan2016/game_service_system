
/* author : limingfan
* date : 2016.02.24
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

#include "MessageDefine.h"
#include "CSrvMsgHandler.h"
#include "CLogic.h"
#include "SrvFrame/CModule.h"
#include "CLogicHandler.h"
#include "base/Function.h"
#include "db/CMySql.h"
#include "base/CMD5.h"
#include "CGameRecord.h"
#include "../common/CommonType.h"


using namespace NProject;

// 兑换日志文件
#define ExchangeLog(format, args...)     m_msgHandler->getLogic().getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)


CLogicHandler::CLogicHandler()
{
	m_msgHandler = NULL;
	m_clearExchangeLogFlag = 0;
}

CLogicHandler::~CLogicHandler()
{
	m_msgHandler = NULL;
}

int CLogicHandler::init(CSrvMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

    // 游戏内虚拟物品兑换
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_GAME_MALL_CONFIG_REQ, (ProtocolHandler)&CLogicHandler::getGameMallCfg, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_EXCHANGE_GAME_GOODS_REQ, (ProtocolHandler)&CLogicHandler::exchangeGameGoods, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_MALL_FISH_COIN_INFO_REQ, (ProtocolHandler)&CLogicHandler::getMallFishCoinInfo, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_FIRST_RECHARGE_PACKAGE_REQ, (ProtocolHandler)&CLogicHandler::getFirstRechargeInfo, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_GAME_COMMON_CONFIG_REQ, (ProtocolHandler)&CLogicHandler::getGameCommonConfig, this);
	
	// 实物兑换
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_GOODS_EXCHANGE_CONFIG_REQ, (ProtocolHandler)&CLogicHandler::getExchangeMallCfg, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_EXCHANGE_PHONE_FARE_REQ, (ProtocolHandler)&CLogicHandler::exchangePhoneFare, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_EXCHANGE_GOODS_REQ, (ProtocolHandler)&CLogicHandler::exchangeGiftGoods, this);
	
	//积分商城
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_MALL_SCORES_INFO_REQ, (ProtocolHandler)&CLogicHandler::getScoresShopInfo, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_EXCHANGE_SCORES_ITEM_REQ, (ProtocolHandler)&CLogicHandler::exchangeScoresItem, this);

	//记录聊天
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_USER_CHAT_LOG_REQ, (ProtocolHandler)&CLogicHandler::chatLog, this);
	// m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_RECEIVE_SUPER_VALUE_PACKAGE_REQ, (ProtocolHandler)&CLogicHandler::receiveSuperValuePackageGift, this);

	//PK场结算
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_PK_PLAY_SETTLEMENT_OF_ACCOUNTS_REQ, (ProtocolHandler)&CLogicHandler::handlePKPlaySettlementOfAccountsReq, this);


	//初始化积分商城
	const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();
	m_clearExchangeLogFlag = mallData.scores_shop.scores_item_clear_flag;
	reExchangeLog(mallData.scores_shop);
		
	//1分钟做一次提交Mysql操作(记录聊天信息)
	m_msgHandler->setTimer(60 * 1000, (TimerHandler)&CLogicHandler::chatLogToMysql, 0, NULL, -1, this);

	return 0;
}

void CLogicHandler::updateConfig()
{
	const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();

	//更新积分商品的库存
	if(mallData.scores_shop.scores_item_clear_flag != m_clearExchangeLogFlag)
	{
		reExchangeLog(mallData.scores_shop);
		m_clearExchangeLogFlag = mallData.scores_shop.scores_item_clear_flag;
	}

	//更新积分商品
	if (mallData.scores_shop.scores_item_list.size() != m_exchangeLog.size())
	{
		unordered_map<uint32, uint32> newExchangeLog;
		
		for (auto scoresItemCfg = mallData.scores_shop.scores_item_list.begin(); 
			 scoresItemCfg != mallData.scores_shop.scores_item_list.end(); 
			 ++scoresItemCfg)
		{
			auto item = m_exchangeLog.find(scoresItemCfg->id);
			if (item == m_exchangeLog.end())
				newExchangeLog.insert(exchange_value(scoresItemCfg->id, scoresItemCfg->exchange_count));
			else
				newExchangeLog.insert(exchange_value(item->first, item->second));
		}

		m_exchangeLog.clear();
		m_exchangeLog = newExchangeLog;
	}

	if (mallData.isSetConfigValueSuccess())
	{
		// 商城配置更新成功了再清空
		m_gameMallCfgRsp.Clear();
		
        // 道具列表
		for (unsigned int i = 0; i < mallData.fish_coin_list.size(); ++i)
		{
			const MallConfigData::game_goods& game_gold = mallData.fish_coin_list[i];
			com_protocol::GameGoodsInfo* ggInfo = m_gameMallCfgRsp.add_goods_list();
			ggInfo->set_id(game_gold.id);
			ggInfo->set_num(game_gold.num);
			ggInfo->set_buy_price(game_gold.buy_price);
			ggInfo->set_coin_type(EPropType::PropFishCoin);
			ggInfo->set_flag(game_gold.flag);
		}
		
		// 钻石购买列表
		for (unsigned int i = 0; i < mallData.diamonds_list.size(); ++i)
		{
			const MallConfigData::game_goods& game_gold = mallData.diamonds_list[i];
			com_protocol::GameGoodsInfo* ggInfo = m_gameMallCfgRsp.add_goods_list();
			ggInfo->set_id(game_gold.id);
			ggInfo->set_num(game_gold.num);
			ggInfo->set_buy_price(game_gold.buy_price);
			ggInfo->set_coin_type(EPropType::PropDiamonds);
			ggInfo->set_flag(game_gold.flag);
		}
		
		// 首冲礼包
		com_protocol::FishCoinInfo* firstFishCoinInfo = m_gameMallCfgRsp.mutable_first_recharge_pkg();
		firstFishCoinInfo->set_id(mallData.first_recharge_package.id);
		firstFishCoinInfo->set_num(mallData.first_recharge_package.num);
		firstFishCoinInfo->set_buy_price(mallData.first_recharge_package.buy_price);
		firstFishCoinInfo->set_flag(mallData.first_recharge_package.flag);
		firstFishCoinInfo->set_gift(mallData.first_recharge_package.gift);
		firstFishCoinInfo->set_first_amount(mallData.first_recharge_package.first_recharge_gold);
		// firstFishCoinInfo->set_mg_charging_id(mallData.first_recharge_package.mg_charging_id);
		// firstFishCoinInfo->set_wogame_charging_id(mallData.first_recharge_package.wogame_charging_id);
		
		
		/*
		// 炮台装备购买列表
		for (unsigned int i = 0; i < mallData.battery_equipment_list.size(); ++i)
		{
			const MallConfigData::game_goods& game_gold = mallData.battery_equipment_list[i];
			com_protocol::GameGoodsInfo* ggInfo = m_gameMallCfgRsp.add_battery_equipments();
			ggInfo->set_id(game_gold.id);
			ggInfo->set_num(game_gold.num);
			ggInfo->set_buy_price(game_gold.buy_price);
			ggInfo->set_coin_type(EPropType::PropFishCoin);
			ggInfo->set_flag(game_gold.flag);
		}
		*/ 

        /*
		// 超值礼包
		com_protocol::SuperValuePackage* superValuePkg = m_gameMallCfgRsp.mutable_super_value_pkg();
		superValuePkg->set_id(mallData.super_value_package.id);
		superValuePkg->set_buy_price(mallData.super_value_package.buy_price);
		superValuePkg->set_money_value(mallData.super_value_package.money_value);
		superValuePkg->set_day(mallData.super_value_package.additional_day);
		superValuePkg->set_remain_day(0);
		for (map<unsigned int, unsigned int>::const_iterator it = mallData.super_value_package.package_contain.begin(); it != mallData.super_value_package.package_contain.end(); ++it)
		{
			com_protocol::GiftInfo* giftInfo = superValuePkg->add_content();
			giftInfo->set_type(it->first);
			giftInfo->set_num(it->second);
		}
		
		for (map<unsigned int, unsigned int>::const_iterator it = mallData.super_value_package.package_day_additional.begin(); it != mallData.super_value_package.package_day_additional.end(); ++it)
		{
			com_protocol::GiftInfo* giftInfo = superValuePkg->add_day_additional();
			giftInfo->set_type(it->first);
			giftInfo->set_num(it->second);
		}
	    */ 
		
		// 金币礼包，钻石礼包，精彩礼包，互动礼包， PK专用礼包
		const MallConfigData::GiftPackage* giftPackageCfg[] = { &mallData.gold_package, &mallData.diamonds_package, &mallData.wonderful_package, &mallData.interaction_package, &mallData.pkPlay_package, };
		for (unsigned int idx = 0; idx < (sizeof(giftPackageCfg) / sizeof(giftPackageCfg[0])); ++idx)
		{
			com_protocol::GiftPackage* giftPackage = m_gameMallCfgRsp.add_gift_pkgs();
			giftPackage->set_id((*giftPackageCfg)[idx].id);
			giftPackage->set_buy_price((*giftPackageCfg)[idx].buy_price);
			giftPackage->set_coin_type((*giftPackageCfg)[idx].buy_type);

			for (map<unsigned int, unsigned int>::const_iterator it = (*giftPackageCfg)[idx].package_contain.begin(); it != (*giftPackageCfg)[idx].package_contain.end(); ++it)
			{
				com_protocol::GiftInfo* giftInfo = giftPackage->add_content();
				giftInfo->set_type(it->first);
				giftInfo->set_num(it->second);
			}
		}
		
		
		// 实物兑换配置
		m_exchangeInfoRsp.Clear();
		
		// 话费额度兑换列表
		for (unsigned int i = 0; i < mallData.phone_fare_list.size(); ++i)
		{
			const MallConfigData::PhoneFare& phoneFareCfg = mallData.phone_fare_list[i];
			if (!phoneFareCfg.is_used) continue;

			com_protocol::PhoneFareInfo* pfInfo = m_exchangeInfoRsp.add_phone_fare_list();
			pfInfo->set_id(phoneFareCfg.id);
			pfInfo->set_expense(phoneFareCfg.expense);
			pfInfo->set_achieve(phoneFareCfg.achieve);
		}
		
		// 实物兑换列表
		for (unsigned int i = 0; i < mallData.goods_list.size(); ++i)
		{
			const MallConfigData::goods& goodsCfg = mallData.goods_list[i];
			if (!goodsCfg.is_used) continue;

			com_protocol::ExchangeGoodsInfo* gdInfo = m_exchangeInfoRsp.add_goods_list();
			gdInfo->set_id(goodsCfg.id);
			gdInfo->set_exchange_count(goodsCfg.exchange_count);
			gdInfo->set_exchange_desc(goodsCfg.exchange_desc);
		}
	}
}

const com_protocol::FishCoinInfo* CLogicHandler::getFishCoinInfo(const unsigned int platformId, const unsigned int id)
{
	com_protocol::FishCoinInfo* fcInfoRet = NULL;
	for (int idx = 0; idx < m_gameMallCfgRsp.fish_coin_list_size(); ++idx)
	{
		if (m_gameMallCfgRsp.fish_coin_list(idx).id() == id)
		{
			fcInfoRet = m_gameMallCfgRsp.mutable_fish_coin_list(idx);
			break;
		}
	}
	
	if (m_gameMallCfgRsp.first_recharge_pkg().id() == id) fcInfoRet = m_gameMallCfgRsp.mutable_first_recharge_pkg();
	
	if (fcInfoRet != NULL)
	{
		// 各个平台的计费点配置
		fcInfoRet->clear_order_charging_id();
		map<unsigned int, MallConfigData::PlatformOrderCharingCfg>::const_iterator orderCharingIt = MallConfigData::MallData::getConfigValue().order_charing_cfg.find(platformId);
		if (orderCharingIt != MallConfigData::MallData::getConfigValue().order_charing_cfg.end())
		{
			map<unsigned int, string>::const_iterator charingIdIt = orderCharingIt->second.goodsId2orderCharingId.find(fcInfoRet->id());
			if (charingIdIt != orderCharingIt->second.goodsId2orderCharingId.end()) fcInfoRet->set_order_charging_id(charingIdIt->second);
		}
	}
	
	return fcInfoRet;
}

// 获取服务的逻辑数据
int CLogicHandler::getLogicData(const char* firstKey, const unsigned int firstKeyLen, const char* secondKey, const unsigned int secondKeyLen, ::google::protobuf::Message& msg)
{
	int result = -1;
	char msgBuffer[MaxMsgLen] = {0};
	int gameDataLen = m_msgHandler->getLogicRedisService().getHField(firstKey, firstKeyLen, secondKey, secondKeyLen, msgBuffer, MaxMsgLen);
	if (gameDataLen > 0)
	{
		result = msg.ParseFromArray(msgBuffer, gameDataLen) ? 0 : ServiceParseFromArrayError;
	}
	else
	{
		result = gameDataLen;
	}
	
	if (result != 0) OptErrorLog("get logic data error, key first = %s, second = %s, result = %d", firstKey, secondKey, result);
	return result;
}

// 获取游戏或者大厅的逻辑数据
void CLogicHandler::getLogicData(const com_protocol::GetUserBaseinfoReq& req, com_protocol::GetUserBaseinfoRsp& rsp)
{
	if (!req.has_data_type()) return;
	
	rsp.set_data_type(req.data_type());
	switch (req.data_type())
	{
		case ELogicDataType::HALL_LOGIC_DATA:
		{
			com_protocol::HallLogicData* hallData = rsp.mutable_hall_data();
			if (getLogicData(HallLogicDataKey, HallLogicDataKeyLen, req.query_username().c_str(), req.query_username().length(), *hallData) != 0) rsp.clear_hall_data();
			break;
		}
		
		case ELogicDataType::BUYU_LOGIC_DATA:
		{
			com_protocol::BuyuLogicData* buyuData = rsp.mutable_buyu_data();
			if (getLogicData(BuyuLogicDataKey, BuyuLogicDataKeyLen, req.query_username().c_str(), req.query_username().length(), *buyuData) != 0) rsp.clear_buyu_data();
			break;
		}
		
		case ELogicDataType::DBCOMMON_LOGIC_DATA:
		{
			m_msgHandler->removeOverdueGoldTicket(req.query_username().c_str(), req.query_username().length());
			com_protocol::DBCommonSrvLogicData* commonDbData = rsp.mutable_commondb_data();
			*commonDbData = m_msgHandler->getLogicDataInstance().getLogicData(req.query_username().c_str(), req.query_username().length()).logicData;
			break;
		}
		
		case ELogicDataType::HALL_BUYU_DATA:
		{
			com_protocol::HallLogicData* hallData = rsp.mutable_hall_data();
			if (getLogicData(HallLogicDataKey, HallLogicDataKeyLen, req.query_username().c_str(), req.query_username().length(), *hallData) != 0) rsp.clear_hall_data();

			com_protocol::BuyuLogicData* buyuData = rsp.mutable_buyu_data();
			if (getLogicData(BuyuLogicDataKey, BuyuLogicDataKeyLen, req.query_username().c_str(), req.query_username().length(), *buyuData) != 0) rsp.clear_buyu_data();
			break;
		}
		
		case ELogicDataType::BUYU_COMMON_DATA:
		{
			com_protocol::BuyuLogicData* buyuData = rsp.mutable_buyu_data();
			if (getLogicData(BuyuLogicDataKey, BuyuLogicDataKeyLen, req.query_username().c_str(), req.query_username().length(), *buyuData) != 0) rsp.clear_buyu_data();
			
			m_msgHandler->removeOverdueGoldTicket(req.query_username().c_str(), req.query_username().length());
			com_protocol::DBCommonSrvLogicData* commonDbData = rsp.mutable_commondb_data();
			*commonDbData = m_msgHandler->getLogicDataInstance().getLogicData(req.query_username().c_str(), req.query_username().length()).logicData;
			break;
		}
		
		case ELogicDataType::FF_CHESS_DATA:
		{
			com_protocol::XiangQiServerData* ffChessData = rsp.mutable_ff_chess_data();
			if (getLogicData(FFLogicLogicDataKey, FFLogicLogicDataKeyLen, req.query_username().c_str(), req.query_username().length(), *ffChessData) != 0) rsp.clear_ff_chess_data();
			break;
		}
		
		case ELogicDataType::FFCHESS_COMMON_DATA:
		{
			com_protocol::XiangQiServerData* ffChessData = rsp.mutable_ff_chess_data();
			if (getLogicData(FFLogicLogicDataKey, FFLogicLogicDataKeyLen, req.query_username().c_str(), req.query_username().length(), *ffChessData) != 0) rsp.clear_ff_chess_data();
			
			com_protocol::DBCommonSrvLogicData* commonDbData = rsp.mutable_commondb_data();
			*commonDbData = m_msgHandler->getLogicDataInstance().updateFFChessGoods(req.query_username().c_str(), req.query_username().length()).logicData;			
			break;
		}
		
		default:
		{
			OptErrorLog("get logic data error, user = %s, type = %d", req.query_username().c_str(), req.data_type());
			break;
		}
	}
}

void CLogicHandler::getGameMallCfg(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetGameMallCfgReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "get game mall config request")) return;
	
	// 获取DB中的用户信息
	CUserBaseinfo user_baseinfo;
	if (!m_msgHandler->getUserBaseinfo(string(m_msgHandler->getContext().userData), user_baseinfo))
	{
		m_gameMallCfgRsp.set_fish_coin(0);
		m_gameMallCfgRsp.set_game_gold(0);
		m_gameMallCfgRsp.set_diamonds(0);
	}
	else
	{
		m_gameMallCfgRsp.set_fish_coin(user_baseinfo.dynamic_info.rmb_gold);
		m_gameMallCfgRsp.set_game_gold(user_baseinfo.dynamic_info.game_gold);
		m_gameMallCfgRsp.set_diamonds(user_baseinfo.dynamic_info.diamonds_number);
	}
	
	const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();
	const com_protocol::DBCommonSrvLogicData& logicData = m_msgHandler->getLogicDataInstance().getLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen).logicData;
	const bool hasRechargeData = (logicData.has_recharge() && logicData.recharge().recharged_id_size() > 0);  // 首充值标识处理
	
	// 特殊平台的充值列表配置
	const vector<unsigned int>* specialPlatformMoneyCfgVector = NULL;
	map<unsigned int, MallConfigData::PlatformRechargeCfg>::const_iterator specialPlatformCfgInfoIt = mallData.special_platform_money_cfg.find(req.platform_id());
	if (specialPlatformCfgInfoIt != mallData.special_platform_money_cfg.end()) specialPlatformMoneyCfgVector = &(specialPlatformCfgInfoIt->second.need_money_vector);

	// 渔币列表
	m_gameMallCfgRsp.clear_fish_coin_list();
	for (unsigned int i = 0; i < mallData.money_list.size(); ++i)
	{
		const MallConfigData::FishCoin& fish_coin = mallData.money_list[i];
		
		// 废弃的配置项则忽略
		// 如果是特殊平台的充值配置则只允许配置列表里的配置项
		if (!fish_coin.is_used || (specialPlatformMoneyCfgVector != NULL
		    && std::find(specialPlatformMoneyCfgVector->begin(), specialPlatformMoneyCfgVector->end(), fish_coin.id) == specialPlatformMoneyCfgVector->end())) continue;

		com_protocol::FishCoinInfo* fcInfo = m_gameMallCfgRsp.add_fish_coin_list();
		fcInfo->set_id(fish_coin.id);
		fcInfo->set_num(fish_coin.num);
		fcInfo->set_buy_price(fish_coin.buy_price);
		fcInfo->set_gift(fish_coin.gift);
		fcInfo->set_first_amount(fish_coin.first_recharge_gold);
		// fcInfo->set_mg_charging_id(fish_coin.mg_charging_id);
		// fcInfo->set_wogame_charging_id(fish_coin.wogame_charging_id);
		
		fcInfo->set_flag(fish_coin.flag);
		if (fcInfo->flag() >= MallItemFlag::FirstNoFlag)
		{
			fcInfo->set_flag(MallItemFlag::First);
			if (hasRechargeData)
			{
				for (int idx = 0; idx < logicData.recharge().recharged_id_size(); ++idx)
				{
					if (logicData.recharge().recharged_id(idx) == fcInfo->id())
					{
						fcInfo->set_flag(fish_coin.flag - MallItemFlag::FirstNoFlag);
						break;
					}
				}
			}
		}
	}

    google::protobuf::Message* rspMsg = &m_gameMallCfgRsp;
	com_protocol::GetFFChessMallCfgRsp ffChessMallCfgRsp;
	if (req.has_mall_type() && req.mall_type() == com_protocol::EGameMallType::E_FF_Chess_Mall)
	{
		// 翻翻棋游戏商城，金币列表处理
		rspMsg = &ffChessMallCfgRsp;
	    google::protobuf::RepeatedPtrField<com_protocol::FishCoinInfo>* fishCoinInfo = ffChessMallCfgRsp.mutable_fish_coin_list();
	    *fishCoinInfo = m_gameMallCfgRsp.fish_coin_list();  // 渔币列表
		
		// 金币列表
		for (unsigned int i = 0; i < mallData.ff_chess_game_gold_list.size(); ++i)
		{
			const MallConfigData::game_goods& game_gold = mallData.ff_chess_game_gold_list[i];
			com_protocol::GameGoodsInfo* ggInfo = ffChessMallCfgRsp.add_gold_list();
			ggInfo->set_id(game_gold.id);
			ggInfo->set_num(game_gold.num);
			ggInfo->set_buy_price(game_gold.buy_price);
			ggInfo->set_coin_type(EPropType::PropFishCoin);
			ggInfo->set_flag(game_gold.flag);
		}
	}
	else
	{
		// 捕鱼游戏商城，炮台装备购买处理
		m_gameMallCfgRsp.clear_battery_equipments();
		const bool hasBatteryEquip = (logicData.has_battery_equipment() && logicData.battery_equipment().ids_size() > 0);
		for (unsigned int i = 0; i < mallData.battery_equipment_list.size(); ++i)
		{
			bool isBuy = false;
			const MallConfigData::game_goods& game_gold = mallData.battery_equipment_list[i];
			if (hasBatteryEquip)
			{
				const com_protocol::BatteryEquipment& btyEquip = logicData.battery_equipment();
				for (int idx = 0; idx < btyEquip.ids_size(); ++idx)
				{
					if (game_gold.id == btyEquip.ids(idx))
					{
						isBuy = true;
						break;
					}
				}
			}
			
			if (!isBuy)
			{
				com_protocol::GameGoodsInfo* ggInfo = m_gameMallCfgRsp.add_battery_equipments();
				ggInfo->set_id(game_gold.id);
				ggInfo->set_num(game_gold.num);
				ggInfo->set_buy_price(game_gold.buy_price);
				ggInfo->set_coin_type(EPropType::PropFishCoin);
				ggInfo->set_flag(game_gold.flag);
			}
		}
	}

	char mallConfigData[NFrame::MaxMsgLen] = { 0 };
	if (rspMsg->SerializeToArray(mallConfigData, NFrame::MaxMsgLen))
	{
		if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_GET_GAME_MALL_CONFIG_RSP;
	    m_msgHandler->sendMessage(mallConfigData, rspMsg->ByteSize(), srcSrvId, srcProtocolId);
	}
	else
	{
		OptErrorLog("game goods mall config SerializeToArray error, platform id = %u, version = %s, mall type = %d, size = %d, buffer len = %d, srcProtocolId = %d",
		req.platform_id(), req.version().c_str(), req.mall_type(), rspMsg->ByteSize(), NFrame::MaxMsgLen, srcProtocolId);
	}
}

// 渔币信息
void CLogicHandler::getMallFishCoinInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetMallFishCoinInfoReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "get mall fish coin information")) return;
	
	unsigned int status = 0;
	com_protocol::GetMallFishCoinInfoRsp rsp;
	if (!req.has_type())  // 新版本渔币
	{
		const com_protocol::FishCoinInfo* fscInfo = getFishCoinInfo(req.platform_id(), req.id());
		if (fscInfo != NULL)
		{
			com_protocol::FishCoinInfo* rspFCInfo = rsp.mutable_fish_info();
			*rspFCInfo = *fscInfo;
			if (m_msgHandler->getLogicDataInstance().fishCoinIsRecharged(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, req.id()))
			{
				status = 1;  // 表示已经购买过了
			}
		}
	}	
	
	if (rsp.has_fish_info())
	{
		rsp.set_result(ServiceSuccess);
		rsp.set_status(status);
		
		if (req.has_type()) rsp.set_type(req.type());
		if (req.has_cb_flag()) rsp.set_cb_flag(req.cb_flag());
		if (req.has_cb_data()) rsp.set_cb_data(req.cb_data());
	}
	else
	{
		rsp.set_result(ServiceItemNotExist);
	}
	
	char send_data[MaxMsgLen] = { 0 };
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, send_data, send_data_len, "send get fish coin information reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_GET_MALL_FISH_COIN_INFO_RSP);
}

void CLogicHandler::exchangeGameGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ExchangeGameGoogsReq exchangeGameGoogsReq;
	if (!m_msgHandler->parseMsgFromBuffer(exchangeGameGoogsReq, data, len, "exchange game goods request")) return;
	
	com_protocol::ExchangeGameGoogsRsp rsp;
	rsp.set_result(ServiceSuccess);
	GameGoodsData gameGoodsData;
	do 
	{
		// 找到兑换信息
		if (!getGameGoodsInfo(exchangeGameGoogsReq, gameGoodsData))
		{
			OptErrorLog("find the goods error, user = %s, type = %u, id = %u, count = %u",
			m_msgHandler->getContext().userData, exchangeGameGoogsReq.type(), exchangeGameGoogsReq.id(), exchangeGameGoogsReq.count());
			rsp.set_result(ServiceItemNotExist);
			break;
		}

		// 获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_msgHandler->getUserBaseinfo(string(m_msgHandler->getContext().userData), user_baseinfo))
		{
			OptErrorLog("exchange game goods, can not find the user = %s, type = %u, id = %u",
			m_msgHandler->getContext().userData, exchangeGameGoogsReq.type(), exchangeGameGoogsReq.id());
			rsp.set_result(ServiceGetUserinfoFailed); // 获取DB中的信息失败
			break;
		}

		// 判断渔币是否足够
		if (gameGoodsData.coinType == EPropType::PropFishCoin && gameGoodsData.coinCount > user_baseinfo.dynamic_info.rmb_gold)
		{
			OptErrorLog("the fish coin not enought, user = %s, amount = %u, type = %u, id = %u, count = %u, need = %u",
			m_msgHandler->getContext().userData, user_baseinfo.dynamic_info.rmb_gold, exchangeGameGoogsReq.type(), exchangeGameGoogsReq.id(), exchangeGameGoogsReq.count(), gameGoodsData.coinCount);
			rsp.set_result(ServiceFishCoinNotEnought);
			break;
		}
		
		// 判断钻石是否足够
		if (gameGoodsData.coinType == EPropType::PropDiamonds && gameGoodsData.coinCount > user_baseinfo.dynamic_info.diamonds_number)
		{
			OptErrorLog("the diamonds not enought, user = %s, amount = %u, type = %u, id = %u, count = %u, need = %u",
			m_msgHandler->getContext().userData, user_baseinfo.dynamic_info.diamonds_number, exchangeGameGoogsReq.type(), exchangeGameGoogsReq.id(), exchangeGameGoogsReq.count(), gameGoodsData.coinCount);
			rsp.set_result(ServiceDiamondsNotEnought);
			break;
		}
		
		// 开始兑换
		unsigned int getGoodsCount = 0;  // 获得目标物品的数量
		char exchangeInfo[1024] = {0};	// 写兑换记录到DB	
		if (exchangeGameGoogsReq.type() == ExchangeType::EBatteryEquip)  // 先看看是否是炮台装备
		{
			const unsigned int batteryEquipmentIdBase = 7000;  // 炮台道具ID值必须是从 7001 开始的
			const char* beType[] = {"雷霆之怒", "炫彩虹霓", "地狱火焰", "金色风暴"};
			GoodsIdToNum::iterator batteryEquipIt = gameGoodsData.id2num.begin();
			const unsigned int ebId = (batteryEquipIt->first % batteryEquipmentIdBase) - 1;
			if (ebId < (sizeof(beType) / sizeof(beType[0])))
			{
			    snprintf(exchangeInfo, sizeof(exchangeInfo) - 1, "%u个%s", batteryEquipIt->second, beType[ebId]);
				getGoodsCount = batteryEquipIt->second;
			}
			else
			{
				OptErrorLog("the battery equipment id invalid, user = %s, type = %u, id = %u", m_msgHandler->getContext().userData, exchangeGameGoogsReq.type(), batteryEquipIt->first);
				rsp.set_result(ServiceExchangeTypeInvalid);
			}
		}
		else
		{
			// 道具值
			uint32_t* type2value[] =
			{
				NULL,                                               //保留
				NULL,                                               //金币
				NULL,                                               //渔币
				NULL,                                               //话费卡
				&user_baseinfo.prop_info.suona_count,				//小喇叭
				&user_baseinfo.prop_info.light_cannon_count,		//激光炮
				&user_baseinfo.prop_info.flower_count,				//鲜花
				&user_baseinfo.prop_info.mute_bullet_count,			//哑弹
				&user_baseinfo.prop_info.slipper_count,			    //拖鞋
				NULL,	                                            //奖券
				&user_baseinfo.prop_info.auto_bullet_count,		    //自动炮子弹
				&user_baseinfo.prop_info.lock_bullet_count,		    //锁定炮子弹
				&user_baseinfo.dynamic_info.diamonds_number,        //钻石
				NULL,												//话费额度
				NULL,												//积分
				&user_baseinfo.prop_info.rampage_count,				//狂暴
				&user_baseinfo.prop_info.dud_shield_count,			//哑弹防护
			};

			const char* type2name[] =
			{
				"",                 //保留
				"金币",             //金币
				"渔币",             //渔币
				"话费卡",           //话费卡
				"小喇叭",		    //小喇叭
				"激光炮",		    //激光炮
				"鲜花",				//鲜花
				"哑弹",			    //哑弹
				"拖鞋",			    //拖鞋
				"奖券",	            //奖券
				"自动炮",		    //自动炮子弹
				"锁定炮",		    //锁定炮子弹
				"钻石",  		    //钻石
				"话费额度",
				"积分",
				"狂暴",				//狂暴
				"哑弹护盾",         //哑弹防护
			};
		
		    unsigned int exchangeInfoLen = 0;
			for (GoodsIdToNum::iterator it = gameGoodsData.id2num.begin(); it != gameGoodsData.id2num.end(); ++it)
			{
				com_protocol::GiftInfo* dstGift = rsp.add_dst_amount();
				uint32_t* value = type2value[it->first];
				if (value == NULL)
				{
					if (it->first != EPropType::PropGold)
					{
						OptErrorLog("the goods type invalid, user = %s, type = %u, id = %u", m_msgHandler->getContext().userData, exchangeGameGoogsReq.type(), it->first);
						rsp.set_result(ServiceExchangeTypeInvalid);
						break;
					}

					user_baseinfo.dynamic_info.game_gold += it->second; //金币
				}
				else
				{
					*value = *value + it->second;
				}
				
				dstGift->set_type(it->first);
				dstGift->set_num(it->second);
				
				exchangeInfoLen += snprintf(exchangeInfo + exchangeInfoLen, sizeof(exchangeInfo) - exchangeInfoLen - 1, "%u个%s", it->second, type2name[it->first]);
				getGoodsCount += it->second;
			}
		}
	
	    if (rsp.result() != ServiceSuccess) break;
		
		// 扣除兑换源货币
		uint32_t& srcCoin = (gameGoodsData.coinType == EPropType::PropFishCoin) ? user_baseinfo.dynamic_info.rmb_gold : user_baseinfo.dynamic_info.diamonds_number;
		srcCoin -= gameGoodsData.coinCount;
		rsp.set_src_amount(gameGoodsData.coinCount);
		
		// 写DB
		// 写动态信息
		if (!m_msgHandler->updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info))
		{
			OptErrorLog("exchange game goods, update dynamic data to db error, user = %s, type = %u, id = %u",
			m_msgHandler->getContext().userData, exchangeGameGoogsReq.type(), exchangeGameGoogsReq.id());
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}
		
		// 写道具信息到DB
		if (exchangeGameGoogsReq.type() != ExchangeType::EBatteryEquip && exchangeGameGoogsReq.type() != ExchangeType::EFFChessGold && exchangeGameGoogsReq.id() != EPropType::PropGold
		    && !m_msgHandler->updateUserPropInfoToMysql(user_baseinfo.prop_info))
		{
			OptErrorLog("exchange game goods, update prop data to db error, user = %s, type = %u, id = %u",
			m_msgHandler->getContext().userData, exchangeGameGoogsReq.type(), exchangeGameGoogsReq.id());
			m_msgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}
		
		// 0：话费卡兑换，1：奖券兑换， 2：渔币兑换物品， 3：积分兑换，4：翻翻棋商城兑换
		unsigned int exchangeType = 2;
		const char* exchangeFlag = "道具";
		switch (exchangeGameGoogsReq.type())
		{
			case ExchangeType::EGiftPackage:
			{
				exchangeFlag = "礼包";
				break;
			}
			
			case ExchangeType::EBatteryEquip:
			{
				exchangeFlag = "炮台装备";
				break;
			}
			
			case ExchangeType::EFFChessGold:
			{
				exchangeType = 4;
				exchangeFlag = "翻翻棋道具";
				break;
			}
			
			default:
			{
				break;
			}
		}

		char sql_tmp[2048] = {0};
		snprintf(sql_tmp, sizeof(sql_tmp) - 1, "insert into tb_exchange_record_info(username,insert_time,exchange_type,exchange_count,exchange_goods_info,deal_status,mobilephone_number,address,recipients_name,cs_username,pay_type,get_goods_count) \
										   values(\'%s\',now(),%u,%u,\'兑换%s%s\', 2, \'\',\'\',\'\',\'\',%u,%u);",
										   m_msgHandler->getContext().userData, exchangeType, gameGoodsData.coinCount, exchangeFlag, exchangeInfo, gameGoodsData.coinType, getGoodsCount);
		if (Success != m_msgHandler->m_pDBOpt->modifyTable(sql_tmp))
		{
			m_msgHandler->mysqlRollback();
			OptErrorLog("write game mall exchange record error, user = %s, exeSql = %s", m_msgHandler->getContext().userData, sql_tmp);
			rsp.set_result(ServiceInsertFailed);
			break;
		}

		// 写到memcached
		if (!m_msgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			OptErrorLog("exchange game goods, update user data to memory error, user = %s, type = %u, id = %u",
			m_msgHandler->getContext().userData, exchangeGameGoogsReq.type(), exchangeGameGoogsReq.id());
			
			m_msgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			break;
		}
		
		m_msgHandler->updateOptStatus(m_msgHandler->getContext().userData, CSrvMsgHandler::UpdateOpt::Modify); // 有修改，更新状态
		m_msgHandler->mysqlCommit(); // 提交
		
		if (exchangeGameGoogsReq.type() == ExchangeType::EBatteryEquip)  // 炮台装备
		{
			m_msgHandler->getLogicDataInstance().exchangeBatteryEquipment(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, exchangeGameGoogsReq.id());
		}
		
		rsp.set_type(exchangeGameGoogsReq.type());
		rsp.set_id(exchangeGameGoogsReq.id());
		rsp.set_coin_type(gameGoodsData.coinType);
		
		OptInfoLog("exchange game goods, user = %s, goods type = %u, id = %u, count = %u, need coin = %u, remain = %u",
		m_msgHandler->getContext().userData, rsp.type(), rsp.id(), exchangeGameGoogsReq.count(), gameGoodsData.coinCount, srcCoin);

	} while (0);
	
	char buffer[MaxMsgLen] = { 0 };
	unsigned int msgLen = MaxMsgLen;
	if (rsp.result() != ServiceSuccess) rsp.clear_dst_amount();
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, buffer, msgLen, "CLogicHandler::exchangeGameGoods reply");
	if (msgData != NULL)
	{
	    m_msgHandler->sendMessage(msgData, msgLen, srcSrvId, CommonSrvBusiness::BUSINESS_EXCHANGE_GAME_GOODS_RSP);
	}
	
	ExchangeLog("exchange buyu goods|%s|%d|%u|%u|%u|%u|%u", m_msgHandler->getContext().userData, rsp.result(),
	exchangeGameGoogsReq.type(), exchangeGameGoogsReq.id(), exchangeGameGoogsReq.count(), gameGoodsData.coinCount, gameGoodsData.coinType);
}

bool CLogicHandler::getGameGoodsInfo(const com_protocol::ExchangeGameGoogsReq& msg, GameGoodsData& gameGoodsData)
{
	if (msg.count() < 1 || msg.count() > 100000) return false;  // 数量错误
	
	switch (msg.type())
	{
		case ExchangeType::EStrategy:
		case ExchangeType::EInteract:
		{
			// 兑换道具
			for (int idx = 0; idx < m_gameMallCfgRsp.goods_list_size(); ++idx)
			{
				const com_protocol::GameGoodsInfo& ggInfo = m_gameMallCfgRsp.goods_list(idx);
				if (ggInfo.id() == msg.id())
				{
					gameGoodsData.id2num.clear();
					gameGoodsData.id2num[ggInfo.id()] = ggInfo.num() * msg.count();
					gameGoodsData.coinCount = ggInfo.buy_price() * msg.count();
					gameGoodsData.coinType = ggInfo.coin_type();

					return true;
				}
			}
			
			break;
		}
		
		case ExchangeType::EGiftPackage:
		{
			// 兑换礼包
			const google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>* giftInfo = NULL;
			for (int idx = 0; idx < m_gameMallCfgRsp.gift_pkgs_size(); ++idx)
			{
				const com_protocol::GiftPackage& giftPkg = m_gameMallCfgRsp.gift_pkgs(idx);
				if (giftPkg.id() == msg.id())
				{
					giftInfo = &(giftPkg.content());
					gameGoodsData.coinCount = giftPkg.buy_price() * msg.count();
					gameGoodsData.coinType = giftPkg.coin_type();
					break;
				}
			}
			
			if (giftInfo != NULL)
			{
				gameGoodsData.id2num.clear();
				for (int idx = 0; idx < giftInfo->size(); ++idx)
				{
					const com_protocol::GiftInfo& gift = giftInfo->Get(idx);
					gameGoodsData.id2num[gift.type()] = gift.num() * msg.count();
				}
				
				return !gameGoodsData.id2num.empty();
			}
			
			break;
		}
		
		case ExchangeType::EBatteryEquip:
		{
			// 兑换炮台装备
			const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();
			for (unsigned int i = 0; i < mallData.battery_equipment_list.size(); ++i)
			{
				const MallConfigData::game_goods& game_gold = mallData.battery_equipment_list[i];			
				if (game_gold.id == msg.id())
				{
					gameGoodsData.id2num.clear();
					gameGoodsData.id2num[game_gold.id] = game_gold.num * msg.count();
					gameGoodsData.coinCount = game_gold.buy_price * msg.count();
					gameGoodsData.coinType = EPropType::PropFishCoin;

					return true;
				}
			}
			
			break;
		}
		
		case ExchangeType::EFFChessGold:
		{
			// 翻翻棋金币兑换
			const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();
			for (unsigned int i = 0; i < mallData.ff_chess_game_gold_list.size(); ++i)
			{
				const MallConfigData::game_goods& game_gold = mallData.ff_chess_game_gold_list[i];			
				if (game_gold.id == msg.id())
				{
					gameGoodsData.id2num.clear();
					gameGoodsData.id2num[EPropType::PropGold] = game_gold.num * msg.count();
					gameGoodsData.coinCount = game_gold.buy_price * msg.count();
					gameGoodsData.coinType = EPropType::PropFishCoin;

					return true;
				}
			}
			
			break;
		}
	}
	
	return false;
}

// 实物兑换
void CLogicHandler::getExchangeMallCfg(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 获取DB中的用户信息
	m_exchangeInfoRsp.clear_mobilephone_number();
	CUserBaseinfo user_baseinfo;
	if (!m_msgHandler->getUserBaseinfo(string(m_msgHandler->getContext().userData), user_baseinfo))
	{
		m_exchangeInfoRsp.set_phone_fare(0);
		m_exchangeInfoRsp.set_voucher_number(0);
	}
	else
	{
		m_exchangeInfoRsp.set_phone_fare(user_baseinfo.dynamic_info.phone_fare);
		m_exchangeInfoRsp.set_voucher_number(user_baseinfo.dynamic_info.voucher_number);
		if (user_baseinfo.dynamic_info.mobile_phone_number[0] != '\0') m_exchangeInfoRsp.set_mobilephone_number(user_baseinfo.dynamic_info.mobile_phone_number);
	}

	char mallConfigData[NFrame::MaxMsgLen] = { 0 };
	if (m_exchangeInfoRsp.SerializeToArray(mallConfigData, NFrame::MaxMsgLen))
	{
	    m_msgHandler->sendMessage(mallConfigData, m_exchangeInfoRsp.ByteSize(), srcSrvId, CommonSrvBusiness::BUSINESS_GET_GOODS_EXCHANGE_CONFIG_RSP);
	}
	else
	{
		OptErrorLog("game exchange config SerializeToArray error, size = %d, buffer len = %d", m_exchangeInfoRsp.ByteSize(), NFrame::MaxMsgLen);
	}
}

void CLogicHandler::exchangePhoneFare(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ExchangePhoneFareReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "exchange phone fare request")) return;

	com_protocol::ExchangePhoneFareRsp rsp;
	rsp.set_result(0);
	do 
	{
		// 手机号码合法性校验，手机号码至少大于10位
		if (req.mobilephone_number().length() < 10 || req.mobilephone_number().length() > 16 || !strIsAlnum(req.mobilephone_number().c_str()))
		{
			rsp.set_result(ServiceMobilephoneInputUnlegal);  // 手机号非法
			break;
		}
			
		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_msgHandler->getUserBaseinfo(string(m_msgHandler->getContext().userData), user_baseinfo))
		{
			rsp.set_result(ServiceGetUserinfoFailed);//获取DB中的信息失败
			break;
		}
		
		if (user_baseinfo.dynamic_info.mobile_phone_number[0] != '\0')
		{
			if (req.mobilephone_number() != user_baseinfo.dynamic_info.mobile_phone_number)
			{
				rsp.set_result(ExchangeMobilePhoneNotMatch);  // 兑换的电话号码不匹配
			    break;
			}
		}
		else
		{
			// 查询该手机号码是否已经绑定过了，高效查询语句值检查是否存在
			CQueryResult* p_result = NULL;
			while (true)
			{
				char sql[128] = { 0 };
				const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "select 1 from tb_user_dynamic_baseinfo where mobile_phone_number=\'%s\' limit 1;", req.mobilephone_number().c_str());
				if (Success != m_msgHandler->m_pDBOpt->queryTableAllResult(sql, sqlLen, p_result))
				{
					OptErrorLog("exchange phone fare, exec sql error:%s", sql);
					rsp.set_result(SaveExchangeMobilePhoneError);
					break;
					
				}
				
	            if (p_result != NULL && p_result->getRowCount() > 0)
				{
					OptErrorLog("already exist exchange mobile phone = %s", req.mobilephone_number().c_str());
					rsp.set_result(ExistExchangeMobilePhoneError);
					break;
				}
				
				break;
			}
			m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
			
			if (rsp.result() != ServiceSuccess) break;
	
			// 第一次兑换由用户输入兑换话费的手机号码做绑定
			strncpy(user_baseinfo.dynamic_info.mobile_phone_number, req.mobilephone_number().c_str(), DBStringFieldLength - 1);
		}
		
		//找到兑换信息
		const com_protocol::PhoneFareInfo* pfInfo = NULL;
		for (int idx = 0; idx < m_exchangeInfoRsp.phone_fare_list_size(); ++idx)
		{
			if (m_exchangeInfoRsp.phone_fare_list(idx).id() == req.id())
			{
				pfInfo = &(m_exchangeInfoRsp.phone_fare_list(idx));
				break;
			}
		}
		
		if (pfInfo == NULL)
		{
			rsp.set_result(ServiceItemNotExist);
			break;
		}

		// 判断话费额度是否足够
		unsigned int expense = pfInfo->expense();
		if (expense > user_baseinfo.dynamic_info.phone_fare)
		{
			rsp.set_result(ServicePhoneFareNotEnought);
			break;
		}

		// 开始兑换
		user_baseinfo.dynamic_info.phone_fare -= expense;

		//写DB
		//写动态信息
		if (!m_msgHandler->updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info))
		{
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}

		//写兑换记录到DB
		char sql_tmp[2048] = { 0 };
		snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_exchange_record_info(username,insert_time,exchange_type,exchange_count,exchange_goods_info,deal_status,mobilephone_number,address,recipients_name,cs_username,pay_type,get_goods_count) \
										   values(\'%s\',now(),0,%u,\'兑换面值%u元的电话卡一张\', 0, \'%s\',\'\',\'\',\'\',%u,%u);", m_msgHandler->getContext().userData,
										   expense, pfInfo->achieve(), req.mobilephone_number().c_str(), EPropType::PropPhoneFareValue, pfInfo->achieve());
		if (Success != m_msgHandler->m_pDBOpt->modifyTable(sql_tmp))
		{
			m_msgHandler->mysqlRollback();
			OptErrorLog("exeSql failed|%s|%s", m_msgHandler->getContext().userData, sql_tmp);
			rsp.set_result(ServiceInsertFailed);
			break;
		}

		//写到memcached
		if (!m_msgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			m_msgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			break;
		}
		
		rsp.set_allocated_info((com_protocol::PhoneFareInfo*)pfInfo);

		//提交
		m_msgHandler->mysqlCommit();
	} while (0);
	
	char send_data[MaxMsgLen] = { 0 };
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, send_data, send_data_len, "send exchange phone fare reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_EXCHANGE_PHONE_FARE_RSP);
	
	ExchangeLog("exchange phone fare|%s|%d|%u|%s", m_msgHandler->getContext().userData, rsp.result(), req.id(), req.mobilephone_number().c_str());
	
	if (rsp.result() == 0) rsp.release_info();
}

void CLogicHandler::exchangeGiftGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/*
	com_protocol::ExchangeGoodsReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "exchange gift goods request")) return;

	com_protocol::ExchangeGoodsRsp rsp;
	rsp.set_exchange_id(req.exchange_id());
	rsp.set_result(0);
	rsp.set_use_count(0);
	do
	{
		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_msgHandler->getUserBaseinfo(string(m_msgHandler->getContext().userData), user_baseinfo))
		{
			rsp.set_result(ServiceGetUserinfoFailed);//获取DB中的信息失败
			break;
		}
		
		//找到兑换信息
		const com_protocol::ExchangeGoodsInfo* gdInfo = NULL;
		for (int idx = 0; idx < m_exchangeInfoRsp.goods_list_size(); ++idx)
		{
			if (m_exchangeInfoRsp.goods_list(idx).id() == req.exchange_id())
			{
				gdInfo = &(m_exchangeInfoRsp.goods_list(idx));
				break;
			}
		}

		if (gdInfo == NULL)
		{
			rsp.set_result(ServiceItemNotExist);
			break;
		}

		//判断兑换券是否足够
		unsigned int exchange_count = gdInfo->exchange_count();
		if (exchange_count > user_baseinfo.dynamic_info.voucher_number)
		{
			rsp.set_result(ServiceVoucherNotEnought);
			break;
		}

		//开始兑换
		user_baseinfo.dynamic_info.voucher_number -= exchange_count;
		rsp.set_use_count(exchange_count);

		//写DB
		//写动态信息
		if (!m_msgHandler->updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info))
		{
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}

		//写兑换记录到DB
		char sql_tmp[2048] = { 0 };
		snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_exchange_record_info(username,insert_time,exchange_type,exchange_count,exchange_goods_info,deal_status,mobilephone_number,address,recipients_name,cs_username,pay_type) \
			values(\'%s\',now(),1,%u,\'%s\', 0, \'%s\',\'%s\',\'%s\',\'\',%u);", m_msgHandler->getContext().userData,
			exchange_count, gdInfo->exchange_desc().c_str(), req.mobilephone_number().c_str(), req.address().c_str(), req.recipients_name().c_str(), EPropType::PropVoucher);
		if (Success != m_msgHandler->m_pDBOpt->modifyTable(sql_tmp))
		{
			m_msgHandler->mysqlRollback();
			OptErrorLog("exeSql failed|%s|%s", m_msgHandler->getContext().userData, sql_tmp);
			rsp.set_result(ServiceInsertFailed);
			break;
		}

		//写到memcached
		if (!m_msgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			m_msgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			break;
		}

		//提交
		m_msgHandler->mysqlCommit();
	} while (0);

	char send_data[MaxMsgLen] = { 0 };
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, send_data, send_data_len, "send exchange gift goods reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_EXCHANGE_GOODS_RSP);
	
	ExchangeLog("exchange gift|%s|%d|%u|%u", m_msgHandler->getContext().userData, rsp.result(), rsp.exchange_id(), rsp.use_count());
	*/ 
}

// 获取首冲礼包信息
void CLogicHandler::getFirstRechargeInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetFirstPackageInfoRsp rsp;
	rsp.set_result(NonExistentFirstPackage);
	
	const com_protocol::FishCoinInfo& firstRechargePkg = m_gameMallCfgRsp.first_recharge_pkg();
	if (!m_msgHandler->getLogicDataInstance().fishCoinIsRecharged(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, firstRechargePkg.id()))
	{
		rsp.set_result(OptSuccess);
		com_protocol::FirstRechargePackage* firstPkg = rsp.mutable_pkg_info();
		firstPkg->set_id(firstRechargePkg.id());
		firstPkg->set_num(firstRechargePkg.num());
		firstPkg->set_buy_price(firstRechargePkg.buy_price());
		firstPkg->set_times(firstRechargePkg.flag());
	}

    char send_data[MaxMsgLen] = { 0 };
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, send_data, send_data_len, "get first package info");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, BUSINESS_GET_FIRST_RECHARGE_PACKAGE_RSP);
}

// 获取公共配置
void CLogicHandler::getGameCommonConfig(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetGameCommonConfigReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "get game common config request")) return;

	com_protocol::GetGameCommonConfigRsp rsp;
	rsp.set_result(ServiceSuccess);
	
	switch (req.type())
	{
		case ECommonConfigType::EBatteryEquipCfg :
		{
			const com_protocol::DBCommonSrvLogicData& logicData = m_msgHandler->getLogicDataInstance().getLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen).logicData;
			const bool hasBatteryEquip = (logicData.has_battery_equipment() && logicData.battery_equipment().ids_size() > 0);
			const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();

			CUserBaseinfo user_baseinfo;
			if (!m_msgHandler->getUserBaseinfo(m_msgHandler->getContext().userData, user_baseinfo))
			{
				OptErrorLog("CLogicHandler getGameCommonConfig, get User Base info, user:%s", m_msgHandler->getContext().userData);
				return;
			}

			rsp.set_diamonds(user_baseinfo.dynamic_info.diamonds_number);
			rsp.set_rmb_gold(user_baseinfo.dynamic_info.rmb_gold);
			for (unsigned int i = 0; i < mallData.battery_equipment_list.size(); ++i)
			{
				unsigned int status = 0;
				const MallConfigData::game_goods& game_gold = mallData.battery_equipment_list[i];
				if (hasBatteryEquip)
				{
					const com_protocol::BatteryEquipment& btyEquip = logicData.battery_equipment();
					for (int idx = 0; idx < btyEquip.ids_size(); ++idx)
					{
						if (game_gold.id == btyEquip.ids(idx))
						{
							status = 1;  // 表示已经购买了
							break;
						}
					}
				}
				
				com_protocol::BatteryEquipmentCfg* btEquipCfg = rsp.add_battery_equipment_cfg();
				btEquipCfg->set_status(status);
				com_protocol::GameGoodsInfo* info = btEquipCfg->mutable_info();
				info->set_id(game_gold.id);
				info->set_num(game_gold.num);
				info->set_buy_price(game_gold.buy_price);
				info->set_coin_type(EPropType::PropFishCoin);
				info->set_flag(game_gold.flag);
			}
		
			break;
		}
		
		default :
		{
			OptErrorLog("get game common config, the type invalid, user = %s, type = %u platform id = %u", m_msgHandler->getContext().userData, req.type(), req.platform_id());
			rsp.set_result(GameCommonConfigTypeError);
			break;
		}
	}
	
	if (rsp.result() == ServiceSuccess)
	{
	    rsp.set_type(req.type());
	    if (req.has_cb_flag()) rsp.set_cb_flag(req.cb_flag());
		if (req.has_cb_data()) rsp.set_cb_data(req.cb_data());
	}
	
	char send_data[MaxMsgLen] = { 0 };
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, send_data, send_data_len, "send get game common config reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_GET_GAME_COMMON_CONFIG_RSP);
}

void CLogicHandler::getScoresShopInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	CUserBaseinfo user_baseinfo;
	if (!m_msgHandler->getUserBaseinfo(m_msgHandler->getContext().userData, user_baseinfo))
	{
		OptErrorLog("CLogicHandler getScoresShopInfo, get User Base info, user:%s", m_msgHandler->getContext().userData);
		return;
	}

	com_protocol::ClientGetScoresItemRsp rsp;
	rsp.set_scores(user_baseinfo.dynamic_info.score);
	rsp.set_clear_time(0);

	const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();
	for (auto itemCfg = mallData.scores_shop.scores_item_list.begin(); itemCfg != mallData.scores_shop.scores_item_list.end(); ++itemCfg)
	{
		auto item = rsp.add_item_list();
		item->set_item_id(itemCfg->id);								//商品ID
		item->set_item_stock_num(getExchangeItemNum(itemCfg->id));
		item->set_name(itemCfg->exchange_desc);						//商品名称
		item->set_item_url(itemCfg->url);							//商品图片路径
		item->set_scores(itemCfg->exchange_scores);					//兑换商品所需积分
		item->set_describe(itemCfg->describe);
	}

	char send_data[MaxMsgLen] = { 0 };
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, send_data, send_data_len, "send get scores shop info");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_GET_MALL_SCORES_INFO_RSP);
}

void CLogicHandler::exchangeScoresItem(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientExchangeScoresItemReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "exchange scores item")) return;

	com_protocol::ClientExchangeScoresItemRsp rsp;
	rsp.set_result(ServiceSuccess);

    unsigned int payCount = 0;
	unsigned int getCount = 0;
	
	do 
	{
		//兑换是否到达上限
		if (getExchangeItemNum(req.item_id()) <= 0)
		{
			rsp.set_scores(ServiceExchangeCeiling);
			break;
		}

		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_msgHandler->getUserBaseinfo(string(m_msgHandler->getContext().userData), user_baseinfo))
		{
			rsp.set_result(ServiceGetUserinfoFailed);//获取DB中的信息失败
			break;
		}

		auto itemCfg = getExchangeItemCfg(req.item_id());
		if (itemCfg == NULL)
		{
			rsp.set_result(ServiceNotFindExchangeItem);
			break;
		}

		//判断积分是否足够
		if (user_baseinfo.dynamic_info.score < itemCfg->exchange_scores)
		{
			rsp.set_result(ServiceScoresNotEnough);
			break;
		}
				
		//开始兑换

		//减少积分
		user_baseinfo.dynamic_info.score -= itemCfg->exchange_scores;
		if (!m_msgHandler->updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info))
		{
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}
				
		//写兑换记录到DB
		char sql_tmp[2048] = { 0 };
		snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_exchange_record_info(username,insert_time,exchange_type,exchange_count,exchange_goods_info,\
										    deal_status,mobilephone_number,address,recipients_name,cs_username,pay_type,get_goods_count) \
										   	values(\'%s\',now(),3,%u,\'%s\', 0, \'%s\',\'%s\',\'%s\',\'\',%u,%u);", m_msgHandler->getContext().userData,
											itemCfg->exchange_scores, itemCfg->describe.c_str(), req.telephone().c_str(), req.address().c_str(),
											req.name().c_str(), EPropType::PropScores, itemCfg->get_goods_count);

		if (Success != m_msgHandler->m_pDBOpt->modifyTable(sql_tmp))
		{
			m_msgHandler->mysqlRollback();
			OptErrorLog("exeSql failed|%s|%s", m_msgHandler->getContext().userData, sql_tmp);
			rsp.set_result(ServiceInsertFailed);
			break;
		}

		//写到memcached
		if (!m_msgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			m_msgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			break;
		}

		//提交
		m_msgHandler->mysqlCommit();

		exchangeLog(req.item_id());
		rsp.set_scores(user_baseinfo.dynamic_info.score);
		auto itemInfo = rsp.mutable_item();
		itemInfo->set_item_id(itemCfg->id);
		itemInfo->set_item_stock_num(getExchangeItemNum(itemCfg->id));
		itemInfo->set_name(itemCfg->exchange_desc);
		itemInfo->set_item_url(itemCfg->url);
		itemInfo->set_describe(itemCfg->describe);
		itemInfo->set_scores(itemCfg->exchange_scores);
		
		payCount = itemCfg->exchange_scores;
	    getCount = itemCfg->get_goods_count;

	} while (0);
			
	char send_data[MaxMsgLen] = { 0 };
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, send_data, send_data_len, "send exchange scores item");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_EXCHANGE_SCORES_ITEM_RSP);
	
	ExchangeLog("score exchange gift|%s|%d|%u|%u|%u", m_msgHandler->getContext().userData, rsp.result(), req.item_id(), payCount, getCount);
}

void CLogicHandler::chatLog(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ChatLogReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "chat log")) return;

	char sql_tmp[2048] = { 0 };
	snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_user_chat_record(username,chat_time,room_rate,chat_type,chat_context) \
									   	 values(\'%s\',now(),\'%d\',\'%s\',\'%s\');", req.user_name().c_str(), req.place(), 
										 req.target_type().c_str(), req.content().c_str());
	m_chatLogSql += sql_tmp;
}

void CLogicHandler::handlePKPlaySettlementOfAccountsReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId,
														unsigned short srcProtocolId)
{
	com_protocol::PKPlaySettlementOfAccountsReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "handle pk play settlement of accounts req")) return;

	com_protocol::PKPlaySettlementOfAccountsRsp rsp;
	rsp.set_result(0);
	rsp.set_table_id(req.table_id());		//桌子ID
	rsp.set_game_result(req.game_result());	//0.正常局 1.表示和局 2.流局

	char buff[NFrame::MaxMsgLen] = { 0 };
	for (auto it = req.player().begin(); it != req.player().end(); ++it)
	{
		auto player = rsp.add_player();
		player->set_user_name(it->user_name());		//用户名
		//player->set_nickname(it->nickname());		//昵称
		player->set_pk_score(it->pk_score());		//PK分数
		player->set_reward_id(it->reward_id());		//奖励ID
		player->set_reward_num(it->reward_num());	//奖励数量
		player->set_ranking(it->ranking());			//排名从0开始

		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_msgHandler->getUserBaseinfo(it->user_name(), user_baseinfo))
		{
			OptErrorLog("handlePKPlaySettlementOfAccountsReq, get user base info, user name:%s", it->user_name().c_str());//获取DB中的信息失败
			continue;
		}
		player->set_nickname(user_baseinfo.static_info.nickname);

		com_protocol::BuyuLogicData buyuData;
		if (getLogicData(BuyuLogicDataKey, BuyuLogicDataKeyLen, it->user_name().c_str(), it->user_name().size(), buyuData) != 0)
		{
			OptErrorLog("handle pk play settlement of accounts req, user:%s", it->user_name().c_str());
			continue;
		}

		com_protocol::PkPlayInfo* pkPlayInfo = NULL;
		if (!buyuData.has_pk_play_info())
		{
			pkPlayInfo = buyuData.mutable_pk_play_info();
			pkPlayInfo->set_win_num(0);
			pkPlayInfo->set_fail_num(0);
			pkPlayInfo->set_drawn_game_num(0);
			pkPlayInfo->set_liu_ju_num(0);
		}
		else
			pkPlayInfo = buyuData.mutable_pk_play_info();

		switch (req.game_result())
		{
		case 0:
			it->ranking() == 0 ? pkPlayInfo->set_win_num(pkPlayInfo->win_num() + 1) : pkPlayInfo->set_fail_num(pkPlayInfo->fail_num() + 1);
			break;

		case 1:
			pkPlayInfo->set_drawn_game_num(pkPlayInfo->drawn_game_num() + 1);
			break;

		case 2:
			pkPlayInfo->set_liu_ju_num(pkPlayInfo->liu_ju_num() + 1);
			break;
		}

		uint32_t sum = (pkPlayInfo->win_num() + pkPlayInfo->fail_num() + pkPlayInfo->drawn_game_num() + pkPlayInfo->liu_ju_num());
		if (sum == 0)
		{
			OptErrorLog("handlePKPlaySettlementOfAccountsReq, sum == 0, win_num:%d, fail_num:%d, drawn_game_num:%d, liu_ju_num:%d", pkPlayInfo->win_num(), pkPlayInfo->fail_num(), pkPlayInfo->drawn_game_num(), pkPlayInfo->liu_ju_num());
			break;
		}
		else
		{
			int rateOfWinning = pkPlayInfo->win_num() / float(sum) * 100;
			player->set_rate_of_winning(rateOfWinning);	//胜率
		}
		
		//设置用户数据
		buyuData.SerializeToArray(buff, NFrame::MaxMsgLen);
		m_msgHandler->getLogicRedisService().setHField(BuyuLogicDataKey, BuyuLogicDataKeyLen, it->user_name().c_str(), it->user_name().size(), buff, buyuData.ByteSize());
	}

	rsp.SerializeToArray(buff, NFrame::MaxMsgLen);
	if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_PK_PLAY_SETTLEMENT_OF_ACCOUNTS_RSP;
	m_msgHandler->sendMessage(buff, rsp.ByteSize(), srcSrvId, srcProtocolId);
}

void CLogicHandler::chatLogToMysql(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{	
	if (m_chatLogSql.empty()) return;

	CQueryResult* qResult = NULL;
	const int rc = m_msgHandler->getMySqlOptDB()->executeMultiSql(m_chatLogSql.c_str(), m_chatLogSql.size(), qResult);
	if (Success != rc) OptErrorLog("write chat log to mysql, exe sql error, rc = %d, sql = %s", rc, m_chatLogSql.c_str());

	if (qResult != NULL) m_msgHandler->getMySqlOptDB()->releaseQueryResult(qResult);
	
	m_chatLogSql.clear();
}

const MallConfigData::scores_item* CLogicHandler::getExchangeItemCfg(uint32 nItemId) const
{
	//获取需要兑换的积分商品配置 
	const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();
	for (auto itemCfg = mallData.scores_shop.scores_item_list.begin(); itemCfg != mallData.scores_shop.scores_item_list.end(); ++itemCfg)
	{
		if (itemCfg->id == nItemId)
			return &(*itemCfg);
	}

	OptErrorLog("CLogicHandler getExchangeItemNeedScores, get scores item error, id:%d", nItemId);
	return NULL;
}

uint32 CLogicHandler::getExchangeItemNum(uint32 nItemId)
{
	auto exchangeItem = m_exchangeLog.find(nItemId);
	
	if (exchangeItem == m_exchangeLog.end())
	{
		OptErrorLog("CLogicHandler getExchangeItemNum, get score item error, item id:%d", nItemId);
		return 0;
	}
	else
		return exchangeItem->second;
}

bool CLogicHandler::exchangeLog(uint32 nItemId)
{
	auto exchangeItem = m_exchangeLog.find(nItemId);
	if (exchangeItem == m_exchangeLog.end())
	{
		OptErrorLog("CLogicHandler exchangeItem, exchange item error, item id:%d", nItemId);
		return false;
	}

	if (exchangeItem->second <= 0)
	{
		OptErrorLog("CLogicHandler exchangeItem, exchange item error, item id:%d, cur num:%d", nItemId, exchangeItem->second);
		return false;
	}

	--exchangeItem->second;
	return true;
}

void CLogicHandler::reExchangeLog(const MallConfigData::ScoresShop &scoresShopCfg)
{
	m_exchangeLog.clear();

	for (auto itemCfg = scoresShopCfg.scores_item_list.begin(); itemCfg != scoresShopCfg.scores_item_list.end(); ++itemCfg)
	{
		m_exchangeLog.insert(exchange_value(itemCfg->id, itemCfg->exchange_count));
	}
}


/*
// 领取每天的超值礼包物品
void CLogicHandler::receiveSuperValuePackageGift(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{

	com_protocol::ReceiveSuperValuePackageReq receiveSuperValuePkgReq;
	if (!m_msgHandler->parseMsgFromBuffer(receiveSuperValuePkgReq, data, len, "receive super value package")) return;
	
	if (receiveSuperValuePkgReq.id() != m_gameMallCfgRsp.super_value_pkg().id())
	{
		OptErrorLog("receive super package, the id is error, user = %s, invalid id = %u, package id = %u",
		m_msgHandler->getContext().userData, receiveSuperValuePkgReq.id(), m_gameMallCfgRsp.super_value_pkg().id());
		return;
	}
	
	// 超值礼包每天赠送的物品
	com_protocol::BuyuGameRecordStorageExt receiveSuperValuePkgRecord; // 游戏记录
	google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo> goodsList;
	const google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>& giftList = m_gameMallCfgRsp.super_value_pkg().day_additional();
	for (int idx = 0; idx < giftList.size(); ++idx)
	{
		com_protocol::ItemChangeInfo* icInfo = goodsList.Add();
		icInfo->set_type(giftList.Get(idx).type());
		icInfo->set_count(giftList.Get(idx).num());
		
		com_protocol::ItemRecordStorage* itemRecord = receiveSuperValuePkgRecord.add_items();
		itemRecord->set_item_type(icInfo->type());
		itemRecord->set_charge_count(icInfo->count());
	}
	
	if (goodsList.size() < 1)
	{
		OptErrorLog("receive super package, no have gift, user = %s, package id = %u", m_msgHandler->getContext().userData, m_gameMallCfgRsp.super_value_pkg().id());
		return;
	}
	
	// 写游戏记录
	char buffer[MaxMsgLen] = { 0 };
	unsigned int msgLen = MaxMsgLen;
	
	receiveSuperValuePkgRecord.set_room_rate(0);
	receiveSuperValuePkgRecord.set_room_name("大厅");
	receiveSuperValuePkgRecord.set_remark("领取每日超值礼包");
	com_protocol::GameRecordPkg recordStore;
	const com_protocol::GameRecordPkg* record = &recordStore;
	const char* msgBuffer = m_msgHandler->serializeMsgToBuffer(receiveSuperValuePkgRecord, buffer, msgLen, "receive super value package gift record");
	if (msgBuffer != NULL)
	{
		recordStore.set_game_type(GameRecordType::BuyuExt);
		recordStore.set_game_record_bin(msgBuffer, msgLen);
	}
	else
	{
		record = NULL;
	}

    com_protocol::ReceiveSuperValuePackageRsp rsp;
	int result = m_msgHandler->changeUserPropertyValue(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, goodsList, record, NULL, rsp.mutable_gifts());
	if (result != ServiceSuccess)
	{
		rsp.clear_gifts();
		OptErrorLog("receive super package error, user = %s, result = %d", m_msgHandler->getContext().userData, result);
	}
	
	rsp.set_result(result);
	msgLen = MaxMsgLen;
	msgBuffer = m_msgHandler->serializeMsgToBuffer(rsp, buffer, msgLen, "receive super value package reply");
	if (msgBuffer != NULL) m_msgHandler->sendMessage(msgBuffer, msgLen, srcSrvId, CommonSrvBusiness::BUSINESS_RECEIVE_SUPER_VALUE_PACKAGE_RSP);
}
*/

