
/* author : limingfan
 * date : 2015.03.11
 * description : 网络消息收发
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "CService.h"
#include "base/ErrorCode.h"
#include "base/Constant.h"
#include "base/CThread.h"
#include "connect/CSocket.h"
#include "connect/CConnectManager.h"


using namespace NErrorCode;

namespace NFrame
{

static const int NetConnectCount = 1024;           // 网络连接长度
static const int WaitEventTimeOut = 1;             // 监听网络连接事件，等待时间，单位毫秒
		

extern CService& getService();  // 获取服务实例

		
// 网络连接管理线程
class CNetConnectThread : public CThread
{
public:
	CNetConnectThread() : m_connMgr(NULL), m_mode(DataHandlerType::NetDataParseActive), m_isRunning(false), m_runResult(Success)
	{
	}
	
	~CNetConnectThread()
	{
		m_connMgr = NULL;
		m_isRunning = false;
	}
	
public:
	int startNetConnect(const char* ip, const int port, ILogicHandler* logicHandler, const ServerCfgData& cfgData, DataHandlerType mode)
	{
		m_mode = mode;
		NEW(m_connMgr, CConnectManager(ip, port, logicHandler));
		int rc = m_connMgr->start(cfgData);
		if (rc != Success) return rc;
		
		rc = start();
		if (rc != Success) return rc;
		
		while (!m_isRunning)
		{
			usleep(1);
			if (m_runResult != Success) return m_runResult;
		}
		
		return Success;
	}
	
	void stopNetConnect()
	{
		if (m_connMgr != NULL)
		{
			m_connMgr->stop();
			while (m_isRunning) usleep(1);
			DELETE(m_connMgr);
		}
	}
	
private:
	virtual void run()
	{
		detach();  // 分离自己，线程结束则自动释放资源
		m_isRunning = true;
		m_runResult = m_connMgr->run(NetConnectCount, WaitEventTimeOut, m_mode);

		m_isRunning = false;
		stop();
	}
	
private:
	CConnectManager* m_connMgr;
	DataHandlerType m_mode;
	bool m_isRunning;
	int m_runResult;
};

// 整个服务框架只需要一个网络连接管理线程
static CNetConnectThread& getNetConnectThread()
{
	static CNetConnectThread instance;
	return instance;
}


CNetMsgComm::CNetMsgComm() : m_dataHandler(NULL), m_handlerMode(DataHandlerType::NetDataParseActive)
{
}

CNetMsgComm::~CNetMsgComm()
{
	m_dataHandler = NULL;
}

// ！说明：必须先初始化成功才能开始收发消息
int CNetMsgComm::init(CCfg* cfgData)
{
	// 检查服务网络配置项是否正确
	struct in_addr ipInAddr;
	const char* localIp = cfgData->getValue("NetConnect", "IP");
	if (CSocket::toIPInAddr(localIp, ipInAddr) != Success) return ServiceIPError;
	
	const char* nodePort = cfgData->getValue("NetConnect", "Port");
	if (nodePort == NULL || atoi(nodePort) < MinConnectPort) return ServicePortError;
	
	// NetConnect 配置项值检查
	const int MinMsgQueueSize = 1024;
	const char* netConnectCfg[] = {"MsgQueueSize", "ActiveTimeInterval", "HBInterval", "HBFailTimes", "CheckNoDataTimes",};
	for (int i = 0; i < (int)(sizeof(netConnectCfg) / sizeof(netConnectCfg[0])); ++i)
	{
		const char* value = cfgData->getValue("NetConnect", netConnectCfg[i]);
		if (value == NULL || atoi(value) <= 1) return SrvConnectCfgError;
		if (i == 0 && atoi(value) < MinMsgQueueSize) return SrvConnectCfgError;
	}
	
	ServerCfgData connCfg;
	connCfg.listenMemCache = NetConnectCount;    // 连接内存池内存块个数
	connCfg.listenNum = NetConnectCount;         // 监听连接的队列长度
	connCfg.count = NetConnectCount * 2;         // 读写缓冲区内存池内存块个数
	connCfg.step = NetConnectCount;              // 自动分配的内存块个数
	connCfg.size = atoi(CCfg::getValue("NetConnect", "MsgQueueSize"));                       // 每个读写数据区内存块的大小
	connCfg.activeInterval = atoi(CCfg::getValue("NetConnect", "ActiveTimeInterval"));       // 连接活跃检测间隔时间，单位秒，超过此间隔时间无数据则关闭连接
	connCfg.checkTimes = atoi(CCfg::getValue("NetConnect", "CheckNoDataTimes"));             // 检查最大socket无数据的次数，超过此最大次数则连接移出消息队列，避免遍历一堆无数据的空连接
	connCfg.hbInterval = atoi(CCfg::getValue("NetConnect", "HBInterval"));                   // 心跳检测间隔时间，单位秒
	connCfg.hbFailedTimes = atoi(CCfg::getValue("NetConnect", "HBFailTimes"));               // 心跳检测连续失败hbFailedTimes次后关闭连接

	const char* dataHandleMode = cfgData->getValue("NetConnect", "DataHandleMode");
	if (dataHandleMode != NULL) m_handlerMode = (DataHandlerType)atoi(dataHandleMode);
	
	// 启动网络连接管理服务
	int rc = getNetConnectThread().startNetConnect(localIp, atoi(nodePort), this, connCfg, m_handlerMode);
	if (rc != Success)
	{
		ReleaseErrorLog("start service net connect manager failed, rc = %d", rc);
		return rc;
	}

    return Success;
}

void CNetMsgComm::unInit()
{
	getNetConnectThread().stopNetConnect();
}

DataHandlerType CNetMsgComm::getDataHandlerMode()
{
	return m_handlerMode;
}
	
// 逻辑层直接从连接收发消息数据
ReturnValue CNetMsgComm::recv(Connect*& conn, char* data, unsigned int& len)
{
	return m_dataHandler->recv(conn, data, len);
}

ReturnValue CNetMsgComm::send(Connect* conn, const char* data, const unsigned int len, bool isNeedWriteMsgHeader)
{
	return m_dataHandler->send(conn, data, len, isNeedWriteMsgHeader);
}

void CNetMsgComm::close(Connect* conn)
{
	if (conn != NULL) m_dataHandler->close(conn);
}

	
// 连接建立的时候调用
// conn 为连接对应的数据，peerIp & peerPort 为连接对端的IP地址和端口号
// 网络管理层根据逻辑层返回码判断是否需要关闭该连接，返回码为 [CloseConnect, SendDataCloseConnect] 则关闭该连接
ReturnValue CNetMsgComm::onCreateConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, void*& cb)
{
	getService().createdClientConnect(conn, peerIp, peerPort);
	return ReturnValue::OptSuccess;
}

// 通知逻辑层connId对应的逻辑连接已被关闭
void CNetMsgComm::onClosedConnect(Connect* conn, void* cb)
{
	getService().closedClientConnect(conn);
}

// 本地端主动创建连接 peerIp&peerPort 为连接对端的IP、端口号
ActiveConnect* CNetMsgComm::getConnectData()
{
	return getService().getActiveConnectData();
}

// 主动连接建立的时候调用
// conn 为连接对应的数据，peerIp & peerPort 为连接对端的IP地址和端口号
// rtVal : 返回连接是否创建成功
// 网络管理层根据逻辑层返回码判断是否需要关闭该连接，返回码为 [CloseConnect, SendDataCloseConnect] 则关闭该连接
ReturnValue CNetMsgComm::doCreateConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, ReturnValue rtVal, void*& cb)
{
	getService().doCreateServiceConnect(conn, peerIp, peerPort, rtVal, cb);
	return ReturnValue::OptSuccess;
}

// 设置连接收发数据接口对象，暂不实现
void CNetMsgComm::setConnMgrInstance(IConnectMgr* instance)
{
	m_dataHandler = instance;
}





// 以下接口不会被用到，空实现
BufferHeader* CNetMsgComm::getBufferHeader()
{
	return NULL;
}

// 从逻辑层获取数据缓冲区，把从连接读到的数据写到该缓冲区	
// 返回值为 (char*)-1 则表示直接丢弃该数据；返回值为非NULL则读取数据；返回值为NULL则不读取数据
char* CNetMsgComm::getWriteBuffer(const int len, const uuid_type connId, BufferHeader* bufferHeader, void*& cb)
{
	return (char*)-1;
}

void CNetMsgComm::submitWriteBuffer(char* buff, const int len, const uuid_type connId, void* cb)
{
}

// 从逻辑层获取数据缓冲区，把该数据写入connLogicId对应的连接里
char* CNetMsgComm::getReadBuffer(int& len, uuid_type& connId, bool& isNeedWriteMsgHeader, void*& cb)
{
	return NULL;
}

void CNetMsgComm::submitReadBuffer(char* buff, const int len, const uuid_type connId, void* cb)
{
}

// 通知逻辑层connId对应的逻辑连接无效了，可能找不到，或者异常被关闭了，不能读写数据等等
void CNetMsgComm::onInvalidConnect(const uuid_type connId, void* cb)
{
}

// 直接从从逻辑层读写消息数据，多了一次中间数据拷贝，性能较低，暂不实现
ReturnValue CNetMsgComm::readData(char* data, int& len, uuid_type& connId)
{
	return OptFailed;
}

ReturnValue CNetMsgComm::writeData(const char* data, const int len, const uuid_type connId)
{
	return OptFailed;
}

}

