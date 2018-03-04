#ifndef __DBCONFIG_H__
#define __DBCONFIG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace DBConfig
{

struct RedisDBCfg
{
    string center_db_ip;
    int center_db_port;
    int center_db_timeout;
    string logic_db_ip;
    int logic_db_port;
    int logic_db_timeout;

    RedisDBCfg() {};

    RedisDBCfg(DOMNode* parent)
    {
        center_db_ip = CXmlConfig::getValue(parent, "center_db_ip");
        center_db_port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "center_db_port"));
        center_db_timeout = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "center_db_timeout"));
        logic_db_ip = CXmlConfig::getValue(parent, "logic_db_ip");
        logic_db_port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "logic_db_port"));
        logic_db_timeout = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "logic_db_timeout"));
    }

    void output() const
    {
        std::cout << "RedisDBCfg : center_db_ip = " << center_db_ip << endl;
        std::cout << "RedisDBCfg : center_db_port = " << center_db_port << endl;
        std::cout << "RedisDBCfg : center_db_timeout = " << center_db_timeout << endl;
        std::cout << "RedisDBCfg : logic_db_ip = " << logic_db_ip << endl;
        std::cout << "RedisDBCfg : logic_db_port = " << logic_db_port << endl;
        std::cout << "RedisDBCfg : logic_db_timeout = " << logic_db_timeout << endl;
    }
};

struct MysqlRecordDBCfg
{
    string ip;
    int port;
    string username;
    string password;
    string dbname;
    unsigned int db_table_count;
    unsigned int need_commit_count;
    unsigned int commit_time_gap;

    MysqlRecordDBCfg() {};

    MysqlRecordDBCfg(DOMNode* parent)
    {
        ip = CXmlConfig::getValue(parent, "ip");
        port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "port"));
        username = CXmlConfig::getValue(parent, "username");
        password = CXmlConfig::getValue(parent, "password");
        dbname = CXmlConfig::getValue(parent, "dbname");
        db_table_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "db_table_count"));
        need_commit_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "need_commit_count"));
        commit_time_gap = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "commit_time_gap"));
    }

    void output() const
    {
        std::cout << "MysqlRecordDBCfg : ip = " << ip << endl;
        std::cout << "MysqlRecordDBCfg : port = " << port << endl;
        std::cout << "MysqlRecordDBCfg : username = " << username << endl;
        std::cout << "MysqlRecordDBCfg : password = " << password << endl;
        std::cout << "MysqlRecordDBCfg : dbname = " << dbname << endl;
        std::cout << "MysqlRecordDBCfg : db_table_count = " << db_table_count << endl;
        std::cout << "MysqlRecordDBCfg : need_commit_count = " << need_commit_count << endl;
        std::cout << "MysqlRecordDBCfg : commit_time_gap = " << commit_time_gap << endl;
    }
};

struct MysqlOptDBCfg
{
    string ip;
    int port;
    string username;
    string password;
    string dbname;

    MysqlOptDBCfg() {};

    MysqlOptDBCfg(DOMNode* parent)
    {
        ip = CXmlConfig::getValue(parent, "ip");
        port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "port"));
        username = CXmlConfig::getValue(parent, "username");
        password = CXmlConfig::getValue(parent, "password");
        dbname = CXmlConfig::getValue(parent, "dbname");
    }

    void output() const
    {
        std::cout << "MysqlOptDBCfg : ip = " << ip << endl;
        std::cout << "MysqlOptDBCfg : port = " << port << endl;
        std::cout << "MysqlOptDBCfg : username = " << username << endl;
        std::cout << "MysqlOptDBCfg : password = " << password << endl;
        std::cout << "MysqlOptDBCfg : dbname = " << dbname << endl;
    }
};

struct MysqlLogicDBCfg
{
    string ip;
    int port;
    string username;
    string password;
    string dbname;
    unsigned int commit_need_count;
    unsigned int commit_need_time;
    unsigned int max_commit_count;
    unsigned int check_time_gap;
    unsigned int db_connect_check_time_gap;
    unsigned int db_connect_check_times;
    string memcached_ip;
    int memcached_port;

    MysqlLogicDBCfg() {};

    MysqlLogicDBCfg(DOMNode* parent)
    {
        ip = CXmlConfig::getValue(parent, "ip");
        port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "port"));
        username = CXmlConfig::getValue(parent, "username");
        password = CXmlConfig::getValue(parent, "password");
        dbname = CXmlConfig::getValue(parent, "dbname");
        commit_need_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "commit_need_count"));
        commit_need_time = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "commit_need_time"));
        max_commit_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "max_commit_count"));
        check_time_gap = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "check_time_gap"));
        db_connect_check_time_gap = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "db_connect_check_time_gap"));
        db_connect_check_times = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "db_connect_check_times"));
        memcached_ip = CXmlConfig::getValue(parent, "memcached_ip");
        memcached_port = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "memcached_port"));
    }

    void output() const
    {
        std::cout << "MysqlLogicDBCfg : ip = " << ip << endl;
        std::cout << "MysqlLogicDBCfg : port = " << port << endl;
        std::cout << "MysqlLogicDBCfg : username = " << username << endl;
        std::cout << "MysqlLogicDBCfg : password = " << password << endl;
        std::cout << "MysqlLogicDBCfg : dbname = " << dbname << endl;
        std::cout << "MysqlLogicDBCfg : commit_need_count = " << commit_need_count << endl;
        std::cout << "MysqlLogicDBCfg : commit_need_time = " << commit_need_time << endl;
        std::cout << "MysqlLogicDBCfg : max_commit_count = " << max_commit_count << endl;
        std::cout << "MysqlLogicDBCfg : check_time_gap = " << check_time_gap << endl;
        std::cout << "MysqlLogicDBCfg : db_connect_check_time_gap = " << db_connect_check_time_gap << endl;
        std::cout << "MysqlLogicDBCfg : db_connect_check_times = " << db_connect_check_times << endl;
        std::cout << "MysqlLogicDBCfg : memcached_ip = " << memcached_ip << endl;
        std::cout << "MysqlLogicDBCfg : memcached_port = " << memcached_port << endl;
    }
};

struct config : public IXmlConfigBase
{
    MysqlLogicDBCfg logic_db_cfg;
    MysqlOptDBCfg opt_db_cfg;
    MysqlRecordDBCfg record_db_cfg;
    RedisDBCfg redis_db_cfg;

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
        DomNodeArray _logic_db_cfg;
        CXmlConfig::getNode(parent, "param", "logic_db_cfg", _logic_db_cfg);
        if (_logic_db_cfg.size() > 0) logic_db_cfg = MysqlLogicDBCfg(_logic_db_cfg[0]);
        
        DomNodeArray _opt_db_cfg;
        CXmlConfig::getNode(parent, "param", "opt_db_cfg", _opt_db_cfg);
        if (_opt_db_cfg.size() > 0) opt_db_cfg = MysqlOptDBCfg(_opt_db_cfg[0]);
        
        DomNodeArray _record_db_cfg;
        CXmlConfig::getNode(parent, "param", "record_db_cfg", _record_db_cfg);
        if (_record_db_cfg.size() > 0) record_db_cfg = MysqlRecordDBCfg(_record_db_cfg[0]);
        
        DomNodeArray _redis_db_cfg;
        CXmlConfig::getNode(parent, "param", "redis_db_cfg", _redis_db_cfg);
        if (_redis_db_cfg.size() > 0) redis_db_cfg = RedisDBCfg(_redis_db_cfg[0]);
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- config : logic_db_cfg ----------" << endl;
        logic_db_cfg.output();
        std::cout << "========== config : logic_db_cfg ==========\n" << endl;
        std::cout << "---------- config : opt_db_cfg ----------" << endl;
        opt_db_cfg.output();
        std::cout << "========== config : opt_db_cfg ==========\n" << endl;
        std::cout << "---------- config : record_db_cfg ----------" << endl;
        record_db_cfg.output();
        std::cout << "========== config : record_db_cfg ==========\n" << endl;
        std::cout << "---------- config : redis_db_cfg ----------" << endl;
        redis_db_cfg.output();
        std::cout << "========== config : redis_db_cfg ==========\n" << endl;
    }
};

}

#endif  // __DBCONFIG_H__

