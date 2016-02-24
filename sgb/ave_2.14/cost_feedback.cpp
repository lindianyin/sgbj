
#include "cost_feedback.h"
#include "singleton.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "statistics.h"
#include "combat.h"

#define COST_FEEDBACK_FIELD "cost_feedback"

Database& GetDb();
extern int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

void InsertSaveDb(const std::string& sql);

feedback::feedback(int c, int d)
:cid(c)
,day(d)
,gold(0)
,silver(0)
,get(1)
{
}

void feedback::save()
{
    InsertSaveDb("replace into char_feedback (cid,day,silver,gold,get) values ("
        + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(day) + ","
        + LEX_CAST_STR(silver) + "," + LEX_CAST_STR(gold) + ","
        + LEX_CAST_STR(get) + ")");
}

char_cost_feedback::char_cost_feedback(int cid, int day)
:m_cid(cid)
{
    for (int i = 1; i <= day; ++i)
    {
        feedback f(cid, i);
        m_feedbacks.push_back(f);
    }
    //m_can_get = 0;
}

feedback* char_cost_feedback::getFeedback(int day)
{
    if (day >= 1 && day <= m_feedbacks.size())
    {
        return &(m_feedbacks[day-1]);
    }
    else
    {
        return NULL;
    }
}

void char_cost_feedback::addFeedback(int day, int gold, int silver)
{
    feedback* f = getFeedback(day);
    if (f)
    {
        f->gold += gold;
        f->silver += silver;
        f->save();
    }
}

int char_cost_feedback::canGet()
{
    for (std::vector<feedback>::iterator it = m_feedbacks.begin(); it != m_feedbacks.end(); ++it)
    {
        if ((*it).get == 1 && (it->silver > 0 || it->gold > 0))
        {
            return 1;
        }
    }
    return 0;
}

cost_feedback_event::cost_feedback_event()
{
    load();
}

void cost_feedback_event::load()
{
    m_day = 0;
    m_end_time = 0;
    m_over_time = 0;
    m_start_time = 0;
    m_start_day = 0;
    m_event_title = "";
    m_event_content = "";
    m_type = 0;
    m_silver_feedback_percent = 0;
    m_gold_feedback_percent = 0;

    std::string data = GeneralDataMgr::getInstance()->getStr(COST_FEEDBACK_FIELD);

    json_spirit::mValue value;
    json_spirit::read(data, value);
    if (value.type() == json_spirit::obj_type)
    {
        json_spirit::mObject& o = value.get_obj();
        READ_INT_FROM_MOBJ(m_start_time,o,"start_time");
        READ_INT_FROM_MOBJ(m_day,o,"day");
        READ_STR_FROM_MOBJ(m_event_title,o,"title");
        READ_STR_FROM_MOBJ(m_event_content,o,"content");
        READ_INT_FROM_MOBJ(m_silver_feedback_percent,o,"silver");
        READ_INT_FROM_MOBJ(m_gold_feedback_percent,o,"gold");

        time_t t_c = m_start_time;
        struct tm tm;
        struct tm *t = &tm;
        localtime_r(&t_c, t);
        t->tm_hour = 0;
        t->tm_min = 0;
        t->tm_sec = 0;
        m_start_day = mktime(t);
        m_end_time = m_start_day + iONE_DAY_SECS * m_day;
        m_over_time = m_end_time + 7 * iONE_DAY_SECS;

        if (m_silver_feedback_percent > 0 && m_gold_feedback_percent > 0)
        {
            m_type = 3;
        }
        else if (m_silver_feedback_percent > 0)
        {
            m_type = 2;
        }
        else if (m_silver_feedback_percent > 0)
        {
            m_type = 1;
        }        
        else
        {
            m_type = 0;
        }
    }
}

int cost_feedback_event::getGoldPercent()
{
    return m_gold_feedback_percent;
}

int cost_feedback_event::getSilverPercent()
{
    return m_silver_feedback_percent;
}

int cost_feedback_event::getDay(time_t t)
{
    int tt = t - m_start_day;
    return tt / iONE_DAY_SECS + 1;
}

bool cost_feedback_event::isOpen()
{
    time_t time_now = time(NULL);
    return m_start_time <= time_now && m_end_time > time_now;
}

bool cost_feedback_event::canGet()
{
    time_t time_now = time(NULL);
    return m_end_time <= time_now && time_now < m_over_time;
}

boost::shared_ptr<char_cost_feedback> cost_feedback_event::getChar(int cid)
{
    std::map<int, boost::shared_ptr<char_cost_feedback> >::iterator it = m_chars.find(cid);
    if (it != m_chars.end())
    {
        return it->second;
    }
    Query q(GetDb());
    q.get_result("select day,silver,gold,get from char_feedback where cid=" + LEX_CAST_STR(cid) + " order by day");
    if (q.num_rows() > 0)
    {
        char_cost_feedback* c = new char_cost_feedback(cid, m_day);
        //有数据
        while (q.fetch_row())
        {
            int d = q.getval();
            feedback* f = c->getFeedback(d);
            if (NULL == f)
            {
                continue;
            }
            f->day = d;
            f->silver = q.getval();
            f->gold = q.getval();
            f->get = q.getval();
            if (f->get != 2)
            {
                f->get = 1;
            }
        }
        //c->m_can_get = c->canGet();
        q.free_result();
        m_chars[cid].reset(c);
    }
    else
    {
        q.free_result();
        //没数据
        time_t time_now = time(NULL);
        if (m_start_time <= time_now && m_end_time > time_now)
        {
            m_chars[cid].reset(new char_cost_feedback(cid, m_day));
        }
    }
    return m_chars[cid];
}

void cost_feedback_event::update_feedback_gold_event(int cid, int gold)
{
    time_t time_now = time(NULL);
    if (m_start_time <= time_now && m_end_time > time_now && m_gold_feedback_percent > 0)
    {
        int day = getDay(time_now);
        int gold_f = -gold;//(-gold * m_gold_feedback_percent) / 100;
        if (gold_f > 0)
        {
            boost::shared_ptr<char_cost_feedback> c = getChar(cid);
            if (c.get())
            {
                c->addFeedback(day, gold_f, 0);
            }
        }
    }
}

void cost_feedback_event::update_feedback_silver_event(int cid, int silver)
{
    time_t time_now = time(NULL);
    if (m_start_time <= time_now && m_end_time > time_now && m_silver_feedback_percent > 0)
    {
        int day = getDay(time_now);
        int silver_f = -silver;//(-silver * m_silver_feedback_percent) / 100;
        if (silver_f > 0)
        {
            boost::shared_ptr<char_cost_feedback> c = getChar(cid);
            if (c.get())
            {
                c->addFeedback(day, 0, silver_f);
            }
        }
    }
}

// type 1 金币  2银币 3金币加银币
void cost_feedback_event::openEvent(const std::string& eventName, const std::string& content, time_t start_time, int silver_per, int gold_percent, int day)
{
    cout<<"openEvent("<<start_time<<","<<day<<","<<silver_per<<","<<gold_percent<<endl;
    if (silver_per <= 0 && gold_percent <= 0)
    {
        ERR();
        return;
    }
    closeEvent();

    m_event_title = eventName;
    m_event_content = content;
    m_start_time = start_time;
    m_silver_feedback_percent = silver_per;
    m_gold_feedback_percent = gold_percent;
    m_day = day;

    time_t t_c = m_start_time;
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&t_c, t);
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    m_start_day = mktime(t);
    m_end_time = m_start_day + iONE_DAY_SECS * day;
    m_over_time = m_end_time + 7 * iONE_DAY_SECS;

    if (m_silver_feedback_percent > 0 && m_gold_feedback_percent > 0)
    {
        m_type = 3;
    }
    else if (m_silver_feedback_percent > 0)
    {
        m_type = 2;
    }
    else if (m_silver_feedback_percent > 0)
    {
        m_type = 1;
    }        
    else
    {
        m_type = 0;
    }

    json_spirit::Object o;
    o.push_back( Pair("start_time", m_start_time) );
    o.push_back( Pair("day", m_day) );
    o.push_back( Pair("title", m_event_title) );
    o.push_back( Pair("content", m_event_content) );
    o.push_back( Pair("silver", m_silver_feedback_percent) );
    o.push_back( Pair("gold", m_gold_feedback_percent) );

    GeneralDataMgr::getInstance()->setStr(COST_FEEDBACK_FIELD, json_spirit::write(o));
}

void cost_feedback_event::closeEvent()
{
    m_chars.clear();
    m_start_time = 0;
    m_end_time = 0;
    m_over_time = 0;
    m_event_title = "";
    m_event_content = "";
    m_silver_feedback_percent = 0;
    m_gold_feedback_percent = 0;

    GeneralDataMgr::getInstance()->setStr(COST_FEEDBACK_FIELD, "{}");

    InsertSaveDb("TRUNCATE char_feedback");
}

//领取回礼奖励
int cost_feedback_event::getFeedbackAward(CharData& cdata, int day, json_spirit::Object& robj)
{
    if (!canGet())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_cost_feedback> c = getChar(cdata.m_id);
    if (c.get())
    {
        feedback* f = c->getFeedback(day);
        if (f)
        {
            if (f->get == 1)
            {
                //给东西
                std::list<Item> items;                
                if (f->gold > 0 && m_gold_feedback_percent > 0)
                {
                    //cdata.addGold(f->gold);
                    Item g;
                    g.type = item_type_gold;
                    g.nums = f->gold * m_gold_feedback_percent / 100;

                    if (g.nums > 0)
                    {
                        items.push_back(g);
                    }
                }
                if (f->silver > 0 && m_silver_feedback_percent > 0)
                {
                    //cdata.addSilver(f->silver);
                    Item s;
                    s.type = item_type_silver;
                    s.nums = f->silver * m_silver_feedback_percent / 100;
                    if (s.nums > 0)
                    {
                        items.push_back(s);
                    }
                }
                if (items.size() > 0)
                {
                    giveLoots(&cdata, items, 0, cdata.m_level, 0, NULL, &robj, true, give_feedback);
                }
                f->get = 2;
                f->save();
                if (c->canGet() == 0)
                {
                    //通知移除
                    cdata.notifyEventRemove(top_level_event_feedback);
                }
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

void cost_feedback_event::getAction(CharData& cdata, json_spirit::Array& elist)
{
    if (time(NULL) >= m_over_time)
    {
        return;
    }
    char_cost_feedback* c = getChar(cdata.m_id).get();
    if (!c)
    {
        return;
    }
    int state = 0;
    if (canGet() && c->canGet())
    {
        state = 1;
    }
    else
    {
        if (!isOpen())
        {
            return;
        }
        int view = cdata.queryExtraData(char_data_type_daily, char_data_daily_view_feedback);
        if (view == 0)
        {
            state = 1;
        }
    }
    json_spirit::Object obj;
    obj.push_back( Pair("type", top_level_event_feedback) );
    obj.push_back( Pair("active", state) );
    obj.push_back( Pair("spic", m_type) );
    elist.push_back(obj);
}

int cost_feedback_event::getEventInfo(CharData& cdata, json_spirit::Object& robj)
{
    int view = cdata.queryExtraData(char_data_type_daily, char_data_daily_view_feedback);    
    if (view == 0)
    {
        cdata.setExtraData(char_data_type_daily, char_data_daily_view_feedback, 1);
        //如果未到可领取时间，不闪了
        if (time(NULL) < m_end_time)
        {
            cdata.notifyEventState(top_level_event_feedback, 0, 0);
        }
    }
    robj.push_back( Pair("start_time", m_start_time) );
    robj.push_back( Pair("get_time", m_end_time) );
    robj.push_back( Pair("over", m_over_time) );
    int day = getDay(time(NULL));
    robj.push_back( Pair("day", day) );
    if ((day - m_day) > 7)
    {
        return HC_ERROR;
    }

    robj.push_back( Pair("title", m_event_title) );
    robj.push_back( Pair("content", m_event_content) );
    robj.push_back( Pair("type", m_type) );

    char_cost_feedback* c = getChar(cdata.m_id).get();
    if (c)
    {
        bool cg = canGet();
        json_spirit::Array glist;
        for (int d = 1; d <= m_day; ++d)
        {
            if (d <= c->m_feedbacks.size())
            {
                json_spirit::Object obj;
                feedback& f = c->m_feedbacks[d-1];
                obj.push_back( Pair("day", d) );
                if (m_silver_feedback_percent > 0)
                {
                    obj.push_back( Pair("silver", (f.silver * m_silver_feedback_percent / 100)) );
                }
                if (m_gold_feedback_percent > 0)
                {
                    obj.push_back( Pair("gold", (f.gold * m_gold_feedback_percent / 100)) );
                }
                if (!cg)
                {
                    obj.push_back( Pair("get", 0) );
                }
                else
                {
                    obj.push_back( Pair("get", f.get) );
                }
                glist.push_back(obj);
            }
        }
        robj.push_back( Pair("list", glist) );
    }
    return HC_SUCCESS;
}

//调试设置活动是第几天了
void cost_feedback_event::debugSetDay(int day)
{
    if (day >= 1)
    {
        if (day > (m_day + 7))
        {
            day = m_day + 8;
        }
        int real_day = getDay(time(NULL));
        if (day != real_day)
        {
            int diff = (real_day - day) * iONE_DAY_SECS;
            m_start_time += diff;
            m_end_time += diff;
            m_start_day += diff;
            m_over_time = m_end_time + 7 * iONE_DAY_SECS;

            json_spirit::Object o;
            o.push_back( Pair("start_time", m_start_time) );
            o.push_back( Pair("day", m_day) );
            o.push_back( Pair("title", m_event_title) );
            o.push_back( Pair("content", m_event_content) );
            o.push_back( Pair("silver", m_silver_feedback_percent) );
            o.push_back( Pair("gold", m_gold_feedback_percent) );

            GeneralDataMgr::getInstance()->setStr(COST_FEEDBACK_FIELD, json_spirit::write(o));
        }
    }
}

//查询消费有礼活动
int ProcessQueryFeedbackEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }    
    return Singleton<cost_feedback_event>::Instance().getEventInfo(*pc, robj);
}

