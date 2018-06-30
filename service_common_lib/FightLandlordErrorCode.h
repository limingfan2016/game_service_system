
/* author : limingfan
 * date : 2018.04.19
 * description : 斗地主游戏服务错误码定义
 */
 
#ifndef __FIGHT_LANDLORD_GAME_ERROR_CODE_H__
#define __FIGHT_LANDLORD_GAME_ERROR_CODE_H__


// 斗地主游戏服务错误码 13301 - 13999
enum EFightLandlordErrorCode
{
	FightLandlordNotFoundPlayer = 13301,          // 找不到游戏玩家
	FightLandlordGameStatusInvalidOpt = 13302,    // 游戏状态错误，操作不允许
	FightLandlordPlayerStatusInvalid = 13303,     // 玩家状态无效
	FightLandlordPlayerOptInvalid = 13304,        // 无效的玩家操作
	FightLandlordNotCurrentOptPlayer = 13305,     // 非当前操作玩家
	FightLandlordGoldInsufficient = 13306,        // 金币不足
	FightLandlordPlayCardParamInvalid = 13307,    // 出牌参数错误
	FightLandlordPlayCardNotExist = 13308,        // 出了不存在的牌
	FightLandlordPlayCardTypeInvalid = 13309,     // 打出的牌型无效
	FightLandlordPlayCardValueSmall = 13310,      // 打出的牌值小，不能压制对方
	FightLandlordPlayCardCountInvalid = 13311,    // 打出的牌数目不对
	FightLandlordPlayCardTypeMismatch = 13312,    // 打出的牌型不匹配
};


#endif // __FIGHT_LANDLORD_GAME_ERROR_CODE_H__

