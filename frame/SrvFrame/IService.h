
/* author : limingfan
 * date : 2015.01.30
 * description : 服务开发接口
 */

#ifndef ISERVICE_H
#define ISERVICE_H

#include "UserType.h"
#include "base/MacroDefine.h"


namespace NFrame
{

class CModule;
class CNetDataHandler;


// 服务基础能力框架
class IService
{
public:
	IService(unsigned int srvType, bool isConnectClient = false);
	virtual ~IService();

public:
    // 获取本服务名&ID
	const char* getName();
	unsigned int getId();
	
    // 注册本服务的各模块实例对象
    int registerModule(unsigned short moduleId, CModule* pInstance);
	
	// 注册纯网络数据处理模块实例对象
	int registerNetModule(CNetDataHandler* pInstance);

    // 停止退出服务
    void stopService();
	
    // 开发者实现接口
public:
	virtual int onInit(const char* name, const unsigned int id);             // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);          // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id) = 0;    // 服务启动后被调用，服务需在此注册本服务的各模块信息
	virtual void onUpdateConfig(const char* name, const unsigned int id);    // 服务配置更新
	virtual int onHandle();                                                  // 服务自己的处理逻辑
	
public:
	virtual void onClosedConnect(void* userData);                            // 通知逻辑层对应的连接已被关闭
	virtual void onCloseConnectProxy(void* userData, int cbFlag);            // 通知逻辑层对应的逻辑连接代理已被关闭
	

DISABLE_COPY_ASSIGN(IService);
};


// 注册&去注册服务实例
class CRegisterService
{
public:
	CRegisterService(IService* pSrvInstance);
	~CRegisterService();

DISABLE_CONSTRUCTION_ASSIGN(CRegisterService);
};


// 注册服务实例宏定义
#define REGISTER_SERVICE(SERVICE_CLASS_NAME)                 \
static SERVICE_CLASS_NAME serviceInstance;                   \
static CRegisterService registerService(&serviceInstance)

} 

#endif // ISERVICE_H
