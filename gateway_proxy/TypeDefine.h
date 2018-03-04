
/* author : limingfan
 * date : 2015.04.23
 * description : 类型定义
 */

#ifndef TYPE_DEFINE_H
#define TYPE_DEFINE_H

#include <unordered_map>
#include "SrvFrame/FrameType.h"
#include "SrvFrame/UserType.h"


using namespace std;

namespace NPlatformService
{

// 协议操作返回码
enum ProtocolReturnCode
{
	Opt_Failed = -1,
	Opt_Success = 0,
};

// 服务ID映射到连接对应的索引值
typedef unordered_map<unsigned int, unsigned int> ServiceIDToConnectIdx;

// 连接对应的数据
struct ConnectUserData
{
	unsigned int index;
	NConnect::Connect* conn;
	ServiceIDToConnectIdx serviceId2ConnectIdx;
};

// 索引值映射到连接对象
typedef unordered_map<unsigned int, ConnectUserData> IndexToConnects;

}

#endif // TYPE_DEFINE_H

