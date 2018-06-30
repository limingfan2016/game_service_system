#ifndef __MALLCONFIGDATA_H__
#define __MALLCONFIGDATA_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace MallConfigData
{

struct MallGoodsCfg
{
    unsigned int type;
    double price;

    MallGoodsCfg() {};

    MallGoodsCfg(DOMNode* parent)
    {
        type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "type"));
        price = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "price"));
    }

    void output() const
    {
        std::cout << "MallGoodsCfg : type = " << type << endl;
        std::cout << "MallGoodsCfg : price = " << price << endl;
    }
};

struct MoneyItem
{
    unsigned int num;
    double price;

    MoneyItem() {};

    MoneyItem(DOMNode* parent)
    {
        num = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "num"));
        price = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "price"));
    }

    void output() const
    {
        std::cout << "MoneyItem : num = " << num << endl;
        std::cout << "MoneyItem : price = " << price << endl;
    }
};

struct CommonCfg
{
    double room_card_to_gold_ratio;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        room_card_to_gold_ratio = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "room_card_to_gold_ratio"));
    }

    void output() const
    {
        std::cout << "CommonCfg : room_card_to_gold_ratio = " << room_card_to_gold_ratio << endl;
    }
};

struct MallData : public IXmlConfigBase
{
    CommonCfg common_cfg;
    vector<unsigned int> gold_mall_list;
    map<unsigned int, MoneyItem> money_map;
    vector<MallGoodsCfg> mall_good_array;

    static const MallData& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static MallData cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    MallData() {};
    virtual ~MallData() {};
    MallData(const char* xmlConfigFile)
    {
        CXmlConfig::setConfigValue(xmlConfigFile, *this);
    }

    virtual void set(DOMNode* parent)
    {
        DomNodeArray _common_cfg;
        CXmlConfig::getNode(parent, "param", "common_cfg", _common_cfg);
        if (_common_cfg.size() > 0) common_cfg = CommonCfg(_common_cfg[0]);
        
        gold_mall_list.clear();
        DomNodeArray _gold_mall_list;
        CXmlConfig::getNode(parent, _gold_mall_list, "gold_mall_list", "value", "unsigned int");
        for (unsigned int i = 0; i < _gold_mall_list.size(); ++i)
        {
            gold_mall_list.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_gold_mall_list[i], "value")));
        }
        
        money_map.clear();
        DomNodeArray _money_map;
        CXmlConfig::getNode(parent, _money_map, "money_map", "MoneyItem");
        for (unsigned int i = 0; i < _money_map.size(); ++i)
        {
            money_map[CXmlConfig::stringToInt(CXmlConfig::getKey(_money_map[i], "key"))] = MoneyItem(_money_map[i]);
        }
        
        mall_good_array.clear();
        DomNodeArray _mall_good_array;
        CXmlConfig::getNode(parent, _mall_good_array, "mall_good_array", "MallGoodsCfg");
        for (unsigned int i = 0; i < _mall_good_array.size(); ++i)
        {
            mall_good_array.push_back(MallGoodsCfg(_mall_good_array[i]));
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- MallData : common_cfg ----------" << endl;
        common_cfg.output();
        std::cout << "========== MallData : common_cfg ==========\n" << endl;
        std::cout << "---------- MallData : gold_mall_list ----------" << endl;
        for (unsigned int i = 0; i < gold_mall_list.size(); ++i)
        {
            std::cout << "value = " << gold_mall_list[i] << endl;
        }
        std::cout << "========== MallData : gold_mall_list ==========\n" << endl;
        std::cout << "---------- MallData : money_map ----------" << endl;
        unsigned int _money_map_count_ = 0;
        for (map<unsigned int, MoneyItem>::const_iterator it = money_map.begin(); it != money_map.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_money_map_count_++ < (money_map.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : money_map ==========\n" << endl;
        std::cout << "---------- MallData : mall_good_array ----------" << endl;
        for (unsigned int i = 0; i < mall_good_array.size(); ++i)
        {
            mall_good_array[i].output();
            if (i < (mall_good_array.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : mall_good_array ==========\n" << endl;
    }
};

}

#endif  // __MALLCONFIGDATA_H__

