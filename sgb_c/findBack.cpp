
#include "findBack.h"
#include "ThreadLocalSingleton.h"
#include "errcode_def.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "SaveDb.h"
#include "spls_timer.h"

Database& GetDb();
extern volatile int iTreasureTimes;

void baseFindBack::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", id));
    obj.push_back( Pair("type", type));
    obj.push_back( Pair("name", name));
    obj.push_back( Pair("memo", memo));
    obj.push_back( Pair("cost", cost));
    obj.push_back( Pair("extra", extra));
    if (type < FIND_TIMES_START)
    {
        obj.push_back( Pair("silver", silver));
        obj.push_back( Pair("char_exp", char_exp));
        #if 0
        json_spirit::Array base_get;
        json_spirit::Object obj;
        //筹码历练
        {
            obj.clear();
            Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_SILVER, silver, 0);
            itm.toObj(obj);
            base_get.push_back(obj);
        }
        {
            obj.clear();
            Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_CHAR_EXP, char_exp, 0);
            itm.toObj(obj);
            base_get.push_back(obj);
        }
        obj.push_back( Pair("get", base_get));
        #endif
    }
    return;
}

findBackMgr::findBackMgr()
{
    Query q(GetDb());
    q.get_result("select id,type,silver,char_exp,extra,cost,name,memo from base_find_back where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseFindBack> b(new baseFindBack());
        b->id = id;
        b->type = q.getval();
        b->silver = q.getval();
        b->char_exp = q.getval();
        b->extra = q.getval();
        b->cost = q.getval();
        b->name = q.getstr();
        b->memo = q.getstr();
        m_total_find[id] = b;
    }
    q.free_result();
}

void findBackMgr::getButton(CharData* pc, json_spirit::Array& list)
{
    if (pc->canFindBack())
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_find_back) );
        obj.push_back( Pair("active", 0) );
        list.push_back(obj);
    }
    return;
}

void findBackMgr::getList(CharData* pc, json_spirit::mObject& o, json_spirit::Object& robj)
{
    if (pc->canFindBack())
    {
        //页面信息
        int page = 1, nums_per_page = 8;
        READ_INT_FROM_MOBJ(page, o, "page");
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
        json_spirit::Array list;
        int first_nums = nums_per_page * (page - 1)+ 1;
        int last_nums = nums_per_page * page;
        std::map<int, boost::shared_ptr<baseFindBack> >::iterator it = m_total_find.begin();
        while (it != m_total_find.end())
        {
            if (it->second.get())
            {
                ++cur_nums;
                if (cur_nums >= first_nums && cur_nums <= last_nums)
                {
                    json_spirit::Object o;
                    it->second->toObj(o);
                    o.push_back( Pair("can_findback", pc->queryExtraData(char_data_type_daily, char_data_daily_findback_start + it->second->id) == 0));
                    list.push_back(o);
                }
            }
            ++it;
        }
        robj.push_back( Pair("list", list));
        int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
        if (maxpage == 0)
        {
            maxpage = 1;
        }
        json_spirit::Object pageobj;
        pageobj.push_back( Pair("maxPage", maxpage) );
        pageobj.push_back( Pair("page", page) );
        pageobj.push_back( Pair("pageNums", nums_per_page) );
        robj.push_back( Pair("page", pageobj) );
    }
    return;
}

boost::shared_ptr<baseFindBack> findBackMgr::getBaseFindBack(int id)
{
    std::map<int, boost::shared_ptr<baseFindBack> >::iterator it = m_total_find.find(id);
    if (it != m_total_find.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<baseFindBack> b;
    b.reset();
    return b;
}

int findBackMgr::findBack(CharData* pc, int id, int cost, json_spirit::Object& robj)
{
    boost::shared_ptr<baseFindBack> b = getBaseFindBack(id);
    if (!b.get())
        return HC_ERROR;
    if (cost)
    {
        if (pc->gold() < b->cost)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
    }
    bool success = false;
    if (b->type < FIND_TIMES_START)
    {
        int silver_get = b->silver;
        int char_exp_get = b->char_exp;
        if (!cost)
        {
            silver_get /= 2;
            char_exp_get /= 2;
        }
        std::list<Item> items;
        //筹码历练
        {
            Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_SILVER, silver_get, 0);
            items.push_back(itm);
        }
        {
            Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_CHAR_EXP, char_exp_get, 0);
            items.push_back(itm);
        }
        giveLoots(pc,items,NULL,&robj,true,loot_find_back);
        success = true;
    }
    else
    {
        switch(b->type)
        {
            case FIND_DAILY_SCORE:
                {
                    if (pc->m_score_tasks.m_task_cnt >= iDailyScoreTaskMax)
                    {
                        pc->m_score_tasks.dailyUpdate();
                        if (!cost)
                        {
                            pc->m_score_tasks.m_task_cnt = b->extra / 2;
                            pc->m_score_tasks.SaveData();
                        }
                        success = true;
                    }
                    else
                    {
                        return HC_ERROR_FIND_BACK;
                    }
                }
                break;
            case FIND_COPY:
                {
                    int has_attack = pc->queryExtraData(char_data_type_daily,char_data_daily_attack_copy);
                    if (has_attack >= iCopyTotal)
                    {
                        int reset_result = 0;
                        if (!cost)
                        {
                            reset_result = b->extra / 2;
                        }
                        pc->setExtraData(char_data_type_daily,char_data_daily_attack_copy, reset_result);
                        success = true;
                    }
                    else
                    {
                        return HC_ERROR_FIND_BACK;
                    }
                }
                break;
            case FIND_TREASURE:
                {
                    int has_done = pc->queryExtraData(char_data_type_daily, char_data_daily_treasure);
                    if (has_done >= iTreasureTimes)
                    {
                        int reset_result = 0;
                        if (!cost)
                        {
                            reset_result = b->extra / 2;
                        }
                        pc->setExtraData(char_data_type_daily,char_data_daily_treasure, reset_result);
                        success = true;
                    }
                    else
                    {
                        return HC_ERROR_FIND_BACK;
                    }
                }
                break;
            case FIND_LEVY:
                {
                    int reset_result = b->extra;
                    if (!cost)
                    {
                        reset_result = b->extra / 2;
                    }
                    pc->setExtraData(char_data_type_daily,char_data_daily_free_levy, reset_result);
                    pc->updateTopButton(top_button_city_levy, 1);
                    success = true;
                }
                break;
        }
    }
    if (success)
    {
        if (cost)
        {
            if (pc->subGold(b->cost,gold_cost_find_back) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
        }
        pc->setExtraData(char_data_type_daily, char_data_daily_findback_start + id, 1);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int ProcessQueryFindBackList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    if (!cdata->canFindBack())
    {
        return HC_ERROR;
    }
    Singleton<findBackMgr>::Instance().getList(cdata.get(),o,robj);
    return HC_SUCCESS;
}

int ProcessFindBack(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 1, cost = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    READ_INT_FROM_MOBJ(cost,o,"cost");
    if (!cdata->canFindBack())
    {
        return HC_ERROR;
    }
    if (cdata->queryExtraData(char_data_type_daily, char_data_daily_findback_start + id) == 0)
    {
        return Singleton<findBackMgr>::Instance().findBack(cdata.get(),id,cost,robj);
    }
    return HC_ERROR;
}

