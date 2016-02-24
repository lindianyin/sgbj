#pragma once

#include <string>
#include <time.h>
#include <map>
#include <list>
#include <vector>

#include "json_spirit.h"

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#include "const_def.h"
#include "net.h"
#include "item.h"
#include "new_combat.hpp"

using namespace net;

struct arenaRecord
{
    uint64_t id;    //ս��Ψһid
    int input;        //ս��ʱ��
    int tid;        //��ս�Ľ�ɫid
    int result;    //��� 1 ʤ�� 2ʧ��
    std::string tname;    //��ս�Ľ�ɫ��
    int type;    //��¼����1����2������
    int rank_result;//�������
};

struct arenaRankRewards
{
    int rank;
    std::list<Item> reward;//����
};

struct arenaTitle
{
    uint64_t id;    //ս��Ψһid
    int input;
    std::string atk_name;    //������ɫ��
    std::string def_name;    //���صĽ�ɫ��
};

//�����̵���Ʒ
struct baseArenaGoods
{
    int needscore;
    Item reward;//����
};

struct CharData;

struct CharArenaData
{
private:
    boost::shared_ptr<CharData> m_charactor;    //��Ӧ�Ľ�ɫ
public:
    CharArenaData(int cid);
    boost::shared_ptr<CharData> getCharData();
    boost::shared_ptr<std::list<boost::shared_ptr<arenaRecord> > > m_arena_records;    //����ս����ʷս��
    boost::shared_ptr<std::list<boost::shared_ptr<arenaRecord> > > getArenaRecords();
    int addArenaRecord(uint64_t id, int result, int target, const std::string& target_name, int type, int rank_result);
    int save(bool force_save);
    void toObj(json_spirit::Object& robj);

    int m_cid;
    bool m_needSave;
    int m_rank;        //����
    int m_under_attack;//���ڱ�˭��ս
    int m_attack_who;    //������ս˭
    int m_total_arena;    //�ܾ�������
    int m_wins;            //��ʤ����
    time_t m_save_time;        //����ʱ��
    int m_trend;                //���������½�څ��  0��׃ 1���� 2�½�
};

class arenaMgr
{
public:
    arenaMgr();
    boost::shared_ptr<CharArenaData> getArenaData(int cid);
    bool canChallege(int rank1, int rank2);
    //������ս����
    int buyChallenge(session_ptr& psession, int cid, json_spirit::Object& robj);
    int querySelfInfo(int cid, json_spirit::Object& robj);
    int queryArenaList(int cid, json_spirit::Object& robj);
    boost::shared_ptr<CharArenaData> addCharactor(int cid, int rank, int totalArena, int wins);
    //�����ȴʱ��
    int clearCD(int cid, json_spirit::Object& robj);
    //ǰ�����������
    CharArenaData* getPreChar(int rank);
    //�������������
    CharArenaData* getNextChar(int rank);

    //��ѯ��������
    int QueryRankRewards(int rank1, int rank2, json_spirit::Object& robj);
    //��������
    int getSeasonAwards(int rank, json_spirit::Array& reward_list);
    int seasonAwards();
    void setArenaFreeTimes(int times);
    int combatResult(chessCombat* pCombat);
    //�������һ��̵�
    int getList(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int getGood(int cid, json_spirit::mObject& o, json_spirit::Object& robj);

private:
    boost::unordered_map<int, boost::shared_ptr<CharArenaData> > m_arena_maps;        //������Ϣ
    std::vector<boost::shared_ptr<CharArenaData> > m_arena_rank;      //�������ڵĽ�ɫ����
    //����ս��Ϣ
    boost::shared_ptr<arenaTitle> m_arena_title;
    uint64_t m_top_battle_id;

    std::vector<arenaRankRewards> m_rewards;        //��������
    int m_last_rank;        //����н�����
    std::vector<int> m_last_top;        //�ϴ������гƺŵ����
    std::vector<baseArenaGoods> m_goods;        //��Ʒ�б�
};

//��þ��������б�
int ProcessQueryArenaRankList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�Լ��ľ�����Ϣ
int ProcessQueryArenaInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ���н���
int ProcessQueryRankRewards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������ս����
int ProcessBuyArena(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����������Ʒ
int ProcessQueryArenaGoodsList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ����������Ʒ
int ProcessGetArenaGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

