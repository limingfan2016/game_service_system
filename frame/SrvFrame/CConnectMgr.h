
/* author : limingfan
 * date : 2015.10.12
 * description : 连接对象管理
 */

#ifndef CCONNECT_MGR_H
#define CCONNECT_MGR_H

#include <string>
#include <unordered_map>

#include "FrameType.h"
#include "base/MacroDefine.h"
#include "base/CMemManager.h"


using namespace std;
using namespace NCommon;
using namespace NConnect;

namespace NFrame
{

// 代理ID
union ProxyID
{
	uuid_type id;
	struct ProxyFlagData
	{
		int flag;
		unsigned int serviceId;
	} proxyFlag;
};
typedef unordered_map<uuid_type, ConnectProxy*> FlagToProxys;

typedef unordered_map<string, Connect*> IDToConnects;
typedef unordered_map<string, ConnectProxy*> IDToConnectProxys;


class CNetMsgComm;

// 连接对象管理
class CConnectMgr
{
public:
	CConnectMgr();
	~CConnectMgr();
	
public:
	void setNetMsgComm(CNetMsgComm* netMsgComm);

	// 提供 实际网络连接对象 的保存、关闭、增删查等（应用上层）操作
public:
	void addConnect(const string& connId, Connect* conn, void* userData = NULL);
	bool removeConnect(const string& connId, bool doClose = true);
	bool haveConnect(const string& connId);
	
	void closeConnect(const string& connId);
	void closeConnect(Connect* conn);
	
    Connect* getConnect(const string& connId);
	const IDToConnects& getConnect();
	void* getUserData(Connect* conn);
	
	// 提供 连接代理对象 的保存、关闭、增删查等（应用上层）操作
public:
	void addConnectProxy(const string& connId, ConnectProxy* conn, void* userData = NULL);
	bool removeConnectProxy(const string& connId, bool doClose = true, int cbFlag = 0);
	bool haveConnectProxy(const string& connId);
	
	bool closeConnectProxy(const string& connId, int cbFlag = 0);
	void closeConnectProxy(ConnectProxy* conn, int cbFlag = 0);
	void clearConnectProxy();
	
    ConnectProxy* getConnectProxy(const string& connId);
	const IDToConnectProxys& getConnectProxy();
	void* getProxyUserData(ConnectProxy* conn);
	
	
	// 代理操作（框架底层）
public:
	void addProxy(uuid_type flag, ConnectProxy* conn);
	ConnectProxy* removeProxy(uuid_type flag);
	ConnectProxy* removeProxy(FlagToProxys::iterator it);
	void clearProxy();
	bool haveProxy(uuid_type flag, FlagToProxys::iterator& it);
	
    ConnectProxy* getProxy(uuid_type flag);
	const FlagToProxys& getProxy();
	
	ConnectProxy* createProxy();
	void destroyProxy(ConnectProxy*& conn);


private:
	IDToConnects m_id2Connects;
	IDToConnectProxys m_id2ConnectProxys;
	FlagToProxys m_flag2Proxys;
	
	CNetMsgComm* m_netMsgComm;
	
	CMemManager m_memForProxyData;


DISABLE_COPY_ASSIGN(CConnectMgr);
};

}

#endif // CCONNECT_MGR_H
