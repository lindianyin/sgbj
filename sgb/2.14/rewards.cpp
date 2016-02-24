#include "rewards.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "singleton.h"
#include "statistics.h"

Database& GetDb();

extern void InsertSaveDb(const std::string& sql);

int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

void char_rewards::save()
{
    InsertSaveDb("delete from char_all_rewards where cid="+LEX_CAST_STR(cid)+" and reward_type="+LEX_CAST_STR(type));
    if (state == 0)
    {
        std::list<Item>::iterator it = m_list.begin();
        while (it != m_list.end())
        {
            InsertSaveDb("INSERT INTO char_all_rewards (`cid`,`reward_type`,`extra`,`itemType`,`itemId`,`fac`,`count`) VALUES ('"
                + LEX_CAST_STR(cid) + "','"
                + LEX_CAST_STR(type) + "','"
                + LEX_CAST_STR(extra) + "','"
                + LEX_CAST_STR(it->type) + "','"
                + LEX_CAST_STR(it->id) + "','"
                + LEX_CAST_STR(it->fac) + "','"
                + LEX_CAST_STR(it->nums) + "')");
            ++it;
        }
    }
}

char_rewards_mgr::char_rewards_mgr()
{
    load();
}

int char_rewards_mgr::get_type(int action_type)
{
    int type = 0;
    switch(action_type)
    {
        case top_level_event_reward_boss:
            type = rewards_type_boss;
            break;
        case top_level_event_reward_boss_kill:
            type = rewards_type_boss_kill;
            break;
        case top_level_event_reward_explore:
            type = rewards_type_explore;
            break;
        case top_level_event_reward_race:
            type = rewards_type_race;
            break;
        case top_level_event_reward_yanhui:
            type = rewards_type_yanhui;
            break;
        case top_level_event_jtz_awards:
            type = rewards_type_jtz;
            break;
        case top_level_event_jt_boss_kill:
            type = rewards_type_jt_boss;
            break;
    }
    return type;
}

int char_rewards_mgr::get_action_type(int type)
{
    int action_type = 0;
    switch(type)
    {
        case rewards_type_boss:
            action_type = top_level_event_reward_boss;
            break;
        case rewards_type_boss_kill:
            action_type = top_level_event_reward_boss_kill;
            break;
        case rewards_type_explore:
            action_type = top_level_event_reward_explore;
            break;
        case rewards_type_race:
            action_type = top_level_event_reward_race;
            break;
        case rewards_type_yanhui:
            action_type = top_level_event_reward_yanhui;
            break;
        case rewards_type_jtz:
            action_type = top_level_event_jtz_awards;
            break;
        case rewards_type_jt_boss:
            action_type = top_level_event_jt_boss_kill;
            break;
    }
    return action_type;
}

int char_rewards_mgr::get_loot_type(int type)
{
    int action_type = 0;
    switch(type)
    {
        case rewards_type_boss:
            action_type = give_boss_rank_loot;
            break;
        case rewards_type_boss_kill:
            action_type = give_boss_last_loot;
            break;
        case rewards_type_explore:
            action_type = give_explore_loot;
            break;
        case rewards_type_race:
            action_type = give_race_loot;
            break;
        case rewards_type_yanhui:
            action_type = give_yanhui_loot;
            break;
        case rewards_type_jtz:
            action_type = give_jtz_loot;
            break;
        case rewards_type_jt_boss:
            action_type = give_jt_boss;
            break;
    }
    return action_type;
}

void char_rewards_mgr::load()
{
    Query q(GetDb());
    q.get_result("SELECT cid,reward_type,extra,itemType,itemId,fac,count FROM char_all_rewards WHERE 1 order by cid,reward_type");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        int cid = q.getval();
        int type = q.getval();
        int extra = q.getval();
        boost::shared_ptr<char_rewards> pcr = getCharRewards(cid,type);
        if (pcr.get())//替换现有的
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.fac = q.getval();
            it.nums = q.getval();
            pcr->m_list.push_back(it);
        }
        else//生成当前的插入
        {
            boost::shared_ptr<CharAllRewards> pcar = getCharAllRewards(cid);
            if (!pcar.get())//先生成玩家信息
            {
                pcar.reset(new CharAllRewards);
                m_char_all_rewards[cid] = pcar;
            }
            CharAllRewards* tmp = pcar.get();
            pcr.reset(new char_rewards);
            (*tmp)[type] = pcr;
            pcr->cid = cid;
            pcr->type = type;
            pcr->extra = extra;
            pcr->state = 0;
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.fac = q.getval();
            it.nums = q.getval();
            pcr->m_list.push_back(it);
        }
    }
    q.free_result();
}

boost::shared_ptr<CharAllRewards> char_rewards_mgr::getCharAllRewards(int cid)
{
    if (m_char_all_rewards[cid].get())
    {
        return m_char_all_rewards[cid];
    }
    boost::shared_ptr<CharAllRewards> p;
    p.reset();
    return p;
}

boost::shared_ptr<char_rewards> char_rewards_mgr::getCharRewards(int cid, int type)
{
    if (m_char_all_rewards[cid].get())
    {
        CharAllRewards* tmp = m_char_all_rewards[cid].get();
        if ((*tmp)[type].get())
        {
            return (*tmp)[type];
        }
    }
    boost::shared_ptr<char_rewards> p;
    p.reset();
    return p;
}

void char_rewards_mgr::updateCharRewards(int cid, int type, int extra, std::list<Item>& getItems)
{
    boost::shared_ptr<char_rewards> pcr = getCharRewards(cid,type);
    if (pcr.get())//替换现有的
    {
        pcr->cid = cid;
        pcr->type = type;
        pcr->extra = extra;
        pcr->state = 0;
        pcr->m_list.clear();
        pcr->m_list = getItems;
        pcr->save();
    }
    else//生成当前的插入
    {
        boost::shared_ptr<CharAllRewards> pcar = getCharAllRewards(cid);
        if (!pcar.get())//先生成玩家信息
        {
            pcar.reset(new CharAllRewards);
            m_char_all_rewards[cid] = pcar;
        }
        CharAllRewards* tmp = pcar.get();
        pcr.reset(new char_rewards);
        (*tmp)[type] = pcr;
        pcr->cid = cid;
        pcr->type = type;
        pcr->extra = extra;
        pcr->state = 0;
        pcr->m_list.clear();
        pcr->m_list = getItems;
        pcr->save();
    }
    //通知按钮出现
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc)
    {
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pc->m_name);
        if (account.get())
        {
            account->Send("{\"type\":"+LEX_CAST_STR(get_action_type(type))+",\"active\":1,\"cmd\":\"addAction\",\"s\":200}");
        }
    }
}

int char_rewards_mgr::getReward(CharData& cdata, int action_type, json_spirit::Object& robj)
{
    int type = get_type(action_type);
    boost::shared_ptr<char_rewards> pcr = getCharRewards(cdata.m_id,type);
    if (pcr.get() && pcr->state == 0)
    {
        pcr->state = 1;
        std::list<Item> items = pcr->m_list;
        giveLoots(&cdata, items, cdata.m_area, cdata.m_level, 0, NULL, &robj, true, get_loot_type(type));
        pcr->save();
        //通知按钮消失
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata.m_name);
            if (account.get())
            {
                account->Send("{\"type\":"+LEX_CAST_STR(get_action_type(type))+",\"active\":1,\"cmd\":\"removeAction\",\"s\":200}");
            }
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

bool char_rewards_mgr::canGetReward(int cid, int type)
{
    boost::shared_ptr<char_rewards> pcr = getCharRewards(cid,type);
    if (pcr.get() && pcr->state == 0)
    {
        return true;
    }
    return false;
}

void char_rewards_mgr::getAction(CharData* pc, json_spirit::Array& blist)
{
    for (int i = rewards_type_start+1; i < rewards_type_end; ++i)
    {
        int action_type = get_action_type(i);
        if (pc && action_type != 0 && canGetReward(pc->m_id, i))
        {
            json_spirit::Object obj;
            obj.push_back( Pair("type", action_type) );
            obj.push_back( Pair("active", 1) );
            blist.push_back(obj);
        }
    }
}

//领取奖励 cmd ：getCharRewards
int ProcessGetCharRewards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int action_type = 0;
    READ_INT_FROM_MOBJ(action_type, o, "action_type");
    robj.push_back( Pair("action_type", action_type) );
    return Singleton<char_rewards_mgr>::Instance().getReward(*pc,action_type,robj);
}

