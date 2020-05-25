
/* author : admin
 * date : 2014.10.19
 * description : 自动加解锁调用，防止处理流程中异常抛出而导致无解锁动作死锁
 */
 
#include "CLock.h"
#include "CMutex.h"

namespace NCommon
{

CLock::CLock(CMutex& rMutex) : m_rMutex(rMutex)
{
	Success = m_rMutex.lock();
}

CLock::~CLock()
{
	if (Success == 0) m_rMutex.unLock();
}


CLockEx::CLockEx(std::mutex* pMutex) : m_success(-1), m_pMutex(pMutex)
{
    lock();
}

CLockEx::~CLockEx()
{
	unlock();
}

bool CLockEx::isLocked() const
{
    return (m_success == 0);
}

void CLockEx::lock()
{
    if (m_pMutex != NULL)
    {
        m_pMutex->lock();
        m_success = 0;
    }
}

void CLockEx::unlock()
{
    if (m_success == 0)
    {
        m_pMutex->unlock();
        m_success = -1;
    }
}

}

