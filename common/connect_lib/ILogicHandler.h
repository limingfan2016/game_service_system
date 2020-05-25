
/* author : admin
 * date : 2014.12.16
 * description : 连接管理使用者，上层业务逻辑需实现的接口
 */
 
#ifndef ILOGIC_HANDLER_H
#define ILOGIC_HANDLER_H

#include "NetType.h"


namespace NConnect
{

// 接口返回码
enum ReturnValue
{
	OptSuccess = 0,                          // 操作成功
	OptFailed = 1,                           // 操作&读写失败
	
	CloseConnect = 2,                        // logic 层要求关闭该连接
	InComplete = 3,                          // logic 层未处理完事务，继续等待处理
	
	ConnectException  = 4,                   // 连接异常，该连接将被连接管理模块主动关闭
	SndDataFailed = 5,                       // 逻辑层往连接里发送数据失败
	NonExistConnect = 6,                     // 逻辑层往不存在的连接里收发数据
	
	ActiveConnecting = 7,                    // 主动发起的连接正在创建中
	ActiveConnTimeOut = 8,                   // 主动发起创建连接超时
	ActiveConnIpPortErr = 9,                 // IP或者Port无效，Port必须大于2000，防止和系统端口号冲突
	ActiveConnError = 10,                    // 主动发起创建连接错误
	
	NotNetData = 11,                         // 无网络数据可收
	SendDataCloseConnect = 12,               // logic 层要求连接里的数据发送完毕后直接关闭连接
    
    PeerIpInBlackList = 13,                  // 连接对端的IP，在黑名单中
    PeerIpWhiteListMode = 14,                // 连接对端的IP，开启白名单模式
    PeerIpInvalid = 15,                      // 连接对端的IP，无效的IP值
};


// 网络连接码流的头部数据
struct BufferHeader
{
	char* header;
	unsigned int len;                        // header 的实际长度大小
};


// 主动连接数据
// 本地端主动创建连接 peerIp&peerPort 为连接对端的IP、端口号
// peerPort : 必须大于2000，防止和系统端口号冲突
// timeOut : 最长timeOut秒后连接还没有建立成功则关闭，并回调返回创建连接超时；单位秒
// userCb : 主动发起建立连接时用户挂接的数据，连接建立过程中会通过回调传回应用层
// userId : 主动发起建立连接时用户传递的ID，连接建立过程中会通过回调传回应用层
// peerIp&peerPort&userId 唯一标识一个主动连接，三者都相同则表示是同一个连接
struct ActiveConnect
{
	ActiveConnect* next;
	unsigned int timeOut;
	void* userCb;
	int userId;
	strIP_t peerIp;
	unsigned short peerPort;
};


// 提供给上层逻辑调用者主动调用
class IConnectMgr
{
public:
    virtual ~IConnectMgr() {};
	
public:
    // 逻辑层直接从连接收发消息数据
	virtual ReturnValue recv(Connect*& conn, char* data, unsigned int& len) = 0;
    virtual ReturnValue send(Connect* conn, const char* data, const unsigned int len, bool isNeedWriteMsgHeader = true) = 0;

public:
	virtual void close(Connect* conn) = 0;
};


// 逻辑层实现该接口
// 使用连接管理的逻辑层实现的接口，连接管理模块调用
class ILogicHandler
{
public:
    virtual ~ILogicHandler() {};
	
	// 以下API为连接管理模块主动调用
public:
    // 本地端主动创建连接 peerIp&peerPort 为连接对端的IP、端口号
    // peerPort : 必须大于2000，防止和系统端口号冲突
    // timeOut : 最长timeOut秒后连接还没有建立成功则关闭，并回调返回创建连接超时；单位秒
	virtual ActiveConnect* getConnectData() = 0;
	
	// 主动连接建立的时候调用
	// conn 为连接对应的数据，peerIp&peerPort 为连接对端的IP地址和端口号
	// rtVal : 返回连接是否创建成功
	// 网络管理层根据逻辑层返回码判断是否需要关闭该连接，返回码为 [CloseConnect, SendDataCloseConnect] 则关闭该连接
    virtual ReturnValue doCreateConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, ReturnValue rtVal, void*& userCb) = 0;
	
public:
    // 被动连接建立的时候调用
	// conn 为连接对应的数据，peerIp & peerPort 为连接对端的IP地址和端口号
	// 网络管理层根据逻辑层返回码判断是否需要关闭该连接，返回码为 [CloseConnect, SendDataCloseConnect] 则关闭该连接
    virtual ReturnValue onCreateConnect(Connect* conn, const char* peerIp, const unsigned short peerPort, void*& userCb) = 0;
	
public:
    // 成功建立了的连接才会回调该关闭接口
	virtual void onClosedConnect(Connect* conn, void* userCb) = 0;            // 通知逻辑层连接已被关闭
	virtual void onInvalidConnect(const uuid_type connId, void* userCb) = 0;  // 通知逻辑层connId对应的连接无效了，可能找不到，或者异常被关闭了，不能读写数据等等
	
	
public:
    // 从逻辑层获取数据缓冲区，把从连接读到的数据写到该缓冲区
	// 返回NULL值则不会读取数据；返回非NULL值则先从连接缓冲区读取len的数据长度，做为getWriteBuffer接口的bufferHeader输入参数
	// 该接口一般做为先读取消息头部，用于解析消息头部使用
	virtual BufferHeader* getBufferHeader() = 0;
	
	// 返回值为 (char*)-1 则表示直接丢弃该数据；返回值为非NULL则读取数据；返回值为NULL则不读取数据
	virtual char* getWriteBuffer(const int len, const uuid_type connId, BufferHeader* bufferHeader, void*& cb) = 0;
	virtual void submitWriteBuffer(char* buff, const int len, const uuid_type connId, void* cb) = 0;

public:
    // 从逻辑层获取数据缓冲区，把该数据写入connLogicId对应的连接里
	// isNeedWriteMsgHeader 是否需要写入网络数据消息头部信息，默认会写消息头信息
	// isNeedWriteMsgHeader 如果逻辑层调用者填值为false，则调用者必须自己写入网络消息头部数据，即返回的可读buff中已经包含了网络消息头数据
    virtual char* getReadBuffer(int& len, uuid_type& connId, bool& isNeedWriteMsgHeader, void*& cb) = 0;
	virtual void submitReadBuffer(char* buff, const int len, const uuid_type connId, void* cb) = 0;


public:
    // 直接从从逻辑层读写消息数据，多了一次中间数据拷贝，性能较低
	virtual ReturnValue readData(char* data, int& len, uuid_type& connId) = 0;
    virtual ReturnValue writeData(const char* data, const int len, const uuid_type connId) = 0;


public:
    // 设置连接收发数据接口对象
	virtual void setConnMgrInstance(IConnectMgr* instance) = 0;
    
public:
    // 检查连接对端的IP是否合法，IP地址白名单&黑名单使用
    virtual ReturnValue checkPeerIp(unsigned int peerIp) {return ReturnValue::OptSuccess;}
};

}


#endif  // ILOGIC_HANDLER_H

