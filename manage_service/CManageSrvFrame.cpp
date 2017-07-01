
/* author : liuxu
* date : 2015.07.02
* description : manage_service服务框架代码
*/
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "CManageSrvFrame.h"
#include "CSrvMsgHandler.h"


using namespace NCommon;
using namespace NFrame;

static CSrvMsgHandler g_handler1;


int CManageSrvFrame::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onInit service ---, name = %s, id = %d", name, id);
	return 0;
}

void CManageSrvFrame::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onUnInit service ---, name = %s, id = %d", name, id);
}

void CManageSrvFrame::onRegister(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onRegister module ---, name = %s, id = %d", name, id);
	
	// 注册模块实例
	registerModule(0, &g_handler1);
}

void CManageSrvFrame::onUpdateConfig(const char* name, const unsigned int id)
{
	ReleaseInfoLog("--- onUpdateConfig ---, name = %s, id = %d", name, id);
	CCfg::reLoadCfg();

	g_handler1.loadConfig();
}

CManageSrvFrame::CManageSrvFrame() :IService::IService(ServiceType::ManageSrv, true)
{
	
}

CManageSrvFrame::~CManageSrvFrame()
{
}

REGISTER_SERVICE(CManageSrvFrame);


