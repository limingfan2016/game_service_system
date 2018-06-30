#ifndef __NCOMMONCONFIG_H__
#define __NCOMMONCONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace NCommonConfig
{

struct ServiceDataFile
{
    string city_id_mapping;
    string ip_city_mapping;
    string sensitive_words_file;

    ServiceDataFile() {};

    ServiceDataFile(DOMNode* parent)
    {
        city_id_mapping = CXmlConfig::getValue(parent, "city_id_mapping");
        ip_city_mapping = CXmlConfig::getValue(parent, "ip_city_mapping");
        sensitive_words_file = CXmlConfig::getValue(parent, "sensitive_words_file");
    }

    void output() const
    {
        std::cout << "ServiceDataFile : city_id_mapping = " << city_id_mapping << endl;
        std::cout << "ServiceDataFile : ip_city_mapping = " << ip_city_mapping << endl;
        std::cout << "ServiceDataFile : sensitive_words_file = " << sensitive_words_file << endl;
    }
};

struct ServiceConfigFile
{
    string db_xml_cfg;
    string service_xml_cfg;
    string mall_xml_cfg;
    string cattles_base_cfg;
    string golden_fraud_base_cfg;
    string fight_landlord_base_cfg;
    string service_cfg_file;

    ServiceConfigFile() {};

    ServiceConfigFile(DOMNode* parent)
    {
        db_xml_cfg = CXmlConfig::getValue(parent, "db_xml_cfg");
        service_xml_cfg = CXmlConfig::getValue(parent, "service_xml_cfg");
        mall_xml_cfg = CXmlConfig::getValue(parent, "mall_xml_cfg");
        cattles_base_cfg = CXmlConfig::getValue(parent, "cattles_base_cfg");
        golden_fraud_base_cfg = CXmlConfig::getValue(parent, "golden_fraud_base_cfg");
        fight_landlord_base_cfg = CXmlConfig::getValue(parent, "fight_landlord_base_cfg");
        service_cfg_file = CXmlConfig::getValue(parent, "service_cfg_file");
    }

    void output() const
    {
        std::cout << "ServiceConfigFile : db_xml_cfg = " << db_xml_cfg << endl;
        std::cout << "ServiceConfigFile : service_xml_cfg = " << service_xml_cfg << endl;
        std::cout << "ServiceConfigFile : mall_xml_cfg = " << mall_xml_cfg << endl;
        std::cout << "ServiceConfigFile : cattles_base_cfg = " << cattles_base_cfg << endl;
        std::cout << "ServiceConfigFile : golden_fraud_base_cfg = " << golden_fraud_base_cfg << endl;
        std::cout << "ServiceConfigFile : fight_landlord_base_cfg = " << fight_landlord_base_cfg << endl;
        std::cout << "ServiceConfigFile : service_cfg_file = " << service_cfg_file << endl;
    }
};

struct CommonCfg : public IXmlConfigBase
{
    ServiceConfigFile config_file;
    ServiceDataFile data_file;

    static const CommonCfg& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static CommonCfg cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    CommonCfg() {};
    virtual ~CommonCfg() {};
    CommonCfg(const char* xmlConfigFile)
    {
        CXmlConfig::setConfigValue(xmlConfigFile, *this);
    }

    virtual void set(DOMNode* parent)
    {
        DomNodeArray _config_file;
        CXmlConfig::getNode(parent, "param", "config_file", _config_file);
        if (_config_file.size() > 0) config_file = ServiceConfigFile(_config_file[0]);
        
        DomNodeArray _data_file;
        CXmlConfig::getNode(parent, "param", "data_file", _data_file);
        if (_data_file.size() > 0) data_file = ServiceDataFile(_data_file[0]);
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- CommonCfg : config_file ----------" << endl;
        config_file.output();
        std::cout << "========== CommonCfg : config_file ==========\n" << endl;
        std::cout << "---------- CommonCfg : data_file ----------" << endl;
        data_file.output();
        std::cout << "========== CommonCfg : data_file ==========\n" << endl;
    }
};

}

#endif  // __NCOMMONCONFIG_H__

