
/* author : liuxu
 * date : 2015.04.28
 * description : 消息中心服务框架代码
 */
 
#ifndef __CMESSAGE_SVR_FRAME_H__
#define __CMESSAGE_SVR_FRAME_H__

#include "SrvFrame/IService.h"


class CMsgCenterHandler;


class CMessageSrvFrame : public NFrame::IService
{
public:
	CMessageSrvFrame();
	~CMessageSrvFrame();
	
public:
	virtual int onInit(const char* name, const unsigned int id);             // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);          // 服务停止时被调用
	virtual void onRegister(const char* name, const unsigned int id);        // 服务启动后被调用，服务需在此注册本服务的各模块信息
	virtual void onUpdateConfig(const char* name, const unsigned int id);    // 服务配置更新

private:
	CMsgCenterHandler* m_pMsgHandler;
};

#endif // __CMESSAGE_SVR_FRAME_H__
