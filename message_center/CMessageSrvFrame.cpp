
/* author : liuxu
* date : 2015.04.28
* description : 消息中心服务框架代码
*/

#include "CMessageSrvFrame.h"
#include "CMsgCenterHandler.h"


using namespace NCommon;
using namespace NFrame;


int CMessageSrvFrame::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onInit service ---, name = %s, id = %d", name, id);
	return 0;
}

void CMessageSrvFrame::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onUnInit service ---, name = %s, id = %d", name, id);
}

void CMessageSrvFrame::onRegister(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onRegister module ---, name = %s, id = %d", name, id);
	
	// 注册模块实例
	static CMsgCenterHandler handler1;
	m_pMsgHandler = &handler1;
	registerModule(0, &handler1);
}

void CMessageSrvFrame::onUpdateConfig(const char* name, const unsigned int id)
{
	m_pMsgHandler->loadConfig(true);
}


CMessageSrvFrame::CMessageSrvFrame() :IService::IService(ServiceType::MessageCenterSrv)
{
	
}

CMessageSrvFrame::~CMessageSrvFrame()
{
}


REGISTER_SERVICE(CMessageSrvFrame);


