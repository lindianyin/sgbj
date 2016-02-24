#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include <boost/thread.hpp>

#include "base_item.h"
#include "data.h"

using namespace json_spirit;

const int iRewardMinRank = 5;

enum new_rankings_type
{
    rankings_event_stronghold = 1,
    rankings_event_silver = 2,
    rankings_event_recharge = 3,
    rankings_event_gold = 4,
    rankings_event_wash = 5,
    rankings_event_baoshi = 6,
    rankings_event_chengzhang = 7,
};

//��ɫ���������
struct new_char_Rankings
{
    int cid;    //��ɫid
    int rank_type;//�������
    int score;    //����
    time_t refresh_time;    //����ʱ��
    int state;//0δ��ȡ1����ȡ
    new_char_Rankings()
    {
        cid = 0;
        score = 0;
        rank_type = 0;
        refresh_time = 0;
        state = 0;
    }
};

//������Ľ���
struct new_rankings_event_award
{
    int rank;
    std::list<Item> awards;

    new_rankings_event_award()
    {
        rank = 0;
    }
};

//�����
struct new_rankings_event
{
    int type;
    std::string memo;
    //std::string mail_title;
    //std::string mail_content;

    std::map<int, boost::shared_ptr<new_rankings_event_award> > rankings_list;
};

class newRankings
{
public:
    static newRankings* getInstance();
    void reload();
    void getAction(CharData* pc, json_spirit::Array& blist);

    //���������
    void RankingsReward();
    //���»
    void update();
    //��ȡ������Ϣ
    int getRankingsInfo(CharData* pc, json_spirit::Object& robj);
    int getLastRankingsInfo(CharData* pc, json_spirit::Object& robj);
    //����������Ϣ
    void updateEventRankings(int cid, int type, int score);
    //����״̬
    bool getRewardState(int cid, int& type, int& rank);
    bool setRewardGet(int cid);
    //��ȡ����
    int getRankRewards(int cid, json_spirit::Object& robj);

private:
    static newRankings* m_handle;
    int m_event_type;

    int m_top_min_score;
    std::map<int, new_char_Rankings> m_score_maps;//������һ���
    std::list<new_char_Rankings> m_now_Rankings;
    std::list<new_char_Rankings> m_last_Rankings;

    std::vector<new_rankings_event> m_event_list;//���л���б�
};


