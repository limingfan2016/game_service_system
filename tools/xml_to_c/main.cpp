
/* author : limingfan
 * date : 2015.05.08
 * description : xml解析生成服务配置代码工具
 */

#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLString.hpp>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdio.h>
#include <ctype.h>


using namespace std;
using namespace xercesc;


static string MainType = "config";
static const char* MainClass = "main";
static const char* StructType = "struct";
static const char* ArrayType = "array";
static const char* VectorType = "vector";
static const char* MapType = "map";
static const char* UnorderMapType = "unordered_map";
static const char* TypeTag = "type";
static const char* NameTag = "name";
static const char* IncludeTag = "include";


// 变量类型
enum VariableType
{
	BaseVar = 0,
	ArrayVar = 1,
	VectorVar = 2,
	MapVar = 3,
	StructVar = 4,
};

// C++ 定义类型
struct CppType
{
	CppType(const string& t, const string& n, const string& v, const string& k, VariableType vType) : type(t), name(n), value(v), key(k), varType(vType) {};
	
	string type;
	string name;
	string value;
	string key;
	VariableType varType;
};

// 数据类struct的属性列表
struct XmlStructType
{
	vector<CppType> attributeList;
};

// 数据类struct名称到定义的映射
typedef unordered_map<string, XmlStructType> StructName2Define;


class CXmlToCPP
{
public:
	CXmlToCPP() : m_DOMXmlParser(NULL)
	{
		try
		{
			XMLPlatformUtils::Initialize();
			m_DOMXmlParser = new XercesDOMParser();
		}
		catch (XMLException& excp)
		{
			string msg;
			charArrayToString(excp.getMessage(), msg);
			printf("XML initialization error : %s\n", msg.c_str());
		}
	}
	
	~CXmlToCPP()
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
			charArrayToString(excp.getMessage(), msg);
			printf("XML terminate error : %s\n", msg.c_str());
		} 
	}

public:
	bool xmlParser(const char* xmlFile)
	{
		if (m_DOMXmlParser == NULL) return false;

		try
		{
			// 解析XML文件内容
			m_DOMXmlParser->parse(xmlFile) ;
			DOMDocument* xmlDoc = m_DOMXmlParser->getDocument();
			DOMElement* pRoot = xmlDoc->getDocumentElement();  // 取得根结点
			if (pRoot == NULL)
			{
				printf("empty XML document\n");
				return false;
			}

			// 根据XML文件内容的定义生成 C++ 数据类型
			makeCppType((DOMNode*)pRoot, string());
		}
		catch(XMLException& excp)
		{
			string msg;
			charArrayToString(excp.getMessage(), msg);
			printf("error parsing file : %s\n", msg.c_str());
			return false;
		}
		
		return !m_name.empty() && !m_include.empty();
	}
	
	// 生成数据类定义文件
	string outputToFile(const char* filePath)
	{
		if (m_name.empty())
		{
			printf("the config file not set name value\n");
			return "";
		}
		
		// 文件不存在则创建
		string fileName = string(filePath) + "/_" + m_name + "_.h";
		ofstream fileInstance(fileName.c_str());
		if(!fileInstance)
		{
			printf("open file error, name = %s\n", fileName.c_str());
			return "";
		}
		
		// 文件头部
		string upperName = "__";
		for (unsigned int i = 0; i < m_name.length(); ++i) upperName += toupper(m_name[i]);
		upperName += "_H__";
		string fileHeader = "#ifndef " + upperName + "\n#define " + upperName;
		fileHeader += "\n\n#include <string>";
		fileHeader += "\n#include <vector>";
		fileHeader += "\n#include <map>";
		fileHeader += "\n#include <unordered_map>";
		fileHeader += "\n#include <iostream>";
		fileHeader += "\n#include <time.h>";
		fileHeader += "\n#include \"" + m_include + ".h\"";
		fileHeader += "\n\n\nusing namespace std;";
		fileHeader += "\nusing namespace NXmlConfig;\n\n";
        fileInstance << fileHeader << endl;
		
		// 命名空间
		fileInstance << "namespace " << m_name << "\n{\n" << endl;
	
		// 文件struct定义体
		string attribute;
		vector<string> setAttributes;       // set函数体代码
		vector<string> outputAttributes;    // ouput函数体代码

		for (vector<string>::reverse_iterator rStructName = m_structNames.rbegin(); rStructName != m_structNames.rend(); rStructName++)
		{
			// struct 定义
			StructName2Define::iterator structIt = m_structName2Define.find(*rStructName);
			if (structIt->first != MainType) fileInstance << "struct " << structIt->first << "\n{" << endl;
			else fileInstance << "struct " << structIt->first << " : public IXmlConfigBase\n{" << endl;

			for (unsigned int idx = 0; idx < structIt->second.attributeList.size(); ++idx)
			{
				const CppType& attributeInfo = structIt->second.attributeList[idx];
				if (attributeInfo.varType == BaseVar)  // 基本类型
				{
					fileInstance << "    " << attributeInfo.type << " " << attributeInfo.name << ";" << endl;
					
					if (attributeInfo.type == "string") attribute = attributeInfo.name + " = CXmlConfig::getValue(parent, \"" + attributeInfo.name + "\");";
				    else if (attributeInfo.type == "int" || attributeInfo.type == "unsigned int") attribute = attributeInfo.name + " = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, \"" + attributeInfo.name + "\"));";
					else if (attributeInfo.type == "long" || attributeInfo.type == "unsigned long") attribute = attributeInfo.name + " = CXmlConfig::stringToLong(CXmlConfig::getValue(parent, \"" + attributeInfo.name + "\"));";
					else if (attributeInfo.type == "double") attribute = attributeInfo.name + " = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, \"" + attributeInfo.name + "\"));";
					setAttributes.push_back(attribute);
					
					attribute = "std::cout << \"" + structIt->first + " : " + attributeInfo.name + " = \" << " + attributeInfo.name + " << endl;";
					outputAttributes.push_back(attribute);
				}
				
				else if (attributeInfo.varType == StructVar)
				{
					fileInstance << "    " << attributeInfo.name << " " << attributeInfo.value << ";" << endl;
					
					attribute = (idx == 0) ? "" : "\n        ";
					string arrayName = "_" + attributeInfo.value;
					attribute =  attribute + "DomNodeArray " + arrayName + ";";
					setAttributes.push_back(attribute);
					
					attribute = string("CXmlConfig::getNode(parent, ") + "\"param\", " + "\"" + attributeInfo.value + "\", " + arrayName + ");";
					setAttributes.push_back(attribute);
					
				    attribute = "if (" + arrayName + ".size() > 0) " + attributeInfo.value + " = " + attributeInfo.name + "(" + arrayName + "[0]);";
					setAttributes.push_back(attribute);
					
					attribute = "std::cout << \"---------- " + structIt->first + " : " + attributeInfo.value + " ----------\" << endl;";
					outputAttributes.push_back(attribute);
					attribute = attributeInfo.value + ".output();";
					outputAttributes.push_back(attribute);
					attribute = "std::cout << \"========== " + structIt->first + " : " + attributeInfo.value + " ==========\\n\" << endl;";
					outputAttributes.push_back(attribute);
				}
				
				else if (attributeInfo.varType == MapVar)
				{
					fileInstance << "    " << attributeInfo.type << "<" << attributeInfo.key << ", " << attributeInfo.value << "> " << attributeInfo.name << ";" << endl;
					
					attribute = (idx == 0) ? "" : "\n        ";
					attribute = attribute + attributeInfo.name + ".clear();";
					setAttributes.push_back(attribute);
					
					string arrayName = "_" + attributeInfo.name;
					attribute = "DomNodeArray " + arrayName + ";";
					setAttributes.push_back(attribute);
					
					if (isBaseType(attributeInfo.value))
						attribute = "CXmlConfig::getNode(parent, " + arrayName + ", \"" + attributeInfo.name + "\", \"value\"" + ", \"" + attributeInfo.value + "\");";
					else
						attribute = "CXmlConfig::getNode(parent, " + arrayName + ", \"" + attributeInfo.name + "\", \"" + attributeInfo.value + "\");";
					setAttributes.push_back(attribute);
					
					// map 的 key & value 形式
					string keyString;
					if (attributeInfo.key == "string")
						keyString = attributeInfo.name + "[CXmlConfig::getKey(" + arrayName + "[i], " + "\"key\")] = ";
				    else if (attributeInfo.key == "int" || attributeInfo.key == "unsigned int")
						keyString = attributeInfo.name + "[CXmlConfig::stringToInt(CXmlConfig::getKey(" + arrayName + "[i], " + "\"key\"))] = ";
					else if (attributeInfo.key == "long" || attributeInfo.key == "unsigned long")
						keyString = attributeInfo.name + "[CXmlConfig::stringToLong(CXmlConfig::getKey(" + arrayName + "[i], " + "\"key\"))] = ";
					else if (attributeInfo.key == "double")
						keyString = attributeInfo.name + "[CXmlConfig::stringToDouble(CXmlConfig::getKey(" + arrayName + "[i], " + "\"key\"))] = ";
					
					string valueString;
					if (attributeInfo.value == "string")
						valueString = "CXmlConfig::getValue(" + arrayName + "[i], " + "\"value\");";
				    else if (attributeInfo.value == "int" || attributeInfo.value == "unsigned int")
						valueString = "CXmlConfig::stringToInt(CXmlConfig::getValue(" + arrayName + "[i], " + "\"value\"));";
					else if (attributeInfo.value == "long" || attributeInfo.value == "unsigned long")
						valueString = "CXmlConfig::stringToLong(CXmlConfig::getValue(" + arrayName + "[i], " + "\"value\"));";
					else if (attributeInfo.value == "double")
						valueString = "CXmlConfig::stringToDouble(CXmlConfig::getValue(" + arrayName + "[i], " + "\"value\"));";
				    else
						valueString = attributeInfo.value + "(" + arrayName + "[i]" + ");";
					
					attribute = "for (unsigned int i = 0; i < " + arrayName + ".size(); ++i)\n        {\n            ";
					attribute = attribute + keyString + valueString + "\n        }";
					setAttributes.push_back(attribute);


					attribute = "std::cout << \"---------- " + structIt->first + " : " + attributeInfo.name + " ----------\" << endl;";
					outputAttributes.push_back(attribute);
					
					if (!isBaseType(attributeInfo.value))
					{
					    attribute = "unsigned int _" + attributeInfo.name + "_count_ = 0;";
					    outputAttributes.push_back(attribute);
					}
					
					attribute = "for (" + attributeInfo.type + "<" + attributeInfo.key + ", " + attributeInfo.value + ">::const_iterator it = " + attributeInfo.name + ".begin(); it != ";
					attribute = attribute + attributeInfo.name + ".end(); ++it)\n        {\n            ";
					
					if (!isBaseType(attributeInfo.value))
					{
						attribute = attribute + "std::cout << \"key = \" << it->first << \", value : \" << endl;\n            it->second.output();";
						attribute = attribute + "\n            if (_" + attributeInfo.name + "_count_++" + " < (" + attributeInfo.name + ".size() - 1)) std::cout << endl;\n        }";
					}
					else
					{
						attribute = attribute + "std::cout << \"key = \" << it->first << \", value = \" << it->second << endl;\n        }";
					}
						
					outputAttributes.push_back(attribute);
					
					attribute = "std::cout << \"========== " + structIt->first + " : " + attributeInfo.name + " ==========\\n\" << endl;";
					outputAttributes.push_back(attribute);
				}
				
				else if (attributeInfo.varType == ArrayVar)
				{
					fileInstance << "    " << "vector<" << attributeInfo.type << "> " << attributeInfo.name << ";" << endl;
					
					attribute = (idx == 0) ? "" : "\n        ";
					attribute = attribute + attributeInfo.name + ".clear();";
					setAttributes.push_back(attribute);
					
					string arrayName = "_" + attributeInfo.name;
					attribute = "DomNodeArray " + arrayName + ";";
					setAttributes.push_back(attribute);
					
					attribute = "CXmlConfig::getNode(parent, " + arrayName + ", \"" + attributeInfo.name + "\", \"" + attributeInfo.type + "\");";
					setAttributes.push_back(attribute);
					
				    attribute = "for (unsigned int i = 0; i < " + arrayName + ".size(); ++i)\n        {\n            " \
					            + attributeInfo.name + ".push_back(" + attributeInfo.type + "(" + arrayName + "[i]));\n        }";
					setAttributes.push_back(attribute);
					
					attribute = "std::cout << \"---------- " + structIt->first + " : " + attributeInfo.name + " ----------\" << endl;";
					outputAttributes.push_back(attribute);
					attribute = "for (unsigned int i = 0; i < " + attributeInfo.name + ".size(); ++i)\n        {\n            " \
					            + attributeInfo.name + "[i].output();\n            if (i < (" + attributeInfo.name + ".size() - 1)) std::cout << endl;\n        }";
					outputAttributes.push_back(attribute);
					attribute = "std::cout << \"========== " + structIt->first + " : " + attributeInfo.name + " ==========\\n\" << endl;";
					outputAttributes.push_back(attribute);
				}
				
				else if (attributeInfo.varType == VectorVar && isBaseType(attributeInfo.value))
				{
					fileInstance << "    " << attributeInfo.type << "<" << attributeInfo.value << "> " << attributeInfo.name << ";" << endl;
					
					attribute = (idx == 0) ? "" : "\n        ";
					attribute = attribute + attributeInfo.name + ".clear();";
					setAttributes.push_back(attribute);
					
					string arrayName = "_" + attributeInfo.name;
					attribute = "DomNodeArray " + arrayName + ";";
					setAttributes.push_back(attribute);
					
					attribute = "CXmlConfig::getNode(parent, " + arrayName + ", \"" + attributeInfo.name + "\", \"value\"" + ", \"" + attributeInfo.value + "\");";
					setAttributes.push_back(attribute);
					
					string valueString;
					if (attributeInfo.value == "string")
						valueString = "CXmlConfig::getValue(" + arrayName + "[i], " + "\"value\"));";
				    else if (attributeInfo.value == "int" || attributeInfo.value == "unsigned int")
						valueString = "CXmlConfig::stringToInt(CXmlConfig::getValue(" + arrayName + "[i], " + "\"value\")));";
					else if (attributeInfo.value == "long" || attributeInfo.value == "unsigned long")
						valueString = "CXmlConfig::stringToLong(CXmlConfig::getValue(" + arrayName + "[i], " + "\"value\")));";
					else if (attributeInfo.value == "double")
						valueString = "CXmlConfig::stringToDouble(CXmlConfig::getValue(" + arrayName + "[i], " + "\"value\")));";
						
					attribute = "for (unsigned int i = 0; i < " + arrayName + ".size(); ++i)\n        {\n            " \
					            + attributeInfo.name + ".push_back(" + valueString + "\n        }";
					setAttributes.push_back(attribute);
					
					attribute = "std::cout << \"---------- " + structIt->first + " : " + attributeInfo.name + " ----------\" << endl;";
					outputAttributes.push_back(attribute);
					
					attribute = "for (unsigned int i = 0; i < " + attributeInfo.name + ".size(); ++i)\n        {\n            " \
					            + "std::cout << \"value = \" << " + attributeInfo.name + "[i] << endl;\n        }";
					outputAttributes.push_back(attribute);
					attribute = "std::cout << \"========== " + structIt->first + " : " + attributeInfo.name + " ==========\\n\" << endl;";
					outputAttributes.push_back(attribute);
				}
			}

            // struct 的构造函数体
	        if (structIt->first != MainType)
			{
			    fileInstance << "\n    " << structIt->first << "() {};" << endl;
	            fileInstance << "\n    " << structIt->first << "(DOMNode* parent)\n    {" << endl;
			}
			else
			{
				fileInstance << "\n    static const " << MainType << "& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)\n    {" << endl;
				fileInstance << "        static " << MainType << " cfgValue(xmlConfigFile);" << endl;
				fileInstance << "        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);" << endl;
			    fileInstance << "        return cfgValue;\n    }\n" << endl;
				
				fileInstance << "    " << MainType << "() {};\n    virtual ~" << MainType << "() {};\n    " << MainType << "(const char* xmlConfigFile)\n    {" << endl;
			    fileInstance << "        CXmlConfig::setConfigValue(xmlConfigFile, *this);\n    }\n" << endl;
				
	            fileInstance << "    virtual void set(DOMNode* parent)\n    {" << endl;
			}
	        for (vector<string>::iterator setAttrList = setAttributes.begin(); setAttrList != setAttributes.end(); setAttrList++)
			{
				fileInstance << "        " << *setAttrList << endl;
			}
			fileInstance << "    }" << endl;
			setAttributes.clear();
			
			fileInstance << "\n    void output() const\n    {" << endl;
			if (structIt->first == MainType)
			{
                fileInstance << "        const time_t __currentSecs__ = time(NULL);" << endl;
				fileInstance << "        const string __currentTime__ = ctime(&__currentSecs__);" << endl;
				fileInstance << "        std::cout << \"\\n!!!!!! set config value result = \" << isSetConfigValueSuccess() << \" !!!!!! current time = \" << __currentTime__;" << endl;
			}
			
	        for (unsigned int i = 0; i < outputAttributes.size(); ++i)
			{
				fileInstance << "        " << outputAttributes[i] << endl;
			}
			fileInstance << "    }" << endl;
			outputAttributes.clear();
	
		    fileInstance << "};\n" << endl;
		}
		
	    fileInstance << "}\n" << endl;
		fileInstance << "#endif  // " << upperName << endl << endl;
		
		fileInstance.close();
		
		return fileName;
	}

private:
	void makeCppType(DOMNode* node, string parentNode)
	{
		if (node == NULL) return;
	
		if (node->getNodeType() == DOMNode::ELEMENT_NODE)
        {
			string type;
			getAttribute(node, TypeTag, type);
			if (type.empty()) return;
			
			string strName;
			charArrayToString(node->getNodeName(), strName);
			
			if (type == StructType)
			{
				if (m_structName2Define.find(strName) == m_structName2Define.end())
				{
					m_structName2Define[strName] = XmlStructType();
					
					string mainClassValue;
					getAttribute(node, MainClass, mainClassValue);

					if (mainClassValue == "true")
					{
						MainType = strName;
						getAttribute(node, NameTag, m_name);
						getAttribute(node, IncludeTag, m_include);
					}
				}
				else
				{
					for (vector<string>::iterator structNameIt = m_structNames.begin(); structNameIt != m_structNames.end(); structNameIt++)
					{
						if (*structNameIt == strName)
						{
							m_structNames.erase(structNameIt);  // 生成代码struct定义顺序需要，先删除之前的
							break;
						}
					}
				}
				
				m_structNames.push_back(strName);
			}

			// struct 数据类定义&属性
			if (m_structName2Define.find(parentNode) != m_structName2Define.end())
			{
				bool isFind = false;
				XmlStructType& xmlType = m_structName2Define[parentNode];
				for (vector<CppType>::iterator attrList = xmlType.attributeList.begin(); attrList != xmlType.attributeList.end(); attrList++)
				{
					if (attrList->name == strName)
					{
						isFind = true;
						break;
					}
				}
				
				string value;
				if (!isFind)
				{
					string key;
					VariableType varType = BaseVar;
					if (type == ArrayType)
					{
						varType = ArrayVar;
						DOMNodeList* nodeList = node->getChildNodes();
						if (nodeList->getLength() > 1) charArrayToString(nodeList->item(1)->getNodeName(), type);
					}
					else if (type == VectorType)
					{
						varType = VectorVar;
						getAttribute(node, "value", value);
					}
					else if (type == StructType)
					{
						varType = StructVar;
						getAttribute(node, "param", value);
					}
					else if (type == MapType || type == UnorderMapType)
					{
						varType = MapVar;
						getAttribute(node, "key", key);
						getAttribute(node, "value", value);
					}
					
					xmlType.attributeList.push_back(CppType(type, strName, value, key, varType));
				}
				else if (type == StructType)
				{
					getAttribute(node, "param", value);
					if (value.size() > 0) xmlType.attributeList.push_back(CppType(type, strName, value, "", StructVar));
				}
			}

			DOMNodeList* nodeList = node->getChildNodes();
			for (unsigned int i = 0; i < nodeList->getLength(); ++i)
			{
				makeCppType(nodeList->item(i), strName);
			}
        }
	}

	void charArrayToString(const XMLCh* xmlChar, string& value)
	{
		if (xmlChar != NULL)
		{
			char* charArray = XMLString::transcode(xmlChar);
			value = charArray;
			XMLString::release(&charArray);
		}
	}
	
	void getAttribute(DOMNode* node, const char* name, string& value)
	{
		if (node->getNodeType() == DOMNode::ELEMENT_NODE)
        {
			XMLCh* typeValue = XMLString::transcode(name);
			charArrayToString(((DOMElement*)node)->getAttribute(typeValue), value);
			XMLString::release(&typeValue);
		}
	}
	
	bool isBaseType(const string& type)
	{
		return (type == "string" || type == "int" || type == "unsigned int" || type == "long" || type == "unsigned long" || type == "double");
	}
	
	void printfInfo()
	{
		cout << "out put xml to c++ file info, main config name = " << m_name << endl;
		for (StructName2Define::iterator structIt = m_structName2Define.begin(); structIt != m_structName2Define.end(); structIt++)
		{
			cout << "struct name = " << structIt->first << " : \n";
			
			for (vector<CppType>::iterator attrList = structIt->second.attributeList.begin(); attrList != structIt->second.attributeList.end(); attrList++)
			{
				cout << attrList->type << " : " << attrList->name << " ; is array = " << attrList->varType << "\n";
			}
			cout << endl;
		}
	}


private:
    XercesDOMParser* m_DOMXmlParser;
	StructName2Define m_structName2Define;
	vector<string> m_structNames;
	string m_name;
	string m_include;
};


int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("please input xml config file and generate c++ file path!\nUsage example : exefile ./config.xml ./dir\n");
		return -1;
	}
	
	CXmlToCPP xml2cpp;
	if (xml2cpp.xmlParser(argv[1]))  // 构造C++数据类型定义
	{
    	string fileName = xml2cpp.outputToFile(argv[2]);  // 生成C++数据类型定义文件
		if (!fileName.empty()) printf("change xml config to c++ file finish, xml file = %s, c++ file = %s\n", argv[1], fileName.c_str());
	}
	else
	{
		printf("xml config format invalid\n");
	}

	return 0;
}

