
/* author : liuxu
 * date : 2015.08.24
 * description : 敏感词过滤
 */
 
#ifndef __CSENSITIVE_WORD_FILTER_H__
#define __CSENSITIVE_WORD_FILTER_H__


static const int ONE_BYTE_LEN = 256;

#pragma pack(1)
typedef struct DFAInfo
{
	bool is_end;
	DFAInfo *parrDFA;	//数组定长为256
}DFAInfo;
#pragma pack()

class CSensitiveWordFilter
{
public:
	CSensitiveWordFilter();
	~CSensitiveWordFilter();
	
public:
	int init(const char *cfg_filepath);
	void unInit();

public:
	//返回false:说明output_str buf太短
	//input_len 指定input_str 字串长度
	//output_len 指定output_str buf长度，并返回output_str实际字串长度
	bool filterContent(const unsigned char *input_str, const unsigned int input_len, unsigned char *output_str, unsigned int &output_len);	

	//判断字符是否有敏感字符
	bool isExistSensitiveStr(const unsigned char *input_str, unsigned int input_len);

private:
	void cleanContralString(char *str);
	void addKeyWords(const unsigned char *str);
	void releaseDFA();
	void releaseDFA(DFAInfo *pdfa);

private:
	DFAInfo *m_p_dfa;

};

#endif // __CSENSITIVE_WORD_FILTER_H__

