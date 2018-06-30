
/* author : limingfan
 * date : 2017.09.11
 * description : 游戏大厅服务错误码定义
 */
 
#ifndef __GAME_HALL_ERROR_CODE_H__
#define __GAME_HALL_ERROR_CODE_H__


// 游戏大厅服务错误码 1001 - 1999
enum EGameHallErrorCode
{
	GameHallVersionMobileOSTypeError = 1001,         // 版本检测手机平台类型错误 
	GameHallVersionPlatformTypeError = 1002,         // 版本检测第三方平台类型错误
	GameHallClientVersionInvalid = 1003,             // 无效的客户端版本
	GameHallNoFoundConnectProxy = 1004,              // 找不到对应的连接代理
	GameHallSetSessionKeyFailed = 1005,              // 设置会话ID错误
	
	GameHallNoFoundChessHallInfo = 1006,             // 找不到棋牌室信息
	GameHallNoFoundGameInfo = 1007,                  // 找不到棋牌室游戏信息
	GameHallNoFoundGameServiceType = 1008,           // 找不到棋牌室游戏服务类型
	
	GameHallCreateRoomNoCheckAgreedPlayer = 1009,    // 非审核通过的玩家不能创建棋牌室房间
	GameHallCreateRoomGoldInsufficient = 1010,       // 创建棋牌室游戏房间金币不足
	
	GameHallNoFoundOnlineGameService = 1011,         // 找不到棋牌室在线游戏服务
    
    GameHallNoFoundPhoneNumberInfo = 1012,           // 找不到电话号码信息
    GameHallAlreadySetPhoneNumber = 1013,            // 已经绑定了该电话号码
    GameHallInvalidPhoneVerificationCode = 1014,     // 无效的手机验证码

	GameHallCreateGameRoomParamInvalid = 1015,       // 创建棋牌室游戏房间参数无效
	GameHallCreateRoomPlayerCountInvalid = 1016,     // 创建棋牌室游戏房间玩家人数无效
	GameHallCreateRoomGameTimesInvalid = 1017,       // 创建棋牌室游戏房间游戏局数无效
	GameHallCreateRoomPayModeInvalid = 1018,         // 创建棋牌室游戏房间付费模式无效
	GameHallCreateRoomStartModeInvalid = 1019,       // 创建棋牌室游戏房间开桌方式无效
	GameHallCreateRoomBaseRateInvalid = 1020,        // 创建棋牌室游戏房间底注（基础倍率）无效
};


#endif // __GAME_HALL_ERROR_CODE_H__

