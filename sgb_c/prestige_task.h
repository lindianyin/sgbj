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
    int race;//����
    int need;//��Ҫ����
    boost::shared_ptr<baseStronghold> target_stronghold;//Ŀ��ؿ�
    int reward_prestige;//��������
    void toObj(json_spirit::Object& obj);
};

//���������������
struct CharPrestigeTask
{
    CharPrestigeTask(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    int tid;
    int cur;//��ǰ����
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
    //��������
    void accept(boost::shared_ptr<basePrestigeTask> t);
    //�������
    int taskDone(int race, int reward_prestige, json_spirit::Object& robj);
    //�����б�
    int getList(int race, json_spirit::Array& rlist);
    //��������
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
    boost::shared_ptr<basePrestigeTask> getPrestigeTask(int tid);//��������id�������
private:
    std::map<int, boost::shared_ptr<basePrestigeTask> > m_total_tasks;//������������
};

int ProcessPrestigeTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessPrestigeTaskDone(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

