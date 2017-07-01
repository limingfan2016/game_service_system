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

struct chess_gift_package
{
    unsigned int id;
    unsigned int board_id;
    unsigned int pieces_id;
    string picture;
    string name;
    string desc;
    unsigned int num;
    unsigned int buy_price;
    unsigned int flag;
    map<unsigned int, unsigned int> gifts;

    chess_gift_package() {};

    chess_gift_package(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        board_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "board_id"));
        pieces_id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "pieces_id"));
        picture = CXmlConfig::getValue(parent, "picture");
        name = CXmlConfig::getValue(parent, "name");
        desc = CXmlConfig::getValue(parent, "desc");
        num = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "num"));
        buy_price = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "buy_price"));
        flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "flag"));
        
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
        std::cout << "chess_gift_package : id = " << id << endl;
        std::cout << "chess_gift_package : board_id = " << board_id << endl;
        std::cout << "chess_gift_package : pieces_id = " << pieces_id << endl;
        std::cout << "chess_gift_package : picture = " << picture << endl;
        std::cout << "chess_gift_package : name = " << name << endl;
        std::cout << "chess_gift_package : desc = " << desc << endl;
        std::cout << "chess_gift_package : num = " << num << endl;
        std::cout << "chess_gift_package : buy_price = " << buy_price << endl;
        std::cout << "chess_gift_package : flag = " << flag << endl;
        std::cout << "---------- chess_gift_package : gifts ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = gifts.begin(); it != gifts.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== chess_gift_package : gifts ==========\n" << endl;
    }
};

struct chess_goods
{
    unsigned int id;
    string name;
    string desc;
    unsigned int num;
    unsigned int flag;
    map<unsigned int, unsigned int> date_price;

    chess_goods() {};

    chess_goods(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        name = CXmlConfig::getValue(parent, "name");
        desc = CXmlConfig::getValue(parent, "desc");
        num = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "num"));
        flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "flag"));
        
        date_price.clear();
        DomNodeArray _date_price;
        CXmlConfig::getNode(parent, _date_price, "date_price", "value", "unsigned int");
        for (unsigned int i = 0; i < _date_price.size(); ++i)
        {
            date_price[CXmlConfig::stringToInt(CXmlConfig::getKey(_date_price[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_date_price[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "chess_goods : id = " << id << endl;
        std::cout << "chess_goods : name = " << name << endl;
        std::cout << "chess_goods : desc = " << desc << endl;
        std::cout << "chess_goods : num = " << num << endl;
        std::cout << "chess_goods : flag = " << flag << endl;
        std::cout << "---------- chess_goods : date_price ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = date_price.begin(); it != date_price.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== chess_goods : date_price ==========\n" << endl;
    }
};

struct game_goods
{
    unsigned int id;
    unsigned int num;
    unsigned int buy_price;
    unsigned int flag;

    game_goods() {};

    game_goods(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        num = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "num"));
        buy_price = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "buy_price"));
        flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "flag"));
    }

    void output() const
    {
        std::cout << "game_goods : id = " << id << endl;
        std::cout << "game_goods : num = " << num << endl;
        std::cout << "game_goods : buy_price = " << buy_price << endl;
        std::cout << "game_goods : flag = " << flag << endl;
    }
};

struct GiftPackage
{
    unsigned int id;
    unsigned int buy_price;
    unsigned int buy_type;
    map<unsigned int, unsigned int> package_contain;

    GiftPackage() {};

    GiftPackage(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        buy_price = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "buy_price"));
        buy_type = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "buy_type"));
        
        package_contain.clear();
        DomNodeArray _package_contain;
        CXmlConfig::getNode(parent, _package_contain, "package_contain", "value", "unsigned int");
        for (unsigned int i = 0; i < _package_contain.size(); ++i)
        {
            package_contain[CXmlConfig::stringToInt(CXmlConfig::getKey(_package_contain[i], "key"))] = CXmlConfig::stringToInt(CXmlConfig::getValue(_package_contain[i], "value"));
        }
    }

    void output() const
    {
        std::cout << "GiftPackage : id = " << id << endl;
        std::cout << "GiftPackage : buy_price = " << buy_price << endl;
        std::cout << "GiftPackage : buy_type = " << buy_type << endl;
        std::cout << "---------- GiftPackage : package_contain ----------" << endl;
        for (map<unsigned int, unsigned int>::const_iterator it = package_contain.begin(); it != package_contain.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== GiftPackage : package_contain ==========\n" << endl;
    }
};

struct scores_item
{
    unsigned int id;
    unsigned int exchange_count;
    string exchange_desc;
    unsigned int exchange_scores;
    unsigned int get_goods_count;
    string url;
    string describe;

    scores_item() {};

    scores_item(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        exchange_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "exchange_count"));
        exchange_desc = CXmlConfig::getValue(parent, "exchange_desc");
        exchange_scores = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "exchange_scores"));
        get_goods_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "get_goods_count"));
        url = CXmlConfig::getValue(parent, "url");
        describe = CXmlConfig::getValue(parent, "describe");
    }

    void output() const
    {
        std::cout << "scores_item : id = " << id << endl;
        std::cout << "scores_item : exchange_count = " << exchange_count << endl;
        std::cout << "scores_item : exchange_desc = " << exchange_desc << endl;
        std::cout << "scores_item : exchange_scores = " << exchange_scores << endl;
        std::cout << "scores_item : get_goods_count = " << get_goods_count << endl;
        std::cout << "scores_item : url = " << url << endl;
        std::cout << "scores_item : describe = " << describe << endl;
    }
};

struct ScoresShop
{
    unsigned int scores_item_clear_flag;
    vector<scores_item> scores_item_list;

    ScoresShop() {};

    ScoresShop(DOMNode* parent)
    {
        scores_item_clear_flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "scores_item_clear_flag"));
        
        scores_item_list.clear();
        DomNodeArray _scores_item_list;
        CXmlConfig::getNode(parent, _scores_item_list, "scores_item_list", "scores_item");
        for (unsigned int i = 0; i < _scores_item_list.size(); ++i)
        {
            scores_item_list.push_back(scores_item(_scores_item_list[i]));
        }
    }

    void output() const
    {
        std::cout << "ScoresShop : scores_item_clear_flag = " << scores_item_clear_flag << endl;
        std::cout << "---------- ScoresShop : scores_item_list ----------" << endl;
        for (unsigned int i = 0; i < scores_item_list.size(); ++i)
        {
            scores_item_list[i].output();
            if (i < (scores_item_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== ScoresShop : scores_item_list ==========\n" << endl;
    }
};

struct FishCoin
{
    unsigned int id;
    unsigned int num;
    unsigned int gift;
    unsigned int first_recharge_gold;
    double buy_price;
    unsigned int is_used;
    unsigned int flag;
    map<int, string> no_show_platform;

    FishCoin() {};

    FishCoin(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        num = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "num"));
        gift = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "gift"));
        first_recharge_gold = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "first_recharge_gold"));
        buy_price = CXmlConfig::stringToDouble(CXmlConfig::getValue(parent, "buy_price"));
        is_used = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "is_used"));
        flag = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "flag"));
        
        no_show_platform.clear();
        DomNodeArray _no_show_platform;
        CXmlConfig::getNode(parent, _no_show_platform, "no_show_platform", "value", "string");
        for (unsigned int i = 0; i < _no_show_platform.size(); ++i)
        {
            no_show_platform[CXmlConfig::stringToInt(CXmlConfig::getKey(_no_show_platform[i], "key"))] = CXmlConfig::getValue(_no_show_platform[i], "value");
        }
    }

    void output() const
    {
        std::cout << "FishCoin : id = " << id << endl;
        std::cout << "FishCoin : num = " << num << endl;
        std::cout << "FishCoin : gift = " << gift << endl;
        std::cout << "FishCoin : first_recharge_gold = " << first_recharge_gold << endl;
        std::cout << "FishCoin : buy_price = " << buy_price << endl;
        std::cout << "FishCoin : is_used = " << is_used << endl;
        std::cout << "FishCoin : flag = " << flag << endl;
        std::cout << "---------- FishCoin : no_show_platform ----------" << endl;
        for (map<int, string>::const_iterator it = no_show_platform.begin(); it != no_show_platform.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== FishCoin : no_show_platform ==========\n" << endl;
    }
};

struct PlatformRechargeCfg
{
    vector<unsigned int> need_money_vector;

    PlatformRechargeCfg() {};

    PlatformRechargeCfg(DOMNode* parent)
    {
        need_money_vector.clear();
        DomNodeArray _need_money_vector;
        CXmlConfig::getNode(parent, _need_money_vector, "need_money_vector", "value", "unsigned int");
        for (unsigned int i = 0; i < _need_money_vector.size(); ++i)
        {
            need_money_vector.push_back(CXmlConfig::stringToInt(CXmlConfig::getValue(_need_money_vector[i], "value")));
        }
    }

    void output() const
    {
        std::cout << "---------- PlatformRechargeCfg : need_money_vector ----------" << endl;
        for (unsigned int i = 0; i < need_money_vector.size(); ++i)
        {
            std::cout << "value = " << need_money_vector[i] << endl;
        }
        std::cout << "========== PlatformRechargeCfg : need_money_vector ==========\n" << endl;
    }
};

struct PlatformOrderCharingCfg
{
    map<unsigned int, string> goodsId2orderCharingId;

    PlatformOrderCharingCfg() {};

    PlatformOrderCharingCfg(DOMNode* parent)
    {
        goodsId2orderCharingId.clear();
        DomNodeArray _goodsId2orderCharingId;
        CXmlConfig::getNode(parent, _goodsId2orderCharingId, "goodsId2orderCharingId", "value", "string");
        for (unsigned int i = 0; i < _goodsId2orderCharingId.size(); ++i)
        {
            goodsId2orderCharingId[CXmlConfig::stringToInt(CXmlConfig::getKey(_goodsId2orderCharingId[i], "key"))] = CXmlConfig::getValue(_goodsId2orderCharingId[i], "value");
        }
    }

    void output() const
    {
        std::cout << "---------- PlatformOrderCharingCfg : goodsId2orderCharingId ----------" << endl;
        for (map<unsigned int, string>::const_iterator it = goodsId2orderCharingId.begin(); it != goodsId2orderCharingId.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value = " << it->second << endl;
        }
        std::cout << "========== PlatformOrderCharingCfg : goodsId2orderCharingId ==========\n" << endl;
    }
};

struct PhoneFare
{
    unsigned int id;
    unsigned int expense;
    unsigned int achieve;
    unsigned int is_used;

    PhoneFare() {};

    PhoneFare(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        expense = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "expense"));
        achieve = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "achieve"));
        is_used = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "is_used"));
    }

    void output() const
    {
        std::cout << "PhoneFare : id = " << id << endl;
        std::cout << "PhoneFare : expense = " << expense << endl;
        std::cout << "PhoneFare : achieve = " << achieve << endl;
        std::cout << "PhoneFare : is_used = " << is_used << endl;
    }
};

struct goods
{
    unsigned int id;
    unsigned int exchange_count;
    string exchange_desc;
    unsigned int is_used;

    goods() {};

    goods(DOMNode* parent)
    {
        id = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "id"));
        exchange_count = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "exchange_count"));
        exchange_desc = CXmlConfig::getValue(parent, "exchange_desc");
        is_used = CXmlConfig::stringToInt(CXmlConfig::getValue(parent, "is_used"));
    }

    void output() const
    {
        std::cout << "goods : id = " << id << endl;
        std::cout << "goods : exchange_count = " << exchange_count << endl;
        std::cout << "goods : exchange_desc = " << exchange_desc << endl;
        std::cout << "goods : is_used = " << is_used << endl;
    }
};

struct MallData : public IXmlConfigBase
{
    vector<goods> goods_list;
    vector<PhoneFare> phone_fare_list;
    FishCoin first_recharge_package;
    map<unsigned int, PlatformOrderCharingCfg> order_charing_cfg;
    map<unsigned int, PlatformRechargeCfg> special_platform_money_cfg;
    vector<FishCoin> money_list;
    vector<game_goods> fish_coin_list;
    vector<game_goods> diamonds_list;
    vector<game_goods> battery_equipment_list;
    ScoresShop scores_shop;
    GiftPackage gold_package;
    GiftPackage diamonds_package;
    GiftPackage interaction_package;
    GiftPackage wonderful_package;
    GiftPackage pkPlay_package;
    vector<game_goods> ff_chess_game_gold_list;
    vector<chess_goods> ff_chess_board_list;
    vector<chess_goods> ff_chess_pieces_list;
    vector<chess_gift_package> ff_chess_gift_package_list;

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
        goods_list.clear();
        DomNodeArray _goods_list;
        CXmlConfig::getNode(parent, _goods_list, "goods_list", "goods");
        for (unsigned int i = 0; i < _goods_list.size(); ++i)
        {
            goods_list.push_back(goods(_goods_list[i]));
        }
        
        phone_fare_list.clear();
        DomNodeArray _phone_fare_list;
        CXmlConfig::getNode(parent, _phone_fare_list, "phone_fare_list", "PhoneFare");
        for (unsigned int i = 0; i < _phone_fare_list.size(); ++i)
        {
            phone_fare_list.push_back(PhoneFare(_phone_fare_list[i]));
        }
        
        DomNodeArray _first_recharge_package;
        CXmlConfig::getNode(parent, "param", "first_recharge_package", _first_recharge_package);
        if (_first_recharge_package.size() > 0) first_recharge_package = FishCoin(_first_recharge_package[0]);
        
        order_charing_cfg.clear();
        DomNodeArray _order_charing_cfg;
        CXmlConfig::getNode(parent, _order_charing_cfg, "order_charing_cfg", "PlatformOrderCharingCfg");
        for (unsigned int i = 0; i < _order_charing_cfg.size(); ++i)
        {
            order_charing_cfg[CXmlConfig::stringToInt(CXmlConfig::getKey(_order_charing_cfg[i], "key"))] = PlatformOrderCharingCfg(_order_charing_cfg[i]);
        }
        
        special_platform_money_cfg.clear();
        DomNodeArray _special_platform_money_cfg;
        CXmlConfig::getNode(parent, _special_platform_money_cfg, "special_platform_money_cfg", "PlatformRechargeCfg");
        for (unsigned int i = 0; i < _special_platform_money_cfg.size(); ++i)
        {
            special_platform_money_cfg[CXmlConfig::stringToInt(CXmlConfig::getKey(_special_platform_money_cfg[i], "key"))] = PlatformRechargeCfg(_special_platform_money_cfg[i]);
        }
        
        money_list.clear();
        DomNodeArray _money_list;
        CXmlConfig::getNode(parent, _money_list, "money_list", "FishCoin");
        for (unsigned int i = 0; i < _money_list.size(); ++i)
        {
            money_list.push_back(FishCoin(_money_list[i]));
        }
        
        fish_coin_list.clear();
        DomNodeArray _fish_coin_list;
        CXmlConfig::getNode(parent, _fish_coin_list, "fish_coin_list", "game_goods");
        for (unsigned int i = 0; i < _fish_coin_list.size(); ++i)
        {
            fish_coin_list.push_back(game_goods(_fish_coin_list[i]));
        }
        
        diamonds_list.clear();
        DomNodeArray _diamonds_list;
        CXmlConfig::getNode(parent, _diamonds_list, "diamonds_list", "game_goods");
        for (unsigned int i = 0; i < _diamonds_list.size(); ++i)
        {
            diamonds_list.push_back(game_goods(_diamonds_list[i]));
        }
        
        battery_equipment_list.clear();
        DomNodeArray _battery_equipment_list;
        CXmlConfig::getNode(parent, _battery_equipment_list, "battery_equipment_list", "game_goods");
        for (unsigned int i = 0; i < _battery_equipment_list.size(); ++i)
        {
            battery_equipment_list.push_back(game_goods(_battery_equipment_list[i]));
        }
        
        DomNodeArray _scores_shop;
        CXmlConfig::getNode(parent, "param", "scores_shop", _scores_shop);
        if (_scores_shop.size() > 0) scores_shop = ScoresShop(_scores_shop[0]);
        
        DomNodeArray _gold_package;
        CXmlConfig::getNode(parent, "param", "gold_package", _gold_package);
        if (_gold_package.size() > 0) gold_package = GiftPackage(_gold_package[0]);
        
        DomNodeArray _diamonds_package;
        CXmlConfig::getNode(parent, "param", "diamonds_package", _diamonds_package);
        if (_diamonds_package.size() > 0) diamonds_package = GiftPackage(_diamonds_package[0]);
        
        DomNodeArray _interaction_package;
        CXmlConfig::getNode(parent, "param", "interaction_package", _interaction_package);
        if (_interaction_package.size() > 0) interaction_package = GiftPackage(_interaction_package[0]);
        
        DomNodeArray _wonderful_package;
        CXmlConfig::getNode(parent, "param", "wonderful_package", _wonderful_package);
        if (_wonderful_package.size() > 0) wonderful_package = GiftPackage(_wonderful_package[0]);
        
        DomNodeArray _pkPlay_package;
        CXmlConfig::getNode(parent, "param", "pkPlay_package", _pkPlay_package);
        if (_pkPlay_package.size() > 0) pkPlay_package = GiftPackage(_pkPlay_package[0]);
        
        ff_chess_game_gold_list.clear();
        DomNodeArray _ff_chess_game_gold_list;
        CXmlConfig::getNode(parent, _ff_chess_game_gold_list, "ff_chess_game_gold_list", "game_goods");
        for (unsigned int i = 0; i < _ff_chess_game_gold_list.size(); ++i)
        {
            ff_chess_game_gold_list.push_back(game_goods(_ff_chess_game_gold_list[i]));
        }
        
        ff_chess_board_list.clear();
        DomNodeArray _ff_chess_board_list;
        CXmlConfig::getNode(parent, _ff_chess_board_list, "ff_chess_board_list", "chess_goods");
        for (unsigned int i = 0; i < _ff_chess_board_list.size(); ++i)
        {
            ff_chess_board_list.push_back(chess_goods(_ff_chess_board_list[i]));
        }
        
        ff_chess_pieces_list.clear();
        DomNodeArray _ff_chess_pieces_list;
        CXmlConfig::getNode(parent, _ff_chess_pieces_list, "ff_chess_pieces_list", "chess_goods");
        for (unsigned int i = 0; i < _ff_chess_pieces_list.size(); ++i)
        {
            ff_chess_pieces_list.push_back(chess_goods(_ff_chess_pieces_list[i]));
        }
        
        ff_chess_gift_package_list.clear();
        DomNodeArray _ff_chess_gift_package_list;
        CXmlConfig::getNode(parent, _ff_chess_gift_package_list, "ff_chess_gift_package_list", "chess_gift_package");
        for (unsigned int i = 0; i < _ff_chess_gift_package_list.size(); ++i)
        {
            ff_chess_gift_package_list.push_back(chess_gift_package(_ff_chess_gift_package_list[i]));
        }
    }

    void output() const
    {
        const time_t __currentSecs__ = time(NULL);
        const string __currentTime__ = ctime(&__currentSecs__);
        std::cout << "\n!!!!!! set config value result = " << isSetConfigValueSuccess() << " !!!!!! current time = " << __currentTime__;
        std::cout << "---------- MallData : goods_list ----------" << endl;
        for (unsigned int i = 0; i < goods_list.size(); ++i)
        {
            goods_list[i].output();
            if (i < (goods_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : goods_list ==========\n" << endl;
        std::cout << "---------- MallData : phone_fare_list ----------" << endl;
        for (unsigned int i = 0; i < phone_fare_list.size(); ++i)
        {
            phone_fare_list[i].output();
            if (i < (phone_fare_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : phone_fare_list ==========\n" << endl;
        std::cout << "---------- MallData : first_recharge_package ----------" << endl;
        first_recharge_package.output();
        std::cout << "========== MallData : first_recharge_package ==========\n" << endl;
        std::cout << "---------- MallData : order_charing_cfg ----------" << endl;
        unsigned int _order_charing_cfg_count_ = 0;
        for (map<unsigned int, PlatformOrderCharingCfg>::const_iterator it = order_charing_cfg.begin(); it != order_charing_cfg.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_order_charing_cfg_count_++ < (order_charing_cfg.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : order_charing_cfg ==========\n" << endl;
        std::cout << "---------- MallData : special_platform_money_cfg ----------" << endl;
        unsigned int _special_platform_money_cfg_count_ = 0;
        for (map<unsigned int, PlatformRechargeCfg>::const_iterator it = special_platform_money_cfg.begin(); it != special_platform_money_cfg.end(); ++it)
        {
            std::cout << "key = " << it->first << ", value : " << endl;
            it->second.output();
            if (_special_platform_money_cfg_count_++ < (special_platform_money_cfg.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : special_platform_money_cfg ==========\n" << endl;
        std::cout << "---------- MallData : money_list ----------" << endl;
        for (unsigned int i = 0; i < money_list.size(); ++i)
        {
            money_list[i].output();
            if (i < (money_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : money_list ==========\n" << endl;
        std::cout << "---------- MallData : fish_coin_list ----------" << endl;
        for (unsigned int i = 0; i < fish_coin_list.size(); ++i)
        {
            fish_coin_list[i].output();
            if (i < (fish_coin_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : fish_coin_list ==========\n" << endl;
        std::cout << "---------- MallData : diamonds_list ----------" << endl;
        for (unsigned int i = 0; i < diamonds_list.size(); ++i)
        {
            diamonds_list[i].output();
            if (i < (diamonds_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : diamonds_list ==========\n" << endl;
        std::cout << "---------- MallData : battery_equipment_list ----------" << endl;
        for (unsigned int i = 0; i < battery_equipment_list.size(); ++i)
        {
            battery_equipment_list[i].output();
            if (i < (battery_equipment_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : battery_equipment_list ==========\n" << endl;
        std::cout << "---------- MallData : scores_shop ----------" << endl;
        scores_shop.output();
        std::cout << "========== MallData : scores_shop ==========\n" << endl;
        std::cout << "---------- MallData : gold_package ----------" << endl;
        gold_package.output();
        std::cout << "========== MallData : gold_package ==========\n" << endl;
        std::cout << "---------- MallData : diamonds_package ----------" << endl;
        diamonds_package.output();
        std::cout << "========== MallData : diamonds_package ==========\n" << endl;
        std::cout << "---------- MallData : interaction_package ----------" << endl;
        interaction_package.output();
        std::cout << "========== MallData : interaction_package ==========\n" << endl;
        std::cout << "---------- MallData : wonderful_package ----------" << endl;
        wonderful_package.output();
        std::cout << "========== MallData : wonderful_package ==========\n" << endl;
        std::cout << "---------- MallData : pkPlay_package ----------" << endl;
        pkPlay_package.output();
        std::cout << "========== MallData : pkPlay_package ==========\n" << endl;
        std::cout << "---------- MallData : ff_chess_game_gold_list ----------" << endl;
        for (unsigned int i = 0; i < ff_chess_game_gold_list.size(); ++i)
        {
            ff_chess_game_gold_list[i].output();
            if (i < (ff_chess_game_gold_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : ff_chess_game_gold_list ==========\n" << endl;
        std::cout << "---------- MallData : ff_chess_board_list ----------" << endl;
        for (unsigned int i = 0; i < ff_chess_board_list.size(); ++i)
        {
            ff_chess_board_list[i].output();
            if (i < (ff_chess_board_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : ff_chess_board_list ==========\n" << endl;
        std::cout << "---------- MallData : ff_chess_pieces_list ----------" << endl;
        for (unsigned int i = 0; i < ff_chess_pieces_list.size(); ++i)
        {
            ff_chess_pieces_list[i].output();
            if (i < (ff_chess_pieces_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : ff_chess_pieces_list ==========\n" << endl;
        std::cout << "---------- MallData : ff_chess_gift_package_list ----------" << endl;
        for (unsigned int i = 0; i < ff_chess_gift_package_list.size(); ++i)
        {
            ff_chess_gift_package_list[i].output();
            if (i < (ff_chess_gift_package_list.size() - 1)) std::cout << endl;
        }
        std::cout << "========== MallData : ff_chess_gift_package_list ==========\n" << endl;
    }
};

}

#endif  // __MALLCONFIGDATA_H__

