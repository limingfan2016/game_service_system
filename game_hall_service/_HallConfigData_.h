#ifndef __HALLCONFIGDATA_H__
#define __HALLCONFIGDATA_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace HallConfigData
{

struct GameServiceCfg
{
    unordered_map<unsigned int, unsigned int> platform_update_flag;

    GameServiceCfg() {};

    GameServiceCfg(DOMNode* parent)
    {
        platform_update_flag.clear();
        DomNodeArray _platform_update_flag;
        CXmlConfig::getNode(parent, _platform_update_flag, "platform_update_flag", "value", "unsigned int");
        for (unsigned int i = 0; i < _platform_update_flag.size(); ++i)
        {
            platform_update_flag[CXmlConfig::stringToInt(CXmlConfig::getKey(_platform_update_flag[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_platform_update_flag[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "---------- GameServiceCfg : platform_update_flag ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = platform_update_flag.begin(); it != platform_update_flag.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== GameServiceCfg : platform_update_flag ==========\n" << endl;
    }
};

struct CommonCfg
{
    unsigned int get_online_room_count;
    unsigned int cache_message_count;
    unsigned int set_phone_number_interval;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        get_online_room_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "get_online_room_count"));
        cache_message_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "cache_message_count"));
        set_phone_number_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "set_phone_number_interval"));
    }

    void output() const
    {
        std::cout << "CommonCfg : get_online_room_count = " << get_online_room_count << endl;
        std::cout << "CommonCfg : cache_message_count = " << cache_message_count << endl;
        std::cout << "CommonCfg : set_phone_number_interval = " << set_phone_number_interval << endl;
    }
};

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

struct HallConfig : public IXmlConfigBase
{
    map<int, client_version> apple_version_list;
    map<int, client_version> android_version_list;
    CommonCfg common_cfg;
    GameServiceCfg game_service_cfg;

    static const HallConfig& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static HallConfig cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    HallConfig() {};
    virtual ~HallConfig() {};
    HallConfig(const char* xmlConfigFile)
    {
        CXmlConfig::setConfigValue(xmlConfigFile, *this);
    }

    virtual void set(DOMNode* parent)
    {
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
        
        DomNodeArray _common_cfg;
        CXmlConfig::getNode(parent, "param", "common_cfg", _common_cfg);
        if (_common_cfg.size() > 0) common_cfg = CommonCfg(_common_cfg[0]);
        
        DomNodeArray _game_service_cfg;
        CXmlConfig::getNode(parent, "param", "game_service_cfg", _game_service_cfg);
        if (_game_service_cfg.size() > 0) game_service_cfg = GameServiceCfg(_game_service_cfg[0]);
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- HallConfig : apple_version_list ----------" << endl;
        unsigned int _apple_version_list_count_ = 0;
        for (map<int, client_version>::const_iterator it = apple_version_list.begin(); it != apple_version_list.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_apple_version_list_count_++ < (apple_version_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== HallConfig : apple_version_list ==========\n" << endl;
        std::cout << "---------- HallConfig : android_version_list ----------" << endl;
        unsigned int _android_version_list_count_ = 0;
        for (map<int, client_version>::const_iterator it = android_version_list.begin(); it != android_version_list.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_android_version_list_count_++ < (android_version_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== HallConfig : android_version_list ==========\n" << endl;
        std::cout << "---------- HallConfig : common_cfg ----------" << endl;
        common_cfg.output();
        std::cout << "========== HallConfig : common_cfg ==========\n" << endl;
        std::cout << "---------- HallConfig : game_service_cfg ----------" << endl;
        game_service_cfg.output();
        std::cout << "========== HallConfig : game_service_cfg ==========\n" << endl;
    }
};

}

#endif  // __HALLCONFIGDATA_H__

