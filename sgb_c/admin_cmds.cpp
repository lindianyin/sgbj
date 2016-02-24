#include "admin_cmds.h"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

static boost::unordered_map<std::string, int>    m_admin_cmds;
static bool g_admin_cmds_inited = false;

void initAdminCmds()
{
    if (g_admin_cmds_inited)
    {
        return;
    }
    //每日更新
    m_admin_cmds["daily_reset"] = admin_daily_reset;
    //每周更新
    m_admin_cmds["week_reset"] = admin_week_reset;
    //心跳处理
    m_admin_cmds["heartBeat"] = admin_heartBeat;
    //保存玩家数据
    m_admin_cmds["save_db"] = admin_save_db;
    //禁言
    m_admin_cmds["gagChar"] = admin_gag_char;
    //冻结帐号
    m_admin_cmds["freezeAccount"] = admin_freeze_account;
    //检查未读邮件
    m_admin_cmds["checkMail"] = admin_check_mail;
    //每日竞技场奖励
    m_admin_cmds["arenaAwards"] = admin_arenaAwards;
    //每日筹码竞技奖励
    m_admin_cmds["pkAwards"] = admin_pkAwards;
    //设置角色等级
    m_admin_cmds["setLevel"] = admin_setLevel;
    //设置角色VIP
    m_admin_cmds["setVip"] = admin_setVip;
    //加点券
    m_admin_cmds["addGold"] = admin_addGold;
    //加礼金
    m_admin_cmds["addBindgold"] = admin_addBindgold;
    //加筹码
    m_admin_cmds["addSilver"] = admin_addSilver;
    //加声望
    m_admin_cmds["addPrestige"] = admin_addPrestige;
    //加道具
    m_admin_cmds["addGem"] = admin_addGem;
    //加英雄
    m_admin_cmds["addHero"] = admin_addHero;
    //设置英雄等级
    m_admin_cmds["heroLevel"] = admin_heroLevel;
    //加装备
    m_admin_cmds["addEquiptment"] = admin_addEquiptment;
    //设置装备等级
    m_admin_cmds["setEquiptment"] = admin_setEquiptment;
    //加宝石
    m_admin_cmds["addBaoshi"] = admin_addBaoshi;
    //发放礼包
    m_admin_cmds["addLibao"] = admin_add_libao;
    //重新加载充值活动
    m_admin_cmds["reloadRechargeEvent"] = admin_reload_recharge_event;
    //开启boss战斗
    m_admin_cmds["openBoss"] = admin_openBoss;
    //关闭boss战斗
    m_admin_cmds["closeBoss"] = admin_closeBoss;
    //开启抽奖活动
    m_admin_cmds["openLottery"] = admin_open_lottery;
    //关闭抽奖活动
    m_admin_cmds["closeLottery"] = admin_close_lottery;

    //修复城守英雄异常
    m_admin_cmds["fixHeroState"] = admin_fix_hero_state;

	m_admin_cmds["changeMallDiscount"] = admin_change_mall_discount;

    g_admin_cmds_inited = true;
}

int getAdminCmds(const std::string& cmds)
{
    if (!g_admin_cmds_inited)
    {
        initAdminCmds();
    }
    boost::unordered_map<std::string, int>::iterator it = m_admin_cmds.find(cmds);
    if (it != m_admin_cmds.end())
    {
        return it->second;
    }
    else
    {
        return admin_empty_cmd;
    }
}

