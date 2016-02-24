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

//竞技购买金币上限
const int iRaceBuyGoldMax = 20;

struct raceRecord
{
    uint64_t id;    //战报唯一id
    int input;        //战报时间
    int tid;        //挑战的角色id
    int result;    //结果 1 胜利 2失败
    std::string tname;    //挑战的角色名
    int type;    //记录类型1攻击2被攻击
    int rank_result;//结果排名
};

struct raceRankRewards
{
    int rank;
    double silver;
    int prestige;
};

struct raceTitle
{
    uint64_t id;    //战报唯一id
    int input;
    std::string atk_name;    //攻击角色名
    std::string def_name;    //防守的角色名
};

//基础商店物品
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
    int type;// 1 资源(金币、军令)2道具
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
    boost::shared_ptr<CharData> m_charactor;    //对应的角色
public:
    CharRaceData(int cid, int mapid, int level);

    CharData* getChar();

    boost::shared_ptr<CharData> getCharData();
    
    int m_cid;

    bool m_needSave;
    int m_rank;        //排名

    int m_mapid;    //地图id
    int m_under_attack;//正在被谁挑战
    int m_attack_who;    //正在挑战谁

    int m_race_times;    //竞技次数

    int m_total_race;    //总竞技次数

    int m_wins;            //连胜次数

    time_t m_race_cd_time;        //竞技冷却时间

    time_t m_save_time;        //存盘时间

    int m_trend;                //排名上升下降趨勢  0不變 1上升 2下降

    int m_score;        //本日竞技积分
    int m_total_score;    //总积分

    int save(bool force_save);

    boost::shared_ptr<std::list<boost::shared_ptr<raceRecord> > > m_race_records;    //竞技战斗历史战报
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
    //购买挑战次数
    int buyChallenge(session_ptr& psession, int cid, json_spirit::Object& robj);
    
    int challengeResult(Combat* pCombat);
    int querySelfInfo(int cid, json_spirit::Object&);
    int queryRaceList(int cid, int type, int counts, json_spirit::Object& robj);
    boost::shared_ptr<CharRaceData> addCharactor(int cid, int level, int mapid, int rank, time_t coolTime, int raceTimes, int totalRace, int wins, int score, int total_score);
    //从竞技场移除角色(被删除)
    int removeCharactor(int cid);

    int updateZone(int cid);    //搬迁竞技场
    //清除冷却时间
    int clearCD(int cid, json_spirit::Object& robj);

    int saveAll();
    int resetAll();
    int updateAll();
    //每年冬季的晚上23点颁奖
    int yearAwards();

    int deleteChar(int cid);

    //前面的挑戰對手
    CharRaceData* getPreRacer(int rank);
    //後面的挑戰對手
    CharRaceData* getNextRacer(int rank);
    //獲取競技排名榜
    int getTop20(json_spirit::Object& robj);

    //查询排名奖励
    int QueryRankRewards(int rank1, int rank2, json_spirit::Object& robj);

    //排名奖励
    int getYearAwards(int rank, int& prestige, int& silver);

    int getRandomServantList(int cid, int mapid, std::vector<int>& list);
    
    int getTopBattleId(){return m_top_battle_id;}

    void setRaceFreeTimes(int times);

    static RaceMgr* getInstance();
    //竞技场兑换商店
    int getList(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int buy(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int left_score(int race_time, int& goal_times);

private:
    static RaceMgr* m_handle;
    boost::unordered_map<int, boost::shared_ptr<CharRaceData> > m_race_maps;        //竞技信息
    std::vector<boost::shared_ptr<CharRaceData> > m_race_rank[max_map_id];      //竞技场内的角色排名
    boost::unordered_map<int,boost::shared_ptr<raceTitle> > m_race_title;
    uint64_t m_top_battle_id;

    boost::shared_ptr<CharRaceData> m_top20[20];    //競技排名前20

    std::vector<raceRankRewards> m_rewards;        //排名奖励
    int m_last_rank;        //最后有奖励的

    std::vector<int> m_last_top_five;        //上次排名前5名
    
    std::vector<baseRaceGoods> m_goods;        //商品列表
};

