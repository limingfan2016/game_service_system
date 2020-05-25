
/* author : admin
 * date : 2015.04.23
 * description : 类型定义
 */

#ifndef TYPE_DEFINE_H
#define TYPE_DEFINE_H

#include <unordered_map>
#include "SrvFrame/FrameType.h"
#include "SrvFrame/UserType.h"
#include "common/CommonType.h"
#include "_NGatewayProxyConfig_.h"


using namespace std;

namespace NPlatformService
{

static const unsigned int MD5ResultBufferSize = 16;

// web server 系统内部消息类型
enum EWebServerSystemMessageType
{
    EHeartbeatType = 1,
    EConnectAddressType = 2,
};

// 加密流程步骤标识
enum EEncryptionProcess
{
    EReceiveFirstMessage = 1,                // 收到第一条消息
    ESendFirstMessage = 2,                    // 发送第一条消息
    EBuildEncryptionFinish = 3,              // 加密流程建立完毕
};


// 服务类型，位数组存储，以达到判断性能最优
typedef char ServiceTypeBitArray[BIT_LEN(NProject::MaxServiceType)];

// 服务间隔时间内最大收包数量
typedef unsigned short ServiceIntervalMaxPkgCount[NProject::MaxServiceType];

// 服务ID映射到连接对应的索引值
typedef unordered_map<unsigned int, unsigned int> ServiceIDToConnectIdx;

// 连接对应的数据
struct ConnectUserData
{
    int closedErrorCode;  // 连接被关闭的错误码
    
	unsigned int index;
	NConnect::Connect* conn;
	ServiceIDToConnectIdx serviceId2ConnectIdx;
    NFrame::ConnectAddress connectAddress;
    
    unsigned int hallServiceId;  // 此连接发往大厅的消息，对应的大厅服务ID
    
    // 数据加密&解密信息
    int encryptionProcess;
    unsigned int sequence;          // 序列号，防止重复包攻击
    unsigned char secretKey[MD5ResultBufferSize];
    
    // 单个连接多少毫秒时间间隔内只能接收多少条客户端消息，超过此消息数量了则关闭连接
    // 防止频繁大量消息包攻击
    unsigned long long currentClientMilliseconds;
    unsigned int currentClientMessageCount;
    
    // 单个连接多少毫秒时间间隔内只能接收多少条服务端消息，超过此消息数量了则关闭连接
    // 防止内部服务出错的情况下发送频繁大量的消息写满缓冲区之后不断申请内存导致内存暴涨
    unsigned long long currentServiceMilliseconds;
    unsigned int currentServiceMessageCount;
    
    ConnectUserData() : closedErrorCode(NFrame::ConnectProxyOperation::PassiveClosed), index(0), conn(NULL), hallServiceId(0),
                                    encryptionProcess(EEncryptionProcess::EReceiveFirstMessage), sequence(0), currentClientMilliseconds(0), currentClientMessageCount(0),
                                    currentServiceMilliseconds(0), currentServiceMessageCount(0)
                                    {secretKey[0] = '\0';}
    ~ConnectUserData() {}
};

// 索引值映射到连接对象
typedef unordered_map<unsigned int, ConnectUserData> IndexToConnects;

}

#endif // TYPE_DEFINE_H

