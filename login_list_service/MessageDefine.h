
/* author : liuxu
 * date : 2015.03.18
 * description : 消息定义文件
 */
 
#ifndef __MESSAGE_DEFINE_H__
#define __MESSAGE_DEFINE_H__


static const unsigned int MAX_MESSAGE_LEN = 1024 * 32;
static const unsigned int MAX_LISTEN_COUNT = 1024;
static const unsigned int MAX_SERVER_COUNT = 1024;


// LoginList 协议
enum ProtocolId
{
	CLIENT_GET_LOGIN_LIST_REQ = 1,
	CLIENT_GET_LOGIN_LIST_RSP = 2,
};

// 网络数据包类型
enum NetPkgType
{
	MSG = 0,          // 用户消息包
	CTL = 1,          // 控制类型包
};

// 控制标示符
enum CtlFlag
{
	HB_RQ = 0,        // 心跳请求包，发往对端
	HB_RP = 1,        // 心跳应答包，来自对端
};

// 网络数据包头部，注意字节对齐
// 1）len字段必填写，并且需要转成网络字节序
// 2）如果是用户协议消息，其他字段为0；如果是心跳消息则根据类型填写
#pragma pack(1)
struct NetPkgHeader
{
	uint32_t len;    // 数据包总长度，包括包头+包体
	uchar8_t type;   // 数据包类型，用户消息包则值为0
	uchar8_t cmd;    // 控制信息
	uchar8_t ver;    // 版本号，目前为0
	uchar8_t res;    // 保留字段，目前为0

	NetPkgHeader() {}
	NetPkgHeader(uint32_t _len, uchar8_t _type, uchar8_t _cmd) : len(_len), type(_type), cmd(_cmd) {}
	~NetPkgHeader() {}
};


// 外部客户端消息头部数据，注意字节对齐
// 1）protocolId、msgLen字段必填写，并且需要转成网络字节序；其他字节可根据需要看是否填写，一般不用填写置0即可
struct ClientMsgHeader
{
	unsigned int serviceId;         // 服务ID
	unsigned short moduleId;        // 服务下的模块ID
	unsigned short protocolId;      // 模块下的协议ID
	unsigned int msgId;             // 消息ID
	unsigned int msgLen;            // 消息码流长度，不包含消息头长度
};

struct MessagePkg
{
	struct NetPkgHeader netHeader;
	struct ClientMsgHeader clientHeader;
	char data[MAX_MESSAGE_LEN];
};
#pragma pack()


#endif // __MESSAGE_DEFINE_H__

