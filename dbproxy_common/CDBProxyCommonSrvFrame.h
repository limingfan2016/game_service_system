
/* author : liuxu
 * date : 2015.03.03
 * description : dbproxy_common服务框架测试代码
 */
 
#ifndef __CDBPROXY_COMMON_SVR_FRAME_H__
#define __CDBPROXY_COMMON_SVR_FRAME_H__

#include "SrvFrame/IService.h"

class CSrvMsgHandler;

class CDBProxyCommonSrvFrame : public NFrame::IService
{
public:
	CDBProxyCommonSrvFrame();
	~CDBProxyCommonSrvFrame();
	
public:
	virtual int onInit(const char* name, const unsigned int id);            // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);          // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);    // 服务启动后被调用，服务需在此注册本服务的各模块信息
	virtual void onUpdateConfig(const char* name, const unsigned int id);   // 服务配置更新
	
private:
    CSrvMsgHandler* m_msgHandler;
};

#endif // __CDBPROXY_COMMON_SVR_FRAME_H__
