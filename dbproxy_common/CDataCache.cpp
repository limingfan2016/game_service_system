
/* author : limingfan
 * date : 2016.05.15
 * description : 快速读写存储内存数据，可替换memcached内存数据库作为本地缓存数据使用
 */

#include <string.h>
#include <time.h>

#include "CDataCache.h"
#include "../common/CommonType.h"


using namespace NCommon;

CDataCache::CDataCache(unsigned int size, unsigned int count, unsigned int step, bool isKeyNeedMD5) :
m_memForDataCache(count, step, size + sizeof(DataCacheContainer))
{
	m_cacheSize = size;
	m_isKeyNeedMD5 = isKeyNeedMD5;
}

CDataCache::~CDataCache()
{
	m_key2dataCache.clear();
	m_cacheSize = 0;
	m_isKeyNeedMD5 = false;
}

unsigned int CDataCache::getCacheSize() const
{
	return m_cacheSize;
}

unsigned int CDataCache::getFreeCount() const
{
	return m_memForDataCache.getFreeCount();
}

unsigned int CDataCache::getMaxCount() const
{
	return m_memForDataCache.getMaxCount();
}


bool CDataCache::get(const char* key, const unsigned int keyLen, char*& value, unsigned int& valueLen)
{
	if (key == NULL || keyLen < 1) return false;
	
	char md5Buff[64] = {0};
	if (m_isKeyNeedMD5)
	{
	    NProject::md5Encrypt(key, keyLen, md5Buff);  // 考虑到key可能存在特殊字符等，因此对key做MD5加密
		key = md5Buff;
    }
	
	KeyToDataCache::iterator it = m_key2dataCache.find(key);
	if (it != m_key2dataCache.end())
	{
		it->second->time = time(NULL);   // 刷新访问时间点
		value = it->second->data;
		valueLen = it->second->len;
		
		return true;
	}
	
	return false;
}

bool CDataCache::set(const char* key, const unsigned int keyLen, const char* value, unsigned int valueLen)
{
	if (key == NULL || keyLen < 1 || value == NULL || valueLen < 1 || valueLen > m_cacheSize)
	{
		OptErrorLog("set data to cache error, key = %p, keyLen = %u, value = %p, valueLen = %u, cache size = %u",
		key, keyLen, value, valueLen, m_cacheSize);
		
	    return false;
	}
	
	char md5Buff[64] = {0};
	if (m_isKeyNeedMD5)
	{
	    NProject::md5Encrypt(key, keyLen, md5Buff);  // 考虑到key可能存在特殊字符等，因此对key做MD5加密
		key = md5Buff;
    }
	
	// 先查找
	KeyToDataCache::iterator it = m_key2dataCache.find(key);
	DataCacheContainer* dataCacheContainer = NULL;
	if (it != m_key2dataCache.end())
	{
		dataCacheContainer = it->second;
	}
	else
	{
		// 不存在则分配内存块并保存key映射
		dataCacheContainer = (DataCacheContainer*)m_memForDataCache.get();
		m_key2dataCache[key] = dataCacheContainer;
	}

	memcpy(dataCacheContainer->data, value, valueLen);
	dataCacheContainer->len = valueLen;
	dataCacheContainer->time = time(NULL);  // 更新刷新时间点
	
	return true;
}

bool CDataCache::remove(const char* key, const unsigned int keyLen)
{
	if (key == NULL || keyLen < 1) return false;
	
	char md5Buff[64] = {0};
	if (m_isKeyNeedMD5)
	{
	    NProject::md5Encrypt(key, keyLen, md5Buff);  // 考虑到key可能存在特殊字符等，因此对key做MD5加密
		key = md5Buff;
    }

	KeyToDataCache::const_iterator it = m_key2dataCache.find(key);
	if (it != m_key2dataCache.end())
	{
		m_memForDataCache.put((char*)it->second);
		m_key2dataCache.erase(it);
		
		return true;
	}
	
	return false;
}

void CDataCache::removeExpiredCache(unsigned int expiredInterval)
{
	const unsigned int expiredTime = time(NULL) - expiredInterval;  // 数据块过期的最后时间点
	for (KeyToDataCache::iterator it = m_key2dataCache.begin(); it != m_key2dataCache.end();)
	{
		if (it->second->time < expiredTime)
		{
			OptWarnLog("remove expired data cache, key = %s", it->first.c_str());
			
			m_memForDataCache.put((char*)it->second);
			m_key2dataCache.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

