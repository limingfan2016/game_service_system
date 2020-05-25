
/* author : admin
 * date : 2015.05.08
 * description : xml解析生成代码API
 */

#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLString.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "xmlConfig.h"


namespace NXmlConfig
{

static const char* MainType = "main";
static const char* MainValue = "true";
static const char* StructType = "struct";
static const char* TypeTag = "type";


ReadXml::ReadXml() : m_DOMXmlParser(NULL)
{
	try
	{
		XMLPlatformUtils::Initialize();
		m_DOMXmlParser = new XercesDOMParser();
	}
	catch (XMLException& excp)
	{
		string msg;
		CXmlConfig::charArrayToString(excp.getMessage(), msg);
		printf("XML initialization error : %s\n", msg.c_str());
	}
}

ReadXml::~ReadXml()
{
	delete m_DOMXmlParser;
	m_DOMXmlParser = NULL;
		
	try
	{
		XMLPlatformUtils::Terminate();
	}
	catch(XMLException& excp)
	{
		string msg;
		CXmlConfig::charArrayToString(excp.getMessage(), msg);
		printf("XML terminate error : %s\n", msg.c_str());
	} 
}

void ReadXml::setConfig(const char* xmlConfigFile, IXmlConfigBase& configVal)
{
	configVal.__isSetConfigValueSuccess__ = false;
	if (m_DOMXmlParser == NULL) return;
	
	if (xmlConfigFile == NULL || *xmlConfigFile == '\0')
	{
		printf("XML file name is null\n");
		return;
	}
	
	struct stat fInfo;
	if (stat(xmlConfigFile, &fInfo) != 0)
	{
		printf("check XML file error, name = %s, error code = %d\n", xmlConfigFile, errno);
		return;
	}
	
	try
	{
		m_DOMXmlParser->parse(xmlConfigFile) ;
		DOMDocument* xmlDoc = m_DOMXmlParser->getDocument();
		if (xmlDoc == NULL)
		{
			printf("parse XML document error, file name = %s\n", xmlConfigFile);
			return;
		}
		
		DOMElement* root = xmlDoc->getDocumentElement();  // 取得根结点
		if (root == NULL)
		{
			printf("empty XML document error, file name = %s\n", xmlConfigFile);
			return;
		}
		
		DomNodeArray parentNode;
		CXmlConfig::getNode((DOMNode*)root, MainType, MainValue, parentNode);
		if (parentNode.size() < 1)
		{
			printf("can not find main config value error, file name = %s\n", xmlConfigFile);
			return;
		}
		
		configVal.set(parentNode[0]);
		configVal.__isSetConfigValueSuccess__ = true;
	}
	catch(XMLException& excp)
	{
		string msg;
		CXmlConfig::charArrayToString(excp.getMessage(), msg);
		printf("parsing xml file error, name = %s, msg = %s\n", xmlConfigFile, msg.c_str());
	}
}

DOMElement* ReadXml::getDocumentRoot(const char* xmlConfigFile)
{
	DOMElement* root = NULL;
	if (m_DOMXmlParser != NULL)
	{
		try
		{
			m_DOMXmlParser->parse(xmlConfigFile) ;
			DOMDocument* xmlDoc = m_DOMXmlParser->getDocument();
			root = xmlDoc->getDocumentElement();  // 取得根结点
			if (root == NULL)
			{
				printf("empty XML document error\n");
			}
		}
		catch(XMLException& excp)
		{
			string msg;
			CXmlConfig::charArrayToString(excp.getMessage(), msg);
			printf("parsing file error : %s\n", msg.c_str());
		}
	}
	
	return root;
}



void CXmlConfig::getChildNode(DOMNode* parentNode, DomNodeArray& domNodeArray, const string& parentName, const string& nodeName, const string& typeValue)
{
	DOMNodeList* nodeList = parentNode->getChildNodes();
    const unsigned int length = nodeList->getLength();
	for (unsigned int i = 0; i < length; ++i)
	{
		getNode(nodeList->item(i), domNodeArray, parentName, nodeName, typeValue);
	}
}

void CXmlConfig::getChildNode(DOMNode* parentNode, const string& typeName, const string& typeValue, DomNodeArray& domNodeArray)
{
	DOMNodeList* nodeList = parentNode->getChildNodes();
	const unsigned int length = nodeList->getLength();
	for (unsigned int i = 0; i < length; ++i)
	{
		getNode(nodeList->item(i), typeName, typeValue, domNodeArray);
	}
}

void CXmlConfig::charArrayToString(const XMLCh* xmlChar, string& value)
{
	if (xmlChar != NULL)
	{
		char* charArray = XMLString::transcode(xmlChar);
		value = charArray;
		XMLString::release(&charArray);
	}
}

void CXmlConfig::getAttribute(DOMNode* node, const char* name, string& value)
{
	if (node != NULL && node->getNodeType() == DOMNode::ELEMENT_NODE)
	{
		XMLCh* typeValue = XMLString::transcode(name);
		charArrayToString(((DOMElement*)node)->getAttribute(typeValue), value);
		XMLString::release(&typeValue);
	}
}

void CXmlConfig::getNode(DOMNode* parentNode, DomNodeArray& domNodeArray, const string& parentName, const string& nodeName, const string& typeValue)
{
	if (parentNode != NULL && parentNode->getNodeType() == DOMNode::ELEMENT_NODE)
	{
		string name;
		DOMNode* parent = parentNode->getParentNode();
		if (parent != NULL) charArrayToString(parent->getNodeName(), name);
		if (name != parentName) return getChildNode(parentNode, domNodeArray, parentName, nodeName, typeValue);
		
		charArrayToString(parentNode->getNodeName(), name);
		if (name != nodeName) return getChildNode(parentNode, domNodeArray, parentName, nodeName, typeValue);

		string type;
		getAttribute(parentNode, TypeTag, type);
		if (type != typeValue) return getChildNode(parentNode, domNodeArray, parentName, nodeName, typeValue);
		
		domNodeArray.push_back(parentNode);
	}
}

void CXmlConfig::getNode(DOMNode* parentNode, const string& typeName, const string& typeValue, DomNodeArray& domNodeArray)
{
	if (parentNode != NULL && parentNode->getNodeType() == DOMNode::ELEMENT_NODE)
	{
		string type;
		getAttribute(parentNode, TypeTag, type);
		if (type != StructType) return getChildNode(parentNode, typeName, typeValue, domNodeArray);
		
		getAttribute(parentNode, typeName.c_str(), type);
		if (type != typeValue) return getChildNode(parentNode, typeName, typeValue, domNodeArray);
		
		domNodeArray.push_back(parentNode);
	}
}

string CXmlConfig::getValue(DOMNode* parentNode, const string& itemName)
{
	string strValue;
	string strName;
	
	DOMNode* firstChildNode = NULL;
	charArrayToString(parentNode->getNodeName(), strName);
	if (strName == itemName)
	{
		firstChildNode = parentNode->getFirstChild();
		if (firstChildNode != NULL) charArrayToString(firstChildNode->getNodeValue(), strValue);
		return strValue;
	}
	
	DOMNodeList* nodeList = parentNode->getChildNodes();
	const unsigned int length = nodeList->getLength();
	for (unsigned int i = 0; i < length; ++i)
	{
		DOMNode* nodeTemp = nodeList->item(i);
		if (nodeTemp->getNodeType() == DOMNode::ELEMENT_NODE)
		{
			charArrayToString(nodeTemp->getNodeName(), strName);
			if (strName == itemName)
			{
				firstChildNode = nodeTemp->getFirstChild();
				if (firstChildNode != NULL) charArrayToString(firstChildNode->getNodeValue(), strValue);
				break;
			}
		}
	}
	
	return strValue;
}

string CXmlConfig::getKey(DOMNode* parentNode, const char* keyName)
{
	string keyValue;
	getAttribute(parentNode, keyName, keyValue);	
	return keyValue;
}


int CXmlConfig::stringToInt(const string str)
{
    return atoi(str.c_str());
}

long CXmlConfig::stringToLong(const string str)
{
	return atol(str.c_str());
}

double CXmlConfig::stringToDouble(const string str)
{
	return atof(str.c_str());
}


void CXmlConfig::setConfigValue(const char* xmlConfigFile, IXmlConfigBase& configVal)
{
	if (xmlConfigFile != NULL && *xmlConfigFile != '\0')
	{
		ReadXml rdXml;
		rdXml.setConfig(xmlConfigFile, configVal);
	}
}

}
	
	