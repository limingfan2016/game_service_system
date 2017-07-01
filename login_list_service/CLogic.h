
/* author : liuxu
* date : 2015.03.18
* description : login_list_service 主逻辑头文件
*/

#include "db/CRedis.h"
#include "connect/CUdp.h"
#include "base/CCfg.h"
#include "base/CLogger.h"
#include "../common/CommonType.h"
#include "MessageDefine.h"
#include "app_server_loginlist.pb.h"

using namespace NProject;
using namespace NCommon;
using namespace NDBOpt;
using namespace NConnect;
using namespace std;


class CConfig
{
public:
	//logic 相关配置
	string listen_ip;
	int listen_port;
	string redis_ip;
	int redis_port;
	int update_redis_time;
	unsigned int hall_login_id;

	CConfig()
	{
		listen_port = 0;
		redis_port = 0;
		update_redis_time = 30;
		hall_login_id = 0;
	}
};

typedef struct
{
	unsigned int ip;
	unsigned short port;
	unsigned int current_persons;       // 当前在线人数
	unsigned int service_id;            // 服务ID
}LoginInfo;

class CLogic
{
public:
	CLogic();
	~CLogic();

	int init();
	void run();
	void stop();

private:
	int loadConfig();
	void updatePkgData();
	void sortLoginInfo(LoginInfo *login_info_arr, int len);

private:
	volatile bool m_isRun;
	CConfig m_config;
	CUdp *m_pUdp;
	CRedis m_redis;

	struct MessagePkg m_pkg;
	int m_pkg_len;
};
