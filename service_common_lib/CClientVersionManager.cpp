
/* author : limingfan
 * date : 2015.05.20
 * description : 客户端软件版本更新管理
 */

#include "CommonDataDefine.h"
#include "CClientVersionManager.h"
#include "protocol/appsrv_common_module.pb.h"
#include "base/ErrorCode.h"


using namespace NErrorCode;
using namespace NCommon;
using namespace NFrame;


namespace NProject
{

CClientVersionManager::CClientVersionManager() : m_msgHandler(NULL), m_rspProtocol(0)
{
	
}

CClientVersionManager::~CClientVersionManager()
{
	m_msgHandler = NULL;
	m_rspProtocol = 0;
}

int CClientVersionManager::init(NFrame::CLogicHandler* msgHandler, unsigned short reqProtocol, unsigned short rspProtocol)
{
	m_msgHandler = msgHandler;
	m_rspProtocol = rspProtocol;
	
	m_msgHandler->registerProtocol(OutsideClientSrv, reqProtocol, (ProtocolHandler)&CClientVersionManager::checkVersion, this);
	
	return Success;
}

void CClientVersionManager::checkVersion(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientCheckVersionReq checkVersionReq;
	if (!checkVersionReq.ParseFromArray(data, len))
	{
		OptErrorLog("check version ParseFromArray message error, msg byte len = %d, buff len = %d", checkVersionReq.ByteSize(), len);
		
		return;
	}
	
	com_protocol::ClientCheckVersionRsp checkVersionRsq;
	std::string* newVersion = checkVersionRsq.mutable_new_version();
	std::string* newFileURL = checkVersionRsq.mutable_new_file_url();
	
	unsigned int flag = 0;
	int rc = getVersionInfo(checkVersionReq.os_type(), checkVersionReq.platform_type(), checkVersionReq.cur_version(), flag, *newVersion, *newFileURL);
	if (rc != Success)
	{
		OptErrorLog("check version get info error, device type = %d, platform type = %d, current version = %s, rc = %d",
		checkVersionReq.os_type(), checkVersionReq.platform_type(), checkVersionReq.cur_version().c_str(), rc);
		
		return;
	}
	checkVersionRsq.set_result((com_protocol::ECheckVersionResult)flag);
	
	char msgBuffer[MaxMsgLen] = {0};
	if (!checkVersionRsq.SerializeToArray(msgBuffer, MaxMsgLen))
	{
		OptErrorLog("check version SerializeToArray message error, msg byte len = %d, buff len = %d", checkVersionRsq.ByteSize(), MaxMsgLen);
		return;
	}
	
	m_msgHandler->sendMsgToProxy(msgBuffer, checkVersionRsq.ByteSize(), m_rspProtocol);
}

}

