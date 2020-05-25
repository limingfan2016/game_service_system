
/* author : admin
 * date : 2014.10.28
 * description : 提供各种日志输出、日志管理功能
 */
 
#ifndef CLOGGER_H
#define CLOGGER_H

#include "Constant.h"


struct tm;
struct timeval;

namespace NCommon
{

enum LogLevel {
	Info = 2,
	Warn = 3,
	Error = 4,
    Business = 5,
    Dev = 6,
    Test = 7,
    Product = 8,
};

/* 调试日志
日志级别开关配置说明：
enum LogLevel {
	Info = 2,
	Warn = 3,
	Error = 4,
    Business = 5,
    Dev = 6,
    Test = 7,
    Product = 8,
};

各服务配置文件 common.cfg 里的 [Logger] 配置项：
[Logger]
WriteDebugLog = 0       // 0值关闭日志
WriteDebugLog = 1       // 1值打开所有级别的日志

// 打开相关级别的日志，各级别对应的值 LogLevel 任意组合，如下配置：
WriteDebugLog = 2       // 只打开 Info 级别日志
WriteDebugLog = 45      // 只打开 Error、Business 级别日志
WriteDebugLog = 34      // 只打开 Warn、Error 级别日志
WriteDebugLog = 478     // 只打开 Error、Test、Product 级别日志
*/


class CLogger
{
public:
	CLogger(const char* pFullName, long size, int maxBakFile, int output = 1);
	~CLogger();

public:
    static CLogger& getDebugLogger();
    static CLogger& getReleaseLogger();
    static CLogger& getOptLogger();
	
public:
	int writeFile(const char* fileName, const int fileLine, const LogLevel logLv, const char* pFormat, ...);
	void setOutput(int value);
	
private:
	void setFileName(const char* pFullName);
	void setDirName(const char* pFullName);
	
private:
    int formatHead(char* logBuff, const int len, const struct tm* pTm, const struct timeval& tv, const LogLevel logLv,
				   const char* fileName, const int fileLine);
	bool createDir();
	int openCurrentFile();
	int openNextFile(int idx, char* fullName, unsigned int len);
	void reOpenFile(int wLen);
	void checkAndNewFile(int wLen);
	void closeFile();
	bool checkCurrentFileIsExist();
	
private:
    // 禁止拷贝、赋值
	CLogger();
    CLogger(const CLogger&);
	CLogger& operator =(const CLogger&);
	
private:
    const char** m_pDir;           // 目录名
	char* m_pName;                 // 文件名
	char m_fullName[MaxFullLen];   // 当前文件名
	int m_dirSize;                 // 目录级数
	long m_maxSize;                // 文件最大长度，超过该长度将自动写新文件
	long m_curSize;                // 文件当前长度
	int m_maxBakFile;              // 最多保留的备份文件个数
	int m_idx;                     // 文件名后缀，从1开始
	int m_fd;                      // 文件描叙符句柄
	int m_output;                  // 是否输出日志，非0值输出，0值关闭
	
private:
    // 保护机制
	int m_writeErrTimes;     // 连续写多少次失败后，重新打开文件
	int m_tryTimes;          // 连续尝试打开多少次文件失败后，关闭日志功能
	

	// only for do test，测试代码
public:
	void output();
	void testWriteFile(int n);
};


}

#endif // CLOGGER_H
