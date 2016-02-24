#pragma once

#include "net.h"
#include "base_item.h"

struct CharData;

class lottery_event
{
public:
    lottery_event();
    //��ѯ�齱����
    int queryLotteryNotice(json_spirit::Object& robj);
    //���Ӽ�¼
    void addLotteryNotice(const std::string& name, Item& item);
    //��ϵͳ����
    void broadLotteryNotice(const std::string& name, Item& item);

    void Save();

    void reload();

    //���ܻ�ý����б�
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

    //�����Ʒ
    Item random_award();

    //�����Ʒ
    Item random_award(int& add_notice, int& pos);

    std::vector<Item> m_awards;
    std::vector<int> m_gailvs;
    std::vector<int> m_need_notice;

    std::string m_strLotteryGet;
    std::string m_strLotteryMsg;
};

//��ѯ�齱�����
int ProcessQueryLotteryEventNotice(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ�齱�������Ʒ�б�
int ProcessQueryLotteryEventAwards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ȡ��Ʒ
int ProcessEventLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

