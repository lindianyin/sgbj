
#ifndef _WINDOWS
#include <execinfo.h>
#include <sys/syscall.h>
#endif

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include<boost/tokenizer.hpp>

#include "weekRanking.h"
#include "utils_all.h"
#include "errcode_def.h"
#include "singleton.h"
#include "rewards.h"


using namespace boost;
#define INFO(x)
extern void InsertSaveDb(const std::string& sql);
extern Database& GetDb();

bool compare_rank_score(char_weekRankings& a, char_weekRankings& b)
{
    return a.score > b.score || (a.score == b.score && a.refresh_time < b.refresh_time);
}

//本周排行信息
int ProcessGetWeekRankingsInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return Singleton<weekRankings>::Instance().getRankingsInfo(cdata,robj);
}

//上周排行信息
int ProcessGetLastWeekRankingsInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return Singleton<weekRankings>::Instance().getLastRankingsInfo(cdata,robj);
}

weekRankings::weekRankings()
{
    Query q(GetDb());
    //各类活动加载
    q.get_result("select type,name,memo from base_weekRankings_event where open = 1 order by pos");
    while (q.fetch_row())
    {
        boost::shared_ptr<weekRankings_event> nre;
        nre.reset(new weekRankings_event);
        nre->type = q.getval();
        nre->name = q.getstr();
        nre->memo = q.getstr();
        m_event_list.push_back(nre);
    }
    q.free_result();
    //奖励加载
    for (std::vector<boost::shared_ptr<weekRankings_event> >::iterator it = m_event_list.begin(); it != m_event_list.end(); ++it)
    {
        q.get_result("select rank,itemType,itemId,counts,extra from base_weekRankings_event_reward where id = "+LEX_CAST_STR((*it)->type)+" order by aid");
        while (q.fetch_row())
        {
            int rank = q.getval();
            boost::shared_ptr<weekRankings_event_award> p_award = (*it)->rankings_list[rank];
            if (!p_award.get())
            {
                p_award.reset(new weekRankings_event_award);
                p_award->rank = rank;
                (*it)->rankings_list[rank] = p_award;
            }
            Item i;
            i.type = q.getval();
            i.id = q.getval();
            i.nums = q.getval();
            i.extra = q.getval();
            p_award->awards.push_back(i);
        }
        q.free_result();
    }
    //玩家积分
    int rank = 0;
    m_top_min_score = 0;
    q.get_result("select cid,score,rank_type,refresh_time from char_weekRankings_now where 1 order by score desc, refresh_time asc");
    while (q.fetch_row())
    {
        int cid = q.getval();
        int score = q.getval();
        int rank_type = q.getval();
        time_t refresh_time = q.getval();
        char_weekRankings ncr;
        ncr.cid = cid;
        ncr.score = score;
        ncr.rank_type = rank_type;
        ncr.refresh_time = refresh_time;
        m_score_maps[cid] = ncr;
        ++rank;
        if (rank <= iRewardMinRank)
        {
            m_now_Rankings.push_back(ncr);
            m_top_min_score = ncr.score;
        }
    }
    q.free_result();
    //上周排行情况
    q.get_result("select cid,rank_type,score,refresh_time from char_weekRankings_last where 1 order by rank");
    while (q.fetch_row())
    {
        char_weekRankings ncr;
        ncr.cid = q.getval();
        ncr.rank_type = q.getval();
        ncr.score = q.getval();
        ncr.refresh_time = q.getval();
        m_last_Rankings.push_back(ncr);
    }
    q.free_result();
    m_event_type = GeneralDataMgr::getInstance()->getInt("weekRanking_event");
    int event_open = GeneralDataMgr::getInstance()->getInt("weekRanking_event_open", 1);
    if (event_open && 0 == m_event_type && m_event_list.size() > 0)
    {
        m_event_type = m_event_list[0]->type;
        GeneralDataMgr::getInstance()->setInt("ranking_event", m_event_type);
    }

    //给予本周的称谓
    if (m_last_Rankings.size())
    {
        char_weekRankings& cr = *(m_last_Rankings.begin());
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cr.cid).get();
        if (pc)
        {
            pc->m_nick.add_nick(nick_weekRank_start - 1 + cr.rank_type);
            pc->SaveNick();
        }
    }
}

//活动是否开启
void weekRankings::getButton(CharData* pc, json_spirit::Array& list)
{
    if (pc->isWeekRankingActionOpen()
         && m_event_type)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_weekRanking) );
        obj.push_back( Pair("active", 0) );
        list.push_back(obj);
    }
}

boost::shared_ptr<weekRankings_event> weekRankings::getWeekRankingsEvent(int type)
{
    for (std::vector<boost::shared_ptr<weekRankings_event> >::iterator it = m_event_list.begin(); it != m_event_list.end(); ++it)
    {
        if ((*it)->type == type)
        {
            return *it;
        }
    }
    boost::shared_ptr<weekRankings_event> p;
    return p;
}

//活动奖励生成
void weekRankings::RankingsReward()
{
    //移除上周的称谓
    if (m_last_Rankings.size())
    {
        char_weekRankings& cr = *(m_last_Rankings.begin());
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cr.cid).get();
        if (pc)
        {
            pc->m_nick.remove_nick(cr.rank_type + nick_weekRank_start - 1);
            pc->SaveNick();
        }
    }
    //生成本周奖励
    m_last_Rankings.clear();
    InsertSaveDb("TRUNCATE TABLE char_weekRankings_last");
    int rank = 0;
    for (std::list<char_weekRankings>::iterator it = m_now_Rankings.begin(); it != m_now_Rankings.end(); ++it)
    {
        ++rank;
        if (rank > iRewardMinRank)
        {
            break;
        }
        //发奖
        boost::shared_ptr<weekRankings_event> wre = getWeekRankingsEvent(m_event_type);
        if (wre.get())
        {
            boost::shared_ptr<weekRankings_event_award> p_award = wre->rankings_list[rank];
            if (p_award.get())
            {
                Singleton<rewardsMgr>::Instance().updateCharRewards(it->cid,REWARDS_TYPE_WEEK_RANKING,rank,p_award->awards);
            }
        }
        char_weekRankings cr;
        cr.cid = it->cid;
        cr.rank_type = it->rank_type;
        cr.score = it->score;
        cr.refresh_time = it->refresh_time;
        m_last_Rankings.push_back(cr);
        InsertSaveDb("replace into char_weekRankings_last (cid,rank,rank_type,score,refresh_time) values ("
                + LEX_CAST_STR(cr.cid)
                + "," + LEX_CAST_STR(rank)
                + "," + LEX_CAST_STR(cr.rank_type)
                + "," + LEX_CAST_STR(cr.score)
                + "," + LEX_CAST_STR(cr.refresh_time)
                + ")");
    }
    //给予本周的称谓
    if (m_last_Rankings.size())
    {
        char_weekRankings& cr = *(m_last_Rankings.begin());
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cr.cid).get();
        if (pc)
        {
            pc->m_nick.add_nick(nick_weekRank_start - 1 + cr.rank_type);
            pc->SaveNick();
        }
    }
    m_score_maps.clear();
    m_now_Rankings.clear();
    InsertSaveDb("TRUNCATE TABLE char_weekRankings_now");
    update();
    return;
}

//更新活动
void weekRankings::update()
{
    bool next = false;
    bool end = true;
    for (std::vector<boost::shared_ptr<weekRankings_event> >::iterator it = m_event_list.begin(); it != m_event_list.end(); ++it)
    {
        if (next)
        {
            m_event_type = (*it)->type;
            end = false;
            break;
        }
        if ((*it)->type == m_event_type)
        {
            next = true;
        }
    }
    if (end)
    {
        m_event_type = 0;
        GeneralDataMgr::getInstance()->setInt("weekRanking_event_open", 0);
    }
    GeneralDataMgr::getInstance()->setInt("weekRanking_event", m_event_type);
}

//获取排行信息
int weekRankings::getRankingsInfo(CharData* pc, json_spirit::Object &robj)
{
    //排行活动信息
    json_spirit::Object event_obj;
    boost::shared_ptr<weekRankings_event> wre = getWeekRankingsEvent(m_event_type);
    if (wre.get())
    {
        event_obj.push_back( Pair("type", m_event_type) );
        event_obj.push_back( Pair("name", wre->name) );
        event_obj.push_back( Pair("memo", wre->memo) );
        json_spirit::Array reward_list;
        for (int rank = 1; rank <= 5; ++rank)
        {
            boost::shared_ptr<weekRankings_event_award> p_award = wre->rankings_list[rank];
            if (p_award.get())
            {
                json_spirit::Object o;
                o.push_back( Pair("rank", rank) );
                json_spirit::Array getlist;
                itemlistToArray(p_award->awards, getlist);
                o.push_back( Pair("get", getlist) );
                reward_list.push_back(o);
            }
        }
        event_obj.push_back( Pair("rank_get", reward_list) );
        robj.push_back( Pair("event_info", event_obj) );
    }
    //排行信息
    json_spirit::Array rank_list;
    int rank = 0;
    for (std::list<char_weekRankings>::iterator it = m_now_Rankings.begin(); it != m_now_Rankings.end(); ++it)
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
                if (cdata->GetGuildId())
                {
                    o.push_back( Pair("guildname", Singleton<guildMgr>::Instance().getGuildName(cdata->GetGuildId())));
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
        if (pc->GetGuildId())
        {
            o.push_back( Pair("guildname", Singleton<guildMgr>::Instance().getGuildName(pc->GetGuildId())));
        }
        o.push_back( Pair("score", m_score_maps[pc->m_id].score) );
        o.push_back( Pair("refresh_time", m_score_maps[pc->m_id].refresh_time) );
        robj.push_back( Pair("self_obj", o) );
    }
    return HC_SUCCESS;
}

//获取排行信息
int weekRankings::getLastRankingsInfo(CharData* pc, json_spirit::Object &robj)
{
    //排行信息
    json_spirit::Array rank_list;
    int rank = 0;
    int last_rank_type = 0;
    for (std::list<char_weekRankings>::iterator it = m_last_Rankings.begin(); it != m_last_Rankings.end(); ++it)
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
                if (cdata->GetGuildId())
                {
                    o.push_back( Pair("guildname", Singleton<guildMgr>::Instance().getGuildName(cdata->GetGuildId())));
                }
            }
            o.push_back( Pair("score", it->score) );
            o.push_back( Pair("refresh_time", it->refresh_time) );
            rank_list.push_back(o);
        }
    }
    robj.push_back( Pair("rank_list", rank_list) );
    boost::shared_ptr<weekRankings_event> wre = getWeekRankingsEvent(last_rank_type);
    if (wre.get())
    {
        robj.push_back( Pair("rank_type", last_rank_type) );
        robj.push_back( Pair("rank_name", wre->name) );
    }
    return HC_SUCCESS;
}

//更新排行信息
void weekRankings::updateEventRankings(int cid, int type, int score)
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
        for (std::list<char_weekRankings>::iterator it = m_now_Rankings.begin(); it != m_now_Rankings.end(); ++it)
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
            char_weekRankings cr;
            cr.cid = cid;
            cr.rank_type = m_event_type;
            cr.score = m_score_maps[cid].score;
            cr.refresh_time = time_now;
            m_now_Rankings.push_back(cr);
        }
        m_now_Rankings.sort(compare_rank_score);
        if (m_now_Rankings.size() > 0)
        {
            m_top_min_score = m_now_Rankings.back().score;
        }
    }
    InsertSaveDb("replace into char_weekRankings_now (cid,rank_type,score,refresh_time) values (" + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(m_event_type) + "," + LEX_CAST_STR(m_score_maps[cid].score) + ",unix_timestamp())");
}

