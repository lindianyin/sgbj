
#pragma once

#include <vector>
#include <list>
#include "libao.h"

struct CharData;

//武将招募活动
struct new_general_event
{
    //招募武将列表
    std::vector<int> m_general_list;

    std::string m_general_string;

    //招募数量对应的奖励
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;

    bool in_list(int gid);
};

//角色武将招募活动的数据
struct char_general_event
{
    new_general_event& m_handle;
    int m_cid;
    time_t m_start_time;
    time_t m_end_time;
    int m_canGet;
    std::vector<int> m_glist;    //已经招募的活动武将
    std::map<int, int> m_geted;    //已经领取的奖励 first=对应武将个数，second: 1不能领取，1可以领取 2已经领取

    char_general_event(new_general_event& h, int cid);
    void save();
    void addGeneral(int gid);
    void addGeneralCheck(int gid);
    int getAwards(CharData& cdata, int count, json_spirit::Object& robj);
};


//宝石活动
struct new_baoshi_event
{
    int m_min_level;
    //宝石等级对应的奖励
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;

    bool in_list(int level);
};

//角色宝石活动数据
struct char_baoshi_event
{
    new_baoshi_event& m_handle;
    int m_cid;
    time_t m_start_time;
    time_t m_end_time;

    int m_canGet;
    std::map<int, int> m_geted;//已经领取的奖励 first=宝石等级，second: 1不能领取，1可以领取 2已经领取
    std::vector<int> m_glist;    //已经招募的活动武将
    char_baoshi_event(new_baoshi_event& h, int c);
    void save();
    void addBaoshi(int level);
    int getAwards(CharData& cdata, int level, json_spirit::Object& robj);
};

struct sign_online_libao
{
    int id;
    int secs;
    boost::shared_ptr<baseLibao> m_rewards;
};

struct sign_event
{
    //在线礼包
    std::vector<sign_online_libao> m_online_libaos;
    //累计签到礼包
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;

    int get_online_libao_secs(int seq);

    bool in_list(int days);

    baseLibao* getOnlineLibao(int seq);

    int getOnlineLibaoCnt();
};

struct char_sign_data
{
    sign_event& m_handle;
    int m_cid;

    time_t m_generate_time;
    struct tm m_tm_generate;

    std::vector<int> m_sign_data;
    int m_cur_online_libao;
    int m_cur_libao_state;
    time_t m_cur_libao_get_time;
    time_t m_sign_time;
    std::map<int, int> m_getted;

    int m_total_sign;

    time_t m_time_now;
    struct tm m_tm_now;

    int m_canGet;

    char_sign_data(sign_event& h, int cid);
    void save();
    int getAwards(CharData& cdata, int days, json_spirit::Object& robj);
    int getOnlineAwards(CharData& cdata, json_spirit::Object& robj);
    int doSign();
    int getSignAction();
    void checkReset();
    int getOnlineLibaoLeftNum();
};

struct char_daily_recharge
{
    int m_cid;
    int m_recharge;    //今天充值
    int m_get;            //0未达成  1可领取 2已经领取
    int m_viewed;        //今天看过

    void dailyReset();

    void save();
};

struct daily_recharge_event
{
    int m_need;
    boost::shared_ptr<baseLibao> m_reward;
    std::map<int, boost::shared_ptr<char_daily_recharge> > m_chars;
};

class new_event_mgr
{
public:
    new_event_mgr();
    int getActionState(int cid);
    void getActionList(CharData* pc, json_spirit::Array& list);
    void getBaoshiEventList(char_baoshi_event& e, json_spirit::Array& list);
    void getGeneralEventList(char_general_event& e, json_spirit::Array& list);
    const std::string& getGeneralhiEvent();

    void getSignAction(int cid, json_spirit::Array& blist);

    void getSignInfo(char_sign_data& e, json_spirit::Object& robj);

    char_general_event* addCharGeneralEvent(CharData& cdata);
    char_baoshi_event* addCharBaoshiEvent(CharData& cdata);

    char_general_event* getCharGeneralEvent(int cid);
    char_baoshi_event* getCharBaoshiEvent(int cid);
    char_sign_data* getCharSignData(int cid);

    void addBaoshi(int cid, int level);
    void addGeneral(int cid, int gid);
    void addGeneralCheck(int cid, int gid);

    char_daily_recharge* getCharDailyRecharge(int cid);
    void getDailyRechargeAction(int cid, json_spirit::Array& list);
    int getDailyRechargeAward(CharData& cdata, json_spirit::Object& robj);
    void dailyReset();
    void addDailyRecharge(CharData& cdata, int recharge);
    int getDailyRechargeInfo(CharData& cdata, json_spirit::Object& robj);
    int getDailyRechargeNeed(CharData& cdata);

private:
    new_baoshi_event m_baoshi_event;
    new_general_event m_general_event;
    sign_event m_sign_event;

    daily_recharge_event m_daily_recharge_event;

    std::map<int, boost::shared_ptr<char_baoshi_event> > m_char_baoshi_events;
    std::map<int, boost::shared_ptr<char_general_event> > m_char_general_events;

    std::map<int, boost::shared_ptr<char_sign_data> > m_char_sign_events;
};

//查询招募武将活动 cmd：queryGeneralEvent
int ProcessQueryGeneralEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//查询宝石合成活动 cmd：queryBaoshiEvent
int ProcessQueryBaoshiEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//查询签到信息
int ProcessQuerySignInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//签到
int ProcessSign(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//签到
int ProcessDebugSign(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//发送签到按钮状态
int ProcessSendSignState(json_spirit::mObject& o);

//查询每日充值信息
int ProcessQueryDailyRecharge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//查询活动信息
int ProcessQueryEventInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//领取活动奖励
int ProcessGetEventReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询登录活动信息
int ProcessQueryLoginEventInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//领取活动奖励
int ProcessGetLoginEventReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

