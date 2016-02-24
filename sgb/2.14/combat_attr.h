#pragma once

#include "combat_def.h"
//#include "baoshi.h"
#include <string.h>
#include "newbaoshi.h"

class iGeneral;

#define my_max(a,b) ((a) > (b) ? (a) : (b))

extern volatile int g_print_debug_info;

//���������˺�����
const int iBaojiMoreDamage = 50;

//��Ѫ���� ǧ��֮
const int iXiXueGailv = 100;

struct combatSpeSkill
{
    int id;                //�ؼ�id
    std::string name;    //�ؼ�����

    int type;            //�ؼ���� 1���˺����� 2������ת��
    int extra;            //�˺�����ϵ�� ���� ������ʽ
                        //�˺�����ϵ�� 3 ��ʾ3��  2 ��ʾ2�� 1��ʾ1.5��
                        //������ʽ �뿴 enum target_range
                        /*
                        enum target_range
                        {
                            range_single = 1,    //����
                            range_chuantou,     //��͸
                            range_fenlie,        //����
                            range_around,        //����
                            range_single_back,    //���� (����)
                            range_three,        //����
                            range_all,            //ȫ�� (�Է�)
                            range_self_single,    //���� (����)
                            range_self_all        //ȫ�� (����)
                        };
                        */
    int chance;            //����
};

struct baseState;
struct newCharStates;
struct npcStrongholdStates;
struct charSkill;

 //��ս���е�����
struct combatAttribute
{
private:
    //�˺����� - ���������ڵ�һ��!!!
    int m_sub_damage[2];    // {�����ܵ���ͨ�˺��ٷֱ�,�����ܵ��Ĳ����˺��ٷֱ�}
    //�Ա��ַ����������ܵ��ض����ֵ��˺��ٷֱ�
    int m_sub_damage_from[5];
    //����+״̬���ӵĶԱ��ֵĻ�������/Ѫ����
    int m_skill_add[skill_add_max];
    //�Ա��ֿ��ƣ����Ӷ��ض����ֵ��˺��ٷֱ�
    int m_more_damage_to[5];
    int m_death_fight;     //��ս

    //����Ѫ��
    int m_hp_stype[5];
    //���ֹ���
    int m_attack_stype[5];
    //�������
    int m_wufang_stype[5];
    //���ֲ߷�
    int m_cefang_stype[5];
    //���ֿ��Ƽӳ�
    int m_moredamage_stype[5];

    int m_weak;        //����״̬�������������˺�

    //���⹥��Ч��
    int m_special_attack[special_attack_max][2];
    int m_special_attack_level[special_attack_max];

    //���⹥������
    int m_special_resist[special_attack_max];
    int m_special_resist_level[special_attack_max];

    int m_boss_inspired;    //bossս������
    int m_race_inspired;    //��������
    int m_camp_inspired;    //��Ӫվ����
    int m_guard_inspired;    //���ٹ���
    int m_maze_inspired;    //�Թ�����
    int m_total_inspired;    //�ۺ������Ĺ���

    //2.14
    int m_org_inspired;        //�Դ�����Ч��
    int m_org_shiqi;            //�Դ�ŭ��

    int m_init_shiqi;        //��ʼʿ��
    int m_casted_shiqi;    //ʩ�ž������ʿ��

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
        return m_race_inspired;    //��������
    }
    void race_inspired(int ins)
    {
        m_race_inspired = ins;    //��������
        m_enable = 1;
    }
    int camp_inspired() const
    {
        return m_camp_inspired;    //��Ӫվ����
    }
    void camp_inspired(int ins)
    {
        m_camp_inspired = ins;    //��Ӫվ����
        m_enable = 1;
    }
    int guard_inspired() const
    {
        return m_guard_inspired;    //���ٹ���
    }
    void guard_inspired(int ins) 
    {
         m_guard_inspired = ins;    //���ٹ���
         m_enable = 1;
    }
    int maze_inspired() const
    {
        return m_maze_inspired;    //�Թ�����
    }
    void maze_inspired(int ins)
    {
        m_maze_inspired = ins;    //�Թ�����
        m_enable = 1;
    }
    int total_inspired() const
    {
        return m_total_inspired + m_org_inspired;    //�ۺ������Ĺ���
    }
    void total_inspired(int ins)
    {
        m_total_inspired = ins;    //�ۺ������Ĺ���
        m_enable = 1;
    }
    int init_shiqi() const
    {
        return m_init_shiqi + m_org_shiqi;
    }

    
    int org_inspired() { return m_org_inspired; }        //�Դ�����Ч��
    int org_shiqi() { return m_org_shiqi; }            //�Դ�ŭ��

    //�Դ�����Ч��
    void org_inspired(int v)
    {
        m_org_inspired = v;
        m_enable = 1;
    }
    //�Դ�ŭ��
    void org_shiqi(int v)
    {
        m_org_shiqi = v;
        m_enable = 1;
    }

    //��ֵ 
    combatAttribute& operator=(const combatAttribute &t1);
    // +=
    combatAttribute &operator+=(const combatAttribute &t1);

    // +=
    combatAttribute &operator+=(const newBaoshi &t1);

    friend int updateCombatAttribute(boost::shared_ptr<baseState>* state, int stateNums, combatAttribute& ct);
    
    friend int updateCombatAttribute(newCharStates& states, std::map<int,boost::shared_ptr<charSkill> >& skl, combatAttribute& ct);
    //��������״̬����ս������
    friend int updateCombatAttribute(npcStrongholdStates& states, combatAttribute& ct);
    
    friend struct CharTrainings;

};
 
enum buff_type_enum
{
    buff_baoji = 1,         //����������
    buff_gedang = 2,        //��������
    buff_shipo = 3,        //����ʶ����
    buff_dodge = 4,        //����������
    buff_mingzhong = 5,    //����������
    buff_fangyu = 6,        //����������
    buff_gongji = 7,        //����������
    buff_no_chaos = 8,        //���߻���
    buff_pu_fang = 11,        //��ͨ����
    buff_ce_fang = 12,        //���Է���
    
    debuff_baoji = 101,    //���ͱ�����
    debuff_gedang = 102,    //���͸���
    debuff_shipo = 103,    //����ʶ����
    debuff_dodge = 104,    //���Ͷ�����
    debuff_miss = 105,        //����������

    debuff_fangyu = 106,    //���ͷ�����
    debuff_gongji = 107,    //���͹�����
    debuff_pu_fang = 111,    //������ͨ����
    debuff_ce_fang = 112,    //���Ͳ��Է���
    
    debuff_chaos = 109,    //����
    debuff_taobing = 110,    //�ӱ�

    effect_sub_nuqi = 1001,     //����ŭ��
    effect_add_nuqi = 1002,    //����ŭ��
    effect_add_hp = 1003,        //������Ѫ
    effect_add_hp_percent = 1004,    //������Ѫ�ٷֱ�
    effect_soul_link = 1005,    //�������
};

//����buff
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

//����Ч��
struct skillEffect
{
    baseBuff* bbuff;
    int value;        // buffЧ��ֵ��(����:ǧ��֮��)
    int lastRound;    // �����غ�
    int chance;        // �������� ǧ��֮��
};

//�佫���⼼��
struct specialSkill
{
    int id;              // Ψһid
    int damage_fac;     // �˺�ϵ��
    int action_type;    // 1����  2���� 0��
    int attack_type;    // �������� 1������Ŀ�� 2��һ�� 3��һ�� 4��Ŀ�꼰��Χ 5���̶�����Ŀ��6��ȫ��
    int injure_type;    // �˺�����  ���� ���� ���� ħ��
    int remote_attack;  // 1Զ�̹���  0��ս
    int attack_score;   //ս������

    int casted_shiqi;    //ʩ�ż��ܺ�ʣ��ʿ��

    int xixue_percent;  //��Ѫ�ٷֱ�

    std::string name;    //��������
    std::string memo;    //��������

    std::list<skillEffect> target_effects;    //���ܲ�����buff��debuff
    std::list<skillEffect> self_effects;    //���ܲ�����buff��debuff
    std::list<skillEffect> enermy_effects;    //���ܲ�����buff��debuff
    std::list<skillEffect> self_all_effects;    //���ܲ�����buff��debuff

    bool parseEffect(const std::string& effect);
};

//���ϵ�buff
struct Buff
{
    baseBuff* bbuff;
    iGeneral* who;
    iGeneral* from_who;
    int value;
    int leftRound;
    bool enable;
};

