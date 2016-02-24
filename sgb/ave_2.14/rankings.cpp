
#ifndef _WINDOWS
#include <execinfo.h>
#include <sys/syscall.h>
#endif

#include "rankings.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_all.h"

#include "spls_errcode.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "data.h"
#include<boost/tokenizer.hpp>

#include "statistics.h"

#include "combat.h"

using namespace boost;

int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
int InsertInternalActionWork(json_spirit::mObject& obj);

extern void InsertSaveDb(const std::string& sql);
extern int m_quit;

#define INFO(x)

Database& GetDb();

splsRankings* splsRankings::m_handle = NULL;

splsRankings* splsRankings::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new splsRankings();
        
        m_handle->m_updating_charRankings = 1;
        m_handle->m_updating_heroRankings = 1;
        m_handle->m_updating_lotteryRankings = 1;
        m_handle->m_updating_eliteRankings = 1;
        m_handle->m_updating_prestigeRankings = 1;
        m_handle->m_updating_attackRankings = 1;
        m_handle->m_updating_ZSTRankings = 1;
        m_handle->_update_rankings_threadptr.reset(new boost::thread(boost::bind(&splsRankings::_updateRankings, m_handle)));
        m_handle->_update_rankings_threadptr->detach();

        std::list<int> clist;
        Query q(GetDb());
        q.get_result("select cid from char_data_extra where type=2 and field=20000 order by value limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        while (q.fetch_row())
        {
            clist.push_back(q.getval());
        }
        q.free_result();

        for (std::list<int>::iterator it = clist.begin(); it != clist.end(); ++it)
        {
            GeneralDataMgr::getInstance()->GetCharData(*it);
        }
    }
    return m_handle;
}

splsRankings::splsRankings()
{
    m_updating_heroRankings = 0;
    m_updating_charRankings = 0;
    m_updating_lotteryRankings = 0;
    m_updating_eliteRankings = 0;
    m_updating_prestigeRankings = 0;
    m_updating_attackRankings = 0;
    m_updating_ZSTRankings = 0;
}

//真正给奖励
void giveRankingsEventReward(rankings_event* pE)
{
    //cout<<"giveRankingsEventReward(),type="<<pE->type<<endl;
    for (std::list<rankings_event_award>::iterator it = pE->rankings_list.begin(); it != pE->rankings_list.end(); ++it)
    {
        if (it->cid)
        {
            //cout<<"giveRankingsEventReward(),cid="<<it->cid<<",rank="<<it->rank<<endl;
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(it->cid);
            if (cdata.get())
            {
                std::list<Item> items = it->awards;
                giveLoots(cdata.get(), items, cdata->m_area, cdata->m_level, 0, NULL, NULL, true, give_rankings_event);
                cdata->NotifyCharData();
                std::string content = pE->mail_content;
                str_replace(content, "$R", LEX_CAST_STR(it->rank));
                std::string getStr = "";
                for (std::list<Item>::iterator itx = items.begin(); itx != items.end(); ++itx)
                {
                    if (getStr == "")
                    {
                        getStr = itx->toString(true);
                    }
                    else
                    {
                        getStr += "," + itx->toString(true);
                    }
                }
                str_replace(content, "$C", getStr);
                sendSystemMail(cdata->m_name, cdata->m_id, pE->mail_title, content);
            }
        }
    }
    InsertSaveDb("update admin_top set state='1' where id=" + LEX_CAST_STR(pE->id));
}

//检查排行榜活动是否开了
void checkRankingsEvent()
{
    //看是否需要发放活动奖励
    Query q2(GetDb());
    q2.get_result("select id,type from admin_top where enable='1' and state='0' and times<=unix_timestamp() order by times");
    while (q2.fetch_row())
    {
        int id = q2.getval();
        int type = q2.getval();

        Query q(GetDb());
        rankings_event* pE = NULL;
        q.get_result("select title,content from admin_topmail where type='" + LEX_CAST_STR(type) + "'");
        if (q.fetch_row())
        {
            pE = new rankings_event;

            pE->type = type;
            pE->id = id;
            pE->mail_title = q.getstr();
            pE->mail_content = q.getstr();
            q.free_result();
        }
        else
        {
            q.free_result();
            return;
        }

        std::map<int, std::list<Item> > reward_map;

        q.get_result("select rank,type from admin_toplist where atype='" + LEX_CAST_STR(type) + "' order by rank");
        while (q.fetch_row())
        {
            int rank = q.getval();
            std::string str = q.getstr();
            boost::char_separator<char> sep(",");
            typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
            tokenizer tok(str, sep);            
            {
                Item item;
                tokenizer::iterator it = tok.begin();
                if (it != tok.end())
                {
                    item.type = atoi((*it).c_str());
                    ++it;
                    if (it != tok.end())
                    {
                        item.id = atoi((*it).c_str());
                        ++it;
                        if (it != tok.end())
                        {
                            item.nums = atoi((*it).c_str());
                            if (reward_map.find(rank) == reward_map.end())
                            {
                                std::list<Item> ilist;
                                ilist.push_back(item);
                                reward_map[rank] = ilist;
                            }
                            else
                            {
                                reward_map[rank].push_back(item);
                            }
                        }
                    }
                }
            }
        }
        q.free_result();

        for (std::map<int, std::list<Item> >::iterator it = reward_map.begin(); it != reward_map.end(); ++it)
        {
            rankings_event_award re;
            re.rank = it->first;
            re.awards = it->second;

            pE->rankings_list.push_back(re);
        }
        if (pE->rankings_list.size())
        {
            json_spirit::mObject obj;
            obj["cmd"] = "rankingsEvent";
            obj["pointer"] = (boost::uint64_t)pE;
            if (0 != InsertInternalActionWork(obj))
            {
                ERR();
            }
        }
    }
    q2.free_result();
}

void splsRankings::_updateCharRankings()
{
    if (m_updating_charRankings)
    {
        m_updating_charRankings = 0;
        //cout<<"_updateCharRankings"<<endl;
        splsCharRankings* pCharRankings = new splsCharRankings;
        for (int i = 0; i < iCharRankingsPage*iRankingsPerPage; ++i)
        {
            pCharRankings->Rankings[i].cid = 0;
            pCharRankings->Rankings[i].rank = 0;
            pCharRankings->Rankings[i].level = 0;
            pCharRankings->Rankings[i].camp = 0;
        }
        Query q(GetDb());

        shhx_rankings rankings;
        rankings.type = 1;
        
        int rank = 0;
        q.get_result("select c.id,c.level as level,c.name,c.nick,cd.camp,cc.name,bo.name,bo.id from charactors as c left join char_corps_members as ccm on c.id=ccm.cid left join char_corps as cc on ccm.corps=cc.id left join char_data as cd on cd.cid=c.id left join base_offical as bo on bo.id=cd.official where c.state!='2' order by level desc,cd.prestige desc,cd.levelupTime desc limit " + LEX_CAST_STR(iCharRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            //cout<<"_updateCharRankings"<<rank<<endl;
            pCharRankings->Rankings[rank-1].rank = rank;
            pCharRankings->Rankings[rank-1].cid = q.getval();
            pCharRankings->Rankings[rank-1].level = q.getval();
            pCharRankings->Rankings[rank-1].name = q.getstr();
            std::string nick_string = q.getstr();
            if (nick_string != "" && nick_string != "[]")
            {
                json_spirit::Value v;
                json_spirit::read(nick_string, v);
                if (v.type() == json_spirit::array_type)
                {
                    pCharRankings->Rankings[rank-1].nicks = v.get_array();
                }
            }
            pCharRankings->Rankings[rank-1].camp = q.getval();
            pCharRankings->Rankings[rank-1].corps = q.getstr();
            pCharRankings->Rankings[rank-1].offical = q.getstr();
            pCharRankings->Rankings[rank-1].olevel = q.getval();

            rankings.cids.push_back(pCharRankings->Rankings[rank-1].cid);
        }
        q.free_result();
        
        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;
        
        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array* pArray = new json_spirit::Array;
            //std::string *pStr = new std::string;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pCharRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("id", pCharRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("camp", pCharRankings->Rankings[rank-1].camp) );
                    obj.push_back( Pair("level", pCharRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("name", pCharRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("nick", pCharRankings->Rankings[rank-1].nicks) );
                    obj.push_back( Pair("troop", pCharRankings->Rankings[rank-1].corps) );
                    obj.push_back( Pair("officer", pCharRankings->Rankings[rank-1].offical) );
                    obj.push_back( Pair("olevel", pCharRankings->Rankings[rank-1].olevel) );
                     pArray->push_back(obj);
                }
            }
            m_charRankingsPages[page-1].reset(pArray);
            //*pStr = json_spirit::write(*pArray, json_spirit::raw_utf8);
            //m_strCharRankingsPages[page-1].reset(pStr);
        
            json_spirit::Object* pobj = new json_spirit::Object;
            m_charRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }

        
        //原来的数据清除
        for (int page = maxpage; page < iCharRankingsPage; ++page)
        {
            m_charRankingsPages[page].reset();
            m_charRankingsPageobj[page].reset();
        }
        m_splsCharRankings.reset(pCharRankings);
        m_updating_charRankings = 0;
    }
}

void splsRankings::_updateHeroRankings()
{
    if (m_updating_heroRankings)
    {
        m_updating_heroRankings = 0;
        Query q(GetDb());
        //英雄排名
        splsHeroRankings* pHeroRankings = new splsHeroRankings;
        for (int i = 0; i < iHeroRankingsPage*iRankingsPerPage; ++i)
        {
            pHeroRankings->Rankings[i].cid = 0;
            pHeroRankings->Rankings[i].rank = 0;
            pHeroRankings->Rankings[i].level = 0;
            pHeroRankings->Rankings[i].camp = 0;
        }
        //排名规则:等级+属性点+成长值
        int rank = 0;
        q.get_result("select cg.cid,cg.id,cd.camp,cg.level,cg.color,cg.add_level+(cg.add_str+cg.add_int+cg.add_tong)/3 as attribute,cg.fac_a as rate,bg.name,c.name,c.nick,cg.genius1,cg.genius2,cg.genius3,cg.genius4,cg.genius5 from char_generals as cg left join base_generals as bg on cg.gid=bg.gid left join charactors as c on cg.cid=c.id left join char_data as cd on c.id=cd.cid where cg.state=0 and cg.level>9 order by (attribute + rate + cg.level) desc limit " + LEX_CAST_STR(iHeroRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            //cout<<"_updateHeroRankings"<<rank<<endl;
            pHeroRankings->Rankings[rank-1].rank = rank;
            pHeroRankings->Rankings[rank-1].cid = q.getval();
            pHeroRankings->Rankings[rank-1].gid = q.getval();
            pHeroRankings->Rankings[rank-1].camp = q.getval();        
            pHeroRankings->Rankings[rank-1].level = q.getval();
            pHeroRankings->Rankings[rank-1].quality = q.getval();
            pHeroRankings->Rankings[rank-1].attributeExtra = (int)q.getnum();//属性点
            pHeroRankings->Rankings[rank-1].rateNow = (int)q.getnum();//成长值
            pHeroRankings->Rankings[rank-1].name = q.getstr();
            const char* name = q.getstr();
            pHeroRankings->Rankings[rank-1].charname = name == NULL ? "" : name;
            std::string nick_string = q.getstr();
            if (nick_string != "" && nick_string != "[]")
            {
                json_spirit::Value v;
                json_spirit::read(nick_string, v);
                if (v.type() == json_spirit::array_type)
                {
                    pHeroRankings->Rankings[rank-1].nicks = v.get_array();
                }
            }
            // 天赋列表
            pHeroRankings->Rankings[rank-1].genius.clear();
            for (int i = 0; i < iGeniusMaxNum; i++)
            {
                pHeroRankings->Rankings[rank-1].genius.push_back(0);
            }
            for (int i = 0; i < iGeniusMaxNum; i++)
            {
                int genius = q.getval();
                genius = genius % 1000;
                pHeroRankings->Rankings[rank-1].genius[i] = genius;
            }
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array *pArray = new json_spirit::Array;
            //std::string *pStr = new std::string;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pHeroRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("id", pHeroRankings->Rankings[rank-1].gid) );
                    obj.push_back( Pair("cid", pHeroRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("level", pHeroRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("quality", pHeroRankings->Rankings[rank-1].quality) );
                    obj.push_back( Pair("add", pHeroRankings->Rankings[rank-1].attributeExtra) );
                    obj.push_back( Pair("name", pHeroRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("userName", pHeroRankings->Rankings[rank-1].charname) );
                    obj.push_back( Pair("nick", pHeroRankings->Rankings[rank-1].nicks) );
                    obj.push_back( Pair("level", pHeroRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("camp", pHeroRankings->Rankings[rank-1].camp) );
                    json_spirit::Array list;
                    for (int i = 0; i < iGeniusMaxNum; i++)
                    {
                        json_spirit::Object genius_obj;
                        genius_obj.push_back( Pair("id", pHeroRankings->Rankings[rank-1].genius[i]) );
                        list.push_back(genius_obj);
                    }
                    obj.push_back( Pair("genius_list", list) );
                    pArray->push_back(obj);
                }
            }
            m_heroRankingsPages[page-1].reset(pArray);
            //*pStr = json_spirit::write(*pArray, json_spirit::raw_utf8);
            json_spirit::Object* pobj = new json_spirit::Object;
            m_heroRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iHeroRankingsPage; ++page)
        {
            m_heroRankingsPages[page].reset();
            m_heroRankingsPageobj[page].reset();
        }
        m_splsHeroRankings.reset(pHeroRankings);
        m_updating_heroRankings = 0;
    }
    return;    
}

//更新梅花易數積分排名
void splsRankings::_updateLotteryScoreRankings()
{
    if (m_updating_lotteryRankings)
    {
        m_updating_lotteryRankings = 0;
        Query q(GetDb());
        //梅花易數積分排行
        splsLotteryScoreRankings* pRankings = new splsLotteryScoreRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pRankings->Rankings[i].cid = 0;
            pRankings->Rankings[i].rank = 0;
            pRankings->Rankings[i].score = 0;
            pRankings->Rankings[i].camp = 0;
            pRankings->Rankings[i].name = "";
            pRankings->Rankings[i].offical = "";
            pRankings->Rankings[i].corps = "";
        }

        int rank = 0;
        q.get_result("select ce.cid,ce.value,c.name,cd.camp,cc.name,bo.name,bo.id from char_data_extra as ce left join charactors as c on ce.cid=c.id left join char_corps_members as ccm on c.id=ccm.cid left join char_corps as cc on ccm.corps=cc.id left join char_data as cd on cd.cid=c.id left join base_offical as bo on bo.id=cd.official where ce.field=" + LEX_CAST_STR(char_data_extra_lottery_score) + " order by ce.value desc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            //cout<<"_updateHeroRankings"<<rank<<endl;
            pRankings->Rankings[rank-1].rank = rank;
            pRankings->Rankings[rank-1].cid = q.getval();
            pRankings->Rankings[rank-1].score = q.getval();
            pRankings->Rankings[rank-1].name = q.getstr();
            pRankings->Rankings[rank-1].camp = q.getval();        
            pRankings->Rankings[rank-1].corps= q.getstr();
            pRankings->Rankings[rank-1].offical = q.getstr();
            pRankings->Rankings[rank-1].olevel = q.getval();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array *pArray = new json_spirit::Array;
            //std::string *pStr = new std::string;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("cid", pRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("score", pRankings->Rankings[rank-1].score) );
                    obj.push_back( Pair("name", pRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("troop", pRankings->Rankings[rank-1].corps) );
                    obj.push_back( Pair("officer", pRankings->Rankings[rank-1].offical) );
                    obj.push_back( Pair("olevel", pRankings->Rankings[rank-1].olevel) );
                    obj.push_back( Pair("camp", pRankings->Rankings[rank-1].camp) );                    
                    pArray->push_back(obj);
                }
            }
            m_lotteryRankingsPages[page-1].reset(pArray);
            //*pStr = json_spirit::write(*pArray, json_spirit::raw_utf8);
            json_spirit::Object* pobj = new json_spirit::Object;
            m_lotteryRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_lotteryRankingsPages[page].reset();
            m_lotteryRankingsPageobj[page].reset();
        }
        m_splsLotteryRankings.reset(pRankings);
        m_updating_lotteryRankings = 0;
    }
    return;    
}

//更新精英战排行
void splsRankings::_updateEliteRankings()
{
    if (m_updating_eliteRankings)
    {
        m_updating_eliteRankings = 0;
        Query q(GetDb());
        //精英战排行
        splsEliteRankings* pRankings = new splsEliteRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pRankings->Rankings[i].cid = 0;
            pRankings->Rankings[i].rank = 0;
            pRankings->Rankings[i].level = 0;
            pRankings->Rankings[i].attack = 0;
            pRankings->Rankings[i].elite_id = 0;
            pRankings->Rankings[i].elite_name = "";
            pRankings->Rankings[i].name = "";
        }

        int rank = 0;
        q.get_result("select c.id,c.level,c.name,c.nick,cd.elite_id,be.name,ce.value from charactors as c left join char_data as cd on cd.cid=c.id left join char_data_extra as ce on cd.cid=ce.cid left join base_elite_armys as be on cd.elite_id=be.eliteid where cd.elite_id>0 and ce.field=" + LEX_CAST_STR(char_data_zhen_attack) + " order by cd.elite_id desc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pRankings->Rankings[rank-1].rank = rank;
            pRankings->Rankings[rank-1].cid = q.getval();
            pRankings->Rankings[rank-1].level = q.getval();
            pRankings->Rankings[rank-1].name = q.getstr();
            std::string nick_string = q.getstr();
            if (nick_string != "" && nick_string != "[]")
            {
                json_spirit::Value v;
                json_spirit::read(nick_string, v);
                if (v.type() == json_spirit::array_type)
                {
                    pRankings->Rankings[rank-1].nicks = v.get_array();
                }
            }
            pRankings->Rankings[rank-1].elite_id = q.getval();
            pRankings->Rankings[rank-1].elite_name = q.getstr();
            pRankings->Rankings[rank-1].attack = q.getval();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array *pArray = new json_spirit::Array;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("cid", pRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("level", pRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("name", pRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("nick", pRankings->Rankings[rank-1].nicks) );
                    obj.push_back( Pair("attack", pRankings->Rankings[rank-1].attack) );
                    obj.push_back( Pair("elite_name", pRankings->Rankings[rank-1].elite_name) );
                    pArray->push_back(obj);
                }
            }
            m_eliteRankingsPages[page-1].reset(pArray);
            //*pStr = json_spirit::write(*pArray, json_spirit::raw_utf8);
            json_spirit::Object* pobj = new json_spirit::Object;
            m_eliteRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_eliteRankingsPages[page].reset();
            m_eliteRankingsPageobj[page].reset();
        }
        m_splsEliteRankings.reset(pRankings);
        m_updating_eliteRankings = 0;
    }
    return;    
}

//更新声望获得周排行
void splsRankings::_updatePrestigeGetRankings()
{
    if (m_updating_prestigeRankings)
    {
        m_updating_prestigeRankings = 0;
        Query q(GetDb());
        splsPrestigeGetRankings* pRankings = new splsPrestigeGetRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pRankings->Rankings[i].cid = 0;
            pRankings->Rankings[i].rank = 0;
            pRankings->Rankings[i].score = 0;
            pRankings->Rankings[i].level = 0;
            pRankings->Rankings[i].name = "";
        }

        int rank = 0;
        q.get_result("select ce.cid,ce.value,c.name,c.nick,c.level from char_data_extra as ce left join charactors as c on ce.cid=c.id where c.level>=45 and ce.field=" + LEX_CAST_STR(char_data_extra_prestige_get) + " order by ce.value desc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pRankings->Rankings[rank-1].rank = rank;
            pRankings->Rankings[rank-1].cid = q.getval();
            pRankings->Rankings[rank-1].score = q.getval();
            pRankings->Rankings[rank-1].name = q.getstr();
            std::string nick_string = q.getstr();
            if (nick_string != "" && nick_string != "[]")
            {
                json_spirit::Value v;
                json_spirit::read(nick_string, v);
                if (v.type() == json_spirit::array_type)
                {
                    pRankings->Rankings[rank-1].nicks = v.get_array();
                }
            }
            pRankings->Rankings[rank-1].level = q.getval();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array *pArray = new json_spirit::Array;
            //std::string *pStr = new std::string;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("cid", pRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("score", pRankings->Rankings[rank-1].score) );
                    obj.push_back( Pair("name", pRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("nick", pRankings->Rankings[rank-1].nicks) );
                    obj.push_back( Pair("level", pRankings->Rankings[rank-1].level) );
                    pArray->push_back(obj);
                }
            }
            m_prestigeRankingsPages[page-1].reset(pArray);
            //*pStr = json_spirit::write(*pArray, json_spirit::raw_utf8);
            json_spirit::Object* pobj = new json_spirit::Object;
            m_prestigeRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_prestigeRankingsPages[page].reset();
            m_prestigeRankingsPageobj[page].reset();
        }
        m_splsPrestigeRankings.reset(pRankings);
        m_updating_prestigeRankings = 0;
    }
    return;    
}

//更新战力排名
void splsRankings::_updateAttackRankings()
{
    if (m_updating_attackRankings)
    {
        m_updating_attackRankings = 0;
        Query q(GetDb());
        splsAttackRankings* pRankings = new splsAttackRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pRankings->Rankings[i].cid = 0;
            pRankings->Rankings[i].rank = 0;
            pRankings->Rankings[i].score = 0;
            pRankings->Rankings[i].level = 0;
            pRankings->Rankings[i].name = "";
            pRankings->Rankings[i].offical = "";
            pRankings->Rankings[i].corps = "";
        }

        int rank = 0;
        q.get_result("select ce.cid,ce.value,c.name,c.nick,c.level,cc.name,bo.name from char_data_extra as ce left join charactors as c on ce.cid=c.id left join char_corps_members as ccm on c.id=ccm.cid left join char_corps as cc on ccm.corps=cc.id left join char_data as cd on cd.cid=c.id left join base_offical as bo on bo.id=cd.official where ce.field=" + LEX_CAST_STR(char_data_zhen_attack) + " order by ce.value desc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pRankings->Rankings[rank-1].rank = rank;
            pRankings->Rankings[rank-1].cid = q.getval();
            pRankings->Rankings[rank-1].score = q.getval();
            pRankings->Rankings[rank-1].name = q.getstr();
            std::string nick_string = q.getstr();
            if (nick_string != "" && nick_string != "[]")
            {
                json_spirit::Value v;
                json_spirit::read(nick_string, v);
                if (v.type() == json_spirit::array_type)
                {
                    pRankings->Rankings[rank-1].nicks = v.get_array();
                }
            }
            pRankings->Rankings[rank-1].level = q.getval();        
            pRankings->Rankings[rank-1].corps= q.getstr();
            pRankings->Rankings[rank-1].offical = q.getstr();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array *pArray = new json_spirit::Array;
            //std::string *pStr = new std::string;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("cid", pRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("score", pRankings->Rankings[rank-1].score) );
                    obj.push_back( Pair("name", pRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("nick", pRankings->Rankings[rank-1].nicks) );
                    obj.push_back( Pair("troop", pRankings->Rankings[rank-1].corps) );
                    obj.push_back( Pair("officer", pRankings->Rankings[rank-1].offical) );
                    obj.push_back( Pair("level", pRankings->Rankings[rank-1].level) );
                    pArray->push_back(obj);
                }
            }
            m_attackRankingsPages[page-1].reset(pArray);
            //*pStr = json_spirit::write(*pArray, json_spirit::raw_utf8);
            json_spirit::Object* pobj = new json_spirit::Object;
            m_attackRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_attackRankingsPages[page].reset();
            m_attackRankingsPageobj[page].reset();
        }
        m_splsAttackRankings.reset(pRankings);
        m_updating_attackRankings = 0;
    }
    return;    
}

//更新战神台排名
void splsRankings::_updateZSTRankings()
{
    if (m_updating_ZSTRankings)
    {
        m_updating_ZSTRankings = 0;
        Query q(GetDb());
        splsZSTRankings* pRankings = new splsZSTRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pRankings->Rankings[i].cid = 0;
            pRankings->Rankings[i].rank = 0;
            pRankings->Rankings[i].score = 0;
            pRankings->Rankings[i].level = 0;
            pRankings->Rankings[i].attack = 0;
            pRankings->Rankings[i].name = "";
            pRankings->Rankings[i].corps = "";
        }

        int rank = 0;
        q.get_result("select cz.cid,cz.star,c.name,c.nick,c.level,cc.name,ce.value from char_zst as cz left join charactors as c on cz.cid=c.id left join char_corps_members as ccm on c.id=ccm.cid left join char_corps as cc on ccm.corps=cc.id left join char_data as cd on cd.cid=c.id left join char_data_extra as ce on cd.cid=ce.cid where ce.field=" + LEX_CAST_STR(char_data_zhen_attack) + " order by cz.star desc,cz.star_update_time asc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pRankings->Rankings[rank-1].rank = rank;
            pRankings->Rankings[rank-1].cid = q.getval();
            pRankings->Rankings[rank-1].score = q.getval();
            pRankings->Rankings[rank-1].name = q.getstr();
            std::string nick_string = q.getstr();
            if (nick_string != "" && nick_string != "[]")
            {
                json_spirit::Value v;
                json_spirit::read(nick_string, v);
                if (v.type() == json_spirit::array_type)
                {
                    pRankings->Rankings[rank-1].nicks = v.get_array();
                }
            }
            pRankings->Rankings[rank-1].level = q.getval();
            pRankings->Rankings[rank-1].corps= q.getstr();
            pRankings->Rankings[rank-1].attack = q.getval();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array *pArray = new json_spirit::Array;
            //std::string *pStr = new std::string;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("cid", pRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("score", pRankings->Rankings[rank-1].score) );
                    obj.push_back( Pair("name", pRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("nick", pRankings->Rankings[rank-1].nicks) );
                    obj.push_back( Pair("troop", pRankings->Rankings[rank-1].corps) );
                    obj.push_back( Pair("level", pRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("attack", pRankings->Rankings[rank-1].attack) );
                    pArray->push_back(obj);
                }
            }
            m_ZSTRankingsPages[page-1].reset(pArray);
            //*pStr = json_spirit::write(*pArray, json_spirit::raw_utf8);
            json_spirit::Object* pobj = new json_spirit::Object;
            m_ZSTRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_ZSTRankingsPages[page].reset();
            m_ZSTRankingsPageobj[page].reset();
        }
        m_splsZSTRankings.reset(pRankings);
        m_updating_ZSTRankings = 0;
    }
    return;    
}

void splsRankings::_updateRankings()
{
    while (!m_quit)
    {
        if (!m_updating_charRankings && 
            !m_updating_heroRankings && 
            !m_updating_lotteryRankings && 
            !m_updating_eliteRankings && 
            !m_updating_prestigeRankings && 
            !m_updating_attackRankings && 
            !m_updating_ZSTRankings)
        {
            GetDb();
            do_sleep(5000);
            continue;
        }
        _updateHeroRankings();
        _updateCharRankings();
        _updateLotteryScoreRankings();
        _updateEliteRankings();
        _updatePrestigeGetRankings();
        _updateAttackRankings();
        _updateZSTRankings();
    }
}

void splsRankings::updateRankings(int type)
{
    switch (type)
    {
        case rankings_type_char:
            m_updating_charRankings = 1;
            break;
        case rankings_type_hero:
            m_updating_heroRankings = 1;
            break;
        case rankings_type_lottery:
            m_updating_lotteryRankings = 1;
            break;
        case rankings_type_elite:
            m_updating_eliteRankings = 1;
            break;
        case rankings_type_prestige:
            m_updating_prestigeRankings = 1;
            break;
        case rankings_type_attack:
            m_updating_attackRankings = 1;
            break;
        case rankings_type_zst:
            m_updating_ZSTRankings = 1;
            break;
    }
}

int splsRankings::getCharRankings(int page, int cid, json_spirit::Object &robj)
{
    int rpage = 1;
    if (page > 0)
    {
        rpage = page;
    }
    else
    {
        for (int i = 0; i < iCharRankingsPage * iRankingsPerPage; ++i)
        {
            if (m_splsCharRankings.get() && m_splsCharRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iCharRankingsPage || !m_charRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_charRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_charRankingsPageobj[rpage-1];

    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

int splsRankings::getHeroRankings(int page, int cid, json_spirit::Object &robj)
{
    int rpage = 1;
    if (page > 0)
    {
        rpage = page;
    }
    else
    {
        for (int i = 0; i < iHeroRankingsPage * iRankingsPerPage; ++i)
        {
            if (m_splsHeroRankings.get() && m_splsHeroRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iHeroRankingsPage || !m_heroRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_heroRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_heroRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

int splsRankings::getLotteryRankings(int page, int cid, json_spirit::Object &robj)
{
    int rpage = 1;
    if (page > 0)
    {
        rpage = page;
    }
    else
    {
        for (int i = 0; i < iRankingsPage * iRankingsPerPage; ++i)
        {
            if (m_splsLotteryRankings.get() && m_splsLotteryRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_lotteryRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_lotteryRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_lotteryRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

int splsRankings::getEliteRankings(int page, int cid, json_spirit::Object &robj)
{
    int rpage = 1;
    if (page > 0)
    {
        rpage = page;
    }
    else
    {
        for (int i = 0; i < iRankingsPage * iRankingsPerPage; ++i)
        {
            if (m_splsEliteRankings.get() && m_splsEliteRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_eliteRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_eliteRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_eliteRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

int splsRankings::getPrestigeRankings(int page, int cid, json_spirit::Object &robj)
{
    int rpage = 1;
    if (page > 0)
    {
        rpage = page;
    }
    else
    {
        for (int i = 0; i < iRankingsPage * iRankingsPerPage; ++i)
        {
            if (m_splsPrestigeRankings.get() && m_splsPrestigeRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_prestigeRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_prestigeRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_prestigeRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

int splsRankings::getAttackRankings(int page, int cid, json_spirit::Object &robj)
{
    int rpage = 1;
    if (page > 0)
    {
        rpage = page;
    }
    else
    {
        for (int i = 0; i < iRankingsPage * iRankingsPerPage; ++i)
        {
            if (m_splsAttackRankings.get() && m_splsAttackRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_attackRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_attackRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_attackRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

int splsRankings::getZSTRankings(int page, int cid, json_spirit::Object &robj)
{
    int rpage = 1;
    if (page > 0)
    {
        rpage = page;
    }
    else
    {
        for (int i = 0; i < iRankingsPage * iRankingsPerPage; ++i)
        {
            if (m_splsZSTRankings.get() && m_splsZSTRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_ZSTRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_ZSTRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_ZSTRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

//更新排行榜活动中的cid字段
void splsRankings::updateRankingsEvent(rankings_event* pE)
{
    //cout<<"splsRankings::updateRankingsEvent(),type="<<pE->type<<endl;
    switch (pE->type)
    {
        case char_rankings:
            {
                boost::shared_ptr<const splsCharRankings> rankings = m_splsCharRankings;
                if (rankings.get())
                {
                    for (std::list<rankings_event_award>::iterator it = pE->rankings_list.begin(); it != pE->rankings_list.end(); ++it)
                    {
                        //cout<<"rank:"<<it->rank<<endl;
                        if (it->rank > 0 && it->rank <= iCharRankingsPage*iRankingsPerPage)
                        {
                            it->cid = rankings->Rankings[it->rank-1].cid;
                            //cout<<"\tcid->"<<it->cid<<endl;
                        }
                    }
                }
            }
            break;
        case hero_rankings:
            {
                boost::shared_ptr<const splsHeroRankings> rankings = m_splsHeroRankings;
                if (rankings.get())
                {
                    for (std::list<rankings_event_award>::iterator it = pE->rankings_list.begin(); it != pE->rankings_list.end(); ++it)
                    {
                        if (it->rank > 0 && it->rank <= iHeroRankingsPage*iRankingsPerPage)
                        {
                            it->cid = rankings->Rankings[it->rank-1].cid;
                        }
                    }
                }
            }
            break;
        case lottery_rankings:
            {
                boost::shared_ptr<const splsLotteryScoreRankings> rankings = m_splsLotteryRankings;
                if (rankings.get())
                {
                    for (std::list<rankings_event_award>::iterator it = pE->rankings_list.begin(); it != pE->rankings_list.end(); ++it)
                    {
                        if (it->rank > 0 && it->rank <= iRankingsPage*iRankingsPerPage)
                        {
                            it->cid = rankings->Rankings[it->rank-1].cid;
                        }
                    }
                }
            }
            break;
        default:
            return;
    }
}

