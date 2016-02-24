
#include "guard.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include "utils_lang.h"
#include "spls_timer.h"
#include "spls_errcode.h"
#include <syslog.h>
#include "statistics.h"
#include "daily_task.h"
#include "singleton.h"
#include "relation.h"
#include "first_seven_goals.h"

#define INFO(x) //cout<<x

extern std::string strGuardRobLog;
extern std::string strGuardMailTitle;
extern std::string strGuardMailContent;
extern std::string strGuardRobSucMailContent;
extern std::string strGuardRobFailMailContent;
extern std::string strGuardNotOpen;
extern std::string strGuardRed;
extern std::string strGuardPurple;
extern std::string strGuardHelpMailTitle;
extern std::string strGuardHelpMailContent;
extern std::string strGuardRobReward;
extern std::string strGuardRobRank;

//请求护送回应提示
const std::string strGuardHelp = "{\"type\":$T,\"cmd\":\"guardHelpAnswer\",\"s\":200}";

extern std::string strGuardEventOpen;

//护送功能次数
static const int iDefaultGuardRobTimes = 4;
static const int iDefaultGuardTimes = 3;
static const int iDefaultGuardHelpTimes = 2;

volatile int iGuardRobTimes = iDefaultGuardRobTimes;
volatile int iGuardTimes = iDefaultGuardTimes;
volatile int iGuardHelpTimes = iDefaultGuardHelpTimes;

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
extern void InsertCombat(Combat* pcombat);
extern void InsertSaveCombat(Combat* pcombat);

//初始化生辰纲战斗
extern Combat* createGuardCombat(int cid, int tid, int true_guard_cid, int& ret);

bool compare_score(robScore& a, robScore& b)
{
    return a.score > b.score;
}

int goods::start()
{
    //重启后的情况
    int leftsecond = m_end_time - time(NULL);

    json_spirit::mObject mobj;
    mobj["cmd"] = "guardDone";
    mobj["cid"] = m_cid;
    boost::shared_ptr<splsTimer> tmsg;
    if (leftsecond <= 0)
    {
        //直接完成了
        tmsg.reset(new splsTimer(1, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    else
    {
        tmsg.reset(new splsTimer(leftsecond, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return 0;
}

int goods::finish()
{
    if (m_end_time >= time(NULL))
    {
        splsTimerMgr::getInstance()->delTimer(_uuid);
        guardMgr::getInstance()->GuardDone(m_cid);
        return 0;
    }
    return -1;
}

int char_goods::getCanRobtime()
{
    int left = iGuardRobTimes - m_robtime;
    if (left >= 0)
    {
        return left;
    }
    else
    {
        return 0;
    }
}

int char_goods::getCanHelptime()
{
    int left = iGuardHelpTimes - m_helptime;
    if (left >= 0)
    {
        return left;
    }
    else
    {
        return 0;
    }
}

int char_goods::getCanGuardtime()
{
    int left = iGuardTimes - m_guardtime;
    if (left >= 0)
    {
        return left;
    }
    else
    {
        return 0;
    }
}

int guardMgr::reload()
{
    Query q(GetDb());
    //基础排名奖
    q.get_result("select rank,prestige,treasure_id,treasure_num,memo from base_rob_rankings_rewards where 1 order by rank");
    while (q.fetch_row())
    {
        robRankRewards rrr;
        rrr.rank = q.getval();
        rrr.prestige = q.getval();
        rrr.treasure_id = q.getval();
        rrr.treasure_num = q.getval();
        rrr.memo = q.getstr();
        m_base_rewards.push_back(rrr);
    }
    q.free_result();
    //玩家积分
    int rank = 0;
    m_topten_min_score = 0;
    q.get_result("select cid,score from char_guard_rankscore where 1 order by score desc");
    while (q.fetch_row())
    {
        int cid = q.getval();
        int score = q.getval();
        m_score_maps[cid] = score;
        ++rank;
        if (rank <= 10)
        {
            robScore rs;
            rs.cid = cid;
            rs.score = score;
            m_topten_score.push_back(rs);
            m_topten_min_score = score;
        }
    }
    q.free_result();
    //上周领取情况
    q.get_result("select rank,cid,state from char_guard_rankget where 1 order by rank");
    while (q.fetch_row())
    {
        last_week_Rewards tmp;
        tmp.rank = q.getval();
        tmp.cid = q.getval();
        tmp.state = q.getval();
        m_last_rewards.push_back(tmp);
    }
    q.free_result();
    q.get_result("SELECT id,name,need_min,silver,prestige,supply FROM base_guard_goods WHERE 1");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        boost::shared_ptr<base_goods> pbg;
        pbg.reset(new base_goods);
        int id = q.getval();
        pbg->name = q.getstr();
        pbg->need_min = q.getval();
        pbg->silver = q.getval();
        pbg->prestige = q.getval();
        pbg->supply = q.getval();

        pbg->color_name = pbg->name;
        addColor(pbg->color_name, id);
        m_base_goods[id] = pbg;
    }
    q.free_result();
    //玩家纲队
    q.get_result("select cid, gid, cooltime, robtime, guardtime, helptime, state, helper_cid, reward_more, start_time, end_time, can_be_rob_time, silver, prestige,supply from char_guard_goods where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<char_goods> pcg;
        pcg.reset(new char_goods);
        int cid = q.getval();
        pcg->m_cid = cid;
        pcg->m_gid = q.getval();
        pcg->m_cooltime = q.getval();
        pcg->m_robtime = q.getval();
        pcg->m_guardtime = q.getval();
        pcg->m_helptime = q.getval();
        pcg->m_state = q.getval();
        pcg->m_helper_info = q.getval();
        int reward_more = q.getval();
        pcg->m_helpwaittime = 0;
        time_t start_time = q.getval();
        time_t end_time = q.getval();
        int can_rob_time = q.getval();
        int silver = q.getval();
        int prestige = q.getval();
        int supply = q.getval();
        boost::shared_ptr<goods> pg;
        pg.reset();
        if (pcg->m_state == 1)
        {
            pg.reset(new goods(cid,pcg->m_gid));
            pg->m_rob_time = can_rob_time;
            pg->m_start_time = start_time;
            pg->m_end_time = end_time;
            pg->m_silver = silver;
            pg->m_prestige = prestige;
            pg->m_reward_more = reward_more;
            pg->m_supply = supply;
            pg->m_needmin = (end_time - start_time) / 60;
            pg->m_help_cid = pcg->m_helper_info;
            pcg->m_guard_goods = pg;
            pcg->m_guard_goods->start();
        }
        m_char_goods[cid] = pcg;
    }
    q.free_result();

    m_guard_reward_more = GeneralDataMgr::getInstance()->getInt("guard_reward", 100);
    m_guard_reward_end_time = GeneralDataMgr::getInstance()->getInt("guard_reward_end", 0);
    if (m_guard_reward_end_time <= time(NULL))
    {
        m_guard_reward_more = 100;
    }
    iGuardRobTimes = GeneralDataMgr::getInstance()->getInt("guard_rob_times", iDefaultGuardRobTimes);
    iGuardTimes = GeneralDataMgr::getInstance()->getInt("guard_times", iDefaultGuardTimes);
    iGuardHelpTimes = GeneralDataMgr::getInstance()->getInt("guard_help_times", iDefaultGuardHelpTimes);
    return 0;
}

int guardMgr::save(int cid)
{
    INFO("guardMgr::save!!!!cid=" << cid << endl);
    std::map<int, boost::shared_ptr<char_goods> >::iterator it = m_char_goods.find(cid);
    if (it != m_char_goods.end())
    {
        if (it->second.get())
        {
            time_t start_time = 0, end_time = 0;
            int can_rob_time = 0, silver = 0, prestige = 0, supply = 0, reward_more = 0;
            if (it->second->m_state == 1 && it->second->m_guard_goods.get())
            {
                start_time = it->second->m_guard_goods->m_start_time;
                end_time = it->second->m_guard_goods->m_end_time;
                can_rob_time = it->second->m_guard_goods->m_rob_time;
                silver = it->second->m_guard_goods->m_silver;
                prestige = it->second->m_guard_goods->m_prestige;
                supply = it->second->m_guard_goods->m_supply;
                reward_more = it->second->m_guard_goods->m_reward_more;
            }
            InsertSaveDb("update char_guard_goods set gid=" + LEX_CAST_STR(it->second->m_gid)
                + ",cooltime=" + LEX_CAST_STR(it->second->m_cooltime)
                + ",robtime=" + LEX_CAST_STR(it->second->m_robtime)
                + ",guardtime=" + LEX_CAST_STR(it->second->m_guardtime)
                + ",helptime=" + LEX_CAST_STR(it->second->m_helptime)
                + ",state=" + LEX_CAST_STR(it->second->m_state)
                + ",helper_cid=" + LEX_CAST_STR(it->second->m_helper_info)
                + ",start_time=" + LEX_CAST_STR(start_time)
                + ",reward_more=" + LEX_CAST_STR(reward_more) 
                + ",end_time=" + LEX_CAST_STR(end_time)
                + ",can_be_rob_time=" + LEX_CAST_STR(can_rob_time)
                + ",silver=" + LEX_CAST_STR(silver)
                + ",prestige=" + LEX_CAST_STR(prestige)
                + ",supply=" + LEX_CAST_STR(supply)
                + " where cid=" + LEX_CAST_STR(cid));
        }
        else
        {
            cout << "cannot get char_goods!!!!cid=" << cid << endl;
        }
    }
    return 0;
}

guardMgr* guardMgr::m_handle = NULL;

guardMgr* guardMgr::getInstance()
{
    if (NULL == m_handle)
    {
        m_handle = new guardMgr();
        m_handle->reload();
    }
    return m_handle;
}

int guardMgr::getinsence(int cid)
{
    m_uid_list[cid] = 1;
    return 0;
}

int guardMgr::getoutsence(int cid)
{
    m_uid_list.erase(cid);
    return 0;
}

int guardMgr::getRobScoreList(int cid, json_spirit::Object& robj)
{
    cout << "getRobScoreList is call!!!" << endl;
    json_spirit::Array dlist;
    int rank = 0;
    for (std::list<robScore>::iterator it = m_topten_score.begin(); it != m_topten_score.end(); ++it)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("rank", ++rank) );
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(it->cid);
        if (!cdata.get())
        {
            continue;
        }
        obj.push_back( Pair("name", cdata->m_name) );
        obj.push_back( Pair("level", cdata->m_level) );
        if (cdata->m_corps_member.get())
        {
            splsCorps* cp = corpsMgr::getInstance()->findCorps(cdata->m_corps_member->corps);
            if (cp)
            {
                obj.push_back( Pair("corp", cp->_name) );
            }
        }
        obj.push_back( Pair("score", it->score) );
        cout << "push rank=" << rank << ",score=" << it->score << endl;
        if (it->cid == cid)
        {
            obj.push_back( Pair("self", 1) );
        }
        dlist.push_back(obj);
        if (rank >= 10)
        {
            break;
        }
    }
    robj.push_back( Pair("score", m_score_maps[cid]) );
    robj.push_back( Pair("can_get", getRewardState(cid, rank)) );
    cout << "self score=" << m_score_maps[cid] << endl;
    robj.push_back( Pair("list", dlist) );
    return HC_SUCCESS;
}

int guardMgr::getRobRankRewardsList(json_spirit::Object& robj)
{
    json_spirit::Array list;
    for (int rank = 0; rank < m_base_rewards.size(); ++rank)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("memo", m_base_rewards[rank].memo) );
        obj.push_back( Pair("rank", rank + 1) );
        list.push_back(obj);
        if (rank >= 10)
        {
            break;
        }
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

bool guardMgr::getRewardState(int cid, int& crank)
{
    for (int rank = 0; rank < m_last_rewards.size(); ++rank)
    {
        if (m_last_rewards[rank].cid == cid && m_last_rewards[rank].state == 0)
        {
            crank = rank+1;
            return true;
        }
    }
    return false;
}

int guardMgr::getRobRankRewards(int cid, json_spirit::Object& robj)
{
    int rank = 0;
    if (getRewardState(cid,rank) && rank > 0)
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cdata.get())
        {
            return HC_ERROR;
        }
        std::string strGet = "";
        if (m_base_rewards[rank-1].prestige)
        {
            cdata->addPrestige(m_base_rewards[rank-1].prestige);
            strGet += (strPrestige + strCounts + LEX_CAST_STR(m_base_rewards[rank-1].prestige) + " ");
        }
        if (m_base_rewards[rank-1].treasure_id > 0 && m_base_rewards[rank-1].treasure_num > 0)
        {
            cdata->addTreasure(m_base_rewards[rank-1].treasure_id, m_base_rewards[rank-1].treasure_num);
            boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(m_base_rewards[rank-1].treasure_id);
            if (tr.get())
            {
                strGet += tr->name + strCounts + LEX_CAST_STR(m_base_rewards[rank-1].treasure_num);
            }
        }
        m_last_rewards[rank-1].state = 1;
        if (strGet != "")
        {
            robj.push_back( Pair("msg", strGet) );
        }
        InsertSaveDb("update char_guard_rankget set state = 1 where rank = " + LEX_CAST_STR(rank) + " and cid = " + LEX_CAST_STR(cid));
        return HC_SUCCESS;
    }
    return HC_ERROR;
    
}

//每周日结算奖励
int guardMgr::raceAwards()
{
    //生成本周奖励
    m_last_rewards.clear();
    InsertSaveDb("TRUNCATE TABLE char_guard_rankget");
    int rank = 0;
    //系统公告
    std::string msg = strGuardRobReward + "\n";
    for (std::list<robScore>::iterator it = m_topten_score.begin(); it != m_topten_score.end(); ++it)
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(it->cid);
        if (!cdata.get())
        {
            continue;
        }
        last_week_Rewards tmp;
        tmp.rank = (++rank);
        tmp.cid = it->cid;
        tmp.state = 0;
        m_last_rewards.push_back(tmp);
        std::string str_rank = strGuardRobRank;
        str_replace(str_rank, "$N", LEX_CAST_STR(rank), true);
        str_replace(str_rank, "$W", MakeCharNameLink(cdata->m_name), true);
        if (rank < m_base_rewards.size())
        {
            str_replace(str_rank, "$R", m_base_rewards[rank-1].memo, true);
        }
        InsertSaveDb("replace into char_guard_rankget (rank,cid,state) values (" + LEX_CAST_STR(tmp.rank) + "," + LEX_CAST_STR(tmp.cid) + ",0)");
        if (rank >= 10)
        {
            break;
        }
        msg += (str_rank + "\n");
    }
    if (m_topten_score.size() > 0)
    {
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }
    m_topten_score.clear();
    m_score_maps.clear();
    InsertSaveDb("TRUNCATE TABLE char_guard_rankscore");
    return HC_SUCCESS;
}

//活动是否开启
void guardMgr::getAction(CharData* pc, json_spirit::Array& blist)
{
    if (pc->m_guardOpen)
    {
        boost::shared_ptr<char_goods> pcg = guardMgr::getInstance()->GetCharGoods(pc->m_id);
        if (pcg.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("type", top_level_event_guard) );
            obj.push_back( Pair("active", 0) );
            obj.push_back( Pair("leftNums", pcg->getCanGuardtime()));
            blist.push_back(obj);
        }
    }
}

int guardMgr::getActionMemo(std::string& memo)
{
    memo = strGuardNotOpen;
    return HC_SUCCESS;
}

int guardMgr::Start(int m_level, int cid)
{
    boost::shared_ptr<char_goods> pcg = GetCharGoods(cid);
    if (pcg.get() && pcg->m_state == 0 && pcg->getCanGuardtime() > 0)
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cdata.get())
        {
            return HC_ERROR;
        }
        //有护送
        int helper_id = 0;
        if (pcg->m_helper_info > 0)
        {
            boost::shared_ptr<char_goods> pcg_helper = GetCharGoods(pcg->m_helper_info);
            if (pcg_helper.get() && pcg_helper->getCanHelptime() > 0)
            {
                helper_id = pcg->m_helper_info;
                ++pcg_helper->m_helptime;
            }
            #if 0
            boost::shared_ptr<CharData> helper_cdata = GeneralDataMgr::getInstance()->GetCharData(pcg->m_helper_info);
            if (helper_cdata.get())
            {
                //日常任务
                dailyTaskMgr::getInstance()->updateDailyTask(*(helper_cdata.get()), daily_task_guard_help);
            }
            #endif
        }
        //取消护送请求
        else if (pcg->m_helper_info == -1)
        {
            pcg->m_helper_info = 0;
        }
        //VIP5及以上，从蓝色品质开始送
        if (cdata->m_vip >= 5 && pcg->m_gid < 2)
        {
            pcg->m_gid = 2;
        }
        boost::shared_ptr<goods> pg;
        pg.reset(new goods(pcg->m_cid, pcg->m_gid));
        boost::shared_ptr<base_goods> pbg = GetBaseGoods(pcg->m_gid);
        if (pbg.get())
        {
            pg->m_silver = pbg->silver * m_level;
            pg->m_prestige = pbg->prestige;
            pg->m_supply = pbg->supply;
            pg->m_needmin = pbg->need_min;
            pg->m_end_time = pg->m_start_time + pbg->need_min * 60;
            pg->m_help_cid = helper_id;
        }
        pcg->m_guard_goods = pg;
        pcg->m_guard_goods->m_reward_more = getGuardEvent();
        if (pcg->m_guard_goods->m_reward_more > 100)
        {
            pg->m_silver = pg->m_silver * pcg->m_guard_goods->m_reward_more/100;
            pg->m_prestige = pg->m_prestige * pcg->m_guard_goods->m_reward_more/100;
            pg->m_supply = pg->m_supply * pcg->m_guard_goods->m_reward_more/100;
        }
        
        pcg->m_guard_goods->start();
        ++(pcg->m_guardtime);
        pcg->m_state = 1;
        save(cid);
        //日常任务
        dailyTaskMgr::getInstance()->updateDailyTask(*(cdata.get()), daily_task_guard);
        //系统公告
        std::string notify_msg = "";
        if (pg->m_gid == 4)
        {
            notify_msg = strGuardRed;
            str_replace(notify_msg, "$N", MakeCharNameLink(cdata->m_name));
        }
        else if(pg->m_gid == 5)
        {
            notify_msg = strGuardPurple;
            str_replace(notify_msg, "$N", MakeCharNameLink(cdata->m_name));
            
        }
        if (notify_msg != "")
        {
            GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
        }
        //广播给当前界面的玩家
        json_spirit::Object obj;
        obj.push_back( Pair ("cmd", "addGuardQueue"));
        json_spirit::Array list;
        json_spirit::Object o;
        o.push_back( Pair ("id", pcg->m_cid));
        o.push_back( Pair ("name", cdata->m_name));
        if (pbg.get())
        {
            o.push_back( Pair ("guardLeftTime", pcg->m_guard_goods->m_end_time - time(NULL)));
            json_spirit::Object goodsVO;
            goodsVO.push_back( Pair("quality", pcg->m_gid) );
            goodsVO.push_back( Pair("totalTime", pbg->need_min) );
            o.push_back( Pair("goodsVO", goodsVO) );
        }
        list.push_back(o);
        obj.push_back( Pair ("list", list));
        obj.push_back( Pair ("s", 200));
        INFO("broadcast start new guard!!!!" << endl);
        std::map<uint64_t,int>::iterator it = m_uid_list.begin();
        while (it != m_uid_list.end())
        {
            boost::shared_ptr<CharData> cd = GeneralDataMgr::getInstance()->GetCharData(it->first);
            if (cd.get())
            {
                INFO("send to cid=" << it->first << endl);
                boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(cd->m_name);
                if (pchar.get())
                {
                    std::string final_result = json_spirit::write(obj, json_spirit::raw_utf8);
                    pchar->Send(final_result);
                }
            }
            ++it;
        }
        //更新 护送 按钮次数
        boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
        if (pchar.get())
        {
            pchar->Send("{\"type\":6,\"cmd\":\"updateAction\",\"s\":200,\"leftNums\":" + LEX_CAST_STR(pcg->getCanGuardtime()) + "}");
        }

        //开始护送，发给仇视我的人
        Singleton<relationMgr>::Instance().postEnemyInfos(cdata->m_id,
                                            cdata->m_name, 0, "", ENEMY_NEWS_ENEMY_GUARD);
        
        //七日目标
        Singleton<seven_Goals_mgr>::Instance().updateGoals(*(cdata.get()),cdata->queryCreateDays(),goals_type_guard);
        
        //act统计
        act_to_tencent(cdata.get(),act_new_guard);

        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int guardMgr::Finish(int cid)
{
    boost::shared_ptr<char_goods> pcg = GetCharGoods(cid);
    if (pcg.get() && pcg->m_state == 1 && pcg->m_guard_goods.get())
    {
        boost::shared_ptr<CharData> cd = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cd.get())
            return HC_ERROR;
        if (cd->m_vip < iGuardFinishVip)
            return HC_ERROR_MORE_VIP_LEVEL;
        int left_second = pcg->m_guard_goods->m_end_time - time(NULL);
        if (left_second > 0)
        {
            int fac = left_second / 600;
            fac += left_second % 600 == 0 ? 0 : 1;

            if (-1 == cd->addGold(-iGuardFinishGold * fac))
                return HC_ERROR_NOT_ENOUGH_GOLD;
            //金币消耗统计
            add_statistics_of_gold_cost(cd->m_id, cd->m_ip_address, iGuardFinishGold * fac, gold_cost_for_accelerate_guard, cd->m_union_id, cd->m_server_id);
#ifdef QQ_PLAT
            gold_cost_tencent(cd.get(),iGuardFinishGold,gold_cost_for_accelerate_guard);
#endif
        }
        //cd->NotifyCharData();
        pcg->m_guard_goods->finish();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//鼓舞
int guardMgr::inspire(int cid, json_spirit::Object& robj)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    int level = m_inspire_map[cid];
    if (level >= 5)
    {
        return HC_ERROR_BOSS_INSPIRE_MAX;    //鼓舞顶级了
    }
    if (pc->m_vip < iGuardInspireVIP)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    //金币是否足够
    if (pc->addGold(-iGuardInspireGold) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //金币消耗统计
    //add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, iGuardInspireGold, gold_cost_for_inspire, pc->m_union_id, pc->m_server_id);
    add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, iGuardInspireGold, gold_cost_for_inspire_guard, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(pc,iGuardInspireGold,gold_cost_for_inspire_guard);
#endif
    pc->NotifyCharData();

    int rands[] = {100,100,100,100,100,30,20,10,5,5};
    //有概率
    if (my_random(1,100) <= rands[level])
    {
        m_inspire_map[cid] = level + 1;
    }
    else
    {
        return HC_ERROR_BOSS_INSPIRE_FAIL;    //失败了
    }
    pc->m_combat_attribute.guard_inspired(m_inspire_map[cid]*20);
    robj.push_back( Pair("level", m_inspire_map[cid]) );
    robj.push_back( Pair("damage", pc->m_combat_attribute.guard_inspired()) );

    robj.push_back( Pair("gold", iGuardInspireGold) );
    std::string suc_msg = strInspireSuccess;
    if (m_inspire_map[cid] > 5)
    {
        str_replace(suc_msg, "$N", LEX_CAST_STR(10));
    }
    else
    {
        str_replace(suc_msg, "$N", LEX_CAST_STR(20));
    }
    robj.push_back( Pair("msg", suc_msg) );
    return HC_SUCCESS;
}

//查询鼓舞信息
int guardMgr::getInspire(int cid, json_spirit::Object& robj)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("inspire", pc->m_combat_attribute.guard_inspired()) );
    return HC_SUCCESS;
}

//清除鼓舞效果
int guardMgr::clearInspire(int cid)
{
    m_inspire_map.erase(cid);
    return 0;
}

int guardMgr::SpeedRobCD(int cid)
{
    boost::shared_ptr<char_goods> pcg = GetCharGoods(cid);
    if (pcg.get() && pcg->m_cooltime > time(NULL))
    {
        boost::shared_ptr<CharData> cd = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cd.get())
            return HC_ERROR;
        if (-1 == cd->addGold(-iGuardSpeedGold))
            return HC_ERROR_NOT_ENOUGH_GOLD;

        //金币消耗统计
        add_statistics_of_gold_cost(cd->m_id, cd->m_ip_address, iGuardSpeedGold, gold_cost_for_clear_guard_cd, cd->m_union_id, cd->m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(cd.get(),iGuardSpeedGold,gold_cost_for_clear_guard_cd);
#endif
        cd->NotifyCharData();
        pcg->m_cooltime = 0;
        save(cid);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int guardMgr::Rob(int atk_id, int def_id, json_spirit::Object& robj)
{
    if (atk_id == def_id)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_goods> pcg_atk = GetCharGoods(atk_id);
    boost::shared_ptr<char_goods> pcg_def = GetCharGoods(def_id);
    if (!pcg_atk.get() || !pcg_def.get())
        return HC_ERROR;
    if (pcg_atk->m_cooltime > time(NULL))
        return HC_ERROR_GUARD_ROB_IS_COLD;
    if (pcg_def->m_state == 1 && pcg_def->m_guard_goods.get())
    {
        if (pcg_atk->getCanRobtime() < 1 || pcg_def->m_guard_goods->m_rob_time < 1)
            return HC_ERROR_GUARD_NO_ROB_TIMES;
        if (pcg_def->m_guard_goods->m_last_rob_cid == atk_id)
            return HC_ERROR_GUARD_ROB_ALREADY;
        //有护送者挑战护送者
        int guard_cid = 0;
        if (pcg_def->m_guard_goods->m_help_cid > 0)
        {
            guard_cid = def_id;
            def_id = pcg_def->m_guard_goods->m_help_cid;
            if (atk_id == def_id)
            {
                return HC_ERROR;
            }
        }

        int ret = HC_SUCCESS;
        Combat* pCombat = createGuardCombat(atk_id, def_id, guard_cid, ret);
        pcg_atk->m_cooltime = time(NULL) + iGuardCD;
        if (HC_SUCCESS == ret && pCombat)
        {
#if 0 //被抢劫的战斗不直接弹出
            json_spirit::Object obj = robj;
            obj.push_back( Pair("cmd", "attackGuard") );
            obj.push_back( Pair("s", 200) );
            obj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
            std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_defender->Name());
            if (account.get())
            {
                account->Send(msg);
            }
#endif
            robj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
            InsertCombat(pCombat);
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(atk_id);
            if (cdata.get())
            {
                //日常任务
                dailyTaskMgr::getInstance()->updateDailyTask(*(cdata.get()),daily_task_guard_rob);
                //act统计
                act_to_tencent(cdata.get(),act_new_guard_rob);
            }
        }
        return ret;
    }
    return HC_ERROR;
}

int guardMgr::combatResult(Combat* pCombat)
{
    int guard_cid = 0;
    if (pCombat->m_extra_data[0] != 0)
    {
        guard_cid = pCombat->m_extra_data[0];
    }
    else
    {
        guard_cid = pCombat->m_defender->getCharId();
    }
    boost::shared_ptr<char_goods> pcg_atk = GetCharGoods(pCombat->m_attacker->getCharId());
    boost::shared_ptr<char_goods> pcg_def = GetCharGoods(guard_cid);
    if (!pcg_atk.get() || !pcg_def.get())
        return HC_ERROR;
    if (pCombat->m_state == attacker_win)
    {
        boost::shared_ptr<CharData> pcd_atk = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_attacker->getCharId());
        if (!pcd_atk.get())
            return HC_ERROR;
        int silver = 0, prestige = 0, supply = 0;
        if (-1 == RobReward(pCombat->m_attacker->getCharId(), guard_cid, silver, prestige, supply))
            return HC_ERROR;

        //劫镖活动
        {
            int score = prestige;
            bool find = false;
            m_score_maps[pcd_atk->m_id] += score;

            //优化
            if (m_score_maps[pcd_atk->m_id] > m_topten_min_score || m_topten_score.size() < 10)
            {
                for (std::list<robScore>::iterator it = m_topten_score.begin(); it != m_topten_score.end(); ++it)
                {
                    if ((*it).cid == pcd_atk->m_id)
                    {
                        (*it).score = m_score_maps[pcd_atk->m_id];
                        find = true;
                        break;
                    }
                }
                if (!find)
                {
                    robScore rs;
                    rs.cid = pcd_atk->m_id;
                    rs.score = m_score_maps[rs.cid];
                    m_topten_score.push_back(rs);
                }
                m_topten_score.sort(compare_score);
                if (m_topten_score.size() > 0)
                {
                    m_topten_min_score = m_topten_score.back().score;
                }
            }            
            InsertSaveDb("replace into char_guard_rankscore (cid,score) values (" + LEX_CAST_STR(pcd_atk->m_id) + "," + LEX_CAST_STR(m_score_maps[pcd_atk->m_id]) + ")");
        }

        //银币
        Item item_silver(item_type_silver, 0, silver, 1);
        pCombat->m_getItems.push_back(item_silver);

        //声望
        Item item_p(item_type_prestige, 0, prestige, 1);
        pCombat->m_getItems.push_back(item_p);
        
        //军粮
        Item item_sp(item_type_treasure, treasure_type_supply, supply, 1);
        pCombat->m_getItems.push_back(item_sp);

        //给东西
        giveLoots(pcd_atk, pCombat, true, give_guard);

        //pcd_atk->addSilver(silver);
        //银币获得统计
        //add_statistics_of_silver_get(pcd_atk->m_id,pcd_atk->m_ip_address,silver,silver_get_guard);
        //pcd_atk->addPrestige(prestige);
        //if (prestige > 0)
        //{
        //    corpsMgr::getInstance()->addEvent(pcd_atk.get(), corps_event_add_exp, prestige, 0);
        //}
        //pcd_atk->addTreasure(treasure_type_supply, supply);
        ++(pcg_atk->m_robtime);
        --(pcg_def->m_guard_goods->m_rob_time);
        (pcg_def->m_guard_goods->m_silver) -= silver;
        (pcg_def->m_guard_goods->m_prestige) -= prestige;
        (pcg_def->m_guard_goods->m_supply) -= supply;
        pcg_def->m_guard_goods->m_last_rob_cid = pCombat->m_attacker->getCharId();

        //记录到信息
        boost::shared_ptr<event_log> p;
        p.reset(new event_log());
        p->m_atk_id = pCombat->m_attacker->getCharId();
        p->m_def_id = guard_cid;
        p->m_gid = pcg_def->m_gid;
        p->silver = silver;
        p->prestige = prestige;
        p->supply = supply;
        if (m_event_log.size() >= 4)
            m_event_log.pop_front();
        m_event_log.push_back(p);
        //广播给其他人
        BroadRobGoodsEvent();
        //发送信件
        pCombat->m_mail_content = strGuardRobSucMailContent;
        boost::shared_ptr<base_goods> pbg = GetBaseGoods(pcg_def->m_gid);
        if (pbg.get())
        {
            str_replace(pCombat->m_mail_content, "$N", pbg->color_name);
        }
        str_replace(pCombat->m_mail_content, "$A", pcd_atk->m_name);
        str_replace(pCombat->m_mail_content, "$S", LEX_CAST_STR(silver));
        str_replace(pCombat->m_mail_content, "$P", LEX_CAST_STR(prestige));
        str_replace(pCombat->m_mail_content, "$s", LEX_CAST_STR(supply));
        pCombat->m_mail_to = pCombat->m_defender->getCharId();
        pCombat->m_mail_to_name = pCombat->m_defender->Name();
        pCombat->m_mail_title = strGuardMailTitle;

        //sendSystemMail(pCombat->m_defender->Name(), pCombat->m_defender->getCharId(), strGuardMailTitle, content, pCombat->m_combat_id);
        save(pcg_atk->m_cid);
        save(pcg_def->m_cid);

        //打劫仇敌成功
        if (Singleton<relationMgr>::Instance().is_my_enemy(pCombat->m_attacker->getCharId(), guard_cid))
        {
            boost::shared_ptr<CharData> ecdata = GeneralDataMgr::getInstance()->GetCharData(guard_cid);
            if (ecdata.get())
            {
                Singleton<relationMgr>::Instance().postEnemyInfos(pCombat->m_attacker->getCharId(),
                                    "", guard_cid, ecdata->m_name, ENEMY_NEWS_ROB_ENEMY_SUCCESS);
            }
        }
        //增加仇恨
        Singleton<relationMgr>::Instance().addHate(guard_cid, pCombat->m_attacker->getCharId(), 1);
    }
    else if(pCombat->m_state == defender_win)
    {
        boost::shared_ptr<CharData> pcd_atk = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_attacker->getCharId());
        if (!pcd_atk.get())
            return HC_ERROR;

        //打劫失败，安慰奖励
        int silver = pCombat->m_defender->level() * 10, prestige = 2;
        
        //劫镖活动
        {
            int score = prestige;
            bool find = false;
            m_score_maps[pcd_atk->m_id] += score;

            //优化
            if (m_score_maps[pcd_atk->m_id] > m_topten_min_score || m_topten_score.size() < 10)
            {
                for (std::list<robScore>::iterator it = m_topten_score.begin(); it != m_topten_score.end(); ++it)
                {
                    if ((*it).cid == pcd_atk->m_id)
                    {
                        (*it).score = m_score_maps[pcd_atk->m_id];
                        find = true;
                        break;
                    }
                }
                if (!find)
                {
                    robScore rs;
                    rs.cid = pcd_atk->m_id;
                    rs.score = m_score_maps[rs.cid];
                    m_topten_score.push_back(rs);
                }
                m_topten_score.sort(compare_score);
                if (m_topten_score.size() > 0)
                {
                    m_topten_min_score = m_topten_score.back().score;
                }
            }            
            InsertSaveDb("replace into char_guard_rankscore (cid,score) values (" + LEX_CAST_STR(pcd_atk->m_id) + "," + LEX_CAST_STR(m_score_maps[pcd_atk->m_id]) + ")");
        }
        //银币
        Item item_silver(item_type_silver, 0, silver, 1);
        pCombat->m_getItems.push_back(item_silver);

        //声望
        Item item_p(item_type_prestige, 0, prestige, 1);
        pCombat->m_getItems.push_back(item_p);

        //给东西
        giveLoots(pcd_atk, pCombat, true, give_guard);

        //if (prestige > 0)
        //{
        //    corpsMgr::getInstance()->addEvent(pcd_atk.get(), corps_event_add_exp, prestige, 0);
        //}

        ++(pcg_atk->m_robtime);

        //发送信件
        pCombat->m_mail_content = strGuardRobFailMailContent;
        boost::shared_ptr<base_goods> pbg = GetBaseGoods(pcg_def->m_gid);
        if (pbg.get())
        {
            str_replace(pCombat->m_mail_content, "$N", pbg->color_name);
        }
        str_replace(pCombat->m_mail_content, "$A", pcd_atk->m_name);
        pCombat->m_mail_to = pCombat->m_defender->getCharId();
        pCombat->m_mail_to_name = pCombat->m_defender->Name();
        pCombat->m_mail_title = strGuardMailTitle;

        //打劫仇敌失败
        if (Singleton<relationMgr>::Instance().is_my_enemy(pCombat->m_attacker->getCharId(), guard_cid))
        {
            boost::shared_ptr<CharData> ecdata = GeneralDataMgr::getInstance()->GetCharData(guard_cid);
            if (ecdata.get())
            {
                Singleton<relationMgr>::Instance().postEnemyInfos(pCombat->m_attacker->getCharId(),
                                    "", guard_cid, ecdata->m_name, ENEMY_NEWS_ROB_ENEMY_FAIL);
            }
        }
        //sendSystemMail(pCombat->m_defender->Name(), pCombat->m_defender->getCharId(), strGuardMailTitle, content);
    }
    pCombat->AppendResult(pCombat->m_result_obj);
    //战报存/发送
    InsertSaveCombat(pCombat);
    return HC_SUCCESS;
}

int guardMgr::RobReward(int atk_id, int def_id, int& silver, int& prestige, int& supply)
{
    boost::shared_ptr<CharData> pcd_atk = GeneralDataMgr::getInstance()->GetCharData(atk_id);
    boost::shared_ptr<CharData> pcd_def = GeneralDataMgr::getInstance()->GetCharData(def_id);
    boost::shared_ptr<char_goods> pcg_def = GetCharGoods(def_id);
    if (!pcd_atk.get() || !pcd_def.get() || !pcg_def.get() || !pcg_def->m_guard_goods.get())
        return -1;
    boost::shared_ptr<base_goods> pbg = GetBaseGoods(pcg_def->m_guard_goods->m_gid);
    if (!pbg.get())
        return -1;
    /*基础劫20%，根据等级差银币每级修正2%，声望每三级修正10%*/
    int fac = pcd_def->m_level - pcd_atk->m_level;
    silver = pbg->silver * pcd_def->m_level * (100 + fac * 2) / 500;
    prestige = pbg->prestige * (100 + fac / 3 * 10) / 500;
    supply = pbg->supply * (100 + fac / 3 * 10) / 500;
    if (silver < 10000)
        silver = 10000;
    if (prestige < 1)
        prestige = 1;
    if (supply < 1)
    {
        supply = 1;
    }
    if (pcg_def->m_guard_goods->m_reward_more > 100)
    {
        silver = silver * pcg_def->m_guard_goods->m_reward_more / 100;
        prestige = prestige * pcg_def->m_guard_goods->m_reward_more / 100;
        supply = supply * pcg_def->m_guard_goods->m_reward_more / 100;
    }
    return 0;
}

int guardMgr::GuardDone(int cid)
{
    boost::shared_ptr<char_goods> pcg = GetCharGoods(cid);
    if (pcg.get() && pcg->m_state == 1 && pcg->m_guard_goods.get())
    {
        boost::shared_ptr<CharData> cd = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cd.get())
            return HC_ERROR;
        cd->addSilver(pcg->m_guard_goods->m_silver);
        //银币获得统计
        add_statistics_of_silver_get(cd->m_id,cd->m_ip_address,pcg->m_guard_goods->m_silver,silver_get_guard, cd->m_union_id, cd->m_server_id);
        cd->addPrestige(pcg->m_guard_goods->m_prestige);
        if (pcg->m_guard_goods->m_prestige > 0)
        {
            corpsMgr::getInstance()->addEvent(cd.get(), corps_event_add_exp, pcg->m_guard_goods->m_prestige, 0);
            add_statistics_of_prestige_get(cd->m_id,cd->m_ip_address,pcg->m_guard_goods->m_prestige,prestige_guard,cd->m_union_id,cd->m_server_id);
        }
        //加军粮
        cd->addTreasure(treasure_type_supply, pcg->m_guard_goods->m_supply);
        add_statistics_of_treasure_cost(cd->m_id,cd->m_ip_address,treasure_type_supply,pcg->m_guard_goods->m_supply,treasure_guard,1,cd->m_union_id,cd->m_server_id);
        //notify result,send mail
        std::string content = strGuardMailContent;
        boost::shared_ptr<base_goods> pbg = GetBaseGoods(pcg->m_gid);
        if (pbg.get())
        {
            str_replace(content, "$N", pbg->color_name);
        }
        str_replace(content, "$S", LEX_CAST_STR(pcg->m_guard_goods->m_silver));
        str_replace(content, "$P", LEX_CAST_STR(pcg->m_guard_goods->m_prestige));
        str_replace(content, "$s", LEX_CAST_STR(pcg->m_guard_goods->m_supply));
        sendSystemMail(cd->m_name, cid, strGuardMailTitle, content);
        pcg->m_state = 0;
        pcg->m_gid = 1;
        pcg->m_guard_goods->m_start_time = 0;
        pcg->m_guard_goods->m_end_time = 0;
        pcg->m_helper_info = 0;
        pcg->m_guard_goods.reset();
        save(cid);
        clearInspire(cid);
        //给玩家自己的弹窗
        json_spirit::Object notify;
        notify.push_back( Pair ("cmd", "guardGoodsSucc"));
        notify.push_back( Pair ("msg", content));
        notify.push_back( Pair ("s", 200));
        //广播给当前界面的玩家
        json_spirit::Object obj;
        obj.push_back( Pair ("cmd", "removeGuardQueue"));
        json_spirit::Array list;
        json_spirit::Object o;
        o.push_back( Pair ("id", pcg->m_cid));
        list.push_back(o);
        obj.push_back( Pair ("list", list));
        obj.push_back( Pair ("s", 200));
        std::map<uint64_t,int>::iterator it = m_uid_list.begin();
        while (it != m_uid_list.end())
        {
            boost::shared_ptr<CharData> cd = GeneralDataMgr::getInstance()->GetCharData(it->first);
            if (cd.get())
            {
                boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(cd->m_name);
                if (pchar.get())
                {
                    std::string final_result = json_spirit::write(obj, json_spirit::raw_utf8);
                    pchar->Send(final_result);
                    if (cd->m_id == cid)
                    {
                        pchar->Send(json_spirit::write(notify, json_spirit::raw_utf8));
                    }
                }
            }
            ++it;
        }

        //好友动态
        if (pcg->m_helper_info > 0)
        {
            Singleton<relationMgr>::Instance().postFriendInfos(cid, cd->m_name, FRIEND_NEWS_INVITE_GUARD_SUCCESS, pcg->m_helper_info, "");
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int guardMgr::GetGuardTaskList(int cid, json_spirit::Object& robj)
{
    json_spirit::Array list, elist;
    std::map<int, boost::shared_ptr<char_goods> >::iterator it = m_char_goods.begin();
    while (it != m_char_goods.end())
    {
        if (it->second.get() && it->second->m_state == 1 && it->second->m_guard_goods.get())
        {
            json_spirit::Object o;
            boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(it->first);
            if (!pcd.get())
            {
                ++it;
                continue;
            }
            o.push_back( Pair("id", it->first) );
            o.push_back( Pair("name", pcd->m_name) );
            if (it->second->m_helper_info > 0)
            {
                o.push_back( Pair("help_id", it->second->m_helper_info) );
            }
            o.push_back( Pair("guardLeftTime", it->second->m_guard_goods->m_end_time - time(NULL)) );
            json_spirit::Object goodsVO;
            goodsVO.push_back( Pair("quality", it->second->m_guard_goods->m_gid) );
            goodsVO.push_back( Pair("totalTime", it->second->m_guard_goods->m_needmin) );
            o.push_back( Pair("goodsVO", goodsVO) );
            list.push_back(o);

            if (Singleton<relationMgr>::Instance().is_my_enemy(cid, it->first))
            {
                json_spirit::Object e;
                e.push_back( Pair("id", it->first));
                elist.push_back(e);
            }
        }
        else
        {
            INFO("cid" << it->first << " is not doing" << endl);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("elist", elist) );
    return HC_SUCCESS;
}

int guardMgr::GetGuardTaskInfo(int cid, int id, json_spirit::Object& robj)
{
    json_spirit::Object info;
    boost::shared_ptr<char_goods> pcg = GetCharGoods(id);
    if (pcg.get() && pcg->m_state == 1 && pcg->m_guard_goods.get())
    {
        int silver = 0, prestige = 0, supply = 0;
        boost::shared_ptr<CharData> be_rob_pcd = GeneralDataMgr::getInstance()->GetCharData(id);
        if (!be_rob_pcd.get())
            return HC_ERROR;
        info.push_back( Pair("id", id) );
        info.push_back( Pair("name", be_rob_pcd->m_name) );
        info.push_back( Pair("level", be_rob_pcd->m_level) );
        if (pcg->m_helper_info > 0)
        {
            boost::shared_ptr<CharData> helper_pcd = GeneralDataMgr::getInstance()->GetCharData(pcg->m_helper_info);
            if (helper_pcd.get())
            {
                info.push_back( Pair("help_name", helper_pcd->m_name) );
                info.push_back( Pair("help_level", helper_pcd->m_level) );
            }
        }
        if (be_rob_pcd->m_corps_member.get())
        {
            info.push_back( Pair("corps", corpsMgr::getInstance()->getCorpsName(be_rob_pcd->m_corps_member->corps)) );
        }
        if (cid == id)//查看自己
        {
            info.push_back( Pair("silver", pcg->m_guard_goods->m_silver) );
            info.push_back( Pair("prestige", pcg->m_guard_goods->m_prestige) );
            info.push_back( Pair("supply", pcg->m_guard_goods->m_supply) );
        }
        else if(RobReward(cid, id, silver, prestige, supply) == 0)
        {
            info.push_back( Pair("silver", silver) );
            info.push_back( Pair("prestige", prestige) );
            info.push_back( Pair("supply", supply) );
        }
        else
        {
            return HC_ERROR;
        }

        info.push_back( Pair("more", pcg->m_guard_goods->m_reward_more) );
        info.push_back( Pair("guardLeftTime", pcg->m_guard_goods->m_end_time - time(NULL)) );
        json_spirit::Object goodsVO;
        goodsVO.push_back( Pair("robTimes", 2 - pcg->m_guard_goods->m_rob_time) );
        goodsVO.push_back( Pair("robTimesTotal", 2) );
        goodsVO.push_back( Pair("quality", pcg->m_guard_goods->m_gid) );
        boost::shared_ptr<base_goods> pbg = GetBaseGoods(pcg->m_guard_goods->m_gid);
        if (pbg.get())
        {
            goodsVO.push_back( Pair("name", pbg->name) );
            goodsVO.push_back( Pair("totalTime", pbg->need_min) );
        }
        info.push_back( Pair("goodsVO", goodsVO) );
    }
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

int guardMgr::BroadRobGoodsEvent()
{
    json_spirit::Object robj;
    json_spirit::Array list;
    std::list<boost::shared_ptr<event_log> >::iterator it = m_event_log.begin();
    while (it != m_event_log.end())
    {
        if ((*it).get())
        {
            json_spirit::Object o;
            std::string msg = strGuardRobLog;
            boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData((*it)->m_atk_id);
            if (pcd.get())
            {
                str_replace(msg, "$A", MakeCharNameLink_other(pcd->m_name));
            }
            pcd = GeneralDataMgr::getInstance()->GetCharData((*it)->m_def_id);
            if (pcd.get())
            {
                str_replace(msg, "$D", MakeCharNameLink_other(pcd->m_name));
            }
            boost::shared_ptr<base_goods> pbg = GetBaseGoods((*it)->m_gid);
            if (pbg.get())
            {
                str_replace(msg, "$N", pbg->color_name);
                str_replace(msg, "$S", LEX_CAST_STR((*it)->silver));
                str_replace(msg, "$P", LEX_CAST_STR((*it)->prestige));
                str_replace(msg, "$s", LEX_CAST_STR((*it)->supply));
            }
            o.push_back( Pair("msg", msg) );
            list.push_back(o);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("cmd", "getRobGoodsEvent") );
    robj.push_back( Pair("s", 200) );
    std::map<uint64_t,int>::iterator it_u = m_uid_list.begin();
    while (it_u != m_uid_list.end())
    {
        boost::shared_ptr<CharData> cd = GeneralDataMgr::getInstance()->GetCharData(it_u->first);
        if (cd.get())
        {
            boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(cd->m_name);
            if (pchar.get())
            {
                std::string final_result = json_spirit::write(robj, json_spirit::raw_utf8);
                pchar->Send(final_result);
            }
        }
        ++it_u;
    }
    return HC_SUCCESS;
}

int guardMgr::GetRobGoodsEvent(json_spirit::Object& robj)
{
    json_spirit::Array list;
    std::list<boost::shared_ptr<event_log> >::iterator it = m_event_log.begin();
    while (it != m_event_log.end())
    {
        if ((*it).get())
        {
            json_spirit::Object o;
            std::string msg = strGuardRobLog;
            boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData((*it)->m_atk_id);
            if (pcd.get())
            {
                str_replace(msg, "$A", MakeCharNameLink_other(pcd->m_name));
            }
            pcd = GeneralDataMgr::getInstance()->GetCharData((*it)->m_def_id);
            if (pcd.get())
            {
                str_replace(msg, "$D", MakeCharNameLink_other(pcd->m_name));
            }
            boost::shared_ptr<base_goods> pbg = GetBaseGoods((*it)->m_gid);
            if (pbg.get())
            {
                str_replace(msg, "$N", pbg->color_name);
                str_replace(msg, "$S", LEX_CAST_STR((*it)->silver));
                str_replace(msg, "$P", LEX_CAST_STR((*it)->prestige));
                str_replace(msg, "$s", LEX_CAST_STR((*it)->supply));
            }
            o.push_back( Pair("msg", msg) );
            list.push_back(o);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int guardMgr::GetGuardGoodsList(CharData* pc, json_spirit::Object& robj)
{
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<base_goods> >::iterator it = m_base_goods.begin();
    while (it != m_base_goods.end())
    {
        if (it->second.get())
        {
            json_spirit::Object o;
            o.push_back( Pair("id", it->first) );
            o.push_back( Pair("spic", it->first) );
            o.push_back( Pair("quality", it->first) );
            o.push_back( Pair("name", it->second->name) );
            o.push_back( Pair("totalTime", it->second->need_min) );
            o.push_back( Pair("silver", it->second->silver * pc->m_level) );
            o.push_back( Pair("prestige", it->second->prestige) );
            o.push_back( Pair("supply", it->second->supply) );
            list.push_back(o);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    json_spirit::Object info;
    info.push_back( Pair("refreshGold", iGuardRefreshGold) );
    info.push_back( Pair("callGold", iGuardCallGold) );
    info.push_back( Pair("inspireGold", iGuardInspireGold) );
    getInspire(pc->m_id,info);
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

int guardMgr::RefreshGoods(CharData* cd, int type, json_spirit::Object& robj)
{
    boost::shared_ptr<char_goods> pcg = GetCharGoods(cd->m_id);
    if (NULL == pcg.get() || pcg->m_state != 0 || pcg->m_gid == 5)
    {
        return HC_ERROR;
    }
    //VIP5及以上，从蓝色品质开始送
    if (cd->m_vip >= 5 && pcg->m_gid < 2)
    {
        pcg->m_gid = 2;
    }
    if (type == 1)
    {
        if (-1 == cd->addGold(-iGuardRefreshGold))
            return HC_ERROR_NOT_ENOUGH_GOLD;
        //金币消耗统计
        add_statistics_of_gold_cost(cd->m_id, cd->m_ip_address, iGuardRefreshGold, gold_cost_for_refresh_guard_goods, cd->m_union_id, cd->m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(cd,iGuardRefreshGold,gold_cost_for_refresh_guard_goods);
#endif
        cd->NotifyCharData();
        if (pcg->m_gid > 0 && pcg->m_gid < 5)
        {
            if (my_random(0,99) < iGuardRefreshPer[pcg->m_gid - 1])
                ++(pcg->m_gid);
        }
    }
    else if (type == 2)
    {
        if (cd->m_vip < iGuardCallVip)
            return HC_ERROR_MORE_VIP_LEVEL;
        if (-1 == cd->addGold(-iGuardCallGold))
            return HC_ERROR_NOT_ENOUGH_GOLD;
        pcg->m_gid = 5;
        //金币消耗统计
        add_statistics_of_gold_cost(cd->m_id, cd->m_ip_address, iGuardCallGold, gold_cost_for_call_guard_goods, cd->m_union_id, cd->m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(cd,iGuardCallGold,gold_cost_for_call_guard_goods);
#endif
        cd->NotifyCharData();
        m_inspire_map[cd->m_id] = 2;
        cd->m_combat_attribute.guard_inspired(m_inspire_map[cd->m_id]*20);
    }
    return HC_SUCCESS;
}

boost::shared_ptr<char_goods> guardMgr::GetCharGoods(int cid)
{
    INFO("get char goods" << endl);
    std::map<int, boost::shared_ptr<char_goods> >::iterator it = m_char_goods.find(cid);
    if (it != m_char_goods.end())
    {
        return it->second;
    }
    else
    {
        Query q(GetDb());
        boost::shared_ptr<char_goods> pcg;
        pcg.reset(new char_goods);
        q.get_result("select gid, cooltime, robtime, guardtime, helptime, state, helper_cid, start_time, end_time, can_be_rob_time, silver, prestige,supply from char_guard_goods where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            pcg->m_cid = cid;
            pcg->m_gid = q.getval();
            pcg->m_cooltime = q.getval();
            pcg->m_robtime = q.getval();
            pcg->m_guardtime = q.getval();
            pcg->m_helptime = q.getval();
            pcg->m_state = q.getval();
            pcg->m_helper_info = q.getval();
            pcg->m_helpwaittime = 0;
            time_t start_time = q.getval();
            time_t end_time = q.getval();
            int can_rob_time = q.getval();
            int silver = q.getval();
            int prestige = q.getval();
            int supply = q.getval();
            boost::shared_ptr<goods> pg;
            pg.reset();
            if (pcg->m_state == 1)
            {
                pg.reset(new goods(cid,pcg->m_gid));
                pg->m_rob_time = can_rob_time;
                pg->m_start_time = start_time;
                pg->m_end_time = end_time;
                pg->m_silver = silver;
                pg->m_prestige = prestige;
                pg->m_supply = supply;
                pg->m_needmin = (end_time - start_time) / 60;
                pcg->m_guard_goods = pg;
                pcg->m_guard_goods->start();
            }
            m_char_goods[cid] = pcg;
            q.free_result();
        }
        else
        {
            q.free_result();
            pcg.reset();
            return open(cid);
        }
        
        return pcg;
    }
}

boost::shared_ptr<base_goods> guardMgr::GetBaseGoods(int gid)
{
    std::map<int, boost::shared_ptr<base_goods> >::iterator it = m_base_goods.find(gid);
    if (it != m_base_goods.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<base_goods> pbg;
        pbg.reset();
        return pbg;
    }
}

int guardMgr::resetAll()
{
    std::map<int, boost::shared_ptr<char_goods> >::iterator it = m_char_goods.begin();
    while (it != m_char_goods.end())
    {
        if (it->second.get())
        {
            it->second->m_cooltime = 0;
            it->second->m_helper_info = 0;
            it->second->m_robtime = 0;//iGuardRobTimes;
            it->second->m_guardtime = 0;//iGuardTimes;
            it->second->m_helptime = 0;//iGuardHelpTimes;
        }
        ++it;
    }
    //InsertSaveDb("update char_guard_goods set cooltime=0,robtime="+LEX_CAST_STR(iGuardRobTimes)+",guardtime="+LEX_CAST_STR(iGuardTimes)+",helptime="+LEX_CAST_STR(iGuardHelpTimes)+" where 1");
    InsertSaveDb("update char_guard_goods set cooltime=0,robtime=0,guardtime=0,helptime=0 where 1");
    return HC_SUCCESS;
}

int guardMgr::deleteChar(int cid)
{
    InsertSaveDb("delete from char_guard_goods where cid=" + LEX_CAST_STR(cid));
    m_char_goods.erase(cid);
    return HC_SUCCESS;
}

int guardMgr::guardtest(int cid)
{
    boost::shared_ptr<char_goods> pcg = GetCharGoods(cid);
    if (pcg.get())
    {
        pcg->m_robtime = 0;
        pcg->m_guardtime = 0;
    }
    return HC_SUCCESS;
}

boost::shared_ptr<char_goods> guardMgr::open(int cid)
{
    std::map<int, boost::shared_ptr<char_goods> >::iterator it = m_char_goods.find(cid);
    if (it == m_char_goods.end())
    {
        boost::shared_ptr<char_goods> pcg;
        pcg.reset(new char_goods);
        pcg->m_cid = cid;
        pcg->m_gid = 1;
        pcg->m_cooltime = 0;
        pcg->m_robtime = 0;//iGuardRobTimes;
        pcg->m_guardtime = 0;//iGuardTimes;
        pcg->m_helptime = 0;//iGuardHelpTimes;
        pcg->m_helper_info = 0;
        pcg->m_helpwaittime = 0;
        pcg->m_state = 0;
        m_char_goods[cid] = pcg;
        //InsertSaveDb("insert into char_guard_goods set cid=" + LEX_CAST_STR(cid) + ",robtime=" + LEX_CAST_STR(iGuardRobTimes) + ",guardtime=" + LEX_CAST_STR(iGuardTimes) + ",helptime=" + LEX_CAST_STR(iGuardHelpTimes));
        InsertSaveDb("insert into char_guard_goods set cid=" + LEX_CAST_STR(cid) + ",robtime=0,guardtime=0,helptime=0");
        return pcg;
    }
    else
    {
        return it->second;
    }
}

void guardMgr::openGuardEvent(int fac, int last_mins)
{
    getGuardEvent();
    if (fac > 100)
    {
        m_guard_reward_more = fac;
        //活动开启
        GeneralDataMgr::getInstance()->broadCastSysMsg(strGuardEventOpen, -1);

        m_guard_reward_end_time = time(NULL) + 60 * last_mins;

        GeneralDataMgr::getInstance()->setInt("guard_reward", m_guard_reward_more);
        GeneralDataMgr::getInstance()->setInt("guard_reward_end", m_guard_reward_end_time);
    }
    else if (fac <= 100 && m_guard_reward_more > 100)
    {
        m_guard_reward_more = 100;
        m_guard_reward_end_time = 0;
        GeneralDataMgr::getInstance()->setInt("guard_reward", m_guard_reward_more);
        GeneralDataMgr::getInstance()->setInt("guard_reward_end", m_guard_reward_end_time);
    }
}

int guardMgr::getGuardEvent()
{
    if (time(NULL) < m_guard_reward_end_time)
    {
        return m_guard_reward_more;
    }
    else
    {
        if (m_guard_reward_more > 100)
        {
            m_guard_reward_more = 100;
            GeneralDataMgr::getInstance()->setInt("guard_reward", m_guard_reward_more);
        }
        return 100;
    }
}

//设置每天可以帮忙护送,劫粮，运粮的次数
void guardMgr::setGuardTimes(int help, int rob, int guard)
{
    //护送功能次数
    if (help <= 0 && rob <= 0 && guard <= 0)
    {
        iGuardHelpTimes = iDefaultGuardHelpTimes;
        iGuardRobTimes = iDefaultGuardRobTimes;
        iGuardTimes = iDefaultGuardTimes;
    }
    else
    {
        iGuardHelpTimes = help;
        iGuardRobTimes = rob;
        iGuardTimes = guard;        
    }
    GeneralDataMgr::getInstance()->setInt("guard_help_times", iGuardHelpTimes);
    GeneralDataMgr::getInstance()->setInt("guard_rob_times", iGuardRobTimes);
    GeneralDataMgr::getInstance()->setInt("guard_times", iGuardTimes);
    //resetAll();
}

struct guard_friend
{
    int id;
    int level;
    int left;
    std::string name;
};

inline bool guard_good_compare(guard_friend& a, guard_friend& b)
{
    return (a.level > b.level);
}

int guardMgr::GetGuardFriendsList(CharData* cdata, json_spirit::Object& robj)
{
    std::list<guard_friend> guard_friends_list;
    boost::shared_ptr<char_ralation> my_rl = Singleton<relationMgr>::Instance().getRelation(cdata->m_id);
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_friends.begin();
            it != my_rl->m_my_friends.end();
                ++it)
    {
        CharData* pc = it->second->getChar().get();
        if (pc && pc->m_is_online && pc->m_guardOpen)
        {
            boost::shared_ptr<char_goods> pcg = GetCharGoods(it->first);
            if (pcg.get() && pcg->getCanHelptime() > 0)
            {
                guard_friend gf;
                gf.id = it->first;
                gf.level = pc->m_level;
                gf.name = pc->m_name;
                gf.left = pcg->getCanHelptime();
                guard_friends_list.push_back(gf);
            }
        }
    }

    json_spirit::Array list;
    if (guard_friends_list.size())
    {
        guard_friends_list.sort(guard_good_compare);
        for (std::list<guard_friend>::iterator it = guard_friends_list.begin(); it != guard_friends_list.end(); ++it)
        {
            json_spirit::Object o;
            o.push_back( Pair("id", it->id) );
            o.push_back( Pair("name", it->name) );
            o.push_back( Pair("level", it->level) );
            o.push_back( Pair("left", it->left) );
            list.push_back(o);
            if (list.size() >= 5)
            {
                break;
            }
        }
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int guardMgr::ApplyGuardHelp(CharData* pc, int friend_id)
{
    boost::shared_ptr<char_goods> pcg_char = GetCharGoods(pc->m_id);
    boost::shared_ptr<char_goods> pcg = GetCharGoods(friend_id);
    boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(friend_id);
    //好友还有护送次数
    if (pcg_char.get() && pcg.get() && pcg->getCanHelptime() > 0 && pcd.get() && pcd->check_friend(pc->m_id))
    {
        //状态不是等候回应或者上次请求已经过期
        if (pcg_char->m_helper_info == 0 || (pcg_char->m_helper_info == -1 && time(NULL) > pcg_char->m_helpwaittime))
        {
            //设置请求状态为等候状态
            pcg_char->m_helper_info = -1;
            pcg_char->m_helpwaittime = time(NULL) + 60;
            //通知被请求玩家
            std::string content = strGuardHelpMailContent;
            str_replace(content, "$N", pc->m_name);
            sendSystemMail(pcd->m_name, pcd->m_id, strGuardHelpMailTitle, content, "", 0, 1, pc->m_id);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int guardMgr::ApplyGuardCancel(int cid)
{
    boost::shared_ptr<char_goods> pcg_char = GetCharGoods(cid);
    if (pcg_char.get() && pcg_char->m_helper_info != 0)
    {
        //设置请求状态为普通
        pcg_char->m_helper_info = 0;
        pcg_char->m_helpwaittime = 0;
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int guardMgr::AnswerGuardHelp(int cid, int friend_id, int type)
{
    boost::shared_ptr<char_goods> pcg_char = GetCharGoods(cid);
    boost::shared_ptr<char_goods> pcg = GetCharGoods(friend_id);
    boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(friend_id);
    //好友还有护送次数
    if (pcg.get() && pcg->m_helper_info == -1 && time(NULL) < pcg->m_helpwaittime 
        && pcg_char.get() && pcg_char->getCanHelptime() > 0 
        && pcd.get() && pcd->check_friend(cid))
    {
        if (type == 0)//拒绝护送
        {
            pcg->m_helper_info = 0;
        }
        else if(type == 1)//同意请求
        {
            pcg->m_helper_info = cid;
        }
        //通知请求玩家
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pcd->m_name);
        if (account)
        {
            std::string msg = strGuardHelp;
            str_replace(msg, "$T", LEX_CAST_STR(type));
            account->Send(msg);
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

