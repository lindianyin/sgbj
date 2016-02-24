#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include <boost/thread.hpp>

#include "base_item.h"
#include "data.h"
#include "singleton.h"

using namespace json_spirit;

const int iThroneNickNum = 3;
const int iThroneData[iThroneNickNum][2] =
{
    {1, 50000}, //至尊一名，最低声望5W
    {2, 30000}, //战神两名，最低声望3W
    {4, 20000}, //统帅四名，最低声望2W
};

//参拜消耗，银币金币
const int iThroneCon[3][2] =
{
    {50000, 0},
    {0, 100},
    {0, 500},
};

enum nick_type
{
    nick_zhizun = 1,
    nick_zhanshen = 2,
    nick_tongshuai = 3,
};

//玩家王座积分排名
struct char_throne_Rankings
{
    int cid;    //角色id
    int type;   //称号类型
    int score;  //积分
    char_throne_Rankings()
    {
        cid = 0;
        score = 0;
        type = 0;
    }
};

//参拜奖励
struct throne_award
{
    int silver;
    int gold;
    std::list<Item> awards;
};

//参拜记录
struct throne_log
{
    time_t log_time;
    std::string memo;
};

class throneMgr
{
public:
    throneMgr();
    ~throneMgr();
    //更新活动
    void update();
    //获取排行信息
    int getLastRankingsInfo(CharData* pc, json_spirit::Object& robj);
    //获取参拜信息
    int getConInfo(CharData* pc, json_spirit::Object& robj);
    //增加参拜信息
    int addConLog(std::string msg);
    //参拜
    int congratulation(CharData* pc, json_spirit::mObject& o, json_spirit::Object& robj);
    bool actionState();
    void getAction(CharData& cdata, json_spirit::Array& elist);

private:
    std::list<char_throne_Rankings> m_last_Rankings;//上周玩家王座排行
    std::map<int, boost::shared_ptr<throne_award> > award_list;//参拜奖励
    std::list<throne_log> m_log_list;//参拜记录
};

int ProcessGetThroneInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessGetConInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessThroneCon(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

