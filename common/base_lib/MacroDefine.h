
/* author : admin
 * date : 2014.10.15
 * description : 各种宏定义
 */
 
#ifndef MACRO_DEFINE_H
#define MACRO_DEFINE_H

#include "CLogger.h"
#include "CMemMonitor.h"


#ifndef NULL
#define NULL 0 
#endif

#ifndef __FILE__
#define __FILE__ ""
#endif

#ifndef __LINE__
#define __LINE__ 0
#endif


// new & delete 宏定义，防止内存操作失败抛出异常
#define NEW(pointer, ClassType)     \
    try                             \
	{                               \
		pointer = new ClassType;    \
        NCommon::CMemMonitor::getInstance().newMemInfo(pointer, __FILE__, __LINE__, sizeof(ClassType));  \
	}                               \
	catch(...)                      \
	{                               \
		pointer = NULL;             \
	}

#define NEW0(pointer, ClassType)        \
        try                             \
        {                               \
            pointer = new ClassType();    \
            NCommon::CMemMonitor::getInstance().newMemInfo(pointer, __FILE__, __LINE__, sizeof(ClassType));  \
        }                               \
        catch(...)                      \
        {                               \
            pointer = NULL;             \
        }

	
#define DELETE(pointer)             \
    try                             \
	{                               \
		delete pointer;             \
        NCommon::CMemMonitor::getInstance().deleteMemInfo(pointer);  \
		pointer = NULL;             \
	}                               \
	catch(...)                      \
	{                               \
		pointer = NULL;             \
	}
	

#define NEW_ARRAY(pointer, ClassArray)    \
    try                                   \
	{                                     \
		pointer = new ClassArray;         \
        NCommon::CMemMonitor::getInstance().newMemInfo(pointer, __FILE__, __LINE__, sizeof(ClassArray));  \
	}                                     \
	catch(...)                            \
	{                                     \
		pointer = NULL;                   \
	}
	
	
#define DELETE_ARRAY(pointer)             \
    try                                   \
	{                                     \
		delete [] pointer;                \
        NCommon::CMemMonitor::getInstance().deleteMemInfo(pointer);  \
		pointer = NULL;                   \
	}                                     \
	catch(...)                            \
	{                                     \
		pointer = NULL;                   \
	}
	


// 各种类型的日志宏
#ifdef DEBUG  // debug log 日志开关
#define SetDebugLogOutput(value)               CLogger::getDebugLogger().setOutput(value)
#define DebugLog(level, format, args...)       CLogger::getDebugLogger().writeFile(__FILE__, __LINE__, level, format, ##args)
#else
#define SetDebugLogOutput(value)
#define DebugLog(level, format, args...)
#endif

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
#define DebugInfoLog(format, args...)          DebugLog(LogLevel::Info, format, ##args)
#define DebugWarnLog(format, args...)          DebugLog(LogLevel::Warn, format, ##args)
#define DebugErrorLog(format, args...)         DebugLog(LogLevel::Error, format, ##args)
#define DebugBusinessLog(format, args...)      DebugLog(LogLevel::Business, format, ##args)
#define DebugDevLog(format, args...)           DebugLog(LogLevel::Dev, format, ##args)
#define DebugTestLog(format, args...)          DebugLog(LogLevel::Test, format, ##args)
#define DebugProductLog(format, args...)       DebugLog(LogLevel::Product, format, ##args)


// 运行日志
#define SetReleaseLogOutput(value)             CLogger::getReleaseLogger().setOutput(value)
#define ReleaseLog(level, format, args...)     CLogger::getReleaseLogger().writeFile(NULL, 0, level, format, ##args)
#define ReleaseInfoLog(format, args...)        ReleaseLog(LogLevel::Info, format, ##args)
#define ReleaseWarnLog(format, args...)        ReleaseLog(LogLevel::Warn, format, ##args)
#define ReleaseErrorLog(format, args...)       ReleaseLog(LogLevel::Error, format, ##args)


// 操作日志
#define SetOptLogOutput(value)                 CLogger::getOptLogger().setOutput(value)
#define OptLog(level, format, args...)         CLogger::getOptLogger().writeFile(NULL, 0, level, format, ##args)
#define OptInfoLog(format, args...)            OptLog(LogLevel::Info, format, ##args)
#define OptWarnLog(format, args...)            OptLog(LogLevel::Warn, format, ##args)
#define OptErrorLog(format, args...)           OptLog(LogLevel::Error, format, ##args)



// 位数组定义
#ifndef CHAR_BIT
#define CHAR_BIT (8)
#endif

#define BIT_MASK(idx)     (1 << ((idx) % CHAR_BIT))
#define BIT_IDX(idx)      ((idx) / CHAR_BIT)
#define BIT_LEN(bn)       (((bn) + (CHAR_BIT - 1)) / CHAR_BIT)
#define BIT_SET(ba, idx)  ((ba)[BIT_IDX(idx)] |= BIT_MASK(idx))
#define BIT_CLE(ba, idx)  ((ba)[BIT_IDX(idx)] &= (~BIT_MASK(idx)))
#define BIT_TST(ba, idx)  ((ba)[BIT_IDX(idx)] & BIT_MASK(idx))



// 禁止默认构造函数	
#define DISABLE_CONSTRUCTION(CLASS_NAME)         \
private:                                         \
	CLASS_NAME()
	
// 禁止析构函数	
#define DISABLE_DECONSTRUCTION(CLASS_NAME)       \
private:                                         \
	~CLASS_NAME()

// 禁止拷贝赋值类函数
#define DISABLE_COPY_ASSIGN(CLASS_NAME)          \
private:                                         \
	CLASS_NAME(const CLASS_NAME&);               \
	CLASS_NAME& operator =(const CLASS_NAME&)
	
// 禁止构造&拷贝&赋值类函数
#define DISABLE_CONSTRUCTION_ASSIGN(CLASS_NAME)  \
DISABLE_CONSTRUCTION(CLASS_NAME);                \
DISABLE_COPY_ASSIGN(CLASS_NAME)

// 禁止类基本函数
#define DISABLE_CLASS_BASE_FUNC(CLASS_NAME)      \
DISABLE_CONSTRUCTION_ASSIGN(CLASS_NAME);         \
DISABLE_DECONSTRUCTION(CLASS_NAME)



#endif // MACRO_DEFINE_H

