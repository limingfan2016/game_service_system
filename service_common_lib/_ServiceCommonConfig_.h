#ifndef __SERVICECOMMONCONFIG_H__
#define __SERVICECOMMONCONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace ServiceCommonConfig
{

struct GameInfo
{
    string name;
    string desc;
    string icon;
    unsigned int room_max_player;
    double service_tax_ratio;

    GameInfo() {};

    GameInfo(DOMNode* parent)
    {
        name = CXmlConfig::getValue(parent, "name");
        desc = CXmlConfig::getValue(parent, "desc");
        icon = CXmlConfig::getValue(parent, "icon");
        room_max_player = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "room_max_player"));
        service_tax_ratio = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "service_tax_ratio"));
    }

    void output() const
    {
        std::cout << "GameInfo : name = " << name << endl;
        std::cout << "GameInfo : desc = " << desc << endl;
        std::cout << "GameInfo : icon = " << icon << endl;
        std::cout << "GameInfo : room_max_player = " << room_max_player << endl;
        std::cout << "GameInfo : service_tax_ratio = " << service_tax_ratio << endl;
    }
};

struct GameConfig
{
    unsigned int get_invitation_count;
    unsigned int last_player_count;
    unsigned int invitation_wait_secs;
    unsigned int check_disband_room_interval;
    unsigned int disband_room_wait_secs;
    unsigned int ask_dismiss_room_interval;
    unsigned int dismiss_room_secs;

    GameConfig() {};

    GameConfig(DOMNode* parent)
    {
        get_invitation_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "get_invitation_count"));
        last_player_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "last_player_count"));
        invitation_wait_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "invitation_wait_secs"));
        check_disband_room_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "check_disband_room_interval"));
        disband_room_wait_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "disband_room_wait_secs"));
        ask_dismiss_room_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "ask_dismiss_room_interval"));
        dismiss_room_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "dismiss_room_secs"));
    }

    void output() const
    {
        std::cout << "GameConfig : get_invitation_count = " << get_invitation_count << endl;
        std::cout << "GameConfig : last_player_count = " << last_player_count << endl;
        std::cout << "GameConfig : invitation_wait_secs = " << invitation_wait_secs << endl;
        std::cout << "GameConfig : check_disband_room_interval = " << check_disband_room_interval << endl;
        std::cout << "GameConfig : disband_room_wait_secs = " << disband_room_wait_secs << endl;
        std::cout << "GameConfig : ask_dismiss_room_interval = " << ask_dismiss_room_interval << endl;
        std::cout << "GameConfig : dismiss_room_secs = " << dismiss_room_secs << endl;
    }
};

struct ChessHallConfig
{
    unsigned int max_no_finish_room_count;
    unsigned int get_hall_player_count;
    unsigned int max_ranking;
    unsigned int player_get_mall_record_count;
    unsigned int get_game_record_count;

    ChessHallConfig() {};

    ChessHallConfig(DOMNode* parent)
    {
        max_no_finish_room_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_no_finish_room_count"));
        get_hall_player_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "get_hall_player_count"));
        max_ranking = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_ranking"));
        player_get_mall_record_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "player_get_mall_record_count"));
        get_game_record_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "get_game_record_count"));
    }

    void output() const
    {
        std::cout << "ChessHallConfig : max_no_finish_room_count = " << max_no_finish_room_count << endl;
        std::cout << "ChessHallConfig : get_hall_player_count = " << get_hall_player_count << endl;
        std::cout << "ChessHallConfig : max_ranking = " << max_ranking << endl;
        std::cout << "ChessHallConfig : player_get_mall_record_count = " << player_get_mall_record_count << endl;
        std::cout << "ChessHallConfig : get_game_record_count = " << get_game_record_count << endl;
    }
};

struct PlayerChatCfg
{
    unsigned int chat_content_length;
    unsigned int interval_secs;
    unsigned int chat_count;

    PlayerChatCfg() {};

    PlayerChatCfg(DOMNode* parent)
    {
        chat_content_length = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "chat_content_length"));
        interval_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "interval_secs"));
        chat_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "chat_count"));
    }

    void output() const
    {
        std::cout << "PlayerChatCfg : chat_content_length = " << chat_content_length << endl;
        std::cout << "PlayerChatCfg : interval_secs = " << interval_secs << endl;
        std::cout << "PlayerChatCfg : chat_count = " << chat_count << endl;
    }
};

struct HeartbeatCfg
{
    unsigned int heart_beat_interval;
    unsigned int heart_beat_count;

    HeartbeatCfg() {};

    HeartbeatCfg(DOMNode* parent)
    {
        heart_beat_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "heart_beat_interval"));
        heart_beat_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "heart_beat_count"));
    }

    void output() const
    {
        std::cout << "HeartbeatCfg : heart_beat_interval = " << heart_beat_interval << endl;
        std::cout << "HeartbeatCfg : heart_beat_count = " << heart_beat_count << endl;
    }
};

struct SrvCommonCfg : public IXmlConfigBase
{
    HeartbeatCfg heart_beat_cfg;
    PlayerChatCfg player_chat_cfg;
    ChessHallConfig chess_hall_cfg;
    GameConfig game_cfg;
    map<unsigned int, GameInfo> game_info_map;

    static const SrvCommonCfg& getConfigValue(const char* xmlConfigFile = NULL, bool isReset = false)
    {
        static SrvCommonCfg cfgValue(xmlConfigFile);
        if (isReset) CXmlConfig::setConfigValue(xmlConfigFile, cfgValue);
        return cfgValue;
    }

    SrvCommonCfg() {};
    virtual ~SrvCommonCfg() {};
    SrvCommonCfg(const char* xmlConfigFile)
    {
        CXmlConfig::setConfigValue(xmlConfigFile, *this);
    }

    virtual void set(DOMNode* parent)
    {
        DomNodeArray _heart_beat_cfg;
        CXmlConfig::getNode(parent, "param", "heart_beat_cfg", _heart_beat_cfg);
        if (_heart_beat_cfg.size() > 0) heart_beat_cfg = HeartbeatCfg(_heart_beat_cfg[0]);
        
        DomNodeArray _player_chat_cfg;
        CXmlConfig::getNode(parent, "param", "player_chat_cfg", _player_chat_cfg);
        if (_player_chat_cfg.size() > 0) player_chat_cfg = PlayerChatCfg(_player_chat_cfg[0]);
        
        DomNodeArray _chess_hall_cfg;
        CXmlConfig::getNode(parent, "param", "chess_hall_cfg", _chess_hall_cfg);
        if (_chess_hall_cfg.size() > 0) chess_hall_cfg = ChessHallConfig(_chess_hall_cfg[0]);
        
        DomNodeArray _game_cfg;
        CXmlConfig::getNode(parent, "param", "game_cfg", _game_cfg);
        if (_game_cfg.size() > 0) game_cfg = GameConfig(_game_cfg[0]);
        
        game_info_map.clear();
        DomNodeArray _game_info_map;
        CXmlConfig::getNode(parent, _game_info_map, "game_info_map", "GameInfo");
        for (unsigned int i = 0; i < _game_info_map.size(); ++i)
        {
            game_info_map[CXmlConfig::stringToInt(CXmlConfig::getKey(_game_info_map[i], "key"))] = GameInfo(_game_info_map[i]);
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- SrvCommonCfg : heart_beat_cfg ----------" << endl;
        heart_beat_cfg.output();
        std::cout << "========== SrvCommonCfg : heart_beat_cfg ==========\n" << endl;
        std::cout << "---------- SrvCommonCfg : player_chat_cfg ----------" << endl;
        player_chat_cfg.output();
        std::cout << "========== SrvCommonCfg : player_chat_cfg ==========\n" << endl;
        std::cout << "---------- SrvCommonCfg : chess_hall_cfg ----------" << endl;
        chess_hall_cfg.output();
        std::cout << "========== SrvCommonCfg : chess_hall_cfg ==========\n" << endl;
        std::cout << "---------- SrvCommonCfg : game_cfg ----------" << endl;
        game_cfg.output();
        std::cout << "========== SrvCommonCfg : game_cfg ==========\n" << endl;
        std::cout << "---------- SrvCommonCfg : game_info_map ----------" << endl;
        unsigned int _game_info_map_count_ = 0;
        for (map<unsigned int, GameInfo>::const_iterator it = game_info_map.begin(); it != game_info_map.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_game_info_map_count_++ < (game_info_map.size() - 1)) std::cout << endl;
        }
        std::cout << "========== SrvCommonCfg : game_info_map ==========\n" << endl;
    }
};

}

#endif  // __SERVICECOMMONCONFIG_H__

