
/* author : limingfan
 * date : 2017.09.25
 * description : Http 操作服务实现
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "base/ErrorCode.h"
#include "base/Function.h"
#include "connect/CSocket.h"
#include "CHttpOptSrv.h"


using namespace NCommon;
using namespace NErrorCode;
using namespace NFrame;


// 充值日志文件
#define RechargeLog(platformId, format, args...)     getDataLogger(platformId).writeFile(NULL, 0, LogLevel::Info, format, ##args)


namespace NPlatformService
{

CHttpMsgHandler::CHttpMsgHandler()
{
	m_pCfg = NULL;
}

CHttpMsgHandler::~CHttpMsgHandler()
{
	m_pCfg = NULL;
}


const NHttpOperationConfig::ThirdPlatformInfo* CHttpMsgHandler::getThirdPlatformInfo(unsigned int id)
{
	map<unsigned int, NHttpOperationConfig::ThirdPlatformInfo>::const_iterator pInfoIt = m_pCfg->third_platform_list.find(id);
	return (pInfoIt != m_pCfg->third_platform_list.end()) ? &pInfoIt->second : NULL;
}

CLogger& CHttpMsgHandler::getDataLogger(const unsigned int id)
{
	IdToDataLogger::iterator loggerIt = m_id2DataLogger.find(id);
	if (loggerIt != m_id2DataLogger.end())
	{
		return *loggerIt->second;
	}
	
	const NHttpOperationConfig::ThirdPlatformInfo* ptInfo = getThirdPlatformInfo(id);
	if (ptInfo != NULL)
	{
		CLogger* dataLogger = NULL;
		NEW(dataLogger, CLogger(ptInfo->recharge_log_file_name.c_str(), ptInfo->recharge_log_file_size, ptInfo->recharge_log_file_count));
		if (dataLogger != NULL)
		{
			m_id2DataLogger[id] = dataLogger;
			return *dataLogger;
		}
	}
	
	return CLogger::getOptLogger();
}

CServiceOperation& CHttpMsgHandler::getSrvOpt()
{
	return m_srvOpt;
}

CRedis& CHttpMsgHandler::getRedisOpt()
{
	return m_redisDbOpt;
}

bool CHttpMsgHandler::getTranscationData(const char* orderId, OrderIdToInfo::iterator& it)
{
    it = m_orderId2Info.find(orderId);
    return it != m_orderId2Info.end();
}

void CHttpMsgHandler::removeTransactionData(OrderIdToInfo::iterator it)
{
	if (it != m_orderId2Info.end())
	{
		com_protocol::DelServiceDataReq delOrderReq;
		delOrderReq.set_primary_key(RechargeOrderListKey, RechargeOrderListKeyLen);
		delOrderReq.add_second_keys(it->first.c_str(), it->first.length());

		int rc = m_srvOpt.sendPkgMsgToDbProxy(delOrderReq, DBPROXY_DEL_SERVICE_DATA_REQ, "", 0, "delete user recharge order");	
		OptInfoLog("remove recharge order data, id = %s, result = %d", it->first.c_str(), rc);

		m_orderId2Info.erase(it);
	}
}

// 充值发放物品
void CHttpMsgHandler::provideRechargeItem(int result, OrderIdToInfo::iterator orderInfoIt, unsigned int platformType, const char* uid, const char* thirdOtherData)
{
    const ThirdOrderInfo& thirdInfo = orderInfoIt->second;
	if (result == SrvOptSuccess)
	{
		// 充值成功，发放物品，充值信息入库
		com_protocol::UserRechargeReq userRechargeReq;
		userRechargeReq.set_order_id(orderInfoIt->first.c_str());
		userRechargeReq.set_username(thirdInfo.user);
		userRechargeReq.set_hall_id(thirdInfo.hallId);
		userRechargeReq.set_item_type(EGameGoodsType::EGoodsRMB);
		userRechargeReq.set_item_id(thirdInfo.itemId);
		userRechargeReq.set_item_count(1);
		userRechargeReq.set_recharge_rmb(thirdInfo.money);
		userRechargeReq.set_third_type(platformType);
		userRechargeReq.set_third_account(uid);
		userRechargeReq.set_third_other_data(thirdOtherData);
	    userRechargeReq.set_item_flag(0);

        int rc = m_srvOpt.sendPkgMsgToDbProxy(userRechargeReq, DBPROXY_USER_RECHARGE_REQ, userRechargeReq.username().c_str(), userRechargeReq.username().length(), "user recharge request");
		OptInfoLog("send third platform recharge payment result to db proxy, rc = %d, user name = %s, bill id = %s",
		rc, userRechargeReq.username().c_str(), userRechargeReq.order_id().c_str());
	}
	else
	{
		// 充值失败则不会发放物品，直接通知客户端
		com_protocol::ServiceRechargeResultNotify rechargeResultNotify;
		rechargeResultNotify.set_result(HOPTRechargeFailed);
		rechargeResultNotify.set_order_id(orderInfoIt->first.c_str());
		rechargeResultNotify.set_item_id(thirdInfo.itemId);
		rechargeResultNotify.set_item_amount(thirdInfo.itemAmount);
		rechargeResultNotify.set_money(thirdInfo.money);
		rechargeResultNotify.set_code(result);

        int rc = m_srvOpt.sendPkgMsgToService(thirdInfo.user, strlen(thirdInfo.user), rechargeResultNotify, thirdInfo.srcSrvId, HTTPOPT_RECHARGE_RESULT_NOTIFY, "recharge failed notify");
		OptWarnLog("send third platform recharge payment result to client, rc = %d, user name = %s, result = %d, bill id = %s",
		rc, thirdInfo.user, result, orderInfoIt->first.c_str());
		
		// removeTransactionData(orderInfoIt);  // 先不要删除，可能存在第三方通知错误，重新发起消息即可，否则订单超时会自动删除
	}
}

// 执行定时任务
void CHttpMsgHandler::doTaskTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount)
{
	OptInfoLog("do task time begin, current timer id = %u, remain count = %u", timerId, remainCount);

	unsigned int rmCount = checkUnHandleOrderData();  // 检查过期的未处理订单

	// 重新设置定时器
	struct tm tmval;
	time_t curSecs = time(NULL);
	unsigned int intervalSecs = 60 * 60 * 24;
	if (localtime_r(&curSecs, &tmval) != NULL)
	{
		tmval.tm_sec = 0;
		tmval.tm_min = 0;
		tmval.tm_hour = atoi(CCfg::getValue("HttpService", "DoTaskTime"));
		++tmval.tm_mday;
		time_t nextSecs = mktime(&tmval);
		if (nextSecs != (time_t)-1)
		{
			intervalSecs = nextSecs - curSecs;
		}
		else
		{
			OptErrorLog("do task timer, get next time error");
		}
	}
	else
	{
		OptErrorLog("do task timer, get local time error");
	}

	unsigned int tId = setTimer(intervalSecs * MillisecondUnit, (TimerHandler)&CHttpMsgHandler::doTaskTimer);
	OptInfoLog("do task time end, next timer id = %u, interval = %u, date = %d-%02d-%02d %02d:%02d:%02d, remove timeout recharge order = %u",
	tId, intervalSecs, tmval.tm_year + 1900, tmval.tm_mon + 1, tmval.tm_mday, tmval.tm_hour, tmval.tm_min, tmval.tm_sec, rmCount);
}

unsigned int CHttpMsgHandler::checkUnHandleOrderData()
{
	// 未处理订单最大时间间隔，超过配置的间隔时间还未处理的订单数据则直接删除
	unsigned int rmCount = 0;
	unsigned int deleteSecs = time(NULL) - atoi(CCfg::getValue("HttpService", "UnHandleOrderMaxIntervalSeconds"));
	for (OrderIdToInfo::iterator it = m_orderId2Info.begin(); it != m_orderId2Info.end();)
	{
		if (it->second.timeSecs < deleteSecs)
		{
			// 充值订单信息记录，超时取消
			// format = name|id|amount|money|rechargeNo|srcSrvId|hallid|timeout
			const ThirdOrderInfo& thirdInfo = it->second;
			RechargeLog(thirdInfo.platformId, "%s|%u|%u|%.2f|%s|%u|%s|timeout",
			thirdInfo.user, thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, it->first.c_str(), thirdInfo.srcSrvId, thirdInfo.hallId);

			time_t createTimeSecs = it->second.timeSecs;
	        struct tm* tmval = localtime(&createTimeSecs);
			OptWarnLog("remove time out unhandle order data, date = %d-%02d-%02d %02d:%02d:%02d, user = %s, hall id = %s, order id = %s, item id = %u, amount = %u, money = %.2f, srcSrvId = %u",
			tmval->tm_year + 1900, tmval->tm_mon + 1, tmval->tm_mday, tmval->tm_hour, tmval->tm_min, tmval->tm_sec,
			thirdInfo.user, thirdInfo.hallId, it->first.c_str(), thirdInfo.itemId, thirdInfo.itemAmount, thirdInfo.money, thirdInfo.srcSrvId);
			
			removeTransactionData(it++);
			++rmCount;
		}
		else
		{
			++it;
		}
	}
	
	return rmCount;
}

int CHttpMsgHandler::getUnHandleOrderData()
{	
	// 获取之前没有处理完毕的订单数据，一般该数据为空
	// 当且仅当用户充值流程没走完并且此种情况下服务异常退出时才会存在未处理完毕的订单数据
	// 注意：用户请求订单，但一直没有取消订单的情况下，该订单会一直存在直到超时被删除
	com_protocol::GetMoreKeyServiceDataReq getRechargeDataReq;
	getRechargeDataReq.set_primary_key(RechargeOrderListKey, RechargeOrderListKeyLen);
	
	int rc = m_srvOpt.sendPkgMsgToDbProxy(getRechargeDataReq, DBPROXY_GET_MORE_SERVICE_DATA_REQ, "", 0, "get all recharge data request");
    if (rc != SrvOptSuccess) OptErrorLog("get no handle recharge order data error, rc = %d", rc);
	
	return rc;
}

bool CHttpMsgHandler::parseOrderData(const char* orderId, const char* orderData, ThirdOrderInfo& orderInfo)
{
	// 订单号 格式 : platformTtimeIindexSsrcSrvId
	// 3T1466042162I123S1001
	
	// 订单数据 格式 : id.amount.money-srv.user-platformId.time.hallId
	// 4003.600.0.98-1001.10000001-3.1466042162.666789
	enum EItemInfo
	{
		Eid = 0,
		Eamount = 1,
		Esize = 2,
	};
	unsigned int itemValue[EItemInfo::Esize] = {0};
	
	char tmpOrderData[MaxRechargeOrderLen] = {'\0'};
	strncpy(tmpOrderData, orderData, MaxRechargeOrderLen - 1);
	
	// 物品信息
	const char* itemInfo = tmpOrderData;
	for (int idx = 0; idx < EItemInfo::Esize; ++idx)
	{
		char* valueEnd = (char*)strchr(itemInfo, '.');
		if (*itemInfo == '-' || *itemInfo == '\0' || valueEnd == NULL)
		{
			OptErrorLog("parse recharge order data error, can not find the goods info, data = %s", orderData);
			return false;
		}
		
		*valueEnd = '\0';
		itemValue[idx] = atoi(itemInfo);
		
		itemInfo = ++valueEnd;
	}
	
	// 充值价格
    char* moneyInfo = (char*)strchr(itemInfo, '-');
	if (moneyInfo == NULL)
	{
		OptErrorLog("parse recharge order data error, can not find the money info, data = %s", orderData);
		return false;
	}
	*moneyInfo = '\0';
	const double money = atof(itemInfo);
	
	// 订单数据 格式 : id.amount.money-srv.user-platformId.time.hallId
	// 用户信息
	char* uName = (char*)strchr(++moneyInfo, '.');
	if (uName == NULL)
	{
		OptErrorLog("parse recharge order data error, can not find the user name info, data = %s", orderData);
		return false;
	}
	*uName = '\0';
	const unsigned int srcSrvId = atoi(moneyInfo);
	++uName;

	// 平台类型ID值
	char* platformInfo = (char*)strchr(uName, '-');
	if (platformInfo == NULL)
	{
		OptErrorLog("parse recharge order data error, can not find the platform info, data = %s", orderData);
		return false;
	}
	
	*platformInfo = '\0';  // uName user 结束符，结束点
	char* dataTimeInfo = (char*)strchr(++platformInfo, '.');  // 订单数据时间戳
	if (dataTimeInfo == NULL)
	{
		OptErrorLog("parse recharge order data error, can not find the time info, data = %s", orderData);
		return false;
	}
	*dataTimeInfo = '\0';  // platformId 结束符，结束点
	const unsigned int platformId = atoi(platformInfo);
	
	char* hallId = (char*)strchr(++dataTimeInfo, '.');  // 棋牌室ID
	if (hallId == NULL)
	{
		OptErrorLog("parse recharge order data error, can not find the hall id info, data = %s", orderData);
		return false;
	}
	
	*hallId = '\0';  // time 结束符，结束点
	const unsigned int timeSecs = atoi(dataTimeInfo);
	++hallId;
	
	if (getThirdPlatformInfo(platformId) == NULL || timeSecs < 1)
	{
		OptErrorLog("parse recharge order data error, can not find the platform or time info, platform = %d, time = %u, order id = %s, data = %s",
		platformId, timeSecs, orderId, orderData);
		return false;
	}
	
	strncpy(orderInfo.user, uName, IdMaxLen - 1);
	strncpy(orderInfo.hallId, hallId, IdMaxLen - 1);
	orderInfo.timeSecs = timeSecs;
	orderInfo.srcSrvId = srcSrvId;
	orderInfo.platformId = platformId;
	orderInfo.itemId = itemValue[EItemInfo::Eid];
	orderInfo.itemAmount = itemValue[EItemInfo::Eamount];
	orderInfo.money = money;

	time_t createTimeSecs = timeSecs;
	struct tm* tmval = localtime(&createTimeSecs);
	OptInfoLog("parse recharge order data, date = %d-%02d-%02d %02d:%02d:%02d, user name = %s, hall id = %s, platform = %u, time = %u, id = %u, amount = %u, money = %.2f, srcSrvId = %u, order id = %s, data = %s",
	tmval->tm_year + 1900, tmval->tm_mon + 1, tmval->tm_mday, tmval->tm_hour, tmval->tm_min, tmval->tm_sec, uName, hallId, platformId, timeSecs,
	itemValue[EItemInfo::Eid], itemValue[EItemInfo::Eamount], money, srcSrvId, orderId, orderData);
	
	return true;
}


// 获取第三方平台订单ID
bool CHttpMsgHandler::getThirdOrderId(const unsigned int platformType, const unsigned int osType, const unsigned int srcSrvId, char* orderId, unsigned int len)
{
	const unsigned int MinOrderBufferLen = 32;
	if (len < MinOrderBufferLen) return false;

    const NHttpOperationConfig::ThirdPlatformInfo* ptInfo = getThirdPlatformInfo(platformType);
    if (ptInfo == NULL)
	{
		OptErrorLog("get third order id, invalid platform type = %u", platformType);
		return false;
	}

    // 订单ID全局唯一
	// 订单号格式 : platformTtimeIindexSsrcSrvId
	// 3T1458259523I23S4001
	const unsigned int MaxOrderIdIndex = 10000;
	unsigned int idLen = snprintf(orderId, len - 1, "%uT%uI%uS%u",
	                              platformType, (unsigned int)time(NULL), ++m_id2OrderIndex[platformType] % MaxOrderIdIndex, srcSrvId);
	orderId[idLen] = 0;

	return true;
}

const ThirdOrderInfo* CHttpMsgHandler::saveThirdOrderData(const char* userName, const char* hallId, const unsigned int srcSrvId, const unsigned int platformId,
														  const char* orderId, const com_protocol::ClientGetRechargeOrderRsp& orderRsp)
{
	if (orderRsp.result() != SrvOptSuccess) return NULL;

	// 订单数据格式 : id.amount.money-srv.user-platformId.time.hallid
	// 4003.600.0.98-1001.10000001-1.1458259523.666789
	char rechargeData[MaxRechargeOrderLen] = {'\0'};
	const unsigned int currentTimeSecs = time(NULL);
	unsigned int dataLen = snprintf(rechargeData, sizeof(rechargeData) - 1, "%u.%u.%.2f-%u.%s-%u.%u.%s",
	orderRsp.item_id(), orderRsp.item_amount(), orderRsp.money(), srcSrvId, userName, platformId, currentTimeSecs, hallId);
	
	// 订单信息临时保存到数据库，防止服务异常退出导致订单数据丢失
	com_protocol::SetServiceDataReq saveOrderReq;
	saveOrderReq.set_primary_key(RechargeOrderListKey, RechargeOrderListKeyLen);
	saveOrderReq.set_second_key(orderId);
	saveOrderReq.set_data(rechargeData, dataLen);
	
	int rc = m_srvOpt.sendPkgMsgToDbProxy(saveOrderReq, DBPROXY_SET_SERVICE_DATA_REQ, userName, strlen(userName), "save user recharge order data request");
    if (rc != SrvOptSuccess)
	{
		OptErrorLog("save recharge order data error, rc = %d", rc);
		return NULL;
	}
	
	// 内存存储
	m_orderId2Info[orderId] = ThirdOrderInfo();
	ThirdOrderInfo& info = m_orderId2Info[orderId];
	strncpy(info.user, userName, IdMaxLen - 1);
	strncpy(info.hallId, hallId, IdMaxLen - 1);
	info.timeSecs = currentTimeSecs;
	info.srcSrvId = srcSrvId;
	info.platformId = platformId;
	info.itemId = orderRsp.item_id();
	info.itemAmount = orderRsp.item_amount();
	info.money = orderRsp.money();
	
	return &info;
}

void CHttpMsgHandler::removeTransactionData(const char* orderId)
{
	removeTransactionData(m_orderId2Info.find(orderId));
}

// 第三方平台充值结果处理
bool CHttpMsgHandler::rechargeReply(const com_protocol::UserRechargeRsp& userRechargeRsp)
{
	const com_protocol::UserRechargeReq& userRechargeInfo = userRechargeRsp.info();
	OrderIdToInfo::iterator orderInfoIt;
	if (!getTranscationData(userRechargeInfo.order_id().c_str(), orderInfoIt))
	{
		OptErrorLog("can not find the recharge order info, user = %s, name = %s, hall id = %s, bill id = %s, money = %.2f, result = %d",
		getContext().userData, userRechargeInfo.username().c_str(), userRechargeInfo.hall_id().c_str(),
		userRechargeInfo.order_id().c_str(), userRechargeInfo.recharge_rmb(), userRechargeRsp.result());

		return false;
	}

    const ThirdOrderInfo& thirdInfo = orderInfoIt->second;
    com_protocol::ServiceRechargeResultNotify rechargeResultNotify;
	rechargeResultNotify.set_result(userRechargeRsp.result());
	rechargeResultNotify.set_order_id(orderInfoIt->first.c_str());
	rechargeResultNotify.set_item_id(thirdInfo.itemId);
	rechargeResultNotify.set_item_amount(userRechargeRsp.item_amount());
	rechargeResultNotify.set_money(thirdInfo.money);

	int rc = m_srvOpt.sendPkgMsgToService(thirdInfo.user, strlen(thirdInfo.user), rechargeResultNotify, thirdInfo.srcSrvId, HTTPOPT_RECHARGE_RESULT_NOTIFY, "recharge result notify");

    // 充值订单信息记录最终结果
    // format = name|id|amount|flag|money|rechargeNo|srcSrvId|finish|hallid|umid|result|other
	RechargeLog(userRechargeInfo.third_type(), "%s|%u|%u|%u|%.2f|%s|%u|finish|%s|%s|%d|%s", thirdInfo.user, thirdInfo.itemId, userRechargeRsp.item_amount(),
	userRechargeInfo.item_flag(), userRechargeInfo.recharge_rmb(), orderInfoIt->first.c_str(), thirdInfo.srcSrvId,
	userRechargeInfo.hall_id().c_str(), userRechargeInfo.third_account().c_str(), userRechargeRsp.result(), userRechargeInfo.third_other_data().c_str());

	OptInfoLog("send recharge result notify, rc = %d, srcSrvId = %u, user name = %s, hall id = %s, result = %d, bill id = %s, money = %.2f, uid = %s, other = %s",
	rc, thirdInfo.srcSrvId, userRechargeInfo.username().c_str(), userRechargeInfo.hall_id().c_str(), userRechargeRsp.result(),
	userRechargeInfo.order_id().c_str(), userRechargeInfo.recharge_rmb(), userRechargeInfo.third_account().c_str(), userRechargeInfo.third_other_data().c_str());
	
	removeTransactionData(orderInfoIt);  // 充值操作处理完毕，删除该充值信息
	return true;
}


void CHttpMsgHandler::getUnHandleOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	// 获取之前没有处理完毕的订单数据，一般该数据为空
	// 当且仅当用户充值流程没走完并且此种情况下服务异常退出时才会存在未处理完毕的订单数据
	// 注意：用户请求订单，但一直没有取消订单的情况下，该订单会一直存在直到超时被删除
    com_protocol::GetMoreKeyServiceDataRsp getRechargeDataRsp;
	if (!m_srvOpt.parseMsgFromBuffer(getRechargeDataRsp, data, len, "get unhandle recharge order data reply")) return;
	
	if (getRechargeDataRsp.result() != SrvOptSuccess)
	{
		OptErrorLog("get unhandle recharge order data reply error, result = %d", getRechargeDataRsp.result());
		return;
	}
	
	for (int idx = 0; idx < getRechargeDataRsp.second_key_data_size(); ++idx)
	{
		const com_protocol::SecondKeyServiceData& skOrderData = getRechargeDataRsp.second_key_data(idx);
		const char* orderId = skOrderData.key().c_str();       // 订单ID值
		const char* orderData = skOrderData.data().c_str();    // 订单数据内容
		
		m_orderId2Info[orderId] = ThirdOrderInfo();
	    ThirdOrderInfo& info = m_orderId2Info[orderId];
		if (!parseOrderData(orderId, orderData, info))
		{
			m_orderId2Info.erase(orderId);
			
			OptErrorLog("exist unhandle recharge order data error, id = %s, data = %s", orderId, orderData);
		}
	}
	
	unsigned int rmCount = checkUnHandleOrderData();
	OptInfoLog("get unhandle recharge order data reply, unhandle count = %d, remove count = %u",
	getRechargeDataRsp.second_key_data_size(), rmCount);
}

// 充值处理操作流程
void CHttpMsgHandler::getRechargeOrder(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientGetRechargeOrderReq getOrderReq;
	if (!m_srvOpt.parseMsgFromBuffer(getOrderReq, data, len, "get recharge order request")) return;

	// 获取订单信息
	com_protocol::ClientGetRechargeOrderRsp getOrderRsp;
	do
	{
		map<unsigned int, MallConfigData::MoneyItem>::const_iterator moneyIt = m_srvOpt.getMallCfg().money_map.find(getOrderReq.item_id());
		if (moneyIt == m_srvOpt.getMallCfg().money_map.end())
		{
			getOrderRsp.set_result(HOPTNoFoundItemInfo);
			break;
		}
		
		char orderId[MaxRechargeOrderLen] = {'\0'};
		if (!getThirdOrderId(getOrderReq.platform_type(), getOrderReq.os_type(), srcSrvId, orderId, sizeof(orderId)))
		{
			getOrderRsp.set_result(HOPTGetOrderIdFailed);
			break;
		}
		
		getOrderRsp.set_result(SrvOptSuccess);
		getOrderRsp.set_order_id(orderId);
		getOrderRsp.set_item_id(getOrderReq.item_id());
		getOrderRsp.set_item_amount(moneyIt->second.num);
		getOrderRsp.set_money(moneyIt->second.price);
		
		// 存储账单数据
		const ThirdOrderInfo* thirdOrderInfo = saveThirdOrderData(getContext().userData, getOrderReq.hall_id().c_str(), srcSrvId,
		                                                          getOrderReq.platform_type(), orderId, getOrderRsp);
		if (thirdOrderInfo == NULL)
		{
			getOrderRsp.Clear();
			getOrderRsp.set_result(HOPTSaveOrderDataError);
			break;
		}

	} while (false);

    int rc = m_srvOpt.sendPkgMsgToService(getOrderRsp, srcSrvId, HTTPOPT_GET_RECHARGE_ORDER_RSP, "get recharge order reply");
    if (rc == SrvOptSuccess && getOrderRsp.result() == SrvOptSuccess)
	{
		// 充值订单信息记录
		// format = name|id|amount|money|orderId|request|platformType|srcSrvId|hallId
		RechargeLog(getOrderReq.platform_type(), "%s|%u|%u|%.2f|%s|request|%u|%u|%s",
		getContext().userData, getOrderRsp.item_id(), getOrderRsp.item_amount(), getOrderRsp.money(),
		getOrderRsp.order_id().c_str(), getOrderReq.platform_type(), srcSrvId, getOrderReq.hall_id().c_str());
	}
	
	// 发送失败则删除订单数据
	if (rc != SrvOptSuccess && getOrderRsp.result() == SrvOptSuccess) removeTransactionData(getOrderRsp.order_id().c_str());
	
	OptInfoLog("get recharge order record, srcSrvId = %u, user = %s, hall id = %s, item id = %u, money = %.2f, order id = %s, platform type = %u, ostype = %u, result = %d, rc = %d",
		       srcSrvId, getContext().userData, getOrderReq.hall_id().c_str(), getOrderReq.item_id(), getOrderRsp.money(), getOrderRsp.order_id().c_str(),
			   getOrderReq.platform_type(), getOrderReq.os_type(), getOrderRsp.result(), rc);
}

void CHttpMsgHandler::cancelRechargeOrder(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::ClientCancelRechargeNotify cancelOrderNotify;
	if (!m_srvOpt.parseMsgFromBuffer(cancelOrderNotify, data, len, "cancel recharge order notify")) return;

	removeTransactionData(cancelOrderNotify.order_id().c_str());  // 删除订单数据
	
	// 充值订单信息记录
	// format = name|orderId|cancel|platformType|srcSrvId|reason|info
	RechargeLog(cancelOrderNotify.platform_type(), "%s|%s|cancel|%u|%u|%d|%s",
	getContext().userData, cancelOrderNotify.order_id().c_str(), cancelOrderNotify.platform_type(), srcSrvId,
	cancelOrderNotify.code(), cancelOrderNotify.info().c_str());
	
	OptInfoLog("cancel recharge order, srcSrvId = %u, user = %s, order id = %s, platform type = %d, reason = %d, info = %s",
	srcSrvId, getContext().userData, cancelOrderNotify.order_id().c_str(), cancelOrderNotify.platform_type(),
	cancelOrderNotify.code(), cancelOrderNotify.info().c_str());
}


void CHttpMsgHandler::saveOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::SetServiceDataRsp setRsp;
	if (!m_srvOpt.parseMsgFromBuffer(setRsp, data, len, "save recharge order data reply")) return;
	
	if (setRsp.result() != SrvOptSuccess) OptErrorLog("save recharge order data error, key = %s, result = %d", setRsp.second_key().c_str(), setRsp.result());
}

void CHttpMsgHandler::removeOrderDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::DelServiceDataRsp delRsp;
	if (!m_srvOpt.parseMsgFromBuffer(delRsp, data, len, "delete recharge order data reply")) return;
	
	if (delRsp.result() != 0)
	{
		const char* key = (delRsp.second_keys_size() > 0) ? delRsp.second_keys(0).c_str() : "";
		OptErrorLog("delete recharge order data error, key = %s, result = %d", key, delRsp.result());
	}
}

void CHttpMsgHandler::rechargeItemReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId)
{
	com_protocol::UserRechargeRsp userRechargeRsp;
	if (!m_srvOpt.parseMsgFromBuffer(userRechargeRsp, data, len, "user recharge reply")) return;
	
	bool isOK = rechargeReply(userRechargeRsp);
	
	const com_protocol::UserRechargeReq& userRechargeInfo = userRechargeRsp.info();
	OptInfoLog("receive recharge reply, user = %s, hall id = %s, platform type = %u, isOK = %d, result = %d, order id = %s",
	userRechargeInfo.username().c_str(), userRechargeInfo.hall_id().c_str(), userRechargeInfo.third_type(), isOK,
	userRechargeRsp.result(), userRechargeInfo.order_id().c_str());
}


int CHttpMsgHandler::onInit(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	if (!m_srvOpt.init(this, CCfg::getValue("HttpService", "ConfigFile")))
	{
		OptErrorLog("init service operation instance error");

		return SrvOptFailed;
	}
	
	m_pCfg = &NHttpOperationConfig::HttpOptConfig::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str());
	if (!m_pCfg->isSetConfigValueSuccess())
	{
		OptErrorLog("set business xml config value error");
		
		return SrvOptFailed;
	}
	
	const DBConfig::config& dbCfg = m_srvOpt.getDBCfg();
	if (!dbCfg.isSetConfigValueSuccess())
	{
		OptErrorLog("get db xml config value error");

		return SrvOptFailed;
	}
	
	// redis DB
	if (!m_redisDbOpt.connectSvr(dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port,
	    dbCfg.redis_db_cfg.center_db_timeout * MillisecondUnit))
	{
		OptErrorLog("connect center redis service failed, ip = %s, port = %u, time out = %u",
		dbCfg.redis_db_cfg.center_db_ip.c_str(), dbCfg.redis_db_cfg.center_db_port, dbCfg.redis_db_cfg.center_db_timeout);
		
		return SrvOptFailed;
	}
	
	if (m_wxLogin.init(this) != SrvOptSuccess)
	{
		OptErrorLog("init wx login instance error");

		return SrvOptFailed;
	}
    
	if (m_httpLogicHandler.init(this) != SrvOptSuccess)
	{
		OptErrorLog("init http logic handler instance error");

		return SrvOptFailed;
	}
    
    if (m_serviceLogicHandler.init(this) != SrvOptSuccess)
	{
		OptErrorLog("init service logic handler instance error");

		return SrvOptFailed;
	}
    
	int rc = getUnHandleOrderData();
	if (rc != SrvOptSuccess)
	{
		OptErrorLog("get unhandle recharge order error, rc = %d", rc);
		return rc;
	}

    registerProtocol(ServiceType::OutsideClientSrv, HTTPOPT_GET_RECHARGE_ORDER_REQ, (ProtocolHandler)&CHttpMsgHandler::getRechargeOrder);
	registerProtocol(ServiceType::OutsideClientSrv, HTTPOPT_CANCEL_RECHARGE_NOTIFY, (ProtocolHandler)&CHttpMsgHandler::cancelRechargeOrder);

	registerProtocol(ServiceType::DBProxySrv, DBPROXY_GET_MOER_SERVICE_DATA_RSP, (ProtocolHandler)&CHttpMsgHandler::getUnHandleOrderDataReply);
	registerProtocol(ServiceType::DBProxySrv, DBPROXY_SET_SERVICE_DATA_RSP, (ProtocolHandler)&CHttpMsgHandler::saveOrderDataReply);
	registerProtocol(ServiceType::DBProxySrv, DBPROXY_DEL_SERVICE_DATA_RSP, (ProtocolHandler)&CHttpMsgHandler::removeOrderDataReply);
	
	registerProtocol(ServiceType::DBProxySrv, DBPROXY_USER_RECHARGE_RSP, (ProtocolHandler)&CHttpMsgHandler::rechargeItemReply);
	
	// 启动定时器执行定时任务
	doTaskTimer(0, 0, NULL, 0);
	
	m_srvOpt.getMallCfg().output();
	m_pCfg->output();

	return SrvOptSuccess;
}

void CHttpMsgHandler::onUnInit(const char* srvName, const unsigned int srvId, unsigned short moduleId)
{
	m_wxLogin.unInit();
    m_httpLogicHandler.unInit();
    m_serviceLogicHandler.unInit();
	
	m_redisDbOpt.disconnectSvr();
}

void CHttpMsgHandler::onUpdateConfig()
{
	m_srvOpt.updateCommonConfig(CCfg::getValue("HttpService", "ConfigFile"));

	m_pCfg = &NHttpOperationConfig::HttpOptConfig::getConfigValue(m_srvOpt.getCommonCfg().config_file.service_cfg_file.c_str(), true);
	if (!m_pCfg->isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update business xml config value error");
	}

	if (!m_srvOpt.getMallCfg(true).isSetConfigValueSuccess())
	{
		ReleaseErrorLog("update mall xml config value error");
	}
	
	ReleaseInfoLog("update config business result = %d, mall result = %d",
	m_pCfg->isSetConfigValueSuccess(), m_srvOpt.getMallCfg().isSetConfigValueSuccess());
	
	m_srvOpt.getMallCfg().output();
	m_pCfg->output();
}



void CHttpOptSrv::onRegister(const char* name, const unsigned int id)
{
	ReleaseInfoLog("register http module, service name = %s, id = %d", name, id);
	
	// 注册模块实例
	const unsigned short HandlerMessageModuleId = 0;
	static CHttpMsgHandler msgHandler;
	registerHttpHandler(&msgHandler);                        // 作为HTTP消息处理模块
	registerModule(HandlerMessageModuleId, &msgHandler);     // 同时也是内部服务消息处理模块
}


CHttpOptSrv::CHttpOptSrv() : CHttpSrv(HttpOperationSrv)
{
}

CHttpOptSrv::~CHttpOptSrv()
{
}

REGISTER_SERVICE(CHttpOptSrv);

}

