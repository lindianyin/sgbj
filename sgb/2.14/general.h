
#ifndef _GENERAL_H_
#define _GENERAL_H_

#include "combat_def.h"

#include <string>
#include "igeneral.h"

#include "combat_attr.h"

//ս����ʼʿ��
#define DEFAULT_SHIQI 50

class Army;
//����佫����
struct CharGeneralData;
//�ؿ��佫����
struct StrongholdGeneralData;
//��ҹؿ�
struct CharStrongholdData;

struct bossData;

struct char_maze_general;

struct char_zst_general;

//�佫��
class General:public iGeneral
{
public:
    General(Army* army, int pos, const CharGeneralData& gdata, int hurted);
    General(Army* army, int pos, const char_maze_general& m_gdata);
    General(Army* army, int pos, const char_zst_general& m_gdata);
    General(Army* army, int pos, const CharStrongholdData& cstronghold, const StrongholdGeneralData& gdata);
    General(Army* army, int pos, const bossData& bd);
    General(Army* army,
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
        int failure = 0,
        int duoshan = 0,
        int gedang = 0,
        int shipo = 0,
        int baoji = 0,
        int xixue = 0,
        int hunluan = 0,
        int weihe = 0,
        int podan = 0
    );
    virtual ~General() {};
    //�����ط������佫
    virtual int Attack(iGeneral& target);
    //������
    virtual int RecieveDamage(int damage, iGeneral& attacker, int attack_type, int injure_type, int attacker_base_type, bool test);

    //�ж�
    virtual int Action();
    //��ò���
    virtual Army& GetArmy() const;
    virtual void setArmy(Army*);

    //����
    virtual int Save();
    //����id
    virtual int Id();

    virtual int getSpic() {return m_spic;}

    virtual int UniqueId() {return m_unique_id;};

    virtual int level() const {return m_level;};

    virtual int baseType() {return m_base_stype;};

    //ȡ����
    virtual int Gong(int totype);
    virtual int Wufang(int fromtype);
    virtual int CeFang(int fromtype);
    virtual int Str() {return m_str;};
    virtual int Int() {return m_int;};
    virtual int Tongyu() {return m_tongyu;};
    virtual int Hp() {return m_hp_now;};
    virtual int MaxHp() {return m_hp_org;};
    virtual const std::string& GetName() {return m_name;};
    virtual int Shiqi() {return m_shiqi;};
    virtual int Pos() {return m_pos;};
    virtual int GetBaoji() {return m_baoji_flag;};

    //���⹥���Ƿ񴥷�
    virtual int Weihe(const iGeneral& target);
    virtual int Chaos(const iGeneral& target);
    //�Ƿ��
    virtual int CheckGedang(const iGeneral& att);
    //�Ƿ����
    virtual int CheckDodge(const iGeneral& att);
    //�Ƿ�ʶ��
    virtual int CheckShipo(const iGeneral& att);
    //�Ƿ���ֿ���
    virtual bool CheckStypeDamage(int stype);
    //������
    virtual bool isLive();
    //��Ѫ
    virtual int Addhp(int hp);
    //��ʿ��
    virtual int AddShiqi(int shiqi, bool type = false);

    //�޹���
    virtual bool NoAttack();

    virtual json_spirit::Object& GetObj();

    virtual json_spirit::Object& UpdateObj();

    json_spirit::Object& GetOrgObj();
    void GenOrgObj();
    void GenAttackMsg(int result);

    virtual int resist(int type) const;

    virtual int resist_level(int type) const;

    //����ͨ�˺��Ͳ����˺��ļ���
    virtual int subDamage(int attack_type);
    //�������ض����ֵ��˺�����
    virtual int subDamageFrom(int base_stype);

    virtual void addBuff(baseBuff* bb, iGeneral* from, int value, int last);    //����һ��buff

    virtual void removeBuff(boost::shared_ptr<Buff> b);
    
    virtual void addGeneratedBuff(boost::shared_ptr<Buff> b);    //����һ��������buff

    virtual void updateGenerateBuff();

    virtual int getBuffValue(int type) const; //��ȡһ��buff��Ч��

    virtual void updateBuff();    //û�غϸ���buff

    virtual void clearBuff();

    int HealAll();
    
    int HealOne();

    virtual void sendBuff();
    virtual void getBuffChange(json_spirit::Array& blist);
    virtual void setBuffChange();
    //����Ŀ����ɵ��˺�
    virtual int Damages(iGeneral& target);

    //��Ŀ���ӱ����˺�
    virtual int taobingDamage(iGeneral& target);

    virtual int total_inspired();
protected:
    //�ܵ��˺�
    int RecieveDamage(int damage, int type, int attacker_base_type, bool test = false);
    //���ض����ּ�ǿ�˺�
    int moreDamageto(int base_type);
    //����ǰ�ж�����״̬
    int CheckState();
    //�㹥��
    int AttackArmy_Single();
    //�㹥��-���Ⱥ���
    int AttackArmy_SingleB();
    //�й���
    int AttackArmy_Row();
    //�湥��
    int AttackArmy_Side();
    //���乥��(�ڽ�����)
    int AttackArmy_Round();
    //����3����
    int AttackArmy_ByStep();
    //ȫ�幥��
    int AttackArmy_All();
    //����
    int Set_BaojiFlag(const iGeneral& target);
    //�����Ƿ�����
    int Attack_Result(iGeneral& target);
    //���Լ��㹥�����˺�
    int tryAttack(iGeneral& target);
    //�趨�˺��Ĺ���
    int Attack(iGeneral& target, int damage, bool special_attack);
    //�����������
    int AttackSoulLink(Army& amy, const std::list<iGeneral*>& glist, int pos);
    //Ӣ������
    int Die(iGeneral& killer);
    void GenDamageInfo(int damage, int attack_type, int damage_type, bool luan = false, bool weihe = false, bool podan = false, bool baoji = false);
    void GenAttackResultMsg(int result);

    void updateSpecial();

    //���ƺ�����ŭ����ʱ���Ƿ񱩻�
    int checkBaoji();

    Army* m_army;       //���ڲ���

    //�Ƿ����
    int m_unique_id;    //�佫Ψһid
    int m_id;           //�佫id
    int m_spic;         //ͷ��id
    int m_charactor;    //������ɫid

    int m_level;        //�ȼ�
    int m_color;

    int m_stype;        //����
    int m_base_stype;   //��������
    std::string m_sname;//������
    int m_soldier_spic; //����ͼƬ
    
    int m_damage_type;  // �������   1�������� 2�����Թ��� 3������  4��ʿ��
    int m_attack_type;  // ������ʽ   1������Ŀ�� 2��һ�� 3��һ�� 4��Ŀ�꼰��Χ 5���̶�����Ŀ��6��ȫ��
    int m_attack_type2; // ������ʽ   1����ս 2��Զ��    
    int m_attack_type3; // ������ʽ   1����ս 2��Զ��    

    int m_real_attack_type;    //ʵ�ʵĹ�����ʽ
    int m_special_attack;        //�ž�����
    int m_special_attack_fac;    //��������ϵ��
    std::string m_attack_skill;    //ת��������ʽ�Ĵ�������

    int m_the_damage_total;      //���ι������˺�

    int m_damage_type2;//�˺���� 1��������,2��ǹ�̣�3������,4������,5��ħ������

    int m_nuqi_add;        //��ͨ�������ӵ�ŭ��
    int m_nuqi_add_baoji;    //�������ӵ�ŭ��
    //���䡢������ͳ��
    int m_str;
    int m_int;
    int m_tongyu;

    //����
    int m_attack;
    int m_wu_fang;
    int m_ce_fang;

    combatAttribute m_combat_attribute;    //ս������

    int m_special_attacks[special_attack_max];
    int m_special_resists[special_attack_max];

    boost::shared_ptr<specialSkill> m_speSkill;

    std::list<boost::shared_ptr<Buff> > m_buff_list;

    //���Ҳ�����buff
    std::list<boost::shared_ptr<Buff> > m_generate_buff_list;

    bool m_buff_changed;

    //std::list<combatSpeSkill> m_more_damage_skills;    //���˺�ս���ؼ�

    //std::list<combatSpeSkill> m_attack_skills;            //�仯������ʽ���ؼ�
    /*
    int m_duoshan;  //��ܸ���
    int m_gedang;   //�񵲸���
    int m_shipo;    //ʶ�Ƹ���

    int m_baoji;    //����
    int m_xixue;    //��Ѫ����
    int m_hunluan;  //����
    int m_weihe;    //����
    int m_podan;    //�Ƶ�*/

    int m_failure;  //ʧ�ܸ���
    //Ѫ��
    int m_hp_org;   //ԭʼѪ��
    int m_hp_now;   //��ǰѪ��

    //ʿ��
    int m_shiqi;    //���˼ӵ�ʿ��
    int m_shiqi_old;//ԭ����ʿ��

    int m_pos;      //9��ͼ��λ��1-9
    std::string m_name; //�佫��
    std::string m_nickname; //�º���

    //����״̬
    int m_chaos_flag;   //���ұ��
    int m_weihe_flag;   //���ű��
    int m_baoji_flag;   //�������

    //�����˺�
    int m_more_damage_fac;        //�ؼ����ӵ��˺�����ϵ��
    std::string m_more_damage_skill;//�˺������Ĵ�������

    int m_type;         // 1 ������  2 ���ط�
    bool b_init_success;    //��ʼ���ɹ�

    json_spirit::Object m_org_obj;  //��ʼobj
    json_spirit::Object m_cur_obj;  //��ǰobj

    json_spirit::Object m_action_obj;
    json_spirit::Array m_result_list;
};

#endif

