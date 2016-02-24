
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

extern Database& GetDb();

std::string strCharChatMessage[maxCharactorSpic];
std::string strVipMessage[iMaxVIP];

std::string strCounts = "undefined";
std::string strGold = "undefined";
std::string strBindGold = "undefined";
std::string strSilver = "undefined";
std::string strExp = "undefined";
std::string strCharExp = "undefined";
std::string strArenaScore = "undefined";
std::string strPrestige1 = "undefined";
std::string strPrestige2 = "undefined";
std::string strPrestige3 = "undefined";
std::string strPrestige4 = "undefined";
std::string strPrestigeRandom = "undefined";

std::string strTreasureRobLog = "undefined";
std::string strTreasureMailTitle = "undefined";
std::string strTreasureMailContent = "undefined";
std::string strTreasureRobSucMailContent = "undefined";
std::string strTreasureRobFailMailContent = "undefined";
std::string strTreasureRobLateMailContent = "undefined";
std::string strTreasureRed = "undefined";
std::string strTreasureOrange = "undefined";

std::string strSweepMailTitle = "undefined";
std::string strSweepMailContent = "undefined";

std::string strDailyScoreGet = "undefined";
std::string strEpicHeroGet = "undefined";

std::string strGuildMsgDonateGold = "undefined";
std::string strGuildMsgDonateSilver = "undefined";
std::string strGuildMsgNewLeader = "undefined";
std::string strGuildMsgNewAss = "undefined";
std::string strGuildMsgFire = "undefined";
std::string strGuildMsgLeave = "undefined";
std::string strGuildMsgJoin = "undefined";

std::string strArenaNotifyMailContent = "undefined";
std::string strArenaNotifyMailTitle = "undefined";


std::string strQuitCombatMailTitle = "undefined";
std::string strQuitCombatMailContent = "undefined";

std::string strArenaTop1Msg = "undefined";
std::string strArenaTop2Msg = "undefined";
std::string strArenaTop3Msg = "undefined";
std::string strPkTop1Msg = "undefined";
std::string strPkTop2Msg = "undefined";
std::string strPkTop3Msg = "undefined";
std::string strPkGet500KMsg = "undefined";
std::string strPkInviteMsg = "undefined";

std::string strWildMailTitle = "undefined";
std::string strWildWinMailContent = "undefined";
std::string strWildLoseMailContent = "undefined";

std::string strRecharge1Msg = "undefined";
std::string strRecharge2Msg = "undefined";

std::string strSmeltHeroStar4Msg = "undefined";
std::string strSmeltHeroStar5Msg = "undefined";
std::string strGoldenHeroStar4Msg = "undefined";
std::string strGoldenHeroStar5Msg = "undefined";
std::string strHeroPackMsg = "undefined";

std::string strLibaoMsg = "undefined";

std::string strSystemPassMap = "undefined";
std::string strSystemPassMap2 = "undefined";
std::string strSystemPassCopyMap = "undefined";
std::string strSystemPassCopyMap2 = "undefined";
std::string strCopyPerfectFinish = "undefined";
std::string strArenaTopChangeMsg= "undefined";
std::string strPkTopChangeMsg = "undefined";

std::string strShenling10 = "undefined";
std::string strShenling20 = "undefined";
std::string strShenling30 = "undefined";
std::string strShenling40 = "undefined";
std::string strShenling50 = "undefined";

std::string strBossWillOpen = "undefined";        //boss战将在xx:xx开启
std::string strBossOpening = "undefined";        //boss战正在进行中
std::string strBossIsClosed = "undefined";        //今天boss战已经结束
std::string strBossKillMsg = "undefined";
std::string strBossTop1Damage = "undefined";
std::string strBossTop2Damage = "undefined";
std::string strBossTop3Damage = "undefined";
std::string strBossMail = "undefined";
std::string strBossKillMail = "undefined";
std::string strBossLog = "undefined";
std::string strBossMailTitle = "undefined";
std::string strBossOpenMsg = "undefined";        //boss战开启了
std::string strBossDyingMsg = "undefined";        //boss奄奄一息
std::string strBossGet300KMsg = "undefined";

std::string strLevyGet300KMsg = "undefined";

std::string strSystemPassGuildMoshen = "undefined";
std::string strSystemPassGuildMoshen2 = "undefined";

std::string strLoginMsg1 = "undefined";
std::string strLoginMsg2 = "undefined";
std::string strLoginMsg3 = "undefined";

std::string strLotteryMsg = "undefined";
std::string strDoubleCombatSilverMsg = "undefined";

std::string strCongratulationSuccessMsg = "undefined";
std::string strCongratulationAllSuccessMsg = "undefined";
std::string strCongratulationMaxMsg = "undefined";

std::string strAuctionMailTitle = "undefined";
std::string strAuctionRemoveMailContent = "undefined";
std::string strAuctionWonMailContent = "undefined";
std::string strAuctionSuccessMailContent = "undefined";
std::string strAuctionExpiredMailContent = "undefined";
std::string strAuctionOutbiddedMailContent = "undefined";
std::string strAuctionCancelledMailContent = "undefined";
std::string strAuctionMsg = "undefined";


static std::map<int,std::string> g_errMsgsMap;

int GeneralDataMgr::loadLang()
{
    Query q(GetDb());
    q.get_result("select id,message from base_chat_message where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        if (id >= 1 && id <= maxCharactorSpic)
        {
            strCharChatMessage[id-1] = q.getstr();
            //INFO("************ load chat message "<<id);
        }
    }
    q.free_result();
    q.get_result("select vip,message from base_vip_message where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        if (id >= 1 && id <= iMaxVIP)
        {
            strVipMessage[id-1] = q.getstr();
        }
    }
    q.free_result();
    //加载语言包
    q.get_result("select field,value from base_lang where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        std::string field = q.getstr();
        if (field == "counts")
        {
            strCounts = q.getstr();
        }
        else if (field == "gold")
        {
            strGold = q.getstr();
        }
        else if (field == "bind_gold")
        {
            strBindGold = q.getstr();
        }
        else if (field == "silver")
        {
            strSilver = q.getstr();
        }
        else if (field == "exp")
        {
            strExp = q.getstr();
        }
        else if (field == "char_exp")
        {
            strCharExp = q.getstr();
        }
        else if (field == "arena_score")
        {
            strArenaScore = q.getstr();
        }
        else if (field == "prestige1")
        {
            strPrestige1 = q.getstr();
        }
        else if (field == "prestige2")
        {
            strPrestige2 = q.getstr();
        }
        else if (field == "prestige3")
        {
            strPrestige3 = q.getstr();
        }
        else if (field == "prestige4")
        {
            strPrestige4 = q.getstr();
        }
        else if (field == "prestigeRandom")
        {
            strPrestigeRandom = q.getstr();
        }
        else if (field == "treasureRobLog")
        {
            strTreasureRobLog = q.getstr();
        }
        else if (field == "treasureMailTitle")
        {
            strTreasureMailTitle = q.getstr();
        }
        else if (field == "treasureMailContent")
        {
            strTreasureMailContent = q.getstr();
        }
        else if (field == "treasureRobSucMailContent")
        {
            strTreasureRobSucMailContent = q.getstr();
        }
        else if (field == "treasureRobFailMailContent")
        {
            strTreasureRobFailMailContent = q.getstr();
        }
        else if (field == "treasureRobLateMailContent")
        {
            strTreasureRobLateMailContent = q.getstr();
        }
        else if (field == "treasureRed")
        {
            strTreasureRed = q.getstr();
        }
        else if (field == "treasureOrange")
        {
            strTreasureOrange = q.getstr();
        }
        else if (field == "sweep_mail_title")
        {
            strSweepMailTitle = q.getstr();
        }
        else if (field == "sweep_mail_content")
        {
            strSweepMailContent = q.getstr();
        }
        else if (field == "daily_socre_msg")
        {
            strDailyScoreGet = q.getstr();
        }
        else if (field == "epic_hero_msg")
        {
            strEpicHeroGet = q.getstr();
        }
        else if (field == "guild_donate_gold")
        {
            strGuildMsgDonateGold = q.getstr();
        }
        else if (field == "guild_donate_silver")
        {
            strGuildMsgDonateSilver = q.getstr();
        }
        else if (field == "guild_new_leader")
        {
            strGuildMsgNewLeader = q.getstr();
        }
        else if (field == "guild_new_ass")
        {
            strGuildMsgNewAss = q.getstr();
        }
        else if (field == "guild_fire")
        {
            strGuildMsgFire = q.getstr();
        }
        else if (field == "guild_leave")
        {
            strGuildMsgLeave = q.getstr();
        }
        else if (field == "guild_join")
        {
            strGuildMsgJoin = q.getstr();
        }
        else if (field == "arena_mail_content")
        {
            strArenaNotifyMailContent = q.getstr();
        }
        else if (field == "arena_mail_title")
        {
            strArenaNotifyMailTitle = q.getstr();
        }
        else if (field == "quit_combat_mail_content")
        {
            strQuitCombatMailContent = q.getstr();
        }
        else if (field == "quit_combat_mail_title")
        {
            strQuitCombatMailTitle = q.getstr();
        }
        else if (field == "arena_top1_msg")
        {
            strArenaTop1Msg = q.getstr();
        }
        else if (field == "arena_top2_msg")
        {
            strArenaTop2Msg = q.getstr();
        }
        else if (field == "arena_top3_msg")
        {
            strArenaTop3Msg = q.getstr();
        }
        else if (field == "pk_top1_msg")
        {
            strPkTop1Msg = q.getstr();
        }
        else if (field == "pk_top2_msg")
        {
            strPkTop2Msg = q.getstr();
        }
        else if (field == "pk_top3_msg")
        {
            strPkTop3Msg = q.getstr();
        }
        else if (field == "pk_get500k_msg")
        {
            strPkGet500KMsg = q.getstr();
        }
        else if (field == "pk_invite")
        {
            strPkInviteMsg = q.getstr();
        }
        else if (field == "wild_mail_title")
        {
            strWildMailTitle = q.getstr();
        }
        else if (field == "wild_win_mail")
        {
            strWildWinMailContent = q.getstr();
        }
        else if (field == "wild_lose_mail")
        {
            strWildLoseMailContent = q.getstr();
        }
        else if (field == "recharge1_msg")
        {
            strRecharge1Msg = q.getstr();
        }
        else if (field == "recharge2_msg")
        {
            strRecharge2Msg = q.getstr();
        }
        else if (field == "smelt_hero_star4_msg")
        {
            strSmeltHeroStar4Msg = q.getstr();
        }
        else if (field == "smelt_hero_star5_msg")
        {
            strSmeltHeroStar5Msg = q.getstr();
        }
        else if (field == "golden_hero_star4_msg")
        {
            strGoldenHeroStar4Msg = q.getstr();
        }
        else if (field == "golden_hero_star5_msg")
        {
            strGoldenHeroStar5Msg = q.getstr();
        }
        else if (field == "hero_pack_msg")
        {
            strHeroPackMsg = q.getstr();
        }
        else if (field == "libao_msg")
        {
            strLibaoMsg = q.getstr();
        }
        else if (field == "sys_pass_map")
        {
            strSystemPassMap = q.getstr();
        }
        else if (field == "sys_pass_map2")
        {
            strSystemPassMap2 = q.getstr();
        }
        else if (field == "sys_pass_copy_map")
        {
            strSystemPassCopyMap = q.getstr();
        }
        else if (field == "sys_pass_copy_map2")
        {
            strSystemPassCopyMap2 = q.getstr();
        }
        else if (field == "copy_perfect_finish")
        {
            strCopyPerfectFinish = q.getstr();
        }
        else if (field == "arena_top_change")
        {
            strArenaTopChangeMsg = q.getstr();
        }
        else if (field == "pk_top_change")
        {
            strPkTopChangeMsg = q.getstr();
        }
        else if (field == "shenling_10")
        {
            strShenling10 = q.getstr();
        }
        else if (field == "shenling_20")
        {
            strShenling20 = q.getstr();
        }
        else if (field == "shenling_30")
        {
            strShenling30 = q.getstr();
        }
        else if (field == "shenling_40")
        {
            strShenling40 = q.getstr();
        }
        else if (field == "shenling_50")
        {
            strShenling50 = q.getstr();
        }
        else if (field == "boss_will_open")
        {
            strBossWillOpen = q.getstr();
        }
        else if (field == "boss_opening")
        {
            strBossOpening = q.getstr();
        }
        else if (field == "boss_is_closed")
        {
            strBossIsClosed = q.getstr();
        }
        else if (field == "boss_kill_msg")
        {
            strBossKillMsg = q.getstr();
        }
        else if (field == "boss_top1_damage")
        {
            strBossTop1Damage = q.getstr();
        }
        else if (field == "boss_top2_damage")
        {
            strBossTop2Damage = q.getstr();
        }
        else if (field == "boss_top3_damage")
        {
            strBossTop3Damage = q.getstr();
        }
        else if (field == "boss_mail")
        {
            strBossMail = q.getstr();
        }
        else if (field == "boss_kill_mail")
        {
            strBossKillMail = q.getstr();
        }
        else if (field == "boss_log")
        {
            strBossLog = q.getstr();
        }
        else if (field == "boss_mail_title")
        {
            strBossMailTitle = q.getstr();
        }
        else if (field == "boss_open_msg")
        {
            strBossOpenMsg = q.getstr();
        }
        else if (field == "boss_dying_msg")
        {
            strBossDyingMsg = q.getstr();
        }
        else if (field == "boss_get300k_msg")
        {
            strBossGet300KMsg = q.getstr();
        }
        else if (field == "levy_get300k_msg")
        {
            strLevyGet300KMsg = q.getstr();
        }
        else if (field == "sys_pass_guild_moshen_msg")
        {
            strSystemPassGuildMoshen = q.getstr();
        }
        else if (field == "sys_pass_guild_moshen_msg2")
        {
            strSystemPassGuildMoshen2 = q.getstr();
        }
        else if (field == "sys_login_notify1")
        {
            strLoginMsg1 = q.getstr();
        }
        else if (field == "sys_login_notify2")
        {
            strLoginMsg2 = q.getstr();
        }
        else if (field == "sys_login_notify3")
        {
            strLoginMsg3 = q.getstr();
        }
        else if (field == "double_combat_silver")
        {
            strDoubleCombatSilverMsg = q.getstr();
        }
        else if ("con_success" == field)
        {
            strCongratulationSuccessMsg = q.getstr();
        }
        else if ("con_all_success" == field)
        {
            strCongratulationAllSuccessMsg = q.getstr();
        }
        else if ("con_max" == field)
        {
            strCongratulationMaxMsg = q.getstr();
        }
        else if ("auction_mail_title" == field)
        {
            strAuctionMailTitle = q.getstr();
        }
        else if ("auction_remove_mail" == field)
        {
            strAuctionRemoveMailContent = q.getstr();
        }
        else if ("auction_won_mail" == field)
        {
            strAuctionWonMailContent = q.getstr();
        }
        else if ("auction_success_mail" == field)
        {
            strAuctionSuccessMailContent = q.getstr();
        }
        else if ("auction_expired_mail" == field)
        {
            strAuctionExpiredMailContent = q.getstr();
        }
        else if ("auction_outbidded_mail" == field)
        {
            strAuctionOutbiddedMailContent = q.getstr();
        }
        else if ("auction_cancel_mail" == field)
        {
            strAuctionCancelledMailContent = q.getstr();
        }
        else if ("auction_msg" == field)
        {
            strAuctionMsg = q.getstr();
        }
        else
        {
            cout<<"******************* unknow base_lang.field "<<field<<" **********************"<<endl;
        }
    }
    q.free_result();

    q.get_result("select errCode,errMsg from base_err_msg where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int error_no = q.getval();
        g_errMsgsMap[error_no] = q.getstr();
    }
    q.free_result();
    return 0;
}

std::string getErrMsg(int error_no)
{
    std::map<int, std::string>::iterator it = g_errMsgsMap.find(error_no);
    if (it != g_errMsgsMap.end())
    {
        return it->second;
    }
    return "";
}

