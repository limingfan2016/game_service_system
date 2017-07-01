
/* author : limingfan
 * date : 2015.08.6
 * description : 大厅逻辑服务实现
 */

#ifndef CHALL_LOGIC_HANDLER_H
#define CHALL_LOGIC_HANDLER_H

#include "base/MacroDefine.h"
#include "SrvFrame/CGameModule.h"
#include "TypeDefine.h"


using namespace NFrame;

namespace NPlatformService
{

class CSrvMsgHandler;


// 大厅逻辑处理
class CHallLogicHandler : public NFrame::CHandler
{
public:
	CHallLogicHandler();
	~CHallLogicHandler();
	
private:
    // 物品兑换
    void exchangePhoneCard(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void exchangePhoneCardReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void exchangeGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void exchangeGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void exchangePhoneFareReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void exchangePhoneFareRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 破产补助免费赠送金币
    void getNoGoldFreeGive(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void receiveNoGoldFreeGive(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void addAllowanceGoldReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 游戏中用户VIP等级变更
	void gameUserVIPLvUpdateNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 绑定邮箱
	void addEmailBox(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void addEmailBoxReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 用户个人财务变更通知，如：金币变化、道具变化、奖券变化等
	void personPropertyNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 停止游戏服务通知
	void stopGameServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 弹出海外广告请求
	void popOverseasNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 排行榜
	void viewRankingList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void viewGoldRankingReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void viewPhoneRankingReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取鱼类的倍率分值
	void getFishRateScore(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 版本热更新
    void checkVersionHotUpdate(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 系统消息已读通知
	void readSystemMsgNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 比赛场正在进行中通知，掉线重连，重入游戏等通知
	void arenaMatchOnNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 用户断线重连，重新登录游戏后取消比赛通知
	void cancelArenaMatchNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取象棋服务信息
	void getXiangQiServiceInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取新手指引信息
	void getNewPlayerGuideInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 新手引导流程已经完毕则通知服务器
	void finishNewPlayerGuideNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取小游戏服务信息
    void getGameServiceInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 韩文版HanYou平台使用优惠券通知
	void useHanYouCouponNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 韩文版HanYou平台使用优惠券获得的奖励通知
	void addHanYouCouponRewardNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 自动积分兑换物品
	void autoScoreExchangeItem(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void autoScoreExchangeItemReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);


	// Google SDK 操作
private:
    void getRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getRechargeTransactionReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void checkRechargeResult(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void checkRechargeResultReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void overseasRechargeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 第三方 SDK 操作
private:
    void getThirdPlatformRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getThirdPlatformRechargeTransactionReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void thirdPlatformRechargeTransactionNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void cancelThirdPlatformRechargeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 检查第三方账号
    void checkThirdUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void checkThirdUserReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    // 检查小米账号
    void checkXiaoMiUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void checkXiaoMiUserReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	//查询腾讯充值是否成功
	void findRechargeSuccess(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void findRechargeSuccessReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);


private:
    void getNoGoldFreeGiveInfo(ConnectProxy* conn, unsigned int srcSrvId, bool isActiveNotify = false);
	void receiveNoGoldFreeGiveHandle(ConnectProxy* conn, unsigned int srcSrvId);

private:
	ConnectProxy* getConnectInfo(const char* logInfo, string& userName);

	//获取分享奖励的下标
	unsigned int getShareRewardIndex(com_protocol::HallLogicData* hallData);
	
public:
	void load(CSrvMsgHandler* msgHandler);
	void unLoad(CSrvMsgHandler* msgHandler);
	
	void onLine(ConnectUserData* ud, ConnectProxy* conn);
	void offLine(com_protocol::HallLogicData* hallLogicData);
	
private:
	CSrvMsgHandler* m_msgHandler;


DISABLE_COPY_ASSIGN(CHallLogicHandler);
};


}

#endif // CHALL_LOGIC_HANDLER_H
