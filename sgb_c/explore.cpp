
#include "explore.h"
#include "utils_all.h"
#include "errcode_def.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "const_def.h"
#include "SaveDb.h"
#include "item.h"
#include "city.h"

class chessCombat;

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

//查询洞穴列表
int ProcessQueryExploreCaveList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    boost::shared_ptr<base_city_building_info> bb = Singleton<cityMgr>::Instance().getBuildingInfo(BUILDING_TYPE_BARRACKS);
    if (bb.get())
    {
        if (cdata->m_level < bb->open_level)
        {
            robj.push_back( Pair("cb_level", 0) );
        }
        else
        {
            boost::shared_ptr<char_barracks> cb = Singleton<cityMgr>::Instance().getCharBarracks(cdata->m_id);
            if (cb.get())
            {
                robj.push_back( Pair("cb_level", cb->m_level));
            }
        }
    }
    boost::shared_ptr<charExplore> ce = Singleton<exploreMgr>::Instance().getCharExploreData(cdata->m_id);
    if (ce.get() && ce->m_start_time)
    {
        robj.push_back( Pair("explore_id", ce->m_cave_id) );
    }
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<baseCave> > cave_list = Singleton<exploreMgr>::Instance().getBaseCaves();
    std::map<int, boost::shared_ptr<baseCave> >::iterator it = cave_list.begin();
    while (it != cave_list.end())
    {
        if (it->second.get())
        {
            json_spirit::Object o;
            it->second->toObj(o);
            list.push_back(o);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//查询挂机洞穴信息
int ProcessQueryExploreCaveInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int cave_id = 1;
    READ_INT_FROM_MOBJ(cave_id,o,"id");
    boost::shared_ptr<charExplore> ce = Singleton<exploreMgr>::Instance().getCharExploreData(cdata->m_id);
    if (ce.get() && ce->m_start_time)
    {
        cave_id = ce->m_cave_id;
        if (ce->m_base_cave.get())
        {
            json_spirit::Object o;
            ce->m_base_cave->toObj(o);
            robj.push_back( Pair("caveInfo", o) );
        }
        int explore_time = time(NULL) - ce->m_start_time;
        if (explore_time > iMaxExploreTime)
            explore_time = iMaxExploreTime;
        robj.push_back( Pair("explore_time", explore_time) );
    }
    else
    {
        boost::shared_ptr<baseCave> be = Singleton<exploreMgr>::Instance().getBaseCave(cave_id);
        if (be.get())
        {
            json_spirit::Object o;
            be->toObj(o);
            robj.push_back( Pair("caveInfo", o) );
        }
    }
    robj.push_back( Pair("id", cave_id) );
    return HC_SUCCESS;
}

//开始探索
int ProcessExplore(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int cave_id = 1;
    READ_INT_FROM_MOBJ(cave_id,o,"id");
    boost::shared_ptr<charExplore> ce = Singleton<exploreMgr>::Instance().getCharExploreData(cdata->m_id);
    if (ce.get())
    {
        int start_cave_id = ce->start(cave_id);
        if (start_cave_id > 0)
        {
            robj.push_back( Pair("id", start_cave_id) );
            return HC_SUCCESS;
        }
        else
        {
            return HC_ERROR;
        }
    }
}

//获取探索奖励
int ProcessGetExploreReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    boost::shared_ptr<charExplore> ce = Singleton<exploreMgr>::Instance().getCharExploreData(cdata->m_id);
    if (ce.get())
    {
        return ce->getReward(cdata.get(),robj);
    }
}

void baseCave::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_id) );
    obj.push_back( Pair("spic", m_spic) );
    obj.push_back( Pair("open_level", m_open_level) );
    obj.push_back( Pair("open_vip", m_open_vip) );
    obj.push_back( Pair("name", m_name) );
    obj.push_back( Pair("memo", m_memo) );
    obj.push_back( Pair("exp_star", m_exp_star) );
    obj.push_back( Pair("silver_star", m_silver_star) );
}

int charExplore::start(int cave_id)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        ERR();
        return 0;
    }
    if (m_start_time > 0)
    {
        ERR();
        return 0;
    }
    int best_cave_id = Singleton<exploreMgr>::Instance().getBestCave(cdata.get());
    if (best_cave_id == 0)
    {
        return 0;
    }
    if (cave_id != 0)
    {
        if (cave_id <= best_cave_id)
        {
            m_cave_id = cave_id;
        }
        else
        {
            m_cave_id = best_cave_id;
        }
    }
    else if (m_cave_id == 0)
    {
        m_cave_id = best_cave_id;
    }
    m_base_cave = Singleton<exploreMgr>::Instance().getBaseCave(m_cave_id);
    m_start_time = time(NULL);
    save();
    return m_cave_id;
}

int charExplore::getReward(CharData* pc, json_spirit::Object& robj)
{
    time_t t_now = time(NULL);
    if (m_start_time > 0 && t_now > m_start_time && m_base_cave.get())
    {
        int time_fac = (t_now - m_start_time)/600;
        if (time_fac > 0)
        {
            if (time_fac > 72)
                time_fac = 72;
            //exp探索时间(每10分钟)*经验星级*经验星级*君主等级
            //silver探索时间(每10分钟)*经验星级*君主等级*君主等级
            int exp_get = time_fac * m_base_cave->m_exp_star * m_base_cave->m_exp_star * pc->m_level / 3;
            int silver_get = time_fac * m_base_cave->m_silver_star * pc->m_level * pc->m_level / 5;
            boost::shared_ptr<char_barracks> cb = Singleton<cityMgr>::Instance().getCharBarracks(pc->m_id);
            if (cb.get())
            {
                silver_get = silver_get * (100.0 + cb->getAdd()) / 100.0;
                exp_get = exp_get * (100.0 + cb->getAdd()) / 100.0;
            }
            //评级A,AA,AAA
            int extra[3] = {25,75,125};
            int index = my_random(1,3);
            silver_get = silver_get * (100 + extra[index-1]) / 100;
            exp_get = exp_get * (100 + extra[index-1]) / 100;
            std::list<Item> items;
            if (silver_get)
            {
                Item tmp(ITEM_TYPE_CURRENCY, CURRENCY_ID_SILVER, silver_get, 0);
                items.push_back(tmp);
            }
            if (exp_get)
            {
                Item tmp(ITEM_TYPE_CURRENCY, CURRENCY_ID_EXP, exp_get, 0);
                items.push_back(tmp);
            }
            giveLoots(pc,items,NULL,&robj,true,loot_explore);
            robj.push_back( Pair("result", index) );
            m_start_time = 0;
            save();
            pc->m_tasks.updateTask(GOAL_EXPLORE, m_cave_id, 1);
            pc->m_tasks.updateTask(GOAL_DAILY_EXPLORE, 0, 1);
            Singleton<goalMgr>::Instance().updateTask(pc->m_id, GOAL_TYPE_EXPLORE, 1);
            return HC_SUCCESS;
        }
        else
        {
            robj.push_back( Pair("result", 0) );
            m_start_time = 0;
            save();
            return HC_SUCCESS;
        }
    }
    if (m_start_time > 0)
    {
        m_start_time = 0;
        save();
    }
    return HC_ERROR;
}

void charExplore::save()
{
    InsertSaveDb("replace into char_explore (cid,caveid,start_time) values (" + LEX_CAST_STR(m_cid)
                                + "," + LEX_CAST_STR(m_cave_id)
                                + "," + LEX_CAST_STR(m_start_time)+ ")");
}

exploreMgr::exploreMgr()
{
    Query q(GetDb());
    //基础探索洞穴表
    q.get_result("select id,spic,open_level,open_vip,name,memo,exp_star,silver_star from base_explore_cave where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseCave> pbc = m_caves[id];
        if (!pbc.get())
        {
            pbc.reset(new baseCave);
            m_caves[id] = pbc;
        }
        pbc->m_id = id;
        pbc->m_spic = q.getval();
        pbc->m_open_level = q.getval();
        pbc->m_open_vip = q.getval();
        pbc->m_name = q.getstr();
        pbc->m_memo = q.getstr();
        pbc->m_exp_star = q.getval();
        pbc->m_silver_star = q.getval();
    }
    q.free_result();
}

int exploreMgr::getBestCave(CharData* pc)
{
    int cave_id = 0;
    int cb_level = 0;
    boost::shared_ptr<base_city_building_info> bb = Singleton<cityMgr>::Instance().getBuildingInfo(BUILDING_TYPE_BARRACKS);
    if (bb.get())
    {
        if (pc->m_level < bb->open_level)
        {
            return 0;
        }
        else
        {
            boost::shared_ptr<char_barracks> cb = Singleton<cityMgr>::Instance().getCharBarracks(pc->m_id);
            if (cb.get())
            {
                cb_level = cb->m_level;
            }
        }
    }
    std::map<int, boost::shared_ptr<baseCave> >::iterator it = m_caves.begin();
    while (it != m_caves.end())
    {
        if (it->second.get())
        {
            if (pc->m_vip >= it->second->m_open_vip && cb_level >= it->second->m_open_level)
            {
                cave_id = it->second->m_id;
            }
        }
        ++it;
    }
    return cave_id;
}

boost::shared_ptr<baseCave> exploreMgr::getBaseCave(int caveid)
{
    std::map<int, boost::shared_ptr<baseCave> >::iterator it = m_caves.find(caveid);
    if (it != m_caves.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseCave> gd;
        gd.reset();
        return gd;
    }
}

boost::shared_ptr<charExplore> exploreMgr::getCharExploreData(int cid)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        ERR();
        boost::shared_ptr<charExplore> p;
        p.reset();
        return p;
    }
    std::map<int, boost::shared_ptr<charExplore> >::iterator it = m_char_explore.find(cid);
    if (it != m_char_explore.end())
    {
        return it->second;
    }
    boost::shared_ptr<charExplore> p;
    p.reset(new charExplore(cid));
    Query q(GetDb());
    q.get_result("SELECT caveid,start_time FROM char_explore WHERE cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        p->m_cave_id = q.getval();
        p->m_start_time = q.getval();
        if (p->m_cave_id)
            p->m_base_cave = getBaseCave(p->m_cave_id);
    }
    q.free_result();
    //表里没有则初始化
    if (p->m_cave_id == 0)
    {
        p->m_cave_id = getBestCave(cdata.get());
        if (p->m_cave_id)
        {
            p->m_base_cave = getBaseCave(p->m_cave_id);
            if (p->m_base_cave.get())
            {
                p->m_start_time = 0;
                p->save();
            }
        }
    }
    return p;
}

