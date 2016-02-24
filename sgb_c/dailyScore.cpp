
#include "dailyScore.h"
#include "ThreadLocalSingleton.h"
#include "errcode_def.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "SaveDb.h"
#include "spls_timer.h"
#include <boost/random/discrete_distribution.hpp>
#include "relation.h"

Database& GetDb();
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

extern std::string strDailyScoreGet;

int ProcessDailyScoreTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_score_tasks.getScoreTaskList(robj);
}

int ProcessDailyScoreRewardList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_score_tasks.getRewardList(robj);
}

int ProcessDailyScoreTaskInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 0, tid = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_INT_FROM_MOBJ(tid, o, "id");
    robj.push_back( Pair("type", type));
    if (type == 1)
    {
        boost::shared_ptr<CharScoreTask> ct = cdata->m_score_tasks.getCharScoreTask(tid);
        if (ct.get())
        {
            ct->toObj(robj);
            return HC_SUCCESS;
        }
    }
    else if(type == 2)
    {
        boost::shared_ptr<CharScoreSpecial> ct = cdata->m_score_tasks.getCharScoreSpecial(tid);
        if (ct.get())
        {
            ct->toObj(robj);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int ProcessDailyScoreDeal(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int purpose = 0, tid = 0;
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    READ_INT_FROM_MOBJ(tid, o, "id");
    robj.push_back( Pair("purpose", purpose));
    if (purpose == 1)//刷新任务
    {
        int free_times = iDailyScoreTaskRefreshFree - cdata->queryExtraData(char_data_type_daily,char_data_daily_daily_score_refresh);
        if (free_times > 0)
        {
            cdata->m_score_tasks.refreshScoreTask(true);
            return HC_SUCCESS;
        }
        else
        {
            if (cdata->subGold(iDailyScoreTaskRefreshCost, gold_cost_daily_score_refresh) < 0)
                return HC_ERROR_NOT_ENOUGH_GOLD;
            cdata->m_score_tasks.refreshScoreTask(true);
            return HC_SUCCESS;
        }
    }
    else if (purpose == 2)//接受任务
    {
        return cdata->m_score_tasks.acceptScoreTask(tid);
    }
    else if (purpose == 3)//完成任务
    {
        return cdata->m_score_tasks.scoreTaskDone(tid, 0, robj);
    }
    else if (purpose == 4)//立即完成
    {
        return cdata->m_score_tasks.scoreTaskDone(tid, 1, robj);
    }
    else if (purpose == 5)//放弃任务
    {
        return cdata->m_score_tasks.cancelScoreTask(tid);
    }
    else if (purpose == 6)//领取奖励
    {
        return cdata->m_score_tasks.getReward(robj);
    }
    return HC_ERROR;
}

baseScoreTask::baseScoreTask()
{
    id = 0;
    type = 0;
    need = 0;
    name = "a task";
    memo = "a task memo";
    quality = 0;
    done_cost = 0;
    score = 0;
}

void baseScoreTask::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", id));
    obj.push_back( Pair("type", type));
    obj.push_back( Pair("spic", type));
    obj.push_back( Pair("need", need));
    obj.push_back( Pair("name", name));
    obj.push_back( Pair("memo", memo));
    obj.push_back( Pair("quality", quality));
    obj.push_back( Pair("score", score));
    obj.push_back( Pair("silver", silver));
    obj.push_back( Pair("done_cost", done_cost));
    return;
}

void CharScoreTask::Save()
{
    InsertSaveDb("update char_daily_score_tasks set cur='" + LEX_CAST_STR(m_cur)
        + "',state='" + LEX_CAST_STR(m_state) + "' where cid=" + LEX_CAST_STR(m_charData.m_id)
        + " and tid=" + LEX_CAST_STR(m_tid)
        + " and type='1'"
        );
}

void CharScoreTask::toObj(json_spirit::Object& obj)
{
    //基础信息，描述名字
    if (m_task.get())
    {
        json_spirit::Object taskInfo;
        m_task->toObj(taskInfo);
        obj.push_back( Pair("taskInfo", taskInfo) );
    }
    obj.push_back( Pair("cur", m_cur));
    obj.push_back( Pair("done", m_done) );
    return;
}

void CharScoreSpecial::Save()
{
    InsertSaveDb("update char_daily_score_tasks set cur='" + LEX_CAST_STR(m_done ? 1 : 0)
        + "' where cid=" + LEX_CAST_STR(m_charData.m_id)
        + " and tid=" + LEX_CAST_STR(m_tid)
        + " and type='2'"
        );
}

void CharScoreSpecial::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("type", m_type));
    obj.push_back( Pair("spic", m_type));
    obj.push_back( Pair("done", m_done) );
    obj.push_back( Pair("name", m_name));
    obj.push_back( Pair("memo", m_memo) );
    return;
}

//获取玩家某积分任务
boost::shared_ptr<CharScoreTask> CharDailyScore::getCharScoreTask(int id)
{
    std::map<int, boost::shared_ptr<CharScoreTask> >::iterator it = m_score_tasks.find(id);
    if (it != m_score_tasks.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<CharScoreTask> p;
    p.reset();
    return p;
}

//接受积分任务
int CharDailyScore::acceptScoreTask(int tid)
{
    if (m_task_cnt >= iDailyScoreTaskMax)
        return HC_ERROR_DAILY_SCORE_MAX;
    boost::shared_ptr<CharScoreTask> ct = getCharScoreTask(tid);
    if (ct.get() && !m_cur_task.get())
    {
        m_cur_task = ct;
        ct->m_state = 1;
        ct->Save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//放弃积分任务
int CharDailyScore::cancelScoreTask(int tid)
{
    boost::shared_ptr<CharScoreTask> ct = getCharScoreTask(tid);
    if (ct.get() && m_cur_task.get() && m_cur_task->m_tid == tid)
    {
        m_cur_task.reset();
        ct->m_cur = 0;
        ct->m_state = 0;
        ct->m_done = false;
        ct->Save();
        m_charData.updateTopButton(top_button_daily, getActive());
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//刷新积分任务
void CharDailyScore::refreshScoreTask(bool add)
{
    m_score_tasks.clear();
    m_cur_task.reset();
    InsertSaveDb("delete from char_daily_score_tasks where cid=" + LEX_CAST_STR(m_charData.m_id)
        + " and type='1'");
    std::vector<int> list;
    bool get_double = false;
    for (int i = 0; i < iDailyScoreTaskCnt; ++i)
    {
        int tmp = 0;
        //第一次刷新特殊处理
        int first_state = m_charData.queryExtraData(char_data_type_normal,char_data_normal_first_refresh_daily_score);
        if (add && first_state == 0)
        {
            std::list<boost::shared_ptr<CharScoreSpecial> >::iterator special_it = m_specials.begin();
            if (special_it != m_specials.end())
            {
                tmp = Singleton<dailyScoreMgr>::Instance().RandomFirstTask((*special_it)->m_type);
                m_charData.setExtraData(char_data_type_normal,char_data_normal_first_refresh_daily_score,1);
            }
        }
        else if (!m_double && !get_double)
        {
            bool must = false;
            if (m_task_cnt == iDailyScoreTaskMax-1)
                must = true;
            tmp = Singleton<dailyScoreMgr>::Instance().RandomGoldTask(must);
            if (tmp > 0)
            {
                get_double = true;
            }
        }
        if (tmp == 0)
        {
            tmp = Singleton<dailyScoreMgr>::Instance().RandomTask();
            while (find(list.begin(),list.end(),tmp) != list.end())
            {
                tmp = Singleton<dailyScoreMgr>::Instance().RandomTask();
            }
        }
        list.push_back(tmp);
    }
    std::vector<int>::iterator it = list.begin();
    while (it != list.end())
    {
        boost::shared_ptr<baseScoreTask> bt = Singleton<dailyScoreMgr>::Instance().getTask(*it);
        if (bt.get())
        {
            boost::shared_ptr<CharScoreTask> ct(new CharScoreTask(m_charData));
            ct->m_tid = *it;
            ct->m_cur = 0;
            ct->m_state = 0;
            ct->m_task = bt;
            ct->m_done = false;
            m_score_tasks[*it] = ct;
            InsertSaveDb("insert into char_daily_score_tasks set type='1',cur='0',state='0',cid=" + LEX_CAST_STR(m_charData.m_id)
            + ",tid=" + LEX_CAST_STR(ct->m_tid));
        }
        ++it;
    }
    if (add)
    {
        int times = m_charData.queryExtraData(char_data_type_daily,char_data_daily_daily_score_refresh);
        m_charData.setExtraData(char_data_type_daily,char_data_daily_daily_score_refresh, ++times);
    }
    m_charData.updateTopButton(top_button_daily, getActive());
    return;
}

//任务完成
int CharDailyScore::scoreTaskDone(int id, int cost, json_spirit::Object& robj)
{
    if (!m_cur_task.get() || !m_cur_task->m_task.get())
    {
        return HC_ERROR;
    }
    if (!m_cur_task->m_done && cost == 0)
    {
        return HC_ERROR;
    }
    if (cost)
    {
        if (m_charData.subGold(m_cur_task->m_task->done_cost, gold_cost_daily_score_done) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
    }
    //给积分
    m_score += m_cur_task->m_task->score;
    m_charData.addSilver(m_cur_task->m_task->silver, silver_get_daily_score);
    ++m_task_cnt;
    if (m_cur_task->m_task->type == DAILY_SCORE_GOLD)
        m_double = true;
    //积分任务完成可能影响目标任务
    bool all_done = false;
    if (m_specials.size() > 0)
    {
        std::list<boost::shared_ptr<CharScoreSpecial> >::iterator it = m_specials.begin();
        while (it != m_specials.end())
        {
            if ((*it).get())
            {
                CharScoreSpecial& ct = *((*it).get());
                if (!ct.m_done && ct.m_type == m_cur_task->m_task->type)
                {
                    ct.m_done = true;
                    ct.Save();
                    break;
                }
            }
            ++it;
        }
        all_done = true;
        it = m_specials.begin();
        while (it != m_specials.end())
        {
            if ((*it).get() && (*it).get()->m_done == false)
            {
                all_done = false;
                break;
            }
            ++it;
        }
    }
    refreshScoreTask();
    if (all_done)
    {
        m_score += iDailySpecilAdd[m_specials.size()-1];
        refreshSpecial();
    }
    SaveData();
    m_charData.m_tasks.updateTask(GOAL_DAILY_SCORE_TASK, 0, m_score);
    m_charData.updateTopButton(top_button_daily, getActive());
    return HC_SUCCESS;
}


//获取玩家某积分任务
boost::shared_ptr<CharScoreSpecial> CharDailyScore::getCharScoreSpecial(int id)
{
    std::list<boost::shared_ptr<CharScoreSpecial> >::iterator it = m_specials.begin();
    while (it != m_specials.end())
    {
        if ((*it).get() && (*it)->m_tid == id)
        {
            return (*it);
        }
        ++it;
    }
    boost::shared_ptr<CharScoreSpecial> p;
    p.reset();
    return p;
}

//刷新目标任务
void CharDailyScore::refreshSpecial()
{
    m_specials.clear();
    InsertSaveDb("delete from char_daily_score_tasks where cid=" + LEX_CAST_STR(m_charData.m_id)
        + " and type='2'");
    if (m_special_reset >= iDailySpecilCnt)
        return;
    ++m_special_reset;
    std::vector<int> list;
    for (int i = 0; i < m_special_reset; ++i)
    {
        int tmp = Singleton<dailyScoreMgr>::Instance().RandomTask();
        while (find(list.begin(),list.end(),tmp) != list.end())
        {
            tmp = Singleton<dailyScoreMgr>::Instance().RandomTask();
        }
        list.push_back(tmp);
    }
    std::vector<int>::iterator it = list.begin();
    while (it != list.end())
    {
        boost::shared_ptr<baseScoreTask> bt = Singleton<dailyScoreMgr>::Instance().getTask(*it);
        if (bt.get())
        {
            boost::shared_ptr<CharScoreSpecial> ct(new CharScoreSpecial(m_charData));
            ct->m_tid = *it;
            ct->m_type = bt->type;
            ct->m_done = false;
            ct->m_name = bt->name;
            ct->m_memo = bt->memo;
            m_specials.push_back(ct);
            InsertSaveDb("insert into char_daily_score_tasks set type='2',cur='0',cid=" + LEX_CAST_STR(m_charData.m_id)
            + ",tid=" + LEX_CAST_STR(ct->m_tid));
        }
        ++it;
    }
}

//任务列表
int CharDailyScore::getScoreTaskList(json_spirit::Object& robj)
{
    json_spirit::Array special_list;
    std::list<boost::shared_ptr<CharScoreSpecial> >::iterator it = m_specials.begin();
    while (it != m_specials.end())
    {
        if ((*it).get())
        {
            CharScoreSpecial& ct = *((*it).get());
            json_spirit::Object obj;
            ct.toObj(obj);
            special_list.push_back(obj);
        }
        ++it;
    }
    robj.push_back( Pair("special_list", special_list) );
    if (m_specials.size() > 0)
        robj.push_back( Pair("special_score", iDailySpecilAdd[m_specials.size()-1]) );
    json_spirit::Array score_list;
    std::map<int, boost::shared_ptr<CharScoreTask> >::iterator it_s = m_score_tasks.begin();
    while (it_s != m_score_tasks.end())
    {
        if (it_s->second.get() && it_s->second->m_task.get())
        {
            CharScoreTask& ct = *(it_s->second.get());
            json_spirit::Object obj;
            ct.toObj(obj);
            if (m_cur_task.get() && ct.m_tid == m_cur_task->m_tid)
            {
                obj.push_back( Pair("cur_task", 1) );
            }
            score_list.push_back(obj);
        }
        ++it_s;
    }
    robj.push_back( Pair("score_list", score_list) );
    robj.push_back( Pair("task_cnt", m_task_cnt) );
    robj.push_back( Pair("task_max", iDailyScoreTaskMax) );
    //刷新次数
    int free_times = iDailyScoreTaskRefreshFree - m_charData.queryExtraData(char_data_type_daily,char_data_daily_daily_score_refresh);
    robj.push_back( Pair("free_refresh_times", free_times > 0 ? free_times : 0) );
    robj.push_back( Pair("refresh_cost", iDailyScoreTaskRefreshCost) );
    return HC_SUCCESS;
}

//奖励列表
int CharDailyScore::getRewardList(json_spirit::Object& robj)
{
    Singleton<dailyScoreMgr>::Instance().getRewardInfo(robj);
    robj.push_back( Pair("can_get", Singleton<dailyScoreMgr>::Instance().canGetReward(m_reward_num+1, m_score)) );
    robj.push_back( Pair("score", m_score) );
    robj.push_back( Pair("reward_num", m_reward_num) );
    robj.push_back( Pair("double_reward", m_double) );
    return HC_SUCCESS;
}

//更新任务
void CharDailyScore::updateTask(int type, int n)
{
    if (m_cur_task.get() && m_cur_task->m_task.get() && m_cur_task->m_task->type == type)
    {
        CharScoreTask& ct = *(m_cur_task.get());
        ct.m_cur += n;
        if (ct.m_cur >= ct.m_task->need)
        {
            ct.m_cur = ct.m_task->need;
            ct.m_done = true;
            m_charData.updateTopButton(top_button_daily, 1);
        }
        else
        {
            ct.m_done = false;
        }
        ct.Save();
    }
    return;
}

int CharDailyScore::getReward(json_spirit::Object& robj)
{
    int ret = Singleton<dailyScoreMgr>::Instance().getReward(m_charData, m_reward_num+1, m_score, robj);
    if (ret == HC_SUCCESS)
    {
        ++m_reward_num;
        m_charData.updateTopButton(top_button_daily, getActive());
        SaveData();
    }
    return ret;
}

void CharDailyScore::load()
{
    Query q(GetDb());
    //积分任务
    q.get_result("select tid,cur,state from char_daily_score_tasks where cid=" + LEX_CAST_STR(m_charData.m_id) + " and type = '1' order by tid");
    while (q.fetch_row())
    {
        int tid = q.getval();
        boost::shared_ptr<baseScoreTask> bt = Singleton<dailyScoreMgr>::Instance().getTask(tid);
        if (!bt.get())
        {
            continue;
        }
        boost::shared_ptr<CharScoreTask> ct(new CharScoreTask(m_charData));
        ct->m_tid = tid;
        ct->m_cur = q.getval();
        ct->m_state = q.getval();
        ct->m_task = bt;
        ct->m_done = ct->m_cur >= bt->need;
        m_score_tasks[bt->id] = ct;
        if(ct->m_state == 1)
        {
            m_cur_task = ct;
        }
    }
    q.free_result();
    //目标任务
    q.get_result("select tid,cur from char_daily_score_tasks where cid=" + LEX_CAST_STR(m_charData.m_id) + " and type = '2' order by tid");
    while (q.fetch_row())
    {
        int tid = q.getval();
        boost::shared_ptr<baseScoreTask> bt = Singleton<dailyScoreMgr>::Instance().getTask(tid);
        if (!bt.get())
        {
            continue;
        }
        boost::shared_ptr<CharScoreSpecial> ct(new CharScoreSpecial(m_charData));
        ct->m_tid = tid;
        ct->m_type = bt->type;
        ct->m_done = (q.getval() == 1);
        ct->m_name = bt->name;
        ct->m_memo = bt->memo;
        m_specials.push_back(ct);
    }
    q.free_result();
    //基础数据
    q.get_result("select score,task_cnt,special_reset,double_state,rewards from char_daily_score_data where cid=" + LEX_CAST_STR(m_charData.m_id));
    if (q.fetch_row())
    {
        m_score = q.getval();
        m_task_cnt = q.getval();
        m_special_reset = q.getval();
        m_double = (q.getval() == 1);
        m_reward_num = q.getval();
    }
    else
    {
        q.free_result();
        SaveData();
    }
    q.free_result();
    if (m_specials.size() == 0)
    {
        refreshSpecial();
        SaveData();
    }
    if (m_score_tasks.size() == 0)
    {
        refreshScoreTask();
    }
}

void CharDailyScore::SaveData()
{
    InsertSaveDb("replace into char_daily_score_data (cid,score,task_cnt,special_reset,double_state,rewards) values ("
        + LEX_CAST_STR(m_charData.m_id) + ","
        + LEX_CAST_STR(m_score) + ","
        + LEX_CAST_STR(m_task_cnt) + ","
        + LEX_CAST_STR(m_special_reset) + ","
        + LEX_CAST_STR(m_double ? 1 : 0) + ","
        + LEX_CAST_STR(m_reward_num) + ")");
}

void CharDailyScore::dailyUpdate()
{
    if (m_task_cnt >= 1)
    {
        m_task_cnt = 0;
        m_score = 0;
        m_reward_num = 0;
        m_special_reset = 0;
        m_double = false;
        refreshScoreTask();
        refreshSpecial();
        SaveData();
    }
}

int CharDailyScore::getActive()
{
    //当前任务完成
    if (m_cur_task.get() && m_cur_task->m_done)
    {
        return 1;
    }
    //可以领取奖励
    if (Singleton<dailyScoreMgr>::Instance().canGetReward(m_reward_num+1, m_score))
    {
        return 1;
    }
    return 0;
}

dailyScoreMgr::dailyScoreMgr()
{
    Query q(GetDb());
    q.get_result("select id,type,need,quality,score,silver,name,memo,done_cost,per from base_daily_score_tasks where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        baseScoreTask* pt = new baseScoreTask();
        pt->id = id;
        pt->type = q.getval();
        pt->need = q.getval();
        pt->quality = q.getval();
        pt->score = q.getval();
        pt->silver = q.getval();
        pt->name = q.getstr();
        pt->memo = q.getstr();
        pt->done_cost = q.getval();
        int per = q.getval();
        boost::shared_ptr<baseScoreTask> bt(pt);
        m_total_tasks[id] = bt;
        if (pt->type == DAILY_SCORE_GOLD)
        {
            m_gold_tid = id;
        }
        else
        {
            m_gailvs.push_back(per);
            m_all_task_id.push_back(id);
        }
    }
    q.free_result();
    //加载奖励
    q.get_result("select id,needscore,itemType,itemId,counts,extra FROM base_daily_score_reward WHERE 1 order by id asc");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        int id = q.getval();
        dailyScoreRewards tmp;
        tmp.needscore = q.getval();
        tmp.rewards.type = q.getval();
        tmp.rewards.id = q.getval();
        tmp.rewards.nums = q.getval();
        tmp.rewards.extra = q.getval();
        m_daily_task_rewards.push_back(tmp);
        iMaxDailyTaskReward = id;
    }
    q.free_result();
}

void dailyScoreMgr::getButton(CharData* pc, json_spirit::Array& list)
{
    if (pc->isDailyScoreOpen())
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_daily) );
        obj.push_back( Pair("active", pc->m_score_tasks.getActive()) );
        list.push_back(obj);
    }
}

boost::shared_ptr<baseScoreTask> dailyScoreMgr::getTask(int tid)
{
    if (m_total_tasks.find(tid) != m_total_tasks.end())
    {
        //cout << "TRUE,taskMgr::getTask id=" << tid << endl;
        return m_total_tasks[tid];
    }
    else
    {
        //cout << "FALSE,taskMgr::getTask id=" << tid << endl;
        boost::shared_ptr<baseScoreTask> tt;
        return tt;
    }
}

bool dailyScoreMgr::canGetReward(int cnt, int score)
{
    bool can_get = false;
    for (int i = cnt; i <= iMaxDailyTaskReward; ++i)
    {
        if (score >= m_daily_task_rewards[i-1].needscore)
        {
            can_get = true;
            break;
        }
    }
    return can_get;
}

void dailyScoreMgr::getRewardInfo(json_spirit::Object& robj)
{
    json_spirit::Array list;
    for (int i = 1; i <= iMaxDailyTaskReward; ++i)
    {
        json_spirit::Object info;
        json_spirit::Object reward;
        m_daily_task_rewards[i-1].rewards.toObj(reward);
        info.push_back( Pair("reward", reward) );
        info.push_back( Pair("score", m_daily_task_rewards[i-1].needscore) );
        list.push_back(info);
    }
    robj.push_back( Pair("reward_list", list) );
    json_spirit::Object taskInfo;
    boost::shared_ptr<baseScoreTask> t = getTask(m_gold_tid);
    if (t.get())
        t->toObj(taskInfo);
    robj.push_back( Pair("taskInfo", taskInfo) );
}

int dailyScoreMgr::getReward(CharData& cdata, int reward_id, int score, json_spirit::Object& robj)
{
    if (reward_id < 1 || reward_id > iMaxDailyTaskReward)
    {
        return HC_ERROR;
    }
    if (score < m_daily_task_rewards[reward_id-1].needscore)
    {
        return HC_ERROR;
    }
    std::list<Item> items;
    items.push_back(m_daily_task_rewards[reward_id-1].rewards);
    if (cdata.m_score_tasks.m_double)
    {
        std::list<Item>::iterator it = items.begin();
        while (it != items.end())
        {
            it->nums *= 2;
            ++it;
        }
    }
    if (!cdata.m_bag.hasSlot(itemlistNeedBagSlot(items)))
    {
        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
    }
    giveLoots(&cdata,items,NULL,&robj,true, loot_daily_score);
    if (reward_id == iMaxDailyTaskReward)
    {
        //系统公告
        std::string notify_msg = strDailyScoreGet;
        str_replace(notify_msg, "$N", MakeCharNameLink(cdata.m_name, cdata.m_nick.get_string()));
        std::string get = m_daily_task_rewards[reward_id-1].rewards.toString(true);
        get = MakeGemLink(get,m_daily_task_rewards[reward_id-1].rewards.id,m_daily_task_rewards[reward_id-1].rewards.nums);
        str_replace(notify_msg, "$R", get);
        if (notify_msg != "")
        {
            GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
        }
        Singleton<relationMgr>::Instance().postCongradulation(cdata.m_id, CONGRATULATION_DAILY_SCORE, 0, 0);
    }
    return HC_SUCCESS;
}

int dailyScoreMgr::RandomGoldTask(bool must)
{
    if (must)
        return m_gold_tid;
    if (my_random(0, iDailyScoreTaskMax) == 0)
        return m_gold_tid;
    return 0;
}

int dailyScoreMgr::RandomTask()
{
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    boost::random::discrete_distribution<> dist(m_gailvs);
    return m_all_task_id[dist(gen)];
}

//新手第一次刷新出紫色目标任务
int dailyScoreMgr::RandomFirstTask(int type)
{
    std::map<int, boost::shared_ptr<baseScoreTask> >::iterator it =  m_total_tasks.begin();
    while (it != m_total_tasks.end())
    {
        if (it->second.get() && it->second->type == type && it->second->quality == 4)
        {
            return it->first;
        }
        ++it;
    }
    return 0;
}

void dailyScoreMgr::dailyUpdate()
{
    Query q(GetDb());
    std::vector<int> list;
    q.get_result("select cid from char_daily_score_data where task_cnt >= 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        list.push_back(q.getval());
    }
    q.free_result();
    std::vector<int>::iterator it = list.begin();
    while (it != list.end())
    {
        InsertSaveDb("delete from char_daily_score_tasks where cid=" + LEX_CAST_STR(*it));
        InsertSaveDb("replace into char_daily_score_data (cid,score,task_cnt,special_reset,double_state,rewards) values ("
            + LEX_CAST_STR(*it) + ",'0','0','0','0','0')");
        ++it;
    }
}

