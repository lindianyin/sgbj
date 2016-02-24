#pragma once

#include "combat_def.h"
//#include "baoshi.h"
#include <string.h>
#include "newbaoshi.h"

class iGeneral;

#define my_max(a,b) ((a) > (b) ? (a) : (b))

extern volatile int g_print_debug_info;

//暴击增加伤害比例
const int iBaojiMoreDamage = 50;

//吸血概率 千分之
const int iXiXueGailv = 100;

struct combatSpeSkill
{
    int id;                //特技id
    std::string name;    //特技名称

    int type;            //特技类别 1、伤害翻倍 2、攻击转变
    int extra;            //伤害翻倍系数 或者 攻击方式
                        //伤害翻倍系数 3 表示3倍  2 表示2倍 1表示1.5倍
                        //攻击方式 请看 enum target_range
                        /*
                        enum target_range
                        {
                            range_single = 1,    //单体
                            range_chuantou,     //穿透
                            range_fenlie,        //分裂
                            range_around,        //眦邻
                            range_single_back,    //单体 (后排)
                            range_three,        //引导
                            range_all,            //全体 (对方)
                            range_self_single,    //单体 (己方)
                            range_self_all        //全体 (己方)
                        };
                        */
    int chance;            //概率
};

struct baseState;
struct newCharStates;
struct npcStrongholdStates;
struct charSkill;

 //在战斗中的属性
struct combatAttribute
{
private:
    //伤害减免 - 这个必须放在第一个!!!
    int m_sub_damage[2];    // {减少受到普通伤害百分比,减少受到的策略伤害百分比}
    //对兵种防御，减免受到特定兵种的伤害百分比
    int m_sub_damage_from[5];
    //技能+状态增加的对兵种的基础攻防/血量等
    int m_skill_add[skill_add_max];
    //对兵种克制，增加对特定兵种的伤害百分比
    int m_more_damage_to[5];
    int m_death_fight;     //死战

    //兵种血量
    int m_hp_stype[5];
    //兵种攻击
    int m_attack_stype[5];
    //兵种物防
    int m_wufang_stype[5];
    //兵种策防
    int m_cefang_stype[5];
    //兵种克制加成
    int m_moredamage_stype[5];

    int m_weak;        //虚弱状态，按比例降低伤害

    //特殊攻击效果
    int m_special_attack[special_attack_max][2];
    int m_special_attack_level[special_attack_max];

    //特殊攻击抗性
    int m_special_resist[special_attack_max];
    int m_special_resist_level[special_attack_max];

    int m_boss_inspired;    //boss战斗鼓舞
    int m_race_inspired;    //竞技鼓舞
    int m_camp_inspired;    //阵营站鼓舞
    int m_guard_inspired;    //护纲鼓舞
    int m_maze_inspired;    //迷宫鼓舞
    int m_total_inspired;    //综合起来的鼓舞

    //2.14
    int m_org_inspired;        //自带鼓舞效果
    int m_org_shiqi;            //自带怒气

    int m_init_shiqi;        //初始士气
    int m_casted_shiqi;    //施放绝技后的士气

    int m_enable;

public:
    void print(bool p = false) const;
    
    combatAttribute();

    void load(const std::string& s);

    void save(std::string& s);

    void enable()
    {
        m_enable = 1;
    }
    void disable()
    {
        m_enable = 0;
        clear();
    }
    void clear();

    void set_damage_from(int type, int percent)
    {
        m_sub_damage_from[type-1] = percent;
        m_enable = 1;
    }
    void set_damage_to(int type, int percent)
    {
        m_more_damage_to[type-1] = percent;
    }

    void sub_damage(int type, int v)
    {
        if (type >= 1 && type <=2)
        {
            m_sub_damage[type-1] = v;
            m_enable = 1;
        }
    }
    int sub_damage(int type) const {return m_sub_damage[type];}
    int sub_damage_from(int base_stype) const {return m_sub_damage_from[base_stype-1];}
    int more_damage_to(int base_type) const {return m_more_damage_to[base_type-1];}
    int skill_add(int type) const {return m_skill_add[type];}
    int soul_add_attack(int base_type) const {return m_attack_stype[base_type-1];}
    int soul_add_wufang(int base_type) const {return m_wufang_stype[base_type-1];}
    int soul_add_cefang(int base_type) const {return m_cefang_stype[base_type-1];}
    int soul_add_hp(int base_type) const {return m_hp_stype[base_type-1];}
    int soul_stypeDamage(int base_type) const {return m_moredamage_stype[base_type-1];}
    int death_fight() const {return m_death_fight;}
    int special_attack(int type)  const
    {
        return m_special_attack[type][0];
    }
    int special_attack_level(int type) const
    {
        return m_special_attack_level[type];
    }
    void special_attack(int type, int value1)
    {
        m_special_attack[type][0] = value1;
    }
    int special_resist(int type) const
    {
        return m_special_resist[type];
    }
    int special_resist_level(int type) const
    {
        return m_special_resist_level[type];
    }
    void add_special_attack_level(int type, int value)
    {
        m_special_attack_level[type] += value;
        m_enable = 1;
    }
    void add_resist_level(int type, int value)
    {
        m_special_resist_level[type] += value;
        m_enable = 1;
    }
    void add_special_attack(int type, int value)
    {
        m_special_attack[type][0] += value;
        m_enable = 1;
    }
    void add_resist(int type, int value)
    {
        m_special_resist[type] += value;
        m_enable = 1;
    }
    void special_resist(int type, int value)
    {
        m_special_resist[type] = value;
        m_enable = 1;
    }

    int boss_inspired() const
    {
        return m_boss_inspired;
    }
    void boss_inspired(int ins)
    {
        m_boss_inspired = ins;
        m_enable = 1;
    }
    int race_inspired() const
    {
        return m_race_inspired;    //竞技鼓舞
    }
    void race_inspired(int ins)
    {
        m_race_inspired = ins;    //竞技鼓舞
        m_enable = 1;
    }
    int camp_inspired() const
    {
        return m_camp_inspired;    //阵营站鼓舞
    }
    void camp_inspired(int ins)
    {
        m_camp_inspired = ins;    //阵营站鼓舞
        m_enable = 1;
    }
    int guard_inspired() const
    {
        return m_guard_inspired;    //护纲鼓舞
    }
    void guard_inspired(int ins) 
    {
         m_guard_inspired = ins;    //护纲鼓舞
         m_enable = 1;
    }
    int maze_inspired() const
    {
        return m_maze_inspired;    //迷宫鼓舞
    }
    void maze_inspired(int ins)
    {
        m_maze_inspired = ins;    //迷宫鼓舞
        m_enable = 1;
    }
    int total_inspired() const
    {
        return m_total_inspired + m_org_inspired;    //综合起来的鼓舞
    }
    void total_inspired(int ins)
    {
        m_total_inspired = ins;    //综合起来的鼓舞
        m_enable = 1;
    }
    int init_shiqi() const
    {
        return m_init_shiqi + m_org_shiqi;
    }

    
    int org_inspired() { return m_org_inspired; }        //自带鼓舞效果
    int org_shiqi() { return m_org_shiqi; }            //自带怒气

    //自带鼓舞效果
    void org_inspired(int v)
    {
        m_org_inspired = v;
        m_enable = 1;
    }
    //自带怒气
    void org_shiqi(int v)
    {
        m_org_shiqi = v;
        m_enable = 1;
    }

    //赋值 
    combatAttribute& operator=(const combatAttribute &t1);
    // +=
    combatAttribute &operator+=(const combatAttribute &t1);

    // +=
    combatAttribute &operator+=(const newBaoshi &t1);

    friend int updateCombatAttribute(boost::shared_ptr<baseState>* state, int stateNums, combatAttribute& ct);
    
    friend int updateCombatAttribute(newCharStates& states, std::map<int,boost::shared_ptr<charSkill> >& skl, combatAttribute& ct);
    //根据三个状态更新战斗属性
    friend int updateCombatAttribute(npcStrongholdStates& states, combatAttribute& ct);
    
    friend struct CharTrainings;

};
 
enum buff_type_enum
{
    buff_baoji = 1,         //提升暴击率
    buff_gedang = 2,        //提升格挡率
    buff_shipo = 3,        //提升识破率
    buff_dodge = 4,        //提升闪避率
    buff_mingzhong = 5,    //提升命中率
    buff_fangyu = 6,        //提升防御力
    buff_gongji = 7,        //提升攻击力
    buff_no_chaos = 8,        //免疫混乱
    buff_pu_fang = 11,        //普通防御
    buff_ce_fang = 12,        //策略防御
    
    debuff_baoji = 101,    //降低暴击率
    debuff_gedang = 102,    //降低格挡率
    debuff_shipo = 103,    //降低识破率
    debuff_dodge = 104,    //降低躲闪率
    debuff_miss = 105,        //降低命中率

    debuff_fangyu = 106,    //降低防御力
    debuff_gongji = 107,    //降低攻击力
    debuff_pu_fang = 111,    //降低普通防御
    debuff_ce_fang = 112,    //降低策略防御
    
    debuff_chaos = 109,    //混乱
    debuff_taobing = 110,    //逃兵

    effect_sub_nuqi = 1001,     //降低怒气
    effect_add_nuqi = 1002,    //增加怒气
    effect_add_hp = 1003,        //增加气血
    effect_add_hp_percent = 1004,    //增加气血百分比
    effect_soul_link = 1005,    //灵魂锁链
};

//基础buff
struct baseBuff
{
    int type;

    std::string name;
    std::string memo;
    baseBuff(int id)
    :type(id)
    {
    }
};

//技能效果
struct skillEffect
{
    baseBuff* bbuff;
    int value;        // buff效果值，(概率:千分之几)
    int lastRound;    // 持续回合
    int chance;        // 触发概率 千分之几
};

//武将特殊技能
struct specialSkill
{
    int id;              // 唯一id
    int damage_fac;     // 伤害系数
    int action_type;    // 1攻击  2治疗 0无
    int attack_type;    // 攻击类型 1、单个目标 2、一列 3、一排 4、目标及周围 5、固定三个目标6、全体
    int injure_type;    // 伤害类型  刀伤 剑伤 火器 魔法
    int remote_attack;  // 1远程攻击  0近战
    int attack_score;   //战力评分

    int casted_shiqi;    //施放技能后，剩余士气

    int xixue_percent;  //吸血百分比

    std::string name;    //技能名字
    std::string memo;    //技能描述

    std::list<skillEffect> target_effects;    //技能产生的buff，debuff
    std::list<skillEffect> self_effects;    //技能产生的buff，debuff
    std::list<skillEffect> enermy_effects;    //技能产生的buff，debuff
    std::list<skillEffect> self_all_effects;    //技能产生的buff，debuff

    bool parseEffect(const std::string& effect);
};

//身上的buff
struct Buff
{
    baseBuff* bbuff;
    iGeneral* who;
    iGeneral* from_who;
    int value;
    int leftRound;
    bool enable;
};

