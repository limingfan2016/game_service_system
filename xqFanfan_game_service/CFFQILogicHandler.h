
/* author : limingfan
 * date : 2017.01.03
 * description : 翻翻棋逻辑处理
 */

#ifndef CFFQILOGIC_HANDLER_H
#define CFFQILOGIC_HANDLER_H

#include <string>
#include <unordered_map>
#include <map>
#include <vector>

#include "SrvFrame/CModule.h"
#include "common/CommonType.h"
#include "TypeDefine.h"


using namespace std;
using namespace NProject;


namespace NPlatformService
{

class CSrvMsgHandler;


// 逻辑处理
class CFFQILogicHandler : public NFrame::CHandler
{
public:
	CFFQILogicHandler();
	~CFFQILogicHandler();

public:
	bool load(CSrvMsgHandler* msgHandler);
	void unLoad();
	
	void updateConfig();
	
	void onLine(XQUserData* ud);
	void offLine(XQUserData* ud);
	
public:
    // 任务统计
	void doTaskStatistics(const XQChessPieces* xqCP, const short lostSide, const com_protocol::SettlementResult result, const char* username);
	
	// 充值成功通知
	void rechargeNotify(const char* username, const float money);
	
public:
    // 设置禁足信息
	unsigned int setRestrictMoveInfo(XQChessPieces* xqCP, const short moveSrcPosition, const short moveDstPosition);
	
	// 清空禁足信息
	void clearRestrictMoveInfo(SRestrictMoveInfo& rstMvInfo);

private:
    // 获取任务列表
	void getTaskInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 领取任务奖励
	void receiveTaskReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void receiveTaskRewardReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	// 游戏渔币&金币商城配置
	void getGoodsMallCfg(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getGoodsMallCfgReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 游戏道具商城配置
	void getFFChessGoodsMall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getFFChessGoodsMallReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void exchangeFFChessGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void exchangeFFChessGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void useFFChessGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void useFFChessGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	// 获取翻翻棋用户基本信息
	void getUserBaseInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getUserBaseInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void setUserMatchResultReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 游戏规则&说明信息
	void getRuleInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 游戏内同桌聊天
	void userChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void userChatNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	CSrvMsgHandler* m_msgHander;
	
	
DISABLE_COPY_ASSIGN(CFFQILogicHandler);
};

}

#endif // CFFQILOGIC_HANDLER_H

