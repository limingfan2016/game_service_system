
/* author : limingfan
 * date : 2017.11.15
 * description : 牛牛游戏服务错误码定义
 */
 
#ifndef __CATTLES_GAME_ERROR_CODE_H__
#define __CATTLES_GAME_ERROR_CODE_H__


// 牛牛游戏服务错误码 11001 - 11999
enum ECattlesErrorCode
{
	CattlesGetSessionKeyError = 11001,         // 获取会话数据错误
	CattlesCheckSessionKeyFailed = 11002,      // 验证会话数据失败
	CattlesSetUserSessionDataFailed = 11003,   // 设置用户会话数据失败
	CattlesNotFoundRoomData = 11004,           // 找不到房间信息
	CattlesRoomSeatInUse = 11005,              // 房间座位已经有人
	CattlesInvalidSeatID = 11006,              // 无效的房间座位号
	CattlesRoomSeatFull = 11007,               // 房间座位满人了
	CattlesNotRoomUserInfo = 11008,            // 找不到房间用户信息
	CattlesGameStatusInvalidOpt = 11009,       // 游戏状态错误，操作不允许
	CattlesPlayerStatusInvalidOpt = 11010,     // 玩家状态错误，操作不允许
	CattlesBaseConfigError = 11011,            // 基础配置错误
	CattlesInvalidChoiceBetValue = 11012,      // 无效的下注额度
	CattlesGameGoldInsufficient = 11013,       // 游戏金币不足
	CattlesRepeateEnterRoom = 11014,           // 同一条连接，重复进入房间
	CattlesGameAlreadyFinish = 11015,          // 游戏已经结束
	CattlesNotInPlay = 11016,                  // 非游戏中玩家
	CattlesPlayerNoSeat = 11017,               // 该玩家没有座位号
    CattlesChoiceBankerGoldError = 11018,      // 玩家抢庄金币不足
	CattlesNoHallCheckOKUser = 11019,          // 不是棋牌室审核通过的玩家
	CattlesCurrentNoBanker = 11020,            // 当前没有满足当庄的玩家
	CattlesPrivateRoomCanNotBeEnter = 11021,   // 私人房间不可以直接进入，需要被邀请才能进入
	CattlesRoomCanNotBeView = 11022,           // 当前房间不可以旁观
	CattlesOnPlayCanNotChangeRoom = 11023,     // 玩家游戏中，不可以更换房间
	CattlesOnPlayCanNotLeaveRoom = 11024,      // 玩家游戏中，不可以离开房间
    CattlesInvalidChessHallId = 11025,         // 无效的棋牌室ID
	
	CattlesRoomCardInsufficient = 11026,       // 房卡数量不足
	CattlesNotRoomCreator = 11027,             // 不是房间的创建者
	CattlesNotAutoStartRoom = 11028,           // 不是自动开桌的房间
	CattlesNotManualStartRoom = 11029,         // 不是手动开桌的房间
	CattlesHasSitDownPlayer = 11030,           // 存在在座未准备的玩家
	CattlesStartPlayerInsufficient = 11031,    // 开桌玩家人数不足
	
	CattlesAskDismissRoomFrequently = 11032,   // 玩家频繁请求解散房间
};


#endif // __CATTLES_GAME_ERROR_CODE_H__

