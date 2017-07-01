
/* author : limingfan
 * date : 2015.08.03
 * description : 充值管理
 */

#ifndef CRECHARGE_MANAGER_H
#define CRECHARGE_MANAGER_H

#include <unordered_map>

#include "CMallMgr.h"
#include "service_common_module.pb.h"
#include "base/MacroDefine.h"
#include "SrvFrame/CLogicHandler.h"


using namespace NFrame;

namespace NProject
{

// 消息处理者类型
enum EMessageHandlerType
{
	EClientGameMall = 1,                           // 获取游戏商城配置
	EClientGetRechargeFishCoinOrder = 2,           // 获取充值渔币订单
	EClientCancelRechargeFishCoinOrder = 3,        // 取消充值渔币订单
	EClientRechargeFishCoinResultNotify = 4,       // 充值渔币结果通知
	EClientExchangeGameGoods = 5,                  // 兑换商城物品
	EClientGetSuperValuePackageInfo = 6,           // 获取超值礼包信息
	EClientReceiveSuperValuePackage = 7,           // 领取超值礼包物品
	EClientGetFirstPackageInfo = 8,                // 获取首冲礼包信息
	EClientGetExchangeGoodsConfig = 9,             // 领取实物兑换信息列表
	EClientExchangePhoneFare = 10,                 // 兑换手机话费额度
	EClientCheckAppleRechargeResult = 11,          // 苹果版本充值结果验证
};

// 应用层监听协议的标志
enum EListenerProtocolFlag
{
	ERechargeResultNotify = 1,                     // 充值结果通知
};

typedef std::unordered_map<unsigned short, unsigned short> MessageRequestToReplyID;  // 请求&应答消息ID

typedef std::unordered_map<unsigned short, MsgHandler> ProtocolIdToHandler;          // 协议ID&协议处理者


// 充值管理
class CRechargeMgr : public NFrame::CHandler
{
public:
	CRechargeMgr();
	~CRechargeMgr();
	
public:
    // 使用充值操作API之前一定要先初始化和设置商城数据，否则直接错误！
    bool init(NFrame::CLogicHandler* msgHandler);
	bool setMallData(const char* data, const unsigned int len);
	int getMallData(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen);
	
	bool setClientMessageHandler(const unsigned short msgHandlerType, const unsigned short reqId, const unsigned short rspId);
	int addProtocolHandler(const unsigned short protocolId, ProtocolHandler handler, CHandler* instance);

public:
    // 第三方平台充值操作
	bool getThirdRechargeTransaction(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen);
	bool getThirdRechargeTransactionReply(const char* data, const unsigned int len, const char* userData, unsigned short protocolId, ConnectProxy* conn);
	bool thirdRechargeTransactionNotify(const char* data, const unsigned int len, const char* userData, unsigned short protocolId, ConnectProxy* conn, int& result);
	bool cancelThirdRechargeNotify(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen);

	
public:
    // 当乐平台充值操作
	bool getDownJoyRechargeTransaction(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen, unsigned short protocolId, ConnectProxy* conn);
	bool getDownJoyRechargeTransactionReply(const char* data, const unsigned int len, unsigned short protocolId, ConnectProxy* conn);
	bool downJoyRechargeTransactionNotify(const char* data, const unsigned int len, unsigned short protocolId, ConnectProxy* conn);
	bool cancelDownJoyRechargeNotify(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen);
	
public:
	//腾讯平台充值操作
	bool findTxRechargeSuccess(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen);

public:
    // 海外Google充值操作
	bool getGoogleRechargeTransaction(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen, unsigned short protocolId, ConnectProxy* conn);
	bool getGoogleRechargeTransactionReply(const char* data, const unsigned int len, unsigned short protocolId, ConnectProxy* conn);
	bool checkGoogleRechargeResult(const char* data, const unsigned int len, const char* userData, const unsigned int userDataLen);
	bool checkGoogleRechargeResultReply(const char* data, const unsigned int len, unsigned short protocolId, ConnectProxy* conn, int& result);
	

private:
    // 新版本渔币充值
	void rechargeFishCoin(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void rechargeFishCoinReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void cancelRechargeFishCoin(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void rechargeFishCoinResultNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 苹果版本充值验证
	void checkAppleRecharge(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	
private:
    // 新版本游戏商城配置
	void getGameGoodsMallCfg(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getGameGoodsMallCfgReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 兑换商城物品
	void exchangeGameGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void exchangeGameGoodsReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取首冲礼包信息
	void getFirstRechargeInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getFirstRechargeInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	/*
	// 获取超值礼包信息
	void getSuperValuePackageInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getSuperValuePackageInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 领取超值礼包物品
	void receiveSuperValuePackageGift(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void receiveSuperValuePackageGiftReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
    */
	
	// 新版本实物兑换信息配置
	void getExchangeMallCfg(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getExchangeMallCfgReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 兑换手机话费额度
	void exchangePhoneFare(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void exchangePhoneFareReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	int sendMessageToHttpService(const char* data, const unsigned int len, unsigned short protocolId, const char* userData, const unsigned int userDataLen, const unsigned int platformType = 0);
	int sendMessageToCommonDbProxy(const char* data, const unsigned int len, unsigned short protocolId, const char* userData, const unsigned int userDataLen);
	
	bool parseMsgFromBuffer(::google::protobuf::Message& msg, const char* buffer, const unsigned int len, const char* info);
	const char* serializeMsgToBuffer(::google::protobuf::Message& msg, const char* info);
	
private:
    char m_msgBuffer[NFrame::MaxMsgLen];
	NFrame::CLogicHandler* m_msgHandler;
	CMallMgr m_mallMgr;
	
	MessageRequestToReplyID m_request2replyId;
	ProtocolIdToHandler m_protocolId2Handler;

	
DISABLE_COPY_ASSIGN(CRechargeMgr);
};

}

#endif // CRECHARGE_MANAGER_H
