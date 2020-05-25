
/* author : admin
 * date : 2015.10.12
 * description : 网关代理服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <openssl/md5.h>

#include "CGatewayProxy.h"
#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"

#include "common/_DBConfig_.h"
#include "common/CRandomNumber.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;
using namespace NProject;


namespace NPlatformService
{

// 数据记录日志文件
#define WriteDataInfoLog(format, args...)   m_srvOpt.getDataLogger()->writeFile(NULL, 0, LogLevel::Info, format, ##args)
#define WriteDataErrorLog(format, args...)   m_srvOpt.getDataLogger()->writeFile(NULL, 0, LogLevel::Error, format, ##args)


CSrvMsgHandler::CSrvMsgHandler()
{
    m_pCfg = NULL;

	memset(&m_gatewayProxySrvData, 0, sizeof(m_gatewayProxySrvData));
	m_connectIndex = 0;
}

CSrvMsgHandler::~CSrvMsgHandler()
{
	memset(&m_gatewayProxySrvData, 0, sizeof(m_gatewayProxySrvData));
	m_connectIndex = 0;
}

// 收到网络客户端发往内部服务的消息				  
int CSrvMsgHandler::onClientMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
								    unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn)
{
    // 直接原大小回包，测性能用
    // sendMsgToClient(msgData, msgLen, protocolId, conn, serviceId, moduleId, msgId);
    // return Success;
    
    // webserver的心跳消息则特殊处理
    if (serviceId == EWebServerSystemMessageType::EHeartbeatType)
    {
        if (protocolId > 0 && msgId > 0)
        {
            const unsigned int curTimeSecs = time(NULL);
            m_gatewayProxySrvData.webServiceData.ip = htonl(msgId);
            m_gatewayProxySrvData.webServiceData.port = protocolId;
            m_gatewayProxySrvData.webServiceData.curTimeSecs = curTimeSecs;

            // 间隔 m_pCfg->commonCfg.webserver_heartbeat_log_interval 秒输出一次心跳日志
            static unsigned int lastTimeSecs = 0;
            if (curTimeSecs - lastTimeSecs > m_pCfg->commonCfg.webserver_heartbeat_log_interval)
            {
                lastTimeSecs = curTimeSecs;

                OptInfoLog("receive web server heartbeat, serviceId = %d, protocolId = %d, msgId = %s, ip = %s, port = %d, id = %lld, fd = %d, userCb = %p",
                serviceId, protocolId, CSocket::toIPStr(*(const struct in_addr*)&m_gatewayProxySrvData.webServiceData.ip), CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id, conn->fd, conn->userCb);
            }

            return Success;
        }
    }
    
    // webserver的连接地址消息则特殊处理
    if (serviceId == EWebServerSystemMessageType::EConnectAddressType)
    {
        if (protocolId > 0 && msgId > 0)
        {
            getConnectUserData(conn, serviceId, protocolId, htonl(msgId), protocolId);

            return Success;
        }
    }
    
    // 消息包合法性检测
    ConnectUserData* userData = getConnectUserData(conn, serviceId, protocolId);
    if (protocolId >= MaxProtocolIDCount || serviceId <= ServiceIdBaseValue)
	{
        WriteDataErrorLog("receive client message protocol or service error|%u|%d|%s|%d",
		serviceId, protocolId, CSocket::toIPStr(userData->connectAddress.peerIp), userData->connectAddress.peerPort);
        
		// OptErrorLog("receive client msg error, serviceId = %u, protocolId = %d, ip = %s, port = %d, id = %lld, fd = %d, userCb = %p",
		// serviceId, protocolId, CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id, conn->fd, conn->userCb);
		
        userData->closedErrorCode = ConnectProxyOperation::InvalidService;
        closeConnect(conn);  // 错误则关闭连接
        
		return InvalidParam;
	}
    
    // 消息目标服务类型检测
    const unsigned int serviceType = serviceId / ServiceIdBaseValue;
    if (serviceType >= MaxServiceType || !BIT_TST(m_serviceType, serviceType))
    {
        WriteDataErrorLog("receive client message service type error|%u|%d|%s|%d|%u|%u",
		serviceId, protocolId, CSocket::toIPStr(userData->connectAddress.peerIp), userData->connectAddress.peerPort, serviceType, MaxServiceType);
        
        // OptErrorLog("the message service type error, serviceId = %d, protocolId = %d, ip = %s, port = %d, id = %lld, fd = %d, userCb = %p, max allow = %u",
		// serviceId, protocolId, CSocket::toIPStr(conn->peerIp), conn->peerPort, conn->id, conn->fd, conn->userCb, MaxServiceType);

        userData->closedErrorCode = ConnectProxyOperation::InvalidService;
        closeConnect(conn);  // 错误则关闭连接

        return InvalidParam;
    }

    // 频繁收包检测
    if (++userData->currentClientMessageCount > m_serviceMaxPkgCount[serviceType])
    {
        const unsigned long long curMillSecs = getCurrentMillisecond();
        if ((curMillSecs - userData->currentClientMilliseconds) < m_pCfg->commonCfg.receive_interval_milliseconds)
        {
            WriteDataErrorLog("receive client frequent message error|%u|%d|%s|%d|%u|%d|%d",
            serviceId, protocolId, CSocket::toIPStr(userData->connectAddress.peerIp), userData->connectAddress.peerPort,
            (curMillSecs - userData->currentClientMilliseconds), m_serviceMaxPkgCount[serviceType], conn->userId);

            userData->closedErrorCode = ConnectProxyOperation::ClientFrequentMessage;
            closeConnect(conn);  // 错误则关闭连接
            
            return ReceiveMessageFrequently;
        }
        
        // 清零重新统计
        userData->currentClientMessageCount = 0;
        userData->currentClientMilliseconds = curMillSecs;
    }
    
    // 超大消息包检测
    unordered_map<unsigned int, unsigned int>::const_iterator otherPkgSizeIt = m_pCfg->commonCfg.other_pkg_size.find(serviceType * ServiceIdBaseValue + protocolId);
    const unsigned int maxPkgSize = (otherPkgSizeIt == m_pCfg->commonCfg.other_pkg_size.end()) ? m_pCfg->commonCfg.max_pkg_size : otherPkgSizeIt->second;
    if (msgLen > maxPkgSize)
    {
        WriteDataErrorLog("receive client oversized message error|%u|%d|%s|%d|%u|%u",
		serviceId, protocolId, CSocket::toIPStr(userData->connectAddress.peerIp), userData->connectAddress.peerPort, msgLen, maxPkgSize);

        userData->closedErrorCode = ConnectProxyOperation::OversizedMessage;
        closeConnect(conn);  // 错误则关闭连接
        
		return ReceiveMessageOversize;
    }

    // 同一个连接，如果是发往大厅服务的消息，必须替换为固定的目标大厅服务ID
    // 否则如果每次都路由取模会导致无法动态增减大厅服务，无法动态升级大厅服务
	if (serviceType == ServiceType::GameHallSrv) serviceId = userData->hallServiceId;  // 游戏大厅服务ID
    
    ++conn->userId;  // 连接上收到的消息包数量
    ++userData->sequence;  // 消息序号递增

    // 支持客户端使用同一个网络TCP连接，发送消息到不同的服务
	// 这里如果服务多了，可以改为位数组查找，这样效率比较高，目前没必要
	// 第一条消息会携带客户端连接地址信息
	const char* conAddrData = NULL;
	unsigned int conAddDataLen = 0;
	if (userData->serviceId2ConnectIdx.find(serviceId) == userData->serviceId2ConnectIdx.end())
	{
        const vector<unsigned int>& firstMessageProtocol = (serviceType != ServiceType::GameHallSrv) ? m_pCfg->commonCfg.game_first_message_protocol : m_pCfg->commonCfg.hall_first_message_protocol;
		if (std::find(firstMessageProtocol.begin(), firstMessageProtocol.end(), protocolId) == firstMessageProtocol.end())
        {
            // 严格验证，服务的第一条消息如果是非第一次发往服务的协议则错误
            OptErrorLog("first add service but not found protocol error, isClose = %d, serviceId = %d, protocolId = %d, userFlag = %d, ip = %s, port = %d, id = %lld, fd = %d",
            userData->serviceId2ConnectIdx.empty(), serviceId, protocolId, userData->index, CSocket::toIPStr(userData->connectAddress.peerIp), userData->connectAddress.peerPort, conn->id, conn->fd);

            WriteDataErrorLog("first add service protocol error|%u|%d|%s|%d|%u",
                serviceId, protocolId, CSocket::toIPStr(userData->connectAddress.peerIp), userData->connectAddress.peerPort, serviceType);
            userData->closedErrorCode = ConnectProxyOperation::InvalidService;
            closeConnect(conn);  // 错误则关闭连接
            
            return InvalidParam;
        }
        
        conAddrData = (const char*)&userData->connectAddress;
		conAddDataLen = sizeof(userData->connectAddress);
	}

	// 把客户端的网络消息转换成内部服务消息发送给指定的服务
	if (sendMessage(msgData, msgLen, userData->index, conAddrData, conAddDataLen, serviceId, protocolId, moduleId, 0, msgId) == Success && conAddDataLen > 0)
	{
		// 相同的服务ID直接覆盖，以避免使用数组存储时候的查找开销
		// 支持客户端使用同一个网络TCP连接，发送消息到不同的服务
		userData->serviceId2ConnectIdx[serviceId] = userData->index;
		
		OptInfoLog("first add service, serviceId = %d, protocolId = %d, userFlag = %d, ip = %s, port = %d, id = %lld, fd = %d",
		serviceId, protocolId, userData->index, CSocket::toIPStr(userData->connectAddress.peerIp), userData->connectAddress.peerPort, conn->id, conn->fd);
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

    ConnectUserData& cuData = it->second;
	if (srcProtocolId != ConnectProxyOperation::ActiveClosed)
	{
        // 频繁收包检测
        if (++cuData.currentServiceMessageCount > m_pCfg->commonCfg.receive_service_pkg_count)
        {
            const unsigned long long curMillSecs = getCurrentMillisecond();
            if ((curMillSecs - cuData.currentServiceMilliseconds) < m_pCfg->commonCfg.receive_service_pkg_interval)
            {
                WriteDataErrorLog("receive service frequent message error|%u|%d|%s|%d|%u|%d",
                srcSrvId, dstProtocolId, CSocket::toIPStr(cuData.connectAddress.peerIp), cuData.connectAddress.peerPort,
                (curMillSecs - cuData.currentServiceMilliseconds), userFlag);

                cuData.closedErrorCode = ConnectProxyOperation::ServiceFrequentMessage;
                closeConnect(cuData.conn);  // 错误则关闭连接
                
                return ReceiveMessageFrequently;
            }
            
            // 清零重新统计
            cuData.currentServiceMessageCount = 0;
            cuData.currentServiceMilliseconds = curMillSecs;
        }
    
        if (msgLen > m_pCfg->commonCfg.send_pkg_size)
        {
            WriteDataInfoLog("send client oversized message warn|%u|%d|%s|%d|%u|%u",
            srcSrvId, dstProtocolId, CSocket::toIPStr(cuData.connectAddress.peerIp), cuData.connectAddress.peerPort, msgLen, m_pCfg->commonCfg.send_pkg_size);
        }
        
		// 服务消息转发给网络客户端
	    if (srcSrvType == ServiceType::GameHallSrv) srcSrvId = m_gatewayProxySrvData.hallServiceId;  // 游戏大厅
	    sendMsgToClient(msgData, msgLen, dstProtocolId, cuData.conn, srcSrvId, srcModuleId, msgId);
	}
	else
	{
		// 这里不可以调用 resetUserData(cuData.conn, NULL) 屏蔽回调; 之后接着调用 closeConnect(cuData.conn)，然后删除连接数据
		// 实际上存在客户端和这里同时关闭连接的情况，因此可能在 resetUserData 之前，连接挂接的数据已经被连接线程放入队列里了
		// 这样的话会导致连接关闭函数 onClosedConnect 被回调时传递的参数是无效的，访问无效参数则直接崩溃
		
		cuData.serviceId2ConnectIdx.erase(srcSrvId);  // 连接被服务主动关闭了

		OptWarnLog("to do active close connect, srcSrvId = %d, userFlag = %d, ip = %s, port = %d, isEmpty = %d, index = %d",
		srcSrvId, userFlag, CSocket::toIPStr(cuData.connectAddress.peerIp), cuData.connectAddress.peerPort, cuData.serviceId2ConnectIdx.empty(), cuData.index);
		
		if (cuData.serviceId2ConnectIdx.empty()) closeConnect(cuData.conn);  // 该连接没有服务对应了，则关闭连接
	}
	
	return Success;
}

void CSrvMsgHandler::updateInfo2LoginList(uint32_t timerId, int userId, void* param, uint32_t remainCount)
{
    m_gatewayProxySrvData.hallServiceId = m_pCfg->commonCfg.game_hall_id;
	m_gatewayProxySrvData.curTimeSecs = time(NULL);

    int rc = SrvOptSuccess;
    const UIntVector serviceIds = getServiceIDList(ServiceType::LoginListSrv);
    for(UIntVector::const_iterator uIt = serviceIds.begin(); uIt != serviceIds.end(); ++uIt)
    {
        rc = m_srvOpt.sendMsgToService((const char*)&m_gatewayProxySrvData, sizeof(m_gatewayProxySrvData), *uIt, SERVICE_GATEWAY_UPDATE_LIST_NOTIFY);
        if (rc != SrvOptSuccess) OptErrorLog("send service info to login list service error, service id = %u, rc = %d", *uIt, rc);
    }
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

ConnectUserData* CSrvMsgHandler::getConnectUserData(Connect* conn, unsigned int serviceId, unsigned short protocolId, unsigned int ip, unsigned short port)
{
    ConnectUserData* userData = (ConnectUserData*)getUserData(conn);
    if (userData == NULL)
    {
        ++m_connectIndex;
        m_idx2Connects[m_connectIndex] = ConnectUserData();
        userData = &(m_idx2Connects[m_connectIndex]);
        userData->index = m_connectIndex;
        userData->conn = conn;
        
        getDestServiceID(ServiceType::GameHallSrv, userData->index, userData->hallServiceId);  // 确定游戏大厅服务ID
        
        if (ip == 0)
        {
            ip = conn->peerIp.s_addr;
            port = conn->peerPort;
        }

        userData->connectAddress.peerIp.s_addr = ip;
        userData->connectAddress.peerPort = port;
        
        // 随机生成消息序号及密钥
        userData->sequence = CRandomNumber::getUIntNumber(1, (unsigned int)-1);
        string seqStr(to_string(userData->sequence));
        MD5((const unsigned char*)seqStr.c_str(), seqStr.length(), userData->secretKey);
        
        conn->userId = 0;  // 连接上收到的消息包数量
        resetUserData(conn, userData);

        ++m_gatewayProxySrvData.currentPersons;

        char md5Str[64] = {0};
        b2str((const char*)userData->secretKey, MD5ResultBufferSize, md5Str, sizeof(md5Str));
        OptInfoLog("add new connect, hallId = %u, serviceId = %u, protocolId = %d, userFlag = %u, ip = %s, port = %d, id = %lld, fd = %d, conn = %p, sequence = %u, md5 = %s",
        userData->hallServiceId, serviceId, protocolId, userData->index, CSocket::toIPStr(userData->connectAddress.peerIp), userData->connectAddress.peerPort, conn->id, conn->fd, conn, userData->sequence, md5Str);
    }
    
    return userData;
}


// 服务配置更新
void CSrvMsgHandler::onUpdateConfig(bool isReset)
{
    if (isReset && !m_srvOpt.updateCommonConfig(CCfg::getValue("GatewayService", "ConfigFile")))
    {
        ReleaseErrorLog("update common xml config value error");
		
		stopService();
		return;
    }

	m_pCfg = &NGatewayProxyConfig::GatewayConfig::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str(), isReset);
	if (!m_pCfg->isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update business xml config value error");
		
		stopService();
		return;
	}
	
	if (!m_srvOpt.getDBCfg(isReset).isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update db xml config value error");
		
		stopService();
		return;
	}
    
    // 放行允许的目标服务消息，服务类型
    memset(m_serviceType, 0, sizeof(m_serviceType));
    for (vector<unsigned int>::const_iterator it = m_pCfg->commonCfg.allow_service_type.begin(); it != m_pCfg->commonCfg.allow_service_type.end(); ++it)
    {
        if (*it >= MaxServiceType)
        {
            ReleaseErrorLog("the allow service type error, value = %u, max allow = %u", *it, MaxServiceType);

            stopService();
            return;
        }
        
        BIT_SET(m_serviceType, *it);
    }
    
    // 服务间隔时间内最大收包数量
    for (unsigned int typeVal = 0; typeVal < MaxServiceType; ++typeVal) m_serviceMaxPkgCount[typeVal] = m_pCfg->commonCfg.receive_interval_count;
    for (unordered_map<unsigned int, unsigned int>::const_iterator it = m_pCfg->commonCfg.other_pkg_count.begin(); it != m_pCfg->commonCfg.other_pkg_count.end(); ++it)
    {
        if (it->first >= MaxServiceType)
        {
            ReleaseErrorLog("the other pkg count service type error, type = %u, count = %u", it->first, it->second);

            stopService();
            return;
        }
        
        m_serviceMaxPkgCount[it->first] = it->second;
    }
    
    // 删除需要过滤掉的大厅服务ID
    const unsigned int rmCount = removeDestServiceID(ServiceType::GameHallSrv, m_pCfg->commonCfg.filter_hall_service_list);

	// 更新timer
    // 定时保存数据到redis
	static unsigned int sTimerId = 0;
    if (sTimerId != 0) m_srvOpt.stopTimer(sTimerId);
    sTimerId = setTimer(MillisecondUnit * m_pCfg->commonCfg.save_data_interval, (TimerHandler)&CSrvMsgHandler::updateInfo2LoginList, 0, NULL, (unsigned int)-1);

	m_srvOpt.getDBCfg().output();
	m_pCfg->output();
    
    ReleaseInfoLog("update config finish, remove hall service count = %u", rmCount);
}

// 通知逻辑层对应的逻辑连接已被关闭
void CSrvMsgHandler::onClosedConnect(void* userData)
{
	// 连接被动关闭处理
	ConnectUserData* ud = (ConnectUserData*)userData;
    const char* optStr = (ud->closedErrorCode == ConnectProxyOperation::PassiveClosed) ? "passive" : "active";
	for (ServiceIDToConnectIdx::iterator it = ud->serviceId2ConnectIdx.begin(); it != ud->serviceId2ConnectIdx.end(); ++it)
	{
	    int rc = sendMessage(NULL, 0, ud->index, NULL, 0, it->first, 0, 0, ud->closedErrorCode, 0);  // 通知各个服务，对应的连接已经被关闭了

		OptWarnLog("send %s close connect msg, result = %d, srcSrvId = %d, userFlag = %d, ip = %s, port = %d, rc = %d",
		optStr, ud->closedErrorCode, it->first, ud->index, CSocket::toIPStr(ud->connectAddress.peerIp), ud->connectAddress.peerPort, rc);
	}
	
	OptWarnLog("on close connect, result = %d, userFlag = %d, ip = %s, port = %d, conn = %p, find = %d",
	ud->closedErrorCode, ud->index, CSocket::toIPStr(ud->connectAddress.peerIp), ud->connectAddress.peerPort, ud->conn, m_idx2Connects.find(ud->index) != m_idx2Connects.end());
		
	m_idx2Connects.erase(ud->index);  // 删除连接对应的数据
	
	if (m_gatewayProxySrvData.currentPersons > 0) --m_gatewayProxySrvData.currentPersons;
}


void CSrvMsgHandler::onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	ReleaseInfoLog("register protocol handler, srvName = %s, srvId = %d, moduleId = %d", srvName, srvId, moduleId);
}

void CSrvMsgHandler::onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
    if (!m_srvOpt.init(this, CCfg::getValue("GatewayService", "ConfigFile")))
	{
		ReleaseErrorLog("init service operation instance error");

		stopService();
		return;
	}
    
    // 刷新配置
    onUpdateConfig(false);
    
    // 创建数据日志配置
	if (!m_srvOpt.createDataLogger("DataLogger", "File", "Size", "BackupCount"))
	{
		ReleaseErrorLog("create data logger error");

		stopService();
		return;
	}

    const char* netIP = m_srvMsgCommCfg->get(srvName, "NetIP");
	m_gatewayProxySrvData.ip = CSocket::toIPInt(netIP);  // 外网IP
	m_gatewayProxySrvData.port = atoi(m_srvMsgCommCfg->get(srvName, "Port"));
    m_gatewayProxySrvData.hallServiceId = m_pCfg->commonCfg.game_hall_id;
	m_gatewayProxySrvData.curTimeSecs = time(NULL);
	m_gatewayProxySrvData.currentPersons = 0;

    ReleaseInfoLog("gateway message handler load, service name = %s, id = %d, net ip = %s, port = %d", srvName, srvId, netIP, m_gatewayProxySrvData.port);
}

void CSrvMsgHandler::onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	// 服务退出，关闭所有在线连接
	for (IndexToConnects::iterator connIt = m_idx2Connects.begin(); connIt != m_idx2Connects.end(); ++connIt)
	{
		for (ServiceIDToConnectIdx::iterator it = connIt->second.serviceId2ConnectIdx.begin(); it != connIt->second.serviceId2ConnectIdx.end(); ++it)
		{
			// 通知各个服务，对应的连接已经被关闭了
			sendMessage(NULL, 0, connIt->second.index, NULL, 0, it->first, 0, 0, ConnectProxyOperation::GatewayServiceStop, 0);
		}
		
		resetUserData(connIt->second.conn, NULL);  // 屏蔽连接关闭时的回调动作
		closeConnect(connIt->second.conn);
	}
	m_idx2Connects.clear();
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
	m_connectNotifyToHandler->onUpdateConfig(true);
}

// 收到外部数据之后调用 onReceiveMessage
// 发送外部数据之前调用 onSendMessage
// 一般用于数据加密&解密
int CGatewayProxySrv::onReceiveMessage(NConnect::Connect* conn, char* msg, unsigned int& len)
{
    if (!m_connectNotifyToHandler->need_encrypt_data()) return SrvOptSuccess;

    static const unsigned int sClientMsgHeaderSize = sizeof(ClientMsgHeader);
    ConnectUserData* userData = (ConnectUserData*)m_connectNotifyToHandler->getUserData(conn);
    ClientMsgHeader* msgHeader = (ClientMsgHeader*)msg;
    
    // 是否是webserver和网关的心跳消息、或者是webserver的第一条消息（发送IP&port数据），此时无需加解密处理
    // 外部业务第一条消息是登录消息，必须存在消息体（存在此限制，导致业务的第一条消息必须携带数据，对于webserver可以使用checksum&sequence字段填值替换该判断）
    if (userData == NULL && sClientMsgHeaderSize == len && msgHeader->msgLen == 0)
    {
        ReleaseWarnLog("receive first message but no body data, checksum = %u, sequence = %u, serviceId = %u, moduleId = %d, protocolId = %d, msgId = %u", 
        msgHeader->checksum, msgHeader->sequence, msgHeader->serviceId, msgHeader->moduleId, msgHeader->protocolId, msgHeader->msgId);
                
        return SrvOptSuccess;
    }

    do
    {
        int decryptLen = len - sizeof(msgHeader->checksum);
        char* decryptBuf = msg + sizeof(msgHeader->checksum);
    
        // 检测checksum
        unsigned int checksum = ntohl(msgHeader->checksum);
        unsigned int calChecksum = genChecksum(decryptBuf, decryptLen);
        if (checksum != calChecksum)
        {
            const struct in_addr peerIp = (userData != NULL) ? userData->connectAddress.peerIp : conn->peerIp;
            const unsigned short peerPort = (userData != NULL) ? userData->connectAddress.peerPort : conn->peerPort;

            ReleaseErrorLog("checksum not equal transport_checksum = %u local_cal_checksum = %u, ip = %s, port = %d, id = %lld, fd = %d, userCb = %p", 
                checksum, calChecksum, CSocket::toIPStr(peerIp), peerPort, conn->id, conn->fd, conn->userCb);
            break;
        }

        // 解密数据
        if (userData != NULL && userData->encryptionProcess == EEncryptionProcess::EBuildEncryptionFinish)  // 加解密流程已经建立完毕
        {
            // aes加解密要求长度为16的倍数
            if (0 != decryptLen % 16 || decryptLen <= 0)
            {
                ReleaseErrorLog("decryptLen not legal. decryptLen = %d, ip = %s, port = %d, id = %lld, fd = %d, userCb = %p", 
                    decryptLen, CSocket::toIPStr(userData->connectAddress.peerIp), userData->connectAddress.peerPort, conn->id, conn->fd, conn->userCb);
                break;
            }
            aesDecrypt(decryptBuf, decryptLen, (char*)(userData->secretKey));

            unsigned int sequence = ntohl(msgHeader->sequence);
            if (sequence != userData->sequence)
            {
                ReleaseErrorLog("sequence not legal. transport_sequence = %u, need_sequence = %u, ip = %s, port = %d, id = %lld, fd = %d, userCb = %p", 
                    sequence, userData->sequence, CSocket::toIPStr(userData->connectAddress.peerIp), userData->connectAddress.peerPort, conn->id, conn->fd, conn->userCb);
                break;
            }
        }
        else // 第一次收到消息
        {
            simpleEncryptDecrypt(decryptBuf, decryptLen);
        }

        return SrvOptSuccess;

    }while (false);

    // 解密失败则关闭连接
    m_connectNotifyToHandler->closeConnect(conn);
    return MessageDecryptError;
}

int CGatewayProxySrv::onSendMessage(NConnect::Connect* conn, char* msg, unsigned int& len)
{
    if (!m_connectNotifyToHandler->need_encrypt_data()) return SrvOptSuccess;

    ConnectUserData* userData = (ConnectUserData*)m_connectNotifyToHandler->getUserData(conn);
    ClientMsgHeader* msgHeader = (ClientMsgHeader*)msg;

    do
    {
        int encryptLen = len - sizeof(msgHeader->checksum);
        char* encryptBuf = msg + sizeof(msgHeader->checksum);

        // 非第一次收到消息后发送消息
        if (userData->encryptionProcess == EEncryptionProcess::EBuildEncryptionFinish)  // 加解密流程已经建立完毕
        {
            // aes加解密要求长度为16的倍数，不足时填充
            encryptLen = (encryptLen + 15) / 16 * 16;
            len = encryptLen + sizeof(msgHeader->checksum);
            aesEncrypt(encryptBuf, encryptLen, (char*)(userData->secretKey));
        }
        else // 第一次收到消息后发送消息
        {
            userData->encryptionProcess = EEncryptionProcess::EBuildEncryptionFinish;  // 发送第一条消息之后，加密流程建立完毕
            msgHeader->sequence = userData->sequence - 1;
            msgHeader->sequence = htonl(msgHeader->sequence);
            simpleEncryptDecrypt(encryptBuf, encryptLen);
        }

        msgHeader->checksum = htonl(genChecksum(encryptBuf, encryptLen));
        return SrvOptSuccess;

    } while (false);
    
    // 加密失败则关闭连接
    m_connectNotifyToHandler->closeConnect(conn);
    return MessageEncryptError;
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

