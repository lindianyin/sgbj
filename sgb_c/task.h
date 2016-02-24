#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "map.h"

struct CharData;

const int iTaskDailyCount = 8;

enum task_goal_type
{
    GOAL_EMPTY = 0,         //�հ�����
    GOAL_STRONGHOLD = 1,    //ͨ��ָ���ؿ�(�ؿ�id��1)
    GOAL_CHAR_LEVEL,        //2��Ҵﵽָ���ȼ�(0���ȼ�)
    GOAL_COMPOUND_HERO,     //3�ϳ�ָ���Ǽ�Ӣ��(�Ǽ�������)
    GOAL_COMPOUND_EQUIPT,   //4�ϳ�ָ��Ʒ��װ��(Ʒ�ʣ�����)
    GOAL_UPGRADE_EQUIPT,    //5ǿ��ָ���ȼ�װ��(�ȼ�������)
    GOAL_DECOMPOSE_HERO,    //6�ֽ�ָ���Ǽ�Ӣ��(�Ǽ�������)
    GOAL_SMELT_HERO,        //7����ָ���Ǽ�Ӣ��(�Ǽ�������)
    GOAL_GOLDEN_HERO,       //8���ָ���Ǽ�Ӣ��(�Ǽ�������)
    GOAL_ARENA,             //9 �������ۻ�����(0������)
    GOAL_ARENA_WIN,         //10 ������ʤ���ۻ�����(0������)
    GOAL_SHENLING_TOWER,    //11������ָ������(0������)
    GOAL_CASTLE_LEVEL,      //12�Ǳ�ָ���ȼ�(0���ȼ�)
    GOAL_METALLURGY_LEVEL,  //13���𷿵ȼ�(0���ȼ�)
    GOAL_SMITHY_LEVEL,      //14�����̵ȼ�(0���ȼ�)
    GOAL_BARRACKS_LEVEL,    //15����Ӫ�ȼ�(0���ȼ�)
    GOAL_OFFLINE_REWARD,    //16�һ�����(0��1)
    GOAL_MAP,               //17ͨ��ָ����ͼ(��ͼid��1)
    GOAL_COLLECT,           //18�ղ���Ϸ(0��1)
    GOAL_HERO_PACK,         //19Ӣ�ۿ���(����id������)
    GOAL_MALL_GEM,          //20�������(����id������)
    GOAL_HERO_STAR,         //21���ָ���Ǽ�Ӣ��(�Ǽ�������)
    GOAL_EQUIPT_QUALITY,    //22���ָ��Ʒ��װ��(Ʒ�ʣ�����)
    GOAL_STRONGHOLD_ATTACK, //23����ָ���ؿ�(�ؿ�id������)
    GOAL_COPY_ATTACK,       //24����ָ������(����id������)
    GOAL_EXPLORE,           //25�һ�̽����Ѩ(��Ѩid������)
    GOAL_LEVY,              //26�Ǳ�����(0��1)
    GOAL_RECRUIT,           //27�Ǳ���ļ(0��1)
    GOAL_TREASURE,          //28�ر�ͼ(0��1)
    GOAL_HERO,              //29���ָ��Ӣ��(Ӣ��id������)
    GOAL_UPGRADE_SKILL,     //30��������(����id���ȼ�)
    GOAL_DONE_DAILY_TASK,   //31����ճ��������(0������)
    GOAL_SET_MAGIC,         //32���ó�սħ��(ħ��id��1)


    //ÿ������Ŀ��
    GOAL_DAILY_EXPLORE = 1001,//1001�һ�̽��(0������)
    GOAL_DAILY_RECRUIT,     //1002��ļ(0������)
    GOAL_DAILY_LEVY,        //1003����(0������)
    GOAL_DAILY_ARENA,       //1004������(0������)
    GOAL_DAILY_ARENA_WIN,   //1005������ʤ��(0������)
    GOAL_DAILY_TREASURE,    //1006�ر�ͼ(0������)
    GOAL_DAILY_STRONGHOLD,  //1007��������ؿ�(�������id������)
    GOAL_DAILY_COPY,        //1008�����������(�������id������)
    GOAL_DAILY_PK,          //1009����pk(0������)
    GOAL_DAILY_HERO_PACK_GOLD,//1010��ҿ���(0������)
    GOAL_DAILY_HERO_PACK_SILVER,//1011���뿪��(0������)
    GOAL_DAILY_LEVY_GOLD,   //1012ǿ������(0������)
    GOAL_DAILY_SHENLING_ATTACK,//1013����������(0������)
    GOAL_DAILY_TREASURE_ROB,//1014��ٱ���(0������)
    GOAL_DAILY_SCORE_TASK,  //1015ÿ�ձ�������(0������)
    GOAL_DAILY_GUILD_MOSHEN,//1016����ħ��(0������)
    GOAL_DAILY_COPY_ATTACK, //1017�������⸱��(0������)
    GOAL_DAILY_WILD_LEVY,   //1018��������(0����������)
    GOAL_DAILY_UPGRADE_EQUIPT,//1019ǿ��װ��(0������)
    GOAL_DAILY_COMPOUND_HERO, //1020�ϳ�ָ���Ǽ�Ӣ��(�Ǽ�������)
    GOAL_DAILY_DECOMPOSE_HERO,//1021�ֽ�ָ���Ǽ�Ӣ��(�Ǽ�������)
    GOAL_DAILY_GOLDEN_HERO, //1022���ָ���Ǽ�Ӣ��(�Ǽ�������)
    GOAL_DAILY_BOSS,        //1023����boss(0������)
};

enum task_type
{
    TASK_TYPE_NORMAL = 1,//����
    TASK_TYPE_SUB = 2,//֧��
    //�
    TASK_TYPE_DAILY = 4,//�ճ�
    //����
};

enum task_update_type
{
    TASK_UPDATE = 1,
    TASK_ADD = 2,
    TASK_DELETE = 3,
};

//����Ŀ��
struct baseGoal
{
    int type;//Ŀ������
    int need[2];//Ŀ�����(Ψһid���ߵȼ�����������)
    std::string memo;//Ŀ������
    boost::shared_ptr<baseStronghold> target_stronghold;//Ŀ��ؿ�
    void toObj(json_spirit::Object& obj);
};

struct baseTask
{
    baseTask();
    int id;
    int type;//���������ͣ����ߣ�֧�ߣ�����ճ������ᣩ
    int task_id;//�����������µ�id��ţ�
    std::string name;//�������
    std::string memo;//��������
    int pre_task_type;//ǰ����������
    int pre_task_id;//ǰ������id
    int need_race;//������������(0��ʾ��������)
    int done_cost;//���Ľ�������Զ����
    int guide_id_get;//��ȡ�����Ӧ����
    int guide_id_reward;//��ȡ��������Ӧ����
    std::list<Item> reward;//����
    std::list<boost::shared_ptr<baseGoal> > goals;//����Ŀ��
    std::list<boost::shared_ptr<baseTask> > m_child_tasks;//��������
    void loadRewards();
    void loadGoals();
    void toObj(json_spirit::Object& obj);
    void toSimpleObj(json_spirit::Object& obj);
    void ChildList(json_spirit::Object& obj);
    void reload();
};

//����Ŀ��
struct CharTaskGoal
{
    int type;//Ŀ������
    int cur;//��ǰ����
    int extra;//ÿ�����������滻Ŀ��
    boost::shared_ptr<baseGoal> base_goal;
    bool done;
};

//�����������
struct CharTask
{
    CharTask(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    int tid;
    bool done;
    boost::shared_ptr<baseTask> m_task;
    std::list<boost::shared_ptr<CharTaskGoal> > m_goals;//����Ŀ��
    void Save();
    void toObj(bool simple, json_spirit::Object& obj);
};

struct CharAllTasks
{
    CharAllTasks(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    std::map<int, boost::shared_ptr<CharTask> > m_all_tasks;

    //��������
    void acceptTask(boost::shared_ptr<baseTask> t);
    //�������
    int taskDone(int id, int cost, json_spirit::Object& robj);
    //�����б�
    int getList(int type, json_spirit::Array& rlist);
    //��������
    int updateTask(int goal_type, int n1 = 0, int n2 = 0);
    boost::shared_ptr<CharTask> getCharTask(int id);
    //��ȡ���ĳ���͵�һ������״̬
    void getCharTaskState(int type, int& tid, int& state);
    bool taskExploreSpecial();
    void load();
    int getCharDailyTaskCnt();
    int getCharDailyState();
    void dailyUpdate();
    void getButton(json_spirit::Array& list);
};

class taskMgr
{
public:
    taskMgr();
    std::map<int, boost::shared_ptr<baseTask> >& getTasks() {return m_total_tasks;}
    std::vector<int>& getDailyTasks() {return m_daily_tasks;}
    boost::shared_ptr<baseTask> getTask(int tid);    //��������id�������
    boost::shared_ptr<baseTask> getTask(int type, int task_id); //�����������ͺͷ���id�������
    void addTask(int tid, boost::shared_ptr<baseTask> bt);
    void removeTask(int tid);
    void reLink();
    int newChar(boost::shared_ptr<CharData> cdata);
    int RandomDailyTask();
private:
    std::map<int, boost::shared_ptr<baseTask> > m_total_tasks;        //��������
    std::vector<int> m_daily_tasks;        //ÿ�ճ�ʼ�ճ�����
};

int ProcessTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessTaskInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessTaskDone(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

