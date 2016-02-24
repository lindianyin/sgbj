
#pragma once

#include "poker.hpp"
#include <string.h>
#include "net.h"
#include "utils_all.h"
#include "json_spirit.h"
#include "spls_timer.h"
#include "item.h"
#include "combat_attr.h"
#include "worker.hpp"

using namespace net;
using namespace json_spirit;

struct CharData;
class Boss;
struct chessCombat;


const int iCombatIgnoreSec = 120;
const int iCombatWaitSec = 5;

const int iChessBlank[8][5] =
{
    {1, 1, 5, 5, 3},
    {1, 2, 5, 2, 1},
    {1, 3, 5, 3, 1},
    {1, 4, 5, 4, 1},
    {1, 5, 5, 1, 4},
    {2, 1, 2, 5, 2},
    {3, 1, 3, 5, 2},
    {4, 1, 4, 5, 2},
};

enum COMBAT_STATE
{
    COMBAT_STATE_INIT = 0,
    COMBAT_STATE_INGOING,   //进行中
    COMBAT_STATE_END,       //结束
};

enum COMBAT_RESULT
{
    COMBAT_RESULT_INIT = 0,
    COMBAT_RESULT_ATTACK_WIN,//攻方胜利
    COMBAT_RESULT_ATTACK_LOSE,//攻方失败
};

enum COMBAT_TYPE
{
    COMBAT_TYPE_INIT = 0,
    COMBAT_TYPE_STRONGHOLD = 1,
    COMBAT_TYPE_ARENA = 2,
    COMBAT_TYPE_COPY = 3,
    COMBAT_TYPE_WILD_CITY = 4,
    COMBAT_TYPE_SHENLING = 5,
    COMBAT_TYPE_TREASURE_ROB = 6,
    COMBAT_TYPE_PK = 7,
    COMBAT_TYPE_GUILD_MOSHEN = 8,
    COMBAT_TYPE_BOSS = 9,
};

enum CARD_NOTIFY_TYPE
{
    CARD_NOTIFY_INIT = 0,
    CARD_NOTIFY_MAGIC = 1,
    CARD_NOTIFY_DEAL = 2,
};

struct chessPlayer
{
    chessPlayer()
    {
        m_auto = true;
        m_ignore = true;
        m_used_magic.clear();
        m_magic_spade = false;
        m_magic_health = false;
        m_magic_damage = false;
        m_wait_round = 0;
    }
    //玩家信息
    int m_cid;
    std::string m_player_name;
    std::string m_chat;//喊话内容
    int m_player_level;
    bool m_auto;//自动玩
    bool m_ignore;//忽略信息
    //出战英雄信息
    int m_hid;
    int m_hp_org;
    int m_hp_left;
    int m_hp_max;
    std::string m_name;
    int m_level;
    int m_star;
    int m_race;
    int m_attack;
    int m_defense;
    int m_magic;
    int m_spic;

    //战斗参数
    combatAttribute m_combat_attribute;

    //魔法效果相关
    std::map<int, int> m_used_magic;
    bool m_magic_spade;
    bool m_magic_health;
    bool m_magic_damage;
    int m_wait_round;

    //手牌
    myCard m_cards[4];

    json_spirit::Array m_magics; //技能
    json_spirit::Array m_equipt;//装备

    void set_my_first();

    void deal(int pos, const baseCard& c)
    {
        m_cards[pos-1].set(c.suit, c.value);
    }

    void getInfo(json_spirit::Object& robj);
    void getExtraInfo(json_spirit::Object& robj);

    //加载战斗角色
    //玩家
    int LoadCharactor(CharData& cdata, int hid = 0);
    int LoadStronghold(CharData& cdata, int strongholdid, int extra);
    int LoadCopy(CharData& cdata, int copyid);
    int LoadShenling(CharData& cdata, int sid);
    int LoadGuildMoshen(int moshenid);
    int LoadBoss(Boss* pb);
};

struct doAction
{
    int cid;
    int type;//对牌形成类型(行，列，正斜，反斜)
    int pos[2];
    int x[2];
    int y[2];
    uint8_t suit[2];
    uint8_t value[2];

    bool cast;
    int magic_id;

    void clear()
    {
        cid = 0;
        type = 0;
        pos[0] = 0;
        pos[1] = 0;
        x[0] = 0;
        x[1] = 0;
        y[0] = 0;
        y[1] = 0;
        suit[0] = 0;
        suit[1] = 0;
        value[0] = 0;
        value[1] = 0;

        cast = false;
        magic_id = 0;
    }
};

struct doRound
{
    std::list<doAction> m_actList;
    void reset()
    {
        m_actList.clear();
    }
};

struct chessGame
{
    chessGame(chessPlayer* p1, chessPlayer* p2);
    void refresh();
    void refreshChess9();
    //牌局开始
    int start();
    //局中操作
    int act(int cid,int seq,int pos1,int x1,int y1,int pos2,int x2,int y2);
    int pass(int cid, json_spirit::Object& robj);
    int cast(int cid, int magic_id);
    int autoAct(int cid, json_spirit::Object& robj);
    void roundEnd();

    //发牌
    int dealCard();
    baseCard* deal();
    void changeCard(uint8_t s, uint8_t v, int cid);
    bool checkPos(int x1, int y1, int x2, int y2, int& type);

    //计算某位置牌触发的牌组
    void checkFiveCard(doAction action);
    int cardDamage(int card_rank);

    chessPlayer* getPlayer(int cid);

    void broadCastMsg(const std::string& msg, int delay_sec = 0);
    void broadCastAct();
    void broadCastCastMagic();
    void broadCastCard(int cid, int next, int seq, int type, int delay_sec = 0);
    void broadCastChess(int type = 1, int delay_sec = 0);

    //ai
    void NPCActRandom(int cid);
    void NPCHighActRandom(int cid);

    chessCombat* m_combat;
    chessPlayer* player1;
    chessPlayer* player2;
    //牌堆
    myPoker m_poker;
    //牌桌
    baseCard m_chess[5][5];
    int m_count;
    //回合操作记录
    std::vector<doRound> m_rounds;
    doAction lastDo;
    //回合信息
    int m_round;
    int m_damage;
    int m_heal;
    int m_round_magic;
    int m_state;
    json_spirit::Array round_list;

    int m_next;
    int m_seq;


    void reset()
    {
        m_rounds.clear();
        m_damage = 0;
        m_heal = 0;
        m_round_magic = 0;
        m_round = 0;
        m_state = COMBAT_STATE_INIT;
        m_next = 0;
        lastDo.clear();
        m_seq = 0;
        for(int i = 0; i < 5; ++i)
        {
            for(int j = 0; j < 5; ++j)
            {
                m_chess[i][j].set(0,0);
            }
        }
        m_count = 0;
    }
    void roundReset()
    {
        ++m_round;
        m_damage = 0;
        m_heal = 0;
        m_round_magic = 0;
        round_list.clear();
        doRound r;
        r.reset();
        m_rounds.push_back(r);
    }
};

struct chessCombat
{
    int m_combat_id;
    int m_type;
    time_t m_ignore_time;
    int m_client_check;
    int m_state;
    int m_result;
    int m_result_type;
    int m_quit_cid; //退出的玩家

    chessPlayer* getPlayer(int cid);
    chessPlayer m_players[2];

    //战斗相关信息
    int m_data_id;
    int m_data_type;
    int m_extra_data[2];    //战斗的一些额外信息

    std::list<Item> m_getItems;          //获得的东西(攻击方)
    std::list<Item> m_getItems2;         //获得的东西(防守方)

    json_spirit::Object m_result_obj;       //战报结果
    json_spirit::Array m_result_array;   //战斗过程结果(全部)
    int m_archive_report;

    chessGame* start_a_game();
    chessGame* m_cur_game;

    chessCombat();
    void getCombatInfo(int cid, json_spirit::Object& robj);
    void AppendResult(json_spirit::Object obj);
    int onStart();
    int checkEnd();
    int quit(int cid);
};

class chessCombatMgr
{
public:
    chessCombatMgr();
    chessCombat* findCombat(int id);
    chessCombat* getCharCombat(int cid);
    chessCombat* createArenaCombat(CharData& c1, CharData& c2);
    chessCombat* createStrongholdCombat(CharData& c1, int strongholdid, bool random_extra);
    chessCombat* createCopyCombat(CharData& c1, int copyid);
    chessCombat* createWildCityCombat(CharData& c1, CharData& c2, int cityid, int hid);
    chessCombat* createShenlingCombat(CharData& c1, int sid);
    chessCombat* createTreasureCombat(CharData& c1, CharData& c2);
    chessCombat* createGuildMoshenCombat(CharData& c1, int moshenid);
    chessCombat* createBossCombat(CharData& c1, int id);
    //循环检查每场战斗状态
    void autoLoop();
    void onCombatEnd(int id);
    void combatTimeout(int id);

public:
    rwlock chessCombats_rwmutex;
    std::map<int, boost::shared_ptr<chessCombat> > m_combats;
    std::map<int, boost::shared_ptr<chessCombat> > m_char_combats;
};

enum combat_cmd_enum
{
    combat_cmd_create = 0,
    combat_cmd_player_act = 0x01,
    combat_cmd_player_quit = 0x02,
    combat_cmd_sign = 0x03,
    combat_cmd_start = 0x04,
    combat_cmd_end = 0x05,
    combat_cmd_timeout = 0x06,
    combat_cmd_continue = 0x07,
    combat_cmd_query = 0x08,
    combat_cmd_player_pass = 0x09,
    combat_cmd_deal = 0x0a,
    combat_cmd_player_auto = 0x0b,
    combat_cmd_player_cast = 0x0c,
    combat_cmd_next = 0x0d,
};

struct chessCombatCmd
{
    int cid;
    std::string cname;
    int cmd;
    json_spirit::mObject mobj;
    chessCombat* _pCombat;
    chessCombatCmd()
    {
        cid = 0;
        cmd = 0;
        _pCombat = NULL;
    }
};

class chessCombatProcesser : public worker<chessCombatCmd>
{
public:
    chessCombatProcesser(jobqueue<chessCombatCmd>& _jobqueue, std::size_t _maxthreads = 4) :
      worker<chessCombatCmd>("chessCombat",_jobqueue, _maxthreads)
    {

    }
    virtual bool work(chessCombatCmd &cmd);       // 在些完成实际任务.
};

//发起挑战
int ProcessChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj);
//牌局行动
int ProcessGameAct(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj);
//牌局过牌
int ProcessGamePass(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//牌局自动
int ProcessGameAuto(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//牌局行动
int ProcessGameCastMagic(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//客户端准备好战斗信号
int ProcessCombatSign(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//牌局继续
int ProcessGameNext(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//牌局开始
int ProcessStartGame(json_spirit::mObject& o);
//发牌
int ProcessGameDeal(json_spirit::mObject& o);
//操作超时
int ProcessGameTimeout(json_spirit::mObject& o);
//牌局结束
int ProcessGameEnd(json_spirit::mObject& o);
//退出挑战
int ProcessQuitChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//牌局npc动作
int ProcessNPCAct(json_spirit::mObject& o);

