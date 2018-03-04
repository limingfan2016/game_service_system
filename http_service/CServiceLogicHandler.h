
/* author : limingfan
 * date : 2018.01.12
 * description : 服务消息逻辑实现
 */
 
#ifndef __CSERVICE_LOGIC_HANDLER_H__
#define __CSERVICE_LOGIC_HANDLER_H__

#include "SrvFrame/CModule.h"
#include "http_base/HttpBaseDefine.h"
#include "BaseDefine.h"


using namespace NCommon;
using namespace NFrame;


namespace NPlatformService
{

class CHttpMsgHandler;

class CServiceLogicHandler : public NFrame::CHandler
{
public:
	CServiceLogicHandler();
	~CServiceLogicHandler();

public:
	int init(CHttpMsgHandler* pMsgHandler);
	void unInit();

private:
	// void serviceMsgHandler(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	CHttpMsgHandler* m_msgHandler;
};

}

#endif // __CSERVICE_LOGIC_HANDLER_H__

