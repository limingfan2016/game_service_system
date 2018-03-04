#ifndef __NOPTMGRCONFIG_H__
#define __NOPTMGRCONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace NOptMgrConfig
{

struct client_version
{
    unsigned int check_flag;
    string check_version;
    string version;
    string url;
    string valid;

    client_version() {};

    client_version(DOMNode* parent)
    {
        check_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "check_flag"));
        check_version = CXmlConfig::getValue(parent, "check_version");
        version = CXmlConfig::getValue(parent, "version");
        url = CXmlConfig::getValue(parent, "url");
        valid = CXmlConfig::getValue(parent, "valid");
    }

    void output() const
    {
        std::cout << "client_version : check_flag = " << check_flag << endl;
        std::cout << "client_version : check_version = " << check_version << endl;
        std::cout << "client_version : version = " << version << endl;
        std::cout << "client_version : url = " << url << endl;
        std::cout << "client_version : valid = " << valid << endl;
    }
};

struct RechargeCfg
{
    string sign_key;
    string post_url;
    map<string, string> gold_price_count;

    RechargeCfg() {};

    RechargeCfg(DOMNode* parent)
    {
        sign_key = CXmlConfig::getValue(parent, "sign_key");
        post_url = CXmlConfig::getValue(parent, "post_url");
        
        gold_price_count.clear();
        DomNodeArray _gold_price_count;
        CXmlConfig::getNode(parent, _gold_price_count, "gold_price_count", "value", "string");
        for (unsigned int i = 0; i < _gold_price_count.size(); ++i)
        {
            gold_price_count[CXmlConfig::getKey(_gold_price_count[i], "key")] = CXmlConfig::getValue(_gold_price_count[i], "value");
        }
    }

    void output() const
    {
        std::cout << "RechargeCfg : sign_key = " << sign_key << endl;
        std::cout << "RechargeCfg : post_url = " << post_url << endl;
        std::cout << "---------- RechargeCfg : gold_price_count ----------" << endl;
        for (map<string, string>::const_iterator it = gold_price_count.begin(); it != gold_price_count.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== RechargeCfg : gold_price_count ==========\n" << endl;
    }
};

struct TimingTaskCfg
{
    unsigned int day_hour;
    unsigned int save_user_login_info_day;

    TimingTaskCfg() {};

    TimingTaskCfg(DOMNode* parent)
    {
        day_hour = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "day_hour"));
        save_user_login_info_day = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "save_user_login_info_day"));
    }

    void output() const
    {
        std::cout << "TimingTaskCfg : day_hour = " << day_hour << endl;
        std::cout << "TimingTaskCfg : save_user_login_info_day = " << save_user_login_info_day << endl;
    }
};

struct CommonCfg
{
    double chess_hall_init_gold;
    unsigned int max_chat_context_size;
    unsigned int max_chat_record_size_to_db;
    unsigned int chat_record_to_db_interval;
    unsigned int get_player_count;
    unsigned int manager_get_mall_record_count;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        chess_hall_init_gold = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "chess_hall_init_gold"));
        max_chat_context_size = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_chat_context_size"));
        max_chat_record_size_to_db = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_chat_record_size_to_db"));
        chat_record_to_db_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "chat_record_to_db_interval"));
        get_player_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "get_player_count"));
        manager_get_mall_record_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "manager_get_mall_record_count"));
    }

    void output() const
    {
        std::cout << "CommonCfg : chess_hall_init_gold = " << chess_hall_init_gold << endl;
        std::cout << "CommonCfg : max_chat_context_size = " << max_chat_context_size << endl;
        std::cout << "CommonCfg : max_chat_record_size_to_db = " << max_chat_record_size_to_db << endl;
        std::cout << "CommonCfg : chat_record_to_db_interval = " << chat_record_to_db_interval << endl;
        std::cout << "CommonCfg : get_player_count = " << get_player_count << endl;
        std::cout << "CommonCfg : manager_get_mall_record_count = " << manager_get_mall_record_count << endl;
    }
};

struct config : public IXmlConfigBase
{
    CommonCfg common_cfg;
    TimingTaskCfg timing_task;
    RechargeCfg recharge_cfg;
    map<int, client_version> apple_version_list;
    map<int, client_version> android_version_list;

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
        DomNodeArray _common_cfg;
        CXmlConfig::getNode(parent, "param", "common_cfg", _common_cfg);
        if (_common_cfg.size() > 0) common_cfg = CommonCfg(_common_cfg[0]);
        
        DomNodeArray _timing_task;
        CXmlConfig::getNode(parent, "param", "timing_task", _timing_task);
        if (_timing_task.size() > 0) timing_task = TimingTaskCfg(_timing_task[0]);
        
        DomNodeArray _recharge_cfg;
        CXmlConfig::getNode(parent, "param", "recharge_cfg", _recharge_cfg);
        if (_recharge_cfg.size() > 0) recharge_cfg = RechargeCfg(_recharge_cfg[0]);
        
        apple_version_list.clear();
        DomNodeArray _apple_version_list;
        CXmlConfig::getNode(parent, _apple_version_list, "apple_version_list", "client_version");
        for (unsigned int i = 0; i < _apple_version_list.size(); ++i)
        {
            apple_version_list[CXmlConfig::stringToInt(CXmlConfig::getKey(_apple_version_list[i], "key"))] = client_version(_apple_version_list[i]);
        }
        
        android_version_list.clear();
        DomNodeArray _android_version_list;
        CXmlConfig::getNode(parent, _android_version_list, "android_version_list", "client_version");
        for (unsigned int i = 0; i < _android_version_list.size(); ++i)
        {
            android_version_list[CXmlConfig::stringToInt(CXmlConfig::getKey(_android_version_list[i], "key"))] = client_version(_android_version_list[i]);
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- config : common_cfg ----------" << endl;
        common_cfg.output();
        std::cout << "========== config : common_cfg ==========\n" << endl;
        std::cout << "---------- config : timing_task ----------" << endl;
        timing_task.output();
        std::cout << "========== config : timing_task ==========\n" << endl;
        std::cout << "---------- config : recharge_cfg ----------" << endl;
        recharge_cfg.output();
        std::cout << "========== config : recharge_cfg ==========\n" << endl;
        std::cout << "---------- config : apple_version_list ----------" << endl;
        unsigned int _apple_version_list_count_ = 0;
        for (map<int, client_version>::const_iterator it = apple_version_list.begin(); it != apple_version_list.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_apple_version_list_count_++ < (apple_version_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : apple_version_list ==========\n" << endl;
        std::cout << "---------- config : android_version_list ----------" << endl;
        unsigned int _android_version_list_count_ = 0;
        for (map<int, client_version>::const_iterator it = android_version_list.begin(); it != android_version_list.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_android_version_list_count_++ < (android_version_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : android_version_list ==========\n" << endl;
    }
};

}

#endif  // __NOPTMGRCONFIG_H__

