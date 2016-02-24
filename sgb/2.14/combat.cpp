
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

    m_mapid = 0;            //�ؿ����ڵ�ͼ
    m_stronghold_type = 0;    //�ؿ����� ��ͨ/��Ӣ/boss
    m_stronghold_level = 0;    //�ؿ��ȼ�
    m_attack_times = 0;        //�Ѿ�����Ĵ���
    m_extra_chance = 0;        //���������

    m_extra_data[0] = 0;
    m_extra_data[1] = 0;
    m_extra_viewer = "";

    m_combat_info.clear();  //ս��˫������
    m_combat_info_text = "";
    m_result_text = "";

    m_levelup_generals.clear();//�������佫
    m_getItems.clear();          //��õĶ���

    m_result_obj.clear();       //ս�����
    m_result_array.clear();   //ս�����̽��

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
    //��������Ѫʧ��
    if (!m_attacker->IsLive())
    {
        //DEBUG_OUT(strCombatResult[1]);
        m_state = defender_win;
        return true;
    }
    //���ط���Ѫʧ��
    if (!m_defender->IsLive())
    {
        //DEBUG_OUT(strCombatResult[0]);
        m_state = attacker_win;
        return true;
    }
    //˫����û�й��������ж�Ϊ���ط�ʤ��
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
        //�Ƿ�غϽ���
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
        //�������ȳ���
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

    //ս����� 
    m_result_type = 1;
    if (m_state == attacker_win)
    {
        /*    ��ʤ  ʤ��ʱ�ҷ�ʣ���ܱ�����80%  1
            ��ʤ  ʤ��ʱ�ҷ�ʣ���ܱ�����60%  2
            ʤ��  ʤ��ʱ�ҷ�ʣ���ܱ�����40%  3
            Сʤ  ʤ��ʱ�ҷ�ʣ���ܱ�����20%  4
            ��ʤ  ʤ��ʱ�ҷ�ʣ���ܱ�����20%  5 */
        m_result_type = 1 + (uint64_t)5 * m_attacker->DieHp() / m_attacker->TotalMaxHp();
        //cout<<" win "<<m_attacker->DieHp()<<"/"<<m_attacker->TotalMaxHp()<<", result "<<type<<endl;
    }
    else
    {
        /*    ����  ʧ��ʱ�з�ʣ���ܱ�����80% 6
            ���  ʧ��ʱ�з�ʣ���ܱ�����60% 7
            ʧ��  ʧ��ʱ�з�ʣ���ܱ�����40% 8
            С��  ʧ��ʱ�з�ʣ���ܱ�����20% 9
            ϧ��  ʧ��ʱ�з�ʣ���ܱ�����20%    10    */
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
        //��һ�ι���ؿ���������������佫����100%
        if (attack_times == 0  && shold->m_Item.get() && shold->m_Item->type == item_type_general)
        {
            extra_chance = 10000;
        }        
        else if (attack_times == 0)
        {
            //�������漰����������-�ص�һ��������
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
    //�������� �ؿ�
    army_data* pArmy_data = new army_data;
    Army* pDefender = new Army(pArmy_data);

    //������ - ��ҽ�ɫ
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

    //ս��˫��������Ϣ
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


//��ʼ������ս��
Combat* createRaceCombat(int cid, int tid, int& ret)
{
    //cout<<"********************Combat::InitRaceCombat*********************"<<endl;
    //��������    
    army_data* pArmy_data_d = new army_data;    
    Army* pDefender = new Army(pArmy_data_d);

    //������
    army_data* pArmy_data_a = new army_data;    
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);

    pCombat->m_type = combat_race;
    pCombat->m_type_id = tid;

    pArmy_data_d->LoadCharactor(tid);
    pArmy_data_a->LoadCharactor(cid);

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //ս��˫��������Ϣ
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

//��ʼ���Ҷ�ս��
Combat* createServantCombat(int cid, int tid, int extra_data, int extra_data2, int& ret)
{
    //cout<<"********************Combat::InitRaceCombat*********************"<<endl;
    //��������
    army_data* pArmy_data_d = new army_data;
    Army* pDefender = new Army(pArmy_data_d);

    //������
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

    //ս��˫��������Ϣ
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

//��ʼ��bossս��
Combat* createBossCombat(int cid, int tid, int& ret)
{
    //cout<<"############### InitBossCombat "<<cid<<","<<tid<<endl;

    //����boss
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

    //������ - ��ҽ�ɫ
    army_data* pArmy_data_a = new army_data;    
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);

    pCombat->m_type = combat_boss;
    pCombat->m_type_id = tid;

    pCombat->m_combat_info.clear();        //ս��˫������
    pCombat->m_combat_info_text = "";
    pCombat->m_result_text = "";

    pCombat->m_levelup_generals.clear(); //�������佫
    pCombat->m_getItems.clear();         //��õĶ���

    pCombat->m_result_obj.clear();        //ս�����
    pCombat->m_result_array.clear();     //ս�����̽��

    pArmy_data_a->LoadCharactor(cid);

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //ս��˫��������Ϣ
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
    //������ - ��ҽ�ɫ
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    pCombat->m_attacker = pAttacker;
    pAttacker->m_combat = pCombat;
    pAttacker->setAttacker();
    pCombat->m_attacker->setEnermy(pCombat->m_defender);
    pCombat->m_defender->setEnermy(pCombat->m_attacker);

    pArmy_data_a->LoadCharactor(cid);

    pCombat->m_attacker->setCombat(pCombat);

    pCombat->m_combat_info.clear();        //ս��˫������
    pCombat->m_combat_info_text = "";
    pCombat->m_result_text = "";
    
    pCombat->m_levelup_generals.clear(); //�������佫
    pCombat->m_getItems.clear();         //��õĶ���
    
    pCombat->m_result_obj.clear();        //ս�����
    pCombat->m_result_array.clear();     //ս�����̽��

    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;

    //�����һ��ս��������buff
    pCombat->m_defender->clearBuff();
    pCombat->m_defender->GetObj(dobj);

    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );

    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();

    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );    
    return HC_SUCCESS;
}

//��ʼ����Ӫս��ս��
Combat* createCampRaceCombat(charCampRace* c1, charCampRace* c2, int& ret)
{
    //cout<<"********************Combat::InitCampRaceCombat*********************"<<endl;
    //��������
    army_data* pArmy_data_d = new army_data;    
    Army* pDefender = new Army(pArmy_data_d);

    //������
    army_data* pArmy_data_a = new army_data;    
    Army* pAttacker = new Army(pArmy_data_a);

    //ս��
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

    //ս��˫��������Ϣ
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


//��ʼ��������ս��
Combat* createGuardCombat(int cid, int tid, int true_guard_cid, int& ret)
{
    //cout<<"********************Combat::InitGuardCombat*********************"<<endl;
    //��������    
    army_data* pArmy_data_d = new army_data;
    Army* pDefender = new Army(pArmy_data_d);

    //������
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

    //ս��˫��������Ϣ
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

//��ʼ������ͨ��λ��ս��
Combat* createTradeCombat(int cid, int tid, int& ret)
{
    //cout<<"********************Combat::InitTradeCombat*********************"<<endl;
    //��������    
    army_data* pArmy_data_d = new army_data;
    Army* pDefender = new Army(pArmy_data_d);

    //������
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);

    pCombat->m_type = combat_trade;
    pCombat->m_type_id = tid;
    
    pArmy_data_d->LoadCharactor(tid);
    pArmy_data_a->LoadCharactor(cid);

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //ս��˫��������Ϣ
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
    //����������Ӣ
    army_data* pArmy_data = new army_data;
    Army* pDefender = new Army(pArmy_data);

    //������ - ��ҽ�ɫ
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

    //ս��˫��������Ϣ
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
    //���������Թ�����
    army_data* pArmy_data = new army_data;
    Army* pDefender = new Army(pArmy_data);

    //������ - ��ҽ�ɫ
    army_data* pArmy_data_a = new army_data;
    Army* pAttacker = new Army(pArmy_data_a);

    Combat* pCombat = new Combat(pAttacker, pDefender);
    pCombat->m_extra_data[0] = extra_data;
    pCombat->m_type = isBoss ? combat_maze_boss : combat_maze;
    pCombat->m_type_id = maze_army_id;
    //pCombat->m_mapid = mapid;

    //�����Թ�����
    if (0 != pArmy_data->loadMazeCombat(maze_army_id, pm.get()))
    {
        delete pArmy_data;
        delete pDefender;
        ret = HC_ERROR;
        return NULL;
    }
    if (!isBoss)
    {
        //���Թ��м������
        pArmy_data_a->LoadMazeCharactor(cid);
    }
    else
    {
        //���ص�ǰ�������
        pArmy_data_a->LoadCharactor(cid);
    }

    pAttacker->setCombat(pCombat);
    pDefender->setCombat(pCombat);

    //ս��˫��������Ϣ
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
    //��������
    army_data* pArmy_data = new army_data;
    Army* pDefender = new Army(pArmy_data);

    //������ - ��ҽ�ɫ
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

    //ս��˫��������Ϣ
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

