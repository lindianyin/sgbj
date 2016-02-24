
#include "magic.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "utils_all.h"
#include "errcode_def.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

//查询技能信息
int ProcessGetMagicInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    json_spirit::Object obj;
    boost::shared_ptr<baseMagic> bsd = Singleton<MagicMgr>::Instance().getBaseMagic(id);
    if (!bsd.get())
    {
        return HC_ERROR;
    }
    obj.push_back( Pair("id", id));
    bsd->toObj(obj);
    robj.push_back( Pair("info", obj));
    return HC_SUCCESS;
}

//显示角色技能列表
int ProcessMagicList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    //窗口展现类型，仅帮客户端储存返回，不处理
    int type = 0, race = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_INT_FROM_MOBJ(race, o, "race");
    robj.push_back( Pair("type", type));
    robj.push_back( Pair("race", race));
    json_spirit::Array magic_array;
    Singleton<MagicMgr>::Instance().getMagicList(magic_array, race);
    robj.push_back( Pair("list", magic_array));
    return HC_SUCCESS;
}

//设置出战技能
int ProcessSetCombatMagic(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_magics.SetCombatMagic(o, robj);
}

//取消出战技能
int ProcessCancelCombatMagic(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_magics.CancelCombatMagic(o, robj);
}

//交换出战技能
int ProcessSwapCombatMagic(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_magics.SwapCombatMagic(o, robj);
}

//显示角色出战技能列表
int ProcessCharCombatMagics(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    json_spirit::Array list;
    cdata->m_magics.getCombatMagics(list);
    robj.push_back( Pair("combat_magics", list));
    return HC_SUCCESS;
}

void baseMagic::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_id));
    obj.push_back( Pair("combatlevel", m_combat_level));
    obj.push_back( Pair("race", m_race));
    obj.push_back( Pair("type", m_type));
    obj.push_back( Pair("name", m_name));
    obj.push_back( Pair("spic", m_spic));
    obj.push_back( Pair("memo", m_memo));
    return;
}

//从数据库中加载技能
int CharTotalMagics::Load()
{
    Query q(GetDb());
    q.get_result("SELECT pos,id FROM char_magics WHERE cid=" + LEX_CAST_STR(m_cid) + " order by pos");
    CHECK_DB_ERR(q);
    int total = q.num_rows();
    while (q.fetch_row())
    {
        int pos = 1, id = 1;
        pos = q.getval();
        id = q.getval();
        m_combat_magics[pos - 1] = id;
    }
    q.free_result();
    if (total < 3)
    {
        for (int i = 0; i < 3; ++i)
        {
            InsertSaveDb("replace into char_magics (cid,pos,id) values ("
                            + LEX_CAST_STR(m_cid)
                            + "," + LEX_CAST_STR(i+1)
                            + "," + LEX_CAST_STR(0) + ")");
        }
    }
    return 0;
}

//设置出战技能
int CharTotalMagics::SetCombatMagic(json_spirit::mObject& o, json_spirit::Object& obj)
{
    int pos = 0, magic_id = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    READ_INT_FROM_MOBJ(magic_id, o, "id");
    if (pos < 1 || pos > 3 || magic_id <= 0)
        return HC_ERROR;
    boost::shared_ptr<baseMagic> bsd = Singleton<MagicMgr>::Instance().getBaseMagic(magic_id);
    if (!bsd.get())
    {
        return HC_ERROR;
    }
    //出战技能判断等级
    if (m_charData.m_level < bsd->m_combat_level)
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    for (int i = 0; i < 3; ++i)
    {
        if (m_combat_magics[i] == magic_id)
        {
            return HC_ERROR;
        }
    }
    if (m_combat_magics[pos-1] != magic_id)
    {
        m_combat_magics[pos-1] = magic_id;
        m_changed = true;
        Save();
        obj.push_back( Pair("id", magic_id));
        obj.push_back( Pair("pos", pos));
        //更新任务
        m_charData.m_tasks.updateTask(GOAL_SET_MAGIC, magic_id, 1);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//取消出战技能
int CharTotalMagics::CancelCombatMagic(json_spirit::mObject& o, json_spirit::Object& obj)
{
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    if (pos < 1 || pos > 3)
        return HC_ERROR;
    if (m_combat_magics[pos-1] > 0)
    {
        m_combat_magics[pos-1] = 0;
        m_changed = true;
        Save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//交换出战技能
int CharTotalMagics::SwapCombatMagic(json_spirit::mObject& o, json_spirit::Object& obj)
{
    int pos1 = 0, pos2 = 0;
    READ_INT_FROM_MOBJ(pos1, o, "pos1");
    READ_INT_FROM_MOBJ(pos2, o, "pos2");
    if (pos1 < 1 || pos1 > 3 || pos2 < 1 || pos2 > 3)
        return HC_ERROR;
    //pos1必须有技能，pos2可以为空
    if (m_combat_magics[pos1-1] > 0 && m_combat_magics[pos2-1] != m_combat_magics[pos1-1])
    {
        int move_magic_id = m_combat_magics[pos1-1];
        //pos2技能状态变更
        if (m_combat_magics[pos2-1] > 0)
        {
            m_combat_magics[pos1-1] = m_combat_magics[pos2-1];
        }
        else
        {
            m_combat_magics[pos1-1] = 0;
        }
        m_combat_magics[pos2-1] = move_magic_id;
        m_changed = true;
        Save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//查询出战技能
int CharTotalMagics::getCombatMagics(json_spirit::Array& list)
{
    for (int pos = 1; pos <= 3; ++pos)
    {
        json_spirit::Object o;
        if (m_combat_magics[pos-1] > 0)
        {
            boost::shared_ptr<baseMagic> bsd = Singleton<MagicMgr>::Instance().getBaseMagic(m_combat_magics[pos-1]);
            if (bsd.get())
            {
                bsd->toObj(o);
            }
        }
        else
        {
            o.push_back( Pair("id", 0));
        }
        list.push_back(o);
    }
    return HC_SUCCESS;
}

bool CharTotalMagics::inCombat(int id)
{
    for (int pos = 1; pos <= 3; ++pos)
    {
        if (m_combat_magics[pos-1] == id)
        {
            return true;
        }
    }
    return false;
}

int CharTotalMagics::Save()
{
    if (m_changed)
    {
        for (int pos = 1; pos <= 3; ++pos)
        {
    		InsertSaveDb("update char_magics set id=" + LEX_CAST_STR(m_combat_magics[pos-1])
    					+ " where cid=" + LEX_CAST_STR(m_cid) + " and pos=" + LEX_CAST_STR(pos));
        }
        m_changed = false;
    }
    return 0;
}

MagicMgr::MagicMgr()
{
	Query q(GetDb());
    //影响战斗结算技能
    q.get_result("SELECT id,race,type,combatlevel,spic,name,memo from base_magics where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseMagic> st;
    	std::map<int, boost::shared_ptr<baseMagic> >::iterator it = m_base_magics.find(id);
    	if (it != m_base_magics.end())
    	{
    		st = it->second;
    	}
        else
        {
            st.reset(new baseMagic);
			m_base_magics[id] = st;
            m_magics.push_back(id);
        }
		st->m_id = id;
        st->m_race = q.getval();
        st->m_type = q.getval();
        st->m_combat_level = q.getval();
        st->m_spic = q.getval();
		st->m_name = q.getstr();
		st->m_memo = q.getstr();
    }
    q.free_result();
}

//获得基础技能
boost::shared_ptr<baseMagic> MagicMgr::getBaseMagic(int id)
{
	std::map<int, boost::shared_ptr<baseMagic> >::iterator it = m_base_magics.find(id);
	if (it != m_base_magics.end())
	{
		return it->second;
	}
	else
	{
		boost::shared_ptr<baseMagic> gd;
		gd.reset();
		return gd;
	}
}

//随机技能id
int MagicMgr::RandomMagic()
{
    int idx = my_random(0, m_magics.size() - 1);
    return m_magics[idx];
}

int MagicMgr::getMagicList(json_spirit::Array& list, int race)
{
	std::map<int, boost::shared_ptr<baseMagic> >::iterator it = m_base_magics.begin();
	while (it != m_base_magics.end())
	{
        if (it->second->m_race == race || it->second->m_race == 0)
        {
            json_spirit::Object o;
            it->second->toObj(o);
            list.push_back(o);
        }
        ++it;
	}
    return HC_SUCCESS;
}

