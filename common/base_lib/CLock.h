
/* author : admin
 * date : 2014.10.19
 * description : 自动加解锁调用，防止处理流程中异常抛出而导致无解锁动作死锁
 */
 
#ifndef CLOCK_H
#define CLOCK_H

#include <mutex>


namespace NCommon
{

class CMutex;

class CLock
{
public:
	CLock(CMutex& rMutex);
	int Success;
	
	~CLock();

private:
    // 禁止拷贝、赋值
	CLock();
	CLock(const CLock&);
	CLock& operator =(const CLock&);
	
private:
	CMutex& m_rMutex;
};


class CLockEx
{
public:
	CLockEx(std::mutex* pMutex);
	~CLockEx();

    bool isLocked() const;
	void lock();
	void unlock();

private:
    // 禁止拷贝、赋值
	CLockEx();
	CLockEx(const CLockEx&);
	CLockEx& operator =(const CLockEx&);
	
private:
    int m_success;
	std::mutex* m_pMutex;
};

}

#endif // CLOCK_H
