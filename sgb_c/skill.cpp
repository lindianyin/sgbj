
#include "skill.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "utils_all.h"
#include "errcode_def.h"
#include "relation.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

//查询技能信息
int ProcessGetSkillInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0, purpose = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    json_spirit::Object s_obj;
    if (purpose == 1)//查询基础技能
    {
        int level = 1;
        READ_INT_FROM_MOBJ(level, o, "level");
        boost::shared_ptr<baseSkill> bsd = Singleton<skillMgr>::Instance().getBaseSkill(id);
        if (!bsd.get())
        {
            return HC_ERROR;
        }
        s_obj.push_back( Pair("id", id));
        s_obj.push_back( Pair("level", level));
        bsd->toObj(level, s_obj);
    }
    else if(purpose == 2)//查询玩家具体技能
    {
        int cid = cdata->m_id;
        READ_INT_FROM_MOBJ(cid, o, "cid");
        if (cid)
        {
            CharData* pcc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (pcc)
            {
                boost::shared_ptr<CharSkill> c_skill = pcc->m_skills.GetSkill(id);
                if (c_skill.get())
                {
                    c_skill->toObj(s_obj);
                }
            }
        }
    }
    robj.push_back( Pair("info", s_obj));
    robj.push_back( Pair("purpose", purpose));
    return HC_SUCCESS;
}

//显示角色技能列表
int ProcessCharSkills(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    //窗口展现类型，仅帮客户端储存返回，不处理
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("type", type));
    int stype = 0;
    READ_INT_FROM_MOBJ(stype, o, "stype");
    robj.push_back( Pair("stype", stype));
    json_spirit::Array skill_array;
    std::map<int, boost::shared_ptr<CharSkill> >::iterator it = cdata->m_skills.m_skills.begin();
    while (it != cdata->m_skills.m_skills.end())
    {
        if (!it->second.get())
        {
            ++it;
            continue;
        }
        if (it->second->m_type != stype)
        {
            ++it;
            continue;
        }
        boost::shared_ptr<CharSkill> c_skill = it->second;
        if (c_skill.get())
        {
            json_spirit::Object s_obj;
            c_skill->toObj(s_obj);
            skill_array.push_back(s_obj);
        }
        else
        {
            ERR();
        }
        ++it;
    }
    robj.push_back( Pair("list", skill_array));
    return HC_SUCCESS;
}

//升级技能信息
int ProcessUpSkillInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int skill_id = 0;
    READ_INT_FROM_MOBJ(skill_id, o, "id");
    return cdata->m_skills.upgradeSkillInfo(skill_id, robj);
}

//升级技能
int ProcessUpSkill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int skill_id = 0;
    READ_INT_FROM_MOBJ(skill_id, o, "id");
    return cdata->m_skills.upgradeSkill(skill_id, robj);
}

void baseSkill::toObj(int level, json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_id));
    obj.push_back( Pair("type", m_type));
    obj.push_back( Pair("name", m_name));
    obj.push_back( Pair("spic", m_spic));
    std::string memo = m_memo;
	for (int i = 0; i < SKILL_VALUE_MAX; ++i)
	{
        if (m_base_rank_value[i] > 0 && m_add_rank_value[i] > 0)
        {
            int tmp = m_base_rank_value[i] + m_add_rank_value[i] * (level-1);
            str_replace(memo, "$V", LEX_CAST_STR(tmp));
            break;
        }
	}
    if (m_base_attack_value > 0 && m_add_attack_value > 0)
    {
        int tmp = m_base_attack_value + m_add_attack_value * (level-1);
        str_replace(memo, "$A", LEX_CAST_STR(tmp));
    }
    if (m_base_magic_value > 0 && m_add_magic_value > 0)
    {
        int tmp = m_base_magic_value + m_add_magic_value * (level-1);
        str_replace(memo, "$M", LEX_CAST_STR(tmp));
    }
    if (m_base_defense_value > 0 && m_add_defense_value > 0)
    {
        int tmp = m_base_defense_value + m_add_defense_value * (level-1);
        str_replace(memo, "$D", LEX_CAST_STR(tmp));
    }
    obj.push_back( Pair("memo", memo));
    return;
}

void baseSkill::loadUpgrade()
{
	Query q(GetDb());
    q.get_result("SELECT level,needlevel,bookid,cost from base_skill_upgrade_data where sid="+LEX_CAST_STR(m_id)+" order by level asc");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        int needlevel = q.getval();
        int book = q.getval();
        int cost = q.getval();
        if (level <= iMaxSkillLevel)
        {
            m_upgrade_needlevel[level - 1] = needlevel;
            m_upgrade_book[level - 1] = book;
            m_upgrade[level - 1] = cost;
        }
    }
    q.free_result();
}

void CharSkill::toObj(json_spirit::Object& obj, int level)
{
    if (!m_base_skill.get())
    {
        return;
    }
    int tmp = m_level;
    if (level > 0)
        tmp = level;
    obj.push_back( Pair("level", tmp));
    obj.push_back( Pair("state", m_state));
    obj.push_back( Pair("combatlevel", m_base_skill->m_combat_level));
    if (tmp <= iMaxSkillLevel)
    {
        obj.push_back( Pair("needlevel", m_base_skill->m_upgrade_needlevel[tmp-1]));
    }
    m_base_skill->toObj(tmp, obj);
    return;
}

int CharSkill::upgrade()
{
    if (!m_base_skill.get())
    {
        return HC_ERROR;
    }
    if (m_level >= iMaxSkillLevel)
    {
        return HC_ERROR;
    }
    if (m_belong_to.m_charData.m_level < m_base_skill->m_upgrade_needlevel[m_level-1])
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    if (m_belong_to.m_charData.silver() < m_base_skill->m_upgrade[m_level-1])
    {
        return HC_ERROR_NOT_ENOUGH_SILVER;
    }
    //扣道具
    if (m_base_skill->m_upgrade_book[m_level-1] > 0 && m_belong_to.m_charData.subGem(m_base_skill->m_upgrade_book[m_level-1],1,gem_cost_skill_upgrade) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GEM;
    }
    m_belong_to.m_charData.subSilver(m_base_skill->m_upgrade[m_level-1], silver_cost_skill_upgrade);
    ++m_level;
    m_changed = true;
    save();
    //更新任务
    m_belong_to.m_charData.m_tasks.updateTask(GOAL_UPGRADE_SKILL, m_sid, m_level);
    if (m_level >= 7)
    {
        Singleton<relationMgr>::Instance().postCongradulation(m_belong_to.m_charData.m_id, CONGRATULATION_SKILL, m_level, 0);
    }
    return HC_SUCCESS;
}

int CharSkill::save()
{
	if (m_changed && m_base_skill.get())
	{
		InsertSaveDb("update char_skills set level=" + LEX_CAST_STR(m_level)
                    + ",state=" + LEX_CAST_STR(m_state)
					+ " where cid=" + LEX_CAST_STR(m_cid) + " and sid=" + LEX_CAST_STR(m_base_skill->m_id));
        m_changed = false;
	}
	return 0;
}

//从数据库中加载技能
int CharTotalSkills::Load()
{
    Query q(GetDb());
    q.get_result("SELECT sid,level FROM char_skills WHERE cid=" + LEX_CAST_STR(m_cid) + " order by sid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<CharSkill> s;
        s.reset(new CharSkill(m_charData, *this));
        s->m_sid = q.getval();
        s->m_cid = m_charData.m_id;
        s->m_level = q.getval();
        s->m_state = 4;//新魔法增加后技能永远生效
        s->m_base_skill = Singleton<skillMgr>::Instance().getBaseSkill(s->m_sid);
        if (!s->m_base_skill.get())
        {
            ERR();
            continue;
        }
        s->m_type = s->m_base_skill->m_type;
        m_skills[s->m_sid] = s;
    }
    q.free_result();
    updateCombatAttribute(m_skills, m_charData.m_combat_attribute, m_charData.m_level);
    return 0;
}

//获得技能
void CharTotalSkills::Add(int id, int level)
{
    boost::shared_ptr<baseSkill> bsd = Singleton<skillMgr>::Instance().getBaseSkill(id);
    if (!bsd.get())
    {
        return;
    }
    boost::shared_ptr<CharSkill> s;
    s.reset(new CharSkill(m_charData, *this));
    s->m_sid = id;
    s->m_cid = m_charData.m_id;
    s->m_level = level;
    s->m_base_skill = bsd;
    s->m_type = bsd->m_type;
    s->m_state = 4;
    InsertSaveDb("replace into char_skills (cid,sid,level,state) values ("
                    + LEX_CAST_STR(m_cid)
                    + "," + LEX_CAST_STR(id)
                    + "," + LEX_CAST_STR(s->m_level)
                    + "," + LEX_CAST_STR(s->m_state) + ")");
    //加入玩家技能队列
    m_skills[s->m_sid] = s;
    return;
}

boost::shared_ptr<CharSkill> CharTotalSkills::GetSkill(int sid)
{
    if (sid > 0)
    {
        std::map<int, boost::shared_ptr<CharSkill> >::iterator it = m_skills.find(sid);
        if (it != m_skills.end())
        {
            return it->second;
        }
    }
    else if (sid == 0)
    {
        std::map<int, boost::shared_ptr<CharSkill> >::iterator it = m_skills.begin();
        if (it != m_skills.end())
        {
            return it->second;
        }
    }
    boost::shared_ptr<CharSkill> hd;
    hd.reset();
    return hd;
}

int CharTotalSkills::upgradeSkillInfo(int skill_id, json_spirit::Object& obj)
{
    boost::shared_ptr<CharSkill> skill = GetSkill(skill_id);
    if (!skill.get())
    {
        ERR();
        return HC_ERROR;
    }
    json_spirit::Object s_obj;
    skill->toObj(s_obj, skill->m_level);
    obj.push_back( Pair("skill_obj", s_obj));
    if (skill->m_level > 0 && skill->m_level < iMaxSkillLevel)
    {
        s_obj.clear();
        skill->toObj(s_obj, skill->m_level+1);
        obj.push_back( Pair("up_skill_obj", s_obj));
        obj.push_back( Pair("cost", skill->m_base_skill->m_upgrade[skill->m_level-1]));
        obj.push_back( Pair("needlevel", skill->m_base_skill->m_upgrade_needlevel[skill->m_level-1]));
        if (skill->m_base_skill->m_upgrade_book[skill->m_level-1])
        {
            int tid = skill->m_base_skill->m_upgrade_book[skill->m_level-1];
            json_spirit::Object gem_obj;
            Item item(ITEM_TYPE_GEM, tid, 1, 0);
            item.toObj(gem_obj);
            int cur_num = m_charData.m_bag.getGemCount(tid);
            gem_obj.push_back( Pair("cur_num", cur_num));
            obj.push_back( Pair("gem_obj", gem_obj));
        }
    }
    return HC_SUCCESS;
}

int CharTotalSkills::upgradeSkill(int skill_id, json_spirit::Object& obj)
{
    //设置新技能状态
    boost::shared_ptr<CharSkill> skill = GetSkill(skill_id);
    if (skill.get() && skill->m_base_skill.get())
    {
        obj.push_back( Pair("id", skill_id));
        obj.push_back( Pair("type", skill->m_type));
        int ret = skill->upgrade();
        if (ret == HC_SUCCESS)
        {
            updateCombatAttribute(m_skills, m_charData.m_combat_attribute, m_charData.m_level);
            m_charData.m_heros.updateAttribute();
        }
        return ret;
    }
    return HC_ERROR;
}

int CharTotalSkills::Save()
{
    if (m_changed)
    {
        std::map<int, boost::shared_ptr<CharSkill> >::iterator it = m_skills.begin();
        while (it != m_skills.end())
        {
            if (it->second.get())
            {
                it->second->save();
            }
            ++it;
        }
        m_changed = false;
    }
    return 0;
}

skillMgr::skillMgr()
{
	Query q(GetDb());
    m_active_skills.clear();
    //影响战斗结算技能
    q.get_result("SELECT id,type,combatlevel,spic,name,memo,val1,val2,val3,val4,val5,val6,val7,val8,val9,val10,val11,val_attack,val_defense,val_magic from base_skills where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int type = q.getval();
        boost::shared_ptr<baseSkill> st;
    	std::map<int, boost::shared_ptr<baseSkill> >::iterator it = m_base_skills.find(id);
    	if (it != m_base_skills.end())
    	{
    		st = it->second;
    	}
        else
        {
            st.reset(new baseSkill);
			m_base_skills[id] = st;
            m_skills.push_back(id);
            if (type == SKILL_TYPE_ACTIVE)
                m_active_skills.push_back(id);
        }
		st->m_id = id;
        st->m_type = type;
        st->m_combat_level = q.getval();
        st->m_spic = q.getval();
		st->m_name = q.getstr();
		st->m_memo = q.getstr();
        //手牌加成
		for (int i = 0; i < SKILL_VALUE_MAX; ++i)
		{
            std::string rank_value = q.getstr();
            if (rank_value != "")
            {
                json_spirit::Value types;
                json_spirit::read(rank_value, types);
                if (types.type() == json_spirit::array_type)
                {
                    json_spirit::Array& types_array = types.get_array();
                    json_spirit::Array::iterator it = types_array.begin();
                    if (it != types_array.end() && (*it).type() == json_spirit::int_type)
                    {
                        st->m_base_rank_value[i] = (*it).get_int();
                    }
                    ++it;
                    if (it != types_array.end() && (*it).type() == json_spirit::int_type)
                    {
                        st->m_add_rank_value[i] = (*it).get_int();
                    }
                }
                else
                {
                    ERR();
                }
            }
            else
            {
                st->m_base_rank_value[i] = 0;
                st->m_add_rank_value[i] = 0;
            }
		}
        //攻防魔
		{
            std::string att_value = q.getstr();
            if (att_value != "")
            {
                json_spirit::Value types;
                json_spirit::read(att_value, types);
                if (types.type() == json_spirit::array_type)
                {
                    json_spirit::Array& types_array = types.get_array();
                    json_spirit::Array::iterator it = types_array.begin();
                    if (it != types_array.end() && (*it).type() == json_spirit::int_type)
                    {
                        st->m_base_attack_value = (*it).get_int();
                    }
                    ++it;
                    if (it != types_array.end() && (*it).type() == json_spirit::int_type)
                    {
                        st->m_add_attack_value = (*it).get_int();
                    }
                }
                else
                {
                    ERR();
                }
            }
            else
            {
                st->m_base_attack_value = 0;
                st->m_add_attack_value = 0;
            }
		}
		{
            std::string def_value = q.getstr();
            if (def_value != "")
            {
                json_spirit::Value types;
                json_spirit::read(def_value, types);
                if (types.type() == json_spirit::array_type)
                {
                    json_spirit::Array& types_array = types.get_array();
                    json_spirit::Array::iterator it = types_array.begin();
                    if (it != types_array.end() && (*it).type() == json_spirit::int_type)
                    {
                        st->m_base_defense_value = (*it).get_int();
                    }
                    ++it;
                    if (it != types_array.end() && (*it).type() == json_spirit::int_type)
                    {
                        st->m_add_defense_value = (*it).get_int();
                    }
                }
                else
                {
                    ERR();
                }
            }
            else
            {
                st->m_base_defense_value = 0;
                st->m_add_defense_value = 0;
            }
		}
		{
            std::string magic_value = q.getstr();
            if (magic_value != "")
            {
                json_spirit::Value types;
                json_spirit::read(magic_value, types);
                if (types.type() == json_spirit::array_type)
                {
                    json_spirit::Array& types_array = types.get_array();
                    json_spirit::Array::iterator it = types_array.begin();
                    if (it != types_array.end() && (*it).type() == json_spirit::int_type)
                    {
                        st->m_base_magic_value = (*it).get_int();
                    }
                    ++it;
                    if (it != types_array.end() && (*it).type() == json_spirit::int_type)
                    {
                        st->m_add_magic_value = (*it).get_int();
                    }
                }
                else
                {
                    ERR();
                }
            }
            else
            {
                st->m_base_magic_value = 0;
                st->m_add_magic_value = 0;
            }
		}
        //加载升级数据
        st->loadUpgrade();
    }
    q.free_result();
}

//获得基础技能
boost::shared_ptr<baseSkill> skillMgr::getBaseSkill(int id)
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

//随机技能id
int skillMgr::RandomSkill()
{
    int idx = my_random(0, m_skills.size() - 1);
    return m_skills[idx];
}

