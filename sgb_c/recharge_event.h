
#include "item.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "json_spirit_value.h"
#include <list>
#include <map>
#include <time.h>
#include "net.h"

struct CharData;

struct recharge_event_reward
{
    int count;
    std::list<Item> items;
};

struct char_recharge_event
{
    char_recharge_event()
    {
        m_single_recharge_reward.clear();
        m_total_recharge_reward_get.clear();
        total_recharge = 0;
    };
    int cid;
    int total_recharge;    //累计充值
    std::map<int, int> m_single_recharge_reward;        //可以领取的单笔充值奖励 first=单笔金额，second=可领次数
    std::map<int, int> m_total_recharge_reward_get;    //已经领取的累计充值奖励 frist=累计金额
    int m_single_recharge_can_get;
    int m_total_recharge_can_get;
};

struct recharge_event
{
    //活动是否开启
    bool isOpen();
    //查询奖励
    boost::shared_ptr<recharge_event_reward> getReward(int count);
    //查询奖励
    boost::shared_ptr<recharge_event_reward> getBestReward(int count);

    time_t m_start_time;
    time_t m_end_time;
    bool m_isOpen;

    //单笔活动 first = 单笔金额
    std::map<int, boost::shared_ptr<recharge_event_reward> > m_events;

};

struct single_recharge_event : public recharge_event
{
};

struct total_recharge_event : public recharge_event
{
};

struct first_recharge_event
{
    //查询奖励
    boost::shared_ptr<recharge_event_reward> getReward(int type);
    //首充活动包含第一次充值任意金额和第一次充值满XX
    std::map<int, boost::shared_ptr<recharge_event_reward> > m_events;
};

//充值活动
class recharge_event_mgr
{
public:
    static recharge_event_mgr* getInstance();
    int getRechargeEventState(CharData* pc);
    void getButton(CharData* pc, json_spirit::Array& list);
    /****************充值活动******************/
    int queryRechargeEvent(int cid, int type, json_spirit::Object& robj);//查询充值活动
    int getReward(CharData* pc, int type, int count, json_spirit::Object& robj);//领取充值奖励
    int reload(int type);//重新加载充值活动
    int reset(int type);//重置充值活动
    int updateRechargeEvent(int cid, int count);//更新充值活动奖励
    void checkState();
    int getCanget(CharData* pc, int type);//领取充值奖励是否可以领取
    bool isOpen(int type = 0);

    /****************首充活动******************/
    int queryFirstRechargeEvent(CharData* pc, json_spirit::Object& robj);
    void updateFirstRechargeEvent(CharData* pc, int org_total, int total);
    int getFirstReward(CharData* pc, int type, json_spirit::Object& robj);

private:
    static recharge_event_mgr* m_handle;
    boost::shared_ptr<char_recharge_event> getChar(int cid);
    total_recharge_event total_event;
    single_recharge_event single_event;
    first_recharge_event first_event;
    std::map<int, boost::shared_ptr<char_recharge_event> > m_char_recharge_events;
};

//查询充值活动
int ProcessQueryRechargeEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询首充活动
int ProcessQueryFirstRechargeEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

