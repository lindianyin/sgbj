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
    //ÿ�ո���
    m_admin_cmds["daily_reset"] = admin_daily_reset;
    //��ʱ�ָ�����
    m_admin_cmds["recover_ling"] = admin_recover_ling;
    //�����������
    m_admin_cmds["save_db"] = admin_save_db;
    //���½�ɫ����
    m_admin_cmds["updateRank"] = admin_updateRank;
    //����Ӣ������
    m_admin_cmds["updateHeroRank"] = admin_updateHeroRank;
    //���¾�������
    m_admin_cmds["updateCorpsRank"] = admin_updateCorpsRank;
    //����bossս��
    m_admin_cmds["openBoss"] = admin_openBoss;
    //�ر�bossս��
    m_admin_cmds["closeBoss"] = admin_closeBoss;
    //��¼��������
    m_admin_cmds["saveOnlines"] = admin_saveOnlines;
    //ˢ���̵�
    m_admin_cmds["resetShop"] = admin_resetShop;
    //ˢ������
    m_admin_cmds["resetFarm"] = admin_resetFarm;
    //ÿ�궬������23�㷢����������
    m_admin_cmds["raceAwards"] = admin_raceAwards;
    m_admin_cmds["debugRaceAwards"] = admin_debug_raceAwards;
    
    //������Ӫս
    m_admin_cmds["openCampRace"] = admin_openCampRace;
    //�ر���Ӫս
    m_admin_cmds["closeCampRace"] = admin_closeCampRace;
    //ÿ�ܸ���
    m_admin_cmds["corpsWeekReset"] = admin_WeekReset;
    //����ÿ�ո��±���ؕ�I
    m_admin_cmds["corpsDailyReset"] = admin_corpsDailyReset;
    //��������Ա�ӽ��
    m_admin_cmds["addGold"] = admin_addGold;
    //������
    m_admin_cmds["addSilver"] = admin_addSilver;
    //�Ӿ���
    m_admin_cmds["addLing"] = admin_addLing;
    //��������
    m_admin_cmds["openDebug"] = admin_openDebug;
    //��������
    m_admin_cmds["heartBeat"] = admin_heartBeat;
    //�������˸���
    m_admin_cmds["openGroupCopy"] = admin_openGroupCopy;
    //�رն��˸���
    m_admin_cmds["closeGroupCopy"] = admin_closeGroupCopy;
    //�����ؿ�����
    m_admin_cmds["setTempo"] = admin_setTempo;
    //��Ӣ��
    m_admin_cmds["addGeneral"] = admin_addGeneral;

    //��װ��
    m_admin_cmds["addEquiptment"] = admin_addEquiptment;
    //�Ӽ���
    m_admin_cmds["setSkill"] = admin_setSkill;
    //������
    m_admin_cmds["setZhen"] = admin_setZhen;
    //�ӵ���
    m_admin_cmds["addTreasure"] = admin_addTreasure;
    //Ӣ������
    m_admin_cmds["generalLevel"] = admin_generalLevel;
    //Ӣ�۳ɳ���
    m_admin_cmds["generalChengzhang"] = admin_generalChengzhang;
    //�����ʺ�
    m_admin_cmds["freezeAccount"] = admin_freeze_account;
    //����
    m_admin_cmds["gagChar"] = admin_gag_char;
    //���� VIP�ȼ�
    m_admin_cmds["setVip"] = admin_set_vip;

    //����װ��ǿ���ȼ�
    m_admin_cmds["setEquiptment"] = admin_setEquiptment;
    //���ñ����ȼ�
    m_admin_cmds["setWeapon"] = admin_setWeapon;
    //��������
    m_admin_cmds["setPrestige"] = admin_setPrestige;

    //��������
    m_admin_cmds["addPrestige"] = admin_addPrestige;

    //�����佫����
    m_admin_cmds["addBaowu"] = admin_addBaowu;

    //���¼���������
    m_admin_cmds["reloadFilter"] = admin_reload_filter;

    //÷�����������Ʒ
    m_admin_cmds["lotteryGet"] = admin_lottery_get;
    
    //����÷����������
    m_admin_cmds["resetLotteryScore"] = admin_resetLotteryScore;

    //�������ϴ���
    m_admin_cmds["clearWash"] = admin_clear_wash;
    //�����佫ϴ���
    m_admin_cmds["setWash"] = admin_set_wash;
    //�����佫�ɳ�
    m_admin_cmds["setGrowth"] = admin_set_growth;
    //ɾ���佫
    m_admin_cmds["delGeneral"] = admin_del_general;
    //�޸�ս��
    m_admin_cmds["setHorse"] = admin_set_horse;
    //�޸ľ��ŵȼ��;���
    m_admin_cmds["setCorps"] = admin_set_corps;
    //�޸Ľ�ɫ���Ź���
    m_admin_cmds["setCorpsContribute"] = admin_set_corps_contribute;
    //�޸�÷����������
    m_admin_cmds["setLotteryScore"] = admin_set_lottery_score;

    //�����佫�츳
    m_admin_cmds["setGenius"] = admin_setGenius;

    //�����ֵ����
    m_admin_cmds["clearRechargeReward"] = admin_clear_recharge_event_reward;
    //���¼��س�ֵ�
    m_admin_cmds["reloadRechargeEvent"] = admin_reload_recharge_event;

    //���þ����
    m_admin_cmds["resetWelfare"] = admin_reset_welfare;
    //���þ����2
    m_admin_cmds["resetWelfare2"] = admin_reset_welfare2;
    
    //gm����
    m_admin_cmds["gmReward"] = admin_gm_reward;
    //�����ʱvip
    m_admin_cmds["checkTmpVip"] = admin_checkTmpVip;
    //����24���������
    m_admin_cmds["guardAwards"] = admin_guardAwards;

    //����ϴ����ۻ
    m_admin_cmds["openWashEvent"] = admin_open_wash_event;

    //�̵���ۻ
    m_admin_cmds["openShopDiscount"] = admin_open_shop_discount;

    //�Ҷ�˫���
    m_admin_cmds["openServantEvent"] = admin_open_servant_event;

    //ս�������
    m_admin_cmds["openHorseEvent"] = admin_open_horse_event;

    //���VIP4�
    m_admin_cmds["openFreeVip4"] = admin_open_free_vip4;

    //��������Ѵ���
    m_admin_cmds["setRaceFreeTimes"] = admin_set_race_free_times;

    //�������ۻ
    m_admin_cmds["openRebornDiscount"] = admin_open_reborn_discount;

    //�޸���������
    m_admin_cmds["fixZhen"] = admin_fix_zhen;

    //�������
    m_admin_cmds["addLibao"] = admin_add_libao;

    //ǿ��װ������
    m_admin_cmds["openEnhanceDiscount"] = admin_open_enhance_equipment_event;

    //�������û����
    m_admin_cmds["openGuardEvent"] = admin_guard_event;

    m_admin_cmds["setGuardTimes"] = admin_set_guard_times;

    m_admin_cmds["setGuardFac"] = admin_set_guard_fac;
    
    //���ӱ�ʯ
    m_admin_cmds["addBaoshi"] = admin_add_baoshi;
    
    //ˢ�±�ʯ�̵�
    m_admin_cmds["refreshBaoshiShop"] = admin_refresh_baoshishop;

    //���¾�����������
    m_admin_cmds["updateCorpsLimit"] = admin_update_corps_limit;

    //������������
    m_admin_cmds["openFeedback"] = admin_open_feedback;
    //�ر���������
    m_admin_cmds["closeFeedback"] = admin_close_feedback;

    //������������
    m_admin_cmds["debugFeedback"] = admin_debug_feedback;

    //������������޸�
    m_admin_cmds["fixShareData"] = admin_fix_share_data;
    
    //���ż���
    m_admin_cmds["corpsJisi"] = admin_corps_jisi;

    //��������
    m_admin_cmds["guardReset"] = admin_guard_reset;
    
    //�շ�������ѽ��� ����1����ɫID   ����2��0���뽱����1�����뽱��
    m_admin_cmds["jpSetInvite"] = admin_jp_set_invite;
    
    //ս�������ۿ�
    m_admin_cmds["horseDiscount"] = admin_horse_discount;

    //�̵�ˢ�´�������
    m_admin_cmds["shopRefreshMore"] = admin_shop_refresh_more;

    //���û��������ų�
    m_admin_cmds["robotCorpsLeader"] = admin_set_robot_corps_leader;
    //�����رջ����˾��� 1������0�ر�
    m_admin_cmds["openRobotCorps"] = admin_open_robot_corps;
    //����һ�������˾���
    m_admin_cmds["createRobotCorps"] = admin_create_robot_corps;
    //��������ս����
    m_admin_cmds["openCorpsFighting"] = admin_open_corps_fighting_singup;
    //��ʼ����ս
    m_admin_cmds["startCorpsFighting"] = admin_start_corps_fighting;
    //���þ���boss
    m_admin_cmds["resetJtBoss"] = admin_reset_jt_boss;

    //��ȡ֧������
    m_admin_cmds["trunkTask"] = admin_trunk_task;
    //�ݱ����߷���
    m_admin_cmds["trainingBack"] = admin_training_back;
    //�Ƴ�����״̬
    m_admin_cmds["removeCm"] = admin_remove_chenmi;

    //�����齱�
    m_admin_cmds["openLottery"] = admin_open_lottery;
    //�رճ齱�
    m_admin_cmds["closeLottery"] = admin_close_lottery;
    //����齱
    m_admin_cmds["clearLottery"] = admin_clear_lottery_msg;
    
    //���ó齱������Ϣ
    m_admin_cmds["lotteryLiteral"] = admin_set_lottery_literal;
    
    //���þ�������ϵ��
    m_admin_cmds["setCorpsFactor"] = admin_set_corpsFactor;
    //����ó������ϵ��
    m_admin_cmds["setTradeFactor"] = admin_set_tradeFactor;
    //���ð���������ϵ��
    m_admin_cmds["setMazeFactor"] = admin_set_mazeFactor;
    //������Ӫս����ϵ��
    m_admin_cmds["setCampRaceFactor"] = admin_set_campRaceFactor;
    //���þ���������ϵ��
    m_admin_cmds["setArenaFactor"] = admin_set_arenaFactor;
    //������������ϵ��
    m_admin_cmds["setFarmFactor"] = admin_set_farmFactor;
    //��������ʵ������
    m_admin_cmds["setBossFactor"] = admin_set_bossFactor;
    //���üҶ�����
    m_admin_cmds["setServantFactor"] = admin_set_servantFactor;
    //����cdn
    m_admin_cmds["updateCDN"] = admin_update_cdn;
    //�����佫����ȼ�
    m_admin_cmds["resetGsoul"] = admin_reset_gsoul;
    //���û��ͼӳ�ʱ������ϵ��
    m_admin_cmds["setGuardFactor"] = admin_set_guardFactor;
	//����ģ���ֵ�
	m_admin_cmds["reRecharge"] = admin_reprocess_recharge_event;
    //�����̳��ۿ�
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

