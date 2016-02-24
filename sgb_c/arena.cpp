
#include "arena.h"
#include "utils_all.h"
#include "data.h"
#include "errcode_def.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_lang.h"
#include <syslog.h>
#include "singleton.h"
#include "rewards.h"
#include "relation.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

volatile int iArenaFreeTimes = iDefaultArenaFreeTimes;    //免费15次挑战

extern std::string strArenaNotifyMailContent;
extern std::string strArenaNotifyMailTitle;
extern std::string strArenaTop1Msg;
extern std::string strArenaTop2Msg;
extern std::string strArenaTop3Msg;

//获得竞技排名列表
int ProcessQueryArenaRankList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<arenaMgr>::Instance().queryArenaList(pc->m_id, robj);
}

//查询自己的竞技信息
int ProcessQueryArenaInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<arenaMgr>::Instance().querySelfInfo(pc->m_id, robj);
}

//查询排行奖励
int ProcessQueryRankRewards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int rank1 = 0, rank2 = 0;
    READ_INT_FROM_MOBJ(rank1,o,"rank1");
    READ_INT_FROM_MOBJ(rank2,o,"rank2");
    return Singleton<arenaMgr>::Instance().QueryRankRewards(rank1, rank2, robj);
}

//购买挑战次数
int ProcessBuyArena(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<arenaMgr>::Instance().buyChallenge(psession, pc->m_id, robj);
}

//竞技积分商品
int ProcessQueryArenaGoodsList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<arenaMgr>::Instance().getList(pc->m_id, o, robj);
}

//领取竞技积分商品
int ProcessGetArenaGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<arenaMgr>::Instance().getGood(pc->m_id, o, robj);
}

inline int _getPreChar(int rank)
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

CharArenaData::CharArenaData(int cid)
:m_cid(cid)
{
    m_needSave = false;
    //m_charactor = GeneralDataMgr::getInstance()->GetCharData(cid);
}

boost::shared_ptr<CharData> CharArenaData::getCharData()
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

int CharArenaData::save(bool force_save)
{
    if (force_save || (time(NULL) > m_save_time && m_needSave))
    {
        m_needSave = false;
        m_save_time = time(NULL) + 300;
        InsertSaveDb("replace into char_arena (cid,rank,totalArena,wins,updateTime) values ("
                        + LEX_CAST_STR(m_cid) + "," + LEX_CAST_STR(m_rank)
                        + "," + LEX_CAST_STR(m_total_arena)
                        + "," + LEX_CAST_STR(m_wins)
                        + ",unix_timestamp()"
                        + ")"
                );
    }
    return HC_SUCCESS;
}

void CharArenaData::toObj(json_spirit::Object& robj)
{
    robj.push_back( Pair("id", m_charactor->m_id) );
    robj.push_back( Pair("hid", m_charactor->m_heros.m_default_hero) );
    robj.push_back( Pair("race", m_charactor->m_race) );
    robj.push_back( Pair("rank", m_rank) );
    robj.push_back( Pair("level", m_charactor->m_level) );
    robj.push_back( Pair("name", m_charactor->m_name) );
    robj.push_back( Pair("spic", m_charactor->m_spic) );
    robj.push_back( Pair("gender", m_charactor->m_gender) );
}

boost::shared_ptr<std::list<boost::shared_ptr<arenaRecord> > > CharArenaData::getArenaRecords()
{
    if (!m_arena_records.get())
    {
        m_arena_records.reset(new std::list<boost::shared_ptr<arenaRecord> >);
        Query q(GetDb());
        q.get_result("select id,input,result,attacker,attackerName,defender,defenderName,extra1,extra2 from battle_records where (attacker=" + LEX_CAST_STR(m_cid) + " or defender=" + LEX_CAST_STR(m_cid) + ") and type=2 order by id desc limit 4");
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
            boost::shared_ptr<arenaRecord> record(new arenaRecord);
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
            m_arena_records->insert(m_arena_records->begin(), record);
        }
        q.free_result();
    }
    return m_arena_records;
}

int CharArenaData::addArenaRecord(uint64_t id, int result, int target, const std::string& target_name, int type, int rank_result)
{
    boost::shared_ptr<std::list<boost::shared_ptr<arenaRecord> > > mygets = getArenaRecords();
    if (!mygets.get())
    {
        ERR();
        return HC_ERROR;
    }

    m_needSave = true;
    boost::shared_ptr<arenaRecord> record(new arenaRecord);
    record->id = id;
    record->input = time(NULL);
    record->result = result;
    record->tid = target;
    record->tname = target_name;
    record->type = type;
    record->rank_result = rank_result;
    m_arena_records->push_back(record);

    while (mygets->size() > 4)
    {
        boost::shared_ptr<arenaRecord> rpt = mygets->front();
        if (rpt.get())
        {
            //顶出去的战报修改为不存档
            InsertSaveDb("update battle_records set archive=archive-1 where archive>0 and id=" + LEX_CAST_STR(rpt->id));
        }
        mygets->pop_front();
    }
    return HC_SUCCESS;
}

arenaMgr::arenaMgr()
{
    Query q(GetDb());
    int rank_rewards_max = 0;
    q.get_result("SELECT count(distinct(rank)) FROM base_arena_rankings_rewards where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        rank_rewards_max = q.getval();
    }
    for (int i = 1; i <= rank_rewards_max; ++i)
    {
        arenaRankRewards rrr;
        rrr.rank = i;
        rrr.reward.clear();
        q.get_result("select itemType,itemId,counts,extra from base_arena_rankings_rewards where rank="+LEX_CAST_STR(i)+" order by id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            Item item;
            item.type = q.getval();
            item.id = q.getval();
            item.nums = q.getval();
            item.extra = q.getval();
            rrr.reward.push_back(item);
        }
        q.free_result();
        if (rrr.reward.size())
        {
            m_rewards.push_back(rrr);
            m_last_rank = rrr.rank;
        }
    }

    if (!q.execute("delete FROM  char_arena WHERE cid NOT IN (SELECT id FROM charactors)"))
    {
        CHECK_DB_ERR(q);
    }
    //榜首信息
    q.get_result("select atk_name,def_name,battle_id,input from char_arena_title where 1 order by input desc limit 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        if (!m_arena_title.get())
        {
            m_arena_title.reset(new arenaTitle);
        }
        m_arena_title->atk_name = q.getstr();
        m_arena_title->def_name = q.getstr();
        m_arena_title->id = q.getval();
        m_arena_title->input = q.getval();
        m_top_battle_id = m_arena_title->id;
    }
    q.free_result();

    //积分商店
    q.get_result("SELECT id,needscore,itemType,itemId,counts,extra FROM base_arena_shop_rewards WHERE 1 order by id asc");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        int id = q.getval();
        baseArenaGoods tmp;
        tmp.needscore = q.getval();
        tmp.reward.type = q.getval();
        tmp.reward.id = q.getval();
        tmp.reward.nums = q.getval();
        tmp.reward.extra = q.getval();
        m_goods.push_back(tmp);
    }
    q.free_result();

    //q.get_result("select cr.cid,c.level,cr.rank,cr.coolTime,cr.raceTimes,cr.openTimes,cr.totalRace,cr.totalOpen,cr.wins from char_race as cr left join charactors as c on cr.cid=c.id where cr.mapid=" + LEX_CAST_STR(i) + " order by cr.rank");
    //这里需要比较长的时间，所以最好放在最后面，以免数据库连接超时
    q.get_result("select cid,rank,totalArena,wins from char_arena where 1 order by rank,updateTime desc");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int cid = q.getval();
        int rank = q.getval();
        int totalArena = q.getval();
        int wins = q.getval();
        addCharactor(cid, rank, totalArena, wins);
    }
    q.free_result();

    iArenaFreeTimes = GeneralDataMgr::getInstance()->getInt("arena_free_times", iDefaultArenaFreeTimes);

    //读取上次结算称号
    std::string top = GeneralDataMgr::getInstance()->getStr("last_arena_top");
    if (top != "")
    {
        json_spirit::Value v;
        json_spirit::read(top, v);
        if (v.type() == json_spirit::array_type)
        {
            json_spirit::Array& a = v.get_array();
            for (json_spirit::Array::iterator it = a.begin(); it != a.end(); ++it)
            {
                json_spirit::Value v2 = *it;
                if (v2.type() == json_spirit::int_type)
                {
                    m_last_top.push_back(v2.get_int());
                }
                else
                {
                    m_last_top.clear();
                    break;
                }
            }
        }
    }
    if (m_last_top.size())
    {
        for (int i = 0; i < m_last_top.size(); ++i)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_last_top[i]).get();
            if (pc)
            {
                pc->m_nick.add_nick(nick_arena_start + i);
                pc->SaveNick();
            }
        }
    }
}

boost::shared_ptr<CharArenaData> arenaMgr::addCharactor(int cid, int rank, int totalArena, int wins)
{
    boost::unordered_map<int, boost::shared_ptr<CharArenaData> >::iterator it = m_arena_maps.find(cid);        //竞技信息
    if (it != m_arena_maps.end() && it->second.get() != NULL)
    {
        ERR();
        return it->second;
    }
    else
    {
        boost::shared_ptr<CharArenaData> rd(new CharArenaData(cid));
        m_arena_maps[cid] = rd;
        rd->m_under_attack = 0;
        rd->m_attack_who = 0;
        rd->m_total_arena = totalArena;
        rd->m_wins = wins;
        rd->m_trend = 0;
        //if (0 == rank)
        {
            m_arena_rank.push_back(rd);
            rd->m_rank = m_arena_rank.size();
        }
        if (rank != rd->m_rank)
        {
            rd->save(true);
        }
        return rd;
    }
}

boost::shared_ptr<CharArenaData> arenaMgr::getArenaData(int cid)
{
    boost::unordered_map<int, boost::shared_ptr<CharArenaData> >::iterator it = m_arena_maps.find(cid);        //竞技信息
    if (it != m_arena_maps.end())
    {
        it->second->getCharData();
        it->second->save(false);
        return it->second;
    }
    else
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (cdata.get())
        {
            boost::shared_ptr<CharArenaData> rd = addCharactor(cid, 0, 0, 0);
            if (rd.get())
            {
                rd->m_arena_records.reset(new std::list<boost::shared_ptr<arenaRecord> >);
                return rd;
            }
        }
        boost::shared_ptr<CharArenaData> rd;
        return rd;
    }
}

//查询自己的竞技场信息
int arenaMgr::querySelfInfo(int cid, json_spirit::Object& robj)
{
    time_t time_now = time(NULL);
    boost::shared_ptr<CharArenaData> rd = getArenaData(cid);
    if (!rd.get() || !rd->getCharData().get())
    {
        ERR();
        return HC_ERROR;
    }


    CharData* pc = rd->getCharData().get();
    if (!pc)
    {
        return HC_ERROR;
    }
    //榜首信息
    if (m_arena_title.get())
    {
        json_spirit::Object title;
        title.push_back( Pair("id", m_arena_title->id));
        title.push_back( Pair("input", (time_now-m_arena_title->input)/60));
        title.push_back( Pair("att_name", m_arena_title->atk_name));
        title.push_back( Pair("def_name", m_arena_title->def_name));
        robj.push_back( Pair("title", title) );
    }
    //挑战记录
    {
        json_spirit::Array history_list;
        boost::shared_ptr<std::list<boost::shared_ptr<arenaRecord> > > totalarenas = rd->getArenaRecords();
        std::list<boost::shared_ptr<arenaRecord> >::reverse_iterator it = (*totalarenas).rbegin();
        while (it != (*totalarenas).rend())
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
    json_spirit::Object arenaInfo;
    arenaInfo.push_back( Pair("rank", rd->m_rank) );

    int buy_times = pc->queryExtraData(char_data_type_daily, char_data_daily_buy_arena);
    int buygold = 2 * buy_times + 2;
    if (buygold > iArenaBuyGoldMax)
    {
        buygold = iArenaBuyGoldMax;
    }
    arenaInfo.push_back( Pair("buyGold", buygold) );
    int challengeTime = pc->queryExtraData(char_data_type_daily, char_data_daily_arena);
    int leftTimes = iArenaFreeTimes + buy_times - challengeTime;
    if (leftTimes < 0)
    {
        leftTimes = 0;
    }
    arenaInfo.push_back( Pair("leftTimes", leftTimes) );
    int cd = pc->queryExtraData(char_data_type_daily, char_data_daily_arena_cd) - time(NULL);
    arenaInfo.push_back( Pair("coolTime", (cd > 0)?cd:0) );
    arenaInfo.push_back( Pair("win", rd->m_wins) );
    arenaInfo.push_back( Pair("trend", rd->m_trend) );
    int max_rank = pc->queryExtraData(char_data_type_normal, char_data_normal_arena_maxrank);
    if (max_rank == 0 || max_rank > rd->m_rank)
    {
        max_rank = rd->m_rank;
        pc->setExtraData(char_data_type_normal, char_data_normal_arena_maxrank, max_rank);
    }
    int max_win = pc->queryExtraData(char_data_type_normal, char_data_normal_arena_maxwin);
    if (max_win == 0 || max_win < rd->m_wins)
    {
        max_win = rd->m_wins;
        pc->setExtraData(char_data_type_normal, char_data_normal_arena_maxwin, max_win);
    }
    arenaInfo.push_back( Pair("max_rank", max_rank) );
    arenaInfo.push_back( Pair("max_win", max_win) );
    int score = pc->queryExtraData(char_data_type_daily, char_data_daily_arena_score);
    arenaInfo.push_back( Pair("score", score) );
    if (score > 2)
    {
        int first_open = pc->queryExtraData(char_data_type_daily, char_data_daily_arena_shop_open);
        arenaInfo.push_back( Pair("open_shop", first_open == 0 ? 1 : 0) );
    }

    robj.push_back( Pair("arenaInfo", arenaInfo) );

    return HC_SUCCESS;
}

void arenaMgr::setArenaFreeTimes(int times)
{
    if (times == 0)
    {
        times = iDefaultArenaFreeTimes;
    }
    if (times > 0)
    {
        iArenaFreeTimes = times;
        GeneralDataMgr::getInstance()->setInt("race_free_times", iArenaFreeTimes);
    }
}

//前面的挑戰對手
CharArenaData* arenaMgr::getPreChar(int rank)
{
    if (rank == 1)
    {
        return NULL;
    }
    else
    {
        if (rank > (int)m_arena_rank.size())
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
            return m_arena_rank[rank - 1].get();
        }
    }
}

//後面的挑戰對手
CharArenaData* arenaMgr::getNextChar(int rank)
{
    rank = rank + 1;
    if (rank > (int)m_arena_rank.size())
    {
        return NULL;
    }
    else
    {
        return m_arena_rank[rank - 1].get();
    }
}

//能否挑戰
bool arenaMgr::canChallege(int rank1, int rank2)
{
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
            min_rank = _getPreChar(min_rank);
        }
        return (rank2 >= min_rank);
    }
}

//查询前后玩家
int arenaMgr::queryArenaList(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharArenaData> rd = getArenaData(cid);
    if (!rd.get() || !rd->getCharData().get())
    {
        ERR();
        return HC_ERROR;
    }
    json_spirit::Array arenaRankList;
    json_spirit::Object info;
    CharArenaData* prd = rd.get();
    CharArenaData* pRace = getPreChar(prd->m_rank);
    while (arenaRankList.size() < iArenaRankShowSize-1 && pRace)
    {
        CharData* cd = pRace->getCharData().get();
        if (!cd)
        {
            ERR();
            cout<<"cid:"<<pRace->m_cid<<endl;
            continue;
        }
        json_spirit::Object o;
        pRace->toObj(o);
        arenaRankList.insert(arenaRankList.begin() ,o);
        pRace = getPreChar(pRace->m_rank);
    }
    if (arenaRankList.size() < iArenaRankShowSize)
    {
        pRace = getNextChar(prd->m_rank - 1);//玩家自身也显示在内
        while (arenaRankList.size() < iArenaRankShowSize && pRace)
        {
            CharData* cd = pRace->getCharData().get();
            if (!cd)
            {
                ERR();
                cout<<"cid:"<<pRace->m_cid<<endl;
                continue;
            }
            json_spirit::Object o;
            pRace->toObj(o);
            if (pRace->m_rank == prd->m_rank)
            {
                o.push_back( Pair("self", 1) );
            }
            arenaRankList.push_back(o);
            pRace = getNextChar(pRace->m_rank);
        }
    }
    time_t t_now = time(NULL);
    time_t tt = time(NULL);
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&tt, t);
    if (t->tm_hour < 19 || (t->tm_hour == 19 && t->tm_min < 30))
    {
        t->tm_hour = 19;
        t->tm_min = 30;
        t->tm_sec = 0;
    }
    time_t t_refresh = mktime(t);
    if (t_refresh < t_now)
    {
        t_refresh += iONE_DAY_SECS;
    }
    int leftTime = t_refresh - t_now;
    info.push_back( Pair("leftTime", leftTime) );
    info.push_back( Pair("rank", rd->m_rank) );
    json_spirit::Array get_list;
    getSeasonAwards(rd->m_rank, get_list);
    info.push_back( Pair("get", get_list) );
    info.push_back( Pair("state", Singleton<rewardsMgr>::Instance().canGetReward(rd->getCharData()->m_id, REWARDS_TYPE_ARENA)) );
    robj.push_back( Pair("arenaRankList", arenaRankList) );
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

//购买挑战次数
int arenaMgr::buyChallenge(net::session_ptr& psession, int cid, json_spirit::Object& robj)
{
    //cout<<"********************RaceMgr::challenge("<<cid<<","<<target<<")*********************"<<endl;
    boost::shared_ptr<CharArenaData> rd = getArenaData(cid);
    if (!rd.get() || !rd->getCharData().get())
    {
        ERR();
        return HC_ERROR;
    }
    CharData* pc = rd->getCharData().get();
    int buyTime = pc->queryExtraData(char_data_type_daily, char_data_daily_buy_arena) + 1;
    int buyGold = 2 * buyTime;
    if (buyGold > iArenaBuyGoldMax)
    {
        buyGold = iArenaBuyGoldMax;
    }
    if (pc->subGold(buyGold, gold_cost_arena_add) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //保存购买次数
    pc->setExtraData(char_data_type_daily, char_data_daily_buy_arena, buyTime);

    int challengeTime = pc->queryExtraData(char_data_type_daily, char_data_daily_arena);

    int leftTimes = iArenaFreeTimes + buyTime - challengeTime;
    if (leftTimes < 0)
    {
        leftTimes = 0;
    }
    robj.push_back( Pair("leftTimes", leftTimes) );
    robj.push_back( Pair("buyGold", buyGold + 2 > iArenaBuyGoldMax ? iArenaBuyGoldMax : buyGold + 2) );
    return HC_SUCCESS;
}

//清除冷却时间
int arenaMgr::clearCD(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharArenaData> td = getArenaData(cid);
    if (!td.get() || !td->getCharData().get())
    {
        ERR();
        return HC_ERROR;
    }
    CharData* pc = td->getCharData().get();
    int cd_sec = pc->queryExtraData(char_data_type_daily, char_data_daily_arena_cd) - time(NULL);
    if (cd_sec > 0)
    {
        int need_gold = cd_sec / 60;
        if (cd_sec % 60 > 0)
            ++need_gold;
        if (pc->subGold(need_gold, gold_cost_arena_cd) < 0)
        {
            robj.push_back( Pair("gold", need_gold) );
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        pc->setExtraData(char_data_type_daily, char_data_daily_arena_cd, 0);
    }
    return HC_SUCCESS;
}

int arenaMgr::getSeasonAwards(int rank, json_spirit::Array& reward_list)
{
    if (rank >= 1 && rank <= m_last_rank)
    {
        for (std::vector<arenaRankRewards>::iterator it = m_rewards.begin(); it != m_rewards.end(); ++it)
        {
            if (rank <= it->rank)
            {
                for (std::list<Item>::iterator it_i = it->reward.begin(); it_i != it->reward.end(); ++it_i)
                {
                    Item& item = *it_i;
                    json_spirit::Object obj;
                    item.toObj(obj);
                    reward_list.push_back(obj);
                }
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

//查询排名奖励
int arenaMgr::QueryRankRewards(int rank1, int rank2, json_spirit::Object& robj)
{
    json_spirit::Array get_list1, get_list2;
    getSeasonAwards(rank1, get_list1);
    getSeasonAwards(rank2, get_list2);
    json_spirit::Object getobj;
    robj.push_back( Pair("min_get", get_list1) );
    robj.push_back( Pair("max_get", get_list2) );
    return HC_SUCCESS;
}

int arenaMgr::combatResult(chessCombat* pCombat)    //战斗结束
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (COMBAT_TYPE_ARENA != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    //cout<<"********************raceMgr::combatResult("<<pCombat->m_players[0].m_cid<<","<<pCombat->m_players[1].m_cid<<")*********************"<<endl;
    boost::shared_ptr<CharArenaData> rd = getArenaData(pCombat->m_players[0].m_cid);
    if (!rd.get() || !rd->getCharData().get())
    {
        ERR();
        return HC_ERROR;
    }
    CharData* pc = rd->getCharData().get();
    boost::shared_ptr<CharArenaData> td = getArenaData(pCombat->m_players[1].m_cid);
    if (!td.get() || !td->getCharData().get())
    {
        ERR();
        return HC_ERROR;
    }
    CharData* tc = td->getCharData().get();
    bool change_rank = false;//是否有排名变动
    pc->m_tasks.updateTask(GOAL_ARENA, 0, 1);
    pc->m_tasks.updateTask(GOAL_DAILY_ARENA, 0, 1);
    pc->m_score_tasks.updateTask(DAILY_SCORE_ARENA);
    if (pCombat->m_result == COMBAT_RESULT_ATTACK_WIN)
    {
        //终结对方连杀
        td->m_wins = 0;
        ++rd->m_wins;

        Item item_p(ITEM_TYPE_CURRENCY, CURRENCY_ID_ARENA_SCORE, 2, 0);
        pCombat->m_getItems.push_back(item_p);

        boost::shared_ptr<CharData> cdata = rd->getCharData();
        giveLoots(cdata, pCombat, true, loot_arena);
        if (td->m_rank < rd->m_rank)
        {
            int old_rank = rd->m_rank;
            int newRank = td->m_rank;

            //交換排名
            m_arena_rank[rd->m_rank - 1] = td;
            m_arena_rank[newRank-1] = rd;
            td->m_rank = rd->m_rank;
            rd->m_rank = newRank;
            td->m_trend = 2;    //下降
            rd->m_trend = 1;    //上升
            change_rank = true;

            //竞技胜利，排名发生变化，给失败方发邮件
            std::string mail_content = strArenaNotifyMailContent;
            str_replace(mail_content, "$W", pCombat->m_players[0].m_player_name);
            str_replace(mail_content, "$S", LEX_CAST_STR(td->m_rank));
            sendSystemMail(pCombat->m_players[1].m_player_name, pCombat->m_players[1].m_cid, strArenaNotifyMailTitle, mail_content,"", pCombat->m_combat_id);

            //新冠军产生公告
            if (rd->m_rank == 1)
            {
                std::string msg = strArenaTopChangeMsg;
                str_replace(msg, "$W", MakeCharNameLink(pc->m_name,pc->m_nick.get_string()));
                str_replace(msg, "$L", MakeCharNameLink(tc->m_name,tc->m_nick.get_string()));
                GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);

                if (!m_arena_title.get())
                {
                    m_arena_title.reset(new arenaTitle);
                }
                m_arena_title->atk_name = pc->m_name;
                m_arena_title->def_name = tc->m_name;
                m_arena_title->id = pCombat->m_combat_id;
                m_arena_title->input = time(NULL);
                InsertSaveDb("replace into char_arena_title (atk_name,def_name,battle_id,input) values ('"
                    + GetDb().safestr(m_arena_title->atk_name) + "','" + GetDb().safestr(m_arena_title->def_name) + "','" + LEX_CAST_STR(m_arena_title->id)+ "','"+LEX_CAST_STR(m_arena_title->input)+"')");
                if (m_top_battle_id != 0)
                {
                    //老第一战报删除
                    InsertSaveDb("update battle_records set archive=archive-1 where archive>0 and id=" + LEX_CAST_STR(m_top_battle_id));
                    m_top_battle_id = pCombat->m_combat_id;
                }
            }
            if (rd->m_rank <= 20)
            {
                Singleton<relationMgr>::Instance().postCongradulation(cdata->m_id, CONGRATULATION_TOP_ARENA, rd->m_rank, 0);
            }
        }
        //任务
        cdata->m_tasks.updateTask(GOAL_ARENA_WIN, 0, 1);
        cdata->m_tasks.updateTask(GOAL_DAILY_ARENA_WIN, 0, 1);
        Singleton<goalMgr>::Instance().updateTask(cdata->m_id, GOAL_TYPE_ARENA_WIN, 1);
    }
    else if (pCombat->m_result == COMBAT_RESULT_ATTACK_LOSE)
    {
        //打输了挑战方增加冷却时间
        pc->setExtraData(char_data_type_daily, char_data_daily_arena_cd, time(NULL) + iArenaCD);
        rd->m_wins = 0;
        ++td->m_wins;

        Item item_p(ITEM_TYPE_CURRENCY, CURRENCY_ID_ARENA_SCORE, 1, 0);
        pCombat->m_getItems.push_back(item_p);

        boost::shared_ptr<CharData> cdata = rd->getCharData();
        giveLoots(cdata, pCombat, true, loot_arena);
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
    rd->addArenaRecord(pCombat->m_combat_id, pCombat->m_result, pCombat->m_players[1].m_cid, pCombat->m_players[1].m_player_name,1,r_rank);
    td->addArenaRecord(pCombat->m_combat_id, pCombat->m_result, pCombat->m_players[0].m_cid, pCombat->m_players[0].m_player_name,2,t_rank);

    if (m_top_battle_id == pCombat->m_combat_id)
    {
        pCombat->m_archive_report = 2;    //被两个人引用
    }
    else
    {
        pCombat->m_archive_report = 3;    //被两个人 还有系统引用
    }
    rd->m_attack_who = 0;
    td->m_under_attack = 0;
    rd->save(true);
    td->save(true);
    return HC_SUCCESS;
}

//每天晚上19点30颁奖
int arenaMgr::seasonAwards()
{
    //移除上次的竞技场称号
    if (m_last_top.size())
    {
        for (int i = 0; i < m_last_top.size(); ++i)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_last_top[i]).get();
            if (pc)
            {
                pc->m_nick.remove_nick(nick_arena_start + i);
                pc->SaveNick();
            }
        }
        m_last_top.clear();
    }
    int rank = 0;
    for (std::vector<boost::shared_ptr<CharArenaData> >::iterator it = m_arena_rank.begin(); it != m_arena_rank.end(); ++it)
    {
        ++rank;
        if (it->get() == NULL)
        {
            continue;
        }
        CharData* pc = it->get()->getCharData().get();
        if (pc == NULL)
        {
            continue;
        }
        if (rank >= 1 && rank <= m_last_rank)
        {
            for (std::vector<arenaRankRewards>::iterator it_r = m_rewards.begin(); it_r != m_rewards.end(); ++it_r)
            {
                if (rank <= it_r->rank)
                {
                    //奖励暂存
                    std::list<Item> tmp_list = it_r->reward;
                    Singleton<rewardsMgr>::Instance().updateCharRewards(pc->m_id,REWARDS_TYPE_ARENA,rank,tmp_list);
                    //给称号
                    if (rank >= 1 && rank <= 3)
                    {
                        pc->m_nick.add_nick(nick_arena_start + rank - 1);
                        pc->SaveNick();
                        m_last_top.push_back(pc->m_id);
                    }
            		std::string msg = "";
            		switch (rank)
            		{
            			case 1:
            				msg = strArenaTop1Msg;
            				str_replace(msg, "$W", MakeCharNameLink(pc->m_name,pc->m_nick.get_string()));
                            str_replace(msg, "$R", itemlistToString(tmp_list));
            				break;
            			case 2:
            				msg = strArenaTop2Msg;
            				str_replace(msg, "$W", MakeCharNameLink(pc->m_name,pc->m_nick.get_string()));
                            str_replace(msg, "$R", itemlistToString(tmp_list));
            				break;
            			case 3:
            				msg = strArenaTop3Msg;
            				str_replace(msg, "$W", MakeCharNameLink(pc->m_name,pc->m_nick.get_string()));
                            str_replace(msg, "$R", itemlistToString(tmp_list));
            				break;
            		}
            		if (msg != "")
            		{
            			GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
            		}
                    break;
                }
            }
        }
    }
    //保存上次排名
    json_spirit::Value v(m_last_top.begin(), m_last_top.end());
    GeneralDataMgr::getInstance()->setStr("last_arena_top", json_spirit::write(v));
    return HC_SUCCESS;
}

//查询商品列表
int arenaMgr::getList(int cid, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharArenaData> rd = getArenaData(cid);
    if (!rd.get() || !rd->getCharData().get())
    {
        ERR();
        return HC_ERROR;
    }
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    int nums_per_page = 8;
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
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
            shop.push_back( Pair("need_score", m_goods[pos-1].needscore) );
            int has_get = rd->getCharData()->queryExtraData(char_data_type_daily, char_data_daily_arena_reward_start + pos);
            shop.push_back( Pair("has_get", has_get) );
            json_spirit::Object get;
            m_goods[pos-1].reward.toObj(get);
            shop.push_back( Pair("get", get) );
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
    int score = rd->getCharData()->queryExtraData(char_data_type_daily, char_data_daily_arena_score);
    robj.push_back( Pair("score", score) );
    int first_open = rd->getCharData()->queryExtraData(char_data_type_daily, char_data_daily_arena_shop_open);
    if (first_open == 0)
    {
        rd->getCharData()->setExtraData(char_data_type_daily, char_data_daily_arena_shop_open, 1);
    }
    return HC_SUCCESS;
}

//领取商品
int arenaMgr::getGood(int cid, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int pos = 0;
    READ_INT_FROM_MOBJ(pos,o,"pos");
    if (pos < 1 || pos > m_goods.size())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharArenaData> rd = getArenaData(cid);
    if (!rd.get() || !rd->getCharData().get())
    {
        ERR();
        return HC_ERROR;
    }
    //玩家领取情况
    int has_get = rd->getCharData()->queryExtraData(char_data_type_daily, char_data_daily_arena_reward_start + pos);
    if (has_get == 0)
    {
        //玩家日常活跃度
        int score = rd->getCharData()->queryExtraData(char_data_type_daily, char_data_daily_arena_score);
        if (score < m_goods[pos-1].needscore)
        {
            return HC_ERROR_NOT_ENOUGH_SCORE;
        }
        std::list<Item> items;
        items.push_back(m_goods[pos-1].reward);
        if (!rd->getCharData()->m_bag.hasSlot(itemlistNeedBagSlot(items)))
        {
            return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
        }
        rd->getCharData()->setExtraData(char_data_type_daily, char_data_daily_arena_reward_start + pos, 1);
        giveLoots(rd->getCharData().get(), items, NULL, &robj, true, loot_arena_shop);
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

