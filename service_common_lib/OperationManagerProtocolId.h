
/* author : limingfan
 * date : 2017.09.11
 * description : 运营管理服务协议码ID定义
 */
 
#ifndef __OPERATION_MANAGER_PROTOCOL_ID_H__
#define __OPERATION_MANAGER_PROTOCOL_ID_H__


// 运营管理服务与商务客户端APP间的消息协议ID
enum EBusinessClientProtocolId
{
	// 客户端APP版本校验
	BSC_VERSION_CHECK_REQ = 1,
	BSC_VERSION_CHECK_RSP = 2,
	
	// 管理员注册
	BSC_MANAGER_REGISTER_REQ = 3,
	BSC_MANAGER_REGISTER_RSP = 4,
	
	// 管理员登录
	BSC_MANAGER_LOGIN_REQ = 5,
	BSC_MANAGER_LOGIN_RSP = 6,
	
	// 管理员登出
	BSC_MANAGER_LOGOUT_NOTIFY = 7,
	
	// 关闭重复的连接
	BSC_CLOSE_REPEATE_CONNECT_NOTIFY = 8,
	
	// 创建棋牌室
	BSC_CREATE_CHESS_HALL_REQ = 9,
	BSC_CREATE_CHESS_HALL_RSP = 10,
	
	// 获取棋牌室信息
	BSC_GET_HALL_INFO_REQ = 11,
	BSC_GET_HALL_INFO_RSP = 12,
	
	// 获取棋牌室里玩家信息
	BSC_GET_PLAYER_INFO_REQ = 13,
	BSC_GET_PLAYER_INFO_RSP = 14,
	
	// 操作棋牌室玩家
	BSC_OPT_HALL_PLAYER_REQ = 15,
	BSC_OPT_HALL_PLAYER_RSP = 16,
	
	// 获取棋牌室游戏信息
	BSC_GET_HALL_GAME_INFO_REQ = 17,
	BSC_GET_HALL_GAME_INFO_RSP = 18,
	
	// 添加新游戏
	BSC_ADD_HALL_NEW_GAME_REQ = 19,
	BSC_ADD_HALL_NEW_GAME_RSP = 20,
	
	// 新增加管理员
	BSC_ADD_MANAGER_REQ = 21,
	BSC_ADD_MANAGER_RSP = 22,
	
	// 玩家申请加入棋牌室服务器通知客户端
	BSC_PLAYER_APPLY_JOIN_HALL_NOTIFY = 23,
	
	// 管理员把玩家踢下线客户端通知服务器
	BSC_PLAYER_QUIT_CHESS_HALL_NOTIFY = 24,

	// 获取商城数据
	BSC_GET_MALL_DATA_REQ = 25,
	BSC_GET_MALL_DATA_RSP = 26,
	
	// 获取商城购买信息
	BSC_GET_MALL_BUY_INFO_REQ = 27,
	BSC_GET_MALL_BUY_INFO_RSP = 28,
	
	// 获取商城赠送信息
	BSC_GET_MALL_GIVE_INFO_REQ = 29,
	BSC_GET_MALL_GIVE_INFO_RSP = 30,
	
	// 管理员确认购买信息
	BSC_CONFIRM_BUY_INFO_REQ = 31,
	BSC_CONFIRM_BUY_INFO_RSP = 32,
	
	// 管理员赠送礼物
	BSC_GIVE_GOODS_REQ = 33,
	BSC_GIVE_GOODS_RSP = 34,
	
	// 管理员设置玩家初始化物品
	BSC_SET_PLAYER_INIT_GOODS_REQ = 35,
	BSC_SET_PLAYER_INIT_GOODS_RSP = 36,
	
	// 管理员获取游戏玩家信息
	BSC_GET_GAME_PLAYER_DATA_REQ = 37,
	BSC_GET_GAME_PLAYER_DATA_RSP = 38,
	
	// 无效连接、未验证通过的连接、重复频繁发消息、作弊、服务重启
	// 踢玩家下线、同一条连接重复登录、非法消息 等等则通知客户端玩家退出游戏
	// 客户端收到此消息，玩家必须退出服务
	// 服务器发送该消息之后，主动关闭连接
	BSC_FORCE_USER_QUIT_NOTIFY = 39,
	
	// 创建棋牌室，验证管理员输入的手机号码
	BSC_CHECK_PHONE_NUMBER_REQ = 40,
	BSC_CHECK_PHONE_NUMBER_RSP = 41,
	
	// 管理员充值通知
	BSC_MANAGER_RECHARGE_NOTIFY = 42,
	
	// 设置棋牌室游戏税收服务比例
	BSC_SET_HALL_GAME_TAX_RATIO_REQ = 43,
	BSC_SET_HALL_GAME_TAX_RATIO_RSP = 44,
	
	// C端管理员操作棋牌室玩家通知B端
	BSC_MANAGER_OPT_HALL_PLAYER_NOTIFY = 45,
	
	// 修改棋牌室信息
	BSC_MODIFY_HALL_INFO_REQ = 46,
	BSC_MODIFY_HALL_INFO_RSP = 47,
};


// 运营管理服务协议ID
enum EOperationManagerProtocolId
{
	// 各服务上下线&停止等状态通知
	OPTMGR_SERVICE_STATUS_NOTIFY = 1,
	
	// 用户上下线通知
	OPTMGR_USER_ONLINE_NOTIFY = 2,
	OPTMGR_USER_OFFLINE_NOTIFY = 3,
	
	// 消息记录
	OPTMGR_MSG_RECORD_NOTIFY = 4,
	
	// 棋牌室玩家状态通知
	OPTMGR_CHESS_HALL_PLAYER_STATUS_NOTIFY = 5,
	
	// 棋牌室玩家被管理员踢下线通知
	OPTMGR_PLAYER_QUIT_CHESS_HALL_NOTIFY = 6,
	
	// 玩家申请加入棋牌室通知管理员
	OPTMGR_PLAYER_APPLY_JOIN_HALL_NOTIFY = 7,
	
	// 玩家购买商城物品通知管理员
	OPTMGR_PLAYER_BUY_MALL_GOODS_NOTIFY = 8,
	
	// C端管理员获取待审核的玩家信息
	OPTMGR_GET_APPLY_JOIN_PLAYERS_REQ = 9,
	OPTMGR_GET_APPLY_JOIN_PLAYERS_RSP = 10,
	
	// C端管理员操作棋牌室玩家
	OPTMGR_OPT_HALL_PLAYER_REQ = 11,
	OPTMGR_OPT_HALL_PLAYER_RSP = 12,
};


// 服务自定义的应答协议ID值
// 其值必须大于 MaxProtocolIDCount = 8192
enum EServiceOperationManagerReplyProtocolId
{
	OPTMGR_CUSTOM_MIN_PROTOCOL_ID = 30000,                  // 服务自定义应答协议最小ID值
	
	OPTMGR_CUSTOM_OPT_HALL_PLAYER_RESULT = 30001,           // C端管理员操作棋牌室玩家应答结果
};


#endif // __OPERATION_MANAGER_PROTOCOL_ID_H__

