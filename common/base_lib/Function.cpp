
/* author : admin
 * date : 2015.3.9
 * description : 通用函数
 */
 
#include "Function.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <openssl/md5.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <vector>


namespace NCommon
{
	int b2str(const char* buf, const int bufLen, char *str, const int strLen, bool capital)
	{
		const char* format = capital ? "%02X" : "%02x";
		int index = 0;
		for (int i = 0; i < bufLen && index < strLen - 2; i++, index += 2)
		{
			sprintf(str + index, format, (unsigned char)buf[i]);
		}
		str[index] = '\0';
		return index;
	}

	bool strIsDigit(const char *str, const int strLen)
	{
		int len = strLen ? strLen : (int)strlen(str);
		for (int i = 0; i < len; i++)
		{
			if (!isdigit(str[i]))
			{
				return false;
			}
		}

		return true;
	}

	bool strIsAlpha(const char *str, const int strLen)
	{
		int len = strLen ? strLen : (int)strlen(str);
		for (int i = 0; i < len; i++)
		{
			if (!isalpha(str[i]))
			{
				return false;
			}
		}

		return true;
	}

	bool strIsAlnum(const char *str, const int strLen)
	{
		int len = strLen ? strLen : (int)strlen(str);
		for (int i = 0; i < len; i++)
		{
			if (!isalnum(str[i]))
			{
				return false;
			}
		}

		return true;
	}
    
    bool strIsAlnumOrNonAscii(const char *str, const int strLen)
    {
        int len = strLen ? strLen : (int)strlen(str);
		for (int i = 0; i < len; i++)
		{
			if (!(isalnum(str[i]) || str[i] < 0))
			{
				return false;
			}
		}

		return true;
    }
    
    bool strIsSqlchar(const char *str, const int strLen)
    {
        static bool isInit = false;
        static bool arrFlag[256] = {0};
        if (!isInit)
        {
            for (unsigned char i = 33; i < 127; ++i)
                arrFlag[i] = true;
            arrFlag['"'] = false;
            arrFlag['\''] = false;
            arrFlag['\\'] = false;
            arrFlag['`'] = false;
            
            isInit = true;
        }
        
        int len = strLen ? strLen : (int)strlen(str);
		for (int i = 0; i < len; i++)
		{
			if (!arrFlag[(unsigned char)str[i]])
			{
				return false;
			}
		}

		return true;
    }
    
    bool strIsSqlcharOrNonAscii(const char *str, const int strLen)
    {
        static bool isInit = false;
        static bool arrFlag[256] = {0};
        if (!isInit)
        {
            for (int i = 33; i < 256; ++i)
                arrFlag[i] = true;
            arrFlag['"'] = false;
            arrFlag['\''] = false;
            arrFlag['\\'] = false;
            arrFlag['`'] = false;
            arrFlag[127] = false;
            
            isInit = true;
        }
        
        int len = strLen ? strLen : (int)strlen(str);
		for (int i = 0; i < len; i++)
		{
			if (!arrFlag[(unsigned char)str[i]])
			{
				return false;
			}
		}

		return true;
    }

	bool strIsUpperHex(const char *str, const int strLen)
	{
		int len = strLen ? strLen : (int)strlen(str);
		for (int i = 0; i < len; i++)
		{
			if (!(isdigit(str[i]) || (str[i] >= 'A' && str[i] <= 'F')))
			{
				return false;
			}
		}

		return true;
	}

	bool strIsDate(const char *str, const int strLen)
	{
		int len = strLen ? strLen : (int)strlen(str);
		if (10 != len)
		{
			return false;
		}

		if (strIsDigit(str, 4) && strIsDigit(str + 5, 2) && strIsDigit(str + 8, 2)
			&& (str[4] == '-' || str[4] == '/') && (str[7] == '-' || str[7] == '/'))
		{
			return true;
		}

		return false;
	}

	unsigned int strToHashValue(const char *str, const int strLen)
	{
		int len = strLen ? strLen : strlen(str);
		unsigned int hashValue = 0;
		int index = 0;
		for (; index <= len - 4; index += 4)
		{
			hashValue += *(unsigned int*)(&str[index]);
		}

		if (index < len)
		{ 
			unsigned int tmp = 1123581325;
			unsigned int remainLen = len - index;
			memcpy(&tmp, str + index, remainLen);
			hashValue += tmp;
		}

		return htonl(hashValue);
	}

    unsigned int strToHashValueCaseInsensitive(const char *str, const int strLen)
    {
        int len = strLen ? strLen : strlen(str);
        std::vector<char> vStr;
        vStr.reserve(len);
        for (int i = 0; i < len; ++i)
        {
            vStr[i] = toupper(str[i]);
        }

        unsigned char md[16] = {'\0'};
	    MD5((const unsigned char*)&vStr[0], len, md);
        return htonl(*((unsigned int*)md));
    }

    // @buf 需要计算的二进制buf
    // @bufLen buf长度
    // @return 返回checksum
    unsigned int genChecksum(const char* buf, const int bufLen)
    {
        static const int magicNum = 197;
        unsigned int checksum = 1123581325;
        for (int i = 0; i < bufLen; ++i)
        {
            checksum += buf[i];
            checksum *= (buf[i] + magicNum);
        }

        return checksum;
    } 

    // @buf 需要加密的二进制buf，同时也存放加密后的结果（即是输入，也是输出，加密后长度不变）
    // @bufLen buf长度
    void simpleEncryptDecrypt(char* buf, const int bufLen)
    {
        static const int magicNum = 197;
        static unsigned char magicArr[magicNum];
        static bool isInit = false;
        if (!isInit)
        {
            int tmp = magicNum;
            for (int i = 0; i < magicNum; ++i)
            {
                magicArr[i] = tmp & 0xFF;
                tmp = tmp * magicArr[i] + i;
            }
            isInit = true;
        }
        
        for (int i = 0; i < bufLen; ++i)
        {
            buf[i] ^= magicArr[i%magicNum];
        }
    }

    bool aesEncrypt(char* buf, int bufLen, const char key[AES_BLOCK_SIZE])
    {
        if(!buf || bufLen <= 0 || !key || bufLen % 16 != 0) return false;

        //加密的初始化向量
        //iv一般设置为全0,可以设置其他，但是加密解密要一样就行
        unsigned char iv[AES_BLOCK_SIZE] = {0};
        AES_KEY aes;
        if(AES_set_encrypt_key((const unsigned char*)key, 128, &aes) < 0)
            return false;

        AES_cbc_encrypt((unsigned char*)buf, (unsigned char*)buf, bufLen, &aes, iv, AES_ENCRYPT);
        return true;
    }
    
    bool aesDecrypt(char* buf, int bufLen, const char key[AES_BLOCK_SIZE])
    {
        if(!buf || bufLen <= 0 || !key || bufLen % 16 != 0) return false;
        //加密的初始化向量
        //iv一般设置为全0,可以设置其他，但是加密解密要一样就行
        unsigned char iv[AES_BLOCK_SIZE] = {0};
        AES_KEY aes;
        if(AES_set_decrypt_key((const unsigned char*)key, 128, &aes) < 0)
            return false;
    
        AES_cbc_encrypt((unsigned char*)buf, (unsigned char*)buf, bufLen, &aes, iv, AES_DECRYPT);
        return true;
    }

    uint64_t getMilliSecsTimestamp()
    {
        struct timeval tv;
     
        gettimeofday(&tv, NULL);
        return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
    }
    
    // 过滤无效的不可见字符
    char* filterInvalidChars(char* charString)
    {
        if (charString == NULL || *charString == '\0') return NULL;

        const char maxInvisibleChar = 32;  // 最大不可见字符值
        while (*charString <= maxInvisibleChar && *charString != '\0')  ++charString;

        if (*charString == '/') return NULL;             // 忽略注释信息
        
        char* pRet = charString;
        while (*charString > maxInvisibleChar)  ++charString;
        *charString = '\0';
        
        return pRet;
    }
    
    // 判断是否为有效的IPv4地址格式
    bool isValidIPv4AddressFormat(const char* ipStr)
    {
        if (ipStr == NULL || *ipStr == '\0') return false;

        unsigned char buf[sizeof(struct in6_addr)] = {0};
        return (inet_pton(AF_INET, ipStr, buf) > 0);
    }
}



