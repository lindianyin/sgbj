#pragma once

#include <string>
#include <map>
#include <list>
#include <boost/cstdint.hpp>

#include "boost/smart_ptr/shared_ptr.hpp"
#include <boost/enable_shared_from_this.hpp>

#include <vector>
#include "json_spirit.h"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include "skill.h"

#define my_max(a,b) ((a) > (b) ? (a) : (b))

extern volatile int g_print_debug_info;

struct combatAttribute;

//根据玩家技能更新战斗属性
extern void updateCombatAttribute(std::map<int,boost::shared_ptr<CharSkill> >& skl, combatAttribute& ct, int level);
//根据关卡技能更新战斗属性
extern void updateCombatAttribute(std::vector<int>& skl, int level, combatAttribute& ct);

//在战斗中的属性
struct combatAttribute
{
private:
    int m_skill_rank_add[SKILL_VALUE_MAX];//技能增加的对手牌的伤害加成
    int m_skill_rank_resist[SKILL_VALUE_MAX];//技能增加的对手牌的伤害减免
    int m_attack_add;
    int m_defense_add;
    int m_magic_add;

    int m_damage_per;//伤害加成
    int m_damage_resist_per;//减伤加成

    int m_boss_inspired;    //boss战斗鼓舞

    int m_enable;

public:
    combatAttribute();
    void print(bool p = false) const;

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
    void set_damage_per(int percent)
    {
        m_damage_per = percent;
        m_enable = 1;
    }
    void set_damage_resist_per(int percent)
    {
        m_damage_resist_per = percent;
        m_enable = 1;
    }
    void clear();
    int skill_rank_add(int type) const {return m_skill_rank_add[type-1];}
    int skill_rank_resist(int type) const {return m_skill_rank_resist[type-1];}
    int skill_attack_add() const {return m_attack_add;}
    int skill_defense_add() const {return m_defense_add;}
    int skill_magic_add() const {return m_magic_add;}
    int damage_per() const {return m_damage_per;}
    int damage_resist_per() const {return m_damage_resist_per;}
    int boss_inspired() const
    {
        return m_boss_inspired;
    }
    void boss_inspired(int ins)
    {
        m_boss_inspired = ins;
        m_enable = 1;
    }
    //赋值
    combatAttribute& operator=(const combatAttribute &t1);
    // +=
    combatAttribute &operator+=(const combatAttribute &t1);

    friend void updateCombatAttribute(std::map<int,boost::shared_ptr<CharSkill> >& skl, combatAttribute& ct, int level);
    friend void updateCombatAttribute(std::vector<int>& skl, int level, combatAttribute& ct);

};

