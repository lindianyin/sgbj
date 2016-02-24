
#include "new_trade.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "singleton.h"
#include "net.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "statistics.h"
#include "daily_task.h"

using namespace net;

Database& GetDb();
extern int getSessionChar(session_ptr& psession, CharData* &pc);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);
void InsertSaveDb(const std::string& sql);

//ó��ʵ������
void tradeRealReward(int& get);

//����ó�׹���
extern std::string str3starTradeMsg;
//�޼鲻��ó�׹���
extern std::string strWjbsTradeMsg;
//�޼鲻������ó�׹���
extern std::string strWjbs3starTradeMsg;

//����1800�룬����ȴ
static const int iTrueCdSecs =  1800;
//ÿ��ѡ��������300����ȴ
static const int iCdSecsEveryChoose = 300;
//����������Ҫ�Ľ��
#ifdef JP_SERVER
static const int iAbandonGold = 3;
#else
static const int iAbandonGold = 5;
#endif

//���̲���������
static const int iMaxWjbs = 4;

//���̲����ҷ���
#ifdef JP_SERVER
static const int iWjbsGold[iMaxWjbs] = {20,30,50,100};
#else
static const int iWjbsGold[iMaxWjbs] = {50,50,100,200};
#endif

//��cd���ã�ÿ����
#ifdef JP_SERVER
static const int iSpeedGoldCostEveryMinute = 3;
#else
static const int iSpeedGoldCostEveryMinute = 1;
#endif

//����λ�ÿ���vip
static const int iLastPosOpenVIP = 4;

//ÿ����ѷ�������
static const int iFreeAbandonEveryDay = 3;

//ó�����̲���ɹ�����
inline bool wjbs_random(int t)
{
    switch (t)
    {
        case 1:        //��һ��100%
            return true;
        case 2:        //�ڶ���90%
            return my_random(1, 100) <= 90;
        case 3:     //������30%
            return my_random(1, 100) <= 30;
        case 4:     //���Ĵ�5%
            return my_random(1, 100) <= 5;        
    }
    return false;
}

void char_trade_info::Save()
{
    std::string sql = "replace into char_trades (cid,refresh_trader1,refresh_trader2,refresh_trader3,refresh_trader4,trader1,trader2,trader3,trader4,coolTime,start,wjbs_count,wjbs_fail,trade_star,trade_silver,trade_goods,trade_goods_count) values (";

    sql += LEX_CAST_STR(m_char_id) + ",";

    for (int i = 0; i < iMaxRefreshTrader; ++i)
    {
        sql += LEX_CAST_STR(m_refresh_traders[i]) + ",";
    }
    
    for (int i = 0; i < iMaxTrader; ++i)
    {
        sql += LEX_CAST_STR(m_my_traders[i]) + ",";
    }
    sql += LEX_CAST_STR(m_cool_time) + ",";
    if (m_in_trade)
    {
        sql += "1,";
    }
    else
    {
        sql += "0,";
    }
    sql += LEX_CAST_STR(m_wjbs_count) + ",";
    if (m_wjbs_fail)
    {
        sql += "1,";
    }
    else
    {
        sql += "0,";
    }
    sql += LEX_CAST_STR(m_trade_star) + ",";
    sql += LEX_CAST_STR(m_trade_silver) + ",";
    sql += LEX_CAST_STR(m_trade_goods) + ",";
    sql += LEX_CAST_STR(m_trade_goods_count) + ")";
    
    InsertSaveDb(sql);
}

json_spirit::Object base_trader::Obj()
{
    return obj;
}

newTradeMgr::newTradeMgr()
{
    reload();
}

void newTradeMgr::reload()
{
    m_trade_open_stronghold = 0;
    
    Query q(GetDb());
    q.get_result("select id,name,spic,gailv from base_traders where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        base_trader* tr = new base_trader;
        tr->id = q.getval();
        tr->name = q.getstr();
        tr->spic = q.getval();
        tr->gailv = q.getval();

        tr->quality = 0;

        tr->obj.clear();
        tr->obj.push_back( Pair("id", tr->id) );
        tr->obj.push_back( Pair("spic", tr->id) );
        tr->obj.push_back( Pair("name", tr->name) );

        boost::shared_ptr<base_trader> sp_bt(tr);
        m_base_traders.push_back(sp_bt);
        m_trader_gailv.push_back(tr->gailv);
        assert(tr->id == m_base_traders.size());
    }
    q.free_result();

    q.get_result("select id,stronghold,fac from base_trade_goods where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        base_trade_goods* gd = new base_trade_goods;
        gd->id = q.getval();
        gd->stronghold = q.getval();
        gd->m_fac = q.getnum();

        boost::shared_ptr<base_trade_goods> sp_gd(gd);
        m_base_trade_goods.push_back(sp_gd);
    }
    q.free_result();

    if (m_base_trade_goods.size())
    {
        m_trade_open_stronghold = m_base_trade_goods[0]->stronghold;
    }

    q.get_result("select id,need1,need2,need3,need4,fac from base_trade_combos where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        base_trade_combo* tcmb = new base_trade_combo;
        tcmb->id = q.getval();
        tcmb->star = tcmb->id;
        for (int i = 0; i < 4; ++i)
        {
            int trader = q.getval();
            if (trader > 0 && trader <= m_base_traders.size())
            {
                if (m_base_traders[trader-1].get())
                {
                    m_base_traders[trader-1].get()->quality = tcmb->star;
                }
                tcmb->m_traders.push_back(trader);
            }
        }
        tcmb->fac = q.getval();
        boost::shared_ptr<base_trade_combo> sp_tcmb(tcmb);
        m_trader_combos.push_back(sp_tcmb);
    }
    q.free_result();

    std::list<boost::shared_ptr<base_trade_combo> >::iterator it = m_trader_combos.begin();
    while (it != m_trader_combos.end())
    {
        base_trade_combo* combo = (*it).get();
        json_spirit::Object obj;
        obj.push_back( Pair("star", combo->star) );
        json_spirit::Array list;
        for (std::list<int>::iterator it2 = combo->m_traders.begin(); it2 != combo->m_traders.end(); ++it2)
        {
            if (*it2 > 0 && *it2 <= m_base_traders.size() && m_base_traders[*it2-1].get())
            {
                list.push_back(m_base_traders[*it2-1].get()->obj);
            }
        }
        //cout<<"load combo , start "<<combo->star<<endl;
        obj.push_back( Pair("list", list) );
        m_trader_combos_array.push_back(obj);
        ++it;
    }
}

boost::shared_ptr<char_trade_info> newTradeMgr::getCharTradeInfo(CharData& cdata)
{
    std::map<int, boost::shared_ptr<char_trade_info> >::iterator it = m_trade_infos.find(cdata.m_id);
    if (it != m_trade_infos.end())
    {
        return it->second;
    }
    Query q(GetDb());
    q.get_result("select refresh_trader1,refresh_trader2,refresh_trader3,refresh_trader4,trader1,trader2,trader3,trader4,coolTime,start,wjbs_count,wjbs_fail,trade_silver,trade_goods,trade_goods_count from char_trades where cid=" + LEX_CAST_STR(cdata.m_id));
    if (q.fetch_row())
    {
        char_trade_info* tinfo = new char_trade_info;
        tinfo->m_char_id = cdata.m_id;
        for (int i = 0; i < iMaxRefreshTrader; ++i)
        {
            tinfo->m_refresh_traders[i] = q.getval();
        }
        for (int i = 0; i < iMaxTrader; ++i)
        {
            tinfo->m_my_traders[i] = q.getval();
        }
        tinfo->m_cool_time = q.getval();
        tinfo->m_in_cool = (tinfo->m_cool_time - time(NULL)) > iTrueCdSecs;
        tinfo->m_in_trade = q.getval();
        tinfo->m_wjbs_count = q.getval();
        tinfo->m_wjbs_fail = q.getval();
        tinfo->m_trade_silver = q.getval();
        tinfo->m_trade_goods = q.getval();
        tinfo->m_trade_goods_count = q.getval();
        q.free_result();
        boost::shared_ptr<char_trade_info> t(tinfo);
        m_trade_infos[cdata.m_id] = t;

        return t;
    }
    else
    {
        q.free_result();
        if (cdata.m_tradeOpen)
        {
            //����ó��
            char_trade_info* tinfo = new char_trade_info;
            tinfo->m_char_id = cdata.m_id;
            for (int i = 0; i < iMaxRefreshTrader; ++i)
            {
                tinfo->m_refresh_traders[i] = i + 1;
            }
            for (int i = 0; i < iMaxTrader; ++i)
            {
                tinfo->m_my_traders[i] = 0;
            }
            if (cdata.m_vip < iLastPosOpenVIP)
            {
                tinfo->m_refresh_traders[iMaxRefreshTrader-1] = 0;
            }
            tinfo->m_cool_time = 0;
            tinfo->m_in_cool = false;
            tinfo->m_in_trade = false;
            tinfo->m_wjbs_count = 0;
            tinfo->m_wjbs_fail = false;
            tinfo->m_trade_silver = 0;
            tinfo->m_trade_goods = 0;
            tinfo->m_trade_goods_count = 0;
            tinfo->m_trade_star = 0;
            q.free_result();
            boost::shared_ptr<char_trade_info> t(tinfo);
            m_trade_infos[cdata.m_id] = t;
            t->Save();
            return t;
        }
    }
    boost::shared_ptr<char_trade_info> t;
    return t;
}

void newTradeMgr::refresh(char_trade_info& t, int len)
{
    boost::shared_ptr<CharData> p_cd = GeneralDataMgr::getInstance()->GetCharData(t.m_char_id);
    if (!p_cd.get())
    {
        return;
    }
    int refresh_num = p_cd->queryExtraData(char_data_type_normal, char_data_trade_refresh_num);
    if (refresh_num == 0)//��һ��ˢ����ڵ�һ��λ���������
    {
        for (int i = 0; i < iMaxRefreshTrader; ++i)
        {
            if (i < len)
            {
                if (i == 0)
                {
                    t.m_refresh_traders[i] = 1;
                }
                else if (i == 1)
                {
                    t.m_refresh_traders[i] = 2;
                }
                else if (i == 2)
                {
                    t.m_refresh_traders[i] = 3;
                }
                else if (i == 3)
                {
                    t.m_refresh_traders[i] = 4;
                }
            }
            else
            {
                t.m_refresh_traders[i] = 0;
            }
        }
        return;
    }
    else if (refresh_num == 1)//�ڶ���ˢ��ɣ�ڵ�һ��λ���������
    {
        for (int i = 0; i < iMaxRefreshTrader; ++i)
        {
            if (i < len)
            {
                if (i == 0)
                {
                    t.m_refresh_traders[i] = 2;
                }
                else if (i == 1)
                {
                    t.m_refresh_traders[i] = 4;
                }
                else if (i == 2)
                {
                    t.m_refresh_traders[i] = 9;
                }
                else if (i == 3)
                {
                    t.m_refresh_traders[i] = 1;
                }
            }
            else
            {
                t.m_refresh_traders[i] = 0;
            }
        }
        return;
    }
    std::vector<int> gailv = m_trader_gailv;
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    for (int i = 0; i < iMaxRefreshTrader; ++i)
    {
        if (i < len)
        {
            boost::random::discrete_distribution<> dist(gailv);
            t.m_refresh_traders[i] = dist(gen) + 1;
            gailv[t.m_refresh_traders[i]-1] = 0;
        }
        else
        {
            t.m_refresh_traders[i] = 0;
        }
    }
}

//ѡ������
int newTradeMgr::selectTrader(CharData& cdata, int id)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    char_trade_info* t = tradeInfo.get();
    //�Ƿ���ȴ��
    if (t->m_in_cool)
    {
        if (t->m_cool_time < time(NULL))
        {
            t->m_in_cool = false;
        }
        else
        {
            return HC_ERROR_IN_TRADE_CD;
        }
    }
    //�Ƿ�ó����
    if (t->m_in_trade)
    {
        return HC_ERROR_IN_TRADE;
    }
    if (id <= 0)
    {
        return HC_ERROR;
    }
    bool valid = false;
    for (int i = 0; i < iMaxRefreshTrader; ++i)
    {
        if (t->m_refresh_traders[i] == id)
        {
            valid = true;
            break;
        }
    }
    //�Ƿ���Ч����
    if (!valid)
    {
        return HC_ERROR;
    }
    if (t->m_my_traders[iMaxTrader-1] > 0)
    {
        //ǰ��
        for (int i = 0; i < (iMaxTrader-1); ++i)
        {
            t->m_my_traders[i] = t->m_my_traders[i+1];
        }
        t->m_my_traders[iMaxTrader-1] = 0;
    }

    //�嵽ĩβ
    for (int i = 0; i < iMaxTrader; ++i)
    {
        if (t->m_my_traders[i] == 0)
        {
            t->m_my_traders[i] = id;
            break;
        }
    }
    //ͳ�ƴ�������������ͳ��2��
    int refresh_num = cdata.queryExtraData(char_data_type_normal, char_data_trade_refresh_num);
    if (refresh_num < 2)
    {
        cdata.setExtraData(char_data_type_normal, char_data_trade_refresh_num, refresh_num + 1);
    }

    //ˢ������ ???
    refresh(*t, cdata.m_vip >= iLastPosOpenVIP ? iMaxRefreshTrader : iMaxRefreshTrader-1);

    //������ȴʱ��
    if (t->m_cool_time < time(NULL))
    {
        t->m_cool_time = time(NULL) + iCdSecsEveryChoose;
    }
    else
    {
        t->m_cool_time += iCdSecsEveryChoose;
    }

    if (t->m_cool_time - time(NULL) > iTrueCdSecs)
    {
        t->m_in_cool = true;
    }

    t->Save();
    return HC_SUCCESS;
}

//��������
int newTradeMgr::abandonTrader(CharData& cdata, int pos)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    char_trade_info* t = tradeInfo.get();
    //�Ƿ�ó����
    if (t->m_in_trade)
    {
        return HC_ERROR_IN_TRADE;
    }
    if (pos > iMaxTrader || pos < 1 || t->m_my_traders[pos-1] <= 0)
    {
        return HC_ERROR;
    }
    int free_abandon = iFreeAbandonEveryDay - cdata.queryExtraData(char_data_type_daily, char_data_daily_trade_abandon);
    if (free_abandon > 0)
    {
        cdata.setExtraData(char_data_type_daily, char_data_daily_trade_abandon, iFreeAbandonEveryDay-free_abandon + 1);
    }
    else
    {
        if (cdata.addGold(-iAbandonGold) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        cdata.NotifyCharData();

        //����������ͳ��
        add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, iAbandonGold, gold_cost_for_trade_abandon, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(&cdata,iAbandonGold,gold_cost_for_trade_abandon);
#endif
    }

    //ȥ������
    for (int i = pos - 1; i < iMaxTrader-1; ++i)
    {
        t->m_my_traders[i] = t->m_my_traders[i+1];
    }
    t->m_my_traders[iMaxTrader-1] = 0;

    //����
    t->Save();
    //actͳ��
    act_to_tencent(&cdata,act_new_trade_abandon);

    return HC_SUCCESS;
}

//��ѯ�������
void newTradeMgr::getTraderCombos(json_spirit::Object& robj)
{
    robj.push_back( Pair("list", m_trader_combos_array) );
}

//��ѯ�Լ�������
int newTradeMgr::getMyTraderList(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    json_spirit::Array list;
    char_trade_info* t = tradeInfo.get();
    for (int i = 0; i < iMaxTrader; ++i)
    {
        if (t->m_my_traders[i] <= 0
            || t->m_my_traders[i] > m_base_traders.size()
            || m_base_traders[t->m_my_traders[i]-1].get() == NULL)
        {
            break;
        }
        base_trader* bt = m_base_traders[t->m_my_traders[i]-1].get();
        json_spirit::Object obj = bt->Obj();
        obj.push_back( Pair("pos", i+1) );
        obj.push_back( Pair("quality", bt->quality) );
        list.push_back(obj);
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("gold", iAbandonGold) );
    int leftTimes = iTradeEveryday - cdata.queryExtraData(char_data_type_daily, char_data_trade_time);
    if (leftTimes < 0)
    {
        leftTimes = 0;
    }
    robj.push_back( Pair("left", leftTimes) );
    if (t->m_in_trade)
    {
        robj.push_back( Pair("inTrade", 1) );
    }
    else if (leftTimes > 0)
    {
        base_trade_combo* cmb = getCombao(*t);
        if (cmb)
        {
            robj.push_back( Pair("canTrade", cmb->star) );
        }
    }
    int free_abandon = iFreeAbandonEveryDay - cdata.queryExtraData(char_data_type_daily, char_data_daily_trade_abandon);
    if (free_abandon > 0)
    {
        robj.push_back( Pair("free", free_abandon) ); 
    }
    return HC_SUCCESS;
}

//��ѯˢ��CD
int newTradeMgr::getCoolTime(CharData& cdata)
{
    //cout<<"getTraderList"<<endl;
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return 0;
    }
    char_trade_info* t = tradeInfo.get();
    //�Ƿ���ȴ��
    if (t->m_in_cool)
    {
        int cd = t->m_cool_time - time(NULL);
        if (cd <= 0)
        {
            t->m_in_cool = false;
            return 0;
        }
        else
        {
            return cd;
        }
    }
    else
    {
        return 0;
    }
}

//��ѯˢ�³����̶�
int newTradeMgr::getTraderList(CharData& cdata, json_spirit::Object& robj)
{
    //cout<<"getTraderList"<<endl;
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    json_spirit::Array list;
    char_trade_info* t = tradeInfo.get();

    //����vip���Զ�ˢ��һ��
    for (int i = 0; i < iMaxRefreshTrader; ++i)
    {
        if (cdata.m_vip >= iLastPosOpenVIP
            && (t->m_refresh_traders[i] <= 0
            || t->m_refresh_traders[i] > m_base_traders.size()
            || m_base_traders[t->m_refresh_traders[i]-1].get() == NULL))
        {
            refresh(*t, iMaxRefreshTrader);
            break;
        }
    }
    for (int i = 0; i < iMaxRefreshTrader; ++i)
    {
        //cout<<t->m_refresh_traders[i]<<endl;
        if (t->m_refresh_traders[i] <= 0
            || t->m_refresh_traders[i] > m_base_traders.size()
            || m_base_traders[t->m_refresh_traders[i]-1].get() == NULL)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("pos", i+1) );
            obj.push_back( Pair("openVip", iLastPosOpenVIP) );
            list.push_back(obj);
        }
        else
        {
            base_trader* bt = m_base_traders[t->m_refresh_traders[i]-1].get();
            json_spirit::Object obj = bt->Obj();
            obj.push_back( Pair("pos", i+1) );
            obj.push_back( Pair("quality", bt->quality) );
            list.push_back(obj);
        }
    }
    //�Ƿ���ȴ��
    if (t->m_in_cool)
    {
        if (t->m_cool_time < time(NULL))
        {
            t->m_in_cool = false;
        }
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("gold", iSpeedGoldCostEveryMinute) );
    int cd = t->m_cool_time - time(NULL);
    if (cd > 0)
        robj.push_back( Pair("cd", cd) );
    robj.push_back( Pair("canSel", t->m_in_cool ? 0 : 1) );
    return HC_SUCCESS;
}

//������ȥ
int newTradeMgr::speedCool(CharData& cdata)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    char_trade_info* t = tradeInfo.get();
    //�Ƿ�ó����
    if (t->m_in_trade)
    {
        return HC_ERROR_IN_TRADE;
    }
    int cd = t->m_cool_time - time(NULL);
    if (cd <= 0)
    {
        return HC_SUCCESS;
    }
    int gold = iSpeedGoldCostEveryMinute*((cd/60) + (cd % 60 ? 1 : 0));
    if (cdata.addGold(-gold) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    cdata.NotifyCharData();
    
    //���ͳ��
    add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, gold, gold_cost_for_trade_speed, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(&cdata,gold,gold_cost_for_trade_speed);
#endif

    t->m_cool_time = 0;
    t->m_in_cool = false;

    t->Save();
    return HC_SUCCESS;
}

base_trade_goods* newTradeMgr::getGoods(int stronghold)
{
    base_trade_goods* ret = NULL;
    for (std::vector<boost::shared_ptr<base_trade_goods> >::iterator it = m_base_trade_goods.begin();
        it != m_base_trade_goods.end();
        ++it)
    {        
        assert ((*it).get());
        if ((*it)->stronghold > stronghold)
        {
            //cout<<"stronghold break "<<(*it)->stronghold<<endl;
            break;
        }
        else
        {
            ret = (*it).get();
        }
    }
    //if (ret)
    //{
    //    cout<<"getGoods("<<stronghold<<") -> "<<ret->stronghold<<endl;
    //}
    return ret;
}

base_trade_combo* newTradeMgr::getCombao(char_trade_info& tr)
{
    base_trade_combo* ret = NULL;
    std::list<boost::shared_ptr<base_trade_combo> >::iterator it = m_trader_combos.begin();
    while (it != m_trader_combos.end())
    {
        assert((*it).get());
        ret = (*it).get();

        bool not_find = false;
        for (std::list<int>::iterator it2 = ret->m_traders.begin(); it2 != ret->m_traders.end(); ++it2)
        {
            bool find = false;
            for (int i = 0; i < iMaxTrader; ++i)
            {
                if (tr.m_my_traders[i] == *it2)
                {
                    find = true;
                    break;
                }
            }
            if (!find)
            {
                not_find = true;
                break;
            }
        }
        if (!not_find)
        {
            return ret;
        }
        ++it;
    }
    return NULL;
}

//ó����Ϣ
void newTradeMgr::getTradeInfo(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return;
    }
    char_trade_info* t = tradeInfo.get();
    if (t->m_in_trade)
    {
        baseTreasure* bt = GeneralDataMgr::getInstance()->GetBaseTreasure(t->m_trade_goods).get();
        if (bt)
        {
            json_spirit::Object goods;
            goods.push_back( Pair("id", bt->id) );
            goods.push_back( Pair("spic", bt->spic) );
            goods.push_back( Pair("name", bt->name) );
            goods.push_back( Pair("need", 5) );
            goods.push_back( Pair("cur", 5) );

            robj.push_back( Pair("needgoods", goods) );
        }
        robj.push_back( Pair("silver", t->m_trade_silver) );
        robj.push_back( Pair("start", 1) );
        if (t->m_wjbs_fail)
        {
            robj.push_back( Pair("wjbsfail", 1) );
        }
        robj.push_back( Pair("wjbsTimes", t->m_wjbs_count) );
        if (t->m_wjbs_count < 0 || t->m_wjbs_count >= iMaxWjbs)
        {
            
        }
        else
        {
            robj.push_back( Pair("wjbs", iWjbsGold[t->m_wjbs_count]) );
        }
        robj.push_back( Pair("wjbs_ling", cdata.m_bag.getGemCount(treasure_type_wjbs)) );
    }
    else
    {
        base_trade_goods* trade_goods = getGoods(cdata.m_currentStronghold);
        if (trade_goods)
        {
            baseTreasure* bt = GeneralDataMgr::getInstance()->GetBaseTreasure(trade_goods->id).get();
            int cur = cdata.m_bag.getGemCount(trade_goods->id);
            if (bt)
            {            
                json_spirit::Object goods;
                goods.push_back( Pair("id", bt->id) );
                goods.push_back( Pair("spic", bt->spic) );
                goods.push_back( Pair("name", bt->name) );
                goods.push_back( Pair("need", 5) );
                goods.push_back( Pair("cur", cur) );
                if (cur < 5 && bt->m_place.get())
                {
                    goods.push_back( Pair("info", bt->m_place->info) );
                }

                robj.push_back( Pair("needgoods", goods) );
            }
            //��Ʒ��������
            if (cur >= 5)
            {
                //�������
                base_trade_combo* cmb = getCombao(*t);
                if (cmb)
                {
                    int silver = cmb->fac * 20000 * trade_goods->m_fac;
                    tradeRealReward(silver);
                    robj.push_back( Pair("silver", silver) );
                }
            }
            robj.push_back( Pair("wjbs_ling", cdata.m_bag.getGemCount(treasure_type_wjbs)) );
        }
    }
}

//��ʼó��
int newTradeMgr::startTrade(CharData& cdata)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    char_trade_info* t = tradeInfo.get();
    if (t->m_in_trade)
    {
        return HC_ERROR;
    }

    int leftTimes = iTradeEveryday - cdata.queryExtraData(char_data_type_daily, char_data_trade_time);
    if (leftTimes <= 0)
    {
        return HC_ERROR;
    }
    base_trade_goods* trade_goods = getGoods(cdata.m_currentStronghold);
    if (!trade_goods)
    {
        return HC_ERROR;
    }
    int cur = cdata.m_bag.getGemCount(trade_goods->id);
    if (cur < 5)
    {
        //������������
        return HC_ERROR;
    }

    base_trade_combo* cmb = getCombao(*t);
    if (!cmb)
    {
        //û���
        return HC_ERROR;
    }

    //��ó�״���
    --leftTimes;
    cdata.setExtraData(char_data_type_daily, char_data_trade_time, iTradeEveryday-leftTimes);
    //�ճ�����
    dailyTaskMgr::getInstance()->updateDailyTask(cdata,daily_task_trade);

    t->m_in_trade = true;
    t->m_trade_silver = cmb->fac * 20000 * trade_goods->m_fac;
    
    //ó��ʵ������
    tradeRealReward(t->m_trade_silver);

    t->m_trade_goods = trade_goods->id;
    t->m_trade_goods_count = 5;
    t->m_wjbs_count = 0;
    t->m_wjbs_fail = false;
    t->m_trade_star = cmb->star;

    //����Ʒ
    int err_code = 0;
    cdata.m_bag.addGem(trade_goods->id, -5, err_code, false);

    int m_my_traders[iMaxTrader];
    memcpy(m_my_traders, t->m_my_traders, sizeof(int)*iMaxTrader);
    
    //�Ƴ���������
    for (std::list<int>::iterator it = cmb->m_traders.begin(); it != cmb->m_traders.end(); ++it)
    {
        for (int i = 0; i < iMaxTrader; ++i)
        {
            if (m_my_traders[i] == *it)
            {
                m_my_traders[i] = 0;
                break;
            }
        }
    }

    memset(t->m_my_traders, 0, sizeof(int)*iMaxTrader);

    int idx = 0;
    for (int i = 0; i < iMaxTrader; ++i)
    {
        if (m_my_traders[i] > 0)
        {
            t->m_my_traders[idx] = m_my_traders[i];
            ++idx;
            if (idx + 1 > iMaxTrader)
            {
                break;
            }
        }
    }
    
    t->Save();
    
    //actͳ��
    act_to_tencent(&cdata,act_new_finish,t->m_trade_star);

    return HC_SUCCESS;
}

//���̲���
int newTradeMgr::tradeWjbs(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    char_trade_info* t = tradeInfo.get();
    if (!t->m_in_trade)
    {
        return HC_ERROR;
    }
    if (t->m_wjbs_fail)
    {
        return HC_ERROR;
    }
    if (t->m_wjbs_count >= iMaxWjbs)
    {
        return HC_ERROR;
    }
    //actͳ��
    act_to_tencent(&cdata,act_new_trade_wjbs,t->m_wjbs_count);

    if (cdata.addTreasure(treasure_type_wjbs, -1) >= 0)
    {
        std::string msg = treasure_expend_msg(treasure_type_wjbs, 1);
        if (msg != "")
        {
            robj.push_back( Pair("msg", msg) );
        }
        //ͳ�Ƶ�������
        add_statistics_of_treasure_cost(cdata.m_id,cdata.m_ip_address,treasure_type_wjbs,1,treasure_unknow,2,cdata.m_union_id,cdata.m_server_id);
    }
    else if (cdata.addGold(-iWjbsGold[t->m_wjbs_count]) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    else
    {
        cdata.NotifyCharData();

        //ͳ�ƽ������
        add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, iWjbsGold[t->m_wjbs_count], gold_cost_for_trade_double, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(&cdata,iWjbsGold[t->m_wjbs_count],gold_cost_for_trade_double);
#endif
    }

    ++t->m_wjbs_count;
    if (wjbs_random(t->m_wjbs_count))
    {
        //�ɹ�
        robj.push_back( Pair("success", 1) );
        t->m_trade_silver *= 4;

        //֧������
        cdata.m_trunk_tasks.updateTask(task_trade_wjbs, 1);
    }
    else
    {
        //ʧ��
        t->m_wjbs_fail = true;
        t->m_trade_silver = t->m_trade_silver * 3 / 2;
    }
    t->Save();
    return HC_SUCCESS;
}

//����ó��
int newTradeMgr::finishTrade(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    char_trade_info* t = tradeInfo.get();
    if (!t->m_in_trade)
    {
        return HC_ERROR;
    }

    cdata.addSilver(t->m_trade_silver);
    //����ͳ��
    add_statistics_of_silver_get(cdata.m_id,cdata.m_ip_address,t->m_trade_silver,silver_get_trade, cdata.m_union_id, cdata.m_server_id);
    
    cdata.NotifyCharData();

    if (t->m_trade_star == 3)
    {
        //ϵͳ��������ó��
        std::string msg = t->m_wjbs_count > 0 ? strWjbs3starTradeMsg : str3starTradeMsg;
        str_replace(msg, "$W", MakeCharNameLink(cdata.m_name));
        str_replace(msg, "$S", LEX_CAST_STR(t->m_trade_silver));
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }
    else if (t->m_wjbs_count > 0)
    {
        //�޼鲻��ó�׹���
        std::string msg = strWjbsTradeMsg;
        str_replace(msg, "$W", MakeCharNameLink(cdata.m_name));
        str_replace(msg, "$S", LEX_CAST_STR(t->m_trade_silver));
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }
    if (t->m_trade_star > 1)
    {
        //֧������
        cdata.m_trunk_tasks.updateTask(task_trade_star, t->m_trade_star);
    }

    robj.push_back( Pair("silver", t->m_trade_silver));
    t->m_in_trade = false;
    t->m_trade_star = 0;
    t->m_trade_goods = 0;
    t->m_trade_goods_count = 0;
    t->m_trade_silver = 0;
    t->m_wjbs_count = 0;
    t->m_wjbs_fail = false;

    t->Save();

    return HC_SUCCESS;
}

//��ѯó�����    cmd��  getTradeCombos
int ProcessGetTradeCombos(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    Singleton<newTradeMgr>::Instance().getTraderCombos(robj);
    return HC_SUCCESS;
}

//��ѯ��ǰ�̶����    cmd��getMyTraderList
int ProcessGetMyTraderList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return Singleton<newTradeMgr>::Instance().getMyTraderList(*cdata.get(), robj);
}

//��������    cmd:    abandonTrader, pos:λ�ã�int��
int ProcessAbandonTrader(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int pos = 0;
    READ_INT_FROM_MOBJ(pos,o,"pos");
    return Singleton<newTradeMgr>::Instance().abandonTrader(*cdata.get(), pos);
}

//��ѯ�̶�    cmd:    getTraderList
int ProcessGetTraderList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int pos = 0;
    READ_INT_FROM_MOBJ(pos,o,"pos");
    return Singleton<newTradeMgr>::Instance().getTraderList(*cdata.get(), robj);
}

//��ȡ���ˣ�    cmd��    selectTrader    ,id:����id��int��
int ProcessSelectTrader(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    //actͳ��
    act_to_tencent(cdata.get(),act_new_trade_select);
    return Singleton<newTradeMgr>::Instance().selectTrader(*cdata.get(), id);
}

//��ѯó����Ϣ��    cmd��getTradeInfo
int ProcessGetTradeInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    Singleton<newTradeMgr>::Instance().getTradeInfo(*cdata.get(), robj);
    return HC_SUCCESS;
}

//��ʼó�ף�    cmd��startTrade
int ProcessStartTrade(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return Singleton<newTradeMgr>::Instance().startTrade(*cdata.get());
}

//ó��wjbs    cmd��tradeWjbs
int ProcessTradeWjbs(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return Singleton<newTradeMgr>::Instance().tradeWjbs(*cdata.get(), robj);
}

//���ó�ף�������    cmd: finishTrade
int ProcessFinishTrade(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return Singleton<newTradeMgr>::Instance().finishTrade(*cdata.get(), robj);
}

