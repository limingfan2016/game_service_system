#ifndef __MSGCENTERCFG_H__
#define __MSGCENTERCFG_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include "base/xmlConfig.h"


using namespace std;
using namespace NXmlConfig;


namespace MsgCenterCfg
{

struct Message
{
    string title;
    string content;

    Message() {};

    Message(DOMNode* parent)
    {
        title = CXmlConfig::getValue(parent, "title");
        content = CXmlConfig::getValue(parent, "content");
    }

    void output() const
    {
        std::cout << "Message : title = " << title << endl;
        std::cout << "Message : content = " << content << endl;
    }
};

struct DayMessageCfg
{
    unsigned int switch_value;
    unsigned int count;
    unsigned int interval;
    unsigned int hour;
    unsigned int minute;
    unsigned int second;
    vector<Message> messageArray;

    DayMessageCfg() {};

    DayMessageCfg(DOMNode* parent)
    {
        switch_value = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "switch_value"));
        count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "count"));
        interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "interval"));
        hour = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "hour"));
        minute = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "minute"));
        second = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "second"));
        
        messageArray.clear();
        DomNodeArray _messageArray;
        CXmlConfig::getNode(parent, _messageArray, "messageArray", "Message");
        for (unsigned int i = 0; i < _messageArray.size(); ++i)
        {
            messageArray.push_back(Message(_messageArray[i]));
        }
    }

    void output() const
    {
        std::cout << "DayMessageCfg : switch_value = " << switch_value << endl;
        std::cout << "DayMessageCfg : count = " << count << endl;
        std::cout << "DayMessageCfg : interval = " << interval << endl;
        std::cout << "DayMessageCfg : hour = " << hour << endl;
        std::cout << "DayMessageCfg : minute = " << minute << endl;
        std::cout << "DayMessageCfg : second = " << second << endl;
        std::cout << "---------- DayMessageCfg : messageArray ----------" << endl;
        for (unsigned int i = 0; i < messageArray.size(); ++i)
        {
            messageArray[i].output();
            if (i < (messageArray.size() - 1)) std::cout << endl;
        }
        std::cout << "========== DayMessageCfg : messageArray ==========\n" << endl;
    }
};

struct TimingPushCfg
{
    unsigned int switch_value;
    string time;
    unsigned int count;
    unsigned int interval;
    vector<Message> messageArray;

    TimingPushCfg() {};

    TimingPushCfg(DOMNode* parent)
    {
        switch_value = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "switch_value"));
        time = CXmlConfig::getValue(parent, "time");
        count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "count"));
        interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "interval"));
        
        messageArray.clear();
        DomNodeArray _messageArray;
        CXmlConfig::getNode(parent, _messageArray, "messageArray", "Message");
        for (unsigned int i = 0; i < _messageArray.size(); ++i)
        {
            messageArray.push_back(Message(_messageArray[i]));
        }
    }

    void output() const
    {
        std::cout << "TimingPushCfg : switch_value = " << switch_value << endl;
        std::cout << "TimingPushCfg : time = " << time << endl;
        std::cout << "TimingPushCfg : count = " << count << endl;
        std::cout << "TimingPushCfg : interval = " << interval << endl;
        std::cout << "---------- TimingPushCfg : messageArray ----------" << endl;
        for (unsigned int i = 0; i < messageArray.size(); ++i)
        {
            messageArray[i].output();
            if (i < (messageArray.size() - 1)) std::cout << endl;
        }
        std::cout << "========== TimingPushCfg : messageArray ==========\n" << endl;
    }
};

struct MessageConfig
{
    unsigned int offline_msg_interval;
    unsigned int personal_msg_retain;
    TimingPushCfg timingPushCfg;
    DayMessageCfg dayMsgCfg;

    MessageConfig() {};

    MessageConfig(DOMNode* parent)
    {
        offline_msg_interval = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "offline_msg_interval"));
        personal_msg_retain = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "personal_msg_retain"));
        
        DomNodeArray _timingPushCfg;
        CXmlConfig::getNode(parent, "param", "timingPushCfg", _timingPushCfg);
        if (_timingPushCfg.size() > 0) timingPushCfg = TimingPushCfg(_timingPushCfg[0]);
        
        DomNodeArray _dayMsgCfg;
        CXmlConfig::getNode(parent, "param", "dayMsgCfg", _dayMsgCfg);
        if (_dayMsgCfg.size() > 0) dayMsgCfg = DayMessageCfg(_dayMsgCfg[0]);
    }

    void output() const
    {
        std::cout << "MessageConfig : offline_msg_interval = " << offline_msg_interval << endl;
        std::cout << "MessageConfig : personal_msg_retain = " << personal_msg_retain << endl;
        std::cout << "---------- MessageConfig : timingPushCfg ----------" << endl;
        timingPushCfg.output();
        std::cout << "========== MessageConfig : timingPushCfg ==========\n" << endl;
        std::cout << "---------- MessageConfig : dayMsgCfg ----------" << endl;
        dayMsgCfg.output();
        std::cout << "========== MessageConfig : dayMsgCfg ==========\n" << endl;
    }
};

struct config : public IXmlConfigBase
{
    MessageConfig msgCfg;

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
        DomNodeArray _msgCfg;
        CXmlConfig::getNode(parent, "param", "msgCfg", _msgCfg);
        if (_msgCfg.size() > 0) msgCfg = MessageConfig(_msgCfg[0]);
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- config : msgCfg ----------" << endl;
        msgCfg.output();
        std::cout << "========== config : msgCfg ==========\n" << endl;
    }
};

}

#endif  // __MSGCENTERCFG_H__

