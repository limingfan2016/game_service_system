
/* author : liuxu
 * date : 2015.03.03
 * description : 消息定义文件
 */
 
#ifndef __MESSAGE_DEFINE_H__
#define __MESSAGE_DEFINE_H__

#include <string>
#include <vector>
#include <unordered_map>
#include "app_server_dbproxy_common.pb.h"
#include "../common/ServiceProtocolId.h"


using namespace std;


/********************************************************************************************/
//服务错误码 3001 - 3999
enum EServiceErrorCode
{
	ServiceFailed = -1,						//操作失败
	ServiceSuccess = 0,						//成功
	ServiceUsernameExist = 3001,			//用户名已存在
	ServiceGetIDFailed = 3002,				//获取自增ID失败
	ServiceInsertFailed = 3003,				//插入用户数据到DB失败
	ServiceUsernameInputUnlegal = 3004,		//用户名输入不合法（用户名要求：username.length() <= 32 && username.length() > 5 && strIsAlnum(username.c_str())）
	ServicePasswdInputUnlegal = 3005,		//密码不合法（要求发送加了MD5后的密码，并转换成大写）
	ServiceNicknameInputUnlegal = 3006,		//昵称不合法（要求长度小于等于32）
	ServicePersonSignInputUnlegal = 3007,	//个性签名不合法（要求：person_sign.length() <= 64）
	ServiceRealnameInputUnlegal = 3008,		//真实姓名不合法（要求：realname.length() <= 16）
	ServiceQQnumInputUnlegal = 3009,		//qq号不合法（要求：strIsDigit(qq_num.c_str()) && qq_num.length() <= 16）
	ServiceAddrInputUnlegal = 3010,			//地址不合法（要求：address.length() < = 64）
	ServiceBirthdayInputUnlegal = 3011,		//生日不合法（要求：strIsDate(birthday.c_str())）
	ServiceHomephoneInputUnlegal = 3012,	//家庭电话不合法（要求：home_phone.length() <= 16）
	ServiceMobilephoneInputUnlegal = 3013,	//手机号码不合法（要求：(mobile_phone.length() <= 16 && strIsAlnum(mobile_phone.c_str())）
	ServiceEmailInputUnlegal = 3014,		//email不合法（要求：email.length() <= 36）
	ServiceIDCInputUnlegal = 3015,			//身份证号码不合法（要求：idc.length() <= 24）
	ServiceOtherContactUnlegal = 3016,		//其它联系方式不合法（要求：other_contact.length() <= 64）
	ServiceGetUserinfoFailed = 3017,		//查找用户信息失败
	ServicePasswordVerifyFailed = 3018,		//密码校验失败
	ServiceUpdateDataToMysqlFailed = 3019,	//更新数据到mysql失败
	ServiceUpdateDataToMemcachedFailed = 3020,	//更新数据到memcached失败
	ServiceLightCannonNotEnought = 3021,	//激光炮不足
	ServiceMuteBulletNotEnought = 3022,		//哑弹不足
	ServiceFlowerNotEnought = 3023,			//鲜花不足
	ServiceSlipperNotEnought = 3024,		//拖鞋不足
	ServiceSuonaNotEnought = 3025,			//喇叭不足
	ServiceIMEIUnlegal = 3026,				//IMEI不合法
	ServiceGetMaxIdFailed = 3027,			//sql获取最大id失败
	ServiceUserLimitLogin = 3028,			//帐号已被限制登陆（封号）
	ServicePlatformIdUnlegal = 3029,		//PLATFORM_ID不合法
	ServiceItemNotExist = 3030,				//物品ID不存在
	ServicePriceNotMatching = 3031,		    //价格不匹配
	ServiceUnknownItemID = 3032,			//未知物品ID
	ServicePhoneCardNotEnought = 3033,		//话费卡不足
	ServiceVoucherNotEnought = 3034,		//兑换券不足
	ServiceEmailBoxInvalid = 3035,		    //email匹配错误
	ServicePropTypeInvalid = 3036,		    //道具类型错误
	ServicePropNotEnought = 3037,		    //道具不足
	ServiceFishCoinNotEnought = 3038,		//渔币不足
	ServiceDiamondsNotEnought = 3039,		//钻石不足
	ServiceExchangeTypeInvalid = 3040,		//渔币兑换的物品类型无效
	ServiceGameGoldNotEnought = 3041,		//游戏金币不足
	ExchangeMobilePhoneNotMatch = 3042,		//兑换提交的手机号码不匹配
	ServicePhoneFareNotEnought = 3043,		//话费额度不足
	SaveExchangeMobilePhoneError = 3044,	//保存兑换话费的手机号码出错
	ExistExchangeMobilePhoneError = 3045,	//该手机号码已经绑定过了
	GameCommonConfigTypeError = 3046,	    //公共配置类型错误
	NonExistentFirstPackage = 3047,         // 已经充值了，没有首冲礼包了
	ServiceExchangeCeiling = 3048,			// 达到兑换上限
	ServiceScoresNotEnough = 3049,			// 积分不足
	ServiceNotFindExchangeItem = 3050,		// 没有找到需要兑换的物品
	ServiceNicknameSensitiveStr = 3051,		// 昵称含有敏感字符
	ServiceUserDataNotExist = 3052,			// 用户数据不存在
	ServiceParseFromArrayError = 3053,      // 解码错误
	ServiceSerializeToArrayError = 3054,    // 编码错误
	ServiceCreateRedGiftFailed = 3055,      // 创建红包口令失败
	ServiceDeviceReceivedRedGift = 3056,    // 该设备已经领取过红包了
	ServiceUserReceivedRedGift = 3057,      // 该用户已经领取过红包了
	ServiceRedGiftTimeError = 3058,         // 红包口令时间错误
	ServiceNoFoundRedGiftError = 3059,      // 找不到该红包口令
	ServiceReceiveRedGiftError = 3060,      // 领取红包口令错误
	ServiceNoFindPKGoldTicketId = 3061,     // 找不到PK场金币门票ID
	ServiceIPLimitLogin = 3062,			    // 该IP已被限制登陆
	ServiceDeviceLimitLogin = 3063,			// 该设备已被限制登陆
	ServiceAlreadyBindMobilePhone = 3064,	// 已经绑定过手机号码了
	ServiceBindMobilePhoneFailed = 3065,	// 绑定手机号码失败
	ExistBindMobilePhoneError = 3066,       // 该手机号码已经绑定过了
	ServiceNotFoundMailMessage = 3067,      // 找不到相应的消息
	AlreadyReceivedMessageReward = 3068,    // 该消息的奖励已经领取过了
	ServiceSetLogicDataError = 3069,        // 设置逻辑数据错误
	InsertFFChessUserInfoError = 3070,      // 添加翻翻棋用户信息错误
	FFChessUserNotExistence = 3071,         // 翻翻棋用户不存在
	UpdateFFChessUserInfoError = 3072,      // 刷新翻翻棋用户信息错误
	CheckWinRedPacketActivityError = 3073,  // 检查抢红包活动信息错误
	CheckRedPacketDataError = 3074,         // 检查红包信息错误
	WinRedPacketPaymentTypeError = 3075,    // 无效的抢红包支付物品类型
	WinRedPacketPayCountInsufficient = 3076,    // 抢红包支付物品的数量不足
	HanYouCouponNotFound = 3077,            // 找不到HanYou平台优惠券
};

//提供的服务

//ServiceType::CommonSrv = 0
enum CommonSrvBusiness{
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
	
	// 查询用户信息
	BUSINESS_QUERY_USER_INFO_REQ = 47,
	BUSINESS_QUERY_USER_INFO_RSP = 48,
	
	// 道具变更新接口
	BUSINESS_CHANGE_PROP_REQ = 49,
	BUSINESS_CHANGE_PROP_RSP = 50,
	
	// 获取游戏商城配置
	BUSINESS_GET_GAME_MALL_CONFIG_REQ = 51,
	BUSINESS_GET_GAME_MALL_CONFIG_RSP = 52,

	// 兑换游戏商品
	BUSINESS_EXCHANGE_GAME_GOODS_REQ = 53,
	BUSINESS_EXCHANGE_GAME_GOODS_RSP = 54,
	
	// 领取超值礼包物品
	BUSINESS_RECEIVE_SUPER_VALUE_PACKAGE_REQ = 55,
	BUSINESS_RECEIVE_SUPER_VALUE_PACKAGE_RSP = 56,
	
	// 获取目标用户的任意信息
	BUSINESS_GET_USER_OTHER_INFO_REQ = 57,
	BUSINESS_GET_USER_OTHER_INFO_RSP = 58,
	
	// 获取实物兑换配置信息
	BUSINESS_GET_GOODS_EXCHANGE_CONFIG_REQ = 59,
	BUSINESS_GET_GOODS_EXCHANGE_CONFIG_RSP = 60,
	
	// 兑换话费额度
	BUSINESS_EXCHANGE_PHONE_FARE_REQ = 61,
	BUSINESS_EXCHANGE_PHONE_FARE_RSP = 62,
	
	// 设置目标用户的任意信息
	BUSINESS_RESET_USER_OTHER_INFO_REQ = 63,
	BUSINESS_RESET_USER_OTHER_INFO_RSP = 64,
	
	// 获取公共配置信息
	BUSINESS_GET_GAME_COMMON_CONFIG_REQ = 65,
	BUSINESS_GET_GAME_COMMON_CONFIG_RSP = 66,
	
	// 获取首冲礼包信息
	BUSINESS_GET_FIRST_RECHARGE_PACKAGE_REQ = 67,
	BUSINESS_GET_FIRST_RECHARGE_PACKAGE_RSP = 68,
	
	// 获取商城渔币信息
	BUSINESS_GET_MALL_FISH_COIN_INFO_REQ = 69,
	BUSINESS_GET_MALL_FISH_COIN_INFO_RSP = 70,

	// 获取积分商城信息
	BUSINESS_GET_MALL_SCORES_INFO_REQ = 71,
	BUSINESS_GET_MALL_SCORES_INFO_RSP = 72,

	//兑换积分商品
	BUSINESS_EXCHANGE_SCORES_ITEM_REQ = 73,
	BUSINESS_EXCHANGE_SCORES_ITEM_RSP = 74,

	//聊天记录
	BUSINESS_USER_CHAT_LOG_REQ = 75,

	// 获取目标多个用户的任意信息
	BUSINESS_GET_MANY_USER_OTHER_INFO_REQ = 76,
	BUSINESS_GET_MANY_USER_OTHER_INFO_RSP = 77,

	//查询用户道具以及防作弊列表信息
	BUSINESS_GET_USER_PROP_AND_PREVENTION_CHEAT_REQ = 78,
	BUSINESS_GET_USER_PROP_AND_PREVENTION_CHEAT_RSP = 79,

	//修改多用户道具
	BUSINESS_CHANGE_MANY_PEOPLE_PROP_REQ = 80,
	BUSINESS_CHANGE_MANY_PEOPLE_PROP_RSP = 81,

	//PK场结算请求
	BUSINESS_PK_PLAY_SETTLEMENT_OF_ACCOUNTS_REQ = 82,
	BUSINESS_PK_PLAY_SETTLEMENT_OF_ACCOUNTS_RSP = 83,

	//游戏记录
	BUSINESS_GAME_RECORD_NOTIFY = 84,

	//获取 PK场排行榜信息
	BUSINESS_GET_PK_PLAY_RANKING_LIST_REQ = 85,
	BUSINESS_GET_PK_PLAY_RANKING_LIST_RSP = 86,

	//获取PK场名人堂信息
	BUSINESS_PK_PLAY_CELEBRITY_INFO_REQ = 87,
	BUSINESS_PK_PLAY_CELEBRITY_INFO_RSP = 88,

	// 添加红包口令
	BUSINESS_ADD_RED_GIFT_REQ = 89,
	BUSINESS_ADD_RED_GIFT_RSP = 90,

	// 领取红包口令
	BUSINESS_RECEIVE_RED_GIFT_REQ = 91,
	BUSINESS_RECEIVE_RED_GIFT_RSP = 92,
	
	// PK场玩家匹配记录
	BUSINESS_PLAYER_MATCHING_OPT_NOTIFY = 93,
	
	// 绑定兑换商城账号信息
	BUSINESS_BINDING_EXCHANGE_MALL_INFO_REQ = 94,
	BUSINESS_BINDING_EXCHANGE_MALL_INFO_RSP = 95,
	
	// 添加邮件信息
	BUSINESS_ADD_MAIL_MESSAGE_REQ = 96,
	BUSINESS_ADD_MAIL_MESSAGE_RSP = 97,
	
	// 获取邮件信息
	BUSINESS_GET_MAIL_MESSAGE_REQ = 98,
	BUSINESS_GET_MAIL_MESSAGE_RSP = 99,
	
	// 操作邮件信息
	BUSINESS_OPT_MAIL_MESSAGE_REQ = 100,
	BUSINESS_OPT_MAIL_MESSAGE_RSP = 101,
	
	// 设置服务逻辑数据
	BUSINESS_SET_SERVICE_LOGIC_DATA_REQ = 102,
	BUSINESS_SET_SERVICE_LOGIC_DATA_RSP = 103,
	
	// 获取翻翻棋道具商城数据
	BUSINESS_GET_FF_CHESS_MALL_REQ = 104,
	BUSINESS_GET_FF_CHESS_MALL_RSP = 105,
	
	// 兑换翻翻棋道具
	BUSINESS_EXCHANGE_FF_CHESS_GOODS_REQ = 106,
	BUSINESS_EXCHANGE_FF_CHESS_GOODS_RSP = 107,
	
	// 使用翻翻棋道具
	BUSINESS_USE_FF_CHESS_GOODS_REQ = 108,
	BUSINESS_USE_FF_CHESS_GOODS_RSP = 109,

	// 获取玩家财富相关信息
	BUSINESS_GET_USER_WEALTH_INFO_REQ = 110,
	BUSINESS_GET_USER_WEALTH_INFO_RSP = 111,
	
	// 重置玩家的兑换绑定手机号码
	BUSINESS_RESET_EXCHANGE_PHONE_NUMBER_REQ = 112,
	BUSINESS_RESET_EXCHANGE_PHONE_NUMBER_RSP = 113,
	
	// 新添加翻翻棋用户基本信息
	BUSINESS_ADD_FF_CHESS_USER_BASE_INFO_REQ = 114,
	BUSINESS_ADD_FF_CHESS_USER_BASE_INFO_RSP = 115,

	// 获取翻翻棋用户基本信息
	BUSINESS_GET_FF_CHESS_USER_BASE_INFO_REQ = 116,
	BUSINESS_GET_FF_CHESS_USER_BASE_INFO_RSP = 117,
	
	// 设置翻翻棋用户对局结果信息
	BUSINESS_SET_FF_CHESS_MATCH_RESULT_REQ = 118,
	BUSINESS_SET_FF_CHESS_MATCH_RESULT_RSP = 119,
	
	// 新增加红包信息
	BUSINESS_ADD_ACTIVITY_RED_PACKET_REQ = 120,
	BUSINESS_ADD_ACTIVITY_RED_PACKET_RSP = 121,
	
	// 获取活动红包数据
	BUSINESS_GET_ACTIVITY_RED_PACKET_REQ = 122,
	BUSINESS_GET_ACTIVITY_RED_PACKET_RSP = 123,
	
	// 韩文版HanYou平台玩家使用优惠券通知
	BUSINESS_USE_HAN_YOU_COUPON_NOTIFY = 124,
	
	// 韩文版HanYou平台获取优惠券奖励
	BUSINESS_ADD_HAN_YOU_COUPON_REWARD_REQ = 125,
	BUSINESS_ADD_HAN_YOU_COUPON_REWARD_RSP = 126,
	
	// 自动积分兑换物品
	BUSINESS_AUTO_SCORE_EXCHANGE_ITEM_REQ = 127,
	BUSINESS_AUTO_SCORE_EXCHANGE_ITEM_RSP = 128,
};


//ServiceType::MessageSrv = 6
enum MessageBusiness
{
	BUSINESS_PRIVATE_CHAT_REQ = 1,
	BUSINESS_PRIVATE_CHAT_RSP = 2,
	BUSINESS_PRIVATE_CHAT_NOTIFY = 3,

	BUSINESS_WORLD_CHAT_REQ = 4,
	BUSINESS_WORLD_CHAT_RSP = 5,
	BUSINESS_WORLD_CHAT_NOTIFY = 6,

	BUSINESS_CHAT_FILTER_REQ = 7,
	BUSINESS_CHAT_FILTER_RSP = 8,

	BUSINESS_GET_USER_SERVICE_ID_REQ = 9,
	BUSINESS_GET_USER_SERVICE_ID_RSP = 10,

	BUSINESS_USER_ENTER_SERVICE_NOTIFY = 11,
	BUSINESS_USER_QUIT_SERVICE_NOTIFY = 12,

	BUSINESS_SYSTEM_MESSAGE_NOTIFY = 13,
	BUSINESS_SERVICE_ONLINE_NOTIFY = 14,

	BUSINESS_STOP_GAME_SERVICE_NOTIFY = 15,
};
//////////////////////////////////////////////////////////////////////////////////////////////


class CUserModifyStat
{
public:
	uint32_t modify_count;
	uint32_t first_modify_timestamp;
	bool is_logout;
};

static const unsigned int DBStringFieldLength = 33;

#pragma pack(1)
class CUserStaticBaseinfo
{
public:
	uint32_t id;
	char register_time[24];
	char username[DBStringFieldLength];
	char password[DBStringFieldLength];
	char nickname[DBStringFieldLength];
	char person_sign[65];
	char realname[17];
	uint32_t portrait_id;
	char qq_num[17];
	char address[65];
	char birthday[12];
	int32_t sex;
	char home_phone[17];
	char mobile_phone[17];
	char email[37];
	uint32_t age;
	char idc[25];
	char other_contact[65];
	uint32_t city_id;
	uint32_t status;
};

class CUserDynamicBaseinfo
{
public:
	uint32_t id;
	char username[DBStringFieldLength];
	uint32_t level;
	uint32_t cur_level_exp;
	uint32_t vip_level;
	uint32_t vip_cur_level_exp;
	char vip_time_limit[24];
	uint32_t score;
	uint64_t game_gold;                   // 游戏金币数量
	uint32_t rmb_gold;                    // 渔币数量
	uint32_t diamonds_number;             // 钻石数量
	uint32_t phone_fare;                  // 话费额度
	char mobile_phone_number[DBStringFieldLength];         // 兑换话费绑定的手机号码
	uint32_t voucher_number;              // 奖券数量
	uint32_t phone_card_number;
	uint32_t sum_recharge;                // 充值总额度RMB单位元
	uint32_t sum_recharge_count;
	char last_login_time[24];
	char last_logout_time[24];
	uint32_t sum_online_time;
	uint32_t sum_login_times;
};

class CUserPropInfo
{
public:
	uint32_t id;
	char username[33];
	uint32_t light_cannon_count;
	uint32_t mute_bullet_count;
	uint32_t flower_count;
	uint32_t slipper_count;
	uint32_t suona_count;
	
	uint32_t auto_bullet_count;      // 自动炮子弹道具
	uint32_t lock_bullet_count;      // 锁定炮子弹道具
	uint32_t rampage_count;		     // 狂暴道具
	
	uint32_t dud_shield_count;		 // 哑弹防护道具
};

class CUserBaseinfo
{
public:
	CUserStaticBaseinfo static_info;
	CUserDynamicBaseinfo dynamic_info;
	CUserPropInfo prop_info;
};


// 翻翻棋用户基本信息
struct SFFChessUserBaseinfo
{
	char username[DBStringFieldLength];
	uint32_t max_rate;
	uint32_t max_continue_win;
	uint32_t current_continue_win;
	uint32_t all_times;
	uint32_t win_times;
	uint64_t win_gold;
	uint64_t lost_gold;
};

#pragma pack()


// 用户信息&游戏记录
struct UserBaseInfoRecord
{
	CUserBaseinfo baseInfo;
	const com_protocol::GameRecordPkg* record;
};
typedef vector<UserBaseInfoRecord> UserBaseInfoRecordVector;


// 充值标识
enum RechargeFlag
{
	First = 1,            // 首次充值
};

// 服务逻辑数据
struct ServiceLogicData
{
	com_protocol::DBCommonSrvLogicData logicData;
	int isUpdate;
};

typedef unordered_map<string, ServiceLogicData> UserToLogicData;


// PK场金币门票信息
struct PKGoldTicket
{
	unsigned int id;
	unsigned int day;
	unsigned int beginHour;
	unsigned int beginMin;
	unsigned int endHour;
	unsigned int endMin;
	int count;
	bool isFullDay;
};
typedef vector<PKGoldTicket> PKGoldTicketVector;

// 金币门票排序信息
struct PKGoldTicketSortInfo
{
	unsigned int index;
	unsigned int endSecs;
	
	PKGoldTicketSortInfo() : index(0), endSecs(0) {};
	PKGoldTicketSortInfo(unsigned int _idx, unsigned int _endSecs) : index(_idx), endSecs(_endSecs) {};
};
typedef vector<PKGoldTicketSortInfo> PKGoldTicketSortInfoVector;


#endif // __MESSAGE_DEFINE_H__

