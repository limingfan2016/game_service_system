#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace Config
{

struct monitorInfo
{
    unsigned int server_type;
    unsigned int stat_type;
    unsigned int time_gap;

    monitorInfo() {};

    monitorInfo(DOMNode* parent)
    {
        server_type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "server_type"));
        stat_type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "stat_type"));
        time_gap = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "time_gap"));
    }

    void output() const
    {
        std::cout << "monitorInfo : server_type = " << server_type << endl;
        std::cout << "monitorInfo : stat_type = " << stat_type << endl;
        std::cout << "monitorInfo : time_gap = " << time_gap << endl;
    }
};

struct MysqlConfig
{
    string mysql_ip;
    int mysql_port;
    string mysql_username;
    string mysql_password;
    string mysql_dbname;

    MysqlConfig() {};

    MysqlConfig(DOMNode* parent)
    {
        mysql_ip = CXmlConfig::getValue(parent, "mysql_ip");
        mysql_port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "mysql_port"));
        mysql_username = CXmlConfig::getValue(parent, "mysql_username");
        mysql_password = CXmlConfig::getValue(parent, "mysql_password");
        mysql_dbname = CXmlConfig::getValue(parent, "mysql_dbname");
    }

    void output() const
    {
        std::cout << "MysqlConfig : mysql_ip = " << mysql_ip << endl;
        std::cout << "MysqlConfig : mysql_port = " << mysql_port << endl;
        std::cout << "MysqlConfig : mysql_username = " << mysql_username << endl;
        std::cout << "MysqlConfig : mysql_password = " << mysql_password << endl;
        std::cout << "MysqlConfig : mysql_dbname = " << mysql_dbname << endl;
    }
};

struct config : public IXmlConfigBase
{
    unsigned int cs_reply_life_time;
    unsigned int cs_reply_check_gap;
    MysqlConfig mysql_config;
    vector<monitorInfo> monitor_info_list;

    static const config& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static config cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    config() {};
    virtual ~config() {};
    config(const char* xmlConfigFile)
    {
        CXmlConfig::setConfigValue(xmlConfigFile, *this);
    }

    virtual void set(DOMNode* parent)
    {
        cs_reply_life_time = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "cs_reply_life_time"));
        cs_reply_check_gap = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "cs_reply_check_gap"));
        
        DomNodeArray _mysql_config;
        CXmlConfig::getNode(parent, "param", "mysql_config", _mysql_config);
        if (_mysql_config.size() > 0) mysql_config = MysqlConfig(_mysql_config[0]);
        
        monitor_info_list.clear();
        DomNodeArray _monitor_info_list;
        CXmlConfig::getNode(parent, _monitor_info_list, "monitor_info_list", "monitorInfo");
        for (unsigned int i = 0; i < _monitor_info_list.size(); ++i)
        {
            monitor_info_list.push_back(monitorInfo(_monitor_info_list[i]));
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "config : cs_reply_life_time = " << cs_reply_life_time << endl;
        std::cout << "config : cs_reply_check_gap = " << cs_reply_check_gap << endl;
        std::cout << "---------- config : mysql_config ----------" << endl;
        mysql_config.output();
        std::cout << "========== config : mysql_config ==========\n" << endl;
        std::cout << "---------- config : monitor_info_list ----------" << endl;
        for (unsigned int i = 0; i < monitor_info_list.size(); ++i)
        {
            monitor_info_list[i].output();
            if (i < (monitor_info_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : monitor_info_list ==========\n" << endl;
    }
};

}

#endif  // __CONFIG_H__

