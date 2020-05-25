
/* author : admin
 * date : 2014.10.19
 * description : 内存监控、记录
 */
 
#ifndef CMEMMONITOR_H
#define CMEMMONITOR_H

#include <time.h>
#include <string>
#include <vector>
#include <unordered_map>


using namespace std;

namespace NCommon
{

// 内存信息
struct SMemInfo
{
    string fileName;
    unsigned int line;
    unsigned int size;
    unsigned int time;
    
    SMemInfo() {};
    SMemInfo(const char* _fileName, unsigned int _line, unsigned int _size, unsigned int _time) : fileName(_fileName), line(_line), size(_size), time(_time) {};
    ~SMemInfo() {};
};
typedef unordered_map<const void*, SMemInfo> AddressToMemInfo;


// 内存池信息
struct SMemPoolInfo
{
    unsigned int count;
    unsigned int size;
    unsigned int time;
    
    SMemPoolInfo() {};
    SMemPoolInfo(unsigned int _count, unsigned int _size, unsigned int _time) : count(_count), size(_size), time(_time) {};
    ~SMemPoolInfo() {};
};
typedef unordered_map<const void*, SMemPoolInfo> AddressToMemPoolInfo;


// 内存操作统计信息
struct SMemOptStatInfo
{
    unsigned int newCount;
    unsigned int delCount;
    unsigned int allSize;

    unsigned int poolNew;
    unsigned int poolDel;
    unsigned int poolSize;

    SMemOptStatInfo() : newCount(0), delCount(0), allSize(0), poolNew(0), poolDel(0), poolSize(0) {};
    ~SMemOptStatInfo() {};
};


class CMemMonitor
{
public:
    static CMemMonitor& getInstance();
    
public:
    int setOutputValue(const char* switchValue = NULL);
    void outputMemInfo();

public:
    inline const AddressToMemInfo& getMemInfo()
    {
        return m_address2MemInfo;
    }
    
    inline const AddressToMemPoolInfo& getMemPoolInfo()
    {
        return m_address2MemPoolInfo;
    }
    
    inline const SMemOptStatInfo& getMemOptStatInfo()
    {
        return m_memOptStatInfo;
    }
    
public:
    inline void newMemInfo(const void* address, const char* fileName, unsigned int line, unsigned int size)
    {
        ++m_memOptStatInfo.newCount;
        m_memOptStatInfo.allSize += size;

        if (m_isOutput != 1) return;
        
        m_address2MemInfo[address] = SMemInfo(fileName, line, size, time(NULL));
    }
    
    inline void deleteMemInfo(const void* address)
    {
        ++m_memOptStatInfo.delCount;
        
        if (m_isOutput != 1) return;
        
        m_address2MemInfo.erase(address);
    }
    
    inline void newMemPoolInfo(const void* address, unsigned int count, unsigned int size)
    {
        ++m_memOptStatInfo.poolNew;
        m_memOptStatInfo.poolSize += (count * size);

        if (m_isOutput != 1) return;
        
        m_address2MemPoolInfo[address] = SMemPoolInfo(count, size, time(NULL));
    }
    
    inline void deleteMemPoolInfo(const void* address)
    {
        ++m_memOptStatInfo.poolDel;

        if (m_isOutput != 1) return;
        
        m_address2MemPoolInfo.erase(address);
    }

private:
    int m_isOutput;
    AddressToMemInfo m_address2MemInfo;
    AddressToMemPoolInfo m_address2MemPoolInfo;
    SMemOptStatInfo m_memOptStatInfo;

private:
    // 禁止拷贝、赋值
    CMemMonitor() : m_isOutput(1) {};
	CMemMonitor(const CMemMonitor&);
	CMemMonitor& operator =(const CMemMonitor&);

};

}

#endif // CMEMMONITOR_H
