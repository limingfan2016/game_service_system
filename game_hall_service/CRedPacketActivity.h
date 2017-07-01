
/* author : limingfan
 * date : 2017.02.15
 * description : 红包活动
 */
 
#ifndef _CRED_PACKET_ACTIVITY_H_
#define _CRED_PACKET_ACTIVITY_H_

#include <string>
#include <vector>
#include <unordered_map>

#include "base/MacroDefine.h"
#include "SrvFrame/CModule.h"
#include "TypeDefine.h"


using namespace NCommon;
using namespace NFrame;

namespace NPlatformService
{

// 活动状态
enum ERedPacketActivityStatus
{
	ERedPacketActivityWait = 1,     // 等待开始活动，倒计时中
	ERedPacketActivityOn = 2,       // 活动中，可以抢红包
	ERedPacketActivityOff = 3,      // 活动结束了
};

// 红包状态
enum ERedPacketStatus
{
	ENotReceiveRedPacket = 1,              // 还没有人领取该红包
	EOnReceiveRedPacket = 2,               // 正在领取红包中
	ESuccessReceivedRedPacket = 3,         // 已经成功领取了	
};

// 红包数据
struct SRedPacketInfo
{
	unsigned int type;
	unsigned int count;
	ERedPacketStatus status;
	
	SRedPacketInfo() : type(0), count(0), status(ERedPacketStatus::ESuccessReceivedRedPacket) {};
	SRedPacketInfo(unsigned int _type, unsigned int _count, ERedPacketStatus _status) : type(_type), count(_count), status(_status) {};
	~SRedPacketInfo() {};
};
typedef std::unordered_map<std::string, SRedPacketInfo> RedPacketIdToInfoMap;         // 红包ID映射红包信息

typedef std::unordered_map<std::string, int> OnReceiveRedPacketUsers;                 // 正在领取红包的玩ID及结果

// 领取了红包的用户信息
struct SRedPacketUserInfo
{
	std::string nickname;
	int portraitId;
	
	SRedPacketUserInfo() {portraitId = 0;};
	SRedPacketUserInfo(const std::string& _nickname, const int _portraitId) : nickname(_nickname), portraitId(_portraitId) {};
	~SRedPacketUserInfo() {};
};
typedef std::unordered_map<std::string, SRedPacketUserInfo> UserIdToInfoMap;          // 用户ID映射昵称


// 红包活动数据
struct SRedPacketActivityData
{
    com_protocol::GetActivityRedPacketDataRsp redPacketActivityData;          // 活动所有历史数据，用于查找红包数据
	UserIdToInfoMap winRedPacketUserId2Infos;                                 // 参与过抢红包的用户ID&用户昵称

    unsigned int lastFinishActivityId;                                        // 上一次结束了的活动ID
	
	unsigned int activityBeginTimeSecs;                                       // 活动开始时间点
	unsigned int activityEndTimeSecs;                                         // 活动结束时间点
	
	OnReceiveRedPacketUsers onReceiveRedPacketUsers;                          // 正在领取红包的玩家ID及结果
	RedPacketIdToInfoMap redPackets;                                          // 红包信息
	unsigned int bestGoldCount;                                               // 当前最佳红包的金币数量（其他类型换算成金币）
	unsigned int winRedPacketIndex;                                           // 正在被多个玩家同时抢的红包的索引值
	
	SRedPacketActivityData()
	{
		reset();
	}
	
	void reset(const unsigned int finishId = 0)
	{
		lastFinishActivityId = finishId;
		
		activityBeginTimeSecs = 0;
		activityEndTimeSecs = 0;
		
		onReceiveRedPacketUsers.clear();
		redPackets.clear();
		
		bestGoldCount = 0;
		winRedPacketIndex = 0;
	}
	
	void update(const unsigned int currentSecs)
	{
		if (currentSecs >= activityEndTimeSecs) reset(activityBeginTimeSecs);
	}
};

class CSrvMsgHandler;


class CRedPacketActivity : public NFrame::CHandler
{
public:
	CRedPacketActivity();
	~CRedPacketActivity();

public:
	void load(CSrvMsgHandler* msgHandler);
	void unLoad();
	
public:
    void modifyUserNickname(const char* username, const com_protocol::ModifyBaseinfoRsp& mdRsp);
	
private:
    // 获取活动红包数据信息
	void getActivityRedPacketReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取抢红包活动时间信息
	void getRedPacketActivityTimeInfo(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 打开抢红包活动UI界面
	void openRedPacketActivity(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 调用了打开抢红包活动UI界面之后
    // 玩家关闭UI页面是必须调用关闭活动消息接口通知服务器
	void closeRedPacketActivity(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	// 用户点击开抢红包
	void userClickRedPacket(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void userClickRedPacketReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 查看红包历史记录列表
	void viewRedPacketHistoryList(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 查看单个红包详细的历史记录信息
	void viewRedPacketDetailedHistory(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 查看个人红包历史记录
	void viewPersonalRedPacketHistory(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
	// 获取当前活动或者下一次活动信息
    const HallConfigData::RedPacketActivityInfo* getActivityData(const unsigned int curSecs, unsigned int& beginSecs, unsigned int& endSecs);

	bool getWinRedPacketInfo(const unsigned int activityId, com_protocol::ClientWinRedPacketInfo& info);
	
	int getWinRedPacketInfoIndex(const unsigned int activityId);
	
	const com_protocol::RedPacketActivityInfo* getWinRedPacketActivity(const unsigned int activityId);
	
	bool generateRedPackets(const unsigned int activityId, const HallConfigData::RedPacketActivityInfo* rpactCfgInfo, SRedPacketActivityData& rpaData);
	
	bool existRedPacket(const RedPacketIdToInfoMap& rpInfoMap, const unsigned int type, const unsigned int count);
	
	const std::string* winRedPacket(const RedPacketIdToInfoMap& rpInfoMap);
	
	ERedPacketActivityStatus getActivityStatus(const unsigned int activityId);
	
private:
	CSrvMsgHandler* m_msgHandler;

	SRedPacketActivityData m_redPacketActivityData;
	

DISABLE_COPY_ASSIGN(CRedPacketActivity);
};

}

#endif // _CRED_PACKET_ACTIVITY_H_
