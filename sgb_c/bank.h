#pragma once

#include <stdint.h>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "net.h"
#include "data.h"
#include "spls_timer.h"

const int iBankSubCaseCount = 4;

struct bank_sub_case
{
    int _feedback_gold;
    int _need_mins;

    bank_sub_case()
    {
        _feedback_gold = 0;
        _need_mins = 0;
    }
};

struct bank_base_case
{
    int _id;
    int _need_gold;
    int _total_feedback_gold;

    bank_sub_case _case_map[iBankSubCaseCount];
};

struct char_bank_case
{
    int m_cid;
    int m_case_id;
    int m_state;    //0����Ͷ��1δͶ��2��Ͷ��3ȫ����ȡ
    int m_get_gold;//�ۼ�����ȡ
    int m_feedback_state[iBankSubCaseCount];        //0������ȡ1δ��ȡ2����ȡ
    time_t m_start_time;    //��ʼͶ��ʱ��
    time_t m_sub_can_get_time[iBankSubCaseCount];    //������ȡ��ʱ��
    boost::shared_ptr<bank_base_case> m_base;

    boost::uuids::uuid m_timer;

    void save();

    char_bank_case()
    {
        m_cid = 0;
        m_case_id = 0;
        m_state = 1;
        m_get_gold = 0;
        memset(m_feedback_state, 0, sizeof(int)*iBankSubCaseCount);
        m_start_time = 0;
        memset(m_sub_can_get_time, 0, sizeof(time_t)*iBankSubCaseCount);
    }
};

typedef std::vector<boost::shared_ptr<char_bank_case> > char_bank_list;

//Ǯׯ
class bankMgr
{
public:
    static bankMgr* getInstance();
    void reload();
    boost::shared_ptr<bank_base_case> GetBaseCase(int case_id);
    boost::shared_ptr<char_bank_list> GetCharBankList(int cid);
    boost::shared_ptr<char_bank_list> open(int cid);
    void getButton(CharData* pc, json_spirit::Array& blist);

private:
    static bankMgr* m_handle;
    int m_max_case_id;
    std::map<int, boost::shared_ptr<bank_base_case> > m_base_banks;
    std::map<int, boost::shared_ptr<char_bank_list> > m_char_banks;
};

//��ȡ���г���
int ProcessGetBankSilver(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�������д��
int ProcessDealBankSilver(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������Ŀ״̬
int ProcessGetBankCaseState(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//Ͷ��������Ŀ
int ProcessBuyBankCase(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡͶ�ʻر�
int ProcessGetBankFeedback(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ������Ŀ��Ϣ
int ProcessGetCaseInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ������Ŀ�б�
int ProcessGetBankList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//Ͷ�ʿ���ȡ
int ProcessBankCaseCanGet(json_spirit::mObject& o);

