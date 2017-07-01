
/* author : limingfan
 * date : 2015.04.23
 * description : 类型定义
 */

#ifndef TYPE_DEFINE_H
#define TYPE_DEFINE_H

#include "common/ServiceProtocolId.h"
#include "SrvFrame/UserType.h"
#include "login_server.pb.h"


namespace NPlatformService
{

static const unsigned int LogicDataMaxLen = 1024 * 32;
static const int MaxConnectIdLen = 36;
static const unsigned int MaxUserDataCount = 128;

// 道具标识
enum EPropFlag
{
	VIPPropReward = 1,		        // vip奖励
	VIPUpLv = 2,			        // VIP等级变更
	ReceiveNewbieGift = 3,          // 领取新手礼包
	RechargeLotteryGift = 4,        // 充值抽奖礼包
	ShareReward = 5,				// 分享奖励
	DeductPkPlayEntranceFee = 6,	// 扣除PK场入场费
	PKPlayRankingListReward = 7,	// PK排行榜奖励
	RedGiftReward = 8,	            // 红包口令奖励
	JoinMatchReward = 9,	        // 对局任务奖励
	WinMatchReward = 10,	        // 赢局任务奖励
	OnlineMatchReward = 11,	        // PK在线任务奖励
	PKRankingListRecord = 12,		// PK排行榜奖励记录
	EnterCatchGiftActivity = 13,    // 参加福袋活动入场费
	CatchGiftActivitySettlement = 14,    // 福袋活动结算奖励物品
};

//获取用户信息
enum GetUserInfoFlag
{
	GetUserVIPInfo = 1,						//获取用户VIP等级
	GetUserVIPLvForReward = 2,				//获取用户VIP等级用于领取奖励
	GetUserVIPLvForVIPUp = 3,				//获取用户VIP等级用于VIP升级
	GetUserAllPropToPKPlayInfoReq = 4,		//取用户所有道具用于PK场信息请求
	GetUserInfoToPKPlaySignUp = 5,			//获取用户信息用于PK场报名
};

//获取多用户信息
enum GetManyUserInfoFlag
{
	GetManyUserInfoStartPkPlay = 1,			//获取多用户信息 用于开始PK场游戏
};

// 协议操作返回码
enum ProtocolReturnCode
{
	Opt_Failed = -1,
	Opt_Success = 0,                       // 操作成功
	SetSessionKeyFailed = 1001,            // 设置 session key 到redis服务失败
	NotLoginSuccessError = 1002,           // 用户没有成功登录操作
	GetLogicDataFailed = 1003,             // 获取逻辑数据失败
	NoLoginRewardReceive = 1004,           // 没有登陆奖励可领取
	ReceivedLoginReward = 1005,            // 已经领取了登陆奖励
	ClientVersionConfigError = 1006,       // 客户端版本信息配置错误
	ClientVersionInvalid = 1007,           // 不可用的客户端版本号
	GetPriceFromMallDataError = 1008,      // 从商城数据获取物品价格错误
	NotNoGoldFreeGive = 1009,              // 没有免费的破产补助可领取了
	NoGoldFreeGiveNoReceiveTime = 1010,    // 破产补助还没到领取时间点
	AlreadyGetedVIPReward = 1011,		   // 已经领取过VIP奖励
	NoVIPUser = 1012,					   // 不是VIP用户
	ClientHotUpdateCfgError = 1013,        // 客户端热更新版本信息配置错误
	ClientHotUpdateVersionInvalid = 1014,           // 客户端热更新版本无效
	NoNewbieGift = 1015,                            // 没有新手礼包
	AlreadyReceivedNewbieGift = 1016,               // 已经领取过新手礼包了
	NoRechargeLotteryGift = 1017,                   // 没有充值抽奖次数了
	RechargeLotteryActivityFinish = 1018,           // 充值抽奖已经结束了
	UserAlreadySignUp = 1019,						// 玩家已报名
	PKPlayAlreadyCloser = 1020,						// 场次没开启
	NotFindSignUpPKPlay = 1021,						// 没有找到要报名的PK场
	SignUpTypeError = 1022,							// 报名类型错误
	SignUpLvNotEnough = 1023,						// 报名需要的用户等级不够
	SignUpConditionNotSatisfied = 1024,				// 报名条件不满足
	NotFindCancelSignUpPKPlay = 1025,				// 没有找到要取消的PK场
	UserPlayingNotCancel = 1026,					// 玩家游戏中不能取消
	MatchingOvertime = 1027,						// 匹配超时
	UserPlaying = 1028,								// 游戏中
	NotFindMatchingTable = 1029,					// 没有找到匹配的桌子
	GoldInsufficient = 1030,						// 金币不足
	FishCoinInsufficient = 1031,					// 渔币不足
	PhoneFareValueInsufficient = 1032,				// 话费不足
	ScoresInsufficient = 1033,						// 积分不足
	DiamondsInsufficient = 1034,					// 钻石不足
	VoucherInsufficient = 1035,						// 奖券不足
	PlayerQuitGameReMatch = 1036,					// 玩家退出游戏, 将重新进行匹配
	NotFindUserInfo = 1037,							// 没找到该用户信息
	AlreadyShare = 1038,							// 已分享过
	PkPlayEnd = 1039,								// PK场游戏结束
	PKPlayRankingListNoneReward = 1040,				// 没有PK排行榜奖励
	ReceiveRedGift = 1041,				            // 已经领取过红包了
	WinRedPacketActivityCfgError = 1042,            // 抢红包活动配置错误
	NotFindWinRedPacketActivity = 1043,		        // 找不到对应的抢红包活动信息
	WinRedPacketActivityNotBegin = 1044,		    // 抢红包活动还没开始
	WinRedPacketActivityAlreadyEnd = 1045,		    // 抢红包活动已经结束了
	WinRedPacketActivityFinish = 1046,		        // 手慢，红包被抢完了
	WinRedPacketActivityOnReceive = 1047,	        // 正在抢红包中
	InvalidCatchGiftActivityTime = 1048,            // 无效的福袋活动时间
	CatchGiftActivityGoldInsufficient = 1049,       // 参加福袋活动入场费金币不足
	CatchGiftActivityConfigError = 1050,            // 福袋活动配置信息错误
	EnterCatchGiftActivityError = 1051,             // 进入福袋活动错误
	CheckFacebookUserError = 1052,                  // 验证Facebook用户错误
};

// common公共协议ID值
enum CommonProtocol
{
	// 游戏服务退出登陆消息
	GAME_SERVICE_LOGOUT_NOTIFY_REQ = 1,
	GAME_SERVICE_LOGOUT_NOTIFY_RSP = 2,
	
	ADD_FRIEND_REQ = 3,
	ADD_FRIEND_RSP = 4,
	REMOVE_FRIEND_REQ = 5,
	REMOVE_FRIEND_RSP = 6,
	
	// 破产补助免费赠送金币
	NO_GOLD_FREE_GIVE_REQ = 7,
	NO_GOLD_FREE_GIVE_RSP = 8,
	GET_NO_GOLD_FREE_GIVE_REQ = 9,
	GET_NO_GOLD_FREE_GIVE_RSP = 10,

	// 游戏中VIP等级变更
	GAME_USER_VIP_LV_UPDATE_NOTIFY = 11,
	
	// 游戏转发消息通知
	GAME_FORWARD_MESSAGE_TO_HALL_NOTIFY = 12,

	// 游戏增加道具通知
	GAME_ADD_PROP_NOTIFY = 13,

	// PK场游戏结束
	LOGINSRV_PK_PLAY_END_NOTIFY = 14,

	// 游戏中生成用户分享ID请求
	LOGINSRV_PLAY_GENERATE_SHARE_ID_REQ = 15,

	LOGINSRV_PLAY_SHARE_REQ = 16,					// 游戏分享请求
	LOGINSRV_PLAY_SHARE_RSP = 17,					// 游戏分享应答

	//获取PK场配置信息
	GET_PK_PLAY_CFG_REQ = 105,
	GET_PK_PLAY_CFG_RSP = 106,

	//PK场主动引导通知
	LOGINSRV_PK_PLAY_ACTIVE_GUIDE_NOTIFY = 107,
	
	// 获取比赛场排名奖励
	COMMON_GET_ARENA_RANKING_REWARD_REQ = 108,
	COMMON_GET_ARENA_RANKING_REWARD_RSP = 109,
	
	// 获取比赛场玩家信息
	COMMON_GET_ARENA_MATCH_USER_INFO_REQ = 110,
	COMMON_GET_ARENA_MATCH_USER_INFO_RSP = 111,
	
	// 通知比赛服务用户主动取消比赛了
	COMMON_CANCEL_ARENA_MATCH_NOTIFY = 112,
	
	// 通知比赛服务更新比赛场配置信息
	COMMON_ARENA_MATCH_CONFIG_INFO_NOTIFY = 113,
	
	// 获取捕鱼卡牌服务信息
	COMMON_GET_POLER_CARK_SERVICE_INFO_REQ = 114,
	COMMON_GET_POLER_CARK_SERVICE_INFO_RSP = 115,
	
	// 韩文版HanYou平台使用优惠券获得的奖励通知
	COMMON_ADD_HAN_YOU_COUPON_REWARD_NOTIFY = 116,
};

// Login服务定义的协议ID值
enum LoginSrvProtocol
{
	// Login 服务与外部客户端之间的协议
    LOGINSRV_REGISTER_USER_REQ = 1,
	LOGINSRV_REGISTER_USER_RSP = 2,
	LOGINSRV_MODIFY_PASSWORD_REQ = 3,
	LOGINSRV_MODIFY_PASSWORD_RSP = 4,
	LOGINSRV_MODIFY_BASEINFO_REQ = 5,
	LOGINSRV_MODIFY_BASEINFO_RSP = 6,
	LOGINSRV_LOGIN_REQ = 7,
	LOGINSRV_LOGIN_RSP = 8,
	LOGINSRV_LOGOUT_REQ = 9,
	LOGINSRV_LOGOUT_RSP = 10,
	LOGINSRV_GET_USER_INFO_REQ = 11,
	LOGINSRV_GET_USER_INFO_RSP = 12,
	LOGINSRV_GET_HALL_INFO_REQ = 13,
	LOGINSRV_GET_HALL_INFO_RSP = 14,
	
	// 登陆签到奖励
	LOGINSRV_GET_LOGIN_REWARD_LIST_REQ = 15,
	LOGINSRV_GET_LOGIN_REWARD_LIST_RSP = 16,
	LOGINSRV_RECEIVE_LOGIN_REWARD_REQ = 17,
	LOGINSRV_RECEIVE_LOGIN_REWARD_RSP = 18,
	
	// 好友操作
	LOGINSRV_GET_FRIEND_DYNAMIC_DATA_REQ = 19,
	LOGINSRV_GET_FRIEND_DYNAMIC_DATA_RSP = 20,
	LOGINSRV_GET_FRIEND_STATIC_DATA_REQ = 21,
	LOGINSRV_GET_FRIEND_STATIC_DATA_BYID_REQ = 22,
	LOGINSRV_GET_FRIEND_STATIC_DATA_RSP = 23,
	LOGINSRV_ADD_FRIEND_NOTIFY = 24,

	LOGINSRV_REMOVE_FRIEND_REQ = 25,
	LOGINSRV_REMOVE_FRIEND_RSP = 26,
	
	// 聊天
	LOGINSRV_PRIVATE_CHAT_REQ = 27,
	LOGINSRV_PRIVATE_CHAT_RSP = 28,
	LOGINSRV_PRIVATE_CHAT_NOTIFY = 29,

	LOGINSRV_WORLD_CHAT_REQ = 30,
	LOGINSRV_WORLD_CHAT_RSP = 31,
	LOGINSRV_WORLD_CHAT_NOTIFY = 32,
	
	LOGINSRV_GET_MALL_CONFIG_REQ = 33,
	LOGINSRV_GET_MALL_CONFIG_RSP = 34,
	
	LOGINSRV_CHECK_CLIENT_VERSION_REQ = 35,
	LOGINSRV_CHECK_CLIENT_VERSION_RSP = 36,
	
	// 获取账号
	LOGINSRV_GET_USERNAME_BY_IMEI_REQ = 37,
	LOGINSRV_GET_USERNAME_BY_IMEI_RSP = 38,
	
	// 当乐平台接口
	LOGINSRV_CHECK_DOWNJOY_USER_REQ = 39,
	LOGINSRV_CHECK_DOWNJOY_USER_RSP = 40,
	LOGINSRV_GET_DOWNJOY_RECHARGE_TRANSACTION_REQ = 41,
	LOGINSRV_GET_DOWNJOY_RECHARGE_TRANSACTION_RSP = 42,
	LOGINSRV_DOWNJOY_RECHARGE_TRANSACTION_NOTIFY = 43,
	LOGINSRV_CANCEL_DOWNJOY_RECHARGE_NOTIFY = 44,
	
	// 好友赠送金币
	LOGINSRV_FRIEND_DONATE_GOLD_REQ = 45,
	LOGINSRV_FRIEND_DONATE_GOLD_RSP = 46,
	LOGINSRV_FRIEND_DONATE_GOLD_NOTIFY = 47,
	
	// 兑换操作
	LOGINSRV_EXCHANGE_PHONE_CARD_REQ = 48,
	LOGINSRV_EXCHANGE_PHONE_CARD_RSP = 49,
	LOGINSRV_EXCHANGE_GOODS_REQ = 50,
	LOGINSRV_EXCHANGE_GOODS_RSP = 51,
	
	// 破产补助免费赠送金币
	LOGINSRV_NO_GOLD_FREE_GIVE_REQ = 52,
	LOGINSRV_NO_GOLD_FREE_GIVE_RSP = 53,
	LOGINSRV_GET_NO_GOLD_FREE_GIVE_REQ = 54,
	LOGINSRV_GET_NO_GOLD_FREE_GIVE_RSP = 55,
	LOGINSRV_ADD_ALLOWANCE_GOLD_RSP = 56,
	
	LOGINSRV_SYSTEM_MESSAGE_NOTIFY = 57,
	
	LOGINSRV_ADD_MAIL_BOX_REQ = 58,
	LOGINSRV_ADD_MAIL_BOX_RSP = 59,
	
	// 用户个人财务变更通知，如：金币变化、道具变化、奖券变化等
	LOGINSRV_PERSON_PROPERTY_NOTIFY = 60,
	
	// 海外版本广告SDK
	LOGINSRV_GET_OVERSEAS_NOTIFY_REQ = 61,
	LOGINSRV_GET_OVERSEAS_NOTIFY_RSP = 62,
	
	// Google SDK 充值操作
	LOGINSRV_GET_GOOGLE_RECHARGE_REQ = 63,
	LOGINSRV_GET_GOOGLE_RECHARGE_RSP = 64,
	LOGINSRV_CHECK_GOOGLE_RECHARGE_REQ = 65,
	LOGINSRV_CHECK_GOOGLE_RECHARGE_RSP = 66,
	
	// 第三方平台操作
	LOGINSRV_THIRD_RECHARGE_TRANSACTION_REQ = 67,
	LOGINSRV_THIRD_RECHARGE_TRANSACTION_RSP = 68,
	LOGINSRV_THIRD_RECHARGE_TRANSACTION_NOTIFY = 69,
	LOGINSRV_CANCEL_THIRD_RECHARGE_NOTIFY = 70,
	
	LOGINSRV_GET_USERNAME_BY_PLATFROM_REQ = 71,
	LOGINSRV_GET_USERNAME_BY_PLATFROM_RSP = 72,
	
	// 小米用户校验
	LOGINSRV_CHECK_XIAOMI_USER_REQ = 73,
	LOGINSRV_CHECK_XIAOMI_USER_RSP = 74,
	
	// 第三方平台用户校验
	LOGINSRV_CHECK_THIRD_USER_REQ = 75,
	LOGINSRV_CHECK_THIRD_USER_RSP = 76,
	
	// 查看排行榜
	LOGINSRV_VIEW_RANKING_LIST_REQ = 77,
	LOGINSRV_GOLD_RANKING_LIST_RSP = 78,
	LOGINSRV_PHONE_RANKING_LIST_RSP = 79,
	
	// 获取鱼类的倍率分值
	LOGINSRV_GET_FISH_RATE_SCORE_REQ = 80,
	LOGINSRV_GET_FISH_RATE_SCORE_RSP = 81,

	//	查询腾讯充值是否成功
	LOGINSRV_FIND_RECHARGE_SUCCESS_REQ = 82,
	LOGINSRV_FIND_RECHARGE_SUCCESS_RSP = 83,
	

	// 获取游戏商城配置
	LOGINSRV_GET_GAME_MALL_CONFIG_REQ = 84,
	LOGINSRV_GET_GAME_MALL_CONFIG_RSP = 85,

	// 兑换游戏商品
	LOGINSRV_EXCHANGE_GAME_GOODS_REQ = 86,
	LOGINSRV_EXCHANGE_GAME_GOODS_RSP = 87,
	
	// 充值渔币
	LOGINSRV_GET_RECHARGE_FISH_COIN_ORDER_REQ = 88,
	LOGINSRV_GET_RECHARGE_FISH_COIN_ORDER_RSP = 89,
	LOGINSRV_RECHARGE_FISH_COIN_NOTIFY = 90,
	LOGINSRV_CANCEL_RECHARGE_FISH_COIN_NOTIFY = 91,

    // 登陆校验
	LOGINSRV_VERIFICATION_REQ = 92,
	LOGINSRV_VERIFICATION_RSP = 93,
	
	// 获取超值礼包信息
	LOGINSRV_GET_SUPER_VALUE_PACKAGE_INFO_REQ = 94,
	LOGINSRV_GET_SUPER_VALUE_PACKAGE_INFO_RSP = 95,
	
	// 领取超值礼包物品
	LOGINSRV_RECEIVE_SUPER_VALUE_PACKAGE_GIFT_REQ = 96,
	LOGINSRV_RECEIVE_SUPER_VALUE_PACKAGE_GIFT_RSP = 97,
	
	// 获取首冲礼包信息
	LOGINSRV_GET_FIRST_PACKAGE_INFO_REQ = 98,
	LOGINSRV_GET_FIRST_PACKAGE_INFO_RSP = 99,
    
	// Login 服务内部之间的协议
	LOGINSRV_REPEAT_LOGIN_NOTIFY = 100,

	LOGINSRV_CLOSE_CONNECT_REQ = 101,
	
	LOGINSRV_GET_OFFLINE_USER_HALL_DATA_RSP = 102,
	LOGINSRV_GET_GAME_LOGIC_DATA_RSP = 103,
	
	LOGINSRV_OVERSEAS_RECHARGE_NOTIFY = 104,  // 海外版本，游戏内充值通知到大厅更新充值时间点
	
	
	// 获取实物兑换信息
	LOGINSRV_GET_EXCHANGE_CONFIG_REQ = 105,
	LOGINSRV_GET_EXCHANGE_CONFIG_RSP = 106,
	
	// 兑换话费额度
	LOGINSRV_EXCHANGE_PHONE_FARE_REQ = 107,
	LOGINSRV_EXCHANGE_PHONE_FARE_RSP = 108,

	//获取用户VIP信息
	LOGINSRV_GET_USER_VIP_INFO_REQ = 109,	
	LOGINSRV_GET_USER_VIP_INFO_RSP = 110,	
	LOGINSRV_VIP_INFO_RSP = 111,

	//领取VIP奖励
	LOGINSRV_GET_VIP_REWARD_REQ = 112,
	LOGINSRV_GET_VIP_REWARD_RSP = 113,

	//可以领取VIP奖励
	LOGINSRV_FIND_VIP_REWARD_NOTIFY = 114,

	//公告
	LOGINSRV_NOTICE_REQ = 115,
	LOGINSRV_NOTICE_RSP = 116,
	LOGINSRV_NEW_NOTICE_NOTIFY = 117,		//新公告通知

	//分享
	LOGINSRV_SHARE_REQ = 118,
	LOGINSRV_SHARE_RSP = 119,
	
	// 版本热更新
	LOGINSRV_VERSION_HOT_UPDATE_REQ = 120,
	LOGINSRV_VERSION_HOT_UPDATE_RSP = 121,
	
	// 获取活动中心列表
	LOGINSRV_GET_ACTIVITY_CENTER_REQ = 122,
	LOGINSRV_GET_ACTIVITY_CENTER_RSP = 123,
	
	// 领取新手礼包
	LOGINSRV_RECEIVE_NEWBIE_GIFT_REQ = 124,
	LOGINSRV_RECEIVE_NEWBIE_GIFT_RSP = 125,
	
	// 充值抽奖
	LOGINSRV_RECHARGE_WIN_GIFT_REQ = 126,
	LOGINSRV_RECHARGE_WIN_GIFT_RSP = 127,

	// 积分商城
	LOGINSRV_GET_SCORES_ITEM_REQ = 128,
	LOGINSRV_GET_SCORES_ITEM_RSP = 129,

	//兑换积分商品
	LOGINSRV_EXCHANGE_SCORES_ITEM_REQ = 130,
	LOGINSRV_EXCHANGE_SCORES_ITEM_RSP = 131,

	//退出挽留
	LOGINSRV_QUIT_RETAIN_REQ = 132,
	LOGINSRV_QUIT_RETAIN_RSP = 133,
	LOGINSRV_QUIT_RETAIN_NO_REWARD_RSP = 134,

	//PK场
	LOGINSRV_GET_PK_INFO_REQ = 135,	//获取PK场信息
	LOGINSRV_GET_PK_INFO_RSP = 136,	

	LOGINSRV_GET_PK_RULE_INFO_REQ = 137,	//获取PK场规则信息
	LOGINSRV_GET_PK_RULE_INFO_RSP = 138,

	LOGINSRV_PK_PLAY_SIGN_UP_REQ = 139,		//PK场报名
	LOGINSRV_PK_PLAY_SIGN_UP_RSP = 140,

	LOGINSRV_PK_PLAY_STATUS_NOTIFY = 141,	//PK场状态通知

	LOGINSRV_PK_PLAY_READY_NOTIFY = 142,	//PK场快速匹配玩家准备 通知

	LOGINSRV_PK_PLAY_READY_REQ = 143,		//PK场快速匹配玩家准备 请求
	LOGINSRV_PK_PLAY_READY_RSP = 144,		//PK场快速匹配玩家准备 应答

	LOGINSRV_PK_PLAY_CANCEL_SIGN_UP_REQ = 145,	//PK场取消报名 请求
	LOGINSRV_PK_PLAY_CANCEL_SIGN_UP_RSP = 146,	//PK场取消报名 应答

	LOGINSRV_PK_PLAY_MATCHING_FINISH_NOTIFY = 147, //匹配成功通知

	LOGINSRV_PK_PLAY_READY_RESULT_NOTIFY = 148,	//PK场快速匹配玩家准备 结果通知

	LOGINSRV_PK_PLAY_RECONNECTION_NOTIFY = 149,			//PK场断线重连
	LOGINSRV_PK_PLAY_RECONNECTION_REQ = 150,			//PK场断线重连请求

	LOGINSRV_CHANGE_PROP_NOTIFY = 151,					// 道具变更通知

	LOGINSRV_PK_PLAY_RECONNECTION_RSP = 152,			//	PK场断线重连应答

	LOGINSRV_USER_ATTRIBUTE_UPDATE_NOTIFY = 153,		// 用户属性更新通知
	
	LOGINSRV_READ_SYSTEM_MSG_NOTIFY = 154,	        // 已读系统消息通知

	LOGINSRV_GET_PK_PLAY_RANKING_INFO_REQ = 155,		// 获取PK场排名信息请求
	LOGINSRV_GET_PK_PLAY_RANKING_INFO_RSP = 156,		// 获取PK场排名信息应答

	LOGINSRV_GET_PK_PLAY_RANKING_REWAR_REQ = 157,		// 获取PK场排名奖励请求
	LOGINSRV_GET_PK_PLAY_RANKING_REWAR_RSP = 158,		// 获取PK场排名奖励应答

	LOGINSRV_GET_PK_PLAY_RANKING_RULE_REQ = 159,		// 获取PK场排名详细规则请求
	LOGINSRV_GET_PK_PLAY_RANKING_RULE_RSP = 160,		// 获取PK场排名详细规则应答

	LOGINSRV_GET_PK_PLAY_CELEBRITY_INFO_REQ = 161,		// 获取PK场名人堂请求
	LOGINSRV_GET_PK_PLAY_CELEBRITY_INFO_RSP = 162,		// 获取PK场名人堂应答
	
	
	// 红包口令相关
	LOGINSRV_GET_RED_GIFT_INFO_REQ = 163,		        // 获取红包口令信息请求
	LOGINSRV_GET_RED_GIFT_INFO_RSP = 164,		        // 获取红包口令信息应答
	
	LOGINSRV_RECEIVE_RED_GIFT_REQ = 165,		        // 领取好友的红包请求
	LOGINSRV_RECEIVE_RED_GIFT_RSP = 166,		        // 领取好友的红包应答
	
	LOGINSRV_VIEW_RED_GIFT_REWARD_REQ = 167,		    // 查看奖励列表请求
	LOGINSRV_VIEW_RED_GIFT_REWARD_RSP = 168,		    // 查看奖励列表应答
	
	LOGINSRV_RECEIVE_RED_GIFT_REWARD_REQ = 169,		    // 领取红包口令获得的奖励请求
	LOGINSRV_RECEIVE_RED_GIFT_REWARD_RSP = 170,		    // 领取红包口令获得的奖励应答
	
	
	// 背包相关
	LOGINSRV_GET_KNAPSACK_INFO_REQ = 171,		        // 获取背包信息请求
	LOGINSRV_GET_KNAPSACK_INFO_RSP = 172,		        // 获取背包信息应答
	
	// PK场任务活动相关
	LOGINSRV_GET_PK_TASK_INFO_REQ = 173,		        // 获取PK场任务信息请求
	LOGINSRV_GET_PK_TASK_INFO_RSP = 174,		        // 获取PK场任务信息应答
	
	LOGINSRV_RECEIVE_PK_TASK_REWARD_REQ = 175,		    // 领取PK场任务奖励请求
	LOGINSRV_RECEIVE_PK_TASK_REWARD_RSP = 176,		    // 领取PK场任务奖励应答
	
	LOGINSRV_PLAYER_QUIT_NOTIFY = 177,		            // 通知前端用户必须退出游戏
	
	// 绑定兑换商城账号信息
	LOGINSRV_BINDING_EXCHANGE_MALL_INFO_REQ = 178,		// 绑定兑换商城信息请求
	LOGINSRV_BINDING_EXCHANGE_MALL_INFO_RSP = 179,		// 绑定兑换商城信息应答
	
	// 比赛场相关
	LOGINSRV_GET_ARENA_INFO_REQ = 180,		            // 获取比赛场信息请求
	LOGINSRV_GET_ARENA_INFO_RSP = 181,		            // 获取比赛场信息应答
	
	LOGINSRV_GET_DETAILED_MATCH_INFO_REQ = 182,		    // 获取详细比赛场次信息请求
	LOGINSRV_GET_DETAILED_MATCH_INFO_RSP = 183,		    // 获取详细比赛场次信息应答
	
	LOGINSRV_ARENA_MATCH_ON_NOTIFY = 184,		        // 比赛场正在进行中主动通知
	
	LOGINSRV_GET_MAIL_MESSAGE_REQ = 185,		        // 获取邮件信息请求
	LOGINSRV_GET_MAIL_MESSAGE_RSP = 186,		        // 获取邮件信息应答
	
	LOGINSRV_OPT_MAIL_MESSAGE_REQ = 187,		        // 操作邮件信息请求
	LOGINSRV_OPT_MAIL_MESSAGE_RSP = 188,		        // 操作邮件信息应答
	
	LOGINSRV_CANCEL_ARENA_MATCH_NOTIFY = 189,		    // 断线重连后，用户主动取消比赛场比赛
	
	// 获取象棋服务信息请求&应答
	LOGINSRV_GET_XIANGQI_SERVICE_INFO_REQ = 190,
	LOGINSRV_GET_XIANGQI_SERVICE_INFO_RSP = 191,
	
	// 打开抢红包活动UI界面
	LOGINSRV_OPEN_RED_PACKET_ACTIVITY_REQ = 192,
	LOGINSRV_OPEN_RED_PACKET_ACTIVITY_RSP = 193,
	
	// 调用了打开抢红包活动UI界面之后
    // 玩家关闭UI页面是必须调用关闭活动消息接口通知服务器
	LOGINSRV_CLOSE_RED_PACKET_ACTIVITY_NOTIFY = 194,
	
	// 同步刷新活动剩余时间，提高前后台时间一致性，服务器定时通知到客户端
	LOGINSRV_UPDATE_RED_ACTIVITY_TIME_NOTIFY = 195,
	
	// 用户点击开抢红包
	LOGINSRV_USER_CLICK_RED_PACKET_REQ = 196,
	LOGINSRV_USER_CLICK_RED_PACKET_RSP = 197,

	// 查看红包历史记录列表
	LOGINSRV_VIEW_RED_PACKET_HISTORY_LIST_REQ = 198,
	LOGINSRV_VIEW_RED_PACKET_HISTORY_LIST_RSP = 199,
	
	// 查看单个红包详细的历史记录信息
	LOGINSRV_VIEW_RED_PACKET_DETAILED_HISTORY_REQ = 200,
	LOGINSRV_VIEW_RED_PACKET_DETAILED_HISTORY_RSP = 201,
	
	// 查看个人红包历史记录
	LOGINSRV_VIEW_PERSONAL_RED_PACKET_HISTORY_REQ = 202,
	LOGINSRV_VIEW_PERSONAL_RED_PACKET_HISTORY_RSP = 203,
	
	// 获取抢红包活动时间信息
	LOGINSRV_GET_RED_PACKET_ACTIVITY_TIME_REQ = 204,
	LOGINSRV_GET_RED_PACKET_ACTIVITY_TIME_RSP = 205,
	
	// 进入福袋活动
	LOGINSRV_ENTER_CATCH_GIFT_ACTIVITY_REQ = 206,
	LOGINSRV_ENTER_CATCH_GIFT_ACTIVITY_RSP = 207,
	
	// 获取福袋活动时间
	LOGINSRV_GET_CATCH_GIFT_ACTIVITY_TIME_REQ = 208,
	LOGINSRV_GET_CATCH_GIFT_ACTIVITY_TIME_RSP = 209,
	
	// 碰撞命中福袋活动掉落的物品
	LOGINSRV_HIT_CATCH_GIFT_ACTIVITY_GOODS_REQ = 210,
	LOGINSRV_HIT_CATCH_GIFT_ACTIVITY_GOODS_RSP = 211,
	
	// 福袋活动增加物品掉落通知
	LOGINSRV_CATCH_GIFT_ACTIVITY_ADD_GOODS_DROP_NOTIFY = 212,
	
	// 福袋活动结束通知
	LOGINSRV_FINISH_CATCH_GIFT_ACTIVITY_NOTIFY = 213,

	// 获取新手引导信息
	LOGINSRV_GET_NEW_PLAYER_GUIDE_REQ = 214,
	LOGINSRV_GET_NEW_PLAYER_GUIDE_RSP = 215,
	
	// 新手引导流程完毕通知
	LOGINSRV_FINISH_NEW_PLAYER_GUIDE_NOTIFY = 216,
	
	// 获取游戏服务信息请求&应答
	LOGINSRV_GET_GAME_SERVICE_INFO_REQ = 217,
	LOGINSRV_GET_GAME_SERVICE_INFO_RSP = 218,
	
	// 苹果版本充值结果验证通知
	LOGINSRV_APPLE_RECHARGE_RESULT_NOTIFY = 219,
	
	// 韩文版HanYou平台使用优惠券通知
	LOGINSRV_USE_HAN_YOU_COUPON_NOTIFY = 220,
	
	// 韩文版HanYou平台使用优惠券获得的奖励通知
	LOGINSRV_ADD_HAN_YOU_COUPON_REWARD_NOTIFY = 221,
	
	// 自动积分兑换物品
	LOGINSRV_AUTO_SCORE_EXCHANGE_ITEM_REQ = 222,
	LOGINSRV_AUTO_SCORE_EXCHANGE_ITEM_RSP = 223,
};

// commonDB服务定义的协议ID值
enum CommonDBSrvProtocol
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
	
	// BUSINESS_PROP_CHARGE_REQ = 23,
	// BUSINESS_PROP_CHARGE_RSP = 24,
	
	BUSINESS_GET_USERNAME_BY_IMEI_REQ = 25,
	BUSINESS_GET_USERNAME_BY_IMEI_RSP = 26,
	
	BUSINESS_GET_USERNAME_BY_PLATFROM_REQ = 33,
	BUSINESS_GET_USERNAME_BY_PLATFROM_RSP = 34,
	
	BUSINESS_EXCHANGE_PHONE_CARD_REQ = 37,
	BUSINESS_EXCHANGE_PHONE_CARD_RSP = 38,

	BUSINESS_EXCHANGE_GOODS_REQ = 39,
	BUSINESS_EXCHANGE_GOODS_RSP = 40,
	
	// 道具变更新接口
	BUSINESS_CHANGE_PROP_REQ = 49,
	BUSINESS_CHANGE_PROP_RSP = 50,

	// 获取目标用户的任意信息
	BUSINESS_GET_USER_OTHER_INFO_REQ = 57,
	BUSINESS_GET_USER_OTHER_INFO_RSP = 58,

	// 兑换话费额度
	BUSINESS_EXCHANGE_PHONE_FARE_REQ = 61,
	BUSINESS_EXCHANGE_PHONE_FARE_RSP = 62,

	// 设置目标用户的任意信息
	BUSINESS_RESET_USER_OTHER_INFO_REQ = 63,
	BUSINESS_RESET_USER_OTHER_INFO_RSP = 64,

	// 获取积分商城信息
	BUSINESS_GET_MALL_SCORES_INFO_REQ = 71,
	BUSINESS_GET_MALL_SCORES_INFO_RSP = 72,

	//兑换积分商品
	BUSINESS_EXCHANGE_SCORES_ITEM_REQ = 73,
	BUSINESS_EXCHANGE_SCORES_ITEM_RSP = 74,

	// 获取目标多个用户的任意信息
	BUSINESS_GET_MANY_USER_OTHER_INFO_REQ = 76,
	BUSINESS_GET_MANY_USER_OTHER_INFO_RSP = 77,

	//查询用户道具以及防作弊列表信息
	BUSINESS_GET_USER_PROP_AND_PREVENTION_CHEAT_REQ = 78,
	BUSINESS_GET_USER_PROP_AND_PREVENTION_CHEAT_RSP = 79,
	
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
	
	// 新增加红包信息
	BUSINESS_ADD_ACTIVITY_RED_PACKET_REQ = 120,
	BUSINESS_ADD_ACTIVITY_RED_PACKET_RSP = 121,
	
	// 获取活动红包数据
	BUSINESS_GET_ACTIVITY_RED_PACKET_REQ = 122,
	BUSINESS_GET_ACTIVITY_RED_PACKET_RSP = 123,
	
	// 韩文版HanYou平台玩家使用优惠券通知
	BUSINESS_USE_HAN_YOU_COUPON_NOTIFY = 124,
	
	// 自动积分兑换物品
	BUSINESS_AUTO_SCORE_EXCHANGE_ITEM_REQ = 127,
	BUSINESS_AUTO_SCORE_EXCHANGE_ITEM_RSP = 128,
};

// game db proxy 服务定义的协议ID值
enum GameServiceDBProtocol
{
	SET_GAME_DATA_REQ = 1,
	SET_GAME_DATA_RSP = 2,
	GET_GAME_DATA_REQ = 3,
	GET_GAME_DATA_RSP = 4,
	
    SET_GAME_RECORD_REQ = 5,
	SET_GAME_RECORD_RSP = 6,
	GET_GAME_RECORD_REQ = 7,
	GET_GAME_RECORD_RSP = 8,

	GET_MORE_KEY_GAME_DATA_REQ = 11,
	GET_MORE_KEY_GAME_DATA_RSP = 12,
};

// PK场服务内部协议
enum PKServiceProtocol
{
	// 大厅通知玩家开始PK游戏
	SERVICE_PLAYER_START_PK_REQ = 1,
	SERVICE_PLAYER_START_PK_RSP = 2,

	// 大厅通知玩家弃权PK游戏
	SERVICE_PLAYER_WAIVER_REQ = 3,
	SERVICE_PLAYER_WAIVER_RSP = 4,

	// 游戏通知大厅PK游戏结束
	SERVICE_PLAYER_END_PK_NOTIFY = 5,
};


// 消息服务协议ID值
enum MessageSrvProtocol
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
	
	// 把消息转发到消息中心，由消息中心路由该消息的目标服务（用户在线的服务）
	BUSINESS_FORWARD_MESSAGE_TO_SERVICE_REQ = 16,
	BUSINESS_FORWARD_MESSAGE_TO_SERVICE_RSP = 17,
	
	// 查看排行榜
	BUSINESS_VIEW_RANKING_LIST_REQ = 18,
	BUSINESS_GOLD_RANKING_LIST_RSP = 19,
	BUSINESS_PHONE_RANKING_LIST_RSP = 20,
	
	BUSINESS_READ_SYSTEM_MSG_NOTIFY = 21,	    // 已读系统消息通知

	// 更新PK排行榜
	BUSINESS_UPDATE_PK_PLAY_RANKING_LIST_REQ = 22,
	BUSINESS_UPDATE_PK_PLAY_RANKING_LIST_RSP = 23,

	// 查看PK场排行
	BUSINESS_VIEW_PK_PLAY_RANKING_LIST_REQ = 24,
	BUSINESS_VIEW_PK_PLAY_RANKING_LIST_RSP = 25,

	BUSINESS_GET_PK_PLAY_RANKING_REWAR_REQ = 26,		// 获取PK场排名奖励请求
	BUSINESS_GET_PK_PLAY_RANKING_REWAR_RSP = 27,		// 获取PK场排名奖励应答

	BUSINESS_GET_PK_PLAY_RANKING_RULE_REQ = 28,		// 获取PK场排名详细规则请求
	BUSINESS_GET_PK_PLAY_RANKING_RULE_RSP = 29,		// 获取PK场排名详细规则应答

	BUSINESS_GET_PK_PLAY_CELEBRITY_INFO_REQ = 30,		// 获取PK场名人堂请求
	BUSINESS_GET_PK_PLAY_CELEBRITY_INFO_RSP = 31,		// 获取PK场名人堂应答

	//获取PK排行榜清空次数
	BUSINESS_GET_PK_PLAY_RANKING_CLEAR_NUM_REQ = 32,
	BUSINESS_GET_PK_PLAY_RANKING_CLEAR_NUM_RSP = 33,

	BUSINESS_OFF_LINE_PK_RANKING_REWARD_REQ = 34,	//PK场离线玩家的奖励
	BUSINESS_OFF_LINE_PK_RANKING_REWARD_RSP = 35,	//PK场离线玩家的奖励

	BUSINESS_ON_LINE_PK_RANKING_REWARD_REQ = 36,	//PK场在线玩家的奖励
	BUSINESS_ON_LINE_PK_RANKING_REWARD_RSP = 37,	//PK场在线玩家的奖励
	
	BUSINESS_FORCE_PLAYER_QUIT_NOTIFY = 38,         // 踢玩家下线&封号锁定玩家账号
	
	BUSINESS_GET_ARENA_RANKING_REQ = 39,	        // 获取比赛场排名信息请求
	BUSINESS_GET_ARENA_RANKING_RSP = 40,	        // 获取比赛场排名信息应答
};

enum ManagerSrvProtocol
{
	// 用户个人财务变更通知，如：金币变化、道具变化、奖券变化等
	PERSON_PROPERTY_NOTIFY = 8,
};

// http服务协议ID值
enum HttpServiceProtocol
{
	CHECK_DOWNJOY_USER_REQ = 1,
	CHECK_DOWNJOY_USER_RSP = 2,
	
	GET_DOWNJOY_RECHARGE_TRANSACTION_REQ = 3,
	GET_DOWNJOY_RECHARGE_TRANSACTION_RSP = 4,
	DOWNJOY_USER_RECHARGE_NOTIFY = 5,
	CANCEL_DOWNJOY_RECHARGE_NOTIFY = 6,
	
	// Google SDK 充值操作
	GET_GOOGLE_RECHARGE_TRANSACTION_REQ = 7,
	GET_GOOGLE_RECHARGE_TRANSACTION_RSP = 8,
	CHECK_GOOGLE_RECHARGE_RESULT_REQ = 9,
	CHECK_GOOGLE_RECHARGE_RESULT_RSP = 10,
	
	// 第三方平台用户充值
	GET_THIRD_RECHARGE_TRANSACTION_REQ = 11,
	GET_THIRD_RECHARGE_TRANSACTION_RSP = 12,
	THIRD_USER_RECHARGE_NOTIFY = 13,
	CANCEL_THIRD_RECHARGE_NOTIFY = 14,
	
	// 小米用户登录校验
	CHECK_XIAOMI_USER_REQ = 15,
	CHECK_XIAOMI_USER_RSP = 16,
	
    // 第三方平台用户校验
	CHECK_THIRD_USER_REQ = 17,
	CHECK_THIRD_USER_RSP = 18,
	
	// 商城配置
	GET_MALL_CONFIG_REQ = 19,
	GET_MALL_CONFIG_RSP = 20,

	//查询腾讯充值是否成功
	FIN_RECHARGE_SUCCESS_REQ = 21,
	FIN_RECHARGE_SUCCESS_RSP = 22,

	//鱼币充值
	FISH_COIN_RECHARGE_NOTIFY = 27,
	
	// 韩友 Facebook 用户登录验证
	CHECK_FACEBOOK_USER_REQ = 30,
	CHECK_FACEBOOK_USER_RSP = 31,
};


// 本服务自定义的应答协议ID值
enum EServiceReplyProtocolId
{
	CUSTOM_GET_USER_REGISTER_TIME_FOR_NEWBIE_ACTIVITY = 30000,      // 获取用户注册时间点看是否可用参与新手礼包活动
	CUSTOM_RECEIVE_ACTIVITY_REWARD = 30001,                         // 领取活动奖励
	CUSTOM_GET_GAME_DATA_FOR_TASK_REWARD = 30002,				    // 获取用户游戏逻辑数据 查看是否存在任务奖励没领取
	CUSTOM_GET_GAME_DATA_FOR_SIGN_UP = 30003,					    // 获取用户游戏逻辑数据 判断能否报名PK场
	CUSTOM_GET_HALL_DATA_FOR_PK_RANKING_REWARD = 30004,			    // 获取用户大厅数据 用于更新PK排行榜奖励
	
	CUSTOM_GET_MORE_HALL_DATA_FOR_PK_RANKING_REWARD = 30005,	    // 获取多个用户大厅数据 用于更新PK排行榜奖励
	CUSTOM_GET_MORE_HALL_DATA_FOR_UP_RANKING_INFO = 30006,		    // 获取多个用户大厅数据 用于更新PK排行榜分数
	
	CUSTOM_GET_MORE_USER_DATA_FOR_VIEW_RED_GIFT_REWARD = 30007,		// 获取多个用户昵称用于查看红包口令奖励列表
	CUSTOM_GET_HALL_DATA_FOR_RED_GIFT_REWARD = 30008,		        // 获取用户大厅数据，领取红包口令奖励
	
	CUSTOM_GET_RED_GIFT_FRIEND_STATIC_DATA = 30009,		            // 获取红包好友的静态数据
	CUSTOM_GET_HALL_DATA_FOR_RED_GIFT_FRIEND = 30010,		        // 获取用户大厅数据，添加红包口令好友
	
	CUSTOM_GET_KNAPSACK_INFO_FROM_COMMON_DB = 30011,		        // 直接从common db proxy获取背包的全部信息
	
	CUSTOM_GET_USERS_INFO_FOR_ARENA_RANKING = 30012,		        // 获取多个排名用户的昵称，比赛场排名使用
	
	CUSTOM_GET_USER_INFO_FOR_CATCH_GIFT_ACTIVITY = 30013,		    // 获取用户的信息，参加福袋活动使用
	CUSTOM_GET_USER_VIP_FOR_CATCH_GIFT_ACTIVITY = 30014,		    // 获取用户的VIP信息，查看福袋活动时间使用
};



// 挂接在连接上的相关数据
struct ConnectUserData
{
	com_protocol::HallLogicData* hallLogicData;
	unsigned int connIdLen;
	unsigned int loginSecs;
	unsigned int portrait_id;              // 用户头像ID
	unsigned char status;                  // 连接状态
	unsigned char versionFlag;             // 客户端版本标识
	unsigned char platformType;            // 平台类型
	unsigned char loginTimes;              // 重复登陆次数
	char nickname[MaxConnectIdLen];        // 用户昵称
	char connId[MaxConnectIdLen];
	unsigned int phoneBill;				   // 本次用户登录获取的话费数
	unsigned int propVoucher;			   // 本次用户登录获取的奖券数
	unsigned int score;					   // 本次用户登录获取的积分
	unsigned int shareId;					   // 分享ID
};

//VIP领取状态
enum VIPStatus
{
	VIPNoReward = 0,		// 没有奖励
	VIPReceiveReward = 1,	// 可领取VIP奖励
	VIPAlreadyGeted = 2,	// 已领取VIP奖励
};


// 活动中心类型
enum EActivityCenterType
{
	ActivityList = 0,		                 // 所有可参加的活动列表
	NewbieGift = 1,	                         // 新手礼包
	RechargeLottery = 2,	                 // 充值抽奖
};

// 新手礼包状态
enum NewbieGiftStatus
{
	InitNewbie = 0,            // 0：初始化状态
	HasNewbieGift = 1,         // 1：存在没有领取的新手礼包
	ReceivedNewbieGift = 2,    // 2：已经领取了新手礼包
	WithoutNewbieGifg = 3,     // 3：不在活动期间注册没有新手礼包
};

enum EPKPlayRankingRewardStatus
{
	ECanAward = 0,			//可以领奖
	EAlreadyAward = 1,		//已经领奖
	ENotAward = 2,			//没有奖励
};

const static int NoneRanking = -1;		//没有排名


// PK任务类型
enum EPKTaskType
{
	EPKJoinType = 1,      // 1：对局任务
	EPKWinType = 2,       // 2：赢局任务
	EPKOnlineType = 3,    // 3：在线任务
};

// 统计数据项
enum EStatisticsDataItem
{
	EAllStatisticsData = 1,                       // 总量统计数据
	EHourStatisticsData = 2,                      // 小时统计数据
	EDayStatisticsData = 3,                       // 天统计数据
	
	ERechargeLotteryDraw = 1,                     // 充值抽奖
	EPKTaskStat = 2,                              // PK任务
	EReceivedRedGift = 3,                         // 领取红包
	ERedGiftTakeRatio = 4,                        // 红包分成
};


}

#endif // TYPE_DEFINE_H
