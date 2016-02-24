
#include "throne.h"
#include "spls_errcode.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include<boost/tokenizer.hpp>

#include "statistics.h"
#include "utils_lang.h"
#include "daily_task.h"

using namespace boost;

int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

extern void InsertSaveDb(const std::string& sql);

#define INFO(x)

Database& GetDb();

//获取皇座信息
int ProcessGetThroneInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return Singleton<throneMgr>::Instance().getLastRankingsInfo(cdata.get(), robj);
}

//获取参拜界面
int ProcessGetConInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return Singleton<throneMgr>::Instance().getConInfo(cdata.get(), robj);
}

//参拜
int ProcessThroneCon(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return Singleton<throneMgr>::Instance().congratulation(cdata.get(), o, robj);
}

throneMgr::throneMgr()
{
    Query q(GetDb());
    //奖励加载
    q.get_result("select type,reward_type,reward_id,reward_nums from base_throne_con_reward where 1 order by type");
    while (q.fetch_row())
    {
        int type = q.getval();
        boost::shared_ptr<throne_award> p_award = award_list[type];
        if (!p_award.get())
        {
            p_award.reset(new throne_award);
            p_award->silver = iThroneCon[type-1][0];
            p_award->gold = iThroneCon[type-1][1];
            award_list[type] = p_award;
        }
        Item i;
        i.type = q.getval();
        i.id = q.getval();
        i.nums = q.getval();
        p_award->awards.push_back(i);
    }
    q.free_result();
    //记录加载
    q.get_result("select log_time,memo from throne_log where 1 order by log_time");
    while (q.fetch_row())
    {
        throne_log t_log;
        t_log.log_time = q.getval();
        t_log.memo = q.getstr();
        m_log_list.push_back(t_log);
    }
    q.free_result();
    //上周领取情况
    q.get_result("select cid,type,score from throne_rankings_last where 1 order by rank");
    while (q.fetch_row())
    {
        char_throne_Rankings ctr;
        ctr.cid = q.getval();
        ctr.type = q.getval();
        ctr.score = q.getval();
        m_last_Rankings.push_back(ctr);
    }
    q.free_result();
    //给予称谓
    std::list<char_throne_Rankings>::iterator it = m_last_Rankings.begin();
    while (it != m_last_Rankings.end())
    {
        if (it->type > 0)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(it->cid).get();
            if (pc)
            {
                pc->m_nick.add_nick(nick_throne_start - 1 + it->type);
                pc->SaveNick();
                pc->set_attack_change(true);
            }
        }
        ++it;
    }
}

throneMgr::~throneMgr()
{
    InsertSaveDb("TRUNCATE TABLE throne_log");
    for (std::list<throne_log>::iterator it = m_log_list.begin(); it != m_log_list.end(); ++it)
    {
        InsertSaveDb("insert into throne_log (log_time,memo) values ('" + LEX_CAST_STR(it->log_time)
            + "','" + GetDb().safestr(it->memo) + "')");
    }
}

void throneMgr::update()
{
    //移除上周的称谓
    std::list<char_throne_Rankings>::iterator it = m_last_Rankings.begin();
    while (it != m_last_Rankings.end())
    {
        if (it->type > 0)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(it->cid).get();
            if (pc)
            {
                pc->m_nick.remove_nick(nick_throne_start - 1 + it->type);
                pc->SaveNick();
                pc->set_attack_change(true);                
            }
        }
        ++it;
    }
    //生成本周奖励
    m_last_Rankings.clear();
    InsertSaveDb("TRUNCATE TABLE throne_rankings_last");
    Query q(GetDb());
    q.get_result("select ce.cid,ce.value from char_data_extra as ce left join charactors as c on ce.cid=c.id where c.level>=45 and ce.field=" + LEX_CAST_STR(char_data_extra_prestige_get) + " order by ce.value desc limit 7");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        char_throne_Rankings ctr;
        ctr.cid = q.getval();
        ctr.score = q.getval();
        ctr.type = 0;
        m_last_Rankings.push_back(ctr);
    }
    q.free_result();
    int type = 0, cnt = iThroneData[type][0];
    it = m_last_Rankings.begin();
    int rank = 0;
    while (it != m_last_Rankings.end())
    {
        //当前头衔还没满并且符合头衔积分
        if (cnt > 0 && it->score >= iThroneData[type][1])
        {
            ++rank;
            it->type = type + 1;
            --cnt;
            InsertSaveDb("replace into throne_rankings_last (cid,rank,type,score) values ("
                    + LEX_CAST_STR(it->cid)
                    + "," + LEX_CAST_STR(rank)
                    + "," + LEX_CAST_STR(it->type)
                    + "," + LEX_CAST_STR(it->score)
                    + ")");
            ++it;
        }
        else
        {
            //判断下一头衔
            ++type;
            if (type < iThroneNickNum)
            {
                cnt = iThroneData[type][0];
                continue;
            }
            else//所有头衔都不符合
            {
                cnt = 0;
                ++rank;
                InsertSaveDb("replace into throne_rankings_last (cid,rank,type,score) values ("
                        + LEX_CAST_STR(it->cid)
                        + "," + LEX_CAST_STR(rank)
                        + "," + LEX_CAST_STR(it->type)
                        + "," + LEX_CAST_STR(it->score)
                        + ")");
                ++it;
            }
        }
    }
    //给予称谓
    it = m_last_Rankings.begin();
    while (it != m_last_Rankings.end())
    {
        if (it->type > 0)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(it->cid).get();
            if (pc)
            {
                pc->m_nick.add_nick(nick_throne_start - 1 + it->type);
                pc->SaveNick();
                pc->set_attack_change(true);
            }
        }
        ++it;
    }
    return;
}

//获取排行信息
int throneMgr::getLastRankingsInfo(CharData* pc, json_spirit::Object &robj)
{
    //排行信息
    json_spirit::Array rank1_list;
    json_spirit::Array rank2_list;
    json_spirit::Array rank3_list;
    for (std::list<char_throne_Rankings>::iterator it = m_last_Rankings.begin(); it != m_last_Rankings.end(); ++it)
    {
        json_spirit::Object o;
        o.push_back( Pair("cid", it->cid) );
        o.push_back( Pair("type", it->type) );
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(it->cid);
        if (cdata.get())
        {
            o.push_back( Pair("name", cdata->m_name) );
            o.push_back( Pair("spic", cdata->m_spic) );
            o.push_back( Pair("level", cdata->m_level) );
            o.push_back( Pair("attack", cdata->getAttack(0)) );
        }
        o.push_back( Pair("score", it->score) );
        if (it->type == nick_zhizun)
            rank1_list.push_back(o);
        else if(it->type == nick_zhanshen)
            rank2_list.push_back(o);
        else if(it->type == nick_tongshuai)
            rank3_list.push_back(o);
    }
    robj.push_back( Pair("rank1_list", rank1_list) );
    robj.push_back( Pair("rank2_list", rank2_list) );
    robj.push_back( Pair("rank3_list", rank3_list) );
    return HC_SUCCESS;
}

//获取参拜界面
int throneMgr::getConInfo(CharData* pc, json_spirit::Object &robj)
{
    json_spirit::Array con_list, log_list;
    for (std::map<int, boost::shared_ptr<throne_award> >::iterator it = award_list.begin(); it != award_list.end(); ++it)
    {
        json_spirit::Object o;
        o.push_back( Pair("type", it->first) );
        if (it->second.get())
        {
            if (it->second->silver > 0)
                o.push_back( Pair("silver", it->second->silver) );
            if (it->second->gold > 0)
                o.push_back( Pair("gold", it->second->gold) );
            json_spirit::Array get;
            std::list<Item> items = it->second->awards;
            for (std::list<Item>::iterator it_i = items.begin(); it_i != items.end(); ++it_i)
            {
                json_spirit::Object tmp;
                it_i->toObj(tmp);
                get.push_back(tmp);
            }
            o.push_back( Pair("get", get) );
        }
        con_list.push_back(o);
    }
    robj.push_back( Pair("con_list", con_list) );
    for (std::list<throne_log>::iterator it = m_log_list.begin(); it != m_log_list.end(); ++it)
    {
        json_spirit::Object tmp;
        tmp.push_back( Pair("log_time", it->log_time) );
        tmp.push_back( Pair("memo", it->memo) );
        log_list.push_back(tmp);
    }
    robj.push_back( Pair("log_list", log_list) );
    robj.push_back( Pair("state", pc->queryExtraData(char_data_type_daily, char_data_daily_con)) );
    return HC_SUCCESS;
}

int throneMgr::addConLog(std::string msg)
{
    if (m_log_list.size() >= 10)
    {
        m_log_list.pop_front();
    }
    throne_log t_log;
    t_log.log_time = time(NULL);
    t_log.memo = msg;
    m_log_list.push_back(t_log);
    return 0;
}

int throneMgr::congratulation(CharData* pc, json_spirit::mObject& o, json_spirit::Object& robj)
{
    if (pc->m_level < 45)
    {
        return HC_ERROR;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    if (pc->queryExtraData(char_data_type_daily, char_data_daily_con) > 0)
        return HC_ERROR;
    if (m_last_Rankings.size())
    {
        boost::shared_ptr<throne_award> p_award = award_list[type];
        if (p_award.get())
        {
            if (p_award->silver > 0)
            {
                if (pc->addSilver(-p_award->silver) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_SILVER;
                }
                else
                {
                    add_statistics_of_silver_cost(pc->m_id, pc->m_ip_address,p_award->silver,silver_cost_for_throne_con, pc->m_union_id, pc->m_server_id);
                }
            }
            if (p_award->gold > 0)
            {
                if (pc->addGold(-p_award->gold) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                else
                {
                    //金币消耗统计
                    add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, p_award->gold, gold_cost_for_throne_con, pc->m_union_id, pc->m_server_id);
                    #ifdef QQ_PLAT
                    gold_cost_tencent(pc,p_award->gold,gold_cost_for_throne_con);
                    #endif
                }
            }
            std::list<Item> items = p_award->awards;
            giveLoots(pc, items, pc->m_area, pc->m_level, 0, NULL, &robj, true, give_throne_loot);
            //记录
            std::string msg = strThroneConMsg;
            str_replace(msg, "$N", MakeCharNameLink(pc->m_name));
            char_throne_Rankings& cr = *(m_last_Rankings.begin());
            CharData* toppc = GeneralDataMgr::getInstance()->GetCharData(cr.cid).get();
            if (toppc)
            {
                std::string nick_s = "";
                if (cr.type == nick_zhizun)
                    nick_s = strThroneNick1;
                else if(cr.type == nick_zhanshen)
                    nick_s = strThroneNick2;
                else if(cr.type == nick_tongshuai)
                    nick_s = strThroneNick3;
                str_replace(msg, "$n", nick_s);
                str_replace(msg, "$W", MakeCharNameLink(toppc->m_name));
            }
            std::string type_s = "";
            if (type == 1)
                type_s = strThroneConType1;
            else if(type == 2)
                type_s = strThroneConType2;
            else if(type == 3)
                type_s = strThroneConType3;
            str_replace(msg, "$C", type_s);
            std::string getStr = "";
            for (std::list<Item>::iterator itx = items.begin(); itx != items.end(); ++itx)
            {
                if (getStr == "")
                {
                    getStr = itx->toString(true);
                }
                else
                {
                    getStr += "," + itx->toString(true);
                }
            }
            str_replace(msg, "$R", getStr);
            addConLog(msg);
            pc->setExtraData(char_data_type_daily, char_data_daily_con, 1);
            pc->NotifyCharData();
            //日常任务
            dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_throne_con);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

bool throneMgr::actionState()
{
    return m_last_Rankings.size() > 0;
}

void throneMgr::getAction(CharData& cdata, json_spirit::Array& elist)
{
    if (actionState() && cdata.m_level >= 45)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_throne) );
        obj.push_back( Pair("active", 0) );
        elist.push_back(obj);
    }
}

