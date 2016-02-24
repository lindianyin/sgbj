
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
    bool m_in_cool;    //�Ƿ�����ȴ
    bool m_in_trade;    //ó����
    int m_wjbs_count;    //���̲������
    bool m_wjbs_fail;    //���̲���ʧ��
    int m_trade_star;    //ó���Ǽ�
    int m_trade_silver;    //ó���ܶ�
    int m_trade_goods;        //ó����Ʒ
    int m_trade_goods_count;    //ó����Ʒ��Ҫ����

    void chooseTrader(int pos);    //ѡ������

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

    //��������
    int abandonTrader(CharData& cdata, int pos);
    //��ѯ�������
    void getTraderCombos(json_spirit::Object& robj);
    //��ѯ�Լ�������
    int getMyTraderList(CharData& cdata, json_spirit::Object& robj);
    //��ѯˢ�³����̶�
    int getTraderList(CharData& cdata, json_spirit::Object& robj);
    //ѡ������
    int selectTrader(CharData& cdata, int id);
    //������ȥ
    int speedCool(CharData& cdata);
    //ó����Ϣ
    void getTradeInfo(CharData& cdata, json_spirit::Object& robj);
    //��ʼó��
    int startTrade(CharData& cdata);
    //���̲���
    int tradeWjbs(CharData& cdata, json_spirit::Object& robj);
    //����ó��
    int finishTrade(CharData& cdata, json_spirit::Object& robj);
    //��ѯˢ��CD
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

