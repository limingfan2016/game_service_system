
/* author : liuxu
 * modify : limingfan
 * date : 2015.06.04
 * description : 游戏记录
 */
 
#ifndef __CGAME_RECORD_H__
#define __CGAME_RECORD_H__

#include "SrvFrame/CModule.h"
#include "db/CMySql.h"
#include "base/CCfg.h"
#include "base/Function.h"
#include "BaseDefine.h"


class CMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace NProject;
using namespace NDBOpt;
using namespace NErrorCode;

static const unsigned int MAX_RECORD_CACHE_COUNT = 100;  // 存储的最大记录个数，达到最大值则提交到DB


// 游戏记录信息
class SRecordDetail
{
public:
	char username[33];
	char record_time[21];
	char record_id[33];
	char record_name[33];
	
	uint32_t item_type;
	int32_t charge_count;
	double cur_count;
	
	char remark[33];
};

// 游戏记录数据
class SRecordCache
{
public:
	int32_t cur_count;
	uint32_t last_exe_timestamp;
	SRecordDetail record[MAX_RECORD_CACHE_COUNT];
};


class CGameRecord
{
public:
	CGameRecord();
	~CGameRecord();

public:
    void procUserGameRecord(const com_protocol::UserGameRecordPkg& pkg);
	void procGameRecord(const com_protocol::GameRecordPkg& pkg, const CUserBaseinfo* p_user_info);
	
public:
	bool commitAllGameRecord();
	bool checkCommitGameRecord();
	
public:
	bool init(CMsgHandler* p_msg_handler);
	void unInit();
	
	bool loadConfig();

private:
	void procRecord(const char* cur_date_time, uint32_t item_type, int32_t charge_count, const char* remark,
	                const uint32_t db_table_id, const char* record_name, const char* record_id, const CUserBaseinfo* p_user_info);
	
	bool commitAllRecord(bool is_check_timeout);
	bool commitRecord(int32_t db_table_id);

private:
	void unInitMysqlHandler();
	bool initMysqlHandler();

	// 取用户当前道具数量
	bool getCurCount(uint32_t item_type, const CUserBaseinfo* p_user_info, double& cur_count);

private:
	CMsgHandler* m_pMsgHandler;
	
	SRecordCache* m_arrRecord;
	
	CDBOpertaion* m_pRecordDBOpt;
};


#endif // __CGAME_RECORD_H__
