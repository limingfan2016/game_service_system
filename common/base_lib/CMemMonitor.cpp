
/* author : admin
 * date : 2014.10.19
 * description : 内存监控、记录
 */

#include <map>

#include "MacroDefine.h"
#include "CCfg.h"
#include "CMemMonitor.h"


namespace NCommon
{

CMemMonitor& CMemMonitor::getInstance()
{
    static CMemMonitor instance;
    return instance;
}
    
int CMemMonitor::setOutputValue(const char* switchValue)
{
    if (switchValue == NULL)
    {
        // 输出内存信息日志开关配置信息
        const char* loggerSection = "Logger";
        const char* outputMemInfoSwitchItem = "OutputMemInfo";
        switchValue = CCfg::getValue(loggerSection, outputMemInfoSwitchItem);
    }

    m_isOutput = (switchValue == NULL) ? 0 : atoi(switchValue);
    if (m_isOutput != 1)
    {
        // 关闭内存操作记录
        m_address2MemInfo.clear();
        m_address2MemPoolInfo.clear();
    }
    
    return m_isOutput;
}
    
void CMemMonitor::outputMemInfo()
{
    if (m_isOutput != 1) return;

    const AddressToMemInfo& memInfo = getMemInfo();
    typedef vector<const SMemInfo*> SMemInfoVector;
    typedef std::map<unsigned int, SMemInfoVector> TimeToSMemInfoMap;
    TimeToSMemInfoMap time2MemInfo;
    unsigned int memSize = 0;

    for (AddressToMemInfo::const_iterator memIt = memInfo.begin(); memIt != memInfo.end(); ++memIt)
    {
        memSize += memIt->second.size;
        time2MemInfo[memIt->second.time].push_back(&memIt->second);
    }
    
    const AddressToMemPoolInfo& memPoolInfo = getMemPoolInfo();
    typedef vector<const SMemPoolInfo*> SMemPoolVector;
    typedef std::map<unsigned int, SMemPoolVector> TimeToSMemPoolMap;
    TimeToSMemPoolMap time2MemPool;
    unsigned int poolSize = 0;
    
    for (AddressToMemPoolInfo::const_iterator memPoolIt = memPoolInfo.begin(); memPoolIt != memPoolInfo.end(); ++memPoolIt)
    {
        poolSize += (memPoolIt->second.size * memPoolIt->second.count);
        time2MemPool[memPoolIt->second.time].push_back(&memPoolIt->second);
    }
    
    ReleaseInfoLog("---------- begin output memory info ----------");
    
    time_t lastTimeSecs = 0;
    for (TimeToSMemInfoMap::const_iterator tmIt = time2MemInfo.begin(); tmIt != time2MemInfo.end(); ++tmIt)
    {
        if (tmIt->first > lastTimeSecs) lastTimeSecs = tmIt->first;
    
        for (SMemInfoVector::const_iterator memIt = tmIt->second.begin(); memIt != tmIt->second.end(); ++memIt)
        {
            ReleaseInfoLog("mem|%u|%u|%u|%s", tmIt->first, (*memIt)->size, (*memIt)->line, (*memIt)->fileName.c_str());
        }
    }
    
    for (TimeToSMemPoolMap::const_iterator tpIt = time2MemPool.begin(); tpIt != time2MemPool.end(); ++tpIt)
    {
        if (tpIt->first > lastTimeSecs) lastTimeSecs = tpIt->first;
    
        for (SMemPoolVector::const_iterator pIt = tpIt->second.begin(); pIt != tpIt->second.end(); ++pIt)
        {
            ReleaseInfoLog("pool|%u|%u|%u", tpIt->first, (*pIt)->size, (*pIt)->count);
        }
    }

    struct tm lastTimeInfo;
    struct tm* pLastTm = localtime_r(&lastTimeSecs, &lastTimeInfo);
    ReleaseInfoLog("memory stage info, last time = %u, [%d-%02d-%02d %02d:%02d:%02d], size = %u, count = %u, pool size = %u, count = %u, diff = %u",
    lastTimeSecs, (pLastTm->tm_year + 1900), (pLastTm->tm_mon + 1), pLastTm->tm_mday, pLastTm->tm_hour, pLastTm->tm_min, pLastTm->tm_sec,
    memSize, memInfo.size(), poolSize, memPoolInfo.size(), memSize - poolSize);

    const SMemOptStatInfo& memOptStatInfo = getMemOptStatInfo();
    ReleaseInfoLog("memory all info, size = %u, new = %u, delete = %u, pool size = %u, new = %u, delete = %u, diff = %u",
    memOptStatInfo.allSize, memOptStatInfo.newCount, memOptStatInfo.delCount,
    memOptStatInfo.poolSize, memOptStatInfo.poolNew, memOptStatInfo.poolDel,
    memOptStatInfo.allSize - memOptStatInfo.poolSize);
    
    ReleaseInfoLog("---------- end output memory info ----------");
}

}

