
/* author : liuxu
 * date : 2015.3.9
 * description : 通用函数
 */
 
#ifndef __FUNCTION_H__
#define __FUNCTION_H__

namespace NCommon
{
	int b2str(const char *buf, const int bufLen, char *str, const int strLen, bool capital = true); //将buf转为16进制字串
	bool strIsDigit(const char *str, const int strLen = 0); //判断字串是否为纯数字
	bool strIsAlpha(const char *str, const int strLen = 0); //判断字串是否为纯字母
	bool strIsAlnum(const char *str, const int strLen = 0); //判断字串是否为数字或字母
	bool strIsUpperHex(const char *str, const int strLen = 0); //判断字串是否为大写的16进制字串（0~9 或 A~F）
	bool strIsDate(const char *str, const int strLen = 0); //判断字串是否为 日期格式
	unsigned int strToHashValue(const char *str, const int strLen = 0); //将一个字符串（或2进制buf）转为uint32类型并返回
}

#endif // __FUNCTION_H__
