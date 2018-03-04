
/* author : liuxu
 * modify : limingfan
 * date : 2015.03.03
 * description : db proxy服务
 */
 
#ifndef _CDBPROXY_SERVICE_H_
#define _CDBPROXY_SERVICE_H_


#include "SrvFrame/IService.h"


class CMsgHandler;

class CDBProxy : public NFrame::IService
{
public:
	CDBProxy();
	~CDBProxy();
	
public:
	virtual int onInit(const char* name, const unsigned int id);             // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);          // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);        // 服务启动后被调用，服务需在此注册本服务的各模块信息
	virtual void onUpdateConfig(const char* name, const unsigned int id);    // 服务配置更新
	
private:
    CMsgHandler* m_msgHandler;
};

#endif // _CDBPROXY_SERVICE_H_

