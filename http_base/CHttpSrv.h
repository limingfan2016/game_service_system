
/* author : limingfan
 * date : 2015.07.01
 * description : Http 服务简单实现
 */

#ifndef CHTTP_SRV_H
#define CHTTP_SRV_H

#include "SrvFrame/IService.h"
#include "SrvFrame/CNetDataHandler.h"
#include "base/CMemManager.h"

#include "CHttpConnectMgr.h"
#include "CHttpData.h"
#include "HttpBaseDefine.h"


using namespace NFrame;

namespace NHttp
{

// 收到外部发送的GET消息协议，处理函数
typedef bool (CHandler::*HttpGetOptHandler)(const char* connId, Connect* conn, const RequestHeader& reqHeaderData);
struct HttpGetOptHandlerObject
{
	CHandler* instance;
	HttpGetOptHandler handler;
	
	HttpGetOptHandlerObject() {};
	HttpGetOptHandlerObject(CHandler* _instance, HttpGetOptHandler _handler) : instance(_instance), handler(_handler) {};
};
typedef unordered_map<string, HttpGetOptHandlerObject> HttpGetOptHandlerInfo;  // key 为用户注册时传递的URL

// 收到外部发送的POST消息协议，处理函数
typedef bool (CHandler::*HttpPostOptHandler)(const char* connId, Connect* conn, const RequestHeader& reqHeaderData, const HttpMsgBody& msgBody);
struct HttpPostOptHandlerObject
{
	CHandler* instance;
	HttpPostOptHandler handler;
	
	HttpPostOptHandlerObject() {};
	HttpPostOptHandlerObject(CHandler* _instance, HttpPostOptHandler _handler) : instance(_instance), handler(_handler) {};
};
typedef unordered_map<string, HttpPostOptHandlerObject> HttpPostOptHandlerInfo;  // key 为用户注册时传递的URL

// 内部发送http请求后收到外部的http应答消息，处理函数
typedef bool (CHandler::*HttpReplyHandler)(ConnectData* cd);
struct HttpReplyHandlerObject
{
	CHandler* instance;
	HttpReplyHandler handler;
	
	HttpReplyHandlerObject() {};
	HttpReplyHandlerObject(CHandler* _instance, HttpReplyHandler _handler) : instance(_instance), handler(_handler) {};
};
typedef unordered_map<unsigned int, HttpReplyHandlerObject> HttpReplyHandlerInfo;  // key 为用户注册&发起请求时传递的requestId


// http消息协议处理对象
class CHttpDataHandler : public NFrame::CNetDataHandler
{
public:
	CHttpDataHandler();
	~CHttpDataHandler();

public:
	// 根据host获取ip&port
	bool getHostInfo(const char* host, strIP_t ip, unsigned short& port, const char* service = "http");
	
	// 建立http连接，同时发送http请求数据
	bool doHttpConnect(const char* ip, unsigned int port, const char* host, const char* url, const CHttpRequest& httpRequest,
	                   const unsigned int requestId = 0, const char* userData = NULL, const unsigned int srcSrvId = 0,
                       const UserCbData* cbData = NULL, const unsigned int cbDataLen = 0, const unsigned short protocolId = 0);

	// 建立http连接，同时发送http请求数据
	bool doHttpConnect(const char* ip, const unsigned short port, ConnectData* connData);
	
public:
	// 获取&释放数据缓冲区
	char* getDataBuffer(unsigned int& len);
	void putDataBuffer(const char* dataBuff);
	
public:
    // 注册http GET消息处理函数
	void registerGetOptHandler(const string& url, HttpGetOptHandler handler, CHandler* instance);
	
	// 注册http POST消息处理函数
	void registerPostOptHandler(const string& url, HttpPostOptHandler handler, CHandler* instance);
	
	// 注册http应答消息处理函数
	void registerHttpReplyHandler(unsigned int requestId, HttpReplyHandler handler, CHandler* instance);

private:	
	int onHandle();  // 主动发送的http请求收到应答消息
	
	void onClosedConnect(void* userData);  // 连接关闭
	
	ConnectData* getConnectData();
	void putConnectData(ConnectData* connData);

private:
    // 被动收到外部的http请求消息
	virtual int onClientData(Connect* conn, const char* data, const unsigned int len);
	int handleRequest(Connect* conn, HttpRequestBuffer* ud);
	
private:
	void onLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	void onUnLoad(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
private:
    // 服务启动时被调用
	virtual int onInit(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	// 服务停止时被调用
	virtual void onUnInit(const char* srvName, const unsigned int srvId, unsigned short moduleId);
	
	// 服务配置更新时被调用
	virtual void onUpdateConfig();

private:
    NCommon::CMemManager m_memForRequestBuffer;
	NCommon::CMemManager m_memForConnectData;
	CHttpConnectMgr m_httpConnectMgr;
	
private:
	HttpGetOptHandlerInfo m_httpGetOptHandlerInfo;
	HttpPostOptHandlerInfo m_httpPostOptHandlerInfo;
	
	HttpReplyHandlerInfo m_httpReplyHandlerInfo;


    friend class CHttpSrv;

DISABLE_COPY_ASSIGN(CHttpDataHandler);
};


// Http 服务
class CHttpSrv : public NFrame::IService
{
public:
	CHttpSrv(unsigned int srvType);
	~CHttpSrv();
	
public:
    // 注册本服务的Http消息处理实例对象
	// 在回调函数onRegister中调用
    int registerHttpHandler(CHttpDataHandler* pInstance);

public:
	virtual int onInit(const char* name, const unsigned int id);            // 服务启动时被调用
	virtual void onUnInit(const char* name, const unsigned int id);         // 服务停止时被调用
	virtual void onUpdateConfig(const char* name, const unsigned int id);   // 服务配置更新时被调用
	
	virtual void onRegister(const char* name, const unsigned int id) = 0;   // 服务启动后被调用，服务需在此注册本服务的各模块信息
	
private:
	virtual int onHandle();                         // 通知逻辑层处理逻辑
	
	virtual void onClosedConnect(void* userData);   // 通知逻辑层对应的逻辑连接已被关闭


private:
    CHttpDataHandler* m_httpDataHandler;
	
	
DISABLE_COPY_ASSIGN(CHttpSrv);
};

}

#endif // CHTTP_SRV_H
