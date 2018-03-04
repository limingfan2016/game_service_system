
/* author : limingfan
 * date : 2016.03.30
 * description : 逻辑数据处理
 */
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "CMsgHandler.h"
#include "CServiceLogicData.h"


using namespace NProject;


CServiceLogicData::CServiceLogicData()
{
	m_msgHandler = NULL;
}

CServiceLogicData::~CServiceLogicData()
{
	m_msgHandler = NULL;
}

int CServiceLogicData::init(CMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;
	
    // 设置服务逻辑数据
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_SET_SERVICE_DATA_REQ, (ProtocolHandler)&CServiceLogicData::setServiceData, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_SERVICE_DATA_REQ, (ProtocolHandler)&CServiceLogicData::getServiceData, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_MORE_SERVICE_DATA_REQ, (ProtocolHandler)&CServiceLogicData::getMoreServiceData, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_DEL_SERVICE_DATA_REQ, (ProtocolHandler)&CServiceLogicData::delServiceData, this);

	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_USER_DATA_REQ, (ProtocolHandler)&CServiceLogicData::getUserData, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_MODIFY_USER_DATA_REQ, (ProtocolHandler)&CServiceLogicData::modifyUserData, this);
    
    m_msgHandler->registerProtocol(ServiceType::CommonSrv, DBPROXY_GET_USER_ATTRIBUTE_VALUE_REQ, (ProtocolHandler)&CServiceLogicData::getUserAttributeValue, this);

    // 启动定时器执行定时任务
	setTaskTimer(0, 0, NULL, 0);
	
	// 小时定时器
	m_msgHandler->setTimer(60 * 60 * MillisecondUnit, (TimerHandler)&CServiceLogicData::setHourTimer, 0, NULL, -1, this);
	
	return 0;
}

void CServiceLogicData::unInit()
{
	// 保存逻辑数据到redis
	saveLogicData();
}

const UserLogicData& CServiceLogicData::getLogicData(const char* userName, unsigned int len)
{
	UserToLogicData::iterator it = m_user2LogicData.find(userName);
	if (it != m_user2LogicData.end()) return it->second;
	
	m_user2LogicData[userName] = UserLogicData();
	UserLogicData& srvData = m_user2LogicData[userName];
	srvData.isUpdate = 0;
	
	char msgBuffer[MaxMsgLen] = {0};
	int gameDataLen = m_msgHandler->getLogicRedisService().getHField(DBProxyLogicDataKey, DBProxyLogicDataKeyLen, userName, len, msgBuffer, MaxMsgLen);
	if (gameDataLen > 0)
	{
		/*
		if (!srvData.logicData.ParseFromArray(msgBuffer, gameDataLen))
		{
			// 出错了只能初始化清空数据重新再来
			OptErrorLog("getLogicData, ParseFromArray message error, user = %s, msg byte len = %d, buff len = %d", userName, srvData.logicData.ByteSize(), gameDataLen);
			srvData.logicData.Clear();
			srvData.isUpdate = 1;
		}
		*/ 
	}
	
	return srvData;
}

UserLogicData& CServiceLogicData::setLogicData(const char* userName, unsigned int len)
{
	UserLogicData& srvData = (UserLogicData&)getLogicData(userName, len);
	srvData.isUpdate = 1;  // update logic data
	return srvData;
}


void CServiceLogicData::saveLogicData()
{
	for (UserToLogicData::iterator it = m_user2LogicData.begin(); it != m_user2LogicData.end(); ++it)
	{
		if (it->second.isUpdate == 1)
		{
			/*
			const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(it->second.logicData, "save user logic data to redis");
			if (msgData != NULL)
			{
				int rc = m_msgHandler->getLogicRedisService().setHField(DBProxyLogicDataKey, DBProxyLogicDataKeyLen,
				                                                        it->first.c_str(), it->first.length(), msgData, it->second.logicData.ByteSize());
				
				if (rc != 0) OptErrorLog("save user logic data to redis error, user = %s, data len = %u, rc = %d", it->first.c_str(), it->second.logicData.ByteSize(), rc);
			}
			else
			{
				OptErrorLog("save user logic data to redis serializeMsgToBuffer error, user = %s, data len = %u",
				it->first.c_str(), it->second.logicData.ByteSize());
			}
			*/ 
		}
	}
	
	m_user2LogicData.clear();
}

// 任务定时器
void CServiceLogicData::setTaskTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	OptInfoLog("task timer begin, current timer id = %u, remain count = %u", timerId, remainCount);
	
	// 执行定时任务
	saveLogicData();

	// 重新设置定时器
	struct tm tmval;
	time_t curSecs = time(NULL);
	unsigned int intervalSecs = 60 * 60 * 24;  // 一天的时间间隔
	if (localtime_r(&curSecs, &tmval) != NULL)
	{
		tmval.tm_sec = 0;
		tmval.tm_min = 0;
		tmval.tm_hour = atoi(CCfg::getValue("Service", "TaskTimerHour"));
		++tmval.tm_mday;
		time_t nextSecs = mktime(&tmval);
		if (nextSecs != (time_t)-1)
		{
			intervalSecs = nextSecs - curSecs;
		}
		else
		{
			OptErrorLog("setTaskTimer, get next time error");
		}
	}
	else
	{
		OptErrorLog("setTaskTimer, get local time error");
	}

	unsigned int tId = m_msgHandler->setTimer(intervalSecs * MillisecondUnit, (TimerHandler)&CServiceLogicData::setTaskTimer, 0, NULL, 1, this);	
	OptInfoLog("task timer end, next timer id = %u, interval = %u, date = %d-%02d-%02d %02d:%02d:%02d",
	tId, intervalSecs, tmval.tm_year + 1900, tmval.tm_mon + 1, tmval.tm_mday, tmval.tm_hour, tmval.tm_min, tmval.tm_sec);
}

// 小时定时器
void CServiceLogicData::setHourTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	OptInfoLog("hour timer begin");
	
	unsigned int updateSuccess = 0;
	unsigned int updateFailed = 0;
	for (UserToLogicData::iterator it = m_user2LogicData.begin(); it != m_user2LogicData.end(); ++it)
	{
		if (it->second.isUpdate == 1)
		{
			/*
			const char* msgData = m_msgHandler->getSrvOpt().serializeMsgToBuffer(it->second.logicData, "hour timer save user logic data to redis");
			if (msgData != NULL)
			{
				int rc = m_msgHandler->getLogicRedisService().setHField(DBProxyLogicDataKey, DBProxyLogicDataKeyLen,
				                                                        it->first.c_str(), it->first.length(), msgData, it->second.logicData.ByteSize());
				if (rc == 0)
				{
					it->second.isUpdate = 0;
					
					++updateSuccess;
				}
				else
				{
					++updateFailed;
					
					OptErrorLog("hour timer save user logic data to redis error, user = %s, data len = %u, rc = %d",
					it->first.c_str(), it->second.logicData.ByteSize(), rc);
				}
			}
			else
			{
				OptErrorLog("hour timer save user logic data to redis serializeMsgToBuffer error, user = %s, data len = %u",
				it->first.c_str(), it->second.logicData.ByteSize());
			}
			*/ 
		}
	}
	
	OptInfoLog("hour timer end, update success = %u, failed = %u", updateSuccess, updateFailed);
}


void CServiceLogicData::setServiceData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetServiceDataRsp rsp;
	rsp.set_result(SrvOptSuccess);
	
	int rc = SrvOptFailed;
	com_protocol::SetServiceDataReq req;
	if (m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "set service data request"))
	{
		rc = m_msgHandler->getLogicRedisService().setHField(req.primary_key().c_str(), req.primary_key().length(),
														    req.second_key().c_str(), req.second_key().length(),
															req.data().c_str(), req.data().length());
	    if (rc != 0)
		{
			rsp.set_result(DBProxySetServiceDataError);
			OptErrorLog("set service data to redis error, primary key = %s, second key = %s, data len = %u, rc = %d",
			req.primary_key().c_str(), req.second_key().c_str(), len, rc);
		}
	}
	else
	{
		rsp.set_result(DBProxyParseFromArrayError);
	}
	
	rsp.set_primary_key(req.primary_key());
	rsp.set_second_key(req.second_key());

    if (srcProtocolId == 0) srcProtocolId = DBPROXY_SET_SERVICE_DATA_RSP;
	rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "set service data reply");
	
	if (rsp.result() != 0 || rc != 0)
	{
		OptErrorLog("set service data error, srcSrvId = %u, srcProtocolId = %d, msg byte len = %d, result = %d, rc = %d",
		srcSrvId, srcProtocolId, rsp.ByteSize(), rsp.result(), rc);
	}
}

void CServiceLogicData::getServiceData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	char dataBuffer[MaxMsgLen] = {0};
	com_protocol::GetServiceDataRsp rsp;
	com_protocol::GetServiceDataReq req;
	if (m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get service data request"))
	{
		int gameDataLen = m_msgHandler->getLogicRedisService().getHField(req.primary_key().c_str(), req.primary_key().length(),
		                                                                 req.second_key().c_str(), req.second_key().length(),
										                                 dataBuffer, MaxMsgLen);
		if (gameDataLen > 0)
		{
			rsp.set_result(SrvOptSuccess);
			rsp.set_data(dataBuffer, gameDataLen);
		}
		else
		{
			rsp.set_result(DBProxyGetServiceDataError);
			OptErrorLog("get service data from redis error, primary key = %s, second key = %s, buffer len = %u, rc = %d",
			req.primary_key().c_str(), req.second_key().c_str(), MaxMsgLen, gameDataLen);
		}
	}
	else
	{
		rsp.set_result(DBProxyParseFromArrayError);
	}
	
	rsp.set_primary_key(req.primary_key());
	rsp.set_second_key(req.second_key());
	rsp.set_cb_data(req.cb_data());
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_SERVICE_DATA_RSP;
	int rc = m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get service data reply");
	
	if (rsp.result() != SrvOptSuccess || rc != 0)
	{
		OptErrorLog("get service data error, srcSrvId = %u, srcProtocolId = %d, msg byte len = %d, buff len = %d, result = %d, rc = %d",
		srcSrvId, srcProtocolId, rsp.ByteSize(), MaxMsgLen, rsp.result(), rc);
	}
}

void CServiceLogicData::getMoreServiceData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetMoreKeyServiceDataRsp getMoreKeyRsp;
	com_protocol::GetMoreKeyServiceDataReq getMoreKeyReq;
	getMoreKeyRsp.set_result(DBProxyParseFromArrayError);
	while (m_msgHandler->getSrvOpt().parseMsgFromBuffer(getMoreKeyReq, data, len, "get more service data request"))
	{
		char dataBuffer[MaxMsgLen] = {0};
		std::vector<std::string> gameDataVector;
		if (getMoreKeyReq.second_keys_size() < 1)
		{
			int ret = m_msgHandler->getLogicRedisService().getHAll(getMoreKeyReq.primary_key().c_str(), getMoreKeyReq.primary_key().length(), gameDataVector);
			if (ret != 0)
			{
				getMoreKeyRsp.set_result(DBProxyGetMoreServiceDataError);
				OptErrorLog("get more service data from redis error, primary key = %s, rc = %d",
				getMoreKeyReq.primary_key().c_str(), ret);
				break;
			}
		}
		
		else if (getMoreKeyReq.second_keys_size() == 1)
		{
			int gameDataLen = m_msgHandler->getLogicRedisService().getHField(getMoreKeyReq.primary_key().c_str(), getMoreKeyReq.primary_key().length(),
													                         getMoreKeyReq.second_keys(0).c_str(), getMoreKeyReq.second_keys(0).length(),
													                         dataBuffer, MaxMsgLen);
			if (gameDataLen <= 0)
			{
				getMoreKeyRsp.set_result(DBProxyGetMoreServiceDataError);
				OptErrorLog("get more service data from redis error, primary key = %s, second key = %s, buffer len = %u, rc = %d",
			    getMoreKeyReq.primary_key().c_str(), getMoreKeyReq.second_keys(0).c_str(), MaxMsgLen, gameDataLen);
				break;
			}
			
			gameDataVector.push_back(std::string(getMoreKeyReq.second_keys(0).c_str(), getMoreKeyReq.second_keys(0).length()));
			gameDataVector.push_back(std::string(dataBuffer, gameDataLen));
		}
		else
		{
			std::vector<std::string> vstrField;
			for (int idx = 0; idx < getMoreKeyReq.second_keys_size(); ++idx)
			{
				vstrField.push_back(getMoreKeyReq.second_keys(idx));
			}
			
			// 返回 0 成功
			int rc = m_msgHandler->getLogicRedisService().getMultiHField(getMoreKeyReq.primary_key().c_str(), getMoreKeyReq.primary_key().length(), vstrField, gameDataVector);
			if (rc != 0)
			{
				getMoreKeyRsp.set_result(DBProxyGetMoreServiceDataError);
				OptErrorLog("get more service data from redis error, primary key = %s, second key count = %u, rc = %d",
			    getMoreKeyReq.primary_key().c_str(), vstrField.size(), rc);
				break;
			}
		}
		
		for (unsigned int i = 1; i < gameDataVector.size(); i += 2)
		{
			com_protocol::SecondKeyServiceData* skData = getMoreKeyRsp.add_second_key_data();
			skData->set_key(gameDataVector[i - 1].c_str(), gameDataVector[i - 1].length());
			skData->set_data(gameDataVector[i].c_str(), gameDataVector[i].length());
		}
		
		getMoreKeyRsp.set_result(SrvOptSuccess);
		getMoreKeyRsp.set_primary_key(getMoreKeyReq.primary_key());
		getMoreKeyRsp.set_cb_data(getMoreKeyReq.cb_data());

		break;
	}
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_MOER_SERVICE_DATA_RSP;
	int sndRc = m_msgHandler->getSrvOpt().sendPkgMsgToService(getMoreKeyRsp, srcSrvId, srcProtocolId, "get more service data reply");
	
	if (getMoreKeyRsp.result() != 0 || sndRc != 0)
	{
		OptErrorLog("get more service data error, srcSrvId = %u, srcProtocolId = %d, msg byte len = %d, buff len = %d, result = %d, rc = %d",
		srcSrvId, srcProtocolId, getMoreKeyRsp.ByteSize(), MaxMsgLen, getMoreKeyRsp.result(), sndRc);
	}
}

void CServiceLogicData::delServiceData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::DelServiceDataReq delReq;
	com_protocol::DelServiceDataRsp delRsp;
	delRsp.set_result(DBProxyParseFromArrayError);
	while (m_msgHandler->getSrvOpt().parseMsgFromBuffer(delReq, data, len, "delete service data request"))
	{
		if (delReq.second_keys_size() < 1)
		{
			delRsp.set_result(InvalidParameter);
			break;
		}
		
		if (delReq.second_keys_size() == 1)
		{
			// 删除成功返回1，不存在返回0
			int rc = m_msgHandler->getLogicRedisService().delHField(delReq.primary_key().c_str(), delReq.primary_key().length(),
			                                                        delReq.second_keys(0).c_str(), delReq.second_keys(0).length());
			if (rc < 0)
			{
				delRsp.set_result(DBProxyDeleteServiceDataError);
				OptErrorLog("delete service data from redis error, primary key = %s, second key = %s, rc = %d",
			    delReq.primary_key().c_str(), delReq.second_keys(0).c_str(), rc);
				break;
			}

            delRsp.add_second_keys(delReq.second_keys(0));
			delRsp.set_deleted_count(1);
		}
		else
		{
			std::vector<std::string> vstrField;
			for (int idx = 0; idx < delReq.second_keys_size(); ++idx)
			{
				vstrField.push_back(delReq.second_keys(idx));
				delRsp.add_second_keys(delReq.second_keys(idx));
			}
			
			// 返回删除成功的个数
			int count = m_msgHandler->getLogicRedisService().delMultiHField(delReq.primary_key().c_str(), delReq.primary_key().length(), vstrField);
			if (count < 0)
			{
				delRsp.clear_second_keys();
				delRsp.set_result(DBProxyDeleteServiceDataError);
				OptErrorLog("delete service data from redis error, primary key = %s, second key count = %u, rc = %d",
			    delReq.primary_key().c_str(), vstrField.size(), count);
				break;
			}
			
			delRsp.set_deleted_count(count);
		}
		
		delRsp.set_result(SrvOptSuccess);
		delRsp.set_primary_key(delReq.primary_key());

		break;
	}
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_DEL_SERVICE_DATA_RSP;
	int sndRc = m_msgHandler->getSrvOpt().sendPkgMsgToService(delRsp, srcSrvId, srcProtocolId, "delete service data reply");
	
	if (delRsp.result() != 0 || sndRc != 0)
	{
		OptErrorLog("delete service data error, srcSrvId = %u, srcProtocolId = %d, msg byte len = %d, result = %d, rc = %d",
		srcSrvId, srcProtocolId, delRsp.ByteSize(), delRsp.result(), sndRc);
	}
}


void CServiceLogicData::getUserData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUserDataRsp rsp;
	com_protocol::GetUserDataReq req;
	if (m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get user data request"))
	{
		rsp.set_result(SrvOptSuccess);
		rsp.set_username(req.username());
		rsp.set_info_type(req.info_type());
		if (req.has_hall_id()) rsp.set_hall_id(req.hall_id());
		
		switch (req.info_type())
		{
			case com_protocol::EUserInfoType::EUserStaticDynamic:
			{
				// 获取DB中的用户信息
				CUserBaseinfo userBaseinfo;
	            rsp.set_result(m_msgHandler->getUserBaseinfo(req.username(), req.hall_id(), userBaseinfo));
				if (rsp.result() == SrvOptSuccess) m_msgHandler->setUserDetailInfo(userBaseinfo, *rsp.mutable_detail_info());
				
				break;
			}
			
			default:
			{
				rsp.set_result(DBProxyInvalidTypeGetUserData);
				OptErrorLog("get usre data error, name = %u, info type = %d", req.username().c_str(), req.info_type());
				break;
			}
		}
	}
	else
	{
		rsp.set_result(DBProxyParseFromArrayError);
	}
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_USER_DATA_RSP;
	m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get user data reply");
}

void CServiceLogicData::modifyUserData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientModifyUserDataRsp rsp;
	do
	{
		com_protocol::ClientModifyUserDataReq req;
		if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "modify user data request"))
		{
			rsp.set_result(DBProxyParseFromArrayError);
			break;
		}
        
        // 获取用户信息
		CUserBaseinfo userBaseinfo;
		rsp.set_result(m_msgHandler->getUserBaseinfo(req.static_info().username(), req.hall_id(), userBaseinfo));
		if (rsp.result() != SrvOptSuccess) break;
		
        // 昵称修改
        bool hasModify = false;
        if (req.static_info().has_nickname())
        {
            const string& nickname = req.static_info().nickname();
            if (nickname.length() < 1 || nickname.length() >= 32)
            {
                rsp.set_result(DBProxyNicknameInputUnlegal);
                break;
            }
            
            /*
            // nickname长度限制，自动截断
            const string newNickname = m_msgHandler->utf8SubStr(nickname.c_str(), nickname.length(),
                                                                0, m_msgHandler->m_pCfg->common_cfg.nickname_length);
            */
     
            if (m_msgHandler->getSensitiveWordFilter().isExistSensitiveStr((const unsigned char*)nickname.c_str(), nickname.length()))
            {
                rsp.set_result(DBProxyNicknameSensitiveStr);
                break;
            }

            hasModify = true;
            strcpy(userBaseinfo.static_info.nickname, nickname.c_str());
        }
        
        // 手机号码修改
        if (req.static_info().has_mobile_phone())
        {
            // 手机号，如1xxxxxxxxxx，格式必须为11位
            const unsigned int phoneNumberLen = 11;
            const string& mobilePhone = req.static_info().mobile_phone();
            if (mobilePhone.length() != phoneNumberLen || *mobilePhone.c_str() != '1'
                || !strIsDigit(mobilePhone.c_str(), mobilePhone.length()))
            {
                rsp.set_result(DBProxyMobilephoneInputUnlegal);
                break;
            }
            
            hasModify = true;
            strcpy(userBaseinfo.static_info.mobile_phone, mobilePhone.c_str());
        }
        
        if (!hasModify) return;
        
        // 将数据实时写入DB
		if (!m_msgHandler->updateUserStaticBaseinfoToMysql(userBaseinfo.static_info))
		{
			rsp.set_result(DBProxyUpdateDataToMysqlFailed);
			break;
		}
		
		if (!m_msgHandler->setUserBaseinfoToMemcached(userBaseinfo))
		{
			m_msgHandler->mysqlRollback();

			rsp.set_result(DBProxySetDataToMemcachedFailed);
			break;
		}

        m_msgHandler->mysqlCommit();

		rsp.set_result(SrvOptSuccess);
		rsp.set_allocated_static_info(req.release_static_info());
	
	} while (false);
	
	if (srcProtocolId == 0) srcProtocolId = DBPROXY_MODIFY_USER_DATA_RSP;
	m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "modify user data reply");
}

void CServiceLogicData::getUserAttributeValue(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
    com_protocol::GetUserAttributeValueRsp rsp;

    do
    {
        com_protocol::GetUserAttributeValueReq req;
        if (!m_msgHandler->getSrvOpt().parseMsgFromBuffer(req, data, len, "get user attribute value request"))
        {
            rsp.set_result(DBProxyParseFromArrayError);
            break;
        }
        
        CUserBaseinfo userBaseinfo;
        rsp.set_result(m_msgHandler->getUserBaseinfo(req.username(), req.hall_id(), userBaseinfo));
        if (rsp.result() != SrvOptSuccess) break;
        
        google::protobuf::RepeatedPtrField<com_protocol::UserAttributeKeyValue>* uaKeyValue = rsp.mutable_attribute_info()->mutable_attributes();
        for (google::protobuf::RepeatedPtrField<com_protocol::UserAttributeKeyValue>::iterator it = uaKeyValue->begin();
             it != uaKeyValue->end(); ++it)
        {
            switch (it->type())
            {
                case com_protocol::UserAttributeType::EUAPhoneNumber:
                {
                    it->set_string_value(userBaseinfo.static_info.mobile_phone);

                    break;
                }
                
                default:
                {
                    rsp.set_result(DBProxyInvalidUserAttributeType);
                    break;
                }
            }
            
            if (rsp.result() != SrvOptSuccess) break;
        }
        
        if (rsp.result() == SrvOptSuccess) rsp.set_allocated_attribute_info(req.release_attribute_info());
    
    } while (false);
    
    if (srcProtocolId == 0) srcProtocolId = DBPROXY_GET_USER_ATTRIBUTE_VALUE_RSP;
	m_msgHandler->getSrvOpt().sendPkgMsgToService(rsp, srcSrvId, srcProtocolId, "get user attribute value reply");
}

