
/* author : liuxu
* date : 2015.08.24
* description : 敏感词过滤
*/

#include <string.h>
#include <stdio.h>

#include "CSensitiveWordFilter.h"
#include <base/Function.h>
#include <base/CLogger.h>
#include <base/MacroDefine.h>


using namespace NCommon;
using namespace std;

CSensitiveWordFilter::CSensitiveWordFilter()
{
	m_p_dfa = NULL;
}

CSensitiveWordFilter::~CSensitiveWordFilter()
{
	releaseDFA();
}

int CSensitiveWordFilter::init(const char *cfg_filepath)
{
	FILE *fp = fopen(cfg_filepath, "r");
	if (NULL == fp){ return -1; }

	char key_words[512] = {0};
	while (fgets(key_words, sizeof(key_words), fp))
	{
		cleanContralString(key_words);
		addKeyWords((unsigned char*)key_words);
	}

	fclose(fp);
	
	return 0;
}

void CSensitiveWordFilter::unInit()
{
	releaseDFA();
}

void CSensitiveWordFilter::addKeyWords(const unsigned char *str)
{
	if (NULL == m_p_dfa)
	{
		m_p_dfa = new DFAInfo();
		bzero(m_p_dfa, sizeof(DFAInfo));
	}
	
	DFAInfo *pcur_arr = m_p_dfa;
	const int len = strlen((const char*)str);
	for (int i = 0; i < len; i++)
	{
		//第一次，就先申请空间
		if (NULL == pcur_arr->parrDFA)
		{
			pcur_arr->parrDFA = new DFAInfo[ONE_BYTE_LEN];
			bzero(pcur_arr->parrDFA, sizeof(DFAInfo)* ONE_BYTE_LEN);
		}
		
		//如果是最后一个字符，则标记结束位
		if (i + 1 == len)
		{
			pcur_arr->parrDFA[str[i]].is_end = true;
		}
		else//跳转到下一个字符
		{
			pcur_arr = &pcur_arr->parrDFA[str[i]];
		}
	}
}

bool CSensitiveWordFilter::filterContent(const unsigned char *input_str, const unsigned int input_len, unsigned char *output_str, unsigned int &output_len)
{
	if (m_p_dfa == NULL || m_p_dfa->parrDFA == NULL || output_len <= 0) return false;
	
	int output_index = 0;
	for (int cur_index = 0; cur_index < (int)input_len;)
	{
		DFAInfo *p_cur_dfa = m_p_dfa;
		int find_index = cur_index;
		int end_index = -1;
		
		//以cur_index为起点，开始查找关键词
		while (NULL != p_cur_dfa->parrDFA)
		{
			if (p_cur_dfa->parrDFA[input_str[find_index]].is_end)	//匹配到一个
			{
				end_index = find_index;
				break;
			}
			else
			{
				p_cur_dfa = &p_cur_dfa->parrDFA[input_str[find_index]];
			}

			find_index++;
		}
		
		if (-1 == end_index)	//没匹配到关键词
		{
			//output_str 空间不够
			if ((int)output_len <= output_index + 1)
			{
				output_str[output_index] = '\0';
				return false;
			}

			output_str[output_index++] = input_str[cur_index++];
		}
		else    //匹配到关键了
		{
			// 修改为全字屏蔽
			/*
			//拷贝第一个 字
			int copy_len = 0;
			if (input_str[cur_index] & 0x80)
			{
				//计算高位连续1的个数(UTF8编码格式)
				for (int i = 0; i < 6; i++)
				{
					unsigned char temp = 0x80;
					if (input_str[cur_index] & (temp >> i))
					{
						copy_len++;
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				copy_len = 1;
			}
		    */

			//output_str 空间不够
			int copy_len = 1;
			if ((int)output_len <= output_index + 1 + copy_len + 2)
			{
				output_str[output_index] = '\0';
				return false;
			}

			// 修改为全字屏蔽
			// memcpy(output_str + output_index, input_str + cur_index, copy_len);
			// output_index += copy_len;
			output_str[output_index++] = '*';
			
			output_str[output_index++] = '*';
			output_str[output_index++] = '*';
			cur_index = end_index + 1;
		}		
	}
	output_str[output_index] = '\0';
	
	return true;
}

bool CSensitiveWordFilter::isExistSensitiveStr(const unsigned char *input_str, unsigned int input_len)
{
	if (m_p_dfa == NULL || m_p_dfa->parrDFA == NULL) return false;
	
	for (int cur_index = 0; cur_index < (int)input_len; ++cur_index)
	{
		DFAInfo *p_cur_dfa = m_p_dfa;
		int find_index = cur_index;
		int end_index = -1;
		
		//以cur_index为起点，开始查找关键词
		while (NULL != p_cur_dfa->parrDFA)
		{
			if (p_cur_dfa->parrDFA[input_str[find_index]].is_end)	//匹配到一个
			{
				end_index = find_index;
				break;
			}
			else
			{
				p_cur_dfa = &p_cur_dfa->parrDFA[input_str[find_index]];
			}

			find_index++;
		}

		if (-1 != end_index) return true;  //匹配到关键了
	}

	return false;
}

void CSensitiveWordFilter::releaseDFA()
{
	releaseDFA(m_p_dfa);
	delete m_p_dfa;
	m_p_dfa = NULL;
}

void CSensitiveWordFilter::releaseDFA(DFAInfo *p_dfa)
{
	if (NULL != p_dfa && NULL != p_dfa->parrDFA)
	{
		for (int i = 0; i < ONE_BYTE_LEN; i++)
		{
			releaseDFA(&p_dfa->parrDFA[i]);
		}
		delete[](p_dfa->parrDFA);
	}
}

void CSensitiveWordFilter::cleanContralString(char *str)
{
	const int len = strlen(str);
	for (int i = len - 1; i >= 0; i--)
	{
		if ((unsigned char)str[i] <= 32)
		{
			str[i] = '\0';
		}
		else
		{
			break;
		}
	}
}

