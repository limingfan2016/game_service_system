
/* author : admin
 * date : 2014.10.15
 * description : 各种常量定义，配置值等等
 */
 
#ifndef CONSTANT_H
#define CONSTANT_H


// 技术性常量配置放到这里
// 非技术性常量放到外面的配置文件里
namespace NCommon
{
static const int CfgKeyValueLen = 128;         // 配置文件名值对字符串的最大长度
static const int MaxFullLen = 2048;            // 支持的最大全路径文件名长度
static const int MaxLogBuffLen = 1024 * 128;   // 支持一次写入的最大日志长度
static const int WriteErrTime = 5;             // 连续写日志文件多少次失败后，重新关闭&打开文件
static const int TryTimes = 10;                // 连续尝试打开多少次日志文件失败后，关闭日志功能
static const int MinConnectPort = 2000;        // socket主动建立连接的最小端口号，防止和系统端口号冲突
}


#endif // CONSTANT_H
