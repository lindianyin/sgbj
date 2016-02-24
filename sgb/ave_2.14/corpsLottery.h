#pragma once

//����������

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

    int total_score;    //�ܻ���
    int total_count;    //�ܴ���

    bool need_save;

    int addCount();

    int addScore(int a);

    //��ѯ�齱����
    int queryLotteryNotice(json_spirit::Object& robj);
    //���Ӽ�¼
    void addLotteryNotice(const std::string& name, Item& item);
    //��ϵͳ����
    void broadLotteryNotice(const std::string& name, Item& item);
    //����
    void Reset();

    void Save();
};

class corpsLotteryMgr
{
public:
    corpsLotteryMgr();

    void reload();

    //���ܻ�ý����б�
    void getAwards(json_spirit::Array& list);

    //�����Ʒ
    Item random_award(int& add_notice, int& pos);

private:
    //�����Ʒ
    Item random_award();

    std::vector<Item> m_awards;
    std::vector<int> m_gailvs;
    std::vector<int> m_need_notice;
};

//��ѯ����÷����������
int ProcessQueryCorpsLotteryNotice(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ����÷������������Ʒ�б�
int ProcessQueryCorpsLotteryAwards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//÷��������ȡ��Ʒ
int ProcessCorpsLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//�������
int ProcessBuyDaoju(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

