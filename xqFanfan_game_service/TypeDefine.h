
/* author : limingfan
 * date : 2016.12.06
 * description : 类型定义
 */

#ifndef TYPE_DEFINE_H
#define TYPE_DEFINE_H

#include <string.h>

#include "SrvFrame/UserType.h"
#include "_XiangQiConfig_.h"
#include "app_server_xiangqi_game.pb.h"


namespace NPlatformService
{

static const int MaxConnectIdLen = 36;
static const unsigned int MaxUserDataCount = 128;

static const unsigned int MaxLocalAsyncDataSize = 128;
static const unsigned int LocalAsyncDataCount = 128;


// 协议操作返回码
enum ProtocolReturnCode
{
	Opt_Failed = -1,
	Opt_Success = 0,                                                     // 操作成功
	XiangQiSessionKeyVerifyFailed = 9001,                                // 会话验证失败
	XiangQiNoFoundConnectProxy = 9002,                                   // 没有找到用户数据
	XiangQiEncodeDeCodeError = 9003,                                     // 消息编解码错误
	XiangQiSendServiceMsgError = 9004,                                   // 发送服务消息失败
	XiangQiGoldNotEnough = 9005,                                         // 游戏币不足
	XiangQiMoveChessPiecesInvalid = 9006,                                // 移动棋子无效
	XiangQiInvalidFFChesssMode = 9007,                                   // 无效的翻翻棋模式
	XiangQiInvalidFFChesssArena = 9008,                                  // 无效的翻翻棋场次
	XiangQiMoreGoldError = 9009,                                         // 游戏币过多
	XiangQiPleasePeaceMoveChessInvalid = 9010,                           // 必须走动棋子N步之后才可以发起求和请求
	XiangQiPleasePeaceOpponentRefuse = 9011,                             // 求和请求被对方拒绝
	XiangQiPleasePeaceTimesInvalid = 9012,                               // 当局已经超过最大的求和次数
	XiangQiMovePositionInvalid = 9013,                                   // 无效的移动位置
	XiangQiNotOurSide = 9014,                                            // 不是我方可以移动的棋子
	XiangQiCanNotEatOpponentSide = 9015,                                 // 不能吃掉对方的棋子
	XiangQiRestrictMove = 9016,                                          // 该棋子已被禁足
};


// XiangQi服务定义的协议ID值
enum XiangQiSrvProtocol
{
	// XiangQi 服务与外部客户端之间的协议
	// 玩家验证
    XIANGQISRV_CHECK_USER_REQ = 1,
	XIANGQISRV_CHECK_USER_RSP = 2,
	
	// 玩家退出游戏
	XIANGQISRV_USER_QUIT_REQ = 3,
	XIANGQISRV_USER_QUIT_RSP = 4,
	
	// 玩家进入游戏通知服务器
	XIANGQISRV_ENTER_GAME_NOTIFY = 5,
	
	// 匹配对手成功通知玩家
	XIANGQISRV_MATCHING_NOTIFY = 6,
	
	// 准备操作通知
	XIANGQISRV_READY_OPT_NOTIFY = 7,
	
	// 通知玩家可以走动棋子了
	XIANGQISRV_MOVE_CHESS_PIECES_NOTIFY = 8,
	
	// 玩家走动了棋子，通知到服务器
	XIANGQISRV_PLAY_CHESS_PIECES_NOTIFY = 9,
	
	// 玩家走动棋子结果，通知到双方玩家
	XIANGQISRV_PLAY_CHESS_RESULT_NOTIFY = 10,
	
	// 棋局结束结算结果，通知到双方玩家
	XIANGQISRV_SETTLEMENT_RESULT_NOTIFY = 11,
	
	// 玩家进入翻翻棋场次
	XIANGQISRV_ENTER_CHESS_ARENA_REQ = 12,
	XIANGQISRV_ENTER_CHESS_ARENA_RSP = 13,
	
	// 玩家退出翻翻棋场次通知服务器
	XIANGQISRV_LEAVE_CHESS_ARENA_NOTIFY = 14,
	
	// 获取翻翻棋场次信息
	XIANGQISRV_GET_ARENA_INFO_REQ = 15,
	XIANGQISRV_GET_ARENA_INFO_RSP = 16,
	
	// 玩家主动认输通知到服务器
	XIANGQISRV_PLAYER_DEFEATE_NOTIFY = 17,
	
	// 玩家发起求和请求
	XIANGQISRV_PLEASE_PEACE_REQ = 18,
	XIANGQISRV_PLEASE_PEACE_RSP = 19,
	
	// 玩家A的求和请求通知到对手另一方玩家B
	// 玩家B的拒绝&同意操作通知到服务器
	XIANGQISRV_PLEASE_PEACE_NOTIFY = 20,
	XIANGQISRV_PLEASE_PEACE_CONFIRM = 21,
	
	// 玩家切换对手
	XIANGQISRV_SWITCH_OPPONENT_REQ = 22,
	XIANGQISRV_SWITCH_OPPONENT_RSP = 23,
	
	// 玩家再来一局
	XIANGQISRV_PLAY_AGAIN_REQ = 24,
	XIANGQISRV_PLAY_AGAIN_RSP = 25,
	
	// 获取任务列表
	XIANGQISRV_GET_TASK_REQ = 26,
	XIANGQISRV_GET_TASK_RSP = 27,
	
	// 领取任务奖励
	XIANGQISRV_RECEIVE_TASK_REWARD_REQ = 28,
	XIANGQISRV_RECEIVE_TASK_REWARD_RSP = 29,
	
	// 物品刷新通知
	XIANGQISRV_GOODS_CHANGE_NOTIFY = 30,
	
	// 获取翻翻棋商城配置信息
	XIANGQISRV_GET_FF_CHESS_MALL_CONFIG_REQ = 31,
	XIANGQISRV_GET_FF_CHESS_MALL_CONFIG_RSP = 32,
	
	// 充值渔币
	XIANGQISRV_GET_RECHARGE_FISH_COIN_ORDER_REQ = 33,
	XIANGQISRV_GET_RECHARGE_FISH_COIN_ORDER_RSP = 34,
	XIANGQISRV_RECHARGE_FISH_COIN_NOTIFY = 35,
	XIANGQISRV_CANCEL_RECHARGE_FISH_COIN_NOTIFY = 36,
	
	// 兑换物品
	XIANGQISRV_EXCHANGE_GOODS_REQ = 37,
	XIANGQISRV_EXCHANGE_GOODS_RSP = 38,
	
	// 获取翻翻棋道具商城数据
	XIANGQISRV_GET_FF_CHESS_GOODS_MALL_REQ = 39,
	XIANGQISRV_GET_FF_CHESS_GOODS_MALL_RSP = 40,
	
	// 兑换翻翻棋道具
	XIANGQISRV_EXCHANGE_FF_CHESS_GOODS_REQ = 41,
	XIANGQISRV_EXCHANGE_FF_CHESS_GOODS_RSP = 42,
	
	// 使用翻翻棋道具
	XIANGQISRV_USE_FF_CHESS_GOODS_REQ = 43,
	XIANGQISRV_USE_FF_CHESS_GOODS_RSP = 44,
	
	// 玩家准备操作结果通知
	XIANGQISRV_READY_OPT_RESULT_NOTIFY = 45,
	
	// 获取翻翻棋用户基本信息
	XIANGQISRV_GET_FF_CHESS_USER_BASE_INFO_REQ = 46,
	XIANGQISRV_GET_FF_CHESS_USER_BASE_INFO_RSP = 47,
	
	// 获取翻翻棋规则&信息
	XIANGQISRV_GET_FF_CHESS_RULE_INFO_REQ = 48,
	XIANGQISRV_GET_FF_CHESS_RULE_INFO_RSP = 49,
	
	// 翻翻棋游戏内同桌聊天
	XIANGQISRV_USER_CHAT_REQ = 50,
	XIANGQISRV_USER_CHAT_NOTIFY = 51,
	
	// 玩家空走棋子通知
	XIANGQISRV_CONTINUE_MOVE_NOTIFY = 52,
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
	
	// 获取游戏商城配置
	BUSINESS_GET_GAME_MALL_CONFIG_REQ = 51,
	BUSINESS_GET_GAME_MALL_CONFIG_RSP = 52,

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
	
	// 修改多用户道具
	BUSINESS_CHANGE_MANY_PEOPLE_PROP_REQ = 80,
	BUSINESS_CHANGE_MANY_PEOPLE_PROP_RSP = 81,
	
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
	// 商城配置
	GET_MALL_CONFIG_REQ = 19,
	GET_MALL_CONFIG_RSP = 20,

	//鱼币充值
	FISH_COIN_RECHARGE_NOTIFY = 27,
};


// 本服务自定义的应答协议ID值
enum EServiceReplyProtocolId
{
	CUSTOM_GET_USER_BASE_INFO_FOR_CHECK = 30000,      // 获取用户基本信息用于验证用户
	CUSTOM_RECEIVE_TASK_REWARD_REPLY = 30001,         // 领取任务奖励应答消息
};



// 连接状态
enum ConnectStatus
{
	ECheckUser = 0,
	EEnterGame = 1,
	EQuitGame = 2,
};

// 关闭连接的方式
enum EQuitGameMode
{
	EActivityQuit = 1,        // 主动退出游戏，关闭连接
	EPassiveQuit = 2,         // 被动关闭连接，退出游戏
	EServiceUnload = 3,       // 服务正常停止
};

// 红黑对阵双方
enum EXQSideFlag
{
	ERedSide = 0,             // 红方
	EBlackSide = 1,           // 黑方
	ESideFlagCount = 2,       // 总数
};

// 变更物品标识
enum EChangeGoodsFlag
{
	EEnterGameServiceCost = 1,         // 进场服务扣费
	EFinishSettlement = 2,             // 游戏结束结算
	EReceiveTaskReward = 3,            // 领取任务奖励
};

// 移动棋子操作
enum EMoveChessPiecesOpt
{
	EMoveBeEatCP = 1,                  // 移动被吃掉的棋子
	EMoveOurCP = 2,                    // 移动棋子
};

enum EFindMoveChessResult
{
	ECanEatCP = 1,                     // 可以吃掉对方棋子
	ECanMoveCP = 2,                    // 可移动我方棋子
	ECanNotMoveCP = 3,                 // 不可移动棋子
};

enum EFFQiTaskType
{
	ETaskRecharge = 1,                 // 充值任务
	ETaskWin = 2,                      // 胜局
	ETaskJoin = 3,                     // 对局
	ETaskContinueWin = 4,              // 连胜
	ETaskEatGeneral = 5,               // 对方将帅一出就被吃
	ETaskOneContinueHit = 6,           // 一局连击
	ETaskOnePaoEatPieces = 7,          // 一局炮吃子
};

enum TaskStatisticsOpt
{
	ETSOpenPieces = 1,                 // 翻开棋子
	ETSEatOpponentPieces = 2,          // 吃掉对手的棋子
	ETSPaoEatPieces = 3,               // 炮吃掉棋子
	ETSContinueHit = 4,                // 连击操作
};


// 挂接在连接上的相关数据
struct XQUserData
{
	char username[MaxConnectIdLen];                  // 用户ID
	char nickname[MaxConnectIdLen];                  // 用户昵称
	unsigned int usernameLen;                        // 用户ID长度
	unsigned int portrait_id;                        // 用户头像ID
	int status;                                      // 连接状态
	
	uint64_t cur_gold;                               // 当前金币数量
	
	unsigned int chessBoardId;                       // 棋盘ID
	unsigned int chessPiecesId;                      // 棋子ID
	
	unsigned int playMode;                           // 翻翻棋模式
	unsigned int arenaId;                            // 场次ID
	unsigned int sideFlag;                           // 红黑对阵双方标识
	
	unsigned int playAgainId;                        // 再来一局标识ID
	
	unsigned int waitTimer;                          // 等待匹配超时定时器ID，超时则自动匹配机器人
	
	com_protocol::XiangQiServerData* srvData;        // 逻辑数据
	

	void init()
	{
		NEW(srvData, com_protocol::XiangQiServerData());
	}
	
	void unInit()
	{
		DELETE(srvData);
	}
};


// 棋牌布局
// ----------------------------
//       棋盘位置分布图
//       0--08--16--24
//       |   |   |   |
//       1--09--17--25
//       |   |   |   |
//       2--10--18--26
//       |   |   |   |
//       3--11--19--27
//       |   |   |   |
//       4--12--20--28
//       |   |   |   |
//       5--13--21--29
//       |   |   |   |
//       6--14--22--30
//       |   |   |   |
//       7--15--23--31
// ----------------------------

/*
// 象棋棋子值定义
enum EChessPiecesValue
{
	EHongQiJiang = 0;                                      // 红棋 将
	EHongQiShi = 1;                                        // 红棋 士
	EHongQiXiang = 2;                                      // 红棋 象
	EHongQiChe = 3;                                        // 红棋 车
	EHongQiMa = 4;                                         // 红棋 马
	EHongQiPao = 5;                                        // 红棋 炮
	EHongQiBing = 6;                                       // 红棋 兵
	
	EHeiQiJiang = 7;                                       // 黑棋 将
	EHeiQiShi = 8;                                         // 黑棋 士
	EHeiQiXiang = 9;                                       // 黑棋 象
	EHeiQiChe = 10;                                        // 黑棋 车
	EHeiQiMa = 11;                                         // 黑棋 马
	EHeiQiPao = 12;                                        // 黑棋 炮
	EHeiQiBing = 13;                                       // 黑棋 兵
	
	ENoneQiZiClose = 14;                                   // 存在棋子，但没有翻开
	ENoneQiZiOpen = 15;                                    // 不存在棋子，空位置
}
*/


static const int MillisecondUnit = 1000;                  // 秒转换为毫秒乘值
static const int PercentValue = 100;                      // 百分比值
static const short MinChessPiecesPosition = 0;
static const short MaxChessPieCesPosition = 31;
static const int MinDifferenceValue = 1;
static const int MaxDifferenceValue = 8;
static const unsigned int MaxChessPieces = 32;            // 最多32个棋子
static const int ClosedChessPiecesValue = 1000;           // 未翻开的棋子基础值
static const unsigned int OpenChessPiecesCount = 4;
static const unsigned int OpenChessPiecesPosition[OpenChessPiecesCount] = {3, 9, 22, 28};  // 默认翻开棋子的4个位置
static const short ContinueHitTimes = 3;                  // ContinueHitTimes次以上则产生连击
static const unsigned int MaxContinueHitCount = 16;       // 连击的次数最多不会超过10次
static const int RobotReadyOptFlag = 10000;               // 机器人准备操作标识
typedef char SideUserName[MaxConnectIdLen];               // 红黑对阵双方玩家ID


// 禁足信息，任意一个棋子A连续追对方N步之后判定棋子A禁足，即可下一步不允许棋子A移动
struct SRestrictMoveInfo
{
	short srcPosition;                                    // 追击的棋子位置
	short dstPosition;                                    // 被追击的棋子位置
	unsigned int stepCount;                               // 已经追击了的步数
	unsigned int robotChaseStep;                          // 机器人被禁足之前追击的步数
};

// 棋局信息
struct XQChessPieces
{
	// 棋子值，未翻开的棋子值 ==（ClosedChessPiecesValue + EChessPiecesValue）
	// 翻开的棋子值 == EChessPiecesValue
	short value[MaxChessPieces];
	
	// 开局翻开的4个棋子值，结算的时候使用
	short openChessPiecesValue[OpenChessPiecesCount];
	
	// 红黑对阵双方玩家ID
	SideUserName sideUserName[EXQSideFlag::ESideFlagCount];
	
	// 抢先加倍值
	short firstDoubleValue;
	
	// 红黑对阵双方当前血量
	int curLife[EXQSideFlag::ESideFlagCount];
	
	// 双方的连击值及连击的次数
	short curHitValue[EXQSideFlag::ESideFlagCount];
	short continueHitValue[MaxContinueHitCount];
	
	// 双方下棋的步数
	unsigned short moveCount[EXQSideFlag::ESideFlagCount];
	
	// 双方已经请求求和的次数
	unsigned short pleasePeace[EXQSideFlag::ESideFlagCount];
	
	// 当前操作的一方
	unsigned short curOptSide;
	
	// 当前操作的定时器ID
	unsigned int curOptTimerId;
	
	// 禁足信息，任意一个棋子A连续追对方N步之后判定棋子A禁足，即可下一步不允许棋子A移动
    SRestrictMoveInfo restrictMoveInfo;
	
	// 双方连续移动的步数总和，超过配置值N则触发和棋，达到配置值M则触发通知，且只通知一次
	unsigned int allContinueStepCount;
	
	// 任务统计信息
	unsigned short eatOpponentGeneral[EXQSideFlag::ESideFlagCount];        // 对方将帅一出就被吃
	short maxContinueHitValue[EXQSideFlag::ESideFlagCount];                // 一局最大连击数
	unsigned short paoEatPiecesCount[EXQSideFlag::ESideFlagCount];         // 一局最大炮吃子次数
};

typedef unordered_map<string, XQChessPieces*> OnPlayUsers;                                   // 正在玩游戏的玩家列表映射到棋盘布局信息

typedef unordered_map<unsigned int, string> ArenaWaitPlayer;                                 // 各个不同场次等待匹配的玩家
typedef unordered_map<unsigned int, ArenaWaitPlayer> ChessModeWaitPlayer;                    // 不同翻翻棋模式不同场次等待匹配的玩家

typedef unordered_map<unsigned int, unsigned int> ArenaOnlinePlayerCount;                    // 各个不同场次在线人数
typedef unordered_map<unsigned int, ArenaOnlinePlayerCount> ChessModeOnlinePlayerCount;      // 不同翻翻棋模式不同场次在线人数


// 再来一局信息
struct SPlayAgainInfo
{
	SPlayAgainInfo() {};
	
	SPlayAgainInfo(const unsigned int trId, const char* redUser, const char* blackUser)
	{
		timerId = trId;
		
		username[EXQSideFlag::ERedSide] = redUser;
		username[EXQSideFlag::EBlackSide] = blackUser;
		
		flag[EXQSideFlag::ERedSide] = 0;
		flag[EXQSideFlag::EBlackSide] = 0;
	};
	
	~SPlayAgainInfo() {};
	
	unsigned int timerId;
	string username[EXQSideFlag::ESideFlagCount];
	unsigned short flag[EXQSideFlag::ESideFlagCount];                                        // 值1则表示玩家点击了 再来一局 按钮
};
typedef unordered_map<unsigned int, SPlayAgainInfo> SPlayAgainInfoMap;                       // 再来一局双方玩家的信息


// 机器人信息
struct SRobotInfo
{
	SRobotInfo() {}
	
	SRobotInfo(const char* _id, const char* _name)
	{
		strncpy(id, _id, sizeof(id) - 1);
		strncpy(name, _name, sizeof(name) - 1);
	}
	
	~SRobotInfo() {}
	
	char id[MaxConnectIdLen];
	char name[MaxConnectIdLen];
};
typedef vector<SRobotInfo> SRobotInfoVector;

typedef unordered_map<string, XQUserData*> RobotIdToData;   // 机器人ID映射到数据


// 下棋时，机器人可以移动的棋子的信息
struct SRobotMoveChessPieces
{
	SRobotMoveChessPieces() : srcPosition(-1), dstPosition(-1), beEatCP(-1), isOpen(true) {}
	SRobotMoveChessPieces(const short srcPos, const short dstPos, const int cp, bool _isOpen = true) : srcPosition(srcPos), dstPosition(dstPos), beEatCP(cp), isOpen(_isOpen) {}
	~SRobotMoveChessPieces() {}
	
	bool operator==(const SRobotMoveChessPieces& rObj) const {return (beEatCP == rObj.beEatCP);};
	
	short srcPosition;       // 源棋子的位置
	short dstPosition;       // 目标棋子的位置
	int beEatCP;             // 被吃掉的棋子
	bool isOpen;             // 目标棋子是否是翻开了的
};
typedef vector<SRobotMoveChessPieces> SRobotMoveChessPiecesVector;


typedef vector<short> ChessPiecesPositions;  // 棋子的位置


// 机器人移动的位置
union URobotMovePosition
{
	long long positionId;
	struct SRbtMovePosition
	{
		int src;
		int dst;
	} position;
};

// 领取任务奖励数据
struct SReceiveTaskRewardInfo
{
	unsigned short recievedTaskId[MaxLocalAsyncDataSize / sizeof(unsigned short) - 1];
};

// 机器人输赢记录
struct SRobotWinLostRecord
{
	unsigned long long allGold;
	unsigned long long winGold;
	
	SRobotWinLostRecord() : allGold(0), winGold(0) {}
	~SRobotWinLostRecord() {}
};
typedef unordered_map<unsigned int, SRobotWinLostRecord> ArenaRobotWinLostRecord;           // 场次机器人输赢记录
typedef unordered_map<unsigned int, ArenaRobotWinLostRecord> PlayModeRobotWinLostRecord;    // 不同模式场次机器人输赢记录

}

#endif // TYPE_DEFINE_H
