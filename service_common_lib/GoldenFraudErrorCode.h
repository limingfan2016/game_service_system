
/* author : limingfan
 * date : 2018.03.29
 * description : 炸金花游戏服务错误码定义
 */
 
#ifndef __GOLDEN_FRAUD_GAME_ERROR_CODE_H__
#define __GOLDEN_FRAUD_GAME_ERROR_CODE_H__


// 炸金花游戏服务错误码 12301 - 12999
enum EGoldenFraudErrorCode
{
	GoldenFraudNotFoundPlayer = 12301,          // 找不到游戏玩家
	GoldenFraudAlreadyViewCard = 12302,         // 玩家已经看牌了
	GoldenFraudNotCurrentOptPlayer = 12303,     // 非当前操作玩家
	GoldenFraudGoldInsufficient = 12304,        // 下注金币不足
	GoldenFraudNotExistIncreaseBet = 12305,     // 不存在的加注额
	GoldenFraudIncreaseBetInvalid = 12306,      // 无效的加注额
	GoldenFraudNotGoldTypeAllIn = 12307,        // 非金币场没有全押
	GoldenFraudPlayerStatusInvalid = 12308,     // 玩家状态无效
	GoldenFraudRoundCanNotViewCard = 12309,     // 当前轮次不可看牌
	GoldenFraudRoundCanNotCompareCard = 12310,  // 当前轮次不可比牌
	GoldenFraudAllInLimit = 12311,              // 全押条件限制
};


#endif // __GOLDEN_FRAUD_GAME_ERROR_CODE_H__

