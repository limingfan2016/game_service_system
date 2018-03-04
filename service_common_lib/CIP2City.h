
/* author : liuxu
* date : 2015.08.06
* description : 根据IP映射城市名
*/
 
#ifndef __CIP_2CITY_H__
#define __CIP_2CITY_H__

#include <string>
#include <vector>
#include "base/Type.h"
#include "connect/CSocket.h"
#include "base/xmlConfig.h"
#include "base/MacroDefine.h"

using namespace xercesc;
using namespace std;
using namespace NXmlConfig;


struct SCityCodeInfo
{
	int									citycode;
	std::string							cityname;
};

struct SProvinceCodeInfo
{
	int									code;
	std::string							name;
	std::vector<SCityCodeInfo>			cities;
};


class CIP2City
{
public:
	CIP2City();
	~CIP2City();

	bool init(const char *city_id_map_file_path, const char *ip_city_map_file_path);
	bool getCityInfoByIP(const char *ip, string &city_name, uint32_t &city_id);	

	// 转换
	unsigned long IpString2IpValue(const char *pszIp) const;
	void IpValue2IpString(unsigned long ipValue, char* pszIpAddress, int nMaxCount) const;
	bool IsRightIpString(const char* pszIp) const;

	//utf-8 gbk gb2312
	static int code_convert(const char *from_charset, const char *to_charset, const char *inbuf, size_t inlen, char *outbuf, size_t outlen);

private:
	bool readCityIDMap();

	unsigned long GetString(std::string& str, unsigned long indexStart) const;
	unsigned long GetValue3(unsigned long indexStart) const;
	unsigned long GetValue4(unsigned long indexStart) const;

	unsigned long SearchIp(unsigned long ipValue, unsigned long indexStart = 0, unsigned long indexEnd = 0) const;
	unsigned long SearchIp(const char* pszIp, unsigned long indexStart = 0, unsigned long indexEnd = 0) const;

	// 获取ip国家名、地区名
	void GetAddressByIp(unsigned long ipValue, std::string& strCountry, std::string& strLocation) const;
	void GetAddressByIp(const char* pszIp, std::string& strCountry, std::string& strLocation) const;
	void GetAddressByOffset(unsigned long ulOffset, std::string& strCountry, std::string& strLocation) const;

private:
	string m_city_id_map_file_path;
	string m_ip_city_map_file_path;

	unsigned long m_indexStart;    // 起始索引偏移
	unsigned long m_indexEnd;        // 结束索引偏移

	vector<SProvinceCodeInfo> m_vprovince_info;
	char *m_ip_city_data;
	uint32_t m_data_len;

};

#endif // __CIP_2CITY_H__
