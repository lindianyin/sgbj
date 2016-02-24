#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include <boost/thread.hpp>

#include "item.h"

const int iRankingsPage = 10;        //排行榜10页
const int iRankingsPerPage = 10;    //每页10个 共 100

using namespace json_spirit;

//角色排名
struct charRankings
{
    int cid;    //角色id
    int rank;    //排名
    int level;    //角色等级
    int race;    //种族
    std::string union_name;  //公会
    std::string name;    //角色名
    int vip;
};

struct gameCharRankings
{
      charRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//财富排名
struct silverRankings
{
    int cid;    //角色id
    int rank;    //排名
    int silver;    //筹码
    int race;    //种族
    std::string union_name;  //公会
    std::string name;    //角色名
    int vip;
};

struct gameSilverRankings
{
      silverRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//英雄排名
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

//关卡进度排名
struct strongholdRankings
{
    int rank;
    int cid;
    int level;
    int stronghold;
    std::string stronghold_name;  //关卡
    std::string union_name;  //公会
    std::string name;    //角色名
    int vip;
};

struct gameStrongholdRankings
{
    strongholdRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//副本进度排名
struct copyRankings
{
    int rank;
    int cid;
    int level;
    int copy;
    std::string copy_name;  //副本
    std::string union_name;  //公会
    std::string name;    //角色名
    int vip;
};

struct gameCopyRankings
{
    copyRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//神灵塔进度排名
struct shenlingRankings
{
    int rank;
    int cid;
    int level;
    int sid;
    std::string union_name;  //公会
    std::string name;    //角色名
    int vip;
};

struct gameShenlingRankings
{
    shenlingRankings Rankings[iRankingsPage*iRankingsPerPage];
};

//战力排名
struct charAttackRankings
{
    int cid;    //角色id
    int rank;    //排名
    int level;    //角色等级
    int attributeExtra;    //战力(出战英雄)
    std::string union_name;  //公会
    std::string name;    //角色名
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
    //更新排名
    void _updateRankings();
    //更新排名
    void updateRankings();

    //更新角色排名
    void _updateCharRankings();
    //更新银币排名
    void _updateSilverRankings();
    //更新英雄排名
    void _updateHeroRankings();
    //更新关卡进度排名
    void _updateStrongholdRankings();
    //更新副本进度排名
    void _updateCopyRankings();
    //更新神灵塔进度排名
    void _updateShenlingRankings();
    //更新战力排名
    void _updateCharAttackRankings();


    //获得角色排名
    int getCharRankings(int page, int cid, json_spirit::Object& robj);
    //获得银币排名
    int getSilverRankings(int page, int cid, json_spirit::Object& robj);
    //查询英雄排名
    int getHeroRankings(int page, int cid, json_spirit::Object& robj);
    //获得关卡进度排名
    int getStrongholdRankings(int page, int cid, json_spirit::Object& robj);
    //获得副本进度排名
    int getCopyRankings(int page, int cid, json_spirit::Object& robj);
    //查询神灵塔进度排名
    int getShenlingRankings(int page, int cid, json_spirit::Object& robj);
    //获得战力排名
    int getCharAttackRankings(int page, int cid, json_spirit::Object& robj);
    
private:

    boost::shared_ptr<const gameCharRankings> m_CharRankings;
    boost::shared_ptr<const gameSilverRankings> m_SilverRankings;
    boost::shared_ptr<const gameHeroRankings> m_HeroRankings;
    boost::shared_ptr<const gameStrongholdRankings> m_StrongholdRankings;
    boost::shared_ptr<const gameCopyRankings> m_CopyRankings;
    boost::shared_ptr<const gameShenlingRankings> m_ShenlingRankings;
    boost::shared_ptr<const gameCharAttackRankings> m_CharAttackRankings;
    //排名列表
    boost::shared_ptr<const json_spirit::Array> m_charRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_silverRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_heroRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_strongholdRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_copyRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_shenlingRankingsPages[iRankingsPage];
    boost::shared_ptr<const json_spirit::Array> m_charAttackRankingsPages[iRankingsPage];
    //页面信息
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

//角色排行
int ProcessGetCharRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//财富排行
int ProcessGetSilverRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//英雄排行
int ProcessGetHeroRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//关卡进度排行
int ProcessGetStrongholdRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//副本进度排行
int ProcessGetCopyRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//神灵塔进度排行
int ProcessGetShenlingRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//角色排行
int ProcessGetCharAttackRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

