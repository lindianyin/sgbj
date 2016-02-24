#pragma once

#include <string>
#include "data.h"
/*
金币消耗类型
*/
enum gold_cost_type
{
    gold_cost_for_rest = 1,                        //金币休息
    gold_cost_for_buy_elite_stronghold = 2,        //购买精英abandon
    gold_cost_for_world_chat = 3,                //世界聊天abandon
    gold_cost_for_refresh_state = 4,            //刷新状态abandon
    gold_cost_for_refresh_smelt = 5,            //刷新冶炼abandon
    gold_cost_for_refresh_explore = 6,            //刷新探索abandon
    gold_cost_for_refresh_research = 7,            //刷新技能研究abandon
    gold_cost_for_buy_research_que = 8,            //开启技能研究abandon
    gold_cost_for_upgrade_research_que = 9,        //升级金钻队列
    gold_cost_for_accelerate_research = 10,        //完成技能研究abandon
    gold_cost_for_buy_smelt_que = 11,            //开启矿石冶炼abandon
    gold_cost_for_accelerate_smelt = 12,        //加速矿石冶炼abandon
    gold_cost_for_buy_train_que = 13,            //开启武将训练位
    gold_cost_for_accelerate_train = 14,        //加速武将训练
    gold_cost_for_refresh_shop = 15,            //刷新商店
    gold_cost_for_inspire = 16,                    //金币鼓舞abandon
    gold_cost_for_stronghold_reget = 17,        //再次获取abandon
    gold_cost_for_accelerate_farm = 18,            //加速屯田成熟度abandon
    gold_cost_for_corps_donate = 19,            //军团捐献
    gold_cost_for_corps_jisi = 20,                //祭天abandon
    gold_cost_for_buy_wash_book = 21,            //购买洗髓经abandon
    gold_cost_for_refresh_guard_goods = 22,        //刷新可护送粮饷
    gold_cost_for_call_guard_goods = 23,        //召唤黄金万两
    gold_cost_for_clear_guard_cd = 24,            //秒劫粮
    gold_cost_for_accelerate_guard = 25,        //加速护送粮饷
    gold_cost_for_clear_boss_cd = 26,            //加速战神兽
    gold_cost_for_buy_baoshi_pos = 27,            //开启宝石位abandon
    gold_cost_for_wash = 28,                    //洗髓abandon
    gold_cost_for_refresh_train = 29,            //刷新训练书abandon
    gold_cost_for_levy = 30,                    //征收
    gold_cost_for_horse_train = 31,                //战马培养
    gold_cost_for_trade = 32,                    //通商abandon
    gold_cost_for_sweep = 33,                    //扫荡加速
    gold_cost_for_race = 34,                    //竞技场加速
    gold_cost_for_corps_create = 35,            //军团创建abandon
    gold_cost_for_corps_invite = 36,            //宴会邀请路人
    gold_cost_for_wash_baoshi = 37,                //金币宝石洗练abandon
    gold_cost_for_lottery = 38,                    //梅花易数abandon
    gold_cost_for_servant = 39,                    //家丁系统abandon
    gold_cost_for_daily_task = 40,                //日常任务abandon
    gold_cost_for_reborn = 41,                    //一键重生
    gold_cost_for_buy_explore = 42,                //购买探索次数abandon
    gold_cost_for_buy_race = 43,                //购买竞技次数
    gold_cost_for_open_genius = 44,                //开启天赋abandon
    gold_cost_for_clean_genius = 45,            //洗天赋abandon
    gold_cost_for_graft_genius = 46,            //移植天赋abandon
    gold_cost_for_delay_farm = 47,                //延长屯田abandon
    gold_cost_for_reset_elitecombat = 48,        //重置精英战役
    gold_cost_for_upgrade_equipment = 49,        //制作装备保留等级
    gold_cost_for_buy_bag = 50,                    //购买仓库格
    gold_cost_for_speed_farm = 51,                //加速屯田
    gold_cost_for_bank = 52,                    //投资钱庄
    gold_cost_for_trade_speed = 53,                //贸易抽取商人加速
    gold_cost_for_trade_abandon = 54,            //贸易弃用商人
    gold_cost_for_trade_double = 55,            //贸易无奸不商
    gold_cost_for_convert_jade = 56,            //金币购买玉石
    gold_cost_for_buy_inherit = 57,                //购买传承丹abandon
    gold_cost_for_speed_enhance = 58,            //加速强化冷却
    gold_cost_for_buy_daoju = 59,                //购买道具
    gold_cost_for_corps_explore_refresh = 60,    //军团探索刷新
    gold_cost_for_corps_explore_speed = 61,        //军团探索加速
    gold_cost_for_corps_recruit = 62,            //军团招募
    gold_cost_for_findback = 63,                //游戏助手找回
    gold_cost_for_baby = 64,                    //替身娃娃
    gold_cost_for_upgrade_farm = 65,            //升级屯田队列
    gold_cost_for_con_friend = 66,                //一键祝贺
    gold_cost_for_refresh_train_normal = 67,    //金币刷新训练书
    gold_cost_for_refresh_train_best = 68,        //至尊刷新训练书
    gold_cost_for_wash_type2 = 69,                //洗髓青铜
    gold_cost_for_wash_type3 = 70,                //洗髓白金
    gold_cost_for_wash_type4 = 71,                //洗髓钻石
    gold_cost_for_wash_type5 = 72,                //洗髓至尊
    gold_cost_for_clear_boss_cd_best = 73,        //战神兽凤凰涅槃
    gold_cost_for_corps_jisi_type2 = 74,        //祭天玉帛
    gold_cost_for_corps_jisi_type3 = 75,        //祭天血祭
    gold_cost_for_servant_exploit = 76,            //壮丁剥削
    gold_cost_for_servant_exploit2 = 77,        //壮丁抽干
    gold_cost_for_servant_escape = 78,            //壮丁赎身
    gold_cost_for_servant_buy_random = 79,        //壮丁购买随机抓捕次数
    gold_cost_for_bank1 = 80,                    //投资钱庄1
    gold_cost_for_bank2 = 81,                    //投资钱庄2
    gold_cost_for_bank3 = 82,                    //投资钱庄3
    gold_cost_for_bank4 = 83,                    //投资钱庄4
    gold_cost_for_inspire_boss = 84,            //战神兽鼓舞
    gold_cost_for_inspire_camprace = 85,        //阵营战鼓舞
    gold_cost_for_inspire_guard = 86,            //护送粮饷鼓舞
    gold_cost_for_maze_full = 87,                //迷宫补血
    gold_cost_for_maze_reset = 88,                //迷宫重置
    gold_cost_for_maze_skip = 89,                //迷宫跳过
    gold_cost_for_maze_change = 90,                //迷宫变卦
    gold_cost_for_maze_kill = 91,                //迷宫暗度
    gold_cost_for_maze_winmora = 92,            //迷宫猜拳必胜
    gold_cost_for_maze_winguess = 93,            //迷宫猜数字必胜
    gold_cost_for_buy_baoshi = 94,                //购买宝石
    gold_cost_for_servant_buy = 95,                //壮丁购买抓捕次数
    gold_cost_for_buy_libao = 96,                //购买礼包
    gold_cost_for_throne_con = 97,              //参拜皇座
    gold_cost_for_zst_refresh = 98,             //刷新战神台星级
    gold_cost_for_zst_buy = 99,             //购买战神台挑战次数

    gold_cost_for_baoshi = 10000,                //购买宝石+宝石类型*100+宝石等级
    gold_cost_for_treasure = 20000,                //购买宝物+宝物id(本地用，腾讯有id区分)
    gold_cost_for_libao = 30000,                //购买礼包+礼包id
};

/*
金币获得类型
*/
enum gold_get_type
{
    gold_get_recharge = 1,                        //充值
    gold_get_active = 2,                        //活动奖励
    gold_get_stronghold = 3,                    //关卡掉落
    gold_get_explore = 4,                        //探索掉落abandon
    gold_get_race = 5,                            //竞技场
    gold_get_trade = 6,                            //通商获得abandon
    gold_get_task = 7,                            //任务奖励
    gold_get_packs = 8,                            //礼包(激活码)
    gold_get_camps = 9,                            //弱势阵营abandon
    gold_get_shop = 10,                            //商店abandon
    gold_get_lottery = 11,                        //罗生盘
    gold_get_daily_task = 12,                    //日常任务abandon
    gold_get_online_gift = 13,                    //在线礼包
    gold_get_bank = 14,                            //钱庄
    gold_get_treasure = 15,                        //金币卡
    gold_get_gm = 16,                            //gm发放
    gold_get_gift = 17,                            //收藏大礼
    gold_get_libao = 18,                        //礼包(vip礼包各类礼包道具)
    gold_get_bank1 = 19,                        //钱庄1返利
    gold_get_bank2 = 20,                        //钱庄2返利
    gold_get_bank3 = 21,                        //钱庄3返利
    gold_get_bank4 = 22,                        //钱庄4返利
    gold_get_gift_recharge = 10001,            //赠送充值
    gold_get_plat_recharge = 10002,            //内币充值
};

/*
银币消耗
*/
enum silver_cost_type
{
    silver_cost_for_upgrade_weapon = 1,            //升级兵器abandon
    silver_cost_for_shop_buy = 2,                //商店购买
    silver_cost_for_buy_hero = 3,                //招募武将
    silver_cost_for_refresh_state = 4,            //刷新状态abandon
    silver_cost_for_buyback_equiptment = 5,        //回购装备
    silver_cost_for_inherit_equiptment = 6,        //传承装备abandon
    silver_cost_for_inspire = 7,                //银币鼓舞abandon
    silver_cost_for_refresh_research = 8,        //刷新技能研究者abandon
    silver_cost_for_refresh_train = 9,            //刷新武将训练书
    silver_cost_for_wash = 10,                    //银币洗髓
    silver_cost_for_refresh_smelt = 11,            //刷新冶炼abandon
    silver_cost_for_stronghold = 12,            //攻击关卡abandon
    silver_cost_for_horse_train = 13,            //战马培养
    silver_cost_for_wash_baoshi = 14,            //宝石洗练abandon
    silver_cost_for_research = 15,                //技能训练abandon
    silver_cost_for_upgrade_star = 16,            //升级北斗七星abandon
    silver_cost_for_enhance = 17,                //强化装备
    silver_cost_for_baoshi_change = 18,            //转化宝石
    silver_cost_for_baoshi_buy = 19,            //购买宝石
    silver_cost_for_corps_create = 20,            //军团创建
    silver_cost_for_open_box = 21,                //开启宝箱
    silver_cost_for_throne_con = 22,            //参拜皇座
};

/*
银币获得类型
*/
enum silver_get_type
{
    silver_get_task = 1,                        //任务奖励
    silver_get_stronghold = 2,                    //关卡掉落
    silver_get_groupCombat = 3,                    //小型战役掉落
    silver_get_explore = 4,                        //探索掉落abandon
    silver_get_race = 5,                        //竞技场
    silver_get_guard = 6,                        //护送粮饷
    silver_get_boss = 7,                        //战神兽
    silver_get_trade = 8,                        //贸易
    silver_get_farm = 9,                        //屯田abandon
    silver_get_sell_equiptment = 10,            //武将装备出售
    silver_get_corps_jisi = 11,                    //祭祀abandon
    silver_get_boss_last = 12,                    //战神兽致命一击
    silver_get_offical = 13,                    //官职俸禄
    silver_get_campRace = 14,                    //阵营战
    silver_get_packs = 15,                        //礼包(激活码)
    silver_get_levy = 16,                        //征收
    silver_get_by_active = 17,                    //活动
    silver_get_by_lottery = 18,                    //罗生盘
    silver_get_by_daily_task = 19,                //日常abandon
    silver_get_by_online_gift = 20,                //在线礼包
    silver_get_by_friend = 21,                    //好友祝贺
    silver_get_by_treasure = 22,                //银币卡
    silver_get_by_libao = 23,                    //礼包(vip礼包各类礼包道具)
    silver_get_by_con_friend = 24,                //祝贺好友
    silver_get_sell_treasure = 25,                //出售道具
    silver_get_by_ranking = 26,                    //排行奖励
    silver_get_by_maze = 27,                    //八卦阵
    silver_get_sell_baoshi = 28,                //出售宝石
    silver_get_sell_unknow = 29,                //出售未分类
};

/*
军令获得/消耗类型
*/
enum ling_statistics_type
{
    ling_stronghold = 1,                //关卡战斗
    ling_explore = 2,                    //探索abandon
    ling_hero_train = 3,                //武将训练
    ling_skill_train = 4,                //技能训练abandon
    ling_task = 5,                        //任务
    ling_rest_by_gold = 6,                //金币休息
    ling_rest_by_active = 7,            //活动休息
    ling_race = 8,                        //竞技场abandon
    ling_trade = 9,                        //通商abandon
    ling_smelt = 10,                    //冶炼abandon
    ling_shop = 11,                        //商店abandon
    ling_map = 12,                        //过图奖励abandon
    ling_farm = 13,                        //屯田abandon
    ling_boss = 14,                        //战神兽
    ling_race_final = 15,                //竞技场排名abandon
    ling_corps = 16,                    //军团宴会奖励
    ling_packs = 17,                    //礼包(激活码)
    ling_lottery = 18,                    //罗生盘
    ling_servant = 19,                    //家丁abandon
    ling_daily_task = 20,                //日常abandon
    ling_online_gift = 21,                //在线礼包
    ling_libao = 22,                    //礼包(vip礼包各类礼包道具)
    ling_elite_combat = 23,                //精英关卡
};

/*
宝物(军粮功勋玉石)
*/
enum treasure_statistics_type
{
    treasure_stronghold = 1,            //关卡
    treasure_smelt = 2,                    //冶炼abandon
    treasure_shop = 3,                    //商店
    treasure_task = 4,                    //任务
    treasure_explore = 5,                //探索abandon
    treasure_groupCombat = 6,            //小型战役
    treasure_active = 7,                //活动
    treasure_lottery = 8,                //罗生盘
    treasure_servant = 9,                //壮丁
    treasure_daily_task = 10,            //日常助手
    treasure_online_gift = 11,            //在线奖励
    treasure_race = 12,                    //竞技场
    treasure_guard_rob = 13,            //劫取粮饷
    treasure_guard = 14,                //护送粮饷
    treasure_farm = 15,                    //屯田
    treasure_farm_water = 16,            //屯田浇水
    treasure_libao = 17,                //礼包(vip礼包各类礼包道具)
    treasure_corps_explore = 18,        //军团探索
    treasure_corps_ymsj = 19,            //军团辕门射戟
    treasure_weapon = 20,                //秘法
    treasure_inspire = 21,                //鼓舞
    treasure_inspire_boss = 22,            //战神兽鼓舞
    treasure_inspire_camprace = 23,        //阵营战鼓舞
    treasure_baoshi = 24,                //兑换宝石
    treasure_buy = 25,                    //金币购买
    treasure_maze = 26,            //八卦阵获得
    treasure_unknow = 9999
};

/*
声望获得
*/
enum prestige_statistics_type
{
    prestige_groupCombat = 1,            //小型战役
    prestige_boss = 2,                    //战神兽
    prestige_boss_ranking = 3,            //战神兽排名
    prestige_guard = 4,                    //护送粮饷
    prestige_guard_rob = 5,                //劫取粮饷
    prestige_jisi = 6,                    //祭天
    prestige_jisi_type1 = 7,            //牺祭
    prestige_jisi_type2 = 8,            //玉帛
    prestige_jisi_type3 = 9,            //血祭
    prestige_camprace = 10,                //阵营战
    prestige_ranking = 11,                //周排行
    prestige_race = 12,                    //竞技场
};

enum give_loot_type
{
    give_stronghold_loot = 1,
    give_boss_loot = 2,
    give_campRace_loot = 3,
    give_groupCombat_loot = 4,
    give_packs_loot = 5,
    give_race_loot = 6,
    give_sweep_loot = 7,
    give_lottery = 8,
    give_daily_task = 9,
    give_rankings_event = 10,    //排行榜活动
    give_online_gift = 11,        //在线礼包
    give_stage_loot = 12,
    give_guard = 13,    //护送粮草
    give_elite = 14,
    give_libao_loot = 15,
    give_servant_loot = 16,
    give_mail_loot = 17,
    give_maze = 18,
    give_seven_goals = 19,
    give_boss_rank_loot = 20,
    give_boss_last_loot = 21,
    give_explore_loot = 22,
    give_yanhui_loot = 23,
    give_feedback = 24,
    give_invite = 25,
    give_daily_recharge = 26,
    give_box_loot = 27,
    give_throne_loot = 28,
    give_jtz_loot = 29,
    give_jt_boss = 30,
    give_zst = 31,
};

enum baoshi_get_cost
{
    baoshi_admin,
    baoshi_merge,    //合并
    baoshi_convert,//宝石转换
    baoshi_buy,    //商店购买
    baoshi_sell,    //买入卖出
    baoshi_gift,    //赠送

    baoshi_give_loot = 1000
};

//宝石消耗获得统计
void add_statistics_of_baoshi_cost(int cid, const std::string& strIP, int union_id, const std::string& server_id, int baoshiType, int baoshiLevel, int baoshiCount, int type);
void add_statistics_of_baoshi_get(int cid, const std::string& strIP, int union_id, const std::string& server_id, int baoshiType, int baoshiLevel, int baoshiCount, int type);

//增加金币消耗统计
void add_statistics_of_gold_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

//增加银币消耗统计
void add_statistics_of_silver_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

//增加金币来源统计
void add_statistics_of_gold_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

//增加银币来源统计
void add_statistics_of_silver_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

//增加声望来源统计
void add_statistics_of_prestige_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

//军令统计stype:1获得2消耗
void add_statistics_of_ling_cost(int cid, const std::string& strIP, int counts, int type, int stype, int union_id, const std::string& server_id);

//宝物统计stype:1获得2消耗
void add_statistics_of_treasure_cost(int cid, const std::string& strIP, int treasure_id, int counts, int type, int stype, int union_id, const std::string& server_id);

#ifdef QQ_PLAT

//腾讯统计

void cost_todo(CharData* pc, const std::string& gold_price, int type, int nums);
void cost_tobuy(CharData* pc, const std::string& gold_price, int tid, int nums);
void gold_cost_tencent(CharData* pc, int gold_price, int type, int tid = 0, int nums = 1);
void gold_cost_tencent(CharData* pc, double gold_price, int type, int tid = 0, int nums = 1);
void gold_get_tencent(CharData* pc, int gold, int type = 1);
void treasure_cost_tencent(CharData* pc, int tid, int nums);
void att_change_tencent(CharData* pc, const std::string& type, const std::string& before, const std::string& after);
void bag_to_tencent(CharData* pc, const std::string& info);
void ser_to_tencent(const std::string& type, const std::string& stype, int nums);
void guide_to_tencent(CharData* pc, int guide_id);
void login_to_tencent(CharData* pc, const std::string& str1, const std::string& str2, const std::string& friend_id, const std::string& feed_id);

#endif
#if 0
enum act_type
{
    act_stronghold = 1,                    //关卡首次击杀        参数1 关卡id
    act_get_daily = 2,                    //活跃度礼包领取        参数1 礼包id
    act_get_online = 3,                    //在线礼包领取        参数1 礼包id,第几个
    act_shop_buy = 4,                    //商店购买
    act_farm_plant = 5,                    //屯田种植
    act_farm_water = 6,                    //浇灌自己
    act_farm_reap = 7,                    //收获
    act_farm_water_friend = 8,            //浇灌好友
    act_farm_water_all = 9,                //一键浇灌好友
    act_farm_click_upgrade = 10,        //点击农田升级[客户端]
    act_farm_upgrade_recharge = 11,        //农田升级金币不足跳转充值[客户端]
    act_trade_click_select = 12,        //点击抽取商人
    act_trade_click_abandon = 13,        //点击放弃商人[客户端]
    act_trade_start = 14,                //开始贸易                参数 1星级
    act_trade_enter = 15,                //进入贸易
    act_trade_get_goods = 16,            //贸易商品不足点击获取[客户端]
    act_trade_get_goods_sweep = 17,        //获取商品点击扫荡[客户端]
    act_trade_click_wjbs = 18,            //点击无商不奸        参数1第几次
    act_trade_get_silver = 19,            //见好就收
    act_wash_use_silver = 20,            //银币洗髓
    act_enhance_equiptment = 21,        //强化装备                   参数1装备类型( 1 武器  2 盾牌 3、扇子 4、道袍 5、兵符)
    act_make_equiptment = 22,            //制作装备                   参数1装备类型( 1 武器  2 盾牌 3、扇子 4、道袍 5、兵符)
    act_train_general = 23,                //武将训练
    act_train_refresh = 24,                //银币刷新武将训练
    act_reborn_click = 25,                //武将重生
    act_exchange_baoshi = 26,            //玉石兑换宝石         参数1 宝石个数
    act_baoshi_xiangqian = 27,            //宝石镶嵌                   参数1 宝石等级
    act_recruit_general = 28,            //招募武将                参数1 武将基础id
    act_general_click_inherit = 29,        //点击传承[客户端]
    act_general_free_inherit = 30,        //免费传承
    act_general_treasure_inherit = 31,    //传承丹传承
    act_general_click_buy_inherit = 32,    //点击传承丹购买[客户端]
    act_mifa_levelup = 33,                //秘法升级                参数1 ( 1 普通攻击 2 普通防御 3 策略攻击 4 策略防御 5 兵力)
    act_formation_new = 34,                //切换默认阵型
    act_formation_change = 35,            //阵型调整位置
    act_train_horse = 36,                //普通战马培养
    act_ssoul_click_levelup = 37,        //兵魂升级                   参数1 1-3,天地人，参数2 1-5
    act_ssoul_click_buy = 38,            //点击“购买碎片”[客户端]
    act_corps_click_apply = 39,            //点击军团加入申请
    act_corps_click_create = 40,        //点击军团创建[客户端]
    act_corps_create_confirm = 41,        //军团创建
    act_corps_explore_speed = 42,        //军团探索任务加速
    act_corps_explore_refresh = 43,        //军团探索任务刷新
    act_open_ling = 44,                    //打开军令购买界面
    act_get_free_ling = 45,                //调兵遣将领取军令
    act_buy_ling = 46,                    //购买军令
    act_bank_back = 47,                    //钱庄返利                参数1 返利编号(1-4)  参数2 返利2级编号(1-4)
    act_arena_challenge = 48,            //竞技挑战
    act_arena_exchange = 49,            //竞技积分兑换
    act_group_copy = 50,                //小型战役                 参数1 战役编号
    act_boss = 51,                        //参加神兽战               参数1 boss编号
    act_camp_race = 52,                    //参加阵营战
    act_supply_click = 53,                //点击增加军粮[客户端]
    act_rob = 54,                        //掠夺关卡
    act_sweep = 55,                        //军粮不足扫荡[客户端]
    act_sweep_click_speed = 56,            //点击加速扫荡次数
    act_congratulation = 57,            //祝贺好友次数
    act_servant_click_random = 58,        //点击随机抓捕
    act_servant_arrest = 59,            //抓捕手下败将
    act_servant_rescue = 60,            //解救家丁
    act_servant_click_yushi = 61,        //家丁界面点击兑换玉石[客户端]
    act_elite_click = 62,                //精英关卡点击闯关      参数 1精英关卡id[客户端]
    act_elite_attack = 63,                //精英关卡攻打        参数 1精英关卡id
    act_view_attack_click = 64,            //查看总战力[客户端]
    act_view_attack_click_by_pop = 65,    //弹窗查看总战力[客户端]
    act_maze_finish = 66,                //八卦阵通关            参数1什么门参数2什么难度
    act_maze_click_change = 67,            //八卦阵更改卦象[客户端]
    act_maze_buy = 68,                    //八卦阵打折物品确认购买
    act_guard = 69,                        //护送粮饷
    act_corps_jisi = 70,                //军团祭祀
    act_corps_lottery = 71,                //军团罗生盘
    act_get_sign = 72,                    //签到礼包                参数1签到天数
    act_get_level = 73,                    //等级礼包                参数1等级
    act_get_vip = 74,                    //vip礼包                    参数1vip等级
    act_yellow_general_get = 75,        //黄钻武将领取
    act_yellow_new_libao = 76,            //黄钻新手礼包
    act_yellow_daily_libao = 77,        //黄钻每日礼包
    act_yellow_daily_year = 78,            //黄钻年费每日礼包
    act_yellow_level_libao = 79,        //黄钻成长礼包        参数1等级
    act_yellow_click = 80,                //点击开通黄钻[客户端]
    act_bag_use = 81,                    //仓库使用
    act_bag_free = 82,                    //仓库剩余
    act_bag_click_buy = 83,                //点击仓库购买[客户端]
    act_maze_click = 84,                //八卦阵点击[客户端]    参数1什么门参数2什么难度
    act_maze_enter = 85,                //八卦阵进入            参数1什么门参数2什么难度

    act_click_mall = 86,                //点击商城
    act_click_mall_buy = 87,            //点击商城购买
    act_click_recharge_in_mall = 88,    //点击商城内的充值
    act_click_recharge_in_daily_recharge = 89,    //点击每日充值内的充值按钮
    act_get_daily_recharge_award = 90,    //领取每日充值奖励
    act_click_farm_yecha = 91,            //点击屯田野产收获
    act_click_horse_gold_train = 92,        //点击金币培养

    act_click_invite = 93,    //点击邀请好友按钮
    act_click_invite_right_now = 94,    //点击立刻邀请
    act_click_recall_right_now = 95,    //点击立刻召回
    act_click_invite_share = 96,        //点击分享， 参数1 分享id
    act_invite_lottery = 97,            //邀请摇奖
    act_click_mall_baoshi = 98,        //商城界面点击宝石标签
    act_click_mall_gem = 99,            //商城界面点击道具标签
    act_click_daily_recharge = 100,    //点击日冲按钮
};
#endif
enum act_new_type
{
    act_new_stronghold = 1, //攻击关卡参数[武将信息]
    act_new_elite = 2,  //攻击精英关卡
    act_new_helplist_click = 3, //点击助手按钮[flex]
    act_new_helplist_reward = 4,    //领取助手奖励[活跃度]
    act_new_sign = 5,   //签到
    act_new_sign_reward = 6,    //签到在线礼领取[礼包id]
    act_new_7_goal_click = 7,   //点击七日目标[flex]
    act_new_7_goal_reward = 8,  //领取七日目标奖励[奖励id]
    act_new_collect_reward = 9, //领取收藏奖励
    act_new_online_reward = 10, //领取新手在线奖励[奖励id]
    act_new_shop = 11,  //商店购买
    act_new_farm = 12,  //种植
    act_new_water = 13, //浇灌
    act_new_farm_get = 14,    //收获
    act_new_water_friend = 15,  //浇灌好友
    act_new_water_friend_all = 16,  //一键浇灌好友
    act_new_nourish = 17,   //农田野产
    act_new_farm_up_click = 18,   //农田升级点击[flex]
    act_new_farm_up_recharge_click = 19,   //农田升级界面充值点击[flex]
    act_new_trade_select = 20,  //抽取商人
    act_new_trade_wjbs_recharge_click = 21,  //无奸不商界面充值点击[flex]
    act_new_trade_wjbs = 22,  //使用无奸不商
    act_new_trade_abandon = 23, //弃用商人
    act_new_finish = 24,    //完成贸易[商人品质]
    act_new_wash = 25,  //洗髓次数
    act_new_wash_confirm = 26,  //洗髓确定保存
    act_new_wash_continue_click = 27, //持续洗髓点击[flex]
    act_new_wash_up = 28, //洗髓星级提升[星级]
    act_new_equipt_up = 29, //升级装备[装备类型1 武器  2 盾牌 3、扇子 4、道袍 5、兵符]
    act_new_equipt_make = 30,   //制造装备[装备类型1 武器  2 盾牌 3、扇子 4、道袍 5、兵符]
    act_new_equipt_make_special = 31,   //保留等级制造[装备等级，品质]
    act_new_train = 32, //训练武将
    act_new_refresh_book_silver = 33,   //银币刷新兵书
    act_new_baoshi_by_yushi = 34,  //玉石购买宝石[数量]
    act_new_baoshi_xiangqian = 35,    //镶嵌宝石[宝石类型，宝石等级]
    act_new_baoshi_change = 36, //转化宝石[宝石等级]
    act_new_yushi_get = 37, //领取玉石
    act_new_baoshi_by_gold = 38,    //抢购宝石[宝石类型]
    act_new_baoshi_conbine = 39,    //合成宝石[宝石等级]
    act_new_reborn_up = 40, //重生星级提升[星级]
    act_new_reborn = 41,    //重生[武将id]
    act_new_buy_general = 42,   //招募武将[武将id]
    act_new_prestige_get_click = 43,    //招募武将界面声望获取攻略点击[flex]
    act_new_inherit_click = 44, //武将界面传承点击[flex]
    act_new_inherit = 45,   //武将传承[传承，被传承，是否传承丹]
    act_new_inherit_buy_click = 46,   //购买传承丹点击[flex]
    act_new_mifa = 47,    //秘法升级[类型1 普通攻击 2 普通防御 3 策略攻击 4 策略防御 5 兵力]
    act_new_zhen_change = 48,   //更换默认阵形
    act_new_zhen_general_change = 49,   //更换阵形武将
    act_new_horse_train_silver = 50,    //银币培养战马
    act_new_soul_up = 51,   //升级兵魂[类型，小类，等级]
    act_new_soul_buy_click = 52,    //购买兵魂点击[flex]
    act_new_corps_apply_click = 53,   //申请军团点击[flex]
    act_new_corps_create_click = 54,   //创建军团点击[flex]
    act_new_corps_create_confirm_click = 55,   //创建军团确定点击[flex]
    act_new_corps_ymsj = 56,    //辕门射戟
    act_new_corps_explore = 57, //军团探索
    act_new_corps_lottery = 58, //罗生盘
    act_new_corps_jisi_click = 59,    //军团祭天活动点击[flex]
    act_new_corps_ymsj_click = 60,   //辕门射戟活动点击[flex]
    act_new_corps_yanhui_click = 61,   //宴会活动点击[flex]
    act_new_corps_explore_click = 62,   //军团探索活动点击[flex]
    act_new_corps_lottery_click = 63,   //罗生盘活动点击[flex]
    act_new_ling_click = 64,   //增加军令按钮点击[flex]
    act_new_ling_free = 65,   //调兵遣将
    act_new_ling_buy = 66,   //军令购买[花费]
    act_new_supply_click = 67,  //增加军粮按钮点击[flex]
    act_new_supply_window_click = 68,   //弹窗增加军粮点击[flex]
    act_new_char_click = 69,   //角色头像点击[flex]
    act_new_vip_click = 70,   // vip等级点击[flex]
    act_new_vip_recharge_click = 71,   // vip界面充值点击[flex]
    act_new_attack_click = 72,   //战力查看点击[flex]
    act_new_race = 73,   //竞技场挑战
    act_new_race_buy = 74,   //竞技场积分兑换
    act_new_guard = 75, //护送粮饷
    act_new_guard_rob = 76,    //劫取粮饷
    act_new_guard_rob_rank_click = 77,    //点击劫粮排行[flex]
    act_new_guard_help_click = 78,    //点击好友护送[flex]
    act_new_servant_catch_random = 79,  //随机抓捕[flex]
    act_new_servant_catch_race = 80,    //竞技场抓捕[flex]
    act_new_servant_catch_enemy = 81,   //仇敌抓捕[flex]
    act_new_servant_rescue_corps = 82,  //军团解救[flex]
    act_new_servant_rescue_friend = 83, //好友解救[flex]
    act_new_servant_interact = 84,  //互动[flex]
    act_new_servant_yushi_click = 85,   //玉石兑换点击[flex]
    act_new_camp_race_battle = 86,  //阵营战开战
    act_new_camp_race = 87, //阵营战参加
    act_new_congratulation = 88,    //祝贺好友
    act_new_congratulation_recv = 89,   //接受祝贺
    act_new_relation_click = 90,    //点击外交按钮[flex]
    act_new_group_copy = 91,    //参加小型战役
    act_new_group_copy_zhaoji_click = 92,    //点击小型战役召集队友[flex]
    act_new_group_copy_random_click = 93,    //点击小型战役随机邀请[flex]
    act_new_boss = 94,  //挑战boss[bossid]
    act_new_maze = 95,  //进入八卦阵[类型，难度]
    act_new_maze_abandon = 96,  //放弃八卦阵[类型，难度]
    act_new_maze_finish = 97,  //完成八卦阵[类型，难度]
    act_new_bag_buy = 98,   //仓库购买[购买个数，买后个数]
    act_new_bag_click = 99,   //仓库按钮点击[flex]
    act_new_mall_click = 100,   //商城按钮点击[flex]
    act_new_mall_baoshi_click = 101,   //商城宝石按钮点击[flex]
    act_new_mall_daoju_click = 102,   //商城道具按钮点击[flex]
    act_new_mall_recharge_click = 103,   //商城充值按钮点击[flex]
    act_new_rank_click = 104,    //点击排行按钮[flex]
    act_new_rank_type_click = 105,    //点击排行内标签[排行类型][flex]
    act_new_jxl_click = 106,    //点击将星录[flex]
    act_new_jxl_type_click = 107,    //点击将星录各分类[类型][flex]
    act_new_jxl = 108,    //启用将星录[类型]
    act_new_vip_libao_click = 109,    //点击vip礼包[flex]
    act_new_vip_daily_get_click = 110,    //点击vip每日礼包界面领取[flex]
    act_new_vip_daily_recharge_click = 111,    //点击vip每日礼包界面充值[flex]
    act_new_vip_libao_type_click = 112,    //点击vip礼包标签[flex]
    act_new_vip_purple_click = 113,    //点击vip专属紫将标签[flex]
    act_new_vip_purple_recharge_click = 114,    //点击vip专属紫将充值[flex]
    act_new_vip_orange_click = 115,    //点击vip专属橙将标签[flex]
    act_new_vip_orange_recharge_click = 116,    //点击vip专属橙将充值[flex]
    act_new_rank_event_click = 117, //周排行按钮点击[flex]
    act_new_rank_event_last_click = 118, //上周排行按钮点击[flex]
    act_new_bank_click = 119,   //钱庄按钮点击[flex]
    act_new_bank = 120, //钱庄投资[类型]
    act_new_daily_click = 121,  //点击日充值礼包[flex]
    act_new_daily_recharge_click = 122,  //点击日充值界面充值[flex]
    act_new_action_click = 123,  //点击精彩活动[flex]
    act_new_daily_action_click = 124,  //点击日常活动[flex]
    act_new_corps_action_click = 125,  //点击军团活动[flex]
    act_new_more_attack_window = 126,    //实力不足飘窗[flex]
    act_new_more_attack_click = 127,    //点击提升实力[flex]
    act_new_more_attack_more_click = 128,    //点击提升实力"了解更多"[flex]
    act_new_more_attack_more_type_click = 129,    //提升实力细分按钮[类型][flex]
    act_new_levy_window = 130,    //征收飘窗[flex]
    act_new_levy_window_click = 131,    //征收飘窗点击[flex]
    act_new_supply_window = 132,    //军粮飘窗[flex]
    act_new_supply_window_click_ = 133,    //军粮飘窗点击[flex]
    act_new_supply_window_type_click = 134,    //军粮补给细分按钮[类型][flex]
};

void act_to_tencent(CharData* pc, int type, int param1 = 0, int param2 = 0, int param3 = 0, std::string param4 = "");

//腾讯操作统计
int ProcessActTencent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);



