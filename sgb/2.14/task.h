#pragma once

#include "base_item.h"
#include <list>
#include <vector>

#include <string>
#include <map>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include "spls_const.h"

using namespace json_spirit;

struct StrongholdData;

class CharData;

//��������,1���ؿ� 2�������ȼ� 3������佫 4���佫�ȼ� 5�����볡�� 6�������ͼ 7�����ܵȼ� 8�����͵ȼ�
//9��������� 10��ˢ�±��� 11��������� 12��������� 13���ղ���Ϸ 
//14��̽������ 15��װ���ȼ� 16�������о� 17��������Ӫ 18������ 19��״̬ˢ�� 20���������� 21�������ȼ�
enum task_type
{
    task_empty = 0,        //�հ�����
    task_attack_stronghold = 1,//����ָ���ؿ�
    task_char_level,        //2�����ﵽָ���ȼ�
    task_get_general,        //3���ָ���佫
    task_general_level,    //4�佫������һ���ȼ�
    task_enter_stage,        //5����ָ������
    task_enter_map,        //6����ָ����ͼ
    task_skill_level,        //7����������һ���ȼ�
    task_zhen_level,        //8����������ָ���ȼ�
    task_buy_weapon,        //9 ����ָ������
    task_refresh_weapon,    //10ˢ��������
    task_send_general,    //11�ű�������ָ���佫����
    task_first_farm,        //12�״ν����������
    task_add_to_favorites,//13�ղ���Ϸ
    task_do_explore,        //14̽������
    task_equipment_level,//15װ���ȼ�
    task_do_research,        //16�о�����
    task_choose_camp,        //17������Ӫ
    task_do_race,            //18����ս��
    task_refresh_state,    //19ˢ��״̬
    task_silver_nums,        //20��������
    task_attack_equipment_level,    //21�����ȼ�
    task_get_treasure,    //22��õ���
    task_group_general_level,    //23����佫�ﵽָ���ȼ�
    task_first_smelt,        //24�״�ұ������
    task_weapon_level,    //25�����ȼ�
    
/****************** ֧������ ***************************/
    
    task_rob_stronghold,//26�Ӷ�ؿ�
    task_get_gem,        //27��õ���
    task_elite_combat,  //28��Ӣս��ͨ��
    task_equipment_make,//29����װ��
    task_buy_bag,        //30����һ������λ��
    task_gather_gem,    //31��������
    task_join_corps,    //32�������
    task_corps_jisi,    //33���ż���
    task_corps_ymsj,    //34����ԯ�����
    task_corps_explore, //35����̽��
    task_daily_score,   //36ÿ�ջ�Ծ��

    task_open_libao,    //37�����

    task_levy,          //���� 38 ����
    task_add_friends,   //���� 39 ����
    task_train,         //ѵ�� 40 ����
    task_normal_wash,   //��ͨϴ�� 41 ����
    task_2gold_wash,    //��ͭϴ�� 42 ����
    task_wash_star,     //ϴ���Ǽ� 43 
    task_horse_train,   //ս������ 44 ����
    task_arena_win,     //������ʤ�� ���� 45
    task_arena_liansheng,//��������ʤ 46
    task_farm_harvest,  //�����ջ� 47 ����
    task_farm_water,    //���ｽ�� ���� 48
    task_farm_yechan,   //����Ұ�� ���� 49
    task_baoshi_exchange,   //��ʯ�һ� 50
    task_baoshi_combine,    //��ʯ�ϳ� 51
    task_baoshi_convert,    //��ʯת�� 52
    task_baoshi_combine_level,//��ʯ�ϳ� 53

    task_arrest_servant,   //ץ��׳��  ���� 54
    task_rescue_servant,   //���׳�� ���� 55
    task_reborn,            //���� ���� 56
    task_reborn_star,      //�����Ǽ� 57

    task_trade_star,    //ó���Ǽ� 58
    task_trade_wjbs,    //ó��ʹ�����̲��� 59
    task_upgrade_soul,  //�����ݱ� 60
    task_center_soul_level, //�ݱ����۵ȼ� 61

    task_maze_score,    //��������� 62

    task_general_inherit,   //�佫���� ���� 63
    task_shop_buy_mat,      //�̵깺����� 64
    task_shop_buy_baoshi,   //�̵깺��ʯ 65
};

enum guide_type_enum
{
    guide_type_login,
    guide_type_gettask,
    guide_type_taskdone,
    guide_type_char_level,
    guide_type_stronghold,
    guide_type_frist_farm,
    guide_type_enter_map,
    guide_type_view_map,
    guide_type_no_ling,
    guide_type_choose_camp,
    guide_type_get_stage_reward,
    guide_type_no_supply
};

enum guide_id_enum
{
    guide_id_enhance = 101,    //ǿ������ ������һ�����򡢵�һ���������߸��ؿ�
    guide_id_upgrade_weapon = 102,    //������������ ��һ�����򡢵�һ��������ȡͨ�ؽ�����
    guide_id_recruit = 103,    //��ļ���� ������һ�����򡢵ڶ��������ڶ����ؿ�
    guide_id_equipment = 104,    //װ������ ������һ�����򡢵ڶ����������ĸ��ؿ�
    guide_id_levy = 105,        //�������� ������һ�����򡢵ڶ��������������ؿ�
    guide_id_next_stage = 106,//��һ�����򡢵ڶ���������ȡͨ�ؽ�����
    guide_id_newbie_present = 107,//������ȡ������� ������һ�����򡢵����������ڶ����ؿ�
    guide_id_train = 108,    //����ѵ�� ������һ�����򡢵������������ĸ��ؿ�
    guide_id_wash = 109,    //ϴ������ ������һ�����򡢵����������������ؿ�
    guide_id_next_map = 110,//����������һ����  ��һ�����򡢵�����������ȡͨ�ؽ�����
    guide_id_race = 111,    //���������� �����ڶ������򡢵�һ������������ֺ󣬾������в�ֹ���һ����
    guide_id_up_offical = 112,//�������� ����������Ϣʱ����һ�ο�������
    guide_id_get_salary = 113,//������ٺ» ����������Ϣʱ����һ�ο�����ٺ»
    guide_id_horse = 114,    //����ս������ �����ڶ������򡢵�һ�������������ֺ�
    guide_id_get_daily = 115,//������ȡ�ճ������ ������Ϸ������Ϣ����һ�ο���ȡ�ճ������
    guide_id_shop = 116,    //����������Ʒ �����ڶ������򡢵ڶ��������������ֺ�
    guide_id_117 = 117,    //������ʾ �����ڶ������򡢵����������������ֺ�
    guide_id_118 = 118,    //������ʾ �����ڶ������򡢵������������߸��ֺ�
    guide_id_sweep = 119,    //ɨ������ ��һ�γ��־�������ʱ
    guide_id_120 = 120,    //������ʾ �������������򡢵ڶ��������ڶ����ֺ�
    guide_id_121 = 121,    //������ʾ �������������򡢵ڶ��������������ֺ�

    guide_id_trade = 122,    //ó�׿���
    guide_id_reborn = 123,//��������
    guide_id_xiangqian = 124,    //��Ƕ����
    guide_id_servant = 125,//׳������
    guide_id_guard = 126,    //���Ϳ���
};

enum task_id_enum
{
    task_id_dongcao = 3,    //��������
    task_id_xueba = 5,        //Ѧ������
    task_id_farm = 11        //��������
};

struct baseTask
{
    baseTask();
    ~baseTask();
    int task_type;        //0��������  1֧������
    int need_task;        //֧��������Ч ֧������ǰ�õ���������

    int id;        //����id
    int type;            //�����б�
    int need[4];
    int done_level;        //�����ﵽ�õȼ��������Զ����

    boost::shared_ptr<StrongholdData> target_stronghold;    //Ŀ��ؿ�
    int mapid;
    int stageid;
    int sweep;

    std::string title;    //�������
    std::string memo;    //��������

    std::list<Item> reward;        //����

    json_spirit::Object detail_obj;    //��������obj
    json_spirit::Object simple_obj;    //����Ϣ��obj

    std::list<boost::shared_ptr<const struct baseTask> > m_trunk_tasks;
    void loadRewards();
};

struct charTask
{
    charTask(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    boost::shared_ptr<const baseTask> _task;
    int tid;
    int cur;
    int need;
    bool done;
    void Save();
};

struct charTrunkTasks
{
    charTrunkTasks(CharData& c)
    :m_charData(c)
    {
    }
    CharData& m_charData;
    std::map<int, boost::shared_ptr<charTask> > m_trunk_tasks;

    //��������
    void acceptTask(boost::shared_ptr<const baseTask> t);
    //�������
    int taskDone(int id, json_spirit::Object& robj);
    //�����б�
    int getList(json_spirit::Array& rlist);
    //��������
    int updateTask(int type, int n1 = 0, int n2 = 0);
    bool getFinishState();
};

class taskMgr
{
public:
    taskMgr();
    boost::shared_ptr<const baseTask> getTask(int tid);    //��������id�������
    boost::shared_ptr<const baseTask> getNext(boost::shared_ptr<const baseTask> task);    //��һ������

    boost::shared_ptr<const baseTask> getTrunkTask(int tid); //��������id�������

    int queryCurTask(boost::shared_ptr<CharData> cdata);    //��ý�ɫ��ǰ����
    int newChar(boost::shared_ptr<CharData> cdata);
    int taskDone(boost::shared_ptr<CharData> cdata, int id, json_spirit::Object& robj);        //��ɫ�������
    //��������
    int getTaskInfo(CharData& cData, int tid, json_spirit::Object& robj);
    //�����б�
    int getTaskList(CharData& cData, int page, int pageNums, json_spirit::Object& robj);
    //ɾ����ɫ
    int deleteChar(int cid);
    //��ɫͨ�س�����ȡ���֧��
    int acceptTrunkTask(CharData& cData, int strongholdid);
    //��ɫ���֧����������ȡ�������
    int acceptTrunkTask2(CharData& cData, int tid);
    
    static taskMgr* getInstance();
    
private:
    static taskMgr* m_handle;
    std::vector<boost::shared_ptr<const baseTask> > m_total_tasks;        //��������
    std::vector<boost::shared_ptr<const baseTask> > m_trunk_tasks;        //����֧������
    std::map<int, std::vector<boost::shared_ptr<const baseTask> > > m_get_trunk_tasks;        //֧�������Ӧ�Ľ�ȡ�ؿ�
    std::map<int, std::vector<boost::shared_ptr<const baseTask> > > m_get_trunk_tasks2;       //֧��������ɿ��ŵ�֧������
    std::map<int, boost::shared_ptr<const baseTask> > m_char_tasks;    //��ɫ�����¼
};

