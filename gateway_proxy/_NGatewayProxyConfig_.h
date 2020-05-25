#ifndef __NGATEWAYPROXYCONFIG_H__
#define __NGATEWAYPROXYCONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace NGatewayProxyConfig
{

struct CommonCfg
{
    unsigned int save_data_interval;
    unsigned int webserver_heartbeat_log_interval;
    unsigned int need_encrypt_data;
    unsigned int send_pkg_size;
    unsigned int receive_service_pkg_interval;
    unsigned int receive_service_pkg_count;
    unsigned int game_hall_id;
    vector<unsigned int> filter_hall_service_list;
    vector<unsigned int> allow_service_type;
    unsigned int max_pkg_size;
    unordered_map<unsigned int, unsigned int> other_pkg_size;
    unsigned int receive_interval_milliseconds;
    unsigned int receive_interval_count;
    unordered_map<unsigned int, unsigned int> other_pkg_count;
    vector<unsigned int> hall_first_message_protocol;
    vector<unsigned int> game_first_message_protocol;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        save_data_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "save_data_interval"));
        webserver_heartbeat_log_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "webserver_heartbeat_log_interval"));
        need_encrypt_data = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "need_encrypt_data"));
        send_pkg_size = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "send_pkg_size"));
        receive_service_pkg_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "receive_service_pkg_interval"));
        receive_service_pkg_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "receive_service_pkg_count"));
        game_hall_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "game_hall_id"));
        
        filter_hall_service_list.clear();
        DomNodeArray _filter_hall_service_list;
        CXmlConfig::getNode(parent, _filter_hall_service_list, "filter_hall_service_list", "value", "unsigned int");
        for (unsigned int i = 0; i < _filter_hall_service_list.size(); ++i)
        {
            filter_hall_service_list.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_filter_hall_service_list[i], "value")));
        }
        
        allow_service_type.clear();
        DomNodeArray _allow_service_type;
        CXmlConfig::getNode(parent, _allow_service_type, "allow_service_type", "value", "unsigned int");
        for (unsigned int i = 0; i < _allow_service_type.size(); ++i)
        {
            allow_service_type.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_allow_service_type[i], "value")));
        }
        max_pkg_size = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_pkg_size"));
        
        other_pkg_size.clear();
        DomNodeArray _other_pkg_size;
        CXmlConfig::getNode(parent, _other_pkg_size, "other_pkg_size", "value", "unsigned int");
        for (unsigned int i = 0; i < _other_pkg_size.size(); ++i)
        {
            other_pkg_size[CXmlConfig::stringToInt(CXmlConfig::getKey(_other_pkg_size[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_other_pkg_size[i], "value"));
        }
        receive_interval_milliseconds = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "receive_interval_milliseconds"));
        receive_interval_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "receive_interval_count"));
        
        other_pkg_count.clear();
        DomNodeArray _other_pkg_count;
        CXmlConfig::getNode(parent, _other_pkg_count, "other_pkg_count", "value", "unsigned int");
        for (unsigned int i = 0; i < _other_pkg_count.size(); ++i)
        {
            other_pkg_count[CXmlConfig::stringToInt(CXmlConfig::getKey(_other_pkg_count[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_other_pkg_count[i], "value"));
        }
        
        hall_first_message_protocol.clear();
        DomNodeArray _hall_first_message_protocol;
        CXmlConfig::getNode(parent, _hall_first_message_protocol, "hall_first_message_protocol", "value", "unsigned int");
        for (unsigned int i = 0; i < _hall_first_message_protocol.size(); ++i)
        {
            hall_first_message_protocol.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_hall_first_message_protocol[i], "value")));
        }
        
        game_first_message_protocol.clear();
        DomNodeArray _game_first_message_protocol;
        CXmlConfig::getNode(parent, _game_first_message_protocol, "game_first_message_protocol", "value", "unsigned int");
        for (unsigned int i = 0; i < _game_first_message_protocol.size(); ++i)
        {
            game_first_message_protocol.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_game_first_message_protocol[i], "value")));
        }
    }

    void output() const
    {
        std::cout << "CommonCfg : save_data_interval = " << save_data_interval << endl;
        std::cout << "CommonCfg : webserver_heartbeat_log_interval = " << webserver_heartbeat_log_interval << endl;
        std::cout << "CommonCfg : need_encrypt_data = " << need_encrypt_data << endl;
        std::cout << "CommonCfg : send_pkg_size = " << send_pkg_size << endl;
        std::cout << "CommonCfg : receive_service_pkg_interval = " << receive_service_pkg_interval << endl;
        std::cout << "CommonCfg : receive_service_pkg_count = " << receive_service_pkg_count << endl;
        std::cout << "CommonCfg : game_hall_id = " << game_hall_id << endl;
        std::cout << "---------- CommonCfg : filter_hall_service_list ----------" << endl;
        for (unsigned int i = 0; i < filter_hall_service_list.size(); ++i)
        {
            std::cout << "value = " << filter_hall_service_list[i] << endl;
        }
        std::cout << "========== CommonCfg : filter_hall_service_list ==========\n" << endl;
        std::cout << "---------- CommonCfg : allow_service_type ----------" << endl;
        for (unsigned int i = 0; i < allow_service_type.size(); ++i)
        {
            std::cout << "value = " << allow_service_type[i] << endl;
        }
        std::cout << "========== CommonCfg : allow_service_type ==========\n" << endl;
        std::cout << "CommonCfg : max_pkg_size = " << max_pkg_size << endl;
        std::cout << "---------- CommonCfg : other_pkg_size ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = other_pkg_size.begin(); it != other_pkg_size.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== CommonCfg : other_pkg_size ==========\n" << endl;
        std::cout << "CommonCfg : receive_interval_milliseconds = " << receive_interval_milliseconds << endl;
        std::cout << "CommonCfg : receive_interval_count = " << receive_interval_count << endl;
        std::cout << "---------- CommonCfg : other_pkg_count ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = other_pkg_count.begin(); it != other_pkg_count.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== CommonCfg : other_pkg_count ==========\n" << endl;
        std::cout << "---------- CommonCfg : hall_first_message_protocol ----------" << endl;
        for (unsigned int i = 0; i < hall_first_message_protocol.size(); ++i)
        {
            std::cout << "value = " << hall_first_message_protocol[i] << endl;
        }
        std::cout << "========== CommonCfg : hall_first_message_protocol ==========\n" << endl;
        std::cout << "---------- CommonCfg : game_first_message_protocol ----------" << endl;
        for (unsigned int i = 0; i < game_first_message_protocol.size(); ++i)
        {
            std::cout << "value = " << game_first_message_protocol[i] << endl;
        }
        std::cout << "========== CommonCfg : game_first_message_protocol ==========\n" << endl;
    }
};

struct GatewayConfig : public IXmlConfigBase
{
    CommonCfg commonCfg;

    static const GatewayConfig& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static GatewayConfig cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    GatewayConfig() {};
    virtual ~GatewayConfig() {};
    GatewayConfig(const char* xmlConfigFile)
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
        std::cout << "---------- GatewayConfig : commonCfg ----------" << endl;
        commonCfg.output();
        std::cout << "========== GatewayConfig : commonCfg ==========\n" << endl;
    }
};

}

#endif  // __NGATEWAYPROXYCONFIG_H__

