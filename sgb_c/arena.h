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
    uint64_t id;    //战报唯一id
    int input;        //战报时间
    int tid;        //挑战的角色id
    int result;    //结果 1 胜利 2失败
    std::string tname;    //挑战的角色名
    int type;    //记录类型1攻击2被攻击
    int rank_result;//结果排名
};

struct arenaRankRewards
{
    int rank;
    std::list<Item> reward;//奖励
};

struct arenaTitle
{
    uint64_t id;    //战报唯一id
    int input;
    std::string atk_name;    //攻击角色名
    std::string def_name;    //防守的角色名
};

//基础商店物品
struct baseArenaGoods
{
    int needscore;
    Item reward;//奖励
};

struct CharData;

struct CharArenaData
{
private:
    boost::shared_ptr<CharData> m_charactor;    //对应的角色
public:
    CharArenaData(int cid);
    boost::shared_ptr<CharData> getCharData();
    boost::shared_ptr<std::list<boost::shared_ptr<arenaRecord> > > m_arena_records;    //竞技战斗历史战报
    boost::shared_ptr<std::list<boost::shared_ptr<arenaRecord> > > getArenaRecords();
    int addArenaRecord(uint64_t id, int result, int target, const std::string& target_name, int type, int rank_result);
    int save(bool force_save);
    void toObj(json_spirit::Object& robj);

    int m_cid;
    bool m_needSave;
    int m_rank;        //排名
    int m_under_attack;//正在被谁挑战
    int m_attack_who;    //正在挑战谁
    int m_total_arena;    //总竞技次数
    int m_wins;            //连胜次数
    time_t m_save_time;        //存盘时间
    int m_trend;                //排名上升下降趨勢  0不變 1上升 2下降
};

class arenaMgr
{
public:
    arenaMgr();
    boost::shared_ptr<CharArenaData> getArenaData(int cid);
    bool canChallege(int rank1, int rank2);
    //购买挑战次数
    int buyChallenge(session_ptr& psession, int cid, json_spirit::Object& robj);
    int querySelfInfo(int cid, json_spirit::Object& robj);
    int queryArenaList(int cid, json_spirit::Object& robj);
    boost::shared_ptr<CharArenaData> addCharactor(int cid, int rank, int totalArena, int wins);
    //清除冷却时间
    int clearCD(int cid, json_spirit::Object& robj);
    //前面的挑戰對手
    CharArenaData* getPreChar(int rank);
    //後面的挑戰對手
    CharArenaData* getNextChar(int rank);

    //查询排名奖励
    int QueryRankRewards(int rank1, int rank2, json_spirit::Object& robj);
    //排名奖励
    int getSeasonAwards(int rank, json_spirit::Array& reward_list);
    int seasonAwards();
    void setArenaFreeTimes(int times);
    int combatResult(chessCombat* pCombat);
    //竞技场兑换商店
    int getList(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int getGood(int cid, json_spirit::mObject& o, json_spirit::Object& robj);

private:
    boost::unordered_map<int, boost::shared_ptr<CharArenaData> > m_arena_maps;        //竞技信息
    std::vector<boost::shared_ptr<CharArenaData> > m_arena_rank;      //竞技场内的角色排名
    //榜首战信息
    boost::shared_ptr<arenaTitle> m_arena_title;
    uint64_t m_top_battle_id;

    std::vector<arenaRankRewards> m_rewards;        //排名奖励
    int m_last_rank;        //最后有奖励的
    std::vector<int> m_last_top;        //上次排名有称号的玩家
    std::vector<baseArenaGoods> m_goods;        //商品列表
};

//获得竞技排名列表
int ProcessQueryArenaRankList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询自己的竞技信息
int ProcessQueryArenaInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询排行奖励
int ProcessQueryRankRewards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//购买挑战次数
int ProcessBuyArena(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//竞技积分商品
int ProcessQueryArenaGoodsList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//领取竞技积分商品
int ProcessGetArenaGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

