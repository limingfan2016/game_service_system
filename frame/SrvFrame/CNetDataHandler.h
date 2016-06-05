
/* author : limingfan
 * date : 2015.06.30
 * description : 纯网络数据处理驱动
 */

#ifndef CNET_DATA_HANDLER_H
#define CNET_DATA_HANDLER_H

#include "CModule.h"
#include "FrameType.h"


namespace NFrame
{

// 纯网络数据处理驱动
class CNetDataHandler : public CModule
{
public:
	CNetDataHandler();
	virtual ~CNetDataHandler();
					
public:
	// 向目标网络客户端发送数据
    int sendDataToClient(Connect* conn, const char* data, const unsigned int len);
	int sendDataToClient(const char* connId, const char* data, const unsigned int len);
						
	// 提供连接对象的保存、关闭、增删查等操作
public:
    // 应用上层如果不使用框架提供的连接容器，则以string key类型发送消息时需要重写该接口
    virtual Connect* getConnect(const string& connId);
	
	void addConnect(const string& connId, Connect* conn, void* userData = NULL);
	bool removeConnect(const string& connId, bool doClose = true);
	bool haveConnect(const string& connId);
	void closeConnect(const string& connId);
	void closeConnect(Connect* conn);
	const IDToConnects& getConnect();
	void* resetUserData(Connect* conn, void* userData = NULL);
	bool resetUserData(const string& connId, void* userData = NULL);
    void* getUserData(Connect* conn);
	
public:
    // 本地端准备发起主动连接
    // peerIp&peerPort&userId 唯一标识一个主动连接，三者都相同则表示是同一个连接
    int doActiveConnect(ActiveConnectHandler cbFunction, const strIP_t peerIp, unsigned short peerPort, unsigned int timeOut, void* userCb = NULL, int userId = 0);
	
private:
    void doCreateActiveConnect(ActiveConnectData* activeConnectData);
	
private:
    // 收到网络客户端的数据
	virtual int onClientMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
								unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn);
						
    virtual int onClientData(Connect* conn, const char* data, const unsigned int len);
	
	virtual void onRegister(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	
	friend class CService;


DISABLE_COPY_ASSIGN(CNetDataHandler);
};

}

#endif // CNET_DATA_HANDLER_H
