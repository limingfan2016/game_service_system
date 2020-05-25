
/* author : admin
 * date : 2015.02.06
 * description : 进程管理操作
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "CProcess.h"
#include "ErrorCode.h"


using namespace NErrorCode;

namespace NCommon
{

// 安装信号处理函数
int CProcess::installSignal(int sigNum, SignalHandler handler)
{
	struct sigaction sigAction;
	sigAction.sa_sigaction = handler;
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags = SA_SIGINFO;
	if (sigaction(sigNum, &sigAction, NULL) != 0)
	{
		ReleaseErrorLog("install signal handler = %d, error = %d, info = %s", sigNum, errno, strerror(errno));
		return SystemCallErr;
	}
	
	return Success;
}

// 忽略信号
int CProcess::ignoreSignal(int sigNum)
{
	struct sigaction sigAction;
	sigAction.sa_handler = SIG_IGN;
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags = 0;
	if (sigaction(sigNum, &sigAction, NULL) != 0)
	{
		ReleaseErrorLog("ignore signal = %d, error = %d, info = %s", sigNum, errno, strerror(errno));
		return SystemCallErr;
	}
	
	return Success;
}

// 把进程转变为守护进程
int CProcess::toDaemon()
{
	return SystemCallErr;
}

// 执行命令
int CProcess::doCommand(const char* cmd)
{
    if (cmd == NULL || *cmd == '\0') return InvalidParam;
    
    errno = 0;
	const int status = ::system(cmd);
    const bool isOk = (WIFEXITED(status) && WEXITSTATUS(status) == 0);
    if (!isOk)
    {
        ReleaseErrorLog("call system do command error, cmd = %s, status = %d, error = %d, info = %s", cmd, status, errno, strerror(errno));

		return SystemCallErr;
    }
    
    return Success;
}

}

