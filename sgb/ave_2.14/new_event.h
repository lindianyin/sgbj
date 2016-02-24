
#pragma once

#include <vector>
#include <list>
#include "libao.h"

struct CharData;

//�佫��ļ�
struct new_general_event
{
    //��ļ�佫�б�
    std::vector<int> m_general_list;

    std::string m_general_string;

    //��ļ������Ӧ�Ľ���
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;

    bool in_list(int gid);
};

//��ɫ�佫��ļ�������
struct char_general_event
{
    new_general_event& m_handle;
    int m_cid;
    time_t m_start_time;
    time_t m_end_time;
    int m_canGet;
    std::vector<int> m_glist;    //�Ѿ���ļ�Ļ�佫
    std::map<int, int> m_geted;    //�Ѿ���ȡ�Ľ��� first=��Ӧ�佫������second: 1������ȡ��1������ȡ 2�Ѿ���ȡ

    char_general_event(new_general_event& h, int cid);
    void save();
    void addGeneral(int gid);
    void addGeneralCheck(int gid);
    int getAwards(CharData& cdata, int count, json_spirit::Object& robj);
};


//��ʯ�
struct new_baoshi_event
{
    int m_min_level;
    //��ʯ�ȼ���Ӧ�Ľ���
    std::map<int, boost::shared_ptr<baseLibao> > m_rewards;

    bool in_list(int level);
};

//��ɫ��ʯ�����
struct char_baoshi_event
{
    new_baoshi_event& m_handle;
    int m_cid;
    time_t m_start_time;
    time_t m_end_time;

    int m_canGet;
    std::map<int, int> m_geted;//�Ѿ���ȡ�Ľ��� first=��ʯ�ȼ���second: 1������ȡ��1������ȡ 2�Ѿ���ȡ
    std::vector<int> m_glist;    //�Ѿ���ļ�Ļ�佫
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
    //�������
    std::vector<sign_online_libao> m_online_libaos;
    //�ۼ�ǩ�����
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
    int m_recharge;    //�����ֵ
    int m_get;            //0δ���  1����ȡ 2�Ѿ���ȡ
    int m_viewed;        //���쿴��

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

//��ѯ��ļ�佫� cmd��queryGeneralEvent
int ProcessQueryGeneralEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ��ʯ�ϳɻ cmd��queryBaoshiEvent
int ProcessQueryBaoshiEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯǩ����Ϣ
int ProcessQuerySignInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//ǩ��
int ProcessSign(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//ǩ��
int ProcessDebugSign(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ǩ����ť״̬
int ProcessSendSignState(json_spirit::mObject& o);

//��ѯÿ�ճ�ֵ��Ϣ
int ProcessQueryDailyRecharge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ���Ϣ
int ProcessQueryEventInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ�����
int ProcessGetEventReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ��¼���Ϣ
int ProcessQueryLoginEventInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ�����
int ProcessGetLoginEventReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

