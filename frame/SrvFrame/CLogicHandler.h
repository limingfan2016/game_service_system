
/* author : limingfan
 * date : 2015.10.12
 * description : 游戏逻辑处理驱动
 */

#ifndef CLOGIC_HANDLER_H
#define CLOGIC_HANDLER_H

#include "FrameType.h"
#include "CModule.h"


namespace NFrame
{

// 网络客户端消息上下文内容
struct ConnectProxyContext
{
	unsigned int serviceId;         // 服务ID
	unsigned short moduleId;        // 服务下的模块ID
	unsigned short protocolId;      // 模块下的协议ID
	unsigned int msgId;             // 消息ID
    ConnectProxy* conn;             // 当前消息对应的连接代理
};


// 游戏逻辑处理驱动（和网关代理进行消息通信）
class CLogicHandler : public CModule
{
public:
	CLogicHandler();
	virtual ~CLogicHandler();


public:
	// 当前网络客户端消息上下文内容
	const ConnectProxyContext& getConnectProxyContext();

	// 设置新的上下文连接对象，返回之前的连接对象
	ConnectProxy* resetContextConnectProxy(ConnectProxy* newConn);
	
	// 停止连接代理，服务停止时调用
	void stopConnectProxy();
	
	// 清理停止连接代理，服务启动时调用，比如服务异常退出，则启动前调用该函数，关闭之前的代理连接
	void cleanUpConnectProxy(const unsigned int proxyId[], const unsigned int len);
	
public:
    // 获取代理信息
	uuid_type getProxyId(ConnectProxy* connProxy);
	ConnectProxy* getProxy(const uuid_type id);
	void closeProxy(const uuid_type id, int cbFlag = 0);      // 服务关闭用户连接时调用
	
	void* resetProxyUserData(ConnectProxy* conn, void* userData = NULL);
    void* getProxyUserData(ConnectProxy* conn);

public:
	// 向目标网络代理发送消息
    int sendMsgToProxy(const char* msgData, const unsigned int msgLen, unsigned short protocolId, unsigned int serviceId = 0, unsigned short moduleId = 0);
	int sendMsgToProxy(const char* msgData, const unsigned int msgLen, unsigned short protocolId,
					   const char* userName, unsigned int serviceId = 0, unsigned short moduleId = 0, unsigned int msgId = 0);
	int sendMsgToProxy(const char* msgData, const unsigned int msgLen, unsigned short protocolId,
					   ConnectProxy* conn, unsigned int serviceId = 0, unsigned short moduleId = 0, unsigned int msgId = 0);


	// 提供连接代理对象的保存、关闭、增删查等操作
public:
    // 应用上层如果不使用框架提供的连接代理容器，则以string key类型发送消息时需要重写该接口
    virtual ConnectProxy* getConnectProxy(const string& connId);
	
	void addConnectProxy(const string& connId, ConnectProxy* conn, void* userData = NULL);
	bool removeConnectProxy(const string& connId, int cbFlag = 0, bool doClose = true);
	bool haveConnectProxy(const string& connId);
	bool closeConnectProxy(const string& connId, int cbFlag = 0);
	void closeConnectProxy(ConnectProxy* conn, int cbFlag = 0);
	const IDToConnectProxys& getConnectProxy();
	bool resetProxyUserData(const string& connId, void* userData = NULL);

private:
    // 收到连接代理的数据
	virtual int onProxyMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
							   unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, ConnectProxy* conn);
	
	virtual void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	virtual bool onPreHandleMessage(const char* msgData, const unsigned int msgLen, const ConnectProxyContext& connProxyContext);
	

private:
    ConnectProxyContext m_connectProxyContext;
    ProtocolIdToHandler* m_proxyProtocolHanders;
	
	
	friend class CService;


DISABLE_COPY_ASSIGN(CLogicHandler);
};

}

#endif // CLOGIC_HANDLER_H
