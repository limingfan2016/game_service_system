
/* author : limingfan
 * date : 2016.03.30
 * description : 逻辑数据处理
 */
 
#ifndef _CLOGIC_DATA_H_
#define _CLOGIC_DATA_H_

#include "base/MacroDefine.h"
#include "SrvFrame/CModule.h"
#include "MessageDefine.h"
#include "_DbproxyCommonConfig_.h"
#include "_MallConfigData_.h"


class CSrvMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace NErrorCode;


class CLogicData : public NFrame::CHandler
{
public:
	CLogicData();
	~CLogicData();

public:
	int init(CSrvMsgHandler* msgHandler);
	void unInit();
	
public:
	const ServiceLogicData& getLogicData(const char* userName, unsigned int len);
	ServiceLogicData& setLogicData(const char* userName, unsigned int len);
	
	const com_protocol::FFChessGoodsData& getFFChessGoods(const char* userName, unsigned int len);
	
public:
	void rechargeFishCoin(const com_protocol::UserRechargeReq& req);
	void exchangeBatteryEquipment(const char* userName, unsigned int len, const unsigned int id);
	bool fishCoinIsRecharged(const char* userName, const unsigned int len, const unsigned int id);
	
public:
	void exchangeFFChessGoods(const char* userName, unsigned int len, const unsigned int useDay, const com_protocol::ExchangeFFChessItemRsp& info);
	const ServiceLogicData& updateFFChessGoods(const char* userName, unsigned int len);
	
private:
    void saveLogicData();
    void setTaskTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount);  // 任务定时器
	
	void addFFChessGoods(const unsigned int chessItemId, const com_protocol::FFChessExchangeType type, const unsigned int useDay, com_protocol::FFChessGoodsData* ffChessData);
	
private:
    void setServiceLogicData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	CSrvMsgHandler* m_msgHandler;
	UserToLogicData m_user2LogicData;
	

DISABLE_COPY_ASSIGN(CLogicData);
};

#endif // _CLOGIC_DATA_H_
