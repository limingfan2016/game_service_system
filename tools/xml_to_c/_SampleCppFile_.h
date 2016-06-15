#ifndef __SAMPLECPPFILE_H__
#define __SAMPLECPPFILE_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace SampleCppFile
{

struct outsideStruct
{
    int id;
    long num;
    double gold;

    outsideStruct() {};

    outsideStruct(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        num = CXmlConfig::stringToLong(CXmlConfig::getValue(parent, "num"));
        gold = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "gold"));
    }

    void output() const
    {
        std::cout << "outsideStruct : id = " << id << endl;
        std::cout << "outsideStruct : num = " << num << endl;
        std::cout << "outsideStruct : gold = " << gold << endl;
    }
};

struct TestStruct
{
    int id;
    long num;
    double gold;
    outsideStruct structInStructInArray;

    TestStruct() {};

    TestStruct(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        num = CXmlConfig::stringToLong(CXmlConfig::getValue(parent, "num"));
        gold = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "gold"));
        
        DomNodeArray _structInStructInArray;
        CXmlConfig::getNode(parent, "param", "structInStructInArray", _structInStructInArray);
        if (_structInStructInArray.size() > 0) structInStructInArray = outsideStruct(_structInStructInArray[0]);
    }

    void output() const
    {
        std::cout << "TestStruct : id = " << id << endl;
        std::cout << "TestStruct : num = " << num << endl;
        std::cout << "TestStruct : gold = " << gold << endl;
        std::cout << "---------- TestStruct : structInStructInArray ----------" << endl;
        structInStructInArray.output();
        std::cout << "========== TestStruct : structInStructInArray ==========\n" << endl;
    }
};

struct outsideStruct2
{
    outsideStruct outsize;

    outsideStruct2() {};

    outsideStruct2(DOMNode* parent)
    {
        DomNodeArray _outsize;
        CXmlConfig::getNode(parent, "param", "outsize", _outsize);
        if (_outsize.size() > 0) outsize = outsideStruct(_outsize[0]);
    }

    void output() const
    {
        std::cout << "---------- outsideStruct2 : outsize ----------" << endl;
        outsize.output();
        std::cout << "========== outsideStruct2 : outsize ==========\n" << endl;
    }
};

struct login_reward
{
    int id;
    long num;
    double gold;

    login_reward() {};

    login_reward(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        num = CXmlConfig::stringToLong(CXmlConfig::getValue(parent, "num"));
        gold = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "gold"));
    }

    void output() const
    {
        std::cout << "login_reward : id = " << id << endl;
        std::cout << "login_reward : num = " << num << endl;
        std::cout << "login_reward : gold = " << gold << endl;
    }
};

struct server_dst_info
{
    string server_dst_ip;
    int server_dst_port;

    server_dst_info() {};

    server_dst_info(DOMNode* parent)
    {
        server_dst_ip = CXmlConfig::getValue(parent, "server_dst_ip");
        server_dst_port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "server_dst_port"));
    }

    void output() const
    {
        std::cout << "server_dst_info : server_dst_ip = " << server_dst_ip << endl;
        std::cout << "server_dst_info : server_dst_port = " << server_dst_port << endl;
    }
};

struct server_type_info
{
    int server_type;
    string server_type_desc;
    double server_dst_count;
    vector<server_dst_info> server_dst_list;

    server_type_info() {};

    server_type_info(DOMNode* parent)
    {
        server_type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "server_type"));
        server_type_desc = CXmlConfig::getValue(parent, "server_type_desc");
        server_dst_count = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "server_dst_count"));
        
        server_dst_list.clear();
        DomNodeArray _server_dst_list;
        CXmlConfig::getNode(parent, _server_dst_list, "server_dst_list", "server_dst_info");
        for (unsigned int i = 0; i < _server_dst_list.size(); ++i)
        {
            server_dst_list.push_back(server_dst_info(_server_dst_list[i]));
        }
    }

    void output() const
    {
        std::cout << "server_type_info : server_type = " << server_type << endl;
        std::cout << "server_type_info : server_type_desc = " << server_type_desc << endl;
        std::cout << "server_type_info : server_dst_count = " << server_dst_count << endl;
        std::cout << "---------- server_type_info : server_dst_list ----------" << endl;
        for (unsigned int i = 0; i < server_dst_list.size(); ++i)
        {
            server_dst_list[i].output();
            if (i < (server_dst_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== server_type_info : server_dst_list ==========\n" << endl;
    }
};

struct TestData : public IXmlConfigBase
{
    int server_type_count;
    double server_type_max;
    string server_type_name;
    vector<server_type_info> server_type_list;
    vector<login_reward> server_login_reward;
    outsideStruct outsize;
    outsideStruct2 outsize2;
    vector<TestStruct> structInArray;
    map<int, int> int2intInMap;
    map<int, string> int2stringInMap;
    map<string, long> string2longInMap;
    unordered_map<string, outsideStruct> structInMap;
    vector<string> stringInVector;
    vector<double> doubleInVector;
    vector<unsigned int> unsignedIntInVector;

    static const TestData& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static TestData cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    TestData() {};
    virtual ~TestData() {};
    TestData(const char* xmlConfigFile)
    {
        CXmlConfig::setConfigValue(xmlConfigFile, *this);
    }

    virtual void set(DOMNode* parent)
    {
        server_type_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "server_type_count"));
        server_type_max = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "server_type_max"));
        server_type_name = CXmlConfig::getValue(parent, "server_type_name");
        
        server_type_list.clear();
        DomNodeArray _server_type_list;
        CXmlConfig::getNode(parent, _server_type_list, "server_type_list", "server_type_info");
        for (unsigned int i = 0; i < _server_type_list.size(); ++i)
        {
            server_type_list.push_back(server_type_info(_server_type_list[i]));
        }
        
        server_login_reward.clear();
        DomNodeArray _server_login_reward;
        CXmlConfig::getNode(parent, _server_login_reward, "server_login_reward", "login_reward");
        for (unsigned int i = 0; i < _server_login_reward.size(); ++i)
        {
            server_login_reward.push_back(login_reward(_server_login_reward[i]));
        }
        
        DomNodeArray _outsize;
        CXmlConfig::getNode(parent, "param", "outsize", _outsize);
        if (_outsize.size() > 0) outsize = outsideStruct(_outsize[0]);
        
        DomNodeArray _outsize2;
        CXmlConfig::getNode(parent, "param", "outsize2", _outsize2);
        if (_outsize2.size() > 0) outsize2 = outsideStruct2(_outsize2[0]);
        
        structInArray.clear();
        DomNodeArray _structInArray;
        CXmlConfig::getNode(parent, _structInArray, "structInArray", "TestStruct");
        for (unsigned int i = 0; i < _structInArray.size(); ++i)
        {
            structInArray.push_back(TestStruct(_structInArray[i]));
        }
        
        int2intInMap.clear();
        DomNodeArray _int2intInMap;
        CXmlConfig::getNode(parent, _int2intInMap, "int2intInMap", "value", "int");
        for (unsigned int i = 0; i < _int2intInMap.size(); ++i)
        {
            int2intInMap[CXmlConfig::stringToInt(CXmlConfig::getKey(_int2intInMap[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_int2intInMap[i], "value"));
        }
        
        int2stringInMap.clear();
        DomNodeArray _int2stringInMap;
        CXmlConfig::getNode(parent, _int2stringInMap, "int2stringInMap", "value", "string");
        for (unsigned int i = 0; i < _int2stringInMap.size(); ++i)
        {
            int2stringInMap[CXmlConfig::stringToInt(CXmlConfig::getKey(_int2stringInMap[i], "key"))] = CXmlConfig::getValue(_int2stringInMap[i], "value");
        }
        
        string2longInMap.clear();
        DomNodeArray _string2longInMap;
        CXmlConfig::getNode(parent, _string2longInMap, "string2longInMap", "value", "long");
        for (unsigned int i = 0; i < _string2longInMap.size(); ++i)
        {
            string2longInMap[CXmlConfig::getKey(_string2longInMap[i], "key")] = CXmlConfig::stringToLong(CXmlConfig::getValue(_string2longInMap[i], "value"));
        }
        
        structInMap.clear();
        DomNodeArray _structInMap;
        CXmlConfig::getNode(parent, _structInMap, "structInMap", "outsideStruct");
        for (unsigned int i = 0; i < _structInMap.size(); ++i)
        {
            structInMap[CXmlConfig::getKey(_structInMap[i], "key")] = outsideStruct(_structInMap[i]);
        }
        
        stringInVector.clear();
        DomNodeArray _stringInVector;
        CXmlConfig::getNode(parent, _stringInVector, "stringInVector", "value", "string");
        for (unsigned int i = 0; i < _stringInVector.size(); ++i)
        {
            stringInVector.push_back(CXmlConfig::getValue(_stringInVector[i], "value"));
        }
        
        doubleInVector.clear();
        DomNodeArray _doubleInVector;
        CXmlConfig::getNode(parent, _doubleInVector, "doubleInVector", "value", "double");
        for (unsigned int i = 0; i < _doubleInVector.size(); ++i)
        {
            doubleInVector.push_back(CXmlConfig::stringToDouble(CXmlConfig::getValue(_doubleInVector[i], "value")));
        }
        
        unsignedIntInVector.clear();
        DomNodeArray _unsignedIntInVector;
        CXmlConfig::getNode(parent, _unsignedIntInVector, "unsignedIntInVector", "value", "unsigned int");
        for (unsigned int i = 0; i < _unsignedIntInVector.size(); ++i)
        {
            unsignedIntInVector.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_unsignedIntInVector[i], "value")));
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "TestData : server_type_count = " << server_type_count << endl;
        std::cout << "TestData : server_type_max = " << server_type_max << endl;
        std::cout << "TestData : server_type_name = " << server_type_name << endl;
        std::cout << "---------- TestData : server_type_list ----------" << endl;
        for (unsigned int i = 0; i < server_type_list.size(); ++i)
        {
            server_type_list[i].output();
            if (i < (server_type_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== TestData : server_type_list ==========\n" << endl;
        std::cout << "---------- TestData : server_login_reward ----------" << endl;
        for (unsigned int i = 0; i < server_login_reward.size(); ++i)
        {
            server_login_reward[i].output();
            if (i < (server_login_reward.size() - 1)) std::cout << endl;
        }
        std::cout << "========== TestData : server_login_reward ==========\n" << endl;
        std::cout << "---------- TestData : outsize ----------" << endl;
        outsize.output();
        std::cout << "========== TestData : outsize ==========\n" << endl;
        std::cout << "---------- TestData : outsize2 ----------" << endl;
        outsize2.output();
        std::cout << "========== TestData : outsize2 ==========\n" << endl;
        std::cout << "---------- TestData : structInArray ----------" << endl;
        for (unsigned int i = 0; i < structInArray.size(); ++i)
        {
            structInArray[i].output();
            if (i < (structInArray.size() - 1)) std::cout << endl;
        }
        std::cout << "========== TestData : structInArray ==========\n" << endl;
        std::cout << "---------- TestData : int2intInMap ----------" << endl;
        for (map<int, int>::const_iterator it = int2intInMap.begin(); it != int2intInMap.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== TestData : int2intInMap ==========\n" << endl;
        std::cout << "---------- TestData : int2stringInMap ----------" << endl;
        for (map<int, string>::const_iterator it = int2stringInMap.begin(); it != int2stringInMap.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== TestData : int2stringInMap ==========\n" << endl;
        std::cout << "---------- TestData : string2longInMap ----------" << endl;
        for (map<string, long>::const_iterator it = string2longInMap.begin(); it != string2longInMap.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== TestData : string2longInMap ==========\n" << endl;
        std::cout << "---------- TestData : structInMap ----------" << endl;
        unsigned int _structInMap_count_ = 0;
        for (unordered_map<string, outsideStruct>::const_iterator it = structInMap.begin(); it != structInMap.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_structInMap_count_++ < (structInMap.size() - 1)) std::cout << endl;
        }
        std::cout << "========== TestData : structInMap ==========\n" << endl;
        std::cout << "---------- TestData : stringInVector ----------" << endl;
        for (unsigned int i = 0; i < stringInVector.size(); ++i)
        {
            std::cout << "value = " << stringInVector[i] << endl;
        }
        std::cout << "========== TestData : stringInVector ==========\n" << endl;
        std::cout << "---------- TestData : doubleInVector ----------" << endl;
        for (unsigned int i = 0; i < doubleInVector.size(); ++i)
        {
            std::cout << "value = " << doubleInVector[i] << endl;
        }
        std::cout << "========== TestData : doubleInVector ==========\n" << endl;
        std::cout << "---------- TestData : unsignedIntInVector ----------" << endl;
        for (unsigned int i = 0; i < unsignedIntInVector.size(); ++i)
        {
            std::cout << "value = " << unsignedIntInVector[i] << endl;
        }
        std::cout << "========== TestData : unsignedIntInVector ==========\n" << endl;
    }
};

}

#endif  // __SAMPLECPPFILE_H__

