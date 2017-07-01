
/* author : limingfan
 * date : 2016.08.11
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

#include "CLogic.h"
#include "CSrvMsgHandler.h"
#include "CLogicHandlerTwo.h"
#include "base/Function.h"
#include "../common/CommonType.h"


using namespace NProject;


// 兑换日志文件
#define ExchangeLog(format, args...)     m_msgHandler->getLogic().getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)


CLogicHandlerTwo::CLogicHandlerTwo()
{
	m_msgHandler = NULL;
}

CLogicHandlerTwo::~CLogicHandlerTwo()
{
	m_msgHandler = NULL;
}

int CLogicHandlerTwo::init(CSrvMsgHandler* msgHandler)
{
	m_msgHandler = msgHandler;
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_ADD_RED_GIFT_REQ, (ProtocolHandler)&CLogicHandlerTwo::addRedGiftReq, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_RECEIVE_RED_GIFT_REQ, (ProtocolHandler)&CLogicHandlerTwo::receiveRedGiftReq, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_PLAYER_MATCHING_OPT_NOTIFY, (ProtocolHandler)&CLogicHandlerTwo::writePKmatchingRecordReq, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_BINDING_EXCHANGE_MALL_INFO_REQ, (ProtocolHandler)&CLogicHandlerTwo::bindingExchangeMallAccount, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_RESET_EXCHANGE_PHONE_NUMBER_REQ, (ProtocolHandler)&CLogicHandlerTwo::resetExchangePhoneNumber, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_ADD_MAIL_MESSAGE_REQ, (ProtocolHandler)&CLogicHandlerTwo::addMailMessage, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_MAIL_MESSAGE_REQ, (ProtocolHandler)&CLogicHandlerTwo::getMailMessage, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_OPT_MAIL_MESSAGE_REQ, (ProtocolHandler)&CLogicHandlerTwo::optMailMessage, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_USER_WEALTH_INFO_REQ, (ProtocolHandler)&CLogicHandlerTwo::getWealthInfo, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_FF_CHESS_MALL_REQ, (ProtocolHandler)&CLogicHandlerTwo::getFFChessMall, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_EXCHANGE_FF_CHESS_GOODS_REQ, (ProtocolHandler)&CLogicHandlerTwo::exchangeFFChessGoods, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_USE_FF_CHESS_GOODS_REQ, (ProtocolHandler)&CLogicHandlerTwo::useFFChessGoods, this);
	
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_ADD_ACTIVITY_RED_PACKET_REQ, (ProtocolHandler)&CLogicHandlerTwo::addActivityRedPacket, this);
	m_msgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_ACTIVITY_RED_PACKET_REQ, (ProtocolHandler)&CLogicHandlerTwo::getActivityRedPacket, this);
	
	return 0;
}

void CLogicHandlerTwo::unInit()
{

}

void CLogicHandlerTwo::updateConfig()
{
	clearRobotScore();  // 清空机器人积分券
}


// 红包口令相关
void CLogicHandlerTwo::addRedGiftReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::AddRedGiftRsp rsp;
	rsp.set_result(ServiceCreateRedGiftFailed);
	
	// 先查找是否已经存在该用户的红包了
	char dataBuffer[512] = {0};
	CQueryResult* p_result = NULL;
	snprintf(dataBuffer, sizeof(dataBuffer) - 1, "select gift_id from tb_red_gift_info where username=\'%s\';", m_msgHandler->getContext().userData);
	if (Success == m_msgHandler->m_pDBOpt->queryTableAllResult(dataBuffer, p_result) && 1 == p_result->getRowCount())
	{
		rsp.set_result(ServiceSuccess);
		rsp.set_red_gift_id(p_result->getNextRow()[0]);
	}
	else
	{
		while (true)
		{
			// 获取该用户的注册时间点
			CUserBaseinfo user_baseinfo;
			if (!m_msgHandler->getUserBaseinfo(m_msgHandler->getContext().userData, user_baseinfo))
			{
				OptErrorLog("get user register time to add red gift error, user = %s", m_msgHandler->getContext().userData);
				break;
			}
			
			struct tm registTime;
			if (strptime(user_baseinfo.static_info.register_time, "%Y-%m-%d %H:%M:%S", &registTime) == NULL)
			{
				OptErrorLog("change user register time to add red gift error, user = %s, time stamp = %s",
				m_msgHandler->getContext().userData, user_baseinfo.static_info.register_time);
				break;
			}
			
			time_t timeSecs = mktime(&registTime);
			if (timeSecs == (unsigned int)-1)
			{
				OptErrorLog("change user register time stamp to add red gift error, user = %s, time stamp = %s",
				m_msgHandler->getContext().userData, user_baseinfo.static_info.register_time);
				break;
			}
			
			char redGiftId[32] = {0};
			getRedGiftId(redGiftId, sizeof(redGiftId));
			snprintf(dataBuffer, sizeof(dataBuffer) - 1, "insert into tb_red_gift_info(username,register_time,gift_id,received_gift_id,received_device_id,create_time) values(\'%s\',%u,\'%s\',\'\',\'\',now());", m_msgHandler->getContext().userData, (unsigned int)timeSecs, redGiftId);
			if (Success == m_msgHandler->m_pDBOpt->modifyTable(dataBuffer))
			{
				m_msgHandler->mysqlCommit();
				
				rsp.set_result(ServiceSuccess);
				rsp.set_red_gift_id(redGiftId);
				OptInfoLog("add red gift, user = %s, id = %s", m_msgHandler->getContext().userData, redGiftId);
			}
			else
			{
				m_msgHandler->mysqlRollback();
				OptErrorLog("add red gift error, user = %s, sql = %s", m_msgHandler->getContext().userData, dataBuffer);
			}
			
			break;
		}
	}
	m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
	
	unsigned int dataBufferLen = sizeof(dataBuffer);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, dataBuffer, dataBufferLen, "add red gift");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, dataBufferLen, srcSrvId, BUSINESS_ADD_RED_GIFT_RSP);
}

void CLogicHandlerTwo::receiveRedGiftReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ReceiveRedGiftReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "receive red gift")) return;
	
	char dataBuffer[512] = {0};
	unsigned int sqlLen = snprintf(dataBuffer, sizeof(dataBuffer) - 1, "select username,register_time,gift_id,received_gift_id,received_device_id from tb_red_gift_info where username=\'%s\' or gift_id=\'%s\' or received_device_id=\'%s\';", m_msgHandler->getContext().userData, req.red_gift_id().c_str(), req.device_id().c_str());
	
	com_protocol::ReceiveRedGiftRsp rsp;
	rsp.set_result(ServiceSuccess);
	
	CQueryResult* p_result = NULL;			
	if (Success == m_msgHandler->m_pDBOpt->queryTableAllResult(dataBuffer, sqlLen, p_result))
	{
		string redGiftUser;                                // 准备领取的红包所属的用户
		// unsigned int redGiftUserRegisterTime = 0;          // 准备领取的红包所属的用户的注册时间点
		// unsigned int receiverRegisterTime = 0;             // 领取红包的用户的注册时间点
		
		RowDataPtr rowData = NULL;
		while ((rowData = p_result->getNextRow()) != NULL)
		{
			if (strcmp(rowData[0], m_msgHandler->getContext().userData) == 0)      // 本用户信息
			{
				if (strlen(rowData[3]) > 0)
				{
					rsp.set_red_gift_id(rowData[3]);
					rsp.set_result(ServiceUserReceivedRedGift);
					OptErrorLog("the user already received red gift error, user = %s, received gift = %s, device = %s",
					m_msgHandler->getContext().userData, rowData[3], rowData[4]);
					
					// 该用户已经领取过红包了
					m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
					sqlLen = snprintf(dataBuffer, sizeof(dataBuffer) - 1, "select username from tb_red_gift_info where gift_id=\'%s\';", rowData[3]);
					if (Success == m_msgHandler->m_pDBOpt->queryTableAllResult(dataBuffer, sqlLen, p_result) && 1 == p_result->getRowCount())
					{
						redGiftUser = p_result->getNextRow()[0];
					}
	
					break;
				}

				// receiverRegisterTime = atoi(rowData[1]);
			}
			else if (strcmp(rowData[2], req.red_gift_id().c_str()) == 0)           // 目标用户信息
			{
				redGiftUser = rowData[0];
				// redGiftUserRegisterTime = atoi(rowData[1]);
			}
			
			if (strcmp(rowData[4], req.device_id().c_str()) == 0)
			{
				rsp.set_result(ServiceDeviceReceivedRedGift);
				OptErrorLog("the device already received red gift error, user = %s, received gift = %s, device = %s",
				m_msgHandler->getContext().userData, rowData[3], rowData[4]);
				break;
			}
		}
		m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
		
		if (rsp.result() == ServiceSuccess)
		{
			if (redGiftUser.empty())
			{
				rsp.set_result(ServiceNoFoundRedGiftError);
			}
			
			/*
			else if (redGiftUserRegisterTime == 0 || receiverRegisterTime == 0 || redGiftUserRegisterTime >= receiverRegisterTime)
			{
				rsp.set_result(ServiceRedGiftTimeError);
			}
			*/ 
		}
		
		while (rsp.result() == ServiceSuccess)
		{
			sqlLen = snprintf(dataBuffer, sizeof(dataBuffer) - 1, "update tb_red_gift_info set received_gift_id=\'%s\',received_device_id=\'%s\' where username=\'%s\';",
			req.red_gift_id().c_str(), req.device_id().c_str(), m_msgHandler->getContext().userData);
			if (Success != m_msgHandler->m_pDBOpt->executeSQL(dataBuffer, sqlLen))
			{
				rsp.set_result(ServiceReceiveRedGiftError);
				OptErrorLog("receive red gift error, user = %s, sql = %s", m_msgHandler->getContext().userData, dataBuffer);
				break;
			}
	
			google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo> goodsList;
			com_protocol::ItemChangeInfo* itemInfo = goodsList.Add();
			itemInfo->set_type(req.gift_info().type());
			itemInfo->set_count(req.gift_info().num());
			
			// 写游戏记录
			com_protocol::BuyuGameRecordStorageExt userReceiveRedGift;
			userReceiveRedGift.set_room_rate(0);
			userReceiveRedGift.set_room_name("大厅");
			userReceiveRedGift.set_remark("领取红包口令");
			com_protocol::ItemRecordStorage* itemRecord = userReceiveRedGift.add_items();
			itemRecord->set_item_type(req.gift_info().type());
			itemRecord->set_charge_count(req.gift_info().num());

            com_protocol::GameRecordPkg record;
            unsigned int dataBufferLen = sizeof(dataBuffer);
			const char* msgData = m_msgHandler->serializeMsgToBuffer(userReceiveRedGift, dataBuffer, dataBufferLen, "receive red gift record");
			if (msgData != NULL)
			{
				record.set_game_type(NProject::GameRecordType::BuyuExt);
				record.set_game_record_bin(msgData, dataBufferLen);
			}
		
			int result = m_msgHandler->changeUserPropertyValue(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, goodsList, &record);
			if (result != ServiceSuccess)
			{
				m_msgHandler->mysqlRollback();
				rsp.set_result(ServiceReceiveRedGiftError);
				OptErrorLog("receive red gift change property value error, user = %s, result = %d", m_msgHandler->getContext().userData, result);
			}
			else
			{
				rsp.set_red_gift_id(req.red_gift_id());
				m_msgHandler->mysqlCommit();
			}

			break;
		}
		
		if (!redGiftUser.empty())
		{
			rsp.set_username(redGiftUser);
			CUserBaseinfo redGiftUserBaseinfo;
			if (m_msgHandler->getUserBaseinfo(redGiftUser.c_str(), redGiftUserBaseinfo))
			{
				redGiftUser = redGiftUserBaseinfo.static_info.nickname;
			}
			else
			{
				OptErrorLog("get red gift user base info error, user = %s", redGiftUser.c_str());
			}
			rsp.set_nickname(redGiftUser);
		}
	}
	else
	{
		rsp.set_result(ServiceReceiveRedGiftError);
		OptErrorLog("get red gift data error, user = %s, sql = %s", m_msgHandler->getContext().userData, dataBuffer);
	}

    unsigned int msgBufferLen = sizeof(dataBuffer);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, dataBuffer, msgBufferLen, "receive red gift send reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, msgBufferLen, srcSrvId, BUSINESS_RECEIVE_RED_GIFT_RSP);
}


void CLogicHandlerTwo::getRedGiftId(char* redGfitId, const unsigned int len)
{
	static time_t redGiftTimeSecs = time(NULL);
	static unsigned int index = 0;
	
	const unsigned int maxIndex = 100;
	const unsigned int timeSecsMod = 1000000000;
	const time_t curSecs = time(NULL);
	if (++index >= maxIndex)
	{
		index = 1;  // 索引重新开始
		if (curSecs <= redGiftTimeSecs) ++redGiftTimeSecs;  // 索引重新开始，则必须保证时间戳不同，否则会出现重复的ID
	}
	if (curSecs > redGiftTimeSecs) redGiftTimeSecs = curSecs;
	
	snprintf(redGfitId, len - 1, "%u%u", index, (unsigned int)(redGiftTimeSecs % timeSecsMod));  // 取模去掉时间戳的第一位
}


// PK场匹配写记录
void CLogicHandlerTwo::writePKmatchingRecordReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::PKPlayerMatchingNotify writeMatchingNotify;
	if (!m_msgHandler->parseMsgFromBuffer(writeMatchingNotify, data, len, "write player matching notify")) return;
	
	char dataBuffer[128] = {0};
	const unsigned int sqlLen = snprintf(dataBuffer, sizeof(dataBuffer) - 1, "insert into tb_buyu_pk_matching(username,opt_time,opt_flag,remarks) values(\'%s\',now(),%u,\'%s\');",
	m_msgHandler->getContext().userData, writeMatchingNotify.opt_flag(), writeMatchingNotify.remarks().c_str());
	if (Success != m_msgHandler->getMySqlOptDB()->executeSQL(dataBuffer, sqlLen))
	{
		OptErrorLog("write player PK matching record error, user = %s, sql = %s", m_msgHandler->getContext().userData, dataBuffer);
	}
}

// 绑定兑换商城账号信息
void CLogicHandlerTwo::bindingExchangeMallAccount(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::BindingExchangeMallAccountReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "binding exchange mall request")) return;
	
	enum
	{
		EBindingPhoneNumber = 1,
		EBindingEMailBox = 2,
	};
	
	com_protocol::BindingExchangeMallAccountRsp rsp;
	rsp.set_result(ServiceGetUserinfoFailed);
	
	CUserBaseinfo user_baseinfo;
	while (m_msgHandler->getUserBaseinfo(m_msgHandler->getContext().userData, user_baseinfo))
	{
		if (req.opt() == EBindingPhoneNumber)
		{
			if (user_baseinfo.static_info.mobile_phone[0] != '\0')  // 是否已经绑定过手机号码了
			{
				rsp.set_result(ServiceAlreadyBindMobilePhone);
				break;
			}
			
			// 检查密码是不是MD5格式
			if (req.password().length() != 32 || !strIsUpperHex(req.password().c_str()))
			{
				rsp.set_result(ServicePasswdInputUnlegal);
				break;
			}
			
			// 手机号码合法性校验，手机号码至少大于10位
			if (!req.has_phone_number() || req.phone_number().length() < 10 || req.phone_number().length() > 16 || !strIsAlnum(req.phone_number().c_str()))
			{
				rsp.set_result(ServiceMobilephoneInputUnlegal);  // 手机号非法
				break;
			}
			
			// 查询该手机号码是否已经绑定过了，高效查询语句值检查是否存在
			CQueryResult* p_result = NULL;
			char sql[128] = { 0 };
			const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "select 1 from tb_user_static_baseinfo where mobile_phone=\'%s\' limit 1;", req.phone_number().c_str());
			if (Success != m_msgHandler->m_pDBOpt->queryTableAllResult(sql, sqlLen, p_result))
			{
				OptErrorLog("binding mobile phone error, user = %s, exec sql error:%s", m_msgHandler->getContext().userData, sql);
				rsp.set_result(ServiceBindMobilePhoneFailed);
				break;
			}
			
			if (p_result != NULL && p_result->getRowCount() > 0)
			{
				m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
				OptErrorLog("already exist bind mobile phone = %s, user = %s", req.phone_number().c_str(), m_msgHandler->getContext().userData);
				rsp.set_result(ExistBindMobilePhoneError);
				break;
			}
			m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
			
			strncpy(user_baseinfo.static_info.mobile_phone, req.phone_number().c_str(), sizeof(user_baseinfo.static_info.mobile_phone) - 1);
			if (req.has_mail_box_name() && strchr(req.mail_box_name().c_str(), '@') != NULL && req.mail_box_name().length() < sizeof(user_baseinfo.static_info.email) - 1)
			{
				strncpy(user_baseinfo.static_info.email, req.mail_box_name().c_str(), sizeof(user_baseinfo.static_info.email) - 1);
			}
		}
		else if (req.opt() == EBindingEMailBox)
		{
			if (!req.has_mail_box_name() || strchr(req.mail_box_name().c_str(), '@') == NULL || req.mail_box_name().length() >= sizeof(user_baseinfo.static_info.email) - 1)
			{
				rsp.set_result(ServiceEmailInputUnlegal);
				break;
			}
			
			strncpy(user_baseinfo.static_info.email, req.mail_box_name().c_str(), sizeof(user_baseinfo.static_info.email) - 1);
		}
		else
		{
			OptErrorLog("binding mobile phone error, user = %s, opt = %d", m_msgHandler->getContext().userData, req.opt());
			rsp.set_result(ServiceBindMobilePhoneFailed);
			break;
		}
		
		// 将数据写入DB
		if (m_msgHandler->updateUserStaticBaseinfoToMysql(user_baseinfo.static_info))
		{
			if (req.opt() == EBindingPhoneNumber)
			{
				// 记录用户&密码
				char sql_tmp[2048] = { 0 };
				const unsigned int sql_len = snprintf(sql_tmp, sizeof(sql_tmp) - 1, "replace into tb_exchange_mall_account(username,password,created_at) values(\'%s\',\'%s\',%u);",
				m_msgHandler->getContext().userData, req.password().c_str(), (unsigned int)time(NULL));
				if (Success != m_msgHandler->getMySqlOptDB()->executeSQL(sql_tmp, sql_len))
				{
					m_msgHandler->mysqlRollback();
					OptErrorLog("add bind exchange mall info record error, user = %s, sql = %s", m_msgHandler->getContext().userData, sql_tmp);
					
					rsp.set_result(ServiceUpdateDataToMysqlFailed);
					break;
				}
			}
		
			if (m_msgHandler->setUserBaseinfoToMemcached(user_baseinfo))
			{
				m_msgHandler->mysqlCommit();
			}
			else
			{
				m_msgHandler->mysqlRollback();
				rsp.set_result(ServiceUpdateDataToMemcachedFailed);
				break;
			}
		}
		else
		{
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}
		
		rsp.set_result(ServiceSuccess);
		rsp.set_allocated_reqdata(&req);
		break;
	}
	
	char dataBuffer[128] = {0};
	unsigned int msgBufferLen = sizeof(dataBuffer);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, dataBuffer, msgBufferLen, "bind exchange mall user info reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, msgBufferLen, srcSrvId, BUSINESS_BINDING_EXCHANGE_MALL_INFO_RSP);
	
	if (rsp.result() == ServiceSuccess) rsp.release_reqdata();
}

// 重置玩家兑换话费时绑定的手机号码
void CLogicHandlerTwo::resetExchangePhoneNumber(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ResetExchangePhoneNumberReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "reset exchange phone number request")) return;

	com_protocol::ResetExchangePhoneNumberRsp rsp;
	rsp.set_result(ServiceSuccess);
	
	do 
	{
		// 获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_msgHandler->getUserBaseinfo(req.username(), user_baseinfo))
		{
			rsp.set_result(ServiceGetUserinfoFailed);//获取DB中的信息失败
			break;
		}
		
		if (req.has_new_phone_number() && req.new_phone_number().length() > 0)
		{
			// 查询该手机号码是否已经绑定过了，高效查询语句值检查是否存在
			CQueryResult* p_result = NULL;
			while (true)
			{
				char sql[128] = { 0 };
				const unsigned int sqlLen = snprintf(sql, sizeof(sql) - 1, "select 1 from tb_user_dynamic_baseinfo where mobile_phone_number=\'%s\' limit 1;", req.new_phone_number().c_str());
				if (Success != m_msgHandler->m_pDBOpt->queryTableAllResult(sql, sqlLen, p_result))
				{
					OptErrorLog("reset exchange phone number, exec sql error = %s", sql);
					rsp.set_result(SaveExchangeMobilePhoneError);
					break;
				}
				
	            if (p_result != NULL && p_result->getRowCount() > 0)
				{
					OptErrorLog("reset exchange phone number, already exist exchange mobile phone = %s, user = %s", req.new_phone_number().c_str(), req.username().c_str());
					rsp.set_result(ExistExchangeMobilePhoneError);
					break;
				}
				
				break;
			}
			m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
			
			if (rsp.result() != ServiceSuccess) break;
	
			// 修改兑换绑定号码
			strncpy(user_baseinfo.dynamic_info.mobile_phone_number, req.new_phone_number().c_str(), DBStringFieldLength - 1);
		}
		else
		{
			memset(user_baseinfo.dynamic_info.mobile_phone_number, 0, sizeof(user_baseinfo.dynamic_info.mobile_phone_number));
		}

		//写DB
		//写动态信息
		if (!m_msgHandler->updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info))
		{
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}

		//写到memcached
		if (!m_msgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			m_msgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			break;
		}

		//提交
		m_msgHandler->mysqlCommit();
	} while (false);
	
	int rc = ServiceFailed;
	char send_data[MaxMsgLen] = { 0 };
	unsigned int send_data_len = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, send_data, send_data_len, "send reset exchange phone number reply");
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_RESET_EXCHANGE_PHONE_NUMBER_RSP);
	
	ExchangeLog("reset exchange phone number|%s|%d|%s|%s", req.username().c_str(), rsp.result(), req.new_phone_number().c_str(), req.reason_info().c_str());
	
	OptInfoLog("reset exchange phone number, user = %s, new exchange mobile phone = %s, reason = %s, result = %d, rc = %d",
	req.username().c_str(), req.new_phone_number().c_str(), req.reason_info().c_str(), rsp.result(), rc);
}


// 添加邮件信息
void CLogicHandlerTwo::addMailMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::AddMailRewardMessageReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "add mail reward message request")) return;
	
	ServiceLogicData& srvData = m_msgHandler->getLogicDataInstance().setLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
	google::protobuf::RepeatedPtrField<com_protocol::RewardMessage>* rewardMsg = srvData.logicData.mutable_mail_message()->mutable_reward_msg();
	const unsigned int id = (rewardMsg->size() > 0) ? (rewardMsg->Get(rewardMsg->size() - 1).id() + 1) : 1;
	
	// 添加新的奖励消息
	com_protocol::RewardMessage* newRewardMsg = rewardMsg->Add();
	newRewardMsg->set_id(id);
	newRewardMsg->set_time_secs(time(NULL));
	newRewardMsg->set_status(EMailMessageStatus::EMessageUnRead);
	newRewardMsg->set_title(req.title());
	newRewardMsg->set_content(req.content());
	
	if (req.gifts_size() > 0)
	{
		const google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>& gifts = req.gifts();
		for (google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>::const_iterator giftIt = gifts.begin(); giftIt != gifts.end(); ++giftIt)
		{
			com_protocol::GiftInfo* newGift = newRewardMsg->add_gifts();
			*newGift = *giftIt;
		}
	}
}

// 获取邮件信息
void CLogicHandlerTwo::getMailMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	const ServiceLogicData& srvData = m_msgHandler->getLogicDataInstance().getLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
	const google::protobuf::RepeatedPtrField<com_protocol::RewardMessage>& rewardMsg = srvData.logicData.mail_message().reward_msg();

    unsigned int timeOutSecs = time(NULL) - m_msgHandler->m_config.common_cfg.mail_message_timeout;  // 超时时间点
	bool hasTimeOutMsg = false;
	
	com_protocol::GetMailMessageRsp rsp;
	int idx = rewardMsg.size();
    while (idx > 0)
	{
		--idx;
		if (rewardMsg.Get(idx).time_secs() < timeOutSecs)
		{
			hasTimeOutMsg = true;
			break;
		}
		
		com_protocol::RewardMessage* rwMsg = (rewardMsg.Get(idx).status() != EMailMessageStatus::EMessageUnRead) ? rsp.add_readed() : rsp.add_un_read();
		*rwMsg = rewardMsg.Get(idx);
	}
	
	char dataBuffer[MaxMsgLen] = {0};
	unsigned int msgBufferLen = sizeof(dataBuffer);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, dataBuffer, msgBufferLen, "get mail message reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, msgBufferLen, srcSrvId, BUSINESS_GET_MAIL_MESSAGE_RSP);
	
	// 删除超时的消息
	if (hasTimeOutMsg)
	{
		ServiceLogicData& removeSrvData = m_msgHandler->getLogicDataInstance().setLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
	    google::protobuf::RepeatedPtrField<com_protocol::RewardMessage>* removeRewardMsg = removeSrvData.logicData.mutable_mail_message()->mutable_reward_msg();
		removeRewardMsg->DeleteSubrange(0, idx + 1);
	}
}
	
// 操作邮件信息
void CLogicHandlerTwo::optMailMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::OptMailMessageReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "operate mail message request")) return;
	
	ServiceLogicData& srvData = m_msgHandler->getLogicDataInstance().setLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
	google::protobuf::RepeatedPtrField<com_protocol::RewardMessage>* rewardMsg = srvData.logicData.mutable_mail_message()->mutable_reward_msg();
	
	com_protocol::OptMailMessageRsp rsp;
	const google::protobuf::RepeatedField<google::protobuf::uint32>& ids = req.ids();
	if (req.opt() == EMailMessageStatus::ECheckUnReadMessage)  // 查看是否存在未读邮件
	{
		unsigned int timeOutSecs = time(NULL) - m_msgHandler->m_config.common_cfg.mail_message_timeout;  // 超时时间点
		int idx = rewardMsg->size();
		while (idx > 0)
		{
			--idx;
			const com_protocol::RewardMessage& rwMsg = rewardMsg->Get(idx);
			if (rwMsg.time_secs() < timeOutSecs) break;

            if (rwMsg.status() == EMailMessageStatus::EMessageUnRead)
			{
				rsp.set_opt(req.opt());
		        rsp.set_result(ServiceSuccess);
				rsp.add_ids(rwMsg.id());
				break;
			}
		}
	}
	
	else if (req.opt() == EMailMessageStatus::EDeleteMessage)  // 删除消息
	{
		for (google::protobuf::RepeatedField<google::protobuf::uint32>::const_iterator idIt = ids.begin(); idIt != ids.end(); ++idIt)
		{
			for (int idx = 0; idx < rewardMsg->size(); ++idx)
			{
				if (*idIt == rewardMsg->Get(idx).id())
				{
					rewardMsg->DeleteSubrange(idx, 1);
					break;
				}
			}
		}
	}
	
	else if (req.opt() == EMailMessageStatus::EMessageReaded)  // 设置为已读消息
	{
		for (google::protobuf::RepeatedField<google::protobuf::uint32>::const_iterator idIt = ids.begin(); idIt != ids.end(); ++idIt)
		{
			for (google::protobuf::RepeatedPtrField<com_protocol::RewardMessage>::iterator rwIt = rewardMsg->begin(); rwIt != rewardMsg->end(); ++rwIt)
			{
				if (*idIt == rwIt->id())
				{
					rwIt->set_status(EMailMessageStatus::EMessageReaded);
					break;
				}
			}
		}
	}
	
	else if (req.opt() == EMailMessageStatus::EMessageReceivedGift)  // 领取奖励
	{
		google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo> goodsList;  // 领取的物品
		com_protocol::BuyuGameRecordStorageExt recordData;                           // 游戏记录
		string remark = "邮箱奖励";

		vector<unsigned int> receiveRewardIndexs;
		rsp.set_opt(req.opt());
		rsp.set_result(ServiceSuccess);
		for (google::protobuf::RepeatedField<google::protobuf::uint32>::const_iterator idIt = ids.begin(); idIt != ids.end(); ++idIt)
		{
			bool isFind = false;
			for (int idx = 0; idx < rewardMsg->size(); ++idx)
			{
				if (*idIt == rewardMsg->Get(idx).id())
				{
					isFind = true;
					receiveRewardIndexs.push_back(idx);
					
					if (rewardMsg->Get(idx).status() != EMailMessageStatus::EMessageReceivedGift)
					{
						rsp.add_ids(*idIt);
						const google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>& rewardGifts = rewardMsg->Get(idx).gifts();
						for (google::protobuf::RepeatedPtrField<com_protocol::GiftInfo>::const_iterator giftIt = rewardGifts.begin(); giftIt != rewardGifts.end(); ++giftIt)
						{
							com_protocol::ItemChangeInfo* itemInfo = goodsList.Add();
							itemInfo->set_type(giftIt->type());
							itemInfo->set_count(giftIt->num());
							
							com_protocol::ItemRecordStorage* itemRecord = recordData.add_items();
							itemRecord->set_item_type(giftIt->type());
							itemRecord->set_charge_count(giftIt->num());
							
							com_protocol::GiftInfo* rspGift = rsp.add_gifts();
							*rspGift = *giftIt;
						}
						
						remark += ("-" + rewardMsg->Get(idx).title());
					}
					else
					{
						rsp.set_result(AlreadyReceivedMessageReward);  // 该消息的奖励已经领取过了
					}
				
					break;
				}
			}
			
			if (!isFind) rsp.set_result(ServiceNotFoundMailMessage);  // 找不到相应的消息
			if (rsp.result() != ServiceSuccess) break;
		}
		
		if (rsp.result() == ServiceSuccess && goodsList.size() > 0)
		{
			// 写游戏记录
			recordData.set_room_rate(0);
			recordData.set_room_name("大厅");
			recordData.set_remark(remark);
            int result = m_msgHandler->changeUserPropertyValue(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, goodsList, recordData);  // 金币&道具&玩家属性等数量变更修改
			rsp.set_result(result);
			
			if (rsp.result() == ServiceSuccess)
			{
				for (vector<unsigned int>::const_iterator receivedIt = receiveRewardIndexs.begin(); receivedIt != receiveRewardIndexs.end(); ++receivedIt)
				{
					rewardMsg->Mutable(*receivedIt)->set_status(EMailMessageStatus::EMessageReceivedGift);  // 刷新状态为已经领取了奖励
				}
			}
		}
	}
	
	if (rsp.opt() == EMailMessageStatus::ECheckUnReadMessage || rsp.opt() == EMailMessageStatus::EMessageReceivedGift)
	{
		char dataBuffer[MaxMsgLen] = {0};
		unsigned int msgBufferLen = sizeof(dataBuffer);
		const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, dataBuffer, msgBufferLen, "operate mail message reply");
		if (msgData != NULL) m_msgHandler->sendMessage(msgData, msgBufferLen, srcSrvId, BUSINESS_OPT_MAIL_MESSAGE_RSP);
	}
}

// 获取玩家财富相关信息
void CLogicHandlerTwo::getWealthInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetUserWealthInfoReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "get user wealth info request")) return;

    com_protocol::GetUserWealthInfoRsp rsp;
	rsp.set_result(ServiceGetUserinfoFailed);
	rsp.set_query_username(req.query_username());
	
	char dataBuffer[MaxMsgLen] = {0};
	CUserBaseinfo user_baseinfo;
	while (m_msgHandler->getUserBaseinfo(req.query_username(), user_baseinfo))
	{
		// 查询该用户是否兑换过话费
		// select 1 from tb_exchange_record_info where username='10000107' and pay_type=13 limit 1;
		CQueryResult* p_result = NULL;
		unsigned int sqlLen = snprintf(dataBuffer, sizeof(dataBuffer) - 1, "select 1 from tb_exchange_record_info where username=\'%s\' and pay_type=%d limit 1;",
		req.query_username().c_str(), EPropType::PropPhoneFareValue);
		if (Success != m_msgHandler->m_pDBOpt->queryTableAllResult(dataBuffer, sqlLen, p_result))
		{
			OptErrorLog("get user wealth exchange phone fare error, user = %s, exec sql error = %s", req.query_username().c_str(), dataBuffer);
			break;
		}
		
		if (p_result != NULL && p_result->getRowCount() > 0) rsp.set_exchange_phone_fare(1);  // 已经兑换过话费了
		m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
		
		if (!rsp.has_exchange_phone_fare())
		{
			// 查询官网兑换商城
			// select 1 from tb_exchange_mall_orders where username='10000107' and charge_type=13 limit 1;
			p_result = NULL;
			sqlLen = snprintf(dataBuffer, sizeof(dataBuffer) - 1, "select 1 from tb_exchange_mall_orders where username=\'%s\' and charge_type=%d limit 1;",
			req.query_username().c_str(), EPropType::PropPhoneFareValue);
			if (Success != m_msgHandler->getMySqlOptDB()->queryTableAllResult(dataBuffer, sqlLen, p_result))
			{
				OptErrorLog("get user wealth exchange mall phone fare error, user = %s, exec sql error = %s", req.query_username().c_str(), dataBuffer);
				break;
			}
			
			if (p_result != NULL && p_result->getRowCount() > 0) rsp.set_exchange_phone_fare(1);  // 已经兑换过话费了
			m_msgHandler->getMySqlOptDB()->releaseQueryResult(p_result);
		}
		
		rsp.set_current_phone_fare(user_baseinfo.dynamic_info.phone_fare);
		rsp.set_sum_recharge_value(user_baseinfo.dynamic_info.sum_recharge);
		rsp.set_result(ServiceSuccess);
		
		break;
	}
	
	unsigned int msgBufferLen = sizeof(dataBuffer);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, dataBuffer, msgBufferLen, "get user wealth info reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, msgBufferLen, srcSrvId, BUSINESS_GET_USER_WEALTH_INFO_RSP);
}

void CLogicHandlerTwo::setFFChessGoods(const unsigned int curSecs, const com_protocol::FFChessGoodsData* ffChessGoods, const MallConfigData::chess_goods& chessGoodsCfg,
                                       const com_protocol::FFChessExchangeType type, com_protocol::FFChessItem* chessGoods)
{
	chessGoods->set_id(chessGoodsCfg.id);
	chessGoods->set_name(chessGoodsCfg.name);
	chessGoods->set_desc(chessGoodsCfg.desc);
	
	com_protocol::EChessItemStatus chessStatus = com_protocol::EChessItemStatus::ENotBuyChess;
	if (ffChessGoods != NULL)
	{
		const unsigned int currentGoodsId = (type == com_protocol::FFChessExchangeType::EFFChessBoard) ? ffChessGoods->current_board_id() : ffChessGoods->current_pieces_id();
		const google::protobuf::RepeatedPtrField<com_protocol::FFChessGoodsInfo>& ffChessGoodsList = (type == com_protocol::FFChessExchangeType::EFFChessBoard) ? ffChessGoods->chess_board() : ffChessGoods->chess_pieces();
		for (google::protobuf::RepeatedPtrField<com_protocol::FFChessGoodsInfo>::const_iterator it = ffChessGoodsList.begin(); it != ffChessGoodsList.end(); ++it)
		{
			if (it->id() == chessGoodsCfg.id)
			{
				chessStatus = (currentGoodsId == chessGoodsCfg.id) ? com_protocol::EChessItemStatus::EInUseChess : com_protocol::EChessItemStatus::EAlreadyBuyChess;
				
				if (it->valid_time() == 0)
				{
					// 永久使用
					chessGoods->set_end_day(0);
					chessGoods->set_item_valid_day(0);
				}
				else if (it->valid_time() > curSecs)
				{
					chessGoods->set_end_day(it->valid_time());
					chessGoods->set_item_valid_day(it->use_day());
				}
				else
				{
					chessStatus = com_protocol::EChessItemStatus::ENotBuyChess;
				}
				
				break;
			}
		}
	}

	chessGoods->set_status(chessStatus);
	if (chessStatus == com_protocol::EChessItemStatus::ENotBuyChess)
	{
		for (map<unsigned int, unsigned int>::const_iterator dpIt = chessGoodsCfg.date_price.begin(); dpIt != chessGoodsCfg.date_price.end(); ++dpIt)
		{
			com_protocol::FFChessItemDatePrice* dpInfo = chessGoods->add_date_price();
			dpInfo->set_day(dpIt->first);
			dpInfo->set_price(dpIt->second);
		}
	}
}

void CLogicHandlerTwo::setFFChessGiftPackage(const unsigned int curSecs, const com_protocol::FFChessGoodsData* ffChessGoods, const MallConfigData::chess_gift_package& chessGoodsCfg,
                                             com_protocol::FFChessGiftPackage* ffChessGiftPackage)
{
	// 礼包
	ffChessGiftPackage->set_id(chessGoodsCfg.id);
	ffChessGiftPackage->set_picture(chessGoodsCfg.picture);
	ffChessGiftPackage->set_name(chessGoodsCfg.name);
	ffChessGiftPackage->set_desc(chessGoodsCfg.desc);
	ffChessGiftPackage->set_price(chessGoodsCfg.buy_price);
	
	const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();
	if (chessGoodsCfg.board_id > 0)
	{
		for (vector<MallConfigData::chess_goods>::const_iterator boardIt = mallData.ff_chess_board_list.begin(); boardIt != mallData.ff_chess_board_list.end(); ++boardIt)
		{
			if (boardIt->id == chessGoodsCfg.board_id)
			{
				setFFChessGoods(curSecs, ffChessGoods, *boardIt, com_protocol::FFChessExchangeType::EFFChessBoard, ffChessGiftPackage->add_gift_chess_board());
				break;
			}
		}
	}
	
	if (chessGoodsCfg.pieces_id > 0)
	{
		for (vector<MallConfigData::chess_goods>::const_iterator piecesIt = mallData.ff_chess_pieces_list.begin(); piecesIt != mallData.ff_chess_pieces_list.end(); ++piecesIt)
		{
			if (piecesIt->id == chessGoodsCfg.pieces_id)
			{
				setFFChessGoods(curSecs, ffChessGoods, *piecesIt, com_protocol::FFChessExchangeType::EFFChessPieces, ffChessGiftPackage->add_gift_chess_pieces());
				break;
			}
		}
	}
	
	// 其他额外的物品
	for (map<unsigned int, unsigned int>::const_iterator it = chessGoodsCfg.gifts.begin(); it != chessGoodsCfg.gifts.end(); ++it)
	{
		com_protocol::GiftInfo* giftGoods = ffChessGiftPackage->add_gift_other_goods();
		giftGoods->set_type(it->first);
		giftGoods->set_num(it->second);
	}
}

void CLogicHandlerTwo::getFFChessMall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetFFChessMallReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "get ff chess mall")) return;
	
	const time_t curSecs = time(NULL);
	com_protocol::GetFFChessMallRsp rsp;
	const com_protocol::FFChessGoodsData& ffChessGoods = m_msgHandler->getLogicDataInstance().getFFChessGoods(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen);
	const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();
	for (vector<MallConfigData::chess_goods>::const_iterator boardIt = mallData.ff_chess_board_list.begin(); boardIt != mallData.ff_chess_board_list.end(); ++boardIt)
	{
		com_protocol::FFChessItem* chessBoard = rsp.add_chess_board();  // 棋盘
		setFFChessGoods(curSecs, &ffChessGoods, *boardIt, com_protocol::FFChessExchangeType::EFFChessBoard, chessBoard);
	}
	
	for (vector<MallConfigData::chess_goods>::const_iterator piecesIt = mallData.ff_chess_pieces_list.begin(); piecesIt != mallData.ff_chess_pieces_list.end(); ++piecesIt)
	{
		com_protocol::FFChessItem* chessPieces = rsp.add_chess_pieces();  // 棋子
		setFFChessGoods(curSecs, &ffChessGoods, *piecesIt, com_protocol::FFChessExchangeType::EFFChessPieces, chessPieces);
	}
	
	// 礼包
	for (vector<MallConfigData::chess_gift_package>::const_iterator giftIt = mallData.ff_chess_gift_package_list.begin(); giftIt != mallData.ff_chess_gift_package_list.end(); ++giftIt)
	{
		com_protocol::FFChessGiftPackage* ffChessGiftPackage = rsp.add_gift_package();  // 礼包
		setFFChessGiftPackage(curSecs, &ffChessGoods, *giftIt, ffChessGiftPackage);
	}
	
	char dataBuffer[MaxMsgLen] = {0};
	unsigned int msgBufferLen = sizeof(dataBuffer);
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, dataBuffer, msgBufferLen, "get ff chess mall reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, msgBufferLen, srcSrvId, BUSINESS_GET_FF_CHESS_MALL_RSP);
}

bool CLogicHandlerTwo::getFFChessGoodsInfo(const com_protocol::ExchangeFFChessItemReq& req, MallConfigData::chess_gift_package& giftPackage, com_protocol::ExchangeFFChessItemRsp& rsp)
{
	giftPackage.id = 0;
	giftPackage.board_id = 0;
	giftPackage.pieces_id = 0;
	giftPackage.buy_price = 0;
	giftPackage.name.clear();

	const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();
	switch (req.type())
	{
		case com_protocol::FFChessExchangeType::EFFChessBoard:
		{
			for (vector<MallConfigData::chess_goods>::const_iterator boardIt = mallData.ff_chess_board_list.begin(); boardIt != mallData.ff_chess_board_list.end(); ++boardIt)
			{
				if (boardIt->id == req.id())
				{
					map<unsigned int, unsigned int>::const_iterator dpIt = boardIt->date_price.find(req.day());
					if (dpIt == boardIt->date_price.end()) break;
						
					giftPackage.board_id = boardIt->id;
					giftPackage.buy_price = dpIt->second;
					giftPackage.name = boardIt->name;
					
					com_protocol::FFChessItem* chessGoods = rsp.mutable_ff_chess_item();
					setFFChessGoods(0, NULL, *boardIt, com_protocol::FFChessExchangeType::EFFChessBoard, chessGoods);
					chessGoods->set_status(com_protocol::EChessItemStatus::EAlreadyBuyChess);
					
					return true;
				}
			}
			
			break;
		}
		
		case com_protocol::FFChessExchangeType::EFFChessPieces:
		{
			for (vector<MallConfigData::chess_goods>::const_iterator piecesIt = mallData.ff_chess_pieces_list.begin(); piecesIt != mallData.ff_chess_pieces_list.end(); ++piecesIt)
			{
				if (piecesIt->id == req.id())
				{
					map<unsigned int, unsigned int>::const_iterator dpIt = piecesIt->date_price.find(req.day());
					if (dpIt == piecesIt->date_price.end()) break;
					
					giftPackage.pieces_id = piecesIt->id;
					giftPackage.buy_price = dpIt->second;
					giftPackage.name = piecesIt->name;
					
					com_protocol::FFChessItem* chessGoods = rsp.mutable_ff_chess_item();
					setFFChessGoods(0, NULL, *piecesIt, com_protocol::FFChessExchangeType::EFFChessPieces, chessGoods);
					chessGoods->set_status(com_protocol::EChessItemStatus::EAlreadyBuyChess);
					
					return true;
				}
			}
			
			break;
		}
		
		case com_protocol::FFChessExchangeType::EFFChessGiftPackage:
		{
			for (vector<MallConfigData::chess_gift_package>::const_iterator giftIt = mallData.ff_chess_gift_package_list.begin(); giftIt != mallData.ff_chess_gift_package_list.end(); ++giftIt)
			{
				if (giftIt->id == req.id())
				{
					giftPackage = *giftIt;
					
					com_protocol::FFChessGiftPackage* ffChessGiftPackage = rsp.mutable_gift_package();
					setFFChessGiftPackage(0, NULL, *giftIt, ffChessGiftPackage);
					if (ffChessGiftPackage->gift_chess_board_size() > 0) ffChessGiftPackage->mutable_gift_chess_board(0)->set_status(com_protocol::EChessItemStatus::EAlreadyBuyChess);
					if (ffChessGiftPackage->gift_chess_pieces_size() > 0) ffChessGiftPackage->mutable_gift_chess_pieces(0)->set_status(com_protocol::EChessItemStatus::EAlreadyBuyChess);

					return true;
				}
			}
			
			break;
		}
	}
	
	return false;
}

void CLogicHandlerTwo::exchangeFFChessGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ExchangeFFChessItemReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "exchange ff chess goods request")) return;
	
	com_protocol::ExchangeFFChessItemRsp rsp;
	rsp.set_result(ServiceSuccess);
	rsp.set_type(req.type());
	MallConfigData::chess_gift_package ffChessGoods;
	do 
	{
		// 找到兑换信息
		if (!getFFChessGoodsInfo(req, ffChessGoods, rsp))
		{
			OptErrorLog("find the ff chess goods error, user = %s, type = %u, id = %u, day = %u", m_msgHandler->getContext().userData, req.type(), req.id(), req.day());
			rsp.set_result(ServiceItemNotExist);
			break;
		}

		// 获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_msgHandler->getUserBaseinfo(string(m_msgHandler->getContext().userData), user_baseinfo))
		{
			OptErrorLog("exchange ff chess goods, can not find the user = %s, type = %u, id = %u, day = %u", m_msgHandler->getContext().userData, req.type(), req.id(), req.day());
			rsp.set_result(ServiceGetUserinfoFailed); // 获取DB中的信息失败
			break;
		}

		// 判断渔币是否足够
		if (ffChessGoods.buy_price > user_baseinfo.dynamic_info.rmb_gold)
		{
			OptErrorLog("exchange ff chess goods, the fish coin not enought, user = %s, amount = %u, type = %u, id = %u, day = %u, need = %u",
			m_msgHandler->getContext().userData, user_baseinfo.dynamic_info.rmb_gold, req.type(), req.id(), req.day(), ffChessGoods.buy_price);
			rsp.set_result(ServiceFishCoinNotEnought);
			break;
		}
		
		// 道具值
		uint32_t* type2value[] =
		{
			NULL,                                               //保留
			NULL,                                               //金币
			NULL,                                               //渔币
			NULL,                                               //话费卡
			&user_baseinfo.prop_info.suona_count,				//小喇叭
			&user_baseinfo.prop_info.light_cannon_count,		//激光炮
			&user_baseinfo.prop_info.flower_count,				//鲜花
			&user_baseinfo.prop_info.mute_bullet_count,			//哑弹
			&user_baseinfo.prop_info.slipper_count,			    //拖鞋
			NULL,	                                            //奖券
			&user_baseinfo.prop_info.auto_bullet_count,		    //自动炮子弹
			&user_baseinfo.prop_info.lock_bullet_count,		    //锁定炮子弹
			&user_baseinfo.dynamic_info.diamonds_number,        //钻石
			NULL,												//话费额度
			NULL,												//积分
			&user_baseinfo.prop_info.rampage_count,				//狂暴
			&user_baseinfo.prop_info.dud_shield_count,			//哑弹防护
		};
		
		const char* type2name[] =
		{
			"",                 //保留
			"金币",             //金币
			"渔币",             //渔币
			"话费卡",           //话费卡
			"小喇叭",		    //小喇叭
			"激光炮",		    //激光炮
			"鲜花",				//鲜花
			"哑弹",			    //哑弹
			"拖鞋",			    //拖鞋
			"奖券",	            //奖券
			"自动炮",		    //自动炮子弹
			"锁定炮",		    //锁定炮子弹
			"钻石",  		    //钻石
			"话费额度",
			"积分",
			"狂暴",				//狂暴
			"哑弹护盾",         //哑弹防护
		};
	
	    // 开始兑换
		const unsigned int goodsCount = sizeof(type2value) / sizeof(type2value[0]);
		unsigned int getGoodsCount = 1;  // 获得目标物品的数量
		char exchangeInfo[1024] = {0};	// 写兑换记录到DB
		unsigned int exchangeInfoLen = snprintf(exchangeInfo, sizeof(exchangeInfo) - 1, "兑换1个%s", ffChessGoods.name.c_str());
		bool hasOtherItem = false;
		for (map<unsigned int, unsigned int>::const_iterator it = ffChessGoods.gifts.begin(); it != ffChessGoods.gifts.end(); ++it)
		{
			if (it->first < goodsCount)
			{
				uint32_t* value = type2value[it->first];
				if (value == NULL)
				{
					if (it->first != EPropType::PropGold)
					{
						OptErrorLog("exchange ff chess goods, the goods type invalid, user = %s, type = %u, id = %u, gift id = %u",
						m_msgHandler->getContext().userData, req.type(), req.id(), it->first);
						rsp.set_result(ServiceExchangeTypeInvalid);
						break;
					}

					user_baseinfo.dynamic_info.game_gold += it->second; //金币
				}
				else
				{
					*value = *value + it->second;
					hasOtherItem = true;
				}
			
				exchangeInfoLen += snprintf(exchangeInfo + exchangeInfoLen, sizeof(exchangeInfo) - exchangeInfoLen - 1, "-%u个%s", it->second, type2name[it->first]);
				getGoodsCount += it->second;
			}
		}
	
	    if (rsp.result() != ServiceSuccess) break;
		
		// 扣除兑换源货币
		user_baseinfo.dynamic_info.rmb_gold -= ffChessGoods.buy_price;
		
		// 写DB
		// 写动态信息
		if (!m_msgHandler->updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info))
		{
			OptErrorLog("exchange ff chess goods, update dynamic data to db error, user = %s, type = %u, id = %u, day = %u",
			m_msgHandler->getContext().userData, req.type(), req.id(), req.day());
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}
		
		// 写道具信息到DB
		if (hasOtherItem && !m_msgHandler->updateUserPropInfoToMysql(user_baseinfo.prop_info))
		{
			OptErrorLog("exchange ff chess goods, update prop data to db error, user = %s, type = %u, id = %u, day = %u",
			m_msgHandler->getContext().userData, req.type(), req.id(), req.day());
			m_msgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}
		
		// 0：话费卡兑换，1：奖券兑换， 2：渔币兑换物品， 3：积分兑换，4：翻翻棋商城兑换，5：翻翻棋道具商城
		char sql_tmp[2048] = {0};
		snprintf(sql_tmp, sizeof(sql_tmp) - 1, "insert into tb_exchange_record_info(username,insert_time,exchange_type,exchange_count,exchange_goods_info,deal_status,mobilephone_number,address,recipients_name,cs_username,pay_type,get_goods_count) \
										   values(\'%s\',now(),5,1,\'%s\', 2, \'\',\'\',\'\',\'\',%u,%u);",
										   m_msgHandler->getContext().userData, exchangeInfo, EPropType::PropFishCoin, getGoodsCount);
		if (Success != m_msgHandler->m_pDBOpt->modifyTable(sql_tmp))
		{
			m_msgHandler->mysqlRollback();
			OptErrorLog("write ff chess mall exchange record error, user = %s, exeSql = %s", m_msgHandler->getContext().userData, sql_tmp);
			rsp.set_result(ServiceInsertFailed);
			break;
		}

		// 写到memcached
		if (!m_msgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			OptErrorLog("exchange ff chess goods, update user data to memory error, user = %s, type = %u, id = %u, day = %u",
			m_msgHandler->getContext().userData, req.type(), req.id(), req.day());
			
			m_msgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			break;
		}
		
		m_msgHandler->updateOptStatus(m_msgHandler->getContext().userData, CSrvMsgHandler::UpdateOpt::Modify); // 有修改，更新状态
		m_msgHandler->mysqlCommit(); // 提交
		
		// 翻翻棋道具信息存储
		m_msgHandler->getLogicDataInstance().exchangeFFChessGoods(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen, req.day(), rsp);
		
		OptInfoLog("exchange ff chess goods, user = %s, goods type = %u, id = %u, day = %u, need coin = %u, remain = %u",
		m_msgHandler->getContext().userData, req.type(), req.id(), req.day(), ffChessGoods.buy_price, user_baseinfo.dynamic_info.rmb_gold);

	} while (0);

	if (rsp.result() != ServiceSuccess)
	{
		rsp.clear_type();
		rsp.clear_ff_chess_item();
		rsp.clear_gift_package();
	}
	
	char buffer[MaxMsgLen] = { 0 };
	unsigned int msgLen = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, buffer, msgLen, "exchange ff chess goods reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, msgLen, srcSrvId, CommonSrvBusiness::BUSINESS_EXCHANGE_FF_CHESS_GOODS_RSP);
	
	ExchangeLog("exchange ff chess goods|%s|%d|%u|%u|%u|%u", m_msgHandler->getContext().userData, rsp.result(), req.type(), req.id(), req.day(), ffChessGoods.buy_price);
}

void CLogicHandlerTwo::useFFChessGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SelectUseFFChessItemReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "use ff chess goods request")) return;
	
	com_protocol::SelectUseFFChessItemRsp rsp;
	rsp.set_result(ServicePropTypeInvalid);
	rsp.set_type(req.type());
	
	com_protocol::FFChessGoodsData* ffChessData = m_msgHandler->getLogicDataInstance().setLogicData(m_msgHandler->getContext().userData, m_msgHandler->getContext().userDataLen).logicData.mutable_ff_chess_goods();
	if (req.type() == com_protocol::FFChessExchangeType::EFFChessBoard)
	{
		for (int idx = 0; idx < ffChessData->chess_board_size(); ++idx)
		{
			if (ffChessData->chess_board(idx).id() == req.id())
			{
				rsp.set_result(ServiceSuccess);
				rsp.set_id(req.id());
				
				ffChessData->set_current_board_id(req.id());
				
				break;
			}
		}
	}
	else if (req.type() == com_protocol::FFChessExchangeType::EFFChessPieces)
	{
		for (int idx = 0; idx < ffChessData->chess_pieces_size(); ++idx)
		{
			if (ffChessData->chess_pieces(idx).id() == req.id())
			{
				rsp.set_result(ServiceSuccess);
				rsp.set_id(req.id());
				
				ffChessData->set_current_pieces_id(req.id());
				
				break;
			}
		}
	}
	
	char buffer[MaxMsgLen] = { 0 };
	unsigned int msgLen = MaxMsgLen;
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, buffer, msgLen, "use ff chess goods reply");
	if (msgData != NULL) m_msgHandler->sendMessage(msgData, msgLen, srcSrvId, CommonSrvBusiness::BUSINESS_USE_FF_CHESS_GOODS_RSP);
}

void CLogicHandlerTwo::addActivityRedPacket(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::AddActivityRedPacketReq req;
	if (!m_msgHandler->parseMsgFromBuffer(req, data, len, "add activity red packet request")) return;

    const com_protocol::WinRedPacketActivityRecord& wrpaRecord = req.record_info();
	const char* redPacketId = wrpaRecord.red_packet_id().c_str();
	unsigned int giftCount = wrpaRecord.gift().num();
		
	com_protocol::AddActivityRedPacketRsp rsp;
	rsp.set_result(ServiceSuccess);
	do 
	{
		// 获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_msgHandler->getUserBaseinfo(req.record_info().receive_username(), user_baseinfo))
		{
			OptErrorLog("add activity red packet, can not find the user = %s, activity id = %u, red packet id = %s, receive time = %u",
			req.record_info().receive_username().c_str(), req.record_info().activity_id(), req.record_info().red_packet_id().c_str(), req.record_info().receive_time());
			
			rsp.set_result(ServiceGetUserinfoFailed); // 获取DB中的信息失败
			break;
		}

		// 判断需要支付的物品数量是否足够
		google::protobuf::RepeatedPtrField<com_protocol::ItemChangeInfo> goodsList;  // 支付&红包物品
		com_protocol::BuyuGameRecordStorageExt recordData;                           // 游戏记录
		if (req.record_info().has_payment_goods() && req.record_info().payment_goods().num() > 0)
		{
			const unsigned int paymentCount = req.record_info().payment_goods().num();
			const unsigned int* currentGoodsCount = NULL;
			switch (req.record_info().payment_goods().type())
			{
				case EPropType::PropGold://金币
				{
					if (paymentCount > user_baseinfo.dynamic_info.game_gold) rsp.set_result(WinRedPacketPayCountInsufficient);
					break;
				}
				
				case EPropType::PropFishCoin://渔币
				{
					currentGoodsCount = &user_baseinfo.dynamic_info.rmb_gold;
					break;
				}
				
				case EPropType::PropDiamonds://钻石
				{
					currentGoodsCount = &user_baseinfo.dynamic_info.diamonds_number;
					break;
				}
				
				case EPropType::PropPhoneFareValue://话费额度
				{
					currentGoodsCount = &user_baseinfo.dynamic_info.phone_fare;
					break;
				}
				
				case EPropType::PropScores://积分
				{
					currentGoodsCount = &user_baseinfo.dynamic_info.score;
					break;
				}
				
				default:
				{
					rsp.set_result(WinRedPacketPaymentTypeError);
					break;
				}
			}
			
			if (currentGoodsCount != NULL && paymentCount > *currentGoodsCount) rsp.set_result(WinRedPacketPayCountInsufficient);
			
			if (rsp.result() != ServiceSuccess) break;
			
			// 抢红包需要支付的物品
			com_protocol::ItemChangeInfo* itemInfo = goodsList.Add();
			itemInfo->set_type(req.record_info().payment_goods().type());
			itemInfo->set_count(-paymentCount);
			
			com_protocol::ItemRecordStorage* itemRecord = recordData.add_items();
			itemRecord->set_item_type(itemInfo->type());
			itemRecord->set_charge_count(-paymentCount);
		}
		
		// 红包活动信息入库
		char sqlBuffer[2048] = {0};
		unsigned int sqlLen = 0;
		CQueryResult* p_result = NULL;
		if (req.has_activity_info())
		{
			// 先查询该活动信息是否已经存在了，高效查询语句值检查是否存在
			bool isExistActivity = false;
			sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "select 1 from tb_win_red_packet_activity where activity_id=%u limit 1;", req.activity_info().activity_id());
			if (Success != m_msgHandler->m_pDBOpt->queryTableAllResult(sqlBuffer, sqlLen, p_result))
			{
				OptErrorLog("check red packet activity error, user = %s, exeSql = %s", req.record_info().receive_username().c_str(), sqlBuffer);
				rsp.set_result(CheckWinRedPacketActivityError);
				break;
			}
			
			if (p_result != NULL && p_result->getRowCount() > 0) isExistActivity = true;
			m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
			
			// 还不存在活动信息则新加入
			if (!isExistActivity)
			{
				const com_protocol::RedPacketActivityInfo& rpaInfo = req.activity_info();
				sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "insert into tb_win_red_packet_activity(create_time,activity_id,activity_name,red_packet_name,blessing_words,red_packet_amount,pay_type,pay_count,best_username) \
								  values(now(),%u,\'%s\',\'%s\',\'%s\',%u,%u,%u,\'\');",
								  rpaInfo.activity_id(), rpaInfo.activity_name().c_str(), rpaInfo.red_packet_name().c_str(), rpaInfo.blessing_words().c_str(),
								  rpaInfo.red_packet_amount(), rpaInfo.payment_goods().type(), rpaInfo.payment_goods().num());
				if (Success != m_msgHandler->m_pDBOpt->executeSQL(sqlBuffer, sqlLen))
				{
					m_msgHandler->mysqlRollback();
					OptErrorLog("write red packet activity error, user = %s, exeSql = %s", req.record_info().receive_username().c_str(), sqlBuffer);
					
					rsp.set_result(ServiceInsertFailed);
					break;
				}
			}
		}
		
		// 先查询该红包信息是否已经存在了，高效查询语句值检查是否存在
		p_result = NULL;
		sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "select 1 from tb_win_red_packet_record where red_packet_id=\'%s\' limit 1;", wrpaRecord.red_packet_id().c_str());
		if (Success != m_msgHandler->m_pDBOpt->queryTableAllResult(sqlBuffer, sqlLen, p_result))
		{
			OptErrorLog("check red packet data error, user = %s, exeSql = %s", req.record_info().receive_username().c_str(), sqlBuffer);
			rsp.set_result(CheckRedPacketDataError);
			break;
		}
		
		if (p_result != NULL && p_result->getRowCount() > 0)
		{
			// 手慢红包被抢完了
			redPacketId = wrpaRecord.red_packet_id_ex().c_str();
			giftCount = 0;
		}
		m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
			
		// 玩家抢红包记录入库
		sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "insert into tb_win_red_packet_record(create_time,activity_id,red_packet_id,gift_type,gift_count,receive_username,receive_time) \
						  values(now(),%u,\'%s\',%u,%u,\'%s\',%u);",
						  wrpaRecord.activity_id(), redPacketId, wrpaRecord.gift().type(), giftCount, wrpaRecord.receive_username().c_str(), wrpaRecord.receive_time());
		if (Success != m_msgHandler->m_pDBOpt->executeSQL(sqlBuffer, sqlLen))
		{
			m_msgHandler->mysqlRollback();
			OptErrorLog("write user win red packet activity record error, user = %s, exeSql = %s", req.record_info().receive_username().c_str(), sqlBuffer);
			
			rsp.set_result(ServiceInsertFailed);
			break;
		}
		
		// 手气最佳玩家
		if (req.record_info().is_best() == 1)
		{
			sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "update tb_win_red_packet_activity set best_username='\%s\' where activity_id=%u;",
							  wrpaRecord.receive_username().c_str(), wrpaRecord.activity_id());
			if (Success != m_msgHandler->m_pDBOpt->executeSQL(sqlBuffer, sqlLen))
			{
				m_msgHandler->mysqlRollback();
				OptErrorLog("update red packet activity best user error, user = %s, exeSql = %s", req.record_info().receive_username().c_str(), sqlBuffer);
				
				rsp.set_result(ServiceUpdateDataToMysqlFailed);
				break;
			}
		}

		// 领取红包获得的物品
		if (giftCount > 0)
		{
			com_protocol::ItemChangeInfo* itemInfo = goodsList.Add();
			itemInfo->set_type(req.record_info().gift().type());
			itemInfo->set_count(giftCount);
			
			com_protocol::ItemRecordStorage* itemRecord = recordData.add_items();
			itemRecord->set_item_type(itemInfo->type());
			itemRecord->set_charge_count(itemInfo->count());
		}

		if (goodsList.size() > 0)
		{
			// 写游戏记录
			recordData.set_room_rate(0);
			recordData.set_room_name("大厅");
			recordData.set_remark(string("抢红包活动-") + wrpaRecord.red_packet_id());
            int result = m_msgHandler->changeUserPropertyValue(wrpaRecord.receive_username().c_str(), wrpaRecord.receive_username().length(), goodsList, recordData);  // 金币&道具&玩家属性等数量变更修改
			rsp.set_result(result);
		}
		
		if (rsp.result() == ServiceSuccess)
		{
			m_msgHandler->mysqlCommit(); // 提交
			
		    if (giftCount == 0)
			{
				rsp.set_result(1);  // 这里特殊处理，设置值1表示手慢红包被抢完了，但正常扣掉了用户参与红包需要支付的物品
			}
			else
			{
			    com_protocol::WinRedPacketActivityRecord* mutableWrpaRecord = req.mutable_record_info();
			    mutableWrpaRecord->set_receive_nickname(user_baseinfo.static_info.nickname);
			    mutableWrpaRecord->set_portrait_id(user_baseinfo.static_info.portrait_id);
			}
		}
		else
		{
			m_msgHandler->mysqlRollback();  // 失败则回滚DB数据
		}
		
	} while (0);
	
	int rc = ServiceFailed;
	char buffer[MaxMsgLen] = { 0 };
	unsigned int msgLen = MaxMsgLen;
	rsp.set_allocated_red_packet_info(&req);	
	const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, buffer, msgLen, "add activity red packet reply");
	if (msgData != NULL) rc = m_msgHandler->sendMessage(msgData, msgLen, srcSrvId, CommonSrvBusiness::BUSINESS_ADD_ACTIVITY_RED_PACKET_RSP);
	
	OptInfoLog("add activity red packet, user = %s, activity id = %u, red packet id = %s, gift count = %u, result = %d, rc = %d",
	           wrpaRecord.receive_username().c_str(), wrpaRecord.activity_id(), redPacketId, giftCount, rsp.result(), rc);
	
	rsp.release_red_packet_info();
}

void CLogicHandlerTwo::getActivityRedPacket(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	char sql_tmp[1024 * 512] = {0};
	unsigned int sqlLen = snprintf(sql_tmp, sizeof(sql_tmp) - 1, "select activity_id,activity_name,red_packet_name,blessing_words,red_packet_amount,best_username from tb_win_red_packet_activity;");
	CQueryResult* p_result = NULL;
	int rc = m_msgHandler->m_pDBOpt->queryTableAllResult(sql_tmp, sqlLen, p_result);
	if (Success == rc)
	{
		com_protocol::GetActivityRedPacketDataRsp rsp;
		RowDataPtr rowData = NULL;
		while ((rowData = p_result->getNextRow()) != NULL)
		{
			com_protocol::RedPacketActivityInfo* activityInfo = rsp.add_activitys();
			activityInfo->set_activity_id(atoi(rowData[0]));
			activityInfo->set_activity_name(rowData[1]);
			activityInfo->set_red_packet_name(rowData[2]);
			activityInfo->set_blessing_words(rowData[3]);
			activityInfo->set_red_packet_amount(atoi(rowData[4]));
			activityInfo->set_best_username(rowData[5]);
		}
		m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
		
		if (rsp.activitys_size() > 0)
		{
			p_result = NULL;
			sqlLen = snprintf(sql_tmp, sizeof(sql_tmp) - 1, "select activity_id,red_packet_id,gift_type,gift_count,receive_username,receive_time from tb_win_red_packet_record where gift_count > 0;");
			rc = m_msgHandler->m_pDBOpt->queryTableAllResult(sql_tmp, sqlLen, p_result);
	        if (Success == rc)
			{
				typedef unordered_map<string, int> WinRedPacketUsernames;
				
				WinRedPacketUsernames winRedPacketUsernames;
				rowData = NULL;
				while ((rowData = p_result->getNextRow()) != NULL)
				{
					com_protocol::WinRedPacketActivityRecord* winRedPacketRecord = rsp.add_win_records();
					winRedPacketRecord->set_activity_id(atoi(rowData[0]));
					winRedPacketRecord->set_red_packet_id(rowData[1]);
					winRedPacketRecord->set_receive_username(rowData[4]);
					winRedPacketRecord->set_receive_time(atoi(rowData[5]));
					com_protocol::GiftInfo* gift = winRedPacketRecord->mutable_gift();
					gift->set_type(atoi(rowData[2]));
					gift->set_num(atoi(rowData[3]));
					
					winRedPacketUsernames[rowData[4]] = 0;
				}
				m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
				
				if (!winRedPacketUsernames.empty())
				{
					sqlLen = snprintf(sql_tmp, sizeof(sql_tmp) - 1, "select username,nickname,portrait_id from tb_user_static_baseinfo where username='1'");
					for (WinRedPacketUsernames::const_iterator it = winRedPacketUsernames.begin(); it != winRedPacketUsernames.end(); ++it)
					{
						sqlLen += snprintf(sql_tmp + sqlLen, sizeof(sql_tmp) - sqlLen - 1, " or username=\'%s\'", it->first.c_str());
					}
					
					if (sqlLen < sizeof(sql_tmp))
					{
						sql_tmp[sqlLen++] = ';';
					}
					
					p_result = NULL;
					rc = m_msgHandler->m_pDBOpt->queryTableAllResult(sql_tmp, sqlLen, p_result);
					if (Success == rc)
					{
						rowData = NULL;
						while ((rowData = p_result->getNextRow()) != NULL)
						{
							com_protocol::WinRedPacketUserInfo* winRedPacketUserInfo = rsp.add_user_infos();
							winRedPacketUserInfo->set_username(rowData[0]);
							winRedPacketUserInfo->set_nickname(rowData[1]);
							winRedPacketUserInfo->set_portrait_id(atoi(rowData[2]));
						}
						m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
					}
					else
					{
						OptErrorLog("get win red packet user info error, rc = %d, sql = %s", rc, sql_tmp);
						return;
					}
				}
				
				char buffer[MaxMsgLen] = { 0 };
				unsigned int msgLen = MaxMsgLen;
				const char* msgData = m_msgHandler->serializeMsgToBuffer(rsp, buffer, msgLen, "get activity red packet data reply");
				if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_GET_ACTIVITY_RED_PACKET_RSP;
				if (msgData != NULL) m_msgHandler->sendMessage(msgData, msgLen, srcSrvId, srcProtocolId);
			}
			else
			{
				OptErrorLog("get user win red packet record error, rc = %d, sql = %s", rc, sql_tmp);
			}
		}
	}
	else
	{
		OptErrorLog("get red packet activity data error, rc = %d, sql = %s", rc, sql_tmp);
	}
}

// 清空机器人积分券
void CLogicHandlerTwo::clearRobotScore()
{
	if (m_msgHandler == NULL) return;
	
	// 配置时间
	int year = m_msgHandler->m_config.common_cfg.clear_robot_score_date / 10000;
	int month = (m_msgHandler->m_config.common_cfg.clear_robot_score_date % 10000) / 100;
	int day = m_msgHandler->m_config.common_cfg.clear_robot_score_date % 100;
	
	// 当前时间
	const time_t curSecs = time(NULL);
	struct tm curTime;
	localtime_r(&curSecs, &curTime);

	if ((curTime.tm_year + 1900) == year && (curTime.tm_mon + 1) == month && curTime.tm_mday == day)  // 日期匹配则清空机器人积分券
	{
		char sql_tmp[2048] = {0};
		unsigned int sqlLen = snprintf(sql_tmp, sizeof(sql_tmp) - 1, "select username from tb_user_platform_map_info where platform_type=%u or platform_type=%u;",
									   ERobotType::ERobotPlatformType, ERobotType::EVIPRobotPlatformType);
		CQueryResult* p_result = NULL;			
		if (Success == m_msgHandler->m_pDBOpt->queryTableAllResult(sql_tmp, sqlLen, p_result))
		{
			com_protocol::ResetUserOtherInfoRsp setRsp;
			CUserBaseinfo user_baseinfo;
			
			// 重置积分券为0
			com_protocol::ResetUserOtherInfoReq resetUserInfo;
			com_protocol::UserOtherInfo* userInfo = resetUserInfo.add_other_info();
			userInfo->set_info_flag(EPropType::PropScores);
			userInfo->set_int_value(0);
			
			unsigned int count = 0;
			RowDataPtr rowData = NULL;
			while ((rowData = p_result->getNextRow()) != NULL)
			{
				resetUserInfo.set_query_username(rowData[0]);
				m_msgHandler->resetUserBaseInfo(resetUserInfo, setRsp, user_baseinfo);
				
				if (setRsp.result() == 0) ++count;
			}
			m_msgHandler->m_pDBOpt->releaseQueryResult(p_result);
			
			OptInfoLog("clear robot score count = %u", count);
		}
		else
		{
			OptErrorLog("clear robot score exe sql error = %s", sql_tmp);
		}
	}
}

