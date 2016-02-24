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
    //定时恢复军令
    m_admin_cmds["recover_ling"] = admin_recover_ling;
    //保存玩家数据
    m_admin_cmds["save_db"] = admin_save_db;
    //更新角色排名
    m_admin_cmds["updateRank"] = admin_updateRank;
    //更新英雄排名
    m_admin_cmds["updateHeroRank"] = admin_updateHeroRank;
    //更新军团排名
    m_admin_cmds["updateCorpsRank"] = admin_updateCorpsRank;
    //开启boss战斗
    m_admin_cmds["openBoss"] = admin_openBoss;
    //关闭boss战斗
    m_admin_cmds["closeBoss"] = admin_closeBoss;
    //记录在线人数
    m_admin_cmds["saveOnlines"] = admin_saveOnlines;
    //刷新商店
    m_admin_cmds["resetShop"] = admin_resetShop;
    //刷新屯田
    m_admin_cmds["resetFarm"] = admin_resetFarm;
    //每年冬季晚上23点发竞技排名奖
    m_admin_cmds["raceAwards"] = admin_raceAwards;
    m_admin_cmds["debugRaceAwards"] = admin_debug_raceAwards;
    
    //开放阵营战
    m_admin_cmds["openCampRace"] = admin_openCampRace;
    //关闭阵营战
    m_admin_cmds["closeCampRace"] = admin_closeCampRace;
    //每周更新
    m_admin_cmds["corpsWeekReset"] = admin_WeekReset;
    //军团每日更新本日貢獻
    m_admin_cmds["corpsDailyReset"] = admin_corpsDailyReset;
    //给测试人员加金币
    m_admin_cmds["addGold"] = admin_addGold;
    //加银币
    m_admin_cmds["addSilver"] = admin_addSilver;
    //加军令
    m_admin_cmds["addLing"] = admin_addLing;
    //开启调试
    m_admin_cmds["openDebug"] = admin_openDebug;
    //心跳处理
    m_admin_cmds["heartBeat"] = admin_heartBeat;
    //开启多人副本
    m_admin_cmds["openGroupCopy"] = admin_openGroupCopy;
    //关闭多人副本
    m_admin_cmds["closeGroupCopy"] = admin_closeGroupCopy;
    //调整关卡进度
    m_admin_cmds["setTempo"] = admin_setTempo;
    //加英雄
    m_admin_cmds["addGeneral"] = admin_addGeneral;

    //加装备
    m_admin_cmds["addEquiptment"] = admin_addEquiptment;
    //加技能
    m_admin_cmds["setSkill"] = admin_setSkill;
    //加阵型
    m_admin_cmds["setZhen"] = admin_setZhen;
    //加道具
    m_admin_cmds["addTreasure"] = admin_addTreasure;
    //英雄满级
    m_admin_cmds["generalLevel"] = admin_generalLevel;
    //英雄成长率
    m_admin_cmds["generalChengzhang"] = admin_generalChengzhang;
    //冻结帐号
    m_admin_cmds["freezeAccount"] = admin_freeze_account;
    //禁言
    m_admin_cmds["gagChar"] = admin_gag_char;
    //设置 VIP等级
    m_admin_cmds["setVip"] = admin_set_vip;

    //设置装备强化等级
    m_admin_cmds["setEquiptment"] = admin_setEquiptment;
    //设置兵器等级
    m_admin_cmds["setWeapon"] = admin_setWeapon;
    //设置声望
    m_admin_cmds["setPrestige"] = admin_setPrestige;

    //增加声望
    m_admin_cmds["addPrestige"] = admin_addPrestige;

    //增加武将宝物
    m_admin_cmds["addBaowu"] = admin_addBaowu;

    //重新加载屏蔽字
    m_admin_cmds["reloadFilter"] = admin_reload_filter;

    //梅花易数获得物品
    m_admin_cmds["lotteryGet"] = admin_lottery_get;
    
    //重置梅花易数积分
    m_admin_cmds["resetLotteryScore"] = admin_resetLotteryScore;

    //清除所有洗髓点
    m_admin_cmds["clearWash"] = admin_clear_wash;
    //设置武将洗髓点
    m_admin_cmds["setWash"] = admin_set_wash;
    //设置武将成长
    m_admin_cmds["setGrowth"] = admin_set_growth;
    //删除武将
    m_admin_cmds["delGeneral"] = admin_del_general;
    //修改战马
    m_admin_cmds["setHorse"] = admin_set_horse;
    //修改军团等级和经验
    m_admin_cmds["setCorps"] = admin_set_corps;
    //修改角色军团贡献
    m_admin_cmds["setCorpsContribute"] = admin_set_corps_contribute;
    //修改梅花易数积分
    m_admin_cmds["setLotteryScore"] = admin_set_lottery_score;

    //设置武将天赋
    m_admin_cmds["setGenius"] = admin_setGenius;

    //清除充值奖励
    m_admin_cmds["clearRechargeReward"] = admin_clear_recharge_event_reward;
    //重新加载充值活动
    m_admin_cmds["reloadRechargeEvent"] = admin_reload_recharge_event;

    //重置军令福利
    m_admin_cmds["resetWelfare"] = admin_reset_welfare;
    //重置军令福利2
    m_admin_cmds["resetWelfare2"] = admin_reset_welfare2;
    
    //gm福利
    m_admin_cmds["gmReward"] = admin_gm_reward;
    //检查临时vip
    m_admin_cmds["checkTmpVip"] = admin_checkTmpVip;
    //周日24点劫镖排行
    m_admin_cmds["guardAwards"] = admin_guardAwards;

    //开启洗髓打折活动
    m_admin_cmds["openWashEvent"] = admin_open_wash_event;

    //商店打折活动
    m_admin_cmds["openShopDiscount"] = admin_open_shop_discount;

    //家丁双倍活动
    m_admin_cmds["openServantEvent"] = admin_open_servant_event;

    //战马培养活动
    m_admin_cmds["openHorseEvent"] = admin_open_horse_event;

    //免费VIP4活动
    m_admin_cmds["openFreeVip4"] = admin_open_free_vip4;

    //竞技场免费次数
    m_admin_cmds["setRaceFreeTimes"] = admin_set_race_free_times;

    //重生打折活动
    m_admin_cmds["openRebornDiscount"] = admin_open_reborn_discount;

    //修复阵型数据
    m_admin_cmds["fixZhen"] = admin_fix_zhen;

    //发放礼包
    m_admin_cmds["addLibao"] = admin_add_libao;

    //强化装备打折
    m_admin_cmds["openEnhanceDiscount"] = admin_open_enhance_equipment_event;

    //护送粮饷活动开启
    m_admin_cmds["openGuardEvent"] = admin_guard_event;

    m_admin_cmds["setGuardTimes"] = admin_set_guard_times;

    m_admin_cmds["setGuardFac"] = admin_set_guard_fac;
    
    //增加宝石
    m_admin_cmds["addBaoshi"] = admin_add_baoshi;
    
    //刷新宝石商店
    m_admin_cmds["refreshBaoshiShop"] = admin_refresh_baoshishop;

    //更新军团人数上限
    m_admin_cmds["updateCorpsLimit"] = admin_update_corps_limit;

    //开启消费有礼活动
    m_admin_cmds["openFeedback"] = admin_open_feedback;
    //关闭消费有礼活动
    m_admin_cmds["closeFeedback"] = admin_close_feedback;

    //调试消费有礼活动
    m_admin_cmds["debugFeedback"] = admin_debug_feedback;

    //好友邀请分享修复
    m_admin_cmds["fixShareData"] = admin_fix_share_data;
    
    //军团祭祀活动
    m_admin_cmds["corpsJisi"] = admin_corps_jisi;

    //护送重置
    m_admin_cmds["guardReset"] = admin_guard_reset;
    
    //日服邀请好友奖励 参数1：角色ID   参数2：0邀请奖励，1被邀请奖励
    m_admin_cmds["jpSetInvite"] = admin_jp_set_invite;
    
    //战马培养折扣
    m_admin_cmds["horseDiscount"] = admin_horse_discount;

    //商店刷新次数倍数
    m_admin_cmds["shopRefreshMore"] = admin_shop_refresh_more;

    //设置机器军团团长
    m_admin_cmds["robotCorpsLeader"] = admin_set_robot_corps_leader;
    //开启关闭机器人军团 1开启，0关闭
    m_admin_cmds["openRobotCorps"] = admin_open_robot_corps;
    //创建一个机器人军团
    m_admin_cmds["createRobotCorps"] = admin_create_robot_corps;
    //开启军团战报名
    m_admin_cmds["openCorpsFighting"] = admin_open_corps_fighting_singup;
    //开始军团战
    m_admin_cmds["startCorpsFighting"] = admin_start_corps_fighting;
    //重置军团boss
    m_admin_cmds["resetJtBoss"] = admin_reset_jt_boss;

    //领取支线任务
    m_admin_cmds["trunkTask"] = admin_trunk_task;
    //演兵道具返还
    m_admin_cmds["trainingBack"] = admin_training_back;
    //移除沉迷状态
    m_admin_cmds["removeCm"] = admin_remove_chenmi;

    //开启抽奖活动
    m_admin_cmds["openLottery"] = admin_open_lottery;
    //关闭抽奖活动
    m_admin_cmds["closeLottery"] = admin_close_lottery;
    //清除抽奖
    m_admin_cmds["clearLottery"] = admin_clear_lottery_msg;
    
    //设置抽奖文字信息
    m_admin_cmds["lotteryLiteral"] = admin_set_lottery_literal;
    
    //设置军团收益系数
    m_admin_cmds["setCorpsFactor"] = admin_set_corpsFactor;
    //设置贸易收益系数
    m_admin_cmds["setTradeFactor"] = admin_set_tradeFactor;
    //设置八卦阵收益系数
    m_admin_cmds["setMazeFactor"] = admin_set_mazeFactor;
    //设置阵营战收益系数
    m_admin_cmds["setCampRaceFactor"] = admin_set_campRaceFactor;
    //设置竞技场收益系数
    m_admin_cmds["setArenaFactor"] = admin_set_arenaFactor;
    //设置屯田收益系数
    m_admin_cmds["setFarmFactor"] = admin_set_farmFactor;
    //设置神兽实际收益
    m_admin_cmds["setBossFactor"] = admin_set_bossFactor;
    //设置家丁收益
    m_admin_cmds["setServantFactor"] = admin_set_servantFactor;
    //更新cdn
    m_admin_cmds["updateCDN"] = admin_update_cdn;
    //重置武将将魂等级
    m_admin_cmds["resetGsoul"] = admin_reset_gsoul;
    //设置护送加成时段收益系数
    m_admin_cmds["setGuardFactor"] = admin_set_guardFactor;
	//重新模拟冲值活动
	m_admin_cmds["reRecharge"] = admin_reprocess_recharge_event;
    //设置商城折扣
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

