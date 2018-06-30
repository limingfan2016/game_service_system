#ifndef __NGOLDENFRAUDCONFIGDATA_H__
#define __NGOLDENFRAUDCONFIGDATA_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace NGoldenFraudConfigData
{

struct CommonCfg
{
    unsigned int flag;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "flag"));
    }

    void output() const
    {
        std::cout << "CommonCfg : flag = " << flag << endl;
    }
};

struct GoldenFraudConfig : public IXmlConfigBase
{
    CommonCfg common_cfg;

    static const GoldenFraudConfig& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static GoldenFraudConfig cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    GoldenFraudConfig() {};
    virtual ~GoldenFraudConfig() {};
    GoldenFraudConfig(const char* xmlConfigFile)
    {
        CXmlConfig::setConfigValue(xmlConfigFile, *this);
    }

    virtual void set(DOMNode* parent)
    {
        DomNodeArray _common_cfg;
        CXmlConfig::getNode(parent, "param", "common_cfg", _common_cfg);
        if (_common_cfg.size() > 0) common_cfg = CommonCfg(_common_cfg[0]);
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- GoldenFraudConfig : common_cfg ----------" << endl;
        common_cfg.output();
        std::cout << "========== GoldenFraudConfig : common_cfg ==========\n" << endl;
    }
};

}

#endif  // __NGOLDENFRAUDCONFIGDATA_H__

