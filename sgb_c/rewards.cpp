#include "rewards.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "singleton.h"

Database& GetDb();

extern void InsertSaveDb(const std::string& sql);

void char_rewards::save()
{
    InsertSaveDb("delete from char_all_rewards where cid="+LEX_CAST_STR(cid)+" and reward_type="+LEX_CAST_STR(type));
    if (state == 0)
    {
        std::list<Item>::iterator it = m_list.begin();
        while (it != m_list.end())
        {
            InsertSaveDb("INSERT INTO char_all_rewards (`cid`,`reward_type`,`extra`,`itemType`,`itemId`,`itemExtra`,`count`) VALUES ('"
                + LEX_CAST_STR(cid) + "','"
                + LEX_CAST_STR(type) + "','"
                + LEX_CAST_STR(extra) + "','"
                + LEX_CAST_STR(it->type) + "','"
                + LEX_CAST_STR(it->id) + "','"
                + LEX_CAST_STR(it->extra) + "','"
                + LEX_CAST_STR(it->nums) + "')");
            ++it;
        }
    }
}

rewardsMgr::rewardsMgr()
{
    Query q(GetDb());
    q.get_result("SELECT cid,reward_type,extra,itemType,itemId,itemExtra,count FROM char_all_rewards WHERE 1 order by cid,reward_type");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        int cid = q.getval();
        int type = q.getval();
        int extra = q.getval();
        boost::shared_ptr<char_rewards> pcr = getCharRewards(cid,type);
        if (pcr.get())
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.extra = q.getval();
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
            it.extra = q.getval();
            it.nums = q.getval();
            pcr->m_list.push_back(it);
        }
    }
    q.free_result();
}

boost::shared_ptr<CharAllRewards> rewardsMgr::getCharAllRewards(int cid)
{
    if (m_char_all_rewards[cid].get())
    {
        return m_char_all_rewards[cid];
    }
    boost::shared_ptr<CharAllRewards> p;
    p.reset();
    return p;
}

boost::shared_ptr<char_rewards> rewardsMgr::getCharRewards(int cid, int type)
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

void rewardsMgr::updateCharRewards(int cid, int type, int extra, std::list<Item>& getItems)
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
        //通知按钮出现
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc)
        {
            pc->addTopButton(top_button_reward_start+type,1);
        }
    }
}

int rewardsMgr::getReward(CharData& cdata, int type, json_spirit::Object& robj)
{
    boost::shared_ptr<char_rewards> pcr = getCharRewards(cdata.m_id,type);
    if (pcr.get() && pcr->state == 0)
    {
        pcr->state = 1;
        std::list<Item> items = pcr->m_list;
        int statistics_type = 0;
        if (type == REWARDS_TYPE_ARENA)
            statistics_type = loot_arena_rank;
        else if(type == REWARDS_TYPE_PK)
            statistics_type = loot_pk_rank;
        else if(type == REWARDS_TYPE_BOSS || type == REWARDS_TYPE_BOSS_KILL)
            statistics_type = loot_boss;
        else if(type == REWARDS_TYPE_WEEK_RANKING)
            statistics_type = loot_weekRanking;
        giveLoots(&cdata, items, NULL, &robj, true, statistics_type);
        pcr->save();
        cdata.removeTopButton(top_button_reward_start+type);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

bool rewardsMgr::canGetReward(int cid, int type)
{
    boost::shared_ptr<char_rewards> pcr = getCharRewards(cid,type);
    if (pcr.get() && pcr->state == 0)
    {
        return true;
    }
    return false;
}

void rewardsMgr::getButton(CharData* pc, json_spirit::Array& list)
{
    for (int i = REWARDS_TYPE_START+1; i < REWARDS_TYPE_END; ++i)
    {
        if (pc && canGetReward(pc->m_id, i))
        {
            json_spirit::Object obj;
            obj.push_back( Pair("type", top_button_reward_start + i) );
            obj.push_back( Pair("active", 1) );
            list.push_back(obj);
        }
    }
    return;
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
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("type", type) );
    return Singleton<rewardsMgr>::Instance().getReward(*pc,type,robj);
}

