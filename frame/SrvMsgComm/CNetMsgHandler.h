
/* author : admin
 * date : 2015.01.19
 * description : 网络消息收发处理
 */

#ifndef CNETMSGHANDLER_H
#define CNETMSGHANDLER_H

#include "messageComm/MsgCommType.h"
#include "base/MacroDefine.h"
#include "base/CMemManager.h"
#include "connect/ILogicHandler.h"


using namespace NCommon;
using namespace NConnect;

namespace NMsgComm
{

// 逻辑层实现该接口
// 使用连接管理的逻辑层实现的接口，连接管理模块调用
class CNetMsgHandler : public ILogicHandler
{
public:
	CNetMsgHandler(ShmData* shmData, CfgData* cfgData);
	~CNetMsgHandler();

private:
    // 获取当前节点对应消息处理者的目标队列，写入网络消息
    MsgQueueList* getNodeMsgQueue(const unsigned int readerId);
	
	// 获取当前节点消息处理者发往外部网络节点的消息队列
	MsgQueueList* getNetMsgQueue(uuid_type& connId);
	MsgQueueList* getNetHandlerMsgQueue(uuid_type& connId);
	
private:
	void onRemoveConnect(void* cb);


	// 以下API为连接管理模块主动调用
private:
    // 本地端主动创建连接 peerIp&peerPort 为连接对端的IP、端口号
    // peerPort : 必须大于2000，防止和系统端口号冲突
    // timeOut : 最长timeOut秒后连接还没有建立成功则关闭，并回调返回创建连接超时；单位秒
	virtual ActiveConnect* getConnectData();
	
	// 主动连接建立的时候调用
	// conn 为连接对应的数据，peerIp&peerPort 为连接对端的IP地址和端口号
	// rtVal : 返回连接是否创建成功
	// 网络管理层根据逻辑层返回码判断是否需要关闭该连接，返回码为 [CloseConnect, SendDataCloseConnect] 则关闭该连接
    virtual ReturnValue doCreateConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, ReturnValue rtVal, void*& cb);
	

private:
    // 被动连接建立的时候调用
	// conn 为连接对应的数据，peerIp & peerPort 为连接对端的IP地址和端口号
	// 网络管理层根据逻辑层返回码判断是否需要关闭该连接，返回码为 [CloseConnect, SendDataCloseConnect] 则关闭该连接
    virtual ReturnValue onCreateConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, void*& cb);
	
private:
	virtual void onClosedConnect(Connect* conn, void* cb);            // 通知逻辑层对应的逻辑连接已被关闭
	virtual void onInvalidConnect(const uuid_type connId, void* cb);  // 通知逻辑层connId对应的逻辑连接无效了，可能找不到，或者异常被关闭了，不能读写数据等等
	
	
private:
    // 从逻辑层获取数据缓冲区，把从连接读到的数据写到该缓冲区
	// 返回NULL值则不会读取数据；返回非NULL值则先从连接缓冲区读取len的数据长度，做为getWriteBuffer接口的bufferHeader输入参数
	// 该接口一般做为先读取消息头部，用于解析消息头部使用
	virtual BufferHeader* getBufferHeader();
	
	// 返回值为 (char*)-1 则表示直接丢弃该数据；返回值为非NULL则读取数据；返回值为NULL则不读取数据
	virtual char* getWriteBuffer(const int len, const uuid_type connId, BufferHeader* bufferHeader, void*& cb);
	virtual void submitWriteBuffer(char* buff, const int len, const uuid_type connId, void* cb);

private:
    // 从逻辑层获取数据缓冲区，把该数据写入connLogicId对应的连接里
	// isNeedWriteMsgHeader 是否需要写入网络数据消息头部信息，默认会写消息头信息
	// isNeedWriteMsgHeader 如果逻辑层调用者填值为false，则调用者必须自己写入网络消息头部数据，即返回的可读buff中已经包含了网络消息头数据
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
    char* m_shmAddr;
	ShmData* m_shm;
    CfgData* m_cfg;
	
	MsgQueueList** m_nodeMsgQueueList;
	MsgHandlerList** m_netMsgHandlers;
	unsigned int m_curNetHandlerIdx;
	QueueIndex m_curNetQueueIdx;
	
	CMemManager* m_memForActiveConn;
	ActiveConnect* m_activeConnects;

	NetMsgHeader m_netMsgHeader;
	BufferHeader m_buffHeader;


DISABLE_CONSTRUCTION_ASSIGN(CNetMsgHandler);
};

}


#endif  // CNETMSGHANDLER_H

