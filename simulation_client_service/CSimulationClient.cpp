
/* author : limingfan
 * date : 2015.09.21
 * description : 仿真客户端服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"
#include "CSimulationClient.h"
#include "_SimulationClientConfigData_.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace NProject;

namespace NPlatformService
{

CSrvMsgHandler::CSrvMsgHandler() : m_memForRobot(RobotBlockCount, RobotBlockCount, sizeof(RobotData))
{
	m_msgBuffer[0] = '\0';
	m_robotIdIdx = 0;
}

CSrvMsgHandler::~CSrvMsgHandler()
{
	m_msgBuffer[0] = '\0';
	m_robotIdIdx = 0;
	m_memForRobot.clear();
}


bool CSrvMsgHandler::serializeMsgToBuffer(::google::protobuf::Message& msg, char* buffer, const unsigned int len, const char* info)
{
	if (!msg.SerializeToArray(buffer, len))
	{
		OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), len, info);
		return false;
	}
	return true;
}

const char* CSrvMsgHandler::serializeMsgToBuffer(::google::protobuf::Message& msg, const char* info)
{
	if (!msg.SerializeToArray(m_msgBuffer, MaxMsgLen))
	{
		OptErrorLog("SerializeToArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), MaxMsgLen, info);
		return NULL;
	}
	return m_msgBuffer;
}

bool CSrvMsgHandler::parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info)
{
	if (!msg.ParseFromArray(buffer, len))
	{
		OptErrorLog("ParseFromArray message error, msg byte len = %d, buff len = %d, info = %s", msg.ByteSize(), len, info);
		return false;
	}
	return true;
}

int CSrvMsgHandler::sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userName, int uNLen, int userFlag, unsigned short handleProtocolId)
{
	// 要根据负载均衡选择commonDBproxy服务
	unsigned int srvId = 0;
	getDestServiceID(DBProxyCommonSrv, userName, uNLen, srvId);
	return sendMessage(data, len, userFlag, userName, uNLen, srvId, protocolId, 0, handleProtocolId);
}

int CSrvMsgHandler::sendMessageToService(NFrame::ServiceType srvType, const char* data, const unsigned int len, unsigned short protocolId, const char* userName, const int uNLen,
                                         int userFlag, unsigned short handleProtocolId)
{
	unsigned int srvId = 0;
	getDestServiceID(srvType, userName, uNLen, srvId);
	return sendMessage(data, len, userFlag, userName, uNLen, srvId, protocolId, 0, handleProtocolId);
}

void CSrvMsgHandler::robotActiveLoginServiceCallBack(Connect* conn, const char* peerIp, const unsigned short peerPort, void* userCb, int userId)
{
	int rc = Opt_Failed;
	RobotData* rData = (RobotData*)userCb;
	if (conn != NULL)
	{
		// 连接建立成功了，则先重置机器人属性值
		com_protocol::ResetUserOtherInfoReq resetRobotInfoReq;
		resetRobotInfoReq.set_query_username(rData->id);
		const SimulationClientConfigData::CommonCfg& robotCommonCfg = SimulationClientConfigData::config::getConfigValue().commonCfg;
		if (rData->min_gold < robotCommonCfg.robot_gold_min) rData->min_gold = robotCommonCfg.robot_gold_min;
		if (rData->max_gold > robotCommonCfg.robot_gold_max) rData->max_gold = robotCommonCfg.robot_gold_max;
		if ((unsigned int)(rData->min_gold * robotCommonCfg.gold_min_percent) < (unsigned int)(rData->max_gold * robotCommonCfg.gold_max_percent))  // 合理范围之内
		{
			rData->min_gold *= robotCommonCfg.gold_min_percent;
			rData->max_gold *= robotCommonCfg.gold_max_percent;
		}
		
		// 重新设置机器人金币数量，根据配置调整金币随机的上下限范围
		com_protocol::UserOtherInfo* info = resetRobotInfoReq.add_other_info();
		info->set_info_flag(EPropType::PropGold);
		info->set_int_value(getRandNumber(rData->min_gold, rData->max_gold));  
		
		// 其他属性值
		const vector<SimulationClientConfigData::RobotProperty>& propertyList = SimulationClientConfigData::config::getConfigValue().reset_property_list;
		for (unsigned int idx = 0; idx < propertyList.size(); ++idx)
		{
			info = resetRobotInfoReq.add_other_info();
			info->set_info_flag(propertyList[idx].type);
			info->set_int_value(getRandNumber(propertyList[idx].min, propertyList[idx].max));
		}
		
		const char* msgData = serializeMsgToBuffer(resetRobotInfoReq, "reset robot info request");
		if (msgData != NULL) rc = sendMessageToCommonDbProxy(msgData, resetRobotInfoReq.ByteSize(), BUSINESS_RESET_USER_OTHER_INFO_REQ, rData->id, strlen(rData->id));
		
		if (rc != Success) closeConnect(conn);  // 失败则关闭连接
	}
	
	OptInfoLog("robot login service, id = %s, ip = %s, port = %d, userId = %d, type = %d, result = %s, rc = %d",
	rData->id, peerIp, peerPort, userId, rData->robot_type, (conn != NULL) ? "success" : "failed", rc);

	if (rc == Success)
	{
		rData->status = RobotStatus::RobotResetProperty;
		addConnect(rData->id, conn, rData);  // 保存成功建立的连接
	}
	else
	{
		// 失败则回收机器人，回收内存块
		releaseRobot(rData);
	}
}

void CSrvMsgHandler::enterServiceRoomReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientEnterRoomVerifyRsp clientEnterRoomVerifyRsp;
	if (!parseMsgFromBuffer(clientEnterRoomVerifyRsp, data, len, "robot enter room reply")) return;
	
	int rc = Opt_Failed;
	int robotGold = 0;
	RobotData* rData = (RobotData*)getUserData(getNetClientContext().conn);
	if (clientEnterRoomVerifyRsp.result() == 0)
	{
		// 登陆游戏成功了，接着进入游戏桌子
		robotGold = clientEnterRoomVerifyRsp.detail_info().dynamic_info().game_gold();
		com_protocol::ClientBuyuEnterTableReq clientBuyuEnterTableReq;
		clientBuyuEnterTableReq.set_table_id(rData->table_id);
		clientBuyuEnterTableReq.set_seat_id(rData->seat_id);
		clientBuyuEnterTableReq.set_is_auto_alocc(0);
		const char* msgData = serializeMsgToBuffer(clientBuyuEnterTableReq, "robot enter table");
		if (msgData != NULL) rc = sendMsgToClient(msgData, clientBuyuEnterTableReq.ByteSize(), FishClient::CLIENT_BUYU_ENTER_TABLE_REQ, rData->service_id);
	}
	
	OptInfoLog("robot enter service room, user id = %s, service id = %d, result = %d, type = %d, gold = %d, rc = %d",
	rData->id, rData->service_id, clientEnterRoomVerifyRsp.result(), rData->robot_type, robotGold, rc);
	
	if (rc == Success)
	{
		rData->status = RobotStatus::RobotEnterTable;
	}
	else
	{
		// 失败则回收机器人，回收内存块
		releaseRobot(rData);
	}
}

void CSrvMsgHandler::enterServiceTableReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientBuyuEnterTableRsp clientBuyuEnterTableRsp;
	if (!parseMsgFromBuffer(clientBuyuEnterTableRsp, data, len, "robot enter table reply")) return;

	RobotData* rData = (RobotData*)getUserData(getNetClientContext().conn);
	OptInfoLog("robot enter service table, id = %s, type = %d, result = %d", rData->id, rData->robot_type, clientBuyuEnterTableRsp.result());

	if (clientBuyuEnterTableRsp.result() == 0 && clientBuyuEnterTableRsp.player_list_size() > 1)
	{
		rData->status = RobotStatus::RobotPlayGame;
		NEW(rData->tableUsers, TableUsers);
		if (rData->tableUsers != NULL)
		{
			for (int i = 0; i < clientBuyuEnterTableRsp.player_list_size(); ++i)
			{
				const string& player = clientBuyuEnterTableRsp.player_list(i).username();
				if (strcmp(player.c_str(), rData->id) != 0) rData->tableUsers->push_back(player);
			}
		}

        // 机器人定时动作任务
		changeBulletRate(0, 0, rData, 0);
		useProp(0, 0, rData, 0);
		doChat(0, 0, rData, 0);

		// only for test
	    // testRobotLogoutNotify(rData->id);
	}
	else
	{
		// 失败则回收机器人，回收内存块
		releaseRobot(rData);
	}
}

void CSrvMsgHandler::enterTableNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	RobotData* rData = (RobotData*)getUserData(getNetClientContext().conn);
	if (rData != NULL && rData->tableUsers != NULL)
	{
		com_protocol::ClientBuyuEnterTableNotify clientBuyuEnterTableNotify;
	    if (!parseMsgFromBuffer(clientBuyuEnterTableNotify, data, len, "user enter table notify")) return;
		
		bool isFind = false;
		for (TableUsers::const_iterator it = rData->tableUsers->begin(); it != rData->tableUsers->end(); ++it)
		{
			if (*it == clientBuyuEnterTableNotify.add_player().username())
			{
				isFind = true;
				break;
			}
		}
		if (!isFind) rData->tableUsers->push_back(clientBuyuEnterTableNotify.add_player().username());
	}
}

void CSrvMsgHandler::leaveTableNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	RobotData* rData = (RobotData*)getUserData(getNetClientContext().conn);
	if (rData != NULL && rData->tableUsers != NULL)
	{
		com_protocol::ClientBuyuQuitNotify clientBuyuQuitNotify;
	    if (!parseMsgFromBuffer(clientBuyuQuitNotify, data, len, "user leave table notify")) return;
		
		for (TableUsers::iterator it = rData->tableUsers->begin(); it != rData->tableUsers->end(); ++it)
		{
			if (*it == clientBuyuQuitNotify.username())
			{
				rData->tableUsers->erase(it);
				break;
			}
		}
	}
}

void CSrvMsgHandler::robotActiveLoginService(const char* service_ip, unsigned int service_port, unsigned int service_id, unsigned int robot_type, unsigned int table_id, unsigned int seat_id, unsigned int min_gold, unsigned int max_gold)
{
	RobotData* rData = (RobotData*)m_memForRobot.get();
	if (rData == NULL)
	{
		OptErrorLog("no memory for create robot");
		return;
	}
	
	RobotUserNameVector& freeRobots = (robot_type == ERobotType::ERobotPlatformType) ? m_normalFreeRobots : m_vipFreeRobots;
	int pos = getRandNumber(1, freeRobots.size()) - 1;  // 随机一个空闲的机器人
	memset(rData, 0, sizeof(RobotData));
	strncpy(rData->id, freeRobots[pos].c_str(), sizeof(rData->id) - 1);
	rData->robot_type = robot_type;
	rData->service_id = service_id;
	rData->table_id = table_id;
	rData->seat_id = seat_id;
	rData->min_gold = min_gold;
	rData->max_gold = max_gold;
	rData->bullet_level = 1;
	rData->status = RobotStatus::RobotLogin;
	
	// 建立到游戏服务的连接
	const unsigned int activeConnectTimeOut = 30;
	static int robotLoginServiceUserId = 0;
	int rc = doActiveConnect((ActiveConnectHandler)&CSrvMsgHandler::robotActiveLoginServiceCallBack, service_ip, service_port, activeConnectTimeOut, rData, ++robotLoginServiceUserId);
	OptInfoLog("robot login service, id = %s, ip = %s, port = %d, userId = %d, type = %d, table = %d, seat = %d, min gold = %d, max gold = %d, rc = %d",
	rData->id, service_ip, service_port, robotLoginServiceUserId, robot_type, table_id, seat_id, min_gold, max_gold, rc);
	
	if (rc == Success)
	{
	    freeRobots.erase(freeRobots.begin() + pos);  // 空闲机器人出队列
	}
	else
	{
		m_memForRobot.put((char*)rData);
	}
}

void CSrvMsgHandler::robotLoginNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::RobotLoginNotify robotLoginNotifyMsg;
	if (!parseMsgFromBuffer(robotLoginNotifyMsg, data, len, "robot login notify")) return;
	
	if (robotLoginNotifyMsg.min_gold() > robotLoginNotifyMsg.max_gold())
	{
		OptErrorLog("robot login notify message param invalid, min gold = %d, max gold = %d", robotLoginNotifyMsg.min_gold(), robotLoginNotifyMsg.max_gold());
		return;
	}
	
	const unsigned int robotType = (robotLoginNotifyMsg.robot_type() == ERobotType::EVIPRobotPlatformType) ? ERobotType::EVIPRobotPlatformType : ERobotType::ERobotPlatformType;  // 兼容处理
	const RobotUserNameVector& freeRobots = (robotType == ERobotType::ERobotPlatformType) ? m_normalFreeRobots : m_vipFreeRobots;
	if (freeRobots.empty())
	{
		m_waitLoginData.push_back(RobotLoginNotifyData(robotType, srcSrvId, robotLoginNotifyMsg.table_id(), robotLoginNotifyMsg.seat_id(),
		robotLoginNotifyMsg.min_gold(), robotLoginNotifyMsg.max_gold()));

        // 没有可用的机器人了，新注册一批
        const SimulationClientConfigData::CommonCfg& robotCommonCfg = SimulationClientConfigData::config::getConfigValue().commonCfg;
	    return registerRobot(robotType, (robotType == ERobotType::ERobotPlatformType) ? robotCommonCfg.robot_register_num : robotCommonCfg.vip_robot_register_num);
	}
	
	const SimulationClientConfigData::NetProxy& netProxy = SimulationClientConfigData::config::getConfigValue().netProxy;
	robotActiveLoginService(netProxy.proxy_ip.c_str(), netProxy.proxy_port, srcSrvId, robotType,
	                        robotLoginNotifyMsg.table_id(), robotLoginNotifyMsg.seat_id(), robotLoginNotifyMsg.min_gold(), robotLoginNotifyMsg.max_gold());
}

void CSrvMsgHandler::robotLogoutNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    Connect* conn = getConnect(getContext().userData);
	if (conn != NULL)
	{
		// 回收机器人，回收内存块
		RobotData* rData = (RobotData*)getUserData(conn);
		OptInfoLog("robot logout service, user id = %s, type = %d, service id = %d, table = %d, seat = %d, status = %d",
		rData->id, rData->robot_type, rData->service_id, rData->table_id, rData->seat_id, rData->status);
		releaseRobot(rData);
	}
}

void CSrvMsgHandler::registerRobot(const unsigned int robotType, const unsigned int num)
{
	if (num > 0)
	{
		// 向dbcommon服务新注册一批机器人
		unsigned int curSecs = time(NULL);
		char platformId[MaxPlatformIdLen] = {0};
		com_protocol::GetUsernameByPlatformReq getUsernameByPlatformReq;
		getUsernameByPlatformReq.set_platform_type(robotType);
		getUsernameByPlatformReq.set_os_type(ClientVersionType::AndroidMerge);
		for (unsigned int i = 0; i < num; ++i)
		{
			snprintf(platformId, sizeof(platformId) - 1, "%u.%d", curSecs, m_robotIdIdx++);
			getUsernameByPlatformReq.set_platform_id(platformId);
			const char* msgData = serializeMsgToBuffer(getUsernameByPlatformReq, "register robot request");
			if (msgData != NULL) sendMessageToCommonDbProxy(msgData, getUsernameByPlatformReq.ByteSize(), BUSINESS_GET_USERNAME_BY_PLATFROM_REQ, "", 0, robotType);
		}
	}
	
	OptInfoLog("register robot, type = %d, count = %d", robotType, num);
}

void CSrvMsgHandler::releaseRobot(RobotData* rData, bool isCloseConnect)
{
	// 关闭机器人对应的连接，注意考虑以下场景
	// 1、连接未建立成功的情况，removeConnect返回false，触发直接回收资源
	// 2、连接建立成功，主动关闭连接的情况，removeConnect返回true，等待网络层回调才回收资源，此次调用不能回收资源
	// 3、连接建立成功，主动关闭&被动关闭连接都回调的情况，rData->status == RobotStatus::RobotRelease，直接回收资源
	if (!removeConnect(rData->id, isCloseConnect) || rData->status == RobotStatus::RobotRelease)
	{
		OptInfoLog("release robot resource, id = %s, type = %d, status = %d", rData->id, rData->robot_type, rData->status);
		
		if (rData->change_bullet_rate_timer_id > 0) killTimer(rData->change_bullet_rate_timer_id);
		if (rData->use_prop_timer_id > 0) killTimer(rData->use_prop_timer_id);
		if (rData->chat_timer_id > 0) killTimer(rData->chat_timer_id);

        RobotUserNameVector& freeRobots = (rData->robot_type == ERobotType::ERobotPlatformType) ? m_normalFreeRobots : m_vipFreeRobots;
		freeRobots.push_back(rData->id);    // 回收机器人ID
		
		DELETE(rData->tableUsers);
		m_memForRobot.put((char*)rData);      // 回收数据内存块
	}
}

/*
void CSrvMsgHandler::clearRobotGoldReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 修改机器人状态
	RobotData* rData = (RobotData*)getUserData(getConnect(getContext().userData));
	if (rData != NULL)
	{
		com_protocol::RoundEndDataChargeRsp roundEndDataChargeRsp;
		if (!parseMsgFromBuffer(roundEndDataChargeRsp, data, len, "clear robot gold reply") || roundEndDataChargeRsp.result() != 0) releaseRobot(rData);
		else rData->status = RobotStatus::RobotResetProperty;
	}
	else
	{
		OptErrorLog("clear robot gold reply but not find the robot information, user data = %s", getContext().userData);
	}
}
*/

void CSrvMsgHandler::resetRobotInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	if (ERobotDataOpt::ESetVIPLevel == getContext().userFlag) return;
	
	Connect* conn = getConnect(getContext().userData);
	RobotData* rData = (RobotData*)getUserData(conn);
	if (rData != NULL)
	{
		com_protocol::ResetUserOtherInfoRsp resetUserOtherInfoRsp;
		if (!parseMsgFromBuffer(resetUserOtherInfoRsp, data, len, "reset robot property reply")
		    || resetUserOtherInfoRsp.result() != 0
			|| rData->status != RobotStatus::RobotResetProperty) return releaseRobot(rData);

		// 机器人重置金币成功后才能登陆游戏服务
		com_protocol::ClientEnterRoomVerifyReq clientEnterRoomVerifyReq;
		clientEnterRoomVerifyReq.set_username(rData->id);
		clientEnterRoomVerifyReq.set_session_key("");
		clientEnterRoomVerifyReq.set_device(rData->robot_type);
		const char* msgData = serializeMsgToBuffer(clientEnterRoomVerifyReq, "robot enter room");
		if (msgData != NULL && Success == sendMsgToClient(msgData, clientEnterRoomVerifyReq.ByteSize(), FishClient::CLIENT_ENTER_ROOM_VERIFY_REQ, conn, rData->service_id))
		{
		    rData->status = RobotStatus::RobotEnterRoom;
		}
		else
		{
			releaseRobot(rData);  // 失败则回收机器人，回收内存块
		}
		
		// log
		unsigned int logLen = 0;
		m_msgBuffer[0] = 0;
		for (int idx = 0; idx < resetUserOtherInfoRsp.other_info_size(); ++idx)
		{
			logLen += snprintf(m_msgBuffer + logLen, NFrame::MaxMsgLen - logLen - 1, "[%d] = %d, ",
			resetUserOtherInfoRsp.other_info(idx).info_flag(), resetUserOtherInfoRsp.other_info(idx).int_value());
		}
		if (logLen > 2) m_msgBuffer[logLen - 2] = 0;
		OptInfoLog("reset robot information, user data = %s, property = %s, status = %d", getContext().userData, m_msgBuffer, rData->status);
	}
	else
	{
		OptErrorLog("reset robot property reply but not find the robot information, user data = %s", getContext().userData);
	}
}

void CSrvMsgHandler::registerRobotReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUsernameByPlatformRsp getUsernameByPlatformRsp;
	if (!parseMsgFromBuffer(getUsernameByPlatformRsp, data, len, "register robot reply")) return;

    RobotUserNameVector& freeRobots = (getContext().userFlag == ERobotType::ERobotPlatformType) ? m_normalFreeRobots : m_vipFreeRobots;
	if (getUsernameByPlatformRsp.result() == 0 && getUsernameByPlatformRsp.has_username())
	{
		// VIP机器人则需要设置VIP等级
		if (getContext().userFlag == ERobotType::EVIPRobotPlatformType)
		{
			com_protocol::ResetUserOtherInfoReq setRobotVipReq;
			setRobotVipReq.set_query_username(getUsernameByPlatformRsp.username());
			const SimulationClientConfigData::CommonCfg& robotCommonCfg = SimulationClientConfigData::config::getConfigValue().commonCfg;
			com_protocol::UserOtherInfo* info = setRobotVipReq.add_other_info();
			info->set_info_flag(EUserInfoFlag::EVipLevel);  // 设置机器人VIP等级
			info->set_int_value(getRandNumber(robotCommonCfg.robot_vip_min, robotCommonCfg.robot_vip_max));  
			
			const char* msgData = serializeMsgToBuffer(setRobotVipReq, "set robot vip level request");
			if (msgData != NULL) sendMessageToCommonDbProxy(msgData, setRobotVipReq.ByteSize(), BUSINESS_RESET_USER_OTHER_INFO_REQ, getUsernameByPlatformRsp.username().c_str(), getUsernameByPlatformRsp.username().length(), ERobotDataOpt::ESetVIPLevel);
		}
		
		// 是否存在等待登陆的机器人数据
		freeRobots.push_back(getUsernameByPlatformRsp.username());
		for (RobotLoginNotifyDataVector::iterator it = m_waitLoginData.begin(); it != m_waitLoginData.end(); ++it)
		{
			if ((int)it->robot_type == getContext().userFlag)
			{
				const SimulationClientConfigData::NetProxy& netProxy = SimulationClientConfigData::config::getConfigValue().netProxy;
				robotActiveLoginService(netProxy.proxy_ip.c_str(), netProxy.proxy_port, it->service_id, it->robot_type, it->table_id, it->seat_id, it->min_gold, it->max_gold);

				m_waitLoginData.erase(it);
				break;
			}
		}
	}
	
	OptInfoLog("register robot reply, type = %d, robots = %d, result = %d", getContext().userFlag, freeRobots.size(), getUsernameByPlatformRsp.result());
}

// 定时变更子弹倍率
void CSrvMsgHandler::changeBulletRate(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	RobotData* rData = (RobotData*)param;
	Connect* conn = getConnect(rData->id);
	if (conn == NULL)
	{
		OptErrorLog("changeBulletRate, can not find the connect data, id = %s", rData->id);
		return;
	}
	
	// 配置参数校验
	const SimulationClientConfigData::BulletCfg& bulletCfg = SimulationClientConfigData::config::getConfigValue().bulletCfg;
	if (bulletCfg.rate_add_percent < 1 || bulletCfg.rate_step_min < 1 || bulletCfg.rate_time_min_percent < 1 || bulletCfg.change_rate_time_min < 1
		|| bulletCfg.rate_step_min > bulletCfg.rate_step_max || bulletCfg.rate_time_min_percent > bulletCfg.rate_time_max_percent
	    || bulletCfg.change_rate_time_min > bulletCfg.change_rate_time_max)
	{
		OptErrorLog("bullet config error");
		return;
	}
	
	// 计算下次子弹倍率变更时间点
	double numerator = getRandNumber(bulletCfg.rate_time_min_percent, bulletCfg.rate_time_max_percent);
	double denominator = getRandNumber(bulletCfg.rate_time_min_percent, bulletCfg.rate_time_max_percent);
	double percent = numerator / denominator;
	unsigned int minSecs = bulletCfg.change_rate_time_min * percent;
	unsigned int maxSecs = bulletCfg.change_rate_time_max * percent;
	unsigned int changeSecs = getRandNumber(minSecs, maxSecs);
	if (changeSecs < 1) changeSecs = 1;
	
	// userId < 0 表示增加步长，userId > 0 表示减少步长，userId == 0 表示步长已增减完毕
	int setBulletLevelOpt = -1;
	const unsigned int maxBulletLevel = SimulationClientConfigData::config::getConfigValue().bullet_rate_level.size();
	if (userId == 0 || (rData->bullet_level == 1 && userId > 0) || (rData->bullet_level >= maxBulletLevel && userId < 0))
	{
		// 计算子弹倍率增减的步长值
		const unsigned int AddRatePercent = 100;
	    userId = 0 - getRandNumber(bulletCfg.rate_step_min, bulletCfg.rate_step_max);  // 增加步长
		if ((rData->bullet_level >= maxBulletLevel)
			|| (rData->bullet_level > 1 && (unsigned int)getRandNumber(1, AddRatePercent) > bulletCfg.rate_add_percent))
		{
			userId = 0 - userId;  // 减少步长
		}
	}
	else if (userId < 0)
	{
		++userId;
		++rData->bullet_level;
		setBulletLevelOpt = 1;
	}
	else
	{
		--userId;
		--rData->bullet_level;
		setBulletLevelOpt = 0;
	}
	
	int rc = Opt_Failed;
	unsigned int bulletRate = 0;
	if (setBulletLevelOpt != -1)
	{
		com_protocol::ClientBuyuSetBulletRateReq clientBuyuSetBulletRateReq;
		bulletRate = SimulationClientConfigData::config::getConfigValue().bullet_rate_level.at(rData->bullet_level);
		clientBuyuSetBulletRateReq.set_bullet_rate(bulletRate);
		clientBuyuSetBulletRateReq.set_operator_(setBulletLevelOpt);
		const char* msgData = serializeMsgToBuffer(clientBuyuSetBulletRateReq, "set bullet rate request");
		if (msgData != NULL) rc = sendMsgToClient(msgData, clientBuyuSetBulletRateReq.ByteSize(), FishClient::CLIENT_BUYU_SET_BULLET_RATE_REQ, conn, rData->service_id);
	}
	
	rData->change_bullet_rate_timer_id = setTimer(changeSecs * 1000, (TimerHandler)&CSrvMsgHandler::changeBulletRate, userId, rData);

	// OptInfoLog("set bullet rate, id = %s, rate = %d, step = %d, next time = %d, timer id = %d, opt = %d, rc = %d",
	// rData->id, bulletRate, userId, changeSecs, rData->change_bullet_rate_timer_id, setBulletLevelOpt, rc);
}

// 定时使用道具
void CSrvMsgHandler::useProp(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	RobotData* rData = (RobotData*)param;
	Connect* conn = getConnect(rData->id);
	if (conn == NULL)
	{
		OptErrorLog("useProp, can not find the connect data, id = %s", rData->id);
		return;
	}
	
	// 配置参数校验
	const SimulationClientConfigData::PropUsage& propUsage = SimulationClientConfigData::config::getConfigValue().propUsage;
	if (propUsage.rate_time_min_percent < 1 || propUsage.rate_time_max_percent < 1 || propUsage.use_prop_time_min < 1 || propUsage.use_prop_time_max < 1
	    || propUsage.rate_time_min_percent > propUsage.rate_time_max_percent || propUsage.use_prop_time_min > propUsage.use_prop_time_max)
	{
		OptErrorLog("prop usage config error");
		return;
	}
	
	// 计算下次使用道具的时间点
	double numerator = getRandNumber(propUsage.rate_time_min_percent, propUsage.rate_time_max_percent);
	double denominator = getRandNumber(propUsage.rate_time_min_percent, propUsage.rate_time_max_percent);
	double percent = numerator / denominator;
	unsigned int minSecs = propUsage.use_prop_time_min * percent;
	unsigned int maxSecs = propUsage.use_prop_time_max * percent;
	unsigned int changeSecs = getRandNumber(minSecs, maxSecs);
	if (changeSecs < 1) changeSecs = 1;
    rData->use_prop_timer_id = setTimer(changeSecs * 1000, (TimerHandler)&CSrvMsgHandler::useProp, 1, rData);
	
    int rc = Opt_Failed;
	if (userId == 1 && rData->tableUsers->size() > 0)
	{
		static const int UsePropType[] = {EPropType::PropFlower, EPropType::PropMuteBullet, EPropType::PropSlipper};
	
	    // 随机用户，随机道具
		com_protocol::ClientBuyuUsePropReq clientBuyuUsePropReq;
		int index = getRandNumber(1, rData->tableUsers->size()) - 1;
		clientBuyuUsePropReq.set_dst_username((*(rData->tableUsers))[index]);
		
		index = getRandNumber(1, sizeof(UsePropType) / sizeof(int)) - 1;
		clientBuyuUsePropReq.set_prop_type(UsePropType[index]);
		const char* msgData = serializeMsgToBuffer(clientBuyuUsePropReq, "robot use prop request");
		if (msgData != NULL) rc = sendMsgToClient(msgData, clientBuyuUsePropReq.ByteSize(), FishClient::CLIENT_BUYU_USE_PROP_REQ, conn, rData->service_id);
	}

	// OptInfoLog("set use prop timer, id = %s, next time = %d, timer id = %d, rc = %d", rData->id, changeSecs, rData->use_prop_timer_id, rc);
}

// 机器人定时发言
void CSrvMsgHandler::doChat(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	RobotData* rData = (RobotData*)param;
	Connect* conn = getConnect(rData->id);
	if (conn == NULL)
	{
		OptErrorLog("to do chat, can not find the connect data, id = %s", rData->id);
		return;
	}
	
	// 配置参数校验
	const SimulationClientConfigData::ChatCfg& chatCfg = SimulationClientConfigData::config::getConfigValue().chatCfg;
	if (chatCfg.rate_time_min_percent < 1 || chatCfg.rate_time_max_percent < 1 || chatCfg.chat_time_min < 1 || chatCfg.chat_time_max < 1
	    || chatCfg.rate_time_min_percent > chatCfg.rate_time_max_percent || chatCfg.chat_time_min > chatCfg.chat_time_max)
	{
		OptErrorLog("prop usage config error");
		return;
	}
	
	// 计算下次使用道具的时间点
	double numerator = getRandNumber(chatCfg.rate_time_min_percent, chatCfg.rate_time_max_percent);
	double denominator = getRandNumber(chatCfg.rate_time_min_percent, chatCfg.rate_time_max_percent);
	double percent = numerator / denominator;
	unsigned int minSecs = chatCfg.chat_time_min * percent;
	unsigned int maxSecs = chatCfg.chat_time_max * percent;
	unsigned int changeSecs = getRandNumber(minSecs, maxSecs);
	if (changeSecs < 1) changeSecs = 1;
    rData->chat_timer_id = setTimer(changeSecs * 1000, (TimerHandler)&CSrvMsgHandler::doChat, 1, rData);
	
    int rc = Opt_Failed;
	if (userId == 1)
	{
	    // 随机聊天内容
		com_protocol::ClientTableChatReq clientTableChatReq;
		const vector<SimulationClientConfigData::Content>& chatContent = SimulationClientConfigData::config::getConfigValue().chatContent;
		int index = getRandNumber(1, chatContent.size()) - 1;
		clientTableChatReq.set_chat_content(chatContent[index].contents);
		const char* msgData = serializeMsgToBuffer(clientTableChatReq, "to do chat");
		if (msgData != NULL) rc = sendMsgToClient(msgData, clientTableChatReq.ByteSize(), FishClient::CLIENT_TABLE_CHAT_REQ, conn, rData->service_id);
	}

	// OptInfoLog("to do chat, id = %s, next time = %d, timer id = %d, rc = %d", rData->id, changeSecs, rData->chat_timer_id, rc);
}

void CSrvMsgHandler::queryUsernameReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::QueryUsernameRsp queryUsernameRsp;
	if (!parseMsgFromBuffer(queryUsernameRsp, data, len, "query robot user name reply")) return;

	// 普通机器人注册
	RobotUserNameVector* pFreeRobots = &m_normalFreeRobots;
	unsigned int robotInitNum = SimulationClientConfigData::config::getConfigValue().commonCfg.robot_init_num;
	if (getContext().userFlag == ERobotType::EVIPRobotPlatformType)  // VIP机器人注册
	{
		pFreeRobots = &m_vipFreeRobots;
		robotInitNum = SimulationClientConfigData::config::getConfigValue().commonCfg.vip_robot_init_num;
	}
	else if (getContext().userFlag != ERobotType::ERobotPlatformType)
	{
		OptErrorLog("query robot user name error, type = %d, count = %d", getContext().userFlag, queryUsernameRsp.usernames_size());
		stopService();
		return;
	}
	
    for (int i = 0; i < queryUsernameRsp.usernames_size(); ++i)
	{
		pFreeRobots->push_back(queryUsernameRsp.usernames(i));
	}
	
	OptInfoLog("query robot user name, type = %d, count = %d, robots = %d, init count = %d, normal = %d, vip = %d",
	getContext().userFlag, queryUsernameRsp.usernames_size(), pFreeRobots->size(), robotInitNum, m_normalFreeRobots.size(), m_vipFreeRobots.size());
	
	if (robotInitNum > pFreeRobots->size())
	{
		// 注册机器人以达到配置要求的个数
		registerRobot(getContext().userFlag, robotInitNum - pFreeRobots->size());
	}
	
	// only for test
	// testRobotLoginNotify(0, 3);
	// testRobotLoginNotify(1, 2);
}

// 直接忽略的消息，不需要做任何处理
void CSrvMsgHandler::dummyMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
}

// 测试代码，测试接口
void CSrvMsgHandler::testRobotLoginNotify(unsigned int table_id, unsigned int seat_id)
{
	// only for test
	com_protocol::RobotLoginNotify robotLoginNotifyMsg;
	robotLoginNotifyMsg.set_service_ip("192.168.1.2");
	robotLoginNotifyMsg.set_service_port(5332);
	robotLoginNotifyMsg.set_table_id(table_id);
	robotLoginNotifyMsg.set_seat_id(seat_id);
	robotLoginNotifyMsg.set_min_gold(100);
	robotLoginNotifyMsg.set_max_gold(1000);

	const char* msgData = serializeMsgToBuffer(robotLoginNotifyMsg, "do test for robot login");
	if (msgData != NULL) robotLoginNotify(msgData, robotLoginNotifyMsg.ByteSize(), 0, 0, 0);
}

void CSrvMsgHandler::testRobotLogoutNotify(const char* username)
{
	// only for test
	Context& ct = (Context&)getContext();
	strncpy(ct.userData, username, MaxUserDataLen);
	robotLogoutNotify("", 0, 0, 0, 0);
}


void CSrvMsgHandler::onClosedConnect(void* userData)
{
	RobotData* rData = (RobotData*)userData;
	OptInfoLog("robot passive close connect, id = %s, table = %d, seat = %d, status = %d", rData->id, rData->table_id, rData->seat_id, rData->status);
	
	// 回收机器人，回收内存块
	rData->status = RobotStatus::RobotRelease;
	releaseRobot(rData, false);
}

void CSrvMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("register protocol handler, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);

	// 注册协议处理函数
	// 收到的服务请求消息
	registerProtocol(BuyuGameSrv, SIMULATION_CLIENT_ROBOT_LOGIN_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::robotLoginNotify);
	registerProtocol(BuyuGameSrv, SIMULATION_CLIENT_ROBOT_LOGOUT_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::robotLogoutNotify);

	// 收到的CommonDB代理服务应答消息
	registerProtocol(DBProxyCommonSrv, BUSINESS_QUERY_USER_NAMES_RSP, (ProtocolHandler)&CSrvMsgHandler::queryUsernameReply);
	registerProtocol(DBProxyCommonSrv, BUSINESS_GET_USERNAME_BY_PLATFROM_RSP, (ProtocolHandler)&CSrvMsgHandler::registerRobotReply);
	
	// registerProtocol(DBProxyCommonSrv, SIMULATION_CLIENT_CLEAR_ROBOT_GOLD, (ProtocolHandler)&CSrvMsgHandler::clearRobotGoldReply);
	registerProtocol(DBProxyCommonSrv, BUSINESS_RESET_USER_OTHER_INFO_RSP, (ProtocolHandler)&CSrvMsgHandler::resetRobotInfoReply);
	
	// 忽略的游戏端消息协议ID
	const unsigned int MaxProtocolIDValue = 256;     // 最大协议ID值
	for (unsigned short i = 0; i < MaxProtocolIDValue; ++i)
	{
		registerProtocol(OutsideClientSrv, i, (ProtocolHandler)&CSrvMsgHandler::dummyMessage);
	}
	
	// 收到的游戏服务应答消息
	registerProtocol(OutsideClientSrv, CLIENT_ENTER_ROOM_VERIFY_RSP, (ProtocolHandler)&CSrvMsgHandler::enterServiceRoomReply);
	registerProtocol(OutsideClientSrv, CLIENT_BUYU_ENTER_TABLE_RSP, (ProtocolHandler)&CSrvMsgHandler::enterServiceTableReply);
	
	registerProtocol(OutsideClientSrv, CLIENT_BUYU_ENTER_TABLE_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::enterTableNotify);
	registerProtocol(OutsideClientSrv, CLIENT_BUYU_QUIT_NOTIFY, (ProtocolHandler)&CSrvMsgHandler::leaveTableNotify);

	//监控统计
	m_monitorStat.init(this);
	m_monitorStat.sendOnlineNotify();
}

void CSrvMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!SimulationClientConfigData::config::getConfigValue(CCfg::getValue("SimulationClientService", "BusinessXmlConfigFile")).isSetConfigValueSuccess())
	{
		OptErrorLog("set business xml config value error");
		stopService();
		return;
	}

	com_protocol::QueryUsernameReq queryUsernameReq;
	queryUsernameReq.set_platform_type(ERobotType::ERobotPlatformType);
	const char* msgData = serializeMsgToBuffer(queryUsernameReq, "query normal robot user name request");
	if (msgData != NULL) sendMessageToCommonDbProxy(msgData, queryUsernameReq.ByteSize(), BUSINESS_QUERY_USER_NAMES_REQ, "", 0, ERobotType::ERobotPlatformType);
	
	queryUsernameReq.set_platform_type(ERobotType::EVIPRobotPlatformType);
	msgData = serializeMsgToBuffer(queryUsernameReq, "query VIP robot user name request");
	if (msgData != NULL) sendMessageToCommonDbProxy(msgData, queryUsernameReq.ByteSize(), BUSINESS_QUERY_USER_NAMES_REQ, "", 0, ERobotType::EVIPRobotPlatformType);
}

void CSrvMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{

}

void CSrvMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	OptInfoLog("message handler running, service name = %s, id = %d, module = %d", srvName, srvId, moduleId);
}



// 主服务代码实现
int CSimulationClient::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("run service name = %s, id = %d", name, id);
	return 0;
}

void CSimulationClient::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("stop service name = %s, id = %d", name, id);
}

void CSimulationClient::onRegister(const char* name, const unsigned int id)
{
	ReleaseInfoLog("register module, service name = %s, id = %d", name, id);
	
	// 注册模块实例
	const unsigned short HandlerMessageModule = 0;
	static CSrvMsgHandler msgHandler;
	m_connectNotifyToHandler = &msgHandler;
	registerModule(HandlerMessageModule, &msgHandler);
}

void CSimulationClient::onUpdateConfig(const char* name, const unsigned int id)
{
	// 更新配置数据
	const SimulationClientConfigData::config& cfgValue = SimulationClientConfigData::config::getConfigValue(CCfg::getValue("SimulationClientService", "BusinessXmlConfigFile"), true);
	OptInfoLog("update config value, service name = %s, id = %d, result = %d", name, id, cfgValue.isSetConfigValueSuccess());
	cfgValue.output();
}

// 通知逻辑层对应的逻辑连接已被关闭
void CSimulationClient::onClosedConnect(void* userData)
{
	m_connectNotifyToHandler->onClosedConnect(userData);
}

CSimulationClient::CSimulationClient() : IService(SimulationClientSrv, true)
{
	m_connectNotifyToHandler = NULL;
}

CSimulationClient::~CSimulationClient()
{
	m_connectNotifyToHandler = NULL;
}

REGISTER_SERVICE(CSimulationClient);

}

