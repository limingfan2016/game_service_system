#ifndef __HALLCONFIGDATA_H__
#define __HALLCONFIGDATA_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace HallConfigData
{

struct CGA_DropGoods
{
    unsigned int min_count;
    unsigned int max_count;
    unsigned int min_speed;
    unsigned int max_speed;
    unsigned int get_min_number;
    unsigned int get_max_number;

    CGA_DropGoods() {};

    CGA_DropGoods(DOMNode* parent)
    {
        min_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_count"));
        max_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_count"));
        min_speed = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_speed"));
        max_speed = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_speed"));
        get_min_number = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "get_min_number"));
        get_max_number = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "get_max_number"));
    }

    void output() const
    {
        std::cout << "CGA_DropGoods : min_count = " << min_count << endl;
        std::cout << "CGA_DropGoods : max_count = " << max_count << endl;
        std::cout << "CGA_DropGoods : min_speed = " << min_speed << endl;
        std::cout << "CGA_DropGoods : max_speed = " << max_speed << endl;
        std::cout << "CGA_DropGoods : get_min_number = " << get_min_number << endl;
        std::cout << "CGA_DropGoods : get_max_number = " << get_max_number << endl;
    }
};

struct CGA_RangeValue
{
    unsigned int min_value;
    unsigned int max_value;

    CGA_RangeValue() {};

    CGA_RangeValue(DOMNode* parent)
    {
        min_value = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_value"));
        max_value = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_value"));
    }

    void output() const
    {
        std::cout << "CGA_RangeValue : min_value = " << min_value << endl;
        std::cout << "CGA_RangeValue : max_value = " << max_value << endl;
    }
};

struct ActivityStageTimeRange
{
    map<unsigned int, CGA_RangeValue> stage_range_cfg;
    map<unsigned int, CGA_DropGoods> drop_goods_map;
    map<unsigned int, unsigned int> drop_bomb_type_rate;

    ActivityStageTimeRange() {};

    ActivityStageTimeRange(DOMNode* parent)
    {
        stage_range_cfg.clear();
        DomNodeArray _stage_range_cfg;
        CXmlConfig::getNode(parent, _stage_range_cfg, "stage_range_cfg", "CGA_RangeValue");
        for (unsigned int i = 0; i < _stage_range_cfg.size(); ++i)
        {
            stage_range_cfg[CXmlConfig::stringToInt(CXmlConfig::getKey(_stage_range_cfg[i], "key"))] = CGA_RangeValue(_stage_range_cfg[i]);
        }
        
        drop_goods_map.clear();
        DomNodeArray _drop_goods_map;
        CXmlConfig::getNode(parent, _drop_goods_map, "drop_goods_map", "CGA_DropGoods");
        for (unsigned int i = 0; i < _drop_goods_map.size(); ++i)
        {
            drop_goods_map[CXmlConfig::stringToInt(CXmlConfig::getKey(_drop_goods_map[i], "key"))] = CGA_DropGoods(_drop_goods_map[i]);
        }
        
        drop_bomb_type_rate.clear();
        DomNodeArray _drop_bomb_type_rate;
        CXmlConfig::getNode(parent, _drop_bomb_type_rate, "drop_bomb_type_rate", "value", "unsigned int");
        for (unsigned int i = 0; i < _drop_bomb_type_rate.size(); ++i)
        {
            drop_bomb_type_rate[CXmlConfig::stringToInt(CXmlConfig::getKey(_drop_bomb_type_rate[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_drop_bomb_type_rate[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "---------- ActivityStageTimeRange : stage_range_cfg ----------" << endl;
        unsigned int _stage_range_cfg_count_ = 0;
        for (map<unsigned int, CGA_RangeValue>::const_iterator it = stage_range_cfg.begin(); it != stage_range_cfg.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_stage_range_cfg_count_++ < (stage_range_cfg.size() - 1)) std::cout << endl;
        }
        std::cout << "========== ActivityStageTimeRange : stage_range_cfg ==========\n" << endl;
        std::cout << "---------- ActivityStageTimeRange : drop_goods_map ----------" << endl;
        unsigned int _drop_goods_map_count_ = 0;
        for (map<unsigned int, CGA_DropGoods>::const_iterator it = drop_goods_map.begin(); it != drop_goods_map.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_drop_goods_map_count_++ < (drop_goods_map.size() - 1)) std::cout << endl;
        }
        std::cout << "========== ActivityStageTimeRange : drop_goods_map ==========\n" << endl;
        std::cout << "---------- ActivityStageTimeRange : drop_bomb_type_rate ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = drop_bomb_type_rate.begin(); it != drop_bomb_type_rate.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== ActivityStageTimeRange : drop_bomb_type_rate ==========\n" << endl;
    }
};

struct CatchGiftActivityCfg
{
    unsigned int open_activity;
    string rule;
    unsigned int activity_time_secs;
    unsigned int min_stage_time_secs;
    unsigned int max_stage_time_secs;
    unsigned int day_free_times;
    unsigned int need_pay_gold;
    unsigned int normal_user_time_interval;
    unsigned int vip_user_time_interval;
    unsigned int min_vip_level;
    unsigned int activity_gold_amount;
    unsigned int activity_score_amount;
    unsigned int goods_drop_secs;
    unsigned int catch_use_secs;
    unsigned int catch_max_goods;
    map<unsigned int, string> settlement_prompt_msg;
    vector<ActivityStageTimeRange> activity_stage_time_range_array;

    CatchGiftActivityCfg() {};

    CatchGiftActivityCfg(DOMNode* parent)
    {
        open_activity = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "open_activity"));
        rule = CXmlConfig::getValue(parent, "rule");
        activity_time_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "activity_time_secs"));
        min_stage_time_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_stage_time_secs"));
        max_stage_time_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_stage_time_secs"));
        day_free_times = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "day_free_times"));
        need_pay_gold = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "need_pay_gold"));
        normal_user_time_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "normal_user_time_interval"));
        vip_user_time_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "vip_user_time_interval"));
        min_vip_level = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_vip_level"));
        activity_gold_amount = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "activity_gold_amount"));
        activity_score_amount = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "activity_score_amount"));
        goods_drop_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "goods_drop_secs"));
        catch_use_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "catch_use_secs"));
        catch_max_goods = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "catch_max_goods"));
        
        settlement_prompt_msg.clear();
        DomNodeArray _settlement_prompt_msg;
        CXmlConfig::getNode(parent, _settlement_prompt_msg, "settlement_prompt_msg", "value", "string");
        for (unsigned int i = 0; i < _settlement_prompt_msg.size(); ++i)
        {
            settlement_prompt_msg[CXmlConfig::stringToInt(CXmlConfig::getKey(_settlement_prompt_msg[i], "key"))] = CXmlConfig::getValue(_settlement_prompt_msg[i], "value");
        }
        
        activity_stage_time_range_array.clear();
        DomNodeArray _activity_stage_time_range_array;
        CXmlConfig::getNode(parent, _activity_stage_time_range_array, "activity_stage_time_range_array", "ActivityStageTimeRange");
        for (unsigned int i = 0; i < _activity_stage_time_range_array.size(); ++i)
        {
            activity_stage_time_range_array.push_back(ActivityStageTimeRange(_activity_stage_time_range_array[i]));
        }
    }

    void output() const
    {
        std::cout << "CatchGiftActivityCfg : open_activity = " << open_activity << endl;
        std::cout << "CatchGiftActivityCfg : rule = " << rule << endl;
        std::cout << "CatchGiftActivityCfg : activity_time_secs = " << activity_time_secs << endl;
        std::cout << "CatchGiftActivityCfg : min_stage_time_secs = " << min_stage_time_secs << endl;
        std::cout << "CatchGiftActivityCfg : max_stage_time_secs = " << max_stage_time_secs << endl;
        std::cout << "CatchGiftActivityCfg : day_free_times = " << day_free_times << endl;
        std::cout << "CatchGiftActivityCfg : need_pay_gold = " << need_pay_gold << endl;
        std::cout << "CatchGiftActivityCfg : normal_user_time_interval = " << normal_user_time_interval << endl;
        std::cout << "CatchGiftActivityCfg : vip_user_time_interval = " << vip_user_time_interval << endl;
        std::cout << "CatchGiftActivityCfg : min_vip_level = " << min_vip_level << endl;
        std::cout << "CatchGiftActivityCfg : activity_gold_amount = " << activity_gold_amount << endl;
        std::cout << "CatchGiftActivityCfg : activity_score_amount = " << activity_score_amount << endl;
        std::cout << "CatchGiftActivityCfg : goods_drop_secs = " << goods_drop_secs << endl;
        std::cout << "CatchGiftActivityCfg : catch_use_secs = " << catch_use_secs << endl;
        std::cout << "CatchGiftActivityCfg : catch_max_goods = " << catch_max_goods << endl;
        std::cout << "---------- CatchGiftActivityCfg : settlement_prompt_msg ----------" << endl;
        for (map<unsigned int, string>::const_iterator it = settlement_prompt_msg.begin(); it != settlement_prompt_msg.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== CatchGiftActivityCfg : settlement_prompt_msg ==========\n" << endl;
        std::cout << "---------- CatchGiftActivityCfg : activity_stage_time_range_array ----------" << endl;
        for (unsigned int i = 0; i < activity_stage_time_range_array.size(); ++i)
        {
            activity_stage_time_range_array[i].output();
            if (i < (activity_stage_time_range_array.size() - 1)) std::cout << endl;
        }
        std::cout << "========== CatchGiftActivityCfg : activity_stage_time_range_array ==========\n" << endl;
    }
};

struct RedPacketInfo
{
    unsigned int gift_type;
    unsigned int gift_amount;
    unsigned int red_packet_count;
    unsigned int min_gift_count;
    unsigned int max_gift_count;

    RedPacketInfo() {};

    RedPacketInfo(DOMNode* parent)
    {
        gift_type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "gift_type"));
        gift_amount = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "gift_amount"));
        red_packet_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "red_packet_count"));
        min_gift_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min_gift_count"));
        max_gift_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_gift_count"));
    }

    void output() const
    {
        std::cout << "RedPacketInfo : gift_type = " << gift_type << endl;
        std::cout << "RedPacketInfo : gift_amount = " << gift_amount << endl;
        std::cout << "RedPacketInfo : red_packet_count = " << red_packet_count << endl;
        std::cout << "RedPacketInfo : min_gift_count = " << min_gift_count << endl;
        std::cout << "RedPacketInfo : max_gift_count = " << max_gift_count << endl;
    }
};

struct RedPacketActivityInfo
{
    string start;
    string finish;
    string activity_name;
    string red_packet_name;
    string red_packet_icon;
    string blessing_words;
    unsigned int payment_type;
    unsigned int payment_count;
    vector<RedPacketInfo> red_packet_list;

    RedPacketActivityInfo() {};

    RedPacketActivityInfo(DOMNode* parent)
    {
        start = CXmlConfig::getValue(parent, "start");
        finish = CXmlConfig::getValue(parent, "finish");
        activity_name = CXmlConfig::getValue(parent, "activity_name");
        red_packet_name = CXmlConfig::getValue(parent, "red_packet_name");
        red_packet_icon = CXmlConfig::getValue(parent, "red_packet_icon");
        blessing_words = CXmlConfig::getValue(parent, "blessing_words");
        payment_type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "payment_type"));
        payment_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "payment_count"));
        
        red_packet_list.clear();
        DomNodeArray _red_packet_list;
        CXmlConfig::getNode(parent, _red_packet_list, "red_packet_list", "RedPacketInfo");
        for (unsigned int i = 0; i < _red_packet_list.size(); ++i)
        {
            red_packet_list.push_back(RedPacketInfo(_red_packet_list[i]));
        }
    }

    void output() const
    {
        std::cout << "RedPacketActivityInfo : start = " << start << endl;
        std::cout << "RedPacketActivityInfo : finish = " << finish << endl;
        std::cout << "RedPacketActivityInfo : activity_name = " << activity_name << endl;
        std::cout << "RedPacketActivityInfo : red_packet_name = " << red_packet_name << endl;
        std::cout << "RedPacketActivityInfo : red_packet_icon = " << red_packet_icon << endl;
        std::cout << "RedPacketActivityInfo : blessing_words = " << blessing_words << endl;
        std::cout << "RedPacketActivityInfo : payment_type = " << payment_type << endl;
        std::cout << "RedPacketActivityInfo : payment_count = " << payment_count << endl;
        std::cout << "---------- RedPacketActivityInfo : red_packet_list ----------" << endl;
        for (unsigned int i = 0; i < red_packet_list.size(); ++i)
        {
            red_packet_list[i].output();
            if (i < (red_packet_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== RedPacketActivityInfo : red_packet_list ==========\n" << endl;
    }
};

struct RedPacketActivityDate
{
    vector<RedPacketActivityInfo> red_packet_activity_time_array;

    RedPacketActivityDate() {};

    RedPacketActivityDate(DOMNode* parent)
    {
        red_packet_activity_time_array.clear();
        DomNodeArray _red_packet_activity_time_array;
        CXmlConfig::getNode(parent, _red_packet_activity_time_array, "red_packet_activity_time_array", "RedPacketActivityInfo");
        for (unsigned int i = 0; i < _red_packet_activity_time_array.size(); ++i)
        {
            red_packet_activity_time_array.push_back(RedPacketActivityInfo(_red_packet_activity_time_array[i]));
        }
    }

    void output() const
    {
        std::cout << "---------- RedPacketActivityDate : red_packet_activity_time_array ----------" << endl;
        for (unsigned int i = 0; i < red_packet_activity_time_array.size(); ++i)
        {
            red_packet_activity_time_array[i].output();
            if (i < (red_packet_activity_time_array.size() - 1)) std::cout << endl;
        }
        std::cout << "========== RedPacketActivityDate : red_packet_activity_time_array ==========\n" << endl;
    }
};

struct WinRedPacketActivity
{
    map<string, RedPacketActivityDate> red_packet_activity_date_map;

    WinRedPacketActivity() {};

    WinRedPacketActivity(DOMNode* parent)
    {
        red_packet_activity_date_map.clear();
        DomNodeArray _red_packet_activity_date_map;
        CXmlConfig::getNode(parent, _red_packet_activity_date_map, "red_packet_activity_date_map", "RedPacketActivityDate");
        for (unsigned int i = 0; i < _red_packet_activity_date_map.size(); ++i)
        {
            red_packet_activity_date_map[CXmlConfig::getKey(_red_packet_activity_date_map[i], "key")] = RedPacketActivityDate(_red_packet_activity_date_map[i]);
        }
    }

    void output() const
    {
        std::cout << "---------- WinRedPacketActivity : red_packet_activity_date_map ----------" << endl;
        unsigned int _red_packet_activity_date_map_count_ = 0;
        for (map<string, RedPacketActivityDate>::const_iterator it = red_packet_activity_date_map.begin(); it != red_packet_activity_date_map.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_red_packet_activity_date_map_count_++ < (red_packet_activity_date_map.size() - 1)) std::cout << endl;
        }
        std::cout << "========== WinRedPacketActivity : red_packet_activity_date_map ==========\n" << endl;
    }
};

struct WinRedPacketActivityCfg
{
    string rule;
    unsigned int one_phone_fare_to_gold;
    unsigned int many_scores_to_one_gold;

    WinRedPacketActivityCfg() {};

    WinRedPacketActivityCfg(DOMNode* parent)
    {
        rule = CXmlConfig::getValue(parent, "rule");
        one_phone_fare_to_gold = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "one_phone_fare_to_gold"));
        many_scores_to_one_gold = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "many_scores_to_one_gold"));
    }

    void output() const
    {
        std::cout << "WinRedPacketActivityCfg : rule = " << rule << endl;
        std::cout << "WinRedPacketActivityCfg : one_phone_fare_to_gold = " << one_phone_fare_to_gold << endl;
        std::cout << "WinRedPacketActivityCfg : many_scores_to_one_gold = " << many_scores_to_one_gold << endl;
    }
};

struct ArenaMatchTime
{
    unsigned int vip;
    unsigned int gold;
    unsigned int max_bullet_rate;
    unsigned int match_payment_gold;
    string start;
    string finish;

    ArenaMatchTime() {};

    ArenaMatchTime(DOMNode* parent)
    {
        vip = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "vip"));
        gold = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "gold"));
        max_bullet_rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_bullet_rate"));
        match_payment_gold = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "match_payment_gold"));
        start = CXmlConfig::getValue(parent, "start");
        finish = CXmlConfig::getValue(parent, "finish");
    }

    void output() const
    {
        std::cout << "ArenaMatchTime : vip = " << vip << endl;
        std::cout << "ArenaMatchTime : gold = " << gold << endl;
        std::cout << "ArenaMatchTime : max_bullet_rate = " << max_bullet_rate << endl;
        std::cout << "ArenaMatchTime : match_payment_gold = " << match_payment_gold << endl;
        std::cout << "ArenaMatchTime : start = " << start << endl;
        std::cout << "ArenaMatchTime : finish = " << finish << endl;
    }
};

struct ArenaMatchDate
{
    map<int, ArenaMatchTime> match_time_map;

    ArenaMatchDate() {};

    ArenaMatchDate(DOMNode* parent)
    {
        match_time_map.clear();
        DomNodeArray _match_time_map;
        CXmlConfig::getNode(parent, _match_time_map, "match_time_map", "ArenaMatchTime");
        for (unsigned int i = 0; i < _match_time_map.size(); ++i)
        {
            match_time_map[CXmlConfig::stringToInt(CXmlConfig::getKey(_match_time_map[i], "key"))] = ArenaMatchTime(_match_time_map[i]);
        }
    }

    void output() const
    {
        std::cout << "---------- ArenaMatchDate : match_time_map ----------" << endl;
        unsigned int _match_time_map_count_ = 0;
        for (map<int, ArenaMatchTime>::const_iterator it = match_time_map.begin(); it != match_time_map.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_match_time_map_count_++ < (match_time_map.size() - 1)) std::cout << endl;
        }
        std::cout << "========== ArenaMatchDate : match_time_map ==========\n" << endl;
    }
};

struct ArenaRankingReward
{
    map<unsigned int, unsigned int> rewards;

    ArenaRankingReward() {};

    ArenaRankingReward(DOMNode* parent)
    {
        rewards.clear();
        DomNodeArray _rewards;
        CXmlConfig::getNode(parent, _rewards, "rewards", "value", "unsigned int");
        for (unsigned int i = 0; i < _rewards.size(); ++i)
        {
            rewards[CXmlConfig::stringToInt(CXmlConfig::getKey(_rewards[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_rewards[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "---------- ArenaRankingReward : rewards ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = rewards.begin(); it != rewards.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== ArenaRankingReward : rewards ==========\n" << endl;
    }
};

struct Arena
{
    string name;
    vector<ArenaRankingReward> ranking_rewards;
    map<string, ArenaMatchDate> match_date_map;

    Arena() {};

    Arena(DOMNode* parent)
    {
        name = CXmlConfig::getValue(parent, "name");
        
        ranking_rewards.clear();
        DomNodeArray _ranking_rewards;
        CXmlConfig::getNode(parent, _ranking_rewards, "ranking_rewards", "ArenaRankingReward");
        for (unsigned int i = 0; i < _ranking_rewards.size(); ++i)
        {
            ranking_rewards.push_back(ArenaRankingReward(_ranking_rewards[i]));
        }
        
        match_date_map.clear();
        DomNodeArray _match_date_map;
        CXmlConfig::getNode(parent, _match_date_map, "match_date_map", "ArenaMatchDate");
        for (unsigned int i = 0; i < _match_date_map.size(); ++i)
        {
            match_date_map[CXmlConfig::getKey(_match_date_map[i], "key")] = ArenaMatchDate(_match_date_map[i]);
        }
    }

    void output() const
    {
        std::cout << "Arena : name = " << name << endl;
        std::cout << "---------- Arena : ranking_rewards ----------" << endl;
        for (unsigned int i = 0; i < ranking_rewards.size(); ++i)
        {
            ranking_rewards[i].output();
            if (i < (ranking_rewards.size() - 1)) std::cout << endl;
        }
        std::cout << "========== Arena : ranking_rewards ==========\n" << endl;
        std::cout << "---------- Arena : match_date_map ----------" << endl;
        unsigned int _match_date_map_count_ = 0;
        for (map<string, ArenaMatchDate>::const_iterator it = match_date_map.begin(); it != match_date_map.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_match_date_map_count_++ < (match_date_map.size() - 1)) std::cout << endl;
        }
        std::cout << "========== Arena : match_date_map ==========\n" << endl;
    }
};

struct ArenaCfg
{
    string rule;
    vector<unsigned int> arena_service_ids;

    ArenaCfg() {};

    ArenaCfg(DOMNode* parent)
    {
        rule = CXmlConfig::getValue(parent, "rule");
        
        arena_service_ids.clear();
        DomNodeArray _arena_service_ids;
        CXmlConfig::getNode(parent, _arena_service_ids, "arena_service_ids", "value", "unsigned int");
        for (unsigned int i = 0; i < _arena_service_ids.size(); ++i)
        {
            arena_service_ids.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_arena_service_ids[i], "value")));
        }
    }

    void output() const
    {
        std::cout << "ArenaCfg : rule = " << rule << endl;
        std::cout << "---------- ArenaCfg : arena_service_ids ----------" << endl;
        for (unsigned int i = 0; i < arena_service_ids.size(); ++i)
        {
            std::cout << "value = " << arena_service_ids[i] << endl;
        }
        std::cout << "========== ArenaCfg : arena_service_ids ==========\n" << endl;
    }
};

struct CfgOnlineTask
{
    string name;
    string desc;
    unsigned int beginHour;
    unsigned int beginMin;
    unsigned int endHour;
    unsigned int endMin;
    map<unsigned int, unsigned int> rewards;

    CfgOnlineTask() {};

    CfgOnlineTask(DOMNode* parent)
    {
        name = CXmlConfig::getValue(parent, "name");
        desc = CXmlConfig::getValue(parent, "desc");
        beginHour = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "beginHour"));
        beginMin = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "beginMin"));
        endHour = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "endHour"));
        endMin = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "endMin"));
        
        rewards.clear();
        DomNodeArray _rewards;
        CXmlConfig::getNode(parent, _rewards, "rewards", "value", "unsigned int");
        for (unsigned int i = 0; i < _rewards.size(); ++i)
        {
            rewards[CXmlConfig::stringToInt(CXmlConfig::getKey(_rewards[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_rewards[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "CfgOnlineTask : name = " << name << endl;
        std::cout << "CfgOnlineTask : desc = " << desc << endl;
        std::cout << "CfgOnlineTask : beginHour = " << beginHour << endl;
        std::cout << "CfgOnlineTask : beginMin = " << beginMin << endl;
        std::cout << "CfgOnlineTask : endHour = " << endHour << endl;
        std::cout << "CfgOnlineTask : endMin = " << endMin << endl;
        std::cout << "---------- CfgOnlineTask : rewards ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = rewards.begin(); it != rewards.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== CfgOnlineTask : rewards ==========\n" << endl;
    }
};

struct CfgWinMatch
{
    string name;
    string desc;
    unsigned int times;
    map<unsigned int, unsigned int> rewards;

    CfgWinMatch() {};

    CfgWinMatch(DOMNode* parent)
    {
        name = CXmlConfig::getValue(parent, "name");
        desc = CXmlConfig::getValue(parent, "desc");
        times = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "times"));
        
        rewards.clear();
        DomNodeArray _rewards;
        CXmlConfig::getNode(parent, _rewards, "rewards", "value", "unsigned int");
        for (unsigned int i = 0; i < _rewards.size(); ++i)
        {
            rewards[CXmlConfig::stringToInt(CXmlConfig::getKey(_rewards[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_rewards[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "CfgWinMatch : name = " << name << endl;
        std::cout << "CfgWinMatch : desc = " << desc << endl;
        std::cout << "CfgWinMatch : times = " << times << endl;
        std::cout << "---------- CfgWinMatch : rewards ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = rewards.begin(); it != rewards.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== CfgWinMatch : rewards ==========\n" << endl;
    }
};

struct CfgJoinMatch
{
    string name;
    string desc;
    unsigned int times;
    map<unsigned int, unsigned int> rewards;

    CfgJoinMatch() {};

    CfgJoinMatch(DOMNode* parent)
    {
        name = CXmlConfig::getValue(parent, "name");
        desc = CXmlConfig::getValue(parent, "desc");
        times = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "times"));
        
        rewards.clear();
        DomNodeArray _rewards;
        CXmlConfig::getNode(parent, _rewards, "rewards", "value", "unsigned int");
        for (unsigned int i = 0; i < _rewards.size(); ++i)
        {
            rewards[CXmlConfig::stringToInt(CXmlConfig::getKey(_rewards[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_rewards[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "CfgJoinMatch : name = " << name << endl;
        std::cout << "CfgJoinMatch : desc = " << desc << endl;
        std::cout << "CfgJoinMatch : times = " << times << endl;
        std::cout << "---------- CfgJoinMatch : rewards ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = rewards.begin(); it != rewards.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== CfgJoinMatch : rewards ==========\n" << endl;
    }
};

struct KnapsackItem
{
    unsigned int type;
    unsigned int show;

    KnapsackItem() {};

    KnapsackItem(DOMNode* parent)
    {
        type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "type"));
        show = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "show"));
    }

    void output() const
    {
        std::cout << "KnapsackItem : type = " << type << endl;
        std::cout << "KnapsackItem : show = " << show << endl;
    }
};

struct Knapsack
{
    int show_zero_value;
    vector<KnapsackItem> knapsack_goods;

    Knapsack() {};

    Knapsack(DOMNode* parent)
    {
        show_zero_value = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "show_zero_value"));
        
        knapsack_goods.clear();
        DomNodeArray _knapsack_goods;
        CXmlConfig::getNode(parent, _knapsack_goods, "knapsack_goods", "KnapsackItem");
        for (unsigned int i = 0; i < _knapsack_goods.size(); ++i)
        {
            knapsack_goods.push_back(KnapsackItem(_knapsack_goods[i]));
        }
    }

    void output() const
    {
        std::cout << "Knapsack : show_zero_value = " << show_zero_value << endl;
        std::cout << "---------- Knapsack : knapsack_goods ----------" << endl;
        for (unsigned int i = 0; i < knapsack_goods.size(); ++i)
        {
            knapsack_goods[i].output();
            if (i < (knapsack_goods.size() - 1)) std::cout << endl;
        }
        std::cout << "========== Knapsack : knapsack_goods ==========\n" << endl;
    }
};

struct RedGiftRewardCondition
{
    unsigned int condition_type;
    unsigned int condition_count;
    string condition_desc;
    unsigned int reward_id;
    unsigned int reward_count;

    RedGiftRewardCondition() {};

    RedGiftRewardCondition(DOMNode* parent)
    {
        condition_type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "condition_type"));
        condition_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "condition_count"));
        condition_desc = CXmlConfig::getValue(parent, "condition_desc");
        reward_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "reward_id"));
        reward_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "reward_count"));
    }

    void output() const
    {
        std::cout << "RedGiftRewardCondition : condition_type = " << condition_type << endl;
        std::cout << "RedGiftRewardCondition : condition_count = " << condition_count << endl;
        std::cout << "RedGiftRewardCondition : condition_desc = " << condition_desc << endl;
        std::cout << "RedGiftRewardCondition : reward_id = " << reward_id << endl;
        std::cout << "RedGiftRewardCondition : reward_count = " << reward_count << endl;
    }
};

struct RedGiftWord
{
    unsigned int reward_type;
    unsigned int reward_value;
    int max_reward_count;
    vector<RedGiftRewardCondition> receive_reward_condition;

    RedGiftWord() {};

    RedGiftWord(DOMNode* parent)
    {
        reward_type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "reward_type"));
        reward_value = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "reward_value"));
        max_reward_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_reward_count"));
        
        receive_reward_condition.clear();
        DomNodeArray _receive_reward_condition;
        CXmlConfig::getNode(parent, _receive_reward_condition, "receive_reward_condition", "RedGiftRewardCondition");
        for (unsigned int i = 0; i < _receive_reward_condition.size(); ++i)
        {
            receive_reward_condition.push_back(RedGiftRewardCondition(_receive_reward_condition[i]));
        }
    }

    void output() const
    {
        std::cout << "RedGiftWord : reward_type = " << reward_type << endl;
        std::cout << "RedGiftWord : reward_value = " << reward_value << endl;
        std::cout << "RedGiftWord : max_reward_count = " << max_reward_count << endl;
        std::cout << "---------- RedGiftWord : receive_reward_condition ----------" << endl;
        for (unsigned int i = 0; i < receive_reward_condition.size(); ++i)
        {
            receive_reward_condition[i].output();
            if (i < (receive_reward_condition.size() - 1)) std::cout << endl;
        }
        std::cout << "========== RedGiftWord : receive_reward_condition ==========\n" << endl;
    }
};

struct RewardItem
{
    map<unsigned int, unsigned int> reward;

    RewardItem() {};

    RewardItem(DOMNode* parent)
    {
        reward.clear();
        DomNodeArray _reward;
        CXmlConfig::getNode(parent, _reward, "reward", "value", "unsigned int");
        for (unsigned int i = 0; i < _reward.size(); ++i)
        {
            reward[CXmlConfig::stringToInt(CXmlConfig::getKey(_reward[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_reward[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "---------- RewardItem : reward ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = reward.begin(); it != reward.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== RewardItem : reward ==========\n" << endl;
    }
};

struct PkPlay
{
    int play_id;
    int user_lv;
    int play_time;
    int ticket;
    map<unsigned int, unsigned int> condition_list;
    unsigned int start_time_hour;
    unsigned int start_time_min;
    unsigned int start_time_sec;
    unsigned int end_time_hour;
    unsigned int end_time_min;
    unsigned int end_time_sec;
    unsigned int reward_type;
    unsigned int reward_num_min;
    unsigned int reward_num_max;
    map<unsigned int, unsigned int> entrance_fee;
    map<unsigned int, unsigned int> carry_prop;

    PkPlay() {};

    PkPlay(DOMNode* parent)
    {
        play_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "play_id"));
        user_lv = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "user_lv"));
        play_time = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "play_time"));
        ticket = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "ticket"));
        
        condition_list.clear();
        DomNodeArray _condition_list;
        CXmlConfig::getNode(parent, _condition_list, "condition_list", "value", "unsigned int");
        for (unsigned int i = 0; i < _condition_list.size(); ++i)
        {
            condition_list[CXmlConfig::stringToInt(CXmlConfig::getKey(_condition_list[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_condition_list[i], "value"));
        }
        start_time_hour = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "start_time_hour"));
        start_time_min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "start_time_min"));
        start_time_sec = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "start_time_sec"));
        end_time_hour = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "end_time_hour"));
        end_time_min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "end_time_min"));
        end_time_sec = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "end_time_sec"));
        reward_type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "reward_type"));
        reward_num_min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "reward_num_min"));
        reward_num_max = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "reward_num_max"));
        
        entrance_fee.clear();
        DomNodeArray _entrance_fee;
        CXmlConfig::getNode(parent, _entrance_fee, "entrance_fee", "value", "unsigned int");
        for (unsigned int i = 0; i < _entrance_fee.size(); ++i)
        {
            entrance_fee[CXmlConfig::stringToInt(CXmlConfig::getKey(_entrance_fee[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_entrance_fee[i], "value"));
        }
        
        carry_prop.clear();
        DomNodeArray _carry_prop;
        CXmlConfig::getNode(parent, _carry_prop, "carry_prop", "value", "unsigned int");
        for (unsigned int i = 0; i < _carry_prop.size(); ++i)
        {
            carry_prop[CXmlConfig::stringToInt(CXmlConfig::getKey(_carry_prop[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_carry_prop[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "PkPlay : play_id = " << play_id << endl;
        std::cout << "PkPlay : user_lv = " << user_lv << endl;
        std::cout << "PkPlay : play_time = " << play_time << endl;
        std::cout << "PkPlay : ticket = " << ticket << endl;
        std::cout << "---------- PkPlay : condition_list ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = condition_list.begin(); it != condition_list.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== PkPlay : condition_list ==========\n" << endl;
        std::cout << "PkPlay : start_time_hour = " << start_time_hour << endl;
        std::cout << "PkPlay : start_time_min = " << start_time_min << endl;
        std::cout << "PkPlay : start_time_sec = " << start_time_sec << endl;
        std::cout << "PkPlay : end_time_hour = " << end_time_hour << endl;
        std::cout << "PkPlay : end_time_min = " << end_time_min << endl;
        std::cout << "PkPlay : end_time_sec = " << end_time_sec << endl;
        std::cout << "PkPlay : reward_type = " << reward_type << endl;
        std::cout << "PkPlay : reward_num_min = " << reward_num_min << endl;
        std::cout << "PkPlay : reward_num_max = " << reward_num_max << endl;
        std::cout << "---------- PkPlay : entrance_fee ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = entrance_fee.begin(); it != entrance_fee.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== PkPlay : entrance_fee ==========\n" << endl;
        std::cout << "---------- PkPlay : carry_prop ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = carry_prop.begin(); it != carry_prop.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== PkPlay : carry_prop ==========\n" << endl;
    }
};

struct playMatchingLv
{
    int play_id;

    playMatchingLv() {};

    playMatchingLv(DOMNode* parent)
    {
        play_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "play_id"));
    }

    void output() const
    {
        std::cout << "playMatchingLv : play_id = " << play_id << endl;
    }
};

struct PkPlayInfo
{
    unsigned int matching_time;
    string play_rule;
    unsigned int play_num_min;
    unsigned int wait_ready_time;
    unsigned int gold_max;
    unsigned int guide_time;
    vector<int> no_guide_service;
    vector<playMatchingLv> matchingLvList;
    vector<PkPlay> PkPlayInfoList;

    PkPlayInfo() {};

    PkPlayInfo(DOMNode* parent)
    {
        matching_time = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "matching_time"));
        play_rule = CXmlConfig::getValue(parent, "play_rule");
        play_num_min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "play_num_min"));
        wait_ready_time = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "wait_ready_time"));
        gold_max = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "gold_max"));
        guide_time = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "guide_time"));
        
        no_guide_service.clear();
        DomNodeArray _no_guide_service;
        CXmlConfig::getNode(parent, _no_guide_service, "no_guide_service", "value", "int");
        for (unsigned int i = 0; i < _no_guide_service.size(); ++i)
        {
            no_guide_service.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_no_guide_service[i], "value")));
        }
        
        matchingLvList.clear();
        DomNodeArray _matchingLvList;
        CXmlConfig::getNode(parent, _matchingLvList, "matchingLvList", "playMatchingLv");
        for (unsigned int i = 0; i < _matchingLvList.size(); ++i)
        {
            matchingLvList.push_back(playMatchingLv(_matchingLvList[i]));
        }
        
        PkPlayInfoList.clear();
        DomNodeArray _PkPlayInfoList;
        CXmlConfig::getNode(parent, _PkPlayInfoList, "PkPlayInfoList", "PkPlay");
        for (unsigned int i = 0; i < _PkPlayInfoList.size(); ++i)
        {
            PkPlayInfoList.push_back(PkPlay(_PkPlayInfoList[i]));
        }
    }

    void output() const
    {
        std::cout << "PkPlayInfo : matching_time = " << matching_time << endl;
        std::cout << "PkPlayInfo : play_rule = " << play_rule << endl;
        std::cout << "PkPlayInfo : play_num_min = " << play_num_min << endl;
        std::cout << "PkPlayInfo : wait_ready_time = " << wait_ready_time << endl;
        std::cout << "PkPlayInfo : gold_max = " << gold_max << endl;
        std::cout << "PkPlayInfo : guide_time = " << guide_time << endl;
        std::cout << "---------- PkPlayInfo : no_guide_service ----------" << endl;
        for (unsigned int i = 0; i < no_guide_service.size(); ++i)
        {
            std::cout << "value = " << no_guide_service[i] << endl;
        }
        std::cout << "========== PkPlayInfo : no_guide_service ==========\n" << endl;
        std::cout << "---------- PkPlayInfo : matchingLvList ----------" << endl;
        for (unsigned int i = 0; i < matchingLvList.size(); ++i)
        {
            matchingLvList[i].output();
            if (i < (matchingLvList.size() - 1)) std::cout << endl;
        }
        std::cout << "========== PkPlayInfo : matchingLvList ==========\n" << endl;
        std::cout << "---------- PkPlayInfo : PkPlayInfoList ----------" << endl;
        for (unsigned int i = 0; i < PkPlayInfoList.size(); ++i)
        {
            PkPlayInfoList[i].output();
            if (i < (PkPlayInfoList.size() - 1)) std::cout << endl;
        }
        std::cout << "========== PkPlayInfo : PkPlayInfoList ==========\n" << endl;
    }
};

struct RechargeReward
{
    unsigned int type;
    unsigned int count;
    unsigned int rate;
    unsigned int flag;

    RechargeReward() {};

    RechargeReward(DOMNode* parent)
    {
        type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "type"));
        count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "count"));
        rate = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rate"));
        flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "flag"));
    }

    void output() const
    {
        std::cout << "RechargeReward : type = " << type << endl;
        std::cout << "RechargeReward : count = " << count << endl;
        std::cout << "RechargeReward : rate = " << rate << endl;
        std::cout << "RechargeReward : flag = " << flag << endl;
    }
};

struct RechargeLotteryActivity
{
    string title;
    string desc;
    string rule;
    string time;
    string start_time;
    string finish_time;
    string lottery_finish_time;
    unsigned int lottery_record_count;
    unsigned int max_phone_fare;
    vector<RechargeReward> recharge_reward_list;

    RechargeLotteryActivity() {};

    RechargeLotteryActivity(DOMNode* parent)
    {
        title = CXmlConfig::getValue(parent, "title");
        desc = CXmlConfig::getValue(parent, "desc");
        rule = CXmlConfig::getValue(parent, "rule");
        time = CXmlConfig::getValue(parent, "time");
        start_time = CXmlConfig::getValue(parent, "start_time");
        finish_time = CXmlConfig::getValue(parent, "finish_time");
        lottery_finish_time = CXmlConfig::getValue(parent, "lottery_finish_time");
        lottery_record_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "lottery_record_count"));
        max_phone_fare = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_phone_fare"));
        
        recharge_reward_list.clear();
        DomNodeArray _recharge_reward_list;
        CXmlConfig::getNode(parent, _recharge_reward_list, "recharge_reward_list", "RechargeReward");
        for (unsigned int i = 0; i < _recharge_reward_list.size(); ++i)
        {
            recharge_reward_list.push_back(RechargeReward(_recharge_reward_list[i]));
        }
    }

    void output() const
    {
        std::cout << "RechargeLotteryActivity : title = " << title << endl;
        std::cout << "RechargeLotteryActivity : desc = " << desc << endl;
        std::cout << "RechargeLotteryActivity : rule = " << rule << endl;
        std::cout << "RechargeLotteryActivity : time = " << time << endl;
        std::cout << "RechargeLotteryActivity : start_time = " << start_time << endl;
        std::cout << "RechargeLotteryActivity : finish_time = " << finish_time << endl;
        std::cout << "RechargeLotteryActivity : lottery_finish_time = " << lottery_finish_time << endl;
        std::cout << "RechargeLotteryActivity : lottery_record_count = " << lottery_record_count << endl;
        std::cout << "RechargeLotteryActivity : max_phone_fare = " << max_phone_fare << endl;
        std::cout << "---------- RechargeLotteryActivity : recharge_reward_list ----------" << endl;
        for (unsigned int i = 0; i < recharge_reward_list.size(); ++i)
        {
            recharge_reward_list[i].output();
            if (i < (recharge_reward_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== RechargeLotteryActivity : recharge_reward_list ==========\n" << endl;
    }
};

struct NewbieGift
{
    unsigned int type;
    unsigned int count;

    NewbieGift() {};

    NewbieGift(DOMNode* parent)
    {
        type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "type"));
        count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "count"));
    }

    void output() const
    {
        std::cout << "NewbieGift : type = " << type << endl;
        std::cout << "NewbieGift : count = " << count << endl;
    }
};

struct NewbieGiftActivity
{
    string title;
    string desc;
    string rule;
    string time;
    string start_time;
    string finish_time;
    vector<NewbieGift> newbie_reward_list;

    NewbieGiftActivity() {};

    NewbieGiftActivity(DOMNode* parent)
    {
        title = CXmlConfig::getValue(parent, "title");
        desc = CXmlConfig::getValue(parent, "desc");
        rule = CXmlConfig::getValue(parent, "rule");
        time = CXmlConfig::getValue(parent, "time");
        start_time = CXmlConfig::getValue(parent, "start_time");
        finish_time = CXmlConfig::getValue(parent, "finish_time");
        
        newbie_reward_list.clear();
        DomNodeArray _newbie_reward_list;
        CXmlConfig::getNode(parent, _newbie_reward_list, "newbie_reward_list", "NewbieGift");
        for (unsigned int i = 0; i < _newbie_reward_list.size(); ++i)
        {
            newbie_reward_list.push_back(NewbieGift(_newbie_reward_list[i]));
        }
    }

    void output() const
    {
        std::cout << "NewbieGiftActivity : title = " << title << endl;
        std::cout << "NewbieGiftActivity : desc = " << desc << endl;
        std::cout << "NewbieGiftActivity : rule = " << rule << endl;
        std::cout << "NewbieGiftActivity : time = " << time << endl;
        std::cout << "NewbieGiftActivity : start_time = " << start_time << endl;
        std::cout << "NewbieGiftActivity : finish_time = " << finish_time << endl;
        std::cout << "---------- NewbieGiftActivity : newbie_reward_list ----------" << endl;
        for (unsigned int i = 0; i < newbie_reward_list.size(); ++i)
        {
            newbie_reward_list[i].output();
            if (i < (newbie_reward_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== NewbieGiftActivity : newbie_reward_list ==========\n" << endl;
    }
};

struct share_reward
{
    map<unsigned int, unsigned int> reward;

    share_reward() {};

    share_reward(DOMNode* parent)
    {
        reward.clear();
        DomNodeArray _reward;
        CXmlConfig::getNode(parent, _reward, "reward", "value", "unsigned int");
        for (unsigned int i = 0; i < _reward.size(); ++i)
        {
            reward[CXmlConfig::stringToInt(CXmlConfig::getKey(_reward[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_reward[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "---------- share_reward : reward ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = reward.begin(); it != reward.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== share_reward : reward ==========\n" << endl;
    }
};

struct HallShare
{
    string title;
    string content;
    vector<share_reward> share_reward_list;

    HallShare() {};

    HallShare(DOMNode* parent)
    {
        title = CXmlConfig::getValue(parent, "title");
        content = CXmlConfig::getValue(parent, "content");
        
        share_reward_list.clear();
        DomNodeArray _share_reward_list;
        CXmlConfig::getNode(parent, _share_reward_list, "share_reward_list", "share_reward");
        for (unsigned int i = 0; i < _share_reward_list.size(); ++i)
        {
            share_reward_list.push_back(share_reward(_share_reward_list[i]));
        }
    }

    void output() const
    {
        std::cout << "HallShare : title = " << title << endl;
        std::cout << "HallShare : content = " << content << endl;
        std::cout << "---------- HallShare : share_reward_list ----------" << endl;
        for (unsigned int i = 0; i < share_reward_list.size(); ++i)
        {
            share_reward_list[i].output();
            if (i < (share_reward_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== HallShare : share_reward_list ==========\n" << endl;
    }
};

struct notice_item
{
    string title;
    string content;
    string picture_path;

    notice_item() {};

    notice_item(DOMNode* parent)
    {
        title = CXmlConfig::getValue(parent, "title");
        content = CXmlConfig::getValue(parent, "content");
        picture_path = CXmlConfig::getValue(parent, "picture_path");
    }

    void output() const
    {
        std::cout << "notice_item : title = " << title << endl;
        std::cout << "notice_item : content = " << content << endl;
        std::cout << "notice_item : picture_path = " << picture_path << endl;
    }
};

struct HallNotice
{
    int eject;
    unsigned int notice_version;
    vector<notice_item> notice_list;

    HallNotice() {};

    HallNotice(DOMNode* parent)
    {
        eject = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "eject"));
        notice_version = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "notice_version"));
        
        notice_list.clear();
        DomNodeArray _notice_list;
        CXmlConfig::getNode(parent, _notice_list, "notice_list", "notice_item");
        for (unsigned int i = 0; i < _notice_list.size(); ++i)
        {
            notice_list.push_back(notice_item(_notice_list[i]));
        }
    }

    void output() const
    {
        std::cout << "HallNotice : eject = " << eject << endl;
        std::cout << "HallNotice : notice_version = " << notice_version << endl;
        std::cout << "---------- HallNotice : notice_list ----------" << endl;
        for (unsigned int i = 0; i < notice_list.size(); ++i)
        {
            notice_list[i].output();
            if (i < (notice_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== HallNotice : notice_list ==========\n" << endl;
    }
};

struct VipItem
{
    unsigned int condition;
    unsigned int subsidy;
    unsigned int reward_day;
    map<unsigned int, unsigned int> reward;

    VipItem() {};

    VipItem(DOMNode* parent)
    {
        condition = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "condition"));
        subsidy = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "subsidy"));
        reward_day = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "reward_day"));
        
        reward.clear();
        DomNodeArray _reward;
        CXmlConfig::getNode(parent, _reward, "reward", "value", "unsigned int");
        for (unsigned int i = 0; i < _reward.size(); ++i)
        {
            reward[CXmlConfig::stringToInt(CXmlConfig::getKey(_reward[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_reward[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "VipItem : condition = " << condition << endl;
        std::cout << "VipItem : subsidy = " << subsidy << endl;
        std::cout << "VipItem : reward_day = " << reward_day << endl;
        std::cout << "---------- VipItem : reward ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = reward.begin(); it != reward.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== VipItem : reward ==========\n" << endl;
    }
};

struct HotUpdateVersion
{
    unsigned int check_flag;
    string check_version;
    string new_version;
    string old_version;
    map<string, string> version_pkg_url;

    HotUpdateVersion() {};

    HotUpdateVersion(DOMNode* parent)
    {
        check_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "check_flag"));
        check_version = CXmlConfig::getValue(parent, "check_version");
        new_version = CXmlConfig::getValue(parent, "new_version");
        old_version = CXmlConfig::getValue(parent, "old_version");
        
        version_pkg_url.clear();
        DomNodeArray _version_pkg_url;
        CXmlConfig::getNode(parent, _version_pkg_url, "version_pkg_url", "value", "string");
        for (unsigned int i = 0; i < _version_pkg_url.size(); ++i)
        {
            version_pkg_url[CXmlConfig::getKey(_version_pkg_url[i], "key")] = CXmlConfig::getValue(_version_pkg_url[i], "value");
        }
    }

    void output() const
    {
        std::cout << "HotUpdateVersion : check_flag = " << check_flag << endl;
        std::cout << "HotUpdateVersion : check_version = " << check_version << endl;
        std::cout << "HotUpdateVersion : new_version = " << new_version << endl;
        std::cout << "HotUpdateVersion : old_version = " << old_version << endl;
        std::cout << "---------- HotUpdateVersion : version_pkg_url ----------" << endl;
        for (map<string, string>::const_iterator it = version_pkg_url.begin(); it != version_pkg_url.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== HotUpdateVersion : version_pkg_url ==========\n" << endl;
    }
};

struct client_version
{
    unsigned int check_flag;
    string check_version;
    string version;
    string url;
    string valid;

    client_version() {};

    client_version(DOMNode* parent)
    {
        check_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "check_flag"));
        check_version = CXmlConfig::getValue(parent, "check_version");
        version = CXmlConfig::getValue(parent, "version");
        url = CXmlConfig::getValue(parent, "url");
        valid = CXmlConfig::getValue(parent, "valid");
    }

    void output() const
    {
        std::cout << "client_version : check_flag = " << check_flag << endl;
        std::cout << "client_version : check_version = " << check_version << endl;
        std::cout << "client_version : version = " << version << endl;
        std::cout << "client_version : url = " << url << endl;
        std::cout << "client_version : valid = " << valid << endl;
    }
};

struct GameRoomCfg
{
    unsigned int buyu_max_count;
    unsigned int current_service_id;
    unsigned int new_service_id;
    unordered_map<unsigned int, unsigned int> platform_update_flag;
    unsigned int current_pk_flag;
    unsigned int new_pk_flag;
    unsigned int current_arena_flag;
    unsigned int new_arena_flag;
    unsigned int current_poker_cark_flag;
    unsigned int new_poker_cark_flag;
    unsigned int open_poker_cark_entrance;
    unsigned int current_chess_flag;
    unsigned int new_chess_flag;
    unsigned int open_chess_entrance;

    GameRoomCfg() {};

    GameRoomCfg(DOMNode* parent)
    {
        buyu_max_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "buyu_max_count"));
        current_service_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "current_service_id"));
        new_service_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "new_service_id"));
        
        platform_update_flag.clear();
        DomNodeArray _platform_update_flag;
        CXmlConfig::getNode(parent, _platform_update_flag, "platform_update_flag", "value", "unsigned int");
        for (unsigned int i = 0; i < _platform_update_flag.size(); ++i)
        {
            platform_update_flag[CXmlConfig::stringToInt(CXmlConfig::getKey(_platform_update_flag[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_platform_update_flag[i], "value"));
        }
        current_pk_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "current_pk_flag"));
        new_pk_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "new_pk_flag"));
        current_arena_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "current_arena_flag"));
        new_arena_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "new_arena_flag"));
        current_poker_cark_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "current_poker_cark_flag"));
        new_poker_cark_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "new_poker_cark_flag"));
        open_poker_cark_entrance = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "open_poker_cark_entrance"));
        current_chess_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "current_chess_flag"));
        new_chess_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "new_chess_flag"));
        open_chess_entrance = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "open_chess_entrance"));
    }

    void output() const
    {
        std::cout << "GameRoomCfg : buyu_max_count = " << buyu_max_count << endl;
        std::cout << "GameRoomCfg : current_service_id = " << current_service_id << endl;
        std::cout << "GameRoomCfg : new_service_id = " << new_service_id << endl;
        std::cout << "---------- GameRoomCfg : platform_update_flag ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = platform_update_flag.begin(); it != platform_update_flag.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== GameRoomCfg : platform_update_flag ==========\n" << endl;
        std::cout << "GameRoomCfg : current_pk_flag = " << current_pk_flag << endl;
        std::cout << "GameRoomCfg : new_pk_flag = " << new_pk_flag << endl;
        std::cout << "GameRoomCfg : current_arena_flag = " << current_arena_flag << endl;
        std::cout << "GameRoomCfg : new_arena_flag = " << new_arena_flag << endl;
        std::cout << "GameRoomCfg : current_poker_cark_flag = " << current_poker_cark_flag << endl;
        std::cout << "GameRoomCfg : new_poker_cark_flag = " << new_poker_cark_flag << endl;
        std::cout << "GameRoomCfg : open_poker_cark_entrance = " << open_poker_cark_entrance << endl;
        std::cout << "GameRoomCfg : current_chess_flag = " << current_chess_flag << endl;
        std::cout << "GameRoomCfg : new_chess_flag = " << new_chess_flag << endl;
        std::cout << "GameRoomCfg : open_chess_entrance = " << open_chess_entrance << endl;
    }
};

struct NoGoldFreeGive
{
    unsigned int gold;
    unsigned int seconds;

    NoGoldFreeGive() {};

    NoGoldFreeGive(DOMNode* parent)
    {
        gold = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "gold"));
        seconds = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "seconds"));
    }

    void output() const
    {
        std::cout << "NoGoldFreeGive : gold = " << gold << endl;
        std::cout << "NoGoldFreeGive : seconds = " << seconds << endl;
    }
};

struct FriendDonateGold
{
    int min;
    int max;
    int day_times;
    int day_count;
    int day_accept;
    int show_count;

    FriendDonateGold() {};

    FriendDonateGold(DOMNode* parent)
    {
        min = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "min"));
        max = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max"));
        day_times = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "day_times"));
        day_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "day_count"));
        day_accept = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "day_accept"));
        show_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "show_count"));
    }

    void output() const
    {
        std::cout << "FriendDonateGold : min = " << min << endl;
        std::cout << "FriendDonateGold : max = " << max << endl;
        std::cout << "FriendDonateGold : day_times = " << day_times << endl;
        std::cout << "FriendDonateGold : day_count = " << day_count << endl;
        std::cout << "FriendDonateGold : day_accept = " << day_accept << endl;
        std::cout << "FriendDonateGold : show_count = " << show_count << endl;
    }
};

struct login_reward
{
    int type;
    int num;
    int special;

    login_reward() {};

    login_reward(DOMNode* parent)
    {
        type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "type"));
        num = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "num"));
        special = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "special"));
    }

    void output() const
    {
        std::cout << "login_reward : type = " << type << endl;
        std::cout << "login_reward : num = " << num << endl;
        std::cout << "login_reward : special = " << special << endl;
    }
};

struct OtherGameSwitch
{
    int open_game;
    unordered_map<unsigned int, unsigned int> open_platform;
    unordered_map<unsigned int, unsigned int> open_chess;
    unordered_map<unsigned int, unsigned int> open_poker_cark;

    OtherGameSwitch() {};

    OtherGameSwitch(DOMNode* parent)
    {
        open_game = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "open_game"));
        
        open_platform.clear();
        DomNodeArray _open_platform;
        CXmlConfig::getNode(parent, _open_platform, "open_platform", "value", "unsigned int");
        for (unsigned int i = 0; i < _open_platform.size(); ++i)
        {
            open_platform[CXmlConfig::stringToInt(CXmlConfig::getKey(_open_platform[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_open_platform[i], "value"));
        }
        
        open_chess.clear();
        DomNodeArray _open_chess;
        CXmlConfig::getNode(parent, _open_chess, "open_chess", "value", "unsigned int");
        for (unsigned int i = 0; i < _open_chess.size(); ++i)
        {
            open_chess[CXmlConfig::stringToInt(CXmlConfig::getKey(_open_chess[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_open_chess[i], "value"));
        }
        
        open_poker_cark.clear();
        DomNodeArray _open_poker_cark;
        CXmlConfig::getNode(parent, _open_poker_cark, "open_poker_cark", "value", "unsigned int");
        for (unsigned int i = 0; i < _open_poker_cark.size(); ++i)
        {
            open_poker_cark[CXmlConfig::stringToInt(CXmlConfig::getKey(_open_poker_cark[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_open_poker_cark[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "OtherGameSwitch : open_game = " << open_game << endl;
        std::cout << "---------- OtherGameSwitch : open_platform ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = open_platform.begin(); it != open_platform.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== OtherGameSwitch : open_platform ==========\n" << endl;
        std::cout << "---------- OtherGameSwitch : open_chess ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = open_chess.begin(); it != open_chess.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== OtherGameSwitch : open_chess ==========\n" << endl;
        std::cout << "---------- OtherGameSwitch : open_poker_cark ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = open_poker_cark.begin(); it != open_poker_cark.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== OtherGameSwitch : open_poker_cark ==========\n" << endl;
    }
};

struct CommonCfg
{
    string scores_clear_start_time;
    unsigned int scores_clear_septum_day;
    unsigned int statistics_interval_secs;

    CommonCfg() {};

    CommonCfg(DOMNode* parent)
    {
        scores_clear_start_time = CXmlConfig::getValue(parent, "scores_clear_start_time");
        scores_clear_septum_day = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "scores_clear_septum_day"));
        statistics_interval_secs = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "statistics_interval_secs"));
    }

    void output() const
    {
        std::cout << "CommonCfg : scores_clear_start_time = " << scores_clear_start_time << endl;
        std::cout << "CommonCfg : scores_clear_septum_day = " << scores_clear_septum_day << endl;
        std::cout << "CommonCfg : statistics_interval_secs = " << statistics_interval_secs << endl;
    }
};

struct config : public IXmlConfigBase
{
    CommonCfg common_cfg;
    OtherGameSwitch other_game_cfg;
    vector<login_reward> login_reward_list;
    FriendDonateGold friend_donate_gold;
    unordered_map<int, NoGoldFreeGive> nogold_free_give;
    unordered_map<int, NoGoldFreeGive> vip_nogold_free_give;
    map<unsigned int, unsigned int> show_fish_rate;
    unordered_map<unsigned int, unsigned int> over_seas_screen_notify;
    unordered_map<unsigned int, unsigned int> over_seas_banner_notify;
    GameRoomCfg game_room_cfg;
    map<int, client_version> apple_version_list;
    map<int, client_version> android_version_list;
    map<int, client_version> android_merge_version;
    map<int, HotUpdateVersion> android_merge_hot_update;
    map<int, VipItem> vip_info_list;
    HallNotice hall_notice;
    HallShare hall_share;
    NewbieGiftActivity newbie_gift_activity;
    RechargeLotteryActivity recharge_lottery_activity;
    PkPlayInfo pk_play_info;
    map<int, RewardItem> pk_play_ranking;
    RedGiftWord red_gift_word;
    Knapsack knapsack_cfg;
    vector<CfgJoinMatch> pk_join_match_list;
    vector<CfgWinMatch> pk_win_match_list;
    vector<CfgOnlineTask> pk_online_task;
    ArenaCfg arena_base_cfg;
    map<int, Arena> arena_cfg_map;
    WinRedPacketActivityCfg win_red_packet_base_cfg;
    WinRedPacketActivity win_red_packet_activity;
    CatchGiftActivityCfg catch_gift_activity_cfg;

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
        DomNodeArray _common_cfg;
        CXmlConfig::getNode(parent, "param", "common_cfg", _common_cfg);
        if (_common_cfg.size() > 0) common_cfg = CommonCfg(_common_cfg[0]);
        
        DomNodeArray _other_game_cfg;
        CXmlConfig::getNode(parent, "param", "other_game_cfg", _other_game_cfg);
        if (_other_game_cfg.size() > 0) other_game_cfg = OtherGameSwitch(_other_game_cfg[0]);
        
        login_reward_list.clear();
        DomNodeArray _login_reward_list;
        CXmlConfig::getNode(parent, _login_reward_list, "login_reward_list", "login_reward");
        for (unsigned int i = 0; i < _login_reward_list.size(); ++i)
        {
            login_reward_list.push_back(login_reward(_login_reward_list[i]));
        }
        
        DomNodeArray _friend_donate_gold;
        CXmlConfig::getNode(parent, "param", "friend_donate_gold", _friend_donate_gold);
        if (_friend_donate_gold.size() > 0) friend_donate_gold = FriendDonateGold(_friend_donate_gold[0]);
        
        nogold_free_give.clear();
        DomNodeArray _nogold_free_give;
        CXmlConfig::getNode(parent, _nogold_free_give, "nogold_free_give", "NoGoldFreeGive");
        for (unsigned int i = 0; i < _nogold_free_give.size(); ++i)
        {
            nogold_free_give[CXmlConfig::stringToInt(CXmlConfig::getKey(_nogold_free_give[i], "key"))] = NoGoldFreeGive(_nogold_free_give[i]);
        }
        
        vip_nogold_free_give.clear();
        DomNodeArray _vip_nogold_free_give;
        CXmlConfig::getNode(parent, _vip_nogold_free_give, "vip_nogold_free_give", "NoGoldFreeGive");
        for (unsigned int i = 0; i < _vip_nogold_free_give.size(); ++i)
        {
            vip_nogold_free_give[CXmlConfig::stringToInt(CXmlConfig::getKey(_vip_nogold_free_give[i], "key"))] = NoGoldFreeGive(_vip_nogold_free_give[i]);
        }
        
        show_fish_rate.clear();
        DomNodeArray _show_fish_rate;
        CXmlConfig::getNode(parent, _show_fish_rate, "show_fish_rate", "value", "unsigned int");
        for (unsigned int i = 0; i < _show_fish_rate.size(); ++i)
        {
            show_fish_rate[CXmlConfig::stringToInt(CXmlConfig::getKey(_show_fish_rate[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_show_fish_rate[i], "value"));
        }
        
        over_seas_screen_notify.clear();
        DomNodeArray _over_seas_screen_notify;
        CXmlConfig::getNode(parent, _over_seas_screen_notify, "over_seas_screen_notify", "value", "unsigned int");
        for (unsigned int i = 0; i < _over_seas_screen_notify.size(); ++i)
        {
            over_seas_screen_notify[CXmlConfig::stringToInt(CXmlConfig::getKey(_over_seas_screen_notify[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_over_seas_screen_notify[i], "value"));
        }
        
        over_seas_banner_notify.clear();
        DomNodeArray _over_seas_banner_notify;
        CXmlConfig::getNode(parent, _over_seas_banner_notify, "over_seas_banner_notify", "value", "unsigned int");
        for (unsigned int i = 0; i < _over_seas_banner_notify.size(); ++i)
        {
            over_seas_banner_notify[CXmlConfig::stringToInt(CXmlConfig::getKey(_over_seas_banner_notify[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_over_seas_banner_notify[i], "value"));
        }
        
        DomNodeArray _game_room_cfg;
        CXmlConfig::getNode(parent, "param", "game_room_cfg", _game_room_cfg);
        if (_game_room_cfg.size() > 0) game_room_cfg = GameRoomCfg(_game_room_cfg[0]);
        
        apple_version_list.clear();
        DomNodeArray _apple_version_list;
        CXmlConfig::getNode(parent, _apple_version_list, "apple_version_list", "client_version");
        for (unsigned int i = 0; i < _apple_version_list.size(); ++i)
        {
            apple_version_list[CXmlConfig::stringToInt(CXmlConfig::getKey(_apple_version_list[i], "key"))] = client_version(_apple_version_list[i]);
        }
        
        android_version_list.clear();
        DomNodeArray _android_version_list;
        CXmlConfig::getNode(parent, _android_version_list, "android_version_list", "client_version");
        for (unsigned int i = 0; i < _android_version_list.size(); ++i)
        {
            android_version_list[CXmlConfig::stringToInt(CXmlConfig::getKey(_android_version_list[i], "key"))] = client_version(_android_version_list[i]);
        }
        
        android_merge_version.clear();
        DomNodeArray _android_merge_version;
        CXmlConfig::getNode(parent, _android_merge_version, "android_merge_version", "client_version");
        for (unsigned int i = 0; i < _android_merge_version.size(); ++i)
        {
            android_merge_version[CXmlConfig::stringToInt(CXmlConfig::getKey(_android_merge_version[i], "key"))] = client_version(_android_merge_version[i]);
        }
        
        android_merge_hot_update.clear();
        DomNodeArray _android_merge_hot_update;
        CXmlConfig::getNode(parent, _android_merge_hot_update, "android_merge_hot_update", "HotUpdateVersion");
        for (unsigned int i = 0; i < _android_merge_hot_update.size(); ++i)
        {
            android_merge_hot_update[CXmlConfig::stringToInt(CXmlConfig::getKey(_android_merge_hot_update[i], "key"))] = HotUpdateVersion(_android_merge_hot_update[i]);
        }
        
        vip_info_list.clear();
        DomNodeArray _vip_info_list;
        CXmlConfig::getNode(parent, _vip_info_list, "vip_info_list", "VipItem");
        for (unsigned int i = 0; i < _vip_info_list.size(); ++i)
        {
            vip_info_list[CXmlConfig::stringToInt(CXmlConfig::getKey(_vip_info_list[i], "key"))] = VipItem(_vip_info_list[i]);
        }
        
        DomNodeArray _hall_notice;
        CXmlConfig::getNode(parent, "param", "hall_notice", _hall_notice);
        if (_hall_notice.size() > 0) hall_notice = HallNotice(_hall_notice[0]);
        
        DomNodeArray _hall_share;
        CXmlConfig::getNode(parent, "param", "hall_share", _hall_share);
        if (_hall_share.size() > 0) hall_share = HallShare(_hall_share[0]);
        
        DomNodeArray _newbie_gift_activity;
        CXmlConfig::getNode(parent, "param", "newbie_gift_activity", _newbie_gift_activity);
        if (_newbie_gift_activity.size() > 0) newbie_gift_activity = NewbieGiftActivity(_newbie_gift_activity[0]);
        
        DomNodeArray _recharge_lottery_activity;
        CXmlConfig::getNode(parent, "param", "recharge_lottery_activity", _recharge_lottery_activity);
        if (_recharge_lottery_activity.size() > 0) recharge_lottery_activity = RechargeLotteryActivity(_recharge_lottery_activity[0]);
        
        DomNodeArray _pk_play_info;
        CXmlConfig::getNode(parent, "param", "pk_play_info", _pk_play_info);
        if (_pk_play_info.size() > 0) pk_play_info = PkPlayInfo(_pk_play_info[0]);
        
        pk_play_ranking.clear();
        DomNodeArray _pk_play_ranking;
        CXmlConfig::getNode(parent, _pk_play_ranking, "pk_play_ranking", "RewardItem");
        for (unsigned int i = 0; i < _pk_play_ranking.size(); ++i)
        {
            pk_play_ranking[CXmlConfig::stringToInt(CXmlConfig::getKey(_pk_play_ranking[i], "key"))] = RewardItem(_pk_play_ranking[i]);
        }
        
        DomNodeArray _red_gift_word;
        CXmlConfig::getNode(parent, "param", "red_gift_word", _red_gift_word);
        if (_red_gift_word.size() > 0) red_gift_word = RedGiftWord(_red_gift_word[0]);
        
        DomNodeArray _knapsack_cfg;
        CXmlConfig::getNode(parent, "param", "knapsack_cfg", _knapsack_cfg);
        if (_knapsack_cfg.size() > 0) knapsack_cfg = Knapsack(_knapsack_cfg[0]);
        
        pk_join_match_list.clear();
        DomNodeArray _pk_join_match_list;
        CXmlConfig::getNode(parent, _pk_join_match_list, "pk_join_match_list", "CfgJoinMatch");
        for (unsigned int i = 0; i < _pk_join_match_list.size(); ++i)
        {
            pk_join_match_list.push_back(CfgJoinMatch(_pk_join_match_list[i]));
        }
        
        pk_win_match_list.clear();
        DomNodeArray _pk_win_match_list;
        CXmlConfig::getNode(parent, _pk_win_match_list, "pk_win_match_list", "CfgWinMatch");
        for (unsigned int i = 0; i < _pk_win_match_list.size(); ++i)
        {
            pk_win_match_list.push_back(CfgWinMatch(_pk_win_match_list[i]));
        }
        
        pk_online_task.clear();
        DomNodeArray _pk_online_task;
        CXmlConfig::getNode(parent, _pk_online_task, "pk_online_task", "CfgOnlineTask");
        for (unsigned int i = 0; i < _pk_online_task.size(); ++i)
        {
            pk_online_task.push_back(CfgOnlineTask(_pk_online_task[i]));
        }
        
        DomNodeArray _arena_base_cfg;
        CXmlConfig::getNode(parent, "param", "arena_base_cfg", _arena_base_cfg);
        if (_arena_base_cfg.size() > 0) arena_base_cfg = ArenaCfg(_arena_base_cfg[0]);
        
        arena_cfg_map.clear();
        DomNodeArray _arena_cfg_map;
        CXmlConfig::getNode(parent, _arena_cfg_map, "arena_cfg_map", "Arena");
        for (unsigned int i = 0; i < _arena_cfg_map.size(); ++i)
        {
            arena_cfg_map[CXmlConfig::stringToInt(CXmlConfig::getKey(_arena_cfg_map[i], "key"))] = Arena(_arena_cfg_map[i]);
        }
        
        DomNodeArray _win_red_packet_base_cfg;
        CXmlConfig::getNode(parent, "param", "win_red_packet_base_cfg", _win_red_packet_base_cfg);
        if (_win_red_packet_base_cfg.size() > 0) win_red_packet_base_cfg = WinRedPacketActivityCfg(_win_red_packet_base_cfg[0]);
        
        DomNodeArray _win_red_packet_activity;
        CXmlConfig::getNode(parent, "param", "win_red_packet_activity", _win_red_packet_activity);
        if (_win_red_packet_activity.size() > 0) win_red_packet_activity = WinRedPacketActivity(_win_red_packet_activity[0]);
        
        DomNodeArray _catch_gift_activity_cfg;
        CXmlConfig::getNode(parent, "param", "catch_gift_activity_cfg", _catch_gift_activity_cfg);
        if (_catch_gift_activity_cfg.size() > 0) catch_gift_activity_cfg = CatchGiftActivityCfg(_catch_gift_activity_cfg[0]);
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- config : common_cfg ----------" << endl;
        common_cfg.output();
        std::cout << "========== config : common_cfg ==========\n" << endl;
        std::cout << "---------- config : other_game_cfg ----------" << endl;
        other_game_cfg.output();
        std::cout << "========== config : other_game_cfg ==========\n" << endl;
        std::cout << "---------- config : login_reward_list ----------" << endl;
        for (unsigned int i = 0; i < login_reward_list.size(); ++i)
        {
            login_reward_list[i].output();
            if (i < (login_reward_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : login_reward_list ==========\n" << endl;
        std::cout << "---------- config : friend_donate_gold ----------" << endl;
        friend_donate_gold.output();
        std::cout << "========== config : friend_donate_gold ==========\n" << endl;
        std::cout << "---------- config : nogold_free_give ----------" << endl;
        unsigned int _nogold_free_give_count_ = 0;
        for (unordered_map<int, NoGoldFreeGive>::const_iterator it = nogold_free_give.begin(); it != nogold_free_give.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_nogold_free_give_count_++ < (nogold_free_give.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : nogold_free_give ==========\n" << endl;
        std::cout << "---------- config : vip_nogold_free_give ----------" << endl;
        unsigned int _vip_nogold_free_give_count_ = 0;
        for (unordered_map<int, NoGoldFreeGive>::const_iterator it = vip_nogold_free_give.begin(); it != vip_nogold_free_give.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_vip_nogold_free_give_count_++ < (vip_nogold_free_give.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : vip_nogold_free_give ==========\n" << endl;
        std::cout << "---------- config : show_fish_rate ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = show_fish_rate.begin(); it != show_fish_rate.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== config : show_fish_rate ==========\n" << endl;
        std::cout << "---------- config : over_seas_screen_notify ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = over_seas_screen_notify.begin(); it != over_seas_screen_notify.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== config : over_seas_screen_notify ==========\n" << endl;
        std::cout << "---------- config : over_seas_banner_notify ----------" << endl;
        for (unordered_map<unsigned int, unsigned int>::const_iterator it = over_seas_banner_notify.begin(); it != over_seas_banner_notify.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== config : over_seas_banner_notify ==========\n" << endl;
        std::cout << "---------- config : game_room_cfg ----------" << endl;
        game_room_cfg.output();
        std::cout << "========== config : game_room_cfg ==========\n" << endl;
        std::cout << "---------- config : apple_version_list ----------" << endl;
        unsigned int _apple_version_list_count_ = 0;
        for (map<int, client_version>::const_iterator it = apple_version_list.begin(); it != apple_version_list.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_apple_version_list_count_++ < (apple_version_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : apple_version_list ==========\n" << endl;
        std::cout << "---------- config : android_version_list ----------" << endl;
        unsigned int _android_version_list_count_ = 0;
        for (map<int, client_version>::const_iterator it = android_version_list.begin(); it != android_version_list.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_android_version_list_count_++ < (android_version_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : android_version_list ==========\n" << endl;
        std::cout << "---------- config : android_merge_version ----------" << endl;
        unsigned int _android_merge_version_count_ = 0;
        for (map<int, client_version>::const_iterator it = android_merge_version.begin(); it != android_merge_version.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_android_merge_version_count_++ < (android_merge_version.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : android_merge_version ==========\n" << endl;
        std::cout << "---------- config : android_merge_hot_update ----------" << endl;
        unsigned int _android_merge_hot_update_count_ = 0;
        for (map<int, HotUpdateVersion>::const_iterator it = android_merge_hot_update.begin(); it != android_merge_hot_update.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_android_merge_hot_update_count_++ < (android_merge_hot_update.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : android_merge_hot_update ==========\n" << endl;
        std::cout << "---------- config : vip_info_list ----------" << endl;
        unsigned int _vip_info_list_count_ = 0;
        for (map<int, VipItem>::const_iterator it = vip_info_list.begin(); it != vip_info_list.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_vip_info_list_count_++ < (vip_info_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : vip_info_list ==========\n" << endl;
        std::cout << "---------- config : hall_notice ----------" << endl;
        hall_notice.output();
        std::cout << "========== config : hall_notice ==========\n" << endl;
        std::cout << "---------- config : hall_share ----------" << endl;
        hall_share.output();
        std::cout << "========== config : hall_share ==========\n" << endl;
        std::cout << "---------- config : newbie_gift_activity ----------" << endl;
        newbie_gift_activity.output();
        std::cout << "========== config : newbie_gift_activity ==========\n" << endl;
        std::cout << "---------- config : recharge_lottery_activity ----------" << endl;
        recharge_lottery_activity.output();
        std::cout << "========== config : recharge_lottery_activity ==========\n" << endl;
        std::cout << "---------- config : pk_play_info ----------" << endl;
        pk_play_info.output();
        std::cout << "========== config : pk_play_info ==========\n" << endl;
        std::cout << "---------- config : pk_play_ranking ----------" << endl;
        unsigned int _pk_play_ranking_count_ = 0;
        for (map<int, RewardItem>::const_iterator it = pk_play_ranking.begin(); it != pk_play_ranking.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_pk_play_ranking_count_++ < (pk_play_ranking.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : pk_play_ranking ==========\n" << endl;
        std::cout << "---------- config : red_gift_word ----------" << endl;
        red_gift_word.output();
        std::cout << "========== config : red_gift_word ==========\n" << endl;
        std::cout << "---------- config : knapsack_cfg ----------" << endl;
        knapsack_cfg.output();
        std::cout << "========== config : knapsack_cfg ==========\n" << endl;
        std::cout << "---------- config : pk_join_match_list ----------" << endl;
        for (unsigned int i = 0; i < pk_join_match_list.size(); ++i)
        {
            pk_join_match_list[i].output();
            if (i < (pk_join_match_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : pk_join_match_list ==========\n" << endl;
        std::cout << "---------- config : pk_win_match_list ----------" << endl;
        for (unsigned int i = 0; i < pk_win_match_list.size(); ++i)
        {
            pk_win_match_list[i].output();
            if (i < (pk_win_match_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : pk_win_match_list ==========\n" << endl;
        std::cout << "---------- config : pk_online_task ----------" << endl;
        for (unsigned int i = 0; i < pk_online_task.size(); ++i)
        {
            pk_online_task[i].output();
            if (i < (pk_online_task.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : pk_online_task ==========\n" << endl;
        std::cout << "---------- config : arena_base_cfg ----------" << endl;
        arena_base_cfg.output();
        std::cout << "========== config : arena_base_cfg ==========\n" << endl;
        std::cout << "---------- config : arena_cfg_map ----------" << endl;
        unsigned int _arena_cfg_map_count_ = 0;
        for (map<int, Arena>::const_iterator it = arena_cfg_map.begin(); it != arena_cfg_map.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_arena_cfg_map_count_++ < (arena_cfg_map.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : arena_cfg_map ==========\n" << endl;
        std::cout << "---------- config : win_red_packet_base_cfg ----------" << endl;
        win_red_packet_base_cfg.output();
        std::cout << "========== config : win_red_packet_base_cfg ==========\n" << endl;
        std::cout << "---------- config : win_red_packet_activity ----------" << endl;
        win_red_packet_activity.output();
        std::cout << "========== config : win_red_packet_activity ==========\n" << endl;
        std::cout << "---------- config : catch_gift_activity_cfg ----------" << endl;
        catch_gift_activity_cfg.output();
        std::cout << "========== config : catch_gift_activity_cfg ==========\n" << endl;
    }
};

}

#endif  // __HALLCONFIGDATA_H__

