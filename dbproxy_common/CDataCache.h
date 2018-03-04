
/* author : limingfan
 * date : 2016.05.15
 * description : 快速读写存储内存数据，可替换memcached内存数据库作为本地缓存数据使用
 */
 
#ifndef _DATA_CACHE_H_
#define _DATA_CACHE_H_


#include <string>
#include <unordered_map>

#include "base/MacroDefine.h"
#include "base/CMemManager.h"


// 数据块容器
struct DataCacheContainer
{
	unsigned int time;         // 数据刷新时间点
	unsigned int len;          // 数据长度
	char data[4];              // 数据存储的起始地址，注意字节对齐
};
typedef std::unordered_map<std::string, DataCacheContainer*> KeyToDataCache;


class CDataCache
{
public:
	CDataCache(unsigned int size, unsigned int count, unsigned int step, bool isKeyNeedMD5 = false);
	~CDataCache();

public:
    unsigned int getCacheSize() const;
    unsigned int getFreeCount() const;
	unsigned int getMaxCount() const;
	
public:
	bool get(const char* key, const unsigned int keyLen, char*& value, unsigned int& valueLen);
	bool set(const char* key, const unsigned int keyLen, const char* value, unsigned int valueLen);
	
	bool remove(const char* key, const unsigned int keyLen);
	void removeExpiredCache(unsigned int expiredInterval);
	
private:
    NCommon::CMemManager m_memForDataCache;
	KeyToDataCache m_key2dataCache;
	unsigned int m_cacheSize;
	bool m_isKeyNeedMD5;
	
	
	DISABLE_CONSTRUCTION_ASSIGN(CDataCache);
};

#endif // _DATA_CACHE_H_

