
#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include <map>
#include <list>
#include "json_spirit.h"
#include <string.h>

const int iMaxRefreshTrader = 4;
const int iMaxTrader = 4;

struct CharData;

struct char_trade_info
{
    int m_char_id;
    int m_refresh_traders[iMaxRefreshTrader];
    int m_my_traders[iMaxTrader];
    time_t m_cool_time;
    bool m_in_cool;    //是否真冷却
    bool m_in_trade;    //贸易中
    int m_wjbs_count;    //无商不奸次数
    bool m_wjbs_fail;    //无商不奸失败
    int m_trade_star;    //贸易星级
    int m_trade_silver;    //贸易总额
    int m_trade_goods;        //贸易商品
    int m_trade_goods_count;    //贸易商品需要数量

    void chooseTrader(int pos);    //选择商人

    void Save();
};

struct base_trader
{
    int id;
    std::string name;
    int spic;
    int gailv;
    int quality;

    json_spirit::Object obj;

    json_spirit::Object Obj();
};

struct base_trade_combo
{
    int star;
    int id;

    std::list<int> m_traders;

    int fac;
};

struct base_trade_goods
{
    int id;
    int stronghold;

    float m_fac;
};

class newTradeMgr
{
public:
    newTradeMgr();

    //放弃商人
    int abandonTrader(CharData& cdata, int pos);
    //查询商人组合
    void getTraderCombos(json_spirit::Object& robj);
    //查询自己的商人
    int getMyTraderList(CharData& cdata, json_spirit::Object& robj);
    //查询刷新出的商队
    int getTraderList(CharData& cdata, json_spirit::Object& robj);
    //选择商人
    int selectTrader(CharData& cdata, int id);
    //加速冷去
    int speedCool(CharData& cdata);
    //贸易信息
    void getTradeInfo(CharData& cdata, json_spirit::Object& robj);
    //开始贸易
    int startTrade(CharData& cdata);
    //无商不奸
    int tradeWjbs(CharData& cdata, json_spirit::Object& robj);
    //结束贸易
    int finishTrade(CharData& cdata, json_spirit::Object& robj);
    //查询刷新CD
    int getCoolTime(CharData& cdata);

    base_trade_goods* getGoods(int stronghold);
    
    base_trade_combo* getCombao(char_trade_info& tr);

    void reload();

    void refresh(char_trade_info& t, int len);
    
private:
    boost::shared_ptr<char_trade_info> getCharTradeInfo(CharData& cdata);
    std::map<int, boost::shared_ptr<char_trade_info> > m_trade_infos;
    std::vector<boost::shared_ptr<base_trader> > m_base_traders;

    std::vector<boost::shared_ptr<base_trade_goods> > m_base_trade_goods;

    std::list<boost::shared_ptr<base_trade_combo> > m_trader_combos;

    json_spirit::Array m_trader_combos_array;

    std::vector<int> m_trader_gailv;

    int m_trade_open_stronghold;
};

