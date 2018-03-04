
/* author : liuxu
* date : 2015.08.06
* description : 根据IP映射城市名
*/
 
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLString.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>

#include "CIP2City.h"

using namespace NCommon;

const int INDEX_LENGTH = 7;        // 一个索引包含4字节的起始IP和3字节的IP记录偏移，共7字节
const int IP_LENGTH = 4;
const int OFFSET_LENGTH = 3;

enum {
	REDIRECT_MODE_1 = 0x01,    // 重定向模式1 偏移量后无地区名
	REDIRECT_MODE_2 = 0x02,    // 重定向模式2 偏移量后有地区名
};

CIP2City::CIP2City()
{
	m_ip_city_data = NULL;
	m_data_len = 0;
}

CIP2City::~CIP2City()
{
	if (m_ip_city_data)
	{
		DELETE_ARRAY(m_ip_city_data);
		m_data_len = 0;
	}
}

bool CIP2City::init(const char *city_id_map_file_path, const char *ip_city_map_file_path)
{
	m_city_id_map_file_path = city_id_map_file_path;
	m_ip_city_map_file_path = ip_city_map_file_path;
	m_vprovince_info.clear();
	
	if (!readCityIDMap())
	{
		return false;
	}

	//一次性将数据读取到内存
	FILE* fp = fopen(m_ip_city_map_file_path.c_str(), "r");
	if (!fp)
	{
		return false;
	}
	fseek(fp, 0L, SEEK_END);
	m_data_len = ftell(fp);
	NEW_ARRAY(m_ip_city_data, char[m_data_len]);
	fseek(fp, 0L, SEEK_SET);
	fread(m_ip_city_data, 1, m_data_len, fp);
	fclose(fp);

	// IP头由两个十六进制4字节偏移量构成，分别为索引开始，和索引结束
	m_indexStart = this->GetValue4(0);
	m_indexEnd = this->GetValue4(4);

	return true;
}

bool CIP2City::getCityInfoByIP(const char *ip, string &city_name, uint32_t &city_id)
{
	std::string strCountry = "", strLocation = "";
	this->GetAddressByIp(ip, strCountry, strLocation);

	int provinceCode = 0, cityCode = 0;
	std::string cityName = "";
	for (size_t i = 0; i < m_vprovince_info.size(); i++)
	{
		if (strCountry.find(m_vprovince_info[i].name.c_str()) != std::string::npos
			|| strLocation.find(m_vprovince_info[i].name.c_str()) != std::string::npos)
		{
			provinceCode = m_vprovince_info[i].code;
			cityName = m_vprovince_info[i].name;
			for (size_t j = 0; j < m_vprovince_info[i].cities.size(); j++)
			{
				if (strCountry.find(m_vprovince_info[i].cities[j].cityname.c_str()) != std::string::npos
					|| strLocation.find(m_vprovince_info[i].cities[j].cityname.c_str()) != std::string::npos)
				{
					cityCode = m_vprovince_info[i].cities[j].citycode;
					cityName = m_vprovince_info[i].cities[j].cityname;
					break;
				}
			}
			break;
		}
	}

	if (provinceCode == 0 && cityCode == 0)
	{
		city_id = m_vprovince_info[0].code;
		city_name = m_vprovince_info[0].name;
	}
	else if (provinceCode != 0 && cityCode == 0)
	{
		city_id = provinceCode;
		city_name = cityName;
	}
	else
	{
		city_id = provinceCode * 10000 + cityCode;
		city_name = cityName;
	}

	return true;
}

bool CIP2City::readCityIDMap()
{
	ReadXml rxml;
	DOMNode* proot = rxml.getDocumentRoot(m_city_id_map_file_path.c_str());
	DOMNodeList* pProvinceList = proot->getChildNodes();

	char conv_str[2048] = { 0 };
	for (size_t i = 0; i < pProvinceList->getLength(); i++)
	{
		DOMNode* pProvince = pProvinceList->item(i);
		if (pProvince->getNodeType() != DOMNode::ELEMENT_NODE)
		{
			continue;
		}

		SProvinceCodeInfo s_province;

		XMLCh* typeValue = XMLString::transcode("code");
		char* char_array = XMLString::transcode(((DOMElement*)pProvince)->getAttribute(typeValue));
		XMLString::release(&typeValue);
		s_province.code = atoi(char_array);
		XMLString::release(&char_array);

		typeValue = XMLString::transcode("name");
		char_array = XMLString::transcode(((DOMElement*)pProvince)->getAttribute(typeValue));
		XMLString::release(&typeValue);
		code_convert("utf-8", "gb2312", char_array, strlen(char_array), conv_str, sizeof(conv_str));
		s_province.name = conv_str;
		XMLString::release(&char_array);

		if (pProvince->hasChildNodes())
		{
			DOMNodeList* p_city_list = pProvince->getChildNodes();
			for (size_t j = 0; j < p_city_list->getLength(); j++)
			{
				DOMNode *p_city = p_city_list->item(j);
				if (p_city->getNodeType() != DOMNode::ELEMENT_NODE)
				{
					continue;
				}
				SCityCodeInfo s_city;

				typeValue = XMLString::transcode("citycode");
				char* char_array = XMLString::transcode(((DOMElement*)p_city)->getAttribute(typeValue));
				XMLString::release(&typeValue);
				s_city.citycode = atoi(char_array);
				XMLString::release(&char_array);
				typeValue = XMLString::transcode("cityname");
				char_array = XMLString::transcode(((DOMElement*)p_city)->getAttribute(typeValue));
				XMLString::release(&typeValue);
				code_convert("utf-8", "gb2312", char_array, strlen(char_array), conv_str, sizeof(conv_str));
				s_city.cityname = conv_str;
				XMLString::release(&char_array);

				s_province.cities.push_back(s_city);
			}
		}

		m_vprovince_info.push_back(s_province);
	}

	return true;
}


// ============================================================================
//    根据IP地址字符串返回其十六进制值（4字节）
// ============================================================================
unsigned long CIP2City::IpString2IpValue(const char *pszIp) const
{
	if (!this->IsRightIpString(pszIp)) {
		return 0;
	}
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int nNum = 0;            // 每个段数值
	const char *pBeg = pszIp;
	const char *pPos = NULL;
	unsigned long ulIp = 0; // 整个IP数值
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	pPos = strchr(pszIp, '.');
	while (pPos != NULL) {
		nNum = atoi(pBeg);
		ulIp += nNum;
		ulIp *= 0x100;
		pBeg = pPos + 1;
		pPos = strchr(pBeg, '.');
	}

	nNum = atoi(pBeg);
	ulIp += nNum;
	return ulIp;
}

// ============================================================================
//    根据ip值获取字符串（由点分割）
// ============================================================================
void CIP2City::IpValue2IpString(unsigned long ipValue,
	char *pszIpAddress,
	int nMaxCount) const
{
	if (!pszIpAddress) {
		return;
	}

	snprintf(pszIpAddress, nMaxCount, "%lu.%lu.%lu.%lu", (ipValue & 0xFF000000) >> 24,
		(ipValue & 0x00FF0000) >> 16, (ipValue & 0x0000FF00) >> 8, ipValue & 0x000000FF);
	pszIpAddress[nMaxCount - 1] = 0;
}


// ============================================================================
//    判断给定IP字符串是否是合法的ip地址
// ==============================================================================
bool CIP2City::IsRightIpString(const char* pszIp) const
{
	if (!pszIp) {
		return false;
	}

	int nLen = strlen(pszIp);
	if (nLen < 7) {

		// 至少包含7个字符"0.0.0.0"
		return false;
	}

	for (int i = 0; i < nLen; ++i) {
		if (!isdigit(pszIp[i]) && pszIp[i] != '.') {
			return false;
		}

		if (pszIp[i] == '.') {
			if (0 == i) {
				if (!isdigit(pszIp[i + 1])) {
					return false;
				}
			}
			else if (nLen - 1 == i) {
				if (!isdigit(pszIp[i - 1])) {
					return false;
				}
			}
			else {
				if (!isdigit(pszIp[i - 1]) || !isdigit(pszIp[i + 1])) {
					return false;
				}
			}
		}
	}
	return true;
}


// ==========================================================================================================
//    从指定位置获取一个十六进制的数 (读取3个字节， 主要用于获取偏移量， 与效率紧密相关的函数，尽可能优化）
// ==========================================================================================================
unsigned long CIP2City::GetValue3(unsigned long indexStart) const
{
	unsigned long ulValue = 0;
	for (int i = 0; i < 3; i++)
	{
		ulValue = ulValue * 0x100 + (uint8_t)m_ip_city_data[indexStart + 2 - i];
	}

	return ulValue;
}

// ==========================================================================================================
//    从指定位置获取一个十六进制的数 (读取4个字节， 主要用于获取IP值， 与效率紧密相关的函数，尽可能优化）
// ==========================================================================================================
unsigned long CIP2City::GetValue4(unsigned long indexStart) const
{
	unsigned long ulValue = 0;
	for (int i = 0; i < 4; i++)
	{
		ulValue = ulValue * 0x100 + (uint8_t)m_ip_city_data[indexStart + 3 - i];
	}

	return ulValue;
}

// ============================================================================
//    从指定位置获取字符串
// ============================================================================
unsigned long CIP2City::GetString(std::string &str, unsigned long indexStart) const
{
	str = &m_ip_city_data[indexStart];

	return str.length();
}

// ============================================================================
//    根据指定IP(十六进制值)，返回其在索引段中的位置(索引)
//    ulIndexStart和ulIndexEnd可以指定搜索范围 均为0表示搜索全部
// ============================================================================
unsigned long CIP2City::SearchIp(unsigned long ipValue,
	unsigned long indexStart,
	unsigned long indexEnd) const
{
	if (0 == indexStart) {
		indexStart = m_indexStart;
	}

	if (0 == indexEnd) {
		indexEnd = m_indexEnd;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	unsigned long indexLeft = indexStart;
	unsigned long indexRight = indexEnd;

	// 先除后乘是为了保证mid指向一个完整正确的索引
	unsigned long indexMid = (indexRight - indexLeft) / INDEX_LENGTH / 2 * INDEX_LENGTH + indexLeft;

	// 起始Ip地址(如172.23.0.0),他和Ip记录中的Ip地址(如172.23.255.255)构成一个Ip范围，在这个范围内的Ip都可以由这条索引来获取国家、地区
	unsigned long ulIpHeader = 0;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	do
	{
		ulIpHeader = this->GetValue4(indexMid);
		if (ipValue < ulIpHeader) {
			indexRight = indexMid;
			indexMid = (indexRight - indexLeft) / INDEX_LENGTH / 2 * INDEX_LENGTH + indexLeft;
		}
		else {
			indexLeft = indexMid;
			indexMid = (indexRight - indexLeft) / INDEX_LENGTH / 2 * INDEX_LENGTH + indexLeft;
		}
	} while (indexLeft < indexMid);            // 注意是根据mid来进行判断

	// 只要符合范围就可以，不需要完全相等
	return indexMid;
}

// ============================================================================
// ==============================================================================
unsigned long CIP2City::SearchIp(const char *pszIp,
	unsigned long indexStart,
	unsigned long indexEnd) const
{
	if (!this->IsRightIpString(pszIp)) {
		return 0;
	}
	return this->SearchIp(this->IpString2IpValue(pszIp), indexStart,
		indexEnd);
}


// ============================================================================
//    通过指定的偏移量来获取ip记录中的国家名和地区名，偏移量可由索引获取
//    ulOffset为Ip记录偏移量
// ============================================================================
void CIP2City::GetAddressByOffset(unsigned long ulOffset,
	std::string &strCountry,
	std::string &strLocation) const
{
	// 略去4字节Ip地址
	ulOffset += IP_LENGTH;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 读取首地址的值
	int nVal = m_ip_city_data[ulOffset];
	unsigned long ulCountryOffset = 0;    // 真实国家名偏移
	unsigned long ulLocationOffset = 0; // 真实地区名偏移
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// 为了节省空间，相同字符串使用重定向，而不是多份拷贝
	if (REDIRECT_MODE_1 == nVal) {

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// 重定向1类型
		unsigned long ulRedirect = this->GetValue3(ulOffset + 1); // 重定向偏移

		if (m_ip_city_data[ulRedirect] == REDIRECT_MODE_2) {

			// 混合类型1，重定向1类型进入后遇到重定向2类型 
			// 0x01 1字节
			// 偏移量 3字节 -----> 0x02 1字节 
			//                     偏移量 3字节 -----> 国家名
			//                     地区名
			ulCountryOffset = this->GetValue3(ulRedirect + 1);
			this->GetString(strCountry, ulCountryOffset);
			ulLocationOffset = ulRedirect + 4;
		}
		else {

			// 单纯的重定向模式1
			// 0x01 1字节
			// 偏移量 3字节 ------> 国家名
			//                      地区名
			ulCountryOffset = ulRedirect;
			ulLocationOffset = ulRedirect + this->GetString(strCountry,
				ulCountryOffset);
		}
	}
	else if (REDIRECT_MODE_2 == nVal) {

		// 重定向2类型
		// 0x02 1字节
		// 国家偏移 3字节 -----> 国家名
		// 地区名
		ulCountryOffset = this->GetValue3(ulOffset + 1);
		this->GetString(strCountry, ulCountryOffset);

		ulLocationOffset = ulOffset + 4;
	}
	else {

		// 最简单的情况 没有重定向
		// 国家名
		// 地区名
		ulCountryOffset = ulOffset;
		ulLocationOffset = ulCountryOffset + this->GetString(strCountry,
			ulCountryOffset);
	}

	// 读取地区
	if (m_ip_city_data[ulLocationOffset] == REDIRECT_MODE_2
		|| m_ip_city_data[ulLocationOffset+1] == REDIRECT_MODE_1) {

		// 混合类型2(最复杂的情形，地区也重定向)
		// 0x01 1字节
		// 偏移量 3字节 ------> 0x02 1字节
		//                      偏移量 3字节 -----> 国家名
		//                      0x01 or 0x02 1字节
		//                      偏移量 3字节 ----> 地区名 偏移量为0表示未知地区
		ulLocationOffset = this->GetValue3(ulLocationOffset + 1);
	}

	this->GetString(strLocation, ulLocationOffset);
}

// ============================================================================
//    根据十六进制ip获取国家名地区名
// ============================================================================
void CIP2City::GetAddressByIp(unsigned long ipValue,
	std::string &strCountry,
	std::string &strLocation) const
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	unsigned long ulIndexOffset = this->SearchIp(ipValue);
	unsigned long ulRecordOffset = this->GetValue3(ulIndexOffset + IP_LENGTH);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	this->GetAddressByOffset(ulRecordOffset, strCountry, strLocation);
}

// ============================================================================
//    根据ip字符串获取国家名地区名
// ============================================================================
void CIP2City::GetAddressByIp(const char *pszIp,
	std::string &strCountry,
	std::string &strLocation) const
{
	if (!this->IsRightIpString(pszIp)) {
		return;
	}
	this->GetAddressByIp(this->IpString2IpValue(pszIp), strCountry, strLocation);
}

int CIP2City::code_convert(const char *from_charset, const char *to_charset, const char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
	iconv_t cd;
	const char *temp = inbuf;
	const char **pin = &temp;
	char **pout = &outbuf;
	memset(outbuf, 0, outlen);
	cd = iconv_open(to_charset, from_charset);
	if (cd == 0) return -1;

	if (iconv(cd, (char **)pin, &inlen, pout, &outlen) == (size_t)-1)
		return -1;

	iconv_close(cd);
	return 0;
}

