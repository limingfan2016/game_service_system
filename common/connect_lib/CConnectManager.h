
/* author : admin
 * date : 2014.12.05
 * description : 网络连接管理
 */
 
#ifndef CCONNECT_MANAGER_H
#define CCONNECT_MANAGER_H

#include <unordered_map>
#include "CTcpConnect.h"
#include "CEpoll.h"
#include "base/CMemManager.h"
#include "base/MacroDefine.h"
#include "NetType.h"


namespace NConnect
{

// 对已经建立、初始化好了的连接的操作
enum ConnectOpt
{
	EAddToQueue = 1,                // 加入消息队列，可以从该连接读取数据
	ENeedReadData = 2,              // 该连接检测到有数据可读，应用层需要从该连接读取数据
};

typedef std::unordered_map<uuid_type, unsigned int> NodeIdToTimeSecs;
typedef std::unordered_map<uuid_type, Connect*> IdToConnect;

class ILogicHandler;

struct ServerCfgData
{
	unsigned int listenMemCache;    // 连接内存池内存块个数
	
	unsigned int count;             // 读写缓冲区内存池内存块个数
	unsigned int step;              // 自动分配的内存块个数
	unsigned int size;              // 每个读写数据区内存块的大小
	
	int listenNum;                  // 监听连接的队列长度
    unsigned int maxMsgSize;  // 最大消息包长度，单个消息包大小超过此长度则关闭连接
    unsigned int checkConnectInterval;   // 遍历所有连接时间间隔（检查连接活跃时间、心跳信息），单位毫秒
    unsigned int activeInterval;             // 连接活跃检测间隔时间，单位秒，超过次间隔时间无数据则关闭连接
    unsigned short checkTimes;      // 检查最大socket无数据的次数，超过此最大次数则连接移出消息队列，避免遍历一堆无数据的空连接
	unsigned short hbInterval;      // 心跳检测间隔时间，单位秒
	unsigned char hbFailedTimes;    // 心跳检测连续失败hbFailedTimes次后关闭连接
};


class CConnectManager
{
public:
	CConnectManager(const char* ip, const int port, ILogicHandler* logicHandler);
	~CConnectManager();
    
    const char* getTcpIp();
    
	
public:
    int start(const ServerCfgData& cfgData);
	
	// 启动连接管理服务，waitTimeout：IO监听数据等待超时时间，单位毫秒；
	// isActive：是否主动收发网络数据
    int run(const int connectCount, const int waitTimeout, DataHandlerType dataHandlerType = ThreadHandle);
    void stop();    // 停止连接管理服务


private:
    int doStart(const ServerCfgData& cfgData);
    void clear();            // 服务退出时清理所有资源
	void clearAllConnect();  // 关闭清理所有连接
	bool isRunning();
	
	void reListen(Connect* listenConn);                           // 重新建立监听
    void acceptConnect(uint32_t eventVal, Connect* listenConn);   // 建立新连接
	void handleConnect(uint32_t eventVal, Connect* conn);         // 处理监听的连接（读写数据）
	void handleConnect();      // 处理所有连接（心跳检测，活跃检测，删除异常连接等等）

private:
	Connect* createConnect(const int fd, const struct in_addr& peerIp, const unsigned short peerPort);
	void addInitedConnect(Connect* conn);
	
	void destroyConnect(Connect* conn);
	void destroyMsgConnect(Connect* conn);
	void destroyLoginConnect(Connect* conn);
	
	void removeConnect(Connect*& listHeader, Connect* conn);
	void closeConnect(Connect* conn);
	void setConnectNormal(Connect* conn);
	
	void addToMsgQueue(Connect* conn, const ConnectOpt opt);
	void removeFromMsgQueue(Connect* conn);

private:
    bool readFromConnect(Connect* conn);
    bool writeToConnect(Connect* conn);
	
	CSingleW_SingleR_Buffer* getReadBuffer();
	void releaseReadBuffer(CSingleW_SingleR_Buffer*& pBuffer);
	
	CSingleW_SingleR_Buffer* getBuffer(CMemManager* memMgr);
	void releaseBuffer(CMemManager* memMgr, CSingleW_SingleR_Buffer*& pBuff);

private:
    void waitDataHandler();           // 等待数据处理线程处理完
	void notifyDataHandler();         // 通知数据处理线程执行处理操作	

private:
    // 处理本地端主动发起的连接
	void doActiveConnect();
	int doActiveConnect(const char* peerIp, const unsigned short peerPort, int& fd, int& errorCode);
	void onActiveConnect(uint32_t eventVal, Connect* conn);
	bool checkActiveConnect(const struct in_addr& peerIp, const unsigned short peerPort, const int userId);  // 检查是否对应的主动连接已经在建立中了
	void addErrorActiveConnect(unsigned int ip, unsigned short port);
	void removeErrorActiveConnect(unsigned int ip, unsigned short port);
	bool checkErrorActiveConnect(unsigned int ip, unsigned short port);
	

	// 提供给数据处理线程调用的API
private:
    unsigned short getHbInterval() const;
	Connect* getMsgConnectList();
	Connect* getMsgConnect(const uuid_type connId);
	
	CSingleW_SingleR_Buffer* getWriteBuffer();
	void releaseWriteBuffer(CSingleW_SingleR_Buffer*& pBuffer);
	
	bool waitConnecter();             // 等待连接处理线程处理完	
	
	
private:
    CTcpConnect m_tcpListener;        // 负责监听连接的socket
	CSocket m_activeConnect;          // 本地端主动对其他外部节点建立连接
	CEpoll m_epoll;                   // epoll IO 监听复用
	
	IdToConnect m_connectMap;         // id映射所有连接对象信息
	Connect* m_loginConnectList;      // 主动建立的连接列表
	Connect* m_msgConnectList;        // 建立的处理逻辑消息连接列表
	unsigned long long m_nextCheckTime;     // 下次检测连接的时间点，单位毫秒
	
	CMemManager* m_memForRead;        // 读socket缓冲区内存管理，连接管理线程使用
	CMemManager* m_memForWrite;       // 写socket缓冲区内存管理，数据处理线程使用
	CMemManager* m_connMemory;        // 连接内存管理

    uuid_type m_connId;               // 各个连接的ID，从1开始
	ILogicHandler* m_logicHandler;    // 业务逻辑处理，读写数据
	
	int m_listenNum;                  // 监听连接的队列长度
	
	SynNotify m_synNotify;            // 连接线程&数据处理线程同步等待&通知
    unsigned int m_maxMsgSize;   // 最大消息包长度，单个消息包大小超过此长度则关闭连接
    unsigned int m_checkConnectInterval;  // 遍历所有连接时间间隔（检查连接活跃时间、心跳信息），单位毫秒
	unsigned int m_activeInterval;             // 连接活跃检测间隔时间，单位秒，超过此间隔时间无数据则关闭连接
	unsigned short m_checkTimes;      // 检查最大socket无数据的次数，超过此最大次数则连接移出消息队列，避免遍历一堆无数据的空连接
	unsigned short m_hbInterval;      // 心跳检测间隔时间，单位秒
	unsigned char m_hbFailedTimes;    // 心跳检测连续失败hbFailedTimes次后关闭连接
	char m_status;                    // 连接服务状态，0:停止；1:运行
	NodeIdToTimeSecs m_errorConns;    // 主动连接，错误连接的下次发起时间
	

	// 禁止构造&拷贝&赋值类函数
	DISABLE_CONSTRUCTION_ASSIGN(CConnectManager);

	friend class CDataHandler;
	friend class CDataTransmit;
	friend class SynWaitNotify;
};

}

#endif // CCONNECT_MANAGER_H
