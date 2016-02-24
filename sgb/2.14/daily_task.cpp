
#include "daily_task.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "statistics.h"
#include "utils_lang.h"
#include "spls_race.h"
#include "groupCopy.h"
#include "guard.h"
#include "servant.h"
#include "singleton.h"
#include "first_seven_goals.h"

using namespace std;
using namespace net;
using namespace json_spirit;

class Combat;

extern volatile int iRaceFreeTimes;    //免费15次挑战

//粮饷相关次数
extern volatile int iGuardRobTimes;
extern volatile int iGuardTimes;
extern volatile int iGuardHelpTimes;

Database& GetDb();

extern void InsertSaveDb(const std::string& sql);

int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

static std::string g_notify_done = "{\"cmd\":\"dailyTaskState\",\"s\":200,\"done\":true}";

// 日常任务

dailyTaskMgr* dailyTaskMgr::m_handle = NULL;

dailyTaskMgr* dailyTaskMgr::getInstance()
{
    if (NULL == m_handle)
    {
        time_t time_start = time(NULL);
        cout<<"dailyTaskMgr::getInstance()..."<<endl;
        m_handle = new dailyTaskMgr();
        m_handle->reload();
        cout<<"dailyTaskMgr::dailyTaskMgr() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

void dailyTaskMgr::reload()
{
    Query q(GetDb());

    q.get_result("SELECT id,pos,can_findback,needtimes,score,memo FROM base_daily_task WHERE 1");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        boost::shared_ptr<base_daily_task> pbd;
        pbd.reset(new base_daily_task);
        pbd->task_id = q.getval();
        pbd->task_pos = q.getval();
        pbd->can_findback = q.getval();
        pbd->needtimes = q.getval();
        pbd->score = q.getval();
        pbd->memo = q.getstr();
        m_base_daily_task_list[pbd->task_id] = pbd;
    }
    q.free_result();
    
    q.get_result("SELECT id,needscore,needlevel,supply,silver,gongxun,prestige FROM base_daily_task_reward WHERE 1 order by id asc");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        int id = 0, supply = 0, silver = 0, gongxun = 0, prestige = 0;
        id = q.getval();
        char_daily_task_rewards tmp;
        tmp._needscore = q.getval();
        tmp._needlevel = q.getval();
        supply = q.getval();
        silver = q.getval();
        gongxun = q.getval();
        prestige = q.getval();
        //军粮奖励
        if (supply > 0)
        {
            Item item;
            item.type = item_type_treasure;
            item.id = treasure_type_supply;
            item.nums = supply;
            tmp._rewards.push_back(item);
        }
        //银币奖励
        if (silver > 0)
        {
            Item item;
            item.type = item_type_silver;
            item.id = 0;
            item.nums = silver;
            tmp._rewards.push_back(item);
        }
        //功勋奖励
        if (gongxun > 0)
        {
            Item item;
            item.type = item_type_treasure;
            item.id = treasure_type_gongxun;
            item.nums = gongxun;
            tmp._rewards.push_back(item);
        }
        //声望奖励
        if (prestige > 0)
        {
            Item item;
            item.type = item_type_prestige;
            item.id = 0;
            item.nums = prestige;
            tmp._rewards.push_back(item);
        }
        m_daily_task_rewards.push_back(tmp);
        iMaxDailyTaskReward = id;
    }
    q.free_result();
}

boost::shared_ptr<base_daily_task> dailyTaskMgr::getDaily_task(int id)
{
    if (m_base_daily_task_list.find(id) != m_base_daily_task_list.end())
    {
        return m_base_daily_task_list[id];
    }
    boost::shared_ptr<base_daily_task> pbd;
    pbd.reset();
    return pbd;
}

//更新日常任務
void dailyTaskMgr::updateDailyTask(CharData& cdata, int task_id, int times)
{
    int reward_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_task_reward);
    if (reward_times >=0 && reward_times < iMaxDailyTaskReward)
    {
        boost::shared_ptr<base_daily_task> pbd = getDaily_task(task_id);
        if (!pbd.get())
        {
            return;
        }
        //增加任务完成次数
        int task_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_task_start + task_id);
        if ((task_times + times) > pbd->needtimes)
        {
            times = pbd->needtimes - task_times;
        }
        task_times += times;
        cdata.setExtraData(char_data_type_daily, char_data_daily_task_start + task_id, task_times);
        //任务完成次数达标则加活跃度
        //if (pbd.get() && task_times == pbd->needtimes)
        {
            int score = cdata.queryExtraData(char_data_type_daily, char_data_daily_task);
            int add_score = pbd->score * times;
            score += add_score;
            cdata.setExtraData(char_data_type_daily, char_data_daily_task, score);

            //支线任务
            cdata.m_trunk_tasks.updateTask(task_daily_score, score);
            //可领取奖励
            if (canGetReward(cdata))
            {
                //通知客户端任务完成，可以领取了
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata.m_name);
                if (account.get())
                {
                    account->Send(g_notify_done);
                    account->Send("{\"type\":1,\"active\":1,\"cmd\":\"updateAction\",\"s\":200}");
                }
            }
            if (cdata.m_helperOpen && add_score > 0)
            {
                //白字显示活跃度改变
                int need_more = 0;
                std::string msg = strDailyScore;
                std::string tmp = "";
                str_replace(msg, "$S", LEX_CAST_STR(score));
                for (int i = 1; i <= iMaxDailyTaskReward; ++i)
                {
                    if (score < m_daily_task_rewards[i-1]._needscore && cdata.m_level >= m_daily_task_rewards[i-1]._needlevel)
                    {
                        need_more = m_daily_task_rewards[i-1]._needscore - score;
                        break;
                    }
                }
                if (need_more != 0 && need_more < 16)
                {
                    tmp = strDailyScore2;
                    str_replace(tmp, "$S", LEX_CAST_STR(need_more));
                }
                str_replace(msg, "$M", tmp);
                GeneralDataMgr::getInstance()->sendNotifyMsg(cdata.m_name,msg);
            }
        }
        //七日目标
        Singleton<seven_Goals_mgr>::Instance().updateGoals(cdata,cdata.queryCreateDays(),goals_type_daily_score);
    }
    return;
}

void dailyTaskMgr::toObj(CharData& cdata, json_spirit::Object& info)
{
    //玩家日常活跃度
    int score = cdata.queryExtraData(char_data_type_daily, char_data_daily_task);
    int reward_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_task_reward);
    #if 0
    if (reward_times >=0 && reward_times < iMaxDailyTaskReward)
    {
        if (score >= m_daily_task_rewards[reward_times]._needscore && cdata.m_level >= m_daily_task_rewards[reward_times]._needlevel)
        {
            cdata._checkGuide(guide_id_get_daily);
        }
    }
    #endif
    int total = 0;
    for (int i = 1; i <= iMaxDailyTaskReward; ++i)
    {
        if (cdata.m_level < m_daily_task_rewards[i-1]._needlevel)
        {
            break;
        }
        if (0 == cdata.queryExtraData(char_data_type_daily, char_data_daily_task_reward_start + i))
        {
            total = m_daily_task_rewards[i - 1]._needscore;
            break;
        }
    }
    if (total == 0)
        total = m_daily_task_rewards[iMaxDailyTaskReward - 1]._needscore;
    info.push_back( Pair("total", total) );
    info.push_back( Pair("current", score) );
    //各奖励具体信息
    json_spirit::Array list;
    for (int i = 1; i <= iMaxDailyTaskReward; ++i)
    {
        json_spirit::Object reward;
        int state = cdata.queryExtraData(char_data_type_daily, char_data_daily_task_reward_start + i);
        reward.push_back( Pair("state", state) );
        getnewReward(cdata, i, reward);
        list.push_back(reward);
    }
    //任务奖励
    info.push_back( Pair("award_list", list) );
}

bool dailyTaskMgr::canGetReward(CharData& cdata)
{
    bool can_get = false;
    int score = cdata.queryExtraData(char_data_type_daily, char_data_daily_task);
    for (int i = 1; i <= iMaxDailyTaskReward; ++i)
    {
        int state = cdata.queryExtraData(char_data_type_daily, char_data_daily_task_reward_start + i);
        if (state == 0 && score >= m_daily_task_rewards[i-1]._needscore && cdata.m_level >= m_daily_task_rewards[i-1]._needlevel)
        {
            can_get = true;
            break;
        }
    }
    return can_get;
}

void dailyTaskMgr::getnewReward(CharData& cdata, int reward_id, json_spirit::Object& info)
{
    info.push_back( Pair("need_score", m_daily_task_rewards[reward_id-1]._needscore) );
    info.push_back( Pair("need_level", m_daily_task_rewards[reward_id-1]._needlevel) );
    std::string memo = m_daily_task_rewards[reward_id-1].toString(cdata.m_level);
    info.push_back( Pair("reward_memo", memo) );
    return;
}

int dailyTaskMgr::_newrewardTask(CharData& cdata, int reward_id, json_spirit::Object& robj)
{
    if (reward_id < 1 || reward_id > iMaxDailyTaskReward)
    {
        return HC_ERROR;
    }
    //玩家领取情况
    int reward_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_task_reward_start + reward_id);
    if (reward_times == 0)
    {
        //玩家日常活跃度
        int score = cdata.queryExtraData(char_data_type_daily, char_data_daily_task);
        if (score < m_daily_task_rewards[reward_id-1]._needscore || cdata.m_level < m_daily_task_rewards[reward_id-1]._needlevel)
        {
            return HC_ERROR;
        }
        cdata.setExtraData(char_data_type_daily, char_data_daily_task_reward_start + reward_id, 1);
        std::list<Item> items = m_daily_task_rewards[reward_id-1]._rewards;
        if (giveLoots(&cdata, items, cdata.m_area, cdata.m_level, 0, NULL, NULL, true, give_daily_task))
        {
            std::list<Item>::iterator it = m_daily_task_rewards[reward_id-1]._rewards.begin();
            while (it != m_daily_task_rewards[reward_id-1]._rewards.end())
            {
                if (it->type == item_type_treasure && it->id == treasure_type_supply)
                {
                    robj.push_back( Pair("supply", it->nums) );
                }
                if (it->type == item_type_treasure && it->id == treasure_type_gongxun)
                {
                    robj.push_back( Pair("gongxun", it->nums) );
                }
                if (it->type == item_type_silver)
                {
                    robj.push_back( Pair("silver", it->nums) );
                }
                if (it->type == item_type_prestige)
                {
                    robj.push_back( Pair("prestige", it->nums) );
                }
                ++it;
            }
        }
        cdata.NotifyCharData();
        //还可以领奖励吗 - 不能了
        if (!canGetReward(cdata))
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata.m_name);
            if (account.get())
            {
                account->Send("{\"type\":1,\"active\":0,\"cmd\":\"updateAction\",\"s\":200}");
            }
        }
    }
    else
    {
        return HC_ERROR;
    }
    //act统计
    act_to_tencent(&cdata,act_new_helplist_reward,m_daily_task_rewards[reward_id-1]._needscore);
    return HC_SUCCESS;
}

int dailyTaskMgr::findBack(boost::shared_ptr<CharData> cdata, int id)
{
    if (cdata->addGold(-10) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //金币消耗统计
    add_statistics_of_gold_cost(cdata->m_id, cdata->m_ip_address, 10, gold_cost_for_findback, cdata->m_union_id, cdata->m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(cdata.get(),10,gold_cost_for_findback);
#endif
    bool success = false;
    switch(id)
    {
        case daily_task_rest:
            {
                if (cdata->m_buyLingOpen)//购买军令
                {
                    cdata->m_gold_rest = 0;
                    cdata->saveCharDailyVar();
                    success = true;
                }
            }
            break;
        case daily_task_race:
            {
                if (cdata->m_raceOpen)//竞技
                {
                    boost::shared_ptr<CharRaceData> rd = RaceMgr::getInstance()->getRaceData(cdata->m_id);
                    if (rd.get() && rd->getChar())
                    {
                        if (rd->m_race_times > iRaceFreeTimes)
                            rd->m_race_times -= iRaceFreeTimes;
                        else
                            rd->m_race_times = 0;
                        InsertSaveDb("update `char_race` set raceTimes=0 where cid=" + LEX_CAST_STR(cdata->m_id));
                        success = true;
                    }
                }
            }
            break;
        case daily_task_levy:
            {
                if (cdata->m_levyOpen)//征收
                {
                    cdata->m_levy_time = 0;
                    cdata->saveCharDailyVar();
                    success = true;
                }
            }
            break;
        case daily_task_group_copy:
            {
                if (cdata->isMapPassed(1))//多人副本
                {
                    groupCopyMgr::getInstance()->reset(cdata->m_id);
                    success = true;
                }
            }
            break;
        case daily_task_horse:
            {
                if (cdata->m_horseOpen)//战马
                {
                    cdata->m_silver_train_horse = 0;
                    cdata->m_gold_train_horse = 0;
                    cdata->setExtraData(char_data_type_daily, char_data_horse_silver_train, cdata->m_silver_train_horse);
                    cdata->setExtraData(char_data_type_daily, char_data_horse_gold_train, cdata->m_gold_train_horse);
                    success = true;
                }
            }
            break;
        case daily_task_farm:
            {
                if (cdata->m_farmOpen)//屯田
                {
                    cdata->setExtraData(char_data_type_daily, char_data_farm_seed, 0);
                    success = true;
                }
            }
            break;
        case daily_task_trade:
            {
                if (cdata->m_tradeOpen)//通商
                {
                    cdata->setExtraData(char_data_type_daily, char_data_trade_time, 0);
                    success = true;
                }
            }
            break;
        case daily_task_corp_jisi:
            {
                if (cdata->m_corps_member.get())//军团相关
                {
                    cdata->m_temp_jisi_times = 0;
                    cdata->saveCharDailyVar();
                    success = true;
                }
            }
            break;
        case daily_task_guard:
            {
                if (cdata->m_guardOpen)//护送
                {
                    boost::shared_ptr<char_goods> pcg = guardMgr::getInstance()->GetCharGoods(cdata->m_id);
                    if (pcg.get())
                    {
                        pcg->m_cooltime = 0;
                        pcg->m_helper_info = 0;
                        pcg->m_robtime = 0;
                        pcg->m_guardtime = 0;
                        pcg->m_helptime = 0;
                        InsertSaveDb("update char_guard_goods set cooltime=0,robtime="+LEX_CAST_STR(iGuardRobTimes)+",guardtime="+LEX_CAST_STR(iGuardTimes)+",helptime="+LEX_CAST_STR(iGuardHelpTimes)+" where cid="+ LEX_CAST_STR(cdata->m_id));
                        success = true;
                        boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
                        if (pchar.get())
                        {
                            pchar->Send("{\"type\":6,\"cmd\":\"updateAction\",\"s\":200,\"leftNums\":" + LEX_CAST_STR(pcg->getCanGuardtime()) + "}");
                        }
                    }
                }
            }
            break;
        case daily_task_servant_catch:
            {
                if (cdata->m_servantOpen)//家丁
                {
                    boost::shared_ptr<charServant> p = servantMgr::getInstance()->GetCharServant(cdata->m_id);
                    if (p.get())
                    {
                        p->m_catch_time = 0;
                        p->m_buy_catch_time = 0;
                        p->m_interact_time = 0;
                        p->m_rescue_time = 0;
                        p->m_sos_time = 0;
                        p->m_be_sos_time_f = 0;
                        p->m_be_sos_time_c = 0;
                        p->m_resist_time = 0;
                        p->m_buy_resist_time = 0;
                        p->m_get_num = 0;
                        p->m_interact_cooltime = 0;
                        p->m_rescue_list.clear();
                        servantMgr::getInstance()->Save_rescue_list(cdata->m_id);
                        servantMgr::getInstance()->Save_data(cdata->m_id);
                        success = true;
                    }
                }
            }
            break;
    }
    if (success)
    {
        cdata->setExtraData(char_data_type_daily, char_data_daily_findback_task_start + id, 1);
        cdata->NotifyCharData();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

bool base_daily_task::to_obj(boost::shared_ptr<CharData> cdata, json_spirit::Object& robj)
{
    int task_times = cdata->queryExtraData(char_data_type_daily, char_data_daily_task_start + task_id);
    robj.push_back( Pair("id", task_id));
    robj.push_back( Pair("pos", task_pos));
    robj.push_back( Pair("counts", task_times));
    //特殊处理的次数
    int need = needtimes;
    #if 0
    if (need >= 10000)
    {
        if (task_id == daily_task_rest)
        {
            need = iVIPRestTimes[cdata->m_vip];
        }
        else if (task_id == daily_task_levy)
        {
            need = iLevyTimes[cdata->m_vip]+iLevyFreeTime;
        }
    }
    #endif
    robj.push_back( Pair("need", need));
    robj.push_back( Pair("score", score));
    robj.push_back( Pair("msg", memo));
    if (task_times >= need && can_findback && cdata->canFindBack())
    {
        robj.push_back( Pair("can_findback", cdata->queryExtraData(char_data_type_daily, char_data_daily_findback_task_start + task_id) == 0));
    }
    return task_times >= need;
}

std::string char_daily_task_rewards::toString(int level)
{
    std::string str_rewards = "";
    for (std::list<Item>::iterator it = _rewards.begin(); it != _rewards.end(); ++it)
    {
        if (str_rewards != "")
        {
            str_rewards += "\n";
        }
        str_rewards += it->toString(false, level);
    }
    return str_rewards;
}

//日常任务奖励领取
int ProcessRewardDailyTask(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    return dailyTaskMgr::getInstance()->_newrewardTask(*(cdata.get()), id, robj);
}

