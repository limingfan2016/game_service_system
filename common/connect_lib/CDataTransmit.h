
/* author : admin
 * date : 2015.06.30
 * description : 纯网络数据读写，不做数据解析、心跳监测等其他额外操作
 */
 
#ifndef CDATATRANSMIT_H
#define CDATATRANSMIT_H

#include "base/MacroDefine.h"
#include "ILogicHandler.h"
#include "NetType.h"


namespace NConnect
{

class CConnectManager;

// 纯网络数据读写，不做数据解析、心跳监测等其他额外操作
class CDataTransmit : public IConnectMgr
{
public:
	CDataTransmit(CConnectManager* connMgr, ILogicHandler* logicHandler);
	~CDataTransmit();

	// 由逻辑层主动收发消息
public:
	virtual ReturnValue recv(Connect*& conn, char* data, unsigned int& len);
    virtual ReturnValue send(Connect* conn, const char* data, const unsigned int len, bool isNeedWriteMsgHeader = true);
	virtual void close(Connect* conn);
	
private:
    CConnectManager* m_connMgr;
	Connect* m_curConn;
	ILogicHandler* m_logicHandler;    // 业务逻辑处理，读写数据
	
// 禁止构造&拷贝&赋值类函数
DISABLE_CONSTRUCTION_ASSIGN(CDataTransmit);
};

}

#endif  // CDATATRANSMIT_H
