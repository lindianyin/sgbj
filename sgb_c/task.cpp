
#include "task.h"
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

void baseGoal::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("type", type));
    obj.push_back( Pair("need", need[1]));
    if (target_stronghold.get())
    {
        json_spirit::Object strongholdInfo;
        strongholdInfo.push_back( Pair("mapid", target_stronghold->m_mapid) );
        strongholdInfo.push_back( Pair("stageid", target_stronghold->m_stageid) );
        strongholdInfo.push_back( Pair("strongholdid", target_stronghold->m_id) );
        strongholdInfo.push_back( Pair("type", target_stronghold->m_type) );
        obj.push_back( Pair("strongholdInfo", strongholdInfo));
    }
    if (type == GOAL_COPY_ATTACK)
    {
        boost::shared_ptr<baseCopy> bc = Singleton<copyMgr>::Instance().getCopyById(need[0]);
        if (bc.get())
        {
            json_spirit::Object copyInfo;
            copyInfo.push_back( Pair("mapid", bc->m_mapid) );
            copyInfo.push_back( Pair("copyid", bc->m_id) );
            obj.push_back( Pair("copyInfo", copyInfo));
        }
    }
    if (type == GOAL_UPGRADE_SKILL)
    {
        boost::shared_ptr<baseSkill> bc = Singleton<skillMgr>::Instance().getBaseSkill(need[0]);
        if (bc.get())
        {
            json_spirit::Object skillInfo;
            skillInfo.push_back( Pair("type", bc->m_type) );
            obj.push_back( Pair("skillInfo", skillInfo));
        }
    }
    return;
}

baseTask::baseTask()
{
    id = 0;
    type = 0;
    task_id = 0;
    name = "a task";
    memo = "a task memo";
    pre_task_type = 0;
    pre_task_id = 0;
    done_cost = 0;
    m_child_tasks.clear();
}

void baseTask::loadRewards()
{
    reward.clear();
    Query q(GetDb());
    q.get_result("select itemType,itemId,counts,extra from base_tasks_reward where taskid="+LEX_CAST_STR(id)+" order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();
        item.extra = q.getval();
        reward.push_back(item);
    }
    q.free_result();
}

void baseTask::loadGoals()
{
    goals.clear();
    Query q(GetDb());
    q.get_result("select type,need1,need2,memo from base_tasks_goal where taskid="+LEX_CAST_STR(id)+" order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<baseGoal> goal(new baseGoal());
        goal->type = q.getval();
        goal->need[0] = q.getval();
        goal->need[1] = q.getval();
        goal->memo = q.getstr();
        goal->target_stronghold.reset();
        if (goal->type == GOAL_STRONGHOLD || goal->type == GOAL_STRONGHOLD_ATTACK)
        {
            if (goal->need[0] != 0)
            {
                goal->target_stronghold = Singleton<mapMgr>::Instance().GetBaseStrongholdData(goal->need[0]);
            }
        }
        goals.push_back(goal);
    }
    q.free_result();
}

void baseTask::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", id));
    obj.push_back( Pair("type", type));
    obj.push_back( Pair("task_id", task_id));
    obj.push_back( Pair("name", name));
    obj.push_back( Pair("memo", memo));
    obj.push_back( Pair("pre_task_type", pre_task_type));
    obj.push_back( Pair("pre_task_id", pre_task_id));
    obj.push_back( Pair("done_cost", done_cost));
    #if 0
    json_spirit::Array goal_list;
    for (std::list<boost::shared_ptr<baseGoal> >::iterator it_g = goals.begin(); it != goals.end(); ++it)
    {
        json_spirit::Object obj;
        if ((*it_g).get())
        {
            (*it_g)->toObj(obj);
        }
        goal_list.push_back(obj);
    }
    obj.push_back( Pair("goal_list", goal_list));
    #endif
    json_spirit::Array reward_list;
    for (std::list<Item>::iterator it = reward.begin(); it != reward.end(); ++it)
    {
        Item& item = *it;
        json_spirit::Object obj;
        item.toObj(obj);
        reward_list.push_back(obj);
    }
    obj.push_back( Pair("reward_list", reward_list));
    return;
}

void baseTask::toSimpleObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", id));
    obj.push_back( Pair("type", type));
    obj.push_back( Pair("task_id", task_id));
    obj.push_back( Pair("name", name));
    obj.push_back( Pair("done_cost", done_cost));
    return;
}

void baseTask::ChildList(json_spirit::Object& obj)
{
    if (m_child_tasks.size())
    {
        json_spirit::Array child_list;
        std::list<boost::shared_ptr<baseTask> >::iterator itc = m_child_tasks.begin();
        while (itc != m_child_tasks.end())
        {
            json_spirit::Object simpleobj;
            (*itc)->toSimpleObj(simpleobj);
            (*itc)->ChildList(simpleobj);
            child_list.push_back(simpleobj);
            ++itc;
        }
        obj.push_back( Pair("list", child_list));
    }
    return;
}

void baseTask::reload()
{
    Query q(GetDb());
    q.get_result("select type,task_id,name,memo,pre_task_type,pre_task_id,done_cost,need_race from base_tasks where id="+LEX_CAST_STR(id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        type = q.getval();
        task_id = q.getval();
        name = q.getstr();
        memo = q.getstr();
        pre_task_type = q.getval();
        pre_task_id = q.getval();
        done_cost = q.getval();
        need_race = q.getval();
        //任务奖励
        loadRewards();
        //任务目标
        loadGoals();
    }
    q.free_result();
    return;
}

void CharTask::Save()
{
    std::vector<int> cur_list;
    std::vector<int> extra_list;
    for(std::list<boost::shared_ptr<CharTaskGoal> >::iterator it = m_goals.begin(); it != m_goals.end(); ++it)
    {
        if ((*it).get())
        {
            cur_list.push_back((*it)->cur);
            extra_list.push_back((*it)->extra);
        }
    }
    const json_spirit::Value val_curs(cur_list.begin(), cur_list.end());
    const json_spirit::Value val_extra(extra_list.begin(), extra_list.end());
    InsertSaveDb("update char_tasks set cur='" + json_spirit::write(val_curs)
        + "',extra='" + json_spirit::write(val_extra)
        + "' where cid=" + LEX_CAST_STR(m_charData.m_id)
        + " and tid=" + LEX_CAST_STR(tid)
        );
}

void CharTask::toObj( bool simple, json_spirit::Object& obj)
{
    //基础信息，描述名字
    if (m_task.get())
    {
        json_spirit::Object taskInfo;
        if (simple)
        {
            m_task->toSimpleObj(taskInfo);
        }
        else
        {
            m_task->toObj(taskInfo);
        }
        obj.push_back( Pair("taskInfo", taskInfo) );
    }
    json_spirit::Array goal_list;
    for(std::list<boost::shared_ptr<CharTaskGoal> >::iterator it = m_goals.begin(); it != m_goals.end(); ++it)
    {
        json_spirit::Object o;
        if ((*it).get() && (*it)->base_goal.get())
        {
            (*it)->base_goal->toObj(o);
            //日常任务有可能需要拼凑描述
            std::string memo = (*it)->base_goal->memo;
            if (m_task->type == TASK_TYPE_DAILY)
            {
                if ((*it)->base_goal->type == GOAL_DAILY_STRONGHOLD)
                {
                    boost::shared_ptr<baseStronghold> bs = Singleton<mapMgr>::Instance().GetBaseStrongholdData((*it)->extra);
                    if (bs.get())
                    {
                        json_spirit::Object strongholdInfo;
                        strongholdInfo.push_back( Pair("mapid", bs->m_mapid) );
                        strongholdInfo.push_back( Pair("stageid", bs->m_stageid) );
                        strongholdInfo.push_back( Pair("strongholdid", bs->m_id) );
                        strongholdInfo.push_back( Pair("type", bs->m_type) );
                        o.push_back( Pair("strongholdInfo", strongholdInfo));
                        str_replace(memo, "$N", bs->m_name);
                    }
                }
                else if ((*it)->base_goal->type == GOAL_DAILY_COPY)
                {
                    boost::shared_ptr<baseCopy> bc = Singleton<copyMgr>::Instance().getCopyById((*it)->extra);
                    if (bc.get())
                    {
                        json_spirit::Object copyInfo;
                        copyInfo.push_back( Pair("mapid", bc->m_mapid) );
                        copyInfo.push_back( Pair("copyid", bc->m_id) );
                        o.push_back( Pair("copyInfo", copyInfo));
                        str_replace(memo, "$N", bc->m_name);
                    }
                }
            }
            o.push_back( Pair("memo", memo));
            o.push_back( Pair("cur", (*it)->cur) );
            goal_list.push_back(o);
        }
    }
    obj.push_back( Pair("goal_list", goal_list));
    obj.push_back( Pair("isDone", done ? 2 : 1) );
    return;
}

//接受任务
void CharAllTasks::acceptTask(boost::shared_ptr<baseTask> t)
{
    if (t.get())
    {
        boost::shared_ptr<CharTask> ct(new CharTask(m_charData));
        ct->tid = t->id;
        ct->m_task = t;
        ct->done = true;
        //各目标
        std::vector<int> cur_list;
        std::vector<int> extra_list;
        std::vector<int> s_list, c_list;
        for(std::list<boost::shared_ptr<baseGoal> >::iterator it = t->goals.begin(); it != t->goals.end(); ++it)
        {
            if ((*it).get())
            {
                baseGoal* bg = (*it).get();
                boost::shared_ptr<CharTaskGoal> ctg(new CharTaskGoal());
                ctg->base_goal = (*it);
                ctg->type = bg->type;
                ctg->cur = 0;
                ctg->done = false;
                ctg->extra = 0;
                //领取日常任务有可能需要随机
                if (t->type == TASK_TYPE_DAILY)
                {
                    if (bg->type == GOAL_DAILY_STRONGHOLD)
                    {
                        int min_id = m_charData.m_cur_strongholdid - 2;
                        int max_id = m_charData.m_cur_strongholdid + 2;
                        if (min_id <= 0)
                            min_id = 1;
                        boost::shared_ptr<baseStronghold> bs = Singleton<mapMgr>::Instance().GetBaseStrongholdData(max_id);
                        if (!bs.get())
                        {
                            max_id = m_charData.m_cur_strongholdid;
                        }
                        int tmp = my_random(min_id,max_id);
                        bs = Singleton<mapMgr>::Instance().GetBaseStrongholdData(tmp);
                        while (find(s_list.begin(),s_list.end(),tmp) != s_list.end() || !bs.get() || bs->m_type == STRONGHOLD_TYPE_BOX || bs->m_type == STRONGHOLD_TYPE_CAPTURE)
                        {
                            tmp = my_random(min_id,max_id);
                            bs = Singleton<mapMgr>::Instance().GetBaseStrongholdData(tmp);
                        }
                        ctg->extra = tmp;
                        s_list.push_back(tmp);
                    }
                    else if (bg->type == GOAL_DAILY_COPY)
                    {
                        int min_id = m_charData.queryExtraData(char_data_type_normal, char_data_normal_copy) - 2;
                        int max_id = m_charData.queryExtraData(char_data_type_normal, char_data_normal_copy) + 2;
                        if (min_id <= 0)
                            min_id = 1;
                        boost::shared_ptr<baseCopy> bc = Singleton<copyMgr>::Instance().getCopyById(max_id);
                        if (!bc.get())
                        {
                            max_id = m_charData.queryExtraData(char_data_type_normal, char_data_normal_copy);
                        }
                        int tmp = my_random(min_id,max_id);
                        while (find(c_list.begin(),c_list.end(),tmp) != c_list.end())
                        {
                            tmp = my_random(min_id,max_id);
                        }
                        ctg->extra = tmp;
                        c_list.push_back(tmp);
                    }
                }
                switch (bg->type)
                {
                    case GOAL_STRONGHOLD:
                    {
                        if (m_charData.m_cur_strongholdid >= bg->need[0])
                        {
                            ctg->cur = 1;
                        }
                        break;
                    }
                    case GOAL_CHAR_LEVEL:
                    {
                        ctg->cur = m_charData.m_level;
                        break;
                    }
                    case GOAL_SHENLING_TOWER:
                    {
                        boost::shared_ptr<CharShenling> cc = Singleton<shenlingMgr>::Instance().getCharShenling(m_charData.m_id);
                        if (cc)
                        {
                            ctg->cur = cc->m_sid - 1;
                        }
                        break;
                    }
                    case GOAL_CASTLE_LEVEL:
                    {
                        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(m_charData.m_id);
                        if (cc)
                        {
                            ctg->cur = cc->m_level;
                        }
                        break;
                    }
                    case GOAL_METALLURGY_LEVEL:
                    {
                        boost::shared_ptr<char_metallurgy> cc = Singleton<cityMgr>::Instance().getCharMetallurgy(m_charData.m_id);
                        if (cc)
                        {
                            ctg->cur = cc->m_level;
                        }
                        break;
                    }
                    case GOAL_SMITHY_LEVEL:
                    {
                        boost::shared_ptr<char_smithy> cc = Singleton<cityMgr>::Instance().getCharSmithy(m_charData.m_id);
                        if (cc)
                        {
                            ctg->cur = cc->m_level;
                        }
                        break;
                    }
                    case GOAL_BARRACKS_LEVEL:
                    {
                        boost::shared_ptr<char_barracks> cc = Singleton<cityMgr>::Instance().getCharBarracks(m_charData.m_id);
                        if (cc)
                        {
                            ctg->cur = cc->m_level;
                        }
                        break;
                    }
                    case GOAL_MAP:
                    {
                        if (m_charData.m_cur_mapid >= bg->need[0])
                        {
                            ctg->cur = 1;
                        }
                        break;
                    }
                    case GOAL_LEVY:
                    {
                        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(m_charData.m_id);
                        if (cc)
                        {
                            ctg->cur = cc->m_levy;
                        }
                        break;
                    }
                    case GOAL_RECRUIT:
                    {
                        time_t t_now = time(NULL);
                        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(m_charData.m_id);
                        if (cc)
                        {
                            ctg->cur = (t_now < cc->m_recruit_cd) ? 1 : 0;
                        }
                        break;
                    }
                    case GOAL_UPGRADE_SKILL:
                    {
                        boost::shared_ptr<CharSkill> p = m_charData.m_skills.GetSkill(bg->need[0]);
                        if (p.get())
                        {
                            ctg->cur = p->m_level;
                        }
                        break;
                    }
                    case GOAL_SET_MAGIC:
                    {
                        for (int i = 0; i < 3; ++i)
                        {
                            if (m_charData.m_magics.m_combat_magics[i] == bg->need[0])
                            {
                                ctg->cur = 1;
                                break;
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
                if (ctg->cur >= bg->need[1])
                {
                    ctg->cur = bg->need[1];
                    ctg->done = true;
                }
                else
                {
                    ct->done = false;
                }
                extra_list.push_back(ctg->extra);
                cur_list.push_back(ctg->cur);
                //插入目标到任务
                ct->m_goals.push_back(ctg);
            }
        }
        m_all_tasks[t->id] = ct;
        const json_spirit::Value val_curs(cur_list.begin(), cur_list.end());
        const json_spirit::Value val_extra(extra_list.begin(), extra_list.end());
        InsertSaveDb("insert into char_tasks set cur='" + json_spirit::write(val_curs)
        + "',extra='" + json_spirit::write(val_extra)
        + "',cid=" + LEX_CAST_STR(m_charData.m_id)
        + ",tid=" + LEX_CAST_STR(ct->tid)
        );
        if (t->guide_id_get > 0)
        {
            //新手引导
            m_charData.checkGuide(t->guide_id_get);
        }
    }
}

//任务列表
int CharAllTasks::getList(int type, json_spirit::Array& rlist)
{
    std::map<int, boost::shared_ptr<CharTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_task.get())
        {
            CharTask& ct = *(it->second.get());
            if (type != 0 && type != ct.m_task->type)
            {
                ++it;
                continue;
            }
            json_spirit::Object obj;
            ct.toObj(true, obj);
            rlist.push_back(obj);
        }
        ++it;
    }
    return HC_SUCCESS;
}

//更新任务
int CharAllTasks::updateTask(int goal_type, int n1, int n2)
{
    cout << "CharAllTasks::updateTask" << " goal_type=" << goal_type << ",n1=" << n1 << ",n2=" << n2 << endl;
    std::map<int, boost::shared_ptr<CharTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_task.get())
        {
            CharTask& ct = *(it->second.get());
            bool need_save = false;
            for(std::list<boost::shared_ptr<CharTaskGoal> >::iterator it_g = ct.m_goals.begin(); it_g != ct.m_goals.end(); ++it_g)
            {
                if ((*it_g).get() && (*it_g)->base_goal.get() && (*it_g)->type == goal_type)
                {
                    CharTaskGoal* ctg = (*it_g).get();
                    switch (ctg->type)
                    {
                        case GOAL_STRONGHOLD:
                        case GOAL_COMPOUND_HERO:
                        case GOAL_COMPOUND_EQUIPT:
                        case GOAL_UPGRADE_EQUIPT:
                        case GOAL_DECOMPOSE_HERO:
                        case GOAL_SMELT_HERO:
                        case GOAL_GOLDEN_HERO:
                        case GOAL_MAP:
                        case GOAL_HERO_PACK:
                        case GOAL_MALL_GEM:
                        case GOAL_HERO_STAR:
                        case GOAL_EQUIPT_QUALITY:
                        case GOAL_STRONGHOLD_ATTACK:
                        case GOAL_COPY_ATTACK:
                        case GOAL_EXPLORE:
                        case GOAL_HERO:
                        {
                            if (n1 == ctg->base_goal->need[0])
                            {
                                need_save = true;
                                ctg->cur += n2;
                            }
                            break;
                        }
                        case GOAL_CHAR_LEVEL:
                        case GOAL_SHENLING_TOWER:
                        case GOAL_CASTLE_LEVEL:
                        case GOAL_METALLURGY_LEVEL:
                        case GOAL_SMITHY_LEVEL:
                        case GOAL_BARRACKS_LEVEL:
                        case GOAL_OFFLINE_REWARD:
                        case GOAL_COLLECT:
                        case GOAL_LEVY:
                        case GOAL_RECRUIT:
                        case GOAL_TREASURE:
                        {
                            need_save = true;
                            ctg->cur = n2;
                            break;
                        }
                        case GOAL_ARENA:
                        case GOAL_ARENA_WIN:
                        case GOAL_DONE_DAILY_TASK:
                        {
                            need_save = true;
                            ctg->cur += n2;
                            break;
                        }
                        case GOAL_UPGRADE_SKILL:
                        case GOAL_SET_MAGIC:
                        {
                            if (n1 == ctg->base_goal->need[0])
                            {
                                need_save = true;
                                ctg->cur = n2;
                            }
                            break;
                        }
                        //日常任务
                        case GOAL_DAILY_EXPLORE:
                        case GOAL_DAILY_RECRUIT:
                        case GOAL_DAILY_LEVY:
                        case GOAL_DAILY_ARENA:
                        case GOAL_DAILY_ARENA_WIN:
                        case GOAL_DAILY_TREASURE:
                        case GOAL_DAILY_PK:
                        case GOAL_DAILY_HERO_PACK_GOLD:
                        case GOAL_DAILY_HERO_PACK_SILVER:
                        case GOAL_DAILY_LEVY_GOLD:
                        case GOAL_DAILY_SHENLING_ATTACK:
                        case GOAL_DAILY_TREASURE_ROB:
                        case GOAL_DAILY_GUILD_MOSHEN:
                        case GOAL_DAILY_COPY_ATTACK:
                        case GOAL_DAILY_WILD_LEVY:
                        case GOAL_DAILY_UPGRADE_EQUIPT:
                        case GOAL_DAILY_BOSS:
                        {
                            need_save = true;
                            ctg->cur += n2;
                            break;
                        }
                        case GOAL_DAILY_STRONGHOLD:
                        case GOAL_DAILY_COPY:
                        {
                            if (n1 == ctg->extra)
                            {
                                need_save = true;
                                ctg->cur += n2;
                            }
                            break;
                        }
                        case GOAL_DAILY_SCORE_TASK:
                        {
                            need_save = true;
                            ctg->cur = n2;
                            break;
                        }
                        case GOAL_DAILY_COMPOUND_HERO:
                        case GOAL_DAILY_DECOMPOSE_HERO:
                        case GOAL_DAILY_GOLDEN_HERO:
                        {
                            if (n1 == ctg->base_goal->need[0])
                            {
                                need_save = true;
                                ctg->cur += n2;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    if (ctg->cur >= ctg->base_goal->need[1])
                    {
                        ctg->cur = ctg->base_goal->need[1];
                        ctg->done = true;
                    }
                    else
                    {
                        ctg->done = false;
                    }
                }
            }
            //目标状态有变动
            if (need_save)
            {
                //判断任务完成情况
                ct.done = true;
                for(std::list<boost::shared_ptr<CharTaskGoal> >::iterator it_g = ct.m_goals.begin(); it_g != ct.m_goals.end(); ++it_g)
                {
                    if ((*it_g).get() && (*it_g)->base_goal.get())
                    {
                        CharTaskGoal* ctg = (*it_g).get();
                        if (ctg->done == false)
                        {
                            ct.done = false;
                            break;
                        }
                    }
                }
                if (ct.done == true)
                {
                    if (ct.m_task->guide_id_reward > 0)
                    {
                        //新手引导
                        m_charData.checkGuide(ct.m_task->guide_id_reward);
                    }
                    if (ct.m_task->type == TASK_TYPE_DAILY)
                    {
                        //日常任务
                        m_charData.updateTopButton(top_button_daily_task, 1, getCharDailyTaskCnt());
                    }
                }
                ct.Save();
                //通知玩家任务完成或者有变化
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_charData.m_name);
                if (account.get())
                {
                    json_spirit::Array list;
                    json_spirit::Object tmp;
                    tmp.push_back( Pair("tid", ct.tid) );
                    tmp.push_back( Pair("type", TASK_UPDATE) );
                    ct.toObj(true, tmp);
                    list.push_back(tmp);
                    json_spirit::Object robj;
                    robj.push_back( Pair("cmd", "updateTask") );
                    robj.push_back( Pair("list", list) );
                    robj.push_back( Pair("s", 200) );
                    account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
                }
            }
        }
        ++it;
    }
    return HC_SUCCESS;
}

//任务完成
int CharAllTasks::taskDone(int id, int cost, json_spirit::Object& robj)
{
    std::map<int, boost::shared_ptr<CharTask> >::iterator it = m_all_tasks.find(id);
    if (it == m_all_tasks.end() || !it->second.get())
    {
        return HC_ERROR;
    }
    CharTask& t = *(it->second.get());
    if (!t.m_task.get())
    {
        return HC_ERROR;
    }
    if (!t.done && cost == 0)
    {
        return HC_ERROR;
    }
    std::list<Item> items = t.m_task->reward;
    if (!m_charData.m_bag.hasSlot(itemlistNeedBagSlot(items)))
    {
        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
    }
    if (cost)
    {
        if (m_charData.subGold(t.m_task->done_cost, gold_cost_task_done) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
    }
    //给奖励
    int loot_type = loot_task;
    if (t.m_task->type == TASK_TYPE_DAILY)
        loot_type = loot_daily_task;
    giveLoots(&m_charData,items,NULL,&robj,true,loot_type);
    //通知玩家任务完成或者有变化
    json_spirit::Array list;
    json_spirit::Object tmp;
    tmp.push_back( Pair("tid", t.tid) );
    tmp.push_back( Pair("type", TASK_DELETE) );
    list.push_back(tmp);
    //开启后续任务
    if (t.m_task->m_child_tasks.size() > 0)
    {
        std::list<boost::shared_ptr<baseTask> > ::iterator it_i = t.m_task->m_child_tasks.begin();
        while (it_i != t.m_task->m_child_tasks.end())
        {
            if (it_i->get() && ((*it_i)->need_race == 0 || (*it_i)->need_race == m_charData.m_race))
            {
                acceptTask(*it_i);
                boost::shared_ptr<CharTask> tmp_ct = getCharTask((*it_i)->id);
                if (tmp_ct.get())
                {
                    tmp.clear();
                    tmp.push_back( Pair("tid", tmp_ct->tid) );
                    tmp.push_back( Pair("type", TASK_ADD) );
                    tmp_ct->toObj(true, tmp);
                    list.push_back(tmp);
                }
            }
            ++it_i;
        }
    }
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_charData.m_name);
    if (account.get())
    {
        json_spirit::Object robj;
        robj.push_back( Pair("cmd", "updateTask") );
        robj.push_back( Pair("list", list) );
        robj.push_back( Pair("s", 200) );
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    //任务完成了，删除这个任务
    InsertSaveDb("delete from char_tasks where cid=" + LEX_CAST_STR(m_charData.m_id) + " and tid=" + LEX_CAST_STR(t.tid));
    m_all_tasks.erase(it);
    if (t.m_task->type == TASK_TYPE_DAILY)
    {
        m_charData.m_tasks.updateTask(GOAL_DONE_DAILY_TASK, 0, 1);
        //日常任务
        m_charData.updateTopButton(top_button_daily_task, getCharDailyState(), getCharDailyTaskCnt());
    }
    return HC_SUCCESS;
}

//获取玩家某任务
boost::shared_ptr<CharTask> CharAllTasks::getCharTask(int id)
{
    std::map<int, boost::shared_ptr<CharTask> >::iterator it = m_all_tasks.find(id);
    if (it != m_all_tasks.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<CharTask> p;
    p.reset();
    return p;
}

//获取玩家某类型第一个任务状态
void CharAllTasks::getCharTaskState(int type, int& tid, int& state)
{
    tid = 0, state = 0;
    std::map<int, boost::shared_ptr<CharTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_task.get())
        {
            CharTask& ct = *(it->second.get());
            if (type != 0 && type != ct.m_task->type)
            {
                ++it;
                continue;
            }
            tid = ct.tid;
            state = ct.done ? 1 : 0;
            break;
        }
        ++it;
    }
    return;
}

//玩家任务影响探索倍数
bool CharAllTasks::taskExploreSpecial()
{
    if (m_all_tasks.find(1008) != m_all_tasks.end())
    {
        return true;
    }
    if (m_all_tasks.find(1008) != m_all_tasks.end())
    {
        return true;
    }
    if (m_all_tasks.find(1008) != m_all_tasks.end())
    {
        return true;
    }
    if (m_all_tasks.find(1008) != m_all_tasks.end())
    {
        return true;
    }
    return false;
}

void CharAllTasks::load()
{
    Query q(GetDb());
    //从数据库读任务记录
    q.get_result("select tid,cur,extra from char_tasks where cid=" + LEX_CAST_STR(m_charData.m_id) + " order by tid");
    while (q.fetch_row())
    {
        int tid = q.getval();
        std::string current = q.getstr();
        std::string extradata = q.getstr();
        boost::shared_ptr<baseTask> bt = Singleton<taskMgr>::Instance().getTask(tid);
        if (!bt.get())
        {
            continue;
        }
        boost::shared_ptr<CharTask> ct(new CharTask(m_charData));
        ct->tid = tid;
        ct->m_task = bt;
        ct->done = true;
        //各目标
        std::vector<int> cur_list;
        std::vector<int> extra_list;
        json_spirit::Value curs;
        json_spirit::Value extra;
        json_spirit::read(current, curs);
        json_spirit::read(extradata, extra);
        if (curs.type() == json_spirit::array_type)
        {
            json_spirit::Array& cur_array = curs.get_array();
            for (json_spirit::Array::iterator it = cur_array.begin(); it != cur_array.end(); ++it)
            {
                if ((*it).type() != json_spirit::int_type)
                {
                    break;
                }
                cur_list.push_back((*it).get_int());
            }
        }
        else
        {
            ERR();
        }
        if (extra.type() == json_spirit::array_type)
        {
            json_spirit::Array& extra_array = extra.get_array();
            for (json_spirit::Array::iterator it = extra_array.begin(); it != extra_array.end(); ++it)
            {
                if ((*it).type() != json_spirit::int_type)
                {
                    break;
                }
                extra_list.push_back((*it).get_int());
            }
        }
        else
        {
            ERR();
        }
        int pos = 0;
        for(std::list<boost::shared_ptr<baseGoal> >::iterator it = bt->goals.begin(); it != bt->goals.end(); ++it)
        {
            if ((*it).get())
            {
                baseGoal* bg = (*it).get();
                boost::shared_ptr<CharTaskGoal> ctg(new CharTaskGoal());
                ctg->base_goal = (*it);
                ctg->type = bg->type;
                ctg->cur = 0;
                ctg->extra = 0;
                if (pos < cur_list.size())
                {
                    ctg->cur = cur_list[pos];
                    ctg->extra = extra_list[pos];
                }
                if (ctg->cur >= bg->need[1])
                {
                    ctg->cur = bg->need[1];
                    ctg->done = true;
                }
                else
                {
                    ct->done = false;
                }
                //插入目标到任务
                ct->m_goals.push_back(ctg);
            }
            ++pos;
        }
        m_all_tasks[bt->id] = ct;
    }
    q.free_result();
}

//获取玩家每日任务数量
int CharAllTasks::getCharDailyTaskCnt()
{
    int cnt = 0;
    std::map<int, boost::shared_ptr<CharTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_task.get())
        {
            CharTask& ct = *(it->second.get());
            if (ct.m_task->type == TASK_TYPE_DAILY)
            {
                ++cnt;
            }
        }
        ++it;
    }
    return cnt;
}

//获取玩家每日任务状态
int CharAllTasks::getCharDailyState()
{
    int state = 0;
    std::map<int, boost::shared_ptr<CharTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_task.get())
        {
            CharTask& ct = *(it->second.get());
            if (ct.m_task->type == TASK_TYPE_DAILY && ct.done)
            {
                state = 1;
                break;
            }
        }
        ++it;
    }
    return state;
}

//每日更新日常任务
void CharAllTasks::dailyUpdate()
{
    int state = m_charData.queryExtraData(char_data_type_daily, char_data_daily_task_refresh);
    if (state > 0)
        return;
    std::map<int, boost::shared_ptr<CharTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_task.get())
        {
            CharTask& ct = *(it->second.get());
            if (ct.m_task->type == TASK_TYPE_DAILY)
            {
                //删除日常任务
                InsertSaveDb("delete from char_tasks where cid=" + LEX_CAST_STR(m_charData.m_id) + " and tid=" + LEX_CAST_STR(ct.tid));
                m_all_tasks.erase(it++);
                continue;
            }
        }
        ++it;
    }
    std::vector<int> task_list = Singleton<taskMgr>::Instance().getDailyTasks();
    for (size_t i = 0; i < task_list.size(); ++i)
    {
        int tmp = task_list[i];
        boost::shared_ptr<baseTask> bt = Singleton<taskMgr>::Instance().getTask(tmp);
        if (bt.get())
        {
            acceptTask(bt);
        }
    }
    m_charData.setExtraData(char_data_type_daily, char_data_daily_task_refresh, 1);
    //日常任务
    m_charData.updateTopButton(top_button_daily_task, getCharDailyState(), getCharDailyTaskCnt());
}

void CharAllTasks::getButton(json_spirit::Array& list)
{
    if (m_charData.isDailyTaskOpen())
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_daily_task) );
        obj.push_back( Pair("active", getCharDailyState()) );
        obj.push_back( Pair("leftNums", getCharDailyTaskCnt()));
        list.push_back(obj);
    }
}

taskMgr::taskMgr()
{
    Query q(GetDb());
    q.get_result("select id,type,task_id,name,memo,pre_task_type,pre_task_id,done_cost,need_race,guide_id_get,guide_id_reward from base_tasks where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        baseTask* pt = new baseTask();
        pt->id = id;
        pt->type = q.getval();
        pt->task_id = q.getval();
        pt->name = q.getstr();
        pt->memo = q.getstr();
        pt->pre_task_type = q.getval();
        pt->pre_task_id = q.getval();
        pt->done_cost = q.getval();
        pt->need_race = q.getval();
        pt->guide_id_get = q.getval();
        pt->guide_id_reward = q.getval();
        //任务奖励
        pt->loadRewards();
        //任务目标
        pt->loadGoals();
        boost::shared_ptr<baseTask> bt(pt);
        m_total_tasks[id] = bt;
        if (pt->type == TASK_TYPE_DAILY && pt->pre_task_type == 0 && pt->pre_task_id == 0)
        {
            m_daily_tasks.push_back(id);
        }
    }
    q.free_result();
    //关联任务
    std::map<int, boost::shared_ptr<baseTask> >::iterator it = m_total_tasks.begin();
    while (it != m_total_tasks.end())
    {
        if (!(it->second).get() || (it->second->pre_task_type == 0 && it->second->pre_task_id == 0))
        {
            ++it;
            continue;
        }
        boost::shared_ptr<baseTask> pre = getTask(it->second->pre_task_type,it->second->pre_task_id);
        if (pre.get())
        {
            pre->m_child_tasks.push_back(it->second);
        }
        ++it;
    }
}

boost::shared_ptr<baseTask> taskMgr::getTask(int tid) //根据任务id获得任务
{
    if (m_total_tasks.find(tid) != m_total_tasks.end())
    {
        //cout << "TRUE,taskMgr::getTask id=" << tid << endl;
        return m_total_tasks[tid];
    }
    else
    {
        //cout << "FALSE,taskMgr::getTask id=" << tid << endl;
        boost::shared_ptr<baseTask> tt;
        return tt;
    }
}

boost::shared_ptr<baseTask> taskMgr::getTask(int type, int task_id) //根据任务类型和分类id获得任务
{
    std::map<int, boost::shared_ptr<baseTask> >::iterator it = m_total_tasks.begin();
    while (it != m_total_tasks.end())
    {
        if (!it->second.get())
        {
            ++it;
            continue;
        }
        if (type == it->second->type && task_id == it->second->task_id)
        {
            return it->second;
        }
        ++it;
    }
    boost::shared_ptr<baseTask> tt;
    return tt;
}

void taskMgr::addTask(int tid, boost::shared_ptr<baseTask> bt)
{
    if (m_total_tasks.find(tid) != m_total_tasks.end())
    {
        return;
    }
    else
    {
        m_total_tasks[tid] = bt;
        return;
    }
}

void taskMgr::removeTask(int tid)
{
    if (m_total_tasks.find(tid) != m_total_tasks.end())
    {
        m_total_tasks.erase(tid);
        return;
    }
    return;
}

void taskMgr::reLink()
{
    //取消关联
    std::map<int, boost::shared_ptr<baseTask> >::iterator it = m_total_tasks.begin();
    while (it != m_total_tasks.end())
    {
        if (!it->second.get())
        {
            ++it;
            continue;
        }
        it->second->m_child_tasks.clear();
        ++it;
    }
    //重新关联
    it = m_total_tasks.begin();
    while (it != m_total_tasks.end())
    {
        if (!it->second.get())
        {
            ++it;
            continue;
        }
        boost::shared_ptr<baseTask> pre = getTask(it->second->pre_task_type,it->second->pre_task_id);
        if (pre.get())
        {
            pre->m_child_tasks.push_back(it->second);
        }
        ++it;
    }
    return;
}

int taskMgr::newChar(boost::shared_ptr<CharData> cdata)
{
    if (!cdata.get())
    {
        return HC_ERROR;
    }
    if(cdata->m_tasks.m_all_tasks.size() == 0)
    {
        boost::shared_ptr<baseTask> first = getTask(1);
        cdata->m_tasks.acceptTask(first);
    }
    return HC_SUCCESS;
}

int taskMgr::RandomDailyTask()
{
    int idx = my_random(0, m_daily_tasks.size() - 1);
    return m_daily_tasks[idx];
}

int ProcessTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    json_spirit::Array tlist;
    cdata->m_tasks.getList(type, tlist);
    robj.push_back( Pair("task_list", tlist) );
    return HC_SUCCESS;
}

int ProcessTaskInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int tid = 0;
    READ_INT_FROM_MOBJ(tid, o, "tid");
    boost::shared_ptr<CharTask> pct = cdata->m_tasks.getCharTask(tid);
    if (pct.get())
    {
        pct->toObj(false, robj);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int ProcessTaskDone(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int tid = 0, cost = 0;
    READ_INT_FROM_MOBJ(tid, o, "tid");
    READ_INT_FROM_MOBJ(cost, o, "cost");
    return cdata->m_tasks.taskDone(tid, cost, robj);
}

