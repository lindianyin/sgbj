
#pragma once

#include <string>
#include <map>
#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "const_def.h"
#include "json_spirit.h"
#include "net.h"

const int iMaxSkillLevel = 10;//������ߵȼ�

enum SKILL_TYPE
{
    SKILL_TYPE_ACTIVE = 1,
    SKILL_TYPE_PASSIVE = 2,
    SKILL_TYPE_ATTRIBUTE = 3,
};

enum SKILL_VALUE_TYPE
{
    SKILL_VALUE_ROYAL_FLUSH = 1,
    SKILL_VALUE_SHELL_FLUSH = 2,
    SKILL_VALUE_STRAIGHT_FLUSH = 3,
    SKILL_VALUE_FOUR_OF_A_KIND = 4,
    SKILL_VALUE_FULL_HOUSE = 5,
    SKILL_VALUE_FLUSH = 6,
    SKILL_VALUE_STRAIGHT = 7,
    SKILL_VALUE_THREE_OF_A_KIND = 8,
    SKILL_VALUE_TWO_PAIR = 9,
    SKILL_VALUE_ONE_PAIR = 10,
    SKILL_VALUE_HIGH_CARD = 11,
    SKILL_VALUE_MAX = 11,
};

struct CharTotalSkills;
struct CharData;

//��������
struct baseSkill
{
    int m_id;
    int m_spic;
    int m_type;
    int m_combat_level;//��ս�ȼ�
    int m_upgrade_needlevel[iMaxSkillLevel];//����������Ҫ�ȼ�
    int m_upgrade[iMaxSkillLevel];//�������ܳ���
    int m_upgrade_book[iMaxSkillLevel];//����������
    int m_base_rank_value[SKILL_VALUE_MAX];//���ܶ�����Ӱ��Ч��(1��������Ӱ�칥��2��������Ӱ�����)
    int m_add_rank_value[SKILL_VALUE_MAX];//���ܶ�����Ӱ��Ч��(1��������Ӱ�칥��2��������Ӱ�����)
    int m_base_attack_value;
    int m_add_attack_value;
    int m_base_defense_value;
    int m_add_defense_value;
    int m_base_magic_value;
    int m_add_magic_value;
    std::string m_name;
    std::string m_memo;
    void toObj(int level, json_spirit::Object& obj);
    void loadUpgrade();
};

//��ɫ����
struct CharSkill
{
    int m_sid;
    int m_cid;
    int m_level;
    int m_type;
    int m_state;//0δ��ս1-3��Ӧ��ս����λ4����������Զ��ս
    boost::shared_ptr<baseSkill> m_base_skill;
    CharTotalSkills& m_belong_to;
    CharSkill(CharData& c, CharTotalSkills& bl)
    :m_belong_to(bl)
    {
        m_level = 1;
        m_state = 0;
        m_changed = false;
    };
    bool m_changed;  //�иĶ�
    void toObj(json_spirit::Object& obj, int level = 0);
    int upgrade();
    int save();
};

struct CharTotalSkills
{
    CharData& m_charData;
    int m_cid;
    std::map<int, boost::shared_ptr<CharSkill> > m_skills;

    CharTotalSkills(int cid, CharData& cdata)
    :m_charData(cdata)
    {
        m_cid = cid;
        m_changed = false;
    };
    int Load();
    void Add(int id, int level = 1);
    boost::shared_ptr<CharSkill> GetSkill(int sid);
    int upgradeSkillInfo(int skill_id, json_spirit::Object& obj);
    int upgradeSkill(int skill_id, json_spirit::Object& obj);//��������
    int Save(); //����
    bool m_changed; //�иĶ�
};

class skillMgr
{
public:
	skillMgr();
	//��û���״̬
	boost::shared_ptr<baseSkill> getBaseSkill(int id);
	int RandomSkill();
private:
    std::vector<int> m_active_skills;
    std::vector<int> m_skills;
    std::map<int, boost::shared_ptr<baseSkill> > m_base_skills;		//������������
};

//��ѯ������Ϣ
int ProcessGetSkillInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʾ��ɫ�����б�
int ProcessCharSkills(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����������Ϣ
int ProcessUpSkillInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������
int ProcessUpSkill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

