
#pragma once

#include <map>
#include <list>
#include "json_spirit.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "base_item.h"
#include "net.h"

struct CharData;

enum enum_goals_type
{
    goals_type_equipt = 1,
    goals_type_level = 2,
    goals_type_daily_score = 3,
    goals_type_maze = 4,
    goals_type_attack_power = 5,
    goals_type_general = 6,
    goals_type_guard = 7,
    goals_type_race = 8,
};

struct Goals
{
    int id;
    int type;
    int param;
    void toObj(json_spirit::Object& obj);
    void checkDone(CharData& cdata, int c_param);
};

struct baseSevenGoals
{
    const json_spirit::Array& getIArray() const;
    void updateObj();    
    
    json_spirit::Array m_item_list;
    int m_day;    //目标开启天数
    std::list<Goals> m_glist;        //目标列表
    std::list<Item> m_ilist;        //奖励列表
};

class seven_Goals_mgr
{
public:
    seven_Goals_mgr();
    baseSevenGoals* getBaseSevenGoals(int day);
    void updateGoals(CharData& cdata, int day, int type = 0, int param = 0);
    int quarGoalInfo(CharData& cdata, int day, json_spirit::Object& robj);
    int getReward(CharData& cdata, int day, json_spirit::Object& robj);
    bool canGetReward(CharData& cdata);
    int getState(CharData& cdata);
    bool checkActionFinish(CharData& cdata);
    void getAction(CharData* pc, json_spirit::Array& blist);
private:
    void load();
    std::map<int, baseSevenGoals* > m_base_seven_goals;
};

//查询目标信息 cmd ：querySevenGoals
int ProcessQuerySevenGoals(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//领取目标奖励 cmd ：getSevenGoals
int ProcessGetSevenGoals(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

