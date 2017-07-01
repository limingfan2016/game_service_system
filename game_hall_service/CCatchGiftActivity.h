
/* author : limingfan
 * date : 2017.03.15
 * description : 福袋活动
 */
 
#ifndef _CCATCH_GIFT_ACTIVITY_H_
#define _CCATCH_GIFT_ACTIVITY_H_

#include <string>
#include <vector>
#include <unordered_map>

#include "base/MacroDefine.h"
#include "SrvFrame/CModule.h"
#include "TypeDefine.h"
#include "_HallConfigData_.h"


using namespace NCommon;
using namespace NFrame;

namespace NPlatformService
{

// 范围标识
enum ECGARangeFlag
{
	ECGA_StageTime = 1,
	ECGA_StageGoldCount = 2,
	ECGA_StageScore = 3,
	ECGA_StageRedPacket = 4,
	ECGA_StageBomb = 5,
	ECGA_StageReduceTime = 6,
	ECGA_StageFreezeTime = 7,
};

typedef std::vector<unsigned int> SCGAGoldNumberVector;  // 掉落金币的数量列表

struct SCGAStageRangeValue
{
	unsigned int dropGoldTimes;                   // 不同阶段掉落金币的次数
	unsigned int timeSecs;                        // 不同阶段的时间间隔，单位秒
	
	SCGAStageRangeValue() : dropGoldTimes(0), timeSecs(0) {};
	SCGAStageRangeValue(unsigned int _dropGoldTimes, unsigned int _timeSecs) : dropGoldTimes(_dropGoldTimes), timeSecs(_timeSecs) {};
	~SCGAStageRangeValue() {};
};
typedef std::vector<SCGAStageRangeValue> SCGAStageRangeValueVector;   // 不同阶段的数据值

struct SCGADropGoods
{
	unsigned int stage;                           // 阶段索引值
	unsigned int type;                            // 物品类型
	unsigned int time;                            // 物品开始掉落的时间点，防玩家外挂作弊行为
	
	SCGADropGoods() : stage(0), type(0), time(0) {};
	SCGADropGoods(unsigned int _stage, unsigned int _type, unsigned int _time) : stage(_stage), type(_type), time(_time) {};
	~SCGADropGoods() {};
};
typedef std::unordered_map<unsigned int, SCGADropGoods> SCGADropGoodsMap;  // 物品ID&物品信息

struct SCGAUserData
{
	unsigned int vipLevel;                        // 用户的VIP等级
	
	unsigned int stageIndex;                      // 当前阶段索引值
	SCGAStageRangeValueVector stageValue;         // 不同阶段的数据值
	
	unsigned int goldNumberIndex;                 // 掉落金币数量索引值
	SCGAGoldNumberVector goldNumbers;             // 整个活动准备掉落的金币数量列表
	
	unsigned int finishTimeSecs;                  // 活动结束时间点，活动时间可能由于时间炸弹而提前结束
	unsigned int timerId;                         // 定时发送掉落物品的定时器ID
	unsigned int timerInterval;                   // 定时器时间间隔，单位毫秒
	
	unsigned int dropGoldCount;                   // 当前阶段已经送出去掉落的金币个数
	unsigned int nextDropScore;                   // 下一次掉落积分券需要的金币数量点（已经掉落的金币个数满足的数量条件）
	unsigned int nextDropRedpacket;               // 下一次掉落红包需要的金币数量点（已经掉落的金币个数满足的数量条件）
	unsigned int nextDropBomb;                    // 下一次掉落炸弹需要的金币数量点（已经掉落的金币个数满足的数量条件）
	
	unsigned int amountGold;                      // 获得的总金币数量
	unsigned int amountScore;                     // 获得的总积分券数量

    unsigned int dropGoodsIdIndex;                // 掉落物品ID的索引值
	SCGADropGoodsMap amountDropGoods;             // 所有已经送出去掉落的物品列表
	
	unsigned int freezeFinishTimeSecs;            // 冷冻炸弹冷却结束时间点，冷却期间不掉落物品
	
	// 防玩家外挂作弊行为
	unsigned int intervalTimeSecs;                // 时间间隔点
	unsigned int hitCount;                        // 时间间隔段内已经命中的物品个数
};

typedef std::unordered_map<std::string, SCGAUserData> SCGAUserDataMap;  // 用户ID&用户活动信息
typedef std::unordered_map<unsigned int, std::string> SCGAUserIdMap;    // 用户ID列表信息

struct SCGAData
{
	unsigned int userIdIndex;                     // 参与活动的用户ID索引值
	SCGAUserDataMap userData;                     // 参与活动的用户数据
	SCGAUserIdMap userIds;                        // 参与活动的用户ID列表
	
	SCGAData() : userIdIndex(0) {};
};


class CSrvMsgHandler;


class CCatchGiftActivity : public NFrame::CHandler
{
public:
	CCatchGiftActivity();
	~CCatchGiftActivity();

public:
	void load(CSrvMsgHandler* msgHandler);
	void unLoad();
	
	void onLine(ConnectUserData* ud, ConnectProxy* conn);
	void offLine(ConnectUserData* ud);
	
private:
	// 进入福袋活动
	void enterCatchGiftActivity(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void enterCatchGiftActivityReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 获取福袋活动时间
	void getCatchGiftActivityTime(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	void getCatchGiftActivityTimeReply(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
	// 碰撞命中福袋活动掉落的物品
	void hitCatchGiftActivityDropGoods(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);
	
private:
    void update(com_protocol::HallLogicData* hallLogicData);
	
	int createCGAUserData(const char* username, const unsigned int vipLevel, const HallConfigData::CatchGiftActivityCfg& cgaCfg, SCGAUserData& cgaUserData);
	
	int startActivity(const char* username, const HallConfigData::CatchGiftActivityCfg& cgaCfg, SCGAUserData& cgaUserData);
	
	int initStageValue(const unsigned int userId, const HallConfigData::CatchGiftActivityCfg& cgaCfg, SCGAUserData& cgaUserData);
	
	void dropGoodsTimer(unsigned int timerId, int userId, void* param, unsigned int remainCount);
	
	bool dropGoods(const unsigned int userId);
	
	// 结束活动
	void finishActivity(unsigned int userId, const char* info = "");
	void finishActivity(const char* username, const unsigned int uLen, const char* info = "");
	void doFinishActivity(const char* username, const unsigned int uLen, const char* info = "");
	
	// 获取活动数据
	SCGAUserData* getUserActivityData(const char* username);
	SCGAUserData* getUserActivityData(const unsigned int userId);
	
	// 清空数据
    void clearUserData(const char* username);
	void clearUserData(const unsigned int userId);
	
private:
	CSrvMsgHandler* m_msgHandler;
	
	SCGAData m_catchGiftActivityData;
	

DISABLE_COPY_ASSIGN(CCatchGiftActivity);
};

}

#endif // _CCATCH_GIFT_ACTIVITY_H_
