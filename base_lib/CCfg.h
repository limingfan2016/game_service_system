//test only
/* author : limingfan
 * date : 2014.10.27
 * description : 配置文件解析读取配置项
 */
 
#ifndef CCFG_H
#define CCFG_H

#include <string.h>
#include <unordered_map>
#include "Constant.h"
#include "MacroDefine.h"
#include "CProcess.h"


typedef void (*ConfigUpdateNotify)(void* callBack);  // 配置文件更新通知接口
typedef std::unordered_map<ConfigUpdateNotify, void*> NotifyFuncToCB;

namespace NCommon
{

struct Key2Value
{
	char key[CfgKeyValueLen];
	char value[CfgKeyValueLen];
	Key2Value* pNext;
	
	Key2Value(const char* k, unsigned int kLen, const char* val, unsigned int vLen) : pNext(NULL)
	{
		if (kLen >= (unsigned int)CfgKeyValueLen) kLen = CfgKeyValueLen - 1;
		if (vLen >= (unsigned int)CfgKeyValueLen) vLen = CfgKeyValueLen - 1;
		
		memcpy(key, k, kLen);
		key[kLen] = '\0';
		
		memcpy(value, val, vLen);
		value[vLen] = '\0';
	}
	
	~Key2Value()
	{
		key[0] = '\0';
		value[0] = '\0';
		pNext = NULL;
	}
};

struct ConfigData;


// 配置文件格式如下：
/*
配置文件说明、注释信息等等
[cfgSection1]  注意说明！配置文件的配置项必须在[section]下，否则配置格式错误
key = value  // 注释信息
key1 = value
......

[cfgSection2]
key = value
......
*/

class CCfg
{
public:
	CCfg(const char* pCfgFile, const char* key2value[], const int len);
	~CCfg();

public:
    static CCfg* initCfgFile(const char* pCfgFile, const char* key2value[], const int len);  // 进程启动时先调用此函数初始化配置文件
	static CCfg* useDefaultCfg(const char* pCfgFile = NULL);                                 // 使用系统默认的配置
	static CCfg* reLoadCfg();                                                                // 重新加载更新配置项

    // 获取配置项值
	static const char* getValue(const char* section, const char* key);
	static const Key2Value* getValueList(const char* section);
	static const char* getKey(const char* section, const char* value);
	static const char* getKey(const char* section, const int value);

public:
	const char* get(const char* section, const char* key);
	const Key2Value* getList(const char* section);
	const char* getKeyByValue(const char* section, const char* value);
	const char* getKeyByValue(const char* section, const int value);
	
	void reLoad();

public:
    static int createDir(const char* pCfgFile);
	
private:
	int createFile(const char* pCfgFile, const char* key2value[], const int len);
	void createKV(const char* pCfgFile, bool isUpdate = false);
	void destroyKV();
	void add(const char* key2value, bool isUpdate = false);
	const char* getItem(const char* section, const char* value, const bool isValue = true);
	
	ConfigData* getSectionData(const char* section);
	ConfigData* getCurCfgData(const char* data, bool& isSection);
	
private:
	void output();  // only for test
	
private:
    // 禁止拷贝、赋值
	CCfg();
	CCfg(const CCfg&);
	CCfg& operator =(const CCfg&);
	
private:
	ConfigData* m_cfgData;
	ConfigData* m_curCfgData;
	char m_fileName[MaxFullLen];
};


// 配置管理
class CConfigManager
{
public:
	static void addUpdateNotify(ConfigUpdateNotify cfgUpdateNotify, void* callBack = NULL);
	static void removeUpdateNotify(ConfigUpdateNotify cfgUpdateNotify);
	
private:
	static void notifyConfigUpdate(int sigNum, siginfo_t* sigInfo, void* context);

private:
    static NotifyFuncToCB st_notifyFuncToCb;
	static bool st_isInit;
	
DISABLE_CLASS_BASE_FUNC(CConfigManager);  // 禁止实例化对象
};

}

#endif // CCFG_H
