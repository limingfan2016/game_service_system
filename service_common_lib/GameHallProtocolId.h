
/* author : limingfan
 * date : 2017.09.11
 * description : 游戏大厅服务协议码ID定义
 */
 
#ifndef __GAME_HALL_PROTOCOL_ID_H__
#define __GAME_HALL_PROTOCOL_ID_H__


// 游戏大厅与客户端间的消息协议ID
enum EClientGameHallProtocolId
{
	// 客户端APP版本校验
	CGH_VERSION_CHECK_REQ = 1,
	CGH_VERSION_CHECK_RSP = 2,
	
	// 用户登录游戏
	CGH_USER_LOGIN_REQ = 3,
	CGH_USER_LOGIN_RSP = 4,
	
	// 用户登出游戏
	CGH_USER_LOGOUT_REQ = 5,
	CGH_USER_LOGOUT_RSP = 6,
	
	// 关闭重复的连接
	CGH_CLOSE_REPEATE_CONNECT_NOTIFY = 7,
	
	// 获取棋牌室信息
	CGH_GET_CHESS_HALL_INFO_REQ = 8,
	CGH_GET_CHESS_HALL_INFO_RSP = 9,
	
	// 获取棋牌室游戏房间信息
	CGH_GET_GAME_ROOM_INFO_REQ = 10,
	CGH_GET_GAME_ROOM_INFO_RSP = 11,
	
	// 申请加入棋牌室
	CGH_APPLY_JOIN_CHESS_HALL_REQ = 12,
	CGH_APPLY_JOIN_CHESS_HALL_RSP = 13,
	
	// 取消申请加入棋牌室
	CGH_CANCEL_APPLY_CHESS_HALL_REQ = 14,
	CGH_CANCEL_APPLY_CHESS_HALL_RSP = 15,
	
	// 管理员设置玩家状态结果通知（审核通过、拒绝、禁止、取消禁止等等）
	CGH_CHESS_HALL_PLAYER_STATUS_NOTIFY = 16,
	
	// 棋牌室玩家被管理员踢下线通知
	CGH_PLAYER_QUIT_CHESS_HALL_NOTIFY = 17,

	// 玩家创建棋牌室游戏房间
	CGH_CREATE_HALL_GAME_ROOM_REQ = 18,
	CGH_CREATE_HALL_GAME_ROOM_RSP = 19,
	
	// 获取游戏玩家信息
	CGH_GET_USER_DATA_REQ = 20,
	CGH_GET_USER_DATA_RSP = 21,
	
	// 刷新棋牌室游戏房间通知
	CGH_UPDATE_HALL_GAME_ROOM_NOTIFY = 22,
	
    // 获取商城数据
	CGH_GET_MALL_DATA_REQ = 23,
	CGH_GET_MALL_DATA_RSP = 24,
	
	// 玩家购买物品
	CGH_BUY_MALL_GOODS_REQ = 25,
	CGH_BUY_MALL_GOODS_RSP = 26,
	
	// 获取购买记录信息
	CGH_GET_MALL_BUY_RECORD_REQ = 27,
	CGH_GET_MALL_BUY_RECORD_RSP = 28,
	
	// 管理员给玩家赠送物品通知客户端
	// 玩家确认赠送物品，客户端通知服务器
	CGH_MANAGER_GIVE_GOODS_NOTIFY = 29,
	CGH_PLAYER_CONFIRM_GIVE_GOODS_NOTIFY = 30,
	
	// 无效连接、未验证通过的连接、重复频繁发消息、作弊、服务重启
	// 踢玩家下线、同一条连接重复登录、非法消息 等等则通知客户端玩家退出游戏
	// 客户端收到此消息，玩家必须退出服务
	// 服务器发送该消息之后，主动关闭连接
	CGH_FORCE_PLAYER_QUIT_NOTIFY = 31,
	
	// 棋牌室大厅玩家发送聊天消息
	CGH_PLAYER_SEND_CHAT_MSG_NOTIFY = 32,
	
	// 服务器向客户端玩家推送消息
	CGH_SERVICE_PUSH_MSG_NOTIFY = 33,
	
	// 获取棋牌室消息（玩家聊天、系统推送消息等）
	CGH_GET_HALL_MESSAGE_REQ = 34,
	CGH_GET_HALL_MESSAGE_RSP = 35,
	
	// 用户进入棋牌室
	CGH_USER_ENTER_HALL_REQ = 36,
	CGH_USER_ENTER_HALL_RSP = 37,
	
	// 获取棋牌室基本信息
	CGH_GET_HALL_BASE_INFO_REQ = 38,
	CGH_GET_HALL_BASE_INFO_RSP = 39,
	
	// 修改游戏玩家信息
	CGH_MODIFY_USER_DATA_REQ = 40,
	CGH_MODIFY_USER_DATA_RSP = 41,

	// 玩家收到通知邀请
	CGH_INVITE_PLAYER_PLAY_NOTIFY = 42,
	
	// 玩家拒绝邀请通知
	CGH_PLAYER_REFUSE_INVITE_NOTIFY = 43,
    
    // 获取游戏战绩信息
	CGH_GET_GAME_RECORD_INFO_REQ = 44,
	CGH_GET_GAME_RECORD_INFO_RSP = 45,
	
    // 获取详细游戏战绩信息
	CGH_GET_DETAILED_GAME_RECORD_REQ = 46,
	CGH_GET_DETAILED_GAME_RECORD_RSP = 47,
    
    // 获取棋牌室当前排名列表
	CGH_GET_HALL_RANKING_REQ = 48,
	CGH_GET_HALL_RANKING_RSP = 49,

    // 验证用户手机号码
	CGH_CHECK_USER_PHONE_NUMBER_REQ = 50,
	CGH_CHECK_USER_PHONE_NUMBER_RSP = 51,
    
    // 校验手机收到的验证码
	CGH_CHECK_MOBILE_CODE_REQ = 52,
	CGH_CHECK_MOBILE_CODE_RSP = 53,
	
	// 玩家申请加入棋牌室通知在线的管理员
	CGH_PLAYER_APPLY_JOIN_HALL_NOTIFY = 54,

	// 棋牌室管理员获取待审核的玩家列表
	CGH_MANAGER_GET_APPLY_JOIN_PLAYERS_REQ = 55,
	CGH_MANAGER_GET_APPLY_JOIN_PLAYERS_RSP = 56,

	// 棋牌室管理员操作玩家
	CGH_MANAGER_OPT_HALL_PLAYER_REQ = 57,
	CGH_MANAGER_OPT_HALL_PLAYER_RSP = 58,
	
	// 获取棋牌室玩家信息列表
	CGH_GET_HALL_PLAYER_INFO_REQ = 59,
	CGH_GET_HALL_PLAYER_INFO_RSP = 60,
	
	// 用户离开当前棋牌室
	CGH_USER_LEAVE_HALL_REQ = 61,
	CGH_USER_LEAVE_HALL_RSP = 62,
	
	// 查询游戏房间
	CGH_QUERY_GAME_ROOM_REQ = 63,
	CGH_QUERY_GAME_ROOM_RSP = 64,
	
	// 获取房间游戏战绩信息
	CGH_GET_ROOM_GAME_RECORD_REQ = 65,
	CGH_GET_ROOM_GAME_RECORD_RSP = 66,
	
    // 获取详细房间游戏战绩信息
	CGH_GET_DETAILED_ROOM_GAME_RECORD_REQ = 67,
	CGH_GET_DETAILED_ROOM_GAME_RECORD_RSP = 68,
	
	// 房卡购买金币
	CGH_ROOM_CARD_BUY_GOLD_REQ = 69,
	CGH_ROOM_CARD_BUY_GOLD_RSP = 70,
	
	// 系统消息通知
	CGH_SYSTEM_MESSAGE_NOTIFY = 71,
};


// 服务内部消息协议ID，值从 1001 开始
// 前 1000 位值由公共协议 EServiceCommonProtocolId 占用
enum EServiceGameHallProtocolId
{
};


// 服务自定义的应答协议ID值
// 其值必须大于 MaxProtocolIDCount = 8192
enum EServiceGameHallReplyProtocolId
{
	SGH_CUSTOM_MIN_PROTOCOL_ID = 30000,                  // 服务自定义应答协议最小ID值
	SGH_CUSTOM_GET_USER_INFO_ID = 30001,                 // 获取用户信息

    SGH_CUSTOM_GET_USER_PHONE_NUMBER = 30002,            // 获取用户手机号码
    SGH_CUSTOM_SET_USER_PHONE_NUMBER = 30003,            // 设置用户手机号码
};


#endif // __GAME_HALL_PROTOCOL_ID_H__

