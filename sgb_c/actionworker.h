#pragma once

#include "login.h"
#include "utils_all.h"
#include "net.h"
#include "worker.hpp"
#include "errcode_def.h"
#include "admin_cmds.h"

#include <sys/syscall.h>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "utils_lang.h"
#include "json_spirit_utils.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "db_thread.h"
#include "text_filter.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "singleton.h"
#include "mails.h"
#include "city.h"
#include "hero.h"
#include "item.h"
#include "task.h"
#include "mall.h"
#include "copy.h"
#include "wild.h"
#include "new_combat.hpp"
#include "combatRecord.h"
#include "arena.h"
#include "rewards.h"
#include "action.h"
#include "explore.h"
#include "rankings.h"
#include "relation.h"
#include "treasure.h"
#include "sweep.h"
#include "pk.h"
#include "dailyScore.h"
#include "recharge_event.h"
#include "statistics.h"
#include "buff.h"
#include "prestige_task.h"
#include "findBack.h"
#include "goal.h"
#include "boss.h"
#include "bank.h"
#include "lottery_event.hpp"
#include "auction.h"
#include "weekRanking.h"

using namespace net;
using namespace json_spirit;

//~{Cb7Q>:<<4NJ}~}
extern volatile int iArenaFreeTimes;

extern std::string GetExeName();

extern void InsertSaveDb(const std::string& sql);
extern void InsertSaveDb(const saveDbJob& job);
extern void InsertDbCharCmd(dbCmd& _dbCmd);
extern void InsertMailcmd(mailCmd&);

//~{PBTvHU3#9+8f~}
extern int ProcessNewAdminNotice(json_spirit::mObject& o);

//~{I>3}HU3#9+8f~}
extern int ProcessDeleteAdminNotice(json_spirit::mObject& o);

//~{P^8DHU3#9+8f~}
extern int ProcessChangeAdminNotice(json_spirit::mObject& o);

//~{9c2%HU3#9+8f~}
extern int ProcessSendAdminNotice(json_spirit::mObject& o);

//~{2iQ/HU3#9+8f~}
extern int ProcessQueryAdminNotice(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//~{9X1U4&@m~}
extern int ProcessShutdown(json_spirit::mObject& o);

//~{C?HU;n6/AP1m~}
extern int ProcessGetDailyActionList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

Database& GetDb();


typedef int (*pFuncProcessCmds)(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj);

typedef int (*pFuncInternalProcessCmds)(json_spirit::mObject& o);

//~{=GI+5G3v#,;X5==GI+Q!Tq=gCf~}
int ProcessLogout(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj)
{
    cout << "ProcessLogout" << endl;
    //~{UK:E5GB<W4L,~}
    boost::shared_ptr<OnlineUser> account = psession->user();
    if (!account.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (account->m_onlineCharactor.get())
    {
        GeneralDataMgr::getInstance()->CharactorLogout(account->m_onlineCharactor);
    }
    account->m_onlineCharactor.reset();

    return HC_SUCCESS;
}

//~{ADLl<GB<<GB<5=J}>]?b~}
inline void save_talk_record(boost::shared_ptr<OnlineCharactor>& user, boost::shared_ptr<OnlineCharactor>& to, int ctype, const std::string& msg)
{
    if (!user.get() || !user->m_account.get() || !user->m_charactor.get())
    {
        return;
    }
    if (to.get() && to->m_charactor.get() && to->m_account.get())
    {
        InsertSaveDb("INSERT INTO admin_count_talk (`qid`,`qname`,`union_id`,`server_id`,`to_qid`,`to_qname`,`to_union_id`,`to_server_id`,`input`,`channel`,`content`) VALUES ('"
            + GetDb().safestr(user->m_account->m_qid) + "','"
            + GetDb().safestr(user->m_charactor->m_name) + "','"
            + LEX_CAST_STR(user->m_account->m_union_id) + "','"
            + user->m_account->m_server_id + "','"
            + GetDb().safestr(to->m_account->m_qid) + "','"
            + GetDb().safestr(to->m_charactor->m_name) + "','"
            + LEX_CAST_STR(to->m_account->m_union_id) + "','"
            + to->m_account->m_server_id + "',"
            + "now(), '" + LEX_CAST_STR(ctype) + "','" + GetDb().safestr(msg) + "')");
    }
    else
    {
        InsertSaveDb("INSERT INTO admin_count_talk (`qid`,`qname`,`union_id`,`server_id`,`input`,`channel`,`content`) VALUES ('"
            + GetDb().safestr(user->m_account->m_qid) + "','"
            + GetDb().safestr(user->m_charactor->m_name) + "','"
            + LEX_CAST_STR(user->m_account->m_union_id) + "','"
            + user->m_account->m_server_id + "',"
            + "now(),'" + LEX_CAST_STR(ctype) + "','" + GetDb().safestr(msg) + "')");
    }
}

int ProcessChat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (!paccount.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    boost::shared_ptr<OnlineCharactor> user = paccount->m_onlineCharactor;
    if (!user.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    // ~{=bNvO{O"4.~}
    if (user->m_charactor->m_can_chat == false)
    {
        return HC_ERROR_FORBIDEN_CHAT;
    }
    int ctype = 0;
    READ_INT_FROM_MOBJ(ctype, o, "ctype");
    std::string msg = "";
    READ_STR_FROM_MOBJ(msg, o, "m");
    std::string to = "";
    boost::shared_ptr<OnlineCharactor> toChar;
    int ret = HC_SUCCESS;
    switch (ctype)
    {
        case channel_race: //~{UsS*F55@~}
            ret = user->CampChat(msg);
            break;
        case channel_world: //~{J@=gF55@~}
            ret = user->WorldChat(msg, false);
            break;
        case channel_horn: //~{@.0HF55@~}
            ret = user->WorldChat(msg, true);
            break;
        case channel_wisper: //~{K=AD~}
            {
                READ_STR_FROM_MOBJ(to,o,"to");
                ret = user->Tell(to, msg, toChar);
            }
            break;
        case channel_broad://~{O5M3O{O"~}
            break;
        case channel_guild: //~{9+;aF55@~}
            ret = user->GuildChat(msg);
            break;
        case channel_room: //~{7?<dF55@~}
            ret = user->PKRoomChat(msg);
            break;
        default:
            ret = HC_ERROR;
    }
    if (ret == HC_SUCCESS || ret == HC_SUCCESS_NO_RET)
    {
        //~{ADLl<GB<1#4f5=J}>]?b~}
        save_talk_record(user, toChar, ctype, msg);
    }
    return ret;
}

//~{44=(=GI+~}
int ProcessCreateChar(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (!paccount.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }

    if (paccount->m_cid > 0)
    {
        return HC_ERROR;
    }

    int spic = 1;
    READ_UINT64_FROM_MOBJ(spic, o, "spic");
    std::string name = "unknow";
    READ_STR_FROM_MOBJ(name, o, "name");
    if (name.length() > 18)
    {
        robj.push_back( Pair("name", name) );
        robj.push_back( Pair("length", name.length()) );
        return HC_ERROR_NAME_TOO_LONG;
    }
    //~{C{WV7G7(~}
    if (name.length() < 3 || !Forbid_word_replace::getInstance()->isLegal(name))
    {
        return HC_ERROR_NAME_ILLEGAL;
    }
    int race = 0;
    READ_INT_FROM_MOBJ(race,o,"race");
    if (race < 1 || race > 4)
    {
        race = my_random(1, 4);
    }
    int ret = CreateChar(paccount->m_account, paccount->m_union_id, paccount->m_server_id, paccount->m_qid, race, spic, name, paccount->m_cid);
    if (HC_SUCCESS == ret)
    {
        robj.push_back( Pair("id", paccount->m_cid));
        //~{Hg9{JG5%=GI+#,44=(=GI+3I9&:s#,V1=S=xHkSNO7~}
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

            //~{<SHk=GI+5H<6PEO"!";F=p!"0WRx!">|An~}
            json_spirit::Object info;
            cdata->getRoleInfo(info);
            robj.push_back( Pair("info", info) );
            if (cdata->m_current_guide)
            {
                cdata->checkGuide(cdata->m_current_guide);
            }
        }
    }
    return ret;
}

//~{=GI+C{JG7q:O7(~}
int ProcessCheckName(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (!paccount.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    std::string name = "unknow";
    READ_STR_FROM_MOBJ(name, o, "name");
    if (name.length() > 18)
    {
        return HC_ERROR_NAME_TOO_LONG;
    }
    //~{C{WV7G7(~}
    if (name.length() < 3 || !Forbid_word_replace::getInstance()->isLegal(name))
    {
        return HC_ERROR_NAME_ILLEGAL;
    }

    Query q(GetDb());
    //~{=GI+1m~}
    q.get_result("select count(*) from charactors where name='" + GetDb().safestr(name) + "'");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        if (q.getval() > 0)
        {
            q.free_result();
            return HC_ERROR_CHAR_EXIST;
        }
        q.free_result();
    }
    else
    {
        q.free_result();
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

//~{;qH!Kf;zC{WV~}
int ProcessGetRandomName(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (!paccount.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    int gender = 0;
    READ_INT_FROM_MOBJ(gender,o,"g");
    if (gender != 1)
    {
        gender = 0;
    }
    robj.push_back( Pair("g", gender) );
    robj.push_back( Pair("name", GeneralDataMgr::getInstance()->getRandomName(gender)));
    return HC_SUCCESS;
}

//~{2iQ/=GI+PEO"~}
int ProcessGetRoleInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj)
{
    //~{UK:E5GB<W4L,~}
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    Object charobj;
    cdata->getRoleInfo(charobj);
    retObj.push_back( Pair("chardata", charobj));
    return HC_SUCCESS;
}

//~{2iQ/=GI+OjO8PEO"~}
int ProcessGetRoleDetail(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    std::string cname = "";
    READ_STR_FROM_MOBJ(cname,o,"name");
    if (cname != "")
    {
        int cid = GeneralDataMgr::getInstance()->GetCharId(cname);
        if (cid)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (pc)
            {
                return pc->getRoleDetail(robj);
            }
        }
    }
    return cdata->getRoleDetail(robj);
}

//~{GkGs=p1RO{7QLaJ>W4L,~}
int ProcessGetConfirmInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array clist;
    for (int i = 0; i < iMaxGoldCostConfirm; ++i)
    {
        if (pc->m_gold_cost_comfirm[i])
        {
            json_spirit::Object obj;
            obj.push_back( Pair("type", i+1) );
            obj.push_back( Pair("enable", 1) );
            clist.push_back(obj);
        }
    }
    robj.push_back( Pair("list", clist) );
    return HC_SUCCESS;
}

//~{IhVC=p1RO{7QLaJ>W4L,~}
int ProcessSetConfirmInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array clist;
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    int enable = 0;
    READ_INT_FROM_MOBJ(enable,o,"enable");
    bool bEnable = (enable == 1);
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("enable", bEnable?1:0) );
    return cdata->enableNoConfirmGoldCost(type, bEnable);
}

//~{<SKY~}
int ProcessSpeedXXX(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    robj.push_back( Pair("purpose", purpose) );
    switch (purpose)
    {
        case 1://~{>:<<@dH4~}
            return Singleton<arenaMgr>::Instance().clearCD(pc->m_id, robj);
            break;
        case 2://~{2X1&M<<SKY~}
            return Singleton<treasureMgr>::Instance().finish(pc->m_id);
            break;
        case 3://~{I(54<SKY~}
            return Singleton<sweepMgr>::Instance().speedUp(pc->m_id);
            break;
        default:
            break;
    }
    return HC_ERROR;
}

//~{;qH!~}
int ProcessGetXXX(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int purpose = 0, id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    robj.push_back( Pair("purpose", purpose) );
    switch (purpose)
    {
        case 1://~{G)5=@[;}=1@x~}
            {
                char_sign_data* e = Singleton<actionMgr>::Instance().getCharSignData(pc->m_id);
                if (e)
                {
                    return e->getAwards(*pc, id, robj);
                }
                break;
            }
        case 2://~{?*7~F_HU=1@x~}
            {
                char_seven_data* e = Singleton<actionMgr>::Instance().getCharSevenData(pc->m_id);
                if (e)
                {
                    return e->getAwards(*pc, id, robj);
                }
                break;
            }
        case 3://~{?*7~G?;/=1@x~}
            {
                char_equipt_level_data* e = Singleton<actionMgr>::Instance().getCharEquiptLevelData(pc->m_id);
                if (e)
                {
                    return e->getAwards(*pc, id, robj);
                }
                break;
            }
        case 4://~{?*7~I}PG=1@x~}
            {
                char_hero_star_data* e = Singleton<actionMgr>::Instance().getCharHeroStarData(pc->m_id);
                if (e)
                {
                    return e->getAwards(*pc, id, robj);
                }
                break;
            }
        case 5://~{?*7~S"P[=1@x~}
            {
                char_hero_data* e = Singleton<actionMgr>::Instance().getCharHeroData(pc->m_id);
                if (e)
                {
                    return e->getAwards(*pc, id, robj);
                }
                break;
            }
        case 6://~{?*7~3e9X=1@x~}
            {
                char_stronghold_data* e = Singleton<actionMgr>::Instance().getCharStrongholdData(pc->m_id);
                if (e)
                {
                    return e->getAwards(*pc, id, robj);
                }
                break;
            }
        case 7://~{3dV5=1@x~} ~{5%1J~}
            {
                int charge = 100;
                READ_INT_FROM_MOBJ(charge,o,"id");
                ret = recharge_event_mgr::getInstance()->getReward(pc, 1, charge, robj);
                if (HC_SUCCESS == ret)
                {
                    int active = recharge_event_mgr::getInstance()->getRechargeEventState(pc);
                    pc->updateTopButton(top_button_rechargeAction, active);
                }
                return ret;
            }
        case 8://~{3dV5=1@x~} ~{@[<F~}
            {
                int charge = 100;
                READ_INT_FROM_MOBJ(charge,o,"id");
                ret = recharge_event_mgr::getInstance()->getReward(pc, 2, charge, robj);
                if (HC_SUCCESS == ret)
                {
                    int active = recharge_event_mgr::getInstance()->getRechargeEventState(pc);
                    pc->updateTopButton(top_button_rechargeAction, active);
                }
                return ret;
            }
        case 9://~{TZO_@q0|=1@x~}
            {
                char_online_libao_data* e = Singleton<actionMgr>::Instance().getCharOnlineLibaoData(pc->m_id);
                if (e)
                {
                    return e->getAwards(*pc, robj);
                }
                break;
            }
        case 10://~{JW3d=1@x~}
            {
                int type = 1;//~{D,HO;n6/~}1
                int first_state = pc->queryExtraData(char_data_type_normal, char_data_normal_first_recharge_event1);
                if (first_state == 2)//~{RQ>-Mj3I~}
                {
                    first_state = pc->queryExtraData(char_data_type_normal, char_data_normal_first_recharge_event2);
                    if (first_state == 2)
                    {
                        return HC_ERROR;
                    }
                    type = 2;
                }
                ret = recharge_event_mgr::getInstance()->getFirstReward(pc, type, robj);
                if (HC_SUCCESS == ret)
                {
                    if (type == 1)
                    {
                        pc->setExtraData(char_data_type_normal, char_data_normal_first_recharge_event1, 2);
                        first_state = pc->queryExtraData(char_data_type_normal, char_data_normal_first_recharge_event2);
                        if (first_state == 2)
                        {
                            pc->removeTopButton(top_button_first_recharge);
                        }
                        else if(first_state == 0 || first_state == 1)
                        {
                            pc->updateTopButton(top_button_first_recharge, first_state);
                        }
                    }
                    else if(type == 2)
                    {
                        pc->setExtraData(char_data_type_normal, char_data_normal_first_recharge_event2, 2);
                        pc->removeTopButton(top_button_first_recharge);
                    }
                }
                return ret;
            }
#ifdef QQ_PLAT
        case 100:    //~{AlH!;FWjW(JtNd=+~}
            {
                if (pc->m_qq_yellow_level <= 0)
                {
                    return HC_ERROR;
                }
                if (pc->m_vip < 2)
                {
                    return HC_ERROR_NEED_MORE_VIP;
                }
                //~{W(JtNd=+JG7qRQ>-AlH!~}
                if (pc->queryExtraData(char_data_type_normal, char_data_normal_qq_yellow_special))
                {
                    return HC_ERROR;
                }
                baseLibao* lb = libaoMgr::getInstance()->getQQSpecialLibao();
                if (lb)
                {
                    if (pc->m_level < lb->m_need_extra)
                    {
                        return HC_ERROR_NEED_MORE_LEVEL;
                    }
                    if (!pc->m_bag.hasSlot(lb->need_slot_num))
                    {
                        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                    }
                    //~{Nd=+IOO^EP6O~}
                    if (pc->m_heros.isFull())
                    {
                        return HC_ERROR_HERO_FULL;
                    }
                    //~{8x6+Nw~}
                    std::list<Item> items = lb->m_list;
                    giveLoots(pc, items, NULL, &robj, true, loot_qq_yellow);
                    pc->setExtraData(char_data_type_normal, char_data_normal_qq_yellow_special, 1);
                }
                return HC_SUCCESS;
            }
        case 101:    //~{AlH!;FWjPBJV@q0|~}
            {
                if (pc->m_qq_yellow_level <= 0)
                {
                    return HC_ERROR;
                }
                //~{JG7qRQ>-AlH!~}
                if (pc->queryExtraData(char_data_type_normal, char_data_normal_qq_yellow_newbie))
                {
                    return HC_ERROR;
                }
                //~{8x6+Nw~}
                baseLibao* lb = libaoMgr::getInstance()->getQQNewbieLibao();
                if (lb)
                {
                    if (!pc->m_bag.hasSlot(lb->need_slot_num))
                    {
                        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                    }
                    //~{8x6+Nw~}
                    std::list<Item> items = lb->m_list;
                    giveLoots(pc, items, NULL, &robj, true, loot_qq_yellow);
                    pc->setExtraData(char_data_type_normal, char_data_normal_qq_yellow_newbie, 1);
                }
                return HC_SUCCESS;
            }
        case 102:    //~{AlH!;FWjC?HU@q0|~}
            {
                if (pc->m_qq_yellow_level <= 0)
                {
                    return HC_ERROR;
                }
                //~{JG7qRQ>-AlH!~}
                if (pc->queryExtraData(char_data_type_daily, char_data_daily_qq_yellow_libao))
                {
                    return HC_ERROR;
                }
                //~{8x6+Nw~}
                baseLibao* lb = libaoMgr::getInstance()->getQQDailyLibao(pc->m_qq_yellow_level);
                if (lb)
                {
                    if (!pc->m_bag.hasSlot(lb->need_slot_num))
                    {
                        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                    }
                    //~{8x6+Nw~}
                    std::list<Item> items = lb->m_list;
                    giveLoots(pc, items, NULL, &robj, true, loot_qq_yellow);
                    pc->setExtraData(char_data_type_daily, char_data_daily_qq_yellow_libao, 1);
                }
                return HC_SUCCESS;
            }
        case 103:    //~{AlH!Dj7Q;FWjC?HU=1@x~}
            {
                if (pc->m_qq_yellow_year <= 0)
                {
                    return HC_ERROR;
                }
                //~{JG7qRQ>-AlH!~}
                if (pc->queryExtraData(char_data_type_daily, char_data_daily_qq_year_yellow_libao))
                {
                    return HC_ERROR;
                }
                baseLibao* lb = libaoMgr::getInstance()->getQQYearDailyLibao();
                if (lb)
                {
                    if (!pc->m_bag.hasSlot(lb->need_slot_num))
                    {
                        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                    }
                    //~{8x6+Nw~}
                    std::list<Item> items = lb->m_list;
                    giveLoots(pc, items, NULL, &robj, true, loot_qq_yellow);
                    pc->setExtraData(char_data_type_daily, char_data_daily_qq_year_yellow_libao, 1);
                }
                return HC_SUCCESS;
            }
        case 104:    //~{AlH!;FWj3I3$@q0|~}
            {
                if (pc->m_qq_yellow_level <= 0)
                {
                    return HC_ERROR;
                }
                //~{JG7qRQ>-AlH!~}
                if (pc->queryExtraData(char_data_type_normal, char_data_normal_qq_yellow_level_libao + id))
                {
                    return HC_ERROR;
                }
                baseLibao* lb = libaoMgr::getInstance()->getQQLevelLibao(id);
                if (lb)
                {
                    if (pc->m_level < lb->m_need_extra)
                    {
                        return HC_ERROR_NEED_MORE_LEVEL;
                    }
                    if (!pc->m_bag.hasSlot(lb->need_slot_num))
                    {
                        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                    }
                    //~{8x6+Nw~}
                    std::list<Item> items = lb->m_list;
                    giveLoots(pc, items, NULL, &robj, true, loot_qq_yellow);
                    pc->setExtraData(char_data_type_normal, char_data_normal_qq_yellow_level_libao + id, 1);
                }
                return HC_SUCCESS;
            }
#endif
        default:
            break;
    }
    return HC_ERROR;
}


/************************* ~{DZ2?=S?Z~} *******************************/

//~{1#4fJ}>]~}
int ProcessSaveDb(json_spirit::mObject& o)
{
    //INFO("ProcessSaveDb()"<<endl);
    int save_all = 0;
    READ_INT_FROM_MOBJ(save_all, o, "all");
    GeneralDataMgr::getInstance()->SaveDb(save_all);
    return HC_SUCCESS;
}

//~{1#3VJ}>]?bA,=S~}
int ProcessKeepDb(json_spirit::mObject& o)
{
    GetDb();
    return HC_SUCCESS;
}

//~{@kO_~}
int ProcessOffline(json_spirit::mObject& o)
{
    cout << "ProcessOffline" << endl;
    std::string account = "";
    READ_STR_FROM_MOBJ(account,o,"account");
    boost::shared_ptr<OnlineUser> ou = GeneralDataMgr::getInstance()->GetAccount(account);
    if (ou.get())
    {
        GeneralDataMgr::getInstance()->Logout(ou);
    }
    return HC_SUCCESS;
}

//~{VXPB<STXJ}>]?b~}
int ProcessReload(json_spirit::mObject& o)
{
    //INFO("ProcessSaveDb()"<<endl);
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    GeneralDataMgr::getInstance()->reload(type);
    return HC_SUCCESS;
}

int ProcessBroadCastMsg(json_spirit::mObject& o)
{
    std::string msg = "";
    READ_STR_FROM_MOBJ(msg,o,"msg");
    if (msg != "")
    {
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }
    return HC_SUCCESS;
}

int ProcessSendMsg(json_spirit::mObject& o)
{
    std::string msg = "";
    std::string name = "";
    READ_STR_FROM_MOBJ(msg,o,"msg");
    READ_STR_FROM_MOBJ(name,o,"name");
    if (msg != "" && name != "")
    {
        boost::shared_ptr<OnlineCharactor> c = GeneralDataMgr::getInstance()->GetOnlineCharactor(name);
        if (c.get())
        {
            c->Send(msg);
        }
    }
    return HC_SUCCESS;
}

//~{C?HU8|PB~}
int dailyUpdate()
{
    //~{;y4!J}>]C?HU8|PB~}
    GeneralDataMgr::getInstance()->dailyUpdate();
    //~{811>C?HU8|PB~}
    Singleton<copyMgr>::Instance().dailyUpdate();
    //~{C?HU1XWv~}
    Singleton<dailyScoreMgr>::Instance().dailyUpdate();
    //~{TZO_@q0|~}
    Singleton<actionMgr>::Instance().resetOnlineLibaoAll();
    //~{9+;aC?HU8|PB~}
    Singleton<guildMgr>::Instance().dailyUpdate();
    //~{H+2?VXVC:s6TTZO_Mf<R5DR;P)4&@m~}
    GeneralDataMgr::getInstance()->dailyOnlineChar();
    return HC_SUCCESS;
}

//~{C?V\8|PB~}
int weekUpdate()
{
    //~{;y4!J}>]C?HU8|PB~}
    GeneralDataMgr::getInstance()->weekUpdate();
    return HC_SUCCESS;
}

//~{Mb2?6(J1JB<~4&@m~}
int ProcessScheduleEvent(json_spirit::mObject& o)
{
    uint64_t start_time = 0;
    if (g_print_debug_info > 1)
    {
        start_time = splsTimeStamp();
    }
    std::string event = "";
    READ_STR_FROM_MOBJ(event,o,"event");

    int cmd = getAdminCmds(event);
    //cout<<"ProcessScheduleEvent,event:"<<event<<endl;
    switch (cmd)
    {
        //~{C?HUK"PB~}
        case admin_daily_reset:
        {
            INFO(" ************ daily_reset at "<<time(NULL)<<" ****************"<<endl);
            dailyUpdate();
            break;
        }
        //~{C?V\K"PB~}
        case admin_week_reset:
        {
            INFO(" ************ week_reset at "<<time(NULL)<<" ****************"<<endl);
            weekUpdate();
            break;
        }
        //~{PDLx~}
        case admin_heartBeat:
        {
            GeneralDataMgr::getInstance()->HeartBeat();
            break;
        }
        //~{1#4fMf<RJ}>]~}
        case admin_save_db:
        {
            //cout<<" ************ save_db at "<<time(NULL)<<" ****************"<<endl;
            int save_all = 0;
            READ_INT_FROM_MOBJ(save_all,o,"param1");
            GeneralDataMgr::getInstance()->SaveDb(save_all);
            break;
        }
        //~{63=aUJ:E~}
        case admin_freeze_account:
        {
            std::string account = "";
            int endtime = 0;
            READ_INT_FROM_MOBJ(endtime,o,"param1");
            READ_STR_FROM_MOBJ(account,o,"extra");
            if (account != "")
            {
                boost::shared_ptr<OnlineUser> ou = GeneralDataMgr::getInstance()->GetAccount(account);
                if (ou.get())
                {
                    GeneralDataMgr::getInstance()->Logout(ou);
                }
            }
            break;
        }
        //~{<l2iN46ASJ<~~}
        case admin_check_mail:
        {
            std::string name = "";
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_STR_FROM_MOBJ(name,o,"extra");
            if (name != "")
            {
                mailCmd cmd;
                cmd.mobj["name"] = name;
                cmd.cid = cid;
                cmd.cmd = mail_cmd_get_unread_list;
                InsertMailcmd(cmd);
            }
            break;
        }
        //~{={QT~}
        case admin_gag_char:
        {
            int cid = 0, endtime = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(endtime,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
                if (account.get())
                {
                    account->m_gag_end_time = endtime;
                }
            }
            break;
        }
        //~{C?HU>:<<3!=1@x~}
        case admin_arenaAwards:
        {
            INFO(" ************ arenaAwards at "<<time(NULL)<<" ****************"<<endl);
            Singleton<arenaMgr>::Instance().seasonAwards();
            break;
        }
        //~{C?HU3oBk>:<<3!=1@x~}
        case admin_pkAwards:
        {
            INFO(" ************ arenaAwards at "<<time(NULL)<<" ****************"<<endl);
            Singleton<PkMgr>::Instance().seasonAwards();
            break;
        }
        //~{IhVC5H<6~}
        case admin_setLevel:
        {
            int cid = 0, level = 1;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(level,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                cdata->levelup(level);
            }
            break;
        }
        //~{IhVC~}VIP~{5H<6~}
        case admin_setVip:
        {
            int cid = 0, vip = 1;
            READ_INT_FROM_MOBJ(cid, o, "param1");
            READ_INT_FROM_MOBJ(vip, o, "param2");
            if (vip < 0)
                vip = 0;
            if (vip > iMaxVIP)
                vip = iMaxVIP;
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                int old_vip = cdata->m_vip;
                cdata->m_vip = vip;
                if (old_vip != cdata->m_vip)
                {
                    InsertSaveDb("update char_data set vip='" + LEX_CAST_STR(cdata->m_vip)
                        + "' where cid=" + LEX_CAST_STR(cdata->m_id));
                }
            }
            break;
        }
        //~{<S5cH/~}
        case admin_addGold:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            int counts = 0;
            READ_INT_FROM_MOBJ(counts,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                if (counts <= 0)
                {
                    if ((cdata->gold(true) + counts) < 0)
                    {
                        cdata->gold(0);
                    }
                    else
                    {
                        cdata->subGold(-counts, gold_cost_init, true);
                    }
                }
                else
                {
                    cdata->addGold(counts, gold_get_init, true);
                }
            }
            break;
        }
        //~{<S@q=p~}
        case admin_addBindgold:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            int counts = 0;
            READ_INT_FROM_MOBJ(counts,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                if (counts <= 0)
                {
                    if ((cdata->gold() + counts) < 0)
                    {
                        cdata->gold(0);
                    }
                    else
                    {
                        cdata->subGold(-counts, gold_cost_init);
                    }
                }
                else
                {
                    cdata->addGold(counts, gold_get_init);
                }
            }
            break;
        }
        //~{<S3oBk~}
        case admin_addSilver:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            int counts = 0;
            READ_INT_FROM_MOBJ(counts,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                if (counts <= 0)
                {
                    if ((cdata->silver() + counts) < 0)
                    {
                        cdata->silver(0);
                    }
                    else
                    {
                        cdata->subSilver(-counts, silver_cost_init);
                    }
                }
                else
                {
                    cdata->addSilver(counts, silver_get_init);
                }
            }
            break;
        }
        //~{<SIyM{~}
        case admin_addPrestige:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            int race = 0;
            READ_INT_FROM_MOBJ(race,o,"param2");
            int counts = 0;
            READ_INT_FROM_MOBJ(counts,o,"param3");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                if (counts > 0)
                {
                    cdata->addPrestige(race, counts);
                }
            }
            break;
        }
        //~{<S5@>_~}
        case admin_addGem:
        {
            int cid = 0, tid = 0, count = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(tid,o,"param2");
            READ_INT_FROM_MOBJ(count,o,"param3");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                if (count > 0)
                {
                    cdata->addGem(tid, count);
                }
                else
                {
                    if (cdata->subGem(tid, -count) < 0)
                    {
                        cdata->subGem(tid, cdata->m_bag.getGemCount(tid));
                    }
                }
            }
            break;
        }
        //~{<SS"P[~}
        case admin_addHero:
        {
            int cid = 0, hid = 1, star = 1, level = 1;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(hid,o,"param2");
            READ_INT_FROM_MOBJ(star,o,"param3");
            READ_INT_FROM_MOBJ(level,o,"param4");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                cdata->m_heros.Add(hid, level, star);
            }
            break;
        }
        //~{S"P[5H<6~}
        case admin_heroLevel:
        {
            int cid = 0, hid = 0, level = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(hid,o,"param2");
            READ_INT_FROM_MOBJ(level,o,"param3");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                if (0 == level)
                {
                    level = cdata->m_level;
                }
                else if (level > cdata->m_level || level < 0)
                {
                    level = cdata->m_level;
                }
                if (0 == hid)
                {
                    std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = cdata->m_heros.m_heros.begin();
                    while (it != cdata->m_heros.m_heros.end())
                    {
                        if (it->second.get())
                        {
                            if (it->second->m_level < level)
                            {
                                it->second->levelup(level);
                            }
                        }
                        ++it;
                    }
                }
                else
                {
                    boost::shared_ptr<CharHeroData> hero_data = cdata->m_heros.GetHero(hid);
                    if (hero_data.get())
                    {
                        hero_data->levelup(level);
                    }
                }
            }
            break;
        }
        //~{<SW018~}
        case admin_addEquiptment:
        {
            int cid = 0, eid = 1, level = 1, quality = 1;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(eid,o,"param2");
            READ_INT_FROM_MOBJ(level,o,"param3");
            READ_INT_FROM_MOBJ(quality,o,"param4");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                cdata->addEquipt(eid, level, quality);
            }
            break;
        }
        //~{IhVCW0185H<6~}
        case admin_setEquiptment:
        {
            int cid = 0, eid = 0, level = 0;
            READ_INT_FROM_MOBJ(cid, o, "param1");
            READ_INT_FROM_MOBJ(eid, o, "param2");
            READ_INT_FROM_MOBJ(level, o, "param3");
            CharData* cdata = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (cdata)
            {
                if (0 == level)
                {
                    level = cdata->m_level;
                }
                else if (level > cdata->m_level || level < 0)
                {
                    level = cdata->m_level;
                }
                //~{130|@o~}
                Equipment* up_equipt = cdata->m_bag.getEquipById(eid);
                if (!up_equipt)
                {
                    //~{S"P[ImIO~}
                    up_equipt = cdata->m_heros.getEquipById(eid);
                    if (!up_equipt)
                    {
                        return HC_SUCCESS;
                    }
                }
                up_equipt->updateLevel(level);
            }
            break;
        }
        case admin_addBaoshi:
        {
            int cid = 0, baoshi_id = 0, level = 0, num = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(baoshi_id,o,"param2");
            READ_INT_FROM_MOBJ(level,o,"param3");
            READ_INT_FROM_MOBJ(num,o,"param4");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                CharData* pc = cdata.get();
                pc->addBaoshi(baoshi_id, level, num);
            }
            break;
        }
        //~{7"7E@q0|~}
        case admin_add_libao:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            int libao_id = 0;
            READ_INT_FROM_MOBJ(libao_id,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                CharData* pc = cdata.get();
                pc->addLibao(libao_id, 1);
            }
            else
            {
                //~{H+7~<S~}

            }
            break;
        }
        //~{VXPB<STX3dV5;n6/~}
        case admin_reload_recharge_event:
        {
            int type = 0;
            READ_INT_FROM_MOBJ(type,o,"param1");
            int clear = 0;
            READ_INT_FROM_MOBJ(clear,o,"param2");
            recharge_event_mgr::getInstance()->reload(type);
            if (clear)
            {
                recharge_event_mgr::getInstance()->reset(type);
            }
            break;
        }
        //~{?*Ft~}boss~{U=67~}
        case admin_openBoss:
        {
            int boss = 0;
            READ_INT_FROM_MOBJ(boss,o,"param1");
            int last_mins = 120;
            READ_INT_FROM_MOBJ(last_mins,o,"param2");
            bossMgr::getInstance()->openBoss(boss, last_mins);
            break;
        }
        //~{9X1U~}boss~{U=67~}
        case admin_closeBoss:
        {
            int boss = 0;
            READ_INT_FROM_MOBJ(boss,o,"param1");
            bossMgr::getInstance()->closeBoss(boss);
            break;
        }
        //~{?*Ft3i=1;n6/~}
        case admin_open_lottery:
        {
            std::string data = "";
            READ_STR_FROM_MOBJ(data,o,"extra");
            json_spirit::mValue value;
            json_spirit::read(data, value);
            if (value.type() == json_spirit::obj_type)
            {
                json_spirit::mObject& o = value.get_obj();
                time_t start_time = 0, end_time = 0;
                READ_INT_FROM_MOBJ(start_time,o,"start_time");
                READ_INT_FROM_MOBJ(end_time,o,"end_time");
                Singleton<lottery_event>::Instance().openEvent(start_time, end_time);
            }
            break;
        }
        //~{9X1U3i=1;n6/~}
        case admin_close_lottery:
        {
            Singleton<lottery_event>::Instance().closeEvent();
            break;
        }
        //~{P^84S"P[W4L,~}
        case admin_fix_hero_state:
        {
            std::vector<int> hid_list;
            std::vector<int> cid_list;
            Query q(GetDb());
            q.get_result("SELECT `id`,`cid` FROM `char_heros` WHERE state = 2");
            while (q.fetch_row())
            {
                int hid = q.getval();
                int cid = q.getval();
                hid_list.push_back(hid);
                cid_list.push_back(cid);
            }
            q.free_result();
            //~{W4L,<l2i~}(~{Rl3#~})
            if (hid_list.size())
            {
                std::vector<int>::iterator h_it = hid_list.begin();
                std::vector<int>::iterator c_it = cid_list.begin();
                while (h_it != hid_list.end() && c_it != cid_list.end())
                {
                    q.get_result("select id from wild_citys where hid = " + LEX_CAST_STR(*h_it));
                    CHECK_DB_ERR(q);
                    if (q.fetch_row())
                    {
                        //it's ok!
                    }
                    else
                    {
                        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(*c_it);
                        if (cdata.get())
                        {
                            boost::shared_ptr<CharHeroData> p_hero = cdata->m_heros.GetHero(*h_it);
                            if (p_hero.get() && p_hero->m_baseHero.get())
                            {
                                p_hero->m_state = HERO_STATE_INIT;
                                p_hero->m_city = 0;
                                p_hero->m_changed = true;
                                p_hero->updateAttribute();
                            }
                        }
                    }
                    q.free_result();
                    ++h_it;
                    ++c_it;
                }
            }
            break;
        }
		case admin_change_mall_discount:
		{
            time_t start_time = 0, end_time = 0;
            int discount = 100;
			READ_INT_FROM_MOBJ(discount,o,"param1");
			READ_INT_FROM_MOBJ(start_time,o,"param2");
			READ_INT_FROM_MOBJ(end_time,o,"param3");
            Singleton<mallMgr>::Instance().openMallDiscountEvent(discount,start_time, end_time);
			break;
		}
        default:
            break;
    }
    if (g_print_debug_info > 1)
    {
        uint64_t cost_us = splsTimeStamp()-start_time;
        if (cost_us > 5000)
        {
            cout<<"****** ProcessScheduleEvent : "<<event<<" cost "<<(double(cost_us)/1000)<<" ms ******"<<endl;
        }
    }
    return HC_SUCCESS;
}

//~{4&@m5GB<PEO"~}
int ProcessLogin(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    std::string username  = "";
    READ_STR_FROM_MOBJ(username,o,"user");
    std::string qid = "";
    READ_STR_FROM_MOBJ(qid,o,"qid");
    int isAdult = 0;
    READ_INT_FROM_MOBJ(isAdult,o,"isAdult");
    int union_id = 0;
    READ_INT_FROM_MOBJ(union_id,o,"union_id");
    std::string server_id = "";
    READ_STR_FROM_MOBJ(server_id,o,"server_id");
    INFO(username<<" login...");
#ifdef QQ_PLAT
    int qq_yellow_level = 0;
    int is_qq_year_yellow = 0;
    std::string iopenid = "";
    std::string feedid = "0";
    std::string str1 = "";
    std::string str2 = "";
    READ_INT_FROM_MOBJ(qq_yellow_level,o,"qqYellowLevel");
    if (qq_yellow_level > 0)
    {
        READ_INT_FROM_MOBJ(is_qq_year_yellow,o,"isYearYellow");
    }
    READ_STR_FROM_MOBJ(iopenid,o,"iopenid");
    READ_STR_FROM_MOBJ(feedid,o,"feedid");
    READ_STR_FROM_MOBJ(str1,o,"login_str1");
    READ_STR_FROM_MOBJ(str2,o,"login_str2");
    return GeneralDataMgr::getInstance()->Login(qid, username, isAdult, union_id, server_id, qq_yellow_level, is_qq_year_yellow, iopenid, feedid, str1, str2, psession, robj);
#else
    return GeneralDataMgr::getInstance()->Login(qid, username, isAdult, union_id, server_id, psession, robj);
#endif
}

//~{;q5CHU3#;n6/=gCf~}
int ProcessGetDailyActionList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array list;
    //boss~{PEO"~}
    {
        bossMgr::getInstance()->getAction(cdata,list);
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//~{;qH!@kO_N4Mj3I~}
int ProcessGetLogOutList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array list;
    json_spirit::Object obj;
    //~{UwJU~}
    {
        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(cdata->m_id);
        if (cc)
        {
            obj.clear();
            obj.push_back( Pair("type", log_out_levy));
            obj.push_back( Pair("num1", cc->levy_left_times()));
            list.push_back(obj);
        }
    }
    //~{2X1&M<~}
    if (cdata->isTreasureOpen())
    {
        boost::shared_ptr<char_treasure> pct = Singleton<treasureMgr>::Instance().getCharTreasure(cdata->m_id);
        if (pct.get())
        {
            obj.clear();
            obj.push_back( Pair("type", log_out_treasure) );
            obj.push_back( Pair("num1", pct->getCanStartTimes()));
            list.push_back(obj);
        }
    }
    //~{C?HU1XWv~}
    if (cdata->isDailyScoreOpen())
    {
        obj.clear();
        obj.push_back( Pair("type", log_out_daily_socre) );
        obj.push_back( Pair("num1", iDailyScoreTaskMax - cdata->m_score_tasks.m_task_cnt));
        list.push_back(obj);
    }
    //~{C?HUHNNq~}
    if (cdata->isDailyTaskOpen())
    {
        obj.clear();
        obj.push_back( Pair("type", log_out_daily_task) );
        obj.push_back( Pair("num1", cdata->m_tasks.getCharDailyTaskCnt()));
        list.push_back(obj);
    }
    //~{G)5=~}
    {
        char_sign_data* e = Singleton<actionMgr>::Instance().getCharSignData(cdata->m_id);
        if (e)
        {
            obj.clear();
            obj.push_back( Pair("type", log_out_sign));
            obj.push_back( Pair("num1", e->m_sign_time > 0 ? 0 : 1));
            list.push_back(obj);
        }
    }
    //~{TZO_=1@x~}
    if (cdata->isOnlineLibaoOpen())
    {
        int active = 0;
        char_online_libao_data* online = Singleton<actionMgr>::Instance().getCharOnlineLibaoData(cdata->m_id);
        if (online)
        {
            obj.clear();
            obj.push_back( Pair("type", log_out_online));
            obj.push_back( Pair("num1", online->getOnlineLibaoCnt()));
            list.push_back(obj);
        }
    }
    //~{3GMb3G3X~}
    {
        obj.clear();
        obj.push_back( Pair("type", log_out_wild));
        obj.push_back( Pair("num1", iWildOwnMax[cdata->m_vip]-cdata->m_wild_citys.getOwnCnt()));
        list.push_back(obj);
    }
    //~{W*EL3i=1~}
    if (cdata->isLotteryActionOpen())
    {
        if (Singleton<lottery_event>::Instance().isOpen())
        {
            int state = cdata->queryExtraData(char_data_type_daily,char_data_daily_get_lottery_score);
            obj.clear();
            obj.push_back( Pair("type", log_out_lottery));
            obj.push_back( Pair("num1", state == 0 ? 1 : 0));
            list.push_back(obj);
        }
    }
    //~{>:<<~}
    if (cdata->isArenaOpen())
    {
        int total_times = cdata->queryExtraData(char_data_type_daily, char_data_daily_arena);
        int buyTime = cdata->queryExtraData(char_data_type_daily, char_data_daily_buy_arena);
        if (total_times < (iArenaFreeTimes + buyTime))
        {
            obj.clear();
            obj.push_back( Pair("type", log_out_arena));
            obj.push_back( Pair("num1", (iArenaFreeTimes + buyTime) - total_times));
            list.push_back(obj);
        }
    }
    //~{IqAiK~~}
    if (cdata->isShenlingOpen())
    {
        int has_attack = cdata->queryExtraData(char_data_type_daily,char_data_daily_shenling);
        if (has_attack < iShenlingFreeTime)
        {
            obj.clear();
            obj.push_back( Pair("type", log_out_shenling));
            obj.push_back( Pair("num1", iShenlingFreeTime - has_attack));
            list.push_back(obj);
        }
    }
    //~{811>~}
    if (cdata->isCopyOpen())
    {
        int has_attack = cdata->queryExtraData(char_data_type_daily,char_data_daily_attack_copy);
        if (has_attack < iCopyTotal)
        {
            obj.clear();
            obj.push_back( Pair("type", log_out_copy));
            obj.push_back( Pair("num1", iCopyTotal - has_attack));
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list));
    return HC_SUCCESS;
}

volatile int g_open_log_msg = 1;

struct my_msg_st {

    long int my_msg_type;
    char some_text[256];

    my_msg_st()
    {
        my_msg_type = 1;
        memset(some_text, 0, 256);
    }
};

void log_msg(const std::string& msg, int type = 1)
{
    static int inited = 0;
    static key_t key = ftok(GetExeName().c_str(), 'a');
    static int msg_id = msgget(key, IPC_CREAT|0666);

    if (inited == 0)
    {
        if (-1 == key)
        {
            cout<<"****** create key fail ******"<<endl;
        }
        else if (msg_id == -1)
        {
            cout<<"****** msgget fail ******"<<endl;
        }
        inited = 1;
    }
    if (g_open_log_msg && msg_id)
    {
        my_msg_st smsg;
        smsg.my_msg_type = type;
        strncpy(smsg.some_text, msg.c_str(), 256);
        msgsnd(msg_id, &smsg, strlen(smsg.some_text) + 1, IPC_NOWAIT);
    }
}

template <typename Job>
class worker_new
{
public:
    worker_new(const std::string& name, jobqueue<Job>& _jobqueue, std::size_t _maxthreads = 1);
    virtual ~~worker_new(void);

public:
    void run();
    void stop();
    virtual bool work(Job& task) = 0;        // ~{EIIz@`PhR*VXTX4KPi:/J}~},~{RTMj3I9$Ww~}.
    int running();
protected:
    void workloop();                    // ~{9$WwQ-;7~}.

private:
    std::string worker_name;
    std::vector<boost::shared_ptr<boost::thread> > threads_;
    boost::mutex mutex_;
    std::size_t maxthreads_;
    volatile int _runing_loop;
    jobqueue<Job>& jobqueue_;
    volatile bool exitthread;
};

template <typename Job>
worker_new<Job>::worker_new(const std::string& name,jobqueue<Job>& _jobqueue, std::size_t _maxthreads/* = 4*/) :
worker_name(name),
maxthreads_(_maxthreads),
_runing_loop(0),
jobqueue_(_jobqueue),
exitthread(false)
{
}

template <typename Job>
worker_new<Job>::~~worker_new(void)
{
}

template <typename Job>
void worker_new<Job>::run()
{
    try
    {
        for (std::size_t i = 0; i < maxthreads_; ++i) {
            boost::shared_ptr<boost::thread> _thread(new boost::thread(
                boost::bind(&worker_new::workloop, this)));
            threads_.push_back(_thread);
        }

        for (std::size_t i = 0; i < maxthreads_; ++i) {
            threads_[i]->join();
        }
    }
    catch (std::exception& e)
    {
        std::cout << "ERROR INFO:" << e.what() << std::endl;
    }
}

template <typename Job>
void worker_new<Job>::stop()
{
    exitthread = true;
    jobqueue_.notify_all();
}

template <typename Job>
int worker_new<Job>::running()
{
    return _runing_loop;
}

template <typename Job>
void worker_new<Job>::workloop()               // ~{KySP9$WwTZP)Mj3I~}.
{
    ++_runing_loop;
#ifdef DEBUG_PER
    time_t last_time = 0;
    uint64_t processed_cmd = 0;
#endif
    //cout<<" ************************* action wookloop , tid="<<boost::this_thread::get_id()<<" *************************  "<<endl;
    do
    {
#ifdef DEBUG_PER

        //if (g_print_debug_info)
        {
            uint64_t processed = jobqueue_._processed_cmds-processed_cmd;
            time_t time_now = time(NULL);
            if (last_time != time_now && time_now % 10 == 0)
            {
                uint64_t inqueue = jobqueue_._total_cmds - jobqueue_._processed_cmds;
                if (processed >= 2000 || inqueue > 100)
                {
                    cout<<"======================= action work : "<<dec<<jobqueue_._processed_cmds<<"/"<<inqueue<<"("<<processed<<"/10s)"<<endl<<flush;
                }
                processed_cmd = jobqueue_._processed_cmds;
            }
            last_time = time_now;
        }
#endif
        try
        {
            //uint64_t _start_time = splsTimeStamp();
            Job task_ = jobqueue_.getjob();
            //task_._start_get = _start_time;
#ifdef DEBUG_PER
            task_._get_time = splsTimeStamp();
#endif
            if (work(task_))
            {
#ifdef DEBUG_PER
                uint64_t time_cost = splsTimeStamp() - task_._get_time;
                if (time_cost > 5000)
                {
                    cout<<"!!!!!!!!!!!! cmd "<<task_.cmd<<" cost "<<time_cost<<" us"<<endl;
                    if (task_.cmd == "scheduleEvent")
                    {
                        json_spirit::mObject& obj  = task_.getRecvObj();
                        std::string event = "";
                        READ_STR_FROM_MOBJ(event,obj,"event");
                        cout<<"event="<<event<<endl;
                    }
                    else if (task_.cmd == "combatResult")
                    {
                        json_spirit::mObject& obj  = task_.getRecvObj();
                        uint64_t combat_id = 0;
                        int type = 0;
                        READ_INT_FROM_MOBJ(type, obj, "type");
                        READ_UINT64_FROM_MOBJ(combat_id, obj, "id");
                        cout<<"battle type "<<type<<",id "<<combat_id<<endl;
                    }
                }
#endif
                continue;
            }
            else
                break;
        }
        catch (std::exception& e)
        {
            std::cerr << "wrok loop Exception: " << e.what() << "\n";
            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);
            for (int i = 0; i < nSize; i++)
            {
                cout << symbols[i] << endl;
            }
            free(symbols);
            --_runing_loop;
            return;
        }
    } while (!exitthread);
    std::cout<< "**************** "<<worker_name<<" workloop break *****************" <<endl;
    --_runing_loop;
    return;
}

class ActionWorker : public worker_new<actionmessage>
{
public:
    ActionWorker(jobqueue<actionmessage>& _jobqueue, std::size_t _maxthreads = 1) :
      worker_new<actionmessage>("mainworker",_jobqueue, _maxthreads)
    {
        /**************~{6TMb=S?Z~}****************/
        //~{44=(5GB<O`9X~}
        m_cmds_process_map["login"] = ProcessLogin;
        m_cmds_process_map["logout"] = ProcessLogout;
        m_cmds_process_map["create"] = ProcessCreateChar;
        m_cmds_process_map["checkName"] = ProcessCheckName;
        m_cmds_process_map["getRandomName"] = ProcessGetRandomName;

        //~{=gCfPEO"~}
        m_cmds_process_map["getRoleInfo"] = ProcessGetRoleInfo;
        m_cmds_process_map["getRoleDetail"] = ProcessGetRoleDetail;
        m_cmds_process_map["getHeroInfo"] = ProcessGetHeroInfo;
        m_cmds_process_map["getHeroList"] = ProcessCharHeros;
        m_cmds_process_map["setChat"] = ProcessCharChatChange;
        //~{2iQ/Mf<RO^J1TvRf~}
        m_cmds_process_map["queryCharBuffs"] = ProcessQueryCharBuffs;
        //~{2iQ/3dV5=S?Z~}
        m_cmds_process_map["getRecharge"] = ProcessQueryRecharge;

        //~{S"P[~}
        m_cmds_process_map["setDefaultHero"] = ProcessSetDefaultHero;
        m_cmds_process_map["queryHeroPack"] = ProcessQueryHeroPack;
        m_cmds_process_map["openHeroPack"] = ProcessOpenHeroPack;
        m_cmds_process_map["buyHeroSize"] = ProcessBuyHeroSize;
        m_cmds_process_map["compoundHeroInfo"] = ProcessCompoundHeroInfo;
        m_cmds_process_map["compoundHero"] = ProcessCompoundHero;
        m_cmds_process_map["decomposeHeroInfo"] = ProcessDecomposeHeroInfo;
        m_cmds_process_map["decomposeHero"] = ProcessDecomposeHero;
        m_cmds_process_map["smeltHeroInfo"] = ProcessSmeltHeroInfo;
        m_cmds_process_map["smeltHeroRefresh"] = ProcessSmeltHeroRefresh;
        m_cmds_process_map["smeltHero"] = ProcessSmeltHero;
        m_cmds_process_map["goldenHeroInfo"] = ProcessGoldenHeroInfo;
        m_cmds_process_map["goldenHero"] = ProcessGoldenHero;
        m_cmds_process_map["queryEpicHeros"] = ProcessEpicHeros;
        m_cmds_process_map["getEpicHero"] = ProcessGetEpicHero;
        m_cmds_process_map["upEpicHeroInfo"] = ProcessUpEpicHeroInfo;
        m_cmds_process_map["upEpicHero"] = ProcessUpEpicHero;
        m_cmds_process_map["heroUseGem"] = ProcessHeroUseGem;//~{S"P[J9SC5@>_~}

        //~{<<D\~}
        m_cmds_process_map["getSkillInfo"] = ProcessGetSkillInfo;
        m_cmds_process_map["getSkillList"] = ProcessCharSkills;
        m_cmds_process_map["upSkillInfo"] = ProcessUpSkillInfo;
        m_cmds_process_map["upSkill"] = ProcessUpSkill;
        //~{D'7(~}
        m_cmds_process_map["getMagicInfo"] = ProcessGetMagicInfo;
        m_cmds_process_map["getMagicList"] = ProcessMagicList;
        m_cmds_process_map["setCombatMagic"] = ProcessSetCombatMagic;
        m_cmds_process_map["cancelCombatMagic"] = ProcessCancelCombatMagic;
        m_cmds_process_map["swapCombatMagic"] = ProcessSwapCombatMagic;
        m_cmds_process_map["getCombatMagic"] = ProcessCharCombatMagics;

        //~{W018~}
        m_cmds_process_map["showEquiptList"] = ProcessShowEquipts;
        m_cmds_process_map["upEquipt"] = ProcessEquipt;    //~{W018~}
        m_cmds_process_map["downEquipt"] = ProcessUnequipt;//~{P6OBW018~}
        m_cmds_process_map["compoundEquiptInfo"] = ProcessCompoundEquiptInfo;
        m_cmds_process_map["compoundEquiptOneKey"] = ProcessCompoundEquiptOneKey;
        m_cmds_process_map["compoundEquipt"] = ProcessCompoundEquipt;
        m_cmds_process_map["upgradeEquiptInfo"] = ProcessUpgradeEquiptInfo;
        m_cmds_process_map["upgradeEquipt"] = ProcessUpgradeEquipt;

        //~{ObG61&J/~}
        m_cmds_process_map["addEquiptBaoshiSlot"] = ProcessAddEquiptBaoshiSlot;
        m_cmds_process_map["queryBaoshiTips"] = ProcessQueryBaoshiInfo;
        m_cmds_process_map["queryBaoshiList"] = ProcessQueryBaoshiList;
        m_cmds_process_map["inlayBaoshi"] = ProcessInlayBaoshi;
        m_cmds_process_map["removeBaoshi"] = ProcessRemoveBaoshi;
        m_cmds_process_map["combineBaoshi"] = ProcessCombineBaoshi;
        m_cmds_process_map["combineAllBaoshi"] = ProcessCombineAllBaoshi;

        //~{ADLl~}
        m_cmds_process_map["chat"] = ProcessChat;

        //~{5XM<9X?(~}
        m_cmds_process_map["getMapStageList"] = ProcessCharMapTempo;
        m_cmds_process_map["getStageStrongholdList"] = ProcessCharStageTempo;
        m_cmds_process_map["getStrongholdInfo"] = ProcessCharStronghold;
        m_cmds_process_map["getStrongholdBox"] = ProcessGetStrongholdBox;
        m_cmds_process_map["getStageFinishLoot"] = ProcessGetStageFinishLoot;
        m_cmds_process_map["checkStageFinish"] = ProcessCheckStageFinish;
        m_cmds_process_map["goldAttackStronghold"] = ProcessGoldAttackStronghold;

        //~{3G3X~}
        m_cmds_process_map["queryCityBuildingList"] = ProcessQueryCityBuildingList;
        m_cmds_process_map["queryCityBuilding"] = ProcessQueryCityBuilding;
        m_cmds_process_map["levelUpCityBuilding"] = ProcessLevelUpCityBuilding;
        m_cmds_process_map["recruit"] = ProcessRecruit;
        m_cmds_process_map["levy"] = ProcessLevy;

        //~{130|~}
        m_cmds_process_map["showBackpack"] = ProcessShowBag;//~{OTJ>130|~}
        m_cmds_process_map["sortBag"] = ProcessSortBag;//~{U{@m130|~}
        m_cmds_process_map["sellBagItem"] = ProcessSellItem;//~{Bt3vNoF7~}
        m_cmds_process_map["buyBagSlot"] = ProcessBuyBagSlot;//~{9:Br130|N;VC~}
        m_cmds_process_map["swapBagSlot"] = ProcessSwapBagSlot;//~{=;;;130|N;~}
        m_cmds_process_map["openBagSlot"] = ProcessOpenBagSlot;//~{J9SC130|NoF7~}
        m_cmds_process_map["openBagLibao"] = ProcessOpenBagLibao;//~{4r?*130|N;@q0|~}
        m_cmds_process_map["getEquipInfo"] = ProcessGetEquipInfo;//~{5%6@GkGsW018PEO"~}
        m_cmds_process_map["getSysEquipInfo"] = ProcessGetSysEquipInfo;//~{7GMf<RW018PEO"~}
        m_cmds_process_map["getGemInfo"] = ProcessGetGemInfo;//~{5%6@GkGs1&NoPEO"~}
        m_cmds_process_map["getLibaoInfo"] = ProcessGetLibaoInfo;//~{5%6@GkGs@q0|PEO"~}
        m_cmds_process_map["showGemList"] = ProcessShowGems;//~{GkGsMf<R5@>_AP1m~}

        //~{IL3G~}
        m_cmds_process_map["queryMallInfo"] = ProcessQueryMallInfo;//~{2iQ/IL3G~}
        m_cmds_process_map["queryMallHotInfo"] = ProcessQueryMallHotInfo;//~{2iQ/IL3GHHBt~}
        m_cmds_process_map["buyMallGoods"] = ProcessBuyMallGoods;//~{9:BrIL3GILF7~}

        //~{HNNq~}
        m_cmds_process_map["getTaskList"] = ProcessTaskList;        //~{HNNqAP1m~}
        m_cmds_process_map["getTaskInfo"] = ProcessTaskInfo;        //~{HNNqOjGi~}
        m_cmds_process_map["getTaskAward"] = ProcessTaskDone;        //~{AlH!HNNq=1@x~}

        /*************** ~{PBJVR}5<=S?Z~} **********************/
        m_cmds_process_map["setGuideComplete"] = ProcessSetGuideState;//~{IhVCPBJVR}5<W4L,~}

        //~{811>~}
        m_cmds_process_map["getCopyMapList"] = ProcessGetCopyMapList;//~{;qH!811>5XM<AP1m~}
        m_cmds_process_map["getCopyList"] = ProcessGetCopyList;//~{;qH!D3UB=Z811>AP1m~}
        m_cmds_process_map["getCopyInfo"] = ProcessGetCopyInfo;//~{;qH!811>>_LePEO"~}
        m_cmds_process_map["resetCopy"] = ProcessResetCopy;//~{VXVCD3811>~}
        m_cmds_process_map["addCopyTimes"] = ProcessAddCopyTimes;//~{Tv<SD3811>9%;w4NJ}~}
        m_cmds_process_map["queryCopyShop"] = ProcessQueryCopyShop;//~{811>IL5j~}
        m_cmds_process_map["buyCopyShopGoods"] = ProcessBuyCopyShopGoods;
        m_cmds_process_map["sweepStart"] = ProcessSweep;
        m_cmds_process_map["getCopyFinishReward"] = ProcessGetCopyFinishReward;//~{AlH!811>5XM<M(9X=1@x~}

        //~{3GMb3G3X~}
        m_cmds_process_map["queryWildCitys"] = ProcessQueryWildCitys;//~{;qH!3GMb3G3XAP1m~}
        m_cmds_process_map["wildCityLevy"] = ProcessWildCityLevy;//~{R0Mb3G3XJUK0~}
        m_cmds_process_map["wildCityDefense"] = ProcessWildCityDefense;//~{R0Mb3G3XIhVC3GJX~}

        //~{>:<<3!~}
        m_cmds_process_map["queryArenaRankList"] = ProcessQueryArenaRankList;//~{;q5C>:<<EEC{AP1m~}
        m_cmds_process_map["queryArenaInfo"] = ProcessQueryArenaInfo;//~{2iQ/WT<:5D>:<<PEO"~}
        m_cmds_process_map["queryArenaRankRewards"] = ProcessQueryRankRewards;//~{2iQ/EEPP=1@x~}
        m_cmds_process_map["buyArena"] = ProcessBuyArena; //~{9:BrLtU=4NJ}~}
        m_cmds_process_map["queryArenaGoodsList"] = ProcessQueryArenaGoodsList;//~{>:<<;}7VILF7~}
        m_cmds_process_map["getArenaGoods"] = ProcessGetArenaGoods;//~{AlH!>:<<;}7VILF7~}

        //~{IyM{~}
        m_cmds_process_map["queryPrestigeAward"] = ProcessQueryPrestigeAward;
        m_cmds_process_map["getPrestigeAward"] = ProcessGetPrestigeAward;
        m_cmds_process_map["queryPrestigeShop"] = ProcessQueryPrestigeShop;
        m_cmds_process_map["buyPrestigeShopGoods"] = ProcessBuyPrestigeShopGoods;
        m_cmds_process_map["getPrestigeTaskList"] = ProcessPrestigeTaskList;
        m_cmds_process_map["getPrestigeTaskAward"] = ProcessPrestigeTaskDone;

        //~{9R;zL=Kw~}
        m_cmds_process_map["queryExploreCaveList"] = ProcessQueryExploreCaveList;
        m_cmds_process_map["queryExploreCaveInfo"] = ProcessQueryExploreCaveInfo;
        m_cmds_process_map["explore"] = ProcessExplore;
        m_cmds_process_map["getExploreReward"] = ProcessGetExploreReward;

        //~{IqAiK~~}
        m_cmds_process_map["getShenlingList"] = ProcessGetShenlingList;
        m_cmds_process_map["getShenlingInfo"] = ProcessGetShenlingInfo;
        m_cmds_process_map["refreshSkill"] = ProcessRefreshSkill;
        m_cmds_process_map["resetShenling"] = ProcessResetShenling;
        m_cmds_process_map["buyShenlingTimes"] = ProcessBuyShenlingTimes;
        m_cmds_process_map["queryShenlingShop"] = ProcessQueryShenlingShop;
        m_cmds_process_map["buyShenlingShopGoods"] = ProcessBuyShenlingShopGoods;

        //~{EEPP0q~}
        m_cmds_process_map["getCharRankList"] = ProcessGetCharRanklist;
        m_cmds_process_map["getSilverRankList"] = ProcessGetSilverRanklist;
        m_cmds_process_map["getHeroRankList"] = ProcessGetHeroRanklist;
        m_cmds_process_map["getStrongholdRanklist"] = ProcessGetStrongholdRanklist;
        m_cmds_process_map["getCopyRanklist"] = ProcessGetCopyRanklist;
        m_cmds_process_map["getShenlingRanklist"] = ProcessGetShenlingRanklist;
        m_cmds_process_map["getCharAttackRankList"] = ProcessGetCharAttackRanklist;

        //~{:CSQ~}
        m_cmds_process_map["getFriendsList"] = ProcessGetFriendsList;//~{:CSQAP1m~}
        m_cmds_process_map["dealFriends"] = ProcessDealFriends;//~{:CSQ2YWw~}
        m_cmds_process_map["congratulation"] = ProcessCongratulation;//~{W#:X:CSQ~}
        m_cmds_process_map["getContratulationList"] = ProcessGetCongratulations;//~{W#:XAP1m~}
        m_cmds_process_map["getRecvedContratulationList"] = ProcessGetRecvedCongratulations;//~{JU5=W#:XAP1m~}

        //~{2X1&M<~}
        m_cmds_process_map["getRobEvent"] = ProcessGetRobEvent;
        m_cmds_process_map["getBaseTreasureInfo"] = ProcessGetBaseTreasureInfo;
        m_cmds_process_map["getAllTreasureList"] = ProcessGetAllTreasureList;
        m_cmds_process_map["getCharTreasure"] = ProcessGetCharTreasure;
        m_cmds_process_map["dealTreasure"] = ProcessDealTreasure;
        m_cmds_process_map["getTreasureList"] = ProcessGetTreasureList;
        m_cmds_process_map["getTreasureInfo"] = ProcessGetTreasureInfo;
        m_cmds_process_map["quitTreasure"] = ProcessQuitTreasure;

        //pk~{3!~}
        m_cmds_process_map["queryPKList"] = ProcessQueryPKList;
        m_cmds_process_map["queryPKInfo"] = ProcessQueryPKInfo;
        m_cmds_process_map["queryPKTop"] = ProcessQueryPKTop;
        m_cmds_process_map["dealPK"] = ProcessDealPK;
        m_cmds_process_map["queryPKRooms"] = ProcessQueryPKRooms;
        m_cmds_process_map["queryRoomInfo"] = ProcessQueryRoomInfo;
        m_cmds_process_map["PKCreateRoom"] = ProcessPKCreateRoom;
        m_cmds_process_map["PKJoinRoom"] = ProcessPKJoinRoom;
        m_cmds_process_map["PKLeaveRoom"] = ProcessPKLeaveRoom;
        m_cmds_process_map["PKKickPlayer"] = ProcessPKKickPlayer;
        m_cmds_process_map["PKChangeSet"] = ProcessPKChangeSet;
        m_cmds_process_map["PKOpenSet"] = ProcessPKOpenSet;
        m_cmds_process_map["PKCloseSet"] = ProcessPKCloseSet;
        m_cmds_process_map["PKSetPassword"] = ProcessPKSetPassword;
        m_cmds_process_map["PKSetBet"] = ProcessPKSetBet;
        m_cmds_process_map["PKSetReady"] = ProcessPKSetReady;
        m_cmds_process_map["PKStart"] = ProcessPKStart;
        m_cmds_process_map["PKRoomInvite"] = ProcessPKInvite;

        //~{C?HU1XWv~}
        m_cmds_process_map["dailyScoreTaskList"] = ProcessDailyScoreTaskList;
        m_cmds_process_map["dailyScoreRewardList"] = ProcessDailyScoreRewardList;
        m_cmds_process_map["dailyScoreTaskInfo"] = ProcessDailyScoreTaskInfo;
        m_cmds_process_map["dailyScoreDeal"] = ProcessDailyScoreDeal;

        //~{PE<~=S?Z~}
        m_cmds_process_map["getMailList"] = ProcessGetMailList;
        m_cmds_process_map["queryMail"] = ProcessQueryMailContent;
        m_cmds_process_map["delMail"] = ProcessDeleteMail;
        m_cmds_process_map["sendMail"] = ProcessSendMail;
        m_cmds_process_map["getUnread"] = ProcessGetUnread;
        m_cmds_process_map["getUnreadList"] = ProcessGetUnreadList;
        m_cmds_process_map["getMailAttach"] = ProcessGetMailAttach;
        m_cmds_process_map["getAllMailAttach"] = ProcessGetAllMailAttach;

        //~{9+;a~}
        m_cmds_process_map["getGuildList"] = ProcessGetGuildList;
        m_cmds_process_map["dealJoinGuild"] = ProcessDealJoinGuild;
        m_cmds_process_map["createGuild"] = ProcessCreateGuild;
        m_cmds_process_map["getGuildInfo"] = ProcessGetGuildInfo;
        m_cmds_process_map["donateGuild"] = ProcessDonate;
        m_cmds_process_map["dissolveGuild"] = ProcessDissolveGuild;
        m_cmds_process_map["quitGuild"] = ProcessQuitGuild;
        m_cmds_process_map["setGuildInfo"] = ProcessSetGuildInfo;
        m_cmds_process_map["getGuildActionList"] = ProcessGetGuildActionList;
        m_cmds_process_map["getGuildBoxList"] = ProcessGetGuildBoxList;
        m_cmds_process_map["getGuildBoxReward"] = ProcessGetGuildBoxReward;
        m_cmds_process_map["getGuildMemberList"] = ProcessGetGuildMemberList;
        m_cmds_process_map["dealGuildMember"] = ProcessDealGuildMember;
        m_cmds_process_map["getGuildEventList"] = ProcessGetGuildEventList;
        m_cmds_process_map["getGuildApplyList"] = ProcessGetGuildApplyList;
        m_cmds_process_map["dealGuildApply"] = ProcessDealGuildApply;
        m_cmds_process_map["getGuildSkillList"] = ProcessGetGuildSkillList;
        m_cmds_process_map["upgradeGuildSkill"] = ProcessUpgradeGuildSkill;
        m_cmds_process_map["getGuildMoshenInfo"] = ProcessGetGuildMoshenInfo;

        //~{UR;X9&D\~}
        m_cmds_process_map["queryFindBackList"] = ProcessQueryFindBackList;
        m_cmds_process_map["findBack"] = ProcessFindBack;

        //~{;qH!@kO_N4Mj3ILaJ>~}
        m_cmds_process_map["getLogOutList"] = ProcessGetLogOutList;

        //~{D?1j~}
        m_cmds_process_map["getGoalTaskList"] = ProcessGoalTaskList;
        m_cmds_process_map["getGoalLevelList"] = ProcessGoalLevelList;
        m_cmds_process_map["getGoalReward"] = ProcessGoalReward;
        m_cmds_process_map["getGoalShop"] = ProcessGoalShop;
        m_cmds_process_map["buyGoalGood"] = ProcessBuyGoalGood;

        //~{8w@`;n6/~}
        m_cmds_process_map["getTopButtonList"] = ProcessTopButtonList;//~{;q5C6%@804E%PEO"~}
        m_cmds_process_map["querySignInfo"] = ProcessQuerySignInfo;//~{G)5==gCf~}
        m_cmds_process_map["doSign"] = ProcessSign;//~{G)5=~}
        m_cmds_process_map["querySignShop"] = ProcessQuerySignShop;//~{G)5=;}7VIL5j~}
        m_cmds_process_map["buySignShopGoods"] = ProcessBuySignShopGoods;//~{9:BrG)5=;}7VILF7~}
        m_cmds_process_map["queryOnlineLibaoInfo"] = ProcessQueryOnlineLibaoInfo;//~{TZO_@q0|~}
        m_cmds_process_map["queryRechargeEvent"] = ProcessQueryRechargeEvent;//~{2iQ/3dV5;n6/PEO"~}
        m_cmds_process_map["queryFirstRechargeEvent"] = ProcessQueryFirstRechargeEvent;//~{2iQ/JW3d;n6/PEO"~}

        //~{O^J1;n6/~}
        m_cmds_process_map["queryTimeLimitActionList"] = ProcessQueryTimeLimitActionList;//~{2iQ/O^J1;n6/AP1m~}
        m_cmds_process_map["querySevenInfo"] = ProcessQuerySevenInfo;//~{2iQ/F_HU;n6/PEO"~}
        m_cmds_process_map["queryEquiptLevelInfo"] = ProcessQueryEquiptLevelInfo;//~{2iQ/G?;/;n6/PEO"~}
        m_cmds_process_map["queryHeroStarInfo"] = ProcessQueryHeroStarInfo;//~{2iQ/I}PG;n6/PEO"~}
        m_cmds_process_map["queryHeroPackInfo"] = ProcessQueryHeroPackInfo;//~{2iQ/K:0|;n6/PEO"~}
        m_cmds_process_map["queryHeroActionInfo"] = ProcessQueryHeroActionInfo;//~{2iQ/S"P[;n6/PEO"~}
        m_cmds_process_map["queryStrongholdActionInfo"] = ProcessQueryStrongholdActionInfo;//~{2iQ/3e9X;n6/PEO"~}

        //BOSS~{U=~}
        m_cmds_process_map["dealBossScene"] = ProcessBossScene;//~{=xHk~}boss~{U=3!>0~}
        m_cmds_process_map["getBossInfo"] = ProcessGetBossInfo;//~{;qH!~}Boss~{PEO"~}
        m_cmds_process_map["getBossHp"] = ProcessGetBossHp;//~{;qH!~}Boss~{51G0Q*A?~}
        m_cmds_process_map["inspireBossDamage"] = ProcessInspire;//~{9DNh~}
        m_cmds_process_map["getBossCoolTime"] = ProcessGetCoolTime;//~{;qH!9%4r~}Boss~{@dH4J1<d~}
        m_cmds_process_map["endBossCoolTime"] = ProcessEndCoolTime;//~{=aJx9%4r~}Boss~{@dH4J1<d~}
        m_cmds_process_map["getBossRank"] = ProcessGetBossRank;//~{;qH!6T~}Boss~{Tl3IIK:&5DMf<RAP1m~}
        m_cmds_process_map["getBossLogEvent"] = ProcessGetBossLogEvent;//~{;qH!6T~}Boss~{Tl3IIK:&HUV>~}

        //~{C?HU;n6/~}
        m_cmds_process_map["queryDailyActionList"] = ProcessGetDailyActionList;//~{;q5C;n6/=gCf~}

        //~{RxPP~}
        m_cmds_process_map["getBankSilver"] = ProcessGetBankSilver;//~{;qH!RxPP3oBk~}
        m_cmds_process_map["dealBankSilver"] = ProcessDealBankSilver;//~{4&@mRxPP4f?n~}
        m_cmds_process_map["getBankCaseState"] = ProcessGetBankCaseState;//~{RxPPOnD?W4L,~}
        m_cmds_process_map["buyBankCase"] = ProcessBuyBankCase;//~{M6WJ~}
        m_cmds_process_map["getBankFeedback"] = ProcessGetBankFeedback;//~{75@{~}
        m_cmds_process_map["getCaseInfo"] = ProcessGetCaseInfo;//~{;qH!OnD?PEO"~}
        m_cmds_process_map["getBankList"] = ProcessGetBankList;//~{;qH!OnD?AP1m~}

        //~{W*EL;n6/~}
        m_cmds_process_map["queryLotteryEventInfo"] = ProcessQueryLotteryEventInfo;
        m_cmds_process_map["queryLotteryEventAwards"] = ProcessQueryLotteryEventAwards;
        m_cmds_process_map["getLotteryScore"] = ProcessGetLotteryScore;
        m_cmds_process_map["getLottery"] = ProcessGetLottery;

        //~{EDBtPP~}
        m_cmds_process_map["auctionSellItem"] = ProcessAuctionSellItem;
        m_cmds_process_map["auctionPlaceBid"] = ProcessAuctionPlaceBid;
        m_cmds_process_map["auctionRemoveItem"] = ProcessAuctionRemoveItem;
        m_cmds_process_map["auctionListBidderItems"] = ProcessAuctionListBidderItems;
        m_cmds_process_map["auctionListOwnerItems"] = ProcessAuctionListOwnerItems;
        m_cmds_process_map["auctionListItems"] = ProcessAuctionListItems;

        /**************~{M3R;=S?Z~}****************/
        m_cmds_process_map["getConfirm"] = ProcessGetConfirmInfo;//~{;qH!=p1RO{7QH7HOW4L,~}
        m_cmds_process_map["enableConfirm"] = ProcessSetConfirmInfo;//~{IhVC=p1RO{7QJG7qPhR*LaJ>~}
        m_cmds_process_map["dealCoolTime"] = ProcessSpeedXXX;//~{<SKY~}
        m_cmds_process_map["dealGet"] = ProcessGetXXX;//~{AlH!~}

        m_cmds_process_map["getSysGemList"] = ProcessGetSysGemList;//~{O5M35@>_AP1m~}
        m_cmds_process_map["buyGem"] = ProcessBuyGem;//~{9:Br5@>_~}
        m_cmds_process_map["getGemCount"] = ProcessGetGemCount;//~{2iQ/5@>_J}A?~}
        m_cmds_process_map["getGemPrice"] = ProcessGetGemPrice;//~{2iQ/5@>_<[8q~}

        //~{2iQ/HU3#9+8f~}
        m_cmds_process_map["queryNotices"] = ProcessQueryAdminNotice;

        //~{AlH!=1@x~} ~{T]4f=1@x~}
        m_cmds_process_map["getCharRewards"] = ProcessGetCharRewards;

        //~{U=67~}
        m_cmds_process_map["challenge"] = ProcessChallenge;//~{7"FpLtU=~}
        m_cmds_process_map["new_combat_quit"] = ProcessQuitChallenge;//~{MK3vLtU=~}
        m_cmds_process_map["new_combat_act"] = ProcessGameAct;//~{EF>VPP6/~}
        m_cmds_process_map["new_combat_pass"] = ProcessGamePass;//~{EF>V9}EF~}
        m_cmds_process_map["new_combat_sign"] = ProcessCombatSign;//~{?M;'6KU=67?*FtPE:E~}
        m_cmds_process_map["new_combat_auto"] = ProcessGameAuto;//~{EF>VWT6/~}
        m_cmds_process_map["new_combat_cast"] = ProcessGameCastMagic;//~{EF>VJ)7(~}
        m_cmds_process_map["new_combat_next"] = ProcessGameNext;//~{EF>V<LPx~}

        //~{U=67;X7E~}
        m_cmds_process_map["getCombatRecord"] = ProcessGetCombatRecord;

        //~{6`HKU=67~}
        m_cmds_process_map["multi_challenge"] = ProcessMultiChallenge;//~{7"FpLtU=~}
        m_cmds_process_map["multi_quitChallenge"] = ProcessMultiQuitChallenge;//~{MK3vLtU=~}
        m_cmds_process_map["multi_myAct"] = ProcessMultiGameAct;//~{EF>VPP6/~}
        m_cmds_process_map["multi_combatSign"] = ProcessMultiCombatSign;//~{?M;'6KU=67?*FtPE:E~}
        m_cmds_process_map["multi_continueChallenge"] = ProcessMultiContinueChallenge;//~{<LPxLtU=~}
        m_cmds_process_map["multi_queryCombatInfo"] = ProcessMultiQueryCombatInfo;//~{<LPxLtU=~}

        //~{V\EEPP~}
        m_cmds_process_map["getWeekRankingsInfo"] = ProcessGetWeekRankingsInfo;    //~{1>V\EEPPPEO"~}
        m_cmds_process_map["getLastWeekRankingsInfo"] = ProcessGetLastWeekRankingsInfo;    //~{IOV\EEPPPEO"~}

#ifdef QQ_PLAT
        //~{2iQ/;FWj=gCf#:~}cmd:getYellowEvent
        m_cmds_process_map["getYellowEvent"] = ProcessQueryQQYellowEvent;
        //~{2iQ/;FWjPBJV@q0|~} cmd~{#:~}queryQQnewbieLibao
        m_cmds_process_map["queryQQnewbieLibao"] = ProcessQueryQQNewbieLibao;
        //~{2iQ/;FWjC?HU@q0|~} cmd~{#:~}queryQQDailyLibao
        m_cmds_process_map["queryQQDailyLibao"] = ProcessQueryQQDailyLibao;
        //~{2iQ/;FWj3I3$@q0|~} cmd~{#:~}queryQQLevelLibao
        m_cmds_process_map["queryQQLevelLibao"] = ProcessQueryQQLevelLibao;
#endif

        /**************~{DZ2?=S?Z~}****************/
        m_internal_cmds_process_map["reload"] = ProcessReload;
        m_internal_cmds_process_map["scheduleEvent"] = ProcessScheduleEvent;
        m_internal_cmds_process_map["keepDb"] = ProcessKeepDb;
        m_internal_cmds_process_map["logout"] = ProcessOffline;
        m_internal_cmds_process_map["checkRecharge"] = ProcessCheckRecharge;
        m_internal_cmds_process_map["checkPack"] = ProcessCheckPack;

        m_internal_cmds_process_map["new_combat_start"] = ProcessStartGame;//~{EF>V?*J<~}
        m_internal_cmds_process_map["new_combat_end"] = ProcessGameEnd;//~{EF>V=aJx~}
        m_internal_cmds_process_map["new_combat_deal"] = ProcessGameDeal;//~{7"EF2YWw~}
        m_internal_cmds_process_map["new_combat_timeout"] = ProcessGameTimeout;//~{2YWw3,J1~}
        m_internal_cmds_process_map["new_combat_npcAct"] = ProcessNPCAct;//~{EF>V~}NPC~{6/Ww~}

        m_internal_cmds_process_map["multi_startGame"] = ProcessMultiStartGame;//~{EF>V?*J<~}
        m_internal_cmds_process_map["multi_gameEnd"] = ProcessMultiGameEnd;//~{EF>V=aJx~}
        m_internal_cmds_process_map["multi_actTimeout"] = ProcessMultiGameTimeout;//~{2YWw3,J1~}
        m_internal_cmds_process_map["multi_npcAct"] = ProcessMultiNPCAct;//~{EF>V~}NPC~{6/Ww~}


        m_internal_cmds_process_map["treasureDone"] = ProcessTreasureDone;//~{2X1&M<Mj3I~}
        m_internal_cmds_process_map["onlineLibaoUpdate"] = ProcessOnlineLibaoUpdate;//~{TZO_@q0|8|PB~}
        m_internal_cmds_process_map["sweepDone"] = ProcessSweepDone;//~{I(54=aJx~}
        m_internal_cmds_process_map["buffChange"] = ProcessBuffChange;//~{O^J1TvRfW4L,8D1d~}
        m_internal_cmds_process_map["cityRecruitUpdate"] = ProcessCityRecruitUpdate;//~{UPD<04E%8|PB~}
        m_internal_cmds_process_map["smeltRefreshUpdate"] = ProcessSmeltRefreshUpdate;//~{H[A6K"PB04E%8|PB~}

        m_internal_cmds_process_map["bankCanGet"] = ProcessBankCaseCanGet;//~{M6WJ?IAlH!~}

        //~{O5M39+8fO`9X~}
        m_internal_cmds_process_map["addAdminNotice"] = ProcessNewAdminNotice;
        m_internal_cmds_process_map["changeAdminNotice"] = ProcessChangeAdminNotice;
        m_internal_cmds_process_map["deleteAdminNotice"] = ProcessDeleteAdminNotice;
        m_internal_cmds_process_map["sendAdminNotice"] = ProcessSendAdminNotice;

        //~{7"KMO5M3O{O"~}
        m_internal_cmds_process_map["broadCastMsg"] = ProcessBroadCastMsg;
        m_internal_cmds_process_map["sendMsg"] = ProcessSendMsg;

        //~{9X7~4&@m~}
        m_internal_cmds_process_map["shutdown"] = ProcessShutdown;

    }

    virtual bool work(actionmessage& task)       // ~{TZP)Mj3IJ5<JHNNq~}.
    {
        try
        {
            log_msg(task.cmd);
            if (task.from()== 1)
            {
                session_ptr psession;
                task.getsession(psession);

                boost::unordered_map<std::string, pFuncInternalProcessCmds>::iterator it = m_internal_cmds_process_map.find(task.cmd);
                if (it != m_internal_cmds_process_map.end())
                {
                    (*(it->second))(task.getRecvObj());
                }
                else
                {
                    INFO("work recv internal cmd "<<task.cmd<<endl);
                }
                return true;
            }
            INFO("main work recv cmd "<<task.cmd<<","<<splsTimeStamp()<<endl);
            //uint64_t recv = splsTimeStamp();
            session_ptr psession;
            task.getsession(psession);
            if (!psession.get())
            {
                INFO("action worker get no session"<<endl);
                return true;
            }
#ifdef DEBUG_PER
            uint64_t recv_time = 0;
            if (psession.get() && psession->is_debug_)
            {
                recv_time = splsTimeStamp();
            }
#endif
            using namespace std;
            using namespace boost;
            using namespace json_spirit;

            bool bFind = false;
            boost::unordered_map<std::string, pFuncProcessCmds>::iterator it;
            if (psession->state() == STATE_AUTHED)
            {
                it = m_cmds_process_map.find(task.cmd);
                bFind = (it != m_cmds_process_map.end());
            }
            else
            {
                it = m_spe_cmds_process_map.find(task.cmd);
                bFind = (it != m_spe_cmds_process_map.end());
            }
            if (bFind)
            {
                Object robj;
                int ret = (*(it->second))(psession, task.getRecvObj(), robj);
                if (HC_SUCCESS_NO_RET != ret)
                {
                    robj.push_back( Pair("cmd", task.cmd));

                    if (HC_ERROR_NO_RET != ret)
                    {
                        robj.push_back( Pair("s", ret));
                        if (ret != HC_SUCCESS)
                        {
                            std::string msg = getErrMsg(ret);
                            if ("" != msg)
                            {
                                robj.push_back( Pair("msg", msg));
                            }
                        }
                    }
                    psession->send(write(robj, json_spirit::raw_utf8));
                }
                CharData* pc = NULL;
                getSessionChar(psession, pc);
                if (pc)
                {
                    pc->NotifyCharData_(psession);
                }
#ifdef DEBUG_PER
                if (psession.get() && psession->is_debug_)
                {
                    cout<<"cmd("<<task.cmd<<") process cost "<<(double(splsTimeStamp()-recv_time)/1000)<<" ms|";
                    cout<<(double(recv_time-task._recv_time)/1000)<<endl;
                    //cout<<"|getjob cost "<<(double(task._get_time-task._start_get)/1000)<<" us"<<endl;
                }
#endif
            }
            else
            {
                Object robj;
                robj.push_back( Pair("cmd", task.cmd));
                robj.push_back( Pair("s", 401));
                robj.push_back( Pair("m", "unknow cmd"));
                psession->send(write(robj, json_spirit::raw_utf8));
            }
            return true;
        }
        catch (std::exception& e)
        {
            syslog(LOG_ERR, "action work , Exception: %s", e.what());
            syslog(LOG_ERR, "cmd: %s", task.cmd.c_str());
            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);
            for (int i = 0; i < nSize; i++)
            {
                syslog(LOG_ERR, symbols[i]);
            }
            free(symbols);
        }
        return true;
    }
private:
    boost::unordered_map<std::string, pFuncProcessCmds> m_cmds_process_map;
    boost::unordered_map<std::string, pFuncProcessCmds> m_spe_cmds_process_map;
    boost::unordered_map<std::string, pFuncInternalProcessCmds> m_internal_cmds_process_map;
};

