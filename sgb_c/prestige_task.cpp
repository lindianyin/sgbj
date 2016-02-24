
#include "prestige_task.h"
#include "ThreadLocalSingleton.h"
#include "errcode_def.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "SaveDb.h"
#include "spls_timer.h"
#include "city.h"
#include "shenling.h"
#include "copy.h"

Database& GetDb();
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

basePrestigeTask::basePrestigeTask()
{
    id = 0;
    race = 0;
    need = 0;
    reward_prestige = 0;
}

void basePrestigeTask::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", id));
    obj.push_back( Pair("race", race));
    obj.push_back( Pair("reward_prestige", reward_prestige));
    obj.push_back( Pair("need", need));
    if (target_stronghold.get())
    {
        json_spirit::Object strongholdInfo;
        strongholdInfo.push_back( Pair("name", target_stronghold->m_name) );
        strongholdInfo.push_back( Pair("mapid", target_stronghold->m_mapid) );
        strongholdInfo.push_back( Pair("stageid", target_stronghold->m_stageid) );
        strongholdInfo.push_back( Pair("strongholdid", target_stronghold->m_id) );
        strongholdInfo.push_back( Pair("type", target_stronghold->m_type) );
        obj.push_back( Pair("strongholdInfo", strongholdInfo));
    }
    return;
}

void CharPrestigeTask::Save()
{
    InsertSaveDb("update char_prestige_tasks set cur='" + LEX_CAST_STR(cur)
        + "',state='" + LEX_CAST_STR(state)
        + "' where cid=" + LEX_CAST_STR(m_charData.m_id)
        + " and tid=" + LEX_CAST_STR(tid)
        );
}

void CharPrestigeTask::toObj(json_spirit::Object& obj)
{
    //基础信息，描述名字
    if (m_task.get())
    {
        m_task->toObj(obj);
    }
    obj.push_back( Pair("cur", cur) );
    obj.push_back( Pair("state", state) );
    return;
}

//接受任务
void CharAllPrestigeTasks::accept(boost::shared_ptr<basePrestigeTask> t)
{
    if (t.get())
    {
        boost::shared_ptr<CharPrestigeTask> ct(new CharPrestigeTask(m_charData));
        ct->tid = t->id;
        ct->m_task = t;
        ct->cur = 0;
        ct->state = 0;
        m_all_prestige_tasks[t->id] = ct;
        InsertSaveDb("insert into char_prestige_tasks set cur='" + LEX_CAST_STR(ct->cur)
        + "',state='" + LEX_CAST_STR(ct->state)
        + "',cid=" + LEX_CAST_STR(m_charData.m_id)
        + ",tid=" + LEX_CAST_STR(ct->tid)
        );
    }
}

//任务列表
int CharAllPrestigeTasks::getList(int race, json_spirit::Array& rlist)
{
    std::map<int, boost::shared_ptr<basePrestigeTask> > tasks = Singleton<PrestigeTaskMgr>::Instance().getPrestigeTasks();
    if (tasks.size() > m_all_prestige_tasks.size())
    {
        for(int i = m_all_prestige_tasks.size(); i < tasks.size(); ++i)
        {
            boost::shared_ptr<basePrestigeTask> bt = Singleton<PrestigeTaskMgr>::Instance().getPrestigeTask(i+1);
            if (bt.get())
            {
                accept(bt);
            }
        }
    }
    int tmp = 0;
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<CharPrestigeTask> >::iterator it = m_all_prestige_tasks.begin();
    while (it != m_all_prestige_tasks.end())
    {
        if (it->second.get() && it->second->m_task.get())
        {
            CharPrestigeTask& ct = *(it->second.get());
            if (race != 0 && race != ct.m_task->race)
            {
                ++it;
                continue;
            }
            if (ct.m_task->reward_prestige != tmp)
            {
                if (tmp != 0)
                {
                    json_spirit::Object robj;
                    robj.push_back( Pair("reward_prestige", tmp) );
                    robj.push_back( Pair("list", list) );
                    rlist.push_back(robj);
                }
                list.clear();
                tmp = ct.m_task->reward_prestige;
            }
            json_spirit::Object obj;
            ct.toObj(obj);
            list.push_back(obj);
        }
        ++it;
    }
    //最后一档
    if (tmp != 0)
    {
        json_spirit::Object robj;
        robj.push_back( Pair("reward_prestige", tmp) );
        robj.push_back( Pair("list", list) );
        rlist.push_back(robj);
    }
    return HC_SUCCESS;
}

//更新任务
void CharAllPrestigeTasks::update(int strongholdid)
{
    if (m_charData.m_level < iPrestigeOpenLevel)
    {
        return;
    }
    std::map<int, boost::shared_ptr<CharPrestigeTask> >::iterator it = m_all_prestige_tasks.begin();
    while (it != m_all_prestige_tasks.end())
    {
        if (it->second.get() && it->second->m_task.get() && it->second->m_task->target_stronghold.get())
        {
            CharPrestigeTask& ct = *(it->second.get());
            bool need_save = false;
            if (ct.state == 0 && ct.m_task->target_stronghold->m_id == strongholdid)
            {
                ++ct.cur;
                need_save = true;
            }
            if (ct.cur >= ct.m_task->need)
            {
                ct.cur = ct.m_task->need;
                if (ct.state == 0)
                {
                    ct.state = 1;
                    m_charData.updateTopButton(top_button_prestige_task, ct.m_task->race);
                }
            }
            //目标状态有变动
            if (need_save)
            {
                ct.Save();
            }
        }
        ++it;
    }
    return;
}

//任务完成
int CharAllPrestigeTasks::taskDone(int race, int reward_prestige, json_spirit::Object& robj)
{
    int cnt = 0;
    std::map<int, boost::shared_ptr<CharPrestigeTask> >::iterator it = m_all_prestige_tasks.begin();
    while (it != m_all_prestige_tasks.end())
    {
        if (it->second.get() && it->second->m_task.get() && it->second->m_task->target_stronghold.get())
        {
            CharPrestigeTask& ct = *(it->second.get());
            if (race != ct.m_task->race || reward_prestige != ct.m_task->reward_prestige)
            {
                ++it;
                continue;
            }
            if (ct.state != 1)
            {
                ++it;
                continue;
            }
            ++cnt;
            ct.state = 2;
            ct.Save();
        }
        ++it;
    }
    m_charData.updateTopButton(top_button_prestige_task, getActive());
    if (cnt > 0)
    {
        std::list<Item> items;
        Item tmp(ITEM_TYPE_CURRENCY, CURRENCY_ID_PRESTIGE_BEGIN+race, reward_prestige * cnt, 0);
        items.push_back(tmp);
        giveLoots(&m_charData,items,NULL,&robj,true,loot_prestige_task);
    }
    return HC_SUCCESS;
}

void CharAllPrestigeTasks::load()
{
    Query q(GetDb());
    m_all_prestige_tasks.clear();
    //从数据库读任务记录
    q.get_result("select tid,cur,state from char_prestige_tasks where cid=" + LEX_CAST_STR(m_charData.m_id) + " order by tid");
    while (q.fetch_row())
    {
        int tid = q.getval();
        int cur = q.getval();
        int state = q.getval();
        boost::shared_ptr<basePrestigeTask> bt = Singleton<PrestigeTaskMgr>::Instance().getPrestigeTask(tid);
        if (!bt.get())
        {
            continue;
        }
        boost::shared_ptr<CharPrestigeTask> ct(new CharPrestigeTask(m_charData));
        ct->tid = tid;
        ct->m_task = bt;
        ct->cur = cur;
        ct->state = state;
        m_all_prestige_tasks[bt->id] = ct;
    }
    q.free_result();
}

int CharAllPrestigeTasks::getActive()
{
    std::map<int, boost::shared_ptr<CharPrestigeTask> >::iterator it = m_all_prestige_tasks.begin();
    while (it != m_all_prestige_tasks.end())
    {
        if (it->second.get() && it->second->m_task.get() && it->second->m_task->target_stronghold.get())
        {
            CharPrestigeTask& ct = *(it->second.get());
            if (ct.state == 1)
            {
                return it->second->m_task->race;
            }
        }
        ++it;
    }
    return 0;
}

PrestigeTaskMgr::PrestigeTaskMgr()
{
    Query q(GetDb());
    q.get_result("select tid,race,need_times,reward,strongholdid from base_prestige_tasks where 1 order by tid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        basePrestigeTask* pt = new basePrestigeTask();
        pt->id = id;
        pt->race = q.getval();
        pt->need = q.getval();
        pt->reward_prestige= q.getval();
        int strongholdid = q.getval();
        if (strongholdid != 0)
        {
            pt->target_stronghold = Singleton<mapMgr>::Instance().GetBaseStrongholdData(strongholdid);
        }
        boost::shared_ptr<basePrestigeTask> bt(pt);
        m_total_tasks[id] = bt;
    }
    q.free_result();
}

void PrestigeTaskMgr::getButton(CharData* pc, json_spirit::Array& list)
{
    if (pc->m_level > iPrestigeOpenLevel)
    {
        int active = pc->m_prestige_tasks.getActive();
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_prestige_task) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    return;
}

boost::shared_ptr<basePrestigeTask> PrestigeTaskMgr::getPrestigeTask(int tid) //根据任务id获得任务
{
    if (m_total_tasks.find(tid) != m_total_tasks.end())
    {
        //cout << "TRUE,taskMgr::getTask id=" << tid << endl;
        return m_total_tasks[tid];
    }
    else
    {
        //cout << "FALSE,taskMgr::getTask id=" << tid << endl;
        boost::shared_ptr<basePrestigeTask> tt;
        return tt;
    }
}

int ProcessPrestigeTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int race = 0;
    READ_INT_FROM_MOBJ(race, o, "race");
    robj.push_back( Pair("race", race) );
    json_spirit::Array tlist;
    cdata->m_prestige_tasks.getList(race, tlist);
    robj.push_back( Pair("task_list", tlist) );
    return HC_SUCCESS;
}

int ProcessPrestigeTaskDone(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int race = 0, reward_prestige = 0;
    READ_INT_FROM_MOBJ(race, o, "race");
    READ_INT_FROM_MOBJ(reward_prestige, o, "reward_prestige");
    robj.push_back( Pair("race", race) );
    return cdata->m_prestige_tasks.taskDone(race, reward_prestige, robj);
}

