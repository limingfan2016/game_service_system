
/* author : limingfan
 * date : 2015.01.15
 * description : 各服务间消息通信中间件
 */

#ifndef CSRVMSGCOMM_H
#define CSRVMSGCOMM_H

#include "messageComm/MsgCommType.h"
#include "base/MacroDefine.h"


using namespace NCommon;

namespace NMsgComm
{

class CSrvMsgComm
{
public:
	CSrvMsgComm();
	~CSrvMsgComm();
	
public:
    int createShm(const char* cfgFile);
	void run();
	void updateConfig();
	
private:
    int getNetNodes(unsigned int& nodeCount);
    int initCfgFile(const char* cfgFile = NULL);
	int initShm();
	void initNetHandler();
	int initSharedMutex();
	void unInitSharedMutex();
	
private:
	ShmData* m_shmData;
	CfgData m_cfgData;
	int m_keyFileFd;
	
DISABLE_COPY_ASSIGN(CSrvMsgComm);
};

}

#endif // CSRVMSGCOMM_H
