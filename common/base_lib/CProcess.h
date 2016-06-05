
/* author : limingfan
 * date : 2015.02.06
 * description : 进程管理操作
 */
 
#ifndef CPROCESS_H
#define CPROCESS_H

#include <signal.h>
#include "MacroDefine.h"


typedef void (*SignalHandler)(int sigNum, siginfo_t* sigInfo, void* context);  // 信号处理函数

namespace NCommon
{

// 进程接收的信号值
namespace SignalNumber
{
    static const int ReloadConfig = SIGRTMIN + 1;         // 更新配置文件
	static const int StopProcess = SIGRTMIN + 2;          // 停止退出进程
};


class CProcess
{
public:
    // 安装信号处理函数
	static int installSignal(int sigNum, SignalHandler handler);
	
	// 忽略信号
	static int ignoreSignal(int sigNum);
	
	// 把进程转变为守护进程
	static int toDaemon();
	

DISABLE_CLASS_BASE_FUNC(CProcess);  // 禁止实例化对象
};

}

#endif // CPROCESS_H
