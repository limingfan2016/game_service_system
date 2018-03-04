
/* author : limingfan
 * date : 2017.09.21
 * description : 类型定义
 */

#ifndef TYPE_DEFINE_H
#define TYPE_DEFINE_H

#include "common/GameHallErrorCode.h"
#include "common/CommonType.h"
#include "common/CServiceOperation.h"

#include "common/GameHallProtocolId.h"
#include "common/DBProxyProtocolId.h"
#include "common/MessageCenterProtocolId.h"
#include "common/OperationManagerProtocolId.h"
#include "common/HttpOperationProtocolId.h"

#include "protocol/appsrv_game_hall.pb.h"
#include "_HallConfigData_.h"


using namespace NProject;

namespace NPlatformService
{

// 挂接在连接上的相关数据
struct HallUserData : public ConnectProxyUserData
{
	// com_protocol::HallLogicData* hallLogicData;
	
	unsigned short versionFlag;              // 客户端版本标识
	unsigned short platformType;             // 平台类型

	char nickname[IdMaxLen];                 // 用户昵称
	char portraitId[PortraitIdMaxLen];       // 用户头像ID
	int gender;                              // 用户性别
	
	char hallId[IdMaxLen];                   // 用户所在棋牌室ID
    int playerStatus;                        // 用户在棋牌室的状态
};


// 大厅缓存的消息（玩家聊天、系统推送消息等）列表
typedef list<com_protocol::MessageInfo> MessageInfoList;

// 棋牌室大厅信息
typedef unordered_map<string, MessageInfoList> HallMessageInfoMap;


// 玩家绑定电话号码信息
struct SSetPhoneNumberData
{
    unsigned int timeSecs;                   // 时间点
    string number;                           // 手机号码
    unsigned int code;                       // 随机验证码
    
    SSetPhoneNumberData() {};
	SSetPhoneNumberData(unsigned int ts, const string& mb, unsigned int cd) : timeSecs(ts), number(mb), code(cd) {};
	~SSetPhoneNumberData() {};
};
typedef unordered_map<string, SSetPhoneNumberData> UserIdToPhoneNumberDataMap;

}


#endif // TYPE_DEFINE_H

