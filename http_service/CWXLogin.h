
/* author : limingfan
 * date : 2017.12.25
 * description : 微信登录实现
 */
 
#ifndef __CWX_LOGIN_H__
#define __CWX_LOGIN_H__

#include "SrvFrame/CModule.h"
#include "http_base/HttpBaseDefine.h"
#include "BaseDefine.h"


using namespace NCommon;
using namespace NFrame;


namespace NPlatformService
{

class CHttpMsgHandler;

class CWXLogin : public NFrame::CHandler
{
public:
	CWXLogin();
	~CWXLogin();

public:
	int init(CHttpMsgHandler* pMsgHandler);
	void unInit();
	
private:
	void setRequestCbData(unsigned int platformtype, const string& userId, const string& cbData, SWXReqestCbData& requestCbData);

	int getTokenInfo(unsigned int platformtype, const string& userId, const string& cbData, const string& code, const char* userData, unsigned int srcSrvId);
    int updateTokenInfo(unsigned int platformtype, const string& userId, const string& cbData, const string& refreshToken, const char* userData, unsigned int srcSrvId);
    int getUserInfo(unsigned int platformtype, const string& userId, const string& cbData, const SWXLoginInfo& loginInfo, const char* userData, unsigned int srcSrvId);
	
private:
    bool getWXTokenInfoReply(ConnectData* cd);
	bool updateWXTokenInfoReply(ConnectData* cd);
	bool getWXUserInfoReply(ConnectData* cd);

private:
	void wxLogin(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	CHttpMsgHandler* m_msgHandler;
	
	WXUIDToLoginInfoMap m_wxUId2Info;
	
private:
	strIP_t m_wxIp;
    unsigned short m_wxPort;
};

}

#endif // __CWX_LOGIN_H__
