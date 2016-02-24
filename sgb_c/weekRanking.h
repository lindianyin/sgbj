#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include <boost/thread.hpp>

#include "item.h"
#include "data.h"

using namespace json_spirit;

const int iRewardMinRank = 5;

enum week_ranking_type
{
    week_ranking_stronghold = 1,
    week_ranking_copy = 2,
    week_ranking_gold = 3,
    week_ranking_prestige = 4,
    week_ranking_shenling = 5,
};

//角色活动积分排名
struct char_weekRankings
{
    int cid;    //角色id
    int rank_type;//排行类别
    int score;    //积分
    time_t refresh_time;    //更新时间
    char_weekRankings()
    {
        cid = 0;
        score = 0;
        rank_type = 0;
        refresh_time = 0;
    }
};

//排名活动的奖励
struct weekRankings_event_award
{
    int rank;
    std::list<Item> awards;

    weekRankings_event_award()
    {
        rank = 0;
    }
};

//排名活动
struct weekRankings_event
{
    int type;
    std::string name;
    std::string memo;
    std::map<int, boost::shared_ptr<weekRankings_event_award> > rankings_list;
};

class weekRankings
{
public:
    weekRankings();
    void getButton(CharData* pc, json_spirit::Array& list);
    boost::shared_ptr<weekRankings_event> getWeekRankingsEvent(int type);
    //活动奖励生成
    void RankingsReward();
    //更新活动
    void update();
    //获取排行信息
    int getRankingsInfo(CharData* pc, json_spirit::Object& robj);
    int getLastRankingsInfo(CharData* pc, json_spirit::Object& robj);
    //更新排行信息
    void updateEventRankings(int cid, int type, int score);

private:
    int m_event_type;
    int m_top_min_score;
    std::map<int, char_weekRankings> m_score_maps;//所有玩家积分
    std::list<char_weekRankings> m_now_Rankings;
    std::list<char_weekRankings> m_last_Rankings;

    std::vector<boost::shared_ptr<weekRankings_event> > m_event_list;//排行活动总列表
};

//本周排行信息
int ProcessGetWeekRankingsInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//上周排行信息
int ProcessGetLastWeekRankingsInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

