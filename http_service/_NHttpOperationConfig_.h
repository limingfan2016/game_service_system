#ifndef __NHTTPOPERATIONCONFIG_H__
#define __NHTTPOPERATIONCONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace NHttpOperationConfig
{

struct ThirdPlatformInfo
{
    unsigned int check_host;
    string host;
    string protocol;
    map<string, string> urls;
    string app_id;
    string app_key;
    string recharge_log_file_name;
    unsigned int recharge_log_file_size;
    unsigned int recharge_log_file_count;

    ThirdPlatformInfo() {};

    ThirdPlatformInfo(DOMNode* parent)
    {
        check_host = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "check_host"));
        host = CXmlConfig::getValue(parent, "host");
        protocol = CXmlConfig::getValue(parent, "protocol");
        
        urls.clear();
        DomNodeArray _urls;
        CXmlConfig::getNode(parent, _urls, "urls", "value", "string");
        for (unsigned int i = 0; i < _urls.size(); ++i)
        {
            urls[CXmlConfig::getKey(_urls[i], "key")] = CXmlConfig::getValue(_urls[i], "value");
        }
        app_id = CXmlConfig::getValue(parent, "app_id");
        app_key = CXmlConfig::getValue(parent, "app_key");
        recharge_log_file_name = CXmlConfig::getValue(parent, "recharge_log_file_name");
        recharge_log_file_size = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "recharge_log_file_size"));
        recharge_log_file_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "recharge_log_file_count"));
    }

    void output() const
    {
        std::cout << "ThirdPlatformInfo : check_host = " << check_host << endl;
        std::cout << "ThirdPlatformInfo : host = " << host << endl;
        std::cout << "ThirdPlatformInfo : protocol = " << protocol << endl;
        std::cout << "---------- ThirdPlatformInfo : urls ----------" << endl;
        for (map<string, string>::const_iterator it = urls.begin(); it != urls.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== ThirdPlatformInfo : urls ==========\n" << endl;
        std::cout << "ThirdPlatformInfo : app_id = " << app_id << endl;
        std::cout << "ThirdPlatformInfo : app_key = " << app_key << endl;
        std::cout << "ThirdPlatformInfo : recharge_log_file_name = " << recharge_log_file_name << endl;
        std::cout << "ThirdPlatformInfo : recharge_log_file_size = " << recharge_log_file_size << endl;
        std::cout << "ThirdPlatformInfo : recharge_log_file_count = " << recharge_log_file_count << endl;
    }
};

struct WXTokenTimeOut
{
    unsigned int adjust_secs;
    unsigned int refresh_token_last_secs;

    WXTokenTimeOut() {};

    WXTokenTimeOut(DOMNode* parent)
    {
        adjust_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "adjust_secs"));
        refresh_token_last_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "refresh_token_last_secs"));
    }

    void output() const
    {
        std::cout << "WXTokenTimeOut : adjust_secs = " << adjust_secs << endl;
        std::cout << "WXTokenTimeOut : refresh_token_last_secs = " << refresh_token_last_secs << endl;
    }
};

struct CheckPhoneNumberCfg
{
    string host;
    string protocol;
    string url;
    string app_id;
    string app_key;
    unsigned int template_id;

    CheckPhoneNumberCfg() {};

    CheckPhoneNumberCfg(DOMNode* parent)
    {
        host = CXmlConfig::getValue(parent, "host");
        protocol = CXmlConfig::getValue(parent, "protocol");
        url = CXmlConfig::getValue(parent, "url");
        app_id = CXmlConfig::getValue(parent, "app_id");
        app_key = CXmlConfig::getValue(parent, "app_key");
        template_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "template_id"));
    }

    void output() const
    {
        std::cout << "CheckPhoneNumberCfg : host = " << host << endl;
        std::cout << "CheckPhoneNumberCfg : protocol = " << protocol << endl;
        std::cout << "CheckPhoneNumberCfg : url = " << url << endl;
        std::cout << "CheckPhoneNumberCfg : app_id = " << app_id << endl;
        std::cout << "CheckPhoneNumberCfg : app_key = " << app_key << endl;
        std::cout << "CheckPhoneNumberCfg : template_id = " << template_id << endl;
    }
};

struct HttpOptConfig : public IXmlConfigBase
{
    CheckPhoneNumberCfg check_phone_number_cfg;
    WXTokenTimeOut wx_token_timeout;
    map<unsigned int, ThirdPlatformInfo> third_platform_list;

    static const HttpOptConfig& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static HttpOptConfig cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    HttpOptConfig() {};
    virtual ~HttpOptConfig() {};
    HttpOptConfig(const char* xmlConfigFile)
    {
        CXmlConfig::setConfigValue(xmlConfigFile, *this);
    }

    virtual void set(DOMNode* parent)
    {
        DomNodeArray _check_phone_number_cfg;
        CXmlConfig::getNode(parent, "param", "check_phone_number_cfg", _check_phone_number_cfg);
        if (_check_phone_number_cfg.size() > 0) check_phone_number_cfg = CheckPhoneNumberCfg(_check_phone_number_cfg[0]);
        
        DomNodeArray _wx_token_timeout;
        CXmlConfig::getNode(parent, "param", "wx_token_timeout", _wx_token_timeout);
        if (_wx_token_timeout.size() > 0) wx_token_timeout = WXTokenTimeOut(_wx_token_timeout[0]);
        
        third_platform_list.clear();
        DomNodeArray _third_platform_list;
        CXmlConfig::getNode(parent, _third_platform_list, "third_platform_list", "ThirdPlatformInfo");
        for (unsigned int i = 0; i < _third_platform_list.size(); ++i)
        {
            third_platform_list[CXmlConfig::stringToInt(CXmlConfig::getKey(_third_platform_list[i], "key"))] = ThirdPlatformInfo(_third_platform_list[i]);
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- HttpOptConfig : check_phone_number_cfg ----------" << endl;
        check_phone_number_cfg.output();
        std::cout << "========== HttpOptConfig : check_phone_number_cfg ==========\n" << endl;
        std::cout << "---------- HttpOptConfig : wx_token_timeout ----------" << endl;
        wx_token_timeout.output();
        std::cout << "========== HttpOptConfig : wx_token_timeout ==========\n" << endl;
        std::cout << "---------- HttpOptConfig : third_platform_list ----------" << endl;
        unsigned int _third_platform_list_count_ = 0;
        for (map<unsigned int, ThirdPlatformInfo>::const_iterator it = third_platform_list.begin(); it != third_platform_list.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_third_platform_list_count_++ < (third_platform_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== HttpOptConfig : third_platform_list ==========\n" << endl;
    }
};

}

#endif  // __NHTTPOPERATIONCONFIG_H__

