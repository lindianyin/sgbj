
#include "skill.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "utils_all.h"
#include "json_spirit.h"
#include "spls_timer.h"
#include "spls_errcode.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

baseSkillMgr* baseSkillMgr::m_handle = NULL;

//static     boost::uuids::nil_generator gen;

skillResearchQue::skillResearchQue()
{
    _uuid = boost::uuids::nil_uuid();
    cid = 0;
    start_time = 0;
    left_mins = 0;
    state = 0;
    more = 0;
    fatigue = 0;
    accelerate_time = 0;
}

int skillResearchQue::start()
{
    if (state == 1)
    {
        return 0;
    }
    int times = left_mins/skill_research_mins;
    if (times > 0)
    {
        if (more > 0)
        {
            final_speed = teacher->speed * (100+more)/100;
        }
        else
        {
            final_speed = teacher->speed;
        }
        
        //start_time = time(NULL);
        //end_time = start_time + left_mins * 60;
        state = 1;
        skill->state = 1;
        //重启后的情况
        int passmin = (time(NULL) - start_time)/60;

        json_spirit::mObject mobj;
        mobj["cmd"] = "researchDone";
        mobj["cid"] = cid;
        mobj["sid"] = skill->skill->base_skill_id;
        boost::shared_ptr<splsTimer> tmsg;

        int pass_times = passmin / skill_research_mins;
        if (pass_times >= times)
        {
            mobj["times"] = times;
            //直接完成了
            tmsg.reset(new splsTimer(0.1, 1, mobj, 1));
            _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        else if (pass_times > 0)
        {
            mobj["times"] = pass_times;
            tmsg.reset(new splsTimer(0.1, 1,mobj,1));
            _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        else
        {
            tmsg.reset(new splsTimer(skill_research_mins*60, 1, mobj, 1));
            _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        end_time = start_time + left_mins * 60;
        //cout<<"***********skill "<<skill->skill->sid<<" research start at "<<start_time<<",end at "<<end_time<<",speed:"<<final_speed <<endl;
        save();
        return 0;
    }
    return 0;
}

int skillResearchQue::stop()
{
    if (state == 1)
    {
        if (skill.get())
        {
            skill->state = 0;
        }
        skill.reset();
        teacher.reset();
        start_time = 0;
        end_time = 0;
        left_mins = 0;
        splsTimerMgr::getInstance()->delTimer(_uuid);
        _uuid = boost::uuids::nil_uuid();
        state = 0;
        return 0;
    }
    return -1;
}

int skillResearchQue::save()
{
    int sid = 0;
    int tid = 0;
    if (skill.get() && skill->skill.get())
    {
        sid = skill->skill->base_skill_id;
    }
    if (teacher.get())
    {
        tid = teacher->id;
    }
    InsertSaveDb("update char_skill_research set type=" + LEX_CAST_STR(type)
            + ",sid=" + LEX_CAST_STR(sid)
            + ",teacher=" + LEX_CAST_STR(tid)
            + ",starttime=" + LEX_CAST_STR(start_time)
            + ",lefttime=" + LEX_CAST_STR(left_mins)
            + ",state=" + LEX_CAST_STR(state)
            + ",fatigue=" + LEX_CAST_STR(fatigue)
            + ",accelerate_time=" + LEX_CAST_STR(accelerate_time)
            + " where cid=" + LEX_CAST_STR(cid) + " and pos=" + LEX_CAST_STR(pos));
    return 0;
}

int charSkill::save()
{
    if (skill.get())
    {
        InsertSaveDb("update char_skills set level=" + LEX_CAST_STR(level)
                    + ",exp=" + LEX_CAST_STR(exp)
                    + " where cid=" + LEX_CAST_STR(cid) + " and sid=" + LEX_CAST_STR(skill->base_skill_id));
    }
    return 0;
}

baseSkillMgr::baseSkillMgr()
{
    load();
}

baseSkillMgr::~baseSkillMgr()
{
}

int baseSkillMgr::load()
{
    Query q(GetDb());
    //基础技能 步弓谋骑械
    q.get_result("SELECT id,name,type,memo,gettype,hp,pugong,pufang,cegong,cefang,gong_to_bb,fang_to_bb,gong_to_gb,fang_to_gb,gong_to_cs,fang_to_cs,gong_to_qb,fang_to_qb,gong_to_qx,fang_to_qx from base_skills where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseSkill> st = GetBaseSkill(id);
        if (!st.get())
        {
            st.reset(new baseSkill);
            m_base_skills[id] = st;
        }
        st->base_skill_id = id;
        st->name = q.getstr();
        st->base_skill_type = q.getval();
        st->memo = q.getstr();
        st->base_skill_gettype = q.getval();
        for (int i = 0; i < skill_add_max; ++i)
        {
            st->effect_per_level[i] = q.getval();
            if (st->effect_per_level[i] > 0)
            {
                st->effect_type = i;
            }
        }
    }
    q.free_result();

    //技能升级所需经验
    std::string sql = "select level";
    for (int i = 1; i <= total_skill_type; ++i)
    {
        sql = sql + ",exp" + LEX_CAST_STR(i);
    }
    sql += " from base_skills_research where 1 order by level";
    q.get_result(sql);
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        if (level >= 1 && level <= max_skill_level)
        {
            for (int i = 0; i < total_skill_type; ++i)
            {
                m_levelup_exps[i][level-1] = q.getval();
            }
        }
    }
    q.free_result();

    //技能研究者
    q.get_result("select id,name,quality,speed,minutes,openMap from base_skills_teacher where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
         boost::shared_ptr<skillTeacher> tea = GetSkillTeacher(id);
         if (!tea.get())
         {
             tea.reset(new skillTeacher);
            tea->id = id;
            m_base_skillsTeachers[id] = tea;
         }
         tea->name = q.getstr();
         tea->quality = q.getval();
         tea->speed = q.getval() * skill_research_mins;
         //tea->hours = q.getval();
         tea->total_mins = q.getval();
         tea->openMap = q.getval();
    }
    q.free_result();
    return 0;
}

baseSkillMgr* baseSkillMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new baseSkillMgr();
    }
    return m_handle;
}

//获得基础技能
boost::shared_ptr<baseSkill> baseSkillMgr::GetBaseSkill(int id)
{
    std::map<int, boost::shared_ptr<baseSkill> >::iterator it = m_base_skills.find(id);
    if (it != m_base_skills.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseSkill> gd;
        gd.reset();
        return gd;
    }
}

boost::shared_ptr<skillTeacher> baseSkillMgr::GetSkillTeacher(int id)
{
    std::map<int, boost::shared_ptr<skillTeacher> >::iterator it = m_base_skillsTeachers.find(id);
    if (it != m_base_skillsTeachers.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<skillTeacher> gd;
        gd.reset();
        return gd;
    }
}

//技能升级所需经验
int baseSkillMgr::getExpNeed(int skill,int level)
{
    if (level > max_skill_level || level <= 0)
    {
        return -1;
    }
    if (skill < 1 || skill > total_skill_type)
    {
        return -1;
    }
    return m_levelup_exps[skill-1][level-1];
}

int baseSkillMgr::updateTeacher(int mapid, boost::shared_ptr<skillTeacher>* teachers)
{
    //cout<<"updateTeacher:"<<mapid<<endl;
    int max_teacher_id = 4 * mapid;
    if (max_teacher_id <= 0)
    {
        max_teacher_id = 4;
    }
    else if (max_teacher_id > (int)m_base_skillsTeachers.size())
    {
        max_teacher_id = m_base_skillsTeachers.size();
    }
    std::vector<boost::shared_ptr<skillTeacher> > teachers_maps;
    for (int i = 1; i <= max_teacher_id; ++i)
    {
        teachers_maps.push_back(m_base_skillsTeachers[i]);
    }
    for (int i = 0; i < skill_teacher_nums; ++i)
    {
        //cout<<"random 1 - "<<teachers_maps.size()<<" result ";
        int idx = my_random(1, teachers_maps.size());
        //cout<<idx<<endl;
        teachers[i].reset();
        teachers[i] = teachers_maps[idx - 1];
        teachers_maps.erase(teachers_maps.begin() + idx - 1);
    }
    return HC_SUCCESS;
}

