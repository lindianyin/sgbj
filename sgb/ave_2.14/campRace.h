#pragma once

//��Ӫսͷ�ļ�
#include <string>
#include <map>
#include <list>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <time.h>
#include "json_spirit.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "utils_all.h"
#include "spls_const.h"
#include "data.h"

class ChatChannel;
class CharData;
struct CharGeneralData;
class Combat;
struct OnlineCharactor;

#ifdef CAMP_RACE_TWO_QUEUE
const int iCampRaceHighQueSize = (iMaxCharLevel - 60)/10 + 1;
#else
const int iCampRaceQueSize = iMaxCharLevel/10;
#endif

enum campRaceState
{
    camp_race_state_idle = 1,
    camp_race_state_wait = 2,
    camp_race_state_fighting = 3,
    camp_race_state_leave = 4
};

enum campRaceReportType
{
    camp_race_report_win = 1,
    camp_race_report_end,
    camp_race_report_skip,
    camp_race_report_fail,
};

struct campRaceReport
{
    int _type;    //���� 1��ܣ�2�ս� 3 ս�� 4 �ֿ�
    int _id1;
    int _id2;
    int _camp1;
    int _camp2;
    std::string _name1;
    std::string _name2;
    int _silver;
    int _prestige;
    int _win_times;
};

struct campRaceReportList
{
    std::list<campRaceReport> _report_list;

    void addReport(int type, int id1, int id2, const std::string& name1, const std::string& name2, int silver, int prestige, int win_times, int, int);
    int getReport(json_spirit::Array& reportArray, int cid, bool self);
};

struct charCampRace
{
    int _cid;    //��ɫid

    int _wins;    //ʤ������
    int _loses;    //ʧ�ܴ���
    int _skips;//��������
    int _max_continue_wins;    //�����ɱ
    int _cur_continue_wins;    //��ǰ��ɱ

    int _prestige;    //�ۼƻ������
    int _silver;    //�ۼƻ������

    int _state;            // 1 ���� 2 �ȴ�  4 �뿪
    int _in_combat;    //��ҹر�ս�����´���Ӫ������ 0 �������1 ��ʾ���ڿ�ս��

    time_t _idle_time;    //����ʱ��

    boost::shared_ptr<CharData> _cdata;
    boost::shared_ptr<OnlineCharactor> _onlineChar;

    int _zhen_type;
    boost::shared_ptr<CharGeneralData> m_generals[9];      //��λ�õ��佫
    int m_generals_hp[9]; //��λ�õ��佫������Ѫ�� -1��ʾ����

    //campRaceReportList _reports;    //����ս��
};

struct campRaceWaitQue
{
#ifdef CAMP_RACE_TWO_QUEUE    //�Ƿ񻮷�Ϊ��������
    //�ߵȼ�ꇠI��ȴ����
    std::vector<boost::shared_ptr<charCampRace> > m_camp_wait_que_high[iCampRaceHighQueSize];
    //�͵ȼ�ꇠI��ȴ����
    std::vector<boost::shared_ptr<charCampRace> > m_camp_wait_que_low[4];    // 30-39��40-49��50-59��60
#else
    //ꇠI��ȴ����
    std::vector<boost::shared_ptr<charCampRace> > m_camp_wait_que[iCampRaceQueSize];    // 30-39��40-49��50-59��60-69,70-79...
#endif
    //���ѡ��ս����
    charCampRace* _getRacer(bool high);

#ifdef CAMP_RACE_TWO_QUEUE
    size_t m_total_high;
    size_t m_total_low;
#else
    size_t m_total;
#endif

    void add(boost::shared_ptr<charCampRace> sp);
    void clear();
};

struct campRaceWinsRanking
{
    int cid;
    int wins;
};

const int iCampRaceRankingsPage = 3;        //���а�3ҳ
const int iCampRaceRankingsPerPage = 8;    //ÿҳ8��
const int iCampRaceRankingsCount = iCampRaceRankingsPage*iCampRaceRankingsPerPage;

struct rankings_event;

struct campRaceWinsRankings
{
    int size;
    int min_wins;
    std::map<int, int> winsMap;
    std::list<campRaceWinsRanking> winsList;

    int rankings[iCampRaceRankingsCount];
    std::string strRangkingsPages[iCampRaceRankingsPage];
    json_spirit::Array aRankingsPages[iCampRaceRankingsPage];

    void updateRankings(std::map<int, int>&);
    //�������а��е�cid�ֶ�
    void updateRankingsEvent(rankings_event* pE);

    std::string getRankings(int page = 1);
};

class campRaceMgr
{
public:
    campRaceMgr();
    //����
    int reload();
    //������Ӫս
    int joinRace(boost::shared_ptr<CharData>, boost::shared_ptr<OnlineCharactor> ou, json_spirit::Object& robj);
    //�뿪��Ӫս
    int leaveRace(CharData* pc, boost::shared_ptr<OnlineCharactor>);
    //��ս/ȡ����ս
    int setFight(CharData* pc, bool enable, json_spirit::Object& robj);
    //����
    int inspire(CharData* pc, int type, json_spirit::Object& robj);    //���� type=1���ҹ��裬type=2 ��ҹ���
    int getInspire(CharData* pc);

    //��ѯս�� type=1����ս�� 2=�Լ���ս��
    int queryHistory(CharData* pc, int type, json_spirit::Object& robj);
    //��ѯ˫����ս��Ա
    int queryCharList(json_spirit::Object& robj);
    //��ѯ����
    int queryPoints(json_spirit::Object& robj);
    //��ѯ��ʤ��Ϣ
    int queryConsecutiveVictory(json_spirit::Object& robj);
    //��ѯ�Լ�����Ӫս��Ϣ
    int querySelfInfo(CharData* pc, json_spirit::Object& robj);
    //��ѯ��Ӫս����ʱ��
    int queryEndTime(json_spirit::Object& robj);
    //��ѯ������Ϣ
    int queryInspireInfo(CharData* pc, json_spirit::Object& robj);
    //׽����ɱ
    int Race();
    
    int combatResult(Combat* pCombat);    //��Ӫս������

    int open(int);    //����

    int close();//����
    //����ʱ��
    int endTime();
    //�����ʤ��¼
    void getMaxWin(std::string& name, int& times, int& camp);
    //˫������
    void getScore(int& score1, int& score2);
    //�μ����б�
    int getRacers(int camp, json_spirit::Array& array, json_spirit::Array& array2);
    //��Ƿ���
    void getAction(CharData* pc, json_spirit::Array& blist);
    int getActionMemo(std::string& memo);

    campRaceReportList& getReportList1();
    campRaceReportList& getReportList2();

    boost::shared_ptr<charCampRace> getChar(int camp, int cid);
    void broadMsg(const std::string& msg);
    void broadMsg(int level_from, int level_to, const std::string& msg);

    std::string getRankings(int page = 1);
    //�������а��е�cid�ֶ�
    void updateRankingsEvent(rankings_event* pE);

    bool isOpen();

    std::string openTime();

    stand_in_mob m_stand_in_mob;

    void getJoinCount(int camp, int& count1, int& count2);

    static campRaceMgr* getInstance();
private:
    int _join(boost::shared_ptr<CharData> cdata, boost::shared_ptr<OnlineCharactor> ou);
    //����ս��
    int _Race(charCampRace* a, charCampRace* d, json_spirit::Array&);

    void updateWaitQue(json_spirit::Array&);
    //�ֿմ���
    void noMatch(charCampRace*);
    //���⽱���������ʼ�
    void giveExtraAndMail(int camp, int type);    // 0 ʧ��û�н���  1 ƽ��һ�뽱�� 2 ʤ��ȫ������

    static campRaceMgr* m_handle;

    std::map<int, boost::shared_ptr<charCampRace> > m_data_map[2];

    std::map<int, boost::shared_ptr<charCampRace> > m_total_data_map;
    //�ٸ��ȴ����
    campRaceWaitQue m_guanfu_que;
    //�G�ֵȴ����
    campRaceWaitQue m_lvlin_que;

    std::map<int, int> m_inspire_map;            //��������

    std::string _str_will_open_msg;
    std::string _str_have_closed;
    std::string _open_time;

    int _open_state;

    openRules _open_rules;    //���Ź��򣬿����ж��
    openRule* _last_rule;    //�ϴο��Ź���

    time_t _start_time;    //����ʱ���
    time_t _end_time;        //����ʱ���
    boost::uuids::uuid _uuid;    //��ʱ��Ψһid - �ر���Ӫս�Ķ�ʱ��
    
    boost::uuids::uuid _uuid_race;    //��ʱ��Ψһid - ��ʱƥ����Ӫս���ֵĶ�ʱ��

    std::string _max_winner;    //�����ʤ���
    int _max_win_times;        //�����ʤ����
    int _max_win_camp;

    int _score[2];                //˫������
    
    campRaceReportList _reports;    //�͵ȼ�ȫ��ս��
    campRaceReportList _reports2;    //�ߵȼ�ȫ��ս��

    campRaceWinsRankings m_rankings;    //��ʤ����

};

