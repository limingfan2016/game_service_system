
/* author : limingfan
 * date : 2017.10.10
 * description : 随机数
 */

#include <stdlib.h>
#include <sys/time.h>
#include <algorithm>

#include "CRandomNumber.h"


namespace NProject
{

CRandomNumber::CRandomNumber()
{
}

CRandomNumber::~CRandomNumber()
{
}

// unsigned int 范围随机
unsigned int CRandomNumber::getUIntNumber(unsigned int min, unsigned int max)
{
	static bool IsInitRandSeed = false;
	if (!IsInitRandSeed)
	{
		IsInitRandSeed = true;
		
		// 初始化随机种子
		initRandSeed();
	}
	
	if (min > max)
	{
		unsigned int tmp = min;
		min = max;
		max = tmp;
	}

	return (rand() % (max - min + 1)) + min;
}

// int 范围随机
int CRandomNumber::getIntNumber(int min, int max)
{
	static bool IsInitRandSeed = false;
	if (!IsInitRandSeed)
	{
		IsInitRandSeed = true;
		
		// 初始化随机种子
		initRandSeed();
	}
	
	if (min > max)
	{
		int tmp = min;
		min = max;
		max = tmp;
	}

	return (rand() % (max - min + 1)) + min;
}

// float 范围随机
float CRandomNumber::getFloatNumber(float fMin, float fMax, const int ratio)
{
	static bool IsInitRandSeed = false;
	if (!IsInitRandSeed)
	{
		IsInitRandSeed = true;
		
		// 初始化随机种子
		initRandSeed();
	}
	
	if (ratio < 1) return 0.00;
	
	// 转换为整型运算
	int iMinValue = fMin * ratio;
	int iMaxValue = fMax * ratio;
	if (iMinValue > iMaxValue)
	{
		int tmp = iMinValue;
		iMinValue = iMaxValue;
		iMaxValue = tmp;
	}
	
	const int maxPercent = iMaxValue - iMinValue + 1;
	return (float)((rand() % maxPercent) + iMinValue) / (float)ratio;
}

// 字符串随机，生成 len - 1 个随机字符存储在buff中，以\0结束，成功则返回buff地址，失败返回NULL
const char* CRandomNumber::getChars(char* buff, const unsigned int len)
{
	if (buff != NULL && len > 0)
	{
		for (unsigned int i = 0; i < len - 1; ++i)
		{
			buff[i] = getIntNumber('a', 'z');
		}
		buff[len - 1] = '\0';
		
		return buff;
	}
	
	return NULL;
}

// 初始化随机种子
void CRandomNumber::initRandSeed()
{
	// 初始化随机种子
	struct timeval curTV;
	gettimeofday(&curTV, NULL);
	srand(curTV.tv_usec);
}


// needFilterNumber 需要过滤掉的随机数
bool CRandomNumber::initNoRepeatUIntNumber(unsigned int min, unsigned int max, const NumberVector* needFilterNumber, unsigned int minNumberCount)
{
	if (min > max)
	{
		unsigned int tmp = min;
		min = max;
		max = tmp;
	}

	m_noRepeatUIntNumbers.clear();
    const bool isNeedFilter = (needFilterNumber != NULL && !needFilterNumber->empty());
	for (unsigned int number = min; number <= max; ++number)
	{
		// 是否存在过滤数
		if (!isNeedFilter || std::find(needFilterNumber->begin(), needFilterNumber->end(), number) == needFilterNumber->end())
		{
			m_noRepeatUIntNumbers.push_back(number);
		}
	}
	
	return m_noRepeatUIntNumbers.size() >= minNumberCount;
}

// 获取不重复的 unsigned int 随机数
// 必须先调用 initNoRepeatUIntNumber 才能调用该函数
// 失败返回 (unsigned int)-1
unsigned int CRandomNumber::getNoRepeatUIntNumber()
{
	if (m_noRepeatUIntNumbers.empty()) return (unsigned int)-1;
	
	const unsigned int lastIdx = m_noRepeatUIntNumbers.size() - 1;
	const unsigned int randNumberIdx = getUIntNumber(0, lastIdx);
	const unsigned int randNumber = m_noRepeatUIntNumbers[randNumberIdx];
	
	m_noRepeatUIntNumbers[randNumberIdx] = m_noRepeatUIntNumbers[lastIdx];
	m_noRepeatUIntNumbers.pop_back();
	
	return randNumber;
}

}

