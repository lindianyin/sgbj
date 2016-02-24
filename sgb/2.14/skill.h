#pragma once

#include <string>
#include <map>
#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "combat_def.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "spls_const.h"

//基础技能
struct baseSkill
{
    int base_skill_id;            //技能id
    int base_skill_type;        //技能类别
    int base_skill_gettype;    //获得方式
    int effect_per_level[skill_add_max];
    int effect_type;
    std::string name;
    std::string memo;
};

//技能研究者
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

//角色技能
struct charSkill
{
    int cid;
    int level;
    int exp;
    int state;        //技能状态 0-正常  1技能升级中
    boost::shared_ptr<baseSkill> skill;             //技能
    int save();
};

//技能研究队列
struct skillResearchQue
{
    int pos;            //位置
    int cid;        //角色id

    boost::shared_ptr<charSkill> skill;                //研究的技能
    boost::shared_ptr<skillTeacher> teacher;        //研究者

    time_t start_time;    //开始时间
    time_t end_time;    //结束时间
    int left_mins;        //剩余分钟
    int exp_added;        //累计增加的经验
    int type;            //队列列别 0 普通 1 金钻
    int more;            //加成
    int final_speed;    //最终速度
    int fatigue;        //疲劳度
    time_t accelerate_time;    //上次加速时间

    int state;            //状态    0 停止 1 研究中
    int start();
    int stop();

    skillResearchQue();

    boost::uuids::uuid _uuid;    //定时器唯一id

    int save();
};

class baseSkillMgr
{
public:
    baseSkillMgr();
    ~baseSkillMgr();
    int load();
    //获得基础状态
    boost::shared_ptr<baseSkill> GetBaseSkill(int id);
    //获得技能研究者
    boost::shared_ptr<skillTeacher> GetSkillTeacher(int id);
    //技能升级所需经验
    int getExpNeed(int skill, int level);
    static baseSkillMgr* getInstance();
    //刷新研究者
    int updateTeacher(int mapid, boost::shared_ptr<skillTeacher>* teachers);

private:
    static baseSkillMgr* m_handle;
    std::map<int, boost::shared_ptr<baseSkill> > m_base_skills;        //基础状态数据
    std::map<int, boost::shared_ptr<skillTeacher> > m_base_skillsTeachers;        //技能研究者

    int m_levelup_exps[total_skill_type][max_skill_level];        //技能升级所需经验
};

