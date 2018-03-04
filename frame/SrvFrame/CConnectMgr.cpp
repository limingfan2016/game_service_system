
/* author : limingfan
 * date : 2015.10.12
 * description : 连接对象管理
 */

#include "CConnectMgr.h"
#include "CNetMsgComm.h"
#include "CService.h"


namespace NFrame
{

static const unsigned int MaxProxyDataCount = 512;  // 为代理数据一次性分配的内存块个数

extern CService& getService();  // 获取服务实例


CConnectMgr::CConnectMgr() : m_memForProxyData(MaxProxyDataCount, MaxProxyDataCount, sizeof(ConnectProxy))
{
	m_netMsgComm = NULL;
}

CConnectMgr::~CConnectMgr()
{
	m_netMsgComm = NULL;
}

void CConnectMgr::setNetMsgComm(CNetMsgComm* netMsgComm)
{
	m_netMsgComm = netMsgComm;
}

// 提供 实际网络连接对象 的保存、关闭、增删查等（应用上层）操作
void CConnectMgr::addConnect(const string& connId, Connect* conn, void* userData)
{
	conn->userCb = userData;
	m_id2Connects[connId] = conn;
}

bool CConnectMgr::removeConnect(const string& connId, bool doClose)
{
	IDToConnects::iterator it = m_id2Connects.find(connId);
    if (it != m_id2Connects.end())
	{
		Connect* conn = it->second;
		m_id2Connects.erase(it);
		if (doClose) m_netMsgComm->close(conn);       // 必须先删除再回调，存在回调函数里应用上层遍历m_id2Connects的场景
		return true;
	}
	
	return false;
}

bool CConnectMgr::haveConnect(const string& connId)
{
    return (m_id2Connects.find(connId) != m_id2Connects.end());
}

void CConnectMgr::closeConnect(const string& connId)
{
	IDToConnects::iterator it = m_id2Connects.find(connId);
    if (it != m_id2Connects.end()) m_netMsgComm->close(it->second);
}

void CConnectMgr::closeConnect(Connect* conn)
{
	if (conn != NULL) m_netMsgComm->close(conn);
}

Connect* CConnectMgr::getConnect(const string& connId)
{
	IDToConnects::iterator it = m_id2Connects.find(connId);
    return (it != m_id2Connects.end()) ? it->second : NULL;
}

const IDToConnects& CConnectMgr::getConnect()
{
	return m_id2Connects;
}

void* CConnectMgr::getUserData(Connect* conn)
{
	return (conn != NULL) ? conn->userCb : NULL;
}



// 提供 连接代理对象 的保存、关闭、增删查等（应用上层）操作
void CConnectMgr::addConnectProxy(const string& connId, ConnectProxy* conn, void* userData)
{
	conn->userCb = userData;
	m_id2ConnectProxys[connId] = conn;
}

bool CConnectMgr::removeConnectProxy(const string& connId, bool doClose, int cbFlag)
{
	IDToConnectProxys::iterator it = m_id2ConnectProxys.find(connId);
    if (it != m_id2ConnectProxys.end())
	{
		ConnectProxy* conn = it->second;
		m_id2ConnectProxys.erase(it);      // 先删除，否则如果回调函数closeProxy里也执行同样的删除操作会导致it失效错误
		if (doClose) getService().closeProxy(conn, true, cbFlag);
		return true;
	}
	
	return false;
}

bool CConnectMgr::haveConnectProxy(const string& connId)
{
    return (m_id2ConnectProxys.find(connId) != m_id2ConnectProxys.end());
}

bool CConnectMgr::closeConnectProxy(const string& connId, int cbFlag)
{
	return removeConnectProxy(connId, true, cbFlag);
}

void CConnectMgr::closeConnectProxy(ConnectProxy* conn, int cbFlag)
{
	if (conn != NULL) getService().closeProxy(conn, true, cbFlag);
}

void CConnectMgr::clearConnectProxy()
{
	m_id2ConnectProxys.clear();
}

ConnectProxy* CConnectMgr::getConnectProxy(const string& connId)
{
	IDToConnectProxys::iterator it = m_id2ConnectProxys.find(connId);
    return (it != m_id2ConnectProxys.end()) ? it->second : NULL;
}

const IDToConnectProxys& CConnectMgr::getConnectProxy()
{
	return m_id2ConnectProxys;
}

void* CConnectMgr::getProxyUserData(ConnectProxy* conn)
{
	return (conn != NULL) ? conn->userCb : NULL;
}



// 代理操作（框架底层）
void CConnectMgr::addProxy(uuid_type flag, ConnectProxy* conn)
{
	m_flag2Proxys[flag] = conn;
}

ConnectProxy* CConnectMgr::removeProxy(uuid_type flag)
{
	return removeProxy(m_flag2Proxys.find(flag));
}

ConnectProxy* CConnectMgr::removeProxy(FlagToProxys::iterator it)
{
	ConnectProxy* conn = NULL;
	if (it != m_flag2Proxys.end())
	{
		conn = it->second;
		m_flag2Proxys.erase(it);
	}
	
	return conn;
}

void CConnectMgr::clearProxy()
{
	m_flag2Proxys.clear();
}

bool CConnectMgr::haveProxy(uuid_type flag, FlagToProxys::iterator& it)
{
	it = m_flag2Proxys.find(flag);
    return (it != m_flag2Proxys.end());
}

ConnectProxy* CConnectMgr::getProxy(uuid_type flag)
{
	FlagToProxys::iterator it = m_flag2Proxys.find(flag);
    return (it != m_flag2Proxys.end()) ? it->second : NULL;
}

const FlagToProxys& CConnectMgr::getProxy()
{
	return m_flag2Proxys;
}

ConnectProxy* CConnectMgr::createProxy()
{
	return (ConnectProxy*)m_memForProxyData.get();
}

void CConnectMgr::destroyProxy(ConnectProxy*& conn)
{
	m_memForProxyData.put((char*)conn);
	conn = NULL;
}

}

