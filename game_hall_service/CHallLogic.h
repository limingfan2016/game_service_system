
/* author : limingfan
 * date : 2015.04.23
 * description : 大厅逻辑服务实现
 */

#ifndef CHALL_LOGIC_H
#define CHALL_LOGIC_H

#include <string>
#include <unordered_map>

#include "base/MacroDefine.h"
#include "SrvFrame/CGameModule.h"
#include "SrvFrame/CDataContent.h"
#include "common/CClientVersionManager.h"
#include "TypeDefine.h"
#include "CLogicHandler.h"
#include "_HallConfigData_.h"


namespace NPlatformService
{

static const unsigned int DataContentSize = 64;
static const unsigned int DataContentCount = 512;


// 消息操作类型
enum OperationType
{
	FriendDonateGold = 1,            // 好友捐金币
};

// 好友操作
enum HallDataFriendOpt
{
	AddFriend = 0,
	RemoveFriend = 1,
	GetFriendDynamicData = 2,
	AddFriendNotifyClient = 3,
	GetDynamicDataForRemoveOther = 4,
	DonateGoldToFriend = 100,            // 好友捐送金索引最低开始值
};

struct FriendOpt
{
	FriendOpt() : srvId(0) {};
	FriendOpt(const std::string& uname, unsigned int srvid) : username(uname), srvId(srvid) {};
	
	std::string username;
	unsigned int srvId;
};
typedef std::unordered_map<std::string, FriendOpt> FriendOptContext;

typedef std::unordered_map<std::string, unsigned int> GetFriendDynamicDataContext;

// 好友捐送金币
struct FriendDonateGoldData
{
	char userName[MaxConnectIdLen];
	unsigned int gold;
	unsigned int notifySrvId;
};


enum UserAccountNumber
{
    DownJoyPlatform = 1,	
};

// 当乐平台返回码
enum DownJoyCode
{
    DownJoySuccess = 2000,
    DownJoyInvalid	= 2,
};

// 当乐平台接口信息
struct DownJoyInfo
{
	DownJoyInfo() {};
	DownJoyInfo(const std::string& _name, const std::string& _umid, unsigned int _ost) : name(_name), umid(_umid), osType(_ost) {};

    std::string name;
	std::string umid;
	unsigned int osType;
};
typedef std::unordered_map<std::string, DownJoyInfo> ConnectId2DownJoyInfo;

struct DownJoyCheck
{
	DownJoyCheck() {};
	DownJoyCheck(int _result, int _valid) : result(_result), valid(_valid) {};
	
	int result;
	int valid;
};
typedef std::unordered_map<std::string, DownJoyCheck> ConnectId2DownJoyCheck;


class CSrvMsgHandler;


// 客户端版本管理
class CVersionManager : public NProject::CClientVersionManager
{
private:
	virtual int getVersionInfo(const unsigned int deviceType, const unsigned int platformType, const string& curVersion, unsigned int& flag, string& newVersion, string& newFileURL);
};


// 大厅逻辑处理
class CHallLogic : public NFrame::CHandler
{
public:
	CHallLogic();
	~CHallLogic();

public:

	//获取VIP的等级
	uint32_t getVIPLv(uint32_t rechargeMoney);
	
	//更新DB上的VIP等级
	void toDBUpdateVIPLv(const char *userName, int addVIPLv, int userFlag = 0);
	
	//生成分享ID
	void generateShareID(const char *pUserName);

	//获得分享奖励
	void getShareReward(const char *pUserName, com_protocol::ClientHallShareRsp &rsp);

    // 获取大厅数据
    bool getUserHallLogicData(const char* username, int opt, unsigned short srcProtocolId = 0, const char* cbData = NULL, const unsigned int cbDataLen = 0);
	
	void addFriendNotifyClient(const char* data, const unsigned int len);
	
private:
	void handleLoginVerf(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 登陆签到奖励
    void getLoginRewardList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void receiveLoginReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void addGoldReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 商城配置
	void getMallConfig(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getMallConfigReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getOfflineUserHallDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 根据平台类型获取用户名
	void getUserNameByPlatform(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getUserNameByPlatformReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// Facebook 用户验证
	void checkFacebookUser(const char* data, const unsigned int len, const com_protocol::GetUsernameByPlatformReq& req);
    void checkFacebookUserReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    // 好友操作
	void addFriend(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void removeFriend(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void removeFriendByClient(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void removeOtherFriend(const com_protocol::GetUserServiceIDRsp& getUserSrvIdRsp);
	void removeFriendReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 好友捐送金币
	void donateGoldToFriend(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void donateGoldToFriendNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void donateGoldToFriendReply(const com_protocol::RoundEndDataChargeRsp& addGoldRsp, const char* friendName, const unsigned int friendNameLen, const int indexKey);
	int sendDonateGoldToFriendNotify(FriendDonateGoldData* fdData, ConnectProxy* conn = NULL);
	void getSrvIdForDonateGold(const com_protocol::GetUserServiceIDRsp& getUserSrvIdRsp, const int indexKey);
	bool setDonateGoldToOfflineUser(com_protocol::HallLogicData& hallLogicData, const int indexKey);
	
	void getFriendDynamicData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getFriendStaticData(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getFriendStaticDataReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getFriendStaticDataById(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getUserSrvIdReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 聊天
private:
    void privateChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void privateChatReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void notifyPrivateChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void worldChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void worldChatReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void notifyWorldChat(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 系统消息通知
	void systemMessageNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void changePropReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void getUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void getManyUserInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//通知用户VIP信息
	void noticeUserVIPInfo(string &userName, uint32_t nVIPLv, uint32_t nRechargeAmount);

	//设置VIP配置信息
	void setVIPCfgInfo(com_protocol::ClientVIPInfoRsp &rsp);

	//领取VIP奖励
	void receiveVIPReward(const string &userName, uint32_t nVIPLv, const HallConfigData::config& vipCfg, com_protocol::HallLogicData* hallData);
	


	// 当乐平台API
private:
    void checkDownJoyUser(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void checkDownJoyUserReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void getDownJoyRechargeTransaction(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getDownJoyRechargeTransactionReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void downJoyRechargeTransactionNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void cancelDownJoyRechargeNotify(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

private:
	bool addFriendOpt(com_protocol::HallLogicData& hallLogicData, com_protocol::AddFriendReq& addFriendReq, unsigned int srcSrvId);
	void removeFriendByName(com_protocol::HallLogicData& hallLogicData, const ::std::string& dst_username);
	void removeFriendOpt(com_protocol::HallLogicData& hallLogicData, com_protocol::RemoveFriendReq& removeFriendReq, unsigned int srcSrvId);
	void transmitMsgToClient(const char* data, const unsigned int len, unsigned short protocolId, const char* info);
	
	int sendMessageToHttpService(const char* data, const unsigned int len, unsigned short protocolId);
	ConnectProxy* getConnectInfo(const char* logInfo, string& userName);

	void getUserVIPInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void getVIPReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	//获得公告请求
	void getNoticeReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//设置用户公告信息
	void setUserNoticeInfo(com_protocol::HallLogicData* hallLogicData, int theDay, ConnectProxy* conn);

	//发送公告提示
	void sendNoticeHint(int theDay, ConnectProxy* conn, com_protocol::HallLogicData* hallLogicData);

	//分享请求
	void handleShareReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//获取积分商城信息
	void handleGetScoresItemReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handleGetScoresItemRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	//积分兑换物品
	void handleExchangeScoresItemReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void handleExchangeScoresItemRsp(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//退出挽留
	void handleQuitRetainReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getGameDataForTaskReward(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	void handleGameAddPropReq(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	void resetUserOtherInfoReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	//设置用户分享信息
	void setUserShareInfo(com_protocol::HallLogicData* hallLogicData, int theDay);

	//设置用户积分信息
	void setUserScoreInfo(com_protocol::HallLogicData* hallLogicData, ConnectProxy* conn);

	//开启一个清空积分的定时器
	void openClearScoresTimer();

	//清空所有在线用户的积分
	void clearAllOnLineUserScores(unsigned int timerId, int userId, void* param, unsigned int remainCount);

	//清空用户的积分
	void clearUserScores(com_protocol::HallLogicData* hallLogicData, const char* userName, int uNLen);

public:
	void load(CSrvMsgHandler* msgHandler);
	void unLoad(CSrvMsgHandler* msgHandler);
	
	void onLine(ConnectUserData* ud, ConnectProxy* conn);
	void offLine(com_protocol::HallLogicData* hallLogicData);
	void updateConfig();
	
private:
	CSrvMsgHandler* m_msgHandler;
	GetFriendDynamicDataContext m_getFriendDynamicData;
	FriendOptContext m_removeFriendOptContext;
	FriendOptContext m_addFriendOptContext;
	CVersionManager m_versionManager;
	
	ConnectId2DownJoyInfo m_connId2DownJoyInfo;
	ConnectId2DownJoyCheck m_connId2DownJoyCheck;
	
	NFrame::CDataContent m_dataContent;
	int m_friendDonateGoldIndex;

	CHallLogicHandler m_hallLogicHandler;
	unsigned int	  m_noticeVersion;
	
	uint32_t		  m_clearScoresNum;				// 积分清空次数
	time_t			  m_clearScoresTime;			// 清空积分的时间
	unsigned int      m_clearScoreTimerId;          // 清空积分定时器ID


DISABLE_COPY_ASSIGN(CHallLogic);
};


}

#endif // CHALL_LOGIC_H
