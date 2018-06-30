
/* author : limingfan
 * date : 2018.03.29
 * description : 炸金花游戏服务协议码ID定义
 */
 
#ifndef __GOLDEN_FRAUD_GAME_PROTOCOL_ID_H__
#define __GOLDEN_FRAUD_GAME_PROTOCOL_ID_H__


// 炸金花游戏与客户端间的消息协议ID
enum EClientGoldenFraudProtocolId
{
	// 所有游戏与客户端间的自定义消息协议ID值从 1001 开始
	// 前 1000 位值由公共协议 EClientGameCommonProtocolId 占用
	
	// 游戏开始发牌给玩家
	GOLDENFRAUD_PUSH_CARD_NOTIFY = 1001,

	// 同步操作消息（弃牌、看牌、加注、跟注、比牌、全押）给其他玩家
	GOLDENFRAUD_PLAYER_OPT_NOTIFY = 1002,

	// 玩家弃牌
	GOLDENFRAUD_GIVE_UP_REQ = 1003,
	GOLDENFRAUD_GIVE_UP_RSP = 1004,

	// 玩家看牌
	GOLDENFRAUD_VIEW_CARD_REQ = 1005,
	GOLDENFRAUD_VIEW_CARD_RSP = 1006,

	// 玩家跟注跟到底
	GOLDENFRAUD_ALWAYS_BET_REQ = 1007,
	GOLDENFRAUD_ALWAYS_BET_RSP = 1008,
	
	// 玩家下注操作（跟注、加注、全押）
	GOLDENFRAUD_DO_BET_REQ = 1009,
	GOLDENFRAUD_DO_BET_RSP = 1010,

	// 玩家比牌
	GOLDENFRAUD_COMPARE_CARD_REQ = 1011,
	GOLDENFRAUD_COMPARE_CARD_RSP = 1012,
	
	// 比牌结果通知
	GOLDENFRAUD_COMPARE_CARD_RESULT_NOTIFY = 1013,
	
	// 结算通知
	GOLDENFRAUD_SETTLEMENT_NOTIFY = 1014,
};


// 服务内部消息协议ID，值从 1001 开始
// 前 1000 位值由公共协议 EServiceCommonProtocolId 占用
enum EServiceGoldFraudProtocolId
{
};


#endif // __GOLDEN_FRAUD_GAME_PROTOCOL_ID_H__

