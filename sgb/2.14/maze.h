#pragma once

#include <list>
#include "base_item.h"
#include <map>
#include "net.h"
#include "combat_attr.h"
#include "data.h"
#include "json_spirit.h"
#include "combat.h"
#include "spls_timer.h"

//�Թ��ܸ�����
const int iMazeSize = 9 * 9 + 3 + 3 + 1 + 1;
//�Թ����
const int iMazeStartPoint = 1;
//�Թ�����
const int iMazeOutPoint = iMazeSize;

//ÿ�ս����Թ�����
const int iMazeTimesEveryday = 2;

struct CharData;


enum maze_mora_result
{
    MAZE_MORA_WIN = 0,
    MAZE_MORA_LOSE = 1,
    MAZE_MORA_DRAW = 2
};

enum maze_event_type
{
    MAZE_EVENT_START,
    MAZE_EVENT_CHANGE_POSTION = 1,//��λ
    MAZE_EVENT_GUESS_NUMBER,        //2����
    MAZE_EVENT_REPEAT,            //3�ظ�
    MAZE_EVENT_WILD,                //4��
    MAZE_EVENT_WEAK,                //5����
    MAGE_EVENT_FULL,                //6�ָ�
    MAZE_EVENT_DISCOUNT,            //7˦��
    MAZE_EVENT_BOMB,                //8ը��
    MAZE_EVENT_LUCKY,                //9�ݼ�
    MAZE_EVENT_MISCHANCE,            //10���
    MAZE_EVENT_SCORE,                //11����
    MAZE_EVENT_MORA,                //12��ȭ
    MAZE_EVENT_GEM,                //13����
    MAZE_EVENT_MONSTER,            //14����
    MAZE_EVENT_OUT,                //15����    
    MAZE_EVENT_LONGSTOP,            //16�ϰ�
    MAZE_EVENT_LOSE_MOVE_TIMES,    //17�����ƶ�����2
};

enum maze_event_result_type
{
    MAZE_EVENT_RESULT_CHANGE_POSTION = 1,//��λ
    MAZE_EVENT_RESULT_GUESS_NUMBER_1,    //2���ݴ���<=6
    MAZE_EVENT_RESULT_GUESS_NUMBER_2,    //3���ݴ���<=8
    MAZE_EVENT_RESULT_GUESS_NUMBER_3,    //4���ݴ���<=10
    MAZE_EVENT_RESULT_GUESS_NUMBER_4,    //5���ݴ���<=15
    MAZE_EVENT_RESULT_GUESS_NUMBER_5,    //6���ݴ���>15
    MAZE_EVENT_RESULT_REPEAT,            //7�ظ�
    MAZE_EVENT_RESULT_WILD,                //8��
    MAZE_EVENT_RESULT_WEAK,                //9����
    MAZE_EVENT_RESULT_FULL,                //10�ָ�
    MAZE_EVENT_RESULT_BUY_DISCOUNT,        //11˦��-������
    MAZE_EVENT_RESULT_CANCEL_DISCOUNT,    //12˦��-������
    MAZE_EVENT_RESULT_BOMB,                //13ը��
    MAZE_EVENT_RESULT_LUCKY_MOVE_MORE,    //14�ݼ�-�ƶ���Χ+1
    MAZE_EVENT_RESULT_LUCKY_VIEW_MORE,    //15�ݼ�-��Ұ��Χ+1
    MAZE_EVENT_RESULT_LUCKY_ADD_MOVE,    //16�ݼ�-�ƶ�����+2
    MAZE_EVENT_RESULT_LONGSTOP,            //17���-�ϰ���
    MAZE_EVENT_RESULT_LOSE_MOVE_TIMES,    //18���-�ƶ�����-2
    MAZE_EVENT_RESULT_DOUBLE_SCORE,        //19���� ˫��
    MAZE_EVENT_RESULT_HALF_SCORE,        //20���� ����
    MAZE_EVENT_RESULT_MORA_WIN,            //21��ȭ Ӯ��
    MAZE_EVENT_RESULT_MORA_DRAW,            //22��ȭ ƽ��
    MAZE_EVENT_RESULT_MORA_LOSE,            //23��ȭ ����
    MAZE_EVENT_RESULT_GEM,                //24����
    MAZE_EVENT_RESULT_ELITE_MONSTER,        //25����-��Ӣ
    MAZE_EVENT_RESULT_NORMAL_MONSTER,    //26����-��ͨ
    MAZE_EVENT_RESULT_OUT                    //27����    
};

struct base_maze_event_result
{
    int result;
    int event_type;
    int score;
    int gailv;
    int param_count;

    std::string msg;
    std::string org_msg;
};

struct base_maze_event
{
    int type;
    std::string name;
    std::string memo;
    std::string score;

    std::vector<boost::shared_ptr<base_maze_event_result> > m_results;
    std::vector<int> m_gailvs;

    boost::shared_ptr<base_maze_event_result> random_result();
};

struct char_maze_point
{
    int id;            //id
    int type;        //�Թ��¼�����
    int result;        //�¼��������

    char_maze_point();
};

//�Թ������buff����
const int iMaxCharMazeBuff = 4;

struct char_maze_buff
{
    int type;                //buff���
    time_t remove_time;    //����ʱ
};

struct base_maze_map_data
{
    int star;    //�Ѷ�
    int tjAttack;
    int tjLevel;
    std::string gailv;
    std::string memo;
    //std::list<Item> loots;
    std::vector<int> boss_id;
    int normal_mid;
    int elite_mid;
};

struct base_maze
{
    int id;
    int openLevel;
    std::string name;
    std::string memo;
    base_maze_map_data stars[3];

    std::list<Item> loots;
};

struct char_maze_boss
{
    int id;
    int state;
};

//�Թ��е��佫����
struct char_maze_general
{
    int pos;
    int id;
    int gid;
    int cid;
    int spic;
    int level;
    int color;
    int b_nickname;
    std::string name;

    //���䡢������ͳ��
    int m_str;
    int m_int;
    int m_tongyu;

    int m_org_attack;
    int m_org_wu_fang;
    int m_org_ce_fang;

    //����
    int m_attack;
    int m_wu_fang;
    int m_ce_fang;

    //ԭʼ
    int m_org_hp_max;    //ԭʼ��Ѫ��
    int m_org_hp_hurt;//ԭʼ����Ѫ��

    combatAttribute m_combat_attribute; //ս������

    int m_inspired;
    
    //��ǰѪ��
    int m_hp_org;    //ԭʼѪ��
    int m_hp_hurt;    //����Ѫ��

    void Save();

    void load();
};

struct char_maze
{
    int m_cid;                //��ɫid
    int m_move_range;        //�ƶ���Χ
    int m_view_range;        //��Ұ��Χ
    int m_double_score;    //�´��Ƿ�˫������
    time_t m_timeout;        //��ʱʱ��
    int m_cur_pos;            //��ǰλ��
    int m_cur_pos_score;    //��ǰλ�û�û���
    int m_cur_pos_pending;//��ǰλ�ô�����
    int m_last_pos;        //�ϴ�λ��
    int m_last_event;        //�ϴδ����¼�����
    int m_left_move_count;    //ʣ���ж�����
    int m_score;                //��ǰ����
    int m_used_times[4];        //4�����ʹ�ô�����1�ָ���2������3��� 4����

    int extra[2];    //�������ݱ���
    
    std::vector<Item> m_lottery_list;    //�齱��Ʒ�б�

    int m_star;                    //�Թ��Ѷ�
    boost::shared_ptr<base_maze> m_curMaze;    //��ǰ�Թ�

    int m_state;                //״̬��0���ؽ׶�  1��boss�׶�
    uint64_t m_boss_combat_id;        //���ڻ�ɱbossս����
    std::list<char_maze_boss> m_bossList;    //���Ի�ɱ��boss�б�

    std::vector<int> m_map_types;        //�Թ���ͼ����
    std::vector<int> m_map_results;    //�Թ���ͼ����

    //char_maze_point m_points[iMazeSize];//�Թ���ͼ����

    std::vector<char_maze_buff> m_buffs;    //buff�б�

    std::list<char_maze_general> m_generals;    //����������佫

    boost::uuids::uuid m_timer;

    class mazeMgr& m_handle;

    char_maze(int id, mazeMgr& m);

    //����ȫ��
    void init();
    void reset_generals(int cid);
    //����ʣ��
    int reset_left(CharData& cdata, json_spirit::Object& robj);
    //�ָ�����Ѫ��
    int mazeFull(CharData& cdata, json_spirit::Object& robj);
    //�Թ�����
    int mazeSkip(CharData& cdata, int id, json_spirit::Object& robj);
    //�Թ����
    int mazeChange(CharData& cdata, int id, json_spirit::Object& robj);
    //�ƶ�
    int mazeMove(CharData& cdata, int id, json_spirit::Object& robj);

    //����
    int mazeTrigger(CharData& cdata, int type, json_spirit::Array& list);

    void add_event_score(boost::shared_ptr<base_maze_event_result> result, json_spirit::Object& obj);

    void add_buff(int type, int last_secs);

    int _mazeFull();

    void combatEnd(Combat* pCombat);

    void notifyFail();

    void updateBuffEffect();

    int getBuffeffect();

    void Save();

    void Clear();

    void SaveGenerals();

    void SaveBoss(int seq);

    void SaveMap();

    void SaveMapState();

    void load();
};

//�Թ���������
struct mazeMonster
{
    int _id;    //����id
    int _level;        //�ȼ�
    int _spic;        //ͼƬ
    int _mod;        //ģ��id
    std::string _name;    //����
    std::list<Item> m_Item_list;

    //�ؿ��Ŀ���
    combatAttribute m_combat_attribute;

    boost::shared_ptr<StrongholdGeneralData> m_generals[9];//�����佫
    int load();
};

class mazeMgr
{
public:
    mazeMgr();
    void load();
    boost::shared_ptr<char_maze> getChar(int cid);
    int getMazeList(CharData& cdata, json_spirit::Object& robj);
    int getMazeDetail(int id, json_spirit::Object& robj);
    boost::shared_ptr<mazeMonster> getMonsterById(int mid);
    int enterMaze(CharData& cdata, int id, int star, json_spirit::Object& robj);
    int queryMazeMap(CharData& cdata, json_spirit::Object& robj);
    int queryMazeEventTips(int type, json_spirit::Object& robj);
    int queryCurMaze(CharData& cdata, json_spirit::Object& robj);
    int queryCurMazeInfo(CharData& cdata, json_spirit::Object& robj);
    int queryMazeTeam(CharData& cdata, json_spirit::Object& robj);
    int mazeMove(CharData& cdata, int toid, json_spirit::Object& robj);
    int mazeSkip(CharData& cdata, int id, json_spirit::Object& robj);
    int mazeChange(CharData& cdata, int id, json_spirit::Object& robj);
    int mazeFull(CharData& cdata, json_spirit::Object& robj);
    int mazeReset(CharData& cdata, json_spirit::Object& robj);
    int getMazeBossList(CharData& cdata, json_spirit::Object& robj);
    int mazeKillBoss(CharData& cdata, json_spirit::Object& robj);
    int mazeMora(CharData& cdata, int type, json_spirit::Object& robj);
    int mazeGuessNumber(CharData& cdata, int useGold, int number, json_spirit::Object& robj);
    int mazeBuy(CharData& cdata, int tid, int count, json_spirit::Object& robj);
    int mazeLottery(CharData& cdata, json_spirit::Object& robj);
    int mazeAbandon(CharData& cdata, json_spirit::Object& robj);
    int mazeQueryBossLoots(CharData& cdata, int id, json_spirit::Object& robj);
    int mazeKill(CharData& cdata, int useGold, json_spirit::Object& robj);

    boost::shared_ptr<base_maze_event_result> getEventResult(int result);
    boost::shared_ptr<base_maze_event_result> randomEventResult(int event);
    void random_maze(std::vector<int>& p);
    void random_lottery_items(std::vector<Item>& list, int count = 3);
    std::map<int, int>& get_discount_items();
    int get_item_discount(int tid);
    int combatResult(Combat* pCombat);

    void getAction(CharData* pc, json_spirit::Array& blist);

    boost::shared_ptr<base_maze> getBaseMaze(int id);

private:
    std::map<int, boost::shared_ptr<char_maze> > m_char_datas;
    std::vector<boost::shared_ptr<base_maze> > m_mazes;
    std::vector<int> m_maze_template;

    //��������¼����ֵ��ܴ���
    int m_maze_event_times[MAZE_EVENT_OUT-1];

    //�����¼�����
    std::vector<boost::shared_ptr<base_maze_event> > m_events;

    //���ֽ������
    std::vector<boost::shared_ptr<base_maze_event_result> > m_results;

    //�齱��Ʒ�б�
    std::vector<Item> m_lottery_items;
    //�ۿ���Ʒ�б�
    std::map<int, int> m_discount_items;

    //���ֹ�������
    std::map<int, boost::shared_ptr<mazeMonster> > m_mazeMonsters;

};

//��ѯ�Թ��б� cmd ��queryMazeList
int ProcessQueryMazeList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ��Ѷ���Ϣ cmd��queryMazeDetail, id:�Թ�id
int ProcessQueryMazeDetail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����Թ� cmd��enterMaze��id���Թ�id��star���Ǽ�
int ProcessEnterMaze(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ��ǰ�Թ� cmd��queryCurMaze
int ProcessQueryCurMaze(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�Թ���ͼ��Ϣ cmd��queryMazeMap
int ProcessQueryMazeMap(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�Թ��¼���Ϣ cmd��queryMazeEventTips,type:���
int ProcessQueryMazeEventTips(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�Թ�״̬ cmd��queryCurMazeInfo
int ProcessQueryCurMazeInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�Թ�������� cmd��queryMazeTeam
int ProcessQueryMazeTeam(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ��ƶ� cmd��mazeMove��id��Ŀ��λ��id
int ProcessMazeMove(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ�С��Ϸ��� cmd��mazeGameScore��score������
int ProcessMazeGameScore(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ��ָ� cmd��mazeFull
int ProcessMazeFull(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ����� cmd: mazeSkip, id:Ŀ��id
int ProcessMazeSkip(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ���� cmd��mazeChange, id:Ŀ��id
int ProcessMazeChange(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ����� cmd��mazeReset
int ProcessMazeReset(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ�boss�б� cmd��mazeBossList
int ProcessMazeBossList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����Թ�boss cmd��mazeKillBoss
int ProcessMazeKillBoss(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ������� cmd��mazeGuessNumber
int ProcessMazeGuessNumber(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ���ȭ cmd��mazeMora
int ProcessMazeMora(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ�����˦����Ʒ cmd��mazeBuy
int ProcessMazeBuy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ��齱 cmd��mazeLottery
int ProcessMazeLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ���ѯ�齱��Ʒ
int ProcessQueryMazeLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ���ѯ�����ִ���
int ProcessQueryMazeGuessTimes(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ���ѯ˦����Ʒ��Ϣ
int ProcessQueryMazeCanBuy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ�����
int ProcessMazeAbandon(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ���ѯ��ǰ�¼����
int ProcessMazeQueryCurResult(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ���ѯBOSS����
int ProcessMazeQueryBossLoots(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����Թ��� cmd��mazeKill
int ProcessMazeKill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Թ���ʱ��
int ProcessMazeTimeout(json_spirit::mObject& o);

