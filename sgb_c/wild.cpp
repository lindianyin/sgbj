
#include "wild.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "errcode_def.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "SaveDb.h"
#include "spls_timer.h"
#include "city.h"
#include "utils_lang.h"


Database& GetDb();
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

//获取城外城池列表
int ProcessQueryWildCitys(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    json_spirit::Array list;
    int pos = 1;
    for(std::vector<int>::iterator it = cdata->m_wild_citys.m_view_citys.begin(); it != cdata->m_wild_citys.m_view_citys.end(); ++it)
    {
        wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(*it);
        if (pwc)
        {
            json_spirit::Object tmp_city;
            pwc->toObj(tmp_city);
            boost::shared_ptr<base_wild_city_info> bw = Singleton<wildMgr>::Instance().getWildInfo(pos);
            if (bw.get())
            {
                tmp_city.push_back( Pair("pos", pos));
                tmp_city.push_back( Pair("name", bw->name));
                tmp_city.push_back( Pair("x", bw->x));
                tmp_city.push_back( Pair("y", bw->y));
            }
            list.push_back(tmp_city);
			++pos;
        }
        else
        {
            *it = 0;
        }
    }
    robj.push_back( Pair("wild_citys", list));
    return HC_SUCCESS;
}

//野外城池收税
int ProcessWildCityLevy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    return cdata->m_wild_citys.wildLevy(id, robj);
}

//野外城池设置城守
int ProcessWildCityDefense(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 1, hid = 0, purpose = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    READ_INT_FROM_MOBJ(hid,o,"hid");
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    robj.push_back( Pair("purpose", purpose));
    return cdata->m_wild_citys.wildDefense(purpose, id, hid, robj);
}

void wild_city::reset()
{
    m_owner_cid = 0;
    m_defense_hid = 0;
    m_levy_start = 0;
    m_fight = 0;
    m_notify = false;
    m_get_notify = false;
}

void wild_city::toObj(json_spirit::Object& robj)
{
    robj.push_back( Pair("id", m_id));
    robj.push_back( Pair("owner_cid", m_owner_cid));
    robj.push_back( Pair("defense_hid", m_defense_hid));
    robj.push_back( Pair("is_fight", m_fight));
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_owner_cid);
    if (cdata.get())
    {
        robj.push_back( Pair("owner_name", cdata->m_name));
    }
    robj.push_back( Pair("levy_cnt", levy_get()));
}

void wild_city::save()
{
    InsertSaveDb("update wild_citys set cid=" + LEX_CAST_STR(m_owner_cid) + ",hid=" + LEX_CAST_STR(m_defense_hid) + ",levy_start=" + LEX_CAST_STR(m_levy_start)
        + " where id=" + LEX_CAST_STR(m_id));
}

void wild_city::broadCastInfo()
{
    for (std::vector<int>::iterator it = m_viewer.begin(); it != m_viewer.end(); ++it)
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(*it);
        if (!cdata.get() || cdata->m_is_online == 0)
        {
            continue;
        }
        json_spirit::Object tmp_city;
        tmp_city.push_back( Pair("cmd", "updateWildCity") );
        tmp_city.push_back( Pair("s", 200) );
        toObj(tmp_city);
        cdata->sendObj(tmp_city);
    }
    if (levy_get() >= iWildCityNotifyLevy)
    {
        m_notify = true;
    }
    if (levy_get() > 0)
    {
        m_get_notify = true;
    }
}

void wild_city::addViewer(int cid)
{
    if (find(m_viewer.begin(),m_viewer.end(),cid) == m_viewer.end())
    {
        m_viewer.push_back(cid);
    }
}

void wild_city::removeViewer(int cid)
{
    std::vector<int>::iterator it = find(m_viewer.begin(),m_viewer.end(),cid);
    if (it != m_viewer.end())
    {
        m_viewer.erase(it);
    }
}

int wild_city::levy_get()
{
    int get = 0;
    int last_sec = time(NULL) - m_levy_start;
    if (m_levy_start > 0 && last_sec > 60)
    {
        get = last_sec / 60 * 200;
    }
    if (get > 100000)
        get = 100000;
    return get;
}

int CharWildCitys::load()
{
    Query q(GetDb());
    std::string sqlcmd = "SELECT citys FROM char_view_wild_citys WHERE cid=" + LEX_CAST_STR(m_chardata.m_id);
    q.get_result(sqlcmd);
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        std::string citys = q.getstr();
        if(citys == "")
        {
            m_view_citys.clear();
            wildViewRefresh();
        }
        json_spirit::Value types;
        json_spirit::read(citys, types);
        if (types.type() == json_spirit::array_type)
        {
            json_spirit::Array& types_array = types.get_array();
            for (json_spirit::Array::iterator it = types_array.begin(); it != types_array.end(); ++it)
            {
                if ((*it).type() != json_spirit::int_type)
                {
                    break;
                }
                int tmp_city = (*it).get_int();
                m_view_citys.push_back(tmp_city);
                wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(tmp_city);
                if (pwc)
                {
                    if (pwc->m_owner_cid == m_chardata.m_id && pwc->m_defense_hid > 0)
                    {
                        boost::shared_ptr<CharHeroData> p_hero = m_chardata.m_heros.GetHero(pwc->m_defense_hid);
                        if (p_hero.get() && p_hero->m_baseHero.get())
                        {
                            p_hero->m_state = HERO_STATE_CITY;
                            p_hero->m_city = tmp_city;
                        }
                    }
                    pwc->addViewer(m_chardata.m_id);
                }
            }
        }
        else
        {
            ERR();
        }
    }
    q.free_result();
    if (m_view_citys.size() < iWildCityCnt)
    {
        wildViewRefresh();
    }
    return 0;
}

int CharWildCitys::wildLevy(int id, json_spirit::Object& robj)
{
    time_t t_now = time(NULL);
    if (id > 0)
    {
        wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(id);
        if (pwc && m_chardata.m_id == pwc->m_owner_cid && pwc->m_defense_hid > 0 && pwc->levy_get() > 0)
        {
            int silver = pwc->levy_get();
            m_chardata.addSilver(silver, silver_get_wild_levy);
            int last_min = (t_now - pwc->m_levy_start) / 60;
            pwc->m_levy_start = t_now;
            pwc->m_notify = false;
            pwc->m_get_notify = false;
            pwc->save();
            robj.push_back( Pair("last_min", last_min));
            robj.push_back( Pair("get_levy", silver));
            m_chardata.m_tasks.updateTask(GOAL_DAILY_WILD_LEVY, 0, silver);
            return HC_SUCCESS;
        }
    }
    else
    {
        if (m_chardata.m_vip < iWildCityFastLevyVIP)
            return HC_ERROR_NEED_MORE_VIP;
        int total = 0;
        for(std::vector<int>::iterator it = m_view_citys.begin(); it != m_view_citys.end(); ++it)
        {
            wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(*it);
            if (pwc && m_chardata.m_id == pwc->m_owner_cid && pwc->m_defense_hid > 0 && pwc->levy_get() > 0)
            {
                total += pwc->levy_get();
                pwc->m_levy_start = t_now;
                pwc->m_notify = false;
                pwc->m_get_notify = false;
                pwc->save();
            }
        }
        if (total > 0)
        {
            m_chardata.addSilver(total, silver_get_wild_levy);
            robj.push_back( Pair("get_levy", total));
            m_chardata.m_tasks.updateTask(GOAL_DAILY_WILD_LEVY, 0, total);
            if (total > 300000)
            {
        		std::string msg = strLevyGet300KMsg;
    			str_replace(msg, "$W", MakeCharNameLink(m_chardata.m_name,m_chardata.m_nick.get_string()));
    			str_replace(msg, "$G", LEX_CAST_STR(total));
        		if (msg != "")
        		{
        			GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        		}
            }
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int CharWildCitys::wildDefense(int purpose, int id, int hid, json_spirit::Object& robj)
{
    time_t t_now = time(NULL);
    wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(id);
    if (pwc == NULL)
        return HC_ERROR;
    if (purpose == 1)
    {
        //设置城守
        if (m_chardata.m_id == pwc->m_owner_cid)
        {
            boost::shared_ptr<CharHeroData> now_hero = m_chardata.m_heros.GetHero(hid);
            if (now_hero.get() && now_hero->m_baseHero.get() && now_hero->m_state == HERO_STATE_INIT)
            {
                if ((now_hero->m_star > 3 && m_chardata.m_level <= 30)
                    || (now_hero->m_star > 4 && m_chardata.m_level <= 40))
                {
                    return HC_ERROR_NEED_MORE_LEVEL;
                }
                if (pwc->m_defense_hid > 0 && pwc->m_defense_hid != hid)
                {
                    boost::shared_ptr<CharHeroData> p_hero = m_chardata.m_heros.GetHero(pwc->m_defense_hid);
                    if (p_hero.get() && p_hero->m_baseHero.get())
                    {
                        p_hero->m_state = HERO_STATE_INIT;
                        p_hero->m_city = 0;
                        p_hero->m_changed = true;
                        p_hero->updateAttribute();
                        if (pwc->levy_get() > 0)
                        {
                            m_chardata.addSilver(pwc->levy_get(), silver_get_wild_levy);
                            pwc->m_levy_start = 0;
                        }
                    }
                    now_hero->m_state = HERO_STATE_CITY;
                    now_hero->m_city = id;
                    now_hero->m_changed = true;
                    now_hero->updateAttribute();
                    pwc->m_defense_hid = hid;
                    pwc->m_levy_start = t_now;
                    pwc->m_notify = false;
                    pwc->m_get_notify = false;
                    pwc->save();
                    return HC_SUCCESS;
                }
                else if(pwc->m_defense_hid == 0)
                {
                    now_hero->m_state = HERO_STATE_CITY;
                    now_hero->m_city = id;
                    now_hero->m_changed = true;
                    now_hero->updateAttribute();
                    pwc->m_defense_hid = hid;
                    pwc->m_levy_start = t_now;
                    pwc->m_notify = false;
                    pwc->m_get_notify = false;
                    pwc->save();
                    return HC_SUCCESS;
                }
            }
        }
    }
    else if(purpose == 2)
    {
        //取消城守
        if (m_chardata.m_id == pwc->m_owner_cid)
        {
            if (pwc->m_defense_hid > 0)
            {
                boost::shared_ptr<CharHeroData> p_hero = m_chardata.m_heros.GetHero(pwc->m_defense_hid);
                if (p_hero.get() && p_hero->m_baseHero.get())
                {
                    if (pwc->levy_get() > 0)
                    {
                        m_chardata.addSilver(pwc->levy_get(), silver_get_wild_levy);
                        pwc->m_levy_start = 0;
                    }
                    p_hero->m_state = HERO_STATE_INIT;
                    p_hero->m_city = 0;
                    p_hero->updateAttribute();
                    p_hero->m_changed = true;
                    pwc->m_defense_hid = 0;
                    pwc->save();
                    return HC_SUCCESS;
                }
            }
        }
    }
    return HC_ERROR;
}

void CharWildCitys::wildViewRefresh()
{
    for(std::vector<int>::iterator it = m_view_citys.begin(); it != m_view_citys.end(); ++it)
    {
        wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(*it);
        if (pwc && m_chardata.m_id == pwc->m_owner_cid)
        {
            ;//保留自己占领的
        }
        else
        {
            if (pwc)
            {
                pwc->removeViewer(m_chardata.m_id);
            }
            *it = 0;
        }
    }
    while(m_view_citys.size() < iWildCityCnt)
    {
        m_view_citys.push_back(0);
    }
    for(std::vector<int>::iterator it = m_view_citys.begin(); it != m_view_citys.end(); ++it)
    {
        if (*it == 0)
        {
            int tmp_city = 0;
            while(checkView(tmp_city))
            {
                tmp_city = Singleton<wildMgr>::Instance().getRandomWildCity();
            }
            wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(tmp_city);
            if (pwc)
            {
                pwc->addViewer(m_chardata.m_id);
            }
            else
            {
                it = m_view_citys.begin();
                continue;
            }
            *it = tmp_city;
        }
    }
    save();
}

bool CharWildCitys::checkView(int id)
{
    for(std::vector<int>::iterator it = m_view_citys.begin(); it != m_view_citys.end(); ++it)
    {
        if (id == *it)
            return true;
    }
    return false;
}

int CharWildCitys::getOwnCnt()
{
    int cnt = 0;
    for(std::vector<int>::iterator it = m_view_citys.begin(); it != m_view_citys.end(); ++it)
    {
        wild_city* pwc = Singleton<wildMgr>::Instance().getWildCity(*it);
        if (pwc && m_chardata.m_id == pwc->m_owner_cid)
        {
            ++cnt;
        }
    }
    return cnt;
}

void CharWildCitys::save()
{
    const json_spirit::Value val_citys(m_view_citys.begin(), m_view_citys.end());
    InsertSaveDb("replace into char_view_wild_citys (cid,citys) values (" + LEX_CAST_STR(m_chardata.m_id)
        + ",'" + json_spirit::write(val_citys) + "')");
}

wildMgr::wildMgr()
{
    Query q(GetDb());
    //基础城外配置信息
    q.get_result("select pos,name,x,y from base_wild_citys where 1 order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int type = q.getval();
        boost::shared_ptr<base_wild_city_info> bc;
        bc.reset(new base_wild_city_info);
        bc->pos = type;
        bc->name = q.getstr();
        bc->x = q.getval();
        bc->y = q.getval();
        base_wildinfo_list.push_back(bc);
    }
    q.free_result();
    //城池总数量
    int city_total = GeneralDataMgr::getInstance()->getInt("wild_city_total", iWildCityTotal);
    m_max_wild_city_id = 0;
    //野外城池占领情况
    q.get_result("select id,cid,hid,levy_start from wild_citys where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<wild_city> pwc;
        pwc.reset(new wild_city);
        pwc->m_id = q.getval();
        pwc->m_owner_cid = q.getval();
        pwc->m_defense_hid = q.getval();
        pwc->m_levy_start = q.getval();
        pwc->m_fight = 0;
        m_wild_citys.push_back(pwc);
        assert(m_wild_citys.size() == pwc->m_id);
        m_max_wild_city_id = pwc->m_id;
    }
    q.free_result();
    while(m_max_wild_city_id < city_total)
    {
        boost::shared_ptr<wild_city> pwc;
        pwc.reset(new wild_city);
        pwc->m_id = ++m_max_wild_city_id;
        pwc->reset();
        m_wild_citys.push_back(pwc);
        assert(m_wild_citys.size() == pwc->m_id);
        InsertSaveDb("replace into wild_citys (id,cid,hid,levy_start) values (" + LEX_CAST_STR(pwc->m_id)
            + "," + LEX_CAST_STR(pwc->m_owner_cid)
            + "," + LEX_CAST_STR(pwc->m_defense_hid)
            + "," + LEX_CAST_STR(pwc->m_levy_start) + ")");
    }
}

boost::shared_ptr<base_wild_city_info> wildMgr::getWildInfo(int pos)
{
    if (pos >= 1 && pos <= base_wildinfo_list.size())
    {
        return base_wildinfo_list[pos-1];
    }
    boost::shared_ptr<base_wild_city_info> p;
    p.reset();
    return p;
}

wild_city* wildMgr::getWildCity(int id)
{
    if (id >= 1 && id <= m_max_wild_city_id)
    {
        return m_wild_citys[id-1].get();
    }
    return NULL;
}

int wildMgr::getRandomWildCity()
{
    return my_random(1, m_max_wild_city_id);
}

void wildMgr::checkWildCity()
{
    for (int i = 0; i < m_max_wild_city_id; ++i)
    {
        if (m_wild_citys[i].get())
        {
            if (m_wild_citys[i]->m_owner_cid > 0
                && m_wild_citys[i]->m_defense_hid > 0
                && !m_wild_citys[i]->m_get_notify
                && m_wild_citys[i]->levy_get() > 0)
            {
                m_wild_citys[i]->broadCastInfo();
            }
            if (m_wild_citys[i]->m_owner_cid > 0
                && m_wild_citys[i]->m_defense_hid > 0
                && !m_wild_citys[i]->m_notify
                && m_wild_citys[i]->levy_get() > iWildCityNotifyLevy)
            {
                m_wild_citys[i]->broadCastInfo();
            }
        }
    }
    return;
}

int wildMgr::combatResult(chessCombat* pCombat)    //战斗结束
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (COMBAT_TYPE_WILD_CITY != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_players[0].m_cid);
    if (!cdata.get())
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> def_cdata = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_players[1].m_cid);
    if (!def_cdata.get())
    {
        ERR();
        return HC_ERROR;
    }
    wild_city* pwc = getWildCity(pCombat->m_data_id);
    if (pwc == NULL)
    {
        ERR();
        return HC_ERROR;
    }
    cout << "wildMgr::combatResult   " << pCombat->m_combat_id << ",reuslt=" << pCombat->m_result << endl;
    if (pCombat->m_result == COMBAT_RESULT_ATTACK_WIN)
    {
        //更改归属权前先帮原主人征收
        int total_silver = pwc->levy_get();
        int sub_silver = total_silver / 2;
        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(pCombat->m_players[1].m_cid);
        if (cc)
        {
            sub_silver = sub_silver * (100-cc->gerRobDefense()) / 100;
        }
        if (total_silver - sub_silver > 0)
        {
            def_cdata->addSilver(total_silver - sub_silver, silver_get_wild_levy);
        }
        if (pwc->m_defense_hid > 0)
        {
            boost::shared_ptr<CharHeroData> p_hero = def_cdata->m_heros.GetHero(pwc->m_defense_hid);
            if (p_hero.get() && p_hero->m_baseHero.get())
            {
                p_hero->m_state = HERO_STATE_INIT;
                p_hero->m_city = 0;
                p_hero->m_changed = true;
                p_hero->updateAttribute();
            }
        }
        pwc->reset();
        pwc->m_owner_cid = pCombat->m_players[0].m_cid;
        pwc->broadCastInfo();
        pwc->save();
        std::string content = strWildWinMailContent;
        str_replace(content, "$N", MakeCharNameLink_other(cdata->m_name));
        str_replace(content, "$G", LEX_CAST_STR(total_silver));
        str_replace(content, "$L", LEX_CAST_STR(sub_silver));
        sendSystemMail(def_cdata->m_name, def_cdata->m_id, strWildMailTitle, content);
        Singleton<goalMgr>::Instance().updateTask(cdata->m_id, GOAL_TYPE_WILD_WIN, 1);
    }
    else if (pCombat->m_result == COMBAT_RESULT_ATTACK_LOSE)
    {
        pwc->m_fight = 0;
        pwc->broadCastInfo();
    }
    return HC_SUCCESS;
}

