
/* author : liuxu
 * modify : limingfan
 * date : 2015.04.28
 * description : 消息处理类
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <algorithm>

#include "CMsgCenterHandler.h"
#include "base/Function.h"


CMsgCenterHandler::CMsgCenterHandler()
{
	m_timingPushTimerId = 0;
	m_dayMsgTimerId = 0;
}

CMsgCenterHandler::~CMsgCenterHandler()
{
	m_timingPushTimerId = 0;
	m_dayMsgTimerId = 0;
}

void CMsgCenterHandler::loadConfig(bool isReset)
{
	if (isReset) m_srvOpt.updateCommonConfig(CCfg::getValue("ServiceConfig", "ConfigFile"));

	m_senitive_word_filter.unInit();
	m_senitive_word_filter.init(m_srvOpt.getCommonCfg().data_file.sensitive_words_file.c_str());
	
	const MsgCenterCfg::config& cfgValue = MsgCenterCfg::config::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str(), isReset);

	if (cfgValue.msgCfg.timingPushCfg.switch_value != 1 && m_timingPushTimerId != 0)
	{
		killTimer(m_timingPushTimerId);
		m_timingPushTimerId = 0;
	}
	
	if (cfgValue.msgCfg.dayMsgCfg.switch_value != 1 && m_dayMsgTimerId != 0)
	{
		killTimer(m_dayMsgTimerId);
		m_dayMsgTimerId = 0;
	}
	
	m_srvOpt.getDBCfg(isReset);
	
	ReleaseInfoLog("update config, service result = %d, db result = %d",
	cfgValue.isSetConfigValueSuccess(), m_srvOpt.getDBCfg().isSetConfigValueSuccess());
	
	m_srvOpt.getDBCfg().output();
	cfgValue.output();
}

void CMsgCenterHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!m_srvOpt.init(this, CCfg::getValue("ServiceConfig", "ConfigFile")))
	{
		ReleaseErrorLog("init service operation instance error");

        stopService();
		return;
	}
	
	loadConfig(false);
	
	ReleaseInfoLog("--- onLoad ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}

void CMsgCenterHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("--- onUnLoad ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);

	ReleaseInfoLog("--- onUnLoad --- all success.");
}
	
void CMsgCenterHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 注册协议处理函数
	registerProtocol(ServiceType::CommonSrv, MSGCENTER_SERVICE_STATUS_NOTIFY, (ProtocolHandler)&CMsgCenterHandler::serviceStatusNotify);
	registerProtocol(ServiceType::CommonSrv, MSGCENTER_USER_ONLINE_NOTIFY, (ProtocolHandler)&CMsgCenterHandler::userEnterServiceNotify);
	registerProtocol(ServiceType::CommonSrv, MSGCENTER_USER_OFFLINE_NOTIFY, (ProtocolHandler)&CMsgCenterHandler::userLeaveServiceNotify);
	registerProtocol(ServiceType::CommonSrv, MSGCENTER_FORWARD_MESSAGE_REQ, (ProtocolHandler)&CMsgCenterHandler::forwardMessageToService);
	registerProtocol(ServiceType::CommonSrv, MSGCENTER_FORWARD_SERVICE_MESSAGE_NOTIFY, (ProtocolHandler)&CMsgCenterHandler::forwardServiceMessageNotify);
	registerProtocol(ServiceType::CommonSrv, MSGCENTER_UPDATE_SERVICE_DATA_NOTIFY, (ProtocolHandler)&CMsgCenterHandler::updateServiceDataNotify);
	registerProtocol(ServiceType::CommonSrv, MSGCENTER_MSG_FILTER_REQ, (ProtocolHandler)&CMsgCenterHandler::chatMsgFilter);
	
	ReleaseInfoLog("--- onRegister ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}
	
void CMsgCenterHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	const DBConfig::config& dbCfg = m_srvOpt.getDBCfg();
	if (!m_centerRedis.connectSvr(dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port,
	    dbCfg.redis_db_cfg.center_db_timeout * MillisecondUnit))
	{
		ReleaseErrorLog("connect center redis service failed, ip = %s, port = %u, time out = %u",
		dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port, dbCfg.redis_db_cfg.center_db_timeout);
		stopService();
		return;
	}

    // 每 updateSrvIdIntervalSecs 秒从redis更新服务ID
	const unsigned int updateSrvIdIntervalSecs = 10;
	setTimer(MillisecondUnit * updateSrvIdIntervalSecs, (TimerHandler)&CMsgCenterHandler::updateAllServiceID, 0, NULL, -1);
	updateAllServiceID(0, 0, NULL, 0);
	
	// 分钟定时器，自该服务启动开始算，每隔1分钟调用一次，永久定时器
	const unsigned int minuteSecs = 60;
	setTimer(MillisecondUnit * minuteSecs, (TimerHandler)&CMsgCenterHandler::setMinuteTimer, 0, NULL, -1);
	
	ReleaseInfoLog("--- onRun ---, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}

void CMsgCenterHandler::updateAllServiceID(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	//获取hall的服务列表
	vector<string> hallData;
	int ret = m_centerRedis.getHAll(NProject::GameHallListKey, NProject::GameHallListLen, hallData);
	if (0 != ret)
	{
		ReleaseErrorLog("get hall service error, ret = %d ", ret);
		//stopService();
		return;
	}
	
	//获取game的服务列表
	vector<string> gameData;
	ret = m_centerRedis.getHAll(NProject::GameServiceListKey, NProject::GameServiceListLen, gameData);
	if (0 != ret)
	{
		ReleaseErrorLog("get game service error, ret = %d ", ret);
		//stopService();
		return;
	}
	
	//先清空
	m_vall_service_id.clear();

	for (unsigned int i = 0; i < hallData.size(); i += 2)
	{
		m_vall_service_id.push_back(*(const uint32_t*)(hallData[i].c_str()));
	}

	for (unsigned int i = 0; i < gameData.size(); i += 2)
	{
		m_vall_service_id.push_back(*(const uint32_t*)(gameData[i].c_str()));
	}
}

void CMsgCenterHandler::setMinuteTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	// 配置某一时间点推送全服消息
    // setPushMsgTimer();
	
	// 设置每天定时推送消息的定时器
    // setDayMsgTimer();
}

// 配置某一时间点推送全服消息
void CMsgCenterHandler::timingPushMessage(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	const MsgCenterCfg::TimingPushCfg& timingPushCfg = MsgCenterCfg::config::getConfigValue().msgCfg.timingPushCfg;
	if (timingPushCfg.switch_value != 1)  // 关闭了
	{
		if (remainCount > 0) killTimer(timerId);
		m_timingPushTimerId = 0;
		return;
	}
	
	if (userId == 0 && timingPushCfg.count > 1)  // 第一次到时间点触发
	{
		// 重新设置间隔时间点触发
		m_timingPushTimerId = setTimer(1000 * timingPushCfg.interval, (TimerHandler)&CMsgCenterHandler::timingPushMessage, 1, NULL, timingPushCfg.count - 1);
	}
	else if (remainCount < 1)  // 最后一次触发了
	{
		m_timingPushTimerId = 0;
	}
	
	// 发送全服系统消息
	for (vector<MsgCenterCfg::Message>::const_iterator it = timingPushCfg.messageArray.begin(); it != timingPushCfg.messageArray.end(); ++it)
	{
        // sendMsgToAllOnlineUsers(it->content.c_str(), it->content.length(), it->title.c_str(), it->title.length());
	}
}

// 每天定时推送全服消息
void CMsgCenterHandler::dayPushMessage(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	const MsgCenterCfg::DayMessageCfg& dayMsgCfg = MsgCenterCfg::config::getConfigValue().msgCfg.dayMsgCfg;
	if (dayMsgCfg.switch_value != 1)  // 关闭了
	{
		if (remainCount > 0) killTimer(timerId);
		m_dayMsgTimerId = 0;
		return;
	}
	
	if (userId == 0 && dayMsgCfg.count > 1)  // 第一次到时间点触发
	{
		// 重新设置间隔时间点触发
		m_dayMsgTimerId = setTimer(1000 * dayMsgCfg.interval, (TimerHandler)&CMsgCenterHandler::dayPushMessage, 1, NULL, dayMsgCfg.count - 1);
	}
	else if (remainCount < 1)  // 最后一次触发了
	{
		m_dayMsgTimerId = 0;
	}
	
	// 发送全服系统消息
	for (vector<MsgCenterCfg::Message>::const_iterator it = dayMsgCfg.messageArray.begin(); it != dayMsgCfg.messageArray.end(); ++it)
	{
        // sendMsgToAllOnlineUsers(it->content.c_str(), it->content.length(), it->title.c_str(), it->title.length());
	}
}

// 设置定时推送消息的定时器
void CMsgCenterHandler::setPushMsgTimer()
{
	const MsgCenterCfg::TimingPushCfg& timingPushCfg = MsgCenterCfg::config::getConfigValue().msgCfg.timingPushCfg;
	if (timingPushCfg.switch_value == 1 && m_timingPushTimerId == 0)  // 定时推送的全服系统消息
	{
		struct tm triggerTime;
		if (strptime(timingPushCfg.time.c_str(), "%Y-%m-%d %H:%M:%S", &triggerTime) != NULL)  // 配置的触发时间点
		{
			time_t triggerSecs = mktime(&triggerTime);
			time_t currentSecs = time(NULL);
			if (triggerSecs != (time_t)-1 && triggerSecs > currentSecs)
			{
				m_timingPushTimerId = setTimer(1000 * (triggerSecs - currentSecs), (TimerHandler)&CMsgCenterHandler::timingPushMessage);
			}
			else if (triggerSecs == (time_t)-1)
			{
				OptErrorLog("get trigger time to set timing message timer error, time = %s", timingPushCfg.time.c_str());
			}
		}
		else
		{
			OptErrorLog("get local time to set timing message timer error, time = %s", timingPushCfg.time.c_str());
		}
	}
}

// 设置每天定时推送消息的定时器
void CMsgCenterHandler::setDayMsgTimer()
{
	const MsgCenterCfg::DayMessageCfg& dayMsgCfg = MsgCenterCfg::config::getConfigValue().msgCfg.dayMsgCfg;
	if (dayMsgCfg.switch_value == 1 && m_dayMsgTimerId == 0)  // 每天定时推送的全服系统消息
	{
		struct tm triggerTime;
		time_t currentSecs = time(NULL);
		if (localtime_r(&currentSecs, &triggerTime) != NULL)
		{
			// 配置的每天触发时间点
			triggerTime.tm_sec = dayMsgCfg.second;
			triggerTime.tm_min = dayMsgCfg.minute;
			triggerTime.tm_hour = dayMsgCfg.hour;
			time_t triggerSecs = mktime(&triggerTime);
			if (triggerSecs != (time_t)-1 && triggerSecs > currentSecs)
			{
				m_dayMsgTimerId = setTimer(1000 * (triggerSecs - currentSecs), (TimerHandler)&CMsgCenterHandler::dayPushMessage);
			}
			else if (triggerSecs == (time_t)-1)
			{
				OptErrorLog("get trigger time to set day message timer error");
			}
		}
		else
		{
			OptErrorLog("get local time to set day message timer error");
		}
	}
}

void CMsgCenterHandler::chatMsgFilter(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::MessageFilterReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "message filter request")) return;

    com_protocol::MessageFilterRsp rsp;
	messageFilter(req.chat_context(), *rsp.mutable_filter_chat_content());

    m_srvOpt.sendPkgMsgToService(rsp, srcSrvId, MSGCENTER_MSG_FILTER_RSP, "message filter reply");

	// 记录聊天
	com_protocol::MessageRecordNotify msgRdNf;
	msgRdNf.set_username(req.username());
	msgRdNf.set_hall_id(req.hall_id());
	msgRdNf.set_game_type(req.game_type());
	msgRdNf.set_room_rate(req.room_rate());
	msgRdNf.set_chat_type(req.chat_type());
	msgRdNf.set_chat_context(req.chat_context());
	
	msgRecord(msgRdNf);
}

void CMsgCenterHandler::userEnterServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserEnterGameNotify nf;
	if (!m_srvOpt.parseMsgFromBuffer(nf, data, len, "user enter service")) return;

	if (nf.service_id() / ServiceIdBaseValue == ServiceType::GameHallSrv)
	{
		m_umap_hall_service_info[nf.username()] = SHallInfo(nf.service_id(), nf.hall_id());
	}
	else
	{
		m_umap_game_service_id[nf.username()] = nf.service_id();
	}

    m_srvOpt.sendMsgToService(nf.username().c_str(), nf.username().length(), ServiceType::DBProxySrv, data, len, DBPROXY_USER_LOGIN_NOTIFY);

    m_srvOpt.sendMsgToService(nf.username().c_str(), nf.username().length(), ServiceType::OperationManageSrv, data, len, OPTMGR_USER_ONLINE_NOTIFY);
}

void CMsgCenterHandler::userLeaveServiceNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserLeaveGameNotify nf;
	if (!m_srvOpt.parseMsgFromBuffer(nf, data, len, "user leave service")) return;

	if (nf.service_id() / ServiceIdBaseValue == ServiceType::GameHallSrv)
	{
		m_umap_hall_service_info.erase(nf.username());
	}
	else
	{
		m_umap_game_service_id.erase(nf.username());
	}

    m_srvOpt.sendMsgToService(nf.username().c_str(), nf.username().length(), ServiceType::DBProxySrv, data, len, DBPROXY_USER_LOGOUT_NOTIFY);

	m_srvOpt.sendMsgToService(nf.username().c_str(), nf.username().length(), ServiceType::OperationManageSrv, data, len, OPTMGR_USER_OFFLINE_NOTIFY);
}

void CMsgCenterHandler::messageFilter(const string& input_str, string& output_str)
{
	static unsigned char filter_str[1024];
	uint32_t len = sizeof(filter_str) - 1;
	m_senitive_word_filter.filterContent((const unsigned char*)input_str.c_str(), input_str.length(), filter_str, len);
	output_str = (const char*)filter_str;
}

// 服务状态通知
void CMsgCenterHandler::serviceStatusNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ServiceStatusNotify nf;
	if (!m_srvOpt.parseMsgFromBuffer(nf, data, len, "service status notify")) return;

	// 通知管理服务&dbproxy
	int rc = m_srvOpt.sendMsgToService("", 0, ServiceType::OperationManageSrv, data, len, OPTMGR_SERVICE_STATUS_NOTIFY);
	m_srvOpt.sendMsgToService("", 0, ServiceType::DBProxySrv, data, len, DBPROXY_SERVICE_STATUS_NOTIFY);

	// 清除之前的在线用户
	const unsigned int srvId = nf.service_id();
	if (srvId / ServiceIdBaseValue == ServiceType::GameHallSrv)
	{
		for (UserNameToHallInfo::iterator it = m_umap_hall_service_info.begin(); it != m_umap_hall_service_info.end();)
		{
			if (it->second.serviceId == srvId)
			{
				m_umap_hall_service_info.erase(it++);
			}
			else
			{
				++it;
			}
		}
	}
	else
	{
		for (UserNameToServiceID::iterator it = m_umap_game_service_id.begin(); it != m_umap_game_service_id.end();)
		{
			if (it->second == srvId)
			{
				m_umap_game_service_id.erase(it++);
			}
			else
			{
				++it;
			}
		}
	}
	
	OptWarnLog("service status notify id = %u, status = %d, type = %u, name = %s, rc = %d",
	nf.service_id(), nf.service_status(), nf.service_type(), nf.service_name().c_str(), rc);
}

// 转发消息到用户所在的服务
void CMsgCenterHandler::forwardMessageToService(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ForwardMessageToServiceReq req;
	if (!m_srvOpt.parseMsgFromBuffer(req, data, len, "forward message request")) return;
	
	unsigned int dstServiceId = 0;
	if (req.dst_service_type() == ServiceType::GameHallSrv)
	{
		// 如果是大厅就找根据用户名找一个服务，因为大厅提供的服务功能都是相同的
		UserNameToHallInfo::const_iterator it = m_umap_hall_service_info.find(req.username());
		if (it != m_umap_hall_service_info.end()) dstServiceId = it->second.serviceId;
		else getDestServiceID(ServiceType::GameHallSrv, req.username().c_str(), req.username().length(), dstServiceId);
	}
	else
	{
		UserNameToServiceID::const_iterator it = m_umap_game_service_id.find(req.username());
		if (it != m_umap_game_service_id.end()) dstServiceId = it->second;
	}
	
	if (dstServiceId == 0)
	{
		OptErrorLog("forward message to service, can not find the user from service, user name = %s, service type = %u",
		req.username().c_str(), req.dst_service_type());
		return;
	}

    sendMessage(req.data().c_str(), req.data().length(), req.src_service_id() / ServiceIdBaseValue, req.src_service_id(), req.src_protocol(), dstServiceId, req.dst_protocol());
}

// 转发服务消息通知
void CMsgCenterHandler::forwardServiceMessageNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ForwardMessageToServiceNotify srvMsgNtf;
	if (!m_srvOpt.parseMsgFromBuffer(srvMsgNtf, data, len, "forward service message notify")) return;
	
	unsigned int dstServiceId = 0;
	for (google::protobuf::RepeatedPtrField<com_protocol::TargetServiceInfo>::const_iterator srvIt = srvMsgNtf.srv_info().begin();
	     srvIt != srvMsgNtf.srv_info().end(); ++srvIt)
	{
		dstServiceId = 0;
		if (srvIt->type() == ServiceType::GameHallSrv)
		{
			UserNameToHallInfo::const_iterator hallIt = m_umap_hall_service_info.find(srvMsgNtf.username());
			if (hallIt != m_umap_hall_service_info.end()) dstServiceId = hallIt->second.serviceId;
		}
		else
		{
			UserNameToServiceID::const_iterator gameIt = m_umap_game_service_id.find(srvMsgNtf.username());
			if (gameIt != m_umap_game_service_id.end()) dstServiceId = gameIt->second;
		}
		
		if (dstServiceId > 0)
		{
			sendMessage(srvMsgNtf.data().c_str(), srvMsgNtf.data().length(), srcSrvId / ServiceIdBaseValue, srcSrvId,
						srcProtocolId, dstServiceId, srvIt->protocol());
		}
	}
}

// 刷新服务数据通知
void CMsgCenterHandler::updateServiceDataNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ServiceUpdateDataNotify srvUpdateDataNtf;
	if (!m_srvOpt.parseMsgFromBuffer(srvUpdateDataNtf, data, len, "update service data notify")) return;

    unsigned int hallServiceId = 0;
	if (srvUpdateDataNtf.hall_id().empty())
	{
		UserNameToHallInfo::const_iterator it = m_umap_hall_service_info.find(srvUpdateDataNtf.src_username());
		if (it == m_umap_hall_service_info.end())
		{
			OptErrorLog("update service data notify can not find hall id error, username = %s", srvUpdateDataNtf.src_username().c_str());
			
			return;
		}
		
		srvUpdateDataNtf.set_hall_id(it->second.hallId);
		hallServiceId = it->second.serviceId;
	}
	
	const string& hallId = srvUpdateDataNtf.hall_id();
	for (UserNameToHallInfo::const_iterator it = m_umap_hall_service_info.begin(); it != m_umap_hall_service_info.end(); ++it)
	{
		if (it->second.hallId == hallId && m_umap_game_service_id.find(it->first) == m_umap_game_service_id.end())
		{
			// 在棋牌室大厅并且不在游戏服务中则视为需要通知的玩家对象
			srvUpdateDataNtf.add_notify_usernames(it->first);
			
			if (hallServiceId == 0) hallServiceId = it->second.serviceId;
		}
	}
	
	if (srvUpdateDataNtf.notify_usernames_size() > 0)
	{
		int rc = m_srvOpt.sendPkgMsgToService(srvUpdateDataNtf, hallServiceId, MSGCENTER_UPDATE_SERVICE_DATA_NOTIFY, "update service data notify");
		OptInfoLog("update service data notify, hallId = %s, serviceId = %u, username = %s, rc = %d",
		hallId.c_str(), hallServiceId, srvUpdateDataNtf.src_username().c_str(), rc);
	}
}

	
void CMsgCenterHandler::msgRecord(const com_protocol::MessageRecordNotify& msgRdNf)
{
	m_srvOpt.sendPkgMsgToService(msgRdNf.username().c_str(), msgRdNf.username().length(), ServiceType::OperationManageSrv, msgRdNf, OPTMGR_MSG_RECORD_NOTIFY, "message record notify");
}

/*
// 踢玩家下线
void CMsgCenterHandler::forcePlayerQuitNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 转发消息给游戏&大厅处理
	auto it = m_umap_game_service_id.find(getContext().userData);  // 先找游戏
	if (it != m_umap_game_service_id.end()) sendMessage(data, len, getContext().userData, getContext().userDataLen, it->second, BUSINESS_FORCE_PLAYER_QUIT_NOTIFY);

	it = m_umap_hall_service_id.find(getContext().userData);
	if (it != m_umap_hall_service_id.end()) sendMessage(data, len, getContext().userData, getContext().userDataLen, it->second, BUSINESS_FORCE_PLAYER_QUIT_NOTIFY);
}

// 发送全服系统消息
void CMsgCenterHandler::sendMsgToAllOnlineUsers(const com_protocol::SystemMessageNotify& sysNotify)
{
	char send_data[MaxMsgLen] = {0};
	const unsigned int dataLen = sysNotify.ByteSize();
	if (!sysNotify.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("send message to online users, SerializeToArray error, msg len = %u, buffer len = %u", dataLen, MaxMsgLen);
		return;
	}

	for (unsigned int i = 0; i < m_vall_service_id.size(); ++i)
	{
		sendMessage(send_data, dataLen, m_vall_service_id[i], BUSINESS_SYSTEM_MESSAGE_NOTIFY);
	}
}

// 发送全服系统消息
void CMsgCenterHandler::sendMsgToAllOnlineUsers(const char* content, const unsigned int contentLen, const char* title, const unsigned int titleLen, ESystemMessageMode mode)
{
	// 系统广播消息，发给所有在线用户玩家
	com_protocol::SystemMessageNotify sysNotify;
	sysNotify.set_content(content, contentLen);
	sysNotify.set_title(title, titleLen);
	sysNotify.set_mode(mode);
	sysNotify.set_type(ESystemMessageType::Group);
	
	sendMsgToAllOnlineUsers(sysNotify);
}
*/
