#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "map.h"

struct CharData;

const int iTaskDailyCount = 8;

enum task_goal_type
{
    GOAL_EMPTY = 0,         //空白任务
    GOAL_STRONGHOLD = 1,    //通关指定关卡(关卡id，1)
    GOAL_CHAR_LEVEL,        //2玩家达到指定等级(0，等级)
    GOAL_COMPOUND_HERO,     //3合成指定星级英雄(星级，次数)
    GOAL_COMPOUND_EQUIPT,   //4合成指定品质装备(品质，次数)
    GOAL_UPGRADE_EQUIPT,    //5强化指定等级装备(等级，次数)
    GOAL_DECOMPOSE_HERO,    //6分解指定星级英雄(星级，次数)
    GOAL_SMELT_HERO,        //7熔炼指定星级英雄(星级，次数)
    GOAL_GOLDEN_HERO,       //8点金指定星级英雄(星级，次数)
    GOAL_ARENA,             //9 竞技场累积次数(0，次数)
    GOAL_ARENA_WIN,         //10 竞技场胜利累积次数(0，次数)
    GOAL_SHENLING_TOWER,    //11神灵塔指定层数(0，层数)
    GOAL_CASTLE_LEVEL,      //12城堡指定等级(0，等级)
    GOAL_METALLURGY_LEVEL,  //13炼金房等级(0，等级)
    GOAL_SMITHY_LEVEL,      //14铁匠铺等级(0，等级)
    GOAL_BARRACKS_LEVEL,    //15练兵营等级(0，等级)
    GOAL_OFFLINE_REWARD,    //16挂机产出(0，1)
    GOAL_MAP,               //17通关指定地图(地图id，1)
    GOAL_COLLECT,           //18收藏游戏(0，1)
    GOAL_HERO_PACK,         //19英雄卡包(卡包id，次数)
    GOAL_MALL_GEM,          //20购买道具(道具id，次数)
    GOAL_HERO_STAR,         //21获得指定星级英雄(星级，次数)
    GOAL_EQUIPT_QUALITY,    //22获得指定品质装备(品质，次数)
    GOAL_STRONGHOLD_ATTACK, //23攻打指定关卡(关卡id，次数)
    GOAL_COPY_ATTACK,       //24攻打指定副本(副本id，次数)
    GOAL_EXPLORE,           //25挂机探索洞穴(洞穴id，次数)
    GOAL_LEVY,              //26城堡征收(0，1)
    GOAL_RECRUIT,           //27城堡招募(0，1)
    GOAL_TREASURE,          //28藏宝图(0，1)
    GOAL_HERO,              //29获得指定英雄(英雄id，次数)
    GOAL_UPGRADE_SKILL,     //30升级技能(技能id，等级)
    GOAL_DONE_DAILY_TASK,   //31完成日常任务次数(0，次数)
    GOAL_SET_MAGIC,         //32设置出战魔法(魔法id，1)


    //每日任务目标
    GOAL_DAILY_EXPLORE = 1001,//1001挂机探索(0，次数)
    GOAL_DAILY_RECRUIT,     //1002招募(0，次数)
    GOAL_DAILY_LEVY,        //1003征收(0，次数)
    GOAL_DAILY_ARENA,       //1004竞技场(0，次数)
    GOAL_DAILY_ARENA_WIN,   //1005竞技场胜利(0，次数)
    GOAL_DAILY_TREASURE,    //1006藏宝图(0，次数)
    GOAL_DAILY_STRONGHOLD,  //1007攻打随机关卡(随机生成id，次数)
    GOAL_DAILY_COPY,        //1008攻打随机副本(随机生成id，次数)
    GOAL_DAILY_PK,          //1009筹码pk(0，次数)
    GOAL_DAILY_HERO_PACK_GOLD,//1010金币开包(0，次数)
    GOAL_DAILY_HERO_PACK_SILVER,//1011筹码开包(0，次数)
    GOAL_DAILY_LEVY_GOLD,   //1012强制征收(0，次数)
    GOAL_DAILY_SHENLING_ATTACK,//1013攻打神灵塔(0，次数)
    GOAL_DAILY_TREASURE_ROB,//1014打劫宝藏(0，次数)
    GOAL_DAILY_SCORE_TASK,  //1015每日必做积分(0，积分)
    GOAL_DAILY_GUILD_MOSHEN,//1016公会魔神(0，次数)
    GOAL_DAILY_COPY_ATTACK, //1017攻打任意副本(0，次数)
    GOAL_DAILY_WILD_LEVY,   //1018城外征收(0，筹码数量)
    GOAL_DAILY_UPGRADE_EQUIPT,//1019强化装备(0，次数)
    GOAL_DAILY_COMPOUND_HERO, //1020合成指定星级英雄(星级，次数)
    GOAL_DAILY_DECOMPOSE_HERO,//1021分解指定星级英雄(星级，次数)
    GOAL_DAILY_GOLDEN_HERO, //1022点金指定星级英雄(星级，次数)
    GOAL_DAILY_BOSS,        //1023攻击boss(0，次数)
};

enum task_type
{
    TASK_TYPE_NORMAL = 1,//主线
    TASK_TYPE_SUB = 2,//支线
    //活动
    TASK_TYPE_DAILY = 4,//日常
    //公会
};

enum task_update_type
{
    TASK_UPDATE = 1,
    TASK_ADD = 2,
    TASK_DELETE = 3,
};

//任务目标
struct baseGoal
{
    int type;//目标类型
    int need[2];//目标参数(唯一id或者等级层数，次数)
    std::string memo;//目标描述
    boost::shared_ptr<baseStronghold> target_stronghold;//目标关卡
    void toObj(json_spirit::Object& obj);
};

struct baseTask
{
    baseTask();
    int id;
    int type;//（任务类型，主线，支线，活动，日常，工会）
    int task_id;//（任务类型下的id序号）
    std::string name;//任务标题
    std::string memo;//任务描述
    int pre_task_type;//前置任务类型
    int pre_task_id;//前置任务id
    int need_race;//任务所需种族(0表示任意种族)
    int done_cost;//消耗金币任务自动完成
    int guide_id_get;//领取任务对应引导
    int guide_id_reward;//领取任务奖励对应引导
    std::list<Item> reward;//奖励
    std::list<boost::shared_ptr<baseGoal> > goals;//任务目标
    std::list<boost::shared_ptr<baseTask> > m_child_tasks;//后续任务
    void loadRewards();
    void loadGoals();
    void toObj(json_spirit::Object& obj);
    void toSimpleObj(json_spirit::Object& obj);
    void ChildList(json_spirit::Object& obj);
    void reload();
};

//任务目标
struct CharTaskGoal
{
    int type;//目标类型
    int cur;//当前计数
    int extra;//每日任务用来替换目标
    boost::shared_ptr<baseGoal> base_goal;
    bool done;
};

//玩家任务数据
struct CharTask
{
    CharTask(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    int tid;
    bool done;
    boost::shared_ptr<baseTask> m_task;
    std::list<boost::shared_ptr<CharTaskGoal> > m_goals;//任务目标
    void Save();
    void toObj(bool simple, json_spirit::Object& obj);
};

struct CharAllTasks
{
    CharAllTasks(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    std::map<int, boost::shared_ptr<CharTask> > m_all_tasks;

    //接受任务
    void acceptTask(boost::shared_ptr<baseTask> t);
    //完成任务
    int taskDone(int id, int cost, json_spirit::Object& robj);
    //任务列表
    int getList(int type, json_spirit::Array& rlist);
    //更新任务
    int updateTask(int goal_type, int n1 = 0, int n2 = 0);
    boost::shared_ptr<CharTask> getCharTask(int id);
    //获取玩家某类型第一个任务状态
    void getCharTaskState(int type, int& tid, int& state);
    bool taskExploreSpecial();
    void load();
    int getCharDailyTaskCnt();
    int getCharDailyState();
    void dailyUpdate();
    void getButton(json_spirit::Array& list);
};

class taskMgr
{
public:
    taskMgr();
    std::map<int, boost::shared_ptr<baseTask> >& getTasks() {return m_total_tasks;}
    std::vector<int>& getDailyTasks() {return m_daily_tasks;}
    boost::shared_ptr<baseTask> getTask(int tid);    //根据任务id获得任务
    boost::shared_ptr<baseTask> getTask(int type, int task_id); //根据任务类型和分类id获得任务
    void addTask(int tid, boost::shared_ptr<baseTask> bt);
    void removeTask(int tid);
    void reLink();
    int newChar(boost::shared_ptr<CharData> cdata);
    int RandomDailyTask();
private:
    std::map<int, boost::shared_ptr<baseTask> > m_total_tasks;        //所有任务
    std::vector<int> m_daily_tasks;        //每日初始日常任务
};

int ProcessTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessTaskInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessTaskDone(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

