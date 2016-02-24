
#include "stdafx.h"

#include "data.h"
#include <iostream>
#include "utils_all.h"

#ifndef _WINDOWS
#include <sys/syscall.h>
#endif

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>
#include "errcode_def.h"
#include "utils_lang.h"

#include "lang_def.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_timer.h"
#include "net.h"
#include "md5.h"
#include "db_thread.h"
#include <pthread.h>
#include "singleton.h"

#include "json_spirit_writer_template.h"

#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "SaveDb.h"
#include "city.h"
#include "libao.h"
#include "explore.h"
#include "relation.h"
#include "mails.h"
#include "action.h"
#include "pk.h"
#include "treasure.h"
#include "recharge_event.h"
#include "rewards.h"
#include "findBack.h"
#include "boss.h"
#include "bank.h"
#include "lottery_event.hpp"
#include "weekRanking.h"

#define INFO(x) cout<<x<<endl

using namespace std;
using namespace net;

boost::unordered_map<uint64_t, Database*> g_dbs;

extern pthread_key_t thread_db_key;
extern void InsertMailcmd(mailCmd&);
extern void InsertSaveDb(const std::string& sql);
extern void InsertSaveDb(const saveDbJob& job);

extern int InsertActionWork(actionmessage& msg);
extern int InsertInternalActionWork(json_spirit::mObject& obj);

extern void InsertDbCharCmd(dbCmd& _dbCmd);

//系统消息的格式
const std::string strSystemMsgFormat = "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_broad)+",\"s\":200,\"m\":\"$M\",\"type\":$T}";

const std::string strGuidemsg = "{\"cmd\":\"currentGuide\",\"id\":$D,\"state\":$S,\"s\":200}";

const std::string strNotifyPresentGet = "{\"cmd\":\"present_get\",\"s\":200}";

#define DEFAULT_GOLD 15
#define DEFAULT_SILVER 100000
static volatile int g_default_gold = DEFAULT_GOLD;
static volatile int g_default_silver = DEFAULT_SILVER;


volatile int g_auth_type = 0;    // 0:md5 , 1:ticket

extern volatile int g_enable_debug_cmd_line;

volatile int g_get_char_id = 0;

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

//外部程序将事件插入到 scheduleEvent表中，来触发一些处理
int checkScheduleEvent()
{
    int max_id = 0;
    Query q(GetDb());
    q.get_result("select id,event,param1,param2,param3,param4,extra,inputTime from schedule_event where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        max_id = q.getval();
        json_spirit::mObject obj;
        obj["cmd"] = "scheduleEvent";
        std::string event = q.getstr();
        obj["event"] = event;
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
    q.free_result();
    q.execute("delete from schedule_event where id<=" + LEX_CAST_STR(max_id));
    CHECK_DB_ERR(q);
    return HC_SUCCESS;
}

void checkCustomSchedule(struct tm& tm_now)
{
    //cout<<"checkCustomSchedule():week:"<<tm_now.tm_wday<<",mon:"<<tm_now.tm_mon<<",day:"    <<tm_now.tm_mday<<",hour:"<<tm_now.tm_hour<<",min:"<<tm_now.tm_min<<endl;
    Query q(GetDb());
    std::string sql = "select type,param1,param2 from custom_shedule where (minute='*' or minute='"
            + LEX_CAST_STR(tm_now.tm_min) + "') and  (hour='*' or hour='"
            + LEX_CAST_STR(tm_now.tm_hour) + "') and (day='*' or day='"
            + LEX_CAST_STR(tm_now.tm_mday) + "') and (month='*' or month='"
            + LEX_CAST_STR(tm_now.tm_mon) + "') and (week='*' or week='"
            + LEX_CAST_STR(tm_now.tm_wday) + "')";
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

int get_statistics_type(int reward_type, int reward_id, int loot_type)
{
    if (loot_type <= 0)
    {
        return 0;
    }
    switch(reward_type)
    {
        case ITEM_TYPE_CURRENCY:
            {
                if (reward_id == CURRENCY_ID_GOLD || reward_id == CURRENCY_ID_BIND_GOLD || reward_id == CURRENCY_ID_BIND_GOLD_LEVEL)
                {
                    switch(loot_type)
                    {
                        case loot_limit_action:
                            return gold_get_limit_action;
                            break;
                        case loot_arena_rank:
                            return gold_get_arena_rank;
                            break;
                        case loot_daily_score:
                            return gold_get_daily_score;
                            break;
                        case loot_qq_yellow:
                            return gold_get_qq_yellow;
                            break;
                        case loot_pk_rank:
                            return gold_get_pk_rank;
                            break;
                        case loot_sign_action:
                            return gold_get_sign_action;
                            break;
                        case loot_prestige_shop:
                            return gold_get_prestige_shop;
                            break;
                        case loot_lottery:
                            return gold_get_lottery;
                            break;
                        case loot_auction:
                            return gold_get_auction;
                            break;
                    }
                }
                else if(reward_id == CURRENCY_ID_SILVER || reward_id == CURRENCY_ID_SILVER_LEVEL)
                {
                    switch(loot_type)
                    {
                        case loot_limit_action:
                            return silver_get_limit_action;
                            break;
                        case loot_sign_action:
                            return silver_get_sign_action;
                            break;
                        case loot_treasure:
                            return silver_get_treasure;
                            break;
                        case loot_recharge_event:
                            return silver_get_recharge_event;
                            break;
                        case loot_libao:
                            return silver_get_libao;
                            break;
                        case loot_online_action:
                            return silver_get_online_action;
                            break;
                        case loot_shenling:
                            return silver_get_shenling;
                            break;
                        case loot_explore:
                            return silver_get_explore;
                            break;
                        case loot_arena_rank:
                            return silver_get_arena_rank;
                            break;
                        case loot_copy:
                            return silver_get_copy;
                            break;
                        case loot_stronghold:
                            return silver_get_stronghold;
                            break;
                        case loot_task:
                            return silver_get_task;
                            break;
                        case loot_daily_task:
                            return silver_get_daily_task;
                            break;
                        case loot_prestige_shop:
                            return silver_get_prestige_shop;
                            break;
                        case loot_qq_yellow:
                            return silver_get_qq_yellow;
                            break;
                        case loot_daily_score:
                            return silver_get_daily_score;
                            break;
                        case loot_guild_box:
                            return silver_get_guild_box;
                            break;
                        case loot_stage:
                            return silver_get_stage;
                            break;
                        case loot_find_back:
                            return silver_get_find_back;
                            break;
                        case loot_wild:
                            return silver_get_wild;
                            break;
                        case loot_copy_finish:
                            return silver_get_copy_finish;
                            break;
                        case loot_guild_moshen:
                            return silver_get_guild_moshen;
                            break;
                        case loot_goal:
                            return silver_get_goal;
                            break;
                        case loot_boss:
                            return silver_get_boss;
                            break;
                        case loot_auction:
                            return silver_get_auction;
                            break;
                        case loot_weekRanking:
                            return silver_get_weekRanking;
                            break;
                    }
                }
                else if(reward_id == CURRENCY_ID_CHAR_EXP || reward_id == CURRENCY_ID_CHAR_EXP_LEVEL)
                {
                    switch(loot_type)
                    {
                        case loot_task:
                            return char_exp_get_task;
                            break;
                        case loot_daily_task:
                            return char_exp_get_daily_task;
                            break;
                        case loot_stage:
                            return char_exp_get_stage;
                            break;
                        case loot_find_back:
                            return char_exp_get_find_back;
                            break;
                    }
                }
                else if((reward_id > CURRENCY_ID_PRESTIGE_BEGIN && reward_id < CURRENCY_ID_PRESTIGE_END) || reward_id == CURRENCY_ID_PRESTIGE_RANDOM)
                {
                    switch(loot_type)
                    {
                        case loot_stronghold:
                            return prestige_get_stronghold;
                            break;
                        case loot_prestige_task:
                            return prestige_get_prestige_task;
                            break;
                        case loot_weekRanking:
                            return prestige_get_weekRanking;
                            break;
                    }
                }
                else if(reward_id == CURRENCY_ID_EXP)
                {
                    switch(loot_type)
                    {
                        case loot_stronghold:
                            return hero_exp_get_stronghold;
                            break;
                        case loot_explore:
                            return hero_exp_get_explore;
                            break;
                    }
                }
            }
            break;
        case ITEM_TYPE_GEM:
            {
                switch(loot_type)
                {
                    case loot_sign_action:
                        return gem_get_sign_action;
                        break;
                    case loot_limit_action:
                        return gem_get_limit_action;
                        break;
                    case loot_arena:
                        return gem_get_arena;
                        break;
                    case loot_arena_shop:
                        return gem_get_arena_shop;
                        break;
                    case loot_copy:
                        return gem_get_copy;
                        break;
                    case loot_copy_shop:
                        return gem_get_copy_shop;
                        break;
                    case loot_daily_score:
                        return gem_get_daily_score;
                        break;
                    case loot_prestige_shop:
                        return gem_get_prestige_shop;
                        break;
                    case loot_guild_box:
                        return gem_get_guild_box;
                        break;
                    case loot_libao:
                        return gem_get_libao;
                        break;
                    case loot_treasure:
                        return gem_get_treasure;
                        break;
                    case loot_stronghold:
                        return gem_get_stronghold;
                        break;
                    case loot_recharge_event:
                        return gem_get_recharge_event;
                        break;
                    case loot_shenling:
                        return gem_get_shenling;
                        break;
                    case loot_shenling_shop:
                        return gem_get_shenling_shop;
                        break;
                    case loot_task:
                        return gem_get_task;
                        break;
                    case loot_pk_rank:
                        return gem_get_pk_rank;
                        break;
                    case loot_qq_yellow:
                        return gem_get_qq_yellow;
                        break;
                    case loot_online_action:
                        return gem_get_online_action;
                        break;
                    case loot_copy_finish:
                        return gem_get_copy_finish;
                        break;
                    case loot_daily_task:
                        return gem_get_daily_task;
                        break;
                    case loot_guild_moshen:
                        return gem_get_guild_moshen;
                        break;
                    case loot_goal:
                        return gem_get_goal;
                        break;
                    case loot_goal_shop:
                        return gem_get_goal_shop;
                        break;
                    case loot_boss:
                        return gem_get_boss;
                        break;
                    case loot_lottery:
                        return gem_get_lottery;
                        break;
                    case loot_present:
                        return gem_get_present;
                        break;
                    case loot_auction:
                        return gem_get_auction;
                        break;
                    case loot_weekRanking:
                        return gem_get_weekRanking;
                        break;
                }
            }
            break;
        case ITEM_TYPE_HERO:
            {
                switch(loot_type)
                {
                    case loot_stronghold:
                        return hero_get_stronghold;
                        break;
                    case loot_auction:
                        return hero_get_auction;
                        break;
                }
            }
            break;
    }
    return 9999;
}

int giveLoots(CharData* cdata, std::list<Item>& getItems, chessCombat* pCombat, json_spirit::Object* robj, bool isAttacker, int loot_type)
{
    int total_get = 0;
    json_spirit::Array getlist;
    std::list<Item>::iterator it = getItems.begin();
    std::string extraMsg = "";
    while (it != getItems.end())
    {
        if (it->nums > 0)
        {
            int statistics_type = get_statistics_type(it->type,it->id,loot_type);
            json_spirit::Object getobj;
            switch (it->type)
            {
                case ITEM_TYPE_CURRENCY://货币类
                    {
                        if (it->id == CURRENCY_ID_GOLD)
                        {
                            cdata->addGold(it->nums, statistics_type, true);
                        }
                        else if(it->id == CURRENCY_ID_SILVER)
                        {
                            cdata->addSilver(it->nums, statistics_type);
                        }
                        else if(it->id == CURRENCY_ID_SILVER_LEVEL)
                        {
                            it->id = CURRENCY_ID_SILVER;
                            it->nums = it->nums * cdata->m_level;
                            cdata->addSilver(it->nums, statistics_type);
                        }
                        else if(it->id == CURRENCY_ID_EXP)
                        {
                            boost::shared_ptr<CharHeroData> p_hero = cdata->m_heros.GetHero(cdata->m_heros.m_default_hero);
                            if (p_hero.get() && p_hero->m_baseHero.get())
                            {
                                p_hero->addExp(it->nums, statistics_type);
                            }
                        }
                        else if(it->id > CURRENCY_ID_PRESTIGE_BEGIN && it->id < CURRENCY_ID_PRESTIGE_END)
                        {
                            int type = it->id - CURRENCY_ID_PRESTIGE_BEGIN;
                            cdata->addPrestige(type, it->nums, statistics_type);
                        }
                        else if(it->id == CURRENCY_ID_PRESTIGE_RANDOM)
                        {
                            int type = my_random(1,4);
                            cdata->addPrestige(type, it->nums, statistics_type);
                        }
                        else if(it->id == CURRENCY_ID_CHAR_EXP)
                        {
                            cdata->addCharExp(it->nums, statistics_type);
                        }
                        else if(it->id == CURRENCY_ID_CHAR_EXP_LEVEL)
                        {
                            it->id = CURRENCY_ID_CHAR_EXP;
                            it->nums = it->nums * cdata->m_level;
                            cdata->addCharExp(it->nums, statistics_type);
                        }
                        else if(it->id == CURRENCY_ID_ARENA_SCORE)
                        {
                            int score = cdata->queryExtraData(char_data_type_daily, char_data_daily_arena_score);
                            cdata->setExtraData(char_data_type_daily, char_data_daily_arena_score, (score+it->nums));
                        }
                        else if(it->id == CURRENCY_ID_BIND_GOLD)
                        {
                            cdata->addGold(it->nums, statistics_type);
                        }
                        else if(it->id == CURRENCY_ID_BIND_GOLD_LEVEL)
                        {
                            it->id = CURRENCY_ID_BIND_GOLD;
                            it->nums = it->nums * cdata->m_level;
                            cdata->addGold(it->nums, statistics_type);
                        }
                        ++total_get;
                    }
                    break;
                case ITEM_TYPE_GEM://道具类
                    {
                        boost::shared_ptr<baseGem> tr = GeneralDataMgr::getInstance()->GetBaseGem(it->id);
                        if (tr.get())
                        {
                            ++total_get;
                            cdata->addGem(it->id, it->nums, statistics_type);
                        }
                        else
                        {
                            ERR();
                        }
                    }
                    break;
                case ITEM_TYPE_EQUIPMENT://装备类
                    {
                        for (int i = 0; i < it->nums; ++i)
                        {
                            int level = it->extra2 > 0 ? it->extra2 : 1;
                            int equip_id = cdata->addEquipt(it->id, level, it->extra);
                            ++total_get;
                        }
                    }
                    break;
                case ITEM_TYPE_HERO://英雄
                    {
                        for (int i = 0; i < it->nums; ++i)
                        {
                            int hero_star = it->extra > 0 ? it->extra : 1;
                            int level = it->extra2 > 0 ? it->extra2 : 1;
                            double add1 = it->d_extra[0];
                            double add2 = it->d_extra[1];
                            double add3 = it->d_extra[2];
                            double add4 = it->d_extra[3];
                            if (cdata->m_heros.Add(it->id, level, hero_star, add1 > 0.1, add1, add2, add3, add4) > 0)
                            {
                                ++total_get;
                                statistics_of_hero_get(cdata->m_id,cdata->m_ip_address,it->id,1,statistics_type,cdata->m_union_id,cdata->m_server_id);
                            }
                        }
                    }
                    break;
                case ITEM_TYPE_LIBAO:
                    {
                        cdata->addLibao(it->id, it->nums);
                        ++total_get;
                    }
                    break;
                case ITEM_TYPE_BAOSHI:
                    {
                        if (it->extra <= 0)
                            it->extra = 1;
                        cdata->addBaoshi(it->id,it->extra,it->nums);
                        ++total_get;
                    }
                    break;
            }
            it->toObj(getobj);
            getlist.push_back(getobj);
        }
        ++it;
    }
    if (pCombat)
    {
        if (isAttacker)
        {
            pCombat->m_result_obj.push_back( Pair("get", getlist) );
            if (extraMsg != "")
            {
                pCombat->m_result_obj.push_back( Pair("msg", extraMsg) );
            }
        }
        else
        {
            pCombat->m_result_obj.push_back( Pair("get2", getlist) );
            if (extraMsg != "")
            {
                pCombat->m_result_obj.push_back( Pair("msg2", extraMsg) );
            }
        }
    }
    else if (robj)
    {
        if (isAttacker)
        {
            robj->push_back( Pair("get", getlist) );
            if (extraMsg != "")
            {
                robj->push_back( Pair("msg", extraMsg) );
            }
        }
        else
        {
            robj->push_back( Pair("get2", getlist) );
            if (extraMsg != "")
            {
                robj->push_back( Pair("msg2", extraMsg) );
            }
        }
    }
    return total_get;
}

int giveLoots(boost::shared_ptr<CharData>& cdata, chessCombat* pCombat, bool isAttacker, int loot_type)
{
    if (isAttacker)
    {
        return giveLoots(cdata.get(), pCombat->m_getItems, pCombat, NULL, isAttacker, loot_type);
    }
    else
    {
        return giveLoots(cdata.get(), pCombat->m_getItems2, pCombat, NULL, isAttacker, loot_type);
    }
}

int baseRaceData::getLevelAdd(int level, int& attack_add, int& defense_add, int& magic_add)
{
    attack_add = 0, defense_add = 0, magic_add = 0;
    if (level > 0 && level <= iMaxLevel)
    {
        attack_add = m_attak_add[level-1];
        defense_add = m_defense_add[level-1];
        magic_add = m_magic_add[level-1];
        return 1;
    }
    return 0;
}

void CharPrestigeAward::save()
{
    const json_spirit::Value val_curs(m_state.begin(), m_state.end());
    InsertSaveDb("replace into char_prestige_award (cid,race,state) values ("
                    + LEX_CAST_STR(m_cid) + "," + LEX_CAST_STR(m_race)
                    + ",'" + json_spirit::write(val_curs)
                    + "')");
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

volatile uint64_t CharData::_refs = 0;

CharData::CharData(int cid, bool b_create)
:m_tempo(*this)
,m_heros(cid, *this)
,m_skills(cid, *this)
,m_bag(*this, BAG_DEFAULT_SIZE)
,m_wild_citys(*this)
,m_tasks(*this)
,m_score_tasks(*this)
,m_prestige_tasks(*this)
,m_Buffs(*this)
,m_magics(cid, *this)
{
    //cout<<"CharData::CharData()"<<cid<<",tid:"<<syscall(SYS_gettid)<<endl;
    m_is_online = 0;
    ++CharData::_refs;
    m_load_success = false;
    m_save_time = time(NULL);
    m_id = cid;
    m_race = 1;    //角色种族
    m_cur_mapid = 1;    //角色所在地区
    m_cur_stageid = 1;
    m_cur_strongholdid = 0;
    m_login_time = 0;         //登录时间

    m_vip = 0;                 //vip等级
    m_vip_exp = 0;
    m_can_world_chat = 0;     //是否可以世界聊天
    m_can_chat = true;         //是否被禁言
    m_char_data_change = false;
    m_account = "";
    m_name = "";
    m_chat = "";

    m_ip_address = "";

    m_total_attack = 0;
    m_total_defense = 0;
    m_total_hp = 0;

    m_gold = 0;
    m_bind_gold = 0;
    m_silver = 0;
    m_bank_silver = 0;

    m_silver_get_combat = 0;
    m_double_times = 0;

#ifdef QQ_PLAT
    m_qq_yellow_level = 0;
    m_qq_yellow_year = 0;
    m_iopenid = "";
    m_feedid = "";
    m_login_str1 = "";
    m_login_str2 = "";
#endif

    memset(m_gold_cost_comfirm, 0, sizeof(int)*iMaxGoldCostConfirm);

    if(b_create)
    {
        Create();
    }
    else
    {
        Load();
    }
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

time_t CharData::queryCreateXDays(int day)
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
    return tmp_create_time + day * DAY;
}

//检查是否触发引导
int CharData::checkGuide(int id)
{
    INFO("********************* CharData::checkGuide:"<<id<<endl);
    if (m_guide_completes[id] == 0)
    {
        //保存当前引导id
        m_current_guide = id;
        setExtraData(char_data_type_normal, char_data_normal_current_guide, m_current_guide);

        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
        if (account.get())
        {
            std::string msg = strGuidemsg;
            str_replace(msg, "$D", LEX_CAST_STR(id), true);
            int state = getGuideState(id);
            str_replace(msg, "$S", LEX_CAST_STR(state), true);
            account->Send(msg);
        }
        return m_current_guide;
    }
    return -1;
}

//查询引导完成情况
time_t CharData::getGuideState(int id)
{
    //部分引导需要判断实际情况
    /*
        步骤3，完成状态: 第1个任务是否完成。(可领取)
        步骤4，完成状态: 第1个任务奖励是否已领取。
        步骤5，完成状态: 第1003个任务是否完成。
        步骤6，完成状态: 第1个成长礼包是否已打开。
        步骤8，完成状态: 第2个任务是否完成。(可领取)
        步骤9，完成状态: 第2个任务奖励是否已领取。
        步骤10，完成状态: 第1004个任务是否完成。
        步骤11，完成状态: 第3个任务是否完成。(可领取)
        步骤12，完成状态: 第3个任务奖励是否已领取。
        步骤13，完成状态: 第1005个任务是否完成。
        步骤14，完成状态: 第4个任务是否完成。(可领取)
        步骤15，完成状态: 第1个场景通关奖励是否领取
        步骤16，完成状态: 第4个任务奖励是否已领取。
    */
    int need_tid = 0, need_state = 0;
    switch (id)
    {
        case 3:
            {
                need_tid = 1;
                need_state = 1;
            }
            break;
        case 4:
            {
                need_tid = 1;
                need_state = 2;
            }
            break;
        case 5:
            {
                need_tid = 1003;
                need_state = 1;
            }
            break;
        case 6:
            {
                int need_libao = libaoMgr::getInstance()->getChengzhangLibao(1);
                int slot = m_bag.getChengzhangLibaoSlot(need_libao);
                if (slot)
                {
                    return 1;
                }
                return 0;
            }
            break;
        case 8:
            {
                need_tid = 2;
                need_state = 1;
            }
            break;
        case 9:
            {
                need_tid = 2;
                need_state = 2;
            }
            break;
        case 10:
            {
                need_tid = 1004;
                need_state = 1;
            }
            break;
        case 11:
            {
                need_tid = 3;
                need_state = 1;
            }
            break;
        case 12:
            {
                need_tid = 3;
                need_state = 2;
            }
            break;
        case 13:
            {
                need_tid = 1005;
                need_state = 1;
            }
            break;
        case 14:
            {
                need_tid = 4;
                need_state = 1;
            }
            break;
        case 15:
            {
                //是否已经领取过了
                int idx = char_data_normal_stage_reward_start + stageIndex(1,1);
                int get = queryExtraData(char_data_type_normal, idx);
                if (get)
                {
                    return 1;
                }
                return 0;
            }
            break;
        case 16:
            {
                need_tid = 4;
                need_state = 2;
            }
            break;
    }
    if (need_tid > 0)//需要判断任务情况
    {
        int tid = 0, state = 0;
        m_tasks.getCharTaskState(1, tid, state);
        if (need_state == 1)//需要该任务完成
        {
            if (tid == need_tid && state == 1)
                return 1;
        }
        else if(need_state == 2)//需要该任务领取
        {
            if (tid > need_tid)
                return 1;
        }
        return 0;
    }
    return m_guide_completes[id];
}

//设置引导完成
void CharData::setGuideStateComplete(int id, int next_guide)
{
    m_current_guide = next_guide;
    setExtraData(char_data_type_normal, char_data_normal_current_guide, m_current_guide);
    m_guide_completes[id] = time(NULL);
    InsertSaveDb("replace into char_guide_complete (cid,guide,input) values ("
        + LEX_CAST_STR(m_id) + "," + LEX_CAST_STR(id) + ",unix_timestamp())");
}

int CharData::Create()
{
    //cout<<"CharData::Create()"<<m_id<<endl;
    m_level = 1;
    m_level_data = GeneralDataMgr::getInstance()->GetLevelData(m_level);
    m_cur_mapid = 1;
    m_vip = m_tmp_vip = iTmpVip;//建号享受临时vip
    m_real_vip = 0;
    m_levelupTime = 0;
    //资源
    m_gold = g_default_gold;
    m_bind_gold = 0;
    m_silver = g_default_silver;
    for (int i = 0; i < 4; ++i)
    {
        m_prestige[i] = 0;
        m_prestige_level[i] = 1;
        m_prestige_award[i].m_cid = m_id;
        m_prestige_award[i].m_race = i+1;
        m_prestige_award[i].m_state.clear();
    }
    m_can_chat = true;
    m_can_world_chat = true;
    m_total_recharge = 0;
    m_char_exp = 0;
    m_cur_stageid = 1;
    m_cur_strongholdid = 0;
    //当前引导id
    m_current_guide = 1;
    //技能暂时全开
    for(int i = 1; i <= 20; ++i)
    {
        m_skills.Add(i);
    }
    m_score_tasks.load();
}

//从数据库中读取角色数据
int CharData::Load()
{
    INFO("CharData::Load()"<<m_id);
    Query q(GetDb());
    q.get_result("SELECT ac.union_id,ac.server_id,ac.qid,"
                "c.account,c.name,c.spic,c.level,c.exp,c.lastlogin,c.createTime,"
                "cd.mapid,cd.vip,cd.chat,cd.race,cd.levelupTime,cd.bagSize,cd.heroSize,cd.vip_exp,"
                "cr.gold,cr.bind_gold,cr.silver,cr.bank_silver,cr.prestige1,cr.prestige2,cr.prestige3,cr.prestige4,"
                "cr.prestige1_level,cr.prestige2_level,cr.prestige3_level,cr.prestige4_level"
                " FROM `charactors` as c"
                " left join `char_resource` as cr on c.id=cr.cid"
                " left join `char_data` as cd on c.id=cd.cid"
                " left join `accounts` as ac on c.account=ac.account"
                " WHERE c.id=" + LEX_CAST_STR(m_id));
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
        m_char_exp = q.getval();
        m_login_time = q.getval();
        m_createTime = q.getval();

        //根据图片确定性别奇数为男
        if (m_spic % 2 != 0)
        {
            m_gender = 1;
        }
        else
        {
            m_gender = 0;
        }

        m_cur_mapid = q.getval();
        m_vip = q.getval();
        m_real_vip = m_vip;
        m_chat = q.getstr();
        m_race = q.getval();
        m_race_data = GeneralDataMgr::getInstance()->GetBaseRace(m_race);
        m_level_data = GeneralDataMgr::getInstance()->GetLevelData(m_level);
        m_levelupTime = q.getval();
        size_t bag_size = q.getval();
        m_bag.addSize(bag_size);
        size_t hero_size = q.getval();
        m_heros.addSize(hero_size);
        m_vip_exp = q.getval();
        if (m_vip < 0)
        {
            m_vip = 0;
        }
        else if (m_vip > iMaxVIP)
        {
            m_vip = iMaxVIP;
        }

        //资源
        m_gold = q.getval();
        m_bind_gold = q.getval();
        m_silver = q.getval();
        m_bank_silver = q.getval();
        for (int i = 0; i < 4; ++i)
        {
            m_prestige[i] = q.getval();
        }
        for (int i = 0; i < 4; ++i)
        {
            m_prestige_level[i] = q.getval();
        }
        for (int i = 0; i < 4; ++i)
        {
            m_prestige_award[i].m_cid = m_id;
            m_prestige_award[i].m_race = i+1;
            m_prestige_award[i].m_state.clear();
        }
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

    //角色特殊字段
    loadExtraData();
    loadLootTimes();
    //角色装备，英雄，背包，关卡进度
    m_tempo.load(m_id, 0);
    m_heros.Load();
    m_skills.Load();
    m_bag.loadBag();
    m_wild_citys.load();
    m_tasks.load();
    m_score_tasks.load();
    m_prestige_tasks.load();
    m_Buffs.load();
    m_magics.Load();
    m_can_chat = true;
    m_can_world_chat = true;
    //当前引导id
    m_current_guide = queryExtraData(char_data_type_normal, char_data_normal_current_guide);

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

    //声望奖励领取情况
    q.get_result("select race,state from char_prestige_award where cid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int race = q.getval();
        if (race >= 1 && race <= 4)
        {
            std::string state = q.getstr();
            if (state != "")
            {
                json_spirit::Value types;
                json_spirit::read(state, types);
                if (types.type() == json_spirit::array_type)
                {
                    json_spirit::Array& types_array = types.get_array();
                    for (json_spirit::Array::iterator it = types_array.begin(); it != types_array.end(); ++it)
                    {
                        if ((*it).type() != json_spirit::int_type)
                        {
                            break;
                        }
                        m_prestige_award[race-1].m_state.push_back((*it).get_int());
                    }
                }
                else
                {
                    ERR();
                }
            }
        }
    }
    q.free_result();

    //累计充值提示
    q.get_result("select total_recharge from char_recharge_total where cid=" + LEX_CAST_STR(m_id));
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
    //临时VIP
    int finish = queryExtraData(char_data_type_normal, char_data_normal_tmp_vip);
    if (finish == 0 && m_level < iTmpVipEndLevel && m_vip < iTmpVip)
    {
        m_vip = m_tmp_vip = iTmpVip;
    }
    else
    {
        m_tmp_vip = 0;
    }
    return 0;
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

int CharData::GetGuildId()
{
    return m_guild_data.get() ? m_guild_data->m_gid : 0;
}

//给角色增加黄金
int CharData::addGold(int gold, int statistics_type, bool only_real)
{
    if (gold < 0)
        return -1;
    if (only_real)
    {
        if (m_gold + gold >= 0)
        {
            m_gold += gold;
            InsertSaveDb("update char_resource set gold=" + LEX_CAST_STR(m_gold)
                    + " where cid=" + LEX_CAST_STR(m_id));
            NotifyCharData();
            statistics_of_gold_get(m_id, m_ip_address, gold, statistics_type, m_union_id, m_server_id);
            return m_gold;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if (m_bind_gold + gold >= 0)
        {
            m_bind_gold += gold;
            InsertSaveDb("update char_resource set bind_gold=" + LEX_CAST_STR(m_bind_gold)
                    + " where cid=" + LEX_CAST_STR(m_id));
            NotifyCharData();
            statistics_of_gold_get(m_id, m_ip_address, gold, statistics_type, m_union_id, m_server_id);
            return m_gold;
        }
        else
        {
            return -1;
        }
    }
}

//给角色减少黄金
int CharData::subGold(int gold, int statistics_type, bool only_real)
{
    if (gold <= 0)
        return -1;
    if (only_real)
    {
        if (m_gold - gold >= 0)
        {
            m_gold -= gold;
            NotifyCharData();
            InsertSaveDb("update char_resource set gold=" + LEX_CAST_STR(m_gold)
                    + " where cid=" + LEX_CAST_STR(m_id));
            m_score_tasks.updateTask(DAILY_SCORE_GOLD);
            statistics_of_gold_cost(m_id, m_ip_address, gold, statistics_type, m_union_id, m_server_id);
            //周排行活动
            Singleton<weekRankings>::Instance().updateEventRankings(m_id,week_ranking_gold,gold);
            return m_gold;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if (m_bind_gold - gold >= 0)
        {
            m_bind_gold -= gold;
            NotifyCharData();
            InsertSaveDb("update char_resource set bind_gold=" + LEX_CAST_STR(m_bind_gold)
                    + " where cid=" + LEX_CAST_STR(m_id));
            m_score_tasks.updateTask(DAILY_SCORE_GOLD);
            statistics_of_gold_cost(m_id, m_ip_address, gold, statistics_type, m_union_id, m_server_id);
            return m_gold + m_bind_gold;
        }
        else if (m_gold >= (gold-m_bind_gold))
        {
            int real_gold_cost = (gold-m_bind_gold);
            m_gold -= real_gold_cost;
            m_bind_gold = 0;
            NotifyCharData();
            InsertSaveDb("update char_resource set bind_gold=0,gold=" + LEX_CAST_STR(m_gold)
                    + " where cid=" + LEX_CAST_STR(m_id));
            m_score_tasks.updateTask(DAILY_SCORE_GOLD);
            statistics_of_gold_cost(m_id, m_ip_address, gold, statistics_type, m_union_id, m_server_id);
            //周排行活动
            Singleton<weekRankings>::Instance().updateEventRankings(m_id,week_ranking_gold,real_gold_cost);
            return m_gold + m_bind_gold;
        }
        else
        {
            return -1;
        }
    }
}

//给角色增加筹码
int CharData::addSilver(int silver, int statistics_type)
{
    if (silver < 0)
        return -1;
    if (m_silver + silver >= 0)
    {
        m_silver += silver;
        InsertSaveDb("update char_resource set silver=" + LEX_CAST_STR(m_silver)
                + " where cid=" + LEX_CAST_STR(m_id));
        NotifyCharData();
        boost::shared_ptr<CharPkData> charPk = Singleton<PkMgr>::Instance().getPkData(m_id);
        if (charPk.get() && charPk->getCharData().get() && charPk->m_roomid > 0)
        {
            Singleton<PkMgr>::Instance().broadInfo(charPk->m_roomid);
        }
        statistics_of_silver_get(m_id, m_ip_address, silver, statistics_type, m_union_id, m_server_id);
        Singleton<goalMgr>::Instance().updateTask(m_id, GOAL_TYPE_SILVER, m_silver);
        return m_silver;
    }
    else
    {
        return -1;
    }
}

//给角色减少筹码
int CharData::subSilver(int silver, int statistics_type)
{
    if (silver <= 0)
        return -1;
    if (m_silver - silver >= 0)
    {
        m_silver -= silver;
        InsertSaveDb("update char_resource set silver=" + LEX_CAST_STR(m_silver)
                + " where cid=" + LEX_CAST_STR(m_id));
        NotifyCharData();
        boost::shared_ptr<CharPkData> charPk = Singleton<PkMgr>::Instance().getPkData(m_id);
        if (charPk.get() && charPk->getCharData().get() && charPk->m_roomid > 0)
        {
            Singleton<PkMgr>::Instance().broadInfo(charPk->m_roomid);
        }
        statistics_of_silver_cost(m_id, m_ip_address, silver, statistics_type, m_union_id, m_server_id);
        return m_silver;
    }
    else
    {
        return -1;
    }
}

//银行筹码存入
int CharData::addBankSilver(int silver)
{
    if (silver < 0)
        return -1;
    if (m_silver < silver)
        return -1;
    if (m_bank_silver + silver >= 0)
    {
        m_silver -= silver;
        m_bank_silver += silver;
        InsertSaveDb("update char_resource set silver=" + LEX_CAST_STR(m_silver)
                + ",bank_silver=" + LEX_CAST_STR(m_bank_silver)
                + " where cid=" + LEX_CAST_STR(m_id));
        NotifyCharData();
        boost::shared_ptr<CharPkData> charPk = Singleton<PkMgr>::Instance().getPkData(m_id);
        if (charPk.get() && charPk->getCharData().get() && charPk->m_roomid > 0)
        {
            Singleton<PkMgr>::Instance().broadInfo(charPk->m_roomid);
        }
        return m_bank_silver;
    }
    else
    {
        return -1;
    }
}

//银行筹码领取
int CharData::subBankSilver(int silver)
{
    if (silver <= 0)
        return -1;
    if (m_bank_silver < silver)
        return -1;
    if (m_silver + silver >= 0)
    {
        //手续费
        int cost = silver * 2 / 100;
        m_bank_silver -= silver;
        m_silver += (silver-cost);
        InsertSaveDb("update char_resource set silver=" + LEX_CAST_STR(m_silver)
                + ",bank_silver=" + LEX_CAST_STR(m_bank_silver)
                + " where cid=" + LEX_CAST_STR(m_id));
        NotifyCharData();
        boost::shared_ptr<CharPkData> charPk = Singleton<PkMgr>::Instance().getPkData(m_id);
        if (charPk.get() && charPk->getCharData().get() && charPk->m_roomid > 0)
        {
            Singleton<PkMgr>::Instance().broadInfo(charPk->m_roomid);
        }
        if (cost > 0)
            statistics_of_silver_cost(m_id, m_ip_address, cost, silver_cost_bank, m_union_id, m_server_id);
        return m_bank_silver;
    }
    else
    {
        return -1;
    }
}

//给角色设置黄金
int CharData::gold(int gold)
{
    m_gold = gold;
    InsertSaveDb("update char_resource set gold=" + LEX_CAST_STR(m_gold)
            + " where cid=" + LEX_CAST_STR(m_id));
    NotifyCharData();
    return m_gold;
}

//给角色设置筹码
int CharData::silver(int silver)
{
    m_silver = silver;
    InsertSaveDb("update char_resource set silver=" + LEX_CAST_STR(m_silver)
            + " where cid=" + LEX_CAST_STR(m_id));
    NotifyCharData();
    boost::shared_ptr<CharPkData> charPk = Singleton<PkMgr>::Instance().getPkData(m_id);
    if (charPk.get() && charPk->getCharData().get() && charPk->m_roomid > 0)
    {
        Singleton<PkMgr>::Instance().broadInfo(charPk->m_roomid);
    }
    if (m_level <= 17 && m_silver == 0)
    {
        int help_times = queryExtraData(char_data_type_normal,char_data_normal_help_times);
        if (help_times < 20)
        {
            checkGuide(guide_id_get_silver);
            addSilver(180000, silver_get_help);
            setExtraData(char_data_type_normal,char_data_normal_help_times,++help_times);
        }
    }
    Singleton<goalMgr>::Instance().updateTask(m_id, GOAL_TYPE_SILVER, m_silver);
    return m_silver;
}

int CharData::combatSilverMax()
{
    if (m_vip <= 0)
    {
        return 0;
    }
    else if(m_vip == 1)
    {
        return 10000;
    }
    else if(m_vip == 2)
    {
        return 20000;
    }
    else if(m_vip == 3)
    {
        return 30000;
    }
    else if(m_vip == 4)
    {
        return 40000;
    }
    return -1;
}

//各族声望
int CharData::addPrestige(int race, int prestige, int statistics_type)
{
    if (race < 1)
        race = 1;
    if (race > 4)
        race = 4;
    if (prestige < 0)
        return -1;
    if (m_prestige[race-1] + prestige >= 0)
    {
        m_prestige[race-1] += prestige;
        boost::shared_ptr<basePrestigeData> p_ld = GeneralDataMgr::getInstance()->GetPrestigeData(m_prestige_level[race-1]+1);
        while (p_ld.get() && m_prestige[race-1] >= p_ld->m_need_exp)
        {
            ++m_prestige_level[race-1];
            m_prestige[race-1] -= p_ld->m_need_exp;
            p_ld = GeneralDataMgr::getInstance()->GetPrestigeData(m_prestige_level[race-1]+1);
            //广播好友祝贺
            if (4 == m_prestige_level[race-1] || 6 == m_prestige_level[race-1] || 9 == m_prestige_level[race-1]
                || 11 == m_prestige_level[race-1] || 16 == m_prestige_level[race-1] || 19 == m_prestige_level[race-1]
                || 30 == m_prestige_level[race-1])
            {
                if (race == 1)
                {
                    Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_PRESTIGE1, m_prestige_level[race-1], 0);
                }
                else if (race == 2)
                {
                    Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_PRESTIGE2, m_prestige_level[race-1], 0);
                }
                else if (race == 3)
                {
                    Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_PRESTIGE3, m_prestige_level[race-1], 0);
                }
                else if (race == 4)
                {
                    Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_PRESTIGE4, m_prestige_level[race-1], 0);
                }
            }
        }
        InsertSaveDb("update char_resource set prestige"+LEX_CAST_STR(race)+"=" + LEX_CAST_STR(m_prestige[race-1])
                + ",prestige"+LEX_CAST_STR(race)+"_level=" + LEX_CAST_STR(m_prestige_level[race-1])
                + " where cid=" + LEX_CAST_STR(m_id));
        statistics_of_prestige_get(m_id, m_ip_address, prestige, statistics_type, m_union_id, m_server_id);
        //周排行活动
        Singleton<weekRankings>::Instance().updateEventRankings(m_id,week_ranking_prestige,prestige);
        return m_prestige[race-1];
    }
    else
    {
        return -1;
    }
}

int CharData::prestige(int race)
{
    if (race < 1)
        race = 1;
    if (race > 4)
        race = 4;
    return m_prestige[race-1];
}

int CharData::prestigeLevel(int race)
{
    if (race < 1)
        race = 1;
    if (race > 4)
        race = 4;
    return m_prestige_level[race-1];
}

int CharData::prestigeMax(int race)
{
    if (race < 1)
        race = 1;
    if (race > 4)
        race = 4;
    boost::shared_ptr<basePrestigeData> p_ld = GeneralDataMgr::getInstance()->GetPrestigeData(m_prestige_level[race-1]+1);
    if (p_ld.get())
    {
        return p_ld->m_need_exp;
    }
    return m_prestige[race-1];
}

//给角色增加历练
int CharData::addCharExp(int exp, int statistics_type)
{
    if (exp < 0)
        return -1;
    if (m_char_exp + exp >= 0)
    {
        m_char_exp += exp;
        boost::shared_ptr<baseLevelData> p_ld = GeneralDataMgr::getInstance()->GetLevelData(m_level+1);
        if (p_ld.get() && m_char_exp >= p_ld->m_need_char_exp)
        {
            levelup(m_level+1);
        }
        else
        {
            InsertSaveDb("update charactors set exp=" + LEX_CAST_STR(m_char_exp)
                + " where id=" + LEX_CAST_STR(m_id));
            NotifyCharData();
        }
        statistics_of_char_exp_get(m_id, m_ip_address, exp, statistics_type, m_union_id, m_server_id);
        return m_char_exp;
    }
    else
    {
        return -1;
    }
}

//角色升级
int CharData::levelup(int level)
{
    if (level < 1)
        level = 1;
    if (level > iMaxLevel)
        level = iMaxLevel;
    if (m_level < level)
    {
        //升级提示信息变动
        json_spirit::Object lvup_obj;
        lvup_obj.push_back( Pair("cmd", "charLevelUp") );
        lvup_obj.push_back( Pair("s", 200) );
        json_spirit::Object org_data;
        org_data.push_back( Pair("level", m_level) );
        if (m_level_data.get())
        {
            org_data.push_back( Pair("rewardAdd", m_level_data->m_reward_add) );
        }
        boost::shared_ptr<CharHeroData> p_hero = m_heros.GetHero(m_heros.m_default_hero);
        if (p_hero.get())
        {
            org_data.push_back( Pair("attack", p_hero->m_attack) );
            org_data.push_back( Pair("defense", p_hero->m_defense) );
            org_data.push_back( Pair("magic", p_hero->m_magic) );
            org_data.push_back( Pair("hp", p_hero->m_hp) );
        }
        lvup_obj.push_back( Pair("org_data", org_data) );

        m_level = level;
        m_char_exp = 0;
        m_levelupTime = time(NULL);    //记录角色升级时间，排名时用到
        m_level_data = GeneralDataMgr::getInstance()->GetLevelData(m_level);
        m_heros.updateAttribute();

        json_spirit::Object now_data;
        now_data.push_back( Pair("level", m_level) );
        if (m_level_data.get())
        {
            now_data.push_back( Pair("rewardAdd", m_level_data->m_reward_add) );
            if (m_level_data->m_guide_id > 0)
            {
                //新手引导
                checkGuide(m_level_data->m_guide_id);
            }
        }
        if (p_hero.get())
        {
            now_data.push_back( Pair("attack", p_hero->m_attack) );
            now_data.push_back( Pair("defense", p_hero->m_defense) );
            now_data.push_back( Pair("magic", p_hero->m_magic) );
            now_data.push_back( Pair("hp", p_hero->m_hp) );
        }
        lvup_obj.push_back( Pair("now_data", now_data) );
        sendObj(lvup_obj);
        //开放顶栏按钮
        if (m_level == iSignOpenLevel)
        {
            addTopButton(top_button_sign, 1);
        }
        if (m_level == iOnlineLibaoOpenLevel)
        {
            int active = 0;
            char_online_libao_data* online = Singleton<actionMgr>::Instance().getCharOnlineLibaoData(m_id);
            active = online->getOnlineLibaoState();
            addTopButton(top_button_online, active);
        }
        if (m_level == iFirstRechargeOpenLevel)
        {
            addTopButton(top_button_first_recharge, 1);
        }
        if (m_level == iTimelimitActionOpenLevel)
        {
            int state = Singleton<actionMgr>::Instance().getTimeLimitActionState(this);
            if (state != 2)
            {
                addTopButton(top_button_timeLimitAction, state);
            }
        }
        if (m_level == iRechargeActionOpenLevel)
        {
            int active = recharge_event_mgr::getInstance()->getRechargeEventState(this);
            addTopButton(top_button_rechargeAction, active);
        }
        if (m_level == iTreasureOpenLevel)
        {
            boost::shared_ptr<char_treasure> pct = Singleton<treasureMgr>::Instance().getCharTreasure(m_id);
            if (pct.get())
            {
                addTopButton(top_button_treasure, 0, pct->getCanStartTimes());
            }
        }
        if (m_level == iDailyTaskOpenLevel)
        {
            m_tasks.dailyUpdate();
            addTopButton(top_button_daily_task, 0, m_tasks.getCharDailyTaskCnt());
        }
        if (m_level == iDailyScoreOpenLevel)
        {
            addTopButton(top_button_daily, 0);
        }
        if (m_level == iPrestigeOpenLevel)
        {
            addTopButton(top_button_prestige_task, 0);
        }
        if (m_level == iBossOpenLevel)
        {
            int active = bossMgr::getInstance()->isOpen() ? 1 : 0;
            addTopButton(top_button_dailyAction, active);
        }
        if (m_level == iBankOpenLevel)
        {
            addTopButton(top_button_bank, 0);
            bankMgr::getInstance()->open(m_id);
        }
        if (m_level == iLotteryActionOpenLevel)
        {
            if (Singleton<lottery_event>::Instance().isOpen())
            {
                addTopButton(top_button_lotteryAction, Singleton<lottery_event>::Instance().getActionState(this));
            }
        }
        if (m_level >= iTmpVipEndLevel)
        {
            m_tmp_vip = 0;
            m_vip = m_real_vip;
            //临时vip结束
            setExtraData(char_data_type_normal, char_data_normal_tmp_vip, 1);
        }
        //同步界面信息
        NotifyCharData();
        InsertSaveDb("update charactors set level=" + LEX_CAST_STR(m_level)
                    + ",exp=" + LEX_CAST_STR(m_char_exp) +
                    + ",lastlogin=" + LEX_CAST_STR(m_login_time) + " where id=" + LEX_CAST_STR(m_id));
        //任务
        m_tasks.updateTask(GOAL_CHAR_LEVEL, 0, m_level);
        //目标
        Singleton<goalMgr>::Instance().updateLevel(m_id, m_level);
        Singleton<goalMgr>::Instance().updateTask(m_id, GOAL_TYPE_CHAR_LEVEL, m_level);
        //升级，广播给好友，受祝贺
        if (10 == m_level || 20 == m_level || 30 == m_level || 40 == m_level || m_level >= 50)
        {
            Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_LEVEL, m_level, 0);
        }
        //好友推送
        if (10 == m_level || 15 == m_level || 20 == m_level)
        {
            if (GeneralDataMgr::getInstance()->canRecommendFriends(this))
            {
                //通知被请求玩家
                json_spirit::Object obj;
                obj.push_back( Pair("cmd", "friendRecommend") );
                obj.push_back( Pair("s", 200) );
                sendObj(obj);
            }
        }
        updateCombatAttribute(m_skills.m_skills, m_combat_attribute, m_level);
    }
    return m_level;
}

//增加角色装备
int CharData::addEquipt(int id, int level, int quality)
{
    if (quality == 0)
        quality = 1;
    //超过背包上限，放到回购中
    if (m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    boost::shared_ptr<baseEquipment> baseEq = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
    if (!baseEq.get())
    {
        ERR();
        return -1;
    }
    int equipt_id = GeneralDataMgr::getInstance()->newEquiptId();
    InsertSaveDb("insert into char_equipment (id,cid,base_id,level,quality) values ("
            + LEX_CAST_STR(equipt_id) + "," + LEX_CAST_STR(m_id)
            + "," + LEX_CAST_STR(id) + "," + LEX_CAST_STR(level) + "," + LEX_CAST_STR(quality) + ")");
    Equipment* pdata = new Equipment(id, equipt_id, m_id, level, quality);
    boost::shared_ptr<item_base> eq(pdata);
    m_bag.addItem(eq);
    pdata->Save();
    //更新任务
    m_tasks.updateTask(GOAL_EQUIPT_QUALITY, quality, 1);
    //通知客户端
    json_spirit::Object obj;
    json_spirit::Object item;
    item.push_back( Pair("type", ITEM_TYPE_EQUIPMENT) );
    item.push_back( Pair("spic", pdata->getSpic()) );
    obj.push_back( Pair("item", item) );
    obj.push_back( Pair("cmd", "notifyGet") );
    obj.push_back( Pair("s", 200) );
    sendObj(obj);
    INFO("add equip price "<<pdata->sellPrice()<<",quality:"<<pdata->getQuality()<<",level "<<pdata->getLevel());
    return pdata->getId();
}

//减少角色装备
int CharData::subEquipt(int id)
{
    //背包里
    Equipment* equip = m_bag.getEquipById(id);
    if (!equip)
    {
        //英雄身上
        return HC_ERROR;
    }
    else
    {
        boost::shared_ptr<item_base> itm = m_bag.removeItem(equip->getSlot());
        if (itm.get())
        {
            itm->Save();
        }
    }
    return HC_SUCCESS;
}

//增加角色道具
int CharData::addGem(int id, int counts, int statistics_type)
{
    if (counts < 0)
        return -1;
    int err_code = 0;
    int32_t ret = m_bag.addGem(id, counts, err_code);
    if (ret > 0)
        statistics_of_gem_get(m_id, m_ip_address, id, counts, statistics_type, m_union_id, m_server_id);
    return ret;
}

//消耗角色道具
int CharData::subGem(int id, int counts, int statistics_type)
{
    if (counts < 0)
        return -1;
    int err_code = 0;
    int32_t ret = m_bag.addGem(id, -counts, err_code);
    if (ret > 0)
        statistics_of_gem_cost(m_id, m_ip_address, id, counts, statistics_type, m_union_id, m_server_id);
    return ret;
}

//增加角色资源道具
int CharData::addCurrency(int id, int counts)
{
    if (counts < 0)
        return -1;
    boost::shared_ptr<baseGem> bt = GeneralDataMgr::getInstance()->GetBaseGem(id);
    if (!bt.get() || !bt->currency)
    {
        return 0;
    }
    Currency& c = m_currencys[id];
    if ((c.count + counts) >= 0)
    {
        int tmp = c.count;
        c.count += counts;
        if (c.id == 0)
        {
            c.type = id;
            c.id = GeneralDataMgr::getInstance()->newGemId();
            //保存数据库
            InsertSaveDb("insert into char_gem (id,cid,tid,nums,slot) value ("
                + LEX_CAST_STR(c.id) + ","
                + LEX_CAST_STR(m_id) + ","
                + LEX_CAST_STR(id) + ","
                + LEX_CAST_STR(c.count) + ",255)");
        }
        else
        {
            //保存数据库
            InsertSaveDb("update char_gem set nums=" + LEX_CAST_STR(c.count)+ " where id=" + LEX_CAST_STR(c.id));
        }
        return c.count;
    }
    else
    {
        return -1;
    }
}

//消耗角色资源道具
int CharData::subCurrency(int id, int counts)
{
    if (counts < 0)
        return -1;
    boost::shared_ptr<baseGem> bt = GeneralDataMgr::getInstance()->GetBaseGem(id);
    if (!bt.get() || !bt->currency)
    {
        return 0;
    }
    Currency& c = m_currencys[id];
    if ((c.count - counts) >= 0 && c.id > 0)
    {
        int tmp = c.count;
        c.count -= counts;
        //保存数据库
        InsertSaveDb("update char_gem set nums=" + LEX_CAST_STR(c.count)+ " where id=" + LEX_CAST_STR(c.id));
        return c.count;
    }
    else
    {
        return -1;
    }
}

int CharData::addLibao(int libao_id, int counts)
{
    int32_t ret = 0;
    for (int i = 0; i < counts; ++i)
    {
        ret = libaoMgr::getInstance()->addLibao(this,libao_id);
    }
    return ret;
}

int CharData::addBaoshi(int base_id, int level, int counts)
{
    return m_bag.addBaoshi(base_id, level, counts);
}

//镶嵌宝石
int CharData::inlayBaoshi(int bagSlot, int eid, int slot)
{
    boost::shared_ptr<item_base> itm = m_bag.getItem(bagSlot);
    if (!itm.get() || itm->getType() != ITEM_TYPE_BAOSHI)
    {
        return HC_ERROR;
    }
    Equipment* edata = m_bag.getEquipById(eid);
    if (!edata)
    {
        edata = m_heros.getEquipById(eid);
        if (!edata)
        {
            return HC_ERROR;
        }
    }
    if (edata)
    {
        //判断类型
        Baoshi* pb = dynamic_cast<Baoshi*>(itm.get());
        if (!(edata->getEquipType() == pb->getBaoshiType()))
        {
            return HC_ERROR_INLAY_TYPE;
        }
        if (edata->m_bag.isFull())
        {
            return HC_ERROR_INLAY_FULL;
        }
        //指定位置
        boost::shared_ptr<item_base> oldbs;
        oldbs.reset();
        if (slot > 0)
        {
            if (slot > edata->m_bag.getSize())
            {
                return HC_ERROR;
            }
            oldbs = edata->m_bag.getItem(slot);
            //为免背包溢出,卸下等新嵌入宝石完成了再放回背包
            edata->m_bag.removeItem(slot,false);
        }
        int ret = HC_ERROR;
        if (pb->getCount() == 1)//镶嵌
        {
            m_bag.removeItem(bagSlot);
            if (slot > 0)
                edata->m_bag.addItem(slot, itm, false);
            else
                edata->m_bag.addItem(itm,false);
            itm->setChanged();
            itm->Save();
            edata->updateAttribute();
            ret = HC_SUCCESS;
        }
        else//拆分再镶嵌
        {
            pb->addCount(-1);
            boost::shared_ptr<item_base> bs = m_bag.cloneBaoshi(pb->getSubType(), pb->getLevel(), 1);
            if (slot > 0)
                edata->m_bag.addItem(slot, bs, false);
            else
                edata->m_bag.addItem(bs,false);
            bs->setChanged();
            itm->setChanged();
            itm->Save();
            bs->Save();
            edata->updateAttribute();
            ret = HC_SUCCESS;
        }
        //卸下的宝石放回背包
        if (oldbs.get())
        {
            Baoshi* p_old = dynamic_cast<Baoshi*>(oldbs.get());
            Baoshi* pm = m_bag.getBaoshiCanMerge(p_old->getSubType(), p_old->getLevel(), 1);
            if (pm)
            {
                pm->addCount(1);
                pm->Save();
                //删除原来的宝石
                oldbs->Clear();
                oldbs->Save();
            }
            else
            {
                m_bag.addItem(oldbs);
                oldbs->Save();
            }
        }
        return ret;
    }
    return HC_ERROR;
}

//移除宝石
int CharData::removeBaoshi(int eid, int slot)
{
    Equipment* edata = m_bag.getEquipById(eid);
    if (!edata)
    {
        edata = m_heros.getEquipById(eid);
        if (!edata)
        {
            return HC_ERROR;
        }
    }
    if (edata)
    {
        boost::shared_ptr<item_base> itm = edata->m_bag.getItem(slot);
        if (itm.get())
        {
            Baoshi* pb = dynamic_cast<Baoshi*>(itm.get());
            Baoshi* pm = m_bag.getBaoshiCanMerge(pb->getSubType(), pb->getLevel(), 1);
            if (pm)
            {
                itm = edata->m_bag.removeItem(slot,false);
                if (itm.get())
                {
                    pm->addCount(1);
                    pm->Save();
                    //删除原来的宝石
                    itm->Clear();
                    itm->Save();
                    edata->updateAttribute();
                }
            }
            else if (m_bag.isFull())
            {
                return HC_ERROR_BAG_FULL;
            }
            else
            {
                itm = edata->m_bag.removeItem(slot,false);
                if (itm.get())
                {
                    m_bag.addItem(itm);
                    itm->Save();
                    edata->updateAttribute();
                }
            }
        }
    }
    return HC_SUCCESS;
}

//合并宝石
int CharData::CombineBaoshi(int base_id, int level, int nums, json_spirit::Object& robj)
{
    if (nums < 0)
    {
        return HC_ERROR;
    }
    if (level >= MAX_BAOSHI_LEVEL)
    {
        return HC_ERROR;
    }
    int ret = HC_SUCCESS;
    std::list<boost::shared_ptr<item_base> > list1;
    std::list<boost::shared_ptr<item_base> > list2;

    int level1_count = 0, level2_count = 0;
    for (int i = 1; i <= m_bag.m_size; ++i)
    {
        //只算在背包里面的
        if (m_bag.m_bagslot[i-1].get())
        {
            item_base* pp = m_bag.m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_BAOSHI && pp->getSubType() == base_id)
            {
                Baoshi* p = dynamic_cast<Baoshi*>(pp);
                if (p->getLevel() == level)
                {
                    level1_count += pp->getCount();
                    list1.push_back(m_bag.m_bagslot[i-1]);
                }
                else if (p->getLevel() == (level + 1))
                {
                    level2_count += pp->getCount();
                    list2.push_back(m_bag.m_bagslot[i-1]);
                }
            }
        }
    }

    if (iBaoshiCombineCnt*nums > level1_count)
    {
        nums = level1_count / iBaoshiCombineCnt;
    }

    //cout<<"combine baoshi type:"<<type<<",nums:"<<nums<<","<<level1_count<<"|"<<level2_count<<endl;
    if (nums == 0)
    {
        return HC_ERROR;
    }
    int sub1 = -iBaoshiCombineCnt*nums;
    int sub = m_bag.addBaoshiCount(base_id, level, sub1);
    if (sub != sub1)//不够扣则返还宝石取消合成
    {
        //cout<<"sub:"<<sub<<",sub1:"<<sub1<<endl;
        m_bag.addBaoshiCount(base_id, level, -sub);
        return HC_ERROR;
    }

    int add = m_bag.addBaoshiCount(base_id, level + 1, nums);
    if (add < nums)//合成太多放不下则返还部分
    {
        m_bag.addBaoshiCount(base_id, level, iBaoshiCombineCnt*(nums-add));
        ret = HC_ERROR_BAG_FULL;
    }
    json_spirit::Array list;
    boost::shared_ptr<baseBaoshi> bbs = GeneralDataMgr::getInstance()->GetBaseBaoshi(base_id);
    if (bbs.get())
    {
        json_spirit::Object o;
        Item item(ITEM_TYPE_BAOSHI, bbs->id, add, level+1);
        item.toObj(o);
        list.push_back(o);
    }
    robj.push_back( Pair("get", list) );
    return ret;
}

int CharData::CombineAllBaoshi(int tolevel, json_spirit::Object& robj)
{
    if (tolevel > MAX_BAOSHI_LEVEL)
    {
        tolevel = MAX_BAOSHI_LEVEL;
    }
    if (tolevel <= 1)
    {
        return HC_SUCCESS;
    }
    int ret = HC_SUCCESS;
    std::map<std::pair<int,int>,int> list_pre;
    std::map<std::pair<int,int>,int> list_now;
    //合成前计数
    for (int i = 1; i <= m_bag.m_size; ++i)
    {
        //只算在背包里面的
        if (m_bag.m_bagslot[i-1].get())
        {
            item_base* pp = m_bag.m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_BAOSHI)
            {
                int base_id = pp->getSubType();
                Baoshi* p = dynamic_cast<Baoshi*>(pp);
                int level = p->getLevel();
                list_pre[std::make_pair(base_id,level)] += pp->getCount();
            }
        }
    }
    //批量合成
    for (int base_id = 1; base_id <= iBaoShiType; ++base_id)
    {
        for (int level = 1; level < tolevel; ++level)
        {
            int level1_count = 0, level2_count = 0;
            for (int i = 1; i <= m_bag.m_size; ++i)
            {
                //只算在背包里面的
                if (m_bag.m_bagslot[i-1].get())
                {
                    item_base* pp = m_bag.m_bagslot[i-1].get();
                    if (pp->getType() == ITEM_TYPE_BAOSHI && pp->getSubType() == base_id)
                    {
                        Baoshi* p = dynamic_cast<Baoshi*>(pp);
                        if (p->getLevel() == level)
                        {
                            level1_count += pp->getCount();
                        }
                        else if (p->getLevel() == (level + 1))
                        {
                            level2_count += pp->getCount();
                        }
                    }
                }
            }
            int nums = level1_count / iBaoshiCombineCnt;
            int sub1 = -iBaoshiCombineCnt*nums;
            int sub = m_bag.addBaoshiCount(base_id, level, sub1);
            if (sub != sub1)
            {
                //cout<<"sub:"<<sub<<",sub1:"<<sub1<<endl;
                m_bag.addBaoshiCount(base_id, level, -sub);
                return HC_ERROR;
            }
            int add = m_bag.addBaoshiCount(base_id, level + 1, nums);
            if (add < nums)
            {
                m_bag.addBaoshiCount(base_id, level, iBaoshiCombineCnt*(nums-add));
                ret = HC_ERROR_BAG_FULL;
            }
        }
    }
    //合成后计数
    for (int i = 1; i <= m_bag.m_size; ++i)
    {
        //只算在背包里面的
        if (m_bag.m_bagslot[i-1].get())
        {
            item_base* pp = m_bag.m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_BAOSHI)
            {
                int base_id = pp->getSubType();
                Baoshi* p = dynamic_cast<Baoshi*>(pp);
                int level = p->getLevel();
                list_now[std::make_pair(base_id,level)] += pp->getCount();
            }
        }
    }
    //统计通知
    json_spirit::Array list;
    for (int base_id = 1; base_id <= iBaoShiType; ++base_id)
    {
        for (int level = 1; level <= tolevel; ++level)
        {
            if (list_now[std::make_pair(base_id,level)] > list_pre[std::make_pair(base_id,level)])
            {
                boost::shared_ptr<baseBaoshi> bbs = GeneralDataMgr::getInstance()->GetBaseBaoshi(base_id);
                if (bbs)
                {
                    json_spirit::Object o;
                    Item item(ITEM_TYPE_BAOSHI, bbs->id, list_now[std::make_pair(base_id,level)] - list_pre[std::make_pair(base_id,level)], level);
                    item.toObj(o);
                    list.push_back(o);
                }
            }
        }
    }
    robj.push_back( Pair("get", list) );
    return ret;
}

//保存角色信息
int CharData::Save()
{
    //INFO("***************** save "<<m_id <<" *************************");
    m_save_time = time(NULL);
    InsertSaveDb("update char_data set levelupTime=" + LEX_CAST_STR(m_levelupTime)
        + ",mapid=" + LEX_CAST_STR(m_cur_mapid)
        + ",vip=" + LEX_CAST_STR(m_real_vip)
        + ",race=" + LEX_CAST_STR(m_race)
        + " where cid=" + LEX_CAST_STR(m_id));
    InsertSaveDb("update charactors set level=" + LEX_CAST_STR(m_level)
        + ",lastlogin=" + LEX_CAST_STR(m_login_time)
        + " where id=" + LEX_CAST_STR(m_id));
    return 0;
}

//更新vip等级
int CharData::updateVip()
{
    int old_vip = m_vip;
    int total = m_total_recharge + m_vip_exp;
    m_vip = 0;
    for (int i = iMaxVIP; i >= 1; --i)
    {
        if (total >= iVIP_recharge[i-1])
        {
            m_vip = i;
            break;
        }
    }
    //真实vip升级
    if (m_real_vip < m_vip)
    {
        m_real_vip = m_vip;
        InsertSaveDb("update char_data set vip='" + LEX_CAST_STR(m_real_vip)
            + "' where cid=" + LEX_CAST_STR(m_id));
		std::string msg = strVipMessage[m_vip-1];
        str_replace(msg, "$W", MakeCharNameLink(m_name,m_nick.get_string()));
		if (msg != "")
		{
			GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
		}
        Singleton<goalMgr>::Instance().updateTask(m_id, GOAL_TYPE_VIP, m_vip);
        Singleton<goalMgr>::Instance().updateVip(m_id, m_vip);
        Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_VIP, m_vip, 0);
    }
    //当前在临时VIP期间
    if (m_tmp_vip != 0)
    {
        if (m_tmp_vip > m_vip)
        {
            m_vip = m_tmp_vip;
        }
        else
        {
            //临时vip结束
            setExtraData(char_data_type_normal, char_data_normal_tmp_vip, 1);
        }
    }
    return m_vip;
}

int CharData::HeartBeat()
{
    return 0;
}

int CharData::getTodayOnlineTime()
{
    //此次登录在线时长
    time_t time_now = time(NULL);
    int online_sec = time_now - m_login_time;
    struct tm tm_i;
    struct tm *t_login = &tm_i;
    struct tm tm_o;
    struct tm *t_now = &tm_o;
    localtime_r(&m_login_time, t_login);
    localtime_r(&time_now, t_now);
    //跨天了减去昨天在线的时间
    if (t_now->tm_mday != t_login->tm_mday)
    {
        time_t today_start = getZeroTime();
        online_sec -= (today_start - m_login_time);
    }
    //之前记录的总在线时长
    int daily_online = queryExtraData(char_data_type_daily, char_data_daily_online_time);
    return (daily_online + online_sec);
}

//查询角色信息
int CharData::getRoleInfo(json_spirit::Object& charobj)
{
    //加入角色等级信息、黄金、白银、军令
    charobj.push_back( Pair("id", m_id));
    charobj.push_back( Pair("spic", m_spic));
    charobj.push_back( Pair("vip", m_vip));
    charobj.push_back( Pair("tmp_vip", m_tmp_vip));
    charobj.push_back( Pair("name", m_name));
    charobj.push_back( Pair("gold", m_gold));
    charobj.push_back( Pair("bind_gold", m_bind_gold));
    charobj.push_back( Pair("silver", m_silver));
    charobj.push_back( Pair("guildid", GetGuildId()));
    charobj.push_back( Pair("guildname", Singleton<guildMgr>::Instance().getGuildName(GetGuildId())));
    charobj.push_back( Pair("mapid", m_cur_mapid));
    charobj.push_back( Pair("stageid", m_cur_stageid));
    charobj.push_back( Pair("strongholdid", m_cur_strongholdid));
    charobj.push_back( Pair("race", m_race));
    charobj.push_back( Pair("level", m_level));
    charobj.push_back( Pair("default_hero", m_heros.m_default_hero));
    charobj.push_back( Pair("char_exp", m_char_exp) );
    boost::shared_ptr<baseLevelData> p_ld = GeneralDataMgr::getInstance()->GetLevelData(m_level+1);
    if (p_ld.get())
    {
        charobj.push_back( Pair("need_char_exp", p_ld->m_need_char_exp) );
    }
    boost::shared_ptr<charExplore> ce = Singleton<exploreMgr>::Instance().getCharExploreData(m_id);
    if (ce.get() && ce->m_start_time)
    {
        charobj.push_back( Pair("explore_cave", ce->m_cave_id) );
    }
    //战力
    boost::shared_ptr<CharHeroData> p_hero = m_heros.GetHero(m_heros.m_default_hero);
    if (p_hero.get())
    {
        charobj.push_back( Pair("attack_power", p_hero->m_attribute) );
    }
    return HC_SUCCESS;
}

//角色详细信息
int CharData::getRoleDetail(json_spirit::Object& robj)
{
    json_spirit::Object role;
    role.push_back( Pair("id", m_id) );
    role.push_back( Pair("spic", m_spic) );
    role.push_back( Pair("name", m_name) );
    role.push_back( Pair("nick", m_nick.get_string()) );
    role.push_back( Pair("chat", m_chat) );
    role.push_back( Pair("level", m_level) );
    role.push_back( Pair("race", m_race) );
    //战力
    boost::shared_ptr<CharHeroData> p_hero = m_heros.GetHero(m_heros.m_default_hero);
    if (p_hero.get())
    {
        role.push_back( Pair("attack_power", p_hero->m_attribute) );
    }
    robj.push_back( Pair("role", role) );

    //种族加成
    json_spirit::Object o;
    if (m_race_data.get())
    {
        int attack_add = 0, defense_add = 0, magic_add = 0;
        if (m_race_data->getLevelAdd(m_level, attack_add, defense_add, magic_add))
        {
            json_spirit::Object curAdd;
            curAdd.push_back( Pair("attack_add", attack_add) );
            curAdd.push_back( Pair("defense_add", defense_add) );
            curAdd.push_back( Pair("magic_add", magic_add) );
            o.push_back( Pair("curAdd", curAdd) );
        }
        if (m_race_data->getLevelAdd(m_level+1, attack_add, defense_add, magic_add))
        {
            json_spirit::Object nextAdd;
            nextAdd.push_back( Pair("attack_add", attack_add) );
            nextAdd.push_back( Pair("defense_add", defense_add) );
            nextAdd.push_back( Pair("magic_add", magic_add) );
            o.push_back( Pair("nextAdd", nextAdd) );
        }
        robj.push_back( Pair("race_data", o) );
    }
    o.clear();
    o.push_back( Pair("char_exp", m_char_exp) );
    if (m_level_data.get())
    {
        o.push_back( Pair("rewardAdd", m_level_data->m_reward_add) );
    }
    boost::shared_ptr<baseLevelData> p_ld = GeneralDataMgr::getInstance()->GetLevelData(m_level+1);
    if (p_ld.get())
    {
        o.push_back( Pair("need_char_exp", p_ld->m_need_char_exp) );
    }
    robj.push_back( Pair("level_data", o) );
    //部队信息
    json_spirit::Array hlist;
    m_heros.getList(hlist);
    robj.push_back( Pair("hlist", hlist) );
    return HC_SUCCESS;
}

//角色详细信息
int CharData::getTopButtonList(json_spirit::Array& list)
{
#ifdef QQ_PLAT
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_yellow) );
        list.push_back(obj);
    }
#endif
    //活动按钮
    Singleton<actionMgr>::Instance().getButton(this,list);
    //藏宝图按钮
    Singleton<treasureMgr>::Instance().getButton(this,list);
    //每日必做按钮
    Singleton<dailyScoreMgr>::Instance().getButton(this,list);
    //充值活动按钮
    recharge_event_mgr::getInstance()->getButton(this,list);
    //日常任务按钮
    m_tasks.getButton(list);
    //目标系统
    boost::shared_ptr<CharGoal> cg = Singleton<goalMgr>::Instance().getCharGoal(m_id);
    if (cg.get())
    {
        cg->getButton(list);
    }
    //每日活动按钮
    if (isBossOpen())
    {
        int active = bossMgr::getInstance()->isOpen() ? 1 : 0;
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_dailyAction) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    //投资银行
    bankMgr::getInstance()->getButton(this,list);
    //转盘
    Singleton<lottery_event>::Instance().getButton(this,list);
#ifndef QQ_PLAT
    //兑换码礼包
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_pack) );
        list.push_back(obj);
    }
#endif
    //周排行
    Singleton<weekRankings>::Instance().getButton(this,list);

    //城堡征收招募炼金房熔炼
    Singleton<cityMgr>::Instance().getButton(this,list);
    //声望任务
    Singleton<PrestigeTaskMgr>::Instance().getButton(this,list);
    //找回
    Singleton<findBackMgr>::Instance().getButton(this,list);

    //各种领取按钮
    Singleton<rewardsMgr>::Instance().getButton(this,list);
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
        getRoleInfo(cobj);
        obj.push_back( Pair("chardata", cobj) );
        sk->send(json_spirit::write(obj, json_spirit::raw_utf8));
    }
    return HC_SUCCESS;
}

//通知客户端顶栏发生变化(每日更新)
int CharData::NotifyCharTopButtonList()
{
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "getTopButtonList"));
    obj.push_back( Pair("s",200) );
    json_spirit::Array list;
    getTopButtonList(list);
    obj.push_back( Pair("list", list) );
    sendObj(obj);
    return HC_SUCCESS;
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

void CharData::loadLootTimes()
{
    Query q(GetDb());
    q.get_result("select type,id,value from char_loot_times where 1 and cid=" + LEX_CAST_STR(m_id));
    while (q.fetch_row())
    {
        int type = q.getval();
        int id = q.getval();
        int value = q.getval();
        m_loot_cnt.insert(std::make_pair(std::make_pair(type,id),value));
    }
    q.free_result();
}

int CharData::getLootTimes(int loot_type, int loot_id)
{
    return m_loot_cnt[std::make_pair(loot_type, loot_id)];
}

void CharData::setLootTimes(int loot_type, int loot_id, int value)
{
    m_loot_cnt[std::make_pair(loot_type, loot_id)] = value;
    InsertSaveDb("replace into char_loot_times (cid,type,id,value) values ("
        + LEX_CAST_STR(m_id) + ","
        + LEX_CAST_STR(loot_type) + ","
        + LEX_CAST_STR(loot_id) + ","
        + LEX_CAST_STR(value) + ")");
}

void CharData::clearLootTimes()
{
    m_loot_cnt.clear();
}

void CharData::SaveNick()
{
    InsertSaveDb("update charactors set nick='" + m_nick.get_string() + "' where id=" + LEX_CAST_STR(m_id));
}

void CharData::updateTopButton(int type, int active, int leftNums, int extra1, int extra2, int extra3)
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        if (leftNums < 0)
        {
            leftNums = 0;
        }
        account->Send("{\"type\":" + LEX_CAST_STR(type)
            + ",\"active\":" + LEX_CAST_STR(active)
            + ",\"leftNums\":" + LEX_CAST_STR(leftNums)
            + ",\"extra1\":" + LEX_CAST_STR(extra1)
            + ",\"extra2\":" + LEX_CAST_STR(extra2)
            + ",\"extra3\":" + LEX_CAST_STR(extra3)
            + ",\"cmd\":\"updateTopButton\",\"s\":200}");
    }
}

void CharData::addTopButton(int type, int active, int leftNums, int extra1, int extra2, int extra3)
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        account->Send("{\"type\":" + LEX_CAST_STR(type)
            + ",\"active\":" + LEX_CAST_STR(active)
            + ",\"leftNums\":" + LEX_CAST_STR(leftNums)
            + ",\"extra1\":" + LEX_CAST_STR(extra1)
            + ",\"extra2\":" + LEX_CAST_STR(extra2)
            + ",\"extra3\":" + LEX_CAST_STR(extra3)
            + ",\"cmd\":\"addTopButton\",\"s\":200}");
    }
}

void CharData::removeTopButton(int type)
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_name);
    if (account.get())
    {
        account->Send("{\"type\":" + LEX_CAST_STR(type) + ",\"cmd\":\"removeTopButton\",\"s\":200}");
    }
}

bool CharData::canFindBack()
{
    return queryExtraData(char_data_type_daily,char_data_daily_findback) > 0;
}

int CharData::ShowEquipts(json_spirit::Object& obj, json_spirit::mObject& o)
{
    //窗口展现类型，仅帮客户端储存返回，不处理
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    obj.push_back( Pair("type", type));
    int cur_nums = 0;
    json_spirit::Array elists;
    int purpose = 0, hid = 0;
    READ_INT_FROM_MOBJ(purpose, o, "purpose");//0全部装备，1英雄装备，2背包装备
    READ_INT_FROM_MOBJ(hid, o, "hid");
    obj.push_back( Pair("purpose", purpose) );
    obj.push_back( Pair("hid", hid) );
    if (purpose == 0 || purpose == 1)
    {
        //查询具体某个英雄的时候支持查别人
        if (purpose == 1 && hid > 0)
        {
            int cid = GeneralDataMgr::getInstance()->getHeroOwner(hid);
            if (cid)
            {
                CharData* pcc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
                if (pcc)
                {
                    boost::shared_ptr<CharHeroData> h_data = pcc->m_heros.GetHero(hid);
                    if (h_data.get())
                    {
                        h_data->m_bag.showBagEquipments(elists, o, cur_nums);
                    }
                }
            }
        }
        else
        {
            //所有英雄身上的装备
            std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = m_heros.m_heros.begin();
            while (it != m_heros.m_heros.end())
            {
                if(it->second.get())
                {
                    //筛选某个英雄
                    if (hid != 0 && hid != it->second->m_id)
                    {
                        ++it;
                        continue;
                    }
                    it->second->m_bag.showBagEquipments(elists, o, cur_nums);
                    cout << "push hero "<< it->second->m_id << " bag size=" << cur_nums << endl;
                }
                ++it;
            }
        }
    }
    if (purpose == 0 || purpose == 2)
    {
        //背包里的装备
        m_bag.showBagEquipments(elists, o, cur_nums);
        cout << "push char bag size=" << cur_nums << endl;
    }
    obj.push_back( Pair("list", elists) );
    //页面信息
    int page = 1, nums_per_page = 8;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 8;
    }
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    obj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//装备东西
int CharData::equipt(int hid, int slot, int eid)
{
    boost::shared_ptr<CharHeroData> hd = m_heros.GetHero(hid);
    if (!hd.get())
    {
        return HC_ERROR;
    }
    int ret = hd->equipt(slot, eid);
    return ret;
}

//卸下东西
int CharData::unequipt(int hid, int slot)
{
    boost::shared_ptr<CharHeroData> hd = m_heros.GetHero(hid);
    if (!hd.get())
    {
        return HC_ERROR;
    }
    //背包满了不能放
    if (m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    int ret = hd->unequipt(slot);
    return ret;
}

//装备东西
int CharData::useGem(int hid, int tid, int nums)
{
    boost::shared_ptr<CharHeroData> hd = m_heros.GetHero(hid);
    if (!hd.get())
    {
        return HC_ERROR;
    }
    int ret = hd->useGem(tid,nums);
    return ret;
}

int CharData::CompoundEquiptInfo(json_spirit::Object& obj, json_spirit::mObject& o)
{
    //铁匠铺
    boost::shared_ptr<char_smithy> cs = Singleton<cityMgr>::Instance().getCharSmithy(m_id);
    if (!cs.get())
    {
        return HC_ERROR;
    }
    int quality = 0, cnt = 0, update_equipt = 0;
    READ_INT_FROM_MOBJ(cnt,o,"count");
    READ_INT_FROM_MOBJ(update_equipt,o,"update_equipt");
    //背包里
    Equipment* up_equipt = m_bag.getEquipById(update_equipt);
    if (!up_equipt)
    {
        //英雄身上
        up_equipt = m_heros.getEquipById(update_equipt);
        if (!up_equipt)
        {
            return HC_ERROR;
        }
    }
    quality = up_equipt->getQuality();
    if (quality >= iMaxQuality)
    {
        ERR();
        return HC_ERROR;
    }
    json_spirit::Object equipt_obj;
    up_equipt->toObj(equipt_obj,up_equipt->getQuality()+1);
    obj.push_back( Pair("equipt_obj", equipt_obj));
    //基础加成和金币加成
    if (cnt >= 1 && cnt <= 8 && quality >= 1 && quality < iMaxQuality)
    {
        obj.push_back( Pair("base_per", iCompoundEquiptPer[quality-1][cnt-1]));
        obj.push_back( Pair("gold_cost", iCompoundEquiptGoldPer[quality-1][cnt-1][0]));
        obj.push_back( Pair("gold_per", iCompoundEquiptGoldPer[quality-1][cnt-1][1]));
        obj.push_back( Pair("silver_cost", up_equipt->getCompoundSilver()));
    }
    else
    {
        obj.push_back( Pair("base_per", 0.0));
        obj.push_back( Pair("gold_cost", 0));
        obj.push_back( Pair("gold_per", 0.0));
        obj.push_back( Pair("silver_cost", 0));
    }
    //种族加成
    if (m_race_data.get())
    {
        obj.push_back( Pair("race_per", m_race_data->m_compound_add));
    }
    //铁匠铺加成
    obj.push_back( Pair("city_per", cs->getCompoundAdd()) );
    return HC_SUCCESS;
}

//一键装满
int CharData::CompoundEquiptOneKey(json_spirit::Object& obj, json_spirit::mObject& o)
{
    //铁匠铺
    boost::shared_ptr<char_smithy> cs = Singleton<cityMgr>::Instance().getCharSmithy(m_id);
    if (!cs.get())
    {
        return HC_ERROR;
    }
    int eid = 0, quality = 0, cnt = 0, update_equipt = 0;
    READ_INT_FROM_MOBJ(update_equipt,o,"update_equipt");
    //背包里
    Equipment* up_equipt = m_bag.getEquipById(update_equipt);
    if (!up_equipt)
    {
        //英雄身上
        up_equipt = m_heros.getEquipById(update_equipt);
        if (!up_equipt)
        {
            return HC_ERROR;
        }
    }
    //确定类型和品质
    eid = up_equipt->getSubType();
    quality = up_equipt->getQuality();
    //合成需要最少两件装备且不能是顶级品质
    if (quality < 1 || quality >= iMaxQuality)
    {
        return HC_ERROR;
    }
    //计算最优
    double per = 0.0;
    //种族加成
    if (m_race_data.get())
    {
        per += m_race_data->m_compound_add;
    }
    //铁匠铺加成
    per += cs->getCompoundAdd();
    //从最大值开始算
    cnt = 8;
    while((iCompoundEquiptPer[quality-1][cnt-1] + per > 100.0 && cnt > 1) || (iCompoundEquiptPer[quality-1][cnt-1] < 1.0 && cnt > 1))
    {
        --cnt;
    }
    std::list<int> equipt_list;
    //从背包里筛选出装备列表
    m_bag.getBagEquipments(0, quality, eid, equipt_list);
    if (equipt_list.size())
    {
        int cur_cnt = 0;
        json_spirit::Array list;
        std::list<int>::iterator it = equipt_list.begin();
        while (it != equipt_list.end())
        {
            if (cur_cnt >= cnt)
            {
                break;
            }
            if (*it == update_equipt)
            {
                ++it;
                continue;
            }
            ++cur_cnt;
            json_spirit::Object tmp;
            tmp.push_back( Pair("id", *it));
            list.push_back(tmp);
            ++it;
        }
        obj.push_back( Pair("equipt_list", list));
    }
    return HC_SUCCESS;
}

int CharData::CompoundEquipt(json_spirit::Object& obj, json_spirit::mObject& o)
{
    //铁匠铺
    boost::shared_ptr<char_smithy> cs = Singleton<cityMgr>::Instance().getCharSmithy(m_id);
    if (!cs.get())
    {
        return HC_ERROR;
    }
    int eid = 0, quality = 0, cnt = 0, cost = 0, update_equipt = 0;
    READ_INT_FROM_MOBJ(cost,o,"cost");
    READ_INT_FROM_MOBJ(update_equipt,o,"update_equipt");
    //背包里
    Equipment* up_equipt = m_bag.getEquipById(update_equipt);
    if (!up_equipt)
    {
        //英雄身上
        up_equipt = m_heros.getEquipById(update_equipt);
        if (!up_equipt)
        {
            return HC_ERROR;
        }
    }
    eid = up_equipt->getSubType();
    quality = up_equipt->getQuality();
    std::list<int> equipt_list;
    json_spirit::mArray list;
    READ_ARRAY_FROM_MOBJ(list,o,"equipt_list");
    json_spirit::mArray::iterator it = list.begin();
    while (it != list.end())
    {
        if ((*it).type() != json_spirit::obj_type)
        {
            ++it;
            continue;
        }
        json_spirit::mObject& tmp_obj = (*it).get_obj();
        int tmp_id = 0;
        READ_INT_FROM_MOBJ(tmp_id,tmp_obj,"id");
        //背包里
        Equipment* equip = m_bag.getEquipById(tmp_id);
        if (!equip)
        {
            //英雄身上
            return HC_ERROR;
        }
        if (eid != equip->getSubType() || quality != equip->getQuality())
        {
            return HC_ERROR;
        }
        equipt_list.push_back(tmp_id);
        ++cnt;
        ++it;
    }
    //合成需要最少两件装备且不能是顶级品质
    if (cnt < 1 || cnt > 8 || quality < 1 || quality >= iMaxQuality)
    {
        return HC_ERROR;
    }
    double per = iCompoundEquiptPer[quality-1][cnt-1];
    if (cost && iCompoundEquiptGoldPer[quality-1][cnt-1][0])
    {
        per += iCompoundEquiptGoldPer[quality-1][cnt-1][1];
    }
    //种族加成
    if (m_race_data.get())
    {
        per += m_race_data->m_compound_add;
    }
    //铁匠铺加成
    per += cs->getCompoundAdd();
    INFO("compoundPer=" << per <<endl);
    if (silver() < up_equipt->getCompoundSilver())
    {
        return HC_ERROR_NOT_ENOUGH_SILVER;
    }
    if (cost && subGold(iCompoundEquiptGoldPer[quality-1][cnt-1][0], gold_cost_equipt_compound) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    if (subSilver(up_equipt->getCompoundSilver(), silver_cost_equipt_compound) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_SILVER;
    }
    //合成
    for(std::list<int>::iterator it_h = equipt_list.begin(); it_h != equipt_list.end(); ++it_h)
    {
        if (subEquipt(*it_h) != 0)
        {
            ERR();
            INFO("Sub eid=" << *it_h <<endl);
        }
    }
    if (my_random(0.0,100.0) < per)
    {
        //升品质
        up_equipt->updateQuality();
        obj.push_back( Pair("result", 1));
        m_tasks.updateTask(GOAL_COMPOUND_EQUIPT, up_equipt->getQuality(), 1);
        //广播好友祝贺
        if (up_equipt->getQuality() == 3)
        {
            Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_COMPOUND_QUALITY3_EQUIPMENT, 0, 0);
        }
        else if (up_equipt->getQuality() == 4)
        {
            Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_COMPOUND_QUALITY4_EQUIPMENT, 0, 0);
        }
        else if (up_equipt->getQuality() == 5)
        {
            Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_COMPOUND_QUALITY5_EQUIPMENT, 0, 0);
        }
    }
    else
    {
        obj.push_back( Pair("result", 0));
    }
    return HC_SUCCESS;
}

int CharData::UpgradeEquiptInfo(json_spirit::Object& obj, json_spirit::mObject& o)
{
    //铁匠铺
    boost::shared_ptr<char_smithy> cs = Singleton<cityMgr>::Instance().getCharSmithy(m_id);
    if (!cs.get())
    {
        return HC_ERROR;
    }
    int update_equipt = 0, level = 0;
    READ_INT_FROM_MOBJ(update_equipt,o,"upgrade_equipt");
    //背包里
    Equipment* up_equipt = m_bag.getEquipById(update_equipt);
    if (!up_equipt)
    {
        //英雄身上
        up_equipt = m_heros.getEquipById(update_equipt);
        if (!up_equipt)
        {
            return HC_ERROR;
        }
    }
    level = up_equipt->getLevel();
    if (level >= m_level)
    {
        ERR();
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    //装备信息
    json_spirit::Object equipt_obj;
    up_equipt->toObj(equipt_obj, 0, level+1);
    obj.push_back( Pair("equipt_obj", equipt_obj));
    boost::shared_ptr<baseUpgradeEquiptData> p = GeneralDataMgr::getInstance()->GetUpgradeEquiptData(level);
    if (!p.get())
    {
        return HC_ERROR;
    }
    obj.push_back( Pair("silver_cost", iUpgradeEquiptSilver*level) );
    //祝福值信息
    obj.push_back( Pair("bless_value", up_equipt->getBlessValue()) );
    obj.push_back( Pair("bless_max", p->m_bless_max) );
    //强化石信息
    obj.push_back( Pair("stone_need", p->m_need_stone));
    json_spirit::Object gem_obj;
    Item item(ITEM_TYPE_GEM, GEM_ID_UPGRADE_STONE, m_bag.getGemCount(GEM_ID_UPGRADE_STONE), 0);
    item.toObj(gem_obj);
    obj.push_back( Pair("gem_obj", gem_obj));
    return HC_SUCCESS;
}

int CharData::UpgradeEquipt(json_spirit::Object& obj, json_spirit::mObject& o)
{
    //铁匠铺
    boost::shared_ptr<char_smithy> cs = Singleton<cityMgr>::Instance().getCharSmithy(m_id);
    if (!cs.get())
    {
        return HC_ERROR;
    }
    int update_equipt = 0, level = 0, auto_upgrade = 0;
    READ_INT_FROM_MOBJ(update_equipt,o,"upgrade_equipt");
    READ_INT_FROM_MOBJ(auto_upgrade,o,"auto_upgrade");
    //背包里
    Equipment* up_equipt = m_bag.getEquipById(update_equipt);
    if (!up_equipt)
    {
        //英雄身上
        up_equipt = m_heros.getEquipById(update_equipt);
        if (!up_equipt)
        {
            return HC_ERROR;
        }
    }
    level = up_equipt->getLevel();
    if (level >= m_level)
    {
        ERR();
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    boost::shared_ptr<baseUpgradeEquiptData> p = GeneralDataMgr::getInstance()->GetUpgradeEquiptData(level);
    if (!p.get())
    {
        return HC_ERROR;
    }
    if (silver() < iUpgradeEquiptSilver*level)
    {
        return HC_ERROR_NOT_ENOUGH_SILVER;
    }
    int gem_cnt = m_bag.getGemCount(GEM_ID_UPGRADE_STONE);
    int cost_gold = 0;
    if (auto_upgrade == 0)
    {
        if (gem_cnt < p->m_need_stone)
        {
            return HC_ERROR_NOT_ENOUGH_GEM;
        }
    }
    else
    {
        if (gem_cnt < p->m_need_stone)
        {
            boost::shared_ptr<baseGem> bt = GeneralDataMgr::getInstance()->GetBaseGem(GEM_ID_UPGRADE_STONE);
            if (!bt.get())
            {
                return HC_ERROR;
            }
            cost_gold = (p->m_need_stone - gem_cnt) * bt->gold_to_buy;
            if (gold() < cost_gold)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
        }
    }
    double fail_per = (double)p->m_fail_per;
    obj.push_back( Pair("bless_value", up_equipt->getBlessValue()) );
    obj.push_back( Pair("bless_max", p->m_bless_max) );
    //祝福影响
    int bless = up_equipt->getBlessValue();
    bless = bless > p->m_bless_max ? p->m_bless_max : bless;
    fail_per -= ((double)(bless / p->m_bless_value) * p->m_bless_per);
    fail_per = fail_per < 0.0 ? 0.0 : fail_per;
    INFO("upgradeFailPer=" << fail_per <<endl);
    //扣资源
    subSilver(iUpgradeEquiptSilver*level, silver_cost_equipt_upgrade);
    //扣道具
    if (cost_gold > 0)
    {
        subGem(GEM_ID_UPGRADE_STONE,gem_cnt,gem_cost_equipt_upgrade);
        subGold(cost_gold,gold_cost_equipt_upgrade);
    }
    else
    {
        subGem(GEM_ID_UPGRADE_STONE,p->m_need_stone,gem_cost_equipt_upgrade);
    }
    m_score_tasks.updateTask(DAILY_SCORE_UPGRADE_EQUIPT);
    int result = 0;
    if (my_random(0.0,100.0) > fail_per)
    {
        //升等级
        up_equipt->clearBlessValue();
        up_equipt->updateLevel(level + 1);
        result = 1;
        m_tasks.updateTask(GOAL_UPGRADE_EQUIPT, up_equipt->getLevel(), 1);
        Singleton<goalMgr>::Instance().updateTask(m_id, GOAL_TYPE_EQUIPT1_UPGRADE+up_equipt->getEquipType()-1, up_equipt->getLevel());
        //装备强化，好友祝贺
        if (up_equipt->getLevel() > 10 && up_equipt->getLevel() % 10 == 0)
        {
            Singleton<relationMgr>::Instance().postCongradulation(m_id, CONGRATULATION_UPGRADE_EQUIPMENT, up_equipt->getLevel(), 0);
        }
    }
    else
    {
        if ((bless + 5) > p->m_bless_max)
        {
            up_equipt->addBlessValue(p->m_bless_max-5);
        }
        else
        {
            up_equipt->addBlessValue(5);
        }
        up_equipt->Save();
    }
    obj.push_back( Pair("result", result));
    obj.push_back( Pair("silver_cost", iUpgradeEquiptSilver*level));
    obj.push_back( Pair("gold_cost", cost_gold));
    //装备信息
    json_spirit::Object equipt_obj;
    up_equipt->toObj(equipt_obj);
    obj.push_back( Pair("equipt_obj", equipt_obj));
    m_heros.updateAttribute();
    m_tasks.updateTask(GOAL_DAILY_UPGRADE_EQUIPT, 0, 1);
    Singleton<goalMgr>::Instance().updateTask(m_id, GOAL_TYPE_UPGRADE_EQUIPT, 1);
    return HC_SUCCESS;
}

int CharData::ShowBaoshis(json_spirit::Object& obj, json_spirit::mObject& o)
{
    //窗口展现类型，仅帮客户端储存返回，不处理
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    obj.push_back( Pair("type", type));
    int cur_nums = 0;
    json_spirit::Array lists;
    int purpose = 0, cid = 0, eid = 0;
    READ_INT_FROM_MOBJ(purpose, o, "purpose");//1装备宝石，2背包宝石
    READ_INT_FROM_MOBJ(cid, o, "cid");
    READ_INT_FROM_MOBJ(eid, o, "eid");
    obj.push_back( Pair("purpose", purpose) );
    obj.push_back( Pair("cid", cid) );
    obj.push_back( Pair("eid", eid) );
    if (purpose == 1)
    {
        //查询具体某个装备的时候支持查别人
        if (cid > 0 && eid > 0)
        {
            CharData* pcc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (pcc)
            {
                Equipment* edata = pcc->m_bag.getEquipById(eid);
                if (edata)
                {
                    edata->m_bag.showBagBaoshis(lists, o, cur_nums);
                    obj.push_back( Pair("bagSize", edata->m_bag.getSize()) );
                    obj.push_back( Pair("bagMaxSize", edata->maxBaoshiSlot()) );
                }
            }
        }
    }
    if (purpose == 2)
    {
        //背包里的宝石
        m_bag.showBagBaoshis(lists, o, cur_nums);
    }
    obj.push_back( Pair("list", lists) );
    //页面信息
    int page = 1, nums_per_page = 8;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 8;
    }
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    obj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

GeneralDataMgr* GeneralDataMgr::m_handle = NULL;

GeneralDataMgr::GeneralDataMgr()
{
    m_inited = false;

    rwlock_init(&guild_chanel_rwmutex);
    rwlock_init(&onlineuser_rwmutex);

    rwlock_init(&onlinechar_rwmutex);
    rwlock_init(&globalchar_rwmutex);
}

GeneralDataMgr::~GeneralDataMgr()
{
    if (m_world_channel.get())     //世界聊天
    {
        m_world_channel->stop();
    }
    if (m_horn_channel.get())     //喇叭聊天
    {
        m_horn_channel->stop();
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

//加载基础数据
int GeneralDataMgr::reloadOtherBaseData()
{
    //角色缺省金币
    g_default_gold = getInt("default_gold", DEFAULT_GOLD);
    //角色缺省筹码
    g_default_silver = getInt("default_silver", DEFAULT_SILVER);
    //是否允许调试命令行
    g_enable_debug_cmd_line = getInt("enable_debug_cmdline", 0);
    Query q(GetDb());
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

    //基础经验
    q.get_result("SELECT level,exp FROM base_exp WHERE 1 order by level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        int exp = q.getval();
        m_base_exps[level] = exp;
    }
    q.free_result();

    q.get_result("SELECT level,fail_per,need_stone,bless_max,bless_value,bless_per FROM base_upgrade_equipt_data WHERE 1 order by level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        boost::shared_ptr<baseUpgradeEquiptData> p;
        p.reset(new baseUpgradeEquiptData);
        p->m_level = level;
        p->m_fail_per = q.getval();
        p->m_need_stone = q.getval();
        p->m_bless_max = q.getval();
        p->m_bless_value = q.getval();
        p->m_bless_per = q.getnum();
        m_base_upgrade_equipt_data[level] = p;
    }
    q.free_result();

    //基础种族
    q.get_result("SELECT id,name,resident_add,silver_add,hp_add,magic_add,compound_add FROM base_race WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int type = q.getval();
        boost::shared_ptr<baseRaceData> p_rd;
        p_rd.reset(new baseRaceData);
        p_rd->m_type = type;
        p_rd->m_name = q.getstr();
        p_rd->m_resident_add = q.getval();
        p_rd->m_silver_add = q.getval();
        p_rd->m_hp_add = q.getval();
        p_rd->m_mag_add = q.getval();
        p_rd->m_compound_add = q.getval();
        memset(p_rd->m_attak_add, 0, sizeof(int)*iMaxLevel);
        memset(p_rd->m_defense_add, 0, sizeof(int)*iMaxLevel);
        memset(p_rd->m_magic_add, 0, sizeof(int)*iMaxLevel);
        m_base_races[type] = p_rd;
    }
    q.free_result();
    q.get_result("SELECT id,level,attack_add,defense_add,magic_add FROM base_race_extra WHERE 1 order by id,level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int type = q.getval();
        int level = q.getval();
        int attak_add = q.getval();
        int defense_add = q.getval();
        int magic_add = q.getval();
        boost::shared_ptr<baseRaceData> p_rd = GetBaseRace(type);
        if (p_rd.get())
        {
            if (level > 0 && level <= iMaxLevel)
            {
                p_rd->m_attak_add[level-1] = attak_add;
                p_rd->m_defense_add[level-1] = defense_add;
                p_rd->m_magic_add[level-1] = magic_add;
            }
        }
        else
        {
            ERR();
        }
    }
    q.free_result();

    //等级战斗加成
    q.get_result("SELECT level,need_char_exp,reward_add,guide_id FROM base_level_add WHERE 1 order by level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        boost::shared_ptr<baseLevelData> p_ld;
        p_ld.reset(new baseLevelData);
        p_ld->m_level = level;
        p_ld->m_need_char_exp = q.getval();
        p_ld->m_reward_add = q.getnum();
        p_ld->m_guide_id = q.getval();
        m_base_levels[level] = p_ld;
    }
    q.free_result();

    cout<<"************ GeneralDataMgr::reloadOtherBaseData() base_equipment ******************"<<endl;
    //初始化基础装备信息
    q.get_result("SELECT id,type,name,quality,value,price,memo FROM base_equipment WHERE 1 order by id");
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
        eq->id = id;
        eq->spic = eq->id;
        eq->type = q.getval();
        eq->name = q.getstr();
        eq->quality = q.getval();
        eq->value = q.getval();
        eq->sellPrice = q.getval();
        eq->memo = q.getstr();
    }
    q.free_result();

    int qualitys[MAX_BAOSHI_LEVEL] = {1,2,3,4,5,6};
    //初始化基础装备信息
    q.get_result("SELECT id,type,name,memo,lv1,lv2,lv3,lv4,lv5,lv6 from base_baoshi where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseBaoshi> pb = GetBaseBaoshi(id);
        if (!pb.get())
        {
            pb.reset(new baseBaoshi);
        }
        m_base_baoshis[id] = pb;
        pb->id = id;
        pb->type = q.getval();
        pb->name = q.getstr();
        pb->memo = q.getstr();
        for (int i = 0; i < MAX_BAOSHI_LEVEL; ++i)
        {
            pb->values[i] = q.getval();
        }
        memcpy(pb->qualitys, qualitys, sizeof(int)*MAX_BAOSHI_LEVEL);
    }
    q.free_result();

    cout<<"************ GeneralDataMgr::reloadOtherBaseData() base_gem ******************"<<endl;
    //基础道具
    q.get_result("SELECT id,spic,type,quality,max_count,sellPrice,name,memo,gold_to_buy,gold_vip,invalidTime,currency,auction,value1,value2,value3 FROM base_gem WHERE inUse=1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseGem> tr = GetBaseGem(id);
        if (!tr.get())
        {
            tr.reset(new baseGem);
        }
        m_base_gems[id] = tr;
        tr->id = id;
        tr->spic = q.getval();
        tr->usage = q.getval();
        tr->quality = q.getval();
        tr->max_size = q.getval();
        tr->sellPrice = q.getval();
        tr->name = q.getstr();
        tr->memo = q.getstr();
        tr->gold_to_buy = q.getval();
        tr->gold_vip = q.getval();
        tr->invalidTime = q.getval();
        tr->currency = (q.getval() > 0);
        tr->auction = (q.getval() > 0);
        tr->extra[0] = q.getval();
        tr->extra[1] = q.getval();
        tr->extra[2] = q.getval();
    }
    q.free_result();
    //基础声望
    q.get_result("SELECT level,need_exp FROM base_prestige WHERE 1 order by level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        boost::shared_ptr<basePrestigeData> p_ld;
        p_ld.reset(new basePrestigeData);
        p_ld->m_level = level;
        p_ld->m_need_exp = q.getval();
        m_base_prestiges[level] = p_ld;
    }
    q.free_result();
    //基础声望奖励
    q.get_result("select race,aid,need_prestige_level,itemType,itemId,counts,extra from base_prestige_award where 1 order by race,aid asc");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int race = q.getval();
        boost::shared_ptr<PrestigeAwards> pa = m_base_prestige_awards[race];
        if (!pa.get())
        {
            pa.reset(new PrestigeAwards);
            m_base_prestige_awards[race] = pa;
        }
        int aid = q.getval();
        boost::shared_ptr<basePrestigeAward> bpa = (*(pa.get()))[aid];
        if (!bpa.get())
        {
            bpa.reset(new basePrestigeAward);
            (*(pa.get()))[aid] = bpa;
        }
        bpa->id = aid;
        bpa->race = race;
        bpa->need_prestige_level = q.getval();
        bpa->m_item.type = q.getval();
        bpa->m_item.id = q.getval();
        bpa->m_item.nums = q.getval();
        bpa->m_item.extra = q.getval();
    }
    q.free_result();
    //基础声望商品
    q.get_result("select race,aid,need_prestige_level,cost,itemType,itemId,counts,extra from base_prestige_shop where 1 order by race,aid asc");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int race = q.getval();
        boost::shared_ptr<PrestigeGoods> pa = m_base_prestige_goods[race];
        if (!pa.get())
        {
            pa.reset(new PrestigeGoods);
            m_base_prestige_goods[race] = pa;
        }
        int aid = q.getval();
        boost::shared_ptr<basePrestigeGoods> bpa = (*(pa.get()))[aid];
        if (!bpa.get())
        {
            bpa.reset(new basePrestigeGoods);
            (*(pa.get()))[aid] = bpa;
        }
        bpa->id = aid;
        bpa->race = race;
        bpa->need_prestige_level = q.getval();
        bpa->silver = q.getval();
        bpa->m_item.type = q.getval();
        bpa->m_item.id = q.getval();
        bpa->m_item.nums = q.getval();
        bpa->m_item.extra = q.getval();
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
        //加载开服时间
        server_open_time = getInt("server_open_time");
        if (server_open_time == 0)
        {
            server_open_time = time(NULL);
            setInt("server_open_time",server_open_time);
        }

        //加载其他基础数据
        reloadOtherBaseData();

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
        //开启时设置全部都不在线
        q.execute("update admin_char set cstat='0' where 1");
        CHECK_DB_ERR(q);
        //清除所有昵称，对应排行模块加载的时候会处理
        q.execute("update charactors set nick='[]'");

        //加载各唯一id最大值
        m_hero_id = 0;
        q.get_result("select max(id) from char_heros");
        if (q.GetErrno())
        {
            CHECK_DB_ERR(q);
            exit(1);
        }
        if (q.fetch_row())
        {
            m_hero_id = q.getval();
        }
        q.free_result();

        m_gem_id = 0;
        q.get_result("select max(id) from char_gem");
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

        m_baoshi_id = 0;
        q.get_result("select max(id) from char_baoshi");
        if (q.GetErrno())
        {
            CHECK_DB_ERR(q);
            exit(1);
        }
        if (q.fetch_row())
        {
            m_baoshi_id = q.getval();
        }
        q.free_result();

        m_combat_id = 0;
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
        q.get_result("select id,cid from char_heros where 1");
        while (q.fetch_row())
        {
            int hid = q.getval();
            int cid = q.getval();
            m_hero_map[hid] = cid;
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

        m_inited = true;
        return 0;
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
        //喇叭频道
        m_handle->GetHornChannel();
        //阵营频道
        for (size_t i = 1; i <= 4; ++i)
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

void GeneralDataMgr::setHeroOwner(int hid, int cid)
{
    m_hero_map[hid] = cid;
}

int GeneralDataMgr::getHeroOwner(int hid)
{
    if (m_hero_map.find(hid) != m_hero_map.end())
    {
        return m_hero_map[hid];
    }
    return 0;
}

void GeneralDataMgr::removeHeroOwner(int hid)
{
    if (m_hero_map.find(hid) != m_hero_map.end())
    {
        m_hero_map.erase(hid);
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

//获得在线挑战列表
int GeneralDataMgr::GetOnlineChallengeList(const std::string& char_name, int level, json_spirit::Array& list)
{
    for (int i = -1; i >= -3; --i)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("id", i) );
        obj.push_back( Pair("name", "X-Man" + LEX_CAST_STR(i)) );
        obj.push_back( Pair("race", my_random(1,4)) );
        obj.push_back( Pair("level", level) );
        obj.push_back( Pair("atk", 10*level) );  //暂时设个数值
        obj.push_back( Pair("def", 5*level) );
        obj.push_back( Pair("magic", 2*level) );
        obj.push_back( Pair("spic", my_random(1,4)) );
        list.push_back(obj);
    }

    readLock lockit(&onlinechar_rwmutex);
    //根据角色名找到已经登录的角色
    std::map<std::string, boost::shared_ptr<OnlineCharactor> >::iterator it = m_online_charactors.begin();
    while (it != m_online_charactors.end())
    {
        if (it->second.get() && it->second->m_charactor.get() && it->second->m_charactor->m_name != char_name
            && it->second->m_charactor->m_level >= (level-5)
            && it->second->m_charactor->m_level <= (level+5))
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->second->m_charactor->m_id) );
            obj.push_back( Pair("name", it->second->m_charactor->m_name) );
            obj.push_back( Pair("race", it->second->m_charactor->m_race) );
            obj.push_back( Pair("level", it->second->m_charactor->m_level) );
            obj.push_back( Pair("atk", 10*it->second->m_charactor->m_level) );  //暂时设个数值
            obj.push_back( Pair("def", 5*it->second->m_charactor->m_level) );
            obj.push_back( Pair("magic", 2*it->second->m_charactor->m_level) );
            obj.push_back( Pair("spic", it->second->m_charactor->m_spic) );
            list.push_back(obj);
            if (list.size() >= 10)
            {
                break;
            }
        }
        ++it;
    }

    return 0;
}

//获得道具列表
void GeneralDataMgr::GetGemList(json_spirit::mObject& o, json_spirit::Object& robj)
{
    int type = 0, usage = 1, buy = 1;
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_INT_FROM_MOBJ(usage, o, "usage");
    READ_INT_FROM_MOBJ(buy, o, "buy");
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("usage", usage) );
    robj.push_back( Pair("buy", buy) );
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<baseGem> >::iterator it = m_base_gems.begin();
    while (it != m_base_gems.end())
    {
        if (it->second.get() && usage == it->second->usage)
        {
            if (type > 0 && type != it->second->id)
            {
                ++it;
                continue;
            }
            if ((buy && it->second->gold_to_buy > 0) || buy == 0)
            {
                json_spirit::Object trobj;
                trobj.push_back( Pair("type", it->second->id) );
                trobj.push_back( Pair("name", it->second->name) );
                trobj.push_back( Pair("spic", it->second->spic) );
                trobj.push_back( Pair("quality", it->second->quality) );
                int price = it->second->sellPrice;
                trobj.push_back( Pair("price", price) );
                trobj.push_back( Pair("gold", it->second->gold_to_buy) );
                trobj.push_back( Pair("memo", it->second->memo) );
                list.push_back(trobj);
            }
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    return;
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

//获得等级需要经验
int GeneralDataMgr::GetBaseExp(int level)
{
    std::map<int, int>::iterator it = m_base_exps.find(level);
    if (it != m_base_exps.end())
    {
        return it->second;
    }
    else
    {
        return -1;
    }
}

boost::shared_ptr<baseUpgradeEquiptData> GeneralDataMgr::GetUpgradeEquiptData(int level)
{
    std::map<int, boost::shared_ptr<baseUpgradeEquiptData> >::iterator it = m_base_upgrade_equipt_data.find(level);
    if (it != m_base_upgrade_equipt_data.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<baseUpgradeEquiptData> p;
    p.reset();
    return p;
}

//获取基础种族信息
boost::shared_ptr<baseRaceData> GeneralDataMgr::GetBaseRace(int type)
{
    std::map<int, boost::shared_ptr<baseRaceData> >::iterator it = m_base_races.find(type);
    if (it != m_base_races.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<baseRaceData> p;
    p.reset();
    return p;
}

//获取等级信息
boost::shared_ptr<baseLevelData> GeneralDataMgr::GetLevelData(int level)
{
    std::map<int, boost::shared_ptr<baseLevelData> >::iterator it = m_base_levels.find(level);
    if (it != m_base_levels.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<baseLevelData> p;
    p.reset();
    return p;
}

//获得基础道具
boost::shared_ptr<baseGem> GeneralDataMgr::GetBaseGem(int tid)
{
    std::map<int, boost::shared_ptr<baseGem> >::iterator it = m_base_gems.find(tid);
    if (it != m_base_gems.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseGem> gd;
        gd.reset();
        return gd;
    }
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

//获得基础宝石
boost::shared_ptr<baseBaoshi> GeneralDataMgr::GetBaseBaoshi(int baseid)
{
    std::map<int, boost::shared_ptr<baseBaoshi> >::iterator it = m_base_baoshis.find(baseid);
    if (it != m_base_baoshis.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseBaoshi> gd;
        gd.reset();
        return gd;
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
int GeneralDataMgr::Login(const std::string& qid, const std::string& account, int isAdult, int union_id, const std::string& server_id, net::session_ptr csocket, Object& robj)
#endif
{
    {
        readLock lockit(&onlineuser_rwmutex);
        if (m_onlineuserlist.size() > 4321)
        {
            return 8888;
        }
    }

    //判断账号是否被冻结
    Query q(GetDb());
    q.get_result("select count(*) from admin_scoll where endtime>unix_timestamp() and account='" + GetDb().safestr(account) + "'");
    if (q.fetch_row() && q.getval() > 0)
    {
        q.free_result();
        return HC_ERROR;
    }
    q.free_result();

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
        {
            writeLock lockit(&onlineuser_rwmutex);
            m_onlineuserlist[account] = paccount;
        }
        INFO("account login "<<account<<endl);
        q.get_result("SELECT id FROM `charactors` WHERE account='" + GetDb().safestr(account) + "' order by id limit 1");
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            paccount->m_cid = q.getval();
        }
        q.free_result();
    }
    paccount->m_isAdult = isAdult;
#ifdef QQ_PLAT
    paccount->m_qq_yellow_year = is_year;
    paccount->m_qq_yellow_level = qq_yellow;
    paccount->m_iopenid = iopenid;
    paccount->m_feedid = feedid;
    paccount->m_login_str1 = str1;
    paccount->m_login_str2 = str2;
#endif

    csocket->user(paccount);
    //直接登录了，不用选角色

    INFO(account<<" -> cid "<<paccount->m_cid<<endl);
    if (paccount->m_cid > 0)
    {
        int ret = paccount->Login(paccount->m_cid);
        if (HC_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            CharData* cdata = paccount->m_onlineCharactor->m_charactor.get();
            if (!cdata)
            {
                ERR();
                return HC_ERROR;
            }

            //加入角色等级信息、黄金、白银、军令
            json_spirit::Object info;
            cdata->getRoleInfo(info);
            robj.push_back( Pair("info", info) );
            if (cdata->m_current_guide)
            {
                cdata->checkGuide(cdata->m_current_guide);
            }
        }
    }
    return HC_SUCCESS;
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
        ch.reset(new ChatChannel("guild", guild_id, "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_guild)+",\"s\":200,"));
        ch->start();
        m_guild_channels[guild_id] = ch;
        INFO("GeneralDataMgr::GetGuildChannel("<<guild_id<<") new a channel");
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
        ch.reset(new ChatChannel("camp", camp, "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_race)+",\"s\":200,\"camp\":" + LEX_CAST_STR(camp) + ","));
        ch->start();
        writeLock lockit(&camp_chanel_rwmutex);
        m_camp_channels[camp] = ch;
        INFO("GeneralDataMgr::GetCampChannel("<<camp<<") new a channel");
        return ch;
    }
}

boost::shared_ptr<ChatChannel> GeneralDataMgr::GetWorldChannel()
{
    if (m_world_channel.get())
    {
        return m_world_channel;
    }
    else
    {
        m_world_channel.reset(new ChatChannel("world", 1, "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_world)+",\"s\":200,"));
        m_world_channel->start();
        return m_world_channel;
    }
}

boost::shared_ptr<ChatChannel> GeneralDataMgr::GetHornChannel()
{
    if (m_horn_channel.get())
    {
        return m_horn_channel;
    }
    else
    {
        m_horn_channel.reset(new ChatChannel("horn", 1, "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_horn)+",\"s\":200,"));
        m_horn_channel->start();
        return m_horn_channel;
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

int GeneralDataMgr::CharactorLogin(boost::shared_ptr<OnlineCharactor> oc)
{
    if (oc.get() && oc->m_account.get() && oc->m_charactor.get())
    {
        writeLock lockit(&onlinechar_rwmutex);
#ifdef QQ_PLAT
        cout<<"charactor login "<<oc->m_account->m_qid<<","<<oc->m_account->m_qq_yellow_level<<","<<oc->m_account->m_qq_yellow_year<<"| cid "<<oc->m_charactor->m_id<<endl;
        oc->m_charactor->m_qq_yellow_level = oc->m_account->m_qq_yellow_level;
        oc->m_charactor->m_qq_yellow_year = oc->m_account->m_qq_yellow_year;
        oc->m_charactor->m_iopenid = oc->m_account->m_iopenid;
        oc->m_charactor->m_feedid = oc->m_account->m_feedid;
        oc->m_charactor->m_login_str1 = oc->m_account->m_login_str1;
        oc->m_charactor->m_login_str2 = oc->m_account->m_login_str2;
#endif
        m_online_charactors[oc->m_charactor->m_name] = oc;
        lockit.unlock();
        //加入世界频道
        if (m_world_channel.get())
        {
            m_world_channel->Add(oc);
        }
        //加入喇叭频道
        if (m_horn_channel.get())
        {
            m_horn_channel->Add(oc);
        }
        //加入阵营频道
        boost::shared_ptr<ChatChannel> ach = GetCampChannel(oc->m_charactor->m_race);
        if (ach.get())
        {
            ach->Add(oc);
        }
        cout<<"charactor login "<<oc->m_account->m_qid<<",gid="<<oc->m_charactor->GetGuildId()<<endl;
        //加入军团频道
        if (oc->m_charactor->GetGuildId() > 0)
        {
            boost::shared_ptr<ChatChannel> gch = GetGuildChannel(oc->m_charactor->GetGuildId());
            if (gch.get())
            {
                gch->Add(oc);
            }
        }
        time_t time_now = time(NULL);

        //登录间隔超过一天有找回功能
        int days_passed = (time_now - oc->m_charactor->m_login_time)/(24*3600);
        if (days_passed >= 1)
        {
            //最后一次登录在前一天0点之前
            time_t z = getZeroTime() - iONE_DAY_SECS;
            if (oc->m_charactor->m_login_time < z)
            {
                oc->m_charactor->setExtraData(char_data_type_daily, char_data_daily_findback, 1);
            }
        }

        //更新登录时间
        oc->m_charactor->m_login_time = time_now;
        oc->m_charactor->m_is_online = 1;
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

        std::string msg = "";//广播
        if (oc->m_charactor->m_vip > 5 && oc->m_charactor->m_nick.m_nick_list.size())
        {
            msg = strLoginMsg1;
            str_replace(msg, "$N", LEX_CAST_STR(oc->m_charactor->m_vip));
        }
        else if (oc->m_charactor->m_vip > 5)
        {
            msg = strLoginMsg2;
            str_replace(msg, "$N", LEX_CAST_STR(oc->m_charactor->m_vip));
        }
        else if (oc->m_charactor->m_nick.m_nick_list.size())
        {
            msg = strLoginMsg3;
        }
        if (msg != "")
        {
            str_replace(msg, "$W", MakeCharNameLink(oc->m_charactor->m_name,oc->m_charactor->m_nick.get_string()));
            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }
        //重置每日任务
        oc->m_charactor->m_tasks.dailyUpdate();
        //活动更新
        if (Singleton<actionMgr>::Instance().isSevenActionOpen(oc->m_charactor.get()))
        {
            Singleton<actionMgr>::Instance().updateSevenAction(oc->m_charactor.get());
        }
        //加入筹码pk
        {
            Singleton<PkMgr>::Instance().addCharactor(oc->m_charactor->m_id);
        }
        //在线礼包处理
        char_online_libao_data* e = Singleton<actionMgr>::Instance().getCharOnlineLibaoData(oc->m_charactor->m_id);
        if (e)
        {
            //在线礼包处理
            e->updateState(1);
        }
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
        //离开喇叭频道
        if (m_horn_channel.get())
        {
            m_horn_channel->Remove(oc);
        }
        //离开阵营频道
        boost::shared_ptr<ChatChannel> ach = GetCampChannel(oc->m_charactor->m_race);
        if (ach.get())
        {
            ach->Remove(oc);
        }
        //离开军团频道
        if (oc->m_charactor->GetGuildId() > 0)
        {
            boost::shared_ptr<ChatChannel> gch = GetGuildChannel(oc->m_charactor->GetGuildId());
            if (gch.get())
            {
                gch->Remove(oc);
            }
        }
        //下线开始探索洞穴
        boost::shared_ptr<charExplore> ce = Singleton<exploreMgr>::Instance().getCharExploreData(oc->m_charactor->m_id);
        if (ce.get())
        {
            ce->start();
        }
        //在线礼包处理
        char_online_libao_data* e = Singleton<actionMgr>::Instance().getCharOnlineLibaoData(oc->m_charactor->m_id);
        if (e)
        {
            e->updateState(3);
        }
        //有战斗状态则弃权
        chessCombat* myCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(oc->m_charactor->m_id);
        if (myCombat)
        {
            myCombat->quit(oc->m_charactor->m_id);
        }
        multi_combat* myMCombat = Singleton<MultiCombatMgr>::Instance().getCharMultiCombat(oc->m_charactor->m_id);
        if (myMCombat)
        {
            myMCombat->quit(oc->m_charactor->m_id);
        }
        //离开筹码pk
        {
            boost::shared_ptr<CharPkData> charPk = Singleton<PkMgr>::Instance().getPkData(oc->m_charactor->m_id);
            if (charPk.get() && charPk->getCharData().get() && charPk->m_roomid > 0)
            {
                //先离开频道
                boost::shared_ptr<ChatChannel> rch = Singleton<PkMgr>::Instance().GetRoomChannel(charPk->m_roomid);
                if (rch.get())
                {
                    rch->Remove(oc);
                }
                //再离开房间
                Singleton<PkMgr>::Instance().leaveRoom(oc->m_charactor->m_id);
            }
            //删除角色pk场管理
            Singleton<PkMgr>::Instance().removeCharactor(oc->m_charactor->m_id);
        }
        time_t time_now = time(NULL);
        //下线更新每日在线时间
        int daily_online = oc->m_charactor->getTodayOnlineTime();
        oc->m_charactor->setExtraData(char_data_type_daily, char_data_daily_online_time, daily_online);
        oc->m_charactor->m_login_time = time_now;
        oc->m_charactor->Save();
        oc->m_charactor->m_is_online = 0;

        //更新在线统计
        dbCmd _dbcmd;
        _dbcmd._cid = oc->m_charactor->m_id;
        _dbcmd._cmd = db_cmd_char_offline;
        InsertDbCharCmd(_dbcmd);
    }
    return 0;
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
    return m_onlineuserlist.size();
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

int GeneralDataMgr::dailyUpdate()
{
#ifdef GLOBAL_CHAR_LOCK
    readLock lockit(&globalchar_rwmutex);
#endif
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get())
        {
            //清空沉迷时间
            //it->second->m_chenmi_time = 0;
            //it->second->m_chenmi_start_time = time(NULL);
            //每日数据
            it->second->clearExtraData(char_data_type_daily);
            it->second->clearLootTimes();
            //重置每日必做
            it->second->m_score_tasks.dailyUpdate();
            //重置每日任务
            it->second->m_tasks.dailyUpdate();
        }
        ++it;
    }
    //删除过期的战报
    InsertSaveDb("delete from battle_records where archive=0 and input<unix_timestamp()");
    //清空沉迷时间
    InsertSaveDb("update charactors set chenmi_time=0 where 1");
    //每日数据清除
    InsertSaveDb("delete from `char_data_extra` where type=1");
    InsertSaveDb("truncate `char_loot_times` where 1");
    //邮件保留15天
    InsertSaveDb("delete from char_mails where cid>0 and archive=0 and unix_timestamp(input) < (unix_timestamp()-1296000)");
    //过期登录数据
    InsertSaveDb("delete from api_login where unix_timestamp(input)+3600<unix_timestamp()");
    return HC_SUCCESS;
}

int GeneralDataMgr::weekUpdate()
{
    cout<<"****** reset week data ******"<<endl;
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get())
        {
            //每周数据
            it->second->clearExtraData(char_data_type_week);
        }
        ++it;
    }
    //每周数据清除
    InsertSaveDb("delete from `char_data_extra` where type=2");
    Singleton<shenlingMgr>::Instance().weekUpdate();
    Singleton<weekRankings>::Instance().RankingsReward();
    return 0;
}

int GeneralDataMgr::dailyOnlineChar()
{
#ifdef GLOBAL_CHAR_LOCK
    readLock lockit(&globalchar_rwmutex);
#endif
    std::map<int, boost::shared_ptr<CharData> >::iterator it = m_chardata_map.begin();
    while (it != m_chardata_map.end())
    {
        if (it->second.get())
        {
            //重新通知顶栏按钮
            it->second->NotifyCharTopButtonList();
        }
        ++it;
    }
    return HC_SUCCESS;
}

boost::shared_ptr<basePrestigeData> GeneralDataMgr::GetPrestigeData(int level)
{
    std::map<int, boost::shared_ptr<basePrestigeData> >::iterator it = m_base_prestiges.find(level);
    if (it != m_base_prestiges.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<basePrestigeData> p;
    p.reset();
    return p;
}

boost::shared_ptr<PrestigeAwards> GeneralDataMgr::GetBasePrestigeAward(int race)
{
    if (m_base_prestige_awards.find(race) != m_base_prestige_awards.end())
    {
        return m_base_prestige_awards[race];
    }
    boost::shared_ptr<PrestigeAwards> tmp;
    tmp.reset();
    return tmp;
}

boost::shared_ptr<PrestigeGoods> GeneralDataMgr::GetBasePrestigeGoods(int race)
{
    if (m_base_prestige_goods.find(race) != m_base_prestige_goods.end())
    {
        return m_base_prestige_goods[race];
    }
    boost::shared_ptr<PrestigeGoods> tmp;
    tmp.reset();
    return tmp;
}

//好友推送
bool GeneralDataMgr::canRecommendFriends(CharData* cdata)
{
    readLock lockit(&onlineuser_rwmutex);
    //等级大于自己的玩家
    for (std::map<std::string, boost::shared_ptr<OnlineCharactor> >::iterator it = m_online_charactors.begin(); it != m_online_charactors.end(); ++it)
    {
        if (it->second.get() && it->second.get()->m_charactor.get())
        {
            CharData* c = it->second.get()->m_charactor.get();
            if (cdata == c || cdata->m_level > c->m_level || (cdata->m_level + 5) < c->m_level || Singleton<relationMgr>::Instance().is_my_attention(cdata->m_id, c->m_id))
            {
                continue;
            }
            return true;
        }
    }
    return false;
}

//好友推送
void GeneralDataMgr::getRecommendFriends(CharData* cdata, std::map<int, boost::shared_ptr<CharData> >& char_list)
{
    char_list.clear();
    readLock lockit(&onlineuser_rwmutex);
    //等级大于自己的玩家
    for (std::map<std::string, boost::shared_ptr<OnlineCharactor> >::iterator it = m_online_charactors.begin(); it != m_online_charactors.end(); ++it)
    {
        if (it->second.get() && it->second.get()->m_charactor.get())
        {
            CharData* c = it->second.get()->m_charactor.get();
            if (cdata == c || cdata->m_level > c->m_level
                || (cdata->m_level + 5) < c->m_level
                || Singleton<relationMgr>::Instance().is_my_attention(cdata->m_id, c->m_id))
            {
                continue;
            }
            char_list[c->m_id] = it->second.get()->m_charactor;
        }
        if (char_list.size() >= 30)
        {
            break;
        }
    }
}

//创建角色
int CreateChar(const std::string& account, int union_id, const std::string& server_id,  const std::string& qid, int race, int spic, const std::string& xname, uint64_t& cid)
{
    std::string name = GetDb().safestr(xname);
    if (name != xname)
    {
        return HC_ERROR_NAME_ILLEGAL;
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

    cout<<"createChar--> new "<<cid<<",tid:"<<syscall(SYS_gettid)<<endl;
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
        p_chardata->m_race = race;
        p_chardata->m_race_data = GeneralDataMgr::getInstance()->GetBaseRace(race);
    }
    saveDbJob job;
    job.sqls.push_back("insert into charactors set account='" + account
            + "',name='" + (name)
            + "',level='1',exp='0',spic='" + LEX_CAST_STR(spic)
            + "',lastlogin=unix_timestamp(),createTime=unix_timestamp(),id=" + LEX_CAST_STR(cid));
    job.sqls.push_back("insert into char_data set cid='" + LEX_CAST_STR(cid)
            + "',race='" + LEX_CAST_STR(race)
            + "',vip='0',chat='" + GetDb().safestr(strCharChatMessage[spic-1]) + "'");
    job.sqls.push_back("insert into char_resource set cid=" + LEX_CAST_STR(cid)
                    + ",gold=" + LEX_CAST_STR(g_default_gold)
                    + ",silver=" + LEX_CAST_STR(g_default_silver)
                    + ",prestige1_level='1',prestige2_level='1',prestige3_level='1',prestige4_level='1'");
    //char_stronghold
    baseMap* pm = Singleton<mapMgr>::Instance().GetBaseMap(1).get();
    assert(pm);
    for (int stageid = 1; stageid <= pm->m_baseStages.size(); ++stageid)
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
        if (pm->m_baseStages[stageid-1].get())
        {
            CharStageData *pStage = NULL;
            //创建场景
            {
                pStage = new CharStageData;
                pStage->m_baseStage = pm->m_baseStages[stageid-1];
                (*pMap)[stageid].reset(pStage);
            }
            std::vector<int> m_stronghold_state;
            for (size_t i = 0; i < pm->m_baseStages[stageid-1]->m_size; ++i)
            {
                int m_state = -2;
                if (stageid == 1 && i == 0)
                {
                    m_state = 0;
                }
                else if (i < pm->m_baseStages[stageid-1]->m_size)
                {
                    m_state = -1;
                }
                boost::shared_ptr<baseStronghold> bstronghold = Singleton<mapMgr>::Instance().GetBaseStrongholdData(mapid, stageid, i+1);
                if (!bstronghold.get())
                {
                    break;
                }
                boost::shared_ptr<CharStrongholdData> p_tmp;
                p_tmp.reset(new CharStrongholdData(cid));
                if (i == pStage->m_strongholds.size())
                {
                    pStage->m_strongholds.push_back(p_tmp);
                }
                else
                {
                    ERR();
                }
                p_tmp->m_baseStronghold = bstronghold;
                p_tmp->m_state = m_state;
                m_stronghold_state.push_back(m_state);
            }
            pStage->m_cid = cid;
            const json_spirit::Value val_state(m_stronghold_state.begin(), m_stronghold_state.end());
            job.sqls.push_back("insert into char_stronghold set cid=" + LEX_CAST_STR(cid) + ",mapid=1,stageid=" + LEX_CAST_STR(stageid) + ",stronghold_state='"+json_spirit::write(val_state)+"'");
        }
    }
    GeneralDataMgr::getInstance()->addCharData(p_chardata);
    Singleton<taskMgr>::Instance().newChar(p_chardata);
    p_chardata->m_wild_citys.load();
    p_chardata->m_heros.Add(race);
    int first_chengzhang = libaoMgr::getInstance()->getChengzhangLibao(1);
    if (first_chengzhang > 0)
    {
        libaoMgr::getInstance()->addLibao(p_chardata.get(), first_chengzhang);
    }
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
    InsertSaveDb("delete from char_resource where cid=" + LEX_CAST_STR(cid));

    //移除内存中的角色数据
    GeneralDataMgr::getInstance()->removeCharData(cid);

    return HC_SUCCESS;
}

//删除角色
int DeleteChar(uint64_t cid)
{
    return _DeleteChar(cid);
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
            json_spirit::mValue value;
            json_spirit::read(*it_content, value);
            //cout << "content = " << *it_content << endl;
            std::list<Item> _items;
            json_spirit::mArray list = value.get_array();
            json_spirit::mArray::iterator it = list.begin();
            while (it != list.end())
            {
                if ((*it).type() != json_spirit::obj_type)
                {
                    ++it;
                    continue;
                }
                json_spirit::mObject& tmp_obj = (*it).get_obj();
                int type = 0, id = 0, count = 0;
                READ_INT_FROM_MOBJ(type,tmp_obj,"type");
                READ_INT_FROM_MOBJ(id,tmp_obj,"id");
                READ_INT_FROM_MOBJ(count,tmp_obj,"count");
                Item item;
                item.type = type;
                item.id = id;
                item.nums = count;
                //cout << "type = " << type << " id = " << id << " count = " << count << endl;
                _items.push_back(item);
                ++it;
            }
            //给礼包东西
            std::list<Item> items = _items;
            giveLoots(cdata.get(), items, NULL, NULL, true, loot_present);
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
    {
        int cid = 0;
        std::string account = "";
        //帐号是否存在
        q.get_result("select c.id,c.account from accounts as ac left join charactors as c on ac.account=c.account where ac.qid='" + GetDb().safestr(*it_qid)
                    + "' and ac.union_id='" + LEX_CAST_STR(*it_union_id)
                    + "' and ac.server_id='" + GetDb().safestr(*it_server_id)
                    + "' order by c.id limit 1");
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            cid = q.getval();
            account = q.getstr();
            q.free_result();

            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                //cout<<"recharge add gold to cid:"<<account_id<<",gold:"<<*it_gold<<endl;
                cdata->addGold(*it_gold, gold_get_init, true);
                //int recharge_type = gold_get_recharge;
                if ("1" == *it_type)
                {
                    //recharge_type = gold_get_gift_recharge;
                }
                else if ("2" == *it_type)
                {
                    //recharge_type = gold_get_plat_recharge;
                }
                //成功处理
                time_t recharge_time = time(NULL);
                int org_total = cdata->m_total_recharge;
                InsertSaveDb("insert into char_recharge set type='" + GetDb().safestr(*it_type) + "',cid=" + LEX_CAST_STR(cdata->m_id)
                        + ",account='" + GetDb().safestr(account) + "',gold='" + LEX_CAST_STR(*it_gold) + "',input=FROM_UNIXTIME(" + LEX_CAST_STR(recharge_time) + ")");
                cdata->m_total_recharge += *it_gold;
                InsertSaveDb("replace into char_recharge_total (cid,total_recharge) values (" + LEX_CAST_STR(cdata->m_id) + "," + LEX_CAST_STR(cdata->m_total_recharge) + ")");
                cdata->updateVip();
                //充值活动
                recharge_event_mgr::getInstance()->updateRechargeEvent(cdata->m_id, *it_gold);
                //首充活动
                recharge_event_mgr::getInstance()->updateFirstRechargeEvent(cdata.get(), org_total, cdata->m_total_recharge);
                //成功处理
                if (!q.execute("update pay_list set pay_result='1',pay_endtime=FROM_UNIXTIME(" + LEX_CAST_STR(recharge_time) + ") where pay_id='" + LEX_CAST_STR(*it_orderno) + "'"))
                {
                    CHECK_DB_ERR(q);
                }
                if (Singleton<lottery_event>::Instance().isOpen())
                {
                    int action_total_recharge = cdata->queryExtraData(char_data_type_normal,char_data_normal_lottery_total_recharge_score);
                    cdata->setExtraData(char_data_type_normal,char_data_normal_lottery_total_recharge_score, action_total_recharge+(*it_gold));
                    int score = cdata->queryExtraData(char_data_type_normal,char_data_normal_lottery_score);
                    cdata->setExtraData(char_data_type_normal,char_data_normal_lottery_score, score+(*it_gold));
                    Singleton<lottery_event>::Instance().notifyActionState(cdata.get());
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
}

//查询充值
int ProcessQueryRecharge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    robj.push_back( Pair("vip", pc->m_vip) );
    robj.push_back( Pair("recharge", pc->m_total_recharge + pc->m_vip_exp) );
    json_spirit::Array list;
    for (int i = 0; i < iMaxVIP; ++i)
    {
        json_spirit::Object o;
        o.push_back( Pair("level", i+1) );
        o.push_back( Pair("need", iVIP_recharge[i]) );
        list.push_back(o);
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
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

//设置引导完成
int ProcessSetGuideState(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    int next_guide = 0;
    READ_INT_FROM_MOBJ(next_guide,o,"next");
    robj.push_back( Pair("next", next_guide) );
    pc->setGuideStateComplete(id, next_guide);
    return HC_SUCCESS;
}

//改变玩家喊话内容
int ProcessCharChatChange(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    std::string chat = "";
    READ_STR_FROM_MOBJ(chat, o, "chat");
    if (chat != "")
    {
        pc->m_chat = chat;
        InsertSaveDb("update char_data set chat='" + GetDb().safestr(pc->m_chat) + "' where cid=" + LEX_CAST_STR(pc->m_id));
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

int ProcessTopButtonList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array list;
    pc->getTopButtonList(list);
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//请求声望奖励
int ProcessQueryPrestigeAward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }

    int race = 1;
    READ_INT_FROM_MOBJ(race, o, "race");
    robj.push_back( Pair("race", race) );
    if (race < 1)
        race = 1;
    if (race > 4)
        race = 4;
    robj.push_back( Pair("prestige", cdata->m_prestige[race-1]));
    robj.push_back( Pair("prestige_max", cdata->prestigeMax(race)));
    robj.push_back( Pair("prestige_level", cdata->m_prestige_level[race-1]));
    json_spirit::Array list;
    boost::shared_ptr<PrestigeAwards> pa = GeneralDataMgr::getInstance()->GetBasePrestigeAward(race);
    if (pa.get())
    {
        PrestigeAwards::iterator it = pa->begin();
        while (it != pa->end())
        {
            if (it->second.get())
            {
                basePrestigeAward* bpa = it->second.get();
                if (NULL == bpa)
                {
                    ERR();
                    return HC_ERROR;
                }
                json_spirit::Object obj;
                obj.push_back( Pair("id", bpa->id));
                obj.push_back( Pair("race", bpa->race));
                obj.push_back( Pair("need_prestige_level", bpa->need_prestige_level));
                if (bpa->id <= cdata->m_prestige_award[race-1].m_state.size())
                {
                    int state = cdata->m_prestige_award[race-1].m_state[bpa->id-1];
                    obj.push_back( Pair("state", state));
                }
                else
                {
                    while (bpa->id > cdata->m_prestige_award[race-1].m_state.size())
                    {
                        cdata->m_prestige_award[race-1].m_state.push_back(0);
                    }
                    cdata->m_prestige_award[race-1].save();
                    obj.push_back( Pair("state", 0));
                }
                json_spirit::Object i_obj;
                bpa->m_item.toObj(i_obj);
                obj.push_back( Pair("item_info", i_obj));
                list.push_back(obj);
            }
            ++it;
        }
        robj.push_back( Pair("list", list) );
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//领取声望奖励
int ProcessGetPrestigeAward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0, race = 1;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(race, o, "race");
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("race", race) );
    if (race < 1)
        race = 1;
    if (race > 4)
        race = 4;
    boost::shared_ptr<PrestigeAwards> pa = GeneralDataMgr::getInstance()->GetBasePrestigeAward(race);
    if (pa.get())
    {
        PrestigeAwards::iterator it = pa->find(id);
        if (it != pa->end() && it->second.get())
        {
            basePrestigeAward* bpa = it->second.get();
            if (cdata->m_prestige_level[race-1] < bpa->need_prestige_level)
            {
                return HC_ERROR_NEED_MORE_PRESTIGE_LEVEL;
            }
            std::list<Item> items;
            Item tmp(bpa->m_item.type, bpa->m_item.id, bpa->m_item.nums, bpa->m_item.extra);
            items.push_back(tmp);
            if (!cdata->m_bag.hasSlot(itemlistNeedBagSlot(items)))
            {
                return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
            }
            if (bpa->id <= cdata->m_prestige_award[race-1].m_state.size()
                && cdata->m_prestige_award[race-1].m_state[bpa->id-1] == 0)
            {
                cdata->m_prestige_award[race-1].m_state[bpa->id-1] = 1;
                cdata->m_prestige_award[race-1].save();
            }
            else
            {
                return HC_ERROR;
            }
            giveLoots(cdata.get(),items,NULL,&robj,true, loot_prestige_shop);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//请求声望商店
int ProcessQueryPrestigeShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }

    int race = 1;
    READ_INT_FROM_MOBJ(race, o, "race");
    robj.push_back( Pair("race", race) );
    if (race < 1)
        race = 1;
    if (race > 4)
        race = 4;
    robj.push_back( Pair("prestige", cdata->m_prestige[race-1]));
    robj.push_back( Pair("prestige_max", cdata->prestigeMax(race)));
    robj.push_back( Pair("prestige_level", cdata->m_prestige_level[race-1]));
    json_spirit::Array list;
    boost::shared_ptr<PrestigeGoods> pg = GeneralDataMgr::getInstance()->GetBasePrestigeGoods(race);
    if (pg.get())
    {
        PrestigeGoods::iterator it = pg->begin();
        while (it != pg->end())
        {
            if (it->second.get())
            {
                basePrestigeGoods* bpg = it->second.get();
                if (NULL == bpg)
                {
                    ERR();
                    return HC_ERROR;
                }
                json_spirit::Object obj;
                obj.push_back( Pair("id", bpg->id));
                obj.push_back( Pair("race", bpg->race));
                obj.push_back( Pair("need_prestige_level", bpg->need_prestige_level));
                obj.push_back( Pair("silver", bpg->silver));
                json_spirit::Object i_obj;
                bpg->m_item.toObj(i_obj);
                obj.push_back( Pair("item_info", i_obj));
                list.push_back(obj);
            }
            ++it;
        }
        robj.push_back( Pair("list", list) );
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//声望商店
int ProcessBuyPrestigeShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0, nums = 0, race = 1;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    READ_INT_FROM_MOBJ(race, o, "race");
    robj.push_back( Pair("race", race) );
    if (race < 1)
        race = 1;
    if (race > 4)
        race = 4;
    if (nums < 1)
        nums = 1;
    boost::shared_ptr<PrestigeGoods> pg = GeneralDataMgr::getInstance()->GetBasePrestigeGoods(race);
    if (pg.get())
    {
        PrestigeGoods::iterator it = pg->find(id);
        if (it != pg->end() && it->second.get())
        {
            basePrestigeGoods* bpg = it->second.get();
            if (cdata->m_prestige_level[race-1] < bpg->need_prestige_level)
            {
                return HC_ERROR_NEED_MORE_PRESTIGE_LEVEL;
            }
            int cost_silver = bpg->silver * nums;
            if (cost_silver < 0)
            {
                return HC_ERROR;
            }
            std::list<Item> items;
            Item tmp(bpg->m_item.type, bpg->m_item.id, bpg->m_item.nums * nums, bpg->m_item.extra);
            items.push_back(tmp);
            if (!cdata->m_bag.hasSlot(itemlistNeedBagSlot(items)))
            {
                return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
            }
            if (tmp.type == ITEM_TYPE_HERO && (cdata->m_heros.m_hero_max - cdata->m_heros.m_heros.size()) < nums)
                return HC_ERROR_HERO_FULL;
            if (cdata->subSilver(cost_silver, silver_cost_buy_prestige) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_SILVER;
            }
            giveLoots(cdata.get(),items,NULL,&robj,true,loot_prestige_shop);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//请求装备列表
int ProcessShowEquipts(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->ShowEquipts(robj, o);
}

//装备
int ProcessEquipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int eid = 1, hid = 0, slot = 0;
    READ_INT_FROM_MOBJ(eid, o, "id");
    READ_INT_FROM_MOBJ(hid, o, "hid");
    READ_INT_FROM_MOBJ(slot, o, "slot");
    robj.push_back( Pair("hid", hid) );
    return cdata->equipt(hid, slot, eid);
}

//卸下
int ProcessUnequipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0, hid = 0, slot = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(slot, o, "slot");
    READ_INT_FROM_MOBJ(hid, o, "hid");
    robj.push_back( Pair("hid", hid) );
    //cout<<"ProcessUnequip:"<<slot<<","<<gid<<","<<id<<endl;
    if (slot)
    {
        return cdata->unequipt(hid, slot);
    }
    else
    {
        Equipment* eq = cdata->m_heros.getEquipById(id);
        if (eq && eq->getContainer() && eq->getContainer()->getType() == BAG_TYPE_HERO)
        {
            HeroBag* tmp = dynamic_cast<HeroBag*>(eq->getContainer());
            return cdata->unequipt(tmp->m_herodata.m_id, eq->getSlot());
        }
    }
    return HC_ERROR;
}

//合成概率信息
int ProcessCompoundEquiptInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->CompoundEquiptInfo(robj,o);
}

//合成装备一键装满
int ProcessCompoundEquiptOneKey(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->CompoundEquiptOneKey(robj,o);
}

//合成装备
int ProcessCompoundEquipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->CompoundEquipt(robj,o);
}

//强化概率信息
int ProcessUpgradeEquiptInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->UpgradeEquiptInfo(robj,o);
}

//强化装备
int ProcessUpgradeEquipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->UpgradeEquipt(robj,o);
}

//对英雄使用道具
int ProcessHeroUseGem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int tid = 1, hid = 0, nums = 0;
    READ_INT_FROM_MOBJ(tid, o, "type");
    READ_INT_FROM_MOBJ(hid, o, "hid");
    READ_INT_FROM_MOBJ(nums, o, "num");
    robj.push_back( Pair("hid", hid) );
    return cdata->useGem(hid, tid, nums);
}

int ProcessAddEquiptBaoshiSlot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    Equipment* equip = cdata->m_bag.getEquipById(id);
    if (!equip)
    {
        equip = cdata->m_heros.getEquipById(id);
        if (!equip)
        {
            return HC_ERROR;
        }
    }
    return equip->addBaoshiSlot();
}

int ProcessQueryBaoshiInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    if (id > 0)
    {
        int eid = 0, cid = 0;
        READ_INT_FROM_MOBJ(eid, o, "eid");
        READ_INT_FROM_MOBJ(cid, o, "cid");
        CharData* owner = cdata.get();
        if (cid != 0)
        {
            owner = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        }
        if (!owner)
        {
            return HC_ERROR;
        }
        Baoshi* p_baoshi = NULL;
        if (eid != 0)
        {
            Equipment* equip = owner->m_bag.getEquipById(eid);
            if (!equip)
            {
                equip = owner->m_heros.getEquipById(eid);
                if (!equip)
                {
                    return HC_ERROR;
                }
            }
            p_baoshi = equip->m_bag.getBaoshiById(id);
        }
        else
        {
            p_baoshi = owner->m_bag.getBaoshiById(id);
        }
        if (!p_baoshi)
        {
            return HC_ERROR;
        }
        json_spirit::Object bq;
        p_baoshi->toObj(bq);
        robj.push_back( Pair("baoshiVO",bq) );
    }
    else
    {
        int base_id = 0, level = 0;
        READ_INT_FROM_MOBJ(base_id, o, "base_id");
        READ_INT_FROM_MOBJ(level, o, "level");
        boost::shared_ptr<baseBaoshi> p = GeneralDataMgr::getInstance()->GetBaseBaoshi(base_id);
        if (p.get())
        {
            if (level < 1 || level > MAX_BAOSHI_LEVEL)
            {
                return HC_ERROR;
            }
            json_spirit::Object bq;
            bq.push_back( Pair("base_id", base_id));
            bq.push_back( Pair("name", p->name) );
            bq.push_back( Pair("level", level) );
            bq.push_back( Pair("value", p->values[level-1]));
            bq.push_back( Pair("quality", p->qualitys[level-1]));
            bq.push_back( Pair("price", level * 1000));
            std::string memo = p->memo;
            str_replace(memo, "$V", LEX_CAST_STR(p->values[level-1]));
            bq.push_back( Pair("memo", memo));
            robj.push_back( Pair("baoshiVO",bq) );
        }
    }
    return HC_SUCCESS;
}

//请求宝石列表
int ProcessQueryBaoshiList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->ShowBaoshis(robj, o);
}

int ProcessInlayBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int bagSlot = 0;
    READ_INT_FROM_MOBJ(bagSlot, o, "bagSlot");
    int eid = 0, slot = 0;
    READ_INT_FROM_MOBJ(eid, o, "eid");
    READ_INT_FROM_MOBJ(slot, o, "slot");
    return pc->inlayBaoshi(bagSlot, eid, slot);
}

int ProcessRemoveBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int eid = 0, slot = 0;
    READ_INT_FROM_MOBJ(eid, o, "eid");
    READ_INT_FROM_MOBJ(slot, o, "slot");
    return cdata->removeBaoshi(eid, slot);
}

int ProcessCombineBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int base_id = 0, level = 0, nums = 0;
    READ_INT_FROM_MOBJ(base_id, o, "base_id");
    READ_INT_FROM_MOBJ(level, o, "level");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    return cdata->CombineBaoshi(base_id, level, nums, robj);
}

int ProcessCombineAllBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int level = 0;
    READ_INT_FROM_MOBJ(level, o, "level");
    return cdata->CombineAllBaoshi(level, robj);
}

