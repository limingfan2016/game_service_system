
/* author : limingfan
* date : 2017.05.18
* description : 消息处理辅助类
*/
 
#ifndef __CLOGIC_HANDLER_THREE_H__
#define __CLOGIC_HANDLER_THREE_H__

#include <string>
#include "SrvFrame/CModule.h"
#include "app_server_dbproxy_common.pb.h"
#include "base/CCfg.h"
#include "base/CLogger.h"
#include "MessageDefine.h"
#include "_DbproxyCommonConfig_.h"
#include "_MallConfigData_.h"


class CSrvMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace std;
using namespace NErrorCode;


class CLogicHandlerThree : public NFrame::CHandler
{
public:
	CLogicHandlerThree();
	~CLogicHandlerThree();
	
public:
	int init(CSrvMsgHandler* pMsgHandler);
    void updateConfig();
	
private:
    // 韩国平台 玩家使用优惠券通知
	void useHanYouCouponNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 韩国平台 玩家使用优惠券获得奖励
	void addHanYouCouponReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 自动积分兑换物品
	void autoScoreExchangeItem(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	CSrvMsgHandler* m_msgHandler;
};

#endif // __CLOGIC_HANDLER_THREE_H__
