#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "map.h"

static volatile int iMaxDailyTaskReward = 6;//ÿ�ձ�����������ȡ����

struct CharData;

enum daily_score_type
{
    DAILY_SCORE_EMPTY = 0,         //�հ�����
    DAILY_SCORE_STRONGHOLD = 1,    //����ؿ�
    DAILY_SCORE_UPGRADE_EQUIPT,    //2ǿ��װ��
    DAILY_SCORE_DECOMPOSE_HERO,    //3�ֽ�Ӣ��
    DAILY_SCORE_SMELT_HERO,        //4����Ӣ��
    DAILY_SCORE_ARENA,             //5 ������
    DAILY_SCORE_COPY_ATTACK,       //6���򸱱�
    DAILY_SCORE_SHENLING_TOWER,    //7������
    DAILY_SCORE_WILD,              //8����
    DAILY_SCORE_GOLD,              //9�������(˫��)
};

//ÿ�ձ�������
struct baseScoreTask
{
    baseScoreTask();
    int id;
    int type;//��������
    int need;//��������
    int quality;//����Ʒ��
    std::string name;//�������
    std::string memo;//��������
    int done_cost;//���Ľ�������Զ����
    int score;//����������
    int silver;//����������
    void toObj(json_spirit::Object& obj);
};

//���ÿ�ձ�����������
struct CharScoreTask
{
    CharScoreTask(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    int m_tid;
    int m_cur;
    int m_state;
    bool m_done;
    boost::shared_ptr<baseScoreTask> m_task;
    void Save();
    void toObj(json_spirit::Object& obj);
};

struct CharScoreSpecial
{
    CharScoreSpecial(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    int m_tid;
    int m_type;
    bool m_done;
    std::string m_name;
    std::string m_memo;
    void Save();
    void toObj(json_spirit::Object& obj);
};

struct CharDailyScore
{
    CharDailyScore(CharData& c)
    :m_charData(c)
    {
        m_special_reset = 0;
        m_score = 0;
        m_task_cnt = 0;
        m_double = false;
        m_reward_num = 0;
    }
    CharData& m_charData;
    int m_special_reset;//Ŀ������ˢ�´���
    std::list<boost::shared_ptr<CharScoreSpecial> > m_specials;//Ŀ������
    std::map<int, boost::shared_ptr<CharScoreTask> > m_score_tasks;//��������
    boost::shared_ptr<CharScoreTask> m_cur_task;//��ǰ��ȡ����
    int m_score;//����
    int m_task_cnt;//���������
    bool m_double;//�Ƿ񷭱�
    int m_reward_num;//������ȡ�˼���
    
    //��ȡ��������
    boost::shared_ptr<CharScoreTask> getCharScoreTask(int id);
    //���ܻ�������
    int acceptScoreTask(int tid);
    //������������
    int cancelScoreTask(int tid);
    //ˢ�»�������
    void refreshScoreTask(bool add = false);
    //��ɻ�������
    int scoreTaskDone(int id, int cost, json_spirit::Object& robj);
    //Ŀ���������
    boost::shared_ptr<CharScoreSpecial> getCharScoreSpecial(int id);
    //ˢ��Ŀ������
    void refreshSpecial();
    //�����б�
    int getScoreTaskList(json_spirit::Object& robj);
    //�����б�
    int getRewardList(json_spirit::Object& robj);
    //��������
    void updateTask(int type, int n = 1);
    int getReward(json_spirit::Object& robj);
    void load();
    void SaveData();
    void dailyUpdate();
    int getActive();
};

struct dailyScoreRewards
{
    int needscore;
    Item rewards;
};

class dailyScoreMgr
{
public:
    dailyScoreMgr();
    void getButton(CharData* pc, json_spirit::Array& list);
    boost::shared_ptr<baseScoreTask> getTask(int tid);    //��������id�������
    bool canGetReward(int cnt, int score);
    void getRewardInfo(json_spirit::Object& robj);
    int getReward(CharData& cdata, int reward_id, int score, json_spirit::Object& robj);
    int RandomGoldTask(bool must = false);
    int RandomTask();
    int RandomFirstTask(int type);
    void dailyUpdate();
private:
    int m_gold_tid;
    std::vector<int> m_gailvs;
    std::vector<int> m_all_task_id;
    std::map<int, boost::shared_ptr<baseScoreTask> > m_total_tasks;//��������
    std::vector<dailyScoreRewards> m_daily_task_rewards;//���н���
};

int ProcessDailyScoreTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessDailyScoreRewardList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessDailyScoreTaskInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessDailyScoreDeal(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

