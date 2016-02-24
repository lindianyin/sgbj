
#include "recharge_event.h"
#include "utils_all.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "net.h"

using namespace json_spirit;
using namespace net;

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
class Combat;
int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

recharge_event_mgr* recharge_event_mgr::m_handle = NULL;
recharge_event_mgr* recharge_event_mgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new recharge_event_mgr();
        m_handle->reload(0);
    }
    return m_handle;
}

//查询充值活动
int recharge_event_mgr::queryRechargeEvent(int cid, int type, json_spirit::Object& robj)
{
    boost::shared_ptr<char_recharge_event> chr = getChar(cid);
    if (!chr.get())
    {
        return HC_ERROR;
    }
    char_recharge_event* pchr = chr.get();
    checkState();

    if (1 == type)
    {
        json_spirit::Object& singleEvent = robj;
        singleEvent.push_back( Pair("type", 1) );

        singleEvent.push_back( Pair("starttime", single_event.m_start_time) );
        singleEvent.push_back( Pair("endtime", single_event.m_end_time) );

        //singleEvent.push_back( Pair("leftSecs", single_event.m_end_time - time(NULL)) );
        
        json_spirit::Array list;
        for (std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = single_event.m_events.begin(); it != single_event.m_events.end(); ++it)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("charge", it->first) );
            obj.push_back( Pair("memo", it->second->m_reward_string) );
            if (pchr->m_single_recharge_reward.find(it->first) != pchr->m_single_recharge_reward.end())
            {
                obj.push_back( Pair("canGet", pchr->m_single_recharge_reward[it->first]) );
            }
            list.push_back(obj);
        }
        singleEvent.push_back( Pair("list", list) );
    }
    else
    {
        json_spirit::Object& totalEvent = robj;
        totalEvent.push_back( Pair("type", 2) );
        totalEvent.push_back( Pair("total", pchr->total_recharge) );
        totalEvent.push_back( Pair("starttime", total_event.m_start_time) );
        totalEvent.push_back( Pair("endtime", total_event.m_end_time) );
        //totalEvent.push_back( Pair("leftSecs", total_event.m_end_time - time(NULL)) );
        json_spirit::Array list;
        for (std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = total_event.m_events.begin(); it != total_event.m_events.end(); ++it)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("charge", it->first) );
            obj.push_back( Pair("memo", it->second->m_reward_string) );
            if (pchr->total_recharge >= it->first && pchr->m_total_recharge_reward_get.find(it->first) == pchr->m_total_recharge_reward_get.end())
            {
                obj.push_back( Pair("canGet", 1) );
            }
            list.push_back(obj);
        }
        totalEvent.push_back( Pair("list", list) );
    }
    return HC_SUCCESS;
}

//领取充值奖励是否可以领取
int recharge_event_mgr::getCanget(CharData* pc, int type)
{
    if (!isOpen(type))
    {
        return -1;
    }
    boost::shared_ptr<char_recharge_event> chr = getChar(pc->m_id);
    if (!chr.get())
    {
        return 0;
    }

    char_recharge_event* pchr = chr.get();
    if (type == 1)
    {
        return pchr->m_single_recharge_can_get;
    }
    else
    {
        return pchr->m_total_recharge_can_get;
    }
}

//领取充值奖励
int recharge_event_mgr::getReward(CharData* pc, int type, int count)
{
    boost::shared_ptr<char_recharge_event> chr = getChar(pc->m_id);
    if (!chr.get())
    {
        return HC_ERROR;
    }
    checkState();
    char_recharge_event* pchr = chr.get();
    if (type == 1)
    {
        if (pchr->m_single_recharge_reward.find(count) != pchr->m_single_recharge_reward.end()
            && pchr->m_single_recharge_reward[count] > 0)
        {
            boost::shared_ptr<recharge_event_reward> r = single_event.getReward(count);
            if (!r.get())
            {
                return HC_ERROR;
            }
            std::list<Item> items = r->items;
            //给奖励
            giveLoots(pc, items, pc->m_area, pc->m_level, 1, NULL, NULL, true, 0);

            //加领取记录
            --pchr->m_single_recharge_reward[count];
            if (pchr->m_single_recharge_reward[count])
            {
                InsertSaveDb("replace into char_recharge_event_records (cid,rtype,rnum,count) values ("
                    + LEX_CAST_STR(pc->m_id) + ","
                    + "'1'," + LEX_CAST_STR(count)
                    + "," + LEX_CAST_STR(pchr->m_single_recharge_reward[count]) + ")");
            }
            else
            {
                InsertSaveDb("delete from char_recharge_event_records where cid=" + LEX_CAST_STR(pc->m_id) + " and rtype='1' and rnum=" + LEX_CAST_STR(count));
            }
            pchr->m_single_recharge_can_get = 0;
            for (std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = single_event.m_events.begin(); it != single_event.m_events.end(); ++it)
            {
                if (pchr->m_single_recharge_reward.find(it->first) != pchr->m_single_recharge_reward.end())
                {
                    if (pchr->m_single_recharge_reward[it->first])
                    {
                        pchr->m_single_recharge_can_get = 1;
                        break;
                    }
                }
            }
        }
        else
        {
            return HC_ERROR;
        }
    }
    else if (2 == type)
    {
        if (pchr->m_total_recharge_reward_get.find(count) == pchr->m_total_recharge_reward_get.end())
        {
            boost::shared_ptr<recharge_event_reward> r = total_event.getReward(count);
            if (!r.get())
            {
                return HC_ERROR;
            }
            Query q(GetDb());
            q.get_result("select total from char_recharge_event_total where cid=" + LEX_CAST_STR(pc->m_id));
            if (q.fetch_row() && q.getval() >= count)
            {
                q.free_result();
            }
            else
            {
                q.free_result();
                return HC_ERROR;
            }
            std::list<Item> items = r->items;
            //给奖励
            giveLoots(pc, items, pc->m_area, pc->m_level, 1, NULL, NULL, true, 0);
            //加领取记录
            pchr->m_total_recharge_reward_get[count] = 1;
            InsertSaveDb("replace into char_recharge_event_records (cid,rtype,rnum,count) values ("
                    + LEX_CAST_STR(pc->m_id) + ","
                    + "'2'," + LEX_CAST_STR(count)
                    + "," + LEX_CAST_STR(pchr->m_total_recharge_reward_get[count]) + ")");

            pchr->m_total_recharge_can_get = 0;
            for (std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = total_event.m_events.begin(); it != total_event.m_events.end(); ++it)
            {
                if (pchr->total_recharge >= it->first && pchr->m_total_recharge_reward_get.find(it->first) == pchr->m_total_recharge_reward_get.end())
                {
                    pchr->m_total_recharge_can_get = 1;
                    break;
                }
            }
        }
        else
        {
            return HC_ERROR;
        }
    }
    return HC_SUCCESS;
}

//重新加载充值活动
int recharge_event_mgr::reload(int type)
{
    Query q(GetDb());

    if (type == 0 || type == 1)
    {
        single_event.m_events.clear();
        q.get_result("select start_time,end_time from custom_recharge_event where type='1' and enable='1'");
        if (q.fetch_row())
        {
            single_event.m_start_time = q.getval();
            single_event.m_end_time = q.getval();

            q.free_result();

            q.get_result("select rnum,type,id,num from custom_recharge_event_reward where rtype='1' order by rnum");
            while (q.fetch_row())
            {
                int rnum = q.getval();
                Item item;
                item.type = q.getval();
                item.id = q.getval();
                item.nums = q.getval();
                if (single_event.m_events.find(rnum) == single_event.m_events.end())
                {
                    boost::shared_ptr<recharge_event_reward> sp;
                    recharge_event_reward* re = new recharge_event_reward;
                    re->count = rnum;        
                    re->m_reward_string = "";
                    sp.reset(re);
                    single_event.m_events[rnum] = sp;
                }
                single_event.m_events[rnum]->items.push_back(item);
                if (single_event.m_events[rnum]->m_reward_string != "")
                {
                    single_event.m_events[rnum]->m_reward_string += ",";
                }
                single_event.m_events[rnum]->m_reward_string += item.toString();
            }
        }
        else
        {
            single_event.m_start_time = 0;
            single_event.m_end_time = 0;
        }
        q.free_result();
    }

    time_t timenow = time(NULL);
    single_event.m_isOpen = (timenow >= single_event.m_start_time && timenow <= single_event.m_end_time);

    if (type == 0 || type == 2)
    {
        total_event.m_events.clear();
        q.get_result("select start_time,end_time from custom_recharge_event where type='2' and enable='1'");
        if (q.fetch_row())
        {
            total_event.m_start_time = q.getval();
            total_event.m_end_time = q.getval();

            q.free_result();

            q.get_result("select rnum,type,id,num from custom_recharge_event_reward where rtype='2' order by rnum");
            while (q.fetch_row())
            {
                int rnum = q.getval();
                Item item;
                item.type = q.getval();
                item.id = q.getval();
                item.nums = q.getval();
                if (total_event.m_events.find(rnum) == total_event.m_events.end())
                {
                    boost::shared_ptr<recharge_event_reward> sp;
                    recharge_event_reward* re = new recharge_event_reward;
                    re->count = rnum;        
                    re->m_reward_string = "";
                    sp.reset(re);
                    total_event.m_events[rnum] = sp;
                }
                total_event.m_events[rnum]->items.push_back(item);
                if (total_event.m_events[rnum]->m_reward_string != "")
                {
                    total_event.m_events[rnum]->m_reward_string += ",";
                }
                total_event.m_events[rnum]->m_reward_string += item.toString();
            }
        }
        else
        {
            total_event.m_start_time = 0;
            total_event.m_end_time = 0;
        }
        q.free_result();
    }
    total_event.m_isOpen = (timenow >= total_event.m_start_time && timenow <= total_event.m_end_time);
    //活动结束就清除奖励
    if (!single_event.isOpen())
    {
        reset(1);
    }
    if (!total_event.isOpen())
    {
        reset(2);
    }
    return 0;
}

//充值活动是否开启
bool recharge_event_mgr::isOpen(int type)
{
    if (1 == type)
    {
        return single_event.isOpen();
    }
    else if (2 == type)
    {
        return total_event.isOpen();
    }
    if (single_event.isOpen())
    {
        //cout<<"single event open..."<<endl;
        return true;
    }
    return total_event.isOpen();
}

int recharge_event_mgr::reset(int type)
{
    switch (type)
    {
        case 1:
        {
            for (std::map<int, boost::shared_ptr<char_recharge_event> >::iterator it = m_char_recharge_events.begin(); it != m_char_recharge_events.end(); ++it)
            {
                if (it->second.get())
                {
                    it->second->m_single_recharge_reward.clear();
                    //it->second->m_total_recharge_reward_get.clear();
                }
            }
            InsertSaveDb("delete from char_recharge_event_records where rtype='1'");
            break;
        }
        case 2:
        {
            for (std::map<int, boost::shared_ptr<char_recharge_event> >::iterator it = m_char_recharge_events.begin(); it != m_char_recharge_events.end(); ++it)
            {
                if (it->second.get())
                {
                    //it->second->m_single_recharge_reward.clear();
                    it->second->m_total_recharge_reward_get.clear();
                    it->second->total_recharge = 0;
                }
            }
            InsertSaveDb("delete from char_recharge_event_records where rtype='2'");
            InsertSaveDb("TRUNCATE TABLE char_recharge_event_total");
            break;
        }
        default:
        {
            m_char_recharge_events.clear();
            InsertSaveDb("TRUNCATE TABLE char_recharge_event_records");
            InsertSaveDb("TRUNCATE TABLE char_recharge_event_total");
            break;
        }
    }
    return 0;
}

//更新充值活动奖励
int recharge_event_mgr::updateRechargeEvent(int cid, int count, time_t rt)
{
    boost::shared_ptr<char_recharge_event> chr = getChar(cid);
    if (!chr.get())
    {
        return HC_ERROR;
    }
    bool needNotify = false;
    char_recharge_event* pchr = chr.get();
    //累计充值活动开启中
    if (total_event.isOpen())
    {
        pchr->total_recharge += count;
        if (pchr->total_recharge)
        {
            InsertSaveDb("replace into char_recharge_event_total (cid,total) values ("
                + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(pchr->total_recharge) + ")");
        }
        pchr->m_total_recharge_can_get = 0;
        for (std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = total_event.m_events.begin(); it != total_event.m_events.end(); ++it)
        {
            if (pchr->total_recharge >= it->first && pchr->m_total_recharge_reward_get.find(it->first) == pchr->m_total_recharge_reward_get.end())
            {
                if (pchr->m_total_recharge_can_get == 0)
                {
                    needNotify = true;
                }
                pchr->m_total_recharge_can_get = 1;
                break;
            }
        }
    }
    if (single_event.isOpen())
    {
        boost::shared_ptr<recharge_event_reward> r = single_event.getBestReward(count);
        if (r.get())
        {
            ++pchr->m_single_recharge_reward[r->count];
            if (pchr->m_single_recharge_reward[r->count])
            {
                InsertSaveDb("replace into char_recharge_event_records (cid,rtype,rnum,count) values ("
                    + LEX_CAST_STR(cid) + ","
                    + "'1'," + LEX_CAST_STR(r->count)
                    + "," + LEX_CAST_STR(pchr->m_single_recharge_reward[r->count]) + ")");
            }
            if (pchr->m_single_recharge_can_get == 0)
            {
                needNotify = true;
            }
            pchr->m_single_recharge_can_get = 1;
        }
    }
    if (needNotify)
    {
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc)
        {
            pc->notifyEventState(top_level_event_opening, 1, 0);
        }
    }
    return HC_SUCCESS;
}

void recharge_event_mgr::checkState()
{
    bool pre_single_state = single_event.m_isOpen;
    time_t timenow = time(NULL);
    single_event.m_isOpen = (timenow >= single_event.m_start_time && timenow <= single_event.m_end_time);

    bool pre_total_state = total_event.m_isOpen;
    total_event.m_isOpen = (timenow >= total_event.m_start_time && timenow <= total_event.m_end_time);

    if (pre_single_state != single_event.m_isOpen)
    {
        reset(1);
    }
    if (pre_total_state != total_event.m_isOpen)
    {
        reset(2);
    }
}

boost::shared_ptr<char_recharge_event> recharge_event_mgr::getChar(int cid)
{
    std::map<int, boost::shared_ptr<char_recharge_event> >::iterator it = m_char_recharge_events.find(cid);
    if (it != m_char_recharge_events.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<char_recharge_event> sp;
        char_recharge_event* re = new char_recharge_event;
        re->m_single_recharge_can_get = 0;
        re->m_total_recharge_can_get = 0;
        re->cid = cid;
        re->total_recharge = 0;
        Query q(GetDb());
        q.get_result("select total from char_recharge_event_total where cid=" + LEX_CAST_STR(cid));
        if (q.fetch_row())
        {
            re->total_recharge = q.getval();
        }
        q.free_result();

        q.get_result("select rtype,rnum,count from char_recharge_event_records where cid=" + LEX_CAST_STR(cid));
        while (q.fetch_row())
        {
            int rtype = q.getval();
            int rnum = q.getval();
            int count = q.getval();
            if (1 == rtype)
            {
                re->m_single_recharge_reward[rnum] = count;
                if (count)
                {
                    re->m_single_recharge_can_get = 1;
                }
            }
            else if (2 == rtype)
            {
                re->m_total_recharge_reward_get[rnum] = count;
            }
        }
        q.free_result();

        for (std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = total_event.m_events.begin(); it != total_event.m_events.end(); ++it)
        {
            if (re->total_recharge >= it->first && re->m_total_recharge_reward_get.find(it->first) == re->m_total_recharge_reward_get.end())
            {
                re->m_total_recharge_can_get = 1;
                break;
            }
        }

        sp.reset(re);
        m_char_recharge_events[cid] = sp;
        return sp;
    }
}

//查询奖励
boost::shared_ptr<recharge_event_reward> recharge_event::getReward(int count)
{
    if (m_events.find(count) != m_events.end())
    {
        return m_events[count];
    }
    else
    {
        boost::shared_ptr<recharge_event_reward> tmp;
        return tmp;
    }
}

//查询奖励
boost::shared_ptr<recharge_event_reward> recharge_event::getBestReward(int count)
{
    boost::shared_ptr<recharge_event_reward> r;
    for (std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = m_events.begin(); it != m_events.end(); ++it)
    {
        if (count >= it->first)
        {
            r = it->second;
        }
        else
        {
            return r;
        }
    }
    return r;
}

//充值活动是否开启
bool recharge_event::isOpen()
{
    return m_isOpen;
}

//查询充值活动
int ProcessQueryRechargeEvent(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    return recharge_event_mgr::getInstance()->queryRechargeEvent(pc->m_id, type, robj);
}

//领取充值活动奖励
int ProcessGetRechargeEventReward(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    int charge = 100;
    READ_INT_FROM_MOBJ(charge,o,"charge");
    ret = recharge_event_mgr::getInstance()->getReward(pc, type, charge);
    if (HC_SUCCESS == ret)
    {
        //更新开服活动按钮状态
        int state = cdata->getOpeningState();
        cdata->notifyEventState(top_level_event_opening, state, 0);
    }
    return ret;
}

