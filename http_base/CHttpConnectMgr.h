
/* author : limingfan
 * date : 2015.07.01
 * description : 连接管理
 */

#ifndef CHTTP_CONNECT_MGR_H
#define CHTTP_CONNECT_MGR_H

#include "base/CThread.h"
#include "base/MacroDefine.h"
#include "base/CMessageQueue.h"
#include "connect/CSocket.h"
#include "connect/CEpoll.h"
#include "HttpBaseDefine.h"


using namespace NCommon;
using namespace NConnect;

namespace NHttp
{

class CHttpConnectMgr : public CThread
{
public:
	CHttpConnectMgr();
	~CHttpConnectMgr();
	
public:
	int starSrv();
    void stopSrv();
	bool isRunning();
	
public:
    bool doHttpConnect(const char* ip, const unsigned short port, ConnectData* connData);

	ConnectData* receiveConnectData();
	bool haveConnectData();
	
private:
    void sendConnectData(ConnectData* connData);

	void closeHttpConnect(ConnectData* connData);

private:
    // http 消息调用
	void onActiveConnect(uint32_t eventVal, ConnectData* conn);
	void handleConnect(uint32_t eventVal, ConnectData* conn);
	
	void writeToConnect(ConnectData* conn);
	void readFromConnect(ConnectData* conn);
	
	int parseReadData(int nRead, ConnectData* conn);
	
private:
    // https 消息使用
    void doSSLConnect(ConnectData* connData);
	
	void writeToSSLConnect(ConnectData* connData);
	void readFromSSLConnect(ConnectData* connData);
	
private:
	virtual void run();  // 线程实现者重写run
	
private:
	CSocket m_activeConnect;          // 本地端主动对其他外部节点建立连接
	CEpoll m_epoll;                   // epoll IO 监听复用
	CAddressQueue m_dataQueue;        // 连接数据队列
	SrvStatus m_status;               // 连接服务状态


DISABLE_COPY_ASSIGN(CHttpConnectMgr);
};

}

#endif // CHTTP_CONNECT_MGR_H
