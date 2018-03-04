
/* author : limingfan
 * date : 2017.12.25
 * description : 微信登录实现
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>

#include "CWXLogin.h"
#include "CHttpOptSrv.h"


using namespace NProject;


namespace NPlatformService
{

CWXLogin::CWXLogin()
{
	m_msgHandler = NULL;
	
	m_wxIp[0] = '\0';
	m_wxPort = 0;
}

CWXLogin::~CWXLogin()
{
	m_msgHandler = NULL;
	
	m_wxIp[0] = '\0';
	m_wxPort = 0;
}

int CWXLogin::init(CHttpMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

	const unsigned int platformTypes[] = {EThirdPlatformType::EClientGameWeiXin, EThirdPlatformType::EBusinessManagerWeiXin};
	const NHttpOperationConfig::ThirdPlatformInfo* wxInfo = NULL;
	for (unsigned int idx = 0; idx < sizeof(platformTypes) / sizeof(platformTypes[0]); ++idx)
	{
		wxInfo = m_msgHandler->getThirdPlatformInfo(platformTypes[idx]);
		if (wxInfo == NULL)
		{
			OptErrorLog("get wei xin platform config info error, type = %d", platformTypes[idx]);
			return SrvOptFailed;
		}
		
		// 目前只有weixin，多平台则需要不同的ip&port
		if (wxInfo->check_host == 1 && !m_msgHandler->getHostInfo(wxInfo->host.c_str(), m_wxIp, m_wxPort, wxInfo->protocol.c_str()))
		{
			OptErrorLog("get wei xin host info error, host = %s, protocol = %s", wxInfo->host.c_str(), wxInfo->protocol.c_str());
			return SrvOptFailed;
		}
	}

	m_msgHandler->registerProtocol(ServiceType::CommonSrv, HTTPOPT_WX_LOGIN_REQ, (ProtocolHandler)&CWXLogin::wxLogin, this);
	
	// http 应答回调函数
	m_msgHandler->registerHttpReplyHandler(EGetWeiXinTokenInfoId, (HttpReplyHandler)&CWXLogin::getWXTokenInfoReply, this);
	m_msgHandler->registerHttpReplyHandler(EUpdateWeiXinTokenInfoId, (HttpReplyHandler)&CWXLogin::updateWXTokenInfoReply, this);
	m_msgHandler->registerHttpReplyHandler(EGetWeiXinUserInfoId, (HttpReplyHandler)&CWXLogin::getWXUserInfoReply, this);

	return SrvOptSuccess;
}

void CWXLogin::unInit()
{
}

void CWXLogin::setRequestCbData(unsigned int platformtype, const string& userId, const string& cbData, SWXReqestCbData& requestCbData)
{
	memset(&requestCbData, 0, sizeof(SWXReqestCbData));  // 底层自动管理回调数据内存

    requestCbData.platformType = platformtype;

	requestCbData.userIdLen = userId.length();
    if (requestCbData.userIdLen > 0 && requestCbData.userIdLen < MaxCbDataLen)
	{
		memcpy(requestCbData.userId, userId.c_str(), requestCbData.userIdLen);
	}
	else
	{
		requestCbData.userIdLen = 0;
	}
	
	requestCbData.cbDataLen = cbData.length();
	if (requestCbData.cbDataLen > 0 && requestCbData.cbDataLen < MaxCbDataLen)
	{
		memcpy(requestCbData.cbData, cbData.c_str(), requestCbData.cbDataLen);
	}
	else
	{
		requestCbData.cbDataLen = 0;
	}
}

int CWXLogin::getTokenInfo(unsigned int platformtype, const string& userId, const string& cbData, const string& code, const char* userData, unsigned int srcSrvId)
{
	// 根据code获取token信息
	// https://api.weixin.qq.com/sns/oauth2/access_token?appid=APPID&secret=SECRET&code=CODE&grant_type=authorization_code
	const NHttpOperationConfig::ThirdPlatformInfo* wxInfo = m_msgHandler->getThirdPlatformInfo(platformtype);
	CHttpRequest httpRequest(HttpMsgType::GET);
	httpRequest.setParamValue("appid", wxInfo->app_id);
	httpRequest.setParamValue("secret", wxInfo->app_key);
	httpRequest.setParamValue("code", code);
	httpRequest.setParamValue("grant_type", "authorization_code");
	
	SWXReqestCbData requestCbData;
	setRequestCbData(platformtype, userId, cbData, requestCbData);

    const char* CGetTokenURL = "/sns/oauth2/access_token";
	if (!m_msgHandler->doHttpConnect(m_wxIp, m_wxPort, wxInfo->host.c_str(), CGetTokenURL, httpRequest,
									 EGetWeiXinTokenInfoId, userData, srcSrvId,
									 &requestCbData, sizeof(requestCbData))) return HOPTGetWeiXinTokenInfoError;

	return SrvOptSuccess;
}

int CWXLogin::updateTokenInfo(unsigned int platformtype, const string& userId, const string& cbData, const string& refreshToken, const char* userData, unsigned int srcSrvId)
{
	// 刷新access_token
	// https://api.weixin.qq.com/sns/oauth2/refresh_token?appid=APPID&grant_type=refresh_token&refresh_token=REFRESH_TOKEN
    const NHttpOperationConfig::ThirdPlatformInfo* wxInfo = m_msgHandler->getThirdPlatformInfo(platformtype);
	CHttpRequest httpRequest(HttpMsgType::GET);
	httpRequest.setParamValue("appid", wxInfo->app_id);
	httpRequest.setParamValue("grant_type", "refresh_token");
	httpRequest.setParamValue("refresh_token", refreshToken);
	
	SWXReqestCbData requestCbData;
	setRequestCbData(platformtype, userId, cbData, requestCbData);
	
    const char* CUpdateTokenURL = "/sns/oauth2/refresh_token";
	if (!m_msgHandler->doHttpConnect(m_wxIp, m_wxPort, wxInfo->host.c_str(), CUpdateTokenURL, httpRequest,
									 EUpdateWeiXinTokenInfoId, userData, srcSrvId,
									 &requestCbData, sizeof(requestCbData))) return HOPTUpdateWeiXinAccessTokenError;
	
	return SrvOptSuccess;
}

int CWXLogin::getUserInfo(unsigned int platformtype, const string& userId, const string& cbData, const SWXLoginInfo& loginInfo, const char* userData, unsigned int srcSrvId)
{
	// 获取用户信息
	// https://api.weixin.qq.com/sns/userinfo?access_token=ACCESS_TOKEN&openid=OPENID
	CHttpRequest httpRequest(HttpMsgType::GET);
	httpRequest.setParamValue("access_token", loginInfo.accessToken);
	httpRequest.setParamValue("openid", loginInfo.openid);
	
	SWXReqestCbData requestCbData;
	setRequestCbData(platformtype, userId, cbData, requestCbData);

	const NHttpOperationConfig::ThirdPlatformInfo* wxInfo = m_msgHandler->getThirdPlatformInfo(platformtype);
    const char* CGetUserInfoURL = "/sns/userinfo";
	if (!m_msgHandler->doHttpConnect(m_wxIp, m_wxPort, wxInfo->host.c_str(), CGetUserInfoURL, httpRequest,
									 EGetWeiXinUserInfoId, userData, srcSrvId,
									 &requestCbData, sizeof(requestCbData))) return HOPTGetWeiXinUserInfoError;
									 
    return SrvOptSuccess;
}


bool CWXLogin::getWXTokenInfoReply(ConnectData* cd)
{
	/*
	{ 
		"access_token":"ACCESS_TOKEN", 
		"expires_in":7200, 
		"refresh_token":"REFRESH_TOKEN",
		"openid":"OPENID", 
		"scope":"SCOPE" 
	}
 
	{
		"errcode":40029,"errmsg":"invalid code"
	}
    */
	
	com_protocol::WXLoginRsp rsp;
	const SWXReqestCbData& wxCbData = *(const SWXReqestCbData*)cd->cbData;
	
	do
	{
		ParamKey2Value paramKey2Value;
		CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
		ParamKey2Value::iterator errorCodeIt = paramKey2Value.find("errcode");
		if (errorCodeIt != paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenInvalidCode);
			break;
		}
		
		ParamKey2Value::iterator accessTokenIt = paramKey2Value.find("access_token");
		if (accessTokenIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		ParamKey2Value::iterator expiresInIt = paramKey2Value.find("expires_in");
		if (expiresInIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		ParamKey2Value::iterator refreshTokenIt = paramKey2Value.find("refresh_token");
		if (refreshTokenIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		ParamKey2Value::iterator openIdIt = paramKey2Value.find("openid");
		if (openIdIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		// 保存token信息
		// 这里key先保存登录openid等信息，最后成功之后才改为保存unionid
		// access_token是调用授权关系接口的调用凭证，由于access_token有效期（目前为2个小时）较短，
		// 当access_token超时后，可以使用refresh_token进行刷新，access_token刷新结果有两种：
        // 1.若access_token已超时，那么进行refresh_token会获取一个新的access_token，新的超时时间；
        // 2.若access_token未超时，那么进行refresh_token不会改变access_token，但超时时间会刷新，相当于续期access_token。
		// refresh_token拥有较长的有效期（30天）且无法续期，当refresh_token失效的后，需要用户重新授权后才可以继续获取用户头像昵称。
		char wxLoginKey[MaxCbDataLen] = {0};
		snprintf(wxLoginKey, sizeof(wxLoginKey) - 1, "%u.%s", wxCbData.platformType, openIdIt->second.c_str());
		SWXLoginInfo& wxLoginInfo = m_wxUId2Info[wxLoginKey];
		wxLoginInfo.refreshToken = refreshTokenIt->second;
		wxLoginInfo.accessToken = accessTokenIt->second;
		wxLoginInfo.openid = openIdIt->second;
		
		// 填写超时时间点
		const NHttpOperationConfig::WXTokenTimeOut& tokenTimeoutCfg = m_msgHandler->m_pCfg->wx_token_timeout;
		wxLoginInfo.refreshTokenTimeOutSecs = time(NULL) + tokenTimeoutCfg.refresh_token_last_secs - tokenTimeoutCfg.adjust_secs;
		wxLoginInfo.accessTokenTimeOutSecs = time(NULL) + atoi(expiresInIt->second.c_str()) - tokenTimeoutCfg.adjust_secs;
		
		// 获取用户信息
		int rc = getUserInfo(wxCbData.platformType, wxCbData.userId, string(wxCbData.cbData, wxCbData.cbDataLen),
		                     wxLoginInfo, cd->keyData.key, cd->keyData.srvId);
		if (rc != SrvOptSuccess)
		{
			m_wxUId2Info.erase(wxLoginKey);

			rsp.set_result(rc);
			break;
		}
		
		return true;

	} while (false);
	
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cd->keyData.key, strlen(cd->keyData.key), rsp, cd->keyData.srvId,
	                                              HTTPOPT_WX_LOGIN_RSP, "wei xin login error reply");

	OptErrorLog("wei xin login get token error, userdata = %s, platform type = %d, id = %s, srcSrvId = %u",
	cd->keyData.key, wxCbData.platformType, wxCbData.userId, cd->keyData.srvId);
	
	return false;
}

bool CWXLogin::updateWXTokenInfoReply(ConnectData* cd)
{
	/*
	{ 
		"access_token":"ACCESS_TOKEN", 
		"expires_in":7200, 
		"refresh_token":"REFRESH_TOKEN",
		"openid":"OPENID", 
		"scope":"SCOPE" 
	}
 
	{
		"errcode":40030,"errmsg":"invalid refresh_token"
	}
    */
	
	com_protocol::WXLoginRsp rsp;
	const SWXReqestCbData& wxCbData = *(const SWXReqestCbData*)cd->cbData;
	
	do
	{
		ParamKey2Value paramKey2Value;
		CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
		ParamKey2Value::iterator errorCodeIt = paramKey2Value.find("errcode");
		if (errorCodeIt != paramKey2Value.end())
		{
			rsp.set_result(HOPTWeiXinRefreshTokenTimeOut);
			break;
		}
		
		ParamKey2Value::iterator accessTokenIt = paramKey2Value.find("access_token");
		if (accessTokenIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		ParamKey2Value::iterator expiresInIt = paramKey2Value.find("expires_in");
		if (expiresInIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		ParamKey2Value::iterator refreshTokenIt = paramKey2Value.find("refresh_token");
		if (refreshTokenIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		ParamKey2Value::iterator openIdIt = paramKey2Value.find("openid");
		if (openIdIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}

		char wxLoginKey[MaxCbDataLen] = {0};
		snprintf(wxLoginKey, sizeof(wxLoginKey) - 1, "%u.%s", wxCbData.platformType, wxCbData.userId);
		SWXLoginInfo& wxLoginInfo = m_wxUId2Info[wxLoginKey];
		wxLoginInfo.accessToken = accessTokenIt->second;
		wxLoginInfo.accessTokenTimeOutSecs = time(NULL) + atoi(expiresInIt->second.c_str()) - m_msgHandler->m_pCfg->wx_token_timeout.adjust_secs;
		
		// 获取用户信息
		int rc = getUserInfo(wxCbData.platformType, wxCbData.userId, string(wxCbData.cbData, wxCbData.cbDataLen),
		                     wxLoginInfo, cd->keyData.key, cd->keyData.srvId);
		if (rc != SrvOptSuccess)
		{
			rsp.set_result(rc);
			break;
		}
		
		return true;

	} while (false);
	
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cd->keyData.key, strlen(cd->keyData.key), rsp, cd->keyData.srvId,
	                                              HTTPOPT_WX_LOGIN_RSP, "wei xin login error reply");

	OptErrorLog("wei xin login update token error, userdata = %s, platform type = %d, id = %s, srcSrvId = %u",
	cd->keyData.key, wxCbData.platformType, wxCbData.userId, cd->keyData.srvId);
	
	return false;
}

bool CWXLogin::getWXUserInfoReply(ConnectData* cd)
{
	/*
	{ 
		"openid":"OPENID",
		"nickname":"NICKNAME",
		"sex":1,
		"province":"PROVINCE",
		"city":"CITY",
		"country":"COUNTRY",
		"headimgurl": "http://wx.qlogo.cn/mmopen/g3MonUZtNHkdmzicIlibx6iaFqAc56vxLSUfpb6n5WKSYVY0ChQKkiaJSgQ1dZuTOgvLLrhJbERQQ4eMsv84eavHiaiceqxibJxCfHe/0",
		"privilege":[
		"PRIVILEGE1", 
		"PRIVILEGE2"
		],
		"unionid": " o6_bmasdasdsad6_2sgVt7hMZOPfL"
	}
 
	{
		"errcode":40003,"errmsg":"invalid openid"
	}
    */
	
	com_protocol::WXLoginRsp rsp;
	const SWXReqestCbData& wxCbData = *(const SWXReqestCbData*)cd->cbData;
	
	do
	{
		ParamKey2Value paramKey2Value;
		CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
		ParamKey2Value::iterator errorCodeIt = paramKey2Value.find("errcode");
		if (errorCodeIt != paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinUserInfoOpenIdError);
			break;
		}
		
		ParamKey2Value::iterator nicknameIt = paramKey2Value.find("nickname");
		if (nicknameIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		ParamKey2Value::iterator sexIt = paramKey2Value.find("sex");
		if (sexIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		ParamKey2Value::iterator headimgurlIt = paramKey2Value.find("headimgurl");
		if (headimgurlIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		ParamKey2Value::iterator openIdIt = paramKey2Value.find("openid");
		if (openIdIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}
		
		ParamKey2Value::iterator unionidIt = paramKey2Value.find("unionid");
		if (unionidIt == paramKey2Value.end())
		{
			rsp.set_result(HOPTGetWeiXinTokenReplyError);
			break;
		}

		// 先使用openid查找，如果存在说明流程路径是从获取refresh token过来的，
		// 则需要删除openid的信息，新增加unionid信息
		char wxLoginKey[MaxCbDataLen] = {0};
		snprintf(wxLoginKey, sizeof(wxLoginKey) - 1, "%u.%s", wxCbData.platformType, openIdIt->second.c_str());
		WXUIDToLoginInfoMap::iterator loginIt = m_wxUId2Info.find(wxLoginKey);
		if (loginIt != m_wxUId2Info.end())
		{
			// 新增加unionid信息
			snprintf(wxLoginKey, sizeof(wxLoginKey) - 1, "%u.%s", wxCbData.platformType, unionidIt->second.c_str());
			m_wxUId2Info[wxLoginKey] = loginIt->second;
			
			m_wxUId2Info.erase(loginIt);  // 删除openid的信息
		}
		
		rsp.set_result(SrvOptSuccess);
		com_protocol::StaticInfo* userStaticInfo = rsp.mutable_user_static_info();
		userStaticInfo->set_username("");
		userStaticInfo->set_nickname(nicknameIt->second);
		userStaticInfo->set_portrait_id(headimgurlIt->second);
		userStaticInfo->set_gender(atoi(sexIt->second.c_str()));
		
		rsp.set_platform_id(unionidIt->second);
		if (wxCbData.cbDataLen > 0) rsp.set_cb_data(wxCbData.cbData, wxCbData.cbDataLen);

	} while (false);
	
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cd->keyData.key, strlen(cd->keyData.key), rsp, cd->keyData.srvId,
	                                              HTTPOPT_WX_LOGIN_RSP, "wei xin login reply");

	OptInfoLog("wei xin login reply, userdata = %s, platform type = %d, id = %s, unionid = %s, srcSrvId = %u, result = %d, nickname = %s, headimgurl = %s, gender = %d",
	cd->keyData.key, wxCbData.platformType, wxCbData.userId, rsp.platform_id().c_str(), cd->keyData.srvId,
	rsp.result(), rsp.user_static_info().nickname().c_str(), rsp.user_static_info().portrait_id().c_str(),
	rsp.user_static_info().gender());
	
	return (rsp.result() == SrvOptSuccess);
}
	
void CWXLogin::wxLogin(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::WXLoginReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "wei xin login request")) return;
	
	OptInfoLog("wei xin login, userdata = %s, username = %s, platform type = %d, id = %s, code = %s",
	m_msgHandler->getContext().userData, req.username().c_str(), req.platform_type(), req.platform_id().c_str(), req.code().c_str());
	
	com_protocol::WXLoginRsp rsp;
    rsp.set_result(HOPTWeiXinLoginInvalidParameter);

	do
	{
		if (req.platform_type() != EClientGameWeiXin && req.platform_type() != EBusinessManagerWeiXin) break;

		int rc = SrvOptFailed;
		if (req.has_code() && !req.code().empty())
		{
			// 获取token信息
			rc = getTokenInfo(req.platform_type(), req.platform_id(), req.cb_data(), req.code(), m_msgHandler->getContext().userData, srcSrvId);
			if (rc != SrvOptSuccess)
			{
				rsp.set_result(rc);
				break;
			}
			
			return;
		}

		if (req.has_platform_id() && !req.platform_id().empty())
		{
			char wxLoginKey[MaxCbDataLen] = {0};
		    snprintf(wxLoginKey, sizeof(wxLoginKey) - 1, "%u.%s", req.platform_type(), req.platform_id().c_str());
			WXUIDToLoginInfoMap::iterator it = m_wxUId2Info.find(wxLoginKey);
			if (it != m_wxUId2Info.end())
			{
				if (it->second.accessTokenTimeOutSecs > time(NULL))
				{
					// 有效期内则直接获取用户信息
					rc = getUserInfo(req.platform_type(), req.platform_id(), req.cb_data(), it->second, m_msgHandler->getContext().userData, srcSrvId);
					if (rc != SrvOptSuccess)
					{
						rsp.set_result(rc);
						break;
					}
					
					return;
				}
				
				if (it->second.refreshTokenTimeOutSecs > time(NULL))
				{
					// access_token超时则重新刷新access_token
					rc = updateTokenInfo(req.platform_type(), req.platform_id(), req.cb_data(), it->second.refreshToken, m_msgHandler->getContext().userData, srcSrvId);
					if (rc != SrvOptSuccess)
					{
						rsp.set_result(rc);
						break;
					}
					
					return;
				}
				
				m_wxUId2Info.erase(it);  // 都超时了则删除
			}
			
			rsp.set_result(HOPTWeiXinRefreshTokenTimeOut);
			break;
		}

	} while (false);
	
	m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, HTTPOPT_WX_LOGIN_RSP, "wei xin login error reply");
	
	OptErrorLog("wei xin login error, result = %d", rsp.result());
}

}
