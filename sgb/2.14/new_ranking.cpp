
#ifndef _WINDOWS
#include <execinfo.h>
#include <sys/syscall.h>
#endif

#include "new_ranking.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_all.h"

#include "spls_errcode.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include<boost/tokenizer.hpp>

#include "statistics.h"

#include "combat.h"

using namespace boost;

int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

extern void InsertSaveDb(const std::string& sql);

#define INFO(x)

Database& GetDb();

bool compare_rank_score(new_char_Rankings& a, new_char_Rankings& b)
{
    return a.score > b.score || (a.score == b.score && a.refresh_time < b.refresh_time);
}

newRankings* newRankings::m_handle = NULL;

newRankings* newRankings::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new newRankings();
        m_handle->reload();
    }
    return m_handle;
}

void newRankings::reload()
{
    Query q(GetDb());
    //各类活动加载
    q.get_result("select type,memo from base_new_rankings_event where open = 1 order by pos");
    while (q.fetch_row())
    {
        new_rankings_event nre;
        nre.type = q.getval();
        nre.memo = q.getstr();
        m_event_list.push_back(nre);
    }
    q.free_result();
    //奖励加载
    for (std::vector<new_rankings_event>::iterator it = m_event_list.begin(); it != m_event_list.end(); ++it)
    {
        q.get_result("select rank,reward_type,reward_id,reward_nums from base_new_rankings_event_reward where id = "+LEX_CAST_STR(it->type)+" order by aid");
        while (q.fetch_row())
        {
            int rank = q.getval();
            boost::shared_ptr<new_rankings_event_award> p_award = it->rankings_list[rank];
            if (!p_award.get())
            {
                p_award.reset(new new_rankings_event_award);
                p_award->rank = rank;
                it->rankings_list[rank] = p_award;
            }
            Item i;
            i.type = q.getval();
            i.id = q.getval();
            i.nums = q.getval();
            p_award->awards.push_back(i);
        }
        q.free_result();
    }
    //玩家积分
    int rank = 0;
    m_top_min_score = 0;
    q.get_result("select cid,score,rank_type,refresh_time from char_new_rankings_now where 1 order by score desc, refresh_time asc");
    while (q.fetch_row())
    {
        int cid = q.getval();
        int score = q.getval();
        int rank_type = q.getval();
        time_t refresh_time = q.getval();
        new_char_Rankings ncr;
        ncr.cid = cid;
        ncr.score = score;
        ncr.rank_type = rank_type;
        ncr.refresh_time = refresh_time;
        ncr.state = 0;
        m_score_maps[cid] = ncr;
        ++rank;
        if (rank <= iRewardMinRank)
        {
            //new_char_Rankings ncr;
            //ncr.cid = cid;
            //ncr.score = score;
            //ncr.rank_type = rank_type;
            //ncr.refresh_time = refresh_time;
            //ncr.state = 0;
            m_now_Rankings.push_back(ncr);
            m_top_min_score = ncr.score;
        }
    }
    q.free_result();
    //上周领取情况
    q.get_result("select cid,rank_type,score,refresh_time,state from char_new_rankings_last where 1 order by rank");
    while (q.fetch_row())
    {
        new_char_Rankings ncr;
        ncr.cid = q.getval();
        ncr.rank_type = q.getval();
        ncr.score = q.getval();
        ncr.refresh_time = q.getval();
        ncr.state = q.getval();
        m_last_Rankings.push_back(ncr);
    }
    q.free_result();
    m_event_type = GeneralDataMgr::getInstance()->getInt("ranking_event");
    int event_open = GeneralDataMgr::getInstance()->getInt("ranking_event_open", 1);
    if (event_open && 0 == m_event_type && m_event_list.size() > 0)
    {
        m_event_type = m_event_list[0].type;
        GeneralDataMgr::getInstance()->setInt("ranking_event", m_event_type);
    }

    //给予本周的称谓
    if (m_last_Rankings.size())
    {
        new_char_Rankings& cr = *(m_last_Rankings.begin());
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cr.cid).get();
        if (pc)
        {
            pc->m_nick.add_nick(nick_ranking_start - 1 + cr.rank_type);
            pc->SaveNick();
        }
    }
}

//活动是否开启
void newRankings::getAction(CharData* pc, json_spirit::Array& blist)
{
    if (pc->m_rankEventOpen && m_event_type)
    {
        int state = 0;
        if (pc->queryExtraData(char_data_type_normal, char_data_view_ranking) == 0)
            state = 1;
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_rankings) );
        obj.push_back( Pair("active", state) );
        blist.push_back(obj);
    }
    if (pc->m_rankEventOpen)
    {
        int type = 0, rank = 0;
        if (getRewardState(pc->m_id,type,rank) && rank > 0 && type > 0)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("type", top_level_event_rankings_reward) );
            obj.push_back( Pair("active", 1) );
            blist.push_back(obj);
        }
    }
}

//活动奖励生成
void newRankings::RankingsReward()
{
    //移除上周的称谓
    if (m_last_Rankings.size())
    {
        new_char_Rankings& cr = *(m_last_Rankings.begin());
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cr.cid).get();
        if (pc)
        {
            pc->m_nick.remove_nick(cr.rank_type + 5);
            pc->SaveNick();
        }
    }
    //生成本周奖励
    m_last_Rankings.clear();
    InsertSaveDb("TRUNCATE TABLE char_new_rankings_last");
    int rank = 0;
    for (std::list<new_char_Rankings>::iterator it = m_now_Rankings.begin(); it != m_now_Rankings.end(); ++it)
    {
        ++rank;
        if (rank > iRewardMinRank)
        {
            break;
        }
        new_char_Rankings cr;
        cr.cid = it->cid;
        cr.rank_type = it->rank_type;
        cr.score = it->score;
        cr.refresh_time = it->refresh_time;
        cr.state = 0;
        m_last_Rankings.push_back(cr);
        InsertSaveDb("replace into char_new_rankings_last (cid,rank,rank_type,score,refresh_time,state) values ("
                + LEX_CAST_STR(cr.cid)
                + "," + LEX_CAST_STR(rank)
                + "," + LEX_CAST_STR(cr.rank_type)
                + "," + LEX_CAST_STR(cr.score)
                + "," + LEX_CAST_STR(cr.refresh_time)
                + "," + LEX_CAST_STR(cr.state)
                + ")");
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(it->cid);
        if (cdata.get())
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
            if (account.get())
            {
                account->Send("{\"cmd\":\"addAction\",\"type\":12,\"s\":200,\"active\":1}");
            }
        }
    }
    //给予本周的称谓
    if (m_last_Rankings.size())
    {
        new_char_Rankings& cr = *(m_last_Rankings.begin());
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cr.cid).get();
        if (pc)
        {
            pc->m_nick.add_nick(nick_ranking_start - 1 + cr.rank_type);
            pc->SaveNick();
        }
    }
    m_score_maps.clear();
    m_now_Rankings.clear();
    InsertSaveDb("TRUNCATE TABLE char_new_rankings_now");
    update();
    return;
}

//更新活动
void newRankings::update()
{
    bool next = false;
    bool end = true;
    for (std::vector<new_rankings_event>::iterator it = m_event_list.begin(); it != m_event_list.end(); ++it)
    {
        if (next)
        {
            m_event_type = it->type;
            end = false;
            break;
        }
        if (it->type == m_event_type)
        {
            next = true;
        }
    }
    //2.13活动不再持续循环
    #if 0
    if (end && m_event_list.size() > 0)
    {
        m_event_type = m_event_list[0].type;
    }
    #else
    if (end)
    {
        m_event_type = 0;
        GeneralDataMgr::getInstance()->setInt("ranking_event_open", 0);
    }
    #endif
    GeneralDataMgr::getInstance()->setInt("ranking_event", m_event_type);
}

//获取排行信息
int newRankings::getRankingsInfo(CharData* pc, json_spirit::Object &robj)
{
    //第一闪
    int first_view = pc->queryExtraData(char_data_type_normal, char_data_view_ranking);
    if (0 == first_view)
    {
        pc->setExtraData(char_data_type_normal, char_data_view_ranking, 1);
        pc->notifyEventState(top_level_event_rankings, 0, 0);
    }
    //排行活动信息
    json_spirit::Object event_obj;
    for (std::vector<new_rankings_event>::iterator it = m_event_list.begin(); it != m_event_list.end(); ++it)
    {
        if (it->type == m_event_type)
        {
            event_obj.push_back( Pair("type", m_event_type) );
            event_obj.push_back( Pair("memo", it->memo) );
            json_spirit::Array reward_list;
            for (int rank = 1; rank <= 5; ++rank)
            {
                boost::shared_ptr<new_rankings_event_award> p_award = it->rankings_list[rank];
                if (p_award.get())
                {
                    json_spirit::Object o;
                    o.push_back( Pair("rank", rank) );
                    json_spirit::Array getlist;
                    for (std::list<Item>::iterator it_i = p_award->awards.begin(); it_i != p_award->awards.end(); ++it_i)
                    {
                        json_spirit::Object getobj;
                        it_i->toObj(getobj);
                        getlist.push_back(getobj);
                    }
                    o.push_back( Pair("get", getlist) );
                    reward_list.push_back(o);
                }
            }
            event_obj.push_back( Pair("rank_get", reward_list) );
            robj.push_back( Pair("event_info", event_obj) );
            break;
        }
    }
    //排行信息
    json_spirit::Array rank_list;
    int rank = 0;
    for (std::list<new_char_Rankings>::iterator it = m_now_Rankings.begin(); it != m_now_Rankings.end(); ++it)
    {
        ++rank;
        if (rank <= iRewardMinRank)
        {
            json_spirit::Object o;
            o.push_back( Pair("rank", rank) );
            o.push_back( Pair("cid", it->cid) );
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(it->cid);
            if (cdata.get())
            {
                o.push_back( Pair("name", cdata->m_name) );
                o.push_back( Pair("level", cdata->m_level) );
                if (cdata->m_corps_member.get())
                {
                    splsCorps* cp = corpsMgr::getInstance()->findCorps(cdata->m_corps_member->corps);
                    if (cp)
                    {
                        o.push_back( Pair("corp", cp->_name) );
                    }
                }
            }
            o.push_back( Pair("score", it->score) );
            o.push_back( Pair("refresh_time", it->refresh_time) );
            rank_list.push_back(o);
        }
    }
    robj.push_back( Pair("rank_list", rank_list) );
    //自身信息
    {
        json_spirit::Object o;
        o.push_back( Pair("cid", pc->m_id) );
        if (pc->m_corps_member.get())
        {
            splsCorps* cp = corpsMgr::getInstance()->findCorps(pc->m_corps_member->corps);
            if (cp)
            {
                o.push_back( Pair("corp", cp->_name) );
            }
        }
        o.push_back( Pair("score", m_score_maps[pc->m_id].score) );
        o.push_back( Pair("refresh_time", m_score_maps[pc->m_id].refresh_time) );
        robj.push_back( Pair("self_obj", o) );
    }
    return HC_SUCCESS;
}

//获取排行信息
int newRankings::getLastRankingsInfo(CharData* pc, json_spirit::Object &robj)
{
    //排行信息
    json_spirit::Array rank_list;
    int rank = 0;
    int last_rank_type = 0;
    for (std::list<new_char_Rankings>::iterator it = m_last_Rankings.begin(); it != m_last_Rankings.end(); ++it)
    {
        ++rank;
        last_rank_type = it->rank_type;
        if (rank <= iRewardMinRank)
        {
            json_spirit::Object o;
            o.push_back( Pair("rank", rank) );
            o.push_back( Pair("cid", it->cid) );
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(it->cid);
            if (cdata.get())
            {
                o.push_back( Pair("name", cdata->m_name) );
                o.push_back( Pair("level", cdata->m_level) );
                if (cdata->m_corps_member.get())
                {
                    splsCorps* cp = corpsMgr::getInstance()->findCorps(cdata->m_corps_member->corps);
                    if (cp)
                    {
                        o.push_back( Pair("corp", cp->_name) );
                    }
                }
            }
            o.push_back( Pair("score", it->score) );
            o.push_back( Pair("refresh_time", it->refresh_time) );
            rank_list.push_back(o);
        }
    }
    robj.push_back( Pair("rank_list", rank_list) );
    robj.push_back( Pair("rank_type", last_rank_type) );
    return HC_SUCCESS;
}

//更新排行信息
void newRankings::updateEventRankings(int cid, int type, int score)
{
    if (m_event_type != type)
        return;
    time_t time_now = time(NULL);
    if (m_score_maps[cid].score == 0 && m_score_maps[cid].cid == 0)
    {
        m_score_maps[cid].cid = cid;
        m_score_maps[cid].rank_type = m_event_type;
    }
    m_score_maps[cid].score += score;
    m_score_maps[cid].refresh_time = time_now;
    if (m_score_maps[cid].score > m_top_min_score || m_now_Rankings.size() < iRewardMinRank)
    {
        bool find = false;
        for (std::list<new_char_Rankings>::iterator it = m_now_Rankings.begin(); it != m_now_Rankings.end(); ++it)
        {
            if (it->cid == cid)
            {
                it->score = m_score_maps[cid].score;
                it->refresh_time = time_now;
                find = true;
            }
        }
        if (!find)
        {
            new_char_Rankings cr;
            cr.cid = cid;
            cr.rank_type = m_event_type;
            cr.score = m_score_maps[cid].score;
            cr.refresh_time = time_now;
            cr.state = 0;
            m_now_Rankings.push_back(cr);
        }
        m_now_Rankings.sort(compare_rank_score);
        if (m_now_Rankings.size() > 0)
        {
            m_top_min_score = m_now_Rankings.back().score;
        }
    }
    InsertSaveDb("replace into char_new_rankings_now (cid,rank_type,score,refresh_time) values (" + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(m_event_type) + "," + LEX_CAST_STR(m_score_maps[cid].score) + ",unix_timestamp())");
}

bool newRankings::getRewardState(int cid, int& type, int& rank)
{
    int tmp_rank = 0;
    for (std::list<new_char_Rankings>::iterator it = m_last_Rankings.begin(); it != m_last_Rankings.end(); ++it)
    {
        ++tmp_rank;
        if (tmp_rank > iRewardMinRank)
        {
            break;
        }
        if (it->cid == cid && it->state == 0)
        {
            type = it->rank_type;
            rank = tmp_rank;
            return true;
        }
    }
    return false;
}

bool newRankings::setRewardGet(int cid)
{
    int tmp_rank = 0;
    for (std::list<new_char_Rankings>::iterator it = m_last_Rankings.begin(); it != m_last_Rankings.end(); ++it)
    {
        ++tmp_rank;
        if (tmp_rank > iRewardMinRank)
        {
            break;
        }
        if (it->cid == cid && it->state == 0)
        {
            it->state = 1;
            InsertSaveDb("update char_new_rankings_last set state = 1 where rank = " + LEX_CAST_STR(tmp_rank) + " and cid = " + LEX_CAST_STR(cid));
            return true;
        }
    }
    return false;
}

int newRankings::getRankRewards(int cid, json_spirit::Object& robj)
{
    int type = 0, rank = 0;
    if (getRewardState(cid,type,rank) && rank > 0 && type > 0)
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cdata.get())
        {
            return HC_ERROR;
        }
        for (std::vector<new_rankings_event>::iterator it = m_event_list.begin(); it != m_event_list.end(); ++it)
        {
            if (it->type == type)
            {
                boost::shared_ptr<new_rankings_event_award> p_award = it->rankings_list[rank];
                if (p_award.get() && setRewardGet(cid))
                {
                    std::list<Item> items = p_award->awards;
                    giveLoots(cdata.get(), items, cdata->m_area, cdata->m_level, 0, NULL, &robj, true, give_rankings_event);
                    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
                    if (account.get())
                    {
                        account->Send("{\"cmd\":\"removeAction\",\"type\":12,\"s\":200}");
                    }
                    cdata->NotifyCharData();
                    return HC_SUCCESS;
                }
            }
        }
    }
    return HC_ERROR;
}

