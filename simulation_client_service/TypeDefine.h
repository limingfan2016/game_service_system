
/* author : limingfan
 * date : 2015.09.21
 * description : 类型定义
 */

#ifndef TYPE_DEFINE_H
#define TYPE_DEFINE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "SrvFrame/UserType.h"
#include "app_simulation_client.pb.h"


using namespace std;

namespace NPlatformService
{

static const unsigned int MaxPlatformIdLen = 64;
static const unsigned int MaxUserIdLen = 36;
static const unsigned int RobotBlockCount = 16;         // 扩展的机器人数据内存块个数


// 协议操作返回码
enum ProtocolReturnCode
{
	Opt_Failed = -1,
	Opt_Success = 0,                       // 操作成功
};

// 仿真客户端协议ID值
enum SimulationClientProtocol
{
	// 机器人登陆、登出消息
	SIMULATION_CLIENT_ROBOT_LOGIN_NOTIFY = 1,
	SIMULATION_CLIENT_ROBOT_LOGOUT_NOTIFY = 2,
};

// dbcommon ServiceType::CommonSrv = 0
enum DbCommonSrvBusiness
{
	BUSINESS_REGISTER_USER_REQ = 1,
	BUSINESS_REGISTER_USER_RSP = 2,

	BUSINESS_MODIFY_PASSWORD_REQ = 3,
	BUSINESS_MODIFY_PASSWORD_RSP = 4,

	BUSINESS_MODIFY_BASEINFO_REQ = 5,
	BUSINESS_MODIFY_BASEINFO_RSP = 6,

	BUSINESS_VERIYFY_PASSWORD_REQ = 7,
	BUSINESS_VERIYFY_PASSWORD_RSP = 8,

	BUSINESS_GET_USER_BASEINFO_REQ = 9,
	BUSINESS_GET_USER_BASEINFO_RSP = 10,

	BUSINESS_ROUND_END_DATA_CHARGE_REQ = 11,
	BUSINESS_ROUND_END_DATA_CHARGE_RSP = 12,

	BUSINESS_LOGIN_NOTIFY_REQ = 13,
	BUSINESS_LOGIN_NOTIFY_RSP = 14,

	BUSINESS_LOGOUT_NOTIFY_REQ = 15,
	BUSINESS_LOGOUT_NOTIFY_RSP = 16,

	BUSINESS_GET_MULTI_USER_INFO_REQ = 17,
	BUSINESS_GET_MULTI_USER_INFO_RSP = 18,
	
	BUSINESS_GET_MALL_CONFIG_REQ = 19,
	BUSINESS_GET_MALL_CONFIG_RSP = 20,

	BUSINESS_GET_PROP_INFO_REQ = 21,
	BUSINESS_GET_PROP_INFO_RSP = 22,

	BUSINESS_PROP_CHARGE_REQ = 23,
	BUSINESS_PROP_CHARGE_RSP = 24,

	BUSINESS_GET_USERNAME_BY_IMEI_REQ = 25,
	BUSINESS_GET_USERNAME_BY_IMEI_RSP = 26,

	MANAGE_MODIFY_PERSON_STATUS_REQ = 27,
	MANAGE_MODIFY_PERSON_STATUS_RSP = 28,

	MANAGE_MODIFY_PASSWORD_REQ = 29,
	MANAGE_MODIFY_PASSWORD_RSP = 30,

	MANAGE_MODIFY_BASEINFO_REQ = 31,
	MANAGE_MODIFY_BASEINFO_RSP = 32,

	BUSINESS_GET_USERNAME_BY_PLATFROM_REQ = 33,
	BUSINESS_GET_USERNAME_BY_PLATFROM_RSP = 34,

	BUSINESS_USER_RECHARGE_REQ = 35,
	BUSINESS_USER_RECHARGE_RSP = 36,

	BUSINESS_EXCHANGE_PHONE_CARD_REQ = 37,
	BUSINESS_EXCHANGE_PHONE_CARD_RSP = 38,

	BUSINESS_EXCHANGE_GOODS_REQ = 39,
	BUSINESS_EXCHANGE_GOODS_RSP = 40,
	
	BUSINESS_RESET_PASSWORD_REQ = 41,
	BUSINESS_RESET_PASSWORD_RSP = 42,

	BUSINESS_MODIFY_EXCHANGE_GOODS_STATUS_REQ = 43,
	BUSINESS_MODIFY_EXCHANGE_GOODS_STATUS_RSP = 44,
	
	// 查询用户ID username
	BUSINESS_QUERY_USER_NAMES_REQ = 45,
	BUSINESS_QUERY_USER_NAMES_RSP = 46,
	
	// 设置目标用户的任意信息
	BUSINESS_RESET_USER_OTHER_INFO_REQ = 63,
	BUSINESS_RESET_USER_OTHER_INFO_RSP = 64,
};

// 捕鱼游戏 ServiceType::OutsideClientSrv = 31
enum FishClient
{
	CLIENT_ENTER_ROOM_VERIFY_REQ = 1,
	CLIENT_ENTER_ROOM_VERIFY_RSP = 2,

	CLIENT_BUYU_ENTER_TABLE_REQ = 3,
	CLIENT_BUYU_ENTER_TABLE_RSP = 4,
	CLIENT_BUYU_ENTER_TABLE_NOTIFY = 5,

	CLIENT_BUYU_SET_BULLET_RATE_REQ = 6,
	
	CLIENT_BUYU_QUIT_REQ = 15,
	CLIENT_BUYU_QUIT_RSP = 16,
	CLIENT_BUYU_QUIT_NOTIFY = 17,

    CLIENT_TABLE_CHAT_REQ = 35,
	CLIENT_TABLE_CHAT_RSP = 36,
	CLIENT_TABLE_CHAT_NOTIFY = 37,
	
	CLIENT_BUYU_USE_PROP_REQ = 41,
	CLIENT_BUYU_USE_PROP_RSP = 42,
	CLIENT_BUYU_USE_PROP_NOTIFY = 43,
};


// 等待登陆的机器人数据
struct RobotLoginNotifyData
{
	RobotLoginNotifyData(unsigned int _robot_type, unsigned int _service_id, unsigned int _table_id, unsigned int _seat_id, unsigned int _min_gold, unsigned int _max_gold):
	robot_type(_robot_type), service_id(_service_id), table_id(_table_id), seat_id(_seat_id), min_gold(_min_gold), max_gold(_max_gold)
	{
	}
	
	unsigned int robot_type;
	unsigned int service_id;
	unsigned int table_id;
	unsigned int seat_id;
	unsigned int min_gold;
	unsigned int max_gold;
};
typedef vector<RobotLoginNotifyData> RobotLoginNotifyDataVector;

// 机器人id数组
typedef vector<string> RobotUserNameVector;

// 机器人同桌的其他用户
typedef vector<string> TableUsers;

// 机器人状态
enum RobotStatus
{
	RobotPlayGame = 0,         // 游戏中

	RobotLogin = 1,            // 连接建立登陆中。。。
	RobotClearGold = 2,        // 清零金币
	RobotResetProperty = 3,    // 重置属性值
	RobotEnterRoom = 4,        // 登陆房间中
	RobotEnterTable = 5,       // 进入桌子中
	RobotRelease = 6,          // 等到回收中
};

// 机器人数据
struct RobotData
{
	char id[MaxUserIdLen];
	unsigned int robot_type;
	unsigned int service_id;
	unsigned int table_id;
	unsigned int seat_id;
	unsigned int min_gold;
	unsigned int max_gold;
	unsigned int bullet_level;  // 子弹等级
	int status;
	
	unsigned int change_bullet_rate_timer_id;
	unsigned int use_prop_timer_id;
	unsigned int chat_timer_id;
	
	TableUsers* tableUsers;
};

// 操作机器人数据
enum ERobotDataOpt
{
	ESetVIPLevel = 1,           // 设置VIP等级
};

}

#endif // TYPE_DEFINE_H
