
#include "lottery_event.hpp"

#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>

#include "loot.h"
#include "utils_all.h"
#include "data.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <list>
#include "statistics.h"
#include "combat.h"
#include "singleton.h"
#include "libao.h"
#include "spls_timer.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

int giveLoots(CharData* cdata, Item& getItem, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

extern std::string strCorpsLotteryGet;
extern std::string strCorpsLotteryMsg;

#define LOTTERY_EVENT_FIELD "lottery_event"
#define LOTTERY_EVENT_LITERAL "lottery_event_literal"

lottery_event::lottery_event()
{
    m_start_time = 0;
    m_end_time = 0;
    m_cost_gold = 20;
    m_spic = 1;

    m_strLotteryGet = "";
    m_strLotteryMsg = "";
    reload();
    need_save = false;
}

int lottery_event::doLottery(CharData& cdata, json_spirit::Object& robj)
{
    time_t tnow = time(NULL);
    if (m_start_time > tnow && m_end_time <= tnow)
    {
        return HC_ERROR;
    }
    
    if (cdata.m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    
    if (cdata.addGold(-m_cost_gold) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    cdata.NotifyCharData();

    int need_notice = 0, pos = 1;
    Item item = random_award(need_notice, pos);
    if (need_notice)
    {
        addLotteryNotice(cdata.m_name, item);
        broadLotteryNotice(cdata.m_name, item);
    }

    giveLoots(&cdata, item, 0, cdata.m_level, 0, NULL, NULL, false, give_lottery);
    robj.push_back( Pair("pos", pos) );
    robj.push_back( Pair("gold", cdata.gold()) );
    return HC_SUCCESS;
}

//查询抽奖公告
int lottery_event::queryLotteryNotice(json_spirit::Object& robj)
{
    robj.push_back( Pair("list", m_notices_value.get_array()) );
    return HC_SUCCESS;
}

//增加记录
void lottery_event::addLotteryNotice(const std::string& name, Item& item)
{
    std::string what = m_strLotteryGet;
    str_replace(what, "$W", item.toString(true));

    json_spirit::Array& notice_array = m_notices_value.get_array();
    json_spirit::Object obj;
    obj.push_back( Pair("name", name) );
    obj.push_back( Pair("get", what) );
    notice_array.push_back(obj);
    while ((int)notice_array.size() > 20)
    {
        notice_array.erase(notice_array.begin());
    }
    need_save = true;
    Save();
}

void lottery_event::getAction(json_spirit::Array& elist)
{
    time_t tnow = time(NULL);
    if (m_start_time > tnow || m_end_time <= tnow)
    {
        return;
    }
    
    json_spirit::Object obj;
    obj.push_back( Pair("type", top_level_event_lottery_event) );
    obj.push_back( Pair("active", 0) );
    obj.push_back( Pair("spic", m_spic) );
    elist.push_back(obj);
}

//通告获得
void lottery_event::broadLotteryNotice(const std::string& name, Item& item)
{
    std::string msg = m_strLotteryMsg;
    str_replace(msg, "$N", MakeCharNameLink(name));
    std::string reward_msg = "";
    if (item.type == item_type_baoshi)
    {
        baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(item.id);
        if (bbs)
        {
            reward_msg = bbs->Name_to_Link(item.fac);
        }
    }
    else if (item.type == item_type_silver || item.type == item_type_gold)
    {
        reward_msg = item.toString(true);
    }
    else if (item.type == item_type_equipment)
    {
        reward_msg = item.toString(true);
    }
    else
    {
        return;
    }
    if (msg != "")
    {
        str_replace(msg, "$M", reward_msg);
        //GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        json_spirit::mObject mobj;
        mobj["cmd"] = "broadCastMsg";
        mobj["msg"] = msg;

        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(4, 1, mobj,1));
        splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return;
}

void lottery_event::reload()
{
    Query q(GetDb());
    q.get_result("select value from custom_settings where code='lottery_event_notices'");
    if (q.fetch_row())
    {
        std::string lottery_notices = q.getstr();
        q.free_result();
        json_spirit::read(lottery_notices, m_notices_value);
    }
    else
    {
        std::string lottery_notices = "[]";
        q.free_result();
        json_spirit::read(lottery_notices, m_notices_value);
    }
    
    q.get_result("select pos,itemType,itemId,count,fac,gailv,notice from base_lottery_event_awards where 1 order by pos");
    while (q.fetch_row())
    {
        int pos = q.getval();
        assert(pos == m_awards.size() + 1);
        Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();
        item.fac = q.getval();
        item.spic = item.id;
        //礼包图片特殊处理
        if (item.type == item_type_libao)
        {
            baseLibao* p = libao_mgr::getInstance()->getBaselibao(item.id);
            if (p)
            {
                item.spic = p->m_spic;
            }
        }

        m_gailvs.push_back(q.getval());
        m_awards.push_back(item);
        m_need_notice.push_back(q.getval());
    }
    q.free_result();

    std::string data = GeneralDataMgr::getInstance()->getStr(LOTTERY_EVENT_FIELD);

    json_spirit::mValue value;
    json_spirit::read(data, value);
    if (value.type() == json_spirit::obj_type)
    {
        json_spirit::mObject& o = value.get_obj();
        READ_INT_FROM_MOBJ(m_start_time,o,"start_time");
        READ_INT_FROM_MOBJ(m_end_time,o,"end_time");
        READ_INT_FROM_MOBJ(m_spic,o,"spic");
        READ_INT_FROM_MOBJ(m_cost_gold,o,"gold");
        if (m_cost_gold < 0)
        {
            m_cost_gold = 20;
        }
    }

    std::string literal = GeneralDataMgr::getInstance()->getStr(LOTTERY_EVENT_LITERAL);
    json_spirit::mValue lv;
    json_spirit::read(literal, lv);
    if (lv.type() == json_spirit::obj_type)
    {
        json_spirit::mObject& o = lv.get_obj();
        READ_STR_FROM_MOBJ(m_strLotteryGet,o,"get");
        READ_STR_FROM_MOBJ(m_strLotteryMsg,o,"announce");
        //cout<<"reload ..obj..."<<endl;
        //cout<<"["<<literal<<"]"<<endl;
        //cout<<"["<<m_strLotteryGet<<"]"<<endl;
        //cout<<"["<<m_strLotteryMsg<<"]"<<endl;
        if (m_strLotteryGet == "")
        {
            m_strLotteryGet = strCorpsLotteryGet;
        }
        if (m_strLotteryMsg == "")
        {
            m_strLotteryMsg = strCorpsLotteryMsg;
        }
    }
    else
    {
        m_strLotteryGet = strCorpsLotteryGet;
        m_strLotteryMsg = strCorpsLotteryMsg;
        //cout<<"can not reload ..obj..."<<endl;
        //cout<<"["<<m_strLotteryGet<<"]"<<endl;
        //cout<<"["<<m_strLotteryMsg<<"]"<<endl;
    }
}

//可能获得奖励列表
void lottery_event::getAwards(json_spirit::Array& list)
{
    for (std::vector<Item>::iterator it = m_awards.begin(); it != m_awards.end(); ++it)
    {
        Item& item = *it;
        json_spirit::Object obj;
        item.toObj(obj);
        list.push_back(obj);
    }
}

void lottery_event::clearMsg()
{
    json_spirit::read("[]", m_notices_value);
    need_save = true;
    Save();
}

int lottery_event::openEvent(int spic, time_t start_time, time_t end_time, int gold)
{
    m_spic = spic;
    m_start_time = start_time;
    m_end_time = end_time;

    if (gold > 0)
    {
        m_cost_gold = gold;
    }
    else
    {
        m_cost_gold = 20;
    }
    json_spirit::Object o;
    o.push_back( Pair("start_time", m_start_time) );
    o.push_back( Pair("end_time", m_end_time) );
    o.push_back( Pair("spic", m_spic) );
    o.push_back( Pair("gold", m_cost_gold) );
    //保存
    GeneralDataMgr::getInstance()->setStr(LOTTERY_EVENT_FIELD, json_spirit::write(o));
    return HC_SUCCESS;
}

int lottery_event::closeEvent()
{
    json_spirit::read("[]", m_notices_value);
    need_save = true;
    m_start_time = 0;
    m_end_time = 0;
    Save();

    //保存
    json_spirit::Object o;
    o.push_back( Pair("start_time", m_start_time) );
    o.push_back( Pair("end_time", m_end_time) );
    o.push_back( Pair("spic", m_spic) );
    o.push_back( Pair("gold", m_cost_gold) );
    //保存
    GeneralDataMgr::getInstance()->setStr(LOTTERY_EVENT_FIELD, json_spirit::write(o));
    return HC_SUCCESS;
}

void lottery_event::setLiteral(const std::string& getMsg, const std::string& msg)
{
    m_strLotteryGet = getMsg;
    m_strLotteryMsg = msg;

    json_spirit::Object o;
    o.push_back( Pair("get", getMsg) );
    o.push_back( Pair("announce", msg) );

    //保存
    GeneralDataMgr::getInstance()->setStr(LOTTERY_EVENT_LITERAL, json_spirit::write(o));
}

//随机物品
Item lottery_event::random_award(int& add_notice, int& pos)
{
    add_notice = 0;
    pos = 1;
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    boost::random::discrete_distribution<> dist(m_gailvs);

    int idx = dist(gen);
    if (idx >= 0 && idx < m_awards.size())
    {
        add_notice = m_need_notice[idx];
        pos = idx + 1;
        return m_awards[idx];
    }
    //不可能出现的异常
    ERR();
    add_notice = m_need_notice[0];
    return m_awards[0];
}

void lottery_event::Save()
{
    if (need_save)
    {
        //保存
        InsertSaveDb("replace into custom_settings (code,value) values ('lottery_event_notices','" +GetDb().safestr(json_spirit::write(m_notices_value)) + "')");
        need_save = false;
    }
}

//查询抽奖活动公告
int ProcessQueryLotteryEventNotice(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    Singleton<lottery_event>::Instance().queryLotteryNotice(robj);
    return HC_SUCCESS;
}

//查询抽奖活动奖励物品列表
int ProcessQueryLotteryEventAwards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array list;
    Singleton<lottery_event>::Instance().getAwards(list);
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("gold", Singleton<lottery_event>::Instance().costGold()));
    robj.push_back( Pair("end_time", Singleton<lottery_event>::Instance().endTime()));
    return HC_SUCCESS;
}

//抽取奖品
int ProcessEventLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<lottery_event>::Instance().doLottery(*pc, robj);
}

