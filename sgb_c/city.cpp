
#include "city.h"
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

Database& GetDb();
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

//获取城内建筑列表
int ProcessQueryCityBuildingList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    json_spirit::Array list;
    for(int building_type = 1; building_type <= BUILDING_TYPE_MAX; ++building_type)
    {
        json_spirit::Object o;
        o.push_back( Pair("type", building_type));
        if (building_type == BUILDING_TYPE_CASTLE)
        {
            char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(cdata->m_id);
            if (cc)
            {
                o.push_back( Pair("level", cc->m_level));
            }
        }
        else if(building_type == BUILDING_TYPE_METALLURGY)
        {
            boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(cdata->m_id);
            if (cm.get())
            {
                o.push_back( Pair("level", cm->m_level));
            }
        }
        else if(building_type == BUILDING_TYPE_SMITHY)
        {
            boost::shared_ptr<char_smithy> cs = Singleton<cityMgr>::Instance().getCharSmithy(cdata->m_id);
            if (cs.get())
            {
                o.push_back( Pair("level", cs->m_level));
            }
        }
        else if(building_type == BUILDING_TYPE_BARRACKS)
        {
            boost::shared_ptr<char_barracks> cb = Singleton<cityMgr>::Instance().getCharBarracks(cdata->m_id);
            if (cb.get())
            {
                o.push_back( Pair("level", cb->m_level));
            }
        }
        else
        {
            o.push_back( Pair("level", 1));
        }
        boost::shared_ptr<base_city_building_info> bb = Singleton<cityMgr>::Instance().getBuildingInfo(building_type);
        if (bb.get())
        {
            o.push_back( Pair("open_level", bb->open_level));
            o.push_back( Pair("name", bb->name));
            o.push_back( Pair("x", bb->x));
            o.push_back( Pair("y", bb->y));
        }
        list.push_back(o);
    }
    robj.push_back( Pair("list", list));
    return HC_SUCCESS;
}

//获取城内建筑信息
int ProcessQueryCityBuilding(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int building_type = 0;
    READ_INT_FROM_MOBJ(building_type, o, "building_type");
    robj.push_back( Pair("building_type", building_type));
    if (building_type == BUILDING_TYPE_CASTLE)
    {
        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(cdata->m_id);
        if (cc)
        {
            cc->toObj(robj);
            return HC_SUCCESS;
        }
    }
    else if(building_type == BUILDING_TYPE_METALLURGY)
    {
        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(cdata->m_id);
        if (cc)
        {
            robj.push_back( Pair("castle_level", cc->m_level));
        }
        boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(cdata->m_id);
        if (cm.get())
        {
            cm->toObj(robj);
            return HC_SUCCESS;
        }
    }
    else if(building_type == BUILDING_TYPE_SMITHY)
    {
        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(cdata->m_id);
        if (cc)
        {
            robj.push_back( Pair("castle_level", cc->m_level));
        }
        boost::shared_ptr<char_smithy> cs = Singleton<cityMgr>::Instance().getCharSmithy(cdata->m_id);
        if (cs.get())
        {
            cs->toObj(robj);
            return HC_SUCCESS;
        }
    }
    else if(building_type == BUILDING_TYPE_BARRACKS)
    {
        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(cdata->m_id);
        if (cc)
        {
            robj.push_back( Pair("castle_level", cc->m_level));
        }
        boost::shared_ptr<char_barracks> cs = Singleton<cityMgr>::Instance().getCharBarracks(cdata->m_id);
        if (cs.get())
        {
            cs->toObj(robj);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//升级城内建筑
int ProcessLevelUpCityBuilding(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int building_type = 0;
    READ_INT_FROM_MOBJ(building_type, o, "building_type");
    robj.push_back( Pair("building_type", building_type));
    if (building_type == BUILDING_TYPE_CASTLE)
    {
        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(cdata->m_id);
        if (cc)
        {
            return cc->levelup(robj);
        }
    }
    else if(building_type == BUILDING_TYPE_METALLURGY)
    {
        boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(cdata->m_id);
        if (cm.get())
        {
            return cm->levelup(robj);
        }
    }
    else if(building_type == BUILDING_TYPE_SMITHY)
    {
        boost::shared_ptr<char_smithy> cs = Singleton<cityMgr>::Instance().getCharSmithy(cdata->m_id);
        if (cs.get())
        {
            return cs->levelup(robj);
        }
    }
    else if(building_type == BUILDING_TYPE_BARRACKS)
    {
        boost::shared_ptr<char_barracks> cb = Singleton<cityMgr>::Instance().getCharBarracks(cdata->m_id);
        if (cb.get())
        {
            return cb->levelup(robj);
        }
    }
    return HC_ERROR;
}

//招募居民
int ProcessRecruit(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(cdata->m_id);
    if (cc)
    {
        return cc->recruit(robj);
    }
    return HC_ERROR;
}

//招募按钮更新
int ProcessCityRecruitUpdate(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid,o,"cid");
    //通知客户端
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (cdata.get())
    {
        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(cdata->m_id);
        if (cc)
        {
            time_t t_now = time(NULL);
            int active = cc->m_recruit_cd <= t_now ? 1 : 0;
            cdata->updateTopButton(top_button_city_recruit, active);
        }
    }
    return HC_SUCCESS;
}

//城堡收税
int ProcessLevy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(cdata->m_id);
    if (cc)
    {
        return cc->levy(type == 2, robj);
    }
    return HC_ERROR;
}

//熔炼刷新按钮更新
int ProcessSmeltRefreshUpdate(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid,o,"cid");
    //通知客户端
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (cdata.get())
    {
        boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(cdata->m_id);
        if (cm.get() && cm->getSmeltCnt())
        {
            time_t t_now = time(NULL);
            time_t refresh = cdata->queryExtraData(char_data_type_normal,char_data_normal_smelt_hero_refresh);
            int active = refresh <= t_now ? 1 : 0;
            cdata->updateTopButton(top_button_smelt_refresh, active);
        }
    }
    return HC_SUCCESS;
}

int char_castle::levelup(json_spirit::Object& robj)
{
    base_castle* bc = Singleton<cityMgr>::Instance().getCastle(m_level + 1);
    if (bc)
    {
        if (m_chardata.m_level < bc->m_need_level)
        {
            return HC_ERROR_NEED_MORE_LEVEL;
        }
        if (bc->m_need_cost < 0 || m_chardata.subSilver(bc->m_need_cost, silver_cost_castle_levelup) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_SILVER;
        }
        m_level = m_level + 1;
        m_resident = (m_resident < bc->m_min_resident) ? bc->m_min_resident : m_resident;
        save();
        robj.push_back( Pair("level", m_level));
        m_chardata.m_tasks.updateTask(GOAL_CASTLE_LEVEL, 0, m_level);
        Singleton<goalMgr>::Instance().updateTask(m_chardata.m_id, GOAL_TYPE_CASTLE_LEVEL, m_level);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int char_castle::recruit(json_spirit::Object& robj)
{
    time_t t_now = time(NULL);
    if (t_now < m_recruit_cd)
    {
        return HC_ERROR_IN_COOLTIME;
    }
    base_castle* bc = Singleton<cityMgr>::Instance().getCastle(m_level);
    if (bc && m_resident < bc->m_max_resident)
    {
        int old_resident = m_resident;
        m_resident += bc->m_output_resident;
        if (m_resident > bc->m_max_resident)
            m_resident = bc->m_max_resident;
        m_recruit_cd = t_now + iRecruitCD;
        save();
        m_chardata.m_heros.updateAttribute();
        robj.push_back( Pair("get_resident", m_resident - old_resident));
        m_chardata.m_tasks.updateTask(GOAL_RECRUIT, 0, 1);
        m_chardata.m_tasks.updateTask(GOAL_DAILY_RECRUIT, 0, 1);
        Singleton<goalMgr>::Instance().updateTask(m_chardata.m_id, GOAL_TYPE_RECRUIT, 1);
        Singleton<goalMgr>::Instance().updateTask(m_chardata.m_id, GOAL_TYPE_RESIDENT, m_resident);
        m_chardata.updateTopButton(top_button_city_recruit, 0);
        //插入定时器
        json_spirit::mObject mobj;
        mobj["cmd"] = "cityRecruitUpdate";
        mobj["cid"] = m_chardata.m_id;
        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(iRecruitCD, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void char_castle::resident_away()
{
    base_castle* bc = Singleton<cityMgr>::Instance().getCastle(m_level);
    if (bc && m_resident > bc->m_min_resident)
    {
        --m_resident;
        save();
        m_chardata.m_heros.updateAttribute();
    }
    return;
}

void char_castle::cal_add(int& att_per, int& def_per, int& levy_per)
{
    base_castle* bc = Singleton<cityMgr>::Instance().getCastle(m_level);
    if (bc)
    {
        int tmp = bc->m_max_resident / 3;
        if (bc->m_max_resident % 3 > 0)
            ++tmp;
        int tmp2 = m_resident / tmp;
        if (m_resident == bc->m_max_resident)
            tmp2 = 3;
        att_per = iResidentBuff[0] * tmp2;
        def_per = iResidentBuff[1] * tmp2;
        levy_per = iResidentBuff[2] * tmp2;
    }
    return;
}

int char_castle::gerRobDefense()
{
    base_castle* bc = Singleton<cityMgr>::Instance().getCastle(m_level);
    if (bc)
    {
        return bc->m_rob_defense;
    }
    return 0;
}

void char_castle::getDefenseAdd(int& att_per, int& def_per, int& magic_per, int& hp_per)
{
    base_castle* bc = Singleton<cityMgr>::Instance().getCastle(m_level);
    if (bc)
    {
        att_per = bc->m_attack_per;
        def_per = bc->m_defense_per;
        magic_per = bc->m_magic_per;
        hp_per = bc->m_hp_per;
    }
    return;
}

int char_castle::levy_get()
{
    int get_silver = 0;
    base_castle* bc = Singleton<cityMgr>::Instance().getCastle(m_level);
    if (bc)
    {
        get_silver = bc->m_output_silver;
        int att_per = 0, def_per = 0, levy_per = 0;
        cal_add(att_per,def_per,levy_per);
        get_silver = get_silver * (100+levy_per) / 100;
    }
    #ifdef QQ_PLAT
    if (m_chardata.m_qq_yellow_level > 0)
    {
        get_silver = get_silver * 110 / 100;
    }
    #endif
    return get_silver;
}

int char_castle::levy_left_times()
{
    int times = 0;
    if (m_levy == 0)
        ++times;
    int free_times = m_chardata.queryExtraData(char_data_type_daily,char_data_daily_free_levy);
    if (free_times > 0)
    {
        times += free_times;
    }
    int gold_times = m_chardata.queryExtraData(char_data_type_daily,char_data_daily_gold_levy);
    if (gold_times < iLevyGoldTime[m_chardata.m_vip])
    {
        times += (iLevyGoldTime[m_chardata.m_vip]-gold_times);
    }
    return times;
}

int char_castle::levy(bool cost, json_spirit::Object& robj)
{
    int free_times = m_chardata.queryExtraData(char_data_type_daily,char_data_daily_free_levy);
    if (m_levy > 0 && !cost)
    {
        if (free_times <= 0)
        {
            return HC_ERROR_IN_COOLTIME;
        }
    }
    if (cost)
    {
        int times = m_chardata.queryExtraData(char_data_type_daily,char_data_daily_gold_levy);
        if (times >= iLevyGoldTime[m_chardata.m_vip])
        {
            return HC_ERROR_NEED_MORE_VIP;
        }
        if (m_chardata.subGold(iLevyCost, gold_cost_levy) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        m_chardata.setExtraData(char_data_type_daily,char_data_daily_gold_levy,++times);
    }
    int get_silver = levy_get();
    if (get_silver > 0)
    {
        if (m_levy > 0 && !cost)
        {
            m_chardata.setExtraData(char_data_type_daily,char_data_daily_free_levy, --free_times);
        }
        m_levy = 1;
        m_chardata.addSilver(get_silver, silver_get_levy);
        save();
        robj.push_back( Pair("get_levy", get_silver));
        //任务
        m_chardata.m_tasks.updateTask(GOAL_LEVY, 0, 1);
        m_chardata.m_tasks.updateTask(GOAL_DAILY_LEVY, 0, 1);
        if (cost)
            m_chardata.m_tasks.updateTask(GOAL_DAILY_LEVY_GOLD, 0, 1);
        //目标
        Singleton<goalMgr>::Instance().updateTask(m_chardata.m_id, GOAL_TYPE_LEVY, 1);
        int active = (m_levy == 0 || free_times > 0) ? 1 : 0;
        m_chardata.updateTopButton(top_button_city_levy, active);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void char_castle::toObj(json_spirit::Object& robj)
{
    time_t time_now = time(NULL);
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&time_now, t);
    base_castle* bc = Singleton<cityMgr>::Instance().getCastle(m_level);
    if (bc)
    {
        json_spirit::Object castle_info;
        castle_info.push_back( Pair("level", m_level));
        castle_info.push_back( Pair("resident", m_resident));
        castle_info.push_back( Pair("resident_max", bc->m_max_resident));
        castle_info.push_back( Pair("output_resident", bc->m_output_resident));
        castle_info.push_back( Pair("rob_def", bc->m_rob_defense));
        castle_info.push_back( Pair("attack_per", bc->m_attack_per));
        castle_info.push_back( Pair("defense_per", bc->m_defense_per));
        castle_info.push_back( Pair("magic_per", bc->m_magic_per));
        castle_info.push_back( Pair("hp_per", bc->m_hp_per));
        castle_info.push_back( Pair("levy_get", levy_get()));
        int free_times = m_chardata.queryExtraData(char_data_type_daily,char_data_daily_free_levy);
        castle_info.push_back( Pair("levy_state", (m_levy == 0 || free_times > 0)) );
        std::string nextTime = "12:00";
        int leftTimes = 0;
        {
            if (t->tm_hour < 8)
            {
                nextTime = "8:00";
                leftTimes = 2;
            }
            else if (t->tm_hour < 20)
            {
                nextTime = "20:00";
                leftTimes = 1;
            }
        }
        if (0 == m_levy)
        {
            ++leftTimes;
        }
        castle_info.push_back( Pair("levy_leftTimes", leftTimes) );
        castle_info.push_back( Pair("levy_nextTime", nextTime) );
        castle_info.push_back( Pair("levy_gold", iLevyCost) );
        int times = m_chardata.queryExtraData(char_data_type_daily,char_data_daily_gold_levy);
        castle_info.push_back( Pair("levy_goldLeftTimes", iLevyGoldTime[m_chardata.m_vip]-times) );
        castle_info.push_back( Pair("levy_goldTotalTimes", iLevyGoldTime[m_chardata.m_vip]) );
        castle_info.push_back( Pair("recruit_cd", m_recruit_cd - time_now));
        
        int att_per = 0, def_per = 0, levy_per = 0;
        cal_add(att_per,def_per,levy_per);
        json_spirit::Object resident_buff;
        resident_buff.push_back( Pair("att_per", att_per));
        resident_buff.push_back( Pair("def_per", def_per));
        resident_buff.push_back( Pair("levy_per", levy_per));
        castle_info.push_back( Pair("resident_buff", resident_buff));
        
        if (m_resident < bc->m_max_resident)
        {
            int per = bc->m_max_resident / 3;
            if (bc->m_max_resident % 3 > 0)
                ++per;
            int need_total = (m_resident / per + 1) * per;
            if (need_total > bc->m_max_resident)
                need_total = bc->m_max_resident;
            need_total -= m_resident;
            int need_resident_times = need_total / bc->m_output_resident;
            if (need_total % bc->m_output_resident > 0)
                ++need_resident_times;
            json_spirit::Object next_resident_buff;
            next_resident_buff.push_back( Pair("att_per", att_per+iResidentBuff[0]));
            next_resident_buff.push_back( Pair("def_per", def_per+iResidentBuff[1]));
            next_resident_buff.push_back( Pair("levy_per", levy_per+iResidentBuff[2]));
            next_resident_buff.push_back( Pair("need_resident_times", need_resident_times));
            castle_info.push_back( Pair("next_resident_buff", next_resident_buff));
        }
        
        robj.push_back( Pair("info", castle_info));
    }
    base_castle* next_bc = Singleton<cityMgr>::Instance().getCastle(m_level + 1);
    if (next_bc)
    {
        json_spirit::Object castle_info;
        castle_info.push_back( Pair("level", next_bc->m_level));
        castle_info.push_back( Pair("resident_min", next_bc->m_min_resident));
        castle_info.push_back( Pair("resident_max", next_bc->m_max_resident));
        castle_info.push_back( Pair("output_resident", next_bc->m_output_resident));
        castle_info.push_back( Pair("rob_def", next_bc->m_rob_defense));
        castle_info.push_back( Pair("attack_per", next_bc->m_attack_per));
        castle_info.push_back( Pair("defense_per", next_bc->m_defense_per));
        castle_info.push_back( Pair("magic_per", next_bc->m_magic_per));
        castle_info.push_back( Pair("hp_per", next_bc->m_hp_per));
        castle_info.push_back( Pair("need_level", next_bc->m_need_level));
        castle_info.push_back( Pair("need_cost", next_bc->m_need_cost));
        
        robj.push_back( Pair("next_info", castle_info));
    }
}

void char_castle::save()
{
    InsertSaveDb("replace into char_castles (cid,level,resident,recruit_cd,levy) values (" + LEX_CAST_STR(m_chardata.m_id)
        + "," + LEX_CAST_STR(m_level) + "," + LEX_CAST_STR(m_resident) + "," + LEX_CAST_STR(m_recruit_cd) + "," + LEX_CAST_STR(m_levy) + ")");
}

int char_metallurgy::levelup(json_spirit::Object& robj)
{
    char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(m_chardata.m_id);
    if (cc)
    {
        boost::shared_ptr<base_metallurgy> bm = Singleton<cityMgr>::Instance().getMetallurgy(m_level+1);
        if (bm.get())
        {
            if (cc->m_level < bm->m_need_level)
            {
                return HC_ERROR_NEED_MORE_CASTLE_LEVEL;
            }
            if (bm->m_need_cost < 0 || m_chardata.subSilver(bm->m_need_cost, silver_cost_metallurgy_levelup) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_SILVER;
            }
            if (getSmeltCnt() == 0 && bm->m_smelt_cnt > 0)
            {
                m_chardata.addTopButton(top_button_smelt_refresh,1);
            }
            m_level = m_level + 1;
            save();
            robj.push_back( Pair("level", m_level));
            m_chardata.m_tasks.updateTask(GOAL_METALLURGY_LEVEL, 0, m_level);
            Singleton<goalMgr>::Instance().updateTask(m_chardata.m_id, GOAL_TYPE_METALLURGY_LEVEL, m_level);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

double char_metallurgy::getCompoundAdd()
{
    boost::shared_ptr<base_metallurgy> bc = Singleton<cityMgr>::Instance().getMetallurgy(m_level);
    if (bc.get())
    {
        return bc->m_compound_add;
    }
    return 0.0;
}

int char_metallurgy::getCompoundMaxStar()
{
    boost::shared_ptr<base_metallurgy> bc = Singleton<cityMgr>::Instance().getMetallurgy(m_level);
    if (bc.get())
    {
        return bc->m_compound_star;
    }
    return 0;
}

int char_metallurgy::getDecomposeMaxStar()
{
    boost::shared_ptr<base_metallurgy> bc = Singleton<cityMgr>::Instance().getMetallurgy(m_level);
    if (bc.get())
    {
        return bc->m_decompose_star;
    }
    return 0;
}

int char_metallurgy::getGoldenMaxStar()
{
    boost::shared_ptr<base_metallurgy> bc = Singleton<cityMgr>::Instance().getMetallurgy(m_level);
    if (bc.get())
    {
        return bc->m_golden_star;
    }
    return 0;
}

int char_metallurgy::getSmeltCnt()
{
    boost::shared_ptr<base_metallurgy> bc = Singleton<cityMgr>::Instance().getMetallurgy(m_level);
    if (bc.get())
    {
        return bc->m_smelt_cnt;
    }
    return 0;
}

void char_metallurgy::toObj(json_spirit::Object& robj)
{
    boost::shared_ptr<base_metallurgy> bc = Singleton<cityMgr>::Instance().getMetallurgy(m_level);
    if (bc.get())
    {
        json_spirit::Object metallurgy_info;
        metallurgy_info.push_back( Pair("level", m_level));
        metallurgy_info.push_back( Pair("city_per", bc->m_compound_add));
        metallurgy_info.push_back( Pair("compound_star", bc->m_compound_star));
        metallurgy_info.push_back( Pair("decompose_star", bc->m_decompose_star));
        metallurgy_info.push_back( Pair("golden_star", bc->m_golden_star));
        metallurgy_info.push_back( Pair("smelt_cnt", bc->m_smelt_cnt));
        robj.push_back( Pair("info", metallurgy_info));
    }
    boost::shared_ptr<base_metallurgy> next_bc = Singleton<cityMgr>::Instance().getMetallurgy(m_level + 1);
    if (next_bc.get())
    {
        json_spirit::Object metallurgy_info;
        metallurgy_info.push_back( Pair("level", next_bc->m_level));
        metallurgy_info.push_back( Pair("city_per", next_bc->m_compound_add));
        metallurgy_info.push_back( Pair("compound_star", next_bc->m_compound_star));
        metallurgy_info.push_back( Pair("decompose_star", next_bc->m_decompose_star));
        metallurgy_info.push_back( Pair("golden_star", next_bc->m_golden_star));
        metallurgy_info.push_back( Pair("smelt_cnt", next_bc->m_smelt_cnt));
        metallurgy_info.push_back( Pair("need_level", next_bc->m_need_level));
        metallurgy_info.push_back( Pair("need_cost", next_bc->m_need_cost));
        robj.push_back( Pair("next_info", metallurgy_info));
    }
}

void char_metallurgy::save()
{
    InsertSaveDb("replace into char_metallurgys (cid,level) values (" + LEX_CAST_STR(m_chardata.m_id)
        + "," + LEX_CAST_STR(m_level) + ")");
}

int char_smithy::levelup(json_spirit::Object& robj)
{
    char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(m_chardata.m_id);
    if (cc)
    {
        boost::shared_ptr<base_smithy> bs = Singleton<cityMgr>::Instance().getSmithy(m_level+1);
        if (bs.get())
        {
            if (cc->m_level < bs->m_need_level)
            {
                ERR();
                return HC_ERROR_NEED_MORE_CASTLE_LEVEL;
            }
            if (bs->m_need_cost < 0 || m_chardata.subSilver(bs->m_need_cost, silver_cost_smithy_levelup) < 0)
            {
                ERR();
                return HC_ERROR_NOT_ENOUGH_SILVER;
            }
            m_level = m_level + 1;
            save();
            robj.push_back( Pair("level", m_level));
            m_chardata.m_tasks.updateTask(GOAL_SMITHY_LEVEL, 0, m_level);
            Singleton<goalMgr>::Instance().updateTask(m_chardata.m_id, GOAL_TYPE_SMITHY_LEVEL, m_level);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

double char_smithy::getCompoundAdd()
{
    boost::shared_ptr<base_smithy> bc = Singleton<cityMgr>::Instance().getSmithy(m_level);
    if (bc.get())
    {
        return bc->m_compound_add;
    }
    return 0.0;
}

void char_smithy::toObj(json_spirit::Object& robj)
{
    boost::shared_ptr<base_smithy> bc = Singleton<cityMgr>::Instance().getSmithy(m_level);
    if (bc.get())
    {
        json_spirit::Object smithy_info;
        smithy_info.push_back( Pair("level", m_level));
        smithy_info.push_back( Pair("city_per", bc->m_compound_add));
        robj.push_back( Pair("info", smithy_info));
    }
    boost::shared_ptr<base_smithy> next_bc = Singleton<cityMgr>::Instance().getSmithy(m_level + 1);
    if (next_bc.get())
    {
        json_spirit::Object smithy_info;
        smithy_info.push_back( Pair("level", next_bc->m_level));
        smithy_info.push_back( Pair("city_per", next_bc->m_compound_add));
        smithy_info.push_back( Pair("need_level", next_bc->m_need_level));
        smithy_info.push_back( Pair("need_cost", next_bc->m_need_cost));
        robj.push_back( Pair("next_info", smithy_info));
    }
}

void char_smithy::save()
{
    InsertSaveDb("replace into char_smithys (cid,level) values (" + LEX_CAST_STR(m_chardata.m_id)
        + "," + LEX_CAST_STR(m_level) + ")");
}

int char_barracks::levelup(json_spirit::Object& robj)
{
    char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(m_chardata.m_id);
    if (cc)
    {
        boost::shared_ptr<base_barracks> bs = Singleton<cityMgr>::Instance().getBarracks(m_level+1);
        if (bs.get())
        {
            if (cc->m_level < bs->m_need_level)
            {
                ERR();
                return HC_ERROR_NEED_MORE_CASTLE_LEVEL;
            }
            if (bs->m_need_cost < 0 || m_chardata.subSilver(bs->m_need_cost, silver_cost_barracks_levelup) < 0)
            {
                ERR();
                return HC_ERROR_NOT_ENOUGH_SILVER;
            }
            m_level = m_level + 1;
            save();
            robj.push_back( Pair("level", m_level));
            m_chardata.m_tasks.updateTask(GOAL_BARRACKS_LEVEL, 0, m_level);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

double char_barracks::getAdd()
{
    boost::shared_ptr<base_barracks> bc = Singleton<cityMgr>::Instance().getBarracks(m_level);
    if (bc.get())
    {
        return bc->m_add;
    }
    return 0.0;
}

void char_barracks::toObj(json_spirit::Object& robj)
{
    boost::shared_ptr<base_barracks> bc = Singleton<cityMgr>::Instance().getBarracks(m_level);
    if (bc.get())
    {
        json_spirit::Object barracks_info;
        barracks_info.push_back( Pair("level", m_level));
        barracks_info.push_back( Pair("city_per", bc->m_add));
        robj.push_back( Pair("info", barracks_info));
    }
    boost::shared_ptr<base_barracks> next_bc = Singleton<cityMgr>::Instance().getBarracks(m_level + 1);
    if (next_bc.get())
    {
        json_spirit::Object barracks_info;
        barracks_info.push_back( Pair("level", next_bc->m_level));
        barracks_info.push_back( Pair("city_per", next_bc->m_add));
        barracks_info.push_back( Pair("need_level", next_bc->m_need_level));
        barracks_info.push_back( Pair("need_cost", next_bc->m_need_cost));
        robj.push_back( Pair("next_info", barracks_info));
    }
}

void char_barracks::save()
{
    InsertSaveDb("replace into char_barracks (cid,level) values (" + LEX_CAST_STR(m_chardata.m_id)
        + "," + LEX_CAST_STR(m_level) + ")");
}

cityMgr::cityMgr()
{
    Query q(GetDb());
    //基础建筑配置信息
    q.get_result("select type,open_level,name,x,y from base_city_buildings where 1 order by type");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int type = q.getval();
        boost::shared_ptr<base_city_building_info> bc;
        bc.reset(new base_city_building_info);
        bc->type = type;
        bc->open_level = q.getval();
        bc->name = q.getstr();
        bc->x = q.getval();
        bc->y = q.getval();
        base_buildinginfo_list.push_back(bc);
    }
    q.free_result();
    
    m_max_castle_level = 0;
    //基础城堡
    q.get_result("select level,need_level,need_cost,min_resident,max_resident,output_resident,output_silver,rob_defense,attack_per,defense_per,magic_per,hp_per from base_castles where 1 order by level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        if (level > 100 || level < 1)
        {
            ERR();
            continue;
        }
        _base_castle[level-1].m_level = level;
        _base_castle[level-1].m_need_level = q.getval();
        _base_castle[level-1].m_need_cost = q.getval();
        _base_castle[level-1].m_min_resident = q.getval();
        _base_castle[level-1].m_max_resident = q.getval();
        _base_castle[level-1].m_output_resident = q.getval();
        _base_castle[level-1].m_output_silver = q.getval();
        _base_castle[level-1].m_rob_defense = q.getval();
        _base_castle[level-1].m_attack_per = q.getval();
        _base_castle[level-1].m_defense_per = q.getval();
        _base_castle[level-1].m_magic_per = q.getval();
        _base_castle[level-1].m_hp_per = q.getval();
        m_max_castle_level = level;
    }
    q.free_result();
    
    m_max_metallurgy_level = 0;
    //基础炼金房
    q.get_result("select level,need_castle_level,need_cost,compound_add,compound_star,decompose_star,golden_star,smelt_cnt from base_metallurgys where 1 order by level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        boost::shared_ptr<base_metallurgy> bm;
        bm.reset(new base_metallurgy);
        bm->m_level = level;
        bm->m_need_level = q.getval();
        bm->m_need_cost = q.getval();
        bm->m_compound_add = q.getnum();
        bm->m_compound_star = q.getval();
        bm->m_decompose_star = q.getval();
        bm->m_golden_star = q.getval();
        bm->m_smelt_cnt = q.getval();
        base_metallurgy_list.push_back(bm);
        m_max_metallurgy_level = level;
    }
    q.free_result();
    
    m_max_smithy_level = 0;
    //基础铁匠铺
    q.get_result("select level,need_castle_level,need_cost,compound_add from base_smithys where 1 order by level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        boost::shared_ptr<base_smithy> bm;
        bm.reset(new base_smithy);
        bm->m_level = level;
        bm->m_need_level = q.getval();
        bm->m_need_cost = q.getval();
        bm->m_compound_add = q.getnum();
        base_smithy_list.push_back(bm);
        m_max_smithy_level = level;
    }
    q.free_result();
    
    m_max_barracks_level = 0;
    //基础练兵营
    q.get_result("select level,need_castle_level,need_cost,reward_add from base_barracks where 1 order by level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        boost::shared_ptr<base_barracks> bm;
        bm.reset(new base_barracks);
        bm->m_level = level;
        bm->m_need_level = q.getval();
        bm->m_need_cost = q.getval();
        bm->m_add = q.getnum();
        base_barracks_list.push_back(bm);
        m_max_barracks_level = level;
    }
    q.free_result();
}

void cityMgr::residentAway()
{
    std::map<int, boost::shared_ptr<char_castle> >::iterator it = m_char_castles.begin();
    while(it != m_char_castles.end())
    {
        char_castle* cc = it->second.get();
        if (cc)
        {
            cc->resident_away();
        }
        ++it;
    }
    InsertSaveDb("update char_castles set resident=resident-1 where resident>1");
    return;
}

void cityMgr::getButton(CharData* pc, json_spirit::Array& list)
{
    time_t t_now = time(NULL);
    char_castle* cc = getCharCastle(pc->m_id);
    if (cc)
    {
        int free_times = pc->queryExtraData(char_data_type_daily,char_data_daily_free_levy);
        int active = (cc->m_levy == 0 || free_times > 0) ? 1 : 0;
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_city_levy) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);

        active = cc->m_recruit_cd <= t_now ? 1 : 0;
        if (active == 0)//不可招募生成定时器
        {
            if (!cc->_uuid.is_nil())
            {
                splsTimerMgr::getInstance()->delTimer(cc->_uuid);
                cc->_uuid = boost::uuids::nil_uuid();
            }
            json_spirit::mObject mobj;
            mobj["cmd"] = "cityRecruitUpdate";
            mobj["cid"] = pc->m_id;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(t_now - cc->m_recruit_cd, 1,mobj,1));
            cc->_uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        obj.clear();
        obj.push_back( Pair("type", top_button_city_recruit) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    boost::shared_ptr<char_metallurgy> cm = getCharMetallurgy(pc->m_id);
    if (cm.get() && cm->getSmeltCnt())
    {
        time_t refresh = pc->queryExtraData(char_data_type_normal,char_data_normal_smelt_hero_refresh);
        int active = refresh <= t_now ? 1 : 0;
        if (active == 0)//不可招募生成定时器
        {
            if (!cm->_uuid.is_nil())
            {
                splsTimerMgr::getInstance()->delTimer(cm->_uuid);
                cm->_uuid = boost::uuids::nil_uuid();
            }
            json_spirit::mObject mobj;
            mobj["cmd"] = "smeltRefreshUpdate";
            mobj["cid"] = pc->m_id;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(t_now - refresh, 1,mobj,1));
            cm->_uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_smelt_refresh) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    return;
}

//重置税收
void cityMgr::resetLevy()
{
    std::map<int, boost::shared_ptr<char_castle> >::iterator it = m_char_castles.begin();
    while(it != m_char_castles.end())
    {
        char_castle* cc = it->second.get();
        if (cc)
        {
            cc->m_levy = 0;
            cc->m_chardata.updateTopButton(top_button_city_levy, 1);
        }
        ++it;
    }
    InsertSaveDb("update char_castles set levy=0 where levy>0");
    return;
}

boost::shared_ptr<base_city_building_info> cityMgr::getBuildingInfo(int type)
{
    if (type >= 1 && type <= base_buildinginfo_list.size())
    {
        return base_buildinginfo_list[type-1];
    }
    boost::shared_ptr<base_city_building_info> p;
    p.reset();
    return p;
}

base_castle* cityMgr::getCastle(int level)
{
    if (level >= 1 && level <= m_max_castle_level)
    {
        return _base_castle + level - 1;
    }
    return NULL;
}

char_castle* cityMgr::getCharCastle(int cid)
{
    if (m_char_castles.find(cid) != m_char_castles.end())
    {
        return m_char_castles[cid].get();
    }
    else
    {
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc)
        {
            Query q(GetDb());
            q.get_result("select level,resident,recruit_cd,levy from char_castles where cid=" + LEX_CAST_STR(cid));
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                char_castle* cc = new char_castle(*pc);
                cc->m_level = q.getval();
                cc->m_resident = q.getval();
                base_castle* bc = getCastle(cc->m_level);
                if (bc && cc->m_resident < bc->m_min_resident)
                {
                    cc->m_resident = bc->m_min_resident;
                }
                cc->m_recruit_cd = q.getval();
                cc->m_levy = q.getval();
                m_char_castles[cid].reset(cc);
                q.free_result();
            }
            else
            {
                q.free_result();
                //没有城堡数据初始化城堡
                char_castle* cc = new char_castle(*pc);
                cc->m_level = 1;
                cc->m_resident = 0;
                base_castle* bc = Singleton<cityMgr>::Instance().getCastle(1);
                if (bc && cc->m_resident < bc->m_max_resident)
                {
                    cc->m_resident = bc->m_min_resident;
                }
                cc->m_recruit_cd = 0;
                cc->m_levy = 0;
                m_char_castles[cid].reset(cc);
                cc->save();
            }
        }
        return m_char_castles[cid].get();
    }
}

boost::shared_ptr<base_metallurgy> cityMgr::getMetallurgy(int level)
{
    if (level >= 1 && level <= m_max_metallurgy_level)
    {
        return base_metallurgy_list[level-1];
    }
    boost::shared_ptr<base_metallurgy> p;
    p.reset();
    return p;
}

int cityMgr::getSmeltOpenLevel(int cnt)
{
    for (int level = 1; level <= m_max_metallurgy_level; ++level)
    {
        if (base_metallurgy_list[level-1].get() && base_metallurgy_list[level-1]->m_smelt_cnt == cnt)
        {
            return base_metallurgy_list[level-1]->m_level;
        }
    }
    return -1;
}

boost::shared_ptr<char_metallurgy> cityMgr::getCharMetallurgy(int cid)
{
    if (m_char_metallurgys.find(cid) != m_char_metallurgys.end())
    {
        return m_char_metallurgys[cid];
    }
    else
    {
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc)
        {
            Query q(GetDb());
            q.get_result("select level from char_metallurgys where cid=" + LEX_CAST_STR(cid));
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                boost::shared_ptr<char_metallurgy> cc;
                cc.reset(new char_metallurgy(*pc));
                cc->m_level = q.getval();
                m_char_metallurgys[cid] = cc;
                q.free_result();
            }
            else
            {
                q.free_result();
                boost::shared_ptr<char_metallurgy> cc;
                cc.reset(new char_metallurgy(*pc));
                cc->m_level = 1;
                m_char_metallurgys[cid] = cc;
                cc->save();
            }
        }
        return m_char_metallurgys[cid];
    }
}

boost::shared_ptr<base_smithy> cityMgr::getSmithy(int level)
{
    if (level >= 1 && level <= m_max_smithy_level)
    {
        return base_smithy_list[level-1];
    }
    boost::shared_ptr<base_smithy> p;
    p.reset();
    return p;
}

boost::shared_ptr<char_smithy> cityMgr::getCharSmithy(int cid)
{
    if (m_char_smithys.find(cid) != m_char_smithys.end())
    {
        return m_char_smithys[cid];
    }
    else
    {
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc)
        {
            Query q(GetDb());
            q.get_result("select level from char_smithys where cid=" + LEX_CAST_STR(cid));
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                boost::shared_ptr<char_smithy> cc;
                cc.reset(new char_smithy(*pc));
                cc->m_level = q.getval();
                m_char_smithys[cid] = cc;
                q.free_result();
            }
            else
            {
                q.free_result();
                boost::shared_ptr<char_smithy> cc;
                cc.reset(new char_smithy(*pc));
                cc->m_level = 1;
                m_char_smithys[cid] = cc;
                cc->save();
            }
        }
        return m_char_smithys[cid];
    }
}

boost::shared_ptr<base_barracks> cityMgr::getBarracks(int level)
{
    if (level >= 1 && level <= m_max_barracks_level)
    {
        return base_barracks_list[level-1];
    }
    boost::shared_ptr<base_barracks> p;
    p.reset();
    return p;
}

boost::shared_ptr<char_barracks> cityMgr::getCharBarracks(int cid)
{
    if (m_char_barracks.find(cid) != m_char_barracks.end())
    {
        return m_char_barracks[cid];
    }
    else
    {
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc)
        {
            Query q(GetDb());
            q.get_result("select level from char_barracks where cid=" + LEX_CAST_STR(cid));
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                boost::shared_ptr<char_barracks> cc;
                cc.reset(new char_barracks(*pc));
                cc->m_level = q.getval();
                m_char_barracks[cid] = cc;
                q.free_result();
            }
            else
            {
                q.free_result();
                boost::shared_ptr<char_barracks> cc;
                cc.reset(new char_barracks(*pc));
                cc->m_level = 1;
                m_char_barracks[cid] = cc;
                cc->save();
            }
        }
        return m_char_barracks[cid];
    }
}

