
/* author : limingfan
 * date : 2018.01.12
 * description : http消息逻辑实现
 */
 
#ifndef __CHTTP_LOGIC_HANDLER_H__
#define __CHTTP_LOGIC_HANDLER_H__

#include "SrvFrame/CModule.h"
#include "http_base/HttpBaseDefine.h"
#include "BaseDefine.h"


using namespace NCommon;
using namespace NFrame;


namespace NPlatformService
{

class CHttpMsgHandler;

class CHttpLogicHandler : public NFrame::CHandler
{
public:
	CHttpLogicHandler();
	~CHttpLogicHandler();

public:
	int init(CHttpMsgHandler* pMsgHandler);
	void unInit();
    
private:
    void SHA256Result(const unsigned char* signStr, unsigned int signStrLen, char* result, unsigned int resultLen);

	// 发送http请求之后收到的应答
private:
    bool sendMobileVerificationCodeReply(ConnectData* cd);
	
	bool sendMobilePhoneMessageReply(ConnectData* cd);
	
	bool checkPhoneHttpReplyMessage(ConnectData* cd);
	
    // 服务收到的POST请求
private:
	// bool receivePostMessage(const char* connId, Connect* conn, const RequestHeader& reqHeaderData, const HttpMsgBody& msgBody);

private:
	void checkPhoneNumber(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    void sendMobilePhoneMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	CHttpMsgHandler* m_msgHandler;
	
private:
	strIP_t m_phoneNumberSrvIp;
    unsigned short m_phoneNumberSrvPort;
};

}

#endif // __CHTTP_LOGIC_HANDLER_H__
