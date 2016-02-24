#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include <boost/thread.hpp>

#include "item.h"

const int iRankingsPage = 10;        //���а�10ҳ
const int iRankingsPerPage = 10;    //ÿҳ10�� �� 100

using namespace json_spirit;

//��ɫ����
struct charRankings
{
    int cid;    //��ɫid
    int rank;    //����
    int level;    //��ɫ�ȼ�
    int race;    //����
    std::string union_name;  //����
    std::string name;    //��ɫ��
    int vip;
};

struct gameCharRankings
{
      charRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//�Ƹ�����
struct silverRankings
{
    int cid;    //��ɫid
    int rank;    //����
    int silver;    //����
    int race;    //����
    std::string union_name;  //����
    std::string name;    //��ɫ��
    int vip;
};

struct gameSilverRankings
{
      silverRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//Ӣ������
struct heroRankings
{
    int rank;
    int cid;
    int hid;
    int level;
    int star;
    int attributeExtra;
    std::string name;
    std::string char_name;
    int vip;
};

struct gameHeroRankings
{
    heroRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//�ؿ���������
struct strongholdRankings
{
    int rank;
    int cid;
    int level;
    int stronghold;
    std::string stronghold_name;  //�ؿ�
    std::string union_name;  //����
    std::string name;    //��ɫ��
    int vip;
};

struct gameStrongholdRankings
{
    strongholdRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//������������
struct copyRankings
{
    int rank;
    int cid;
    int level;
    int copy;
    std::string copy_name;  //����
    std::string union_name;  //����
    std::string name;    //��ɫ��
    int vip;
};

struct gameCopyRankings
{
    copyRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//��������������
struct shenlingRankings
{
    int rank;
    int cid;
    int level;
    int sid;
    std::string union_name;  //����
    std::string name;    //��ɫ��
    int vip;
};

struct gameShenlingRankings
{
    shenlingRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//ս������
struct charAttackRankings
{
    int cid;    //��ɫid
    int rank;    //����
    int level;    //��ɫ�ȼ�
    int attributeExtra;    //ս��(��սӢ��)
    std::string union_name;  //����
    std::string name;    //��ɫ��
    int vip;
};

struct gameCharAttackRankings
{
      charAttackRankings Rankings[iRankingsPage*iRankingsPerPage];
};

class rankingMgr
{
public:
    rankingMgr();
    //��������
    void _updateRankings();
    //��������
    void updateRankings();

    //���½�ɫ����
    void _updateCharRankings();
    //������������
    void _updateSilverRankings();
    //����Ӣ������
    void _updateHeroRankings();
    //���¹ؿ���������
    void _updateStrongholdRankings();
    //���¸�����������
    void _updateCopyRankings();
    //������������������
    void _updateShenlingRankings();
    //����ս������
    void _updateCharAttackRankings();


    //��ý�ɫ����
    int getCharRankings(int page, int cid, json_spirit::Object& robj);
    //�����������
    int getSilverRankings(int page, int cid, json_spirit::Object& robj);
    //��ѯӢ������
    int getHeroRankings(int page, int cid, json_spirit::Object& robj);
    //��ùؿ���������
    int getStrongholdRankings(int page, int cid, json_spirit::Object& robj);
    //��ø�����������
    int getCopyRankings(int page, int cid, json_spirit::Object& robj);
    //��ѯ��������������
    int getShenlingRankings(int page, int cid, json_spirit::Object& robj);
    //���ս������
    int getCharAttackRankings(int page, int cid, json_spirit::Object& robj);
    
private:

    boost::shared_ptr<const gameCharRankings> m_CharRankings;
    boost::shared_ptr<const gameSilverRankings> m_SilverRankings;
    boost::shared_ptr<const gameHeroRankings> m_HeroRankings;
    boost::shared_ptr<const gameStrongholdRankings> m_StrongholdRankings;
    boost::shared_ptr<const gameCopyRankings> m_CopyRankings;
    boost::shared_ptr<const gameShenlingRankings> m_ShenlingRankings;
    boost::shared_ptr<const gameCharAttackRankings> m_CharAttackRankings;
    //�����б�
    boost::shared_ptr<const json_spirit::Array> m_charRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_silverRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_heroRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_strongholdRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_copyRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_shenlingRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_charAttackRankingsPages[iRankingsPage];
    //ҳ����Ϣ
    boost::shared_ptr<const json_spirit::Object> m_charRankingsPageobj[iRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_silverRankingsPageobj[iRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_heroRankingsPageobj[iRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_strongholdRankingsPageobj[iRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_copyRankingsPageobj[iRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_shenlingRankingsPageobj[iRankingsPage];
    boost::shared_ptr<const json_spirit::Object> m_charAttackRankingsPageobj[iRankingsPage];

    volatile int m_updating_charRankings;
    volatile int m_updating_silverRankings;
    volatile int m_updating_heroRankings;
    volatile int m_updating_strongholdRankings;
    volatile int m_updating_copyRankings;
    volatile int m_updating_shenlingRankings;
    volatile int m_updating_charAttackRankings;

    boost::shared_ptr<boost::thread> _update_rankings_threadptr;
};

//��ɫ����
int ProcessGetCharRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Ƹ�����
int ProcessGetSilverRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//Ӣ������
int ProcessGetHeroRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ؿ���������
int ProcessGetStrongholdRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������������
int ProcessGetCopyRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������������
int ProcessGetShenlingRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ɫ����
int ProcessGetCharAttackRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

