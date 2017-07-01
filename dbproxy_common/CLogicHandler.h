
/* author : limingfan
* date : 2016.02.24
* description : 消息处理辅助类
*/
 
#ifndef __CLOGIC_HANDLER_H__
#define __CLOGIC_HANDLER_H__

#include <string>
#include "SrvFrame/CModule.h"
#include "db/CMySql.h"
#include "db/CMemcache.h"
#include "app_server_dbproxy_common.pb.h"
#include "base/CCfg.h"
#include "base/CLogger.h"
#include "MessageDefine.h"
#include "_DbproxyCommonConfig_.h"
#include "_MallConfigData_.h"


class CSrvMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace std;
using namespace NDBOpt;
using namespace NErrorCode;


// 兑换的游戏商品数据
typedef map<unsigned int, unsigned int> GoodsIdToNum;
struct GameGoodsData
{
	unsigned int coinCount;                     // 需要的源商品总数量，渔币或者钻石
	unsigned int coinType;                      // 源商品总类型，渔币或者钻石
	GoodsIdToNum id2num;                        // 兑换的目标商量类型及数量
};


class CLogicHandler : public NFrame::CHandler
{
public:
	CLogicHandler();
	~CLogicHandler();
	
public:
	int init(CSrvMsgHandler* pMsgHandler);
    void updateConfig();
	
public:
	const com_protocol::FishCoinInfo* getFishCoinInfo(const unsigned int platformId, const unsigned int id);
	
	// 获取服务的逻辑数据
    int getLogicData(const char* firstKey, const unsigned int firstKeyLen, const char* secondKey, const unsigned int secondKeyLen, ::google::protobuf::Message& msg);

	// 获取游戏或者大厅的逻辑数据
	void getLogicData(const com_protocol::GetUserBaseinfoReq& req, com_protocol::GetUserBaseinfoRsp& rsp);
	
	//获取兑换商品所需积分
	const MallConfigData::scores_item* getExchangeItemCfg(uint32 nItemId) const;
	
private:
    // 虚拟道具兑换
	void getGameMallCfg(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void exchangeGameGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 渔币信息
	void getMallFishCoinInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 话费、实物兑换
	void getExchangeMallCfg(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void exchangePhoneFare(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void exchangeGiftGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取公共配置
	void getGameCommonConfig(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取首冲礼包信息
    void getFirstRechargeInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// void receiveSuperValuePackageGift(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取积分商城信息
	void getScoresShopInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//兑换积分商品
	void exchangeScoresItem(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//聊天记录
	void chatLog(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void handlePKPlaySettlementOfAccountsReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void chatLogToMysql(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
private:
	bool getGameGoodsInfo(const com_protocol::ExchangeGameGoogsReq& msg, GameGoodsData& gameGoodsData);

	//获取指定积分商品的剩余数量
	uint32 getExchangeItemNum(uint32 nItemId);

	//兑换记录
	bool exchangeLog(uint32 nItemId);

	//还原兑换记录
	void reExchangeLog(const MallConfigData::ScoresShop &scoresShopCfg);

	typedef unordered_map<uint32, uint32>::value_type exchange_value;

private:
	CSrvMsgHandler* m_msgHandler;
	
	com_protocol::GetGameMallCfgRsp m_gameMallCfgRsp;
	com_protocol::GetExchangeInfoRsp m_exchangeInfoRsp;

	unsigned int					m_clearExchangeLogFlag;		//清空积分商品兑换记录标记
	unordered_map<uint32, uint32>	m_exchangeLog;				//积分商品兑换记录	
	string							m_chatLogSql;				//
};

#endif // __CLOGIC_HANDLER_H__
