
/* author : liuxu
 * date : 2015.3.9
 * description : 通用函数
 */
 
#include "Function.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <arpa/inet.h>

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
			char tmp[] = "3061";
			unsigned int remainLen = len - index;
			memcpy(tmp, str + index, remainLen);
			hashValue += *(unsigned int*)tmp;
		}

		return htonl(hashValue);
	}
}



