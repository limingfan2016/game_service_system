
/* author : limingfan
 * date : 2015.03.27
 * description : 游戏逻辑开发框架API
 */

#ifndef CGAME_MODULE_H
#define CGAME_MODULE_H

#include "CNetDataHandler.h"


namespace NFrame
{

// 网络客户端消息上下文内容
struct NetClientContext
{
	unsigned int serviceId;       // 服务ID
	unsigned short moduleId;        // 服务下的模块ID
	unsigned short protocolId;      // 模块下的协议ID
	unsigned int msgId;             // 消息ID
    Connect* conn;                  // 当前消息对应的连接
};


// 游戏模块开发者接口
class CGameModule : public CNetDataHandler
{
public:
	CGameModule();
	virtual ~CGameModule();
	
public:
	// 当前网络客户端消息上下文内容
	const NetClientContext& getNetClientContext();
	
	// 设置新的上下文连接对象，返回之前的连接对象
	Connect* setContextConnect(Connect* newConn);

public:
	// 向目标网络客户端发送消息
    int sendMsgToClient(const char* msgData, const unsigned int msgLen, unsigned short protocolId, unsigned int serviceId = 0, unsigned short moduleId = 0);
	int sendMsgToClient(const char* msgData, const unsigned int msgLen, unsigned short protocolId,
	                    const char* userName, unsigned int serviceId = 0, unsigned short moduleId = 0, unsigned int msgId = 0);
	int sendMsgToClient(const char* msgData, const unsigned int msgLen, unsigned short protocolId,
	                    Connect* conn, unsigned int serviceId = 0, unsigned short moduleId = 0, unsigned int msgId = 0);

private:
    // 收到网络客户端的消息				  
	int onClientMessage(const char* msgData, const unsigned int msgLen, const unsigned int msgId,
						unsigned int serviceId, unsigned short moduleId, unsigned short protocolId, Connect* conn);
	
private:
	NetClientContext m_netClientContext;
    ProtocolIdToHandler* m_clientProtocolHanders;


DISABLE_COPY_ASSIGN(CGameModule);
};

}

#endif // CGAME_MODULE_H
