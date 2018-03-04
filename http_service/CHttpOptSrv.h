
/* author : limingfan
 * date : 2017.09.25
 * description : Http 操作服务实现
 */

#ifndef CHTTP_OPT_SRV_H
#define CHTTP_OPT_SRV_H

#include "base/CLogger.h"
#include "db/CRedis.h"
#include "http_base/CHttpSrv.h"
#include "BaseDefine.h"

#include "CWXLogin.h"
#include "CHttpLogicHandler.h"
#include "CServiceLogicHandler.h"


using namespace NFrame;
using namespace NDBOpt;
using namespace NProject;

namespace NPlatformService
{

// 消息协议处理对象
class CHttpMsgHandler : public CHttpDataHandler
{
public:
	CHttpMsgHandler();
	~CHttpMsgHandler();
	
public:
	const NHttpOperationConfig::ThirdPlatformInfo* getThirdPlatformInfo(unsigned int id);

	NCommon::CLogger& getDataLogger(const unsigned int id);
	
	CServiceOperation& getSrvOpt();
	
	CRedis& getRedisOpt();

public:
    // 充值处理操作流程
    bool getTranscationData(const char* orderId, OrderIdToInfo::iterator& it);
	void removeTransactionData(OrderIdToInfo::iterator it);

	// 充值，物品发放
	void provideRechargeItem(int result, OrderIdToInfo::iterator orderInfoIt, unsigned int platformType, const char* uid = "", const char* thirdOtherData = "");
	
private:
	void doTaskTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount);

	unsigned int checkUnHandleOrderData();

	int getUnHandleOrderData();
	bool parseOrderData(const char* orderId, const char* orderData, ThirdOrderInfo& orderInfo);
	
private:
	// 获取第三方平台订单ID
	bool getThirdOrderId(const unsigned int platformType, const unsigned int osType, const unsigned int srcSrvId, char* orderId, unsigned int len);
    const ThirdOrderInfo* saveThirdOrderData(const char* userName, const char* hallId, const unsigned int srcSrvId, const unsigned int platformId,
											 const char* orderId, const com_protocol::ClientGetRechargeOrderRsp& orderRsp);
    void removeTransactionData(const char* orderId);
	
	bool rechargeReply(const com_protocol::UserRechargeRsp& userRechargeRsp);

private:
	void getUnHandleOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getRechargeOrder(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void cancelRechargeOrder(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    void saveOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void removeOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void rechargeItemReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
public:
    const NHttpOperationConfig::HttpOptConfig* m_pCfg;  // 公共配置数据，外部可直接访问
	
private:
    // 服务启动时被调用
	virtual int onInit(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	// 服务停止时被调用
	virtual void onUnInit(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	// 服务配置更新时被调用
	virtual void onUpdateConfig();
	
private:
    CServiceOperation m_srvOpt;
	
	CRedis m_redisDbOpt;
	
	IdToDataLogger m_id2DataLogger;
	IdToOrderIndex m_id2OrderIndex;
	
	OrderIdToInfo m_orderId2Info;
	
private:
	CWXLogin m_wxLogin;
    CHttpLogicHandler m_httpLogicHandler;
    CServiceLogicHandler m_serviceLogicHandler;


DISABLE_COPY_ASSIGN(CHttpMsgHandler);
};


// Http 服务
class CHttpOptSrv : public CHttpSrv
{
public:
	CHttpOptSrv();
	~CHttpOptSrv();

private:
	virtual void onRegister(const char* name, const unsigned int id);       // 服务启动后被调用，服务需在此注册本服务的各模块信息
	
	
DISABLE_COPY_ASSIGN(CHttpOptSrv);
};

}

#endif // CHTTP_OPT_SRV_H
