
#include "treasure.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_all.h"
#include "utils_lang.h"
#include "spls_timer.h"
#include "errcode_def.h"
#include <syslog.h>
#include "task.h"
#include "goal.h"
#include "singleton.h"
#include "relation.h"

#define INFO(x) //cout<<x

extern std::string strTreasureRobLog;
extern std::string strTreasureMailTitle;
extern std::string strTreasureMailContent;
extern std::string strTreasureRobSucMailContent;
extern std::string strTreasureRobFailMailContent;
extern std::string strTreasureRobLateMailContent;
extern std::string strTreasureRed;
extern std::string strTreasureOrange;

static const int iDefaultTreasureRobTimes = 4;
static const int iDefaultTreasureTimes = 5;

volatile int iTreasureRobTimes = iDefaultTreasureRobTimes;
volatile int iTreasureTimes = iDefaultTreasureTimes;

static const int iTreasureFinishGold = 1;
static const int iTreasureRefreshGold = 10;
static const int iTreasureRefreshGoldTimes[iMaxVIP+1] = {10,10,10,10,10,10,12,14,16,18,20};
static const int iTreasureRefreshSilver = 6000;
static const int iTreasureRefreshSilverTimes = 5;
static const int iTreasurePurpleVip = 6;
static const int iTreasureCallVip = 5;
static const int iTreasureCallGold = 500;
static const int iTreasurePosX[2] = {250, 850};
static const int iTreasurePosY[2] = {250, 550};
//刷新进阶概率
const int iTreasureRefreshPer[] = {70, 35, 15, 8, 5};


Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

//藏宝图完成的处理
int ProcessTreasureDone(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    Singleton<treasureMgr>::Instance().jobDone(cid);
    return HC_SUCCESS;
}

//获取公告信息
int ProcessGetRobEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return Singleton<treasureMgr>::Instance().getRobEvent(robj);
}

//获取藏宝图列表
int ProcessGetBaseTreasureInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int tid = 0;
    READ_INT_FROM_MOBJ(tid, o, "tid");
    return Singleton<treasureMgr>::Instance().getTreasureInfo(tid, robj);
}

//获取藏宝图列表
int ProcessGetAllTreasureList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return Singleton<treasureMgr>::Instance().getTreasureList(cdata, robj);
}

//获取角色当前藏宝图信息
int ProcessGetCharTreasure(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return Singleton<treasureMgr>::Instance().getCharTreasureInfo(cdata, robj);
}

int ProcessDealTreasure(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type) );
    if (type == 1 || type == 2 || type == 4)
        return Singleton<treasureMgr>::Instance().refresh(cdata, type, robj);
    else if(type == 3)
        return Singleton<treasureMgr>::Instance().start(cdata->m_id);
    else
        return HC_ERROR;
}

//进行中的藏宝图列表
int ProcessGetTreasureList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return Singleton<treasureMgr>::Instance().getList(cdata->m_id, robj);
}

//进行中的藏宝图信息
int ProcessGetTreasureInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return Singleton<treasureMgr>::Instance().getInfo(cdata->m_id, id, robj);
}

//离开藏宝图界面
int ProcessQuitTreasure(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    Singleton<treasureMgr>::Instance().getoutsence(cdata->m_id);
    return HC_SUCCESS;
}

int treasure::start()
{
    //重启后的情况
    int leftsecond = m_end_time - time(NULL);

    json_spirit::mObject mobj;
    mobj["cmd"] = "treasureDone";
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

int treasure::finish()
{
    if (m_end_time >= time(NULL))
    {
        splsTimerMgr::getInstance()->delTimer(_uuid);
        Singleton<treasureMgr>::Instance().jobDone(m_cid);
        return 0;
    }
    return -1;
}

int char_treasure::getCanRobTimes()
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        return 0;
    }
    int left = iTreasureRobTimes - cdata->queryExtraData(char_data_type_daily, char_data_daily_treasure_rob);
    if (left >= 0)
    {
        return left;
    }
    else
    {
        return 0;
    }
}

int char_treasure::getCanStartTimes()
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        return 0;
    }
    int left = iTreasureTimes - cdata->queryExtraData(char_data_type_daily, char_data_daily_treasure);
    if (left >= 0)
    {
        return left;
    }
    else
    {
        return 0;
    }
}

void char_treasure::save()
{
    time_t start_time = 0, end_time = 0;
    int can_rob_time = 0, silver = 0;
    if (m_state == 1 && m_treasure.get())
    {
        start_time = m_treasure->m_start_time;
        end_time = m_treasure->m_end_time;
        can_rob_time = m_treasure->m_rob_time;
        silver = m_treasure->m_silver;
    }
    InsertSaveDb("update char_treasure set tid=" + LEX_CAST_STR(m_tid)
        + ",state=" + LEX_CAST_STR(m_state)
        + ",refresh_times=" + LEX_CAST_STR(m_refresh_times)
        + ",start_time=" + LEX_CAST_STR(start_time)
        + ",end_time=" + LEX_CAST_STR(end_time)
        + ",can_be_rob_time=" + LEX_CAST_STR(can_rob_time)
        + ",silver=" + LEX_CAST_STR(silver)
        + ",x=" + LEX_CAST_STR(m_x)
        + ",y=" + LEX_CAST_STR(m_y)
        + " where cid=" + LEX_CAST_STR(m_cid));
}

void char_treasure::reset()
{
    m_state = 0;
    m_x = 0;
    m_y = 0;
    m_tid = 1;
    //VIP6从蓝色品质开始送
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (cdata.get())
    {
        if (cdata->m_vip >= iTreasurePurpleVip && m_tid < 3)
        {
            m_tid = 3;
        }
    }
    m_refresh_times = 0;
    m_treasure.reset();
}

treasureMgr::treasureMgr()
{
    reload();
}

int treasureMgr::reload()
{
    Query q(GetDb());
    q.get_result("SELECT id,name,need_min,silver FROM base_treasure WHERE 1");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        boost::shared_ptr<base_treasures> pbg;
        pbg.reset(new base_treasures);
        int id = q.getval();
        pbg->name = q.getstr();
        pbg->need_min = q.getval();
        pbg->silver = q.getval();
        pbg->color_name = pbg->name;
        addColor(pbg->color_name, id);
        //掉落加载
        Singleton<lootMgr>::Instance().getTreasureLootInfo(id, pbg->m_Item_list);
        m_base_treasures[id] = pbg;
    }
    q.free_result();
    //玩家藏宝图
    q.get_result("select cid, tid, state, refresh_times, start_time, end_time, can_be_rob_time, silver, x, y from char_treasure where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<char_treasure> pcg;
        pcg.reset(new char_treasure);
        int cid = q.getval();
        pcg->m_cid = cid;
        pcg->m_tid = q.getval();
        pcg->m_state = q.getval();
        pcg->m_refresh_times = q.getval();
        time_t start_time = q.getval();
        time_t end_time = q.getval();
        int can_rob_time = q.getval();
        int silver = q.getval();
        pcg->m_x = q.getval();
        pcg->m_y = q.getval();
        if (pcg->m_state == 1)
        {
            boost::shared_ptr<treasure> pg;
            pg.reset(new treasure(cid,pcg->m_tid));
            pg->m_rob_time = can_rob_time;
            pg->m_start_time = start_time;
            pg->m_end_time = end_time;
            pg->m_silver = silver;
            pg->m_needmin = (end_time - start_time) / 60;
            pcg->m_treasure = pg;
            pcg->m_treasure->start();
        }
        m_char_treasures[cid] = pcg;
    }
    q.free_result();
    iTreasureRobTimes = GeneralDataMgr::getInstance()->getInt("treasure_rob_times", iDefaultTreasureRobTimes);
    iTreasureTimes = GeneralDataMgr::getInstance()->getInt("treasure_times", iDefaultTreasureTimes);
    return 0;
}

void treasureMgr::getButton(CharData* pc, json_spirit::Array& list)
{
    if (pc->isTreasureOpen())
    {
        boost::shared_ptr<char_treasure> pct = getCharTreasure(pc->m_id);
        if (pct.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("type", top_button_treasure) );
            obj.push_back( Pair("active", 0) );
            obj.push_back( Pair("leftNums", pct->getCanStartTimes()));
            list.push_back(obj);
        }
    }
}

int treasureMgr::getinsence(int cid)
{
    m_uid_list[cid] = 1;
    return 0;
}

int treasureMgr::getoutsence(int cid)
{
    m_uid_list.erase(cid);
    return 0;
}

int treasureMgr::start(int cid)
{
    boost::shared_ptr<char_treasure> pcg = getCharTreasure(cid);
    if (pcg.get() && pcg->m_state == 0 && pcg->getCanStartTimes() > 0)
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cdata.get())
        {
            return HC_ERROR;
        }
        //VIP6从蓝色品质开始送
        if (cdata->m_vip >= iTreasurePurpleVip && pcg->m_tid < 3)
        {
            pcg->m_tid = 3;
        }
        boost::shared_ptr<treasure> pg;
        pg.reset(new treasure(pcg->m_cid, pcg->m_tid));
        boost::shared_ptr<base_treasures> pbg = getBaseTreasure(pcg->m_tid);
        if (pbg.get())
        {
            pg->m_silver = pbg->silver;
            pg->m_needmin = pbg->need_min;
            pg->m_end_time = pg->m_start_time + pbg->need_min * 60;
        }
        pcg->m_treasure = pg;
        pcg->m_treasure->start();
        int times = cdata->queryExtraData(char_data_type_daily, char_data_daily_treasure);
        cdata->setExtraData(char_data_type_daily, char_data_daily_treasure, ++times);
        cdata->m_tasks.updateTask(GOAL_TREASURE, 0, 1);
        cdata->m_tasks.updateTask(GOAL_DAILY_TREASURE, 0, 1);
        Singleton<goalMgr>::Instance().updateTask(cdata->m_id, GOAL_TYPE_TREASURE_QUALITY, pg->m_tid);
        cdata->updateTopButton(top_button_treasure, 0, pcg->getCanStartTimes());
        pcg->m_state = 1;
        pcg->m_x = my_random(iTreasurePosX[0],iTreasurePosX[1]);
        pcg->m_y = my_random(iTreasurePosY[0],iTreasurePosY[1]);
        pcg->save();
        //系统公告
        std::string notify_msg = "";
        if (pg->m_tid == TREASURE_RED)
        {
            notify_msg = strTreasureRed;
            str_replace(notify_msg, "$N", MakeCharNameLink(cdata->m_name,cdata->m_nick.get_string()));
        }
        else if(pg->m_tid == TREASURE_ORANGE)
        {
            notify_msg = strTreasureOrange;
            str_replace(notify_msg, "$N", MakeCharNameLink(cdata->m_name,cdata->m_nick.get_string()));
        }
        if (notify_msg != "")
        {
            GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
        }
        //广播好友祝贺
        if (pg->m_tid == TREASURE_ORANGE)
        {
            Singleton<relationMgr>::Instance().postCongradulation(cdata->m_id, CONGRATULATION_TREASURE_QUALITY5, 0, 0);
        }
        else if (pg->m_tid == TREASURE_RED)
        {
            Singleton<relationMgr>::Instance().postCongradulation(cdata->m_id, CONGRATULATION_TREASURE_QUALITY6, 0, 0);
        }
        //广播给当前界面的玩家
        json_spirit::Object obj;
        obj.push_back( Pair ("cmd", "addTreasureQueue"));
        json_spirit::Array list;
        json_spirit::Object o;
        o.push_back( Pair ("id", pcg->m_cid));
        o.push_back( Pair ("name", cdata->m_name));
        if (pbg.get())
        {
            o.push_back( Pair ("leftTime", pcg->m_treasure->m_end_time - time(NULL)));
            o.push_back( Pair("quality", pcg->m_tid) );
            o.push_back( Pair("x", pcg->m_x) );
            o.push_back( Pair("y", pcg->m_y) );
        }
        list.push_back(o);
        obj.push_back( Pair ("list", list));
        obj.push_back( Pair ("s", 200));
        INFO("broadcast start new treasure!!!!" << endl);
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
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int treasureMgr::finish(int cid)
{
    boost::shared_ptr<char_treasure> pcg = getCharTreasure(cid);
    if (pcg.get() && pcg->m_state == 1 && pcg->m_treasure.get())
    {
        boost::shared_ptr<CharData> cd = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cd.get())
            return HC_ERROR;
        int left_second = pcg->m_treasure->m_end_time - time(NULL);
        if (left_second > 0)
        {
            int fac = left_second / 60;
            fac += left_second % 60 == 0 ? 0 : 1;
            if (cd->subGold(iTreasureFinishGold * fac, gold_cost_treasure_finish) < 0)
                return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        pcg->m_treasure->finish();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int treasureMgr::combatResult(chessCombat* pCombat)
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (COMBAT_TYPE_TREASURE_ROB != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<char_treasure> pcg_atk = getCharTreasure(pCombat->m_players[0].m_cid);
    boost::shared_ptr<char_treasure> pcg_def = getCharTreasure(pCombat->m_players[1].m_cid);
    if (!pcg_atk.get() || !pcg_def.get())
        return HC_ERROR;
    boost::shared_ptr<CharData> pcd_atk = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_players[0].m_cid);
    if (!pcd_atk.get())
        return HC_ERROR;
    pcd_atk->m_tasks.updateTask(GOAL_DAILY_TREASURE_ROB, 0, 1);
    if (pCombat->m_result == COMBAT_RESULT_ATTACK_WIN)
    {
        int silver = 0;
        if (-1 == robReward(pCombat->m_players[0].m_cid, pCombat->m_players[1].m_cid, silver))
        {
            //打劫宝物已经结束发送信件
            std::string content = strTreasureRobLateMailContent;
            str_replace(content, "$A", pCombat->m_players[1].m_player_name);
            sendSystemMail(pCombat->m_players[0].m_player_name, pCombat->m_players[0].m_cid, strTreasureMailTitle, content, "", pCombat->m_combat_id);
            return HC_SUCCESS;
        }

        //银币
        Item item_silver(ITEM_TYPE_CURRENCY, CURRENCY_ID_SILVER, silver, 0);
        pCombat->m_getItems.push_back(item_silver);

        //给东西
        giveLoots(pcd_atk, pCombat, true, loot_treasure);

        int rob = pcd_atk->queryExtraData(char_data_type_daily, char_data_daily_treasure_rob);
        pcd_atk->setExtraData(char_data_type_daily, char_data_daily_treasure_rob, ++rob);
        //保存被打劫后的信息
        (pcg_def->m_treasure->m_silver) -= silver;
        pcg_def->save();

        //记录到信息
        boost::shared_ptr<event_log> p;
        p.reset(new event_log());
        p->m_atk_id = pCombat->m_players[0].m_cid;
        p->m_def_id = pCombat->m_players[1].m_cid;
        p->m_tid = pcg_def->m_tid;
        p->m_silver = silver;
        if (m_event_log.size() >= 4)
            m_event_log.pop_front();
        m_event_log.push_back(p);
        //广播给其他人
        broadRobEvent();
        //发送信件
        std::string content = strTreasureRobSucMailContent;
        boost::shared_ptr<base_treasures> pbg = getBaseTreasure(pcg_def->m_tid);
        if (pbg.get())
        {
            str_replace(content, "$N", pbg->color_name);
        }
        str_replace(content, "$A", pcd_atk->m_name);
        str_replace(content, "$S", LEX_CAST_STR(silver));
        sendSystemMail(pCombat->m_players[1].m_player_name, pCombat->m_players[1].m_cid, strTreasureMailTitle, content, "", pCombat->m_combat_id);
    }
    else if(pCombat->m_result == COMBAT_RESULT_ATTACK_LOSE)
    {
        int rob = pcd_atk->queryExtraData(char_data_type_daily, char_data_daily_treasure_rob);
        pcd_atk->setExtraData(char_data_type_daily, char_data_daily_treasure_rob, ++rob);

        //发送信件
        std::string content = strTreasureRobFailMailContent;
        boost::shared_ptr<base_treasures> pbg = getBaseTreasure(pcg_def->m_tid);
        if (pbg.get())
        {
            str_replace(content, "$N", pbg->color_name);
        }
        str_replace(content, "$A", pcd_atk->m_name);
        sendSystemMail(pCombat->m_players[1].m_player_name, pCombat->m_players[1].m_cid, strTreasureMailTitle, content, "", pCombat->m_combat_id);
    }
    return HC_SUCCESS;
}

int treasureMgr::robReward(int atk_id, int def_id, int& silver)
{
    boost::shared_ptr<CharData> pcd_atk = GeneralDataMgr::getInstance()->GetCharData(atk_id);
    boost::shared_ptr<CharData> pcd_def = GeneralDataMgr::getInstance()->GetCharData(def_id);
    boost::shared_ptr<char_treasure> pcg_def = getCharTreasure(def_id);
    if (!pcd_atk.get() || !pcd_def.get() || !pcg_def.get() || !pcg_def->m_treasure.get())
        return -1;
    boost::shared_ptr<base_treasures> pbg = getBaseTreasure(pcg_def->m_treasure->m_tid);
    if (!pbg.get())
        return -1;
    /*基础劫20%，根据等级差银币每级修正2%*/
    int fac = pcd_def->m_level - pcd_atk->m_level;
    silver = pbg->silver * (20 + fac * 2) / 100;
    if (silver < (pbg->silver / 10))
        silver = (pbg->silver / 10);
    return 0;
}

int treasureMgr::jobDone(int cid)
{
    boost::shared_ptr<char_treasure> pcg = getCharTreasure(cid);
    if (pcg.get() && pcg->m_state == 1 && pcg->m_treasure.get())
    {
        boost::shared_ptr<CharData> cd = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cd.get())
            return HC_ERROR;
        /***********随机获得掉落处理****************/
        std::list<Item> items;
        Item tmp(ITEM_TYPE_CURRENCY, CURRENCY_ID_SILVER, pcg->m_treasure->m_silver, 0);
        items.push_back(tmp);
        Singleton<lootMgr>::Instance().getTreasureLoots(pcg->m_tid, items);
        std::string attack_info = itemlistToAttach(items);
        //notify result,send mail
        std::string content = strTreasureMailContent;
        boost::shared_ptr<base_treasures> pbg = getBaseTreasure(pcg->m_tid);
        if (pbg.get())
        {
            str_replace(content, "$N", pbg->color_name);
        }
        str_replace(content, "$R", itemlistToString(items));
        sendSystemMail(cd->m_name, cid, strTreasureMailTitle, content, attack_info, 0, 0, loot_treasure);
        pcg->reset();
        pcg->save();
        //广播给当前界面的玩家
        json_spirit::Object obj;
        obj.push_back( Pair ("cmd", "removeTreasureQueue"));
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
                }
            }
            ++it;
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int treasureMgr::getList(int cid, json_spirit::Object& robj)
{
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<char_treasure> >::iterator it = m_char_treasures.begin();
    while (it != m_char_treasures.end())
    {
        if (it->second.get() && it->second->m_state == 1 && it->second->m_treasure.get())
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
            o.push_back( Pair("leftTime", it->second->m_treasure->m_end_time - time(NULL)) );
            o.push_back( Pair("quality", it->second->m_treasure->m_tid) );
            o.push_back( Pair("x", it->second->m_x) );
            o.push_back( Pair("y", it->second->m_y) );
            list.push_back(o);
        }
        else
        {
            INFO("cid" << it->first << " is not doing" << endl);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int treasureMgr::getInfo(int cid, int id, json_spirit::Object& robj)
{
    json_spirit::Object info;
    boost::shared_ptr<char_treasure> pcg = getCharTreasure(id);
    if (pcg.get() && pcg->m_state == 1 && pcg->m_treasure.get())
    {
        int silver = 0;
        boost::shared_ptr<CharData> be_rob_pcd = GeneralDataMgr::getInstance()->GetCharData(id);
        if (!be_rob_pcd.get())
            return HC_ERROR;
        info.push_back( Pair("id", id) );
        info.push_back( Pair("name", be_rob_pcd->m_name) );
        info.push_back( Pair("level", be_rob_pcd->m_level) );
        if (be_rob_pcd->GetGuildId())
        {
            info.push_back( Pair("guild", Singleton<guildMgr>::Instance().getGuildName(be_rob_pcd->GetGuildId())) );
        }
        else
        {
            info.push_back( Pair("guild", "") );
        }
        if (cid == id)//查看自己
        {
            info.push_back( Pair("silver", pcg->m_treasure->m_silver) );
        }
        else if(robReward(cid, id, silver) == 0)
        {
            info.push_back( Pair("silver", silver) );
        }
        else
        {
            return HC_ERROR;
        }
        info.push_back( Pair("leftTime", pcg->m_treasure->m_end_time - time(NULL)) );
        json_spirit::Object goodsVO;
        goodsVO.push_back( Pair("robTimes", pcg->m_treasure->m_rob_time) );
        goodsVO.push_back( Pair("robTimesTotal", 2) );
        goodsVO.push_back( Pair("quality", pcg->m_treasure->m_tid) );
        boost::shared_ptr<base_treasures> pbg = getBaseTreasure(pcg->m_treasure->m_tid);
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

int treasureMgr::broadRobEvent()
{
    json_spirit::Object robj;
    json_spirit::Array list;
    std::list<boost::shared_ptr<event_log> >::iterator it = m_event_log.begin();
    while (it != m_event_log.end())
    {
        if ((*it).get())
        {
            json_spirit::Object o;
            std::string msg = strTreasureRobLog;
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
            boost::shared_ptr<base_treasures> pbg = getBaseTreasure((*it)->m_tid);
            if (pbg.get())
            {
                str_replace(msg, "$N", pbg->color_name);
                str_replace(msg, "$S", LEX_CAST_STR((*it)->m_silver));
            }
            o.push_back( Pair("msg", msg) );
            list.push_back(o);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("cmd", "getRobEvent") );
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

int treasureMgr::getRobEvent(json_spirit::Object& robj)
{
    json_spirit::Array list;
    std::list<boost::shared_ptr<event_log> >::iterator it = m_event_log.begin();
    while (it != m_event_log.end())
    {
        if ((*it).get())
        {
            json_spirit::Object o;
            std::string msg = strTreasureRobLog;
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
            boost::shared_ptr<base_treasures> pbg = getBaseTreasure((*it)->m_tid);
            if (pbg.get())
            {
                str_replace(msg, "$N", pbg->color_name);
                str_replace(msg, "$S", LEX_CAST_STR((*it)->m_silver));
            }
            o.push_back( Pair("msg", msg) );
            list.push_back(o);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int treasureMgr::getTreasureInfo(int tid, json_spirit::Object& robj)
{
    boost::shared_ptr<base_treasures> pbg = getBaseTreasure(tid);
    if (pbg.get())
    {
        json_spirit::Object o;
        o.push_back( Pair("id", tid) );
        o.push_back( Pair("spic", tid) );
        o.push_back( Pair("quality", tid) );
        o.push_back( Pair("name", pbg->name) );
        o.push_back( Pair("totalTime", pbg->need_min) );
        o.push_back( Pair("silver", pbg->silver) );
        //loot
        json_spirit::Array get_list;
        itemlistToArray(pbg->m_Item_list, get_list);
        o.push_back( Pair("get_list", get_list) );
        robj.push_back( Pair("info", o) );

    }
    return HC_SUCCESS;
}

int treasureMgr::getTreasureList(CharData* pc, json_spirit::Object& robj)
{
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<base_treasures> >::iterator it = m_base_treasures.begin();
    while (it != m_base_treasures.end())
    {
        if (it->second.get())
        {
            json_spirit::Object o;
            o.push_back( Pair("id", it->first) );
            o.push_back( Pair("spic", it->first) );
            o.push_back( Pair("quality", it->first) );
            o.push_back( Pair("name", it->second->name) );
            o.push_back( Pair("totalTime", it->second->need_min) );
            o.push_back( Pair("silver", it->second->silver) );
            //loot
            json_spirit::Array get_list;
            itemlistToArray(it->second->m_Item_list, get_list);
            o.push_back( Pair("get_list", get_list) );
            list.push_back(o);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    json_spirit::Object info;
    boost::shared_ptr<char_treasure> pcg = getCharTreasure(pc->m_id);
    if (pcg.get())
    {
        info.push_back( Pair("refreshGold", iTreasureRefreshGold) );
        info.push_back( Pair("refreshGoldTimes", iTreasureRefreshGoldTimes[pc->m_vip]+iTreasureRefreshSilverTimes-pcg->m_refresh_times) );
        info.push_back( Pair("refreshSilver", iTreasureRefreshSilver) );
        info.push_back( Pair("refreshSilverTimes", iTreasureRefreshSilverTimes-pcg->m_refresh_times) );
        info.push_back( Pair("callGold", iTreasureCallGold) );
        info.push_back( Pair("callVip", iTreasureCallVip) );
    }
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

int treasureMgr::getCharTreasureInfo(CharData* pc, json_spirit::Object& robj)
{
    json_spirit::Object info;
    boost::shared_ptr<char_treasure> pcg = getCharTreasure(pc->m_id);
    if (pcg.get())
    {
        if (pcg->m_state == 1 && pcg->m_treasure.get())
            info.push_back( Pair("leftTime", pcg->m_treasure->m_end_time - time(NULL)) );
        info.push_back( Pair("startTimesLeft", pcg->getCanStartTimes()) );
        info.push_back( Pair("robTimesLeft", pcg->getCanRobTimes()) );
        //VIP6从蓝色品质开始送
        if (pc->m_vip >= iTreasurePurpleVip && pcg->m_tid < 3)
        {
            pcg->m_tid = 3;
        }
        boost::shared_ptr<base_treasures> pbg = getBaseTreasure(pcg->m_tid);
        if (pbg.get())
        {
            json_spirit::Object goodsVO;
            goodsVO.push_back( Pair("id", pcg->m_tid) );
            goodsVO.push_back( Pair("spic", pcg->m_tid) );
            goodsVO.push_back( Pair("quality", pcg->m_tid) );
            goodsVO.push_back( Pair("name", pbg->name) );
            goodsVO.push_back( Pair("totalTime", pbg->need_min) );
            goodsVO.push_back( Pair("silver", pbg->silver) );
            info.push_back( Pair("goodsVO", goodsVO) );
        }
        robj.push_back( Pair("info", info) );
        getinsence(pc->m_id);
    }
    return HC_SUCCESS;
}

int treasureMgr::refresh(CharData* pc, int type, json_spirit::Object& robj)
{
    boost::shared_ptr<char_treasure> pcg = getCharTreasure(pc->m_id);
    if (NULL == pcg.get() || pcg->m_state != 0 || pcg->m_tid >= TREASURE_MAX)
    {
        return HC_ERROR;
    }
    bool update = false;
    if (type == 1)
    {
        //筹码刷新
        if (pcg->m_refresh_times >= iTreasureRefreshSilverTimes)
        {
            return HC_ERROR;
        }
        if (pc->subSilver(iTreasureRefreshSilver, silver_cost_treasure_refresh) < 0)
            return HC_ERROR_NOT_ENOUGH_SILVER;
        if (pcg->m_tid > 0 && pcg->m_tid < TREASURE_MAX)
        {
            //第一次刷新特殊处理
            int times = pc->queryExtraData(char_data_type_normal,char_data_normal_first_refresh_treasure);
            if (times < 5)
            {
                update = true;
                ++(pcg->m_tid);
                pc->setExtraData(char_data_type_normal,char_data_normal_first_refresh_treasure,++times);
            }
            else if (my_random(0,99) < iTreasureRefreshPer[pcg->m_tid - 1])
            {
                update = true;
                ++(pcg->m_tid);
            }
        }
        ++pcg->m_refresh_times;
    }
    else if (type == 2)
    {
        //点券刷新
        if (pcg->m_refresh_times < iTreasureRefreshSilverTimes)
        {
            return HC_ERROR;
        }
        if (pcg->m_refresh_times >= iTreasureRefreshGoldTimes[pc->m_vip]+iTreasureRefreshSilverTimes)
        {
            return HC_ERROR;
        }
        if (pc->subGold(iTreasureRefreshGold, gold_cost_treasure_refresh) < 0)
            return HC_ERROR_NOT_ENOUGH_GOLD;
        if (pcg->m_tid > 0 && pcg->m_tid < TREASURE_MAX)
        {
            if (my_random(0,99) < iTreasureRefreshPer[pcg->m_tid - 1])
            {
                update = true;
                ++(pcg->m_tid);
            }
        }
        ++pcg->m_refresh_times;
    }
    else if (type == 4)
    {
        if (pc->m_vip < iTreasureCallVip)
            return HC_ERROR_NEED_MORE_VIP;
        if (pc->subGold(iTreasureCallGold, gold_cost_treasure_call) < 0)
            return HC_ERROR_NOT_ENOUGH_GOLD;
        pcg->m_tid = TREASURE_RED;
        update = true;
    }
    pcg->save();
    robj.push_back( Pair("update", update?1:0) );
    return HC_SUCCESS;
}

boost::shared_ptr<char_treasure> treasureMgr::getCharTreasure(int cid)
{
    INFO("get char treasure" << endl);
    std::map<int, boost::shared_ptr<char_treasure> >::iterator it = m_char_treasures.find(cid);
    if (it != m_char_treasures.end())
    {
        return it->second;
    }
    else
    {
        Query q(GetDb());
        boost::shared_ptr<char_treasure> pcg;
        pcg.reset(new char_treasure);
        q.get_result("select cid, tid, state, refresh_times, start_time, end_time, can_be_rob_time, silver, x, y from char_treasure where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            pcg->m_cid = cid;
            pcg->m_tid = q.getval();
            pcg->m_state = q.getval();
            pcg->m_refresh_times = q.getval();
            time_t start_time = q.getval();
            time_t end_time = q.getval();
            int can_rob_time = q.getval();
            int silver = q.getval();
            pcg->m_x = q.getval();
            pcg->m_y = q.getval();
            boost::shared_ptr<treasure> pg;
            pg.reset();
            if (pcg->m_state == 1)
            {
                pg.reset(new treasure(cid,pcg->m_tid));
                pg->m_rob_time = can_rob_time;
                pg->m_start_time = start_time;
                pg->m_end_time = end_time;
                pg->m_silver = silver;
                pg->m_needmin = (end_time - start_time) / 60;
                pcg->m_treasure = pg;
                pcg->m_treasure->start();
            }
            m_char_treasures[cid] = pcg;
            q.free_result();
        }
        else
        {
            q.free_result();
            pcg.reset(new char_treasure);
            pcg->m_cid = cid;
            pcg->reset();
            m_char_treasures[cid] = pcg;
            InsertSaveDb("insert into char_treasure set cid=" + LEX_CAST_STR(cid));
        }
        return pcg;
    }
}

boost::shared_ptr<base_treasures> treasureMgr::getBaseTreasure(int tid)
{
    std::map<int, boost::shared_ptr<base_treasures> >::iterator it = m_base_treasures.find(tid);
    if (it != m_base_treasures.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<base_treasures> pbg;
        pbg.reset();
        return pbg;
    }
}

