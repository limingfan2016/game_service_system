
/* author : limingfan
 * date : 2016.08.11
 * description : 逻辑数据处理
 */
 
#ifndef _CLOGIC_LOGIC_HANDLER_TWO_H_
#define _CLOGIC_LOGIC_HANDLER_TWO_H_

#include "base/MacroDefine.h"
#include "SrvFrame/CModule.h"
#include "MessageDefine.h"
#include "_DbproxyCommonConfig_.h"
#include "_MallConfigData_.h"


class CSrvMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace NErrorCode;


// 邮件消息状态，0：未读， 1：已读未领取， 2：已读已领取
enum EMailMessageStatus
{
	EDeleteMessage = 0,                 // 删除消息操作码
	
	// 状态码
	EMessageUnRead = 0,                 // 未读消息
	EMessageReaded = 1,                 // 已读消息
	EMessageReceivedGift = 2,           // 已领取奖励消息
	ECheckUnReadMessage = 3,            // 查看是否存在未读邮件
};
	
	
class CLogicHandlerTwo : public NFrame::CHandler
{
public:
	CLogicHandlerTwo();
	~CLogicHandlerTwo();

public:
	int init(CSrvMsgHandler* msgHandler);
	void unInit();
	
	void updateConfig();

private:
    // 红包口令相关
	void addRedGiftReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void receiveRedGiftReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// PK场匹配写记录
	void writePKmatchingRecordReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 绑定兑换商城账号信息
	void bindingExchangeMallAccount(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 重置玩家兑换话费时绑定的手机号码
    void resetExchangePhoneNumber(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 添加邮件信息
	void addMailMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取邮件信息
	void getMailMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 操作邮件信息
	void optMailMessage(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取玩家财富相关信息
	void getWealthInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	void getFFChessMall(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void exchangeFFChessGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void useFFChessGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	void addActivityRedPacket(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getActivityRedPacket(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    void getRedGiftId(char* redGfitId, const unsigned int len);
	
	void setFFChessGoods(const unsigned int curSecs, const com_protocol::FFChessGoodsData* ffChessGoods, const MallConfigData::chess_goods& chessGoodsCfg,
						 const com_protocol::FFChessExchangeType type, com_protocol::FFChessItem* chessGoods);
	
	void setFFChessGiftPackage(const unsigned int curSecs, const com_protocol::FFChessGoodsData* ffChessGoods, const MallConfigData::chess_gift_package& chessGoodsCfg,
	                           com_protocol::FFChessGiftPackage* ffChessGiftPackage);
						 
	bool getFFChessGoodsInfo(const com_protocol::ExchangeFFChessItemReq& req, MallConfigData::chess_gift_package& giftPackage, com_protocol::ExchangeFFChessItemRsp& rsp);
	
	// 清空机器人积分券
	void clearRobotScore();
	
private:
	CSrvMsgHandler* m_msgHandler;
	

DISABLE_COPY_ASSIGN(CLogicHandlerTwo);
};

#endif // _CLOGIC_LOGIC_HANDLER_TWO_H_
