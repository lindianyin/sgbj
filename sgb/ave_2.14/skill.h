#pragma once

#include <string>
#include <map>
#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "combat_def.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "spls_const.h"

//��������
struct baseSkill
{
    int base_skill_id;            //����id
    int base_skill_type;        //�������
    int base_skill_gettype;    //��÷�ʽ
    int effect_per_level[skill_add_max];
    int effect_type;
    std::string name;
    std::string memo;
};

//�����о���
struct skillTeacher
{
    int id;
    int quality;
    int speed;
    //int hours;
    int total_mins;
    int openMap;
    std::string name;
};

//��ɫ����
struct charSkill
{
    int cid;
    int level;
    int exp;
    int state;        //����״̬ 0-����  1����������
    boost::shared_ptr<baseSkill> skill;             //����
    int save();
};

//�����о�����
struct skillResearchQue
{
    int pos;            //λ��
    int cid;        //��ɫid

    boost::shared_ptr<charSkill> skill;                //�о��ļ���
    boost::shared_ptr<skillTeacher> teacher;        //�о���

    time_t start_time;    //��ʼʱ��
    time_t end_time;    //����ʱ��
    int left_mins;        //ʣ�����
    int exp_added;        //�ۼ����ӵľ���
    int type;            //�����б� 0 ��ͨ 1 ����
    int more;            //�ӳ�
    int final_speed;    //�����ٶ�
    int fatigue;        //ƣ�Ͷ�
    time_t accelerate_time;    //�ϴμ���ʱ��

    int state;            //״̬    0 ֹͣ 1 �о���
    int start();
    int stop();

    skillResearchQue();

    boost::uuids::uuid _uuid;    //��ʱ��Ψһid

    int save();
};

class baseSkillMgr
{
public:
    baseSkillMgr();
    ~baseSkillMgr();
    int load();
    //��û���״̬
    boost::shared_ptr<baseSkill> GetBaseSkill(int id);
    //��ü����о���
    boost::shared_ptr<skillTeacher> GetSkillTeacher(int id);
    //�����������辭��
    int getExpNeed(int skill, int level);
    static baseSkillMgr* getInstance();
    //ˢ���о���
    int updateTeacher(int mapid, boost::shared_ptr<skillTeacher>* teachers);

private:
    static baseSkillMgr* m_handle;
    std::map<int, boost::shared_ptr<baseSkill> > m_base_skills;        //����״̬����
    std::map<int, boost::shared_ptr<skillTeacher> > m_base_skillsTeachers;        //�����о���

    int m_levelup_exps[total_skill_type][max_skill_level];        //�����������辭��
};

