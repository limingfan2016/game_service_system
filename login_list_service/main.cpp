
/* author : liuxu
 * date : 2015.03.18
 * description : login_list 主入口
 */

#include "CLogic.h"
#include "base/CProcess.h"


CLogic g_logic;

void exit_logic(int sigNum, siginfo_t* sigInfo, void* context)
{
	ReleaseWarnLog("receive signal = %d, and exit service", sigNum);
	g_logic.stop();
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("useage: ./bin ./config/common.cfg");
		return 0;
	}
	
	CCfg::useDefaultCfg(argv[1]);

	
	int rc = g_logic.init();
	if (0 != rc)
	{
		ReleaseErrorLog("logic.init() error|%d", rc);
	}
	else
	{
		CProcess::installSignal(SIGTERM, exit_logic);  // 服务正常退出信号
		g_logic.run();
	}

	return 0;
}
