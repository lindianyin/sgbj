
#include "stdafx.h"

#include "army.h"
#include "combat.h"
#include "combat_def.h"
#include <iostream>
#include "igeneral.h"
#include "singleton.h"

#include "utils_lang.h"
#include "json_spirit.h"

#include "spls_errcode.h"
#include "utils_all.h"
#include "eliteCombat.h"
#include "maze.h"
#include "char_zst.h"
#include <boost/lexical_cast.hpp>
#include "data.h"
#include "boss.h"

#define INFO(x) cout<<x

using namespace std;
using namespace json_spirit;

int g_break_combat_process = 0;

extern void InsertSaveDb(const std::string& sql);

volatile uint64_t Combat::_refs = 0;

Combat::Combat(Army* attacker, Army* defender)
{
    ++Combat::_refs;
    m_type = 0;

    m_fight_secs = TIME_EVERY_COMBAT;

    m_attacker = attacker;
    m_attacker->setAttacker();
    m_attacker->m_combat = this;
    m_attacker->setEnermy(defender);
    
    m_defender = defender;
    m_defender->m_combat = this;
    m_defender->setEnermy(attacker);

    m_round = 0;
    m_state = 0;
    m_combat_id = 0;
    m_result_type = 0;

    m_mapid = 0;            //关卡所在地图
    m_stronghold_type = 0;    //关卡类型 普通/精英/boss
    m_stronghold_level = 0;    //关卡等级
    m_attack_times = 0;        //已经攻打的次数
    m_extra_chance = 0;        //额外掉落率

    m_extra_data[0] = 0;
    m_extra_data[1] = 0;
    m_extra_viewer = "";

    m_combat_info.clear();  //战斗双方数据
    m_combat_info_text = "";
    m_result_text = "";

    m_levelup_generals.clear();//升级的武将
    m_getItems.clear();          //获得的东西

    m_result_obj.clear();       //战斗结果
    m_result_array.clear();   //战斗过程结果

    m_failMsg = "";

    m_archive_report = 0;

    m_final_result = "";

    m_position = 0;

    m_mail_to = 0;
    m_mail_to_name = "";
    m_mail_title = "";
    m_mail_content = "";

    //memset(m_levelup_generals, 0, sizeof(int)*5);
}

Combat::~Combat()
{
    --Combat::_refs;
}

uint64_t Combat::refs()
{
    return Combat::_refs;
}

json_spirit::Object& Combat::getCombatInfo()
{
    return m_combat_info;
}

void Combat::setCombatInfo()
{
    m_combat_info_text = json_spirit::write(m_combat_info);
}

bool Combat::CheckEnd()
{
    //攻击方无血失败
    if (!m_attacker->IsLive())
    {
        //DEBUG_OUT(strCombatResult[1]);
        m_state = defender_win;
        return true;
    }
    //防守方无血失败
    if (!m_defender->IsLive())
    {
        //DEBUG_OUT(strCombatResult[0]);
        m_state = attacker_win;
        return true;
    }
    //双方都没有攻击力，判定为防守方胜利
    if (m_defender->NoAttack() && m_attacker->NoAttack())
    {
        m_state = defender_win;
        //DEBUG_OUT(strCombatResult[1]);
        return true;
    }
    return false;
}

int Combat::CalcResult()
{
    m_round = 1;
    AppendRoundInfo();
    int attack_pos = 0;
    int defend_pos = 0;
    do
    {
        //是否回合结束
        if (attack_pos == 9 && defend_pos == 9)
        {
            attack_pos = 0;
            defend_pos = 0;
            m_attacker->Reset();
            m_defender->Reset();
            ++m_round;
            AppendRoundInfo();
            if (m_round > 25)
            {
                m_state = attacker_win;
                break;
            }
        }
        //攻击方先出手
        if (attack_pos != 9)
        {
            attack_pos = m_attacker->Action();
        }
        if (CheckEnd())
        {
            break;
        }
        if (defend_pos != 9)
        {
            defend_pos = m_defender->Action();
        }
    } while (!CheckEnd());

    m_attacker->Calc();
    m_defender->Calc();
    //cout<<m_result<<endl;

    //战斗结果 
    m_result_type = 1;
    if (m_state == attacker_win)
    {
        /*    完胜  胜利时我方剩余总兵力≥80%  1
            大胜  胜利时我方剩余总兵力≥60%  2
            胜利  胜利时我方剩余总兵力≥40%  3
            小胜  胜利时我方剩余总兵力≥20%  4
            险胜  胜利时我方剩余总兵力＜20%  5 */
        m_result_type = 1 + (uint64_t)5 * m_attacker->DieHp() / m_attacker->TotalMaxHp();
        //cout<<" win "<<m_attacker->DieHp()<<"/"<<m_attacker->TotalMaxHp()<<", result "<<type<<endl;
    }
    else
    {
        /*    溃败  失败时敌方剩余总兵力≥80% 6
            大败  失败时敌方剩余总兵力≥60% 7
            失败  失败时敌方剩余总兵力≥40% 8
            小败  失败时敌方剩余总兵力≥20% 9
            惜败  失败时敌方剩余总兵力＜20%    10    */
        m_result_type = 6 + (uint64_t)5 * m_defender->DieHp() / m_defender->TotalMaxHp();
        //cout<<" lose "<<m_defender->DieHp()<<"/"<<m_defender->TotalMaxHp()<<", result "<<type<<endl;
    }

    std::string memo = m_state == attacker_win ?strCombatResultMemo : strCombatResultMemo2;
    if (defender_win == m_state && 1 == m_type)
    {
        memo = m_failMsg;
    }
    else
    {
        str_replace(memo, "$A", m_attacker->Name());
        str_replace(memo, "$D", m_defender->Name());
    }

    json_spirit::Object& robj = m_result_obj;
    robj.push_back( Pair("cmd", "battleResult") );
    robj.push_back( Pair("position", m_position) );
    robj.push_back( Pair("type", m_type) );

    if (combat_servant == m_type)
    {
        robj.push_back( Pair("stype", m_extra_data[0]) );
    }
    
    robj.push_back( Pair("result", m_result_type) );
    robj.push_back( Pair("rounds", m_round) );
    robj.push_back( Pair("die", m_attacker->DieHp()) );
    robj.push_back( Pair("die2", m_defender->DieHp()) );

    if (m_type == 1)
    {
        robj.push_back( Pair("cost", m_attacker->_army_data->m_hp_cost) );
        robj.push_back( Pair("cost2", m_defender->_army_data->m_hp_cost) );
    }
    else
    {
        robj.push_back( Pair("cost", 0) );
        robj.push_back( Pair("cost2", 0) );
    }

    robj.push_back( Pair("memo", memo) );

    if (combat_group_copy == m_type)
    {
        m_result_array.push_back(m_result_obj);
        std::string strResult = json_spirit::write(m_result_array);
        m_final_result = "{\"cmd\":\"battle\",\"s\":200,\"cmdlist\":" + strResult + "}";
    }

    //json_spirit::Object oo;
    //oo.push_back( Pair("cmd", "getBattleList") );
    //oo.push_back( Pair("s", "200") );
    //oo.push_back( Pair("getBattleList", m_combat_info) );
    //m_combat_info_text = json_spirit::write(oo, json_spirit::raw_utf8);
    return m_state;
}

void Combat::AppendResult(json_spirit::Object obj)
{
    m_result_array.push_back(obj);
}

void Combat::AppendRoundInfo()
{
    json_spirit::Object obj;
    obj.push_back( Pair("act", 0) );
    obj.push_back( Pair("round", m_round) );
    m_result_array.push_back(obj);
}

Combat* createStrongholdCombat(int cid, int strongholdId, int& ret)
{
    //cout<<"createStrongholdCombat()!!!!!!"<<endl;
    int attack_times = 0;
    int extra_chance = 0;
    boost::shared_ptr<StrongholdData> shold = GeneralDataMgr::getInstance()->GetStrongholdData(strongholdId);
    if (!shold.get())
    {
        ret = HC_ERROR;
        return NULL;
    }
    boost::shared_ptr<CharStrongholdData> cd = GeneralDataMgr::getInstance()->GetCharStrongholdData(cid,shold->m_map_id,shold->m_stage_id, shold->m_strongholdpos);
    if (cd.get())
    {
        if (cd->m_state < 0)
        {
            ERR();
            ret = HC_ERROR;
            return NULL;
        }
        attack_times = cd->m_state;
        //第一次攻打关卡，如果掉落物是武将，则100%
        if (attack_times == 0  && shold->m_Item.get() && shold->m_Item->type == item_type_general)
        {
            extra_chance = 10000;
        }        
        else if (attack_times == 0)
        {
            //蒋门神涉及到新手引导-必掉一个盘龙棍
            if (55 == strongholdId)
            {
                extra_chance = 10000;
            }
        }
    }
    else
    {
        ERR();
        cout<<"cid:"<<cid<<",mapid"<<shold->m_map_id<<"stage"<<shold->m_stage_id<<"pos"<<shold->m_strongholdpos<<endl;
        ret = HC_ERROR;
        return NULL;
    }
    //被攻击方 关卡
    army_data* pArmy_data = new army_data;
    Army* pDefender = new Army(pArmy_data);

    //攻击方 - 玩家角色
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);
    pCombat->m_type = combat_stronghold;
    pCombat->m_type_id = strongholdId;
    pCombat->m_mapid = shold->m_map_id;
    pCombat->m_stageId = shold->m_stage_id;
    pCombat->m_strongholdId = shold->m_id;
    pCombat->m_strongholdPos = shold->m_strongholdpos;
    pCombat->m_stronghold_type = shold->m_isepic;
    pCombat->m_stronghold_level = shold->m_level;
    pCombat->m_attack_times = attack_times;
    pCombat->m_extra_chance = extra_chance;
    pCombat->m_failMsg = shold->m_failMsg;

    if (0 != pArmy_data->LoadStronghold(*(cd.get()), strongholdId))
    {
        delete pArmy_data;
        delete pDefender;
        ret = HC_ERROR;
        return NULL;
    }

    pArmy_data_a->LoadCharactor(cid);

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );

    ret = HC_SUCCESS;
    return pCombat;
}


//初始化竞技战斗
Combat* createRaceCombat(int cid, int tid, int& ret)
{
    //cout<<"********************Combat::InitRaceCombat*********************"<<endl;
    //被攻击方    
    army_data* pArmy_data_d = new army_data;    
    Army* pDefender = new Army(pArmy_data_d);

    //攻击方
    army_data* pArmy_data_a = new army_data;    
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);

    pCombat->m_type = combat_race;
    pCombat->m_type_id = tid;

    pArmy_data_d->LoadCharactor(tid);
    pArmy_data_a->LoadCharactor(cid);

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );

    ret = HC_SUCCESS;
    return pCombat;
}

//初始化家丁战斗
Combat* createServantCombat(int cid, int tid, int extra_data, int extra_data2, int& ret)
{
    //cout<<"********************Combat::InitRaceCombat*********************"<<endl;
    //被攻击方
    army_data* pArmy_data_d = new army_data;
    Army* pDefender = new Army(pArmy_data_d);

    //攻击方
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);
    pCombat->m_extra_data[0] = extra_data;
    pCombat->m_extra_data[1] = extra_data2;

    pCombat->m_type = combat_servant;
    pCombat->m_type_id = tid;

    pArmy_data_d->LoadCharactor(tid);
    pArmy_data_a->LoadCharactor(cid);

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );
    pCombat->m_combat_info.push_back( Pair("stype", pCombat->m_extra_data[0]) );

    ret = HC_SUCCESS;
    return pCombat;
}

//初始化boss战斗
Combat* createBossCombat(int cid, int tid, int& ret)
{
    //cout<<"############### InitBossCombat "<<cid<<","<<tid<<endl;

    //加载boss
    spls_boss* bs = bossMgr::getInstance()->getBoss(tid, cid).get();
    if (bs == NULL)
    {
        ret = HC_ERROR;
        return NULL;
    }
    army_data* pArmy_data_d = new army_data;    
    pArmy_data_d->m_army = new Army(pArmy_data_d);
    if (HC_SUCCESS != pArmy_data_d->LoadBoss(*bs))
    {
        delete pArmy_data_d->m_army;
        delete pArmy_data_d;
        ret = HC_ERROR;
        return NULL;
    }
    Army* pDefender = pArmy_data_d->m_army;

    //攻击方 - 玩家角色
    army_data* pArmy_data_a = new army_data;    
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);

    pCombat->m_type = combat_boss;
    pCombat->m_type_id = tid;

    pCombat->m_combat_info.clear();        //战斗双方数据
    pCombat->m_combat_info_text = "";
    pCombat->m_result_text = "";

    pCombat->m_levelup_generals.clear(); //升级的武将
    pCombat->m_getItems.clear();         //获得的东西

    pCombat->m_result_obj.clear();        //战斗结果
    pCombat->m_result_array.clear();     //战斗过程结果

    pArmy_data_a->LoadCharactor(cid);

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );

    //cout<<"############### InitBossCombat success "<<cid<<","<<tid<<endl;

    ret = HC_SUCCESS;
    return pCombat;
}

int createBossCombat(int cid, Combat* pCombat)
{
    if (!pCombat->m_defender)
    {
        return HC_ERROR;
    }
    pCombat->m_defender->m_action_pos = -1;
    //攻击方 - 玩家角色
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    pCombat->m_attacker = pAttacker;
    pAttacker->m_combat = pCombat;
    pAttacker->setAttacker();
    pCombat->m_attacker->setEnermy(pCombat->m_defender);
    pCombat->m_defender->setEnermy(pCombat->m_attacker);

    pArmy_data_a->LoadCharactor(cid);

    pCombat->m_attacker->setCombat(pCombat);

    pCombat->m_combat_info.clear();        //战斗双方数据
    pCombat->m_combat_info_text = "";
    pCombat->m_result_text = "";
    
    pCombat->m_levelup_generals.clear(); //升级的武将
    pCombat->m_getItems.clear();         //获得的东西
    
    pCombat->m_result_obj.clear();        //战斗结果
    pCombat->m_result_array.clear();     //战斗过程结果

    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;

    //清除上一场战斗遗留的buff
    pCombat->m_defender->clearBuff();
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );    
    return HC_SUCCESS;
}

//初始化阵营战的战斗
Combat* createCampRaceCombat(charCampRace* c1, charCampRace* c2, int& ret)
{
    //cout<<"********************Combat::InitCampRaceCombat*********************"<<endl;
    //被攻击方
    army_data* pArmy_data_d = new army_data;    
    Army* pDefender = new Army(pArmy_data_d);

    //攻击方
    army_data* pArmy_data_a = new army_data;    
    Army* pAttacker = new Army(pArmy_data_a);

    //战斗
    Combat* pCombat = new Combat(pAttacker, pDefender);

    pCombat->m_type = combat_camp_race;
    pCombat->m_type_id = 0;

    if (0 != pArmy_data_d->LoadCampRace(c2))
    {
        ret = HC_ERROR;
        ERR();
        return NULL;
    }
    if (0 != pArmy_data_a->LoadCampRace(c1))
    {
        ret = HC_ERROR;
        ERR();
        return NULL;
    }

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    pAttacker->clearBuff();
    pDefender->clearBuff();

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );

    ret = HC_SUCCESS;
    return pCombat;
}


//初始化生辰纲战斗
Combat* createGuardCombat(int cid, int tid, int true_guard_cid, int& ret)
{
    //cout<<"********************Combat::InitGuardCombat*********************"<<endl;
    //被攻击方    
    army_data* pArmy_data_d = new army_data;
    Army* pDefender = new Army(pArmy_data_d);

    //攻击方
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);
    pCombat->m_extra_data[0] = true_guard_cid;

    pCombat->m_type = combat_guard;
    pCombat->m_type_id = tid;

    pArmy_data_d->LoadCharactor(tid);
    pArmy_data_a->LoadCharactor(cid);

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );

    ret = HC_SUCCESS;
    return pCombat;
}

//初始化抢夺通商位置战斗
Combat* createTradeCombat(int cid, int tid, int& ret)
{
    //cout<<"********************Combat::InitTradeCombat*********************"<<endl;
    //被攻击方    
    army_data* pArmy_data_d = new army_data;
    Army* pDefender = new Army(pArmy_data_d);

    //攻击方
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);

    pCombat->m_type = combat_trade;
    pCombat->m_type_id = tid;
    
    pArmy_data_d->LoadCharactor(tid);
    pArmy_data_a->LoadCharactor(cid);

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );

    ret = HC_SUCCESS;
    //cout<<"********************Combat::InitTradeCombatend*********************"<<endl;
    return pCombat;
}

Combat* createEliteCombat(int cid, int mapid, int eliteid, int& ret)
{
    boost::shared_ptr<eliteCombat> pec = eliteCombatMgr::getInstance()->getEliteCombat(mapid,eliteid);
    if (!pec.get())
    {
        ret = HC_ERROR;
        return NULL;
    }
    //被攻击方精英
    army_data* pArmy_data = new army_data;
    Army* pDefender = new Army(pArmy_data);

    //攻击方 - 玩家角色
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);
    pCombat->m_type = combat_elite;
    pCombat->m_type_id = eliteid;
    pCombat->m_mapid = mapid;

    if (0 != pArmy_data->loadEliteCombat(eliteid, pec.get()))
    {
        delete pArmy_data;
        delete pDefender;
        ret = HC_ERROR;
        return NULL;
    }

    pArmy_data_a->LoadCharactor(cid);

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );

    ret = HC_SUCCESS;
    return pCombat;
}

Combat* createMazeCombat(int cid, int maze_army_id, int extra_data, int& ret, bool isBoss)
{
    boost::shared_ptr<mazeMonster> pm = Singleton<mazeMgr>::Instance().getMonsterById(maze_army_id);
    if (!pm.get())
    {
        ret = HC_ERROR;
        return NULL;
    }
    //被攻击方迷宫怪物
    army_data* pArmy_data = new army_data;
    Army* pDefender = new Army(pArmy_data);

    //攻击方 - 玩家角色
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);
    pCombat->m_extra_data[0] = extra_data;
    pCombat->m_type = isBoss ? combat_maze_boss : combat_maze;
    pCombat->m_type_id = maze_army_id;
    //pCombat->m_mapid = mapid;

    //加载迷宫怪物
    if (0 != pArmy_data->loadMazeCombat(maze_army_id, pm.get()))
    {
        delete pArmy_data;
        delete pDefender;
        ret = HC_ERROR;
        return NULL;
    }
    if (!isBoss)
    {
        //从迷宫中加载玩家
        pArmy_data_a->LoadMazeCharactor(cid);
    }
    else
    {
        //加载当前玩家数据
        pArmy_data_a->LoadCharactor(cid);
    }

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );

    ret = HC_SUCCESS;
    return pCombat;
}

Combat* createZSTCombat(int cid, int star, int army_id, int& ret, bool isBoss)
{
    boost::shared_ptr<base_ZST_Stronghold> pzst = Singleton<zstMgr>::Instance().getBaseZSTStronghold(army_id);
    if (!pzst.get())
    {
        ret = HC_ERROR;
        return NULL;
    }
    //被攻击方
    army_data* pArmy_data = new army_data;
    Army* pDefender = new Army(pArmy_data);

    //攻击方 - 玩家角色
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);
    pCombat->m_extra_data[0] = star;
    pCombat->m_type = combat_zst;
    pCombat->m_type_id = army_id;

    if (0 != pArmy_data->loadZSTCombat(army_id, pzst.get()))
    {
        delete pArmy_data;
        delete pDefender;
        ret = HC_ERROR;
        return NULL;
    }
    pArmy_data_a->LoadZSTCharactor(cid);
    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );

    ret = HC_SUCCESS;
    return pCombat;
}

