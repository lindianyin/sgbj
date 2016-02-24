
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

//贸易实际收益
void tradeRealReward(int& get);

//三星贸易公告
extern std::string str3starTradeMsg;
//无奸不商贸易公告
extern std::string strWjbsTradeMsg;
//无奸不商三星贸易公告
extern std::string strWjbs3starTradeMsg;

//超过1800秒，真冷却
static const int iTrueCdSecs =  1800;
//每次选商人增加300秒冷却
static const int iCdSecsEveryChoose = 300;
//放弃商人需要的金币
#ifdef JP_SERVER
static const int iAbandonGold = 3;
#else
static const int iAbandonGold = 5;
#endif

//无商不奸最多次数
static const int iMaxWjbs = 4;

//无商不奸金币费用
#ifdef JP_SERVER
static const int iWjbsGold[iMaxWjbs] = {20,30,50,100};
#else
static const int iWjbsGold[iMaxWjbs] = {50,50,100,200};
#endif

//秒cd费用，每分钟
#ifdef JP_SERVER
static const int iSpeedGoldCostEveryMinute = 3;
#else
static const int iSpeedGoldCostEveryMinute = 1;
#endif

//最后的位置开放vip
static const int iLastPosOpenVIP = 4;

//每天免费放弃三次
static const int iFreeAbandonEveryDay = 3;

//贸易无商不奸成功概率
inline bool wjbs_random(int t)
{
    switch (t)
    {
        case 1:        //第一次100%
            return true;
        case 2:        //第二次90%
            return my_random(1, 100) <= 90;
        case 3:     //第三次30%
            return my_random(1, 100) <= 30;
        case 4:     //第四次5%
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
            //插入贸易
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
    if (refresh_num == 0)//第一次刷濮阳在第一个位置配合引导
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
    else if (refresh_num == 1)//第二次刷柴桑在第一个位置配合引导
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

//选择商人
int newTradeMgr::selectTrader(CharData& cdata, int id)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    char_trade_info* t = tradeInfo.get();
    //是否冷却中
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
    //是否贸易中
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
    //是否有效商人
    if (!valid)
    {
        return HC_ERROR;
    }
    if (t->m_my_traders[iMaxTrader-1] > 0)
    {
        //前移
        for (int i = 0; i < (iMaxTrader-1); ++i)
        {
            t->m_my_traders[i] = t->m_my_traders[i+1];
        }
        t->m_my_traders[iMaxTrader-1] = 0;
    }

    //插到末尾
    for (int i = 0; i < iMaxTrader; ++i)
    {
        if (t->m_my_traders[i] == 0)
        {
            t->m_my_traders[i] = id;
            break;
        }
    }
    //统计次数，根据引导统计2次
    int refresh_num = cdata.queryExtraData(char_data_type_normal, char_data_trade_refresh_num);
    if (refresh_num < 2)
    {
        cdata.setExtraData(char_data_type_normal, char_data_trade_refresh_num, refresh_num + 1);
    }

    //刷新商人 ???
    refresh(*t, cdata.m_vip >= iLastPosOpenVIP ? iMaxRefreshTrader : iMaxRefreshTrader-1);

    //增加冷却时间
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

//放弃商人
int newTradeMgr::abandonTrader(CharData& cdata, int pos)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    char_trade_info* t = tradeInfo.get();
    //是否贸易中
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

        //加入金币消耗统计
        add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, iAbandonGold, gold_cost_for_trade_abandon, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(&cdata,iAbandonGold,gold_cost_for_trade_abandon);
#endif
    }

    //去掉商人
    for (int i = pos - 1; i < iMaxTrader-1; ++i)
    {
        t->m_my_traders[i] = t->m_my_traders[i+1];
    }
    t->m_my_traders[iMaxTrader-1] = 0;

    //保存
    t->Save();
    //act统计
    act_to_tencent(&cdata,act_new_trade_abandon);

    return HC_SUCCESS;
}

//查询商人组合
void newTradeMgr::getTraderCombos(json_spirit::Object& robj)
{
    robj.push_back( Pair("list", m_trader_combos_array) );
}

//查询自己的商人
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

//查询刷新CD
int newTradeMgr::getCoolTime(CharData& cdata)
{
    //cout<<"getTraderList"<<endl;
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return 0;
    }
    char_trade_info* t = tradeInfo.get();
    //是否冷却中
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

//查询刷新出的商队
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

    //升级vip后自动刷新一次
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
    //是否冷却中
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

//加速冷去
int newTradeMgr::speedCool(CharData& cdata)
{
    boost::shared_ptr<char_trade_info> tradeInfo = getCharTradeInfo(cdata);
    if (!tradeInfo.get())
    {
        return HC_ERROR;
    }
    char_trade_info* t = tradeInfo.get();
    //是否贸易中
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
    
    //金币统计
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

//贸易信息
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
            //商品数量够了
            if (cur >= 5)
            {
                //组合有了
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

//开始贸易
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
        //道具数量不够
        return HC_ERROR;
    }

    base_trade_combo* cmb = getCombao(*t);
    if (!cmb)
    {
        //没组合
        return HC_ERROR;
    }

    //加贸易次数
    --leftTimes;
    cdata.setExtraData(char_data_type_daily, char_data_trade_time, iTradeEveryday-leftTimes);
    //日常任务
    dailyTaskMgr::getInstance()->updateDailyTask(cdata,daily_task_trade);

    t->m_in_trade = true;
    t->m_trade_silver = cmb->fac * 20000 * trade_goods->m_fac;
    
    //贸易实际收益
    tradeRealReward(t->m_trade_silver);

    t->m_trade_goods = trade_goods->id;
    t->m_trade_goods_count = 5;
    t->m_wjbs_count = 0;
    t->m_wjbs_fail = false;
    t->m_trade_star = cmb->star;

    //扣商品
    int err_code = 0;
    cdata.m_bag.addGem(trade_goods->id, -5, err_code, false);

    int m_my_traders[iMaxTrader];
    memcpy(m_my_traders, t->m_my_traders, sizeof(int)*iMaxTrader);
    
    //移除现有商人
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
    
    //act统计
    act_to_tencent(&cdata,act_new_finish,t->m_trade_star);

    return HC_SUCCESS;
}

//无商不奸
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
    //act统计
    act_to_tencent(&cdata,act_new_trade_wjbs,t->m_wjbs_count);

    if (cdata.addTreasure(treasure_type_wjbs, -1) >= 0)
    {
        std::string msg = treasure_expend_msg(treasure_type_wjbs, 1);
        if (msg != "")
        {
            robj.push_back( Pair("msg", msg) );
        }
        //统计道具消耗
        add_statistics_of_treasure_cost(cdata.m_id,cdata.m_ip_address,treasure_type_wjbs,1,treasure_unknow,2,cdata.m_union_id,cdata.m_server_id);
    }
    else if (cdata.addGold(-iWjbsGold[t->m_wjbs_count]) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    else
    {
        cdata.NotifyCharData();

        //统计金币消耗
        add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, iWjbsGold[t->m_wjbs_count], gold_cost_for_trade_double, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(&cdata,iWjbsGold[t->m_wjbs_count],gold_cost_for_trade_double);
#endif
    }

    ++t->m_wjbs_count;
    if (wjbs_random(t->m_wjbs_count))
    {
        //成功
        robj.push_back( Pair("success", 1) );
        t->m_trade_silver *= 4;

        //支线任务
        cdata.m_trunk_tasks.updateTask(task_trade_wjbs, 1);
    }
    else
    {
        //失败
        t->m_wjbs_fail = true;
        t->m_trade_silver = t->m_trade_silver * 3 / 2;
    }
    t->Save();
    return HC_SUCCESS;
}

//结束贸易
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
    //银币统计
    add_statistics_of_silver_get(cdata.m_id,cdata.m_ip_address,t->m_trade_silver,silver_get_trade, cdata.m_union_id, cdata.m_server_id);
    
    cdata.NotifyCharData();

    if (t->m_trade_star == 3)
    {
        //系统公告三星贸易
        std::string msg = t->m_wjbs_count > 0 ? strWjbs3starTradeMsg : str3starTradeMsg;
        str_replace(msg, "$W", MakeCharNameLink(cdata.m_name));
        str_replace(msg, "$S", LEX_CAST_STR(t->m_trade_silver));
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }
    else if (t->m_wjbs_count > 0)
    {
        //无奸不商贸易公告
        std::string msg = strWjbsTradeMsg;
        str_replace(msg, "$W", MakeCharNameLink(cdata.m_name));
        str_replace(msg, "$S", LEX_CAST_STR(t->m_trade_silver));
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }
    if (t->m_trade_star > 1)
    {
        //支线任务
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

//查询贸易组合    cmd：  getTradeCombos
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

//查询当前商队情况    cmd：getMyTraderList
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

//放弃商人    cmd:    abandonTrader, pos:位置（int）
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

//查询商队    cmd:    getTraderList
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

//抽取商人：    cmd：    selectTrader    ,id:商人id（int）
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
    //act统计
    act_to_tencent(cdata.get(),act_new_trade_select);
    return Singleton<newTradeMgr>::Instance().selectTrader(*cdata.get(), id);
}

//查询贸易信息：    cmd：getTradeInfo
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

//开始贸易：    cmd：startTrade
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

//贸易wjbs    cmd：tradeWjbs
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

//完成贸易，拿银币    cmd: finishTrade
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

