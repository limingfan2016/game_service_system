
/* author : limingfan
 * date : 2015.04.23
 * description : 类型定义
 */

#ifndef __HTTP_BASE_DEFINE_H__
#define __HTTP_BASE_DEFINE_H__

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <string>
#include <unordered_map>
#include "base/Type.h"


using namespace std;

namespace NHttp
{

// 常量定义
static const unsigned int MaxReceiveNetBuffSize = 10240;
static const unsigned int MaxNetBuffSize = 8192;
static const unsigned int MaxDataQueueSize = 8192;
static const unsigned int MaxRequestDataLen = 4096;
static const unsigned int MaxRequestBufferCount = 32;
static const unsigned int MaxConnectDataCount = 32;
static const unsigned int MaxKeySize = 36;
static const unsigned int MaxConnectIdLen = 36;

// http 请求消息头部结束标识
static const char* RequestHeaderFlag = "\r\n\r\n";
static const unsigned int RequestHeaderFlagLen = strlen(RequestHeaderFlag);

static const unsigned short HttpsPort = 443;
static const unsigned int HttpConnectTimeOut = 10;  // 连接超时时间，单位秒

// GET & POST 协议标识
static const char* ProtocolGetMethod = "GET";
static const unsigned int ProtocolGetMethodLen = strlen(ProtocolGetMethod);
static const char* ProtocolPostMethod = "POST";
static const unsigned int ProtocolPostMethodLen = strlen(ProtocolPostMethod);


typedef unordered_map<string, string> ParamKey2Value;

// http 请求头部数据
struct RequestHeader
{
	const char* method;
	unsigned int methodLen;
	
	const char* url;
	unsigned int urlLen;
	
	ParamKey2Value paramKey2Value;
};

// http 消息体数据
struct HttpMsgBody
{
	unsigned int len;
	
	ParamKey2Value paramKey2Value;
};

// 收到外部的http请求数据
// 该结构内存块也同时用作数据缓冲区使用（调用API接口get&put）
struct HttpRequestBuffer
{
	unsigned int reqDataLen;
	char reqData[MaxRequestDataLen];  // 收到的请求数据
	
	unsigned int connIdLen;
	char connId[MaxConnectIdLen];     // 连接ID标识
};

// 连接服务状态
enum SrvStatus
{
	Stop = 0,
	Run = 1,
	Stopping = 2,
};

// 连接状态
enum ConnectStatus
{
	Normal = 0,
	Connecting = 1,
	TimeOut = 2,
	AlreadyWrite = 3,
	CanRead = 4,
	ConnectError = 5,
	
	SSLConnecting = 6,
	SSLConnectError = 7,

	DataError = 8,
};

// 连接上挂接的用户数据
struct UserKeyData
{
	unsigned int requestId;  // 主动发送http请求对应的ID
	char key[MaxKeySize];
	unsigned int srvId;
	unsigned short protocolId;
};

// 用户挂接的回调数据
struct UserCbData
{
	unsigned int flag;  // 0：默认内部管理内存； 1：外部挂接的内存由外部管理（创建&释放）
};

// http连接对应的数据
struct ConnectData
{
	// 用户挂接和连接相关的数据
	UserKeyData keyData;
	
	// 需要发送的数据
	char sendData[MaxNetBuffSize];
	unsigned int sendDataLen;
	unsigned int timeOutSecs;
	
	// 收到的数据
	char receiveData[MaxReceiveNetBuffSize];
	unsigned int receiveDataLen;
	unsigned int receiveDataIdx;

	int fd;
	unsigned int lastTimeSecs;
	int status;         // 连接状态
	
	SSL* sslConnect;    // https 连接操作对象
	
	const UserCbData* cbData;   // 用户挂接的数据
};

}

#endif // __HTTP_BASE_DEFINE_H__

