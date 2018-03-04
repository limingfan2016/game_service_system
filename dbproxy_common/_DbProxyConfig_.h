#ifndef __DBPROXYCONFIG_H__
#define __DBPROXYCONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace DbProxyConfig
{

struct RegisterInit
{
    double game_gold;
    double room_card;

    RegisterInit() {};

    RegisterInit(DOMNode* parent)
    {
        game_gold = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "game_gold"));
        room_card = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "room_card"));
    }

    void output() const
    {
        std::cout << "RegisterInit : game_gold = " << game_gold << endl;
        std::cout << "RegisterInit : room_card = " << room_card << endl;
    }
};

struct CommonConfig
{
    unsigned int nickname_length;

    CommonConfig() {};

    CommonConfig(DOMNode* parent)
    {
        nickname_length = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "nickname_length"));
    }

    void output() const
    {
        std::cout << "CommonConfig : nickname_length = " << nickname_length << endl;
    }
};

struct config : public IXmlConfigBase
{
    CommonConfig common_cfg;
    RegisterInit register_init;

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
        if (_common_cfg.size() > 0) common_cfg = CommonConfig(_common_cfg[0]);
        
        DomNodeArray _register_init;
        CXmlConfig::getNode(parent, "param", "register_init", _register_init);
        if (_register_init.size() > 0) register_init = RegisterInit(_register_init[0]);
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- config : common_cfg ----------" << endl;
        common_cfg.output();
        std::cout << "========== config : common_cfg ==========\n" << endl;
        std::cout << "---------- config : register_init ----------" << endl;
        register_init.output();
        std::cout << "========== config : register_init ==========\n" << endl;
    }
};

}

#endif  // __DBPROXYCONFIG_H__

