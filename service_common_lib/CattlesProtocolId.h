
/* author : limingfan
 * date : 2017.11.15
 * description : 牛牛游戏服务协议码ID定义
 */
 
#ifndef __CATTLES_GAME_PROTOCOL_ID_H__
#define __CATTLES_GAME_PROTOCOL_ID_H__


// 牛牛游戏与客户端间的消息协议ID
enum EClientCattlesProtocolId
{
	// 所有游戏与客户端间的自定义消息协议ID值从 1001 开始
	// 前 1000 位值由公共协议 EClientGameCommonProtocolId 占用

	// 准备按钮状态通知（准备按钮是否可以点击）
	CATTLES_PREPARE_STATUS_NOTIFY = 1001,

	// 确定庄家通知客户端
	CATTLES_CONFIRM_BANKER_NOTIFY = 1002,
	
	// 等待客户端表现完毕，之后才能通知玩家开始下注
	CATTLES_CLIENT_PERFORM_FINISH_NOTIFY = 1003,

	// 通知玩家开始下注
	CATTLES_NOTIFY_PLAYER_CHOICE_BET = 1004,
	
	// 玩家下注
	CATTLES_PLAYER_CHOICE_BET_REQ = 1005,
	CATTLES_PLAYER_CHOICE_BET_RSP = 1006,
	CATTLES_PLAYER_CHOICE_BET_NOTIFY = 1007,
	
	// 发牌给玩家
	CATTLES_PUSH_CARD_TO_PLAYER_NOTIFY = 1008,
	
	// 玩家亮牌
	CATTLES_PLAYER_OPEN_CARD_REQ = 1009,
	CATTLES_PLAYER_OPEN_CARD_RSP = 1010,
	CATTLES_PLAYER_OPEN_CARD_NOTIFY = 1011,
	
	// 结算消息通知玩家
	CATTLES_SETTLEMENT_NOTIFY = 1012,
	
	// 玩家下注超时通知
	CATTLES_PLAYER_CHOICE_BET_TIME_OUT_NOTIFY = 1013,

	// 通知玩家开始抢庄
	CATTLES_COMPETE_BANKER_NOTIFY = 1014,

	// 玩家抢庄动作（抢庄或者不抢庄）
	CATTLES_PLAYER_CHOICE_BANKER_REQ = 1015,
	CATTLES_PLAYER_CHOICE_BANKER_RSP = 1016,
	CATTLES_PLAYER_CHOICE_BANKER_NOTIFY = 1017,
	
	// 通知玩家可以手动开始游戏（房卡手动开桌）
	CATTLES_MANUAL_START_GAME_NOTIFY = 1018,
};


// 服务内部消息协议ID，值从 1001 开始
// 前 1000 位值由公共协议 EServiceCommonProtocolId 占用
enum EServiceCattlesProtocolId
{
};


// 服务自定义的应答协议ID值
// 其值必须大于 MaxProtocolIDCount = 8192
enum EServiceCattlesReplyProtocolId
{
	CATTLES_MIN_PROTOCOL_ID = 30000,                  // 服务自定义应答协议最小ID值

	CATTLES_GET_USER_INFO_FOR_ENTER_ROOM = 30001,     // 用户进入房间获取个人信息
	CATTLES_GET_ROOM_INFO_FOR_ENTER_ROOM = 30002,     // 用户进入房间获取房间信息
};


#endif // __CATTLES_GAME_PROTOCOL_ID_H__

