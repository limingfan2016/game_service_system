
/* author : admin
 * date : 2015.01.09
 * description : 定时器
 */
 
#ifndef __CTIMER_H__
#define __CTIMER_H__

#include <vector>
#include <unordered_map>
#include "CThread.h"

namespace NCommon
{

	class CMutex;
	class CMemManager;

	class CTimerI
	{
	public:
		virtual bool OnTimer(unsigned int timerId, void *pParam, unsigned int remainCount) = 0;
	};

	class STimerInfo
	{
	public:
		unsigned int timerId;
		CTimerI * pTimerI;		//回调类指针
		unsigned int runCounts; // -1代表无限次
		unsigned int timeGap; //执行时间间隔，单位ms
		void * pParam;
		long long lastExeTimestamp; //上次的执行时间戳，单位ms

	public:
		STimerInfo()
		{
			timerId = 0;
			pTimerI = (CTimerI *)0;
			runCounts = 1;
			timeGap = 100;
			pParam = (void *)0;
			lastExeTimestamp = 0;
		}
	};

	class CTimer : public CThread
	{
	public:
		CTimer();
		~CTimer();
		
		int startTimer();
		
		//设置定时器，时间单位ms，最小粒度100ms，runCounts 为-1时代表无限次，返回值 timerId 非0
		unsigned int setTimer(unsigned int timeGap, CTimerI* pTimerI, void *pParam = (void *)0, unsigned int runCounts = 1);
		void killTimer(unsigned int timerId);
		void clearAllTimer();
		int stopTimer();

	private:
		// 禁止拷贝、赋值
		CTimer(const CTimer&);
		CTimer& operator =(const CTimer&);
		virtual void run();  // 线程实现者重写run
		int insertTimer(STimerInfo *pTimerInfo);
		int eraseTimer(unsigned int index);
		
		inline int cmpTimerInfo(STimerInfo *pTimerInfo_1, STimerInfo *pTimerInfo_2)
		{
			long long timeVal1 = pTimerInfo_1->lastExeTimestamp + pTimerInfo_1->timeGap;
			long long timeVal2 = pTimerInfo_2->lastExeTimestamp + pTimerInfo_2->timeGap;
			return (timeVal1 < timeVal2) ? -1 : 1;
			// return int(pTimerInfo_1->lastExeTimestamp + pTimerInfo_1->timeGap - pTimerInfo_2->lastExeTimestamp - pTimerInfo_2->timeGap);
		}
		
		inline void swapTimerInfo(std::vector<STimerInfo*> &m_vTimer, unsigned int indexA, unsigned int indexB)
		{
			STimerInfo *tmp = m_vTimer[indexA];
			m_vTimer[indexA] = m_vTimer[indexB];
			m_vTimer[indexB] = tmp;
		}

	private:
		unsigned int m_curTimerId;
		CMemManager *m_pMemManager;
		CMutex *m_pMutex;
		std::unordered_map<unsigned int, bool> m_umapKillTimer;
		std::vector<STimerInfo*> m_vTimer;
		volatile bool m_isRun;
	};

}

#endif // CTIMER_H


