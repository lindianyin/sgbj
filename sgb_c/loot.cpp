
#include "loot.h"
#include "utils_all.h"
#include "errcode_def.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "const_def.h"

#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "net.h"
#include "singleton.h"
#include "utils_lang.h"

Database& GetDb();

lootMgr::lootMgr()
{
    Query q(GetDb());
    //掉落限制
    q.get_result("select type,id,value from base_loot_times where 1");
    while (q.fetch_row())
    {
        int type = q.getval();
        int id = q.getval();
        int value = q.getval();
        m_all_loot_limit.insert(std::make_pair(std::make_pair(type,id),value));
    }
    q.free_result();
    //关卡掉落
    q.get_result("select id,itemType,itemId,counts,extra,chance from base_stronghold_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_stronghold_loot_groups.find(id);
        if (it != m_stronghold_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(id));
            m_stronghold_loot_groups[id] = lootG;
        }
        lootChance lc;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.item.extra = q.getval();
        lc.chance = (int)(q.getnum() * 10000.0);

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    //通关奖励掉落
    q.get_result("select id,mapid,num,itemType,itemId,counts,extra from base_stage_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int stageid = q.getval();
        int mapid = q.getval();
        int _id = stageIndex(mapid,stageid);//转化场景id为唯一值
        int num = q.getval();//箱子编号
        int id = _id * 10 + num;
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_stage_loot_groups.find(id);
        if (it != m_stage_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(id));
            m_stage_loot_groups[id] = lootG;
        }
        lootChance lc;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.item.extra = q.getval();

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    //副本掉落
    q.get_result("select id,itemType,itemId,counts,extra,chance from base_copy_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_copy_loot_groups.find(id);
        if (it != m_copy_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(id));
            m_copy_loot_groups[id] = lootG;
        }
        lootChance lc;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.item.extra = q.getval();
        lc.chance = (int)(q.getnum() * 10000.0);

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    //神灵塔掉落
    q.get_result("select id,itemType,itemId,counts,extra,chance,beshow from base_shenling_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_shenling_loot_groups.find(id);
        if (it != m_shenling_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(id));
            m_shenling_loot_groups[id] = lootG;
        }
        lootChance lc;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.item.extra = q.getval();
        lc.chance = (int)(q.getnum() * 10000.0);
        lc.show = q.getval();

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    //藏宝图掉落
    q.get_result("select id,itemType,itemId,counts,extra,chance from base_treasure_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_treasure_loot_groups.find(id);
        if (it != m_treasure_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(id));
            m_treasure_loot_groups[id] = lootG;
        }
        lootChance lc;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.item.extra = q.getval();
        lc.chance = (int)(q.getnum() * 10000.0);
        lc.show = q.getval();

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    //在线礼包掉落
    q.get_result("select id,itemType,itemId,counts,extra,chance from base_online_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_online_loot_groups.find(id);
        if (it != m_online_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(id));
            m_online_loot_groups[id] = lootG;
        }
        lootChance lc;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.item.extra = q.getval();
        lc.chance = (int)(q.getnum() * 10000.0);

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    //转盘掉落
    q.get_result("select id,itemType,itemId,counts,extra,chance from base_lottery_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_lottery_loot_groups.find(id);
        if (it != m_lottery_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(id));
            m_lottery_loot_groups[id] = lootG;
        }
        lootChance lc;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.item.extra = q.getval();
        lc.chance = (int)(q.getnum() * 10000.0);

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
}

lootMgr::~lootMgr()
{
}

int lootMgr::getLootLimit(int item_type, int item_id)
{
    return m_all_loot_limit[std::make_pair(item_type, item_id)];
}

//关卡掉落
int lootMgr::getStrongholdLoots(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_stronghold_loot_groups.find(id);
    if (it != m_stronghold_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->randomLoots(loots);
        }
    }
    return 0;
}

boost::shared_ptr<Item> lootMgr::getStrongholdLootInfo(int id)
{
    boost::shared_ptr<Item> p_result;
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_stronghold_loot_groups.find(id);
    if (it != m_stronghold_loot_groups.end())
    {
        if (it->second.get())
        {
            std::list<lootChance>::iterator it_l = it->second->m_all_loots.begin();
            if (it_l != it->second->m_all_loots.end())
            {
                p_result.reset(&(it_l->item));
                return p_result;
            }
        }
    }
    p_result.reset();
    return p_result;
}

//副本掉落
int lootMgr::getCopyLoots(int id, std::list<Item>& loots, int extra)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_copy_loot_groups.find(id);
    if (it != m_copy_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots, extra);
        }
    }
    return 0;
}

int lootMgr::getCopyLootInfo(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_copy_loot_groups.find(id);
    if (it != m_copy_loot_groups.end())
    {
        if (it->second.get())
        {
            std::list<lootChance>::iterator it_l = it->second->m_all_loots.begin();
            while (it_l != it->second->m_all_loots.end())
            {
                loots.push_back(it_l->item);
                ++it_l;
            }
        }
    }
    return 0;
}

//神灵塔掉落
int lootMgr::getShenlingLoots(int id, std::list<Item>& loots, int extra)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_shenling_loot_groups.find(id);
    if (it != m_shenling_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots, extra);
        }
    }
    return 0;
}

int lootMgr::getShenlingLootInfo(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_shenling_loot_groups.find(id);
    if (it != m_shenling_loot_groups.end())
    {
        if (it->second.get())
        {
            std::list<lootChance>::iterator it_l = it->second->m_all_loots.begin();
            while (it_l != it->second->m_all_loots.end())
            {
                if (it_l->show == 1)
                {
                    loots.push_back(it_l->item);
                }
                ++it_l;
            }
        }
    }
    return 0;
}

int lootMgr::getShenlingAll(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_shenling_loot_groups.find(id);
    if (it != m_shenling_loot_groups.end())
    {
        if (it->second.get())
        {
            std::list<lootChance>::iterator it_l = it->second->m_all_loots.begin();
            while (it_l != it->second->m_all_loots.end())
            {
                loots.push_back(it_l->item);
                ++it_l;
            }
        }
    }
    return 0;
}

//藏宝图掉落
int lootMgr::getTreasureLoots(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_treasure_loot_groups.find(id);
    if (it != m_treasure_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots, 1);
        }
    }
    return 0;
}

int lootMgr::getTreasureLootInfo(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_treasure_loot_groups.find(id);
    if (it != m_treasure_loot_groups.end())
    {
        if (it->second.get())
        {
            std::list<lootChance>::iterator it_l = it->second->m_all_loots.begin();
            while (it_l != it->second->m_all_loots.end())
            {
                loots.push_back(it_l->item);
                ++it_l;
            }
        }
    }
    return 0;
}

//通关奖励掉落
int lootMgr::getStageLoots(int id, std::list<Item>& loots, json_spirit::Object& robj)
{
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    int nummap[] = {30,30,30};
    int numprob[] = {1,2,3};//箱子编号
    boost::random::discrete_distribution<> distg(nummap);
    int num = numprob[distg(gen)];
    //cout << num << " is get" << endl;
    int result_id = id * 10 + num;
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_stage_loot_groups.find(result_id);
    if (it != m_stage_loot_groups.end())
    {
        if (it->second.get())
        {
            std::list<lootChance> temp = it->second->m_all_loots;
            std::list<lootChance>::iterator it_l = temp.begin();
            while (it_l != temp.end())
            {
                loots.push_back(it_l->item);
                ++it_l;
            }
            getLootList(id,num,robj);
        }
    }
    return 0;
}

//在线礼包掉落
int lootMgr::getOnlineLoots(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_online_loot_groups.find(id);
    if (it != m_online_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots);
        }
    }
    return 0;
}

int lootMgr::getOnlineLootInfo(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_online_loot_groups.find(id);
    if (it != m_online_loot_groups.end())
    {
        if (it->second.get())
        {
            std::list<lootChance>::iterator it_l = it->second->m_all_loots.begin();
            while (it_l != it->second->m_all_loots.end())
            {
                loots.push_back(it_l->item);
                ++it_l;
            }
        }
    }
    return 0;
}

//转盘活动掉落
int lootMgr::getLotteryLoots(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_lottery_loot_groups.find(id);
    if (it != m_lottery_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots);
        }
    }
    return 0;
}

int lootMgr::getLotteryLootInfo(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_lottery_loot_groups.find(id);
    if (it != m_lottery_loot_groups.end())
    {
        if (it->second.get())
        {
            std::list<lootChance>::iterator it_l = it->second->m_all_loots.begin();
            while (it_l != it->second->m_all_loots.end())
            {
                loots.push_back(it_l->item);
                ++it_l;
            }
        }
    }
    return 0;
}

void lootMgr::getLootList(int id, int num, json_spirit::Object& robj)
{
    for (int i = 1; i <= 3; ++i)
    {
        //cout << num << " is get" << endl;
        int _id = id * 10 + i;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_stage_loot_groups.find(_id);
        if (it != m_stage_loot_groups.end())
        {
            if (it->second.get())
            {
                std::list<lootChance> temp = it->second->m_all_loots;
                //cout << "get boxid=" << _id << " LootList" << endl;
                json_spirit::Object box;
                json_spirit::Array list;
                std::list<lootChance>::iterator it_l = temp.begin();
                while (it_l != temp.end())
                {
                    json_spirit::Object obj;
                    it_l->item.toObj(obj);
                    list.push_back(obj);
                    ++it_l;
                }
                box.push_back( Pair("loot_list", list) );
                if (i == num)
                {
                    box.push_back( Pair("get", true) );
                }
                else
                {
                    box.push_back( Pair("get", false) );
                }
                robj.push_back( Pair("box"+LEX_CAST_STR(i), box) );
            }
        }
    }
    return;
}

lootGroup::lootGroup(int id)
:_id(id)
{
}

lootGroup::~lootGroup()
{
}

int lootGroup::reload()
{
    return 0;
}

int lootGroup::insertLoots(std::list<Item>& loots, int idx)
{
    std::list<lootChance>::iterator it = m_all_loots.begin();
    while (it != m_all_loots.end())
    {
        //掉落这个
        if (idx <= 0)
        {
            loots.push_back(it->item);
            return 0;
        }
        --idx;
        ++it;
    }
    return -1;
}

//根据组合概率生成extra个掉落(必掉)
int lootGroup::getLoots(std::list<Item>& loots, int extra_cnt)
{
    int get_cnt = 0;
    std::vector<int> gailv;
    std::list<lootChance>::iterator it = m_all_loots.begin();
    while (it != m_all_loots.end())
    {
        gailv.push_back(it->chance);
        ++it;
    }
    //生成extra个掉落
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    for (int i = 0; i < extra_cnt; ++i)
    {
        boost::random::discrete_distribution<> dist(gailv);
        int idx = dist(gen);
        //插入idx到实际掉落并且不再随机该项
        if (insertLoots(loots, idx) == 0)
        {
            ++get_cnt;
        }
        gailv[idx] = 0;
    }
    return get_cnt;
}

//根据总概率随机掉落(可能不掉)
int lootGroup::randomLoots(std::list<Item>& loots)
{
    int role = my_random(1, 10000);
    int cur_role = 0;
    std::list<lootChance>::iterator it = m_all_loots.begin();
    while (it != m_all_loots.end())
    {
        cur_role += it->chance;
        //掉落这个
        if (role <= cur_role)
        {
            loots.push_back(it->item);
            return 1;
        }
        ++it;
    }
    return 0;
}

int lootGroup::Item_list(std::list<Item>& items)
{
    std::list<lootChance> temp = m_all_loots;
    std::list<lootChance>::iterator it = temp.begin();
    while (it != temp.end())
    {
        items.push_back(it->item);
        ++it;
    }
    return 0;
}

multiLootGroup::multiLootGroup(int id)
:_id(id)
{
}

multiLootGroup::~multiLootGroup()
{
}

int multiLootGroup::reload()
{
    return 0;
}

//生成掉落 extra 额外的概率
int multiLootGroup::getLoots(std::list<Item>& loots, int extra)
{
    std::list<multiLootChance>::iterator it = m_all_loots.begin();
    while (it != m_all_loots.end())
    {
        //掉落这个
        if (my_random(1, 10000) <= (it->chance + extra))
        {
            loots.push_back(it->item);
        }
        ++it;
    }
    return 0;
}

