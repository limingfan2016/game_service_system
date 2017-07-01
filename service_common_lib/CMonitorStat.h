
/* author : liuxu
 * date : 2015.07.03
 * description : 监控统计
 */

#ifndef __CMONITOR_STAT_H__
#define __CMONITOR_STAT_H__


#include <string>
#include "base/MacroDefine.h"
#include "SrvFrame/CModule.h"


using namespace std;

namespace NProject
{
	// ServiceType::MessageSrv = 6
	enum EMessageBusiness
	{
		BUSINESS_SERVICE_ONLINE_NOTIFY = 14,
	};

    // ServiceType::ManageSrv = 7
	enum EManageBusiness
	{
		GET_MONITOR_STAT_DATA_REQ = 120,	//主动发起请求
		GET_MONITOR_STAT_DATA_RSP = 121,	//处理协议
	};

	class CMonitorStat : public NFrame::CHandler
	{
	public:
		CMonitorStat();
		~CMonitorStat();

	public:
		int init(NFrame::CModule* msg_hander);
		
	public:
		int sendOnlineNotify();

	public:
		void handleGetMonitorStatDataReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	protected:
		NFrame::CModule* m_msg_handler;

		DISABLE_COPY_ASSIGN(CMonitorStat);
	};


}

#endif // __CMONITOR_STAT_H__
