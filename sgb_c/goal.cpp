
#include "goal.h"
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
#include "copy.h"

Database& GetDb();
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

void baseGoalTask::loadRewards()
{
    reward.clear();
    vip_reward.clear();
    Query q(GetDb());
    q.get_result("select itemType,itemId,counts,extra from base_goal_tasks_reward where type = '1' and taskid="+LEX_CAST_STR(id)+" order by id");
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

    q.get_result("select itemType,itemId,counts,extra from base_goal_tasks_reward where type = '2' and taskid="+LEX_CAST_STR(id)+" order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();
        item.extra = q.getval();
        vip_reward.push_back(item);
    }
    q.free_result();
}

void baseGoalTask::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", id));
    obj.push_back( Pair("spic", spic));
    obj.push_back( Pair("type", type));
    obj.push_back( Pair("need_extra", need_extra));
    obj.push_back( Pair("need_vip", need_vip));
    obj.push_back( Pair("name", name));
    obj.push_back( Pair("memo", memo));
    json_spirit::Array reward_list;
    itemlistToArray(reward,reward_list);
    obj.push_back( Pair("reward_list", reward_list));
    reward_list.clear();
    itemlistToArray(vip_reward,reward_list);
    obj.push_back( Pair("vip_reward_list", reward_list));
    return;
}

void baseGoalTask::toSimpleObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", id));
    obj.push_back( Pair("spic", spic));
    obj.push_back( Pair("need_level", need_level));
    obj.push_back( Pair("name", name));
    obj.push_back( Pair("memo", memo));
    return;
}

void baseGoalLevelReward::loadRewards()
{
    reward.clear();
    Query q(GetDb());
    q.get_result("select itemType,itemId,counts,extra from base_goal_level_reward where tid="+LEX_CAST_STR(id)+" order by id");
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

void baseGoalLevelReward::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", id));
    obj.push_back( Pair("spic", id));
    obj.push_back( Pair("level", need_level));
    obj.push_back( Pair("name", name));
    obj.push_back( Pair("memo", memo));
    json_spirit::Array reward_list;
    itemlistToArray(reward,reward_list);
    obj.push_back( Pair("reward_list", reward_list));
    return;
}

void baseGoalLevelReward::toSimpleObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", id));
    obj.push_back( Pair("spic", id));
    obj.push_back( Pair("need_level", need_level));
    obj.push_back( Pair("name", name));
    obj.push_back( Pair("memo", memo));
    return;
}

void CharGoalTask::Save()
{
    InsertSaveDb("update char_goals set cur='" + LEX_CAST_STR(cur)
        + "',normal_state='" + LEX_CAST_STR(normal_state)
        + "',vip_state='" + LEX_CAST_STR(vip_state)
        + "' where cid=" + LEX_CAST_STR(cid)
        + " and tid=" + LEX_CAST_STR(tid));
}

void CharGoalTask::toObj(json_spirit::Object& obj)
{
    //基础信息，描述名字
    if (m_base.get())
    {
        m_base->toObj(obj);
        obj.push_back( Pair("cur", cur) );
        obj.push_back( Pair("normal_state", normal_state) );
        obj.push_back( Pair("vip_state", vip_state) );
        obj.push_back( Pair("isDone", done ? 1 : 0) );
        std::string goal = m_base->goal;
        if (goal.find("$N") != std::string::npos)
        {
            str_replace(goal, "$N", LEX_CAST_STR(cur) + "/" + LEX_CAST_STR(m_base->need_extra));
        }
        obj.push_back( Pair("goal", goal) );
    }
    return;
}

void CharGoalTask::toSimpleObj(json_spirit::Object& obj)
{
    //基础信息，描述名字
    if (m_base.get())
    {
        m_base->toSimpleObj(obj);
        std::string goal = m_base->goal;
        if (goal.find("$N") != std::string::npos)
        {
            str_replace(goal, "$N", LEX_CAST_STR(cur) + "/" + LEX_CAST_STR(m_base->need_extra));
        }
        obj.push_back( Pair("goal", goal) );
    }
    return;
}

//接受任务
void CharGoal::acceptTask(boost::shared_ptr<baseGoalTask> t)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL)
    {
        return;
    }
    cout << "CharGoal::acceptTask" <<endl;
    if (t.get())
    {
        cout << "CharGoal::acceptTask tid=" << t->id <<endl;
        boost::shared_ptr<CharGoalTask> ct(new CharGoalTask());
        ct->cid = cid;
        ct->tid = t->id;
        ct->m_base = t;
        ct->cur = 0;
        ct->normal_state = CharGoalTask::STATE_INIT;
        ct->vip_state = CharGoalTask::STATE_INIT;
        ct->done = false;
        switch (t->type)
        {
            case GOAL_TYPE_CHAR_LEVEL:
            {
                ct->cur = pc->m_level;
                break;
            }
            case GOAL_TYPE_EQUIPT1_QUALITY:
            case GOAL_TYPE_EQUIPT2_QUALITY:
            case GOAL_TYPE_EQUIPT3_QUALITY:
            case GOAL_TYPE_EQUIPT4_QUALITY:
            {
                int etype = t->type - GOAL_TYPE_EQUIPT1_QUALITY + 1;
                int tmp = 0;
                std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = pc->m_heros.m_heros.begin();
                while (it != pc->m_heros.m_heros.end())
                {
                    if(it->second.get())
                    {
                        tmp = it->second->m_bag.bestEquipmentQuality(etype);
                        if (tmp > ct->cur)
                            ct->cur = tmp;
                    }
                    ++it;
                }
                break;
            }
            case GOAL_TYPE_METALLURGY_LEVEL:
            {
                boost::shared_ptr<char_metallurgy> cc = Singleton<cityMgr>::Instance().getCharMetallurgy(pc->m_id);
                if (cc)
                {
                    ct->cur = cc->m_level;
                }
                break;
            }
            case GOAL_TYPE_SHENLING_TOWER:
            {
                boost::shared_ptr<CharShenling> cc = Singleton<shenlingMgr>::Instance().getCharShenling(pc->m_id);
                if (cc)
                {
                    ct->cur = cc->m_sid - 1;
                }
                break;
            }
            case GOAL_TYPE_VIP:
            {
                ct->cur = pc->m_vip;
                break;
            }
            case GOAL_TYPE_COPY_MAP:
            {
                ct->cur = Singleton<copyMgr>::Instance().getCharCurMap(pc->m_id) - 1;
                break;
            }
            case GOAL_TYPE_HERO_CNT:
            {
                ct->cur = pc->m_heros.m_heros.size();
                break;
            }
            case GOAL_TYPE_CASTLE_LEVEL:
            {
                char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(pc->m_id);
                if (cc)
                {
                    ct->cur = cc->m_level;
                }
                break;
            }
            case GOAL_TYPE_HERO_STAR:
            {
                std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = pc->m_heros.m_heros.begin();
                while (it != pc->m_heros.m_heros.end())
                {
                    if(it->second.get() && it->second->m_star > ct->cur)
                    {
                        ct->cur = it->second->m_star;
                    }
                    ++it;
                }
                break;
            }
            case GOAL_TYPE_SILVER:
            {
                ct->cur = pc->silver();
                break;
            }
            case GOAL_TYPE_SMITHY_LEVEL:
            {
                boost::shared_ptr<char_smithy> cc = Singleton<cityMgr>::Instance().getCharSmithy(pc->m_id);
                if (cc)
                {
                    ct->cur = cc->m_level;
                }
                break;
            }
            case GOAL_TYPE_EPIC_HERO:
            {
                std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = pc->m_heros.m_heros.begin();
                while (it != pc->m_heros.m_heros.end())
                {
                    if(it->second.get() && it->second->m_baseHero.get() && it->second->m_baseHero->m_epic)
                    {
                        ++ct->cur;
                    }
                    ++it;
                }
                break;
            }
            case GOAL_TYPE_RESIDENT:
            {
                char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(pc->m_id);
                if (cc)
                {
                    ct->cur = cc->m_resident;
                }
                break;
            }
            case GOAL_TYPE_EQUIPT_QUALITY:
            {
                std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = pc->m_heros.m_heros.begin();
                while (it != pc->m_heros.m_heros.end())
                {
                    if(it->second.get())
                    {
                        int tmp = it->second->m_bag.bestEquipmentQuality(0);
                        if (tmp > ct->cur)
                            ct->cur = tmp;
                    }
                    ++it;
                }
                break;
            }
            case GOAL_TYPE_EQUIPT1_UPGRADE:
            case GOAL_TYPE_EQUIPT2_UPGRADE:
            case GOAL_TYPE_EQUIPT3_UPGRADE:
            case GOAL_TYPE_EQUIPT4_UPGRADE:
            {
                int etype = t->type - GOAL_TYPE_EQUIPT1_UPGRADE + 1;
                int tmp = 0;
                std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = pc->m_heros.m_heros.begin();
                while (it != pc->m_heros.m_heros.end())
                {
                    if(it->second.get())
                    {
                        tmp = it->second->m_bag.bestEquipmentLevel(etype);
                        if (tmp > ct->cur)
                            ct->cur = tmp;
                    }
                    ++it;
                }
                tmp = pc->m_bag.bestEquipmentLevel(etype);
                if (tmp > ct->cur)
                    ct->cur = tmp;
                break;
            }
            default:
                break;
        }
        if (ct->cur >= t->need_extra)
        {
            ct->cur = t->need_extra;
            ct->done = true;
            ct->normal_state = CharGoalTask::STATE_GET;
            if (pc->m_vip >= t->need_vip)
                ct->vip_state = CharGoalTask::STATE_GET;
        }
        m_all_tasks[t->id] = ct;
        InsertSaveDb("insert into char_goals set cur='" + LEX_CAST_STR(ct->cur)
            + "',normal_state='" + LEX_CAST_STR(ct->normal_state)
            + "',vip_state='" + LEX_CAST_STR(ct->vip_state)
            + "',cid=" + LEX_CAST_STR(cid)
            + ",tid=" + LEX_CAST_STR(ct->tid));
    }
}

//任务列表
int CharGoal::getList(int level, json_spirit::Array& rlist)
{
    if (level == 0)
    {
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc == NULL)
        {
            return HC_ERROR;
        }
        std::map<int, boost::shared_ptr<CharGoalTask> >::reverse_iterator re_it = m_all_tasks.rbegin();
        while (re_it != m_all_tasks.rend())
        {
            if (re_it->second.get() && re_it->second->m_base.get() && re_it->second->m_base->need_level <= pc->m_level)
            {
                level = re_it->second->m_base->need_level;
                break;
            }
            ++re_it;
        }
    }
    std::map<int, boost::shared_ptr<CharGoalTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_base.get() && it->second->m_base->need_level == level)
        {
            CharGoalTask& ct = *(it->second.get());
            json_spirit::Object obj;
            ct.toObj(obj);
            rlist.push_back(obj);
        }
        ++it;
    }
    return HC_SUCCESS;
}

//更新任务
int CharGoal::updateTask(int goal_type, int extra)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL)
    {
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<CharGoalTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_base.get())
        {
            CharGoalTask& ct = *(it->second.get());
            if (ct.done || ct.m_base->type != goal_type || ct.m_base->need_level > pc->m_level)
            {
                ++it;
                continue;
            }
            switch (ct.m_base->type)
            {
                case GOAL_TYPE_CHAR_LEVEL:
                case GOAL_TYPE_EQUIPT1_QUALITY:
                case GOAL_TYPE_EQUIPT2_QUALITY:
                case GOAL_TYPE_EQUIPT3_QUALITY:
                case GOAL_TYPE_EQUIPT4_QUALITY:
                case GOAL_TYPE_METALLURGY_LEVEL:
                case GOAL_TYPE_SHENLING_TOWER:
                case GOAL_TYPE_VIP:
                case GOAL_TYPE_HERO_PACK:
                case GOAL_TYPE_COPY_MAP:
                case GOAL_TYPE_HERO_CNT:
                case GOAL_TYPE_TREASURE_QUALITY:
                case GOAL_TYPE_CASTLE_LEVEL:
                case GOAL_TYPE_HERO_STAR:
                case GOAL_TYPE_SILVER:
                case GOAL_TYPE_SMITHY_LEVEL:
                case GOAL_TYPE_EPIC_HERO:
                case GOAL_TYPE_RESIDENT:
                case GOAL_TYPE_EQUIPT_QUALITY:
                case GOAL_TYPE_EQUIPT1_UPGRADE:
                case GOAL_TYPE_EQUIPT2_UPGRADE:
                case GOAL_TYPE_EQUIPT3_UPGRADE:
                case GOAL_TYPE_EQUIPT4_UPGRADE:
                {
                    ct.cur = extra;
                    break;
                }
                case GOAL_TYPE_SIGN:
                case GOAL_TYPE_RECRUIT:
                case GOAL_TYPE_UPGRADE_EQUIPT:
                case GOAL_TYPE_LEVY:
                case GOAL_TYPE_EXPLORE:
                case GOAL_TYPE_COMPOUND_HERO:
                case GOAL_TYPE_ARENA_WIN:
                case GOAL_TYPE_WILD_WIN:
                case GOAL_TYPE_DECOMPOSE_HERO:
                {
                    ct.cur += extra;
                    break;
                }
                default:
                    break;
            }
            if (ct.cur >= ct.m_base->need_extra)
            {
                ct.cur = ct.m_base->need_extra;
                ct.done = true;
                ct.normal_state = CharGoalTask::STATE_GET;
                if (pc->m_vip >= ct.m_base->need_vip)
                    ct.vip_state = CharGoalTask::STATE_GET;
            }
            else
            {
                ct.done = false;
            }
            if (ct.done == true)
            {
                getCharGoalTag();
            }
            ct.Save();
        }
        ++it;
    }
    return HC_SUCCESS;
}

int CharGoal::updateVip(int vip)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL)
    {
        return HC_ERROR;
    }
    bool notify = false;
    std::map<int, boost::shared_ptr<CharGoalTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_base.get())
        {
            CharGoalTask& ct = *(it->second.get());
            if (!ct.done)
            {
                ++it;
                continue;
            }
            if (ct.vip_state == CharGoalTask::STATE_INIT && ct.m_base->need_vip <= vip)
            {
                ct.vip_state = CharGoalTask::STATE_GET;
                notify = true;
            }
            ct.Save();
        }
        ++it;
    }
    if (notify)
    {
        getCharGoalTag();
    }
    return HC_SUCCESS;
}

int CharGoal::updateLevel(int level)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL)
    {
        return HC_ERROR;
    }
    //升级可能需要接新任务
    std::map<int, boost::shared_ptr<baseGoalTask> > all_tasks = Singleton<goalMgr>::Instance().getGoalTasks();
    std::map<int, boost::shared_ptr<baseGoalTask> >::iterator it_t = all_tasks.begin();
    while (it_t != all_tasks.end())
    {
        if (it_t->second.get() && it_t->second->need_level <= pc->m_level)
        {
            if (m_all_tasks.find(it_t->first) == m_all_tasks.end())
            {
                acceptTask(it_t->second);
            }
        }
        ++it_t;
    }
    //升级可能更新等级奖励
    std::map<int, boost::shared_ptr<baseGoalLevelReward> > level_rewards = Singleton<goalMgr>::Instance().getLevelRewards();
    std::map<int, boost::shared_ptr<baseGoalLevelReward> >::iterator it_level = level_rewards.begin();
    while (it_level != level_rewards.end())
    {
        if (it_level->second.get())
        {
            int level_state = pc->queryExtraData(char_data_type_normal,char_data_normal_goal_level_reward_start+it_level->first);
            if (level_state == CharGoalTask::STATE_INIT && it_level->second->need_level <= level)
            {
                pc->setExtraData(char_data_type_normal,char_data_normal_goal_level_reward_start+it_level->first,CharGoalTask::STATE_GET);
            }
        }
        ++it_level;
    }
    getCharGoalTag();
    return HC_SUCCESS;
}

//领取奖励
int CharGoal::getReward(int id, int type, json_spirit::Object& robj)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL)
    {
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<CharGoalTask> >::iterator it = m_all_tasks.find(id);
    if (it == m_all_tasks.end() || !it->second.get())
    {
        return HC_ERROR;
    }
    CharGoalTask& t = *(it->second.get());
    if (!t.m_base.get())
    {
        return HC_ERROR;
    }
    if (type == 1 && t.normal_state == CharGoalTask::STATE_GET)
    {
        std::list<Item> items = t.m_base->reward;
        if (!pc->m_bag.hasSlot(itemlistNeedBagSlot(items)))
        {
            return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
        }
        //给奖励
        giveLoots(pc,items,NULL,&robj,true,loot_goal);
        t.normal_state = CharGoalTask::STATE_ALREADY;
    }
    else if (type == 2 && t.vip_state == CharGoalTask::STATE_GET)
    {
        std::list<Item> items = t.m_base->vip_reward;
        if (!pc->m_bag.hasSlot(itemlistNeedBagSlot(items)))
        {
            return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
        }
        //给奖励
        giveLoots(pc,items,NULL,&robj,true,loot_goal);
        t.vip_state = CharGoalTask::STATE_ALREADY;
    }
    else
    {
        return HC_ERROR;
    }
    t.Save();
    getCharGoalTag();
    return HC_SUCCESS;
}

//领取奖励
int CharGoal::getLevelReward(int id, json_spirit::Object& robj)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<baseGoalLevelReward> base = Singleton<goalMgr>::Instance().getLevelReward(id);
    if (base.get() == NULL)
    {
        return HC_ERROR;
    }
    if (pc->m_level < base->need_level)
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    int state = pc->queryExtraData(char_data_type_normal,char_data_normal_goal_level_reward_start+id);
    if (state != CharGoalTask::STATE_GET)
    {
        return HC_ERROR;
    }
    std::list<Item> items = base->reward;
    if (!pc->m_bag.hasSlot(itemlistNeedBagSlot(items)))
    {
        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
    }
    //给奖励
    giveLoots(pc,items,NULL,&robj,true,loot_goal);
    pc->setExtraData(char_data_type_normal,char_data_normal_goal_level_reward_start+id,CharGoalTask::STATE_ALREADY);
    getCharGoalTag();
    return HC_SUCCESS;
}

//任务列表
boost::shared_ptr<CharGoalTask> CharGoal::getCharGoalTask(int tid)
{
    if (m_all_tasks.find(tid) != m_all_tasks.end())
    {
        return m_all_tasks[tid];
    }
    else
    {
        boost::shared_ptr<CharGoalTask> tt;
        return tt;
    }
}

void CharGoal::load()
{
    Query q(GetDb());
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL)
    {
        return;
    }
    q.get_result("select tid,cur,normal_state,vip_state from char_goals where cid=" + LEX_CAST_STR(cid) + " order by tid");
    while (q.fetch_row())
    {
        int tid = q.getval();
        boost::shared_ptr<baseGoalTask> bt = Singleton<goalMgr>::Instance().getGoalTask(tid);
        if (!bt.get())
        {
            continue;
        }
        boost::shared_ptr<CharGoalTask> ct(new CharGoalTask());
        ct->cid = cid;
        ct->tid = tid;
        ct->cur = q.getval();
        ct->normal_state = q.getval();
        ct->vip_state = q.getval();
        ct->m_base = bt;
        if (ct->cur >= ct->m_base->need_extra)
        {
            ct->done = true;
            if (ct->normal_state == CharGoalTask::STATE_INIT)
            {
                ct->normal_state = CharGoalTask::STATE_GET;
            }
            if (pc->m_vip >= ct->m_base->need_vip && ct->vip_state == CharGoalTask::STATE_INIT)
            {
                ct->vip_state = CharGoalTask::STATE_GET;
            }
        }
        else
        {
            ct->done = false;
        }
        m_all_tasks[bt->id] = ct;
    }
    q.free_result();
    std::map<int, boost::shared_ptr<baseGoalTask> > all_tasks = Singleton<goalMgr>::Instance().getGoalTasks();
    std::map<int, boost::shared_ptr<baseGoalTask> >::iterator it_t = all_tasks.begin();
    while (it_t != all_tasks.end())
    {
        if (it_t->second.get() && it_t->second->need_level <= pc->m_level)
        {
            if (m_all_tasks.find(it_t->first) == m_all_tasks.end())
            {
                acceptTask(it_t->second);
            }
        }
        ++it_t;
    }
}

//获取玩家领取状态
int CharGoal::getCharGoalState(int& type, int& need_level, int& id)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL)
    {
        return 0;
    }
    int state = 0;
    std::map<int, boost::shared_ptr<CharGoalTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_base.get() && it->second->m_base->need_level <= pc->m_level)
        {
            CharGoalTask& ct = *(it->second.get());
            if (ct.normal_state == CharGoalTask::STATE_GET || ct.vip_state == CharGoalTask::STATE_GET)
            {
                state = 1;
                type = 1;
                id = it->first;
                need_level = ct.m_base->need_level;
                break;
            }
        }
        ++it;
    }
    if (state == 0)
    {
        std::map<int, boost::shared_ptr<baseGoalLevelReward> > level_rewards = Singleton<goalMgr>::Instance().getLevelRewards();
        std::map<int, boost::shared_ptr<baseGoalLevelReward> >::iterator it_level = level_rewards.begin();
        while (it_level != level_rewards.end())
        {
            if (it_level->second.get())
            {
                int level_state = pc->queryExtraData(char_data_type_normal,char_data_normal_goal_level_reward_start+it_level->first);
                if (level_state == CharGoalTask::STATE_GET)
                {
                    state = 1;
                    type = 2;
                    id = it_level->first;
                    break;
                }
            }
            ++it_level;
        }
    }
    return state;
}

//推送目标侧边栏
void CharGoal::getCharGoalTag()
{
    int type = 0, id = 0, need_level = 0, next_id = 0;
    bool get = false;
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL)
    {
        return;
    }
    std::map<int, boost::shared_ptr<CharGoalTask> >::iterator it = m_all_tasks.begin();
    while (it != m_all_tasks.end())
    {
        if (it->second.get() && it->second->m_base.get() && it->second->m_base->need_level <= pc->m_level)
        {
            CharGoalTask& ct = *(it->second.get());
            if (ct.normal_state == CharGoalTask::STATE_GET || ct.vip_state == CharGoalTask::STATE_GET)
            {
                type = 1;
                id = it->first;
                need_level = ct.m_base->need_level;
                get = true;
                cout << "CharGoal::getCharGoalTag canget " << "type=1,id=" << id << ",need_level=" << need_level << endl;
                break;
            }
        }
        ++it;
    }
    if (!get)
    {
        std::map<int, boost::shared_ptr<baseGoalLevelReward> > level_rewards = Singleton<goalMgr>::Instance().getLevelRewards();
        std::map<int, boost::shared_ptr<baseGoalLevelReward> >::iterator it_level = level_rewards.begin();
        while (it_level != level_rewards.end())
        {
            if (it_level->second.get())
            {
                int level_state = pc->queryExtraData(char_data_type_normal,char_data_normal_goal_level_reward_start+it_level->first);
                if (level_state == CharGoalTask::STATE_GET)
                {
                    type = 2;
                    id = it_level->first;
                    get = true;
                    cout << "CharGoal::getCharGoalTag canget " << "type=2,id=" << id << endl;
                    break;
                }
                if (next_id == 0 && it_level->second->need_level == (pc->m_level + 1))
                {
                    type = 2;
                    next_id = it_level->first;
                }
            }
            ++it_level;
        }
    }
    if (!get && next_id > 0)
    {
        id = next_id;
        cout << "CharGoal::getCharGoalTag cannot get " << "but has next_id=" << id << endl;
    }
    if (type > 0 && id > 0)
    {
        int active = get ? 1 : 0;
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "getGoalTag"));
        obj.push_back( Pair("s",200) );
        obj.push_back( Pair("active", active) );
        obj.push_back( Pair("type", type) );
        if (type == 1)
        {
            json_spirit::Object info;
            boost::shared_ptr<CharGoalTask> bg = getCharGoalTask(id);
            bg->toSimpleObj(info);
            obj.push_back( Pair("info", info) );
        }
        else if (type == 2)
        {
            json_spirit::Object info;
            boost::shared_ptr<baseGoalLevelReward> bg = Singleton<goalMgr>::Instance().getLevelReward(id);
            bg->toSimpleObj(info);
            obj.push_back( Pair("info", info) );
        }
        pc->sendObj(obj);
        pc->updateTopButton(top_button_goal, active, 0, type, need_level, id);
    }
    else
    {
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "removeGoalTag"));
        obj.push_back( Pair("s",200) );
        pc->sendObj(obj);
        pc->updateTopButton(top_button_goal, 0);
    }
    return;
}

void CharGoal::getButton(json_spirit::Array& list)
{
    int extra1 = 0, extra2 = 0, extra3 = 0;
    json_spirit::Object obj;
    obj.push_back( Pair("type", top_button_goal) );
    obj.push_back( Pair("active", getCharGoalState(extra1,extra2,extra3)) );
    obj.push_back( Pair("leftNums", 0));
    obj.push_back( Pair("extra1", extra1));
    obj.push_back( Pair("extra2", extra2));
    obj.push_back( Pair("extra3", extra3));
    list.push_back(obj);
    //同时推送侧边栏
    getCharGoalTag();
}

goalMgr::goalMgr()
{
    Query q(GetDb());
    q.get_result("select id,spic,type,need_extra,need_level,need_vip,name,memo,goal from base_goals where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        baseGoalTask* pt = new baseGoalTask();
        pt->id = id;
        pt->spic = q.getval();
        pt->type = q.getval();
        pt->need_extra = q.getval();
        pt->need_level = q.getval();
        pt->need_vip = q.getval();
        pt->name = q.getstr();
        pt->memo = q.getstr();
        pt->goal = q.getstr();
        //任务奖励
        pt->loadRewards();
        boost::shared_ptr<baseGoalTask> bt(pt);
        m_total_tasks[id] = bt;
    }
    q.free_result();
    q.get_result("select id,need_level,name,memo from base_goal_level where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int level = q.getval();
        baseGoalLevelReward* pt = new baseGoalLevelReward();
        pt->id = id;
        pt->need_level = level;
        pt->name = q.getstr();
        pt->memo = q.getstr();
        //任务奖励
        pt->loadRewards();
        boost::shared_ptr<baseGoalLevelReward> bt(pt);
        m_level_rewards[id] = bt;
    }
    q.free_result();
    q.get_result("select id,need_level,org_cost,cost,itemType,itemId,counts,extra from base_goal_shop where 1 order by id asc");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseGoalGoods> bg;
        bg.reset(new baseGoalGoods);
        bg->id = id;
        bg->need_level = q.getval();
        bg->org_cost = q.getval();
        bg->cost = q.getval();
        bg->reward.type = q.getval();
        bg->reward.id = q.getval();
        bg->reward.nums = q.getval();
        bg->reward.extra = q.getval();
        m_goods[id] = bg;
    }
    q.free_result();
}

boost::shared_ptr<CharGoal> goalMgr::getCharGoal(int cid)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL)
    {
        boost::shared_ptr<CharGoal> cg;
        cg.reset();
        return cg;
    }
    if (m_char_goals.find(cid) != m_char_goals.end())
    {
        return m_char_goals[cid];
    }
    else
    {
        boost::shared_ptr<CharGoal> cg;
        cg.reset(new CharGoal());
        cg->cid = cid;
        cg->load();
        m_char_goals[cid]=cg;
        return m_char_goals[cid];
    }
}

boost::shared_ptr<baseGoalTask> goalMgr::getGoalTask(int tid)
{
    if (m_total_tasks.find(tid) != m_total_tasks.end())
    {
        return m_total_tasks[tid];
    }
    else
    {
        boost::shared_ptr<baseGoalTask> tt;
        return tt;
    }
}

boost::shared_ptr<baseGoalLevelReward> goalMgr::getLevelReward(int id)
{
    if (m_level_rewards.find(id) != m_level_rewards.end())
    {
        return m_level_rewards[id];
    }
    else
    {
        boost::shared_ptr<baseGoalLevelReward> tt;
        return tt;
    }
}

void goalMgr::getGoalGood(int level, json_spirit::Array& list)
{
    std::map<int, boost::shared_ptr<baseGoalGoods> >::iterator it = m_goods.begin();
    while (it != m_goods.end())
    {
        if (it->second.get() && it->second->need_level == level)
        {
            baseGoalGoods* bpg = it->second.get();
            if (NULL == bpg)
            {
                ERR();
                ++it;
                continue;
            }
            json_spirit::Object obj;
            obj.push_back( Pair("id", bpg->id));
            obj.push_back( Pair("need_level", bpg->need_level));
            obj.push_back( Pair("org_cost", bpg->org_cost));
            obj.push_back( Pair("cost", bpg->cost));
            json_spirit::Object i_obj;
            bpg->reward.toObj(i_obj);
            obj.push_back( Pair("item_info", i_obj));
            list.push_back(obj);
        }
        ++it;
    }
}

boost::shared_ptr<baseGoalGoods> goalMgr::getBaseGood(int id)
{
    if (m_goods.find(id) != m_goods.end())
    {
        return m_goods[id];
    }
    else
    {
        boost::shared_ptr<baseGoalGoods> tt;
        return tt;
    }
}

int goalMgr::updateTask(int cid, int goal_type, int extra)
{
    boost::shared_ptr<CharGoal> cg = getCharGoal(cid);
    if (cg.get())
    {
        return cg->updateTask(goal_type, extra);
    }
    return HC_ERROR;
}

int goalMgr::updateVip(int cid, int vip)
{
    boost::shared_ptr<CharGoal> cg = getCharGoal(cid);
    if (cg.get())
    {
        return cg->updateVip(vip);
    }
    return HC_ERROR;
}

int goalMgr::updateLevel(int cid, int level)
{
    boost::shared_ptr<CharGoal> cg = getCharGoal(cid);
    if (cg.get())
    {
        return cg->updateLevel(level);
    }
    return HC_ERROR;
}

int ProcessGoalTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int level = 0;
    READ_INT_FROM_MOBJ(level, o, "level");
    robj.push_back( Pair("level", level) );
    json_spirit::Array tlist;
    boost::shared_ptr<CharGoal> cg = Singleton<goalMgr>::Instance().getCharGoal(cdata->m_id);
    if (cg.get())
    {
        cg->getList(level, tlist);
        robj.push_back( Pair("list", tlist) );
    }
    return HC_SUCCESS;
}

int ProcessGoalLevelList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    json_spirit::Array tlist;
    std::map<int, boost::shared_ptr<baseGoalLevelReward> > level_rewards = Singleton<goalMgr>::Instance().getLevelRewards();
    std::map<int, boost::shared_ptr<baseGoalLevelReward> >::iterator it_level = level_rewards.begin();
    while (it_level != level_rewards.end())
    {
        if (it_level->second.get())
        {
            int state = cdata->queryExtraData(char_data_type_normal,char_data_normal_goal_level_reward_start+it_level->first);
            if (state == CharGoalTask::STATE_INIT && cdata->m_level >= it_level->second->need_level)
            {
                cdata->setExtraData(char_data_type_normal,char_data_normal_goal_level_reward_start+it_level->first,CharGoalTask::STATE_GET);
                state = CharGoalTask::STATE_GET;
            }
            json_spirit::Object obj;
            it_level->second->toObj(obj);
            obj.push_back( Pair("state", state) );
            tlist.push_back(obj);
        }
        ++it_level;
    }
    robj.push_back( Pair("list", tlist) );
    return HC_SUCCESS;
}

int ProcessGoalReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int tid = 0, type = 0, purpose = 0;
    READ_INT_FROM_MOBJ(tid, o, "tid");
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    robj.push_back( Pair("purpose", purpose) );
    boost::shared_ptr<CharGoal> cg = Singleton<goalMgr>::Instance().getCharGoal(cdata->m_id);
    if (cg.get())
    {
        if (purpose == 1)
        {
            return cg->getReward(tid, type, robj);
        }
        else if (purpose == 2)
        {
            return cg->getLevelReward(tid, robj);
        }
    }
    return HC_ERROR;
}

int ProcessGoalShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }

    int level = 1;
    READ_INT_FROM_MOBJ(level, o, "level");
    robj.push_back( Pair("level", level) );
    json_spirit::Array list;
    Singleton<goalMgr>::Instance().getGoalGood(level, list);
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int ProcessBuyGoalGood(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0, nums = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    boost::shared_ptr<baseGoalGoods> bg = Singleton<goalMgr>::Instance().getBaseGood(id);
    if (bg.get())
    {
        if (bg->need_level > cdata->m_level)
        {
            return HC_ERROR_NEED_MORE_LEVEL;
        }
        int cost_gold = bg->cost * nums;
        if (cost_gold < 0)
        {
            return HC_ERROR;
        }
        std::list<Item> items;
        Item tmp(bg->reward.type, bg->reward.id, bg->reward.nums * nums, bg->reward.extra);
        items.push_back(tmp);
        if (!cdata->m_bag.hasSlot(itemlistNeedBagSlot(items)))
        {
            return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
        }
        if (tmp.type == ITEM_TYPE_HERO && (cdata->m_heros.m_hero_max - cdata->m_heros.m_heros.size()) < nums)
            return HC_ERROR_HERO_FULL;
        if (cdata->subGold(cost_gold, gold_cost_buy_goal, true) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        giveLoots(cdata.get(),items,NULL,&robj,true,loot_goal_shop);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

