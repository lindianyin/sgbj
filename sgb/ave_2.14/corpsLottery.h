#pragma once

//军团罗生盘

#include "base_item.h"

#include "net.h"

#include "singleton.h"

#include <vector>

using namespace net;

struct corpsLottery
{
    corpsLottery(int id);

    int corps_id;
    json_spirit::Value m_notices_value;

    int total_score;    //总积分
    int total_count;    //总次数

    bool need_save;

    int addCount();

    int addScore(int a);

    //查询抽奖公告
    int queryLotteryNotice(json_spirit::Object& robj);
    //增加记录
    void addLotteryNotice(const std::string& name, Item& item);
    //获奖系统公告
    void broadLotteryNotice(const std::string& name, Item& item);
    //重置
    void Reset();

    void Save();
};

class corpsLotteryMgr
{
public:
    corpsLotteryMgr();

    void reload();

    //可能获得奖励列表
    void getAwards(json_spirit::Array& list);

    //随机物品
    Item random_award(int& add_notice, int& pos);

private:
    //随机物品
    Item random_award();

    std::vector<Item> m_awards;
    std::vector<int> m_gailvs;
    std::vector<int> m_need_notice;
};

//查询军团梅花易数公告
int ProcessQueryCorpsLotteryNotice(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//查询军团梅花易数奖励物品列表
int ProcessQueryCorpsLotteryAwards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//梅花易数抽取奖品
int ProcessCorpsLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//购买道具
int ProcessBuyDaoju(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

