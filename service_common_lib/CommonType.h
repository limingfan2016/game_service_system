
/* author : limingfan
 * date : 2015.03.18
 * description : 各服务公用的数据类型定义及API
 */

#ifndef CPROJECT_COMMON_TYPE_H
#define CPROJECT_COMMON_TYPE_H


#include "base/Type.h"
#include "base/CCfg.h"
#include "CommonDataDefine.h"


namespace NProject
{

#pragma pack(1)  // 网络数据全部1字节对齐

// 逻辑 redis DB 中服务数据存储信息
// 二级Key为用户ID，Value为充值订单数据
static const char* RechargeOrderListKey = "RechargeOrderList";
static const int RechargeOrderListKeyLen = strlen(RechargeOrderListKey);

// 二级Key为微信用户唯一ID，Value为微信访问接口refresh_token数据
static const char* WXLoginListKey = "WXLoginList";
static const int WXLoginListKeyLen = strlen(WXLoginListKey);


// 二级Key为游戏服务ID，Value为游戏服务信息数据
static const char* GameServiceListKey = "GameServiceList";
static const int GameServiceListLen = strlen(GameServiceListKey);


static const char* GameHallListKey = "GameHallList";
static const int GameHallListLen = strlen(GameHallListKey);
struct GameHallServiceData              // 此数据内容对应的二级key为游戏大厅服务id
{
	unsigned int curTimeSecs;			// 当前时间戳
	unsigned int currentPersons;        // 当前在线人数
};


// session key 对应的数据内容
static const char* SessionKey = "SessionKey";
static const int SessionKeyLen = strlen(SessionKey);
struct SessionKeyData       // 此数据内容对应的二级key为 用户ID
{
	unsigned int ip;
	unsigned short port;
	unsigned int srvId;
	unsigned int timeSecs;
	unsigned short userVersion;         // 客户端新老版本号标识
	
	unsigned int hallId;                // 棋牌室ID
	int status;                         // 玩家在棋牌室的状态
};


static const char* GatewayProxyListKey = "GatewayProxyList";
static const int GatewayProxyListKeyLen = strlen(GatewayProxyListKey);
struct GatewayProxyServiceData          // 此数据内容对应的二级key为网关代理服务id
{
	unsigned int ip;
	unsigned short port;
	unsigned int curTimeSecs;			// 当前时间戳
	unsigned int currentPersons;        // 当前在线人数
};


static const char* GameUserKey = "GameUser";
static const int GameUserKeyLen = strlen(GameUserKey);
struct GameServiceData					// 此数据内容对应的二级key为 用户ID
{
	unsigned int srvId;                 // 游戏服务ID
	unsigned int hallId;                // 棋牌室ID
	unsigned int roomId;                // 房间ID
	int status;                         // 玩家在房间的状态
	unsigned int timeSecs;              // 数据更新时间点
};

#pragma pack()


/*
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
*/


// 从redis获取到的大厅逻辑数据，一级key为"HallLogicData"，二级key为用户ID
static const char* HallLogicDataKey = "HallLogicData";
static const int HallLogicDataKeyLen = strlen(HallLogicDataKey);

// 从redis获取到的DBProxy服务逻辑数据，一级key为"DBProxyLogicData"，二级key为用户ID
static const char* DBProxyLogicDataKey = "DBProxyLogicData";
static const int DBProxyLogicDataKeyLen = strlen(DBProxyLogicDataKey);

// 从redis获取到的用户充值数据，一级key为"RechargeData"，二级key为订单ID
static const char* RechargeDataKey = "RechargeData";
static const int RechargeDataKeyLen = strlen(RechargeDataKey);


// 初使化服务ID列表（getDestServiceID之前调用）
bool initServiceIDList(NCommon::CCfg *pcfg);

// 根据目标服务类型 username 获取目标服务id（用作负载均衡）
unsigned int getDestServiceID(uint32_t dest_service_type, const char *str, const int str_len);

// 根据目标服务类型 index 获取目标服务id（用作负载均衡）
unsigned int getDestServiceID(uint32_t dest_service_type, const unsigned int index);

// 根据目标服务类型 username 获取目标服务id（用作负载均衡）
bool getDestServiceID(uint32_t dest_service_type, const char *str, const int str_len, uint32_t &service_id);

// 根据目标服务类型 index 获取目标服务id（用作负载均衡）
bool getDestServiceID(uint32_t dest_service_type, const unsigned int index, uint32_t &service_id);

// 根据类型获取http服务ID
unsigned int getHttpServiceId(const unsigned int type);

// 将session key的前10字节 转为16进制字串
void getSessionKey(const SessionKeyData& skd, char sessionKey[IdMaxLen]);


// 当前一年当中的第几个天数
int getCurrentYearDay();

// 获取当前日期
const char* getCurrentDate(char* date, unsigned int len, const char* format);

// 解析后返回物品类型
unsigned int parseGameGoodsType(const unsigned int type);

// 根据物品类型获取物品中文名称
const char* getGameGoodsChineseName(const unsigned int type);


// MD5加密
void md5Encrypt(const char* data, unsigned int len, char* md5Buff, bool lowercase = true);

// base64编解码
bool base64Encode(const char* msg, int len, char* buffer, unsigned int& bufferLen, bool noNL = true);
int base64Decode(const char* str, int len, char* buffer, unsigned int bufferLen, bool noNL = true);

// URL编解码
bool urlEncode(const char* str, unsigned int len, char* enCode, unsigned int& codeLen);
bool urlDecode(const char* enCode, unsigned int codeLen, char* str, unsigned int& len);

// 3DES ECB解码
string dESEcb3Encrypt(const string &ciphertext, const string &key);

// 先对消息做SHA1摘要，接着对摘要做RSA签名
bool sha1RSASign(const char* privateKey, unsigned int privateKeyLen, const unsigned char* msg, unsigned int len, unsigned char* sigret, unsigned int& siglen, bool needSha1 = true);

// SHA1摘要做RSA验证
bool sha1RSAVerify(const char* publicKey, unsigned int publicKeyLen, const unsigned char* msg, unsigned int len, const unsigned char* sigret, unsigned int siglen, bool needSha1 = true);

// 哈希1运算 消息认证码
char* HMAC_SHA1(const void* key, unsigned int keyLen, const unsigned char* data, unsigned int dataLen, unsigned char* md, unsigned int& mdLen);

// 哈希运算消息认证
char* Compute_HMAC(const void* evpMd, const void* key, unsigned int keyLen, const unsigned char* data, unsigned int dataLen, unsigned char* md, unsigned int& mdLen);

}

#endif // CPROJECT_COMMON_TYPE_H

