
/* author : limingfan
 * date : 2018.01.12
 * description : http消息逻辑实现
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>

#include "CHttpLogicHandler.h"
#include "CHttpOptSrv.h"


using namespace NProject;


namespace NPlatformService
{

CHttpLogicHandler::CHttpLogicHandler()
{
	m_msgHandler = NULL;
	
	m_checkPhoneNumberSrvIp[0] = '\0';
	m_checkPhoneNumberSrvPort = 0;
}

CHttpLogicHandler::~CHttpLogicHandler()
{
	m_msgHandler = NULL;
	
	m_checkPhoneNumberSrvIp[0] = '\0';
	m_checkPhoneNumberSrvPort = 0;
}

int CHttpLogicHandler::init(CHttpMsgHandler* pMsgHandler)
{
	m_msgHandler = pMsgHandler;

    // 发送手机验证码服务IP&Port
    if (!m_msgHandler->getHostInfo(m_msgHandler->m_pCfg->check_phone_number_cfg.host.c_str(), m_checkPhoneNumberSrvIp,
                                   m_checkPhoneNumberSrvPort, m_msgHandler->m_pCfg->check_phone_number_cfg.protocol.c_str()))
    {
        OptErrorLog("get check phone number host info error, host = %s, protocol = %s",
        m_msgHandler->m_pCfg->check_phone_number_cfg.host.c_str(), m_msgHandler->m_pCfg->check_phone_number_cfg.protocol.c_str());

        return SrvOptFailed;
    }

    // 服务消息注册
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, HTTPOPT_CHECK_PHONE_NUMBER_REQ, (ProtocolHandler)&CHttpLogicHandler::checkPhoneNumber, this);
	
	// http 应答回调函数
	m_msgHandler->registerHttpReplyHandler(ESendMobileVerificationCodeId, (HttpReplyHandler)&CHttpLogicHandler::sendMobileVerificationCodeReply, this);

	return SrvOptSuccess;
}

void CHttpLogicHandler::unInit()
{
}

void CHttpLogicHandler::SHA256Result(const unsigned char* signStr, unsigned int signStrLen, char* result, unsigned int resultLen)
{
    unsigned char md[SHA256_DIGEST_LENGTH] = {0};
    SHA256(signStr, signStrLen, md);

    unsigned int dataLen = 0;
    for (unsigned int idx = 0; idx < SHA256_DIGEST_LENGTH; ++idx)
    {
        dataLen += snprintf(result + dataLen, resultLen - dataLen - 1, "%02x", md[idx]);
    }
}

bool CHttpLogicHandler::sendMobileVerificationCodeReply(ConnectData* cd)
{
	/*
    正常返回如下：
    "Content-Type": "application/json; charset=utf-8"
    {
      "status":"ok",
      "data":{"rrid" : "1233445555"}
    }
    
    异常返回如下：
    {
      "errcode": 7xxxx,
      "message": "xxxxxx"
    }
    */
	
	com_protocol::ClientCheckPhoneNumberRsp rsp;

	do
	{
		ParamKey2Value paramKey2Value;
		CHttpDataParser::parseJSONContent(strchr(cd->receiveData, '{'), paramKey2Value, '"', true);
		ParamKey2Value::iterator errorCodeIt = paramKey2Value.find("errcode");
		if (errorCodeIt != paramKey2Value.end())
		{
			rsp.set_result(HOPTCheckPhoneNumberReplyError);
			break;
		}
		
		ParamKey2Value::iterator statusIt = paramKey2Value.find("status");
		if (statusIt == paramKey2Value.end() || statusIt->second != "ok")
		{
			rsp.set_result(HOPTCheckPhoneNumberReplyError);
			break;
		}
        
        rsp.set_result(SrvOptSuccess);

	} while (false);
	
	m_msgHandler->getSrvOpt().sendPkgMsgToService(cd->keyData.key, strlen(cd->keyData.key), rsp, cd->keyData.srvId,
	                                              HTTPOPT_CHECK_PHONE_NUMBER_RSP, "check phone number reply");

	OptInfoLog("check phone number reply, userdata = %s, srcSrvId = %u, result = %d", cd->keyData.key, cd->keyData.srvId, rsp.result());

	return (rsp.result() == SrvOptSuccess);
}

void CHttpLogicHandler::checkPhoneNumber(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    /*
    URL https://sms.wilddog.com/api/v1/{:appId}/code/send
    返回数据格式    JSON
    HTTP 请求方式   POST

    参数说明
    参数	    类型	必选	说明
    templateId	long	是	   模板 ID
    mobile	    string	是	   收信人手机号，如1xxxxxxxxxx，格式必须为11位
    signature	string	是	   数字签名，合法性验证，其中参与数字签名计算的参数包括 templateId， mobile，timestamp， 若使用的是自定义验证码模板，还需要params参数
    timestamp	string	是	   UNIX 时间戳，精度是毫秒
    params	    string	否	   短信参数列表，用于依次填充模板，若使用自定义验证码模板，则此参数必填，JSONArray格式，如[“xxx”,”yyy”]
    
    注意：
    生成数字签名时, 参数不要使用 urlencode. 在调用 api 时, 才需要对参数做 urlencode
    
    返回说明

    正常返回如下：
    "Content-Type": "application/json; charset=utf-8"
    {
      "status":"ok",
      "data":{"rrid" : "1233445555"}
    }
    
    异常返回如下：
    {
      "errcode": 7xxxx,
      "message": "xxxxxx"
    }
    
    sign string = "mobile=13800138000&params=[\"王小豆\",\"个人版套版套餐\"]&templateId=100001&timestamp=1506065665377&kYVAi9tXAbPOURnkWiiWADRuNi6DJy7JmSg02myB";
    &signature=01bbc7fb885d1d383b1c82c77c30ee151b590193c6b4f7208c966d5df867e7c7
    */

    com_protocol::ClientCheckPhoneNumberReq req;
	if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "check phone number request")) return;
    
    // 当前毫秒
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    unsigned long long currentMilliSecond = currentTime.tv_sec * MillisecondUnit + currentTime.tv_usec / MillisecondUnit;
    
    const NHttpOperationConfig::CheckPhoneNumberCfg& checkPhoneNumberCfg = m_msgHandler->m_pCfg->check_phone_number_cfg;
    char signStr[1024] = {0};
    unsigned int signStrLen = snprintf(signStr, sizeof(signStr) - 1, "mobile=%s&params=[%u]&templateId=%u&timestamp=%llu&%s",
    req.phone_number().c_str(), req.verification_code(), checkPhoneNumberCfg.template_id, currentMilliSecond, checkPhoneNumberCfg.app_key.c_str());
    
    // 参数sha256结果
    char signResult[128] = {0};
    SHA256Result((const unsigned char*)signStr, signStrLen, signResult, sizeof(signResult));
    
    // &signature=01bbc7fb885d1d383b1c82c77c30ee151b590193c6b4f7208c966d5df867e7c7
	signStrLen -= checkPhoneNumberCfg.app_key.length();  // 去掉app key
    signStrLen += snprintf(signStr + signStrLen, sizeof(signStr) - signStrLen - 1, "signature=%s", signResult);
    
    // 发送手机验证码
	// https://sms.wilddog.com/api/v1/{appId}/code/send
	CHttpRequest httpRequest(HttpMsgType::POST);
	httpRequest.setParam(signStr, signStrLen);

	bool result = m_msgHandler->doHttpConnect(m_checkPhoneNumberSrvIp, m_checkPhoneNumberSrvPort,
                                              checkPhoneNumberCfg.host.c_str(), checkPhoneNumberCfg.url.c_str(),
                                              httpRequest, ESendMobileVerificationCodeId, m_msgHandler->getContext().userData, srcSrvId);
    if (!result)
    {
        com_protocol::ClientCheckPhoneNumberRsp rsp;
        rsp.set_result(HOPTSendCheckPhoneNumberInfoError);
        m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, HTTPOPT_CHECK_PHONE_NUMBER_RSP, "check phone number error reply");
    }
    
    OptInfoLog("check phone number request, userdata = %s, number = %s, verification code = %u, result = %d",
    m_msgHandler->getContext().userData, req.phone_number().c_str(), req.verification_code(), result);
}

}
