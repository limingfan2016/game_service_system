
/* author : liuxu
 * modify : limingfan
 * date : 2015.03.03
 * description : 消息定义文件
 */
 
#ifndef __BESE_DEFINE_H__
#define __BESE_DEFINE_H__

#include <string>
#include <vector>
#include <unordered_map>

#include "../common/DBProxyProtocolId.h"
#include "../common/DBProxyErrorCode.h"
#include "../common/CommonType.h"
#include "../common/CServiceOperation.h"
#include "../common/MessageCenterProtocolId.h"

#include "_DbProxyConfig_.h"
#include "protocol/appsrv_db_proxy.pb.h"


using namespace std;

static const unsigned int DBStringFieldLength = 36;

#pragma pack(1)
class CUserStaticBaseinfo
{
public:
	uint32_t register_time;
	char username[DBStringFieldLength];
	char password[DBStringFieldLength];
	char nickname[DBStringFieldLength];
	char portrait_id[512];
	char person_sign[65];
	char mobile_phone[17];
	char email[37];
	int32_t gender;
	
	uint32_t city_id;
	uint32_t status;
	
	/*
	char realname[17];
	char qq_num[17];
	char address[65];
	char birthday[12];
	uint32_t age;
	char idc[25];
	char other_contact[65];
	*/ 
};

class CUserDynamicBaseinfo
{
public:
	uint32_t create_time;
	char hall_id[DBStringFieldLength];
	char username[DBStringFieldLength];
    uint32_t user_status;

	double rmb_gold;                      // RMB货币数量
	double game_gold;                     // 游戏金币数量
	double room_card;                     // 房卡数量
	
	double profit_loss;                   // 输赢利润总额
	double tax_cost;                      // 扣税服务费用等
	double exchange;                      // 兑换总额
	
	double recharge;                      // 充值总额度RMB单位元
	uint32_t recharge_times;              // 充值次数
	
	uint32_t sum_online_secs;             // 累计在线时间
	uint32_t sum_login_times;             // 累计登陆次数
	char login_ip[DBStringFieldLength];   // 登陆IP
	
	/*
	uint32_t level;
	uint32_t cur_level_exp;
	uint32_t vip_level;
	uint32_t vip_cur_level_exp;
	char vip_time_limit[24];
	double rmb_gold;                      // RMB货币数量
	double game_gold;                     // 游戏金币数量
	double room_card;                     // 房卡数量
	double sum_recharge;                  // 充值总额度RMB单位元
	uint32_t sum_recharge_count;          // 充值次数
	char last_login_time[24];
	char last_logout_time[24];
	*/ 
};

class CUserBaseinfo
{
public:
	CUserStaticBaseinfo static_info;
	CUserDynamicBaseinfo dynamic_info;
};
#pragma pack()


// 用户数据修改统计信息
class CUserModifyStat
{
public:
	uint32_t modify_count;
	uint32_t first_modify_timestamp;
	bool is_logout;
};

// 棋牌室排名数据
struct SHallRanking
{
	string username;
	double value;
	unsigned int gameType;
	
	SHallRanking() : value(0.00), gameType(0) {};
	SHallRanking(const string& usrn, const double vl, unsigned int gt) : username(usrn), value(vl), gameType(gt) {};
	~SHallRanking() {};
	
	bool operator== (const string& usrn) const
	{
		return username == usrn;
	}
	
	bool operator< (const SHallRanking& obj) const
	{
		return value > obj.value;  // 降序排序
	}
};
typedef vector<SHallRanking> SHallRankingVector;


// 用户逻辑数据
struct UserLogicData
{
	// com_protocol::DBCommonSrvLogicData logicData;
	int isUpdate;
};
typedef unordered_map<string, UserLogicData> UserToLogicData;


// 在线房间用户数据
struct OnlineUser
{
	unsigned int serviceId;
	string roomId;
	int seatId;
	
	OnlineUser() {};
	OnlineUser(unsigned int srvId, const string& rmId, int stId) : serviceId(srvId), roomId(rmId), seatId(stId) {};
	~OnlineUser() {};
};
typedef unordered_map<string, OnlineUser> OnlineUserMap;


// 缓存最近一起玩游戏的玩家
typedef unordered_map<string, NProject::StringInList> LastPlayerList;              // 玩家到一起游戏的玩家列表映射
typedef unordered_map<string, LastPlayerList> HallLastPlayerList;      // 棋牌室到玩家列表映射


#endif // __BESE_DEFINE_H__

