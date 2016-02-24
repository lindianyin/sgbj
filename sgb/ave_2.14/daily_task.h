#pragma once

#include <list>
#include <string>

#include "json_spirit.h"
#include "base_item.h"

#include "boost/smart_ptr/shared_ptr.hpp"

class CharData;

static volatile int iMaxDailyTaskReward = 6;        //每日任务奖励领取次数

//日常任务开放等级
const int iDailyTaskOpenLevel = 25;


enum daily_task_type
{
    daily_task_rest = 1,//购买军令
    daily_task_salary = 2,//俸禄
    //daily_task_trade = 3,//通商
    daily_task_race = 4,//竞技
    daily_task_equipt = 5,//强化
    daily_task_levy = 6,//征收
    daily_task_wash = 7,//洗髓
    daily_task_group_copy = 8,//战役
    daily_task_horse = 9,//战马
    daily_task_farm = 10,//屯田
    daily_task_trade = 11,//贸易
    daily_task_corp_jisi = 12,//军团祭天
    daily_task_corp_yanhui = 13,//军团宴会
    daily_task_guard = 14,//护送
    daily_task_guard_rob = 15,//劫取
    daily_task_guard_help = 16,//帮助好友护送
    daily_task_servant = 17,//家丁
    daily_task_servant_rescue = 18,//解救壮丁
    daily_task_servant_catch = 19,//抓捕壮丁
    daily_task_attak_boss1 = 20,//挑战神兽应龙
    daily_task_attak_boss4 = 21,//挑战神兽玄冥
    daily_task_camprace = 22,//阵营战
    daily_task_general_reborn = 23,//武将重生
    daily_task_general_train = 24,//武将训练
    daily_task_baoshi_exchange = 25,//兑换宝石
    daily_task_maze = 26,//八卦阵
    daily_task_corp_explore = 27,//军团探索
    daily_task_corp_ymsj = 28,//辕门射戟
    daily_task_corp_lottery = 29,//罗生盘
    daily_task_get_yushi = 30,//领取玉石
    daily_task_throne_con = 31,//参拜
};

//日常助手任务
struct base_daily_task
{
    int task_id;
    int task_pos;//排序用
    int can_findback;//是否属于可找回
    int needtimes;
    int score;
    std::string memo;
    bool to_obj(boost::shared_ptr<CharData> cdata, json_spirit::Object& robj);
};

//不同领取次数的任务奖励
struct char_daily_task_rewards
{
    int _needscore;
    int _needlevel;
    std::list<Item> _rewards;
    std::string toString(int level);
};

class dailyTaskMgr
{
public:
    void updateDailyTask(CharData& cdata, int task_id, int times = 1);
    void toObj(CharData& cdata, json_spirit::Object& info);
    void getnewReward(CharData& cdata, int reward_id, json_spirit::Object& info);
    int _newrewardTask(CharData& cdata, int reward_id, json_spirit::Object& robj);
    int findBack(boost::shared_ptr<CharData> cdata, int id);

    bool canGetReward(CharData& cdata);
    boost::shared_ptr<base_daily_task> getDaily_task(int id);
    static dailyTaskMgr* getInstance();
private:
    static dailyTaskMgr* m_handle;

    void reload();

    //日常任务奖励
    std::vector<char_daily_task_rewards> m_daily_task_rewards;
    std::map<int, boost::shared_ptr<base_daily_task> > m_base_daily_task_list;

};

