
/* author : limingfan
 * date : 2016.03.30
 * description : 逻辑数据处理
 */
 
#ifndef _CLOGIC_DATA_H_
#define _CLOGIC_DATA_H_

#include "base/MacroDefine.h"
#include "SrvFrame/CModule.h"
#include "BaseDefine.h"


class CMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace NErrorCode;


class CServiceLogicData : public NFrame::CHandler
{
public:
	CServiceLogicData();
	~CServiceLogicData();

public:
	int init(CMsgHandler* msgHandler);
	void unInit();
	
public:
	const UserLogicData& getLogicData(const char* userName, unsigned int len);
	UserLogicData& setLogicData(const char* userName, unsigned int len);
	
private:
    void saveLogicData();
	
    void setTaskTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount);  // 任务定时器
    void setHourTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount);  // 小时定时器

private:
    void setServiceData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    void getServiceData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getMoreServiceData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void delServiceData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getUserData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void modifyUserData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    
    void getUserAttributeValue(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	CMsgHandler* m_msgHandler;
	UserToLogicData m_user2LogicData;
	

DISABLE_COPY_ASSIGN(CServiceLogicData);
};

#endif // _CLOGIC_DATA_H_
