
/* author : admin
 * date : 2014.10.27
 * description : 配置文件解析读取配置项
 */

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "Type.h"
#include "CCfg.h"


namespace NCommon
{

struct ConfigData
{
	char section[CfgKeyValueLen];
	Key2Value* keyValue;
	ConfigData* pNext;
	
	ConfigData(const char* sect, Key2Value* kv = NULL) : keyValue(kv), pNext(NULL)
	{
		strncpy(section, sect, CfgKeyValueLen - 1);
		section[CfgKeyValueLen - 1] = '\0';
	}
};


CCfg::CCfg(const char* pCfgFile, const char* key2value[], const int len) : m_cfgData(NULL), m_curCfgData(NULL)
{
	m_fileName[0] = '\0';
	
	if (pCfgFile == NULL || *pCfgFile == '\0' || strlen(pCfgFile) >= (unsigned int)MaxFullLen)  // 文件名无效
	{
		return;
	}

	if (access(pCfgFile, F_OK | W_OK) != 0)  // 不存在则创建
	{
		if (createDir(pCfgFile) == 0)  // 创建目录
		{
			createFile(pCfgFile, key2value, len); // 创建配置文件
		}
	}
	else
	{
		createKV(pCfgFile);  // 创建名值对
	}
	
	strcpy(m_fileName, pCfgFile);
}

CCfg::~CCfg()
{
	destroyKV();
	m_fileName[0] = '\0';
}

int CCfg::createDir(const char* pCfgFile)
{
	int rc = 0;
	int dirNameSize = 0;
	char dirName[MaxFullLen] = {0};	
	const char* pFind = strchr(pCfgFile, '/');
	while (pFind != NULL)
	{
		// 目录名处理
		dirNameSize = pFind - pCfgFile;
		if (dirNameSize > 0)
		{
			memcpy(dirName, pCfgFile, dirNameSize);
			dirName[dirNameSize] = '\0';
			
			rc = access(dirName, F_OK | W_OK);
			if (rc != 0)  // 不存在则创建
			{
				rc = mkdir(dirName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);  // 保持和手动创建的目录权限一致
				if (rc != 0)  // 创建目录失败直接退出
				{
					break;
				}
			}
		}
		
		pFind = strchr(++pFind, '/');  // 继续查找下一级目录名
	}
	
	return rc;
}

int CCfg::createFile(const char* pCfgFile, const char* key2value[], const int len)
{
	int rc = -1;
	FILE* pf = fopen(pCfgFile, "w");
	if (pf != NULL)
	{
		rc = 0;
		for (int i = 0; i < len; i++)
		{
			add(key2value[i]);
			
			if (fputs(key2value[i], pf) == EOF)
			{
				rc = -1;
				break;
			}
		}
		fclose(pf);
	}
	
	return rc;
}

void CCfg::createKV(const char* pCfgFile, bool isUpdate)
{
	FILE* pf = fopen(pCfgFile, "r");
	if (pf != NULL)
	{	
		char keyValue[CfgKeyValueLen] = {0};
		while (fgets(keyValue, CfgKeyValueLen - 1, pf) != NULL)
		{
			add(keyValue, isUpdate);
		}
		fclose(pf);
	}
}

void CCfg::destroyKV()
{
	m_curCfgData = NULL;
	
	ConfigData* pDel = NULL;
	Key2Value* delKV = NULL;
	while (m_cfgData != NULL)
	{
		pDel = m_cfgData;
		m_cfgData = m_cfgData->pNext;
		
		while (pDel->keyValue != NULL)
		{
			delKV = pDel->keyValue;
			pDel->keyValue = delKV->pNext;
			DELETE(delKV);
		}
		DELETE(pDel);
	}
}

void CCfg::add(const char* key2value, bool isUpdate)
{
	while (*key2value == ' ')  key2value++;    // 先过滤掉前空白符号
	
	if (*key2value == '/') return;             // 忽略注释信息
	
	bool isSection = false;
	ConfigData* cfgData = getCurCfgData(key2value, isSection);
	if (cfgData == NULL || isSection) return;  // cfgData == NULL 则配置出错了，不支持非section的配置项
	
	const char* pFind = strchr(key2value, '=');
	if (pFind == NULL) return;

	// Key值
	const char* last = pFind - 1;
	while (*last == ' ')  last--;    // 过滤掉后空白符号
	int keyLen = last - key2value + 1;

	// Value值
	pFind++;
	while (*pFind == ' ')  pFind++;  // 过滤掉前空白符号
	
	int valueLen = 0;
	last = pFind;
	char cVal = *last;
	while (cVal != '\0')
	{
		if (cVal == ' ' || cVal == ';' || cVal == '\n' || cVal == '\r') break;  // 判断是否遇到结束符号
		cVal = *(++last);
		valueLen++;
	}

	// Key&Value值
	if (keyLen > 0 && valueLen > 0)
	{
		// 先检查是否是更新的配置项
		Key2Value* pUpKV = cfgData->keyValue;
        while (isUpdate && pUpKV != NULL)
		{
			if (strlen(pUpKV->key) == (unsigned int)keyLen && memcmp(pUpKV->key, key2value, keyLen) == 0)
			{
				// 更新配置项
				if (valueLen >= CfgKeyValueLen) valueLen = CfgKeyValueLen - 1;
				memcpy(pUpKV->value, pFind, valueLen);
		        pUpKV->value[valueLen] = '\0';
				return;
			}
			pUpKV = pUpKV->pNext;
		}
		
		// 新增加的配置项
		Key2Value* kv = NULL;
		NEW(kv, Key2Value(key2value, keyLen, pFind, valueLen));
		kv->pNext = cfgData->keyValue;
		cfgData->keyValue = kv;
	}
}

const char* CCfg::getItem(const char* section, const char* item, const bool isValue)
{
	if (section == NULL || *section == '\0' || item == NULL || *item == '\0') return NULL;
	
	ConfigData* cfgData = getSectionData(section);
	if (cfgData != NULL)
	{
	    Key2Value* pFind = cfgData->keyValue;
        while (pFind != NULL)
		{
			if (isValue)
			{
				if (strcmp(pFind->key, item) == 0) return pFind->value;
			}
			else 
			{
                if (strcmp(pFind->value, item) == 0) return pFind->key;
			}
			pFind = pFind->pNext;
		}
	}
	
	return NULL;
}

const char* CCfg::get(const char* section, const char* key)
{
	return getItem(section, key);
}

const Key2Value* CCfg::getList(const char* section)
{
	if (section == NULL || *section == '\0') return NULL;
	
	ConfigData* cfgData = getSectionData(section);
	return cfgData != NULL ? cfgData->keyValue : NULL;
}

const char* CCfg::getKeyByValue(const char* section, const char* value)
{
	return getItem(section, value, false);
}

const char* CCfg::getKeyByValue(const char* section, const int value)
{
	strInt_t strValue = {0};
	snprintf(strValue, StrIntLen - 1, "%d", value);
	return getItem(section, strValue, false);
}


ConfigData* CCfg::getSectionData(const char* section)
{
	if (m_curCfgData != NULL && (strcmp(section, m_curCfgData->section) == 0)) return m_curCfgData;
	
	m_curCfgData = m_cfgData;
	while (m_curCfgData != NULL)
	{
		if (strcmp(section, m_curCfgData->section) == 0) return m_curCfgData;
		m_curCfgData = m_curCfgData->pNext;
	}
	
	return NULL;
}

ConfigData* CCfg::getCurCfgData(const char* data, bool& isSection)
{
	isSection = false;
	const char* pSecBegin = (*data == '[') ? data : NULL;
	if (pSecBegin != NULL)
	{
		const char* pSecEnd = strrchr(data, ']');
		if (pSecEnd != NULL && pSecEnd > (pSecBegin + 1))
		{
			isSection = true;
			unsigned int len = pSecEnd - pSecBegin - 1;
			char section[CfgKeyValueLen] = {0};
			memcpy(section, pSecBegin + 1, len);
			section[len] = '\0';
			ConfigData* cfgData = getSectionData(section);
			if (cfgData == NULL)
			{
				NEW(cfgData, ConfigData(section));
				cfgData->pNext = m_cfgData;
				m_cfgData = cfgData;
			}
			m_curCfgData = cfgData;
		}
	}
	
	return m_curCfgData;
}

const char* CCfg::getFileName() const
{
    return m_fileName;
}

void CCfg::reLoad()
{
	if (m_fileName[0] != '\0')
	{
		createKV(m_fileName, true);  // 重新读取创建配置项
		// output();
	}
}

// only for test
void CCfg::output()
{
	ReleaseInfoLog("\n");
	ConfigData* cfgData = m_cfgData;
	while (cfgData != NULL)
	{
		ReleaseInfoLog("CfgData = %p, Pointer = %p, Section = %s", cfgData, cfgData->section, cfgData->section);
		
		Key2Value* pOutput = cfgData->keyValue;
        while (pOutput != NULL)
		{
			ReleaseInfoLog("kv = %p, key = %s, value = %s", pOutput, pOutput->key, pOutput->value);
			pOutput = pOutput->pNext;
		}
		ReleaseInfoLog("\n");
		
		cfgData = cfgData->pNext;
	}
}


// 进程启动时先调用此函数初始化配置文件
static CCfg* pCfgObj = NULL;
CCfg* CCfg::initCfgFile(const char* pCfgFile, const char* key2value[], const int len)
{
	static CCfg cfgObj(pCfgFile, key2value, len);  // 全局唯一
	pCfgObj = &cfgObj;
	return pCfgObj;
}

const char* CCfg::getValue(const char* section, const char* key)
{
	return pCfgObj->get(section, key);
}

const Key2Value* CCfg::getValueList(const char* section)
{
	return pCfgObj->getList(section);
}

const char* CCfg::getKey(const char* section, const char* value)
{
	return pCfgObj->getKeyByValue(section, value);
}

const char* CCfg::getKey(const char* section, const int value)
{
	return pCfgObj->getKeyByValue(section, value);
}

CCfg* CCfg::reLoadCfg()
{
	pCfgObj->reLoad();
	return pCfgObj;
}

// 使用系统默认的配置
CCfg* CCfg::useDefaultCfg(const char* pCfgFile)
{
	// 默认配置！
	const char* cfgFileFullName = (pCfgFile != NULL) ? pCfgFile : "./config/common.cfg";   // 默认的配置文件全路径名
	const char* key2value[] = {
		"/* 配置文件说明 */\n\n"
		"\n// 日志配置项\n"
        "[Logger]  // 日志配置项\n",
		"DebugLogFile = ./logs/debug/debug\n",             // debug 日志文件路径名
		"ReleaseLogFile = ./logs/release/release\n",       // release 日志文件路径名
		"OptLogFile = ./logs/opt/opt\n",                   // operation 操作日志文件路径名
		"LogFileSize = 8388608    // 每个日志文件8M大小\n",                    // 日志文件最大容量
		"LogBakFileCount = 20     // 备份文件个数\n",                          // 最多可保存的备份日志文件个数
		"WriteDebugLog = 1        // 非0值打开调试日志，0值关闭调试日志\n\n",    // 是否输出debug日志，非0值输出，0值关闭
		
		"\n// 网络连接配置项，各项值必须大于1，否则配置错误\n",
        "[NetConnect]\n",
		"IP = 192.168.1.2               // 本节点IP\n",
		"Port = 60016                   // 本节点通信端口号，最小值不能小于2000，防止和系统端口号冲突\n",
		"ActiveTimeInterval = 604800    // 连接活跃检测间隔时间，单位秒，超过此间隔时间无数据则关闭连接\n",
		"HBInterval = 3600              // 心跳检测间隔时间，单位秒\n",
		"HBFailTimes = 10               // 心跳检测连续失败HBFailTimes次后关闭连接\n",
		"MsgQueueSize = 32768           // 连接消息队列的大小，最小值必须大于等于1024\n\n",
	};
	
	return initCfgFile(cfgFileFullName, key2value, (int)(sizeof(key2value) / sizeof(key2value[0])));
}


// 配置管理
NotifyFuncToCB CConfigManager::st_notifyFuncToCb;
bool CConfigManager::st_isInit = false;

void CConfigManager::notifyConfigUpdate(int sigNum, siginfo_t* sigInfo, void* context)
{
	for (NotifyFuncToCB::iterator it = st_notifyFuncToCb.begin(); it != st_notifyFuncToCb.end(); ++it)
	{
		it->first(it->second);
	}
}

void CConfigManager::addUpdateNotify(ConfigUpdateNotify cfgUpdateNotify, void* callBack)
{
	if (!st_isInit)
	{
		st_isInit = true;
		CProcess::installSignal(SignalNumber::ReloadConfig, notifyConfigUpdate);
	}
	
	if (cfgUpdateNotify != NULL) st_notifyFuncToCb[cfgUpdateNotify] = callBack;
}

void CConfigManager::removeUpdateNotify(ConfigUpdateNotify cfgUpdateNotify)
{
	st_notifyFuncToCb.erase(cfgUpdateNotify);
}


}



