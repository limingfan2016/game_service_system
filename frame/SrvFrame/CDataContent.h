
/* author : limingfan
 * date : 2015.07.27
 * description : 数据内容存储
 */

#ifndef CDATA_CONTENT_H
#define CDATA_CONTENT_H

#include <string>
#include <unordered_map>

#include "base/MacroDefine.h"
#include "base/CMemManager.h"


using namespace std;

namespace NFrame
{

class CModule;

static const unsigned int SrvIdLen = 16;
typedef unordered_map<unsigned int, char*> IndexToLocalAsyncData;          // 服务索引&本地异步数据

// 本地异步存储的内容
// 使用场景：发送异步消息后存储本地数据，等异步应答消息回来后可以方便的取出之前存储的数据
class CLocalAsyncData
{
public:
	CLocalAsyncData(CModule* module, const unsigned int bufferSize, const unsigned int bufferCount);
	~CLocalAsyncData();

public:
    char* createData();                                     // 创建数据缓存区，以便可以存储本地异步数据，高效率
	bool createData(const char* data, unsigned int len);    // 创建&存储本地异步数据，多了一次数据拷贝，低效率
	
	void destroyData(const char* data = NULL);              // 销毁&清空本地异步数据
	void destroyAllData();                                  // 销毁&清空所有本地异步数据
	
public:
	unsigned int getBufferSize() const;                     // 获取数据缓存区大小容量
	char* getData();                                        // 取得本地异步数据
	
private:
	void generateDataId();

private:
    NCommon::CMemManager m_memForData;
	unsigned int m_bufferSize;
	
	unsigned int m_index;
	IndexToLocalAsyncData m_idx2LocalAsyncData;
	
	CModule* m_module;
	char m_strSrvId[SrvIdLen];
	unsigned int m_srvIdLen;

	
DISABLE_CONSTRUCTION_ASSIGN(CLocalAsyncData);
};




typedef unordered_map<string, char*> SecondKeyToData;          // 二级key数据
typedef unordered_map<int, SecondKeyToData> FirstKeyToMap;     // 一级key数据

// 本地数据存储
class CDataContent
{
public:
	CDataContent(const unsigned int bufferSize, const unsigned int bufferCount);
	~CDataContent();
	
public:
    char* get(int firstKey, const char* secondKey, bool isRemove = true);
	char* get(int firstKey, const string& secondKey, bool isRemove = true);
	void set(int firstKey, const char* secondKey, char* data);
	void set(int firstKey, const string& secondKey, char* data);
	
	SecondKeyToData* get(int firstKey);
	void clear(int firstKey);

    void remove(int firstKey, const char* secondKey);
	void remove(int firstKey);
	void removeAll();
	
public:
	char* getBuffer();
	void putBuffer(char* buffer);
	unsigned int getBufferSize();
	
private:
    NCommon::CMemManager m_memForData;
	FirstKeyToMap m_firstKeyToData;

	
DISABLE_CONSTRUCTION_ASSIGN(CDataContent);
};

}

#endif // CDATA_CONTENT_H
