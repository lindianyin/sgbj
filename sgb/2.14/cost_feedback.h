
#pragma once

#include "net.h"
#include "data.h"

struct feedback
{
    int cid;
    int day;
    int gold;
    int silver;    
    int get;
    feedback(int c, int d);
    void save();
};

struct char_cost_feedback
{
    int m_cid;
    //int m_can_get;
    std::vector<feedback> m_feedbacks;

    void addFeedback(int day, int gold, int silver);
    feedback* getFeedback(int day);
    char_cost_feedback(int cid, int day);
    int canGet();
};

class cost_feedback_event
{
public:
    cost_feedback_event();
    void load();
    int getGoldPercent();
    int getSilverPercent();
    bool isOpen();

    bool canGet();

    int getDay(time_t t);

    boost::shared_ptr<char_cost_feedback> getChar(int cid);

    void update_feedback_gold_event(int cid, int gold);

    void update_feedback_silver_event(int cid, int silver);

    // type 1 金币  2银币 3金币加银币
    void openEvent(const std::string& eventName, const std::string& content, time_t start_time, int type, int percent, int day);

    void closeEvent();

    //领取回礼奖励
    int getFeedbackAward(CharData& cdata, int day, json_spirit::Object& robj);

    void getAction(CharData& cdata, json_spirit::Array& elist);

    int getEventInfo(CharData& cdata, json_spirit::Object& robj);

    void debugSetDay(int day);

private:
    time_t m_start_time;
    time_t m_end_time;
    time_t m_over_time;

    time_t m_start_day;

    int m_day;
    int m_type;
    
    int m_silver_feedback_percent;
    int m_gold_feedback_percent;

    std::string m_event_title;
    std::string m_event_content;

    std::map<int, boost::shared_ptr<char_cost_feedback> > m_chars;
};

//查询消费有礼活动
int ProcessQueryFeedbackEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//领取回礼奖励
int getFeedbackAward(int cid, int day);

