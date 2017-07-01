#ifndef __SIMULATIONCLIENTCONFIGDATA_H__
#define __SIMULATIONCLIENTCONFIGDATA_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace SimulationClientConfigData
{

struct Content
{
    string contents;

    Content() {};

    Content(DOMNode* parent)
    {
        contents = CXmlConfig::getValue(parent, "contents");
    }

    void output() const
    {
        std::cout << "Content : contents = " << contents << endl;
    }
};

struct ChatCfg
{
    unsigned int rate_time_min_percent;
    unsigned int rate_time_max_percent;
    unsigned int chat_time_min;
    unsigned int chat_time_max;

    ChatCfg() {};

    ChatCfg(DOMNode* parent)
    {
        rate_time_min_percent = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rate_time_min_percent"));
        rate_time_max_percent = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rate_time_max_percent"));
        chat_time_min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "chat_time_min"));
        chat_time_max = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "chat_time_max"));
    }

    void output() const
    {
        std::cout << "ChatCfg : rate_time_min_percent = " << rate_time_min_percent << endl;
        std::cout << "ChatCfg : rate_time_max_percent = " << rate_time_max_percent << endl;
        std::cout << "ChatCfg : chat_time_min = " << chat_time_min << endl;
        std::cout << "ChatCfg : chat_time_max = " << chat_time_max << endl;
    }
};

struct PropUsage
{
    unsigned int rate_time_min_percent;
    unsigned int rate_time_max_percent;
    unsigned int use_prop_time_min;
    unsigned int use_prop_time_max;

    PropUsage() {};

    PropUsage(DOMNode* parent)
    {
        rate_time_min_percent = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rate_time_min_percent"));
        rate_time_max_percent = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rate_time_max_percent"));
        use_prop_time_min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "use_prop_time_min"));
        use_prop_time_max = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "use_prop_time_max"));
    }

    void output() const
    {
        std::cout << "PropUsage : rate_time_min_percent = " << rate_time_min_percent << endl;
        std::cout << "PropUsage : rate_time_max_percent = " << rate_time_max_percent << endl;
        std::cout << "PropUsage : use_prop_time_min = " << use_prop_time_min << endl;
        std::cout << "PropUsage : use_prop_time_max = " << use_prop_time_max << endl;
    }
};

struct BulletCfg
{
    unsigned int rate_add_percent;
    unsigned int rate_step_min;
    unsigned int rate_step_max;
    unsigned int rate_time_min_percent;
    unsigned int rate_time_max_percent;
    unsigned int change_rate_time_min;
    unsigned int change_rate_time_max;

    BulletCfg() {};

    BulletCfg(DOMNode* parent)
    {
        rate_add_percent = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rate_add_percent"));
        rate_step_min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rate_step_min"));
        rate_step_max = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rate_step_max"));
        rate_time_min_percent = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rate_time_min_percent"));
        rate_time_max_percent = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rate_time_max_percent"));
        change_rate_time_min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "change_rate_time_min"));
        change_rate_time_max = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "change_rate_time_max"));
    }

    void output() const
    {
        std::cout << "BulletCfg : rate_add_percent = " << rate_add_percent << endl;
        std::cout << "BulletCfg : rate_step_min = " << rate_step_min << endl;
        std::cout << "BulletCfg : rate_step_max = " << rate_step_max << endl;
        std::cout << "BulletCfg : rate_time_min_percent = " << rate_time_min_percent << endl;
        std::cout << "BulletCfg : rate_time_max_percent = " << rate_time_max_percent << endl;
        std::cout << "BulletCfg : change_rate_time_min = " << change_rate_time_min << endl;
        std::cout << "BulletCfg : change_rate_time_max = " << change_rate_time_max << endl;
    }
};

struct RobotProperty
{
    unsigned int type;
    unsigned int min;
    unsigned int max;

    RobotProperty() {};

    RobotProperty(DOMNode* parent)
    {
        type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "type"));
        min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min"));
        max = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max"));
    }

    void output() const
    {
        std::cout << "RobotProperty : type = " << type << endl;
        std::cout << "RobotProperty : min = " << min << endl;
        std::cout << "RobotProperty : max = " << max << endl;
    }
};

struct NetProxy
{
    string proxy_ip;
    unsigned int proxy_port;

    NetProxy() {};

    NetProxy(DOMNode* parent)
    {
        proxy_ip = CXmlConfig::getValue(parent, "proxy_ip");
        proxy_port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "proxy_port"));
    }

    void output() const
    {
        std::cout << "NetProxy : proxy_ip = " << proxy_ip << endl;
        std::cout << "NetProxy : proxy_port = " << proxy_port << endl;
    }
};

struct CommonCfg
{
    unsigned int robot_init_num;
    unsigned int robot_register_num;
    unsigned int robot_gold_min;
    unsigned int robot_gold_max;
    double gold_min_percent;
    double gold_max_percent;
    unsigned int vip_robot_init_num;
    unsigned int vip_robot_register_num;
    unsigned int robot_vip_min;
    unsigned int robot_vip_max;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        robot_init_num = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "robot_init_num"));
        robot_register_num = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "robot_register_num"));
        robot_gold_min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "robot_gold_min"));
        robot_gold_max = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "robot_gold_max"));
        gold_min_percent = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "gold_min_percent"));
        gold_max_percent = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "gold_max_percent"));
        vip_robot_init_num = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "vip_robot_init_num"));
        vip_robot_register_num = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "vip_robot_register_num"));
        robot_vip_min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "robot_vip_min"));
        robot_vip_max = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "robot_vip_max"));
    }

    void output() const
    {
        std::cout << "CommonCfg : robot_init_num = " << robot_init_num << endl;
        std::cout << "CommonCfg : robot_register_num = " << robot_register_num << endl;
        std::cout << "CommonCfg : robot_gold_min = " << robot_gold_min << endl;
        std::cout << "CommonCfg : robot_gold_max = " << robot_gold_max << endl;
        std::cout << "CommonCfg : gold_min_percent = " << gold_min_percent << endl;
        std::cout << "CommonCfg : gold_max_percent = " << gold_max_percent << endl;
        std::cout << "CommonCfg : vip_robot_init_num = " << vip_robot_init_num << endl;
        std::cout << "CommonCfg : vip_robot_register_num = " << vip_robot_register_num << endl;
        std::cout << "CommonCfg : robot_vip_min = " << robot_vip_min << endl;
        std::cout << "CommonCfg : robot_vip_max = " << robot_vip_max << endl;
    }
};

struct config : public IXmlConfigBase
{
    CommonCfg commonCfg;
    NetProxy netProxy;
    vector<RobotProperty> reset_property_list;
    BulletCfg bulletCfg;
    map<unsigned int, unsigned int> bullet_rate_level;
    PropUsage propUsage;
    ChatCfg chatCfg;
    vector<Content> chatContent;

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
        
        DomNodeArray _netProxy;
        CXmlConfig::getNode(parent, "param", "netProxy", _netProxy);
        if (_netProxy.size() > 0) netProxy = NetProxy(_netProxy[0]);
        
        reset_property_list.clear();
        DomNodeArray _reset_property_list;
        CXmlConfig::getNode(parent, _reset_property_list, "reset_property_list", "RobotProperty");
        for (unsigned int i = 0; i < _reset_property_list.size(); ++i)
        {
            reset_property_list.push_back(RobotProperty(_reset_property_list[i]));
        }
        
        DomNodeArray _bulletCfg;
        CXmlConfig::getNode(parent, "param", "bulletCfg", _bulletCfg);
        if (_bulletCfg.size() > 0) bulletCfg = BulletCfg(_bulletCfg[0]);
        
        bullet_rate_level.clear();
        DomNodeArray _bullet_rate_level;
        CXmlConfig::getNode(parent, _bullet_rate_level, "bullet_rate_level", "value", "unsigned int");
        for (unsigned int i = 0; i < _bullet_rate_level.size(); ++i)
        {
            bullet_rate_level[CXmlConfig::stringToInt(CXmlConfig::getKey(_bullet_rate_level[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_bullet_rate_level[i], "value"));
        }
        
        DomNodeArray _propUsage;
        CXmlConfig::getNode(parent, "param", "propUsage", _propUsage);
        if (_propUsage.size() > 0) propUsage = PropUsage(_propUsage[0]);
        
        DomNodeArray _chatCfg;
        CXmlConfig::getNode(parent, "param", "chatCfg", _chatCfg);
        if (_chatCfg.size() > 0) chatCfg = ChatCfg(_chatCfg[0]);
        
        chatContent.clear();
        DomNodeArray _chatContent;
        CXmlConfig::getNode(parent, _chatContent, "chatContent", "Content");
        for (unsigned int i = 0; i < _chatContent.size(); ++i)
        {
            chatContent.push_back(Content(_chatContent[i]));
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
        std::cout << "---------- config : netProxy ----------" << endl;
        netProxy.output();
        std::cout << "========== config : netProxy ==========\n" << endl;
        std::cout << "---------- config : reset_property_list ----------" << endl;
        for (unsigned int i = 0; i < reset_property_list.size(); ++i)
        {
            reset_property_list[i].output();
            if (i < (reset_property_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : reset_property_list ==========\n" << endl;
        std::cout << "---------- config : bulletCfg ----------" << endl;
        bulletCfg.output();
        std::cout << "========== config : bulletCfg ==========\n" << endl;
        std::cout << "---------- config : bullet_rate_level ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = bullet_rate_level.begin(); it != bullet_rate_level.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== config : bullet_rate_level ==========\n" << endl;
        std::cout << "---------- config : propUsage ----------" << endl;
        propUsage.output();
        std::cout << "========== config : propUsage ==========\n" << endl;
        std::cout << "---------- config : chatCfg ----------" << endl;
        chatCfg.output();
        std::cout << "========== config : chatCfg ==========\n" << endl;
        std::cout << "---------- config : chatContent ----------" << endl;
        for (unsigned int i = 0; i < chatContent.size(); ++i)
        {
            chatContent[i].output();
            if (i < (chatContent.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : chatContent ==========\n" << endl;
    }
};

}

#endif  // __SIMULATIONCLIENTCONFIGDATA_H__

