
#include "recharge_event.h"
#include "utils_all.h"
#include "errcode_def.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "utils_lang.h"

using namespace json_spirit;
using namespace net;

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

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

        json_spirit::Array list;
        for (std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = single_event.m_events.begin(); it != single_event.m_events.end(); ++it)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("charge", it->first) );
            if (pchr->m_single_recharge_reward.find(it->first) != pchr->m_single_recharge_reward.end())
            {
                obj.push_back( Pair("canGet", pchr->m_single_recharge_reward[it->first]) );
            }
            else
            {
                obj.push_back( Pair("canGet", 0) );
            }
            json_spirit::Array get_list;
            itemlistToArray(it->second->items, get_list);
            obj.push_back( Pair("get", get_list) );
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
        json_spirit::Array list;
        for (std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = total_event.m_events.begin(); it != total_event.m_events.end(); ++it)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("charge", it->first) );
            if (pchr->m_total_recharge_reward_get.find(it->first) != pchr->m_total_recharge_reward_get.end())
            {
                obj.push_back( Pair("hasGet", 1) );
            }
            else
            {
                obj.push_back( Pair("hasGet", 0) );
            }
            if (pchr->total_recharge >= it->first && pchr->m_total_recharge_reward_get.find(it->first) == pchr->m_total_recharge_reward_get.end())
            {
                obj.push_back( Pair("canGet", 1) );
            }
            else
            {
                obj.push_back( Pair("canGet", 0) );
            }
            json_spirit::Array get_list;
            itemlistToArray(it->second->items, get_list);
            obj.push_back( Pair("get", get_list) );
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
int recharge_event_mgr::getReward(CharData* pc, int type, int count, json_spirit::Object& robj)
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
            if (!pc->m_bag.hasSlot(itemlistNeedBagSlot(items)))
            {
                return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
            }
            //给奖励
            giveLoots(pc, items, NULL, &robj, true, loot_recharge_event);

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
            if (!pc->m_bag.hasSlot(itemlistNeedBagSlot(items)))
            {
                return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
            }
            //给奖励
            giveLoots(pc, items, NULL, &robj, true, loot_recharge_event);
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

            q.get_result("select rnum,type,id,num,extra from custom_recharge_event_reward where rtype='1' order by rnum");
            while (q.fetch_row())
            {
                int rnum = q.getval();
                Item item;
                item.type = q.getval();
                item.id = q.getval();
                item.nums = q.getval();
                item.extra = q.getval();
                if (single_event.m_events.find(rnum) == single_event.m_events.end())
                {
                    boost::shared_ptr<recharge_event_reward> sp;
                    recharge_event_reward* re = new recharge_event_reward;
                    re->count = rnum;
                    sp.reset(re);
                    single_event.m_events[rnum] = sp;
                }
                single_event.m_events[rnum]->items.push_back(item);
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

            q.get_result("select rnum,type,id,num,extra from custom_recharge_event_reward where rtype='2' order by rnum");
            while (q.fetch_row())
            {
                int rnum = q.getval();
                Item item;
                item.type = q.getval();
                item.id = q.getval();
                item.nums = q.getval();
                item.extra = q.getval();
                if (total_event.m_events.find(rnum) == total_event.m_events.end())
                {
                    boost::shared_ptr<recharge_event_reward> sp;
                    recharge_event_reward* re = new recharge_event_reward;
                    re->count = rnum;
                    sp.reset(re);
                    total_event.m_events[rnum] = sp;
                }
                total_event.m_events[rnum]->items.push_back(item);
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
    //首充活动
    if (type == 0)
    {
        first_event.m_events.clear();
        q.get_result("select rtype,type,id,num,extra from custom_first_recharge_event_reward where 1 order by rtype");
        while (q.fetch_row())
        {
            int rtype = q.getval();
            Item item;
            item.type = q.getval();
            item.id = q.getval();
            item.nums = q.getval();
            item.extra = q.getval();
            if (first_event.m_events.find(rtype) == first_event.m_events.end())
            {
                boost::shared_ptr<recharge_event_reward> sp;
                recharge_event_reward* re = new recharge_event_reward;
                re->count = 0;
                sp.reset(re);
                first_event.m_events[rtype] = sp;
            }
            first_event.m_events[rtype]->items.push_back(item);
        }
        q.free_result();
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
int recharge_event_mgr::updateRechargeEvent(int cid, int count)
{
    boost::shared_ptr<char_recharge_event> chr = getChar(cid);
    if (!chr.get())
    {
        return HC_ERROR;
    }
    int state = 0;
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
                    state = 2;
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
                state = 1;
            }
            pchr->m_single_recharge_can_get = 1;
        }
    }
    if (state)
    {
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc)
        {
            pc->updateTopButton(top_button_rechargeAction, state);
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

int recharge_event_mgr::getRechargeEventState(CharData* pc)
{
    //单笔冲
    int state = getCanget(pc, 1);
    if (state > 0)
    {
        return 1;
    }
    //累计冲
    state = getCanget(pc, 2);
    if (state > 0)
    {
        return 2;
    }
    return 0;
}

int recharge_event_mgr::queryFirstRechargeEvent(CharData* pc, json_spirit::Object& robj)
{
    //每天第一闪
    int first_view = pc->queryExtraData(char_data_type_daily, char_data_daily_view_first_recharge);
    if (0 == first_view)
    {
        pc->setExtraData(char_data_type_daily, char_data_daily_view_first_recharge, 1);
    }
    //首充活动
    int type = 1;//默认活动1
    int first_state = pc->queryExtraData(char_data_type_normal, char_data_normal_first_recharge_event1);
    if (first_state == 2)//已经完成
    {
        first_state = pc->queryExtraData(char_data_type_normal, char_data_normal_first_recharge_event2);
        if (first_state == 2)
        {
            return HC_ERROR;
        }
        type = 2;
    }
    std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = first_event.m_events.find(type);
    if (it != first_event.m_events.end())
    {
        json_spirit::Array get_list;
        itemlistToArray(it->second->items, get_list);
        robj.push_back( Pair("get", get_list) );
    }
    robj.push_back( Pair("canGet", first_state) );
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("cur_recharge", pc->m_total_recharge) );
    if (first_view == 0 && (first_state == 0 || first_state == 1))
    {
        pc->updateTopButton(top_button_first_recharge, first_state);
    }
    return HC_SUCCESS;
}

//更新充值活动奖励
void recharge_event_mgr::updateFirstRechargeEvent(CharData* pc, int org_total, int total)
{
    //首充活动
    if (pc == NULL)
        return;
    int state = 0;
    int type = 1;//默认活动1
    int first_state = pc->queryExtraData(char_data_type_normal, char_data_normal_first_recharge_event1);
    if (first_state == 0)//还未完成
    {
        if (org_total == 0 && total > 0)
        {
            pc->setExtraData(char_data_type_normal, char_data_normal_first_recharge_event1, 1);
            state = 1;
        }
    }
    first_state = pc->queryExtraData(char_data_type_normal, char_data_normal_first_recharge_event2);
    if (first_state == 0)
    {
        if (org_total < 1500 && total >= 1500)
        {
            pc->setExtraData(char_data_type_normal, char_data_normal_first_recharge_event2, 1);
            state = 1;
        }
    }
    if (state)
    {
        pc->updateTopButton(top_button_first_recharge, state);
    }
    return;
}

int recharge_event_mgr::getFirstReward(CharData* pc, int type, json_spirit::Object& robj)
{
    int check_data = 0;
    if (type == 1)
    {
        check_data = char_data_normal_first_recharge_event1;
    }
    else if(type == 2)
    {
        check_data = char_data_normal_first_recharge_event2;
    }
    //状态检测
    if (pc->queryExtraData(char_data_type_normal, check_data) != 1)
    {
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<recharge_event_reward> >::iterator it = first_event.m_events.find(type);
    if (it != first_event.m_events.end())
    {
        if (it->second.get())
        {
            std::list<Item> items = it->second->items;
            if (!pc->m_bag.hasSlot(itemlistNeedBagSlot(items)))
            {
                return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
            }
            //给奖励
            giveLoots(pc, items, NULL, &robj, true, loot_recharge_event);
    		std::string msg = "";
    		switch (type)
    		{
    			case 1:
    				msg = strRecharge1Msg;
    				str_replace(msg, "$W", MakeCharNameLink(pc->m_name,pc->m_nick.get_string()));
                    str_replace(msg, "$R", itemlistToStringWithLink(items));
    				break;
    			case 2:
    				msg = strRecharge2Msg;
    				str_replace(msg, "$W", MakeCharNameLink(pc->m_name,pc->m_nick.get_string()));
                    str_replace(msg, "$R", itemlistToStringWithLink(items));
    				break;
    		}
    		if (msg != "")
    		{
    			GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    		}
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

void recharge_event_mgr::getButton(CharData* pc, json_spirit::Array& list)
{
    //充值活动
    if (pc->isRechargeActionOpen())
    {
        int active = getRechargeEventState(pc);
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_rechargeAction) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    if (pc->isFirstRechargeOpen())
    {
        //首充活动
        int first_state = pc->queryExtraData(char_data_type_normal, char_data_normal_first_recharge_event1);
        if (first_state == 2)//已经完成
        {
            first_state = pc->queryExtraData(char_data_type_normal, char_data_normal_first_recharge_event2);
        }
        if (first_state == 0 || first_state == 1)
        {
            //每天第一闪
            if (pc->queryExtraData(char_data_type_daily, char_data_daily_view_first_recharge) == 0)
            {
                first_state = 1;
            }
            json_spirit::Object obj;
            obj.push_back( Pair("type", top_button_first_recharge) );
            obj.push_back( Pair("active", first_state) );
            list.push_back(obj);
        }
    }
    return;
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
int ProcessQueryRechargeEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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

//查询首充活动
int ProcessQueryFirstRechargeEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    return recharge_event_mgr::getInstance()->queryFirstRechargeEvent(pc, robj);
}

