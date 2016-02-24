#pragma once

enum admin_cmds_enum
{
    admin_empty_cmd = 0,
    //每日更新
    admin_daily_reset = 1,
    //每周更新
    admin_week_reset,
    //心跳
    admin_heartBeat,
    //保存玩家数据
    admin_save_db,
    //玩家禁言
    admin_gag_char,
    //冻结帐号
    admin_freeze_account,
    //检查未读邮件
    admin_check_mail,
    //每日竞技场奖励
    admin_arenaAwards,
    //设置角色等级
    admin_setLevel,
    //设置角色VIP
    admin_setVip,
    //加点券
    admin_addGold,
    //加礼金
    admin_addBindgold,
    //加筹码
    admin_addSilver,
    //加声望
    admin_addPrestige,
    //加道具
    admin_addGem,
    //加英雄
    admin_addHero,
    //设置英雄等级
    admin_heroLevel,
    //加装备
    admin_addEquiptment,
    //设置装备等级
    admin_setEquiptment,
    //加宝石
    admin_addBaoshi,
    //发放礼包
    admin_add_libao,
    //重新加载充值活动
    admin_reload_recharge_event,
    //每日筹码竞技奖励
    admin_pkAwards,
    //开启boss战斗
    admin_openBoss,
    //关闭boss战斗
    admin_closeBoss,
    //开启抽奖活动
    admin_open_lottery,
    //关闭抽奖活动
    admin_close_lottery,

    //修复英雄状态
    admin_fix_hero_state,

	//改变商城的折扣(折扣活动)
	admin_change_mall_discount,


};

#include <string>

void initAdminCmds();
int getAdminCmds(const std::string& cmds);

