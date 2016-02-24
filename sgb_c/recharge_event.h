
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
    int total_recharge;    //�ۼƳ�ֵ
    std::map<int, int> m_single_recharge_reward;        //������ȡ�ĵ��ʳ�ֵ���� first=���ʽ�second=�������
    std::map<int, int> m_total_recharge_reward_get;    //�Ѿ���ȡ���ۼƳ�ֵ���� frist=�ۼƽ��
    int m_single_recharge_can_get;
    int m_total_recharge_can_get;
};

struct recharge_event
{
    //��Ƿ���
    bool isOpen();
    //��ѯ����
    boost::shared_ptr<recharge_event_reward> getReward(int count);
    //��ѯ����
    boost::shared_ptr<recharge_event_reward> getBestReward(int count);

    time_t m_start_time;
    time_t m_end_time;
    bool m_isOpen;

    //���ʻ first = ���ʽ��
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
    //��ѯ����
    boost::shared_ptr<recharge_event_reward> getReward(int type);
    //�׳�������һ�γ�ֵ������͵�һ�γ�ֵ��XX
    std::map<int, boost::shared_ptr<recharge_event_reward> > m_events;
};

//��ֵ�
class recharge_event_mgr
{
public:
    static recharge_event_mgr* getInstance();
    int getRechargeEventState(CharData* pc);
    void getButton(CharData* pc, json_spirit::Array& list);
    /****************��ֵ�******************/
    int queryRechargeEvent(int cid, int type, json_spirit::Object& robj);//��ѯ��ֵ�
    int getReward(CharData* pc, int type, int count, json_spirit::Object& robj);//��ȡ��ֵ����
    int reload(int type);//���¼��س�ֵ�
    int reset(int type);//���ó�ֵ�
    int updateRechargeEvent(int cid, int count);//���³�ֵ�����
    void checkState();
    int getCanget(CharData* pc, int type);//��ȡ��ֵ�����Ƿ������ȡ
    bool isOpen(int type = 0);

    /****************�׳�******************/
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

//��ѯ��ֵ�
int ProcessQueryRechargeEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�׳�
int ProcessQueryFirstRechargeEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

