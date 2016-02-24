
#include "combat_attr.h"
#include "utils_all.h"
#include "singleton.h"

void combatAttribute::load(const std::string& s)
{
    json_spirit::mValue value;
    json_spirit::read(s, value);

    if (value.type() != json_spirit::obj_type)
    {
        return;
    }

    json_spirit::mObject& obj = value.get_obj();
    //技能+状态增加的对兵种的基础攻防/血量等
    read_int_array(obj, "skill_add", m_skill_rank_add, SKILL_VALUE_MAX);
    read_int_array(obj, "skill_resist", m_skill_rank_resist, SKILL_VALUE_MAX);

    READ_INT_FROM_MOBJ(m_attack_add,obj,"attack_add");
    READ_INT_FROM_MOBJ(m_defense_add,obj,"defense_add");
    READ_INT_FROM_MOBJ(m_magic_add,obj,"magic_add");

    READ_INT_FROM_MOBJ(m_damage_per,obj,"damage_per");
    READ_INT_FROM_MOBJ(m_damage_resist_per,obj,"damage_resist_per");

    READ_INT_FROM_MOBJ(m_boss_inspired, obj, "boss_ins");    //boss战斗鼓舞

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
        cout<<"ROYAL_FLUSH "<<m_skill_rank_add[0]<<","<<m_skill_rank_resist[0]<<endl;
        cout<<"SHELL_FLUSH "<<m_skill_rank_add[1]<<","<<m_skill_rank_resist[1]<<endl;
        cout<<"STRAIGHT_FLUSH "<<m_skill_rank_add[2]<<","<<m_skill_rank_resist[2]<<endl;
        cout<<"FOUR_OF_A_KIND "<<m_skill_rank_add[3]<<","<<m_skill_rank_resist[3]<<endl;
        cout<<"FULL_HOUSE "<<m_skill_rank_add[4]<<","<<m_skill_rank_resist[4]<<endl;
        cout<<"FLUSH "<<m_skill_rank_add[5]<<","<<m_skill_rank_resist[5]<<endl;
        cout<<"STRAIGHT "<<m_skill_rank_add[6]<<","<<m_skill_rank_resist[6]<<endl;
        cout<<"THREE_OF_A_KIND "<<m_skill_rank_add[7]<<","<<m_skill_rank_resist[7]<<endl;
        cout<<"TWO_PAIR "<<m_skill_rank_add[8]<<","<<m_skill_rank_resist[8]<<endl;
        cout<<"ONE_PAIR "<<m_skill_rank_add[9]<<","<<m_skill_rank_resist[9]<<endl;
        cout<<"HIGH_CARD "<<m_skill_rank_add[10]<<","<<m_skill_rank_resist[10]<<endl;

        cout<<"*******************************************"<<endl;
    }
}

void combatAttribute::save(std::string& s)
{
    json_spirit::Object obj;
    //技能+状态增加的对兵种的基础攻防/血量等
    write_int_array(obj, "skill_add", m_skill_rank_add, SKILL_VALUE_MAX);
    write_int_array(obj, "skill_resist", m_skill_rank_resist, SKILL_VALUE_MAX);

    obj.push_back( Pair("attack_add", m_attack_add) );
    obj.push_back( Pair("defense_add", m_defense_add) );
    obj.push_back( Pair("magic_add", m_magic_add) );

    obj.push_back( Pair("damage_per", m_damage_per) );
    obj.push_back( Pair("damage_resist_per", m_damage_resist_per) );

    obj.push_back( Pair("boss_ins", m_boss_inspired) );    //boss战斗鼓舞
    s = json_spirit::write(obj);
    //int m_enable;
}

combatAttribute::combatAttribute()
{
    //cout<<"combatAttribute()"<<endl;
    memset(&m_skill_rank_add, 0, sizeof(combatAttribute));
}

void combatAttribute::clear()
{
    memset(&m_skill_rank_add, 0, sizeof(combatAttribute));
}

//赋值
combatAttribute& combatAttribute::operator=(const combatAttribute &t1)
{
    //cout<<"combatAttribute = "<<endl;
    if (t1.m_enable == 0)
    {
        memset(&m_skill_rank_add, 0, sizeof(combatAttribute));
    }
    else
    {
        memcpy(m_skill_rank_add, t1.m_skill_rank_add, sizeof(combatAttribute));
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
    for (int i = 0; i < SKILL_VALUE_MAX; ++i)
    {
        m_skill_rank_add[i] += t1.m_skill_rank_add[i];
    }
    for (int i = 0; i < SKILL_VALUE_MAX; ++i)
    {
        m_skill_rank_resist[i] += t1.m_skill_rank_resist[i];
    }
    m_attack_add += t1.m_attack_add;
    m_defense_add += t1.m_defense_add;
    m_magic_add += t1.m_magic_add;
    m_damage_per += t1.m_damage_per;
    m_damage_resist_per += t1.m_damage_resist_per;
    m_boss_inspired += t1.m_boss_inspired;
    return *this;
}

//根据玩家技能更新战斗属性
void updateCombatAttribute(std::map<int,boost::shared_ptr<CharSkill> >& skl, combatAttribute& ct, int level)
{
    //鼓舞效果保留
    int m_boss_inspired = ct.m_boss_inspired;    //boss战斗鼓舞

    memset(&ct.m_skill_rank_add, 0, sizeof(combatAttribute));

    ct.m_boss_inspired = m_boss_inspired;

    std::map<int, boost::shared_ptr<CharSkill> >::iterator it = skl.begin();
    while (it != skl.end())
    {
        if (it->second.get() && it->second->m_base_skill.get() && it->second->m_state > 0
            && level >= it->second->m_base_skill->m_combat_level)
        {
            ct.m_enable = 1;
            //手牌加成
            for (int i = 0; i < SKILL_VALUE_MAX; ++i)
            {
                if (it->second->m_type == SKILL_TYPE_ACTIVE)
                {
                    ct.m_skill_rank_add[i] += (it->second->m_base_skill->m_base_rank_value[i] + it->second->m_base_skill->m_add_rank_value[i] * (it->second->m_level-1));
                }
                else if(it->second->m_type == SKILL_TYPE_PASSIVE)
                {
                    ct.m_skill_rank_resist[i] += (it->second->m_base_skill->m_base_rank_value[i] + it->second->m_base_skill->m_add_rank_value[i] * (it->second->m_level-1));
                }
            }
            //属性加成
            ct.m_attack_add += (it->second->m_base_skill->m_base_attack_value + it->second->m_base_skill->m_add_attack_value * (it->second->m_level-1));
            ct.m_defense_add += (it->second->m_base_skill->m_base_defense_value + it->second->m_base_skill->m_add_defense_value * (it->second->m_level-1));
            ct.m_magic_add += (it->second->m_base_skill->m_base_magic_value + it->second->m_base_skill->m_add_magic_value * (it->second->m_level-1));
        }
        ++it;
    }
    return;
}

//根据关卡技能更新战斗属性
void updateCombatAttribute(std::vector<int>& skl, int level, combatAttribute& ct)
{
    //鼓舞效果保留
    int m_boss_inspired = ct.m_boss_inspired;    //boss战斗鼓舞

    memset(&ct.m_skill_rank_add, 0, sizeof(combatAttribute));

    ct.m_boss_inspired = m_boss_inspired;

    std::vector<int>::iterator it = skl.begin();
    while (it != skl.end())
    {
        boost::shared_ptr<baseSkill> bs = Singleton<skillMgr>::Instance().getBaseSkill(*it);
        if (bs.get())
        {
            ct.m_enable = 1;
            //手牌加成
            for (int i = 0; i < SKILL_VALUE_MAX; ++i)
            {
                if (bs->m_type == SKILL_TYPE_ACTIVE)
                {
                    ct.m_skill_rank_add[i] += (bs->m_base_rank_value[i] + bs->m_add_rank_value[i] * (level-1));
                }
                else if(bs->m_type == SKILL_TYPE_PASSIVE)
                {
                    ct.m_skill_rank_resist[i] += (bs->m_base_rank_value[i] + bs->m_add_rank_value[i] * (level-1));
                }
            }
            //属性加成
            ct.m_attack_add += (bs->m_base_attack_value + bs->m_add_attack_value * (level-1));
            ct.m_defense_add += (bs->m_base_defense_value + bs->m_add_defense_value * (level-1));
            ct.m_magic_add += (bs->m_base_magic_value + bs->m_add_magic_value * (level-1));
        }
        ++it;
    }
    return;
}

