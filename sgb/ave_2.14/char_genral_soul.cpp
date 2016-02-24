
#include "char_general_soul.hpp"
#include "utils_all.h"
#include "data.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <list>
#include "statistics.h"
#include "singleton.h"
#include "spls_timer.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

//普通重置返回比例
const int iNormalResetReturnPercent = 70;

charGeneralSoulMgr::charGeneralSoulMgr()
{
    reload();
}

void charGeneralSoulMgr::reload()
{
    for (int i = 0; i < iMaxGeneralSoulLevel; ++i)
    {
        m_general_souls[i].level = i + 1;
        m_general_souls[i].cost = 1;
        if (i == (iMaxGeneralSoulLevel-1))
        {
            m_general_souls[i].next = NULL;
        }
        else
        {
            m_general_souls[i].next = &(m_general_souls[i+1]);
        }
    }
    Query q(GetDb());
    q.get_result("select quality,level,attack,wufang,cefang,hp from base_general_soul where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int quality = q.getval();
        int level = q.getval();
        if (level >= 1 && level <= iMaxGeneralSoulLevel
            && quality >= 0 && quality <= 5)
        {
            m_general_souls[level-1].attr[quality].attack = q.getval();
            m_general_souls[level-1].attr[quality].wufang = q.getval();
            m_general_souls[level-1].attr[quality].cefang = q.getval();
            m_general_souls[level-1].attr[quality].hp = q.getval();
        }
        else
        {
            ERR();
            cout<<"base_general_soul:quality="<<quality<<",level="<<level<<endl;
        }
    }
    q.free_result();

    q.get_result("select level,cost from base_general_soul_cost where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        if (level >= 1 && level <= iMaxGeneralSoulLevel)
        {
            m_general_souls[level-1].cost = q.getval();
        }
    }
    q.free_result();

    for (int lv = 1; lv <= iMaxGeneralSoulLevel; ++lv)
    {
        m_general_souls[lv-1].total_cost = 0;
    }

    for (int lv = 1; lv <= iMaxGeneralSoulLevel; ++lv)
    {
        for (int add_lv = lv; add_lv <= iMaxGeneralSoulLevel; ++add_lv)
        {
            for (int quality = 0; quality <= 5; ++quality)
            {
                m_general_souls[add_lv-1].total_attr[quality] += m_general_souls[lv-1].attr[quality];                
            }
            m_general_souls[add_lv-1].total_cost += m_general_souls[lv-1].cost;
        }
    }
}

base_general_soul* charGeneralSoulMgr::getSoul(int level)
{
    if (level >= 1 && level <= iMaxGeneralSoulLevel)
    {
        return &(m_general_souls[level-1]);
    }
    else
    {
        return NULL;
    }
}

//查询将魂信息
int ProcessQueryGSoulInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    //将魂数量
    int c = pc->treasureCount(treasure_type_general_soul);
    robj.push_back( Pair("soul", c) );
    
    int gid = 0;
    READ_INT_FROM_MOBJ(gid,o,"gid");
    CharGeneralData* g = pc->GetGenerals().GetGenral(gid).get();
    if (g)
    {
        robj.push_back( Pair("maxLv", iMaxGeneralSoulLevel) );
        if (g->m_general_soul)
        {
            int quality = 0;
            if (g->m_color >= 0 && g->m_color <= 5)
            {
                quality = g->m_color;
            }
            robj.push_back( Pair("level", g->m_general_soul->level) );
            robj.push_back( Pair("attack", g->m_general_soul->total_attr[quality].attack) );
            robj.push_back( Pair("pufang", g->m_general_soul->total_attr[quality].wufang) );
            robj.push_back( Pair("cefang", g->m_general_soul->total_attr[quality].cefang) );
            robj.push_back( Pair("bingli", g->m_general_soul->total_attr[quality].hp) );

            if (g->m_general_soul->next)
            {
                robj.push_back( Pair("cost", g->m_general_soul->next->cost) );
            }
            robj.push_back( Pair("resetGold", iResetGeneralSoulGold) );
            robj.push_back( Pair("resetReturn", g->m_general_soul->total_cost) );
            int ret2 = g->m_general_soul->total_cost*iNormalResetReturnPercent/100;
            if (ret2 == 0)
            {
                ret2 = 1;
            }
            robj.push_back( Pair("resetReturn2", ret2) );
        }
        else
        {
            robj.push_back( Pair("level", 0) );
            robj.push_back( Pair("attack", 0) );
            robj.push_back( Pair("pufang", 0) );
            robj.push_back( Pair("cefang", 0) );
            robj.push_back( Pair("bingli", 0) );

            base_general_soul* gs = Singleton<charGeneralSoulMgr>::Instance().getSoul(1);
            if (gs)
            {
                robj.push_back( Pair("cost", gs->cost) );
            }
        }
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

//查询将魂属性
int ProcessQueryGSoulAttr(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid,o,"gid");
    boost::shared_ptr<CharGeneralData> g = pc->GetGenerals().GetGenral(gid);
    if (g.get())
    {
        int level = 1;
        READ_INT_FROM_MOBJ(level,o,"level");
        base_general_soul* gs = Singleton<charGeneralSoulMgr>::Instance().getSoul(level);
        if (NULL == gs)
        {
            return HC_ERROR;
        }

        int quality = 0;
        if (g->m_color >= 0 && g->m_color <= 5)
        {
            quality = g->m_color;
        }
        robj.push_back( Pair("level", level) );
        robj.push_back( Pair("attack", gs->attr[quality].attack) );
        robj.push_back( Pair("pufang", gs->attr[quality].wufang) );
        robj.push_back( Pair("cefang", gs->attr[quality].cefang) );
        robj.push_back( Pair("bingli", gs->attr[quality].hp) );

        if (g->m_general_soul == NULL || g->m_general_soul->level < gs->level)
        {
            robj.push_back( Pair("cost", gs->cost) );
        }
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

//升级将魂
int ProcessUpgradeGSoul(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid,o,"gid");
    CharGeneralData* g = pc->GetGenerals().GetGenral(gid).get();
    if (g)
    {
        base_general_soul* ng = NULL;
        if (g->m_general_soul)
        {
            ng = g->m_general_soul->next;            
        }
        else
        {
            ng = Singleton<charGeneralSoulMgr>::Instance().getSoul(1);
        }
        if (NULL == ng || ng->cost <= 0)
        {
            return HC_ERROR;
        }

        int tid = treasure_type_general_soul;
        if (pc->treasureCount(tid) < ng->cost)
        {
            return HC_ERROR;
        }
        
        int err_code = 0;
        pc->addTreasure(tid, -ng->cost, err_code);
        add_statistics_of_treasure_cost(pc->m_id, pc->m_ip_address, treasure_type_general_soul, ng->cost, 100, 2, pc->m_union_id, pc->m_server_id);

        g->m_general_soul = ng;

        json_spirit::Object a;
        a.push_back( Pair("attack", ng->getAttack2(g->m_color)) );
        a.push_back( Pair("pufang", ng->getWufang2(g->m_color)) );
        a.push_back( Pair("cefang", ng->getCefang2(g->m_color)) );
        a.push_back( Pair("bingli", ng->getBingli2(g->m_color)) );
        robj.push_back( Pair("add", a) );

        //更新战力
        pc->set_attack_change();        
        //保存
        InsertSaveDb("update char_generals set soul=" + LEX_CAST_STR(ng->level) + " where id=" + LEX_CAST_STR(g->m_id));
        
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

//重置将魂
int ProcessResetGSoul(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid,o,"gid");
    CharGeneralData* g = pc->GetGenerals().GetGenral(gid).get();
    if (g)
    {
        if (g->m_general_soul)
        {
            if (g->m_general_soul->total_cost <= 0)
            {
                return HC_ERROR;
            }
            if (pc->m_bag.isFull())
            {
                return HC_ERROR_BAG_FULL;
            }
            int useGold = 0;
            READ_INT_FROM_MOBJ(useGold,o,"gold");
            if (useGold)
            {
                if (pc->addGold(-iResetGeneralSoulGold) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                int err_code = 0;
                pc->addTreasure(treasure_type_general_soul, g->m_general_soul->total_cost, err_code);
                add_statistics_of_treasure_cost(pc->m_id, pc->m_ip_address, treasure_type_general_soul, g->m_general_soul->total_cost, 101, 1, pc->m_union_id, pc->m_server_id);
                robj.push_back( Pair("returnCount", g->m_general_soul->total_cost) );
                pc->NotifyCharData();
            }
            else
            {
                //数量打折
                int ret2 = g->m_general_soul->total_cost*iNormalResetReturnPercent/100;
                if (ret2 == 0)
                {
                    ret2 = 1;
                }
                int err_code = 0;
                pc->addTreasure(treasure_type_general_soul, ret2, err_code);
                add_statistics_of_treasure_cost(pc->m_id, pc->m_ip_address, treasure_type_general_soul, ret2, 101, 1, pc->m_union_id, pc->m_server_id);
                robj.push_back( Pair("returnCount", ret2) );
            }
            g->m_general_soul = NULL;
            //更新战力
            pc->set_attack_change();        
            //保存
            InsertSaveDb("update char_generals set soul=0 where id=" + LEX_CAST_STR(g->m_id));
        }
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

void resetAllGeneralSoul(int cid, int gid, int level)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc)
    {
        base_general_soul* toGsoul = Singleton<charGeneralSoulMgr>::Instance().getSoul(level);
        int toLevel = toGsoul != NULL ? toGsoul->level : 0;
        CharTotalGenerals& ag = pc->GetGenerals();
        if (gid == 0)
        {
            for (std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = ag.m_generals.begin(); it != ag.m_generals.end(); ++it)
            {
                CharGeneralData* g = it->second.get();
                if (g && g->m_general_soul != toGsoul)
                {
                    g->m_general_soul = toGsoul;
                    //保存
                    InsertSaveDb("update char_generals set soul=" + LEX_CAST_STR(toLevel) + " where id=" + LEX_CAST_STR(g->m_id));
                }
            }
            for (std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = ag.m_fired_generals.begin(); it != ag.m_fired_generals.end(); ++it)
            {
                CharGeneralData* g = it->second.get();
                if (g && g->m_general_soul != toGsoul)
                {
                    g->m_general_soul = toGsoul;
                    //保存
                    InsertSaveDb("update char_generals set soul=" + LEX_CAST_STR(toLevel) + " where id=" + LEX_CAST_STR(g->m_id));
                }
            }
            //更新战力
            pc->set_attack_change();

            int cur_count = pc->treasureCount(treasure_type_general_soul);
            int total_get = 0;
            Query q(GetDb());
            q.get_result("SELECT sum(hnums) FROM  `admin_count_smost` WHERE  `cid` =" + LEX_CAST_STR(pc->m_id) + " AND  `treasure_id` =4012 and `stype`='1' and `type`!=101");
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                total_get = q.getval();
            }
            q.free_result();
            int err_code = 0;
            pc->addTreasure(treasure_type_general_soul, total_get - cur_count, err_code);

            q.execute("delete from `admin_count_smost` WHERE  `cid` =" + LEX_CAST_STR(pc->m_id) + " AND  `treasure_id` =4012 and `stype`='1' and `type`=101");
            CHECK_DB_ERR(q);
            q.execute("delete from `admin_count_smost` WHERE  `cid` =" + LEX_CAST_STR(pc->m_id) + " AND  `treasure_id` =4012 and `stype`='2'");
            CHECK_DB_ERR(q);            
        }
        else
        {
            CharGeneralData* g = pc->GetGenerals().GetGenral(gid).get();
            if (g && g->m_general_soul != toGsoul)
            {
                g->m_general_soul = toGsoul;
                //保存
                InsertSaveDb("update char_generals set soul=" + LEX_CAST_STR(toLevel) + " where id=" + LEX_CAST_STR(g->m_id));
                //更新战力
                pc->set_attack_change();
            }
        }
    }
}

