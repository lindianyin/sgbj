#pragma once

#include <string>
#include "data.h"

/*
掉落类型
*/
enum statistics_loot_type
{
    loot_init = 0,
    loot_sign_action = 1,               //签到奖励
    loot_limit_action = 2,              //限时活动
    loot_arena = 3,                     //竞技场战斗
    loot_arena_shop = 4,                //竞技场商店
    loot_copy = 5,                      //副本战斗
    loot_copy_shop = 6,                 //副本商店
    loot_daily_score = 7,               //每日必做
    loot_prestige_shop = 8,             //声望商店
    loot_explore = 9,                   //探索洞穴
    loot_guild_box = 10,                //公会宝箱
    loot_libao = 11,                    //礼包
    loot_treasure = 12,                 //藏宝图
    loot_stronghold = 13,               //关卡战斗
    loot_stage = 14,                    //通关奖励
    loot_recharge_event = 15,           //充值活动
    loot_shenling = 16,                 //神灵塔战斗
    loot_shenling_shop = 17,            //神灵塔商店
    loot_task = 18,                     //任务
    loot_arena_rank = 19,               //竞技场排行
    loot_pk_rank = 20,                  //pk排行
    loot_qq_yellow = 21,                //黄钻
    loot_find_back = 22,                //找回奖励
    loot_present = 23,                  //激活码礼包
    loot_copy_finish = 24,              //副本通关奖励
    loot_online_action = 25,            //在线奖励
    loot_prestige_task = 26,            //声望任务
    loot_wild = 27,                     //城外战斗
    loot_guild_moshen = 28,             //公会魔神
    loot_goal = 29,                     //目标系统
    loot_goal_shop = 30,                //目标商店
    loot_daily_task = 31,               //日常任务
    loot_boss = 32,                     //boss
    loot_lottery = 33,                  //转盘
    loot_auction = 34,                  //拍卖行
    loot_weekRanking = 35,              //周排行
};

/*
礼金获得类型
*/
enum statistics_gold_get_type
{
    gold_get_init = 0,
    gold_get_limit_action = 1,          //限时活动
    gold_get_arena_rank = 2,            //竞技场排行
    gold_get_daily_score = 3,           //每日必做
    gold_get_qq_yellow = 4,             //黄钻
    gold_get_pk_rank = 5,               //筹码排行
    gold_get_sign_action = 6,           //签到奖励
    gold_get_bank = 7,                  //投资
    gold_get_prestige_shop = 8,         //声望商店
    gold_get_lottery = 9,               //转盘
    gold_get_auction = 10,              //拍卖行
};

/*
筹码获得类型
*/
enum statistics_silver_get_type
{
    silver_get_init = 0,
    silver_get_limit_action = 1,        //限时活动
    silver_get_sign_action = 2,         //签到奖励
    silver_get_treasure = 3,            //藏宝图
    silver_get_recharge_event = 4,      //充值活动
    silver_get_libao = 5,               //礼包
    silver_get_online_action = 6,       //在线奖励
    silver_get_levy = 7,                //征收
    silver_get_wild_levy = 8,           //城外征收
    silver_get_shenling = 9,            //神灵塔战斗pve
    silver_get_explore = 10,            //探索洞穴
    silver_get_arena_rank = 11,         //竞技场排行
    silver_get_copy = 12,               //副本战斗pve
    silver_get_stronghold = 13,         //关卡战斗pve
    silver_get_task = 14,               //任务
    silver_get_prestige_shop = 15,      //声望商店
    silver_get_pk = 16,                 //筹码场战斗pvp
    silver_get_qq_yellow = 17,          //黄钻
    silver_get_daily_score = 18,        //每日必做
    silver_get_guild_box = 19,          //公会宝箱
    silver_get_stage = 20,              //通关奖励
    silver_get_help = 21,               //新手保护
    silver_get_sell = 22,               //出售
    silver_get_gem = 23,                //道具获得
    silver_get_wild = 24,               //城外战斗
    silver_get_find_back = 25,          //找回奖励
    silver_get_copy_finish = 26,        //副本通关
    silver_get_guild_moshen = 27,       //公会魔神战斗pve
    silver_get_daily_task = 28,         //日常任务
    silver_get_boss = 29,               //boss
    silver_get_goal = 30,               //目标系统
    silver_get_double = 31,             //翻倍获得
    silver_get_by_friend = 32,          //好友祝贺
    silver_get_auction = 33,            //拍卖行
    silver_get_weekRanking = 34,        //周排行
};

/*
道具获得类型
*/
enum statistics_gem_get_type
{
    gem_get_hero_decompose = 1,         //英雄分解
    gem_get_buy = 2,                    //购买道具
    gem_get_buy_mall = 3,               //购买商城道具
    gem_get_shenling_add = 4,           //神灵塔增加次数
    gem_get_sign_action = 5,            //签到奖励
    gem_get_limit_action = 6,           //限时活动
    gem_get_arena = 7,                  //竞技场战斗
    gem_get_arena_shop = 8,             //竞技场商店
    gem_get_copy = 9,                   //副本战斗
    gem_get_copy_shop = 10,             //副本商店
    gem_get_daily_score = 11,           //每日必做
    gem_get_prestige_shop = 12,         //声望商店
    gem_get_guild_box = 13,             //公会宝箱
    gem_get_libao = 14,                 //礼包
    gem_get_treasure = 15,              //藏宝图
    gem_get_stronghold = 16,            //关卡战斗
    gem_get_recharge_event = 17,        //充值活动
    gem_get_shenling = 18,              //神灵塔战斗
    gem_get_shenling_shop = 19,         //神灵塔商店
    gem_get_task = 20,                  //任务
    gem_get_pk_rank = 21,               //pk排行
    gem_get_qq_yellow = 22,             //黄钻
    gem_get_online_action = 23,         //在线奖励
    gem_get_copy_finish = 24,           //副本通关
    gem_get_daily_task = 25,            //日常任务
    gem_get_guild_moshen = 26,          //公会魔神战斗pve
    gem_get_goal = 27,                  //目标系统
    gem_get_goal_shop = 28,             //目标商店
    gem_get_boss = 29,                  //boss
    gem_get_lottery = 30,               //转盘
    gem_get_present = 31,               //激活码礼包
    gem_get_auction = 32,               //拍卖行
    gem_get_weekRanking = 33,           //周排行
};

/*
历练获得类型
*/
enum statistics_char_exp_get_type
{
    char_exp_get_task = 1,              //任务
    char_exp_get_stage = 2,             //通关奖励
    char_exp_get_find_back = 3,         //找回奖励
    char_exp_get_daily_task = 4,        //日常任务
};

/*
声望获得类型
*/
enum statistics_prestige_get_type
{
    prestige_get_stronghold = 1,        //关卡战斗
    prestige_get_prestige_task = 2,     //声望任务
    prestige_get_gem = 3,               //道具获得
    prestige_get_boss = 4,              //boss战获得
    prestige_get_weekRanking = 5,       //周排行
};

/*
英雄经验获得类型
*/
enum statistics_hero_exp_get_type
{
    hero_exp_get_stronghold = 1,        //关卡战斗
    hero_exp_get_gem = 2,               //道具获得
    hero_exp_get_explore = 3,           //探索洞穴
};

enum statistics_hero_get_type
{
    hero_get_stronghold = 1,        //关卡战斗
    hero_get_auction = 2,
};

/*
筹码消耗类型
*/
enum statistics_silver_cost_type
{
    silver_cost_init = 0,
    silver_cost_equipt_compound = 1,    //装备合成
    silver_cost_equipt_upgrade = 2,     //装备升级
    silver_cost_guild_donate = 3,       //公会捐献
    silver_cost_hero_compound = 4,      //英雄合成
    silver_cost_hero_decompose = 5,     //英雄分解
    silver_cost_hero_pack = 6,          //英雄开包
    silver_cost_buy_mall_good = 7,      //购买商城商品
    silver_cost_treasure_refresh = 8,   //藏宝图刷新
    silver_cost_castle_levelup = 9,     //城堡升级
    silver_cost_metallurgy_levelup = 10,//炼金房升级
    silver_cost_smithy_levelup = 11,    //铁匠铺升级
    silver_cost_barracks_levelup = 12,  //兵营升级
    silver_cost_wild_attack = 13,       //城外战斗
    silver_cost_buy_prestige = 14,      //购买声望商店
    silver_cost_guild_create = 15,      //创建公会
    silver_cost_hero_golden = 16,       //英雄点金
    silver_cost_skill_upgrade = 17,     //技能升级
    silver_cost_quit_combat = 18,       //逃跑惩罚
    silver_cost_stronghold = 19,        //关卡战斗pve
    silver_cost_copy = 20,              //副本战斗pve
    silver_cost_shenling = 21,          //神灵塔战斗pve
    silver_cost_guild_moshen = 22,      //公会魔神战斗pve
    silver_cost_boss_inspire = 23,      //boss鼓舞
    silver_cost_boss = 24,              //boss战斗pve
    silver_cost_bank = 25,              //银行手续费
    silver_cost_auction = 26,           //拍卖
    silver_cost_pk = 27,                //pk买活
};

/*
金币消耗类型
*/
enum statistics_gold_cost_type
{
    gold_cost_init = 0,
    gold_cost_sign = 1,                 //补签
    gold_cost_arena_add = 2,            //竞技场购买次数
    gold_cost_arena_cd = 3,             //竞技场冷却
    gold_cost_levy = 4,                 //强制征收
    gold_cost_copy_reset = 5,           //副本重置
    gold_cost_copy_add = 6,             //副本购买次数
    gold_cost_daily_score_refresh = 7,  //每日必做刷新
    gold_cost_daily_score_done = 8,     //每日必做快速完成
    gold_cost_equipt_compound = 9,      //装备合成
    gold_cost_equipt_upgrade = 10,      //装备升级
    gold_cost_guild_donate = 11,        //公会捐献
    gold_cost_buy_hero_size = 12,       //购买英雄位
    gold_cost_hero_compound = 13,       //英雄合成
    gold_cost_hero_pack = 14,           //英雄开包
    gold_cost_buy_gem = 15,             //购买道具
    gold_cost_buy_bag = 16,             //购买背包位
    gold_cost_horn = 17,                //喇叭聊天
    gold_cost_buy_mall_good = 18,       //购买商城商品
    gold_cost_shenling_refresh_skill = 19,//神灵塔刷新技能
    gold_cost_shenling_reset = 20,      //神灵塔重置
    gold_cost_shenling_add = 21,        //神灵塔增加次数
    gold_cost_sweep_cd = 22,            //扫荡冷却
    gold_cost_task_done = 23,           //任务快速完成
    gold_cost_treasure_finish = 24,     //藏宝图快速完成
    gold_cost_treasure_refresh = 25,    //藏宝图刷新
    gold_cost_treasure_call = 26,       //藏宝图一键召唤
    gold_cost_hero_smelt_refresh = 27,  //英雄熔炼刷新
    gold_cost_attack_stronghold = 28,   //点券通关
    gold_cost_find_back = 29,           //找回
    gold_cost_buy_goal = 30,            //购买目标商品
    gold_cost_boss_cd = 31,             //秒boss冷却
    gold_cost_boss_inspire = 32,        //鼓舞boss
    gold_cost_for_bank = 33,            //投资
    gold_cost_double = 34,              //翻倍
    gold_cost_for_con_friend = 35,      //祝贺好友
    gold_cost_auction = 36,             //拍卖
};

/*
宝物消耗类型
*/
enum statistics_gem_cost_type
{
    gem_cost_use = 1,                   //使用
    gem_cost_copy_add = 2,              //副本购买次数
    gem_cost_equipt_upgrade = 3,        //装备升级
    gem_cost_hero_smelt = 4,            //英雄熔炼
    gem_cost_hero_golden = 5,           //英雄点金
    gem_cost_skill_upgrade = 6,         //技能升级
    gem_cost_hero_epic = 7,             //神将
    gem_cost_shenling_shop = 8,         //神灵塔商店
    gem_cost_shenling = 9,              //神灵塔战斗
    gem_cost_horn = 10,                 //喇叭聊天
    gem_cost_auction = 11,              //拍卖
    gem_cost_equipt_add_slot = 12,      //装备开孔
};

void statistics_of_gold_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);
void statistics_of_gold_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

void statistics_of_silver_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);
void statistics_of_silver_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

void statistics_of_prestige_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);
void statistics_of_char_exp_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);
void statistics_of_hero_exp_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

void statistics_of_gem_get(int cid, const std::string& strIP, int gem_id, int counts, int type, int union_id, const std::string& server_id);
void statistics_of_gem_cost(int cid, const std::string& strIP, int gem_id, int counts, int type, int union_id, const std::string& server_id);

void statistics_of_hero_get(int cid, const std::string& strIP, int hero_id, int counts, int type, int union_id, const std::string& server_id);

