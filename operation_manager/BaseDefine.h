
/* author : limingfan
 * date : 2017.09.15
 * description : 消息处理类
 */
 
#ifndef __BASE_DEFINE_H__
#define __BASE_DEFINE_H__

#include "../common/OperationManagerProtocolId.h"
#include "../common/DBProxyProtocolId.h"
#include "../common/MessageCenterProtocolId.h"
#include "../common/HttpOperationProtocolId.h"
#include "../common/OperationManagerErrorCode.h"
#include "../common/CommonType.h"
#include "../common/CServiceOperation.h"

#include "_NOptMgrConfig_.h"
#include "protocol/appsrv_operation_manager.pb.h"


using namespace NProject;


// 挂接在连接代理上的相关数据
struct OptMgrUserData : public ConnectProxyUserData
{
	unsigned short versionFlag;                // 客户端版本标识
	unsigned short platformType;               // 平台类型

	char nickname[IdMaxLen];                   // 用户昵称
	// char portraitId[PortraitIdMaxLen];         // 用户头像ID
	unsigned int level;                        // 等级

	char chessHallId[IdMaxLen];                // 所在棋牌室ID
};


// 棋牌室游戏类型信息
typedef unordered_map<unsigned int, com_protocol::BSHallGameInfo> GameTypeToInfoMap;

// 棋牌室信息
struct SChessHallData
{
	int yearDay;                               // 一年中的第几天，棋牌室信息每天全部重新更新一次
	
	bool needUpdatePlayer;                     // 是否需要刷新当前在线&离线人数，玩家所在的游戏服务重启时需要刷新
    unsigned int offilnePlayers;               // 当前离线人数

    unsigned int forbidPlayers;                // 当前被禁止的人数
    unsigned int applyPlayers;                 // 当前申请的人数
	
	bool needUpdateChessHallInfo;              // 是否需要刷新棋牌室信息
	com_protocol::BSChessHallInfo baseInfo;    // 棋牌室基本信息

	GameTypeToInfoMap gameInfo;                // 棋牌室游戏信息
	
	SChessHallData() : yearDay(-1), needUpdatePlayer(false), needUpdateChessHallInfo(false) {};
	~SChessHallData() {};
};
typedef unordered_map<string, SChessHallData> ChessHallIdToDataMap;  // 棋牌室信息列表


// 创建棋牌室，管理员填写验证手机号码
struct SPhoneNumberData
{
    unsigned int timeSecs;                   // 时间点
    string number;                           // 手机号码
    unsigned int code;                       // 随机验证码
    
    SPhoneNumberData() {};
	SPhoneNumberData(unsigned int ts, const string& mb, unsigned int cd) : timeSecs(ts), number(mb), code(cd) {};
	~SPhoneNumberData() {};
};
typedef unordered_map<string, SPhoneNumberData> ManagerToPhoneNumberDataMap;


#endif // __BASE_DEFINE_H__
