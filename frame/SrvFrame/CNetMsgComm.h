
/* author : limingfan
 * date : 2015.03.11
 * description : 网络消息收发
 */

#ifndef CNETMSGCOMM_H
#define CNETMSGCOMM_H

#include <string>
#include <unordered_map>

#include "FrameType.h"
#include "base/MacroDefine.h"
#include "base/CCfg.h"
#include "connect/ILogicHandler.h"


using namespace std;
using namespace NCommon;
using namespace NConnect;

namespace NFrame
{

// 逻辑层实现该接口
// 使用连接管理的逻辑层实现的接口，连接管理模块调用
class CNetMsgComm : public ILogicHandler
{
public:
	CNetMsgComm();
	~CNetMsgComm();

    // ！说明：必须先初始化成功才能开始收发消息
public:
    int init(CCfg* cfgData);
	void unInit();
	
	DataHandlerType getDataHandlerMode();
	
public:
    // 逻辑层直接从连接收发消息数据
	ReturnValue recv(Connect*& conn, char* data, unsigned int& len);
    ReturnValue send(Connect* conn, const char* data, const unsigned int len, bool isNeedWriteMsgHeader = true);
	
	void close(Connect* conn);


	// 以下API为连接管理模块主动调用
private:
    // 本地端主动创建连接 peerIp&peerPort 为连接对端的IP、端口号
	virtual ActiveConnect* getConnectData();
	
	// 主动连接建立的时候调用
	// conn 为连接对应的数据，peerIp & peerPort 为连接对端的IP地址和端口号
	// 网络管理层根据逻辑层返回码判断是否需要关闭该连接，返回码为 [CloseConnect, SendDataCloseConnect] 则关闭该连接
    virtual ReturnValue doCreateConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, ReturnValue rtVal, void*& cb);

private:
    // 被动连接建立的时候调用
	// conn 为连接对应的数据，peerIp & peerPort 为连接对端的IP地址和端口号
	// 网络管理层根据逻辑层返回码判断是否需要关闭该连接，返回码为 [CloseConnect, SendDataCloseConnect] 则关闭该连接
    virtual ReturnValue onCreateConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, void*& cb);
	
	virtual void onClosedConnect(Connect* conn, void* cb);   // 通知逻辑层对应的逻辑连接已被关闭
	
	virtual void onInvalidConnect(const uuid_type connId, void* cb);  // 通知逻辑层connLogicId对应的逻辑连接无效了，可能找不到，或者异常被关闭了，不能读写数据等等
	
	
private:
    // 从逻辑层获取数据缓冲区，把从连接读到的数据写到该缓冲区
	virtual BufferHeader* getBufferHeader();
	
	// 返回值为 (char*)-1 则表示直接丢弃该数据；返回值为非NULL则读取数据；返回值为NULL则不读取数据
	virtual char* getWriteBuffer(const int len, const uuid_type connId, BufferHeader* bufferHeader, void*& cb);
	virtual void submitWriteBuffer(char* buff, const int len, const uuid_type connId, void* cb);

private:
    // 从逻辑层获取数据缓冲区，把该数据写入connLogicId对应的连接里
    virtual char* getReadBuffer(int& len, uuid_type& connId, bool& isNeedWriteMsgHeader, void*& cb);
	virtual void submitReadBuffer(char* buff, const int len, const uuid_type connId, void* cb);


private:
    // 直接从从逻辑层读写消息数据，多了一次中间数据拷贝，性能较低
	virtual ReturnValue readData(char* data, int& len, uuid_type& connId);
    virtual ReturnValue writeData(const char* data, const int len, const uuid_type connId);


private:
    // 设置连接收发数据接口对象
	virtual void setConnMgrInstance(IConnectMgr* instance);
	

private:
    IConnectMgr* m_dataHandler;
	DataHandlerType m_handlerMode;


DISABLE_COPY_ASSIGN(CNetMsgComm);
};

}

#endif // CNETMSGCOMM_H
