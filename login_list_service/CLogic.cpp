
/* author : liuxu
* date : 2015.03.18
* description : login_list_service 主逻辑实现
*/

#include "CLogic.h"
#include <string.h>
#include "MessageDefine.h"
#include "base/Function.h"
#include "_LoginListConfig_.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>


enum RetValue
{
	LoadConfigErr = 1,
	CreateUDPErr = 2,
	ConnectUDPErr = 3,
};


CLogic::CLogic()
{
	m_isRun = false;
	m_pUdp = NULL;

	bzero(&m_pkg, sizeof(m_pkg));
	m_pkg_len = 0;
}

CLogic::~CLogic()
{
	m_isRun = false;
	if (m_pUdp)
	{
		delete m_pUdp;
		m_pUdp = NULL;
	}
}

int CLogic::init()
{
	int rc = loadConfig();
	if (rc != 0) return rc;

	m_pUdp = new CUdp(m_config.listen_ip.c_str(), m_config.listen_port);
	if (0 != m_pUdp->create(MAX_LISTEN_COUNT))
	{
		return CreateUDPErr;
	}

	if (!m_redis.connectSvr(m_config.redis_ip.c_str(), m_config.redis_port))
	{
		return ConnectUDPErr;
	}

	updatePkgData();

	return 0;
}

void CLogic::run()
{
	m_isRun = true;

	struct MessagePkg pkg;
	struct sockaddr_in client_addr;
	size_t addr_len = sizeof(client_addr);
	while (m_isRun)
	{
		static uint32_t proc_count = 0;
		bzero(&client_addr, sizeof(client_addr));
		addr_len = sizeof(client_addr);
		size_t ret = m_pUdp->recvFrom((void*)&pkg, sizeof(pkg), (struct sockaddr*)&client_addr, (socklen_t*)&addr_len);
		if (ret > 0 && -1 != (int)ret) // 正常接收
		{
			if (CLIENT_GET_LOGIN_LIST_REQ == ntohs(pkg.clientHeader.protocolId))
			{
				proc_count++;
				if (m_pUdp->sendTo((void*)&m_pkg, m_pkg_len, (struct sockaddr*)&client_addr, addr_len) < 0)
				{
					ReleaseErrorLog("send data failed. error code:%d, desc:%s", errno, strerror(errno));
				}
			}
			else
			{
				ReleaseErrorLog("unknow protocol id:%d, from ip:%s, port:%u", ntohs(pkg.clientHeader.protocolId), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
			}
		}
		else
		{
			usleep(10 * 1000);
		}

		static time_t last_update_timestamp = time(NULL);
		time_t cur_timestamp = time(NULL);
		if (cur_timestamp - last_update_timestamp >= m_config.update_redis_time)
		{
			updatePkgData();
			ReleaseInfoLog("Recv PKG Stat|%u|%u", cur_timestamp - last_update_timestamp, proc_count);
			last_update_timestamp = cur_timestamp;
			proc_count = 0;
		}
	}
}

void CLogic::stop()
{
	m_isRun = false;
}

void CLogic::sortLoginInfo(LoginInfo *login_info_arr, int len)
{
	for (int i = 0; i < len; i++)
	{
		for (int j = i + 1; j < len; j++)
		{
			if (login_info_arr[i].current_persons > login_info_arr[j].current_persons)
			{
				LoginInfo tmp;
				tmp.ip = login_info_arr[i].ip;
				tmp.port = login_info_arr[i].port;
				tmp.current_persons = login_info_arr[i].current_persons;
				tmp.service_id = login_info_arr[i].service_id;
				login_info_arr[i].ip = login_info_arr[j].ip;
				login_info_arr[i].port = login_info_arr[j].port;
				login_info_arr[i].current_persons = login_info_arr[j].current_persons;
				login_info_arr[i].service_id = login_info_arr[j].service_id;
				login_info_arr[j].ip = tmp.ip;
				login_info_arr[j].port = tmp.port;
				login_info_arr[j].current_persons = tmp.current_persons;
				login_info_arr[j].service_id = tmp.service_id;
			}
		}
	}
}

void CLogic::updatePkgData()
{
	vector<string> rel;
	int ret = m_redis.getHAll(NProject::GatewayProxyListKey, NProject::GatewayProxyListKeyLen, rel);
	if (0 != ret)
	{
		ReleaseErrorLog("m_redis.getHAll error|%d", ret);
		return;
	}

    const unordered_map<unsigned int, LoginListConfig::ServiceInfo>& srvInfoList = LoginListConfig::config::getConfigValue().filter_service_list;
	LoginInfo login_info_arr[MAX_SERVER_COUNT];
	int login_info_count = 0;
	time_t cur_timestamp = time(NULL);
	for (int i = 1; i < (int)rel.size(); i += 2)
	{
		const unsigned int* srvId = (const unsigned int*)rel[i - 1].c_str();
		if (srvInfoList.find(*srvId) == srvInfoList.end())  // 非过滤服务
		{
			const LoginServiceData* p_login_data = (const LoginServiceData*)rel[i].c_str();
			if ((int)cur_timestamp - (int)p_login_data->curTimeSecs > m_config.update_redis_time * 3) //删除超时的login server info
			{ 
				ret = m_redis.delHField(NProject::GatewayProxyListKey, NProject::GatewayProxyListKeyLen, rel[i - 1].c_str(), rel[i - 1].length());
				if (ret < 0)
				{
					ReleaseErrorLog(" m_redis.delHField failed|%s|%s", NProject::GatewayProxyListKey, rel[i - 1].c_str());
				}
			}
			else //将数据拷贝到新的结构体
			{
				login_info_arr[login_info_count].ip = p_login_data->ip;
				login_info_arr[login_info_count].port = p_login_data->port;
				login_info_arr[login_info_count].current_persons = p_login_data->current_persons;
				login_info_arr[login_info_count].service_id = m_config.hall_login_id;
				login_info_count++;
			}
		}
	}

	//将Login服务信息以当前人数排序（从小到大）
	sortLoginInfo(login_info_arr, login_info_count);

	//打包
	com_protocol::ClientGetLoginListRsp rsp;
	for (int i = 0; i < login_info_count; i++)
	{
		com_protocol::LoginServerInfo *plogin_info = rsp.add_login_info_list();
		plogin_info->set_ip(login_info_arr[i].ip);
		plogin_info->set_port(login_info_arr[i].port);
		plogin_info->set_current_persons(login_info_arr[i].current_persons);
		plogin_info->set_service_id(login_info_arr[i].service_id);
	}
	int data_len = rsp.ByteSize();

	bzero(&m_pkg, sizeof(m_pkg));
	m_pkg_len = sizeof(m_pkg.netHeader) + sizeof(m_pkg.clientHeader) + data_len;
	m_pkg.netHeader.len = htonl(m_pkg_len);
	m_pkg.netHeader.type = 0;
	m_pkg.netHeader.cmd = 0;
	m_pkg.netHeader.ver = 1;
	m_pkg.netHeader.res = 0;
	m_pkg.clientHeader.serviceId = htons(0);
	m_pkg.clientHeader.moduleId = htons(0);
	m_pkg.clientHeader.protocolId = htons(ProtocolId::CLIENT_GET_LOGIN_LIST_RSP);
	m_pkg.clientHeader.msgId = htonl(0);
	m_pkg.clientHeader.msgLen = htonl(data_len);
	if (!rsp.SerializeToArray(m_pkg.data, MAX_MESSAGE_LEN))
	{
		ReleaseErrorLog("--- ProtocolId::CLIENT_GET_LOGIN_LIST_RSP--- pack failed.");
		stop();
	}

	return ;
}

int CLogic::loadConfig()
{
	ReleaseInfoLog("begin load config.");

	//logic 相关配置
	const char* rel = CCfg::getValue("logic", "listen_ip");
	if (NULL != rel){ m_config.listen_ip = rel; }
	rel = CCfg::getValue("logic", "listen_port");
	if (NULL != rel){ m_config.listen_port = atoi(rel); }
	rel = CCfg::getValue("logic", "redis_ip");
	if (NULL != rel){ m_config.redis_ip = rel; }
	rel = CCfg::getValue("logic", "redis_port");
	if (NULL != rel){ m_config.redis_port = atoi(rel); }
	rel = CCfg::getValue("logic", "update_redis_time");
	if (NULL != rel){ m_config.update_redis_time = atoi(rel); }
	
	rel = CCfg::getValue("logic", "hall_login_id");
	if (NULL != rel){ m_config.hall_login_id = atoi(rel); }
	
	if (!LoginListConfig::config::getConfigValue(CCfg::getValue("logic", "BusinessXmlConfigFile")).isSetConfigValueSuccess())
	{
		ReleaseErrorLog("load business config error.");
		return LoadConfigErr;
	}

	ReleaseInfoLog("load config success.");

	return 0;
}

