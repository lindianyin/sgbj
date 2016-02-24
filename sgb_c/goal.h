#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "item.h"

struct CharData;

enum goal_type
{
    GOAL_TYPE_EMPTY = 0,        //�հ�����
    GOAL_TYPE_CHAR_LEVEL,       //1��Ҵﵽָ���ȼ�X
    GOAL_TYPE_EQUIPT1_QUALITY,  //2װ��Ʒ��X������
    GOAL_TYPE_EQUIPT2_QUALITY,  //3װ��Ʒ��X�Ķ���
    GOAL_TYPE_EQUIPT3_QUALITY,  //4װ��Ʒ��X���·�
    GOAL_TYPE_EQUIPT4_QUALITY,  //5װ��Ʒ��X����
    GOAL_TYPE_METALLURGY_LEVEL, //6���𷿵ȼ�X
    GOAL_TYPE_SIGN,             //7ǩ��X����
    GOAL_TYPE_RECRUIT,          //8��ļX����
    GOAL_TYPE_UPGRADE_EQUIPT,   //9ǿ��װ��X����
    GOAL_TYPE_LEVY,             //10����X����
    GOAL_TYPE_EXPLORE,          //11�һ�̽����ѨX����
    GOAL_TYPE_SHENLING_TOWER,   //12������ָ������X
    GOAL_TYPE_COMPOUND_HERO,    //13�ϳ�Ӣ��X����
    GOAL_TYPE_ARENA_WIN,        //14������ʤ��X����
    GOAL_TYPE_VIP,              //15VIPX
    GOAL_TYPE_HERO_PACK,        //16Ʒ��XӢ�ۿ���
    GOAL_TYPE_COPY_MAP,         //17ͨ�ظ�����ͼX
    GOAL_TYPE_WILD_WIN,         //18��ռ�ǳ�X����
    GOAL_TYPE_HERO_CNT,         //19Ӣ������X��
    GOAL_TYPE_TREASURE_QUALITY, //20�ر�ͼXƷ��
    GOAL_TYPE_CASTLE_LEVEL,     //21�Ǳ�ָ���ȼ�X
    GOAL_TYPE_HERO_STAR,        //22Ӣ���Ǽ�X
    GOAL_TYPE_SILVER,           //23��������X
    GOAL_TYPE_DECOMPOSE_HERO,   //24�ֽ�Ӣ��X����
    GOAL_TYPE_SMITHY_LEVEL,     //25�����̵ȼ�X
    GOAL_TYPE_EPIC_HERO,        //26������X
    GOAL_TYPE_RESIDENT,         //27��������X
    GOAL_TYPE_EQUIPT_QUALITY,   //28װ��Ʒ��X������װ��
    GOAL_TYPE_EQUIPT1_UPGRADE,  //29�����������ȼ�X
    GOAL_TYPE_EQUIPT2_UPGRADE,  //30�������Ƶ��ȼ�X
    GOAL_TYPE_EQUIPT3_UPGRADE,  //31�����·����ȼ�X
    GOAL_TYPE_EQUIPT4_UPGRADE,  //32�����鵽�ȼ�X
};

struct baseGoalTask
{
    baseGoalTask()
    {
        id = 0;
        type = 0;
        need_extra = 0;
        need_level = 0;
        need_vip = 0;
        name = "a task";
        goal = "a goal";
        memo = "a task memo";
    }
    int id;
    int type;
    int spic;
    int need_extra;
    int need_level;//��������ȼ�
    int need_vip;//���⽱������vip
    std::string name;//�������
    std::string goal;//����Ŀ��
    std::string memo;//��������
    std::list<Item> reward;//����
    std::list<Item> vip_reward;//����
    void loadRewards();
    void toObj(json_spirit::Object& obj);
    void toSimpleObj(json_spirit::Object& obj);
};

//����Ŀ��
struct baseGoalLevelReward
{
    int id;
    int need_level;
    std::string name;//�������
    std::string memo;//��������
    std::list<Item> reward;//����
    void loadRewards();
    void toObj(json_spirit::Object& obj);
    void toSimpleObj(json_spirit::Object& obj);
};

//Ŀ����Ʒ
struct baseGoalGoods
{
    int id;
    int need_level;
    int org_cost;
    int cost;
    Item reward;
};

//���Ŀ������
struct CharGoalTask
{
    enum reward_state
    {
        STATE_INIT = 0,
        STATE_GET = 1,//������ȡ
        STATE_ALREADY = 2,//�Ѿ���ȡ
    };
    int cid;
    int tid;
    int cur;
    int normal_state;
    int vip_state;
    bool done;
    boost::shared_ptr<baseGoalTask> m_base;
    void Save();
    void toObj(json_spirit::Object& obj);
    void toSimpleObj(json_spirit::Object& obj);
};

struct CharGoal
{
    int cid;
    std::map<int, boost::shared_ptr<CharGoalTask> > m_all_tasks;
    
    void acceptTask(boost::shared_ptr<baseGoalTask> t);
    //�����б�
    int getList(int level, json_spirit::Array& rlist);
    //��������
    int updateTask(int goal_type, int extra);
    //����vip
    int updateVip(int vip);
    //���µȼ�
    int updateLevel(int level);
    //��ȡĿ�꽱��
    int getReward(int id, int type, json_spirit::Object& robj);
    //��ȡĿ�꽱��
    int getLevelReward(int level, json_spirit::Object& robj);
    boost::shared_ptr<CharGoalTask> getCharGoalTask(int tid);
    void load();
    int getCharGoalState(int& type, int& need_level, int& id);
    void getButton(json_spirit::Array& list);
    void getCharGoalTag();
};

class goalMgr
{
public:
    goalMgr();
    boost::shared_ptr<CharGoal> getCharGoal(int cid);
    boost::shared_ptr<baseGoalTask> getGoalTask(int tid);    //��������id�������
    std::map<int, boost::shared_ptr<baseGoalTask> >& getGoalTasks() {return m_total_tasks;}
    boost::shared_ptr<baseGoalLevelReward> getLevelReward(int id);
    std::map<int, boost::shared_ptr<baseGoalLevelReward> >& getLevelRewards() {return m_level_rewards;}
    void getGoalGood(int level, json_spirit::Array& list);
    boost::shared_ptr<baseGoalGoods> getBaseGood(int id);
    int updateTask(int cid, int goal_type, int extra);
    int updateVip(int cid, int vip);
    int updateLevel(int cid, int level);
private:
    std::map<int, boost::shared_ptr<baseGoalTask> > m_total_tasks;//��������
    std::map<int, boost::shared_ptr<baseGoalLevelReward> > m_level_rewards;//�ȼ�����
    std::map<int, boost::shared_ptr<baseGoalGoods> > m_goods;
    std::map<int, boost::shared_ptr<CharGoal> > m_char_goals;
};

//��ȡĿ�������б�
int ProcessGoalTaskList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//Ŀ��ȼ�����
int ProcessGoalLevelList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡĿ�꽱��
int ProcessGoalReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//Ŀ����Ʒ
int ProcessGoalShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����Ŀ����Ʒ
int ProcessBuyGoalGood(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

