
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
    //�����Ϣ
    int m_cid;
    std::string m_player_name;
    std::string m_chat;//��������
    int m_player_level;
    bool m_auto;//�Զ���

    //��սӢ����Ϣ
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

    //ս������
    int m_virtual_combat;
    int m_virtual_silver;
    int m_org_silver;
    int m_silver;
    int m_bet_silver;
    int m_round_bet;//��¼ĳ�غ���ע
    int m_combat_state;//�ƾֽ���ʱ�������(-2�Լ��˳�-1δ�μ�0��δ����1�Ѿ�����2�Ѿ�����)
    int m_deal_state;//�ƾַ��Ʊ��(0δ��1�Ѿ���)

    combatAttribute m_combat_attribute;
    //ָ������
    std::vector<int> m_card_rank;

    //�Լ�Ӣ���ƣ��Է�Ӣ���ƣ��Լ��������ţ��Լ������ƣ��Է�������
    //����ս��:��һ�ֹ����ƣ���һ�ֹ�����2���Լ�Ӣ���ƣ��Լ��������ţ��ڶ��ֹ����ƣ��ڶ��ֹ�����2
    sevenCards m_cards;

    json_spirit::Array m_equipt;//װ��

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

    //����ս����ɫ
    //���
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
    //�ƾֿ�ʼ
    int onStart();
    //���в���
    int onCheck(int cid);
    int onFold(int cid);
    int onDouble(int cid);
    int onHalf(int cid);
    int onCall(int cid);
    //�ƾֽ���
    int onEnd();

    //����
    int dealCard();
    int dealer();//�������ƴ�С��ȡ������
    baseCard* deal();
    int deal1();
    int deal2();
    int deal3();
    int deal4();
    void resetDeal();

    //�������
    void betBase();
    void getSilver();
    void backSilver();
    int startCid();
    int nextCid(int cid);
    void resetCall();

    //���ƹ������ɻ�ʤ����б�
    void compare_result();
    //���AIѺע����
    int NPCActRandom(int cid);

    void doAttack();

    combatPlayer* getPlayer(int cid);

    void broadCastMsg(const std::string& msg);

    void broadCastAct(int cid, int type, int call, int more, int total, int left, int canMore, int next, int seq);
    void broadCastAct();

    void broadCastCard(bool pri, std::vector<std::pair<int, int> >& card_list, int next, int seq);

    int m_combat_id;
    multi_combat* p_combat;
    std::vector<int> m_pos_list;//����λ��
    int m_player_cnt;//���������
    int m_fold_cnt;//���������
    //�˿��ƶ�
    myPoker m_poker;
    //������
    baseCard public_cards[4];

    betRound m_rounds[5];
    int m_result;
    combatPlayer* m_attacker;//����ս��������(������ȡ�����Ĺ���)
    std::vector<int> m_winner_list;//��ʤ�ߣ������Ƕ���
    std::vector<int> m_damage_list;//ʧ�����ܵ��˺�
    int m_silver;
    int m_exp;
    int m_round;
    int m_state;
    int m_base_bet;
    int m_call_bet;//��call��Ҫ����
    int m_total_bet;//�������
    int m_dealer;

    int m_next;//�ȴ�˭
    int m_seq;//������

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

    //ս�������Ϣ
    int m_data_id;
    int m_data_type;
    int m_extra_data[2];    //ս����һЩ������Ϣ

    json_spirit::Object m_result_obj;       //ս�����
    json_spirit::Array m_result_array;   //ս�����̽��(ȫ��)
    int m_archive_report;


    combatPlayer* getPlayer(int cid);
    void addPlayer(int pos, boost::shared_ptr<combatPlayer> p);
    void removePlayer(combatPlayer* p);
    void toObj(json_spirit::Object& obj);

    std::map<int, boost::shared_ptr<combatPlayer> > m_players;
    std::vector<int> m_pos_list;//����ս��λ��
    int m_winner;//���ʤ����

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
    //ѭ�����ÿ��ս��״̬
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
    virtual bool work(MultiCombatCmd &cmd);       // ��Щ���ʵ������.
};

//������ս
int ProcessMultiChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj);
//�ƾ��ж�
int ProcessMultiGameAct(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj);
//�ͻ���׼����ս���ź�
int ProcessMultiCombatSign(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ƾֿ�ʼ
int ProcessMultiStartGame(json_spirit::mObject& o);
//������ʱ
int ProcessMultiGameTimeout(json_spirit::mObject& o);
//�ƾֽ���
int ProcessMultiGameEnd(json_spirit::mObject& o);
//�˳���ս
int ProcessMultiQuitChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����μ�ս��
int ProcessMultiContinueChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ս����Ϣ
int ProcessMultiQueryCombatInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ƾ�npc����
int ProcessMultiNPCAct(json_spirit::mObject& o);

