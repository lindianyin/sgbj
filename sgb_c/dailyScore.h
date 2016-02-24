#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "map.h"

static volatile int iMaxDailyTaskReward = 6;//每日必做任务奖励领取次数

struct CharData;

enum daily_score_type
{
    DAILY_SCORE_EMPTY = 0,         //空白任务
    DAILY_SCORE_STRONGHOLD = 1,    //攻打关卡
    DAILY_SCORE_UPGRADE_EQUIPT,    //2强化装备
    DAILY_SCORE_DECOMPOSE_HERO,    //3分解英雄
    DAILY_SCORE_SMELT_HERO,        //4熔炼英雄
    DAILY_SCORE_ARENA,             //5 竞技场
    DAILY_SCORE_COPY_ATTACK,       //6攻打副本
    DAILY_SCORE_SHENLING_TOWER,    //7神灵塔
    DAILY_SCORE_WILD,              //8城外
    DAILY_SCORE_GOLD,              //9金币消耗(双倍)
};

//每日必做任务
struct baseScoreTask
{
    baseScoreTask();
    int id;
    int type;//任务类型
    int need;//任务条件
    int quality;//任务品质
    std::string name;//任务标题
    std::string memo;//任务描述
    int done_cost;//消耗金币任务自动完成
    int score;//任务奖励积分
    int silver;//任务奖励筹码
    void toObj(json_spirit::Object& obj);
};

//玩家每日必做任务数据
struct CharScoreTask
{
    CharScoreTask(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    int m_tid;
    int m_cur;
    int m_state;
    bool m_done;
    boost::shared_ptr<baseScoreTask> m_task;
    void Save();
    void toObj(json_spirit::Object& obj);
};

struct CharScoreSpecial
{
    CharScoreSpecial(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    int m_tid;
    int m_type;
    bool m_done;
    std::string m_name;
    std::string m_memo;
    void Save();
    void toObj(json_spirit::Object& obj);
};

struct CharDailyScore
{
    CharDailyScore(CharData& c)
    :m_charData(c)
    {
        m_special_reset = 0;
        m_score = 0;
        m_task_cnt = 0;
        m_double = false;
        m_reward_num = 0;
    }
    CharData& m_charData;
    int m_special_reset;//目标任务刷新次数
    std::list<boost::shared_ptr<CharScoreSpecial> > m_specials;//目标任务
    std::map<int, boost::shared_ptr<CharScoreTask> > m_score_tasks;//积分任务
    boost::shared_ptr<CharScoreTask> m_cur_task;//当前领取任务
    int m_score;//积分
    int m_task_cnt;//完成任务数
    bool m_double;//是否翻倍
    int m_reward_num;//奖励领取了几个
    
    //获取积分任务
    boost::shared_ptr<CharScoreTask> getCharScoreTask(int id);
    //接受积分任务
    int acceptScoreTask(int tid);
    //放弃积分任务
    int cancelScoreTask(int tid);
    //刷新积分任务
    void refreshScoreTask(bool add = false);
    //完成积分任务
    int scoreTaskDone(int id, int cost, json_spirit::Object& robj);
    //目标积分任务
    boost::shared_ptr<CharScoreSpecial> getCharScoreSpecial(int id);
    //刷新目标任务
    void refreshSpecial();
    //任务列表
    int getScoreTaskList(json_spirit::Object& robj);
    //奖励列表
    int getRewardList(json_spirit::Object& robj);
    //更新任务
    void updateTask(int type, int n = 1);
    int getReward(json_spirit::Object& robj);
    void load();
    void SaveData();
    void dailyUpdate();
    int getActive();
};

struct dailyScoreRewards
{
    int needscore;
    Item rewards;
};

class dailyScoreMgr
{
public:
    dailyScoreMgr();
    void getButton(CharData* pc, json_spirit::Array& list);
    boost::shared_ptr<baseScoreTask> getTask(int tid);    //根据任务id获得任务
    bool canGetReward(int cnt, int score);
    void getRewardInfo(json_spirit::Object& robj);
    int getReward(CharData& cdata, int reward_id, int score, json_spirit::Object& robj);
    int RandomGoldTask(bool must = false);
    int RandomTask();
    int RandomFirstTask(int type);
    void dailyUpdate();
private:
    int m_gold_tid;
    std::vector<int> m_gailvs;
    std::vector<int> m_all_task_id;
    std::map<int, boost::shared_ptr<baseScoreTask> > m_total_tasks;//所有任务
    std::vector<dailyScoreRewards> m_daily_task_rewards;//所有奖励
};

int ProcessDailyScoreTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessDailyScoreRewardList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessDailyScoreTaskInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessDailyScoreDeal(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

