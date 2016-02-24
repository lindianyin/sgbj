#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "item.h"

struct CharData;

enum goal_type
{
    GOAL_TYPE_EMPTY = 0,        //空白任务
    GOAL_TYPE_CHAR_LEVEL,       //1玩家达到指定等级X
    GOAL_TYPE_EQUIPT1_QUALITY,  //2装备品质X的武器
    GOAL_TYPE_EQUIPT2_QUALITY,  //3装备品质X的盾牌
    GOAL_TYPE_EQUIPT3_QUALITY,  //4装备品质X的衣服
    GOAL_TYPE_EQUIPT4_QUALITY,  //5装备品质X的书
    GOAL_TYPE_METALLURGY_LEVEL, //6炼金房等级X
    GOAL_TYPE_SIGN,             //7签到X次数
    GOAL_TYPE_RECRUIT,          //8招募X次数
    GOAL_TYPE_UPGRADE_EQUIPT,   //9强化装备X次数
    GOAL_TYPE_LEVY,             //10征收X次数
    GOAL_TYPE_EXPLORE,          //11挂机探索洞穴X次数
    GOAL_TYPE_SHENLING_TOWER,   //12神灵塔指定层数X
    GOAL_TYPE_COMPOUND_HERO,    //13合成英雄X次数
    GOAL_TYPE_ARENA_WIN,        //14竞技场胜利X次数
    GOAL_TYPE_VIP,              //15VIPX
    GOAL_TYPE_HERO_PACK,        //16品质X英雄卡包
    GOAL_TYPE_COPY_MAP,         //17通关副本地图X
    GOAL_TYPE_WILD_WIN,         //18攻占城池X次数
    GOAL_TYPE_HERO_CNT,         //19英雄数量X个
    GOAL_TYPE_TREASURE_QUALITY, //20藏宝图X品质
    GOAL_TYPE_CASTLE_LEVEL,     //21城堡指定等级X
    GOAL_TYPE_HERO_STAR,        //22英雄星级X
    GOAL_TYPE_SILVER,           //23筹码数量X
    GOAL_TYPE_DECOMPOSE_HERO,   //24分解英雄X次数
    GOAL_TYPE_SMITHY_LEVEL,     //25铁匠铺等级X
    GOAL_TYPE_EPIC_HERO,        //26神将数量X
    GOAL_TYPE_RESIDENT,         //27居民数量X
    GOAL_TYPE_EQUIPT_QUALITY,   //28装备品质X的任意装备
    GOAL_TYPE_EQUIPT1_UPGRADE,  //29升级武器到等级X
    GOAL_TYPE_EQUIPT2_UPGRADE,  //30升级盾牌到等级X
    GOAL_TYPE_EQUIPT3_UPGRADE,  //31升级衣服到等级X
    GOAL_TYPE_EQUIPT4_UPGRADE,  //32升级书到等级X
};

struct baseGoalTask
{
    baseGoalTask()
    {
        id = 0;
        type = 0;
        need_extra = 0;
        need_level = 0;
        need_vip = 0;
        name = "a task";
        goal = "a goal";
        memo = "a task memo";
    }
    int id;
    int type;
    int spic;
    int need_extra;
    int need_level;//任务所需等级
    int need_vip;//额外奖励所需vip
    std::string name;//任务标题
    std::string goal;//任务目标
    std::string memo;//任务描述
    std::list<Item> reward;//奖励
    std::list<Item> vip_reward;//奖励
    void loadRewards();
    void toObj(json_spirit::Object& obj);
    void toSimpleObj(json_spirit::Object& obj);
};

//功能目标
struct baseGoalLevelReward
{
    int id;
    int need_level;
    std::string name;//任务标题
    std::string memo;//任务描述
    std::list<Item> reward;//奖励
    void loadRewards();
    void toObj(json_spirit::Object& obj);
    void toSimpleObj(json_spirit::Object& obj);
};

//目标商品
struct baseGoalGoods
{
    int id;
    int need_level;
    int org_cost;
    int cost;
    Item reward;
};

//玩家目标任务
struct CharGoalTask
{
    enum reward_state
    {
        STATE_INIT = 0,
        STATE_GET = 1,//可以领取
        STATE_ALREADY = 2,//已经领取
    };
    int cid;
    int tid;
    int cur;
    int normal_state;
    int vip_state;
    bool done;
    boost::shared_ptr<baseGoalTask> m_base;
    void Save();
    void toObj(json_spirit::Object& obj);
    void toSimpleObj(json_spirit::Object& obj);
};

struct CharGoal
{
    int cid;
    std::map<int, boost::shared_ptr<CharGoalTask> > m_all_tasks;
    
    void acceptTask(boost::shared_ptr<baseGoalTask> t);
    //任务列表
    int getList(int level, json_spirit::Array& rlist);
    //更新任务
    int updateTask(int goal_type, int extra);
    //更新vip
    int updateVip(int vip);
    //更新等级
    int updateLevel(int level);
    //领取目标奖励
    int getReward(int id, int type, json_spirit::Object& robj);
    //领取目标奖励
    int getLevelReward(int level, json_spirit::Object& robj);
    boost::shared_ptr<CharGoalTask> getCharGoalTask(int tid);
    void load();
    int getCharGoalState(int& type, int& need_level, int& id);
    void getButton(json_spirit::Array& list);
    void getCharGoalTag();
};

class goalMgr
{
public:
    goalMgr();
    boost::shared_ptr<CharGoal> getCharGoal(int cid);
    boost::shared_ptr<baseGoalTask> getGoalTask(int tid);    //根据任务id获得任务
    std::map<int, boost::shared_ptr<baseGoalTask> >& getGoalTasks() {return m_total_tasks;}
    boost::shared_ptr<baseGoalLevelReward> getLevelReward(int id);
    std::map<int, boost::shared_ptr<baseGoalLevelReward> >& getLevelRewards() {return m_level_rewards;}
    void getGoalGood(int level, json_spirit::Array& list);
    boost::shared_ptr<baseGoalGoods> getBaseGood(int id);
    int updateTask(int cid, int goal_type, int extra);
    int updateVip(int cid, int vip);
    int updateLevel(int cid, int level);
private:
    std::map<int, boost::shared_ptr<baseGoalTask> > m_total_tasks;//所有任务
    std::map<int, boost::shared_ptr<baseGoalLevelReward> > m_level_rewards;//等级奖励
    std::map<int, boost::shared_ptr<baseGoalGoods> > m_goods;
    std::map<int, boost::shared_ptr<CharGoal> > m_char_goals;
};

//获取目标任务列表
int ProcessGoalTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//目标等级奖励
int ProcessGoalLevelList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//领取目标奖励
int ProcessGoalReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//目标商品
int ProcessGoalShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//购买目标商品
int ProcessBuyGoalGood(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

