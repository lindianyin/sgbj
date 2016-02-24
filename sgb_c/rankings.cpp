
#ifndef _WINDOWS
#include <execinfo.h>
#include <sys/syscall.h>
#endif

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include<boost/tokenizer.hpp>
#include "rankings.h"
#include "utils_all.h"
#include "errcode_def.h"
#include "data.h"

using namespace boost;

extern int m_quit;

#define INFO(x)

Database& GetDb();

//角色排行
int ProcessGetCharRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return Singleton<rankingMgr>::Instance().getCharRankings(page, pc->m_id, robj);
}

//财富排行
int ProcessGetSilverRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return Singleton<rankingMgr>::Instance().getSilverRankings(page, pc->m_id, robj);
}

//英雄排行
int ProcessGetHeroRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return Singleton<rankingMgr>::Instance().getHeroRankings(page, pc->m_id, robj);
}

//关卡进度排行
int ProcessGetStrongholdRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return Singleton<rankingMgr>::Instance().getStrongholdRankings(page, pc->m_id, robj);
}

//副本进度排行
int ProcessGetCopyRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return Singleton<rankingMgr>::Instance().getCopyRankings(page, pc->m_id, robj);
}

//神灵塔进度排行
int ProcessGetShenlingRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return Singleton<rankingMgr>::Instance().getShenlingRankings(page, pc->m_id, robj);
}

//战力排行
int ProcessGetCharAttackRanklist(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return Singleton<rankingMgr>::Instance().getCharAttackRankings(page, pc->m_id, robj);
}

rankingMgr::rankingMgr()
{
    m_updating_charRankings = 1;
    m_updating_silverRankings = 1;
    m_updating_heroRankings = 1;
    m_updating_strongholdRankings= 1;
    m_updating_copyRankings= 1;
    m_updating_shenlingRankings= 1;
    m_updating_charAttackRankings = 1;
    _update_rankings_threadptr.reset(new boost::thread(boost::bind(&rankingMgr::_updateRankings, this)));
    _update_rankings_threadptr->detach();
}

void rankingMgr::_updateRankings()
{
    while (!m_quit)
    {
        if (!m_updating_charRankings &&
            !m_updating_silverRankings &&
            !m_updating_heroRankings &&
            !m_updating_strongholdRankings &&
            !m_updating_copyRankings &&
            !m_updating_shenlingRankings &&
            !m_updating_charAttackRankings)
        {
            GetDb();
            do_sleep(5000);
            continue;
        }
        if (m_updating_charRankings)
            _updateCharRankings();
        if (m_updating_silverRankings)
            _updateSilverRankings();
        if (m_updating_heroRankings)
            _updateHeroRankings();
        if (m_updating_strongholdRankings)
            _updateStrongholdRankings();
        if (m_updating_copyRankings)
            _updateCopyRankings();
        if (m_updating_shenlingRankings)
            _updateShenlingRankings();
        if (m_updating_charAttackRankings)
            _updateCharAttackRankings();
    }
}

void rankingMgr::updateRankings()
{
    m_updating_charRankings = 1;
    m_updating_silverRankings = 1;
    m_updating_heroRankings = 1;
    m_updating_strongholdRankings = 1;
    m_updating_copyRankings = 1;
    m_updating_shenlingRankings = 1;
    m_updating_charAttackRankings = 1;
}

void rankingMgr::_updateCharRankings()
{
    if (m_updating_charRankings)
    {
        m_updating_charRankings = 0;
        gameCharRankings* pCharRankings = new gameCharRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pCharRankings->Rankings[i].cid = 0;
            pCharRankings->Rankings[i].rank = 0;
            pCharRankings->Rankings[i].level = 0;
            pCharRankings->Rankings[i].race = 0;
            pCharRankings->Rankings[i].union_name = "";
            pCharRankings->Rankings[i].name = "";
            pCharRankings->Rankings[i].vip = 0;
        }
        Query q(GetDb());

        int rank = 0;
        q.get_result("select c.id,c.level as level,c.name,cd.race,cd.vip,cg.name from charactors as c left join char_data as cd on cd.cid=c.id left join char_guild_data as cgd on c.id=cgd.cid left join char_guilds as cg on cgd.gid=cg.id where 1 order by level desc,cd.levelupTime desc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pCharRankings->Rankings[rank-1].rank = rank;
            pCharRankings->Rankings[rank-1].cid = q.getval();
            pCharRankings->Rankings[rank-1].level = q.getval();
            pCharRankings->Rankings[rank-1].name = q.getstr();
            pCharRankings->Rankings[rank-1].race = q.getval();
            pCharRankings->Rankings[rank-1].vip = q.getval();
            pCharRankings->Rankings[rank-1].union_name = q.getstr();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array* pArray = new json_spirit::Array;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pCharRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("id", pCharRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("race", pCharRankings->Rankings[rank-1].race) );
                    obj.push_back( Pair("level", pCharRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("vip", pCharRankings->Rankings[rank-1].vip) );
                    obj.push_back( Pair("name", pCharRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("unionname", pCharRankings->Rankings[rank-1].union_name) );
                    pArray->push_back(obj);
                }
            }
            m_charRankingsPages[page-1].reset(pArray);

            json_spirit::Object* pobj = new json_spirit::Object;
            m_charRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_charRankingsPages[page].reset();
        }
        m_CharRankings.reset(pCharRankings);
        m_updating_charRankings = 0;
    }
}

void rankingMgr::_updateSilverRankings()
{
    if (m_updating_silverRankings)
    {
        m_updating_silverRankings = 0;
        gameSilverRankings* pSilverRankings = new gameSilverRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pSilverRankings->Rankings[i].cid = 0;
            pSilverRankings->Rankings[i].rank = 0;
            pSilverRankings->Rankings[i].silver = 0;
            pSilverRankings->Rankings[i].race = 0;
            pSilverRankings->Rankings[i].union_name = "";
            pSilverRankings->Rankings[i].name = "";
            pSilverRankings->Rankings[i].vip = 0;
        }
        Query q(GetDb());

        int rank = 0;
        q.get_result("select c.id,c.name,(cr.silver+cr.bank_silver) as silver,cd.race,cd.vip,cg.name from charactors as c left join char_data as cd on cd.cid=c.id left join char_resource as cr on c.id = cr.cid left join char_guild_data as cgd on c.id=cgd.cid left join char_guilds as cg on cgd.gid=cg.id where 1 order by silver desc,c.id asc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pSilverRankings->Rankings[rank-1].rank = rank;
            pSilverRankings->Rankings[rank-1].cid = q.getval();
            pSilverRankings->Rankings[rank-1].name = q.getstr();
            pSilverRankings->Rankings[rank-1].silver = q.getval();
            pSilverRankings->Rankings[rank-1].race = q.getval();
            pSilverRankings->Rankings[rank-1].vip = q.getval();
            pSilverRankings->Rankings[rank-1].union_name = q.getstr();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array* pArray = new json_spirit::Array;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pSilverRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("id", pSilverRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("race", pSilverRankings->Rankings[rank-1].race) );
                    obj.push_back( Pair("silver", pSilverRankings->Rankings[rank-1].silver) );
                    obj.push_back( Pair("vip", pSilverRankings->Rankings[rank-1].vip) );
                    obj.push_back( Pair("name", pSilverRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("unionname", pSilverRankings->Rankings[rank-1].union_name) );
                    pArray->push_back(obj);
                }
            }
            m_silverRankingsPages[page-1].reset(pArray);

            json_spirit::Object* pobj = new json_spirit::Object;
            m_silverRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_silverRankingsPages[page].reset();
        }
        m_SilverRankings.reset(pSilverRankings);
        m_updating_silverRankings = 0;
    }
}

void rankingMgr::_updateHeroRankings()
{
    if (m_updating_heroRankings)
    {
        m_updating_heroRankings = 0;
        Query q(GetDb());
        //英雄排名
        gameHeroRankings* pHeroRankings = new gameHeroRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pHeroRankings->Rankings[i].cid = 0;
            pHeroRankings->Rankings[i].rank = 0;
            pHeroRankings->Rankings[i].hid = 0;
            pHeroRankings->Rankings[i].level = 0;
            pHeroRankings->Rankings[i].star = 0;
            pHeroRankings->Rankings[i].attributeExtra= 0;
            pHeroRankings->Rankings[i].name= "";
            pHeroRankings->Rankings[i].char_name= "";
            pHeroRankings->Rankings[i].vip = 0;
        }
        int rank = 0;
        q.get_result("select ch.cid,ch.id,ch.level,ch.star,(ch.hero_attack+ch.hero_defense+ch.hero_magic) as attribute,bh.name,c.name,cd.vip from char_heros as ch left join base_heros as bh on ch.hid=bh.hid left join charactors as c on ch.cid=c.id left join char_data as cd on c.id = cd.cid where attribute>0 order by attribute desc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pHeroRankings->Rankings[rank-1].rank = rank;
            pHeroRankings->Rankings[rank-1].cid = q.getval();
            pHeroRankings->Rankings[rank-1].hid = q.getval();
            pHeroRankings->Rankings[rank-1].level = q.getval();
            pHeroRankings->Rankings[rank-1].star = q.getval();
            pHeroRankings->Rankings[rank-1].attributeExtra = q.getval();//属性点
            pHeroRankings->Rankings[rank-1].name = q.getstr();
            const char* name = q.getstr();
            pHeroRankings->Rankings[rank-1].char_name = name == NULL ? "" : name;
            pHeroRankings->Rankings[rank-1].vip = q.getval();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array *pArray = new json_spirit::Array;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pHeroRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("id", pHeroRankings->Rankings[rank-1].hid) );
                    obj.push_back( Pair("cid", pHeroRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("level", pHeroRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("star", pHeroRankings->Rankings[rank-1].star) );
                    obj.push_back( Pair("add", pHeroRankings->Rankings[rank-1].attributeExtra) );
                    obj.push_back( Pair("name", pHeroRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("charname", pHeroRankings->Rankings[rank-1].char_name) );
                    obj.push_back( Pair("vip", pHeroRankings->Rankings[rank-1].vip) );
                    pArray->push_back(obj);
                }
            }
            m_heroRankingsPages[page-1].reset(pArray);

            json_spirit::Object* pobj = new json_spirit::Object;
            m_heroRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_heroRankingsPages[page].reset();
            m_heroRankingsPageobj[page].reset();
        }
        m_HeroRankings.reset(pHeroRankings);
        m_updating_heroRankings = 0;
    }
    return;
}

void rankingMgr::_updateStrongholdRankings()
{
    if (m_updating_strongholdRankings)
    {
        m_updating_strongholdRankings = 0;
        Query q(GetDb());
        //关卡进度排名
        gameStrongholdRankings* pStrongholdRankings = new gameStrongholdRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pStrongholdRankings->Rankings[i].cid = 0;
            pStrongholdRankings->Rankings[i].rank = 0;
            pStrongholdRankings->Rankings[i].stronghold = 0;
            pStrongholdRankings->Rankings[i].level = 0;
            pStrongholdRankings->Rankings[i].name= "";
            pStrongholdRankings->Rankings[i].union_name= "";
            pStrongholdRankings->Rankings[i].stronghold_name= "";
            pStrongholdRankings->Rankings[i].vip = 0;
        }
        int rank = 0;
        q.get_result("select ce.cid,ce.value,c.level,c.name,bs.name,cg.name,cd.vip from char_data_extra as ce left join charactors as c on ce.cid=c.id left join char_data as cd on c.id = cd.cid left join base_stronghold as bs on ce.value=bs.id left join char_guild_data as cgd on c.id=cgd.cid left join char_guilds as cg on cgd.gid=cg.id where ce.field=" + LEX_CAST_STR(char_data_normal_stronghold) + " order by ce.value desc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pStrongholdRankings->Rankings[rank-1].rank = rank;
            pStrongholdRankings->Rankings[rank-1].cid = q.getval();
            pStrongholdRankings->Rankings[rank-1].stronghold = q.getval();
            pStrongholdRankings->Rankings[rank-1].level = q.getval();
            pStrongholdRankings->Rankings[rank-1].name = q.getstr();
            const char* name = q.getstr();
            pStrongholdRankings->Rankings[rank-1].stronghold_name = name == NULL ? "" : name;
            pStrongholdRankings->Rankings[rank-1].union_name = q.getstr();
            pStrongholdRankings->Rankings[rank-1].vip = q.getval();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array *pArray = new json_spirit::Array;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pStrongholdRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("id", pStrongholdRankings->Rankings[rank-1].stronghold) );
                    obj.push_back( Pair("cid", pStrongholdRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("level", pStrongholdRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("strongholdname", pStrongholdRankings->Rankings[rank-1].stronghold_name) );
                    obj.push_back( Pair("name", pStrongholdRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("unionname", pStrongholdRankings->Rankings[rank-1].union_name) );
                    obj.push_back( Pair("vip", pStrongholdRankings->Rankings[rank-1].vip) );
                    pArray->push_back(obj);
                }
            }
            m_strongholdRankingsPages[page-1].reset(pArray);

            json_spirit::Object* pobj = new json_spirit::Object;
            m_strongholdRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_strongholdRankingsPages[page].reset();
            m_strongholdRankingsPageobj[page].reset();
        }
        m_StrongholdRankings.reset(pStrongholdRankings);
        m_updating_strongholdRankings = 0;
    }
    return;
}

void rankingMgr::_updateCopyRankings()
{
    if (m_updating_copyRankings)
    {
        m_updating_copyRankings = 0;
        Query q(GetDb());
        //副本进度排名
        gameCopyRankings* pCopyRankings = new gameCopyRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pCopyRankings->Rankings[i].cid = 0;
            pCopyRankings->Rankings[i].rank = 0;
            pCopyRankings->Rankings[i].copy = 0;
            pCopyRankings->Rankings[i].level = 0;
            pCopyRankings->Rankings[i].name= "";
            pCopyRankings->Rankings[i].union_name= "";
            pCopyRankings->Rankings[i].copy_name= "";
            pCopyRankings->Rankings[i].vip = 0;
        }
        int rank = 0;
        q.get_result("select ce.cid,ce.value,c.level,c.name,bc.name,cg.name,cd.vip from char_data_extra as ce left join charactors as c on ce.cid=c.id left join char_data as cd on c.id=cd.cid left join base_copys as bc on ce.value=bc.copyid left join char_guild_data as cgd on c.id=cgd.cid left join char_guilds as cg on cgd.gid=cg.id where ce.field=" + LEX_CAST_STR(char_data_normal_copy) + " order by ce.value desc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pCopyRankings->Rankings[rank-1].rank = rank;
            pCopyRankings->Rankings[rank-1].cid = q.getval();
            pCopyRankings->Rankings[rank-1].copy = q.getval();
            pCopyRankings->Rankings[rank-1].level = q.getval();
            pCopyRankings->Rankings[rank-1].name = q.getstr();
            const char* name = q.getstr();
            pCopyRankings->Rankings[rank-1].copy_name = name == NULL ? "" : name;
            pCopyRankings->Rankings[rank-1].union_name = q.getstr();
            pCopyRankings->Rankings[rank-1].vip = q.getval();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array *pArray = new json_spirit::Array;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pCopyRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("id", pCopyRankings->Rankings[rank-1].copy) );
                    obj.push_back( Pair("cid", pCopyRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("level", pCopyRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("copyname", pCopyRankings->Rankings[rank-1].copy_name) );
                    obj.push_back( Pair("name", pCopyRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("unionname", pCopyRankings->Rankings[rank-1].union_name) );
                    obj.push_back( Pair("vip", pCopyRankings->Rankings[rank-1].vip) );
                    pArray->push_back(obj);
                }
            }
            m_copyRankingsPages[page-1].reset(pArray);

            json_spirit::Object* pobj = new json_spirit::Object;
            m_copyRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_copyRankingsPages[page].reset();
            m_copyRankingsPageobj[page].reset();
        }
        m_CopyRankings.reset(pCopyRankings);
        m_updating_copyRankings = 0;
    }
    return;
}

void rankingMgr::_updateShenlingRankings()
{
    if (m_updating_shenlingRankings)
    {
        m_updating_shenlingRankings = 0;
        Query q(GetDb());
        //英雄排名
        gameShenlingRankings* pShenlingRankings = new gameShenlingRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pShenlingRankings->Rankings[i].cid = 0;
            pShenlingRankings->Rankings[i].rank = 0;
            pShenlingRankings->Rankings[i].sid = 0;
            pShenlingRankings->Rankings[i].level = 0;
            pShenlingRankings->Rankings[i].name= "";
            pShenlingRankings->Rankings[i].union_name= "";
            pShenlingRankings->Rankings[i].vip = 0;
        }
        int rank = 0;
        q.get_result("select ce.cid,ce.value,c.level,c.name,cg.name,cd.vip from char_data_extra as ce left join charactors as c on ce.cid=c.id left join char_data as cd on c.id=cd.cid left join char_guild_data as cgd on c.id=cgd.cid left join char_guilds as cg on cgd.gid=cg.id where ce.field=" + LEX_CAST_STR(char_data_normal_shenling) + " order by ce.value desc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pShenlingRankings->Rankings[rank-1].rank = rank;
            pShenlingRankings->Rankings[rank-1].cid = q.getval();
            pShenlingRankings->Rankings[rank-1].sid = q.getval();
            pShenlingRankings->Rankings[rank-1].level = q.getval();
            pShenlingRankings->Rankings[rank-1].name = q.getstr();
            pShenlingRankings->Rankings[rank-1].union_name = q.getstr();
            pShenlingRankings->Rankings[rank-1].vip = q.getval();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array *pArray = new json_spirit::Array;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pShenlingRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("id", pShenlingRankings->Rankings[rank-1].sid) );
                    obj.push_back( Pair("cid", pShenlingRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("level", pShenlingRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("name", pShenlingRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("unionname", pShenlingRankings->Rankings[rank-1].union_name) );
                    obj.push_back( Pair("vip", pShenlingRankings->Rankings[rank-1].vip) );
                    pArray->push_back(obj);
                }
            }
            m_shenlingRankingsPages[page-1].reset(pArray);

            json_spirit::Object* pobj = new json_spirit::Object;
            m_shenlingRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_shenlingRankingsPages[page].reset();
            m_shenlingRankingsPageobj[page].reset();
        }
        m_ShenlingRankings.reset(pShenlingRankings);
        m_updating_shenlingRankings = 0;
    }
    return;
}

void rankingMgr::_updateCharAttackRankings()
{
    if (m_updating_charAttackRankings)
    {
        m_updating_charAttackRankings = 0;
        gameCharAttackRankings* pCharAttackRankings = new gameCharAttackRankings;
        for (int i = 0; i < iRankingsPage*iRankingsPerPage; ++i)
        {
            pCharAttackRankings->Rankings[i].cid = 0;
            pCharAttackRankings->Rankings[i].rank = 0;
            pCharAttackRankings->Rankings[i].level = 0;
            pCharAttackRankings->Rankings[i].attributeExtra = 0;
            pCharAttackRankings->Rankings[i].union_name = "";
            pCharAttackRankings->Rankings[i].name = "";
            pCharAttackRankings->Rankings[i].vip = 0;
        }
        Query q(GetDb());

        int rank = 0;
        q.get_result("select ch.cid,c.level,c.name,ch.attribute as attribute,cg.name,cd.vip from char_heros as ch left join charactors as c on ch.cid=c.id left join char_data as cd on c.id=cd.cid left join char_guild_data as cgd on c.id=cgd.cid left join char_guilds as cg on cgd.gid=cg.id where ch.state='1' and attribute>0 order by attribute desc limit " + LEX_CAST_STR(iRankingsPage*iRankingsPerPage));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            ++rank;
            pCharAttackRankings->Rankings[rank-1].rank = rank;
            pCharAttackRankings->Rankings[rank-1].cid = q.getval();
            pCharAttackRankings->Rankings[rank-1].level = q.getval();
            pCharAttackRankings->Rankings[rank-1].name = q.getstr();
            pCharAttackRankings->Rankings[rank-1].attributeExtra = q.getval();
            pCharAttackRankings->Rankings[rank-1].union_name = q.getstr();
            pCharAttackRankings->Rankings[rank-1].vip = q.getval();
        }
        q.free_result();

        int maxpage = (rank + iRankingsPerPage - 1)/iRankingsPerPage;

        for (int page = 1; page <= maxpage; ++page)
        {
            json_spirit::Array* pArray = new json_spirit::Array;
            for (int i = 0; i < iRankingsPerPage; ++i)
            {
                int rank = (page - 1) * iRankingsPerPage + i + 1;
                if (pCharAttackRankings->Rankings[rank-1].cid > 0)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("rank", rank) );
                    obj.push_back( Pair("id", pCharAttackRankings->Rankings[rank-1].cid) );
                    obj.push_back( Pair("add", pCharAttackRankings->Rankings[rank-1].attributeExtra) );
                    obj.push_back( Pair("level", pCharAttackRankings->Rankings[rank-1].level) );
                    obj.push_back( Pair("name", pCharAttackRankings->Rankings[rank-1].name) );
                    obj.push_back( Pair("unionname", pCharAttackRankings->Rankings[rank-1].union_name) );
                    obj.push_back( Pair("vip", pCharAttackRankings->Rankings[rank-1].vip) );
                    pArray->push_back(obj);
                }
            }
            m_charAttackRankingsPages[page-1].reset(pArray);

            json_spirit::Object* pobj = new json_spirit::Object;
            m_charAttackRankingsPageobj[page-1].reset(pobj);
            json_spirit::Object& pageobj = *pobj;
            pageobj.push_back( Pair("maxPage", maxpage) );
            pageobj.push_back( Pair("page", page) );
            pageobj.push_back( Pair("pageNums", iRankingsPerPage) );
        }
        //原来的数据清除
        for (int page = maxpage; page < iRankingsPage; ++page)
        {
            m_charAttackRankingsPages[page].reset();
        }
        m_CharAttackRankings.reset(pCharAttackRankings);
        m_updating_charAttackRankings = 0;
    }
}

int rankingMgr::getCharRankings(int page, int cid, json_spirit::Object &robj)
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
            if (m_CharRankings.get() && m_CharRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_charRankingsPages[rpage-1].get())
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

int rankingMgr::getSilverRankings(int page, int cid, json_spirit::Object &robj)
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
            if (m_SilverRankings.get() && m_SilverRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_silverRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_silverRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_silverRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

int rankingMgr::getHeroRankings(int page, int cid, json_spirit::Object &robj)
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
            if (m_HeroRankings.get() && m_HeroRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_heroRankingsPages[rpage-1].get())
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

int rankingMgr::getStrongholdRankings(int page, int cid, json_spirit::Object &robj)
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
            if (m_StrongholdRankings.get() && m_StrongholdRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_strongholdRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_strongholdRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_strongholdRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

int rankingMgr::getCopyRankings(int page, int cid, json_spirit::Object &robj)
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
            if (m_CopyRankings.get() && m_CopyRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_copyRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_copyRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_copyRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

int rankingMgr::getShenlingRankings(int page, int cid, json_spirit::Object &robj)
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
            if (m_ShenlingRankings.get() && m_ShenlingRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_shenlingRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_shenlingRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_shenlingRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

int rankingMgr::getCharAttackRankings(int page, int cid, json_spirit::Object &robj)
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
            if (m_CharAttackRankings.get() && m_CharAttackRankings->Rankings[i].cid == cid)
            {
                rpage = (i+iRankingsPerPage)/iRankingsPerPage;
                break;
            }
        }
    }
    if (rpage < 1 || rpage > iRankingsPage || !m_charAttackRankingsPages[rpage-1].get())
    {
        rpage = 1;
    }
    boost::shared_ptr<const json_spirit::Array> cur_page = m_charAttackRankingsPages[rpage-1];
    boost::shared_ptr<const json_spirit::Object> cur_pageobj = m_charAttackRankingsPageobj[rpage-1];
    if (cur_page.get() && cur_pageobj.get())
    {
        robj.push_back( Pair("list", *(cur_page.get())) );
        robj.push_back( Pair("page", *(cur_pageobj.get())) );
    }
    return HC_SUCCESS;
}

