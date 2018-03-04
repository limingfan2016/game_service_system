
/* author : limingfan
 * date : 2017.11.26
 * description : 消息处理辅助类
 */
 
#ifndef __CLOGIC_HANDLER_H__
#define __CLOGIC_HANDLER_H__

#include "SrvFrame/CModule.h"
#include "BaseDefine.h"


class CMsgHandler;

using namespace NCommon;
using namespace NFrame;
using namespace NProject;


class CDBLogicHandler : public NFrame::CHandler
{
public:
	CDBLogicHandler();
	~CDBLogicHandler();

public:
	int init(CMsgHandler* pMsgHandler);
	void unInit();
    void updateConfig();

public:
    // 设置记录基本信息
    void setRecordBaseInfo(com_protocol::ChangeRecordBaseInfo& recordInfo, unsigned int srcSrvId,
						   const string& hallId, const string& roomId, int roomRate, int optType,
						   const RecordIDType recordId, const char* remark = "");
	// 道具变更记录
    int recordItemChange(const string& username, const google::protobuf::RepeatedPtrField<com_protocol::ItemChange>& goodsList,
						 const com_protocol::ChangeRecordBaseInfo& recordData, const com_protocol::ChangeRecordPlayerInfo& playerInfo);
						 
	void writeItemDataLog(const google::protobuf::RepeatedPtrField<com_protocol::ItemChange>& items,
                          const string& srcName, const string& dstName,
						  int optType, int result, const char* info = "");

	// 金币&道具&玩家属性等数量变更修改
    int changeUserGoodsValue(const string& hall_id, const string& username, google::protobuf::RepeatedPtrField<com_protocol::ItemChange>& goodsList, CUserBaseinfo& userBaseinfo,
                             const com_protocol::ChangeRecordBaseInfo& recordData, const com_protocol::ChangeRecordPlayerInfo& playerInfo);
								
private:
    int getInvitationPlayerInfo(const char* hallId, const char* username, const char* sql, unsigned int sqlLen, const unsigned int invitationCount,
                                google::protobuf::RepeatedPtrField<com_protocol::InvitationInfo>& invitations, int& currentIdx);
	
private:
	void changeItem(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void changeMoreUsersItem(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    void getInvitationList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void setLastPlayers(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getLastPlayerList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getHallPlayerList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    void getGameRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getDetailedGameRecord(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);


private:
	CMsgHandler* m_msgHandler;
	
	HallLastPlayerList m_hallLastPlayers;
};


#endif // __CLOGIC_HANDLER_H__
