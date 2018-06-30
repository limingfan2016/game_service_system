
/* author : limingfan
 * date : 2018.04.19
 * description : 斗地主游戏服务协议码ID定义
 */
 
#ifndef __FIGHT_LANDLORD_GAME_PROTOCOL_ID_H__
#define __FIGHT_LANDLORD_GAME_PROTOCOL_ID_H__


// 斗地主游戏与客户端间的消息协议ID
enum EClientFightLandlordProtocolId
{
	// 所有游戏与客户端间的自定义消息协议ID值从 2001 开始
	// 前 2000 位值由公共协议 EClientGameCommonProtocolId 占用
	
	// 游戏开始发牌给玩家
	FIGHTLANDLORD_PUSH_CARD_NOTIFY = 2001,
	
	// 玩家操作（叫地主、抢地主、出牌、托管）
	FIGHTLANDLORD_PLAYER_OPT_REQ = 2002,
	FIGHTLANDLORD_PLAYER_OPT_RSP = 2003,
	FIGHTLANDLORD_PLAYER_OPT_NOTIFY = 2004,
	
	// 结算通知
	FIGHTLANDLORD_SETTLEMENT_NOTIFY = 2015,
};


// 服务内部消息协议ID，值从 1001 开始
// 前 1000 位值由公共协议 EServiceCommonProtocolId 占用
enum EServiceFightLandlordProtocolId
{
};


#endif // __FIGHT_LANDLORD_GAME_PROTOCOL_ID_H__

