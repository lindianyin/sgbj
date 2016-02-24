#pragma once

#include "net.h"
#include "item.h"

struct CharData;

class lottery_event
{
public:
    lottery_event();
    void reload();
    void getInfo(CharData& cdata, json_spirit::Object& robj);
    void getAwards(int type, json_spirit::Array& list);
    int doLottery(CharData& cdata, int type, json_spirit::Object& robj);
    void broadLotteryNotice(const std::string& name, Item& item);

    int openEvent(time_t start_time, time_t end_time);
    int closeEvent();
    bool isOpen() { return m_start_time <= time(NULL) && m_end_time > time(NULL); }
    int getActionState(CharData* pc);
    void notifyActionState(CharData* pc);
    void getButton(CharData* pc, json_spirit::Array& list);
private:
    time_t m_start_time;
    time_t m_end_time;
};

//��ѯ�齱�������Ʒ�б�
int ProcessQueryLotteryEventInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�齱�������Ʒ�б�
int ProcessQueryLotteryEventAwards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ����
int ProcessGetLotteryScore(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ��Ʒ
int ProcessGetLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

