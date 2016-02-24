
#include "stdafx.h"

#include "data.h"
#include "combat_def.h"
#include <iostream>
#include <math.h>

#include "utils_all.h"
#include "new_trade.h"

#ifndef _WINDOWS
#include <sys/syscall.h>
#endif

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>
#include "spls_errcode.h"
#include "utils_lang.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "combat_attr.h"
#include "equipt_upgrade.h"
#include "explore.h"
#include "spls_timer.h"
#include "spls_race.h"
#include "net.h"
#include "boss.h"
#include "campRace.h"
#include "guard.h"
#include "servant.h"
#include "md5.h"
#include "groupCopy.h"
#include "db_thread.h"
#include "statistics.h"
#include <pthread.h>
#include "daily_task.h"
#include "recharge_event.h"
#include "online_gift.h"
#include "shhx_event.h"
#include "singleton.h"
#include "equipment_make.h"
#include "newbaoshi.h"
#include "packs.h"

#include "json_spirit_writer_template.h"

#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "mails.h"
#include "SaveDb.h"
#include "libao.h"
#include "relation.h"
#include "new_ranking.h"
#include "bank.h"
#include "new_event.h"
#include "first_seven_goals.h"
#include "corpsExplore.h"
#include "kingnet_analyzer.h"

#include "cost_feedback.h"
#include "qq_invite.h"

#include "eliteCombat.h"

#include "char_jxl.h"
#include "throne.h"
#include "corpsFighting.hpp"

#include "char_general_soul.hpp"

#include "admin_cmds.h"

using namespace std;
using namespace net;

boost::unordered_map<uint64_t, Database*> g_dbs;

extern pthread_key_t thread_db_key;
extern void InsertMailcmd(mailCmd& pmailCmd);
extern void InsertSaveDb(const std::string& sql);
extern void InsertSaveDb(const saveDbJob& job);

extern int InsertActionWork(actionmessage& msg);
extern int InsertInternalActionWork(json_spirit::mObject& obj);

extern int getAction(CharData* cdata, json_spirit::Array& elist);

extern void updateGlobalFac(int season);
extern int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
extern void InsertDbCharCmd(dbCmd& _dbCmd);

extern bool canUpgradeEquipment(equipment_scroll* sp, Bag& m_bag);

#define INFO(x) //cout<<x<<endl

const char szChargeResponseSuccess[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nConnection: close\r\nContent-length: 3\r\nServer: lighttpd/1.4.26\r\n\r\n200";
const char szChargeResponseNotFound[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nConnection: close\r\nContent-length: 3\r\nServer: lighttpd/1.4.26\r\n\r\n404";
const char szChargeResponseAuthFail[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nConnection: close\r\nContent-length: 3\r\nServer: lighttpd/1.4.26\r\n\r\n401";
const char szChargeResponseServerError[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nConnection: close\r\nContent-length: 3\r\nServer: lighttpd/1.4.26\r\n\r\n500";

extern std::string strRecommendFriends;
extern std::string strRecommendFriendsContent;

//地图开放
extern volatile int gMaxOpenMap;

#define DEFAULT_GOLD 10
#define DEFAULT_SILVER 10000
#define DEFAULT_LING 180
#define DEFAULT_SUPPLY 150

static volatile int g_default_gold = DEFAULT_GOLD;
static volatile int g_default_silver = DEFAULT_SILVER;
static volatile int g_default_ling = DEFAULT_LING;
static volatile int g_default_supply = DEFAULT_SUPPLY;

static volatile int g_tmpvip_secs = iTmpVip_time;
volatile int g_sweep_fast_vip = iSweepEliteFinishVip;

volatile int g_wash_discount = 100;        //洗髓活动折扣
volatile int g_wash_real_cost[5];

volatile int g_shop_discount = 100;        //商店折扣

//战马训练次数
extern volatile int iHorseTrainTime;        //每天培养战马次数
extern volatile int iHorseGoldTrainTime;    //每天金币培养战马次数

//战马训练折扣
extern volatile int iHorseTrainDiscount;

//重生打折活动
volatile int g_reborn_discount = 100;

volatile int g_auth_type = 0;    // 0:md5 , 1:ticket

extern volatile int g_enable_debug_cmd_line;

//测试充值类型 0禁止 1允许每天一次 2无限制
extern volatile int gTestRechargeType;

//购买军令图标闪烁
const std::string strRestNotify = "{\"active\":1,\"cmd\":\"updateRest\",\"s\":200}";
const std::string strRestUnNotify = "{\"active\":0,\"cmd\":\"updateRest\",\"s\":200}";

//武将链接
const std::string strGeneralLink = "<A HREF=\"event:{'id':$G,'cmd':'showGeneral','cid':$C}\" TARGET=\"\"><U>$N</U></A>";

const std::string strNotifyLevelup = "{\"cmd\":\"nchange\",\"list\":[{\"id\":$C,\"online\":1,\"name\":\"$N\",\"gender\":$G,\"level\":$L}],\"s\":200}";
const std::string strNotifyOffline = "{\"cmd\":\"nchange\",\"list\":[{\"id\":$C,\"online\":0}],\"s\":200}";
const std::string strNotifyOnline = "{\"cmd\":\"nchange\",\"list\":[{\"id\":$C,\"online\":1,\"name\":\"$N\",\"gender\":$G,\"level\":$L}],\"s\":200}";
const std::string strNotifyEnter = "{\"cmd\":\"enter\",\"id\":$C,\"level\":$L,\"name\":\"$N\",\"gender\":$G,\"online\":1,\"s\":200}";
const std::string strNotifyLeave = "{\"cmd\":\"leave\",\"id\":$C,\"level\":$L,\"s\":200}";
const std::string strNotifyPresentGet = "{\"cmd\":\"present_get\",\"s\":200}";

#define CHANGE_MALL_DISCOUNT "change_mall_discount"

volatile int g_get_char_id = 0;

//军团收益系数
static int gCorpsFactor = 100;
//军团实际收益
void corpsRealReward(int& get)
{
    if (gCorpsFactor > 100)
    {
        get = get * gCorpsFactor/100;
    }
}

//贸易收益系数
static int gTradeFactor = 100;
//贸易实际收益
void tradeRealReward(int& get)
{
    if (gTradeFactor > 100)
    {
        get = get * gTradeFactor/100;
    }
}

//八卦阵收益系数
static int gMazeFactor = 100;
//八卦阵实际收益
void mazeRealReward(int& get)
{
    if (gMazeFactor > 100)
    {
        get = get * gMazeFactor/100;
    }
}

//阵营战收益系数
static int gCampRaceFactor = 100;
//阵营战实际收益
void campRaceRealReward(int& get)
{
    if (gCampRaceFactor>100)
    {
        get = get*gCampRaceFactor/100;
    }
}

//竞技场收益系数
static int gArenaFactor = 100;
//竞技场实际收益
void arenaRealReward(int& get)
{
    if (gArenaFactor>100)
    {
        get = get*gArenaFactor/100;
    }
}

//屯田收益系数
static int gFarmFactor = 100;
//屯田实际收益
void farmRealReward(int& get)
{
    if (gFarmFactor>100)
    {
        get = get*gFarmFactor/100;
    }
}

//神兽实际收益
static int gBossFactor = 100;
void bossRealReward(int& get)
{
    if (gBossFactor>100)
    {
        get = get*gBossFactor/100;
    }
}

void setRewardFactor(int type, int factor)
{
    switch (type)
    {
        //设置军团收益系数
        case admin_set_corpsFactor:
        {
            gCorpsFactor = factor;
            GeneralDataMgr::getInstance()->setInt("corpsFactor", factor);
            break;
        }
        //设置贸易收益系数
        case admin_set_tradeFactor:
        {
            gTradeFactor = factor;
            GeneralDataMgr::getInstance()->setInt("tradeFactor", factor);
            break;
        }
        //设置八卦阵收益系数
        case admin_set_mazeFactor:
        {
            gMazeFactor = factor;
            GeneralDataMgr::getInstance()->setInt("mazeFactor", factor);
            break;
        }
        //设置阵营战收益系数
        case admin_set_campRaceFactor:
        {
            gCampRaceFactor = factor;
            GeneralDataMgr::getInstance()->setInt("campRaceFactor", factor);
            break;
        }
        //设置竞技场收益系数
        case admin_set_arenaFactor:
        {
            gArenaFactor = factor;
            GeneralDataMgr::getInstance()->setInt("arenaFactor", factor);
            break;
        }
        //设置屯田收益系数
        case admin_set_farmFactor:
        {
            gFarmFactor = factor;
            GeneralDataMgr::getInstance()->setInt("farmFactor", factor);
            break;
        }
        //设置神兽实际收益
        case admin_set_bossFactor:
        {
            gBossFactor = factor;
            GeneralDataMgr::getInstance()->setInt("bossFactor", factor);
            break;
        }
    }
}

void close_thread_db(void* db)
{
    Database* pdb = reinterpret_cast<Database*>(db);
    if (pdb)
    {
        delete pdb;
    }
}

int getSessionChar(net::session_ptr& psession, CharData* &pc)
{
    pc = NULL;
    //账号登录状态
    boost::shared_ptr<OnlineUser> account = psession->user();
    if (!account.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!account->m_onlineCharactor.get() || !account->m_onlineCharactor->m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    pc = account->m_onlineCharactor->m_charactor.get();
    return HC_SUCCESS;
}

int getSessionChar(net::session_ptr& psession, boost::shared_ptr<CharData>& cdata)
{
    //账号登录状态
    boost::shared_ptr<OnlineUser> account = psession->user();
    if (!account.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!account->m_onlineCharactor.get() || !account->m_onlineCharactor->m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    cdata = account->m_onlineCharactor->m_charactor;
    return HC_SUCCESS;
}

Database& GetDb2()
{
    bool keepalive = true;
    uint64_t tid = syscall(SYS_gettid);
    if (g_dbs[tid] == 0)
    {
        g_dbs[tid] = new Database("localhost", "c_user", "23rf234", GetDbName(), mysqlOption(), mysqlPort());//, CLIENT_INTERACTIVE);
        cout<<"new db:"<<g_dbs[tid]<<",tid"<<tid<<endl;
    }
    if (keepalive)
    {
        g_dbs[tid]->KeepAlive();
        int retry = 0;
        while (g_dbs[tid]->Connected() == false)
        {
            cout<<"thread "<<tid<<",db disconnected."<<endl;
            delete g_dbs[tid];
            g_dbs[tid] = new Database("localhost", "c_user", "23rf234", GetDbName(), mysqlOption(), mysqlPort());//, CLIENT_INTERACTIVE);
            cout<<"new db:"<<g_dbs[tid]<<",tid"<<tid<<endl;
            g_dbs[tid]->KeepAlive();
            ++retry;
            if (retry > 3)
            {
                do_sleep(2000);
            }
        }
    }
    return *g_dbs[tid];
}

Database& GetDb()
{
#if 1
    bool keepalive = true;
    uint64_t tid = syscall(SYS_gettid);
    Database* pdb = reinterpret_cast<Database*>(pthread_getspecific (thread_db_key));
    if (!pdb)
    {
        pdb = new Database("localhost", "c_user", "23rf234", GetDbName(), mysqlOption(), mysqlPort());//, CLIENT_INTERACTIVE);
        pthread_setspecific (thread_db_key, pdb);
        cout<<"new database:"<<pdb<<",tid"<<tid<<endl;
    }
    if (keepalive)
    {
        pdb->KeepAlive();
        int retry = 0;
        while (pdb->Connected() == false)
        {
            cout<<"thread "<<tid<<",db disconnected."<<endl;
            delete pdb;
            pdb = new Database("localhost", "c_user", "23rf234", GetDbName(), mysqlOption(), mysqlPort());//, CLIENT_INTERACTIVE);
            pthread_setspecific (thread_db_key, pdb);
            cout<<"new db:"<<pdb<<",tid"<<tid<<endl;
            pdb->KeepAlive();
            ++retry;
            if (retry > 3)
            {
                do_sleep(2000);
            }
        }
    }
    return *pdb;
#else
    static Database::Mutex mutex_;
    static Database db(mutex_, "localhost", "c_user", "23rf234", GetDbName(), mysqlOption(), mysqlPort());
    return db;
#endif
}

//檢查玩家身上是否有重複的武將
void checkCharGenerals()
{
    Query q(GetDb());
    std::list<int> c_list;
    std::list<int> g_list;
    std::list<int> n_list;

    q.get_result("SELECT count(*),cid, gid FROM char_generals WHERE state =0 GROUP BY cid, gid HAVING COUNT( cid ) >=2");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        n_list.push_back(q.getval());
        c_list.push_back(q.getval());
        g_list.push_back(q.getval());
    }
    q.free_result();

    std::list<int> change_list;
    std::list<int>::iterator itc = c_list.begin();
    std::list<int>::iterator itg = g_list.begin();
    std::list<int>::iterator itn = n_list.begin();
    while (itc != c_list.end() && itg != g_list.end() && itn != n_list.end())
    {
        q.get_result("select id from char_generals where state=0 and cid=" + LEX_CAST_STR(*itc) + " and gid=" + LEX_CAST_STR(*itg) + " order by level,(add_level+add_str+add_int+add_tong) limit " + LEX_CAST_STR(*itn - 1));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            change_list.push_back(q.getval());
        }
        q.free_result();
        ++itc;
        ++itg;
        ++itn;
    }

    std::list<int>::iterator it_change = change_list.begin();
    while(it_change != change_list.end())
    {
        if (!q.execute("update char_generals set state=1,delete_time=unix_timestamp()+3600*72 where state=0 and id=" + LEX_CAST_STR(*it_change)))
        {
            CHECK_DB_ERR(q);
        }
        ++it_change;
    }
}

#include "lang_def.h"

void accountOffline(const std::string& account)
{
    json_spirit::mObject obj;
    obj["cmd"] = "logout";
    obj["account"] = account;
    if (0 != InsertInternalActionWork(obj))
    {
        ERR();
    }
}

int getCharDailyVar(int cid, const std::string& field)
{
    Query q(GetDb());
    int value = 0;
    q.get_result("select " + field + " from char_daily_temp where cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        value = q.getval();
    }
    q.free_result();
    return value;
}

int getCharYearVar(int cid, const std::string& field)
{
    Query q(GetDb());
    int value = 0;
    q.get_result("select " + field + " from char_year_temp where cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        value = q.getval();
    }
    q.free_result();
    return value;
}

//检查是否有需要删除的角色
int checkDeleteChar()
{
    Query q(GetDb());
    q.get_result("select id from charactors where state='1' and (delete_time+600)<unix_timestamp()");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        json_spirit::mObject obj;
        obj["cmd"] = "deleteChar";
        obj["cid"] = q.getval();

        if (0 != InsertInternalActionWork(obj))
        {
            ERR();
        }
    }
    q.free_result();
    return 0;
}

//外部程序将事件插入到 scheduleEvent表中，来触发一些处理
int checkScheduleEvent()
{
    int max_id = 0;
    Query q(GetDb());

    /**
    recover_ling 1
    resetWelfare
    daily_reset
    corpsDailyReset
    raceAwards
    saveOnlines
    corpsWeekReset
    **/

    int recover_ling = 0, resetWelfare = 0, resetWelfare2 = 0, daily_reset = 0, corpsDailyReset = 0, raceAwards = 0, saveOnlines = 0, corpsWeekReset = 0;
    q.get_result("select id,event,param1,param2,param3,param4,extra,inputTime from schedule_event where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        max_id = q.getval();
        json_spirit::mObject obj;
        obj["cmd"] = "scheduleEvent";
        std::string event = q.getstr();
        obj["event"] = event;

        if ("recover_ling" == event)
        {
            int add = q.getval();
            recover_ling += add;
        }
        else if ("resetWelfare" == event)
        {
            resetWelfare = 1;
        }
        else if ("resetWelfare1" == event)
        {
            resetWelfare2 = 1;
        }
        else if ("daily_reset" == event)
        {
            daily_reset = 1;
        }
        else if ("corpsDailyReset" == event)
        {
            corpsDailyReset = 1;
        }
        else if ("raceAwards" == event)
        {
            raceAwards = 1;
        }
        else if ("saveOnlines" == event)
        {
            saveOnlines = 1;
        }
        else if ("corpsWeekReset" == event)
        {
            corpsWeekReset = 1;
        }
        else
        {
            obj["param1"] = q.getval();
            obj["param2"] = q.getval();
            obj["param3"] = q.getval();
            obj["param4"] = q.getval();
            obj["extra"] = q.getstr();
            obj["input"] = q.getval();
            if (0 != InsertInternalActionWork(obj))
            {
                ERR();
            }
        }
    }
    q.free_result();
    q.execute("delete from schedule_event where id<=" + LEX_CAST_STR(max_id));
    CHECK_DB_ERR(q);

    if (recover_ling != 0)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "scheduleEvent";
        obj["event"] = "recover_ling";
        obj["param1"] = recover_ling;
        InsertInternalActionWork(obj);
    }
    if (resetWelfare != 0)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "scheduleEvent";
        obj["event"] = "resetWelfare";
        InsertInternalActionWork(obj);
    }
    if (resetWelfare2 != 0)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "scheduleEvent";
        obj["event"] = "resetWelfare2";
        InsertInternalActionWork(obj);
    }
    if (daily_reset != 0)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "scheduleEvent";
        obj["event"] = "daily_reset";
        InsertInternalActionWork(obj);
    }
    if (corpsDailyReset != 0)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "scheduleEvent";
        obj["event"] = "corpsDailyReset";
        InsertInternalActionWork(obj);
    }
    if (raceAwards != 0)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "scheduleEvent";
        obj["event"] = "raceAwards";
        InsertInternalActionWork(obj);
    }
    if (saveOnlines != 0)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "scheduleEvent";
        obj["event"] = "saveOnlines";
        InsertInternalActionWork(obj);
    }
    if (corpsWeekReset != 0)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "scheduleEvent";
        obj["event"] = "corpsWeekReset";
        InsertInternalActionWork(obj);
    }
    return HC_SUCCESS;
}

int checkRecharge()
{
    Query q(GetDb());
    q.get_result("select count(*) from pay_list where (UNIX_TIMESTAMP()-UNIX_TIMESTAMP(pay_time)) > 8 && pay_result='0' && pay_iscancel = '0'");
    CHECK_DB_ERR(q);
    if (q.fetch_row() && q.getval() > 0)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "checkRecharge";
        if (0 != InsertInternalActionWork(obj))
        {
            ERR();
        }
    }
    q.free_result();
    return 0;
}

int checkPack()
{
    Query q(GetDb());
    q.get_result("select count(*) from present_list where processed='0'");
    CHECK_DB_ERR(q);
    if (q.fetch_row() && q.getval() > 0)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "checkPack";
        if (0 != InsertInternalActionWork(obj))
        {
            ERR();
        }
    }
    q.free_result();
    return 0;
}

//根据三个状态更新战斗属性
int updateCombatAttribute(boost::shared_ptr<baseState>* state, int stateNums, combatAttribute& ct)
{
    //鼓舞效果保留
    int m_boss_inspired = ct.m_boss_inspired;    //boss战斗鼓舞
    int m_race_inspired = ct.m_race_inspired;    //竞技鼓舞
    int m_camp_inspired = ct.m_camp_inspired;    //阵营站鼓舞
    int m_guard_inspired = ct.m_guard_inspired;    //护纲鼓舞
    int m_maze_inspired = ct.m_maze_inspired;    //迷宫鼓舞

    ct.clear();
    ct.m_boss_inspired = m_boss_inspired;
    //cout<<"updateCombatAttribute boss inspired "<<m_boss_inspired<<endl;
    ct.m_race_inspired = m_race_inspired;
    ct.m_camp_inspired = m_camp_inspired;
    ct.m_guard_inspired = m_guard_inspired;
    ct.m_maze_inspired = m_maze_inspired;

    for (int i = 0; i < stateNums; ++i)
    {
        boost::shared_ptr<baseState>& st = state[i];
        if (!st.get())
        {
            continue;
        }
        ct.m_enable = 1;
        //特殊攻击类
        if (st->effect_type <= weihe_state)
        {
            switch (st->effect_type)
            {
                case xixue_state:
                    ct.m_special_attack[st->effect_type][0]    = max(st->effect_gailv,ct.m_special_attack[st->effect_type][0]);
                    ct.m_special_attack[st->effect_type][1] += st->effect_value;
                    if (ct.m_special_attack[st->effect_type][1] > 100)
                    {
                        ct.m_special_attack[st->effect_type][1] = 100;
                    }
                    break;
                case podan_state:
                    ct.m_special_attack[st->effect_type][0]    = max(st->effect_gailv,ct.m_special_attack[st->effect_type][0]);
                    ct.m_special_attack[st->effect_type][1] += st->effect_value;
                    break;
                case baoji_state:
                    ct.m_special_attack[st->effect_type][0]    += st->effect_gailv;
                    break;
                default:
                    ct.m_special_attack[st->effect_type][0]    += st->effect_gailv;
                    break;
            }
        }
        //加强技能类
        else if (st->effect_type <= mingjing_state)
        {

        }
        else//伤害增减类
        {
            switch (st->effect_type)
            {
                case pufang_state:
                    ct.m_sub_damage[0] += st->effect_value;
                    break;
                case cefang_state:
                    ct.m_sub_damage[1] += st->effect_value;
                    break;
                case bubingfang_state:
                    ct.m_sub_damage_from[0] += st->effect_value;
                    break;
                case gongbingfang_state:
                    ct.m_sub_damage_from[1] += st->effect_value;
                    break;
                case qibingfang_state:
                    ct.m_sub_damage_from[3] += st->effect_value;
                    break;
                case moushifang_state:
                    ct.m_sub_damage_from[2] += st->effect_value;
                    break;
                case qixiefang_state:
                    ct.m_sub_damage_from[4] += st->effect_value;
                    break;
                case bubingke_state:
                    ct.m_more_damage_to[0] += st->effect_value;
                    break;
                case gongbingke_state:
                    ct.m_more_damage_to[1] += st->effect_value;
                    break;
                case qibingke_state:
                    ct.m_more_damage_to[3] += st->effect_value;
                    break;
                case moushike_state:
                    ct.m_more_damage_to[2] += st->effect_value;
                    break;
                case qixieke_state:
                    ct.m_more_damage_to[4] += st->effect_value;
                    break;
                case shizhan_state:
                    ct.m_death_fight = true;
                    break;
                case weak_state:
                    ct.m_weak += st->effect_value;
            }
        }
    }
    return HC_SUCCESS;
}

//根据三个状态更新战斗属性
int updateCombatAttribute(newCharStates& states, std::map<int,boost::shared_ptr<charSkill> >& skl, combatAttribute& ct)
{
    //鼓舞效果保留
    int m_boss_inspired = ct.m_boss_inspired;    //boss战斗鼓舞
    int m_race_inspired = ct.m_race_inspired;    //竞技鼓舞
    int m_camp_inspired = ct.m_camp_inspired;    //阵营站鼓舞
    int m_guard_inspired = ct.m_guard_inspired;  //护纲鼓舞
    int m_maze_inspired = ct.m_maze_inspired;    //迷宫鼓舞

    memset(&ct.m_sub_damage, 0, sizeof(combatAttribute));
    ct.m_boss_inspired = m_boss_inspired;
    //cout<<"updateCombatAttribute boss inspired "<<m_boss_inspired<<endl;
    ct.m_race_inspired = m_race_inspired;
    ct.m_camp_inspired = m_camp_inspired;
    ct.m_guard_inspired = m_guard_inspired;
    ct.m_maze_inspired = m_maze_inspired;

    for (int i = 0; i < 3; ++i)
    {
        if (states._cur_state[i] > 8 || states._cur_state[i] <= 0 || states._effects[states._cur_state[i]-1] == 0)
        {
            continue;
        }
        int effect_type = states._cur_state[i] - 1;
        //格挡和躲闪需要换下
        if (effect_type == dodge_state)
        {
            effect_type = parry_state;
        }
        else if (parry_state == effect_type)
        {
            effect_type = dodge_state;
        }
        int effect_value = states._effects[states._cur_state[i]-1];

        ct.m_enable = 1;
        //特殊攻击类
        if (effect_type <= weihe_state)
        {
            switch (effect_type)
            {
                case xixue_state:
                    ct.m_special_attack[effect_type][0]    = max(iXiXueGailv,ct.m_special_attack[effect_type][0]);
                    ct.m_special_attack[effect_type][1] += (effect_value/10);
                    if (ct.m_special_attack[effect_type][1] > 100)
                    {
                        ct.m_special_attack[effect_type][1] = 100;
                    }
                    break;
                case podan_state:
                    ct.m_special_attack[effect_type][0]    = 1000;
                    ct.m_special_attack[effect_type][1] += effect_value;
                    break;
                case baoji_state:
                    ct.m_special_attack[effect_type][0]    += effect_value;
                    break;
                default:
                    ct.m_special_attack[effect_type][0]    += effect_value;
                    break;
            }
        }
    }

    std::map<int, boost::shared_ptr<charSkill> >::iterator it = skl.begin();
    while (it != skl.end())
    {
        if (it->second.get() && it->second->skill.get())
        {
            ct.m_enable = 1;
            for (int i = 0; i < skill_add_max; ++i)
            {
                ct.m_skill_add[i] += it->second->skill->effect_per_level[i] * it->second->level;
            }
        }
        ++it;
    }

    return HC_SUCCESS;
}

//根据三个状态更新战斗属性
int updateCombatAttribute(npcStrongholdStates& states, combatAttribute& ct)
{
    //鼓舞效果保留
    int m_boss_inspired = ct.m_boss_inspired;    //boss战斗鼓舞
    int m_race_inspired = ct.m_race_inspired;    //竞技鼓舞
    int m_camp_inspired = ct.m_camp_inspired;    //阵营站鼓舞
    int m_guard_inspired = ct.m_guard_inspired;    //护纲鼓舞
    int m_maze_inspired = ct.m_maze_inspired;    //迷宫鼓舞

    ct.clear();
    ct.m_boss_inspired = m_boss_inspired;
    //cout<<"updateCombatAttribute boss inspired "<<m_boss_inspired<<endl;
    ct.m_race_inspired = m_race_inspired;
    ct.m_camp_inspired = m_camp_inspired;
    ct.m_guard_inspired = m_guard_inspired;
    ct.m_maze_inspired = m_maze_inspired;

    for (int i = 0; i < 3; ++i)
    {
        if (0 == states._cur_state[i] || states._effects[i] == 0)
        {
            continue;
        }
        int effect_type = states._cur_state[i] - 1;
        //格挡和躲闪需要换下
        if (effect_type == dodge_state)
        {
            effect_type = parry_state;
        }
        else if (parry_state == effect_type)
        {
            effect_type = dodge_state;
        }
        int effect_value = states._effects[i];

        ct.m_enable = 1;
        //特殊攻击类
        if (effect_type <= weihe_state)
        {
            switch (effect_type)
            {
                case xixue_state:
                    ct.m_special_attack[effect_type][0]    = max(iXiXueGailv,ct.m_special_attack[effect_type][0]);
                    ct.m_special_attack[effect_type][1] += (effect_value/10);
                    if (ct.m_special_attack[effect_type][1] > 100)
                    {
                        ct.m_special_attack[effect_type][1] = 100;
                    }
                    break;
                case podan_state:
                    ct.m_special_attack[effect_type][0]    = 1000;
                    ct.m_special_attack[effect_type][1] += effect_value;
                    break;
                case baoji_state:
                    ct.m_special_attack[effect_type][0]    += effect_value;
                    break;
                default:
                    ct.m_special_attack[effect_type][0]    += effect_value;
                    break;
            }
        }
    }
    return HC_SUCCESS;
}

double getRandomA()
{
    double rand1 = 0.0, rand2 = 0.0;
    //系数a
    /*11-12-26
        0.02-0.05 30%
        0.06-0.09 45%
        0.10-0.12 20%
        0.13-0.15 5%
    */
    int rand_a = my_random(0,100);
    if (rand_a < 30)
    {
        rand1 = 0.02, rand2 = 0.05;
    }
    else if(rand_a < 75)
    {
        rand1 = 0.06, rand2 = 0.09;
    }
    else if(rand_a < 95)
    {
        rand1 = 0.10, rand2 = 0.12;
    }
    else
    {
        rand1 = 0.13, rand2 = 0.15;
    }
    double fac = my_random(rand1,rand2);
    int fac1 = (int)(fac * 100);
    return (double)fac1/100;

    //return my_random(rand1,rand2);
}

double getRandomB()
{
    double rand1 = 0.0, rand2 = 0.0;
    //系数b
    /*11-12-26
        0.02-0.05 30%
        0.06-0.08 30%
        0.08-0.11 35%
        0.11-0.13 4%
        0.13-0.16 1%
    */
    int rand_b = my_random(0,100);
    if (rand_b < 30)
    {
        rand1 = 0.02, rand2 = 0.05;
    }
    else if(rand_b < 60)
    {
        rand1 = 0.06, rand2 = 0.08;
    }
    else if(rand_b < 95)
    {
        rand1 = 0.08, rand2 = 0.11;
    }
    else if(rand_b < 99)
    {
        rand1 = 0.11, rand2 = 0.13;
    }
    else
    {
        rand1 = 0.13, rand2 = 0.16;
    }
    double fac = my_random(rand1,rand2);
    int fac1 = (int)(fac * 100);
    return (double)fac1/100;
}

//重生需要的等级
inline int rebornLevel(int quality, double fac)
{
    if (quality >= 0 && quality <= 5)
    {
        if (fac >=iChengZhangMax[quality])
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
    int ifac = (int)(10*fac);
    if (ifac <= 80)
    {
        return 30;
    }
    if (ifac <= 120)
    {
        return 50;
    }
    if (ifac <= 180)
    {
        return 60;
    }
    if (ifac <= 260)
    {
        return 70;
    }
    else// if (ifac < 360)
    {
        return 80;
    }
}

//一键重生需要的金币
inline int fastRebornGold(double fac)
{
    int gold = 0;
    int ifac = (int)(10*fac);
    if (ifac <= 80)
    {
        gold = 300;
    }
    else if (ifac <= 120)
    {
        gold = 400;
    }
    else if (ifac <= 180)
    {
        gold = 500;
    }
    else if (ifac <= 260)
    {
        gold = 600;
    }
    else// if (ifac < 360)
    {
        gold = 700;
    }
    if (gold && g_reborn_discount != 100)
    {
        gold = gold * g_reborn_discount / 100;
    }
    return gold;
}

inline int improveChengzhang(double& chengz)
{
    #if 0
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    int ifac = (int)chengz;
    if (ifac < 8)
    {
        int prob[] = {4,8,40,25,10,8,5};
        double pmap[] = {0.02,0.05,0.1,0.2,0.3,0.4,0.5};
        boost::random::discrete_distribution<> dist(prob);
        chengz += pmap[dist(gen)];
    }
    else if (ifac < 12)
    {
        int prob[] = {15,25,25,15,12,8};
        double pmap[] = {0.02,0.05,0.1,0.2,0.3,0.4};
        boost::random::discrete_distribution<> dist(prob);
        chengz += pmap[dist(gen)];
    }
    else if (ifac < 16)
    {
        int prob[] = {20,40,20,13,7};
        double pmap[] = {0.02,0.05,0.1,0.2,0.3};
        boost::random::discrete_distribution<> dist(prob);
        chengz += pmap[dist(gen)];
    }
    else if (ifac < 20)
    {
        int prob[] = {65,20,10,5};
        double pmap[] = {0.02,0.05,0.1,0.2};
        boost::random::discrete_distribution<> dist(prob);
        chengz += pmap[dist(gen)];
    }
    else
    {
        int prob[] = {65,20,10,5};
        double pmap[] = {0.01,0.02,0.05,0.1};
        boost::random::discrete_distribution<> dist(prob);
        chengz += pmap[dist(gen)];
    }
    return ifac;
    #else
    int ifac = (int)chengz;
    chengz += 0.05;
    return ifac;
    #endif
}

inline int getColor(double chengzhang)
{
    int ifac = (int)chengzhang;
    if (ifac <= 5)
    {
        return 0;
    }
    if (ifac <= 7)
    {
        return 1;
    }
    if (ifac <= 9)
    {
        return 2;
    }
    if (ifac <= 11)
    {
        return 3;
    }
    if (ifac <= 13)
    {
        return 3;
    }
    return 5;
}

inline void AttributeToAttack(int _tong, int _int, int _str, int type, int& attack, int& pufang, int& cefang, int& hp)
{
    switch (type)
    {
        case act_wuli_attack:
        {
            attack = (2 * _str);
            break;
        }
        case celue_damage:
        default:
        {
            attack = (2 * _int);
            break;
        }
    }
    pufang = (7*_str)/5;
    cefang = (7*_int)/5;
    hp = 3*_tong;
}

boost::shared_ptr<ZhenData> NewZhen(CharData& c, int type, int level)
{
    boost::shared_ptr<ZhenData> zhen;

    if (level > 5)
    {
        level = 5;
    }
    else if (level < 1)
    {
        level = 1;
    }
    int pos[5] = {-1, -1, -1, -1, -1};
    for (int i = 0; i < level; ++i)
    {
        pos[i] = 0;
    }
    boost::shared_ptr<BaseZhenData> bzhen = GeneralDataMgr::getInstance()->GetBaseZhen(type);
    if (bzhen.get())
    {
        std::string sql = "insert into char_zhens set cid=" + LEX_CAST_STR(c.m_id)
            + ",type=" + LEX_CAST_STR(type)
            + ",level=" + LEX_CAST_STR(level);
            //+ ",name='" + GetDb().safestr(bzhen->m_name) + "'";

        sql += ",pos" + LEX_CAST_STR(bzhen->m_open_pos[0]) + "=" + LEX_CAST_STR(pos[0]);
        sql += ",pos" + LEX_CAST_STR(bzhen->m_open_pos[1]) + "=" + LEX_CAST_STR(pos[1]);
        sql += ",pos" + LEX_CAST_STR(bzhen->m_open_pos[2]) + "=" + LEX_CAST_STR(pos[2]);
        sql += ",pos" + LEX_CAST_STR(bzhen->m_open_pos[3]) + "=" + LEX_CAST_STR(pos[3]);
        sql += ",pos" + LEX_CAST_STR(bzhen->m_open_pos[4]) + "=" + LEX_CAST_STR(pos[4]);
        InsertSaveDb(sql);
        zhen.reset(new ZhenData(c.m_id, c));
        zhen->m_zhen_type = type;
        zhen->m_level = level;
        zhen->m_name = bzhen->m_name;
        for (size_t i = 0; i < 9; ++i)
        {
            zhen->m_generals[i] = -2;
        }
        for (int i = 0; i < level; ++i)
        {
            zhen->m_generals[bzhen->m_open_pos[i] - 1] = 0;
        }
    }
    else
    {
        ERR();
    }
    return zhen;
}

int getAttackPos(int row, int pos[])
{
    static int seqs[3][9] =
    {
        {1, 4, 7, 2, 5, 8, 3, 6, 9},
        {2, 5, 8, 1, 4, 7, 3, 6, 9},
        {3, 6, 9, 2, 5, 8, 1, 4, 7}
    };
    for (int i = 0; i < 9; ++i)
    {
        if (pos[seqs[row][i]-1] > 0)
        {
            return seqs[row][i];
        }
    }
    return 0;
}

//根据攻击方式和对方的阵型情况返回攻击范围
json_spirit::Array getAttackRange(int atype, int mypos, int pos[])
{
    json_spirit::Array v;
    switch (atype)
    {
        //单体攻击
        case 1:
        {
            int apos = getAttackPos(POS_TO_ROW(mypos), pos);
            if (apos > 0)
            {
                v.push_back(apos);
            }
            break;
        }
        //一列
        case 2:
        {
            int apos = getAttackPos(POS_TO_ROW(mypos), pos);
            if (apos > 0)
            {
                int row = POS_TO_ROW(apos);
                switch (row)
                {
                    case 0:
                        v.push_back(1);
                        v.push_back(4);
                        v.push_back(7);
                        break;
                    case 1:
                        v.push_back(2);
                        v.push_back(5);
                        v.push_back(8);
                        break;
                    case 2:
                    default:
                        v.push_back(3);
                        v.push_back(6);
                        v.push_back(9);
                        break;
                }
            }
            break;
        }
        //一排
        case 3:
        {
            int apos = getAttackPos(POS_TO_ROW(mypos), pos);
            if (apos > 0)
            {
                int side = POS_TO_SIDE(apos);
                switch (side)
                {
                    case 0:
                        v.push_back(1);
                        v.push_back(2);
                        v.push_back(3);
                        break;
                    case 1:
                        v.push_back(4);
                        v.push_back(5);
                        v.push_back(6);
                        break;
                    case 2:
                    default:
                        v.push_back(7);
                        v.push_back(8);
                        v.push_back(9);
                        break;
                }
            }
            break;
        }
        //引导
        case 6:
        {
            //第一排
            if (pos[0] > 0)
            {
                v.push_back(1);
            }
            else if (pos[3] > 0)
            {
                v.push_back(4);
            }
            else if (pos[6] > 0)
            {
                v.push_back(7);
            }
            else
            {
                v.push_back(1);
            }
            //第二排
            if (pos[1] > 0)
            {
                v.push_back(2);
            }
            else if (pos[4] > 0)
            {
                v.push_back(5);
            }
            else if (pos[7] > 0)
            {
                v.push_back(8);
            }
            else
            {
                v.push_back(2);
            }
            //第三排
            if (pos[2] > 0)
            {
                v.push_back(3);
            }
            else if (pos[5] > 0)
            {
                v.push_back(6);
            }
            else if (pos[8] > 0)
            {
                v.push_back(9);
            }
            else
            {
                v.push_back(3);
            }
            break;
        }
        //全体
        case 7:
        {
            for (int i = 1; i <= 9; ++i)
            {
                v.push_back(i);
            }
            break;
        }
    }
#if 0
    cout<<"type:"<<atype<<",pos:"<<mypos<<endl;
    cout<<pos[0]<<","<<pos[3]<<","<<pos[6]<<endl;
    cout<<pos[1]<<","<<pos[4]<<","<<pos[7]<<endl;
    cout<<pos[2]<<","<<pos[5]<<","<<pos[8]<<endl;
    cout<<json_spirit::write(v);
#endif
    return v;
}

void ItemToObj(Item* sitem, boost::shared_ptr<json_spirit::Object>& sgetobj)
{
    if (sitem == NULL)
    {
        return;
    }
    sgetobj.reset();
    const Item& item = *sitem;

    boost::shared_ptr<json_spirit::Object> getobj;
    switch (item.type)
    {
        case item_type_silver:    //银币
            {
                getobj.reset(new json_spirit::Object);
                getobj->push_back( Pair("silver", item.nums));
                sgetobj = getobj;
            }
            break;
        case item_type_treasure:    //道具
            {
                getobj.reset(new json_spirit::Object);

                json_spirit::Object gem;
                gem.push_back( Pair("id", item.id));
                boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(item.id);
                if (tr.get())
                {
                    gem.push_back( Pair("name", tr->name));
                    gem.push_back( Pair("spic", tr->spic));
                    gem.push_back( Pair("quality", tr->quality));
                    gem.push_back( Pair("type", tr->usage));
                    gem.push_back( Pair("nums", item.nums));
                }
                else
                {
                    cout<<"tid->"<<item.id<<endl;
                    ERR();
                }
                getobj->push_back( Pair("gem", gem));
                sgetobj = getobj;
            }
            break;
        case item_type_equipment://装备
            {
                getobj.reset(new json_spirit::Object);

                json_spirit::Object equip;
                equip.push_back( Pair("id", item.id));
                boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment(item.id);
                if (tr.get())
                {
                    equip.push_back( Pair("name", tr->name));
                    equip.push_back( Pair("spic", tr->baseid));
                    equip.push_back( Pair("quality", tr->quality));
                }
                else
                {
                    ERR();
                }
                getobj->push_back( Pair("equipment", equip));
                sgetobj = getobj;
            }
            break;
        case item_type_general://武将
            {
                getobj.reset(new json_spirit::Object);

                json_spirit::Object general;
                general.push_back( Pair("id", item.id));
                boost::shared_ptr<GeneralTypeData> tr = GeneralDataMgr::getInstance()->GetBaseGeneral(item.id);
                if (tr.get())
                {
                    general.push_back( Pair("name", tr->m_name));
                    general.push_back( Pair("spic", tr->m_spic));
                    general.push_back( Pair("quality", tr->m_quality));
                }
                else
                {
                    ERR();
                }
                getobj->push_back( Pair("general", general));
                sgetobj = getobj;
            }
            break;
        case item_type_zhen:    //阵型
        {
            getobj.reset(new json_spirit::Object);

            boost::shared_ptr<BaseZhenData> bz = GeneralDataMgr::getInstance()->GetBaseZhen(item.id);
            if (bz.get())
            {
                json_spirit::Object zhen;
                zhen.push_back( Pair("type", bz->m_type));
                zhen.push_back( Pair("name", bz->m_name));
                zhen.push_back( Pair("level", 5));
                getobj->push_back( Pair("zhen", zhen));
                sgetobj = getobj;
            }
            break;
        }
        case item_type_skill:    //技能
        {
            getobj.reset(new json_spirit::Object);

            boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(item.id);
            if (bs.get())
            {
                json_spirit::Object skill;
                skill.push_back( Pair("id", item.id));
                skill.push_back( Pair("name", bs->name));
                skill.push_back( Pair("level", item.nums));
                skill.push_back( Pair("spic", item.id));
                getobj->push_back( Pair("skill", skill));
                sgetobj = getobj;
            }
            break;
        }
        case item_type_gold:    //金币
        {
            getobj.reset(new json_spirit::Object);
            getobj->push_back( Pair("gold", item.nums) );
            sgetobj = getobj;
            break;
        }
        case item_type_ling:    //军令
        {
            getobj.reset(new json_spirit::Object);
            getobj->push_back( Pair("ling", item.nums) );
            sgetobj = getobj;
            break;
        }
        case item_type_prestige:
        {
            getobj.reset(new json_spirit::Object);
            getobj->push_back( Pair("prestige", item.nums) );
            sgetobj = getobj;
            break;
        }
        case item_type_baoshi:    //宝石
        {
            getobj.reset(new json_spirit::Object);
            baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(item.id);
            if (bbs)
            {
                json_spirit::Object baoshi;
                baoshi.push_back( Pair("id", item.id));
                baoshi.push_back( Pair("name", bbs->name));
                baoshi.push_back( Pair("level", 1));
                baoshi.push_back( Pair("nums", item.nums));
                getobj->push_back( Pair("baoshi", baoshi));
                sgetobj = getobj;
            }
            break;
        }
        case item_type_libao:    //礼包
        {
            getobj.reset(new json_spirit::Object);
            baseLibao* p = libao_mgr::getInstance()->getBaselibao(item.id);
            if (p != NULL)
            {
                json_spirit::Object libao;
                libao.push_back( Pair("id", item.id));
                libao.push_back( Pair("name", p->m_name));
                libao.push_back( Pair("nums", item.nums));
                libao.push_back( Pair("spic", p->m_spic));
                libao.push_back( Pair("quality", p->m_quality));
                getobj->push_back( Pair("libao", libao));
                sgetobj = getobj;
            }
            break;
        }
    }
}

int getQualityWashValue(int quality, int value)
{
    if (quality < 0)
    {
        return value;
    }
    else if (quality > 5)
    {
        quality = 5;
    }
    int quality_fac[] = {100, 105, 110, 116, 123, 130};
    return (value*quality_fac[quality]/100);
}

volatile uint64_t CharData::_refs = 0;

CharData::CharData(int cid, bool b_create)
:m_tempo(*this,cid)
,m_bag(*this)
,m_selled_bag(*this)
,m_generals(cid, *this)
,m_zhens(cid, *this)
,m_task(*this)
,m_trunk_tasks(*this)
//,m_backpack(*this)
,m_shop(*this)
,m_new_weapons(cid)
,m_horse(cid)
,m_newStates(*this)
,m_Buffs(*this)
,m_jxl_buff(*this)
{
    //cout<<"CharData::CharData()"<<cid<<",tid:"<<syscall(SYS_gettid)<<endl;
    m_close_friend_to = 0;
    m_vip_exp = 0;
    m_qq_yellow_level = 0;    //QQ黄钻等级
    m_qq_yellow_year = 0;    //是否QQ年费黄钻
    m_recommend_friend_refresh = 0;
    m_up_generals = iDefaultUpGenerals;
    m_fight_cd = 0;
    m_enhance_cd = 0;
    m_can_enhance = true;
    m_char_data_change = false;
    m_is_online = 0;
    m_general_limit = 0;
    m_currentStronghold = 0;
    ++CharData::_refs;
    m_temp_score = 0;
    m_load_success = false;
    m_hp_cost = 0;
    m_save_time = time(NULL);
    m_id = cid;
    //m_guildId = 0; //角色公会
    m_camp = 0;    //角色阵营
    m_area = 1;    //角色所在地区

    m_check_chenmi = false;
    m_login_time = 0;         //登录时间
    m_chenmi_time = 0;
    m_notify_chenmi_time = 0;
    m_chenmi_start_time = 0;

    m_vip = 0;                 //vip等级
    m_can_world_chat = 0;     //是否可以世界聊天
    m_can_chat = true;         //是否被禁言
    m_account = "";
    m_name = "";             //角色名
    m_chat = "";

    m_ip_address = "";

    m_first_explore = 0;
    m_second_explore = 0;

    //武器+装备
    m_total_pugong = 0;
    m_total_pufang = 0;
    m_total_cegong = 0;
    m_total_cefang = 0;
    m_total_bingli = 0;

    memset(m_gold_cost_comfirm, 0, sizeof(int)*iMaxGoldCostConfirm);

    m_current_map = 0;            //当前打开地图
    m_current_stage = 0;        //当前打开场景

    m_trade_state = 0;
    m_tradeque_type = 0;
    m_tradeque_id = 0;
    m_tradeque_pos = 0;
    m_baoshi_count = 0;

    if(b_create)
    {
        Create();
    }
    else
    {
        Load();
    }
    m_teachers_change = false;
    m_book_change = false;

    m_last_stronghold = 0;
    m_reget_times = 0;
    m_last_stronghold_mapid = 0;
    m_last_stronghold_level = 0;
    m_last_stronghold_type = 0;

    m_weapon_attack_change = true;

    m_copy_id = 0;
    m_copy_id_leave = 0;
}

CharData::~CharData()
{
    --CharData::_refs;
    INFO("~CharData()...Save()");
    if (m_load_success)
    {
        Save();
    }
}

uint64_t CharData::refs()
{
    return CharData::_refs;
}

bool CharData::updatePanelOpen()
{
    uint8_t pre_state = m_panel_junt + m_panel_interior + m_panel_zhuj + m_panel_army;

    //军团按钮
    m_panel_junt = m_corpsOpen;

    //内政按钮开放 屯田，探索，商店，通商
    m_panel_interior = m_farmOpen + m_exploreOpen + m_shopOpen + m_tradeOpen ? 1 : 0;

    //武将(部队)按钮 招募,训练，洗髓，宝物，装备&强化
    m_panel_army =    m_trainOpen + m_washOpen + m_baowuOpen + m_equiptOpen + m_equiptEnhanceOpen ? 1 : 0;

    //主将(军事)按钮开放 布阵，科技 ,技能，战马,占星
    m_panel_zhuj = m_zhenOpen + m_weaponOpen + m_skillOpen + m_horseOpen ? 1 : 0;

    return (pre_state != (m_panel_junt + m_panel_interior + m_panel_zhuj + m_panel_army));
}

int CharData::isNewPlayer()
{
    //return ((m_createTime + 12 * 3600) - time(NULL));
    return m_level < 34;
}

int CharData::queryCreateDays()
{
    time_t t_c = m_createTime;
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&t_c, t);
    if (t->tm_hour >= 24 || t->tm_hour < 0)
    {
        ;
    }
    else
    {
        t->tm_hour = 0;
        t->tm_min = 0;
        t->tm_sec = 0;
    }
    time_t tmp_create_time = (int)(mktime(t));
    return ((time(NULL) - tmp_create_time) / (24*3600)) + 1;
}

bool CharData::isWashEventOpen()
{
    return ((m_wash_open_time + 7*24*3600) > time(NULL));
}

int CharData::Create()
{
    //cout<<"CharData::Create()"<<m_id<<endl;
    m_level = 1;
    m_state = 0;
    m_deleteTime = 0;
    m_chenmi_time = 0;
    m_free_rest = 5;
    m_free_rest_time = 0;
    //size_t bag_size = 0;
    //m_bag.addSize(bag_size);
    m_first_explore = 0;
    m_second_explore = 0;
    m_area = 1;
    m_vip = 0;
    m_prestige = 0;
    m_levelupTime = 0;
    //资源
    m_gold = g_default_gold;
    m_silver = g_default_silver;
    m_ling = g_default_ling;
    m_explore_ling = iFreeExploreTimes;
    m_officalcanlevelup = false;
    m_currentStronghold = 0;
    m_cur_stage = 1;

    if (g_default_supply)
    {
        //初始军粮
        int err_code = 0;
        m_bag.addGem(treasure_type_supply, g_default_supply, err_code);
    }

    //送三国霸礼包
    //{
    //    int err_code = 0;
    //    m_bag.addLibao(17);
    //}
    //初始引导
    setExtraData(char_data_type_normal, char_data_current_guide, 1);
    //角色功能开放初始化
    {
        m_officeOpen = m_currentStronghold >= iOfficalOpenStronghold ? 1 : 0;
        m_skillOpen = m_currentStronghold >=iSkillOpenStronghold ? 1 : 0;
        m_zhenOpen = m_currentStronghold >=iZhenOpenStronghold ? 1 : 0;
        m_equiptOpen = m_currentStronghold >=iEquipOpenStronghold ? 1 : 0;
        m_equiptEnhanceOpen = m_currentStronghold >=iEquipEnhanceOpenStronghold ? 1 : 0;
        m_recruitOpen = 1;    //直接开放
        m_sweepOpen = m_currentStronghold >= iSweepOpenStronghold ? 1 : 0;    //扫荡开放
        m_levyOpen = m_currentStronghold >=iLevyOpenStronghold ? 1 : 0;
        m_weaponOpen = m_currentStronghold >= newWeaponMgr::getInstance()->openStronghold() ? 1 : 0;

        m_eliteOpen = m_currentStronghold >=iEliteOpenStronghold ? 1 : 0;

        m_raceOpen = m_currentStronghold >=iRaceOpenStronghold ? 1 : 0;
        m_skillTrainOpen = m_currentStronghold >=iSkillOpenStronghold ? 1 : 0;

        m_trainOpen = m_currentStronghold >=iTrainOpenStronghold ? 1 : 0;
        m_farmOpen = m_currentStronghold >=iFarmOpenStronghold[0] ? 1 : 0;
        m_exploreOpen = m_currentStronghold >=iExploreOpenStronghold ? 1 : 0;
        m_baowuOpen = m_currentStronghold >=iXiangqianOpenStronghold ? 1 : 0;
        m_shopOpen = m_currentStronghold >=iShopOpenStronghold ? 1 : 0;
        m_washOpen = m_currentStronghold >=iWashOpenStronghold ? 1 : 0;
        m_rebornOpen = m_currentStronghold >= iRebornOpenStronghold ? 1 : 0;
        m_horseOpen = m_currentStronghold >=iHorseOpenStronghold ? 1 : 0;
        m_servantOpen = m_currentStronghold >=iServantOpenStronghold ? 1 : 0;
        m_tradeOpen = m_currentStronghold >=iTradeOpenStronghold ? 1 : 0;
        m_guardOpen = m_currentStronghold >=iGuardOpenStronghold ? 1 : 0;
        m_corpsOpen = m_currentStronghold >=iCorpsOpenStronghold ? 1 : 0;
        m_bossOpen = m_currentStronghold >=iBossOpenStronghold ? 1 : 0;
        m_helperOpen = m_currentStronghold >= iHelperOpenStronghold ? 1 : 0;    //助手开放
        m_campraceOpen = m_currentStronghold >= iCampraceOpenStronghold ? 1 : 0;//阵营战开放
        m_buyLingOpen = m_currentStronghold >= iBuyLingOpenStronghold ? 1 : 0;    //购买军令
        m_rankEventOpen = m_currentStronghold >=iRankEventOpenStronghold ? 1 : 0;
        m_bankOpen = m_currentStronghold >=iBankOpenStronghold ? 1 : 0;
        m_soulOpen = m_currentStronghold >= Singleton<trainingMgr>::Instance().openStronghold() ? 1 : 0;
        m_sevenOpen = m_currentStronghold >= iSevenOpenStronghold ? 1 : 0;
        m_jxlOpen = m_currentStronghold >= iJxlOpenStronghold ? 1 : 0;
        m_generalSoulOpen = m_currentStronghold >= iGeneralSoulOpenStronghold ? 1 : 0;
        m_wash_open_time = 0;

        //更新4个大按钮的开放
        updatePanelOpen();
    }
    //当前引导id
    m_current_guide = 1;

    m_change_spic = 0;

    m_change_spic_time = 0;

    //今日培养战马次数
    m_gold_train_horse = 0;
    m_silver_train_horse = 0;
    m_test_recharge_time = 0;
    m_offical = 1;
    boost::shared_ptr<baseoffical> p_bo = GeneralDataMgr::getInstance()->GetBaseOffical(m_offical);
    if (p_bo.get())
    {
        m_salary = p_bo->m_salary;
        m_offical_name = p_bo->m_name;
    }
    else
    {
        m_salary = 0;
        m_offical_name = "";
    }
    GeneralDataMgr::getInstance()->GetMapMemo(m_area,m_area_name,m_area_memo);
    m_can_chat = true;
    m_can_world_chat = true;
    m_general_limit = 3;
    m_total_recharge = 0;
    memset(m_skill_power, 0, 5*sizeof(int));
    m_newbie_reward_canGet = 0;
    checkWeapon();
    //checkSouls();
    m_welfare = 0;
    m_continue_days = 0;
    m_total_continue_days = 0;
    //加载玩家VIP奖励领取信息
    for (int i = 1; i <= 12; ++i)
    {
        CharVIPPresent cvp;
        cvp.cid = m_id;
        cvp.present = GeneralDataMgr::getInstance()->getBaseVIPPresent(i);
        cvp.state = 0;
        m_vip_present[i] = cvp;
    }
    m_tmp_vip = 0;
    m_tmp_vip_start_time = 0;
    m_tmp_vip_end_time = 0;
    memset(m_wash_event, 0, sizeof(int)*10);
    m_daily_wash_times = 0;
    m_wash_event_state = 0;

    //金币休息次数
    m_gold_rest = 0;
    //金币刷新探索次数
    m_explore_refresh_times = 0;
    //官职俸禄领取情况
    m_hasgetsalary = 1;
    //购买洗髓次数
    m_buy_xisui_time = 0;
    //今日征收次数
    m_levy_time = 0;
    //祭祀次数
    m_temp_jisi_times = 0;
    //军团宴会次数
    m_temp_corps_yanhui = 0;
    //屯田收获次数
    m_farm_harvest = 0;

    //第一次先发一次
    m_upgrade_weapon_state = -1;
    m_enhance_state = -1;
    //updateEnhanceCost();
    //updateUpgradeWeaponCost();
    //updateEnhanceCDList();
    //updateUpgradeWeaponCDList();

    int first_chengzhang = libao_mgr::getInstance()->getChengzhangLibao(1);
    if (first_chengzhang > 0)
    {
        m_bag.addLibao(first_chengzhang);
        m_chengzhang_reward.push_back(0);
    }
}

//从数据库中读取角色数据
int CharData::Load()
{
    INFO("CharData::Load()"<<m_id);
    Query q(GetDb());
    q.get_result("SELECT ac.union_id,ac.server_id,ac.qid,"
                "c.account,c.name,c.spic,c.level,c.lastlogin,c.state,c.delete_time,"
                "c.createTime,c.chenmi_time,c.continue_days,c.total_continue_days,"
                "cd.freeRest,cd.freeRestTime,cd.backPack,cd.finish_first_explore,cd.finish_second_explore,"
                "cd.mapid,cd.vip,cd.chat,cd.camp,cd.prestige,cd.official,cd.levelupTime,cd.exp,"
                "cr.gold,cr.silver,cr.ling,cr.explore_ling"
                " FROM `charactors` as c"
                " left join `char_resource` as cr on c.id=cr.cid"
                " left join `char_data` as cd on c.id=cd.cid"
                " left join `accounts` as ac on c.account=ac.account"
                " WHERE (0=c.state or c.delete_time>unix_timestamp()) and c.id=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_load_success = true;

        m_union_id = q.getval();
        m_server_id = q.getstr();
        m_qid = q.getstr();
        m_account = q.getstr();
        m_name = q.getstr();
        m_spic = q.getval();
        m_level = q.getval();
        m_login_time = q.getval();
        m_state = q.getval();
        m_deleteTime = q.getval();

         m_createTime = q.getval();
        m_chenmi_time = q.getval();
        m_continue_days = q.getval();
        m_total_continue_days = q.getval();

        //根据图片确定性别奇数为男
        if (m_spic % 2 != 0)
        {
            m_gender = 1;
        }
        else
        {
            m_gender = 0;
        }

        m_free_rest = q.getval();
        m_free_rest_time = q.getval();
        size_t bag_size = q.getval();
        m_bag.addSize(bag_size);
        m_first_explore = q.getval();
        m_second_explore = q.getval();
        m_area = q.getval();
        m_vip = q.getval();
        m_chat = q.getstr();
        m_camp = q.getval();
        m_prestige = q.getval();
        m_offical = q.getval();
        m_levelupTime = q.getval();
        m_vip_exp = q.getval();
        if (m_vip < 0)
        {
            m_vip = 0;
        }
        else if (m_vip > 12)
        {
            m_vip = 12;
        }

        //资源
        m_gold = q.getval();
        m_silver = q.getval();
        m_ling = q.getval();
        m_explore_ling = q.getval();
        m_officalcanlevelup = OfficalLevelUpState();
    }
    else
    {
        q.free_result();
        cout<<"****** CharData::Load() fail,"<<m_id<<endl;
        void * array[25];
        int nSize = backtrace(array, 25);
        char ** symbols = backtrace_symbols(array, nSize);
        if (symbols != NULL)
        {
            for (int i = 0; i < nSize; i++)
            {
                if (symbols[i] != NULL)
                {
                    cout << symbols[i] << endl;
                }
            }
            free(symbols);
        }
        cout<<"****** end of CharData::Load() fail"<<endl;
        m_load_success = false;
        return -1;
    }
    q.free_result();

    updateBaoshiCount();

    //角色特殊字段
    loadExtraData();

    //背包:装备/道具
    m_bag.loadCharBag(m_id);
    //回购包
    m_selled_bag.load();

    m_Buffs.load();

    m_generals.Load();
    m_zhens.Load();

    //当前引导id
    m_current_guide = queryExtraData(char_data_type_normal, char_data_current_guide);

    m_change_spic = queryExtraData(char_data_type_normal, char_data_change_spic);

    m_change_spic_time = queryExtraData(char_data_type_normal, char_data_change_spic_time);

    //今日培养战马次数
    m_gold_train_horse = queryExtraData(char_data_type_daily, char_data_horse_gold_train);
    m_silver_train_horse = queryExtraData(char_data_type_daily, char_data_horse_silver_train);

    m_test_recharge_time = 0;
    q.get_result("select unix_timestamp(input) from char_recharge where type='test' and cid=" + LEX_CAST_STR(m_id) + " order by input desc limit 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        time_t last_time = q.getval();
        m_test_recharge_time = last_time + 3600*6;
    }
    q.free_result();

    m_total_test_recharge = 0;
    q.get_result("select sum(gold) from char_recharge where type='test' and cid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_total_test_recharge = q.getval();
    }
    q.free_result();

    boost::shared_ptr<baseoffical> p_bo = GeneralDataMgr::getInstance()->GetBaseOffical(m_offical);
    if (p_bo.get())
    {
        m_salary = p_bo->m_salary;
        m_offical_name = p_bo->m_name;
    }
    else
    {
        m_salary = 0;
        m_offical_name = "";
    }

    GeneralDataMgr::getInstance()->GetMapMemo(m_area,m_area_name,m_area_memo);

    //m_guildId = 0;
    m_can_chat = true;
    m_can_world_chat = true;

    //从其他信息表载入角色装备，武器，状态，武将，阵形，关卡进度
    m_tempo.load(m_id, 0);

    //武将上限
    for (int i = 0; i < sizeof(iGeneralLimitStronghold)/sizeof(int); ++i)
    {
        if (iGeneralLimitStronghold[i] > m_currentStronghold)
        {
            m_general_limit = i;
            break;
        }
    }
    //角色功能开放初始化
    {
        m_officeOpen = m_currentStronghold >= iOfficalOpenStronghold ? 1 : 0;
        m_skillOpen = m_currentStronghold >=iSkillOpenStronghold ? 1 : 0;
        m_zhenOpen = m_currentStronghold >=iZhenOpenStronghold ? 1 : 0;
        m_equiptOpen = m_currentStronghold >=iEquipOpenStronghold ? 1 : 0;
        m_equiptEnhanceOpen = m_currentStronghold >=iEquipEnhanceOpenStronghold ? 1 : 0;
        m_recruitOpen = 1;    //直接开放
        m_sweepOpen = m_currentStronghold >= iSweepOpenStronghold ? 1 : 0;    //扫荡开放
        m_levyOpen = m_currentStronghold >=iLevyOpenStronghold ? 1 : 0;
        m_weaponOpen = m_currentStronghold >= newWeaponMgr::getInstance()->openStronghold() ? 1 : 0;

        m_eliteOpen = m_currentStronghold >=iEliteOpenStronghold ? 1 : 0;

        m_raceOpen = m_currentStronghold >=iRaceOpenStronghold ? 1 : 0;
        m_skillTrainOpen = m_currentStronghold >=iSkillOpenStronghold ? 1 : 0;

        m_trainOpen = m_currentStronghold >=iTrainOpenStronghold ? 1 : 0;
        m_farmOpen = m_currentStronghold >=iFarmOpenStronghold[0] ? 1 : 0;
        m_exploreOpen = m_currentStronghold >=iExploreOpenStronghold ? 1 : 0;
        m_baowuOpen = m_currentStronghold >=iXiangqianOpenStronghold ? 1 : 0;
        m_shopOpen = m_currentStronghold >=iShopOpenStronghold ? 1 : 0;
        m_washOpen = m_currentStronghold >=iWashOpenStronghold ? 1 : 0;
        m_rebornOpen = m_currentStronghold >= iRebornOpenStronghold ? 1 : 0;
        m_horseOpen = m_currentStronghold >=iHorseOpenStronghold ? 1 : 0;
        m_servantOpen = m_currentStronghold >=iServantOpenStronghold ? 1 : 0;
        m_tradeOpen = m_currentStronghold >=iTradeOpenStronghold ? 1 : 0;
        m_guardOpen = m_currentStronghold >=iGuardOpenStronghold ? 1 : 0;
        m_corpsOpen = m_currentStronghold >=iCorpsOpenStronghold ? 1 : 0;
        m_bossOpen = m_currentStronghold >=iBossOpenStronghold ? 1 : 0;
        m_helperOpen = m_currentStronghold >= iHelperOpenStronghold ? 1 : 0;    //助手开放
        m_campraceOpen = m_currentStronghold >= iCampraceOpenStronghold ? 1 : 0;//阵营战开放
        m_buyLingOpen = m_currentStronghold >= iBuyLingOpenStronghold ? 1 : 0;    //购买军令
        m_rankEventOpen = m_currentStronghold >=iRankEventOpenStronghold ? 1 : 0;
        m_bankOpen = m_currentStronghold >=iBankOpenStronghold ? 1 : 0;
        m_soulOpen = m_currentStronghold >= Singleton<trainingMgr>::Instance().openStronghold() ? 1 : 0;
        m_sevenOpen = m_currentStronghold >= iSevenOpenStronghold ? 1 : 0;
        m_jxlOpen = m_currentStronghold >= iJxlOpenStronghold ? 1 : 0;

        m_generalSoulOpen = m_currentStronghold >= iGeneralSoulOpenStronghold ? 1 : 0;

        if (m_currentStronghold >= iSecondUpGeneralStronghold)
        {
            m_up_generals = iDefaultUpGenerals + 2;
        }
        else if (m_currentStronghold >= iFirstUpGeneralStronghold)
        {
            m_up_generals = iDefaultUpGenerals + 1;
        }
        if (m_washOpen)
        {
            m_wash_open_time = queryExtraData(char_data_type_normal, char_data_wash_start_time);
            if (m_wash_open_time == 0)
            {
                m_wash_open_time = time(NULL);
                setExtraData(char_data_type_normal, char_data_wash_start_time, m_wash_open_time);
            }
        }

        //更新4个大按钮的开放
        updatePanelOpen();
    }
    //角色技能
    q.get_result("select sid,level,exp from char_skills where cid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int sid = q.getval();
        boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(sid);
        if (bs.get())
        {
            boost::shared_ptr<charSkill> cs = m_skill_list[sid];
            if (!cs.get())
            {
                cs.reset(new charSkill);
                m_skill_list[sid] = cs;
            }
            cs->skill = bs;
            cs->level  = q.getval();
            cs->exp = q.getval();
            cs->state = 0;
            cs->cid = m_id;
        }
        else
        {
            ERR();
            InsertSaveDb("delete from char_skills where cid=" + LEX_CAST_STR(m_id)+ " and sid=" + LEX_CAST_STR(sid));
        }
    }
    q.free_result();

    //角色技能研究者
    q.get_result("select id,teacher from char_skill_teachers where cid=" + LEX_CAST_STR(m_id) + " and id<=3 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int pos = q.getval();
        int teacher = q.getval();
        m_teachers[pos - 1] = baseSkillMgr::getInstance()->GetSkillTeacher(teacher);
        if (!m_teachers[pos - 1].get())
        {
            ERR();
        }
    }
    q.free_result();

    //角色技能研究队列
    q.get_result("select pos,cid,sid,teacher,starttime,lefttime,state,type,fatigue,accelerate_time from char_skill_research where cid=" + LEX_CAST_STR(m_id) + " order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        skillResearchQue que;
        que.pos = q.getval();
        que.cid = q.getval();
        int sid = q.getval();
        int teacher_id = q.getval();
        que.start_time = q.getval();
        que.left_mins = q.getval();
        que.end_time = que.start_time + que.left_mins * 60;
        que.state = q.getval();
        que.type = q.getval();
        que.fatigue = q.getval();
        que.accelerate_time = q.getval();
        que.final_speed = 0;
        if (que.type > 0)
        {
            que.more = skill_vip_queue_more_speed;
        }
        int mod = que.left_mins % skill_research_mins;
        if (mod != 0)
        {
            que.left_mins = que.left_mins + skill_research_mins - mod;
        }
        m_skill_queue.push_back(que);
        //cout<<"******load skill queue "<<que.state<<","<<sid<<","<<teacher_id<<","<<que.left_mins<<endl;
        //启动定时研究
        if (que.state > 0 && sid > 0 && teacher_id > 0 && que.left_mins > 0)
        {
            boost::shared_ptr<skillTeacher> tea = baseSkillMgr::getInstance()->GetSkillTeacher(teacher_id);
            if (!tea.get())
            {
                ERR();
                m_skill_queue[m_skill_queue.size()-1].teacher.reset();
                m_skill_queue[m_skill_queue.size()-1].state = 0;
                m_skill_queue[m_skill_queue.size()-1].left_mins = 0;
                m_skill_queue[m_skill_queue.size()-1].skill.reset();
            }
            else
            {
                m_skill_queue[m_skill_queue.size()-1].teacher = tea;
                m_skill_queue[m_skill_queue.size()-1].skill = m_skill_list[sid];
                m_skill_queue[m_skill_queue.size()-1].state = 0;
                if (m_skill_queue[m_skill_queue.size()-1].skill.get())
                {
                    m_skill_queue[m_skill_queue.size()-1].start();
                }
            }
        }
        else
        {
            m_skill_queue[m_skill_queue.size()-1].state = 0;
            m_skill_queue[m_skill_queue.size()-1].left_mins = 0;
        }
    }
    q.free_result();

    //武将训练位置加载
    LoadTrainList();

    //训练兵书
    q.get_result("select pos,bid from char_train_books where cid=" + LEX_CAST_STR(m_id) + " and pos<=3 order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int pos = q.getval();
        int bid = q.getval();
        m_book[pos - 1] = TrainMgr::getInstance()->GetBook(bid);
        if (!m_book[pos - 1].get())
        {
            ERR();
        }
    }
    q.free_result();

    //角色状态
    //m_newStates.load();
    //更新状态和技能的作用
    updateCombatAttribute();

    //商店
    m_shop.Load();

    //金币消费不提示
    q.get_result("select type from char_goldCost_noConfirm where cid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int type = q.getval();
        if (type >= 1 && type <= iMaxGoldCostConfirm)
        {
            m_gold_cost_comfirm[type-1] = 1;
        }
    }
    q.free_result();

    //累计充值提示
    q.get_result("select total_recharge from char_total_recharge where cid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_total_recharge = q.getval();
    }
    else
    {
        m_total_recharge = 0;
    }
    q.free_result();

    //地区攻略领取情况
    q.get_result("select mapid,get from char_map_intro_get where cid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int get = q.getval();
        m_map_intro_get[id] = get;
    }
    q.free_result();

    //新手引导完成情况
    q.get_result("select guide,input from char_guide_complete where cid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        time_t input = q.getval();
        m_guide_completes[id] = input;
    }
    q.free_result();

    m_newbie_reward_canGet = 0;

    //还在新手冲锋号期间内
    //if (m_createTime + iNewbieGoGoGoSecs > time(NULL))
    {
        q.get_result("select level from char_newbie_event where cid=" + LEX_CAST_STR(m_id));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int level = q.getval();
            m_newbie_reward[level] = true;
        }
        q.free_result();

        //更新新手冲锋号是否可以获得
        updateNewbieEventState();
    }

    q.get_result("select state from char_chengzhang_event where cid=" + LEX_CAST_STR(m_id) + " order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int state = q.getval();
        m_chengzhang_reward.push_back(state);
    }
    q.free_result();

    //加载新兵器系统
    q.get_result("select wType,wid,level from char_new_weapons where cid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int type = q.getval();
        if (type >= 1 && type <= 5)
        {
            int id = q.getval();
            m_new_weapons._weapons[type-1]._type = type;
            m_new_weapons._weapons[type-1]._level = q.getval();
            m_new_weapons._weapons[type-1]._baseWeapon = newWeaponMgr::getInstance()->getWeapon(id);
            if (m_new_weapons._weapons[type-1]._baseWeapon)
            {
                m_new_weapons._weapons[type-1]._effect = m_new_weapons._weapons[type-1]._baseWeapon->effect(m_new_weapons._weapons[type-1]._level);
                m_new_weapons._weapons[type-1]._cost = m_new_weapons._weapons[type-1]._baseWeapon->levelCost(m_new_weapons._weapons[type-1]._level);

                if (id >= 6)
                {
                    if (m_new_weapons._weapons[type-1]._level <= m_new_weapons._weapons[type-1]._baseWeapon->_openLevel)
                    {
                        m_new_weapons._weapons[type-1]._level = m_new_weapons._weapons[type-1]._level % 20 - 1 + m_new_weapons._weapons[type-1]._baseWeapon->_openLevel;
                    }
                }
            }
            else
            {
                ERR();
                cout<<"error id "<<id<<endl;
            }
        }
        else
        {
            ERR();
        }
    }
    q.free_result();

    checkWeapon();
    //m_new_weapons.updateNewAttack();

    //if (m_soulOpen)
    //{
    //    Singleton<trainingMgr>::Instance().getChar(m_id);
    //}

    if (m_horseOpen)
    {
        //加载战马
        q.get_result("select horseid,exp,action_start,action_end,pugong,pufang,cegong,cefang,bingli from char_horses where cid=" + LEX_CAST_STR(m_id));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            m_horse.cid = m_id;
            m_horse.horseid = q.getval();
            m_horse.exp = q.getval();
            m_horse.start_time = q.getval();
            m_horse.end_time = q.getval();
            m_horse.pugong = q.getval();
            m_horse.pufang = q.getval();
            m_horse.cegong = q.getval();
            m_horse.cefang = q.getval();
            m_horse.bingli = q.getval();
            q.free_result();
            m_horse.horse = horseMgr::getInstance()->getHorse(m_horse.horseid);
            if (m_horse.horse == NULL)
            {
                ERR();
                cout << "error horse_id " << m_horse.horseid << endl;
            }
            else
            {
                //升级经验变化的处理
                baseHorse* next_horse = horseMgr::getInstance()->getHorse(m_horse.horse->id + 1);
                while (next_horse && m_horse.exp >= next_horse->need_exp && m_horse.horse->star < iHorseStarsMax)
                {
                    m_horse.exp -= next_horse->need_exp;
                    m_horse.horse = next_horse;
                    ++m_horse.horseid;
                    next_horse = horseMgr::getInstance()->getHorse(m_horse.horse->id + 1);
                }
            }
        }
        else
        {
            m_horse.cid = m_id;
            m_horse.exp = 0;
            m_horse.horseid = 1;
            m_horse.horse = horseMgr::getInstance()->getHorse(m_horse.horseid);
            InsertSaveDb("insert into char_horses set cid=" + LEX_CAST_STR(m_id) + ",horseid='1'");
        }
        q.free_result();
        //加载果子活动
        if (m_horse.end_time > time(NULL))
        {
            int last_horse_id = 0;
            q.get_result("select horse_id,fruit_id,state,start_time,end_time from char_horses_action where cid=" + LEX_CAST_STR(m_id) + " order by horse_id");
            CHECK_DB_ERR(q);
            while (q.fetch_row())
            {
                int horse_id = q.getval();
                int fruit_id = q.getval();
                if (horse_id != last_horse_id)
                {
                    CharHorseFruitAction chfa(m_id,horse_id);
                    baseHorse* p = horseMgr::getInstance()->getHorse(horse_id);
                    if (p)
                    {
                        chfa.horse_star = p->star;
                        chfa.horse_name = p->name;
                        if (fruit_id)
                        {
                            boost::shared_ptr<baseHorseFruit> pbh = horseMgr::getInstance()->getBaseHorseFruit(fruit_id);
                            if (pbh.get())
                            {
                                CharHorseFruit chf(m_id);
                                chf.fruit = pbh;
                                chf.state = q.getval();
                                chf.start_time = q.getval();
                                chf.end_time = q.getval();
                                if (chf.state == 1 || chf.state == 2)
                                    chf.start();
                                chfa.fruits_list.push_back(chf);
                            }
                        }
                        m_horse.action_list.push_back(chfa);
                    }
                    last_horse_id = horse_id;
                }
                else
                {
                    if (m_horse.action_list[m_horse.action_list.size() - 1].horse_id == horse_id && fruit_id)
                    {
                        boost::shared_ptr<baseHorseFruit> pbh = horseMgr::getInstance()->getBaseHorseFruit(fruit_id);
                        if (pbh.get())
                        {
                            CharHorseFruit chf(m_id);
                            chf.fruit = pbh;
                            chf.state = q.getval();
                            chf.start_time = q.getval();
                            chf.end_time = q.getval();
                            if (chf.state == 1 || chf.state == 2)
                                chf.start();
                            m_horse.action_list[m_horse.action_list.size() - 1].fruits_list.push_back(chf);
                        }
                    }
                }
            }
        }
    }

    //m_horse.updateNewAttack();

    //加载兵魂
    //m_training.load();
    //checkSouls();

    //load char_data_temp
    q.get_result("select welfare from char_data_temp where cid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_welfare = q.getval();
    }
    else
    {
        m_welfare = 0;
    }
    q.free_result();

#if 0
    if (m_currentStronghold >= iContinueLoginStronghold)
    {
        m_login_present.clear();
        //加载玩家连续登录奖励领取信息
        q.get_result("select pid,state from char_continue_login_present where cid=" + LEX_CAST_STR(m_id));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            CharLoginPresent clp;
            clp.cid = m_id;
            int pid = q.getval();
            clp.state = q.getval();
            clp.present = GeneralDataMgr::getInstance()->getBaseLoginPresent(pid);
            m_login_present[pid] = clp;
        }
        q.free_result();
        if (m_login_present.empty())
        {
            for (int i = 1; i <= 7; ++i)
            {
                CharLoginPresent clp;
                clp.cid = m_id;
                clp.present = GeneralDataMgr::getInstance()->getBaseLoginPresent(i);
                if (i == 1)
                {
                    InsertSaveDb("insert into char_continue_login_present set state=1,cid=" + LEX_CAST_STR(m_id) + ",pid=" + LEX_CAST_STR(i));
                    clp.state = 1;
                }
                else
                {
                    InsertSaveDb("insert into char_continue_login_present set cid=" + LEX_CAST_STR(m_id) + ",pid=" + LEX_CAST_STR(i));
                    clp.state = 0;
                }
                m_login_present[i] = clp;
            }
        }
    }
    else
    {
        m_continue_days = 0;
        m_total_continue_days = 0;
    }
#endif

    m_vip_present.clear();
    //加载玩家VIP奖励领取信息
    for (int i = 1; i <= 12; ++i)
    {
        CharVIPPresent cvp;
        cvp.cid = m_id;
        cvp.present = GeneralDataMgr::getInstance()->getBaseVIPPresent(i);
        q.get_result("select * from char_vip_present where cid=" + LEX_CAST_STR(m_id) + " and vip_id=" + LEX_CAST_STR(i));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            cvp.state = 2;
        }
        else if (m_vip >= i && cvp.present)
            cvp.state = 1;
        else
            cvp.state = 0;
        q.free_result();
        m_vip_present[i] = cvp;
    }

    //临时VIP
    q.get_result("SELECT starttime,endtime FROM char_tmp_vip WHERE cid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_tmp_vip = iTmpVip_level;
        m_tmp_vip_start_time = q.getval();
        m_tmp_vip_end_time = q.getval();
        if (m_tmp_vip_end_time > time(NULL) && m_vip < m_tmp_vip)
        {
            m_vip = m_tmp_vip;
        }
    }
    else
    {
        m_tmp_vip = 0;
        m_tmp_vip_start_time = 0;
        m_tmp_vip_end_time = 0;
    }
    q.free_result();

    memset(m_wash_event, 0, sizeof(int)*10);
    m_daily_wash_times = 0;
    m_wash_event_state = 0;
    if (m_washOpen && isWashEventOpen())
    {
        m_daily_wash_times = queryExtraData(char_data_type_daily, char_data_super_wash_times);
        for (int i = 0; i < iWashEventNum; ++i)
        {
            m_wash_event[i] = queryExtraData(char_data_type_daily, char_data_wash_event + i);
        }
        updateWashEventState();
    }

    //金币休息次数
    m_gold_rest = getCharDailyVar(m_id, "goldRest");
    //金币刷新探索次数
    m_explore_refresh_times = getCharDailyVar(m_id, "refresh_explore ");
    //官职俸禄领取情况
    m_hasgetsalary = getCharDailyVar(m_id, "hasgetSalary");
    if (!m_officeOpen || m_offical < 2)
    {
        m_hasgetsalary = 1;
    }
    //购买洗髓次数
    m_buy_xisui_time = getCharDailyVar(m_id, "buy_xisui_time");
    //今日征收次数
    m_levy_time = getCharDailyVar(m_id, "levy_time");
    //祭祀次数
    m_temp_jisi_times = getCharDailyVar(m_id, "jishi_time");
    //军团宴会次数
    m_temp_corps_yanhui = getCharDailyVar(m_id, "corps_yanhui");
    //屯田收获次数
    m_farm_harvest = getCharYearVar(m_id, "farm_havest");

    //第一次先发一次
    m_upgrade_weapon_state = -1;
    m_enhance_state = -1;
    updateEnhanceCost();
    updateUpgradeWeaponCost();
    updateEnhanceCDList();
    updateUpgradeWeaponCDList();

    //cout<<"charData::load("<<m_id<<")"<<endl;
    m_jxl_buff.load();
#ifdef QQ_PLAT
    m_close_friend_to = Singleton<inviteMgr>::Instance().getCloseFriendTo(m_id);
#else
    m_close_friend_to = 0;
#endif
    //cout<<"CharData::Load() .. "<<m_id<<",silver:"<<m_silver<<",enhance silver:"<<m_enhance_silver<<",state:"<<m_enhance_state;
    //cout<<" .. "<<"gongxun:"<<getGongxun()<<",upgrade gongxun:"<<m_upgrade_weapon_gongxun<<",state:"<<m_upgrade_weapon_state<<endl;

    return 0;
}

int CharData::updateNewbieEventState()
{
    m_newbie_reward_canGet = libao_mgr::getInstance()->getLevelLibaoState(*this);
    return m_newbie_reward_canGet;
}

int CharData::updateWashEventState()
{
    int old_state = m_wash_event_state;
    m_wash_event_state = 0;
    for (int i = 0; i < iWashEventNum; ++i)
    {
        if (m_daily_wash_times >= iWashEvent[i][0])
        {
            m_wash_event_state = 1;
            break;
        }
    }
    return m_wash_event_state != old_state ? 1 : 0;
}

#if 0
//查询礼品活动状态
int CharData::getGiftState()
{
    int state = 0;

    time_t continue_login_gift_end = queryExtraData(char_data_type_normal, char_data_get_continue_login_day);
    if (m_currentStronghold >= iContinueLoginStronghold && time(NULL) < continue_login_gift_end)
    {
        //登陆礼包
        std::map<int,CharLoginPresent>::iterator it = m_login_present.begin();
        while (it != m_login_present.end())
        {
            CharLoginPresent& clp = it->second;
            if (clp.present)
            {
                if (clp.state == 1)
                {
                    state = 1;
                    return state;
                }
            }
            ++it;
        }
    }

    //首充
    if (state == 0 && m_currentStronghold >= iFirstRechargeStronghold && queryExtraData(char_data_type_normal, char_data_first_recharge_gift) == 1)
    {
        state = 1;
    }

    //VIP
    if (state == 0)
    {
        std::map<int,CharVIPPresent>::iterator it = m_vip_present.begin();
        while (it != m_vip_present.end())
        {
            CharVIPPresent& cvp = it->second;
            if (cvp.present)
            {
                if (cvp.state == 1)
                {
                    state = 1;
                    break;
                }
            }
            ++it;
        }
    }
    //新手冲锋用做等级礼包
    if (state == 0)
    {
        if (m_newbie_reward_canGet == 1)
        {
            //cout<<m_id<<" - newbie event can get"<<endl;
            return 1;
        }
    }
    return state;
}
#endif

//查询VIP按钮状态
int CharData::getVipState()
{
#ifdef QQ_PLAT
    //V5武将可以领取
    int vip_general_state = queryExtraData(char_data_type_normal, char_data_qq_yellow_special);
    if (vip_general_state == 0 && m_vip >= 5 && m_level >= 30)
    {
        return 1;
    }

    //V8武将可以领取
    int vip8_general_state = queryExtraData(char_data_type_normal, char_data_vip8_general);
    if (vip8_general_state == 0 && m_vip >= 8 && m_level >= 40)
    {
        return 1;
    }

    //V10武将可以领取
    int vip10_general_state = queryExtraData(char_data_type_normal, char_data_vip10_general);
    if (vip10_general_state == 0 && m_vip >= 10 && m_level >= 70)
    {
        return 1;
    }
    //vipGet:1      VIP礼包已经全部领取
    int state = 2;
    std::map<int,CharVIPPresent>::iterator it = m_vip_present.begin();
    while (it != m_vip_present.end())
    {
        CharVIPPresent cvp = it->second;
        if (cvp.present.get())
        {
            if (cvp.state == 1)
            {
                state = 1;
                break;
            }
            else if (state == 2)
            {
                state = 0;
            }
            break;
        }
        ++it;
    }
    if (state == 1)
    {
        return state;
    }
    else if (state == 2 && vip_general_state == 1 && vip8_general_state == 1)
    {
        return 2;
    }

    int viewed = queryExtraData(char_data_type_daily, char_data_daily_view_vip_benefit);
    if (0 == viewed)
    {
        return 1;
    }
    return 0;
#else
    int viewed = queryExtraData(char_data_type_daily, char_data_daily_view_vip_benefit);
    if (0 == viewed)
    {
        return 1;
    }

    //V5武将可以领取
    int vip_general_state = queryExtraData(char_data_type_normal, char_data_vip_special_libao);
    if (vip_general_state == 0 && m_vip >= 5 && m_level >= 30)
    {
        return 1;
    }

    //V8武将可以领取
    int vip8_general_state = queryExtraData(char_data_type_normal, char_data_vip8_general);
    if (vip8_general_state == 0 && m_vip >= 8 && m_level >= 40)
    {
        return 1;
    }
    //V10武将可以领取
    int vip10_general_state = queryExtraData(char_data_type_normal, char_data_vip10_general);
    if (vip10_general_state == 0 && m_vip >= 10 && m_level >= 70)
    {
        return 1;
    }
    //vipGet:1      VIP礼包已经全部领取
    int state = 2;
    std::map<int,CharVIPPresent>::iterator it = m_vip_present.begin();
    while (it != m_vip_present.end())
    {
        CharVIPPresent cvp = it->second;
        if (cvp.present.get())
        {
            if (cvp.state == 1)
            {
                state = 1;
                break;
            }
            else if (state == 2)
            {
                state = 0;
            }
            break;
        }
        ++it;
    }
    if (state == 1)
    {
        return 1;
    }
    if (m_vip > 0)
    {
        int daily_get = queryExtraData(char_data_type_daily, char_data_daily_vip_libao);
        if (0 == daily_get)
        {
            return 1;
        }
        #if 0
        int week_get = queryExtraData(char_data_type_week, char_data_week_vip_libao);
        if (0 == week_get)
        {
            return 1;
        }
        #endif
    }
    return 0;
#endif
}

//查询开服活动状态
int CharData::getOpeningState()
{
    #if 0
    int viewed = queryExtraData(char_data_type_daily, char_data_daily_view_new_event);
    if (viewed == 0)
    {
        return 1;
    }
    #endif
    //累计冲
    int    state = recharge_event_mgr::getInstance()->getCanget(this, 2);
    if (state > 0)
    {
        return 1;
    }

    //单笔冲
    state = recharge_event_mgr::getInstance()->getCanget(this, 1);
    if (state > 0)
    {
        return 1;
    }
    //等级礼包
    if (m_newbie_reward_canGet == 1)
    {
        //cout<<m_id<<" - newbie event can get"<<endl;
        return 1;
    }
    return Singleton<new_event_mgr>::Instance().getActionState(m_id);
}

//查询日常活动状态
int CharData::getDailyState()
{
    //阵营战
    int state = 0;
    if (m_campraceOpen && campRaceMgr::getInstance()->isOpen())
    {
        state = 1;
    }
    //BOSS战
    if (state == 0 && m_bossOpen && bossMgr::getInstance()->isOpen())
    {
        state = 1;
    }
    if (state == 0 && m_campraceOpen && queryExtraData(char_data_type_normal, char_data_view_camprace) == 0)
    {
        state = 1;
    }
    if (state == 0 && m_bossOpen && queryExtraData(char_data_type_normal, char_data_view_boss) == 0)
    {
        state = 1;
    }
    return state;
}

//通知开服活动按钮的状态
void CharData::notifyOpeningState()
{
    int state = getOpeningState();

    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        if (state >= 1)
        {
            account->Send("{\"type\":7,\"active\":" + LEX_CAST_STR(state) + ",\"cmd\":\"updateAction\",\"s\":200}");
        }
        else if(state == 0)
        {
            account->Send("{\"type\":7,\"active\":0,\"cmd\":\"updateAction\",\"s\":200}");
        }
    }
    //cout<<"notify opening state "<<state<<",cid:"<<m_id<<endl;
}

void CharData::notifyEventState(int type, int active, int leftNums)
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        if (leftNums)
        {
               account->Send("{\"type\":" + LEX_CAST_STR(type)
                    + ",\"active\":" + LEX_CAST_STR(active)
                    + ",\"leftNums\":" + LEX_CAST_STR(leftNums)
                    + ",\"cmd\":\"updateAction\",\"s\":200}");
        }
        else
        {
            account->Send("{\"type\":" + LEX_CAST_STR(type)
                    + ",\"active\":" + LEX_CAST_STR(active)
                    + ",\"leftNums\":0,\"cmd\":\"updateAction\",\"s\":200}");
        }
    }
}

//通知VIP活动按钮的状态
void CharData::notifyVipState()
{
    int state = getVipState();

#ifdef QQ_PLAT
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        if (state == 2)
        {
            account->Send("{\"type\":17,\"cmd\":\"removeAction\",\"s\":200}");
        }
        else
        {
            account->Send("{\"type\":17,\"active\":" + LEX_CAST_STR(state) + ",\"cmd\":\"updateAction\",\"s\":200}");
        }
    }
#else
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        if (state)
        {
            account->Send("{\"type\":17,\"active\":1,\"cmd\":\"updateAction\",\"s\":200}");
        }
        else
        {
            account->Send("{\"type\":17,\"active\":0,\"cmd\":\"updateAction\",\"s\":200}");
        }
    }
#endif
    //cout<<"notify gift state "<<state<<",cid:"<<m_id<<endl;
}

//通知成长礼包领取状态
void CharData::notifyChengzhangState()
{
    int state = libao_mgr::getInstance()->getChengzhangState(*this);

    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        if (state)
        {
            account->Send("{\"pos\":" + LEX_CAST_STR(state) + ",\"cmd\":\"chengzhangAction\",\"s\":200}");
        }
        else
        {
            account->Send("{\"pos\":0,\"cmd\":\"chengzhangAction\",\"s\":200}");
        }
    }
}

#if 0
//通知礼包活动按钮的状态
void CharData::notifyGiftState()
{
    int state = getGiftState();

    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        if (state)
        {
            account->Send("{\"type\":8,\"active\":1,\"cmd\":\"updateAction\",\"s\":200}");
        }
        else
        {
            account->Send("{\"type\":8,\"active\":0,\"cmd\":\"updateAction\",\"s\":200}");
        }
    }
    //cout<<"notify gift state "<<state<<",cid:"<<m_id<<endl;
}
#endif

//通知礼包活动按钮移除
void CharData::notifyEventRemove(int type)
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
           account->Send("{\"type\":" + LEX_CAST_STR(type) + ",\"cmd\":\"removeAction\",\"s\":200}");
    }
}

bool CharData::checkWeapon()
{
    bool open_new = false;
    //兵器铺开放
    if (!m_weaponOpen && m_currentStronghold >= newWeaponMgr::getInstance()->openStronghold())
    {
        m_weaponOpen = 1;
        open_new = true;
    }
    int openCount = 0;
    for (int type = 1; type <= 5; ++type)
    {
        if (!m_new_weapons._weapons[type-1]._baseWeapon && m_currentStronghold >= newWeaponMgr::getInstance()->openStronghold(type))
        {
            ++openCount;
            InsertSaveDb("insert into char_new_weapons (cid,wType,wid,level) values (" + LEX_CAST_STR(m_id) +
                        "," + LEX_CAST_STR(type) + "," + LEX_CAST_STR(type) + ",1)");
            m_new_weapons._weapons[type-1]._baseWeapon = newWeaponMgr::getInstance()->getDefaultWeapon(type);
            m_new_weapons._weapons[type-1]._level = 1;
            if (m_new_weapons._weapons[type-1]._baseWeapon)
            {
                m_new_weapons._weapons[type-1]._effect = m_new_weapons._weapons[type-1]._baseWeapon->effect(1);
                m_new_weapons._weapons[type-1]._cost = m_new_weapons._weapons[type-1]._baseWeapon->levelCost(m_new_weapons._weapons[type-1]._level);
                //新秘法开启
                if (!open_new)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "notify") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair("type", notify_msg_new_weapon) );
                    obj.push_back( Pair("name", m_new_weapons._weapons[type-1]._baseWeapon->_name) );
                    sendObj(obj);
                }
            }
        }
    }
    if (openCount > 0)
    {
        updateUpgradeWeaponCost();
        updateUpgradeWeaponCDList();
    }
    return open_new;
}

#if 0
bool CharData::checkSouls()
{
    bool open_new = false;
    //练兵开放
    if (!m_soulOpen && m_currentStronghold >= Singleton<trainingMgr>::Instance().openStronghold())
    {
        m_soulOpen = 1;
        open_new = true;
    }
    for (int type = 1; type <= 3; ++type)
    {
        if (!m_training.m_soldier_souls[type-1]._centerSoul && m_currentStronghold >= Singleton<trainingMgr>::Instance().openStronghold(type))
        {
            m_training.m_soldier_souls[type-1].type = type;
            m_training.m_soldier_souls[type-1]._centerSoul = Singleton<trainingMgr>::Instance().getDefaultSoul(type,0);
            //兵魂
            for (int stype = 1; stype <= 5; ++stype)
            {
                m_training.m_soldier_souls[type-1]._soul[stype-1] = Singleton<trainingMgr>::Instance().getDefaultSoul(type,stype);
            }
            InsertSaveDb("insert into char_training (cid,sType,center_id,id_1,id_2,id_3,id_4,id_5) values (" + LEX_CAST_STR(m_id) +
                        "," + LEX_CAST_STR(type) + "," + LEX_CAST_STR(m_training.m_soldier_souls[type-1]._centerSoul->_id)
                        + "," + LEX_CAST_STR(m_training.m_soldier_souls[type-1]._soul[0]->_id)
                        + "," + LEX_CAST_STR(m_training.m_soldier_souls[type-1]._soul[1]->_id)
                        + "," + LEX_CAST_STR(m_training.m_soldier_souls[type-1]._soul[2]->_id)
                        + "," + LEX_CAST_STR(m_training.m_soldier_souls[type-1]._soul[3]->_id)
                        + "," + LEX_CAST_STR(m_training.m_soldier_souls[type-1]._soul[4]->_id)
                        + ")");
        }
    }
    return open_new;
}
#endif

//武将列表
CharTotalGenerals& CharData::GetGenerals()
{
    return m_generals;
}

//阵型列表
CharZhens& CharData::GetZhens()
{
    return m_zhens;
}

CharTempoData& CharData::GetTempo()
{
    return m_tempo;
}

//角色名
const std::string& CharData::GetCharName()
{
    return m_name;
}
//角色id
uint64_t CharData::GetCharId()
{
    return m_id;
}

//工会id
int CharData::GetGuildId()
{
    return m_corps_member.get() ? m_corps_member->corps : 0;
}

//获取当前军粮上限
void CharData::GetMaxSupply(int& need_supply, int& max_supply)
{
    boost::shared_ptr<StrongholdData> shold = GeneralDataMgr::getInstance()->GetStrongholdData(m_currentStronghold + 1);
    if (shold.get())
    {
        need_supply = shold->m_need_supply;
        max_supply = shold->m_need_supply*5;
    }
    else
    {
        shold = GeneralDataMgr::getInstance()->GetStrongholdData(m_currentStronghold);
        if (shold.get())
        {
            need_supply = shold->m_need_supply;
            max_supply = shold->m_need_supply*5;
        }
    }
    if (m_level < 15)
    {
        max_supply = 1000000;
    }
    return;
}

//加入公会
int CharData::JoinGuildId(uint64_t gid)
{
    return 0;
}
//离开工会
int CharData::LeaveGuild()
{
    return 0;
}

//给角色增加或减少黄金
int CharData::addGold(int gold)      //增加减少都可以，返回-1表示不够减，正常返回变化后的数量
{
    if (m_gold + gold >= 0)
    {
        m_gold += gold;
        InsertSaveDb("update char_resource set gold=" + LEX_CAST_STR(m_gold)
                + " where cid=" + LEX_CAST_STR(m_id));
        //金币消费活动
        if (gold < 0)
        {
            shhx_cost_event::getInstance()->update_cost_event(m_id, -gold);
            //周排行活动
            int score = -gold;
            newRankings::getInstance()->updateEventRankings(m_id,rankings_event_gold,score);

            //消费活动
            Singleton<cost_feedback_event>::Instance().update_feedback_gold_event(m_id, gold);
        }
        return m_gold;
    }
    else
    {
        return -1;
    }
}

//给角色增加或减少黄金
int CharData::gold(int gold)      //增加减少都可以，返回-1表示不够减，正常返回变化后的数量
{
    m_gold = gold;
    InsertSaveDb("update char_resource set gold=" + LEX_CAST_STR(m_gold)
            + " where cid=" + LEX_CAST_STR(m_id));
    return m_gold;
}

//给角色增加或减少军令
int CharData::addLing(int ling)
{
    if (m_ling + ling >= 0)
    {
        m_ling += ling;
        InsertSaveDb("update char_resource set ling=" + LEX_CAST_STR(m_ling)
                + " where cid=" + LEX_CAST_STR(m_id));

        if (m_ling == 0)
        {
            //新手引导
            checkGuide(guide_type_no_ling, 0, 0);
        }
        return m_ling;
    }
    else
    {
        return -1;
    }
}

//给角色增加或减少军令
int CharData::ling(int ling)
{
    m_ling = ling;
    InsertSaveDb("update char_resource set ling=" + LEX_CAST_STR(m_ling)
            + " where cid=" + LEX_CAST_STR(m_id));

    if (m_ling == 0)
    {
        //新手引导
        checkGuide(guide_type_no_ling, 0, 0);
    }
    return m_ling;
}

//给角色增加或减少探索军令
int CharData::addExploreLing(int ling)
{
    if (m_explore_ling + ling >= 0)
    {
        m_explore_ling += ling;
        InsertSaveDb("update char_resource set explore_ling=" + LEX_CAST_STR(m_explore_ling)
                + " where cid=" + LEX_CAST_STR(m_id));

        return m_explore_ling;
    }
    else
    {
        return -1;
    }
}

//给角色增加或减少探索军令
int CharData::resetExploreLing()
{
    m_explore_ling = iFreeExploreTimes;
    InsertSaveDb("update char_resource set explore_ling=" + LEX_CAST_STR(m_explore_ling)
            + " where cid=" + LEX_CAST_STR(m_id));

    return m_explore_ling;
}

//给角色增加或减少白银
int CharData::addSilver(int silver, bool buy_back)
{
    if (m_silver + silver >= 0)
    {
        m_silver += silver;
        updateTask(task_silver_nums, m_silver, 0);
        InsertSaveDb("update char_resource set silver=" + LEX_CAST_STR(m_silver)
        + " where cid=" + LEX_CAST_STR(m_id));

        updateEnhanceCDList();
        if (silver < 0 && !buy_back)
        {
            //周排行活动
            int score = -silver / 100;
            newRankings::getInstance()->updateEventRankings(m_id,rankings_event_silver,score);

            //消费活动
            Singleton<cost_feedback_event>::Instance().update_feedback_silver_event(m_id, silver);
        }
        return m_silver;
    }
    else
    {
        return -1;
    }
}

//给角色白银
int CharData::silver(int silver)
{
    m_silver = silver;
    updateTask(task_silver_nums, m_silver, 0);
    InsertSaveDb("update char_resource set silver=" + LEX_CAST_STR(m_silver)
    + " where cid=" + LEX_CAST_STR(m_id));
    return m_silver;
}

int CharData::action_first_recharge(int gold_num)
{
    #if 0
    if (gold_num >= 100)
    {
        std::string strGet = "";
        addSilver(500000);
        add_statistics_of_silver_get(m_id,m_ip_address,500000,silver_get_by_active);
        if (strGet != "")
        {
            strGet += ",";
        }
        strGet += strSilver + strCounts + LEX_CAST_STR(500000);
        addLing(50);
        add_statistics_of_ling_cost(m_id,m_ip_address, 50, ling_active, 1);
        if (strGet != "")
        {
            strGet += ",";
        }
        strGet += strLing + strCounts + LEX_CAST_STR(50);
        /*********** 广播 ************/
        std::string b_msg = strSystemFirstRecharge;
        str_replace(b_msg, "$W", m_name);
        str_replace(b_msg, "$R", strGet);
        GeneralDataMgr::getInstance()->broadCastSysMsg(b_msg, -1);
        return HC_SUCCESS;
    }
    #endif
    return HC_ERROR;
}

//角色升级
int CharData::level(int level)
{
    if (m_level < level)
    {
#ifdef QQ_PLAT
        //腾讯统计
        att_change_tencent(this,"level",LEX_CAST_STR(m_level),LEX_CAST_STR(level));
#endif
        m_level = level;
        //升级处理
        m_levelupTime = time(NULL);    //记录角色升级时间，排名时用到

        //升级符合新手冲锋号领取条件
        if (m_level % 5 == 0)
        {
            //int leftTime = m_createTime + iNewbieGoGoGoSecs - m_levelupTime;
            //if (leftTime > 0)
            {
                //更新新手冲锋号是否可以获得
                updateNewbieEventState();
                //通知开服活动按钮状态
                notifyOpeningState();
            }
        }
        //升级，广播给好友，受祝贺
        if (m_level == 15 || level >= 50 || 20 == m_level || 30 == m_level || 40 == m_level)
        {
            Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_LEVEL, m_level, 0);
        }

        //好友推送
        if (10 == m_level || 15 == m_level || 26 == m_level)
        {
            if (GeneralDataMgr::getInstance()->canRecommendFriends(this))
            {
                m_recommend_friend_refresh = 0;
                //通知被请求玩家
                sendSystemMail(m_name, m_id, strRecommendFriends, strRecommendFriendsContent,"", 0, 2, 0);
            }
        }
        #if 0
        //升级广播
        if (m_level % 10 == 0)
        {
            /*********** 广播 ************/
            std::string msg = strSystemLevelUpMsg;
            str_replace(msg, "$N", MakeCharNameLink(m_name));
            str_replace(msg, "$L", LEX_CAST_STR(m_level));
            //只向所在地图广播
            GeneralDataMgr::getInstance()->broadCastSysMapMsg(msg, -1, m_area);
        }
        #endif
        //武将宝石数量改为跟角色等级挂钩
        if (updateBaoshiCount())
        {
            updateGeneralsBaoshiCount();
            //提示 开启镶嵌新空格
            //第一个开启不通知
            if (m_baoshi_count >= 2)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("cmd", "notify") );
                obj.push_back( Pair("s", 200) );
                obj.push_back( Pair("type", notify_msg_more_xiangqian) );
                obj.push_back( Pair("nums", m_baoshi_count) );
                sendObj(obj);
            }
        }

        //七日目标
        Singleton<seven_Goals_mgr>::Instance().updateGoals(*this,queryCreateDays(),goals_type_level);

        //更新任务
        updateTask(task_char_level, m_level);
        //checkGuide(guide_type_choose_camp, 0, 0);
        //新手引导
        checkGuide(guide_type_char_level, m_level, 0);

        InsertSaveDb("update charactors set level=" + LEX_CAST_STR(m_level)
                    + ",lastlogin=" + LEX_CAST_STR(m_login_time) + " where id=" + LEX_CAST_STR(m_id));

        //攻擊力變化
        set_attack_change();

        updateEnhanceCost();
        updateEnhanceCDList();
        updateUpgradeWeaponCost();
        updateUpgradeWeaponCDList();

        if (iOpenNearMap <= m_area)
        {
            std::string msg = strNotifyLevelup;
            str_replace(msg, "$C", LEX_CAST_STR(m_id));
            str_replace(msg, "$L", LEX_CAST_STR(m_level));
            str_replace(msg, "$N", m_name);
            str_replace(msg, "$G", LEX_CAST_STR(m_gender));
            boost::shared_ptr<ChatChannel> ch = GeneralDataMgr::getInstance()->GetMapChannel(m_area);
            if (ch.get())
            {
                ch->BroadMsg(msg);
            }
        }
#ifndef QQ_PLAT    //V5专属武将
        if (level == 30 && m_vip >= 5)
        {
            notifyEventState(top_level_event_vip_present, 1, 0);
        }
#else
        if (m_close_friend_to > 0)
        {
            Singleton<inviteMgr>::Instance().close_friend_levelup(m_close_friend_to, m_id, m_level);
        }
#endif

    //新手福利是否需要过期通知
    if (!isNewPlayer() && queryExtraData(char_data_type_normal, char_data_new_player_end) == 0)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "notify") );
        obj.push_back( Pair("s", 200) );
        obj.push_back( Pair("type", notify_msg_new_player_end) );
        sendObj(obj);
    }
    //成长礼包状态
    notifyChengzhangState();

    }
    return m_level;
}

int CharData::saveCharDailyVar()
{
    InsertSaveDb("replace into char_daily_temp (cid,goldRest,refresh_explore,hasgetSalary,buy_xisui_time,levy_time,jishi_time,corps_yanhui) values (" + LEX_CAST_STR(m_id)
            + "," + LEX_CAST_STR(m_gold_rest)
            + "," + LEX_CAST_STR(m_explore_refresh_times)
            + "," + LEX_CAST_STR(m_hasgetsalary)
            + "," + LEX_CAST_STR(m_buy_xisui_time)
            + "," + LEX_CAST_STR(m_levy_time)
            + "," + LEX_CAST_STR(m_temp_jisi_times)
            + "," + LEX_CAST_STR(m_temp_corps_yanhui)
            + ")");
    return 0;
}

void CharData::SaveWeapons(int type)
{
    if (type < 1 || type > 5)
    {
        //保存兵器信息
        for (int i = 0; i < 5; ++i)
        {
            if (m_new_weapons._weapons[i]._baseWeapon)
            {
                InsertSaveDb("update char_new_weapons set wid=" + LEX_CAST_STR(m_new_weapons._weapons[i]._baseWeapon->_id)
                + ",level=" + LEX_CAST_STR(m_new_weapons._weapons[i]._level)
                + " where cid=" + LEX_CAST_STR(m_id) + " and wType=" + LEX_CAST_STR(i+1)
                );
            }
        }
    }
    else
    {
        if (m_new_weapons._weapons[type-1]._baseWeapon)
        {
            InsertSaveDb("update char_new_weapons set wid=" + LEX_CAST_STR(m_new_weapons._weapons[type-1]._baseWeapon->_id)
                    + ",level=" + LEX_CAST_STR(m_new_weapons._weapons[type-1]._level)
                    + " where cid=" + LEX_CAST_STR(m_id) + " and wType=" + LEX_CAST_STR(type)
                    );
        }
    }
}

//保存角色信息
int CharData::Save()
{
    //INFO("***************** save "<<m_id <<" *************************");
    m_save_time = time(NULL);
    m_generals.Save();
    m_zhens.Save();
    //真实VIP比临时VIP大则储存VIP信息
    if (m_vip > m_tmp_vip)
    {
        //保存角色等级喊话内容
        InsertSaveDb("update char_data set levelupTime=" + LEX_CAST_STR(m_levelupTime)
            + ",freeRest=" + LEX_CAST_STR(m_free_rest)
            + ",freeRestTime=" + LEX_CAST_STR(m_free_rest_time)
            + ",finish_first_explore=" + LEX_CAST_STR(m_first_explore)
            + ",finish_second_explore=" + LEX_CAST_STR(m_second_explore)
            + ",mapid=" + LEX_CAST_STR(m_area)
            + ",vip=" + LEX_CAST_STR(m_vip)
            + ",prestige=" + LEX_CAST_STR(m_prestige)
            + ",official=" + LEX_CAST_STR(m_offical)
            + ",camp=" + LEX_CAST_STR(m_camp)
            + ",chat='" + GetDb().safestr(m_chat) + "'  where cid=" + LEX_CAST_STR(m_id));
    }
    else
    {
        //保存角色等级喊话内容
        InsertSaveDb("update char_data set levelupTime=" + LEX_CAST_STR(m_levelupTime)
            + ",freeRest=" + LEX_CAST_STR(m_free_rest)
            + ",freeRestTime=" + LEX_CAST_STR(m_free_rest_time)
            + ",finish_first_explore=" + LEX_CAST_STR(m_first_explore)
            + ",finish_second_explore=" + LEX_CAST_STR(m_second_explore)
            + ",mapid=" + LEX_CAST_STR(m_area)
            + ",prestige=" + LEX_CAST_STR(m_prestige)
            + ",official=" + LEX_CAST_STR(m_offical)
            + ",camp=" + LEX_CAST_STR(m_camp)
            + ",chat='" + GetDb().safestr(m_chat) + "'  where cid=" + LEX_CAST_STR(m_id));
    }

    InsertSaveDb("update charactors set level=" + LEX_CAST_STR(m_level)
        + ",lastlogin=" + LEX_CAST_STR(m_login_time)
        + ",chenmi_time=" + LEX_CAST_STR(m_chenmi_time>18000?18000:m_chenmi_time)
        + ",continue_days=" + LEX_CAST_STR(m_continue_days)
        + ",total_continue_days=" + LEX_CAST_STR(m_total_continue_days)
        + " where id=" + LEX_CAST_STR(m_id));

    /*保存资源、金币
    InsertSaveDb("update char_resource set gold=" + LEX_CAST_STR(m_gold)
        + ",silver=" + LEX_CAST_STR(m_silver)
        + ",ling=" + LEX_CAST_STR(m_ling)
        + ",explore_ling=" + LEX_CAST_STR(m_explore_ling)
        + " where cid=" + LEX_CAST_STR(m_id));*/
    /*保存兵器信息
    for (int i = 0; i < 5; ++i)
    {
        if (m_new_weapons._weapons[i]._baseWeapon)
        {
            InsertSaveDb("update char_new_weapons set wid=" + LEX_CAST_STR(m_new_weapons._weapons[i]._baseWeapon->_id)
            + ",level=" + LEX_CAST_STR(m_new_weapons._weapons[i]._level)
            + " where cid=" + LEX_CAST_STR(m_id) + " and wType=" + LEX_CAST_STR(i+1)
            );
        }
    }*/
    //金币休息次数
    //saveCharDailyVar();

    //装备
    //道具
    //m_backpack.Save();

    //保存技能

    //保存技能研究队列

    //关卡进度
    //技能研究者
    if (m_teachers_change)
    {
        m_teachers_change = false;
        for (int i = 0; i < skill_teacher_nums; ++i)
        {
            int tid = 0;
            if (m_teachers[i].get())
            {
                tid = m_teachers[i]->id;
            }
            InsertSaveDb("replace into char_skill_teachers (cid,teacher,id) values ("
                     + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(tid) + ","
                     + LEX_CAST_STR(i+1) + ")");
        }
    }
    //训练兵书
    if (m_book_change)
    {
        m_book_change = false;
        for (int i = 0; i < general_book_nums; ++i)
        {
            if (m_book[i].get())
            {
                InsertSaveDb("replace into char_train_books (cid,bid,pos) values ("
                     + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(m_book[i]->id) + ","
                     + LEX_CAST_STR(i+1) + ")");
            }
        }
    }
    //关卡进度保存
    //m_tempo.Save();
    //探索信息保存
    exploreMgr::getInstance()->Save(m_id);

    return 0;
}

int CharData::HeartBeat()
{
    return 0;
}

//查询角色信息
int CharData::getCharInfo(json_spirit::Object& charobj)
{
    int timenow = time(NULL);
    int corps = 0;
    if (m_corps_member.get())
    {
        corps = m_corps_member->corps;
        splsCorps* cp = corpsMgr::getInstance()->findCorps(corps);
        if (cp)
        {
            charobj.push_back( Pair("corp", cp->_name) );
        }
    }
    //加入角色等级信息、黄金、白银、军令
    charobj.push_back( Pair("id", m_id));
    charobj.push_back( Pair("spic", m_spic));
    charobj.push_back( Pair("gid", corps));
    charobj.push_back( Pair("vip", m_vip));
    if (isNewPlayer()>0)
    {
        charobj.push_back( Pair("tmpvip_lefttime", isNewPlayer()));
    }
    charobj.push_back( Pair("name", m_name));
    charobj.push_back( Pair("gold", m_gold));
    charobj.push_back( Pair("silver", m_silver));
    charobj.push_back( Pair("ling", m_ling));
    charobj.push_back( Pair("maxLing", iSystemRecoverMaxLing));
    charobj.push_back( Pair("prestige", m_prestige));
    charobj.push_back( Pair("supply", m_bag.getGemCount(treasure_type_supply)));
    charobj.push_back( Pair("gongxun", getGongxun()) );
    charobj.push_back( Pair("yushi", treasureCount(treasure_type_yushi)) );
    int need_supply = 0, max_supply = 0;
    GetMaxSupply(need_supply,max_supply);
    charobj.push_back( Pair("need_supply", need_supply) );
    charobj.push_back( Pair("max_supply", max_supply) );
    boost::shared_ptr<StrongholdData> shold = GeneralDataMgr::getInstance()->GetStrongholdData(m_currentStronghold + 1);
    if (shold.get())
    {
        charobj.push_back( Pair("cur_mapid", shold->m_map_id) );
        charobj.push_back( Pair("cur_stageid", shold->m_stage_id) );
        charobj.push_back( Pair("cur_strongholdid", shold->m_id) );
        charobj.push_back( Pair("finish_all_stronghold", false) );
    }
    else
    {
        shold = GeneralDataMgr::getInstance()->GetStrongholdData(m_currentStronghold);
        if (shold.get())
        {
            charobj.push_back( Pair("cur_mapid", shold->m_map_id) );
            charobj.push_back( Pair("cur_stageid", shold->m_stage_id) );
            charobj.push_back( Pair("cur_strongholdid", shold->m_id) );
            charobj.push_back( Pair("finish_all_stronghold", true) );
        }
    }

    //官职开放了就有官职当前官职可以是0
    if (m_officeOpen)
    {
        charobj.push_back( Pair("officer", m_offical_name) );
        charobj.push_back( Pair("canLevelUp", m_officalcanlevelup) );
    }

    json_spirit::Object area;
    area.push_back( Pair("area", m_area));
    area.push_back( Pair("name", m_area_name));
    area.push_back( Pair("memo", m_area_memo));
    std::string area_name = GeneralDataMgr::getInstance()->GetStageName(m_area,m_cur_stage);
    area.push_back( Pair("stage_name", area_name));

    charobj.push_back( Pair("area", area));
    charobj.push_back( Pair("camp", m_camp));

    charobj.push_back( Pair("level", m_level));

    charobj.push_back( Pair("openNext", m_level >= (m_area * 20)));
    //部队信息
    int attack = 0;
    if (m_weapon_attack_change)
    {
        updateAttackDefense();
    }
    boost::shared_ptr<ZhenData> zdata = m_zhens.GetZhen(m_zhens.GetDefault());
    if (zdata.get())
    {
        attack = zdata->getAttack();
    }
    charobj.push_back( Pair("attack", attack) );
    charobj.push_back( Pair("bag_full", m_bag.isFull()) );

    return HC_SUCCESS;
}

//查询角色的按钮开放信息
int CharData::getPanelInfo(json_spirit::Object& panelobj)
{
    //按钮信息
    panelobj.push_back( Pair("zhuj", m_panel_zhuj));
    panelobj.push_back( Pair("army", m_panel_army));
    panelobj.push_back( Pair("junt", m_panel_junt));
    panelobj.push_back( Pair("interior", m_panel_interior));
    return HC_SUCCESS;
}

//通知客户端角色信息发生变化
int CharData::NotifyCharData()
{
    m_char_data_change = true;
}

//通知客户端角色信息发生变化
int CharData::NotifyCharData_(net::session_ptr& sk)
{
    if (m_char_data_change)
    {
        m_char_data_change = false;

        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "getRoleInfo"));
        obj.push_back( Pair("s",200) );
        json_spirit::Object cobj;
        getCharInfo(cobj);
        obj.push_back( Pair("chardata", cobj) );
        json_spirit::Object pobj;
        getPanelInfo(pobj);
        obj.push_back( Pair("panel", pobj) );
        sk->send(json_spirit::write(obj, json_spirit::raw_utf8));
    }
    return HC_SUCCESS;
}

//查询角色的阵型武将信息
int CharData::getZhenGeneral(int zhenid, json_spirit::Object& robj, int type, int id)
{
    CharZhens& char_zhens = GetZhens();
    json_spirit::Array generallist;
    int total = 0;
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = char_zhens.m_zhens.find(zhenid);
    if (it != char_zhens.m_zhens.end())
    {
        robj.push_back( Pair("level", it->second->m_level) );
        for (size_t i = 0; i < 9; ++i)
        {
            json_spirit::Object generalobj;
            CharTotalGenerals& char_generals = GetGenerals();
            std::map<int, boost::shared_ptr<CharGeneralData> >::iterator itd = char_generals.m_generals.find(it->second->m_generals[i]);
            if (itd != char_generals.m_generals.end())
            {
                boost::shared_ptr<CharGeneralData> gd = itd->second;
                boost::shared_ptr<GeneralTypeData> base_gd = GeneralDataMgr::getInstance()->GetBaseGeneral(gd->m_gid);
                boost::shared_ptr<BaseSoldierData> base_sd = GeneralDataMgr::getInstance()->GetBaseSoldier(gd->m_stype);

                if (gd.get() && base_gd.get() && base_sd.get())
                {
                    generalobj.push_back( Pair("id", gd->m_id));
                    generalobj.push_back( Pair("level", gd->m_level));
                    generalobj.push_back( Pair("color", gd->m_color));
                    //generalobj.push_back( Pair("isOriginal", gd->m_gid <= 6));
                    generalobj.push_back( Pair("name", base_gd->m_name));
                    generalobj.push_back( Pair("spic", base_gd->m_spic));
                    //擅长
                    generalobj.push_back( Pair("good_at", base_gd->m_good_at) );

                    if (type > 0 && id > 0)
                    {
                        if (base_gd->m_speSkill.get())
                        {
                            generalobj.push_back( Pair("spe_name", base_gd->m_speSkill->name));
                            generalobj.push_back( Pair("spe_memo", base_gd->m_speSkill->memo));
                        }
                        json_spirit::Object soldierobj;
                        soldierobj.push_back( Pair("type", base_sd->m_base_type));
                        soldierobj.push_back( Pair("attack_type", base_sd->m_damage_type));
                        soldierobj.push_back( Pair("name", base_sd->m_name));
                        soldierobj.push_back( Pair("memo", base_sd->m_desc));
                        generalobj.push_back( Pair("soldier", soldierobj));

                        //攻击范围...
                        if (base_gd->m_speSkill.get() && base_gd->m_speSkill->action_type == 1)
                        {
                            int atype = base_gd->m_speSkill->attack_type;
                            int pos[9];
                            memset(pos, 0, sizeof(int)*9);
                            if (type == 1)
                            {
                                boost::shared_ptr<StrongholdData> sh = GeneralDataMgr::getInstance()->GetStrongholdData(id);
                                if (sh.get())
                                {
                                    for (int i = 0; i < 9; ++i)
                                    {
                                        if (sh->m_generals[i].get())
                                        {
                                            pos[i] = 1;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                boost::shared_ptr<eliteCombat> sh = eliteCombatMgr::getInstance()->getEliteCombatById(id);
                                if (sh.get())
                                {
                                    for (int i = 0; i < 9; ++i)
                                    {
                                        if (sh->m_generals[i].get())
                                        {
                                            pos[i] = 1;
                                        }
                                    }
                                }
                            }
                            json_spirit::Array v = getAttackRange(atype, i+1, pos);
                            generalobj.push_back( Pair("range", v) );
                        }
                    }
                    ++total;
                }
            }
            generalobj.push_back( Pair("position", it->second->m_generals[i]));

            generallist.push_back(generalobj);
        }
    }
    robj.push_back( Pair("list", generallist));
    //已经上阵人数，可以上阵人数
    robj.push_back( Pair("cur", total));
    robj.push_back( Pair("max", m_up_generals));
    return HC_SUCCESS;
}

//通知客户端阵型武将信息发生变化
int CharData::NotifyZhenData()
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "changeFormation"));
        obj.push_back( Pair("s",200) );
        getZhenGeneral(m_zhens.m_default_zhen, obj);
        obj.push_back( Pair("attack", getAttack(m_zhens.m_default_zhen)));
        account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
        return HC_SUCCESS;
    }
    return 0;
}

void CharData::notifyChangeMap()
{
    if (m_area >= iOpenNearMap)
    {
        boost::shared_ptr<ChatChannel> ch = GeneralDataMgr::getInstance()->GetMapChannel(m_area);
        if (ch.get())
        {
            std::string msg = strNotifyEnter;
            str_replace(msg, "$C", LEX_CAST_STR(m_id));
            str_replace(msg, "$L", LEX_CAST_STR(m_level));
            str_replace(msg, "$N", (m_name));
            str_replace(msg, "$G", LEX_CAST_STR(m_gender));
            ch->BroadMsg(msg);

            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
            if (account.get())
            {
                ch->Add(account);
            }
        }
        if (m_area > 1)
        {
            ch = GeneralDataMgr::getInstance()->GetMapChannel(m_area - 1);
            //离开地图频道
            if (ch.get())
            {
                ch->Remove(m_id);
                if ((m_area-1) >= iOpenNearMap)
                {
                    std::string msg = strNotifyLeave;
                    str_replace(msg, "$C", LEX_CAST_STR(m_id));
                    str_replace(msg, "$L", LEX_CAST_STR(m_level));
                    ch->BroadMsg(msg);
                }
            }
        }
    }
    else
    {
        boost::shared_ptr<ChatChannel> ch = GeneralDataMgr::getInstance()->GetMapChannel(m_area);
        if (ch.get())
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
            if (account.get())
            {
                ch->Add(account);
            }
        }
        if (m_area > 1)
        {
            ch = GeneralDataMgr::getInstance()->GetMapChannel(m_area - 1);
            //离开地图频道
            if (ch.get())
            {
                ch->Remove(m_id);
            }
        }
    }
}

//更新功能开放
int CharData::updateOpen()
{
    bool open_new = false;
    //屯田位置开启
    for (int i = 0; i < 6; ++i)
    {
        if (m_currentStronghold < iFarmOpenStronghold[i])
        {
            continue;
        }
        if (i == 0 && m_farmOpen == 0)
        {
            m_panel_interior = 1;
            m_farmOpen = 1;
            open_new = true;
        }
        farmMgr::getInstance()->open(m_id);
        boost::shared_ptr<fieldlist> p_fl = farmMgr::getInstance()->GetCharFieldList(m_id);
        if (p_fl.get() && i < (int)(*p_fl).size())
        {
            if ((*p_fl)[i].get() && (*p_fl)[i]->m_state == field_lock)
            {
                (*p_fl)[i]->m_state = field_free;
                (*p_fl)[i]->start_nourish();
                (*p_fl)[i]->save();
            }
        }
        //新版本增加屯田数量老号处理
        if (p_fl.get() && i >= (int)(*p_fl).size())
        {
            for (int j = i+1; j <= 6; ++j)
            {
                int state = field_lock;
                //等级判断
                if (m_currentStronghold >= iFarmOpenStronghold[j - 1])
                    state = field_free;
                boost::shared_ptr<field> p_f;
                p_f.reset(new field);
                p_f->m_cid = m_id;
                p_f->m_pos = j;
                p_f->m_state = state;
                p_f->m_type = 0;
                p_f->m_start_time = 0;
                p_f->m_end_time = 0;
                p_f->m_reward_num = 0;
                p_f->m_left_num = 0;
                p_f->m_need_level = iFarmOpenLevel[j - 1];
                p_f->m_supply = 1000;
                if (p_f->m_state == field_free)
                    p_f->start_nourish();
                (*p_fl).push_back(p_f);
                InsertSaveDb("insert into char_farm_field (cid,pos,state,nourish_time) values (" + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(j) + "," + LEX_CAST_STR(state) + ",unix_timestamp())");
            }
        }
    }

    //贸易开启
    if (m_currentStronghold == iTradeOpenStronghold)
    {
        m_tradeOpen = 1;
        open_new = true;
        //_checkGuide(guide_id_trade);
    }

    //阵型开放
    if (m_currentStronghold == iZhenOpenStronghold)
    {
        //cout<<m_id<<"->open zhen"<<endl;
        m_panel_army = 1;
        m_zhenOpen = 1;
        open_new = true;
    }

    //兵器铺开放
    if (checkWeapon())
    {
        //cout<<m_id<<"->open weapon"<<endl;
        open_new = true;
        m_weaponOpen = 1;
    }
    for (int i = 1; i <= 3; ++i)
    {
        if (m_currentStronghold < Singleton<trainingMgr>::Instance().openStronghold(i))
        {
            continue;
        }
        if (i == 1 && m_soulOpen == 0)
        {
            m_soulOpen = 1;
            open_new = true;
        }
        boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(*this);
        if (ct.get())
        {
            ct->checkSouls(*this);
        }
    }
    //扫荡开放
    if (m_currentStronghold >= iSweepOpenStronghold)
    {
        open_new = true;
        m_sweepOpen = 1;
    }

    //装备系统开放
    if (m_currentStronghold == iEquipOpenStronghold)
    {
        m_equiptOpen = 1;
        open_new = true;

        //std::string tmp;
        //赠送一个铁指环
        //addEquipt(1, tmp); 取消
    }
    //装备强化开放
    if (m_currentStronghold == iEquipEnhanceOpenStronghold)
    {
        m_equiptEnhanceOpen = 1;
        open_new = true;
        //_checkGuide(guide_id_enhance);
    }
    //征收开放
    if (m_currentStronghold == iLevyOpenStronghold)
    {
        open_new = true;
        m_levyOpen = 1;
        //_checkGuide(guide_id_levy);
    }
    //BOSS开放
    if (m_currentStronghold == iBossOpenStronghold)
    {
        open_new = true;
        m_bossOpen = 1;
    }
    //英雄训练
    if (m_currentStronghold == iTrainOpenStronghold)
    {
        //训练开放
        openTrain();
        m_trainOpen = 1;
        open_new = true;
        //_checkGuide(guide_id_train);
    }
    //技能训练
    if (m_currentStronghold == iSkillOpenStronghold)
    {
        m_skillTrainOpen = 1;
        m_skillOpen = 1;
        open_new = true;
    }
    //探索
    if (m_currentStronghold == iExploreOpenStronghold)
    {
        //探索开放
        exploreMgr::getInstance()->ExploreRefresh(m_id);
        //插入玩家探索表
        InsertSaveDb("insert into char_explore_can set cid=" + LEX_CAST_STR(m_id));
        InsertSaveDb("insert into char_explore_has set cid=" + LEX_CAST_STR(m_id));
        exploreMgr::getInstance()->Save(m_id);
        m_exploreOpen = 1;
        open_new = true;
    }
    //官职系统
    //if (m_currentStronghold == iOfficalOpenStronghold)
    //{
    //    m_officeOpen = 1;
    //}
    //精英战役开放
    if (m_currentStronghold == iEliteOpenStronghold)
    {
        open_new = true;
        m_eliteOpen = 1;
    }
    //竞技场开放
    if (m_currentStronghold == iRaceOpenStronghold)
    {
        open_new = true;
        //cout<<m_id<<"->open race"<<endl;
        m_raceOpen = 1;
        RaceMgr::getInstance()->updateZone(m_id);
        //_checkGuide(guide_id_race);
    }
    //军团开放
    if (m_currentStronghold == iCorpsOpenStronghold)
    {
        m_corpsOpen = 1;
        m_panel_junt = 1;
        open_new = true;
    }
    //护送粮草开放
    if (m_currentStronghold == iGuardOpenStronghold)
    {
        open_new = true;
        m_guardOpen = 1;
        guardMgr::getInstance()->open(m_id);
        //_checkGuide(guide_id_guard);
    }

    //神秘商店开放
    if (m_currentStronghold == iShopOpenStronghold)
    {
        m_shop.Load();
        m_shopOpen = 1;
        open_new = true;
        //_checkGuide(guide_id_shop);
    }
    //洗髓开放
    if (m_currentStronghold == iWashOpenStronghold)
    {
        m_washOpen = 1;
        open_new = true;
        m_wash_open_time = time(NULL);
        setExtraData(char_data_type_normal, char_data_wash_start_time, m_wash_open_time);
        //_checkGuide(guide_id_wash);
    }
    if (m_currentStronghold == iRebornOpenStronghold)
    {
        m_rebornOpen = 1;
        //_checkGuide(guide_id_reborn);
        //重生开放刷红书，秒训练CD
        m_book[0] = TrainMgr::getInstance()->GetBook(18);
        if (m_train_queue.size() > 0 && m_train_queue[0].state == 2)
        {
            splsTimerMgr::getInstance()->delTimer(m_train_queue[0]._uuid);
            int pos = 1;
            {
                ++(m_train_queue[pos - 1].speed_time);
                m_train_queue[pos - 1].state = 1;
                m_train_queue[pos - 1].general.reset();
                m_train_queue[pos - 1].start_time = 0;
                m_train_queue[pos - 1].end_time = 0;
                m_train_queue[pos - 1].save();
                m_train_queue[pos-1]._uuid = boost::uuids::nil_uuid();
            }
        }
    }
    //战马开放
    if (m_currentStronghold == iHorseOpenStronghold)
    {
        m_horseOpen = 1;
        m_horse.cid = m_id;
        m_horse.horseid = 1;
        m_horse.exp = 0;
        m_horse.start_time = time(NULL);
        m_horse.end_time = m_horse.start_time + iHorseActionTime;
        m_horse.horse = horseMgr::getInstance()->getHorse(m_horse.horseid);
        InsertSaveDb("insert into char_horses set cid=" + LEX_CAST_STR(m_id) + ",horseid='1'");
        m_horse.updateActionFruit();
        m_horse.save();
        open_new = true;
        //_checkGuide(guide_id_horse);
    }

    if (m_currentStronghold == iHelperOpenStronghold)
    {
        m_helperOpen = 1;    //助手开放
        //通知助手开放
        open_new = true;

    }
    if (m_currentStronghold == iCampraceOpenStronghold)
    {
        m_campraceOpen = 1;//阵营战开放
        //通知阵营战开放
        open_new = true;
    }
    //购买军令开放
    if (m_currentStronghold == iBuyLingOpenStronghold)
    {
        m_buyLingOpen = 1;
        open_new = true;
    }
    if (m_currentStronghold == iRankEventOpenStronghold)
    {
        m_rankEventOpen = 1;
        open_new = true;
    }
    if (m_currentStronghold == iBankOpenStronghold)
    {
        m_bankOpen = 1;
        bankMgr::getInstance()->open(m_id);
        open_new = true;
    }
    if (m_currentStronghold == iSevenOpenStronghold)
    {
        m_sevenOpen = 1;
        open_new = true;
    }

    if (m_currentStronghold == iJxlOpenStronghold)
    {
        m_jxlOpen = 1;
        open_new = true;
        m_jxl_buff.load();
    }

    if (m_currentStronghold == iGeneralSoulOpenStronghold)
    {
        m_generalSoulOpen = 1;
        open_new = true;
    }

#if 0
    //连续登录奖励开启统计
    time_t continue_login_gift_end = queryExtraData(char_data_type_normal, char_data_get_continue_login_day);
    if (m_currentStronghold == iContinueLoginStronghold && time(NULL) < continue_login_gift_end)
    {
        m_continue_days = 1;
        m_total_continue_days = 1;
        InsertSaveDb("update charactors set continue_days=" + LEX_CAST_STR(m_continue_days)
                + " where id=" + LEX_CAST_STR(m_id));
        m_login_present.clear();
        saveDbJob job;
        job.sqls.push_back("delete from char_continue_login_present where cid=" + LEX_CAST_STR(m_id));
        for (int i = 1; i <= 7; ++i)
        {
            CharLoginPresent clp;
            clp.cid = m_id;
            clp.present = GeneralDataMgr::getInstance()->getBaseLoginPresent(i);
            if (i == 1)
            {
                job.sqls.push_back("insert into char_continue_login_present set state=1,cid=" + LEX_CAST_STR(m_id) + ",pid=" + LEX_CAST_STR(i));
                clp.state = 1;
            }
            else
            {
                job.sqls.push_back("insert into char_continue_login_present set cid=" + LEX_CAST_STR(m_id) + ",pid=" + LEX_CAST_STR(i));
                clp.state = 0;
            }
            m_login_present[i] = clp;
        }
        InsertSaveDb(job);
        //更新礼包活动按钮
        notifyGiftState();
    }
#endif
    if (m_currentStronghold == iFirstRechargeStronghold)
    {
        if (queryExtraData(char_data_type_normal, char_data_first_recharge_gift) == 1
            || queryExtraData(char_data_type_daily, char_data_daily_view_first_recharge) == 0)
        {
            //更新礼包活动按钮
            notifyEventState(top_level_event_first_recharge, 1, 0);
        }
        else
        {
            notifyEventState(top_level_event_first_recharge, 0, 0);
        }
    }
    //在线奖励开启
    time_t onlinegift_end = queryExtraData(char_data_type_normal, char_data_get_onlinegift_day);
    if (m_currentStronghold == iOnlineGiftStronghold && time(NULL) < onlinegift_end)
    {
        //在线礼包开始计时
        boost::shared_ptr<char_online_gift> cog = online_gift_mgr::getInstance()->getChar(m_id);
        if (cog.get() && cog->_canGet == 0)
        {
            cog->reset();
        }
    }
    //壮丁开启
    if (m_currentStronghold == iServantRealOpenStronghold)
    {
        servantMgr::getInstance()->open(m_id);
    }
    if (m_currentStronghold == iServantOpenStronghold)
    {
        open_new = true;
        m_servantOpen = 1;
        //servantMgr::getInstance()->open(m_id);
        //_checkGuide(guide_id_servant);
    }
    /*
    场次名称    等级段
    初级竞技场    30-40
    中级竞技场    40-60
    高级竞技场    60-80
    精英竞技场    80-100
    大师竞技场    100+

    if (m_level == iRaceOpenLevel || m_level == 40 || m_level == 60 || m_level == 80)
    {
        open_new = true;
    }*/
    //宝石镶嵌开放的时候，赠送玉石
    if (m_currentStronghold == iXiangqianOpenStronghold)
    {
        m_baowuOpen = 1;
        open_new = true;
        addTreasure(treasure_type_yushi, 8000);
#ifdef QQ_PLAT
        gold_cost_tencent(this,0,gold_cost_for_convert_jade,treasure_type_yushi,8000);
#endif
        //_checkGuide(guide_id_xiangqian);
    }
    //八卦阵开放
    if (m_currentStronghold == iMazeOpenStronghold)
    {
        open_new = true;
    }
    //第一张图通关，开放小型战役
    if (m_currentStronghold == STRONGHOLD_ID(1,3,8))
    {
        open_new = true;
    }
    //开启新功能调用接口主动发送给客户端
    if (open_new)
    {
        updatePanelOpen();
        NotifyCharOpenInfo();
        //NotifyCharData();
    }

    return 0;
}

#if 0
//通知客户端角色经验信息发生变化
int CharData::NotifyCharExp()
{
    //INFO("************NotifyCharData()"<<m_name);
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "getRoleExp"));
        obj.push_back( Pair("s",200) );
        obj.push_back( Pair("curExp", m_exp) );
        if (m_level < iMaxCharLevel)
        {
            obj.push_back( Pair("levelupExp", get_level_cost(m_level + 1)) );
        }
        else
        {
            obj.push_back( Pair("levelupExp", get_level_cost(iMaxCharLevel)) );
        }
        account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
        return HC_SUCCESS;
    }
    return 0;
}
#endif

//新开放功能的通知
int CharData::NotifyCharOpenInfo()
{
    m_open_info_change = true;
    return HC_SUCCESS;
}

void CharData::realNotifyOpenInfo(net::session_ptr& sk)
{
    if (m_open_info_change)
    {
        m_open_info_change = false;
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "getOpenInfo") );
        obj.push_back( Pair("s", 200) );
        getOpeninfo(obj);
        sk->send(json_spirit::write(obj, json_spirit::raw_utf8));

        obj.clear();
        obj.push_back( Pair("cmd", "getUpdateList") );
        obj.push_back( Pair("s", 200) );
        getUpdateListCD(obj);
        sk->send(json_spirit::write(obj, json_spirit::raw_utf8));

        obj.clear();
        obj.push_back( Pair("cmd", "getActionInfo") );
        obj.push_back( Pair("s", 200) );
        json_spirit::Array elist;
        getAction(this, elist);
        obj.push_back( Pair("list", elist) );
        sk->send(json_spirit::write(obj, json_spirit::raw_utf8));
    }
}

int CharData::sendObj(json_spirit::Object& obj)
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        account->Send(json_spirit::write(obj));
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

#if 0
int CharData::NotifyCharState()
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        json_spirit::Object robj;
        robj.push_back( Pair("cmd", "getOpenStateList") );
        robj.push_back( Pair("s", 200) );
        json_spirit::Array slist;
        m_newStates.getStateInfo(slist);
        json_spirit::Object info;
        m_newStates.getCostInfo(info);
        robj.push_back( Pair("info", info) );
        robj.push_back( Pair("list", slist) );
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return HC_SUCCESS;
}
#endif

//检查战败提示引导
int CharData::_checkNotifyFail(int strongholdid)
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        int notify_id = 0, notify_type = 0;
        if (strongholdid == 9)//鲁智深
        {
            notify_id = 3;
        }
        else if(strongholdid == 25)//陆谦
        {
            notify_id = 4;
            if (m_new_weapons._weapons[0]._level < 5 || m_new_weapons._weapons[1]._level < 5)//武器等级判断
            {
                notify_type = 1;
            }
            else if (m_generals.GetGenralLevel(7) < 12)//鲁智深等级
            {
                notify_type = 2;
            }
            else
            {
                notify_type = 3;
            }
        }
        else if(strongholdid == 50)//杨志
        {
            notify_id = 5;
            if (getSkillLevel(1) < 10 || getSkillLevel(4) < 10 || getSkillLevel(5) < 10 || getSkillLevel(9) < 10)//技能等级
            {
                notify_type = 1;
            }
            else if (m_new_weapons._weapons[0]._level < 8 || m_new_weapons._weapons[1]._level < 8
                    || m_new_weapons._weapons[3]._level < 8 || m_new_weapons._weapons[4]._level < 8)//武器等级判断
            {
                notify_type = 2;
            }
            else if (m_generals.GetGenralLevel(8) < 18)//史进等级
            {
                notify_type = 3;
            }
            else
            {
                notify_type = 4;
            }
        }
        else if(strongholdid == 59)//武松
        {
            notify_id = 6;
            int tmp_id = 0;
            if (m_bag.maxEquipLevel(1, tmp_id) < 0)//武器装备情况
            {
                notify_type = 1;
            }
            else if (getSkillLevel(4) < 15 || getSkillLevel(9) < 15)//技能等级
            {
                notify_type = 2;
            }
            else if ((m_new_weapons._weapons[0]._baseWeapon == NULL || m_new_weapons._weapons[0]._baseWeapon->_quality < 2)
                    || (m_new_weapons._weapons[1]._baseWeapon == NULL || m_new_weapons._weapons[1]._baseWeapon->_quality < 2)
                    || (m_new_weapons._weapons[3]._baseWeapon == NULL || m_new_weapons._weapons[3]._baseWeapon->_quality < 2)
                    || (m_new_weapons._weapons[4]._baseWeapon == NULL || m_new_weapons._weapons[4]._baseWeapon->_quality < 2))//武器等级判断
            {
                notify_type = 3;
            }
            else
            {
                notify_type = 4;
            }
        }
        else if(strongholdid == 75)//秦明
        {
            notify_id = 7;
            int tmpid = 0;
            if (getSkillLevel(1) < 15 || getSkillLevel(5) < 15)//技能等级
            {
                notify_type = 1;
            }
            else if (getSkillLevel(2) < 9 || getSkillLevel(3) < 9)//技能等级
            {
                notify_type = 2;
            }
            else if (m_new_weapons._weapons[0]._level < 9 || m_new_weapons._weapons[1]._level < 9
                    || m_new_weapons._weapons[3]._level < 9 || m_new_weapons._weapons[4]._level < 9)//武器等级判断
            {
                notify_type = 3;
            }
            else if ((m_bag.maxEquipLevel(1, tmpid) < 9))//武器装备情况
            {
                notify_type = 4;
            }
            else if (m_generals.GetGenralLevel(10) < 30)//武松等级
            {
                notify_type = 5;
            }
            else if (m_level < 33)//主将等级
            {
                notify_type = 6;
            }
            else
            {
                notify_type = 7;
            }
        }
        else if(strongholdid == 100)//黄文炳
        {
            notify_id = 8;
            int tmpid = 0;
            if (getSkillLevel(1) < 20 || getSkillLevel(5) < 20 || getSkillLevel(9) < 20)//技能等级
            {
                notify_type = 1;
            }
            else if (getSkillLevel(2) < 15 || getSkillLevel(3) < 15)//技能等级
            {
                notify_type = 2;
            }
            else if (m_new_weapons._weapons[0]._level < 20 || m_new_weapons._weapons[1]._level < 20
                    || m_new_weapons._weapons[3]._level < 20 || m_new_weapons._weapons[4]._level < 20)//武器等级判断
            {
                notify_type = 3;
            }
            else if ((m_bag.maxEquipLevel(1, tmpid) < 12)
                    || (m_bag.maxEquipLevel(2, tmpid) < 12)
                    || (m_bag.maxEquipLevel(3, tmpid) < 12)
                    || (m_bag.maxEquipLevel(4, tmpid) < 12))//武器装备情况
            {
                notify_type = 4;
            }
            else if (m_generals.GetGenralLevel(10) < 39)//武松等级
            {
                notify_type = 5;
            }
            else
            {
                notify_type = 6;
            }
        }
        else if(strongholdid == 150)//史文恭
        {
            notify_id = 9;
        }
        if (notify_id != 0)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "notifyFail") );
            json_spirit::Object info;
            info.push_back( Pair("id", notify_id) );
            info.push_back( Pair("type", notify_type) );
            obj.push_back( Pair("info", info) );
            obj.push_back( Pair("s", 200) );
            account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
        }
        return 0;
    }
    return -1;
}

//检查是否触发引导
int CharData::_checkGuide(int id)
{
    INFO("********************* CharData::_checkGuide:"<<id<<endl);
    if (getGuideState(id) == 0)
    {
        //cout<<"########################## CharData::_checkGuide:"<<id<<endl;
        //setGuideStateComplete(id);

        //保存当前引导id
        m_current_guide = id;
        setExtraData(char_data_type_normal, char_data_current_guide, m_current_guide);

        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
        if (account.get())
        {
            std::string msg = strGuidemsg;
            str_replace(msg, "$D", LEX_CAST_STR(id), true);
            int state = getGuideState1(id);
            str_replace(msg, "$S", LEX_CAST_STR(state), true);
            account->Send(msg);
        }
        return m_current_guide;
    }
    return -1;
}

//检查是否触发引导
int CharData::checkGuide(int type, int param1, int param2)
{
    //cout<<"********************* CharData::checkGuide:"<<type<<","<<param1<<endl;

    int guide_id = 0, guide_id2 = 0;
    switch (type)
    {
        case guide_type_login:        //首次登陆触发
            guide_id = 1;
            break;

        //case guide_type_choose_camp://选择阵营
        //    if (m_camp == 0 && m_level >= iChooseCampLevel)
        //    {
        //        guide_id = 22;        //阵营和官职引导
        //    }
        //    break;

        //case guide_type_gettask:    //领取任务触发(完成前一个任务)
        //    {
        //        switch (param1)
        //        {
        //            case task_id_farm:        //领取屯田任务,触发屯田引导
        //                guide_id = 7;
        //                break;
        //        }
        //        break;
        //    }
        case guide_type_get_stage_reward:
            if (param1 == 1)
            {
                switch (param2)
                {
                    case 1:
                        guide_id = guide_id_upgrade_weapon;
                        break;
                    case 2:
                        guide_id = guide_id_next_stage;
                        break;
                    case 3:
                        guide_id = guide_id_next_map;
                        break;
                }
            }
            break;

#if 0
        case guide_type_no_supply:    //军粮不足引导
            guide_id = guide_id_sweep;
            break;
        case guide_type_char_level:    //主将等级
            {
                switch (param1)
                {
                    case 7:        //关卡进度7级，触发攻打鲁智深引导
                        guide_id = 5;
                        break;
                    case 8:        //关卡8级，触发进入风雪山神庙1引导
                        guide_id = 8;
                        break;
                    case 10:    //关卡10级，触发购买兵器引导
                        guide_id = 10;
                        break;
                    case 12:    //关卡12级，购买策防引导
                        guide_id = 11;
                        break;
                    case 15:    //关卡15级，触发返回区域引导
                        guide_id = 25;
                        break;
                    case 18:    //关卡18级，触发阵型掉落引导
                        guide_id = 13;
                        break;
                    case 20:    //关卡20级，地区攻略引导
                        guide_id = 14;
                        guide_id2 = 30;
                        break;
                    case 25:    //关卡25级，触发武松招募引导
                        guide_id = 33;
                        break;
                    case 28:    //关卡进度28級，強化裝備引導
                        guide_id = 17;
                        break;
                    case iTradeOpenLevel:
                        guide_id = 26;    // 16级通商引导
                        break;
                    #if 0//竞技场和探索引导换由接任务时候触发 by xhw12-6-19
                    case iRaceOpenLevel:
                        guide_id = 27;    // 17级竞技场引导
                        break;
                    case iExploreOpenLevel:
                        guide_id = 18;    //探索引导
                        break;
                    #endif
                    case 30:
                        guide_id = 19;    //战马引导
                        break;
                    case iFirstStateLevel:
                        guide_id = 20;    //状态引导
                        break;
                    case 50:    //主將50，洗髓引導
                        guide_id = 32;
                        break;
                }
            }
            break;
        case guide_type_taskdone:    //任务完成时(完成条件满足)
            {
                switch (param1)
                {
                    case task_id_dongcao:        //任务2完成-引导4
                        guide_id = 4;
                        break;
                }
            }
            break;
        case guide_type_enter_map:    //进入地图
            if (param1 == 2)
            {
                //进入第2图群魔乱舞，且兵器等级达到20级
                for (int i = 0; i < 5; ++i)
                {
                    if (m_new_weapons._weapons[i]._level == 20)
                    {
                        guide_id = 15;    //重铸兵器引导
                        break;
                    }
                }
            }
            break;
        //case guide_type_frist_farm:    //第一次屯田
        //    guide_id = 7;
        //    break;
        case guide_type_no_ling:    //购买军令引导
            guide_id = 21;
            break;
#endif
    }
    int ret = -1;
    if (guide_id > 0)
    {
        ret = _checkGuide(guide_id);
    }
    if (guide_id2 > 0)
    {
        ret = _checkGuide(guide_id2);
    }
    return ret;
}

//增加角色装备
int CharData::addEquipt(int id, std::string& msg, bool notify)
{
    boost::shared_ptr<baseEquipment> baseEq = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
    if (!baseEq.get())
    {
        ERR();
        return -1;
    }
    //if (!m_backpack.equipments.haveEquip(id))
    {
        int value = baseEq->baseValue;// * (100 + my_random(-10,10))/100;
        int value2 = baseEq->baseValue2;// * (100 + my_random(-10,10))/100;
        int equipt_id = GeneralDataMgr::getInstance()->newEquiptId();
        InsertSaveDb("insert into char_equipment (id,cid,base_id,qLevel,orgAttr,orgAttr2,addAttr,addAttr2,state,deleteTime) values ("
                + LEX_CAST_STR(equipt_id) + "," + LEX_CAST_STR(m_id)
                + "," + LEX_CAST_STR(id) + ",0,"
                + LEX_CAST_STR(value) + ","
                + LEX_CAST_STR(value2)
                + ",0,0,0,0)");
        EquipmentData* pdata = new EquipmentData(id, equipt_id);

        //m_backpack.b_changed = true;
        pdata->id = equipt_id;
        pdata->baseid = id;
        pdata->cid = m_id;
        //pdata->m_deleteTime = 0;
        pdata->qLevel = 0;
        pdata->type = baseEq->type;
        //pdata->slot = 0;
        pdata->value = value;
        pdata->value2 = value2;
        pdata->addValue = 0;
        pdata->addValue2 = 0;
        pdata->quality = baseEq->quality;
        pdata->up_quality = baseEq->up_quality;
        pdata->baseEq = baseEq;
        pdata->price = pdata->quality > 0 ? 50*pdata->quality*(pdata->qLevel+1)*(pdata->qLevel+20) : 50*(pdata->qLevel+1)*(pdata->qLevel+20);
        boost::shared_ptr<iItem> eq(pdata);
        //超过背包上限，放到回购中
        if (m_bag.addItem(eq) == 0)
        {
            eq->setDeleteTime();
            m_selled_bag.add(eq);
            eq->Save();
            msg += getErrMsg(HC_ERROR_BACKPACK_FULL_GET_EQUIPT);
        }
        pdata->Save();
        //更新任务
        m_trunk_tasks.updateTask(task_equipment_level, id, 0);
        m_trunk_tasks.updateTask(task_equipment_make, id);
        if (msg == "")
        {
            json_spirit::Object obj;

            if (notify)
            {
                obj.push_back( Pair("cmd", "notify") );
                obj.push_back( Pair("s", 200) );
                obj.push_back( Pair("type", notify_msg_new_equipment) );
                sendObj(obj);
            }

            obj.clear();
            json_spirit::Object item;
            item.push_back( Pair("type", item_type_equipment) );
            item.push_back( Pair("spic", pdata->getSpic()) );
            obj.push_back( Pair("item", item) );
            obj.push_back( Pair("cmd", "notify") );
            obj.push_back( Pair("s", 200) );
            obj.push_back( Pair("type", notify_msg_new_get) );
            sendObj(obj);
        }
        //INFO("add equip price "<<pdata->price<<",quality:"<<pdata->quality<<",level "<<pdata->qLevel);
        return pdata->id;
    }
    //else
    //{
    //    return HC_ERROR;
    //}
}

//增加角色道具
int CharData::addTreasure(int id, int counts)
{
#if 0
    if (counts == 0 || id == 0)
    {
        return 0;
    }
    Treasure* tr = m_backpack.getTreasure(id);
    if (counts < 0 && tr == NULL)
    {
        return -1;
    }
    else if (counts > 0 && tr == NULL)
    {
        Treasure tre;
        tre.id = id;
        tre.nums = 0;
        boost::shared_ptr<baseTreasure> btr = GeneralDataMgr::getInstance()->GetBaseTreasure(id);
        if (btr.get())
        {
            tre.quality = btr->quality;
            tre.usage = btr->usage;
            tre.sellPrice = btr->sellPrice;
        }
        m_backpack.treasures[id] = tre;
        tr = &m_backpack.treasures[id];
    }
    if ((tr->nums + counts) >= 0)
    {
        tr->nums += counts;
        if (!tr->nums)
        {
            InsertSaveDb("delete from char_treasures where cid=" + LEX_CAST_STR(m_id) + " and tid=" + LEX_CAST_STR(id));
        }
        else
        {
            InsertSaveDb("replace into char_treasures (cid,tid,nums) value ("
                    + LEX_CAST_STR(m_id) + ","
                    + LEX_CAST_STR(id) + ","
                    + LEX_CAST_STR(tr->nums) +")");
        }

        updateTask(task_get_treasure, id, tr->nums);
        return tr->nums;
    }
    else
    {
        return -1;
    }
#else
    int err_code = 0;
    int32_t ret = m_bag.addGem(id, counts, err_code);
    if (counts > 0 && ret > 0)
    {
        updateTask(task_get_treasure, id, ret);
    }
    return ret;
#endif
}

//增加角色道具需要错误码返回信息
int CharData::addTreasure(int id, int counts, int& err_code)
{
    int32_t ret = m_bag.addGem(id, counts, err_code);
    if (counts > 0 && ret > 0)
    {
        updateTask(task_get_treasure, id, ret);
    }
    return ret;
}

//角色道具數量
int CharData::treasureCount(int id)
{
#if 0
    Treasure* tr = m_backpack.getTreasure(id);
    if (tr != NULL)
    {
        return tr->nums;
    }
    else
    {
        return 0;
    }
#else
    return m_bag.getGemCount(id);
#endif
}

//显示背包物品
int CharData::showBackpack(int page, int pagenums, json_spirit::Object& robj)
{
#if 0
    if (page < 1)
    {
        page = 1;
    }
    int maxpage = 1;
    json_spirit::Array elists;
    json_spirit::Array ilists;
    m_backpack.listBackpack(page, pagenums, maxpage, elists, ilists);
    robj.push_back( Pair("equipList", elists) );
    robj.push_back( Pair("gemList", ilists) );
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", pagenums) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
#else
    return m_bag.showBag(page, pagenums, robj);
#endif
}

//显示身上装备
int CharData::getEquiped(int gid, json_spirit::Object& robj)
{
    //武将是否存在
    boost::shared_ptr<CharGeneralData> gd = m_generals.GetGenral(gid);
    if (!gd.get())
    {
        return HC_ERROR;
    }
    json_spirit::Array roleEquips;
    gd->getList(roleEquips);
    robj.push_back( Pair("roleEquips", roleEquips) );
    robj.push_back( Pair("equip_power", gd->m_equip_power) );
    robj.push_back( Pair("gid", gd->m_id) );
    return HC_SUCCESS;
}

//显示身上装备详细信息
int CharData::getEquipmentInfo(int id, json_spirit::Object& robj)
{
    EquipmentData* equip = m_bag.getEquipById(id);
    if (!equip)
    {
        equip = m_selled_bag.getEquipById(id);
        if (!equip)
        {
            equip = m_generals.getEquipById(id);
            if (!equip)
            {
                return HC_ERROR;
            }
        }
    }
    json_spirit::Object eq;
    equip->toObj(eq);
    robj.push_back( Pair("equipVO", eq) );
    return HC_SUCCESS;
}

//卖装备
int CharData::sellEquipment(int id, json_spirit::Object& robj)
{
    int value = 0;
    EquipmentData* equip = m_bag.getEquipById(id);
    if (!equip)
    {
        return HC_ERROR;
    }
    int32_t silver = equip->sellPrice();
    uint8_t slot = equip->getSlot();
    //银币获得统计
    add_statistics_of_silver_get(m_id, m_ip_address, silver, silver_get_sell_equiptment, m_union_id, m_server_id);
    equip->setDeleteTime();

    boost::shared_ptr<iItem> itm = m_bag.removeItem(slot);
    m_selled_bag.add(itm);
    equip->Save();
    addSilver(silver);
    NotifyCharData();
    robj.push_back( Pair("price", silver) );

    updateEnhanceCost();
    updateEnhanceCDList();

    return HC_SUCCESS;
}

//卖装备
int CharData::sellTreasure(int id, int count, json_spirit::Object& robj)
{
    return HC_SUCCESS;
}

//回购装备
int CharData::buybackEquipment(int id, json_spirit::Object& robj)
{
    if (m_bag.isFull())
    {
        return HC_ERROR_BACKPACK_FULL_BUYBACK;
    }

    boost::shared_ptr<iItem> itm = m_selled_bag.removeByEquipId(id);
    if (itm.get())
    {
        EquipmentData* p = dynamic_cast<EquipmentData*>(itm.get());
        if (addSilver(-p->sellPrice()*2, true) < 0)
        {
            m_selled_bag.add(itm);
            return HC_ERROR_NOT_ENOUGH_SILVER;
        }
        NotifyCharData();
        m_bag.addItem(itm);
        p->buyBack();
        p->Save();
        robj.push_back( Pair("price", p->sellPrice()) );

        updateEnhanceCost();
        updateEnhanceCDList();
    }
    return HC_SUCCESS;
}

//显示回购
int CharData::listSelledEquipment(int page, int page_per, json_spirit::Object& o)
{
    //cout<<"show selled bag"<<endl;
    m_selled_bag.getList(page, page_per, o);
    return HC_SUCCESS;
}

//显示宝物详细信息
int CharData::getTreasureInfo(int id, int nums, json_spirit::Object& robj)
{
    //cout<<"getTreasureInfo "<<id<<endl;
    boost::shared_ptr<baseTreasure> btr = GeneralDataMgr::getInstance()->GetBaseTreasure(id);
    if (!btr.get())
    {
        ERR();
        return HC_ERROR;
    }

    json_spirit::Object trobj;
    trobj.push_back( Pair("id", btr->id) );
    trobj.push_back( Pair("name", btr->name) );
    trobj.push_back( Pair("spic", btr->spic) );
    trobj.push_back( Pair("quality", btr->quality) );
    int price = btr->sellPrice;
    if (nums > 0)
        price *= nums;
    trobj.push_back( Pair("price", price) );
    trobj.push_back( Pair("memo", btr->memo) );
    robj.push_back( Pair("gemVO", trobj) );

    //cout<<"usage "<<btr->usage<<endl;
    if (btr->usage == 8)
    {
        getScrollTips(btr.get(), robj);
    }
    else if (btr->usage == 9)
    {
        robj.push_back( Pair("canMake", btr->canMake) );
    }
    return HC_SUCCESS;
}

//装备东西
int CharData::equip(int gid, int slot, int eid)
{
    //cout<<"CharData::equip(),gid:"<<gid<<",slot:"<<slot<<",eid:"<<eid<<endl;
    //武将是否存在
    boost::shared_ptr<CharGeneralData> gd = m_generals.GetGenral(gid);
    if (!gd.get())
    {
        return HC_ERROR;
    }
    int ret = gd->equip(slot, eid);
    //m_backpack.equipments.update_count();
    return ret;
}

//一键装备
int CharData::onekeyEquip(int gid)
{
    //武将是否存在
    boost::shared_ptr<CharGeneralData> gd = m_generals.GetGenral(gid);
    if (!gd.get())
    {
        return HC_ERROR;
    }

    CharGeneralData* g = gd.get();
    for (uint8_t slot = equip_ring; slot <= equip_slot_max; ++slot)
    {
        boost::shared_ptr<iItem> itm = g->m_equipments.getItem(slot);
        int quality = -1, qLevel = -1;
        if (itm.get())
        {
            EquipmentData* ed = dynamic_cast<EquipmentData*>(itm.get());
            quality = ed->up_quality;
            qLevel = ed->qLevel;
        }
        int eid = m_bag.getBestEquipment(slot, g->m_level, quality, qLevel);
        if (eid > 0)
        {
            g->equip(slot, eid);
        }
    }
    return HC_SUCCESS;
}

//卸下东西
int CharData::unequip(int gid, int slot)
{
    //cout<<"CharData::unequip "<<gid<<","<<slot<<endl;
    //武将是否存在
    boost::shared_ptr<CharGeneralData> gd = m_generals.GetGenral(gid);
    if (!gd.get())
    {
        return HC_ERROR;
    }
    //背包满了不能放
    if (m_bag.isFull())
    {
        return HC_ERROR_BACKPACK_FULL_NO_UNEQIPT;
    }
    int ret = gd->unequip(slot);
    //m_backpack.equipments.update_count();
    return ret;
}

//购买背包
int CharData::buyBagSlot(int num, json_spirit::Object& robj)
{
    if (num < 0)
    {
        num = 1;
    }
    if (num + m_bag.size() > MAX_BAG_SIZE)
    {
        num = MAX_BAG_SIZE - m_bag.size();
        if (num < 1)
        {
            return HC_ERROR;
        }
    }

    //优先扣道具
    int tcount = m_bag.getGemCount(treasure_type_bag_key);

    int buyed = m_bag.size() - BAG_DEFAULT_SIZE;
    int gold_need = 0, dj_use = 0;

    for (int i = buyed + 1; i <= (buyed+num); ++i)
    {
        if (dj_use < tcount)
        {
            ++dj_use;
        }
        else
        {
            int tmp = ((i-1) / 3 + 1) * 2;
            if (tmp >= 40)
                tmp = 40;
            gold_need += tmp;
        }
    }
    //int gold_need = (2*buyed+num+1)*num;
    if (addGold(-gold_need) >= 0)
    {
        if (dj_use > 0)
        {
            addTreasure(treasure_type_bag_key, -dj_use);
            //通知道具使用
            add_statistics_of_treasure_cost(m_id,m_ip_address,treasure_type_bag_key,dj_use,treasure_unknow,2,m_union_id,m_server_id);
            std::string msg = treasure_expend_msg(treasure_type_bag_key, dj_use);
            if (msg != "")
            {
                robj.push_back( Pair("msg", msg) );
            }
        }
        NotifyCharData();
        m_bag.addSize(num);
        //保存背包购买
        InsertSaveDb("update char_data set backPack=" + LEX_CAST_STR(buyed+num) + " where cid=" + LEX_CAST_STR(m_id));
        //金币消耗统计
        add_statistics_of_gold_cost(m_id, m_ip_address, gold_need, gold_cost_for_buy_bag, m_union_id, m_server_id);
        m_trunk_tasks.updateTask(task_buy_bag, buyed+num);
        act_to_tencent(this,act_new_bag_buy,num,m_bag.size());
#ifdef QQ_PLAT
        gold_cost_tencent(this,gold_need,gold_cost_for_buy_bag);
#endif
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
}

int CharData::getPugong(bool load_horse)
{
    if (load_horse && m_horse.horse != NULL)
    {
        return m_total_pugong + m_horse.horse->pugong + m_horse.pugong;
    }
    return m_total_pugong;
}

int CharData::getCegong(bool load_horse)
{
    if (load_horse && m_horse.horse != NULL)
    {
        return m_total_cegong + m_horse.horse->cegong + m_horse.cegong;
    }
    return m_total_cegong;
}

int CharData::getPufang(bool load_horse)
{
    if (load_horse && m_horse.horse != NULL)
    {
        return m_total_pufang + m_horse.horse->pufang + m_horse.pufang;
    }
    return m_total_pufang;
}

int CharData::getCefang(bool load_horse)
{
    if (load_horse && m_horse.horse != NULL)
    {
        return m_total_cefang + m_horse.horse->cefang + m_horse.cefang;
    }
    return m_total_cefang;
}

int CharData::getBingli(bool load_horse)
{
    if (load_horse && m_horse.horse != NULL)
    {
        return m_total_bingli + m_horse.horse->bingli + m_horse.bingli;
    }
    return m_total_bingli;
}

//更新状态，技能的影响
void CharData::updateCombatAttribute()
{
    ::updateCombatAttribute(m_newStates, m_skill_list, m_combat_attribute);

    //更新技能数值加成
    int attack = 0;
    int defense = 0;
    memset(m_skill_power, 0, 5*sizeof(int));
    for (int i = 0; i < skill_add_max; ++i)
    {
        if (i <= skill_add_cefang)
        {
            m_skill_power[i] += m_combat_attribute.skill_add(i);
        }
        else
        {
            if (i%2 == 1)
            {
                attack += m_combat_attribute.skill_add(i);
            }
            else
            {
                defense += m_combat_attribute.skill_add(i);
            }
        }
    }
    attack /= 5;
    defense /= 5;
    m_skill_power[skill_add_pugong] += attack;
    m_skill_power[skill_add_cegong] += attack;
    m_skill_power[skill_add_pufang] += defense;
    m_skill_power[skill_add_cefang] += defense;
}

#if 0
//刷新状态 type 0 系统刷新 1 银币刷新 2 金币刷新
int CharData::refreshStates(int type, int pos)
{
    INFO("refresh states:type=" << type << ",pos=" << pos << ",cid="<<m_id<<endl);
    if (m_level >= iFirstStateLevel)
    {
        if (0 == type)
        {
            if (m_newStates.refresh())
            {
                updateCombatAttribute();
            }
        }
        else
        {
            int level = m_newStates.refresh(type, pos);
            if (type > 0 && level > 0)
            {
                updateCombatAttribute();
                //更新任务
                updateTask(task_refresh_state, level, 0);
            }
        }
    }
    return HC_SUCCESS;
}
#endif

//获取战马
int CharData::getHorse(json_spirit::Object& horse)
{
    if (m_horse.horse != NULL)
    {
        horse.push_back( Pair("name", m_horse.horse->name) );
        horse.push_back( Pair("spic", m_horse.horse->spic) );
        horse.push_back( Pair("quality", m_horse.horse->quality) );
        horse.push_back( Pair("stars", m_horse.horse->star) );
        horse.push_back( Pair("turns", m_horse.horse->turn) );

        horse.push_back( Pair("atk_normal", m_horse.horse->pugong) );
        horse.push_back( Pair("def_normal", m_horse.horse->pufang) );
        horse.push_back( Pair("atk_tactics", m_horse.horse->cegong) );
        horse.push_back( Pair("def_tactics", m_horse.horse->cefang) );
        horse.push_back( Pair("army", m_horse.horse->bingli) );
    }
    return HC_SUCCESS;
}

//更新攻防加成
int CharData::updateAttackDefense()
{
    m_weapon_attack_change = false;
    m_total_pugong = 0;
    m_total_pufang = 0;
    m_total_cegong = 0;
    m_total_cefang = 0;
    m_total_bingli = 0;
    //兵器
    m_total_pugong += m_new_weapons._weapons[0]._effect;
    m_total_pufang += m_new_weapons._weapons[1]._effect;
    m_total_cegong += m_new_weapons._weapons[2]._effect;
    m_total_cefang += m_new_weapons._weapons[3]._effect;
    m_total_bingli += m_new_weapons._weapons[4]._effect;

    #if 0
    //装备
    if (m_backpack.equipments.m_equiped[0].get())
    {
        m_total_pugong += m_backpack.equipments.m_equiped[0]->value;
    }
    if (m_backpack.equipments.m_equiped[1].get())
    {
        m_total_pufang += m_backpack.equipments.m_equiped[1]->value;
    }
    if (m_backpack.equipments.m_equiped[2].get())
    {
        m_total_cegong += m_backpack.equipments.m_equiped[2]->value;
    }
    if (m_backpack.equipments.m_equiped[3].get())
    {
        m_total_cefang += m_backpack.equipments.m_equiped[3]->value;
    }
    if (m_backpack.equipments.m_equiped[4].get())
    {
        m_total_bingli += m_backpack.equipments.m_equiped[4]->value;
    }
    #endif
    //攻擊力變化
    set_attack_change();
    return 0;
}

//战斗消耗/军令
int CharData::combatCost(bool win, int type)
{
    bool bCostLing = false;
    switch (type)
    {
        case combat_stronghold:
            if (win)// || m_level > 30)
            {
                bCostLing = true;
            }
            break;
    }
    if (bCostLing)
    {
        addLing(-4);
        //军令统计
        add_statistics_of_ling_cost(m_id,m_ip_address,4,ling_stronghold,2, m_union_id, m_server_id);
        if (m_hp_cost > 0 && type == combat_stronghold)
        {
            addLing(-1);
            m_hp_cost = 0;
        }
    }
    return 0;
}

//获得装备强化信息
int CharData::getEquipmentUpInfo(int id, json_spirit::Object& robj)
{
    EquipmentData* equip = NULL;
    if (id == 0)
    {
        equip = m_bag.getDefaultEquip(0, m_level);
    }
    else
    {
        equip = m_bag.getEquipById(id);
    }
    if (NULL == equip)
    {
        equip = m_generals.getEquipById(id);
    }
    if (!equip)
    {
        return HC_ERROR;
    }

    //if (equip->qLevel >= m_level)
    //{
    //    return HC_ERROR_UPGRADE_MAX_LEVEL;
    //}
    json_spirit::Object eq;
    equip->toObj(eq);

    robj.push_back( Pair("equipVO", eq) );

    int org = 0;
    int needs = equipmentUpgrade::getInstance()->getUpgradeNeeds(equip->quality, equip->type, 1 + equip->qLevel, org);
    robj.push_back( Pair("org", org) );
    robj.push_back( Pair("silver", needs) );
    int cd = m_enhance_cd - time(NULL);
    if (cd > 0)
    {
        robj.push_back( Pair("leftTime", cd) );
        robj.push_back( Pair("can_enhance", m_can_enhance) );
    }
    else
    {
        m_can_enhance = true;
    }
    return HC_SUCCESS;
}

//更新强化需要的最少银币
void CharData::updateEnhanceCost()
{
    m_enhance_eid = 0;
    m_enhance_silver = m_bag.getMinEnhanceCost(m_enhance_eid);
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.begin();
    while (it != m_generals.m_generals.end())
    {
        if (it->second.get())
        {
            int eid = 0;
            int cost = it->second->m_equipments.getMinEnhanceCost(eid);
            if (cost > 0 && (m_enhance_silver == 0 || cost < m_enhance_silver))
            {
                m_enhance_silver = cost;
                m_enhance_eid = eid;
            }
        }
        ++it;
    }
}

//更新强化队列状态
void CharData::updateEnhanceCDList()
{
    int state = m_enhance_state;
    m_enhance_state = (m_enhance_silver > 0 && m_silver >= m_enhance_silver) ? 1 : 0;
    if (m_enhance_state != state)
    {
        m_open_info_change = true;
    }
}

//更新强化秘法需要的最少功勋
void CharData::updateUpgradeWeaponCost()
{
    m_upgrade_weapon_gongxun = 0;
    m_upgrade_weapon_type = 0;
    for (int i = 0; i < 5; ++i)
    {
        if (m_new_weapons._weapons[i]._baseWeapon)
        {
            if (m_new_weapons._weapons[i]._level < m_level && (m_upgrade_weapon_gongxun == 0 || m_new_weapons._weapons[i]._cost < m_upgrade_weapon_gongxun))
            {
                m_upgrade_weapon_gongxun = m_new_weapons._weapons[i]._cost;
                m_upgrade_weapon_type = i + 1;
            }
        }
    }
}

//更新秘法队列状态
void CharData::updateUpgradeWeaponCDList()
{
    int state = m_upgrade_weapon_state;
    m_upgrade_weapon_state = (m_upgrade_weapon_gongxun > 0 && m_bag.getGemCurrency(treasure_type_gongxun) >= m_upgrade_weapon_gongxun) ? 1 : 0;
    if (state != m_upgrade_weapon_state)
    {
        m_open_info_change = true;
    }
}

//强化冷却加速
int CharData::enhanceSpeed()
{
    if (m_vip >= iEnhanceNoCDVip)
    {
        return HC_ERROR;
    }
    int cd = m_enhance_cd - time(NULL);
    if (cd > 0)
    {
        int needgold = 0;
        if (cd % 60 == 0)
        {
            needgold = cd / 60;
        }
        else
        {
            needgold = cd / 60 + 1;
        }
        if (isNewPlayer() > 0)
            needgold = 0;
        if (needgold > 0)
        {
            if (addGold(-needgold) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            //金币消耗统计
            add_statistics_of_gold_cost(m_id, m_ip_address, needgold, gold_cost_for_speed_enhance, m_union_id, m_server_id);
#ifdef QQ_PLAT
            gold_cost_tencent(this,needgold,gold_cost_for_speed_enhance);
#endif
        }
    }

    m_enhance_cd = 0;
    m_can_enhance = true;
    return HC_SUCCESS;
}

//强化
int CharData::enhanceEquipment(int id, json_spirit::Object& robj)
{
    //cout<<"CharData::enhanceEquipment()"<<endl;
    //先从背包中找
    EquipmentData* equip = m_bag.getEquipById(id);
    if (!equip)
    {
        //再从武将身上找
        equip = m_generals.getEquipById(id);
    }
    if (!equip)
    {
        return HC_ERROR;
    }

    if (equip->qLevel >= m_level)
    {
        return HC_ERROR_UPGRADE_MAX_LEVEL;
    }

    int org = 0;
    int needs = equipmentUpgrade::getInstance()->getUpgradeNeeds(equip->quality, equip->type, 1 + equip->qLevel, org);
    if (needs == 0)
    {
        return HC_ERROR_UPGRADE_MAX_LEVEL;
    }
    if (addSilver(-needs) >= 0)
    {
        //银币消耗统计
        add_statistics_of_silver_cost(m_id, m_ip_address, needs, silver_cost_for_enhance, m_union_id, m_server_id);
        NotifyCharData();
        ++equip->qLevel;
        int value2 = 0;
        equip->addValue += equipmentUpgrade::getInstance()->getUpgradeValue(equip->up_quality, equip->type, equip->qLevel, value2);

        if (value2 > 0)
        {
            equip->addValue2 += value2;
        }
        int q = equip->quality;
        if (q == 0)
        {
            q = 1;
        }
        equip->price = 50*q*(equip->qLevel+1)*(equip->qLevel+20);
        equip->setChanged();
        json_spirit::Object eq;
        equip->toObj(eq);
        robj.push_back( Pair("equipVO", eq) );

        //m_backpack.b_changed = true;
        //保存到数据库
        equip->Save();

        //更新任务
        m_trunk_tasks.updateTask(task_equipment_level, equip->baseid, equip->qLevel);

        m_trunk_tasks.updateTask(task_attack_equipment_level, equip->baseid, equip->qLevel);


        //強化身上的裝備才會改變戰力
        if (equip->getGeneral().get())
        {
            equip->getGeneral()->updateEquipmentEffect();
            m_weapon_attack_change = true;
            //新战力
            //equip->getGeneral()->equip_change = true;
            equip->getGeneral()->updateEquipmentEffect();
        }
        //cout<<"CharData::enhanceEquipment() return 200"<<endl;

        updateEnhanceCost();
        updateEnhanceCDList();
        //日常任务
        dailyTaskMgr::getInstance()->updateDailyTask(*this,daily_task_equipt);
        //黄钻特权
        if (m_qq_yellow_level == 0 && m_vip < iEnhanceNoCDVip)
        {
            time_t t_now = time(NULL);
            if (m_enhance_cd < t_now)
            {
                m_enhance_cd = t_now + 120;
            }
            else
            {
                m_enhance_cd += 120;
            }
            if ((m_enhance_cd - t_now) > TIME_MAX_ENHANCE)
            {
                m_can_enhance = false;
                //新手在真冷却需要弹窗通知
                if (isNewPlayer() > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "notify") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair("type", notify_msg_new_player_enhance) );
                    obj.push_back( Pair("nums", isNewPlayer()) );
                    sendObj(obj);
                }
            }
        }
        //装备强化，好友祝贺
        if (equip->qLevel > 10 && equip->qLevel % 10 == 0)
        {
            Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_ENHANCE_EQUIPMENT, equip->baseid, equip->qLevel);
        }
        //act统计
        act_to_tencent(this,act_new_equipt_up,equip->type);
#ifdef QQ_PLAT

        if (equip->qLevel == 20)
        {
            //分享
            Singleton<inviteMgr>::Instance().update_event(m_id, SHARE_EVENT_FIRST_ENHANCE_EQUIPTMENT_20_LEVEL, 20);
        }
#endif
        return HC_SUCCESS;
    }
    else
    {
        //cout<<"CharData::enhanceEquipment() return need more silver"<<endl;
        return HC_ERROR_NOT_ENOUGH_SILVER;
    }
}

//设置强化等级
int CharData::setEquipmentLevel(int id, int level)
{
    //先从背包中找
    EquipmentData* equip = m_bag.getEquipById(id);
    if (!equip)
    {
        //再从武将身上找
        equip = m_generals.getEquipById(id);
    }
    if (!equip)
    {
        return HC_ERROR;
    }

    if (level < 0)
    {
        level = 0;
    }
    if (equip->qLevel == level)
    {
        return HC_SUCCESS;
    }
    else if (equip->qLevel < level)
    {
        int org = 0;
        //装备升级
        while (equip->qLevel < level && equipmentUpgrade::getInstance()->getUpgradeNeeds(equip->quality, equip->type, 1 + equip->qLevel, org))
        {
            ++equip->qLevel;
            int value2 = 0;
            equip->addValue += equipmentUpgrade::getInstance()->getUpgradeValue(equip->up_quality, equip->type, equip->qLevel, value2);
            if (value2 > 0)
            {
                equip->addValue2 += value2;
            }
        }
    }
    else
    {
        //装备降级
        while (equip->qLevel > level)
        {
            int value2 = 0;
            equip->addValue -= equipmentUpgrade::getInstance()->getUpgradeValue(equip->up_quality, equip->type, equip->qLevel, value2);
            --equip->qLevel;
            if (value2 > 0)
            {
                equip->addValue2 -= value2;
            }
        }
    }
    equip->setChanged();
    //m_backpack.b_changed = true;
    //保存到数据库
    equip->Save();
    //更新任务
    m_trunk_tasks.updateTask(task_equipment_level, equip->baseid, equip->qLevel);
    m_trunk_tasks.updateTask(task_attack_equipment_level, equip->baseid, equip->qLevel);
    return HC_SUCCESS;
}

//获得强化装备列表
int CharData::getEnhanceEquiptlist(int page, int nums_per_page, json_spirit::Object& robj)
{
    //cout<<"CharData::getEnhanceEquiptlist()"<<endl;
    bool need_all = false;
    json_spirit::Array elists;
    if (page <= 0)
    {
        page = 1;
    }
    //每页数量为0处理为全部返回
    if (nums_per_page == 0)
    {
        need_all =  true;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;

    for (uint8_t slot = 1; slot <= m_bag.size(); ++slot)
    {
        boost::shared_ptr<iItem> itm = m_bag.getItem(slot);
        if (itm.get() && itm->getType() == iItem_type_equipment)
        //if (it->second->state == 0 && it->second->qLevel < 20*it->second->quality)
            //&& equipmentUpgrade::getInstance()->getUpgradeNeeds(it->second->quality, 1 + it->second->qLevel) < m_backpack.treasures[it->second->quality])
        {
            EquipmentData* p = dynamic_cast<EquipmentData*>(itm.get());
            if (p->qLevel < 20*p->quality)
            {
                ++cur_nums;
                if (need_all || (cur_nums >= first_nums && cur_nums <= last_nums))
                {
                    boost::shared_ptr<baseEquipment> beq = GeneralDataMgr::getInstance()->GetBaseEquipment(itm->getSubtype());
                    if (!beq.get())
                    {
                        ERR();
                        continue;
                    }
                    json_spirit::Object obj;
                    obj.push_back( Pair("id", beq->baseid) );
                    obj.push_back( Pair("spic", beq->baseid) );
                    obj.push_back( Pair("name", beq->name) );
                    obj.push_back( Pair("quality", beq->quality) );
                    obj.push_back( Pair("type", beq->type) );
                    obj.push_back( Pair("level", p->qLevel) );
                    obj.push_back( Pair("isOn", 0) );
                    elists.push_back(obj);
                }
            }
        }
    }
    robj.push_back( Pair("list", elists) );
    int maxpage = 0;
    if (!need_all)
        maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    //cout<<"CharData::getEnhanceEquiptlist() ret"<<endl;
    return HC_SUCCESS;
}

/************** 技能研究相关 *************************/

//技能研究定时加点
int CharData::skillResearchAdd(int sid, int times)
{
    if (times <= 0)
    {
        times = 1;
    }
    for (size_t i = 0; i < m_skill_queue.size(); ++i)
    {
        if (m_skill_queue[i].state && m_skill_queue[i].skill.get() && m_skill_queue[i].skill->skill.get() && m_skill_queue[i].skill->skill->base_skill_id == sid)
        {
            m_skill_queue[i].left_mins -= skill_research_mins*times;
            if (!m_skill_queue[i].skill.get())
            {
                ERR();
                return -1;
            }
            m_skill_queue[i].skill->exp += m_skill_queue[i].final_speed*times;
            //INFO("************* queue "<<i<<" add exp "<<m_skill_queue[i].final_speed);
            //判断是否升级
            int need_exp = baseSkillMgr::getInstance()->getExpNeed(m_skill_queue[i].skill->skill->base_skill_id, m_skill_queue[i].skill->level + 1);
            while (-1 != need_exp && m_skill_queue[i].skill->exp >= need_exp)
            {
                ++m_skill_queue[i].skill->level;
                m_skill_queue[i].skill->exp -= need_exp;
                updateCombatAttribute();
                need_exp = baseSkillMgr::getInstance()->getExpNeed(m_skill_queue[i].skill->skill->base_skill_id, m_skill_queue[i].skill->level + 1);
                //INFO("*****************skill level up !!!!!!!!!!");
                //是否需要通知玩家升级了
                updateTask(task_skill_level, sid, m_skill_queue[i].skill->level);

                //攻擊力變化
                set_attack_change();
            }
            if (m_skill_queue[i].left_mins <= 0)
            {
                m_skill_queue[i].skill->state = 0;
                m_skill_queue[i].skill->save();

                m_skill_queue[i].left_mins = 0;
                m_skill_queue[i].state = 0;
                m_skill_queue[i].start_time = 0;
                m_skill_queue[i].teacher.reset();
                m_skill_queue[i].skill.reset();

                m_skill_queue[i].save();
                //是否通知玩家完成
            }
            else
            {
                m_skill_queue[i].start_time = time(NULL);
                m_skill_queue[i].end_time = m_skill_queue[i].start_time + m_skill_queue[i].left_mins * 60;
                m_skill_queue[i].save();
                m_skill_queue[i].skill->save();

                json_spirit::mObject mobj;
                mobj["cmd"] = "researchDone";
                mobj["cid"] = m_id;
                mobj["sid"] = sid;
                boost::shared_ptr<splsTimer> tmsg;
                tmsg.reset(new splsTimer(skill_research_mins*60, 1, mobj, 1));
                m_skill_queue[i]._uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
            }
            return 0;
        }
    }
    return -2;
}

//技能研究开始
int CharData::startSkillResearch(int sid, int teacher)
{
    //cout<<"startSkillResearch("<<sid<<","<<teacher<<")"<<endl;
    boost::shared_ptr<skillTeacher> tea;
    //是否存在这个研究者
    for (int i = 0; i < skill_teacher_nums; ++i)
    {
        //if (m_teachers[i].get())
        //{
        //    cout<<(i+1)<<"/teacher:"<<m_teachers[i]->id<<endl;
        //}
        if (m_teachers[i].get() && m_teachers[i]->id == teacher)
        {
            tea = m_teachers[i];
            //cout<<"finded..."<<i<<endl;
            break;
        }
    }
    if (!tea.get())
    {
        ERR();
        return HC_ERROR;
    }
    //先判断技能是否可以继续升级
    if (!m_skill_list[sid].get())
    {
        ERR();
        return HC_ERROR;
    }
    if (m_skill_list[sid]->state == 1)
    {
        return HC_ERROR_SKILL_RESEARCHING;
    }
    //不能再升级啦
    int exp_levelup = baseSkillMgr::getInstance()->getExpNeed(sid, m_skill_list[sid]->level + 1);
    if (exp_levelup < 0)
    {
        return HC_ERROR_SKILL_LEVEL_MAX;
    }
    //等级不能超过角色等级一半
    if (m_skill_list[sid]->level >= m_level /2)
    {
        return HC_ERROR_SKILL_LEVEL_MAX;
    }
    //再判断队列是否有空闲
    for (size_t i = 0; i < m_skill_queue.size(); ++i)
    {
        if (m_skill_queue[i].state == 0)
        {
#if 0
            //令够吗
            if (addLing(-1) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_LING;
            }
            //军令统计
            add_statistics_of_ling_cost(m_id,m_ip_address,1,ling_skill_train,2);

#else
            //银币足够吗
            int silver_need = 50 * (m_skill_list[sid]->level * m_skill_list[sid]->level - 2 * m_skill_list[sid]->level) + 1000;
            if (addSilver(-silver_need) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_SILVER;
            }
            //银币消耗统计
            add_statistics_of_silver_cost(m_id, m_ip_address, silver_need, silver_cost_for_research, m_union_id, m_server_id);
#endif
            NotifyCharData();
            m_skill_queue[i].skill = m_skill_list[sid];
            m_skill_queue[i].teacher = tea;
            m_skill_queue[i].start_time = time(NULL);
            m_skill_queue[i].exp_added = 0;
            m_skill_queue[i].left_mins = tea->total_mins;
            //INFO("******************queue start "<<i);
            m_skill_queue[i].start();
            //开始技能研究时需要刷新队列
            baseSkillMgr::getInstance()->updateTeacher(m_area, m_teachers);
            m_skill_queue[i].save();


            //更新任务
            updateTask(task_do_research, 0, 0);
            return HC_SUCCESS;
        }
    }
    //没有空闲队列
    return HC_ERROR_QUEUE_FULL;
}

//停止技能研究
int CharData::stopSkillResearch(int sid)
{
    //判断队列是否有在研究这个技能
    for (size_t i = 0; i < m_skill_queue.size(); ++i)
    {
        if (m_skill_queue[i].state == 1 && m_skill_queue[i].skill.get() && m_skill_queue[i].skill->skill.get() && m_skill_queue[i].skill->skill->base_skill_id == sid)
        {
            //找到了
            m_skill_queue[i].stop();
            m_skill_queue[i].save();
            return HC_SUCCESS;
        }
    }
    //没有在研究这个队列啊
    return HC_ERROR;
}

int CharData::stopSkillResearchAll()
{
    for (size_t i = 0; i < m_skill_queue.size(); ++i)
    {
        if (m_skill_queue[i].state == 1 && m_skill_queue[i].skill.get() && m_skill_queue[i].skill->skill.get())
        {
            //找到了
            m_skill_queue[i].stop();
            m_skill_queue[i].save();
        }
    }
    return HC_SUCCESS;
}

//获得技能详细信息
int CharData::getSkillDetail(int sid, json_spirit::Object& o)
{
    //先判断技能是否存在
    if (!m_skill_list[sid].get() || !m_skill_list[sid]->skill.get())
    {
        return HC_ERROR;
    }

    json_spirit::Object sk;
    sk.push_back( Pair("id", m_skill_list[sid]->skill->base_skill_id) );
    sk.push_back( Pair("name", m_skill_list[sid]->skill->name) );
    sk.push_back( Pair("spic", m_skill_list[sid]->skill->base_skill_id) );
    std::string memo = m_skill_list[sid]->skill->memo;
    int add_val = m_skill_list[sid]->skill->effect_per_level[m_skill_list[sid]->skill->effect_type];
    int now = m_skill_list[sid]->level * add_val;
    int next = now + add_val;
    str_replace(memo, "$N", LEX_CAST_STR(now));
    str_replace(memo, "$n", LEX_CAST_STR(next));
    sk.push_back( Pair("memo", memo) );
    sk.push_back( Pair("level", m_skill_list[sid]->level) );
    sk.push_back( Pair("state", m_skill_list[sid]->state + 1) );
    o.push_back( Pair("skillVO", sk) );
    return HC_SUCCESS;
}

//获得技能列表
int CharData::getSkillList(json_spirit::Object& o)
{
    //转换显示
    static int convert_map[6] = {0,2,1,3,0,4};
    json_spirit::Array list[5];
    std::map<int, boost::shared_ptr<charSkill> >::iterator it = m_skill_list.begin();
    while (it != m_skill_list.end())
    {
        if (it->second.get() && it->second->skill.get() && (it->second->skill->base_skill_type >=1 && it->second->skill->base_skill_type <= 5))
        {
            json_spirit::Object sk;
            sk.push_back( Pair("id", it->second->skill->base_skill_id) );
            sk.push_back( Pair("spic", it->second->skill->base_skill_id) );
            sk.push_back( Pair("level", it->second->level) );
            sk.push_back( Pair("state", it->second->state + 1) );
            sk.push_back( Pair("isFull", it->second->level < (m_level/2) ? 0 : 1) );
            list[convert_map[it->second->skill->base_skill_type]].push_back(sk);
        }
        ++it;
    }
    for (int i = 0; i < 5; ++i)
    {
        o.push_back( Pair("list" + LEX_CAST_STR(i+1), list[i]) );
    }
    return HC_SUCCESS;
}

int CharData::getSkillLevel(int sid)
{
    std::map<int, boost::shared_ptr<charSkill> >::iterator it = m_skill_list.find(sid);
    if (it != m_skill_list.end())
    {
        if (it->second.get())
        {
            return it->second->level;
        }
    }
    return 0;
}

//获得技能研究列表
int CharData::getSkillResearchList(json_spirit::Object& o)
{
    json_spirit::Array ques;

    bool diamond = false;
    //判断队列是否有在研究这个技能
    for (size_t i = 0; i < m_skill_queue.size(); ++i)
    {
        json_spirit::Object que;
        if (m_skill_queue[i].state == 1
            && m_skill_queue[i].skill.get()
            && m_skill_queue[i].skill->skill.get()
            && m_skill_queue[i].teacher.get())
        {
            //正在研究中的
            //json_spirit::Object que;
            que.push_back( Pair("position", m_skill_queue[i].pos) );    //位置
            que.push_back( Pair("state", m_skill_queue[i].state + 1) );        //状态
            que.push_back( Pair("type", m_skill_queue[i].type + 1) );        //队列级别

            que.push_back( Pair("id", m_skill_queue[i].teacher->id) );    //研究者id
            que.push_back( Pair("name", m_skill_queue[i].teacher->name) );//研究者姓名
            que.push_back( Pair("color", m_skill_queue[i].teacher->quality) );//研究者品质
            que.push_back( Pair("expSpeed", m_skill_queue[i].final_speed/skill_research_mins) );//研究速度，每分钟
            que.push_back( Pair("expTotal", m_skill_queue[i].skill->exp) );    //累计经验
            //cout<<"left time = "<<m_skill_queue[i].end_time<<"-"<<time(NULL)<<endl;
            int left_time = (m_skill_queue[i].end_time - time(NULL))/60;
            if (left_time < 0)
            {
                left_time = 1;
            }
            que.push_back( Pair("leftTime", left_time) );    //剩余时间
            //que.push_back( Pair("leftTime", m_skill_queue[i].left_mins) );    //剩余时间
            int exp_levelup = baseSkillMgr::getInstance()->getExpNeed(m_skill_queue[i].skill->skill->base_skill_id, m_skill_queue[i].skill->level + 1);
            que.push_back( Pair("expMax", exp_levelup) );//升级经验

            json_spirit::Object sk;    //技能信息
            sk.push_back( Pair("id", m_skill_queue[i].skill->skill->base_skill_id) );
            sk.push_back( Pair("spic", m_skill_queue[i].skill->skill->base_skill_id) );
            sk.push_back( Pair("level", m_skill_queue[i].skill->level) );
            que.push_back( Pair("skillVO", sk) );

            //能否加速
            if (m_vip >= skill_queue_speedup_vip)
            {
                que.push_back( Pair("faster", 1) );
            }
            if (!diamond && m_skill_queue[i].type == 0 && m_vip >= skill_queue_levelup_vip)
            {
                diamond = true;
                que.push_back( Pair("diamond", 1) );    //vip4可以变金钻
                que.push_back( Pair("gold", skill_queue_levelup_gold) );
            }
            //ques.push_back(que);
        }
        else
        {
            que.push_back( Pair("position", m_skill_queue[i].pos) );    //位置
            que.push_back( Pair("state", m_skill_queue[i].state + 1) );        //状态
            que.push_back( Pair("type", m_skill_queue[i].type + 1) );    //队列级别
            //能否开钻
            if (!diamond && m_skill_queue[i].type == 0 && m_vip >= skill_queue_levelup_vip)
            {
                diamond = true;
                que.push_back( Pair("diamond", 1) );    //vip4可以变金钻
                que.push_back( Pair("gold", skill_queue_levelup_gold) );
            }
            //ques.push_back(que);
        }
        struct tm tm_now;
        time_t t_now = time(NULL);
        localtime_r(&t_now, &tm_now);
        time_t refresh_time = spls_mktime(tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, iRefreshHour, iRefreshMin);
        if ((t_now >= refresh_time && m_skill_queue[i].accelerate_time < refresh_time)
            || (t_now < refresh_time && m_skill_queue[i].accelerate_time < (refresh_time - 86400)))
        {
            m_skill_queue[i].accelerate_time = refresh_time;
            m_skill_queue[i].fatigue = 0;
        }
        //疲劳度
        que.push_back( Pair("tiredTotal", m_skill_queue[i].fatigue) );
        que.push_back( Pair("tiredMax", m_area * 24) );
        //加入队列
        ques.push_back(que);
    }
    //剩余没开的队列
    for (int i = m_skill_queue.size() + 1; i <= skill_queue_max; ++i)
    {
        json_spirit::Object que;
        int state = 0;
        //满足开放要求，显示需要金币
        if (m_vip >= skill_queue_data[i-1][0])
        {
            state = 3;
            que.push_back( Pair("gold",  skill_queue_data[i-1][1]) );    //需要的金币
        }
        else
        {
            state = 4;
            que.push_back( Pair("vip",  skill_queue_data[i-1][0]) );    //vip等级
        }
        que.push_back( Pair("position",  i) );    //位置
        que.push_back( Pair("state",  state) );    //状态
        ques.push_back(que);
    }
    o.push_back ( Pair("list", ques) );
    return HC_SUCCESS;
}

//获得技能研究信息
int CharData::getSkillResearchInfo(int sid, json_spirit::Object& o)
{
    //技能id字段返回
    o.push_back( Pair("id", sid) );
    //o.push_back( Pair("ling", 1) );
    if (m_area > 0 && m_area <= max_map_id)
    {
        o.push_back( Pair("silver", skill_teacher_update_silver[m_area-1]) );
    }
    //是否可以使用金币刷新
    if (m_vip >= iSkill_teacher_update_gold_vip_level)
    {
        o.push_back( Pair("gold", iSkill_teacher_update_gold) );
    }
    if (!m_teachers[0].get())
    {
        m_teachers_change = true;
        baseSkillMgr::getInstance()->updateTeacher(m_area, m_teachers);
    }

    charSkill* pcSkill = m_skill_list[sid].get();
    int start_level = 0, end_level = 0;
    int start_level2 = 0, end_level2 = 0;
    int need_exp = 0;
    int need_exp2 = 0;
    if (pcSkill)
    {
        need_exp = baseSkillMgr::getInstance()->getExpNeed(sid, pcSkill->level + 1);
        need_exp2 = baseSkillMgr::getInstance()->getExpNeed(sid, pcSkill->level + 2);
        start_level = pcSkill->level;
        start_level2 = getPercent(pcSkill->exp, need_exp);
        //cout<<" start level "<<pcSkill->exp<<"/"<<need_exp<<endl;
    }
    //研究需要的银币
    o.push_back( Pair("researchSilver", (start_level*start_level-2*start_level)*50+1000) );

    json_spirit::Array teacher_list;
    for (int i = 0; i < skill_teacher_nums; ++i)
    {
        if (m_teachers[i].get())
        {
            json_spirit::Object t;
            if (pcSkill)
            {
                end_level = 0;
                int exp_end = pcSkill->exp + m_teachers[i]->speed * m_teachers[i]->total_mins /skill_research_mins;
                if (need_exp < 0)
                {
                    end_level = pcSkill->level;
                }
                //cout<<" exp end "<<exp_end<<endl;
                else if (exp_end >= need_exp)
                {
                    exp_end -= need_exp;
                    if (need_exp2 < 0)
                    {
                        end_level = pcSkill->level + 1;
                    }
                    else if (exp_end >= need_exp2)
                    {
                        exp_end -= need_exp2;
                        end_level = pcSkill->level + 2;
                        end_level2 = getPercent(exp_end, baseSkillMgr::getInstance()->getExpNeed(sid, pcSkill->level + 3));
                    }
                    else
                    {
                        end_level = pcSkill->level + 1;
                        end_level2 = getPercent(exp_end, need_exp2);
                    }
                }
                else
                {
                    //cout<<exp_end<<"|"<<need_exp<<"|"<<getPercent(exp_end, need_exp)<<endl;
                    end_level = pcSkill->level;
                    end_level2 = getPercent(exp_end, need_exp);
                    //cout<<"end level :"<<start_level<<"=>"<<LEX_CAST_STR(end_level)<<endl;
                }
                t.push_back( Pair("levelStart", percentToString(start_level, start_level2)) );
                t.push_back( Pair("levelEnd", percentToString(end_level, end_level2)) );
            }
            t.push_back( Pair("id", m_teachers[i]->id) );
            t.push_back( Pair("name", m_teachers[i]->name) );
            t.push_back( Pair("color", m_teachers[i]->quality) );
            t.push_back( Pair("totalTime", m_teachers[i]->total_mins) );
            t.push_back( Pair("expSpeed", m_teachers[i]->speed/skill_research_mins) );
            teacher_list.push_back(t);
        }
    }
    o.push_back( Pair("list", teacher_list) );
    return HC_SUCCESS;
}

//技能掉落，技能升级
int CharData::skillLevelup(int sid, int level)
{
    boost::shared_ptr<charSkill> sk;
    std::map<int, boost::shared_ptr<charSkill> >::iterator it = m_skill_list.find(sid);
    if (it != m_skill_list.end() && it->second.get())
    {
        sk = it->second;
        if (sk->level < level)
        {
            ++sk->level;
            //更新任务
            updateTask(task_skill_level, sid, sk->level);
            updateCombatAttribute();

            //攻擊力變化
            set_attack_change();

            sk->save();
            return sk->level;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        InsertSaveDb("insert into char_skills (cid,sid,level,exp) values (" + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(sid) + ",1,0)");
        {
            sk.reset(new charSkill);
            m_skill_list[sid] = sk;
            sk->cid = m_id;
            sk->level = 1;
            sk->exp = 0;
            sk->state = 0;
            sk->skill = baseSkillMgr::getInstance()->GetBaseSkill(sid);
            sk->save();
            //更新任务
            updateTask(task_skill_level, sid, sk->level);
            updateCombatAttribute();

            //攻擊力變化
            set_attack_change();
            return sk->level;
        }
    }
    return -1;
}

//技能掉落，技能升级
int CharData::setSkillLevel(int sid, int level)
{
    if (level >= 100)
    {
        level = 100;
    }
    if (level <= 0)
    {
        level = 1;
    }
    boost::shared_ptr<charSkill> sk;
    std::map<int, boost::shared_ptr<charSkill> >::iterator it = m_skill_list.find(sid);
    if (it != m_skill_list.end() && it->second.get())
    {
        sk = it->second;
        if (level <= 0)
        {
            InsertSaveDb("delete from char_skills where cid=" + LEX_CAST_STR(m_id) + " and sid=" + LEX_CAST_STR(sid));
            m_skill_list.erase(it);
            updateCombatAttribute();

            //攻擊力變化
            set_attack_change();
            return 0;
        }
        sk->level = level;
        //更新任务
        updateTask(task_skill_level, sid, sk->level);
        updateCombatAttribute();
        sk->save();

        //攻擊力變化
        set_attack_change();
        return sk->level;
    }
    else
    {
        if (level <= 0)
        {
            return 0;
        }
        InsertSaveDb("insert into char_skills (cid,sid,level,exp) values (" + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(sid) + "," + LEX_CAST_STR(level) + ",0)");
        {
            sk.reset(new charSkill);
            m_skill_list[sid] = sk;
            sk->cid = m_id;
            sk->level = level;
            sk->exp = 0;
            sk->state = 0;
            sk->skill = baseSkillMgr::getInstance()->GetBaseSkill(sid);
            sk->save();
            updateCombatAttribute();

            //攻擊力變化
            set_attack_change();
            return sk->level;
        }
    }
    return -1;
}

//刷新研究者
int CharData::updateTeachers(json_spirit::Object& o, int type)
{
    if (type == 1)
    {
        //银币不够
        if (addSilver(-skill_teacher_update_silver[m_area - 1]) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_SILVER;
        }
        //银币消耗统计
        add_statistics_of_silver_cost(m_id,m_ip_address,skill_teacher_update_silver[m_area - 1],silver_cost_for_refresh_research, m_union_id, m_server_id);
    }
    else
    {
        if (m_vip < iSkill_teacher_update_gold_vip_level)
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        if (addGold(-iSkill_teacher_update_gold) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(m_id, m_ip_address, iSkill_teacher_update_gold, gold_cost_for_refresh_research, m_union_id, m_server_id);
    }
    m_teachers_change = true;
    NotifyCharData();
    //保存
    return baseSkillMgr::getInstance()->updateTeacher(m_area, m_teachers);
}

//购买
int CharData::buyResearchQue(json_spirit::Object& o)
{
    int buy_pos = m_skill_queue.size() + 1;
    //队列满了
    if (buy_pos > skill_queue_max)
    {
        return HC_ERROR_SKILL_QUEUE_MAX;
    }
    //判断vip条件是否满足
    if (m_vip < skill_queue_data[buy_pos-1][0])
    {
        o.push_back( Pair("vip", skill_queue_data[buy_pos-1][0]) );
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    //金币是否足够
    if (addGold(-skill_queue_data[buy_pos-1][1]) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //金币消耗统计
    add_statistics_of_gold_cost(m_id, m_ip_address, skill_queue_data[buy_pos-1][1], gold_cost_for_buy_research_que, m_union_id, m_server_id);
    InsertSaveDb("insert into char_skill_research (pos,cid,type,sid,teacher,starttime,state) values ("
            + LEX_CAST_STR(buy_pos) + ","
            + LEX_CAST_STR(m_id) + ",0,0,0,0,0)");
    {
        NotifyCharData();
        skillResearchQue que;
        que.pos = buy_pos;
        que.cid = m_id;
        que.start_time = 0;
        que.left_mins = 0;
        que.state = 0;
        que.exp_added = 0;
        que.type = 0;
        que.final_speed = 0;
        m_skill_queue.push_back(que);
        que.save();
        return HC_SUCCESS;
    }
}

//升级队列
int CharData::upgradeResearchQue(json_spirit::Object& o)
{
    //判断vip条件是否满足
    if (m_vip < skill_queue_levelup_vip)
    {
        o.push_back( Pair("vip", skill_queue_levelup_vip) );
        return HC_ERROR;
    }
    for (size_t i = 0; i < m_skill_queue.size(); ++i)
    {
        if (m_skill_queue[i].type == 0)
        {
            //金币是否足够
            if (addGold(-skill_queue_levelup_gold) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            //金币消耗统计
            add_statistics_of_gold_cost(m_id, m_ip_address, skill_queue_levelup_gold, gold_cost_for_upgrade_research_que, m_union_id, m_server_id);
#ifdef QQ_PLAT
            gold_cost_tencent(this,skill_queue_levelup_gold,gold_cost_for_upgrade_research_que);
#endif
            NotifyCharData();
            m_skill_queue[i].type = 1;
            m_skill_queue[i].more = skill_vip_queue_more_speed;
            m_skill_queue[i].save();
            return HC_SUCCESS;
        }
    }
    //找不到可以升级的队列
    return HC_ERROR;
}

//加速研究 type = 0 查询金币消耗
int CharData::speedupResearch(int sid, int type, json_spirit::Object& o)
{
    o.push_back( Pair("id", sid) );
    //判断队列是否有在研究这个技能
    for (size_t i = 0; i < m_skill_queue.size(); ++i)
    {
        if (m_skill_queue[i].state == 1
            && m_skill_queue[i].skill.get()
            && m_skill_queue[i].skill->skill.get()
            && m_skill_queue[i].skill->skill->base_skill_id == sid)
        {
            if (m_skill_queue[i].state == 0)
            {
                return HC_ERROR;
            }
            int goldneed = (m_skill_queue[i].left_mins / skill_queue_speedup_mins)
                        + (m_skill_queue[i].left_mins % skill_queue_speedup_mins ? 1 : 0);
            if (goldneed < 0)
            {
                goldneed = 0;
            }

            o.push_back( Pair("gold", goldneed) );

            struct tm tm_now;
            time_t t_now = time(NULL);
            localtime_r(&t_now, &tm_now);
            time_t refresh_time = spls_mktime(tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, iRefreshHour, iRefreshMin);
            if ((t_now >= refresh_time && m_skill_queue[i].accelerate_time < refresh_time)
                || (t_now < refresh_time && m_skill_queue[i].accelerate_time < (refresh_time - 86400)))
            {
                m_skill_queue[i].accelerate_time = refresh_time;
                m_skill_queue[i].fatigue = 0;
            }
            if (m_skill_queue[i].fatigue + goldneed > m_area * 24)
            {
                return HC_ERROR_RESEARCH_TOO_TIRED;
            }
            //找到了
            if (type == 1)
            {
                if (addGold(-goldneed) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                //金币消耗统计
                add_statistics_of_gold_cost(m_id, m_ip_address, goldneed, gold_cost_for_accelerate_research, m_union_id, m_server_id);
                //增加疲劳度
                m_skill_queue[i].fatigue += goldneed;
                m_skill_queue[i].accelerate_time = t_now;
                NotifyCharData();
                m_skill_queue[i].skill->exp += (m_skill_queue[i].final_speed * m_skill_queue[i].left_mins/skill_research_mins);

                int need_exp = baseSkillMgr::getInstance()->getExpNeed(m_skill_queue[i].skill->skill->base_skill_id, m_skill_queue[i].skill->level + 1);
                bool blevelup = false;
                while (-1 != need_exp && m_skill_queue[i].skill->exp >= need_exp)
                {
                    ++m_skill_queue[i].skill->level;
                    m_skill_queue[i].skill->exp -= need_exp;

                    need_exp = baseSkillMgr::getInstance()->getExpNeed(m_skill_queue[i].skill->skill->base_skill_id, m_skill_queue[i].skill->level + 1);
                    //INFO("*****************skill level up !!!!!!!!!!");
                    //是否需要通知玩家升级了
                    updateTask(task_skill_level, sid, m_skill_queue[i].skill->level);
                    blevelup = true;
                }
                if (blevelup)
                {
                    updateCombatAttribute();
                    //攻擊力變化
                    set_attack_change();
                }

                m_skill_queue[i].save();
                m_skill_queue[i].skill->save();

                m_skill_queue[i].stop();

                return HC_SUCCESS;
            }
            else
            {
                return HC_SUCCESS;
            }
        }
    }
    //没有在研究这个队列啊
    return HC_ERROR;
}

/******************** 训练相关 *******************************/

int CharData::openTrain()
{
    //TrainMgr::getInstance()->updateBook(0, m_book);
    m_book[0] = TrainMgr::getInstance()->GetBook(9);
    #if 0
    for (int i = 1; i <= 5; ++i)
    {
        int state = 0;
        if (i == 1)
        {
            state = 1;
        }
        InsertSaveDb("insert into char_train_place (cid,pos,state) values (" + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(i) + "," + LEX_CAST_STR(state) + ")");
        generalTrainQue que;
        que.pos = i;
        que.cid = m_id;
        que.start_time = 0;
        que.end_time = 0;
        que.state = state;
        m_train_queue.push_back(que);
    }
    #else
    InsertSaveDb("insert into char_train_place (cid,pos,state) values ("
        + LEX_CAST_STR(m_id) + ",1,1)");
    generalTrainQue que;
    que.pos = 1;
    que.cid = m_id;
    que.type = 0;
    que.start_time = 0;
    que.end_time = 0;
    que.state = 1;
    que.speed_time = 0;
    m_train_queue.push_back(que);
    #endif
    for (int i = 1; i <= general_book_nums; ++i)
    {
        InsertSaveDb("replace into char_train_books (cid,pos,bid) values (" + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(i) + "," + LEX_CAST_STR(m_book[i - 1]->id) + ")");
    }
    return HC_SUCCESS;
}

int CharData::LoadTrainList()
{
    Query q(GetDb());
    m_train_queue.clear();
    //武将训练队列
    q.get_result("select pos,cid,gid,type,starttime,endtime,state,speed_time,cur_level,pre_level,cur_color,pre_color from char_train_place where state!= 0 and cid=" + LEX_CAST_STR(m_id) + " order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        if ((int)m_train_queue.size() >= skill_queue_max)
        {
            break;
        }
        generalTrainQue que;
        int pos = q.getval();
        que.pos = m_train_queue.size() + 1;
        int cid = q.getval();
        que.cid = cid;
        int gid = q.getval();
        que.type = q.getval();
        que.start_time = q.getval();
        que.end_time = q.getval();
        que.state = q.getval();
        que.speed_time = q.getval();
        int cur_level = q.getval();
        int pre_level = q.getval();
        int cur_color = q.getval();
        int pre_color = q.getval();
        boost::shared_ptr<charGeneral> p;
        p.reset();
        if (que.state == 2)
        {
            INFO("load train general=" << gid << endl);
            p.reset(new charGeneral);
            p->cid = cid;
            p->gid = gid;
            p->cur_level = cur_level;
            p->pre_level = pre_level;
            p->cur_color = cur_color;
            p->pre_color = pre_color;
        }
        que.general = p;
        m_train_queue.push_back(que);
        //cout<<"******load train queue "<<que.state<<","<<cid<<","<<gid<<endl;
        //启动定时研究
        if (que.state == 2 && gid > 0)
        {
            que.start();
        }
        if (pos != que.pos)
        {
            que.save();
        }
    }
    q.free_result();
    if (m_train_queue.size() == 0 && m_trainOpen)
    {
        openTrain();
    }
    return 0;
}

//获得训练列表
int CharData::getTrainList(json_spirit::Object& o)
{
    json_spirit::Array list;
    for (int i = 0; i < (int)m_train_queue.size(); ++i)
    {
        json_spirit::Object t;
        t.push_back( Pair("state", m_train_queue[i].state) );
        t.push_back( Pair("type", m_train_queue[i].type + 1) );
        if (m_train_queue[i].state == 2 && m_train_queue[i].general.get())
        {
            INFO("get train general=" << m_train_queue[i].general->gid << endl);
            t.push_back( Pair("id", m_train_queue[i].general->gid) );
            boost::shared_ptr<CharGeneralData> p_gd = m_generals.GetGenral(m_train_queue[i].general->gid);
            if (p_gd.get())
            {
                boost::shared_ptr<GeneralTypeData> p = GeneralDataMgr::getInstance()->GetBaseGeneral(p_gd->m_gid);
                if (p.get())
                {
                    t.push_back( Pair("name", p->m_name) );
                    t.push_back( Pair("spic", p->m_spic) );
                }
            }
            t.push_back( Pair("pre_color", m_train_queue[i].general->pre_color) );
            t.push_back( Pair("pre_level", m_train_queue[i].general->pre_level) );
            t.push_back( Pair("cur_color", m_train_queue[i].general->cur_color) );
            t.push_back( Pair("cur_level", m_train_queue[i].general->cur_level) );

            t.push_back( Pair("coolTime", m_train_queue[i].end_time - time(NULL)) );
            t.push_back( Pair("speedCost", m_train_queue[i].getSpeedGold()) );
        }
        if (m_train_queue[i].type == 0 && m_vip >= general_queue_levelup_vip)
        {
            t.push_back( Pair("gold", general_queue_levelup_gold) );
        }
        list.push_back(t);
    }
    for (int i = (int)m_train_queue.size(); i < skill_queue_max; ++i)
    {
        json_spirit::Object t;
        t.push_back( Pair("state", 0) );
        //满足开放要求，显示需要金币
        if (m_vip >= general_queue_data[i][0])
        {
            t.push_back( Pair("gold",  general_queue_data[i][1]) );    //需要的金币
        }
        else
        {
            t.push_back( Pair("vip",  general_queue_data[i][0]) );    //vip等级
        }
        list.push_back(t);
    }
    o.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//获得兵书列表
int CharData::getBookList(int id, json_spirit::Object& o)
{
    boost::shared_ptr<CharGeneralData> gd = m_generals.GetGenral(id);
    if (gd.get())
    {
        json_spirit::Object g;
        g.push_back( Pair("id", id) );
        g.push_back( Pair("rebornOpen", m_rebornOpen) );
        g.push_back( Pair("level", gd->m_level) );
        g.push_back( Pair("quality", gd->m_color) );
        g.push_back( Pair("growRate", gd->m_chengzhang));
        g.push_back( Pair("growRate_max", iChengZhangMax[gd->m_color]));
        g.push_back( Pair("brave", gd->m_str));
        g.push_back( Pair("wisdom", gd->m_int));
        g.push_back( Pair("govern", gd->m_tongyu));
        int rlv = rebornLevel(gd->m_color, gd->m_chengzhang);
        g.push_back( Pair("rebornLv", rlv));
        g.push_back( Pair("reborn_point", gd->m_reborn_point));
        int reborn_point_max = iRebornPoint[0];
        if (gd->m_chengzhang_star.get())
        {
            reborn_point_max = iRebornPoint[gd->m_chengzhang_star->id];
        }
        g.push_back( Pair("reborn_point_max", reborn_point_max));
        if (rlv > 0 && gd->m_level >= rlv && m_vip >= iOnekeyRebronVip)
        {
            g.push_back( Pair("gold", fastRebornGold(gd->m_chengzhang)) );
        }
        json_spirit::Object reborn_star_info;
        if (gd->m_chengzhang_star.get())
        {
            reborn_star_info.push_back( Pair("star", gd->m_chengzhang_star->id));
            reborn_star_info.push_back( Pair("gongji", gd->m_chengzhang_star->gongji));
            reborn_star_info.push_back( Pair("fangyu", gd->m_chengzhang_star->fangyu));
            reborn_star_info.push_back( Pair("bingli", gd->m_chengzhang_star->bingli));
            g.push_back( Pair("reborn_star_info", reborn_star_info));
        }
        if (gd->m_chengzhang_next_star.get())
        {
            reborn_star_info.clear();
            reborn_star_info.push_back( Pair("star", gd->m_chengzhang_next_star->id));
            reborn_star_info.push_back( Pair("gongji", gd->m_chengzhang_next_star->gongji));
            reborn_star_info.push_back( Pair("fangyu", gd->m_chengzhang_next_star->fangyu));
            reborn_star_info.push_back( Pair("bingli", gd->m_chengzhang_next_star->bingli));
            reborn_star_info.push_back( Pair("need_chengzhang", gd->m_chengzhang_next_star->need_chengzhang));
            g.push_back( Pair("reborn_next_star_info", reborn_star_info));
        }
        o.push_back( Pair("ginfo", g) );
    }
    if (m_area > 0 && m_area <= max_map_id)
    {
        json_spirit::Object info;
		int cost_silver = 10 * m_level * m_level;
		if (g_reborn_discount < 100)
		{
			cost_silver = cost_silver * g_reborn_discount / 100;
			if (cost_silver < 1)
			{
				cost_silver = 1;
			}
		}
        info.push_back( Pair("silver", cost_silver) );
        int times = queryExtraData(char_data_type_daily, char_data_book_refresh);

        if (m_vip >= iBookRefreshVIP)
        {
            int tmp = 0;
            #ifdef JP_SERVER
            tmp = (times+1);
            #else
            tmp = (times+1)/5;
            tmp += ((times+1)%5 > 0 ? 1 : 0);
            #endif
            int cost_gold = iBookRefreshGold * tmp;
            if (cost_gold > 20)
                cost_gold = 20;
			if (g_reborn_discount < 100)
			{
				cost_gold = cost_gold * g_reborn_discount / 100;
				if (cost_gold < 1)
				{
					cost_gold = 1;
				}
			}
            info.push_back( Pair("gold", cost_gold) );
        }
        if (m_vip >= iBookBestRefresh)
        {
        	int cost_gold = iBookBestRefreshGold;
			if (g_reborn_discount < 100)
			{
				cost_gold = cost_gold * g_reborn_discount / 100;
				if (cost_gold < 1)
				{
					cost_gold = 1;
				}
			}
            info.push_back( Pair("gold2", cost_gold) );
        }
        info.push_back( Pair("book_ling", m_bag.getGemCount(treasure_type_book_refresh)) );
        o.push_back( Pair("info", info) );
    }

    if (!m_book[0].get())
    {
        m_book_change = true;
        TrainMgr::getInstance()->updateBook(0, m_book);
    }
    json_spirit::Array list;
    for (int i = 0; i < general_book_nums; ++i)
    {
        if (m_book[i].get())
        {
            json_spirit::Object t;
            t.push_back( Pair("id", m_book[i]->id) );
            t.push_back( Pair("name", m_book[i]->name) );
            t.push_back( Pair("quality", m_book[i]->quality) );
            t.push_back( Pair("memo", m_book[i]->memo) );
            t.push_back( Pair("addLevel", m_book[i]->uplevel) );
            t.push_back( Pair("trainTime", m_book[i]->hours) );
            list.push_back(t);
        }
    }
    o.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//武将训练
int CharData::generalTrain(int gid, int bid, int pos, json_spirit::Object& o)
{
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.find(gid);
    if (it != m_generals.m_generals.end() && it->second.get())
    {
        //if (it->second->m_level >= m_level)
        //{
        //    return HC_ERROR_GENERAL_LEVEL_MAX;
        //}
        boost::shared_ptr<Book> p_b;
        p_b.reset();
        for (int i = 0; i < general_book_nums; ++i)
        {
            if (m_book[i].get() && m_book[i]->id == bid)
            {
                p_b = m_book[i];
                break;
            }
        }
        if (!p_b.get())
        {
            return HC_ERROR;
        }
        if (pos > 0 && pos <= (int)m_train_queue.size())
        //for (int i = 0; i < (int)m_train_queue.size(); ++i)
        {
            int i = pos - 1;
            if (m_train_queue[i].state == 1)
            {
                if (-1 == addLing(-1))
                {
                    return HC_ERROR_NOT_ENOUGH_LING;
                }
                //军令统计
                add_statistics_of_ling_cost(m_id,m_ip_address,1,ling_hero_train,2, m_union_id, m_server_id);

                int final_level = it->second->m_level + p_b->uplevel;
                int add_reborn_point = 0;
                if (m_train_queue[i].type == 1)
                    final_level += general_vip_queue_more_level;
                if (final_level > m_level)
                {
                    add_reborn_point = final_level - m_level;
                    final_level = m_level;
                }
                if (add_reborn_point && m_rebornOpen)
                {
                    it->second->m_reborn_point += add_reborn_point;
                    it->second->m_changed = true;
                    it->second->Save();
                    o.push_back( Pair("get_reborn_point", add_reborn_point) );
                }
                m_train_queue[i].state = 2;
                m_train_queue[i].pos = i + 1;
                m_train_queue[i].cid = m_id;
                boost::shared_ptr<charGeneral> p_g;
                p_g.reset(new charGeneral);
                p_g->cid = m_id;
                p_g->gid = it->second->m_id;
                p_g->pre_level = it->second->m_level;
                p_g->pre_color = it->second->m_color;
                while (1)
                {
                    if (-1 == it->second->Levelup(final_level))
                    {
                        break;
                    }
                }
                //武将升级活动
                shhx_generl_upgrade_event::getInstance()->add_score(m_id, it->second->m_level - p_g->pre_level);

                p_g->cur_level = it->second->m_level;
                p_g->cur_color = it->second->m_color;
                m_train_queue[i].general = p_g;
                m_train_queue[i].start_time = time(NULL);
                if (m_train_queue[i].type == 1)
                    m_train_queue[i].end_time = m_train_queue[i].start_time + (p_b->hours * 1800);
                else
                    m_train_queue[i].end_time = m_train_queue[i].start_time + p_b->hours * 3600;
                m_train_queue[i].start();
                m_train_queue[i].save();
                TrainMgr::getInstance()->updateBook(0, m_book);
                m_book_change = true;

                NotifyCharData();
                NotifyZhenData();
                //日常任务
                dailyTaskMgr::getInstance()->updateDailyTask(*this,daily_task_general_train);
                if (it->second->m_baseGeneral)
                {
                    o.push_back( Pair("name", it->second->m_baseGeneral->m_name) );
                }
                //支线任务
                m_trunk_tasks.updateTask(task_train, 1);

                o.push_back( Pair("pre_level", p_g->pre_level) );
                o.push_back( Pair("cur_level", p_g->cur_level) );
                //act统计
                act_to_tencent(this,act_new_train);
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

//升级队列
int CharData::upgradeGeneralTrainQue(int pos, json_spirit::Object& o)
{
    //判断vip条件是否满足
    if (m_vip < general_queue_levelup_vip)
    {
        o.push_back( Pair("vip", general_queue_levelup_vip) );
        return HC_ERROR;
    }
    if (pos > 0 && pos <= (int)m_train_queue.size())
    //for (size_t i = 0; i < m_train_queue.size(); ++i)
    {
        int i = pos - 1;
        if (m_train_queue[i].state > 0 && m_train_queue[i].type == 0)
        {
            //金币是否足够
            if (addGold(-general_queue_levelup_gold) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            //金币消耗统计
            add_statistics_of_gold_cost(m_id, m_ip_address, general_queue_levelup_gold, gold_cost_for_upgrade_research_que, m_union_id, m_server_id);
#ifdef QQ_PLAT
            gold_cost_tencent(this,general_queue_levelup_gold,gold_cost_for_upgrade_research_que);
#endif
            NotifyCharData();
            m_train_queue[i].type = 1;
            m_train_queue[i].save();
            return HC_SUCCESS;
        }
    }
    //找不到可以升级的队列
    return HC_ERROR;
}

//训练位冷却重置
int CharData::generalTrainSpeed(int pos)
{
    if (m_vip < iTrainSpeedVip)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    if (pos > 0 && pos <= (int)m_train_queue.size() && m_train_queue[pos-1].state == 2)
    {
        time_t time_now = time(NULL);
        int need_gold = m_train_queue[pos-1].getSpeedGold();
        if (need_gold <= 0)
        {
            return HC_SUCCESS;
        }
        if (-1 == addGold(-need_gold))
            return HC_ERROR_NOT_ENOUGH_GOLD;
        //金币消耗统计
        add_statistics_of_gold_cost(m_id, m_ip_address, need_gold, gold_cost_for_accelerate_train, m_union_id, m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(this,need_gold,gold_cost_for_accelerate_train);
#endif
        splsTimerMgr::getInstance()->delTimer(m_train_queue[pos-1]._uuid);
        //if (splsTimerMgr::getInstance()->delTimer(m_train_queue[pos-1]._uuid))
        {
            ++(m_train_queue[pos - 1].speed_time);
            m_train_queue[pos - 1].state = 1;
            m_train_queue[pos - 1].general.reset();
            m_train_queue[pos - 1].start_time = 0;
            m_train_queue[pos - 1].end_time = 0;
            m_train_queue[pos - 1].save();
            m_train_queue[pos-1]._uuid = boost::uuids::nil_uuid();
        }
        //免费银币刷新一次
        m_book_change = true;
        TrainMgr::getInstance()->updateBook(1, m_book);
        NotifyCharData();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//训练位冷却状态
int CharData::generalTrainCoolTime(int& state)
{
    int lefttime = 0;
    time_t timenow = time(NULL);
    for (size_t j = 0; j < m_train_queue.size(); ++j)
    {
        if (m_train_queue[j].state == 0)
        {
            continue;
        }
        else
        {
            if (lefttime == 0 || (m_train_queue[j].end_time - timenow) < lefttime)
            {
                lefttime = m_train_queue[j].end_time - timenow;
                if (lefttime < 0)
                {
                    break;
                }
            }
        }
    }
    if (lefttime < 0)
    {
        lefttime = 0;
    }
    if (lefttime == 0)
    {
        state = 1;
    }
    else
    {
        state = 2;
    }
    return lefttime;
}

int CharData::updateBooks(int type, json_spirit::Object& robj)
{
    //银币不够
    if (type == 1)
    {
        int cost_silver = 10*m_level*m_level;
		if (g_reborn_discount < 100)
		{
			cost_silver = cost_silver * g_reborn_discount / 100;
			if (cost_silver < 1)
			{
				cost_silver = 1;
			}
		}
        if (addSilver(-cost_silver) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_SILVER;
        }
        //银币消耗统计
        add_statistics_of_silver_cost(m_id,m_ip_address,cost_silver,silver_cost_for_refresh_train, m_union_id, m_server_id);
        //act统计
        act_to_tencent(this,act_new_refresh_book_silver);
    }
    else if (type == 2)
    {
        if (m_vip < iBookRefreshVIP)
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        int times = queryExtraData(char_data_type_daily, char_data_book_refresh);

        int tmp = 0;
        #ifdef JP_SERVER
        tmp = (times+1);
        #else
        tmp = (times+1)/5;
        tmp += ((times+1)%5 > 0 ? 1 : 0);
        #endif
        int cost_gold = iBookRefreshGold * tmp;
        if (cost_gold > 20)
            cost_gold = 20;

		if (g_reborn_discount < 100)
		{
			cost_gold = cost_gold * g_reborn_discount / 100;
			if (cost_gold < 1)
			{
				cost_gold = 1;
			}
		}

        if (addTreasure(treasure_type_book_refresh, -1) >= 0)
        {
            std::string msg = treasure_expend_msg(treasure_type_book_refresh, 1);
            if (msg != "")
            {
                robj.push_back( Pair("msg", msg) );
            }
            //统计道具消耗
            add_statistics_of_treasure_cost(m_id,m_ip_address,treasure_type_book_refresh,1,treasure_unknow,2,m_union_id,m_server_id);
        }
        else if (addGold(-cost_gold) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(m_id, m_ip_address, cost_gold, gold_cost_for_refresh_train_normal, m_union_id, m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(this,cost_gold,gold_cost_for_refresh_train_normal);
#endif
        setExtraData(char_data_type_daily, char_data_book_refresh, times+1);
    }
    else
    {
        type = 3;
        if (m_vip < iBookBestRefresh)
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }
		int cost_gold = iBookBestRefreshGold;
		if (g_reborn_discount < 100)
		{
			cost_gold = cost_gold * g_reborn_discount / 100;
			if (cost_gold < 1)
			{
				cost_gold = 1;
			}
		}
        if (addGold(-cost_gold) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(m_id, m_ip_address, cost_gold, gold_cost_for_refresh_train_best, m_union_id, m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(this,cost_gold,gold_cost_for_refresh_train_best);
#endif
    }
    m_book_change = true;
    NotifyCharData();
    return TrainMgr::getInstance()->updateBook(type, m_book);
}

//购买
int CharData::buyTrainQue(int pos, json_spirit::Object& o)
{
    int can_pos = m_train_queue.size() + 1;
    //队列满了
    if (pos > general_queue_max || can_pos != pos)
    {
        return HC_ERROR;
    }
    //判断vip条件是否满足
    if (m_vip < general_queue_data[pos-1][0])
    {
        o.push_back( Pair("vip", general_queue_data[pos-1][0]) );
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    if (general_queue_data[pos-1][1] > 0)
    {
        //金币是否足够
        if (addGold(-general_queue_data[pos-1][1]) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(m_id, m_ip_address, general_queue_data[pos-1][1], gold_cost_for_buy_train_que, m_union_id, m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(this,general_queue_data[pos-1][1],gold_cost_for_buy_train_que);
#endif
        NotifyCharData();
    }

    generalTrainQue que;
    que.pos = pos;
    que.cid = m_id;
    que.type = 0;
    que.start_time = 0;
    que.end_time = 0;
    que.state = 1;
    que.speed_time = 0;
    InsertSaveDb("insert into char_train_place (cid,type,pos,state) values ("
        + LEX_CAST_STR(que.cid)
        + "," + LEX_CAST_STR(que.type)
        + "," + LEX_CAST_STR(que.pos)
        + "," + LEX_CAST_STR(que.state) + ")");

    m_train_queue.push_back(que);
    return HC_SUCCESS;
}

void CharData::resetTrainQue()
{
    for (size_t i = 0; i < m_train_queue.size(); ++i)
    {
        m_train_queue[i].resetSpeedtime();
    }
}

int CharData::generalInheritObj(int gid1, int gid2, int type, json_spirit::Object& o)
{
    if (gid1 == gid2)
    {
        return HC_ERROR_SAME_GENERAL;
    }
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.find(gid1);
    if (it != m_generals.m_generals.end() && it->second.get())
    {
        boost::shared_ptr<CharGeneralData> pg1 = it->second;
        it = m_generals.m_generals.find(gid2);
        if (it != m_generals.m_generals.end() && it->second.get())
        {
            boost::shared_ptr<CharGeneralData> pg2 = it->second;
            int src_level = 0, src_tong = 0, src_int = 0, src_str = 0;
            double src_chengzhang = 0.0;
            //传承属性
            if (type == 2)//cost treasure
            {
                src_level = pg1->m_level;
                src_tong = pg1->m_wash_tong;
                src_int = pg1->m_wash_int;
                src_str = pg1->m_wash_str;
                src_chengzhang = pg1->m_chengzhang;
            }
            else
            {
                src_level = pg1->m_level * 70 / 100;
                src_tong = pg1->m_wash_tong * 70 / 100;
                src_int = pg1->m_wash_int * 70 / 100;
                src_str = pg1->m_wash_str * 70 / 100;
                src_chengzhang = pg1->m_chengzhang * 70.0 / 100.0;
            }
            if (src_chengzhang > iChengZhangMax[pg2->m_color])
            {
                src_chengzhang = iChengZhangMax[pg2->m_color];
            }
            json_spirit::Object generalobj1;
            json_spirit::Object generalobj2;
            //各项属性比较生成传承结果
            if (src_level > pg2->m_level)
            {
                int tmp_level = src_level - pg2->m_level;
                generalobj2.push_back( Pair("level", src_level));
                generalobj1.push_back( Pair("level", pg1->m_level - tmp_level));
            }
            else
            {
                generalobj2.push_back( Pair("level", pg2->m_level));
                generalobj1.push_back( Pair("level", pg1->m_level));
            }
            if (src_tong > pg2->m_wash_tong)
            {
                int tmp_tong = src_tong - pg2->m_wash_tong;
                generalobj2.push_back( Pair("wash_tong", src_tong));
                generalobj1.push_back( Pair("wash_tong", pg1->m_wash_tong - tmp_tong));
            }
            else
            {
                generalobj2.push_back( Pair("wash_tong", pg2->m_wash_tong));
                generalobj1.push_back( Pair("wash_tong", pg1->m_wash_tong));
            }
            if (src_int > pg2->m_wash_int)
            {
                int tmp_int = src_int - pg2->m_wash_int;
                generalobj2.push_back( Pair("wash_int", src_int));
                generalobj1.push_back( Pair("wash_int", pg1->m_wash_int - tmp_int));
            }
            else
            {
                generalobj2.push_back( Pair("wash_int", pg2->m_wash_int));
                generalobj1.push_back( Pair("wash_int", pg1->m_wash_int));
            }
            if (src_str > pg2->m_wash_str)
            {
                int tmp_str = src_str - pg2->m_wash_str;
                generalobj2.push_back( Pair("wash_str", src_str));
                generalobj1.push_back( Pair("wash_str", pg1->m_wash_str - tmp_str));
            }
            else
            {
                generalobj2.push_back( Pair("wash_str", pg2->m_wash_str));
                generalobj1.push_back( Pair("wash_str", pg1->m_wash_str));
            }
            if (src_chengzhang > pg2->m_chengzhang)
            {
                double tmp_chengzhang = src_chengzhang - pg2->m_chengzhang;
                generalobj2.push_back( Pair("chengzhang", src_chengzhang));
                generalobj1.push_back( Pair("chengzhang", pg1->m_chengzhang - tmp_chengzhang));
            }
            else
            {
                generalobj2.push_back( Pair("chengzhang", pg2->m_chengzhang));
                generalobj1.push_back( Pair("chengzhang", pg1->m_chengzhang));
            }
            if (type == 2)
            {
                o.push_back( Pair("g1_yes", generalobj1));
                o.push_back( Pair("g2_yes", generalobj2));
            }
            else
            {
                o.push_back( Pair("g1_no", generalobj1));
                o.push_back( Pair("g2_no", generalobj2));
            }
            return HC_SUCCESS;
        }
        else
        {
            return HC_ERROR;
        }
    }
    return HC_ERROR;
}

int CharData::generalInheritInfo(int gid1, int gid2, json_spirit::Object& o)
{
    if (gid1 == gid2)
    {
        return HC_ERROR_SAME_GENERAL;
    }
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.find(gid1);
    if (it != m_generals.m_generals.end() && it->second.get())
    {
        boost::shared_ptr<CharGeneralData> pg1 = it->second;
        it = m_generals.m_generals.find(gid2);
        if (it != m_generals.m_generals.end() && it->second.get())
        {
            boost::shared_ptr<CharGeneralData> pg2 = it->second;
            json_spirit::Object generalobj;
            generalobj.push_back( Pair("level", pg1->m_level));
            generalobj.push_back( Pair("chengzhang", pg1->m_chengzhang));
            generalobj.push_back( Pair("wash_tong", pg1->m_wash_tong));
            generalobj.push_back( Pair("wash_int", pg1->m_wash_int));
            generalobj.push_back( Pair("wash_str", pg1->m_wash_str));
            o.push_back( Pair("g1", generalobj));
            generalobj.clear();
            generalobj.push_back( Pair("level", pg2->m_level));
            generalobj.push_back( Pair("chengzhang", pg2->m_chengzhang));
            generalobj.push_back( Pair("wash_tong", pg2->m_wash_tong));
            generalobj.push_back( Pair("wash_int", pg2->m_wash_int));
            generalobj.push_back( Pair("wash_str", pg2->m_wash_str));
            o.push_back( Pair("g2", generalobj));
            //插入传承结果
            generalInheritObj(gid1,gid2,1,o);
            generalInheritObj(gid1,gid2,2,o);
            return HC_SUCCESS;
        }
        else
        {
            return HC_ERROR;
        }
    }
    return HC_ERROR;
}

//武将传承
int CharData::generalInherit(int gid1, int gid2, int type, json_spirit::Object& o)
{
    if (gid1 == gid2)
    {
        return HC_ERROR_SAME_GENERAL;
    }
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.find(gid1);
    if (it != m_generals.m_generals.end() && it->second.get())
    {
        boost::shared_ptr<CharGeneralData> pg1 = it->second;
        if (!pg1->m_baseGeneral.get())
            return HC_ERROR;
        it = m_generals.m_generals.find(gid2);
        if (it != m_generals.m_generals.end() && it->second.get())
        {
            boost::shared_ptr<CharGeneralData> pg2 = it->second;
            int src_level = 0, src_tong = 0, src_int = 0, src_str = 0;
            double src_chengzhang = 0.0;
            //传承属性
            if (type == 2)//cost treasure
            {
                //act统计
                act_to_tencent(this,act_new_inherit, pg1->m_gid, pg2->m_gid, 1);
                if (addTreasure(treasure_type_chuanchengdan, -pg1->m_baseGeneral->m_inherit_cnt) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_CHUANCHENG;
                }
#ifdef QQ_PLAT
                treasure_cost_tencent(this,treasure_type_chuanchengdan,pg1->m_baseGeneral->m_inherit_cnt);
#endif
                src_level = pg1->m_level;
                src_tong = pg1->m_wash_tong;
                src_int = pg1->m_wash_int;
                src_str = pg1->m_wash_str;
                src_chengzhang = pg1->m_chengzhang;
            }
            else
            {
                //act统计
                act_to_tencent(this,act_new_inherit, pg1->m_gid, pg2->m_gid, 0);
                src_level = pg1->m_level * 70 / 100;
                src_tong = pg1->m_wash_tong * 70 / 100;
                src_int = pg1->m_wash_int * 70 / 100;
                src_str = pg1->m_wash_str * 70 / 100;
                src_chengzhang = pg1->m_chengzhang * 70.0 / 100.0;
            }
            if (src_chengzhang > iChengZhangMax[pg2->m_color])
            {
                src_chengzhang = iChengZhangMax[pg2->m_color];
            }
            //各项属性比较生成传承结果
            //bool level_change = false;
            if (src_level > pg2->m_level)
            {
                int tmp_level = src_level - pg2->m_level;
                pg2->m_level = src_level;
                pg1->m_level -= tmp_level;
                //level_change = true;
            }
            if (src_tong > pg2->m_wash_tong)
            {
                int tmp_tong = src_tong - pg2->m_wash_tong;
                pg2->m_wash_tong = src_tong;
                pg1->m_wash_tong -= tmp_tong;
            }
            if (src_int > pg2->m_wash_int)
            {
                int tmp_int = src_int - pg2->m_wash_int;
                pg2->m_wash_int = src_int;
                pg1->m_wash_int -= tmp_int;
            }
            if (src_str > pg2->m_wash_str)
            {
                int tmp_str = src_str - pg2->m_wash_str;
                pg2->m_wash_str = src_str;
                pg1->m_wash_str -= tmp_str;
            }
            pg1->updateWashStar();
            pg2->updateWashStar();
            if (src_chengzhang > pg2->m_chengzhang)
            {
                double tmp_chengzhang = src_chengzhang - pg2->m_chengzhang;
                pg2->m_chengzhang = src_chengzhang;
                pg1->m_chengzhang -= tmp_chengzhang;
                pg1->updateChengzhangStar();
                pg2->updateChengzhangStar();
            }
            //if (level_change)
            {
                //等级加成重新计算
                pg1->m_add = 0.0;
                for (int level = 2; level <= pg1->m_level; ++level)
                {
                    double add = pg1->m_chengzhang;
                    int temp = (int)(add * 100);
                    pg1->m_add += (double)temp/100;
                }
                pg2->m_add = 0.0;
                for (int level = 2; level <= pg2->m_level; ++level)
                {
                    double add = pg2->m_chengzhang;
                    int temp = (int)(add * 100);
                    pg2->m_add += (double)temp/100;
                }
            }
            //攻擊力變化
            set_attack_change();
            //新战力
            pg1->general_change = true;
            pg1->wash_change = true;
            pg1->reborn_change = true;

            pg2->general_change = true;
            pg2->wash_change = true;
            pg2->reborn_change = true;

            pg1->updateAttribute();
            pg2->updateAttribute();
            NotifyZhenData();
            //通知结果信息

            //支线任务
            m_trunk_tasks.updateTask(task_general_inherit, 1);
            return HC_SUCCESS;
        }
        else
        {
            return HC_ERROR;
        }
    }
    return HC_ERROR;
}

int CharData::buyInherit(int num)
{
    if (num < 0)
        return HC_ERROR;
    if (addGold(-num*50) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //金币消耗统计
    add_statistics_of_gold_cost(m_id, m_ip_address, 50*num, gold_cost_for_treasure+treasure_type_chuanchengdan, m_union_id, m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(this,50,gold_cost_for_buy_daoju,treasure_type_chuanchengdan,num);
#endif
    addTreasure(treasure_type_chuanchengdan, num);
    return HC_SUCCESS;
}

/******************** 休息相关 *******************************/

//获得休息金币消耗
int CharData::getRestCost(int times)
{
    if (times < 1)
        return 0;
    if (times <= 1)
    {
        return 5;
    }
    else if(times <= 3)
    {
        return 10;
    }
    else if(times <= 7)
    {
        return 20;
    }
    else if(times <= 15)
    {
        return 40;
    }
    else if(times <= 22)
    {
        return 60;
    }
    else
    {
        return 80;
    }
    return 0;
}

//休息
int CharData::rest(int type, json_spirit::Object& robj)
{
    if (m_buyLingOpen == 0)
    {
        return HC_ERROR;
    }
    if (type == 1)
    {
        if (m_welfare == 0)
        {
        #ifdef VN_SERVER
            int ling_get = 12;
            //注册前三天
            if (regDays() <= 3)
            {
                ling_get = 20;
            }
        #else
            int ling_get = 12;
        #endif
            //军令获得数值4倍
            if(ling_get > 0)
                ling_get *= 4;
            addLing(ling_get);
            m_welfare = 1;
            NotifyCharData();
            add_statistics_of_ling_cost(m_id, m_ip_address,ling_get,ling_rest_by_active, 1, m_union_id, m_server_id);
            InsertSaveDb("replace into char_data_temp (cid,welfare) values (" + LEX_CAST_STR(m_id)
                    + "," + LEX_CAST_STR(m_welfare)
                    + ")");
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
            if (account)
            {
                account->Send(strRestUnNotify);
            }
            //act统计
            act_to_tencent(this,act_new_ling_free);
            return HC_SUCCESS;
        }
        else
        {
            return HC_ERROR;
        }
    }
    else
    {
        int iGoldNeed = getRestCost(m_gold_rest + 1);
        //休息有次数限制
        if (m_gold_rest >= iVIPRestTimes[m_vip])
        {
            return HC_ERROR;
        }
        if (iGoldNeed <= 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        if (addGold(-iGoldNeed) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(m_id, m_ip_address, iGoldNeed, gold_cost_for_rest, m_union_id, m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(this,iGoldNeed,gold_cost_for_rest);
#endif
        act_to_tencent(this,act_new_ling_buy,iGoldNeed);
        ++m_gold_rest;
        saveCharDailyVar();
        //取消vip增加休息军令
        //if (m_vip >= 0 && m_vip <= 10)
        //{
        //    addLing(iRestLing[m_vip]);
        //    //军令统计
        //    add_statistics_of_ling_cost(m_id,m_ip_address,iRestLing[m_vip],ling_rest_by_gold,1);
        //}
        //else
        {
            addLing(iRestLing);
            //军令统计
            add_statistics_of_ling_cost(m_id,m_ip_address,iRestLing,ling_rest_by_gold,1, m_union_id, m_server_id);
        }
        //日常任务
        dailyTaskMgr::getInstance()->updateDailyTask(*this,daily_task_rest);
    }
    NotifyCharData();
    return HC_SUCCESS;
}

//查询休息信息
int CharData::queryRestInfo(json_spirit::Object& robj)
{
    json_spirit::Object info;
    time_t t_now = time(NULL);
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&t_now, t);
    std::string nextTime = "12:00";
    int leftTimes = 0;
    info.push_back( Pair("canGet", 1 - m_welfare) );
    int recover_ling2 = 12;
#ifdef VN_SERVER
    //11:00 15:00 19:00 21:00 24:00 刷新领取按钮.
    //注册前三天
    if (regDays() <= 3)
    {
        nextTime = "11:00";
        recover_ling2 = 20;
        if (t->tm_hour < 11)
        {
            nextTime = "11:00";
            leftTimes = 5;
        }
        else if (t->tm_hour < 12)
        {
            nextTime = "12:00";
            leftTimes = 4;
        }
        else if (t->tm_hour < 15)
        {
            nextTime = "15:00";
            leftTimes = 3;
        }
        else if (t->tm_hour < 19)
        {
            nextTime = "19:00";
            leftTimes = 2;
        }
        else if (t->tm_hour < 21)
        {
            nextTime = "21:00";
            leftTimes = 1;
        }
    }
    else
    {
        if (t->tm_hour < 12)
        {
            nextTime = "12:00";
            leftTimes = 2;
        }
        else if (t->tm_hour < 19)
        {
            nextTime = "19:00";
            leftTimes = 1;
        }
    }
    if (0 == m_welfare)
    {
        ++leftTimes;
    }
#else
    {
        if (t->tm_hour < 12)
        {
            nextTime = "12:00";
            leftTimes = 2;
        }
        else if (t->tm_hour < 19)
        {
            nextTime = "19:00";
            leftTimes = 1;
        }
    }
    if (0 == m_welfare)
    {
        ++leftTimes;
    }
    if (leftTimes == 0)
    {
        nextTime = "12:00";
    }
#endif
    //军令获得数值4倍
    if(recover_ling2 > 0)
        recover_ling2 *= 4;
    info.push_back( Pair("recover_ling2", recover_ling2) );
    info.push_back( Pair("leftTimes", leftTimes) );
    info.push_back( Pair("nextTime", nextTime) );
    int iGoldNeed = getRestCost(m_gold_rest + 1);
    info.push_back( Pair("cost_gold", iGoldNeed) );
    info.push_back( Pair("recover_ling", iRestLing) );
    info.push_back( Pair("rest", iVIPRestTimes[m_vip] - m_gold_rest > 0 ? (iVIPRestTimes[m_vip] - m_gold_rest) : 0) );
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

//增加声望
int CharData::addPrestige(int prestige)
{
    int old_prestige = m_prestige;
    m_prestige += prestige;
    if (m_prestige < 0)
    {
        m_prestige = 0;
    }
    else
    {
        m_officalcanlevelup = OfficalLevelUpState();
    }
    if (prestige > 0)
    {
        //周积分
        int score = queryExtraData(char_data_type_week, char_data_extra_prestige_get);
        setExtraData(char_data_type_week, char_data_extra_prestige_get,score + prestige);
    }
    InsertSaveDb("update char_data set prestige='" + LEX_CAST_STR(m_prestige)
            + "',official='" + LEX_CAST_STR(m_offical)
            + "' where cid=" + LEX_CAST_STR(m_id));

    return m_prestige;
}

int CharData::OfficalLevelUp()
{
    boost::shared_ptr<baseoffical> p_next = GeneralDataMgr::getInstance()->GetBaseOffical(m_offical + 1);
    if (p_next.get())
    {
        //提升官职
        if (m_prestige >= p_next->need_prestige)
        {
            m_hasgetsalary = 0;
            m_offical = p_next->m_id;
            m_salary = p_next->m_salary;
            m_offical_name = p_next->m_name;
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "OfficalUpInfo"));
            obj.push_back( Pair("offical", p_next->m_name));
            obj.push_back( Pair("salary", p_next->m_salary));

            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
            if (account.get())
            {
                obj.push_back( Pair("s",200) );
                account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
                account->Send("{\"type\":9,\"active\":1,\"cmd\":\"addAction\",\"s\":200}");
            }
            //通知将星录亮
            if (Singleton<jxl_mgr>::Instance().needNotifyOffical(m_offical))
            {
                notifyEventState(top_level_event_jxl, 1, 0);
            }
        }
    }
    return m_offical;
}

bool CharData::OfficalLevelUpState()
{
    bool canUp = false;
    boost::shared_ptr<baseoffical> p_next = GeneralDataMgr::getInstance()->GetBaseOffical(m_offical + 1);
    if (p_next.get())
    {
        //满足升官官职
        if (m_prestige >= p_next->need_prestige)
        {
            canUp = true;
        }
    }
    return canUp;
}

//能否购买官职武将
bool CharData::canBuyOfficalGeneral(int gid)
{
    return GeneralDataMgr::getInstance()->canBuyOfficalGeneral(gid, m_currentStronghold, m_offical);
}

int CharData::buyOfficalGeneral(int gid)
{
    officalgenerals* og = GeneralDataMgr::getInstance()->getOfficalGeneral(gid);
    if (og)
    {
        //官职不够或者关卡进度不够
        if (og->m_special
            || m_currentStronghold < og->need_slevel
            || m_offical < og->need_offical)
        {
            return HC_ERROR;
        }
        //武将上限判断
        if (m_generals.m_generals.size() >= m_general_limit)
        {
            return HC_ERROR_TOO_MUCH_GENERALS;
        }
        int needSilver = og->m_price;
        if (needSilver > 0)
        {
            if (addSilver(-needSilver) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_SILVER;
            }
            //银币消耗统计
            add_statistics_of_silver_cost(m_id,m_ip_address,needSilver,silver_cost_for_buy_hero, m_union_id, m_server_id);
        }
        m_generals.Add(gid);

        //激活了新的将星录
        int jxl_id = Singleton<jxl_mgr>::Instance().checkActivation(*this, gid);
        if (jxl_id)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "notify") );
            obj.push_back( Pair("s", 200) );
            obj.push_back( Pair("type", notify_msg_jxl) );
            obj.push_back( Pair("id", jxl_id) );
            sendObj(obj);
            //notifyEventState(top_level_event_jxl, 1, 0);
        }
        //act统计
        act_to_tencent(this,act_new_buy_general, gid);
#ifdef QQ_PLAT
        //首次招募武将分享
        Singleton<inviteMgr>::Instance().update_event(m_id, SHARE_EVENT_FIRST_GENERAL, 0);
#endif
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int CharData::buyShopGoods(int pos, json_spirit::Object& o)
{
    return m_shop.buy(pos);
}

int CharData::refreshShopGoods(int type)
{
    return m_shop.refresh(2);
}

//更新任务
int CharData::updateTask(int type, int n1, int n2)
{
    INFO("updateTask()"<<type<<","<<n1<<","<<n2<<endl);
    if (!m_task._task.get() || m_task._task->type == task_empty)
    {
        return HC_SUCCESS;
    }
    bool b_notify = false;
    if (m_task._task->type != type)
    {
        //等级达到一定，自动完成该任务
        if (task_char_level == type && n1 >= m_task._task->done_level)
        {
            m_task.cur = m_task.need;
            m_task.done = true;
        }
        else
        {
            return HC_SUCCESS;
        }
    }
    else
    {
        switch (type)
        {
            case task_attack_stronghold://击败指定关卡
            case task_get_general:        //获得指定武将
            case task_send_general:     //排兵布阵，让指定武将上阵
            case task_enter_map:        //进入指定地图
            case task_refresh_state:    //刷新状态-指定等级
                if (n1 == m_task._task->need[0])
                {
                    m_task.cur = 1;
                    m_task.done = true;
                }
                break;

            case task_char_level:        //主将达到指定等级
            case task_silver_nums:        //银币数量
                m_task.cur = n1;
                b_notify = true;
                if (n1 >= m_task._task->need[0])
                {
                    m_task.done = true;
                }
                break;

            case task_general_level:     //5 武将升级到一定等级
            case task_skill_level:        //技能升级到一定等级
            case task_zhen_level:        //阵型升级到指定等级
            case task_get_treasure:
                if (n1 == m_task._task->need[0])
                {
                    m_task.cur = n2;
                    b_notify = true;
                    if (n2 >= m_task._task->need[1])
                    {
                        m_task.cur = m_task._task->need[1];
                        m_task.done = true;
                    }
                }
                break;
            case task_enter_stage:        //进入指定场景
                if (n1 == m_task._task->need[0])
                {
                    if (n2 == m_task._task->need[1])
                    {
                        m_task.cur = 1;
                        m_task.done = true;
                    }
                }
                break;
            case task_buy_weapon:        //10 购买指定兵器
                if (n1 == m_task._task->need[0])
                {
                    if (n2 >= m_task._task->need[1])
                    {
                        m_task.cur = 1;
                        m_task.done = true;
                    }
                }
                break;

            case task_refresh_weapon:    //刷新铁匠铺
            case task_first_farm:        //首次进行屯田操作
            case task_first_smelt:        //首次进行冶炼
            case task_add_to_favorites://收藏游戏
            case task_do_research:        //研究技能
            case task_choose_camp:        //选择阵营
            case task_do_race:            //竞技战斗
            case task_equipment_make:    //装备制造
                m_task.cur = 1;
                m_task.done = true;
                break;
            case task_do_explore:
                ++m_task.cur;
                b_notify = true;
                if (m_task.cur >= m_task.need)
                {
                    m_task.cur = m_task.need;
                    m_task.done = true;
                }
                break;
            case task_attack_equipment_level:
                if (n1 == m_task._task->need[0]
                    || (n1 >= 11 && n1 <= 15 && (n1-10) == m_task._task->need[0]))
                {
                    if (m_task.cur < n2)
                    {
                        m_task.cur = n2;
                        b_notify = true;
                    }
                    if (m_task.cur >= m_task.need)
                    {
                        m_task.cur = m_task.need;
                        m_task.done = true;
                    }
                }
                break;
            case task_group_general_level:
                {
                    if (n1 >= m_task._task->need[0])
                    {
                        int counts = m_generals.getGeneralCounts(m_task._task->need[0]);
                        m_task.cur = counts;
                        b_notify = true;
                        if (m_task.cur >= m_task.need)
                        {
                            m_task.cur = m_task.need;
                            m_task.done = true;
                        }
                    }
                }
                break;
            case task_equipment_level:    //装备等级
                if (n1 == m_task._task->need[0])
                {
                    m_task.cur = n2;
                    b_notify = true;
                    if (n2 >= m_task._task->need[1])
                    {
                        m_task.cur = m_task._task->need[1];
                        if (m_task.cur == 0)
                        {
                            m_task.cur = 1;
                        }
                        m_task.done = true;
                    }
                }
                break;
            case task_weapon_level:    //兵器等级
                {
                    if (n1 == m_task._task->need[0])
                    {
                        m_task.cur = n2;
                        b_notify = true;
                        if (n2 >= m_task._task->need[1])
                        {
                            m_task.cur = m_task._task->need[1];
                            if (m_task.cur == 0)
                            {
                                m_task.cur = 1;
                            }
                            m_task.done = true;
                        }
                    }
                    else if (n1 > m_task._task->need[0] && (n1 - m_task._task->need[0])%5 == 0)
                    {
                        m_task.cur = m_task._task->need[1];
                        m_task.done = true;
                    }
                }
                break;
            case task_buy_bag:
                {
                    if (n1 >= m_task._task->need[0])
                    {
                        m_task.cur = m_task._task->need[0];
                        m_task.done = true;
                    }
                }
                break;
            case task_open_libao:
                {
                    cout<<"update task libao, n1:"<<n1<<","<<m_task._task->need[0]<<endl;
                    if (n1 == m_task._task->need[0])
                    {
                        m_task.cur = m_task._task->need[0];
                        m_task.done = true;
                    }
                    break;
                }
        }
    }
    if (m_task.done)
    {
        //新手引导
        checkGuide(guide_type_taskdone, m_task.tid, 0);

        InsertSaveDb("replace into char_tasks (cid,tid,state) values ("
                + LEX_CAST_STR(m_id) + ","
                + LEX_CAST_STR(m_task.tid) + ",0)");
    }
    if (m_task.done || b_notify)
    {
        //通知玩家任务完成或者有变化
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
        if (account.get())
        {
            json_spirit::Object robj;
            robj.push_back( Pair("cmd", "getCurTask") );
            robj.push_back( Pair("s", 200) );
            taskMgr::getInstance()->getTaskInfo(*this, 0, robj);
            account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
        }
    }
    return HC_SUCCESS;
}

bool CharData::CheckHasGeneral(int base_id)
{
    return m_generals.GetGeneralByType(base_id) != 0;
}

bool CharData::HasGeneral(int base_id)
{
    if (m_generals.GetGeneralByType(base_id) > 0)
    {
        return true;
    }
    return m_generals.GetFiredGeneralByType(base_id) > 0;
}

bool CharData::CheckHasFireGeneral(int base_id, int& general_id)
{
    bool find = false;
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_fired_generals.begin();
    while (it != m_generals.m_fired_generals.end())
    {
        if (it->second.get() && it->second->m_gid == base_id)
        {
            find = true;
            general_id = it->second->m_id;
        }
        ++it;
    }
    return find;
}

bool CharData::CheckHasEquipt(int base_id)
{
    if (m_bag.CheckHasEquipt(base_id))
    {
        return true;
    }
    else
    {
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.begin();
        while (it != m_generals.m_generals.end())
        {
            if (it->second.get() && it->second->m_equipments.CheckHasEquipt(base_id))
            {
                return true;
            }
            ++it;
        }
    }
    return false;
}

void CharData::init_task_done()
{
    switch (m_task._task->type)
    {
        case task_attack_stronghold://击败指定关卡
        case task_get_general:        //获得指定武将
            m_task.need = 1;
            m_task.cur = 1;
            break;
        case task_char_level:        //主将达到指定等级
            m_task.need = m_task._task->need[0];
            m_task.cur = m_level;
            break;
        case task_do_explore:        //探索
            m_task.need = m_task._task->need[0];
            m_task.cur = 0;
            break;
        case task_silver_nums:        //银币数量
            m_task.need = m_task._task->need[0];
            m_task.cur = silver();
            break;
        case task_general_level:    //5 武将升级到一定等级
        case task_skill_level:        //技能升级到一定等级
        case task_zhen_level:        //阵型升级到指定等级
        case task_attack_equipment_level://攻击装备等级
        case task_get_treasure:     //道具数量
        case task_group_general_level://多个英雄达到指定等级
        case task_weapon_level:
            m_task.need = m_task._task->need[1];
            m_task.cur = m_task.need;
            break;
        case task_enter_stage:        //进入指定场景
        case task_enter_map:        //进入指定地图
        case task_send_general:     //排兵布阵，让指定武将上阵
        case task_buy_weapon:        //10 购买指定兵器
        case task_refresh_weapon:    //刷新铁匠铺
        case task_first_farm:        //首次进行屯田操作
        case task_first_smelt:        //首次进行冶炼
        case task_add_to_favorites://收藏游戏
        case task_do_research:        //技能研究
        case task_choose_camp:        //加入阵营
        case task_do_race:            //竞技战斗
        case task_refresh_state:    //刷新状态
        case task_equipment_make:    //制造装备
        case task_buy_bag:
            m_task.need = 1;
            m_task.cur = m_task.need;
            break;
        case task_equipment_level:    //装备强化等级
            m_task.need = m_task._task->need[1];
            if (m_task.need == 0)
            {
                m_task.need = 1;
            }
            m_task.cur = m_task.need;
            break;
        default:
            m_task.need = 1;
            m_task.cur = m_task.need;
            break;
    }
}

//判断任务是否已经完成
int CharData::checkTask()
{
    //cout<<"checkTask()"<<endl;

    if (!m_task._task.get() || m_task._task->type == task_empty || m_task.done)
    {
        return HC_SUCCESS;
    }

    //达到一定等级任务自动完成
    if (!m_task.done && m_level >= m_task._task->done_level)
    {
        m_task.done = true;
        init_task_done();
        return HC_SUCCESS;
    }

    switch (m_task._task->type)
    {
        case task_attack_stronghold://击败指定关卡
            {
                m_task.need = 1;
                m_task.cur = 0;
                boost::shared_ptr<StrongholdData> shold = GeneralDataMgr::getInstance()->GetStrongholdData(m_task._task->need[0]);
                if (!shold.get())
                {
                    return HC_ERROR;
                }
                boost::shared_ptr<CharStrongholdData> cd = GeneralDataMgr::getInstance()->GetCharStrongholdData(m_id,shold->m_map_id,shold->m_stage_id, shold->m_strongholdpos);
                if (cd.get() && cd->m_state >= 1)
                {
                    m_task.cur = 1;
                    m_task.done = true;
                }
            }
            break;
        case task_char_level:        //主将达到指定等级
            m_task.need = m_task._task->need[0];
            m_task.cur = m_level;
            if (m_level >= m_task._task->need[0])
            {
                m_task.cur = m_task._task->need[0];
                m_task.done = true;
            }
            break;
        case task_get_general:        //获得指定武将
            m_task.need = 1;
            m_task.cur = 0;
            if (m_generals.GetGeneralByType(m_task._task->need[0]) > 0)
            {
                m_task.cur = 1;
                m_task.done = true;
            }
            break;
        case task_general_level:     //5 武将升级到一定等级
            {
                m_task.need = m_task._task->need[1];
                m_task.cur = 0;
                int gid = m_generals.GetGeneralByType(m_task._task->need[0]);
                if (gid > 0)
                {
                    boost::shared_ptr<CharGeneralData> g = m_generals.GetGenral(gid);
                    if (g.get())
                    {
                        m_task.cur = g->m_level;
                        if (g->m_level >= m_task._task->need[1])
                        {
                            m_task.cur = m_task._task->need[1];
                            m_task.done = true;
                        }
                    }
                }
            }
            break;
        case task_skill_level:        //技能升级到一定等级
            {
                m_task.need = m_task._task->need[1];
                m_task.cur = 0;
                std::map<int, boost::shared_ptr<charSkill> >::iterator it = m_skill_list.find(m_task._task->need[0]);
                if (it == m_skill_list.end() || !it->second.get())
                {
                    return HC_SUCCESS;
                }
                charSkill* pSkill = it->second.get();
                m_task.cur = pSkill->level;
                if (pSkill->level >= m_task._task->need[1])
                {
                    m_task.cur = m_task._task->need[1];
                    m_task.done = true;
                }
            }
            break;
        case task_zhen_level:        //阵型升级到指定等级
            {
                m_task.need = m_task._task->need[1];
                m_task.cur = 0;
                boost::shared_ptr<ZhenData> zhen = m_zhens.GetZhen(m_task._task->need[0]);
                if (!zhen.get())
                {
                    return HC_SUCCESS;
                }
                m_task.cur = zhen->m_level;
                if (zhen->m_level >= m_task._task->need[1])
                {
                    m_task.cur = m_task._task->need[1];
                    m_task.done = true;
                }
            }
            break;
        case task_enter_map:        //进入指定地图
            if (m_area >= m_task._task->need[0] || m_current_map == m_task._task->need[0])
            {
                m_task.need = 1;
                m_task.cur = 1;
                m_task.done = true;
            }
            else
            {
                m_task.need = 1;
                m_task.cur = 0;
            }
            break;
        case task_send_general:     //排兵布阵，让指定武将上阵
            {
                bool b_break = false;
                std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.m_zhens.begin();
                while (it != m_zhens.m_zhens.end())
                {
                    if (it->second.get())
                    {
                        for (int i = 0; i < 9; ++i)
                        {
                            if (it->second->m_generals[i] > 0)
                            {
                                boost::shared_ptr<CharGeneralData> g = m_generals.GetGenral(it->second->m_generals[i]);
                                if (g.get() && g->m_gid == m_task._task->need[0])
                                {
                                    b_break = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (b_break)
                    {
                        break;
                    }
                    ++it;
                }
                //已经在阵上了直接完成
                if (b_break)
                {
                    m_task.need = 1;
                    m_task.cur = 1;
                    m_task.done = true;
                }
                else
                {
                    m_task.need = 1;
                    m_task.cur = 0;
                }
                break;
            }
        case task_buy_weapon:        //10 购买指定兵器
            {
                m_task.need = 1;
                m_task.cur = 1;
                m_task.done = true;
            }
            break;

        case task_enter_stage:        //进入指定场景
            if (m_area > m_task._task->need[0]
                || (m_current_map == m_task._task->need[0]
                    && m_current_stage >= m_task._task->need[1]))
            {
                m_task.need = 1;
                m_task.cur = 1;
                m_task.done = true;
            }
            else
            {
                m_task.need = 1;
                m_task.cur = 0;
            }
            break;
        case task_refresh_weapon:    //刷新铁匠铺
        case task_add_to_favorites://收藏游戏
        case task_do_research:        //研究技能
        case task_do_race:            //竞技战斗
        case task_refresh_state:    //刷新状态
            m_task.need = 1;
            m_task.cur = 0;
            break;
        case task_do_explore:        //探索
            m_task.need = m_task._task->need[0];
            m_task.cur = 0;
            break;
        case task_first_farm:        //首次进行屯田操作
            {
                bool isFarming = false;
                boost::shared_ptr<fieldlist> fl = farmMgr::getInstance()->GetCharFieldList(m_id);
                if (fl.get())
                {
                    for (size_t j = 0; j < (*fl).size(); ++j)
                    {
                        if ((*fl)[j]->m_state == 1)
                        {
                            isFarming = true;
                            break;
                        }
                    }
                }
                if (isFarming)
                {
                    m_task.need = 1;
                    m_task.cur = 1;
                    m_task.done = true;
                }
                else
                {
                    m_task.need = 1;
                    m_task.cur = 0;
                }
            }
            break;
        case task_first_smelt:        //首次进行冶炼
            {
#if 0
                boost::shared_ptr<charSmeltData> sp_smelt = SmeltMgr::getInstance()->getCharSmeltData(m_id);
                //有一个冶炼队列正在进行
                if (sp_smelt.get() && sp_smelt->SmeltList[0].state == 2)
                {
                    m_task.need = 1;
                    m_task.cur = 1;
                    m_task.done = true;
                }
                else
                {
                    m_task.need = 1;
                    m_task.cur = 0;
                }
#else
                m_task.need = 1;
                m_task.cur = 1;
                m_task.done = true;
#endif
            }
            break;
        case task_equipment_level:    //装备等级
            {
                int tmpid = 0;
                m_task.need = m_task._task->need[1];
                m_task.cur = m_bag.maxEquipLevel(m_task._task->need[0], tmpid);
                //得到兵器
                if (m_task._task->need[1] == 0)
                {
                    m_task.need = 1;
                    if (m_task.cur >= 0)
                    {
                        m_task.cur = 1;
                    }
                    else
                    {
                        m_task.cur = 0;
                    }
                }
                else
                {
                    if (m_task.cur < 0)
                    {
                        m_task.cur = 0;
                    }
                }
                if (m_task.cur >= m_task.need)
                {
                    m_task.cur = m_task.need;
                    m_task.done = true;
                }
            }
            break;
        case task_silver_nums:    //银币数量
            {
                m_task.need = m_task._task->need[0];
                m_task.cur = m_silver;
                if (m_task.cur >= m_task.need)
                {
                    m_task.cur = m_task.need;
                    m_task.done = true;
                }
            }
            break;
        case task_attack_equipment_level://普攻/策攻装备强化等级
            {
                int tmpid = 0;
                int level1 = m_bag.maxEquipLevel(m_task._task->need[0], tmpid);
                int level2 = m_bag.maxEquipLevel(m_task._task->need[0] + 10, tmpid);
                m_task.cur = max(level1, level2);
                m_task.need = m_task._task->need[1];
                if (m_task.cur >= m_task.need)
                {
                    m_task.cur = m_task.need;
                    m_task.done = true;
                }
            }
            break;
        case task_get_treasure:    //道具数量
            {
                m_task.need = m_task._task->need[1];
                m_task.cur = m_bag.getGemCount(m_task._task->need[0]);
                if (m_task.cur >= m_task.need)
                {
                    m_task.cur = m_task.need;
                    m_task.done = true;
                }
            }
            break;
        case task_group_general_level:    //多个武将达到指定等级
            {
                m_task.need = m_task._task->need[1];
                m_task.cur = m_generals.getGeneralCounts(m_task._task->need[0]);
                if (m_task.cur >= m_task.need)
                {
                    m_task.cur = m_task.need;
                    m_task.done = true;
                }
            }
            break;
        case task_choose_camp:        //选择阵营
            if (m_camp > 0)
            {
                m_task.cur = 1;
                m_task.need = 1;
                m_task.done = true;
            }
            else
            {
                m_task.cur = 0;
                m_task.need = 1;
            }
            break;
        case task_weapon_level:        //兵器升级到指定等级
            {
                baseNewWeapon* pb = newWeaponMgr::getInstance()->getWeapon(m_task._task->need[0]);
                if (!pb || pb->_type > 5 || pb->_type < 1)
                {
                    m_task.cur = m_task._task->need[1];
                    m_task.need = m_task._task->need[1];
                    m_task.done = true;
                }
                else if (!m_new_weapons._weapons[pb->_type-1]._baseWeapon
                          || m_new_weapons._weapons[pb->_type-1]._baseWeapon->_id < pb->_id)
                {
                    m_task.cur = 0;
                    m_task.need = m_task._task->need[1];
                }
                else if (m_new_weapons._weapons[pb->_type-1]._baseWeapon->_id == pb->_id
                            && m_new_weapons._weapons[pb->_type-1]._level < m_task._task->need[1])
                {
                    m_task.cur = m_new_weapons._weapons[pb->_type-1]._level;
                    m_task.need = m_task._task->need[1];
                }
                else
                {
                    m_task.cur = m_task._task->need[1];
                    m_task.need = m_task._task->need[1];
                    m_task.done = true;
                }
                break;
            }
        case task_equipment_make:    //装备制造
            {
                int tmpid = 0;
                m_task.need = 1;
                if (CheckHasEquipt(m_task._task->need[0]))
                {
                    m_task.cur = 1;
                    m_task.done = true;
                }
                else
                {
                    m_task.cur = 0;
                }
            }
            break;
        case task_buy_bag:    //购买背包
            {
                m_task.need = m_task._task->need[0];
                m_task.cur = m_bag.size() - BAG_DEFAULT_SIZE;
                if (m_task.cur >= m_task.need)
                {
                    m_task.cur = m_task.need;
                    m_task.done = true;
                }
            }
            break;
        default:
            {
                m_task.need = 1;
                m_task.cur = 0;
                break;
            }
    }
    return HC_SUCCESS;
}

//设置金币消费有提示
int CharData::enableNoConfirmGoldCost(int type, bool enable)
{
    if (type >= 1 && type <= iMaxGoldCostConfirm)
    {
        if (enable)
        {
            m_gold_cost_comfirm[type-1] = 1;
            InsertSaveDb("replace into char_goldCost_noConfirm (cid,type) values (" + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(type) + ")");
        }
        else
        {
            m_gold_cost_comfirm[type-1] = 0;
            InsertSaveDb("delete from char_goldCost_noConfirm where cid=" + LEX_CAST_STR(m_id) + " and type=" + LEX_CAST_STR(type));
        }
    }
    return HC_SUCCESS;
}

//设置金币消费有提示
bool CharData::getNoConfirmGoldCost(int type)
{
    if (type >= 1 && type <= iMaxGoldCostConfirm)
    {
        return m_gold_cost_comfirm[type-1];
    }
    return false;
}

//更新vip等级
int CharData::updateVip()
{
    int old_vip = m_vip;
    int total = m_total_recharge + m_vip_exp;
    if (total >= iVIP_recharge[11])
    {
        m_vip = 12;
    }
    else if (total >= iVIP_recharge[10])
    {
        m_vip = 11;
    }
    else if (total >= iVIP_recharge[9])
    {
        m_vip = 10;
    }
    else if (total >= iVIP_recharge[8])
    {
        m_vip = 9;
    }
    else if (total >= iVIP_recharge[7])
    {
        m_vip = 8;
    }
    else if (total >= iVIP_recharge[6])
    {
        m_vip = 7;
    }
    else if (total >= iVIP_recharge[5])
    {
        m_vip = 6;
    }
    else if (total >= iVIP_recharge[4])
    {
        m_vip = 5;
    }
    else if (total >= iVIP_recharge[3])
    {
        m_vip = 4;
        //NotifyCharState();
    }
    else if (total >= iVIP_recharge[2])
    {
        m_vip = 3;
        exploreMgr::getInstance()->ExploreRefresh(m_id);
    }
    else if (total >= iVIP_recharge[1])
    {
        m_vip = 2;
        //NotifyCharState();
    }
    else if (total >= iVIP_recharge[0])
    {
        m_vip = 1;
    }
    else
    {
        m_vip = 0;
    }
    //当前在临时VIP期间
    if (m_tmp_vip != 0 && m_tmp_vip_end_time != 0)
    {
        //先把真实VIP存起来
        InsertSaveDb("update char_data set vip='" + LEX_CAST_STR(m_vip)
            + "' where cid=" + LEX_CAST_STR(m_id));

        //更新VIP奖励可领取状态
        std::map<int,CharVIPPresent>::iterator it = m_vip_present.begin();
        while (it != m_vip_present.end())
        {
            if (it->first <= m_vip && it->second.state == 0)
            {
                it->second.state = 1;
            }
            ++it;
        }
        //通知礼包按钮状态
        notifyVipState();
        NotifyCharOpenInfo();
        if (m_tmp_vip > m_vip)
        {
            //临时VIP期间还是享受VIP
            m_vip = m_tmp_vip;
        }
    }
    else
    {
        if (old_vip < m_vip)
        {
            InsertSaveDb("update char_data set vip='" + LEX_CAST_STR(m_vip)
                + "' where cid=" + LEX_CAST_STR(m_id));

            //更新VIP奖励可领取状态
            std::map<int,CharVIPPresent>::iterator it = m_vip_present.begin();
            while (it != m_vip_present.end())
            {
                if (it->first <= m_vip && it->second.state == 0)
                {
                    it->second.state = 1;
                }
                ++it;
            }
            //通知礼包按钮状态
            notifyVipState();
            NotifyCharOpenInfo();

            //VIP等级祝贺
            if (m_vip >= 1 && m_vip <= 4)
            {
                Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_VIP_LEVEL_1, m_vip, 0);
            }
            else if (m_vip <= 8)
            {
                Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_VIP_LEVEL_2, m_vip, 0);
            }
            else
            {
                Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_VIP_LEVEL_3, m_vip, 0);
            }
        }
    }
    if (m_vip >= iEnhanceNoCDVip)
    {
        m_enhance_cd = 0;
        m_can_enhance = true;
    }
    return m_vip;
}

/*单笔充值活动奖励判断
int CharData::updateRechargeReward(int num, time_t t)
{
    int id = 0;
    if (m_createTime + iRechargeFirst - time(NULL) > 0)
    {
        id = GeneralDataMgr::getInstance()->checkRechargePresent(num,1);
    }
    else if (m_createTime + iRechargeFirst + iRechargeSecond - time(NULL) > 0)
    {
        id = GeneralDataMgr::getInstance()->checkRechargePresent(num,2);
    }
    else
        return HC_ERROR;
    if (id != 0)
    {
        ++m_recharge_reward[id];
        InsertSaveDb("replace into char_recharge_event (cid,id,num) values ("
                + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(id) + "," +LEX_CAST_STR(m_recharge_reward[id]) + ")");

        return HC_SUCCESS;
    }
    return HC_ERROR;
}*/

//查询引导完成情况
time_t CharData::getGuideState(int id)
{
    return m_guide_completes[id];
}

//设置引导完成
void CharData::setGuideStateComplete(int id, int next_guide)
{
    m_current_guide = next_guide;
    setExtraData(char_data_type_normal, char_data_current_guide, m_current_guide);
    m_guide_completes[id] = time(NULL);
    InsertSaveDb("replace into char_guide_complete (cid,guide,input) values ("
        + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(id) + ",unix_timestamp())");
#ifdef QQ_PLAT
    if (id > 100 && id < 1000)
        guide_to_tencent(this,id);
#endif
}

//领取地图攻略奖励
int CharData::getMapIntroReward(int mapid, json_spirit::Object& o)
{
    if (-1 == addLing(5))
        return HC_ERROR;
    //军令统计
    add_statistics_of_ling_cost(m_id,m_ip_address,5,ling_map,1, m_union_id, m_server_id);
    m_map_intro_get[mapid] = 1;
    std::string msg = strGetLing;
    str_replace(msg, "$L", LEX_CAST_STR(5), true);
    o.push_back( Pair("msg", msg) );
    InsertSaveDb("replace into char_map_intro_get (cid,mapid,get) values ("
        + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(mapid) + ",1)");
    return HC_SUCCESS;
}

int CharData::getUpdateListCD(json_spirit::Object& robj)
{
    time_t timenow = time(NULL);
    json_spirit::Array list;
    static int typeMap[] = {8,9,5,3,1,10,11,12,13,6};
    for (int j = 0; j < 10; ++j)
    {
        int i = typeMap[j];
        json_spirit::Object updateCD;
        /**类型,1屯田队列,2技能训练,3英雄训练,4冶炼队列,
        5征收信息,6战马,7家丁, 8强化队列, 9秘法队列,
        10小型战役,11贸易 ,12领取玉石,13军团探索**/
        int lefttime = 0, state = 0;
        if (i == 1 && m_currentStronghold >= iFarmOpenStronghold[0])
        {
            lefttime = farmMgr::getInstance()->getCoolTime(m_id, state);
            updateCD.clear();
            int now_seed = queryExtraData(char_data_type_daily, char_data_farm_seed);
            int total_seed = farmMgr::getInstance()->FieldNum(m_id)*2;
            updateCD.push_back( Pair("seed", total_seed - now_seed) );
            updateCD.push_back( Pair("state", state) );
            updateCD.push_back( Pair("leftTime", lefttime) );
            updateCD.push_back( Pair("rewardTimes", farmMgr::getInstance()->getRewardTimes(m_id)) );
            updateCD.push_back( Pair("type", i) );
            list.push_back(updateCD);
        }
        else if (i == 3 && m_trainOpen)
        {
            lefttime = generalTrainCoolTime(state);
            updateCD.clear();
            updateCD.push_back( Pair("state", state) );
            updateCD.push_back( Pair("leftTime", lefttime) );
            updateCD.push_back( Pair("type", i) );
            list.push_back(updateCD);
        }
        else if (i == 5 && m_levyOpen)
        {
            updateCD.clear();
            updateCD.push_back( Pair("state", 3) );
            updateCD.push_back( Pair("cur", m_levy_time) );
            updateCD.push_back( Pair("total", iLevyTimes[m_vip] + iLevyFreeTime) );
            updateCD.push_back( Pair("type", i) );
            list.push_back(updateCD);
        }
        else if (i == 6 && m_horse.horse)
        {
            updateCD.clear();
            updateCD.push_back( Pair("state", 3) );
            updateCD.push_back( Pair("turns", m_horse.horse->turn) );
            updateCD.push_back( Pair("stars", m_horse.horse->star) );
            if (m_silver_train_horse < iHorseTrainTime)
            {
                updateCD.push_back( Pair("cur", m_silver_train_horse) );
                updateCD.push_back( Pair("total", iHorseTrainTime) );
            }
            updateCD.push_back( Pair("type", i) );
            list.push_back(updateCD);
        }
        else if (i == 7 && m_servantOpen)
        {
            updateCD.clear();
            boost::shared_ptr<charServant> p = servantMgr::getInstance()->GetCharServant(m_id);
            if (p.get())
            {
                updateCD.push_back( Pair("state", p->m_type) );
                updateCD.push_back( Pair("type", i) );
                if (p->m_type != 0)
                {
                    updateCD.push_back( Pair("interact", p->m_interact_time) );
                    updateCD.push_back( Pair("total_interact", iServantInteractTime) );
                }
                list.push_back(updateCD);
            }
        }
        else if (i == 8 && m_equiptEnhanceOpen > 0 && m_enhance_state > 0)
        {
            updateCD.clear();
            updateCD.push_back( Pair("type", i) );
            updateCD.push_back( Pair("id", m_enhance_eid) );
            int cd = m_enhance_cd - time(NULL);
            if (cd <= 0)
            {
                m_can_enhance = true;
            }
            if (!m_can_enhance || cd > TIME_MAX_ENHANCE)
            {
                updateCD.push_back( Pair("leftTime", cd) );
                if (isNewPlayer() > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "notify") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair("type", notify_msg_new_player_enhance) );
                    obj.push_back( Pair("nums", isNewPlayer()) );
                    sendObj(obj);
                }
            }
            list.push_back(updateCD);
        }
        else if (i == 9 && m_weaponOpen > 0 && m_upgrade_weapon_state > 0)
        {
            updateCD.clear();
            updateCD.push_back( Pair("type", i) );
            updateCD.push_back( Pair("id", m_upgrade_weapon_type) );
            list.push_back(updateCD);
        }
        else if (i == 10 && isMapPassed(1))
        {
            updateCD.clear();
            updateCD.push_back( Pair("type", i) );
            int total_time = 0;
            int can_time = groupCopyMgr::getInstance()->getAllCopyCanAttackTimes(m_id, total_time);
            updateCD.push_back( Pair("cur", total_time - can_time) );
            updateCD.push_back( Pair("total", total_time) );
            if (can_time > 0)
                list.push_back(updateCD);
        }
        else if (i == 11 && m_tradeOpen)
        {
            updateCD.clear();
            updateCD.push_back( Pair("type", i) );
            int cur = queryExtraData(char_data_type_daily, char_data_trade_time);
            updateCD.push_back( Pair("cur", cur) );
            updateCD.push_back( Pair("total", iTradeEveryday) );
            int cd = Singleton<newTradeMgr>::Instance().getCoolTime(*this);
            if (cd > 0)
            {
                updateCD.push_back( Pair("leftTime", cd) );
            }
            if (cur < iTradeEveryday)
                list.push_back(updateCD);
        }
        else if (i == 12 && m_baoshi_count)
        {
            updateCD.clear();
            updateCD.push_back( Pair("type", i) );
            int cur = queryExtraData(char_data_type_daily, char_data_get_yushi);
            updateCD.push_back( Pair("cur", cur) );
            updateCD.push_back( Pair("total", iFreeYushi) );
            int cd_time = queryExtraData(char_data_type_daily, char_data_yushi_time_cd);
            if (iFreeYushi > cur && cd_time > timenow)
            {
                updateCD.push_back( Pair("leftTime", cd_time - timenow) );
            }
            list.push_back(updateCD);
        }
        else if (i == 13 && m_corps_member.get())
        {
            updateCD.clear();
            updateCD.push_back( Pair("type", i) );
            int cur = queryExtraData(char_data_type_daily, char_data_daily_corps_explore);
            updateCD.push_back( Pair("cur", cur) );
            int corps_explore_times = 0;
            splsCorps* cp = corpsMgr::getInstance()->findCorps(m_corps_member.get()->corps);
            if (cp)
            {
                corps_explore_times = iCorpsExploreTimesOneday[cp->_level];
            }
            updateCD.push_back( Pair("total", corps_explore_times) );
            charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(m_id).get();
            if (c)
            {
                int lefttime = c->getDoneTime();
                if (corps_explore_times > cur && lefttime > 0)
                {
                    updateCD.push_back( Pair("leftTime", lefttime) );
                }
            }
            if (corps_explore_times > cur)
                list.push_back(updateCD);
        }
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//获取功能开放信息
int CharData::getOpeninfo(json_spirit::Object& robj)
{
    json_spirit::Object open;
    open.push_back( Pair("weaponOpen", m_weaponOpen) );
    //open.push_back( Pair("raceOpen", m_raceOpen) );
    open.push_back( Pair("skillOpen", m_skillOpen) );
    open.push_back( Pair("skillTrainOpen", m_skillTrainOpen) );
    open.push_back( Pair("equiptOpen", m_equiptOpen) );
    open.push_back( Pair("equiptEnhanceOpen", m_equiptEnhanceOpen) );
    open.push_back( Pair("zhenOpen", m_zhenOpen) );
    open.push_back( Pair("trainOpen", m_trainOpen) );
    open.push_back( Pair("farmOpen", m_farmOpen) );
    open.push_back( Pair("exploreOpen", m_exploreOpen) );
    open.push_back( Pair("shopOpen", m_shopOpen) );
    open.push_back( Pair("tradeOpen", m_tradeOpen) );
    open.push_back( Pair("washOpen", m_washOpen) );
    open.push_back( Pair("recruitOpen", m_recruitOpen) );
    open.push_back( Pair("sweepOpen", m_sweepOpen) );
    //int m_partyOpen = (m_corps_member.get() && corpsMgr::getInstance()->getCorpsLevel(m_corps_member->corps) >= iCorpsLevelForParty) ? 1 : 0;
    //open.push_back( Pair("partyOpen", m_partyOpen) );
    open.push_back( Pair("horseOpen", m_horseOpen) );
    //open.push_back( Pair("servantOpen", m_servantOpen) );
    open.push_back( Pair("treasureOpen", m_baowuOpen) );
    open.push_back( Pair("buyLingOpen", m_buyLingOpen) );
    open.push_back( Pair("rankEventOpen", m_rankEventOpen) );
    open.push_back( Pair("bankOpen", m_bankOpen) );
    open.push_back( Pair("soulOpen", m_soulOpen) );
    open.push_back( Pair("sevenOpen", m_sevenOpen) );
    open.push_back( Pair("gsoulOpen", m_generalSoulOpen) );
    robj.push_back( Pair("openVO", open) );
    return HC_SUCCESS;
}

int CharData::WashInfo(int gid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharGeneralData> pgd = m_generals.GetGenral(gid);
    if (!pgd.get())
    {
        return HC_ERROR;
    }
    json_spirit::Object info;
    //int32_t count = m_bag.getGemCount(treasure_type_xisui_book);
    //info.push_back( Pair("nums", count) );
    //普通洗髓银币
    info.push_back( Pair("Price1", g_wash_real_cost[0] * m_area) );
    info.push_back( Pair("Vip1", iWashConfig[0][0] * m_area) );
    //其他洗髓金币
    for (int i = 1; i < 5; ++i)
    {
        info.push_back( Pair("Price"+LEX_CAST_STR(i+1), g_wash_real_cost[i]) );
        info.push_back( Pair("Vip"+LEX_CAST_STR(i+1), iWashConfig[i][0]) );
    }
    info.push_back( Pair("discount", g_wash_discount) );

    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

int CharData::Wash(int gid, int type, json_spirit::Object& robj)
{
    boost::shared_ptr<CharGeneralData> pgd = m_generals.GetGenral(gid);
    if (!pgd.get())
    {
        return HC_ERROR;
    }
    bool add_full = false;
    int full_val = m_level * 10 + 50;
    if (m_vip >= iWashFullValVIP)
    {
        full_val *= 2;
    }
    double per = queryExtraData(char_data_type_normal, char_data_wash_per) / 10000.0;
    json_spirit::Object info;
    info.push_back( Pair("per", per*100.0) );
    if (type == 1)//cost silver
    {
        INFO("********wash by silver" << endl);
        if (addSilver(-(g_wash_real_cost[0]*m_area)) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_SILVER;
        }
        //银币消耗统计
        add_statistics_of_silver_cost(m_id,m_ip_address,g_wash_real_cost[0]*m_area,silver_cost_for_wash, m_union_id, m_server_id);
        NotifyCharData();
        //前两次每次加5
        if (pgd->m_wash_times < 2)
        {
            pgd->m_tmp_wash_result[0] = pgd->m_wash_str + 5;
            pgd->m_tmp_wash_result[1] = pgd->m_wash_int + 5;
            pgd->m_tmp_wash_result[2] = pgd->m_wash_tong + 5;
        }
        else
        {
            pgd->m_tmp_wash_result[0] = WashCal(pgd->m_wash_str, iWashConfig[type-1][2], iWashConfig[type-1][3], 10, full_val, per);
            pgd->m_tmp_wash_result[1] = WashCal(pgd->m_wash_int, iWashConfig[type-1][2], iWashConfig[type-1][3], 10, full_val, per);
            pgd->m_tmp_wash_result[2] = WashCal(pgd->m_wash_tong, iWashConfig[type-1][2], iWashConfig[type-1][3], 10, full_val, per);
        }
        ++pgd->m_wash_times;
        int tmp_per = (int)(per * 10000);
        if ((tmp_per + 10) < 200)
        {
            tmp_per += 10;
        }
        else if(per < 200)
        {
            tmp_per = 200;
        }
        per = tmp_per / 10000.0;
        setExtraData(char_data_type_normal, char_data_wash_per, tmp_per);
    }
    else if (type == 2 || type == 3 || type == 4)//cost gold
    {
        if (m_vip < iWashConfig[type-1][0])
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }

        if (addGold(-g_wash_real_cost[type-1]) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        {
            switch (type)
            {
                case 2:
                    add_statistics_of_gold_cost(m_id, m_ip_address, g_wash_real_cost[type-1], gold_cost_for_wash_type2, m_union_id, m_server_id);
#ifdef QQ_PLAT
                    gold_cost_tencent(this,g_wash_real_cost[type-1],gold_cost_for_wash_type2);
#endif
                    break;
                case 3:
                    add_statistics_of_gold_cost(m_id, m_ip_address, g_wash_real_cost[type-1], gold_cost_for_wash_type3, m_union_id, m_server_id);
#ifdef QQ_PLAT
                    gold_cost_tencent(this,g_wash_real_cost[type-1],gold_cost_for_wash_type3);
#endif
                    break;
                case 4:
                    add_statistics_of_gold_cost(m_id, m_ip_address, g_wash_real_cost[type-1], gold_cost_for_wash_type4, m_union_id, m_server_id);
#ifdef QQ_PLAT
                    gold_cost_tencent(this,g_wash_real_cost[type-1],gold_cost_for_wash_type4);
#endif
                    break;
            }
        }
        NotifyCharData();
        pgd->m_tmp_wash_result[0] = WashCal(pgd->m_wash_str, iWashConfig[type-1][2], iWashConfig[type-1][3], 10, full_val, per);
        pgd->m_tmp_wash_result[1] = WashCal(pgd->m_wash_int, iWashConfig[type-1][2], iWashConfig[type-1][3], 10, full_val, per);
        pgd->m_tmp_wash_result[2] = WashCal(pgd->m_wash_tong, iWashConfig[type-1][2], iWashConfig[type-1][3], 10, full_val, per);

        ++pgd->m_wash_times;
        int tmp_per = (int)(per * 10000);
        if (type == 2)
        {
            if((tmp_per + 100) < 1000)
            {
                tmp_per += 100;
            }
            else if(tmp_per < 1000)
            {
                tmp_per = 1000;
            }
        }
        else if(type == 3)
        {
            if((tmp_per + 500) < 3000)
            {
                tmp_per += 500;
            }
            else if(tmp_per < 3000)
            {
                tmp_per = 3000;
            }
        }
        else if(type == 4)
        {
            if((tmp_per + 1000) < 5000)
            {
                tmp_per += 1000;
            }
            else if(tmp_per < 5000)
            {
                tmp_per = 5000;
            }
        }
        per = tmp_per / 10000.0;
        setExtraData(char_data_type_normal, char_data_wash_per, tmp_per);
    }
    else if (type == 5)//至尊规则特殊
    {
        if (m_vip < iWashConfig[type-1][0])
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        if (addGold(-g_wash_real_cost[type-1]) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(m_id, m_ip_address, g_wash_real_cost[type-1], gold_cost_for_wash_type5, m_union_id, m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(this,g_wash_real_cost[type-1],gold_cost_for_wash_type5);
#endif
        NotifyCharData();
        ++pgd->m_wash_times;
        pgd->m_tmp_wash_result[0] = pgd->m_wash_str + 10;
        pgd->m_tmp_wash_result[1] = pgd->m_wash_int + 10;
        pgd->m_tmp_wash_result[2] = pgd->m_wash_tong + 10;
        #if 0
        //低于保底必加10
        if (pgd->m_wash_str < iWashConfig[type-1][2])
        {
            pgd->m_tmp_wash_result[0] = pgd->m_wash_str + 10;
        }
        else//高于保底，50%几率提升
        {
            if (my_random(0,100) < 50)
            {
                pgd->m_tmp_wash_result[0] = pgd->m_wash_str;
            }
            else
            {
                //pgd->m_tmp_wash_result[0] = WashCal(pgd->m_wash_str, iWashConfig[type-1][2], pgd->m_wash_str, 10, full_val);
                pgd->m_tmp_wash_result[0] = pgd->m_wash_str + 10;
            }
        }
        if (pgd->m_wash_int < iWashConfig[type-1][2])
        {
            pgd->m_tmp_wash_result[1] = pgd->m_wash_int + 10;
        }
        else
        {
            if (my_random(0,100) < 50)
            {
                pgd->m_tmp_wash_result[1] = pgd->m_wash_int;
            }
            else
            {
                //pgd->m_tmp_wash_result[1] = WashCal(pgd->m_wash_int, iWashConfig[type-1][2], pgd->m_wash_int, 10, full_val);
                pgd->m_tmp_wash_result[1] = pgd->m_wash_int + 10;
            }
        }
        if (pgd->m_wash_tong < iWashConfig[type-1][2])
        {
            pgd->m_tmp_wash_result[2] = pgd->m_wash_tong + 10;
        }
        else
        {
            if (my_random(0,100) < 50)
            {
                pgd->m_tmp_wash_result[2] = pgd->m_wash_tong;
            }
            else
            {
                //pgd->m_tmp_wash_result[2] = WashCal(pgd->m_wash_tong, iWashConfig[type-1][2], pgd->m_wash_tong, 10, full_val);
                pgd->m_tmp_wash_result[2] = pgd->m_wash_tong + 10;
            }
        }
        #endif
    }
    else
    {
        //wrong type
        return HC_ERROR;
    }
    //超过上限修正，下降超过20点修正
    if (pgd->m_tmp_wash_result[0] > full_val)
    {
        pgd->m_tmp_wash_result[0] = full_val;
    }
    else if ((pgd->m_tmp_wash_result[0] + 20) < pgd->m_wash_str)
    {
        pgd->m_tmp_wash_result[0] = pgd->m_wash_str - (pgd->m_wash_str - pgd->m_tmp_wash_result[0])%10 - 10;
    }
    if (pgd->m_tmp_wash_result[1] > full_val)
    {
        pgd->m_tmp_wash_result[1] = full_val;
    }
    else if ((pgd->m_tmp_wash_result[1] + 20) < pgd->m_wash_int)
    {
        pgd->m_tmp_wash_result[1] = pgd->m_wash_int - (pgd->m_wash_int - pgd->m_tmp_wash_result[1])%10 - 10;
    }
    if (pgd->m_tmp_wash_result[2] > full_val)
    {
        pgd->m_tmp_wash_result[2] = full_val;
    }
    else if ((pgd->m_tmp_wash_result[2] + 20) < pgd->m_wash_tong)
    {
        pgd->m_tmp_wash_result[2] = pgd->m_wash_tong - (pgd->m_wash_tong - pgd->m_tmp_wash_result[2])%10 - 10;
    }
    info.push_back( Pair("max", full_val) );
    info.push_back( Pair("oldbrave", pgd->m_wash_str) );
    info.push_back( Pair("oldwisdom", pgd->m_wash_int) );
    info.push_back( Pair("oldgovern", pgd->m_wash_tong) );
    info.push_back( Pair("newbrave", pgd->m_tmp_wash_result[0]) );
    info.push_back( Pair("newwisdom", pgd->m_tmp_wash_result[1]) );
    info.push_back( Pair("newgovern", pgd->m_tmp_wash_result[2]) );
    info.push_back( Pair("new_per", per*100.0) );
    robj.push_back( Pair("info", info) );
    //日常任务
    dailyTaskMgr::getInstance()->updateDailyTask(*this,daily_task_wash);

    switch (type)
    {
        case 1:
            //支线任务
            m_trunk_tasks.updateTask(task_normal_wash, 1);
            break;
        case 2:
            //支线任务
            m_trunk_tasks.updateTask(task_2gold_wash, 1);
            break;
        default:
            break;
    }
    //act统计
    act_to_tencent(this,act_new_wash);
    return HC_SUCCESS;
}

int CharData::WashConfirm(int gid)
{
    boost::shared_ptr<CharGeneralData> pgd = m_generals.GetGenral(gid);
    if (!pgd.get())
    {
        return HC_ERROR;
    }
    if (pgd->m_tmp_wash_result[0] == 0 ||
        pgd->m_tmp_wash_result[1] == 0 ||
        pgd->m_tmp_wash_result[2] == 0)
    {
        return HC_ERROR;
    }
    else
    {
        int total_wash_old = pgd->m_wash_str + pgd->m_wash_int + pgd->m_wash_tong;
        //周排行活动
        int score = pgd->m_tmp_wash_result[0] + pgd->m_tmp_wash_result[1] + pgd->m_tmp_wash_result[2] - pgd->m_wash_str - pgd->m_wash_int - pgd->m_wash_tong;
        if (score > 0)
            newRankings::getInstance()->updateEventRankings(m_id,rankings_event_wash,score);
        pgd->m_wash_str = pgd->m_tmp_wash_result[0];
        pgd->m_wash_int = pgd->m_tmp_wash_result[1];
        pgd->m_wash_tong = pgd->m_tmp_wash_result[2];
        //星级变化
        pgd->updateWashStar(true);
        setExtraData(char_data_type_normal, char_data_wash_per, 0);
        //攻擊力變化
        set_attack_change();
        //新战力
        pgd->wash_change = true;

        //洗髓祝贺
        int total_wash = pgd->m_wash_str + pgd->m_wash_int + pgd->m_wash_tong;
        if (total_wash >= 100 && (total_wash/100) != (total_wash_old/100))
        {
            if (total_wash < 1000)
            {
                Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_WASH_1, gid, total_wash);
            }
            else if (total_wash < 2000)
            {
                Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_WASH_2, gid, total_wash);
            }
            else
            {
                Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_WASH_3, gid, total_wash);
            }
        }
        //int ret = pgd->SetColor();
        //pgd->broadWashMsg(ret);

        //更新屬性
        pgd->updateAttribute();
        //act统计
        act_to_tencent(this,act_new_wash_confirm);
        return HC_SUCCESS;
    }
}

//清除超过回收时间的武将和装备
int CharData::ClearData()
{
    return HC_SUCCESS;
}

//某张地图是否已经通关
bool CharData::isMapPassed(int mapid)
{
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = m_tempo.CharMapsData.find(mapid);
    if (it != m_tempo.CharMapsData.end())
    {
        boost::shared_ptr<CharMapData> md = it->second;
        CharMapData::iterator itm = (*md).begin();
        while (itm != (*md).end())
        {
            if (itm->second.get() && itm->second->m_baseStage.get())
            {
                if (itm->second->m_stronghold[0].get() && itm->second->m_stronghold[itm->second->m_baseStage->size - 1].get())
                {
                    if (itm->second->m_stronghold[itm->second->m_baseStage->size - 1]->m_state > 0)
                    {
                        ;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            ++itm;
        }

        return true;
    }
    else
    {
        return false;
    }
}

//考虑沉迷因素后的获得
int CharData::chenmiGet(int input)
{
    /*    角色进行游戏时间3小时后，弹窗提示收益减半
        角色进行游戏时间5小时后，弹窗提示收益为0 */
    if (GeneralDataMgr::getInstance()->isChenmiEnable() && m_check_chenmi)
    {
        uint64_t total_time = m_chenmi_time + time(NULL) - m_chenmi_start_time;
#ifdef TEST_SERVER
        if (total_time >= 480)
        {
            return 0;
        }
        if (total_time >= 320)
        {
            return (input-1)/2 + 1;
        }
#else
        if (total_time >= 18000)
        {
            return 0;
        }
        if (total_time >= 10800)
        {
            return (input-1)/2 + 1;
        }
#endif
        return input;
    }
    else
    {
        return input;
    }
}

//获得征收金币消耗
int CharData::getLevyCost(int times)
{
#ifdef JP_SERVER
    if (times < 1)
        return 0;
    if (times == 1)
    {
        return 3;
    }
    else if (times <= 10)
    {
        return 5;
    }
    else if(times <= 30)
    {
        return 10;
    }
    else if(times <= 50)
    {
        return 20;
    }
    else if(times <= 100)
    {
        return 40;
    }
    else if(times <= 300)
    {
        return 80;
    }
    else
    {
        return 200;
    }
#else
    if (times < 1)
        return 0;
    if (times <= 10)
    {
        return times * 2;
    }
    else if(times <= 30)
    {
        return 20;
    }
    else if(times <= 50)
    {
        return 40;
    }
    else if(times <= 100)
    {
        return 80;
    }
    else if(times <= 300)
    {
        return 200;
    }
    else
    {
        return 400;
    }
#endif
    return 0;
}

//获得征收银币单次奖励
int CharData::getLevyReward()
{
    // wlj 修改 20130614,征收修改
    if (m_level <= 40)
    {
        return 1550*m_level + 10000;
    }
    else if(m_level <= 60)
    {
        return 2150 * m_level;
    }
    else
    {
        return (int)(129000.0*(1.0+0.05*((double)m_level-60.0)));
    }
    //return (int)(1000.0 * (double)m_level * (1.0 + ((double)m_area - 1.0) / 4.0));
}

//设置兵器等级
int CharData::setWeaponLevel(int type, int quality, int level)
{
    if (type > 5 || type < 1
        || quality > 5 || quality < 1)
    {
        return HC_ERROR;
    }
    if (level < 1)
    {
        level = 1;
    }
    if (level == m_new_weapons._weapons[type-1]._level)
    {
        return HC_SUCCESS;
    }
    if (NULL == m_new_weapons._weapons[type-1]._baseWeapon)
    {
        return HC_ERROR;
    }
    baseNewWeapon* pb = newWeaponMgr::getInstance()->getWeapon(type + (quality-1) * 5);
    if (pb)
    {
        if (level > pb->_maxLevel)
        {
            level = pb->_maxLevel;
        }
        m_new_weapons._weapons[type-1]._baseWeapon = pb;
        m_new_weapons._weapons[type-1]._level = level;
        m_new_weapons._weapons[type-1]._effect = pb->effect(level);
        m_new_weapons._weapons[type-1]._cost = pb->levelCost(level);
        SaveWeapons(type);
        if (m_area == 2 && pb->_mapid == 1 && level == 20)
        {
            //进入第2图群魔乱舞，且兵器等级达到20级->引导15
            _checkGuide(15);
        }
        //更新任务
        updateTask(task_weapon_level, pb->_id, level);
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

void CharData::set_attack_change(bool e)    //設置變化
{
    m_zhens.set_attack_change();
    if (e)
    {
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.begin();
        while (it != m_generals.m_generals.end())
        {
            if (it->second.get())
            {
                it->second->general_change = true;
                //it->second->equip_change = true;
                it->second->updateEquipmentEffect();
                it->second->wash_change = true;
                it->second->baoshi_change = true;
                it->second->reborn_change = true;
            }
            ++it;
        }
    }
    NotifyCharData();
}

int CharData::get_skill_attack()    //技能對戰力加成
{
    /*
    状态、兵种克制
    */
    int more_damage = 0;
    for (int i = 1; i <= 5; ++i)
    {
        more_damage += m_combat_attribute.more_damage_to(i);
    }
    if (more_damage > 0)
    {
        return more_damage / 5;
    }
    return 0;
}

int CharData::buff_attack(int damage_type, int attack, int hp, int wufang, int cefang)
{
    //buff加成
    int hp_buff = 0, attack_buff = 0, wu_fang_buff = 0, ce_fang_buff = 0;
    for(int i = 0; i < 5; ++i)
    {
        switch(i+1)
        {
            case 1:
                hp_buff = hp * (m_Buffs.buffs[i].m_value) / 100;
                break;
            case 2:
                if (act_wuli_attack == damage_type)
                {
                    attack_buff = attack * (m_Buffs.buffs[i].m_value) / 100;
                }
                break;
            case 3:
                wu_fang_buff = wufang * (m_Buffs.buffs[i].m_value) / 100;
                break;
            case 4:
                if (act_wuli_attack != damage_type)
                {
                    attack_buff = attack * (m_Buffs.buffs[i].m_value) / 100;
                }
                break;
            case 5:
                ce_fang_buff = cefang * (m_Buffs.buffs[i].m_value) / 100;
                break;
            default:
                break;
        }
    }
    //皇座称号加成
    int hp_throne = 0, attack_throne = 0, wu_fang_throne = 0, ce_fang_throne = 0;
    int throne_per = 0;
    if (m_nick.check_nick(nick_throne_start))
    {
        throne_per = 8;
    }
    else if(m_nick.check_nick(nick_throne_start + 1))
    {
        throne_per = 5;
    }
    else if(m_nick.check_nick(nick_throne_start + 2))
    {
        throne_per = 3;
    }
    hp_throne = hp * throne_per / 100;
    attack_throne = attack * throne_per / 100;
    wu_fang_throne = wufang * throne_per / 100;
    ce_fang_throne = cefang * throne_per / 100;

    hp += (hp_buff + hp_throne);
    attack += (attack_buff + attack_throne);
    wufang += (wu_fang_buff + wu_fang_throne);
    cefang += (ce_fang_buff + ce_fang_throne);
    return (attack * 2 + wufang + cefang + hp);
}

int CharData::getAttack(int zid)        //攻擊力查詢
{
    if (m_weapon_attack_change)
    {
        updateAttackDefense();
    }
    boost::shared_ptr<ZhenData> zdata = m_zhens.GetZhen(zid == 0 ? m_zhens.GetDefault() : zid);
    if (zdata.get())
    {
        return zdata->getAttack();
    }
    else
    {
        return 0;
    }
}

//查詢可以鑲嵌寶石的英雄列表
int CharData::queryBaoshiGeneralList(json_spirit::Array& glist)
{
    std::list<baoshi_general> bglist;
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.begin();
    while (it != m_generals.m_generals.end())
    {
        if (!it->second.get())
        {
            ++it;
            continue;
        }
        boost::shared_ptr<CharGeneralData> gd = it->second;
        //if (gd.get() && gd->m_baowu_level && gd->m_baowu_type)
        if (gd.get()/* && gd->m_level >= 30*/)
        {
            boost::shared_ptr<GeneralTypeData> base_gd = GeneralDataMgr::getInstance()->GetBaseGeneral(gd->m_gid);
            if (base_gd.get())
            {
                baoshi_general bg;
                bg.id = gd->m_id;
                bg.level = gd->m_level;
                bg.color = gd->m_color;
                bg.name = base_gd->m_name;
                bg.spic = base_gd->m_spic;
                bg.baoshi_num = gd->m_baoshis.getUsed();
                //gd->getBaoshiCount(bg.baoshi_num, bg.baoshi_hole);

                bool inserted = false;
                for (std::list<baoshi_general>::iterator it = bglist.begin(); it != bglist.end(); ++it)
                {
                    if (it->baoshi_num <= bg.baoshi_num)
                    {
                        inserted = true;
                        bglist.insert(it, bg);
                        break;
                    }
                }
                if (!inserted)
                {
                    bglist.push_back(bg);
                }
            }
        }
        ++it;
    }
    //按照镶嵌宝石的数量排序
    for (std::list<baoshi_general>::iterator it = bglist.begin(); it != bglist.end(); ++it)
    {
        json_spirit::Object generalobj;
        generalobj.push_back( Pair("id", it->id));
        generalobj.push_back( Pair("level", it->level));
        generalobj.push_back( Pair("color", it->color));
        generalobj.push_back( Pair("name", it->name));
        generalobj.push_back( Pair("spic", it->spic));
        generalobj.push_back( Pair("baoshi", it->baoshi_num));
        //generalobj.push_back( Pair("hole", it->baoshi_hole));
        glist.push_back(generalobj);
    }
    return HC_SUCCESS;
}

//查詢英雄寶石
int CharData::queryGeneralBaoshi(int gid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharGeneralData> gdata;
    if (gid == 0)
    {
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.begin();
        while (it != m_generals.m_generals.end())
        {
            if (!it->second.get())
            {
                ++it;
                continue;
            }
            boost::shared_ptr<CharGeneralData> gd = it->second;
            if (gd.get()/* && gd->m_level >= 30*/)
            {
                gdata = gd;
                break;
            }
            else
            {
                ++it;
            }
        }
    }
    else
    {
        gdata = m_generals.GetGenral(gid);
    }
    if (gdata.get())
    {
        CharGeneralData* pg = gdata.get();
        if (pg->m_baseGeneral.get())
        {
            robj.push_back( Pair("name", pg->m_baseGeneral->m_name));
            robj.push_back( Pair("spic", pg->m_baseGeneral->m_spic));
            robj.push_back( Pair("level", pg->m_level));
            robj.push_back( Pair("bName", pg->m_baowu));
            robj.push_back( Pair("bLevel", pg->m_baowu_level));
            robj.push_back( Pair("bSpic", pg->m_baseGeneral->m_baowu_spic));

            robj.push_back( Pair("tjList", pg->m_baseGeneral->m_tj_baoshi_list));
            pg->m_baoshis.showGeneralBaoshis(robj);
            robj.push_back( Pair("baoshi_power", pg->m_baoshi_power));
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//查詢英雄寶石总屬性
int CharData::queryGeneralBaoshiInfo(int gid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharGeneralData> gdata;
    if (gid == 0)
    {
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.begin();
        while (it != m_generals.m_generals.end())
        {
            if (!it->second.get())
            {
                ++it;
                continue;
            }
            boost::shared_ptr<CharGeneralData> gd = it->second;
            if (gd.get()/* && gd->m_level >= 30*/)
            {
                gdata = gd;
                break;
            }
            else
            {
                ++it;
            }
        }
    }
    else
    {
        gdata = m_generals.GetGenral(gid);
    }
    if (gdata.get())
    {
        CharGeneralData* pg = gdata.get();
        if (pg->m_baseGeneral.get()/* && pg->m_level >= 30*/)
        {
            json_spirit::Array alist;    //屬性列表
            pg->m_baoshis.baoshiAttrList(alist);
            robj.push_back( Pair("list", alist) );
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//移除寶石
int CharData::removeBaoshi(int gid, int slot)
{
    //cout<<"removeBaoshi ..."<<endl;
    boost::shared_ptr<CharGeneralData> gdata = m_generals.GetGenral(gid);
    if (gdata.get())
    {
        boost::shared_ptr<iItem> itm = gdata->m_baoshis.getItem(slot);
        if (itm.get())
        {
            newBaoshi* pb = dynamic_cast<newBaoshi*>(itm.get());
            newBaoshi* pm = m_bag.getBaoshiCanMerge(pb->m_base.type, pb->level(), 1);
            if (pm)
            {
                //cout<<"removeBaoshi,getBaoshiCanmerge..."<<endl;
                itm = gdata->m_baoshis.removeItem(slot,false);
                if (itm.get())
                {
                    gdata->updateBaoshiAttr();
                    //cout<<"removeBaoshi,getBaoshiCanmerge.add(1)"<<endl;
                    pm->addCount(1);
                    pm->Save();
                    //删除原来的宝石
                    itm->Clear();
                    itm->Save();
                    //通知战斗属性变化
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "baoshi_change") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair("name", pm->m_base.baseAttr.name) );
                    obj.push_back( Pair("add_pre", pm->value()) );
                    obj.push_back( Pair("add_now", 0) );
                    sendObj(obj);
                }
            }
            else if (m_bag.isFull())
            {
                return HC_ERROR_BAG_FULL;
            }
            else
            {
                itm = gdata->m_baoshis.removeItem(slot,false);
                //cout<<"removeBaoshi,no getBaoshiCanmerge"<<endl;
                if (itm.get())
                {
                    gdata->updateBaoshiAttr();
                    //cout<<"removeBaoshi,bag add baoshi"<<endl;
                    m_bag.addItem(itm);
                    itm->Save();
                    //通知战斗属性变化
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "baoshi_change") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair("name", pb->m_base.baseAttr.name) );
                    obj.push_back( Pair("add_pre", pb->value()) );
                    obj.push_back( Pair("add_now", 0) );
                    sendObj(obj);
                }
            }
            set_attack_change();
            NotifyZhenData();
        }
    }
    return HC_SUCCESS;
}

//鑲嵌寶石
int CharData::xiangruBaoshi(int bagSlot, int gid, int slot)
{
    boost::shared_ptr<iItem> itm = m_bag.getItem(bagSlot);
    if (!itm.get() || itm->getType() != iItem_type_baoshi)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharGeneralData> gdata = m_generals.GetGenral(gid);
    if (gdata.get() && gdata->m_baseSoldier.get())
    {
        if (!gdata->m_baoshis.canXiangqian(slot, itm->getSubtype()))
        {
            return HC_ERROR_XIANGQIAN_SAME_TYPE;
        }
        //判断攻击类型和宝石类型是否无效
        if (gdata->m_baseSoldier->m_damage_type == wuli_damage)
        {
            if (itm->getSubtype() == 8 || itm->getSubtype() == 11)
            {
                return HC_ERROR_XIANGQIAN_WULI_ERR;
            }
        }
        else if(gdata->m_baseSoldier->m_damage_type == celue_damage)
        {
            if (itm->getSubtype() == 6 || itm->getSubtype() == 10)
            {
                return HC_ERROR_XIANGQIAN_CELUE_ERR;
            }
        }
        if (slot == 0)
        {
            if (gdata->m_baoshis.isFull())
            {
                return HC_ERROR_XIANGQIAN_FULL;
            }
            else
            {
                newBaoshi* pb = dynamic_cast<newBaoshi*>(itm.get());
                //通知战斗属性变化
                json_spirit::Object obj;
                obj.push_back( Pair("cmd", "baoshi_change") );
                obj.push_back( Pair("s", 200) );
                obj.push_back( Pair("name", pb->m_base.baseAttr.name) );
                obj.push_back( Pair("add_pre", 0) );
                obj.push_back( Pair("add_now", pb->value()) );
                sendObj(obj);
                //act统计
                act_to_tencent(this,act_new_baoshi_xiangqian, pb->m_base.type, pb->level());
                if (pb->getCount() == 1)
                {
                    m_bag.removeItem(bagSlot);
                    gdata->m_baoshis.addItem(itm,false);
                    itm->setChanged();
                    itm->Save();
                    gdata->updateBaoshiAttr();
                    set_attack_change();
                    NotifyZhenData();
                    return HC_SUCCESS;
                }
                else
                {
                    pb->addCount(-1);
                    boost::shared_ptr<iItem> bs = Singleton<newBaoshiMgr>::Instance().cloneBaoshi(pb->m_base.type, pb->level(), 1);
                    gdata->m_baoshis.addItem(bs,false);
                    bs->setChanged();
                    itm->setChanged();
                    itm->Save();
                    bs->Save();
                    gdata->updateBaoshiAttr();
                    set_attack_change();
                    NotifyZhenData();
                    return HC_SUCCESS;
                }
            }
        }
        else
        {
            boost::shared_ptr<iItem> oldbs = gdata->m_baoshis.getItem(slot);
            if (oldbs.get())
            {
                return HC_ERROR;
            }

            newBaoshi* pb = dynamic_cast<newBaoshi*>(itm.get());
            //通知战斗属性变化
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "baoshi_change") );
            obj.push_back( Pair("s", 200) );
            obj.push_back( Pair("name", pb->m_base.baseAttr.name) );
            obj.push_back( Pair("add_pre", 0) );
            obj.push_back( Pair("add_now", pb->value()) );
            //act统计
            act_to_tencent(this,act_new_baoshi_xiangqian, pb->m_base.type, pb->level());
            if (pb->getCount() == 1)
            {
                m_bag.removeItem(bagSlot);
                gdata->m_baoshis.addItem(slot, itm, false);
                itm->setChanged();
                itm->Save();
                gdata->updateBaoshiAttr();
                set_attack_change();
                NotifyZhenData();
                return HC_SUCCESS;
            }
            else
            {
                pb->addCount(-1);
                boost::shared_ptr<iItem> bs = Singleton<newBaoshiMgr>::Instance().cloneBaoshi(pb->m_base.type, pb->level(), 1);
                gdata->m_baoshis.addItem(slot, bs, false);
                bs->setChanged();
                itm->setChanged();
                itm->Save();
                bs->Save();
                gdata->updateBaoshiAttr();
                set_attack_change();
                NotifyZhenData();
                return HC_SUCCESS;
            }
            //swapBag(m_bag, bagSlot, gdata->m_baoshis, slot);
        }
    }
    return HC_ERROR;
}

int CharData::giveBaoshi(int type, int level, int reason)
{
    return Singleton<newBaoshiMgr>::Instance().addBaoshi(this, type, level, 1, reason);
}

//加载特殊字段
void CharData::loadExtraData()
{
    Query q(GetDb());
    q.get_result("select type,field,value from char_data_extra where (type=0 or type=1 or type=2) and cid=" + LEX_CAST_STR(m_id));
    while (q.fetch_row())
    {
        int type = q.getval();
        int field = q.getval();
        int value = q.getval();
        switch (type)
        {
            case 0:
                m_normal_extra_data[field] = value;
                break;
            case 1:
                m_daily_extra_data[field] = value;
                break;
            case 2:
                m_week_extra_data[field] = value;
                break;
        }
    }
    q.free_result();
}

//取角色字段值
int CharData::queryExtraData(int type, int field)
{
    switch (type)
    {
        case 0:
            if (m_normal_extra_data.find(field) != m_normal_extra_data.end())
            {
                return m_normal_extra_data[field];
            }
            break;
        case 1:
            if (m_daily_extra_data.find(field) != m_daily_extra_data.end())
            {
                return m_daily_extra_data[field];
            }
            break;
        case 2:
            if (m_week_extra_data.find(field) != m_week_extra_data.end())
            {
                return m_week_extra_data[field];
            }
            break;
    }
    return 0;
}

//设置角色字段值
void CharData::setExtraData(int type, int field, int value)
{
    switch (type)
    {
        case 0:
            m_normal_extra_data[field] = value;
            InsertSaveDb("replace into char_data_extra (cid,type,field,value) values ("
                + LEX_CAST_STR(m_id) + ",0,"
                + LEX_CAST_STR(field) + ","
                + LEX_CAST_STR(value) + ")");
            break;
        case 1:
            m_daily_extra_data[field] = value;
            InsertSaveDb("replace into char_data_extra (cid,type,field,value) values ("
                + LEX_CAST_STR(m_id) + ",1,"
                + LEX_CAST_STR(field) + ","
                + LEX_CAST_STR(value) + ")");
            break;
        case 2:
            m_week_extra_data[field] = value;
            InsertSaveDb("replace into char_data_extra (cid,type,field,value) values ("
                + LEX_CAST_STR(m_id) + ",2,"
                + LEX_CAST_STR(field) + ","
                + LEX_CAST_STR(value) + ")");
            break;
    }
}

//清除角色字段值
void CharData::clearExtraData(int type)
{
    switch (type)
    {
        case 0:
            //m_normal_extra_data.clear();
            break;
        case 1:
            m_daily_extra_data.clear();
            break;
        case 2:
            m_week_extra_data.clear();
            break;
    }
}

int CharData::getChangeSpic()
{
    if (m_change_spic == 0)
    {
        return 0;
    }
    if (time(NULL) < m_change_spic_time)
    {
        return m_change_spic;
    }
    else
    {
        m_change_spic = 0;
        setExtraData(char_data_type_normal, char_data_change_spic, 0);
        return 0;
    }
}

//某样装备的最高等级
int CharData::maxEquipLevel(int type)
{
    int id = 0;
    int qLevel = m_bag.maxEquipLevel(type, id);
    for (std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.begin(); it != m_generals.m_generals.end(); ++it)
    {
        if (it->second.get())
        {
            int id2 = 0;
            int qqLevel = it->second->m_equipments.maxEquipLevel(type, id2);
            if (qqLevel > qLevel)
            {
                qLevel = qqLevel;
                id = id2;
            }
        }
    }
    return qLevel;
}

//全服免费VIP 4活动
int CharData::startTmpVIP2()
{
    //试用VIP开启
    if (m_currentStronghold > iTmpVip_OpenStronghold && m_vip < iTmpVip_level
        && GeneralDataMgr::getInstance()->isFreeVIPEventOpen())
    {
        m_vip = m_tmp_vip = iTmpVip_level;
        m_tmp_vip_start_time = time(NULL);
        m_enhance_cd = 0;
        m_can_enhance = true;
        m_tmp_vip_end_time = GeneralDataMgr::getInstance()->freeVIPEndtime();
        InsertSaveDb("replace into char_tmp_vip set cid=" + LEX_CAST_STR(m_id) + ",starttime=" + LEX_CAST_STR(m_tmp_vip_start_time) + ",endtime=" + LEX_CAST_STR(m_tmp_vip_end_time));
        NotifyCharData();
        /*std::string notify_msg = strTmpVIP;
        str_replace(notify_msg, "$N", MakeCharNameLink(m_name));
        if (notify_msg != "")
        {
            GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
        }*/
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int CharData::startTmpVIP()
{
    //试用VIP开启
    if (m_currentStronghold == iTmpVip_OpenStronghold && m_vip < iTmpVip_level)
    {
        m_vip = m_tmp_vip = iTmpVip_level;
        m_tmp_vip_start_time = time(NULL);
        m_tmp_vip_end_time = m_tmp_vip_start_time + g_tmpvip_secs;
        m_enhance_cd = 0;
        m_can_enhance = true;
        InsertSaveDb("replace into char_tmp_vip set cid=" + LEX_CAST_STR(m_id) + ",starttime=" + LEX_CAST_STR(m_tmp_vip_start_time) + ",endtime=" + LEX_CAST_STR(m_tmp_vip_end_time));
        NotifyCharData();
        std::string notify_msg = strTmpVIP;
        str_replace(notify_msg, "$N", MakeCharNameLink(m_name));
        if (notify_msg != "")
        {
            GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

bool CharData::check_friend(int id)
{
    Singleton<relationMgr>::Instance().is_my_friend(m_id, id);
}

//注册天数
int CharData::regDays()
{
    time_t olgift_end = queryExtraData(char_data_type_normal, char_data_get_onlinegift_day);
    time_t tnow = time(NULL);
    if (olgift_end > tnow)
    {
        return 3 - (olgift_end - tnow) / iONE_DAY_SECS;
    }
    else
    {
        return 3 + (iONE_DAY_SECS - 1 + tnow - olgift_end) / iONE_DAY_SECS;
    }
}

//更新宝石孔-true 宝石孔变化了
bool CharData::updateBaoshiCount()
{
    int baoshi_count = m_baoshi_count;
    //修改宝石孔开放条件时，需要修改另外一个地方-->
    if (m_level < 28)
    {
        m_baoshi_count = 0;
    }
    else if (m_level < 35)
    {
        m_baoshi_count = 1;
    }
    else if (m_level < 40)
    {
        m_baoshi_count = 2;
    }
    else if (m_level < 45)
    {
        m_baoshi_count = 3;
    }
    else if (m_level < 50)
    {
        m_baoshi_count = 4;
    }
    else if (m_level < 55)
    {
        m_baoshi_count = 5;
    }
    else
    {
        m_baoshi_count = 6;
    }
    return m_baoshi_count != baoshi_count;
}

//更新武将的宝石孔
int CharData::updateGeneralsBaoshiCount()
{
    //武将宝石数量改为跟角色等级挂钩
    {
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.begin();
        while (it != m_generals.m_generals.end())
        {
            if (it->second.get())
            {
                it->second->m_baoshis.size(m_baoshi_count);
            }
            ++it;
        }
    }

    {
        //武将宝石数量改为跟角色等级挂钩
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_fired_generals.begin();
        while (it != m_generals.m_fired_generals.end())
        {
            if (it->second.get())
            {
                it->second->m_baoshis.size(m_baoshi_count);
            }
            ++it;
        }
    }
}

#if 0
bool CharData::addExp(int exp)
{
    bool levelup = false;
    if (exp < 0)
    {
        uint64_t subexp = (uint64_t)(-exp);
        if (subexp > m_exp)
        {
            m_exp = 0;
        }
        else
        {
            m_exp -= subexp;
        }
    }
    else
    {
        //保存经验
        InsertSaveDb("update char_data set exp=" + LEX_CAST_STR(m_exp) + " where cid=" + LEX_CAST_STR(m_id));
        uint64_t addexp = (uint64_t)exp;
        m_exp += addexp;
        //未超过地图等级限制和最高等级限制
        if (m_level < 20*m_area && m_level < iMaxCharLevel)
        {
            int lv = m_level;
            uint64_t level_up_exp = get_level_cost(lv + 1);
            while (m_exp >= level_up_exp && m_level < 20*m_area && m_level < iMaxCharLevel)
            {
                cout<<"cur exp:"<<m_exp<<",levelup exp:"<<level_up_exp<<",cur level:"<<lv<<endl;
                ++lv;
                level(lv);
                m_exp -= level_up_exp;
                level_up_exp = get_level_cost(lv);
            }
            levelup = true;
        }
        NotifyCharExp();
    }
    return levelup;
}
#endif


//角色详细信息
int CharData::getRoleDetail(json_spirit::Object& robj)
{
    json_spirit::Object role;
    role.push_back( Pair("userid", m_id) );
    role.push_back( Pair("spic", m_spic) );
    role.push_back( Pair("name", m_name) );
    role.push_back( Pair("level", m_level) );
    //role.push_back( Pair("camp", m_camp) );
    //role.push_back( Pair("prestige", m_prestige) );

    if (m_corps_member.get())
    {
        role.push_back( Pair("troop", corpsMgr::getInstance()->getCorpsName(m_corps_member->corps)) );
    }

    robj.push_back( Pair("role", role) );

    //官职开放了才有官职
    if (m_officeOpen)
    {
        json_spirit::Object o;
        bool canGet = false;
        boost::shared_ptr<baseoffical> p_bo = GeneralDataMgr::getInstance()->GetBaseOffical(m_offical);
        if (p_bo.get())
        {
            json_spirit::Object curPost;
            curPost.push_back( Pair("id", p_bo->m_id) );
            curPost.push_back( Pair("name", p_bo->m_name) );
            curPost.push_back( Pair("prestige", m_prestige) );
            curPost.push_back( Pair("salary", p_bo->m_salary) );
            curPost.push_back( Pair("canGet", m_hasgetsalary) );
            curPost.push_back( Pair("canLevelUp", m_officalcanlevelup) );
            o.push_back( Pair("curPost", curPost) );
            canGet = m_hasgetsalary == 0;
        }
        else
        {
            json_spirit::Object curPost;
            curPost.push_back( Pair("prestige", m_prestige) );
            curPost.push_back( Pair("salary", 0) );
            curPost.push_back( Pair("canGet", 1) );
            curPost.push_back( Pair("canLevelUp", m_officalcanlevelup) );
            o.push_back( Pair("curPost", curPost) );
        }

        int gid = 0;
        if (m_officalcanlevelup)
        {
            gid = _checkGuide(guide_id_up_offical);
        }
        if (canGet && gid == 0)
        {
            _checkGuide(guide_id_get_salary);
        }
        p_bo = GeneralDataMgr::getInstance()->GetBaseOffical(m_offical + 1);
        if (p_bo.get())
        {
            json_spirit::Object nextPost;
            nextPost.push_back( Pair("id", p_bo->m_id) );
            nextPost.push_back( Pair("name", p_bo->m_name) );
            nextPost.push_back( Pair("prestige", p_bo->need_prestige) );
            nextPost.push_back( Pair("salary", p_bo->m_salary) );
            json_spirit::Array list;
            std::list<boost::shared_ptr<officalgenerals> >& p_list = GeneralDataMgr::getInstance()->GetBaseOfficalGenerals();
            std::list<boost::shared_ptr<officalgenerals> >::iterator it = p_list.begin();
            while(it != p_list.end())
            {
                if ((*it).get() && (*it)->need_offical == (m_offical + 1))
                {
                    json_spirit::Object officalgeneral;
                    officalgeneral.push_back( Pair("id", (*it)->m_gid) );
                    officalgeneral.push_back( Pair("name", (*it)->m_name) );
                    officalgeneral.push_back( Pair("spic", (*it)->m_spic) );
                    officalgeneral.push_back( Pair("price", (*it)->m_price) );
                    list.push_back(officalgeneral);
                }
                ++it;
            }
            nextPost.push_back( Pair("generals", list) );
            o.push_back( Pair("nextPost", nextPost) );
        }
        robj.push_back( Pair("officer", o) );
    }

    //演兵数值
    json_spirit::Array slist;
    int tmp_pugong = 0, tmp_pufang = 0, tmp_cegong = 0, tmp_cefang = 0, tmp_bingli = 0;
    if (m_soulOpen)
    {
        boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(*this);
        if (ct.get())
        {
            for (int i = 0; i < iTrainingNum; ++i)
            {
                if (ct->m_soldier_souls[i]._centerSoul)
                {
                    baseSoul* p_center = ct->m_soldier_souls[i]._centerSoul;
                    if (p_center->_level > 0)
                    {
                        if (p_center->_bingli > 0)
                        {
                            tmp_bingli += p_center->_bingli;
                        }
                        if (p_center->_attack > 0)
                        {
                            tmp_pugong += p_center->_attack;
                            tmp_cegong += p_center->_attack;
                        }
                        if (p_center->_wufang > 0)
                        {
                            tmp_pufang += p_center->_wufang;
                        }
                        if (p_center->_cefang > 0)
                        {
                            tmp_cefang += p_center->_cefang;
                        }
                    }
                }
            }
        }
    }
    slist.push_back(tmp_pugong);
    slist.push_back(tmp_pufang);
    slist.push_back(tmp_cegong);
    slist.push_back(tmp_cefang);
    slist.push_back(tmp_bingli);
    robj.push_back( Pair("slist", slist) );
    //兵器数值
    json_spirit::Array wlist;
    for (int i = 0; i < 5; ++i)
    {
        wlist.push_back(m_new_weapons._weapons[i]._effect);
    }
    robj.push_back( Pair("wlist", wlist) );
    //战马数值
    json_spirit::Array horselist;
    if (m_horse.horse)
    {
        horselist.push_back(m_horse.horse->pugong + m_horse.pugong);
        horselist.push_back(m_horse.horse->pufang + m_horse.pufang);
        horselist.push_back(m_horse.horse->cegong + m_horse.cegong);
        horselist.push_back(m_horse.horse->cefang + m_horse.cefang);
        horselist.push_back(m_horse.horse->bingli + m_horse.bingli);
    }
    else
    {
        horselist.push_back(0);
        horselist.push_back(0);
        horselist.push_back(0);
        horselist.push_back(0);
        horselist.push_back(0);
    }
    robj.push_back( Pair("horselist", horselist) );

    if (m_weapon_attack_change)
    {
        updateAttackDefense();
    }

    //总数值加成
    json_spirit::Array tlist;
    tlist.push_back(getPugong(true) + tmp_pugong);
    tlist.push_back(getPufang(true) + tmp_pufang);
    tlist.push_back(getCegong(true) + tmp_cegong);
    tlist.push_back(getCefang(true) + tmp_cefang);
    tlist.push_back(getBingli(true) + tmp_bingli);
    robj.push_back( Pair("tlist", tlist) );

    //装备信息
    //json_spirit::Array elist;
    //m_backpack.equipments.getList(elist);
    //robj.push_back( Pair("elist", elist) );

    //兵器信息
    //json_spirit::Array wlist;
    //m_new_weapons.getList(wlist);
    //robj.push_back( Pair("wlist", wlist) );

    //部队信息
    int attack = 0;
    json_spirit::Array hlist;
    boost::shared_ptr<ZhenData> zdata = m_zhens.GetZhen(m_zhens.GetDefault());
    if (zdata.get())
    {
        zdata->getList(hlist);
        attack = zdata->getAttack();
    }
    robj.push_back( Pair("hlist", hlist) );
    robj.push_back( Pair("attack", attack) );
    return HC_SUCCESS;
}

int CharData::addGongxun(int a)
{
    int err_code = 0;
    return m_bag.addGem(treasure_type_gongxun, a, err_code);
}

int CharData::getGongxun()
{
    return m_bag.getGemCurrency(treasure_type_gongxun);
}

int CharData::addLibao(int libao_id, int counts)
{
    int32_t ret = 0;
    for (int i = 0; i < counts; ++i)
    {
        ret = m_bag.addLibao(libao_id);
    }
    return ret;
}

int CharData::getGuideState1(int guide)
{
#if 0
    /*
        步骤2，是否可以领取第一个任务奖励。
        步骤3，是否已领取第一个任务奖励。
        步骤4，是否可以领取第2个任务奖励。
        步骤5，是否已领取第二个任务奖励。
        步骤6，是否可以领取第3个任务奖励。
        步骤7，是否已领取第三个任务奖励。
        步骤8，是否可以领取第4个任务奖励。
        步骤9，武将是否已穿装备。
    */
    switch (guide)
    {
        case 2:
            if (m_task.tid == 1 && m_task.done)
            {
                return 1;
            }
            break;
        case 3:
            if (m_task.tid > 1)
            {
                return 1;
            }
            break;
        case 4:
            if (m_task.tid == 2 && m_task.done)
            {
                return 1;
            }
            break;
        case 5:
            if (m_task.tid > 2)
            {
                return 1;
            }
            break;
        case 6:
            if (m_task.tid == 3 && m_task.done)
            {
                return 1;
            }
            break;
        case 7:
            if (m_task.tid > 3)
            {
                return 1;
            }
            break;
        case 8:
            if (m_task.tid == 4 && m_task.done)
            {
                return 1;
            }
            break;
        case 9:
            {
                std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.begin();
                while (it != m_generals.m_generals.end())
                {
                    if (it->second->m_equipments.getUsed())
                    {
                        return 1;
                    }
                    ++it;
                }
            }
            break;
    }
    return 0;
#else
    /*
        步骤2，完成状态: 第1个任务是否完成。(可领取)
        步骤3，完成状态: 第1个任务奖励是否已领取。
        步骤4，完成状态: 第2个任务是否完成。
        步骤5，完成状态: 第2个任务奖励是否已领取。
        步骤6，完成状态: 第3个任务是否完成。
        步骤7，完成状态: 武将是否已穿装备。
        步骤8，完成状态:第3个任务奖励是否已领取。
        步骤9，完成状态: 第4个任务是否完成。
    */
    switch (guide)
    {
        case 2:
            if (m_task.tid == 1 && m_task.done)
            {
                return 1;
            }
            break;
        case 3:
            if (m_task.tid > 1)
            {
                return 1;
            }
            break;
        case 4:
            if (m_task.tid == 2 && m_task.done)
            {
                return 1;
            }
            break;
        case 5:
            if (m_task.tid > 2)
            {
                return 1;
            }
            break;
        case 6:
            if (m_task.tid == 3 && m_task.done)
            {
                return 1;
            }
            break;
        case 7:
            {
                std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.m_generals.begin();
                while (it != m_generals.m_generals.end())
                {
                    if (it->second->m_equipments.getUsed())
                    {
                        return 1;
                    }
                    ++it;
                }
            }
            break;
        case 8:
            if (m_task.tid > 3)
            {
                return 1;
            }
            break;
        case 9:
            if (m_task.tid == 4 && m_task.done)
            {
                return 1;
            }
            break;
    }
    return 0;
#endif
}

int CharData::getGuideState2(int guide)
{
    if (guide >= 2 && guide <= 9)
    {
        return getGuideState1(guide);
    }
    else
    {
        return 1;
    }
}

int CharData::getCurrentGuide(int& state)
{
    state = getGuideState1(m_current_guide);
    return m_current_guide;
}

//关卡状态
int CharData::getStrongholdState(int strongholdId)
{
    boost::shared_ptr<StrongholdData> stronghold_data = GeneralDataMgr::getInstance()->GetStrongholdData(strongholdId);
    StrongholdData *bShold = stronghold_data.get();
    if (NULL == bShold)
    {
        return -2;
    }
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = m_tempo.CharMapsData.find(bShold->m_map_id);
    if (it != m_tempo.CharMapsData.end())
    {
        //所在的地图数据
        CharMapData* md = it->second.get();
        if (md == NULL)
        {
            return -2;
        }
        CharMapData::iterator itm = md->find(bShold->m_stage_id);
        if (itm != md->end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            //所在场景
            CharStageData* cStage = itm->second.get();
            if (NULL == cStage)
            {
                return -2;
            }
            CharStrongholdData* pCShold = cStage->m_stronghold[bShold->m_strongholdpos - 1].get();
            if (pCShold == NULL)
            {
                return -2;
            }
            return pCShold->m_state;
        }
    }
    return -2;
}

boost::shared_ptr<CharStrongholdData> CharData::getDestStronghold()
{
    boost::shared_ptr<CharStrongholdData> cd;
    boost::shared_ptr<StrongholdData> stronghold_data = GeneralDataMgr::getInstance()->GetStrongholdData(m_currentStronghold+1);
    StrongholdData *bShold = stronghold_data.get();
    if (NULL == bShold)
    {
        stronghold_data = GeneralDataMgr::getInstance()->GetStrongholdData(m_currentStronghold);
        bShold = stronghold_data.get();
        if (NULL == bShold)
        {
            return cd;
        }
    }
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = m_tempo.CharMapsData.find(bShold->m_map_id);
    if (it != m_tempo.CharMapsData.end())
    {
        //所在的地图数据
        CharMapData* md = it->second.get();
        if (md == NULL)
        {
            return cd;
        }
        CharMapData::iterator itm = md->find(bShold->m_stage_id);
        if (itm != md->end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            //所在场景
            CharStageData* cStage = itm->second.get();
            if (NULL == cStage)
            {
                return cd;
            }
            return cStage->m_stronghold[bShold->m_strongholdpos - 1];
        }
    }
    return cd;
}

void CharData::SaveNick()
{
    InsertSaveDb("update charactors set nick='" + m_nick.get_string() + "' where id=" + LEX_CAST_STR(m_id));
}

std::string CharData::getUserMsg()
{
#ifdef QQ_PLAT
    //平台id|游戏id|生日|性别|游戏内好友数|平台内好友数|游戏等级|游戏经验值|金币余额|银币余额|1|2|3|4|vip等级|首次进入时间|分区|国家|游戏平台
    std::string user_msg = ""+LEX_CAST_STR(m_qid)+"|"+LEX_CAST_STR(m_id)+"||"
        +LEX_CAST_STR(m_gender)+"|"+LEX_CAST_STR(Singleton<relationMgr>::Instance().getFriendsCount(m_id))
        +"|0|"+LEX_CAST_STR(m_level)+"|" + LEX_CAST_STR(m_prestige)+ "|"+LEX_CAST_STR(m_gold)+"|"+LEX_CAST_STR(m_silver)
        +"|" + LEX_CAST_STR(m_ling)+ "|||" + LEX_CAST_STR(m_qq_yellow_level) + "|"+LEX_CAST_STR(m_vip)+"|"+LEX_CAST_STR(m_createTime)+"|"+m_server_id+"|zh_CN|" + LEX_CAST_STR(m_union_id);
    return user_msg;
#else
    int need_supply = 0, max_supply = 0;
    GetMaxSupply(need_supply,max_supply);
    //平台id|游戏id|性别|游戏等级|游戏声望值|金币余额|银币余额|军令|军粮数|所需军粮|总战力|vip等级|首次进入时间|分区|游戏平台|vcode|sid
    std::string user_msg = ""+LEX_CAST_STR(m_qid)+"|"+LEX_CAST_STR(m_id)+"|"
        +LEX_CAST_STR(m_gender)+"|"+LEX_CAST_STR(m_level)+"|" + LEX_CAST_STR(m_prestige)+ "|"+LEX_CAST_STR(m_gold)+"|"+LEX_CAST_STR(m_silver)
        +"|" + LEX_CAST_STR(m_ling)+ "|" + LEX_CAST_STR(m_bag.getGemCount(treasure_type_supply)) + "|" + LEX_CAST_STR(need_supply) + "|" + LEX_CAST_STR(getAttack()) + "|"+LEX_CAST_STR(m_vip)+"|"+LEX_CAST_STR(m_createTime)+"|"+m_server_id+"|" + LEX_CAST_STR(m_union_id)+"|"+m_vcode+"|"+m_sid;
    return user_msg;
#endif
}

bool CharData::canFindBack()
{
    return queryExtraData(char_data_type_daily,char_data_daily_findback) > 0;
}

/***************************武将相关 **************************/

void BaseSoldierData::addObj(const std::string& name, json_spirit::Object& generalobj)
{
    json_spirit::Object soldierobj;
    soldierobj.push_back( Pair("type", m_base_type));
    soldierobj.push_back( Pair("attack_type", m_damage_type));
    //soldierobj.push_back( Pair("attack_range", m_attack_type));
    soldierobj.push_back( Pair("name", m_name));
    soldierobj.push_back( Pair("spic", m_spic));
    soldierobj.push_back( Pair("memo", m_desc));

    generalobj.push_back( Pair("soldier", soldierobj));
}

void GeneralTypeData::toObj(json_spirit::Object& generalobj)
{
    generalobj.push_back( Pair("name", m_name));
    generalobj.push_back( Pair("spic", m_spic));
    generalobj.push_back( Pair("memo", m_desc));
    generalobj.push_back( Pair("base_brave", base_str));
    generalobj.push_back( Pair("base_wisdom", base_int));
    generalobj.push_back( Pair("base_govern", base_tongyu));
    json_spirit::Object tianfu;
    tianfu.push_back( Pair("name", m_new_tianfu.m_name) );
    tianfu.push_back( Pair("memo", m_new_tianfu.m_memo) );
    generalobj.push_back( Pair("tianfu", tianfu) );

    generalobj.push_back( Pair("inherit_cnt", m_inherit_cnt));
    if (m_speSkill.get())
    {
        generalobj.push_back( Pair("attack_range", m_speSkill->attack_type));
        generalobj.push_back( Pair("spe_name", m_speSkill->name));
        generalobj.push_back( Pair("spe_memo", m_speSkill->memo));
    }
    generalobj.push_back( Pair("good_at", m_good_at));
}

//武将升级
int CharGeneralData::Levelup(int level)
{
    if (m_level < level && m_level < m_belong_to.m_charData.m_level)
    {
        INFO("********level up" << endl);
        INFO("fac=" << m_chengzhang << endl);

        ++m_level;

        double add = m_chengzhang;
        int temp = (int)(add * 100);
        m_add += (double)temp/100;

        //if ((int)m_add >= 1)
        //{
        //    SetColor();
        //}
        //更新屬性
        updateAttribute();
        INFO("this levelup getadd=" << add << endl);

        //更新任务
        m_belong_to.m_charData.updateTask(task_general_level, m_gid, m_level);
        m_belong_to.m_charData.updateTask(task_group_general_level, m_level, 0);

        //攻擊力變化
        m_belong_to.m_charData.set_attack_change();

        //新战力
        general_change = true;
        //equip_change = true;
        updateEquipmentEffect();
        reborn_change = true;

        //m_belong_to.m_charData.NotifyZhenData();
        //updateBaoshiCount();

        return 0;
    }
    return -1;
}

//更新可以镶嵌的宝石数量
void CharGeneralData::updateBaoshiCount()
{
    m_baoshis.size(m_belong_to.m_charData.m_baoshi_count);
}

void CharGeneralData::updateEquipmentEffect()
{
    m_attack = 0;
    m_pufang = 0;
    m_cefang = 0;
    m_hp = 0;

    for (int i = 1; i <= equip_slot_max; ++i)
    {
        EquipmentData* eq = dynamic_cast<EquipmentData*>(m_equipments.getItem(i).get());
        if (eq)
        {
            switch (eq->type)
            {
                case equip_ring:
                    m_attack += (eq->value + eq->addValue);
                    break;
                case equip_shield:
                    m_pufang += (eq->value + eq->addValue);
                    break;
                case equip_cloth:
                    m_cefang += (eq->value + eq->addValue);
                    break;
                case equip_fu:
                    m_hp += (eq->value + eq->addValue);
                    break;
                case equip_sword:
                    m_attack += (eq->value + eq->addValue);
                    break;
                case equip_necklace:
                    m_pufang += (eq->value + eq->addValue);
                    m_cefang += (eq->value2 + eq->addValue2);
                    break;
            }
        }
    }
    m_belong_to.m_charData.m_weapon_attack_change = true;
    //新战力
    equip_change = true;
    m_belong_to.m_charData.NotifyCharData();
}

int CharGeneralData::SetColor(bool update_color /*= false*/)
{
    bool bUpColor = false;
    if (update_color)
    {
        bUpColor = true;
        m_color_link = m_link_name;
        addColor(m_color_link, m_color);
    }
#if 1
    //绰号暂时都开启，没有特殊作用
    b_nickname = 1;
#else
    //绰号一旦获得了就不会失去
    if (0 == b_nickname && m_color >= 5 && m_genius_count == iGeniusMaxNum)
    {
        b_nickname = 1;
        std::string msg = strGeniusOpenBroad;
        str_replace(msg, "$N", m_belong_to.m_charData.m_name);
        str_replace(msg, "$n", m_baseGeneral->m_name);
        str_replace(msg, "$G", m_baseGeneral->m_nickname);
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg,-1);
    }
#endif
    if (bUpColor)
    {
        //m_belong_to.m_charData.NotifyZhenData();
        return m_color;
    }
    return 0;
}

#if 0
int CharGeneralData::xiangqian(boost::shared_ptr<baowuBaoshi> baoshi)
{
    if (baoshi.get())
    {
        //cout<<"general xiang qian,baowu level:"<<m_baowu_level<<",baoshi vector size() "<<m_baoshi.size()<<endl;
        for (size_t i = 0; i < m_baoshi.size(); ++i)
        {
            if (!m_baoshi[i].get())
            {
                //cout<<i<<",find a location."<<endl;
                m_baoshi[i] = baoshi;
                return HC_SUCCESS;
            }
        }
        int baoshiNum = (m_baowu_level + 1) / 2;
        if (baoshiNum >= iGeneralMaxBaoshi)
        {
            baoshiNum = iGeneralMaxBaoshi;
        }
        if ((int)m_baoshi.size() < baoshiNum)
        {
            m_baoshi.push_back(baoshi);
            return HC_SUCCESS;
        }
        else
        {
            return HC_ERROR_BAOSHI_NOS_POS;
        }
    }
    return HC_ERROR;
}
#endif

void CharGeneralData::toObj(json_spirit::Object& generalobj)
{
    if (!m_baseGeneral.get())
    {
        return;
    }
    if (!m_baseSoldier.get())
    {
        return;
    }
    generalobj.push_back( Pair("id", m_id));
    generalobj.push_back( Pair("level", m_level));
    generalobj.push_back( Pair("color", m_color));
    int total_str = m_str;
    int total_int = m_int;
    int total_tong = m_tongyu;
    switch(m_baseGeneral->m_baowu_type)
    {
        case 1:
            total_str += m_baowu_add;
            break;
        case 2:
            total_int += m_baowu_add;
            break;
        case 3:
            total_tong += m_baowu_add;
            break;
        default:
            break;
    }
    generalobj.push_back( Pair("brave", total_str));
    generalobj.push_back( Pair("wisdom", total_int));
    generalobj.push_back( Pair("govern", total_tong));

    int add_tong = (int)m_add, add_str = (int)m_add, add_int = (int)m_add, tmp_type = 0;
    if (m_baseGeneral->m_new_tianfu.m_more_tong)
    {
        add_tong = (int)((100+m_baseGeneral->m_new_tianfu.m_more_tong)*m_add/100);
    }
    if (m_baseGeneral->m_new_tianfu.m_more_int)
    {
        add_int = (int)((100+m_baseGeneral->m_new_tianfu.m_more_int)*m_add/100);
    }
    if (m_baseGeneral->m_new_tianfu.m_more_str)
    {
        add_str = (int)((100+m_baseGeneral->m_new_tianfu.m_more_str)*m_add/100);
    }
    switch (m_baseGeneral->m_tianfu)
    {
        case 1:
            tmp_type = 3;//天赋类型转换成显示类型
            break;
        case 2:
            tmp_type = 2;//天赋类型转换成显示类型
            break;
        case 3:
            tmp_type = 1;//天赋类型转换成显示类型
            break;
        default:
            break;
    }
    generalobj.push_back( Pair("add_brave", add_str));
    generalobj.push_back( Pair("add_wisdom", add_int));
    generalobj.push_back( Pair("add_govern", add_tong));
    //命中暴击之类的信息

    //命中 (抗躲闪)
    generalobj.push_back( Pair("hit", m_combat_attr.special_resist_level(special_attack_dodge)));
    //暴击
    generalobj.push_back( Pair("crit", m_combat_attr.special_attack_level(special_attack_baoji)));
    //识破
    generalobj.push_back( Pair("shipo", m_combat_attr.special_attack_level(special_attack_shipo)));
    //格挡
    generalobj.push_back( Pair("parry", m_combat_attr.special_attack_level(special_attack_parry)));
    //躲闪
    generalobj.push_back( Pair("resist_hit", m_combat_attr.special_attack_level(special_attack_dodge)));
    //抗暴击
    generalobj.push_back( Pair("resist_crit", m_combat_attr.special_resist_level(special_attack_baoji)));
    //抗识破
    generalobj.push_back( Pair("resist_shipo", m_combat_attr.special_resist_level(special_attack_shipo)));
    //抗格挡
    generalobj.push_back( Pair("resist_parry", m_combat_attr.special_resist_level(special_attack_parry)));
    //攻击防御之类的信息
    int attack = 0, bingli = 0, pufang = 0, cefang = 0;
    if (act_wuli_attack == m_baseSoldier->m_damage_type)
    {
        attack = m_attack + m_combat_attr.skill_add(skill_add_pugong) + (2*m_str) + m_belong_to.m_charData.getPugong(true);
    }
    else
    {
        attack = m_attack + m_combat_attr.skill_add(skill_add_cegong) + (2*m_int) + m_belong_to.m_charData.getCegong(true);
    }
    bingli = m_hp + m_combat_attr.skill_add(skill_add_hp) + 3*m_tongyu + m_belong_to.m_charData.getBingli(true);
    pufang = m_pufang + m_combat_attr.skill_add(skill_add_pufang) + (7*m_str)/5 + m_belong_to.m_charData.getPufang(true);
    cefang = m_cefang + m_combat_attr.skill_add(skill_add_cefang) + (7*m_int)/5 + m_belong_to.m_charData.getCefang(true);
    //成长星级加成
    if (m_chengzhang_star.get())
    {
        attack += m_chengzhang_star->gongji;
        pufang += m_chengzhang_star->fangyu;
        cefang += m_chengzhang_star->fangyu;
        bingli += m_chengzhang_star->bingli;
    }
    //将魂加成
    if (m_general_soul)
    {
        attack += m_general_soul->getAttack(m_color);
        pufang += m_general_soul->getWufang(m_color);
        cefang += m_general_soul->getCefang(m_color);
        bingli += m_general_soul->getBingli(m_color);
    }
    if (m_belong_to.m_charData.m_soulOpen)
    {
        boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(m_belong_to.m_charData);
        if (ct.get())
        {
            attack += (ct->_combatAttr.soul_add_attack(m_baseSoldier->m_base_type));
            bingli += (ct->_combatAttr.soul_add_hp(m_baseSoldier->m_base_type));
            pufang += (ct->_combatAttr.soul_add_wufang(m_baseSoldier->m_base_type));
            cefang += (ct->_combatAttr.soul_add_cefang(m_baseSoldier->m_base_type));
        }
    }

    //武将天赋
    if (m_baseGeneral.get())
    {
        if (m_baseGeneral->m_new_tianfu.m_more_hp)
        {
            bingli = (100 + m_baseGeneral->m_new_tianfu.m_more_hp) * bingli / 100;
        }
    }

    //限时增益效果加成
    int hp_buff = 0, attack_buff = 0, wu_fang_buff = 0, ce_fang_buff = 0;
    //兵力物攻物防策攻策防
    for (int i = 0; i < 5; ++i)
    {
        switch(i+1)
        {
            case 1:
                hp_buff = bingli * (m_belong_to.m_charData.m_Buffs.buffs[i].m_value) / 100;
                break;
            case 2:
                if (act_wuli_attack == m_baseSoldier->m_damage_type)
                {
                    attack_buff = attack * (m_belong_to.m_charData.m_Buffs.buffs[i].m_value) / 100;
                }
                break;
            case 3:
                wu_fang_buff = pufang * (m_belong_to.m_charData.m_Buffs.buffs[i].m_value) / 100;
                break;
            case 4:
                if (act_wuli_attack != m_baseSoldier->m_damage_type)
                {
                    attack_buff = attack * (m_belong_to.m_charData.m_Buffs.buffs[i].m_value) / 100;
                }
                break;
            case 5:
                ce_fang_buff = cefang * (m_belong_to.m_charData.m_Buffs.buffs[i].m_value) / 100;
                break;
            default:
                break;
        }
    }

    //将星录加成
    int hp_jxl = 0, cefang_jxl = 0, wufang_jxl = 0, attack_jxl = 0;
    m_belong_to.m_charData.m_jxl_buff.total_buff_attr.get_add(attack, bingli, pufang, cefang, attack_jxl, hp_jxl, wufang_jxl, cefang_jxl);

    //皇座称号加成
    int hp_throne = 0, attack_throne = 0, wu_fang_throne = 0, ce_fang_throne = 0;
    int throne_per = 0;
    if (m_belong_to.m_charData.m_nick.check_nick(nick_throne_start))
    {
        throne_per = 8;
    }
    else if(m_belong_to.m_charData.m_nick.check_nick(nick_throne_start + 1))
    {
        throne_per = 5;
    }
    else if(m_belong_to.m_charData.m_nick.check_nick(nick_throne_start + 2))
    {
        throne_per = 3;
    }
    hp_throne = bingli * throne_per / 100;
    attack_throne = attack * throne_per / 100;
    wu_fang_throne = pufang * throne_per / 100;
    ce_fang_throne = cefang * throne_per / 100;

    //cdata->m_jxl_buff.total_buff_attr.dump();
    //cout<<"general jxl:"<<hp_jxl<<","<<attack_jxl<<","<<cefang_jxl<<","<<wufang_jxl<<endl;
    bingli += (hp_buff + hp_jxl + hp_throne);
    attack += (attack_buff + attack_jxl + attack_throne);
    pufang += (wu_fang_buff + wufang_jxl + wu_fang_throne);
    cefang += (ce_fang_buff + cefang_jxl + ce_fang_throne);

    generalobj.push_back( Pair("gongji", attack));
    generalobj.push_back( Pair("bingli", bingli));
    generalobj.push_back( Pair("pufang", pufang));
    generalobj.push_back( Pair("cefang", cefang));
    //洗髓需要的信息
    int full_val = m_belong_to.m_charData.m_level * 10 + 50;
    if (m_belong_to.m_charData.m_vip >= iWashFullValVIP)
    {
        full_val *= 2;
    }
    generalobj.push_back( Pair("max", full_val) );
    generalobj.push_back( Pair("oldbrave", m_wash_str) );
    generalobj.push_back( Pair("oldwisdom", m_wash_int) );
    generalobj.push_back( Pair("oldgovern", m_wash_tong) );
    double per = m_belong_to.m_charData.queryExtraData(char_data_type_normal, char_data_wash_per) / 10000.0;
    generalobj.push_back( Pair("per", per*100.0) );
    json_spirit::Object wash_star_info;
    if (m_wash_star.get())
    {
        wash_star_info.push_back( Pair("star", m_wash_star->id));
        wash_star_info.push_back( Pair("type", tmp_type));
        int newV = getQualityWashValue(m_baseGeneral->m_quality, m_wash_star->value);
        wash_star_info.push_back( Pair("value", newV));
        generalobj.push_back( Pair("wash_star_info", wash_star_info));
    }
    if (m_wash_next_star.get())
    {
        wash_star_info.clear();
        wash_star_info.push_back( Pair("star", m_wash_next_star->id));
        wash_star_info.push_back( Pair("type", tmp_type));
        int newV = getQualityWashValue(m_baseGeneral->m_quality, m_wash_next_star->value);
        wash_star_info.push_back( Pair("value", newV));
        wash_star_info.push_back( Pair("need_score", m_wash_next_star->need_score));
        generalobj.push_back( Pair("wash_next_star_info", wash_star_info));
    }

    generalobj.push_back( Pair("growRate", m_chengzhang));
    generalobj.push_back( Pair("growRate_max", iChengZhangMax[m_color]));
    generalobj.push_back( Pair("rebornLv", rebornLevel(m_color, m_chengzhang)));
    generalobj.push_back( Pair("rebornOpen", m_belong_to.m_charData.m_rebornOpen));
    //if (m_baseGeneral.get())
    {
        m_baseGeneral->toObj(generalobj);
        generalobj.push_back( Pair("nickname", m_baseGeneral->m_nickname));
        generalobj.push_back( Pair("nickname_active", b_nickname));
        json_spirit::Object treasure;
        if (m_baseGeneral->m_baowu_type != 0)
        {
            treasure.push_back( Pair("name", m_baseGeneral->m_baowu));
            treasure.push_back( Pair("level", m_baowu_level));
            treasure.push_back( Pair("add", m_baowu_add));
            treasure.push_back( Pair("type", m_baseGeneral->m_baowu_type));
            treasure.push_back( Pair("spic", m_baseGeneral->m_baowu_spic));

            m_baoshis.showGeneralBaoshis(treasure);
        }
        generalobj.push_back( Pair("treasure", treasure));
    }
    generalobj.push_back( Pair("equipt_used", m_equipments.getUsed() > 0));
    generalobj.push_back( Pair("baoshi_used", m_baoshis.getUsed() > 0));
    //if (m_baseSoldier.get())
    {
        m_baseSoldier->addObj("soldier", generalobj);
    }
    generalobj.push_back( Pair("wash_power", m_wash_power));
    generalobj.push_back( Pair("reborn_power", m_reborn_power));
    //总战力
    generalobj.push_back( Pair("total_power", getAttack()));
}

//武将信息存盘
int CharGeneralData::Save()
{
    if (m_changed)
    {
        std::string strsqlgenius = "";
#if 0
        for(int i = 0; i < iGeniusMaxNum; ++i)
        {
            if (m_genius[i] == 0)
                break;
            int genius = m_genius[i];
            if (m_genius_lock[i])
            {
                genius += 1000;
            }
            if (strsqlgenius == "")
            {
                strsqlgenius = "genius1="+LEX_CAST_STR(genius);
            }
            else
                strsqlgenius = strsqlgenius + ",genius" + LEX_CAST_STR(i + 1) + "=" + LEX_CAST_STR(genius);
        }
        if (strsqlgenius != "")
        {
            strsqlgenius += ",";
        }
#endif
        //保存到数据库
        InsertSaveDb("update char_generals set " + strsqlgenius + "level=" + LEX_CAST_STR(m_level)
                    + ",color=" + LEX_CAST_STR(m_color)
                    + ",state=" + LEX_CAST_STR(m_state)
                    + ",delete_time=" + LEX_CAST_STR(m_delete_time)
                    + ",fac_a=" + LEX_CAST_STR(m_chengzhang)
                    + ",reborn_point=" + LEX_CAST_STR(m_reborn_point)
                    //+ ",fac_b=" + LEX_CAST_STR(m_chengzhang[1])
                    //+ ",fac_a_max=" + LEX_CAST_STR(m_chengzhang_max[0])
                    //+ ",fac_b_max=" + LEX_CAST_STR(m_chengzhang_max[1])
                    + ",add_level=" + LEX_CAST_STR(m_add)
                    + ",add_str=" + LEX_CAST_STR(m_wash_str)
                    + ",add_int=" + LEX_CAST_STR(m_wash_int)
                    + ",add_tong=" + LEX_CAST_STR(m_wash_tong)
                    + ",baowu_level=" + LEX_CAST_STR(m_baowu_level)
                    + ",reborn_times=" + LEX_CAST_STR(m_reborn_times)
                    + ",wash_times=" + LEX_CAST_STR(m_wash_times)
                    + ",nickname=" + LEX_CAST_STR(b_nickname)
                    + " where id=" + LEX_CAST_STR(m_id)
        );
        //cout << "*****************save generalid=" << m_id << endl;
        m_changed = false;
    }
    return 0;
}

//更新屬性
void CharGeneralData::updateAttribute()
{
    if (m_baseGeneral.get())
    {
        GeneralTypeData* bg = m_baseGeneral.get();
        //天赋作用，属性点加12%
        int add_tong = (int)m_add, add_str = (int)m_add, add_int = (int)m_add;
        if (bg->m_new_tianfu.m_more_tong)
        {
            add_tong = (int)((100+bg->m_new_tianfu.m_more_tong)*m_add/100);
        }
        if (bg->m_new_tianfu.m_more_int)
        {
            add_int = (int)((100+bg->m_new_tianfu.m_more_int)*m_add/100);
        }
        if (bg->m_new_tianfu.m_more_str)
        {
            add_str = (int)((100+bg->m_new_tianfu.m_more_str)*m_add/100);
        }
        m_str = m_baseGeneral->base_str + add_str + m_wash_str;
        m_int = m_baseGeneral->base_int + add_int + m_wash_int;
        m_tongyu = m_baseGeneral->base_tongyu + add_tong + m_wash_tong;
        //洗髓星级加成
        if (m_wash_star.get())
        {
            int newV = getQualityWashValue(m_baseGeneral->m_quality, m_wash_star->value);
            switch (m_baseGeneral->m_tianfu)
            {
                case 1:
                    m_tongyu += (newV);
                    break;
                case 2:
                    m_int += (newV);
                    break;
                case 3:
                    m_str += (newV);
                    break;
                default:
                    break;
            }
        }
        m_changed = true;
        updateNewAttack();
        Save();
    }
}

void CharGeneralData::addToList(base_genius Genius, int type)
{
    if (type == 1)
    {
        std::list<combatSpeSkill>::iterator it = m_more_damage_skills.begin();
        while(it != m_more_damage_skills.end())
        {
            if ((*it).extra > Genius.genius_val)
            {
                ++it;
            }
            else
            {
                break;
            }
        }
        combatSpeSkill tmp;
        tmp.id = Genius.id;
        tmp.name = Genius.cname;
        tmp.type = type;
        tmp.extra = Genius.genius_val;
        tmp.chance = Genius.genius_per * 10;
        m_more_damage_skills.insert(it,tmp);
    }
    else if(type == 2)
    {
        std::list<combatSpeSkill>::iterator it = m_attack_skills.begin();
        while(it != m_attack_skills.end())
        {
            if ((*it).extra > Genius.genius_val)
            {
                ++it;
            }
            else
            {
                break;
            }
        }
        combatSpeSkill tmp;
        tmp.id = Genius.id;
        tmp.name = Genius.cname;
        tmp.type = type;
        tmp.extra = Genius.genius_val;
        tmp.chance = Genius.genius_per * 10;
        m_attack_skills.insert(it,tmp);
    }
}

//更新天赋加成
void CharGeneralData::updateGeniusAttribute()
{
    m_more_damage_skills.clear();
    m_attack_skills.clear();
#if 0
    memset(&m_combat_attr.m_sub_damage, 0, sizeof(combatAttribute));
    for (int i = 0; i < iGeniusMaxNum; ++i)
    {
        if (m_genius[i] < 1)
        {
            continue;
        }
        base_genius Genius = geniusMgr::getInstance()->GetGenius(m_genius[i]);
        m_combat_attr.m_enable = true;
        switch (Genius.genius_type)
        {
            //伤害增减
            case genius_type_bubing_fangyu:
                m_combat_attr.m_sub_damage_from[0] += Genius.genius_val;
                break;
            case genius_type_gongbing_fangyu:
                m_combat_attr.m_sub_damage_from[1] += Genius.genius_val;
                break;
            case genius_type_qibing_fangyu:
                m_combat_attr.m_sub_damage_from[3] += Genius.genius_val;
                break;
            case genius_type_moushi_fangyu:
                m_combat_attr.m_sub_damage_from[2] += Genius.genius_val;
                break;
            case genius_type_jixie_fangyu:
                m_combat_attr.m_sub_damage_from[4] += Genius.genius_val;
                break;
            case genius_type_putong_fangyu:
                m_combat_attr.m_sub_damage[0] += Genius.genius_val;
                break;
            case genius_type_celue_fangyu:
                m_combat_attr.m_sub_damage[1] += Genius.genius_val;
                break;
            case genius_type_bubing_shanghai:
                m_combat_attr.m_more_damage_to[0] += Genius.genius_val;
                break;
            case genius_type_gongbing_shanghai:
                m_combat_attr.m_more_damage_to[1] += Genius.genius_val;
                break;
            case genius_type_qibing_shanghai:
                m_combat_attr.m_more_damage_to[3] += Genius.genius_val;
                break;
            case genius_type_moushi_shanghai:
                m_combat_attr.m_more_damage_to[2] += Genius.genius_val;
                break;
            case genius_type_jixie_shanghai:
                m_combat_attr.m_more_damage_to[4] += Genius.genius_val;
                break;
            case genius_type_BGQ_shanghai:
                m_combat_attr.m_more_damage_to[0] += Genius.genius_val;
                m_combat_attr.m_more_damage_to[1] += Genius.genius_val;
                m_combat_attr.m_more_damage_to[3] += Genius.genius_val;
                break;
            case genius_type_MJ_shanghai:
                m_combat_attr.m_more_damage_to[2] += Genius.genius_val;
                m_combat_attr.m_more_damage_to[4] += Genius.genius_val;
                break;
            //特殊效果
            case genius_type_baoji:
                m_combat_attr.m_special_attack[special_attack_baoji][0]    = Genius.genius_val;
                m_combat_attr.m_special_attack[special_attack_baoji][1]    = 50;
                break;
            case genius_type_dodge:
                m_combat_attr.m_special_attack[special_attack_dodge][0]    = Genius.genius_val;
                break;
            case genius_type_parry:
                m_combat_attr.m_special_attack[special_attack_parry][0]    = Genius.genius_val;
                break;
            case genius_type_shipo:
                m_combat_attr.m_special_attack[special_attack_shipo][0]    = Genius.genius_val;
                break;
            case genius_type_chaos:
                m_combat_attr.m_special_attack[special_attack_chaos][0]    = Genius.genius_val;
                break;
            case genius_type_weihe:
                m_combat_attr.m_special_attack[special_attack_weihe][0]    = Genius.genius_val;
                break;
            case genius_type_BAndD:
                m_combat_attr.m_special_attack[special_attack_baoji][0]    = Genius.genius_val;
                m_combat_attr.m_special_attack[special_attack_baoji][1]    = 50;
                m_combat_attr.m_special_attack[special_attack_dodge][0]    = Genius.genius_val;
                break;
            case genius_type_PAndS:
                m_combat_attr.m_special_attack[special_attack_parry][0]    = Genius.genius_val;
                m_combat_attr.m_special_attack[special_attack_shipo][0]    = Genius.genius_val;
                break;
            case genius_type_CAndW:
                m_combat_attr.m_special_attack[special_attack_chaos][0]    = Genius.genius_val;
                m_combat_attr.m_special_attack[special_attack_weihe][0]    = Genius.genius_val;
                break;
            case genius_type_baoji_d:
                m_combat_attr.m_special_resist[special_attack_baoji] = Genius.genius_val;
                break;
            case genius_type_dodge_d:
                m_combat_attr.m_special_resist[special_attack_dodge] = Genius.genius_val;
                break;
            case genius_type_parry_d:
                m_combat_attr.m_special_resist[special_attack_parry] = Genius.genius_val;
                break;
            case genius_type_shipo_d:
                m_combat_attr.m_special_resist[special_attack_shipo] = Genius.genius_val;
                break;
            case genius_type_chaos_d:
                m_combat_attr.m_special_resist[special_attack_chaos] = Genius.genius_val;
                break;
            case genius_type_weihe_d:
                m_combat_attr.m_special_resist[special_attack_weihe] = Genius.genius_val;
                break;
            case genius_type_BAndD_d:
                m_combat_attr.m_special_resist[special_attack_baoji] = Genius.genius_val;
                m_combat_attr.m_special_resist[special_attack_dodge] = Genius.genius_val;
                break;
            case genius_type_PAndS_d:
                m_combat_attr.m_special_resist[special_attack_parry] = Genius.genius_val;
                m_combat_attr.m_special_resist[special_attack_shipo] = Genius.genius_val;
                break;
            case genius_type_CAndW_d:
                m_combat_attr.m_special_resist[special_attack_chaos] = Genius.genius_val;
                m_combat_attr.m_special_resist[special_attack_weihe] = Genius.genius_val;
                break;
            //伤害转换
            case genius_type_attack_times:
                addToList(Genius, 1);
                break;
            case genius_type_attack_type:
                addToList(Genius, 2);
                break;
        }
    }
    m_changed = true;
    Save();
#endif
    return;
}

//洗髓系統公告
void CharGeneralData::broadWashMsg(int ret)
{
    if (m_baseGeneral.get())
    {
        std::string notify_msg = "";
        if (3 == ret)
        {
            notify_msg = strWashYellow;
            str_replace(notify_msg, "$P", MakeCharNameLink(m_belong_to.m_charData.m_name));
            str_replace(notify_msg, "$H", m_baseGeneral->m_name);
            //std::string link_name = NameToLink(m_cid,m_id,m_baseGeneral->m_name);
            str_replace(notify_msg, "$h", m_link_name);
        }
        else if (4 == ret)
        {
            notify_msg = strWashRed;
            str_replace(notify_msg, "$P", MakeCharNameLink(m_belong_to.m_charData.m_name));
            str_replace(notify_msg, "$H", m_baseGeneral->m_name);
            //std::string link_name = NameToLink(m_cid,m_id,m_baseGeneral->m_name);
            str_replace(notify_msg, "$h", m_link_name);
        }
        else if (5 == ret)
        {
            notify_msg = strWashPurple;
            str_replace(notify_msg, "$P", MakeCharNameLink(m_belong_to.m_charData.m_name));
            str_replace(notify_msg, "$H", m_baseGeneral->m_name);
            //std::string link_name = NameToLink(m_cid,m_id,m_baseGeneral->m_name);
            str_replace(notify_msg, "$h", m_link_name);
        }
        if (notify_msg != "")
        {
            #if 1
            GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
            #else
            GeneralDataMgr::getInstance()->broadCastToEveryone(notify_msg,3,5000);
            #endif
        }
    }
}

//重生系統公告
void CharGeneralData::broadRebornMsg(int ret)
{
    if (m_baseGeneral.get())
    {
        std::string notify_msg = "";
        //char chengzhang[1024];
        //sprintf(chengzhang,"%.2f", m_chengzhang);
        if (ret < 4)
        {
            notify_msg = strRebornMsg1;
            str_replace(notify_msg, "$P", MakeCharNameLink(m_belong_to.m_charData.m_name));
            str_replace(notify_msg, "$H", m_color_link);
            str_replace(notify_msg, "$R", LEX_CAST_STR(ret));
        }
        else if (ret < 7)
        {
            notify_msg = strRebornMsg2;
            str_replace(notify_msg, "$P", MakeCharNameLink(m_belong_to.m_charData.m_name));
            str_replace(notify_msg, "$H", m_color_link);
            str_replace(notify_msg, "$R", LEX_CAST_STR(ret));
        }
        else
        {
            notify_msg = strRebornMsg3;
            str_replace(notify_msg, "$P", MakeCharNameLink(m_belong_to.m_charData.m_name));
            str_replace(notify_msg, "$H", m_color_link);
            str_replace(notify_msg, "$R", LEX_CAST_STR(ret));
        }
        if (notify_msg != "")
        {
            #if 1
            GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
            #else
            GeneralDataMgr::getInstance()->broadCastToEveryone(notify_msg,3,5000);
            #endif
        }
    }
}

std::string CharGeneralData::NameToLink()
{
    return m_link_name;
}

//移除所有宝石
void CharGeneralData::removeAllBaoshi()
{

}

//穿上装备
int CharGeneralData::equip(int slot, int eid)
{
    //cout<<"CharGeneralData::equip(),slot:"<<(int)slot<<",eid:"<<eid<<endl;
    boost::shared_ptr<iItem> eqq = m_belong_to.m_charData.m_bag.getEquipItemById(eid);
    //没这件装备
    if (!eqq.get())// || eq->getContainer != &(m_belong_to.m_charData.m_bag))
    {
        //cout<<"not find this equipment in bag."<<endl;
        return HC_ERROR;
    }
    EquipmentData* eq = dynamic_cast<EquipmentData*>(eqq.get());
    //装备等级是否满足要求
    if (eq->baseEq->needLevel > m_level)
    {
        return HC_ERROR_NOT_ENOUGH_GENERAL_LEVEL;
    }
    //slot是否正确
    if (slot > equip_slot_max || slot < equip_ring)
    {
        slot = eq->type;        //现在 类型就是装备位置
    }

    //只能在背包中
    if (eq->getContainer() != &m_belong_to.m_charData.m_bag)
    {
        return HC_ERROR;
    }
    //从背包中移除
    m_belong_to.m_charData.m_bag.removeItem(eq->getSlot());

    //原先的位置上是否有装备
    int add_pre = 0, add2_pre = 0, add_now = 0, add2_now = 0;
    boost::shared_ptr<iItem> itm = m_equipments.getItem(slot);
    if (itm.get())
    {
        //cout<<"CharGeneralData::equip(), remove item slot:"<<(int)slot<<",eid:"<<eid<<endl;
        m_equipments.removeItem(slot);
        m_belong_to.m_charData.m_bag.addItem(itm);
        EquipmentData* ed = dynamic_cast<EquipmentData*>(itm.get());
        ed->Save();
        add_pre = ed->value + ed->addValue;
        add2_pre = ed->value2 + ed->addValue2;
    }

    //cout<<"m_equipments.addItem(itm)"<<endl;
    //装备上
    m_equipments.addItem(slot, eqq);
    EquipmentData* ed_now = dynamic_cast<EquipmentData*>(eqq.get());
    add_now = ed_now->value + ed_now->addValue;
    add2_now = ed_now->value2 + ed_now->addValue2;

    updateEquipmentEffect();
    eq->Save();

    //影響戰力
    m_belong_to.m_charData.m_weapon_attack_change = true;
    //通知战斗属性变化
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "equip_change") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("type", ed_now->type) );
    obj.push_back( Pair("add_pre", add_pre) );
    obj.push_back( Pair("add2_pre", add2_pre) );
    obj.push_back( Pair("add_now", add_now) );
    obj.push_back( Pair("add2_now", add2_now) );
    m_belong_to.m_charData.sendObj(obj);
    return HC_SUCCESS;
}

//卸下装备
int CharGeneralData::unequip(int slot)
{
    //cout<<"CharGeneralData::unequip(),slot:"<<(int)slot<<",gid:"<<m_id<<endl;
    //包裹满了
    if (m_belong_to.m_charData.m_bag.isFull())
    {
        return HC_ERROR_BACKPACK_FULL_NO_UNEQIPT;
    }
    if (slot > equip_slot_max
        || slot < equip_ring)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<iItem> itm = m_equipments.getItem(slot);
    if (!itm.get() || itm->getType() != iItem_type_equipment)
    {
        //if (itm.get())
        //{
        //    cout<<"item type "<<itm->getType()<<endl;
        //}
        //cout<<"CharGeneralData::unequip(),slot:"<<(int)slot<<",return HC_ERROR"<<endl;
        return HC_ERROR;
    }
    int add_pre = 0, add2_pre = 0;
    EquipmentData* eq = dynamic_cast<EquipmentData*>(itm.get());
    m_equipments.removeItem(slot);
    m_belong_to.m_charData.m_bag.addItem(itm);
    eq->Save();
    add_pre = eq->value + eq->addValue;
    add2_pre = eq->value2 + eq->addValue2;

    updateEquipmentEffect();
    //影響戰力
    m_belong_to.m_charData.m_weapon_attack_change = true;
    //通知战斗属性变化
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "equip_change") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("type", eq->type) );
    obj.push_back( Pair("add_pre", add_pre) );
    obj.push_back( Pair("add2_pre", add2_pre) );
    obj.push_back( Pair("add_now", 0) );
    obj.push_back( Pair("add2_now", 0) );
    m_belong_to.m_charData.sendObj(obj);
    return HC_SUCCESS;
}

//移除所有装备
int CharGeneralData::removeAllEquipment()
{
    for (size_t i = 1; i <= equip_slot_max; ++i)
    {
        boost::shared_ptr<iItem> itm = m_equipments.removeItem(i);
        if (itm.get())
        {
            if (0 == m_belong_to.m_charData.m_bag.addItem(itm))
            {
                json_spirit::Object robj;
                m_belong_to.m_charData.sellEquipment(itm->getId(), robj);
            }
            else
            {
                itm->Save();
            }
        }
    }

    //影響戰力
    //m_belong_to.m_charData.m_weapon_attack_change = true;
    //新战力
    //equip_change = true;
    updateEquipmentEffect();
    return HC_SUCCESS;
}

//身上装备的简单列表
void CharGeneralData::getList(json_spirit::Array& elist)
{
    //cout<<"CharGeneralData::getList(),gid:"<<m_id<<endl;
    for (size_t i = 1; i <= equip_slot_max; ++i)
    {
        json_spirit::Object obj;
        boost::shared_ptr<iItem> itm = m_equipments.getItem(i);
        if (itm.get())    //装备在身上的
        {
            EquipmentData* eq = dynamic_cast<EquipmentData*>(itm.get());
            if (eq->quality > 0)
            {
                obj.push_back( Pair("have_equip", true) );
                //cout<<"slot "<<i<<": type "<<eq->type<<", id "<<eq->id<<endl;
                obj.push_back( Pair("id", eq->id) );
                obj.push_back( Pair("type", eq->type) );
                obj.push_back( Pair("quality", eq->quality) );
                obj.push_back( Pair("up_quality", eq->up_quality) );
                obj.push_back( Pair("level", eq->qLevel) );
                obj.push_back( Pair("addNums", eq->getvalue()) );
                if (eq->getvalue2())
                {
                    obj.push_back( Pair("addNums2", eq->getvalue2()) );
                }
                obj.push_back( Pair("slot", i) );
                if (eq->baseEq.get())
                {
                    obj.push_back( Pair("name", eq->name()) );
                    obj.push_back( Pair("spic", eq->baseid) );
                }
                equipment_scroll* sp = Singleton<equipment_scroll_mgr>::Instance().getScrollBySrcId(eq->baseid);
                if (sp && sp->m_equipment->needLevel <= m_level && m_belong_to.m_charData.m_bag.getGemCount(sp->m_gem_id) > 0)
                {
                    if (canUpgradeEquipment(sp, m_belong_to.m_charData.m_bag))
                    {
                        obj.push_back( Pair("canUp", 2) );
                    }
                    else
                    {
                        obj.push_back( Pair("canUp", 1) );
                    }
                }
            }
            else
            {
                obj.push_back( Pair("have_equip", true) );
                //cout<<"slot "<<i<<": type "<<eq->type<<", id "<<eq->id<<endl;
                obj.push_back( Pair("id", eq->id) );
                obj.push_back( Pair("quality", eq->quality) );
                obj.push_back( Pair("up_quality", eq->up_quality) );
                obj.push_back( Pair("level", eq->qLevel) );
                obj.push_back( Pair("addNums", eq->getvalue()) );
                if (eq->getvalue2())
                {
                    obj.push_back( Pair("addNums2", eq->getvalue2()) );
                }
                obj.push_back( Pair("slot", i) );
                if (eq->baseEq.get())
                {
                    obj.push_back( Pair("name", eq->name()) );
                    obj.push_back( Pair("spic", eq->baseid) );
                }
                m_belong_to.m_charData.m_bag.getBestBagEquipments(m_belong_to.m_charData.m_level, i, obj);
            }
        }
        else
        {
            m_belong_to.m_charData.m_bag.getBestBagEquipments(m_belong_to.m_charData.m_level, i, obj);
            //boost::shared_ptr<baseEquipment> eq = GeneralDataMgr::getInstance()->GetBaseEquipment(i);
            //obj.push_back( Pair("name", eq->name) );
            //obj.push_back( Pair("type", eq->type) );
            //obj.push_back( Pair("spic", eq->baseid) );
            obj.push_back( Pair("slot", i) );
            obj.push_back( Pair("have_equip", false) );
        }
        elist.push_back(obj);
    }
}


void CharGeneralData::updateBaoshiAttr()
{
    m_combat_attr.clear();
    m_combat_attr.enable();
    m_combat_attr += m_baseSoldier->m_combat_attribute;
    for (int i = 1; i <= m_baoshis.m_size; ++i)
    {
        if (m_baoshis.m_bagslot[i-1].get() && m_baoshis.m_bagslot[i-1]->getType() == iItem_type_baoshi)
        {
            newBaoshi* p = dynamic_cast<newBaoshi*>(m_baoshis.m_bagslot[i-1].get());
            m_combat_attr += *p;
        }
    }
    //新战力
    baoshi_change = true;
    updateNewAttack();
    //cout<<"updateBaoshiAttr()"<<endl;
    //m_combat_attr.print(true);
}

void CharGeneralData::updateChengzhangStar()
{
    m_chengzhang_star = GeneralDataMgr::getInstance()->GetBaseChengzhangStarByValue(m_chengzhang);
    if (m_chengzhang_star.get())
    {
        m_chengzhang_next_star = GeneralDataMgr::getInstance()->GetBaseChengzhangStarByStar(m_chengzhang_star->id + 1);
    }
    else
    {
        m_chengzhang_next_star = GeneralDataMgr::getInstance()->GetBaseChengzhangStarByStar(1);
    }
}

void CharGeneralData::updateWashStar(bool act)
{
    int score = m_wash_str + m_wash_int + m_wash_tong;
    int pre_star = 0, now_star = 0;
    if (m_wash_star.get())
        pre_star = m_wash_star->id;
    m_wash_star = GeneralDataMgr::getInstance()->GetBaseWashStarByValue(score);
    if (m_wash_star.get())
    {
        m_wash_next_star = GeneralDataMgr::getInstance()->GetBaseWashStarByStar(m_wash_star->id + 1);
        now_star = m_wash_star->id;
    }
    else
    {
        m_wash_next_star = GeneralDataMgr::getInstance()->GetBaseWashStarByStar(1);
    }
    if (act && now_star > pre_star)
    {
        //act统计
        act_to_tencent(&(m_belong_to.m_charData),act_new_wash_up,now_star);
    }
    //支线任务
    if (now_star > pre_star)
    {
        m_belong_to.m_charData.m_trunk_tasks.updateTask(task_wash_star, now_star);
    }
}

void CharGeneralData::updateNewAttack()
{
    //武将
    if (general_change && m_baseGeneral.get() && m_baseSoldier.get())
    {
        general_change = false;
        m_general_score = 0;
        m_general_power = 0;
        /**评分**/
        //三围评分
        int tmp = (m_baseGeneral->base_str + m_baseGeneral->base_int + m_baseGeneral->base_tongyu);
        if (m_belong_to.m_charData.m_level < 11)
        {
            m_general_score = (tmp / 4);
        }
        else if(m_belong_to.m_charData.m_level < 40)
        {
            m_general_score = (tmp / 6);
        }
        else if(m_belong_to.m_charData.m_level < 62)
        {
            m_general_score = (tmp / 14);
        }
        else
        {
            m_general_score = (int)(tmp / 57.4);
        }

        /**战力=攻击*2 +普防+策防+兵力加成**/
        //基础三围战力
        int attack = 0, pufang = 0, cefang = 0, hp = 0;
        AttributeToAttack(m_baseGeneral->base_tongyu, m_baseGeneral->base_int, m_baseGeneral->base_str, m_baseSoldier->m_damage_type, attack, pufang, cefang, hp);
        m_general_power += m_belong_to.m_charData.buff_attack(m_baseSoldier->m_damage_type, attack, hp, pufang, cefang);
        //天赋战力
        attack = 0, pufang = 0, cefang = 0, hp = 0;
        int _str = 0, _int = 0, _tongyu = 0;
        if (m_baseGeneral->m_new_tianfu.m_more_tong)
        {
            _tongyu = (int)(m_baseGeneral->m_new_tianfu.m_more_tong*m_add/100);
        }
        if (m_baseGeneral->m_new_tianfu.m_more_int)
        {
            _int = (int)(m_baseGeneral->m_new_tianfu.m_more_int*m_add/100);
        }
        if (m_baseGeneral->m_new_tianfu.m_more_str)
        {
            _str = (int)(m_baseGeneral->m_new_tianfu.m_more_str*m_add/100);
        }
        AttributeToAttack(_tongyu, _int, _str, m_baseSoldier->m_damage_type, attack, pufang, cefang, hp);
        m_general_power += m_belong_to.m_charData.buff_attack(m_baseSoldier->m_damage_type, attack, hp, pufang, cefang);
        //必杀技战力
        if (m_baseGeneral->m_speSkill.get())
        {
            m_general_power += m_baseGeneral->m_speSkill->attack_score;
        }
    }
    //装备
    if (equip_change)
    {
        equip_change = false;
        m_equip_score = 0;
        m_equip_power = 0;
        /**评分**/
        int all_add = 0;
        int all_quality = 0;
        for (int i = 1; i <= equip_slot_max; ++i)
        {
            EquipmentData* eq = dynamic_cast<EquipmentData*>(m_equipments.getItem(i).get());
            if (eq)
            {
                all_add += eq->qLevel;
                all_quality += eq->up_quality;
            }
        }
        int tmp = 0;
        if (m_belong_to.m_charData.m_level < 30)
        {
            ;
        }
        else if(m_belong_to.m_charData.m_level < 50)
        {
            tmp = (int)((all_quality / 6.0 - 1.0) * 20.0);
        }
        else if(m_belong_to.m_charData.m_level < 60)
        {
            tmp = (int)((all_quality / 6.0 - 2.0) * 20.0);
        }
        else if(m_belong_to.m_charData.m_level < 70)
        {
            tmp = (int)((all_quality / 6.0 - 3.0) * 20.0);
        }
        else if(m_belong_to.m_charData.m_level < 80)
        {
            tmp = (int)((all_quality / 6.0 - 4.0) * 20.0);
        }
        else if(m_belong_to.m_charData.m_level < 90)
        {
            tmp = (int)((all_quality / 6.0 - 5.0) * 20.0);
        }
        else
        {
            tmp = (int)((all_quality / 6.0 - 6.0) * 20.0);
        }
        if (tmp < 0)
            tmp = 0;
        m_equip_score += tmp;
        m_equip_score += (int)(all_add / (double)m_belong_to.m_charData.m_level / 6.0 * 80.0);

        /**战力**/
        m_equip_power += m_belong_to.m_charData.buff_attack(m_baseSoldier->m_damage_type, m_attack, m_hp, m_pufang, m_cefang);
    }
    //洗髓
    if (wash_change && m_baseSoldier.get())
    {
        wash_change = false;
        m_wash_power = 0;
        /**战力**/
        //三围战力=攻击普防策防兵力加成
        //基础三围战力
        int attack = 0, pufang = 0, cefang = 0, hp = 0;
        AttributeToAttack(m_wash_tong, m_wash_int, m_wash_str, m_baseSoldier->m_damage_type, attack, pufang, cefang, hp);
        m_wash_power += m_belong_to.m_charData.buff_attack(m_baseSoldier->m_damage_type, attack, hp, pufang, cefang);
    }
    //宝石
    if (baoshi_change && m_baseSoldier.get())
    {
        baoshi_change = false;
        m_baoshi_power = 0;
        /**战力**/
        int tmp_baoshi_type = 0;
        switch (m_baseSoldier->m_damage_type)
        {
            case act_wuli_attack:
            {
                tmp_baoshi_type = 10;
                break;
            }
            case celue_damage:
            default:
            {
                tmp_baoshi_type = 11;
                break;
            }
        }
        for (int i = 1; i <= m_baoshis.m_size; ++i)
        {
            if (m_baoshis.m_bagslot[i-1].get() && m_baoshis.m_bagslot[i-1]->getType() == iItem_type_baoshi)
            {
                int attack = 0, pufang = 0, cefang = 0, hp = 0;
                newBaoshi* p = dynamic_cast<newBaoshi*>(m_baoshis.m_bagslot[i-1].get());
                //特效宝石=属性等级平方/100
                if (p->m_base.type < 9)
                {
                    m_baoshi_power += (p->value() * p->value() / 100);
                }
                else if(p->m_base.type == 9)//怒气宝石*10
                {
                    m_baoshi_power += (p->value() * 10);
                }
                else if(p->m_base.type == tmp_baoshi_type)//攻击
                {
                    attack += p->value();
                }
                else if(p->m_base.type == 12)//普防
                {
                    pufang += p->value();
                }
                else if(p->m_base.type == 13)//策防
                {
                    cefang += p->value();
                }
                else if(p->m_base.type == 14)//血量
                {
                    hp += p->value();
                }
                m_baoshi_power += m_belong_to.m_charData.buff_attack(m_baseSoldier->m_damage_type, attack, hp, pufang, cefang);
            }
        }
    }
    //重生
    if (reborn_change && m_baseSoldier.get())
    {
        reborn_change = false;
        m_reborn_power = 0;
        /**战力**/
        //三围战力=攻击普防策防兵力加成
        //基础三围战力
        int attack = 0, pufang = 0, cefang = 0, hp = 0;
        AttributeToAttack(m_add, m_add, m_add, m_baseSoldier->m_damage_type, attack, pufang, cefang, hp);
        if (m_chengzhang_star.get())
        {
            attack += m_chengzhang_star->gongji;
            pufang += m_chengzhang_star->fangyu;
            cefang += m_chengzhang_star->fangyu;
            hp += m_chengzhang_star->bingli;
        }
        m_reborn_power += m_belong_to.m_charData.buff_attack(m_baseSoldier->m_damage_type, attack, hp, pufang, cefang);
    }
    //兵魂
    m_soul_power = 0;
    if (m_baseSoldier.get() && m_belong_to.m_charData.m_soulOpen)
    {
        int attack = 0, pufang = 0, cefang = 0, hp = 0;
        boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(m_belong_to.m_charData);
        if (ct.get())
        {
            attack += (ct->_combatAttr.soul_add_attack(m_baseSoldier->m_base_type));
            hp += (ct->_combatAttr.soul_add_hp(m_baseSoldier->m_base_type));
            pufang += (ct->_combatAttr.soul_add_wufang(m_baseSoldier->m_base_type));
            cefang += (ct->_combatAttr.soul_add_cefang(m_baseSoldier->m_base_type));
            m_soul_power += m_belong_to.m_charData.buff_attack(m_baseSoldier->m_damage_type, attack, hp, pufang, cefang);
        }
    }

    m_jxl_power = 0;
    if (m_belong_to.m_charData.m_jxlOpen)
    {
        //攻击防御之类的信息
        int attack = 0, bingli = 0, pufang = 0, cefang = 0;
        if (act_wuli_attack == m_baseSoldier->m_damage_type)
        {
            attack = m_attack + m_combat_attr.skill_add(skill_add_pugong) + (2*m_str) + m_belong_to.m_charData.getPugong(true);
        }
        else
        {
            attack = m_attack + m_combat_attr.skill_add(skill_add_cegong) + (2*m_int) + m_belong_to.m_charData.getCegong(true);
        }
        bingli = m_hp + m_combat_attr.skill_add(skill_add_hp) + 3*m_tongyu + m_belong_to.m_charData.getBingli(true);
        pufang = m_pufang + m_combat_attr.skill_add(skill_add_pufang) + (7*m_str)/5 + m_belong_to.m_charData.getPufang(true);
        cefang = m_cefang + m_combat_attr.skill_add(skill_add_cefang) + (7*m_int)/5 + m_belong_to.m_charData.getCefang(true);
        //成长星级加成
        if (m_chengzhang_star.get())
        {
            attack += m_chengzhang_star->gongji;
            pufang += m_chengzhang_star->fangyu;
            cefang += m_chengzhang_star->fangyu;
            bingli += m_chengzhang_star->bingli;
        }
        if (m_belong_to.m_charData.m_soulOpen)
        {
            boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(m_belong_to.m_charData);
            if (ct.get())
            {
                attack += (ct->_combatAttr.soul_add_attack(m_baseSoldier->m_base_type));
                bingli += (ct->_combatAttr.soul_add_hp(m_baseSoldier->m_base_type));
                pufang += (ct->_combatAttr.soul_add_wufang(m_baseSoldier->m_base_type));
                cefang += (ct->_combatAttr.soul_add_cefang(m_baseSoldier->m_base_type));
            }
        }
        m_jxl_power = m_belong_to.m_charData.m_jxl_buff.total_buff_attr.get_attack(attack, bingli, pufang, cefang);

        //cout<<"jxl power"<<m_jxl_power<<endl;
    }

    m_gsoul_power = 0;
    {
        int attack = 0, pufang = 0, cefang = 0, hp = 0;
        if (m_general_soul)
        {
            int quality = 0;
            if (m_color >= 0 && m_color <= 5)
            {
                quality = m_color;
            }
            attack = m_general_soul->total_attr[quality].attack;
            hp = m_general_soul->total_attr[quality].hp;
            pufang = m_general_soul->total_attr[quality].wufang;
            cefang = m_general_soul->total_attr[quality].cefang;
            m_gsoul_power = m_belong_to.m_charData.buff_attack(m_baseSoldier->m_damage_type, attack, hp, pufang, cefang);
        }
    }
    /*cout << "CharGeneralData::updateNewAttack!!!" << endl;
    cout << "gid=" << m_id << ",belong_to cid=" << m_cid << endl;
    cout << "general_score=" << m_general_score << endl;
    cout << "general_power=" << m_general_power << endl;
    cout << "equip_score=" << m_equip_score << endl;
    cout << "equip_power=" << m_equip_power << endl;
    cout << "wash_power=" << m_wash_power << endl;
    cout << "baoshi_power=" << m_baoshi_power << endl;
    cout << "reborn_power=" << m_reborn_power << endl;
    cout << "end!!!!!!!!!!!!" << endl;*/
}

//获取武将总战力
int CharGeneralData::getAttack()
{
    //总战力
    int total = m_general_power + m_equip_power + m_wash_power + m_baoshi_power + m_reborn_power + m_soul_power;
    switch (m_baseSoldier->m_damage_type)
    {
        case act_wuli_attack:
        {
            total += m_belong_to.m_charData.m_new_weapons.getNewPower_pu();
            break;
        }
        case celue_damage:
        default:
        {
            total += m_belong_to.m_charData.m_new_weapons.getNewPower_ce();
            break;
        }
    }
    total += m_belong_to.m_charData.m_horse.getNewPower();
    return total;
}

//获取现有状态
int CharStrongholdData::getStates(json_spirit::Array& states)
{
    return m_states.getStateInfo(states);
}

void CharStrongholdData::save_state()
{
    if (m_baseStronghold.get())
    {
        InsertSaveDb("UPDATE char_stronghold SET pos" + LEX_CAST_STR(m_baseStronghold->m_strongholdpos) + "=" + LEX_CAST_STR(m_state)
                            + " WHERE cid=" + LEX_CAST_STR(m_cid) + " AND stageid=" + LEX_CAST_STR(m_baseStronghold->m_stage_id) + " AND mapid=" + LEX_CAST_STR(m_baseStronghold->m_map_id));
    }
}

bool CharStageData::isGroupPassed(int group)
{
    for (int i = 0; i < 25; ++i)
    {
        if (m_stronghold[i].get() && m_stronghold[i]->m_baseStronghold.get() && m_stronghold[i]->m_baseStronghold->m_group == group)
        {
            if (m_stronghold[i]->m_state <= 0)
            {
                return false;
            }
        }
    }
    return true;
}

void CharStageData::groupPass(int group)
{
    m_cur_group = group + 1;
    openGroup(group + 1);
}

void CharStageData::openGroup(int group)
{
    for (int i = 0; i < 25; ++i)
    {
        if (m_stronghold[i].get() && m_stronghold[i]->m_baseStronghold.get() && m_stronghold[i]->m_baseStronghold->m_group == group)
        {
            if (m_stronghold[i]->m_state < 0)
            {
                m_stronghold[i]->m_state = 0;
                m_stronghold[i]->save_state();
            }
        }
    }
}

int CharStageData::curGroup()
{
    m_cur_group = 1;
    for (int i = 0; i < 25; ++i)
    {
        if (m_stronghold[i].get() && m_stronghold[i]->m_baseStronghold.get() && m_stronghold[i]->m_state >= 0)
        {
            if (m_stronghold[i]->m_baseStronghold->m_group > m_cur_group)
            {
                m_cur_group = m_stronghold[i]->m_baseStronghold->m_group;
            }
        }
    }
    return m_cur_group;
}

//关卡增加一条攻略
int strongholdRaiders::addRecords(const std::string& name, int level, uint64_t bid, int attack, int hurt)
{
    int archive = 0;
    if (iArchiveFirstStrongholdSuccess && m_first_report.bid == 0)
    {
        m_first_report.bid = bid;
        m_first_report.aLevel = level;
        m_first_report.aName = name;
        m_first_report.attack_time = time(NULL);
        m_first_report.attack = attack;
        m_first_report.hurt = hurt;
        archive = 1;
    }
    if (m_best_report.bid == 0
        || m_best_report.hurt >= hurt)
    {
        if (m_best_report.bid > 0 && m_best_report.bid != m_first_report.bid)
        {
            //顶出去的战报修改为不存档
            InsertSaveDb("update battle_records set archive=0 where id=" + LEX_CAST_STR(m_best_report.bid));
        }
        m_best_report.bid = bid;
        m_best_report.aLevel = level;
        m_best_report.aName = name;
        m_best_report.attack_time = time(NULL);
        m_best_report.attack = attack;
        m_best_report.hurt = hurt;
        archive = 1;
    }
    return archive;
}

void strongholdRaiders::load(const std::string& name, int level, uint64_t id, int attack, int hurt, time_t input)
{
    if (iArchiveFirstStrongholdSuccess && m_first_report.bid == 0)
    {
        m_first_report.bid = id;
        m_first_report.attack_time = input;
        m_first_report.hurt = hurt;
        m_first_report.aName = name;
        m_first_report.aLevel = level;
        m_first_report.attack = attack;
    }
    if (m_best_report.bid == 0
        || m_best_report.hurt > hurt
        || (m_best_report.hurt == hurt && m_best_report.attack_time < input))
    {
        m_best_report.bid = id;
        m_best_report.attack_time = input;
        m_best_report.hurt = hurt;
        m_best_report.aName = name;
        m_best_report.aLevel = level;
        m_best_report.attack = attack;
    }
}

void strongholdRaiders::getRadiers(json_spirit::Object& robj)
{
    if (m_first_report.bid > 0)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("id", m_first_report.bid) );
        obj.push_back( Pair("name", m_first_report.aName) );
        obj.push_back( Pair("level", m_first_report.aLevel) );
        obj.push_back( Pair("time", m_first_report.attack_time) );
        obj.push_back( Pair("attack", m_first_report.attack) );
        obj.push_back( Pair("hurt", m_first_report.hurt) );
        robj.push_back( Pair("first", obj) );
    }
    if (m_best_report.bid > 0)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("id", m_best_report.bid) );
        obj.push_back( Pair("name", m_best_report.aName) );
        obj.push_back( Pair("level", m_best_report.aLevel) );
        obj.push_back( Pair("time", m_best_report.attack_time) );
        obj.push_back( Pair("attack", m_best_report.attack) );
        obj.push_back( Pair("hurt", m_best_report.hurt) );
        robj.push_back( Pair("best", obj) );
    }
}

int StrongholdData::getAttack()
{
    int hp = 0, attack = 0, pufang = 0, cefang = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        json_spirit::Object generalobj;
        if (m_generals[i].get())
        {
            hp += m_generals[i]->m_hp;
            attack += m_generals[i]->m_attack;
            pufang += m_generals[i]->m_pufang;
            cefang += m_generals[i]->m_cefang;
        }
    }
    return (hp + pufang + cefang + attack * 2) * 10 / 100;
}

int CharTempoData::load(int cid, int load_map = 0)
{
    Query q(GetDb());
    std::string sqlcmd = "SELECT mapid,stageid,pos1,pos2,pos3,pos4,pos5,pos6,pos7,pos8,pos9,pos10,pos11,pos12,pos13,pos14,pos15,pos16,pos17,pos18,pos19,pos20,pos21,pos22,pos23,pos24,pos25 FROM char_stronghold WHERE cid=" + LEX_CAST_STR(cid);
    if (load_map > 0)
    {
        sqlcmd += " and mapid=" + LEX_CAST_STR(load_map);
    }
    q.get_result(sqlcmd);
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        CharMapData *pMap = NULL;
        int mapid = q.getval();
        //是否存在该地图数据，没有则创建
        if (CharMapsData[mapid].get())
        {
            pMap = CharMapsData[mapid].get();
        }
        else
        {
            pMap = new CharMapData;
            CharMapsData[mapid].reset(pMap);
        }
        int stageid = q.getval();
        CharStageData *pStage = NULL;
        //是否存在该场景，没有则创建
        if ((*pMap)[stageid].get())
        {
            pStage = (*pMap)[stageid].get();
        }
        else
        {
            pStage = new CharStageData;

            boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(mapid);
            if (bm.get() && stageid >= 1 && stageid <= 3)
            {
                pStage->m_baseStage = bm->stages[stageid-1];
            }
            (*pMap)[stageid].reset(pStage);
        }
        int m_finished = 0, m_size = 25;
        for (size_t i = 0; i < 25; ++i)
        {
            int m_state = q.getval();
            if (-2 == m_state)
            {
                m_size = i;
                break;
            }
            boost::shared_ptr<StrongholdData> bstronghold = GeneralDataMgr::getInstance()->GetStrongholdData(mapid, stageid, i+1);
            if (!bstronghold.get())
            {
                ERR();
                cout<<"cid:"<<cid<<","<<mapid<<","<<stageid<<","<<(i+1)<<endl;
                break;
            }
            pStage->m_stronghold[i].reset(new CharStrongholdData(cid, bstronghold->m_id, (3 * mapid -1), bstronghold->m_stateNum));
            CharStrongholdData* pshd = pStage->m_stronghold[i].get();
            pshd->m_cid = cid;
            pshd->m_baseStronghold = bstronghold;
            pshd->m_state = m_state;
            if (m_state > 0)
            {
                ++m_finished;
                if (m_charData.m_currentStronghold < bstronghold->m_id)
                {
                    m_charData.m_currentStronghold = bstronghold->m_id;
                }
                m_charData.m_cur_stage = bstronghold->m_stage_id;
            }
        }
        pStage->m_cid = cid;
        pStage->m_finished = m_finished;
    }
    q.free_result();

    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.begin();
    while (it != CharMapsData.end())
    {
        if (it->first == load_map || 0 == load_map)
        {
            CharMapData::iterator it1 = it->second->begin();
            while (it1 != it->second->end())
            {
                boost::shared_ptr<CharStageData> psd = it1->second;
                if (psd.get() && psd->m_baseStage.get())
                {
                    psd->curGroup();
                }
                else
                {
                    ERR();
                }
                ++it1;
            }
        }
        ++it;
    }
    return 0;
}

int CharTempoData::update(int stronghold, bool bBroad = true)
{
    boost::shared_ptr<StrongholdData> stronghold_data = GeneralDataMgr::getInstance()->GetStrongholdData(stronghold);
    StrongholdData *bShold = stronghold_data.get();
    if (NULL == bShold)
    {
        ERR();
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.find(bShold->m_map_id);
    if (it != CharMapsData.end())
    {
        //所在的地图数据
        CharMapData* md = it->second.get();
        if (md == NULL)
        {
            ERR();
            return HC_ERROR;
        }
        CharMapData::iterator itm = md->find(bShold->m_stage_id);
        if (itm != md->end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            //所在场景
            CharStageData* cStage = itm->second.get();
            if (NULL == cStage)
            {
                ERR();
                cout<<m_cid<<"CharStageData NULL,strongholdid,map,stage,"<<stronghold<<","<<bShold->m_map_id<<","<<bShold->m_stage_id<<endl;
                return HC_ERROR;
            }
            CharStrongholdData* pCShold = cStage->m_stronghold[bShold->m_strongholdpos - 1].get();
            if (pCShold == NULL)
            {
                ERR();
                cout<<m_cid<<"CharStrongholdData NULL,strongholdid,map,stage,"<<stronghold<<","<<bShold->m_map_id<<","<<bShold->m_stage_id<<endl;
                return HC_ERROR;
            }
            //取得方阵是几介的
            int side = 0;
            switch(cStage->m_baseStage->size)
            {
                case 9:
                    side = 3;
                    break;
                case 16:
                    side = 4;
                    break;
                case 25:
                default:
                    side = 5;
                    break;
            }

            //如果是第一次攻打成功这个关卡，则场景完成次数加1
            if (pCShold->m_state == 0)
            {
                ++cStage->m_finished;
            }
            //设置总攻击次数
            pCShold->m_state++;
            pCShold->save_state();

#if 0
            //下方如果没越界则开放
            if ((bShold->m_strongholdpos + side) <= cStage->m_baseStage->size
                && cStage->m_stronghold[bShold->m_strongholdpos + side - 1].get()
                && cStage->m_stronghold[bShold->m_strongholdpos + side - 1]->m_state == -1)
            {
                cStage->m_stronghold[bShold->m_strongholdpos + side - 1]->m_state = 0;
                cStage->m_stronghold[bShold->m_strongholdpos + side - 1]->save_state();
            }
            //不是右边界则紧挨着的也开放
            if (bShold->m_strongholdpos % side != 0
                && cStage->m_stronghold[bShold->m_strongholdpos].get()
                && cStage->m_stronghold[bShold->m_strongholdpos]->m_state == -1)
            {
                cStage->m_stronghold[bShold->m_strongholdpos]->m_state = 0;
                cStage->m_stronghold[bShold->m_strongholdpos]->save_state();
            }
#else
            if (pCShold->m_state == 1)
            {
                int group = pCShold->m_baseStronghold->m_group;
                //如果同一个组的全部打完，开放下一组
                if (cStage->isGroupPassed(group))
                {
                    cStage->groupPass(group);
                }
            }
#endif
            //如果是本地图的最后一个关卡,并且是第一次攻打成功
            if (bBroad && bShold->m_strongholdpos == cStage->m_baseStage->size && bShold->m_baseStage->id == 3 && pCShold->m_state == 1)
            {
                /*********** 广播 ************/
                std::string msg = strSystemPassStage;
                std::string tmp = "";
                str_replace(msg, "$W", MakeCharNameLink(m_charData.m_name));
                str_replace(msg, "$T", bShold->m_name);
                boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(bShold->m_map_id + 1);
                if (bm.get())
                {
                    tmp = strSystemPassStage2;
                    str_replace(tmp, "$N", bm->name);
                }
                str_replace(msg, "$M", tmp);
                //只向所在地图广播
                GeneralDataMgr::getInstance()->broadCastSysMapMsg(msg, -1, bShold->m_map_id);

                //祝贺
                Singleton<relationMgr>::Instance().postCongradulation(m_charData.m_id, CONGRATULATION_PASS_STAGE, bShold->m_map_id, bShold->m_stage_id);
            }
            //第一次占领
            //if (pCShold->m_state == 1)
            {
                //看是否要开放下一个场景
                if (++itm != (*md).end())
                {
                    CharStageData* pNextStage = itm->second.get();
                    if (pNextStage == NULL || !pNextStage->m_stronghold[0].get() || !pNextStage->m_baseStage.get())
                    {
                        ERR();
                        return HC_ERROR;
                    }
                    //下个场景没开，等级足够就开放下一个场景
                    if (pNextStage->m_stronghold[0]->m_state < 0 &&
                        pNextStage->m_baseStage->openLevel <= bShold->m_level)
                    {
                        m_charData.m_cur_stage = pNextStage->m_baseStage->id;
                        //开放本地图的下一个场景
                        pNextStage->openGroup(1);
                    }
                }
                //更新任务
                m_charData.updateTask(task_attack_stronghold, stronghold);
            }
            //第一次占领的一些功能开放
            if (pCShold->m_state == 1)
            {
                if (bShold->m_guide_id > 0)
                {
                    //新手引导
                    m_charData._checkGuide(bShold->m_guide_id);
                }
                if (m_charData.m_currentStronghold < stronghold)
                {
                    m_charData.m_currentStronghold = stronghold;
                    //cout<<m_charData.m_id<<"-> update open "<<stronghold<<endl;
                    m_charData.updateOpen();
                    int ret = taskMgr::getInstance()->acceptTrunkTask(m_charData,m_charData.m_currentStronghold);
                    if (ret == HC_SUCCESS)
                    {
                        //通知玩家任务完成或者有变化
                        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_charData.m_name);
                        if (account.get())
                        {
                            json_spirit::Object robj;
                            robj.push_back( Pair("cmd", "getCurTask") );
                            robj.push_back( Pair("s", 200) );
                            taskMgr::getInstance()->getTaskInfo(m_charData, 0, robj);
                            account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
                        }
                    }

                    //通知将星录亮
                    if (Singleton<jxl_mgr>::Instance().needNotifyStronghold(m_charData.m_currentStronghold))
                    {
                        m_charData.notifyEventState(top_level_event_jxl, 1, 0);
                    }
                }
                //有些关卡第一次打败后送装备
                switch (stronghold)
                {
                    //第十个关卡系统赠送六件白色装备
                    case 10:
                        {
                            std::string msg;
                            m_charData.addEquipt(121, msg, false);
                            m_charData.addEquipt(122, msg, false);
                            m_charData.addEquipt(123, msg, false);
                            m_charData.addEquipt(124, msg, false);
                            m_charData.addEquipt(125, msg, false);
                            m_charData.addEquipt(126, msg, false);
                        }
                        break;
                    //击败江东小校 Lv33,仓库内赠送道具辨才符1个
                    case 65:
                        {
                            int err_code = 0;
                            m_charData.addTreasure(treasure_type_wjbs, 1, err_code);
                        }
                        break;
                }
                //阵型开放
                if (bShold->m_open_zhen > 0)
                {
                    m_charData.m_zhens.setLevel(bShold->m_open_zhen, bShold->m_open_zhen_level);
                }
                //武将上限改变
                if (bShold->m_general_limit > m_charData.m_general_limit)
                {
                    m_charData.m_general_limit = bShold->m_general_limit;
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "notify") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair("type", notify_msg_new_general_limit) );
                    obj.push_back( Pair("nums", bShold->m_general_limit) );
                    m_charData.sendObj(obj);
                }
                if (stronghold == iFirstUpGeneralStronghold
                    || stronghold == iSecondUpGeneralStronghold)
                {
                    if (stronghold == iFirstUpGeneralStronghold)
                    {
                        m_charData.m_up_generals = iDefaultUpGenerals + 1;
                    }
                    else
                    {
                        m_charData.m_up_generals = iDefaultUpGenerals + 2;
                    }
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "notify") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair("type", notify_msg_more_up_zhen) );
                    obj.push_back( Pair("nums", m_charData.m_up_generals) );
                    m_charData.sendObj(obj);
                }
            }
        }
        else
        {
            ERR();
        }
    }
    return HC_SUCCESS;
}

//插入角色关卡进度
int CharTempoData::InitCharTempo(int mapid)
{
    //插入玩家关卡进度
    boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(mapid);
    if (!bm.get())
    {
        ERR();
        return HC_ERROR;
    }
    Query q(GetDb());
    for (int stageid = 1; stageid <= 3; ++stageid)
    {
        if (bm->stages[stageid-1].get())
        {
            int size = bm->stages[stageid-1]->size;
            std::string sqlcmd = "replace into char_stronghold set cid=" + LEX_CAST_STR(m_cid) + ",mapid=" + LEX_CAST_STR(mapid) + ",stageid=" + LEX_CAST_STR(stageid);
            for (int i = 0; i < size; ++i)
            {
                if (bm->stages[stageid-1]->_baseStrongholds[i].get())
                {
                    if (stageid == 1 && bm->stages[stageid-1]->_baseStrongholds[i]->m_group == 1)
                    {
                        sqlcmd += ",pos1=0";    //第一组直接可以打
                    }
                    else
                    {
                        sqlcmd += ",pos" + LEX_CAST_STR(i+1) + "=-1";
                    }
                }
            }

            if (!q.execute(sqlcmd))
            {
                DB_ERROR(q);
            }
        }
    }
    //加载
    load(m_cid, mapid);
    return HC_SUCCESS;
}

int CharTempoData::Save()
{
    Query q(GetDb());
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.begin();
    while (it != CharMapsData.end())
    {
        CharMapData::iterator it1 = it->second->begin();
        while (it1 != it->second->end())
        {
            boost::shared_ptr<CharStageData> psd = it1->second;
            if (psd.get() && psd->m_baseStage.get())
            {
                for (size_t i = 0; i < (size_t)psd->m_baseStage->size; ++i)
                {
                    if (psd->m_stronghold[i].get() && psd->m_stronghold[i]->m_baseStronghold.get())
                    {
                        InsertSaveDb("UPDATE char_stronghold SET pos" + LEX_CAST_STR(i+1) + "=" + LEX_CAST_STR(psd->m_stronghold[i]->m_state)
                            + " WHERE cid=" + LEX_CAST_STR(m_cid) + " AND stageid=" + LEX_CAST_STR(it1->first) + " AND mapid=" + LEX_CAST_STR(it->first));
                    }
                }
            }
            ++it1;
        }
        ++it;
    }
    return 0;
}

int CharTempoData::get_stage_finish_loot(int mapid, int stageid , json_spirit::Object& robj)
{
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.find(mapid);
    if (it != CharMapsData.end())
    {
        //所在的地图数据
        CharMapData* md = it->second.get();
        if (md == NULL)
        {
            ERR();
            return HC_ERROR;
        }
        CharMapData::iterator itm = md->find(stageid);
        if (itm != md->end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            //所在场景
            CharStageData* cStage = itm->second.get();
            if (NULL == cStage)
            {
                ERR();
                cout<<m_cid<<"CharStageData NULL"<<endl;
                return HC_ERROR;
            }
            //场景最后关卡
            CharStrongholdData* pCShold = cStage->m_stronghold[cStage->m_baseStage->size - 1].get();
            if (pCShold == NULL)
            {
                ERR();
                cout<<m_cid<<"CharStrongholdData NULL"<<endl;
                return HC_ERROR;
            }
            //攻打成功
            if (pCShold->m_state > 0)
            {
                //是否已经领取过了
                int idx = char_data_stage_award_start + (mapid-1)*3 + stageid;
                int get = m_charData.queryExtraData(char_data_type_normal, idx);
                if (get)
                {
                    return HC_ERROR;
                }
                //设置已经领取
                m_charData.setExtraData(char_data_type_normal, idx, 1);

                // 通关奖励
                std::list<Item> items;
                int id = (mapid - 1) * 3 + stageid;
                //cout << "get_stage_finish_loot !!! mapid=" << mapid << ",stageid=" << stageid << endl;
                lootMgr::getInstance()->getStageLoots(id, items, robj);
                //给东西
                giveLoots(&m_charData, items, 0, m_charData.m_level, 0, NULL, NULL, true, give_stage_loot);

                //领完通关奖励的引导
                if (cStage->m_baseStage->stage_finish_guide > 0)
                {
                    m_charData._checkGuide(cStage->m_baseStage->stage_finish_guide);
                }
                return HC_SUCCESS;
            }
        }
        else
        {
            ERR();
        }
    }
    return HC_ERROR;
}

bool CharTempoData::check_stage_finish(int mapid, int stageid)
{
    //是否已经领取过了
    int idx = char_data_stage_award_start + (mapid-1)*3 + stageid;
    int get = m_charData.queryExtraData(char_data_type_normal, idx);
    if (get)
    {
        return false;
    }
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.find(mapid);
    if (it != CharMapsData.end())
    {
        //所在的地图数据
        CharMapData* md = it->second.get();
        if (md == NULL)
        {
            ERR();
            return false;
        }
        CharMapData::iterator itm = md->find(stageid);
        if (itm != md->end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            //所在场景
            CharStageData* cStage = itm->second.get();
            if (NULL == cStage)
            {
                ERR();
                cout<<m_cid<<"CharStageData NULL"<<mapid<<","<<stageid<<endl;
                return false;
            }
            //场景最后关卡
            CharStrongholdData* pCShold = cStage->m_stronghold[cStage->m_baseStage->size - 1].get();
            if (pCShold == NULL)
            {
                ERR();
                cout<<m_cid<<"CharStrongholdData NULL"<<mapid<<","<<stageid<<endl;
                return false;
            }
            //第一次攻打成功
            if (pCShold->m_state > 0)
            {
                return true;
            }
        }
        else
        {
            ERR();
        }
    }
    return false;
}

bool CharTempoData::check_stronghold_can_sweep(int mapid, int stageid, int pos)
{
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.find(mapid);
    if (it != CharMapsData.end())
    {
        //所在的地图数据
        CharMapData* md = it->second.get();
        if (md == NULL)
        {
            return false;
        }
        CharMapData::iterator itm = md->find(stageid);
        if (itm != md->end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            //所在场景
            CharStageData* cStage = itm->second.get();
            if (NULL == cStage)
            {
                return false;
            }
            //场景最后关卡
            CharStrongholdData* pCShold = cStage->m_stronghold[pos-1].get();
            if (pCShold == NULL)
            {
                return false;
            }
            if (pCShold->m_state > 0)
            {
                return true;
            }
        }
        else
        {
            ERR();
        }
    }
    return false;
}

//从数据库中加载武将
int CharTotalGenerals::Load()
{
    Query q(GetDb());
    q.get_result("SELECT id,gid,level,color,state,delete_time,fac_a,reborn_point,add_level,add_str,add_int,add_tong,baowu_level,reborn_times,wash_times,nickname,soul FROM char_generals WHERE cid=" + LEX_CAST_STR(m_cid) + " order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<CharGeneralData> gd;
        gd.reset(new CharGeneralData(m_charData, *this));
        gd->m_id = q.getval();
        gd->m_cid = m_cid;
        gd->m_stype = 0;
        gd->m_gid = q.getval();
        gd->m_baseGeneral = GeneralDataMgr::getInstance()->GetBaseGeneral(gd->m_gid);
        if (!gd->m_baseGeneral.get())
        {
            continue;
        }
        gd->m_level = q.getval();
        gd->m_color = q.getval();
        gd->m_state = q.getval();
        gd->m_delete_time = q.getval();
        gd->m_chengzhang = q.getnum();
        gd->m_reborn_point = q.getval();
        //gd->m_chengzhang[1] = q.getnum();
        //gd->m_chengzhang_max[0] = q.getnum();
        //gd->m_chengzhang_max[1] = q.getnum();
        gd->m_add = q.getnum();
        gd->m_wash_str = q.getval();
        gd->m_wash_int = q.getval();
        gd->m_wash_tong = q.getval();
        gd->m_baowu_level = q.getval();
        gd->m_reborn_times = q.getval();
        gd->m_wash_times = q.getval();
        gd->m_genius_count = 0;
#if 0
        // 加载天赋列表
        for (int i = 0; i < iGeniusMaxNum; i++)
        {
            int genuis = q.getval();
            //千位数用来表示是否锁定
            if (genuis > 1000)
            {
                gd->m_genius_lock[i] = true;
            }
            genuis = genuis % 1000;
            if (genuis)
            {
                ++gd->m_genius_count;
            }
            gd->m_genius[i] = genuis;
        }
#endif
        gd->b_nickname = q.getval();

        int soul_level = q.getval();
        gd->m_general_soul = Singleton<charGeneralSoulMgr>::Instance().getSoul(soul_level);
        //gd->updateGeniusAttribute();

        gd->m_link_name = strGeneralLink;
        str_replace(gd->m_link_name, "$G", LEX_CAST_STR(gd->m_id));
        str_replace(gd->m_link_name, "$C", LEX_CAST_STR(m_cid));
        str_replace(gd->m_link_name, "$N", gd->m_baseGeneral->m_name);
        gd->m_color_link = "<font color=\"#ffffff\">" + gd->m_link_name + "</font>";
        gd->SetColor(true);

        gd->m_stype = gd->m_baseGeneral->m_stype;
        gd->m_spic = gd->m_baseGeneral->m_spic;
        gd->m_baowu = gd->m_baseGeneral->m_baowu;
        gd->m_baowu_type = gd->m_baseGeneral->m_baowu_type;
        gd->m_baowu_add = gd->m_baowu_level > 0 ? (gd->m_baseGeneral->m_baowu_baseval + (gd->m_baowu_level - 1)* gd->m_baseGeneral->m_baowu_addperlev) : 0;

        //gd->m_tongyu = gd->m_baseGeneral->base_tongyu;
        //gd->m_str = gd->m_baseGeneral->base_str;
        //gd->m_int = gd->m_baseGeneral->base_int;

        if (gd->m_chengzhang == 0.00)
        {
            gd->m_chengzhang = gd->m_baseGeneral->m_base_chengzhang;
        }
        gd->updateChengzhangStar();

        //天赋作用，属性点加12%
        int add_tong = (int)gd->m_add, add_str = (int)gd->m_add, add_int = (int)gd->m_add;
        if (gd->m_baseGeneral->m_new_tianfu.m_more_tong)
        {
            add_tong = (int)((100+gd->m_baseGeneral->m_new_tianfu.m_more_tong)*gd->m_add/100);
        }
        if (gd->m_baseGeneral->m_new_tianfu.m_more_int)
        {
            add_int = (int)((100+gd->m_baseGeneral->m_new_tianfu.m_more_int)*gd->m_add/100);
        }
        if (gd->m_baseGeneral->m_new_tianfu.m_more_str)
        {
            add_str = (int)((100+gd->m_baseGeneral->m_new_tianfu.m_more_str)*gd->m_add/100);
        }

        gd->m_str = gd->m_baseGeneral->base_str + add_str + gd->m_wash_str;
        gd->m_int = gd->m_baseGeneral->base_int + add_int + gd->m_wash_int;
        gd->m_tongyu = gd->m_baseGeneral->base_tongyu + add_tong + gd->m_wash_tong;

        gd->updateWashStar();
        //洗髓星级加成
        if (gd->m_wash_star.get())
        {
            int newV = getQualityWashValue(gd->m_baseGeneral->m_quality, gd->m_wash_star->value);
            switch (gd->m_baseGeneral->m_tianfu)
            {
                case 1:
                    gd->m_tongyu += (newV);
                    break;
                case 2:
                    gd->m_int += (newV);
                    break;
                case 3:
                    gd->m_str += (newV);
                    break;
                default:
                    break;
            }
        }

        gd->m_baseSoldier = GeneralDataMgr::getInstance()->GetBaseSoldier(gd->m_stype);

        if (gd->m_baseSoldier.get() == NULL)
        {
            cout<<"error,stype "<<gd->m_stype<<",general type "<<gd->m_gid<<endl;
            continue;
        }
        gd->m_combat_attr = gd->m_baseSoldier->m_combat_attribute;
        if (gd->m_state == 0)
        {
            m_generals[gd->m_id] = gd;
        }
        else
        {
            m_fired_generals[gd->m_id] = gd;
        }
        gd->m_equipments.gd = gd;
        gd->m_baoshis.gd = gd;

        //根据等级设置可以镶嵌的宝石个数
        gd->updateBaoshiCount();
        gd->updateNewAttack();
    }
    q.free_result();

    //加载武将宝物上面的宝石及未镶嵌的宝石

    //加载武将身上的装备

    //INFO("*************** CharEquipments::load *********************");
    q.get_result("select ce.id,ce.base_id,be.type,ce.gid,ce.qLevel,ce.orgAttr,ce.addAttr,ce.orgAttr2,ce.addAttr2,be.quality,be.up_quality,ce.slot from char_equipment as ce left join base_equipment as be on ce.base_id=be.id where ce.gid>0 and ce.state=0 and ce.cid=" + LEX_CAST_STR(m_cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int baseid = q.getval();
        uint16_t type = q.getval();
        int gid = q.getval();

        boost::shared_ptr<CharGeneralData> gd = GetGenral(gid);
        CharGeneralData* pg = gd.get();
        if (!pg)
        {
            continue;
        }

        boost::shared_ptr<iItem> e;
        EquipmentData* eq = new EquipmentData(baseid, id);
        e.reset(eq);

        eq->cid = m_cid;
        eq->id = id;
        eq->baseid = baseid;
        eq->type = type;
        eq->qLevel = q.getval();
        eq->value = q.getval();
        eq->addValue = q.getval();
        eq->value2 = q.getval();
        eq->addValue2 = q.getval();
        eq->quality = q.getval();
        eq->up_quality = q.getval();
        uint8_t slot = q.getval();
        eq->price = eq->quality > 0 ? 50*eq->quality*(eq->qLevel+1)*(eq->qLevel+20) : 50*(eq->qLevel+1)*(eq->qLevel+20);
        if (eq->type >= 1 && eq->type <= equip_slot_max)
        {
        }
        else
        {
            continue;
        }

        if (slot >= 1 && slot <= pg->m_equipments.size()
            && pg->m_equipments.getItem(slot).get())
        {
            pg->m_equipments.addItem(e);
            //保存
            eq->Save();
        }
        else
        {
            pg->m_equipments.addItem(slot, e);
        }
        eq->baseEq = GeneralDataMgr::getInstance()->GetBaseEquipment(eq->baseid);
    }
    q.free_result();

    //INFO("*************** CharEquipments::load *********************");
    q.get_result("select id,type,gid,slot,level,nums from char_baoshi where state=0 and cid=" + LEX_CAST_STR(m_cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int type = q.getval();
        int gid = q.getval();
        int slot = q.getval();
        int level = q.getval();
        int nums = q.getval();
        //int exp = q.getval();

        baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(type);
        if (!bbs)
        {
            continue;
        }
        boost::shared_ptr<iItem> e;
        newBaoshi* eq = new newBaoshi(id, *bbs, level, nums);
        e.reset(eq);

        Singleton<newBaoshiMgr>::Instance().registerBaoshi(id, e);

        boost::shared_ptr<CharGeneralData> gd;
        if (gid > 0)
        {
            gd = GetGenral(gid);
        }
        //cout<<"load char baoshi gid "<<gid<<endl;
        CharGeneralData* pg = gd.get();
        if (!pg)
        {
            //cout<<"load char baoshi g null "<<gid<<endl;
            //位置上已有东西
            if (slot >= 1 && slot <= m_charData.m_bag.size())
            {
                if (m_charData.m_bag.getItem(slot).get())
                {
                    slot = m_charData.m_bag.addItem(e);
                    //保存
                    e->Save();
                }
                else
                {
                    m_charData.m_bag.addItem(slot, e);
                }
            }
            else
            {
                //cout<<"add baoshi to bag"<<endl;
                m_charData.m_bag.addItem(e);
                //保存
                e->Save();
            }
        }
        else
        {
            if (pg->m_baoshis.isFull())
            {
                pg->m_baoshis.addSize(1);
            }
            if (slot >= 1 && slot <= pg->m_baoshis.size()
                && pg->m_baoshis.getItem(slot).get())
            {
                pg->m_baoshis.addItem(e);
                //保存
                e->Save();
            }
            else
            {
                pg->m_baoshis.addItem(slot, e);
            }
            pg->m_combat_attr += (*eq);
        }
    }
    q.free_result();

    //装备/宝石效果更新下
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.begin();
    while (it != m_generals.end())
    {
        if (it->second.get())
        {
            CharGeneralData* cg = it->second.get();
            cg->updateEquipmentEffect();
            cg->updateBaoshiAttr();
        }
        ++it;
    }

    return 0;
}

//解雇武将
int CharTotalGenerals::Fire(int id)
{
    if (m_charData.m_zhens.Check(id))
    {
        return HC_ERROR_ZHEN;
    }
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.find(id);
    if (it != m_generals.end())
    {
        if (it->second.get())
        {
            //it->second->m_changed = true;
            //m_changed = true;
            it->second->m_state = 1;
            //it->second->m_delete_time = time(NULL) + 3600*72;   //3天的回购事件
            it->second->m_changed = true;
            it->second->Save();
            m_fired_generals[id] = it->second;

            //宝石移除
            //it->second->removeAllBaoshi();
            //移除所有装备
            //it->second->removeAllEquipment();

            m_generals.erase(it);

            //所有阵型的中的该武将自动下阵
            m_charData.m_zhens.Down(id);
            m_charData.NotifyZhenData();
        }
        return HC_SUCCESS;
    }
    else
    {
        return -1;
    }
}


//根据武将基础类型查找武将
int CharTotalGenerals::GetGeneralByType(int gtype)
{
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.begin();
    while (it != m_generals.end())
    {
        if (it->second.get() && it->second->m_gid == gtype)
        {
            return it->second->m_id;
        }
        ++it;
    }
    return 0;
}

//根据武将基础类型查找武将
int CharTotalGenerals::GetFiredGeneralByType(int gtype)
{
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_fired_generals.begin();
    while (it != m_fired_generals.end())
    {
        if (it->second.get() && it->second->m_gid == gtype)
        {
            return it->second->m_id;
        }
        ++it;
    }
    return 0;
}

//回购武将
int CharTotalGenerals::Buyback(int id)
{
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_fired_generals.find(id);
    if (it != m_fired_generals.end())
    {
        if (it->second.get())
        {
            //判断是否超过回购时间
            //if (it->second->m_delete_time < time(NULL))
            //{
                //return -2;
            //}
            //判断是否已经存在该武将
            if (GetGeneralByType(it->second->m_gid) > 0)
            {
                return HC_ERROR_HAS_GENERAL_ALREADY;
            }
            //武将上限判断
            if (m_generals.size() >= m_charData.m_general_limit)
            {
                return HC_ERROR_TOO_MUCH_GENERALS;
            }
            //it->second->m_changed = true;
            //m_changed = true;
            it->second->m_state = 0;
            it->second->m_delete_time = 0;
            it->second->m_changed = true;
            it->second->Save();
            m_generals[id] = it->second;
            m_fired_generals.erase(it);

            //为了老玩家
            Singleton<new_event_mgr>::Instance().addGeneralCheck(m_cid, m_generals[id]->m_gid);
            return 200;
        }
        else
        {
            return -4;
        }
    }
    else
    {
        return -1;
    }
}

//武将重生
int CharTotalGenerals::Reborn(int id, json_spirit::Object& robj, int fast)
{
    if (fast > 0 && m_charData.m_vip < iOnekeyRebronVip)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.find(id);
    if (it != m_generals.end())
    {
        if (it->second.get() && it->second->m_level >= rebornLevel(it->second->m_color, it->second->m_chengzhang))
        {
#if 0
            int done = m_charData.queryExtraData(char_data_type_daily, char_data_free_reborn);
            if ((m_charData.m_vip >= iRebornFreeVip && done < iRebornFreeTimes))
            {
                m_charData.setExtraData(char_data_type_daily, char_data_free_reborn, done + 1);
                robj.push_back( Pair("msg", strRebornFree) );
            }
            else
            {
                if (m_charData.addGold(-iRebornGold) < 0)
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                //金币消耗统计
                add_statistics_of_gold_cost(m_charData.m_id, m_charData.m_ip_address, iRebornGold, gold_cost_for_reborn);
            }
#endif

            int reborn_times = 0;
            if (fast)
            {
                //一键扣除金币
                int gold = fastRebornGold(it->second->m_chengzhang);
                if (m_charData.addGold(-gold) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                m_charData.NotifyCharData();
                //金币消耗统计
                add_statistics_of_gold_cost(m_charData.m_id, m_charData.m_ip_address, gold, gold_cost_for_reborn, m_charData.m_union_id, m_charData.m_server_id);
#ifdef QQ_PLAT
                gold_cost_tencent(&m_charData,gold,gold_cost_for_reborn);
#endif
            }
            else
            {
                int cost_point = iRebornPoint[0];
                if (it->second->m_chengzhang_star.get())
                {
                    cost_point = iRebornPoint[it->second->m_chengzhang_star->id];
                }
                //扣除重生点数
                if (it->second->m_reborn_point >= cost_point)
                {
                    it->second->m_reborn_point -= cost_point;
                }
                else
                {
                    return HC_ERROR_NOT_ENOUGH_REBORN_POINT;
                }
            }

            json_spirit::Object g;
            if (it->second->m_baseGeneral.get())
                g.push_back( Pair("name", it->second->m_baseGeneral->m_name) );
            g.push_back( Pair("level", it->second->m_level) );
            g.push_back( Pair("quality", it->second->m_color) );
            g.push_back( Pair("growRate", it->second->m_chengzhang));
            g.push_back( Pair("brave", it->second->m_str));
            g.push_back( Pair("wisdom", it->second->m_int));
            g.push_back( Pair("govern", it->second->m_tongyu));
            robj.push_back( Pair("old_ginfo", g) );
            double old_chengzhang = it->second->m_chengzhang;

            ++it->second->m_reborn_times;
            ++reborn_times;
            int old_fac = improveChengzhang(it->second->m_chengzhang);
            //成长率上限
            if (it->second->m_chengzhang > iChengZhangMax[it->second->m_color])
            {
                it->second->m_chengzhang = iChengZhangMax[it->second->m_color];
            }

            //普通重生一次性重生到底
            if (0 == fast)
            {
                while (it->second->m_level >= rebornLevel(it->second->m_color, it->second->m_chengzhang))
                {
                    int cost_point = iRebornPoint[0];
                    if (it->second->m_chengzhang_star.get())
                    {
                        cost_point = iRebornPoint[it->second->m_chengzhang_star->id];
                    }
                    //扣除重生点数
                    if (it->second->m_reborn_point >= cost_point)
                    {
                        it->second->m_reborn_point -= cost_point;
                        ++it->second->m_reborn_times;
                        ++reborn_times;
                        improveChengzhang(it->second->m_chengzhang);
                        //成长率上限
                        if (it->second->m_chengzhang > iChengZhangMax[it->second->m_color])
                        {
                            it->second->m_chengzhang = iChengZhangMax[it->second->m_color];
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            int now_fac = (int)it->second->m_chengzhang;
            //新点数进阶了一个整数公告
            if (now_fac > old_fac)
            {
                //重生祝贺，成长率到整数时
                Singleton<relationMgr>::Instance().postCongradulation(m_charData.m_id, CONGRATULATION_REBORN, id, 100 * it->second->m_chengzhang);
            }
            //成长率升星公告
            if (it->second->m_chengzhang_star.get())
            {
                int old_star = it->second->m_chengzhang_star->id;
                it->second->m_chengzhang_star = GeneralDataMgr::getInstance()->GetBaseChengzhangStarByValue(it->second->m_chengzhang);
                if (it->second->m_chengzhang_star.get() && it->second->m_chengzhang_star->id > old_star)
                {
                    it->second->broadRebornMsg(it->second->m_chengzhang_star->id);
                    it->second->m_chengzhang_next_star = GeneralDataMgr::getInstance()->GetBaseChengzhangStarByStar(it->second->m_chengzhang_star->id + 1);
                    //act统计
                    act_to_tencent(&m_charData,act_new_reborn_up, it->second->m_chengzhang_star->id);

                    //支线任务
                    m_charData.m_trunk_tasks.updateTask(task_reborn_star, it->second->m_chengzhang_star->id);
                }
            }
            else
            {
                it->second->m_chengzhang_star = GeneralDataMgr::getInstance()->GetBaseChengzhangStarByValue(it->second->m_chengzhang);
                if (it->second->m_chengzhang_star.get())
                {
                    it->second->broadRebornMsg(it->second->m_chengzhang_star->id);
                    it->second->m_chengzhang_next_star = GeneralDataMgr::getInstance()->GetBaseChengzhangStarByStar(it->second->m_chengzhang_star->id + 1);
                    //act统计
                    act_to_tencent(&m_charData,act_new_reborn_up, it->second->m_chengzhang_star->id);

                    //支线任务
                    m_charData.m_trunk_tasks.updateTask(task_reborn_star, it->second->m_chengzhang_star->id);
                }
            }
            if ((it->second->m_chengzhang - old_chengzhang) > 0.01)
            {
                //周排行活动
                int score = (int)((it->second->m_chengzhang - old_chengzhang) * 100);
                if (score > 0)
                    newRankings::getInstance()->updateEventRankings(m_charData.m_id,rankings_event_chengzhang,score);
            }
            //等级加成重新计算
            it->second->m_add = 0.0;
            for (int level = 2; level <= it->second->m_level; ++level)
            {
                double add = it->second->m_chengzhang;
                int temp = (int)(add * 100);
                it->second->m_add += (double)temp/100;
            }
            it->second->updateAttribute();
            m_charData.NotifyZhenData();
            //攻擊力變化
            m_charData.set_attack_change();
            //新战力
            it->second->general_change = true;
            it->second->reborn_change = true;

            g.clear();
            if (it->second->m_baseGeneral.get())
                g.push_back( Pair("name", it->second->m_baseGeneral->m_name) );
            g.push_back( Pair("level", it->second->m_level) );
            g.push_back( Pair("quality", it->second->m_color) );
            g.push_back( Pair("growRate", it->second->m_chengzhang));
            g.push_back( Pair("brave", it->second->m_str));
            g.push_back( Pair("wisdom", it->second->m_int));
            g.push_back( Pair("govern", it->second->m_tongyu));
            robj.push_back( Pair("new_ginfo", g) );
            //日常任务
            dailyTaskMgr::getInstance()->updateDailyTask(m_charData,daily_task_general_reborn,reborn_times);
            //act统计
            act_to_tencent(&m_charData,act_new_reborn, it->second->m_gid);

            //支线任务
            m_charData.m_trunk_tasks.updateTask(task_reborn, reborn_times);
        }
        else
            return HC_ERROR;
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

//武将重生
int CharTotalGenerals::Recover(int id, json_spirit::Object& robj)
{
#if 0
    if (m_charData.m_vip < iRebornVip)
        return HC_ERROR_MORE_VIP_LEVEL;
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.find(id);
    if (it != m_generals.end())
    {
        if (it->second.get() && it->second->m_level >= iRebornOpenLevel && (it->second->m_chengzhang[0] != it->second->m_chengzhang_max[0] || it->second->m_chengzhang[1] != it->second->m_chengzhang_max[1]))
        {
            it->second->m_chengzhang[0] = it->second->m_chengzhang_max[0];
            it->second->m_chengzhang[1] = it->second->m_chengzhang_max[1];
            it->second->m_color = 0;
            it->second->m_level = 1;
            it->second->m_add = 0.0;
            it->second->updateAttribute();
        }
        else
            return HC_ERROR;
    }
    else
    {
        return HC_ERROR;
    }
#endif
    return HC_SUCCESS;
}

//获得武将
int CharTotalGenerals::Add(int id, bool broad, int level, bool setFac, double fac_aa)
{
    bool need_fire = false;
    if (GetGeneralByType(id) > 0)
    {
        //need_fire = true;
        //所有武将只能获得唯一一个
        return 0;
    }

    //武将上限判断
    if (m_generals.size() >= m_charData.m_general_limit)
    {
        return -1;
    }

    boost::shared_ptr<GeneralTypeData> bgd = GeneralDataMgr::getInstance()->GetBaseGeneral(id);
    if (!bgd.get())
    {
        return 0;
    }
    GeneralTypeData* pbgd = bgd.get();
    double fac_a = setFac ? fac_aa : bgd->m_base_chengzhang;
    int color = pbgd->m_quality;
    int gid = GeneralDataMgr::getInstance()->newGeneralId();

    InsertSaveDb("insert into char_generals (id,cid,gid,level,color,fac_a) values ("
                    + LEX_CAST_STR(gid)
                    + "," + LEX_CAST_STR(m_cid)
                    + "," + LEX_CAST_STR(id)
                    + "," + LEX_CAST_STR(level)
                    + "," + LEX_CAST_STR(color)
                    + "," + LEX_CAST_STR(fac_a) + ")");

    boost::shared_ptr<CharGeneralData> gd;
    gd.reset(new CharGeneralData(m_charData, *this));
    gd->m_id = gid;
    gd->m_cid = m_cid;
    gd->m_stype = 0;
    gd->m_gid = id;
    gd->m_level = level;
    gd->m_tongyu = pbgd->base_tongyu;
    gd->m_str = pbgd->base_str;
    gd->m_int = pbgd->base_int;
    gd->m_color = color;
    gd->m_state = 0;
    gd->m_delete_time = 0;
    gd->m_chengzhang = bgd->m_base_chengzhang;
    gd->updateChengzhangStar();
    gd->m_reborn_point = 0;
    gd->m_add = 0;
    gd->m_wash_str = 0;
    gd->m_wash_int = 0;
    gd->m_wash_tong = 0;
    gd->updateWashStar();
    gd->m_reborn_times = 0;
    gd->m_wash_times = 0;

    gd->m_genius_count = 0;
#if 0
    for (int i = 0; i < iGeniusMaxNum; ++i)
    {
        gd->m_genius_lock[i] = false;
    }
#endif
    gd->b_nickname = 0;

    gd->m_baowu_level = 0;
    gd->m_stype = bgd->m_stype;
    gd->m_spic = bgd->m_spic;
    gd->m_baowu = bgd->m_baowu;
    gd->m_baowu_type = bgd->m_baowu_type;
    gd->m_baowu_add = gd->m_baowu_level > 0 ? (bgd->m_baowu_baseval + (gd->m_baowu_level - 1)* bgd->m_baowu_addperlev) : 0;
    gd->m_baseGeneral = bgd;
    gd->m_baseSoldier = GeneralDataMgr::getInstance()->GetBaseSoldier(gd->m_stype);
    if (bgd.get())
    {
        gd->m_link_name = strGeneralLink;
        str_replace(gd->m_link_name, "$G", LEX_CAST_STR(gd->m_id));
        str_replace(gd->m_link_name, "$C", LEX_CAST_STR(m_cid));
        str_replace(gd->m_link_name, "$N", gd->m_baseGeneral->m_name);
        gd->m_color_link = "<font color=\"#ffffff\">" + gd->m_link_name + "</font>";
        gd->SetColor(true);
    }
    m_generals[gd->m_id] = gd;
    gd->m_equipments.gd = gd;

    for (int i = 2; i <= level; ++i)
    {
        double add = gd->m_chengzhang;
        int temp = (int)(add * 100);
        gd->m_add += (double)temp/100;
    }

    gd->m_baoshis.gd = gd;
    //更新宝石孔数量
    gd->updateBaoshiCount();
    //更新屬性
    gd->updateAttribute();

    GeneralDataMgr::getInstance()->addGeneral(gid, m_cid);
    //解雇武将，把武将放到回购列表
    if (need_fire)
    {
        Fire(gid);
    }
    //更新任务
    m_charData.updateTask(task_get_general, id);
    //更新支线任务
    m_charData.m_trunk_tasks.updateTask(task_get_general, id);

    if (broad && gd->m_color > 0)
    {
        //广播招募信息
        std::string notify_msg = strBuyGeneral;
        str_replace(notify_msg, "$N", MakeCharNameLink(m_charData.m_name));
        str_replace(notify_msg, "$G", gd->m_color_link);
        GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
    }
    //每次招募橙色武将，好友来祝贺
    if (pbgd->m_quality >= 4)
    {
        Singleton<relationMgr>::Instance().postCongradulation(m_charData.m_id, CONGRATULATION_FRIST_QUALITY4_GENERAL, gid, 0);
    }
    else if (pbgd->m_quality >= 1)
    {
        if (m_charData.queryExtraData(char_data_type_daily, char_data_first_general_quality1 + pbgd->m_quality - 1))
        {
        }
        else
        {
            m_charData.setExtraData(char_data_type_daily, char_data_first_general_quality1 + pbgd->m_quality - 1, 1);
            Singleton<relationMgr>::Instance().postCongradulation(m_charData.m_id,
                                CONGRATULATION_FRIST_QUALITY1_GENERAL + pbgd->m_quality - 1,
                                gid, 0);
        }
    }
    Singleton<new_event_mgr>::Instance().addGeneral(m_cid, id);
    //七日目标
    Singleton<seven_Goals_mgr>::Instance().updateGoals(m_charData,m_charData.queryCreateDays(),goals_type_general,id);

    return gid;
}

bool CharTotalGenerals::CheckTreasureCanUp(int id)
{
    bool beCanUp = false;
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.find(id);
    if (it != m_generals.end())
    {
        if (it->second.get())
        {
            if (it->second->m_baowu_type != 0)
            {
                if (it->second->m_baowu_level < 10)
                {
                    beCanUp = true;
                }
            }
        }
    }
    return beCanUp;
}

int CharTotalGenerals::UpdateTreasure(std::string& general_name, std::string& baowu_name, int id)
{
    INFO( "updatetreasure id=" << id << endl);
    if (id != 0)
    {
        if (CheckTreasureCanUp(id))
        {
            std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.find(id);
            if (it != m_generals.end() && it->second.get())
            {
                boost::shared_ptr<GeneralTypeData> bgd = GeneralDataMgr::getInstance()->GetBaseGeneral(it->second->m_gid);
                if (bgd.get())
                {
                    it->second->m_baowu_level = it->second->m_baowu_level + 1;
                    it->second->m_baowu_add = (bgd->m_baowu_baseval + (it->second->m_baowu_level - 1)* bgd->m_baowu_addperlev);
                    general_name = bgd->m_name;
                    baowu_name = bgd->m_baowu;
                    it->second->m_changed = true;
                    //it->second->m_belong_to.m_changed = true;

                    //攻擊力變化
                    m_charData.set_attack_change();

                    it->second->Save();
                    return HC_SUCCESS;
                }
            }
        }
    }
    else
    {
        std::vector<int> generals;
        CharZhens& char_zhens = m_charData.GetZhens();
        std::map<int, boost::shared_ptr<ZhenData> >::iterator it = char_zhens.m_zhens.find(char_zhens.m_default_zhen);
        if (it != char_zhens.m_zhens.end())
        {
            if (it->second.get())
            {
                for (size_t i = 0; i < 9; ++i)
                {
                    if (it->second->m_generals[i] > 0 && CheckTreasureCanUp(it->second->m_generals[i]))
                    {
                        generals.push_back(it->second->m_generals[i]);
                    }
                }
            }
        }
        if (!generals.empty())
        {
            return UpdateTreasure(general_name,baowu_name,generals[my_random(0, generals.size() - 1)]);
        }
        else
        {
            std::map<int, boost::shared_ptr<CharGeneralData> >::iterator itc = m_generals.begin();
            while(itc != m_generals.end())
            {
                if (itc->second.get())
                {
                    if (itc->second->m_id > 0 && CheckTreasureCanUp(itc->second->m_id))
                    {
                        generals.push_back(itc->second->m_id);
                    }
                }
                ++itc;
            }
            if (!generals.empty())
            {
                return UpdateTreasure(general_name,baowu_name,generals[my_random(0, generals.size() - 1)]);
            }
        }
    }
    return HC_ERROR;
}

//删除武将
int CharTotalGenerals::deleteGenral(int id)
{
    if (id > 0)
    {
        //先解雇
        Fire(id);
        boost::shared_ptr<CharGeneralData> spG;

        //从解雇中删除
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_fired_generals.find(id);
        if (it != m_fired_generals.end())
        {
            spG = it->second;
            m_fired_generals.erase(it);
        }
        if (spG.get())
        {
            InsertSaveDb("delete from char_generals where id=" + LEX_CAST_STR(spG->m_id));
        }
        return 1;
    }
    return 0;
}

//修改武将属性
int CharTotalGenerals::modifyGeneral(int id, int t, int z, int y)
{
    boost::shared_ptr<CharGeneralData> spG = GetGenral(id);
    if (spG.get())
    {
        CharGeneralData* pg = spG.get();
        pg->m_wash_int = z;
        pg->m_wash_str = y;
        pg->m_wash_tong = t;
        //pg->SetColor();
        pg->updateAttribute();
        pg->updateWashStar();
        //攻擊力變化
        pg->m_belong_to.m_charData.set_attack_change();
        pg->Save();
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

//清除所有洗髓加点
int CharTotalGenerals::clearWash()
{
    for (std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.begin(); it != m_generals.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->m_wash_int = 0;
            it->second->m_wash_tong = 0;
            it->second->m_wash_str = 0;
            //if ((int)it->second->m_add >= 1)
            //{
            //    it->second->SetColor();
            //}
            //更新屬性
            it->second->updateAttribute();
            it->second->Save();
        }
    }
    for (std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_fired_generals.begin(); it != m_fired_generals.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->m_wash_int = 0;
            it->second->m_wash_tong = 0;
            it->second->m_wash_str = 0;
            //if ((int)it->second->m_add >= 1)
            //{
            //    it->second->SetColor();
            //}
            //更新屬性
            it->second->updateAttribute();
            it->second->Save();
        }
    }
    m_charData.set_attack_change();
    return 0;
}

//修改武将成长
int CharTotalGenerals::modifyGeneralGrowth(int id, double fac1)
{
    boost::shared_ptr<CharGeneralData> spG = GetGenral(id);
    if (spG.get())
    {
        CharGeneralData* pg = spG.get();
        pg->m_chengzhang = fac1;
        //等级加成重新计算
        pg->m_add = 0.0;
        for (int level = 2; level <= pg->m_level; ++level)
        {
            double add = pg->m_chengzhang;
            int temp = (int)(add * 100);
            pg->m_add += (double)temp/100;
        }
        //更新屬性
        pg->updateAttribute();
        pg->updateChengzhangStar();
        m_charData.NotifyZhenData();
        //攻擊力變化
        m_charData.set_attack_change();

        pg->Save();
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

//回购武将列表
std::map<int, boost::shared_ptr<CharGeneralData> >& CharTotalGenerals::GetFiredGeneralsList()
{
    #if 0
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_fired_generals.begin();
    while (it != m_fired_generals.end())
    {
        if(it->second.get())
        {
            if (it->second->m_delete_time < time(NULL))
            {
                //cout << "char_fired_general:" << it->second->m_id << " delete" << endl;
                InsertSaveDb("delete from char_generals where id=" + LEX_CAST_STR(it->second->m_id));
                m_fired_generals.erase(it);
                it = m_fired_generals.begin();
                continue;
            }
        }
        ++it;
    }
    #endif
    return m_fired_generals;
}

//根据id取武将
boost::shared_ptr<CharGeneralData> CharTotalGenerals::GetGenral(int id)
{
    if (id > 0)
    {
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.find(id);
        if (it != m_generals.end())
        {
            return it->second;
        }
        else
        {
            it = m_fired_generals.find(id);
            if (it != m_fired_generals.end())
            {
                return it->second;
            }
        }
    }
    else if (id == 0)
    {
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.begin();
        if (it != m_generals.end())
        {
            return it->second;
        }
    }
    boost::shared_ptr<CharGeneralData> gd;
    gd.reset();
    return gd;
}

//达到一定等级的武将数量
int CharTotalGenerals::getGeneralCounts(int level)
{
    int counts = 0;
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.begin();
    while (it != m_generals.end())
    {
        if (it->second.get())
        {
            CharGeneralData* cg = it->second.get();
            if (cg->m_level >= level)
            {
                ++counts;
            }
        }
        ++it;
    }
    return counts;
}

int CharTotalGenerals::GetGenralLevel(int gtype)
{
    int gid = GetGeneralByType(gtype);
    if (gid > 0)
    {
        boost::shared_ptr<CharGeneralData> p_gd = GetGenral(gid);
        if (p_gd.get() && p_gd->m_state == 0)
            return p_gd->m_level;
    }
    return 0;
}

EquipmentData* CharTotalGenerals::getEquipById(int id)
{
    EquipmentData* eq = NULL;
    for (std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.begin(); it != m_generals.end(); ++it)
    {
        if (it->second.get())
        {
            eq = it->second->m_equipments.getEquipById(id);
            if (eq)
            {
                return eq;
            }
        }
    }
    return NULL;
}

int CharTotalGenerals::Save()
{
    if (m_changed)
    {
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_generals.begin();
        while (it != m_generals.end())
        {
            if (it->second.get())
            {
                it->second->Save();
            }
            ++it;
        }
        it = m_fired_generals.begin();
        while (it != m_fired_generals.end())
        {
            if (it->second.get())
            {
                it->second->Save();
            }
            ++it;
        }
        m_changed = false;
    }
    return 0;
}

int ZhenData::Save()
{
    if (m_changed)
    {
        m_changed = false;
        std::string sql = "update char_zhens set level=" + LEX_CAST_STR(m_level);
        for (size_t i = 0; i < 9; ++i)
        {
            sql += ",pos" + LEX_CAST_STR(i+1) + "=" + LEX_CAST_STR(m_generals[i]);
        }
        sql += " where cid= " + LEX_CAST_STR(m_cid) + " and type=" + LEX_CAST_STR(m_zhen_type);
        //保存到数据库
        InsertSaveDb(sql);
    }
    return 0;
}

void ZhenData::updateNewAttack()
{
    CharTotalGenerals& char_generals = m_charData.GetGenerals();
    //评分
    m_general_score = 0;
    m_equip_score = 0;
    m_wash_score = 0;
    m_baoshi_score = 0;
    m_reborn_score = 0;
    m_level_score = 0;
    //战力
    m_general_power = 0;
    m_equip_power = 0;
    m_wash_power = 0;
    m_baoshi_power = 0;
    m_reborn_power = 0;
    m_weapon_power = 0;
    m_soul_power = 0;
    m_jxl_power = 0;
    m_gsoul_power = 0;

    //临时数据
    int general_cnt = 0;
    int wash_tmp = 0;
    int wash_full_val = m_charData.m_level * 10 + 50;
    if (m_charData.m_vip >= iWashFullValVIP)
    {
        wash_full_val *= 2;
    }
    int baoshi_tmp = 0;
    double chengzhang_tmp = 0.0;
    double chengzhang_full_val = 0.0;

    int equip_level_tmp = 0, equip_cnt_tmp = 0;
    int level_tmp = 0;

    //处理每个武将
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] > 0)
        {
            ++general_cnt;
            boost::shared_ptr<CharGeneralData> sp = char_generals.GetGenral(m_generals[i]);
            if (sp.get())
            {
                CharGeneralData& gdata = *sp.get();
                sp->updateNewAttack();
                //武将
                {
                    m_general_score += gdata.m_general_score;
                    m_general_power += gdata.m_general_power;
                }

                //装备
                {
                    //m_equip_score += gdata.m_equip_score;
                    m_equip_power += gdata.m_equip_power;
                    for (int j = 1; j <= equip_slot_max; ++j)
                    {
                        EquipmentData* eq = dynamic_cast<EquipmentData*>(gdata.m_equipments.getItem(j).get());
                        if (eq)
                        {
                            equip_level_tmp += eq->qLevel;
                            ++equip_cnt_tmp;
                        }
                    }
                }

                //洗髓
                {
                    wash_tmp += gdata.m_wash_str;
                    wash_tmp += gdata.m_wash_int;
                    wash_tmp += gdata.m_wash_tong;

                    m_wash_power += gdata.m_wash_power;
                }

                //宝石
                {
                    for (int j = 1; j <= gdata.m_baoshis.m_size; ++j)
                    {
                        if (gdata.m_baoshis.m_bagslot[j-1].get() && gdata.m_baoshis.m_bagslot[j-1]->getType() == iItem_type_baoshi)
                        {
                            newBaoshi* p = dynamic_cast<newBaoshi*>(gdata.m_baoshis.m_bagslot[j-1].get());
                            baoshi_tmp += p->level();
                        }
                    }

                    m_baoshi_power += gdata.m_baoshi_power;
                }

                //重生
                {
                    if (gdata.m_baseGeneral.get())
                    {
                        chengzhang_tmp += (gdata.m_chengzhang - gdata.m_baseGeneral->m_base_chengzhang);
                        chengzhang_full_val += (iChengZhangMax[gdata.m_color] - gdata.m_baseGeneral->m_base_chengzhang);
                    }

                    m_reborn_power += gdata.m_reborn_power;
                }
                //秘法
                {
                    if (gdata.m_baseSoldier.get())
                    {
                        switch (gdata.m_baseSoldier->m_damage_type)
                        {
                            case act_wuli_attack:
                            {
                                m_weapon_power += gdata.m_belong_to.m_charData.m_new_weapons.getNewPower_pu();
                                break;
                            }
                            case celue_damage:
                            default:
                            {
                                m_weapon_power += gdata.m_belong_to.m_charData.m_new_weapons.getNewPower_ce();
                                break;
                            }
                        }
                    }
                }
                //兵魂
                {
                    m_soul_power += gdata.m_soul_power;
                }
                //将星录
                m_jxl_power += gdata.m_jxl_power;
                //将魂
                m_gsoul_power += gdata.m_gsoul_power;
                //等级
                {
                    level_tmp += gdata.m_level;
                }
            }
        }
    }
    //装备评分总和后除以5
    //m_equip_score /= 5;
    m_equip_score = (int)((double)equip_level_tmp / m_charData.m_level / equip_cnt_tmp * 100.0);
    //洗髓评分=武将洗髓总和/洗髓上限/上阵人数*100
    //m_wash_score = (int)((double)wash_tmp / wash_full_val / general_cnt / 3.0 * 100.0);
    if (m_charData.m_level <= 35)
    {
        m_wash_score = (int)((double)wash_tmp / wash_full_val / general_cnt / 3.0 * 200.0);
    }
    else if(m_charData.m_level <= 50)
    {
        m_wash_score = (int)((double)wash_tmp / wash_full_val / general_cnt / 3.0 * 150.0);
    }
    else
    {
        m_wash_score = (int)((double)wash_tmp / wash_full_val / general_cnt / 3.0 * 100.0);
    }
    //宝石评分=宝石等级总和/开启宝石孔数/上阵武将数/13*100
    //m_baoshi_score = (int)((double)baoshi_tmp / m_charData.m_baoshi_count / general_cnt / 13.0 * 100.0);
    if (m_charData.m_level <= 35)
    {
        m_baoshi_score = (int)((double)baoshi_tmp / m_charData.m_baoshi_count / general_cnt / 5.0 * 100.0);
    }
    else if(m_charData.m_level <= 50)
    {
        m_baoshi_score = (int)((double)baoshi_tmp / m_charData.m_baoshi_count / general_cnt / 7.0 * 100.0);
    }
    else
    {
        m_baoshi_score = (int)((double)baoshi_tmp / m_charData.m_baoshi_count / general_cnt / 13.0 * 100.0);
    }
    //重生评分=提升的成长率/可提升的总成长率*100
    m_reborn_score = (int)(chengzhang_tmp / chengzhang_full_val * 100.0);
    //等级评分
    m_level_score = (int)((double)level_tmp / m_charData.m_level / general_cnt * 100.0);

    //评分上限100
    m_general_score = m_general_score > 100 ? 100 : m_general_score;
    m_equip_score = m_equip_score > 100 ? 100 : m_equip_score;
    m_wash_score = m_wash_score > 100 ? 100 : m_wash_score;
    m_baoshi_score = m_baoshi_score > 100 ? 100 : m_baoshi_score;
    m_reborn_score = m_reborn_score > 100 ? 100 : m_reborn_score;
    m_level_score = m_level_score > 100 ? 100 : m_level_score;
    /*cout << "ZhenData::updateNewAttack!!!" << endl;
    cout << "cid=" << m_cid << endl;
    cout << "general_score=" << m_general_score << endl;
    cout << "general_power=" << m_general_power << endl;
    cout << "equip_score=" << m_equip_score << endl;
    cout << "equip_power=" << m_equip_power << endl;
    cout << "wash_score=" << m_wash_score << endl;
    cout << "wash_power=" << m_wash_power << endl;
    cout << "baoshi_score=" << m_baoshi_score << endl;
    cout << "baoshi_power=" << m_baoshi_power << endl;
    cout << "reborn_score=" << m_reborn_score << endl;
    cout << "reborn_power=" << m_reborn_power << endl;
    cout << "end!!!!!!!!!!!!" << endl;*/
    //七日目标
    Singleton<seven_Goals_mgr>::Instance().updateGoals(m_charData,m_charData.queryCreateDays(),goals_type_attack_power);
    m_charData.notifyChengzhangState();
    return;
}

int ZhenData::updateAttack()
{
    //获得角色武将数据
    CharTotalGenerals& char_generals = m_charData.GetGenerals();

    m_org_attack = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] > 0)
        {
            boost::shared_ptr<CharGeneralData> sp = char_generals.GetGenral(m_generals[i]);
            if (sp.get())
            {
                const CharGeneralData& gdata = *sp.get();
                if (gdata.m_baseSoldier.get())
                {
                    int64_t damage = 0;
                    int org_damage = 0, damage_fac = 0;
                    //勇武、智力、统御
                    int _str = gdata.m_str;
                    int _int = gdata.m_int;
                    int _tongyu = gdata.m_tongyu;
                    int _attack = 0;
                    int _level = gdata.m_level;
                    if (gdata.m_baowu_type != 0)
                    {
                        switch(gdata.m_baowu_type)
                        {
                            case 1:
                                _str += gdata.m_baowu_add;
                                break;
                            case 2:
                                _int += gdata.m_baowu_add;
                                break;
                            case 3:
                                _tongyu += gdata.m_baowu_add;
                                break;
                            default:
                                break;
                        }
                    }
                    switch (gdata.m_baseSoldier->m_damage_type)
                    {
                        case act_wuli_attack:
                        {
                            _attack = gdata.m_attack + 2 * _str + m_charData.getPugong(true) + m_charData.m_combat_attribute.skill_add(1);
                            org_damage = _attack;
                            if (org_damage <= 0)
                            {
                                org_damage = 1;
                            }
                            break;
                        }
                        case celue_damage:
                        default:
                        {
                            _attack = gdata.m_attack + 2 * _int + m_charData.getCegong(true) + m_charData.m_combat_attribute.skill_add(3);
                            org_damage = _attack;
                            if (org_damage <= 0)
                            {
                                org_damage = 1;
                            }
                            break;
                        }
                    }
                    damage = (org_damage);
                    m_org_attack += damage;
                }
            }
        }
    }
    return m_org_attack;
}

int ZhenData::getAttack()
{
    if (m_attack_change)
    {
        m_attack_change = false;
        m_charData.m_new_weapons.updateNewAttack();
        m_charData.m_horse.updateNewAttack();
        updateNewAttack();
    }
    int cnt = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] > 0)
        {
            ++cnt;
        }
    }
    int total_power = (m_general_power + m_equip_power + m_wash_power + m_baoshi_power + m_reborn_power + m_weapon_power + m_soul_power + m_jxl_power + m_gsoul_power);
    total_power += (m_charData.m_horse.getNewPower() * cnt);
    //cout << "zhen=" << m_zhen_type << "power=" << total_power << endl;
    int tmp = m_charData.queryExtraData(char_data_type_normal, char_data_zhen_attack);
    if (tmp != total_power)
    {
        m_charData.setExtraData(char_data_type_normal, char_data_zhen_attack, total_power);
    }
    return total_power;
}

void ZhenData::set_attack_change()
{
    m_attack_change = true;
}

//带简单信息的英雄列表
void ZhenData::getList(json_spirit::Array& hlist)
{
    //获得角色武将数据
    CharTotalGenerals& char_generals = m_charData.GetGenerals();
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] > 0)
        {
            boost::shared_ptr<CharGeneralData> sp = char_generals.GetGenral(m_generals[i]);
            if (sp.get())
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", m_generals[i]) );
                obj.push_back( Pair("spic", sp->m_spic) );
                obj.push_back( Pair("level", sp->m_level) );
                obj.push_back( Pair("color", sp->m_color) );
                obj.push_back( Pair("name", sp->m_baseGeneral->m_name) );
                obj.push_back( Pair("good_at", sp->m_baseGeneral->m_good_at) );
                hlist.push_back(obj);
            }
        }
    }
}

//阵上武将战力
void ZhenData::getList2(json_spirit::Array& hlist)
{
    //获得角色武将数据
    CharTotalGenerals& char_generals = m_charData.GetGenerals();
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] > 0)
        {
            boost::shared_ptr<CharGeneralData> sp = char_generals.GetGenral(m_generals[i]);
            if (sp.get())
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", m_generals[i]) );
                obj.push_back( Pair("gid", sp->m_gid) );
                obj.push_back( Pair("attack", sp->getAttack()) );
                hlist.push_back(obj);
            }
        }
    }
}

int ZhenData::getGeneralCounts()
{
    int cnt = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] > 0)
        {
            ++cnt;
        }
    }
    return cnt;
}

int CharZhens::Levelup(int type, int level)
{
    boost::shared_ptr<ZhenData> zhen = GetZhen(type);
    if (zhen.get())
    {
        if (zhen->m_level < 5 && zhen->m_level < level)
        {
            ++zhen->m_level;
            //是否需要通知玩家升级了
            m_charData.updateTask(task_zhen_level, type, zhen->m_level);
            boost::shared_ptr<BaseZhenData> bz = GeneralDataMgr::getInstance()->GetBaseZhen(type);
            if (bz.get())
            {
                int open_pos = bz->m_open_pos[zhen->m_level - 1];
                zhen->m_generals[open_pos - 1] = 0;
            }
            else
            {
                ERR();
            }
            zhen->m_changed = true;
            zhen->Save();
            return zhen->m_level;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        //获得1级阵法
        boost::shared_ptr<ZhenData> zhen = NewZhen(m_charData, type, 1);
        if (zhen.get())
        {
            m_zhens[zhen->m_zhen_type] = zhen;
            //是否需要通知玩家升级了
            m_charData.updateTask(task_zhen_level, type, zhen->m_level);
            return zhen->m_level;
        }
        else
        {
            ERR();
            cout<<"cid"<<m_cid<<","<<type<<endl;
            return -1;
        }
    }
    return -1;
}

void CharZhens::setLevel(int type, int level)
{
    //cout<<"set zhen level "<<type<<","<<level<<endl;
    int zlevel = 0;
    boost::shared_ptr<ZhenData> zhen = GetZhen(type);
    if (!zhen.get())
    {
        //获得1级阵法
        zhen = NewZhen(m_charData, type, 1);
        m_zhens[zhen->m_zhen_type] = zhen;
        zlevel = 1;
    }
    if (zhen.get())
    {
        if (level > 5)
        {
            level = 5;
        }
        if (zhen->m_level < 5 && zhen->m_level < level)
        {
            //cout<<"!!!!!set zhen level "<<type<<","<<level<<endl;
            zhen->m_level = level;
            zlevel = level;
            //是否需要通知玩家升级了
            m_charData.updateTask(task_zhen_level, type, zhen->m_level);
            boost::shared_ptr<BaseZhenData> bz = GeneralDataMgr::getInstance()->GetBaseZhen(type);
            if (bz.get())
            {
                for (int l = 0; l < zhen->m_level; ++l)
                {
                    int open_pos = bz->m_open_pos[l];
                    if (zhen->m_generals[open_pos - 1] < 0)
                    {
                        zhen->m_generals[open_pos - 1] = 0;
                    }
                }
            }
            else
            {
                ERR();
            }
            zhen->m_changed = true;
            zhen->Save();
        }
        if (zlevel > 0)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "notify") );
            obj.push_back( Pair("s", 200) );
            obj.push_back( Pair("type", notify_msg_new_zhen) );
            obj.push_back( Pair("name", zhen->m_name) );
            obj.push_back( Pair("level", zlevel) );
            m_charData.sendObj(obj);
        }
    }
    else
    {
        ERR();
        cout<<"cid"<<m_cid<<","<<type<<endl;
        return;
    }
    return;
}

//玩家阵型
int CharZhens::Load()
{
    if (0 == m_cid)
    {
        return 0;
    }

    //INFO(m_cid<<"*********** CharZhens load zhens ");
    Query q(GetDb());
    m_default_zhen = 1;
    q.get_result("SELECT default_zhen from char_default_zhen WHERE cid=" + LEX_CAST_STR(m_cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_default_zhen = q.getval();
        q.free_result();
    }
    else
    {
        q.free_result();
        //没有默认阵型，插入
        InsertSaveDb("insert into char_default_zhen set cid=" + LEX_CAST_STR(m_cid) + ",default_zhen=1");
    }

    q.get_result("SELECT c.type,c.level,b.name,c.pos1,c.pos2,c.pos3,c.pos4,c.pos5,c.pos6,c.pos7,c.pos8,c.pos9 FROM char_zhens as c left join base_zhens as b on c.type=b.type WHERE c.cid=" + LEX_CAST_STR(m_cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<ZhenData> zhen;
        zhen.reset(new ZhenData(m_cid, m_charData));
        zhen->m_zhen_type = q.getval();
        zhen->m_level = q.getval();
        zhen->m_name = q.getstr();
        for (size_t i = 0; i < 9; ++i)
        {
            zhen->m_generals[i] = q.getval();
        }
        m_zhens[zhen->m_zhen_type] = zhen;
       // INFO("************ add zhen :"<<zhen->m_zhen_type);
    }
    q.free_result();
    return 0;
}

int CharZhens::Swap(int zhenid, int pos1, int pos2)   //交换两个位置的武将
{
    if (pos1 > 9 || pos1 < 1 || pos2 > 9 || pos2 < 1)
    {
        return -1;
    }
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.find(zhenid);
    if (it == m_zhens.end())
    {
        return -2;
    }
    ZhenData* zhen = it->second.get();
    if (zhen == NULL)
    {
        return -2;
    }
    //判断位置所在武将是否有
    if ((zhen->m_generals[pos1 - 1] >= 0 && zhen->m_generals[pos2 - 1] > 0)
        || (zhen->m_generals[pos1 - 1] > 0 && zhen->m_generals[pos2 - 1] >= 0))
    {
        //交换
        int temp = zhen->m_generals[pos1 - 1];
        zhen->m_generals[pos1 - 1] = zhen->m_generals[pos2 - 1];
        zhen->m_generals[pos2 - 1] = temp;
        m_changed = true;
        zhen->m_changed = true;
    }
    //一个有效，一个无效，相当于换下武将
    else if (zhen->m_generals[pos1 - 1] > 0 && zhen->m_generals[pos2 - 1] < 0)
    {
        zhen->m_generals[pos1 - 1] = 0;
        m_changed = true;
        zhen->m_changed = true;
        //攻擊力變化
        m_charData.set_attack_change();
    }
    else if (zhen->m_generals[pos2 - 1] > 0 && zhen->m_generals[pos1 - 1] < 0)
    {
        zhen->m_generals[pos2 - 1] = 0;
        m_changed = true;
        zhen->m_changed = true;
        //攻擊力變化
        m_charData.set_attack_change();
    }
    Save();
    return HC_SUCCESS;
}

int CharZhens::Up(int zhenid, int pos, int gid)        //武将上阵
{
    if (pos > 9 || pos < 1)
    {
        return -1;
    }
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.find(zhenid);
    if (it == m_zhens.end())
    {
        return -2;
    }
    ZhenData* zhen = it->second.get();
    if (zhen == NULL)
    {
        return -2;
    }
    if (zhen->m_generals[pos - 1] < 0)
    {
        return -2;
    }
    boost::shared_ptr<CharGeneralData> g = m_charData.m_generals.GetGenral(gid);
    if (!g.get())
    {
        return -3;
    }
    int count = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (zhen->m_generals[i] == gid)
        {
            return HC_ERROR;
        }
        else if (zhen->m_generals[i] > 0)
        {
            ++count;
        }
    }

    if (zhen->m_generals[pos - 1] == 0 && (count + 1 > m_charData.m_up_generals))
    {
        return HC_ERROR_UPZHEN_FULL;
    }

    zhen->m_generals[pos - 1] = gid;
    m_changed = true;
    zhen->m_changed = true;
    //攻擊力變化
    m_charData.set_attack_change();

    if (m_default_zhen == zhenid)
    {
        m_charData.NotifyCharData();
    }
    Save(zhenid);

    //更新任务
    m_charData.updateTask(task_send_general, g->m_gid);
    return HC_SUCCESS;
}

int CharZhens::Up(int zhenid, int gid)        //武将上阵,只要有空位就上
{
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.find(zhenid);
    if (it == m_zhens.end())
    {
        return -2;
    }
    ZhenData* zhen = it->second.get();
    if (zhen == NULL)
    {
        return -2;
    }
    boost::shared_ptr<CharGeneralData> g = m_charData.m_generals.GetGenral(gid);
    if (!g.get())
    {
        return -3;
    }
    int count = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (zhen->m_generals[i] == gid)
        {
            return HC_SUCCESS;
        }
        else if (zhen->m_generals[i] > 0)
        {
            ++count;
        }
    }
    if (count >= m_charData.m_up_generals)
    {
        return HC_ERROR_UPZHEN_FULL;
    }
    for (size_t pos = 1; pos <= 9; ++pos)
    {
        if (zhen->m_generals[pos - 1] == 0)
        {
            zhen->m_generals[pos - 1] = gid;
            m_changed = true;
            zhen->m_changed = true;
            Save(zhenid);

            //更新任务
            m_charData.updateTask(task_send_general, g->m_gid);
            //攻擊力變化
            m_charData.set_attack_change();

            if (m_default_zhen == zhenid)
            {
                m_charData.NotifyCharData();
            }
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int CharZhens::Down(int zhenid, int pos)              //武将下阵
{
    if (pos > 9 || pos < 1)
    {
        return -1;
    }
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.find(zhenid);
    if (it == m_zhens.end())
    {
        return -2;
    }
    ZhenData* zhen = it->second.get();
    if (zhen == NULL)
    {
        return -2;
    }
    //默认阵形必须保留一个武将
    if (m_default_zhen == zhenid)
    {
        int cnt = 0;
        for (size_t i = 0; i < 9; ++i)
        {
            if (zhen->m_generals[i] > 0)
            {
                ++cnt;
            }
        }
        if (cnt < 2)
        {
            return HC_ERROR_ZHEN_NEED_ONE;
        }
    }
    if (zhen->m_generals[pos - 1] > 0)
    {
        zhen->m_generals[pos - 1] = 0;
        m_changed = true;
        zhen->m_changed = true;
        //攻擊力變化
        m_charData.set_attack_change();
        if (m_default_zhen == zhenid)
        {
            m_charData.NotifyCharData();
        }
        Save(zhenid);
    }
    return HC_SUCCESS;
}

int CharZhens::Down(int gid)              //所有阵型中的该武将下阵
{
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.begin();
    while (it != m_zhens.end())
    {
        if (it->second.get())
        {
            for (int i = 0; i < 9; ++i)
            {
                if (it->second->m_generals[i] == gid)
                {
                    it->second->m_generals[i] = 0;
                    m_changed = true;
                    it->second->m_changed = true;
                }
            }
        }
        ++it;
    }
    if (m_changed)
    {
        //攻擊力變化
        m_charData.set_attack_change();
        m_charData.NotifyCharData();
        Save();
    }
    return HC_SUCCESS;
}

int CharZhens::SetDefault(int zhenid)                 //设置缺省阵型
{
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.find(zhenid);
    if (it != m_zhens.end() && it->second.get())
    {
        ZhenData* zhen = it->second.get();
        int cnt = 0;
        for (size_t i = 0; i < 9; ++i)
        {
            if (zhen->m_generals[i] > 0)
            {
                ++cnt;
            }
        }
        if (cnt < 1)
        {
            return HC_ERROR_DEFAULT_ZHEN;
        }
    }
    if (m_default_zhen != zhenid)
    {
        //攻擊力變化
        m_charData.set_attack_change();
        m_charData.NotifyCharData();
        //m_changed_default = true;
        m_default_zhen = zhenid;
        InsertSaveDb("Update char_default_zhen set default_zhen=" + LEX_CAST_STR(m_default_zhen) + " where cid="+ LEX_CAST_STR(m_cid));
    }
    return HC_SUCCESS;
}

int CharZhens::GetDefault()                           //缺省阵型
{
    return m_default_zhen;
}

boost::shared_ptr<ZhenData> CharZhens::GetZhen(int zhenid)        //获取阵型信息
{
    //INFO(m_cid<<"********GetZhen*******"<<zhenid);
    //INFO("show zhens");
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.find(zhenid);
    if (it != m_zhens.end())
    {
        return it->second;
    }
    boost::shared_ptr<ZhenData> z;
    //INFO("********not find");
    return z;
}

bool CharZhens::Check(int gid)              //是否是默认阵形的最后一个武将
{
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.find(m_default_zhen);
    if (it != m_zhens.end() && it->second.get())
    {
        ZhenData* zhen = it->second.get();
        int cnt = 0;
        bool in_zhen = false;
        for (size_t i = 0; i < 9; ++i)
        {
            if (zhen->m_generals[i] == gid)
            {
                in_zhen = true;
            }
            if (zhen->m_generals[i] > 0)
            {
                ++cnt;
            }
        }
        if (in_zhen && cnt < 1)
        {
            return true;
        }
    }
    return false;
}

int CharZhens::Save(int type)
{
    if (m_changed_default)
    {
        //保存到数据库
        InsertSaveDb("Update char_default_zhen set default_zhen=" + LEX_CAST_STR(m_default_zhen) + " where cid="+ LEX_CAST_STR(m_cid));
        m_changed_default = false;
    }
    if (m_changed)
    {
        std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.begin();
        while (it != m_zhens.end())
        {
            if (it->second.get() && (0 == type || it->second->m_zhen_type == type))
            {
                it->second->Save();
            }
            ++it;
        }
        m_changed = false;
    }
    return 0;
}

void CharZhens::set_attack_change()
{
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = m_zhens.begin();
    while (it != m_zhens.end())
    {
        if (it->second.get())
        {
            it->second->set_attack_change();
        }
        ++it;
    }
}

void baseStage::addLoot(boost::shared_ptr<Item> item, boost::shared_ptr<json_spirit::Object> o)
{
    if (!o.get() || !item.get())
    {
        return;
    }

    if (!loots_array.get())
    {
        loots_array.reset(new json_spirit::Array);
    }
    if (item->type == 1)
    {
        loots_array->push_back(*(o.get()));
    }
    else
    {
        loots_array->insert(loots_array->begin(), *(o.get()));
    }
}

bool specialSkill::parseEffect(const std::string& effect)
{
    if (effect == "")
    {
        return true;
    }
    json_spirit::mValue mvalue;
    json_spirit::read(effect, mvalue);
    if (mvalue.type() != json_spirit::obj_type)
    {
        ERR();
        cout<<"parse error,not a object"<<endl;
        return false;
    }
    json_spirit::mObject& mobj = mvalue.get_obj();
    std::string type = "", to = "";
    READ_STR_FROM_MOBJ(type,mobj,"type");
    READ_STR_FROM_MOBJ(to,mobj,"to");
    int value = 1, round = 0, gailv = 100;
    READ_INT_FROM_MOBJ(value,mobj,"value");
    READ_INT_FROM_MOBJ(round,mobj,"round");
    READ_INT_FROM_MOBJ(gailv,mobj,"gailv");

    int itarget = GeneralDataMgr::getInstance()->getSpeSkillTarget(to);
    if (itarget == 0)
    {
        ERR();
        cout<<"parse error,'to':"<<to<<endl;
        return false;
    }
    int itype = GeneralDataMgr::getInstance()->getSpeSkillType(type);
    baseBuff* pb = GeneralDataMgr::getInstance()->getBaseBuff(itype);
    if (itype == 0 || NULL == pb)
    {
        ERR();
        cout<<"parse error,'type':"<<type<<",effect:"<<effect<<endl;
        return false;
    }

    skillEffect eft;
    eft.bbuff = pb;
    eft.chance = 10*gailv;
    eft.value = value;
    eft.lastRound = round;

    switch (itype)
    {
        //概率性的buff转换为千分之几
        case buff_baoji:         //提升暴击率
        case buff_gedang:        //提升格挡率
        case buff_shipo:        //提升识破率
        case buff_dodge:        //提升闪避率
        case buff_mingzhong:    //提升命中率

        case debuff_baoji:        //降低暴击率
        case debuff_gedang:        //降低格挡率
        case debuff_shipo:        //降低识破率
        case debuff_dodge:        //降低躲闪率
        case debuff_miss:        //降低命中率
            eft.value *= 10;
            break;
        case effect_soul_link:
            eft.value = 1;
    }
    switch (itarget)
    {
        case range_single:
            //cout<<"add target buff "<<pb->type<<",chance:"<<eft.chance<<",value:"<<eft.value<<",round:"<<eft.lastRound<<endl;
            target_effects.push_back(eft);
            break;
        case range_all:
            //cout<<"add all enermy buff "<<pb->type<<",chance:"<<eft.chance<<",value:"<<eft.value<<",round:"<<eft.lastRound<<endl;
            enermy_effects.push_back(eft);
            break;
        case range_self_single:
            //cout<<"add self buff "<<pb->type<<",chance:"<<eft.chance<<",value:"<<eft.value<<",round:"<<eft.lastRound<<endl;
            self_effects.push_back(eft);
            break;
        case range_self_all:
            //cout<<"add own team buff "<<pb->type<<",chance:"<<eft.chance<<",value:"<<eft.value<<",round:"<<eft.lastRound<<endl;
            self_all_effects.push_back(eft);
            break;
    }
    return true;
}

void combatAttribute::load(const std::string& s)
{
    json_spirit::mValue value;
    json_spirit::read(s, value);

    if (value.type() != json_spirit::obj_type)
    {
        return;
    }

    json_spirit::mObject& obj = value.get_obj();
    //伤害减免 - 这个必须放在第一个!!!
    read_int_array(obj, "sub_damage", m_sub_damage, 2);    // {减少受到普通伤害百分比,减少受到的策略伤害百分比}

    //对兵种防御，减免受到特定兵种的伤害百分比
    read_int_array(obj, "sub_damage_from", m_sub_damage_from, 5);

    //技能+状态增加的对兵种的基础攻防/血量等
    read_int_array(obj, "skill_add", m_skill_add, skill_add_max);

    //对兵种克制，增加对特定兵种的伤害百分比
    read_int_array(obj, "damage_to", m_more_damage_to, 5);

    READ_INT_FROM_MOBJ(m_death_fight,obj,"death_fight");//死战

    //兵种加成
    read_int_array(obj, "hp_stype", m_hp_stype, 5);
    read_int_array(obj, "attack_stype", m_attack_stype, 5);
    read_int_array(obj, "wufang_stype", m_wufang_stype, 5);
    read_int_array(obj, "cefang_stype", m_cefang_stype, 5);
    read_int_array(obj, "moredamage_stype", m_moredamage_stype, 5);

    READ_INT_FROM_MOBJ(m_weak,obj,"weak");//虚弱状态，按比例降低伤害

    //特殊攻击效果
    read_int_array(obj, "special_attack", (int*)m_special_attack, special_attack_max*2);
    read_int_array(obj, "special_alevel", m_special_attack_level, special_attack_max);

    //特殊攻击抗性
    read_int_array(obj, "resist", m_special_resist, special_attack_max);
    read_int_array(obj, "resist_lv", m_special_resist_level, special_attack_max);

    READ_INT_FROM_MOBJ(m_boss_inspired, obj, "boss_ins");    //boss战斗鼓舞
    READ_INT_FROM_MOBJ(m_race_inspired, obj, "race_ins");    //竞技鼓舞
    READ_INT_FROM_MOBJ(m_camp_inspired, obj, "camp_ins");    //阵营站鼓舞
    READ_INT_FROM_MOBJ(m_guard_inspired, obj, "guard_ins");    //护纲鼓舞
    READ_INT_FROM_MOBJ(m_maze_inspired, obj, "maze_ins");    //迷宫鼓舞
    READ_INT_FROM_MOBJ(m_total_inspired, obj, "total_ins");    //综合起来的鼓舞

    READ_INT_FROM_MOBJ(m_init_shiqi, obj, "init_shiqi");        //初始士气
    READ_INT_FROM_MOBJ(m_casted_shiqi, obj, "casted_shiqi");    //施放绝技后的士气

    READ_INT_FROM_MOBJ(m_org_shiqi, obj, "org_shiqi");      //自带士气
    READ_INT_FROM_MOBJ(m_org_inspired, obj, "org_ins");    //自带鼓舞

    //print(true);
    //int m_enable;
}

void combatAttribute::print(bool p) const
{
    if (p || g_print_debug_info >= 3)
    {
        using namespace std;

        cout<<"*******************************************"<<endl;
        cout<<this<<endl;
        if (m_enable > 0)
        {
            cout<<"enable!!!!!"<<endl;
        }
        else
        {
            cout<<"disenable!!!!!!!!!!!!!!!!!!!"<<endl;
        }

        cout<<"normal damage -"<<m_sub_damage[0]<<endl;
        cout<<"celue damage -"<<m_sub_damage[1]<<endl;
        cout<<endl;

        cout<<"damage from bubing -"<<m_sub_damage_from[0]<<endl;
        cout<<"damage from gongbing -"<<m_sub_damage_from[1]<<endl;
        cout<<"damage from qibing -"<<m_sub_damage_from[3]<<endl;
        cout<<"damage from moushi -"<<m_sub_damage_from[2]<<endl;
        cout<<"damage from qxie -"<<m_sub_damage_from[4]<<endl;
        cout<<endl;

        cout<<"more damage to bubing +"<<m_more_damage_to[0]<<endl;
        cout<<"more damage to gongbing +"<<m_more_damage_to[1]<<endl;
        cout<<"more damage to qibing +"<<m_more_damage_to[3]<<endl;
        cout<<"more damage to moushi +"<<m_more_damage_to[2]<<endl;
        cout<<"more damage to qxie +"<<m_more_damage_to[4]<<endl;
        cout<<endl;

        cout<<"skill add [";
        for (int i = 0; i < skill_add_max; ++i)
        {
            cout<<m_skill_add[i]<<",";
        }
        cout<<"]"<<endl;

        cout<<"m_hp_stype[";
        for (int i = 0; i < 5; ++i)
        {
            cout<<m_hp_stype[i]<<",";
        }
        cout<<"]"<<endl;

        cout<<"m_moredamage_stype[";
        for (int i = 0; i < 5; ++i)
        {
            cout<<m_moredamage_stype[i]<<",";
        }
        cout<<"]"<<endl;

        cout<<"baoji "<<m_special_attack[0][0]<<","<<m_special_attack[0][1]<<endl;
        cout<<"dodge "<<m_special_attack[1][0]<<","<<m_special_attack[1][1]<<endl;
        cout<<"parry "<<m_special_attack[2][0]<<","<<m_special_attack[2][1]<<endl;
        cout<<"shipo "<<m_special_attack[3][0]<<","<<m_special_attack[3][1]<<endl;
        cout<<"xixue "<<m_special_attack[4][0]<<","<<m_special_attack[4][1]<<endl;
        cout<<"chaos "<<m_special_attack[5][0]<<","<<m_special_attack[5][1]<<endl;
        cout<<"podan "<<m_special_attack[6][0]<<","<<m_special_attack[6][1]<<endl;
        cout<<"weihe "<<m_special_attack[7][0]<<","<<m_special_attack[7][1]<<endl;
        if (m_death_fight)
        {
            cout<<endl;
            cout<<"death fight!!!!!!!!!!!!!"<<endl;
        }
        cout<<"weak "<<m_weak<<endl;
        cout<<"init shiqi "<<m_init_shiqi<<endl;

        cout<<"*******************************************"<<endl;
    }
}

void combatAttribute::save(std::string& s)
{
    //cout<<"combatAttribute::save()"<<endl;
    //print(true);
    json_spirit::Object obj;
    //伤害减免 - 这个必须放在第一个!!!
    write_int_array(obj, "sub_damage", m_sub_damage, 2);    // {减少受到普通伤害百分比,减少受到的策略伤害百分比}

    //对兵种防御，减免受到特定兵种的伤害百分比
    write_int_array(obj, "sub_damage_from", m_sub_damage_from, 5);

    //技能+状态增加的对兵种的基础攻防/血量等
    write_int_array(obj, "skill_add", m_skill_add, skill_add_max);

    //对兵种克制，增加对特定兵种的伤害百分比
    write_int_array(obj, "damage_to", m_more_damage_to, 5);

    obj.push_back( Pair("death_fight", m_death_fight) );//死战

    //兵种加成
    write_int_array(obj, "hp_stype", m_hp_stype, 5);
    write_int_array(obj, "attack_stype", m_attack_stype, 5);
    write_int_array(obj, "wufang_stype", m_wufang_stype, 5);
    write_int_array(obj, "cefang_stype", m_cefang_stype, 5);
    write_int_array(obj, "moredamage_stype", m_moredamage_stype, 5);

    obj.push_back( Pair("weak", m_weak) );//虚弱状态，按比例降低伤害

    //特殊攻击效果
    write_int_array(obj, "special_attack", (int*)m_special_attack, special_attack_max*2);
    write_int_array(obj, "special_alevel", m_special_attack_level, special_attack_max);

    //特殊攻击抗性
    write_int_array(obj, "resist", m_special_resist, special_attack_max);
    write_int_array(obj, "resist_lv", m_special_resist_level, special_attack_max);

    obj.push_back( Pair("boss_ins", m_boss_inspired) );    //boss战斗鼓舞
    obj.push_back( Pair("race_ins", m_race_inspired) );    //竞技鼓舞
    obj.push_back( Pair("camp_ins", m_camp_inspired) );    //阵营站鼓舞
    obj.push_back( Pair("guard_ins", m_guard_inspired) );    //护纲鼓舞
    obj.push_back( Pair("maze_ins", m_maze_inspired) );    //迷宫鼓舞
    obj.push_back( Pair("total_ins", m_total_inspired) );    //综合起来的鼓舞

    obj.push_back( Pair("init_shiqi", m_init_shiqi) );        //初始士气
    obj.push_back( Pair("casted_shiqi", m_casted_shiqi) );    //施放绝技后的士气

    obj.push_back( Pair("org_shiqi", m_org_shiqi) );     //自带士气
    obj.push_back( Pair("org_ins", m_org_inspired) );    //自带鼓舞

    s = json_spirit::write(obj);
    //int m_enable;
}

combatAttribute::combatAttribute()
{
    //cout<<"combatAttribute()"<<endl;
    memset(&m_sub_damage, 0, sizeof(combatAttribute));
}

void combatAttribute::clear()
{
    memset(&m_sub_damage, 0, sizeof(combatAttribute));
}

//赋值
combatAttribute& combatAttribute::operator=(const combatAttribute &t1)
{
    //cout<<"combatAttribute = "<<endl;
    if (t1.m_enable == 0)
    {
        memset(&m_sub_damage, 0, sizeof(combatAttribute));
    }
    else
    {
        memcpy(m_sub_damage, t1.m_sub_damage, sizeof(combatAttribute));
    }
    return *this;
}
// +=
combatAttribute& combatAttribute::operator+=(const combatAttribute &t1)
{
    //cout<<"combatAttribute += "<<&t1<<endl;
    if (t1.m_enable > 0)
    {
    }
    else
    {
        //cout<<"------------------------- t1.enable = false"<<endl;
        //t1.print(true);
        return *this;
    }
    m_sub_damage[0] += t1.m_sub_damage[0];
    m_sub_damage[1] += t1.m_sub_damage[1];
    for (int i = 0; i < 5; ++i)
    {
        m_sub_damage_from[i] += t1.m_sub_damage_from[i];
    }
    for (int i = 0; i < skill_add_max; ++i)
    {
        m_skill_add[i] += t1.m_skill_add[i];
    }
    for (int i = 0; i < 5; ++i)
    {
        m_more_damage_to[i] += t1.m_more_damage_to[i];
    }
    //练兵兵种属性
    for (int i = 0; i < 5; ++i)
    {
        m_hp_stype[i] += t1.m_hp_stype[i];
        m_attack_stype[i] += t1.m_attack_stype[i];
        m_wufang_stype[i] += t1.m_wufang_stype[i];
        m_cefang_stype[i] += t1.m_cefang_stype[i];
        m_moredamage_stype[i] += t1.m_moredamage_stype[i];
    }
    //暴击 概率叠加，伤害比例不变
    m_special_attack[special_attack_baoji][0] += t1.m_special_attack[special_attack_baoji][0];
    m_special_attack[special_attack_baoji][1] = my_max(m_special_attack[special_attack_baoji][1],t1.m_special_attack[special_attack_baoji][1]);
    //躲闪概率叠加
    m_special_attack[special_attack_dodge][0] += t1.m_special_attack[special_attack_dodge][0];
    //格挡概率叠加
    m_special_attack[special_attack_parry][0] += t1.m_special_attack[special_attack_parry][0];
    //识破概率叠加
    m_special_attack[special_attack_shipo][0] += t1.m_special_attack[special_attack_shipo][0];
    //吸血概率不变,吸血比例叠加
    m_special_attack[special_attack_xixue][0] = my_max(m_special_attack[special_attack_xixue][0],t1.m_special_attack[special_attack_xixue][0]);
    m_special_attack[special_attack_xixue][1] += t1.m_special_attack[special_attack_xixue][1];
    if (m_special_attack[special_attack_xixue][1]>100)
    {
        m_special_attack[special_attack_xixue][1] = 100;
    }
    //混乱概率叠加
    m_special_attack[special_attack_chaos][0] += t1.m_special_attack[special_attack_chaos][0];
    //破胆数值叠加
    m_special_attack[special_attack_podan][0] = my_max(m_special_attack[special_attack_podan][0],t1.m_special_attack[special_attack_podan][0]);
    m_special_attack[special_attack_podan][1] += t1.m_special_attack[special_attack_podan][1];
    //威吓概率叠加
    m_special_attack[special_attack_weihe][0] += t1.m_special_attack[special_attack_weihe][0];

    //抗性叠加
    for (int i = 0; i < special_attack_max; ++i)
    {
        m_special_resist[i] += t1.m_special_resist[i];
        m_special_resist_level[i] += t1.m_special_resist_level[i];
        m_special_attack_level[i] += t1.m_special_attack_level[i];
    }

    //死战标记
    if (!m_death_fight && t1.m_death_fight)
    {
        m_death_fight = true;
    }
    //虚弱叠加
    m_weak += t1.m_weak;

    m_boss_inspired += t1.m_boss_inspired;
    m_race_inspired += t1.m_race_inspired;
    m_camp_inspired += t1.m_camp_inspired;
    m_guard_inspired += t1.m_guard_inspired;
    m_maze_inspired += t1.m_maze_inspired;    //迷宫鼓舞

    //cout<<"+++++++++++++++++++++ +init shiqi"<<t1.m_init_shiqi<<endl;
    m_init_shiqi += t1.m_init_shiqi;

    m_org_inspired += t1.m_org_inspired;
    m_org_shiqi += t1.m_org_shiqi;

    return *this;
}

// +=
combatAttribute& combatAttribute::operator+=(const newBaoshi &t1)
{
    switch (t1.m_base.atype)
    {
        case combat_attr_pugong: //普通攻击 1
            m_skill_add[skill_add_pugong] += t1.value();
            break;
        case combat_attr_pufang: //普通防御 2
            m_skill_add[skill_add_pufang] += t1.value();
            break;
        case combat_attr_cegong: //策略攻击 3
            m_skill_add[skill_add_cegong] += t1.value();
            break;
        case combat_attr_cefang: //策略防御 4
            m_skill_add[skill_add_cefang] += t1.value();
            break;
        case combat_attr_bingli: //兵力 5
            m_skill_add[skill_add_hp] += t1.value();
            break;
        case combat_attr_baoji:    //暴击 6
            //暴击 概率叠加，伤害比例不变
            m_special_attack_level[special_attack_baoji] += t1.value();
            break;
        case combat_attr_dodge:    //躲闪 7
            //躲闪概率叠加
            m_special_attack_level[special_attack_dodge] += t1.value();
            break;
        case combat_attr_parry:    //格挡 8
            //格挡概率叠加
            m_special_attack_level[special_attack_parry] += t1.value();
            break;
        case combat_attr_shipo:    //识破 9
            //识破概率叠加
            m_special_attack_level[special_attack_shipo] += t1.value();
            break;
        case combat_attr_xixue:    //吸血 10
            //吸血概率不变,吸血比例叠加
            m_special_attack_level[special_attack_xixue] += t1.value();
            break;
        case combat_attr_chaos:    //混乱 11
            //混乱概率叠加
            m_special_attack_level[special_attack_chaos] += t1.value();
            break;
        case combat_attr_podan:    //破胆 12
            //破胆数值叠加
            m_special_attack_level[special_attack_podan] += t1.value();
            break;
        case combat_attr_weihe:    //威慑 13
            //威吓概率叠加
            m_special_attack_level[special_attack_weihe] += t1.value();
            break;
        case combat_attr_resist_baoji://抗暴击 14
        case combat_attr_resist_dodge://抗躲闪 15
        case combat_attr_resist_parry://抗格挡 16
        case combat_attr_resist_shipo://抗识破 17
        case combat_attr_resist_xixue://抗吸血 18
        case combat_attr_resist_chaos://抗混乱 19
        case combat_attr_resist_podan://抗破胆 20
        case combat_attr_resist_weihe://抗威慑 21
            //抗性叠加
            m_special_resist_level[t1.m_base.atype - combat_attr_resist_baoji] += t1.value();
            break;

        case combat_attr_sub_damage_wuli:        //物理伤害减少 22
            m_sub_damage[0] += t1.value();
            break;
        case combat_attr_sub_damage_celue:    //策略伤害减少 23
            m_sub_damage[1] += t1.value();
            break;
        case combat_attr_ke_bubing:    //对步兵克制 24
        case combat_attr_ke_gbing:        //对弓兵克制 25
        case combat_attr_ke_moushi:    //对策士克制 26
        case combat_attr_ke_qibing:    //对骑士克制 27
        case combat_attr_ke_qixie:        //对器械克制 28
            m_more_damage_to[0 + t1.m_base.atype - combat_attr_ke_bubing] += t1.value();
            break;

        case combat_attr_fang_bubing:    //对步兵防御 29
        case combat_attr_fang_gbing: //对弓兵防御 30
        case combat_attr_fang_moushi:    //对策士防御 31
        case combat_attr_fang_qibing:    //对骑士防御 32
        case combat_attr_fang_qixie: //对器械防御 33
            m_sub_damage_from[0 + t1.m_base.atype - combat_attr_fang_bubing] += t1.value();
            break;

        case combat_attr_death_fight:    //死战 34
            if (!m_death_fight && t1.value())
            {
                m_death_fight = true;
            }
            break;

        case combat_attr_inspire:        //鼓舞 35
            m_boss_inspired += t1.value();
            m_race_inspired += t1.value();
            m_camp_inspired += t1.value();
            m_guard_inspired += t1.value();
            m_maze_inspired += t1.value();
            break;

        case combat_attr_shiqi:        //初始士气 36
            m_init_shiqi += t1.value();
            break;

    }
    return *this;
}

GeneralDataMgr* GeneralDataMgr::m_handle = NULL;

GeneralDataMgr::GeneralDataMgr()
{
    m_free_vip4_endtime = 0;
    m_enable_chenmi = false;
    m_inited = false;

    m_camp_strength[0] = 0;
    m_camp_strength[1] = 0;

    m_camp_count[0] = 0;
    m_camp_count[1] = 0;

    //m_camp_race_wins[0] = 0;
    //m_camp_race_wins[1] = 0;

    //m_weak_camp = 0;
    rwlock_init(&guild_chanel_rwmutex);
    rwlock_init(&camp_chanel_rwmutex);
    rwlock_init(&onlineuser_rwmutex);

    rwlock_init(&onlinechar_rwmutex);
    rwlock_init(&globalchar_rwmutex);

    for (int mapid = 1; mapid <= max_map_id; ++mapid)
    {
        boost::shared_ptr<ChatChannel> ch;
        ch.reset(new ChatChannel("map" + LEX_CAST_STR(mapid), mapid, "{\"cmd\":\"chat\",\"ctype\":4,\"s\":200,"));
        ch->start();
        m_map_channels[mapid-1] = ch;
    }
}

GeneralDataMgr::~GeneralDataMgr()
{
    if (m_world_channel.get())     //世界聊天
    {
        m_world_channel->stop();
    }
    boost::unordered_map<int, boost::shared_ptr<ChatChannel> >::iterator it_camp = m_camp_channels.begin();         //阵营聊天
    while (it_camp != m_camp_channels.end())
    {
        if (it_camp->second.get())
        {
            it_camp->second->stop();
        }
        ++it_camp;
    }
    boost::unordered_map<uint64_t, boost::shared_ptr<ChatChannel> >::iterator it_guild = m_guild_channels.begin();   //公会聊天
    while (it_guild != m_guild_channels.end())
    {
        if (it_guild->second.get())
        {
            it_guild->second->stop();
        }
        ++it_guild;
    }
}

int GeneralDataMgr::reloadBaseSoldier()
{
    cout<<"************ GeneralDataMgr::reloadBaseSoldier() ******************"<<endl;

    Query q(GetDb());
    //初始化基础兵种信息
    q.get_result("SELECT stype,spic,act_type,act_target,act_type2,act_type3,base_stype,name,attack,pufang,cefang,memo,spe1,spe2,spe3,damage_type,spe_fac FROM base_soldiers WHERE 1 order by stype");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int sid = q.getval();
        boost::shared_ptr<BaseSoldierData> p_soldierdata = GetBaseSoldier(sid);
        if (!p_soldierdata.get())
        {
            p_soldierdata.reset(new BaseSoldierData);
        }
        m_base_soldiers_map[sid] = p_soldierdata;
        p_soldierdata->m_stype = sid;
        p_soldierdata->m_spic = q.getval();
        p_soldierdata->m_damage_type = q.getval();
        if (p_soldierdata->m_damage_type != 1)
        {
            p_soldierdata->m_damage_type = 2;
        }
        p_soldierdata->m_attack_type = q.getval();
        p_soldierdata->m_attack_type2 = q.getval();
        p_soldierdata->m_attack_type3 = q.getval();
        p_soldierdata->m_base_type = q.getval();
        p_soldierdata->m_name = q.getstr();
        p_soldierdata->m_attack = q.getval();
        p_soldierdata->m_wufang = q.getval();
        p_soldierdata->m_cefang = q.getval();
        p_soldierdata->m_desc = q.getstr();
        p_soldierdata->m_pec[0] = baseStateMgr::getInstance()->GetBaseState(q.getval());
        p_soldierdata->m_pec[1] = baseStateMgr::getInstance()->GetBaseState(q.getval());
        p_soldierdata->m_pec[2] = baseStateMgr::getInstance()->GetBaseState(q.getval());
        p_soldierdata->m_damage_type2 = q.getval();
        p_soldierdata->m_special_attack_fac = q.getval();

        //兵种相克
        //防御：步---骑---策----弓----器---步   步兵对骑兵防御+10% 以此类推
        //攻击：步---弓---骑----器----策---步   步兵对弓兵攻击+10% 以此类推
        switch (p_soldierdata->m_base_type)
        {
            case base_soldier_bubing:
                p_soldierdata->m_combat_attribute.set_damage_from(base_soldier_qibing, 10);
                p_soldierdata->m_combat_attribute.set_damage_to(base_soldier_gongbing, 10);
                break;
            case base_soldier_gongbing:
                p_soldierdata->m_combat_attribute.set_damage_from(base_soldier_qixie, 10);
                p_soldierdata->m_combat_attribute.set_damage_to(base_soldier_qibing, 10);
                break;
            case base_soldier_moushi:
                p_soldierdata->m_combat_attribute.set_damage_from(base_soldier_gongbing, 10);
                p_soldierdata->m_combat_attribute.set_damage_to(base_soldier_bubing, 10);
                break;
            case base_soldier_qibing:
                p_soldierdata->m_combat_attribute.set_damage_from(base_soldier_moushi, 10);
                p_soldierdata->m_combat_attribute.set_damage_to(base_soldier_qixie, 10);
                break;
            case base_soldier_qixie:
                p_soldierdata->m_combat_attribute.set_damage_from(base_soldier_bubing, 10);
                p_soldierdata->m_combat_attribute.set_damage_to(base_soldier_moushi, 10);
                break;
        }
        //状态效果
        updateCombatAttribute(p_soldierdata->m_pec, 3, p_soldierdata->m_combat_attribute);
    }
    q.free_result();
    return 0;
}

int GeneralDataMgr::reloadBaseStrongholdGenerals()
{
    cout<<"************ GeneralDataMgr::reloadBaseStrongholdGenerals() ******************"<<endl;

    Query q(GetDb());
    std::map<int, boost::shared_ptr<StrongholdData> >::iterator it = m_stronghold_data_map.begin();
    while (it != m_stronghold_data_map.end())
    {
        if (it->second.get())
        {
            StrongholdData* ps = it->second.get();
            //初始化关卡兵力信息
            q.get_result("SELECT pos,name,spic,color,stype,hp,attack,pufang,cefang,str,wisdom,skill FROM base_stronghold_generals WHERE stronghold=" + LEX_CAST_STR(it->second->m_id));
            CHECK_DB_ERR(q);
            while (q.fetch_row())
            {
                int pos = q.getval();
                if (pos >= 9)
                {
                    pos = 9;
                }
                else if (pos < 1)
                {
                    pos = 1;
                }
                boost::shared_ptr<StrongholdGeneralData> sg;
                if (!(it->second->m_generals[pos-1].get()))
                {
                    sg.reset(new (StrongholdGeneralData));
                    it->second->m_generals[pos-1] = sg;
                }
                else
                {
                    sg = it->second->m_generals[pos-1];
                }
                sg->m_pos = pos;
                sg->m_name = q.getstr();
                sg->m_spic = q.getval();
                sg->m_color = q.getval();
                sg->m_stype = q.getval();
                sg->m_hp = q.getval();
                sg->m_attack = q.getval();
                sg->m_pufang = q.getval();
                sg->m_cefang = q.getval();
                sg->m_str = q.getval();
                sg->m_int = q.getval();
                sg->m_speSkill = getSpeSkill(q.getval());
                sg->m_level = it->second->m_level;

                sg->m_baseSoldier = GeneralDataMgr::getInstance()->GetBaseSoldier(sg->m_stype);

                //boss级别的怪
                if (ps->m_id % 4 == 0 && ps->m_name == sg->m_name)
                {
                    json_spirit::Object boss;
                    boss.push_back( Pair("spic", sg->m_spic) );
                    boss.push_back( Pair("name", sg->m_name) );
                    boss.push_back( Pair("level",sg->m_level) );
                    boss.push_back( Pair("color",sg->m_color) );
                    if (sg->m_speSkill.get())
                    {
                        boss.push_back( Pair("skill", sg->m_speSkill->name) );
                    }
                    ps->m_baseStage->boss_list.insert(ps->m_baseStage->boss_list.begin(), boss);
                }
            }
            q.free_result();

            //读入攻略
            q.get_result("select id,input,hurt,attackerName,alevel,aAttack from battle_records where type=1 and archive=1 and defender="
                    + LEX_CAST_STR(it->first) + " order by input,hurt");
            CHECK_DB_ERR(q);
            while (q.fetch_row())
            {
                int id = q.getval();
                time_t input = q.getval();
                int hurt = q.getval();
                std::string name = q.getstr();
                int level = q.getval();
                int attack = q.getval();

                ps->m_raiders.load(name, level, id, attack, hurt, input);
            }
            q.free_result();
        }
        ++it;
    }
    return 0;
}

int GeneralDataMgr::reloadBaseEquipments()
{
    cout<<"************ GeneralDataMgr::reloadBaseEquipments() ******************"<<endl;

    Query q(GetDb());

    int max_id = 0;
    //初始化基础兵种信息
    q.get_result("SELECT id,type,name,quality,up_quality,value,value2,price,memo FROM base_equipment WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseEquipment> eq = GetBaseEquipment(id);
        if (!eq.get())
        {
            eq.reset(new baseEquipment);
        }
        m_base_equipments[id] = eq;
        eq->baseid = id;
        eq->type = q.getval();
        eq->name = q.getstr();
        eq->slot = eq->type;
        eq->quality = q.getval();
        eq->up_quality = q.getval();
        eq->baseValue = q.getval();
        eq->baseValue2 = q.getval();
        eq->basePrice = q.getval();
        eq->desc = q.getstr();

        eq->link_name = eq->name;
        addColor(eq->link_name, eq->quality);

        switch (eq->up_quality)
        {
            case 0:
                eq->needLevel = 0;
                break;
            case 1:
                eq->needLevel = 0;
                break;
            case 2:
                eq->needLevel = 30;
                break;
            case 3:
                eq->needLevel = 50;
                break;
            case 4:
                eq->needLevel = 60;
                break;
            case 5:
                eq->needLevel = 70;
                break;
            case 6:
                eq->needLevel = 80;
                break;
            case 7:
                eq->needLevel = 90;
                break;
            //日本70级活动红色装备
            case 8:
                eq->needLevel = 70;
                break;
            default:
                eq->needLevel = 200;
                break;
        }
        max_id = id;
    }
    q.free_result();

    for (int i = 1; i <= max_id; ++i)
    {
        boost::shared_ptr<baseEquipment> eq = GetBaseEquipment(i);
        boost::shared_ptr<baseEquipment> eq2 = GetBaseEquipment(i+equip_slot_max);
        if (eq.get() && eq2.get())
        {
            if (eq->type == eq2->type)
            {
                if ((eq->quality + 1) != eq2->quality && (eq->up_quality + 1) != eq2->up_quality)
                {
                    //cout<<"type:"<<eq->type<<",quality:"<<eq->quality<<",up_quality:"<<eq->up_quality<<endl;
                    //ERR();
                }
                else
                {
                    eq->m_next = eq2;
                }
            }
        }
    }

    //删除垃圾数据
    q.execute("delete from char_equipment where state='1' and deleteTime<unix_timestamp()");
    //q.execute("delete from char_generals where state='1' and delete_time<unix_timestamp()");
    CHECK_DB_ERR(q);
    return 0;
}

void GeneralDataMgr::updateSeason()
{
    ++m_season;
    if (m_season == 5)
    {
        m_season = 1;
        ++m_year;
    }
#if 0
    boost::shared_ptr<DateInfo> d = GetDate();
    if (d.get())
    {
        d->year = m_year;
        d->season = m_season;
        switch (m_season)
        {
            case 1:
                d->effect = strSpringEffect;
                break;
            case 2:
                d->effect = strSummerEffect;
                break;
            case 3:
                d->effect = strAutumnEffect;
                break;
            default:
                d->effect = strWinterEffect;
                break;
        }
    }
    //设置季节伤害加成
    updateGlobalFac(m_season);
#endif

    InsertSaveDb("replace into custom_settings (code,value) values ('season'," + LEX_CAST_STR(m_season) + ")");
    InsertSaveDb("replace into custom_settings (code,value) values ('year'," + LEX_CAST_STR(m_year) + ")");
}

void GeneralDataMgr::updateRaceRewardTime()
{
    m_race_reward_time = m_race_reward_time + 3 * 86400;
    InsertSaveDb("replace into custom_settings (code,value) values ('race_reward_time'," + LEX_CAST_STR(m_race_reward_time) + ")");
}

int GeneralDataMgr::resetShopGoods()
{
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get())
        {
            it->second->refreshShopGoods(0);
        }
        ++it;
    }
    InsertSaveDb("TRUNCATE TABLE char_shop_goods");
    return HC_SUCCESS;
}

#if 0
int GeneralDataMgr::refreshSmeltTask()
{
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get())
        {
            SmeltMgr::getInstance()->Refresh(it->first, 0);
        }
        ++it;
    }
    return HC_SUCCESS;
}
#endif

int GeneralDataMgr::getSpeSkillType(const std::string& type)
{
    return m_speskill_type_string_map[type];
}

int GeneralDataMgr::getSpeSkillTarget(const std::string& target)
{
    return m_target_string_map[target];
}

baseBuff* GeneralDataMgr::getBaseBuff(int id)
{
    std::map<int, boost::shared_ptr<baseBuff> >::iterator it = m_base_buff_map.find(id);
    if (it != m_base_buff_map.end())
    {
        return it->second.get();
    }
    return NULL;
}

//加载基础数据
int GeneralDataMgr::reloadOtherBaseData()
{
    m_year = getInt("year");
    if (0 == m_year)
    {
        m_year = iSplsFirstYear;
        InsertSaveDb("replace into custom_settings (code,value) values ('year'," + LEX_CAST_STR(m_year) + ")");
    }

    m_season = getInt("season");
    if (0 == m_season)
    {
        m_season = 1;
        InsertSaveDb("replace into custom_settings (code,value) values ('season'," + LEX_CAST_STR(m_season) + ")");
    }

    m_race_reward_time = getInt("race_reward_time");
    if (0 == m_race_reward_time)
    {
        time_t t_now = time(NULL);
        struct tm tm;
        struct tm *t = &tm;
        localtime_r(&t_now, t);
        if (t->tm_hour >= 24 || t->tm_hour < 0)
        {
            ;
        }
        else
        {
            t->tm_hour = 23;
            t->tm_min = 0;
            t->tm_sec = 0;
        }
        m_race_reward_time = (int)(mktime(t));
        InsertSaveDb("replace into custom_settings (code,value) values ('race_reward_time'," + LEX_CAST_STR(m_race_reward_time) + ")");
    }

    //是否开启防沉迷
    m_enable_chenmi = getInt("enable_chenmi") > 0;

    //角色缺省金币
    g_default_gold = getInt("default_gold", DEFAULT_GOLD);
    //角色缺省银币
    g_default_silver = getInt("default_silver", DEFAULT_SILVER);
    //角色缺省军令
    g_default_ling = getInt("default_ling", DEFAULT_LING);
    //缺省军粮
    g_default_supply = getInt("default_supply", DEFAULT_SUPPLY);
    //是否允许调试命令行
    g_enable_debug_cmd_line = getInt("enable_debug_cmdline", 0);

    //临时vip持续时间-秒
    g_tmpvip_secs = getInt("temp_vip_hours", (iTmpVip_time/3600)) * 3600;

    //扫荡立即完成需要的VIP等级
    g_sweep_fast_vip = getInt("sweep_fast_vip",iSweepEliteFinishVip);

    //洗髓折扣
    g_wash_discount = getInt("wash_discount", 100);
    for (int i = 0; i < 5; ++i)
    {
        g_wash_real_cost[i] = iWashConfig[i][1] * g_wash_discount / 100;
        if (g_wash_real_cost[i] < 1)
        {
            g_wash_real_cost[i] = 1;
        }
    }

    //重生折扣
    g_reborn_discount = getInt("reborn_discount", 100);
    if (g_reborn_discount < 0 || g_reborn_discount > 100)
    {
        g_reborn_discount = 100;
    }

    //军团收益系数
    gCorpsFactor = getInt("corpsFactor", 100);
    //贸易收益系数
    gTradeFactor = getInt("tradeFactor", 100);
    //八卦阵收益系数
    gMazeFactor = getInt("mazeFactor", 100);
    //阵营战收益系数
    gCampRaceFactor = getInt("campRaceFactor", 100);
    //竞技场收益系数
    gArenaFactor = getInt("arenaFactor", 100);
    //屯田收益系数
    gFarmFactor = getInt("farmFactor", 100);
    //神兽实际收益
    gBossFactor = getInt("bossFactor", 100);

    //地图开放
    gMaxOpenMap = getInt("open_map");
    if (gMaxOpenMap < 1 || gMaxOpenMap > max_map_id)
    {
        gMaxOpenMap = max_map_id;
    }

    //免费VIP4开放
    m_free_vip4_endtime = getInt("free_vip4_endtime", 0);
    if (m_free_vip4_endtime < time(NULL))
    {
        m_free_vip4_endtime = 0;
        setInt("free_vip4_endtime", 0);
    }

    //测试充值
    gTestRechargeType = getInt("enable_test_recharge", 0);

    //两个阵营阵营战连胜的天数
    //m_camp_race_wins[0] = getInt("camp_race_win1");
    //m_camp_race_wins[1] = getInt("camp_race_win2");

    m_base_buff_map[buff_baoji].reset(new baseBuff(buff_baoji));
    m_base_buff_map[buff_gedang].reset(new baseBuff(buff_gedang));
    m_base_buff_map[buff_shipo].reset(new baseBuff(buff_shipo));
    m_base_buff_map[buff_dodge].reset(new baseBuff(buff_dodge));

    m_base_buff_map[buff_mingzhong].reset(new baseBuff(buff_mingzhong));
    m_base_buff_map[buff_fangyu].reset(new baseBuff(buff_fangyu));
    m_base_buff_map[buff_gongji].reset(new baseBuff(buff_gongji));
    m_base_buff_map[buff_no_chaos].reset(new baseBuff(buff_no_chaos));

    m_base_buff_map[debuff_baoji].reset(new baseBuff(debuff_baoji));
    m_base_buff_map[debuff_gedang].reset(new baseBuff(debuff_gedang));
    m_base_buff_map[debuff_shipo].reset(new baseBuff(debuff_shipo));
    m_base_buff_map[debuff_dodge].reset(new baseBuff(debuff_dodge));
    m_base_buff_map[debuff_miss].reset(new baseBuff(debuff_miss));

    m_base_buff_map[debuff_fangyu].reset(new baseBuff(debuff_fangyu));
    m_base_buff_map[debuff_gongji].reset(new baseBuff(debuff_gongji));
    m_base_buff_map[debuff_chaos].reset(new baseBuff(debuff_chaos));
    m_base_buff_map[debuff_taobing].reset(new baseBuff(debuff_taobing));

    m_base_buff_map[buff_pu_fang].reset(new baseBuff(buff_pu_fang));
    m_base_buff_map[buff_ce_fang].reset(new baseBuff(buff_ce_fang));

    m_base_buff_map[debuff_pu_fang].reset(new baseBuff(debuff_pu_fang));
    m_base_buff_map[debuff_ce_fang].reset(new baseBuff(debuff_ce_fang));

    m_base_buff_map[effect_sub_nuqi].reset(new baseBuff(effect_sub_nuqi));
    m_base_buff_map[effect_add_nuqi].reset(new baseBuff(effect_add_nuqi));
    m_base_buff_map[effect_add_hp].reset(new baseBuff(effect_add_hp));
    m_base_buff_map[effect_add_hp_percent].reset(new baseBuff(effect_add_hp_percent));

    m_base_buff_map[effect_soul_link].reset(new baseBuff(effect_soul_link));

    m_speskill_type_string_map["addBaoji"] = buff_baoji,         //提升暴击率
    m_speskill_type_string_map["addGedang"] = buff_gedang,        //提升格挡率
    m_speskill_type_string_map["addShipo"] = buff_shipo,        //提升识破率
    m_speskill_type_string_map["addDuoshan"] = buff_dodge,        //提升闪避率
    m_speskill_type_string_map["addMingzhong"] = buff_mingzhong,//提升命中率
    m_speskill_type_string_map["addFang"] = buff_fangyu,        //提升防御力
    m_speskill_type_string_map["addGong"] = buff_gongji,        //提升攻击力
    m_speskill_type_string_map["noHunluan"] = buff_no_chaos,    //免疫混乱
    m_speskill_type_string_map["addPufang"] = buff_pu_fang,        //提升普通防御力
    m_speskill_type_string_map["addCefang"] = buff_ce_fang,        //提升策略防御力

    m_speskill_type_string_map["subBaoji"] = debuff_baoji,        //降低暴击率
    m_speskill_type_string_map["subGedang"] = debuff_gedang,    //降低格挡率
    m_speskill_type_string_map["subShipo"] = debuff_shipo,        //降低识破率
    m_speskill_type_string_map["subDuoshan"] = debuff_dodge,    //降低躲闪率
    m_speskill_type_string_map["subMingzhong"] = debuff_miss,    //降低命中率

    m_speskill_type_string_map["subFang"] = debuff_fangyu,        //降低防御力
    m_speskill_type_string_map["subGong"] = debuff_gongji,        //降低攻击力
    m_speskill_type_string_map["hunluan"] = debuff_chaos,        //混乱
    m_speskill_type_string_map["taobing"] = debuff_taobing,        //逃兵

    m_speskill_type_string_map["subNuqi"] = effect_sub_nuqi,     //降低怒气
    m_speskill_type_string_map["addNuqi"] = effect_add_nuqi,    //增加怒气
    m_speskill_type_string_map["addHp"] = effect_add_hp,        //增加气血
    m_speskill_type_string_map["addHpPercent"] = effect_add_hp_percent,    //增加气血百分比
    m_speskill_type_string_map["soulLink"] = effect_soul_link;    //灵魂锁链

    m_target_string_map["target"] = range_single;
    m_target_string_map["self"] = range_self_single;
    m_target_string_map["other-side-team"] = range_all;
    m_target_string_map["own-team"] = range_self_all;

    cout<<"************ GeneralDataMgr::reloadOtherBaseData() ******************"<<endl;

    //基础装备表
    reloadBaseEquipments();

    cout<<"************ GeneralDataMgr::reloadOtherBaseData() base_treasures ******************"<<endl;

    Query q(GetDb());
    //基础宝物
    q.get_result("SELECT id,spic,type,value,quality,max_count,sellPrice,name,memo,currency,gold_to_buy,invalidTime FROM base_treasures WHERE inUse=1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseTreasure> tr = GetBaseTreasure(id);
        if (!tr.get())
        {
            tr.reset(new baseTreasure);
            tr->canMakeInited = false;
        }
        m_base_treasures[id] = tr;
        tr->id = id;
        tr->spic = q.getval();
        tr->usage = q.getval();
        tr->value = q.getval();
        tr->quality = q.getval();
        tr->max_size = q.getval();
        tr->sellPrice = q.getval();
        tr->name = q.getstr();
        tr->memo = q.getstr();
        tr->currency = (q.getval() > 0);
        tr->gold_to_buy = q.getval();
        tr->invalidTime = q.getval();

        tr->b_used_for_task = (tr->usage == ITEM_USAGE_FOR_TASK);
    }
    q.free_result();

    cout<<"************ GeneralDataMgr::reloadOtherBaseData() base_zhens ******************"<<endl;

    q.get_result("select id,name,memo,power,attack_score,xixue,action,target,nuqiLeft,effect1,effect2,effect3 from base_spe_skill where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<specialSkill> spesk = getSpeSkill(id);
        if (spesk.get())
        {
            ERR();
            cout<<"table: base_spe_skill,id:"<<id<<endl;
            exit(0);
        }
        spesk.reset(new specialSkill);
        spesk->id = id;
        spesk->name = q.getstr();
        spesk->memo = q.getstr();
        spesk->damage_fac = q.getval();
        spesk->attack_score = q.getval();
        spesk->xixue_percent = q.getval();
        std::string action = q.getstr();
        if (action == "heal")
        {
            spesk->action_type = 2;
        }
        else
        {
            spesk->action_type = 1;
        }
        std::string target = q.getstr();
        if (target == "dot")
        {
            spesk->attack_type = range_single;
        }
        else if (target == "line")
        {
            spesk->attack_type = range_chuantou;
        }
        else if (target == "side")
        {
            spesk->attack_type = range_fenlie;
        }
        else if (target == "all")
        {
            spesk->attack_type = range_all;
        }
        else if (target == "all-front")
        {
            spesk->attack_type = range_three;
        }
        else
        {
            ERR();
            cout<<"table: base_spe_skill,unknow 'target':"<<target<<",id:"<<id<<endl;
            exit(0);
        }
        spesk->casted_shiqi = q.getval();

        //特效
        std::string effect1 = q.getstr();
        std::string effect2 = q.getstr();
        std::string effect3 = q.getstr();
        //cout<<"parse effect ,skill"<<id<<"-->";
        if (!spesk->parseEffect(effect1))
        {
            cout<<"table: base_spe_skill,effect1 error,id:"<<id<<endl;
            exit(1);
        }
        if (!spesk->parseEffect(effect2))
        {
            cout<<"table: base_spe_skill,effect2 error,id:"<<id<<endl;
            exit(1);
        }
        if (!spesk->parseEffect(effect3))
        {
            cout<<"table: base_spe_skill,effect3 error,id:"<<id<<endl;
            exit(1);
        }
        m_spe_skill_map[id] = spesk;
    }
    q.free_result();

    //INFO("load base zhens");
    //初始化基础阵型
    q.get_result("select type,name,level1,level2,level3,level4,level5 from base_zhens where enable=1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int type = q.getval();
        boost::shared_ptr<BaseZhenData> bZhen = GetBaseZhen(type);
        if (!bZhen.get())
        {
            bZhen.reset(new BaseZhenData);
            m_base_zhens[type] = bZhen;
            //INFO("add base zhen "<<type);
        }
        bZhen->m_type = type;
        bZhen->m_name = q.getstr();
        for (size_t i = 0; i < 5; ++i)
        {
            bZhen->m_open_pos[i] = q.getval();
            assert(bZhen->m_open_pos[i] >= 1 && bZhen->m_open_pos[i] <= 9);
        }
    }
    q.free_result();

    cout<<"************ GeneralDataMgr::reloadOtherBaseData() base_weapons ******************"<<endl;

    //初始化基础武将信息
    q.get_result("SELECT gid,name,chuohao,jxl,spic,stype,tong,str,wisdom,memo,baowu,bwType,bwBaseValue,bwAddPerLev,baowu_spic,chengzhang,chuancheng_cnt,spe_skill,quality,tj1,tj2,tj3,good_at,tianfu,tianfuMemo,tongMore,wisdomMore,strMore,attack,hp,wumian,cemian,cri,r_cri,parry,r_parry,hit,dodge,shipo,r_shipo,nuqi FROM base_generals WHERE 1 order by gid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int gid = q.getval();
        GeneralTypeData* p_generaldata = GetBaseGeneral(gid).get();
        if (!p_generaldata)
        {
            p_generaldata = new GeneralTypeData;
            m_base_generals_map[gid].reset(p_generaldata);
        }
        p_generaldata->m_gid = gid;
        p_generaldata->m_name = q.getstr();
        p_generaldata->m_nickname = q.getstr();
        p_generaldata->m_jxl = q.getstr();
        p_generaldata->m_spic = q.getval();
        p_generaldata->m_stype = q.getval();
        p_generaldata->base_tongyu = q.getval();
        p_generaldata->base_str = q.getval();
        p_generaldata->base_int = q.getval();
        p_generaldata->m_desc = q.getstr();
        p_generaldata->m_baowu = q.getstr();
        p_generaldata->m_baowu_type = q.getval();
        p_generaldata->m_baowu_baseval = q.getval();
        p_generaldata->m_baowu_addperlev = q.getval();
        p_generaldata->m_baowu_spic = q.getval();
        p_generaldata->m_base_chengzhang = q.getnum();
        p_generaldata->m_inherit_cnt = q.getval();
        p_generaldata->m_spe_skill_id = q.getval();
        p_generaldata->m_quality = q.getval();

        p_generaldata->m_tj_baoshi_list.push_back(q.getstr());
        p_generaldata->m_tj_baoshi_list.push_back(q.getstr());
        p_generaldata->m_tj_baoshi_list.push_back(q.getstr());

        p_generaldata->m_speSkill = getSpeSkill(p_generaldata->m_spe_skill_id);

        //擅长
        p_generaldata->m_good_at = q.getval();

        //tianfu  tianfuMemo  tongMore    wisdomMore  strMore attack  hp  wumian  cemian  cri r_cri   parry   r_parry hit dodge   shipo   r_shipo nuqi
        p_generaldata->m_new_tianfu.m_name = q.getstr();
        p_generaldata->m_new_tianfu.m_memo = q.getstr();
        p_generaldata->m_new_tianfu.m_more_tong = q.getval();
        p_generaldata->m_new_tianfu.m_more_int = q.getval();
        p_generaldata->m_new_tianfu.m_more_str = q.getval();
        p_generaldata->m_new_tianfu.m_combatAttr.org_inspired(q.getval());
        p_generaldata->m_new_tianfu.m_more_hp = q.getval();
        p_generaldata->m_new_tianfu.m_combatAttr.sub_damage(1, q.getval());
        p_generaldata->m_new_tianfu.m_combatAttr.sub_damage(2, q.getval());
        p_generaldata->m_new_tianfu.m_combatAttr.add_special_attack(special_attack_baoji, 10*q.getval());
        p_generaldata->m_new_tianfu.m_combatAttr.add_resist(special_attack_baoji, 10*q.getval());
        p_generaldata->m_new_tianfu.m_combatAttr.add_special_attack(special_attack_parry, 10*q.getval());
        p_generaldata->m_new_tianfu.m_combatAttr.add_resist(special_attack_parry, 10*q.getval());
        p_generaldata->m_new_tianfu.m_combatAttr.add_resist(special_attack_dodge, 10*q.getval());
        p_generaldata->m_new_tianfu.m_combatAttr.add_special_attack(special_attack_dodge, 10*q.getval());
        p_generaldata->m_new_tianfu.m_combatAttr.add_special_attack(special_attack_shipo, 10*q.getval());
        p_generaldata->m_new_tianfu.m_combatAttr.add_resist(special_attack_shipo, 10*q.getval());
        p_generaldata->m_new_tianfu.m_combatAttr.org_shiqi(q.getval());

        p_generaldata->m_tianfu = 0;
        if (p_generaldata->m_new_tianfu.m_more_str)
        {
            p_generaldata->m_tianfu = 3;
        }
        else if (p_generaldata->m_new_tianfu.m_more_int)
        {
            p_generaldata->m_tianfu = 2;
        }
        else if (p_generaldata->m_new_tianfu.m_more_tong)
        {
            p_generaldata->m_tianfu = 1;
        }

        if (GetBaseSoldier(p_generaldata->m_stype).get() == NULL)
        {
            cout<<"general type "<<gid<<",unknow stype "<<p_generaldata->m_stype<<endl;
            exit(1);
        }
    }
    q.free_result();

    cout<<"************ GeneralDataMgr::reloadOtherBaseData() strongholds ******************"<<endl;

    //std::map<int, int> m_zhen_open_map;
    m_zhen_open_map.clear();

    q.get_result("select open_stronghold,type from base_zhens order by type");
    while (q.fetch_row())
    {
        int stronghold = q.getval();
        int type = q.getval();
        m_zhen_open_map[stronghold] = type;
    }
    q.free_result();

    if (m_zhen_open_map.size() == 0)
    {
        m_zhen_open_map[76] = 2;
        m_zhen_open_map[96] = 3;
        m_zhen_open_map[106] = 4;
        m_zhen_open_map[116] = 5;
        m_zhen_open_map[126] = 6;
        m_zhen_open_map[136] = 7;
        m_zhen_open_map[146] = 8;
    }

    //初始化关卡信息
    q.get_result("SELECT s.id,s.level,s.mapid,s.stageid,s.strongholdpos,s.group,s.name,s.spic,s.color,s.type,s.model,s.x,s.y,s.message,s.stateNum,s.failMsg,s.needSupply,s.hit,s.crit,s.shipo,s.parry,s.resist_hit,s.resist_crit,s.resist_shipo,s.resist_parry,s.gongxun,s.rob_supply FROM base_stronghold as s LEFT JOIN base_stronghold_loots AS l ON s.id = l.id WHERE 1 order by s.id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int strongholdid = q.getval();
        boost::shared_ptr<StrongholdData> p_strongholddata = GetStrongholdData(strongholdid);
        if (!p_strongholddata.get())
        {
            p_strongholddata.reset(new StrongholdData);
            m_stronghold_data_map[strongholdid] = p_strongholddata;
        }
        p_strongholddata->m_id = strongholdid;
        p_strongholddata->m_level = q.getval();
        p_strongholddata->m_map_id = q.getval();
        p_strongholddata->m_stage_id = q.getval();
        p_strongholddata->m_strongholdpos = q.getval();
        p_strongholddata->m_group = q.getval();
        p_strongholddata->m_name = q.getstr();
        p_strongholddata->m_spic = q.getval();
        p_strongholddata->m_color = q.getval();
        p_strongholddata->m_isepic = q.getval();
        p_strongholddata->m_model = q.getval();
        p_strongholddata->m_x = q.getval();
        p_strongholddata->m_y = q.getval();
        p_strongholddata->m_chat = q.getstr();
        p_strongholddata->m_stateNum = q.getval();
        p_strongholddata->m_failMsg = q.getstr();

        p_strongholddata->m_need_supply = q.getval();

        //特性hit,crit,shipo,parry,resist_hit,resist_crit,resist_shipo,resist_parry
        p_strongholddata->m_combat_attribute.special_resist(special_attack_dodge, 10 * q.getval());
        p_strongholddata->m_combat_attribute.special_attack(special_attack_baoji, 10 * q.getval());
        p_strongholddata->m_combat_attribute.special_attack(special_attack_shipo, 10 * q.getval());
        p_strongholddata->m_combat_attribute.special_attack(special_attack_parry, 10 * q.getval());
        p_strongholddata->m_combat_attribute.special_attack(special_attack_dodge, 10 * q.getval());
        p_strongholddata->m_combat_attribute.special_resist(special_attack_baoji, 10 * q.getval());
        p_strongholddata->m_combat_attribute.special_resist(special_attack_shipo, 10 * q.getval());
        p_strongholddata->m_combat_attribute.special_resist(special_attack_parry, 10 * q.getval());
        p_strongholddata->m_combat_attribute.enable();

        p_strongholddata->m_Item = lootMgr::getInstance()->getStrongholdLootInfo(strongholdid);

        ItemToObj((p_strongholddata->m_Item.get()), p_strongholddata->m_loot);
        //直接根据有无掉落来确定是否精英关卡
        if (!p_strongholddata->m_Item.get())
        {
            p_strongholddata->m_isepic = 1;
        }
        else if (p_strongholddata->m_isepic == 1)
        {
            p_strongholddata->m_isepic = 2;
        }

        p_strongholddata->m_gongxun = q.getval();

        //掠夺获得的军粮数量
        p_strongholddata->m_rob_supply = q.getval();

        boost::shared_ptr<baseMap> bm = GetBaseMap(p_strongholddata->m_map_id);
        if (bm.get() && (p_strongholddata->m_stage_id >=1 && p_strongholddata->m_stage_id <=3) && bm->stages[p_strongholddata->m_stage_id-1].get())
        {
            if (p_strongholddata->m_strongholdpos>=1 && p_strongholddata->m_strongholdpos<=25)
            {
                p_strongholddata->m_baseStage = bm->stages[p_strongholddata->m_stage_id -1];
                bm->stages[p_strongholddata->m_stage_id -1]->_baseStrongholds[p_strongholddata->m_strongholdpos-1] = p_strongholddata;
                bm->stages[p_strongholddata->m_stage_id -1]->size++;
                //加入掉落
                bm->stages[p_strongholddata->m_stage_id -1]->addLoot(p_strongholddata->m_Item, p_strongholddata->m_loot);
                p_strongholddata->m_baseStage->spic = p_strongholddata->m_spic;
            }
        }
        else
        {
            ERR();
        }

        if (m_zhen_open_map.find(strongholdid) != m_zhen_open_map.end())
        {
            p_strongholddata->m_open_zhen = m_zhen_open_map[strongholdid];
            p_strongholddata->m_open_zhen_level = 5;
        }

        //武将上限
        for (int i = 0; i < sizeof(iGeneralLimitStronghold)/sizeof(int); ++i)
        {
            if (iGeneralLimitStronghold[i] > strongholdid)
            {
                p_strongholddata->m_general_limit = i;
                break;
            }
        }
        p_strongholddata->m_guide_id = 0;
        p_strongholddata->m_guide_fail = 0;
    }
    q.free_result();

    //基础官职
    q.get_result("SELECT id,name,prestige,salary,skill_id,farm_id FROM base_offical WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseoffical> p_bo;
        p_bo.reset(new baseoffical);
        p_bo->m_id = id;
        p_bo->m_name = q.getstr();
        p_bo->need_prestige = q.getval();
        p_bo->m_salary = q.getval();
        p_bo->m_sid = q.getval();
        p_bo->m_fid = q.getval();
        m_base_officals[id] = p_bo;
    }
    q.free_result();

    //基础官职技能
    q.get_result("SELECT id,name,type,add_per_level,effect,memo FROM base_offical_skills WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseofficalskill> p_bos;
        p_bos.reset(new baseofficalskill);
        p_bos->m_sid = id;
        p_bos->m_spic = id;
        p_bos->m_name = q.getstr();
        p_bos->m_type = q.getval();
        p_bos->m_add_per_level = q.getval();
        p_bos->m_effect = q.getstr();
        p_bos->m_memo = q.getstr();
        m_base_offical_skills[id] = p_bos;
    }
    q.free_result();

    //基础官职武将
    q.get_result("SELECT gid,name,spic,stype,need_silver,need_slevel,need_offical,quality,especial,good_at FROM base_generals WHERE 1 order by need_offical,gid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<officalgenerals> p_generaldata;
        p_generaldata.reset(new officalgenerals);
        m_base_offical_generals.push_back(p_generaldata);
        p_generaldata->m_gid = q.getval();
        p_generaldata->m_name = q.getstr();
        p_generaldata->m_spic = q.getval();
        p_generaldata->m_sid = q.getval();
        p_generaldata->m_price = q.getval();
        p_generaldata->need_slevel = q.getval();
        p_generaldata->need_offical = q.getval();
        p_generaldata->m_quality = q.getval();
        p_generaldata->m_special = q.getval() == 0;
        p_generaldata->m_good_at = q.getval();
        if (p_generaldata->m_special)
        {
            p_generaldata->need_offical = 1;
        }

        if (p_generaldata->need_slevel)
        {
            if (m_stronghold_offical_generals.find(p_generaldata->need_slevel) != m_stronghold_offical_generals.end())
            {
                std::list<boost::shared_ptr<officalgenerals> > list;
                list.push_back(p_generaldata);
                m_stronghold_offical_generals[p_generaldata->need_slevel] = list;
            }
            else
            {
                std::list<boost::shared_ptr<officalgenerals> > &list = m_stronghold_offical_generals[p_generaldata->need_slevel];
                list.push_back(p_generaldata);
            }
        }

        if (p_generaldata->need_offical)
        {
            if (m_offical_generals.find(p_generaldata->need_offical) != m_offical_generals.end())
            {
                std::list<boost::shared_ptr<officalgenerals> > list;
                list.push_back(p_generaldata);
                m_offical_generals[p_generaldata->need_offical] = list;
            }
            else
            {
                std::list<boost::shared_ptr<officalgenerals> > &list = m_offical_generals[p_generaldata->need_offical];
                list.push_back(p_generaldata);
            }
        }

        m_base_offical_generals_map[p_generaldata->m_gid] = p_generaldata;
    }
    q.free_result();

    //基础连续登录奖励
    q.get_result("SELECT id,spic,quality,ling,silver,treasure,treasure_num,name,memo FROM base_continue_login_present WHERE 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<baseLoginPresent> pblp;
        pblp.reset(new baseLoginPresent);
        pblp->id = q.getval();
        pblp->spic = q.getval();
        pblp->quality = q.getval();
        pblp->ling = q.getval();
        pblp->silver = q.getval();
        pblp->treasure = q.getval();
        pblp->treasure_num = q.getval();
        pblp->name = q.getstr();
        pblp->memo = q.getstr();
        m_base_login_present[pblp->id] = pblp;
    }
    q.free_result();
    //基础VIP奖励
    q.get_result("SELECT vip,item_type,item_id,item_nums,item_fac FROM base_vip_present WHERE 1 order by vip,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int vip = q.getval();
        boost::shared_ptr<baseVIPPresent> pbvp = getBaseVIPPresent(vip);
        Item it;
        it.type = q.getval();
        it.id = q.getval();
        it.nums = q.getval();
        it.fac = q.getval();
        if (pbvp.get())
        {
            pbvp->m_list.push_back(it);
        }
    }
    q.free_result();
    std::map<int, boost::shared_ptr<baseVIPPresent> >::iterator it = m_base_vip_present.begin();
    while (it != m_base_vip_present.end())
    {
        if (it->second.get())
            it->second->updateObj();
        ++it;
    }
    //基础充值满奖励
    q.get_result("SELECT id,needgold,prestige,silver,gold,ling,treasure,treasure_num,treasure1,treasure1_num,treasure2,treasure2_num,memo FROM base_recharge_present WHERE 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<baseRechargePresent> pbrp;
        pbrp.reset(new baseRechargePresent);
        pbrp->id = q.getval();
        pbrp->needgold = q.getval();
        pbrp->prestige = q.getval();
        pbrp->silver = q.getval();
        pbrp->gold = q.getval();
        pbrp->ling = q.getval();
        pbrp->treasure = q.getval();
        pbrp->treasure_num = q.getval();
        pbrp->treasure1 = q.getval();
        pbrp->treasure1_num = q.getval();
        pbrp->treasure2 = q.getval();
        pbrp->treasure2_num = q.getval();
        pbrp->memo = q.getstr();
        m_base_recharge_present[pbrp->id] = pbrp;
    }
    q.free_result();
    //首充礼包
    q.get_result("SELECT type,id,nums FROM base_first_recharge_gifts WHERE 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        Item itm;
        itm.type = q.getval();
        itm.id = q.getval();
        itm.nums = q.getval();
        if (itm.nums > 0)
        {
            m_first_recharge_gift.push_back(itm);
        }
    }
    q.free_result();

    //商城宝物
    q.get_result("SELECT id,be_suggest,gold_to_buy,itemType,itemId,fac FROM base_mall_goods WHERE inUse=1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseGoods> p_bg;
        p_bg.reset(new baseGoods);
        m_base_mall_goods[id] = p_bg;
        p_bg->id = id;
        p_bg->be_suggest = q.getval();
        p_bg->gold_to_buy = q.getval();
        boost::shared_ptr<Item> p_i;
        p_i.reset(new Item);
        p_i->type = q.getval();
        p_i->id = q.getval();
        p_i->fac = q.getval();
        p_i->nums = 1;
        p_bg->m_item = p_i;
        p_bg->type = p_i->type;
    }
    q.free_result();

    //商城折扣活动
	m_mall_discount_st.discount = 100;
	m_mall_discount_st.end = 0;
	m_mall_discount_st.start = 0;
	std::string mall_discount_str = getStr(CHANGE_MALL_DISCOUNT);
	if(mall_discount_str != "")
	{
		json_spirit::mValue mall_discount_value;
		json_spirit::read(mall_discount_str, mall_discount_value);
        if (mall_discount_value.type() == json_spirit::obj_type)
        {
            json_spirit::mObject& o = mall_discount_value.get_obj();
            READ_INT_FROM_MOBJ(m_mall_discount_st.discount,o,"discount");
            READ_INT_FROM_MOBJ(m_mall_discount_st.start,o,"start_time");
            READ_INT_FROM_MOBJ(m_mall_discount_st.end,o,"end_time");
        }
	}

    //随机名字表
    q.get_result("SELECT value FROM base_first_name_male WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        m_first_name_list_male.push_back(q.getstr());
    }
    q.free_result();
    q.get_result("SELECT value FROM base_second_name_male WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        m_second_name_list_male.push_back(q.getstr());
    }
    q.free_result();

    q.get_result("SELECT value FROM base_first_name_female WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        m_first_name_list_female.push_back(q.getstr());
    }
    q.free_result();
    q.get_result("SELECT value FROM base_second_name_female WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        m_second_name_list_female.push_back(q.getstr());
    }
    q.free_result();

    //基础成长率星级
    q.get_result("SELECT id,need_chengzhang,gongji,fangyu,bingli FROM base_chengzhang_stars WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseChengzhangStars> p;
        p.reset(new baseChengzhangStars);
        p->id = id;
        p->need_chengzhang = q.getnum();
        //cout<<"chengzhang "<<p->need_chengzhang<<endl;
        p->gongji = q.getval();
        p->fangyu = q.getval();
        p->bingli = q.getval();
        m_base_chengzhang_stars[id] = p;
    }
    q.free_result();
    //基础洗髓星级
    q.get_result("SELECT id,need_score,value FROM base_wash_stars WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseWashStars> p;
        p.reset(new baseWashStars);
        p->id = id;
        p->need_score = q.getval();
        p->value = q.getval();
        m_base_wash_stars[id] = p;
    }
    q.free_result();
    return 0;
}

int GeneralDataMgr::reload(int flag)
{
    cout<<"************ GeneralDataMgr::reload() ******************"<<endl;

    //重新加载所有基础表
    if (0 == flag)
    {
        //檢查重複武將
        checkCharGenerals();

        reloadMap();    //地图
        reloadStage();    //场景

        //基础装备
        reloadBaseEquipments();

        //重新加载基础兵的数据
        reloadBaseSoldier();

        //加载其他基础数据
        reloadOtherBaseData();

        //重新加载关卡兵力数据
        reloadBaseStrongholdGenerals();

        //INFO("LOAD chat MESSage ********************");
        m_combat_id = 0;
        Query q(GetDb());

        //登录的加密方式
        g_auth_type = 0;
        q.get_result("select set_value from admin_setting where set_key='login_safe'");
        if (q.fetch_row())
        {
            g_auth_type = q.getval();
        }
        cout<<"account auth type "<<g_auth_type<<endl;
        q.free_result();
        //引导
        q.get_result("select mapid,stage,stronghold,guide from base_stronghold_guide");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int mapid = q.getval();
            boost::shared_ptr<baseMap> bm = GetBaseMap(mapid);
            if (bm.get())
            {
                int stage = q.getval();
                int stronghold = q.getval();
                int guide = q.getval();
                if (stage >= 1 && stage <= 3)
                {
                    if (bm->stages[stage-1].get())
                    {
                        if (stronghold == 0)
                        {
                            bm->stages[stage-1]->stage_finish_guide = guide;
                        }
                        else if (stronghold > 25)
                        {
                            ERR();
                            cout<<"stronghold:"<<stronghold<<endl;
                        }
                        else if (stronghold > 0)
                        {
                            if (bm->stages[stage-1]->_baseStrongholds[stronghold-1].get())
                            {
                                bm->stages[stage-1]->_baseStrongholds[stronghold-1]->m_guide_id = guide;
                            }
                            else
                            {
                                ERR();
                                cout<<"stronghold NULL:"<<stronghold<<endl;
                            }
                        }
                        else
                        {
                            if (bm->stages[stage-1]->_baseStrongholds[-stronghold-1].get())
                            {
                                bm->stages[stage-1]->_baseStrongholds[-stronghold-1]->m_guide_fail = guide;
                            }
                            else
                            {
                                ERR();
                                cout<<"stronghold NULL:"<<stronghold<<endl;
                            }
                        }
                    }
                    else
                    {
                        ERR();
                        cout<<"stage:"<<stage<<endl;
                    }
                }
                else
                {
                    ERR();
                    cout<<"stage:"<<stage<<endl;
                }
            }
            else
            {
                ERR();
                cout<<"mapid:"<<mapid<<endl;
            }
        }
        q.free_result();

        //开启时设置全部都不在线
        q.execute("update admin_char set cstat='0' where 1");
        CHECK_DB_ERR(q);

        q.execute("update charactors set nick='[]'");

        q.get_result("select max(id) from battle_records");
        if (q.GetErrno())
        {
            CHECK_DB_ERR(q);
            exit(1);
        }
        if (q.fetch_row())
        {
            m_combat_id = q.getubigint();
        }
        q.free_result();

        m_general_id = 0;
        q.get_result("select max(id) from char_generals");
        if (q.GetErrno())
        {
            CHECK_DB_ERR(q);
            exit(1);
        }
        if (q.fetch_row())
        {
            m_general_id = q.getval();
        }
        q.free_result();

        m_equipt_id = 0;
        q.get_result("select max(id) from char_equipment");
        if (q.GetErrno())
        {
            CHECK_DB_ERR(q);
            exit(1);
        }
        if (q.fetch_row())
        {
            m_equipt_id = q.getval();
        }
        q.free_result();

        m_gem_id = 0;
        q.get_result("select max(id) from char_treasures");
        if (q.GetErrno())
        {
            CHECK_DB_ERR(q);
            exit(1);
        }
        if (q.fetch_row())
        {
            m_gem_id = q.getval();
        }
        q.free_result();

        m_charactor_id = 0;
        q.get_result("select max(id) from charactors");
        if (q.GetErrno())
        {
            CHECK_DB_ERR(q);
            exit(1);
        }
        if (q.fetch_row())
        {
            m_charactor_id = q.getval();
        }
        q.free_result();

        int first_charactor_id = getInt("charactor_id", 0);
        if (first_charactor_id > m_charactor_id)
        {
            m_charactor_id = first_charactor_id;
        }
        loadLang();

#if 0
        q.get_result("select id,cid from char_baoshi where 1");
        while (q.fetch_row())
        {
            int bid = q.getval();
            int cid = q.getval();
            m_baoshi_map[bid] = cid;
        }
        q.free_result();
#endif
        q.get_result("select id,cid from char_generals where 1");
        while (q.fetch_row())
        {
            int gid = q.getval();
            int cid = q.getval();
            m_general_map[gid] = cid;
        }
        q.free_result();

        //映射名字和id关系
        q.get_result("SELECT id,name FROM charactors WHERE 1");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int id = q.getval();
            std::string cname = q.getstr();
            m_charid_map[cname] = id;
        }
        q.free_result();

#if 0
        for (int i = 0; i < max_map_id; ++i)
        {
            cout<<"map "<<(i+1)<<":";
            if (m_base_maps[i].get())
            {
                for (int s = 0; s < 3; ++s)
                {
                    cout<<m_base_maps[i]->stages[s]->size<<",";
                }
            }
            cout<<endl;
        }
#endif
        m_inited = true;
        return 0;
    }
    //重新加载基础兵的数据
    if (0x1 & flag)
    {
        reloadBaseSoldier();
        return 0;
    }
    //重新加载关卡兵力数据
    if (0x2 & flag)
    {
        reloadBaseStrongholdGenerals();
    }
    return 0;
}

GeneralDataMgr* GeneralDataMgr::getInstance()
{
    if (m_handle == NULL)
    {
        time_t time_start = time(NULL);
        cout<<"GeneralDataMgr::getInstance()..."<<endl;
        m_handle = new GeneralDataMgr();
        m_handle->reload(0);

        /**** 初始化聊天频道 ****/

        //世界频道
        m_handle->GetWorldChannel();
        //阵营频道
        for (size_t i = 0; i <= 2; ++i)
        {
            m_handle->GetCampChannel(i);
        }
        cout<<"GeneralDataMgr::getInstance() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

void GeneralDataMgr::release()
{
    if (m_handle != NULL)
    {
        delete m_handle;
        m_handle = NULL;
    }
}

boost::shared_ptr<specialSkill> GeneralDataMgr::getSpeSkill(int id)
{
    std::map<int, boost::shared_ptr<specialSkill> >::iterator it = m_spe_skill_map.find(id);        //怒气技能
    if (it != m_spe_skill_map.end())
    {
        return it->second;
    }
    boost::shared_ptr<specialSkill> tmp;
    return tmp;
}

//获得关卡id，参数地图id，场景id，关卡编号
int GeneralDataMgr::GetStrongholdid(int mapid, int stageid, int strongholdpos)
{
    Query q(GetDb());
    int strongholdid = 0;
    q.get_result("SELECT id FROM base_stronghold WHERE mapid=" + LEX_CAST_STR(mapid) + " and stageid=" + LEX_CAST_STR(stageid) + " and strongholdpos=" + LEX_CAST_STR(strongholdpos));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        strongholdid = q.getval();
    }
    q.free_result();
    return strongholdid;
}

//获得关卡位置信息
int GeneralDataMgr::GetStrongholdPos(int& mapid, int& stageid, int strongholdid)
{
    Query q(GetDb());
    int strongholdpos = 0;
    q.get_result("SELECT mapid,stageid,strongholdpos FROM base_stronghold WHERE id=" + LEX_CAST_STR(strongholdid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        mapid = q.getval();
        stageid = q.getval();
        strongholdpos = q.getval();
    }
    q.free_result();
    return strongholdpos;
}

//获得地图描述信息
int GeneralDataMgr::GetMapMemo(int mapid, std::string& name, std::string& memo)
{
    boost::shared_ptr<baseMap> bm = GetBaseMap(mapid);
    if (bm.get())
    {
        name = bm->name;
        memo = bm->memo;
        return 0;
    }
    else
    {
        ERR();
        return -1;
    }
}

//获取场景等级限制
int GeneralDataMgr::GetStageLimitLevel(int mapid, int stageid)
{
    if (stageid > 3 || stageid < 1)
    {
        return -1;
    }
    boost::shared_ptr<baseMap> bm = GetBaseMap(mapid);
    if (bm.get())
    {
        if ((bm->stages[stageid-1]).get())
        {
            return bm->stages[stageid-1]->openLevel;
        }
    }
    ERR();
    return -1;
}

//获取场景名字
std::string GeneralDataMgr::GetStageName(int mapid, int stageid)
{
    if (stageid > 3 || stageid < 1)
    {
        return "";
    }
    boost::shared_ptr<baseMap> bm = GetBaseMap(mapid);
    if (bm.get())
    {
        if ((bm->stages[stageid-1]).get())
        {
            return bm->stages[stageid-1]->name;
        }
    }
    ERR();
    return "";
}

//获得基础兵种信息
boost::shared_ptr<BaseSoldierData> GeneralDataMgr::GetBaseSoldier(int sid)
{
    std::map<int, boost::shared_ptr<BaseSoldierData> >::iterator it = m_base_soldiers_map.find(sid);
    if (it != m_base_soldiers_map.end())
    {
        return it->second;
    }
    else
    {
        if (m_inited)
        {
            ERR();
            cout<<"!!!!s"<<sid<<endl;
        }
        boost::shared_ptr<BaseSoldierData> gd;
        gd.reset();
        return gd;
    }
}

//获得基础武将信息
boost::shared_ptr<GeneralTypeData> GeneralDataMgr::GetBaseGeneral(int gid)
{
    std::map<int, boost::shared_ptr<GeneralTypeData> >::iterator it = m_base_generals_map.find(gid);
    if (it != m_base_generals_map.end())
    {
        return it->second;
    }
    else
    {
        if (m_inited)
        {
            ERR();
            cout<<"!!!!g"<<gid<<endl;
        }
        boost::shared_ptr<GeneralTypeData> gd;
        gd.reset();
        return gd;
    }
}

//获得基础阵型信息
boost::shared_ptr<BaseZhenData> GeneralDataMgr::GetBaseZhen(int zid)
{
    std::map<int, boost::shared_ptr<BaseZhenData> >::iterator it = m_base_zhens.find(zid);
    if (it != m_base_zhens.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<BaseZhenData> gd;
        gd.reset();
        return gd;
    }
}

//获得成长率星级
boost::shared_ptr<baseChengzhangStars> GeneralDataMgr::GetBaseChengzhangStarByValue(double chengzhang)
{
    std::map<int, boost::shared_ptr<baseChengzhangStars> >::reverse_iterator r_it = m_base_chengzhang_stars.rbegin();
    while (r_it != m_base_chengzhang_stars.rend())
    {
        if (chengzhang >= r_it->second->need_chengzhang || (fabs(chengzhang - r_it->second->need_chengzhang) < 1e-6))
        {
            return r_it->second;
        }
        ++r_it;
    }
    boost::shared_ptr<baseChengzhangStars> p;
    p.reset();
    return p;
}
boost::shared_ptr<baseChengzhangStars> GeneralDataMgr::GetBaseChengzhangStarByStar(int star)
{
    std::map<int, boost::shared_ptr<baseChengzhangStars> >::iterator it = m_base_chengzhang_stars.find(star);
    if (it != m_base_chengzhang_stars.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseChengzhangStars> p;
        p.reset();
        return p;
    }
}

//获得洗髓星级
boost::shared_ptr<baseWashStars> GeneralDataMgr::GetBaseWashStarByValue(int score)
{
    std::map<int, boost::shared_ptr<baseWashStars> >::reverse_iterator r_it = m_base_wash_stars.rbegin();
    while (r_it != m_base_wash_stars.rend())
    {
        if (score >= r_it->second->need_score)
        {
            return r_it->second;
        }
        ++r_it;
    }
    boost::shared_ptr<baseWashStars> p;
    p.reset();
    return p;
}
boost::shared_ptr<baseWashStars> GeneralDataMgr::GetBaseWashStarByStar(int star)
{
    std::map<int, boost::shared_ptr<baseWashStars> >::iterator it = m_base_wash_stars.find(star);
    if (it != m_base_wash_stars.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseWashStars> p;
        p.reset();
        return p;
    }
}

void GeneralDataMgr::addCharData(boost::shared_ptr<CharData> cdata)
{
    if (cdata.get())
    {
        m_chardata_map[cdata->m_id] = cdata;
        std::string name = cdata->m_name;
        //lower_str(name);
        m_charid_map[name] = cdata->m_id;
    }
    return;
}

//获得角色信息
boost::shared_ptr<CharData> GeneralDataMgr::GetCharData(int cid)
{
    if (0 == cid)
    {
        boost::shared_ptr<CharData> p_chardata;
        return p_chardata;
    }
#ifdef GLOBAL_CHAR_LOCK
    readLock lockit(&globalchar_rwmutex);
#endif
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.find(cid);
    if (it != m_chardata_map.end())
    {
        return it->second;
    }
    else
    {
#ifdef GLOBAL_CHAR_LOCK
        lockit.unlock();
        writeLock lockit(globalchar_rwmutex);
#endif
        it = m_chardata_map.find(cid);
        if (it != m_chardata_map.end())
        {
            return it->second;
        }

        if (g_get_char_id == cid)
        {
            cout<<"------------> getCharData() <-------------"<<endl;
            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);
            if (symbols != NULL)
            {
                for (int i = 0; i < nSize; i++)
                {
                    if (symbols[i] != NULL)
                    {
                        cout << symbols[i] << endl;
                    }
                }
                free(symbols);
            }
            cout<<"------------> end xxxxxxxx <-------------"<<endl;
        }
        g_get_char_id = cid;
        //cout<<"GetCharData--> new "<<cid<<",tid:"<<syscall(SYS_gettid)<<endl;
        boost::shared_ptr<CharData> p_chardata;
        p_chardata.reset(new CharData(cid));
        if (!p_chardata->m_load_success)
        {
            p_chardata.reset();
            //cout<<"getCharData load fail,"<<cid<<endl;
            g_get_char_id = 0;
            return p_chardata;
        }
        m_chardata_map[cid] = p_chardata;
        std::string name = p_chardata->m_name;
        //lower_str(name);
        m_charid_map[name] = cid;

#ifdef GLOBAL_CHAR_LOCK
        lockit.unlock();
#endif
        taskMgr::getInstance()->queryCurTask(p_chardata);

        g_get_char_id = 0;
        return p_chardata;
    }
}

//获得角色id
int GeneralDataMgr::GetCharId(const std::string& cname)
{
    std::string name = cname;
    //lower_str(name);
    return m_charid_map[name];
}


boost::shared_ptr<CharStrongholdData> GeneralDataMgr::GetCharStrongholdData(int cid, int mapid, int stageid, int strongholdpos)
{
    if (strongholdpos >= 1 && strongholdpos <= 25)
    {
        //获得角色信息
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        CharTempoData& char_tempo = cdata->GetTempo();
        std::map<int, boost::shared_ptr<CharMapData> >::iterator it = char_tempo.CharMapsData.find(mapid);
        if (it != char_tempo.CharMapsData.end())
        {
            boost::shared_ptr<CharMapData> md = it->second;
            CharMapData::iterator itm = (*md).find(stageid);
            if (itm != (*md).end())
            {
                return itm->second->m_stronghold[strongholdpos - 1];
            }
        }
    }
    boost::shared_ptr<CharStrongholdData> p_sd;
    p_sd.reset();
    return p_sd;
}

//获得基础装备
boost::shared_ptr<baseEquipment> GeneralDataMgr::GetBaseEquipment(int baseid)
{
    std::map<int, boost::shared_ptr<baseEquipment> >::iterator it = m_base_equipments.find(baseid);
    if (it != m_base_equipments.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseEquipment> gd;
        gd.reset();
        return gd;
    }
}

//获得基础宝物
boost::shared_ptr<baseTreasure> GeneralDataMgr::GetBaseTreasure(int tid)
{
    std::map<int, boost::shared_ptr<baseTreasure> >::iterator it = m_base_treasures.find(tid);
    if (it != m_base_treasures.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseTreasure> gd;
        gd.reset();
        return gd;
    }
}

//保存角色信息
int GeneralDataMgr::SaveDb(int save_all = 0)
{
    int counts = 0;
    //15分钟保存一次
    time_t time_save = time(NULL) - 900;
#ifdef GLOBAL_CHAR_LOCK
    readLock lockit(&globalchar_rwmutex);
#endif
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    if (save_all)
    {
        INFO("************************** save all *********************************");
        while (it != m_chardata_map.end())
        {
            if (it->second.get())
            {
                it->second->Save();
            }
            ++it;
        }
    }
    else
    {
        //一次最多保存50个
        while (it != m_chardata_map.end() && (counts < 50))
        {
            if (it->second.get() && it->second->m_save_time <= time_save)
            {
                it->second->Save();
                ++counts;
            }
            ++it;
        }
    }
    return counts;
}

//基础关卡信息
boost::shared_ptr<StrongholdData> GeneralDataMgr::GetStrongholdData(int strongholdid)
{
    std::map<int, boost::shared_ptr<StrongholdData> >::iterator it = m_stronghold_data_map.find(strongholdid);
    if (it != m_stronghold_data_map.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<StrongholdData> gd;
        gd.reset();
        return gd;
    }
}

//基础关卡信息
boost::shared_ptr<StrongholdData> GeneralDataMgr::GetStrongholdData(int mapid, int stageId, int pos)
{
    if (mapid >= 1 && mapid <= 10 && m_base_maps[mapid-1].get() && stageId >= 1 && stageId <=3
        && m_base_maps[mapid-1]->stages[stageId-1].get() && pos >= 1 && pos <= 25)
    {
        return m_base_maps[mapid-1]->stages[stageId-1]->_baseStrongholds[pos-1];
    }
    else
    {
        boost::shared_ptr<StrongholdData> gd;
        gd.reset();
        return gd;
    }
}

//关卡武将信息
boost::shared_ptr<StrongholdGeneralData> GeneralDataMgr::GetStrongholdGeneralData(int gid)
{
    std::map<int, boost::shared_ptr<StrongholdGeneralData> >::iterator it = m_stronghold_generals_map.find(gid);
    if (it != m_stronghold_generals_map.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<StrongholdGeneralData> gd;
        gd.reset();
        return gd;
    }
}

//创建在线角色
boost::shared_ptr<OnlineCharactor> GeneralDataMgr::CreateOnlineCharactor(boost::shared_ptr<OnlineUser> account, uint64_t cid)
{
    boost::shared_ptr<OnlineCharactor> gd;
    gd.reset(new OnlineCharactor(account, cid));
    return gd;
}

//获得角色信息
boost::shared_ptr<OnlineCharactor> GeneralDataMgr::GetOnlineCharactor(const std::string& char_name)
{
    readLock lockit(&onlinechar_rwmutex);
    //根据角色名找到已经登录的角色
    std::map<std::string, boost::shared_ptr<OnlineCharactor> >::iterator it = m_online_charactors.find(char_name);
    if (it != m_online_charactors.end())
    {
        return it->second;
    }
    //////////////////////////////
    boost::shared_ptr<OnlineCharactor> gd;
    gd.reset();
    return gd;
}

//在线账户
boost::shared_ptr<OnlineUser> GeneralDataMgr::GetAccount(const std::string& account)
{
    readLock lockit(&onlineuser_rwmutex);
    boost::unordered_map<std::string, boost::shared_ptr<OnlineUser> >::iterator it = m_onlineuserlist.find(account);
    if (it != m_onlineuserlist.end())
    {
        return it->second;
    }
    else
    {
        lockit.unlock();
        boost::shared_ptr<OnlineUser> p;
        p.reset();
        return p;
    }
}

//玩家下线
int GeneralDataMgr::Logout(boost::shared_ptr<OnlineUser>& p)
{
    if (p->m_onlineCharactor.get())
    {
        GeneralDataMgr::getInstance()->CharactorLogout(p->m_onlineCharactor);
    }
    p->m_onlineCharactor.reset();
    //m_world_channel->Remove(p);

    //删除在线
    {
        writeLock lockit(&onlineuser_rwmutex);
        (void)m_onlineuserlist.erase(p->m_account);
    }
    INFO("GeneralDataMgr::Logout("<<p->m_account<<")"<<endl);
    p.reset();
    return 0;
}

//玩家登录
#ifdef QQ_PLAT
int GeneralDataMgr::Login(const std::string& qid, const std::string& account, int isAdult, int union_id, const std::string& server_id, int qq_yellow, int is_year, const std::string& iopenid, const std::string& feedid, const std::string& str1, const std::string& str2, net::session_ptr csocket, Object& robj)
#else
int GeneralDataMgr::Login(const std::string& qid, const std::string& account, int isAdult, int union_id, const std::string& server_id, int qq_yellow, int is_year, const std::string& vcode, const std::string& sid, net::session_ptr csocket, Object& robj)
#endif
{
    //cout<<"login "<<qid<<","<<qq_yellow<<","<<is_year<<endl;
    {
        readLock lockit(&onlineuser_rwmutex);
        if (m_onlineuserlist.size() > 4321)
        {
            return 8888;
        }
    }
    //踢出其他的登录
    boost::shared_ptr<OnlineUser> paccount = GetAccount(account);
    if (paccount.get())
    {
        if (paccount->m_sockethandle.get())
        {
            //踢掉前面的登录
            paccount->m_sockethandle->state(STATE_CONNECTED);
            Object robj;
            std::string bye = "bye";
            robj.push_back( Pair("cmd", bye));
            robj.push_back( Pair("s", 401));
            paccount->m_sockethandle->send(write(robj));
            paccount->m_sockethandle->closeconnect(true);
            paccount->m_sockethandle.reset();
            INFO("account kick "<<account<<endl);
        }
        if (paccount->m_onlineCharactor.get())
        {
            GeneralDataMgr::getInstance()->CharactorLogout(paccount->m_onlineCharactor);
            paccount->m_onlineCharactor.reset();
        }
        paccount->m_sockethandle = csocket;
        paccount->m_ipaddress = csocket->remote_ip();
    }
    else
    {
        //创建在线
        paccount.reset(new OnlineUser(qid, account, union_id, server_id, csocket));
        writeLock lockit(&onlineuser_rwmutex);
        m_onlineuserlist[account] = paccount;
        INFO("account login "<<account<<endl);
    }
    paccount->m_isAdult = isAdult;
    paccount->m_qq_yellow_year = is_year;
    paccount->m_qq_yellow_level = qq_yellow;
#ifdef QQ_PLAT
    paccount->m_iopenid = iopenid;
    paccount->m_feedid = feedid;
    paccount->m_login_str1 = str1;
    paccount->m_login_str2 = str2;
#else
    paccount->m_vcode = vcode;
    paccount->m_sid = sid;
#endif

    robj.push_back( Pair("login", 2) );
    csocket->user(paccount);

#if 0//#ifdef ONE_CHARACTOR
    //单角色直接登录了，不用选角色
    if (paccount->m_charactorlist.size())
    {
        json_spirit::mObject obj;
        obj["cmd"] = "roleLogin";
        obj["id"] = paccount->m_charactorlist.begin()->m_cid;
        actionmessage act_msg(obj, 0);
        act_msg.setsession(csocket);
        if (0 != InsertActionWork(act_msg))
        {
            ERR();
        }
        return HC_SUCCESS_NO_RET;
    }
#endif
    return HC_SUCCESS;
}

boost::shared_ptr<ChatChannel> GeneralDataMgr::GetWorldChannel()
{
    if (m_world_channel.get())
    {
        return m_world_channel;
    }
    else
    {
        m_world_channel.reset(new ChatChannel("world", 1, "{\"cmd\":\"chat\",\"ctype\":2,\"s\":200,"));
        m_world_channel->start();
        return m_world_channel;
    }
}

//系统消息广播
int GeneralDataMgr::broadCastSysMsg(const std::string& msg, int type)
{
    std::string strSysMsg = strSystemMsgFormat;
    std::string sendmsg = json_spirit::add_esc_chars<std::string>(msg, true);
    str_replace(strSysMsg, "$M", sendmsg, true);
    str_replace(strSysMsg, "$T", LEX_CAST_STR(type), true);
    return GetWorldChannel()->BroadMsg(strSysMsg);
}

//系统消息广播- level等级以上
int GeneralDataMgr::broadCastSysMsg(const std::string& msg, int type, int level)
{
    std::string strSysMsg = strSystemMsgFormat;
    std::string sendmsg = json_spirit::add_esc_chars<std::string>(msg, true);
    str_replace(strSysMsg, "$M", sendmsg, true);
    str_replace(strSysMsg, "$T", LEX_CAST_STR(type), true);
    return GetWorldChannel()->BroadMsg(strSysMsg, level);
}

//系统消息广播-地图广播
int GeneralDataMgr::broadCastSysMapMsg(const std::string& msg, int type, int map)
{
    boost::shared_ptr<ChatChannel> ch = GeneralDataMgr::getInstance()->GetMapChannel(map);
    if (ch.get())
    {
        std::string strSysMsg = strSystemMsgFormat;
        std::string sendmsg = json_spirit::add_esc_chars<std::string>(msg, true);
        str_replace(strSysMsg, "$M", sendmsg, true);
        str_replace(strSysMsg, "$T", LEX_CAST_STR(type), true);
        ch->BroadMsg(strSysMsg);
    }
}

int GeneralDataMgr::broadCastToEveryone(const std::string& msg, int repeatnums, int interval)
{
    std::map<std::string, boost::shared_ptr<OnlineCharactor> >::iterator it = m_online_charactors.begin();
    while (it != m_online_charactors.end())
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "broadCast"));
            obj.push_back( Pair("s",200) );
            obj.push_back( Pair("content", msg) );
            obj.push_back( Pair("repeat", repeatnums) );
            obj.push_back( Pair("interval", interval) );
            it->second->Send(json_spirit::write(obj, json_spirit::raw_utf8));
        }
        ++it;
    }
    return 0;
}

boost::shared_ptr<ChatChannel> GeneralDataMgr::GetMapChannel(int mapid)
{
    if (mapid > 0 && mapid <= max_map_id)
    {
        return m_map_channels[mapid-1];
    }
    boost::shared_ptr<ChatChannel> ch;
    return ch;
}

boost::shared_ptr<ChatChannel> GeneralDataMgr::GetGuildChannel(uint64_t guild_id)
{
    readLock lockit(&guild_chanel_rwmutex);
    boost::unordered_map<uint64_t, boost::shared_ptr<ChatChannel> >::iterator it = m_guild_channels.find(guild_id);
    if (it != m_guild_channels.end())
    {
        return it->second;
    }
    else
    {
        lockit.unlock();
        writeLock wlockit(&guild_chanel_rwmutex);
        it = m_guild_channels.find(guild_id);
        if (it != m_guild_channels.end())
        {
            return it->second;
        }
        boost::shared_ptr<ChatChannel> ch;
        ch.reset(new ChatChannel("guild", guild_id, "{\"cmd\":\"chat\",\"ctype\":3,\"s\":200,"));
        ch->start();
        m_guild_channels[guild_id] = ch;
        return ch;
    }
}

boost::shared_ptr<ChatChannel> GeneralDataMgr::GetCampChannel(int camp)
{
    INFO("GeneralDataMgr::GetCampChannel("<<camp<<")");
    readLock lockit(&camp_chanel_rwmutex);
    boost::unordered_map<int, boost::shared_ptr<ChatChannel> >::iterator it = m_camp_channels.find(camp);
    if (it != m_camp_channels.end())
    {
        return it->second;
    }
    else
    {
        lockit.unlock();
        boost::shared_ptr<ChatChannel> ch;
        ch.reset(new ChatChannel("camp", camp, "{\"cmd\":\"chat\",\"ctype\":1,\"s\":200,\"camp\":" + LEX_CAST_STR(camp) + ","));
        ch->start();
        writeLock lockit(&camp_chanel_rwmutex);
        m_camp_channels[camp] = ch;
        INFO("GeneralDataMgr::GetCampChannel("<<camp<<") new a channel");
        return ch;
    }
}

int GeneralDataMgr::CharactorLogin(boost::shared_ptr<OnlineCharactor> oc)
{
    if (oc.get() && oc->m_account.get() && oc->m_charactor.get())
    {
        writeLock lockit(&onlinechar_rwmutex);

        //cout<<"charactor login "<<oc->m_account->m_qid<<","<<oc->m_account->m_qq_yellow_level<<","<<oc->m_account->m_qq_yellow_year<<"| cid "<<oc->m_charactor->m_id<<endl;
        oc->m_charactor->m_qq_yellow_level = oc->m_account->m_qq_yellow_level;
        oc->m_charactor->m_qq_yellow_year = oc->m_account->m_qq_yellow_year;
#ifdef QQ_PLAT
        oc->m_charactor->m_iopenid = oc->m_account->m_iopenid;
        oc->m_charactor->m_feedid = oc->m_account->m_feedid;
        oc->m_charactor->m_login_str1 = oc->m_account->m_login_str1;
        oc->m_charactor->m_login_str2 = oc->m_account->m_login_str2;
#else
        oc->m_charactor->m_vcode = oc->m_account->m_vcode;
        oc->m_charactor->m_sid = oc->m_account->m_sid;
#endif
        m_online_charactors[oc->m_charactor->m_name] = oc;
        lockit.unlock();
        //加入世界频道
        if (m_world_channel.get())
        {
            m_world_channel->Add(oc);
        }
        //加入阵营频道
        boost::shared_ptr<ChatChannel> ach = GetCampChannel(oc->m_charactor->m_camp);
        if (ach.get())
        {
            ach->Add(oc);
        }
        //加入军团频道
        if (oc->m_charactor->GetGuildId() > 0)
        {
            boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(oc->m_charactor->GetGuildId());
            if (gch.get())
            {
                gch->Add(oc);
            }
        }
        //加入地图频道
        //if (oc->m_charactor->m_area >= iOpenNearMap)
        {
            boost::shared_ptr<ChatChannel> ch = GeneralDataMgr::getInstance()->GetMapChannel(oc->m_charactor->m_area);
            if (ch.get())
            {
                ch->Add(oc);
            }
        }
        time_t time_now = time(NULL);
        //沉迷系统
        if (oc->m_account->m_isAdult == 1 || !GeneralDataMgr::getInstance()->isChenmiEnable())
        {
            oc->m_charactor->m_check_chenmi = false;
            oc->m_charactor->m_chenmi_time = 0;
        }
        else
        {
            oc->m_charactor->m_check_chenmi = true;
            //cout<<"org chen mi time="<<oc->m_charactor->m_chenmi_time;
            uint64_t rest_time = (uint64_t)(time_now > oc->m_charactor->m_login_time ? time_now - oc->m_charactor->m_login_time : 0);
            oc->m_charactor->m_chenmi_time = oc->m_charactor->m_chenmi_time > rest_time ? (oc->m_charactor->m_chenmi_time - rest_time) : 0;
            //cout<<",time_now="<<time_now<<",login time="<<oc->m_charactor->m_login_time<<",rest time="<<rest_time<<",chen mi time="<<oc->m_charactor->m_chenmi_time<<endl;
        }
        oc->m_charactor->m_notify_chenmi_time = 0;

        //更新连续登录天数
        time_t continue_login_gift_end = oc->m_charactor->queryExtraData(char_data_type_normal, char_data_get_continue_login_day);
        if (oc->m_charactor->m_currentStronghold >= iContinueLoginStronghold && time_now < continue_login_gift_end)
        {
            struct tm tm_l;
            struct tm *t_l = &tm_l;
            localtime_r(&(oc->m_charactor->m_login_time), t_l);
            struct tm tm_n;
            struct tm *t_n = &tm_n;
            localtime_r(&time_now, t_n);
            bool reset = false;//是否重新累计周期
            if (oc->m_charactor->m_continue_days == 0)
                oc->m_charactor->m_continue_days = 1;
            if (oc->m_charactor->m_total_continue_days == 0)
                oc->m_charactor->m_total_continue_days = 1;
            if (t_l->tm_mday != t_n->tm_mday && time_now - oc->m_charactor->m_login_time < 86400)
            {
                ++oc->m_charactor->m_continue_days;
                ++oc->m_charactor->m_total_continue_days;
                InsertSaveDb("update charactors set continue_days=" + LEX_CAST_STR(oc->m_charactor->m_continue_days)
                    + ",total_continue_days=" + LEX_CAST_STR(oc->m_charactor->m_total_continue_days)
                    + " where id=" + LEX_CAST_STR(oc->m_charactor->m_id));
            }
            else if(t_l->tm_mday != t_n->tm_mday && time_now - oc->m_charactor->m_login_time >= 86400)
            {
                reset = true;
                oc->m_charactor->m_continue_days = 1;
                oc->m_charactor->m_total_continue_days = 1;
                InsertSaveDb("update charactors set continue_days=" + LEX_CAST_STR(oc->m_charactor->m_continue_days)
                    + ",total_continue_days=" + LEX_CAST_STR(oc->m_charactor->m_total_continue_days)
                    + " where id=" + LEX_CAST_STR(oc->m_charactor->m_id));
            }
#if 0
            std::map<int,CharLoginPresent>::iterator it = oc->m_charactor->m_login_present.begin();
            while (it != oc->m_charactor->m_login_present.end())
            {
                if (it->first == 7 && it->second.state == 2 && oc->m_charactor->m_continue_days >= 8)
                {
                    oc->m_charactor->m_continue_days = 1;
                    InsertSaveDb("update charactors set continue_days=" + LEX_CAST_STR(oc->m_charactor->m_continue_days)
                        + ",total_continue_days=" + LEX_CAST_STR(oc->m_charactor->m_total_continue_days)
                        + " where id=" + LEX_CAST_STR(oc->m_charactor->m_id));
                    reset = true;
                }
                if (reset)
                {
                    it->second.state = 0;
                    InsertSaveDb("update char_continue_login_present set state=" + LEX_CAST_STR(it->second.state)
                            + " where pid=" + LEX_CAST_STR(it->first)
                            + " and cid=" +LEX_CAST_STR(oc->m_charactor->m_id));
                }
                if (it->first <= oc->m_charactor->m_continue_days && it->second.state == 0)
                {
                    it->second.state = 1;
                    InsertSaveDb("update char_continue_login_present set state=" + LEX_CAST_STR(it->second.state)
                            + " where pid=" + LEX_CAST_STR(it->first)
                            + " and cid=" +LEX_CAST_STR(oc->m_charactor->m_id));
                }
                else if (it->first > oc->m_charactor->m_continue_days && it->second.state == 1)
                {
                    it->second.state = 0;
                    InsertSaveDb("update char_continue_login_present set state=" + LEX_CAST_STR(it->second.state)
                            + " where pid=" + LEX_CAST_STR(it->first)
                            + " and cid=" +LEX_CAST_STR(oc->m_charactor->m_id));
                }
                ++it;
            }
#endif
        }
        //距离上次登录时间过去几天了
        {
            int days_passed = (time_now - oc->m_charactor->m_login_time)/(24*3600);
            //登录间隔超过一天有找回功能
            if (days_passed >= 1)
            {
                //最后一次登录在前一天0点之前
                time_t z = getZeroTime() - iONE_DAY_SECS;
                if (oc->m_charactor->m_login_time < z)
                {
                    oc->m_charactor->setExtraData(char_data_type_daily, char_data_daily_findback, 1);
                }
            }
        }
        //七日目标
        Singleton<seven_Goals_mgr>::Instance().updateGoals(*(oc->m_charactor.get()),oc->m_charactor->queryCreateDays());
        //更新登录时间
        oc->m_charactor->m_login_time = time_now;
        oc->m_charactor->m_chenmi_start_time = time_now;

        oc->m_charactor->m_is_online = 1;
        //创建号少于三天，而且领取天数少于三天才开始在线礼包
        time_t onlinegift_end = oc->m_charactor->queryExtraData(char_data_type_normal, char_data_get_onlinegift_day);
        if (oc->m_charactor->m_currentStronghold >= iOnlineGiftStronghold && time_now < onlinegift_end)
        {
            //在线礼包开始计时
            boost::shared_ptr<char_online_gift> cog = online_gift_mgr::getInstance()->getChar(oc->m_charactor->m_id);
            if (cog.get() && cog->_canGet == 0)
            {
                cog->reset();
            }
        }
        //更新在线统计
        dbCmd _dbcmd;
        _dbcmd._cid = oc->m_charactor->m_id;
        _dbcmd._cmd = db_cmd_char_online;
        InsertDbCharCmd(_dbcmd);

        oc->m_charactor->m_ip_address = oc->m_account->m_ipaddress;
        //查询是否被禁言
        //dbCmd _dbcmd;
        _dbcmd._cid= oc->m_charactor->m_id;
        _dbcmd._cmd = db_cmd_load_char_gag;
        InsertDbCharCmd(_dbcmd);

        //查询未读邮件
        mailCmd cmd;
        cmd.mobj["name"] = oc->m_charactor->m_name;
        cmd.cid = oc->m_charactor->m_id;
        cmd.cmd = mail_cmd_get_unread_list;
        InsertMailcmd(cmd);

        //加载祝贺，以弹出通知
        boost::shared_ptr<char_ralation> my_rl = Singleton<relationMgr>::Instance().getRelation(oc->m_charactor->m_id);
        my_rl->try_load_congratulations();
        my_rl->try_load_recved_congratulations();

        if (iOpenNearMap <= oc->m_charactor->m_area)
        {
            boost::shared_ptr<ChatChannel> ch = GeneralDataMgr::getInstance()->GetMapChannel(oc->m_charactor->m_area);
            if (ch.get())
            {
                std::string msg = strNotifyOnline;
                str_replace(msg, "$C", LEX_CAST_STR(oc->m_charactor->m_id));
                str_replace(msg, "$L", LEX_CAST_STR(oc->m_charactor->m_level));
                str_replace(msg, "$N", (oc->m_charactor->m_name));
                str_replace(msg, "$G", LEX_CAST_STR(oc->m_charactor->m_gender));
                ch->BroadMsg(msg);
            }
        }

        //新手福利是否需要过期通知
        if (!oc->m_charactor->isNewPlayer() && oc->m_charactor->queryExtraData(char_data_type_normal, char_data_new_player_end) == 0)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "notify") );
            obj.push_back( Pair("s", 200) );
            obj.push_back( Pair("type", notify_msg_new_player_end) );
            oc->m_charactor->sendObj(obj);
        }
        //腾讯统计
        #ifdef QQ_PLAT
        login_to_tencent(oc->m_charactor.get(),oc->m_charactor->m_login_str1,oc->m_charactor->m_login_str2,oc->m_charactor->m_iopenid,oc->m_charactor->m_feedid);
        int tecent_bag = oc->m_charactor->queryExtraData(char_data_type_daily, char_data_daily_tencent_bag);
        if (tecent_bag == 0)
        {
            oc->m_charactor->m_bag.login_to_tencent();
            oc->m_charactor->setExtraData(char_data_type_daily, char_data_daily_tencent_bag, 1);
        }
        #endif
    }
    return 0;
}

int GeneralDataMgr::CharactorLogout(boost::shared_ptr<OnlineCharactor> oc)
{
    if (oc.get() && oc->m_charactor.get() && oc->m_account.get())
    {
        INFO("GeneralDataMgr::CharactorLogout("<<oc->m_charactor->m_id<<")"<<endl);
        writeLock lockit(&onlinechar_rwmutex);
        m_online_charactors.erase(oc->m_charactor->m_name);
        lockit.unlock();
        //离开世界频道
        if (m_world_channel.get())
        {
            m_world_channel->Remove(oc);
        }
        //离开阵营频道
        boost::shared_ptr<ChatChannel> ach = GetCampChannel(oc->m_charactor->m_camp);
        if (ach.get())
        {
            ach->Remove(oc);
        }
        //离开军团频道
        if (oc->m_charactor->GetGuildId() > 0)
        {
            boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(oc->m_charactor->GetGuildId());
            if (gch.get())
            {
                gch->Remove(oc);
            }
        }
        //登出时离开副本
        if (oc->m_charactor->m_copy_id > 0)
        {
            boost::shared_ptr<spls_boss> boss = bossMgr::getInstance()->getBoss(oc->m_charactor->m_copy_id, oc->m_charactor->m_id);
            if (boss.get())
            {
                oc->m_charactor->m_copy_id_leave = oc->m_charactor->m_copy_id;
                boss->leave(oc->m_charactor, oc);
            }
        }
        //离开地图频道
        //if (oc->m_charactor->m_area >= iOpenNearMap)
        {
            boost::shared_ptr<ChatChannel> ch = GeneralDataMgr::getInstance()->GetMapChannel(oc->m_charactor->m_area);
            if (ch.get())
            {
                ch->Remove(oc);
            }
        }
        //登出时离开阵营战
        {
            campRaceMgr::getInstance()->leaveRace(oc->m_charactor.get(), oc);
        }
        //登出时离开多人副本
        {
            groupCopyMgr::getInstance()->leaveCopy(oc->m_charactor->m_id, oc->m_sockethandle);
        }
        //登出时离开军团战界面
        Singleton<corpsFightingMgr>::Instance().leaveJtz(oc->m_charactor->m_id);

        //下线更新防沉迷时间
        if (oc->m_charactor->m_check_chenmi)
        {
            oc->m_charactor->m_chenmi_time = oc->m_charactor->m_chenmi_time + time(NULL) - oc->m_charactor->m_chenmi_start_time;
        }
        time_t time_now = time(NULL);
        //更新连续登录天数
        time_t continue_login_gift_end = oc->m_charactor->queryExtraData(char_data_type_normal, char_data_get_continue_login_day);
        if (oc->m_charactor->m_currentStronghold >= iContinueLoginStronghold && time_now < continue_login_gift_end)
        {
            struct tm tm_l;
            struct tm *t_l = &tm_l;
            localtime_r(&(oc->m_charactor->m_login_time), t_l);
            struct tm tm_n;
            struct tm *t_n = &tm_n;
            localtime_r(&time_now, t_n);
            bool reset = false;//是否重新累计周期
            if (oc->m_charactor->m_continue_days == 0)
                oc->m_charactor->m_continue_days = 1;
            if (oc->m_charactor->m_total_continue_days == 0)
                oc->m_charactor->m_total_continue_days = 1;
            if (t_l->tm_mday != t_n->tm_mday && time_now - oc->m_charactor->m_login_time < 86400)
            {
                ++oc->m_charactor->m_continue_days;
                ++oc->m_charactor->m_total_continue_days;
                InsertSaveDb("update charactors set continue_days=" + LEX_CAST_STR(oc->m_charactor->m_continue_days)
                    + ",total_continue_days=" + LEX_CAST_STR(oc->m_charactor->m_total_continue_days)
                    + " where id=" + LEX_CAST_STR(oc->m_charactor->m_id));
            }
            else if(t_l->tm_mday != t_n->tm_mday && time_now - oc->m_charactor->m_login_time >= 86400)
            {
                reset = true;
                oc->m_charactor->m_continue_days = 1;
                oc->m_charactor->m_total_continue_days = 1;
                InsertSaveDb("update charactors set continue_days=" + LEX_CAST_STR(oc->m_charactor->m_continue_days)
                    + ",total_continue_days=" + LEX_CAST_STR(oc->m_charactor->m_total_continue_days)
                    + " where id=" + LEX_CAST_STR(oc->m_charactor->m_id));
            }
#if 0
            std::map<int,CharLoginPresent>::iterator it = oc->m_charactor->m_login_present.begin();
            while (it != oc->m_charactor->m_login_present.end())
            {
                if (it->first == 7 && it->second.state == 2 && oc->m_charactor->m_continue_days >= 8)
                {
                    oc->m_charactor->m_continue_days = 1;
                    InsertSaveDb("update charactors set continue_days=" + LEX_CAST_STR(oc->m_charactor->m_continue_days)
                        + ",total_continue_days=" + LEX_CAST_STR(oc->m_charactor->m_total_continue_days)
                        + " where id=" + LEX_CAST_STR(oc->m_charactor->m_id));
                    reset = true;
                }
                if (reset)
                {
                    it->second.state = 0;
                    InsertSaveDb("update char_continue_login_present set state=" + LEX_CAST_STR(it->second.state)
                            + " where pid=" + LEX_CAST_STR(it->first)
                            + " and cid=" +LEX_CAST_STR(oc->m_charactor->m_id));
                }
                if (it->first <= oc->m_charactor->m_continue_days && it->second.state == 0)
                {
                    it->second.state = 1;
                    InsertSaveDb("update char_continue_login_present set state=" + LEX_CAST_STR(it->second.state)
                            + " where pid=" + LEX_CAST_STR(it->first)
                            + " and cid=" +LEX_CAST_STR(oc->m_charactor->m_id));
                }
                else if (it->first > oc->m_charactor->m_continue_days && it->second.state == 1)
                {
                    it->second.state = 0;
                    InsertSaveDb("update char_continue_login_present set state=" + LEX_CAST_STR(it->second.state)
                            + " where pid=" + LEX_CAST_STR(it->first)
                            + " and cid=" +LEX_CAST_STR(oc->m_charactor->m_id));
                }
                ++it;
            }
#endif
        }
        oc->m_charactor->m_login_time = time_now;
        oc->m_charactor->Save();
        oc->m_charactor->m_is_online = 0;

        //更新在线统计
        dbCmd _dbcmd;
        _dbcmd._cid = oc->m_charactor->m_id;
        _dbcmd._cmd = db_cmd_char_offline;
        InsertDbCharCmd(_dbcmd);

        //更新列表
        std::list<CharactorInfo>::iterator it = oc->m_account->m_charactorlist.begin();
        while (it != oc->m_account->m_charactorlist.end())
        {
            if (it->m_cid == oc->m_cid)
            {
                it->m_lastlogin = oc->m_charactor->m_login_time;
                it->m_level = oc->m_charactor->m_level;

                CharactorInfo c = *it;
                oc->m_account->m_charactorlist.erase(it);
                oc->m_account->m_charactorlist.insert(oc->m_account->m_charactorlist.begin(), c);
                break;
            }
            ++it;
        }
        if (iOpenNearMap <= oc->m_charactor->m_area)
        {
            boost::shared_ptr<ChatChannel> ch = GeneralDataMgr::getInstance()->GetMapChannel(oc->m_charactor->m_area);
            if (ch.get())
            {
                std::string msg = strNotifyOffline;
                str_replace(msg, "$C", LEX_CAST_STR(oc->m_charactor->m_id));
                ch->BroadMsg(msg);
            }
        }
        //腾讯记录
#ifdef QQ_PLAT
        oc->m_charactor->m_bag.logout_to_tencent();
        _analyzer.DestoryInstance(LEX_CAST_STR(oc->m_charactor->m_id).c_str());
#endif
    }
    return 0;
}

/*获得角色登录列表
int GeneralDataMgr::GetCharactorInfoList(const std::string& account, std::list<boost::shared_ptr<CharactorInfo> >& charactorlist)
{
    INFO("get charactor list:"<<account);
    charactorlist.clear();
    Query q(GetDb());
    q.get_result("SELECT `id`,lastlogin,name,spic,vip,level FROM `charactors` WHERE account='" + account + "' order by lastlogin desc");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<CharactorInfo> gd;
        gd.reset(new CharactorInfo);
        gd->m_cid = q.getval();
        gd->m_lastlogin = q.getval();
        gd->m_name = q.getstr();
        gd->m_spic = q.getval();
        gd->m_vip = q.getval();
        gd->m_level = q.getval();
        charactorlist.push_back(gd);
    }
    q.free_result();
    return 0;
}*/

//回复玩家军令
int GeneralDataMgr::addLing(int counts)
{
    //军令获得数值4倍
    if(counts > 0)
        counts *= 4;
#ifdef GLOBAL_CHAR_LOCK
    readLock lockit(&globalchar_rwmutex);
#endif
    std::string lingmsg = strRecoverLing;
    str_replace(lingmsg, "$L", LEX_CAST_STR(counts));

    std::string msg = strCharMsgFormat;
    str_replace(msg, "$M", lingmsg, true);

    std::string failmsg = strCharMsgFormat;
    str_replace(failmsg, "$M", strRecoverLingFull, true);

    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get())
        {
            if (it->second->ling() < iSystemRecoverMaxLing)
            {
                if ((it->second->ling() + counts) > iSystemRecoverMaxLing)
                {
                    it->second->ling(iSystemRecoverMaxLing);
                }
                else
                {
                    it->second->addLing(counts);
                }
                it->second->NotifyCharData();
                //sendNotifyMsg(it->second->m_name,msg);
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(it->second->m_name);
                if (account.get())
                {
                    account->Send(msg);
                }
            }
            else
            {
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(it->second->m_name);
                if (account.get())
                {
                    account->Send(failmsg);
                }
            }
        }
        ++it;
    }
    InsertSaveDb("update char_resource set ling=if(ling+" + LEX_CAST_STR(counts) + ">"
                + LEX_CAST_STR(iSystemRecoverMaxLing)
                + "," + LEX_CAST_STR(iSystemRecoverMaxLing)
                + ",ling+" + LEX_CAST_STR(counts) + ") where ling<" + LEX_CAST_STR(iSystemRecoverMaxLing));
    return HC_SUCCESS;
}

//重置福利
int GeneralDataMgr::resetWelfare()
{
#ifdef GLOBAL_CHAR_LOCK
    readLock lockit(&globalchar_rwmutex);
#endif
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get())
        {
            it->second->m_welfare = 0;
            //boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(it->second->m_name);
            //if (account)
            //{
            //    account->Send(strRestNotify);
            //}
        }
        ++it;
    }
    GetWorldChannel()->BroadMsg(strRestNotify);
    InsertSaveDb("update char_data_temp set welfare=0 where 1");
    return HC_SUCCESS;
}

//重置福利 注册前三天的用户
int GeneralDataMgr::resetWelfare2()
{
#ifdef VN_SERVER

#ifdef GLOBAL_CHAR_LOCK
    readLock lockit(&globalchar_rwmutex);
#endif

    time_t timex = time(NULL);
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&timex, t);
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    time_t tstart = mktime(t) - 2*iONE_DAY_SECS;

    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get() && it->second->m_createTime >= tstart)
        {
            it->second->m_welfare = 0;
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(it->second->m_name);
            if (account)
            {
                account->Send(strRestNotify);
            }
        }
        ++it;
    }
    InsertSaveDb("update char_data_temp set welfare=0 where cid in (select id from charactors where createTime>="
                + LEX_CAST_STR(tstart) + ")");
#else

#endif
    return HC_SUCCESS;
}

int GeneralDataMgr::resetAll()
{
#ifdef GLOBAL_CHAR_LOCK
    readLock lockit(&globalchar_rwmutex);
#endif
    bool farm_reset = (GeneralDataMgr::getInstance()->getSeason() == 2 || GeneralDataMgr::getInstance()->getSeason() == 4);
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get())
        {
            //金币休息重置
            it->second->m_gold_rest = 0;
            //金币探索刷新次数
            it->second->m_explore_refresh_times = 0;
            it->second->resetExploreLing();
            //领取俸禄
            if (it->second->m_officeOpen && it->second->m_offical >= 2)
            {
                it->second->m_hasgetsalary = 0;
            }
            it->second->m_temp_jisi_times = 0;
            it->second->m_temp_corps_yanhui = 0;
            //购买洗髓次数
            it->second->m_buy_xisui_time = 0;
            //征收次数重置
            it->second->m_levy_time = 0;
            it->second->m_gold_train_horse = 0;
            it->second->m_silver_train_horse = 0;
            if(farm_reset)
                it->second->m_farm_harvest = 0;

            //清空沉迷时间
            it->second->m_chenmi_time = 0;
            it->second->m_chenmi_start_time = time(NULL);

            it->second->clearExtraData(char_data_type_daily);

            it->second->m_daily_wash_times = 0;
            memset(it->second->m_wash_event, 0, sizeof(int)*10);
            it->second->m_wash_event_state = 0;
            it->second->resetTrainQue();

        }
        ++it;
    }

    InsertSaveDb("TRUNCATE TABLE char_daily_temp");

    //刷新探索令
    InsertSaveDb("update char_resource set explore_ling=" + LEX_CAST_STR(iFreeExploreTimes) + " where 1");
    //删除过期的战报
    InsertSaveDb("delete from battle_records where archive=0 and input<unix_timestamp()");
    //删除三天前聊天记录
    InsertSaveDb("delete from admin_count_talk where (unix_timestamp() - unix_timestamp(input)) > 259200");
    //清空沉迷时间
    InsertSaveDb("update charactors set chenmi_time=0 where 1");
    //每日數據清除
    InsertSaveDb("delete from `char_data_extra` where type=1");
    //邮件保留7天
    InsertSaveDb("delete from char_mails where cid>0 and archive=0 and unix_timestamp(input) < (unix_timestamp()-604800)");
    //过期道具删除
    InsertSaveDb("delete from `char_treasures` where state=1 and deleteTime<unix_timestamp()");
    InsertSaveDb("delete FROM  `char_treasures` WHERE invalidTime>0 and invalidTime < UNIX_TIMESTAMP() ");
    //过期登录数据
    InsertSaveDb("delete from api_login where unix_timestamp(input)+3600<unix_timestamp()");
    //过期祝贺
    InsertSaveDb("delete from char_congratulations where (unix_timestamp(input)+2592000)<unix_timestamp()");
    return HC_SUCCESS;
}

//发放GM奖励
int GeneralDataMgr::GM_reward()
{
#ifdef GLOBAL_CHAR_LOCK
    readLock lockit(&globalchar_rwmutex);
#endif
    std::list<boost::shared_ptr<CharGMPresent> > m_gm_list;
    m_gm_list.clear();
    Query q(GetDb());
    q.get_result("SELECT cid,gm_level,has_first_give,has_give FROM char_gm_list WHERE 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<CharGMPresent> pgm;
        pgm.reset(new CharGMPresent);
        pgm->gm_id = q.getval();
        pgm->gm_level = q.getval();
        pgm->gm_first = q.getval();
        pgm->gm_get_reward = q.getval();
        m_gm_list.push_back(pgm);
    }
    q.free_result();
    std::list<boost::shared_ptr<CharGMPresent> >::iterator it = m_gm_list.begin();
    while (it != m_gm_list.end())
    {
        if ((*it).get())
        {
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData((*it)->gm_id);
            if (!cdata.get())
            {
                ++it;
                continue;
            }
            //取消GM资格
            if (time(NULL) - cdata->m_login_time >= iGM_Cancel)
            {
                InsertSaveDb("delete from char_gm_list where cid=" + LEX_CAST_STR((*it)->gm_id));
                ++it;
                continue;
            }
            //首次发放
            if ((*it)->gm_first == 0)
            {
                (*it)->gm_first = 1;
                if ((*it)->gm_level > iGM_Level_Max)
                {
                    (*it)->gm_level = iGM_Level_Max;
                }
                int recharge_gold = iGM_Level_Gold[(*it)->gm_level - 1];
                GM_recharge(cdata,recharge_gold);
                InsertSaveDb("update char_gm_list set has_first_give=1 where cid=" + LEX_CAST_STR((*it)->gm_id));
            }
            //发放
            else if ((*it)->gm_get_reward == 0 && cdata->m_total_continue_days % 7 == 0 && (*it)->gm_level > 0)
            {
                (*it)->gm_get_reward = 1;
                if ((*it)->gm_level > iGM_Level_Max)
                {
                    (*it)->gm_level = iGM_Level_Max;
                }
                int recharge_gold = iGM_Level_Gold[(*it)->gm_level - 1];
                GM_recharge(cdata,recharge_gold);
                InsertSaveDb("update char_gm_list set has_give=1 where cid=" + LEX_CAST_STR((*it)->gm_id));
            }
            //重置
            else if ((*it)->gm_get_reward == 1 && cdata->m_total_continue_days % 7 != 0)
            {
                (*it)->gm_get_reward = 0;
                InsertSaveDb("update char_gm_list set has_give=0 where cid=" + LEX_CAST_STR((*it)->gm_id));
            }
        }
        ++it;
    }
    return HC_SUCCESS;
}

void GeneralDataMgr::GM_recharge(boost::shared_ptr<CharData>& cdata, int recharge_gold)
{
    //直接加
    InsertSaveDb("insert into char_recharge set type='gm_reward',cid=" + LEX_CAST_STR(cdata->m_id)
            + ",account='',gold='" + LEX_CAST_STR(recharge_gold) + "',input=now()");
    cdata->addGold(recharge_gold);
    //金币获得统计
    add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address,recharge_gold,gold_get_plat_recharge, cdata->m_union_id, cdata->m_server_id);
    cdata->m_total_recharge += recharge_gold;
    InsertSaveDb("replace into char_total_recharge (cid,total_recharge) values (" + LEX_CAST_STR(cdata->m_id) + "," + LEX_CAST_STR(cdata->m_total_recharge) + ")");
    cdata->updateVip();

    //周排行活动
    int score = recharge_gold;
    if (score > 0)
        newRankings::getInstance()->updateEventRankings(cdata->m_id,rankings_event_recharge,score);
    return;
}

void GeneralDataMgr::checkTmpVIP()
{
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get() && it->second->m_tmp_vip_end_time != 0)
        {
            if (it->second->m_tmp_vip_end_time <= time(NULL))
            {
                it->second->m_tmp_vip = 0;
                it->second->m_tmp_vip_start_time = 0;
                it->second->m_tmp_vip_end_time = 0;
                Query q(GetDb());
                q.get_result("SELECT vip FROM `char_data` WHERE cid=" + LEX_CAST_STR(it->second->m_id));
                CHECK_DB_ERR(q);
                if (q.fetch_row())
                {
                    it->second->m_vip = q.getval();
                }
                else
                {
                    it->second->m_vip = 0;
                }
                q.free_result();
                //通知客户端试用VIP到期
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(it->second->m_name);
                if (account)
                {
                    account->Send("{\"cmd\":\"endTmpVIP\",\"s\":200}");
                }
                //it->second->NotifyCharData();
            }
        }
        ++it;
    }
    InsertSaveDb("delete from `char_tmp_vip` where endtime<=unix_timestamp()");
}

bool compare_lottery_score(boost::shared_ptr<CharData> a, boost::shared_ptr<CharData> b)
{
    if (!a.get())
    {
        return false;
    }
    if (!b.get())
    {
        return true;
    }
    return a->queryExtraData(char_data_type_week, char_data_extra_lottery_score) > b->queryExtraData(char_data_type_week, char_data_extra_lottery_score);
}

int GeneralDataMgr::weekReset()
{
    cout<<"****** reset week data ******"<<endl;

    //保存梅花易数积分榜
    std::list<boost::shared_ptr<CharData> > lotteryRankings;
    for (std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin(); it != m_chardata_map.end(); ++it)
    {
        if (it->second.get())
        {
            //保存梅花易数积分榜
            if (it->second->queryExtraData(char_data_type_week, char_data_extra_lottery_score) > 0)
            {
                lotteryRankings.push_back(it->second);
            }
        }
    }
    //保存梅花易数积分榜
    std::string lotteryScore_context = "";
    lotteryRankings.sort(compare_lottery_score);
    int rank = 1;
    for (std::list<boost::shared_ptr<CharData> >::iterator it = lotteryRankings.begin(); it != lotteryRankings.end(); ++it)
    {
        lotteryScore_context += LEX_CAST_STR(rank) + "\t" + (*it)->m_name + "(" + LEX_CAST_STR((*it)->m_id) + ")" + "\t\t" + LEX_CAST_STR((*it)->queryExtraData(char_data_type_week, char_data_extra_lottery_score)) + "\n";
        ++rank;
    }
    InsertSaveDb("insert into char_mails (input,unread,archive,type,`from`,title,content,cid) values (now(),'1','0','1',0,'lottery score','"
                    + GetDb().safestr(lotteryScore_context)    + "','0')");

    //至尊皇座更新
    Singleton<throneMgr>::Instance().update();

    for (std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin(); it != m_chardata_map.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->clearExtraData(char_data_type_week);
        }
    }

    //每週數據清除
    InsertSaveDb("delete from `char_data_extra` where type=2");
    return 0;
}

int GeneralDataMgr::reloadMap()
{
    cout<<"************ GeneralDataMgr::reloadMap() ******************"<<endl;
    Query q(GetDb());
    q.get_result("select id,name,openLevel,memo,intro,get from base_maps where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        if (!m_base_maps[mapid-1].get())
        {
            m_base_maps[mapid-1].reset(new baseMap);
        }
        m_base_maps[mapid-1]->name = q.getstr();
        m_base_maps[mapid-1]->openLevel = q.getval();
        m_base_maps[mapid-1]->memo = q.getstr();
        m_base_maps[mapid-1]->intro = q.getstr();
        m_base_maps[mapid-1]->get = q.getstr();
    }
    q.free_result();
    return HC_SUCCESS;
}

int GeneralDataMgr::reloadStage()
{
    cout<<"************ GeneralDataMgr::reloadStage() ******************"<<endl;

    Query q(GetDb());
    q.get_result("select mapid,id,name,openLevel from base_stages where 1 order by mapid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        if (!m_base_maps[mapid-1].get())
        {
            ERR();
            continue;
        }
        int stage = q.getval();
        if (stage < 1 || stage > 3)
        {
            ERR();
            continue;
        }
        if (!m_base_maps[mapid-1]->stages[stage-1].get())
        {
            m_base_maps[mapid-1]->stages[stage-1].reset(new baseStage);
        }
        m_base_maps[mapid-1]->stages[stage-1]->mapid = mapid;
        m_base_maps[mapid-1]->stages[stage-1]->id = stage;
        m_base_maps[mapid-1]->stages[stage-1]->name = q.getstr();
        m_base_maps[mapid-1]->stages[stage-1]->openLevel = q.getval();

        m_base_maps[mapid-1]->stages[stage-1]->stage_finish_guide = 0;

        //m_base_maps[mapid-1]->stages[stage-1]->first = q.getval();
        //m_base_maps[mapid-1]->stages[stage-1]->last = q.getval();
        m_base_maps[mapid-1]->stages[stage-1]->size = 0;//m_base_maps[mapid-1]->stages[stage-1]->last + 1 - m_base_maps[mapid-1]->stages[stage-1]->first;

        m_base_maps[mapid-1]->stages[stage-1]->_baseMap.reset();
        m_base_maps[mapid-1]->stages[stage-1]->_baseMap = m_base_maps[mapid-1];
    }
    q.free_result();
    return HC_SUCCESS;
}

boost::shared_ptr<baseMap> GeneralDataMgr::GetBaseMap(int mapid)
{
    if (mapid >= 1 && mapid <= 10)
    {
        return m_base_maps[mapid-1];
    }
    else
    {
        boost::shared_ptr<baseMap> bm;
        return bm;
    }
}

boost::shared_ptr<baseoffical> GeneralDataMgr::GetBaseOffical(int id)
{
    INFO("getbaseoffical is call" << endl);
    std::map<int, boost::shared_ptr<baseoffical> >::iterator it = m_base_officals.find(id);
    if (it != m_base_officals.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<baseoffical> p;
    p.reset();
    return p;
}

std::list<boost::shared_ptr<officalgenerals> >& GeneralDataMgr::GetBaseOfficalGenerals()
{
    return m_base_offical_generals;
}

std::list<Item>& GeneralDataMgr::GetFirstRechargeGift()
{
    return m_first_recharge_gift;
}

//获得当前年份、季节
boost::shared_ptr<DateInfo> GeneralDataMgr::GetDate()
{
    if (m_spls_date.get())
    {
        return m_spls_date;
    }
    else
    {
        m_spls_date.reset(new DateInfo);
        m_spls_date->year = m_year;
        m_spls_date->season = m_season;
#if 0
        switch (m_season)
        {
            case 1:
                m_spls_date->effect = strSpringEffect;
                break;
            case 2:
                m_spls_date->effect = strSummerEffect;
                break;
            case 3:
                m_spls_date->effect = strAutumnEffect;
                break;
            default:
                m_spls_date->effect = strWinterEffect;
                break;
        }
#endif
        return m_spls_date;
    }
}

//获得当前年份、季节
int GeneralDataMgr::getYear()
{
    boost::shared_ptr<DateInfo> dt = GetDate();
    if (dt.get())
    {
        return dt->year;
    }
    return 1000;
}

//获得当前年份、季节
int GeneralDataMgr::getSeason()
{
    boost::shared_ptr<DateInfo> dt = GetDate();
    if (dt.get())
    {
        return dt->season;
    }
    return 1;
}

//获得当前年份、季节
std::string GeneralDataMgr::getSeasonString()
{
    boost::shared_ptr<DateInfo> dt = GetDate();
    if (dt.get())
    {
        if (dt->season >= 1 && dt->season <= 4)
        {
            return strSeason[dt->season-1];
        }
    }
    return strSeason[0];
}

//获得竞技场奖励时间
int GeneralDataMgr::GetRaceRewardTime()
{
    return m_race_reward_time;
}

baseLoginPresent* GeneralDataMgr::getBaseLoginPresent(int pid)
{
    baseLoginPresent* p = NULL;
    std::map<int, boost::shared_ptr<baseLoginPresent> >::iterator it = m_base_login_present.find(pid);
    if (it != m_base_login_present.end())
    {
        p = it->second.get();
    }
    return p;
}

boost::shared_ptr<baseVIPPresent> GeneralDataMgr::getBaseVIPPresent(int id)
{
    boost::shared_ptr<baseVIPPresent> p;
    std::map<int, boost::shared_ptr<baseVIPPresent> >::iterator it = m_base_vip_present.find(id);
    if (it != m_base_vip_present.end())
    {
        p = it->second;
    }
    else
    {
        p.reset(new baseVIPPresent);
        p->vip = id;
        m_base_vip_present[p->vip] = p;
    }
    return p;
}

baseRechargePresent* GeneralDataMgr::getBaseRechargePresent(int id)
{
    baseRechargePresent* p = NULL;
    std::map<int, boost::shared_ptr<baseRechargePresent> >::iterator it = m_base_recharge_present.find(id);
    if (it != m_base_recharge_present.end())
    {
        p = it->second.get();
    }
    return p;
}

int GeneralDataMgr::checkRechargePresent(int num, int type)
{
    int id = 0;
    std::map<int, boost::shared_ptr<baseRechargePresent> >::iterator it = m_base_recharge_present.begin();
    while (it != m_base_recharge_present.end())
    {
        if (type == 2 && it->first <= 7)
        {
            ++it;
            continue;
        }
        if (num >= it->second->needgold)
            id = it->first;
        else
            break;
        ++it;
    }
    return id;
}

//删除角色时，移除角色数据
int GeneralDataMgr::removeCharData(int cid)
{
#ifdef GLOBAL_CHAR_LOCK
    writeLock lockit(&globalchar_rwmutex);
#endif
    m_chardata_map.erase(cid);
    return HC_SUCCESS;
}

//在线人数 - record = ture, 同时插入到后台表中进行统计
int GeneralDataMgr::getTotalOnline(bool record = false)
{
    readLock lockit(&onlineuser_rwmutex);
    if (record)
    {
        InsertSaveDb("INSERT INTO admin_online (`id`, `jtime`, `num`) VALUES (NULL, now(), '" + LEX_CAST_STR(m_onlineuserlist.size()) + "')");
    }
    return     m_onlineuserlist.size();
}

//心跳
int GeneralDataMgr::HeartBeat()
{
    //if (m_enable_chenmi)
    {
        int counts = 0;
        uint64_t start_time = 0;
        if (g_print_debug_info > 1)
        {
            start_time = splsTimeStamp();
        }
        readLock lockit(&onlineuser_rwmutex);
        for (std::map<std::string, boost::shared_ptr<OnlineCharactor> >::iterator it = m_online_charactors.begin(); it != m_online_charactors.end(); ++it)
        {
            if (it->second.get())
            {
                it->second.get()->onHeartBeat();
                ++counts;
            }
        }
        if (g_print_debug_info > 1)
        {
            cout<<"****** heart beart : "<<counts<<" charactors cost "<<(double(splsTimeStamp()-start_time)/1000)<<" ms ******"<<endl;
        }
        return counts;
    }
    return 0;
}

//给角色发送消息
int GeneralDataMgr::sendSysMsg(const std::string& char_name, const std::string& msg, int type)
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(char_name);
    if (account.get())
    {
        std::string strSysMsg = strSystemMsgFormat;
        str_replace(strSysMsg, "$M", msg, true);
        str_replace(strSysMsg, "$T", LEX_CAST_STR(type), true);
        account->Send(strSysMsg);
        return HC_SUCCESS;
    }
    return HC_ERROR_CHAR_NOT_ONLINE;
}

//给角色发送消息
int GeneralDataMgr::sendNotifyMsg(const std::string& char_name, const std::string& msg)
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(char_name);
    if (account.get())
    {
        std::string strSysMsg = strCharMsgFormat;
        str_replace(strSysMsg, "$M", msg, true);
        account->Send(strSysMsg);
        return HC_SUCCESS;
    }
    return HC_ERROR_CHAR_NOT_ONLINE;
}

//设置角色关卡进度
int GeneralDataMgr::updateTempo(int cid, int level)
{
    boost::shared_ptr<CharData> cdata = GetCharData(cid);
    CharData* pc = cdata.get();
    if (pc)
    {
        if (level > iMaxCharLevel)
        {
            level = iMaxCharLevel;
        }
        CharTempoData& tempo = pc->GetTempo();

        for (int mapid = 1; mapid <= 10; ++mapid)
        {
            bool need_break = false;
            if (m_base_maps[mapid-1].get())
            {
                for (int stageid = 1; stageid <= 3; ++stageid)
                {
                    if (m_base_maps[mapid-1]->stages[stageid-1].get())
                    {
                        for (int stronghold = 1; stronghold <= 25; ++stronghold)
                        {
                            if (m_base_maps[mapid-1]->stages[stageid-1]->_baseStrongholds[stronghold-1].get())
                            {
                                StrongholdData* pSh = m_base_maps[mapid-1]->stages[stageid-1]->_baseStrongholds[stronghold-1].get();
                                if (pSh)
                                {
                                    if (pSh->m_level > level)
                                    {
                                        need_break = true;
                                    }
                                    else
                                    {
                                        std::list<Item> getItems;
                                        lootMgr::getInstance()->getStrongholdLoots(pSh->m_id, getItems, 10000);
                                        if (getItems.size() == 0)
                                        {
                                            lootMgr::getInstance()->getWorldItemFall(getItems);
                                        }
                                        //给东西
                                        if (0 == giveLoots(pc, getItems, pSh->m_map_id, pSh->m_level, pSh->m_isepic, NULL, NULL, true, 0))
                                        {
                                            //什么东西都没得到 (技能到顶/阵型到顶)
                                            getItems.clear();
                                            lootMgr::getInstance()->getWorldItemFall(getItems);
                                            giveLoots(pc, getItems, pSh->m_map_id, pSh->m_level, pSh->m_isepic, NULL, NULL, true, 0);
                                        }
                                        pc->level(pSh->m_level);
                                        while (pc->m_area < pSh->m_map_id)
                                        {
                                            boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(cdata->m_area + 1);
                                            if (!bm.get() || bm->openLevel == 0)
                                            {
                                                continue;
                                            }
                                            //pc->level(bm->openLevel);
                                            ++pc->m_area;
                                            pc->m_cur_stage = 1;
                                            pc->notifyChangeMap();
                                            pc->m_tempo.InitCharTempo(pc->m_area);
                                            pc->Save();
                                            //更新任务
                                            pc->updateTask(task_enter_map, pc->m_area);
                                            //新手引导
                                            pc->checkGuide(guide_type_enter_map, pc->m_area, 0);
                                        }
                                        tempo.update(pSh->m_id, false);
                                    }
                                }
                            }
                        }
                    }
                    if (need_break)
                    {
                        break;
                    }
                }
            }
            if (need_break)
            {
                break;
            }
        }
    }
    return HC_SUCCESS;
}

//获得地图附近玩家
int GeneralDataMgr::getNearPlayerList(CharData* cdata, json_spirit::Object& robj)
{
    json_spirit::Array list;
    std::list<boost::shared_ptr<CharData> > char_list;
    char_list.clear();

    int online_count = 0;
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        //玩家自己或者不同地图的跳过
        if (!it->second.get()
            || it->second->m_id == cdata->m_id
            || cdata->m_area != it->second->m_area)
        {
            ++it;
            continue;
        }
        std::list<boost::shared_ptr<CharData> >::iterator it_c = char_list.begin();
        while (it_c != char_list.end())
        {
            if (!(*it_c).get())
            {
                ++it_c;
                continue;
            }
            if ((*it_c)->m_is_online < it->second->m_is_online)
            {
                break;
            }
            else if((*it_c)->m_is_online == it->second->m_is_online && (*it_c)->m_level < it->second->m_level)
            {
                break;
            }
            ++it_c;
        }
        char_list.insert(it_c, it->second);
        while (char_list.size() >  20)
        {
            char_list.pop_back();
        }
        if (it->second->m_is_online)
        {
            ++online_count;
            if (online_count >= 20)
            {
                break;
            }
        }
        ++it;
    }
    int cnt = 0;
    if (char_list.size())
    {
        std::list<boost::shared_ptr<CharData> >::iterator itt = char_list.begin();
        while (itt != char_list.end())
        {
            if (cnt >= 20)
            {
                break;
            }
            if (!(*itt).get())
            {
                ++itt;
                continue;
            }
            //玩家自己或者不同地图的跳过
            //if ((*itt)->m_id == cdata->m_id || cdata->m_area != (*itt)->m_area)
            //{
            //    ++itt;
            //    continue;
            //}
            json_spirit::Object obj;
            obj.push_back( Pair("id", (*itt)->m_id) );
            obj.push_back( Pair("name", (*itt)->m_name) );
            obj.push_back( Pair("gender", (*itt)->m_gender) );
            obj.push_back( Pair("level", (*itt)->m_level) );
            obj.push_back( Pair("online", (*itt)->m_is_online) );
            obj.push_back( Pair("b_friend", Singleton<relationMgr>::Instance().is_my_attention(cdata->m_id, (*itt)->m_id)) );
            list.push_back(obj);
            ++cnt;
            ++itt;
        }
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//好友推送
bool GeneralDataMgr::canRecommendFriends(CharData* cdata)
{
    readLock lockit(&onlineuser_rwmutex);

    for (int area = cdata->m_area; area < max_map_id; ++area)
    {
        //优先同一地图的，等级大于自己的玩家
        for (std::map<std::string, boost::shared_ptr<OnlineCharactor> >::iterator it = m_online_charactors.begin(); it != m_online_charactors.end(); ++it)
        {
            if (it->second.get() && it->second.get()->m_charactor.get())
            {
                CharData* c = it->second.get()->m_charactor.get();
                if (cdata == c || area != c->m_area || cdata->m_level > c->m_level || Singleton<relationMgr>::Instance().is_my_attention(cdata->m_id, c->m_id))
                {
                    continue;
                }
                return true;
            }
        }
    }
    return false;
}

//好友推送
int GeneralDataMgr::getRecommendFriends(CharData* cdata, json_spirit::Object& robj)
{
    json_spirit::Array list;

    std::map<int, boost::shared_ptr<CharData> >& char_list = cdata->m_recommend_friends;

    //是否需要刷新
    if (time(NULL) > (cdata->m_recommend_friend_refresh + 300))
    {
        char_list.clear();
        readLock lockit(&onlineuser_rwmutex);

        for (int area = cdata->m_area; area < max_map_id; ++area)
        {
            //优先同一地图的，等级大于自己的玩家
            for (std::map<std::string, boost::shared_ptr<OnlineCharactor> >::iterator it = m_online_charactors.begin(); it != m_online_charactors.end(); ++it)
            {
                if (it->second.get() && it->second.get()->m_charactor.get())
                {
                    CharData* c = it->second.get()->m_charactor.get();
                    if (cdata == c || area != c->m_area || cdata->m_level > c->m_level
                        || Singleton<relationMgr>::Instance().is_my_attention(cdata->m_id, c->m_id))
                    {
                        continue;
                    }
                    char_list[c->m_id] = it->second.get()->m_charactor;
                }
                if (char_list.size() >= 10)
                {
                    break;
                }
            }
            if (char_list.size() >= 10)
            {
                break;
            }
        }
        cdata->m_recommend_friend_refresh = time(NULL);
    }
    if (char_list.size() < 10)
    {
        readLock lockit(&onlineuser_rwmutex);
        //等级大于自己的玩家
        for (std::map<std::string, boost::shared_ptr<OnlineCharactor> >::iterator it = m_online_charactors.begin(); it != m_online_charactors.end(); ++it)
        {
            if (it->second.get() && it->second.get()->m_charactor.get())
            {
                CharData* c = it->second.get()->m_charactor.get();
                if (cdata == c || cdata->m_area != c->m_area || cdata->m_level > c->m_level
                    || Singleton<relationMgr>::Instance().is_my_attention(cdata->m_id, c->m_id))
                {
                    continue;
                }
                char_list[c->m_id] = it->second.get()->m_charactor;
            }
            if (char_list.size() >= 10)
            {
                break;
            }
        }
    }

    if (char_list.size())
    {
        std::map<int, boost::shared_ptr<CharData> >::iterator itt = char_list.begin();
        while (itt != char_list.end())
        {
            if (itt->second.get() == NULL
                || Singleton<relationMgr>::Instance().is_my_attention(cdata->m_id, itt->first))
            {
                ++itt;
                continue;
            }
            json_spirit::Object obj;
            obj.push_back( Pair("id", itt->second.get()->m_id) );
            obj.push_back( Pair("name", itt->second.get()->m_name) );
            obj.push_back( Pair("gender", itt->second.get()->m_gender) );
            obj.push_back( Pair("level", itt->second.get()->m_level) );
            if (itt->second.get()->m_corps_member.get())
            {
                obj.push_back( Pair("troop", corpsMgr::getInstance()->getCorpsName(itt->second.get()->m_corps_member->corps)) );
            }
            obj.push_back( Pair("gcount", itt->second.get()->GetGenerals().m_generals.size()) );
            boost::shared_ptr<ZhenData> zdata = itt->second.get()->m_zhens.GetZhen(itt->second.get()->m_zhens.GetDefault());
            if (zdata.get())
            {
                obj.push_back( Pair("attack", zdata->getAttack()) );
            }
            list.push_back(obj);
            ++itt;
        }
        robj.push_back( Pair("list", list) );
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

int GeneralDataMgr::getOpenZhen(int stronghold)
{
    int ret = 0;
    std::map<int, int>::iterator it = m_zhen_open_map.begin();    //阵型开放关卡
    while (it != m_zhen_open_map.end())
    {
        if (it->first <= stronghold && ret < it->second)
        {
            ret = it->second;
        }
        ++it;
    }
    return ret;
}

//查询官职武将
officalgenerals* GeneralDataMgr::getOfficalGeneral(int gid)
{
    std::map<int, boost::shared_ptr<officalgenerals> >::iterator it = m_base_offical_generals_map.find(gid);
    if (it != m_base_offical_generals_map.end())
    {
        return (it->second.get());
    }
    return NULL;
}

//能否购买官职武将
bool GeneralDataMgr::canBuyOfficalGeneral(int gid, int slevel, int offical)
{
    officalgenerals* og = getOfficalGeneral(gid);
    if (og)
    {
        //官职不够或者关卡进度不够
        if (og->m_special
            || slevel < og->need_slevel
            || offical < og->need_offical)
        {
            return false;
        }
        return true;
    }
    return false;
}

#if 0
int GeneralDataMgr::getWeakCamps()
{
    int weakcamp = 0;
    if (camp1_cnt < 10 && camp2_cnt < 30)
        return 0;
    if (camp1_cnt < camp2_cnt)
    {
        if (((double)camp2_cnt - (double)camp1_cnt) / (double)camp1_cnt > 0.1)
            weakcamp = 1;
    }
    else
    {
        if (((double)camp1_cnt - (double)camp2_cnt) / (double)camp2_cnt > 0.1)
            weakcamp = 2;
    }
    return weakcamp;
}

int GeneralDataMgr::updateCampCnt(int type)
{
    if (type == 1)
        camp1_cnt++;
    else if(type == 2)
        camp2_cnt++;
    //cout << "camp1_cnt=" << camp1_cnt << ",camp2_cnt=" << camp2_cnt << endl;
    return 0;
}

int GeneralDataMgr::RevoltCamps(int& type)
{
    int weakcamp = getWeakCamps();
    if (type == 1 && weakcamp == 2)
    {
        type = 2;
        //camp2_cnt++;
        //camp1_cnt--;
        //if (camp1_cnt < 0)
        //    camp1_cnt = 0;
        return 0;
    }
    else if (type == 2 && weakcamp == 1)
    {
        type = 1;
        //camp1_cnt++;
        //camp2_cnt--;
        //if (camp2_cnt < 0)
        //    camp2_cnt = 0;
        return 0;
    }
    return -1;
}
#endif

//检查系统公告变化
void GeneralDataMgr::checkAdminNotice(int type)
{
    //cout<<"checkAdminNotice("<<type<<")"<<endl;
    if (1 == type)
    {
        for (std::list<admin_notice>::iterator it = m_adminNotices.begin(); it != m_adminNotices.end(); ++it)
        {
            it->_state = 0;
        }
        Query q(GetDb());
        q.get_result("select id,message from admin_notice where types='1' and state='1' and now()>=starttime and now()<endtime");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int id = q.getval();
            //cout<<"get notice "<<id<<endl;
            std::string message = q.getstr();
            bool bNewNotice = true;
            for (std::list<admin_notice>::iterator it = m_adminNotices.begin(); it != m_adminNotices.end(); ++it)
            {
                //cout<<"loop notice "<<it->_id<<endl;
                if (id == it->_id)
                {
                    if (it->_message == message)
                    {
                        it->_state = 1;    //无变化
                    }
                    else
                    {
                        it->_state = 2;    //内容有变化
                        it->_message = message;
                    }
                    bNewNotice = false;
                    break;
                }
            }
            if (bNewNotice)
            {
                admin_notice newNotice;
                newNotice._id = id;
                newNotice._message = message;
                newNotice._state = 3;    //新增的
                m_adminNotices.push_back(newNotice);
                //cout<<"add new notice "<<id<<endl;
            }
        }
        q.free_result();

        std::list<admin_notice>::iterator it = m_adminNotices.begin();
        while (it != m_adminNotices.end())
        {
            //cout<<"loop 2222222222 notice "<<it->_id<<endl;
            switch (it->_state)
            {
                case 0:
                {
                    //广播删除
                    json_spirit::mObject obj;
                    obj["cmd"] = "deleteAdminNotice";
                    obj["id"] = it->_id;
                    if (0 != InsertInternalActionWork(obj))
                    {
                        ERR();
                    }
                    it = m_adminNotices.erase(it);
                    break;
                }
                case 2:
                    {
                        //广播变化
                        json_spirit::mObject obj;
                        obj["cmd"] = "changeAdminNotice";
                        obj["id"] = it->_id;
                        obj["msg"] = it->_message;
                        if (0 != InsertInternalActionWork(obj))
                        {
                            ERR();
                        }
                        ++it;
                    }
                    break;
                case 3:
                    {
                        //广播新增
                        json_spirit::mObject obj;
                        obj["cmd"] = "addAdminNotice";
                        obj["id"] = it->_id;
                        obj["msg"] = it->_message;
                        if (0 != InsertInternalActionWork(obj))
                        {
                            ERR();
                        }
                        ++it;
                        break;
                    }
                default:
                    ++it;
                    break;
            }
        }
    }
    else
    {
        std::list<int> id_list;
        std::list<std::string> msg_list;
        std::list<int> type_list;
        Query q(GetDb());
        q.get_result("select id,message,types from admin_notice where types!='1' and state='1' and count>0 and intertval > 0 and now()>starttime");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            id_list.push_back(q.getval());
            msg_list.push_back(q.getstr());
            type_list.push_back(q.getval());
        }
        q.free_result();
        std::list<int>::iterator it_id = id_list.begin();
        std::list<std::string>::iterator it_m = msg_list.begin();
        std::list<int>::iterator it_type = type_list.begin();
        while (it_id != id_list.end() && it_m != msg_list.end() && it_type != type_list.end())
        {
            q.execute("update admin_notice set count=count-1,starttime=FROM_UNIXTIME(UNIX_TIMESTAMP()+intertval) where count>0 and id=" + LEX_CAST_STR(*it_id));
            //广播消息
            json_spirit::mObject obj;
            obj["cmd"] = "sendAdminNotice";
            obj["id"] = *it_id;
            obj["msg"] = *it_m;
            obj["type"] = *it_type;

            if (0 != InsertInternalActionWork(obj))
            {
                ERR();
            }
            ++it_id;
            ++it_m;
            ++it_type;
        }
    }
}

int GeneralDataMgr::adminNoticeDeleted(int id)
{
    std::list<admin_notice>::iterator it = m_currentAdminNotices.begin();
    while (it != m_currentAdminNotices.end())
    {
        if (it->_id == id)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "notice") );
            obj.push_back( Pair("type", 2) );
            obj.push_back( Pair("id", id) );
            GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(json_spirit::write(obj));
            m_currentAdminNotices.erase(it);
            return HC_SUCCESS;
        }
        ++it;
    }
    return HC_SUCCESS;
}

int GeneralDataMgr::adminNoticeNew(int id, const std::string& message)
{
    admin_notice newNotice;
    newNotice._id = id;
    newNotice._message = message;
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "notice") );
    obj.push_back( Pair("type", 1) );
    obj.push_back( Pair("id", id) );
    obj.push_back( Pair("content", message) );
    GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(json_spirit::write(obj));
    m_currentAdminNotices.push_back(newNotice);
    return HC_SUCCESS;
}

int GeneralDataMgr::adminNoticeChanged(int id, const std::string& message)
{
    std::list<admin_notice>::iterator it = m_currentAdminNotices.begin();
    while (it != m_currentAdminNotices.end())
    {
        if (it->_id == id)
        {
            it->_message = message;
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "notice") );
            obj.push_back( Pair("type", 3) );
            obj.push_back( Pair("id", id) );
            obj.push_back( Pair("content", message) );
            GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(json_spirit::write(obj));
            return HC_SUCCESS;
        }
        ++it;
    }
    return HC_SUCCESS;
}

int GeneralDataMgr::getAdminNotice(json_spirit::Array& notice_list)
{
    notice_list.clear();
    std::list<admin_notice>::iterator it = m_currentAdminNotices.begin();
    while (it != m_currentAdminNotices.end())
    {
        json_spirit::Object obj;
        obj.push_back( Pair("id", it->_id) );
        obj.push_back( Pair("content", it->_message) );
        notice_list.push_back(obj);
        ++it;
    }
    return HC_SUCCESS;
}

void GeneralDataMgr::shutdown()
{
    cout<<"*************** shutdown server start *******************"<<endl;
    boost::unordered_map<std::string, boost::shared_ptr<OnlineUser> >::iterator it = m_onlineuserlist.begin();
    while (it != m_onlineuserlist.end())
    {
        if (it->second.get() && it->second->m_onlineCharactor.get())
        {
            GeneralDataMgr::getInstance()->CharactorLogout(it->second->m_onlineCharactor);
        }
        ++it;
    }
    cout<<"*************** shutdown server end *******************"<<endl;
}

void GeneralDataMgr::addGeneral(int gid, int cid)
{
    m_general_map[gid] = cid;
}

int GeneralDataMgr::getGeneralOwner(int gid)
{
    if (m_general_map.find(gid) != m_general_map.end())
    {
        return m_general_map[gid];
    }
    return 0;
}

//camp = 1,2,0,
//void GeneralDataMgr::campRaceWinner(int camp)
//{
//    switch (camp)
//    {
//        case 1:
//            ++m_camp_race_wins[0];
//            setInt("camp_race_win1", m_camp_race_wins[0]);
//            break;
//        case 2:
//            ++m_camp_race_wins[1];
//            setInt("camp_race_win2", m_camp_race_wins[1]);
//            break;
//    }
//}

int GeneralDataMgr::getInt(const std::string& field, int defaultv)
{
    int value = defaultv;
    Query q(GetDb());
    q.get_result("SELECT value from custom_settings WHERE code='" + field + "'");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        value = q.getval();
    }
    q.free_result();

    return value;
}

void GeneralDataMgr::setInt(const std::string& field, int value)
{
    InsertSaveDb("replace into custom_settings (code,value) values ('" + field + "','" + LEX_CAST_STR(value) + "')");
}

std::string GeneralDataMgr::getStr(const std::string& field)
{
    std::string value = "";
    Query q(GetDb());
    q.get_result("SELECT value from custom_settings WHERE code='" + field + "'");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        value = q.getstr();
    }
    q.free_result();

    return value;
}

void GeneralDataMgr::setStr(const std::string& field, const std::string& value)
{
    InsertSaveDb("replace into custom_settings (code,value) values ('" + field + "','" + GetDb().safestr(value) + "')");
}

//全服免费VIP4活动是否开启
bool GeneralDataMgr::isFreeVIPEventOpen()
{
    return m_free_vip4_endtime > 0 && time(NULL) < m_free_vip4_endtime;
}

//开启免费vip4
void GeneralDataMgr::openFreeVIP4Event(int day)
{
    m_free_vip4_endtime = 0;
    //持续几天
    if (day > 0)
    {
        time_t timex = time(NULL);

        struct tm tm;
        struct tm *t = &tm;
        localtime_r(&timex, t);
        t->tm_hour = 23;
        t->tm_min = 59;
        t->tm_sec = 59;
        m_free_vip4_endtime = mktime(t) + (day - 1) * iONE_DAY_SECS;

        setInt("free_vip4_endtime", m_free_vip4_endtime);
    }
}

void GeneralDataMgr::getRandomServantList(std::vector<int>& list)
{
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get() && it->second->m_currentStronghold >= iServantRealOpenStronghold && it->second->m_level < 35)
        {
            list.push_back(it->first);
        }
        ++it;
    }
}

int GeneralDataMgr::GetBaseMallGoodId(int type, int id)
{
    std::map<int, boost::shared_ptr<baseGoods> >::iterator it = m_base_mall_goods.begin();
    while(it != m_base_mall_goods.end())
    {
        boost::shared_ptr<baseGoods> p_bg = it->second;
        if (p_bg.get() && p_bg->m_item.get())
        {
            if (p_bg->m_item->id == id && p_bg->m_item->type == type)
            {
                return p_bg->id;
            }
        }
        ++it;
    }
    return 0;
}

std::map<int, boost::shared_ptr<baseGoods> >& GeneralDataMgr::GetBaseMallGoods()
{
    return m_base_mall_goods;
}

void GeneralDataMgr::openMallDiscountEvent(int discount, time_t start_time, time_t end_time)
{
    m_mall_discount_st.discount = discount;
    m_mall_discount_st.start = start_time;
    m_mall_discount_st.end = end_time;
    //json object to string
    json_spirit::Object discount_obj;
    discount_obj.push_back( Pair("discount", m_mall_discount_st.discount) );
    discount_obj.push_back( Pair("start_time", m_mall_discount_st.start) );
    discount_obj.push_back( Pair("end_time", m_mall_discount_st.end) );
    setStr(CHANGE_MALL_DISCOUNT, json_spirit::write(discount_obj));
}

float GeneralDataMgr::getMallDiscount()
{
    float fDiscount = 1;
    time_t tNow = time(NULL);
    if(m_mall_discount_st.start< tNow && tNow< m_mall_discount_st.end)
    {
        fDiscount = m_mall_discount_st.discount / 100.0;
    }
    return fDiscount;
}

std::string GeneralDataMgr::getRandomName(int gender/*1 male, 0 female*/)
{
    if (gender == 1)
    {
        return m_first_name_list_male[my_random(0, m_first_name_list_male.size()-1)] + m_second_name_list_male[my_random(0, m_second_name_list_male.size()-1)];
    }
    else
    {
        return m_first_name_list_female[my_random(0, m_first_name_list_female.size()-1)] + m_second_name_list_female[my_random(0, m_second_name_list_female.size()-1)];
    }
}

#ifdef QQ_PLAT
//每5分钟腾讯系统统计
void GeneralDataMgr::to_Tencent()
{
    ser_to_tencent("online_cnt", "", m_online_charactors.size());
}
#endif

#if 0
void GeneralDataMgr::checkWeakCamp()
{
    //阵营战连胜3次强势阵营
    if (m_camp_race_wins[0] >= 3)
    {
        m_weak_camp = 2;
    }
    else if (m_camp_race_wins[1] >= 3)
    {
        m_weak_camp = 1;
    }
    else
    {
        Query q(GetDb());
        q.get_result("SELECT cd.camp,sum(c.level) FROM `char_data` as cd left join `charactors` as c on cd.cid = c.id WHERE UNIX_TIMESTAMP()-c.lastlogin < 259200 and (cd.camp = 1 or cd.camp = 2) group by camp");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int camp = q.getval();
            if (camp == 1)
                m_camp_strength[0] = q.getval();
            else if (camp == 2)
                m_camp_strength[1] = q.getval();
        }
        q.free_result();

        q.get_result("SELECT cd.camp,count(cd.cid) FROM `char_data` as cd left join `charactors` as c on cd.cid = c.id WHERE UNIX_TIMESTAMP()-c.lastlogin < 259200 and (cd.camp = 1 or cd.camp = 2) group by camp");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int camp = q.getval();
            if (camp == 1)
                m_camp_count[0] = q.getval();
            else if (camp == 2)
                m_camp_count[1] = q.getval();
        }
        q.free_result();

        if (m_camp_count[0] < 10 && m_camp_count[1] < 30)
        {
            m_weak_camp = 0;
        }
        else if (m_camp_strength[0] > (6*m_camp_strength[1]/5))
        {
            m_weak_camp = 2;
        }
        else if (m_camp_strength[1] > (6*m_camp_strength[0]/5))
        {
            m_weak_camp = 1;
        }
        else
        {
            m_weak_camp = 0;
        }
    }
}
#endif

void stand_in_get::save()
{
    //存数据库
    InsertSaveDb("replace into char_stand_in (cid,type,enable,payed,prestige,silver) values ("
            + LEX_CAST_STR(cid) + ","
            + LEX_CAST_STR(type) + ","
            + LEX_CAST_STR(enable) + ","
            + LEX_CAST_STR(payed) + ","
            + LEX_CAST_STR(prestige) + ","
            + LEX_CAST_STR(silver) + ")"    );
}

//查询替身娃娃设置状态
void stand_in_mob::getStandIn(int cid, int& enable, int& silver, int& prestige)
{
    enable = 0;
    silver = 0;
    prestige = 0;
    std::map<int, stand_in_get>::iterator it = m_stand_ins.find(cid);
    if (it != m_stand_ins.end())
    {
        enable = it->second.enable;
        silver = it->second.silver;
        prestige = it->second.prestige;
    }
    return;
}

//设置替身娃娃
void stand_in_mob::setStandIn(int cid, int enable)
{
    std::map<int, stand_in_get>::iterator it = m_stand_ins.find(cid);
    if (it != m_stand_ins.end())
    {
        if (it->second.enable != enable)
        {
            it->second.enable = enable;
            if (enable == 0)
                it->second.payed = 0;
            //存数据库
            it->second.save();
        }
    }
    else if (enable)
    {
        stand_in_get g;
        g.enable = 1;
        g.prestige = 0;
        g.silver = 0;
        g.cid = cid;
        g.type = m_type;
        g.payed = 0;
        //存数据库
        g.save();

        m_stand_ins[cid] = g;
    }
}

//替身娃娃扣金币
void stand_in_mob::processGold()
{
    for (std::map<int, stand_in_get>::iterator it = m_stand_ins.begin(); it != m_stand_ins.end(); ++it)
    {
        if (it->second.enable && it->second.payed == 0)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(it->first).get();
            if (pc)
            {
                if (pc->addGold(-30) >= 0)
                {
                    //金币消耗统计
                    add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 30, gold_cost_for_baby, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
                    gold_cost_tencent(pc,30,gold_cost_for_baby);
#endif
                    it->second.payed = 1;
                    pc->NotifyCharData();
                }
                else
                {
                    //钱不够就取消替身娃娃
                    it->second.enable = 0;
                }
                //存数据库
                it->second.save();
            }
        }
    }
}

//处理替身娃娃的奖励
void stand_in_mob::processReward(int prestige_fac, int attack_fac)
{
    if (m_type >= baby_boss_start && m_type <= baby_boss_end)
    {
        int boss_id = m_type - baby_boss_start;
        boost::shared_ptr<spls_boss> spb = bossMgr::getInstance()->getBoss(boss_id, 0);
        if (!spb.get())
        {
            return;
        }
        for (std::map<int, stand_in_get>::iterator it = m_stand_ins.begin(); it != m_stand_ins.end(); ++it)
        {
            if (it->second.enable)
            {
                CharData* pc = GeneralDataMgr::getInstance()->GetCharData(it->first).get();
                if (pc && spb->m_damage_maps[pc->m_id] == 0 && (it->second.payed || pc->addGold(-30) >= 0))
                {
                    if (it->second.payed == 0)
                    {
                        //金币消耗统计
                        add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 30, gold_cost_for_baby, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
                        gold_cost_tencent(pc,30,gold_cost_for_baby);
#endif
                    }
                    int silver = pc->m_level * 3000;
                    int prestige = 100;
                    if (attack_fac > 0 && prestige_fac > 0)
                    {
                        prestige = pc->getAttack() * prestige_fac / attack_fac;
                    }
                    if (prestige < 100)
                    {
                        prestige = 100;
                    }
                    pc->addPrestige(prestige);
                    pc->addSilver(silver);
                    it->second.silver = silver;
                    it->second.prestige = prestige;
                    it->second.payed = 0;

                    pc->NotifyCharData();

                    //存数据库
                    it->second.save();
                    if (boss_id == 1)
                    {
                        dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_attak_boss1);
                    }
                    else if (boss_id == 4)
                    {
                        dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_attak_boss4);
                    }

                    std::string mailContent = strBossBabyMail;
                    str_replace(mailContent, "$B", spb->_boss._name, true);
                    str_replace(mailContent, "$S", LEX_CAST_STR(silver), true);
                    str_replace(mailContent, "$P", LEX_CAST_STR(prestige), true);
                    //发送系统邮件
                    sendSystemMail(pc->m_name, pc->m_id, strBossMailTitle, mailContent);
                }
            }
        }
    }
    else if (m_type == baby_camp_race)
    {
        for (std::map<int, stand_in_get>::iterator it = m_stand_ins.begin(); it != m_stand_ins.end(); ++it)
        {
            if (it->second.enable)
            {
                CharData* pc = GeneralDataMgr::getInstance()->GetCharData(it->first).get();
                if (pc && (it->second.payed || pc->addGold(-30) >= 0))
                {
                    if (it->second.payed == 0)
                    {
                        //金币消耗统计
                        add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 30, gold_cost_for_baby, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
                        gold_cost_tencent(pc,30,gold_cost_for_baby);
#endif
                    }
                    int silver = pc->m_level * 3000;
                    int prestige = 100;
                    if (attack_fac > 0 && prestige_fac > 0)
                    {
                        prestige = pc->getAttack() * prestige_fac / attack_fac;
                    }
                    if (prestige < 100)
                    {
                        prestige = 100;
                    }
                    pc->addPrestige(prestige);
                    pc->addSilver(silver);
                    it->second.silver = silver;
                    it->second.prestige = prestige;
                    it->second.payed = 0;

                    pc->NotifyCharData();

                    //存数据库
                    it->second.save();
                    dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_camprace);
                    std::string mailContent = strCampRaceBabyMail;
                    str_replace(mailContent, "$S", LEX_CAST_STR(silver), true);
                    str_replace(mailContent, "$P", LEX_CAST_STR(prestige), true);
                    //发送系统邮件
                    sendSystemMail(pc->m_name, pc->m_id, strCampRaceMailTitle, mailContent);
                }
            }
        }
    }
}

void stand_in_mob::load()
{
    Query q(GetDb());
    q.get_result("select cid,enable,payed,silver,prestige from char_stand_in where type=" + LEX_CAST_STR(m_type));
    while (q.fetch_row())
    {
        stand_in_get g;
        g.type = m_type;
        g.cid = q.getval();
        g.enable = q.getval();
        g.payed = q.getval();
        g.silver = q.getval();
        g.prestige = q.getval();
        m_stand_ins[g.cid] = g;
    }
    q.free_result();
}

const json_spirit::Array& baseVIPPresent::getArray() const
{
    return m_item_list;
}

void baseVIPPresent::updateObj()
{
    m_item_list.clear();
    for (std::list<Item>::iterator it_i = m_list.begin(); it_i != m_list.end(); ++it_i)
    {
        json_spirit::Object obj;
        it_i->toObj(obj);
        m_item_list.push_back(obj);
    }
}

void nick::add_nick(int n)
{
    std::list<int>::iterator it = m_nick_list.begin();
    while (it != m_nick_list.end())
    {
        if (n < *it)
        {
            m_nick_list.insert(it, n);
            return;
        }
        else if (n == *it)
        {
            return;
        }
        ++it;
    }
    m_nick_list.push_back(n);
}

void nick::remove_nick(int n)
{
    std::list<int>::iterator it = m_nick_list.begin();
    while (it != m_nick_list.end())
    {
        if (*it == n)
        {
            m_nick_list.erase(it);
            return;
        }
        ++it;
    }
}

bool nick::check_nick(int n)
{
    std::list<int>::iterator it = m_nick_list.begin();
    while (it != m_nick_list.end())
    {
        if (*it == n)
        {
            return true;
        }
        ++it;
    }
    return false;
}

std::string nick::get_string()
{
    if (m_nick_list.size())
    {
        std::string ret = "[";
        std::list<int>::iterator it = m_nick_list.begin();
        while (it != m_nick_list.end())
        {
            if (it != m_nick_list.begin())
            {
                ret += ",";
            }
            ret += LEX_CAST_STR(*it);
            ++it;
        }
        ret += "]";
        return ret;
    }
    else
    {
        return "[]";
    }
}

void nick::init(const std::string& data)
{
    json_spirit::Value value;
    json_spirit::read(data, value);

    if (value.type() == json_spirit::array_type)
    {
        json_spirit::Array& a = value.get_array();
        for (json_spirit::Array::iterator it = a.begin(); it != a.end(); ++it)
        {
            if ((*it).type() == json_spirit::int_type)
            {
                add_nick((*it).get_int());
            }
        }
    }
}

//创建角色
int CreateChar(const std::string& account, int union_id, const std::string& server_id,  const std::string& qid, int camp, int spic, const std::string& xname, int g1, int g2, uint64_t& cid, int inv_id)
{
    std::string name = GetDb().safestr(xname);
    if (name != xname)
    {
        return HC_ERROR_NAME_ILLEGAL;
    }
    static boost::shared_ptr<BaseZhenData> zhen = GeneralDataMgr::getInstance()->GetBaseZhen(iDefaultZhenType);
    if (!zhen.get())
    {
        ERR();
        return HC_ERROR;
    }
    if (spic < 1)
    {
        spic = 1;
    }
    else if (spic > maxCharactorSpic)
    {
        spic = maxCharactorSpic;
    }
    int check_cid = GeneralDataMgr::getInstance()->GetCharId(name);
    if (check_cid)
    {
        return HC_ERROR_CHAR_EXIST;
    }
    //创建内存角色
    time_t time_now = time(NULL);
    cid = GeneralDataMgr::getInstance()->newCharId();
    boost::shared_ptr<CharData> p_chardata;

    //cout<<"createChar--> new "<<cid<<",tid:"<<syscall(SYS_gettid)<<endl;
    p_chardata.reset(new CharData(cid,true));
    //charactors,char_data,char_resource
    {
        p_chardata->m_account = account;
        p_chardata->m_union_id = union_id;
        p_chardata->m_server_id = server_id;
        p_chardata->m_qid = qid;
        p_chardata->m_name = name;
        p_chardata->m_spic = spic;
        p_chardata->m_login_time = time_now;
         p_chardata->m_createTime = time_now;
        struct tm tm;
        struct tm *t = &tm;
        localtime_r(&time_now, t);
        t->tm_hour = 0;
        t->tm_min = 0;
        t->tm_sec = 0;
        time_t tend = mktime(t) + 3*iONE_DAY_SECS;
        //time_t tend = time_now - (time_now % iONE_DAY_SECS) + (3 * iONE_DAY_SECS);
        p_chardata->setExtraData(char_data_type_normal, char_data_get_onlinegift_day, tend);
        tend += 4*iONE_DAY_SECS;
        p_chardata->setExtraData(char_data_type_normal, char_data_get_continue_login_day, tend);
        //根据图片确定性别奇数为男
        if (p_chardata->m_spic % 2 != 0)
        {
            p_chardata->m_gender = 1;
        }
        else
        {
            p_chardata->m_gender = 0;
        }
        p_chardata->m_chat = strCharChatMessage[spic-1];
        p_chardata->m_camp = camp;
        p_chardata->m_close_friend_to = inv_id;
    }
    saveDbJob job;
    job.sqls.push_back("insert into charactors set account='" + account
            + "',name='" + (name)
            + "',level='1',spic='" + LEX_CAST_STR(spic)
            + "',lastlogin=unix_timestamp(),createTime=unix_timestamp(),id=" + LEX_CAST_STR(cid));
    job.sqls.push_back("insert into char_data set cid='" + LEX_CAST_STR(cid)
            + "',camp='" + LEX_CAST_STR(camp)
            + "',backPack=0,freeRest='5',vip='0',chat='" + GetDb().safestr(strCharChatMessage[spic-1]) + "',cLevel=1,exp=0,official=1");
    job.sqls.push_back("insert into char_resource set cid=" + LEX_CAST_STR(cid)
                    + ",gold=" + LEX_CAST_STR(g_default_gold)
                    + ",silver=" + LEX_CAST_STR(g_default_silver)
                    + ",ling=" + LEX_CAST_STR(g_default_ling)
                    + ",explore_ling=" + LEX_CAST_STR(iFreeExploreTimes)
                    + ",rtime=unix_timestamp()");
    //char_generals
    int gid1 = 0, gid2 = 0;
    if (g1 > 0)
    {
        gid1 = p_chardata->m_generals.Add(g1, false);
    }
    if (g2 > 0)
    {
        gid2 = p_chardata->m_generals.Add(g2, false);
    }

    //char_zhens
    {
        p_chardata->m_zhens.m_default_zhen = zhen->m_type;
        boost::shared_ptr<ZhenData> zhen_d;
        zhen_d.reset(new ZhenData(cid, *p_chardata));
        zhen_d->m_zhen_type = zhen->m_type;
        zhen_d->m_level = 5;
        zhen_d->m_name = zhen->m_name;

        //直接开放5级阵
        zhen_d->m_generals[zhen->m_open_pos[0]-1] = gid2;
        zhen_d->m_generals[zhen->m_open_pos[1]-1] = gid1;
        zhen_d->m_generals[zhen->m_open_pos[2]-1] = 0;
        zhen_d->m_generals[zhen->m_open_pos[3]-1] = 0;
        zhen_d->m_generals[zhen->m_open_pos[4]-1] = 0;

        p_chardata->m_zhens.m_zhens[zhen_d->m_zhen_type] = zhen_d;
    }
    std::string sql = "insert into char_zhens set cid=" + LEX_CAST_STR(cid) + ",type=" + LEX_CAST_STR(zhen->m_type)
        + ",level=2";
        sql += ",pos" + LEX_CAST_STR(zhen->m_open_pos[0]) + "=" + LEX_CAST_STR(gid2);
        sql += ",pos" + LEX_CAST_STR(zhen->m_open_pos[1]) + "=" + LEX_CAST_STR(gid1);
        sql += ",pos" + LEX_CAST_STR(zhen->m_open_pos[2]) + "='0'";
        sql += ",pos" + LEX_CAST_STR(zhen->m_open_pos[3]) + "='0'";
        sql += ",pos" + LEX_CAST_STR(zhen->m_open_pos[4]) + "='0'";
    job.sqls.push_back(sql);
    job.sqls.push_back("insert into char_default_zhen set cid=" + LEX_CAST_STR(cid) + ",default_zhen=" + LEX_CAST_STR(zhen->m_type));

    //char_stronghold
    baseMap* pm = GeneralDataMgr::getInstance()->GetBaseMap(1).get();
    assert(pm);
    for (int stageid = 1; stageid <= 3; ++stageid)
    {
        CharMapData *pMap = NULL;
        int mapid = 1;
        //是否存在该地图数据，没有则创建
        if (p_chardata->m_tempo.CharMapsData[mapid].get())
        {
            pMap = p_chardata->m_tempo.CharMapsData[mapid].get();
        }
        else
        {
            pMap = new CharMapData;
            p_chardata->m_tempo.CharMapsData[mapid].reset(pMap);
        }
        if (pm->stages[stageid-1].get())
        {
            CharStageData *pStage = NULL;
            //创建场景
            {
                pStage = new CharStageData;
                pStage->m_baseStage = pm->stages[stageid-1];
                (*pMap)[stageid].reset(pStage);
            }
            for (size_t i = 0; i < 25; ++i)
            {
                int m_state = -2;
                if (stageid == 1 && i == 0)
                {
                    m_state = 0;
                }
                else if (i < pm->stages[stageid-1]->size)
                {
                    m_state = -1;
                }
                boost::shared_ptr<StrongholdData> bstronghold = GeneralDataMgr::getInstance()->GetStrongholdData(mapid, stageid, i+1);
                if (!bstronghold.get())
                {
                    break;
                }
                pStage->m_stronghold[i].reset(new CharStrongholdData(cid, bstronghold->m_id, (3 * mapid -1), bstronghold->m_stateNum));
                CharStrongholdData* pshd = pStage->m_stronghold[i].get();
                pshd->m_cid = cid;
                pshd->m_baseStronghold = bstronghold;
                pshd->m_state = m_state;
            }
            pStage->m_cid = cid;
            pStage->m_finished = 0;
            pStage->m_cur_group = 1;

            std::string sqlcmd = "insert into char_stronghold set cid=" + LEX_CAST_STR(cid) + ",mapid=1,stageid=" + LEX_CAST_STR(stageid);
            for (int i = 0; i < pm->stages[stageid-1]->size; ++i)
            {
                if (stageid == 1 && i == 0)
                {
                    sqlcmd += ",pos1=0";
                    continue;
                }
                sqlcmd += ",pos" + LEX_CAST_STR(i+1) + "=-1";

            }
            job.sqls.push_back(sqlcmd);
        }
    }

    //char_skill_research
    skillResearchQue que;
    que.pos = 1;
    que.cid = cid;
    int sid = 0;
    int teacher_id = 0;
    que.start_time = 0;
    que.left_mins = 0;
    que.end_time = 0;
    que.state = 0;
    que.type = 0;
    que.fatigue = 0;
    que.accelerate_time = 0;
    que.final_speed = 0;
    que.more = 0;
    p_chardata->m_skill_queue.push_back(que);
    job.sqls.push_back("insert into char_skill_research (pos,cid,type,sid,teacher,starttime,state) values ("
            + LEX_CAST_STR(1) + ","
            + LEX_CAST_STR(cid) + ",0,0,0,0,0)");
    GeneralDataMgr::getInstance()->addCharData(p_chardata);
    taskMgr::getInstance()->newChar(p_chardata);

    Singleton<new_event_mgr>::Instance().addCharGeneralEvent(*p_chardata);
    Singleton<new_event_mgr>::Instance().addCharBaoshiEvent(*p_chardata);
    //if (job.sqls.size())
    {
        InsertSaveDb(job);
    }
    return HC_SUCCESS;
}

//删除角色
int _DeleteChar(int cid)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    //角色表
    InsertSaveDb("delete from charactors where id=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_data where cid=" + LEX_CAST_STR(cid));
    //探索表
    exploreMgr::getInstance()->deleteChar(cid);
    //InsertSaveDb("delete from char_explore_can where cid=" + LEX_CAST_STR(cid));
    //InsertSaveDb("delete from char_explore_has where cid=" + LEX_CAST_STR(cid));
    //屯田表
    farmMgr::getInstance()->deleteChar(cid);
    //InsertSaveDb("delete from char_farm_task where cid=" + LEX_CAST_STR(cid));
    //InsertSaveDb("delete from char_farm_field where cid=" + LEX_CAST_STR(cid));
    //官职技能表
    //InsertSaveDb("delete from char_offical_skills where cid=" + LEX_CAST_STR(cid));
    //训练表
    InsertSaveDb("delete from char_train_books where cid=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_train_place where cid=" + LEX_CAST_STR(cid));
    //竞技表
    RaceMgr::getInstance()->deleteChar(cid);
    //InsertSaveDb("delete from char_race where cid=" + LEX_CAST_STR(cid));
    //InsertSaveDb("delete from char_race_get where cid=" + LEX_CAST_STR(cid));
    //冶炼表
#if 0
    SmeltMgr::getInstance()->deleteChar(cid);
#endif
    //InsertSaveDb("delete from char_smelt where cid=" + LEX_CAST_STR(cid));
    //InsertSaveDb("delete from char_smelt_task where cid=" + LEX_CAST_STR(cid));
    //军团表
    corpsMgr::getInstance()->deleteChar(cid);
    //InsertSaveDb("delete from char_corps where cid=" + LEX_CAST_STR(cid));
    //InsertSaveDb("delete from char_corps_applications where cid=" + LEX_CAST_STR(cid));
    //InsertSaveDb("delete from char_corps_members where cid=" + LEX_CAST_STR(cid));
    //生辰纲
    guardMgr::getInstance()->deleteChar(cid);

    //玩家资源表
    InsertSaveDb("delete from char_resource where cid=" + LEX_CAST_STR(cid));
    //玩家武将
    InsertSaveDb("delete from char_generals where cid=" + LEX_CAST_STR(cid));
    //玩家阵型
    InsertSaveDb("delete from char_zhens where cid=" + LEX_CAST_STR(cid));
    //玩家阵型
    InsertSaveDb("delete from char_default_zhen where cid=" + LEX_CAST_STR(cid));
    //玩家兵器
    InsertSaveDb("delete from char_new_weapons where cid=" + LEX_CAST_STR(cid));
    //道具表
    InsertSaveDb("delete from char_treasures where cid=" + LEX_CAST_STR(cid));
    //装备表
    InsertSaveDb("delete from char_equipment where cid=" + LEX_CAST_STR(cid));

    //临时表
    InsertSaveDb("delete from char_daily_temp where cid=" + LEX_CAST_STR(cid));
    //状态
    InsertSaveDb("delete from char_new_states where cid=" + LEX_CAST_STR(cid));
    //关卡表
    InsertSaveDb("delete from char_stronghold where cid=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_stronghold_states where cid=" + LEX_CAST_STR(cid));
    //商品
    InsertSaveDb("delete from char_shop_goods where cid=" + LEX_CAST_STR(cid));
    //邮件
    InsertSaveDb("delete from char_mails where cid=" + LEX_CAST_STR(cid));
    //新手冲锋号
    InsertSaveDb("delete from char_newbie_event where cid=" + LEX_CAST_STR(cid));
    //金币确认消息
    InsertSaveDb("delete from char_goldCost_noConfirm where cid=" + LEX_CAST_STR(cid));
    //新手引导完成情况
    InsertSaveDb("delete from char_guide_complete where cid=" + LEX_CAST_STR(cid));
    //地区攻略领取情况
    InsertSaveDb("delete from char_map_intro_get where cid=" + LEX_CAST_STR(cid));
    //连续登录奖励领取情况
    InsertSaveDb("delete from char_continue_login_present where cid=" + LEX_CAST_STR(cid));
    //VIP奖励领取情况
    InsertSaveDb("delete from char_vip_present where cid=" + LEX_CAST_STR(cid));
    //充值满就送奖励领取情况
    InsertSaveDb("delete from char_recharge_event where cid=" + LEX_CAST_STR(cid));

    //技能相关
    if (cdata.get())
    {
        cdata->stopSkillResearchAll();
    }
    InsertSaveDb("delete from char_skills where cid=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_skill_research where cid=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_skill_teachers where cid=" + LEX_CAST_STR(cid));

    //扫荡移除
    InsertSaveDb("delete from char_sweep_task where cid=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_sweep_result where cid=" + LEX_CAST_STR(cid));
    //通商移除
    //tradeMgr::getInstance()->deleteChar(cid);
    //战马移除
    InsertSaveDb("delete from char_horses where cid=" + LEX_CAST_STR(cid));

    //梅花易数移除
    InsertSaveDb("delete from char_lottery_records where cid=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_data_extra where cid=" + LEX_CAST_STR(cid));

    //宝石移除
    InsertSaveDb("delete from char_baoshi where cid=" + LEX_CAST_STR(cid));

    //多人副本移除

    //家丁移除
    servantMgr::getInstance()->deleteChar(cid);
    //日常任务移除

    //移除内存中的角色数据
    GeneralDataMgr::getInstance()->removeCharData(cid);

    return HC_SUCCESS;
}

//删除角色
int DeleteChar(uint64_t cid)
{
    return _DeleteChar(cid);
}

//int GetCharInfo(int cid, int& camp, int& guild, std::string& name)
//{
//    guild = 0;
//   Query q(GetDb());
//    q.get_result("SELECT c.name,c.camp FROM `charactors` as c WHERE c.id=" + LEX_CAST_STR(cid));
//   if (q.fetch_row())
//    {
//        name = q.getstr();
//        camp = q.getval();
//   }
//    q.free_result();
//    return 0;
//}

#ifdef VN_SERVER
int clearDeadCharactors()
{
    cout<<" >>>>>>>>> try remove the dead accounts <<<<<<<<<<"<<endl;
    int count = 0;
    {
        Query q(GetDb());
        //90天未登录
        q.get_result("select count(*) from charactors where (lastlogin+90*24*3600)<unix_timestamp()");
        if (q.fetch_row())
        {
            count = q.getval();
            q.free_result();
        }
        else
        {
            q.free_result();
        }
    }

    if (count)
    {
        time_t time_start = time(NULL);
        {
            Query q(GetDb());
            q.execute("delete from charactors where (lastlogin+90*24*3600)<unix_timestamp()");
            CHECK_DB_ERR(q);
        }

        std::string delete_tables[][2] =
        {
            {"char_all_rewards","cid"},
            {"char_bank_cases","cid"},
            {"char_baoshi","cid"},
            {"char_baoshi_events","cid"},
            {"char_boss_damage_rankings","cid"},
            {"char_buffs","cid"},
            {"char_camp_race_rankings","cid"},
            {"char_chengzhang_event","cid"},
            {"char_close_friend","cid"},
            {"char_close_friend_id","cid"},  {"char_close_friend_id","fid"},
            {"char_congratulations","cid"},  {"char_congratulations","fid"},
            {"char_continue_login_present","cid"},
            {"char_copy_attack","cid"},
            {"char_corps_applications","cid"},
            //{"char_corps_boss","cid"},
            {"char_corps_event","cid"},
            {"char_corps_explore_data","cid"},
            {"char_corps_fighting","cid"},
            {"char_corps_history","cid"},
            //{"char_corps_lottery_data","cid"},
            {"char_corps_members","cid"},
            {"char_daily_recharge","cid"},
            {"char_daily_task","cid"},
            {"char_daily_temp","cid"},
            {"char_data","cid"},
            {"char_data_extra","cid"},
            {"char_data_temp","cid"},
            {"char_default_zhen","cid"},
            {"char_elite_map_tempo","cid"},
            {"char_elite_tempo","cid"},
            {"char_enemys","cid"},  {"char_enemys","eid"},
            {"char_enemy_infos","cid"}, {"char_enemy_infos","eid"},
            {"char_equipment","cid"},
            {"char_explore_can","cid"},
            {"char_explore_has","cid"},
            {"char_farm_field","cid"},
            {"char_farm_water","cid"},
            {"char_feedback","cid"},
            {"char_friends","cid"}, {"char_friends","friend_id"},
            {"char_generals","cid"},
            //{"char_generals_genius_lock","cid"},
            {"char_general_events","cid"},
            {"char_gm_list","cid"},
            {"char_gm_question","cid"},
            {"char_goldCost_noConfirm","cid"},
            {"char_guard_goods","cid"},
            {"char_guard_rankget","cid"},
            {"char_guard_rankscore","cid"},
            {"char_guide_complete","cid"},
            {"char_horses","cid"},
            {"char_horses_action","cid"},
            {"char_invite_data","cid"},
            {"char_jxl_buffs","cid"},
            {"char_libao","cid"},
            {"char_lottery_records","cid"},
            {"char_mails","cid"},
            {"char_map_intro_get","cid"},
            {"char_mazes","cid"},
            {"char_maze_boss","cid"},
            {"char_maze_buffs","cid"},
            {"char_maze_generals","cid"},
            {"char_maze_item_list","cid"},
            {"char_newbie_event","cid"},
            {"char_new_rankings_last","cid"},
            {"char_new_rankings_now","cid"},
            {"char_new_states","cid"},
            {"char_new_weapons","cid"},
            {"char_offical_skills","cid"},
            {"char_opened_packs","cid"},
            {"char_presents","cid"},
            //{"char_qq_yellow","cid"},
            {"char_race","cid"},
            //{"char_race_title","cid"},
            {"char_recharge","cid"},
            {"char_recharge_event","cid"},
            {"char_recharge_event_records","cid"},
            {"char_recharge_event_total","cid"},
            {"char_recved_congratulations","cid"},{"char_recved_congratulations","fid"},
            {"char_resource","cid"},
            {"char_servant","cid"},
            {"char_servant_enemy","cid"},{"char_servant_enemy","enemyid"},
            {"char_servant_event","cid"},
            {"char_servant_loser","cid"},{"char_servant_loser","loserid"},
            {"char_servant_rescue","cid"},{"char_servant_rescue","rescue_cid"},
            {"char_share_event","cid"},
            {"char_shop_goods","cid"},
            {"char_signs","cid"},
            {"char_skills","cid"},
            {"char_skill_research","cid"},
            {"char_skill_teachers","cid"},
            {"char_smelt","cid"},
            {"char_smelt_task","cid"},
            {"char_stand_in","cid"},
            {"char_stronghold","cid"},
            {"char_stronghold_states","cid"},
            {"char_sweep_result","cid"},
            {"char_sweep_task","cid"},
            {"char_sync","cid"},
            {"char_tasks","cid"},
            {"char_tmp_vip","cid"},
            {"char_total_recharge","cid"},
            {"char_trades","cid"},
            {"char_training","cid"},
            {"char_train_books","cid"},
            {"char_train_place","cid"},
            {"char_treasures","cid"},
            {"char_trunk_tasks","cid"},
            {"char_vip_present","cid"},
            {"char_year_temp","cid"},
            {"char_zhens","cid"},
            {"char_zst","cid"},
            {"char_zst_generals","cid"},
            {"char_zst_maps","cid"},
            {"throne_rankings_last","cid"},
            {"",""}
        };
        for (int i = 0; delete_tables[i][0] != ""; ++i)
        {
            Query q(GetDb());
            if (delete_tables[i][0] == "char_mails")
            {
                q.execute("delete from " + delete_tables[i][0] + " where " + delete_tables[i][1] + "!=0 and " + delete_tables[i][1] + " not in (select id from charactors)");
            }
            else
            {
                q.execute("delete from " + delete_tables[i][0] + " where " + delete_tables[i][1] + " not in (select id from charactors)");
            }
            CHECK_DB_ERR(q);
        }
        cout<<"clear "<<count<<" dead charactors,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return 0;
}
#else
int clearDeadCharactors()
{
    return 0;
}
#endif

//清理死号
int clearDeadCharactorsOld()
{
    time_t time_start = time(NULL);

    std::list<int> delete_list;
    Query q(GetDb());
    q.get_result("select id from charactors where (level<=2 and (lastlogin+24*3600)<unix_timestamp()) or (level>=3 and level<=7 and (lastlogin+36*3600)<unix_timestamp()) or (level>=8 and level<=12 and (lastlogin+72*3600)<unix_timestamp())");
    while (q.fetch_row())
    {
        delete_list.push_back(q.getval());
    }
    q.free_result();
    for (std::list<int>::iterator it = delete_list.begin(); it != delete_list.end(); ++it)
    {
        int cid = *it;

        if (cid == 0)
        {
            continue;
        }
        //充值用户不删除
        q.get_result("select count(*) from pay_list as p left join accounts as a on p.qid=a.qid left join charactors as c on c.account=a.account where p.pay_result='1' and c.id=" + LEX_CAST_STR(cid));
        if (q.fetch_row() && q.getval() > 0)
        {
            q.free_result();
            continue;
        }
        else
        {
            q.free_result();
        }
        //角色表
        q.execute("delete from charactors where id=" + LEX_CAST_STR(cid));
        q.execute("delete from char_data where cid=" + LEX_CAST_STR(cid));
        //探索表
        q.execute("delete from char_explore_can where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_explore_has where cid=" + LEX_CAST_STR(cid));

        //屯田表
        q.execute("delete from char_farm_task where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_farm_field where cid=" + LEX_CAST_STR(cid));

        //官职技能表
        //q.execute("delete from char_offical_skills where cid=" + LEX_CAST_STR(cid));
        //训练表
        q.execute("delete from char_train_books where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_train_place where cid=" + LEX_CAST_STR(cid));
        //竞技表
        q.execute("delete from char_race where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_race_get where cid=" + LEX_CAST_STR(cid));
#if 0
        //冶炼表
        q.execute("delete from char_smelt where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_smelt_task where cid=" + LEX_CAST_STR(cid));
#endif
        //军团表
        //corpsMgr::getInstance()->deleteChar(cid);
        q.execute("delete from char_corps where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_corps_applications where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_corps_members where cid=" + LEX_CAST_STR(cid));
        //生辰纲
        //guardMgr::getInstance()->deleteChar(cid);

        //玩家资源表
        q.execute("delete from char_resource where cid=" + LEX_CAST_STR(cid));
        //玩家武将
        q.execute("delete from char_generals where cid=" + LEX_CAST_STR(cid));
        //玩家阵型
        q.execute("delete from char_zhens where cid=" + LEX_CAST_STR(cid));
        //玩家阵型
        q.execute("delete from char_default_zhen where cid=" + LEX_CAST_STR(cid));
        //玩家兵器
        q.execute("delete from char_new_weapons where cid=" + LEX_CAST_STR(cid));
        //道具表
        q.execute("delete from char_treasures where cid=" + LEX_CAST_STR(cid));
        //装备表
        q.execute("delete from char_equipment where cid=" + LEX_CAST_STR(cid));

        //临时表
        q.execute("delete from char_daily_temp where cid=" + LEX_CAST_STR(cid));
        //状态
        q.execute("delete from char_new_states where cid=" + LEX_CAST_STR(cid));
        //关卡表
        q.execute("delete from char_stronghold where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_stronghold_states where cid=" + LEX_CAST_STR(cid));
        //商品
        q.execute("delete from char_shop_goods where cid=" + LEX_CAST_STR(cid));
        //邮件
        q.execute("delete from char_mails where cid=" + LEX_CAST_STR(cid));
        //新手冲锋号
        q.execute("delete from char_newbie_event where cid=" + LEX_CAST_STR(cid));
        //金币确认消息
        q.execute("delete from char_goldCost_noConfirm where cid=" + LEX_CAST_STR(cid));
        //新手引导完成情况
        q.execute("delete from char_guide_complete where cid=" + LEX_CAST_STR(cid));
        //地区攻略领取情况
        q.execute("delete from char_map_intro_get where cid=" + LEX_CAST_STR(cid));
        //连续登录奖励领取情况
        q.execute("delete from char_continue_login_present where cid=" + LEX_CAST_STR(cid));
        //VIP奖励领取情况
        q.execute("delete from char_vip_present where cid=" + LEX_CAST_STR(cid));
        //充值满就送奖励领取情况
        q.execute("delete from char_recharge_event where cid=" + LEX_CAST_STR(cid));

        //技能相关
        q.execute("delete from char_skills where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_skill_research where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_skill_teachers where cid=" + LEX_CAST_STR(cid));

        //扫荡移除

        //通商移除

        //战马移除
        q.execute("delete from char_horses where cid=" + LEX_CAST_STR(cid));

        //梅花易数移除
        q.execute("delete from char_lottery_records where cid=" + LEX_CAST_STR(cid));
        q.execute("delete from char_data_extra where cid=" + LEX_CAST_STR(cid));

        //宝石移除
        q.execute("delete from char_baoshi where cid=" + LEX_CAST_STR(cid));

        //多人副本移除

        //家丁移除

        //日常任务移除
    }
    cout<<"clear dead charactors finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    return 0;
}

//战斗回放
std::string getCombatRecord(int id)
{
    Query q(GetDb());
    q.get_result("select record from battle_records where id=" + LEX_CAST_STR(id));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        std::string result = q.getstr();
        q.free_result();
        return result;
    }
    else
    {
        q.free_result();
        ERR();
        cout<<"battle record not exist:"<<id<<endl;
        return "";
    }
}

//检查礼包
int ProcessCheckPack(json_spirit::mObject& o)
{
    Query q(GetDb());
    std::list<int> id_list;
    std::list<int> cid_list;
    std::list<std::string> content_list;
    q.get_result("select id,cid,content from present_list where processed='0'");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        id_list.push_back(q.getval());
        cid_list.push_back(q.getval());
        content_list.push_back(q.getstr());
    }
    q.free_result();

    std::list<int>::iterator it_id = id_list.begin();
    std::list<int>::iterator it_cid = cid_list.begin();
    std::list<std::string>::iterator it_content = content_list.begin();

    for (;it_id != id_list.end(); ++it_id, ++it_cid, ++it_content)
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(*it_cid);
        if (cdata.get())
        {
            json_spirit::Object robj;
            packsMgr::getInstance()->getPacks(cdata.get(), *it_content, robj);
            //成功处理
            if (!q.execute("update present_list set processed='1',get_time=" + LEX_CAST_STR(time(NULL)) + " where id='" + LEX_CAST_STR(*it_id) + "'"))
            {
                CHECK_DB_ERR(q);
            }
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
            if (account)
            {
                account->Send(strNotifyPresentGet);
            }
        }
        else
        {
            //无法处理
            if (!q.execute("update present_list set processed='2',get_time=" + LEX_CAST_STR(time(NULL)) + " where id='" + LEX_CAST_STR(*it_id) + "'"))
            {
                CHECK_DB_ERR(q);
            }
        }
    }
    return HC_SUCCESS;
}

//检查充值
int ProcessCheckRecharge(json_spirit::mObject& o)
{
    Query q(GetDb());
    std::list<int> orderno_list;
    std::list<std::string> qid_list;
    std::list<int> union_id_list;
    std::list<std::string> server_id_list;
    std::list<int> gold_list;
    std::list<std::string> type_list;
    q.get_result("select pay_id,qid,union_id,server_id,pay_net_money,pay_type from pay_list where (UNIX_TIMESTAMP()-UNIX_TIMESTAMP(pay_time)) > 8 && pay_result='0' && pay_iscancel = '0'");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        orderno_list.push_back(q.getval());
        qid_list.push_back(q.getstr());
        union_id_list.push_back(q.getval());
        server_id_list.push_back(q.getstr());
        gold_list.push_back(q.getval());
        type_list.push_back(q.getstr());
    }
    q.free_result();

    std::list<int>::iterator it_union_id = union_id_list.begin();
    std::list<int>::iterator it_gold = gold_list.begin();
    std::list<int>::iterator it_orderno = orderno_list.begin();
    std::list<std::string>::iterator it_qid = qid_list.begin();
    std::list<std::string>::iterator it_server_id = server_id_list.begin();
    std::list<std::string>::iterator it_type = type_list.begin();
    for (;it_orderno != orderno_list.end(); ++it_qid, ++it_union_id, ++it_gold, ++it_orderno, ++it_server_id, ++it_type)
#ifdef ONE_CHARACTOR
    {
        int account_id = 0;
        //帐号是否存在
        q.get_result("select c.id from accounts as ac left join charactors as c on ac.account=c.account where ac.qid='" + GetDb().safestr(*it_qid)
                    + "' and ac.union_id='" + LEX_CAST_STR(*it_union_id)
                    + "' and ac.server_id='" + GetDb().safestr(*it_server_id)
                    + "' order by c.id limit 1");
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            account_id = q.getval();
            q.free_result();

            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(account_id);
            if (cdata.get())
            {
                //cout<<"recharge add gold to cid:"<<account_id<<",gold:"<<*it_gold<<endl;
                cdata->addGold(*it_gold);
                int recharge_type = gold_get_recharge;
                if ("1" == *it_type)
                {
                    recharge_type = gold_get_gift_recharge;
                }
                else if ("2" == *it_type)
                {
                    recharge_type = gold_get_plat_recharge;
                }
                //金币获得统计
                add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address,*it_gold, recharge_type, cdata->m_union_id, cdata->m_server_id);
                //成功处理
                time_t recharge_time = time(NULL);
                InsertSaveDb("insert into char_recharge set type='" + GetDb().safestr(*it_type) + "',cid=" + LEX_CAST_STR(cdata->m_id)
                        + ",account='',gold='" + LEX_CAST_STR(*it_gold) + "',input=FROM_UNIXTIME(" + LEX_CAST_STR(recharge_time) + ")");
                if (cdata->queryExtraData(char_data_type_normal, char_data_first_recharge_gift) == 0)
                {
                    //设置首充礼包
                    cdata->setExtraData(char_data_type_normal, char_data_first_recharge_gift, 1);
                    //更新礼包活动按钮
                    cdata->notifyEventState(top_level_event_first_recharge, 1, 0);
                }
                cdata->m_total_recharge += *it_gold;
                InsertSaveDb("replace into char_total_recharge (cid,total_recharge) values (" + LEX_CAST_STR(cdata->m_id) + "," + LEX_CAST_STR(cdata->m_total_recharge) + ")");
                cdata->updateVip();
                //充值活动
                recharge_event_mgr::getInstance()->updateRechargeEvent(cdata->m_id, *it_gold, recharge_time);
                //cdata->updateRechargeReward(*it_gold);
                //日充值活动
                Singleton<new_event_mgr>::Instance().addDailyRecharge(*(cdata.get()), *it_gold);

                //周排行活动
                int score = *it_gold;
                if (score > 0)
                    newRankings::getInstance()->updateEventRankings(cdata->m_id,rankings_event_recharge,score);

                //通知金币变化
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
                if (account.get() && account->m_sockethandle.get())
                {
                    cdata->NotifyCharData();
                    cdata->NotifyCharOpenInfo();
                    cdata->NotifyCharData_(account->m_sockethandle);
                    cdata->realNotifyOpenInfo(account->m_sockethandle);
                }
                //成功处理
                if (!q.execute("update pay_list set pay_result='1',pay_endtime=FROM_UNIXTIME(" + LEX_CAST_STR(recharge_time) + ") where pay_id='" + LEX_CAST_STR(*it_orderno) + "'"))
                {
                    CHECK_DB_ERR(q);
                }
            }
            else
            {
                //无法处理
                if (!q.execute("update pay_list set pay_result='2',pay_endtime=now(),pay_reason='account not exist' where pay_id='" + LEX_CAST_STR(*it_orderno) + "'"))
                {
                    CHECK_DB_ERR(q);
                }
            }
        }
        else
        {
            q.free_result();
            //无法处理
            if (!q.execute("update pay_list set pay_result='2',pay_endtime=now(),pay_reason='account not exist' where pay_id='" + LEX_CAST_STR(*it_orderno) + "'"))
            {
                CHECK_DB_ERR(q);
            }
        }
    }
    return HC_SUCCESS;
#else
    {
        int account_id = 0;
        int iMoney = 0;
        //帐号是否存在
        q.get_result("select id,money from accounts where qid='" + GetDb().safestr(*it_qid)
            + "' and union_id='" + LEX_CAST_STR(*it_union_id)
            + "' and server_id='" + GetDb().safestr(*it_server_id) + "'");
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            account_id = q.getval();
            iMoney = q.getval();
            q.free_result();

            iMoney += *it_gold;
            if (!q.execute("update accounts set money=" + LEX_CAST_STR(iMoney) + " where id=" + LEX_CAST_STR(account_id)))
            {
                CHECK_DB_ERR(q);
            }
            else
            {
                //成功处理
                if (!q.execute("update pay_list set pay_result='1',pay_endtime=now() where pay_id='" + LEX_CAST_STR(*it_orderno) + "'"))
                {
                    CHECK_DB_ERR(q);
                }
            }
        }
        else
        {
            q.free_result();
            //无法处理
            if (!q.execute("update pay_list set pay_result='2',pay_endtime=now(),pay_reason='account not exist' where pay_id='" + LEX_CAST_STR(*it_orderno) + "'"))
            {
                CHECK_DB_ERR(q);
            }
        }
    }
    return HC_SUCCESS;
#endif
}

//充值
int ProcessRecharge(session_ptr& psession, json_spirit::mObject& o)
{
    //GET /?orderno=1321513362&qid=1111&union_id=10000&server_id=s1&ip=127.0.0.1&money=12&password=4E3E39F5C29FB79DC2E00E3C90BA2824 HTTP/1.1
    //cout<<"************** ProcessRecharge() *************"<<endl;
    std::string orderNo = "", qid = "", union_id = "", server_id = "", ip = "", money = "", password = "";
    READ_STR_FROM_MOBJ(orderNo,o,"orderno");
    READ_STR_FROM_MOBJ(qid,o,"qid");
    READ_STR_FROM_MOBJ(union_id,o,"union_id");
    READ_STR_FROM_MOBJ(server_id,o,"server_id");
    READ_STR_FROM_MOBJ(ip,o,"ip");
    READ_STR_FROM_MOBJ(money,o,"money");
    READ_STR_FROM_MOBJ(password,o,"password");

    //验证密钥是否正确
    //生成验证码
    std::string strUpdate = orderNo + qid + union_id + server_id + ip + money + getRechargeKey(atoi(union_id.c_str()));
    CMD5 md5;
    unsigned char res[16];
    md5.MessageDigest((const unsigned char *)strUpdate.c_str(), strUpdate.length(), res);
    std::string finalSign = hexstr(res,16);

    //cout<<" final sign = "<<finalSign<<endl;
    //比较验证码
    if (finalSign != password)
    {
        psession->send(szChargeResponseAuthFail, false);
        psession->closeconnect();
        //cout<<" ****** auth fail! ****** "<<endl;
        //cout<<"["<<password<<"]"<<endl;
        return HC_ERROR_WRONG_PASSWORD;
    }

    Query q(GetDb());
    int account_id = 0;
    int iMoney = 0;
    //cout<<"qid:"<<qid<<endl;
    //cout<<"union_id:"<<union_id<<endl;
    //帐号是否存在
    q.get_result("select id,money from accounts where qid='" + GetDb().safestr(qid) + "' and union_id='" + GetDb().safestr(union_id) + "'");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        account_id = q.getval();
        iMoney = q.getval();
        q.free_result();
    }
    else
    {
        q.free_result();
        psession->send(szChargeResponseNotFound, false);
        psession->closeconnect();
        //cout<<" ****** account not exist! ****** "<<endl;
        return HC_ERROR_WRONG_ACCOUNT;
    }
    int addMoney = atoi(money.c_str());
    iMoney += addMoney;
    if (!q.execute("update accounts set money=" + LEX_CAST_STR(iMoney) + " where id=" + LEX_CAST_STR(account_id)))
    {
        CHECK_DB_ERR(q);
    }
    else
    {
        if (!q.execute("insert into pay_list (pay_orderno,qid,union_id,server_id,pay_ip,pay_time,pay_net_money) values ('"
                + GetDb().safestr(orderNo) + "','" + GetDb().safestr(qid) + "','" + GetDb().safestr(union_id)
                + "','" + GetDb().safestr(server_id) + "','" + GetDb().safestr(ip) + "',now()," + LEX_CAST_STR(money) + ")"))
        {
            CHECK_DB_ERR(q);
            if (!q.execute("update accounts set money=" + LEX_CAST_STR(iMoney - addMoney) + " where id='" + LEX_CAST_STR(account_id)))
            {
                CHECK_DB_ERR(q);
            }
            psession->send(szChargeResponseServerError, false);
            psession->closeconnect();
            return HC_ERROR;
        }
    }
    psession->send(szChargeResponseSuccess, false);
    psession->closeconnect();
    //cout<<" ****** ProcessRecharge success ****** "<<endl;
    return HC_SUCCESS;
}

//测试用户验证
int AuthAccount(const std::string& account, const std::string& password)
{
    Query q(GetDb());
    q.get_result("select password from temp_test_accounts where account='" + GetDb().safestr(account) + "'");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        if (q.getstr() == password)
        {
            q.free_result();
            return HC_SUCCESS;
        }
        else
        {
            q.free_result();
            return HC_ERROR_WRONG_PASSWORD;
        }
    }
    else
    {
        //现有账户可直接登录
        q.free_result();
        q.get_result("select count(*) from charactors where account='" + GetDb().safestr(account) + "'");
        CHECK_DB_ERR(q);
        if (q.fetch_row() && q.getval() > 0)
        {
            q.free_result();
            return HC_SUCCESS;
        }
        q.free_result();
        return HC_ERROR_WRONG_PASSWORD;
    }
}

//测试用户防沉迷测试
int getTestAccountChenmiState(const std::string& account)
{
    Query q(GetDb());
    q.get_result("select isAdult from temp_test_chenmi where account='" + GetDb().safestr(account) + "'");
    if (q.fetch_row())
    {
        int isAdult = q.getval();
        q.free_result();
        return isAdult;
    }
    else
    {
        q.free_result();
        return 0;
    }
}

int authAccount(const std::string& qid,
                const std::string& qname,
                int union_id,
                const std::string& server_id,
                std::string& account)
{
    Query q(GetDb());
    q.get_result("select account from accounts where qid='" + GetDb().safestr(qid)
            + "' and union_id=" + LEX_CAST_STR(union_id)
            + " and server_id='" + GetDb().safestr(server_id) + "'");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        account = q.getstr();
        q.free_result();

        //检查帐号是否被冻结
        q.get_result("select count(*) from admin_scoll where endtime>unix_timestamp() and account='" + account + "'");
        CHECK_DB_ERR(q);
        if (q.fetch_row() && q.getval() > 0)
        {
            q.free_result();
            return HC_ERROR_ACCOUNT_BE_FREEZED;
        }
        else
        {
            q.free_result();
        }
        return HC_SUCCESS;
    }
    else
    {
        int retry = 10;
        //现有账户可直接登录
        q.free_result();
        do
        {
            --retry;
             q.execute("insert into accounts (account,qid,qname,union_id,server_id,createTime) values (uuid(),'" + GetDb().safestr(qid) + "','"
                         + GetDb().safestr(qname) + "'," + LEX_CAST_STR(union_id) + ",'"  + GetDb().safestr(server_id) + "',unix_timestamp())");
        }
        while (q.GetErrno() == 1062 && retry);
        if (q.GetErrno())
        {
            return HC_ERROR;
        }
        else
        {
            q.get_result("select account from accounts where qid='" + GetDb().safestr(qid)
                            + "' and union_id=" + LEX_CAST_STR(union_id)
                            + " and server_id='" + GetDb().safestr(server_id) + "'");
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                account = q.getstr();
                q.free_result();
                return HC_SUCCESS;
            }
            else
            {
                q.free_result();
                return HC_ERROR;
            }
        }
    }
}

//平台用户验证
int platformAuthAccount(const std::string& qid,
                          const std::string& qname,
                          int union_id,
                          const std::string& server_id,
                          time_t _time,
                          int iAdult,
                          const std::string& extra1,
                          const std::string& extra2,
                          const std::string& sign,
                          std::string& account)
{
    account = "";
    //有效时间5分钟
    if (time(NULL) - _time > 300)
    {
        INFO(" ************** time now = "<<time(NULL)<<", _time = "<<_time<<", "<<(time(NULL) - _time)<<endl);
        return HC_ERROR_AUTH_EXPIRED;
    }

    //生成验证码
    if (extra1 == "reload" && extra2 == "reload")
    {
        getAuthKey(-1);
    }

    std::string strUpdate = "qid=" + qid + "&qname=" + qname + "&union_id=" + LEX_CAST_STR(union_id)
                    + "&time=" + LEX_CAST_STR(_time) + "&server_id=" + server_id + "&extra1=" + extra1
                    + "&extra2=" + extra2 + getAuthKey(union_id);

    CMD5 md5;
    unsigned char res[16];
    md5.MessageDigest((const unsigned char *)strUpdate.c_str(), strUpdate.length(), res);

    INFO(strUpdate<<endl);

    std::string finalSign = hexstr(res,16);

    INFO(" auth time "<<_time<<endl);
    INFO(" final sign = "<<finalSign<<endl);
    //比较验证码
    if (finalSign != sign)
    {
        return HC_ERROR_WRONG_PASSWORD;
    }

    return authAccount(qid, qname, union_id, server_id, account);
}

int ticketAuth( const std::string& ip,
                const std::string& ticket, std::string& qid,
                std::string& qname,
                  int& union_id,
                  std::string& server_id,
                  int& iAdult,
                  std::string& extra1,
                  std::string& extra2,
                  std::string& account)
{
    Query q(GetDb());
    q.get_result("select union_id,server_id,qid,qname,isAdult,extra1,extra2 from api_login where ip='" + ip + "' and `sign`='" + GetDb().safestr(ticket) + "' and (unix_timestamp(input)+300)>unix_timestamp()");
    if (q.fetch_row())
    {
        union_id = q.getval();
        server_id = q.getstr();
        qid = q.getstr();
        qname = q.getstr();
        iAdult = q.getval();
        extra1 = q.getstr();
        extra2 = q.getstr();

        q.free_result();

        return authAccount(qid, qname, union_id, server_id, account);
    }
    else
    {
        q.free_result();
        return HC_ERROR_WRONG_PASSWORD;
    }
}

void checkCustomSchedule(struct tm& tm_now)
{
    boost::shared_ptr<DateInfo> d = GeneralDataMgr::getInstance()->GetDate();
    if (d.get())
    {
        //cout<<"checkCustomSchedule():season"<<d->season<<",week:"<<tm_now.tm_wday<<",mon:"<<tm_now.tm_mon<<",day:"    <<tm_now.tm_mday<<",hour:"<<tm_now.tm_hour<<",min:"<<tm_now.tm_min<<endl;
        Query q(GetDb());
        std::string sql = "select type,param1,param2 from custom_shedule where (minute='*' or minute='"
                + LEX_CAST_STR(tm_now.tm_min) + "') and  (hour='*' or hour='"
                + LEX_CAST_STR(tm_now.tm_hour) + "') and (day='*' or day='"
                + LEX_CAST_STR(tm_now.tm_mday) + "') and (month='*' or month='"
                + LEX_CAST_STR(tm_now.tm_mon) + "') and (week='*' or week='"
                + LEX_CAST_STR(tm_now.tm_wday) + "') and (season='*' or season='"
                + LEX_CAST_STR(d->season) + "')";
        q.get_result(sql);
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            std::string event = q.getstr();
            int param1 = q.getval();
            int param2 = q.getval();
            json_spirit::mObject obj;
            obj["cmd"] = "scheduleEvent";
            obj["event"] = event;
            obj["param1"] = param1;
            obj["param2"] = param2;
            //cout<<"insert action message:cmd:scheduleEvent,event:"<<event<<",param1:"<<param1<<",param2:"<<param2<<endl;
            if (0 != InsertInternalActionWork(obj))
            {
                ERR();
            }
        }
        q.free_result();
    }
}

std::string getAuthKey(int union_id)
{
    static std::map<int, std::string> keymap;
    if (-1 == union_id)
    {
        keymap.clear();
    }
    std::map<int, std::string>::iterator it = keymap.find(union_id);
    if (it == keymap.end())
    {
        Query q(GetDb());
        q.get_result("select `key` from admin_keys where union_id='" + LEX_CAST_STR(union_id) + "'");
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            keymap[union_id] = q.getstr();
        }
        else
        {
            keymap[union_id] = "123456789";
        }
        q.free_result();
    }
    return keymap[union_id];
}

std::string getRechargeKey(int union_id)
{
    static std::map<int, std::string> keymap;
    std::map<int, std::string>::iterator it = keymap.find(union_id);
    if (it == keymap.end())
    {
        Query q(GetDb());
        q.get_result("select `key` from admin_keys where union_id=" + LEX_CAST_STR(union_id));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            keymap[union_id] = q.getstr();
        }
        else
        {
            keymap[union_id] = "123456789";
        }
        q.free_result();
    }
    return keymap[union_id];
}

//新增日常公告
int ProcessNewAdminNotice(json_spirit::mObject& o)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    std::string msg = "";
    READ_STR_FROM_MOBJ(msg,o,"msg");
    return GeneralDataMgr::getInstance()->adminNoticeNew(id, msg);
}

//删除日常公告
int ProcessDeleteAdminNotice(json_spirit::mObject& o)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    return GeneralDataMgr::getInstance()->adminNoticeDeleted(id);
}

//修改日常公告
int ProcessChangeAdminNotice(json_spirit::mObject& o)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    std::string msg = "";
    READ_STR_FROM_MOBJ(msg,o,"msg");
    return GeneralDataMgr::getInstance()->adminNoticeChanged(id, msg);
}

//广播日常公告
int ProcessSendAdminNotice(json_spirit::mObject& o)
{
    std::string msg = "";
    READ_STR_FROM_MOBJ(msg,o,"msg");
    int type = -1;
    READ_INT_FROM_MOBJ(type,o,"type");
    //临时公告用红色字体
    if (2 == type)
    {
        type = -1;
        msg = "<font color=\"#FF0000\">" + msg + "</font>";
    }
    else if (3 == type)
    {
        type = 1;
    }
    return GeneralDataMgr::getInstance()->broadCastSysMsg(msg, type);
}

//查询日常公告
int ProcessQueryAdminNotice(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array nlist;
    GeneralDataMgr::getInstance()->getAdminNotice(nlist);
    robj.push_back( Pair("list", nlist) );
    return HC_SUCCESS;
}

//关闭处理
int ProcessShutdown(json_spirit::mObject& o)
{
    GeneralDataMgr::getInstance()->shutdown();
    return HC_SUCCESS;
}

//英雄镶嵌宝石总属性
int ProcessGeneralBaoshiInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid, o, "id");
    int bcid = GeneralDataMgr::getInstance()->getGeneralOwner(gid);
    if (bcid == pc->m_id)
    {
        return pc->queryGeneralBaoshiInfo(gid, robj);
    }
    else if (bcid > 0)
    {
        //cout<<"general cid :"<<bcid<<endl;
        CharData* ppc = GeneralDataMgr::getInstance()->GetCharData(bcid).get();
        if (ppc)
        {
            //cout<<"find char!"<<endl;
            return ppc->queryGeneralBaoshiInfo(gid, robj);
        }
    }
    return HC_ERROR;
}

//可鑲嵌武將列表
int ProcessQueryBaoshiGenerals(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (!pc->m_baowuOpen)
    {
        return HC_ERROR;
    }
    json_spirit::Array glist;
    pc->queryBaoshiGeneralList(glist);
    robj.push_back( Pair("list", glist) );
    return HC_SUCCESS;
}

//武將寶石列表
int ProcessQueryGeneralBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid, o, "id");

    robj.push_back( Pair("id", gid) );

    int bcid = GeneralDataMgr::getInstance()->getGeneralOwner(gid);
    if (bcid == pc->m_id)
    {
        return pc->queryGeneralBaoshi(gid, robj);
    }
    else if (bcid > 0)
    {
        //cout<<"general cid :"<<bcid<<endl;
        CharData* ppc = GeneralDataMgr::getInstance()->GetCharData(bcid).get();
        if (ppc)
        {
            //cout<<"find char!"<<endl;
            return ppc->queryGeneralBaoshi(gid, robj);
        }
    }
    return HC_ERROR;
}

//兑换宝石
int ProcessExchangeBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (!pc->m_baowuOpen)
    {
        return HC_ERROR;
    }
    //int btype = 1;
    //READ_INT_FROM_MOBJ(btype,o,"btype");
    int nums = 1, auto_buy = 0, real_nums = 0;
    READ_INT_FROM_MOBJ(nums,o,"nums");
    READ_INT_FROM_MOBJ(auto_buy,o,"auto_buy");
    std::map<int,int> baoshi_get;
    for (int i = 1; i <= nums; ++i)
    {
        int type = 0;
        ret = Singleton<newBaoshiMgr>::Instance().buyBaoshi(pc, type, robj, 1, auto_buy);
        if (ret == HC_SUCCESS && type != 0)
        {
            int tmp_count = baoshi_get[type];
            baoshi_get[type] = ++tmp_count;
            ++real_nums;
        }
        if (ret != HC_SUCCESS)
        {
            if (i > 1)
            {
                ret = HC_SUCCESS;
            }
            break;
        }
    }
    if (ret == HC_SUCCESS)
    {
        json_spirit::Array list;
        for (int type = 1; type <= 14; ++type)
        {
            if (baoshi_get[type] > 0)
            {
                baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(type);
                if (bbs)
                {
                    json_spirit::Object o;
                    o.push_back( Pair("name", bbs->name) );
                    o.push_back( Pair("count", baoshi_get[type]) );
                    list.push_back(o);
                }
            }
        }
        robj.push_back( Pair("get_list", list) );
        //日常任务
        dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_baoshi_exchange,real_nums);
        //act统计
        act_to_tencent(pc,act_new_baoshi_by_yushi,real_nums);

        //支线任务
        pc->m_trunk_tasks.updateTask(task_baoshi_exchange, real_nums);
    }
    robj.push_back( Pair("yushi", pc->treasureCount(treasure_type_yushi)) );
    robj.push_back( Pair("silver", pc->silver()) );
    return ret;
}

//兑换玉石
int ProcessExchangeYushi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (!pc->m_baowuOpen)
    {
        return HC_ERROR;
    }

    int gold = 0;
    READ_INT_FROM_MOBJ(gold,o,"gold");
    if (gold < 0)
    {
        return HC_ERROR;
    }
    if (gold == 0)
    {
        robj.push_back( Pair("yushi", 150) );
        return HC_SUCCESS;
    }
    else if (pc->addGold(-gold) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    else
    {
        //金币消耗统计
        add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, gold, gold_cost_for_convert_jade, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(pc,0.007,gold_cost_for_convert_jade,treasure_type_yushi,150*gold);
#endif
        pc->addTreasure(treasure_type_yushi, 150*gold);
        add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_yushi,150*gold,treasure_buy,1,pc->m_union_id,pc->m_server_id);
        pc->NotifyCharData();
        //robj.push_back( Pair("yushi", 150*gold) );
        return HC_SUCCESS;
    }
}

//查询兑换宝石
int ProcessQueryExchangeBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (!pc->m_baowuOpen)
    {
        return HC_ERROR;
    }

    json_spirit::Array exlist;
    json_spirit::Object obj;
    {
        obj.push_back( Pair("type", 1) );
        obj.push_back( Pair("name", "baoshi") );
        obj.push_back( Pair("quality", 1) );
        obj.push_back( Pair("cost", 1800) );
        obj.push_back( Pair("silver_cost", 10000) );
        exlist.push_back(obj);
    }
    robj.push_back( Pair("list", exlist) );
    robj.push_back( Pair("yushi", LEX_CAST_STR(pc->treasureCount(treasure_type_yushi))) );
    robj.push_back( Pair("silver", LEX_CAST_STR(pc->silver())) );
    return HC_SUCCESS;
}

#if 0
//查询宝石升级信息
int ProcessQueryBaoshiLevelupInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return pc->levelupBaoshiInfo(id, robj);
}
#endif

//查询道具数量
int ProcessQueryTreasure(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("nums", pc->treasureCount(id)) );
    return HC_SUCCESS;
}

//查询道具价格
int ProcessQueryTreasurePrice(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("id", id) );
    boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(id);
    if (!bt.get())
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("sell_price", bt->sellPrice) );
    robj.push_back( Pair("need_gold", bt->gold_to_buy) );
    return HC_SUCCESS;
}

