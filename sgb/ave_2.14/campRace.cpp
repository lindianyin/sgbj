
#include "campRace.h"
#include "spls_errcode.h"
#include "data.h"
#include "utils_all.h"
#include "combat.h"
#include "spls_timer.h"
#include "net.h"
#include "utils_lang.h"
#include <syslog.h>
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "igeneral.h"
#include "statistics.h"
#include "rankings.h"
#include "daily_task.h"
#include "qq_invite.h"
#include "singleton.h"

using namespace json_spirit;
using namespace net;

#define INFO(x)

extern void InsertCombat(Combat* pcombat);
extern void InsertSaveCombat(Combat* pcombat);

Database& GetDb();
extern int getSessionChar(session_ptr& psession, CharData* &pc);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);
extern void InsertSaveDb(const std::string& sql);

extern std::string strNewCampRaceRank1Msg;

//初始化阵营战的战斗
extern Combat* createCampRaceCombat(charCampRace* c1, charCampRace* c2, int& ret);

//static     boost::uuids::nil_generator gen;

//通知积分变化
const std::string strNotifyScore = "{\"cmd\":\"getGroupMark\",\"mark1\":$1,\"s\":200,\"mark2\":$2}";
//通知最高连杀
const std::string strNotifyMaxWin = "{\"cmd\":\"getGroupWins\",\"nums\":$C,\"s\":200,\"name\":\"$N\",\"camp\":$Z}";
//通知加入阵营战
const std::string strNotifyJoin = "{\"cmd\":\"updateGroupList\",\"s\":200,\"list\":[{\"name\":\"$N\",\"update\":2,\"state\":1,\"type\":$T,\"id\":$C,\"group\":$G}]}";
//通知阵营战状态变化
const std::string strNotifyState = "{\"cmd\":\"updateGroupList\",\"s\":200,\"list\":[{\"update\":1,\"state\":$S,\"type\":$T,\"id\":$C,\"group\":$G}]}";
//通知离开阵营战
const std::string strNotifyLeave = "{\"cmd\":\"updateGroupList\",\"s\":200,\"list\":[{\"update\":3,\"state\":1,\"type\":$T,\"id\":$C,\"group\":$G}]}";
//通知关闭阵营战
const std::string strNotifyClose = "{\"cmd\":\"closeGroupBattle\",\"s\":200}";

const std::string strNotifyCampRaceOpen = "{\"cmd\":\"updateAction\",\"type\":2,\"s\":200,\"active\":1}";
const std::string strNotifyCampRaceClose = "{\"cmd\":\"updateAction\",\"type\":2,\"s\":200,\"active\":0}";

//阵营战实际收益
void campRaceRealReward(int& get);

//随机选出战的人
charCampRace* campRaceWaitQue::_getRacer(bool high)
{
#ifdef CAMP_RACE_TWO_QUEUE    //是否划分为两个队列        
    if (high)
    {
        if (m_total_high > 0)
        {
            int iRand = my_random(1, m_total_high);
            for (int i = iCampRaceHighQueSize - 1; i >= 0; --i)
            {                
                if (iRand > m_camp_wait_que_high[i].size())
                {
                    iRand -= m_camp_wait_que_high[i].size();
                }
                else
                {
                    charCampRace* pCcr = m_camp_wait_que_high[i][iRand-1].get();
                    m_camp_wait_que_high[i].erase(m_camp_wait_que_high[i].begin() + iRand - 1);
                    if (m_total_high)
                    {
                        --m_total_high;
                    }
                    return pCcr;
                }
            }
        }
        return NULL;
    }
    else
    {
        if (m_total_low > 0)
        {
            int iRand = my_random(1, m_total_low);
            for (int i = 3; i >= 0; --i)
            {
                if (iRand > m_camp_wait_que_low[i].size())
                {
                    iRand -= m_camp_wait_que_low[i].size();
                }
                else
                {
                    charCampRace* pCcr = m_camp_wait_que_low[i][iRand-1].get();
                    m_camp_wait_que_low[i].erase(m_camp_wait_que_low[i].begin() + iRand - 1);
                    if (m_total_low)
                    {
                        --m_total_low;
                    }
                    return pCcr;
                }
            }
        }
        return NULL;
    }
#else
    (void)high;
    for (int i = iCampRaceQueSize - 1; i >= 0; --i)
    {
        //40%的概率下面一个队列的人出战
        int size1 = m_camp_wait_que[i].size();
        if (size1)
        {
            int size2 = (i > 0) ? m_camp_wait_que[i-1].size() : 0;
            int idx = i;
            if (size2 && my_random(1,100) <= 40)
            {
                idx = i - 1;
            }
            int iRand = my_random(0, m_camp_wait_que[idx].size() - 1);
            charCampRace* pCcr = m_camp_wait_que[idx][iRand].get();
            m_camp_wait_que[idx].erase(m_camp_wait_que[idx].begin() + iRand);
            if (m_total)
            {
                --m_total;
            }
            return pCcr;
        }
        else
        {
            
        }
    }
    return NULL;
#endif
}

void campRaceWaitQue::add(boost::shared_ptr<charCampRace> sp)
{
    charCampRace* ccr = sp.get();
    if (ccr
        && ccr->_in_combat == 0
        && (ccr->_state == camp_race_state_wait || ccr->_state == camp_race_state_fighting)
        && ccr->_cdata.get() && ccr->_idle_time <= 0)
    {
#ifdef CAMP_RACE_TWO_QUEUE    //是否划分为两个队列        
        if (ccr->_cdata->m_level > iCampRaceLevel_low)
        {
            size_t queIdx = 0;//(ccr->_cdata->m_level-iCampRaceLevel_low)/10;
            if (queIdx + 1 > (int)iCampRaceHighQueSize)
            {
                queIdx = iCampRaceHighQueSize - 1;
            }
            m_camp_wait_que_high[queIdx].push_back(sp);
            ++m_total_high;
        }
        else
        {
            size_t queIdx = (ccr->_cdata->m_level - 1)/10;
            if (queIdx > 3)
            {
                queIdx = 3;
            }
            m_camp_wait_que_low[queIdx].push_back(sp);
            ++m_total_low;
        }
#else
        int queIdx = 0;(ccr->_cdata->m_level - 1)/10;
        if (queIdx > (iCampRaceQueSize - 1))
        {
            queIdx = iCampRaceQueSize - 1;
        }
        else if (queIdx < 0)
        {
            queIdx = 0;
        }
        m_camp_wait_que[queIdx].push_back(sp);
        ++m_total;
#endif
    }
}

void campRaceWaitQue::clear()
{
#ifdef CAMP_RACE_TWO_QUEUE    //是否划分为两个队列        
    //先清空等待隊列
    for (int i = 0; i < 4; ++i)
    {
        m_camp_wait_que_low[i].clear();
    }
    for (int i = 0; i < iCampRaceHighQueSize; ++i)
    {
        m_camp_wait_que_high[i].clear();
    }
    m_total_high = 0;
    m_total_low = 0;
#else
    //先清空等待隊列
    for (int i = 0; i < iCampRaceQueSize; ++i)
    {
        m_camp_wait_que[i].clear();
    }
#endif
}

bool compare_camp_race_wins_ranking(const campRaceWinsRanking& a, const campRaceWinsRanking& b)
{
    return (a.wins >= b.wins);
}

void campRaceWinsRankings::updateRankings(std::map<int, int>& dmap)
{
    std::map<int, int>::iterator it = dmap.begin();
    while (it != dmap.end())
    {
        int wins = it->second;
        int cid = it->first;
        if (winsMap.size() < (size_t)iCampRaceRankingsCount
            || wins > min_wins)
        {
            if (winsMap[cid] < wins)
            {
                winsMap[cid] = wins;
                if (wins < min_wins)
                {
                    min_wins = wins;
                }
            }
        }
        ++it;
    }

    int pre_rank1 = 0;
    if (winsList.size())
    {
        pre_rank1 = winsList.begin()->cid;
    }
    
    // map -> list
    winsList.clear();
    for (it = winsMap.begin(); it != winsMap.end(); ++it)
    {
        campRaceWinsRanking dr;
        dr.cid = it->first;
        dr.wins = it->second;
        winsList.push_back(dr);
    }

    //sort list
    winsList.sort(compare_camp_race_wins_ranking);

    //去掉多餘的
    while (winsList.size() > (size_t)iCampRaceRankingsCount)
    {
        winsList.pop_back();
    }

    //最低连胜
    min_wins = 0;
    if (winsList.size())
    {
        min_wins = winsList.rbegin()->wins;

        {
            //第一名是否易主
            int new_rank1 = winsList.begin()->cid;
            if (new_rank1 != pre_rank1)
            {
                //廣播
                CharData* pc = GeneralDataMgr::getInstance()->GetCharData(new_rank1).get();
                if (pc)
                {
                    //XXX在阵营战中刷新了最高连胜纪录，夺得阵营风云榜首！
                    std::string msg = strNewCampRaceRank1Msg;
                    str_replace(msg, "$C", MakeCharNameLink(pc->m_name), true);
                    //str_replace(msg, "$D", LEX_CAST_STR(winsList.begin()->wins), true);
                    GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                }
            }
        }
    }

    winsMap.clear();

    for (int i = 0; i < iCampRaceRankingsPage; ++i)
    {
        strRangkingsPages[i] = "";
        aRankingsPages[i].clear();
    }

    InsertSaveDb("TRUNCATE TABLE char_camp_race_rankings");
    //生成每頁的array
    int count = 0;
    int maxPage = 0;
    memset(rankings, 0, sizeof(int)*iCampRaceRankingsCount);
    std::list<campRaceWinsRanking>::iterator itr = winsList.begin();
    while (itr != winsList.end())
    {
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(itr->cid).get();
        if (pc && itr->wins > 0)
        {
            if (count >= 0 && count < iCampRaceRankingsCount)
            {
                rankings[count] = pc->m_id;
            }
            int pageIdx = count /iCampRaceRankingsPerPage;
            maxPage = pageIdx + 1;
            json_spirit::Object obj;
            obj.push_back( Pair("rank", count + 1) );
            obj.push_back( Pair("name", pc->m_name) );
            json_spirit::Array a(pc->m_nick.m_nick_list.begin(), pc->m_nick.m_nick_list.end());
            obj.push_back( Pair("nick", a) );
            obj.push_back( Pair("level", pc->m_level) );
            obj.push_back( Pair("win", itr->wins) );
            obj.push_back( Pair("camp", pc->m_camp) );
            if (pc->m_horse.horse)
            {
                obj.push_back( Pair("hturn", pc->m_horse.horse->turn) );
                obj.push_back( Pair("hstar", pc->m_horse.horse->star) );
                obj.push_back( Pair("hname", pc->m_horse.horse->name) );
                obj.push_back( Pair("hquality", pc->m_horse.horse->quality) );
            }
            aRankingsPages[pageIdx].push_back(obj);
            ++count;

            InsertSaveDb("insert into `char_camp_race_rankings` set cid=" + LEX_CAST_STR(itr->cid) + ",wins=" + LEX_CAST_STR(itr->wins));
            //重新生成map
            winsMap[itr->cid] = itr->wins;
        }
        ++itr;
    }
    //生成每頁的string
    for (int i = 0; i < iCampRaceRankingsPage; ++i)
    {
        if (aRankingsPages[i].size())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "getCampRaceRankList") );
            obj.push_back( Pair("s", 200) );
            json_spirit::Object pageobj;
            pageobj.push_back( Pair("maxPage", maxPage) );
            pageobj.push_back( Pair("page", i + 1) );
            pageobj.push_back( Pair("pageNums", iCampRaceRankingsPerPage) );

            obj.push_back( Pair("list", aRankingsPages[i]) );
            obj.push_back( Pair("page", pageobj) );

            strRangkingsPages[i] = json_spirit::write(obj);
        }
    }
}

//更新排行榜活动中的cid字段
void campRaceWinsRankings::updateRankingsEvent(rankings_event* pE)
{
    //cout<<"splsRankings::updateRankingsEvent(),type="<<pE->type<<endl;
    for (std::list<rankings_event_award>::iterator it = pE->rankings_list.begin(); it != pE->rankings_list.end(); ++it)
    {
        //cout<<"rank:"<<it->rank<<endl;
        if (it->rank > 0 && it->rank <= iCampRaceRankingsCount)
        {
            it->cid = rankings[it->rank-1];
            //cout<<"\tcid->"<<it->cid<<endl;
        }
    }
}

std::string campRaceWinsRankings::getRankings(int page)
{
    if (page <= 0 || page > iCampRaceRankingsPage)
    {
        page = 1;
    }
    if (strRangkingsPages[page-1] != "")
    {
        return strRangkingsPages[page-1];
    }
    else
    {
        return "{\"cmd\":\"getCampRaceRankList\",\"s\":201}";
    }
}

campRaceMgr* campRaceMgr::m_handle = NULL;
campRaceMgr* campRaceMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new campRaceMgr();        
        m_handle->reload();
        std::string memo;
        m_handle->getActionMemo(memo);
    }
    return m_handle;
}

campRaceMgr::campRaceMgr()
:m_stand_in_mob(baby_camp_race)
{
    //_channel.reset(new ChatChannel("_campRace", 1, "{\"cmd\":\"chat\",\"ctype\":10,\"s\":200,"));
    _open_state = 0;

    _start_time = 0;    //开启时间戳
    _end_time = 0;        //结束时间戳

    _max_winner = "";    //最多连胜玩家
    _max_win_times = 0;    //最多连胜次数
    _max_win_camp = 0;

    _score[0] = 0;        //双方积分
    _score[1] = 0;

    _str_will_open_msg = "";
    _str_have_closed = "";
    _open_time = "";

    _last_rule = NULL;
    _uuid_race = boost::uuids::nil_uuid();
    _uuid = boost::uuids::nil_uuid();

    _str_have_closed = strCampRaceIsClosed;    
}

int campRaceMgr::reload()
{
    Query q(GetDb());

    q.get_result("select season,day,month,hour,minute,week from custom_shedule where type='openCampRace'");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        std::string season = q.getstr();
        std::string day = q.getstr();
        std::string month = q.getstr();
        std::string hour = q.getstr();
        std::string minute = q.getstr();
        std::string week = q.getstr();
        _open_rules.addRule(season, month, day, week, hour, minute);
    }
    q.free_result();

    std::map<int, int> temp_maps;
    //连胜排名
    q.get_result("select cid,wins from char_camp_race_rankings where 1 order by wins desc,id limit " + LEX_CAST_STR(iCampRaceRankingsCount));
    while (q.fetch_row())
    {
        int cid = q.getval();
        int wins = q.getval();
        
        temp_maps[cid] = wins;
    }
    q.free_result();
    m_rankings.updateRankings(temp_maps);

    return HC_SUCCESS;
}

boost::shared_ptr<charCampRace> campRaceMgr::getChar(int camp, int cid)
{
    std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_total_data_map.find(cid);
    if (it != m_total_data_map.end())
    {
        return it->second;
    }
    /*if (camp == 2 || camp == 1)
    {
        std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_data_map[camp-1].find(cid);
        if (it != m_data_map[camp-1].end())
        {
            return it->second;
        }
    }*/
    boost::shared_ptr<charCampRace> empty;
    return empty;
}

void campRaceMgr::getJoinCount(int camp, int& count1, int& count2)
{
    count1 = 0;
    count2 = 0;
    std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_data_map[camp-1].begin();
    while (it != m_data_map[camp-1].end())
    {
        charCampRace* ccr = it->second.get();
        if (ccr && ccr->_cdata.get() && ccr->_cdata->m_level <= iCampRaceLevel_low)
        {
            ++count1;
        }
        else
        {
            ++count2;
        }
        ++it;
    }
    return;
}

//加入阵营战
int campRaceMgr::joinRace(boost::shared_ptr<CharData> cdata, boost::shared_ptr<OnlineCharactor> ou, json_spirit::Object& robj)
{    
    INFO("####################### join race "<<cdata->m_id<<endl);

    boost::shared_ptr<charCampRace> spCcr = getChar(cdata->m_camp, cdata->m_id);
    if (spCcr.get())
    {
        spCcr->_onlineChar = ou;
        if (spCcr->_state == camp_race_state_leave)
        {
            if (m_data_map[0].size() > m_data_map[1].size())
            {
                cdata->m_camp = 2;
                m_data_map[1][cdata->m_id] = spCcr;
            }
            else
            {
                cdata->m_camp = 1;
                m_data_map[0][cdata->m_id] = spCcr;
            }
            spCcr->_state = camp_race_state_idle;
            memset(spCcr->m_generals_hp, 0, sizeof(int)*9);

            std::string msg = strNotifyJoin;
            str_replace(msg, "$N", cdata->m_name);
            str_replace(msg, "$T", LEX_CAST_STR(cdata->m_camp));
            str_replace(msg, "$C", LEX_CAST_STR(cdata->m_id));
            str_replace(msg, "$G", (cdata->m_level > iCampRaceLevel_low ? "2" : "1"));
            broadMsg(msg);
        }
    }
    else
    {
        int camp1_low = 0, camp1_high = 0, camp2_low = 0, camp2_high = 0;
        getJoinCount(1, camp1_low, camp1_high);
        getJoinCount(2, camp2_low, camp2_high);
        if (cdata->m_level <= iCampRaceLevel_low)
        {
            if (camp1_low > camp2_low)
            {
                cdata->m_camp = 2;
            }
            else
            {
                cdata->m_camp = 1;
            }
        }
        else
        {            
            if (camp1_high > camp2_high)
            {
                cdata->m_camp = 2;
            }
            else
            {
                cdata->m_camp = 1;
            }
        }
        _join(cdata, ou);

        std::string msg = strNotifyJoin;
        str_replace(msg, "$N", cdata->m_name);
        str_replace(msg, "$T", LEX_CAST_STR(cdata->m_camp));
        str_replace(msg, "$C", LEX_CAST_STR(cdata->m_id));
        str_replace(msg, "$G", (cdata->m_level > iCampRaceLevel_low ? "2" : "1"));
        broadMsg(msg);
    }
    return HC_SUCCESS;
}

int campRaceMgr::_join(boost::shared_ptr<CharData> cdata, boost::shared_ptr<OnlineCharactor> ou)
{
    boost::shared_ptr<charCampRace> spCcr(new charCampRace);
    spCcr->_cid = cdata->m_id;
    spCcr->_wins = 0;
    spCcr->_loses = 0;
    spCcr->_skips = 0;
    spCcr->_max_continue_wins = 0;
    spCcr->_cur_continue_wins = 0;
    spCcr->_prestige = 0;
    spCcr->_silver = 0;
    spCcr->_state = camp_race_state_idle;
    spCcr->_idle_time = 0;
    spCcr->_cdata = cdata;
    spCcr->_onlineChar = ou;
    spCcr->_in_combat = 0;
    memset(spCcr->m_generals_hp, 0, 9 * sizeof(int));

    m_data_map[cdata->m_camp-1][cdata->m_id] = spCcr;
    m_total_data_map[cdata->m_id] = spCcr;
    //_channel->Add(ou);
    return HC_SUCCESS;
}

//离开阵营战
int campRaceMgr::leaveRace(CharData* pc, boost::shared_ptr<OnlineCharactor> ou)
{
    if (!_open_state)
    {
        return HC_SUCCESS;
    }
    if (!pc)
    {
        return HC_ERROR;
    }
    INFO("####################### leave race "<<pc->m_id<<endl);

    if (pc->m_camp > 2 || pc->m_camp < 1)
    {
        return HC_ERROR_CAMP_RACE_NO_CAMP;
    }
    boost::shared_ptr<charCampRace> spCcr = getChar(pc->m_camp, pc->m_id);
    if (!spCcr.get() || spCcr->_state == camp_race_state_leave)
    {
        return HC_ERROR_CAMP_RACE_NOT_IN;
    }
    json_spirit::Object tempObj;
    campRaceMgr::getInstance()->setFight(pc, false, tempObj);
    spCcr->_state = camp_race_state_leave;
    spCcr->_onlineChar.reset();
    m_data_map[pc->m_camp-1].erase(pc->m_id);

    std::string msg = strNotifyLeave;
    str_replace(msg, "$T", LEX_CAST_STR(spCcr->_cdata->m_camp));
    str_replace(msg, "$C", LEX_CAST_STR(spCcr->_cdata->m_id));
    str_replace(msg, "$G", (spCcr->_cdata->m_level > iCampRaceLevel_low ? "2" : "1"));
    broadMsg(msg);
    //_channel->Remove(ou);
    return HC_SUCCESS;
}

//参战/取消参战
int campRaceMgr::setFight(CharData* pc, bool enable, json_spirit::Object& robj)
{
    if (!pc)
    {
        return HC_ERROR;
    }
    if (pc->m_camp > 2 || pc->m_camp < 1)
    {
        return HC_ERROR_CAMP_RACE_NO_CAMP;
    }
    boost::shared_ptr<charCampRace> spCcr = getChar(pc->m_camp, pc->m_id);
    if (!spCcr.get() || spCcr->_state == camp_race_state_leave)
    {
        return HC_ERROR_CAMP_RACE_NOT_IN;
    }
    if (enable && camp_race_state_idle == spCcr->_state)
    {
        spCcr->_state = camp_race_state_wait;
        CharZhens& zhens = spCcr->_cdata->GetZhens();
        boost::shared_ptr<ZhenData> zdata = zhens.GetZhen(zhens.GetDefault());
        if (zdata.get())
        {
            CharTotalGenerals& cg = spCcr->_cdata->GetGenerals();
            for (size_t i = 0; i < 9; ++i)
            {
                spCcr->m_generals[i].reset();
                if (zdata->m_generals[i] > 0)
                {
                    spCcr->m_generals[i] = cg.GetGenral(zdata->m_generals[i]);
                }
            }
        }
        else
        {
            ERR();
        }
        std::string msg = strNotifyState;
        str_replace(msg, "$S", "2");
        str_replace(msg, "$T", LEX_CAST_STR(pc->m_camp));
        str_replace(msg, "$C", LEX_CAST_STR(pc->m_id));
        str_replace(msg, "$G", (pc->m_level > iCampRaceLevel_low ? "2" : "1"));
        broadMsg(msg);
    }
    else if (!enable)
    {
        spCcr->_state = camp_race_state_idle;
        //补充血量
        memset(spCcr->m_generals_hp, 0, sizeof(int)*9);
        spCcr->_cur_continue_wins = 0;
        std::string msg = strNotifyState;
        str_replace(msg, "$S", "1");
        str_replace(msg, "$T", LEX_CAST_STR(pc->m_camp));
        str_replace(msg, "$C", LEX_CAST_STR(pc->m_id));
        str_replace(msg, "$G", (pc->m_level > iCampRaceLevel_low ? "2" : "1"));
        broadMsg(msg);
    }
    return HC_SUCCESS;
}

//鼓舞
int campRaceMgr::inspire(CharData* pc, int type, json_spirit::Object& robj)
{
    if (NULL == pc)
    {
        robj.push_back( Pair("curGx", pc->getGongxun()) );    
        return HC_ERROR;
    }
    int level = m_inspire_map[pc->m_id];
    //功勋鼓舞
    if (type == 2)
    {
        if (level >= 5)
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_BOSS_INSPIRE_MAX;    //鼓舞顶级了
        }
        //功勋是否足够
        if (pc->addGongxun(-200) < 0)
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_NOT_ENOUGH_GONGXUN;
        }
        add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_gongxun,200,treasure_inspire,2,pc->m_union_id,pc->m_server_id);
        add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_gongxun,200,treasure_inspire_camprace,2,pc->m_union_id,pc->m_server_id);
        pc->NotifyCharData();
        //有概率
        if (my_random(1,100) <= iInspireSilver[level])
        {
            m_inspire_map[pc->m_id] = level + 1;
        }
        else
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_BOSS_INSPIRE_FAIL;    //失败了
        }
    }
    else
    {
        if (level >= 10)
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_BOSS_INSPIRE_MAX;    //鼓舞顶级了
        }
        if (pc->m_vip < iInspireGoldVIPLevel)
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        //金币是否足够
        if (pc->addGold(-20) < 0)
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        //add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 20, gold_cost_for_inspire, pc->m_union_id, pc->m_server_id);
        add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 20, gold_cost_for_inspire_camprace, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(pc,20,gold_cost_for_inspire_camprace);
#endif
        pc->NotifyCharData();
        int rands[] = {100,100,100,100,100,30,20,10,5,5};
        //有概率
        if (my_random(1,100) <= rands[level])
        {
            m_inspire_map[pc->m_id] = level + 1;
        }
        else
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_BOSS_INSPIRE_FAIL;    //失败了
        }
    }
    if (m_inspire_map[pc->m_id] > 5)
    {
        pc->m_combat_attribute.camp_inspired((m_inspire_map[pc->m_id] - 5)*10 + 100);
    }
    else
    {
        pc->m_combat_attribute.camp_inspired(m_inspire_map[pc->m_id]*20);
    }

    robj.push_back( Pair("level", m_inspire_map[pc->m_id]) );
    robj.push_back( Pair("damage", pc->m_combat_attribute.camp_inspired()) );
    robj.push_back( Pair("hp", 3*pc->m_combat_attribute.camp_inspired()/2) );

    robj.push_back( Pair("gold", 20) );
    robj.push_back( Pair("gx", 200) );

    robj.push_back( Pair("curGx", pc->getGongxun()) );    

    std::string suc_msg = strInspireCampRaceSuccess;
    if (m_inspire_map[pc->m_id] > 5)
    {
        str_replace(suc_msg, "$N", LEX_CAST_STR(10));
        str_replace(suc_msg, "$H", LEX_CAST_STR(15));
    }
    else
    {
        str_replace(suc_msg, "$N", LEX_CAST_STR(20));
        str_replace(suc_msg, "$H", LEX_CAST_STR(30));
    }
    robj.push_back( Pair("msg", suc_msg) );
    return HC_SUCCESS;
}

int campRaceMgr::getInspire(CharData* pc)
{
    if (pc != NULL && m_inspire_map.find(pc->m_id) != m_inspire_map.end())
    {
        return m_inspire_map[pc->m_id];
    }
    return 0;
}

//捉对厮杀
int campRaceMgr::Race()
{
    if (!_open_state)
    {
        splsTimerMgr::getInstance()->delTimer(_uuid_race);
        _uuid_race = boost::uuids::nil_uuid();
        return HC_ERROR;
    }
    json_spirit::Array stateChange;
    updateWaitQue(stateChange);
    //INFO("11111111111 campRaceMgr::Race()..."<<endl);
    charCampRace* ar = NULL, *br = NULL;
    for (;;)
    {
        if (my_random(1, 100) <= 50)
        {
            ar = m_guanfu_que._getRacer(false);
            br = m_lvlin_que._getRacer(false);
        }
        else
        {
            br = m_lvlin_que._getRacer(false);
            ar = m_guanfu_que._getRacer(false);
        }
        if (ar && br)
        {
            _Race(ar, br, stateChange);            
        }
        else
        {
            break;
        }
    }
    //轮空处理
    if (ar)
    {
        do
        {
            noMatch(ar);
            ar = m_guanfu_que._getRacer(false);
        } while (ar);
    }
    if (br)
    {
        do
        {
            noMatch(br);
            br = m_lvlin_que._getRacer(false);
        } while (br);
    }

    //INFO("222222222222 campRaceMgr::Race()..."<<endl);
    for (;;)
    {
        if (my_random(1, 100) <= 50)
        {
            ar = m_guanfu_que._getRacer(true);
            br = m_lvlin_que._getRacer(true);
        }
        else
        {
            br = m_lvlin_que._getRacer(true);
            ar = m_guanfu_que._getRacer(true);
        }
        if (ar && br)
        {
            _Race(ar, br, stateChange);            
        }
        else
        {
            break;
        }
    }
    //轮空处理
    if (ar)
    {
        do
        {
            noMatch(ar);
            ar = m_guanfu_que._getRacer(true);
        } while (ar);
    }
    if (br)
    {
        do
        {
            noMatch(br);
            br = m_lvlin_que._getRacer(true);
        } while (br);
    }

    //广播状态发生变化
    json_spirit::Object obj;            
    obj.push_back( Pair("cmd", "updateGroupList") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("list", stateChange) );
    std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
    broadMsg(msg);
    return HC_SUCCESS;
}

//轮空处理
void campRaceMgr::noMatch(charCampRace* ccr)
{
    //轮空只是获得一些银币，加一点冷却时间
    ccr->_idle_time = 0;
    ++ccr->_skips;
    int silver = 25 * ccr->_cdata->m_level;
    ccr->_silver += silver;
    ccr->_cdata->addSilver(silver);
    add_statistics_of_silver_get(ccr->_cdata->m_id,ccr->_cdata->m_ip_address,silver,silver_get_campRace, ccr->_cdata->m_union_id, ccr->_cdata->m_server_id);
    //int prestige = 0;
    if (ccr->_cdata->m_level > iCampRaceLevel_low)
    {
        _reports2.addReport(camp_race_report_skip, ccr->_cid, 0, ccr->_cdata->m_name, "", silver, 0, 0, ccr->_cdata->m_camp, 0);
    }
    else
    {
        _reports.addReport(camp_race_report_skip, ccr->_cid, 0, ccr->_cdata->m_name, "", silver, 0, 0, ccr->_cdata->m_camp, 0);
    }    
    if (ccr->_onlineChar.get())
    {
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "addGroupReport") );
        obj.push_back( Pair("s", 200) );
        obj.push_back( Pair("type", camp_race_report_skip) );
        obj.push_back( Pair("camp1", ccr->_cdata->m_camp) );
        obj.push_back( Pair("id1", ccr->_cid) );
        obj.push_back( Pair("name1", ccr->_cdata->m_name) );
        obj.push_back( Pair("silver", silver) );
        //obj.push_back( Pair("prestige", prestige) );
        ccr->_onlineChar->Send(json_spirit::write(obj, json_spirit::raw_utf8));
    }
}

int campRaceMgr::_Race(charCampRace * a1,charCampRace * a2, json_spirit::Array& stateChange)
{
    //cout<<"campRaceMgr::_Race()"<<endl;
    charCampRace* attacker = NULL, *defender = NULL;
    //确定攻击方和防守方
    if (a1->_cur_continue_wins > a2->_cur_continue_wins)
    {
        attacker = a2;
        defender = a1;
    }
    else if (a1->_cur_continue_wins == a2->_cur_continue_wins)
    {
        if (my_random(1, 100) <= 50)
        {
            attacker = a1;
            defender = a2;
        }
        else
        {
            attacker = a2;
            defender = a1;
        }
    }
    else
    {
        attacker = a1;
        defender = a2;
    }

    int ret = HC_SUCCESS;
    Combat* pCombat = createCampRaceCombat(attacker, defender, ret);
    if (HC_SUCCESS == ret && pCombat)
    {
        json_spirit::Object robj;
        robj.push_back( Pair("cmd", "attack") );
        robj.push_back( Pair("s", 200) );
        robj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
        std::string msg = json_spirit::write(robj, json_spirit::raw_utf8);
        //立即返回战斗双方的信息
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(a1->_cdata->m_name);
        if (account.get())
        {
            account->Send(msg);
        }
        boost::shared_ptr<OnlineCharactor> account2 = GeneralDataMgr::getInstance()->GetOnlineCharactor(a2->_cdata->m_name);
        if (account2.get())
        {
            account2->Send(msg);
        }        
        //向战斗双方发送战报
        InsertCombat(pCombat);
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(attacker->_cid);
        if (cdata.get())
        {
            //日常任务
            dailyTaskMgr::getInstance()->updateDailyTask(*(cdata.get()),daily_task_camprace);
            //act统计
            act_to_tencent(cdata.get(),act_new_camp_race_battle);
        }
        cdata = GeneralDataMgr::getInstance()->GetCharData(defender->_cid);
        if (cdata.get())
        {
            //日常任务
            dailyTaskMgr::getInstance()->updateDailyTask(*(cdata.get()),daily_task_camprace);
            //act统计
            act_to_tencent(cdata.get(),act_new_camp_race_battle);
        }
    }    
    //广播状态发生变化
    json_spirit::Object obj1;
    obj1.push_back( Pair("type", attacker->_cdata->m_camp) );
    obj1.push_back( Pair("update", 1) );
    obj1.push_back( Pair("state", camp_race_state_fighting) );
    obj1.push_back( Pair("id", attacker->_cid) );
    obj1.push_back( Pair("group", (attacker->_cdata->m_level > iCampRaceLevel_low ? 2 : 1)) );
    stateChange.push_back(obj1);

    json_spirit::Object obj2;
    obj2.push_back( Pair("type", defender->_cdata->m_camp) );
    obj2.push_back( Pair("update", 1) );
    obj2.push_back( Pair("state", camp_race_state_fighting) );
    obj2.push_back( Pair("id", defender->_cid) );
    obj2.push_back( Pair("group", (defender->_cdata->m_level > iCampRaceLevel_low ? 2 : 1)) );
    stateChange.push_back(obj2);

    attacker->_state = camp_race_state_fighting;
    defender->_state = camp_race_state_fighting;

    //2.12胜利失败都不轮空，看战斗也可匹配
    attacker->_idle_time = 1;    //战斗后，失败方跳过一次匹配,+1
    defender->_idle_time = 1;
    attacker->_in_combat = 0;
    defender->_in_combat = 0;
    return HC_SUCCESS;
}

void campRaceMgr::updateWaitQue(json_spirit::Array& stateChange)
{
    //先清空等待隊列
    m_guanfu_que.clear();
    m_lvlin_que.clear();
    for (int i = 0; i < 2; ++i)
    {
        std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_data_map[i].begin();
        while (it != m_data_map[i].end())
        {
            charCampRace* ccr = it->second.get();
            if (ccr)
            {
                if (ccr->_idle_time)
                {
                    --ccr->_idle_time;
                }
                //战斗状态下冷却时间未到了变为等待状态
                if (ccr->_state == camp_race_state_fighting && ccr->_idle_time > 0)
                {
                    ccr->_state = camp_race_state_wait;
                    //广播状态发生变化
                    json_spirit::Object obj1;
                    //obj1.push_back( Pair("cmd", "addGroupReport") );
                    //obj1.push_back( Pair("s", 200) );
                    obj1.push_back( Pair("type", ccr->_cdata->m_camp) );
                    obj1.push_back( Pair("update", 1) );
                    obj1.push_back( Pair("state", camp_race_state_wait) );
                    obj1.push_back( Pair("id", ccr->_cid) );
                    obj1.push_back( Pair("group", (ccr->_cdata->m_level > iCampRaceLevel_low ? 2 : 1)) );
                    stateChange.push_back(obj1);
                }
                else
                {
                    if (0 == i)
                    {
                        m_guanfu_que.add(it->second);
                    }
                    else
                    {
                        m_lvlin_que.add(it->second);
                    }
                }
            }        
            ++it;
        }
    }
}

int campRaceMgr::combatResult(Combat* pCombat)    //战斗结束
{
    //cout<<"################ campRaceMgr::combatResult() ################################"<<endl;
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<charCampRace> attacker = getChar(pCombat->m_attacker->_army_data->m_camp, pCombat->m_attacker->getCharId());
    boost::shared_ptr<charCampRace> defender = getChar(pCombat->m_defender->_army_data->m_camp, pCombat->m_defender->getCharId());
    charCampRace* winner = (attacker_win == pCombat->m_state) ? attacker.get() : defender.get();
    charCampRace* loser = (defender_win == pCombat->m_state) ? attacker.get() : defender.get();
    if (winner && loser)
    {
        //cout<<"############### campRaceMgr::combatResult(2) #############################"<<endl;
        campRaceReportList* pReport = winner->_cdata->m_level > iCampRaceLevel_low ? &_reports2 : &_reports;
        //胜利方奖励 + 胜利方连胜奖励
        ++winner->_cur_continue_wins;
        ++winner->_wins;

        if (winner->_cur_continue_wins > winner->_max_continue_wins)
        {
            winner->_max_continue_wins = winner->_cur_continue_wins;
        }
        if (winner->_cur_continue_wins >= _max_win_times)
        {
            //更新最高连胜
            _max_win_times = winner->_cur_continue_wins;
            _max_winner = winner->_cdata->m_name;
            _max_win_camp = winner->_cdata->m_camp;
            
            std::string msg = strNotifyMaxWin;
            str_replace(msg, "$C", LEX_CAST_STR(_max_win_times));
            str_replace(msg, "$N", _max_winner);
            str_replace(msg, "$Z", LEX_CAST_STR(_max_win_camp));
            broadMsg(msg);
            
            INFO("############################ broadMsg new max win ##########################"<<endl);
        }
        //单场银币最多500*主将等级
        int winner_silver = winner->_cur_continue_wins > 10 ? 500 * winner->_cdata->m_level  :50 * winner->_cdata->m_level * winner->_cur_continue_wins;
        int winner_prestige = 5 * winner->_cur_continue_wins;
        
        //阵营战实际收益
        campRaceRealReward(winner_silver);
        campRaceRealReward(winner_prestige);

        int maxPrestige = iCampRaceMaxPrestige;
        campRaceRealReward(maxPrestige);

        if (winner->_prestige + winner_prestige > maxPrestige)
        {
            winner_prestige = maxPrestige - winner->_prestige;
        }
        winner->_prestige += winner_prestige;
        winner->_silver += winner_silver;

        Army* winner_army = (attacker_win == pCombat->m_state) ? pCombat->m_attacker : pCombat->m_defender;
        //更新剩余血量
        for (int i = 1; i <= 9; ++i)
        {
            iGeneral* g = winner_army->GetGeneral(i);
            if (g)
            {
                int cur_hp = g->Hp();
                if (cur_hp)
                {
                    winner->m_generals_hp[i-1] = g->MaxHp() - g->Hp();
                    //cout<<"pos "<<i<<" hurt "<<winner->m_generals_hp[i-1]<<"/"<<g->MaxHp()<<endl;
                }
                else
                {
                    winner->m_generals_hp[i-1] = -1;
                    //cout<<"pos "<<i<<" die"<<endl;
                }
            }
            else
            {
                winner->m_generals_hp[i-1] = -1;
                //cout<<"pos "<<i<<" die"<<endl;
            }
        }

        //胜利信息
        pReport->addReport(camp_race_report_win, winner->_cid, loser->_cid, winner->_cdata->m_name, loser->_cdata->m_name, winner_silver, winner_prestige, winner->_cur_continue_wins, winner->_cdata->m_camp, loser->_cdata->m_camp);
        {
            json_spirit::Object obj;            
            obj.push_back( Pair("cmd", "addGroupReport") );
            obj.push_back( Pair("s", 200) );            
            obj.push_back( Pair("type", camp_race_report_win) );
            obj.push_back( Pair("camp1", winner->_cdata->m_camp) );
            obj.push_back( Pair("camp2", loser->_cdata->m_camp) );
            obj.push_back( Pair("id1", winner->_cid) );
            obj.push_back( Pair("id2", loser->_cid) );
            obj.push_back( Pair("name1", winner->_cdata->m_name) );
            obj.push_back( Pair("name2", loser->_cdata->m_name) );
            obj.push_back( Pair("silver", winner_silver) );
            obj.push_back( Pair("prestige", winner_prestige) );
            obj.push_back( Pair("killNums", winner->_cur_continue_wins) );

            std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
            if (winner->_cdata->m_level > iCampRaceLevel_low)
            {
                broadMsg(iCampRaceLevel_low + 1, 200, msg);
            }
            else
            {
                broadMsg(0, iCampRaceLevel_low, msg);
            }
        }
        
        //胜利方终结连胜奖励
        int end_prestige = loser->_cur_continue_wins >= 2 ? (loser->_cur_continue_wins* 10) : 0;
        //阵营战实际收益
        campRaceRealReward(end_prestige);
        if (end_prestige + winner->_prestige > maxPrestige)
        {
            end_prestige = maxPrestige - winner->_prestige;
        }
        winner->_prestige += end_prestige;
        winner_prestige += end_prestige;
        //终结连胜信息
        if (loser->_cur_continue_wins >= 2)
        {
            pReport->addReport(camp_race_report_end, winner->_cid, loser->_cid, winner->_cdata->m_name, loser->_cdata->m_name, 0, end_prestige, loser->_cur_continue_wins, winner->_cdata->m_camp, loser->_cdata->m_camp);
            json_spirit::Object obj;
            
            obj.push_back( Pair("cmd", "addGroupReport") );
            obj.push_back( Pair("s", 200) );            
            obj.push_back( Pair("type", camp_race_report_end) );
            obj.push_back( Pair("camp1", winner->_cdata->m_camp) );
            obj.push_back( Pair("camp2", loser->_cdata->m_camp) );
            obj.push_back( Pair("id1", winner->_cid) );
            obj.push_back( Pair("id2", loser->_cid) );
            obj.push_back( Pair("name1", winner->_cdata->m_name) );
            obj.push_back( Pair("name2", loser->_cdata->m_name) );
            obj.push_back( Pair("prestige", end_prestige) );
            obj.push_back( Pair("killNums", loser->_cur_continue_wins) );

            std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
            if (winner->_cdata->m_level > iCampRaceLevel_low)
            {
                broadMsg(iCampRaceLevel_low + 1, 200, msg);
            }
            else
            {
                broadMsg(0, iCampRaceLevel_low, msg);
            }
        }

        Item item_silver;
        item_silver.type = item_type_silver;
        item_silver.nums = winner_silver;

        Item item_p;
        item_p.type = item_type_prestige;
        item_p.nums = winner_prestige;

        //失败方奖励
        loser->_cur_continue_wins = 0;
        ++loser->_loses;
        int loser_prestige = 1;
        //阵营战实际收益
        campRaceRealReward(loser_prestige);

        int loser_silver = 10 * loser->_cdata->m_level;
        //阵营战实际收益
        campRaceRealReward(loser_silver);
        if (loser->_prestige + loser_prestige > maxPrestige)
        {
            loser_prestige = maxPrestige - loser->_prestige;
        }
        loser->_prestige += loser_prestige;
        loser->_silver += loser_silver;
        loser->_state = camp_race_state_idle;    //打败了，变成空闲
        {
            std::string msg = strNotifyState;
            str_replace(msg, "$S", "1");
            str_replace(msg, "$T", LEX_CAST_STR(loser->_cdata->m_camp));
            str_replace(msg, "$C", LEX_CAST_STR(loser->_cid));
            str_replace(msg, "$G", (loser->_cdata->m_level > iCampRaceLevel_low ? "2" : "1"));
            broadMsg(msg);
        }
        //补充血量
        memset(loser->m_generals_hp, 0, sizeof(int)*9);
        //战败信息
        pReport->addReport(camp_race_report_fail, loser->_cid, 0, loser->_cdata->m_name, "", loser_silver, loser_prestige, loser->_wins, loser->_cdata->m_camp, 0);
        if (loser->_onlineChar.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "addGroupReport") );
            obj.push_back( Pair("s", 200) );
            obj.push_back( Pair("type", camp_race_report_fail) );
            obj.push_back( Pair("camp1", loser->_cdata->m_camp) );
            obj.push_back( Pair("id1", loser->_cid) );
            obj.push_back( Pair("name1", loser->_cdata->m_name) );
            obj.push_back( Pair("silver", loser_silver) );
            obj.push_back( Pair("prestige", loser_prestige) );
            loser->_onlineChar->Send(json_spirit::write(obj, json_spirit::raw_utf8));
        }
        

        Item item_silver2;
        item_silver2.type = item_type_silver;
        item_silver2.nums = loser_silver;

        Item item_p2;
        item_p2.type = item_type_prestige;
        item_p2.nums = loser_prestige;

        //更新双方积分
        if (winner->_cdata->m_camp == 1 || winner->_cdata->m_camp == 2)
        {
            //cout<<"####################### ++score "<<winner->_cdata->m_camp<<","<<winner_prestige<<endl;
            _score[winner->_cdata->m_camp - 1] += winner_prestige;
        }
        if (loser->_cdata->m_camp == 1 || loser->_cdata->m_camp == 2)
        {
            //cout<<"####################### ++score "<<loser->_cdata->m_camp<<","<<loser_prestige<<endl;
            _score[loser->_cdata->m_camp - 1] += loser_prestige;
        }
        {
            std::string msg = strNotifyScore;
            str_replace(msg, "$1", LEX_CAST_STR(_score[0]));
            str_replace(msg, "$2", LEX_CAST_STR(_score[1]));
            broadMsg(msg);
        }

        //双方获得战利品
        if (pCombat->m_state == attacker_win)
        {
            pCombat->m_getItems.push_back(item_p);
            pCombat->m_getItems.push_back(item_silver);
            giveLoots(winner->_cdata, pCombat, true, give_campRace_loot);

            pCombat->m_getItems2.push_back(item_p2);
            pCombat->m_getItems2.push_back(item_silver2);
            giveLoots(loser->_cdata, pCombat, false, give_campRace_loot);
        }
        else
        {
            pCombat->m_getItems2.push_back(item_p);
            pCombat->m_getItems2.push_back(item_silver);
            giveLoots(winner->_cdata, pCombat, false, give_campRace_loot);

            pCombat->m_getItems.push_back(item_p2);
            pCombat->m_getItems.push_back(item_silver2);
            giveLoots(loser->_cdata, pCombat, true, give_campRace_loot);
        }
        //刷新状态
        //winner->_cdata.get()->refreshStates(0, 0);
        //loser->_cdata.get()->refreshStates(0, 0);

        //胜利方可以继续战斗 - wlj 2012/2/2
        winner->_idle_time = 1;

    }
    //战斗结果    
    if (winner->_onlineChar.get() || loser->_onlineChar.get())
    {
        pCombat->AppendResult(pCombat->m_result_obj);
        //战报存/发送
        InsertSaveCombat(pCombat);        
    }
    return HC_SUCCESS;
}

int campRaceMgr::open(int last_mins)        //开启
{
    if (_open_state == 0)
    {
        _open_state = 1;
        //std::string msg = strBossOpenMsg;
        //str_replace(msg, "$B", _boss._name);
        GeneralDataMgr::getInstance()->broadCastSysMsg(strCampRaceOpenMsg, -1);

        GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(strNotifyCampRaceOpen, iCampraceOpenStronghold/2+1);

        _start_time = time(NULL);
        _end_time = _start_time + last_mins * 60;

        json_spirit::mObject mobj;
        mobj["cmd"] = "scheduleEvent";
        mobj["event"] = "closeCampRace";

        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(last_mins*60, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);

        boost::uuids::uuid _uuid_race;    //定时器唯一id - 定时匹配阵营战对手的定时器

        mobj.clear();
        mobj["cmd"] = "matchCampRace";

        boost::shared_ptr<splsTimer> tmsg_race;
        tmsg_race.reset(new splsTimer(iCampRaceMatchPeriod, 1, mobj, last_mins*60 /iCampRaceMatchPeriod));
        _uuid_race = splsTimerMgr::getInstance()->addTimer(tmsg_race);

        //替身娃娃扣金币
        m_stand_in_mob.processGold();
    }

    return HC_SUCCESS;
}

static bool compare_campRaceData(boost::shared_ptr<charCampRace>& a, boost::shared_ptr<charCampRace>& b)
{
    return a.get() && b.get() && a->_prestige > b->_prestige;
}

int campRaceMgr::close()    //关闭
{
    _open_state = 0;
    //splsTimerMgr::getInstance()->delTimer(_uuid);
    _uuid = boost::uuids::nil_uuid();

    GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(strNotifyCampRaceClose, iCampraceOpenStronghold/2+1);

    //广播结束消息
    std::string closeMsg = strNotifyClose;
#if 0
    std::string msg = "";
    if (_score[0] > _score[1])
    {
        msg = strCampRaceEndMsg;
        str_replace(msg, "$C", strCamp1Name);
        str_replace(closeMsg, "$1", "1");
    }
    else if (_score[0] < _score[1])
    {
        msg = strCampRaceEndMsg;
        str_replace(msg, "$C", strCamp2Name);
        str_replace(closeMsg, "$1", "2");
    }
    else
    {
        msg = strCampRaceDrawMsg;
        str_replace(closeMsg, "$1", "0");
    }
    GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
#endif
    broadMsg(closeMsg);

    //清除鼓舞效果
    for (int i = 0; i < 2; ++i)
    {
        std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_data_map[i].begin();
        while (it != m_data_map[i].end())
        {
            if (it->second.get() && (it->second->_wins + it->second->_loses) && it->second->_cdata.get())
            {
                m_inspire_map.erase(it->first);
                it->second->_cdata->m_combat_attribute.camp_inspired(0);
                //act统计
                act_to_tencent(it->second->_cdata.get(),act_new_camp_race);
                #ifdef QQ_PLAT
                //阵营战分享
                Singleton<inviteMgr>::Instance().update_event(it->first, SHARE_EVENT_FIRST_CAMP_RACE, 0);
                #endif
            }
            ++it;
        }
    }

    //胜利的阵营发奖励
    /*
    if (_score[0] > _score[1])
    {
        giveExtraAndMail(1, 2);
        giveExtraAndMail(2, 0);
        GeneralDataMgr::getInstance()->campRaceWinner(1);
    }
    else if (_score[0] < _score[1])
    {
        giveExtraAndMail(1, 0);
        giveExtraAndMail(2, 2);
        GeneralDataMgr::getInstance()->campRaceWinner(2);
    }
    else
    {
        giveExtraAndMail(1, 1);
        giveExtraAndMail(2, 1);
        GeneralDataMgr::getInstance()->campRaceWinner(0);
    }*/

    //更新连胜榜
    std::map<int, int> temp_maps;
    for (int i = 0; i < 2; ++i)
    {
        std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_data_map[i].begin();
        while (it != m_data_map[i].end())
        {
            if (it->second.get())
            {
                temp_maps[it->first] = it->second->_max_continue_wins;
            }
            ++it;
        }
    }
    m_rankings.updateRankings(temp_maps);

    //清除老数据
    _max_winner = "";    //最多连胜玩家
    _max_win_times = 0;    //最多连胜次数
    _max_win_camp = 0;

    _score[0] = 0;        //双方积分
    _score[1] = 0;
    _reports._report_list.clear();    //低等级全体战报
    _reports2._report_list.clear();    //高等级全体战报

    std::list<boost::shared_ptr<charCampRace> > tops;
    for (std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_total_data_map.begin(); it != m_total_data_map.end(); ++it)
    {
        tops.push_back(it->second);
    }
    tops.sort(compare_campRaceData);

    int top_damage_count = 0, top_10_presige = 0, top_10_attack = 0;
    for (std::list<boost::shared_ptr<charCampRace> >::iterator it = tops.begin(); it != tops.end(); ++it)
    {
        if ((*it).get())
        {
            top_10_presige += (*it)->_prestige;
            boost::shared_ptr<CharData> cdata = (*it)->_cdata;
            if (cdata.get())
            {
                top_10_attack += cdata->getAttack();
            }
            ++top_damage_count;
            if (top_damage_count >= 10)
            {
                break;
            }
        }
    }
    int prestige_fac = 0, attack_fac = 0;
    if (top_damage_count > 0)
    {
        prestige_fac = top_10_presige/top_damage_count;
        attack_fac = top_10_attack/top_damage_count;
    }

    //替身娃娃奖励
    m_stand_in_mob.processReward(prestige_fac, attack_fac);

    m_data_map[0].clear();
    m_data_map[1].clear();
    m_total_data_map.clear();

    //清空等待隊列
    m_guanfu_que.clear();
    m_lvlin_que.clear();
    
    return HC_SUCCESS;
}

//额外奖励并发送邮件
void campRaceMgr::giveExtraAndMail(int camp, int type)    //type 0 失败没有奖励  1 平局一半奖励 2 胜利全部奖励
{
    std::string strContent = "";
    switch (type)
    {
        case 0:
            strContent = strCampRaceMailExtraLose;
            if (camp == 1)
            {
                str_replace(strContent, "$C", strCamp2Name);
            }
            else
            {
                str_replace(strContent, "$C", strCamp1Name);
            }
            break;
        case 1:
            strContent = strCampRaceMailExtraDraw;
            break;
        case 2:
            strContent = strCampRaceMailExtraWin;
            if (camp == 2)
            {
                str_replace(strContent, "$C", strCamp2Name);
            }
            else
            {
                str_replace(strContent, "$C", strCamp1Name);
            }
            break;
    }
    std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_data_map[camp-1].begin();
    while (it != m_data_map[camp-1].end())
    {
        if (it->second.get() && (it->second->_wins + it->second->_loses + it->second->_skips) && it->second->_cdata.get())
        {
            // 2w * 所在地图 id
            int silver = type*10000*it->second->_cdata->m_area;
            it->second->_cdata->addSilver(silver);
            //银币获得统计
            add_statistics_of_silver_get(it->second->_cdata->m_id,it->second->_cdata->m_ip_address,silver,silver_get_campRace, it->second->_cdata->m_union_id, it->second->_cdata->m_server_id);
            std::string content = strContent;
            str_replace(content, "$P", LEX_CAST_STR(it->second->_prestige));
            str_replace(content, "$S", LEX_CAST_STR(it->second->_silver));
            if (silver)
            {
                str_replace(content, "$E", LEX_CAST_STR(silver));
            }
            sendSystemMail(it->second->_cdata->m_name, it->first, strCampRaceMailTitle, content);
        }
        ++it;
    }
}

//结束时间戳
int campRaceMgr::endTime()
{
    int time_left = _end_time - time(NULL);
    if (time_left < 0)
    {
        return 0;
    }
    return time_left;
}

//最大连胜纪录
void campRaceMgr::getMaxWin(std::string& name, int& times, int& camp)
{
    name = _max_winner;
    times = _max_win_times;
    camp = _max_win_camp;
}

//参加者列表
int campRaceMgr::getRacers(int camp, json_spirit::Array& array, json_spirit::Array& array2)
{
    if (camp != 1 && camp != 2)
    {
        return HC_ERROR;
    }
    for (std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_data_map[camp-1].begin(); it != m_data_map[camp-1].end(); ++it)
    {
        if (it->second.get() && it->second.get()->_cdata.get()
            && it->second.get()->_state != camp_race_state_leave)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->second.get()->_cdata->m_id) );
            obj.push_back( Pair("name", it->second.get()->_cdata->m_name) );
            if (it->second.get()->_idle_time)
            {
                obj.push_back( Pair("state", camp_race_state_fighting) );
            }
            else
            {
                obj.push_back( Pair("state", it->second.get()->_state) );
            }
            if (it->second.get()->_cdata->m_level <= iCampRaceLevel_low)
            {
                array.push_back(obj);
            }
            else
            {
                array2.push_back(obj);
            }
        }
    }
    return HC_SUCCESS;
}

campRaceReportList& campRaceMgr::getReportList1()
{
    return _reports;
}

campRaceReportList& campRaceMgr::getReportList2()
{
    return _reports2;
}

//双方积分
void campRaceMgr::getScore(int& score1, int& score2)
{
    score1 = _score[0];
    score2 = _score[1];
}

//活动是否开启
void campRaceMgr::getAction(CharData* pc, json_spirit::Array& blist)
{
    time_t timep;
    struct tm m_timenow;
    time(&timep);
    localtime_r(&timep, &m_timenow);

    int season = 0;
    boost::shared_ptr<DateInfo> date = GeneralDataMgr::getInstance()->GetDate();
    if (date.get())
    {
        season = date->season;
    }
    else
    {
        ERR();
    }
    openRule* pOpenRule = _open_rules.getRule(m_timenow, season);
    if (_open_state || pOpenRule)
    {
        //cout<<"campRace()::"<<_open_state<<","<<(pOpenRule?"rule":"non rule)");

        json_spirit::Object obj;
        obj.push_back( Pair("type", action_camp_race) );
        if (_open_state)
        {
            obj.push_back( Pair("state", 1) );
        }
        else
        {
            if ((pOpenRule->_open_hour > m_timenow.tm_hour
                    || (pOpenRule->_open_hour == m_timenow.tm_hour && pOpenRule->_open_min > m_timenow.tm_min)))
            {
                obj.push_back( Pair("state", 0) );
            }
            else
            {
                obj.push_back( Pair("state", 2) );
            }
        }
        if (pOpenRule)
        {
            std::string startTime = time2string(pOpenRule->_open_hour, pOpenRule->_open_min);
            //cout<<"open at "<<startTime<<",";
            obj.push_back( Pair("startTime", startTime) );

            int endmin = pOpenRule->_open_min + 30;
            int endhour = pOpenRule->_open_hour;

            std::string endTime = time2string(endhour, endmin);

            obj.push_back( Pair("endTime", endTime) );
        }
        int enable = 0, silver = 0, prestige = 0;
        m_stand_in_mob.getStandIn(pc->m_id, enable, silver, prestige);
        obj.push_back( Pair("baby_state", enable) );
        obj.push_back( Pair("baby_silver", silver) );
        obj.push_back( Pair("baby_prestige", prestige) );
        blist.push_back(obj);
    }
}

int campRaceMgr::getActionMemo(std::string& memo)
{
    time_t timep;
    struct tm m_timenow;
    time(&timep);
    localtime_r(&timep, &m_timenow);

    int season = 0;
    boost::shared_ptr<DateInfo> date = GeneralDataMgr::getInstance()->GetDate();
    if (date.get())
    {
        season = date->season;
    }
    else
    {
        ERR();
    }    
    if (_open_state)
    {
        //正在开启
        memo = strCampRaceIsOpening;
    }
    else
    {
        openRule* pOpenRule = _open_rules.getRule(m_timenow, season);
        if (pOpenRule)
        {
            //即将开启
            if ((pOpenRule->_open_hour > m_timenow.tm_hour
                    || (pOpenRule->_open_hour == m_timenow.tm_hour && pOpenRule->_open_min > m_timenow.tm_min)))
            {
                if (_last_rule != pOpenRule)
                {
                    _str_will_open_msg = strCampRaceWillOpen;
                    _open_time = "";
                    if (pOpenRule->_open_hour < 10)
                    {
                        str_replace(_str_will_open_msg, "$H", "0" + LEX_CAST_STR(pOpenRule->_open_hour));
                        _open_time = "0" + LEX_CAST_STR(pOpenRule->_open_hour);
                    }
                    else
                    {
                        str_replace(_str_will_open_msg, "$H", LEX_CAST_STR(pOpenRule->_open_hour));
                        _open_time = LEX_CAST_STR(pOpenRule->_open_hour);
                    }
                    if (pOpenRule->_open_min < 10)
                    {
                        str_replace(_str_will_open_msg, "$M", "0" + LEX_CAST_STR(pOpenRule->_open_min));
                        _open_time += ":0" + LEX_CAST_STR(pOpenRule->_open_min);
                    }
                    else
                    {
                        str_replace(_str_will_open_msg, "$M", LEX_CAST_STR(pOpenRule->_open_min));
                        _open_time += ":" + LEX_CAST_STR(pOpenRule->_open_min);
                    }
                }                        
                memo = _str_will_open_msg; 
            }                        
            else
            {
                //结束了
                memo = _str_have_closed;
            }
            _last_rule = pOpenRule;
        }
        else
        {
            return HC_ERROR;
        }
    }
    return HC_SUCCESS;
}

void campRaceMgr::broadMsg(const std::string& msg)
{
    for (int i = 0; i < 2; ++i)
    {
        std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_data_map[i].begin();
        while (it != m_data_map[i].end())
        {
            if (it->second.get() && it->second->_onlineChar.get())
            {
                it->second->_onlineChar->Send(msg);
            }
            ++it;
        }
    }
}

void campRaceMgr::broadMsg(int level_from, int level_to, const std::string& msg)
{
    for (int i = 0; i < 2; ++i)
    {
        std::map<int, boost::shared_ptr<charCampRace> >::iterator it = m_data_map[i].begin();
        while (it != m_data_map[i].end())
        {
            if (it->second.get()
                && it->second->_onlineChar.get()
                && it->second->_cdata->m_level >= level_from
                && it->second->_cdata->m_level <= level_to)
            {
                it->second->_onlineChar->Send(msg);
            }
            ++it;
        }
    }
}

std::string campRaceMgr::getRankings(int page)
{
    return m_rankings.getRankings(page);
}

bool campRaceMgr::isOpen()
{
    return _open_state;
}

std::string campRaceMgr::openTime()
{
    return _open_time;
}

//更新排行榜活动中的cid字段
void campRaceMgr::updateRankingsEvent(rankings_event* pE)
{
    m_rankings.updateRankingsEvent(pE);
}

void campRaceReportList::addReport(int type, int id1, int id2, const std::string& name1, const std::string& name2, int silver, int prestige, int win_times, int camp1, int camp2)
{
    campRaceReport rpt;
    rpt._type = type;
    rpt._id1 = id1;
    rpt._id2 = id2;
    rpt._name1 = name1;
    rpt._name2 = name2;
    rpt._silver = silver;
    rpt._prestige = prestige;
    rpt._win_times = win_times;
    rpt._camp1 = camp1;
    rpt._camp2 = camp2;
    _report_list.insert(_report_list.begin(), rpt);
}

int campRaceReportList::getReport(json_spirit::Array& reportArray, int cid, bool self)
{
    int reportCounts = 0;
    std::list<campRaceReport>::iterator it = _report_list.begin();
    if (it != _report_list.end())
    {
        for (;;)
        {
            if ( (self && (it->_id1 == cid || it->_id2 == cid))
                 || (!self && ( (it->_type == camp_race_report_skip && it->_id1 == cid)
                                || (it->_type == camp_race_report_win || it->_type == camp_race_report_end)
                               )
                    )
               )
            {
                ++reportCounts;
                json_spirit::Object obj;
                obj.push_back( Pair("type", it->_type) );
                obj.push_back( Pair("id1", it->_id1) );
                obj.push_back( Pair("id2", it->_id2) );
                obj.push_back( Pair("camp1", it->_camp1) );
                obj.push_back( Pair("camp2", it->_camp2) );
                obj.push_back( Pair("name1", it->_name1) );
                obj.push_back( Pair("name2", it->_name2) );
                obj.push_back( Pair("killNums", it->_win_times) );
                obj.push_back( Pair("silver", it->_silver) );
                obj.push_back( Pair("prestige", it->_prestige) );
                reportArray.insert(reportArray.begin(), obj);
            }
            ++it;
            if (reportCounts >= 20 || it == _report_list.end())
            {
                break;
            }
        }
    }
    return reportCounts;
}

//cmd:getGroupTime//获得阵营战结束时间
int ProcessGetGroupTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    robj.push_back( Pair("time", campRaceMgr::getInstance()->endTime()) );
    return HC_SUCCESS;
}

//cmd:getGroupWins//获得阵营战连胜榜
int ProcessGetGroupWins(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int times = 0, camp = 0;
    std::string name = "";
    campRaceMgr::getInstance()->getMaxWin(name, times, camp);
    robj.push_back( Pair("camp", camp) );
    robj.push_back( Pair("name", name) );
    robj.push_back( Pair("nums", times) );
    return HC_SUCCESS;
}

//cmd:getGroupMark//获得阵营战双方分数
int ProcessGetGroupMark(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int score1 = 0, score2 = 0;    
    //双方积分
    campRaceMgr::getInstance()->getScore(score1, score2);
    robj.push_back( Pair("mark1", score1) );
    robj.push_back( Pair("mark2", score2) );
    return HC_SUCCESS;
}

//cmd:getGroupInspire//获得鼓舞增加的战斗力百分数
int ProcessGetGroupInspire(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int level = campRaceMgr::getInstance()->getInspire(cdata.get());
    robj.push_back( Pair("damage", cdata->m_combat_attribute.camp_inspired()) );
    robj.push_back( Pair("hp", 3*cdata->m_combat_attribute.camp_inspired()/2) );
    robj.push_back( Pair("level", level) );

    robj.push_back( Pair("gold", 20) );
    robj.push_back( Pair("gx", 200) );
    robj.push_back( Pair("curGx", cdata->getGongxun()) );

    return HC_SUCCESS;
}

//cmd:getGroupList //获得阵营战列表
int ProcessGetGroupList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type) );
    json_spirit::Array array1, array2;
    campRaceMgr::getInstance()->getRacers(type, array1, array2);
    
    robj.push_back( Pair("list1", array1) );
    robj.push_back( Pair("list2", array2) );
    return HC_SUCCESS;
}

//cmd:getGroupReportList//获得战报列表
int ProcessGetGroupReportList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type) );

    json_spirit::Array array;
    if (cdata->m_level > iCampRaceLevel_low)
    {
        json_spirit::Array array;
        campRaceMgr::getInstance()->getReportList2().getReport(array, cdata->m_id, type == 2);
        robj.push_back( Pair("list", array) );
    }
    else
    {
        json_spirit::Array array;
        campRaceMgr::getInstance()->getReportList1().getReport(array, cdata->m_id, type == 2);
        robj.push_back( Pair("list", array) );
    }
    return HC_SUCCESS;
}

//cmd:inspireGroup//鼓舞战斗力
int ProcessInspireGroup(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    return campRaceMgr::getInstance()->inspire(cdata.get(), type, robj);
}


void notifyCampRaceStandIn(session_ptr& psession, int cid)
{
    //发送替身娃娃相关信息
    int enable = 0, silver = 0, prestige = 0;
    campRaceMgr::getInstance()->m_stand_in_mob.getStandIn(cid, enable, silver, prestige);
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "getBabyInfo") );
    obj.push_back( Pair("s", 200) );
    
    json_spirit::Object info;
    info.push_back( Pair("state", enable) );
    info.push_back( Pair("silver", silver) );
    info.push_back( Pair("gold", 30) );
    info.push_back( Pair("prestige", prestige) );

    json_spirit::Object action;
    action.push_back( Pair("type", action_camp_race) );
    if (campRaceMgr::getInstance()->isOpen())
    {
        action.push_back( Pair("active", 1) );
    }
    action.push_back( Pair("openLevel", 0) );
    action.push_back( Pair("openTime", campRaceMgr::getInstance()->openTime()) );

    info.push_back( Pair("action", action) );
    obj.push_back( Pair("info", info) );
    psession->send(write(obj, json_spirit::raw_utf8));
}

//加入阵营战 cmd: enterGroupBattle
int ProcessJoinCampRace(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> account = psession->user();
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get() || !account.get())
    {
        return ret;
    }

    if (cdata->m_campraceOpen == 0)
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    //if (cdata->m_camp > 2 || cdata->m_camp < 1)
    //{
    //    return HC_ERROR_CAMP_RACE_NO_CAMP;
    //}

    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type) );
    if (type == 1)
    {
        robj.push_back( Pair("new_player", cdata->isNewPlayer() > 0) );
        robj.push_back( Pair("new_player_time", cdata->isNewPlayer()) );
        if (!campRaceMgr::getInstance()->isOpen())
        {
            // vip 5以上可以使用替身娃娃
            if (cdata->m_vip >= 5)
            {
                //发送替身娃娃相关信息
                notifyCampRaceStandIn(psession, cdata->m_id);
                return HC_SUCCESS_NO_RET;
            }
            return HC_ERROR_CAMP_RACE_NOT_OPEN;
        }
        else if (cdata->m_vip >= 5)
        {
            int enable = 0, silver = 0, prestige = 0;
            campRaceMgr::getInstance()->m_stand_in_mob.getStandIn(cdata->m_id, enable, silver, prestige);
            if (enable == 1)
            {
                //发送替身娃娃相关信息
                notifyCampRaceStandIn(psession, cdata->m_id);
                return HC_SUCCESS_NO_RET;
            }
        }
        return campRaceMgr::getInstance()->joinRace(cdata, account->m_onlineCharactor, robj);
    }
    else
    {
        return campRaceMgr::getInstance()->leaveRace(cdata.get(), account->m_onlineCharactor);
    }
}

//cmd:startGroupBattle//作战
int ProcessStartGroupBattle(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return campRaceMgr::getInstance()->setFight(cdata.get(), true, robj);
}

//cmd:stopGroupBattle//结束作战
int ProcessStopGroupBattle(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return campRaceMgr::getInstance()->setFight(cdata.get(), false, robj);
}

//cmd:getGroupBattleInfo//获得阵营战玩家信息
int ProcessGetGroupBattleInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    boost::shared_ptr<charCampRace> raceD = campRaceMgr::getInstance()->getChar(cdata->m_camp, cdata->m_id);
    if (!raceD.get())
    {
        return HC_ERROR_CAMP_RACE_NOT_IN;
    }
    charCampRace* pRace = raceD.get();
    robj.push_back( Pair("maxKill", pRace->_max_continue_wins) );
    robj.push_back( Pair("curKill", pRace->_cur_continue_wins) );
    robj.push_back( Pair("winNums", pRace->_wins) );
    robj.push_back( Pair("failNums", pRace->_loses) );
    robj.push_back( Pair("prestige", pRace->_prestige) );
    robj.push_back( Pair("silver", pRace->_silver) );
    robj.push_back( Pair("isJoinBattle", pRace->_state == camp_race_state_idle ? 2 : 1) );
    pRace->_in_combat = 0;
    return HC_SUCCESS;
}

//定时进行阵营战对手匹配
int ProcessMatchCampRace(json_spirit::mObject& o)
{
    campRaceMgr::getInstance()->Race();
    return HC_SUCCESS;
}

//cmd:getCampRaceRankList//获取阵营战连胜排行榜
int ProcessGetCampRaceRankList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    std::string msg = campRaceMgr::getInstance()->getRankings(page);
    psession->send(msg);
    return HC_SUCCESS_NO_RET;
}

