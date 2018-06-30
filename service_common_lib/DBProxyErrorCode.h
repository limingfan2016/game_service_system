
/* author : limingfan
 * date : 2017.09.11
 * description : DB代理服务错误码定义
 */
 
#ifndef __DBPROXY_ERROR_CODE_H__
#define __DBPROXY_ERROR_CODE_H__


// DB代理服务错误码 2001 - 2999
enum EDBProxyErrorCode
{
	DBProxySetServiceDataError = 2001,              // 设置服务数据错误
	DBProxyParseFromArrayError = 2002,              // 解码错误
	DBProxySerializeToArrayError = 2003,            // 编码错误
	
	DBProxyPlatformIdUnlegal = 2004,                // 平台ID非法
	DBProxyGetMaxIdFailed = 2005,                   // 获取最大用户ID错误
	DBProxyUsernameInputUnlegal = 2006,             // 输入用户名非法
	DBProxyPasswdInputUnlegal = 2007,               // 输入密码非法
	DBProxyNicknameInputUnlegal = 2008,             // 输入昵称非法
	DBProxyPersonSignInputUnlegal = 2009,           // 输入个人签名非法
	DBProxyRealnameInputUnlegal = 2010,             // 输入真实姓名非法
	DBProxyPortraitIdInputUnlegal = 2011,           // 输入头像ID非法
	DBProxyQQnumInputUnlegal = 2012,                // 输入QQ号码非法
	DBProxyAddrInputUnlegal = 2013,                 // 输入住址非法
	DBProxyBirthdayInputUnlegal = 2014,             // 输入生日非法
	DBProxyMobilephoneInputUnlegal = 2015,          // 输入手机号码非法
	DBProxyEmailInputUnlegal = 2016,                // 输入email非法
	DBProxyIDCInputUnlegal = 2017,                  // 输入IDC非法
	DBProxyOtherContactUnlegal = 2018,              // 输入联系信息非法
	DBProxyRegisterStaticInfoFailed = 2019,         // 注册用户静态信息错误
	DBProxyRegisterDynamicInfoFailed = 2020,        // 注册用户动态信息错误
	DBProxyRegisterPlatformInfoFailed = 2021,       // 注册平台信息错误
	DBProxyNicknameSensitiveStr = 2022,             // 昵称存在敏感信息

	DBProxyGetUserInfoFromMemcachedFailed = 2023,   // 从内存DB中获取用户信息失败
	DBProxyUserLimitLogin = 2024,                   // 用户限制登录
	DBProxyIPLimitLogin = 2025,                     // IP限制登录
	DBProxyDeviceLimitLogin = 2026,                 // 设备限制登录
	DBProxyPasswordVerifyFailed = 2027,             // 密码校验失败
	DBProxySetDataToMemcachedFailed = 2028,         // 设置内存数据错误
	
	DBProxyGetServiceDataError = 2029,              // 获取服务数据错误
	DBProxyGetMoreServiceDataError = 2030,          // 获取多服务数据错误
	DBProxyDeleteServiceDataError = 2031,           // 删除服务数据错误
	
	DBProxyNotFoundChessHallError = 2032,           // 找不到棋牌室错误
	DBProxyQueryHallGameError = 2033,               // 查找棋牌室游戏错误
	DBProxyQueryHallOnlineRoomError = 2034,         // 查找棋牌室在线房间错误
	DBProxyQueryOnlinePlayerError = 2035,           // 查找棋牌室在线玩家错误
	DBProxyNotFoundOnlinePlayerError = 2036,        // 找不到在线玩家
	DBProxyGetHallRankingDataError = 2037,          // 获取排名数据错误

	DBProxyQueryHallGameRoomInfoError = 2038,       // 查询棋牌室游戏房间信息错误
	DBProxyNoFoundGameRoomInfo = 2039,              // 找不到棋牌室游戏房间信息
	DBProxyParseRoomInfoError = 2040,               // 房间信息解码错误
	
	DBProxyInvalidTypeGetUserData = 2041,           // 获取不支持的用户数据
	DBProxyGoodsTypeInvalid = 2042,                 // 无效的物品类型
	DBProxyGoodsNotEnought = 2043,                  // 物品数量不足
	DBProxyAddGoodsChangeRecordError = 2044,        // 添加物品数量变更记录失败
	DBProxyNoGoodsForChangeError = 2045,            // 没有可变更的物品
	
	DBProxyQueryMallGiveGoodsInfoError = 2046,      // 查询商城赠送物品信息错误
	
	DBProxyChessHallIdParamError = 2047,            // 棋牌室ID参数错误
	DBProxyApplyJoinMsgLargeError = 2048,           // 申请加入提示信息过大
	DBProxyQueryChessHallIdError = 2049,            // 查询棋牌室ID错误
	DBProxyGetHallPlayerStatusError = 2050,         // 获取棋牌室玩家状态错误
	DBProxyForbidPlayerError = 2051,                // 该玩家已被棋牌室禁止
	DBProxyPlayerAlreadyInHallError = 2052,         // 玩家已经在棋牌室中了
	DBProxySetHallPlayerStatusError = 2053,         // 设置棋牌室玩家状态错误

	DBProxyQueryUserHallInfoError = 2054,           // 查询玩家所在棋牌室信息错误
	DBProxyGetUserHallInfoError = 2055,             // 获取玩家所在棋牌室信息错误
	DBProxyGetUserInfoFromDBError = 2056,           // 从DB中获取棋牌室玩家信息错误
	DBProxySetUserInfoToMemcachedError = 2057,      // 设置棋牌室玩家信息到内存DB错误
	
	DBProxyUpdateHallPlayerInfoError = 2058,        // 刷新棋牌室玩家信息错误

	DBProxyCancelApplyChessHallError = 2059,        // 取消申请棋牌室错误

	DBProxyCreateRoomInvalidParam = 2060,           // 创建房间参数无效
	DBProxySerializeRoomInfoError = 2061,           // 房间信息编码错误
	DBProxyFormatRoomInfoError = 2062,              // 格式化房间信息错误
	DBProxyAddHallGameRoomError = 2063,             // 添加棋牌室游戏房间错误
	
	DBProxyCreateRoomGoldInsufficient = 2064,       // 创建棋牌室游戏房间金币不足
	
	DBProxyUpdateDataToMysqlFailed = 2065,          // 刷新数据到DB错误
	DBProxyQueryInvitationPlayerError = 2066,       // 查找邀请玩家信息错误
	DBProxyQueryInvitationRoomInfoError = 2067,     // 查找邀请玩家所在的房间信息错误
	
	DBProxyGetUserStaticInfoFailed = 2068,          // 获取用户静态信息失败
	DBProxyGetUserDynamicInfoFailed = 2069,         // 获取用户动态信息失败
	
	DBProxyInvalidParameter = 2070,                 // 无效参数
	
	DBProxyUpdateMallOptStatusError = 2071,         // 刷新商城操作状态错误
	
	DBProxyUpdateChessHallGoodsError = 2072,        // 刷新棋牌室物品错误
	DBProxyWriteOptMallGoodsRecordError = 2073,     // 操作商城物品写记录错误
	
	DBProxyGetMallGoodsInfoError = 2074,            // 获取商城物品信息错误
	DBProxyQueryMallPlayerBuyInfoError = 2075,      // 查询商城玩家购买信息错误
    
    DBProxyQueryGameRecordInfoError = 2076,         // 查询游戏记录信息错误
    DBProxyQueryDetailedGameRecordError = 2077,     // 查询详细游戏记录错误
    DBProxyQueryGameRecordPlayerInfoError = 2078,   // 查询游戏记录玩家信息错误
    DBProxyGetGameRecordPlayerInfoError = 2079,     // 获取游戏记录玩家信息错误
    
    DBProxyInvalidUserAttributeType = 2080,         // 无效的用户属性类型
    
    DBProxyQueryUserRoomInfoError = 2081,           // 查询用户房间信息错误
    DBProxyUserCreateMoreNoFinishRoom = 2082,       // 用户创建了过多的没有使用完毕的房间
    DBProxyQueryHallGameInfoError = 2083,           // 查询大厅游戏信息错误
    DBProxyNoFoundHallGameInfoError = 2084,         // 找不到大厅游戏信息
	
	DBProxyAddGoodsFormatRecordInfoError = 2085,    // 格式化物品数量变更记录信息错误
	
	DBProxyCreateHallRechargeInfoError = 2086,      // 创建棋牌室充值信息错误
	DBProxyManagerRechargeGoodsError = 2087,        // 棋牌室管理员充值错误
	DBProxyWriteManagerRechargeRecordError = 2088,  // 管理员充值写记录错误
	
	DBProxyNoFoundRoomGameInfo = 2089,              // 找不到棋牌室房间里对于的游戏信息
	
	DBProxyQueryUserPlatformInfoError = 2090,       // 查询用户所在的平台信息错误
	DBProxyGetUserPlatformInfoError = 2091,         // 获取用户所在的平台信息错误
	DBProxyQueryUserManageLevelError = 2092,        // 查询用户管理员级别错误
	
	DBProxyQueryManagerPlatformInfoError = 2093,    // 获取管理员所在的平台信息错误
	DBProxyQueryManagerIdError = 2094,              // 查询棋牌室管理员ID错误
	
	DBProxyCreateRoomCardInsufficient = 2095,       // 创建棋牌室游戏房间房卡数量不足
	
	DBProxyGenerateRoomGameRecordSqlError = 2096,   // 生成房间游戏记录sql语句错误
	DBProxyAddRoomGameRecordError = 2097,           // 添加房间游戏记录错误
	
	DBProxyQueryGameRoomError = 2098,               // 查询游戏房间失败
	DBProxyNoFoundGameRoomError = 2099,             // 找不到棋牌室游戏房间
	DBProxyGameRoomStatusError = 2100,              // 游戏房间状态错误
	
	DBProxyQueryRoomGameRecordError = 2101,         // 查询房间游戏记录信息错误
	DBProxyQueryRoomRecordPlayerError = 2102,       // 查询房间游戏记录玩家信息错误
	DBProxyGetRoomRecordPlayerError = 2103,         // 获取房间游戏记录玩家信息错误
	DBProxyQueryDetailedRoomRecordError = 2104,     // 查询房间游戏详细记录错误
	
	DBProxyUpdateUserStaticInfoFailed = 2105,       // 刷新用户静态信息失败
	DBProxyUpdateUserDynamicInfoFailed = 2106,      // 刷新用户动态信息失败
	
	DBProxyBuyGoldRoomCardInsufficient = 2107,      // 购买金币房卡数量不足
	DBProxyAddGoodsExchangeRecordError = 2108,      // 添加物品兑换记录错误
};


#endif // __DBPROXY_ERROR_CODE_H__

