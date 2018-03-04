#ifndef __GATEWAYPROXYCONFIG_H__
#define __GATEWAYPROXYCONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace GatewayProxyConfig
{

struct CommonCfg
{
    unsigned int game_hall_id;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        game_hall_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "game_hall_id"));
    }

    void output() const
    {
        std::cout << "CommonCfg : game_hall_id = " << game_hall_id << endl;
    }
};

struct config : public IXmlConfigBase
{
    CommonCfg commonCfg;

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
        DomNodeArray _commonCfg;
        CXmlConfig::getNode(parent, "param", "commonCfg", _commonCfg);
        if (_commonCfg.size() > 0) commonCfg = CommonCfg(_commonCfg[0]);
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- config : commonCfg ----------" << endl;
        commonCfg.output();
        std::cout << "========== config : commonCfg ==========\n" << endl;
    }
};

}

#endif  // __GATEWAYPROXYCONFIG_H__

