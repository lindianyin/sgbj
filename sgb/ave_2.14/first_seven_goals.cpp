#include "first_seven_goals.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "singleton.h"
#include "maze.h"
#include "data.h"
#include "guard.h"
#include "spls_race.h"
#include "statistics.h"

Database& GetDb();

int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

extern volatile int iGuardTimes;

void Goals::toObj(json_spirit::Object& obj)
{
    std::string n = "";
    obj.push_back( Pair("id", id) );
    obj.push_back( Pair("type", type) );
    switch (type)
    {
        case goals_type_level:
        case goals_type_daily_score:
        case goals_type_attack_power:
        case goals_type_guard:
        case goals_type_race:
            obj.push_back( Pair("param", param) );
            break;
        case goals_type_maze:
            {
                boost::shared_ptr<base_maze> tr = Singleton<mazeMgr>::Instance().getBaseMaze(param);
                if (tr.get())
                {
                    n = tr->name;
                }
                break;
            }
        case goals_type_general:
            {
                boost::shared_ptr<GeneralTypeData> tr = GeneralDataMgr::getInstance()->GetBaseGeneral(param);
                if (tr.get())
                {
                    n = tr->m_name;
                }
                break;
            }
        case goals_type_equipt:
            {
                boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment(param);
                if (tr.get())
                {
                    n = tr->name;
                }
                break;
            }
    }
    if (n != "")
        obj.push_back( Pair("param", n) );
}

void Goals::checkDone(CharData& cdata, int c_param)
{
    if (cdata.queryExtraData(char_data_type_normal, char_data_seven_goals_small_start + id) == 1)
    {
        return;
    }
    bool finish = false;
    switch (type)
    {
        case goals_type_equipt:
            {
                if (c_param != 0)
                {
                    finish = (c_param == param);
                }
                else
                {
                    finish = cdata.CheckHasEquipt(param);
                }
                break;
            }
        case goals_type_level:
            {
                finish = (cdata.m_level >= param);
                break;
            }
        case goals_type_daily_score:
            {
                int score = cdata.queryExtraData(char_data_type_daily, char_data_daily_task);
                finish = (score >= param);
                break;
            }
        case goals_type_maze:
            {
                finish = (c_param == param);
                break;
            }
        case goals_type_attack_power:
            {
                int attack = cdata.getAttack();
                finish = (attack >= param);
                break;
            }
        case goals_type_general:
            {
                if (c_param != 0)
                {
                    finish = (c_param == param);
                }
                else
                {
                    finish = cdata.CheckHasGeneral(param);
                }
                break;
            }
        case goals_type_guard:
            {
                boost::shared_ptr<char_goods> pcg = guardMgr::getInstance()->GetCharGoods(cdata.m_id);
                if (pcg.get())
                {
                    finish = (pcg->m_guardtime >= param);
                }
                break;
            }
        case goals_type_race:
            {
                boost::shared_ptr<CharRaceData> rd = RaceMgr::getInstance()->getRaceData(cdata.m_id);
                if (rd.get() && rd->getChar())
                {
                    finish = (rd->m_race_times >= param);
                }
            }
            break;
    }
    if (finish)
        cdata.setExtraData(char_data_type_normal, char_data_seven_goals_small_start + id, 1);
}

const json_spirit::Array& baseSevenGoals::getIArray() const
{
    return m_item_list;
}

void baseSevenGoals::updateObj()
{
    m_item_list.clear();
    for (std::list<Item>::iterator it_i = m_ilist.begin(); it_i != m_ilist.end(); ++it_i)
    {
        json_spirit::Object obj;
        it_i->toObj(obj);
        m_item_list.push_back(obj);
    }
}

seven_Goals_mgr::seven_Goals_mgr()
{
    load();
}

void seven_Goals_mgr::load()
{
    Query q(GetDb());
    //七日目标
    q.get_result("SELECT id,day,type,param FROM base_first_7_goals as l WHERE 1 order by l.day");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        int id = q.getval();
        int day = q.getval();
        baseSevenGoals* p_bsg = getBaseSevenGoals(day);
        if (!p_bsg)
        {
            p_bsg = new baseSevenGoals;
            p_bsg->m_day = day;
            m_base_seven_goals[day] = p_bsg;
        }
        Goals g;
        g.id = id;
        g.type = q.getval();
        g.param = q.getval();
        p_bsg->m_glist.push_back(g);
    }
    q.free_result();
    //七日奖励
    q.get_result("select l.day,l.itemType,l.itemId,l.count from base_first_7_golas_rewards as l where 1 order by l.day");
    while (q.fetch_row())
    {
        int day = q.getval();
        baseSevenGoals* p_bsg = getBaseSevenGoals(day);
        if (p_bsg != NULL)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            p_bsg->m_ilist.push_back(it);
        }
    }
    q.free_result();
    for (std::map<int, baseSevenGoals* >::iterator it = m_base_seven_goals.begin(); it != m_base_seven_goals.end(); ++it)
    {
        it->second->updateObj();
    }
}

baseSevenGoals* seven_Goals_mgr::getBaseSevenGoals(int day)
{
    return m_base_seven_goals[day];
}

void seven_Goals_mgr::updateGoals(CharData& cdata, int day, int type/*=0*/, int param/*=0*/)
{
    if (day < 1 || day > 7)
    {
        return;
    }
    for (int i = 1; i <= day; ++i)
    {
        baseSevenGoals* p_bsg = getBaseSevenGoals(i);
        if (p_bsg)
        {
            bool finish = true;
            for (std::list<Goals>::iterator it_g = p_bsg->m_glist.begin(); it_g != p_bsg->m_glist.end(); ++it_g)
            {
                if (type == 0 || type == it_g->type)
                {
                    it_g->checkDone(cdata, param);
                }
                if (cdata.queryExtraData(char_data_type_normal, char_data_seven_goals_small_start + it_g->id) == 0)
                {
                    finish = false;
                }
            }
            if (finish && cdata.queryExtraData(char_data_type_normal, char_data_seven_goals_start + i) == 0)
            {
                cdata.setExtraData(char_data_type_normal, char_data_seven_goals_start + i, 1);
            }
        }
    }    
}

int seven_Goals_mgr::quarGoalInfo(CharData& cdata, int day, json_spirit::Object& robj)
{
    int cur_day = cdata.queryCreateDays();
    if (!cdata.m_sevenOpen || checkActionFinish(cdata))
    {
        return HC_ERROR;
    }
    if (day == 0)
        day = cur_day;
    if (day < 1 || day > 7)
    {
        return HC_ERROR;
    }
    baseSevenGoals* p_bsg = getBaseSevenGoals(day);
    if (p_bsg)
    {
        json_spirit::Array goal_list;
        for (std::list<Goals>::iterator it_g = p_bsg->m_glist.begin(); it_g != p_bsg->m_glist.end(); ++it_g)
        {
            json_spirit::Object obj;
            it_g->toObj(obj);
            obj.push_back( Pair("state", cdata.queryExtraData(char_data_type_normal, char_data_seven_goals_small_start + it_g->id)) );
            goal_list.push_back(obj);
        }
        robj.push_back( Pair("goals_list", goal_list) );
        robj.push_back( Pair("reward_list", p_bsg->getIArray()) );
        robj.push_back( Pair("reward_state", cdata.queryExtraData(char_data_type_normal, char_data_seven_goals_start + day)) );
        robj.push_back( Pair("cur_day", cur_day) );
        time_t t_c = cdata.m_createTime;
        struct tm tm;
        struct tm *t = &tm;
        localtime_r(&t_c, t);
        if (t->tm_hour >= 0 && t->tm_hour < 24)
        {
            t->tm_hour = 0;
            t->tm_min = 0;
            t->tm_sec = 0;
        }
        if (day > cur_day)
        {
            time_t tmp_time = (int)(mktime(t)) + (day-1) * (24*3600);
            robj.push_back( Pair("start_time", (tmp_time - time(NULL))) );
        }
        else
        {
            time_t tmp_time = (int)(mktime(t)) + 7 * (24*3600);
            robj.push_back( Pair("end_time", (tmp_time - time(NULL))) );
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int seven_Goals_mgr::getReward(CharData& cdata, int day, json_spirit::Object& robj)
{
    if (checkActionFinish(cdata))
    {
        return HC_ERROR;
    }
    if (day < 1 || day > 7)
    {
        return HC_ERROR;
    }
    //玩家领取情况
    int state = cdata.queryExtraData(char_data_type_normal, char_data_seven_goals_start + day);
    if (state == 1)
    {
        baseSevenGoals* p_bsg = Singleton<seven_Goals_mgr>::Instance().getBaseSevenGoals(day);
        if (p_bsg)
        {
            std::list<Item> items = p_bsg->m_ilist;
            giveLoots(&cdata, items, cdata.m_area, cdata.m_level, 0, NULL, &robj, true, give_seven_goals);
            cdata.setExtraData(char_data_type_normal, char_data_seven_goals_start + day, 2);
            cdata.NotifyCharData();
            //还可以领奖励吗 - 不能了
            if (checkActionFinish(cdata))
            {
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata.m_name);
                if (account.get())
                {
                    account->Send("{\"type\":18,\"cmd\":\"removeAction\",\"s\":200}");
                }
            }
            else if (0 == getState(cdata))
            {
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata.m_name);
                if (account.get())
                {
                    account->Send("{\"type\":18,\"active\":0,\"cmd\":\"updateAction\",\"s\":200}");
                }
            }
            //act统计
            act_to_tencent(&cdata,act_new_7_goal_reward,day);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int seven_Goals_mgr::getState(CharData& cdata)
{
    int char_days = cdata.queryCreateDays();
    for (int i = 1; i <= char_days; ++i)
    {
        int state = cdata.queryExtraData(char_data_type_normal, char_data_seven_goals_start + i);
        if (state != 2)
        {
            return 1;
        }
    }
    return 0;
}

bool seven_Goals_mgr::canGetReward(CharData& cdata)
{
    bool can_get = false;
    int char_days = cdata.queryCreateDays();
    for (int i = 1; i <= 7; ++i)
    {
        int state = cdata.queryExtraData(char_data_type_normal, char_data_seven_goals_start + i);
        if (state == 1 && char_days >= i)
        {
            can_get = true;
            break;
        }
    }
    return can_get;
}

bool seven_Goals_mgr::checkActionFinish(CharData& cdata)
{
    bool goals_finish = true;
    int char_days = cdata.queryCreateDays();
    if (char_days < 1 || char_days > 7)
        return true;
    for (int i = 1; i <= 7; ++i)
    {
        int state = cdata.queryExtraData(char_data_type_normal, char_data_seven_goals_start + i);
        if (state != 2)
        {
            goals_finish = false;
            break;
        }
    }
    return goals_finish;
}

void seven_Goals_mgr::getAction(CharData* pc, json_spirit::Array& blist)
{
    if (pc && pc->m_sevenOpen && !checkActionFinish(*pc))
    {
        int state = getState(*pc);
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_seven_goals) );
        obj.push_back( Pair("active", state) );
        blist.push_back(obj);
    }
}

//查询目标信息 cmd ：querySevenGoals
int ProcessQuerySevenGoals(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int day = 0;
    READ_INT_FROM_MOBJ(day, o, "day");
    robj.push_back( Pair("day", day) );
    //第一闪
    int first_view = pc->queryExtraData(char_data_type_normal, char_data_view_seven);
    if (0 == first_view)
    {
        pc->setExtraData(char_data_type_normal, char_data_view_seven, 1);
        if (!Singleton<seven_Goals_mgr>::Instance().canGetReward(*pc))
        {
            pc->notifyEventState(top_level_event_daily, 0, 0);
        }
    }
    return Singleton<seven_Goals_mgr>::Instance().quarGoalInfo(*pc,day,robj);
}

//领取目标奖励 cmd ：getSevenGoals
int ProcessGetSevenGoals(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int day = 0;
    READ_INT_FROM_MOBJ(day, o, "day");
    robj.push_back( Pair("day", day) );
    return Singleton<seven_Goals_mgr>::Instance().getReward(*pc,day,robj);
}

