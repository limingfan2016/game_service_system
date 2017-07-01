
/* author : liuxu
* date : 2015.03.03
* description : dbproxy_common服务框架代码
*/
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "CDBProxyCommonSrvFrame.h"
#include "CSrvMsgHandler.h"


using namespace NCommon;
using namespace NFrame;


int CDBProxyCommonSrvFrame::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onInit service ---, name = %s, id = %d", name, id);
	return 0;
}

void CDBProxyCommonSrvFrame::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onUnInit service ---, name = %s, id = %d", name, id);
}

void CDBProxyCommonSrvFrame::onRegister(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onRegister module ---, name = %s, id = %d", name, id);
	
	// 注册模块实例
	static CSrvMsgHandler handler1;
	m_msgHandler = &handler1;
	registerModule(0, &handler1);
}

void CDBProxyCommonSrvFrame::onUpdateConfig(const char* name, const unsigned int id)
{
	m_msgHandler->onUpdateConfig();
}


CDBProxyCommonSrvFrame::CDBProxyCommonSrvFrame() :IService::IService(ServiceType::DBProxyCommonSrv)
{
	
}

CDBProxyCommonSrvFrame::~CDBProxyCommonSrvFrame()
{
}

REGISTER_SERVICE(CDBProxyCommonSrvFrame);


