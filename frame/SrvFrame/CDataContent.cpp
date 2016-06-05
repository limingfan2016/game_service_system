
/* author : limingfan
 * date : 2015.07.27
 * description : 数据内容存储
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "CDataContent.h"
#include "CModule.h"


using namespace NCommon;

namespace NFrame
{

const unsigned int MaxIdFlagIndex = 100000;  // 最大index
const unsigned int IdxBufferSize = sizeof(unsigned int);


CLocalAsyncData::CLocalAsyncData(CModule* module, const unsigned int bufferSize, const unsigned int bufferCount) : m_memForData(bufferCount, bufferCount, bufferSize + IdxBufferSize)
{
	m_bufferSize = bufferSize;
	m_index = 0;
	m_module = module;
	
	static unsigned int s_instanceId = 0;  // 实例ID值
	++s_instanceId;
	s_instanceId = s_instanceId % MaxIdFlagIndex;
	m_srvIdLen = snprintf(m_strSrvId, SrvIdLen - 1, "%u%u", m_module->getSrvId(), s_instanceId);
}

CLocalAsyncData::~CLocalAsyncData()
{
	clearAll();
	
	m_bufferSize = 0;
	m_index = 0;
	
	m_module = NULL;
	m_strSrvId[0] = '\0';
	m_srvIdLen = 0;
}

char* CLocalAsyncData::getBuffer()
{
	if (m_module->getContext().srvAsyncDataFlagLen > 0 && strstr(m_module->getContext().srvAsyncDataFlag, m_strSrvId) != NULL) return NULL;  // 已经存在了
	
	m_index = ++m_index % MaxIdFlagIndex;
	
	// 异步数据索引，服务ID&索引值
	char flagData[MaxLocalAsyncDataFlagLen] = {0};
	unsigned int flagLen = snprintf(flagData, MaxLocalAsyncDataFlagLen - 1, "%s%u|", m_strSrvId, m_index);
	
	unsigned int currentFlagLen = m_module->getContext().srvAsyncDataFlagLen;    // 当前标识数据长度
	if ((MaxLocalAsyncDataFlagLen - currentFlagLen) < flagLen) return NULL;      // 剩余buffer空间不足
		
	// 申请内存块空间，以便存储用户的本地异步数据
	char* buffer = m_memForData.get();
	if (buffer != NULL)
	{
		memcpy(((Context&)m_module->getContext()).srvAsyncDataFlag + currentFlagLen, flagData, flagLen + 1);
		((Context&)m_module->getContext()).srvAsyncDataFlagLen = currentFlagLen + flagLen;
		
		*((unsigned int*)buffer) = m_index;      // 内存块的头部存储索引值
		buffer = buffer + IdxBufferSize;
		m_idx2LocalAsyncData[m_index] = buffer;  // 存储buffer
	}
	
	return buffer;
}

unsigned int CLocalAsyncData::getBufferSize() const
{
	return m_bufferSize;
}

bool CLocalAsyncData::setData(const char* data, unsigned int len)
{
	if (data == NULL || len < 1 || len > m_bufferSize) return false;

	char* buffer = getBuffer();
	if (buffer != NULL) memcpy(buffer, data, len);
	
	return (buffer != NULL);
}

const char* CLocalAsyncData::getData()
{
	const char* srvId = strstr(m_module->getContext().srvAsyncDataFlag, m_strSrvId);
	if (srvId == NULL) return NULL;
	
	char* srvIdEnd = (char*)strchr(srvId + m_srvIdLen, '|');
	if (srvIdEnd == NULL) return NULL;
	
	*srvIdEnd = '\0';
	unsigned int idx = atoi(srvId + m_srvIdLen);
	*srvIdEnd = '|';
	
	IndexToLocalAsyncData::const_iterator dataIt = m_idx2LocalAsyncData.find(idx);
	return (dataIt != m_idx2LocalAsyncData.end()) ? dataIt->second : NULL;
}

void CLocalAsyncData::clearData(const char* data)
{
	if (data == NULL) data = getData();
	if (data != NULL)
	{
		data = data - IdxBufferSize;
		unsigned int idx = *((const unsigned int*)data);
		m_idx2LocalAsyncData.erase(idx);  // 删除数据关联
		m_memForData.put((char*)data);    // 释放内存空间块
		
		if (m_module->getContext().srvAsyncDataFlagLen > 0)
		{
			char flagData[MaxLocalAsyncDataFlagLen] = {0};
			unsigned int flagLen = snprintf(flagData, MaxLocalAsyncDataFlagLen - 1, "%s%u|", m_strSrvId, idx);
			char* srvIdFlag = (char*)strstr(m_module->getContext().srvAsyncDataFlag, flagData);
			if (srvIdFlag != NULL)
			{
				unsigned int remainLen = m_module->getContext().srvAsyncDataFlagLen - (srvIdFlag - m_module->getContext().srvAsyncDataFlag + flagLen) + 1;
				memmove(srvIdFlag, srvIdFlag + flagLen, remainLen);
				((Context&)m_module->getContext()).srvAsyncDataFlagLen = m_module->getContext().srvAsyncDataFlagLen - flagLen;
			}
		}
	}
}

void CLocalAsyncData::clearAll()
{
	for (IndexToLocalAsyncData::const_iterator dataIt = m_idx2LocalAsyncData.begin(); dataIt != m_idx2LocalAsyncData.end(); ++dataIt)
	{
		clearData(dataIt->second);
	}
	
	m_idx2LocalAsyncData.clear();
}




	
	
	
// 本地数据存储
CDataContent::CDataContent(const unsigned int bufferSize, const unsigned int bufferCount) : m_memForData(bufferCount, bufferCount, bufferSize)
{
}

CDataContent::~CDataContent()
{
}

char* CDataContent::get(int firstKey, const char* secondKey, bool isRemove)
{
	return get(firstKey, string(secondKey), isRemove);
}

char* CDataContent::get(int firstKey, const string& secondKey, bool isRemove)
{
	FirstKeyToMap::iterator firstKeyDataIt = m_firstKeyToData.find(firstKey);
	if (firstKeyDataIt == m_firstKeyToData.end()) return NULL;
	
	SecondKeyToData::iterator secondKeyDataIt = firstKeyDataIt->second.find(secondKey);
	if (secondKeyDataIt == firstKeyDataIt->second.end()) return NULL;
	
	char* data = secondKeyDataIt->second;
	if (isRemove) firstKeyDataIt->second.erase(secondKeyDataIt);
	
	return data;
}

void CDataContent::set(int firstKey, const char* secondKey, char* data)
{
	set(firstKey, string(secondKey), data);
}

void CDataContent::set(int firstKey, const string& secondKey, char* data)
{
	FirstKeyToMap::iterator firstKeyDataIt = m_firstKeyToData.find(firstKey);
	if (firstKeyDataIt != m_firstKeyToData.end())
	{
		firstKeyDataIt->second[secondKey] = data;
	}
	else
	{
		SecondKeyToData skData;
		skData[secondKey] = data;
		m_firstKeyToData[firstKey] = skData;
	}
}

SecondKeyToData* CDataContent::get(int firstKey)
{
	FirstKeyToMap::iterator firstKeyDataIt = m_firstKeyToData.find(firstKey);
	return (firstKeyDataIt != m_firstKeyToData.end()) ? &(firstKeyDataIt->second) : NULL;
}

void CDataContent::clear(int firstKey)
{
	FirstKeyToMap::iterator firstKeyDataIt = m_firstKeyToData.find(firstKey);
	if (firstKeyDataIt != m_firstKeyToData.end()) firstKeyDataIt->second.clear();
}

void CDataContent::remove(int firstKey, const char* secondKey)
{
    FirstKeyToMap::iterator firstKeyDataIt = m_firstKeyToData.find(firstKey);
	if (firstKeyDataIt != m_firstKeyToData.end())
	{
		SecondKeyToData::iterator secondKeyDataIt = firstKeyDataIt->second.find(secondKey);
	    if (secondKeyDataIt != firstKeyDataIt->second.end()) firstKeyDataIt->second.erase(secondKeyDataIt);
	}
}
 
void CDataContent::remove(int firstKey)
{
	FirstKeyToMap::iterator firstKeyDataIt = m_firstKeyToData.find(firstKey);
	if (firstKeyDataIt != m_firstKeyToData.end()) m_firstKeyToData.erase(firstKeyDataIt);
}

void CDataContent::removeAll()
{
	m_firstKeyToData.clear();
}


char* CDataContent::getBuffer()
{
	return m_memForData.get();
}

void CDataContent::putBuffer(char* buffer)
{
	m_memForData.put(buffer);
}

unsigned int CDataContent::getBufferSize()
{
	return m_memForData.getBuffSize();
}

}

