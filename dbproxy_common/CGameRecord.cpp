
/* author : liuxu
* date : 2015.06.04
* description : 游戏记录
*/
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "../common/CommonType.h"
#include "base/Function.h"
#include "CGameRecord.h"
#include "MessageDefine.h"
#include "_MallConfigData_.h"
#include "CSrvMsgHandler.h"
#include "SrvFrame/CModule.h"
#include "CLogic.h"
#include "base/Function.h"
#include "db/CMySql.h"
#include "base/CMD5.h"


using namespace NProject;

CGameRecord::CGameRecord()
{
	m_arrBuyuRecord = NULL;

	m_p_msg_handler = NULL;
	for (int i = 0; i < NProject::GameType::MAX_GAME_TYPE; i++)
	{
		m_p_arrDBOpt[i] = NULL;
	}
}

CGameRecord::~CGameRecord()
{
	if (m_arrBuyuRecord)
	{
		DELETE_ARRAY(m_arrBuyuRecord);
	}
	
	m_p_msg_handler = NULL;
}

void CGameRecord::setParam(CSrvMsgHandler *p_msg_handler)
{
	m_p_msg_handler = p_msg_handler;
}

bool CGameRecord::loadConfig()
{
	return true;
}

bool CGameRecord::init()
{
	unInitMysqlHandler();
	if (!initMysqlHandler())
	{
		return false;
	}
	NEW_ARRAY(m_arrBuyuRecord, SBuyuRecordCache[m_p_msg_handler->m_config.game_record_config[NProject::GameType::BUYU].table_count]);
	uint32_t cur_timestamp = time(NULL);
	for (uint32_t i = 0; i < m_p_msg_handler->m_config.game_record_config[NProject::GameType::BUYU].table_count; i++)
	{
		m_arrBuyuRecord[i].cur_count = 0;
		m_arrBuyuRecord[i].last_exe_timestamp = cur_timestamp;
	}

	return true;
}

void CGameRecord::unInitMysqlHandler()
{
	for (int i = NProject::GameType::BUYU; i < NProject::GameType::MAX_GAME_TYPE; i++)
	{
		if (m_p_arrDBOpt[i])
		{
			CMySql::destroyDBOpt(m_p_arrDBOpt[i]);
			m_p_arrDBOpt[i] = NULL;
		}
	}
}

bool CGameRecord::initMysqlHandler()
{
	const vector<DbproxyCommonConfig::GameRecordConfig> &game_record_config = m_p_msg_handler->m_config.game_record_config;
	for (int i = NProject::GameType::BUYU; i < NProject::GameType::MAX_GAME_TYPE; i++)
	{
		if (0 != CMySql::createDBOpt(m_p_arrDBOpt[i], game_record_config[i].mysql_ip.c_str(), game_record_config[i].mysql_username.c_str(),
			game_record_config[i].mysql_password.c_str(), game_record_config[i].mysql_dbname.c_str(), game_record_config[i].mysql_port))
		{
			ReleaseErrorLog("Game record CMySql::createDBOpt failed. ip:%s username:%s passwd:%s dbname:%s port:%d", game_record_config[i].mysql_ip.c_str(),
				game_record_config[i].mysql_username.c_str(), game_record_config[i].mysql_password.c_str(), game_record_config[i].mysql_dbname.c_str(), game_record_config[i].mysql_port);
			return false;
		}
	}

	return true;
}

bool CGameRecord::getCurCount(uint32_t item_type, const CUserBaseinfo *p_user_info, uint64_t &cur_count)
{
	if (p_user_info == NULL)
	{
		OptErrorLog("CGameRecord getCurCount, p_user_info == NULL");
		return false;
	}

    item_type = parseGameGoodsType(item_type);
	switch (item_type)
	{
	case PropGold:				//金币
		cur_count = p_user_info->dynamic_info.game_gold;
		break;
	case PropFishCoin:				//渔币
		cur_count = p_user_info->dynamic_info.rmb_gold;
		break;
	case PropTelephoneFare:		//话费卡
		cur_count = p_user_info->dynamic_info.phone_card_number;
		break;
	case PropSuona:				//小喇叭
		cur_count = p_user_info->prop_info.suona_count;
		break;
	case PropLightCannon:		//激光炮
		cur_count = p_user_info->prop_info.light_cannon_count;
		break;
	case PropFlower:				//鲜花
		cur_count = p_user_info->prop_info.flower_count;
		break;
	case PropMuteBullet:			//哑弹
		cur_count = p_user_info->prop_info.mute_bullet_count;
		break;
	case PropSlipper:			//拖鞋
		cur_count = p_user_info->prop_info.slipper_count;
		break;
	case PropVoucher:			//奖券
		cur_count = p_user_info->dynamic_info.voucher_number;
		break;
	case PropAutoBullet:			//自动炮子弹
		cur_count = p_user_info->prop_info.auto_bullet_count;
		break;
	case PropLockBullet:			//锁定炮子弹
		cur_count = p_user_info->prop_info.lock_bullet_count;
		break;
	case PropDiamonds:			//钻石
		cur_count = p_user_info->dynamic_info.diamonds_number;
		break;
	case PropPhoneFareValue:	//话费额度
		cur_count = p_user_info->dynamic_info.phone_fare;
		break;

	case PropScores:			//积分
		cur_count = p_user_info->dynamic_info.score;
		break;

	case PropRampage:			//狂暴
		cur_count = p_user_info->prop_info.rampage_count;
		break;

	case PropDudShield:			//哑弹防护
		cur_count = p_user_info->prop_info.dud_shield_count;
		break;

	case EUserInfoFlag::EVipLevel:	        // VIP 等级
		cur_count = p_user_info->dynamic_info.vip_level;
		break;
	case EUserInfoFlag::ERechargeAmount:	// 累积充值的总额度
		cur_count = p_user_info->dynamic_info.sum_recharge;
		break;
		
	case EPropType::EPKDayGoldTicket:	   // PK场全天金币对战门票
	case EPropType::EPKHourGoldTicket:	   // PK场限时金币对战门票
	{
		cur_count = getPKGoldTicketCount(item_type, p_user_info);
		break;
	}
	
	default:
		OptErrorLog("procBuyuGameRecord, the item type invalid, user = %s, type = %u,", p_user_info->static_info.username, item_type);
		return false;
	}
	
	return true;
}

// 获取PK场金币对战门票数量
uint64_t CGameRecord::getPKGoldTicketCount(uint32_t item_type, const CUserBaseinfo *p_user_info)
{
	unsigned int allCount = 0;
	unsigned int dayCount = 0;
	const com_protocol::PKTicket& pkTicket = m_p_msg_handler->getLogicDataInstance().getLogicData(p_user_info->static_info.username, strlen(p_user_info->static_info.username)).logicData.pk_ticket();
	for (int idx = 0; idx < pkTicket.gold_ticket_size(); ++idx)
	{
		const com_protocol::Ticket& ticket = pkTicket.gold_ticket(idx);
		allCount += ticket.count();
		if (!ticket.has_begin_hour()) dayCount += ticket.count();
	}
	
	return (item_type == EPropType::EPKDayGoldTicket) ? dayCount : (allCount - dayCount);
}

void CGameRecord::procGameRecord(const com_protocol::GameRecordPkg &pkg, const CUserBaseinfo *p_user_info, bool bRecordProp)
{
	switch (pkg.game_type())
	{
	case NProject::GameRecordType::Game:
	case NProject::GameRecordType::Buyu:
		procBuyuGameRecord(pkg, p_user_info);
		break;
		
	case NProject::GameRecordType::BuyuExt:
		procBuyuGameRecordExt(pkg, p_user_info, bRecordProp);
		break;

	default:
		OptErrorLog("procGameRecord|Unknow game_type:%u", pkg.game_type());
		break;
	}
}

bool CGameRecord::commitAllRecord()
{
	bool ret = commitBuyuAllRecord();

	return ret;
}

bool CGameRecord::checkCommitRecord()
{
	bool ret = commitBuyuAllRecord(true);

	return ret;
}

bool CGameRecord::commitBuyuAllRecord(bool is_check_timeout)
{
	uint32_t cur_timestamp = time(NULL);
	bool ret = false;
	for (size_t i = 0; i < m_p_msg_handler->m_config.game_record_config[NProject::GameType::BUYU].table_count; i++)
	{
		ret = true;
		if (is_check_timeout && cur_timestamp - m_arrBuyuRecord[i].last_exe_timestamp >= m_p_msg_handler->m_config.server_config.check_time_gap)
		{
			ret = commitBuyuTableRecord(i);
		}
		else if (!is_check_timeout)
		{
			ret = commitBuyuTableRecord(i);
		}

		if (!ret)
		{
			return ret;
		}
	}
	return true;
}

bool CGameRecord::commitBuyuTableRecord(int32_t table_id)
{
	if (m_arrBuyuRecord == NULL || m_arrBuyuRecord[table_id].cur_count == 0)
	{
		return true;
	}

	//格式sql语句
	int sql_size = 0;
	static char sql[1024 * MAX_BUYU_RECOUND_CACHE_COUNT] = { 0 };
	SBuyuRecordCache *precord_cache = &m_arrBuyuRecord[table_id];
	int cur_index = 0;
	sql_size = snprintf(sql, sizeof(sql), "insert into tb_buyu_game_records_%03u(username,record_time,room_rate,room_name,item_type,charge_count,cur_count,remark,record_id)", table_id);
	sql_size += snprintf(sql + sql_size, sizeof(sql)-sql_size, " values(\'%s\', \'%s\', %u, \'%s\', %u, %d, %lu, \'%s\', \'%s\')", precord_cache->record[cur_index].username,
		precord_cache->record[cur_index].record_time, precord_cache->record[cur_index].room_rate, precord_cache->record[cur_index].room_name, precord_cache->record[cur_index].item_type,
		precord_cache->record[cur_index].charge_count, precord_cache->record[cur_index].cur_count, precord_cache->record[cur_index].remark, precord_cache->record[cur_index].record_id);
	for (cur_index = 1; cur_index < precord_cache->cur_count; cur_index++)
	{
		sql_size += snprintf(sql + sql_size, sizeof(sql)-sql_size, ",(\'%s\', \'%s\', %u, \'%s\', %u, %d, %lu, \'%s\', \'%s\')", precord_cache->record[cur_index].username,
			precord_cache->record[cur_index].record_time, precord_cache->record[cur_index].room_rate, precord_cache->record[cur_index].room_name, precord_cache->record[cur_index].item_type,
			precord_cache->record[cur_index].charge_count, precord_cache->record[cur_index].cur_count, precord_cache->record[cur_index].remark, precord_cache->record[cur_index].record_id);
	}
	sql_size += snprintf(sql + sql_size, sizeof(sql)-sql_size, ";");

	//统计时间
	unsigned long long start;
	unsigned long long end;
	struct timeval startTime;
	struct timeval endTime;
	gettimeofday(&startTime, NULL);
	start = startTime.tv_sec * 1000000 + startTime.tv_usec;

	//执行sql
	int rc = m_p_arrDBOpt[NProject::GameType::BUYU]->executeSQL(sql, sql_size);

	gettimeofday(&endTime, NULL);
	end = endTime.tv_sec * 1000000 + endTime.tv_usec;
	unsigned long long timeuse = end - start;
	ReleaseInfoLog("muilty sql exe|%d|%lu|%lf", precord_cache->cur_count, timeuse, 1000000.0 / ((double)timeuse / (double)precord_cache->cur_count));
	if (Success != rc)
	{
		ReleaseErrorLog("insert error|%d|%s", rc, sql);
	}

	//重新初使化
	precord_cache->cur_count = 0;
	precord_cache->last_exe_timestamp = time(NULL);

	return rc == ECommon::Success;
}

void CGameRecord::procBuyuGameRecord(const com_protocol::GameRecordPkg &pkg, const CUserBaseinfo *p_user_info)
{
	com_protocol::BuyuGameRecordStorage record;
	if (!record.ParseFromArray(pkg.game_record_bin().c_str(), pkg.game_record_bin().length()))
	{
		char log[1024];
		b2str(pkg.game_record_bin().c_str(), (int)pkg.game_record_bin().length(), log, (int)sizeof(log));
		OptErrorLog("--- procBuyuGameRecord--- unpack failed.| len:%u, data:%s", pkg.game_record_bin().length(), log);
		return;
	}

	//获取日期
	char cur_date_time[21] = { 0 };
	uint32_t table_id = strToHashValue(p_user_info->static_info.username) % m_p_msg_handler->m_config.game_record_config[NProject::GameType::BUYU].table_count;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm* pTm = localtime(&tv.tv_sec);
	snprintf(cur_date_time, sizeof(cur_date_time), "%04d-%02d-%02d %02d:%02d:%02d", (pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday,
		pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
		
	procBuyuGameRecord(cur_date_time, record.room_name().c_str(), record.room_rate(), record.item_type(), record.charge_count(), record.remark().c_str(), table_id,
	                   record.record_id().c_str(), p_user_info);
}

void CGameRecord::procBuyuGameRecordExt(const com_protocol::GameRecordPkg &pkg, const CUserBaseinfo *p_user_info, bool bRecordProp)
{
	com_protocol::BuyuGameRecordStorageExt record;
	if (!record.ParseFromArray(pkg.game_record_bin().c_str(), pkg.game_record_bin().length()))
	{
		char log[1024];
		b2str(pkg.game_record_bin().c_str(), (int)pkg.game_record_bin().length(), log, (int)sizeof(log));
		OptErrorLog("--- procBuyuGameRecordExt--- unpack failed.| len:%u, data:%s", pkg.game_record_bin().length(), log);
		return;
	}

	//获取日期
	char cur_date_time[21] = { 0 };
	uint32_t table_id = strToHashValue(p_user_info->static_info.username) % m_p_msg_handler->m_config.game_record_config[NProject::GameType::BUYU].table_count;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm* pTm = localtime(&tv.tv_sec);
	snprintf(cur_date_time, sizeof(cur_date_time), "%04d-%02d-%02d %02d:%02d:%02d", (pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday,
		pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
		
	for (int idx = 0; idx < record.items_size(); ++idx)
	{
		const com_protocol::ItemRecordStorage& item = record.items(idx);
		procBuyuGameRecord(cur_date_time, record.room_name().c_str(), record.room_rate(), item.item_type(), item.charge_count(), record.remark().c_str(), table_id,
			record.record_id().c_str(), p_user_info, bRecordProp);
	}
}

void CGameRecord::procBuyuGameRecord(const char* cur_date_time, const char* room_name, int32_t room_rate, uint32_t item_type, int32_t charge_count,
                                     const char* remark, uint32_t table_id, const char* record_id, const CUserBaseinfo *p_user_info, bool bRecordProp)
{
	//获取对应物品当前数量
	uint64_t cur_count = 0;
	if (bRecordProp)
	{
		if (!getCurCount(item_type, p_user_info, cur_count))
			return;
	}
	
	//把数据放到缓冲
	int cur_index = m_arrBuyuRecord[table_id].cur_count;
	SBuyuRecordDetail *pcur_record = &(m_arrBuyuRecord[table_id].record[cur_index]);
	strncpy(pcur_record->username, p_user_info->static_info.username, sizeof(pcur_record->username) - 1);
	pcur_record->username[sizeof(pcur_record->username) - 1] = '\0';
	strncpy(pcur_record->record_time, cur_date_time, sizeof(pcur_record->record_time) - 1);
	pcur_record->record_time[sizeof(pcur_record->record_time) - 1] = '\0';
	strncpy(pcur_record->room_name, room_name, sizeof(pcur_record->room_name) - 1);
	pcur_record->room_name[sizeof(pcur_record->room_name) - 1] = '\0';
	strncpy(pcur_record->remark, remark, sizeof(pcur_record->remark) - 1);
	pcur_record->remark[sizeof(pcur_record->remark) - 1] = '\0';
	pcur_record->room_rate = room_rate;
	pcur_record->item_type = item_type;
	pcur_record->charge_count = charge_count;
	pcur_record->cur_count = cur_count;
	strncpy(pcur_record->record_id, record_id, sizeof(pcur_record->record_id) - 1);
	
	m_arrBuyuRecord[table_id].cur_count++;
	if (0 == cur_index)
	{
		m_arrBuyuRecord[table_id].last_exe_timestamp = time(NULL);
	}

	//缓冲区满了就一次性写到DB
	if (MAX_BUYU_RECOUND_CACHE_COUNT == m_arrBuyuRecord[table_id].cur_count)
	{
		commitBuyuTableRecord(table_id);
	}
}

