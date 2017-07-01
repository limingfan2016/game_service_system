
/* author : liuxu
* date : 2015.06.04
* description : 游戏记录
*/
 
#ifndef __CGAME_RECORD_H__
#define __CGAME_RECORD_H__

#include <string>
#include "SrvFrame/CModule.h"
#include "db/CMySql.h"
#include "db/CMemcache.h"
#include "app_server_dbproxy_common.pb.h"
#include "base/CCfg.h"
#include "_DbproxyCommonConfig_.h"
#include "../common/CommonType.h"

class CSrvMsgHandler;
class CUserBaseinfo;

using namespace NCommon;
using namespace NFrame;
using namespace std;
using namespace NDBOpt;
using namespace NErrorCode;

#define MAX_BUYU_RECOUND_CACHE_COUNT 100


class SBuyuRecordDetail
{
public:
	char username[33];
	char record_time[21];
	char room_name[33];
	char remark[33];
	int32_t room_rate;
	uint32_t item_type;
	int32_t charge_count;
	uint64_t cur_count;
	char record_id[33];
};

class SBuyuRecordCache
{
public:
	int32_t cur_count;
	uint32_t last_exe_timestamp;
	SBuyuRecordDetail record[MAX_BUYU_RECOUND_CACHE_COUNT];
};

class CGameRecord
{
public:
	CGameRecord();
	~CGameRecord();

	void setParam(CSrvMsgHandler *p_msg_handler);
	bool loadConfig();
	bool init();

	void procGameRecord(const com_protocol::GameRecordPkg &pkg, const CUserBaseinfo *p_user_info, bool bRecordProp = true);
	bool commitAllRecord();
	bool checkCommitRecord();

private:
	void procBuyuGameRecord(const com_protocol::GameRecordPkg &pkg, const CUserBaseinfo *p_user_info);
	void procBuyuGameRecordExt(const com_protocol::GameRecordPkg &pkg, const CUserBaseinfo *p_user_info, bool bRecordProp = true);
	void procBuyuGameRecord(const char* cur_date_time, const char* room_name, int32_t room_rate, uint32_t item_type, int32_t charge_count,
		const char* remark, uint32_t table_id, const char* record_id, const CUserBaseinfo *p_user_info, bool bRecordProp = true);
	bool commitBuyuAllRecord(bool is_check_timeout = false);
	bool commitBuyuTableRecord(int32_t table_id);

private:
	void unInitMysqlHandler();
	bool initMysqlHandler();

	//取用户当前道具数量
	bool getCurCount(uint32_t item_type, const CUserBaseinfo *p_user_info, uint64_t &cur_count);
	
	// 获取PK场金币对战门票数量
	uint64_t getPKGoldTicketCount(uint32_t item_type, const CUserBaseinfo *p_user_info);

private:
	CSrvMsgHandler *m_p_msg_handler;
	CDBOpertaion *m_p_arrDBOpt[NProject::GameType::MAX_GAME_TYPE];

	SBuyuRecordCache *m_arrBuyuRecord;
};

#endif // __CLOGIC_H__
