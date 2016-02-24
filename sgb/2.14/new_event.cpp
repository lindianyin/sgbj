
#include "new_event.h"
#include "singleton.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "spls_errcode.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "statistics.h"

#include "combat.h"
#include "igeneral.h"
#include "loot.h"
#include "SaveDb.h"
#include "daily_task.h"
#include "spls_timer.h"

extern std::string strGeneralEventString;
extern std::string strBaoshiEventString;

int getSessionChar(net::session_ptr& psession, boost::shared_ptr<CharData>& cdata);
Database& GetDb();
int giveLoots(CharData* cdata, Item& getItem, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

//日充值默认额度
#ifdef SF_SERVER
const int iDefaultDailyRecharge = 2000;
#else
const int iDefaultDailyRecharge = 100;
#endif

//查询招募武将活动 cmd：queryGeneralEvent
int ProcessQueryGeneralEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_general_event* e = Singleton<new_event_mgr>::Instance().getCharGeneralEvent(cdata->m_id);
    if (e)
    {
        robj.push_back( Pair("starttime", e->m_start_time) );
        robj.push_back( Pair("endtime", e->m_end_time) );
        robj.push_back( Pair("glist", Singleton<new_event_mgr>::Instance().getGeneralhiEvent()) );
        json_spirit::Array list;
        Singleton<new_event_mgr>::Instance().getGeneralEventList(*e, list);
        robj.push_back( Pair("list", list) );
    }
    return HC_SUCCESS;
}

//查询宝石合成活动 cmd：queryBaoshiEvent
int ProcessQueryBaoshiEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_baoshi_event* e = Singleton<new_event_mgr>::Instance().getCharBaoshiEvent(cdata->m_id);
    if (e)
    {
        robj.push_back( Pair("starttime", e->m_start_time) );
        robj.push_back( Pair("endtime", e->m_end_time) );
        json_spirit::Array list;
        Singleton<new_event_mgr>::Instance().getBaoshiEventList(*e, list);
        robj.push_back( Pair("list", list) );
    }
    return HC_SUCCESS;
}

//查询签到信息
int ProcessQuerySignInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_sign_data* e = Singleton<new_event_mgr>::Instance().getCharSignData(cdata->m_id);
    if (e)
    {
        Singleton<new_event_mgr>::Instance().getSignInfo(*e, robj);
        cdata->notifyEventState(top_level_event_sign, e->m_canGet, e->getOnlineLibaoLeftNum());
        int tmp = cdata->queryExtraData(char_data_type_normal, char_data_first_sign_info);
        if (tmp == 0)
        {
            cdata->_checkGuide(178);
            cdata->setExtraData(char_data_type_normal, char_data_first_sign_info, tmp + 1);
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//签到
int ProcessSign(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_sign_data* e = Singleton<new_event_mgr>::Instance().getCharSignData(cdata->m_id);
    if (e)
    {
        int ret = e->doSign();
        if (HC_SUCCESS == ret)
        {
            cdata->notifyEventState(top_level_event_sign, e->m_canGet, e->getOnlineLibaoLeftNum());
        }
        return ret;
    }
    return HC_ERROR;
}

//签到
int ProcessDebugSign(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_sign_data* e = Singleton<new_event_mgr>::Instance().getCharSignData(cdata->m_id);
    if (e)
    {
        int day = 1;
        READ_INT_FROM_MOBJ(day,o,"day");
        if (day == e->m_tm_now.tm_mday)
        {
            return e->doSign();
        }
        if (day < 0 || day > 31)
        {
            return HC_ERROR;
        }
        for (std::vector<int>::iterator it = e->m_sign_data.begin(); it != e->m_sign_data.end(); ++it)
        {
            if (*it == day)
            {
                return HC_SUCCESS;
            }
        }
        e->m_sign_data.push_back(day);
        ++e->m_total_sign;

        //新的可以领取
        if (e->m_handle.in_list(e->m_total_sign))
        {
            e->m_getted[e->m_total_sign] = 1;
            if (e->m_canGet == 0)
            {
                //通知点亮
            }
            e->m_canGet = 1;
        }
        else
        {
            e->m_canGet = e->getSignAction();
        }
        cdata->notifyEventState(top_level_event_sign, e->m_canGet, e->getOnlineLibaoLeftNum());
        e->save();
    }
}

//发送签到按钮状态
int ProcessSendSignState(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid,o,"cid");
    char_sign_data* e = Singleton<new_event_mgr>::Instance().getCharSignData(cid);
    if (e)
    {
        e->m_canGet = e->getSignAction();
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc)
        {
            pc->notifyEventState(top_level_event_sign, e->m_canGet, e->getOnlineLibaoLeftNum());
        }
    }
    return HC_SUCCESS;
}

//查询每日充值信息
int ProcessQueryDailyRecharge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return Singleton<new_event_mgr>::Instance().getDailyRechargeInfo(*(cdata.get()), robj);
}

#ifdef VN_EN_SERVER
//查询活动信息
int ProcessQueryEventInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type) );
    return Singleton<new_event_mgr>::Instance().getEvent(cdata.get(), type, robj);
}

//领取活动奖励
int ProcessGetEventReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 0, need = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    READ_INT_FROM_MOBJ(need,o,"need");
    robj.push_back( Pair("type", type) );
    return Singleton<new_event_mgr>::Instance().getReward(cdata.get(), type, need, robj);
}

//查询登录活动信息
int ProcessQueryLoginEventInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_login_data* e = Singleton<new_event_mgr>::Instance().getCharLoginEventData(cdata->m_id);
    if (e)
    {
        Singleton<new_event_mgr>::Instance().getLoginEvent(cdata.get(), *e, robj);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//领取活动奖励
int ProcessGetLoginEventReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int need = 0;
    READ_INT_FROM_MOBJ(need,o,"need");
    char_login_data* e = Singleton<new_event_mgr>::Instance().getCharLoginEventData(cdata->m_id);
    if (e)
    {
        return e->getAwards(*cdata.get(), need, robj);
    }
    return HC_ERROR;
}
#endif

new_event_mgr::new_event_mgr()
{
    Query q(GetDb());

    //孙策，曹仁，徐庶，黄月英，徐晃，孙尚香
    m_general_event.m_general_list.push_back(4);
    m_general_event.m_general_list.push_back(7);
    m_general_event.m_general_list.push_back(8);
    m_general_event.m_general_list.push_back(9);
    m_general_event.m_general_list.push_back(12);
    m_general_event.m_general_list.push_back(1000);

    //武将招募活动
    q.get_result("select gcount,itemType,itemId,count from base_general_event_rewards where 1 order by gcount,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int gcount = q.getval();
        Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();

        baseLibao* pb = m_general_event.m_rewards[gcount].get();
        if (pb == NULL)
        {
            m_general_event.m_rewards[gcount].reset(new baseLibao);
            pb = m_general_event.m_rewards[gcount].get();
            pb->m_name = strGeneralEventString;
            str_replace(pb->m_name, "$C", LEX_CAST_STR(gcount));
        }
        pb->m_list.push_back(item);
    }
    q.free_result();

    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_general_event.m_rewards.begin(); it != m_general_event.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->updateObj();
        }
    }

    for (std::vector<int>::iterator it = m_general_event.m_general_list.begin(); it != m_general_event.m_general_list.end(); ++it)
    {
        boost::shared_ptr<GeneralTypeData> bg = GeneralDataMgr::getInstance()->GetBaseGeneral(*it);
        if (bg.get())
        {
            std::string name = bg->m_name;
            addColor(name, bg->m_quality);
            if (m_general_event.m_general_string != "")
            {
                m_general_event.m_general_string += " ";
            }
            m_general_event.m_general_string += name;
        }
    }

    m_baoshi_event.m_min_level = 99;
    //宝石活动
    q.get_result("select level,itemType,itemId,count from base_baoshi_event_rewards where 1 order by level,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();

        baseLibao* pb = m_baoshi_event.m_rewards[level].get();
        if (pb == NULL)
        {
            m_baoshi_event.m_rewards[level].reset(new baseLibao);
            pb = m_baoshi_event.m_rewards[level].get();
            pb->m_name = strBaoshiEventString;
            str_replace(pb->m_name, "$L", LEX_CAST_STR(level));
        }
        pb->m_list.push_back(item);

        if (level < m_baoshi_event.m_min_level)
        {
            m_baoshi_event.m_min_level = level;
        }
    }
    q.free_result();

    //宝石活动
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_baoshi_event.m_rewards.begin(); it != m_baoshi_event.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->updateObj();
        }
    }

    //签到活动
    q.get_result("select seq,secs,itemType,itemId,counts from base_sign_online_libao where 1 order by seq,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int seq = q.getval();
        int secs = q.getval();
        Item itm;
        itm.type = q.getval();
        itm.id = q.getval();
        itm.nums = q.getval();
        if (itm.type == item_type_baoshi)
        {
            itm.fac = itm.nums;
            itm.nums = 1;
        }
        baseLibao* pb = NULL;
        sign_online_libao ol_libao;
        if (seq == m_sign_event.m_online_libaos.size())
        {
            ol_libao = m_sign_event.m_online_libaos[seq-1];
            pb = ol_libao.m_rewards.get();
            pb->m_list.push_back(itm);
        }
        else if (seq == (m_sign_event.m_online_libaos.size() + 1))
        {
            ol_libao.id = seq;
            ol_libao.secs = secs;
            pb = new baseLibao;
            ol_libao.m_rewards.reset(pb);
            m_sign_event.m_online_libaos.push_back(ol_libao);
            pb->m_list.push_back(itm);
        }
        else
        {
            assert(false);
        }
    }
    q.free_result();

    q.get_result("select days,itemType,itemId,counts from base_sign_libao where 1 order by days,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int days = q.getval();
        Item itm;
        itm.type = q.getval();
        itm.id = q.getval();
        itm.nums = q.getval();

        if (m_sign_event.m_rewards[days].get() == NULL)
        {
            baseLibao* pb = new baseLibao;
            pb->m_level = days;    //等级用来做天数
            m_sign_event.m_rewards[days].reset(pb);
            pb->m_list.push_back(itm);
        }
        else
        {
            m_sign_event.m_rewards[days]->m_list.push_back(itm);
        }
    }
    q.free_result();

    m_daily_recharge_event.m_reward.reset(new baseLibao);
    q.get_result("select itemType,itemId,counts from base_daily_recharge_awards where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        Item itm;
        itm.type = q.getval();
        itm.id = q.getval();
        itm.nums = q.getval();
        m_daily_recharge_event.m_reward->m_list.push_back(itm);
    }
    q.free_result();
    m_daily_recharge_event.m_reward->updateObj();

    m_daily_recharge_event.m_need = GeneralDataMgr::getInstance()->getInt("daily_recharge", iDefaultDailyRecharge);

    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_sign_event.m_rewards.begin(); it != m_sign_event.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->updateObj();
        }
    }
    for (std::vector<sign_online_libao>::iterator it = m_sign_event.m_online_libaos.begin(); it != m_sign_event.m_online_libaos.end(); ++it)
    {
        if (it->m_rewards.get())
        {
            it->m_rewards->updateObj();
        }
    }

#ifdef VN_EN_SERVER
    //各类活动
    {
        std::vector<int> event_list;
        q.get_result("select id from base_event where isOpen = 1 order by id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            event_list.push_back(q.getval());
        }
        q.free_result();
        for (int i = 0; i < event_list.size(); ++i)
        {
            if (m_events[event_list[i]].get() == NULL)
            {
                template_event* pe = new template_event;
                pe->m_type = event_list[i];
                m_events[event_list[i]].reset(pe);
            }
            q.get_result("select value,itemType,itemId,counts,fac from base_event_rewards where type="+LEX_CAST_STR(event_list[i])+" order by value,id");
            CHECK_DB_ERR(q);
            while (q.fetch_row())
            {
                int value = q.getval();
                Item itm;
                itm.type = q.getval();
                itm.id = q.getval();
                itm.nums = q.getval();
                itm.fac = q.getval();

                if (m_events[event_list[i]]->m_rewards[value].get() == NULL)
                {
                    baseLibao* pb = new baseLibao;
                    m_events[event_list[i]]->m_rewards[value].reset(pb);
                    pb->m_list.push_back(itm);
                }
                else
                {
                    m_events[event_list[i]]->m_rewards[value]->m_list.push_back(itm);
                }
            }
            q.free_result();
            for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_events[event_list[i]]->m_rewards.begin(); it != m_events[event_list[i]]->m_rewards.end(); ++it)
            {
                if (it->second.get())
                {
                    it->second->updateObj();
                }
            }
            //时间设置
            m_events[event_list[i]]->m_start_time = 0;
            m_events[event_list[i]]->m_end_time = 0;
            std::string event_db = "";
            bool last_forever = false;
            switch(event_list[i])
            {
                case event_race_win:
                    event_db = "event_race_win";
                    break;
                case event_attack_power:
                    event_db = "event_attack_power";
                    break;
                case event_train_horse:
                    event_db = "event_train_horse";
                    break;
                case event_turn_horse:
                    event_db = "event_turn_horse";
                    last_forever = true;
                    break;
                case event_general_star:
                    event_db = "event_general_star";
                    last_forever = true;
                    break;
            }
            if (!last_forever)
            {
                std::string data = GeneralDataMgr::getInstance()->getStr(event_db);
                json_spirit::mValue value;
                json_spirit::read(data, value);
                if (value.type() == json_spirit::obj_type)
                {
                    json_spirit::mObject& o = value.get_obj();
                    READ_INT_FROM_MOBJ(m_events[event_list[i]]->m_start_time,o,"start_time");
                    READ_INT_FROM_MOBJ(m_events[event_list[i]]->m_end_time,o,"end_time");
                }
                if (m_events[event_list[i]]->m_start_time == 0 && m_events[event_list[i]]->m_end_time == 0)
                {
                    time_t t_now = time(NULL);
                    struct tm tm;
                    struct tm *t = &tm;
                    localtime_r(&t_now, t);
                    t->tm_hour = 0;
                    t->tm_min = 0;
                    t->tm_sec = 0;
                    m_events[event_list[i]]->m_start_time = mktime(t);
                    m_events[event_list[i]]->m_end_time = m_events[event_list[i]]->m_start_time + 3 * iONE_DAY_SECS;
                    json_spirit::Object o;
                    o.push_back( Pair("start_time", m_events[event_list[i]]->m_start_time) );
                    o.push_back( Pair("end_time", m_events[event_list[i]]->m_end_time) );
                    GeneralDataMgr::getInstance()->setStr(event_db, json_spirit::write(o));
                }
            }
            else
            {
                m_events[event_list[i]]->m_start_time = -1;
                m_events[event_list[i]]->m_end_time = -1;
            }
        }
    }
    //登录活动
    {
        q.get_result("select day,itemType,itemId,counts,fac from base_event_login where 1 order by day,id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int day = q.getval();
            Item itm;
            itm.type = q.getval();
            itm.id = q.getval();
            itm.nums = q.getval();
            itm.fac = q.getval();

            if (m_login_event.m_rewards[day].get() == NULL)
            {
                baseLibao* pb = new baseLibao;
                m_login_event.m_rewards[day].reset(pb);
                pb->m_list.push_back(itm);
            }
            else
            {
                m_login_event.m_rewards[day]->m_list.push_back(itm);
            }
        }
        q.free_result();
        for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_login_event.m_rewards.begin(); it != m_login_event.m_rewards.end(); ++it)
        {
            if (it->second.get())
            {
                it->second->updateObj();
            }
        }
        //时间设置
        m_login_event.m_start_time = 0;
        m_login_event.m_end_time = 0;
        std::string data = GeneralDataMgr::getInstance()->getStr("event_login");
        json_spirit::mValue value;
        json_spirit::read(data, value);
        if (value.type() == json_spirit::obj_type)
        {
            json_spirit::mObject& o = value.get_obj();
            READ_INT_FROM_MOBJ(m_login_event.m_start_time,o,"start_time");
            READ_INT_FROM_MOBJ(m_login_event.m_end_time,o,"end_time");
            READ_INT_FROM_MOBJ(m_login_event.m_cur_state,o,"cur_state");
            READ_INT_FROM_MOBJ(m_login_event.m_cur_day,o,"cur_day");
        }
        if (m_login_event.m_start_time == 0 && m_login_event.m_end_time == 0)
        {
            time_t t_now = time(NULL);
            struct tm tm;
            struct tm *t = &tm;
            localtime_r(&t_now, t);
            t->tm_hour = 0;
            t->tm_min = 0;
            t->tm_sec = 0;
            m_login_event.m_start_time = mktime(t);
            m_login_event.m_end_time = m_login_event.m_start_time + 12 * iONE_DAY_SECS;
            m_login_event.m_cur_state = 1;
            m_login_event.m_cur_day = 1;
            json_spirit::Object o;
            o.push_back( Pair("start_time", m_login_event.m_start_time) );
            o.push_back( Pair("end_time", m_login_event.m_end_time) );
            o.push_back( Pair("cur_state", m_login_event.m_cur_state) );
            o.push_back( Pair("cur_day", m_login_event.m_cur_day) );
            GeneralDataMgr::getInstance()->setStr("event_login", json_spirit::write(o));
        }
    }
#endif
}

//查询活动状态
int new_event_mgr::getActionState(int cid)
{
    //武将活动
    char_general_event* ge = getCharGeneralEvent(cid);
    if (ge && ge->m_end_time > time(NULL) && ge->m_canGet)
    {
        return 1;
    }
    //宝石活动
    char_baoshi_event* be = getCharBaoshiEvent(cid);
    if (be && be->m_end_time > time(NULL) && be->m_canGet)
    {
        return 1;
    }
#ifdef VN_EN_SERVER
    if (isLoginEventOpen())
    {
        char_login_data* be = getCharLoginEventData(cid);
        if (be && be->m_canGet)
        {
            return 1;
        }
    }
    for (int i = 1; i <= event_max; ++i)
    {
        if (isEventOpen(i))
        {
            char_event_data* e = getCharEventData(cid, i);
            if (e && e->m_canGet)
            {
                return 1;
            }
        }
    }
#endif
    return 0;
}

void new_event_mgr::getActionList(CharData* pc, json_spirit::Array& list)
{
    json_spirit::Object info;
    //招募武将 id=4
    char_general_event* ge = getCharGeneralEvent(pc->m_id);
    if (ge && ge->m_end_time > time(NULL))
    {
        info.clear();
        info.push_back( Pair("id", 4) );
        info.push_back( Pair("state", ge->m_canGet) );
        list.push_back(info);
    }
    //宝石活动 id=5
    char_baoshi_event* be = getCharBaoshiEvent(pc->m_id);
    if (be && be->m_end_time > time(NULL))
    {
        info.clear();
        info.push_back( Pair("id", 5) );
        info.push_back( Pair("state", be->m_canGet) );
        list.push_back(info);
    }
#ifdef VN_EN_SERVER
    //登录活动 id=6
    if (isLoginEventOpen())
    {
        char_login_data* be = getCharLoginEventData(pc->m_id);
        if (be)
        {
            info.clear();
            info.push_back( Pair("id", 6) );
            info.push_back( Pair("state", be->m_canGet) );
            list.push_back(info);
        }
    }
    for (int i = 1; i <= event_max; ++i)
    {
        if (isEventOpen(i))
        {
            char_event_data* e = getCharEventData(pc->m_id, i);
            if (e)
            {
                info.clear();
                info.push_back( Pair("id", i+100) );
                info.push_back( Pair("state", e->m_canGet) );
                list.push_back(info);
            }
        }
    }
#endif
    return;
}

void new_event_mgr::getBaoshiEventList(char_baoshi_event& ce, json_spirit::Array& list)
{
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_baoshi_event.m_rewards.begin(); it != m_baoshi_event.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->first) );
            obj.push_back( Pair("get", ce.m_geted[it->first]) );
            obj.push_back( Pair("requirement", it->second->m_name) );
            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );

            list.push_back(obj);
        }
    }
}

void new_event_mgr::getGeneralEventList(char_general_event& ce, json_spirit::Array& list)
{
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_general_event.m_rewards.begin(); it != m_general_event.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->first) );
            obj.push_back( Pair("get", ce.m_geted[it->first]) );
            obj.push_back( Pair("requirement", it->second->m_name) );
            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );

            list.push_back(obj);
        }
    }
}

void new_event_mgr::getSignInfo(char_sign_data& e, json_spirit::Object& robj)
{
    robj.push_back( Pair("total", e.m_total_sign) );
    robj.push_back( Pair("curTime", e.m_time_now) );
    robj.push_back( Pair("canSign", e.m_sign_time == 0 ? 1 : 0) );
    robj.push_back( Pair("total", e.m_total_sign) );
    json_spirit::Array slist(e.m_sign_data.begin(), e.m_sign_data.end());
    robj.push_back( Pair("signList", slist) );
    robj.push_back( Pair("curO", e.m_cur_online_libao) );
    robj.push_back( Pair("curS", e.m_cur_libao_state) );
    robj.push_back( Pair("curT", e.m_cur_libao_get_time) );

    json_spirit::Array olist;
    for (std::vector<sign_online_libao>::iterator it = m_sign_event.m_online_libaos.begin(); it != m_sign_event.m_online_libaos.end(); ++it)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("id", it->id) );
        if (it->id < e.m_cur_online_libao || (e.m_sign_time > 0 && e.m_cur_online_libao == 0))
        {
            obj.push_back( Pair("get", 2) );
        }
        else if (it->id == e.m_cur_online_libao)
        {
            obj.push_back( Pair("get", e.m_cur_libao_state) );
            if (e.m_cur_libao_state == 0 && e.m_cur_libao_get_time > 0)
            {
                obj.push_back( Pair("left", e.m_cur_libao_get_time - e.m_time_now) );
            }
        }
        else if (it->id > e.m_cur_online_libao)
        {
            obj.push_back( Pair("get", 0) );
        }
        if (it->m_rewards.get())
        {
            const json_spirit::Array& rlist = it->m_rewards->getArray();
            obj.push_back( Pair("list", rlist) );
        }

        olist.push_back(obj);
    }

    json_spirit::Array tlist;
    int cur_t = 1, cur_state = 2;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_sign_event.m_rewards.begin(); it != m_sign_event.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            //前面有未领取的
            if (cur_state == 1)
            {
                ;
            }
            //前面全部领取了
            else if (cur_state == 2)
            {
                cur_t = it->first;
                cur_state = e.m_getted[it->first];
            }
            //前面全部不能领取
            else if (cur_state == 0)
            {
                if (e.m_getted[it->first] == 1)
                {
                    cur_t = it->first;
                    cur_state = 1;
                }
            }
            json_spirit::Object obj;
            obj.push_back( Pair("times", it->first) );
            obj.push_back( Pair("get", e.m_getted[it->first]) );

            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );

            tlist.push_back(obj);
        }
    }

    robj.push_back( Pair("olist", olist) );
    robj.push_back( Pair("show_t", cur_t) );
    robj.push_back( Pair("tlist", tlist) );

    e.m_canGet = e.getSignAction();
}

const std::string& new_event_mgr::getGeneralhiEvent()
{
    return m_general_event.m_general_string;
}

char_general_event* new_event_mgr::getCharGeneralEvent(int cid)
{
    if (m_char_general_events.find(cid) != m_char_general_events.end())
    {
        return m_char_general_events[cid].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select start_time,end_time,getted,glist from char_general_events where end_time>unix_timestamp() and cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            char_general_event* ce = new char_general_event(m_general_event, cid);
            ce->m_start_time = q.getval();
            ce->m_end_time = q.getval();
            std::string data = q.getstr();
            std::vector<int> get_list;
            read_int_vector(data, get_list);
            for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
            {
                ce->m_geted[*it] = 2;
            }
            std::string gs = q.getstr();
            read_int_vector(gs, ce->m_glist);

            for (std::map<int, boost::shared_ptr<baseLibao> >::iterator itr = ce->m_handle.m_rewards.begin(); itr != ce->m_handle.m_rewards.end(); ++itr)
            {
                if (itr->second.get() && ce->m_glist.size() >= itr->first)
                {
                    if (ce->m_geted[itr->first] == 0)
                    {
                        ce->m_geted[itr->first] = 1;
                        ce->m_canGet = 1;
                    }
                }
            }
            m_char_general_events[cid].reset(ce);
            q.free_result();
        }
        else
        {
            q.free_result();
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            struct tm tm;
            struct tm *t = &tm;
            localtime_r(&pc->m_createTime, t);
            t->tm_hour = 0;
            t->tm_min = 0;
            t->tm_sec = 0;
            time_t end_time = mktime(t) + 7 * iONE_DAY_SECS;
            if (end_time > time(NULL))
            {
                return addCharGeneralEvent(*pc);
            }
        }
        return m_char_general_events[cid].get();
    }
}

char_baoshi_event* new_event_mgr::getCharBaoshiEvent(int cid)
{
    if (m_char_baoshi_events.find(cid) != m_char_baoshi_events.end())
    {
        return m_char_baoshi_events[cid].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select end_time,start_time,getted,glist from char_baoshi_events where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            time_t end_time = q.getval();
            if (end_time > time(NULL))
            {
                char_baoshi_event* ce = new char_baoshi_event(m_baoshi_event, cid);
                ce->m_start_time = q.getval();
                ce->m_end_time = end_time;
                std::string data = q.getstr();
                std::vector<int> get_list;
                read_int_vector(data, get_list);
                for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
                {
                    ce->m_geted[*it] = 2;
                }
                std::string gs = q.getstr();
                read_int_vector(gs, ce->m_glist);

                for (std::vector<int>::iterator it = ce->m_glist.begin(); it != ce->m_glist.end(); ++it)
                {
                    if (ce->m_geted[*it] == 0)
                    {
                        ce->m_geted[*it] = 1;
                    }
                }
                m_char_baoshi_events[cid].reset(ce);
            }
            q.free_result();
        }
        else
        {
            q.free_result();
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            struct tm tm;
            struct tm *t = &tm;
            localtime_r(&pc->m_createTime, t);
            t->tm_hour = 0;
            t->tm_min = 0;
            t->tm_sec = 0;
            time_t end_time = mktime(t) + 30 * iONE_DAY_SECS;
            if (end_time > time(NULL))
            {
                return addCharBaoshiEvent(*pc);
            }
        }
        return m_char_baoshi_events[cid].get();
    }
}

void new_event_mgr::addBaoshi(int cid, int level)
{
    //cout<<"event add baoshi level "<<level<<",min level "<<m_baoshi_event.m_min_level<<endl;
    if (level < m_baoshi_event.m_min_level)
    {
        return;
    }
    if (false == m_baoshi_event.in_list(level))
    {
        return;
    }
    char_baoshi_event* e = getCharBaoshiEvent(cid);
    if (e)
    {
        e->addBaoshi(level);
    }
}

void new_event_mgr::addGeneral(int cid, int gid)
{
    if (false == m_general_event.in_list(gid))
    {
        return;
    }
    char_general_event* e = getCharGeneralEvent(cid);
    if (e)
    {
        e->addGeneral(gid);
    }
}

void new_event_mgr::addGeneralCheck(int cid, int gid)
{
    if (false == m_general_event.in_list(gid))
    {
        return;
    }
    char_general_event* e = getCharGeneralEvent(cid);
    if (e)
    {
        e->addGeneralCheck(gid);
    }
}

char_general_event* new_event_mgr::addCharGeneralEvent(CharData& cdata)
{
    char_general_event* ce = NULL;
    if (m_char_general_events[cdata.m_id].get() != NULL)
    {
        ce = m_char_general_events[cdata.m_id].get();
        ce->m_geted.clear();
        ce->m_glist.clear();
    }
    else
    {
        ce = new char_general_event(m_general_event, cdata.m_id);
        m_char_general_events[cdata.m_id].reset(ce);
    }
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&cdata.m_createTime, t);
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    ce->m_start_time = mktime(t);
    ce->m_end_time = ce->m_start_time + 7 * iONE_DAY_SECS;

    InsertSaveDb("replace into char_general_events (cid,start_time,end_time,getted,glist) values (" + LEX_CAST_STR(cdata.m_id)
        + "," + LEX_CAST_STR(ce->m_start_time) + "," + LEX_CAST_STR(ce->m_end_time) + ",'','')");
    return ce;
}

char_baoshi_event* new_event_mgr::addCharBaoshiEvent(CharData& cdata)
{
    char_baoshi_event* ce = NULL;
    if (m_char_baoshi_events[cdata.m_id].get() != NULL)
    {
        ce = m_char_baoshi_events[cdata.m_id].get();
        ce->m_geted.clear();
        ce->m_glist.clear();
    }
    else
    {
        ce = new char_baoshi_event(m_baoshi_event, cdata.m_id);
        m_char_baoshi_events[cdata.m_id].reset(ce);
    }
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&cdata.m_createTime, t);
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    ce->m_start_time = mktime(t);
    ce->m_end_time = ce->m_start_time + 30 * iONE_DAY_SECS;

    InsertSaveDb("replace into char_baoshi_events (cid,start_time,end_time,getted,glist) values (" + LEX_CAST_STR(cdata.m_id)
        + "," + LEX_CAST_STR(ce->m_start_time) + "," + LEX_CAST_STR(ce->m_end_time) + ",'','')");
    return ce;
}

char_sign_data* new_event_mgr::getCharSignData(int cid)
{
    if (m_char_sign_events.find(cid) != m_char_sign_events.end())
    {
        m_char_sign_events[cid]->checkReset();
        return m_char_sign_events[cid].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select ctime,sign_data,cur_gift,gift_state,gift_time,sign_time,reward_getted from char_signs where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            char_sign_data* ce = new char_sign_data(m_sign_event, cid);
            ce->m_time_now = time(NULL);
            localtime_r(&ce->m_time_now, &ce->m_tm_now);

            ce->m_generate_time = q.getval();
            localtime_r(&ce->m_generate_time, &ce->m_tm_generate);

            std::string data = q.getstr();
            read_int_vector(data, ce->m_sign_data);

            ce->m_cur_online_libao = q.getval();
            ce->m_cur_libao_state = q.getval();
            ce->m_cur_libao_get_time = q.getval();
            ce->m_sign_time = q.getval();
            std::string getted_data = q.getstr();
            q.free_result();

            ce->m_total_sign = ce->m_sign_data.size();

            if (ce->m_sign_data.size() > 0 && ce->m_tm_now.tm_mday == *ce->m_sign_data.rbegin())
            {
                if (ce->m_sign_time == 0)
                {
                    ce->m_sign_time = ce->m_time_now;
                    ce->m_cur_online_libao = 1;
                    ce->m_cur_libao_state = 0;
                    ce->m_cur_libao_get_time = ce->m_time_now + ce->m_handle.get_online_libao_secs(1);
                }
            }
            else
            {
                ce->m_sign_time = 0;
                ce->m_cur_online_libao = 0;
                ce->m_cur_libao_get_time = 0;
                ce->m_cur_libao_state = 0;
            }

            if (ce->m_sign_time == 0 || ce->m_cur_libao_state == 1)
            {
                ce->m_canGet = 1;
            }

            std::vector<int> get_list;
            read_int_vector(getted_data, get_list);
            for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
            {
                ce->m_getted[*it] = 2;
            }

            for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_sign_event.m_rewards.begin(); it != m_sign_event.m_rewards.end(); ++it)
            {
                if (ce->m_getted[it->first] == 0 && it->first <= ce->m_total_sign)
                {
                    ce->m_getted[it->first] = 1;
                    ce->m_canGet = 1;
                }
            }
            m_char_sign_events[cid].reset(ce);
            ce->checkReset();
        }
        else
        {
            q.free_result();

            //是否要插入数据?
            char_sign_data* ce = new char_sign_data(m_sign_event, cid);
            ce->m_time_now = time(NULL);
            localtime_r(&ce->m_time_now, &ce->m_tm_now);

            //全部重置
            ce->m_getted.clear();
            ce->m_sign_data.clear();
            ce->m_tm_generate = ce->m_tm_now;
            ce->m_generate_time = ce->m_time_now;

            ce->m_sign_time = 0;
            ce->m_total_sign = 0;
            ce->m_cur_online_libao = 0;
            ce->m_cur_libao_state = 0;
            ce->m_cur_libao_get_time = 0;

            InsertSaveDb("replace into char_signs (cid,ctime,sign_data,cur_gift,gift_state,gift_time,sign_time,reward_getted) values ("
                + LEX_CAST_STR(ce->m_cid) + ","
                + LEX_CAST_STR(ce->m_generate_time) + ",'',0,0,0,0,'')");

            m_char_sign_events[cid].reset(ce);
        }
        return m_char_sign_events[cid].get();
    }
}

void new_event_mgr::getSignAction(int cid, json_spirit::Array& blist)
{
    char_sign_data* e = getCharSignData(cid);
    if (e)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_sign) );
        obj.push_back( Pair("active", e->getSignAction()) );
        obj.push_back( Pair("leftNums", e->getOnlineLibaoLeftNum()) );
        blist.push_back(obj);
        return;
    }
    return;
}

char_daily_recharge* new_event_mgr::getCharDailyRecharge(int cid)
{
    std::map<int, boost::shared_ptr<char_daily_recharge> >::iterator it = m_daily_recharge_event.m_chars.find(cid);
    if (it != m_daily_recharge_event.m_chars.end())
    {
        return it->second.get();
    }
    Query q(GetDb());
    q.get_result("select get,viewed,recharge from char_daily_recharge where cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        char_daily_recharge* c = new char_daily_recharge;
        c->m_cid = cid;
        c->m_get = q.getval();
        c->m_recharge = q.getval();
        c->m_viewed = q.getval();
        q.free_result();
        m_daily_recharge_event.m_chars[cid].reset(c);
        return c;
    }
    else
    {
        q.free_result();
        //cout<<"------------> new_event_mgr::new char_daily_recharge "<<cid<<" <---------------"<<endl;
        InsertSaveDb("insert into char_daily_recharge (cid,get,viewed,recharge) values (" + LEX_CAST_STR(cid) + ",0,0,0)");
        char_daily_recharge* c = new char_daily_recharge;
        c->m_cid = cid;
        c->m_get = 0;
        c->m_recharge = 0;
        c->m_viewed = 0;
        m_daily_recharge_event.m_chars[cid].reset(c);
        return c;
    }

}

void new_event_mgr::getDailyRechargeAction(int cid, json_spirit::Array& list)
{
    char_daily_recharge* c = getCharDailyRecharge(cid);
    if (c)
    {
        if (c->m_get == 2)
        {
            return;
        }
        int state = 0;
        if (c->m_get == 1 || c->m_viewed == 0)
        {
            state = 1;
        }
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_daily_recharge) );
        obj.push_back( Pair("active", state) );
        list.push_back(obj);
    }
}

int new_event_mgr::getDailyRechargeAward(CharData& cdata, json_spirit::Object& robj)
{
    char_daily_recharge* c = getCharDailyRecharge(cdata.m_id);
    if (c)
    {
        if (c->m_get == 1)
        {
            std::list<Item> items = m_daily_recharge_event.m_reward->m_list;
            giveLoots(&cdata, items, cdata.m_area, cdata.m_level, 0, NULL, &robj, false, give_daily_recharge);
            cdata.notifyEventRemove(top_level_event_daily_recharge);
            c->m_get = 2;
            c->save();
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

void new_event_mgr::dailyReset()
{
    cout<<"------------> new_event_mgr::dailyReset() < ---------------"<<endl;
    for (std::map<int, boost::shared_ptr<char_daily_recharge> >::iterator it = m_daily_recharge_event.m_chars.begin(); it != m_daily_recharge_event.m_chars.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->dailyReset();
        }
    }
    InsertSaveDb("update char_daily_recharge set get=if(get=2,0,get),viewed=0,recharge=0 where 1");
#ifdef VN_EN_SERVER
    updateLoginEvent();
#endif
}

void new_event_mgr::addDailyRecharge(CharData& cdata, int recharge)
{
    char_daily_recharge* c = getCharDailyRecharge(cdata.m_id);
    if (c)
    {
        c->m_recharge += recharge;
        if (c->m_get == 0 && c->m_recharge >= m_daily_recharge_event.m_need)
        {
            c->m_get = 1;
            //通知 保存
            cdata.notifyEventState(top_level_event_daily_recharge, 1, 0);
            c->save();
        }
        else
        {
            c->save();
        }
    }
}

int new_event_mgr::getDailyRechargeInfo(CharData& cdata, json_spirit::Object& robj)
{
    char_daily_recharge* c = getCharDailyRecharge(cdata.m_id);
    if (c)
    {
        const json_spirit::Array& list = m_daily_recharge_event.m_reward->getArray();
        robj.push_back( Pair("list", list) );
        robj.push_back( Pair("get", c->m_get) );
        robj.push_back( Pair("recharged", c->m_recharge) );
        robj.push_back( Pair("need", m_daily_recharge_event.m_need) );
        if (c->m_viewed == 0)
        {
            c->m_viewed = 1;
            c->save();

            if (c->m_get != 1)
            {
                cdata.notifyEventState(top_level_event_daily_recharge, 0, 0);
            }
        }
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

int new_event_mgr::getDailyRechargeNeed(CharData& cdata)
{
    char_daily_recharge* c = getCharDailyRecharge(cdata.m_id);
    if (c)
    {
        if (c->m_get == 0)
        {
            return m_daily_recharge_event.m_need - c->m_recharge;
        }
    }
    return 0;
}

#ifdef VN_EN_SERVER
template_event* new_event_mgr::getEventTemp(int type)
{
    if (m_events.find(type) != m_events.end())
    {
        return m_events[type].get();
    }
    return NULL;
}

bool new_event_mgr::isEventOpen(int type)
{
    template_event* pEvent = getEventTemp(type);
    if (pEvent)
    {
        return pEvent->isOpen();
    }
    return false;
}

int new_event_mgr::getEvent(CharData* pc, int type, json_spirit::Object& robj)
{
    char_event_data* e = getCharEventData(pc->m_id, type);
    if (e == NULL)
    {
        return HC_ERROR;
    }
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = e->m_handle.m_rewards.begin(); it != e->m_handle.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("need", it->first) );
            if (type == event_turn_horse)
            {
                baseHorse* b = horseMgr::getInstance()->getHorse(it->first);
                if (b)
                {
                    obj.push_back( Pair("name", b->name) );
                }
            }
            obj.push_back( Pair("get", e->m_geted[it->first]) );
            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
    if (e->m_handle.m_start_time > 0 && e->m_handle.m_end_time > 0)
    {
        robj.push_back( Pair("start_time", e->m_handle.m_start_time) );
        robj.push_back( Pair("end_time", e->m_handle.m_end_time));
    }
    return HC_SUCCESS;
}

int new_event_mgr::getReward(CharData* pc, int type, int need, json_spirit::Object& robj)
{
    char_event_data* e = getCharEventData(pc->m_id, type);
    if (e == NULL)
    {
        return HC_ERROR;
    }
    return e->getAwards(*pc, need, robj);
}

char_event_data* new_event_mgr::getCharEventData(int cid, int type)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    template_event* pEvent = getEventTemp(type);
    if (pc == NULL || pEvent == NULL || !pEvent->isOpen())
    {
        return NULL;
    }
    if (m_all_char_event_data.find(std::make_pair(cid, type)) != m_all_char_event_data.end())
    {
        return m_all_char_event_data[std::make_pair(cid, type)].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select reward_getted,finish_list from char_event_data where cid=" + LEX_CAST_STR(cid) + " and type=" + LEX_CAST_STR(type));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            char_event_data* ce = new char_event_data(*pEvent, cid);
            std::string data = q.getstr();
            std::vector<int> get_list;
            read_int_vector(data, get_list);
            for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
            {
                ce->m_geted[*it] = 2;
            }
            std::string f = q.getstr();
            read_int_vector(f, ce->m_finish_list);
            for (std::vector<int>::iterator it = ce->m_finish_list.begin(); it != ce->m_finish_list.end(); ++it)
            {
                if (ce->m_geted[*it] == 0)
                {
                    ce->m_geted[*it] = 1;
                }
            }
            m_all_char_event_data[std::make_pair(cid, type)].reset(ce);
            q.free_result();
        }
        else
        {
            q.free_result();
            char_event_data* ce = new char_event_data(*pEvent, cid);
            m_all_char_event_data[std::make_pair(cid, type)].reset(ce);
            InsertSaveDb("replace into char_event_data (cid,type,reward_getted,finish_list) values (" + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(type) + ",'','')");
        }
        return m_all_char_event_data[std::make_pair(cid, type)].get();
    }
}

void new_event_mgr::updateEvent(int cid, int type, int value)
{
    char_event_data* e = getCharEventData(cid, type);
    if (e)
    {
        e->update(value);
    }
    return;
}

bool new_event_mgr::isLoginEventOpen()
{
    return m_login_event.isOpen();
}

void new_event_mgr::getLoginEvent(CharData * pc, char_login_data & e, json_spirit::Object & robj)
{
    json_spirit::Array list;
    int start_index = (m_login_event.m_cur_state-1) * 3 + 1, end_index = start_index+2;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_login_event.m_rewards.begin(); it != m_login_event.m_rewards.end(); ++it)
    {
        if (it->second.get() && it->first >= start_index && it->first <= end_index)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("need", it->first) );
            obj.push_back( Pair("get", e.m_geted[it->first]) );
            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("start_time", m_login_event.m_start_time) );
    robj.push_back( Pair("end_time", m_login_event.m_end_time));
    return;
}

char_login_data* new_event_mgr::getCharLoginEventData(int cid)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL || !m_login_event.isOpen())
    {
        return NULL;
    }
    if (m_char_login_data.find(cid) != m_char_login_data.end())
    {
        return m_char_login_data[cid].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select reward_getted,finish_list from char_event_login where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            char_login_data* ce = new char_login_data(m_login_event, cid);
            std::string data = q.getstr();
            std::vector<int> get_list;
            read_int_vector(data, get_list);
            for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
            {
                ce->m_geted[*it] = 2;
            }
            std::string f = q.getstr();
            read_int_vector(f, ce->m_finish_list);
            for (std::vector<int>::iterator it = ce->m_finish_list.begin(); it != ce->m_finish_list.end(); ++it)
            {
                if (ce->m_geted[*it] == 0)
                {
                    ce->m_geted[*it] = 1;
                    ce->m_canGet = 1;
                }
            }
            m_char_login_data[cid].reset(ce);
            q.free_result();
        }
        else
        {
            q.free_result();
            char_login_data* ce = new char_login_data(m_login_event, cid);
            m_char_login_data[cid].reset(ce);
            InsertSaveDb("replace into char_event_login (cid,reward_getted,finish_list) values (" + LEX_CAST_STR(pc->m_id) + ",'','')");
        }
        return m_char_login_data[cid].get();
    }
}

void new_event_mgr::checkLoginEvent(CharData* pc)
{
    if (pc == NULL)
        return;
    char_login_data* e = getCharLoginEventData(pc->m_id);
    if (e)
    {
        e->check(*pc);
    }
    return;
}

void new_event_mgr::updateLoginEvent()
{
    if (!m_login_event.isOpen())
        return;
    int end_index = (m_login_event.m_cur_state-1) * 3 + 3;
    if (end_index >= m_login_event.m_rewards.size())
    {
        return;
    }
    ++m_login_event.m_cur_day;
    while (m_login_event.m_cur_day > end_index)
    {
        ++m_login_event.m_cur_state;
        end_index += 3;
        if (end_index >= m_login_event.m_rewards.size())
        {
            return;
        }
    }
    return;
}
#endif

bool new_general_event::in_list(int gid)
{
    for (std::vector<int>::iterator it = m_general_list.begin(); it != m_general_list.end(); ++it)
    {
        if (gid == *it)
        {
            return true;
        }
    }
    return false;
}

bool new_baoshi_event::in_list(int level)
{
    if (m_rewards.find(level) != m_rewards.end())
    {
        return m_rewards[level].get() != NULL;
    }
    else
    {
        return false;
    }
}

char_general_event::char_general_event(new_general_event& h, int cid)
:m_handle(h)
{
    m_cid = cid;
    m_start_time = 0;
    m_end_time = 0;
    m_canGet = 0;
}

void char_general_event::addGeneralCheck(int gid)
{
    if (m_handle.in_list(gid))
    {
        for (std::vector<int>::iterator it = m_glist.begin(); it != m_glist.end(); ++it)
        {
            if (gid == *it)
            {
                return;
            }
        }
        m_glist.push_back(gid);
        int count = m_glist.size();
        if (m_handle.m_rewards.find(count) != m_handle.m_rewards.end() && m_handle.m_rewards[count].get() != NULL)
        {
            if (m_geted[count] == 0)
            {
                m_geted[count] = 1;
                if (m_canGet == 0)
                {
                    //通知点亮
                    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_cid).get();
                    if (pc)
                    {
                        pc->notifyEventState(top_level_event_opening, 1, 0);
                    }
                }
                m_canGet = 1;
            }
        }
        save();
    }
}

void char_general_event::addGeneral(int gid)
{
    if (m_handle.in_list(gid))
    {
        m_glist.push_back(gid);
        if (m_handle.m_rewards[m_glist.size()].get() != NULL)
        {
            if (m_geted[m_glist.size()] == 0)
            {
                m_geted[m_glist.size()] = 1;
                if (m_canGet == 0)
                {
                    //通知点亮
                    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_cid).get();
                    if (pc)
                    {
                        pc->notifyEventState(top_level_event_opening, 1, 0);
                    }
                }
                m_canGet = 1;
            }
        }
        save();
    }
}

int char_general_event::getAwards(CharData& cdata, int count, json_spirit::Object& robj)
{
    switch (m_geted[count])
    {
        case 1:
            if (m_handle.m_rewards.find(count) != m_handle.m_rewards.end() && m_handle.m_rewards[count].get())
            {
                baseLibao* lb = m_handle.m_rewards[count].get();
                if ((cdata.m_bag.size()-cdata.m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                //给东西
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, 0, cdata.m_level, 0, NULL, &robj, true, give_libao_loot);

                m_geted[count] = 2;
                m_canGet = 0;
                for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
                {
                    if (it->second == 1)
                    {
                        m_canGet = 1;
                        break;
                    }
                }
                save();
                int state = cdata.getOpeningState();
                cdata.notifyEventState(top_level_event_opening, state, 0);
                return HC_SUCCESS;
            }
            else
            {
                ERR();
                cout<<"count:"<<count<<endl;
                return HC_ERROR;
            }
            break;
        case 2:
        case 0:
        default:
            return HC_ERROR;
    }
}

void char_general_event::save()
{
    std::string sql = "update char_general_events set getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "',glist='";
    int g_count = 0;
    for (std::vector<int>::iterator it = m_glist.begin(); it != m_glist.end(); ++it)
    {
        if (g_count)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++g_count;
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

char_baoshi_event::char_baoshi_event(new_baoshi_event& h, int cid)
:m_handle(h)
{
    m_cid = cid;
    m_start_time = 0;
    m_end_time = 0;
    m_canGet = 0;
}

void char_baoshi_event::addBaoshi(int level)
{
    //cout<<"char_baoshi_event add baoshi level "<<level<<endl;
    if (m_handle.in_list(level))
    {
        if (m_geted[level] == 0)
        {
            m_glist.push_back(level);
            m_geted[level] = 1;
            save();
            if (m_canGet == 0)
            {
                //通知点亮
                CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_cid).get();
                if (pc)
                {
                    pc->notifyEventState(top_level_event_opening, 1, 0);
                }
            }
            m_canGet = 1;
        }
    }
}

void char_baoshi_event::save()
{
    std::string sql = "update char_baoshi_events set getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "',glist='";
    int g_count = 0;
    for (std::vector<int>::iterator it = m_glist.begin(); it != m_glist.end(); ++it)
    {
        if (g_count)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++g_count;
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

int char_baoshi_event::getAwards(CharData& cdata, int count, json_spirit::Object& robj)
{
    switch (m_geted[count])
    {
        case 1:
            if (m_handle.m_rewards.find(count) != m_handle.m_rewards.end() && m_handle.m_rewards[count].get())
            {
                baseLibao* lb = m_handle.m_rewards[count].get();
                if ((cdata.m_bag.size()-cdata.m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                //给东西
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, 0, cdata.m_level, 0, NULL, &robj, true, give_libao_loot);

                m_geted[count] = 2;
                m_canGet = 0;
                for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
                {
                    if (it->second == 1)
                    {
                        m_canGet = 1;
                        break;
                    }
                }
                save();
                int state = cdata.getOpeningState();
                cdata.notifyEventState(top_level_event_opening, state, 0);
                return HC_SUCCESS;
            }
            else
            {
                ERR();
                cout<<"count:"<<count<<endl;
                return HC_ERROR;
            }
            break;
        case 2:
        case 0:
        default:
            return HC_ERROR;
    }
}

int sign_event::get_online_libao_secs(int seq)
{
    if (seq >= 1 && seq <= m_online_libaos.size())
    {
        return m_online_libaos[seq-1].secs;
    }
    return 0;
}

bool sign_event::in_list(int days)
{
    return m_rewards.find(days) != m_rewards.end();
}

baseLibao* sign_event::getOnlineLibao(int seq)
{
    if (seq >= 1 && seq <= m_online_libaos.size())
    {
         return m_online_libaos[seq-1].m_rewards.get();
    }
    return NULL;
}

int sign_event::getOnlineLibaoCnt()
{
    return m_online_libaos.size();
}

char_sign_data::char_sign_data(sign_event& h, int cid)
:m_handle(h)
,m_cid(cid)
{
    m_cur_online_libao = 0;
    m_sign_time = 0;
    m_canGet = 0;
}

void char_sign_data::save()
{
    std::string sql = "update char_signs set ctime=" + LEX_CAST_STR(m_generate_time) + ",sign_data='";
    int scount = 0;
    for (std::vector<int>::iterator it = m_sign_data.begin(); it != m_sign_data.end(); ++it)
    {
        if (scount)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++scount;
    }
    sql += "',cur_gift=" + LEX_CAST_STR(m_cur_online_libao)
        + ",gift_state=" + LEX_CAST_STR(m_cur_libao_state)
        + ",gift_time=" + LEX_CAST_STR(m_cur_libao_get_time)
        + ",sign_time=" + LEX_CAST_STR(m_sign_time)
        + ",reward_getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_getted.begin(); it != m_getted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

int char_sign_data::getAwards(CharData& cdata, int days, json_spirit::Object& robj)
{
    if (m_getted[days] == 1)
    {
        if (m_handle.m_rewards.find(days) != m_handle.m_rewards.end())
        {
            baseLibao* lb = m_handle.m_rewards[days].get();
            if (lb)
            {
                //给东西
                if ((cdata.m_bag.size()-cdata.m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, 0, cdata.m_level, 0, NULL, &robj, true, give_libao_loot);
            }
        }

        m_getted[days] = 2;
        m_canGet = getSignAction();
        save();
    }
    return HC_SUCCESS;
}

int char_sign_data::getOnlineAwards(CharData& cdata, json_spirit::Object& robj)
{
    if (m_cur_libao_state == 1)
    {
        baseLibao* lb = m_handle.getOnlineLibao(m_cur_online_libao);
        if (lb)
        {
            //给东西
            if ((cdata.m_bag.size()-cdata.m_bag.getUsed()) < lb->need_slot_num)
            {
                return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
            }
            std::list<Item> items = lb->m_list;
            giveLoots(&cdata, items, 0, cdata.m_level, 0, NULL, &robj, true, give_libao_loot);
            //act统计
            act_to_tencent(&cdata,act_new_sign_reward,m_cur_online_libao);
        }

        m_cur_libao_state = 0;
        ++m_cur_online_libao;
        lb = m_handle.getOnlineLibao(m_cur_online_libao);
        //还有下一个
        if (lb)
        {
            int secs = m_handle.get_online_libao_secs(m_cur_online_libao);
            m_cur_libao_get_time = m_time_now + secs;
            json_spirit::mObject mobj;
            mobj["cmd"] = "notifySign";
            mobj["cid"] = m_cid;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(secs, 1, mobj, 1));
            splsTimerMgr::getInstance()->addTimer(tmsg);
            //cout<<"get online sign gift, next "<<m_cur_online_libao<<endl;
        }
        else
        {
            //cout<<"get last online sign gift "<<(m_cur_online_libao-1)<<endl;
            m_cur_libao_get_time = 0;
            m_cur_online_libao = 0;
        }

        m_canGet = getSignAction();

        save();
    }
    return HC_SUCCESS;
}

int char_sign_data::doSign()
{
    //今天欠了?
    if (m_sign_time > 0)
    {
        return HC_SUCCESS;
    }
    //今天欠了?
    //if (m_sign_data.size() > 0 && *m_sign_data.rbegin() == m_tm_now.tm_mday)
    //{
    //    return HC_SUCCESS;
    //}

    m_sign_time = m_time_now;
    m_sign_data.push_back(m_tm_now.tm_mday);
    m_cur_online_libao = 1;
    m_cur_libao_state = 0;
    int secs = m_handle.get_online_libao_secs(1);
    m_cur_libao_get_time = m_time_now + secs;
    ++m_total_sign;

    //新的可以领取
    if (m_handle.in_list(m_total_sign))
    {
        m_getted[m_total_sign] = 1;
        m_canGet = 1;
    }
    else
    {
        m_canGet = getSignAction();
    }

    json_spirit::mObject mobj;
    mobj["cmd"] = "notifySign";
    mobj["cid"] = m_cid;
    boost::shared_ptr<splsTimer> tmsg;
    tmsg.reset(new splsTimer(secs, 1, mobj, 1));
    splsTimerMgr::getInstance()->addTimer(tmsg);

    save();
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (cdata.get())
    {
        //act统计
        act_to_tencent(cdata.get(),act_new_sign);
    }
    return HC_SUCCESS;
}

void char_sign_data::checkReset()
{
    m_time_now = time(NULL);
    localtime_r(&m_time_now, &m_tm_now);
    //月份变化
    if (m_tm_now.tm_mon != m_tm_generate.tm_mon
        || m_tm_now.tm_year != m_tm_generate.tm_year)
    {
        //全部重置
        m_getted.clear();
        m_sign_data.clear();
        m_tm_generate = m_tm_now;
        m_generate_time = m_time_now;

        m_sign_time = 0;
        m_total_sign = 0;
        m_cur_online_libao = 1;
        m_cur_libao_state = 0;
        m_cur_libao_get_time = 0;

        m_canGet = 1;

        save();
    }
    else if (m_tm_now.tm_mday != m_tm_generate.tm_mday)
    {
        //在线礼包重置
        m_tm_generate = m_tm_now;
        m_generate_time = m_time_now;
        m_sign_time = 0;
        m_cur_online_libao = 1;
        m_cur_libao_state = 0;
        m_cur_libao_get_time = 0;
        m_canGet = 1;
        save();
    }
    else
    {
        if (m_sign_time > 0 && m_cur_online_libao > 0 && m_cur_libao_state == 0 && m_cur_libao_get_time <= m_time_now)
        {
            m_cur_libao_state = 1;
            m_canGet = 1;
        }
    }
}

int char_sign_data::getSignAction()
{
    //未签到
    if (m_sign_time == 0)
    {
        return 1;
    }
    //或者可以领取
    if (m_cur_libao_state == 1)
    {
        return 1;
    }
    for (std::map<int, int>::iterator it = m_getted.begin(); it != m_getted.end(); ++it)
    {
        if (it->second == 1)
        {
            return 1;
        }
    }
    return 0;
}

int char_sign_data::getOnlineLibaoLeftNum()
{
    int total = m_handle.getOnlineLibaoCnt();
    //未签到
    if (m_sign_time <= 0)
    {
        return total;
    }
    int left = 0;
    if (m_cur_online_libao > 0 && m_cur_online_libao <= total)
    {
        left = total - m_cur_online_libao + 1;
    }
    return left;
}

void char_daily_recharge::save()
{
    InsertSaveDb("update char_daily_recharge set get="
        + LEX_CAST_STR(m_get) + ",viewed=" + LEX_CAST_STR(m_viewed)
        + ",recharge=" + LEX_CAST_STR(m_recharge)
        + " where cid=" + LEX_CAST_STR(m_cid));
}

void char_daily_recharge::dailyReset()
{
    if (m_get == 2)
    {
        m_get = 0;
    }
    m_viewed = 0;
    m_recharge = 0;
}

#ifdef VN_EN_SERVER
bool template_event::isOpen()
{
    if (m_start_time == -1 && m_end_time == -1)
        return true;
    time_t time_now = time(NULL);
    return m_start_time <= time_now && time_now < m_end_time;
}

bool template_event::inList(int value)
{
    if (m_rewards.find(value) != m_rewards.end())
    {
        return m_rewards[value].get() != NULL;
    }
    else
    {
        return false;
    }
}

int template_event::checkMore(int value)
{
    for (std::map<int, boost::shared_ptr<baseLibao> >::reverse_iterator r_it = m_rewards.rbegin(); r_it != m_rewards.rend(); ++r_it)
    {
        if (value >= r_it->first)
        {
            return r_it->first;
        }
    }
    return 0;
}

char_event_data::char_event_data(template_event& h, int cid)
:m_handle(h)
{
    m_cid = cid;
    m_canGet = 0;
}

int char_event_data::getAwards(CharData& cdata, int need, json_spirit::Object& robj)
{
    if (m_geted[need] == 1)
    {
        if (m_handle.m_rewards.find(need) != m_handle.m_rewards.end())
        {
            baseLibao* lb = m_handle.m_rewards[need].get();
            if (lb)
            {
                //给东西
                if ((cdata.m_bag.size()-cdata.m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, 0, cdata.m_level, 0, NULL, &robj, true, give_libao_loot);
            }
        }
        m_geted[need] = 2;
        m_canGet = 0;
        for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
        {
            if (it->second == 1)
            {
                m_canGet = 1;
                break;
            }
        }
        int state = cdata.getOpeningState();
        cdata.notifyEventState(top_level_event_opening, state, 0);
        save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void char_event_data::update(int value)
{
    if (m_handle.m_type == event_attack_power)
    {
        value = m_handle.checkMore(value);
    }
    if (value <= 0)
        return;
    if (m_handle.inList(value))
    {
        if (m_geted[value] == 0)
        {
            m_geted[value] = 1;
            m_finish_list.push_back(value);
            save();
            m_canGet = 1;
            //通知点亮
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_cid).get();
            if (pc)
            {
                pc->notifyEventState(top_level_event_opening, 1, 0);
            }
        }
    }
    if (m_handle.m_type == event_attack_power)
    {
        --value;
        update(value);
    }
}

void char_event_data::save()
{
    std::string sql = "update char_event_data set reward_getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "',finish_list='";
    int finish_count = 0;
    for (std::vector<int>::iterator it = m_finish_list.begin(); it != m_finish_list.end(); ++it)
    {
        if (finish_count)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++finish_count;
    }
    sql += ("' where cid=" + LEX_CAST_STR(m_cid) + " and type=" + LEX_CAST_STR(m_handle.m_type));
    InsertSaveDb(sql);
}

bool login_event::isOpen()
{
    time_t time_now = time(NULL);
    return m_start_time <= time_now && time_now < m_end_time;
}

bool login_event::inList(int day)
{
    int start_index = (m_cur_state-1) * 3 + 1, end_index = start_index+2;
    if (day > end_index || day < start_index)
    {
        return false;
    }
    if (m_rewards.find(day) != m_rewards.end())
    {
        return m_rewards[day].get() != NULL;
    }
    return false;
}

char_login_data::char_login_data(login_event& h, int cid)
:m_handle(h)
{
    m_cid = cid;
    m_canGet = 0;
}

int char_login_data::getAwards(CharData& cdata, int day, json_spirit::Object& robj)
{
    if (!m_handle.inList(day))
    {
        return HC_ERROR;
    }
    if (m_geted[day] == 1)
    {
        if (m_handle.m_rewards.find(day) != m_handle.m_rewards.end())
        {
            baseLibao* lb = m_handle.m_rewards[day].get();
            if (lb)
            {
                //给东西
                if ((cdata.m_bag.size()-cdata.m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, 0, cdata.m_level, 0, NULL, &robj, true, give_libao_loot);
            }
        }
        m_geted[day] = 2;
        m_canGet = 0;
        std::map<int, int>::iterator it = m_geted.begin();
        while(it != m_geted.end())
        {
            if (it->second == 1)
            {
                m_canGet = 1;
                break;
            }
            ++it;
        }
        cdata.setExtraData(char_data_type_daily, char_data_daily_login_event, 1);
        int state = cdata.getOpeningState();
        cdata.notifyEventState(top_level_event_opening, state, 0);
        save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void char_login_data::check(CharData& cdata)
{
    int today_get = cdata.queryExtraData(char_data_type_daily, char_data_daily_login_event);
    if (today_get > 0)
        return;
    int start_index = (m_handle.m_cur_state-1) * 3 + 1, end_index = start_index+3;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator itr = m_handle.m_rewards.begin(); itr != m_handle.m_rewards.end(); ++itr)
    {
        if (itr->second.get())
        {
            //设置之前阶段的为领取
            if (itr->first < start_index && m_geted[itr->first] != 2)
            {
                m_geted[itr->first] = 2;
                continue;
            }
            if (m_geted[itr->first] == 0)
            {
                m_geted[itr->first] = 1;
                m_finish_list.push_back(itr->first);
                m_canGet = 1;
                cdata.setExtraData(char_data_type_daily, char_data_daily_login_event, 1);
                //通知点亮
                CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_cid).get();
                if (pc)
                {
                    pc->notifyEventState(top_level_event_opening, 1, 0);
                }
                save();
                return;
            }
        }
    }
}

void char_login_data::save()
{
    std::string sql = "update char_event_login set reward_getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "',finish_list='";
    int finish_count = 0;
    for (std::vector<int>::iterator it = m_finish_list.begin(); it != m_finish_list.end(); ++it)
    {
        if (finish_count)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++finish_count;
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}
#endif

