
/* author : admin
 * date : 2014.10.29
 * description : 各种类型定义
 */
 
#ifndef TYPE_H
#define TYPE_H

#include <stdio.h>


// 网络码流传输的类型
typedef char char8_t;
typedef short short16_t;
typedef int int32_t;
typedef long long long64_t;
typedef float float32_t;
typedef double double64_t;

typedef unsigned char uchar8_t;
typedef unsigned short ushort16_t;
typedef unsigned int uint32_t;
typedef unsigned long long ulong64_t;


typedef ulong64_t uuid_type;  // 唯一用户ID类型

static const int StrIPLen = 32;
typedef char strIP_t[StrIPLen];  // IP 类型

static const int StrIntLen = 32;
typedef char strInt_t[StrIntLen];   // 整型


namespace NCommon
{

static inline char* intValueToChars(const int val, strInt_t str)
{
	snprintf(str, StrIntLen - 1, "%d", val);
	return str;
}


// 节点ID
union NodeID
{
	uuid_type id;
	struct IpPort
	{
		unsigned int ip;
		unsigned int port;
	} ipPort;
};


// 共享内存相关数据类型
// 数据包类型
enum PkgFlag
{
	USER_DATA = 1,          // 用户数据
	BUFF_END = 2,           // 数据包缓冲区已到尾部
};

// 数据包头
struct PkgHeader
{
	int len;              // 数据包长度
	unsigned char flag;   // 数据包标识符
	unsigned char cmd;    // 控制信息，目前为0
	unsigned char ver;    // 版本号，目前为0
	unsigned char res;    // 保留字段，目前为0
};

// 统计数据信息
struct PkgRecord
{
	unsigned int writeCount;     // 写入的数据包总个数
	unsigned int writeSize;      // 写入的数据包总长度
	
	unsigned int readCount;      // 读取的数据包总个数
	unsigned int readSize;       // 读取的数据包总长度
};

// 数据包队列
struct PkgQueue
{
	int size;
	volatile int write;
	volatile int read;
	PkgRecord record;
};

}


#endif  // TYPE_H
