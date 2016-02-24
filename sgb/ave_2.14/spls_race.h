#pragma once

#include <string>
#include <time.h>
#include <map>
#include <list>
#include <vector>

#include "json_spirit.h"

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#include "spls_const.h"
#include "net.h"

using namespace net;

//��������������
const int iRaceBuyGoldMax = 20;

struct raceRecord
{
    uint64_t id;    //ս��Ψһid
    int input;        //ս��ʱ��
    int tid;        //��ս�Ľ�ɫid
    int result;    //��� 1 ʤ�� 2ʧ��
    std::string tname;    //��ս�Ľ�ɫ��
    int type;    //��¼����1����2������
    int rank_result;//�������
};

struct raceRankRewards
{
    int rank;
    double silver;
    int prestige;
};

struct raceTitle
{
    uint64_t id;    //ս��Ψһid
    int input;
    std::string atk_name;    //������ɫ��
    std::string def_name;    //���صĽ�ɫ��
};

//�����̵���Ʒ
struct baseRaceGoods
{
    baseRaceGoods()
    {
        type = 0;
        id = 0;
        num = 0;
        spic = 0;
        price = 0;
        quality = 0;
        //need_notify = false;
        name = "";
        memo = "";
    }
    int type;// 1 ��Դ(��ҡ�����)2����
    int id;
    int num;
    int spic;
    int price;
    int quality;
    //bool need_notify;
    std::string name;
    std::string memo;
};

struct CharData;

struct CharRaceData
{
private:
    boost::shared_ptr<CharData> m_charactor;    //��Ӧ�Ľ�ɫ
public:
    CharRaceData(int cid, int mapid, int level);

    CharData* getChar();

    boost::shared_ptr<CharData> getCharData();
    
    int m_cid;

    bool m_needSave;
    int m_rank;        //����

    int m_mapid;    //��ͼid
    int m_under_attack;//���ڱ�˭��ս
    int m_attack_who;    //������ս˭

    int m_race_times;    //��������

    int m_total_race;    //�ܾ�������

    int m_wins;            //��ʤ����

    time_t m_race_cd_time;        //������ȴʱ��

    time_t m_save_time;        //����ʱ��

    int m_trend;                //���������½�څ��  0��׃ 1���� 2�½�

    int m_score;        //���վ�������
    int m_total_score;    //�ܻ���

    int save(bool force_save);

    boost::shared_ptr<std::list<boost::shared_ptr<raceRecord> > > m_race_records;    //����ս����ʷս��
    boost::shared_ptr<std::list<boost::shared_ptr<raceRecord> > > getRaceRecords();
    int addRaceRecord(uint64_t id, int result, int target, const std::string& target_name, int type, int rank_result);
};

class Combat;

class RaceMgr
{
public:
    RaceMgr();
    int reload();
    int getAction(CharData* cdata, json_spirit::Array& elist);
    boost::shared_ptr<CharRaceData> getRaceData(int cid);
    bool canChallege(int rank1, int rank2);
    int challenge(session_ptr& psession, int cid, int target, json_spirit::Object& robj);
    //������ս����
    int buyChallenge(session_ptr& psession, int cid, json_spirit::Object& robj);
    
    int challengeResult(Combat* pCombat);
    int querySelfInfo(int cid, json_spirit::Object&);
    int queryRaceList(int cid, int type, int counts, json_spirit::Object& robj);
    boost::shared_ptr<CharRaceData> addCharactor(int cid, int level, int mapid, int rank, time_t coolTime, int raceTimes, int totalRace, int wins, int score, int total_score);
    //�Ӿ������Ƴ���ɫ(��ɾ��)
    int removeCharactor(int cid);

    int updateZone(int cid);    //��Ǩ������
    //�����ȴʱ��
    int clearCD(int cid, json_spirit::Object& robj);

    int saveAll();
    int resetAll();
    int updateAll();
    //ÿ�궬��������23��佱
    int yearAwards();

    int deleteChar(int cid);

    //ǰ�����������
    CharRaceData* getPreRacer(int rank);
    //�������������
    CharRaceData* getNextRacer(int rank);
    //�@ȡ����������
    int getTop20(json_spirit::Object& robj);

    //��ѯ��������
    int QueryRankRewards(int rank1, int rank2, json_spirit::Object& robj);

    //��������
    int getYearAwards(int rank, int& prestige, int& silver);

    int getRandomServantList(int cid, int mapid, std::vector<int>& list);
    
    int getTopBattleId(){return m_top_battle_id;}

    void setRaceFreeTimes(int times);

    static RaceMgr* getInstance();
    //�������һ��̵�
    int getList(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int buy(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int left_score(int race_time, int& goal_times);

private:
    static RaceMgr* m_handle;
    boost::unordered_map<int, boost::shared_ptr<CharRaceData> > m_race_maps;        //������Ϣ
    std::vector<boost::shared_ptr<CharRaceData> > m_race_rank[max_map_id];      //�������ڵĽ�ɫ����
    boost::unordered_map<int,boost::shared_ptr<raceTitle> > m_race_title;
    uint64_t m_top_battle_id;

    boost::shared_ptr<CharRaceData> m_top20[20];    //��������ǰ20

    std::vector<raceRankRewards> m_rewards;        //��������
    int m_last_rank;        //����н�����

    std::vector<int> m_last_top_five;        //�ϴ�����ǰ5��
    
    std::vector<baseRaceGoods> m_goods;        //��Ʒ�б�
};

