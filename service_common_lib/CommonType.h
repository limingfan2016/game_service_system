
/* author : limingfan
 * date : 2015.03.18
 * description : 各服务公用的数据类型定义
 */

#ifndef CPROJECT_COMMON_TYPE_H
#define CPROJECT_COMMON_TYPE_H

#include <string>
#include <vector>

#include "base/Type.h"
#include "base/CCfg.h"

using namespace std;

namespace NProject
{

#pragma pack(1)  // 网络数据全部1字节对齐

// 协议操作返回码
enum ProtocolReturnCode
{
	Opt_Success = 0,                         // 操作成功
	Opt_Failed = -1,                         // 操作失败
	GetPriceFromMallDataError = -2,          // 从商城数据获取物品价格错误
};


// 第三方平台类型
enum ThirdPlatformType
{
	// 安卓设备类型
	AoRong = 0,                 // 奥榕公司
	DownJoy = 1,                // 当乐平台
	GoogleSDK = 2,              // Google SDK 充值
    YouWan = 3,                 // 有玩平台
    WanDouJia = 4,              // 豌豆荚平台
    XiaoMi = 5,                 // 小米平台
	JinNiu = 6,                 // 劲牛平台
    QiHu360 = 7,                // 360平台
	UCSDK = 8, 					// UC平台
    Oppo = 9,                   // Oppo平台
    MuMaYi = 10,                // 木蚂蚁
    SoGou = 11, 			    // 搜狗
    AnZhi = 12,                 // 安智
    MeiZu = 13, 				// 魅族
    TXQQ = 14,					// 应用宝QQ登录
    TXWX = 15, 					// 应用宝WX登录
    M4399 = 16,                 // 4399
	WoGame = 17,                // 联通沃商店
	YdMM = 18,                	// 移动MM
	LoveGame = 19,              // 爱游戏
	Nduo = 20,                  // N多市场
	Uucun = 21,                 // 悠悠村
	Huawei = 22,                // 华为
	Zhuoyi = 23,                // 卓易
	ZhuoyiTest = 24,		    // 卓易Test
	TT = 25,                    // TT语音
	Baidu = 26,                 // 百度
	YunZhongKu = 27,            // 云中酷
	Vivo = 28,                  // Vivo
	MiGu = 29,                  // 咪咕
	MoGuWan = 30,               // 蘑菇玩
	YiJie = 31,                 // 易接
	SanXing = 32,               // 三星
	HaiMaWan = 33,              // 海马玩
	AppleIOS = 34,              // 苹果
	HanYou = 35,                // 韩国合作平台
	ManTuo = 36,                // 曼拓cps
	HanYouFacebook = 37,        // 韩友Facebook
	MaxPlatformType,            // 最大值
	
	// 苹果设备类型值 = 安卓设备类型 * ClientDeviceBase + 安卓设备类型
	// 比如当乐平台苹果设备应用值为 101 = 1 * 100 + 1
	ClientDeviceBase = 1000,     // 客户端不同设备类型基值
};

// 客户端设备版本类型
enum ClientVersionType
{
	AndroidSeparate = 0,        // 安卓设备（大厅、游戏分开版本）
	AppleMerge = 1,             // 苹果设备
	AndroidMerge = 2,           // 安卓设备（大厅、游戏合并版本）
};

// 游戏商品类型
enum EGameGoodsType
{
	EGold = 0,                  // 金币
	EProp = 1,                  // 道具
	EFishCoin = 2,              // 渔币
};

// 物品类型ID值
enum EPropType
{
	PropGold = 1,				//金币
	PropFishCoin = 2,			//渔币
	PropTelephoneFare = 3,		//话费卡
	PropSuona = 4,				//小喇叭
	PropLightCannon = 5,		//激光炮
	PropFlower = 6,				//鲜花
	PropMuteBullet = 7,			//哑弹
	PropSlipper = 8,			//拖鞋
	PropVoucher = 9,			//奖券
	PropAutoBullet = 10,		//自动炮子弹
	PropLockBullet = 11,		//锁定炮子弹
	PropDiamonds = 12,			//钻石
	PropPhoneFareValue = 13,	//话费额度
	PropScores = 14,			//积分
	PropRampage = 15,			//狂暴
	PropDudShield = 16,         //哑弹盾，防护哑弹
	PropMax,
	
	
	// 其他定义
	EPKDayGoldTicket = 901,     // PK场全天金币对战门票ID类型
	EPKHourGoldTicket = 902,    // PK场限时金币对战门票ID类型
	PhoneCarkExchange = 1001,   // 话费卡兑换
	AllGoodsTypeMax,
};

//游戏记录类型
enum EGameRecordType
{
	//防止与现有道具ID重复
	RecordPkPlayScore = 2001,			//PK场分数
};

// 用户信息标识符
enum EUserInfoFlag
{
	EVipLevel = 201,            // VIP 等级
    ERechargeAmount = 202,      // 累积充值的总额度
	EUserNickname = 203,		// 用户昵称
	EUserPortraitId = 204,		// 用户头像
	EUserSex = 205,				// 用户性别
};


enum RankingDataType
{
	Gold = 1,                  // 金币排行
	Phone = 2,                 // 话费卡排行
};

// 商城物品展示标识
enum MallItemFlag
{
	NoFlag = 0,               // 不展示标识
	First = 1,                // 首冲
	Hot = 2,                  // 热销
	Worth = 3,                // 超值
	Discount = 4,             // 优惠
	
	// 首充物品标识
	FirstNoFlag = 100,        // 首冲&不展示标识
	FirstHot = 102,           // 首冲&热销
	FirstWorth = 103,         // 首冲&超值
	FirstDiscount = 104,      // 首冲&优惠
};

// 前端数值属性类型
enum EClientPropNumType
{
	enPropNumType_coin				= 0,	// 金币
	enPropNumType_phoneCard,				// 充值卡数
	enPropNumType_exchangeTicket,			// 兑换劵数
	enPropNumType_iconId,					// 头像
	enPropNumType_gender,					// 0:男性	1:女性
	enPropNumType_level,					// 等级
	enPropNumType_fishCoin,					// 鱼币
	enPropNumType_phoneValue,				// 话费额度
	enPropNumType_diamond,					// 钻石
	enPropNumType_vip,						// vip等级

	enPropNumType_max,
};

enum GameType
{
	ALL_GAME = 0,
	BUYU = 1,                                // 捕鱼
	XIANGQI_FAN_FAN = 2,                     // 象棋翻翻
	MAX_GAME_TYPE,
};

enum GameRecordType
{
	Game = GameType::ALL_GAME,
	Buyu = GameType::BUYU,
	BuyuExt = 2,
	Landlords = 3,
	MaxGameRecordType,
};

// 系统通知消息模式
enum ESystemMessageMode
{
	ShowDialogBoxAndRoll = 0,           // 0：系统对话框和跑马灯展示
	ShowDialogBox = 1,                  // 1：只在系统对话框展示
	ShowRoll = 2,                       // 2：只在跑马灯展示
	ShowWorldDialogBox = 3,             // 3：只在世界对话框展示
	ShowWorldDialogBoxAndRoll = 4,      // 4：世界对话框和跑马灯展示
};

// 系统通知消息模式
enum ESystemMessageType
{
    Group = 0,
	Person = 1,
};

// 兑换的目标物品大类型
enum ExchangeType
{
	EStrategy = 1,               // 1:策略类道具（渔币兑换）
	EInteract = 2,               // 2:互动类道具（钻石兑换）
	EGiftPackage = 3,            // 3:礼包类（渔币兑换）
	EBatteryEquip = 4,           // 4:炮台装备类型（渔币兑换）
	EFFChessGold = 5,            // 5:翻翻棋金币（渔币兑换）
};

// 公共配置大类型
enum ECommonConfigType
{
	EBatteryEquipCfg = 1,        // 1:炮台装备配置
};

// 服务操作类型
enum EServiceOperationType
{
	EClientRequest = 1,          // 客户端请求消息
};

// 游戏转发到大厅的消息通知类型
enum ELoginForwardMessageNotifyType
{
	EPlayerUpLevel = 1,          // 玩家等级升级
	EBatteryUpLevel = 2,         // 炮台解锁等级
	ERechargeValue = 3,          // 充值操作
	
	EAddRedGiftReward = 101,     // 添加红包奖励信息
};

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

// PK场次ID
enum EPKFieldID
{
	FieldIDNone = -1,

	FieldIDPhoneFare,		//话费场
	FieldIDVoucher,			//奖券场
	FieldIDDiamonds,		//钻石场
	FieldIDGold,			//金币场

	FieldIDMax,
};

// 道具变更记录信息
struct RecordItem
{
	unsigned int type;
	int count;
	
	RecordItem() {};
	RecordItem(unsigned int _type, int _count) : type(_type), count(_count) {};
};
typedef vector<RecordItem> RecordItemVector;

// 道具变更记录信息(多人)
struct MoreUserRecordItem
{
	string src_username;          // 源用户
	string dst_username;          // 目标用户
	string remark;                // 游戏记录信息
	RecordItemVector items;       // 变更的道具
};
typedef vector<MoreUserRecordItem> MoreUserRecordItemVector;



// 从redis获取到的大厅房间数据
static const int MaxKeyLen = 36;
static const char* HallKey = "Hall";
static const int HallKeyLen = strlen(HallKey);
struct RedisRoomData            // 此数据内容对应的key为 游戏服务id
{
    unsigned int ip;
	unsigned int port;
    int type;                   // 房间类型，比如 扑鱼游戏
	int rule;                   // 房间规则描述，使用数值映射，前端配置描述
	int rate;                   // 金币倍率等等
	unsigned int current_persons;       // 当前在线人数
	unsigned int max_persons;           // 可同时在线最大人数
	unsigned int id;                    // 房间ID
	char name[MaxKeyLen];               // 房间名字
	unsigned int limit_game_gold;       // 限制进入房间的最低金币数量
	unsigned int limit_up_game_gold;    // 限制进入房间的最多金币数量
	unsigned int update_timestamp;		// 更新的时间戳
};

// session key 对应的数据内容
static const char* SessionKey = "SessionKey";
static const int SessionKeyLen = strlen(SessionKey);
struct SessionKeyData       // 此数据内容对应的key为 用户ID
{
	unsigned int ip;
	unsigned short port;
	unsigned int timeSecs;
	unsigned int loginSrvId;
	unsigned char userVersion;          // 客户端新老版本号标识
	unsigned char reserve;              // 保留
};

void getSessionKey(const SessionKeyData& skd, char sessionKey[MaxKeyLen]);//将key的前10字节 转为16进制字串

static const int MaxServiceCount = 100; //同一类服务的最大服务进程数
bool initServiceIDList(NCommon::CCfg *pcfg);	//初使化服务ID列表（getDestServiceID之前调用）
bool getDestServiceID(uint32_t dest_service_type, const char *str, const int str_len, uint32_t &service_id); //根据目标服务类型 username 获取目标服务id（用作负载均衡）
bool getDestServiceID(uint32_t dest_service_type, const unsigned int index, uint32_t &service_id); //根据目标服务类型 index 获取目标服务id（用作负载均衡）
unsigned int getHttpServiceId(const unsigned int type);

static const char* LoginListKey = "LoginList";
static const int LoginListLen = strlen(LoginListKey);
struct LoginServiceData       // 此数据内容对应的key为 Login服务id
{
	unsigned int ip;
	unsigned short port;
	unsigned int curTimeSecs;			// 当前时间戳
	unsigned int current_persons;       // 当前在线人数
};

static const char* GatewayProxyListKey = "GatewayProxyList";
static const int GatewayProxyListKeyLen = strlen(GatewayProxyListKey);
struct GatewayProxyServiceData          // 此数据内容对应的key为网关代理服务id
{
	unsigned int ip;
	unsigned short port;
	unsigned int curTimeSecs;			// 当前时间戳
	unsigned int current_persons;       // 当前在线人数
};

static const char* GameServiceKey = "GameService";
static const int GameServiceLen = strlen(GameServiceKey);
struct GameServiceData					// 此数据内容对应的key为 用户ID
{
	unsigned int id;  // 服务ID
};


// 从redis获取到的大厅逻辑数据，一级key为"HallLogicData"，二级key为用户ID
static const char* HallLogicDataKey = "HallLogicData";
static const int HallLogicDataKeyLen = strlen(HallLogicDataKey);

// 从redis获取到的捕鱼逻辑数据，一级key为"BuyuLogicData"，二级key为用户ID
static const char* BuyuLogicDataKey = "BuyuLogicData";
static const int BuyuLogicDataKeyLen = strlen(BuyuLogicDataKey);

// 从redis获取到的翻翻棋逻辑数据，一级key为"FFChessLogicData"，二级key为用户ID
static const char* FFLogicLogicDataKey = "FFChessLogicData";
static const int FFLogicLogicDataKeyLen = strlen(FFLogicLogicDataKey);

// 从redis获取到的DbCommon服务逻辑数据，一级key为"DBCommonLogicData"，二级key为用户ID
static const char* DBCommonLogicDataKey = "DBCommonLogicData";
static const int DBCommonLogicDataKeyLen = strlen(DBCommonLogicDataKey);

// 从redis获取到的Http服务逻辑数据，一级key为"HttpLogicData"，二级key为用户ID
static const char* HttpLogicDataKey = "HttpLogicData";
static const int HttpLogicDataKeyLen = strlen(HttpLogicDataKey);

// 从redis获取到的用户充值数据，一级key为"RechargeData"，二级key为订单ID
static const char* RechargeDataKey = "RechargeData";
static const int RechargeDataKeyLen = strlen(RechargeDataKey);

// 从redis获取到的用户充值数据（渔币充值），一级key为"FishCoinRecharge"，二级key为订单ID
static const char* FishCoinRechargeKey = "FishCoinRecharge";
static const int FishCoinRechargeKeyLen = strlen(FishCoinRechargeKey);

// 从redis获取到的排行榜数据，一级key为"RankingData"，二级key为服务ID
static const char* RankingDataKey = "RankingData";
static const int RankingDataKeyLen = strlen(RankingDataKey);

// PK场索引
static const char* PkIndexKey = "PkIndex";	
static const int PkIndexKeyLen = strlen(PkIndexKey);

// PK场玩家
static const char* PkPlayerKey = "PkPlayer";
static const int PkPlayerKeyLen = strlen(PkPlayerKey);

// PK场 PK类型
static const char* PkTypeKey = "PkType";
static const int PkTypeKeyLen = strlen(PkTypeKey);



// 获取随机数
int getRandNumber(const int min, const int max);
unsigned int getPositiveRandNumber(unsigned int min, unsigned int max);

// 获取随机字符串，生成 len - 1 个随机字符，以\0结束
const char* getRandChars(char* buff, const unsigned int len);

// MD5加密
void md5Encrypt(const char* data, unsigned int len, char* md5Buff, bool lowercase = true);


// base64编解码
bool base64Encode(const char* msg, int len, char* buffer, unsigned int& bufferLen, bool noNL = true);
int base64Decode(const char* str, int len, char* buffer, unsigned int bufferLen, bool noNL = true);


// URL编解码
bool urlEncode(const char* str, unsigned int len, char* enCode, unsigned int& codeLen);
bool urlDecode(const char* enCode, unsigned int codeLen, char* str, unsigned int& len);

//3DES ECB解码
string dESEcb3Encrypt(const string &ciphertext, const string &key);



// 先对消息做SHA1摘要，接着对摘要做RSA签名
bool sha1RSASign(const char* privateKey, unsigned int privateKeyLen, const unsigned char* msg, unsigned int len, unsigned char* sigret, unsigned int& siglen, bool needSha1 = true);

// SHA1摘要做RSA验证
bool sha1RSAVerify(const char* publicKey, unsigned int publicKeyLen, const unsigned char* msg, unsigned int len, const unsigned char* sigret, unsigned int siglen, bool needSha1 = true);

// 哈希1运算 消息认证码
char* HMAC_SHA1(const void* key, unsigned int keyLen, const unsigned char* data, unsigned int dataLen, unsigned char* md, unsigned int& mdLen);

// 哈希运算消息认证
char* Compute_HMAC(const void* evpMd, const void* key, unsigned int keyLen, const unsigned char* data, unsigned int dataLen, unsigned char* md, unsigned int& mdLen);


// 获取捕鱼游戏相关等级需要的经验
unsigned int getBuyuLevelExp(unsigned int lv);

// 当前一年当中的第几个天数
int getCurrentYearDay();

// 获取当前日期
const char* getCurrentDate(char* date, unsigned int len, const char* format);


// 是否是PK场金币门票
bool checkPKGoldTicket(const unsigned int value, unsigned int& id, unsigned int& prefix, bool& isFullDay,
                       unsigned int& day, unsigned int& beginHour, unsigned int& beginMin, unsigned int& endHour, unsigned int& endMin);

// 解析后返回物品类型
unsigned int parseGameGoodsType(const unsigned int type);

// 根据物品类型获取物品中文名称
const char* getGameGoodsChineseName(const unsigned int type);


#pragma pack()

}

#endif // CPROJECT_COMMON_TYPE_H
