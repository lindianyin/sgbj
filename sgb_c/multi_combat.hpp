
#pragma once

#include "poker.hpp"
#include "new_combat.hpp"
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

const int iAIAct[TOTAL_HAND_RANK][3] =
{
    {10, 15, 75},//ROYAL_FLUSH
    {10, 15, 75},//SHELL_FLUSH
    {10, 15, 75},//STRAIGHT_FLUSH
    {8, 15, 77},//FOUR_OF_A_KIND
    {5, 13, 82},//FULL_HOUSE
    {5, 10, 85},//FLUSH
    {12, 16, 72},//STRAIGHT
    {0, 0, 100},//THREE_OF_A_KIND
    {0, 0, 100},//TWO_PAIR
    {0, 0, 100},//ONE_PAIR
    {0, 0, 100},//HIGH_CARD
};

enum COMBAT_CMD
{
    COMBAT_CMD_START = 0,
    COMBAT_CMD_DOUBLEBET = 1,
    COMBAT_CMD_HALFBET = 2,
    COMBAT_CMD_CALL = 3,
    COMBAT_CMD_CHECK = 4,
    COMBAT_CMD_FOLD = 5,
};

enum GAME_RESULT
{
    GAME_PINGJU = 0,
    GAME_OVER = 1,
    GAME_FOLD = 2
};

struct combatPlayer
{
    //玩家信息
    int m_cid;
    std::string m_player_name;
    std::string m_chat;//喊话内容
    int m_player_level;
    bool m_auto;//自动玩

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
    int m_virtual_combat;
    int m_virtual_silver;
    int m_org_silver;
    int m_silver;
    int m_bet_silver;
    int m_round_bet;//记录某回合下注
    int m_combat_state;//牌局进行时操作标记(-2自己退出-1未参加0还未操作1已经跟过2已经放弃)
    int m_deal_state;//牌局发牌标记(0未发1已经发)

    combatAttribute m_combat_attribute;
    //指定牌型
    std::vector<int> m_card_rank;

    //自己英雄牌，对方英雄牌，自己暗牌三张，自己公共牌，对方公共牌
    //多人战斗:第一轮公共牌，第一轮公共牌2，自己英雄牌，自己暗牌两张，第二轮公共牌，第二轮公共牌2
    sevenCards m_cards;

    json_spirit::Array m_equipt;//装备

    void set_my_first();
    void set_his_first(combatPlayer* p);

    void deal_public_1(const baseCard& c)
    {
        m_cards.cards[0].set(c.suit, c.value);
    }
    void deal_public_2(const baseCard& c)
    {
        m_cards.cards[1].set(c.suit, c.value);
    }
    void deal_public_3(const baseCard& c)
    {
        m_cards.cards[5].set(c.suit, c.value);
    }
    void deal_public_4(const baseCard& c)
    {
        m_cards.cards[6].set(c.suit, c.value);
    }

    void deal_private_1(const baseCard& c)
    {
        m_cards.cards[2].set(c.suit, c.value);
    }
    void deal_private_2(const baseCard& c)
    {
        m_cards.cards[3].set(c.suit, c.value);
    }
    void deal_private_3(const baseCard& c)
    {
        m_cards.cards[4].set(c.suit, c.value);
    }

    void deal_my_public(const baseCard& c)
    {
        m_cards.cards[1].set(c.suit, c.value);
    }
    void deal_his_public(const baseCard& c)
    {
        m_cards.cards[6].set(c.suit, c.value);
    }

    void getInfo(json_spirit::Object& robj);
    void getExtraInfo(json_spirit::Object& robj);
    bool inCombat() {return m_combat_state >= 0;}

    //加载战斗角色
    //玩家
    int LoadCharactor(CharData& cdata, int hid = 0);
};

struct betAction
{
    int cid;
    int type;
    int call;
    int more;
    int total;
    int left;

    void clear()
    {
        cid = 0;
        type = 0;
        call = 0;
        more = 0;
        total = 0;
        left = 0;
    }
};

struct betRound
{
    std::list<betAction> m_actList;
    void reset()
    {
        m_actList.clear();
    }
};

struct multi_combat;

struct multi_cardgame
{
    multi_cardgame(multi_combat* c);
    //牌局开始
    int onStart();
    //局中操作
    int onCheck(int cid);
    int onFold(int cid);
    int onDouble(int cid);
    int onHalf(int cid);
    int onCall(int cid);
    //牌局结束
    int onEnd();

    //发牌
    int dealCard();
    int dealer();//根据手牌大小获取发牌者
    baseCard* deal();
    int deal1();
    int deal2();
    int deal3();
    int deal4();
    void resetDeal();

    //筹码操作
    void betBase();
    void getSilver();
    void backSilver();
    int startCid();
    int nextCid(int cid);
    void resetCall();

    //手牌估算生成获胜玩家列表
    void compare_result();
    //随机AI押注操作
    int NPCActRandom(int cid);

    void doAttack();

    combatPlayer* getPlayer(int cid);

    void broadCastMsg(const std::string& msg);

    void broadCastAct(int cid, int type, int call, int more, int total, int left, int canMore, int next, int seq);
    void broadCastAct();

    void broadCastCard(bool pri, std::vector<std::pair<int, int> >& card_list, int next, int seq);

    int m_combat_id;
    multi_combat* p_combat;
    std::vector<int> m_pos_list;//多人位置
    int m_player_cnt;//参与玩家数
    int m_fold_cnt;//放弃玩家数
    //扑克牌堆
    myPoker m_poker;
    //公共牌
    baseCard public_cards[4];

    betRound m_rounds[5];
    int m_result;
    combatPlayer* m_attacker;//多人战斗攻击者(多个玩家取最弱的攻击)
    std::vector<int> m_winner_list;//获胜者，可以是多人
    std::vector<int> m_damage_list;//失败者受到伤害
    int m_silver;
    int m_exp;
    int m_round;
    int m_state;
    int m_base_bet;
    int m_call_bet;//跟call需要筹码
    int m_total_bet;//桌面筹码
    int m_dealer;

    int m_next;//等待谁
    int m_seq;//步骤标记

    betAction lastAct;

    boost::uuids::uuid m_timer;

    void reset()
    {
        for (int i = 0; i < 4; ++i)
        {
            public_cards[i].suit = 0;
            public_cards[i].value = 0;
        }
        for (int i = 0; i < 5; ++i)
        {
            m_rounds[i].reset();
        }
        m_fold_cnt = 0;
        m_result = 0;
        m_attacker = NULL;
        m_winner_list.clear();
        m_damage_list.clear();
        m_silver = 0;
        m_exp = 0;
        m_round = 0;
        m_state = COMBAT_STATE_INIT;
        m_call_bet = 0;
        m_total_bet = 0;
        m_next = 0;
        m_seq = 0;
        lastAct.clear();
    }
};

struct multi_combat
{
    int m_combat_id;
    int m_type;
    int m_base_bet;
    time_t m_ignore_time;
    int m_client_check;
    int m_state;

    //战斗相关信息
    int m_data_id;
    int m_data_type;
    int m_extra_data[2];    //战斗的一些额外信息

    json_spirit::Object m_result_obj;       //战报结果
    json_spirit::Array m_result_array;   //战斗过程结果(全部)
    int m_archive_report;


    combatPlayer* getPlayer(int cid);
    void addPlayer(int pos, boost::shared_ptr<combatPlayer> p);
    void removePlayer(combatPlayer* p);
    void toObj(json_spirit::Object& obj);

    std::map<int, boost::shared_ptr<combatPlayer> > m_players;
    std::vector<int> m_pos_list;//多人战斗位置
    int m_winner;//最后胜利者

    multi_cardgame* start_a_game();
    multi_cardgame* m_cur_game;


    multi_combat();
    void getCombatInfo(int cid, json_spirit::Object& robj);
    void AppendResult(json_spirit::Object obj);
    int processCombat(int cid, int cmd, int seq);
    void onStart();
    int checkEnd();
    int quit(int cid);
    int continueNext(int cid);
    void broadCastCharList();
};

class MultiCombatMgr
{
public:
    MultiCombatMgr();
    multi_combat* findMultiCombat(int id);
    void addCharMultiCombat(int cid, multi_combat* pCombat);
    void removeCharMultiCombat(int cid);
    multi_combat* getCharMultiCombat(int cid);
    multi_combat* createPkCombat(std::vector<int>& list, int bet);
    //循环检查每场战斗状态
    void autoLoop();
    void onCombatEnd(int id);
    void combatTimeout(int id, int notify);

public:
    rwlock combats_rwmutex;
    std::map<int, multi_combat* > m_combats;
    std::map<int, multi_combat* > m_char_combats;
};

struct MultiCombatCmd
{
    int cid;
    std::string cname;
    int cmd;
    json_spirit::mObject mobj;
    multi_combat* _pCombat;
    MultiCombatCmd()
    {
        cid = 0;
        cmd = 0;
        _pCombat = NULL;
    }
};

class MultiCombatProcesser : public worker<MultiCombatCmd>
{
public:
    MultiCombatProcesser(jobqueue<MultiCombatCmd>& _jobqueue, std::size_t _maxthreads = 4) :
      worker<MultiCombatCmd>("multi_combat",_jobqueue, _maxthreads)
    {

    }
    virtual bool work(MultiCombatCmd &cmd);       // 在些完成实际任务.
};

//发起挑战
int ProcessMultiChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj);
//牌局行动
int ProcessMultiGameAct(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj);
//客户端准备好战斗信号
int ProcessMultiCombatSign(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//牌局开始
int ProcessMultiStartGame(json_spirit::mObject& o);
//操作超时
int ProcessMultiGameTimeout(json_spirit::mObject& o);
//牌局结束
int ProcessMultiGameEnd(json_spirit::mObject& o);
//退出挑战
int ProcessMultiQuitChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//继续参加战斗
int ProcessMultiContinueChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//请求战斗信息
int ProcessMultiQueryCombatInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//牌局npc动作
int ProcessMultiNPCAct(json_spirit::mObject& o);

