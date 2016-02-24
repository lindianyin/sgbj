
#include "lottery_event.hpp"

#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>

#include "loot.h"
#include "utils_all.h"
#include "data.h"
#include "errcode_def.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <list>
#include "statistics.h"
#include "singleton.h"
#include "spls_timer.h"

extern std::string strLotteryMsg;

#define LOTTERY_EVENT_FIELD "lottery_event"

lottery_event::lottery_event()
{
    m_start_time = 0;
    m_end_time = 0;
    reload();
}

void lottery_event::reload()
{
    std::string data = GeneralDataMgr::getInstance()->getStr(LOTTERY_EVENT_FIELD);
    json_spirit::mValue value;
    json_spirit::read(data, value);
    if (value.type() == json_spirit::obj_type)
    {
        json_spirit::mObject& o = value.get_obj();
        READ_INT_FROM_MOBJ(m_start_time,o,"start_time");
        READ_INT_FROM_MOBJ(m_end_time,o,"end_time");
    }
    return;
}

void lottery_event::getInfo(CharData& cdata, json_spirit::Object& robj)
{
    robj.push_back( Pair("start_time", m_start_time) );
    robj.push_back( Pair("end_time", m_end_time) );
    int score = cdata.queryExtraData(char_data_type_normal,char_data_normal_lottery_score);
    robj.push_back( Pair("score", score) );
    int state = cdata.queryExtraData(char_data_type_daily,char_data_daily_get_lottery_score);
    robj.push_back( Pair("free_state", state) );
    robj.push_back( Pair("recharge", cdata.m_total_recharge) );
    int total_times = cdata.queryExtraData(char_data_type_normal,char_data_normal_lottery_total_recharge_score) / iLotteryRechargeGet;
    int times = cdata.queryExtraData(char_data_type_normal,char_data_normal_lottery_get_recharge_score);
    if (times < total_times)
    {
        robj.push_back( Pair("recharge_state", total_times - times) );
    }
    else
    {
        robj.push_back( Pair("recharge_state", 0) );
    }
    return;
}

void lottery_event::getAwards(int type, json_spirit::Array& list)
{
    std::list<Item> items;
    Singleton<lootMgr>::Instance().getLotteryLootInfo(type, items);
    itemlistToArray(items,list);
    return;
}

int lottery_event::doLottery(CharData& cdata, int type, json_spirit::Object& robj)
{
    if (!isOpen())
    {
        return HC_ERROR;
    }
    if (cdata.m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    //扣除积分
    int score = cdata.queryExtraData(char_data_type_normal,char_data_normal_lottery_score);
    if (score >= iLotteryCost[type-1])
    {
        cdata.setExtraData(char_data_type_normal,char_data_normal_lottery_score, score-iLotteryCost[type-1]);
    }
    else
    {
        return HC_ERROR;
    }
    //发奖
    std::list<Item> items;
    Singleton<lootMgr>::Instance().getLotteryLoots(type, items);
    giveLoots(&cdata, items, NULL, &robj, true, loot_lottery);
    notifyActionState(&cdata);
    return HC_SUCCESS;
}

void lottery_event::broadLotteryNotice(const std::string& name, Item& item)
{
    std::string msg = strLotteryMsg;
    str_replace(msg, "$N", MakeCharNameLink(name));
    str_replace(msg, "$M", item.toString(true));
    json_spirit::mObject mobj;
    mobj["cmd"] = "broadCastMsg";
    mobj["msg"] = msg;

    boost::shared_ptr<splsTimer> tmsg;
    tmsg.reset(new splsTimer(4, 1, mobj,1));
    splsTimerMgr::getInstance()->addTimer(tmsg);
    return;
}

int lottery_event::getActionState(CharData* pc)
{
    int active = 0;
    //抽奖状态
    int score = pc->queryExtraData(char_data_type_normal,char_data_normal_lottery_score);
    if (score >= iLotteryCost[0])
    {
        active = 1;
    }
    if (active != 1)
    {
        //免费领取状态
        int state = pc->queryExtraData(char_data_type_daily,char_data_daily_get_lottery_score);
        if (state == 0)
        {
            active = 1;
        }
    }
    if (active != 1)
    {
        //充值领取状态
        int total_times = pc->queryExtraData(char_data_type_normal,char_data_normal_lottery_total_recharge_score) / iLotteryRechargeGet;
        int times = pc->queryExtraData(char_data_type_normal,char_data_normal_lottery_get_recharge_score);
        if (times < total_times)
        {
            active = 1;
        }
    }
    return active;
}

void lottery_event::notifyActionState(CharData* pc)
{
    if (!isOpen() || !pc->isLotteryActionOpen())
    {
        return;
    }
    int active = getActionState(pc);
    pc->updateTopButton(top_button_lotteryAction, active);
}

void lottery_event::getButton(CharData* pc, json_spirit::Array& list)
{
    if (!isOpen() || !pc->isLotteryActionOpen())
    {
        return;
    }
    json_spirit::Object obj;
    obj.push_back( Pair("type", top_button_lotteryAction) );
    obj.push_back( Pair("active", getActionState(pc)) );
    list.push_back(obj);
}

int lottery_event::openEvent(time_t start_time, time_t end_time)
{
    m_start_time = start_time;
    m_end_time = end_time;

    json_spirit::Object o;
    o.push_back( Pair("start_time", m_start_time) );
    o.push_back( Pair("end_time", m_end_time) );
    GeneralDataMgr::getInstance()->setStr(LOTTERY_EVENT_FIELD, json_spirit::write(o));
    return HC_SUCCESS;
}

int lottery_event::closeEvent()
{
    m_start_time = 0;
    m_end_time = 0;
    json_spirit::Object o;
    o.push_back( Pair("start_time", m_start_time) );
    o.push_back( Pair("end_time", m_end_time) );
    GeneralDataMgr::getInstance()->setStr(LOTTERY_EVENT_FIELD, json_spirit::write(o));
    return HC_SUCCESS;
}

//查询抽奖活动奖励物品列表
int ProcessQueryLotteryEventInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (!pc->isLotteryActionOpen())
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    Singleton<lottery_event>::Instance().getInfo(*pc, robj);
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
    if (!pc->isLotteryActionOpen())
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("type", type) );
    if (type < 1 || type > 3)
    {
        type = 1;
    }
    robj.push_back( Pair("need", iLotteryCost[type-1]) );
    json_spirit::Array list;
    Singleton<lottery_event>::Instance().getAwards(type,list);
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//领取积分
int ProcessGetLotteryScore(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (!pc->isLotteryActionOpen())
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("type", type) );
    if (type == 1)
    {
        //免费领取
        int state = pc->queryExtraData(char_data_type_daily,char_data_daily_get_lottery_score);
        if (state == 0)
        {
            pc->setExtraData(char_data_type_daily,char_data_daily_get_lottery_score, 1);
            int score = pc->queryExtraData(char_data_type_normal,char_data_normal_lottery_score);
            pc->setExtraData(char_data_type_normal,char_data_normal_lottery_score, score+iLotteryGet);
            Singleton<lottery_event>::Instance().notifyActionState(pc);
            return HC_SUCCESS;
        }
    }
    else if(type == 2)
    {
        //充值领取
        int total_times = pc->queryExtraData(char_data_type_normal,char_data_normal_lottery_total_recharge_score) / iLotteryRechargeGet;
        int times = pc->queryExtraData(char_data_type_normal,char_data_normal_lottery_get_recharge_score);
        if (times < total_times)
        {
            pc->setExtraData(char_data_type_normal,char_data_normal_lottery_get_recharge_score, times + 1);
            int score = pc->queryExtraData(char_data_type_normal,char_data_normal_lottery_score);
            pc->setExtraData(char_data_type_normal,char_data_normal_lottery_score, score+iLotteryGet);
            Singleton<lottery_event>::Instance().notifyActionState(pc);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//抽取奖品
int ProcessGetLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (!pc->isLotteryActionOpen())
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("type", type) );
    if (type < 1 || type > 3)
    {
        type = 1;
    }
    return Singleton<lottery_event>::Instance().doLottery(*pc, type, robj);
}

