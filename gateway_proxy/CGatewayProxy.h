
/* author : admin
 * date : 2015.10.12
 * description : 网关代理服务实现
 */

#ifndef CGATEWAY_PROXY_SRV_H
#define CGATEWAY_PROXY_SRV_H

#include "SrvFrame/IService.h"
#include "SrvFrame/CGameModule.h"
#include "common/CServiceOperation.h"
#include "TypeDefine.h"


namespace NPlatformService
{

// 消息协议处理对象
class CSrvMsgHandler : public NFrame::CGameModule
{
public:
	CSrvMsgHandler();
	~CSrvMsgHandler();
    
public:
    bool inline need_encrypt_data() const
    {
        return (m_pCfg->commonCfg.need_encrypt_data == 1);
    }
	
private:
	// 定时把数据更新到loginlist服务
	void updateInfo2LoginList(uint32_t timerId, int userId, void* param, uint32_t remainCount);

	int stopServiceConnect(unsigned int srcSrvId);
    ConnectUserData* getConnectUserData(Connect* conn, unsigned int serviceId, unsigned short protocolId, unsigned int ip = 0, unsigned short port = 0);

private:
    virtual void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	virtual void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
private:
	// 收到网络客户端发往内部服务的消息				  
	virtual int onClientMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
                                unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn);
								
    // 收到内部服务发往网络客户端的消息
	virtual int onServiceMessage(const char* msgData, const unsigned int msgLen, const char* userData, const unsigned int userDataLen,
								 unsigned short dstProtocolId, unsigned int srcSrvId, unsigned short srcSrvType, unsigned short srcModuleId,
						         unsigned short srcProtocolId, int userFlag, unsigned int msgId, const char* srvAsyncDataFlag, unsigned int srvAsyncDataFlagLen);
	
public:
	void onUpdateConfig(bool isReset);     // 服务配置更新
	
	void onClosedConnect(void* userData);   // 通知逻辑层对应的逻辑连接已被关闭
	
public:
    const NGatewayProxyConfig::GatewayConfig* m_pCfg;

private:
    NProject::CServiceOperation m_srvOpt;
	NProject::GatewayProxyServiceData m_gatewayProxySrvData;

	IndexToConnects m_idx2Connects;
	unsigned int m_connectIndex;
    
    ServiceTypeBitArray m_serviceType;
    ServiceIntervalMaxPkgCount m_serviceMaxPkgCount;


DISABLE_COPY_ASSIGN(CSrvMsgHandler);
};


// 网关代理服务
class CGatewayProxySrv : public NFrame::IService
{
public:
	CGatewayProxySrv();
	~CGatewayProxySrv();

private:
	virtual int onInit(const char* name, const unsigned int id);              // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);           // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);         // 服务启动后被调用，服务需在此注册本服务的各模块信息
	virtual void onUpdateConfig(const char* name, const unsigned int id);     // 服务配置更新
    
private:
    // 收到外部数据之后调用 onReceiveMessage
    // 发送外部数据之前调用 onSendMessage
    // 一般用于数据加密&解密
    virtual int onReceiveMessage(NConnect::Connect* conn, char* msg, unsigned int& len);
    virtual int onSendMessage(NConnect::Connect* conn, char* msg, unsigned int& len);
	
private:
	virtual void onClosedConnect(void* userData);                             // 通知逻辑层对应的逻辑连接已被关闭

private:
    CSrvMsgHandler* m_connectNotifyToHandler;
	
	
DISABLE_COPY_ASSIGN(CGatewayProxySrv);
};

}

#endif // CGATEWAY_PROXY_SRV_H
