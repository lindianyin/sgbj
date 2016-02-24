#ifndef _LOOT_H
#define _LOOT_H

#include "boost/smart_ptr/shared_ptr.hpp"
#include <list>
#include <map>
#include "item.h"
#include "data.h"
#include "json_spirit.h"

struct lootChance
{
    int chance;//万分之几
    int show;//是否展示
    Item item;
    lootChance()
    {
        chance = 0;
        item.type = 0;
        item.id = 0;
        item.nums = 0;
        item.extra = 0;
        show = 1;
    }
};

//掉落组
class lootGroup
{
public:
    lootGroup(int id);
    ~lootGroup();
    int reload();
    int insertLoots(std::list<Item>& loots, int idx = 0);
    int getLoots(std::list<Item>& loots, int extra_cnt = 1);
    int randomLoots(std::list<Item>& loots);
    int Item_list(std::list<Item>& items);
    friend class lootMgr;
private:
    std::list<lootChance> m_all_loots;
    int _id;
};

struct multiLootChance
{
    int chance;//万分之几
    Item item;
    multiLootChance()
    {
        chance = 0;
        item.type = 0;
        item.id = 0;
        item.nums = 0;
        item.extra = 0;
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

class lootMgr
{
public:
    lootMgr();
    ~lootMgr();
    int getLootLimit(int item_type, int item_id);
    //生成关卡掉落
    int getStrongholdLoots(int id, std::list<Item>& items);
    boost::shared_ptr<Item> getStrongholdLootInfo(int id);
    //生成副本掉落 extra 掉落次数
    int getCopyLoots(int id, std::list<Item>& loots, int extra);
    int getCopyLootInfo(int id, std::list<Item>& loots);
    //生成神灵塔掉落
    int getShenlingLoots(int id, std::list<Item>& loots, int extra);
    int getShenlingLootInfo(int id, std::list<Item>& loots);
    int getShenlingAll(int id, std::list<Item>& loots);
    //生成藏宝图掉落
    int getTreasureLoots(int id, std::list<Item>& loots);
    int getTreasureLootInfo(int id, std::list<Item>& loots);
    //生成通关奖励
    int getStageLoots(int id, std::list<Item>& loots, json_spirit::Object& robj);
    void getLootList(int id, int num, json_spirit::Object& robj);
    //在线礼包奖励
    int getOnlineLoots(int id, std::list<Item>& loots);
    int getOnlineLootInfo(int id, std::list<Item>& loots);
    //转盘活动奖励
    int getLotteryLoots(int id, std::list<Item>& loots);
    int getLotteryLootInfo(int id, std::list<Item>& loots);
private:
    std::map<std::pair<int, int>, int> m_all_loot_limit;//掉落次数限制控制
    std::map<int, boost::shared_ptr<lootGroup> > m_stronghold_loot_groups;    //关卡掉落
    std::map<int, boost::shared_ptr<lootGroup> > m_stage_loot_groups;            //通关奖励掉落
    std::map<int, boost::shared_ptr<lootGroup> > m_copy_loot_groups;    //副本掉落
    std::map<int, boost::shared_ptr<lootGroup> > m_shenling_loot_groups;    //神灵塔掉落
    std::map<int, boost::shared_ptr<lootGroup> > m_treasure_loot_groups;    //藏宝图掉落
    std::map<int, boost::shared_ptr<lootGroup> > m_online_loot_groups;    //在线礼包掉落
    std::map<int, boost::shared_ptr<lootGroup> > m_lottery_loot_groups;    //转盘活动掉落
};

#endif

