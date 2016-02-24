#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include <boost/thread.hpp>

#include "base_item.h"
#include "data.h"
#include "singleton.h"

using namespace json_spirit;

const int iThroneNickNum = 3;
const int iThroneData[iThroneNickNum][2] =
{
    {1, 50000}, //����һ�����������5W
    {2, 30000}, //ս���������������3W
    {4, 20000}, //ͳ˧�������������2W
};

//�ΰ����ģ����ҽ��
const int iThroneCon[3][2] =
{
    {50000, 0},
    {0, 100},
    {0, 500},
};

enum nick_type
{
    nick_zhizun = 1,
    nick_zhanshen = 2,
    nick_tongshuai = 3,
};

//���������������
struct char_throne_Rankings
{
    int cid;    //��ɫid
    int type;   //�ƺ�����
    int score;  //����
    char_throne_Rankings()
    {
        cid = 0;
        score = 0;
        type = 0;
    }
};

//�ΰݽ���
struct throne_award
{
    int silver;
    int gold;
    std::list<Item> awards;
};

//�ΰݼ�¼
struct throne_log
{
    time_t log_time;
    std::string memo;
};

class throneMgr
{
public:
    throneMgr();
    ~throneMgr();
    //���»
    void update();
    //��ȡ������Ϣ
    int getLastRankingsInfo(CharData* pc, json_spirit::Object& robj);
    //��ȡ�ΰ���Ϣ
    int getConInfo(CharData* pc, json_spirit::Object& robj);
    //���Ӳΰ���Ϣ
    int addConLog(std::string msg);
    //�ΰ�
    int congratulation(CharData* pc, json_spirit::mObject& o, json_spirit::Object& robj);
    bool actionState();
    void getAction(CharData& cdata, json_spirit::Array& elist);

private:
    std::list<char_throne_Rankings> m_last_Rankings;//���������������
    std::map<int, boost::shared_ptr<throne_award> > award_list;//�ΰݽ���
    std::list<throne_log> m_log_list;//�ΰݼ�¼
};

int ProcessGetThroneInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessGetConInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessThroneCon(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

