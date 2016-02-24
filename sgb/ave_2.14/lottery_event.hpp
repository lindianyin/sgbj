#pragma once

#include "net.h"
#include "base_item.h"

struct CharData;

class lottery_event
{
public:
    lottery_event();
    //查询抽奖公告
    int queryLotteryNotice(json_spirit::Object& robj);
    //增加记录
    void addLotteryNotice(const std::string& name, Item& item);
    //获奖系统公告
    void broadLotteryNotice(const std::string& name, Item& item);

    void Save();

    void reload();

    //可能获得奖励列表
    void getAwards(json_spirit::Array& list);

    int costGold() {return m_cost_gold; }

    time_t endTime() { return m_end_time; }

    int doLottery(CharData& cdata, json_spirit::Object& robj);

    int openEvent(int spic, time_t start_time, time_t end_time, int gold);

    int closeEvent();

    void getAction(json_spirit::Array& elist);

    void setLiteral(const std::string& getMsg, const std::string& msg);

    void clearMsg();

private:
    int m_spic;
    
    time_t m_start_time;
    time_t m_end_time;

    int m_cost_gold;

    json_spirit::Value m_notices_value;

    bool need_save;

    //随机物品
    Item random_award();

    //随机物品
    Item random_award(int& add_notice, int& pos);

    std::vector<Item> m_awards;
    std::vector<int> m_gailvs;
    std::vector<int> m_need_notice;

    std::string m_strLotteryGet;
    std::string m_strLotteryMsg;
};

//查询抽奖活动公告
int ProcessQueryLotteryEventNotice(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//查询抽奖活动奖励物品列表
int ProcessQueryLotteryEventAwards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//抽取奖品
int ProcessEventLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

