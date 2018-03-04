
/* author : liuxu
 * modify : limingfan
 * date : 2015.03.03
 * description : db proxy服务
 */


#include "CDBProxy.h"
#include "CMsgHandler.h"


using namespace NCommon;
using namespace NFrame;


int CDBProxy::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onInit service ---, name = %s, id = %d", name, id);
	
	return 0;
}

void CDBProxy::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onUnInit service ---, name = %s, id = %d", name, id);
}

void CDBProxy::onRegister(const char* name, const unsigned int id)
{
	// 注册模块实例
	static CMsgHandler handler;
	m_msgHandler = &handler;
	registerModule(0, &handler);
	
	ReleaseInfoLog("--- onRegister module ---, name = %s, id = %d", name, id);
}

void CDBProxy::onUpdateConfig(const char* name, const unsigned int id)
{
	m_msgHandler->onUpdateConfig();
}


CDBProxy::CDBProxy() :IService::IService(ServiceType::DBProxySrv)
{
}

CDBProxy::~CDBProxy()
{
}

REGISTER_SERVICE(CDBProxy);

