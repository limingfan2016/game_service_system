
/* author : liuxu
* date : 2015.07.03
* description : 监控统计
*/

#include "CMonitorStat.h"
#include "CommonType.h"
#include "service_common_module.pb.h"
#include "SrvFrame/UserType.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "base/MacroDefine.h"


using namespace NErrorCode;
using namespace NCommon;
using namespace NFrame;


namespace NProject
{

	CMonitorStat::CMonitorStat() : m_msg_handler(NULL)
	{
	
	}

	CMonitorStat::~CMonitorStat()
	{
		m_msg_handler = NULL;
	}

	int CMonitorStat::init(NFrame::CModule* msg_handler)
	{
		m_msg_handler = msg_handler;

		m_msg_handler->registerProtocol(ManageSrv, NProject::EManageBusiness::GET_MONITOR_STAT_DATA_REQ, (ProtocolHandler)&CMonitorStat::handleGetMonitorStatDataReq, this);

		return Success;
	}
	
	int CMonitorStat::sendOnlineNotify()
	{
		unsigned int dstSrvId = 0;
		getDestServiceID(ServiceType::MessageSrv, "", 0, dstSrvId);
		return m_msg_handler->sendMessage(NULL, 0, NULL, 0, dstSrvId, EMessageBusiness::BUSINESS_SERVICE_ONLINE_NOTIFY);
	}

	void CMonitorStat::handleGetMonitorStatDataReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
	{
		com_protocol::GetMonitorStatDataReq req;
		if (!req.ParseFromArray(data, len))
		{
			char log[1024];
			b2str(data, (int)len, log, (int)sizeof(log));
			OptErrorLog("--- handleGetMonitorStatDataReq--- unpack failed.| len:%u, data:%s", len, data);
			return;
		}

		if (1 == req.stat_type())
		{
			com_protocol::GetMonitorStatDataRsp rsp;
			rsp.set_stat_type(1);

			char send_data[MaxMsgLen] = { 0 };
			int send_data_len = 0;
			send_data_len = rsp.ByteSize();
			rsp.SerializeToArray(send_data, MaxMsgLen);

			m_msg_handler->sendMessage(send_data, send_data_len, srcSrvId, NProject::EManageBusiness::GET_MONITOR_STAT_DATA_RSP);
		}
		
	}

}

