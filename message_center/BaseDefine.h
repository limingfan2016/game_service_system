
/* author : limingfan
 * date : 2017.09.18
 * description : 基础类型定义
 */
 
#ifndef __BASE_DEFINE_H__
#define __BASE_DEFINE_H__

#include "../common/MessageCenterProtocolId.h"
#include "../common/MessageCenterErrorCode.h"
#include "../common/CommonType.h"
#include "../common/CServiceOperation.h"
#include "../common/OperationManagerProtocolId.h"
#include "../common/DBProxyProtocolId.h"

#include "_MsgCenterCfg_.h"
#include "protocol/appsrv_message_center.pb.h"


using namespace NProject;

typedef unordered_map<string, unsigned int> UserNameToServiceID;

// 大厅、棋牌室信息
struct SHallInfo
{
	unsigned int serviceId;
	string hallId;
	
	SHallInfo() {};
	SHallInfo(unsigned int srvId, const string& hId) : serviceId(srvId), hallId(hId) {};
	~SHallInfo() {};
};

typedef unordered_map<string, SHallInfo> UserNameToHallInfo;


#endif // __BASE_DEFINE_H__

