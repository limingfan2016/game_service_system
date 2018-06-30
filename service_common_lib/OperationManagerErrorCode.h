
/* author : limingfan
 * date : 2017.09.11
 * description : 运营管理服务错误码定义
 */
 
#ifndef __OPERATION_MANAGER_ERROR_CODE_H__
#define __OPERATION_MANAGER_ERROR_CODE_H__


// 运营管理服务错误码 4001 - 4999
enum EOperationManagerErrorCode
{
	BSVersionMobileOSTypeError = 4001,              // 版本检测手机类型错误
	BSVersionPlatformTypeError = 4002,              // 版本检测平台类型错误
	BSClientVersionInvalid = 4003,                  // 无效的客户端版本
	BSUsernameInputUnlegal = 4004,                  // 名字输入非法
	BSNicknameInputUnlegal = 4005,                  // 昵称输入非法
	BSPasswdInputUnlegal = 4006,                    // 密码输入非法
	BSAddrInputUnlegal = 4007,                      // 住址输入非法
	BSMobilephoneInputUnlegal = 4008,               // 手机号码输入非法
	BSEmailInputUnlegal = 4009,                     // email输入非法
	BSQueryPlatformInfoError = 4010,                // 查找注册平台信息错误
	BSExistPlatformInfoError = 4011,                // 该平台信息已经注册过了
	BSRegisterManagerError = 4012,                  // 注册管理员错误
	BSQueryManagerInfoError = 4013,                 // 查找管理员信息错误
	BSNonExistManagerNameError = 4014,              // 管理员名字不存在
	BSPasswordVerifyFailed = 4015,                  // 密码验证错误
	BSUserLimitLogin = 4016,                        // 用户被限制登录
	
	BSCreateChessHallManagerLevelLimit = 4017,      // 创建棋牌室权限限制
	BSAlreadyExistChessHall = 4018,                 // 已经存在棋牌室
	BSChessHallNameInputUnlegal = 4019,             // 棋牌室名称输入非法
	BSChessHallDescInputUnlegal = 4020,             // 棋牌室描述输入非法
	BSAddChessHallError = 4021,                     // 添加棋牌室错误
	BSUpdateChessHallIDError = 4022,                // 刷新棋牌室ID错误
	
	BSGetHallInfoError = 4023,                      // 获取棋牌室信息错误
	BSNoFoundHallInfo = 4024,                       // 找不到棋牌室信息
	BSGetHallPlayerInfoError = 4025,                // 获取棋牌室玩家信息错误
	BSUpdateHallPlayerInfoError = 4026,             // 刷新棋牌室玩家信息错误
	BSNoFoundGameInfo = 4027,                       // 找不到游戏信息
	BSAddNewGameError = 4028,                       // 添加新游戏错误
	BSUpdateGameStatusError = 4029,                 // 刷新游戏状态错误
	
	BSGetHallPlayerStatusError = 4030,              // 获取棋牌室玩家状态错误
	BSForbidPlayerError = 4031,                     // 该玩家已被棋牌室禁止
	BSPlayerAlreadyInHallError = 4032,              // 玩家已经在棋牌室中了
	BSSetHallPlayerStatusError = 4033,              // 设置棋牌室玩家状态错误
	BSSetPlayerCheckFinishError = 4034,             // 设置棋牌室玩家审核结束状态错误
	BSApplyJoinMsgLargeError = 4035,                // 申请加入提示信息过大
	BSCancelApplyChessHallError = 4036,             // 取消申请棋牌室错误
	BSApplyJoinHallIdParamError = 4037,             // 申请加入棋牌室ID参数错误
	BSQueryChessHallIdError = 4038,                 // 查询棋牌室ID错误
	
	BSCreateRoomInvalidParam = 4039,                // 创建房间参数无效
	BSSerializeRoomInfoError = 4040,                // 房间信息编码错误
	BSFormatRoomInfoError = 4041,                   // 格式化房间信息错误
	BSAddHallGameRoomError = 4042,                  // 添加棋牌室游戏房间错误
	
	BSChessHallIdParamError = 4043,                 // 棋牌室ID参数错误
	BSQueryMallOptSumValueError = 4044,             // 查询商城操作总额错误
	BSQueryMallInfoError = 4045,                    // 查询商城信息错误
	BSUpdateMallOptStatusError = 4046,              // 刷新商城操作状态错误
	BSPlayerInitGoldParamError = 4047,              // 设置玩家初始化金币参数错误
	BSUpdatePlayerInitGoldError = 4048,             // 刷新棋牌室玩家初始化金币错误
	
	BSQueryMallPlayerBuyInfoError = 4049,           // 查询商城玩家购买信息错误
	BSGetMallGoodsInfoError = 4050,                 // 获取商城物品信息错误
	BSParseBuyMallGoodsInfoError = 4051,            // 解析购买商城物品信息错误
	BSUpdateChessHallGoldError = 4052,              // 刷新棋牌室金币错误
	BSWriteOptMallGoodsRecordError = 4053,          // 操作商城物品写记录错误
	
	BSGiveGoodsTypeInvalid = 4054,                  // 赠送物品类型无效
	BSGiveGoodsParamInvalid = 4055,                 // 赠送物品参数无效
	BSChessHallGoodsInsufficient = 4056,            // 棋牌室物品不足
	BSGiveGoodsUsernameInvalid = 4057,              // 赠送物品的目标玩家ID无效
	
	BSQueryPlayerInfoError = 4058,                  // 查询玩家信息错误
	BSDBNoFoundPlayerInfoError = 4059,              // DB中没有玩家信息
	
	BSInvalidParameter = 4060,                      // 参数无效
	BSPortraitIdInputUnlegal = 4061,                // 输入头像ID非法
	BSGetUserMaxIdFailed = 4062,                    // 获取用户最大ID错误
	
	BSInvalidMobilePhoneNumber = 4063,              // 无效的手机号码
	BSInvalidPhoneVerificationCode = 4064,          // 无效的手机验证码
	
	BSNoFoundGameData = 4065,                       // 找不到游戏数据
	BSInvalidGameTaxRatio = 4066,                   // 无效的游戏服务税收比例
	BSUpdateGameTaxRatioError = 4067,               // 刷新游戏服务税收比例错误
	
	BSQueryManagerIdError = 4068,                   // 查询棋牌室管理员ID错误
	BSGetManagerIdError = 4069,                     // 获取棋牌室管理员ID错误
	BSCheckManagerInfoError = 4070,                 // 检查棋牌室管理员信息错误
	BSQueryApplyJoinPlayerInfoError = 4071,         // 查询棋牌室待审核玩家信息错误
	
	BSManagerLevelLimit = 4072,                     // 棋牌室管理员权限限制
	BSChessHallContactNameInputUnlegal = 4073,      // 棋牌室联系人姓名输入非法
	BSModifyChessHallInfoParamInvalid = 4074,       // 修改棋牌室信息参数无效
	BSModifyChessHallInfoError = 4075,              // 修改棋牌室信息错误
};


#endif // __OPERATION_MANAGER_ERROR_CODE_H__

