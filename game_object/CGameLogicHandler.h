
/* author : limingfan
 * date : 2018.01.05
 * description : 消息处理辅助类
 */
 
#ifndef __CGAME_LOGIC_HANDLER_H__
#define __CGAME_LOGIC_HANDLER_H__

#include "SrvFrame/CModule.h"
#include "GameDataDefine.h"


using namespace NCommon;
using namespace NFrame;

namespace NPlatformService
{

class CGameMsgHandler;

class CGameLogicHandler : public NFrame::CHandler
{
public:
	CGameLogicHandler();
	~CGameLogicHandler();

public:
	int init(CGameMsgHandler* pMsgHandler);
	void unInit();
    void updateConfig();
	
public:
    // 发送赢家公告
    void sendWinnerNotice(const char* hallId, const string& username, const string& noticeMsg,
	                      const StringVector& matchingWords, const vector<unsigned int>& objColour);

private:
    void setInvitationStatus(const SRoomData& roomData, google::protobuf::RepeatedPtrField<com_protocol::InvitationInfo>* invitationInfoList);
	
	void onPlayerDismissRoom(unsigned int timerId, int userId, void* param, unsigned int remainCount);

private:
	// 邀请玩家列表
    void getInvitationList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getInvitationListReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 最近一起游戏的玩家列表
	void getLastPlayerList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getLastPlayerListReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	// 玩家邀请玩家一起游戏（邀请源端）
	void playerInvitePlayerPlay(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 玩家收到邀请一起游戏的通知（邀请目标端）
	void playerReceiveInvitationNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 拒绝玩家邀请通知（拒绝源端）
	void refusePlayerInvitationNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 玩家收到拒绝邀请通知（拒绝目标端）
	void playerReceiveRefuseInvitationNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 玩家主动发起请求解散房间（房卡模式）
	void playerAskDismissRoom(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void playerAnswerDismissRoom(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
    // 收到物品通知
	void giveGoodsNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

    // 玩家聊天
	void playerChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);


private:
	CGameMsgHandler* m_msgHandler;
};

}


#endif // __CGAME_LOGIC_HANDLER_H__
