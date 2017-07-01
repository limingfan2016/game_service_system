
/* author : liuxu
* date : 2015.03.10
* description : 消息处理辅助类
*/
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "MessageDefine.h"
#include "CSrvMsgHandler.h"
#include "SrvFrame/CModule.h"
#include "CLogic.h"
#include "base/Function.h"
#include "db/CMySql.h"
#include "base/CMD5.h"
#include "CGameRecord.h"
#include "../common/CommonType.h"


using namespace NProject;

// 数据记录日志文件
#define DataRecordLog(format, args...)     getDataLogger().writeFile(NULL, 0, LogLevel::Info, format, ##args)


CLogic::CLogic()
{
	m_psrvMsgHandler = NULL;
	m_firstRechargeItemId = 0;
}

CLogic::~CLogic()
{
	m_psrvMsgHandler = NULL;
	m_firstRechargeItemId = 0;
}

int CLogic::init(CSrvMsgHandler *psrvMsgHandle)
{
	m_psrvMsgHandler = psrvMsgHandle;
	
	// 日志配置检查
	const unsigned int MinRechargeLogFileSize = 1024 * 1024 * 8;
    const unsigned int MinRechargeLogFileCount = 20;
	const char* rechargeLoggerCfg[] = {"LogFileName", "LogFileSize", "LogFileCount",};
	for (unsigned int j = 0; j < (sizeof(rechargeLoggerCfg) / sizeof(rechargeLoggerCfg[0])); ++j)
	{
		const char* value = CCfg::getValue("Recharge", rechargeLoggerCfg[j]);
		if (value == NULL)
		{
			ReleaseErrorLog("recharge log config item error, can not find item = %s", rechargeLoggerCfg[j]);
			return -1;
		}
		
		if ((j == 1 && (unsigned int)atoi(value) < MinRechargeLogFileSize)
			|| (j == 2 && (unsigned int)atoi(value) < MinRechargeLogFileCount))
		{
			ReleaseErrorLog("recharge log config item error, file min size = %d, count = %d", MinRechargeLogFileSize, MinRechargeLogFileCount);
			return -1;
		}
	}
	
	m_psrvMsgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_GET_MALL_CONFIG_REQ, (ProtocolHandler)&CLogic::getMallConfigReq, this);
	m_psrvMsgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_QUERY_USER_NAMES_REQ, (ProtocolHandler)&CLogic::queryUserNameReq, this);
	m_psrvMsgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_QUERY_USER_INFO_REQ, (ProtocolHandler)&CLogic::queryUserInfoReq, this);

	//充值
	m_psrvMsgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_USER_RECHARGE_REQ, (ProtocolHandler)&CLogic::handldMsgUserRechargeReq,this);
	
	m_psrvMsgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_EXCHANGE_PHONE_CARD_REQ, (ProtocolHandler)&CLogic::handldMsgExchangePhoneCardReq, this);
	
	m_psrvMsgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_RESET_PASSWORD_REQ, (ProtocolHandler)&CLogic::resetPasswordReq, this);
	m_psrvMsgHandler->registerProtocol(ServiceType::CommonSrv, CommonSrvBusiness::BUSINESS_MODIFY_EXCHANGE_GOODS_STATUS_REQ, (ProtocolHandler)&CLogic::handldMsgModifyExchangeGoodsStatusReq, this);

	return 0;
}

void CLogic::updateConfig()
{
	const MallConfigData::MallData& mallData = MallConfigData::MallData::getConfigValue();
	if (mallData.isSetConfigValueSuccess())
	{
		//配置更新成功了再清空
		m_get_mall_config_rsp.Clear();
		m_mitem_info.clear();
		m_mechange_info.clear();

		com_protocol::GetMallConfigRsp &getMallConfigRsp = m_get_mall_config_rsp;
	
	    /*
		// 金币列表
		for (unsigned int i = 0; i < mallData.gold_list.size(); ++i)
		{
			const MallConfigData::gold& gold = mallData.gold_list[i];
			if (!gold.is_used)
			{
				continue;
			}

			com_protocol::ItemInfo* item = getMallConfigRsp.add_gold_list()->mutable_gold();
			item->set_id(gold.id);
			item->set_num(gold.num);
			item->set_buy_price(gold.buy_price);
			
			item->set_flag(gold.flag);
			if (gold.flag >= MallItemFlag::FirstNoFlag)  // 首次充值的物品标识
			{
				m_firstRechargeItemId = gold.id;
				item->set_flag(MallItemFlag::First);
			}
			
			for (unsigned int giftIdx = 0; giftIdx < gold.gift_array.size(); ++giftIdx)
			{
				com_protocol::GiftInfo* gift = item->add_gift_array();
				gift->set_type(gold.gift_array[giftIdx].type);
				gift->set_num(gold.gift_array[giftIdx].num);
			}

			SItemInfo ii;
			ii.item_num = gold.num;
			ii.buy_price = gold.buy_price;
			ii.giftVector = (PayGiftVector*)&(gold.gift_array);
			m_mitem_info.insert(make_pair(gold.id, ii));
		}
		
		// 道具列表
		for (unsigned int i = 0; i < mallData.item_list.size(); ++i)
		{
			if (!mallData.item_list[i].is_used)
			{
				continue;
			}

			com_protocol::ItemInfo* item = getMallConfigRsp.add_item_list()->mutable_item();
			item->set_id(mallData.item_list[i].id);
			item->set_num(mallData.item_list[i].num);
			item->set_buy_price(mallData.item_list[i].buy_price);

			SItemInfo ii;
			ii.item_num = mallData.item_list[i].num;
			ii.buy_price = mallData.item_list[i].buy_price;
			m_mitem_info.insert(make_pair(mallData.item_list[i].id, ii));
		}
		
		// 话费卡列表
		m_exchangeCount2Get.clear();
		for (unsigned int i = 0; i < mallData.phone_card_list.size(); ++i)
		{
			if (!mallData.phone_card_list[i].is_used)
			{
				continue;
			}

			com_protocol::ExchangePhoneCardInfo* phone_card = getMallConfigRsp.add_phone_card_list()->mutable_phone_card();
			phone_card->set_id(mallData.phone_card_list[i].id);
			phone_card->set_exchange_count(mallData.phone_card_list[i].exchange_count);
			phone_card->set_exchange_get(mallData.phone_card_list[i].exchange_get);

			SExchangeInfo exchange_info;
			bzero(&exchange_info, sizeof(exchange_info));
			exchange_info.exchange_count = mallData.phone_card_list[i].exchange_count;
			exchange_info.exchange_get = mallData.phone_card_list[i].exchange_get;
			strncpy(exchange_info.exchange_desc, mallData.phone_card_list[i].exchange_desc.c_str(), sizeof(exchange_info.exchange_desc) - 1);
			m_mechange_info.insert(make_pair(mallData.phone_card_list[i].id, exchange_info));
			
			m_exchangeCount2Get[exchange_info.exchange_count] = exchange_info.exchange_get;
		}
		*/
		
		
		// 话费额度兑换列表
		for (unsigned int i = 0; i < mallData.phone_fare_list.size(); ++i)
		{
			const MallConfigData::PhoneFare& phoneFareCfg = mallData.phone_fare_list[i];
			if (!phoneFareCfg.is_used) continue;
			
			m_exchangeCount2Get[phoneFareCfg.expense] = phoneFareCfg.achieve;
		}

		// 兑换实物列表
		for (unsigned int i = 0; i < mallData.goods_list.size(); ++i)
		{
			if (!mallData.goods_list[i].is_used)
			{
				continue;
			}

			com_protocol::ExchangeGoodsInfo* goods = getMallConfigRsp.add_goods_list()->mutable_goods();
			goods->set_id(mallData.goods_list[i].id);
			goods->set_exchange_count(mallData.goods_list[i].exchange_count);
			goods->set_exchange_desc(mallData.goods_list[i].exchange_desc);

			SExchangeInfo exchange_info;
			bzero(&exchange_info, sizeof(exchange_info));
			exchange_info.exchange_count = mallData.goods_list[i].exchange_count;
			exchange_info.exchange_get = 1;
			strncpy(exchange_info.exchange_desc, mallData.goods_list[i].exchange_desc.c_str(), sizeof(exchange_info.exchange_desc) - 1);
			m_mechange_info.insert(make_pair(mallData.goods_list[i].id, exchange_info));
		}
	}
}

int CLogic::procRegisterUserReq(const com_protocol::RegisterUserReq &req, const uint32_t platform_type)
{
	string sql("");
	int rel = getRegisterSql(req, sql);
	if (0 != rel)
	{
		return rel;
	}

	//插入用户静态表数据
	if (Success == m_psrvMsgHandler->m_pDBOpt->modifyTable(sql.c_str(), sql.length()))
	{
		//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql.c_str());
	}
	else
	{
		OptErrorLog("exeSql failed|%s|%s", req.username().c_str(), sql.c_str());
		return ServiceUsernameExist;
	}

	//获取刚插入用户静态表的自增ID
	char sql_tmp[2048] = {0};
	CQueryResult *p_result = NULL;
	snprintf(sql_tmp, sizeof(sql_tmp), "select id from tb_user_static_baseinfo where username=\'%s\';", req.username().c_str());
	if (Success != m_psrvMsgHandler->m_pDBOpt->queryTableAllResult(sql_tmp, p_result)
		|| 1 != p_result->getRowCount())
	{
		m_psrvMsgHandler->m_pDBOpt->releaseQueryResult(p_result);
		m_psrvMsgHandler->mysqlRollback();
		return ServiceGetIDFailed;
	}
	unsigned int id = atoi(p_result->getNextRow()[0]);
	m_psrvMsgHandler->m_pDBOpt->releaseQueryResult(p_result);

	// 各个平台通用初始化值 
	const DbproxyCommonConfig::RegisterInit& register_init_value = m_psrvMsgHandler->m_config.register_init;
	uint32_t type2value[] =
	{
		0,                                              //保留
		register_init_value.game_gold,                  //金币
		0,                                              //渔币
		0,                                              //话费卡
		register_init_value.suona_count,				//小喇叭
		register_init_value.light_cannon_count,		    //激光炮
		register_init_value.flower_count,				//鲜花
		register_init_value.mute_bullet_count,			//哑弹
		register_init_value.slipper_count,			    //拖鞋
		0,	                                            //奖券
		register_init_value.auto_bullet_count,		    //自动炮子弹
		register_init_value.lock_bullet_count,		    //锁定炮子弹
		0,                                              //钻石
		0,                                              //话费额度
		0,					                            //积分
		register_init_value.rampage_count,				//狂暴
		register_init_value.dud_shield_count,		    //哑弹防护
	};

	if (platform_type == ThirdPlatformType::AoRong)     // 奥榕平台
	{
		const unsigned int typeCount = sizeof(type2value) / sizeof(type2value[0]);
		for (map<unsigned int, unsigned int>::const_iterator aorongIt = m_psrvMsgHandler->m_config.aorong_platform_init.begin(); aorongIt != m_psrvMsgHandler->m_config.aorong_platform_init.end(); ++aorongIt)
		{
			if (aorongIt->first < typeCount) type2value[aorongIt->first] = aorongIt->second;
		}
	}
	
	//插入用户动态表信息
	snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_user_dynamic_baseinfo(id, username, game_gold) value(%u, \'%s\', %u);",
		id, req.username().c_str(), type2value[EPropType::PropGold]);
	if (Success == m_psrvMsgHandler->m_pDBOpt->modifyTable(sql_tmp))
	{
		//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql_tmp);
	}
	else
	{
		m_psrvMsgHandler->mysqlRollback();
		OptErrorLog("exeSql failed|%s|%s", req.username().c_str(), sql_tmp);
		return ServiceInsertFailed;
	}

	//插入用户道具表信息
	snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_user_prop_info(id, username, light_cannon_count, mute_bullet_count, flower_count, slipper_count, suona_count, auto_bullet_count, lock_bullet_count, rampage_count, dud_shield_count) \
		value(%u, \'%s\', %u, %u, %u, %u, %u, %u, %u, %u, %u);", id, req.username().c_str(), type2value[EPropType::PropLightCannon], type2value[EPropType::PropMuteBullet],
		type2value[EPropType::PropFlower], type2value[EPropType::PropSlipper], type2value[EPropType::PropSuona], type2value[EPropType::PropAutoBullet],
		type2value[EPropType::PropLockBullet], type2value[EPropType::PropRampage], type2value[EPropType::PropDudShield]);
	if (Success == m_psrvMsgHandler->m_pDBOpt->modifyTable(sql_tmp))
	{
		//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql_tmp);
	}
	else
	{
		m_psrvMsgHandler->mysqlRollback();
		OptErrorLog("exeSql failed|%s|%s", req.username().c_str(), sql_tmp);
		return ServiceInsertFailed;
	}

	// m_psrvMsgHandler->mysqlCommit();

	return ServiceSuccess;
}

int CLogic::procModifyPasswordReq(const com_protocol::ModifyPasswordReq &req, const string &username)
{
	CMD5 md5;
	string new_passwd;

	//新密码不是MD5格式
	if (!(req.new_passwd().length() == 32 && strIsUpperHex(req.new_passwd().c_str())))
	{
		return ServicePasswdInputUnlegal;
	}

	//获取DB中的用户信息
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(username, user_baseinfo))
	{
		return ServiceGetUserinfoFailed;//获取DB中的信息失败
	}

	if (!req.is_reset())
	{
		//输入的旧密码不是MD5格式
		if (!(req.old_passwd().length() == 32 && strIsUpperHex(req.old_passwd().c_str())))
		{
			return ServicePasswdInputUnlegal;
		}

		//计算出输入的旧密码
		char tmp_str[128] = { 0 };
		memcpy(tmp_str, username.c_str(), username.length());
		memcpy(tmp_str + username.length(), req.old_passwd().c_str(), req.old_passwd().length());
		md5.GenerateMD5((unsigned char*)tmp_str, username.length() + req.new_passwd().length());

		if (md5.ToString() != user_baseinfo.static_info.password)
		{
			return ServicePasswordVerifyFailed;//输入的旧密码校验失败
		}
		
	}
	
	//修改密码
	char tmp_str[128] = { 0 };
	memcpy(tmp_str, username.c_str(), username.length());
	memcpy(tmp_str + username.length(), req.new_passwd().c_str(), req.new_passwd().length());
	md5.GenerateMD5((unsigned char*)tmp_str, username.length() + req.new_passwd().length());
	new_passwd = md5.ToString();	
	strcpy(user_baseinfo.static_info.password, new_passwd.c_str());

	//将数据写入DB
	if (m_psrvMsgHandler->updateUserStaticBaseinfoToMysql(user_baseinfo.static_info))
	{
		if (m_psrvMsgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			m_psrvMsgHandler->mysqlCommit();
		}
		else
		{
			m_psrvMsgHandler->mysqlRollback();
			return ServiceUpdateDataToMemcachedFailed;
		}
	}
	else
	{
		return ServiceUpdateDataToMysqlFailed;
	}

	return 0;
}

int CLogic::procModifyBaseinfoReq(const com_protocol::ModifyBaseinfoReq &req, const string &username)
{
	//获取DB中的用户信息
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(username, user_baseinfo))
	{
		return ServiceGetUserinfoFailed;//获取DB中的信息失败
	}
	
	//校验输入是否合法
	if (req.has_nickname())
	{
		if (!(req.nickname().length() <= 32))
		{
			return ServiceNicknameInputUnlegal;
		}
		//敏感字符过滤
		if (m_psrvMsgHandler->getSensitiveWordFilter().isExistSensitiveStr((const unsigned char *)req.nickname().c_str(), req.nickname().size()))
		{
			return ServiceNicknameSensitiveStr;
		}

		strcpy(user_baseinfo.static_info.nickname, req.nickname().c_str());
	}
	
	if (req.has_person_sign())
	{
		if (req.person_sign().length() > 64)
		{
			return ServicePersonSignInputUnlegal;
		}
		strcpy(user_baseinfo.static_info.person_sign, req.person_sign().c_str());
	}

	if (req.has_realname())
	{
		if (req.realname().length() > 16)
		{
			return ServiceRealnameInputUnlegal;
		}
		strcpy(user_baseinfo.static_info.realname, req.realname().c_str());
	}
	
	if (req.has_portrait_id())
	{
		user_baseinfo.static_info.portrait_id = req.portrait_id();
	}

	if (req.has_qq_num())
	{
		if (!strIsDigit(req.qq_num().c_str()) || req.qq_num().length() > 16)
		{
			return ServiceQQnumInputUnlegal;
		}
		strcpy(user_baseinfo.static_info.qq_num, req.qq_num().c_str());
	}

	if (req.has_address())
	{
		if (req.address().length() > 64)
		{
			return ServiceAddrInputUnlegal;
		}
		strcpy(user_baseinfo.static_info.address, req.address().c_str());
	}

	if (req.has_birthday())
	{
		if (!strIsDate(req.birthday().c_str()))
		{
			return ServiceBirthdayInputUnlegal;
		}
		strcpy(user_baseinfo.static_info.birthday, req.birthday().c_str());
	}

	if (req.has_sex())
	{
		user_baseinfo.static_info.sex = req.sex();
	}

	if (req.has_home_phone())
	{
		if (req.home_phone().length() > 16 || !strIsAlnum(req.home_phone().c_str()))
		{
			return ServiceHomephoneInputUnlegal;
		}
		strcpy(user_baseinfo.static_info.home_phone, req.home_phone().c_str());
	}

	if (req.has_mobile_phone())
	{
		if (req.mobile_phone().length() > 16 || !strIsAlnum(req.mobile_phone().c_str()))
		{
			return ServiceMobilephoneInputUnlegal;
		}
		strcpy(user_baseinfo.static_info.mobile_phone, req.mobile_phone().c_str());
	}

	if (req.has_email())
	{
		if (req.email().length() > 36)
		{
			return ServiceEmailInputUnlegal;
		}
		strcpy(user_baseinfo.static_info.email, req.email().c_str());
	}

	if (req.has_age())
	{
		user_baseinfo.static_info.age = req.age();
	}

	if (req.has_idc())
	{
		if (req.idc().length() > 24)
		{
			return ServiceIDCInputUnlegal;
		}
		strcpy(user_baseinfo.static_info.idc, req.idc().c_str());
	}

	if (req.has_other_contact())
	{
		if (req.other_contact().length() > 64)
		{
			return ServiceOtherContactUnlegal;
		}
		strcpy(user_baseinfo.static_info.other_contact, req.other_contact().c_str());
	}

	if (req.has_city_id())
	{
		user_baseinfo.static_info.city_id = req.city_id();
	}

	//将数据写入DB
	if (m_psrvMsgHandler->updateUserStaticBaseinfoToMysql(user_baseinfo.static_info))
	{
		if (m_psrvMsgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			m_psrvMsgHandler->mysqlCommit();
		}
		else
		{
			m_psrvMsgHandler->mysqlRollback();
			return ServiceUpdateDataToMemcachedFailed;
		}
	}
	else
	{
		return ServiceUpdateDataToMysqlFailed;
	}

	return ServiceSuccess;
}

void CLogic::procModifyBaseinfoReq(const com_protocol::ModifyBaseinfoReq &req, const string &username, com_protocol::ModifyBaseinfoRsp &rsp)
{
	//获取DB中的用户信息
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(username, user_baseinfo))
	{
		rsp.set_result(ServiceGetUserinfoFailed);
		return ;
	}
	rsp.set_result(ServiceSuccess);

	//校验输入是否合法
	if (req.has_nickname())
	{
		rsp.mutable_base_info_rsp()->set_nickname(user_baseinfo.static_info.nickname);
		if (!(req.nickname().length() <= m_psrvMsgHandler->m_config.common_cfg.nickname_length))
		{
			rsp.set_result(ServiceNicknameInputUnlegal);
		}
		//敏感字符过滤
		else if (m_psrvMsgHandler->getSensitiveWordFilter().isExistSensitiveStr((const unsigned char *)req.nickname().c_str(), req.nickname().size()))
		{
			rsp.set_result(ServiceNicknameSensitiveStr);
		}
		else
		{
			rsp.mutable_base_info_rsp()->set_nickname(req.nickname());
			strcpy(user_baseinfo.static_info.nickname, req.nickname().c_str());
		}
	}

	if (req.has_person_sign())
	{
		rsp.mutable_base_info_rsp()->set_person_sign(user_baseinfo.static_info.person_sign);
		if (req.person_sign().length() > 64)
		{
			rsp.set_result(ServicePersonSignInputUnlegal);
		}
		else
		{
			rsp.mutable_base_info_rsp()->set_person_sign(req.person_sign());
			strcpy(user_baseinfo.static_info.person_sign, req.person_sign().c_str());
		}
	}

	if (req.has_realname())
	{
		rsp.mutable_base_info_rsp()->set_realname(user_baseinfo.static_info.realname);
		if (req.realname().length() > 16)
		{
			rsp.set_result(ServiceRealnameInputUnlegal);
		}
		else
		{
			rsp.mutable_base_info_rsp()->set_realname(req.realname());
			strcpy(user_baseinfo.static_info.realname, req.realname().c_str());
		}
	}

	if (req.has_portrait_id())
	{
		user_baseinfo.static_info.portrait_id = req.portrait_id();
		rsp.mutable_base_info_rsp()->set_portrait_id(req.portrait_id());
	}

	if (req.has_qq_num())
	{
		rsp.mutable_base_info_rsp()->set_qq_num(user_baseinfo.static_info.qq_num);
		if (!strIsDigit(req.qq_num().c_str()) || req.qq_num().length() > 16)
		{
			rsp.set_result(ServiceQQnumInputUnlegal);
		}
		else
		{
			strcpy(user_baseinfo.static_info.qq_num, req.qq_num().c_str());
			rsp.mutable_base_info_rsp()->set_qq_num(req.qq_num());
		}
	}

	if (req.has_address())
	{
		rsp.mutable_base_info_rsp()->set_address(user_baseinfo.static_info.address);
		if (req.address().length() > 64)
		{
			rsp.set_result(ServiceAddrInputUnlegal);
		}
		else
		{
			rsp.mutable_base_info_rsp()->set_address(req.address());
			strcpy(user_baseinfo.static_info.address, req.address().c_str());
		}
	}

	if (req.has_birthday())
	{
		rsp.mutable_base_info_rsp()->set_birthday(user_baseinfo.static_info.birthday);
		if (!strIsDate(req.birthday().c_str()))
		{
			rsp.set_result(ServiceBirthdayInputUnlegal);
		}
		else
		{
			rsp.mutable_base_info_rsp()->set_birthday(req.birthday());
			strcpy(user_baseinfo.static_info.birthday, req.birthday().c_str());
		}		
	}

	if (req.has_sex())
	{
		user_baseinfo.static_info.sex = req.sex();
		rsp.mutable_base_info_rsp()->set_sex(req.sex());
	}

	if (req.has_home_phone())
	{
		rsp.mutable_base_info_rsp()->set_home_phone(user_baseinfo.static_info.home_phone);
		if (req.home_phone().length() > 16 || !strIsAlnum(req.home_phone().c_str()))
		{
			rsp.set_result(ServiceHomephoneInputUnlegal);
		}
		else
		{
			rsp.mutable_base_info_rsp()->set_home_phone(req.home_phone());
			strcpy(user_baseinfo.static_info.home_phone, req.home_phone().c_str());
		}
	}

	if (req.has_mobile_phone())
	{
		rsp.mutable_base_info_rsp()->set_mobile_phone(user_baseinfo.static_info.mobile_phone);

		if (req.mobile_phone().length() > 16 || !strIsAlnum(req.mobile_phone().c_str()))
		{
			rsp.set_result(ServiceMobilephoneInputUnlegal);
		}
		else
		{
			rsp.mutable_base_info_rsp()->set_mobile_phone(req.mobile_phone());
			strcpy(user_baseinfo.static_info.mobile_phone, req.mobile_phone().c_str());
		}
	}

	if (req.has_email())
	{
		rsp.mutable_base_info_rsp()->set_email(user_baseinfo.static_info.email);
		if (req.email().length() > 36)
		{
			rsp.set_result(ServiceEmailInputUnlegal);
		}
		else
		{
			rsp.mutable_base_info_rsp()->set_email(req.email());
			strcpy(user_baseinfo.static_info.email, req.email().c_str());
		}
	}

	if (req.has_age())
	{
		user_baseinfo.static_info.age = req.age();
		rsp.mutable_base_info_rsp()->set_age(req.age());
	}

	if (req.has_idc())
	{
		rsp.mutable_base_info_rsp()->set_idc(user_baseinfo.static_info.idc);
		if (req.idc().length() > 24)
		{
			rsp.set_result(ServiceIDCInputUnlegal);
		}
		else
		{
			strcpy(user_baseinfo.static_info.idc, req.idc().c_str());
			rsp.mutable_base_info_rsp()->set_idc(req.idc());
		}
	}

	if (req.has_other_contact())
	{
		rsp.mutable_base_info_rsp()->set_other_contact(user_baseinfo.static_info.other_contact);
		if (req.other_contact().length() > 64)
		{
			rsp.set_result(ServiceOtherContactUnlegal);
		}
		else
		{
			strcpy(user_baseinfo.static_info.other_contact, req.other_contact().c_str());
			rsp.mutable_base_info_rsp()->set_other_contact(req.other_contact().c_str());
		}
	}

	if (req.has_city_id())
	{
		user_baseinfo.static_info.city_id = req.city_id();
		rsp.mutable_base_info_rsp()->set_city_id(req.city_id());
	}

	//将数据写入DB
	if (m_psrvMsgHandler->updateUserStaticBaseinfoToMysql(user_baseinfo.static_info))
	{
		if (m_psrvMsgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			m_psrvMsgHandler->mysqlCommit();
		}
		else
		{
			m_psrvMsgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			rsp.clear_base_info_rsp();
		}
	}
	else
	{
		rsp.set_result(ServiceUpdateDataToMysqlFailed);
		rsp.clear_base_info_rsp();
	}
}

int CLogic::procVeriyfyPasswordReq(const com_protocol::VerifyPasswordReq &req)
{
	// 获取DB中的用户信息
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(req.username(), user_baseinfo))
	{
		return ServiceGetUserinfoFailed;//获取DB中的信息失败
	}
	
	// 先查看帐号状态是否已经被锁定
	if (0 != user_baseinfo.static_info.status)
	{
		return ServiceUserLimitLogin;
	}
	
	// 检查该账号是否被锁定了
	char sqlBuffer[2048] = { 0 };
	CQueryResult *p_result = NULL;
	unsigned int sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "select username,login_id,login_ip from tb_locked_account where username=\'%s\' or login_ip=\'%s\';",
	req.username().c_str(), req.login_ip().c_str());
	
	// 兼容处理，存在login id
	if (req.login_id().length() > 0)
	{
		--sqlLen;  // 去掉语句最后的分号，重新组装新的sql语句添加 login id 条件信息
		sqlLen += snprintf(sqlBuffer + sqlLen, sizeof(sqlBuffer) - sqlLen - 1, " or login_id=\'%s\';", req.login_id().c_str());
	}
	
	if (Success == m_psrvMsgHandler->getMySqlOptDB()->queryTableAllResult(sqlBuffer, sqlLen, p_result) && p_result->getRowCount() > 0)
	{
		OptErrorLog("the user account has be locked, name = %s, login id = %s, login ip = %s, count = %d",
		req.username().c_str(), req.login_id().c_str(), req.login_ip().c_str(), p_result->getRowCount());
		
		int result = ServiceUserLimitLogin;
		RowDataPtr rowData = NULL;
		while ((rowData = p_result->getNextRow()) != NULL)
		{
			if (strcmp(rowData[2], req.login_ip().c_str()) == 0)      // IP
			{
				result = ServiceIPLimitLogin;
				break;
			}
			else if (req.login_id().length() > 0 && strcmp(rowData[1], req.login_id().c_str()) == 0)           // 设备ID
			{
				result = ServiceDeviceLimitLogin;
				break;
			}
		}
		
		m_psrvMsgHandler->getMySqlOptDB()->releaseQueryResult(p_result);

		return result;
	}
	m_psrvMsgHandler->getMySqlOptDB()->releaseQueryResult(p_result);

	//计算出输入的旧密码
	char tmp_str[128] = { 0 };
	CMD5 md5;
	memcpy(tmp_str, req.username().c_str(), req.username().length());
	memcpy(tmp_str + req.username().length(), req.passwd().c_str(), req.passwd().length());
	md5.GenerateMD5((unsigned char*)tmp_str, req.username().length() + req.passwd().length());

	if (md5.ToString() != user_baseinfo.static_info.password)
	{
		return ServicePasswordVerifyFailed;//密码校验失败
	}
	
	// 更新登陆ID&IP
	sqlLen = snprintf(sqlBuffer, sizeof(sqlBuffer) - 1, "update tb_user_platform_map_info set login_id=\'%s\',login_ip=\'%s\' where username=\'%s\';",
	req.login_id().c_str(), req.login_ip().c_str(), req.username().c_str());
	m_psrvMsgHandler->m_pDBOpt->executeSQL(sqlBuffer, sqlLen);

	return ServiceSuccess;
}

int CLogic::procGetUserBaseinfoReq(const com_protocol::GetUserBaseinfoReq &req, com_protocol::DetailInfo &detail_info)
{
	//获取DB中的用户信息
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(req.query_username(), user_baseinfo))
	{
		return ServiceGetUserinfoFailed;//获取DB中的信息失败
	}

	//设置用户静态信息
	com_protocol::StaticInfo *pstatic_info = detail_info.mutable_static_info();
	pstatic_info->set_username(user_baseinfo.static_info.username);
	pstatic_info->set_register_time(user_baseinfo.static_info.register_time);
	pstatic_info->set_nickname(user_baseinfo.static_info.nickname);
	pstatic_info->set_person_sign(user_baseinfo.static_info.person_sign);
	pstatic_info->set_realname(user_baseinfo.static_info.realname);
	pstatic_info->set_portrait_id(user_baseinfo.static_info.portrait_id);
	pstatic_info->set_qq_num(user_baseinfo.static_info.qq_num);
	pstatic_info->set_address(user_baseinfo.static_info.address);
	pstatic_info->set_birthday(user_baseinfo.static_info.birthday);
	pstatic_info->set_sex(user_baseinfo.static_info.sex);
	pstatic_info->set_home_phone(user_baseinfo.static_info.home_phone);
	pstatic_info->set_mobile_phone(user_baseinfo.static_info.mobile_phone);
	pstatic_info->set_email(user_baseinfo.static_info.email);
	pstatic_info->set_age(user_baseinfo.static_info.age);
	pstatic_info->set_idc(user_baseinfo.static_info.idc);
	pstatic_info->set_other_contact(user_baseinfo.static_info.other_contact);
	pstatic_info->set_city_id(user_baseinfo.static_info.city_id);

	//设置用户动态信息
	com_protocol::DynamicInfo *pdynamic_info = detail_info.mutable_dynamic_info();
	pdynamic_info->set_level(user_baseinfo.dynamic_info.level);
	pdynamic_info->set_cur_level_exp(user_baseinfo.dynamic_info.cur_level_exp);
	pdynamic_info->set_vip_level(user_baseinfo.dynamic_info.vip_level);
	pdynamic_info->set_vip_cur_level_exp(user_baseinfo.dynamic_info.vip_cur_level_exp);
	pdynamic_info->set_vip_time_limit(user_baseinfo.dynamic_info.vip_time_limit);
	pdynamic_info->set_score(user_baseinfo.dynamic_info.score);
	pdynamic_info->set_game_gold(user_baseinfo.dynamic_info.game_gold);
	pdynamic_info->set_rmb_gold(user_baseinfo.dynamic_info.rmb_gold);
	pdynamic_info->set_voucher_number(user_baseinfo.dynamic_info.voucher_number);
	pdynamic_info->set_phone_card_number(user_baseinfo.dynamic_info.phone_card_number);
	pdynamic_info->set_diamonds_number(user_baseinfo.dynamic_info.diamonds_number);
	pdynamic_info->set_phone_fare(user_baseinfo.dynamic_info.phone_fare);
	pdynamic_info->set_sum_recharge(user_baseinfo.dynamic_info.sum_recharge);
	pdynamic_info->set_sum_login_times(user_baseinfo.dynamic_info.sum_login_times);

	//设置用户道具信息
	com_protocol::PropInfo *pprop_info = detail_info.mutable_prop_info();
	pprop_info->set_light_cannon_count(user_baseinfo.prop_info.light_cannon_count);
	pprop_info->set_mute_bullet_count(user_baseinfo.prop_info.mute_bullet_count);
	pprop_info->set_flower_count(user_baseinfo.prop_info.flower_count);
	pprop_info->set_slipper_count(user_baseinfo.prop_info.slipper_count);
	pprop_info->set_suona_count(user_baseinfo.prop_info.suona_count);
	
	
	// 新增接口信息，道具信息列表
	struct PropItemData
	{
		uint32_t type;
		uint32_t value;
	};

    const CUserPropInfo& pIf = user_baseinfo.prop_info;
	const PropItemData propItemType2Value[] = {{EPropType::PropSuona, pIf.suona_count}, {EPropType::PropLightCannon, pIf.light_cannon_count}, {EPropType::PropFlower, pIf.flower_count},
	{EPropType::PropMuteBullet, pIf.mute_bullet_count}, {EPropType::PropSlipper, pIf.slipper_count}, {EPropType::PropAutoBullet, pIf.auto_bullet_count},
	{EPropType::PropLockBullet, pIf.lock_bullet_count}, {EPropType::PropRampage, pIf.rampage_count}, {EPropType::PropDudShield, pIf.dud_shield_count} };
	
	for (unsigned int idx = 0; idx < sizeof(propItemType2Value) / sizeof(propItemType2Value[0]); ++idx)
	{
		com_protocol::PropItem* propItem = detail_info.add_prop_items();
		propItem->set_type(propItemType2Value[idx].type);
		propItem->set_count(propItemType2Value[idx].value);
	}

	return ServiceSuccess;
}

int CLogic::procRoundEndDataChargeReq(const com_protocol::RoundEndDataChargeReq &req, uint64_t& gameGold)
{
	//获取DB中的用户信息
	gameGold = 0;
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(string(m_psrvMsgHandler->getContext().userData, m_psrvMsgHandler->getContext().userDataLen), user_baseinfo))
	{
		return ServiceGetUserinfoFailed;//获取DB中的信息失败
	}
	
	gameGold = user_baseinfo.dynamic_info.game_gold;  // 更新之前的金币数量，如果失败则返回当前值

	if (req.has_delta_exp() && req.delta_exp() > 0) //经验只增不减
	{
		user_baseinfo.dynamic_info.cur_level_exp += req.delta_exp();	//以后还需判断是否升级	
	}
	
	if (req.has_delta_vip_exp() &&req.delta_vip_exp() > 0)//经验只增不减
	{
		user_baseinfo.dynamic_info.vip_cur_level_exp += req.delta_vip_exp();//以后还需判断是否升级
	}
	
	double rel = 0;
	if (req.has_delta_score())
	{
		rel = (double)user_baseinfo.dynamic_info.score + (double)req.delta_score();
		if (rel < -0.000001)
		{
			user_baseinfo.dynamic_info.score = 0;
		}
		else
		{
			user_baseinfo.dynamic_info.score = user_baseinfo.dynamic_info.score + req.delta_score();
		}
	}
	
	if (req.has_delta_game_gold())
	{
		rel = (double)user_baseinfo.dynamic_info.game_gold + (double)req.delta_game_gold();
		if (rel < -0.000001)
		{
			user_baseinfo.dynamic_info.game_gold = 0;
		}
		else
		{
			user_baseinfo.dynamic_info.game_gold = user_baseinfo.dynamic_info.game_gold + req.delta_game_gold();
		}
	}
	
	/*
	if (req.has_delta_rmb_gold())
	{
		rel = (double)user_baseinfo.dynamic_info.rmb_gold + (double)req.delta_rmb_gold();
		if (rel < -0.000001)
		{
			user_baseinfo.dynamic_info.rmb_gold = 0;
		}
		else
		{
			user_baseinfo.dynamic_info.rmb_gold = user_baseinfo.dynamic_info.rmb_gold + req.delta_rmb_gold();
		}
	}
	*/ 

	if (req.has_delta_voucher_number())
	{
		rel = (double)user_baseinfo.dynamic_info.voucher_number + (double)req.delta_voucher_number();
		if (rel < -0.000001)
		{
			user_baseinfo.dynamic_info.voucher_number = 0;
		}
		else
		{
			user_baseinfo.dynamic_info.voucher_number = user_baseinfo.dynamic_info.voucher_number + req.delta_voucher_number();
		}
	}

	if (req.has_delta_phone_card_number())
	{
		rel = (double)user_baseinfo.dynamic_info.phone_card_number + (double)req.delta_phone_card_number();
		if (rel < -0.000001)
		{
			user_baseinfo.dynamic_info.phone_card_number = 0;
		}
		else
		{
			user_baseinfo.dynamic_info.phone_card_number = user_baseinfo.dynamic_info.phone_card_number + req.delta_phone_card_number();
		}
	}
	
	
	//将数据同步到memcached
	if (!m_psrvMsgHandler->setUserBaseinfoToMemcached(user_baseinfo))
	{
		return ServiceUpdateDataToMemcachedFailed;
	}

	m_psrvMsgHandler->updateOptStatus(string(m_psrvMsgHandler->getContext().userData, m_psrvMsgHandler->getContext().userDataLen), CSrvMsgHandler::UpdateOpt::Modify);

	if (req.has_game_record())
	{
		m_psrvMsgHandler->m_p_game_record->procGameRecord(req.game_record(), &user_baseinfo);
	}
	
	gameGold = user_baseinfo.dynamic_info.game_gold;  // 更新之后的金币数量，成功返回最新值

	return ServiceSuccess;
}

int CLogic::procLoginNotifyReq(const com_protocol::LoginNotifyReq &req)
{
	//获取DB中的用户信息
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(string(m_psrvMsgHandler->getContext().userData, m_psrvMsgHandler->getContext().userDataLen), user_baseinfo))
	{
		return ServiceGetUserinfoFailed;//获取DB中的信息失败
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm* pTm = localtime(&tv.tv_sec);
	snprintf(user_baseinfo.dynamic_info.last_login_time, sizeof(user_baseinfo.dynamic_info.last_login_time), "%04d-%02d-%02d %02d:%02d:%02d",
		(pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
	user_baseinfo.dynamic_info.sum_login_times++;
	
	// 兼容处理，老用户的账号第一次登陆新版本服务器时，老用户的奖券转换成积分券，加到积分券数量中去
	if (req.has_lottery_to_score() && req.lottery_to_score() == 1)
	{
		// 目前，应该奖券是先转换为积分券，然后由于积分券时间过期，会被直接清零，这里把比例调整为 1:1 ，防止出现大额转换
		// const unsigned int allNewScore = user_baseinfo.dynamic_info.score + user_baseinfo.dynamic_info.voucher_number * 2500;  // 老的奖券换算成积分券，1:2500 的比例
		const unsigned int allNewScore = user_baseinfo.dynamic_info.score + user_baseinfo.dynamic_info.voucher_number * 1;  // 老的奖券换算成积分券，1:1 的比例
		DataRecordLog("login change score|%s|%u|%u|%u", m_psrvMsgHandler->getContext().userData, user_baseinfo.dynamic_info.score, user_baseinfo.dynamic_info.voucher_number, allNewScore);
				
		OptInfoLog("user login new version service and reset score value, user = %s, before score = %u, voucher_number = %u, after score = %u",
		m_psrvMsgHandler->getContext().userData, user_baseinfo.dynamic_info.score, user_baseinfo.dynamic_info.voucher_number, allNewScore);

		// 老的奖券换算成积分券
		user_baseinfo.dynamic_info.score = allNewScore;
		user_baseinfo.dynamic_info.voucher_number = 0;  // 清零老数据
	}

	//将数据同步到memcached
	if (!m_psrvMsgHandler->setUserBaseinfoToMemcached(user_baseinfo))
	{
		return ServiceUpdateDataToMemcachedFailed;
	}

	m_psrvMsgHandler->updateOptStatus(string(m_psrvMsgHandler->getContext().userData, m_psrvMsgHandler->getContext().userDataLen), CSrvMsgHandler::UpdateOpt::Login);

	return ServiceSuccess;
}

int CLogic::procLogoutNotifyReq(const com_protocol::LogoutNotifyReq &req)
{
	//获取DB中的用户信息
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(string(m_psrvMsgHandler->getContext().userData, m_psrvMsgHandler->getContext().userDataLen), user_baseinfo))
	{
		return ServiceGetUserinfoFailed;//获取DB中的信息失败
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm* pTm = localtime(&tv.tv_sec);
	snprintf(user_baseinfo.dynamic_info.last_logout_time, sizeof(user_baseinfo.dynamic_info.last_logout_time), "%04d-%02d-%02d %02d:%02d:%02d", 
		(pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday,	pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
	user_baseinfo.dynamic_info.sum_online_time += req.online_time();

	//将数据同步到memcached
	if (!m_psrvMsgHandler->setUserBaseinfoToMemcached(user_baseinfo))
	{
		return ServiceUpdateDataToMemcachedFailed;
	}

	m_psrvMsgHandler->updateOptStatus(string(m_psrvMsgHandler->getContext().userData, m_psrvMsgHandler->getContext().userDataLen), CSrvMsgHandler::UpdateOpt::Logout);

	return ServiceSuccess;
}

int CLogic::procGetMultiUserInfoReq(const com_protocol::GetMultiUserInfoReq &req, com_protocol::GetMultiUserInfoRsp &rsp)
{
	vector<string> dbUser;
	CUserBaseinfo user_baseinfo;
	for (int i = 0; i < req.username_lst_size(); i++)
	{
		if (m_psrvMsgHandler->getUserBaseinfo(req.username_lst(i), user_baseinfo, true))  //获取内存DB中的用户信息
		{
			com_protocol::UserSimpleInfo* pusi = rsp.add_user_simple_info_lst();
			pusi->set_result(0);
		    pusi->set_username(req.username_lst(i));
			pusi->set_nickname(user_baseinfo.static_info.nickname);
			pusi->set_portrait_id(user_baseinfo.static_info.portrait_id);
			pusi->set_sex(user_baseinfo.static_info.sex);
		}
		else
		{
			dbUser.push_back(req.username_lst(i));
		}
	}
	
	if (dbUser.size() > 0)
	{
		char sql[10240] = {"select username,nickname,portrait_id,sex from tb_user_static_baseinfo where "};
		unsigned int len = strlen(sql);
		for (vector<string>::iterator it = dbUser.begin(); it != dbUser.end(); ++it)
		{
			len += snprintf(sql + len, sizeof(sql) - len - 1, "username=\'%s\' OR ", it->c_str());
		}
		len -= strlen(" OR ");
		sql[len] = ';';
		
		CQueryResult* qst = m_psrvMsgHandler->queryUserInfo(sql, len + 1);  //获取DB中的用户信息
		if (qst != NULL)
		{
			RowDataPtr rowData = NULL;
			while ((rowData = qst->getNextRow()) != NULL)
			{
				com_protocol::UserSimpleInfo* pusi = rsp.add_user_simple_info_lst();
				pusi->set_result(0);
				pusi->set_username(rowData[0]);
				pusi->set_nickname(rowData[1]);
				pusi->set_portrait_id(atoi(rowData[2]));
				pusi->set_sex(atoi(rowData[3]));
			}
			
			m_psrvMsgHandler->releaseQueryResult(qst);
		}
	}

	return 0;
}

int CLogic::getRegisterSql(const com_protocol::RegisterUserReq &req, string &sql)
{
	//输入合法性判定
	string username = req.username();
	if (!(username.length() <= 32 && username.length() > 5 && strIsAlnum(username.c_str())))
	{
		return ServiceUsernameInputUnlegal;
	}
	
	//passwd = md5(username + md5(passwd))
	string passwd = req.passwd();
	if (!(passwd.length() == 32 && strIsUpperHex(passwd.c_str())))
	{
		return ServicePasswdInputUnlegal;
	}
	CMD5 md5;
	passwd = req.username() + req.passwd();
	md5.GenerateMD5((const unsigned char*)passwd.c_str(), passwd.length());
	passwd = md5.ToString();
	
	string nickname = req.nickname();
	if (!(nickname.length() <= 32))
	{
		return ServiceNicknameInputUnlegal;
	}
	
	string person_sign = req.has_person_sign() ? req.person_sign() : "";
	if (person_sign != "" && person_sign.length() > 64)
	{
		return ServicePersonSignInputUnlegal;
	}
	
	string realname = req.has_realname() ? req.realname() : "";
	if (realname != "" && realname.length() > 16)
	{
		return ServiceRealnameInputUnlegal;
	}
	
	int32_t portrait_id = req.has_portrait_id() ? req.portrait_id() : 0;
	
	string qq_num = req.has_qq_num() ? req.qq_num() : "";
	if (qq_num != "" && (!strIsDigit(qq_num.c_str()) || qq_num.length() > 16))
	{
		return ServiceQQnumInputUnlegal;
	}
	
	string address = req.has_address() ? req.address() : "";
	if (address != "" && address.length() > 64)
	{
		return ServiceAddrInputUnlegal;
	}
	
	string birthday = req.has_birthday() ? req.birthday() : "1000-01-01";
	if (birthday != "" && !strIsDate(birthday.c_str()))
	{
		return ServiceBirthdayInputUnlegal;
	}
	
	int32_t sex = req.has_sex() ? req.sex() : 0;

	string home_phone = req.has_home_phone() ? req.home_phone() : "";
	if (home_phone != "" && (home_phone.length() > 16 || !strIsAlnum(home_phone.c_str())))
	{
		return ServiceHomephoneInputUnlegal;
	}
	
	string mobile_phone = req.has_mobile_phone() ? req.mobile_phone() : "";
	if (mobile_phone != "" && (mobile_phone.length() > 16 || !strIsAlnum(mobile_phone.c_str())))
	{
		return ServiceMobilephoneInputUnlegal;
	}
	
	string email = req.has_email() ? req.email() : "";
	if (email != "" && email.length() > 36)
	{
		return ServiceEmailInputUnlegal;
	}
	
	uint32_t age = req.has_age() ? req.age() : 0;

	string idc = req.has_idc() ? req.idc() : "";
	if (idc != "" && idc.length() > 24)
	{
		return ServiceIDCInputUnlegal;
	}
	
	string other_contact = req.has_other_contact() ? req.other_contact() : "";
	if (other_contact != "" && other_contact.length() > 64)
	{
		return ServiceOtherContactUnlegal;
	}
	
	uint32_t city_id = req.has_city_id() ? req.city_id() : 0;

	char str_tmp[2048] = { 0 };
	snprintf(str_tmp, sizeof(str_tmp), "insert into tb_user_static_baseinfo(register_time, username, password, nickname, person_sign, realname, portrait_id, \
									   qq_num, address, birthday, sex, home_phone, mobile_phone, email, age, idc, other_contact, city_id) value(now(),\'%s\',\
									   \'%s\',\'%s\',\'%s\',\'%s\',\'%u\',\'%s\',\'%s\',\'%s\',\'%d\',\'%s\',\'%s\',\'%s\',\'%u\',\'%s\',\'%s\',\'%u\');",
									   username.c_str(), passwd.c_str(), nickname.c_str(), person_sign.c_str(), realname.c_str(), portrait_id, qq_num.c_str(),
									   address.c_str(), birthday.c_str(), sex, home_phone.c_str(), mobile_phone.c_str(), email.c_str(), age, idc.c_str(),
									   other_contact.c_str(), city_id);
	sql = str_tmp;

	return ServiceSuccess;
}


unsigned int CLogic::getItemFlag(const unsigned int id)
{
	return (id == m_firstRechargeItemId) ?  MallItemFlag::First : MallItemFlag::NoFlag;  // 首次充值的物品标识
}

// 获取商城配置
void CLogic::getMallConfigReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	//获取DB中的用户信息
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(string(m_psrvMsgHandler->getContext().userData), user_baseinfo))
	{
		m_get_mall_config_rsp.set_voucher_number(0);
		m_get_mall_config_rsp.set_phone_card_number(0);
	}
	else
	{
		m_get_mall_config_rsp.set_voucher_number(user_baseinfo.dynamic_info.voucher_number);
		m_get_mall_config_rsp.set_phone_card_number(user_baseinfo.dynamic_info.phone_card_number);
	}

	char mallConfigData[NFrame::MaxMsgLen] = { 0 };
	uint32_t buf_len = m_get_mall_config_rsp.ByteSize();
	m_get_mall_config_rsp.SerializeToArray(mallConfigData, NFrame::MaxMsgLen);

	m_psrvMsgHandler->sendMessage(mallConfigData, buf_len, srcSrvId, CommonSrvBusiness::BUSINESS_GET_MALL_CONFIG_RSP);
}


void CLogic::handldMsgUserRechargeReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserRechargeReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgUserRechargeReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

    char dataBuffer[MaxMsgLen] = { 0 };
	com_protocol::UserRechargeRsp rsp;
	rsp.set_result(ServiceSuccess);
	rsp.set_item_amount(0);
	rsp.set_allocated_info(&req);
	do 
	{
		// 新版本渔币充值
		if (req.item_type() == EGameGoodsType::EFishCoin)
		{
			const com_protocol::FishCoinInfo* fcInfo = m_psrvMsgHandler->getLogicHander().getFishCoinInfo(req.third_type(), req.item_id());
			if (fcInfo == NULL)
			{
				OptErrorLog("recharge fish coin money the id is invalid, user = %s, id = %u", req.username().c_str(), req.item_id());
				
				rsp.set_result(ServiceItemNotExist);
			    break;
			}
			
			// 是否是首充值
			unsigned int itemFlag = (m_psrvMsgHandler->getLogicDataInstance().fishCoinIsRecharged(req.username().c_str(), req.username().length(), req.item_id())) ?  MallItemFlag::NoFlag : MallItemFlag::First;
			req.set_item_flag(itemFlag);
			unsigned int amount = (req.item_flag() != MallItemFlag::First) ? (fcInfo->num() + fcInfo->gift()) : fcInfo->first_amount();
			rsp.set_item_amount(amount);
			
			// 验证金额，最多保留小数点后两位
			int int_real_rmb = (int)(fcInfo->buy_price() * 100);                   // 实际价格
			int int_recharge_rmb = (int)(req.recharge_rmb() * 100);                // 玩家充值价格
			if (int_recharge_rmb < 1 || int_real_rmb != int_recharge_rmb)
			{
				OptErrorLog("recharge fish coin money not matching, user = %s, real rmb = %.2f, value = %d, pay rmb = %.2f, value = %d",
				req.username().c_str(), fcInfo->buy_price(), int_real_rmb, req.recharge_rmb(), int_recharge_rmb);
				
				rsp.set_result(ServicePriceNotMatching);
				break;
			}
			
			// 充值渔币
			uint32_t ret = rechargeFishCoin(rsp, fcInfo, dataBuffer, MaxMsgLen);
			if (0 != ret)
			{
				OptErrorLog("recharge fish coin money error, user = %s, result = %d", req.username().c_str(), ret);
	            rsp.set_result(ret);
				break;
			}
			
			m_psrvMsgHandler->getLogicDataInstance().rechargeFishCoin(req);

			break;
		}
		
		rsp.set_result(ServiceItemNotExist);
		OptErrorLog("recharge error, user = %s, invalid item type = %d", req.username().c_str(), req.item_type());
		break;
		
		
		/*
		// 老版本代码，为了兼容先保留
		//计算item_amount
		unordered_map<uint32_t, SItemInfo>::iterator ite = m_mitem_info.find(req.item_id());
		if (ite == m_mitem_info.end())
		{
			rsp.set_result(ServiceItemNotExist);
			break;
		}
		
		// 如果是首充则翻倍
		req.set_item_flag(getItemFlag(req.item_id()));
		unsigned int count = (req.item_flag() != NProject::MallItemFlag::First) ? req.item_count() : (req.item_count() * 2);
		rsp.set_item_amount(ite->second.item_num * count);

		//验证金额，最多保留小数点后两位
		float real_rmb = ite->second.buy_price * req.item_count();
		int int_real_rmb = (int)(real_rmb * 100);                   // 实际价格
		int int_recharge_rmb = (int)(req.recharge_rmb() * 100);     // 玩家充值价格
		if (int_recharge_rmb < 1 || int_real_rmb != int_recharge_rmb)
		{
			OptErrorLog("recharge money not matching, real rmb = %.2f, value = %d, pay rmb = %.2f, value = %d",
			real_rmb, int_real_rmb, req.recharge_rmb(), int_recharge_rmb);
			
			rsp.set_result(ServicePriceNotMatching);
			break;
		}

		//开始充值
		uint32_t ret = recharge(rsp, ite->second.giftVector, dataBuffer, MaxMsgLen);
		if (0 != ret)
		{
			rsp.set_result(ret);
			break;
		}
		*/ 
	} while (0);

	DataRecordLog("recharge|%s|%d|%u|%s|%u|%u|%u|%u|%u|%.2f|%s|%s|gift%s", req.username().c_str(), rsp.result(), req.third_type(), req.bills_id().c_str(),
	            req.item_type(), req.item_id(), req.item_count(), req.item_flag(), rsp.item_amount(), req.recharge_rmb(),
				req.third_account().c_str(), req.third_other_data().c_str(), dataBuffer);

	if (rsp.SerializeToArray(dataBuffer, MaxMsgLen))
	{
		m_psrvMsgHandler->sendMessage(dataBuffer, rsp.ByteSize(), srcSrvId, CommonSrvBusiness::BUSINESS_USER_RECHARGE_RSP);
	}
	else
	{
		OptErrorLog("--- handldMsgUserRechargeReq--- pack failed.| userData:%s, result:%d", m_psrvMsgHandler->getContext().userData, rsp.result());
	}

	rsp.release_info();
}

void CLogic::handldMsgExchangePhoneCardReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	/*
	com_protocol::ExchangePhoneCardReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgExchangePhoneCardReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::ExchangePhoneCardRsp rsp;
	rsp.set_exchange_id(req.exchange_id());
	rsp.set_result(0);
	rsp.set_use_count(0);
	do 
	{
		//找到兑换信息
		unordered_map<uint32_t, SExchangeInfo>::iterator ite = m_mechange_info.find(req.exchange_id());
		if (ite == m_mechange_info.end())
		{
			rsp.set_result(ServiceItemNotExist);
			break;
		}

		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_psrvMsgHandler->getUserBaseinfo(string(m_psrvMsgHandler->getContext().userData), user_baseinfo))
		{
			rsp.set_result(ServiceGetUserinfoFailed);//获取DB中的信息失败
			break;
		}

		//判断话费卡是否足够
		if (ite->second.exchange_count > user_baseinfo.dynamic_info.phone_card_number)
		{
			rsp.set_result(ServicePhoneCardNotEnought);
			break;
		}

		//开始兑换
		user_baseinfo.dynamic_info.phone_card_number -= ite->second.exchange_count;
		rsp.set_use_count(ite->second.exchange_count);

		//写DB
		//写动态信息
		if (!m_psrvMsgHandler->updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info))
		{
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}

		//写兑换记录到DB
		char sql_tmp[2048] = { 0 };
		snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_exchange_record_info(username,insert_time,exchange_type,exchange_count,exchange_goods_info,deal_status,mobilephone_number,address,recipients_name,cs_username,pay_type) \
										   values(\'%s\',now(),0,%u,\'兑换面值%u元的电话卡一张\', 0, \'%s\',\'\',\'\',\'\',%u);", m_psrvMsgHandler->getContext().userData,
										   ite->second.exchange_count, ite->second.exchange_get, req.mobilephone_number().c_str(), EPropType::PropTelephoneFare);
		if (Success == m_psrvMsgHandler->m_pDBOpt->modifyTable(sql_tmp))
		{
			//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql_tmp);
		}
		else
		{
			m_psrvMsgHandler->mysqlRollback();
			OptErrorLog("exeSql failed|%s|%s", m_psrvMsgHandler->getContext().userData, sql_tmp);
			rsp.set_result(ServiceInsertFailed);
			break;
		}

		//写到memcached
		if (!m_psrvMsgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			m_psrvMsgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			break;
		}

		//提交
		m_psrvMsgHandler->mysqlCommit();
	} while (0);
	
	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	rsp.SerializeToArray(send_data, MaxMsgLen);

	m_psrvMsgHandler->sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_EXCHANGE_PHONE_CARD_RSP);
	*/ 
}

/*
void CLogic::handldMsgExchangeGoodsReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ExchangeGoodsReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgExchangeGoodsReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::ExchangeGoodsRsp rsp;
	rsp.set_exchange_id(req.exchange_id());
	rsp.set_result(0);
	rsp.set_use_count(0);
	do
	{
		//找到兑换信息
		unordered_map<uint32_t, SExchangeInfo>::iterator ite = m_mechange_info.find(req.exchange_id());
		if (ite == m_mechange_info.end())
		{
			rsp.set_result(ServiceItemNotExist);
			break;
		}

		//获取DB中的用户信息
		CUserBaseinfo user_baseinfo;
		if (!m_psrvMsgHandler->getUserBaseinfo(string(m_psrvMsgHandler->getContext().userData), user_baseinfo))
		{
			rsp.set_result(ServiceGetUserinfoFailed);//获取DB中的信息失败
			break;
		}

		//判断兑换券是否足够
		if (ite->second.exchange_count > user_baseinfo.dynamic_info.voucher_number)
		{
			rsp.set_result(ServiceVoucherNotEnought);
			break;
		}

		//开始兑换
		user_baseinfo.dynamic_info.voucher_number -= ite->second.exchange_count;
		rsp.set_use_count(ite->second.exchange_count);

		//写DB
		//写动态信息
		if (!m_psrvMsgHandler->updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info))
		{
			rsp.set_result(ServiceUpdateDataToMysqlFailed);
			break;
		}

		//写兑换记录到DB
		char sql_tmp[2048] = { 0 };
		snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_exchange_record_info(username,insert_time,exchange_type,exchange_count,exchange_goods_info,deal_status,mobilephone_number,address,recipients_name,cs_username) \
			values(\'%s\',now(),1,%u,\'%s\', 0, \'%s\',\'%s\',\'%s\',\'\');", m_psrvMsgHandler->getContext().userData,
			ite->second.exchange_count, ite->second.exchange_desc, req.mobilephone_number().c_str(), req.address().c_str(), req.recipients_name().c_str());
		if (Success == m_psrvMsgHandler->m_pDBOpt->modifyTable(sql_tmp))
		{
			//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql_tmp);
		}
		else
		{
			m_psrvMsgHandler->mysqlRollback();
			OptErrorLog("exeSql failed|%s|%s", m_psrvMsgHandler->getContext().userData, sql_tmp);
			rsp.set_result(ServiceInsertFailed);
			break;
		}

		//写到memcached
		if (!m_psrvMsgHandler->setUserBaseinfoToMemcached(user_baseinfo))
		{
			m_psrvMsgHandler->mysqlRollback();
			rsp.set_result(ServiceUpdateDataToMemcachedFailed);
			break;
		}

		//提交
		m_psrvMsgHandler->mysqlCommit();
	} while (0);

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	rsp.SerializeToArray(send_data, MaxMsgLen);

	m_psrvMsgHandler->sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_EXCHANGE_GOODS_RSP);
}
*/

void CLogic::handldMsgModifyExchangeGoodsStatusReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ManageModifyExchangGoodsStatusReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- handldMsgModifyExchangeGoodsStatusReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}

	com_protocol::ManageModifyExchangGoodsStatusRsp rsp;
	rsp.set_result(0);

	char sql[2048] = { 0 };
	snprintf(sql, sizeof(sql), "update tb_exchange_record_info set deal_status=%u where id=%u;", req.deal_status(), req.id());

	if (Success == m_psrvMsgHandler->m_pDBOpt->modifyTable(sql))
	{
		m_psrvMsgHandler->mysqlCommit();
		//OptInfoLog("exeSql success|%s|%s", user_static_baseinfo.username, sql);
	}
	else
	{
		rsp.set_result(ServiceUpdateDataToMysqlFailed);
		OptErrorLog("exeSql failed|%s|%s", req.username().c_str(), sql);
	}

	char send_data[MaxMsgLen] = { 0 };
	int send_data_len = rsp.ByteSize();
	rsp.SerializeToArray(send_data, MaxMsgLen);

	m_psrvMsgHandler->sendMessage(send_data, send_data_len, srcSrvId, CommonSrvBusiness::BUSINESS_MODIFY_EXCHANGE_GOODS_STATUS_RSP);
}

uint32_t CLogic::addItemCount(CUserBaseinfo& user_baseinfo, unsigned int type, unsigned int amount)
{
	switch (type)
	{
	case EPropType::PropGold:
		user_baseinfo.dynamic_info.game_gold += amount;
		break;
	case EPropType::PropSuona:
		user_baseinfo.prop_info.suona_count += amount;
		break;
	case EPropType::PropLightCannon:
		user_baseinfo.prop_info.light_cannon_count += amount;
		break;
	case EPropType::PropFlower:
		user_baseinfo.prop_info.flower_count += amount;
		break;
	case EPropType::PropMuteBullet:
		user_baseinfo.prop_info.mute_bullet_count += amount;
		break;
	case EPropType::PropSlipper:
		user_baseinfo.prop_info.slipper_count += amount;
		break;
	case EPropType::PropAutoBullet:  // 自动炮
		user_baseinfo.prop_info.auto_bullet_count += amount;
		break;
	case EPropType::PropLockBullet:  // 锁定炮
		user_baseinfo.prop_info.lock_bullet_count += amount;
		break;

	case EPropType::PropRampage:	// 狂暴
		user_baseinfo.prop_info.rampage_count += amount;
		break;
		
	case EPropType::PropDudShield:	// 哑弹防护
		user_baseinfo.prop_info.dud_shield_count += amount;
		break;

	default:
		return ServiceUnknownItemID;
		break;
	}
	
	return ServiceSuccess;
}

CLogger& CLogic::getDataLogger()
{
	static CLogger dataLogger(CCfg::getValue("Recharge", "LogFileName"), atoi(CCfg::getValue("Recharge", "LogFileSize")), atoi(CCfg::getValue("Recharge", "LogFileCount")));
	return dataLogger;
}

/*
// 老版本充值，为了兼容先保留代码
uint32_t CLogic::recharge(com_protocol::UserRechargeRsp &rsp, PayGiftVector* payGifts, char* giftInfo, unsigned int len)
{
	//获取DB中的用户信息
	giftInfo[0] = '\0';
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(rsp.info().username().c_str(), user_baseinfo))
	{
		return ServiceGetUserinfoFailed;//获取DB中的信息失败
	}

	//充值金币
	bool hasItem = false;
	if (0 == rsp.info().item_type())
	{
		user_baseinfo.dynamic_info.game_gold += rsp.item_amount();
		if (payGifts != NULL)
		{
			unsigned int giftDataLen = 0;
			const PayGiftVector& rechargeGift = *payGifts;
			for (unsigned int giftIdx = 0; giftIdx < rechargeGift.size(); ++giftIdx)
			{
				const MallConfigData::gift_item& g_item = rechargeGift[giftIdx];
				if (g_item.num > 0 && addItemCount(user_baseinfo, g_item.type, g_item.num) == ServiceSuccess)
				{
					com_protocol::GiftInfo* g_info = rsp.add_gift_array();
					g_info->set_type(g_item.type);
					g_info->set_num(g_item.num);
					
					giftDataLen += snprintf(giftInfo + giftDataLen, len - giftDataLen - 1, "%u=%u&", g_item.type, g_item.num);
					
				    if (g_item.type != EPropType::PropGold) hasItem = true;
				}
			}
			
			if (giftDataLen > 0) giftInfo[giftDataLen - 1] = '\0';
		}
	}
	
	//充值道具
	else if (1 == rsp.info().item_type())
	{
		unsigned int rc = addItemCount(user_baseinfo, rsp.info().item_id(), rsp.item_amount());
		if (rc != ServiceSuccess) return rc;
		hasItem = true;
	}

	user_baseinfo.dynamic_info.sum_recharge_count++;
	user_baseinfo.dynamic_info.sum_recharge += rsp.info().recharge_rmb();

	//写DB
	//写动态信息
	if (!m_psrvMsgHandler->updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info))
	{
		return ServiceUpdateDataToMysqlFailed;
	}

	//写道具信息
	if (hasItem)
	{
		if (!m_psrvMsgHandler->updateUserPropInfoToMysql(user_baseinfo.prop_info))
		{
			m_psrvMsgHandler->mysqlRollback();
			return ServiceUpdateDataToMysqlFailed;
		}
	}

	//写充值记录到DB
	char sql_tmp[2048] = { 0 };
	snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_recharge_record_info(bills_id,username,insert_time,item_type,item_id,item_count,item_amount,recharge_rmb,third_account,third_type,third_others_data) \
									   		values(\"%s\",\"%s\",now(),%u,%u,%u,%u,%f,\"%s\",%u,\"%s\");", rsp.info().bills_id().c_str(),
											rsp.info().username().c_str(), rsp.info().item_type(), rsp.info().item_id(), rsp.info().item_count(), rsp.item_amount(), rsp.info().recharge_rmb(),
											rsp.info().third_account().c_str(), rsp.info().third_type(), rsp.info().third_other_data().c_str());
	if (Success == m_psrvMsgHandler->m_pDBOpt->modifyTable(sql_tmp))
	{
		//OptInfoLog("exeSql success|%s|%s", req.username().c_str(), sql_tmp);
	}
	else
	{
		m_psrvMsgHandler->mysqlRollback();
		OptErrorLog("exeSql failed|%s|%s", rsp.info().username().c_str(), sql_tmp);
		return ServiceInsertFailed;
	}

	//写到memcached
	if (!m_psrvMsgHandler->setUserBaseinfoToMemcached(user_baseinfo))
	{
		m_psrvMsgHandler->mysqlRollback();
		return ServiceUpdateDataToMemcachedFailed;
	}

	//提交
	m_psrvMsgHandler->mysqlCommit();

	return 0;
}
*/


// 新版本渔币充值
uint32_t CLogic::rechargeFishCoin(const com_protocol::UserRechargeRsp& rsp, const com_protocol::FishCoinInfo* fcInfo, char* giftInfo, unsigned int len)
{
	// 获取DB中的用户信息
	giftInfo[0] = '\0';
	CUserBaseinfo user_baseinfo;
	if (!m_psrvMsgHandler->getUserBaseinfo(rsp.info().username().c_str(), user_baseinfo))
	{
		return ServiceGetUserinfoFailed;  // 获取DB中的信息失败
	}

	// 充值渔币
	user_baseinfo.dynamic_info.rmb_gold += rsp.item_amount();
	snprintf(giftInfo, len - 1, "%u=%u", fcInfo->id(), fcInfo->gift());

	user_baseinfo.dynamic_info.sum_recharge_count++;
	user_baseinfo.dynamic_info.sum_recharge += rsp.info().recharge_rmb();

	// 写DB
	// 写动态信息
	if (!m_psrvMsgHandler->updateUserDynamicBaseinfoToMysql(user_baseinfo.dynamic_info))
	{
		return ServiceUpdateDataToMysqlFailed;
	}

	// 写充值记录到DB
	char sql_tmp[2048] = { 0 };
	snprintf(sql_tmp, sizeof(sql_tmp), "insert into tb_recharge_record_info(bills_id,username,insert_time,item_type,item_id,item_count,item_amount,recharge_rmb,third_account,third_type,third_others_data) \
									   		values(\"%s\",\"%s\",now(),%u,%u,%u,%u,%f,\"%s\",%u,\"%s\");", rsp.info().bills_id().c_str(),
											rsp.info().username().c_str(), rsp.info().item_type(), rsp.info().item_id(), rsp.info().item_count(), rsp.item_amount(), rsp.info().recharge_rmb(),
											rsp.info().third_account().c_str(), rsp.info().third_type(), rsp.info().third_other_data().c_str());
	if (Success != m_psrvMsgHandler->m_pDBOpt->modifyTable(sql_tmp))
	{
		m_psrvMsgHandler->mysqlRollback();
		OptErrorLog("recharge fish coin, exeSql failed|%s|%s", rsp.info().username().c_str(), sql_tmp);
		return ServiceInsertFailed;
	}

	// 写到memcached
	if (!m_psrvMsgHandler->setUserBaseinfoToMemcached(user_baseinfo))
	{
		m_psrvMsgHandler->mysqlRollback();
		return ServiceUpdateDataToMemcachedFailed;
	}

	//提交
	m_psrvMsgHandler->mysqlCommit();

	return 0;
}

// 直接重置密码
void CLogic::resetPasswordReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ResetPasswordReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- resetPasswordReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}
	
	//获取DB中的用户信息
	com_protocol::ResetPasswordRsp resetPasswordRsp;
	CUserBaseinfo user_baseinfo;
	if (m_psrvMsgHandler->getUserBaseinfo(req.username().c_str(), user_baseinfo))
	{
		if (strcmp(user_baseinfo.static_info.email, req.email_box().c_str()) == 0)
		{
			// 生成6位随机密码
			char randPassword[8] = {0};
			NProject::getRandChars(randPassword, 4);  // 前3位随机字符
			for (int i = 0; i < 3; ++i) randPassword[i + 3] = NProject::getRandNumber(0, 9) + 48;  // 后3位随机数字，转换为字符
			randPassword[6] = '\0';
			
			// 密码MD5加密
			char md5Buff[36] = {0};
			NProject::md5Encrypt(randPassword, 6, md5Buff, false);
			
			com_protocol::ModifyPasswordReq modifyPassword;
			modifyPassword.set_is_reset(1);
			modifyPassword.set_new_passwd(md5Buff);
			modifyPassword.set_old_passwd("");
			int rc = procModifyPasswordReq(modifyPassword, req.username());  // 修改密码
			resetPasswordRsp.set_result(rc);
			if (rc == ServiceSuccess)
			{
				resetPasswordRsp.set_username(req.username());
				resetPasswordRsp.set_email_box(req.email_box());
				resetPasswordRsp.set_new_passwd(randPassword);
			}
		}
		else
		{
		    resetPasswordRsp.set_result(ServiceEmailBoxInvalid);
		}
	}
	else
	{
		resetPasswordRsp.set_result(ServiceGetUserinfoFailed);  //获取DB中的信息失败
	}
	
	int sRc = -1;
	char send_data[MaxMsgLen] = {0};
	if (resetPasswordRsp.SerializeToArray(send_data, MaxMsgLen))
	{
		if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_RESET_PASSWORD_RSP;
	    sRc = m_psrvMsgHandler->sendMessage(send_data, resetPasswordRsp.ByteSize(), srcSrvId, srcProtocolId);
	}
	
	OptInfoLog("resetPasswordReq| result:%d, username:%s, rc:%d", resetPasswordRsp.result(), req.username().c_str(), sRc);
}

void CLogic::queryUserNameReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::QueryUsernameReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- queryUserNameReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}
	
	char sql_tmp[2048] = {0};
	string condition;
	if (req.has_platform_type())
	{
		snprintf(sql_tmp, sizeof(sql_tmp), "where platform_type=%u", req.platform_type());
		condition = sql_tmp;
	}
	
	if (req.has_platform_id())
	{
		if (condition.length() == 0) snprintf(sql_tmp, sizeof(sql_tmp), "where platform_id=\'%s\'", req.platform_id().c_str());
		else snprintf(sql_tmp, sizeof(sql_tmp), " and platform_id=\'%s\'", req.platform_id().c_str());
		condition += sql_tmp;
	}
	
	if (req.has_os_type())
	{
		if (condition.length() == 0) snprintf(sql_tmp, sizeof(sql_tmp), "where os_type=%u", req.os_type());
		else snprintf(sql_tmp, sizeof(sql_tmp), " and os_type=%u", req.os_type());
		condition += sql_tmp;
	}
	
	condition += ";";
	unsigned int sqlLen = snprintf(sql_tmp, sizeof(sql_tmp), "select username from tb_user_platform_map_info %s", condition.c_str());
	CQueryResult* p_result = NULL;			
	if (Success == m_psrvMsgHandler->m_pDBOpt->queryTableAllResult(sql_tmp, sqlLen, p_result))
	{
		com_protocol::QueryUsernameRsp queryUsernameRsp;
		RowDataPtr rowData = NULL;
		while ((rowData = p_result->getNextRow()) != NULL)
		{
			queryUsernameRsp.add_usernames(rowData[0]);
		}
		m_psrvMsgHandler->m_pDBOpt->releaseQueryResult(p_result);
		
		char send_data[MaxMsgLen] = {0};
		if (queryUsernameRsp.SerializeToArray(send_data, MaxMsgLen))
		{
			if (srcProtocolId == 0) srcProtocolId = CommonSrvBusiness::BUSINESS_QUERY_USER_NAMES_RSP;
			m_psrvMsgHandler->sendMessage(send_data, queryUsernameRsp.ByteSize(), srcSrvId, srcProtocolId);
		}
		else
		{
			OptErrorLog("queryUserNameReq QueryUsernameRsp SerializeToArray failed|%d", queryUsernameRsp.ByteSize());
		}
	}
	else
	{
		OptErrorLog("queryUserNameReq exeSql failed|%s", sql_tmp);
	}
}

void CLogic::queryUserInfoReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::GetRankingItemReq req;
	if (!req.ParseFromArray(data, len))
	{
		char log[1024];
		b2str(data, (int)len, log, (int)sizeof(log));
		OptErrorLog("--- queryUserInfoReq--- unpack failed.| len:%u, data:%s", len, log);
		return;
	}
	
	com_protocol::GetRankingItemRsp rsp;
	if (req.has_flag()) rsp.set_flag(req.flag());
	getRankingItemInfo(req, rsp);
	
	char send_data[MaxMsgLen] = { 0 };
	if (!rsp.SerializeToArray(send_data, MaxMsgLen))
	{
		OptErrorLog("--- queryUserInfoReq--- pack failed.| userData:%s, len:%d", m_psrvMsgHandler->getContext().userData, rsp.ByteSize());
		return;
	}
	
	m_psrvMsgHandler->sendMessage(send_data, rsp.ByteSize(), srcSrvId, CommonSrvBusiness::BUSINESS_QUERY_USER_INFO_RSP);
}

int CLogic::getRankingItemInfo(const com_protocol::GetRankingItemReq& req, com_protocol::GetRankingItemRsp& rsp)
{
	// 是否存在兑换信息
	bool hasPhoneCarkExchange = false;
	for (int itemIdx = 0; itemIdx < req.types_size(); ++itemIdx)
	{
		if (EPropType::PhoneCarkExchange == req.types(itemIdx))
		{
			hasPhoneCarkExchange = true;
			break;
		}
	}
	
	unordered_map<string, com_protocol::UserRankingItem*> user2RankingItem;
	vector<string> dbUser;
	CUserBaseinfo user_baseinfo;
	for (int i = 0; i < req.username_list_size(); i++)
	{
		const string& uName = req.username_list(i);
		if (m_psrvMsgHandler->getUserBaseinfo(uName, user_baseinfo, true))  //获取内存DB中的用户信息
		{
			com_protocol::UserRankingItem* rankingItem = rsp.add_ranking_items();
			if (hasPhoneCarkExchange) user2RankingItem[uName] = rankingItem;
			
			rankingItem->set_username(req.username_list(i));
			for (int itemIdx = 0; itemIdx < req.types_size(); ++itemIdx)
			{
				uint64_t itemCount = 0;
				switch (req.types(itemIdx))
				{
					case EPropType::PropGold:
					itemCount = user_baseinfo.dynamic_info.game_gold;
					break;
					
					case EPropType::PropVoucher:
					itemCount = user_baseinfo.dynamic_info.voucher_number;
					break;
					
					case EPropType::PropPhoneFareValue:
					itemCount = user_baseinfo.dynamic_info.phone_fare;
					break;
					
					case EPropType::PropScores: // 积分
					itemCount = user_baseinfo.dynamic_info.score;
					break;					
					
					case EPropType::PhoneCarkExchange:
					break;
					
					default:
					OptErrorLog("getRankingItemInfo from memory db, invalid type = %d", req.types(itemIdx));
					break;
				}
				
				if (itemCount > 0)
				{
					com_protocol::RankingItem* item = rankingItem->add_items();
					item->set_type(req.types(itemIdx));
			        item->set_count(itemCount);
				}
			}
		}
		else
		{
			dbUser.push_back(uName);  // 需要从mysql db查找
		}
	}
	
	if (dbUser.size() > 0)
	{
		char sql[10240] = {"select username,game_gold,score,voucher_number,phone_fare from tb_user_dynamic_baseinfo where "};
		unsigned int len = strlen(sql);
		for (vector<string>::iterator it = dbUser.begin(); it != dbUser.end(); ++it)
		{
			len += snprintf(sql + len, sizeof(sql) - len - 1, "username=\'%s\' OR ", it->c_str());
		}
		len -= strlen(" OR ");
		sql[len] = ';';
		
		CQueryResult* qst = m_psrvMsgHandler->queryUserInfo(sql, len + 1);  //获取DB中的用户信息
		if (qst != NULL)
		{
			RowDataPtr rowData = NULL;
			while ((rowData = qst->getNextRow()) != NULL)
			{
				com_protocol::UserRankingItem* rankingItem = rsp.add_ranking_items();
				if (hasPhoneCarkExchange) user2RankingItem[rowData[0]] = rankingItem;
				
				rankingItem->set_username(rowData[0]);
				for (int itemIdx = 0; itemIdx < req.types_size(); ++itemIdx)
				{
					uint64_t itemCount = 0;
					switch (req.types(itemIdx))
					{
						case EPropType::PropGold:
						itemCount = atoll(rowData[1]);
						break;
						
						case EPropType::PropScores:
						itemCount = atoi(rowData[2]);
						break;
						
						case EPropType::PropVoucher:
						itemCount = atoi(rowData[3]);
						break;
						
						case EPropType::PropPhoneFareValue:
						itemCount = atoi(rowData[4]);
						break;
					
						case EPropType::PhoneCarkExchange:
						break;
						
						default:
						OptErrorLog("getRankingItemInfo from mysql db, invalid type = %d", req.types(itemIdx));
						break;
					}
					
					if (itemCount > 0)
					{
						com_protocol::RankingItem* item = rankingItem->add_items();
						item->set_type(req.types(itemIdx));
						item->set_count(itemCount);
					}
				}
			}
			
			m_psrvMsgHandler->releaseQueryResult(qst);
		}
	}
	
	if (user2RankingItem.size() > 0)
	{
		// 兑换记录
		char sql[10240] = {"select username,exchange_count from tb_exchange_record_info where exchange_type=0 and ("};
		unsigned int len = strlen(sql);
		for (unordered_map<string, com_protocol::UserRankingItem*>::iterator it = user2RankingItem.begin(); it != user2RankingItem.end(); ++it)
		{
			len += snprintf(sql + len, sizeof(sql) - len - 1, "username=\'%s\' OR ", it->first.c_str());
		}
		len -= strlen(" OR ");
		sql[len++] = ')';
		sql[len] = ';';
		
		CQueryResult* qst = m_psrvMsgHandler->queryUserInfo(sql, len + 1);  //获取DB中的用户信息
		if (qst != NULL)
		{
			RowDataPtr rowData = NULL;
			while ((rowData = qst->getNextRow()) != NULL)
			{
				const uint32_t exchange_get = m_exchangeCount2Get[atoi(rowData[1])];
				com_protocol::UserRankingItem* rankingItem = user2RankingItem[rowData[0]];
				if (exchange_get < 1 || rankingItem == NULL) continue;
				
				const int itemIdx = (rankingItem->items_size() > 0) ? (rankingItem->items_size() - 1) : 0;
				com_protocol::RankingItem* item = rankingItem->mutable_items(itemIdx);
				if (item->type() == EPropType::PhoneCarkExchange)
				{
					item->set_count(exchange_get + item->count());
				}
				else
				{
					item = rankingItem->add_items();
					item->set_type(EPropType::PhoneCarkExchange);
					item->set_count(exchange_get);
				}
			}
			
			m_psrvMsgHandler->releaseQueryResult(qst);
		}
	}

	return 0;
}

