
/* author : limingfan
 * date : 2014.10.15
 * description : 各种宏定义
 */
 
#ifndef MACRO_DEFINE_H
#define MACRO_DEFINE_H

#include "CLogger.h"


#ifndef NULL
#define NULL 0 
#endif


// new & delete 宏定义，防止内存操作失败抛出异常
#define NEW(pointer, ClassType)     \
    try                             \
	{                               \
		pointer = new ClassType;    \
	}                               \
	catch(...)                      \
	{                               \
		pointer = NULL;             \
	}
	
	
#define DELETE(pointer)             \
    try                             \
	{                               \
		delete pointer;             \
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
	}                                     \
	catch(...)                            \
	{                                     \
		pointer = NULL;                   \
	}
	
	
#define DELETE_ARRAY(pointer)             \
    try                                   \
	{                                     \
		delete [] pointer;                \
		pointer = NULL;                   \
	}                                     \
	catch(...)                            \
	{                                     \
		pointer = NULL;                   \
	}
	


// 各种类型的日志宏
#ifdef DEBUG  // debug log 日志开关
#define DebugLog(level, format, args...)       CLogger::getDebugLogger().writeFile(__FILE__, __LINE__, level, format, ##args)
#else
#define DebugLog(level, format, args...)
#endif

// 调试日志
#define DebugInfoLog(format, args...)          DebugLog(Info, format, ##args)
#define DebugWarnLog(format, args...)          DebugLog(Warn, format, ##args)
#define DebugErrorLog(format, args...)         DebugLog(Error, format, ##args)

// 运行日志
#define ReleaseLog(level, format, args...)     CLogger::getReleaseLogger().writeFile(NULL, 0, level, format, ##args)
#define ReleaseInfoLog(format, args...)        ReleaseLog(Info, format, ##args)
#define ReleaseWarnLog(format, args...)        ReleaseLog(Warn, format, ##args)
#define ReleaseErrorLog(format, args...)       ReleaseLog(Error, format, ##args)

// 操作日志
#define OptLog(level, format, args...)         CLogger::getOptLogger().writeFile(NULL, 0, level, format, ##args)
#define OptInfoLog(format, args...)            OptLog(Info, format, ##args)
#define OptWarnLog(format, args...)            OptLog(Warn, format, ##args)
#define OptErrorLog(format, args...)           OptLog(Error, format, ##args)

		

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

