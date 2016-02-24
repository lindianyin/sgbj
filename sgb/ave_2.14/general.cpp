
#include "stdafx.h"

#include "general.h"
#include "utils_all.h"
#include "json_spirit.h"
#include "army.h"
#include "combat_def.h"
#include "combat.h"
#include "data.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "singleton.h"

#include "boss.h"
#include "maze.h"
#include "char_zst.h"
#include "char_general_soul.hpp"

using namespace boost;

#include "utils_lang.h"

#define INFO(x)// cout<<x

using namespace std;

//#define TEST_FIGHT_RANDOM

//季节不同，物理伤害/策略伤害的加成
int g_season_damage_face[2] = {0,0};

//根据季节确定伤害减免
int GetGlobalFac(int type)
{
    switch (type)
    {
        case celue_damage:
            return g_season_damage_face[1];
        case wuli_damage:
            return g_season_damage_face[0];
        default:
            return 0;
    }
}

//根据士气确定伤害加成
int getShiqiMoreDamage(int shiqi)
{
    if (shiqi >= 200)
    {
        return 50;
    }
    else if (shiqi >= 150)
    {
        return 30;
    }
    else if (shiqi >= 100)
    {
        return 20;
    }
    else if (shiqi >= 50)
    {
        return 10;
    }
    else if (shiqi >= 1)
    {
        return 5;
    }
    else
    {
        return 0;
    }
}

//根据士气确定伤害减少
int getShiqiSubDamage(int shiqi)
{
    if (shiqi >= 200)
    {
        return 30;
    }
    else if (shiqi >= 150)
    {
        return 15;
    }
    else if (shiqi >= 100)
    {
        return 10;
    }
    else if (shiqi >= 50)
    {
        return 5;
    }
    else
    {
        return 0;
    }
}

/*     春天物理攻击结算时增加15%伤害；
    夏天物理攻击结算时减少15%伤害；
    秋天策略攻击结算时增加15%伤害；
    冬天策略攻击结算时减少15%伤害 */
void updateGlobalFac(int season)
{
    switch (season)
    {
        case 1:
            g_season_damage_face[0] = 15;
            g_season_damage_face[1] = 0;
            break;
        case 2:
            g_season_damage_face[0] = -15;
            g_season_damage_face[1] = 0;
            break;
        case 3:
            g_season_damage_face[0] = 0;
            g_season_damage_face[1] = 15;
            break;
        case 4:
            g_season_damage_face[0] = 0;
            g_season_damage_face[1] = -15;
            break;
    }
}

/*

7 4 1        1 4 7
8 5 2   <->  2 5 8
9 6 3        3 6 9

//玩家武将数据
struct CharGeneralData
{
    int m_cid;      //角色id
    int m_gid;      //武将id
    int m_level;    //等级
    int m_str;      //力量
    int m_int;      //智力
    int m_tongyu;   //统御
    double m_chengzhang;    //成长
    int m_hp_cost;  //消耗兵力需要的银子
    int Save();
};

*/

General::General(Army* army, int pos, const CharGeneralData& gdata, int hurted)
:m_army(army)
{
    m_nuqi_add = iAttackSuccessShiqi;
    m_nuqi_add_baoji = 2*m_nuqi_add;
    m_buff_changed = false;
    m_special_attack = 0;
    //血量
    m_hp_org = 100; //原始血量
    m_hp_now = 100; //当前血量

    m_unique_id = gdata.m_id;
    b_init_success = false;
    m_id = gdata.m_gid;         //武将id
    m_spic = gdata.m_spic;      //头像
    m_charactor = gdata.m_cid;  //所属角色id

    m_level = gdata.m_level;    //等级
    m_color = gdata.m_color;

    //勇武、智力、统御
    m_str = gdata.m_str;
    m_int = gdata.m_int;
    m_tongyu = gdata.m_tongyu;
    if (gdata.m_baowu_type != 0)
    {
        switch(gdata.m_baowu_type)
        {
            case 1:
                m_str += gdata.m_baowu_add;
                break;
            case 2:
                m_int += gdata.m_baowu_add;
                break;
            case 3:
                m_tongyu += gdata.m_baowu_add;
                break;
            default:
                break;
        }
    }
    //角色数据
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_charactor);
    if (!cdata.get())
    {
        return;
    }
    //战斗属性继承自主将
    //cdata->m_combat_attribute.print(true);
    m_combat_attribute = cdata->m_combat_attribute;
    //cout<<"cdata.boss_inspired="<<cdata->m_combat_attribute.m_boss_inspired<<","<<m_id<<"__general.boss_inspired="<<m_combat_attribute.m_boss_inspired<<endl;

    //获得基础武将数据
    boost::shared_ptr<GeneralTypeData> bg = GeneralDataMgr::getInstance()->GetBaseGeneral(m_id);
    if (!bg.get())
    {
        return;
    }
    //兵种
    m_stype = bg->m_stype;
    //获得基础兵种数据
    boost::shared_ptr<BaseSoldierData> bs = GeneralDataMgr::getInstance()->GetBaseSoldier(m_stype);
    if (!bs.get())
    {
        return;
    }
    //战斗属性加上兵种自带的属性
    m_combat_attribute += bs->m_combat_attribute;

    //cout<<"1 -----"<<gdata.m_cid<<"."<<gdata.m_gid<<endl;
    //m_combat_attribute.print(true);

    //cout<<"2 -----"<<endl;
    //gdata.m_combat_attr.print(true);

    //战斗属性
     m_combat_attribute += gdata.m_combat_attr;

    //获得练兵数据
    boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(*(cdata.get()));
    if (ct.get())
    {
        //战斗属性加上兵魂属性
        m_combat_attribute += ct->_combatAttr;
    }

    m_base_stype = bs->m_base_type;
    m_damage_type = bs->m_damage_type;    // 伤害类型
    m_damage_type2 = bs->m_damage_type2;  // 伤害类型
    m_attack_type = bs->m_attack_type;    // 攻击方式
    m_attack_type2 = bs->m_attack_type2;  // 攻击方式 近战 | 远程
    m_attack_type3 = bs->m_attack_type3;  // 攻击方式 近战 | 远程
    m_real_attack_type = m_attack_type;
    m_special_attack_fac = bs->m_special_attack_fac;
    
    m_soldier_spic = bs->m_spic;        //兵种头像
    m_sname = bs->m_name;               //兵种名

    //攻防根据战斗类型是否加载战马属性
    bool load_horse = true;
    #if 0
    if (army && army->GetCombatHandle())
    {
        switch (army->GetCombatHandle()->type())
        {
            case combat_stronghold:
            case combat_group_copy:
            case combat_elite:
                break;
            default:
                load_horse = true;
                break;
        }
    }
    #endif
    if (act_wuli_attack == m_damage_type)
    {
        m_attack = 2 * m_str + cdata->getPugong(load_horse) + m_combat_attribute.skill_add(1) + gdata.m_attack + m_combat_attribute.soul_add_attack(m_base_stype);
    }
    else
    {
        m_attack = 2 * m_int + cdata->getCegong(load_horse) + m_combat_attribute.skill_add(3) + gdata.m_attack + m_combat_attribute.soul_add_attack(m_base_stype);
    }
    m_wu_fang = 7 * m_str / 5 + cdata->getPufang(load_horse) + m_combat_attribute.skill_add(2) + gdata.m_pufang + m_combat_attribute.soul_add_wufang(m_base_stype);
    m_ce_fang = 7 * m_int / 5 + cdata->getCefang(load_horse) + m_combat_attribute.skill_add(4) + gdata.m_cefang + m_combat_attribute.soul_add_cefang(m_base_stype);

    m_failure = 0;  //失败概率
    
    /****血量 *******************************************/
    m_hp_org = 3*m_tongyu;   //原始血量
    if (m_hp_org <= 0)
    {
        m_hp_org = 1;
    }
    
    //兵器装备加成血量
    m_hp_org += (cdata->getBingli(load_horse));
    //技能加血
    m_hp_org += m_combat_attribute.skill_add(0);
    m_hp_org += gdata.m_hp;
    m_hp_org += m_combat_attribute.soul_add_hp(m_base_stype);
    //成长星级加成
    if (gdata.m_chengzhang_star.get())
    {
        m_attack += gdata.m_chengzhang_star->gongji;
        m_wu_fang += gdata.m_chengzhang_star->fangyu;
        m_ce_fang += gdata.m_chengzhang_star->fangyu;
        m_hp_org += gdata.m_chengzhang_star->bingli;
    }

    //将魂加成
    if (gdata.m_general_soul)
    {
        m_attack += gdata.m_general_soul->getAttack(gdata.m_color);
        m_wu_fang += gdata.m_general_soul->getWufang(gdata.m_color);
        m_ce_fang += gdata.m_general_soul->getCefang(gdata.m_color);
        m_hp_org += gdata.m_general_soul->getBingli(gdata.m_color);
    }

    //兵魂加成打印
    /*{
        cout << "***********load_general,stype=" << m_base_stype << "***********" << endl;
        cout << "add_attack" << m_combat_attribute.soul_add_attack(m_base_stype);
        cout << " add_wufang" << m_combat_attribute.soul_add_wufang(m_base_stype);
        cout << " add_cefang" << m_combat_attribute.soul_add_cefang(m_base_stype);
        cout << " add_hpfang" << m_combat_attribute.soul_add_hp(m_base_stype);
        cout << " add_moreDamage" << m_combat_attribute.soul_stypeDamage(m_base_stype) << endl;
    }*/

    //武将天赋
    if (gdata.m_baseGeneral.get())
    {
        m_combat_attribute += gdata.m_baseGeneral->m_new_tianfu.m_combatAttr;
        if (gdata.m_baseGeneral->m_new_tianfu.m_more_hp)
        {
            m_hp_org = (100 + gdata.m_baseGeneral->m_new_tianfu.m_more_hp) * m_hp_org / 100;
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
                hp_buff = m_hp_org * (cdata->m_Buffs.buffs[i].m_value) / 100;
                break;
            case 2:
                if (act_wuli_attack == m_damage_type)
                {
                    attack_buff = m_attack * (cdata->m_Buffs.buffs[i].m_value) / 100;
                }
                break;
            case 3:
                wu_fang_buff = m_wu_fang * (cdata->m_Buffs.buffs[i].m_value) / 100;
                break;
            case 4:
                if (act_wuli_attack != m_damage_type)
                {
                    attack_buff = m_attack * (cdata->m_Buffs.buffs[i].m_value) / 100;
                }
                break;
            case 5:
                ce_fang_buff = m_ce_fang * (cdata->m_Buffs.buffs[i].m_value) / 100;
                break;
            default:
                break;
        }
    }

    //将星录加成
    int hp_jxl = 0, cefang_jxl = 0, wufang_jxl = 0, attack_jxl = 0;
    cdata->m_jxl_buff.total_buff_attr.get_add(m_attack, m_hp_org, m_wu_fang, m_ce_fang, attack_jxl, hp_jxl, wufang_jxl, cefang_jxl);

    //皇座称号加成
    int hp_throne = 0, attack_throne = 0, wu_fang_throne = 0, ce_fang_throne = 0;
    int throne_per = 0;
    if (cdata->m_nick.check_nick(nick_throne_start))
    {
        throne_per = 8;
    }
    else if(cdata->m_nick.check_nick(nick_throne_start + 1))
    {
        throne_per = 5;
    }
    else if(cdata->m_nick.check_nick(nick_throne_start + 2))
    {
        throne_per = 3;
    }
    hp_throne = m_hp_org * throne_per / 100;
    attack_throne = m_attack * throne_per / 100;
    wu_fang_throne = m_wu_fang * throne_per / 100;
    ce_fang_throne = m_ce_fang * throne_per / 100;

    //cdata->m_jxl_buff.total_buff_attr.dump();
    //cout<<"general jxl:"<<hp_jxl<<","<<attack_jxl<<","<<cefang_jxl<<","<<wufang_jxl<<endl;
    m_hp_org += (hp_buff + hp_jxl + hp_throne);
    m_attack += (attack_buff + attack_jxl + attack_throne);
    m_wu_fang += (wu_fang_buff + wufang_jxl + wu_fang_throne);
    m_ce_fang += (ce_fang_buff + cefang_jxl + ce_fang_throne);

    cdata->m_jxl_buff.total_buff_attr.add_special(m_combat_attribute);

    //阵营战鼓舞，加血量 - 血量和攻击的比例是3:2
    if (m_army && m_army->GetCombatHandle()
        && m_army->GetCombatHandle()->type() == combat_camp_race
        && cdata->m_combat_attribute.camp_inspired() > 0)
    {
        m_hp_org = (100 + 3*cdata->m_combat_attribute.camp_inspired()/2) * m_hp_org / 100;
    }
    //迷宫鼓舞，影响血量，普防，策防和最终伤害
    else if (m_army && m_army->GetCombatHandle()
        && m_army->GetCombatHandle()->type() == combat_maze_boss
        && cdata->m_combat_attribute.maze_inspired() != 0)
    {
        //cdata->m_combat_attribute.total_inspired(cdata->m_combat_attribute.maze_inspired());
        //cout<<"maze boss general(), org hp "<<m_hp_org;
        m_hp_org = (100 + cdata->m_combat_attribute.maze_inspired()) * m_hp_org / 100;
        //cout<<"->"<<m_hp_org<<",inspired "<<cdata->m_combat_attribute.maze_inspired()<<endl;
        m_attack = (100 + cdata->m_combat_attribute.maze_inspired()) * m_attack / 100;
        m_wu_fang = (100 + cdata->m_combat_attribute.maze_inspired()) * m_wu_fang / 100;
        m_ce_fang = (100 + cdata->m_combat_attribute.maze_inspired()) * m_ce_fang / 100;
    }
    /****************************************************/

    m_hp_now = m_hp_org;   //当前血量
    m_hp_now -= hurted;
    if (m_hp_now < 0)
    {
        m_hp_now = 1;
    }

    //士气
    m_shiqi = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();    //受伤加的士气
    m_shiqi_old = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();
    m_pos = pos;      //9宫图中位置1-9
    m_name = bg->m_name;    //武将名
    m_nickname = "";
    if (gdata.b_nickname)
    {
        m_nickname = bg->m_nickname;
    }
    //特殊状态
    m_chaos_flag = 0;   //混乱标记
    m_weihe_flag = 0;   //威吓标记
    m_baoji_flag = 0;   //暴击标记

    if (m_army)
    {
        m_type = m_army->isAttacker() ? 1 : 2;
    }
    else
    {
        m_type = 0;
    }

    //战斗技能
    m_speSkill = gdata.m_baseGeneral->m_speSkill;

    b_init_success = true; //初始化成功
    GenOrgObj();
}

General::General(Army* army, int pos, const char_maze_general& m_gdata)
:m_army(army)
{
    m_nuqi_add = iAttackSuccessShiqi;
    m_nuqi_add_baoji = 2*m_nuqi_add;
    m_buff_changed = false;
    m_special_attack = 0;
    //血量
    m_hp_org = 100; //原始血量
    m_hp_now = 100; //当前血量

    m_unique_id = m_gdata.id;
    b_init_success = false;
    m_id = m_gdata.gid;         //武将id
    m_spic = m_gdata.spic;      //头像
    m_charactor = m_gdata.cid;  //所属角色id

    m_level = m_gdata.level;    //等级
    m_color = m_gdata.color;

    //勇武、智力、统御(天赋加成)
    m_str = m_gdata.m_str;
    m_int = m_gdata.m_int;
    m_tongyu = m_gdata.m_tongyu;
    //战斗属性(角色兵种武将天赋)
     m_combat_attribute += m_gdata.m_combat_attribute;
    //角色数据
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_charactor);
    if (!cdata.get())
    {
        return;
    }

    //获得基础武将数据
    boost::shared_ptr<GeneralTypeData> bg = GeneralDataMgr::getInstance()->GetBaseGeneral(m_id);
    if (!bg.get())
    {
        return;
    }
    //兵种
    m_stype = bg->m_stype;
    //获得基础兵种数据
    boost::shared_ptr<BaseSoldierData> bs = GeneralDataMgr::getInstance()->GetBaseSoldier(m_stype);
    if (!bs.get())
    {
        return;
    }

    m_base_stype = bs->m_base_type;
    m_damage_type = bs->m_damage_type;    // 伤害类型
    m_damage_type2 = bs->m_damage_type2;  // 伤害类型
    m_attack_type = bs->m_attack_type;    // 攻击方式
    m_attack_type2 = bs->m_attack_type2;  // 攻击方式 近战 | 远程
    m_attack_type3 = bs->m_attack_type3;  // 攻击方式 近战 | 远程
    m_real_attack_type = m_attack_type;
    m_special_attack_fac = bs->m_special_attack_fac;
    
    m_soldier_spic = bs->m_spic;        //兵种头像
    m_sname = bs->m_name;               //兵种名

    //攻防
    m_attack = m_gdata.m_attack;
    m_wu_fang = m_gdata.m_wu_fang;
    m_ce_fang = m_gdata.m_ce_fang;
    m_failure = 0;  //失败概率
    
    //血量 
    m_hp_org = m_gdata.m_hp_org;   //原始血量
    m_hp_now = m_gdata.m_hp_org - m_gdata.m_hp_hurt;   //当前血量
    if (m_hp_now < 0)
    {
        m_hp_now = 1;
    }

    //士气
    m_shiqi = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();    //受伤加的士气
    m_shiqi_old = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();
    m_pos = pos;      //9宫图中位置1-9
    m_name = bg->m_name;    //武将名
    m_nickname = "";
    if (m_gdata.b_nickname)
    {
        m_nickname = bg->m_nickname;
    }
    //特殊状态
    m_chaos_flag = 0;   //混乱标记
    m_weihe_flag = 0;   //威吓标记
    m_baoji_flag = 0;   //暴击标记

    if (m_army)
    {
        m_type = m_army->isAttacker() ? 1 : 2;
    }
    else
    {
        m_type = 0;
    }

    //buff的效果，狂暴，虚弱
    /*int inspired = m_combat_attribute.maze_inspired();
    if (inspired != 0)
    {
        cout<<"maze general().org hp "<<m_hp_org<<"->";
        m_combat_attribute.total_inspired(inspired);
        m_hp_org = m_hp_org * (100+inspired)/100;
        cout<<m_hp_org<<",inspired "<<inspired<<endl;
        m_hp_now = m_hp_now * (100+inspired)/100;
        m_wu_fang = m_wu_fang * (100+inspired)/100;
        m_ce_fang = m_ce_fang * (100+inspired)/100;
        if (m_hp_org < 1)
        {
            m_hp_org = 1;
        }
        if (m_hp_now < 0)
        {
            m_hp_now = 1;
        }
    }*/
    //战斗技能
    m_speSkill = bg->m_speSkill;

    b_init_success = true; //初始化成功
    GenOrgObj();
}

General::General(Army* army, int pos, const char_zst_general& m_gdata)
:m_army(army)
{
    m_nuqi_add = iAttackSuccessShiqi;
    m_nuqi_add_baoji = 2*m_nuqi_add;
    m_buff_changed = false;
    m_special_attack = 0;
    //血量
    m_hp_org = 100; //原始血量
    m_hp_now = 100; //当前血量

    m_unique_id = m_gdata.id;
    b_init_success = false;
    m_id = m_gdata.gid;         //武将id
    m_spic = m_gdata.spic;      //头像
    m_charactor = m_gdata.cid;  //所属角色id

    m_level = m_gdata.level;    //等级
    m_color = m_gdata.color;

    //勇武、智力、统御(天赋加成)
    m_str = m_gdata.m_str;
    m_int = m_gdata.m_int;
    m_tongyu = m_gdata.m_tongyu;
    //战斗属性(角色兵种武将天赋)
     m_combat_attribute += m_gdata.m_combat_attribute;
    //角色数据
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_charactor);
    if (!cdata.get())
    {
        return;
    }

    //获得基础武将数据
    boost::shared_ptr<GeneralTypeData> bg = GeneralDataMgr::getInstance()->GetBaseGeneral(m_id);
    if (!bg.get())
    {
        return;
    }
    //兵种
    m_stype = bg->m_stype;
    //获得基础兵种数据
    boost::shared_ptr<BaseSoldierData> bs = GeneralDataMgr::getInstance()->GetBaseSoldier(m_stype);
    if (!bs.get())
    {
        return;
    }

    m_base_stype = bs->m_base_type;
    m_damage_type = bs->m_damage_type;    // 伤害类型
    m_damage_type2 = bs->m_damage_type2;  // 伤害类型
    m_attack_type = bs->m_attack_type;    // 攻击方式
    m_attack_type2 = bs->m_attack_type2;  // 攻击方式 近战 | 远程
    m_attack_type3 = bs->m_attack_type3;  // 攻击方式 近战 | 远程
    m_real_attack_type = m_attack_type;
    m_special_attack_fac = bs->m_special_attack_fac;
    
    m_soldier_spic = bs->m_spic;        //兵种头像
    m_sname = bs->m_name;               //兵种名

    //攻防
    m_attack = m_gdata.m_attack;
    m_wu_fang = m_gdata.m_wu_fang;
    m_ce_fang = m_gdata.m_ce_fang;
    m_failure = 0;  //失败概率
    
    //血量 
    m_hp_org = m_gdata.m_hp_org;   //原始血量
    m_hp_now = m_gdata.m_hp_org - m_gdata.m_hp_hurt;   //当前血量
    if (m_hp_now < 0)
    {
        m_hp_now = 1;
    }

    //士气
    m_shiqi = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();    //受伤加的士气
    m_shiqi_old = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();
    m_pos = pos;      //9宫图中位置1-9
    m_name = bg->m_name;    //武将名
    m_nickname = "";
    if (m_gdata.b_nickname)
    {
        m_nickname = bg->m_nickname;
    }
    //特殊状态
    m_chaos_flag = 0;   //混乱标记
    m_weihe_flag = 0;   //威吓标记
    m_baoji_flag = 0;   //暴击标记

    if (m_army)
    {
        m_type = m_army->isAttacker() ? 1 : 2;
    }
    else
    {
        m_type = 0;
    }

    //战斗技能
    m_speSkill = bg->m_speSkill;

    b_init_success = true; //初始化成功
    GenOrgObj();
}

General::General(Army* army, int pos, const CharStrongholdData& cstronghold, const StrongholdGeneralData& gdata)
:m_army(army)
{
    m_nuqi_add = iAttackSuccessShiqi;
    m_nuqi_add_baoji = 2*m_nuqi_add;

    m_buff_changed = false;
    m_special_attack = 0;
    //血量
    m_hp_org = 100; //原始血量
    m_hp_now = 100; //当前血量

    b_init_success = false;
    m_unique_id = 0;
    m_id = 0;           //武将id
    m_spic = gdata.m_spic;           //武将id
    m_charactor = 0;    //所属角色id

    m_level = gdata.m_level;//关卡等级就是关卡兵的等级
    m_color = gdata.m_color;
    
    //后面这里加上根据角色id和关卡id来获得守将的三个状态
    m_combat_attribute = cstronghold.m_combat_attribute;
    m_combat_attribute.total_inspired(0);
    //******************************************

    //加上关卡的基础看行
    if (cstronghold.m_baseStronghold.get())
    {
        m_combat_attribute += cstronghold.m_baseStronghold->m_combat_attribute;
    }

    //勇武、智力、统御
    m_str = gdata.m_str;
    m_int = gdata.m_int;
    m_tongyu = 0;
    //兵种
    m_stype = gdata.m_stype;
    //获得基础兵种数据
    boost::shared_ptr<BaseSoldierData> bs = GeneralDataMgr::getInstance()->GetBaseSoldier(m_stype);
    if (!bs.get())
    {
        ERR();
        return;
    }
    //加上兵种自身的战斗属性
    m_combat_attribute += bs->m_combat_attribute;

    m_base_stype = bs->m_base_type;
    m_damage_type = bs->m_damage_type;  // 0 物理攻击 1策略攻击
    m_damage_type2 = bs->m_damage_type2;  // 伤害类型
    m_attack_type = bs->m_attack_type;    // 攻击方式 一排，一列等
    m_attack_type2 = bs->m_attack_type2;  // 攻击方式 近战 | 远程
    m_attack_type3 = bs->m_attack_type3;  // 攻击方式 近战 | 远程

    m_real_attack_type = m_attack_type;    
    m_soldier_spic = bs->m_spic;        //兵种头像
    m_sname = bs->m_name;               //兵种名
    m_special_attack_fac = bs->m_special_attack_fac;

    //攻防
    m_attack = gdata.m_attack;

    m_wu_fang = gdata.m_pufang;
    m_ce_fang = gdata.m_cefang;

    m_failure = 0;  //失败概率

    //血量
    m_hp_org = gdata.m_hp;  //原始血量
    m_hp_now = m_hp_org;    //当前血量

    //士气
    m_shiqi = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();    //受伤加的士气
    m_shiqi_old = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();

    m_pos = pos;    //9宫图中位置1-9
    m_name = gdata.m_name;    //武将名
    m_nickname = "";

    //特殊状态
    m_chaos_flag = 0;   //混乱标记
    m_weihe_flag = 0;   //威吓标记
    m_baoji_flag = 0;   //暴击标记

    if (m_army)
    {
        m_type = m_army->isAttacker() ? 1 : 2;
    }
    else
    {
        m_type = 0;
    }
    //战斗技能
    m_speSkill = gdata.m_speSkill;

    b_init_success = true; //初始化成功

    GenOrgObj();
}

General::General(Army* army, int pos, const bossData& bd)
:m_army(army)
{
    //boss每次加10点怒气，5回合后出大招
    m_nuqi_add = 20;
    m_nuqi_add_baoji = 20;

    m_buff_changed = false;
    m_special_attack = 0;
    //血量
    m_hp_org = 100; //原始血量
    m_hp_now = 100; //当前血量

    b_init_success = false;
    m_unique_id = 0;
    m_id = bd._type;
    m_spic = bd._spic;
    m_soldier_spic = bd._soldier_spic;
    m_charactor = 0;

    m_real_attack_type = 0;

    m_level = bd._level;
    m_color = 0;

    //后面这里加上根据角色id和关卡id来获得守将的三个状态
    //m_combat_attribute = cstronghold.m_combat_attribute;
    //******************************************

    //勇武、智力、统御
    m_str = bd._str;
    m_int = bd._wisdom;
    m_tongyu = 0;
    //兵种
    m_stype = 0;
    m_base_stype = bd._stype;
    m_sname = bd._name;

    //加上兵种自身的战斗属性
    m_combat_attribute += bd.m_combat_attribute;
    m_combat_attribute.total_inspired(0);

    //m_base_stype = bs->m_base_type;
    //m_damage_type = bs->m_damage_type;  // 0 物理攻击 1策略攻击
    //m_attack_type = bs->m_attack_type;  // 攻击方式
    //m_soldier_spic = bs->m_spic;        //兵种头像
    //m_sname = bs->m_name;               //兵种名
    m_special_attack_fac = 220;

    //攻防
    m_attack = bd._damage1;

    m_wu_fang = bd._pufang;
    m_ce_fang = bd._cefang;

    m_failure = 0;  //失败概率

    //血量
    m_hp_org = bd._max_hp;  //原始血量
    m_hp_now = bd._cur_hp;  //当前血量

    //士气
    m_shiqi = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();    //受伤加的士气
    m_shiqi_old = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();

    m_pos = pos;    //9宫图中位置1-9
    m_name = bd._name;    //武将名
    m_nickname = "";

    //特殊状态
    m_chaos_flag = 0;   //混乱标记
    m_weihe_flag = 0;   //威吓标记
    m_baoji_flag = 0;   //暴击标记

    if (m_army)
    {
        m_type = m_army->isAttacker() ? 1 : 2;
    }
    else
    {
        m_type = 0;
    }

    m_speSkill = bd.m_speSkill;

    m_attack_type = 2;
    m_attack_type2 = 2;
    m_attack_type3 = 2;

    b_init_success = true; //初始化成功

    GenOrgObj();
}

General::General(Army* army,
        int id,
        int cid,
        int pos,
        const std::string& name,
        int level,
        int tong,
        int str,
        int wisdom,
        int stype,
        int base_type,
        int damage_type,
        int attack_type,
        int gong,
        int pufang,
        int cefang,
        int hp,
        int failure,
        int duoshan,
        int gedang,
        int shipo,
        int baoji,
        int xixue,
        int hunluan,
        int weihe,
        int podan
)
:m_army(army)
{
    m_nuqi_add = iAttackSuccessShiqi;
    m_nuqi_add_baoji = 2*m_nuqi_add;
    m_buff_changed = false;
    m_special_attack = 0;
    m_unique_id = 0;
    m_id = id;           //武将id
    m_spic = id;
    m_charactor = cid;    //所属角色id

    m_level = level;        //等级
    m_color = 0;

    m_stype = stype;        //兵种
    m_base_stype = base_type;   //基础兵种
    m_damage_type = damage_type;  // 攻击类别   1、物理攻击 2、策略攻击 3、治疗  4、士气
    m_damage_type2 = 1;      //伤害类型
    m_attack_type = attack_type;  // 攻击方式   1、单个目标 2、一列 3、一排 4、目标及周围 5、固定三个目标6、全体
    if (m_attack_type < range_single || m_attack_type > range_all)
    {
        m_attack_type = range_single;
    }
    m_real_attack_type = m_attack_type;

    m_soldier_spic = 1;        //兵种头像
    m_sname = "superman";      //兵种名

    //勇武、智力、统御
    m_str = str;
    m_int = wisdom;
    m_tongyu = tong;

    if (m_damage_type <= wuli_damage)
    {
        m_damage_type = wuli_damage;
    }
    else if (m_damage_type >= celue_damage)
    {
        m_damage_type = celue_damage;
    }
    //攻防
    m_attack = gong;
    m_wu_fang = pufang;
    m_ce_fang = cefang;

    //特性
    m_combat_attribute.special_attack(special_attack_dodge, duoshan);  //躲避概率
    m_combat_attribute.special_attack(special_attack_parry, gedang);   //格挡概率
    m_combat_attribute.special_attack(special_attack_shipo, shipo);    //识破概率

    m_combat_attribute.special_attack(special_attack_baoji, baoji);        //暴击
    m_combat_attribute.special_attack(special_attack_xixue, xixue);         //吸血概率
    m_combat_attribute.special_attack(special_attack_chaos, hunluan);      //混乱
    m_combat_attribute.special_attack(special_attack_weihe, weihe);        //威吓
    m_combat_attribute.special_attack(special_attack_podan, podan);        //破胆

    m_failure = failure;  //失败概率
    //血量
    m_hp_org = hp;   //原始血量
    m_hp_now = hp;   //当前血量

    //士气
    m_shiqi = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();    //受伤加的士气
    m_shiqi_old = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();

    m_pos = pos;      //9宫图中位置1-9
    m_name = name; //武将名
    m_nickname = "";
    b_init_success = true;
    m_chaos_flag = 0;   //混乱标记
    m_weihe_flag = 0;   //威吓标记
    m_baoji_flag = 0;   //暴击标记

    m_special_attack_fac = 220;

    if (m_army)
    {
        m_type = m_army->isAttacker() ? 1 : 2;
    }
    else
    {
        m_type = 0;
    }

    GenOrgObj();
}


//特殊攻击是否触发
int General::Weihe(const iGeneral& target)
{
    int weihe = m_combat_attribute.special_attack(special_attack_weihe) - target.resist(special_attack_weihe);
#ifdef TEST_FIGHT_RANDOM
    if (weihe > 0)
    {
        cout<<"check weihe if random(1,1000)<="<<weihe<<endl;
        int iRand = my_random(1, 1000);
        cout<<" rand = "<<iRand<<endl;
        return iRand <= weihe ? 1 : 0;
    }
#else
    if (weihe > 0 && my_random(1, 1000) <= weihe)
    {
        return 1;
    }
#endif
    return 0;
}

int General::Chaos(const iGeneral& target)
{
    //是否混乱免疫
    if (target.getBuffValue(buff_no_chaos) > 0)
    {
        return 0;
    }
    int luan = m_combat_attribute.special_attack(special_attack_chaos) - target.resist(special_attack_chaos);
#ifdef TEST_FIGHT_RANDOM
    if (luan > 0)
    {
        cout<<"check huluan if random(1,1000)<="<<luan<<endl;
        int iRand = my_random(1, 1000);
        cout<<" rand = "<<iRand<<endl;
        return iRand <= luan ? 1 : 0;
    }
#else
    if (luan > 0 && my_random(1, 1000) <= luan)
    {
        return 1;
    }
#endif
    return 0;
}

//是否格挡
int General::CheckGedang(const iGeneral& att)
{
    int parry = m_special_attacks[special_attack_parry] - att.resist(special_attack_parry);
    parry += getBuffValue(buff_gedang);

    int lv_diff = att.level() - m_level;
    if (lv_diff > 0 && parry > 0)
    {
        if (lv_diff > 70)
        {
            lv_diff = 70;
        }
        parry = parry * (100 -lv_diff) / 100;
    }

#ifdef TEST_FIGHT_RANDOM
    if (parry > 0)
    {
        cout<<"check gedang if random(1,1000)<="<<parry<<endl;
        int iRand = my_random(1, 1000);
        cout<<" rand = "<<iRand<<endl;
        return iRand <= parry ? 1 : 0;
    }
#else
    if (parry > 0 && my_random(1, 1000) <= parry)
    {
        return 1;
    }
#endif
    return 0;
}

//是否躲闪
int General::CheckDodge(const iGeneral& att)
{
    int dodge = m_special_attacks[special_attack_dodge] - att.resist(special_attack_dodge);
    dodge += getBuffValue(buff_dodge);
    dodge -= att.getBuffValue(buff_mingzhong);

    int lv_diff = att.level() - m_level;
    if (lv_diff > 0 && dodge > 0)
    {
        if (lv_diff > 70)
        {
            lv_diff = 70;
        }
        dodge = dodge * (100 -lv_diff) / 100;
    }

#ifdef TEST_FIGHT_RANDOM
    if (dodge > 0)
    {
        cout<<"check dodge if random(1,1000)<="<<dodge<<endl;
        int iRand = my_random(1, 1000);
        cout<<" rand = "<<iRand<<endl;
        return iRand <= dodge ? 1 : 0;
    }
#else
    if (dodge > 0 && my_random(1, 1000) <= dodge)
    {
        return 1;
    }
#endif
    return 0;
}

//是否识破
int General::CheckShipo(const iGeneral& att)
{    
    int shipo = m_special_attacks[special_attack_shipo] - att.resist(special_attack_shipo);
    shipo += getBuffValue(buff_shipo);

    int lv_diff = att.level() - m_level;
    if (lv_diff > 0 && shipo > 0)
    {
        if (lv_diff > 70)
        {
            lv_diff = 70;
        }
        shipo = shipo * (100 -lv_diff) / 100;
    }    
#ifdef TEST_FIGHT_RANDOM
    if (shipo > 0)
    {
        cout<<"check shipo if random(1,1000)<="<<shipo<<endl;
        int iRand = my_random(1, 1000);
        cout<<" rand = "<<iRand<<endl;
        return iRand <= shipo ? 1 : 0;
    }
#else
    if (shipo > 0 && my_random(1, 1000) <= shipo)
    {
        return 1;
    }
#endif
    return 0;
}

//是否兵种克制
bool General::CheckStypeDamage(int stype)
{
    if (m_base_stype >= 1 && m_base_stype <= 5)
    {
        //兵种克制(1步2弓3士4马5车)
        //步克器1->5
        //器克策5->3
        //策克骑3->4
        //骑克弓4->2
        //弓克步2->1
        const int tmp[5] = {5,1,4,2,3};
        return (stype == tmp[m_base_stype-1]);
    }
    else
    {
        return false;
    }
}

//还活着
bool General::isLive()
{
    //cout<<"hp now "<<m_hp_now<<endl;
    return m_hp_now > 0;
}

//加血
int General::Addhp(int hp)
{
    int old_hp = m_hp_now;
    m_hp_now += hp;
    if (m_hp_now > m_hp_org || m_hp_now <= 0)
    {
        m_hp_now = m_hp_org;
    }
    return m_hp_now - old_hp;
}

//无攻击
bool General::NoAttack()
{
    return (m_hp_now <= 0);// || m_stype == 17 || m_stype == 29);
}

int General::CheckState()
{
    //威吓逃兵少血
    for (std::list<boost::shared_ptr<Buff> >::iterator it = m_buff_list.begin(); it != m_buff_list.end(); ++it)
    {
        if (it->get() && (*it)->bbuff && (*it)->bbuff->type == debuff_taobing && (*it)->leftRound > 0)
        {
            //少血
            int damage = (*it)->value;
            if (damage <= 0)
            {
                damage = 1;
            }
            damage = RecieveDamage(damage, weihe_damage, 0);
            json_spirit::Object robj;
            json_spirit::Object obj = UpdateObj();
            robj.push_back( json_spirit::Pair("cmd", "damage"));
            robj.push_back( json_spirit::Pair("dtype", weihe_damage));
            robj.push_back( json_spirit::Pair("damage", damage));
            robj.push_back( json_spirit::Pair("g", obj));

            GetArmy().GetCombat().AppendResult(robj);
            if (m_hp_now == 0)
            {
                Die(*this);
                return -1;
            }
        }
    }
    //混乱，错过一次攻击
    if (m_chaos_flag)
    {
        --m_chaos_flag;
#if 0
        json_spirit::Object robj;
        json_spirit::Object obj = UpdateObj();
        robj.push_back( json_spirit::Pair("cmd", "action"));
        robj.push_back( json_spirit::Pair("action", 5));
        robj.push_back( json_spirit::Pair("type", m_type));
        robj.push_back( json_spirit::Pair("general", obj));
        robj.push_back( json_spirit::Pair("result", 1));
        GetArmy().GetCombat().AppendResult(robj);
#endif
        return -1;
    }
    return 0;
}

//正常的攻击
int General::Attack(iGeneral& target)
{
    int damage = Damages(target);
    //士气会增加伤害,特技攻击伤害系数决定特技伤害
    if (m_special_attack)
    {
        if (m_special_attack != 100)
        {
            // 超过100的士气，没点士气增加0.5%的伤害
            damage = damage * (100 + m_special_attack) / 200;
        }
        if (m_special_attack_fac != 100)
        {
            if (m_special_attack_fac == 9999)
            {
                //cout<<"General::Attack() fac 9999 "<<endl;
                damage = 9999999;
            }
            else
            {
                damage = damage * (m_special_attack_fac) / 100;
            }
        }
    }
    damage = target.RecieveDamage(damage, *this, m_damage_type, m_damage_type2, m_base_stype, false);
    if (m_special_attack && target.isLive())
    {
        for (std::list<skillEffect>::iterator it = m_speSkill->target_effects.begin(); it != m_speSkill->target_effects.end(); ++it)
        {
#ifdef TEST_SERVER
            int chance = 0;
            if (it->chance >= 1000)
            {
                //chance = 0;
            }
            else
            {
                chance = my_random(1, 1000);
                cout<<"add target buff "<<it->bbuff->type<<",random(1-1000)->"<<chance<<" vs "<<it->chance<<endl;
            }
            if (chance <= it->chance)
#else
            if (it->chance >= 1000 || my_random(1, 1000) <= it->chance)
#endif
            {
                switch (it->bbuff->type)
                {
                    //增加怒气
                    case effect_add_nuqi:
                    {
                        target.AddShiqi(it->value);
                        //加怒气信息
                        json_spirit::Object obj;
                        obj.push_back( Pair("re", ATTACK_RESULT_ADD_NUQI) );
                        obj.push_back( Pair("g", GetObj()) );
                        obj.push_back( Pair("nuqi", it->value) );
                        m_result_list.push_back(obj);
                        break;
                    }
                    //降低怒气
                    case effect_sub_nuqi:
                    {
                        //降低怒气也暴击
                        int nuqi = it->value;
                        if (m_baoji_flag)
                        {
                            nuqi = nuqi * (100+m_baoji_flag)/100;
                        }
                        target.AddShiqi(-nuqi);
                        //加怒气信息
                        json_spirit::Object obj;
                        obj.push_back( Pair("re", ATTACK_RESULT_SUB_NUQI) );
                        obj.push_back( Pair("g", GetObj()) );
                        obj.push_back( Pair("nuqi", nuqi) );
                        m_result_list.push_back(obj);
                        break;
                    }
                    default:
                        //增加buff
                        target.addBuff(it->bbuff, this, it->value, it->lastRound);
                        break;
                }
            }
        }
    }
    return damage;
}

//尝试计算攻击的伤害
int General::tryAttack(iGeneral& target)
{
    int damage = Damages(target);
    //士气会增加伤害,特技攻击伤害系数决定特技伤害
    if (m_special_attack)
    {
        if (m_special_attack != 100)
        {
            // 超过100的士气，没点士气增加0.5%的伤害
            damage = damage * (100 + m_special_attack) / 200;
        }
        if (m_special_attack_fac != 100)
        {
            if (m_special_attack_fac == 9999)
            {
                //cout<<"General::Attack() fac 9999 "<<endl;
                damage = 9999999;
            }
            else
            {
                damage = damage * (m_special_attack_fac) / 100;
            }
        }
    }
    damage = target.RecieveDamage(damage, *this, m_damage_type, m_damage_type2, m_base_stype, true);    
    return damage;
}

//设定伤害的攻击
int General::Attack(iGeneral& target, int damage, bool special_attack)
{
    damage = target.RecieveDamage(damage, *this, m_damage_type, m_damage_type2, m_base_stype, false);
    if (special_attack && m_special_attack && target.isLive())
    {
        for (std::list<skillEffect>::iterator it = m_speSkill->target_effects.begin(); it != m_speSkill->target_effects.end(); ++it)
        {
#ifdef TEST_SERVER
            int chance = 0;
            if (it->chance >= 1000)
            {
                //chance = 0;
            }
            else
            {
                chance = my_random(1, 1000);
                cout<<"add target buff "<<it->bbuff->type<<",random(1-1000)->"<<chance<<" vs "<<it->chance<<endl;
            }
            if (chance <= it->chance)
#else
            if (it->chance >= 1000 || my_random(1, 1000) <= it->chance)
#endif
            {
                switch (it->bbuff->type)
                {
                    //增加怒气
                    case effect_add_nuqi:
                    {
                        target.AddShiqi(it->value);
                        //加怒气信息
                        json_spirit::Object obj;
                        obj.push_back( Pair("re", ATTACK_RESULT_ADD_NUQI) );
                        obj.push_back( Pair("g", GetObj()) );
                        obj.push_back( Pair("nuqi", it->value) );
                        m_result_list.push_back(obj);
                        break;
                    }
                    //降低怒气
                    case effect_sub_nuqi:
                    {
                        //降低怒气也暴击
                        int nuqi = it->value;
                        if (m_baoji_flag)
                        {
                            nuqi = nuqi * (100+m_baoji_flag)/100;
                        }
                        target.AddShiqi(-nuqi);
                        //加怒气信息
                        json_spirit::Object obj;
                        obj.push_back( Pair("re", ATTACK_RESULT_SUB_NUQI) );
                        obj.push_back( Pair("g", GetObj()) );
                        obj.push_back( Pair("nuqi", nuqi) );
                        m_result_list.push_back(obj);
                        break;
                    }
                    default:
                        //增加buff
                        target.addBuff(it->bbuff, this, it->value, it->lastRound);
                        break;
                }
            }
        }
    }
    return damage;
}

void General::GenAttackMsg(int result)
{
    json_spirit::Object robj;
    robj.push_back( json_spirit::Pair("cmd", "action"));
    robj.push_back( json_spirit::Pair("action", m_damage_type));
    robj.push_back( json_spirit::Pair("type", m_type));
    robj.push_back( json_spirit::Pair("general", m_cur_obj));
    robj.push_back( json_spirit::Pair("result", result));

    //普通攻击是近战的
    int attack_type = 1;
    //绝招攻击
    if (m_special_attack)
    {
        attack_type = m_attack_type2;
    }
    else
    {
        attack_type = m_attack_type3;
    }
    robj.push_back( json_spirit::Pair("attackType", attack_type));
    GetArmy().GetCombat().AppendResult(robj);
}

void General::GenAttackResultMsg(int result)
{
    json_spirit::Object robj;
    robj.push_back( json_spirit::Pair("cmd", "result"));
    robj.push_back( json_spirit::Pair("type", m_type));
    robj.push_back( json_spirit::Pair("general", m_cur_obj));
    robj.push_back( json_spirit::Pair("result", result));
    GetArmy().GetCombat().AppendResult(robj);
}

int General::Attack_Result(iGeneral& target)
{
    AddShiqi(0, true);
    //是否失败
    if (m_failure > 0 && m_failure >= my_random(0, 100))
    {
        //攻击失败信息
        //GenAttackMsg(ATTACK_RESULT_FAIL);
        return ATTACK_RESULT_FAIL;
    }

    bool bDodge = false;
    //是否躲闪
    if (target.CheckDodge(*this))
    {
        //躲闪信息
        json_spirit::Object obj;
        obj.push_back( Pair("re", ATTACK_RESULT_DUOSHAN) );
        obj.push_back( Pair("g", target.GetObj()) );
        m_result_list.push_back(obj);
        //return ATTACK_RESULT_DUOSHAN;
        bDodge = true;
    }
    //物理攻击判断先格挡
    if (wuli_damage == m_damage_type)
    {
        if (target.CheckGedang(*this))
        {
            //被格挡/识破自己受伤害
            int damage = target.Attack(*this);

            //格挡信息
            json_spirit::Object obj;
            obj.push_back( Pair("re", ATTACK_RESULT_GEDANG) );
            obj.push_back( Pair("g", target.GetObj()) );
            obj.push_back( Pair("damage", damage) );
            m_result_list.push_back(obj);
            if (bDodge)
            {
                return ATTACK_RESULT_GEDANG_DUOSHAN;
            }
            else
            {
                return ATTACK_RESULT_GEDANG;
            }
        }
    }
    else if (celue_damage == m_damage_type)
    {
        if (target.CheckShipo(*this))
        {
            //识破信息
            int damage = target.Attack(*this);
            json_spirit::Object obj;
            obj.push_back( Pair("re", ATTACK_RESULT_SHIPO) );
            obj.push_back( Pair("g", target.GetObj()) );
            obj.push_back( Pair("damage", damage) );
            m_result_list.push_back(obj);
            if (bDodge)
            {
                return ATTACK_RESULT_SHIPO_DUOSHAN;
            }
            else
            {
                return ATTACK_RESULT_SHIPO;
            }
        }
    }
    if (bDodge)
    {
        return ATTACK_RESULT_DUOSHAN;
    }
    if (Set_BaojiFlag(target))
    {
        return ATTACK_RESULT_BAOJI;
    }
    else
    {
        return ATTACK_RESULT_SUCCESS;
    }    
}

int General::resist(int type) const
{
    if (type >= 0 && type < special_attack_max)
    {
        return m_special_resists[type];
    }
    return 0;
}

int General::resist_level(int type) const
{
    if (type >= 0 && type < special_attack_max)
    {
        return m_combat_attribute.special_resist_level(type);
    }
    return 0;
}

//暴击判定
int General::Set_BaojiFlag(const iGeneral& target)
{
    int baoji = m_special_attacks[special_attack_baoji] - target.resist(special_attack_baoji);
    baoji += getBuffValue(buff_baoji);

    int lv_diff = target.level() - m_level;
    if (lv_diff > 0 && baoji > 0)
    {
        if (lv_diff > 70)
        {
            lv_diff = 70;
        }
        baoji = baoji * (100 -lv_diff) / 100;
    }

#ifdef TEST_FIGHT_RANDOM
    if (baoji > 0)
    {
        cout<<"check baoji if random(1,1000)<="<<baoji<<endl;
        int iRand = my_random(1, 1000);
        cout<<" rand = "<<iRand<<endl;
        if (iRand <= baoji)
        {
            m_baoji_flag = m_combat_attribute.m_special_attack[special_attack_baoji][1];
            cout<<"baoji "<<m_baoji_flag<<endl;
            return m_baoji_flag;
        }
    }
#else
    if (baoji > 0 && baoji >= my_random(1, 1000))
    {
        m_baoji_flag = iBaojiMoreDamage;
        return m_baoji_flag;
    }
#endif
    m_baoji_flag = 0;
    return m_baoji_flag;
}

//受普通伤害和策略伤害的减免
int General::subDamage(int attack_type)
{    
    switch (attack_type)
    {
        case wuli_damage:
        case celue_damage:
            return m_combat_attribute.sub_damage(attack_type-1);
        default:
            return 0;
    }
}

//受来自特定兵种的伤害减免
int General::subDamageFrom(int base_stype)
{
    if (base_stype >= 1 && base_stype <= 5)
    {
        return m_combat_attribute.sub_damage_from(base_stype);
    }
    else
    {
        return 0;
    }
}

//对特定兵种加强伤害
int General::moreDamageto(int base_stype)
{
    if (base_stype >= 1 && base_stype <= 5)
    {
        return m_combat_attribute.more_damage_to(base_stype);
    }
    else
    {
        return 0;
    }
}

//伤害公式
int General::Damages(iGeneral& target)
{
    int org_damage = 0;
    int damage_fac = 0;

    int64_t damage = 0;
    if (wuli_damage == m_damage_type)
    {
        org_damage = Gong(target.baseType())- target.Wufang(m_base_stype);
        if (org_damage <= 0)
        {
            org_damage = 1;
        }
    }
    else if (celue_damage == m_damage_type)
    {
        org_damage = Gong(target.baseType()) - target.CeFang(m_base_stype);
        if (org_damage <= 0)
        {
            org_damage = 1;
        }
    }
    //兵种克制增加兵魂效果
    if (CheckStypeDamage(target.baseType()))
    {
        //cout << "attaker_stype=" << m_base_stype << ",defender_stype=" << target.baseType() << endl;
        //cout << "org_damage=" << org_damage;
        //cout << " more " << m_combat_attribute.soul_stypeDamage(m_base_stype) << "%";
        org_damage = org_damage * (100 + m_combat_attribute.soul_stypeDamage(m_base_stype)) / 100;
        //cout << " after_damage=" << org_damage << endl;
    }
    //else
    //{
    //    //cout << "FAIL********attaker_stype=" << m_base_stype << ",defender_stype=" << target.baseType() << endl;
    //}
    int lv_diff = m_level - target.level();
    if (lv_diff > 0)
    {
        if (lv_diff > 30)
        {
            lv_diff = 30;
        }
        damage = (org_damage) * (100 + lv_diff) / 100;
    }
    else
    {
        damage = org_damage;
    }

    //暴击+50%伤害
    if (m_baoji_flag)
    {
        damage = (m_baoji_flag + 100) * damage / 100;
    }
#ifdef TEST_SERVER
    INFO("|damage:"<<damage);
#endif    

    /*
    状态、兵种克制
    */
    int more_damage = moreDamageto(target.baseType());
    if (more_damage > 0)
    {
        damage = (100+more_damage)*damage / 100;
    }
#ifdef TEST_SERVER
    INFO("|damage:"<<damage);
#endif

    /*死战效果*/
    if (m_combat_attribute.death_fight() && m_hp_org > m_hp_now)
    {
        damage = (2*m_hp_org - m_hp_now) * damage / m_hp_org;
    }
#ifdef TEST_SERVER
        INFO("|damage:hp_org/hp_now:"<<m_hp_org<<"/"<<m_hp_now<<","<<damage);
#endif    
    //鼓舞效果
    INFO("|damage: boss inspired "<<m_combat_attribute.boss_inspired()<<","<<damage);
    if (m_combat_attribute.total_inspired() != 0)
    {
        damage = (m_combat_attribute.total_inspired() + 100) * damage / 100;
#ifdef TEST_SERVER
        INFO("|damage: boss inspired "<<m_combat_attribute.total_inspired()<<","<<damage);
#endif    
    }

    //攻击方士气影响
    //int shiqi = Shiqi();
    //int shiqi_more_damage = getShiqiMoreDamage(shiqi);
    //if (shiqi_more_damage != 0)
    //{
    //    damage = (100 + shiqi_more_damage) * damage / 100;
    //}

    //被攻击方士气
    //int shiqi_d = target.Shiqi();
    //int shiqi_sub_damage = getShiqiSubDamage(shiqi_d);
    //if (shiqi_sub_damage != 0)
    //{
    //    damage = (100 - shiqi_sub_damage) * damage /100;
    //}

#if 0
    //虚弱状态
    if (m_combat_attribute.m_weak > 0)
    {
        damage = (100 - m_combat_attribute.m_weak) * damage / 100;
    }
#endif

#ifdef TEST_SERVER
    //INFO("|shiqi:"<<shiqi<<"vs"<<shiqi_d<<",damage:"<<damage);
#endif

    //季节原因的伤害变化
    //int fac = GetGlobalFac(m_damage_type);
    //if (fac != 0)
    //{
    //    damage = (100 + fac) * damage / 100;
    //}
    //兵种防御效果
    if (m_base_stype > 0)
    {
        int sub_damage = target.subDamageFrom(m_base_stype);
        if (sub_damage > 0)
        {
            damage = (100-sub_damage)*damage/100;
        }
    }

    //基本防御效果
    int sub_damage = target.subDamage(m_damage_type);
    if (sub_damage > 0)
    {
        damage = (100-sub_damage)*damage/100;
    }

    {
        //最低伤害5%
        int _min_damage = 0;
        if (target.baseType() == 6)    //boss的伤害最小值特殊处理
        {
            _min_damage = my_random(1,100);
        }
        else
        {
            _min_damage = target.MaxHp()/20;
        }
        if (damage <= _min_damage)
        {
            damage = _min_damage;
        }
    }
#ifdef TEST_SERVER
    INFO("|damage:"<<damage);
#endif
    //伤害浮动 +- 10%
    //damage = damage * my_random(95,105)/100 + 1;

#ifdef TEST_SERVER
    INFO("|final damage "<<damage<<endl);
#endif
    return (int)damage;
}

//对目标逃兵的伤害
int General::taobingDamage(iGeneral& target)
{
    int org_damage = 0;
    int damage_fac = 0;

    int64_t damage = 0;
    if (wuli_damage == m_damage_type)
    {
        org_damage = Gong(target.baseType())- target.Wufang(m_base_stype);
        if (org_damage <= 0)
        {
            org_damage = 1;
        }
        int lv_diff = m_level - target.level();
        if (lv_diff > 0)
        {
            if (lv_diff > 30)
            {
                lv_diff = 30;
            }
            damage = (org_damage) * (100 + lv_diff) / 100;
        }
        else
        {
            damage = org_damage;
        }
    }
    else if (celue_damage == m_damage_type)
    {
        org_damage = Gong(target.baseType()) - target.CeFang(m_base_stype);
        if (org_damage <= 0)
        {
            org_damage = 1;
        }
        
        int lv_diff = m_level - target.level();
        if (lv_diff > 0)
        {
            if (lv_diff > 30)
            {
                lv_diff = 30;
            }
            damage = (org_damage) * (100 + lv_diff) / 100;
        }
        else
        {
            damage = org_damage;
        }
    }
    /*
    状态、兵种克制
    */
    int more_damage = moreDamageto(target.baseType());
    if (more_damage > 0)
    {
        damage = (100+more_damage)*damage / 100;
    }

    //鼓舞效果
    if (m_combat_attribute.total_inspired() > 0)
    {
        damage = (m_combat_attribute.total_inspired() + 100) * damage / 100;
    }

    //季节原因的伤害变化
    //int fac = GetGlobalFac(m_damage_type);
    //if (fac != 0)
    //{
    //    damage = (100 + fac) * damage / 100;
    //}
    //兵种防御效果
    if (m_base_stype > 0)
    {
        int sub_damage = target.subDamageFrom(m_base_stype);
        if (sub_damage > 0)
        {
            damage = (100-sub_damage)*damage/100;
        }
    }

    //基本防御效果
    int sub_damage = target.subDamage(m_damage_type);
    if (sub_damage > 0)
    {
        damage = (100-sub_damage)*damage/100;
    }

    //最低伤害5%
    int _min_damage = 0;
    if (target.baseType() == 6)    //boss的伤害最小值特殊处理
    {
        _min_damage = my_random(1,100);
    }
    else
    {
        _min_damage = target.MaxHp()/20;
    }
    if (damage <= _min_damage)
    {
        damage = _min_damage;
    }
    return (int)(damage/2);
}

//受到伤害扣除生命 - 伤害减免在这里处理
int General::RecieveDamage(int damage, int type, int attacker_base_type = 0, bool test)
{
    if (!test)
    {
        if (m_hp_now > damage)
        {
            m_hp_now -= damage;
        }
        else
        {
            //damages = m_hp_now;
            m_hp_now = 0;
        }
    }
    /*伤害信息
    json_spirit::Object robj;
    robj.push_back( json_spirit::Pair("cmd", "attack"));
    robj.push_back( json_spirit::Pair("type", damage));
    robj.push_back( json_spirit::Pair("dd", Id()));
    robj.push_back( json_spirit::Pair("dam", damage));
    robj.push_back( json_spirit::Pair("hp2", Hp()));
    if (wuli_damage == type          //物理伤害
        || celue_damage == type)     //策略伤害
    {
        //增加士气
        AddShiqi(damages * 200 / m_hp_org);
    }
    robj.push_back( json_spirit::Pair("shiqi", Shiqi()));
    m_combat_handle.AppendResult(json_spirit::write(robj));*/
#ifdef TEST_SERVER
    INFO("|recieve final damage "<<damage<<endl);
#endif
    
    //伤害信息
    return damage;
}

void General::GenDamageInfo(int damage, int attack_type, int damage_type, bool luan, bool weihe, bool podan, bool baoji)
{
#if 0
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "damage") );
    robj.push_back( Pair("dtype", attack_type) );
    robj.push_back( Pair("injure", damage_type) );
    robj.push_back( Pair("damage", damage) );
    if (luan)
    {
        robj.push_back( Pair("chaos", 1) );
    }
    if (weihe)
    {
        robj.push_back( Pair("weihe", 1) );
    }
    if (podan)
    {
        robj.push_back( Pair("podan", 1) );
    }
    if (baoji)
    {
        robj.push_back( Pair("force", 1) );
    }
    robj.push_back( Pair("type", m_type) );
    robj.push_back( Pair("general", GetObj()) );

    GetArmy().GetCombat().AppendResult(robj);
#endif    
}

//受到伤害扣除生命 - 伤害减免在这里处理
int General::RecieveDamage(int damage, iGeneral& attacker, int attack_type, int injure_type, int attacker_base_type = 0, bool test = false)
{
    //int hp_old = m_hp_now;
    int org_damages = RecieveDamage(damage, attack_type, attacker_base_type, test);
    int damages = org_damages;// >= hp_old ? hp_old : org_damages;
    if (test)
    {
        return damages;
    }
    //死亡
    if (m_hp_now == 0)
    {
        Die(attacker);
    }
    UpdateObj();
    return damages;
}

int General::total_inspired()
{
    return m_combat_attribute.total_inspired() - m_combat_attribute.org_inspired();
}

//英雄死亡
int General::Die(iGeneral& killer)
{
    return 0;
}

//增加士气
int General::AddShiqi(int add, bool type)
{
    m_shiqi_old = m_shiqi;
    m_shiqi += add;
    if (m_shiqi > 200)
    {
        m_shiqi = 200;
    }
    else if (m_shiqi < 0)
    {
        m_shiqi = 0;
    }
    add = m_shiqi - m_shiqi_old;
    if (type)
    {
        m_shiqi_old = m_shiqi;
    }
    m_cur_obj.clear();
    return add;
}

//攻击灵魂锁链
int General::AttackSoulLink(Army& amy, const std::list<iGeneral*>& glist, int pos)
{
    //对方有灵魂锁链
    int ret = ATTACK_RESULT_SUCCESS;
    int total_damage = 0;
    int result_list[9];
    memset(result_list, 0, sizeof(int)*9);

    //计算伤害
    std::list<iGeneral*>::const_iterator it = glist.begin();
    while (it != glist.end())
    {
        int result = Attack_Result(**it);
        if (ATTACK_RESULT_SUCCESS == result || ATTACK_RESULT_BAOJI == result)
        {
            total_damage += tryAttack(**it);
        }
        if (result > ret)
        {
            ret = result;
        }
        result_list[(*it)->Pos()-1] = result;
        ++it;
    }

    m_the_damage_total += total_damage;

    if (total_damage > 0)
    {
        int count = amy.getLiveGeneralCount();
        assert(count > 0);
        
        int link_damage = total_damage / count;
        if (link_damage <= 0)
        {
            link_damage = 1;
        }
        //全体伤害
        std::list<iGeneral*> glist;
        amy.GetAllGenerals(pos, glist);
        std::list<iGeneral*>::iterator it = glist.begin();
        while (it != glist.end())
        {
            int result = result_list[(*it)->Pos()-1];
            int damage = 0;
            if (ATTACK_RESULT_SUCCESS == result || ATTACK_RESULT_BAOJI == result)
            {
                damage = Attack(**it, link_damage, true);
            }
            else
            {
                damage = Attack(**it, link_damage, false);
                result = ATTACK_RESULT_SUCCESS;
            }
            json_spirit::Object obj;
            obj.push_back( Pair("re", result) );
            obj.push_back( Pair("g", (**it).GetObj()) );
            obj.push_back( Pair("damage", damage) );
            m_result_list.push_back(obj);
            ++it;
        }
    }
    return ret;
}

//点攻击
int General::AttackArmy_Single()
{
    Army* army = GetArmy().GetEnermy();
    //获得目标
    iGeneral* target = army->GetRowGeneral(POS_TO_ROW(m_pos));
    if (NULL != target)
    {
        int soul_link = army->in_soul_link();
        if (soul_link == 0)
        {
            //Set_BaojiFlag(*target);
            int result = Attack_Result(*target);
            if (ATTACK_RESULT_SUCCESS == result || ATTACK_RESULT_BAOJI == result)
            {
                //命中
                int damage = Attack(*target);
                m_the_damage_total += damage;
                json_spirit::Object obj;
                obj.push_back( Pair("re", result) );
                obj.push_back( Pair("g", target->GetObj()) );
                obj.push_back( Pair("damage", damage) );
                m_result_list.push_back(obj);
            }
            return result;
        }
        else
        {
            //对方有灵魂锁链    
            int result = Attack_Result(*target);
            if (ATTACK_RESULT_SUCCESS == result || ATTACK_RESULT_BAOJI == result)
            {
                //命中了
                int damage = tryAttack(*target);
                m_the_damage_total += damage;
                //分摊伤害
                std::list<iGeneral*> glist;
                army->GetAllGenerals(target->Pos(), glist);

                int link_damage = damage / glist.size();
                if (link_damage <= 0)
                {
                    link_damage = 1;
                }
                std::list<iGeneral*>::iterator it = glist.begin();
                while (it != glist.end())
                {
                    int real_damage = 0, real_result = 0;
                    if (target == *it)
                    {
                        real_damage = Attack(**it, link_damage, true);
                        real_result = result;
                    }
                    else
                    {
                        real_damage = Attack(**it, link_damage, false);
                        real_result = ATTACK_RESULT_SUCCESS;
                    }
                    json_spirit::Object obj;
                    obj.push_back( Pair("re", real_result) );
                    obj.push_back( Pair("g", (**it).GetObj()) );
                    obj.push_back( Pair("damage", real_damage) );
                    m_result_list.push_back(obj);
                    ++it;
                }
            }
            return result;
        }
    }
    else
    {
        //赢了
        return ATTACK_RESULT_SUCCESS;
    }
}

//列攻击-优先前排
int General::AttackArmy_Row()
{
    Army * army = GetArmy().GetEnermy();
    //获得目标
    iGeneral* target = army->GetRowGeneral(POS_TO_ROW(m_pos));
    if (NULL != target)
    {
        std::list<iGeneral*> glist;
        army->GetRowGenerals(target->Pos(), glist);

        //对方是否有灵魂锁链
        int soul_link = army->in_soul_link();
        if (soul_link == 0)
        {
            int ret = ATTACK_RESULT_SUCCESS;
            std::list<iGeneral*>::iterator it = glist.begin();
            while (it != glist.end())
            {
                int result = Attack_Result(**it);
                if (ATTACK_RESULT_SUCCESS == result || ATTACK_RESULT_BAOJI == result)
                {
                    int damage = Attack(**it);
                    m_the_damage_total += damage;
                    json_spirit::Object obj;
                    obj.push_back( Pair("re", result) );
                    obj.push_back( Pair("g", (**it).GetObj()) );
                    obj.push_back( Pair("damage", damage) );
                    m_result_list.push_back(obj);
                }
                ++it;
                if (result > ret)
                {
                    ret = result;
                }
            }
            return ret;
        }
        else
        {
            //对方有灵魂锁链
            return AttackSoulLink(*army, glist, target->Pos());
        }
    }
    else
    {
        //赢了
        return ATTACK_RESULT_SUCCESS;
    }
}

//面攻击-优先前排
int General::AttackArmy_Side()
{
    //cout<<"*****************AttackArmy_Side()*******************"<<endl;
    Army * army = GetArmy().GetEnermy();
    //获得目标
    iGeneral* target = army->GetRowGeneral(POS_TO_ROW(m_pos));
    if (NULL != target)
    {
        std::list<iGeneral*> glist;
        army->GetSideGenerals(target->Pos(), glist);

        //对方是否有灵魂锁链
        int soul_link = army->in_soul_link();
        if (soul_link == 0)
        {
            int ret = ATTACK_RESULT_SUCCESS;
            std::list<iGeneral*>::iterator it = glist.begin();
            while (it != glist.end())
            {
                int result = Attack_Result(**it);
                if (ATTACK_RESULT_SUCCESS == result || ATTACK_RESULT_BAOJI == result)
                {
                    int damage = Attack(**it);
                    m_the_damage_total += damage;
                    json_spirit::Object obj;
                    obj.push_back( Pair("re", result) );
                    obj.push_back( Pair("g", (**it).GetObj()) );
                    obj.push_back( Pair("damage", damage) );
                    m_result_list.push_back(obj);
                }
                if (result > ret)
                {
                    ret = result;
                }
                ++it;
            }
            return ret;
        }
        else
        {
            return AttackSoulLink(*army, glist, target->Pos());
        }
    }
    else
    {
        //赢了
        return ATTACK_RESULT_SUCCESS;
    }
}

//溅射攻击
int General::AttackArmy_Round()
{
    Army * army = GetArmy().GetEnermy();
    //获得目标
    iGeneral* target = army->GetRowGeneral(POS_TO_ROW(m_pos));
    if (NULL != target)
    {
        std::list<iGeneral*> glist;
        army->GetAroundGenerals(target->Pos(), glist);
        //对方是否有灵魂锁链
        int soul_link = army->in_soul_link();
        if (soul_link == 0)
        {
            int ret = ATTACK_RESULT_SUCCESS;
            std::list<iGeneral*>::iterator it = glist.begin();
            while (it != glist.end())
            {
                int result = Attack_Result(**it);
                if (ATTACK_RESULT_SUCCESS == result || ATTACK_RESULT_BAOJI == result)
                {
                    int damage = Attack(**it);
                    m_the_damage_total += damage;
                    json_spirit::Object obj;
                    obj.push_back( Pair("re", result) );
                    obj.push_back( Pair("g", (**it).GetObj()) );
                    obj.push_back( Pair("damage", damage) );
                    m_result_list.push_back(obj);
                }
                if (result > ret)
                {
                    ret = result;
                }
                ++it;
            }
            return ret;
        }
        else
        {
            //对方有灵魂锁链
            return AttackSoulLink(*army, glist, target->Pos());
        }
    }
    else
    {
        //赢了
        return ATTACK_RESULT_SUCCESS;
    }
}

/*
//连环攻击
int General::AttackArmy_ByStep()
{
    Army & army = GetArmy().GetEnermy();
    //获得目标
    iGeneral* target = army.GetRowGeneral(POS_TO_ROW(m_pos));
    if (NULL != target)
    {
        Set_BaojiFlag();
        //攻击目标
        int result = Attack_Result(*target);
        if (ATTACK_RESULT_SUCCESS == result)
        {
            std::list<iGeneral*> glist;
            army.GetSomeGenerals(target->Pos(), 3, glist);
            std::list<iGeneral*>::iterator it = glist.begin();
            while (it != glist.end())
            {
                Attack(**it);
                ++it;
            }
        }
        return result;
    }
    else
    {
        //赢了
        return ATTACK_RESULT_SUCCESS;
    }
}
*/

//引导攻击
int General::AttackArmy_ByStep()
{
    Army * army = GetArmy().GetEnermy();
    //获得目标
    int cur_row = POS_TO_ROW(m_pos);
    iGeneral* target = army->GetRowGeneral(cur_row);
    if (NULL != target)
    {
        cur_row = POS_TO_ROW(target->Pos());
        std::list<iGeneral*> glist;
        glist.push_back(target);
        for (int row = 0; row < 3; ++row)
        {
            if (cur_row != row)
            {
                iGeneral* t = army->GetRowGeneral2(row);
                if (NULL != t)
                {
                    glist.push_back(t);
                }
            }
        }

        //对方是否有灵魂锁链
        int soul_link = army->in_soul_link();
        if (soul_link == 0)
        {
            int ret = ATTACK_RESULT_SUCCESS;
            std::list<iGeneral*>::iterator it = glist.begin();
            while (it != glist.end())
            {
                int result = Attack_Result(**it);
                if (ATTACK_RESULT_SUCCESS == result || ATTACK_RESULT_BAOJI == result)
                {
                    int damage = Attack(**it);
                    m_the_damage_total += damage;
                    json_spirit::Object obj;
                    obj.push_back( Pair("re", result) );
                    obj.push_back( Pair("g", (**it).GetObj()) );
                    obj.push_back( Pair("damage", damage) );
                    m_result_list.push_back(obj);
                }
                if (result > ret)
                {
                    ret = result;
                }
                ++it;
            }
            return ret;
        }
        else
        {
            //对方有灵魂锁链
            return AttackSoulLink(*army, glist, target->Pos());
        }
    }
    else
    {
        //赢了
        return ATTACK_RESULT_SUCCESS;
    }
}

//全体攻击
int General::AttackArmy_All()
{
    Army * army = GetArmy().GetEnermy();
    //获得目标
    iGeneral* target = army->GetRowGeneral(POS_TO_ROW(m_pos));
    if (NULL != target)
    {
        int ret = ATTACK_RESULT_SUCCESS;
        std::list<iGeneral*> glist;
        army->GetAllGenerals(target->Pos(), glist);

        //对方是否有灵魂锁链
        int soul_link = army->in_soul_link();
        if (soul_link == 0)
        {
            std::list<iGeneral*>::iterator it = glist.begin();
            while (it != glist.end())
            {
                int result = Attack_Result(**it);
                if (ATTACK_RESULT_SUCCESS == result || ATTACK_RESULT_BAOJI == result)
                {
                    int damage = Attack(**it);
                    m_the_damage_total += damage;
                    json_spirit::Object obj;
                    obj.push_back( Pair("re", result) );
                    obj.push_back( Pair("g", (**it).GetObj()) );
                    obj.push_back( Pair("damage", damage) );
                    m_result_list.push_back(obj);
                }
                if (result > ret)
                {
                    ret = result;
                }
                ++it;
            }
            return ATTACK_RESULT_SUCCESS;
        }
        else
        {
            //对方有灵魂锁链
            return AttackSoulLink(*army, glist, target->Pos());
        }
    }
    else
    {
        //赢了
        return ATTACK_RESULT_SUCCESS;
    }
}

int General::HealOne()
{
    Army * army = GetArmy().GetEnermy();
    iGeneral* g = GetArmy().GetMinhpGeneral();
    //获得目标
    iGeneral* target = army->GetRowGeneral(POS_TO_ROW(m_pos));
    if (target != NULL && NULL != g)
    {
        int damage = Damages(*target);
        //士气会增加伤害,特技攻击伤害系数决定特技伤害
        if (m_special_attack)
        {
            if (m_special_attack != 100)
            {
                // 超过100的士气，没点士气增加0.5%的伤害
                damage = damage * (100 + m_special_attack) / 200;
            }
            if (m_special_attack_fac != 100)
            {
                if (m_special_attack_fac == 9999)
                {
                    //cout<<"General::Attack() fac 9999 "<<endl;
                    damage = 9999999;
                }
                else
                {
                    damage = damage * (m_special_attack_fac) / 100;
                }
            }
        }
        //不真正受到伤害，只是计算伤害
        damage = target->RecieveDamage(damage, *this, celue_damage, m_damage_type2, m_base_stype, true);

        json_spirit::Object obj;
        int more_heal = checkBaoji();
        if (more_heal)
        {
            damage = damage * (100+more_heal)/100;
            obj.push_back( Pair("cri", more_heal) );
        }
        //int real = g->Addhp(damage);
        g->Addhp(damage);
        g->UpdateObj();
        //治疗信息
        obj.push_back( Pair("re", ATTACK_RESULT_HEAL) );
        obj.push_back( Pair("g", g->GetObj()) );
        obj.push_back( Pair("heal", damage) );
        m_result_list.push_back(obj);
    }
    return ATTACK_RESULT_SUCCESS;
}

//治疗和增加怒气的时候是否暴击
int General::checkBaoji()
{
    int baoji = m_special_attacks[special_attack_baoji];
    baoji += getBuffValue(buff_baoji);
    baoji = baoji / 2;

    if (baoji > 0 && baoji >= my_random(1, 1000))
    {
        m_baoji_flag = iBaojiMoreDamage;
        return m_baoji_flag;
    }
    else
    {
        return 0;
    }
}

int General::HealAll()
{
    Army * army = GetArmy().GetEnermy();
    iGeneral* target = army->GetRowGeneral(POS_TO_ROW(m_pos));
    if (target != NULL)
    {
        int damage = Damages(*target);
        //士气会增加伤害,特技攻击伤害系数决定特技伤害
        if (m_special_attack)
        {
            if (m_special_attack != 100)
            {
                // 超过100的士气，没点士气增加0.5%的伤害
                damage = damage * (100 + m_special_attack) / 200;
            }
            if (m_special_attack_fac != 100)
            {
                if (m_special_attack_fac == 9999)
                {
                    //cout<<"General::Attack() fac 9999 "<<endl;
                    damage = 9999999;
                }
                else
                {
                    damage = damage * (m_special_attack_fac) / 100;
                }
            }
        }

        //不真正受到伤害，只是计算伤害
        damage = target->RecieveDamage(damage, *this, celue_damage, m_damage_type2, m_base_stype, true);

        std::list<iGeneral*> glist;
        m_army->GetAllGenerals(target->Pos(), glist);
        std::list<iGeneral*>::iterator it = glist.begin();
        while (it != glist.end())
        {
            json_spirit::Object obj;
            int more_heal = checkBaoji();
            if (more_heal)
            {
                damage = damage * (100+more_heal)/100;
                obj.push_back( Pair("cri", more_heal) );
            }
            //int real = (*it)->Addhp(damage);
            (*it)->Addhp(damage);
            (*it)->UpdateObj();
            //治疗信息

            obj.push_back( Pair("re", ATTACK_RESULT_HEAL) );
            obj.push_back( Pair("g", (*it)->GetObj()) );
            obj.push_back( Pair("heal", damage) );
            m_result_list.push_back(obj);
            ++it;
        }        
    }
    return ATTACK_RESULT_SUCCESS;
}

int General::Gong(int totype)
{
    int gong = m_attack;
    int gj = getBuffValue(buff_gongji);
    if (totype >= 1 && totype <= 5)
    {
        gong = m_attack + m_combat_attribute.skill_add(totype * 2 + 3); // 5 7 9 11
    }
    else
    {
        gong = m_attack;
    }
    if (gj != 0)
    {
        gong = gong * (100 + gj)/100;
    }
    return gong;
}

int General::Wufang(int fromtype)
{
    int fang = m_wu_fang;
    if (fromtype >= 1 && fromtype <= 5)
    {
        fang = m_wu_fang + m_combat_attribute.skill_add(fromtype * 2 + 4); // 6 8 10 12
    }

    int fangyu = getBuffValue(buff_fangyu);
    if (fangyu != 0)
    {
        fang = (100 + fangyu) * fang / 100;
    }
    fang += getBuffValue(buff_pu_fang);
    return fang;
}

int General::CeFang(int fromtype)
{
    int fang = m_ce_fang;
    if (fromtype >= 1 && fromtype <= 5)
    {
        fang = m_ce_fang + m_combat_attribute.skill_add(fromtype * 2 + 4); // 6 8 10 12
    }

    int fangyu = getBuffValue(buff_fangyu);
    if (fangyu != 0)
    {
        fang = (100 + fangyu) * fang / 100;
    }
    fang += getBuffValue(buff_ce_fang);
    return fang;
}

//行动
int General::Action()
{
    m_baoji_flag = 0;
    m_the_damage_total = 0;
    m_action_obj.clear();
    m_result_list.clear();
    if (-1 == CheckState())
    {
        return ATTACK_RESULT_SUCCESS;
    }

    json_spirit::Array blist;
#if 0    
    //攻击方式的转换
    m_real_attack_type = m_attack_type;
    //只有单体攻击的英雄才会有可能转变攻击方式
    if (m_attack_type == range_single)
    {
        for (std::list<combatSpeSkill>::iterator it = m_attack_skills.begin(); it != m_attack_skills.end(); ++it)
        {
            if (my_random(1, 1000) <= it->chance)
            {
                m_real_attack_type = it->extra;
                m_attack_skill = it->name;
                break;
            }
        }
    }
#endif
    int at = 1;
    int action_type = 1;    // 1攻击 2治疗， 3无
    //士气超过100，可以放绝招了
    if (m_shiqi >= 100 && m_speSkill.get())
    {
        m_real_attack_type = m_speSkill->attack_type;
        m_special_attack = m_shiqi;
        m_special_attack_fac = m_speSkill->damage_fac;
        //施放绝技后剩余的怒气
        m_shiqi =  m_speSkill->casted_shiqi + m_combat_attribute.org_shiqi();

        at = m_attack_type2;
        action_type = m_speSkill->action_type;
        m_action_obj.push_back( Pair("skill", m_speSkill->name) );
        m_action_obj.push_back( Pair("sid", m_speSkill->id) );
    }
    else
    {
        //cout<<"normal attack ... shiqi "<<m_shiqi<<endl;
        //士气不到100，使用单体攻击
        m_real_attack_type = range_single;
        m_special_attack = 0;

        at = m_attack_type3;
    }
    int ret = ATTACK_RESULT_SUCCESS;

    if (action_type == 1)
    {
        switch (m_real_attack_type)
        {
            case range_none:
                break;
            case range_single:  //单体
                ret = AttackArmy_Single();
                break;
            case range_chuantou://穿透
                ret = AttackArmy_Row();
                break;
            case range_fenlie:  //分裂
                ret = AttackArmy_Side();
                break;
            case range_around:  //眦邻
                ret = AttackArmy_Round();
                break;
            case range_three:   //固定3个
                ret = AttackArmy_ByStep();
                break;
            case range_all:     //全体
                ret = AttackArmy_All();
                break;
            default:
                ret = AttackArmy_Single();
                break;
        }
        //特殊攻击吸血效果
        if (m_special_attack && m_speSkill.get() && m_speSkill.get()->xixue_percent)
        {
            int add_hp = m_the_damage_total * m_speSkill.get()->xixue_percent / 100;
            if (add_hp)
            {
                this->Addhp(add_hp);
                this->UpdateObj();
                //治疗信息
                json_spirit::Object obj;
                obj.push_back( Pair("re", ATTACK_RESULT_HEAL) );
                obj.push_back( Pair("g", this->GetObj()) );
                obj.push_back( Pair("heal", add_hp) );
                m_result_list.push_back(obj);
            }
        }
    }
    else if (action_type == 2)
    {
        switch (m_real_attack_type)
        {
            default:
            case range_single:    //己方单体治疗
            case range_self_single:
                ret = HealOne();
                break;
            case range_all:        //己方全体治疗
            case range_self_all:
                ret = HealAll();
                break;
        }
    }
    if (m_special_attack && isLive())
    {
        //自己
        for (std::list<skillEffect>::iterator it = m_speSkill->self_effects.begin(); it != m_speSkill->self_effects.end(); ++it)
        {
            if (it->chance >= 1000 || my_random(1, 1000) <= it->chance)
            {
                switch (it->bbuff->type)
                {
                    //增加怒气
                    case effect_add_nuqi:
                        AddShiqi(it->value);
                        break;
                    //降低怒气
                    case effect_sub_nuqi:
                        AddShiqi(-it->value);
                        break;
                    //增加气血
                    case effect_add_hp:
                        {
                            //int real = Addhp(it->value);
                            Addhp(it->value);
                            json_spirit::Object obj;
                            obj.push_back( Pair("re", ATTACK_RESULT_HEAL) );                            
                            obj.push_back( Pair("heal", it->value) );
                            obj.push_back( Pair("g", GetObj()) );
                            m_result_list.push_back(obj);
                            break;
                        }
                    //百分比加气血
                    case effect_add_hp_percent:
                        {
                            int heal = it->value * MaxHp() / 100;
                            Addhp(heal);
                            json_spirit::Object obj;
                            obj.push_back( Pair("re", ATTACK_RESULT_HEAL) );                            
                            obj.push_back( Pair("heal", heal) );
                            obj.push_back( Pair("g", GetObj()) );
                            m_result_list.push_back(obj);
                            break;
                        }
                    default:
                        //增加buff
                        //cout<<m_id<<" add self buff "<<it->bbuff->type<<endl;
                        addBuff(it->bbuff, this, it->value, it->lastRound);
                        break;
                }
            }
        }

        if (m_speSkill->enermy_effects.size())
        {
            Army * army = GetArmy().GetEnermy();
            std::list<iGeneral*> glist;
            army->GetAllGenerals(0, glist);
            //敌方全体的
            for (std::list<skillEffect>::iterator it = m_speSkill->enermy_effects.begin(); it != m_speSkill->enermy_effects.end(); ++it)
            {
#ifdef TEST_SERVER
                int chance = 0;
                if (it->chance >= 1000)
                {
                    //chance = 0;
                }
                else
                {
                    chance = my_random(1, 1000);
                    cout<<"add all enermy buff "<<it->bbuff->type<<",random(1-1000)->"<<chance<<" vs "<<it->chance<<endl;
                }
                if (chance <= it->chance)
#else
                if (it->chance >= 1000 || my_random(1, 1000) <= it->chance)
#endif
                {
                    switch (it->bbuff->type)
                    {
                        //增加怒气
                        case effect_add_nuqi:
                            {
                                std::list<iGeneral*>::iterator it2 = glist.begin();
                                while (it2 != glist.end())
                                {
                                    if ((*it2)->isLive())
                                    {
                                        (*it2)->AddShiqi(it->value);
                                        //加怒气信息
                                        json_spirit::Object obj;
                                        obj.push_back( Pair("re", ATTACK_RESULT_ADD_NUQI) );
                                        obj.push_back( Pair("g", (*it2)->GetObj()) );
                                        obj.push_back( Pair("nuqi", it->value) );
                                        m_result_list.push_back(obj);
                                    }
                                    ++it2;
                                }
                            }
                            break;
                        //降低怒气
                        case effect_sub_nuqi:
                            {
                                std::list<iGeneral*>::iterator it2 = glist.begin();
                                while (it2 != glist.end())
                                {
                                    if ((*it2)->isLive())
                                    {
                                        json_spirit::Object obj;
                                        //降怒气暴击
                                        int nuqi = it->value;
                                        int baoji = Set_BaojiFlag(*(*it2));
                                        if (baoji)
                                        {
                                            nuqi = (100+baoji)*nuqi/100;
                                            obj.push_back( Pair("cri", baoji) );
                                        }
                                        (*it2)->AddShiqi(-nuqi);
                                        //加怒气信息
                                        obj.push_back( Pair("re", ATTACK_RESULT_SUB_NUQI) );
                                        obj.push_back( Pair("g", (*it2)->GetObj()) );
                                        obj.push_back( Pair("nuqi", nuqi) );
                                        m_result_list.push_back(obj);
                                    }
                                    ++it2;
                                }
                            }
                            break;
                        default:
                            //增加buff
                            {
                                std::list<iGeneral*>::iterator it2 = glist.begin();
                                while (it2 != glist.end())
                                {
                                    if ((*it2)->isLive())
                                    {
                                        (*it2)->addBuff(it->bbuff, this, it->value, it->lastRound);
                                    }
                                    ++it2;
                                }
                            }
                            break;
                    }
                }
            }
        }

        //己方全体的
        if (m_speSkill->self_all_effects.size())
        {
            std::list<iGeneral*> glist;
            m_army->GetAllGenerals(0, glist);
            for (std::list<skillEffect>::iterator it = m_speSkill->self_all_effects.begin(); it != m_speSkill->self_all_effects.end(); ++it)
            {
                if (it->chance >= 1000 || my_random(1, 1000) <= it->chance)
                {
                    switch (it->bbuff->type)
                    {
                        //增加怒气
                        case effect_add_nuqi:
                            {
                                std::list<iGeneral*>::iterator it2 = glist.begin();
                                while (it2 != glist.end())
                                {
                                    //不加自己的怒气
                                    if ((*it2) != this && (*it2)->isLive())
                                    {
                                        json_spirit::Object obj;
                                        //增加怒气也会暴击
                                        int baoji = checkBaoji();
                                        int nuqi = it->value;
                                        if (baoji)
                                        {
                                            nuqi = (100+baoji)*nuqi/100;
                                            obj.push_back( Pair("cri", baoji) );
                                        }
                                        (*it2)->AddShiqi(nuqi);
                                        //加怒气信息                                        
                                        obj.push_back( Pair("re", ATTACK_RESULT_ADD_NUQI) );
                                        obj.push_back( Pair("g", (*it2)->GetObj()) );
                                        obj.push_back( Pair("nuqi", nuqi) );
                                        m_result_list.push_back(obj);
                                    }
                                    ++it2;
                                }
                            }
                            break;
                        //降低怒气
                        case effect_sub_nuqi:
                            {
                                std::list<iGeneral*>::iterator it2 = glist.begin();
                                while (it2 != glist.end())
                                {
                                    if ((*it2)->isLive())
                                    {
                                        (*it2)->AddShiqi(-it->value);
                                        //降低怒气信息
                                        json_spirit::Object obj;
                                        obj.push_back( Pair("re", ATTACK_RESULT_SUB_NUQI) );
                                        obj.push_back( Pair("g", (*it2)->GetObj()) );
                                        obj.push_back( Pair("nuqi", it->value) );
                                        m_result_list.push_back(obj);
                                    }
                                    ++it2;
                                }
                            }
                            break;
                        default:
                            //增加buff
                            {
                                std::list<iGeneral*>::iterator it2 = glist.begin();
                                while (it2 != glist.end())
                                {
                                    if ((*it2)->isLive())
                                    {
                                        (*it2)->addBuff(it->bbuff, this, it->value, it->lastRound);
                                    }
                                    ++it2;
                                }
                            }
                            break;
                    }
                }
            }
        }
    }
    if (!m_special_attack)
    {
        if (ret == ATTACK_RESULT_SUCCESS || ret == ATTACK_RESULT_GEDANG || ret == ATTACK_RESULT_SHIPO)
        {
            //攻击成功加士气
            AddShiqi(m_nuqi_add, false);
        }
        else if (ret == ATTACK_RESULT_BAOJI)
        {
            //攻击成功加士气
            AddShiqi(m_nuqi_add_baoji, false);
        }
        else
        {
            
        }
    }
    UpdateObj();
    m_action_obj.push_back( Pair("g", GetObj()) );
    m_action_obj.push_back( Pair("act", action_type) );    // 攻击|加血|加怒气
    m_action_obj.push_back( Pair("dtype", m_damage_type) );    // 普通，策略
    m_action_obj.push_back( Pair("atype", m_real_attack_type) );    // 攻击方式:单个，一排，一列，全体，每排第一个,己方全体
    m_action_obj.push_back( Pair("at", at) );    // 1近战  2远程
    m_action_obj.push_back( Pair("injure", m_damage_type2) );// 伤害类型-刀砍

    m_action_obj.push_back( Pair("list", m_result_list) );

    m_army->getBuffChange(blist);
    if (m_army->GetEnermy())
    {
        m_army->GetEnermy()->getBuffChange(blist);
    }
    m_action_obj.push_back( Pair("buffList", blist) );

    GetArmy().GetCombat().AppendResult(m_action_obj);

    //m_army->sendBuff();
    //m_army->GetEnermy()->sendBuff();
    
    m_special_attack = 0;
    //cout<<"action end ... shiqi "<<m_shiqi<<endl;
    return ret;
}

//获得部队
Army& General::GetArmy() const
{
    return *m_army;
}

void General::setArmy(Army* a)
{
    m_army = a;
    if (m_army)
    {
        m_type = m_army->isAttacker() ? 1 : 2;
        if (m_charactor && m_army->GetCombatHandle())
        {
            switch (m_army->GetCombatHandle()->type())
            {
                case combat_boss:
                    //cout<<"General::setArmy set boss inspired "<<m_combat_attribute.boss_inspired()<<endl;
                    m_combat_attribute.total_inspired(m_combat_attribute.boss_inspired());
                    break;
                case combat_race:
                    m_combat_attribute.total_inspired( m_combat_attribute.race_inspired() );
                    break;
                case combat_camp_race:
                    m_combat_attribute.total_inspired( m_combat_attribute.camp_inspired() );
                    break;
                case combat_guard:
                    if (m_type == 2)//护纲鼓舞只在防守有效
                    {
                        m_combat_attribute.total_inspired( m_combat_attribute.guard_inspired() );
                    }
                    break;
                case combat_maze:    //迷宫战斗
                case combat_maze_boss://不处理
                    //m_combat_attribute.total_inspired( m_combat_attribute.maze_inspired() );
                    break;
                default:
                    m_combat_attribute.total_inspired(0);
                    break;
            }
        }
    }
    else
    {
        m_type = 0;
    }
    
}

//保存
int General::Save()
{
    return 0;
}

//返回id
int General::Id()
{
    return m_id;
}

json_spirit::Object& General::GetOrgObj()
{
    return m_org_obj;
}

json_spirit::Object& General::GetObj()
{
    if (m_cur_obj.size() == 0)
    {
        UpdateObj();
    }
    return m_cur_obj;
}

void General::GenOrgObj()
{
    updateSpecial();
    json_spirit::Array attrs;
    for (int i = 0; i < special_attack_max; ++i)
    {
        attrs.push_back(m_special_attacks[i]);
        attrs.push_back(m_special_resists[i]);
    }
    //武将信息
    m_org_obj.clear();
    m_org_obj.push_back ( Pair("id", m_id) );
    m_org_obj.push_back ( Pair("spic", m_spic) );
    m_org_obj.push_back ( Pair("name", m_name) );
#if 0
    m_org_obj.push_back ( Pair("nickname", m_nickname) );
#endif
    m_org_obj.push_back ( Pair("level", m_level) );
    m_org_obj.push_back ( Pair("color", m_color) );
    m_org_obj.push_back ( Pair("maxHp", m_hp_org) );
    //战神兽奄奄一息
    if (m_army && m_army->GetCombatHandle()
        && m_army->GetCombatHandle()->type() == combat_boss
        && !(m_army->isAttacker())
        && m_hp_now < (m_hp_org  * 5 / 100))
    {
        m_org_obj.push_back ( Pair("hp", m_hp_org  * 5 / 100) );
    }
    else
    {
        m_org_obj.push_back ( Pair("hp", m_hp_now) );
    }
    //m_org_obj.push_back ( Pair("hp", m_hp_now) );
    m_org_obj.push_back ( Pair("position", m_pos) );
    m_org_obj.push_back ( Pair("morale", m_shiqi) );

    if (m_speSkill.get())
    {
        m_org_obj.push_back ( Pair("spe_name", m_speSkill->name) );
    }

    //武将兵种
    json_spirit::Object soldier;

    soldier.push_back ( Pair("type", m_base_stype) );
    soldier.push_back ( Pair("spic", m_soldier_spic) );    
    m_org_obj.push_back ( Pair("soldier", soldier) );

    INFO(m_charactor<<"["<<m_pos<<"]"<<m_stype);
    //m_combat_attribute.print();

    UpdateObj();
    return;
}

json_spirit::Object& General::UpdateObj()
{
    //战斗过程数据变化
    m_cur_obj.clear();
    m_cur_obj.push_back ( Pair("id", m_id) );
    m_cur_obj.push_back ( Pair("type", m_army && m_army->isAttacker() ? 1 : 2) );
    m_cur_obj.push_back ( Pair("position", m_pos) );
    //战神兽奄奄一息
    if (m_army && m_army->GetCombatHandle()
        && m_army->GetCombatHandle()->type() == combat_boss
        && !(m_army->isAttacker())
        && (m_hp_now < (m_hp_org * 5 / 100)))
    {
        m_cur_obj.push_back ( Pair("hp", m_hp_org * 5 / 100) );
    }
    else
    {
        m_cur_obj.push_back ( Pair("hp", m_hp_now) );
    }
    m_cur_obj.push_back ( Pair("morale", m_shiqi) );
    return m_cur_obj;
}

void General::addBuff(baseBuff* bb, iGeneral* from, int value, int last)    //增加一个buff
{
    if (bb && last > 0 && value > 0)
    {
        if (debuff_taobing == bb->type)
        {
            value = from->taobingDamage(*this);
        }
        else
        if (debuff_chaos == bb->type)
        {
            if (getBuffValue(buff_no_chaos) > 0 || resist(special_attack_chaos) > 10000)
            {
                return;
            }
            if (last > m_chaos_flag)
            {
                m_chaos_flag = last;
            }
        }

        for (std::list<boost::shared_ptr<Buff> >::iterator it = m_buff_list.begin(); it != m_buff_list.end(); ++it)
        {
            //来自同一个人的同类型buff，延长回合数
            if (it->get() && (*it)->bbuff == bb && (*it)->from_who == from)
            {
                if (last >= (*it)->leftRound)
                {
                    (*it)->leftRound = last;
                    (*it)->enable = false;
                    m_buff_changed = true;
                    return;
                }
                else
                {
                    return;
                }
            }
        }
        Buff *pb = new Buff;
        pb->enable = false;
        pb->bbuff = bb;
        pb->value = value;
        pb->leftRound = last;
        pb->who = this;
        pb->from_who = from;

        boost::shared_ptr<Buff> b(pb);

        m_buff_list.push_back(b);

        from->addGeneratedBuff(b);

        m_buff_changed = true;
    }
}

void General::removeBuff(boost::shared_ptr<Buff> b)
{
    for (std::list<boost::shared_ptr<Buff> >::iterator it = m_buff_list.begin(); it != m_buff_list.end(); ++it)
    {
        if (it->get() == b.get())
        {
            m_buff_changed = true;
            m_buff_list.erase(it);
            return;
        }
    }
}

void General::addGeneratedBuff(boost::shared_ptr<Buff> b)    //增加一个产生的buff
{
    m_generate_buff_list.push_back(b);
}

void General::updateGenerateBuff()
{
    m_baoji_flag = 0;
    for (std::list<boost::shared_ptr<Buff> >::iterator it = m_generate_buff_list.begin(); it != m_generate_buff_list.end(); ++it)
    {
        if (it->get())
        {
            if (!(*it)->enable)
            {
                (*it)->enable = true;
            }
            else
            {
                if ((*it)->leftRound > 0)
                {
                    (*it)->leftRound--;
                    (*it)->who->setBuffChange();
                }
                else
                {
                    //(*it)->who->m_buff_changed = true;
                    (*it)->who->removeBuff(*it);
                    it = m_generate_buff_list.erase(it);
                }
            }
        }
    }
}

int General::getBuffValue(int type) const //获取一类buff的效果
{
    int value = 0;
    for (std::list<boost::shared_ptr<Buff> >::const_iterator it = m_buff_list.begin(); it != m_buff_list.end(); ++it)
    {
        if (it->get() && (*it)->bbuff && (*it)->leftRound > 0)
        {
            if ((*it)->bbuff->type == type)
            {
                value += (*it)->value;
            }
            else if (type < 100 && (*it)->bbuff->type == (100 + type))
            {
                value -= (*it)->value;
            }
        }
    }
    return value;
}

void General::updateBuff()    //每个回合更新buff
{
    m_buff_changed = m_buff_list.size() > 0;
    for (std::list<boost::shared_ptr<Buff> >::iterator it = m_buff_list.begin(); it != m_buff_list.end(); ++it)
    {
        if (it->get())
        {
            if ((*it)->leftRound > 0)
            {
                --(*it)->leftRound;

                //通知buff移除
            }
            else
            {
                //删除buff
                it = m_buff_list.erase(it);
            }
        }
    }
}

void General::sendBuff()
{
    if (m_buff_changed)
    {
        m_buff_changed = false;
        json_spirit::Object b;
        b.push_back( Pair("cmd", "buff") );
        json_spirit::Object g = GetObj();
        json_spirit::Array blist;
        for (std::list<boost::shared_ptr<Buff> >::iterator it = m_buff_list.begin(); it != m_buff_list.end(); ++it)
        {
            if (it->get())
            {
                json_spirit::Object obj;
                obj.push_back( Pair("t", (*it)->bbuff->type) );
                obj.push_back( Pair("v", (*it)->value) );
                obj.push_back( Pair("r", (*it)->leftRound) );
                blist.push_back(obj);
            }
        }
        g.push_back( Pair("blist", blist) );
        b.push_back( Pair("g", g) );
        GetArmy().GetCombat().AppendResult(b);
    }
}

void General::getBuffChange(json_spirit::Array& blist)
{
    if (m_buff_changed)
    {
        m_buff_changed = false;
        json_spirit::Object g = GetObj();
        json_spirit::Array bflist;
        for (std::list<boost::shared_ptr<Buff> >::iterator it = m_buff_list.begin(); it != m_buff_list.end(); ++it)
        {
            if (it->get())
            {
                json_spirit::Object obj;
                obj.push_back( Pair("t", (*it)->bbuff->type) );
                obj.push_back( Pair("v", (*it)->value) );
                obj.push_back( Pair("r", (*it)->leftRound) );
                if ((*it)->enable == false)
                {
                    obj.push_back( Pair("a", 1) );
                }
                bflist.push_back(obj);
            }
        }
        g.push_back( Pair("blist", bflist) );
        blist.push_back(g);
    }
}

void General::setBuffChange()
{
    m_buff_changed = true;
}

void General::clearBuff()
{
    m_buff_list.clear();

    //士气
    m_shiqi = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();    //受伤加的士气
    m_shiqi_old = DEFAULT_SHIQI + m_combat_attribute.init_shiqi();

    m_chaos_flag = 0;
    m_baoji_flag = 0;
    m_weihe_flag = 0;
}

void General::updateSpecial()
{
    for (int i = 0; i < special_attack_max; ++i)
    {
        m_special_attacks[i] = m_combat_attribute.special_attack(i);
        m_special_resists[i] = m_combat_attribute.special_resist(i);

        if (m_army)
        {
            int level = GetArmy().level();
            if (level > 0)
            {
                //m_special_attacks[i] += (m_combat_attribute.special_attack_level(i) * 50 / GetArmy().level());
                //m_special_resists[i] += (m_combat_attribute.special_resist_level(i) * 50 / GetArmy().level());
                m_special_attacks[i] += (m_combat_attribute.special_attack_level(i) * 50 / 40);
                m_special_resists[i] += (m_combat_attribute.special_resist_level(i) * 50 / 40);
            }
        }
    }
}

