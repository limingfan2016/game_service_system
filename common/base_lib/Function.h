
/* author : admin
 * date : 2015.3.9
 * description : 通用函数
 */
 
#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include <openssl/aes.h>
#include <cstdint>


namespace NCommon
{
	int b2str(const char *buf, const int bufLen, char *str, const int strLen, bool capital = true); //将buf转为16进制字串
	bool strIsDigit(const char *str, const int strLen = 0); //判断字串是否为纯数字
	bool strIsAlpha(const char *str, const int strLen = 0); //判断字串是否为纯字母
	bool strIsAlnum(const char *str, const int strLen = 0); //判断字串是否为数字或字母
    bool strIsAlnumOrNonAscii(const char *str, const int strLen = 0); //判断字串是否为数字或字母或非ascii(比如汉字等)
    bool strIsSqlchar(const char *str, const int strLen = 0); //判断字串是否为ascii内sql合法字串
    bool strIsSqlcharOrNonAscii(const char *str, const int strLen = 0); //判断字串是否为ascii内sql合法字串或非ascii(比如汉字等)
	bool strIsUpperHex(const char *str, const int strLen = 0); //判断字串是否为大写的16进制字串（0~9 或 A~F）
	bool strIsDate(const char *str, const int strLen = 0); //判断字串是否为 日期格式
	unsigned int strToHashValue(const char *str, const int strLen = 0); //将一个字符串（或2进制buf）转为uint32类型并返回
    unsigned int strToHashValueCaseInsensitive(const char *str, const int strLen = 0); //将一个字符串（不分区大小写）转为uint32类型并返回
    unsigned int genChecksum(const char* buf, const int bufLen); // 计算二进制内容的checksum
    void simpleEncryptDecrypt(char* buf, const int bufLen);  // 简单的对称加密算法, buf即是输入，也是输出
    bool aesEncrypt(char* buf, int bufLen, const char key[AES_BLOCK_SIZE]); // aes加密，bufLen为16的倍数
    bool aesDecrypt(char* buf, int bufLen, const char key[AES_BLOCK_SIZE]); // aes解密，bufLen为16的倍数
    uint64_t getMilliSecsTimestamp();  // 当前时间毫秒数
    char* filterInvalidChars(char* charString);  // 过滤无效的不可见字符
    bool isValidIPv4AddressFormat(const char* ipStr);  // 判断是否为有效的IPv4地址格式
}

#endif // __FUNCTION_H__
