
/* author : liuxu
 * modify : limingfan
 * date : 2015.06.04
 * description : 游戏记录
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "CGameRecord.h"
#include "CMsgHandler.h"


CGameRecord::CGameRecord()
{
	m_pMsgHandler = NULL;
	m_arrRecord = NULL;
	m_pRecordDBOpt = NULL;
}

CGameRecord::~CGameRecord()
{
	if (m_arrRecord)
	{
		DELETE_ARRAY(m_arrRecord);
	}
	
	m_pMsgHandler = NULL;
	m_pRecordDBOpt = NULL;
}

bool CGameRecord::init(CMsgHandler* p_msg_handler)
{
	m_pMsgHandler = p_msg_handler;
	
	/*
	unInitMysqlHandler();
	if (!initMysqlHandler()) return false;

    uint32_t cur_timestamp = time(NULL);
	NEW_ARRAY(m_arrRecord, SRecordCache[m_pMsgHandler->m_pDBCfg->record_db_cfg.db_table_count]);
	for (uint32_t i = 0; i < m_pMsgHandler->m_pDBCfg->record_db_cfg.db_table_count; ++i)
	{
		m_arrRecord[i].cur_count = 0;
		m_arrRecord[i].last_exe_timestamp = cur_timestamp;
	}
    */

	return true;
}

void CGameRecord::unInit()
{
	// unInitMysqlHandler();
}

bool CGameRecord::loadConfig()
{
	return true;
}

bool CGameRecord::initMysqlHandler()
{
	const DBConfig::MysqlRecordDBCfg& grdCfg = m_pMsgHandler->getSrvOpt().getDBCfg().record_db_cfg;
	if (0 != CMySql::createDBOpt(m_pRecordDBOpt, grdCfg.ip.c_str(), grdCfg.username.c_str(),
		grdCfg.password.c_str(), grdCfg.dbname.c_str(), grdCfg.port))
	{
		ReleaseErrorLog("Game record CMySql::createDBOpt failed. ip:%s username:%s passwd:%s dbname:%s port:%d",
	    grdCfg.ip.c_str(), grdCfg.username.c_str(), grdCfg.password.c_str(),
		grdCfg.dbname.c_str(), grdCfg.port);
		
		return false;
	}

	return true;
}

void CGameRecord::unInitMysqlHandler()
{
	if (m_pRecordDBOpt)
	{
		CMySql::destroyDBOpt(m_pRecordDBOpt);
		m_pRecordDBOpt = NULL;
	}
}


bool CGameRecord::getCurCount(uint32_t item_type, const CUserBaseinfo* p_user_info, double& cur_count)
{
	if (p_user_info == NULL)
	{
		OptErrorLog("get current item count, p_user_info == NULL");
		
		return false;
	}

    const unsigned int itemType = parseGameGoodsType(item_type);
	switch (itemType)
	{
	case EGoodsRMB:
	    cur_count = p_user_info->dynamic_info.rmb_gold;
	    break;
	
	case EGoodsGold:
		cur_count = p_user_info->dynamic_info.game_gold;
		break;
		
	case EGoodsRoomCard:
	    cur_count = p_user_info->dynamic_info.room_card;
	    break;
	
	default:
		OptErrorLog("get goods type for record, the type invalid, user = %s, type = %u, parse type = %u",
		p_user_info->static_info.username, item_type, itemType);
		return false;
	}
	
	return true;
}

void CGameRecord::procRecord(const char* cur_date_time, uint32_t item_type, int32_t charge_count, const char* remark,
                             const uint32_t db_table_id, const char* record_name, const char* record_id, const CUserBaseinfo* p_user_info)
{
	// 获取对应物品当前数量
	double goods_cur_count = 0;
	if (!getCurCount(item_type, p_user_info, goods_cur_count)) return;
	
	// 把数据放到缓冲
	const int cur_index = m_arrRecord[db_table_id].cur_count;
	if (0 == cur_index) m_arrRecord[db_table_id].last_exe_timestamp = time(NULL);
	
	SRecordDetail* pcur_record = &(m_arrRecord[db_table_id].record[cur_index]);
	strncpy(pcur_record->username, p_user_info->static_info.username, sizeof(pcur_record->username) - 1);
	strncpy(pcur_record->record_time, cur_date_time, sizeof(pcur_record->record_time) - 1);
	strncpy(pcur_record->record_name, record_name, sizeof(pcur_record->record_name) - 1);
	strncpy(pcur_record->record_id, record_id, sizeof(pcur_record->record_id) - 1);
	strncpy(pcur_record->remark, remark, sizeof(pcur_record->remark) - 1);

	pcur_record->item_type = item_type;
	pcur_record->charge_count = charge_count;
	pcur_record->cur_count = goods_cur_count;
	
	++m_arrRecord[db_table_id].cur_count;

	// 缓冲区满了或者达到配置数量值则一次性写到DB
	if (m_arrRecord[db_table_id].cur_count >= (int)MAX_RECORD_CACHE_COUNT
	    || m_arrRecord[db_table_id].cur_count >= (int)m_pMsgHandler->getSrvOpt().getDBCfg().record_db_cfg.need_commit_count)
	{
		commitRecord(db_table_id);
	}
}

bool CGameRecord::commitAllRecord(bool is_check_timeout)
{
	/*
	const uint32_t cur_timestamp = time(NULL);
	const unsigned int dbTableCount = m_pMsgHandler->m_pDBCfg->record_db_cfg.db_table_count;
	const unsigned int checkTimeGap = m_pMsgHandler->m_pDBCfg->record_db_cfg.commit_time_gap;
	
	for (unsigned int i = 0; i < dbTableCount; i++)
	{
		if (!is_check_timeout || (cur_timestamp - m_arrRecord[i].last_exe_timestamp) >= checkTimeGap)
		{
			if (!commitRecord(i)) return false;
		}
	}
	*/

	return true;
}

bool CGameRecord::commitRecord(int32_t db_table_id)
{
	if (m_arrRecord == NULL || m_arrRecord[db_table_id].cur_count == 0)
	{
		return true;
	}

	// 格式sql语句
	int cur_index = 0;
	static char sql[1024 * MAX_RECORD_CACHE_COUNT] = {0};
	SRecordCache* precord_cache = &m_arrRecord[db_table_id];
	const SRecordDetail& fRdDetail = precord_cache->record[cur_index];
	
	unsigned int sql_size = snprintf(sql, sizeof(sql) - 1, "insert into tb_user_game_records_%03u(username,record_time,record_name,record_id,item_type,charge_count,cur_count,remark) values(\'%s\', \'%s\', \'%s\', \'%s\', %u, %d, %.2f, \'%s\')",
	                                 db_table_id, fRdDetail.username, fRdDetail.record_time, fRdDetail.record_name,
									 fRdDetail.record_id, fRdDetail.item_type, fRdDetail.charge_count, fRdDetail.cur_count, fRdDetail.remark);
	for (cur_index = 1; cur_index < precord_cache->cur_count; ++cur_index)
	{
		const SRecordDetail& rdDetail = precord_cache->record[cur_index];
		sql_size += snprintf(sql + sql_size, sizeof(sql) - sql_size - 1, ",(\'%s\', \'%s\', \'%s\', \'%s\', %u, %d, %.2f, \'%s\')",
		                     rdDetail.username, rdDetail.record_time, rdDetail.record_name, rdDetail.record_id,
							 rdDetail.item_type, rdDetail.charge_count, rdDetail.cur_count, rdDetail.remark);
	}
	sql_size += snprintf(sql + sql_size, sizeof(sql) - sql_size - 1, ";");

	// 统计时间
	unsigned long long start;
	unsigned long long end;
	struct timeval startTime;
	struct timeval endTime;
	gettimeofday(&startTime, NULL);
	start = startTime.tv_sec * 1000000 + startTime.tv_usec;

	// 执行sql
	int rc = m_pRecordDBOpt->executeSQL(sql, sql_size);

	gettimeofday(&endTime, NULL);
	end = endTime.tv_sec * 1000000 + endTime.tv_usec;
	unsigned long long timeuse = end - start;
	ReleaseInfoLog("write game record muilty sql exe|%d|%lu|%lf", precord_cache->cur_count, timeuse, 1000000.0 / ((double)timeuse / (double)precord_cache->cur_count));
	if (Success != rc)
	{
		ReleaseErrorLog("write game record insert error|%d|%s", rc, sql);
	}

	// 重新初使化
	precord_cache->cur_count = 0;
	precord_cache->last_exe_timestamp = time(NULL);

	return rc == ECommon::Success;
}


void CGameRecord::procUserGameRecord(const com_protocol::UserGameRecordPkg& pkg)
{
	CUserBaseinfo user_baseinfo;
	for (int idx = 0; idx < pkg.game_record_size(); ++idx)
	{
		const com_protocol::GameRecordPkg& userPkg = pkg.game_record(idx);
		if (m_pMsgHandler->getUserBaseinfo(userPkg.username(), "", user_baseinfo) == SrvOptSuccess)
		{
		    procGameRecord(userPkg, &user_baseinfo);
		}
		else
		{
			OptErrorLog("get user info for game record error, username = %s", userPkg.username().c_str());
		}
	}
}

void CGameRecord::procGameRecord(const com_protocol::GameRecordPkg& pkg, const CUserBaseinfo* p_user_info)
{
	com_protocol::GameRecordStorage record;
	if (!record.ParseFromArray(pkg.game_record_bin().c_str(), pkg.game_record_bin().length()))
	{
		OptErrorLog("handle game record unpack error.| len:%u, user:%s",
		pkg.game_record_bin().length(), p_user_info->static_info.username);
		
		return;
	}

	// 获取日期
	char cur_date_time[21] = { 0 };
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm* pTm = localtime(&tv.tv_sec);
	snprintf(cur_date_time, sizeof(cur_date_time) - 1, "%04d-%02d-%02d %02d:%02d:%02d",
	         (pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
		
    // 用户名索引到记录DB中的记录表ID
	uint32_t db_table_id = strToHashValue(p_user_info->static_info.username) % m_pMsgHandler->getSrvOpt().getDBCfg().record_db_cfg.db_table_count;
	for (int idx = 0; idx < record.items_size(); ++idx)
	{
		const com_protocol::ItemRecordStorage& item = record.items(idx);
		procRecord(cur_date_time, item.item_type(), item.charge_count(), record.remark().c_str(), db_table_id,
			       record.record_name().c_str(), record.record_id().c_str(), p_user_info);
	}
}

bool CGameRecord::commitAllGameRecord()
{
	return commitAllRecord(false);
}

bool CGameRecord::checkCommitGameRecord()
{
	return commitAllRecord(true);
}
