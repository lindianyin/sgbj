#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include <boost/thread.hpp>

#include "base_item.h"

#ifdef VN_SERVER
const int iCharRankingsPage = 25;    //������ҳ��
const int iHeroRankingsPage = 25;    //�佫���а�ҳ��
#else
const int iCharRankingsPage = 8;    //������ҳ��
const int iHeroRankingsPage = 8;    //�佫���а�ҳ��
#endif
const int iRankingsPage = 8;        //���а�25ҳ
const int iRankingsPerPage = 8;    //ÿҳ8�� �� 200

using namespace json_spirit;

enum rankings_type
{
    rankings_type_char = 1,
    rankings_type_hero = 2,
    rankings_type_lottery = 3,
    rankings_type_elite = 4,
    rankings_type_prestige = 5,
    rankings_type_attack = 6,
    rankings_type_zst = 7,
};

//��ɫ����
struct charRankings
{
    int cid;    //��ɫid
    int rank;    //����
    int level;    //��ɫ�ȼ�
    int camp;    //��Ӫ
    int olevel;    //��ְ�ȼ�
    json_spirit::Array nicks;
    std::string name;    //��ɫ��
    std::string corps;    //������
    std::string offical;//��ְ��
};

struct splsCharRankings
{
      charRankings Rankings[iCharRankingsPage*iRankingsPerPage];
};

//Ӣ������
struct heroRankings
{
    int rank;
    int cid;
    int gid;
    int camp;
    int level;
    int quality;
    int attributeExtra;
    int rateNow;
    std::vector<int> genius;
    std::string name;
    std::string charname;
    json_spirit::Array nicks;
};

struct splsHeroRankings
{
    heroRankings Rankings[iHeroRankingsPage*iRankingsPerPage];
};

struct lotteryScoreRankings
{
    int rank;    //����
    int cid;    //��ɫid
    int camp;    //��Ӫ
    int score;    //����
    int olevel;    //��ְ�ȼ�
    std::string name;    //��ɫ��
    std::string corps;        //������
    std::string offical;    //��ְ��
};

struct splsLotteryScoreRankings
{
    lotteryScoreRankings Rankings[iRankingsPage*iRankingsPerPage];
};

struct eliteRankings
{
    int rank;    //����
    int cid;    //��ɫid
    int level;    //��ɫ�ȼ�
    int attack;    //ս��
    int elite_id;    //��Ӣid
    json_spirit::Array nicks;
    std::string name;    //��ɫ��
    std::string elite_name;        //��Ӣ��
};

struct splsEliteRankings
{
    eliteRankings Rankings[iRankingsPage*iRankingsPerPage];
};

struct prestigeGetRankings
{
    int rank;    //����
    int cid;    //��ɫid
    int level;  //�ȼ�
    int score;    //�������ֵ
    json_spirit::Array nicks;
    std::string name;    //��ɫ��
};

struct splsPrestigeGetRankings
{
    prestigeGetRankings Rankings[iRankingsPage*iRankingsPerPage];
};

struct attackRankings
{
    int rank;    //����
    int cid;    //��ɫid
    int level;  //�ȼ�
    int score;    //ս��
    json_spirit::Array nicks;
    std::string name;    //��ɫ��
    std::string corps;        //������
    std::string offical;    //��ְ��
};

struct splsAttackRankings
{
    attackRankings Rankings[iRankingsPage*iRankingsPerPage];
};

struct ZSTRankings
{
    int rank;    //����
    int cid;    //��ɫid
    int level;  //�ȼ�
    int attack;    //ս��
    int score;    //�Ǽ�
    json_spirit::Array nicks;
    std::string name;    //��ɫ��
    std::string corps;        //������
};

struct splsZSTRankings
{
    ZSTRankings Rankings[iRankingsPage*iRankingsPerPage];
};

enum shhx_rankings_event
{
    char_rankings = 1,
    hero_rankings = 2,
    boss_rankings = 3,
    camp_race_rankings = 4,
    lottery_rankings = 5
};

struct shhx_rankings
{
    int type;
    std::vector<int> cids;
};

//������Ľ���
struct rankings_event_award
{
    int cid;
    int rank;
    std::list<Item> awards;

    rankings_event_award()
    {
        cid = 0;
        rank = 0;
    }
};

//�����
struct rankings_event
{
    int id;
    int type;
    std::string mail_title;
    std::string mail_content;

    std::list<rankings_event_award> rankings_list;
};

//������а��Ƿ���
void checkRankingsEvent();
//����������
void giveRankingsEventReward(rankings_event* pE);

class splsRankings
{
public:
    splsRankings();
    
    //��������
    void updateRankings(int);

    //���½�ɫ����
    void _updateCharRankings();
    //����Ӣ������
    void _updateHeroRankings();
    //����÷���ה��e������
    void _updateLotteryScoreRankings();
    //���¾�Ӣս��������
    void _updateEliteRankings();
    //������������
    void _updatePrestigeGetRankings();
    //����ս������
    void _updateAttackRankings();
    //����ս��̨����
    void _updateZSTRankings();

    //��������
    void _updateRankings();

    //��ý�ɫ����
    int getCharRankings(int page, int cid, json_spirit::Object& robj);
    //��ѯӢ������
    int getHeroRankings(int page, int cid, json_spirit::Object& robj);
    //��ѯ÷���ה�����
    int getLotteryRankings(int page, int cid, json_spirit::Object& robj);
    //��ѯ��Ӣս����
    int getEliteRankings(int page, int cid, json_spirit::Object &robj);
    //��ѯ�����������
    int getPrestigeRankings(int page, int cid, json_spirit::Object &robj);
    //��ѯս������
    int getAttackRankings(int page, int cid, json_spirit::Object &robj);
    //��ѯս��̨����
    int getZSTRankings(int page, int cid, json_spirit::Object &robj);

    //�������а��е�cid�ֶ�
    void updateRankingsEvent(rankings_event* pE);

    static splsRankings* getInstance();

private:
    static splsRankings* m_handle;

    boost::shared_ptr<const splsCharRankings> m_splsCharRankings;
    boost::shared_ptr<const splsHeroRankings> m_splsHeroRankings;
    boost::shared_ptr<const splsLotteryScoreRankings> m_splsLotteryRankings;
    boost::shared_ptr<const splsEliteRankings> m_splsEliteRankings;
    boost::shared_ptr<const splsPrestigeGetRankings> m_splsPrestigeRankings;
    boost::shared_ptr<const splsAttackRankings> m_splsAttackRankings;
    boost::shared_ptr<const splsZSTRankings> m_splsZSTRankings;

    boost::shared_ptr<const json_spirit::Array> m_charRankingsPages[iCharRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_heroRankingsPages[iHeroRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_lotteryRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_eliteRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_prestigeRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_attackRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_ZSTRankingsPages[iRankingsPage];

    boost::shared_ptr<const json_spirit::Object> m_charRankingsPageobj[iCharRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_heroRankingsPageobj[iHeroRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_lotteryRankingsPageobj[iRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_eliteRankingsPageobj[iRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_prestigeRankingsPageobj[iRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_attackRankingsPageobj[iRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_ZSTRankingsPageobj[iRankingsPage];

    //boost::shared_ptr<const std::string> m_strCharRankingsPages[iRankingsPage];
    //boost::shared_ptr<const std::string> m_strHeroRankingsPages[iRankingsPage];

    volatile int m_updating_heroRankings;
    volatile int m_updating_charRankings;
    volatile int m_updating_lotteryRankings;
    volatile int m_updating_eliteRankings;
    volatile int m_updating_prestigeRankings;
    volatile int m_updating_attackRankings;
    volatile int m_updating_ZSTRankings;

    boost::shared_ptr<boost::thread> _update_rankings_threadptr;
};

