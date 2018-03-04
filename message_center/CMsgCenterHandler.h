
/* author : liuxu
 * modify : limingfan
 * date : 2015.04.28
 * description : 消息处理类
 */
 
#ifndef __CSVR_MSG_HANDLER_H__
#define __CSVR_MSG_HANDLER_H__

#include "../common/CSensitiveWordFilter.h"
#include "../common/_DBConfig_.h"
#include "SrvFrame/CModule.h"
#include "db/CRedis.h"
#include "base/CCfg.h"
#include "db/CMemcache.h"
#include "base/CMemManager.h"
#include "BaseDefine.h"


using namespace NCommon;
using namespace NFrame;
using namespace NDBOpt;
using namespace NErrorCode;


class CMsgCenterHandler : public NFrame::CModule
{
public:
	CMsgCenterHandler();
	~CMsgCenterHandler();

public:
	void loadConfig(bool isReset);

private:
	void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId);

private:
	// 消息处理
	void userEnterServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void userLeaveServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 消息信息过滤
    void chatMsgFilter(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 服务状态通知
	void serviceStatusNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 转发消息到用户所在的服务
	void forwardMessageToService(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 转发服务消息通知
	void forwardServiceMessageNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 刷新服务数据通知
	void updateServiceDataNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	
private:
    // 分钟定时器，自该服务启动开始算，每隔1分钟调用一次，永久定时器
    void setMinuteTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	// 配置某一时间点推送全服消息
	void timingPushMessage(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	// 每天定时推送全服消息
	void dayPushMessage(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	// 定时更新服务ID
	void updateAllServiceID(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
private:
	// 消息记录
	void msgRecord(const com_protocol::MessageRecordNotify& msgRdNf);
	
	// 消息过滤
	void messageFilter(const string& input_str, string& output_str);

	// 设置定时推送消息的定时器
    void setPushMsgTimer();
	
	// 设置每天定时推送消息的定时器
    void setDayMsgTimer();
	
	// 踢玩家下线
	// void forcePlayerQuitNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 发送全服系统消息
	// void sendMsgToAllOnlineUsers(const com_protocol::SystemMessageNotify& sysNotify);
    // void sendMsgToAllOnlineUsers(const char* content, const unsigned int contentLen, const char* title, const unsigned int titleLen,
	//                              ESystemMessageMode mode = ESystemMessageMode::ShowDialogBoxAndRoll);

private:
	CServiceOperation m_srvOpt;
	
	CRedis m_centerRedis;
	
	CSensitiveWordFilter m_senitive_word_filter;
	
	UserNameToHallInfo m_umap_hall_service_info;
	UserNameToServiceID m_umap_game_service_id;

	vector<uint32_t> m_vall_service_id;

	unsigned int m_timingPushTimerId;   // 定时推送消息定时器ID
	unsigned int m_dayMsgTimerId;       // 每天定时推送消息定时器ID
};


#endif // __CSVR_MSG_HANDLER_H__
