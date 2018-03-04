
/* author : limingfan
 * date : 2015.10.12
 * description : 网关代理服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "CGatewayProxy.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"

#include "common/_DBConfig_.h"
#include "_GatewayProxyConfig_.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace NDBOpt;
using namespace NProject;


namespace NPlatformService
{

CSrvMsgHandler::CSrvMsgHandler()
{
	memset(&m_gatewayProxySrvData, 0, sizeof(m_gatewayProxySrvData));
	m_connectIndex = 0;
	
	m_gameHallId = 0;
}

CSrvMsgHandler::~CSrvMsgHandler()
{
	memset(&m_gatewayProxySrvData, 0, sizeof(m_gatewayProxySrvData));
	m_connectIndex = 0;
	
	m_gameHallId = 0;
}

// 收到网络客户端发往内部服务的消息				  
int CSrvMsgHandler::onClientMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
								    unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn)
{
    if (protocolId >= MaxProtocolIDCount || serviceId <= ServiceIdBaseValue)
	{
		OptErrorLog("receive net client msg error, serviceId = %d, protocolId = %d, ip = %s, port = %d, id = %lld, fd = %d, userCb = %p",
		serviceId, protocolId, CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id, conn->fd, conn->userCb);
		
		return InvalidParam;
	}

	ConnectUserData* userData = (ConnectUserData*)getUserData(conn);
	if (userData == NULL)
	{
		++m_connectIndex;
		m_idx2Connects[m_connectIndex] = ConnectUserData();
		userData = &(m_idx2Connects[m_connectIndex]);
		userData->index = m_connectIndex;
		userData->conn = conn;
		resetUserData(conn, userData);
		
		++m_gatewayProxySrvData.currentPersons;
		
		OptInfoLog("add new connect, serviceId = %d, userFlag = %d, protocolId = %d, ip = %s, port = %d, id = %lld, fd = %d",
		serviceId, userData->index, protocolId, CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id, conn->fd);
	}
	
	const unsigned int serviceType = serviceId / ServiceIdBaseValue;
	if (serviceType == ServiceType::GameHallSrv) getDestServiceID(serviceType, userData->index, serviceId);  // 游戏大厅

    // 支持客户端使用同一个网络TCP连接，发送消息到不同的服务
	// 这里如果服务多了，可以改为位数组查找，这样效率比较高，目前没必要
	// 第一条消息会携带客户端连接地址信息
	const char* conAddrData = NULL;
	unsigned int conAddDataLen = 0;
	if (userData->serviceId2ConnectIdx.find(serviceId) == userData->serviceId2ConnectIdx.end())
	{
		static ConnectAddress connectAddress;
		connectAddress.peerIp = conn->peerIp;
		connectAddress.peerPort = conn->peerPort;
		conAddrData = (const char*)&connectAddress;
		conAddDataLen = sizeof(connectAddress);
	}
	
	// 把客户端的网络消息转换成内部服务消息发送给指定的服务
	if (sendMessage(msgData, msgLen, userData->index, conAddrData, conAddDataLen, serviceId, protocolId, moduleId, 0, msgId) == Success && conAddDataLen > 0)
	{
		// 相同的服务ID直接覆盖，以避免使用数组存储时候的查找开销
		// 支持客户端使用同一个网络TCP连接，发送消息到不同的服务
		userData->serviceId2ConnectIdx[serviceId] = userData->index;
		
		OptInfoLog("first add service, serviceId = %d, userFlag = %d, protocolId = %d, ip = %s, port = %d, id = %lld, fd = %d",
		serviceId, userData->index, protocolId, CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id, conn->fd);
	}
	
	return Success;
}

// 收到内部服务发往网络客户端的消息				
int CSrvMsgHandler::onServiceMessage(const char* msgData, const unsigned int msgLen, const char* userData, const unsigned int userDataLen,
							         unsigned short dstProtocolId, unsigned int srcSrvId, unsigned short srcSrvType,
							         unsigned short srcModuleId, unsigned short srcProtocolId, int userFlag, unsigned int msgId, const char* srvAsyncDataFlag, unsigned int srvAsyncDataFlagLen)
{
	if (dstProtocolId >= MaxProtocolIDCount)
	{
		OptErrorLog("receive error service msg, srcServiceId = %d, srcServiceType = %d, dstProtocolId = %d", srcSrvId, srcSrvType, dstProtocolId);
		return InvalidParam;
	}

	// 服务要求停止连接代理
	if (srcProtocolId == ConnectProxyOperation::StopProxy) return stopServiceConnect(srcSrvId);
	
	IndexToConnects::iterator it = m_idx2Connects.find(userFlag);
	if (it == m_idx2Connects.end())
	{
		int rc = sendMessage(NULL, 0, userFlag, NULL, 0, srcSrvId, 0, 0, ConnectProxyOperation::PassiveClosed, 0);  // 通知服务，对应的连接已经被关闭了
		
		OptErrorLog("can not find the connect proxy data, srcServiceId = %d, srcServiceType = %d, dstProtocolId = %d, userFlag = %d, rc = %d",
		srcSrvId, srcSrvType, dstProtocolId, userFlag, rc);
		
		return InvalidParam;
	}

	if (srcProtocolId != ConnectProxyOperation::ActiveClosed)
	{
		// 服务消息转发给网络客户端
	    if (srcSrvType == ServiceType::GameHallSrv) srcSrvId = m_gameHallId;  // 游戏大厅
	    sendMsgToClient(msgData, msgLen, dstProtocolId, it->second.conn, srcSrvId, srcModuleId, msgId);
	}
	else
	{
		// 这里不可以调用 resetUserData(it->second.conn, NULL) 屏蔽回调; 之后接着调用 closeConnect(it->second.conn)，然后删除连接数据
		// 实际上存在客户端和这里同时关闭连接的情况，因此可能在 resetUserData 之前，连接挂接的数据已经被连接线程放入队列里了
		// 这样的话会导致连接关闭函数 onClosedConnect 被回调时传递的参数是无效的，访问无效参数则直接崩溃
		
		it->second.serviceId2ConnectIdx.erase(srcSrvId);  // 连接被服务主动关闭了
		
		OptWarnLog("to do active close connect, srcSrvId = %d, userFlag = %d, ip = %s, port = %d, isEmpty = %d, index = %d",
		srcSrvId, userFlag, CSocket::toIPStr(it->second.conn->peerIp), it->second.conn->peerPort, it->second.serviceId2ConnectIdx.empty(), it->second.index);
		
		if (it->second.serviceId2ConnectIdx.empty()) closeConnect(it->second.conn);  // 该连接没有服务对应了，则关闭连接
	}
	
	return Success;
}


// 定时保存数据到redis服务
void CSrvMsgHandler::saveDataToDb(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	static const unsigned int gatewayProxySrvId = getSrvId();
	m_gatewayProxySrvData.curTimeSecs = time(NULL);
	int rc = m_redisDbOpt.setHField(GatewayProxyListKey, GatewayProxyListKeyLen,
	                                (const char*)&gatewayProxySrvId, sizeof(gatewayProxySrvId), (const char*)&m_gatewayProxySrvData, sizeof(m_gatewayProxySrvData));
	if (rc != 0) OptErrorLog("in saveDataToDb set gateway proxy service data to redis center service failed, rc = %d", rc);
}

// 停止该服务的所有连接
int CSrvMsgHandler::stopServiceConnect(unsigned int srcSrvId)
{
	OptWarnLog("stop service connect, id = %u", srcSrvId);
	
	// 关闭该服务的所有连接
	for (IndexToConnects::iterator connIt = m_idx2Connects.begin(); connIt != m_idx2Connects.end(); ++connIt)
	{
		ServiceIDToConnectIdx::iterator srvIdxIt = connIt->second.serviceId2ConnectIdx.find(srcSrvId);
		if (srvIdxIt != connIt->second.serviceId2ConnectIdx.end())
		{
			// 这里不可以调用 resetUserData(it->second.conn, NULL) 屏蔽回调; 之后接着调用 closeConnect(it->second.conn)，然后删除连接数据
		    // 实际上存在客户端和这里同时关闭连接的情况，因此可能在 resetUserData 之前，连接挂接的数据已经被连接线程放入队列里了
		    // 这样的话会导致连接关闭函数 onClosedConnect 被回调时传递的参数是无效的，访问无效参数则直接崩溃
			// 也因此这里不能调用 m_idx2Connects.erase(connIt++); 来删除连接数据，必须在回调函数 onClosedConnect 里调用
			connIt->second.serviceId2ConnectIdx.erase(srvIdxIt);  // 连接被服务主动关闭了
		    if (connIt->second.serviceId2ConnectIdx.empty()) closeConnect(connIt->second.conn);  // 该连接没有服务对应了，则关闭连接
		}
	}
	
	return Success;
}

// 服务配置更新
void CSrvMsgHandler::onUpdateConfig()
{
	m_gameHallId = GatewayProxyConfig::config::getConfigValue().commonCfg.game_hall_id;
}

// 通知逻辑层对应的逻辑连接已被关闭
void CSrvMsgHandler::onClosedConnect(void* userData)
{
	// 连接被动关闭处理
	ConnectUserData* ud = (ConnectUserData*)userData;
	for (ServiceIDToConnectIdx::iterator it = ud->serviceId2ConnectIdx.begin(); it != ud->serviceId2ConnectIdx.end(); ++it)
	{
	    int rc = sendMessage(NULL, 0, ud->index, NULL, 0, it->first, 0, 0, ConnectProxyOperation::PassiveClosed, 0);  // 通知各个服务，对应的连接已经被关闭了
		
		OptWarnLog("send passive close connect msg, srcSrvId = %d, userFlag = %d, ip = %s, port = %d, rc = %d",
		it->first, ud->index, CSocket::toIPStr(ud->conn->peerIp), ud->conn->peerPort, rc);
	}
	
	OptWarnLog("on close connect, userFlag = %d, ip = %s, port = %d, find = %d",
	ud->index, CSocket::toIPStr(ud->conn->peerIp), ud->conn->peerPort, m_idx2Connects.find(ud->index) != m_idx2Connects.end());
		
	m_idx2Connects.erase(ud->index);  // 删除连接对应的数据
	
	if (m_gatewayProxySrvData.currentPersons > 0) --m_gatewayProxySrvData.currentPersons;
}


void CSrvMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("register protocol handler, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}

void CSrvMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!GatewayProxyConfig::config::getConfigValue(CCfg::getValue("GatewayProxyService", "BusinessXmlConfigFile")).isSetConfigValueSuccess())
	{
		ReleaseErrorLog("set business xml config value error");
		stopService();
		return;
	}
	
    const DBConfig::config& dbCfg = DBConfig::config::getConfigValue(CCfg::getValue("GatewayProxyService", "DbConfigFile"));
	if (!dbCfg.isSetConfigValueSuccess())
	{
		ReleaseErrorLog("set db xml config value error");
		stopService();
		return;
	}

	if (!m_redisDbOpt.connectSvr(dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port,
	    dbCfg.redis_db_cfg.center_db_timeout * MillisecondUnit))
	{
		ReleaseErrorLog("gateway connect center redis service failed, ip = %s, port = %u, time out = %u",
		dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port, dbCfg.redis_db_cfg.center_db_timeout);
		
		stopService();
		return;
	}
	
	static const unsigned int gatewayProxySrvId = getSrvId();
	m_gatewayProxySrvData.ip = CSocket::toIPInt(CCfg::getValue("NetConnect", "NetIP"));  // 外网IP
	m_gatewayProxySrvData.port = atoi(CCfg::getValue("NetConnect", "Port"));
	m_gatewayProxySrvData.curTimeSecs = time(NULL);
	m_gatewayProxySrvData.currentPersons = 0;
	int rc = m_redisDbOpt.setHField(GatewayProxyListKey, GatewayProxyListKeyLen,
	                                (const char*)&gatewayProxySrvId, sizeof(gatewayProxySrvId), (const char*)&m_gatewayProxySrvData, sizeof(m_gatewayProxySrvData));
	if (rc != 0)
	{
		ReleaseErrorLog("set gateway proxy service data to redis center service failed, rc = %d", rc);
		stopService();
		return;
	}
	
	m_gameHallId = GatewayProxyConfig::config::getConfigValue().commonCfg.game_hall_id;
	
	// 定时保存数据到redis
	unsigned int millisecondUnit = 1000; // 秒转换为毫秒乘值
	unsigned int timerId = setTimer(millisecondUnit * atoi(CCfg::getValue("GatewayProxyService", "SaveDataToDBInterval")),
	                                (TimerHandler)&CSrvMsgHandler::saveDataToDb, 0, NULL, (unsigned int)-1);

    ReleaseInfoLog("gateway message handler load, service name = %s, id = %d, timer id = %u", srvName, srvId, timerId);
}

void CSrvMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 服务退出，关闭所有在线连接
	for (IndexToConnects::iterator connIt = m_idx2Connects.begin(); connIt != m_idx2Connects.end(); ++connIt)
	{
		for (ServiceIDToConnectIdx::iterator it = connIt->second.serviceId2ConnectIdx.begin(); it != connIt->second.serviceId2ConnectIdx.end(); ++it)
		{
			// 通知各个服务，对应的连接已经被关闭了
			sendMessage(NULL, 0, connIt->second.index, NULL, 0, it->first, 0, 0, ConnectProxyOperation::PassiveClosed, 0);
		}
		
		resetUserData(connIt->second.conn, NULL);  // 屏蔽连接关闭时的回调动作
		closeConnect(connIt->second.conn);
	}
	m_idx2Connects.clear();
	
	m_redisDbOpt.disconnectSvr();
}

void CSrvMsgHandler::onRun(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
}



int CGatewayProxySrv::onInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("run gateway service name = %s, id = %d", name, id);
	return 0;
}

void CGatewayProxySrv::onUnInit(const char* name, const unsigned int id)
{
	ReleaseInfoLog("stop gateway service name = %s, id = %d", name, id);
}

void CGatewayProxySrv::onRegister(const char* name, const unsigned int id)
{
	ReleaseInfoLog("register gateway module, service name = %s, id = %d", name, id);
	
	// 注册模块实例
	const unsigned short HandlerMessageModule = 0;
	static CSrvMsgHandler msgHandler;
	m_connectNotifyToHandler = &msgHandler;
	registerModule(HandlerMessageModule, &msgHandler);
}

void CGatewayProxySrv::onUpdateConfig(const char* name, const unsigned int id)
{
	const GatewayProxyConfig::config& cfgValue = GatewayProxyConfig::config::getConfigValue(CCfg::getValue("GatewayProxyService", "BusinessXmlConfigFile"), true);
	const DBConfig::config& dbCfg = DBConfig::config::getConfigValue(CCfg::getValue("GatewayProxyService", "DbConfigFile"), true);

	m_connectNotifyToHandler->onUpdateConfig();

	cfgValue.output();
	dbCfg.output();
	
	ReleaseInfoLog("update config value, service name = %s, id = %d, gateway result = %d, db result = %d",
	name, id, cfgValue.isSetConfigValueSuccess(), dbCfg.isSetConfigValueSuccess());
}

// 通知逻辑层对应的逻辑连接已被关闭
void CGatewayProxySrv::onClosedConnect(void* userData)
{
	m_connectNotifyToHandler->onClosedConnect(userData);
}


CGatewayProxySrv::CGatewayProxySrv() : IService(GatewaySrv, true)
{
	m_connectNotifyToHandler = NULL;
}

CGatewayProxySrv::~CGatewayProxySrv()
{
	m_connectNotifyToHandler = NULL;
}

REGISTER_SERVICE(CGatewayProxySrv);

}

