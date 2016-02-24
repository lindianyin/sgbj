#pragma once

#include <string>
#include "json_spirit.h"

/*
类型
1    银币
2    道具
3    装备
4    武将
5    阵型
6    技能
7    金币
8    地图等级银币
9    军令
10    声望
11    N×主将等级的银币
12    武将宝物
13    N×主将等级的道具
14    经验
15    宝石
16    强化石
17    礼包
18    竞技场积分
19    N×主将等级的声望
*/
enum item_type_enum
{
    item_type_silver = 1,
    item_type_treasure = 2,
    item_type_equipment = 3,
    item_type_general = 4,
    item_type_zhen = 5,
    item_type_skill = 6,
    item_type_gold = 7,
    item_type_silver_map = 8,
    item_type_ling = 9,
    item_type_prestige = 10,
    item_type_silver_level = 11,     // n*主将等级的银币
    item_type_general_baowu = 12,//英雄宝物-随机加
    item_type_treasure_level = 13,//n*主将等级的道具
    item_type_exp = 14,            //主将经验
    item_type_baoshi = 15,        //掉落宝石
    item_type_stone = 16,            //等级对应的强化石类型(类似竞技排名奖励)
    item_type_libao = 17,//礼包
    item_type_arena_score = 18,    //竞技场积分
    item_type_prestige_level = 19,//n*等级的声望
};

enum enum_treasure_type
{
    treasure_type_yushi = 9,
    treasure_type_mati_tie = 10,
    treasure_type_yinyang_fish = 11,
    treasure_type_supply = 12,
    treasure_type_silver_card = 511,
    treasure_type_gold_card = 512,
    treasure_type_jade_card = 513,
    treasure_type_prestige_card = 514,
    treasure_type_food_card = 515,
    treasure_type_honor_card = 516,
    treasure_type_box1 = 517,
    treasure_type_box2 = 518,
    treasure_type_box3 = 519,
    treasure_type_gongxun = 2001,
    treasure_type_chuanchengdan = 2002,
    treasure_type_corps_lottery = 4001,
    treasure_type_soul_type1 = 4002,//人休碎片
    treasure_type_soul_type2 = 4003,//地伤
    treasure_type_soul_type3 = 4004,//天生
    treasure_type_friend_lottery = 4005,

    treasure_type_bag_key = 4006,
    treasure_type_levy_ling = 4007,
    treasure_type_explore_ling = 4008,
    treasure_type_soul_ling = 4009,
    treasure_type_wjbs = 4010,
    treasure_type_book_refresh = 4011,

    treasure_type_general_soul = 4012,

    treasure_type_buff_bingli = 5001,
    treasure_type_buff_wugong = 5002,
    treasure_type_buff_wufang = 5003,
    treasure_type_buff_cegong = 5004,
    treasure_type_buff_cefang = 5005,
};

struct Item
{
    int type;   //掉落物品种类，1银币，2宝物 3装备 4英雄 5阵型 6技能 7金币 8地图产量倍数的银币，9军令
    int id;     //type类型中的id
    int nums;   //数量
    int fac;    //倍数
    int spic;    //图片id

    int guid;    //物品的唯一id

    std::string toString(bool withColor = false, int char_level = 0) const;
    Item()
    {
        type = 0;
        id = 0;
        nums = 0;
        fac = 0;
        guid = 0;
        spic = 0;
    }
    Item(int type_, int id_, int nums_, int fac_)
    :type(type_)
    ,id(id_)
    ,nums(nums_)
    ,fac(fac_)
    {
        guid = 0;
        spic = id;
    }
    void toObj(json_spirit::Object& obj);

    std::string name() const;
};

