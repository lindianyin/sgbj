#ifndef _LOOT_H
#define _LOOT_H

#include "boost/smart_ptr/shared_ptr.hpp"
#include <list>
#include <map>
#include "base_item.h"
#include "data.h"
#include "json_spirit.h"

int getMapSilver(int mapid, int level, bool elite);    //地图id，关卡等级，是否精英

class lootGroup;

struct lootChance
{
    int chance;            //万分之几
    int lootGroupId;    //掉落之二次掉落，0表示掉物品

    int score;    //积分
    boost::shared_ptr<lootGroup> _lootGroup;
    Item item;
    lootChance()
    {
        chance = 0;
        lootGroupId = 0;
        score = 0;
        item.type = 0;
        item.id = 0;
        item.nums = 0;
    }
};

//掉落组
class lootGroup
{
public:
    lootGroup(int id);
    ~lootGroup();
    int reload();
    int getLoots(std::list<Item>& loots, int depth, int extra = 0);
    int getLoots(Item& item, int depth, int extra = 0);

    int rand_Item_list(const Item& item, std::list<Item>& items, int size);
    int Item_list(std::list<Item>& items);
    std::string getLootList(const std::string& dim);
    int updateChance(int& org);
    friend class lootMgr;
private:
    std::list<lootChance> m_all_loots;
    int _id;
};

struct multiLootChance
{
    int chance;            //万分之几

    int score;    //积分
    Item item;
    multiLootChance()
    {
        chance = 0;
        score = 0;
        item.type = 0;
        item.id = 0;
        item.nums = 0;
    }
};

//掉落组
class multiLootGroup
{
public:
    multiLootGroup(int id);
    ~multiLootGroup();
    int reload();
    int getLoots(std::list<Item>& loots, int extra = 0);

    friend class lootMgr;
private:
    std::list<multiLootChance> m_all_loots;
    int _id;
};

struct lootPlaceInfo
{
    int mapId;
    int stageId;
    int pos;
    int type;//关卡1，精英战役2
    std::string mapName;
    std::string stageName;
    std::string strongholdName;

    json_spirit::Object info;
};

class lootMgr
{
public:
    lootMgr();
    ~lootMgr();
    //生成关卡掉落 extra 额外掉落概率，只对第一个起作用
    int getStrongholdLoots(int id, std::list<Item>& items, int extra = 0);
    //精英战役掉落
    int getEliteCombatsLoots(int id, std::list<Item>& loots, int extra = 0);
    //通关奖励
    int getStageLoots(int id, std::list<Item>& loots, json_spirit::Object& robj);
    //多人副本掉落
    int getGroupCopyLoots(int id, std::list<Item>& loots, int extra = 0);
    //多人副本掉落
    std::string getGroupCopyLoots(int id, const std::string&);
    //迷宫boss掉落
    int getMazeBossLoots(int id, std::list<Item>& loots, int extra);
    //迷宫boss掉落
    int getMazeBossLootsInfo(int id, std::list<Item>& loots);
    //关卡掉落信息
    boost::shared_ptr<Item> getStrongholdLootInfo(int id);
    //精英关卡掉落信息
    int getEliteCombatsLootInfo(int id, std::list<Item>& loots);
    //生成世界掉落
    int getWorldItemFall(std::list<Item>& items);
    //生成世界掉落
    int getWorldItemFall(Item& item);
    //关卡额外掉落箱子
    int getBoxItemFall(int level, std::list<Item>& items);

    //梅花易数抽奖
    int getLotteryItem(std::list<Item>& items, int extra = 0);
    //随机显示12种东西
    int getLotteryRandItems(const Item& item, std::list<Item>& rand_items);

    //返回关卡掉落组
    boost::shared_ptr<lootGroup> getSecondLootgroup(int id);
    //返回场景宝箱列表
    void getLootList(int id, int num, json_spirit::Object& robj);

    //获取宝箱掉落
    int getBoxLoots(int id, std::list<Item>& loots, int extra);
    //获取宝箱掉落列表
    int getBoxLootsInfo(int id, std::list<Item>& items_list);

    //战神台场景掉落
    int getZSTLoots(int mapid, int stageid, int star, std::list<Item>& loots, int extra);
    //战神台场景掉落
    int getZSTLootsInfo(int mapid, int stageid, int star, std::list<Item>& loots);
    //战神台地图掉落
    int getZSTMapLoots(int mapid, std::list<Item>& loots, int extra);
    //战神台场景掉落
    int getZSTMapLootsInfo(int mapid, std::list<Item>& loots);
    
    //从db读掉落信息
    int reload(int id);

    int getLootPlace(int type, int id, json_spirit::Object& robj);
    
    static lootMgr* getInstance();

    void addLootPlace(int type, int id, int stronghold);
    void addLootPlace_E(int type, int id, int elite);

    void updaeLootPlace();
private:
    std::map<int, int> m_stronghold_limits;        //关卡次数限制
    std::map<int, boost::shared_ptr<lootGroup> > m_stronghold_loot_groups;    //关卡掉落
    std::map<int, boost::shared_ptr<lootGroup> > m_sub_loot_groups;            //掉落中的二次掉落
    std::map<int, boost::shared_ptr<lootGroup> > m_stage_loot_groups;            //通关奖励掉落
    boost::shared_ptr<lootGroup> m_world_loot;        //世界掉落
    std::map<int, boost::shared_ptr<lootGroup> > m_groupCopy_loot_groups;        //多人副本掉落
    std::map<int, boost::shared_ptr<lootGroup> > m_elitecombats_loot_groups;    //关卡掉落

    std::map<int, boost::shared_ptr<multiLootGroup> > m_maze_loot_groups;    //迷宫掉落
    std::map<int, boost::shared_ptr<multiLootGroup> > m_zst_loot_groups;    //战神台场景掉落
    std::map<int, boost::shared_ptr<multiLootGroup> > m_zst_map_loot_groups;    //战神台地图掉落

    std::map<int, boost::shared_ptr<lootPlaceInfo> > m_gem_loot_places;
    std::map<int, boost::shared_ptr<lootPlaceInfo> > m_equipment_loot_places;

    //梅花易数抽奖奖品
    boost::shared_ptr<lootGroup> m_lottery_awards;        //梅花易数奖品
    
    std::map<int, boost::shared_ptr<lootGroup> > m_box_loot_groups;    //宝箱掉落
    
    static lootMgr* m_handle;
};

#endif

