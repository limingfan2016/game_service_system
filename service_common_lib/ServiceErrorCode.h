
/* author : limingfan
 * date : 2016.10.11
 * description : 各服务错误码定义
 */
 
#ifndef __SERVICE_ERROR_CODE_H__
#define __SERVICE_ERROR_CODE_H__


// 捕鱼服务错误码 4001 - 4999
enum EServiceErrorCode
{
	ServiceOptFailed = -1,					//操作失败
	ServiceSuccess = 0,						//成功
	ServiceSessionKeyVerifyFailed = 4001,	//SessionKey校验失败
	ServiceSeatnumOutOfRange = 4002,		//座位号超出范围
	ServiceSeatHasPeople = 4003,			//座位有人了
	ServiceGoldNotEnough = 4004,			//金币不足
	ServiceDstUserNotAgree = 4005,			//目标用户不同意
	ServiceFishListIsEmpty = 4006,			//鱼列表为空
	ServiceBulletNotExist = 4007,			//子弹不存在
	ServiceFishNotExist = 4008,				//鱼不存在
	ServiceFishKindNotMatching = 4009,		//鱼的种类不匹配
	ServiceCannotUsePropType = 4010,		//不能使用此道具类型
	ServiceMuteBulleting = 4011,			//中哑弹期间不能发射子弹
	ServiceLightIDNotExist = 4012,			//激光炮ID不存在
	ServiceFishListUnmatching = 4013,		//捕获鱼的列表不匹配
	ServicePropListUnmatching = 4014,		//捕获道具的列表不匹配
	ServicePropNotExist = 4015,				//道具不存在
	ServicePropKindNotMatching = 4016,		//道具的各类不匹配
	ServiceClientVersionConfigError = 4017,	//客户端版本配置错误
	ServiceClientVersionInvalid = 4018,		//客户端版本无效
	ServiceInsertOpinionFailed = 4019,		//插入反溃意见失败
	ServiceGoldMoreEnough = 4020,			//金币过多
	ServiceSeatCountMoreEnough = 4021,		//满员了
	ServiceBulletNotZero = 4022,			//子弹倍率不能为0
	ServiceBulletMoreEnough = 4023,			//子弹倍率过高
	ServiceNotTaskReward = 4024,			//不存在任务奖励
	ServiceInvalidTaskReward = 4025,	    //无效的任务奖励
	ServiceBatteryLvOutOfRange = 4026,		//炮台等级超过有效范围
	ServiceDiamondsNotEnought = 4027,		//钻石不足
	ServiceBulletError = 4028,				//子弹倍率不正确
	SpecialFishNotCatchOtherFish = 4029,	//特殊鱼没有捕获其他鱼
	ServiceFishNotActivityProduce = 4030,	//话费鱼不是活动产生的
	ServicePhoneBillMoreEnough = 4031,		//话费已达到最大上限
	ServiceBatterySkinNotOwned = 4032,		//该炮台皮肤未拥有
	ServiceBatterySkinNotWear = 4033,		//该炮台皮肤已是卸下状态
	ServiceBatterySkinWear = 4034,			//该炮台皮肤已是装上状态
	ServiceBatterySkinOwned = 4035,			//该炮台皮肤已拥有
	ServiceNotUsedRampage = 4036,			//没有使用狂暴
	ServiceLoginQueryUserInfo = 4037,       //登陆过程查询用户信息失败
	ServiceInvalidUserInfo = 4038,          //无效的用户信息
	ServiceRampageNotUserLockBullet = 4039,	//狂暴中不能使用锁定炮
	ServiceLockBulletNotUserRampage = 4040,	//锁定炮使用中不能使用狂暴
	ServiceUserNonExistent = 4041,	        //用户不存在
	ServiceJoinAnotherMatch = 4042,	        //已经参加了其他不同场次的比赛
	ServiceInvalidMatchTime = 4043,	        //无效的比赛时间
	ServiceMaxBulletLevel = 4044,	        //已达到最大炮台等级
	ServiceArenaMatchVipLimit = 4045,	    //非VIP用户不能参加比赛
	ServiceArenaMatchNotGold = 4046,	    //比赛金币不足
	ServiceMatchBulletRateLimit = 4047,	    //已达到比赛场最大炮台等级
	ServiceArenaMatchTimeLimit = 4048,	    //比赛时间限制，离比赛剩余多少时间内玩家不可以再进入
	
	// 捕鱼卡牌错误码开始值
	PokerCarkInvalidTime = 4301,            // 无效的捕鱼卡牌活动时间
	PokerCarkInvalidArena = 4302,           // 无效的捕鱼卡牌场次
	PokerCarkSignNotGold = 4303,            // 报名金币不足
	PokerCarkSignErrorBatteryLevel = 4304,  // 报名炮台等级限制
	PokerCarkSignAgainError = 4305,         // 重复报名错误
	PokerCarkSignNotFoundError = 4306,      // 找不到玩家的报名信息
	PokerCarkArenaConfigError = 4307,       // 找不到场次配置信息
	PokerCarkMatchNotFoundError = 4308,     // 找不到玩家的参赛信息
	PokerCarkBulletNotEnough = 4309,        // 子弹数量不足
	PokerCarkCountEnough = 4310,            // 选择的扑克牌数量已经足够了

	
	// PK场错误码开始值
	ServiceInvalidPKIndex = 4501,	        //无效的PK场索引值
	ServiceAddWagerFail = 4502,				//加注失败
	ServiceUsePropUpperLimit = 4503,		//该道具已达到使用上限
	ServiceUseNotAddWagerTime = 4504,		// 非加注时间
	ServiceNotGameTime = 4505,				// 非游戏时间不允许发弹
	ServiceNotFindWaivePlayer = 4506,		// 没有找到要弃权的玩家
	ServiceInvalidUser = 4507,				// 无效用户
	ServicePlayEnd = 4508,					// 游戏结束
};



#endif // __SERVICE_ERROR_CODE_H__

