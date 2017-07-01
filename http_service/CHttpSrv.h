
/* author : limingfan
 * date : 2015.07.01
 * description : Http 服务简单实现
 */

#ifndef CHTTP_SRV_H
#define CHTTP_SRV_H

#include "SrvFrame/IService.h"
#include "SrvFrame/CNetDataHandler.h"
#include "base/CMemManager.h"
#include "base/CLogger.h"
#include "common/CMonitorStat.h"
#include "db/CRedis.h"
#include "CMall.h"
#include "CHttpConnectMgr.h"
#include "CGoogleSDKOpt.h"
#include "CThirdPlatformOpt.h"
#include "CHttpData.h"
#include "TypeDefine.h"


using namespace NFrame;
using namespace NDBOpt;

namespace NPlatformService
{

static const char* HttpService = "http";


// 校验第三方用户请求处理
typedef void (CHandler::*CheckUserRequest)(const ParamKey2Value& key2value, unsigned int srcSrvId);
struct CheckUserRequestObject
{
	CHandler* instance;
	CheckUserRequest handler;
};

// 校验第三方用户应答处理
typedef bool (CHandler::*CheckUserHandler)(ConnectData* cd);
struct CheckUserHandlerObject
{
	CHandler* instance;
	CheckUserHandler handler;
};

// 支付通知Get协议处理
typedef bool (CHandler::*UserPayGetOptHandler)(Connect* conn, const RequestHeader& reqHeaderData);
struct UserPayGetOptHandlerObject
{
	CHandler* instance;
	UserPayGetOptHandler handler;
};

// 支付通知Post协议处理
typedef bool (CHandler::*UserPayPostOptHandler)(Connect* conn, const HttpMsgBody& msgBody);
struct UserPayPostOptHandlerObject
{
	CHandler* instance;
	UserPayPostOptHandler handler;
};

// http请求应答处理函数
typedef bool (CHandler::*HttpReplyHandler)(ConnectData* cd);
struct HttpReplyHandlerObject
{
	CHandler* instance;
	HttpReplyHandler handler;
	
	HttpReplyHandlerObject() {};
	HttpReplyHandlerObject(CHandler* _instance, HttpReplyHandler _handler) : instance(_instance), handler(_handler) {};
};
typedef unordered_map<unsigned int, HttpReplyHandlerObject> HttpReplyHandlerInfo;


// 消息协议处理对象
class CSrvMsgHandler : public NFrame::CNetDataHandler
{
public:
	CSrvMsgHandler();
	~CSrvMsgHandler();
	
public:
	void onClosedConnect(void* userData);
	int onHandle();  // 主动发送的http请求收到应答消息

	
public:
	bool serializeMsgToBuffer(::google::protobuf::Message& msg, char* buffer, const unsigned int len, const char* info);
	const char* serializeMsgToBuffer(::google::protobuf::Message& msg, const char* info);
	bool parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info);
	
	int sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userName, int uNLen, int userFlag = 0);
	int sendMessageToService(NFrame::ServiceType srvType, const ::google::protobuf::Message& msg, unsigned short protocolId, const char* userName, const int uNLen,
							 int userFlag = 0, unsigned short handleProtocolId = 0);
							 
	// CRedis& getRedisService();
	bool getHostInfo(const char* host, strIP_t ip, unsigned short& port, const char* service = HttpService);
	
	ConnectData* getConnectData();
	void putConnectData(ConnectData* connData);
	
	char* getDataBuffer(unsigned int& len);
	void putDataBuffer(const char* dataBuff);
	
	bool doHttpConnect(unsigned int platformType, unsigned int srcSrvId, const CHttpRequest& httpRequest, const char* url = NULL, unsigned int flag = 0, const UserCbData* cbData = NULL, unsigned int len = 0, const char* userData = NULL, unsigned short protocolId = CHECK_THIRD_USER_RSP);
	bool doHttpConnect(unsigned int platformType, unsigned int srcSrvId, const CHttpRequest& httpRequest, unsigned int id, const char* url, const UserCbData* cbData = NULL, unsigned int len = 0, const char* userData = NULL, unsigned short protocolId = CHECK_THIRD_USER_RSP);
	bool doHttpConnect(const char* ip, const unsigned short port, ConnectData* connData);
	
	// 新增充值赠送物品
	void getGiftInfo(const com_protocol::UserRechargeRsp& userRechargeRsp, google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>* giftInfo, char* logInfo, unsigned int logInfoLen);
	

public:
	// const ServiceLogicData& getLogicData(const char* userName, unsigned int len);
	// ServiceLogicData& setLogicData(const char* userName, unsigned int len);
	CMall& getMallMgr();
 
	
public:
    // 注册http消息处理函数
	void registerCheckUserHandler(unsigned int platformType, CheckUserHandler handler, CHandler* instance);
	void registerUserPayGetHandler(unsigned int platformType, UserPayGetOptHandler handler, CHandler* instance);
	void registerUserPayPostHandler(unsigned int platformType, UserPayPostOptHandler handler, CHandler* instance);
	
	void registerCheckUserRequest(unsigned int platformType, CheckUserRequest handler, CHandler* instance);
	void handleCheckUserRequest(unsigned int platformType, const ParamKey2Value& key2value, unsigned int srcSrvId);
	
	void registerHttpReplyHandler(unsigned int id, HttpReplyHandler handler, CHandler* instance);

private:
	void rechargeItemReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	int handleRequest(Connect* conn, ConnectUserData* ud);  // 被动收到第三方平台的请求消息
	
	// void saveLogicData();
	void setCheckLogicDataTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount);

private:
	virtual int onClientData(Connect* conn, const char* data, const unsigned int len);  // 被动收到第三方平台的请求消息
	
private:
	virtual void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	

	// 当乐平台API
private:
    void checkDownJoyUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getDownJoyRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void cancelDownJoyRechargeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
public:
	bool handleDownJoyPaymentMsg(Connect* conn, ConnectUserData* ud, const RequestHeader& reqHeaderData);
	
private:
	bool checkDownJoyCfg();
	bool checkDownJoyUserReply(ConnectData* cd);
	bool getRechargeTransaction(unsigned int srcSrvId, const ThirdPlatformType type, const com_protocol::GetDownJoyPaymentNoReq& msg, char* rechargeTranscation, unsigned int len, unsigned int& platformType);
	bool handleDownJoyPaymentMsg(const RequestHeader& reqHeaderData, const char* param, const char* signature);
	bool downJoyRechargeItemReply(const com_protocol::UserRechargeRsp& userRechargeRsp);
	NCommon::CLogger& getDownJoyRechargeLogger();
	

private:
    NCommon::CMemManager m_memForUserData;
	NCommon::CMemManager m_memForConnectData;
	CHttpConnectMgr m_httpConnectMgr;
	NProject::CMonitorStat m_monitorStat;
	char m_msgBuffer[NFrame::MaxMsgLen];
	
	DownJoyConfig m_downJoyCfg;
	unsigned int m_rechargeTransactionId;
	
	CGoogleSDKOpt m_googleSDKOpt;
	CThirdPlatformOpt m_thirdPlatformOpt;
	
	// CRedis m_redisDbOpt;
	
	// UserToLogicData m_user2LogicData;
	CMall m_mallMgr;
	
private:
	CheckUserHandlerObject m_checkUserHandlers[ThirdPlatformType::MaxPlatformType];
	UserPayGetOptHandlerObject m_userPayGetOptHandlers[ThirdPlatformType::MaxPlatformType];
	UserPayPostOptHandlerObject m_userPayPostOptHandlers[ThirdPlatformType::MaxPlatformType];
	CheckUserRequestObject m_checkUserRequests[ThirdPlatformType::MaxPlatformType];
	HttpReplyHandlerInfo m_httpReplyHandlerInfo;


DISABLE_COPY_ASSIGN(CSrvMsgHandler);
};


// Http 服务
class CHttpSrv : public NFrame::IService
{
public:
	CHttpSrv();
	~CHttpSrv();

private:
	virtual int onInit(const char* name, const unsigned int id);            // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);         // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);       // 服务启动后被调用，服务需在此注册本服务的各模块信息
	virtual void onUpdateConfig(const char* name, const unsigned int id);   // 服务配置更新
	
private:
	virtual void onClosedConnect(void* userData);   // 通知逻辑层对应的逻辑连接已被关闭
	virtual int onHandle();                         // 通知逻辑层处理逻辑


private:
    CSrvMsgHandler* m_connectNotifyToHandler;
	
	
DISABLE_COPY_ASSIGN(CHttpSrv);
};

}

#endif // CHTTP_SRV_H
