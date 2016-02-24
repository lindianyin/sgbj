
#include "stdafx.h"

//#include "combat.h"
#include "igeneral.h"
#include "army.h"
#include "combat_def.h"
#include "combat.h"
#include "general.h"
#include <iostream>
#include "data.h"
#include "utils_all.h"

#include "json_spirit.h"

#include "spls_errcode.h"
#include <boost/lexical_cast.hpp>

#include "boss.h"
#include "campRace.h"
#include "guard.h"
#include "eliteCombat.h"

#include "groupCopy.h"
#include "maze.h"
#include "singleton.h"
#include "corpsFighting.hpp"
#include "char_zst.h"

#define INFO(x)

using namespace json_spirit;

using namespace std;

volatile uint64_t Army::_refs = 0;

Army::Army(army_data* _data)
:_army_data(_data)
{
    ++Army::_refs;
    m_enermy = NULL;
    m_is_attacker = false;
    m_combat = NULL;
    m_action_pos = -1;
    _data->m_army = this;
}

Army::~Army()
{
    --Army::_refs;
}

uint64_t Army::refs()
{
    return Army::_refs;
}

//��ɫ���ӣ����佫���
iGeneral* Army::GetGeneral(int pos)
{
    CHECK_VALID_POS(pos, NULL);
    if (_army_data->m_generals[pos - 1] != NULL && _army_data->m_generals[pos - 1]->isLive())
    {
        return _army_data->m_generals[pos - 1];
    }
    return NULL;
}

//��ɫ���ӣ����佫���
iGeneral* Army::GetGeneral2(int pos)
{
    CHECK_VALID_POS(pos, NULL);
    if (_army_data->m_generals[pos - 1] != NULL)
    {
        return _army_data->m_generals[pos - 1];
    }
    return NULL;
}

//���ȱ��ţ�����û�д����һ�ſ�ʼ�����ȴ�ǰ����
iGeneral* Army::GetRowGeneral(int row)
{
    static int seqs[3][9] =
    {
        {1, 4, 7, 2, 5, 8, 3, 6, 9},
        {2, 5, 8, 1, 4, 7, 3, 6, 9},
        {3, 6, 9, 2, 5, 8, 1, 4, 7}
    };
    CHECK_VALID_ROW(row, NULL);
    for (size_t i = 0; i < 9; ++i)
    {
        iGeneral* g = GetGeneral(seqs[row][i]);
        if (NULL != g)
        {
            return g;
        }
    }
    return NULL;
}

//һ�ŵĵ�һ��Ŀ�꣬���ȴ�ǰ����
iGeneral* Army::GetRowGeneral2(int row)
{
    static int seqs[3][3] =
    {
        {1, 4, 7},
        {2, 5, 8},
        {3, 6, 9}
    };
    CHECK_VALID_ROW(row, NULL);
    for (size_t i = 0; i < 3; ++i)
    {
        iGeneral* g = GetGeneral(seqs[row][i]);
        if (NULL != g)
        {
            return g;
        }
    }
    return NULL;
}

//���ȱ��ţ�����û�д����һ�ſ�ʼ�����ȴӺ�ǰ
iGeneral* Army::GetRowGeneralr(int row)
{
    static int seqs[3][9] =
    {
        {7, 4, 1, 8, 5, 2, 9, 6, 3},
        {8, 5, 2, 7, 4, 1, 9, 6, 3},
        {9, 6, 3, 8, 5, 2, 7, 4, 1}
    };
    CHECK_VALID_ROW(row, NULL);
    for (size_t i = 0; i < 9; ++i)
    {
        iGeneral* g = GetGeneral(seqs[row][i]);
        if (NULL != g)
        {
            return g;
        }
    }
    return NULL;
}

//��ȡָ���佫������佫�����û�з����Լ�
iGeneral* Army::GetBackGeneralr(int pos)
{
    CHECK_VALID_POS(pos, NULL);
    if (IS_END_POS(pos))
    {
        return GetGeneral(pos);
    }
    else
    {
        int end_pos = POS_TO_ROW(pos) + 7;
        //��ָ��λ�ú��濪ʼ
        for (int i = pos  + 3; i <= end_pos; i += 3)
        {
            iGeneral* g = GetGeneral(i);
            if (NULL != g)
            {
                return g;
            }
        }
        return GetGeneral(pos);
    }
}

int Army::CheckPush(int pos,  std::list < iGeneral * > & glist)
{
    CHECK_VALID_POS(pos, -1);
    iGeneral* g = GetGeneral(pos);
    if (NULL != g)
    {
         glist.push_back(g);
    }
    return 0;
}

//pos����һ����佫
int Army::GetSideGenerals(int pos, std::list<iGeneral*>& glist)
{
    //cout<<"*******************GetSideGenerals,pos="<<pos<<endl;
    CheckPush(pos, glist);
    int side = POS_TO_SIDE(pos);
    switch (side)
    {
        case 0:
            //cout<<"*****************GetSideGenerals(1,2,3)*******************"<<endl;
            if (pos != 1)
            {
                CheckPush(1, glist);
            }
            if (pos != 2)
            {
                CheckPush(2, glist);
            }
            if (pos != 3)
            {
                CheckPush(3, glist);
            }
            break;
        case 1:
            //cout<<"*****************GetSideGenerals(4,5,6)*******************"<<endl;
            if (pos != 4)
            {
                CheckPush(4, glist);
            }
            if (pos != 5)
            {
                CheckPush(5, glist);
            }
            if (pos != 6)
            {
                CheckPush(6, glist);
            }
            break;
        case 2:
        default:
            //cout<<"*****************GetSideGenerals(7,8,9)*******************"<<endl;
            if (pos != 7)
            {
                CheckPush(7, glist);
            }
            if (pos != 8)
            {
                CheckPush(8, glist);
            }
            if (pos != 9)
            {
                CheckPush(9, glist);
            }
            break;
    }
    return 0;
}

//pos����һ�ŵ��佫
int Army::GetRowGenerals(int pos, std::list<iGeneral*>& glist)
{
    CheckPush(pos, glist);
    int row = POS_TO_ROW(pos);
    switch (row)
    {
        case 0:
            if (pos != 1)
            {
                CheckPush(1, glist);
            }
            if (pos != 4)
            {
                CheckPush(4, glist);
            }
            if (pos != 7)
            {
                CheckPush(7, glist);
            }
            break;
        case 1:
            if (pos != 2)
            {
                CheckPush(2, glist);
            }
            if (pos != 5)
            {
                CheckPush(5, glist);
            }
            if (pos != 8)
            {
                CheckPush(8, glist);
            }
            break;
        case 2:
        default:
            if (pos != 3)
            {
                CheckPush(3, glist);
            }
            if (pos != 6)
            {
                CheckPush(6, glist);
            }
            if (pos != 9)
            {
                CheckPush(9, glist);
            }
            break;
    }
    return 0;
}

//pos����̶������ĵ��佫
int Army::GetSomeGenerals(int pos, int counts, std::list<iGeneral*>& glist)
{
    CHECK_VALID_POS(pos, -1);
    int row = POS_TO_ROW(pos);
    static int seqs[3][9] =
    {
        {1, 4, 7, 2, 5, 8, 3, 6, 9},
        {2, 5, 8, 1, 4, 7, 3, 6, 9},
        {3, 6, 9, 2, 5, 8, 1, 4, 7}
    };
    int nums = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        iGeneral* g = GetGeneral(seqs[row][i]);
        if (NULL != g)
        {
            glist.push_back(g);
            ++nums;
            if (nums >= 3)
            {
                break;
            }
        }
    }
    return 0;
}

//pos�ڽ����佫
int Army::GetAroundGenerals(int pos, std::list<iGeneral*>& glist)
{
    CheckPush(pos, glist);
    switch (pos)
    {
        case 1:
            CheckPush(2, glist);
            CheckPush(4, glist);
            break;
        case 2:
            CheckPush(1, glist);
            CheckPush(3, glist);
            CheckPush(5, glist);
            break;
        case 3:
            CheckPush(2, glist);
            CheckPush(6, glist);
            break;
        case 4:
            CheckPush(1, glist);
            CheckPush(5, glist);
            CheckPush(7, glist);
            break;
        case 5:
            CheckPush(2, glist);
            CheckPush(4, glist);
            CheckPush(6, glist);
            CheckPush(8, glist);
            break;
        case 6:
            CheckPush(3, glist);
            CheckPush(5, glist);
            CheckPush(9, glist);
            break;
        case 7:
            CheckPush(4, glist);
            CheckPush(8, glist);
            break;
        case 8:
            CheckPush(5, glist);
            CheckPush(7, glist);
            CheckPush(9, glist);
            break;
        case 9:
            CheckPush(6, glist);
            CheckPush(8, glist);
            break;
    }
    return 0;
}

//ȫ���佫
int Army::GetAllGenerals(int pos, std::list<iGeneral*>& glist)
{
    CheckPush(pos, glist);
    for (size_t i = 1; i <= 9; ++i)
    {
        if (pos == (int)i)
        {
            continue;
        }
        iGeneral* g = GetGeneral(i);
        if (NULL != g)
        {
            glist.push_back(g);
        }
    }
    return 0;
}

//ȫ���佫
int Army::sendBuff()
{
    for (size_t i = 1; i <= 9; ++i)
    {
        iGeneral* g = GetGeneral(i);
        if (NULL != g)
        {
            g->sendBuff();
        }
    }
    return 0;
}

//ȫ���佫
int Army::getBuffChange(json_spirit::Array& blist)
{
    for (size_t i = 1; i <= 9; ++i)
    {
        iGeneral* g = GetGeneral(i);
        if (NULL != g)
        {
            g->getBuffChange(blist);
        }
    }
    return 0;
}

//ȫ���佫
void Army::updateBuff()
{
    for (size_t i = 1; i <= 9; ++i)
    {
        iGeneral* g = GetGeneral(i);
        if (NULL != g)
        {
            g->updateBuff();
        }
    }
}

//ȫ���佫
void Army::clearBuff()
{
    for (size_t i = 1; i <= 9; ++i)
    {
        iGeneral* g = GetGeneral(i);
        if (NULL != g)
        {
            g->clearBuff();
        }
    }
}

//�Ƿ����
bool Army::IsLive()
{
    for (size_t i = 1; i <= 9; ++i)
    {
        iGeneral* g = GetGeneral(i);
        if (NULL != g)
        {
            return true;
        }
    }
    return false;
}

//�ֵ��ж��������ж���pos
int Army::Action()
{
    while (m_action_pos < 9)
    {
        ++m_action_pos;
        iGeneral* g = GetGeneral2(m_action_pos);
        if (NULL != g)
        {
            bool actioned = false;
            if (g->isLive())
            {
                g->Action();
                m_combat->add_time(TIME_PER_ACTION);
                actioned = true;
            }
            g->updateGenerateBuff();
            sendBuff();
            m_enermy->sendBuff();
            if (actioned)
            {
                return m_action_pos;
            }
        }
    }
    return m_action_pos;
}

//����
int Army::Reset()
{
    m_action_pos = -1;
    //updateBuff();
    //sendBuff();
    return 0;
}

//Ѫ�����ٵ��佫
iGeneral* Army::GetMinhpGeneral()
{
    int hurt = 101;
    iGeneral* ret = NULL;
    for (size_t i = 1; i <= 9; ++i)
    {
        iGeneral* g = GetGeneral(i);
        if (NULL != g)
        {
            int ghurt = 100 * g->Hp()/g->MaxHp();
            if (ghurt < hurt)
            {
                ret = g;
                hurt = ghurt;
            }
        }
    }
    return ret;
}

//Ѫ�������佫
iGeneral* Army::GetMaxhpGeneral()
{
    iGeneral* ret = NULL;
    for (size_t i = 1; i <= 9; ++i)
    {
        iGeneral* g = GetGeneral(i);
        if (NULL != g && (NULL == ret || (g->Hp() < ret->Hp())))
        {
            ret = g;
        }
    }
    return ret;
}

Army* Army::GetEnermy()
{
    return m_enermy;
}

Combat& Army::GetCombat()
{
    return *m_combat;
}

void Army::setCombat(Combat* c)
{
    m_combat = c;
    if (_army_data)
    {
        _army_data->setArmy(this);
    }
}

Combat* Army::GetCombatHandle()
{
    return m_combat;
}

bool Army::NoAttack()
{
    for (size_t i = 1; i <= 9; ++i)
    {
        iGeneral* g = GetGeneral(i);
        if (NULL != g && !g->NoAttack())
        {
            return false;
        }
    }
    return true;
}

int Army::GetObj(json_spirit::Object& army_obj, bool brief/*=false*/)
{
    army_obj.clear();
    army_obj.push_back( Pair("name", _army_data->m_name) );
    if (_army_data->m_type == 0)
    {
        army_obj.push_back( Pair("id", _army_data->m_charactor) );
    }
    else
    {
        army_obj.push_back( Pair("id", 0) );
    }
    army_obj.push_back( Pair("level", _army_data->m_level) );
    army_obj.push_back( Pair("spic", _army_data->m_spic) );
    army_obj.push_back( Pair("type", _army_data->m_type+1) );
    json_spirit::Array garray;
    for (size_t i = 0; i < 9; ++i)
    {
        if (_army_data->m_generals[i])
        {
            garray.push_back(_army_data->m_generals[i]->GetOrgObj());
        }
    }
    army_obj.push_back( Pair("generals", garray) );
    if (!brief)
    {
        army_obj.push_back( Pair("shout", _army_data->m_shoutMsg) );
        army_obj.push_back( Pair("weapon", _army_data->m_weapons) );
        army_obj.push_back( Pair("state", _army_data->m_state) );
        army_obj.push_back( Pair("horse", _army_data->m_horse) );
        army_obj.push_back( Pair("buffs", _army_data->m_buff) );
        army_obj.push_back( Pair("jxl", _army_data->m_jxl) );
    }
    switch (m_combat->type())
    {
        case combat_stronghold:
        case combat_group_copy:
        case combat_elite:
            break;
        case combat_guard://�����ط��й�����Ϣ
            {
                if (!m_is_attacker)
                {
                    guardMgr::getInstance()->getInspire(_army_data->m_charactor,army_obj);
                }
            }
            break;
        default:            
            break;
    }
    return 0;
}

int Army::DieHp()
{
    return _army_data->m_hp_die;
}

int Army::TotalHp()
{
    return _army_data->m_hp_total;
}

int Army::TotalMaxHp()
{
    return _army_data->m_hp_max > 0 ? _army_data->m_hp_max : 1;
}

void Army::Calc()
{
    _army_data->m_hp_total = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (_army_data->m_generals[i])
        {
            _army_data->m_hp_total += _army_data->m_generals[i]->Hp();
        }
    }    
    _army_data->m_hp_die = _army_data->m_hp_max - _army_data->m_hp_total;
    _army_data->m_hp_cost = costSilver(_army_data->m_hp_die, _army_data->m_level);
    return;
}

//�������������
int Army::in_soul_link()
{
    for (size_t i = 1; i <= 9; ++i)
    {
        iGeneral* g = GetGeneral(i);
        if (NULL != g && g->isLive())
        {
            return g->getBuffValue(effect_soul_link);
        }
    }
    return 0;
}

//���ŵ��佫����
int Army::getLiveGeneralCount()
{
    int ret = 0;
    for (size_t i = 1; i <= 9; ++i)
    {
        iGeneral* g = GetGeneral(i);
        if (NULL != g)
        {
            ++ret;
        }
    }
    return ret;
}

volatile uint64_t army_data::_refs = 0;

army_data::army_data()
{
    ++army_data::_refs;
    for (size_t i = 0; i < 9; ++i)
    {
        m_generals[i] = NULL;
    }    
    m_charactor = 0;       //��ɫid��or ϵͳid
    m_type = 0;             //��ҽ�ɫ or ϵͳ��
    m_camp = 0;             //��Ӫ
    m_name = "";    //��ɫ����
    m_shoutMsg = "";//��������
    m_level = 0;
    m_spic = 0;
    m_army = NULL;
    
    m_hp_cost = 0;            //ļ������
    m_hp_total = 0;
    m_hp_die = 0;
    m_hp_max = 0;

    m_attack_value = 0;
}

army_data::~army_data()
{
    --army_data::_refs;
    for (size_t i = 0; i < 9; ++i)
    {
        iGeneral* g = m_generals[i];
        if (NULL != g)
        {
            m_generals[i] = NULL;
            delete g;
        }
    }
}

uint64_t army_data::refs()
{
    return army_data::_refs;
}

void army_data::setArmy(Army* pArmy)
{
    m_army = pArmy;
    for (int i = 0; i < 9; ++i)
    {
        if (m_generals[i])
        {
            m_generals[i]->setArmy(pArmy);
        }
    }
}

int army_data::LoadCharactor(int cid)     //���ؽ�ɫ����
{
    m_charactor = cid;
    //��ý�ɫ��Ϣ
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        ERR();
        return -1;
    }
    m_camp = cdata->m_camp;
    m_level = cdata->m_level;
    m_name = cdata->m_name;
    m_spic = cdata->m_spic;
    m_shoutMsg = cdata->m_chat;
    
    //ս������ǰ���´ι�����ƽʱװ��ж�²�ȥ����
    cdata->updateAttackDefense();

    m_weapons.clear();
    CharData* pc = cdata.get();
    //����        
    for (int i = 0; i < 5; ++i)
    {
        if (pc->m_new_weapons._weapons[i]._baseWeapon)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("name", pc->m_new_weapons._weapons[i]._baseWeapon->_name) );
            obj.push_back( Pair("quality", pc->m_new_weapons._weapons[i]._baseWeapon->_quality) );
            obj.push_back( Pair("type", i+1) );
            obj.push_back( Pair("addNums", pc->m_new_weapons._weapons[i]._effect) );
            obj.push_back( Pair("spic", pc->m_new_weapons._weapons[i]._baseWeapon->_id) );
            m_weapons.push_back(obj);
        }
    }

    //״̬
    m_state.clear();
    //cdata->m_newStates.getStateInfo(m_state);
    //ս��
    m_horse.clear();
    cdata->getHorse(m_horse);
    //��ʱ����Ч��
    m_buff.clear();
    cdata->m_Buffs.getBuffInfo(m_buff);
    //����¼�ӳ�
    cdata->m_jxl_buff.getInfo(m_jxl);

    //��ý�ɫ�佫����
    CharTotalGenerals& char_generals = cdata->GetGenerals();
    //��ɫ������Ϣ
    CharZhens& zhens = cdata->GetZhens();
    boost::shared_ptr<ZhenData> zdata = zhens.GetZhen(zhens.GetDefault());
    if (!zdata.get())
    {
        ERR();
        cout<<"default zhen "<<zhens.GetDefault()<<endl;
        return -1;
    }
    m_hp_max = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] != NULL)
        {
            delete m_generals[i];
            m_generals[i] = NULL;
        }
        if (zdata->m_generals[i] > 0)
        {
            boost::shared_ptr<CharGeneralData> sp = char_generals.GetGenral(zdata->m_generals[i]);
            if (sp.get())
            {
                const CharGeneralData& gdata = *sp.get();
                if (gdata.m_baseSoldier.get())
                {
                    m_generals[i] = new General(m_army, i + 1, gdata, 0);
                    if (m_generals[i])
                    {
                        m_hp_max += m_generals[i]->MaxHp();
                    }
                }
            }
        }
    }
    m_attack_value = pc->getAttack(0);
    return 0;
}

int army_data::LoadMazeCharactor(int cid)     //���ؽ�ɫ���󲿶�
{
    m_charactor = cid;
    //��ý�ɫ��Ϣ
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        ERR();
        return -1;
    }
    m_camp = cdata->m_camp;
    m_level = cdata->m_level;
    m_name = cdata->m_name;
    m_spic = cdata->m_spic;
    m_shoutMsg = cdata->m_chat;

    m_weapons.clear();
    CharData* pc = cdata.get();
    //����        
    for (int i = 0; i < 5; ++i)
    {
        if (pc->m_new_weapons._weapons[i]._baseWeapon)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("name", pc->m_new_weapons._weapons[i]._baseWeapon->_name) );
            obj.push_back( Pair("quality", pc->m_new_weapons._weapons[i]._baseWeapon->_quality) );
            obj.push_back( Pair("type", i+1) );
            obj.push_back( Pair("addNums", pc->m_new_weapons._weapons[i]._effect) );
            obj.push_back( Pair("spic", pc->m_new_weapons._weapons[i]._baseWeapon->_id) );
            m_weapons.push_back(obj);
        }
    }

    //״̬
    m_state.clear();
    //ս��
    m_horse.clear();
    cdata->getHorse(m_horse);
    //��ý�ɫ�佫����
    m_hp_max = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] != NULL)
        {
            delete m_generals[i];
            m_generals[i] = NULL;
        }
    }
    boost::shared_ptr<char_maze> cm = Singleton<mazeMgr>::Instance().getChar(cid);
    if (cm.get() && cm->m_curMaze.get())
    {
        char_maze* pcm = cm.get();
        for (std::list<char_maze_general>::iterator it = pcm->m_generals.begin(); it != pcm->m_generals.end(); ++it)    //����������佫
        {
            char_maze_general& g = *it;
            if (g.m_hp_org > g.m_hp_hurt)
            {
                m_generals[g.pos-1] = new General(m_army, g.pos, g);
                if (m_generals[g.pos-1])
                {
                    m_hp_max += m_generals[g.pos-1]->MaxHp();
                }
            }
        }
    }
    m_attack_value = pc->getAttack(0);
    return 0;
}

int army_data::LoadZSTCharactor(int cid)     //���ؽ�ɫ����
{
    m_charactor = cid;
    //��ý�ɫ��Ϣ
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        ERR();
        return -1;
    }
    m_camp = cdata->m_camp;
    m_level = cdata->m_level;
    m_name = cdata->m_name;
    m_spic = cdata->m_spic;
    m_shoutMsg = cdata->m_chat;

    m_weapons.clear();
    CharData* pc = cdata.get();
    //����        
    for (int i = 0; i < 5; ++i)
    {
        if (pc->m_new_weapons._weapons[i]._baseWeapon)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("name", pc->m_new_weapons._weapons[i]._baseWeapon->_name) );
            obj.push_back( Pair("quality", pc->m_new_weapons._weapons[i]._baseWeapon->_quality) );
            obj.push_back( Pair("type", i+1) );
            obj.push_back( Pair("addNums", pc->m_new_weapons._weapons[i]._effect) );
            obj.push_back( Pair("spic", pc->m_new_weapons._weapons[i]._baseWeapon->_id) );
            m_weapons.push_back(obj);
        }
    }

    //״̬
    m_state.clear();
    //ս��
    m_horse.clear();
    cdata->getHorse(m_horse);
    //��ý�ɫ�佫����
    m_hp_max = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] != NULL)
        {
            delete m_generals[i];
            m_generals[i] = NULL;
        }
    }
    boost::shared_ptr<char_zst> cz = Singleton<zstMgr>::Instance().getChar(cid);
    if (cz.get())
    {
        for (std::list<char_zst_general>::iterator it = cz->m_generals.begin(); it != cz->m_generals.end(); ++it)    //����������佫
        {
            char_zst_general& g = *it;
            if (g.m_hp_org > g.m_hp_hurt)
            {
                m_generals[g.pos-1] = new General(m_army, g.pos, g);
                if (m_generals[g.pos-1])
                {
                    m_hp_max += m_generals[g.pos-1]->MaxHp();
                }
            }
        }
    }
    m_attack_value = pc->getAttack(0);
    return 0;
}

//���عؿ�����
int army_data::LoadStronghold(CharStrongholdData& cstronghold, int id)
{
    m_type = 1;
    m_charactor = id;
    //�ؿ�����
    //��ùؿ��佫����
    boost::shared_ptr<StrongholdData> stronghold_data = GeneralDataMgr::getInstance()->GetStrongholdData(id);
    if (!stronghold_data.get())
    {
        ERR();
        return -1;
    }
    m_name = stronghold_data->m_name;
    m_shoutMsg = stronghold_data->m_chat;
    m_level = stronghold_data->m_level;
    m_spic = stronghold_data->m_spic;
    for (size_t i = 0; i < 9; ++i)
    {
        if (stronghold_data->m_generals[i].get())
        {
            const StrongholdGeneralData& gdata = *(stronghold_data->m_generals[i]);
            if (gdata.m_baseSoldier.get())
            {
                m_generals[i] = new General(m_army, i + 1, cstronghold, gdata);
                if (m_generals[i])
                {
                    m_hp_max += m_generals[i]->MaxHp();
                }
            }            
        }
    }
    //״̬
    cstronghold.getStates(m_state);
    return 0;
}

int army_data::LoadBoss(spls_boss& bs)     //����boss
{
    if (!m_generals[4])
    {
        INFO("############### load boss "<<bs._boss._id<<endl);
        m_type = 3;
        m_charactor = bs._boss._id;
        bossData& bd = bs._boss;
        m_generals[4] = new combatBoss(m_army, 5, bd);
        if (m_generals[4])
        {
            m_hp_max += m_generals[4]->MaxHp();
        }
        
        m_name = bd._name;
        m_shoutMsg = bd._chat;
        m_level = bd._level;
        m_spic = bd._spic;
        if (bd._state > 0)
        {
            boost::shared_ptr<baseState> bs = baseStateMgr::getInstance()->GetBaseState(bd._state);
            if (bs.get())
            {
                //ֻ��8��״̬�ǿɼ���
                if (bs->effect_type >= special_attack_baoji && bs->effect_type < special_attack_max)
                {
                    json_spirit::Object obj;
                    switch (bs->effect_type)
                    {
                        case special_attack_baoji:
                        case special_attack_shipo:
                        case special_attack_xixue:
                        case special_attack_chaos:
                        case special_attack_weihe:
                            obj.push_back( Pair("id", bs->effect_type + 1) );
                            obj.push_back( Pair("effect", LEX_CAST_STR(bs->effect_gailv/10) + "%") );
                            break;
                        case special_attack_dodge:
                            obj.push_back( Pair("id", 3) );
                            obj.push_back( Pair("effect", LEX_CAST_STR(bs->effect_gailv/10) + "%") );
                            break;
                        case special_attack_parry:
                            obj.push_back( Pair("id", 2) );
                            obj.push_back( Pair("effect", LEX_CAST_STR(bs->effect_gailv/10) + "%") );
                            break;
                        case special_attack_podan:
                            obj.push_back( Pair("id", bs->effect_type + 1) );
                            obj.push_back( Pair("effect", LEX_CAST_STR(bs->effect_value)) );
                            break;
                    }
                    obj.push_back( Pair("level", 20) );
                    m_state.push_back(obj);
                }
            }
        }
        INFO("############### load boss success"<<bs._boss._id<<endl);
    }
    else
    {
        m_generals[4]->GenOrgObj();
    }
    return HC_SUCCESS;
}

//������Ӫսһ������
int army_data::LoadCampRace(charCampRace* ccr)
{
    m_charactor = ccr->_cid;
    //��ý�ɫ��Ϣ
    CharData* cdata = ccr->_cdata.get();
    if (!cdata)
    {
        ERR();
        return -1;
    }
    m_camp = cdata->m_camp;
    m_level = cdata->m_level;
    m_name = cdata->m_name;
    m_spic = cdata->m_spic;
    m_shoutMsg = cdata->m_chat;
    
    //ս������ǰ���´ι�����ƽʱװ��ж�²�ȥ����
    cdata->updateAttackDefense();

    m_weapons.clear();
    //����        
    for (int i = 0; i < 5; ++i)
    {
        if (cdata->m_new_weapons._weapons[i]._baseWeapon)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("name", cdata->m_new_weapons._weapons[i]._baseWeapon->_name) );
            obj.push_back( Pair("quality", cdata->m_new_weapons._weapons[i]._baseWeapon->_quality) );
            obj.push_back( Pair("type", i+1) );
            obj.push_back( Pair("addNums", cdata->m_new_weapons._weapons[i]._effect) );
            obj.push_back( Pair("spic", cdata->m_new_weapons._weapons[i]._baseWeapon->_id) );
            m_weapons.push_back(obj);
        }
    }
    //״̬
    m_state.clear();
    //cdata->m_newStates.getStateInfo(m_state);
    //ս��
    m_horse.clear();
    cdata->getHorse(m_horse);

    m_hp_max = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] != NULL)
        {
            delete m_generals[i];
            m_generals[i] = NULL;
        }
        if (ccr->m_generals[i].get() && ccr->m_generals_hp[i] != -1)
        {
            const CharGeneralData& gdata = *(ccr->m_generals[i].get());
            if (gdata.m_baseSoldier.get())
            {
                m_generals[i] = new General(m_army, i + 1, gdata, ccr->m_generals_hp[i]);
                if (m_generals[i])
                {
                    m_hp_max += m_generals[i]->Hp();
                }
            }
        }
    }
    return 0;
}

//���ؾ���սսһ������
int army_data::LoadCorpsFighting(corpsFihtingMember* ccr)
{
    m_charactor = ccr->_cid;
    //��ý�ɫ��Ϣ
    CharData* cdata = ccr->_cdata.get();
    if (!cdata)
    {
        ERR();
        return -1;
    }
    m_camp = cdata->m_camp;
    m_level = cdata->m_level;
    m_name = cdata->m_name;
    m_spic = cdata->m_spic;
    m_shoutMsg = cdata->m_chat;
    
    //ս������ǰ���´ι�����ƽʱװ��ж�²�ȥ����
    cdata->updateAttackDefense();

    m_weapons.clear();
    //����        
    for (int i = 0; i < 5; ++i)
    {
        if (cdata->m_new_weapons._weapons[i]._baseWeapon)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("name", cdata->m_new_weapons._weapons[i]._baseWeapon->_name) );
            obj.push_back( Pair("quality", cdata->m_new_weapons._weapons[i]._baseWeapon->_quality) );
            obj.push_back( Pair("type", i+1) );
            obj.push_back( Pair("addNums", cdata->m_new_weapons._weapons[i]._effect) );
            obj.push_back( Pair("spic", cdata->m_new_weapons._weapons[i]._baseWeapon->_id) );
            m_weapons.push_back(obj);
        }
    }
    //״̬
    m_state.clear();
    //cdata->m_newStates.getStateInfo(m_state);
    //ս��
    m_horse.clear();
    cdata->getHorse(m_horse);

    m_hp_max = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        if (m_generals[i] != NULL)
        {
            delete m_generals[i];
            m_generals[i] = NULL;
        }
        if (ccr->m_generals[i].get() && ccr->m_generals_hp[i] != -1)
        {
            const CharGeneralData& gdata = *(ccr->m_generals[i].get());
            if (gdata.m_baseSoldier.get())
            {
                m_generals[i] = new General(m_army, i + 1, gdata, ccr->m_generals_hp[i]);
                if (m_generals[i])
                {
                    m_hp_max += m_generals[i]->Hp();
                }
            }
        }
    }
    return 0;
}

int army_data::loadGroupCopy(int copyId, int pos, groupCopyArmy* pgroupArmy)
{
    if (!pgroupArmy)
    {
        ERR();
        return -1;
    }
    m_type = 1;
    m_charactor = copyId * 1000 + pos;
    
    m_name = pgroupArmy->m_name;
    m_shoutMsg = "";
    m_level = pgroupArmy->m_level;
    m_spic = pgroupArmy->m_spic;

    CharStrongholdData cstronghold(0,0,0,0);
    for (size_t i = 0; i < 9; ++i)
    {
        if (pgroupArmy->m_generals[i].get())
        {
            const StrongholdGeneralData& gdata = *(pgroupArmy->m_generals[i]);
            cstronghold.m_combat_attribute = pgroupArmy->m_combat_attribute;
            if (gdata.m_baseSoldier.get())
            {
                m_generals[i] = new General(m_army, i + 1, cstronghold, gdata);
                if (m_generals[i])
                {
                    m_hp_max += m_generals[i]->MaxHp();
                }
            }            
        }
    }
    return 0;
}

int army_data::loadEliteCombat(int id, eliteCombat* peliteCombat)
{
    m_type = 1;
    m_charactor = id;
    
    m_name = peliteCombat->_name;
    m_shoutMsg = "";
    m_level = peliteCombat->_level;
    m_spic = 0;

    CharStrongholdData cstronghold(0,0,0,0);
    for (size_t i = 0; i < 9; ++i)
    {
        if (peliteCombat->m_generals[i].get())
        {
            const StrongholdGeneralData& gdata = *(peliteCombat->m_generals[i]);
            if (m_spic == 0 && gdata.m_special)
            {
                m_spic = gdata.m_spic;
            }
            cstronghold.m_combat_attribute = peliteCombat->m_combat_attribute;
            if (gdata.m_baseSoldier.get())
            {
                m_generals[i] = new General(m_army, i + 1, cstronghold, gdata);
                if (m_generals[i])
                {
                    m_hp_max += m_generals[i]->MaxHp();
                }
            }
        }
    }
    return 0;
}

int army_data::loadMazeCombat(int id, mazeMonster* pm)
{
    m_type = 1;
    m_charactor = id;
    
    m_name = pm->_name;
    m_shoutMsg = "";
    m_level = pm->_level;
    m_spic = 0;

    CharStrongholdData cstronghold(0,0,0,0);
    for (size_t i = 0; i < 9; ++i)
    {
        if (pm->m_generals[i].get())
        {
            const StrongholdGeneralData& gdata = *(pm->m_generals[i]);
            if (m_spic == 0 && gdata.m_special)
            {
                m_spic = gdata.m_spic;
            }
            cstronghold.m_combat_attribute = pm->m_combat_attribute;
            if (gdata.m_baseSoldier.get())
            {
                m_generals[i] = new General(m_army, i + 1, cstronghold, gdata);
                if (m_generals[i])
                {
                    m_hp_max += m_generals[i]->MaxHp();
                }
            }
        }
    }
    return 0;
}

//���عؿ�����
int army_data::loadZSTCombat(int id, base_ZST_Stronghold* pzst)
{
    m_type = 1;
    m_charactor = id;
    
    m_name = pzst->_name;
    m_shoutMsg = "";
    m_level = pzst->_level;
    m_spic = pzst->_spic;

    CharStrongholdData cstronghold(0,0,0,0);
    for (size_t i = 0; i < 9; ++i)
    {
        if (pzst->m_generals[i].get())
        {
            const StrongholdGeneralData& gdata = *(pzst->m_generals[i]);
            if (m_spic == 0 && gdata.m_special)
            {
                m_spic = gdata.m_spic;
            }
            cstronghold.m_combat_attribute = pzst->m_combat_attribute;
            if (gdata.m_baseSoldier.get())
            {
                m_generals[i] = new General(m_army, i + 1, cstronghold, gdata);
                if (m_generals[i])
                {
                    m_hp_max += m_generals[i]->MaxHp();
                }
            }
        }
    }
    return 0;
}

void army_data::_update()
{
    for (int i = 0; i < 9; ++i)
    {
        if (m_generals[i])
        {
            m_generals[i]->GenOrgObj();
        }
    }
}
