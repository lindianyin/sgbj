
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
const int iDefaultDailyRecharge = 100;

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

