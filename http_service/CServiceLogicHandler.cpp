
/* author : limingfan
 * date : 2018.01.12
 * description : 服务消息逻辑实现
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>

#include "CServiceLogicHandler.h"
#include "CHttpOptSrv.h"


using namespace NProject;


namespace NPlatformService
{

CServiceLogicHandler::CServiceLogicHandler()
{
	m_msgHandler = NULL;
}

CServiceLogicHandler::~CServiceLogicHandler()
{
	m_msgHandler = NULL;
}

int CServiceLogicHandler::init(CHttpMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

    // 服务消息注册
	// m_msgHandler->registerProtocol(ServiceType::CommonSrv, XXX, (ProtocolHandler)&CServiceLogicHandler::serviceMsgHandler, this);
	
	return SrvOptSuccess;
}

void CServiceLogicHandler::unInit()
{
}

// void CServiceLogicHandler::serviceMsgHandler(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
// {
// }

}
