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
    unsigned int hall_login_id;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        hall_login_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "hall_login_id"));
    }

    void output() const
    {
        std::cout << "CommonCfg : hall_login_id = " << hall_login_id << endl;
    }
};

struct config : public IXmlConfigBase
{
    CommonCfg commonCfg;
    unordered_map<unsigned int, unsigned int> moduleId2ServiceId;

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
        
        moduleId2ServiceId.clear();
        DomNodeArray _moduleId2ServiceId;
        CXmlConfig::getNode(parent, _moduleId2ServiceId, "moduleId2ServiceId", "value", "unsigned int");
        for (unsigned int i = 0; i < _moduleId2ServiceId.size(); ++i)
        {
            moduleId2ServiceId[CXmlConfig::stringToInt(CXmlConfig::getKey(_moduleId2ServiceId[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_moduleId2ServiceId[i], "value"));
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- config : commonCfg ----------" << endl;
        commonCfg.output();
        std::cout << "========== config : commonCfg ==========\n" << endl;
        std::cout << "---------- config : moduleId2ServiceId ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = moduleId2ServiceId.begin(); it != moduleId2ServiceId.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== config : moduleId2ServiceId ==========\n" << endl;
    }
};

}

#endif  // __GATEWAYPROXYCONFIG_H__

