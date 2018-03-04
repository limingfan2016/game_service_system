
/* author : limingfan
 * date : 2015.11.17
 * description : Http 数据处理
 */

#ifndef CHTTP_DATA_H
#define CHTTP_DATA_H

#include "base/MacroDefine.h"
#include "HttpBaseDefine.h"


namespace NHttp
{

// 消息数据分析处理
class CHttpDataParser
{
public:
	static int getHttpBodyLen(const char* header, const char* body, unsigned int& len);
	static int parseHeader(const char* data, RequestHeader& reqData);
	static int parseBody(const char* header, const char* body, HttpMsgBody& msgBody);
	
public:
	static void parseJSONContent(const char* content, ParamKey2Value& paramKey2Value, char separ = '"', bool removeValueSpace = false, bool removeKeySpace = false);
	static void parseKeyToValue(const char* content, ParamKey2Value& paramKey2Value);


DISABLE_CLASS_BASE_FUNC(CHttpDataParser);
};



//  Http消息类型
enum HttpMsgType
{
	GET = 0,
	POST = 1,
};


// Http消息内容
class CHttpMessage
{
public:
	CHttpMessage();
	~CHttpMessage();

public:
    void setHeaderKeyValue(const string& key, const string& value);
	void setHeaderKeyValue(const char* key, const char* value);
	
public:
	void setContent(const string& content);
	void setContent(const char* content, const unsigned int len);
	void setContent(const char* content);

protected:
    ParamKey2Value m_paramKey2Value;
	string m_content;


DISABLE_COPY_ASSIGN(CHttpMessage);
};


// Http消息协议请求数据对象
class CHttpRequest : public CHttpMessage
{
public:
	CHttpRequest(HttpMsgType msgType);
	~CHttpRequest();

public:
	void setParam(const string& param);
	void setParam(const char* param, const unsigned int len);
	void setParam(const char* param);
	
public:
	void setParamValue(const string& key, const string& value);
	void setParamValue(const char* key, const char* value);
	
public:
	const char* getMessage(char* buffer, unsigned int& len, const char* host, const char* url) const;

private:
    HttpMsgType m_msgType;
	string m_param;


DISABLE_COPY_ASSIGN(CHttpRequest);
};


// Http消息协议响应数据对象
class CHttpResponse : public CHttpMessage
{
public:
	CHttpResponse();
	~CHttpResponse();

public:
	const char* getMessage(unsigned int& len);
	const char* getMessage(char* buffer, unsigned int& len);

private:
	char m_rspMsg[MaxNetBuffSize];


DISABLE_COPY_ASSIGN(CHttpResponse);
};



// SSL对象操作
class CSSLObject
{
public:
	static CSSLObject& getInstance();

public:
    SSL* createSSLInstance();
	void destroySSLInstance(SSL*& instance);

private:
	CSSLObject();
	~CSSLObject();

private:
    SSL_CTX* m_SSLContext;
	

DISABLE_COPY_ASSIGN(CSSLObject);
};


}

#endif // CHTTP_DATA_H
