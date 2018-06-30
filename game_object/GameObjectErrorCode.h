
/* author : limingfan
 * date : 2018.03.30
 * description : 游戏服务基础错误码定义
 */
 
#ifndef __GAME_OJBECT_ERROR_CODE_H__
#define __GAME_OJBECT_ERROR_CODE_H__


// 游戏服务错误码 [1 - 300]
enum EGameObjectErrorCode
{
	GameObjectGetSessionKeyError = 1,          // 获取会话数据错误
	GameObjectCheckSessionKeyFailed = 2,       // 验证会话数据失败
	GameObjectSetUserSessionDataFailed = 3,    // 设置用户会话数据失败
	GameObjectNotFoundRoomData = 4,            // 找不到房间信息
	GameObjectRoomSeatInUse = 5,               // 房间座位已经有人
	GameObjectInvalidSeatID = 6,               // 无效的房间座位号
	GameObjectRoomSeatFull = 7,                // 房间座位满人了
	GameObjectNotRoomUserInfo = 8,             // 找不到房间用户信息
	GameObjectGameStatusInvalidOpt = 9,        // 游戏状态错误，操作不允许
	GameObjectPlayerStatusInvalidOpt = 10,     // 玩家状态错误，操作不允许
	GameObjectGameGoldInsufficient = 11,       // 游戏金币不足
	GameObjectRepeateEnterRoom = 12,           // 同一条连接，重复进入房间
	GameObjectGameAlreadyFinish = 13,          // 游戏已经结束
	GameObjectNotInPlay = 14,                  // 非游戏中玩家
	GameObjectPlayerNoSeat = 15,               // 该玩家没有座位号
	GameObjectNoHallCheckOKUser = 16,          // 不是棋牌室审核通过的玩家
	GameObjectCurrentNoBanker = 17,            // 当前没有满足当庄的玩家
	GameObjectPrivateRoomCanNotBeEnter = 18,   // 私人房间不可以直接进入，需要被邀请才能进入
	GameObjectRoomCanNotBeView = 19,           // 当前房间不可以旁观
	GameObjectOnPlayCanNotChangeRoom = 20,     // 玩家游戏中，不可以更换房间
	GameObjectOnPlayCanNotLeaveRoom = 21,      // 玩家游戏中，不可以离开房间
    GameObjectInvalidChessHallId = 22,         // 无效的棋牌室ID
	GameObjectRoomCardInsufficient = 23,       // 房卡数量不足
	GameObjectNotRoomCreator = 24,             // 不是房间的创建者
	GameObjectNotAutoStartRoom = 25,           // 不是自动开桌的房间
	GameObjectNotManualStartRoom = 26,         // 不是手动开桌的房间
	GameObjectStartPlayerInsufficient = 27,    // 开桌玩家人数不足
	GameObjectAskDismissRoomFrequently = 28,   // 玩家频繁请求解散房间
};


#endif // __GAME_OJBECT_ERROR_CODE_H__

