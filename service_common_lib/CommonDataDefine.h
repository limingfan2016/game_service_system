
/* author : limingfan
 * date : 2017.11.17
 * description : 公共数据定义
 */
 
#ifndef __COMMON_DATA_DEFINE_H__
#define __COMMON_DATA_DEFINE_H__

#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>

#include "SrvFrame/UserType.h"

using namespace std;


namespace NProject
{

// 所有游戏与客户端间的固定消息协议ID，范围[0 --- 1000]
enum EClientGameCommonProtocolId
{
	// 玩家进入房间
	COMM_PLAYER_ENTER_ROOM_REQ = 1,
	COMM_PLAYER_ENTER_ROOM_RSP = 2,
	COMM_PLAYER_ENTER_ROOM_NOTIFY = 3,
	
	// 关闭重复进入房间通知到客户端
	COMM_CLOSE_REPEATE_CONNECT_NOTIFY = 4,
	
	// 玩家离开房间
	COMM_PLAYER_LEAVE_ROOM_REQ = 5,
	COMM_PLAYER_LEAVE_ROOM_RSP = 6,
	COMM_PLAYER_LEAVE_ROOM_NOTIFY = 7,
	
	// 玩家在房间中坐下位置
	COMM_PLAYER_SIT_DOWN_REQ = 8,
	COMM_PLAYER_SIT_DOWN_RSP = 9,
	COMM_PLAYER_SIT_DOWN_NOTIFY = 10,
	
	// 玩家点击准备游戏
	COMM_PLAYER_PREPARE_GAME_REQ = 11,
	COMM_PLAYER_PREPARE_GAME_RSP = 12,
	COMM_PLAYER_PREPARE_GAME_NOTIFY = 13,
	
	// 断线重连，重入游戏房间
	COMM_PLAYER_REENTER_ROOM_REQ = 14,
	COMM_PLAYER_REENTER_ROOM_RSP = 15,
	
	// 更新玩家状态通知
	COMM_UPDATE_PLAYER_STATUS_NOTIFY = 16,
	
	// 客户端&服务器心跳消息，客户端收到服务器消息，需要回复一样的心跳消息
	COMM_SERVICE_HEART_BEAT_NOTIFY = 17,
	
	// 游戏房间中没有在座的玩家，此时房间里的所有玩家（旁观者）需退出房间，房间已使用完毕
	COMM_GAME_ROOM_FINISH_NOTIFY = 18,
	
	// 无效连接、未验证通过的连接、重复频繁发消息、作弊、服务重启
	// 踢玩家下线、同一条连接重复登录、非法消息 等等则通知客户端玩家退出游戏
	// 客户端收到此消息，玩家必须退出服务
	// 服务器发送该消息之后，主动关闭连接
	COMM_FORCE_PLAYER_QUIT_NOTIFY = 19,

	// 1、房间内玩家聊天发送消息
	// 2、服务器向客户端玩家推送消息
	COMM_SEND_MSG_INFO_NOTIFY = 20,
	
	// 获取邀请玩家列表
	COMM_GET_INVITATION_LIST_REQ = 21,
	COMM_GET_INVITATION_LIST_RSP = 22,
	
	// 获取最近的游戏玩家列表
	COMM_GET_LAST_PLAYER_LIST_REQ = 23,
	COMM_GET_LAST_PLAYER_LIST_RSP = 24,
	
	// 邀请玩家参与游戏
	// 1、源玩家发出通知邀请
	// 2、目标玩家收到通知邀请
	COMM_INVITE_PLAYER_PLAY_NOTIFY = 25,
	
	// 玩家拒绝邀请通知
	// 1、源拒绝玩家发出通知
	// 2、被拒绝玩家收到通知
	COMM_PLAYER_REFUSE_INVITE_NOTIFY = 26,
	
	// 玩家更换房间
	COMM_PLAYER_CHANGE_ROOM_REQ = 27,
	COMM_PLAYER_CHANGE_ROOM_RSP = 28,
	COMM_PLAYER_CHANGE_ROOM_NOTIFY = 29,
	
	// 通知客户端开始游戏
	COMM_START_GAME_NOTIFY = 30,
	
	// 玩家手动开始游戏
	COMM_PLAYER_START_GAME_REQ = 31,
	COMM_PLAYER_START_GAME_RSP = 32,
	
	// 玩家解散房间
	COMM_PLAYER_DISMISS_ROOM_REQ = 33,
	COMM_PLAYER_DISMISS_ROOM_RSP = 34,
	COMM_PLAYER_DISMISS_ROOM_ASK_NOTIFY = 35,
	COMM_PLAYER_DISMISS_ROOM_ANSWER_NOTIFY = 36,
	COMM_PLAYER_CONFIRM_DISMISS_ROOM_NOTIFY = 37,
	COMM_PLAYER_CANCEL_DISMISS_ROOM_NOTIFY = 38,
	
	// 准备下一局游戏通知（房卡场）
	COMM_PREPARE_GAME_NOTIFY = 39,
	
	// 通知客户端结束游戏
	COMM_FINISH_GAME_NOTIFY = 40,
	
	// 倒计时通知
	COMM_TIME_OUT_SECONDS_NOTIFY = 41,
	
	// 游戏准备按钮状态通知
	COMM_PREPARE_STATUS_NOTIFY = 42,
	
	// 通知玩家可以手动开始游戏（房卡手动开桌）
	COMM_MANUAL_START_GAME_NOTIFY = 43,
	
	// 房卡大结算通知
	COMM_ROOM_TOTAL_SETTLEMENT_NOTIFY = 44,
};


// 服务内部公共的消息协议ID，范围[0 --- 1000]
enum EServiceCommonProtocolId
{
	// 关闭重复的连接，用户重复登录了
	// 所有服务包括大厅和游戏服务收到该消息则对应的用户必须退出服务
	SERVICE_CLOSE_REPEATE_CONNECT_NOTIFY = 1,
	
	// 管理员给玩家赠送物品通知大厅&游戏服务
	SERVICE_MANAGER_GIVE_GOODS_NOTIFY = 2,
	
	// 游戏服务系统公告消息通知
	SERVICE_GAME_MSG_NOTICE_NOTIFY = 3,
	
	// 玩家收到其他游戏玩家邀请一起游戏通知
	SERVICE_PLAYER_INVITATION_NOTIFY = 4,
	
	// 玩家收到拒绝邀请通知
	SERVICE_REFUSE_INVITATION_NOTIFY = 5,
};


// 服务内部公共的自定义应答协议ID值
// 其值必须大于 MaxProtocolIDCount = 8192
enum EServiceCommonReplyProtocolId
{
	SERVICE_COMMON_MIN_PROTOCOL_ID = 10000,                  // 服务自定义应答协议最小ID值

	SERVICE_COMMON_GET_USER_INFO_FOR_ENTER_ROOM = 10001,     // 用户进入房间获取个人信息
	SERVICE_COMMON_GET_ROOM_INFO_FOR_ENTER_ROOM = 10002,     // 用户进入房间获取房间信息
};


// 服务公共错误码
enum EServiceCommonErrorCode
{
	SrvOptSuccess = 0,					      // 操作成功

	// 不能和基础库错误码范围[0 -- 300] & 各个服务的错误码范围[1001 开始]冲突
	// 因此服务的公共错误码从901开始
	SrvOptFailed = 901,   			          // 操作失败

	InvalidParameter = 902,                   // 无效参数
	ServiceNoFoundConnectProxy = 903,         // 服务找不到连接代理
	ParseMessageFromArrayError = 904,         // 消息解码失败
	SerializeMessageToArrayError = 905,       // 消息编码失败
};


// 玩家在游戏服务房间里的状态
enum EGamePlayerStatus
{
	EInRoom = 0,              // 玩家在房间中
	EPlayRoom = 1,            // 玩家正在游戏中
	EDisconnectRoom = 2,      // 玩家掉线了
	ELeaveRoom = 3,           // 玩家离开房间了

	ERoomDisband = 100,       // 房间已经解散了
};

// 物品类型
enum EGameGoodsType
{
	// 游戏物品
	EGoodsRMB = 1,            // 人民币对应的货币
	EGoodsGold = 2,           // 游戏金币
	EGoodsRoomCard = 3,       // 游戏房卡
	EMaxGameGoodsType,        // 最大游戏物品类型
	
	// 商城物品
	EMallBeer = 101,          // 啤酒
	EMallStone = 102,         // 石头
	EMallBomb = 103,          // 炸弹
	EMallFlower = 104,        // 鲜花
	EMallSoap = 105,          // 肥皂
	EMaxMallGoodsType,        // 最大商城物品类型
	
	// 其他类型
	EWinLoseResult = 1001,    // 输赢结果
};

// 服务操作类型
enum EServiceOperationType
{
	EClientRequest = 1,       // 客户端请求消息
};


// 第三方平台类型
enum EThirdPlatformType
{
    EClientGameWeiXin = 1, 			  // 游戏C端微信平台类型
	EBusinessManagerWeiXin = 2,       // 商务B端微信平台类型

	EClientGameMobilePhone = 3,       // 游戏C端手机登录类型
	EBusinessManagerMobilePhone = 4,  // 商务B端手机登录类型
};


// 服务类型
enum ServiceType
{
	CommonSrv = NFrame::CommonServiceType,                  // 公共类型
	
    // 平台服务类型
	GameHallSrv = 1,                                        // 游戏大厅
	DBProxySrv = 2,                                         // DB代理服务
	MessageCenterSrv = 3,	                                // 消息中心服务
	OperationManageSrv = 4,		                            // 运营管理服务
	GatewaySrv = NFrame::GatewayServiceType,	            // 网关服务
	HttpOperationSrv = 6,                                   // Http操作服务
    OutsideClientSrv = NFrame::OutsideClientServiceType,    // 外部客户端服务类型
	
    // 游戏服务类型
	CattlesGame = 11,                                       // 牛牛游戏
	GoldenFraudGame = 12,                                   // 炸金花游戏
	FightLandlordGame = 13,                                 // 斗地主游戏
};

// 服务ID的倍数基值，即服务 ServiceType = ServiceID / ServiceIdBaseValue
static const unsigned int ServiceIdBaseValue = 1000;

/*
// 逻辑数据类型 
enum ELogicDataType
{
	HALL_LOGIC_DATA = 1,             // 1：大厅
	BUYU_LOGIC_DATA = 2,             // 2：捕鱼
	DBCOMMON_LOGIC_DATA = 3,         // 3：DB common 代理
	HALL_BUYU_DATA = 4,              // 4：大厅 & 捕鱼
	BUYU_COMMON_DATA = 5,            // 5：捕鱼 & CommonDB数据
	FF_CHESS_DATA = 6,               // 6：翻翻棋
	FFCHESS_COMMON_DATA = 7,         // 7：翻翻棋 & CommonDB数据
};

// 机器人类型
enum ERobotType
{
    ERobotPlatformType = 1000,       // 普通机器人
    EVIPRobotPlatformType = 2000,    // VIP机器人
};
*/


// static const unsigned int LogicDataMaxLen = 1024 * 215;
static const unsigned int MillisecondUnit = 1000;                     // 秒转换为毫秒乘值
static const unsigned int MinuteSecondUnit = 60;                      // 分钟转换为秒乘值
static const unsigned int OneDaySeconds = 1 * 24 * 60 * 60;           // 一天的秒数

static const unsigned int IdMaxLen = 32;                              // 各类ID的最大长度
static const unsigned int PortraitIdMaxLen = 256;                     // 用户头像ID最大长度（微信的玩家头像链接ID比较长，100个字符以上）
static const unsigned int NumberStrMaxLen = 16;                       // 数字最大长度

typedef char NumberStr[NumberStrMaxLen];                              // 数字字符串

typedef char RecordIDType[IdMaxLen];                                  // 记录ID


typedef vector<unsigned int> UIntVector;                              // unsigned int 数组

typedef vector<string> StringVector;                                  // string 数组
typedef vector<string*> StringPointerVector;                          // string* 数组
typedef vector<const string*> ConstStringPointerVector;               // const string* 数组
typedef vector<char*> CharPointerVector;                              // char* 数组
typedef vector<const char*> ConstCharPointerVector;                   // const char* 数组

typedef list<string> StringInList;                                    // string 链表

typedef unordered_map<unsigned int, unsigned int> UIntUnorderedMap;   // unsigned int key & value 无序 map
typedef map<unsigned int, unsigned int> UIntKeyValueMap;              // unsigned int key & value 有序 map

typedef unordered_map<string, string> StringUnorderedMap;             // string key & value 无序 map
typedef map<string, string> StringKeyValueMap;                        // string key & value 有序 map


// 扑克牌数值定义
static const unsigned int AllPokerCardCount = 54;
static const unsigned int BasePokerCardCount = 52;
static const unsigned int PokerCardBaseValue = 16;
static const unsigned char AllPokerCard[AllPokerCardCount] =
{
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	// 方块 A - K
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	// 梅花 A - K
	0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	// 红桃 A - K
	0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	// 黑桃 A - K
	0x4E,0x4F,															// 小王、大王
};


// 玩家聊天限制信息
struct SUserChatInfo
{
	unsigned int chatSecs;                                              // 上一次聊天的时间点
	unsigned int chatCount;                                             // 间隔时间内已经发送聊天消息的次数
	
	SUserChatInfo() : chatSecs(0), chatCount(0) {};
	~SUserChatInfo() {};
};
typedef unordered_map<string, SUserChatInfo> UserChatInfoMap;           // 聊天玩家ID映射聊天限制信息


}


#endif // __COMMON_DATA_DEFINE_H__

