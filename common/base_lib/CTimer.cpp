
/* author : admin
* date : 2015.01.09
* description : 定时器
*/

#include <sys/time.h>
#include <math.h>
#include <queue>
#include <unistd.h>
#include "CTimer.h"
#include "CThread.h"
#include "CMemManager.h"
#include "CMutex.h"
#include "MacroDefine.h"
#include "CLock.h"


namespace NCommon
{
	const unsigned int TimerMemBlockCount = 1024;  // 定时器内存块数量
	
	CTimer::CTimer()
	{
		m_isRun = false;
		m_curTimerId = 0;
		NEW(m_pMemManager, CMemManager(TimerMemBlockCount, TimerMemBlockCount, sizeof(STimerInfo)));
		NEW(m_pMutex, CMutex(MutexType::recursiveMutexType));
		m_vTimer.reserve(TimerMemBlockCount);
	}

	CTimer::~CTimer()
	{
        m_isRun = false;
		m_curTimerId = 0;

		DELETE(m_pMemManager);
		DELETE(m_pMutex);
	}

	int CTimer::startTimer()
	{
		m_isRun = true;
		int rc = start();
		if (rc != 0)
		{
			return rc;
		}
		//add other

		return rc;
	}

	//设置定时器，时间单位ms，最小粒度100ms，返回值 timerId 非0
	unsigned int CTimer::setTimer(unsigned int timeGap, CTimerI* pTimerI, void *pParam, unsigned int runCounts)
	{
		if (runCounts == 0) return 0;  // 无效参数，return 0;表示setTimer失败
		
		CLock lock(*m_pMutex);
		
		// 存在一种极端情况，触发OnTimer之后，上层再调用killTimer(timerId)会导致已经触发了的timerId永久存储在底层的m_umapKillTimer map里，造成内存泄漏
		// 更极端的是，timer id值unsigned int被++循环一遍之后，新设置的timer id值m_curTimerId和被删除的timerId值相同，则新设置的timer会被误当做被killTimer的id处理因此不会被触发，导致错误
		// 解决方案一：timer生成的id唯一且随机，或者timer id改为unsigned long long超大类型，此方案只能降低出错概率；
		// 解决方案二：setTimer时检查新生成的id是否在killTimer m_umapKillTimer map里，如果存在则从map里删除
		if (++m_curTimerId == 0) m_curTimerId = 1;
		
		// 出现上述错误的情况概率极低，先屏蔽解决方案
		// unsigned int 最大值42亿，假设每秒一次setTimer调用也需要133年才一个轮回
		// std::unordered_map<unsigned int, bool>::iterator killTimerIt = m_umapKillTimer.find(m_curTimerId);
		// if (killTimerIt != m_umapKillTimer.end()) m_umapKillTimer.erase(killTimerIt);
		
		STimerInfo *pTimerInfo = (STimerInfo*)m_pMemManager->get();
		pTimerInfo->timerId = m_curTimerId;
		pTimerInfo->pTimerI = pTimerI;
		pTimerInfo->runCounts = runCounts;
		if (timeGap < 100)
		{
			timeGap = 100;  // 支持的最小单位100毫秒
		}
		pTimerInfo->timeGap = timeGap;
		pTimerInfo->pParam = pParam;
		struct timeval nowTime;
		gettimeofday(&nowTime, NULL);
		pTimerInfo->lastExeTimestamp = nowTime.tv_sec * 1000 + nowTime.tv_usec / 1000;

		insertTimer(pTimerInfo);

		return pTimerInfo->timerId;
	}


	void CTimer::killTimer(unsigned int timerId)
	{
        if (timerId > 0)
        {
            CLock lock(*m_pMutex);
		    m_umapKillTimer[timerId] = true;
        }
	}

	int CTimer::stopTimer()
	{
		m_isRun = false;
		join(getId(), NULL);

		return 0;
	}

	void CTimer::clearAllTimer()
	{
		CLock lock(*m_pMutex);
		m_umapKillTimer.clear();
		for (std::vector<STimerInfo*>::iterator ite = m_vTimer.begin();
			ite != m_vTimer.end();
			ite++)
		{
			m_pMemManager->put((char*)(*ite));
		}
		m_pMemManager->free();
		m_vTimer.clear();
	}
	
	void CTimer::run()
	{
		while (m_isRun)
		{
			struct timeval nowTime;
			gettimeofday(&nowTime, NULL);
			long long nowMs = nowTime.tv_sec * 1000 + nowTime.tv_usec / 1000;

			//扫描到期的定时器
			bool isRun = false;
			unsigned int curIndex = 0;
			while (m_isRun)
			{
				CLock lock(*m_pMutex);
				if (m_vTimer.size() > curIndex && m_vTimer[curIndex]->lastExeTimestamp + m_vTimer[curIndex]->timeGap <= nowMs + 5)
				{
					isRun = true;
					STimerInfo *pTimerInfo = m_vTimer[curIndex];
					eraseTimer(curIndex);

					//判断是否已被killtimer
					std::unordered_map<unsigned int, bool>::iterator killTimerIt = m_umapKillTimer.find(pTimerInfo->timerId);
					if (killTimerIt != m_umapKillTimer.end())
					{
						//释放
						m_umapKillTimer.erase(killTimerIt);
						m_pMemManager->put((char*)pTimerInfo);
						continue;
					}

					if (pTimerInfo->runCounts > 1)
					{
						//再放回timer数组里
						if (pTimerInfo->runCounts != (unsigned int)(-1))
						{
							pTimerInfo->runCounts--;
						}
						pTimerInfo->lastExeTimestamp = nowMs;
						insertTimer(pTimerInfo);

						pTimerInfo->pTimerI->OnTimer(pTimerInfo->timerId, pTimerInfo->pParam, pTimerInfo->runCounts);
					}
					else if (pTimerInfo->runCounts == 1)
					{
						pTimerInfo->runCounts--;

						pTimerInfo->pTimerI->OnTimer(pTimerInfo->timerId, pTimerInfo->pParam, pTimerInfo->runCounts);
						
						// 存在应用上层有可能在 OnTimer 回调里调用killtimer pTimerInfo->timerId的情况，则此处必须从m_umapKillTimer中删除，否则会导致内存泄漏
						killTimerIt = m_umapKillTimer.find(pTimerInfo->timerId);
						if (killTimerIt != m_umapKillTimer.end()) m_umapKillTimer.erase(killTimerIt);
						
						m_pMemManager->put((char*)pTimerInfo);
					}
					else //直接释放,理论上不会走到这
					{
						m_pMemManager->put((char*)pTimerInfo);
					}
				}
				else
				{
					break;
				}
			}

			if (isRun)
			{
				isRun = false;
			}
			else
			{
				usleep(10 * 1000);
			}
		}
	}

	int CTimer::insertTimer(STimerInfo *pTimerInfo)
	{
		unsigned int curIndex = m_vTimer.size();
		m_vTimer.push_back(pTimerInfo);

		unsigned int parentIndex = 0;
		for (; curIndex > 0; curIndex = parentIndex)
		{
			parentIndex = (curIndex-1) / 2;

			//新插入的timer比它的父节点要早执行
			if (cmpTimerInfo(m_vTimer[curIndex], m_vTimer[parentIndex]) < 0)
			{
				swapTimerInfo(m_vTimer, curIndex, parentIndex);
			}
			else
			{
				break;
			}
		}

		return 0;
	}

	int CTimer::eraseTimer(unsigned int index)
	{
		if (index >= m_vTimer.size())
		{
			return -1;
		}

		//将最后一个节点挪到被删除点的位置
		m_vTimer[index] = m_vTimer[m_vTimer.size() - 1];
		m_vTimer.pop_back();

		unsigned int parentIndex = (index-1) / 2;
		unsigned int curIndex = index;
		int cmpRel = 0;
		//自己就是根节点，只与叶子结点对比
		if (curIndex == 0)
		{
			cmpRel = 1;
		}
		else
		{
			cmpRel = cmpTimerInfo(m_vTimer[curIndex], m_vTimer[parentIndex]);
		}

		//比根早，往根节点挪
		if (cmpRel < 0)
		{
			swapTimerInfo(m_vTimer, curIndex, parentIndex);
			curIndex = parentIndex;
			for (; curIndex > 0; curIndex = parentIndex)
			{
				parentIndex = (curIndex-1) / 2;

				//新插入的timer比它的父节点要早执行
				if (cmpTimerInfo(m_vTimer[curIndex], m_vTimer[parentIndex]) < 0)
				{
					swapTimerInfo(m_vTimer, curIndex, parentIndex);
				}
				else
				{
					break;
				}
			}
		}
		else if (cmpRel == 0)	//与根节点一样大，不挪
		{
			return 0;
		}
		else  //比根晚，往叶子节点挪
		{
			unsigned int mSize = m_vTimer.size();
			unsigned int childIndex = 0;
			for (; (curIndex + 1) * 2 <= mSize; curIndex = childIndex)
			{
				//获取左儿子
				childIndex = curIndex * 2 + 1;
				//如果有右儿子，则取早执行的那个
				if (mSize > childIndex + 1)
				{
					if (cmpTimerInfo(m_vTimer[childIndex], m_vTimer[childIndex + 1]) > 0)
					{
						childIndex++;
					}
				}

				//如果比儿子晚执行，就交换
				if (cmpTimerInfo(m_vTimer[curIndex], m_vTimer[childIndex]) > 0)
				{
					swapTimerInfo(m_vTimer, curIndex, childIndex);
				}
				else//比儿子早，就结束
				{
					break;
				}
			}
		}

		return 0;
	}
}

