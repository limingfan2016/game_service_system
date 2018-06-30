#ifndef __NCATTLESBASECONFIG_H__
#define __NCATTLESBASECONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace NCattlesBaseConfig
{

struct RateRuleCfg
{
    map<int, unsigned int> cattles_value_rate;

    RateRuleCfg() {};

    RateRuleCfg(DOMNode* parent)
    {
        cattles_value_rate.clear();
        DomNodeArray _cattles_value_rate;
        CXmlConfig::getNode(parent, _cattles_value_rate, "cattles_value_rate", "value", "unsigned int");
        for (unsigned int i = 0; i < _cattles_value_rate.size(); ++i)
        {
            cattles_value_rate[CXmlConfig::stringToInt(CXmlConfig::getKey(_cattles_value_rate[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_cattles_value_rate[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "---------- RateRuleCfg : cattles_value_rate ----------" << endl;
        for (map<int, unsigned int>::const_iterator it = cattles_value_rate.begin(); it != cattles_value_rate.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== RateRuleCfg : cattles_value_rate ==========\n" << endl;
    }
};

struct BaseNumberCfg
{
    unsigned int max_base_bet_value;
    vector<unsigned int> bet_rate;

    BaseNumberCfg() {};

    BaseNumberCfg(DOMNode* parent)
    {
        max_base_bet_value = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_base_bet_value"));
        
        bet_rate.clear();
        DomNodeArray _bet_rate;
        CXmlConfig::getNode(parent, _bet_rate, "bet_rate", "value", "unsigned int");
        for (unsigned int i = 0; i < _bet_rate.size(); ++i)
        {
            bet_rate.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_bet_rate[i], "value")));
        }
    }

    void output() const
    {
        std::cout << "BaseNumberCfg : max_base_bet_value = " << max_base_bet_value << endl;
        std::cout << "---------- BaseNumberCfg : bet_rate ----------" << endl;
        for (unsigned int i = 0; i < bet_rate.size(); ++i)
        {
            std::cout << "value = " << bet_rate[i] << endl;
        }
        std::cout << "========== BaseNumberCfg : bet_rate ==========\n" << endl;
    }
};

struct WinnerNotice
{
    string notice_msg;
    unsigned int notice_condition;
    vector<unsigned int> object_colour;

    WinnerNotice() {};

    WinnerNotice(DOMNode* parent)
    {
        notice_msg = CXmlConfig::getValue(parent, "notice_msg");
        notice_condition = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "notice_condition"));
        
        object_colour.clear();
        DomNodeArray _object_colour;
        CXmlConfig::getNode(parent, _object_colour, "object_colour", "value", "unsigned int");
        for (unsigned int i = 0; i < _object_colour.size(); ++i)
        {
            object_colour.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_object_colour[i], "value")));
        }
    }

    void output() const
    {
        std::cout << "WinnerNotice : notice_msg = " << notice_msg << endl;
        std::cout << "WinnerNotice : notice_condition = " << notice_condition << endl;
        std::cout << "---------- WinnerNotice : object_colour ----------" << endl;
        for (unsigned int i = 0; i < object_colour.size(); ++i)
        {
            std::cout << "value = " << object_colour[i] << endl;
        }
        std::cout << "========== WinnerNotice : object_colour ==========\n" << endl;
    }
};

struct RoomCardCfg
{
    double creator_pay_count;
    double average_pay_count;

    RoomCardCfg() {};

    RoomCardCfg(DOMNode* parent)
    {
        creator_pay_count = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "creator_pay_count"));
        average_pay_count = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "average_pay_count"));
    }

    void output() const
    {
        std::cout << "RoomCardCfg : creator_pay_count = " << creator_pay_count << endl;
        std::cout << "RoomCardCfg : average_pay_count = " << average_pay_count << endl;
    }
};

struct CommonCfg
{
    unsigned int start_game_secs;
    unsigned int prepare_game_secs;
    unsigned int compete_banker_secs;
    unsigned int confirm_banker_secs;
    unsigned int choice_bet_secs;
    unsigned int open_card_secs;
    double create_room_need_gold_multiplier;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        start_game_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "start_game_secs"));
        prepare_game_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "prepare_game_secs"));
        compete_banker_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "compete_banker_secs"));
        confirm_banker_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "confirm_banker_secs"));
        choice_bet_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "choice_bet_secs"));
        open_card_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "open_card_secs"));
        create_room_need_gold_multiplier = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "create_room_need_gold_multiplier"));
    }

    void output() const
    {
        std::cout << "CommonCfg : start_game_secs = " << start_game_secs << endl;
        std::cout << "CommonCfg : prepare_game_secs = " << prepare_game_secs << endl;
        std::cout << "CommonCfg : compete_banker_secs = " << compete_banker_secs << endl;
        std::cout << "CommonCfg : confirm_banker_secs = " << confirm_banker_secs << endl;
        std::cout << "CommonCfg : choice_bet_secs = " << choice_bet_secs << endl;
        std::cout << "CommonCfg : open_card_secs = " << open_card_secs << endl;
        std::cout << "CommonCfg : create_room_need_gold_multiplier = " << create_room_need_gold_multiplier << endl;
    }
};

struct CattlesBaseConfig : public IXmlConfigBase
{
    map<int, string> card_type_name;
    CommonCfg common_cfg;
    RoomCardCfg room_card_cfg;
    vector<unsigned int> room_play_times;
    WinnerNotice winner_notice;
    BaseNumberCfg gold_base_number_cfg;
    BaseNumberCfg card_base_number_cfg;
    vector<RateRuleCfg> rate_rule_cfg;
    vector<unsigned int> player_count_cfg;
    map<int, unsigned int> special_card_rate;

    static const CattlesBaseConfig& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static CattlesBaseConfig cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    CattlesBaseConfig() {};
    virtual ~CattlesBaseConfig() {};
    CattlesBaseConfig(const char* xmlConfigFile)
    {
        CXmlConfig::setConfigValue(xmlConfigFile, *this);
    }

    virtual void set(DOMNode* parent)
    {
        card_type_name.clear();
        DomNodeArray _card_type_name;
        CXmlConfig::getNode(parent, _card_type_name, "card_type_name", "value", "string");
        for (unsigned int i = 0; i < _card_type_name.size(); ++i)
        {
            card_type_name[CXmlConfig::stringToInt(CXmlConfig::getKey(_card_type_name[i], "key"))] = CXmlConfig::getValue(_card_type_name[i], "value");
        }
        
        DomNodeArray _common_cfg;
        CXmlConfig::getNode(parent, "param", "common_cfg", _common_cfg);
        if (_common_cfg.size() > 0) common_cfg = CommonCfg(_common_cfg[0]);
        
        DomNodeArray _room_card_cfg;
        CXmlConfig::getNode(parent, "param", "room_card_cfg", _room_card_cfg);
        if (_room_card_cfg.size() > 0) room_card_cfg = RoomCardCfg(_room_card_cfg[0]);
        
        room_play_times.clear();
        DomNodeArray _room_play_times;
        CXmlConfig::getNode(parent, _room_play_times, "room_play_times", "value", "unsigned int");
        for (unsigned int i = 0; i < _room_play_times.size(); ++i)
        {
            room_play_times.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_room_play_times[i], "value")));
        }
        
        DomNodeArray _winner_notice;
        CXmlConfig::getNode(parent, "param", "winner_notice", _winner_notice);
        if (_winner_notice.size() > 0) winner_notice = WinnerNotice(_winner_notice[0]);
        
        DomNodeArray _gold_base_number_cfg;
        CXmlConfig::getNode(parent, "param", "gold_base_number_cfg", _gold_base_number_cfg);
        if (_gold_base_number_cfg.size() > 0) gold_base_number_cfg = BaseNumberCfg(_gold_base_number_cfg[0]);
        
        DomNodeArray _card_base_number_cfg;
        CXmlConfig::getNode(parent, "param", "card_base_number_cfg", _card_base_number_cfg);
        if (_card_base_number_cfg.size() > 0) card_base_number_cfg = BaseNumberCfg(_card_base_number_cfg[0]);
        
        rate_rule_cfg.clear();
        DomNodeArray _rate_rule_cfg;
        CXmlConfig::getNode(parent, _rate_rule_cfg, "rate_rule_cfg", "RateRuleCfg");
        for (unsigned int i = 0; i < _rate_rule_cfg.size(); ++i)
        {
            rate_rule_cfg.push_back(RateRuleCfg(_rate_rule_cfg[i]));
        }
        
        player_count_cfg.clear();
        DomNodeArray _player_count_cfg;
        CXmlConfig::getNode(parent, _player_count_cfg, "player_count_cfg", "value", "unsigned int");
        for (unsigned int i = 0; i < _player_count_cfg.size(); ++i)
        {
            player_count_cfg.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_player_count_cfg[i], "value")));
        }
        
        special_card_rate.clear();
        DomNodeArray _special_card_rate;
        CXmlConfig::getNode(parent, _special_card_rate, "special_card_rate", "value", "unsigned int");
        for (unsigned int i = 0; i < _special_card_rate.size(); ++i)
        {
            special_card_rate[CXmlConfig::stringToInt(CXmlConfig::getKey(_special_card_rate[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_special_card_rate[i], "value"));
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- CattlesBaseConfig : card_type_name ----------" << endl;
        for (map<int, string>::const_iterator it = card_type_name.begin(); it != card_type_name.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== CattlesBaseConfig : card_type_name ==========\n" << endl;
        std::cout << "---------- CattlesBaseConfig : common_cfg ----------" << endl;
        common_cfg.output();
        std::cout << "========== CattlesBaseConfig : common_cfg ==========\n" << endl;
        std::cout << "---------- CattlesBaseConfig : room_card_cfg ----------" << endl;
        room_card_cfg.output();
        std::cout << "========== CattlesBaseConfig : room_card_cfg ==========\n" << endl;
        std::cout << "---------- CattlesBaseConfig : room_play_times ----------" << endl;
        for (unsigned int i = 0; i < room_play_times.size(); ++i)
        {
            std::cout << "value = " << room_play_times[i] << endl;
        }
        std::cout << "========== CattlesBaseConfig : room_play_times ==========\n" << endl;
        std::cout << "---------- CattlesBaseConfig : winner_notice ----------" << endl;
        winner_notice.output();
        std::cout << "========== CattlesBaseConfig : winner_notice ==========\n" << endl;
        std::cout << "---------- CattlesBaseConfig : gold_base_number_cfg ----------" << endl;
        gold_base_number_cfg.output();
        std::cout << "========== CattlesBaseConfig : gold_base_number_cfg ==========\n" << endl;
        std::cout << "---------- CattlesBaseConfig : card_base_number_cfg ----------" << endl;
        card_base_number_cfg.output();
        std::cout << "========== CattlesBaseConfig : card_base_number_cfg ==========\n" << endl;
        std::cout << "---------- CattlesBaseConfig : rate_rule_cfg ----------" << endl;
        for (unsigned int i = 0; i < rate_rule_cfg.size(); ++i)
        {
            rate_rule_cfg[i].output();
            if (i < (rate_rule_cfg.size() - 1)) std::cout << endl;
        }
        std::cout << "========== CattlesBaseConfig : rate_rule_cfg ==========\n" << endl;
        std::cout << "---------- CattlesBaseConfig : player_count_cfg ----------" << endl;
        for (unsigned int i = 0; i < player_count_cfg.size(); ++i)
        {
            std::cout << "value = " << player_count_cfg[i] << endl;
        }
        std::cout << "========== CattlesBaseConfig : player_count_cfg ==========\n" << endl;
        std::cout << "---------- CattlesBaseConfig : special_card_rate ----------" << endl;
        for (map<int, unsigned int>::const_iterator it = special_card_rate.begin(); it != special_card_rate.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== CattlesBaseConfig : special_card_rate ==========\n" << endl;
    }
};

}

#endif  // __NCATTLESBASECONFIG_H__

