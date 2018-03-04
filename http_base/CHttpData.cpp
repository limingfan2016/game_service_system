
/* author : limingfan
 * date : 2015.11.17
 * description : Http 数据处理
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "CHttpData.h"
#include "base/ErrorCode.h"


using namespace NCommon;
using namespace NErrorCode;

namespace NHttp
{

// 消息数据分析处理API
int CHttpDataParser::getHttpBodyLen(const char* header, const char* body, unsigned int& len)
{
	// 支持的POST消息必须存在消息体内容
	// 只要消息头没有 Content-Length: xxx 标识的一律认为是错误的消息，即POST消息必须存在消息体内容
	const char* CLFag = "Content-Length: ";
	const char* contentLenFlag = strcasestr(header, CLFag);
	if (contentLenFlag == NULL || contentLenFlag > body) return BodyDataError;
	
	char* contentLenEnd = (char*)strstr(contentLenFlag, "\r\n");
	if (contentLenEnd == NULL) return BodyDataError;
	
	*contentLenEnd = '\0';
	unsigned int contentLen = atoi(contentLenFlag + strlen(CLFag));
	*contentLenEnd = '\r';

	if (contentLen >= len) return BodyDataError;

	if (body == NULL || strlen(body) < contentLen) return IncompleteBodyData;  // 数据不完整，继续等待接收数据

	len = contentLen;
	return Success;
}


int CHttpDataParser::parseHeader(const char* data, RequestHeader& reqData)
{
	// GET /index.php?a=xx&b=yxz HTTP/1.1
	// POST /index.php HTTP/1.1
	reqData.method = data;
	reqData.methodLen = 0;
	reqData.url = NULL;
	reqData.urlLen = 0;
	
	const char* firstLine = strstr(data, "\r\n");  // 只分析http头部第一行
	while (++data != firstLine)
	{
		if (reqData.methodLen == 0 && *data == ' ')
		{
			reqData.methodLen = data - reqData.method;   // 方法 结束位置
			reqData.url = data + 1;                      // url 开始位置
		}
		else if (reqData.urlLen == 0 && (*data == '?' || *data == ' '))
		{
			reqData.urlLen = data - reqData.url;         // url 结束位置
			break;
		}
	}
	
	// 参数名值对解析
	const char* key = data + 1;
	unsigned int keyLen = 0;
	const char* value = NULL;
	while (++data < firstLine && *data != ' ')
	{
		if (*data == '=')
		{
			keyLen = data - key;
			value = data + 1;
			while (++data < firstLine && *data != ' ')
			{
				if (*data == '&')
				{
					reqData.paramKey2Value[string(key, keyLen)] = string(value, data - value);
					value = NULL;
					key = data + 1;
					break;
				}
			}
		}
		
		// 最后一个参数名值对
		if (*data == ' ' && value != NULL)
		{
			reqData.paramKey2Value[string(key, keyLen)] = string(value, data - value);
			break;
		}
	}

	return (reqData.methodLen > 0 && reqData.urlLen > 0) ? (int)Success : (int)HeaderDataError;
}

int CHttpDataParser::parseBody(const char* header, const char* body, HttpMsgBody& msgBody)
{
	int rc = getHttpBodyLen(header, body, msgBody.len);
	if (rc != Success) return rc;

	parseJSONContent(body, msgBody.paramKey2Value);
	parseKeyToValue(body, msgBody.paramKey2Value);

	return Success;
}


void CHttpDataParser::parseJSONContent(const char* content, ParamKey2Value& paramKey2Value, char separ, bool removeValueSpace, bool removeKeySpace)
{
	if (content == NULL || *content == '\0') return;
	
	// JSON格式，参数名值对解析
	// data format : {"valid":"1","roll":true,"interval":60,"times":4,"msg_code":2000,"msg_desc":"成功"}
	// data format : {"sign":"2eec751c598ec6faf69e89a6aa68ebb4","data":{"failedDesc":"","amount":"0.10","tradeTime":"20151228170406","tradeId":"20151228170358000285","gameId":"617579","payType":"999","attachInfo":"info","orderStatus":"S","orderId":"8.1451293436.1"},"ver":"1.0"}
	const char* keyBegin = NULL;
	const char* keyEnd = NULL;
	const char* valueBegin = NULL;
	char* valueEnd = NULL;
	
	char parseData[MaxRequestDataLen] = {0};
	strncpy(parseData, content, MaxRequestDataLen - 1);
	const unsigned int len = strlen(parseData) - 1;
	
	// if (*parseData == '{' && parseData[len] == '}') parseData[len] = '\0';  // 去掉多余的结束符
	if (*parseData == '{')
	{
	    char* endChr = strrchr(parseData, '}');
		if (endChr != NULL) *endChr = '\0';  // 去掉多余的结束符等
	}
	
	int keyLen = 0;
	int valueLen = 0;
	content = parseData;
	char* separate = (char*)strchr(content, ',');  // 查找名值对分隔符
	int flag = (separate != NULL) ? 1 : -1;        // -1表示只解析一次就结束循环
	if (flag == -1) separate = parseData + len + 1;
	while (flag != 0)
	{
		*separate = '\0';  // 截断当前名值对后面的数据
		keyBegin = strchr(content, separ);
		keyEnd = (keyBegin != NULL) ? strchr(keyBegin + 1, separ) : NULL;
		
		// 先检查是否存在嵌套的对象
		valueBegin = (keyEnd != NULL) ? strchr(keyEnd + 1, '{') : NULL;
		if (valueBegin != NULL)
		{
			*separate = ',';                                                    // 得先还原名值对分隔符才能继续查找对象内容
		    valueEnd = (char*)strchr(valueBegin + 1, '}');                      // 对象的结束位置
			const char* jsonObj = strchr(valueBegin + 1, '{');                  // 下一个json对象
			while (valueEnd != NULL && jsonObj != NULL && jsonObj < valueEnd)   // 继续存在嵌套对象
			{
				valueEnd = (char*)strchr(valueEnd + 1, '}');
				jsonObj = strchr(jsonObj + 1, '{');                             // 下一个json对象
			}
			
			if (valueEnd != NULL)                                // 存在合法的json对象结束符}
			{
				separate = (char*)strchr(++valueEnd, ',');       // 是否还存在其他名值对数据
				*valueEnd = '\0';                                // 截断当前json对象
				parseJSONContent(valueBegin, paramKey2Value, separ, removeValueSpace, removeKeySpace);    // 递归解析当前json对象
				
				if (separate == NULL) break;                     // 没有名值对数据了
				goto CheckFinish;
			}
			*separate = '\0';                                    // 截断当前名值对后面的数据
		}
		
		valueBegin = (keyEnd != NULL) ? strchr(keyEnd + 1, separ) : NULL;
		if (valueBegin != NULL)
		{
			valueEnd = (char*)strchr(valueBegin + 1, separ);
		}
		else if (keyEnd != NULL)
		{
			valueBegin = strchr(keyEnd + 1, ':');
			valueEnd = separate;
		}
		
		if (keyBegin == NULL || keyEnd == NULL || valueBegin == NULL || valueEnd == NULL) break;
		
		// key开始地址及长度
		keyLen = keyEnd - keyBegin - 1;
		++keyBegin;

        // value开始地址及长度
		valueLen = valueEnd - valueBegin - 1;
		++valueBegin;
		
		if (removeValueSpace)
		{
			// 去掉前后多余的空格
			while (valueLen > 0 && *valueBegin == ' ')
			{
				--valueLen;
				++valueBegin;
			}
			while (valueLen > 0 && valueBegin[valueLen - 1] == ' ') --valueLen;
		}
		
		if (removeKeySpace)
		{
			// 去掉前后多余的空格
			while (keyLen > 0 && *keyBegin == ' ')
			{
				--keyLen;
				++keyBegin;
			}
			while (keyLen > 0 && keyBegin[keyLen - 1] == ' ') --keyLen;
		}
		
		paramKey2Value[string(keyBegin, keyLen)] = string(valueBegin, valueLen);
		
		// paramKey2Value[string(keyBegin + 1, keyEnd - keyBegin - 1)] = string(valueBegin + 1, valueEnd - valueBegin - 1);
		if (flag == -1) break;                                 // -1表示只解析一次就结束循环
		
		CheckFinish:
		{
			content = separate + 1;
			separate = (char*)strchr(content, ',');
			flag = (separate != NULL) ? 1 : -1;                // -1表示只解析一次就结束循环
			if (flag == -1) separate = parseData + len;
		}
	}
}

void CHttpDataParser::parseKeyToValue(const char* content, ParamKey2Value& paramKey2Value)
{
	if (content == NULL || *content == '\0') return;
	
	// format : a=b&c=d&1=2
	// 参数名值对解析
	const char* key = content;
	unsigned int keyLen = 0;
	const char* value = NULL;
	while (*(++content) != '\0')
	{
		if (*content == '=')
		{
			keyLen = content - key;
			value = content + 1;
			while (*(++content) != '\0')
			{
				if (*content == '&')
				{
					paramKey2Value[string(key, keyLen)] = string(value, content - value);
					value = NULL;
					key = content + 1;
					break;
				}
			}
		}
		
		// 最后一个参数名值对
		if (*content == '\0')
		{
			if (value != NULL && content - value > 0) paramKey2Value[string(key, keyLen)] = string(value, content - value);
			break;
		}
	}
}



// Http消息内容
CHttpMessage::CHttpMessage()
{
}

CHttpMessage::~CHttpMessage()
{
}

void CHttpMessage::setHeaderKeyValue(const string& key, const string& value)
{
	m_paramKey2Value[key] = value;
}

void CHttpMessage::setHeaderKeyValue(const char* key, const char* value)
{
	m_paramKey2Value[key] = value;
}

void CHttpMessage::setContent(const string& content)
{
	m_content = content;
}

void CHttpMessage::setContent(const char* content, const unsigned int len)
{
	m_content = string(content, len);
}

void CHttpMessage::setContent(const char* content)
{
	m_content = content;
}


// Http消息协议请求数据对象
CHttpRequest::CHttpRequest(HttpMsgType msgType) : m_msgType(msgType)
{
}

CHttpRequest::~CHttpRequest()
{
}

void CHttpRequest::setParam(const string& param)
{
	m_param = param;
}

void CHttpRequest::setParam(const char* param, const unsigned int len)
{
	m_param = string(param, len);
}

void CHttpRequest::setParam(const char* param)
{
	m_param = param;
}

void CHttpRequest::setParamValue(const string& key, const string& value)
{
	if (m_param.length() > 0)
	{
		m_param += ('&' + key + '=' + value);
	}
	else
	{
		m_param = (key + '=' + value);
	}
}

void CHttpRequest::setParamValue(const char* key, const char* value)
{
	setParamValue(string(key), string(value));
}

const char* CHttpRequest::getMessage(char* buffer, unsigned int& len, const char* host, const char* url) const
{
	if (buffer == NULL || len <= 1 || host == NULL || url == NULL) return NULL;

	const unsigned int bufferSize = len;
	const char* msgName[] = {"GET", "POST"};
	const char* separate = (m_param.length() < 1) ? "" : "?";
	
	// 数据例子 : GET /api/uid/check?uid=8139480&token=1234567890&appkey_id=100668669 HTTP/1.1\r\nHost: pay.wandoujia.com\r\n\r\n
	len = snprintf(buffer, bufferSize - 1, "%s %s%s%s HTTP/1.1\r\nHost: %s\r\n", msgName[m_msgType], url, separate, m_param.c_str(), host);
	for (ParamKey2Value::const_iterator it = m_paramKey2Value.begin(); it != m_paramKey2Value.end(); ++it)
	{
		len += snprintf(buffer + len, bufferSize - len - 1, "%s: %s\r\n", it->first.c_str(), it->second.c_str());
	}
	
	if (m_content.length() > 0)
	{
	    len += snprintf(buffer + len, bufferSize - len - 1, "Content-Length: %u\r\n\r\n%s", (unsigned int)m_content.length(), m_content.c_str());
	}
	else
	{
		len += snprintf(buffer + len, bufferSize - len - 1, "%s", "\r\n");
	}
	buffer[len] = 0;
	
	return buffer;
}



// Http消息协议响应对象数据
CHttpResponse::CHttpResponse()
{
}

CHttpResponse::~CHttpResponse()
{
}

const char* CHttpResponse::getMessage(unsigned int& len)
{
	len = MaxNetBuffSize;
	return getMessage(m_rspMsg, len);
}

const char* CHttpResponse::getMessage(char* buffer, unsigned int& len)
{
	if (buffer == NULL || len <= 1) return NULL;

	// "HTTP/1.1 200 OK\r\nContent-Length: 7\r\n\r\nsuccess" : "HTTP/1.1 200 OK\r\nContent-Length: 7\r\n\r\nfailure";
	const unsigned int bufferSize = len;
	len = snprintf(buffer, bufferSize - 1, "HTTP/1.1 200 OK\r\n");
	for (ParamKey2Value::const_iterator it = m_paramKey2Value.begin(); it != m_paramKey2Value.end(); ++it)
	{
		len += snprintf(buffer + len, bufferSize - len - 1, "%s: %s\r\n", it->first.c_str(), it->second.c_str());
	}
	
	len += snprintf(buffer + len, bufferSize - len - 1, "Content-Length: %u\r\n\r\n%s", (unsigned int)m_content.length(), m_content.c_str());
	buffer[len] = 0;
	
	return buffer;
}



// SSL对象操作
CSSLObject& CSSLObject::getInstance()
{
	static CSSLObject instance;
	return instance;
}

SSL* CSSLObject::createSSLInstance()
{
    return (m_SSLContext != NULL) ? SSL_new(m_SSLContext) : NULL;
}

void CSSLObject::destroySSLInstance(SSL*& instance)
{
	if (instance != NULL)
	{
		SSL_shutdown(instance);
        SSL_free(instance);
		instance = NULL;
	}
}


CSSLObject::CSSLObject()
{
	SSL_load_error_strings();
    SSL_library_init();
    m_SSLContext = SSL_CTX_new(SSLv23_client_method());
}

CSSLObject::~CSSLObject()
{
	if (m_SSLContext != NULL) SSL_CTX_free(m_SSLContext);
	ERR_free_strings();
}

}

