
/* author : limingfan
 * date : 2014.12.17
 * description : 网络连接管理socket读写数据线程
 */
 
#ifndef CDATAHANDLER_H
#define CDATAHANDLER_H

#include "base/MacroDefine.h"
#include "base/CThread.h"
#include "ILogicHandler.h"
#include "NetType.h"


namespace NConnect
{

class CConnectManager;

// 连接收发数据分析、数据处理对象
class CDataHandler : public CThread, public IConnectMgr
{
public:
	CDataHandler(CConnectManager* connMgr, ILogicHandler* logicHandler);
	~CDataHandler();
	
	// 启动线程，逻辑层被动收发消息
public:
	int startHandle();
	void finishHandle();
	bool isRunning();
	
	virtual const char* getName();  // 线程名称
	
	// 由逻辑层主动收发消息
public:
	virtual ReturnValue recv(Connect*& conn, char* data, unsigned int& len);
    virtual ReturnValue send(Connect* conn, const char* data, const unsigned int len, bool isNeedWriteMsgHeader = true);
	virtual void close(Connect* conn);

public:
	static unsigned int getCanReadDataSize(Connect* conn);
	static unsigned int getCanWriteDataSize(Connect* conn);
	
	// 读写数据
	// isRead : 为false时从连接读出的数据会被直接丢弃，不会拷贝到data调用者空间
	static unsigned int read(Connect* conn, char* data, unsigned int len, bool isRead = true);
	static bool write(CConnectManager* connMgr, Connect* conn, const char* data, int len);
	static bool writeToSkt(CConnectManager* connMgr, Connect* conn, const char* data, const unsigned int len, int& wtLen);
	static void releaseWrtBuff(CConnectManager* connMgr, Connect* conn);  // 释放申请的连接写缓冲区
	
private:
    // 读写数据
	// isRead : 为false时从连接读出的数据会被直接丢弃，不会拷贝到data调用者空间
	unsigned int readData(Connect* conn, char* data, unsigned int len, bool isRead = true);
	
	bool readPkgHeader(Connect* curConn, unsigned int& dataSize);
	bool readPkgBody(Connect* curConn, unsigned int dataSize, unsigned int& pkgLen, char* pkgData = NULL);

	bool writeData(Connect* conn, const char* data, int len);
	void writeHbRequestPkg(Connect* curConn, unsigned int curSecs);

    // 处理数据	
	int handleData(Connect* msgConnList, NetPkgHeader& userPkgHeader);
	bool handleData(Connect* curConn, unsigned int dataSize, unsigned int curSecs, unsigned int& pkgLen, char* pkgData = NULL);
	bool readFromLogic(NetPkgHeader& userPkgHeader);
	
	// 释放申请的连接写缓冲区
	void releaseWriteBuffer(Connect* conn);
	
private:
	virtual void run();
	
private:
    CConnectManager* m_connMgr;
	Connect* m_curConn;
	ILogicHandler* m_logicHandler;    // 业务逻辑处理，读写数据
	char m_status;                    // 线程处理数据状态，停止；运行；正在停止中
	
// 禁止构造&拷贝&赋值类函数
DISABLE_CONSTRUCTION_ASSIGN(CDataHandler);
};




// 提供给上层逻辑调用者调用
class CLogicConnectMgr : public IConnectMgr
{
public:
	CLogicConnectMgr(CDataHandler* dataHandler);
	~CLogicConnectMgr();
	
public:
    // 逻辑层直接从连接收发消息数据
	virtual ReturnValue recv(Connect*& conn, char* data, unsigned int& len);
    virtual ReturnValue send(Connect* conn, const char* data, const unsigned int len, bool isNeedWriteMsgHeader = true);

public:
	virtual void close(Connect* conn);
	
private:
    CDataHandler* m_dataHandler;
	
// 禁止构造&拷贝&赋值类函数
DISABLE_CONSTRUCTION_ASSIGN(CLogicConnectMgr);
};




// 防止操作异常情况下线程不解锁
class SynWaitNotify
{
public:
	SynWaitNotify(CConnectManager* connMgr);
	~SynWaitNotify();
	
private:
	CConnectManager* m_connMgr;

// 禁止构造&拷贝&赋值类函数
DISABLE_CONSTRUCTION_ASSIGN(SynWaitNotify);
};


}

#endif // CDATAHANDLER_H
