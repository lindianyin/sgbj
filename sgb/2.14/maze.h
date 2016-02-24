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

//迷宫总格子数
const int iMazeSize = 9 * 9 + 3 + 3 + 1 + 1;
//迷宫入口
const int iMazeStartPoint = 1;
//迷宫出口
const int iMazeOutPoint = iMazeSize;

//每日进入迷宫次数
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
    MAZE_EVENT_CHANGE_POSTION = 1,//换位
    MAZE_EVENT_GUESS_NUMBER,        //2推演
    MAZE_EVENT_REPEAT,            //3重复
    MAZE_EVENT_WILD,                //4狂暴
    MAZE_EVENT_WEAK,                //5虚弱
    MAGE_EVENT_FULL,                //6恢复
    MAZE_EVENT_DISCOUNT,            //7甩卖
    MAZE_EVENT_BOMB,                //8炸弹
    MAZE_EVENT_LUCKY,                //9遁甲
    MAZE_EVENT_MISCHANCE,            //10横祸
    MAZE_EVENT_SCORE,                //11积分
    MAZE_EVENT_MORA,                //12猜拳
    MAZE_EVENT_GEM,                //13宝物
    MAZE_EVENT_MONSTER,            //14怪物
    MAZE_EVENT_OUT,                //15出口    
    MAZE_EVENT_LONGSTOP,            //16障碍
    MAZE_EVENT_LOSE_MOVE_TIMES,    //17减少移动次数2
};

enum maze_event_result_type
{
    MAZE_EVENT_RESULT_CHANGE_POSTION = 1,//换位
    MAZE_EVENT_RESULT_GUESS_NUMBER_1,    //2推演次数<=6
    MAZE_EVENT_RESULT_GUESS_NUMBER_2,    //3推演次数<=8
    MAZE_EVENT_RESULT_GUESS_NUMBER_3,    //4推演次数<=10
    MAZE_EVENT_RESULT_GUESS_NUMBER_4,    //5推演次数<=15
    MAZE_EVENT_RESULT_GUESS_NUMBER_5,    //6推演次数>15
    MAZE_EVENT_RESULT_REPEAT,            //7重复
    MAZE_EVENT_RESULT_WILD,                //8狂暴
    MAZE_EVENT_RESULT_WEAK,                //9虚弱
    MAZE_EVENT_RESULT_FULL,                //10恢复
    MAZE_EVENT_RESULT_BUY_DISCOUNT,        //11甩卖-购买了
    MAZE_EVENT_RESULT_CANCEL_DISCOUNT,    //12甩卖-放弃了
    MAZE_EVENT_RESULT_BOMB,                //13炸弹
    MAZE_EVENT_RESULT_LUCKY_MOVE_MORE,    //14遁甲-移动范围+1
    MAZE_EVENT_RESULT_LUCKY_VIEW_MORE,    //15遁甲-视野范围+1
    MAZE_EVENT_RESULT_LUCKY_ADD_MOVE,    //16遁甲-移动次数+2
    MAZE_EVENT_RESULT_LONGSTOP,            //17横祸-障碍物
    MAZE_EVENT_RESULT_LOSE_MOVE_TIMES,    //18横祸-移动次数-2
    MAZE_EVENT_RESULT_DOUBLE_SCORE,        //19积分 双倍
    MAZE_EVENT_RESULT_HALF_SCORE,        //20积分 减半
    MAZE_EVENT_RESULT_MORA_WIN,            //21猜拳 赢了
    MAZE_EVENT_RESULT_MORA_DRAW,            //22猜拳 平局
    MAZE_EVENT_RESULT_MORA_LOSE,            //23猜拳 输了
    MAZE_EVENT_RESULT_GEM,                //24宝物
    MAZE_EVENT_RESULT_ELITE_MONSTER,        //25怪物-精英
    MAZE_EVENT_RESULT_NORMAL_MONSTER,    //26怪物-普通
    MAZE_EVENT_RESULT_OUT                    //27出口    
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
    int type;        //迷宫事件类型
    int result;        //事件触发结果

    char_maze_point();
};

//迷宫内最大buff数量
const int iMaxCharMazeBuff = 4;

struct char_maze_buff
{
    int type;                //buff类别
    time_t remove_time;    //倒计时
};

struct base_maze_map_data
{
    int star;    //难度
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

//迷宫中的武将数据
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

    //勇武、智力、统御
    int m_str;
    int m_int;
    int m_tongyu;

    int m_org_attack;
    int m_org_wu_fang;
    int m_org_ce_fang;

    //攻防
    int m_attack;
    int m_wu_fang;
    int m_ce_fang;

    //原始
    int m_org_hp_max;    //原始满血量
    int m_org_hp_hurt;//原始受伤血量

    combatAttribute m_combat_attribute; //战斗属性

    int m_inspired;
    
    //当前血量
    int m_hp_org;    //原始血量
    int m_hp_hurt;    //受伤血量

    void Save();

    void load();
};

struct char_maze
{
    int m_cid;                //角色id
    int m_move_range;        //移动范围
    int m_view_range;        //视野范围
    int m_double_score;    //下次是否双倍积分
    time_t m_timeout;        //超时时间
    int m_cur_pos;            //当前位置
    int m_cur_pos_score;    //当前位置获得积分
    int m_cur_pos_pending;//当前位置待处理
    int m_last_pos;        //上次位置
    int m_last_event;        //上次触发事件类型
    int m_left_move_count;    //剩余行动次数
    int m_score;                //当前积分
    int m_used_times[4];        //4类操作使用次数，1恢复，2跳过，3变更 4重置

    int extra[2];    //额外数据保存
    
    std::vector<Item> m_lottery_list;    //抽奖物品列表

    int m_star;                    //迷宫难度
    boost::shared_ptr<base_maze> m_curMaze;    //当前迷宫

    int m_state;                //状态，0闯关阶段  1打boss阶段
    uint64_t m_boss_combat_id;        //正在击杀boss战斗中
    std::list<char_maze_boss> m_bossList;    //可以击杀的boss列表

    std::vector<int> m_map_types;        //迷宫地图数据
    std::vector<int> m_map_results;    //迷宫地图数据

    //char_maze_point m_points[iMazeSize];//迷宫地图数据

    std::vector<char_maze_buff> m_buffs;    //buff列表

    std::list<char_maze_general> m_generals;    //队伍里面的武将

    boost::uuids::uuid m_timer;

    class mazeMgr& m_handle;

    char_maze(int id, mazeMgr& m);

    //重置全部
    void init();
    void reset_generals(int cid);
    //重置剩余
    int reset_left(CharData& cdata, json_spirit::Object& robj);
    //恢复队伍血量
    int mazeFull(CharData& cdata, json_spirit::Object& robj);
    //迷宫跳过
    int mazeSkip(CharData& cdata, int id, json_spirit::Object& robj);
    //迷宫变更
    int mazeChange(CharData& cdata, int id, json_spirit::Object& robj);
    //移动
    int mazeMove(CharData& cdata, int id, json_spirit::Object& robj);

    //触发
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

//迷宫怪物数据
struct mazeMonster
{
    int _id;    //怪物id
    int _level;        //等级
    int _spic;        //图片
    int _mod;        //模型id
    std::string _name;    //名字
    std::list<Item> m_Item_list;

    //关卡的抗性
    combatAttribute m_combat_attribute;

    boost::shared_ptr<StrongholdGeneralData> m_generals[9];//防守武将
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

    //各种随机事件出现的总次数
    int m_maze_event_times[MAZE_EVENT_OUT-1];

    //各种事件类型
    std::vector<boost::shared_ptr<base_maze_event> > m_events;

    //各种结果类型
    std::vector<boost::shared_ptr<base_maze_event_result> > m_results;

    //抽奖物品列表
    std::vector<Item> m_lottery_items;
    //折扣物品列表
    std::map<int, int> m_discount_items;

    //各种怪物数据
    std::map<int, boost::shared_ptr<mazeMonster> > m_mazeMonsters;

};

//查询迷宫列表 cmd ：queryMazeList
int ProcessQueryMazeList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫难度信息 cmd：queryMazeDetail, id:迷宫id
int ProcessQueryMazeDetail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//进入迷宫 cmd：enterMaze，id：迷宫id，star：星级
int ProcessEnterMaze(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询当前迷宫 cmd：queryCurMaze
int ProcessQueryCurMaze(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询迷宫地图信息 cmd：queryMazeMap
int ProcessQueryMazeMap(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询迷宫事件信息 cmd：queryMazeEventTips,type:类别
int ProcessQueryMazeEventTips(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询迷宫状态 cmd：queryCurMazeInfo
int ProcessQueryCurMazeInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询迷宫队伍情况 cmd：queryMazeTeam
int ProcessQueryMazeTeam(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫移动 cmd：mazeMove，id：目标位置id
int ProcessMazeMove(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫小游戏结果 cmd：mazeGameScore，score：积分
int ProcessMazeGameScore(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫恢复 cmd：mazeFull
int ProcessMazeFull(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫跳过 cmd: mazeSkip, id:目标id
int ProcessMazeSkip(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫变更 cmd：mazeChange, id:目标id
int ProcessMazeChange(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫重置 cmd：mazeReset
int ProcessMazeReset(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫boss列表 cmd：mazeBossList
int ProcessMazeBossList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//攻击迷宫boss cmd：mazeKillBoss
int ProcessMazeKillBoss(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫猜数字 cmd：mazeGuessNumber
int ProcessMazeGuessNumber(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫猜拳 cmd：mazeMora
int ProcessMazeMora(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫购买甩卖商品 cmd：mazeBuy
int ProcessMazeBuy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫抽奖 cmd：mazeLottery
int ProcessMazeLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫查询抽奖物品
int ProcessQueryMazeLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫查询猜数字次数
int ProcessQueryMazeGuessTimes(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫查询甩卖商品信息
int ProcessQueryMazeCanBuy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫放弃
int ProcessMazeAbandon(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫查询当前事件结果
int ProcessMazeQueryCurResult(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫查询BOSS掉落
int ProcessMazeQueryBossLoots(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//攻击迷宫怪 cmd：mazeKill
int ProcessMazeKill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//迷宫超时了
int ProcessMazeTimeout(json_spirit::mObject& o);

