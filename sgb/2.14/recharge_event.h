
#include "base_item.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "json_spirit_value.h"
#include <list>
#include <map>
#include <time.h>

struct CharData;

struct recharge_event_reward
{
    int count;
    std::list<Item> items;
    std::string m_reward_string;
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
    boost::shared_ptr<recharge_event_reward> getReward(int);
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

//��ֵ�
class recharge_event_mgr
{
public:
    //��ѯ��ֵ�
    int queryRechargeEvent(int cid, int type, json_spirit::Object&);
    //��ȡ��ֵ����
    int getReward(CharData* pc, int type, int count);
    //���¼��س�ֵ�
    int reload(int type);
    //���ó�ֵ�
    int reset(int type);
    //���³�ֵ�����
    int updateRechargeEvent(int cid, int count, time_t rt);

    void checkState();
    //��ȡ��ֵ�����Ƿ������ȡ
    int getCanget(CharData* pc, int type);

    bool isOpen(int type = 0);

    static recharge_event_mgr* getInstance();
    
private:
    static recharge_event_mgr* m_handle;
    boost::shared_ptr<char_recharge_event> getChar(int cid);
    total_recharge_event total_event;
    single_recharge_event single_event;
    std::map<int, boost::shared_ptr<char_recharge_event> > m_char_recharge_events;
};

