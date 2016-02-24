
#include "new_combat.hpp"
#include <mysql/mysql.h>
#include "Database.h"
#include "errcode_def.h"
#include "data.h"
#include "singleton.h"
#include "copy.h"
#include "wild.h"
#include "arena.h"
#include "shenling.h"
#include "treasure.h"
#include "pk.h"
#include "boss.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include <algorithm>
#include "combatRecord.h"
#include "magic.h"

using namespace net;
using namespace json_spirit;

#define IS_NPC(x) ((x)<0)
#define IS_NOT_NPC(x) ((x)>0)

extern volatile int iArenaFreeTimes;

extern Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
extern int InsertInternalActionWork(json_spirit::mObject& obj);
extern void InsertCombat(chessCombatCmd& pcombatCmd);

extern std::string strQuitCombatMailContent;
extern std::string strQuitCombatMailTitle;

//发起挑战
int createChessCombat(int cid, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (cdata == NULL)
    {
        ERR();
        return HC_ERROR;
    }
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    int toid = 0;
    READ_INT_FROM_MOBJ(toid,o,"to");
    robj.push_back( Pair("type", type));
    int in_combat = 1;
    if (type == COMBAT_TYPE_STRONGHOLD)
    {
        if (toid < 1)
        {
            return HC_ERROR;
        }
        boost::shared_ptr<baseStronghold> shold = Singleton<mapMgr>::Instance().GetBaseStrongholdData(toid);
        if (!shold.get())
        {
            return HC_ERROR;
        }
        if (shold->m_type != STRONGHOLD_TYPE_NORMAL
            && shold->m_type != STRONGHOLD_TYPE_EXPLORE
            && shold->m_type != STRONGHOLD_TYPE_CAPTURE)
        {
            return HC_ERROR;
        }
        bool random_extra = false;
        boost::shared_ptr<CharStrongholdData> cd = Singleton<mapMgr>::Instance().GetCharStrongholdData(cdata->m_id,shold->m_mapid,shold->m_stageid, shold->m_pos);
        if (cd.get())
        {
            //未开放
            if (cd->m_state < 0)
            {
                return HC_ERROR;
            }
            //探索需要判断每日次数
            if (shold->m_type == STRONGHOLD_TYPE_EXPLORE)
            {
                if (cd->isPassed())
                {
                    random_extra = true;//通关过的探索才有随机倍数
                    if (cdata->queryExtraData(char_data_type_daily,char_data_daily_explore_begin+shold->m_id))
                    {
                        return HC_ERROR_NOT_ENOUGH_TIMES;
                    }
                }
            }
            //俘虏事件判断是否上限
            if (shold->m_type == STRONGHOLD_TYPE_CAPTURE && cd->isPassed())
            {
                return HC_ERROR;
            }
        }
        else
        {
            return HC_ERROR;
        }
        chessCombat* pb = Singleton<chessCombatMgr>::Instance().createStrongholdCombat(*cdata, toid, random_extra);
        if (NULL == pb)
        {
            ERR();
            return HC_ERROR;
        }
        //返回战斗信息
        pb->getCombatInfo(cdata->m_id, robj);
    }
    else if(type == COMBAT_TYPE_COPY)
    {
        if (toid < 1)
        {
            return HC_ERROR;
        }
        boost::shared_ptr<baseCopy> bc = Singleton<copyMgr>::Instance().getCopyById(toid);
        if (!bc.get())
        {
            return HC_ERROR;
        }
        if (cdata->m_level < bc->m_openLevel)
        {
            return HC_ERROR_NEED_MORE_LEVEL;
        }
        int has_attack = cdata->queryExtraData(char_data_type_daily,char_data_daily_attack_copy);
        if (has_attack >= iCopyTotal)
        {
            return HC_ERROR_NOT_ENOUGH_TIMES;
        }
        boost::shared_ptr<CharCopyData> cd = Singleton<copyMgr>::Instance().getCharCopy(cdata->m_id,bc->m_mapid,bc->m_id);
        if (cd.get())
        {
            if (cd->m_can_attack <= 0)
            {
                return HC_ERROR_NOT_ENOUGH_TIMES;
            }
        }
        else
        {
            return HC_ERROR;
        }
        chessCombat* pb = Singleton<chessCombatMgr>::Instance().createCopyCombat(*cdata, toid);
        if (NULL == pb)
        {
            ERR();
            return HC_ERROR;
        }
        //返回战斗信息
        pb->getCombatInfo(cdata->m_id, robj);
    }
    else if(type == COMBAT_TYPE_ARENA)
    {
        if (IS_NPC(toid))
        {
            return HC_ERROR;
        }
        else
        {
            if (cdata->m_id == toid)
            {
                return HC_ERROR;
            }
            boost::shared_ptr<CharArenaData> rd = Singleton<arenaMgr>::Instance().getArenaData(cdata->m_id);
            if (!rd.get() || !rd->getCharData().get() || rd->m_under_attack > 0 || rd->m_attack_who > 0)
            {
                ERR();
                return HC_ERROR_SELF_IN_COMBAT;
            }
            boost::shared_ptr<CharArenaData> td = Singleton<arenaMgr>::Instance().getArenaData(toid);
            if (!td.get() || !td->getCharData().get() || td->m_under_attack > 0 || td->m_attack_who > 0)
            {
                ERR();
                return HC_ERROR_TARGET_IN_COMBAT;
            }
            CharData* tc = td->getCharData().get();
            int cd_sec = cdata->queryExtraData(char_data_type_daily, char_data_daily_arena_cd) - time(NULL);
            //挑战方的冷却时间
            if (cd_sec > 0)
            {
                robj.push_back( Pair("coolTime", cd_sec) );
                return HC_ERROR_IN_COOLTIME_RACE;
            }
            //排名差距是否能挑战
            if (!Singleton<arenaMgr>::Instance().canChallege(rd->m_rank, td->m_rank))
            {
                return HC_ERROR;
            }
            int total_times = cdata->queryExtraData(char_data_type_daily, char_data_daily_arena);
            int buyTime = cdata->queryExtraData(char_data_type_daily, char_data_daily_buy_arena);
            if (total_times < (iArenaFreeTimes + buyTime))
            {
            }
            else
            {
                return HC_ERROR_NOT_ENOUGH_TIMES;
            }
            cdata->setExtraData(char_data_type_daily, char_data_daily_arena, total_times+1);
            ++rd->m_total_arena;
            rd->m_needSave = true;

            chessCombat* pb = Singleton<chessCombatMgr>::Instance().createArenaCombat(*cdata, *tc);
            if (NULL == pb)
            {
                ERR();
                return HC_ERROR;
            }
            //返回战斗信息
            pb->getCombatInfo(cdata->m_id, robj);
            rd->m_attack_who = toid;
            td->m_under_attack = cdata->m_id;
        }
    }
    else if(type == COMBAT_TYPE_WILD_CITY)
    {
        time_t time_now = time(NULL);
        struct tm tm;
        struct tm *t = &tm;
        localtime_r(&time_now, t);
        if (t->tm_hour < 7)
        {
            return HC_ERROR_IN_PROTECT;
        }
        int own_cnt = cdata->m_wild_citys.getOwnCnt();
        if (own_cnt >= iWildOwnMax[cdata->m_vip])
        {
            return HC_ERROR_NEED_MORE_VIP;
        }
        wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(toid);
        if (pwc == NULL || pwc->m_fight)
        {
            ERR();
            return HC_ERROR_CITY_IN_COMBAT;
        }
        if (cdata->m_id == pwc->m_owner_cid)
        {
            return HC_ERROR;
        }
        if (cdata->subSilver(iWildCityAttackCost, silver_cost_wild_attack) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_SILVER;
        }
        if (pwc->m_owner_cid == 0)//无人直接占领
        {
            //直接占领
            wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(toid);
            if (pwc == NULL)
            {
                ERR();
                return HC_ERROR;
            }
            pwc->reset();
            pwc->m_owner_cid = cdata->m_id;
            pwc->broadCastInfo();
            pwc->save();
            in_combat = 0;
        }
        else
        {
            CharData* toCdata = GeneralDataMgr::getInstance()->GetCharData(pwc->m_owner_cid).get();
            if (toCdata == NULL)
            {
                ERR();
                return HC_ERROR;
            }
            if (pwc->m_defense_hid == 0)//无城守直接占领
            {
                //直接占领
                wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(toid);
                if (pwc == NULL)
                {
                    ERR();
                    return HC_ERROR;
                }
                pwc->reset();
                pwc->m_owner_cid = cdata->m_id;
                pwc->broadCastInfo();
                pwc->save();
                in_combat = 0;
            }
            else
            {
                chessCombat* pb = Singleton<chessCombatMgr>::Instance().createWildCityCombat(*cdata, *toCdata, toid, pwc->m_defense_hid);
                if (NULL == pb)
                {
                    ERR();
                    return HC_ERROR;
                }
                //返回战斗信息
                pb->getCombatInfo(cdata->m_id, robj);
                pwc->m_fight = 1;
                pwc->broadCastInfo();
            }
        }
        cdata->m_score_tasks.updateTask(DAILY_SCORE_WILD);
    }
    else if(type == COMBAT_TYPE_SHENLING)
    {
        if (toid < 1)
        {
            return HC_ERROR;
        }
        boost::shared_ptr<baseShenling> bc = Singleton<shenlingMgr>::Instance().getShenlingById(toid);
        if (!bc.get())
        {
            return HC_ERROR;
        }
        int has_attack = cdata->queryExtraData(char_data_type_daily,char_data_daily_shenling);
        if (has_attack >= iShenlingFreeTime)
        {
            if (cdata->m_bag.getGemCount(GEM_ID_SHENLING_KEY) <= 0)
            {
                return HC_ERROR_NOT_ENOUGH_TIMES;
            }
        }
        boost::shared_ptr<CharShenling> cd = Singleton<shenlingMgr>::Instance().getCharShenling(cdata->m_id);
        if (cd.get())
        {
            if (cd->m_sid != toid)
            {
                return HC_ERROR;
            }
        }
        else
        {
            return HC_ERROR;
        }
        chessCombat* pb = Singleton<chessCombatMgr>::Instance().createShenlingCombat(*cdata, toid);
        if (NULL == pb)
        {
            ERR();
            return HC_ERROR;
        }
        //返回战斗信息
        pb->getCombatInfo(cdata->m_id, robj);
    }
    else if(type == COMBAT_TYPE_TREASURE_ROB)
    {
        if (cdata->m_id == toid)
        {
            return HC_ERROR;
        }
        boost::shared_ptr<char_treasure> pcg_atk = Singleton<treasureMgr>::Instance().getCharTreasure(cdata->m_id);
        boost::shared_ptr<char_treasure> pcg_def = Singleton<treasureMgr>::Instance().getCharTreasure(toid);
        if (!pcg_atk.get() || !pcg_def.get())
            return HC_ERROR;
        if (pcg_def->m_state == 1 && pcg_def->m_treasure.get())
        {
            if (pcg_atk->getCanRobTimes() < 1)
                return HC_ERROR_NOT_ENOUGH_TIMES;
            if (pcg_def->m_treasure->m_rob_time < 1)
                return HC_ERROR_TREASURE_NOT_ENOUGH_TIMES;
            CharData* toCdata = GeneralDataMgr::getInstance()->GetCharData(toid).get();
            if (toCdata == NULL)
            {
                ERR();
                return HC_ERROR;
            }
            chessCombat* pb = Singleton<chessCombatMgr>::Instance().createTreasureCombat(*cdata, *toCdata);
            if (NULL == pb)
            {
                ERR();
                return HC_ERROR;
            }
            //返回战斗信息
            pb->getCombatInfo(cdata->m_id, robj);
            //战斗正常开始就扣除可打劫次数
            --(pcg_def->m_treasure->m_rob_time);
            pcg_def->save();
        }
        else
        {
            return HC_ERROR;
        }
    }
    else if(type == COMBAT_TYPE_GUILD_MOSHEN)
    {
        if (toid < 1)
        {
            return HC_ERROR;
        }
        boost::shared_ptr<baseGuildMoshen> bc = Singleton<guildMgr>::Instance().getGuildMoshen(toid);
        if (!bc.get() || !bc->m_hero.get())
        {
            ERR();
            return HC_ERROR;
        }
        Guild* cp = Singleton<guildMgr>::Instance().getGuild(cdata->GetGuildId());
        if (!cp)
        {
            return HC_ERROR_NOT_JOIN_GUILD;
        }
        int char_moshen = cdata->queryExtraData(char_data_type_daily, char_data_daily_guild_moshen) + 1;
        if (char_moshen != toid)
        {
            return HC_ERROR;
        }
        int has_attack = cdata->queryExtraData(char_data_type_daily,char_data_daily_guild_moshen_start+toid);
        if (has_attack > 0)
        {
            return HC_ERROR;
        }
        chessCombat* pb = Singleton<chessCombatMgr>::Instance().createGuildMoshenCombat(*cdata, toid);
        if (NULL == pb)
        {
            ERR();
            return HC_ERROR;
        }
        //返回战斗信息
        pb->getCombatInfo(cdata->m_id, robj);
    }
    else if(type == COMBAT_TYPE_BOSS)
    {
        if (toid < 1)
        {
            return HC_ERROR;
        }
        boost::shared_ptr<Boss> spb = bossMgr::getInstance()->getBoss(toid);
        Boss* pboss = spb.get();
        if (NULL == pboss)
        {
            return HC_ERROR;
        }
        int cool = 0;
        int ret = pboss->canAttack(cid, cool);
        if (HC_SUCCESS != ret)
        {
            if (cool > 0)
            {
                robj.push_back( Pair("cool", cool) );
            }
            return ret;
        }
        chessCombat* pb = Singleton<chessCombatMgr>::Instance().createBossCombat(*cdata, toid);
        if (NULL == pb)
        {
            ERR();
            return HC_ERROR;
        }
        //返回战斗信息
        pb->getCombatInfo(cdata->m_id, robj);
    }
    else
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("in_combat", in_combat));
    return HC_SUCCESS;
}

//发起挑战
int ProcessChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    chessCombat* myCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(cdata->m_id);
    if (myCombat)
    {
        ERR();
        cout << "cid=" << cdata->m_id << " is in combat!!! combat_type=" << myCombat->m_type << endl;
        return HC_ERROR;
    }
    //插入战斗处理
    chessCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_create;
    cmd._pCombat = NULL;
    InsertCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//牌局行动
int ProcessGameAct(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    chessCombat* myCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    chessCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_player_act;
    cmd._pCombat = myCombat;
    InsertCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//牌局过牌
int ProcessGamePass(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    chessCombat* myCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    chessCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_player_pass;
    cmd._pCombat = myCombat;
    InsertCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//牌局过牌
int ProcessGameAuto(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    chessCombat* myCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    chessCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_player_auto;
    cmd._pCombat = myCombat;
    InsertCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//牌局魔法
int ProcessGameCastMagic(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    chessCombat* myCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    chessCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_player_cast;
    cmd._pCombat = myCombat;
    InsertCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//牌局继续
int ProcessGameNext(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    chessCombat* myCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    chessCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_next;
    cmd._pCombat = myCombat;
    InsertCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//退出挑战
int ProcessQuitChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    chessCombat* myCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    chessCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_player_quit;
    cmd._pCombat = myCombat;
    InsertCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//客户端准备好战斗信号
int ProcessCombatSign(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    chessCombat* myCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    chessCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_sign;
    cmd._pCombat = myCombat;
    InsertCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//牌局开始
int ProcessStartGame(json_spirit::mObject& o)
{
    int combat_id = 0;
    READ_INT_FROM_MOBJ(combat_id,o,"id");
    cout << "ProcessStartGame " << combat_id << endl;
    chessCombat* pb = Singleton<chessCombatMgr>::Instance().findCombat(combat_id);
    if (pb && pb->m_state != COMBAT_STATE_END)
    {
        cout << "state " << pb->m_state << endl;
        chessCombatCmd cmd;
        cmd.mobj = o;
        cmd.cmd = combat_cmd_start;
        cmd._pCombat = pb;
        InsertCombat(cmd);
        return HC_SUCCESS_NO_RET;
    }
    return HC_SUCCESS;
}

//发牌
int ProcessGameDeal(json_spirit::mObject& o)
{
    int combat_id = 0;
    READ_INT_FROM_MOBJ(combat_id,o,"id");
    chessCombat* pb = Singleton<chessCombatMgr>::Instance().findCombat(combat_id);
    if (pb)
    {
        chessCombatCmd cmd;
        cmd.mobj = o;
        cmd.cmd = combat_cmd_deal;
        cmd._pCombat = pb;
        InsertCombat(cmd);
        return HC_SUCCESS_NO_RET;
    }
    return HC_SUCCESS;
}

//牌局结束
int ProcessGameEnd(json_spirit::mObject& o)
{
    int combat_id = 0;
    READ_INT_FROM_MOBJ(combat_id,o,"id");
    chessCombat* pb = Singleton<chessCombatMgr>::Instance().findCombat(combat_id);
    if (pb)
    {
        chessCombatCmd cmd;
        cmd.mobj = o;
        cmd.cmd = combat_cmd_end;
        cmd._pCombat = pb;
        InsertCombat(cmd);
        return HC_SUCCESS_NO_RET;
    }
    return HC_SUCCESS;
}

//操作超时
int ProcessGameTimeout(json_spirit::mObject& o)
{
    int combat_id = 0;
    READ_INT_FROM_MOBJ(combat_id,o,"id");
    chessCombat* pb = Singleton<chessCombatMgr>::Instance().findCombat(combat_id);
    if (pb)
    {
        chessCombatCmd cmd;
        cmd.mobj = o;
        cmd.cmd = combat_cmd_timeout;
        cmd._pCombat = pb;
        InsertCombat(cmd);
        return HC_SUCCESS_NO_RET;
    }
    return HC_SUCCESS;
}

//牌局npc动作
int ProcessNPCAct(json_spirit::mObject& o)
{
    int combat_id = 0, cid = 0;
    READ_INT_FROM_MOBJ(combat_id,o,"id");
    READ_INT_FROM_MOBJ(cid,o,"cid");
    chessCombat* pb = Singleton<chessCombatMgr>::Instance().findCombat(combat_id);
    if (pb)
    {
        chessCombatCmd cmd;
        cmd.mobj = o;
        cmd.cname = "";
        cmd.cid = cid;
        cmd.cmd = combat_cmd_player_act;
        cmd._pCombat = pb;
        InsertCombat(cmd);
        return HC_SUCCESS_NO_RET;
    }
    return HC_SUCCESS;
}

int chessPlayer::LoadCharactor(CharData& cdata, int hid)
{
    if (hid == 0)
        hid = cdata.m_heros.m_default_hero;
    boost::shared_ptr<CharHeroData> p_hero = cdata.m_heros.GetHero(hid);
    if (!p_hero.get() || !p_hero->m_baseHero.get())
    {
        ERR();
        return -1;
    }
    if (!p_hero->m_init_attr)
    {
        p_hero->updateAttribute();
    }
    m_auto = false;
    m_ignore = false;
    m_cid = cdata.m_id;
    m_hid = p_hero->m_id;
    m_player_name = cdata.m_name;
    m_player_level = cdata.m_level;
    m_chat = cdata.m_chat;
    m_hp_org = p_hero->m_hp;
    m_hp_left = p_hero->m_hp;
    m_hp_max = p_hero->m_hp;
    m_name = p_hero->m_baseHero->m_name;
    m_level = p_hero->m_level;
    m_star = p_hero->m_star;
    m_race = p_hero->m_race;
    m_attack = p_hero->m_attack;
    m_defense = p_hero->m_defense;
    m_magic = p_hero->m_magic;
    m_spic = p_hero->m_spic;
    m_combat_attribute = cdata.m_combat_attribute;

    //技能加载
    m_magics.clear();
    cdata.m_magics.getCombatMagics(m_magics);
    m_equipt.clear();
    json_spirit::mObject o;
    int cur_nums = 0;
    p_hero->m_bag.showBagEquipments(m_equipt, o, cur_nums);
    return 0;
}

int chessPlayer::LoadStronghold(CharData& cdata, int strongholdid, int extra)
{
    boost::shared_ptr<baseStronghold> bs = Singleton<mapMgr>::Instance().GetBaseStrongholdData(strongholdid);
    if (!bs.get())
    {
        ERR();
        return -1;
    }
    CharStrongholdData* pcStronghold = Singleton<mapMgr>::Instance().GetCharStrongholdData(cdata.m_id, bs->m_mapid, bs->m_stageid, bs->m_pos).get();
    if (!pcStronghold || !pcStronghold->m_baseStronghold.get() && !pcStronghold->m_baseStronghold->m_hero.get())
    {
        ERR();
        return -1;
    }
    boost::shared_ptr<baseStrongholdHeroData> p_hero = pcStronghold->m_baseStronghold->m_hero;
    m_cid = -1;
    m_player_name = "";
    m_player_level = p_hero->m_level;
    m_chat = bs->m_chat;
    m_hid = strongholdid;
    m_hp_org = p_hero->m_hp * extra;
    m_hp_left = p_hero->m_hp * extra;
    m_hp_max = p_hero->m_hp * extra;
    m_name = p_hero->m_name;
    m_level = p_hero->m_level;
    m_star = p_hero->m_star;
    m_race = p_hero->m_race;
    m_attack = p_hero->m_attack * extra;
    m_defense = p_hero->m_defense * extra;
    m_magic = p_hero->m_magic * extra;
    m_spic = p_hero->m_spic;
    m_combat_attribute = bs->m_combat_attribute;
    return 0;
}

int chessPlayer::LoadCopy(CharData& cdata, int copyid)
{
    boost::shared_ptr<baseCopy> bc = Singleton<copyMgr>::Instance().getCopyById(copyid);
    if (!bc.get() || !bc->m_hero.get())
    {
        ERR();
        return -1;
    }
    boost::shared_ptr<baseStrongholdHeroData> p_hero = bc->m_hero;
    m_cid = -1;
    m_player_name = "";
    m_player_level = p_hero->m_level;
    m_chat = bc->m_chat;
    m_hid = copyid;
    m_hp_org = p_hero->m_hp;
    m_hp_left = p_hero->m_hp;
    m_hp_max = p_hero->m_hp;
    m_name = p_hero->m_name;
    m_level = p_hero->m_level;
    m_star = p_hero->m_star;
    m_race = p_hero->m_race;
    m_attack = p_hero->m_attack;
    m_defense = p_hero->m_defense;
    m_magic = p_hero->m_magic;
    m_spic = p_hero->m_spic;
    return 0;
}

int chessPlayer::LoadShenling(CharData& cdata, int sid)
{
    boost::shared_ptr<baseShenling> bc = Singleton<shenlingMgr>::Instance().getShenlingById(sid);
    if (!bc.get() || !bc->m_hero.get())
    {
        ERR();
        return -1;
    }
    boost::shared_ptr<CharShenling> cs = Singleton<shenlingMgr>::Instance().getCharShenling(cdata.m_id);
    if (!cs.get())
    {
        ERR();
        return -1;
    }
    boost::shared_ptr<baseStrongholdHeroData> p_hero = bc->m_hero;
    m_cid = -1;
    m_player_name = "";
    m_player_level = p_hero->m_level;
    m_chat = bc->m_chat;
    m_hid = sid;
    m_hp_org = p_hero->m_hp;
    m_hp_left = p_hero->m_hp;
    m_hp_max = p_hero->m_hp;
    m_name = p_hero->m_name;
    m_level = p_hero->m_level;
    m_star = p_hero->m_star;
    m_race = p_hero->m_race;
    m_attack = p_hero->m_attack;
    m_defense = p_hero->m_defense;
    m_magic = p_hero->m_magic;
    m_spic = p_hero->m_spic;
    //技能加载
    m_magics.clear();
    std::vector<int> magic;
    for (int i = 0; i < 3; ++i)
    {
        magic.push_back(cs->m_magics[i]);
        boost::shared_ptr<baseMagic> p_bs = Singleton<MagicMgr>::Instance().getBaseMagic(cs->m_magics[i]);
        json_spirit::Object obj;
        if (!p_bs.get())
        {
            continue;
        }
        p_bs->toObj(obj);
        m_magics.push_back(obj);
    }
    return 0;
}

int chessPlayer::LoadGuildMoshen(int moshenid)
{
    boost::shared_ptr<baseGuildMoshen> bc = Singleton<guildMgr>::Instance().getGuildMoshen(moshenid);
    if (!bc.get() || !bc->m_hero.get())
    {
        ERR();
        return -1;
    }
    boost::shared_ptr<baseStrongholdHeroData> p_hero = bc->m_hero;
    m_cid = -1;
    m_player_name = "";
    m_player_level = p_hero->m_level;
    m_chat = bc->m_chat;
    m_hid = moshenid;
    m_hp_org = p_hero->m_hp;
    m_hp_left = p_hero->m_hp;
    m_hp_max = p_hero->m_hp;
    m_name = p_hero->m_name;
    m_level = p_hero->m_level;
    m_star = p_hero->m_star;
    m_race = p_hero->m_race;
    m_attack = p_hero->m_attack;
    m_defense = p_hero->m_defense;
    m_magic = p_hero->m_magic;
    m_spic = p_hero->m_spic;
    return 0;
}

int chessPlayer::LoadBoss(Boss* pb)
{
    if (pb == NULL)
    {
        ERR();
        return -1;
    }
    m_cid = -1;
    m_player_name = "";
    m_player_level = pb->_boss._level;
    m_chat = pb->_boss._chat;
    m_hid = pb->_boss._id;
    m_hp_org = pb->_boss._cur_hp;
    m_hp_left = pb->_boss._cur_hp;
    m_hp_max = pb->_boss._max_hp;
    m_name = pb->_boss._name;
    m_level = pb->_boss._level;
    m_star = pb->_boss._star;
    m_race = pb->_boss._race;
    m_attack = pb->_boss._attack;
    m_defense = pb->_boss._defense;
    m_magic = pb->_boss._magic;
    m_spic = pb->_boss._spic;
    return 0;
}

void chessPlayer::getInfo(json_spirit::Object& robj)
{
    robj.push_back( Pair("id", m_cid) );
    robj.push_back( Pair("name", IS_NPC(m_cid) ? m_name : m_player_name) );
    robj.push_back( Pair("chat", m_chat) );
    if (IS_NOT_NPC(m_cid))
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
        if (cdata.get())
        {
            robj.push_back( Pair("nick", cdata->m_nick.get_string()) );
        }
    }
    robj.push_back( Pair("hp", m_hp_left) );
    robj.push_back( Pair("max_hp", m_hp_max) );
    robj.push_back( Pair("race", m_race) );
    robj.push_back( Pair("level", m_level) );
    robj.push_back( Pair("star", m_star) );
    robj.push_back( Pair("card", 100*m_race+getLevelRank(m_level)) );
    robj.push_back( Pair("attack", m_attack) );
    robj.push_back( Pair("defense", m_defense) );
    robj.push_back( Pair("magic", m_magic) );
    robj.push_back( Pair("spic", m_spic) );
    robj.push_back( Pair("skill", m_magics) );
    robj.push_back( Pair("equipt", m_equipt) );
}

void chessPlayer::getExtraInfo(json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        ERR();
        return;
    }
    //声望信息
    json_spirit::Array prestigelist;
    for (int type = 1; type <= 4; ++type)
    {
        json_spirit::Object prestigeobj;
        prestigeobj.push_back( Pair("type", type) );
        prestigeobj.push_back( Pair("prestige_level", cdata->m_prestige_level[type-1]) );
        prestigeobj.push_back( Pair("prestige", cdata->prestige(type)) );
        prestigeobj.push_back( Pair("max_prestige", cdata->prestigeMax(type)) );
        prestigelist.push_back(prestigeobj);
    }
    robj.push_back( Pair("prestige", prestigelist) );
    boost::shared_ptr<CharHeroData> p_hero = cdata->m_heros.GetHero(cdata->m_heros.m_default_hero);
    if (p_hero.get() && p_hero->m_baseHero.get())
    {
        json_spirit::Object org_exp;
        org_exp.push_back( Pair("exp", p_hero->m_exp) );
        org_exp.push_back( Pair("level", p_hero->m_level) );
        if (p_hero->m_level < iMaxLevel)
            org_exp.push_back( Pair("need_exp", GeneralDataMgr::getInstance()->GetBaseExp(p_hero->m_level + 1)));
        robj.push_back( Pair("exp", org_exp) );
    }
}

void chessPlayer::set_my_first()
{
    baseCard c;
    c.suit = m_race;
    c.value = getLevelRank(m_level);
    deal(1,c);
}

chessGame::chessGame(chessPlayer* p1, chessPlayer* p2)
{
    player1 = p1;
    player2 = p2;
    //重置手牌
    for (int i = 0; i < 4; ++i)
    {
        player1->m_cards[i].pos = i + 1;
        player1->m_cards[i].suit = 0;
        player1->m_cards[i].trans_suit = 0;
        player1->m_cards[i].value = 0;
        player1->m_cards[i].trans_suit = 0;
    }
    for (int i = 0; i < 4; ++i)
    {
        player2->m_cards[i].pos = i + 1;
        player2->m_cards[i].suit = 0;
        player2->m_cards[i].trans_suit = 0;
        player2->m_cards[i].value = 0;
        player2->m_cards[i].trans_suit = 0;
    }
    //设置英雄牌
    player1->set_my_first();
    player2->set_my_first();
    //双方同花色同大小
    if (player1->m_cards[0].suit == player2->m_cards[0].suit
        && player1->m_cards[0].value == player2->m_cards[0].value)
    {
        if (player1->m_cards[0].suit < 4)
        {
            ++player1->m_cards[0].suit;
        }
        else
        {
            --player2->m_cards[0].suit;
        }
    }
    reset();
    refresh();
    //双方都发一张牌先
    baseCard* card1 = deal();
    baseCard* card2 = deal();
    player1->deal(2, *card1);
    player2->deal(2, *card2);
}

void chessGame::refresh()
{
    m_poker.reShuffle();
    m_poker.remove(0,Red_Joker);
    for (int i = 0; i < 4; ++i)
    {
        m_poker.remove(player1->m_cards[i].suit, player1->m_cards[i].value);
        m_poker.remove(player2->m_cards[i].suit, player2->m_cards[i].value);
    }
    for(int i = 0; i < 5; ++i)
    {
        for(int j = 0; j < 5; ++j)
        {
            //发九张牌到牌桌[1,1][[3,3]
            if ((i >= 1 && i < 4) && (j >= 1 && j < 4))
            {
                baseCard* card = deal();
                m_chess[i][j].set(card->suit,card->value);
            }
            else
            {
                m_chess[i][j].set(0,0);
            }
        }
    }
    m_count = 9;
}

void chessGame::refreshChess9()
{
    //保存原牌
    std::vector<baseCard> tmp_list;
    for(int i = 0; i < 5; ++i)
    {
        for(int j = 0; j < 5; ++j)
        {
            //发九张牌到牌桌[1,1][[3,3]
            if ((i >= 1 && i < 4) && (j >= 1 && j < 4))
            {
                baseCard card;
                card.set(m_chess[i][j].suit,m_chess[i][j].value);
                tmp_list.push_back(card);
            }
        }
    }
    //打乱
    std::random_shuffle(tmp_list.begin(), tmp_list.end(), myrandom);
    //插回
    int cur = 0;
    for(int i = 0; i < 5; ++i)
    {
        for(int j = 0; j < 5; ++j)
        {
            //发九张牌到牌桌[1,1][[3,3]
            if ((i >= 1 && i < 4) && (j >= 1 && j < 4) && cur < tmp_list.size())
            {
                m_chess[i][j].set(tmp_list[cur].suit,tmp_list[cur].value);
                ++cur;
            }
        }
    }
    broadCastChess(2, 2);
}

int chessGame::start()
{
    if (COMBAT_STATE_INIT == m_state)
    {
        m_next = player1->m_cid;
        m_state = COMBAT_STATE_INGOING;
        m_seq = 1;
        m_round = 0;
        doRound r;
        r.reset();
        m_rounds.push_back(r);
        lastDo.clear();
        json_spirit::Object o;
        o.push_back( Pair("cmd", "new_combat_start") );
        o.push_back( Pair("s", 200) );
        json_spirit::Array list1, list2;
        for(int i = 0; i < 4; ++i)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("card", 100*player1->m_cards[i].suit+player1->m_cards[i].value) );
            list1.push_back(obj);

            obj.clear();
            obj.push_back( Pair("card", 100*player2->m_cards[i].suit+player2->m_cards[i].value) );
            list2.push_back(obj);
        }
        o.push_back( Pair("cid1", player1->m_cid) );
        o.push_back( Pair("list1", list1) );
        o.push_back( Pair("cid2", player2->m_cid) );
        o.push_back( Pair("list2", list2) );
        o.push_back( Pair("next", 0) );
        o.push_back( Pair("seq", 1) );
        broadCastMsg(write(o));

        //发牌延后定时器开始
        json_spirit::mObject mobj;
        mobj["cmd"] = "new_combat_deal";
        mobj["id"] = m_combat->m_combat_id;
        boost::shared_ptr<splsTimer> tmsg;
        int wait_time = 1;
        tmsg.reset(new splsTimer(wait_time, 1, mobj,1));
        splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return HC_SUCCESS;
}

chessPlayer* chessGame::getPlayer(int cid)
{
    if (player1->m_cid == cid)
    {
        return player1;
    }
    if (player2->m_cid == cid)
    {
        return player2;
    }
    return NULL;
}

void chessGame::broadCastMsg(const std::string& msg, int delay_sec)
{
    if (IS_NOT_NPC(player1->m_cid) && !player1->m_ignore)
    {
        if (delay_sec == 0)
        {
            boost::shared_ptr<OnlineCharactor> c1 = GeneralDataMgr::getInstance()->GetOnlineCharactor(player1->m_player_name);
            if (c1.get())
            {
                c1->Send(msg);
            }
        }
        else if(delay_sec > 0)
        {
            json_spirit::mObject mobj;
            mobj["cmd"] = "sendMsg";
            mobj["msg"] = msg;
            mobj["name"] = player1->m_player_name;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(delay_sec, 1, mobj,1));
            splsTimerMgr::getInstance()->addTimer(tmsg);
        }
    }
    if (IS_NOT_NPC(player2->m_cid) && !player2->m_ignore)
    {
        if (delay_sec == 0)
        {
            boost::shared_ptr<OnlineCharactor> c2 = GeneralDataMgr::getInstance()->GetOnlineCharactor(player2->m_player_name);
            if (c2.get())
            {
                c2->Send(msg);
            }
        }
        else if(delay_sec > 0)
        {
            json_spirit::mObject mobj;
            mobj["cmd"] = "sendMsg";
            mobj["msg"] = msg;
            mobj["name"] = player2->m_player_name;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(delay_sec, 1, mobj,1));
            splsTimerMgr::getInstance()->addTimer(tmsg);
        }
    }
}

void chessGame::broadCastAct()
{
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "new_combat_act") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("round", m_round+1) );
    obj.push_back( Pair("id", lastDo.cid) );
    obj.push_back( Pair("next", m_next) );
    obj.push_back( Pair("seq", m_seq) );
    json_spirit::Array list;
    json_spirit::Object act;
    act.push_back( Pair("pos", lastDo.pos[0]) );
    act.push_back( Pair("x", lastDo.x[0]) );
    act.push_back( Pair("y", lastDo.y[0]) );
    act.push_back( Pair("card", 100*lastDo.suit[0]+lastDo.value[0]) );
    list.push_back(act);
    act.clear();
    act.push_back( Pair("pos", lastDo.pos[1]) );
    act.push_back( Pair("x", lastDo.x[1]) );
    act.push_back( Pair("y", lastDo.y[1]) );
    act.push_back( Pair("card", 100*lastDo.suit[1]+lastDo.value[1]) );
    list.push_back(act);
    obj.push_back( Pair("list", list) );
    std::string msg = json_spirit::write(obj);
    broadCastMsg(msg);
}

void chessGame::broadCastCastMagic()
{
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "new_combat_cast") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("round", m_round+1) );
    obj.push_back( Pair("cid", lastDo.cid) );
    obj.push_back( Pair("id", lastDo.magic_id) );
    boost::shared_ptr<baseMagic> bsd = Singleton<MagicMgr>::Instance().getBaseMagic(lastDo.magic_id);
    if (bsd.get())
    {
        json_spirit::Object o;
        bsd->toObj(o);
        obj.push_back( Pair("info", o) );
    }
    std::string msg = json_spirit::write(obj);
    broadCastMsg(msg);
}

void chessGame::broadCastCard(int cid, int next, int seq, int type, int delay_sec)
{
    //同步手牌信息
    chessPlayer* p = cid == player1->m_cid ? player1 : player2;
    if (p)
    {
        json_spirit::Array list;
        for(int i = 0; i < 4; ++i)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("card", 100*p->m_cards[i].suit+p->m_cards[i].value) );
            cout << 100*p->m_cards[i].suit+p->m_cards[i].value << " ";
            list.push_back(obj);
        }
        cout << endl;
        json_spirit::Object o;
        o.push_back( Pair("cmd", "new_combat_hand") );
        o.push_back( Pair("s", 200) );
        o.push_back( Pair("cid", cid) );
        o.push_back( Pair("list", list) );
        o.push_back( Pair("round", m_round+1) );
        o.push_back( Pair("type", type) );
        o.push_back( Pair("next", next) );
        o.push_back( Pair("seq", seq) );
        std::string msg = json_spirit::write(o);
        broadCastMsg(msg, delay_sec);
    }
}

void chessGame::broadCastChess(int type, int delay_sec)
{
    cout << "broadCastChess" << endl;
    //牌桌信息通知双方
    json_spirit::Array list;
    for(int i = 0; i < 5; ++i)
    {
        for(int j = 0; j < 5; ++j)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("x", i+1) );
            obj.push_back( Pair("y", j+1) );
            obj.push_back( Pair("card", 100*m_chess[i][j].suit+m_chess[i][j].value) );
            list.push_back(obj);
        }
    }
    json_spirit::Object o;
    o.push_back( Pair("cmd", "new_combat_chess") );
    o.push_back( Pair("s", 200) );
    o.push_back( Pair("type", type) );
    o.push_back( Pair("list", list) );
    std::string msg = json_spirit::write(o);
    broadCastMsg(msg, delay_sec);
}

int chessGame::cardDamage(int card_rank)
{
    if (card_rank > 11 || card_rank < 1)
        return 0;
    chessPlayer* attack_player = lastDo.cid == player1->m_cid ? player1 : player2;
    chessPlayer* defense_player = lastDo.cid == player1->m_cid ? player2 : player1;
    int damage = 0;
    //（自身攻击*5+自身魔力*5）-（敌方防御/2+敌方魔力/2）
    damage = (attack_player->m_attack + attack_player->m_magic) * 5 - (defense_player->m_defense + defense_player->m_magic) / 2;
    //新公式在老伤害基础上/3*牌型加成
    damage = damage * (100 + iDamagePer[card_rank-1]) / 300 ;

    //技能影响
    int add_per = 0;
    int resist_per = 0;

    add_per += attack_player->m_combat_attribute.skill_rank_add(card_rank);
    resist_per += defense_player->m_combat_attribute.skill_rank_resist(card_rank);

    add_per += attack_player->m_combat_attribute.damage_per();
    resist_per += defense_player->m_combat_attribute.damage_resist_per();

    damage = damage * (100 + add_per - resist_per) / 100;
    if (damage < 1)
        damage = 1;
    return damage;
}

void chessGame::checkFiveCard(doAction action)
{
    cout << "chessGame::checkFiveCard" << endl;
    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            cout << 100*m_chess[i][j].suit+m_chess[i][j].value << " ";
            if (j == 4)
                cout << endl;
        }
    }
    cout << endl;
    int x = 0, y = 0;
    bool check_type = false;
    chessPlayer* attack_player = lastDo.cid == player1->m_cid ? player1 : player2;
    if (attack_player == NULL)
        return;
    for (int cal_cnt = 0; cal_cnt < 2; ++cal_cnt)
    {
        x = action.x[cal_cnt];
        y = action.y[cal_cnt];
        //检查行，列，正斜，反斜
        for (int type = 1; type <= 4; ++type)
        {
            if (type == action.type)
            {
                if (check_type)
                {
                    continue;
                }
                else
                {
                    check_type = true;
                }
            }
            fiveCards final;
            std::vector<int> x_list, y_list;
            int i = 1, j = 1, cnt = 0;
            switch(type)
            {
                case 1:
                    {
                        i = 1, j = y;
                        while(i <= 5 && j <= 5 && m_chess[i-1][j-1].valid())
                        {
                            final.cards[cnt].set(m_chess[i-1][j-1].suit, m_chess[i-1][j-1].value);
                            x_list.push_back(i);
                            y_list.push_back(j);
                            ++cnt;
                            ++i;
                        }
                    }
                    break;
                case 2:
                    {
                        i = x, j = 1;
                        while(i <= 5 && j <= 5 && m_chess[i-1][j-1].valid())
                        {
                            final.cards[cnt].set(m_chess[i-1][j-1].suit, m_chess[i-1][j-1].value);
                            x_list.push_back(i);
                            y_list.push_back(j);
                            ++cnt;
                            ++j;
                        }
                    }
                    break;
                case 3:
                    {
                        if ((x == 1 && y == 1) || (x == 5 && y == 5))
                        {
                            i = 1, j = 1;
                            while(i <= 5 && j <= 5 && m_chess[i-1][j-1].valid())
                            {
                                final.cards[cnt].set(m_chess[i-1][j-1].suit, m_chess[i-1][j-1].value);
                                x_list.push_back(i);
                                y_list.push_back(j);
                                ++cnt;
                                ++i;
                                ++j;
                            }
                        }
                    }
                    break;
                case 4:
                    {
                        if ((x == 1 && y == 5) || (x == 5 && y == 1))
                        {
                            i = 1, j = 5;
                            while(i <= 5 && j <= 5 && m_chess[i-1][j-1].valid())
                            {
                                final.cards[cnt].set(m_chess[i-1][j-1].suit, m_chess[i-1][j-1].value);
                                x_list.push_back(i);
                                y_list.push_back(j);
                                ++cnt;
                                ++i;
                                --j;
                            }
                        }
                    }
                    break;
            }
            //组成牌组
            if (cnt == 5)
            {
                final.evaluator();
                if (attack_player->m_magic_spade)
                {
                    if (final.rank == STRAIGHT)
                    {
                        //同花顺
                        switch (final.score)
                        {
                            case Ace + 1:
                                final.rank = ROYAL_FLUSH;
                                break;
                            case Ace:
                                final.rank = SHELL_FLUSH;
                                break;
                            default:
                                final.rank = STRAIGHT_FLUSH;
                                break;
                        }
                    }
                    if (final.rank < FLUSH)
                        final.rank = FLUSH;
                }
                int tmp_damage = cardDamage(final.rank);//本次伤害
                int tmp_heal = 0, race_value = 0;
                if (IS_NOT_NPC(lastDo.cid))
                {
                    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(lastDo.cid);
                    if (cdata.get())
                    {
                        race_value = cdata->m_race;
                    }
                }
                else
                {
                    race_value = attack_player->m_race;
                }
                if (attack_player->m_magic_damage)
                {
                    tmp_damage += (tmp_damage * iMagicDamage[race_value-1][final.rank-1] / 100);
                }
                if (attack_player->m_magic_health)
                {
                    tmp_heal += (tmp_damage * iMagicHeal[race_value-1][final.rank-1] / 100);
                }
                m_damage += tmp_damage;
                m_heal += tmp_heal;
                json_spirit::Object o;
                json_spirit::Array pos_list;
                for (int index = 0; index < 5; ++index)
                {
                    json_spirit::Object c;
                    c.push_back( Pair("x", x_list[index]) );
                    c.push_back( Pair("y", y_list[index]) );
                    pos_list.push_back(c);
                }
                o.push_back( Pair("pos_list", pos_list) );
                o.push_back( Pair("rank", final.rank) );
                o.push_back( Pair("damage", tmp_damage) );
                o.push_back( Pair("heal", tmp_heal) );
                round_list.push_back(o);
                cout << "x=" << x << ",y=" << y << ",type=" << type << ",rank=" << final.rank << endl;
            }
        }
    }
}

void chessGame::roundEnd()
{
    //攻击
    chessPlayer* attack_player = lastDo.cid == player1->m_cid ? player1 : player2;
    chessPlayer* defense_player = lastDo.cid == player1->m_cid ? player2 : player1;

    if (attack_player->m_magic_health)
    {
        attack_player->m_hp_left += m_heal;
        if (attack_player->m_hp_left > attack_player->m_hp_max)
            attack_player->m_hp_left = attack_player->m_hp_max;
    }

    defense_player->m_hp_left -= m_damage;
    if (defense_player->m_hp_left < 0)
    {
        defense_player->m_hp_left = 0;
        m_state = COMBAT_STATE_END;
    }

    attack_player->m_magic_spade = false;
    attack_player->m_magic_health = false;
    attack_player->m_magic_damage= false;

    //通知操作结果
    int wait_time = 0;
    if (IS_NPC(lastDo.cid)
        || (lastDo.cid == player1->m_cid && player1->m_auto)
        || (lastDo.cid == player2->m_cid && player2->m_auto))
    {
        wait_time = 2;//npc攻击延迟发送攻击消息(客户端播放npc放牌动画)
    }
    json_spirit::Object result;
    result.push_back( Pair("cmd", "new_combat_attack") );
    result.push_back( Pair("s", 200) );
    result.push_back( Pair("last_attack", m_state == COMBAT_STATE_END) );
    result.push_back( Pair("list", round_list) );
    if (m_round_magic > 0)
    {
        boost::shared_ptr<baseMagic> bsd = Singleton<MagicMgr>::Instance().getBaseMagic(m_round_magic);
        if (bsd.get())
        {
            json_spirit::Object o;
            bsd->toObj(o);
            result.push_back( Pair("info", o) );
        }
    }

    json_spirit::Object aObj;
    aObj.push_back( Pair("id", attack_player->m_cid) );
    aObj.push_back( Pair("hp", attack_player->m_hp_left) );
    result.push_back( Pair("attacker", aObj) );

    json_spirit::Object dObj;
    dObj.push_back( Pair("id", defense_player->m_cid) );
    dObj.push_back( Pair("hp", defense_player->m_hp_left) );
    result.push_back( Pair("defenser", dObj) );

    std::string msg = write(result);
    broadCastMsg(msg, wait_time);
    roundReset();
    //战斗是否结束?
    if (m_combat)
    {
        m_combat->checkEnd();
    }
}

int chessGame::act(int cid,int seq,int pos1,int x1,int y1,int pos2,int x2,int y2)
{
    if (cid != m_next || seq != m_seq)
    {
        cout << "cid=" << cid << ",next_cid=" << m_next << endl;
        cout << "seq=" << seq << ",next_seq=" << m_seq << endl;
        ERR();
        return HC_ERROR;
    }
    if (!checkPos(x1, y1, x2, y2, lastDo.type))
    {
        ERR();
        return HC_ERROR;
    }
    chessPlayer* p = getPlayer(cid);
    if (p)
    {
        //玩家手牌合法性
        if (p->m_cards[pos1-1].value <= 0 || p->m_cards[pos2-1].value <= 0)
        {
            ERR();
            return HC_ERROR;
        }
        //目标位置合法性
        if (m_chess[x1-1][y1-1].valid() || m_chess[x2-1][y2-1].valid())
        {
            ERR();
            for (int i = 0; i < 5; ++i)
            {
                for (int j = 0; j < 5; ++j)
                {
                    cout << 100*m_chess[i][j].suit+m_chess[i][j].value << " ";
                    if (j == 4)
                        cout << endl;
                }
            }
            return HC_ERROR;
        }
        lastDo.cid = cid;
        lastDo.pos[0] = pos1;
        lastDo.x[0] = x1;
        lastDo.y[0] = y1;
        lastDo.suit[0] = p->m_cards[pos1-1].suit;
        lastDo.value[0] = p->m_cards[pos1-1].value;
        lastDo.pos[1] = pos2;
        lastDo.x[1] = x2;
        lastDo.y[1] = y2;
        lastDo.suit[1] = p->m_cards[pos2-1].suit;
        lastDo.value[1] = p->m_cards[pos2-1].value;
        m_rounds[m_round].m_actList.push_back(lastDo);
        cout << "push act" << endl;
        //设置牌桌重置手牌
        m_chess[x1-1][y1-1].set(p->m_cards[pos1-1].suit,p->m_cards[pos1-1].value);
        m_chess[x2-1][y2-1].set(p->m_cards[pos2-1].suit,p->m_cards[pos2-1].value);
        m_count += 2;
        p->m_cards[pos1-1].suit = 0;
        p->m_cards[pos1-1].value = 0;
        p->m_cards[pos2-1].suit = 0;
        p->m_cards[pos2-1].value = 0;
        m_next = 0;
        ++m_seq;
        broadCastAct();
        checkFiveCard(lastDo);
        roundEnd();
        cout << "act finish" << endl;
        return HC_SUCCESS_NO_RET;
    }
    return HC_ERROR;
}

int chessGame::pass(int cid, json_spirit::Object& robj)
{
    chessPlayer* p = getPlayer(cid);
    if (p)
    {
        robj.push_back( Pair("cid", cid) );
        //玩家手牌抛弃
        for (int i = 0; i < 5; ++i)
        {
            p->m_cards[i].pos = i + 1;
            p->m_cards[i].suit = 0;
            p->m_cards[i].trans_suit = 0;
            p->m_cards[i].value = 0;
            p->m_cards[i].trans_suit = 0;
        }
        lastDo.clear();
        lastDo.cid = cid;
        m_rounds[m_round].m_actList.push_back(lastDo);

        m_next = 0;
        roundReset();

        ++m_seq;
        //broadCastAct();
        //broadCastCard(cid,m_next,m_seq, CARD_NOTIFY_PASS);
        //dealCard();
        //发牌延后定时器开始
        json_spirit::mObject mobj;
        mobj["cmd"] = "new_combat_deal";
        mobj["id"] = m_combat->m_combat_id;
        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(3, 1, mobj,1));
        splsTimerMgr::getInstance()->addTimer(tmsg);
        return HC_SUCCESS_NO_RET;
    }
    return HC_ERROR;
}

int chessGame::cast(int cid, int magic_id)
{
    if (m_round_magic > 0)
    {
        return HC_ERROR;
    }
    chessPlayer* p = player1->m_cid ==cid ? player1 : player2;
    chessPlayer* p_target = player1->m_cid == cid ? player2 : player1;
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<baseMagic> bsd = Singleton<MagicMgr>::Instance().getBaseMagic(magic_id);
    if (!bsd.get())
    {
        return HC_ERROR;
    }
    if (!cdata->m_magics.inCombat(magic_id))
    {
        return HC_ERROR;
    }
    if (p->m_used_magic[magic_id] > 0)
    {
        return HC_ERROR;
    }
    int ret = HC_ERROR;
    if (bsd->m_race == 0)
    {
        //公共技能
        switch(bsd->m_type)
        {
            case MAGIC_REFRESH:
                {
                    refreshChess9();
                    p->m_used_magic[magic_id] = 1;
                    ret = HC_SUCCESS_NO_RET;
                }
                break;
            case MAGIC_SPADE:
                {
                    p->m_magic_spade = true;
                    p->m_used_magic[magic_id] = 1;
                    ret = HC_SUCCESS_NO_RET;
                }
                break;
            case MAGIC_JOKE:
                {
                    uint8_t suit = 0;
                    uint8_t value = Red_Joker;
                    changeCard(suit, value, cdata->m_id);
                    p->m_used_magic[magic_id] = 1;
                    broadCastCard(p->m_cid, m_next, m_seq, CARD_NOTIFY_MAGIC, 2);
                    ret = HC_SUCCESS_NO_RET;
                }
                break;
            default:
                break;
        }
    }
    else
    {
        //种族技能
        switch(bsd->m_type)
        {
            case MAGIC_NOVA:
                {
                    p_target->m_wait_round = 2;
                    p->m_used_magic[magic_id] = 1;
                    ret = HC_SUCCESS_NO_RET;
                }
                break;
            case MAGIC_HEALTH:
                {
                    p->m_magic_health = true;
                    p->m_used_magic[magic_id] = 1;
                    ret = HC_SUCCESS_NO_RET;
                }
                break;
            case MAGIC_DAMAGE:
                {
                    p->m_magic_damage = true;
                    p->m_used_magic[magic_id] = 1;
                    ret = HC_SUCCESS_NO_RET;
                }
                break;
            default:
                break;
        }
    }
    if (ret == HC_SUCCESS_NO_RET)
    {
        m_round_magic = magic_id;
        lastDo.cid = cid;
        lastDo.cast = true;
        lastDo.magic_id = magic_id;
        m_rounds[m_round].m_actList.push_back(lastDo);
        broadCastCastMagic();
    }
    return ret;
}

int chessGame::autoAct(int cid, json_spirit::Object& robj)
{
    chessPlayer* p = getPlayer(cid);
    if (p)
    {
        p->m_auto = p->m_auto ? false : true;
        robj.push_back( Pair("auto", p->m_auto) );
        //立刻自动
        if ((m_next == cid && p->m_auto))
        {
            NPCHighActRandom(m_next);
        }
        return HC_SUCCESS_NO_RET;
    }
    return HC_ERROR;
}

int chessGame::dealCard()
{
    if (lastDo.cid == 0)
    {
        m_next = player1->m_cid;
    }
    else
    {
        m_next = (lastDo.cid == player1->m_cid ? player2->m_cid : player1->m_cid);
    }
    cout << "dealCard to " << m_next << endl;
    //给玩家补满手牌
    chessPlayer* p = getPlayer(m_next);
    if (p == NULL)
        return HC_ERROR;
    if (p->m_wait_round > 0)
    {
        //停一回合
        lastDo.clear();
        lastDo.cid = m_next;
        m_rounds[m_round].m_actList.push_back(lastDo);
        m_next = 0;
        roundReset();
        --p->m_wait_round;
        return dealCard();
    }
    if (p)
    {
        for(int i = 0; i < 4; ++i)
        {
            if (p->m_cards[i].value <= 0)
            {
                baseCard* card = deal();
                p->deal(i+1, *card);
            }
        }
    }
    ++m_seq;
    broadCastCard(p->m_cid, m_next, m_seq, CARD_NOTIFY_DEAL);
    //npc自动
    if (IS_NPC(m_next)
        || (m_next == player1->m_cid && player1->m_auto)
        || (m_next == player2->m_cid && player2->m_auto))
    {
        NPCHighActRandom(m_next);
    }
    return HC_SUCCESS_NO_RET;
}

baseCard* chessGame::deal()
{
    baseCard* c = m_poker.deal();
    if (c == NULL)
    {
        m_poker.reShuffle();
        m_poker.remove(0,Red_Joker);
        for (int i = 0; i < 4; ++i)
        {
            m_poker.remove(player1->m_cards[i].suit, player1->m_cards[i].value);
            m_poker.remove(player2->m_cards[i].suit, player2->m_cards[i].value);
        }
        for(int i = 0; i < 5; ++i)
        {
            for(int j = 0; j < 5; ++j)
            {
                m_poker.remove(m_chess[i][j].suit, m_chess[i][j].value);
            }
        }
        c = m_poker.deal();
        if (c == NULL)
        {
            ERR();
        }
    }
    return c;
}

void chessGame::changeCard(uint8_t s, uint8_t v, int cid)
{
    chessPlayer* p = cid == player1->m_cid ? player1 : player2;
    std::vector<int> index_list;
    for (int i = 0; i < 4; ++i)
    {
        if (p->m_cards[i].value > 0)
        {
            index_list.push_back(i);
        }
    }
    int index = index_list[my_random(0,index_list.size()-1)];
    p->m_cards[index].set(s, v);
}

bool chessGame::checkPos(int x1, int y1, int x2, int y2, int& type)
{
    for(int i = 0; i < 8; ++i)
    {
        if ((x1 == iChessBlank[i][0] && y1 == iChessBlank[i][1] && x2 == iChessBlank[i][2] && y2 == iChessBlank[i][3])
            || (x2 == iChessBlank[i][0] && y2 == iChessBlank[i][1] && x1 == iChessBlank[i][2] && y1 == iChessBlank[i][3]))
        {
            type = iChessBlank[i][4];
            return true;
        }
    }
    return false;
}

void chessGame::NPCActRandom(int cid)
{
    bool find = false;
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0, hand1 = 0, hand2 = 0;
    chessPlayer* p = getPlayer(cid);
    if (p)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (find)
            {
                break;
            }
            x1 = iChessBlank[i][0];
            y1 = iChessBlank[i][1];
            x2 = iChessBlank[i][2];
            y2 = iChessBlank[i][3];
            cout << x1 << " " << y1 << endl;
            cout << x2 << " " << y2 << endl;
            if (m_chess[x1-1][y1-1].valid() || m_chess[x2-1][y2-1].valid())
            {
                cout << "has card" << endl;
                continue;
            }
            //选定位置后挑选两张牌
            for(int pos = 1; pos <= 3; ++pos)
            {
                if (find)
                {
                    break;
                }
                if (p->m_cards[pos-1].value > 0 && p->m_cards[pos-1].suit >= 0)
                {
                    for(int pos2 = pos+1; pos2 <= 4; ++pos2)
                    {
                        if (find)
                        {
                            break;
                        }
                        if (p->m_cards[pos2-1].value > 0 && p->m_cards[pos2-1].suit >= 0)
                        {
                            hand1 = pos;
                            hand2 = pos2;
                            find = true;
                        }
                    }
                }
            }
        }
    }
    if (find)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "new_combat_npcAct";
        obj["id"] = m_combat->m_combat_id;
        obj["cid"] = m_next;
        obj["seq"] = m_seq;
        json_spirit::mArray list;
        {
            json_spirit::mObject tmp;
            tmp["pos"] = hand1;
            tmp["x"] = x1;
            tmp["y"] = y1;
            list.push_back(tmp);
        }
        {
            json_spirit::mObject tmp;
            tmp["pos"] = hand2;
            tmp["x"] = x2;
            tmp["y"] = y2;
            list.push_back(tmp);
        }
        obj["list"] = list;
        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(3, 1, obj,1));
        splsTimerMgr::getInstance()->addTimer(tmsg);
        cout << "npc act " << x1 << " " << y1 << ", " << x2 << " " << y2 << endl;
    }
}

void chessGame::NPCHighActRandom(int cid)
{
    bool find = false;
    int result_x1 = 0, result_x2 = 0, result_y1 = 0, result_y2 = 0, result_pos1 = 0, result_pos2 = 0, max_damage = 0;
    int cur_x1 = 0, cur_y1 = 0, cur_x2 = 0, cur_y2 = 0;
    chessPlayer* p = getPlayer(cid);
    if (p)
    {
        for (int i = 0; i < 8; ++i)
        {
            cur_x1 = iChessBlank[i][0];
            cur_y1 = iChessBlank[i][1];
            cur_x2 = iChessBlank[i][2];
            cur_y2 = iChessBlank[i][3];
            if (m_chess[cur_x1-1][cur_y1-1].valid() || m_chess[cur_x2-1][cur_y2-1].valid())
            {
                continue;
            }
            cout << cur_x1 << " " << cur_y1 << endl;
            cout << cur_x2 << " " << cur_y2 << endl;
            //选定位置后挑选两张牌
            for(int pos = 1; pos <= 4; ++pos)
            {
                if (p->m_cards[pos-1].value > 0 && p->m_cards[pos-1].suit >= 0)
                {
                    for(int pos2 = 1; pos2 <= 4; ++pos2)
                    {
                        if (pos2 == pos)
                            continue;
                        if (p->m_cards[pos2-1].value > 0 && p->m_cards[pos2-1].suit >= 0)
                        {
                            //当前坐标和当前两张备选牌组合模拟放牌计算结果
                            m_chess[cur_x1-1][cur_y1-1].set(p->m_cards[pos-1].suit,p->m_cards[pos-1].value);
                            m_chess[cur_x2-1][cur_y2-1].set(p->m_cards[pos2-1].suit,p->m_cards[pos2-1].value);
                            doAction action;
                            action.clear();
                            action.cid = cid;
                            action.type = iChessBlank[i][4];
                            action.pos[0] = pos;
                            action.x[0] = cur_x1;
                            action.y[0] = cur_y1;
                            action.pos[1] = pos2;
                            action.x[1] = cur_x2;
                            action.y[1] = cur_y2;
                            checkFiveCard(action);
                            if (m_damage > 0 && m_damage > max_damage)
                            {
                                max_damage = m_damage;
                                result_x1 = cur_x1;
                                result_x2 = cur_x2;
                                result_y1 = cur_y1;
                                result_y2 = cur_y2;
                                result_pos1 = pos;
                                result_pos2 = pos2;
                                find = true;
                                cout << "new big act " << result_x1 << " " << result_y1 << " put "<< pos <<", " << result_x2 << " " << result_y2 << " put "<< pos2 << endl;
                            }
                            m_damage = 0;
                            m_heal = 0;
                            round_list.clear();
                            m_chess[cur_x1-1][cur_y1-1].set(0,0);
                            m_chess[cur_x2-1][cur_y2-1].set(0,0);
                        }
                    }
                }
            }
        }
    }
    if (find)
    {
        json_spirit::mObject obj;
        obj["cmd"] = "new_combat_npcAct";
        obj["id"] = m_combat->m_combat_id;
        obj["cid"] = m_next;
        obj["seq"] = m_seq;
        json_spirit::mArray list;
        {
            json_spirit::mObject tmp;
            tmp["pos"] = result_pos1;
            tmp["x"] = result_x1;
            tmp["y"] = result_y1;
            list.push_back(tmp);
        }
        {
            json_spirit::mObject tmp;
            tmp["pos"] = result_pos2;
            tmp["x"] = result_x2;
            tmp["y"] = result_y2;
            list.push_back(tmp);
        }
        obj["list"] = list;
        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(3, 1, obj,1));
        splsTimerMgr::getInstance()->addTimer(tmsg);
    }
}

chessCombat::chessCombat()
{
    m_combat_id = GeneralDataMgr::getInstance()->newCombatId();
    m_type = COMBAT_TYPE_INIT;
    m_ignore_time = time(NULL) + iCombatIgnoreSec;
    m_client_check = 1;
    m_state = COMBAT_STATE_INIT;
    m_result = COMBAT_RESULT_INIT;
    m_result_type = 0;
    m_quit_cid = 0;

    m_data_id = 0;
    m_data_type = 0;
    m_extra_data[0] = 0;
    m_extra_data[1] = 0;

    m_archive_report = 0;
    m_cur_game = NULL;
}

int chessCombat::onStart()
{
    if (m_cur_game && m_cur_game->m_state == COMBAT_STATE_INIT)
    {
        m_cur_game->start();
    }
    return 0;
}

int chessCombat::quit(int cid)
{
    if (m_state == COMBAT_STATE_END)
    {
        return HC_SUCCESS;
    }
    if (m_cur_game && m_cur_game->m_state != COMBAT_STATE_END)
    {
        m_quit_cid = cid;
        m_cur_game->m_state = COMBAT_STATE_END;
        return checkEnd();
    }
    return HC_SUCCESS;
}

int chessCombat::checkEnd()
{
    //是否结束了，一方血量为0
    if (m_cur_game)
    {
        if (m_cur_game->m_state == COMBAT_STATE_END)
        {
            cout << "combat!!!end!!! attack_cid=" << m_players[0].m_cid << endl;
            cout << "combat!!!end!!! defense_cid=" << m_players[1].m_cid << endl;
            cout << "combat!!!end!!! quit_cid=" << m_quit_cid << endl;
            if (m_quit_cid == m_players[0].m_cid
                || m_players[0].m_hp_left <= 0)
            {
                m_state = COMBAT_STATE_END;
                m_result = COMBAT_RESULT_ATTACK_LOSE;
                //90三星，60两星，其他一星
                double hp_per = (double)m_players[1].m_hp_left / (double)m_players[1].m_hp_max;
                if (hp_per > 0.9)
                {
                    m_result_type = 3;
                }
                else if(hp_per > 0.6)
                {
                    m_result_type = 2;
                }
                else
                {
                    m_result_type = 1;
                }
                json_spirit::mObject obj;
                obj["cmd"] = "new_combat_end";
                obj["id"] = m_combat_id;
                InsertInternalActionWork(obj);
                return HC_SUCCESS;
            }
            else if (m_quit_cid == m_players[1].m_cid
                     || m_players[1].m_hp_left <= 0)
            {
                m_state = COMBAT_STATE_END;
                m_result = COMBAT_RESULT_ATTACK_WIN;
                //90三星，60两星，其他一星
                double hp_per = (double)m_players[0].m_hp_left / (double)m_players[0].m_hp_max;
                if (hp_per > 0.9)
                {
                    m_result_type = 3;
                }
                else if(hp_per > 0.6)
                {
                    m_result_type = 2;
                }
                else
                {
                    m_result_type = 1;
                }
                json_spirit::mObject obj;
                obj["cmd"] = "new_combat_end";
                obj["id"] = m_combat_id;
                InsertInternalActionWork(obj);
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

chessGame* chessCombat::start_a_game()
{
    chessGame* new_game = new chessGame(&m_players[0], &m_players[1]);
    new_game->m_combat = this;
    return new_game;
}

chessPlayer* chessCombat::getPlayer(int cid)
{
    if (cid == m_players[0].m_cid)
    {
        return &m_players[0];
    }
    else if (cid == m_players[1].m_cid)
    {
        return &m_players[1];
    }
    return NULL;
}

void chessCombat::getCombatInfo(int cid, json_spirit::Object& robj)
{
    chessPlayer* wo = getPlayer(cid);
    chessPlayer* ta = NULL;
    if (NULL == wo)
    {
        wo = &m_players[0];
        ta = &m_players[1];
    }
    else if (wo == &m_players[0])
    {
        ta = &m_players[1];
    }
    else
    {
        ta = &m_players[0];
    }
    json_spirit::Object wobj;
    wo->getInfo(wobj);
    robj.push_back( Pair("wo", wobj) );

    json_spirit::Object tobj;
    ta->getInfo(tobj);
    robj.push_back( Pair("ta", tobj) );
}

void chessCombat::AppendResult(json_spirit::Object obj)
{
    m_result_array.push_back(obj);
}

chessCombatMgr::chessCombatMgr()
{
    rwlock_init(&chessCombats_rwmutex);;
}

chessCombat* chessCombatMgr::findCombat(int id)
{
    readLock lockit(&chessCombats_rwmutex);
    std::map<int, boost::shared_ptr<chessCombat> >::iterator it = m_combats.find(id);
    if (it != m_combats.end())
    {
        return it->second.get();
    }
    return NULL;
}

chessCombat* chessCombatMgr::getCharCombat(int cid)
{
    std::map<int, boost::shared_ptr<chessCombat> >::iterator it = m_char_combats.find(cid);
    if (it != m_char_combats.end())
    {
        return it->second.get();
    }
    return NULL;
}

chessCombat* chessCombatMgr::createArenaCombat(CharData& c1, CharData& c2)
{
    chessCombat* new_combat = new chessCombat;
    new_combat->m_type = COMBAT_TYPE_ARENA;

    if (new_combat->m_players[0].LoadCharactor(c1) == -1)
        return NULL;

    if (new_combat->m_players[1].LoadCharactor(c2) == -1)
        return NULL;
    new_combat->m_players[1].m_auto = true;
    new_combat->m_players[1].m_ignore = true;

    //创建战斗牌局等候客户端确认才开始
    new_combat->m_cur_game = new_combat->start_a_game();

    boost::shared_ptr<chessCombat> spcombat(new_combat);
    writeLock lockit(&chessCombats_rwmutex);
    m_combats[new_combat->m_combat_id] = spcombat;
    m_char_combats[c1.m_id] = spcombat;
    //m_char_combats[c2.m_id] = spcombat;

    cout<<"createCombat("<<c1.m_id<<","<<c2.m_id<<")"<<endl;
    return new_combat;
}

chessCombat* chessCombatMgr::createStrongholdCombat(CharData& c1, int strongholdid, bool random_extra)
{
    chessCombat* new_combat = new chessCombat;
    new_combat->m_type = COMBAT_TYPE_STRONGHOLD;
    new_combat->m_data_id = strongholdid;
    if (strongholdid == 1 && c1.m_cur_strongholdid == 0)
    {
        new_combat->m_ignore_time += iONE_DAY_SECS;
    }
    int extra = 1;
    if (random_extra)
    {
        int tmp = my_random(1, 100);
        if (tmp < 33)
        {
            extra = 2;
        }
        else if(tmp < 66)
        {
            extra = 3;
        }
        else
        {
            extra = 5;
        }
    }
    new_combat->m_extra_data[0] = extra;

    if (new_combat->m_players[0].LoadCharactor(c1) == -1)
        return NULL;

    if (new_combat->m_players[1].LoadStronghold(c1, strongholdid, extra) == -1)
        return NULL;

    //创建战斗牌局等候客户端确认才开始
    new_combat->m_cur_game = new_combat->start_a_game();

    boost::shared_ptr<chessCombat> spcombat(new_combat);
    writeLock lockit(&chessCombats_rwmutex);
    m_combats[new_combat->m_combat_id] = spcombat;
    m_char_combats[c1.m_id] = spcombat;
    //m_char_combats[c2.m_id] = spcombat;

    cout<<"createStrongholdCombat("<<c1.m_id<<","<<strongholdid<<")"<<endl;
    return new_combat;
}

chessCombat* chessCombatMgr::createCopyCombat(CharData& c1, int copyid)
{
    chessCombat* new_combat = new chessCombat;
    new_combat->m_type = COMBAT_TYPE_COPY;
    new_combat->m_data_id = copyid;

    if (new_combat->m_players[0].LoadCharactor(c1) == -1)
        return NULL;

    if (new_combat->m_players[1].LoadCopy(c1, copyid) == -1)
        return NULL;

    //创建战斗牌局等候客户端确认才开始
    new_combat->m_cur_game = new_combat->start_a_game();

    boost::shared_ptr<chessCombat> spcombat(new_combat);
    writeLock lockit(&chessCombats_rwmutex);
    m_combats[new_combat->m_combat_id] = spcombat;
    m_char_combats[c1.m_id] = spcombat;
    //m_char_combats[c2.m_id] = spcombat;

    cout<<"createCopyCombat("<<c1.m_id<<","<<copyid<<")"<<endl;
    return new_combat;
}

chessCombat* chessCombatMgr::createWildCityCombat(CharData& c1, CharData& c2, int cityid, int hid)
{
    chessCombat* new_combat = new chessCombat;
    new_combat->m_type = COMBAT_TYPE_WILD_CITY;
    new_combat->m_data_id = cityid;

    if (new_combat->m_players[0].LoadCharactor(c1) == -1)
        return NULL;

    if (new_combat->m_players[1].LoadCharactor(c2, hid) == -1)
        return NULL;
    new_combat->m_players[1].m_auto = true;
    new_combat->m_players[1].m_ignore = true;

    //创建战斗牌局等候客户端确认才开始
    new_combat->m_cur_game = new_combat->start_a_game();

    boost::shared_ptr<chessCombat> spcombat(new_combat);
    writeLock lockit(&chessCombats_rwmutex);
    m_combats[new_combat->m_combat_id] = spcombat;
    m_char_combats[c1.m_id] = spcombat;
    //m_char_combats[c2.m_id] = spcombat;

    cout<<"createWildCityCombat("<<c1.m_id<<","<<c2.m_id<<")"<<endl;
    return new_combat;
}

chessCombat* chessCombatMgr::createShenlingCombat(CharData& c1, int sid)
{
    chessCombat* new_combat = new chessCombat;
    new_combat->m_type = COMBAT_TYPE_SHENLING;
    new_combat->m_data_id = sid;

    if (new_combat->m_players[0].LoadCharactor(c1) == -1)
        return NULL;

    if (new_combat->m_players[1].LoadShenling(c1, sid) == -1)
        return NULL;

    //创建战斗牌局等候客户端确认才开始
    new_combat->m_cur_game = new_combat->start_a_game();

    boost::shared_ptr<chessCombat> spcombat(new_combat);
    writeLock lockit(&chessCombats_rwmutex);
    m_combats[new_combat->m_combat_id] = spcombat;
    m_char_combats[c1.m_id] = spcombat;
    //m_char_combats[c2.m_id] = spcombat;

    cout<<"createShenlingCombat("<<c1.m_id<<","<<sid<<")"<<endl;
    return new_combat;
}

chessCombat* chessCombatMgr::createTreasureCombat(CharData& c1, CharData& c2)
{
    chessCombat* new_combat = new chessCombat;
    new_combat->m_type = COMBAT_TYPE_TREASURE_ROB;

    if (new_combat->m_players[0].LoadCharactor(c1) == -1)
        return NULL;

    if (new_combat->m_players[1].LoadCharactor(c2) == -1)
        return NULL;
    new_combat->m_players[1].m_auto = true;
    new_combat->m_players[1].m_ignore = true;

    //创建战斗牌局等候客户端确认才开始
    new_combat->m_cur_game = new_combat->start_a_game();

    boost::shared_ptr<chessCombat> spcombat(new_combat);
    writeLock lockit(&chessCombats_rwmutex);
    m_combats[new_combat->m_combat_id] = spcombat;
    m_char_combats[c1.m_id] = spcombat;
    //m_char_combats[c2.m_id] = spcombat;

    cout<<"createTreasureCombat("<<c1.m_id<<","<<c2.m_id<<")"<<endl;
    return new_combat;
}

chessCombat* chessCombatMgr::createGuildMoshenCombat(CharData& c1, int moshenid)
{
    Guild* cp = Singleton<guildMgr>::Instance().getGuild(c1.GetGuildId());
    if (!cp)
    {
        return NULL;
    }
    bool buffed = false;
    if (moshenid <= cp->m_moshen_list.size())
    {
        buffed = true;
    }

    chessCombat* new_combat = new chessCombat;
    new_combat->m_type = COMBAT_TYPE_GUILD_MOSHEN;
    new_combat->m_data_id = moshenid;

    if (new_combat->m_players[0].LoadCharactor(c1) == -1)
        return NULL;
    if (buffed)
    {
        //攻击增加100%,免伤增加80%
        new_combat->m_players[0].m_combat_attribute.set_damage_per(100);
        new_combat->m_players[0].m_combat_attribute.set_damage_resist_per(80);
    }

    if (new_combat->m_players[1].LoadGuildMoshen(moshenid) == -1)
        return NULL;

    //创建战斗牌局等候客户端确认才开始
    new_combat->m_cur_game = new_combat->start_a_game();

    boost::shared_ptr<chessCombat> spcombat(new_combat);
    writeLock lockit(&chessCombats_rwmutex);
    m_combats[new_combat->m_combat_id] = spcombat;
    m_char_combats[c1.m_id] = spcombat;
    //m_char_combats[c2.m_id] = spcombat;

    cout<<"createGuildMoshenCombat("<<c1.m_id<<","<<moshenid<<")"<<endl;
    return new_combat;
}

chessCombat* chessCombatMgr::createBossCombat(CharData& c1, int id)
{
    boost::shared_ptr<Boss> spb = bossMgr::getInstance()->getBoss(id);
    Boss* pb = spb.get();
    if (NULL == pb)
    {
        ERR();
        return NULL;
    }

    chessCombat* new_combat = new chessCombat;
    new_combat->m_type = COMBAT_TYPE_BOSS;
    new_combat->m_data_id = id;

    if (new_combat->m_players[0].LoadCharactor(c1) == -1)
    {
        ERR();
        return NULL;
    }
    //鼓舞效果
    if (new_combat->m_players[0].m_combat_attribute.boss_inspired())
    {
        //攻击增加
        new_combat->m_players[0].m_combat_attribute.set_damage_per(new_combat->m_players[0].m_combat_attribute.boss_inspired());
    }

    if (new_combat->m_players[1].LoadBoss(pb) == -1)
    {
        ERR();
        return NULL;
    }

    //创建战斗牌局等候客户端确认才开始
    new_combat->m_cur_game = new_combat->start_a_game();

    boost::shared_ptr<chessCombat> spcombat(new_combat);
    writeLock lockit(&chessCombats_rwmutex);
    m_combats[new_combat->m_combat_id] = spcombat;
    m_char_combats[c1.m_id] = spcombat;
    //m_char_combats[c2.m_id] = spcombat;

    cout<<"createBossCombat("<<c1.m_id<<","<<id<<")"<<endl;
    return new_combat;
}

void chessCombatMgr::autoLoop()
{
    readLock lockit(&chessCombats_rwmutex);
    time_t t_now = time(NULL);
    std::map<int, boost::shared_ptr<chessCombat> >::iterator it = m_combats.begin();
    while (it != m_combats.end())
    {
        if (!it->second.get() || it->second->m_state != COMBAT_STATE_INIT)
        {
            ++it;
            continue;
        }
        chessCombat* p = it->second.get();
        if (p->m_client_check <= 0)
        {
            //通知客户端全部就绪
            json_spirit::Object o;
            o.push_back( Pair("cmd", "new_combat_ready") );
            o.push_back( Pair("s", 200) );
            p->m_cur_game->broadCastMsg(json_spirit::write(o, json_spirit::raw_utf8));

            //客户端确认则正式开始
            p->m_state = COMBAT_STATE_INGOING;
            //开场通知牌桌信息
            p->m_cur_game->broadCastChess();
            //定时器开始
            json_spirit::mObject mobj;
            mobj["cmd"] = "new_combat_start";
            mobj["id"] = p->m_combat_id;
            boost::shared_ptr<splsTimer> tmsg;
            int wait_time = 1;
            tmsg.reset(new splsTimer(wait_time, 1, mobj,1));
            splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        else if (p->m_ignore_time < t_now)
        {
            p->m_state = COMBAT_STATE_END;
            //超时处理
            json_spirit::mObject mobj;
            mobj["cmd"] = "new_combat_timeout";
            mobj["id"] = p->m_combat_id;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(1, 1, mobj,1));
            splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        ++it;
    }
}

void chessCombatMgr::onCombatEnd(int id)
{
    std::map<int, boost::shared_ptr<chessCombat> >::iterator it = m_combats.find(id);
    if (it != m_combats.end())
    {
        boost::shared_ptr<chessCombat> cbt = it->second;
        if (cbt.get())
        {
            chessCombat* pb = cbt.get();
            json_spirit::Object o;
            o.push_back( Pair("cmd", "new_combat_result"));
            o.push_back( Pair("s", 200));
            if (IS_NOT_NPC(pb->m_players[0].m_cid))
            {
                json_spirit::Object attacker_extra;
                pb->m_players[0].getExtraInfo(attacker_extra);
                o.push_back( Pair("org_attacker_data", attacker_extra));
            }
            if (IS_NOT_NPC(pb->m_players[1].m_cid))
            {
                json_spirit::Object defenser_extra;
                pb->m_players[1].getExtraInfo(defenser_extra);
                o.push_back( Pair("org_defenser_data", defenser_extra));
            }
            //各类战斗结果处理
            switch (pb->m_type)
            {
                case COMBAT_TYPE_STRONGHOLD:
                    Singleton<mapMgr>::Instance().combatResult(pb);
                    break;
                case COMBAT_TYPE_ARENA:
                    Singleton<arenaMgr>::Instance().combatResult(pb);
                    break;
                case COMBAT_TYPE_COPY:
                    Singleton<copyMgr>::Instance().combatResult(pb);
                    break;
                case COMBAT_TYPE_WILD_CITY:
                    Singleton<wildMgr>::Instance().combatResult(pb);
                    break;
                case COMBAT_TYPE_SHENLING:
                    Singleton<shenlingMgr>::Instance().combatResult(pb);
                    break;
                case COMBAT_TYPE_TREASURE_ROB:
                    Singleton<treasureMgr>::Instance().combatResult(pb);
                    break;
                case COMBAT_TYPE_GUILD_MOSHEN:
                    Singleton<guildMgr>::Instance().combatResult(pb);
                    break;
                case COMBAT_TYPE_BOSS:
                    bossMgr::getInstance()->combatResult(pb);
                    break;
            }
            if (IS_NOT_NPC(pb->m_players[0].m_cid))
            {
                json_spirit::Object attacker_extra;
                pb->m_players[0].getExtraInfo(attacker_extra);
                o.push_back( Pair("attacker_data", attacker_extra));
            }
            if (IS_NOT_NPC(pb->m_players[1].m_cid))
            {
                json_spirit::Object defenser_extra;
                pb->m_players[1].getExtraInfo(defenser_extra);
                o.push_back( Pair("defenser_data", defenser_extra));
            }
            json_spirit::Object attacker;
            json_spirit::Object defenser;
            pb->m_players[0].getInfo(attacker);
            pb->m_players[1].getInfo(defenser);
            o.push_back( Pair("type", pb->m_type));
            o.push_back( Pair("quit_cid", pb->m_quit_cid));
            o.push_back( Pair("attacker", attacker));
            o.push_back( Pair("defenser", defenser));
            pb->m_result_obj.push_back( Pair("extra1", pb->m_extra_data[0]) );
            pb->m_result_obj.push_back( Pair("extra2", pb->m_extra_data[1]) );
            pb->m_result_obj.push_back( Pair("result", pb->m_result) );
            pb->m_result_obj.push_back( Pair("result_type", pb->m_result_type) );
            o.push_back( Pair("result", pb->m_result_obj));
            //结果发送
            std::string msg = write(o);
            boost::shared_ptr<OnlineCharactor> p1 = GeneralDataMgr::getInstance()->GetOnlineCharactor(pb->m_players[0].m_player_name);
            if (p1.get() && !pb->m_players[0].m_ignore)
            {
                p1->Send(msg);
            }
            boost::shared_ptr<OnlineCharactor> p2 = GeneralDataMgr::getInstance()->GetOnlineCharactor(pb->m_players[1].m_player_name);
            if (p2.get() && !pb->m_players[1].m_ignore)
            {
                p2->Send(msg);
            }
            pb->AppendResult(o);
            //保存战斗
            {
                //战斗结果存入数据库
                std::string strRecord = "{\"cmd\":\"getCombatRecord\",\"s\":200,\"cmdlist\":" + json_spirit::write(pb->m_result_array) + "}";
                Singleton<combatRecordMgr>::Instance().addCombatRecord(pb->m_combat_id, strRecord);
                InsertSaveDb("insert into battle_records (id,type,attacker,defender,attackerName,defenderName,input,record,_date,result,aLevel,dLevel,archive,extra1,extra2) values ("
                                + LEX_CAST_STR(pb->m_combat_id)
                                + "," + LEX_CAST_STR(pb->m_type)
                                + "," + LEX_CAST_STR(pb->m_players[0].m_cid)
                                + "," + LEX_CAST_STR(pb->m_players[1].m_cid)
                                + ",'" + GetDb().safestr(pb->m_players[0].m_player_name)
                                + "','" + GetDb().safestr(pb->m_players[1].m_player_name)
                                + "',unix_timestamp(),'" + GetDb().safestr(strRecord)
                                + "',curdate()," + LEX_CAST_STR(pb->m_result)
                                + "," + LEX_CAST_STR(pb->m_players[0].m_level)
                                + "," + LEX_CAST_STR(pb->m_players[1].m_level)
                                + "," + LEX_CAST_STR(pb->m_archive_report)
                                + "," + LEX_CAST_STR(pb->m_extra_data[0])
                                + "," + LEX_CAST_STR(pb->m_extra_data[1])
                                +")");
            }
            //数据清理
            delete pb->m_cur_game;
            pb->m_cur_game = NULL;
            if (pb->m_players[0].m_cid > 0)
            {
                chessCombat* tmpCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(pb->m_players[0].m_cid);
                if (tmpCombat && tmpCombat->m_combat_id == pb->m_combat_id)
                {
                    m_char_combats.erase(pb->m_players[0].m_cid);
                }
            }
            if (pb->m_players[1].m_cid > 0)
            {
                chessCombat* tmpCombat = Singleton<chessCombatMgr>::Instance().getCharCombat(pb->m_players[1].m_cid);
                if (tmpCombat && tmpCombat->m_combat_id == pb->m_combat_id)
                {
                    m_char_combats.erase(pb->m_players[1].m_cid);
                }
            }
        }
        writeLock lockit(&chessCombats_rwmutex);
        m_combats.erase(it);
    }
}

void chessCombatMgr::combatTimeout(int id)
{
    chessCombat* pb = findCombat(id);
    if (pb)
    {
        delete pb->m_cur_game;
        pb->m_cur_game = NULL;
        if (pb->m_players[0].m_cid > 0)
        {
            chessCombat* tmpCombat = getCharCombat(pb->m_players[0].m_cid);
            if (tmpCombat && tmpCombat->m_combat_id == pb->m_combat_id)
            {
                m_char_combats.erase(pb->m_players[0].m_cid);
            }
        }
        if (pb->m_players[1].m_cid > 0)
        {
            chessCombat* tmpCombat = getCharCombat(pb->m_players[1].m_cid);
            if (tmpCombat && tmpCombat->m_combat_id == pb->m_combat_id)
            {
                m_char_combats.erase(pb->m_players[1].m_cid);
            }
        }
        writeLock lockit(&chessCombats_rwmutex);
        m_combats.erase(id);
    }
}

bool chessCombatProcesser::work(chessCombatCmd &Cmd)       // 在些完成实际任务.
{
    try
    {
        int ret = HC_SUCCESS;
        std::string cmd = "";
        json_spirit::Object robj;
        if (Cmd.cmd == combat_cmd_create)
        {
            cmd = "new_combat_challenge";
            ret = createChessCombat(Cmd.cid,Cmd.mobj,robj);
        }
        else
        {
            chessCombat* combat = Cmd._pCombat;
            if (combat == NULL)
            {
                std::cout<<"CombatProcess receive a empty combat...break."<<endl;
                return false;
            }
            //处理命令
            switch (Cmd.cmd)
            {
                case combat_cmd_player_act:
                    {
                        cmd = "new_combat_act";
                        int seq = 0, pos1 = 0, x1 = 0, y1 = 0, pos2 = 0, x2 = 0, y2 = 0;
                        READ_INT_FROM_MOBJ(seq,Cmd.mobj,"seq");
                        json_spirit::mArray list;
                        READ_ARRAY_FROM_MOBJ(list,Cmd.mobj,"list");
                        json_spirit::mArray::iterator it = list.begin();
                        while (it != list.end())
                        {
                            if ((*it).type() != json_spirit::obj_type)
                            {
                                ++it;
                                continue;
                            }
                            json_spirit::mObject& tmp_obj = (*it).get_obj();
                            int pos = 0, x = 0, y = 0;
                            READ_INT_FROM_MOBJ(pos,tmp_obj,"pos");
                            READ_INT_FROM_MOBJ(x,tmp_obj,"x");
                            READ_INT_FROM_MOBJ(y,tmp_obj,"y");
                            if (pos <= 0 || pos > 4 || x <= 0 || x > 5 || y <= 0 || y > 5)
                            {
                                ERR();
                                ret = HC_ERROR;
                            }
                            cout<<"new_combat_act("<<Cmd.cid<<","<<pos<<","<<x<<","<<y<<","<<seq<<"), combat id "<<combat->m_combat_id<<endl;
                            if (pos1 == 0)
                            {
                                pos1 = pos;
                                x1 = x;
                                y1 = y;
                            }
                            else
                            {
                                pos2 = pos;
                                x2 = x;
                                y2 = y;
                            }
                            ++it;
                        }
                        if (ret == HC_SUCCESS && combat->m_cur_game && combat->m_state == COMBAT_STATE_INGOING && combat->m_cur_game->m_state == COMBAT_STATE_INGOING)
                        {
                            ret = combat->m_cur_game->act(Cmd.cid, seq, pos1, x1, y1, pos2, x2, y2);
                        }
                        else
                        {
                            ERR();
                            ret = HC_ERROR;
                        }
                    }
                    break;
                case combat_cmd_player_quit:
                    {
                        cmd = "new_combat_quit";
                        combat->quit(Cmd.cid);
                    }
                    break;
                case combat_cmd_player_pass:
                    {
                        cmd = "new_combat_pass";
                        if (combat->m_state != COMBAT_STATE_END && combat->m_cur_game && combat->m_cur_game->m_state != COMBAT_STATE_END)
                        {
                            combat->m_cur_game->pass(Cmd.cid, robj);
                        }
                    }
                    break;
                case combat_cmd_player_cast:
                    {
                        cmd = "new_combat_cast";
                        int magic_id = 0;
                        READ_INT_FROM_MOBJ(magic_id,Cmd.mobj,"id");
                        if (combat->m_state != COMBAT_STATE_END && combat->m_cur_game && combat->m_cur_game->m_state != COMBAT_STATE_END)
                        {
                            ret = combat->m_cur_game->cast(Cmd.cid, magic_id);
                        }
                    }
                    break;
                case combat_cmd_player_auto:
                    {
                        cmd = "new_combat_auto";
                        if (combat->m_state != COMBAT_STATE_END && combat->m_cur_game && combat->m_cur_game->m_state != COMBAT_STATE_END)
                        {
                            combat->m_cur_game->autoAct(Cmd.cid, robj);
                        }
                    }
                    break;
                case combat_cmd_sign:
                    {
                        cmd = "new_combat_sign";
                        if (combat->m_client_check > 0)
                            --combat->m_client_check;
                        robj.push_back( Pair("wait", combat->m_client_check > 0));
                    }
                    break;
                case combat_cmd_next:
                    {
                        if (combat->m_cur_game && combat->m_cur_game->m_state == COMBAT_STATE_INGOING)
                        {
                            //是否铺满
                            if (combat->m_cur_game->m_count >= 25)
                            {
                                combat->m_cur_game->refresh();
                                combat->m_cur_game->broadCastChess(1);
                                //发牌延后定时器开始
                                json_spirit::mObject mobj;
                                mobj["cmd"] = "new_combat_deal";
                                mobj["id"] = combat->m_combat_id;
                                boost::shared_ptr<splsTimer> tmsg;
                                tmsg.reset(new splsTimer(4, 1, mobj,1));
                                splsTimerMgr::getInstance()->addTimer(tmsg);
                            }
                            else
                            {
                                combat->m_cur_game->dealCard();
                            }
                        }
                    }
                    break;
                case combat_cmd_start:
                    {
                        combat->onStart();
                    }
                    break;
                case combat_cmd_deal:
                    {
                        if (combat->m_cur_game && combat->m_cur_game->m_state == COMBAT_STATE_INGOING)
                        {
                            combat->m_cur_game->dealCard();
                        }
                    }
                    break;
                case combat_cmd_end:
                    {
                        Singleton<chessCombatMgr>::Instance().onCombatEnd(combat->m_combat_id);
                    }
                    break;
                case combat_cmd_timeout:
                    {
                        Singleton<chessCombatMgr>::Instance().combatTimeout(combat->m_combat_id);
                    }
                    break;
            }
        }
        if (HC_SUCCESS_NO_RET != ret && Cmd.cid > 0 && cmd != "")
        {
            robj.push_back( Pair("cmd", cmd) );
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
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(Cmd.cname);
            if (account.get())
            {
                account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
            }
        }
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "combat work , Exception: " << e.what() << "\n";
    }
    return true;
}

