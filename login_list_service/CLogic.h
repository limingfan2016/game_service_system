
/* author : liuxu
 * date : 2015.03.18
 * description : login_list 主逻辑头文件
 */

#include "db/CRedis.h"
#include "connect/CUdp.h"
#include "base/CCfg.h"
#include "base/CLogger.h"
#include "../common/CommonType.h"

#include "MessageDefine.h"
#include "appsrv_login_list.pb.h"


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
	
	unsigned int update_service_time;
	unsigned int remove_service_time;
	
	unsigned int hall_login_id;
	unsigned int operation_manager_id;

	CConfig()
	{
		listen_port = 0;
		redis_port = 0;
		update_service_time = 30;  // 从redis获取服务数据间隔时间，单位秒
		remove_service_time = 180; // 服务数据超时则删除时间间隔，单位秒
		
		hall_login_id = 0;
		operation_manager_id = 0;
	}
};


typedef struct
{
	unsigned int ip;
	unsigned short port;
	unsigned int current_persons;            // 当前在线人数
	unsigned int hall_login_id;              // 游戏大厅服务ID
	unsigned int operation_manager_id;       // 运营管理服务ID
} LoginInfo;


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
	unsigned int m_pkg_len;
};

