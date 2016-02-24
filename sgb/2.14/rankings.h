#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "json_spirit.h"
#include <boost/thread.hpp>

#include "base_item.h"

#ifdef VN_SERVER
const int iCharRankingsPage = 25;    //君主榜页数
const int iHeroRankingsPage = 25;    //武将排行榜页数
#else
const int iCharRankingsPage = 8;    //君主榜页数
const int iHeroRankingsPage = 8;    //武将排行榜页数
#endif
const int iRankingsPage = 8;        //排行榜25页
const int iRankingsPerPage = 8;    //每页8个 共 200

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

//角色排名
struct charRankings
{
    int cid;    //角色id
    int rank;    //排名
    int level;    //角色等级
    int camp;    //阵营
    int olevel;    //官职等级
    json_spirit::Array nicks;
    std::string name;    //角色名
    std::string corps;    //军团名
    std::string offical;//官职名
};

struct splsCharRankings
{
      charRankings Rankings[iCharRankingsPage*iRankingsPerPage];
};

//英雄排名
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
    int rank;    //排名
    int cid;    //角色id
    int camp;    //阵营
    int score;    //积分
    int olevel;    //官职等级
    std::string name;    //角色名
    std::string corps;        //军团名
    std::string offical;    //官职名
};

struct splsLotteryScoreRankings
{
    lotteryScoreRankings Rankings[iRankingsPage*iRankingsPerPage];
};

struct eliteRankings
{
    int rank;    //排名
    int cid;    //角色id
    int level;    //角色等级
    int attack;    //战力
    int elite_id;    //精英id
    json_spirit::Array nicks;
    std::string name;    //角色名
    std::string elite_name;        //精英名
};

struct splsEliteRankings
{
    eliteRankings Rankings[iRankingsPage*iRankingsPerPage];
};

struct prestigeGetRankings
{
    int rank;    //排名
    int cid;    //角色id
    int level;  //等级
    int score;    //声望获得值
    json_spirit::Array nicks;
    std::string name;    //角色名
};

struct splsPrestigeGetRankings
{
    prestigeGetRankings Rankings[iRankingsPage*iRankingsPerPage];
};

struct attackRankings
{
    int rank;    //排名
    int cid;    //角色id
    int level;  //等级
    int score;    //战力
    json_spirit::Array nicks;
    std::string name;    //角色名
    std::string corps;        //军团名
    std::string offical;    //官职名
};

struct splsAttackRankings
{
    attackRankings Rankings[iRankingsPage*iRankingsPerPage];
};

struct ZSTRankings
{
    int rank;    //排名
    int cid;    //角色id
    int level;  //等级
    int attack;    //战力
    int score;    //星级
    json_spirit::Array nicks;
    std::string name;    //角色名
    std::string corps;        //军团名
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

//排名活动的奖励
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

//排名活动
struct rankings_event
{
    int id;
    int type;
    std::string mail_title;
    std::string mail_content;

    std::list<rankings_event_award> rankings_list;
};

//检查排行榜活动是否开了
void checkRankingsEvent();
//真正给奖励
void giveRankingsEventReward(rankings_event* pE);

class splsRankings
{
public:
    splsRankings();
    
    //更新排名
    void updateRankings(int);

    //更新角色排名
    void _updateCharRankings();
    //更新英雄排名
    void _updateHeroRankings();
    //更新梅花易數積分排名
    void _updateLotteryScoreRankings();
    //更新精英战进度排名
    void _updateEliteRankings();
    //更新声望排行
    void _updatePrestigeGetRankings();
    //更新战力排行
    void _updateAttackRankings();
    //更新战神台排行
    void _updateZSTRankings();

    //更新排名
    void _updateRankings();

    //获得角色排名
    int getCharRankings(int page, int cid, json_spirit::Object& robj);
    //查询英雄排名
    int getHeroRankings(int page, int cid, json_spirit::Object& robj);
    //查询梅花易數排名
    int getLotteryRankings(int page, int cid, json_spirit::Object& robj);
    //查询精英战排名
    int getEliteRankings(int page, int cid, json_spirit::Object &robj);
    //查询声望获得排名
    int getPrestigeRankings(int page, int cid, json_spirit::Object &robj);
    //查询战力排名
    int getAttackRankings(int page, int cid, json_spirit::Object &robj);
    //查询战神台排名
    int getZSTRankings(int page, int cid, json_spirit::Object &robj);

    //更新排行榜活动中的cid字段
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

