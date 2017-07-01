#ifndef __LOGINLISTCONFIG_H__
#define __LOGINLISTCONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace LoginListConfig
{

struct ServiceInfo
{
    string ip;
    unsigned int port;

    ServiceInfo() {};

    ServiceInfo(DOMNode* parent)
    {
        ip = CXmlConfig::getValue(parent, "ip");
        port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "port"));
    }

    void output() const
    {
        std::cout << "ServiceInfo : ip = " << ip << endl;
        std::cout << "ServiceInfo : port = " << port << endl;
    }
};

struct config : public IXmlConfigBase
{
    unordered_map<unsigned int, ServiceInfo> filter_service_list;

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
        filter_service_list.clear();
        DomNodeArray _filter_service_list;
        CXmlConfig::getNode(parent, _filter_service_list, "filter_service_list", "ServiceInfo");
        for (unsigned int i = 0; i < _filter_service_list.size(); ++i)
        {
            filter_service_list[CXmlConfig::stringToInt(CXmlConfig::getKey(_filter_service_list[i], "key"))] = ServiceInfo(_filter_service_list[i]);
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- config : filter_service_list ----------" << endl;
        unsigned int _filter_service_list_count_ = 0;
        for (unordered_map<unsigned int, ServiceInfo>::const_iterator it = filter_service_list.begin(); it != filter_service_list.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_filter_service_list_count_++ < (filter_service_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : filter_service_list ==========\n" << endl;
    }
};

}

#endif  // __LOGINLISTCONFIG_H__

