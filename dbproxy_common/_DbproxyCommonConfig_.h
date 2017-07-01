#ifndef __DBPROXYCOMMONCONFIG_H__
#define __DBPROXYCOMMONCONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace DbproxyCommonConfig
{

struct GameRecordConfig
{
    unsigned int game_type;
    unsigned int table_count;
    string mysql_ip;
    int mysql_port;
    string mysql_username;
    string mysql_password;
    string mysql_dbname;

    GameRecordConfig() {};

    GameRecordConfig(DOMNode* parent)
    {
        game_type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "game_type"));
        table_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "table_count"));
        mysql_ip = CXmlConfig::getValue(parent, "mysql_ip");
        mysql_port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "mysql_port"));
        mysql_username = CXmlConfig::getValue(parent, "mysql_username");
        mysql_password = CXmlConfig::getValue(parent, "mysql_password");
        mysql_dbname = CXmlConfig::getValue(parent, "mysql_dbname");
    }

    void output() const
    {
        std::cout << "GameRecordConfig : game_type = " << game_type << endl;
        std::cout << "GameRecordConfig : table_count = " << table_count << endl;
        std::cout << "GameRecordConfig : mysql_ip = " << mysql_ip << endl;
        std::cout << "GameRecordConfig : mysql_port = " << mysql_port << endl;
        std::cout << "GameRecordConfig : mysql_username = " << mysql_username << endl;
        std::cout << "GameRecordConfig : mysql_password = " << mysql_password << endl;
        std::cout << "GameRecordConfig : mysql_dbname = " << mysql_dbname << endl;
    }
};

struct RegisterInit
{
    unsigned int game_gold;
    unsigned int light_cannon_count;
    unsigned int mute_bullet_count;
    unsigned int flower_count;
    unsigned int slipper_count;
    unsigned int suona_count;
    unsigned int auto_bullet_count;
    unsigned int lock_bullet_count;
    unsigned int rampage_count;
    unsigned int dud_shield_count;

    RegisterInit() {};

    RegisterInit(DOMNode* parent)
    {
        game_gold = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "game_gold"));
        light_cannon_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "light_cannon_count"));
        mute_bullet_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "mute_bullet_count"));
        flower_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "flower_count"));
        slipper_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "slipper_count"));
        suona_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "suona_count"));
        auto_bullet_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "auto_bullet_count"));
        lock_bullet_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "lock_bullet_count"));
        rampage_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "rampage_count"));
        dud_shield_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "dud_shield_count"));
    }

    void output() const
    {
        std::cout << "RegisterInit : game_gold = " << game_gold << endl;
        std::cout << "RegisterInit : light_cannon_count = " << light_cannon_count << endl;
        std::cout << "RegisterInit : mute_bullet_count = " << mute_bullet_count << endl;
        std::cout << "RegisterInit : flower_count = " << flower_count << endl;
        std::cout << "RegisterInit : slipper_count = " << slipper_count << endl;
        std::cout << "RegisterInit : suona_count = " << suona_count << endl;
        std::cout << "RegisterInit : auto_bullet_count = " << auto_bullet_count << endl;
        std::cout << "RegisterInit : lock_bullet_count = " << lock_bullet_count << endl;
        std::cout << "RegisterInit : rampage_count = " << rampage_count << endl;
        std::cout << "RegisterInit : dud_shield_count = " << dud_shield_count << endl;
    }
};

struct CommonConfig
{
    unsigned int nickname_length;
    unsigned int mail_message_timeout;
    unsigned int clear_robot_score_date;

    CommonConfig() {};

    CommonConfig(DOMNode* parent)
    {
        nickname_length = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "nickname_length"));
        mail_message_timeout = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "mail_message_timeout"));
        clear_robot_score_date = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "clear_robot_score_date"));
    }

    void output() const
    {
        std::cout << "CommonConfig : nickname_length = " << nickname_length << endl;
        std::cout << "CommonConfig : mail_message_timeout = " << mail_message_timeout << endl;
        std::cout << "CommonConfig : clear_robot_score_date = " << clear_robot_score_date << endl;
    }
};

struct MysqlOptDBCfg
{
    string mysql_ip;
    int mysql_port;
    string mysql_username;
    string mysql_password;
    string mysql_dbname;

    MysqlOptDBCfg() {};

    MysqlOptDBCfg(DOMNode* parent)
    {
        mysql_ip = CXmlConfig::getValue(parent, "mysql_ip");
        mysql_port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "mysql_port"));
        mysql_username = CXmlConfig::getValue(parent, "mysql_username");
        mysql_password = CXmlConfig::getValue(parent, "mysql_password");
        mysql_dbname = CXmlConfig::getValue(parent, "mysql_dbname");
    }

    void output() const
    {
        std::cout << "MysqlOptDBCfg : mysql_ip = " << mysql_ip << endl;
        std::cout << "MysqlOptDBCfg : mysql_port = " << mysql_port << endl;
        std::cout << "MysqlOptDBCfg : mysql_username = " << mysql_username << endl;
        std::cout << "MysqlOptDBCfg : mysql_password = " << mysql_password << endl;
        std::cout << "MysqlOptDBCfg : mysql_dbname = " << mysql_dbname << endl;
    }
};

struct ServerConfig
{
    string mysql_ip;
    int mysql_port;
    string mysql_username;
    string mysql_password;
    string mysql_dbname;
    string memcached_ip;
    int memcached_port;
    unsigned int commit_need_counts;
    unsigned int commit_need_time;
    unsigned int check_time_gap;
    unsigned int max_commit_count_per_second;
    unsigned int db_connect_check_time_gap;
    unsigned int db_connect_check_times;

    ServerConfig() {};

    ServerConfig(DOMNode* parent)
    {
        mysql_ip = CXmlConfig::getValue(parent, "mysql_ip");
        mysql_port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "mysql_port"));
        mysql_username = CXmlConfig::getValue(parent, "mysql_username");
        mysql_password = CXmlConfig::getValue(parent, "mysql_password");
        mysql_dbname = CXmlConfig::getValue(parent, "mysql_dbname");
        memcached_ip = CXmlConfig::getValue(parent, "memcached_ip");
        memcached_port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "memcached_port"));
        commit_need_counts = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "commit_need_counts"));
        commit_need_time = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "commit_need_time"));
        check_time_gap = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "check_time_gap"));
        max_commit_count_per_second = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_commit_count_per_second"));
        db_connect_check_time_gap = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "db_connect_check_time_gap"));
        db_connect_check_times = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "db_connect_check_times"));
    }

    void output() const
    {
        std::cout << "ServerConfig : mysql_ip = " << mysql_ip << endl;
        std::cout << "ServerConfig : mysql_port = " << mysql_port << endl;
        std::cout << "ServerConfig : mysql_username = " << mysql_username << endl;
        std::cout << "ServerConfig : mysql_password = " << mysql_password << endl;
        std::cout << "ServerConfig : mysql_dbname = " << mysql_dbname << endl;
        std::cout << "ServerConfig : memcached_ip = " << memcached_ip << endl;
        std::cout << "ServerConfig : memcached_port = " << memcached_port << endl;
        std::cout << "ServerConfig : commit_need_counts = " << commit_need_counts << endl;
        std::cout << "ServerConfig : commit_need_time = " << commit_need_time << endl;
        std::cout << "ServerConfig : check_time_gap = " << check_time_gap << endl;
        std::cout << "ServerConfig : max_commit_count_per_second = " << max_commit_count_per_second << endl;
        std::cout << "ServerConfig : db_connect_check_time_gap = " << db_connect_check_time_gap << endl;
        std::cout << "ServerConfig : db_connect_check_times = " << db_connect_check_times << endl;
    }
};

struct config : public IXmlConfigBase
{
    ServerConfig server_config;
    MysqlOptDBCfg mysql_opt_db_cfg;
    CommonConfig common_cfg;
    map<unsigned int, unsigned int> aorong_platform_init;
    RegisterInit register_init;
    vector<GameRecordConfig> game_record_config;

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
        DomNodeArray _server_config;
        CXmlConfig::getNode(parent, "param", "server_config", _server_config);
        if (_server_config.size() > 0) server_config = ServerConfig(_server_config[0]);
        
        DomNodeArray _mysql_opt_db_cfg;
        CXmlConfig::getNode(parent, "param", "mysql_opt_db_cfg", _mysql_opt_db_cfg);
        if (_mysql_opt_db_cfg.size() > 0) mysql_opt_db_cfg = MysqlOptDBCfg(_mysql_opt_db_cfg[0]);
        
        DomNodeArray _common_cfg;
        CXmlConfig::getNode(parent, "param", "common_cfg", _common_cfg);
        if (_common_cfg.size() > 0) common_cfg = CommonConfig(_common_cfg[0]);
        
        aorong_platform_init.clear();
        DomNodeArray _aorong_platform_init;
        CXmlConfig::getNode(parent, _aorong_platform_init, "aorong_platform_init", "value", "unsigned int");
        for (unsigned int i = 0; i < _aorong_platform_init.size(); ++i)
        {
            aorong_platform_init[CXmlConfig::stringToInt(CXmlConfig::getKey(_aorong_platform_init[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_aorong_platform_init[i], "value"));
        }
        
        DomNodeArray _register_init;
        CXmlConfig::getNode(parent, "param", "register_init", _register_init);
        if (_register_init.size() > 0) register_init = RegisterInit(_register_init[0]);
        
        game_record_config.clear();
        DomNodeArray _game_record_config;
        CXmlConfig::getNode(parent, _game_record_config, "game_record_config", "GameRecordConfig");
        for (unsigned int i = 0; i < _game_record_config.size(); ++i)
        {
            game_record_config.push_back(GameRecordConfig(_game_record_config[i]));
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- config : server_config ----------" << endl;
        server_config.output();
        std::cout << "========== config : server_config ==========\n" << endl;
        std::cout << "---------- config : mysql_opt_db_cfg ----------" << endl;
        mysql_opt_db_cfg.output();
        std::cout << "========== config : mysql_opt_db_cfg ==========\n" << endl;
        std::cout << "---------- config : common_cfg ----------" << endl;
        common_cfg.output();
        std::cout << "========== config : common_cfg ==========\n" << endl;
        std::cout << "---------- config : aorong_platform_init ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = aorong_platform_init.begin(); it != aorong_platform_init.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== config : aorong_platform_init ==========\n" << endl;
        std::cout << "---------- config : register_init ----------" << endl;
        register_init.output();
        std::cout << "========== config : register_init ==========\n" << endl;
        std::cout << "---------- config : game_record_config ----------" << endl;
        for (unsigned int i = 0; i < game_record_config.size(); ++i)
        {
            game_record_config[i].output();
            if (i < (game_record_config.size() - 1)) std::cout << endl;
        }
        std::cout << "========== config : game_record_config ==========\n" << endl;
    }
};

}

#endif  // __DBPROXYCOMMONCONFIG_H__

