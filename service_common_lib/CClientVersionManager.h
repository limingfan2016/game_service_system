
/* author : limingfan
 * date : 2015.05.20
 * description : 客户端软件版本更新管理
 */

#ifndef CLIENT_VERSION_MANAGER_H
#define CLIENT_VERSION_MANAGER_H


#include <string>
#include "base/MacroDefine.h"
#include "SrvFrame/CLogicHandler.h"


using namespace std;

namespace NProject
{

// 客户端版本管理
class CClientVersionManager : public NFrame::CHandler
{
public:
	CClientVersionManager();
	~CClientVersionManager();

public:
	int init(NFrame::CLogicHandler* msgHander, unsigned short reqProtocol, unsigned short rspProtocol);

private:
	virtual int getVersionInfo(const unsigned int osType, const unsigned int platformType, const string& curVersion, unsigned int& flag, string& newVersion, string& newFileURL) = 0;
	
private:
	void checkVersion(const char* data, const unsigned int len, unsigned int srcSrvId, unsigned short srcModuleId, unsigned short srcProtocolId);

	
protected:
    NFrame::CLogicHandler* m_msgHandler;
	unsigned short m_rspProtocol;

	
DISABLE_COPY_ASSIGN(CClientVersionManager);
};

}

#endif // CLIENT_VERSION_MANAGER_H
