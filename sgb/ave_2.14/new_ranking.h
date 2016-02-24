#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include <boost/thread.hpp>

#include "base_item.h"
#include "data.h"

using namespace json_spirit;

const int iRewardMinRank = 5;

enum new_rankings_type
{
    rankings_event_stronghold = 1,
    rankings_event_silver = 2,
    rankings_event_recharge = 3,
    rankings_event_gold = 4,
    rankings_event_wash = 5,
    rankings_event_baoshi = 6,
    rankings_event_chengzhang = 7,
};

//角色活动积分排名
struct new_char_Rankings
{
    int cid;    //角色id
    int rank_type;//排行类别
    int score;    //积分
    time_t refresh_time;    //更新时间
    int state;//0未领取1已领取
    new_char_Rankings()
    {
        cid = 0;
        score = 0;
        rank_type = 0;
        refresh_time = 0;
        state = 0;
    }
};

//排名活动的奖励
struct new_rankings_event_award
{
    int rank;
    std::list<Item> awards;

    new_rankings_event_award()
    {
        rank = 0;
    }
};

//排名活动
struct new_rankings_event
{
    int type;
    std::string memo;
    //std::string mail_title;
    //std::string mail_content;

    std::map<int, boost::shared_ptr<new_rankings_event_award> > rankings_list;
};

class newRankings
{
public:
    static newRankings* getInstance();
    void reload();
    void getAction(CharData* pc, json_spirit::Array& blist);

    //活动奖励生成
    void RankingsReward();
    //更新活动
    void update();
    //获取排行信息
    int getRankingsInfo(CharData* pc, json_spirit::Object& robj);
    int getLastRankingsInfo(CharData* pc, json_spirit::Object& robj);
    //更新排行信息
    void updateEventRankings(int cid, int type, int score);
    //奖励状态
    bool getRewardState(int cid, int& type, int& rank);
    bool setRewardGet(int cid);
    //领取奖励
    int getRankRewards(int cid, json_spirit::Object& robj);

private:
    static newRankings* m_handle;
    int m_event_type;

    int m_top_min_score;
    std::map<int, new_char_Rankings> m_score_maps;//所有玩家积分
    std::list<new_char_Rankings> m_now_Rankings;
    std::list<new_char_Rankings> m_last_Rankings;

    std::vector<new_rankings_event> m_event_list;//排行活动总列表
};


