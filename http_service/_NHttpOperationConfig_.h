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

struct PhoneMessageCfg
{
    string host;
    string protocol;
    string app_id;
    string app_key;
    string code_url;
    string notify_url;
    unsigned int code_template_id;
    unsigned int notify_template_id;

    PhoneMessageCfg() {};

    PhoneMessageCfg(DOMNode* parent)
    {
        host = CXmlConfig::getValue(parent, "host");
        protocol = CXmlConfig::getValue(parent, "protocol");
        app_id = CXmlConfig::getValue(parent, "app_id");
        app_key = CXmlConfig::getValue(parent, "app_key");
        code_url = CXmlConfig::getValue(parent, "code_url");
        notify_url = CXmlConfig::getValue(parent, "notify_url");
        code_template_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "code_template_id"));
        notify_template_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "notify_template_id"));
    }

    void output() const
    {
        std::cout << "PhoneMessageCfg : host = " << host << endl;
        std::cout << "PhoneMessageCfg : protocol = " << protocol << endl;
        std::cout << "PhoneMessageCfg : app_id = " << app_id << endl;
        std::cout << "PhoneMessageCfg : app_key = " << app_key << endl;
        std::cout << "PhoneMessageCfg : code_url = " << code_url << endl;
        std::cout << "PhoneMessageCfg : notify_url = " << notify_url << endl;
        std::cout << "PhoneMessageCfg : code_template_id = " << code_template_id << endl;
        std::cout << "PhoneMessageCfg : notify_template_id = " << notify_template_id << endl;
    }
};

struct HttpOptConfig : public IXmlConfigBase
{
    PhoneMessageCfg phone_message_cfg;
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
        DomNodeArray _phone_message_cfg;
        CXmlConfig::getNode(parent, "param", "phone_message_cfg", _phone_message_cfg);
        if (_phone_message_cfg.size() > 0) phone_message_cfg = PhoneMessageCfg(_phone_message_cfg[0]);
        
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
        std::cout << "---------- HttpOptConfig : phone_message_cfg ----------" << endl;
        phone_message_cfg.output();
        std::cout << "========== HttpOptConfig : phone_message_cfg ==========\n" << endl;
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

