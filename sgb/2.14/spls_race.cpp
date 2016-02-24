#include "statistics.h"
#include "servant.h"

#include "spls_race.h"
#include "utils_all.h"
#include "data.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "combat.h"

#include "utils_lang.h"
#include <syslog.h>
#include "daily_task.h"
#include "singleton.h"
#include "relation.h"
#include "first_seven_goals.h"
#include "rewards.h"
#include "qq_invite.h"
#include "new_event.h"

static int iDefaultRaceFreeTimes = 15;
volatile int iRaceFreeTimes = iDefaultRaceFreeTimes;    //免费15次挑战

inline int getRaceIdx(int mapid)
{
    return 0;
}

//竞技场实际收益
void arenaRealReward(int& get);

extern void InsertSaveDb(const std::string& sql);
extern void InsertCombat(Combat* pcombat);
extern void InsertSaveCombat(Combat* pcombat);

extern std::string strRaceNotifyMailTitle;
extern std::string strRaceNotifyMailContent;

//初始化竞技战斗
extern Combat* createRaceCombat(int cid, int tid, int& ret);

Database& GetDb();

CharRaceData::CharRaceData(int cid, int mapid, int level)
:m_cid(cid)
{
    m_wins = 0;
    m_needSave = false;
    //m_charactor = GeneralDataMgr::getInstance()->GetCharData(cid);
}

CharData* CharRaceData::getChar()
{
    if (m_charactor.get())
    {
        return m_charactor.get();
    }
    else
    {
        m_charactor = GeneralDataMgr::getInstance()->GetCharData(m_cid);
        return m_charactor.get();
    }
}

boost::shared_ptr<CharData> CharRaceData::getCharData()
{
    if (m_charactor.get())
    {
        return m_charactor;
    }
    else
    {
        m_charactor = GeneralDataMgr::getInstance()->GetCharData(m_cid);
        return m_charactor;
    }
}

int CharRaceData::save(bool force_save)
{
    if (force_save || (time(NULL) > m_save_time && m_needSave))
    {
        m_needSave = false;
        m_save_time = time(NULL) + 300;
        InsertSaveDb("replace into char_race (cid,rank,coolTime,raceTimes,totalRace,mapid,wins,score,total_score,updateTime) values ("
                        + LEX_CAST_STR(m_cid) + "," + LEX_CAST_STR(m_rank)
                        + "," + LEX_CAST_STR(m_race_cd_time)
                        + "," + LEX_CAST_STR(m_race_times)
                        + "," + LEX_CAST_STR(m_total_race)
                        + "," + LEX_CAST_STR(m_mapid)
                        + "," + LEX_CAST_STR(m_wins)
                        + "," + LEX_CAST_STR(m_score)
                        + "," + LEX_CAST_STR(m_total_score)
                        + ",unix_timestamp()"
                        + ")"
                );
    }
    return HC_SUCCESS;
}

boost::shared_ptr<std::list<boost::shared_ptr<raceRecord> > > CharRaceData::getRaceRecords()
{
    if (!m_race_records.get())
    {
        m_race_records.reset(new std::list<boost::shared_ptr<raceRecord> >);
        Query q(GetDb());
        q.get_result("select id,input,result,attacker,attackerName,defender,defenderName,extra1,extra2 from battle_records where (attacker=" + LEX_CAST_STR(m_cid) + " or defender=" + LEX_CAST_STR(m_cid) + ") and type=2 order by id desc limit 5");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            uint64_t id = q.getubigint();
            int input = q.getval();
            int result = q.getval();
            int attacker = q.getval();
            std::string attacker_name = q.getstr();
            int defender = q.getval();
            std::string defender_name = q.getstr();
            int extra1 = q.getval();
            int extra2 = q.getval();
            boost::shared_ptr<raceRecord> record(new raceRecord);
            if (m_cid == attacker)
            {
                record->id = id;
                record->input = input;
                record->result = result;
                record->tid = defender;
                record->tname = defender_name;
                record->type = 1;
                record->rank_result = extra1;
            }
            else if(m_cid == defender)
            {
                record->id = id;
                record->input = input;
                record->result = result;
                record->tid = attacker;
                record->tname = attacker_name;
                record->type = 2;
                record->rank_result = extra2;
            }
            m_race_records->insert(m_race_records->begin(), record);
        }
        q.free_result();
    }
    return m_race_records;
}

int CharRaceData::addRaceRecord(uint64_t id, int result, int target, const std::string& target_name, int type, int rank_result)
{
    boost::shared_ptr<std::list<boost::shared_ptr<raceRecord> > > mygets = getRaceRecords();
    if (!mygets.get())
    {
        ERR();
        return HC_ERROR;
    }

    m_needSave = true;
    boost::shared_ptr<raceRecord> record(new raceRecord);
    record->id = id;
    record->input = time(NULL);
    record->result = result;
    record->tid = target;
    record->tname = target_name;
    record->type = type;
    record->rank_result = rank_result;
    m_race_records->push_back(record);

    while (mygets->size() > 5)
    {
        boost::shared_ptr<raceRecord> rpt = mygets->front();
        if (rpt.get() && rpt->id != RaceMgr::getInstance()->getTopBattleId())
        {
            //顶出去的战报修改为不存档
            InsertSaveDb("update battle_records set archive=archive-1 where  archive>0 and id=" + LEX_CAST_STR(rpt->id));
        }
        mygets->pop_front();
    }
    return HC_SUCCESS;
}

RaceMgr* RaceMgr::m_handle = NULL;

RaceMgr::RaceMgr()
{

}

RaceMgr* RaceMgr::getInstance()
{
    if (m_handle == NULL)
    {
        time_t time_start = time(NULL);
        cout<<"RaceMgr::getInstance()..."<<endl;
        m_handle = new RaceMgr();
        m_handle->reload();
        cout<<"RaceMgr::getInstance() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

//加载竞技数据
int RaceMgr::reload()
{
    cout<<"************** RaceMgr::reload() ****************" <<endl;
    //for (int i = max_map_id; i >= 1; --i)
    {
        Query q(GetDb());

        q.get_result("select rank,silver,prestige from base_race_rankings_rewards where 1 order by rank");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            raceRankRewards rrr;
            rrr.rank = q.getval();
            rrr.silver = q.getnum();
            rrr.prestige = q.getval();

            m_last_rank = rrr.rank;
            m_rewards.push_back(rrr);
        }
        q.free_result();

        if (!q.execute("delete FROM  char_race WHERE cid NOT IN (SELECT id FROM charactors)"))
        {
            CHECK_DB_ERR(q);
        }

        q.get_result("select mapid,atk_name,def_name,battle_id,input from char_race_title where 1");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            boost::shared_ptr<raceTitle> p_rt;
            p_rt.reset(new raceTitle);
            int mapid = q.getval();
            p_rt->atk_name = q.getstr();
            p_rt->def_name = q.getstr();
            p_rt->id = q.getval();
            p_rt->input = q.getval();
            m_race_title[mapid] = p_rt;
            m_top_battle_id = p_rt->id;
        }
        q.free_result();

        //积分商店
        q.get_result("select good_id,nums,price from base_race_shop where good_type=1 order by id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            baseRaceGoods brg;
            brg.type = 1;
            brg.id = q.getval();
            switch (brg.id)
            {
                case 1:
                    brg.name = strGold;
                    break;
                case 2:
                    brg.name = strSilver;
                    break;
                case 3:
                    brg.name = strPrestige;
                    break;
            }
            brg.num = q.getval();
            brg.price = q.getval();
            m_goods.push_back(brg);
        }
        q.free_result();
        q.get_result("select bt.id,bt.spic,bt.name,bt.memo,bt.quality,brs.nums,brs.price from base_treasures as bt left join base_race_shop as brs on bt.id = brs.good_id where brs.good_type=2 order by brs.id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            baseRaceGoods brg;
            brg.type = 2;
            brg.id = q.getval();
            brg.spic = q.getval();
            brg.name = q.getstr();
            brg.memo = q.getstr();
            brg.quality = q.getval();
            brg.num = q.getval();
            brg.price = q.getval();
            m_goods.push_back(brg);
        }
        q.free_result();

        //q.get_result("select cr.cid,c.level,cr.rank,cr.coolTime,cr.raceTimes,cr.openTimes,cr.totalRace,cr.totalOpen,cr.wins from char_race as cr left join charactors as c on cr.cid=c.id where cr.mapid=" + LEX_CAST_STR(i) + " order by cr.rank");
        //这里需要比较长的时间，所以最好放在最后面，以免数据库连接超时
        q.get_result("select cr.mapid,cr.cid,c.level,cr.rank,cr.coolTime,cr.raceTimes,cr.totalRace,cr.wins,cr.score,cr.total_score from char_race as cr left join charactors as c on cr.cid=c.id where 1 order by cr.rank,cr.updateTime desc");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int mapid = q.getval();
            int cid = q.getval();
            int level = q.getval();
            int rank = q.getval();
            time_t coolTime = q.getval();
            int raceTimes = q.getval();
            int totalRace = q.getval();
            int wins = q.getval();
            int score = q.getval();
            int total_score = q.getval();
            addCharactor(cid, level, mapid, rank, coolTime, raceTimes, totalRace, wins, score, total_score);
        }
        q.free_result();
    }
#ifndef OLD_RACE
    for (size_t i = 0; i < 20; ++i)
    {
        if ((i+1) <= m_race_rank[0].size())
        {
            m_top20[i] = m_race_rank[0][i];
        }
    }
#endif

    iRaceFreeTimes = GeneralDataMgr::getInstance()->getInt("race_free_times", iDefaultRaceFreeTimes);

    //读取上次的前5名
    std::string top5 = GeneralDataMgr::getInstance()->getStr("last_arena_top_five");
    if (top5 != "")
    {
        json_spirit::Value v;
        json_spirit::read(top5, v);
        if (v.type() == json_spirit::array_type)
        {
            json_spirit::Array& a = v.get_array();
            for (json_spirit::Array::iterator it = a.begin(); it != a.end(); ++it)
            {
                json_spirit::Value v2 = *it;
                if (v2.type() == json_spirit::int_type)
                {
                    m_last_top_five.push_back(v2.get_int());
                }
                else
                {
                    m_last_top_five.clear();
                    break;
                }
            }
        }
    }

    if (m_last_top_five.size())
    {
        for (int i = 0; i < m_last_top_five.size(); ++i)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_last_top_five[i]).get();
            if (pc)
            {
                pc->m_nick.add_nick(i+1);
                pc->SaveNick();
            }
        }
    }
    return HC_SUCCESS;
}

int RaceMgr::getAction(CharData* cdata, json_spirit::Array& elist)
{
    //竞技的开启状态
    if (cdata->m_raceOpen)
    {
        boost::shared_ptr<CharRaceData> rd = RaceMgr::getInstance()->getRaceData(cdata->m_id);

        if (!rd.get() || !rd->getChar())
        {
            if (cdata->m_raceOpen)
            {
                RaceMgr::getInstance()->updateZone(cdata->m_id);
                rd = getRaceData(cdata->m_id);
            }
        }
        if (rd.get() && rd->getChar())
        {
            json_spirit::Object obj;
            obj.clear();
            obj.push_back( Pair("type", top_level_event_race) );
            obj.push_back( Pair("active", 0) );
            int buy_times = cdata->queryExtraData(char_data_type_daily, char_data_buy_race);
            int leftTimes = iRaceFreeTimes + buy_times - rd->m_race_times;
            if (leftTimes < 0)
            {
                leftTimes = 0;
            }
            obj.push_back( Pair("leftNums", leftTimes) );
            elist.push_back(obj);
        }
    }
    return 1;
}

boost::shared_ptr<CharRaceData> RaceMgr::addCharactor(int cid, int level, int mapid, int rank, time_t coolTime, int raceTimes, int totalRace, int wins, int score, int total_score)
{
    boost::unordered_map<int, boost::shared_ptr<CharRaceData> >::iterator it = m_race_maps.find(cid);        //竞技信息
    if (it != m_race_maps.end() && it->second.get() != NULL)
    {
        ERR();
        return it->second;
    }
    else
    {
        boost::shared_ptr<CharRaceData> rd(new CharRaceData(cid, mapid, level));
        m_race_maps[cid] = rd;
        rd->m_mapid = mapid;
        rd->m_under_attack = 0;
        rd->m_attack_who = 0;
        rd->m_race_times = raceTimes;
        rd->m_total_race = totalRace;
        rd->m_race_cd_time = coolTime;
        rd->m_wins = wins;
        rd->m_trend = 0;
        rd->m_score = score;
        rd->m_total_score = total_score;
        //if (0 == rank)
        {
            int raceIdx = getRaceIdx(mapid);
            m_race_rank[raceIdx].push_back(rd);
            rd->m_rank = m_race_rank[raceIdx].size();
        }
        if (rank != rd->m_rank)
        {
            rd->save(true);
        }
        if (rd->m_rank >= 1 && rd->m_rank <= 20)
        {
            m_top20[rd->m_rank - 1] = rd;
        }
        return rd;
    }
}

//从竞技场移除角色(被删除)
int RaceMgr::removeCharactor(int cid)
{
    m_race_maps.erase(cid);
    return HC_SUCCESS;
}

boost::shared_ptr<CharRaceData> RaceMgr::getRaceData(int cid)
{
    boost::unordered_map<int, boost::shared_ptr<CharRaceData> >::iterator it = m_race_maps.find(cid);        //竞技信息
    if (it != m_race_maps.end())
    {
        it->second->getChar();
        it->second->save(false);
        return it->second;
    }
    else
    {
        boost::shared_ptr<CharRaceData> rd;
        return rd;
    }
}

//查询自己的竞技场信息
int RaceMgr::querySelfInfo(int cid, json_spirit::Object& robj)
{
    time_t time_now = time(NULL);
    boost::shared_ptr<CharRaceData> rd = getRaceData(cid);
    if (!rd.get() || !rd->getChar())
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (cdata.get() && cdata->m_raceOpen)
        {
            RaceMgr::getInstance()->updateZone(cdata->m_id);
            rd = getRaceData(cid);
            if (!rd.get() || !rd->getChar())
            {
                return HC_ERROR;
            }
        }
        else
        {
            return HC_ERROR;
        }
    }

    CharData* pc = rd->getChar();
    if (!pc)
    {
        return HC_ERROR;
    }
    //挑战记录
    {
        json_spirit::Array history_list;
        boost::shared_ptr<std::list<boost::shared_ptr<raceRecord> > > totalraces = rd->getRaceRecords();
        std::list<boost::shared_ptr<raceRecord> >::reverse_iterator it = (*totalraces).rbegin();
        while (it != (*totalraces).rend())
        {
            json_spirit::Object o;
            o.push_back( Pair("id", (*it)->id));
            o.push_back( Pair("input", (time_now-(*it)->input)/60));
            o.push_back( Pair("name", (*it)->tname));
            o.push_back( Pair("result", (*it)->result));
            o.push_back( Pair("type", (*it)->type));
            o.push_back( Pair("rank_result", (*it)->rank_result));
            history_list.push_back(o);
            ++it;
        }
        robj.push_back( Pair("historyList", history_list) );
    }
    boost::shared_ptr<raceTitle> p_rt = m_race_title[getRaceIdx(rd->m_mapid)];
    if (p_rt.get())
    {
        json_spirit::Object title;
        title.push_back( Pair("id", p_rt->id));
        title.push_back( Pair("input", (time_now-p_rt->input)/60));
        title.push_back( Pair("att_name", p_rt->atk_name));
        title.push_back( Pair("def_name", p_rt->def_name));
        robj.push_back( Pair("title", title) );
    }
    json_spirit::Object raceInfo;
    raceInfo.push_back( Pair("rank", rd->m_rank) );

    int buy_times = pc->queryExtraData(char_data_type_daily, char_data_buy_race);
    int buygold = 2 * buy_times + 2;
    if (buygold > iRaceBuyGoldMax)
    {
        buygold = iRaceBuyGoldMax;
    }
    raceInfo.push_back( Pair("buyGold", buygold) );

    int ling = iRaceFreeTimes+ buy_times - rd->m_race_times;
    if (ling < 0)
    {
        ling = 0;
    }
    raceInfo.push_back( Pair("ling", ling) );
    int cd = rd->m_race_cd_time-time(NULL);
    raceInfo.push_back( Pair("coolTime", (cd > 0)?cd:0) );
    raceInfo.push_back( Pair("win", rd->m_wins) );
    raceInfo.push_back( Pair("trend", rd->m_trend) );
    int max_rank = pc->queryExtraData(char_data_type_normal, char_data_race_maxrank);
    if (max_rank == 0 || max_rank > rd->m_rank)
    {
        max_rank = rd->m_rank;
        pc->setExtraData(char_data_type_normal, char_data_race_maxrank, max_rank);
    }
    raceInfo.push_back( Pair("max_rank", max_rank) );
    raceInfo.push_back( Pair("prestige", pc->prestige()) );
    raceInfo.push_back( Pair("silver", pc->silver()) );
    raceInfo.push_back( Pair("score", rd->m_score) );
    raceInfo.push_back( Pair("total", rd->m_total_score) );

    robj.push_back( Pair("raceInfo", raceInfo) );

    return HC_SUCCESS;
}

void RaceMgr::setRaceFreeTimes(int times)
{
    if (times == 0)
    {
        times = iDefaultRaceFreeTimes;
    }
    if (times > 0)
    {
        iRaceFreeTimes = times;
        GeneralDataMgr::getInstance()->setInt("race_free_times", iRaceFreeTimes);
    }
}

inline int _getPreRancer(int rank)
{
    if (rank >= 1000)
    {
        rank = 990 + (rank % 10);
    }
    else if (rank >= 500)
    {
        rank = rank - 50;
    }
    else if(rank >= 300)
    {
        rank = rank - 10;
    }
    else if (rank >= 100)
    {
        rank = rank - 5;
    }
    else if (rank <= 1)
    {
        rank = 0;
    }
    else
    {
        rank = rank - 1;
    }
    return rank;
}

//前面的挑戰對手
CharRaceData* RaceMgr::getPreRacer(int rank)
{
    if (rank == 1)
    {
        return NULL;
    }
    else
    {
        if (rank > (int)m_race_rank[0].size())
        {
            return NULL;
        }
        else
        {
            if (rank >= 10000)
            {
                rank = rank - 500;
            }
            else if (rank >= 5000)
            {
                rank = rank - 200;
            }
            else if (rank >= 1000)
            {
                rank = 990 + (rank % 10);
            }
            else if (rank >= 500)
            {
                rank = rank - 50;
            }
            else if (rank >= 300)
            {
                rank = rank - 10;
            }
            else if (rank >= 100)
            {
                rank = rank - 5;
            }
            else
            {
                rank = rank - 1;
            }
            return m_race_rank[0][rank - 1].get();
        }
    }
}

//後面的挑戰對手
CharRaceData* RaceMgr::getNextRacer(int rank)
{
    rank = rank + 1;
    if (rank > (int)m_race_rank[0].size())
    {
        return NULL;
    }
    else
    {
        return m_race_rank[0][rank - 1].get();
    }
}

//能否挑戰
bool RaceMgr::canChallege(int rank1, int rank2)
{
#ifdef OLD_RACE
    return (rank2 >= (rank1 - 10) && rank2 <= (rank1 + 10));
#else
    if (rank1 == rank2)
    {
        return false;
    }
    if (rank1 < 100)
    {
        int rank_s = rank1 - 10;
        int rank_e = 0;
        if (rank_s >= 1)
        {
            rank_e = rank1 - 1;
        }
        else
        {
            rank_s = 1;
            rank_e = 11;
        }
        return (rank2 >= rank_s && rank2 <= rank_e);
    }
    else
    {
        if (rank1 < rank2)
        {
            return false;
        }
        int min_rank = rank1;
        for (int i = 1; i <= 9; ++i)
        {
            min_rank = _getPreRancer(min_rank);
        }
        return (rank2 >= min_rank);
    }
#endif
}

//查询前后10名
int RaceMgr::queryRaceList(int cid, int type, int counts, json_spirit::Object& robj)
{
    boost::shared_ptr<CharRaceData> rd = getRaceData(cid);
    if (!rd.get() || !rd->getChar())
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (cdata.get() && cdata->m_raceOpen)
        {
            RaceMgr::getInstance()->updateZone(cdata->m_id);
            rd = getRaceData(cid);
            if (!rd.get() || !rd->getChar())
            {
                return HC_ERROR;
            }
        }
        else
        {
            return HC_ERROR;
        }
    }
    json_spirit::Array raceRankList;
    json_spirit::Object info;
#ifdef OLD_RACE
    int ranks = rd->m_rank - 10;
    int ranke = rd->m_rank - 1;
    if (type == 2)
    {
        ranks = rd->m_rank + 1;
        ranke = rd->m_rank + 10;
    }
    if (ranks <= 0)
    {
        ranks = 1;
    }
    std::vector<boost::shared_ptr<CharRaceData> > & race_rank = m_race_rank[getRaceIdx(rd->m_mapid)];
    if (ranke > (int)race_rank.size())
    {
        ranke = race_rank.size();
    }
    if (ranke >= 1 && ranks <= (int)race_rank.size())
    {
        for (int i = ranks; i <= ranke; ++i)
        {
            CharRaceData *pRace = race_rank[i-1].get();
            if (pRace)
            {
                CharData* cd = pRace->getChar();
                if (!cd)
                {
                    ERR();
                    cout<<"cid:"<<pRace->m_cid<<endl;
                    continue;
                }
                json_spirit::Object o;
                o.push_back( Pair("id", cd->m_id) );
                o.push_back( Pair("group", cd->m_camp) );
                o.push_back( Pair("rank", i) );
                o.push_back( Pair("level", cd->m_level) );
                o.push_back( Pair("name", cd->m_name) );
                o.push_back( Pair("spic", cd->m_spic) );
                o.push_back( Pair("gender", cd->m_gender) );

                raceRankList.push_back(o);
            }
        }
    }
#else
    CharRaceData* prd = rd.get();
    CharRaceData* pRace = getPreRacer(prd->m_rank);
    while (raceRankList.size() < 9 && pRace)
    {
        CharData* cd = pRace->getChar();
        if (!cd)
        {
            ERR();
            cout<<"cid:"<<pRace->m_cid<<endl;
            continue;
        }
        json_spirit::Object o;
        o.push_back( Pair("id", cd->m_id) );
        o.push_back( Pair("group", cd->m_camp) );
        o.push_back( Pair("rank", pRace->m_rank) );
        o.push_back( Pair("level", cd->m_level) );
        o.push_back( Pair("name", cd->m_name) );
        o.push_back( Pair("spic", cd->m_spic) );
        o.push_back( Pair("gender", cd->m_gender) );
        o.push_back( Pair("change", cd->getChangeSpic()) );
        o.push_back( Pair("attack", cd->getAttack(0)) );

        raceRankList.insert(raceRankList.begin() ,o);

        pRace = getPreRacer(pRace->m_rank);
    }
    if (raceRankList.size() < 10)
    {
        pRace = getNextRacer(prd->m_rank - 1);//玩家自身也显示在内
        while (raceRankList.size() < 10 && pRace)
        {
            CharData* cd = pRace->getChar();
            if (!cd)
            {
                ERR();
                cout<<"cid:"<<pRace->m_cid<<endl;
                continue;
            }
            json_spirit::Object o;
            o.push_back( Pair("id", cd->m_id) );
            o.push_back( Pair("group", cd->m_camp) );
            o.push_back( Pair("rank", pRace->m_rank) );
            o.push_back( Pair("level", cd->m_level) );
            o.push_back( Pair("name", cd->m_name) );
            o.push_back( Pair("spic", cd->m_spic) );
            o.push_back( Pair("gender", cd->m_gender) );
            o.push_back( Pair("change", cd->getChangeSpic()) );
            o.push_back( Pair("attack", cd->getAttack(0)) );
            if (pRace->m_rank == prd->m_rank)
            {
                o.push_back( Pair("self", 1) );
            }

            raceRankList.push_back(o);
            pRace = getNextRacer(pRace->m_rank);
        }
    }
#endif
    time_t t_now = time(NULL);
    time_t t_refresh = GeneralDataMgr::getInstance()->GetRaceRewardTime();
    int leftTime = t_refresh - t_now;
    if (leftTime <= 0)
        leftTime += 3 * 86400;
    info.push_back( Pair("leftTime", leftTime) );
    info.push_back( Pair("rank", rd->m_rank) );
    json_spirit::Object getobj;
    int prestige = 0;
    int silver = 0;
    getYearAwards(rd->m_rank, prestige, silver);
    getobj.push_back( Pair("prestige", prestige) );
    //getobj.push_back( Pair("ling", ling) );
    getobj.push_back( Pair("silver", silver) );
    info.push_back( Pair("get", getobj) );
    robj.push_back( Pair("raceRankList", raceRankList) );
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

//竞技挑战
int RaceMgr::challenge(session_ptr& psession, int cid, int target, json_spirit::Object& robj)
{
    //cout<<"********************RaceMgr::challenge("<<cid<<","<<target<<")*********************"<<endl;
    if (cid == target)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharRaceData> rd = getRaceData(cid);
    if (!rd.get() || !rd->getChar())
    {
        ERR();
        return HC_ERROR;
    }

    boost::shared_ptr<CharRaceData> td = getRaceData(target);
    if (!td.get() || !td->getChar())
    {
        ERR();
        return HC_ERROR;
    }
    CharData* pc = rd->getChar();
    CharData* tc = td->getChar();

    int cd_sec = rd->m_race_cd_time - time(NULL);
    //挑战方的冷却时间
    if (cd_sec > 0)
    {
        robj.push_back( Pair("coolTime", cd_sec) );
        return HC_ERROR_IN_COOLTIME_RACE;
    }
    if (rd->m_attack_who > 0)
    {
        return HC_ERROR_IN_COOLTIME_RACE;
    }
    //被挑战方是否正在被挑战
    if (td->m_under_attack > 0)
    {
        return HC_ERROR_TARGET_IS_BUSY;
    }
    //是否能挑戰
    if (!canChallege(rd->m_rank, td->m_rank))
    {
        return HC_ERROR;
    }
    int total_times = rd->m_race_times;
    if (total_times < iRaceFreeTimes)
    {
        ;
    }
    else
    {
        //购买的次数
        int buyTime = pc->queryExtraData(char_data_type_daily, char_data_buy_race);
        if (total_times < (iRaceFreeTimes + buyTime))
        {
        }
        else
        {
            return HC_ERROR;
        }
#if 0
        if (rd->m_charactor->addLing(iRaceFreeTimes-1-total_times) < 0)
        {
            robj.push_back( Pair("ling", (1+total_times-iRaceFreeTimes)) );
            return HC_ERROR_NOT_ENOUGH_LING;
        }
        //军令统计
        add_statistics_of_ling_cost(rd->m_charactor->m_id,rd->m_charactor->m_ip_address,1+total_times-iRaceFreeTimes,ling_race,2);
#endif
    }
    ++rd->m_race_times;
    ++rd->m_total_race;

    td->m_under_attack = pc->m_id;

    rd->m_needSave = true;

    //七日目标
    Singleton<seven_Goals_mgr>::Instance().updateGoals(*pc, pc->queryCreateDays(),goals_type_race);

    //act统计
    act_to_tencent(pc,act_new_race);

    int ret = HC_SUCCESS;
    Combat* pCombat = createRaceCombat(cid, target, ret);

    if (pCombat && HC_SUCCESS == ret)
    {
        //立即返回战斗双方的信息
        pCombat->setCombatInfo();
        InsertCombat(pCombat);
        std::string sendMsg = "{\"cmd\":\"race\",\"s\":200,\"getBattleList\":" + pCombat->getCombatInfoText() + "}";
        psession->send(sendMsg);
        //更新任务
        pc->updateTask(task_do_race, 0, 0);
        ret = HC_SUCCESS_NO_RET;

        //日常任务
        dailyTaskMgr::getInstance()->updateDailyTask(*pc, daily_task_race);
        //更新按钮次数
        int buy_times = pc->queryExtraData(char_data_type_daily, char_data_buy_race);
        int leftTimes = iRaceFreeTimes + buy_times - rd->m_race_times;
        if (leftTimes < 0)
        {
            leftTimes = 0;
        }
        pc->notifyEventState(top_level_event_race, 0, leftTimes);
    }
    //cout<<"********************RaceMgr::challenge("<<cid<<","<<target<<") end *********************"<<endl;
    return ret;
}

//购买挑战次数
int RaceMgr::buyChallenge(session_ptr& psession, int cid, json_spirit::Object& robj)
{
    //cout<<"********************RaceMgr::challenge("<<cid<<","<<target<<")*********************"<<endl;
    boost::shared_ptr<CharRaceData> rd = getRaceData(cid);
    if (!rd.get() || !rd->getChar())
    {
        ERR();
        return HC_ERROR;
    }
    CharData* pc = rd->getChar();
    int buyTime = pc->queryExtraData(char_data_type_daily, char_data_buy_race) + 1;
    int buyGold = 2 * buyTime;
    if (buyGold > iRaceBuyGoldMax)
    {
        buyGold = iRaceBuyGoldMax;
    }
    if (pc->addGold(-buyGold) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //金币消耗统计
    add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, buyGold, gold_cost_for_buy_race, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(pc, buyGold, gold_cost_for_buy_race);
#endif

    //保存购买次数
    pc->setExtraData(char_data_type_daily, char_data_buy_race, buyTime);

    int leftTimes = iRaceFreeTimes + buyTime - rd->m_race_times;
    if (leftTimes < 0)
    {
        leftTimes = 0;
    }
    //改变上面的按钮次数
    pc->notifyEventState(top_level_event_race, 0, leftTimes);

    robj.push_back( Pair("buyGold", buyGold + 2 > iRaceBuyGoldMax ? iRaceBuyGoldMax : buyGold + 2) );
    int ling = iRaceFreeTimes+ buyTime - rd->m_race_times;
    if (ling < 0)
    {
        ling = 0;
    }
    robj.push_back( Pair("ling", ling) );
    return HC_SUCCESS;
}

//竞技战斗结束
int RaceMgr::challengeResult(Combat* pCombat)
{
    //cout<<"********************RaceMgr::challengeResult("<<pCombat->m_attacker.getCharId()<<","<<pCombat->m_defender.getCharId()<<")*********************"<<endl;
    boost::shared_ptr<CharRaceData> rd = getRaceData(pCombat->m_attacker->getCharId());
    if (!rd.get() || !rd->getChar())
    {
        ERR();
        return HC_ERROR;
    }
    CharData* pc = rd->getChar();
    boost::shared_ptr<CharRaceData> td = getRaceData(pCombat->m_defender->getCharId());
    if (!td.get() || !td->getChar())
    {
        ERR();
        return HC_ERROR;
    }
    CharData* tc = td->getChar();
    bool change_rank = false;//是否有排名变动

    //获得物品加上积分
    Item score;
    score.type = item_type_arena_score;
    score.nums = rd->m_race_times > 20 ? 100 : 5*rd->m_race_times;

    //竞技场实际收益
    arenaRealReward(score.nums);

    rd->m_score += score.nums;
    rd->m_total_score += score.nums;

    //攻击方胜利了，获得宝箱
    if (pCombat->m_state == attacker_win)
    {
        //终结对方连杀公告
        if (td->m_wins >= 5)
        {
            std::string msg = strRace_msg_stop_win;
            str_replace(msg, "$W", MakeCharNameLink(pc->m_name));
            str_replace(msg, "$T", MakeCharNameLink(tc->m_name));
            str_replace(msg, "$N", LEX_CAST_STR(td->m_wins));

            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }
        td->m_wins = 0;
        /*连续击杀公告 2012-1-4 修改
            连胜X    修饰语
            8-10    还需要继续加油
            11-14    无公告
            15-20    已经杀得入魔了
            21-29    拜托谁去终结他吧
            ≥30    已经超越战神了
        */
        ++rd->m_wins;
        if (rd->m_wins >= 8 && (rd->m_wins <= 10 || rd->m_wins >= 15))
        {
            std::string msg = "";
            if (rd->m_wins >= 30)
            {
                msg = strRace_msg_win_26;
            }
            else if (rd->m_wins >= 21)
            {
                msg = strRace_msg_win_15_25;
            }
            else if (rd->m_wins >=15)
            {
                msg = strRace_msg_win_9_14;
            }
            else
            {
                msg = strRace_msg_win_5_8;
            }
            str_replace(msg, "$W", MakeCharNameLink(pc->m_name));
            str_replace(msg, "$N", LEX_CAST_STR(rd->m_wins));

            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }
        int type = item_type_silver;
        int counts = 800 + pc->m_level * 20;
        //竞技场实际收益
        arenaRealReward(counts);
        Item item_p;
        item_p.type = type;
        item_p.nums = counts;
        pCombat->m_getItems.push_back(item_p);

        Item item_prestige;
        item_prestige.type = item_type_prestige;
        item_prestige.nums = iRaceWinPrestige;
        //竞技场实际收益
        arenaRealReward(item_prestige.nums);
        pCombat->m_getItems.push_back(item_prestige);

        pCombat->m_getItems.push_back(score);

        boost::shared_ptr<CharData> cdata = rd->getCharData();
        giveLoots(cdata, pCombat, true, give_race_loot);
        #if 0
        rd->m_charactor->addPrestige(iRaceWinPrestige);
        std::string notify = strRace_msg_win_prestige;
        str_replace(notify, "$P", LEX_CAST_STR(iRaceWinPrestige));
        json_spirit::Object robj;
        robj.push_back( Pair("cmd", "message"));
        robj.push_back( Pair("s", 200));
        robj.push_back( Pair("msg", notify));
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
        if (account.get())
        {
            account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
        }
        #endif
        //军团事件
        //corpsMgr::getInstance()->addEvent(rd->m_charactor.get(), corps_event_add_exp, iRaceWinPrestige, 0);
        pc->NotifyCharData();
        //排名变化
        if (getRaceIdx(td->m_mapid) != getRaceIdx(rd->m_mapid))
        {
            //有一个人进入到新的竞技场了，排名就不变
        }
        else if (td->m_mapid >= 1 && td->m_mapid <= max_map_id
            && td->m_rank < rd->m_rank)
        {
            int old_rank = rd->m_rank;
            int mapid = td->m_mapid;
            int newRank = td->m_rank;

            int raceIdx = getRaceIdx(mapid);
#if 0
            //插入排名
            for (int i = rd->m_rank; i > td->m_rank; --i)
            {
                if (i >= 2)
                {
                    ++(m_race_rank[raceIdx][i-2]->m_rank);
                    m_race_rank[raceIdx][i-1] = m_race_rank[raceIdx][i-2];
                    m_race_rank[raceIdx][i-1]->m_trend = 2;    //下降
                    //更新競技排名榜
                    if (m_race_rank[raceIdx][i-1]->m_rank <= 20)
                    {
                        m_top20[m_race_rank[raceIdx][i-1]->m_rank - 1] = m_race_rank[raceIdx][i-1];
                    }
                }
            }
            m_race_rank[raceIdx][newRank-1] = rd;
            rd->m_rank = newRank;
            if (rd->m_rank <= 20)
            {
                m_top20[rd->m_rank - 1] = rd;
            }
            rd->m_trend = 1;    //上升
            change_rank = true;
#else
            //交換排名
            m_race_rank[raceIdx][rd->m_rank - 1] = td;
            m_race_rank[raceIdx][newRank-1] = rd;
            td->m_rank = rd->m_rank;
            rd->m_rank = newRank;
            td->m_trend = 2;    //下降
            rd->m_trend = 1;    //上升
            change_rank = true;

            //更新競技排名榜
            if (td->m_rank <= 20)
            {
                m_top20[td->m_rank - 1] = td;
            }
            if (rd->m_rank <= 20)
            {
                m_top20[rd->m_rank - 1] = rd;
            }
#endif
            //进入前20，祝贺
            if (rd->m_rank <= 20 && old_rank > 20)
            {
                Singleton<relationMgr>::Instance().postCongradulation(pc->m_id, CONGRATULATION_TOP_RACER, rd->m_rank, 0);
            }

            //竞技胜利，排名发生变化，给失败方发邮件
            pCombat->m_mail_content = strRaceNotifyMailContent;
            str_replace(pCombat->m_mail_content, "$W", pCombat->m_attacker->Name());
            str_replace(pCombat->m_mail_content, "$S", LEX_CAST_STR(td->m_rank));

            pCombat->m_mail_to = pCombat->m_defender->getCharId();
            pCombat->m_mail_to_name = pCombat->m_defender->Name();
            pCombat->m_mail_title = strRaceNotifyMailTitle;

            //新冠军产生公告
            if (rd->m_rank == 1)
            {
                std::string racetitle = strRace_new_title;
                str_replace(racetitle, "$W", MakeCharNameLink(pc->m_name));
                str_replace(racetitle, "$L", MakeCharNameLink(tc->m_name));
                boost::shared_ptr<raceTitle> p_rt = m_race_title[raceIdx];
                if (!p_rt.get())
                {
                    p_rt.reset(new raceTitle);
                    m_race_title[raceIdx] = p_rt;
                }
                p_rt->atk_name = pc->m_name;
                p_rt->def_name = tc->m_name;
                p_rt->id = pCombat->m_combat_id;
                p_rt->input = time(NULL);
                InsertSaveDb("replace into char_race_title (mapid,atk_name,def_name,battle_id,input) values ("
                                + LEX_CAST_STR(raceIdx) + ",'" + GetDb().safestr(p_rt->atk_name) + "','" + GetDb().safestr(p_rt->def_name) + "','" + LEX_CAST_STR(p_rt->id)+ "','"+LEX_CAST_STR(p_rt->input)+"')");
                GeneralDataMgr::getInstance()->broadCastSysMsg(racetitle, -1);
                if (m_top_battle_id != 0)
                {
                    //老第一战报删除
                    InsertSaveDb("update battle_records set archive=archive-1 where  archive>0 and id=" + LEX_CAST_STR(m_top_battle_id));
                    m_top_battle_id = pCombat->m_combat_id;
                }
            }
        }
        //打赢家丁系统增加
        //if (rd->m_charactor->m_servantOpen && td->m_charactor->m_servantOpen)
        {
            boost::shared_ptr<charServant> p = servantMgr::getInstance()->GetCharServant(pc->m_id);
            boost::shared_ptr<charServant> p1 = servantMgr::getInstance()->GetCharServant(tc->m_id);
            if (p.get() && p1.get() && !servantMgr::getInstance()->CheckServant(p->m_cid, tc->m_id) && !servantMgr::getInstance()->CheckCanList(p->m_cid, tc->m_id))
            {
                servantMgr::getInstance()->addServantLoser(pc->m_id,tc->m_id);
                servantMgr::getInstance()->Save_loser_list(pc->m_id);
            }
        }

        //首场竞技胜利，祝贺
        if (pc->queryExtraData(char_data_type_normal, char_data_first_race_success) == 0)
        {
            Singleton<relationMgr>::Instance().postCongradulation(pc->m_id, CONGRATULATION_FRIST_RACE_WIN, 0, 0);
            pc->setExtraData(char_data_type_normal, char_data_first_race_success, 1);
        }
#ifdef QQ_PLAT
        //竞技场分享
        Singleton<inviteMgr>::Instance().update_event(pc->m_id, SHARE_EVENT_FIRST_AREANA, 0);
#endif

        //支线任务
        pc->m_trunk_tasks.updateTask(task_arena_win, 1);
        pc->m_trunk_tasks.updateTask(task_arena_liansheng, rd->m_wins);
#ifdef VN_EN_SERVER
        //活动更新
        if (Singleton<new_event_mgr>::Instance().isEventOpen(event_race_win))
        {
            Singleton<new_event_mgr>::Instance().updateEvent(pc->m_id,event_race_win,rd->m_wins);
        }
#endif
    }
    else
    {
        //打输了挑战方增加冷却时间
        rd->m_race_cd_time = time(NULL) + iRaceCD;
        //被终结连杀公告
        if (rd->m_wins >= 5)
        {
            std::string msg = strRace_msg_stop_win;
            str_replace(msg, "$W", MakeCharNameLink(tc->m_name));
            str_replace(msg, "$T", MakeCharNameLink(pc->m_name));
            str_replace(msg, "$N", LEX_CAST_STR(rd->m_wins));

            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }
        rd->m_wins = 0;
        //连续击杀公告
        ++td->m_wins;
        if (td->m_wins >= 5)
        {
            std::string msg = "";
            if (td->m_wins >= 26)
            {
                msg = strRace_msg_win_26;
            }
            else if (td->m_wins >= 15)
            {
                msg = strRace_msg_win_15_25;
            }
            else if (td->m_wins >=9)
            {
                msg = strRace_msg_win_9_14;
            }
            else
            {
                msg = strRace_msg_win_5_8;
            }

            str_replace(msg, "$W", MakeCharNameLink(tc->m_name));
            str_replace(msg, "$N", LEX_CAST_STR(td->m_wins));

            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }
#if 0
        rd->m_charactor->addPrestige(iRaceLosePrestige);
        std::string notify = strRace_msg_lose_prestige;
        str_replace(notify, "$P", LEX_CAST_STR(iRaceLosePrestige));
        json_spirit::Object robj;
        robj.push_back( Pair("cmd", "message"));
        robj.push_back( Pair("s", 200));
        robj.push_back( Pair("msg", notify));
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
        if (account.get())
        {
            account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
        }
#else
        int type = item_type_silver;
        int counts = 400 + pc->m_level * 10;
        //竞技场实际收益
        arenaRealReward(counts);
        Item item_p;
        item_p.type = type;
        item_p.nums = counts;
        pCombat->m_getItems.push_back(item_p);
        Item item_prestige;
        item_prestige.type = item_type_prestige;
        item_prestige.nums = iRaceLosePrestige;
        //竞技场实际收益
        arenaRealReward(item_prestige.nums);
        pCombat->m_getItems.push_back(item_prestige);

        pCombat->m_getItems.push_back(score);

        boost::shared_ptr<CharData> cdata = rd->getCharData();
        giveLoots(cdata, pCombat, true, give_race_loot);
#endif
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
        if (account.get() && pc->m_horseOpen && pc->m_level < 45)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "notifyFail") );
            json_spirit::Object info;
            info.push_back( Pair("id", 2) );
            info.push_back( Pair("type", 0) );
            obj.push_back( Pair("info", info) );
            obj.push_back( Pair("s", 200) );
            account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
        }
#ifdef VN_EN_SERVER
        //活动更新
        if (Singleton<new_event_mgr>::Instance().isEventOpen(event_race_win))
        {
            Singleton<new_event_mgr>::Instance().updateEvent(tc->m_id,event_race_win,td->m_wins);
        }
#endif
        //军团事件
        //corpsMgr::getInstance()->addEvent(rd->m_charactor.get(), corps_event_add_exp, iRaceLosePrestige, 0);
    }
    //最新战报
    //提示排名变动的信息字段
    int r_rank = 0, t_rank = 0;
    if (change_rank)
    {
        r_rank = rd->m_rank;
        t_rank = td->m_rank;
    }

    pCombat->m_extra_data[0] = r_rank;
    pCombat->m_extra_data[1] = t_rank;
    rd->addRaceRecord(pCombat->m_combat_id, pCombat->m_state, pCombat->m_defender->getCharId(), pCombat->m_defender->Name(),1,r_rank);
    td->addRaceRecord(pCombat->m_combat_id, pCombat->m_state, pCombat->m_attacker->getCharId(), pCombat->m_attacker->Name(),2,t_rank);

    //战斗打完双方空闲了
    td->m_under_attack = 0;
    rd->m_attack_who = 0;

    //判断是否要搬迁竞技场
    if (td->m_mapid != tc->m_area)
    {
        updateZone(tc->m_id);
    }
    if (rd->m_mapid != pc->m_area)
    {
        updateZone(pc->m_id);
    }

    pCombat->m_archive_report = 2;    //被两个人引用
    pCombat->AppendResult(pCombat->m_result_obj);

    rd->save(true);
    td->save(true);

    //刷新状态
    //rd->m_charactor->refreshStates(0,0);
    //消耗兵器/军令
    pc->combatCost(pCombat->m_state == attacker_win, combat_race);

    //战报存/发送
    InsertSaveCombat(pCombat);

    return HC_SUCCESS;
}

//搬迁竞技场
int RaceMgr::updateZone(int cid)
{
    boost::shared_ptr<CharRaceData> rd = getRaceData(cid);
    if (!rd.get())
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        //初次搬入竞技
        if (cdata.get())
        {
            //插入数据库
            //InsertSaveDb("replace into char_race (cid,rank,coolTime,raceTimes,openTimes,totalRace,totalOpen,mapid) values ("
            //            + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(m_race_maps[cid]->m_rank)
            //            + ",0,0,0,0,0," + LEX_CAST_STR(cdata->m_area) + ")"
            //    );
            boost::shared_ptr<CharRaceData> rd = addCharactor(cid,cdata->m_level, cdata->m_area, 0, 0, 0, 0, 0, 0, 0);
            if (rd.get())
            {
                rd->m_race_records.reset(new std::list<boost::shared_ptr<raceRecord> >);
            }
            if (m_race_maps.size() > 1)
            {
                cdata->_checkGuide(guide_id_race);
            }
            return HC_SUCCESS;
        }
        else
        {
            ERR();
            return HC_ERROR;
        }
    }
    //还在战斗的，等战斗结束搬入新竞技场
    if (rd->m_under_attack || rd->m_attack_who)
    {
        return  HC_SUCCESS;
    }
    CharData* pc = rd->getChar();
    if (!pc)
    {
        return HC_SUCCESS;
    }
    if (pc->m_area >= 1 && pc->m_area <= max_map_id
        && pc->m_area > rd->m_mapid)
    {
        rd->m_mapid = pc->m_area;
        int raceIdx = getRaceIdx(rd->m_mapid);
        int raceIdx_to = getRaceIdx(pc->m_area);
        if (m_race_maps.find(pc->m_id) != m_race_maps.end())
        {
            if (raceIdx != raceIdx_to)
            {
                //从原来的竞技场移除，更新后面的排名
                if (rd->m_mapid > 1)
                {
                    assert(rd->m_rank >= 1 && rd->m_rank <= (int)m_race_rank[raceIdx].size());
                    m_race_rank[raceIdx].erase(m_race_rank[raceIdx].begin()+rd->m_rank-1);
                    for (size_t i = rd->m_rank; i <= m_race_rank[raceIdx].size(); ++i)
                    {
                        m_race_rank[raceIdx][i-1]->m_rank = i;
                    }
                }
                //加到新的竞技场最后
                //rd->m_mapid = rd->m_charactor->m_area;
                m_race_rank[raceIdx_to].push_back(rd);
                rd->m_rank = m_race_rank[raceIdx_to].size();
            }
        }
        else
        {
            //加到新的竞技场最后
            //rd->m_mapid = rd->m_charactor->m_area;
            m_race_rank[raceIdx_to].push_back(rd);
            rd->m_rank = m_race_rank[raceIdx_to].size();
        }
    }
    return HC_SUCCESS;
}

//清除冷却时间
int RaceMgr::clearCD(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharRaceData> td = getRaceData(cid);
    if (!td.get() || !td->getChar())
    {
        ERR();
        return HC_ERROR;
    }
    int cd_sec = td->m_race_cd_time - time(NULL);
    if (cd_sec > 0)
    {
        CharData* pc = td->getChar();
        int need_gold = cd_sec / 60;
        if (cd_sec % 60 > 0)
            ++need_gold;
        if (pc->addGold(-need_gold) < 0)
        {
            robj.push_back( Pair("gold", need_gold) );
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, need_gold, gold_cost_for_race, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(pc, need_gold, gold_cost_for_race);
#endif
        pc->NotifyCharData();
        td->m_race_cd_time = 0;
    }
    return HC_SUCCESS;
}

int RaceMgr::saveAll()
{
    boost::unordered_map<int, boost::shared_ptr<CharRaceData> >::iterator it = m_race_maps.begin();
    while (it != m_race_maps.end())
    {
        it->second->save(true);
        ++it;
    }
    boost::unordered_map<int, boost::shared_ptr<raceTitle> >::iterator it_title = m_race_title.begin();
    while (it_title != m_race_title.end())
    {
        InsertSaveDb("replace into char_race_title (mapid,atk_name,def_name,battle_id,input) values ("
                        + LEX_CAST_STR(it_title->first) + ",'" + GetDb().safestr(it_title->second->atk_name) + "','" + GetDb().safestr(it_title->second->def_name) + "','" + LEX_CAST_STR(it_title->second->id)+ "','"+LEX_CAST_STR(it_title->second->input)+"')");
        ++it;
    }
    return HC_SUCCESS;
}

//重置竞技次数
int RaceMgr::resetAll()
{
    boost::unordered_map<int, boost::shared_ptr<CharRaceData> >::iterator it = m_race_maps.begin();
    while (it != m_race_maps.end())
    {
        it->second->m_race_times = 0;
        it->second->m_race_cd_time = 0;
        it->second->m_score = 0;
        ++it;
    }
    InsertSaveDb("update `char_race` set coolTime=0,raceTimes=0,score=0 where 1");
    return HC_SUCCESS;
}

int RaceMgr::updateAll()
{
    Query q(GetDb());
    q.get_result("select id from charactors where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int cid = q.getval();
        updateZone(cid);
    }
    q.free_result();
    return HC_SUCCESS;
}

//每年冬季的晚上23点颁奖
int RaceMgr::yearAwards()
{
    //移除上次的竞技场称号
    if (m_last_top_five.size())
    {
        for (int i = 0; i < m_last_top_five.size(); ++i)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_last_top_five[i]).get();
            if (pc)
            {
                pc->m_nick.remove_nick(i+1);
                pc->SaveNick();
            }
        }
        m_last_top_five.clear();
    }
    int year = GeneralDataMgr::getInstance()->getYear();
    std::string season = GeneralDataMgr::getInstance()->getSeasonString();
    std::string strTitle = strRaceMailTitle;
    std::string this_strRaceMailContent = strRaceMailContent;
    str_replace(this_strRaceMailContent, "$Y", LEX_CAST_STR(year), true);
    str_replace(this_strRaceMailContent, "$S", season, true);

    str_replace(strTitle, "$Y", LEX_CAST_STR(year), true);
    str_replace(strTitle, "$S", season, true);

    int rank = 0;
    for (std::vector<boost::shared_ptr<CharRaceData> >::iterator it = m_race_rank[0].begin(); it != m_race_rank[0].end(); ++it)
    {
        ++rank;
        if (it->get() == NULL)
        {
            continue;
        }
        CharData* pc = it->get()->getChar();
        if (pc == NULL)
        {
            continue;
        }
        int prestige = 0;
        //int ling = 0;
        int silver = 0;
        if (HC_SUCCESS != getYearAwards(rank, prestige, silver))
        {
            break;
        }
        else
        {
            std::string strContent = this_strRaceMailContent;
            std::string strGet = "";
            //奖励暂存
            std::list<Item> tmp_list;
            if (prestige)
            {
                //竞技场实际收益
                //arenaRealReward(prestige);
                Item item_p;
                item_p.type = item_type_prestige;
                item_p.nums = prestige;
                tmp_list.push_back(item_p);
                //pc->addPrestige(prestige);
                //add_statistics_of_prestige_get(pc->m_id,pc->m_ip_address,prestige,prestige_race,pc->m_union_id,pc->m_server_id);
                strGet += strPrestige + strCounts + LEX_CAST_STR(prestige);
            }

            if (silver)
            {
                //竞技场实际收益
                //arenaRealReward(silver);
                Item item_p;
                item_p.type = item_type_silver;
                item_p.nums = silver;
                tmp_list.push_back(item_p);
                //pc->addSilver(silver);
                //add_statistics_of_silver_get(pc->m_id,pc->m_ip_address,silver,silver_get_race, pc->m_union_id, pc->m_server_id);
                if (strGet != "")
                {
                    strGet += ",";
                }
                strGet += strSilver + strCounts + LEX_CAST_STR(silver);
            }
            Singleton<char_rewards_mgr>::Instance().updateCharRewards(pc->m_id,rewards_type_race,rank,tmp_list);

            pc->NotifyCharData();

            str_replace(strContent, "$R", LEX_CAST_STR(rank), true);
            str_replace(strContent, "$G", strGet, true);

            //发送系统邮件
            sendSystemMail(pc->m_name, pc->m_id, strTitle, strContent);

            //给称号
            if (rank >= 1 && rank <= 5)
            {
                pc->m_nick.add_nick(rank);
                pc->SaveNick();

                m_last_top_five.push_back(pc->m_id);
            }
        }
    }
    //保存上次排名前5
    json_spirit::Value v(m_last_top_five.begin(), m_last_top_five.end());
    GeneralDataMgr::getInstance()->setStr("last_arena_top_five", json_spirit::write(v));

    return HC_SUCCESS;
}

//獲取競技排名榜
int RaceMgr::getTop20(json_spirit::Object& robj)
{
    json_spirit::Array raceRankList;
    json_spirit::Object info;
    for (int i = 0; i < 20; ++i)
    {
        if (m_top20[i].get())
        {
            CharRaceData* pRace = m_top20[i].get();
            CharData* cd = pRace->getChar();
            if (!cd)
            {
                ERR();
                cout<<"cid:"<<pRace->m_cid<<endl;
                continue;
            }
            json_spirit::Object o;
            o.push_back( Pair("id", cd->m_id) );
            o.push_back( Pair("group", cd->m_camp) );
            o.push_back( Pair("rank", pRace->m_rank) );
            o.push_back( Pair("level", cd->m_level) );
            o.push_back( Pair("name", cd->m_name) );
            o.push_back( Pair("spic", cd->m_spic) );
            o.push_back( Pair("trend", pRace->m_trend) );
            o.push_back( Pair("attack", cd->getAttack(0)) );

            raceRankList.push_back(o);
        }
    }
    robj.push_back( Pair("list", raceRankList) );
    return HC_SUCCESS;
}

int RaceMgr::getYearAwards(int rank, int& prestige, int& silver)
{
    prestige = 0;
    silver = 0;

    if (rank >= 1 && rank <= m_last_rank)
    {
        for (std::vector<raceRankRewards>::iterator it = m_rewards.begin(); it != m_rewards.end(); ++it)
        {
            if (rank <= it->rank)
            {
                silver = (int)(it->silver * 10000.00);
                prestige = it->prestige;
                //竞技场实际收益
                arenaRealReward(silver);
                arenaRealReward(prestige);
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

int RaceMgr::getRandomServantList(int cid, int mapid, std::vector<int>& list)
{
    int raceIdx = getRaceIdx(mapid);
    for (size_t i = 11; i <= m_race_rank[raceIdx].size() && i <= 100; ++i)
    {
        //cout << "check rank=" << i << endl;
        if (m_race_rank[raceIdx][i-1].get())
        {
            CharData* pc = m_race_rank[raceIdx][i-1]->getChar();
            if (pc && pc->m_id != cid && pc->m_servantOpen)
            {
                //cout << "push_back=" << m_race_rank[raceIdx][i-1]->m_charactor->m_id;
                list.push_back(pc->m_id);
            }
        }
    }
    return 0;
}

//查询排名奖励
int RaceMgr::QueryRankRewards(int rank1, int rank2, json_spirit::Object& robj)
{
    int min_prestige = 0, max_prestige = 0, min_silver = 0, max_silver = 0;
    getYearAwards(rank1, min_prestige, min_silver);
    getYearAwards(rank2, max_prestige, max_silver);

    json_spirit::Object getobj;
    getobj.push_back( Pair("min_prestige", min_prestige) );
    getobj.push_back( Pair("max_prestige", max_prestige) );
    getobj.push_back( Pair("min_silver", min_silver) );
    getobj.push_back( Pair("max_silver", max_silver) );
    robj.push_back( Pair("get", getobj) );
    return HC_SUCCESS;
}

int RaceMgr::deleteChar(int cid)
{
    boost::shared_ptr<CharRaceData> rd = getRaceData(cid);
    if (rd.get())
    {
        //从原来的竞技场移除，更新后面的排名
        if (rd->m_mapid > 1)
        {
            int raceIdx = getRaceIdx(rd->m_mapid);
            assert(rd->m_rank >= 1 && rd->m_rank <= (int)m_race_rank[raceIdx].size());
            m_race_rank[raceIdx].erase(m_race_rank[raceIdx].begin()+rd->m_rank-1);
            for (size_t i = rd->m_rank; i <= m_race_rank[raceIdx].size(); ++i)
            {
                m_race_rank[raceIdx][i-1]->m_rank = i;
            }
        }
    }
    m_race_maps.erase(cid);
    InsertSaveDb("delete from char_race where cid=" + LEX_CAST_STR(cid));
    //InsertSaveDb("delete from char_race_get where cid=" + LEX_CAST_STR(cid));
    return HC_SUCCESS;
}

//查询商品列表
int RaceMgr::getList(int cid, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharRaceData> rd = getRaceData(cid);
    if (!rd.get() || !rd->getChar())
    {
        ERR();
        return HC_ERROR;
    }
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    int nums_per_page = 0;
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page == 0)
    {
        nums_per_page = 8;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    json_spirit::Array list;
    for (int pos = 1; pos <= m_goods.size(); ++pos)
    {
        ++cur_nums;
        if (cur_nums >= first_nums && cur_nums <= last_nums)
        {
            json_spirit::Object shop;
            shop.push_back( Pair("pos", pos) );
            shop.push_back( Pair("id", m_goods[pos-1].id) );
            shop.push_back( Pair("type", m_goods[pos-1].type) );;
            shop.push_back( Pair("name", m_goods[pos-1].name) );
            shop.push_back( Pair("memo", m_goods[pos-1].memo) );
            shop.push_back( Pair("nums", m_goods[pos-1].num) );
            shop.push_back( Pair("quality", m_goods[pos-1].quality) );
            shop.push_back( Pair("spic", m_goods[pos-1].spic) );
            shop.push_back( Pair("price", m_goods[pos-1].price) );
            list.push_back(shop);
        }
     }
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    robj.push_back( Pair("list", list));
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    robj.push_back( Pair("score", rd->m_total_score) );
    robj.push_back( Pair("race_times", rd->m_race_times) );
    //竞技场实际收益
    int canGet = rd->m_race_times >= 20 ? 100 : 5*(rd->m_race_times+1);
    arenaRealReward(canGet);
    robj.push_back( Pair("can_get", canGet) );
    return HC_SUCCESS;
}

//购买商品
int RaceMgr::buy(int cid, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int pos = 0, nums = 0;
    READ_INT_FROM_MOBJ(pos,o,"pos");
    READ_INT_FROM_MOBJ(nums,o,"nums");
    boost::shared_ptr<CharRaceData> rd = getRaceData(cid);
    if (!rd.get() || !rd->getChar())
    {
        ERR();
        return HC_ERROR;
    }
    if (pos >= 1 && pos <= m_goods.size() && nums > 0)
    {
        CharData* pc = rd->getChar();
        int need_score = (m_goods[pos-1].price * nums);
        if (rd->m_total_score < need_score || need_score <= 0)
        {
            return HC_ERROR_NOT_ENOUGH_RACESCORE;
        }
        if (m_goods[pos-1].type == 2)
        {
            if (pc->m_bag.isFull())
            {
                return HC_ERROR_BAG_FULL;
            }
        }
        //扣除兑换点数
        rd->m_total_score -= need_score;
        switch (m_goods[pos-1].type)
        {
            //资源(金币银币声望)
            case 1:
                {
                    if (m_goods[pos-1].id == 1)
                    {
                        pc->addGold(m_goods[pos-1].num * nums);
                        //金币获得统计
                        add_statistics_of_gold_get(pc->m_id, pc->m_ip_address,m_goods[pos-1].num * nums,gold_get_race, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
                        gold_get_tencent(pc, m_goods[pos-1].num * nums);
#endif
                    }
                    else if(m_goods[pos-1].id == 2)
                    {
                        pc->addSilver(m_goods[pos-1].num * nums);
                        add_statistics_of_silver_get(pc->m_id, pc->m_ip_address,m_goods[pos-1].num * nums,silver_get_race, pc->m_union_id, pc->m_server_id);
                    }
                    else if(m_goods[pos-1].id == 3)
                    {
                        pc->addPrestige(m_goods[pos-1].num * nums);
                    }
                    pc->NotifyCharData();
                }
                break;
            //道具
            case 2:
                {
                    add_statistics_of_treasure_cost(pc->m_id, pc->m_ip_address,m_goods[pos-1].id,m_goods[pos-1].num,treasure_race,1, pc->m_union_id, pc->m_server_id);
                    pc->addTreasure(m_goods[pos-1].id, m_goods[pos-1].num * nums);
                }
                break;
        }
        rd->m_needSave = true;
        rd->save(true);
        robj.push_back( Pair("goods_name", m_goods[pos-1].name) );
        robj.push_back( Pair("goods_nums", m_goods[pos-1].num * nums) );
        //act统计
        act_to_tencent(pc, act_new_race_buy);
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

int RaceMgr::left_score(int race_time, int& goal_times)
{
    int ret = 0;
    for (int i = race_time; i < iRaceScoreTimes; ++i)
    {
        if ((i + 1) == 5)
        {
            ret += 100;
            goal_times = 5;
            break;
        }
        else if ((i + 1) == 10)
        {
            ret += 150;
            goal_times = 10;
            break;
        }
        else if ((i + 1) == 15)
        {
            ret += 200;
            goal_times = 15;
            break;
        }
        else if ((i + 1) == iRaceScoreTimes)
        {
            ret += 300;
            goal_times = iRaceScoreTimes;
            break;
        }
    }
    //竞技场实际收益
    arenaRealReward(ret);
    return ret;
}

