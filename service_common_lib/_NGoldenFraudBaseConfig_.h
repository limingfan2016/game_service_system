#ifndef __NGOLDENFRAUDBASECONFIG_H__
#define __NGOLDENFRAUDBASECONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace NGoldenFraudBaseConfig
{

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
    unsigned int play_game_secs;
    unsigned int compare_card_secs;
    unsigned int prepare_game_secs;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        play_game_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "play_game_secs"));
        compare_card_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "compare_card_secs"));
        prepare_game_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "prepare_game_secs"));
    }

    void output() const
    {
        std::cout << "CommonCfg : play_game_secs = " << play_game_secs << endl;
        std::cout << "CommonCfg : compare_card_secs = " << compare_card_secs << endl;
        std::cout << "CommonCfg : prepare_game_secs = " << prepare_game_secs << endl;
    }
};

struct GoldenFraudBaseConfig : public IXmlConfigBase
{
    map<int, string> card_type_name;
    CommonCfg common_cfg;
    vector<unsigned int> play_round_times;
    RoomCardCfg room_card_cfg;
    vector<unsigned int> room_play_times;
    vector<unsigned int> gold_bet_rate;
    vector<unsigned int> card_bet_rate;
    vector<unsigned int> player_count_cfg;
    WinnerNotice winner_notice;

    static const GoldenFraudBaseConfig& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static GoldenFraudBaseConfig cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    GoldenFraudBaseConfig() {};
    virtual ~GoldenFraudBaseConfig() {};
    GoldenFraudBaseConfig(const char* xmlConfigFile)
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
        
        play_round_times.clear();
        DomNodeArray _play_round_times;
        CXmlConfig::getNode(parent, _play_round_times, "play_round_times", "value", "unsigned int");
        for (unsigned int i = 0; i < _play_round_times.size(); ++i)
        {
            play_round_times.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_play_round_times[i], "value")));
        }
        
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
        
        gold_bet_rate.clear();
        DomNodeArray _gold_bet_rate;
        CXmlConfig::getNode(parent, _gold_bet_rate, "gold_bet_rate", "value", "unsigned int");
        for (unsigned int i = 0; i < _gold_bet_rate.size(); ++i)
        {
            gold_bet_rate.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_gold_bet_rate[i], "value")));
        }
        
        card_bet_rate.clear();
        DomNodeArray _card_bet_rate;
        CXmlConfig::getNode(parent, _card_bet_rate, "card_bet_rate", "value", "unsigned int");
        for (unsigned int i = 0; i < _card_bet_rate.size(); ++i)
        {
            card_bet_rate.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_card_bet_rate[i], "value")));
        }
        
        player_count_cfg.clear();
        DomNodeArray _player_count_cfg;
        CXmlConfig::getNode(parent, _player_count_cfg, "player_count_cfg", "value", "unsigned int");
        for (unsigned int i = 0; i < _player_count_cfg.size(); ++i)
        {
            player_count_cfg.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_player_count_cfg[i], "value")));
        }
        
        DomNodeArray _winner_notice;
        CXmlConfig::getNode(parent, "param", "winner_notice", _winner_notice);
        if (_winner_notice.size() > 0) winner_notice = WinnerNotice(_winner_notice[0]);
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- GoldenFraudBaseConfig : card_type_name ----------" << endl;
        for (map<int, string>::const_iterator it = card_type_name.begin(); it != card_type_name.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== GoldenFraudBaseConfig : card_type_name ==========\n" << endl;
        std::cout << "---------- GoldenFraudBaseConfig : common_cfg ----------" << endl;
        common_cfg.output();
        std::cout << "========== GoldenFraudBaseConfig : common_cfg ==========\n" << endl;
        std::cout << "---------- GoldenFraudBaseConfig : play_round_times ----------" << endl;
        for (unsigned int i = 0; i < play_round_times.size(); ++i)
        {
            std::cout << "value = " << play_round_times[i] << endl;
        }
        std::cout << "========== GoldenFraudBaseConfig : play_round_times ==========\n" << endl;
        std::cout << "---------- GoldenFraudBaseConfig : room_card_cfg ----------" << endl;
        room_card_cfg.output();
        std::cout << "========== GoldenFraudBaseConfig : room_card_cfg ==========\n" << endl;
        std::cout << "---------- GoldenFraudBaseConfig : room_play_times ----------" << endl;
        for (unsigned int i = 0; i < room_play_times.size(); ++i)
        {
            std::cout << "value = " << room_play_times[i] << endl;
        }
        std::cout << "========== GoldenFraudBaseConfig : room_play_times ==========\n" << endl;
        std::cout << "---------- GoldenFraudBaseConfig : gold_bet_rate ----------" << endl;
        for (unsigned int i = 0; i < gold_bet_rate.size(); ++i)
        {
            std::cout << "value = " << gold_bet_rate[i] << endl;
        }
        std::cout << "========== GoldenFraudBaseConfig : gold_bet_rate ==========\n" << endl;
        std::cout << "---------- GoldenFraudBaseConfig : card_bet_rate ----------" << endl;
        for (unsigned int i = 0; i < card_bet_rate.size(); ++i)
        {
            std::cout << "value = " << card_bet_rate[i] << endl;
        }
        std::cout << "========== GoldenFraudBaseConfig : card_bet_rate ==========\n" << endl;
        std::cout << "---------- GoldenFraudBaseConfig : player_count_cfg ----------" << endl;
        for (unsigned int i = 0; i < player_count_cfg.size(); ++i)
        {
            std::cout << "value = " << player_count_cfg[i] << endl;
        }
        std::cout << "========== GoldenFraudBaseConfig : player_count_cfg ==========\n" << endl;
        std::cout << "---------- GoldenFraudBaseConfig : winner_notice ----------" << endl;
        winner_notice.output();
        std::cout << "========== GoldenFraudBaseConfig : winner_notice ==========\n" << endl;
    }
};

}

#endif  // __NGOLDENFRAUDBASECONFIG_H__

