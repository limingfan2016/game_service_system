
/* author : limingfan
 * date : 2015.05.08
 * description : xml解析生成代码API
 */

#ifndef _XML_CONFIG_H_
#define _XML_CONFIG_H_


#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <string>
#include <vector>


using namespace std;
using namespace xercesc;

namespace NXmlConfig
{

typedef vector<DOMNode*> DomNodeArray;

class IXmlConfigBase;

class ReadXml
{
public:
	ReadXml();
	~ReadXml();
	
public:
	void setConfig(const char* xmlConfigFile, IXmlConfigBase& configVal);
	
public:
    DOMElement* getDocumentRoot(const char* xmlConfigFile);


private:
    XercesDOMParser* m_DOMXmlParser;
};


class IXmlConfigBase
{
public:
    bool isSetConfigValueSuccess() const { return __isSetConfigValueSuccess__; };

public:
    IXmlConfigBase() : __isSetConfigValueSuccess__(false) {};
    virtual ~IXmlConfigBase() {};
    virtual void set(DOMNode* parent) = 0;
	
private:
	bool __isSetConfigValueSuccess__;
	
	friend class ReadXml;
};


class CXmlConfig
{
public:
	static void setConfigValue(const char* xmlConfigFile, IXmlConfigBase& configVal);

public:
	static void getNode(DOMNode* parentNode, DomNodeArray& domNodeArray, const string& parentName, const string& nodeName, const string& typeValue = "struct");
	static void getNode(DOMNode* parentNode, const string& typeName, const string& typeValue, DomNodeArray& domNodeArray);
	static string getValue(DOMNode* parentNode, const string& itemName);
	static string getKey(DOMNode* parentNode, const char* keyName);
	
public:
    static void getChildNode(DOMNode* parentNode, DomNodeArray& domNodeArray, const string& parentName, const string& nodeName, const string& typeValue);
	static void getChildNode(DOMNode* parentNode, const string& typeName, const string& typeValue, DomNodeArray& domNodeArray);
    static void charArrayToString(const XMLCh* xmlChar, string& value);
	static void getAttribute(DOMNode* node, const char* name, string& value);
	
public:
	static int stringToInt(const string str);
	static long stringToLong(const string str);
	static double stringToDouble(const string str);
	
private:
	CXmlConfig();
};

}


#endif  // _XML_CONFIG_H_

