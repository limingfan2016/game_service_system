
/* author : limingfan
 * date : 2017.10.10
 * description : 随机数
 */

#ifndef __RANDOM_NUMBER_H__
#define __RANDOM_NUMBER_H__

#include <vector>
#include "base/MacroDefine.h"


namespace NProject
{

typedef std::vector<unsigned int> NumberVector;

class CRandomNumber
{
public:
	CRandomNumber();
	~CRandomNumber();

public:
    // unsigned int 范围随机
    static unsigned int getUIntNumber(unsigned int min, unsigned int max);
	
	// int 范围随机
    static int getIntNumber(int min, int max);
	
	// float 范围随机
    static float getFloatNumber(float fMin, float fMax, const int ratio = 1000);
	
	// 字符串随机，生成 len - 1 个随机字符存储在buff中，以\0结束，成功则返回buff地址，失败返回NULL
    static const char* getChars(char* buff, const unsigned int len);

    // 初始化随机种子
	static void initRandSeed();
	
public:
    // needFilterNumber 需要过滤掉的随机数
	bool initNoRepeatUIntNumber(unsigned int min, unsigned int max, const NumberVector* needFilterNumber = NULL, unsigned int minNumberCount = 100);
	
	// 获取不重复的 unsigned int 随机数
	// 必须先调用 initNoRepeatUIntNumber 才能调用该函数
	// 失败返回 (unsigned int)-1
	unsigned int getNoRepeatUIntNumber();

private:
    NumberVector m_noRepeatUIntNumbers;


	DISABLE_COPY_ASSIGN(CRandomNumber);
};

}


#endif // __RANDOM_NUMBER_H__
