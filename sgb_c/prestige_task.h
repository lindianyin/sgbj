#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "map.h"

struct CharData;

struct basePrestigeTask
{
    basePrestigeTask();
    int id;
    int race;//种族
    int need;//需要次数
    boost::shared_ptr<baseStronghold> target_stronghold;//目标关卡
    int reward_prestige;//奖励声望
    void toObj(json_spirit::Object& obj);
};

//玩家声望任务数据
struct CharPrestigeTask
{
    CharPrestigeTask(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    int tid;
    int cur;//当前计数
    int state;
    boost::shared_ptr<basePrestigeTask> m_task;
    void Save();
    void toObj(json_spirit::Object& obj);
};

struct CharAllPrestigeTasks
{
    CharAllPrestigeTasks(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    std::map<int, boost::shared_ptr<CharPrestigeTask> > m_all_prestige_tasks;
    //接受任务
    void accept(boost::shared_ptr<basePrestigeTask> t);
    //完成任务
    int taskDone(int race, int reward_prestige, json_spirit::Object& robj);
    //任务列表
    int getList(int race, json_spirit::Array& rlist);
    //更新任务
    void update(int strongholdid);
    void load();
    int getActive();
};

class PrestigeTaskMgr
{
public:
    PrestigeTaskMgr();
    void getButton(CharData* pc, json_spirit::Array& list);
    std::map<int, boost::shared_ptr<basePrestigeTask> >& getPrestigeTasks() {return m_total_tasks;}
    boost::shared_ptr<basePrestigeTask> getPrestigeTask(int tid);//根据任务id获得任务
private:
    std::map<int, boost::shared_ptr<basePrestigeTask> > m_total_tasks;//所有声望任务
};

int ProcessPrestigeTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessPrestigeTaskDone(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

