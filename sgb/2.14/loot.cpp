
#include "loot.h"
#include "utils_all.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_const.h"

#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "net.h"
#include "equipment_make.h"
#include "singleton.h"
#include "eliteCombat.h"
#include "utils_lang.h"

Database& GetDb();

lootMgr* lootMgr::m_handle = NULL;

lootMgr::lootMgr()
{
}

lootMgr::~lootMgr()
{
}

lootMgr* lootMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new lootMgr();
        m_handle->reload(0);
    }
    return m_handle;
}

//关卡掉落
int lootMgr::getStrongholdLoots(int id, std::list<Item>& loots, int extra)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_stronghold_loot_groups.find(id);
    if (it != m_stronghold_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots, 1, extra);
        }
    }
    //普通关卡无特殊掉落
    return 0;
}

//精英战役掉落
int lootMgr::getEliteCombatsLoots(int id, std::list<Item>& loots, int extra)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_elitecombats_loot_groups.find(id);
    if (it != m_elitecombats_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots, 1, extra);
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

//多人副本掉落
int lootMgr::getGroupCopyLoots(int id, std::list<Item>& loots, int extra)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_groupCopy_loot_groups.find(id);
    if (it != m_groupCopy_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots, 1, extra);
        }
    }
    return 0;
}

//多人副本掉落
std::string lootMgr::getGroupCopyLoots(int id, const std::string& dim)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_groupCopy_loot_groups.find(id);
    if (it != m_groupCopy_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLootList(dim);
        }
    }
    return 0;
}

//迷宫boss掉落
int lootMgr::getMazeBossLoots(int id, std::list<Item>& loots, int extra)
{
    std::map<int, boost::shared_ptr<multiLootGroup> >::iterator it = m_maze_loot_groups.find(id);
    if (it != m_maze_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots, extra);
        }
    }
    return 0;
}

//战神台场景掉落
int lootMgr::getZSTLoots(int mapid, int stageid, int star, std::list<Item>& loots, int extra)
{
    int idx = mapid * 10000 + stageid * 100 + star;
    std::map<int, boost::shared_ptr<multiLootGroup> >::iterator it = m_zst_loot_groups.find(idx);
    if (it != m_zst_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots, extra);
        }
    }
    return 0;
}

//战神台场景掉落
int lootMgr::getZSTLootsInfo(int mapid, int stageid, int star, std::list<Item>& loots)
{
    return getZSTLoots(mapid, stageid, star, loots, 10000);
}

//战神台地图掉落
int lootMgr::getZSTMapLoots(int mapid, std::list<Item>& loots, int extra)
{
    std::map<int, boost::shared_ptr<multiLootGroup> >::iterator it = m_zst_map_loot_groups.find(mapid);
    if (it != m_zst_map_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots, extra);
        }
    }
    return 0;
}

//战神台场景掉落
int lootMgr::getZSTMapLootsInfo(int mapid, std::list<Item>& loots)
{
    return getZSTMapLoots(mapid, loots, 10000);
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

int lootMgr::getEliteCombatsLootInfo(int id, std::list<Item>& loots)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_elitecombats_loot_groups.find(id);
    if (it != m_elitecombats_loot_groups.end())
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

//迷宫boss掉落
int lootMgr::getMazeBossLootsInfo(int id, std::list<Item>& loots)
{
    return getMazeBossLoots(id, loots, 10000);
}

//返回关卡掉落组
boost::shared_ptr<lootGroup> lootMgr::getSecondLootgroup(int id)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_sub_loot_groups.find(id);
    if (it != m_sub_loot_groups.end())
    {
        return it->second;
    }
    boost::shared_ptr<lootGroup> lg;
    return lg;
}

//世界掉落
int lootMgr::getWorldItemFall(std::list<Item>& items)
{
    if (m_world_loot.get())
    {
        return m_world_loot->getLoots(items, 1, 0);
    }
    ERR();
    return -1;
}

//世界掉落
int lootMgr::getWorldItemFall(Item& item)
{
    if (m_world_loot.get())
    {
        return m_world_loot->getLoots(item, 1, 0);
    }
    ERR();
    return -1;
}

//掉落宝箱
int lootMgr::getBoxItemFall(int level, std::list<Item>& items)
{
    int tmp = my_random(1, 1000);
    if (tmp <= 10)
    {
        Item itm;
        itm.type = item_type_gold;
        itm.id = 0;
        itm.nums = 5;
        items.push_back(itm);
    }
    else if(tmp <= 15)
    {
        Item itm;
        itm.type = item_type_gold;
        itm.id = 0;
        itm.nums = 10;
        items.push_back(itm);
    }
    else
    {
        bool b_loot = false;
        tmp = my_random(1, 100);
        std::vector<int> per;
        if (level < 30)
        {
            ;
        }
        else if(level < 50 && tmp <= 1)
        {
            b_loot = true;
            per.push_back(80);
            per.push_back(20);
            per.push_back(0);
        }
        else if(level < 70 && tmp <= 2)
        {
            b_loot = true;
            per.push_back(30);
            per.push_back(60);
            per.push_back(10);
        }
        else if(tmp <= 3)
        {
            b_loot = true;
            per.push_back(20);
            per.push_back(40);
            per.push_back(40);
        }
        if (b_loot && per.size() > 0)
        {
            boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
            int numprob[] = {treasure_type_box1,treasure_type_box2,treasure_type_box3};//箱子编号
            boost::random::discrete_distribution<> distg(per);
            Item itm;
            itm.type = item_type_treasure;
            itm.id = numprob[distg(gen)];
            itm.nums = 1;
            items.push_back(itm);
        }
    }
    return 0;
}

//梅花易数抽奖
int lootMgr::getLotteryItem(std::list<Item>& items, int extra)
{
    return m_lottery_awards.get()->getLoots(items, extra);
}

//随机显示12种东西
int lootMgr::getLotteryRandItems(const Item& item, std::list<Item>& rand_items)
{
    //随机12种掉落，包含实际抽出的掉落
    rand_items.clear();
    m_lottery_awards.get()->rand_Item_list(item, rand_items, 11);
    int rand_location = my_random(0, rand_items.size() - 1);
    std::list<Item>::iterator it = rand_items.begin();
    while (rand_location)
    {
        ++it;
        --rand_location;
    }
    rand_items.insert(it, item);
    return 0;    
}

//宝箱掉落
int lootMgr::getBoxLoots(int id, std::list<Item>& loots, int extra)
{
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_box_loot_groups.find(id);
    if (it != m_box_loot_groups.end())
    {
        if (it->second.get())
        {
            return it->second->getLoots(loots, 1, extra);
        }
    }
    return 0;
}

//宝箱掉落列表
int lootMgr::getBoxLootsInfo(int id, std::list<Item>& items_list)
{
    items_list.clear();
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_box_loot_groups.find(id);
    if (it != m_box_loot_groups.end())
    {
        if (it->second.get())
        {
            it->second->Item_list(items_list);
        }
    }
    return 0;
}

void lootMgr::addLootPlace(int type, int id, int stronghold)
{
    boost::shared_ptr<StrongholdData> sg = GeneralDataMgr::getInstance()->GetStrongholdData(stronghold);
    if (!sg.get() || !sg->m_baseStage.get() || !sg->m_baseStage->_baseMap.get())
    {
        ERR();
        if (!sg.get())
        {
            cout<<"stronghold id "<<stronghold<<",null stronghold"<<endl;
        }
        else if (!sg->m_baseStage.get())
        {
            cout<<"stronghold id "<<stronghold<<",null baseStage"<<endl;
        }
        else if (!sg->m_baseStage->_baseMap.get())
        {
            cout<<"stronghold id "<<stronghold<<",null baseStage->_baseMap"<<endl;
        }
        
        return;
    }

    //cout<<"*************** addLootPlace "<<type<<","<<id<<","<<stronghold<<endl;
    if (type == iItem_type_gem)
    {
        std::map<int, boost::shared_ptr<lootPlaceInfo> >::iterator it = m_gem_loot_places.find(id);
        if (it == m_gem_loot_places.end())
        {
            boost::shared_ptr<lootPlaceInfo> lp(new lootPlaceInfo);
            lp->type = 1;
            lp->mapId = sg->m_map_id;
            lp->stageId = sg->m_stage_id;
            lp->pos = sg->m_strongholdpos;
            lp->mapName = sg->m_baseStage->_baseMap->name;
            lp->stageName = sg->m_baseStage->name;
            lp->strongholdName = sg->m_name;

            lp->info.push_back( Pair("type", lp->type) );
            lp->info.push_back( Pair("mapId", lp->mapId) );
            lp->info.push_back( Pair("stageId", lp->stageId) );
            lp->info.push_back( Pair("pos", lp->pos) );
            lp->info.push_back( Pair("map", lp->mapName) );
            lp->info.push_back( Pair("stage", lp->stageName) );
            lp->info.push_back( Pair("stronghold", lp->strongholdName) );
            m_gem_loot_places[id] = lp;
        }

        boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(id);
        if (!bt.get())
        {
            return;
        }

        if (!bt->m_place.get())
        {
            bt->m_place = m_gem_loot_places[id];
        }
        //装备卷轴
        if (bt->usage == 8)
        {
            //卷轴的掉落等同于装备的掉落
            equipment_scroll* sp = Singleton<equipment_scroll_mgr>::Instance().getScroll(bt->value);
            if (!sp)
            {
                return;
            }
            id = sp->m_equipment->baseid;
            type = iItem_type_equipment;
        }
        else
        {
            return;
        }
    }
    if (type == iItem_type_equipment)
    {
        std::map<int, boost::shared_ptr<lootPlaceInfo> >::iterator it = m_equipment_loot_places.find(id);
        if (it == m_equipment_loot_places.end())
        {
            boost::shared_ptr<lootPlaceInfo> lp(new lootPlaceInfo);
            lp->type = 1;
            lp->mapId = sg->m_map_id;
            lp->stageId = sg->m_stage_id;
            lp->pos = sg->m_strongholdpos;
            lp->mapName = sg->m_baseStage->_baseMap->name;
            lp->stageName = sg->m_baseStage->name;
            lp->strongholdName = sg->m_name;

            lp->info.push_back( Pair("type", lp->type) );
            lp->info.push_back( Pair("mapId", lp->mapId) );
            lp->info.push_back( Pair("stageId", lp->stageId) );
            lp->info.push_back( Pair("pos", lp->pos) );
            lp->info.push_back( Pair("map", lp->mapName) );
            lp->info.push_back( Pair("stage", lp->stageName) );
            lp->info.push_back( Pair("stronghold", lp->strongholdName) );

            m_equipment_loot_places[id] = lp;
        }

        boost::shared_ptr<baseEquipment> bt = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
        if (bt.get() && !bt->m_place.get())
        {
            bt->m_place = m_equipment_loot_places[id];
        }
    }
}

void lootMgr::addLootPlace_E(int type, int id, int elite)
{
    boost::shared_ptr<eliteCombat> ec = eliteCombatMgr::getInstance()->getEliteCombatById(elite);
    if (!ec.get())
    {
        return;
    }
    if (type == iItem_type_gem)
    {
        std::map<int, boost::shared_ptr<lootPlaceInfo> >::iterator it = m_gem_loot_places.find(id);
        if (it == m_gem_loot_places.end())
        {
            boost::shared_ptr<lootPlaceInfo> lp(new lootPlaceInfo);
            lp->type = 2;
            lp->mapId = ec->_mapid;
            lp->pos = ec->_id;
            boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(ec->_mapid);
            if (bm.get())
            {
                lp->mapName = bm->name;
            }
            lp->strongholdName = ec->_name;

            lp->info.push_back( Pair("type", lp->type) );
            lp->info.push_back( Pair("mapId", lp->mapId) );
            lp->info.push_back( Pair("pos", lp->pos) );
            lp->info.push_back( Pair("map", lp->mapName) );
            lp->info.push_back( Pair("stronghold", lp->strongholdName) );
            m_gem_loot_places[id] = lp;
        }

        boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(id);
        if (!bt.get())
        {
            return;
        }

        if (!bt->m_place.get())
        {
            bt->m_place = m_gem_loot_places[id];
        }
        //装备卷轴
        if (bt->usage == 8)
        {
            //卷轴的掉落等同于装备的掉落
            equipment_scroll* sp = Singleton<equipment_scroll_mgr>::Instance().getScroll(bt->value);
            if (!sp)
            {
                return;
            }
            id = sp->m_equipment->baseid;
            type = iItem_type_equipment;
        }
        else
        {
            return;
        }
    }
    if (type == iItem_type_equipment)
    {
        std::map<int, boost::shared_ptr<lootPlaceInfo> >::iterator it = m_equipment_loot_places.find(id);
        if (it == m_equipment_loot_places.end())
        {
            boost::shared_ptr<lootPlaceInfo> lp(new lootPlaceInfo);
            lp->type = 2;
            lp->mapId = ec->_mapid;
            lp->pos = ec->_id;
            boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(ec->_mapid);
            if (bm.get())
            {
                lp->mapName = bm->name;
            }
            lp->strongholdName = ec->_name;

            lp->info.push_back( Pair("type", lp->type) );
            lp->info.push_back( Pair("mapId", lp->mapId) );
            lp->info.push_back( Pair("pos", lp->pos) );
            lp->info.push_back( Pair("map", lp->mapName) );
            lp->info.push_back( Pair("stronghold", lp->strongholdName) );

            m_equipment_loot_places[id] = lp;
        }

        boost::shared_ptr<baseEquipment> bt = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
        if (bt.get() && !bt->m_place.get())
        {
            bt->m_place = m_equipment_loot_places[id];
        }
    }
}


//查询掉落地点
int lootMgr::getLootPlace(int type, int id, json_spirit::Object& robj)
{
    boost::shared_ptr<lootPlaceInfo> lp;
    if (type == iItem_type_gem)
    {
        std::map<int, boost::shared_ptr<lootPlaceInfo> >::iterator it = m_gem_loot_places.find(id);
        if (it != m_gem_loot_places.end())
        {
            lp = it->second;
        }
    }
    else if (type == item_type_equipment)
    {
        std::map<int, boost::shared_ptr<lootPlaceInfo> >::iterator it = m_equipment_loot_places.find(id);
        if (it != m_gem_loot_places.end())
        {
            lp = it->second;
        }
    }

    if (!lp.get())
    {
        ERR();
        cout<<"type:"<<type<<",id:"<<id<<endl;
        return HC_ERROR;
    }

    robj.push_back( Pair("info", lp->info) );
    return HC_SUCCESS;
}

void lootMgr::updaeLootPlace()
{
    for (std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_stronghold_loot_groups.begin(); it != m_stronghold_loot_groups.end(); ++it)
    {
        if (it->second.get())
        {
            for (std::list<lootChance>::iterator it2 = it->second->m_all_loots.begin(); it2 != it->second->m_all_loots.end(); ++it2)
            {
                addLootPlace(it2->item.type, it2->item.id, it->first);
            }
        }
    }
    for (std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_elitecombats_loot_groups.begin(); it != m_elitecombats_loot_groups.end(); ++it)
    {
        if (it->second.get())
        {
            for (std::list<lootChance>::iterator it2 = it->second->m_all_loots.begin(); it2 != it->second->m_all_loots.end(); ++it2)
            {
                addLootPlace_E(it2->item.type, it2->item.id, it->first);
            }
        }
    }
}

int lootMgr::reload(int id)
{
    Query q(GetDb());
    //二次掉落
    q.get_result("select id,chance,loopGroup2,itemType,itemId,counts from base_loots where type=2 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int groupId = q.getval();
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_sub_loot_groups.find(groupId);
        if (it != m_sub_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(groupId));
            m_sub_loot_groups[groupId] = lootG;
        }
        lootChance lc;
        lc.chance = (int)(q.getnum() * 10000.0);
        lc.lootGroupId = q.getval();
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_sub_loot_groups.begin();
    while (it != m_sub_loot_groups.end())
    {
        if (it->second.get())
        {
            std::list<lootChance>::iterator it2 = it->second->m_all_loots.begin();
            while (it2 != it->second->m_all_loots.end())
            {
                if (it2->lootGroupId > 0)
                {
                    it2->_lootGroup = getSecondLootgroup(it2->lootGroupId);
                }
                ++it2;
            }
        }
        ++it;
    }

    m_world_loot.reset(new lootGroup(1));
    //世界掉落
    q.get_result("select chance,loopGroup2,itemType,itemId,counts from base_loots where type=1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        lootChance lc;
        lc.chance = (int)(q.getnum() * 10000.0);
        lc.lootGroupId = q.getval();
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        if (lc.lootGroupId > 0)
        {
            lc._lootGroup = getSecondLootgroup(lc.lootGroupId);
        }
        m_world_loot->m_all_loots.push_back(lc);
    }
    q.free_result();

    //关卡掉落
    q.get_result("select id,itemType,itemId,counts,chance from base_stronghold_loots where 1");
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
        lc.lootGroupId = 0;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.chance = (int)(q.getnum() * 10000.0);

        //增加到掉落信息中
        //addLootPlace(lc.item.type, lc.item.id, id);

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();

    //精英战役掉落
    q.get_result("select id,itemType,itemId,counts,chance from base_elite_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_elitecombats_loot_groups.find(id);
        if (it != m_elitecombats_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(id));
            m_elitecombats_loot_groups[id] = lootG;
        }
        lootChance lc;
        lc.lootGroupId = 0;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.chance = (int)(q.getnum() * 10000.0);

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();

    for (std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_elitecombats_loot_groups.begin(); it != m_elitecombats_loot_groups.end(); ++it)
    {
        int org = 0;
        int ret = it->second->updateChance(org);
        if (org != 10000)
        {
            if (ret == 10000)
            {
                //cout<<"elite id"<<it->first<<", loot chance reset "<<org<<"-> 10000"<<endl;
            }
            else
            {
                cout<<"!!!!!!!!! elite id"<<it->first<<", loot chance error "<<org<<"->"<<ret<<endl;
                assert(false);
            }
        }
    }

    //通关奖励掉落
    q.get_result("select id,mapid,num,itemType,itemId,counts,num from base_stage_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int stageid = q.getval();
        int mapid = q.getval();
        int _id = (mapid - 1) * 3 + stageid;//转化场景id为唯一值
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
        lc.lootGroupId = 0;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();

    //多人副本掉落
    q.get_result("select copy_id,itemType,itemId,counts,chance from base_group_copy_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_groupCopy_loot_groups.find(id);
        if (it != m_groupCopy_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(id));
            m_groupCopy_loot_groups[id] = lootG;
        }
        lootChance lc;
        lc.lootGroupId = 0;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.chance = (int)(q.getnum() * 10000.0);

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();

    m_lottery_awards.reset(new lootGroup(1));
    //梅花易数奖品
    q.get_result("select chance,itemType,itemId,counts,score,fac from base_lottery_awards where type=1 and group_id=1");
    while (q.fetch_row())
    {
        lootChance lc;
        lc.lootGroupId = 0;
        lc.chance = (int)(q.getnum() * 10000.0);
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.score = q.getval();
        lc.item.fac = q.getval();
        m_lottery_awards->m_all_loots.push_back(lc);
    }
    q.free_result();

    //迷宫boss掉落
    q.get_result("select bid,type,id,nums,gailv from base_maze_boss_loots where 1 order by bid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<multiLootGroup> lootG;
        std::map<int, boost::shared_ptr<multiLootGroup> >::iterator it = m_maze_loot_groups.find(id);
        if (it != m_maze_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new multiLootGroup(id));
            m_maze_loot_groups[id] = lootG;
        }
        multiLootChance lc;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.chance = 100 * q.getval();    //万分之几

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    
    //宝箱掉落
    q.get_result("select id,itemType,itemId,counts,chance,notify from base_box_loots where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<lootGroup> lootG;
        std::map<int, boost::shared_ptr<lootGroup> >::iterator it = m_box_loot_groups.find(id);
        if (it != m_box_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new lootGroup(id));
            m_box_loot_groups[id] = lootG;
        }
        lootChance lc;
        lc.lootGroupId = 0;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.chance = (int)(q.getnum() * 10000.0);
        lc.score = q.getval();

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    
    //战神台场景掉落
    q.get_result("select mapid,stageid,star,type,id,nums,gailv from base_zst_stage_loots where 1 order by stageid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        int stageid = q.getval();
        int star = q.getval();
        int idx = mapid * 10000 + stageid * 100 + star;
        boost::shared_ptr<multiLootGroup> lootG;
        std::map<int, boost::shared_ptr<multiLootGroup> >::iterator it = m_zst_loot_groups.find(idx);
        if (it != m_zst_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new multiLootGroup(idx));
            m_zst_loot_groups[idx] = lootG;
        }
        multiLootChance lc;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.chance = 100 * q.getval();    //万分之几

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    //战神台地图掉落
    q.get_result("select mapid,type,id,nums,gailv from base_zst_map_loots where 1 order by mapid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        boost::shared_ptr<multiLootGroup> lootG;
        std::map<int, boost::shared_ptr<multiLootGroup> >::iterator it = m_zst_map_loot_groups.find(mapid);
        if (it != m_zst_map_loot_groups.end())
        {
            lootG = it->second;
        }
        else
        {
            lootG.reset(new multiLootGroup(mapid));
            m_zst_map_loot_groups[mapid] = lootG;
        }
        multiLootChance lc;
        lc.item.type = q.getval();
        lc.item.id = q.getval();
        lc.item.nums = q.getval();
        lc.chance = 100 * q.getval();    //万分之几

        lootG->m_all_loots.push_back(lc);
    }
    q.free_result();
    return 0;
}

void ItemToObj(Item* sitem, boost::shared_ptr<json_spirit::Object>& sgetobj);

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
                    if (it_l->lootGroupId == 0)
                    {
                        json_spirit::Object obj;
                        boost::shared_ptr<json_spirit::Object> p_obj;
                        ItemToObj(&(it_l->item), p_obj);
                        if (p_obj.get())
                        {
                            list.push_back(*(p_obj.get()));
                        }
                        //cout << "type=" << it_l->item.type << " is push" << endl;
                    }
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

//生成掉落 extra 额外的概率
int lootGroup::getLoots(std::list<Item>& loots, int depth, int extra)
{
    //防止配置错误循环嵌套了
    if (depth > 3)
    {
        return -1;
    }
    int role = my_random(1, 10000);
    int cur_role = 0;
    std::list<lootChance>::iterator it = m_all_loots.begin();
    while (it != m_all_loots.end())
    {
        cur_role += it->chance;
        //掉落这个
        if (role <= (cur_role + extra))
        {
            if (it->lootGroupId == 0)
            {
                loots.push_back(it->item);
                return it->score;
            }
            else
            {
                boost::shared_ptr<lootGroup> lootG = lootMgr::getInstance()->getSecondLootgroup(it->lootGroupId);
                if (lootG.get())
                {
                    return lootG->getLoots(loots, depth+1);
                }
                else
                {
                    return 0;
                }
            }
        }
        ++it;
    }
    return -1;
}


//生成掉落 extra 额外的概率
int lootGroup::getLoots(Item& item, int depth, int extra)
{
    //防止配置错误循环嵌套了
    if (depth > 3)
    {
        return -1;
    }
    int role = my_random(1, 10000);
    int cur_role = 0;
    std::list<lootChance>::iterator it = m_all_loots.begin();
    while (it != m_all_loots.end())
    {
        cur_role += it->chance;
        //掉落这个
        if (role <= (cur_role + extra))
        {
            if (it->lootGroupId == 0)
            {
                item.id = it->item.id;
                item.fac = it->item.fac;
                item.nums = it->item.nums;
                item.type = it->item.type;
                return it->score;
            }
            else
            {
                boost::shared_ptr<lootGroup> lootG = lootMgr::getInstance()->getSecondLootgroup(it->lootGroupId);
                if (lootG.get())
                {
                    return lootG->getLoots(item, depth+1);
                }
                else
                {
                    return 0;
                }
            }
        }
        ++it;
    }
    return -1;
}


int lootGroup::rand_Item_list(const Item& item, std::list<Item>& items, int size)
{
    std::list<lootChance> temp = m_all_loots;
    std::list<lootChance>::iterator it = temp.begin();
    int randx = my_random(0, temp.size() - 1);
    while (randx)
    {
        --randx;
        ++it;
    }
    while (temp.size() && (int)items.size() < size)
    {
        if (it->lootGroupId == 0
            && (it->item.type != item.type
            || it->item.id != item.id
            || it->item.nums != item.nums))
        {
            items.push_back(it->item);
        }
        temp.erase(it);
        it = temp.begin();
        int rand = my_random(0, temp.size() - 1);
        while (rand)
        {
            ++it;
            --rand;
        }
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

std::string lootGroup::getLootList(const std::string& dim)
{
    std::string strLoots = "";
    std::list<lootChance>::iterator it = m_all_loots.begin();
    while (it != m_all_loots.end())
    {
        if (it->lootGroupId == 0)
        {
            if (strLoots != "")
            {
                strLoots += dim;
            }
            if (it->item.type == item_type_treasure)
            {
                boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(it->item.id);
                if (tr.get())
                {
                    strLoots += tr->name;
                    strLoots += (strCounts + LEX_CAST_STR(it->item.nums));
                }
            }
            else
            {
                strLoots += it->item.toString();
            }
        }
        ++it;
    }
    return strLoots;
}

int lootGroup::updateChance(int& org)
{
    int total = 0;
    std::list<lootChance>::iterator it = m_all_loots.begin();
    while (it != m_all_loots.end())
    {
        lootChance& ltc = *it;
        total += ltc.chance;
        ++it;
    }

    org = total;
    if (total != 10000)
    {
        int total_now = 0;
        it = m_all_loots.begin();
        while (it != m_all_loots.end())
        {
            lootChance& ltc = *it;
            ltc.chance = (ltc.chance * 10000) / total;

            ++it;
            if (it == m_all_loots.end() && (ltc.chance + total_now) < 10000)
            {
                ltc.chance = 10000 - total_now;
                break;
            }
            else
            {
                total_now += ltc.chance;
            }
        }

        //这里计算下，判断上面的算法有无错误
        total_now = 0;
        it = m_all_loots.begin();
        while (it != m_all_loots.end())
        {
            lootChance& ltc = *it;
            total_now += ltc.chance;
            ++it;
        }
        return total_now;
    }
    else
    {
        return 0;
    }
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
    int score = 0;
    std::list<multiLootChance>::iterator it = m_all_loots.begin();
    while (it != m_all_loots.end())
    {
        //掉落这个
        if (my_random(1, 10000) <= (it->chance + extra))
        {
            loots.push_back(it->item);
            score += it->score;            
        }
        ++it;
    }
    return score;
}

//获得地图id对应的银币产量
int getMapSilver(int mapid, int level, bool elite)    //地图id，关卡等级，是否精英
{
    if (mapid > max_map_id)
    {
        mapid = max_map_id;
    }
    else if (mapid < 1)
    {
        mapid = 1;
    }
    if (elite)
    {
        return map_elite_silver_field[mapid-1]*level;
    }
    else
    {
        return map_normal_silver_field[mapid-1]*level;
    }
}

/**查询掉落信息**/
int ProcessQueryDLPlace(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int type = 0, id = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    READ_INT_FROM_MOBJ(id,o,"id");
    return lootMgr::getInstance()->getLootPlace(type, id, robj);
}

