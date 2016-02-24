#pragma once

#include <list>
#include <string>

#include "json_spirit.h"
#include "base_item.h"

#include "boost/smart_ptr/shared_ptr.hpp"

class CharData;

static volatile int iMaxDailyTaskReward = 6;        //ÿ����������ȡ����

//�ճ����񿪷ŵȼ�
const int iDailyTaskOpenLevel = 25;


enum daily_task_type
{
    daily_task_rest = 1,//�������
    daily_task_salary = 2,//ٺ»
    //daily_task_trade = 3,//ͨ��
    daily_task_race = 4,//����
    daily_task_equipt = 5,//ǿ��
    daily_task_levy = 6,//����
    daily_task_wash = 7,//ϴ��
    daily_task_group_copy = 8,//ս��
    daily_task_horse = 9,//ս��
    daily_task_farm = 10,//����
    daily_task_trade = 11,//ó��
    daily_task_corp_jisi = 12,//���ż���
    daily_task_corp_yanhui = 13,//�������
    daily_task_guard = 14,//����
    daily_task_guard_rob = 15,//��ȡ
    daily_task_guard_help = 16,//�������ѻ���
    daily_task_servant = 17,//�Ҷ�
    daily_task_servant_rescue = 18,//���׳��
    daily_task_servant_catch = 19,//ץ��׳��
    daily_task_attak_boss1 = 20,//��ս����Ӧ��
    daily_task_attak_boss4 = 21,//��ս������ڤ
    daily_task_camprace = 22,//��Ӫս
    daily_task_general_reborn = 23,//�佫����
    daily_task_general_train = 24,//�佫ѵ��
    daily_task_baoshi_exchange = 25,//�һ���ʯ
    daily_task_maze = 26,//������
    daily_task_corp_explore = 27,//����̽��
    daily_task_corp_ymsj = 28,//ԯ�����
    daily_task_corp_lottery = 29,//������
    daily_task_get_yushi = 30,//��ȡ��ʯ
    daily_task_throne_con = 31,//�ΰ�
};

//�ճ���������
struct base_daily_task
{
    int task_id;
    int task_pos;//������
    int can_findback;//�Ƿ����ڿ��һ�
    int needtimes;
    int score;
    std::string memo;
    bool to_obj(boost::shared_ptr<CharData> cdata, json_spirit::Object& robj);
};

//��ͬ��ȡ������������
struct char_daily_task_rewards
{
    int _needscore;
    int _needlevel;
    std::list<Item> _rewards;
    std::string toString(int level);
};

class dailyTaskMgr
{
public:
    void updateDailyTask(CharData& cdata, int task_id, int times = 1);
    void toObj(CharData& cdata, json_spirit::Object& info);
    void getnewReward(CharData& cdata, int reward_id, json_spirit::Object& info);
    int _newrewardTask(CharData& cdata, int reward_id, json_spirit::Object& robj);
    int findBack(boost::shared_ptr<CharData> cdata, int id);

    bool canGetReward(CharData& cdata);
    boost::shared_ptr<base_daily_task> getDaily_task(int id);
    static dailyTaskMgr* getInstance();
private:
    static dailyTaskMgr* m_handle;

    void reload();

    //�ճ�������
    std::vector<char_daily_task_rewards> m_daily_task_rewards;
    std::map<int, boost::shared_ptr<base_daily_task> > m_base_daily_task_list;

};

