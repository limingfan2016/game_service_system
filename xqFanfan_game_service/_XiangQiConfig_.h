#ifndef __XIANGQICONFIG_H__
#define __XIANGQICONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace XiangQiConfig
{

struct RuleInfo
{
    string title;
    string content;

    RuleInfo() {};

    RuleInfo(DOMNode* parent)
    {
        title = CXmlConfig::getValue(parent, "title");
        content = CXmlConfig::getValue(parent, "content");
    }

    void output() const
    {
        std::cout << "RuleInfo : title = " << title << endl;
        std::cout << "RuleInfo : content = " << content << endl;
    }
};

struct ChessTaskConfig
{
    unsigned int type;
    string picture;
    string title;
    unsigned int value;
    unsigned int amount;
    map<unsigned int, unsigned int> gifts;

    ChessTaskConfig() {};

    ChessTaskConfig(DOMNode* parent)
    {
        type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "type"));
        picture = CXmlConfig::getValue(parent, "picture");
        title = CXmlConfig::getValue(parent, "title");
        value = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "value"));
        amount = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "amount"));
        
        gifts.clear();
        DomNodeArray _gifts;
        CXmlConfig::getNode(parent, _gifts, "gifts", "value", "unsigned int");
        for (unsigned int i = 0; i < _gifts.size(); ++i)
        {
            gifts[CXmlConfig::stringToInt(CXmlConfig::getKey(_gifts[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_gifts[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "ChessTaskConfig : type = " << type << endl;
        std::cout << "ChessTaskConfig : picture = " << picture << endl;
        std::cout << "ChessTaskConfig : title = " << title << endl;
        std::cout << "ChessTaskConfig : value = " << value << endl;
        std::cout << "ChessTaskConfig : amount = " << amount << endl;
        std::cout << "---------- ChessTaskConfig : gifts ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = gifts.begin(); it != gifts.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== ChessTaskConfig : gifts ==========\n" << endl;
    }
};

struct CheatConfig
{
    unsigned int cheat_pao_eat_our_rate;
    unsigned int cheat_pao_eat_our_max_value;
    unsigned int our_pao_cheat_eat_count;
    unsigned int opponent_pao_cheat_eat_count;
    unsigned int our_pao_cheat_base_rate;
    unsigned int opponent_pao_cheat_base_rate;
    unsigned int opponent_open_cp_cheat_base_rate;

    CheatConfig() {};

    CheatConfig(DOMNode* parent)
    {
        cheat_pao_eat_our_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "cheat_pao_eat_our_rate"));
        cheat_pao_eat_our_max_value = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "cheat_pao_eat_our_max_value"));
        our_pao_cheat_eat_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "our_pao_cheat_eat_count"));
        opponent_pao_cheat_eat_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "opponent_pao_cheat_eat_count"));
        our_pao_cheat_base_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "our_pao_cheat_base_rate"));
        opponent_pao_cheat_base_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "opponent_pao_cheat_base_rate"));
        opponent_open_cp_cheat_base_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "opponent_open_cp_cheat_base_rate"));
    }

    void output() const
    {
        std::cout << "CheatConfig : cheat_pao_eat_our_rate = " << cheat_pao_eat_our_rate << endl;
        std::cout << "CheatConfig : cheat_pao_eat_our_max_value = " << cheat_pao_eat_our_max_value << endl;
        std::cout << "CheatConfig : our_pao_cheat_eat_count = " << our_pao_cheat_eat_count << endl;
        std::cout << "CheatConfig : opponent_pao_cheat_eat_count = " << opponent_pao_cheat_eat_count << endl;
        std::cout << "CheatConfig : our_pao_cheat_base_rate = " << our_pao_cheat_base_rate << endl;
        std::cout << "CheatConfig : opponent_pao_cheat_base_rate = " << opponent_pao_cheat_base_rate << endl;
        std::cout << "CheatConfig : opponent_open_cp_cheat_base_rate = " << opponent_open_cp_cheat_base_rate << endl;
    }
};

struct RobotReadyOpt
{
    unsigned int get_red_first_rate;
    unsigned int min_red_first_secs;
    unsigned int max_red_first_secs;
    unsigned int add_double_rate;
    unsigned int min_add_double_secs;
    unsigned int max_add_double_secs;

    RobotReadyOpt() {};

    RobotReadyOpt(DOMNode* parent)
    {
        get_red_first_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "get_red_first_rate"));
        min_red_first_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_red_first_secs"));
        max_red_first_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_red_first_secs"));
        add_double_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "add_double_rate"));
        min_add_double_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_add_double_secs"));
        max_add_double_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_add_double_secs"));
    }

    void output() const
    {
        std::cout << "RobotReadyOpt : get_red_first_rate = " << get_red_first_rate << endl;
        std::cout << "RobotReadyOpt : min_red_first_secs = " << min_red_first_secs << endl;
        std::cout << "RobotReadyOpt : max_red_first_secs = " << max_red_first_secs << endl;
        std::cout << "RobotReadyOpt : add_double_rate = " << add_double_rate << endl;
        std::cout << "RobotReadyOpt : min_add_double_secs = " << min_add_double_secs << endl;
        std::cout << "RobotReadyOpt : max_add_double_secs = " << max_add_double_secs << endl;
    }
};

struct RobotConfig
{
    RobotReadyOpt robot_ready_opt;
    double min_player_gold_rate;
    double max_player_gold_rate;
    unsigned int min_wait_play;
    unsigned int max_wait_play;
    unsigned int min_refuse_peace;
    unsigned int max_refuse_peace;
    unsigned int min_chase_step;
    unsigned int max_chase_step;
    unsigned int pao_eat_our_rate;
    unsigned int pao_eat_our_max_value;
    int dodge_steps;
    CheatConfig cheat_cfg;
    vector<string> nick_name;

    RobotConfig() {};

    RobotConfig(DOMNode* parent)
    {
        DomNodeArray _robot_ready_opt;
        CXmlConfig::getNode(parent, "param", "robot_ready_opt", _robot_ready_opt);
        if (_robot_ready_opt.size() > 0) robot_ready_opt = RobotReadyOpt(_robot_ready_opt[0]);
        min_player_gold_rate = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "min_player_gold_rate"));
        max_player_gold_rate = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "max_player_gold_rate"));
        min_wait_play = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_wait_play"));
        max_wait_play = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_wait_play"));
        min_refuse_peace = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_refuse_peace"));
        max_refuse_peace = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_refuse_peace"));
        min_chase_step = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_chase_step"));
        max_chase_step = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_chase_step"));
        pao_eat_our_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "pao_eat_our_rate"));
        pao_eat_our_max_value = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "pao_eat_our_max_value"));
        dodge_steps = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "dodge_steps"));
        
        DomNodeArray _cheat_cfg;
        CXmlConfig::getNode(parent, "param", "cheat_cfg", _cheat_cfg);
        if (_cheat_cfg.size() > 0) cheat_cfg = CheatConfig(_cheat_cfg[0]);
        
        nick_name.clear();
        DomNodeArray _nick_name;
        CXmlConfig::getNode(parent, _nick_name, "nick_name", "value", "string");
        for (unsigned int i = 0; i < _nick_name.size(); ++i)
        {
            nick_name.push_back(CXmlConfig::getValue(_nick_name[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "---------- RobotConfig : robot_ready_opt ----------" << endl;
        robot_ready_opt.output();
        std::cout << "========== RobotConfig : robot_ready_opt ==========\n" << endl;
        std::cout << "RobotConfig : min_player_gold_rate = " << min_player_gold_rate << endl;
        std::cout << "RobotConfig : max_player_gold_rate = " << max_player_gold_rate << endl;
        std::cout << "RobotConfig : min_wait_play = " << min_wait_play << endl;
        std::cout << "RobotConfig : max_wait_play = " << max_wait_play << endl;
        std::cout << "RobotConfig : min_refuse_peace = " << min_refuse_peace << endl;
        std::cout << "RobotConfig : max_refuse_peace = " << max_refuse_peace << endl;
        std::cout << "RobotConfig : min_chase_step = " << min_chase_step << endl;
        std::cout << "RobotConfig : max_chase_step = " << max_chase_step << endl;
        std::cout << "RobotConfig : pao_eat_our_rate = " << pao_eat_our_rate << endl;
        std::cout << "RobotConfig : pao_eat_our_max_value = " << pao_eat_our_max_value << endl;
        std::cout << "RobotConfig : dodge_steps = " << dodge_steps << endl;
        std::cout << "---------- RobotConfig : cheat_cfg ----------" << endl;
        cheat_cfg.output();
        std::cout << "========== RobotConfig : cheat_cfg ==========\n" << endl;
        std::cout << "---------- RobotConfig : nick_name ----------" << endl;
        for (unsigned int i = 0; i < nick_name.size(); ++i)
        {
            std::cout << "value = " << nick_name[i] << endl;
        }
        std::cout << "========== RobotConfig : nick_name ==========\n" << endl;
    }
};

struct ChessArenaConfig
{
    string name;
    unsigned int base_rate;
    unsigned int bedefeate_rate;
    unsigned int other_rate;
    unsigned int max_rate;
    unsigned int service_cost;
    unsigned int min_gold;
    unsigned int max_gold;
    unsigned int life;
    unsigned int win_rate_percent;

    ChessArenaConfig() {};

    ChessArenaConfig(DOMNode* parent)
    {
        name = CXmlConfig::getValue(parent, "name");
        base_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "base_rate"));
        bedefeate_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "bedefeate_rate"));
        other_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "other_rate"));
        max_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_rate"));
        service_cost = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "service_cost"));
        min_gold = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_gold"));
        max_gold = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_gold"));
        life = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "life"));
        win_rate_percent = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "win_rate_percent"));
    }

    void output() const
    {
        std::cout << "ChessArenaConfig : name = " << name << endl;
        std::cout << "ChessArenaConfig : base_rate = " << base_rate << endl;
        std::cout << "ChessArenaConfig : bedefeate_rate = " << bedefeate_rate << endl;
        std::cout << "ChessArenaConfig : other_rate = " << other_rate << endl;
        std::cout << "ChessArenaConfig : max_rate = " << max_rate << endl;
        std::cout << "ChessArenaConfig : service_cost = " << service_cost << endl;
        std::cout << "ChessArenaConfig : min_gold = " << min_gold << endl;
        std::cout << "ChessArenaConfig : max_gold = " << max_gold << endl;
        std::cout << "ChessArenaConfig : life = " << life << endl;
        std::cout << "ChessArenaConfig : win_rate_percent = " << win_rate_percent << endl;
    }
};

struct CommConfig
{
    string name;
    unsigned int service_flag;
    unsigned int max_online_persons;
    unsigned int wait_matching_secs;
    unsigned int compete_red_first_secs;
    unsigned int add_double_secs;
    unsigned int play_chess_pieces_secs;
    unsigned int please_peace_move_count;
    unsigned int please_peace_max_times;
    unsigned int play_again_wait_secs;
    unsigned int restrict_move_step;
    unsigned int restrict_move_prompt;
    unsigned int continue_move_step;
    unsigned int continue_move_prompt;

    CommConfig() {};

    CommConfig(DOMNode* parent)
    {
        name = CXmlConfig::getValue(parent, "name");
        service_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "service_flag"));
        max_online_persons = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_online_persons"));
        wait_matching_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "wait_matching_secs"));
        compete_red_first_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "compete_red_first_secs"));
        add_double_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "add_double_secs"));
        play_chess_pieces_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "play_chess_pieces_secs"));
        please_peace_move_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "please_peace_move_count"));
        please_peace_max_times = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "please_peace_max_times"));
        play_again_wait_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "play_again_wait_secs"));
        restrict_move_step = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "restrict_move_step"));
        restrict_move_prompt = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "restrict_move_prompt"));
        continue_move_step = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "continue_move_step"));
        continue_move_prompt = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "continue_move_prompt"));
    }

    void output() const
    {
        std::cout << "CommConfig : name = " << name << endl;
        std::cout << "CommConfig : service_flag = " << service_flag << endl;
        std::cout << "CommConfig : max_online_persons = " << max_online_persons << endl;
        std::cout << "CommConfig : wait_matching_secs = " << wait_matching_secs << endl;
        std::cout << "CommConfig : compete_red_first_secs = " << compete_red_first_secs << endl;
        std::cout << "CommConfig : add_double_secs = " << add_double_secs << endl;
        std::cout << "CommConfig : play_chess_pieces_secs = " << play_chess_pieces_secs << endl;
        std::cout << "CommConfig : please_peace_move_count = " << please_peace_move_count << endl;
        std::cout << "CommConfig : please_peace_max_times = " << please_peace_max_times << endl;
        std::cout << "CommConfig : play_again_wait_secs = " << play_again_wait_secs << endl;
        std::cout << "CommConfig : restrict_move_step = " << restrict_move_step << endl;
        std::cout << "CommConfig : restrict_move_prompt = " << restrict_move_prompt << endl;
        std::cout << "CommConfig : continue_move_step = " << continue_move_step << endl;
        std::cout << "CommConfig : continue_move_prompt = " << continue_move_prompt << endl;
    }
};

struct config : public IXmlConfigBase
{
    unordered_map<unsigned int, unsigned int> chess_pieces_value;
    unordered_map<unsigned int, unsigned int> chess_pieces_life;
    unordered_map<unsigned int, unsigned int> settlement_value;
    CommConfig common_cfg;
    map<unsigned int, ChessArenaConfig> classic_arena_cfg;
    RobotConfig robot_cfg;
    map<unsigned int, ChessTaskConfig> task_cfg;
    RuleInfo rule_info;

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
        chess_pieces_value.clear();
        DomNodeArray _chess_pieces_value;
        CXmlConfig::getNode(parent, _chess_pieces_value, "chess_pieces_value", "value", "unsigned int");
        for (unsigned int i = 0; i < _chess_pieces_value.size(); ++i)
        {
            chess_pieces_value[CXmlConfig::stringToInt(CXmlConfig::getKey(_chess_pieces_value[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_chess_pieces_value[i], "value"));
        }
        
        chess_pieces_life.clear();
        DomNodeArray _chess_pieces_life;
        CXmlConfig::getNode(parent, _chess_pieces_life, "chess_pieces_life", "value", "unsigned int");
        for (unsigned int i = 0; i < _chess_pieces_life.size(); ++i)
        {
            chess_pieces_life[CXmlConfig::stringToInt(CXmlConfig::getKey(_chess_pieces_life[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_chess_pieces_life[i], "value"));
        }
        
        settlement_value.clear();
        DomNodeArray _settlement_value;
        CXmlConfig::getNode(parent, _settlement_value, "settlement_value", "value", "unsigned int");
        for (unsigned int i = 0; i < _settlement_value.size(); ++i)
        {
            settlement_value[CXmlConfig::stringToInt(CXmlConfig::getKey(_settlement_value[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_settlement_value[i], "value"));
        }
        
        DomNodeArray _common_cfg;
        CXmlConfig::getNode(parent, "param", "common_cfg", _common_cfg);
        if (_common_cfg.size() > 0) common_cfg = CommConfig(_common_cfg[0]);
        
        classic_arena_cfg.clear();
        DomNodeArray _classic_arena_cfg;
        CXmlConfig::getNode(parent, _classic_arena_cfg, "classic_arena_cfg", "ChessArenaConfig");
        for (unsigned int i = 0; i < _classic_arena_cfg.size(); ++i)
        {
            classic_arena_cfg[CXmlConfig::stringToInt(CXmlConfig::getKey(_classic_arena_cfg[i], "key"))] = ChessArenaConfig(_classic_arena_cfg[i]);
        }
        
        DomNodeArray _robot_cfg;
        CXmlConfig::getNode(parent, "param", "robot_cfg", _robot_cfg);
        if (_robot_cfg.size() > 0) robot_cfg = RobotConfig(_robot_cfg[0]);
        
        task_cfg.clear();
        DomNodeArray _task_cfg;
        CXmlConfig::getNode(parent, _task_cfg, "task_cfg", "ChessTaskConfig");
        for (unsigned int i = 0; i < _task_cfg.size(); ++i)
        {
            task_cfg[CXmlConfig::stringToInt(CXmlConfig::getKey(_task_cfg[i], "key"))] = ChessTaskConfig(_task_cfg[i]);
        }
        
        DomNodeArray _rule_info;
        CXmlConfig::getNode(parent, "param", "rule_info", _rule_info);
        if (_rule_info.size() > 0) rule_info = RuleInfo(_rule_info[0]);
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- config : chess_pieces_value ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = chess_pieces_value.begin(); it != chess_pieces_value.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== config : chess_pieces_value ==========\n" << endl;
        std::cout << "---------- config : chess_pieces_life ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = chess_pieces_life.begin(); it != chess_pieces_life.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== config : chess_pieces_life ==========\n" << endl;
        std::cout << "---------- config : settlement_value ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = settlement_value.begin(); it != settlement_value.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== config : settlement_value ==========\n" << endl;
        std::cout << "---------- config : common_cfg ----------" << endl;
        common_cfg.output();
        std::cout << "========== config : common_cfg ==========\n" << endl;
        std::cout << "---------- config : classic_arena_cfg ----------" << endl;
        unsigned int _classic_arena_cfg_count_ = 0;
        for (map<unsigned int, ChessArenaConfig>::const_iterator it = classic_arena_cfg.begin(); it != classic_arena_cfg.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_classic_arena_cfg_count_++ < (classic_arena_cfg.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : classic_arena_cfg ==========\n" << endl;
        std::cout << "---------- config : robot_cfg ----------" << endl;
        robot_cfg.output();
        std::cout << "========== config : robot_cfg ==========\n" << endl;
        std::cout << "---------- config : task_cfg ----------" << endl;
        unsigned int _task_cfg_count_ = 0;
        for (map<unsigned int, ChessTaskConfig>::const_iterator it = task_cfg.begin(); it != task_cfg.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_task_cfg_count_++ < (task_cfg.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : task_cfg ==========\n" << endl;
        std::cout << "---------- config : rule_info ----------" << endl;
        rule_info.output();
        std::cout << "========== config : rule_info ==========\n" << endl;
    }
};

}

#endif  // __XIANGQICONFIG_H__

