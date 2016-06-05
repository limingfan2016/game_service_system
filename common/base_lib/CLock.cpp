
/* author : limingfan
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


}

