#pragma once

#include "login.h"
#include "utils_all.h"
#include "net.h"
#include "worker.hpp"
#include "spls_errcode.h"

#include <sys/syscall.h>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "utils_lang.h"
#include "combat.h"
#include "json_spirit_utils.h"
#include "igeneral.h"
#include "explore.h"

#include "spls_race.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "farm.h"
#include "rankings.h"
//#include "smelt.h"
#include "corps.h"
#include "mails.h"
#include "boss.h"
#include "packs.h"
#include "campRace.h"
#include "guard.h"
#include "sweep.h"
#include "db_thread.h"
#include "groupCombat.h"
#include "groupCopy.h"
#include "text_filter.h"
#include "admin_cmds.h"
#include "horse.h"
#include "servant.h"
#include "statistics.h"
#include "lottery.h"
#include "daily_task.h"
#include "rankings.h"
#include "genius.h"
#include "recharge_event.h"
#include "online_gift.h"
#include "shhx_event.h"
#include "eliteCombat.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "singleton.h"
#include "shop.h"
#include "libao.h"
#include "combatRecord.h"
#include "relation.h"
#include "new_ranking.h"
#include "new_trade.h"
#include "bank.h"

#include "corpsLottery.h"
#include "corpsExplore.h"
#include "maze.h"
#include "new_event.h"
#include "first_seven_goals.h"
#include "rewards.h"

#include "cost_feedback.h"
#include "qq_invite.h"
#include "char_jxl.h"
#include "throne.h"
#include "corpsFighting.hpp"
#include "char_zst.h"
#include "lottery_event.hpp"
#include "char_general_soul.hpp"
#include "equipt_upgrade.h"

using namespace net;
using namespace json_spirit;

//Ŀǰ���ŵ�ͼ
volatile int gMaxOpenMap = max_map_id;

//���Գ�ֵ���� 0��ֹ 1����ÿ��һ�� 2������
volatile int gTestRechargeType = 0;

//ϴ������
extern volatile int g_wash_discount/* = 100*/;        //ϴ���ۿ�
extern volatile int g_wash_real_cost[5];

//�̵���ۻ
extern volatile int g_shop_discount;

//�̵�ˢ�´������� x ?
extern volatile int g_shop_refresh_more;

//�̵�ˢ��ʱ�� 6Сʱ
extern volatile int g_shop_refresh_secs;

//����װ���Ĵ��ۻ
extern volatile int g_equiptment_enhance_discount;

//ս��ѵ������
extern volatile int iHorseTrainTime;        //ÿ������ս�����
extern volatile int iHorseGoldTrainTime;    //ÿ��������ս�����

//ս��ѵ���ۿ�
extern volatile int iHorseTrainDiscount;

//��Ѿ�������
extern volatile int iRaceFreeTimes;    //���15����ս

//�������ۻ
extern volatile int g_reborn_discount;

//#define debug_combat_result    1

extern std::string strGroupCopyIconTips;
extern std::string strGetManyGoldMsg;
extern std::string strLotteryGetHeroMsg;
const std::string strRestNotify = "{\"active\":1,\"cmd\":\"updateRest\",\"s\":200}";

extern std::string strNeedMoreLevel;
extern std::string strWashEventOpen;
extern std::string strWashEventClose;

#ifdef JP_SERVER
extern std::string strJpInviteMail1Title;
extern std::string strJpInviteMail2Title;
extern std::string strJpInviteMail1Content;
extern std::string strJpInviteMail2Content;

extern std::string strJpInvitedMail1Title;
extern std::string strJpInvitedMail2Title;
extern std::string strJpInvitedMail1Content;
extern std::string strJpInvitedMail2Content;
#endif

extern std::string GetExeName();

//���ݹ�����ʽ�ͶԷ�������������ع�����Χ
json_spirit::Array getAttackRange(int atype, int mypos, int pos[]);

extern void InsertCombat(Combat* pcombat);
extern void InsertSaveCombat(Combat* pcombat);
extern void InsertSaveDb(const std::string& sql);
extern void InsertSaveDb(const saveDbJob& job);

extern void InsertMailcmd(mailCmd& pmailCmd);
extern void InsertDbCharCmd(dbCmd& _dbCmd);

extern void setRewardFactor(int type, int factor);

int get_statistics_type(int reward_type, int statistics_type);

//��������״̬����ս������
extern int updateCombatAttribute(npcStrongholdStates& states, combatAttribute& ct);

//���ý���ȼ�
void resetAllGeneralSoul(int cid, int gid, int level);

//����boss����
extern int ProcessEnterBossScene(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡBoss�̶���Ϣ
extern int ProcessGetBossInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡBossʣ��Ѫ��
extern int ProcessGetBossHp(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡBossս�μ�����
extern int ProcessGetBossPerson(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����
extern int ProcessInspire(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//getBossCoolTime //��ȡ����Boss��ȴʱ��
extern int ProcessGetCoolTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:endCoolTime //��������Boss��ȴʱ��
extern int ProcessEndCoolTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getBossDamage//��ȡ��Boss����˺�������б�
extern int ProcessGetBossDamage(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getActionInfo//��û������Ϣ
extern int ProcessGetAction(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getActionInfo//��û������Ϣ
extern int ProcessGetActionMemo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//cmd:getGiftList//�����Ʒ������Ϣ
extern int ProcessListPacks(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
extern int ProcessQueryUnGetGifts(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);


//cmd:getGroupTime//�����Ӫս����ʱ��
extern int ProcessGetGroupTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getGroupWins//�����Ӫս��ʤ��
extern int ProcessGetGroupWins(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getGroupMark//�����Ӫս˫������
extern int ProcessGetGroupMark(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getGroupInspire//��ù������ӵ�ս�����ٷ���
extern int ProcessGetGroupInspire(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getGroupBattleInfo//�����Ӫս����б�
extern int ProcessGetGroupList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getGroupReportList//���ս���б�
extern int ProcessGetGroupReportList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:inspireGroup//����ս����
extern int ProcessInspireGroup(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������Ӫս cmd: enterGroupBattle
extern int ProcessJoinCampRace(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:startGroupBattle//��ս
extern int ProcessStartGroupBattle(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:stopGroupBattle//������ս
extern int ProcessStopGroupBattle(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getGroupBattleInfo//�����Ӫս�����Ϣ
extern int ProcessGetGroupBattleInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������
extern int ProcessMatchCampRace(json_spirit::mObject& o);

//��ѯ������Ϣ
extern int ProcessGetFeteList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//����
extern int ProcessFete(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ���
extern int ProcessQueryYanhui(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//�ټ����
extern int ProcessYanhuiZhaoji(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//�������
extern int ProcessJoinYanhui(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

extern int ProcessInviteSomeone(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ʾ����
extern int ProcessListNewWeapons(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʾ��������
extern int ProcessQueryWeaponMemo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʾ����������Ϣ
extern int ProcessQueryWeaponUpgradeInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������
extern int ProcessUpgradeWeapon(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

extern Combat* createStrongholdCombat(int cid, int strongholdId, int& ret);

//��ֵ
extern int ProcessRecharge(session_ptr& psession);

//�����ճ�����
extern int ProcessNewAdminNotice(json_spirit::mObject& o);

//ɾ���ճ�����
extern int ProcessDeleteAdminNotice(json_spirit::mObject& o);

//�޸��ճ�����
extern int ProcessChangeAdminNotice(json_spirit::mObject& o);

//�㲥�ճ�����
extern int ProcessSendAdminNotice(json_spirit::mObject& o);

//��ѯ�ճ�����
extern int ProcessQueryAdminNotice(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//�رմ���
extern int ProcessShutdown(json_spirit::mObject& o);

//cmd:getBossRankList//��ȡ��Boss�������˺���������а�
extern int ProcessGetBossRankList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//cmd:getCampRaceRankList//��ȡ��Ӫս��ʤ���а�
extern int ProcessGetCampRaceRankList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ��ֵ�
extern int ProcessQueryRechargeEvent(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ȡ��ֵ�����
extern int ProcessGetRechargeEventReward(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

/***************** �Ƕ���P�ӿ� ***********************/
//��ʯtips
int ProcessQueryBaoshiInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʯ�}���б�
int ProcessQueryBaoshiList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���Ƕ�䌢�б�
int ProcessQueryBaoshiGenerals(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�䌢��ʯ�б�
int ProcessQueryGeneralBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ȡ���䌢��ʯ
int ProcessRemoveBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Ƕ��ʯ
int ProcessXiangqianBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ϲ���ʯ
int ProcessCombineBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//һ���ϲ���ʯ
int ProcessCombineAllBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʯת��
int ProcessChangeBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�������ʯ
int ProcessGetYushi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ʯ
int ProcessBuyShopBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʯ�̵�
int ProcessBaoshiShop(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//ϴ����ʯ
//int ProcessWashBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ԃ��ʯϴ����Ϣ
//int ProcessQueryBaoshiWashInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ʯ����
//int ProcessOpenBaoshiAttribute(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������ʯ
//int ProcessLevelupBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�һ���ʯ
int ProcessExchangeBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�һ���ʯ
int ProcessExchangeYushi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ�һ���ʯ
int ProcessQueryExchangeBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ��ʯ������Ϣ
//int ProcessQueryBaoshiLevelupInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//Ӣ����Ƕ��ʯ������
int ProcessGeneralBaoshiInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//÷��������ȡ��Ʒ
int ProcessLottery(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ÷���������˼�¼
int ProcessQueryLotteryRecords(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ÷������ȫ������
int ProcessQueryLotteryNotice(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ÷����������
int ProcessQueryScore(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ��������
int ProcessQueryTreasure(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ���߼۸�
int ProcessQueryTreasurePrice(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡÿ��������
int ProcessRewardDailyTask(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

/**��ѯ������Ϣ**/
int ProcessQueryDLPlace(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

/***************** ��Ӣս�۽ӿ� ***********************/

//���ݵ�ǰ���������ѯ��Ӣս���б�
extern int ProcessGetEliteCombatList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����
extern int ProcessAttackEliteCombat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���þ�Ӣս��
extern int ProcessResetEliteCombat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//�����װ����Ϣ
extern int ProcessGetSysEquipInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

void ItemToObj(Item* sitem, boost::shared_ptr<json_spirit::Object>& sgetobj);

//ף�غ���
extern int ProcessCongratulation(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ף���б�
extern int ProcessGetCongratulations(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ף���б�
extern int ProcessGetRecvedCongratulations(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���Ѷ�̬�б�
//extern int ProcessGetFriendInfoList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����б�
extern int ProcessGetEnemyList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Ƴ����
extern int ProcessRemoveEnemy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Ƽ������б�
extern int ProcessGetRecommendFriends(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ж�̬�б�
extern int ProcessGetEnemyInfoList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ó��

//��ѯó�����    cmd��  getTradeCombos
int ProcessGetTradeCombos(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ��ǰ�̶����    cmd��getMyTraderList
int ProcessGetMyTraderList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������    cmd:    abandonTrader, pos:λ�ã�int��
int ProcessAbandonTrader(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�̶�    cmd:    getTraderList
int ProcessGetTraderList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ���ˣ�    cmd��    selectTrader    ,id:����id��int��
int ProcessSelectTrader(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯó����Ϣ��    cmd��getTradeInfo
int ProcessGetTradeInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʼó�ף�    cmd��startTrade
int ProcessStartTrade(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ó��wjbs    cmd��tradeWjbs
int ProcessTradeWjbs(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���ó�ף�������    cmd: finishTrade
int ProcessFinishTrade(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//Ǯׯ

//Ͷ��
extern int ProcessBuyBankCase(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����
extern int ProcessGetBankFeedback(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ��Ŀ��Ϣ
extern int ProcessGetCaseInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ��Ŀ�б�
extern int ProcessGetBankList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//����ϵͳ
//��ѯ�����б� cmd ��querySoulList
extern int ProcessQuerySoulList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������������ cmd ��upgradeSoul
extern int ProcessUpgradeSoul(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
extern int ProcessGetSoulsCostInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
extern int ProcessBuySoulsDaoju(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//cmd:queryJtBossTime
int ProcessQueryJtBossTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//cmd:setJtBossTime
int ProcessSetJtBossTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//cmd:setJtBossTime
int ProcessCheckJtBossOpen(json_spirit::mObject& o);

Database& GetDb();

//����Ӣ��
static std::list<boost::shared_ptr<GeneralTypeData> > create_attack_generals;
//����Ӣ��
static std::list<boost::shared_ptr<GeneralTypeData> > create_defense_generals;

static Object query_Create_obj;

void InitCreateList()
{
    static bool inited = false;
    if (inited)
    {
        return;
    }
    create_defense_generals.clear();
    for (size_t i = 0; i < 3; ++i)
    {
        int gid = create_choose_generals_id[i];
        INFO("initCreatelist"<<gid<<endl);
        //��û����佫����
        boost::shared_ptr<GeneralTypeData> gd = GeneralDataMgr::getInstance()->GetBaseGeneral(gid);
        if (gd.get())
        {
            create_defense_generals.push_back(gd);
        }
        else
        {
            INFO("init Create list get null "<<gid<<endl);
        }
    }

    create_attack_generals.clear();
    for (size_t i = 3; i < 6; ++i)
    {
        int gid = create_choose_generals_id[i];
        INFO("initCreatelist"<<gid<<endl);
        //��û����佫����
        boost::shared_ptr<GeneralTypeData> gd = GeneralDataMgr::getInstance()->GetBaseGeneral(gid);
        if (gd.get())
        {
            create_attack_generals.push_back(gd);
        }
        else
        {
            INFO("init Create list get null "<<gid<<endl);
        }
    }
    json_spirit::Array char_array;
    std::list<boost::shared_ptr<GeneralTypeData> >::iterator it = create_defense_generals.begin();
    while (it != create_defense_generals.end())
    {
        if ((*it).get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", (*it)->m_gid));        //�佫id
            obj.push_back( Pair("spic", (*it)->m_gid));        //�佫id
            obj.push_back( Pair("name", (*it)->m_name));     //�佫��
            obj.push_back( Pair("wisdom", (*it)->base_int)); //��
            obj.push_back( Pair("govern", (*it)->base_tongyu));//ͳ
            obj.push_back( Pair("brave", (*it)->base_str));  //��
            boost::shared_ptr<BaseSoldierData> sd = GeneralDataMgr::getInstance()->GetBaseSoldier((*it)->m_stype);
            if (sd.get())
            {
                json_spirit::Object sobj;
                sobj.push_back( Pair("id", (*it)->m_stype));//����id
                sobj.push_back( Pair("spic", (*it)->m_stype));//����id
                sobj.push_back( Pair("name", sd->m_name));          //������
                obj.push_back( Pair("soldier", sobj));   //��
                obj.push_back( Pair("memo", sd->m_desc));     //����
            }
            char_array.push_back(obj);
        }
        ++it;
    }
    query_Create_obj.push_back( Pair("dlist", char_array));

    char_array.clear();
    it = create_attack_generals.begin();
    while (it != create_attack_generals.end())
    {
        if ((*it).get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", (*it)->m_gid));        //�佫id
            obj.push_back( Pair("spic", (*it)->m_spic));        //�佫id
            obj.push_back( Pair("name", (*it)->m_name));     //�佫��
            obj.push_back( Pair("wisdom", (*it)->base_int)); //��
            obj.push_back( Pair("govern", (*it)->base_tongyu));//ͳ
            obj.push_back( Pair("brave", (*it)->base_str));  //��
            boost::shared_ptr<BaseSoldierData> sd = GeneralDataMgr::getInstance()->GetBaseSoldier((*it)->m_stype);
            if (sd.get())
            {
                json_spirit::Object sobj;
                sobj.push_back( Pair("id", (*it)->m_stype));//����id
                sobj.push_back( Pair("spic", (*it)->m_stype));//����id
                sobj.push_back( Pair("name", sd->m_name));          //������
                obj.push_back( Pair("soldier", sobj));   //��
                obj.push_back( Pair("memo", sd->m_desc));     //����
            }
            char_array.push_back(obj);
        }
        ++it;
    }
    query_Create_obj.push_back( Pair("alist", char_array));
    query_Create_obj.push_back( Pair("rolenum", maxCharactorSpic));
    query_Create_obj.push_back( Pair("cmd", "qcreate"));
    inited = true;
}

typedef int (*pFuncProcessCmds)(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj);

typedef int (*pFuncInternalProcessCmds)(json_spirit::mObject& o);

int get_statistics_type(int reward_type, int statistics_type)
{
    switch(reward_type)
    {
        case item_type_silver:
            {
                switch(statistics_type)
                {
                    case give_stronghold_loot:
                    case give_sweep_loot:
                    case give_stage_loot:
                        return silver_get_stronghold;
                        break;
                    case give_packs_loot:
                        return silver_get_packs;
                        break;
                    case give_campRace_loot:
                        return silver_get_campRace;
                        break;
                    case give_groupCombat_loot:
                        return silver_get_groupCombat;
                        break;
                    case give_boss_loot:
                        return silver_get_boss;
                        break;
                    case give_race_loot:
                        return silver_get_race;
                        break;
                    case give_lottery:
                        return silver_get_by_lottery;
                        break;
                    case give_daily_task:
                        return silver_get_by_daily_task;
                        break;
                    case give_online_gift:
                        return silver_get_by_online_gift;
                        break;
                    case give_guard:
                        return silver_get_guard;
                        break;
                    case give_libao_loot:
                    case give_mail_loot:
                        return silver_get_by_libao;
                        break;
                    case give_rankings_event:
                        return silver_get_by_ranking;
                        break;
                    case give_maze:
                        return silver_get_by_maze;
                        break;
                    case give_boss_last_loot:
                        return silver_get_boss_last;
                        break;
                }
            }
            break;
        case item_type_gold:
            {
                switch(statistics_type)
                {
                    case give_stronghold_loot:
                    case give_sweep_loot:
                    case give_stage_loot:
                        return gold_get_stronghold;
                        break;
                    case give_packs_loot:
                        return gold_get_packs;
                        break;
                    case give_race_loot:
                        return gold_get_race;
                        break;
                    case give_lottery:
                        return gold_get_lottery;
                        break;
                    case give_online_gift:
                        return gold_get_online_gift;
                        break;
                    case give_libao_loot:
                    case give_mail_loot:
                        return gold_get_libao;
                        break;
                }
            }
            break;
        case item_type_ling:
            {
                switch(statistics_type)
                {
                    case give_stronghold_loot:
                    case give_sweep_loot:
                    case give_stage_loot:
                        return ling_stronghold;
                        break;
                    case give_boss_loot:
                        return ling_boss;
                        break;
                    case give_packs_loot:
                        return ling_packs;
                        break;
                    case give_race_loot:
                        return ling_race;
                        break;
                    case give_lottery:
                        return ling_lottery;
                        break;
                    case give_daily_task:
                        return ling_daily_task;
                        break;
                    case give_online_gift:
                        return ling_online_gift;
                        break;
                    case give_libao_loot:
                    case give_mail_loot:
                        return ling_libao;
                        break;
                    case give_yanhui_loot:
                        return ling_corps;
                        break;
                }
            }
            break;
        case item_type_treasure:
            {
                switch (statistics_type)
                {
                    case give_stronghold_loot:
                    case give_sweep_loot:
                    case give_stage_loot:
                        return treasure_stronghold;
                        break;
                    case give_groupCombat_loot:
                        return treasure_groupCombat;
                        break;
                    case give_lottery:
                        return treasure_lottery;
                        break;
                    case give_online_gift:
                        return treasure_online_gift;
                        break;
                    case give_guard:
                        return treasure_guard_rob;
                        break;
                    case give_daily_task:
                        return treasure_daily_task;
                        break;
                    case give_libao_loot:
                    case give_mail_loot:
                        return treasure_libao;
                        break;
                    case give_maze:
                        return treasure_maze;
                        break;
                }
            }
            break;
        case item_type_prestige:
            {
                switch (statistics_type)
                {
                    case give_groupCombat_loot:
                        return prestige_groupCombat;
                        break;
                    case give_boss_loot:
                        return prestige_boss;
                        break;
                    case give_guard:
                        return prestige_guard_rob;
                        break;
                    case give_campRace_loot:
                        return prestige_camprace;
                        break;
                    case give_rankings_event:
                        return prestige_ranking;
                        break;
                    case give_race_loot:
                        return prestige_race;
                        break;
                    case give_boss_rank_loot:
                        return prestige_boss_ranking;
                        break;
                }
            }
            break;
    }
    return 9999;
}

int giveLoots(CharData* cdata, Item& getItem, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type)
{
    std::list<Item> getItems;
    getItems.push_back(getItem);
    return giveLoots(cdata, getItems, mapid, level, type, pCombat, robj, isAttacker, statistics_type);
}

int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type)
{
    int total_get = 0;
    json_spirit::Array getlist;
    std::list<Item>::iterator it = getItems.begin();          //��õĶ���
    std::string extraMsg = "";
    while (it != getItems.end())
    {
        json_spirit::Object getobj;
        switch (it->type)
        {
            case item_type_silver:    //����
                {
                    //����������
                    it->nums = cdata->chenmiGet(it->nums);
                    cdata->addSilver(it->nums);
                    cdata->NotifyCharData();
                    ++total_get;
                    //���һ��ͳ��
                    if (get_statistics_type(it->type,statistics_type))
                        add_statistics_of_silver_get(cdata->m_id,cdata->m_ip_address,it->nums,get_statistics_type(it->type,statistics_type), cdata->m_union_id, cdata->m_server_id);
                }
                break;
            case item_type_silver_map://��ͼ��������������
                {
                    it->type = item_type_silver;
                    it->nums *= getMapSilver(mapid, level, type > 1);
                    continue;
                }
                break;
            case item_type_silver_level://n*�����ȼ�������
                {
                    it->type = item_type_silver;
                    it->nums = level * it->nums;
                    continue;
                }
                break;
            case item_type_treasure_level://N*�ȼ��ĵ���
                {
                    it->type = item_type_treasure;
                    it->nums = level * it->nums;
                    continue;
                }
            case item_type_treasure:    //����
                {
                    boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(it->id);
                    if (tr.get())
                    {
                        ++total_get;
                        it->nums = cdata->chenmiGet(it->nums);
                        //������ͳ��
                        if (get_statistics_type(it->type,statistics_type))
                            add_statistics_of_treasure_cost(cdata->m_id,cdata->m_ip_address,it->id,it->nums,get_statistics_type(it->type,statistics_type),1, cdata->m_union_id, cdata->m_server_id);
                        int err_code = 0;
                        cdata->addTreasure(it->id, it->nums, err_code);
                        if (err_code != 0)
                            extraMsg += getErrMsg(HC_ERROR_BACKPACK_FULL_GET_EQUIPT);

                        if (treasure_type_supply == it->id)
                        {
                            cdata->NotifyCharData();
                        }
                        //�ؿ����
                        if (give_sweep_loot == statistics_type || give_stronghold_loot == statistics_type)
                        {
                            cdata->m_trunk_tasks.updateTask(task_gather_gem, it->id, it->nums);
                        }
#ifdef QQ_PLAT
                        //��ҿ�����ߵ�����Ҫ��¼�۸�Ϊ0
                        if (it->id == treasure_type_chuanchengdan || it->id == treasure_type_corps_lottery || it->id == treasure_type_soul_type1 || it->id == treasure_type_soul_type2 || it->id == treasure_type_soul_type3)
                        {
                            gold_cost_tencent(cdata,0,gold_cost_for_buy_daoju,it->id,it->nums);
                        }
#endif
                    }
                    else
                    {
                        ERR();
                    }
                }
                break;
            case item_type_equipment://װ��
                {
                    for (int i = 0; i < it->nums; ++i)
                    {
                        int equip_id = cdata->addEquipt(it->id, extraMsg);
                        ++total_get;
                        #if 0
                        if (extraMsg == "")
                        {
                            json_spirit::Object obj;
                            obj.push_back( Pair("cmd", "notify") );
                            obj.push_back( Pair("s", 200) );
                            obj.push_back( Pair("type", notify_msg_new_equipment) );
                            cdata->sendObj(obj);
                        }
                        #endif
                    }
                }
                break;
            case item_type_general://�佫
                {
                    if (cdata->m_generals.Add(it->id, false, it->nums) > 0)
                    {
                        ++total_get;
                    }
                    else
                    {
                        it->type = item_type_silver;
                        it->nums = 100;
                        lootMgr::getInstance()->getWorldItemFall(*it);
                        continue;
                    }
                }
                break;
            case item_type_gold:    //���
                {
                    //����������
                    int gold = cdata->chenmiGet(it->nums);
                    cdata->addGold(gold);
                    cdata->NotifyCharData();
                    ++total_get;
                    //�P�����Y�@��100�����M��ȫ������
                    if (pCombat && pCombat->m_type == combat_stronghold && it->nums >= 100)
                    {
                        std::string msg = strGetManyGoldMsg;
                        str_replace(msg, "$G", LEX_CAST_STR(it->nums));
                        str_replace(msg, "$W", MakeCharNameLink(pCombat->m_attacker->Name()));
                        GeneralDataMgr::getInstance()->broadCastSysMsg(msg,-1);
                    }
                    //��һ��ͳ��
                    if (get_statistics_type(it->type,statistics_type))
                        add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address,it->nums,get_statistics_type(it->type,statistics_type), cdata->m_union_id, cdata->m_server_id);
                    if (statistics_type == give_online_gift)
                    {
                        //ֱ�Ӽ�
                        InsertSaveDb("insert into char_recharge set type='online_gift',cid=" + LEX_CAST_STR(cdata->m_id)
                                + ",account='',gold='" + LEX_CAST_STR(gold) + "',input=now()");
                        cdata->m_total_recharge += gold;
                        InsertSaveDb("replace into char_total_recharge (cid,total_recharge) values (" + LEX_CAST_STR(cdata->m_id) + "," + LEX_CAST_STR(cdata->m_total_recharge) + ")");
                        cdata->updateVip();
                    }
#ifdef QQ_PLAT
                    gold_get_tencent(cdata,gold);
#endif
                }
                break;
            case item_type_ling:    //����
                it->nums = cdata->chenmiGet(it->nums);
                cdata->addLing(it->nums);
                cdata->NotifyCharData();
                ++total_get;
                //����ͳ��
                if (get_statistics_type(it->type,statistics_type))
                    add_statistics_of_ling_cost(cdata->m_id,cdata->m_ip_address,it->nums,get_statistics_type(it->type,statistics_type),1, cdata->m_union_id, cdata->m_server_id);
                break;
            case item_type_zhen:    //����
            {
                int level = cdata->m_zhens.Levelup(it->id, it->nums);
                if (level > 0)
                {
                    ++total_get;
                    it->nums = level;
                }
                else
                {
                    it->type = item_type_silver;
                    it->nums = 100;
                    lootMgr::getInstance()->getWorldItemFall(*it);
                    continue;
                }
                break;
            }
            case item_type_skill:    //����
            {
                int level = cdata->skillLevelup(it->id, it->nums);
                if (level > 0)
                {
                    ++total_get;
                    it->nums = level;
                }
                else
                {
                    it->type = item_type_silver;
                    it->nums = 100;
                    lootMgr::getInstance()->getWorldItemFall(*it);
                    continue;
                }
                break;
            }
            case item_type_prestige_level://n*�ȼ�������
            {
                it->type = item_type_prestige;
                it->nums = level * it->nums;
                continue;
            }
            //����
            case item_type_prestige:
            {
                it->nums = cdata->chenmiGet(it->nums);
                cdata->addPrestige(it->nums);
                corpsMgr::getInstance()->addEvent(cdata, corps_event_add_exp, it->nums, 0);
                ++total_get;
                //�������ͳ��
                if (get_statistics_type(it->type,statistics_type))
                    add_statistics_of_prestige_get(cdata->m_id,cdata->m_ip_address,it->nums,get_statistics_type(it->type,statistics_type), cdata->m_union_id, cdata->m_server_id);
                break;
            }
            //Ӣ�۱���
            case item_type_general_baowu:
            {
                for (int i = 0; i < it->nums; ++i)
                {
                    std::string general_name = "";
                    std::string baowu_name = "";
                    if (cdata->m_generals.UpdateTreasure(general_name, baowu_name) == HC_SUCCESS)
                    {
                        if (robj || pCombat)
                        {
                            std::string msg = strBuyGeneralTreasure;
                            str_replace(msg, "$G", general_name);
                            str_replace(msg, "$T", baowu_name);
                            if (extraMsg != "")
                            {
                                extraMsg = extraMsg + "\n" + msg;
                            }
                            else
                            {
                                extraMsg = msg;
                            }
                        }
                    }
                    ++total_get;
                }
                break;
            }
            //��ʯ��Ŀǰֻ֧��÷����������
            case item_type_baoshi:
            {
                if (it->fac == 0)
                    it->fac = 1;
                if (it->nums == 0)
                {
                    it->nums = 1;
                }
                for (int i = 0; i < it->nums; ++i)
                {
                    it->guid = cdata->giveBaoshi(it->id, it->fac, baoshi_give_loot + statistics_type);
                    ++total_get;
                }
                break;
            }
            //��������
            //case item_type_exp:
            //{
            //    cdata->addExp(it->nums);
            //    getobj.push_back( Pair("exp", it->nums) );
            //    ++total_get;
            //    break;
            //}
            case item_type_libao:
            {
                cdata->addLibao(it->id, it->nums);
                ++total_get;
                break;
            }
        }
        it->toObj(getobj);
        getlist.push_back(getobj);
        ++it;
    }
    if (pCombat)
    {
        if (isAttacker)
        {
            pCombat->m_result_obj.push_back( Pair("get", getlist) );
            if (extraMsg != "")
            {
                pCombat->m_result_obj.push_back( Pair("msg", extraMsg) );
            }
        }
        else
        {
            pCombat->m_result_obj.push_back( Pair("get2", getlist) );
            if (extraMsg != "")
            {
                pCombat->m_result_obj.push_back( Pair("msg2", extraMsg) );
            }
        }
    }
    else if (robj)
    {
        if (give_lottery == statistics_type)
        {
            if (extraMsg != "")
            {
                robj->push_back( Pair("msg", extraMsg) );
            }
        }
        else if (isAttacker)
        {
            robj->push_back( Pair("get", getlist) );
            if (extraMsg != "")
            {
                robj->push_back( Pair("msg", extraMsg) );
            }
        }
        else
        {
            robj->push_back( Pair("get2", getlist) );
            if (extraMsg != "")
            {
                robj->push_back( Pair("msg2", extraMsg) );
            }
        }
    }
    return total_get;
}

int giveLoots(boost::shared_ptr<CharData>& cdata, Combat* pCombat, bool isAttacker, int statistics_type)
{
    if (isAttacker)
    {
        return giveLoots(cdata.get(), pCombat->m_getItems, pCombat->m_mapid, pCombat->m_stronghold_level, pCombat->m_stronghold_type, pCombat, NULL, isAttacker, statistics_type);
    }
    else
    {
        return giveLoots(cdata.get(), pCombat->m_getItems2, pCombat->m_mapid, pCombat->m_stronghold_level, pCombat->m_stronghold_type, pCombat, NULL, isAttacker, statistics_type);
    }
}

//��ɫ��¼
int ProcessCharLogin(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj)
{
    //�˺ŵ�¼״̬
    boost::shared_ptr<OnlineUser> m_ponlineuser = psession->user();
    if (!m_ponlineuser.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    uint64_t char_id = 0;
    READ_UINT64_FROM_MOBJ(char_id, o, "id");

    bool find_char = false;
    std::list<CharactorInfo>::iterator it = m_ponlineuser->m_charactorlist.begin();
    while (it != m_ponlineuser->m_charactorlist.end())
    {
        if (it->m_state == 0 && it->m_cid == char_id)
        {
            find_char = true;
            break;
        }
        ++it;
    }
    if (!find_char)
    {
        return HC_ERROR;
    }

    int ret = m_ponlineuser->Login(char_id);
    if (HC_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        CharData* cdata = m_ponlineuser->m_onlineCharactor->m_charactor.get();
        if (!cdata)
        {
            ERR();
            return HC_ERROR;
        }
        //��� VIP4�
        //cdata->startTmpVIP2();

        //�����ɫ�ȼ���Ϣ���ƽ𡢰���������
        retObj.push_back( Pair("cmd", "roleLogin"));
        //Object charobj;
        //cdata->getCharInfo(charobj);
        //retObj.push_back( Pair("chardata", charobj));
        //��ť��Ϣ
        //Object panelobj;
        //cdata->getPanelInfo(charobj);
        if (cdata->m_copy_id_leave)
        {
            boost::shared_ptr<spls_boss> boss = bossMgr::getInstance()->getBoss(cdata->m_copy_id_leave, cdata->m_id);
            if (boss.get() && boss->_boss._open_state > 0)
            {
                json_spirit::Object bossInfo;
                bossInfo.push_back( Pair("id", cdata->m_copy_id_leave) );
                bossInfo.push_back( Pair("spic", boss->_boss._spic) );
                retObj.push_back( Pair("boss", bossInfo) );
            }
        }
        //retObj.push_back( Pair("panel", panelobj));
        retObj.push_back( Pair("s", HC_SUCCESS));

        //cout<<"char login cid "<<cdata->m_id<<",current guide "<<cdata->m_current_guide<<endl;
        if (cdata->m_current_guide)
        {
            cdata->_checkGuide(cdata->m_current_guide);
        }
        //��������
        //cdata->checkGuide(guide_type_login, 0, 0);
        //cdata->checkGuide(guide_type_choose_camp, 0, 0);

        //��������
        //cdata->checkGuide(guide_type_char_level, cdata->m_level, 0);

        psession->send(write(retObj));
        //֪ͨ��ɫ�佫��Ϣ
        cdata->NotifyZhenData();

#ifdef QQ_PLAT
        //cout<<"login:"<<m_ponlineuser->m_feedid<<","<<m_ponlineuser->m_login_str2<<","<<m_ponlineuser->m_iopenid<<","<<m_ponlineuser->m_server_id<<endl;
        //�ٻصĺ���
        if ("4" == m_ponlineuser->m_feedid && m_ponlineuser->m_login_str2 == "feed_4")
        {
            Query q(GetDb());
            q.get_result("select c.id from charactors as c left join accounts as a on c.account=a.account where a.qid='"
                + GetDb().safestr(m_ponlineuser->m_iopenid) + "' and a.server_id='" + GetDb().safestr(m_ponlineuser->m_server_id) + "'");
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                int ifcid = q.getval();
                q.free_result();

                Singleton<inviteMgr>::Instance().add_recall(ifcid, char_id);
            }
            else
            {
                q.free_result();
            }
        }
#endif

        return HC_SUCCESS_NO_RET;
    }
}

//��ɫ�ǳ����ص���ɫѡ�����
int ProcessLogout(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj)
{
    //�˺ŵ�¼״̬
    boost::shared_ptr<OnlineUser> account = psession->user();
    if (!account.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (account->m_onlineCharactor.get())
    {
        GeneralDataMgr::getInstance()->CharactorLogout(account->m_onlineCharactor);
    }
    account->m_onlineCharactor.reset();

    return HC_SUCCESS;
}

//��ʾ��ɫ�б�
int ProcessCharList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (!paccount.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (paccount->m_state == 0)
    {
        INFO("load account ... "<<endl);
        dbCmd _dbcmd;
        _dbcmd._account = paccount->m_account;
        _dbcmd._cmd = db_cmd_load_account;
        InsertDbCharCmd(_dbcmd);
        //�����ɫ��Ϣ
        return HC_SUCCESS_NO_RET;
    }
    json_spirit::Array char_array;
    std::list<CharactorInfo>::iterator it = paccount->m_charactorlist.begin();
    while (it != paccount->m_charactorlist.end())
    {
        if (it->m_state == 0 || it->m_deleteTime > time(NULL))
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->m_cid));
            obj.push_back( Pair("name", it->m_name));
            obj.push_back( Pair("level", it->m_level));
            obj.push_back( Pair("spic", it->m_spic));
            obj.push_back( Pair("lastlogin", it->m_lastlogin));
            obj.push_back( Pair("state", it->m_state));
            if (it->m_state == 1)
            {
                obj.push_back( Pair("leftTime", it->m_deleteTime - time(NULL)));
            }
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(it->m_cid).get();
            if (pc)
            {
                json_spirit::Object g;
                g.push_back( Pair("id", pc->m_current_guide) );
                g.push_back( Pair("state", pc->getGuideState1(pc->m_current_guide)) );
                obj.push_back( Pair("guide", g) );
            }
            char_array.push_back(obj);
        }
        ++it;
    }
    robj.push_back( Pair("charlist", char_array));
    return HC_SUCCESS;
}

//�����¼��¼�����ݿ�
inline void save_talk_record(boost::shared_ptr<OnlineCharactor>& user, boost::shared_ptr<OnlineCharactor>& to, int ctype, const std::string& msg)
{
    if (!user.get() || !user->m_account.get() || !user->m_charactor.get())
    {
        return;
    }
    if (to.get() && to->m_charactor.get() && to->m_account.get())
    {
        InsertSaveDb("INSERT INTO admin_count_talk (`qid`,`qname`,`union_id`,`server_id`,`to_qid`,`to_qname`,`to_union_id`,`to_server_id`,`input`,`channel`,`content`) VALUES ('"
            + GetDb().safestr(user->m_account->m_qid) + "','"
            + GetDb().safestr(user->m_charactor->m_name) + "','"
            + LEX_CAST_STR(user->m_account->m_union_id) + "','"
            + user->m_account->m_server_id + "','"
            + GetDb().safestr(to->m_account->m_qid) + "','"
            + GetDb().safestr(to->m_charactor->m_name) + "','"
            + LEX_CAST_STR(to->m_account->m_union_id) + "','"
            + to->m_account->m_server_id + "',"
            + "now(), '" + LEX_CAST_STR(ctype) + "','" + GetDb().safestr(msg) + "')");
    }
    else
    {
        InsertSaveDb("INSERT INTO admin_count_talk (`qid`,`qname`,`union_id`,`server_id`,`input`,`channel`,`content`) VALUES ('"
            + GetDb().safestr(user->m_account->m_qid) + "','"
            + GetDb().safestr(user->m_charactor->m_name) + "','"
            + LEX_CAST_STR(user->m_account->m_union_id) + "','"
            + user->m_account->m_server_id + "',"
            + "now(),'" + LEX_CAST_STR(ctype) + "','" + GetDb().safestr(msg) + "')");
    }
}

int ProcessChat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (!paccount.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    boost::shared_ptr<OnlineCharactor> user = paccount->m_onlineCharactor;
    if (!user.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    // ������Ϣ��
    if (user->m_charactor->m_can_chat == false)
    {
        return HC_ERROR_FORBIDEN_CHAT;
    }
    int ctype = 0;
    READ_INT_FROM_MOBJ(ctype, o, "ctype");
    std::string msg = "";
    READ_STR_FROM_MOBJ(msg, o, "m");
    std::string to = "";
    boost::shared_ptr<OnlineCharactor> toChar;
    int ret = HC_SUCCESS;
    switch (ctype)
    {
        case 1: //��ӪƵ��
            ret = user->CampChat(msg);
            break;
        case 2: //����Ƶ��
            ret = user->WorldChat(msg);
            if (HC_ERROR_NEED_MORE_LEVEL == ret)
            {
                std::string msg = strNeedMoreLevel;
                str_replace(msg, "$L", LEX_CAST_STR(iWorldChatLevel));
                robj.push_back( Pair("s", ret) );
                robj.push_back( Pair("msg", msg) );
                ret = HC_ERROR_NO_RET;
            }
            break;
        case 3: //����Ƶ��
            ret = user->GuildChat(msg);
            break;
        case 4: //˽��
            {
                READ_STR_FROM_MOBJ(to,o,"to");
                ret = user->Tell(to, msg, toChar);
            }
            break;
        case 6: //����Ƶ��(��ѽӿ�)
            ret = user->WorldChat(msg, false);
        default:
            ret = HC_ERROR;
    }
    if (ret == HC_SUCCESS)
    {
        //�����¼���浽���ݿ�
        save_talk_record(user, toChar, ctype, msg);
    }
    return ret;
}

//��ʾ��ɫ�佫�б�
int ProcessCharGenerals(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array general_array;
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    int nums_per_page = 0;
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page == 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    CharTotalGenerals& char_generals = pc->GetGenerals();
    std::list<boost::shared_ptr<CharGeneralData> > char_generals_list;
    char_generals_list.clear();
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = char_generals.m_generals.begin();
    while (it != char_generals.m_generals.end())
    {
        if (!it->second.get())
        {
            ++it;
            continue;
        }
        std::list<boost::shared_ptr<CharGeneralData> >::iterator it_c = char_generals_list.begin();
        while (it_c != char_generals_list.end())
        {
            if (!(*it_c).get())
            {
                ++it_c;
                continue;
            }
            if ((*it_c)->m_color < it->second->m_color)
            {
                break;
            }
            else if((*it_c)->m_color == it->second->m_color && (*it_c)->m_level < it->second->m_level)
            {
                break;
            }
            ++it_c;
        }
        char_generals_list.insert(it_c,it->second);
        ++it;
    }
    if (char_generals_list.size())
    {
        std::list<boost::shared_ptr<CharGeneralData> >::iterator itt = char_generals_list.begin();
        while (itt != char_generals_list.end())
        {
            if (!(*itt).get())
            {
                ++itt;
                continue;
            }
            #if 0
            //����װ��ǿ��������佫�б�
            if (type == 3 && ((*itt)->m_equipments.getUsed() == 0))
            {
                ++itt;
                continue;
            }
            #endif
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                boost::shared_ptr<CharGeneralData> gd = (*itt);
                if (gd.get())
                {
                    boost::shared_ptr<GeneralTypeData> base_gd = GeneralDataMgr::getInstance()->GetBaseGeneral(gd->m_gid);
                    boost::shared_ptr<BaseSoldierData> base_sd = GeneralDataMgr::getInstance()->GetBaseSoldier(gd->m_stype);
                    if (base_gd.get() && base_sd.get())
                    {
                        Object generalobj;
                        generalobj.push_back( Pair("id", gd->m_id));
                        generalobj.push_back( Pair("level", gd->m_level));
                        generalobj.push_back( Pair("color", gd->m_color));

                        generalobj.push_back( Pair("name", base_gd->m_name));
                        generalobj.push_back( Pair("spic", base_gd->m_spic));
                        generalobj.push_back( Pair("good_at", base_gd->m_good_at));
                        general_array.push_back(generalobj);
                    }
                }
                else
                {
                    ERR();
                }
            }
            ++itt;
        }
    }

    robj.push_back( Pair("list", general_array));
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("page", pageobj) );
    robj.push_back( Pair("maxNums", pc->m_general_limit) );
    robj.push_back( Pair("nums", pc->m_generals.m_generals.size()) );
    return HC_SUCCESS;
}

//��ʾ��ɫ�����б�
int ProcessCharZhen(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array zhen_array;
    CharZhens& char_zhens = pc->GetZhens();
    std::map<int, boost::shared_ptr<ZhenData> >::iterator it = char_zhens.m_zhens.begin();
    while (it != char_zhens.m_zhens.end())
    {
        boost::shared_ptr<ZhenData> zd = it->second;

        Object zhenobj;
        zhenobj.push_back( Pair("id", zd->m_zhen_type));
        zhenobj.push_back( Pair("level", zd->m_level));
        zhenobj.push_back( Pair("name", zd->m_name));

        zhen_array.push_back(zhenobj);

        ++it;
    }

    robj.push_back( Pair("list", zhen_array));
    robj.push_back( Pair("defaultFormation", char_zhens.m_default_zhen));
    return HC_SUCCESS;
}

//��ʾ�ع��佫��Ϣ
int ProcessCharFiredGenerals(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    //��ý�ɫ�佫����
    json_spirit::Array general_array;
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    int nums_per_page = 0;
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page == 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    CharTotalGenerals& char_generals = pc->GetGenerals();
    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = char_generals.GetFiredGeneralsList().begin();
    while (it != char_generals.m_fired_generals.end())
    {
        ++cur_nums;
        if (cur_nums >= first_nums && cur_nums <= last_nums)
        {
            boost::shared_ptr<CharGeneralData> gd = it->second;
            boost::shared_ptr<GeneralTypeData> base_gd = GeneralDataMgr::getInstance()->GetBaseGeneral(gd->m_gid);
            boost::shared_ptr<BaseSoldierData> base_sd = GeneralDataMgr::getInstance()->GetBaseSoldier(gd->m_stype);

            if (gd.get() && base_gd.get() && base_sd.get())
            {
                Object generalobj;
                generalobj.push_back( Pair("id", gd->m_id));
                generalobj.push_back( Pair("level", gd->m_level));
                generalobj.push_back( Pair("color", gd->m_color));
                generalobj.push_back( Pair("leftTime", gd->m_delete_time - time(NULL)));
                generalobj.push_back( Pair("brave", gd->m_str));

                generalobj.push_back( Pair("base_brave", base_gd->base_str));
                generalobj.push_back( Pair("name", base_gd->m_name));
                generalobj.push_back( Pair("spic", base_gd->m_spic));

                general_array.push_back(generalobj);
            }
            else
            {
                ERR();
            }
        }
        ++it;
    }

    robj.push_back( Pair("list", general_array));
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//��ʾ�����佫
int ProcessZhenGenerals(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    CharZhens& char_zhens = pc->GetZhens();
    int zhenid = 0;
    READ_INT_FROM_MOBJ(zhenid, o, "id");
    if (zhenid == 0)
    {
        zhenid = char_zhens.m_default_zhen;
    }
    robj.push_back( Pair("id", zhenid) );
    int type = 0, sid = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    READ_INT_FROM_MOBJ(sid,o,"sid");

    //�ؿ����⴦��
    if (type == 1 && sid > (pc->m_currentStronghold + 1))
    {
        sid = pc->m_currentStronghold + 1;
    }

    pc->getZhenGeneral(zhenid,robj, type, sid);
    robj.push_back( Pair("attack", pc->getAttack(zhenid)));
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("sid", sid) );
    return HC_SUCCESS;
}

//��ѯ�佫��Ϣ
int ProcessGeneral(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    //����佫����
    int gid = 0, purpose = 0;
    READ_INT_FROM_MOBJ(gid, o, "id");
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    Object generalobj;
    if (purpose == 1)
    {
        CharTotalGenerals& char_generals = pc->GetGenerals();
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = char_generals.m_generals.find(gid);
        if (it != char_generals.m_generals.end()  && it->second.get())
        {
            it->second->toObj(generalobj);
        }
    }
    else if (purpose == 2)
    {
        CharTotalGenerals& char_generals = pc->GetGenerals();
        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = char_generals.m_fired_generals.find(gid);
        if (it != char_generals.m_fired_generals.end() && it->second.get())
        {
            it->second->toObj(generalobj);
        }
    }
    else if (purpose == 3)
    {
        //ȡ�ؿ��佫ʱ��gid��ʾ�佫pos
        int strongholdid = 0;
        READ_INT_FROM_MOBJ(strongholdid, o, "gateid");
        boost::shared_ptr<StrongholdData> sd = GeneralDataMgr::getInstance()->GetStrongholdData(strongholdid);
        if (sd.get())
        {
            if (gid >= 1 && gid <= 9 && sd->m_generals[gid - 1].get())
            {
                boost::shared_ptr<BaseSoldierData> base_sd = GeneralDataMgr::getInstance()->GetBaseSoldier(sd->m_generals[gid - 1]->m_stype);
                if (base_sd.get())
                {
                    Object soldierobj;
                    soldierobj.push_back( Pair("type", base_sd->m_base_type));
                    soldierobj.push_back( Pair("attack_type", base_sd->m_damage_type));
                    soldierobj.push_back( Pair("attack_range", base_sd->m_attack_type));
                    soldierobj.push_back( Pair("name", base_sd->m_name));
                    soldierobj.push_back( Pair("spic", base_sd->m_spic));
                    soldierobj.push_back( Pair("memo", base_sd->m_desc));
                    if (sd->m_generals[gid - 1]->m_speSkill.get())
                    {
                        soldierobj.push_back( Pair("spe_attack_range", sd->m_generals[gid - 1]->m_speSkill->attack_type));
                        soldierobj.push_back( Pair("spe_name", sd->m_generals[gid - 1]->m_speSkill->name));
                        soldierobj.push_back( Pair("spe_memo", sd->m_generals[gid - 1]->m_speSkill->memo));
                    }

                    generalobj.push_back( Pair("soldier", soldierobj));
                }
                else
                {
                    ERR();
                    cout<<"stype:"<<sd->m_generals[gid - 1]->m_stype<<endl;
                }
            }
            else
            {
                ERR();
                cout<<strongholdid<<","<<gid<<endl;
            }
        }
        else
        {
            ERR();
            cout<<strongholdid<<","<<gid<<endl;
        }
    }
    else if(purpose == 4)    //��ѯ�������б�(���������佫)
    {
        //std::list<boost::shared_ptr<officalgenerals> >& p_list = GeneralDataMgr::getInstance()->GetBaseOfficalGenerals();
        //std::list<boost::shared_ptr<officalgenerals> >::iterator it = p_list.begin();
        //while (it != p_list.end())
        //{
        //    if ((*it).get() && (*it)->m_gid == gid)
        //    {
        boost::shared_ptr<GeneralTypeData> base_gd = GeneralDataMgr::getInstance()->GetBaseGeneral(gid);
        if (!base_gd.get())
        {
            return HC_ERROR;
        }
        boost::shared_ptr<BaseSoldierData> base_sd = GeneralDataMgr::getInstance()->GetBaseSoldier(base_gd->m_stype);
        if (!base_sd.get())
        {
            return HC_ERROR;
        }
        generalobj.push_back( Pair("id", gid));
        generalobj.push_back( Pair("level", 1));
        generalobj.push_back( Pair("brave", base_gd->base_str));
        generalobj.push_back( Pair("wisdom", base_gd->base_int));
        generalobj.push_back( Pair("govern", base_gd->base_tongyu));
        generalobj.push_back( Pair("color", base_gd->m_quality));
        generalobj.push_back( Pair("nickname", base_gd->m_nickname));
        generalobj.push_back( Pair("growRate", base_gd->m_base_chengzhang));
        generalobj.push_back( Pair("growRate_max", iChengZhangMax[base_gd->m_quality]));

        base_gd->toObj(generalobj);
        base_sd->addObj("soldier", generalobj);

        Object treasure;
        if (base_gd->m_baowu_type != 0)
        {
            treasure.push_back( Pair("name", base_gd->m_baowu));
            treasure.push_back( Pair("level", 0));
            treasure.push_back( Pair("add", 0));
            treasure.push_back( Pair("type", base_gd->m_baowu_type));
        }
        generalobj.push_back( Pair("treasure", treasure));
        generalobj.push_back( Pair("good_at", base_gd->m_good_at));
        //        break;
        //    }
        //    ++it;
        //}
    }
    else if (5 == purpose)    //��ѯ�佫��Ϣ�����Բ���˵�
    {
        int cid = GeneralDataMgr::getInstance()->getGeneralOwner(gid);
        if (cid)
        {
            CharData* pcc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (pcc)
            {
                CharTotalGenerals& char_generals = pcc->GetGenerals();
                std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = char_generals.m_generals.find(gid);
                if (it != char_generals.m_generals.end() && it->second.get())
                {
                    it->second->toObj(generalobj);
                }
                else
                {
                    //�����б��Ҳ��������һع����佫
                    it = char_generals.m_fired_generals.find(gid);
                    if (it != char_generals.m_fired_generals.end() && it->second.get())
                    {
                        it->second->toObj(generalobj);
                    }
                }
            }
        }
        else
        {
            return HC_ERROR;
        }
    }
    else if (purpose == 6)
    {
        //ȡ��Ӣ�ؿ��佫ʱ��gid��ʾ�佫pos
        int strongholdid = 0;
        READ_INT_FROM_MOBJ(strongholdid, o, "gateid");
        boost::shared_ptr<eliteCombat> sd = eliteCombatMgr::getInstance()->getEliteCombatById(strongholdid);
        if (sd.get())
        {
            if (gid >= 1 && gid <= 9 && sd->m_generals[gid - 1].get())
            {
                boost::shared_ptr<BaseSoldierData> base_sd = GeneralDataMgr::getInstance()->GetBaseSoldier(sd->m_generals[gid - 1]->m_stype);
                if (base_sd.get())
                {
                    Object soldierobj;
                    soldierobj.push_back( Pair("type", base_sd->m_base_type));
                    soldierobj.push_back( Pair("attack_type", base_sd->m_damage_type));
                    soldierobj.push_back( Pair("attack_range", base_sd->m_attack_type));
                    soldierobj.push_back( Pair("name", base_sd->m_name));
                    soldierobj.push_back( Pair("spic", base_sd->m_spic));
                    soldierobj.push_back( Pair("memo", base_sd->m_desc));
                    if (sd->m_generals[gid - 1]->m_speSkill.get())
                    {
                        soldierobj.push_back( Pair("spe_attack_range", sd->m_generals[gid - 1]->m_speSkill->attack_type));
                        soldierobj.push_back( Pair("spe_name", sd->m_generals[gid - 1]->m_speSkill->name));
                        soldierobj.push_back( Pair("spe_memo", sd->m_generals[gid - 1]->m_speSkill->memo));
                    }

                    generalobj.push_back( Pair("soldier", soldierobj));
                }
                else
                {
                    ERR();
                    cout<<"stype:"<<sd->m_generals[gid - 1]->m_stype<<endl;
                }
            }
            else
            {
                ERR();
                cout<<strongholdid<<","<<gid<<endl;
            }
        }
        else
        {
            ERR();
            cout<<strongholdid<<","<<gid<<endl;
        }
    }
    robj.push_back( Pair("info", generalobj));
    robj.push_back( Pair("purpose", purpose));
    return HC_SUCCESS;
}

//Ӣ�۽�ͻع�����
int ProcessGeneralChange(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int gid = 0, purpose = 0;
    READ_INT_FROM_MOBJ(gid, o, "id");
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    CharTotalGenerals& char_generals = pc->GetGenerals();
    robj.push_back( Pair("purpose", purpose));
    if (purpose == 1)
    {
        return char_generals.Fire(gid);
    }
    else if (purpose == 2)
    {
        return char_generals.Buyback(gid);
    }
    else if (purpose == 3)//����
    {
        if (pc->m_rebornOpen)
        {
            int fast = 0;
            READ_INT_FROM_MOBJ(fast,o,"fast");
            robj.push_back( Pair("fast", fast));
            return char_generals.Reborn(gid, robj, fast);
        }
        else
        {
            return HC_ERROR_NEED_MORE_LEVEL;
        }
    }
    else if (purpose == 4)//�ָ�
    {
        return char_generals.Recover(gid,robj);
    }
    return HC_ERROR;
}

//�Խ�ɫ���Ͳ���
int ProcessZhenGeneralChange(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    CharZhens& char_zhens = pc->GetZhens();
    int zhenid = char_zhens.m_default_zhen;
    int purpose = 0;
    READ_INT_FROM_MOBJ(zhenid, o, "id");
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    //purpose 1:����2:�Ͻ�3:�½�4:����Ĭ����
    switch (purpose)
    {
        case 1:
            {
                int pos1 = 0, pos2= 0, gid = 0;
                READ_INT_FROM_MOBJ(pos1, o, "pos1");
                READ_INT_FROM_MOBJ(pos2, o, "pos2");
                READ_INT_FROM_MOBJ(gid, o, "gid");
                ret = char_zhens.Swap(zhenid, pos1, pos2);
                //actͳ��
                act_to_tencent(pc,act_new_zhen_general_change);
            }
            break;
        case 2:
            {
                int pos1 = 0, gid = 0;
                READ_INT_FROM_MOBJ(pos1, o, "pos1");
                READ_INT_FROM_MOBJ(gid, o, "gid");
                ret = char_zhens.Up(zhenid, pos1, gid);
                //actͳ��
                act_to_tencent(pc,act_new_zhen_general_change);
            }
            break;
        case 3:
            {
                int pos1 = 0;
                READ_INT_FROM_MOBJ(pos1, o, "pos1");
                ret = char_zhens.Down(zhenid, pos1);
                //actͳ��
                act_to_tencent(pc,act_new_zhen_general_change);
            }
            break;
        case 4:
            //actͳ��
            act_to_tencent(pc,act_new_zhen_change);
            ret = char_zhens.SetDefault(zhenid);
            break;
        default:
            ret = HC_ERROR;
            break;
    }
    if (HC_SUCCESS == ret)
    {
        pc->NotifyZhenData();
        robj.push_back( Pair("attack", pc->getAttack(zhenid)));
    }
    robj.push_back( Pair("purpose", purpose));
    return ret;
}

//�ı���Һ�������
int ProcessCharChat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    robj.push_back( Pair("chat", pc->m_chat));
    return HC_SUCCESS;
}

//�ı���Һ�������
int ProcessCharChatChange(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_vip < 3)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    std::string chat = "";
    READ_STR_FROM_MOBJ(chat, o, "chat");
    if (chat != "")
    {
        pc->m_chat = chat;
        InsertSaveDb("update char_data set chat='" + GetDb().safestr(pc->m_chat) + "' where cid=" + LEX_CAST_STR(pc->m_id));
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

//ս��ϵͳ��ѯ
int ProcessGetAttackPower(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    CharZhens& char_zhens = pc->GetZhens();
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    //0�ۺ�1�佫2װ��3�ط�4ϴ��5ս��6��ʯ7����8����9�佫�ȼ�
    boost::shared_ptr<ZhenData> zdata = char_zhens.GetZhen(char_zhens.GetDefault());
    if (!zdata.get())
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("purpose", purpose));
    switch (purpose)
    {
        case 0:
            {
                Array list;
                Object o;
                //�佫
                {
                    o.clear();
                    o.push_back( Pair("type", 1));
                    o.push_back( Pair("score", zdata->m_general_score));
                    list.push_back(o);
                }
                //װ��
                {
                    o.clear();
                    o.push_back( Pair("type", 2));
                    o.push_back( Pair("score", zdata->m_equip_score));
                    list.push_back(o);
                }
                //�ط�
                {
                    o.clear();
                    o.push_back( Pair("type", 3));
                    if (pc->m_weaponOpen)
                    {
                        o.push_back( Pair("score", pc->m_new_weapons.getNewScore()));
                    }
                    else
                    {
                        o.push_back( Pair("level", newWeaponMgr::getInstance()->openStronghold()/2+1));
                    }
                    list.push_back(o);
                }
                //ϴ��
                {
                    o.clear();
                    o.push_back( Pair("type", 4));
                    if (pc->m_washOpen)
                    {
                        o.push_back( Pair("score", zdata->m_wash_score));
                    }
                    else
                    {
                        o.push_back( Pair("level", iWashOpenStronghold/2+1));
                    }
                    list.push_back(o);
                }
                //ս��
                {
                    o.clear();
                    o.push_back( Pair("type", 5));
                    if (pc->m_horseOpen)
                    {
                        o.push_back( Pair("score", pc->m_horse.getNewScore()));
                    }
                    else
                    {
                        o.push_back( Pair("level", iHorseOpenStronghold/2+1));
                    }
                    list.push_back(o);
                }
                //��ʯ
                {
                    o.clear();
                    o.push_back( Pair("type", 6));
                    if (pc->m_level > 28)
                    {
                        o.push_back( Pair("score", zdata->m_baoshi_score));
                    }
                    else
                    {
                        o.push_back( Pair("level", 28));
                    }
                    list.push_back(o);
                }
                //����
                {
                    o.clear();
                    o.push_back( Pair("type", 7));
                    if (pc->m_rebornOpen)
                    {
                        o.push_back( Pair("score", zdata->m_reborn_score));
                    }
                    else
                    {
                        o.push_back( Pair("level", iRebornOpenStronghold/2+1));
                    }
                    list.push_back(o);
                }
                //����
                {
                    o.clear();
                    o.push_back( Pair("type", 8));
                    if (pc->m_soulOpen)
                    {
                        boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(*pc);
                        if (ct.get())
                        {
                            o.push_back( Pair("score", ct->getNewScore()));
                        }
                    }
                    else
                    {
                        o.push_back( Pair("level", Singleton<trainingMgr>::Instance().openLevel()));
                    }
                    list.push_back(o);
                }
                //�ȼ�
                {
                    o.clear();
                    o.push_back( Pair("type", 9));
                    o.push_back( Pair("score", zdata->m_level_score));
                    list.push_back(o);
                }
                robj.push_back( Pair("list", list));
            }
            break;
        case 1:
            {
                robj.push_back( Pair("score", zdata->m_general_score));
                robj.push_back( Pair("power", zdata->m_general_power));
                json_spirit::Array generallist;
                CharTotalGenerals& char_generals = pc->GetGenerals();
                {
                    for (size_t i = 0; i < 9; ++i)
                    {
                        std::map<int, boost::shared_ptr<CharGeneralData> >::iterator itd = char_generals.m_generals.find(zdata->m_generals[i]);
                        if (itd != char_generals.m_generals.end())
                        {
                            boost::shared_ptr<CharGeneralData> gd = itd->second;
                            boost::shared_ptr<GeneralTypeData> base_gd = GeneralDataMgr::getInstance()->GetBaseGeneral(gd->m_gid);
                            if (gd.get() && base_gd.get())
                            {
                                json_spirit::Object generalobj;
                                generalobj.push_back( Pair("id", gd->m_id));
                                generalobj.push_back( Pair("level", gd->m_level));
                                generalobj.push_back( Pair("color", gd->m_color));

                                json_spirit::Object tianfu;
                                tianfu.push_back( Pair("name", base_gd->m_new_tianfu.m_name) );
                                tianfu.push_back( Pair("memo", base_gd->m_new_tianfu.m_memo) );
                                generalobj.push_back( Pair("tianfu", tianfu) );

                                generalobj.push_back( Pair("total", base_gd->base_str+base_gd->base_int+base_gd->base_tongyu));
                                generalobj.push_back( Pair("name", base_gd->m_name));
                                generalobj.push_back( Pair("spic", base_gd->m_spic));
                                //�ó�
                                generalobj.push_back( Pair("good_at", base_gd->m_good_at) );

                                if (base_gd->m_speSkill.get())
                                {
                                    generalobj.push_back( Pair("spe_name", base_gd->m_speSkill->name));
                                }
                                generallist.push_back(generalobj);
                            }
                        }
                    }
                }
                robj.push_back( Pair("list", generallist));
            }
            break;
        case 2:
            {
                robj.push_back( Pair("score", zdata->m_equip_score));
                robj.push_back( Pair("power", zdata->m_equip_power));
            }
            break;
        case 3:
            {
                robj.push_back( Pair("score", pc->m_new_weapons.getNewScore()));
                robj.push_back( Pair("power", zdata->m_weapon_power));
            }
            break;
        case 4:
            {
                robj.push_back( Pair("score", zdata->m_wash_score));
                robj.push_back( Pair("power", zdata->m_wash_power));
            }
            break;
        case 5:
            {
                robj.push_back( Pair("score", pc->m_horse.getNewScore()));
                robj.push_back( Pair("power", pc->m_horse.getNewPower()));
            }
            break;
        case 6:
            {
                robj.push_back( Pair("score", zdata->m_baoshi_score));
                robj.push_back( Pair("power", zdata->m_baoshi_power));
            }
            break;
        case 7:
            {
                robj.push_back( Pair("score", zdata->m_reborn_score));
                robj.push_back( Pair("power", zdata->m_reborn_power));
            }
            break;
        case 8:
            {
                boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(*pc);
                if (ct.get())
                {
                    robj.push_back( Pair("score", ct->getNewScore()));
                }
                robj.push_back( Pair("power", zdata->m_soul_power));
            }
            break;
        case 9:
            {
                robj.push_back( Pair("score", zdata->m_level_score));
            }
            break;
        default:
            break;
    }
    return HC_SUCCESS;
}

//��ʾ��ҵ�ͼ��������
int ProcessCharMapTempo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    CharTempoData& char_tempo = pc->GetTempo();
    int mapid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "id");

    if (mapid > pc->m_area)
    {
        if (pc->m_area >= gMaxOpenMap)
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(pc->m_area + 1);
        if (!bm.get() || bm->openLevel == 0)
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        //ͨ�ز��ܽ�����һͼ
        if (pc->isMapPassed(pc->m_area))
        {
            ++pc->m_area;
            pc->m_cur_stage = 1;
            pc->notifyChangeMap();
            pc->m_tempo.InitCharTempo(pc->m_area);
            pc->Save();
            //��������
            pc->updateTask(task_enter_map, pc->m_area);

            //��������
            pc->checkGuide(guide_type_enter_map, pc->m_area, 0);
            //cout<<"auto move to next map..."<<endl;
        }
        else
        {
            //cout<<"!!! auto move to next map fail..."<<endl;
            return HC_ERROR_MAP_NOT_OPEN;
        }
    }

    json_spirit::Array maptempo;
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = char_tempo.CharMapsData.find(mapid);
    if (it != char_tempo.CharMapsData.end())
    {
        boost::shared_ptr<CharMapData> md = it->second;
        CharMapData::iterator itm = (*md).begin();
        int pre_state = 0;
        while (itm != (*md).end())
        {
            if (itm->second.get() && itm->second->m_baseStage.get())
            {
                Object obj;
                obj.push_back( Pair("id", itm->first));
                obj.push_back( Pair("name", itm->second->m_baseStage->name));
                if (itm->second->m_stronghold[0].get() && itm->second->m_stronghold[itm->second->m_baseStage->size - 1].get())
                {
                    if (itm->second->m_stronghold[itm->second->m_baseStage->size - 1]->m_state > 0)
                    {
                        obj.push_back( Pair("state", 2));
                        pre_state = 2;
                    }
                    else if (itm->second->m_stronghold[0]->m_state > -1)
                    {
                        obj.push_back( Pair("state", 1));
                        pre_state = 1;
                    }
                    else
                    {
                        obj.push_back( Pair("state", 0));
                        pre_state = 0;
                    }
                }
                else
                {
                    obj.push_back( Pair("state", 0));
                    pre_state = 0;
                }
                obj.push_back( Pair("cur_gate", itm->second->m_finished));
                obj.push_back( Pair("total_gate", itm->second->m_baseStage->size));
                maptempo.push_back(obj);
            }
            ++itm;
        }

#if 1
        //�����ط������
        if (pc->isMapPassed(mapid))
        {
            //����ͼ�Ѿ�ͨ����
            Object obj;
            groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(mapid);
            if (pCopy)
            {
                obj.push_back( Pair("id", pCopy->_id));
                obj.push_back( Pair("name", pCopy->_name));
                obj.push_back( Pair("state", 1));
                obj.push_back( Pair("type", 1));
                maptempo.push_back(obj);
            }
        }
#endif
        //��������
        pc->checkGuide(guide_type_view_map, mapid, 0);
    }
    Object area;
    std::string name = "";
    std::string memo = "";
    ret = GeneralDataMgr::getInstance()->GetMapMemo(mapid,name,memo);
    if (ret == 0)
    {
        area.push_back( Pair("area", mapid));
        area.push_back( Pair("name", name));
        area.push_back( Pair("memo", memo));
    }
    if (mapid < gMaxOpenMap && pc->isMapPassed(mapid))
    {
        Object next_area;
        ret = GeneralDataMgr::getInstance()->GetMapMemo(mapid + 1,name,memo);
        if (ret == 0)
        {
            next_area.push_back( Pair("area", mapid + 1));
            next_area.push_back( Pair("name", name));
            next_area.push_back( Pair("memo", memo));
        }
        robj.push_back( Pair("nextArea", next_area));
    }
    if (mapid > 1)
    {
        Object pre_area;
        ret = GeneralDataMgr::getInstance()->GetMapMemo(mapid - 1,name,memo);
        if (ret == 0)
        {
            pre_area.push_back( Pair("area", mapid - 1));
            pre_area.push_back( Pair("name", name));
            pre_area.push_back( Pair("memo", memo));
        }
        robj.push_back( Pair("preArea", pre_area));
    }
    robj.push_back( Pair("list", maptempo));
    robj.push_back( Pair("area", area));
    robj.push_back( Pair("mapid", mapid));
    return HC_SUCCESS;
}

//��ʾ��ҵ���������Ϣ
int ProcessCharStage(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    CharTempoData& char_tempo = pc->GetTempo();
    int mapid = 0, stageid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(stageid, o, "stageid");
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = char_tempo.CharMapsData.find(mapid);
    Object info;
    if (it != char_tempo.CharMapsData.end() && it->second.get())
    {
        boost::shared_ptr<CharMapData> md = it->second;
        CharMapData::iterator itm = (*md).find(stageid);
        if (itm != (*md).end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            info.push_back( Pair("id", itm->first));
            info.push_back( Pair("mapid", mapid));
            info.push_back( Pair("name", itm->second->m_baseStage->name));
            if (itm->second->m_stronghold[0].get())
            {
                info.push_back( Pair("isOpen", itm->second->m_stronghold[0]->m_state > -1));
            }
            else
            {
                info.push_back( Pair("isOpen", false));
            }
            info.push_back( Pair("cur_gate", itm->second->m_finished));
            int stage_size = itm->second->m_baseStage->size;
            info.push_back( Pair("total_gate", stage_size));
            info.push_back( Pair("spic", itm->second->m_baseStage->spic));
            int ret = GeneralDataMgr::getInstance()->GetStageLimitLevel(mapid,stageid);
            if (ret != -1)
            {
                info.push_back( Pair("limit", ret));
            }
            if (itm->second->m_baseStage->loots_array.get())
            {
                info.push_back( Pair("get", *(itm->second->m_baseStage->loots_array.get())));
            }
            if (itm->second->m_baseStage->_baseStrongholds[stage_size - 1].get())
            {
                info.push_back( Pair("general_name", itm->second->m_baseStage->_baseStrongholds[stage_size - 1]->m_name));
            }
            info.push_back( Pair("boss", itm->second->m_baseStage->boss_list) );
        }
    }
    robj.push_back( Pair("info", info));
    return HC_SUCCESS;
}

//��ʾ��ҳ����ؿ�����
int ProcessCharStageTempo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    CharTempoData& char_tempo = pc->GetTempo();
    int mapid = 0, stageid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(stageid, o, "stageid");

    if (mapid > pc->m_area)
    {
        if (pc->m_area >= gMaxOpenMap)
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(pc->m_area + 1);
        if (!bm.get() || bm->openLevel == 0)
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        //ͨ�ز��ܽ�����һͼ
        if (pc->isMapPassed(pc->m_area))
        {
            ++pc->m_area;
            pc->m_cur_stage = 1;
            pc->notifyChangeMap();
            pc->m_tempo.InitCharTempo(pc->m_area);
            pc->Save();
            //��������
            pc->updateTask(task_enter_map, pc->m_area);

            //��������
            pc->checkGuide(guide_type_enter_map, pc->m_area, 0);
            //cout<<"auto move to next map..."<<endl;
        }
        else
        {
            //cout<<"!!! auto move to next map fail..."<<endl;
            return HC_ERROR_MAP_NOT_OPEN;
        }
    }

    json_spirit::Array stronghold_array;
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = char_tempo.CharMapsData.find(mapid);
    if (it != char_tempo.CharMapsData.end())
    {
        boost::shared_ptr<CharMapData> md = it->second;
        CharMapData::iterator itm = (*md).find(stageid);
        if (itm != (*md).end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            //��������
            pc->updateTask(task_enter_stage, mapid, stageid);
            for (size_t i = 0; i < (size_t)itm->second->m_baseStage->size; ++i)
            {
                //��ǰ����һ����ʾ���ٺ����ֱ��������
                if (i > 24 || !itm->second->m_stronghold[i].get() || !itm->second->m_stronghold[i]->m_baseStronghold.get()
                    || itm->second->m_stronghold[i]->m_baseStronghold->m_group >= (itm->second->m_cur_group + 2)
                    )
                {
                    continue;
                }
                StrongholdData* shold = itm->second->m_stronghold[i]->m_baseStronghold.get();
                if (shold)
                {
                    Object stronghold;
                    stronghold.push_back( Pair("pos", i + 1));
                    stronghold.push_back( Pair("id", shold->m_id));
                    stronghold.push_back( Pair("name", shold->m_name));
                    stronghold.push_back( Pair("level", shold->m_level));
                    stronghold.push_back( Pair("elite", shold->m_isepic));
                    stronghold.push_back( Pair("model", shold->m_model));
                    stronghold.push_back( Pair("color", shold->m_color));
                    stronghold.push_back( Pair("x", shold->m_x));
                    stronghold.push_back( Pair("y", shold->m_y));
                    stronghold.push_back( Pair("state", itm->second->m_stronghold[i]->m_state));
                    stronghold.push_back( Pair("group", itm->second->m_stronghold[i]->m_baseStronghold->m_group));
                    stronghold_array.push_back(stronghold);
                }
            }
            robj.push_back( Pair("group", itm->second->m_cur_group));
        }
    }
    robj.push_back( Pair("list", stronghold_array));
    robj.push_back( Pair("mapid", mapid));
    robj.push_back( Pair("stageid", stageid));
    pc->m_current_map = mapid;
    pc->m_current_stage = mapid;
    return HC_SUCCESS;
}

//��ʾ��ҵ����ؿ���Ϣ
int ProcessCharStronghold(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0, stageid = 0, strongholdpos = 0;
    Object info;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(stageid, o, "stageid");
    READ_INT_FROM_MOBJ(strongholdpos, o, "pos");

    if (mapid > pc->m_area)
    {
        if (pc->m_area >= gMaxOpenMap)
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(pc->m_area + 1);
        if (!bm.get() || bm->openLevel == 0)
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        //ͨ�ز��ܽ�����һͼ
        if (pc->isMapPassed(pc->m_area))
        {
            ++pc->m_area;
            pc->m_cur_stage = 1;
            pc->notifyChangeMap();
            pc->m_tempo.InitCharTempo(pc->m_area);
            pc->Save();
            //��������
            pc->updateTask(task_enter_map, pc->m_area);

            //��������
            pc->checkGuide(guide_type_enter_map, pc->m_area, 0);
            //cout<<"auto move to next map..."<<endl;
        }
        else
        {
            //cout<<"!!! auto move to next map fail..."<<endl;
            return HC_ERROR_MAP_NOT_OPEN;
        }
    }

    boost::shared_ptr<CharStrongholdData> cd = GeneralDataMgr::getInstance()->GetCharStrongholdData(pc->m_id,mapid,stageid,strongholdpos);
    //���Ŀ��ؿ������ܴ��ȴ���Ŀǰ���Դ�Ĺؿ�
    if (!cd.get() || cd->m_state < 0)
    {
        cd = pc->getDestStronghold();
    }
    if (!cd.get())
    {
        ERR();
        cout<<"cid:"<<pc->m_id<<",mapid"<<mapid<<"stage"<<stageid<<"pos"<<strongholdpos<<endl;
        return HC_ERROR;
    }
    boost::shared_ptr<StrongholdData>& sd = cd->m_baseStronghold;
    if (!sd.get())
    {
        ERR();
        return HC_ERROR;
    }

    //�ؿ�����Ϣ
    info.push_back( Pair("pos", sd->m_strongholdpos));
    info.push_back( Pair("id", sd->m_id));
    info.push_back( Pair("name", sd->m_name));
    info.push_back( Pair("level", sd->m_level));
    info.push_back( Pair("elite", sd->m_isepic));
    info.push_back( Pair("spic", sd->m_spic));
    info.push_back( Pair("state", cd->m_state));

    //������Ϣ
    if (sd->m_loot.get() && sd->m_Item.get())
    {
        if (sd->m_Item->type == item_type_skill)
        {
            boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(sd->m_Item->id);
            if (bs.get())
            {
                json_spirit::Object getobj;
                json_spirit::Object skill;
                skill.push_back( Pair("id", sd->m_Item->id));
                skill.push_back( Pair("name", bs->name));
                skill.push_back( Pair("level", sd->m_Item->nums));
                skill.push_back( Pair("spic", sd->m_Item->id));
                skill.push_back( Pair("curlevel", pc->getSkillLevel(sd->m_Item->id)));
                getobj.push_back( Pair("skill", skill));
                info.push_back( Pair("get", getobj));
            }
        }
        else if (sd->m_Item->type == item_type_zhen)
        {
            boost::shared_ptr<BaseZhenData> bz = GeneralDataMgr::getInstance()->GetBaseZhen(sd->m_Item->id);
            if (bz.get())
            {
                json_spirit::Object getobj;
                json_spirit::Object zhen;
                zhen.push_back( Pair("type", sd->m_Item->id));
                zhen.push_back( Pair("name", bz->m_name));
                zhen.push_back( Pair("level", 5));
                boost::shared_ptr<ZhenData> zd = pc->m_zhens.GetZhen(sd->m_Item->id);
                zhen.push_back( Pair("curlevel", zd.get() ? zd->m_level : 0));
                getobj.push_back( Pair("zhen", zhen));
                info.push_back( Pair("get", getobj));
            }
        }
        else
        {
            info.push_back( Pair("get", *(sd->m_loot.get())));
        }
    }

    int pos[9];
    memset(pos, 0, sizeof(int)*9);
    boost::shared_ptr<ZhenData> zdata = pc->m_zhens.GetZhen(pc->m_zhens.m_default_zhen);
    if (zdata.get())
    {
        for (int i = 0; i < 9; ++i)
        {
            if (zdata->m_generals[i] > 0)
            {
                pos[i] = 1;
            }
        }
    }

    //�ؿ��佫��Ϣ
    json_spirit::Array generallist;
    for (size_t i = 0; i < 9; ++i)
    {
        Object generalobj;
        if (sd->m_generals[i].get())
        {
            generalobj.push_back( Pair("spic", sd->m_generals[i]->m_spic));
            generalobj.push_back( Pair("color", sd->m_generals[i]->m_color));
            generalobj.push_back( Pair("name", sd->m_generals[i]->m_name));
            generalobj.push_back( Pair("level", sd->m_level));

            if (sd->m_generals[i]->m_speSkill.get())
            {
                generalobj.push_back( Pair("spe_name",  sd->m_generals[i]->m_speSkill->name));
                generalobj.push_back( Pair("spe_memo",  sd->m_generals[i]->m_speSkill->memo));
            }
            json_spirit::Object soldierobj;
            soldierobj.push_back( Pair("type", sd->m_generals[i]->m_baseSoldier->m_base_type));
            soldierobj.push_back( Pair("attack_type", sd->m_generals[i]->m_baseSoldier->m_damage_type));
            soldierobj.push_back( Pair("name", sd->m_generals[i]->m_baseSoldier->m_name));
            soldierobj.push_back( Pair("memo", sd->m_generals[i]->m_baseSoldier->m_desc));
            generalobj.push_back( Pair("soldier", soldierobj));

            //�ó�
            generalobj.push_back( Pair("good_at", 0) );

            //������Χ...
            if (sd->m_generals[i]->m_speSkill.get() && sd->m_generals[i]->m_speSkill->action_type == 1)
            {
                int atype = sd->m_generals[i]->m_speSkill->attack_type;

                json_spirit::Array v = getAttackRange(atype, i+1, pos);
                generalobj.push_back( Pair("range", v) );
            }

            generalobj.push_back( Pair("position", sd->m_generals[i]->m_pos));
        }
        else
        {
            generalobj.push_back( Pair("position", -2));
        }
        generallist.push_back(generalobj);
    }
    info.push_back( Pair("gate_formation", generallist));

    info.push_back( Pair("attack", sd->getAttack()));

    //��Ҫ�ľ�����
    if (cd->m_state == 0)
    {
        info.push_back( Pair("needSupply", sd->m_need_supply));
    }
    else
    {
        info.push_back( Pair("robSupply", sd->m_rob_supply));
    }

    info.push_back( Pair("mapid", mapid) );
    info.push_back( Pair("stageid", stageid) );
    robj.push_back( Pair("info", info));

    return HC_SUCCESS;
}

//��ʾ�ؿ�����
int ProcessStrongholdRaiders(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    if (id > (pc->m_currentStronghold + 1))
    {
        id = pc->m_currentStronghold + 1;
    }
    robj.push_back( Pair("id", id) );
    boost::shared_ptr<StrongholdData> sd = GeneralDataMgr::getInstance()->GetStrongholdData(id);
    if (sd.get())
    {
        sd->m_raiders.getRadiers(robj);
    }
    return HC_SUCCESS;
}

//��ȡͨ�ؽ���
int ProcessGetStageFinishLoot(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0, stageid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(stageid, o, "stageid");
    return pc->GetTempo().get_stage_finish_loot(mapid,stageid,robj);
}

//��ѯͨ�ؽ���
int ProcessCheckStageFinish(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0, stageid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(stageid, o, "stageid");
    robj.push_back( Pair("mapid", mapid) );
    robj.push_back( Pair("stageid", stageid) );
    robj.push_back( Pair("can_get", pc->GetTempo().check_stage_finish(mapid,stageid)) );
    return HC_SUCCESS;
}

int ProcessGetDropInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int strongholdid = 0;
    READ_INT_FROM_MOBJ(strongholdid, o, "id");
    //������Ϣ
    boost::shared_ptr<StrongholdData> sd = GeneralDataMgr::getInstance()->GetStrongholdData(strongholdid);
    if (sd.get() && sd->m_loot.get() && sd->m_Item.get())
    {
        if (sd->m_Item->type == item_type_skill)
        {
            boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(sd->m_Item->id);
            if (bs.get())
            {
                json_spirit::Object getobj;
                json_spirit::Object skill;
                skill.push_back( Pair("id", sd->m_Item->id));
                skill.push_back( Pair("name", bs->name));
                skill.push_back( Pair("level", sd->m_Item->nums));
                skill.push_back( Pair("spic", sd->m_Item->id));
                skill.push_back( Pair("curlevel", cdata->getSkillLevel(sd->m_Item->id)));
                getobj.push_back( Pair("skill", skill));
                robj.push_back( Pair("get", getobj));
            }
        }
        else if (sd->m_Item->type == item_type_zhen)
        {
            boost::shared_ptr<BaseZhenData> bz = GeneralDataMgr::getInstance()->GetBaseZhen(sd->m_Item->id);
            if (bz.get())
            {
                json_spirit::Object getobj;
                json_spirit::Object zhen;
                zhen.push_back( Pair("id", sd->m_Item->id));
                zhen.push_back( Pair("name", bz->m_name));
                zhen.push_back( Pair("level", 5));
                boost::shared_ptr<ZhenData> zd = cdata->m_zhens.GetZhen(sd->m_Item->id);
                zhen.push_back( Pair("curlevel", zd.get() ? zd->m_level : 0));
                getobj.push_back( Pair("zhen", zhen));
                robj.push_back( Pair("get", getobj));
            }
        }
        else
        {
            robj.push_back( Pair("get", *(sd->m_loot.get())));
        }
    }
    return HC_SUCCESS;
}

//̽��������
int ProcessExploreInterface(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array skill_list;
    json_spirit::Array general_list;
    std::list<boost::shared_ptr<explore_reward> > er_list;
    er_list = exploreMgr::getInstance()->GetExploreReward();
    if (er_list.empty())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<exploreplace_list> has_list = exploreMgr::getInstance()->GetHasExplore(cdata->m_id);
    if (!has_list.get())
    {
        cout << "has_list get fail" << endl;
        return HC_ERROR;
    }
    std::list<boost::shared_ptr<explore_reward> >::iterator it = er_list.begin();
    while (it != er_list.end())
    {
        if ((*it).get() && (*it)->mapid <= cdata->m_area)
        {
            //��������ȼ�������ֱ������
            if ((*it)->type == 1 && (*it)->need > cdata->m_level)
            {
                ++it;
                continue;
            }
            //����������ڲ��Ի���û�и��佫��ֱ������
            if ((*it)->type == 2 && ((*it)->need != GeneralDataMgr::getInstance()->getSeason() || !(cdata->CheckHasGeneral((*it)->id))))
            {
                ++it;
                continue;
            }
            std::string s_combo = "";
            int cnt = 0;
            for (size_t i = 1; i <= (*it)->combo.size(); ++i)
            {
                int N = (*it)->combo.size() - i;//�Ƚϳ��ȴӳ�����
                std::list<int> tmp;
                exploreplace_list::reverse_iterator ite = (*has_list).rbegin();
                while (N != 0 && ite != (*has_list).rend())
                {
                    tmp.push_front(*ite);
                    --N;
                    ++ite;
                }
                if (N != 0)//has_list�������޷�ƥ�䣬�����¸�����
                {
                    continue;
                }
                else
                {
                    std::list<int>::iterator it_tmp = tmp.begin();
                    size_t i_combo = 0;
                    for( ; it_tmp != tmp.end(),i_combo < (*it)->combo.size(); ++it_tmp, ++i_combo)
                    {
                        if((*it)->combo[i_combo] == *it_tmp)//ƥ�䵽�����������
                        {
                            ++cnt;
                        }
                        else if(it_tmp != tmp.end())//ûƥ����tmp�ͽ���������Ч
                        {
                            cnt = 0;
                            break;
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (cnt != 0)//ƥ��ɹ�����ѭ��
                    {
                        break;
                    }
                }
            }
            for (int i = 0; i < (int)(*it)->combo.size(); ++i)
            {
                std::string name = exploreMgr::getInstance()->GetPlaceName((*it)->combo[i]);
                if (i < cnt)
                {
                    //s_combo += ("<font color=\"#DAA520\">" + name + "</font>");
                    if (i == 0)
                    {
                        s_combo += ("<font color=\"#74D3FB\">" + name + "</font>");
                    }
                    else if(i == 1)
                    {
                        s_combo += ("<font color=\"#00FF0C\">" + name + "</font>");
                    }
                    else if(i == 2)
                    {
                        s_combo += ("<font color=\"#ff0000\">" + name + "</font>");
                    }
                }
                else
                {
                    s_combo += name;
                }
                if (i < (int)((*it)->combo.size() - 1))
                {
                    s_combo += "+";
                }
                else
                {
                    s_combo += "=";
                }
            }
            s_combo += exploreMgr::getInstance()->GetRewardName((*it)->type,(*it)->id);
            if ((*it)->type == 1)
            {
                Object skill_reward;
                if (cdata->getSkillLevel((*it)->id) < cdata->m_level / 2)
                {
                    skill_reward.push_back( Pair("can_get", true));
                }
                else
                {
                    skill_reward.push_back( Pair("can_get", false));
                }
                skill_reward.push_back( Pair("combo", s_combo));
                skill_list.push_back(skill_reward);
            }
            else if ((*it)->type == 2)
            {
                Object general_reward;
                int gid = cdata->m_generals.GetGeneralByType((*it)->id);
                general_reward.push_back( Pair("can_get", cdata->m_generals.CheckTreasureCanUp(gid)));
                general_reward.push_back( Pair("combo", s_combo));
                boost::shared_ptr<GeneralTypeData> bgd = GeneralDataMgr::getInstance()->GetBaseGeneral((*it)->id);
                if (bgd.get())
                {
                    general_reward.push_back( Pair("general", bgd->m_name));
                }
                general_list.push_back(general_reward);
            }
        }
        ++it;
    }
    robj.push_back( Pair("list1", skill_list));
    robj.push_back( Pair("list2", general_list));
    json_spirit::Array can_explore_list;
    boost::shared_ptr<exploreplace_list> can_list = exploreMgr::getInstance()->GetCanExplore(cdata->m_id);
    if (!can_list.get())
    {
        cout << "can_list get fail" << endl;
        return HC_ERROR;
    }
    exploreplace_list::iterator ite = (*can_list).begin();
    while (ite != (*can_list).end())
    {
        Object ep;
        boost::shared_ptr<explore_place> tmp = exploreMgr::getInstance()->GetBaseExplorePlace(*ite);
        if (tmp.get())
        {
            ep.push_back( Pair("id", tmp->id));
            ep.push_back( Pair("name", tmp->name));
            ep.push_back( Pair("spic", tmp->spic));
            ep.push_back( Pair("condition", exploreMgr::getInstance()->GetComboPos(cdata->m_id,tmp->id)));
            can_explore_list.push_back(ep);
        }
        ++ite;
    }
    json_spirit::Array has_explore_list;
    ite = (*has_list).begin();
    while (ite != (*has_list).end())
    {
        Object ep;
        boost::shared_ptr<explore_place> tmp = exploreMgr::getInstance()->GetBaseExplorePlace(*ite);
        if (tmp.get())
        {
            ep.push_back( Pair("id", tmp->id));
            ep.push_back( Pair("name", tmp->name));
            ep.push_back( Pair("spic", tmp->spic));
            has_explore_list.push_back(ep);
        }
        ++ite;
    }
    robj.push_back( Pair("list3", can_explore_list));
    robj.push_back( Pair("list4", has_explore_list));
    robj.push_back( Pair("gold", exploreMgr::getInstance()->GetGoldRefreshGold(cdata->m_id)));
    robj.push_back( Pair("refresh_unlimit", cdata->m_vip >= iExploreRefreshUnlimitVIP));
    if (cdata->m_vip < iExploreRefreshUnlimitVIP)
    {
        robj.push_back( Pair("refresh_time", iExploreRefreshTimes[cdata->m_vip] - cdata->m_explore_refresh_times));
    }
    robj.push_back( Pair("ling", cdata->exploreLing()));

#if 0
    int gold_cost = 0;
    int buy_time = cdata->queryExtraData(char_data_type_daily, char_data_buy_explore) + 1;
    for (int i = 0; i < iExploreCostLevel; ++i)
    {
        if (buy_time <= iExploreBuyGold[i][0])
        {
            gold_cost = iExploreBuyGold[i][1];
            break;
        }
    }
    if (gold_cost > iExploreBuyGoldMax)
    {
        gold_cost = iExploreBuyGoldMax;
    }
    robj.push_back( Pair("buyGold", gold_cost));
#endif
    return HC_SUCCESS;
}

//̽��
int ProcessExplore(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int pid = 0;
    READ_INT_FROM_MOBJ(pid, o, "id");
    ret = exploreMgr::getInstance()->Explore(cdata->m_id,pid,robj);
    if (ret == HC_SUCCESS || ret == HC_ERROR_SKILL_LEVEL_MAX || ret == HC_ERROR_GENERALTREASURE_MAX)
    {
        exploreMgr::getInstance()->ExploreRefresh(cdata->m_id);
        //��������
        cdata->updateTask(task_do_explore);

        //�ճ�����
        //dailyTaskMgr::getInstance()->updateDailyTask(*cdata);
    }
    return ret;
}

//����̽��
int ProcessBuyExplore(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return exploreMgr::getInstance()->buyExploreLing(cdata, robj);
}

//̽���¼�������ʾ
int ProcessComboTips(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int pid = 0;
    READ_INT_FROM_MOBJ(pid, o, "pid");
    json_spirit::Array combo_list;

    boost::shared_ptr<exploreplace_list> has_list = exploreMgr::getInstance()->GetHasExplore(cdata->m_id);
    if (!has_list.get())
    {
        return HC_ERROR;
    }
    std::list<boost::shared_ptr<explore_reward> > er_list;
    er_list = exploreMgr::getInstance()->GetExploreReward();
    if (er_list.empty())
    {
        return HC_ERROR;
    }
    std::list<boost::shared_ptr<explore_reward> >::iterator it = er_list.begin();
    while (it != er_list.end())
    {
        if ((*it).get() && (*it)->mapid <= cdata->m_area)
        {
            //��������ȼ�������ֱ������
            if ((*it)->type == 1 && (*it)->need > cdata->m_level)
            {
                ++it;
                continue;
            }
            //����������ڲ��Ի���û�и��佫��ֱ������
            if ((*it)->type == 2 && ((*it)->need != GeneralDataMgr::getInstance()->getSeason() || !(cdata->CheckHasGeneral((*it)->id))))
            {
                ++it;
                continue;
            }
            std::string s_combo = "";
            int cnt = 0;
            for (size_t i = 1; i <= (*it)->combo.size(); ++i)
            {
                int N = (*it)->combo.size() - i;//�Ƚϳ��ȴӳ�����
                std::list<int> tmp;
                tmp.push_front(pid);//�����ָ���pid���뵽�ѷ���ĩβ���бȽ�
                exploreplace_list::reverse_iterator ite = (*has_list).rbegin();
                while (N != 0 && ite != (*has_list).rend())
                {
                    tmp.push_front(*ite);
                    --N;
                    ++ite;
                }
                if (N != 0)//has_list�������޷�ƥ�䣬�����¸�����
                {
                    continue;
                }
                else
                {
                    std::list<int>::iterator it_tmp = tmp.begin();
                    size_t i_combo = 0;
                    for( ; it_tmp != tmp.end(),i_combo < (*it)->combo.size(); ++it_tmp, ++i_combo)
                    {
                        if((*it)->combo[i_combo] == *it_tmp)//ƥ�䵽�����������
                        {
                            ++cnt;
                        }
                        else if(it_tmp != tmp.end())//ûƥ����tmp�ͽ���������Ч
                        {
                            cnt = 0;
                            break;
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (cnt != 0)//ƥ��ɹ�����ѭ��
                    {
                        break;
                    }
                }
            }
            //ƥ��ɹ���Ⱦɫ����cnt-1����ɫcntΪ��ɫ
            if (cnt != 0)
            {
                for (int i = 0; i < (int)(*it)->combo.size(); ++i)
                {
                    std::string name = exploreMgr::getInstance()->GetPlaceName((*it)->combo[i]);
                    /*if (i < (cnt - 1))
                    {
                        s_combo += ("<font color=\"#DAA520\">" + name + "</font>");
                    }*/
                    if(i < cnt)
                    {
                        //s_combo += ("<font color=\"#00FF00\">" + name + "</font>");
                        if (i == 0)
                        {
                            s_combo += ("<font color=\"#74D3FB\">" + name + "</font>");
                        }
                        else if(i == 1)
                        {
                            s_combo += ("<font color=\"#00FF0C\">" + name + "</font>");
                        }
                        else if(i == 2)
                        {
                            s_combo += ("<font color=\"#ff0000\">" + name + "</font>");
                        }
                    }
                    else
                    {
                        s_combo += name;
                    }
                    if (i < (int)((*it)->combo.size() - 1))
                    {
                        s_combo += "+";
                    }
                    else
                    {
                        s_combo += "=";
                    }
                }
                s_combo += exploreMgr::getInstance()->GetRewardName((*it)->type,(*it)->id);
                Object combo_reward;
                combo_reward.push_back( Pair("combo", s_combo));
                combo_list.push_back(combo_reward);
            }
        }
        ++it;
    }
    robj.push_back( Pair("list", combo_list));
    return HC_SUCCESS;
}

int ProcessCheckExploreState(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    boost::shared_ptr<exploreplace_list> has_list = exploreMgr::getInstance()->GetHasExplore(cdata->m_id);
    if (!has_list.get())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<exploreplace_list> can_list = exploreMgr::getInstance()->GetCanExplore(cdata->m_id);
    if (!can_list.get())
    {
        return HC_ERROR;
    }
    std::list<boost::shared_ptr<explore_reward> > er_list;
    er_list = exploreMgr::getInstance()->GetExploreReward();
    if (er_list.empty())
    {
        return HC_ERROR;
    }
    exploreplace_list::iterator it_canlist = (*can_list).begin();
    while (it_canlist != (*can_list).end())
    {
        Object ep;
        boost::shared_ptr<explore_place> place = exploreMgr::getInstance()->GetBaseExplorePlace(*it_canlist);
        if (place.get())
        {
            std::list<boost::shared_ptr<explore_reward> >::iterator it = er_list.begin();
            while (it != er_list.end())
            {
                if ((*it).get() && (*it)->mapid <= cdata->m_area)
                {
                    //��������ȼ�������ֱ������
                    if ((*it)->type == 1 && (*it)->need > cdata->m_level)
                    {
                        ++it;
                        continue;
                    }
                    //����������ڲ��Ի���û�и��佫��ֱ������
                    if ((*it)->type == 2 && ((*it)->need != GeneralDataMgr::getInstance()->getSeason() || !(cdata->CheckHasGeneral((*it)->id))))
                    {
                        ++it;
                        continue;
                    }
                    int cnt = 0;
                    int N = (*it)->combo.size() - 1;
                    std::list<int> tmp;
                    tmp.push_front(place->id);
                    exploreplace_list::reverse_iterator ite = (*has_list).rbegin();
                    while (N != 0 && ite != (*has_list).rend())
                    {
                        tmp.push_front(*ite);
                        --N;
                        ++ite;
                    }
                    if (N != 0)//has_list�������޷�ƥ�䣬�����¸�����
                    {
                        ++it;
                        continue;
                    }
                    else
                    {
                        std::list<int>::iterator it_tmp = tmp.begin();
                        size_t i_combo = 0;
                        for( ; it_tmp != tmp.end(),i_combo < (*it)->combo.size(); ++it_tmp, ++i_combo)
                        {
                            if((*it)->combo[i_combo] == *it_tmp)//ƥ�䵽�����������
                            {
                                ++cnt;
                            }
                            else if(it_tmp != tmp.end())//ûƥ����tmp�ͽ���������Ч
                            {
                                cnt = 0;
                                break;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    std::string memo = strExploreTips;
                    //ƥ��ɹ��򷵻ؽ������ѭ��
                    if (cnt != 0)
                    {
                        std::string a_name = "";
                        std::string p_name = "";
                        for (int i = 0; i < (int)(*it)->combo.size(); ++i)
                        {
                            std::string name = exploreMgr::getInstance()->GetPlaceName((*it)->combo[i]);
                            if(i < cnt)
                            {
                                if(i == 0)
                                {
                                    if (i == cnt - 1)
                                        p_name = ("<font color=\"#74D3FB\">" + name + "</font>");
                                    else
                                        a_name += ("<font color=\"#74D3FB\">" + name + "</font>");
                                }
                                else if(i == 1)
                                {
                                    if (i == cnt - 1)
                                        p_name = ("<font color=\"#00FF0C\">" + name + "</font>");
                                    else
                                        a_name += ("<font color=\"#00FF0C\">" + name + "</font>");
                                }
                                else if(i == 2)
                                {
                                    if (i == cnt - 1)
                                        p_name = ("<font color=\"#ff0000\">" + name + "</font>");
                                    else
                                        a_name += ("<font color=\"#ff0000\">" + name + "</font>");
                                }
                            }
                            else
                            {
                                a_name += name;
                            }
                            if (i < (int)((*it)->combo.size() - 1))
                            {
                                a_name += " ";
                            }
                        }
                        str_replace(memo, "$A", a_name);
                        str_replace(memo, "$N", p_name);
                        str_replace(memo, "$R", "<font color=\"#DAA520\">" + exploreMgr::getInstance()->GetRewardName((*it)->type,(*it)->id) + "</font>");
                        str_replace(memo, "$P", p_name);
                        robj.push_back( Pair("memo", memo));
                        robj.push_back( Pair("id", place->id));
                        robj.push_back( Pair("name", place->name));
                        robj.push_back( Pair("spic", place->spic));
                        return HC_SUCCESS;
                    }
                }
                ++it;
            }
        }
        ++it_canlist;
    }
    return HC_SUCCESS;
}

//�������
int ProcessFarmList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array fields_list;
    boost::shared_ptr<fieldlist> p_fl = farmMgr::getInstance()->GetCharFieldList(cdata->m_id);
    if (p_fl.get())
    {
        for (int i = 0; i < (int)(*p_fl).size(); ++i)
        {
            if ((*p_fl)[i].get())
            {
                Object field;
                field.push_back( Pair("state", (*p_fl)[i]->m_state) );
                field.push_back( Pair("type", (*p_fl)[i]->m_type) );
                int left_sec = ((*p_fl)[i]->m_end_time - time(NULL));
                if (left_sec <= 0)
                {
                    left_sec = 0;
                }
                field.push_back( Pair("leftTime", left_sec) );
                field.push_back( Pair("openNeed", (*p_fl)[i]->m_need_level) );
                field.push_back( Pair("reward_num", (*p_fl)[i]->m_reward_num) );
                fields_list.push_back(field);
            }
        }
    }
    robj.push_back( Pair("farmList", fields_list) );
    Object info;
    int now_seed = cdata->queryExtraData(char_data_type_daily, char_data_farm_seed);
    int total_seed = farmMgr::getInstance()->FieldNum(cdata->m_id)*2;
    info.push_back( Pair("seed", total_seed - now_seed) );
    int now_water = cdata->queryExtraData(char_data_type_daily, char_data_farm_water);
    info.push_back( Pair("water", iFarmWater - now_water) );
    int water_cd = cdata->queryExtraData(char_data_type_daily, char_data_farm_water_cd);
    if (water_cd > time(NULL))
        info.push_back( Pair("water_cd", water_cd - time(NULL)) );
    info.push_back( Pair("nourishReward", farmMgr::getInstance()->getNourishReward(cdata->m_id)) );
    info.push_back( Pair("nourishMax", farmMgr::getInstance()->FieldNum(cdata->m_id)*1440) );
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

//���￪ʼ
int ProcessStartFarm(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");

    ret = farmMgr::getInstance()->StartFarmed(cdata->m_id, pos, robj);
    if (HC_SUCCESS == ret)
    {
        //��������
        cdata->updateTask(task_first_farm);

        //��������
        //cdata->checkGuide(guide_type_frist_farm, 0, 0);
        //�ճ�����
        dailyTaskMgr::getInstance()->updateDailyTask(*cdata,daily_task_farm);
        //actͳ��
        act_to_tencent(cdata,act_new_farm);
    }
    return ret;
}

//�������
int ProcessSetFarmed(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    robj.push_back( Pair("pos", pos) );
    return farmMgr::getInstance()->SetFarmed(cdata->m_id, pos, robj);
}

//�ӳ�����
int ProcessDelayFarm(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    return HC_SUCCESS;
    //return farmMgr::getInstance()->DelayFarmed(cdata->m_id, pos);
}

//�ջ���������
int ProcessSpeedUpHarvest(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (cdata->m_vip < iFarmSpeedVip)
        return HC_ERROR_MORE_VIP_LEVEL;
    return HC_SUCCESS;
    //return farmMgr::getInstance()->SpeedUpHarvest(cdata->m_id);
}

//�������
int ProcessUpFarmField(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return farmMgr::getInstance()->UpFarmField(cdata->m_id,robj);
}

//�������
int ProcessWaterFarmField(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return farmMgr::getInstance()->WaterFarmField(cdata->m_id,robj);
}

//����������
int ProcessWaterFriendField(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    robj.push_back( Pair("purpose", purpose) );
    if (purpose == 1)
    {
        int id = 0;
        READ_INT_FROM_MOBJ(id, o, "id");
        ret = farmMgr::getInstance()->WaterFarmFriendField(cdata->m_id, id, robj);
        if (ret == HC_SUCCESS)
        {
            cdata->NotifyCharData();
            //actͳ��
            act_to_tencent(cdata,act_new_water_friend);
        }
        return ret;
    }
    else if(purpose == 2)
    {
        ret = farmMgr::getInstance()->WaterFarmFriendFieldAll(cdata->m_id, robj);
        if (ret == HC_SUCCESS)
        {
            cdata->NotifyCharData();
            //actͳ��
            act_to_tencent(cdata,act_new_water_friend_all);
        }
        return ret;
    }
}

//���������Ϣ
int ProcessGetWaterFriendList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    int nums_per_page = 0;
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page == 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    boost::shared_ptr<char_ralation> my_rl = Singleton<relationMgr>::Instance().getRelation(cdata->m_id);
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_attentions.begin();
            it != my_rl->m_my_attentions.end();
                ++it)
    {
        CharData* pc = it->second->getChar().get();
        if (pc)
        {
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", pc->m_id) );
                obj.push_back( Pair("name", pc->m_name) );
                obj.push_back( Pair("level", pc->m_level) );
                obj.push_back( Pair("state", !farmMgr::getInstance()->getWaterState(cdata->m_id,pc->m_id)) );
                list.push_back(obj);
            }
        }
    }
    robj.push_back( Pair("list", list) );
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}


//��ȡ��������
int ProcessRewardNourish(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return farmMgr::getInstance()->rewardNourish(cdata->m_id, robj);
}

//��ʾ��ɫ�����佫ѡ���б�
int ProcessQcreate(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (!paccount.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    //robj.clear();
    //robj = query_Create_obj;
    robj.push_back( Pair("s", 200) );
    return HC_SUCCESS;
}

//������ɫ
int ProcessCreateChar(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (!paccount.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }

#ifdef ONE_CHARACTOR
    if (paccount->m_charactorlist.size() > 0)
    {
        return HC_ERROR;
    }
#else
    if (paccount->m_charactorlist.size() >= 3)
    {
        return HC_ERROR;
    }
#endif

    int spic = 1;
    READ_UINT64_FROM_MOBJ(spic, o, "spic");
    std::string name = "unknow";
    READ_STR_FROM_MOBJ(name, o, "name");
    if (name.length() > 18)
    {
        robj.push_back( Pair("name", name) );
        robj.push_back( Pair("length", name.length()) );
        return HC_ERROR_NAME_TOO_LONG;
    }
    //���ַǷ�
    if (name.length() < 3 || !Forbid_word_replace::getInstance()->isLegal(name))
    {
        return HC_ERROR_NAME_ILLEGAL;
    }
#if 0
    int g1 = 3;
    READ_UINT64_FROM_MOBJ(g1, o, "g1");
    int g2 = 4;
    READ_UINT64_FROM_MOBJ(g2, o, "g2");
    if (g2 != create_choose_generals_id[0] && g2 != create_choose_generals_id[1] && g2 != create_choose_generals_id[2])
    {
        g2 = create_choose_generals_id[0];
    }
    if (g1 != create_choose_generals_id[3] && g1 != create_choose_generals_id[4] && g1 != create_choose_generals_id[5])
    {
        g1 = create_choose_generals_id[3];
    }
#else
    int g1 = 1, g2 = 0;
#endif
    uint64_t cid = 0, inv_id = 0;

#ifdef QQ_PLAT
    //�����
    if (paccount->m_login_str2 == "inv")
    {
        Query q(GetDb());
        q.get_result("select c.id from charactors as c left join accounts as a on c.account=a.account where a.qid='"
            + GetDb().safestr(paccount->m_iopenid) + "' and a.server_id='" + GetDb().safestr(paccount->m_server_id) + "'");
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            inv_id = q.getval();
            q.free_result();
        }
        else
        {
            q.free_result();
        }
    }
#endif

    int camp = 0;
    int ret = CreateChar(paccount->m_account, paccount->m_union_id, paccount->m_server_id, paccount->m_qid, camp, spic, name, g1, g2, cid, inv_id);
    if (HC_SUCCESS == ret)
    {
#ifdef QQ_PLAT
        if (inv_id > 0)
        {
            //����ɹ�
            Singleton<inviteMgr>::Instance().add_invited(inv_id, cid);
        }
#endif
        robj.push_back( Pair("id", cid));
        //���µ�¼�б�
        CharactorInfo gd;
        gd.m_cid = cid;
        gd.m_lastlogin = time(NULL);
        gd.m_name = name;
        gd.m_spic = spic;
        gd.m_state = 0;
        gd.m_deleteTime = 0;
        gd.m_level = 1;
        paccount->m_charactorlist.insert(paccount->m_charactorlist.begin(), gd);
        //����ǵ���ɫ��������ɫ�ɹ���ֱ�ӽ�����Ϸ
#if 0//def ONE_CHARACTOR
        json_spirit::mObject obj;
        obj["cmd"] = "roleLogin";
        obj["id"] = cid;
        actionmessage act_msg(obj, 0);
        act_msg.setsession(psession);
        if (0 != InsertActionWork(act_msg))
        {
            ERR();
        }
        return HC_SUCCESS_NO_RET;
#endif
    }
    return ret;
}

//��ɫ���Ƿ�Ϸ�
int ProcessCheckName(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (!paccount.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    std::string name = "unknow";
    READ_STR_FROM_MOBJ(name, o, "name");
    if (name.length() > 18)
    {
        return HC_ERROR_NAME_TOO_LONG;
    }
    //���ַǷ�
    if (name.length() < 3 || !Forbid_word_replace::getInstance()->isLegal(name))
    {
        return HC_ERROR_NAME_ILLEGAL;
    }

    Query q(GetDb());
    //��ɫ��
    q.get_result("select count(*) from charactors where name='" + GetDb().safestr(name) + "'");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        if (q.getval() > 0)
        {
            q.free_result();
            return HC_ERROR_CHAR_EXIST;
        }
        q.free_result();
    }
    else
    {
        q.free_result();
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

//��ȡ�������
int ProcessGetRandomName(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (!paccount.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    int gender = 0;
    READ_INT_FROM_MOBJ(gender,o,"g");
    if (gender != 1)
    {
        gender = 0;
    }
    robj.push_back( Pair("g", gender) );
    robj.push_back( Pair("name", GeneralDataMgr::getInstance()->getRandomName(gender)));
    return HC_SUCCESS;
}

//��ѯ��ɫ��Ϣ
int ProcessQueryChar(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj)
{
    //�˺ŵ�¼״̬
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //�����ɫ�ȼ���Ϣ���ƽ𡢰���������
    Object charobj;
    charobj.push_back( Pair("id", cdata->m_id));
    charobj.push_back( Pair("spic", cdata->m_spic));
    charobj.push_back( Pair("gid", cdata->GetGuildId()));
    charobj.push_back( Pair("vip", cdata->m_vip));
    charobj.push_back( Pair("name", cdata->m_name));
    charobj.push_back( Pair("gold", cdata->gold()));
    charobj.push_back( Pair("silver", cdata->silver()));
    charobj.push_back( Pair("ling", cdata->ling()));
    charobj.push_back( Pair("area", cdata->m_area));
    charobj.push_back( Pair("level", cdata->m_level));

    retObj.push_back( Pair("chardata", charobj));

    return HC_SUCCESS;
}

//��ѯ��ɫ��ť��Ϣ
int ProcessQueryPanel(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj)
{
    //�˺ŵ�¼״̬
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //��ť��Ϣ
    Object panelobj;
    cdata->getPanelInfo(panelobj);
    retObj.push_back( Pair("panel", panelobj));

    return HC_SUCCESS;
}


//��ѯ��ݡ�����
int ProcessQueryDate(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj)
{
    boost::shared_ptr<DateInfo> date = GeneralDataMgr::getInstance()->GetDate();
    if (date.get())
    {
        retObj.push_back( Pair("year", date->year));
        retObj.push_back( Pair("season", date->season));
        //retObj.push_back( Pair("effect", date->effect));
    }
    return HC_SUCCESS;
}

//ɾ����ɫ
int ProcessDeleteChar(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //�˺ŵ�¼״̬
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (paccount == NULL)
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    int char_id = 0;
    READ_INT_FROM_MOBJ(char_id, o, "id");

    //�����б�
    std::list<CharactorInfo>::iterator it = paccount->m_charactorlist.begin();
    while (it != paccount->m_charactorlist.end())
    {
        if (it->m_cid == char_id)
        {
#if 0
            it->m_state = 1;
            it->m_deleteTime = time(NULL) + 72*3600;
            InsertSaveDb("update charactors set state='1' and delete_time=" + LEX_CAST_STR(it->m_deleteTime) + " where cid=" + LEX_CAST_STR(char_id));
#else
            DeleteChar(char_id);
            paccount->m_charactorlist.erase(it);
#endif
            break;
        }
        ++it;
    }
    return HC_SUCCESS;
}

//ɾ����ɫ
int ProcessUnDeleteChar(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //�˺ŵ�¼״̬
    boost::shared_ptr<OnlineUser> paccount = psession->user();
    if (paccount == NULL)
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    int char_id = 0;
    READ_INT_FROM_MOBJ(char_id, o, "id");

    //�����б�
    std::list<CharactorInfo>::iterator it = paccount->m_charactorlist.begin();
    while (it != paccount->m_charactorlist.end())
    {
        if (it->m_cid == char_id)
        {
            if (it->m_state == 1 && it->m_deleteTime > time(NULL))
            {
                it->m_state = 0;
                it->m_deleteTime = 0;
                InsertSaveDb("update charactors set state='0' and delete_time=0 where cid=" + LEX_CAST_STR(char_id));
            }
            //DeleteChar(char_id);
            //paccount->m_charactorlist.erase(it);
            break;
        }
        ++it;
    }
    return HC_SUCCESS;
}

//��ɫ��Ϣ
int ProcessQueryCharData(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //�����ɫ�ȼ���Ϣ���ƽ𡢰���������
    Object charobj;
    cdata->getCharInfo(charobj);
    retObj.push_back( Pair("chardata", charobj));
    //��ť��Ϣ
    Object panelobj;
    cdata->getPanelInfo(panelobj);
    retObj.push_back( Pair("panel", panelobj));

    return HC_SUCCESS;
}

/*��ѯ��ɫ����
int ProcessQueryCharExp(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& obj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    obj.push_back( Pair("curExp", cdata->exp()) );
    if (cdata->m_level < iMaxCharLevel)
    {
        obj.push_back( Pair("levelupExp", get_level_cost(cdata->m_level + 1)) );
    }
    else
    {
        obj.push_back( Pair("levelupExp", get_level_cost(iMaxCharLevel)) );
    }
    return HC_SUCCESS;
}*/

//��Ǩ����һ��ͼ
int ProcessMoveNextMap(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& retObj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (cdata->m_area >= gMaxOpenMap)
    {
        return HC_ERROR_MAP_NOT_OPEN;
    }
    boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(cdata->m_area + 1);
    if (!bm.get() || bm->openLevel == 0)
    {
        return HC_ERROR_MAP_NOT_OPEN;
    }
    //if (cdata->m_level >= (bm->openLevel))
    //ͨ�ز��ܽ�����һͼ
    if (cdata->isMapPassed(cdata->m_area))
    {
        ++cdata->m_area;
        cdata->m_cur_stage = 1;
        cdata->notifyChangeMap();
        cdata->m_tempo.InitCharTempo(cdata->m_area);
        cdata->Save();
        //��������
        cdata->updateTask(task_enter_map, cdata->m_area);

        //��������
        cdata->checkGuide(guide_type_enter_map, cdata->m_area, 0);

        //if (cdata->m_level >= iRaceOpenLevel)
        //{
        //    RaceMgr::getInstance()->updateZone(cdata->m_id);
        //}
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

extern std::string strFightCD;

//�����ؿ�
int ProcessAttackStronghold(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
#ifdef DEBUG_PER
    std::list<uint64_t> times;
    times.push_back(splsTimeStamp());
#endif
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int fcd = pc->m_fight_cd - time(NULL);
    if (fcd > 0)
    {
        std::string msg = strFightCD;
        str_replace(msg, "$T", LEX_CAST_STR(fcd));
        robj.push_back( Pair("msg", msg) );
        return HC_ERROR_NO_RET;
    }
    if (pc->ling() <= 3)
    {
        return HC_ERROR_NOT_ENOUGH_LING;
    }
    uint64_t stronghold = 0;
    READ_UINT64_FROM_MOBJ(stronghold, o, "id");
    if (stronghold < 1)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<StrongholdData> shold = GeneralDataMgr::getInstance()->GetStrongholdData(stronghold);
    if (!shold.get())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharStrongholdData> cd = GeneralDataMgr::getInstance()->GetCharStrongholdData(pc->m_id,shold->m_map_id,shold->m_stage_id, shold->m_strongholdpos);
    if (cd.get())
    {
        if (cd->m_state < 0)
        {
            return HC_ERROR;
        }
        //�״�ռ����Ҫ��������
        if (cd->m_state == 0)
        {
            if (pc->treasureCount(treasure_type_supply) < shold->m_need_supply)
            {
                //�������㲻����Ҫ����ֱ�ӷ��ش�����
                return HC_ERROR_NOT_ENOUGH_SUPPLY;
                #if 0
                //֧��������δ��ɵ�
                if (!pc->m_trunk_tasks.getFinishState())
                {
                    return HC_ERROR_NOT_ENOUGH_SUPPLY;
                }
                //�Ƿ񴥷�������������
                if (pc->checkGuide(guide_type_no_supply, 0, 0) > 0)
                {
                    return HC_ERROR;
                }
                else
                {
                    return HC_ERROR_NOT_ENOUGH_SUPPLY;    //�����뻹��Ҫ����
                }
                #endif
            }
        }
        else
        {
            #if 0
            //ļ�����Ĳ���ģ�Ҫ��������һ����
            if (pc->m_hp_cost > 0)
            {
                if (pc->addSilver(-pc->m_hp_cost) < 0)
                {
                    if (pc->ling() <= 1)
                    {
                        return HC_ERROR_NOT_ENOUGH_LING;
                    }
                }
                else
                {
                    //��������ͳ��
                    add_statistics_of_silver_cost(pc->m_id,pc->m_ip_address,pc->m_hp_cost,silver_cost_for_stronghold);
                    pc->m_hp_cost = 0;
                }
            }
            #endif
        }
    }
    else
    {
        return HC_ERROR;
    }
#ifdef DEBUG_PER
    times.push_back(splsTimeStamp());
#endif
    ret = HC_SUCCESS;
    Combat* pCombat = createStrongholdCombat(pc->m_id, stronghold, ret);
#ifdef DEBUG_PER
    times.push_back(splsTimeStamp());
#endif
    if (HC_SUCCESS == ret && pCombat)
    {
        //��������vip��Ϊ����Щ�쳣�������������
        //pc->startTmpVIP();

        INFO("stronghold InsertCombat()"<<endl);
        //��������ս��˫������Ϣ
        pCombat->setCombatInfo();
        InsertCombat(pCombat);
        std::string sendMsg = "{\"cmd\":\"attack\",\"s\":200,\"getBattleList\":" + pCombat->getCombatInfoText() + "}";
        psession->send(sendMsg);

        //�״�ռ�����CD
        if (cd->m_state == 0)
        {
            pc->m_fight_cd = time(NULL) + TIME_MAX_A_COMBAT;
        }
#ifdef DEBUG_PER
        times.push_back(splsTimeStamp());
        if (times.size()  > 1)
        {
            uint64_t timestart = *(times.begin());
            uint64_t timeend = *(times.rbegin());
            uint64_t totalcost = timeend - timestart;
            if (totalcost > 2000)
            {
                printf("attack stronghold, total cost %ld us,", timeend-timestart);
                for (std::list<uint64_t>::iterator it = times.begin(); it != times.end(); ++it)
                {
                    printf("%ld,", *it - timestart);
                    timestart = *it;
                }
                printf("\n");
            }
        }
#endif
        return HC_SUCCESS_NO_RET;
    }
    return ret;
}

//��ʾ����װ��
int ProcessGetEquipList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }

    int gid = 0;
    READ_INT_FROM_MOBJ(gid,o,"gid");
    //�佫�Ƿ����
    boost::shared_ptr<CharGeneralData> gd = cdata->m_generals.GetGenral(gid);
    if (!gd.get())
    {
        return HC_ERROR;
    }
    return cdata->getEquiped(gid, robj);
}

//��ʾ����
int ProcessShowBackpack(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");

    if (type == iItem_type_equipment)
    {
        return cdata->m_bag.showBagEquipments(robj);
    }
    else
    {
        int page = 1;
        READ_INT_FROM_MOBJ(page, o, "page");
        int pageNums = 0;
        READ_INT_FROM_MOBJ(pageNums, o, "pageNums");
        return cdata->showBackpack(page, pageNums, robj);
    }
}

//��������װ����Ϣ
int ProcessGetEquipInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
   CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 1, cid = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(cid, o, "cid");
    if (cid != 0)
    {
        CharData* pcc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pcc)
        {
            return pcc->getEquipmentInfo(id, robj);
        }
    }
    return cdata->getEquipmentInfo(id, robj);
}

//����������Ʒ��Ϣ
int ProcessGetGemInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 1, nums = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(id, o, "nums");
    return cdata->getTreasureInfo(id, nums, robj);
}

//�������������Ϣ
int ProcessGetLibaoInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id, o, "id");
    return cdata->getLibaoInfo(id, robj);
}

//װ����
int ProcessEquip(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int eid = 1, gid = 0, slot = 0;
    READ_INT_FROM_MOBJ(eid, o, "id");
    READ_INT_FROM_MOBJ(gid, o, "gid");
    READ_INT_FROM_MOBJ(slot, o, "slot");
    return cdata->equip(gid, slot, eid);
}

//һ��װ����
int ProcessOnekeyEquip(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid, o, "gid");
    return cdata->onekeyEquip(gid);
}

//ж��װ��
int ProcessUnequip(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0, gid = 0, slot = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(slot, o, "slot");
    READ_INT_FROM_MOBJ(gid, o, "gid");
    //cout<<"ProcessUnequip:"<<slot<<","<<gid<<","<<id<<endl;
    if (slot)
    {
        return cdata->unequip(gid, slot);
    }
    else
    {
        EquipmentData* eq = cdata->m_generals.getEquipById(id);
        if (eq && eq->getContainer() && eq->getContainer()->gd.get())
        {
            return cdata->unequip(eq->getContainer()->gd->m_id, eq->getSlot());
        }
    }
    return HC_ERROR;
}

//������Ʒ
int ProcessSellEquip(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 1;
    int type = 1;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(type, o, "type");
    if (1 == type)
    {
        return cdata->sellEquipment(id, robj);
    }
    else
    {
        int count = 0;
        READ_INT_FROM_MOBJ(count,o,"count");
        return cdata->sellTreasure(id, count, robj);
    }
}

//�ع�
int ProcessBuyback(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = iItem_type_equipment;
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(type, o, "type");
    if (iItem_type_equipment == type)
    {
        return cdata->buybackEquipment(id, robj);
    }
    else
    {
        if (cdata->m_bag.isFull())
        {
            return HC_ERROR_BACKPACK_FULL_BUYBACK;
        }
        int ret = cdata->m_selled_bag.buyBack(type, id);
        if (ret == HC_SUCCESS && type == iItem_type_equipment)
        {
            cdata->updateEnhanceCost();
            cdata->updateEnhanceCDList();
        }
        return ret;
    }
}

//������Ʒ- type=1,װ�� 2-����
int ProcessSellItem(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int silver_get = 0;
    mArray list;
    READ_ARRAY_FROM_MOBJ(list,o,"slot");
    mArray::iterator it = list.begin();
    while (it != list.end())
    {
        if ((*it).type() != obj_type)
        {
            ++it;
            continue;
        }
        mObject& tmp_obj = (*it).get_obj();
        int pos = 0;
        READ_INT_FROM_MOBJ(pos,tmp_obj,"pos");
        boost::shared_ptr<iItem> itm = cdata->m_bag.getItem(pos);
        if (itm.get())
        {
            int silver = itm->sellPrice();
            if (silver > 0)
            {
                silver *= itm->getCount();
                cdata->addSilver(silver);
                itm->setDeleteTime();
                cdata->m_bag.removeItem(pos);
                cdata->m_selled_bag.add(itm);
                itm->Save();
                silver_get += silver;
                //cout<<"sell,"<<itm->getType()<<","<<itm->getSubtype()<<endl;
                //����������Ʒ��Ҫ��������
                switch (itm->getType())
                {
                    case item_type_treasure:
                    {
                        boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(itm->getSubtype());
                        if (bt.get() && bt->b_used_for_task)
                        {
                            int32_t xcount = cdata->m_bag.getCount((uint16_t) iItem_type_gem, itm->getSubtype());
                            cdata->m_trunk_tasks.updateTask(task_get_gem, itm->getSubtype(), xcount);
                            //cout<<"sell,update trank task "<<itm->getSubtype()<<","<<xcount<<endl;
                        }
                        add_statistics_of_silver_get(cdata->m_id, cdata->m_ip_address, itm->sellPrice(), silver_get_sell_treasure, cdata->m_union_id, cdata->m_server_id);
                        break;
                    }
                    case iItem_type_equipment:
                    {
                        cdata->updateEnhanceCost();
                        cdata->updateEnhanceCDList();
                        add_statistics_of_silver_get(cdata->m_id, cdata->m_ip_address, itm->sellPrice(), silver_get_sell_equiptment, cdata->m_union_id, cdata->m_server_id);
                        break;
                    }
                    case iItem_type_baoshi:
                    {
                        newBaoshi* pb = dynamic_cast<newBaoshi*>(itm.get());
                        add_statistics_of_silver_get(cdata->m_id, cdata->m_ip_address, itm->sellPrice(), silver_get_sell_treasure, cdata->m_union_id, cdata->m_server_id);
                        add_statistics_of_baoshi_cost(cdata->m_id, cdata->m_ip_address, cdata->m_union_id, cdata->m_server_id, itm->getSubtype(), pb->level(), itm->getCount(), baoshi_sell);
                        break;
                    }
                    default:
                    {
                        add_statistics_of_silver_get(cdata->m_id, cdata->m_ip_address, itm->sellPrice(), silver_get_sell_unknow, cdata->m_union_id, cdata->m_server_id);
                        break;
                    }
                }
            }
        }
        ++it;
    }
    if (silver_get > 0)
    {
        cdata->NotifyCharData();
        robj.push_back( Pair("price", silver_get) );
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//�ع�װ��
int ProcessBuybackEquip(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id, o, "id");
    return cdata->buybackEquipment(id, robj);
}

//����װ���ع��б�
int ProcessSelledEquiplist(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    int pageNums = 0;
    READ_INT_FROM_MOBJ(pageNums, o, "pageNums");
    return cdata->listSelledEquipment(page, pageNums, robj);
}

//����װ��ǿ����Ϣ
int ProcessEnhanceEquipInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id, o, "id");
    return cdata->getEquipmentUpInfo(id, robj);
}

//����װ��ǿ���б�
int ProcessEnhanceEquiplist(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    int pageNums = 0;
    READ_INT_FROM_MOBJ(pageNums, o, "pageNums");
    return cdata->getEnhanceEquiptlist(page, pageNums, robj);
}

//װ��ǿ��
int ProcessEnhanceEquip(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (!cdata->m_can_enhance)
    {
        return HC_ERROR_ENHANCE_CD;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    return cdata->enhanceEquipment(id, robj);
}

//��ȡѵ���б�
int ProcessGetTrainList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return cdata->getTrainList(robj);
}

//��ȡ�����б�
int ProcessGetBookList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"gid");
    return cdata->getBookList(id, robj);
}

//��ʼѵ��
int ProcessTrain(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid, o, "gid");
    int bid = 0;
    READ_INT_FROM_MOBJ(bid, o, "bid");
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    return cdata->generalTrain(gid,bid,pos,robj);
}

//�������е�����
int ProcessUpgradeGeneralTrainQue(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    return cdata->upgradeGeneralTrainQue(pos,robj);
}

//�佫������Ϣ
int ProcessInheritInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int gid1 = 0, gid2 = 0;
    READ_INT_FROM_MOBJ(gid1, o, "gid1");
    READ_INT_FROM_MOBJ(gid2, o, "gid2");
    return cdata->generalInheritInfo(gid1,gid2,robj);
}

//�佫����
int ProcessInherit(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int gid1 = 0, gid2 = 0, type = 1;
    READ_INT_FROM_MOBJ(gid1, o, "gid1");
    READ_INT_FROM_MOBJ(gid2, o, "gid2");
    READ_INT_FROM_MOBJ(type, o, "type");
    return cdata->generalInherit(gid1,gid2,type,robj);
}

//���򴫳е�
int ProcessBuyInherit(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int num = 0;
    READ_INT_FROM_MOBJ(num, o, "num");
    return cdata->buyInherit(num);
}

//��ü����б�
int ProcessGetSkillList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return cdata->getSkillList(robj);
}

//��ü��������б�
int ProcessGetSkillResearchList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return cdata->getSkillResearchList(robj);
}

//��õ���������Ϣ
int ProcessGetSkillInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return cdata->getSkillDetail(id, robj);
}

//��ü���������Ϣ�б�
int ProcessGetSkillResearchInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return cdata->getSkillResearchInfo(id, robj);
}

//�����о�
int ProcessResearchSkill(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    int sid = 0;
    READ_INT_FROM_MOBJ(sid,o,"sid");
    return cdata->startSkillResearch(sid, id);
}

//ˢ�¼����о���
int ProcessUpdateTeachers(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    return cdata->updateTeachers(robj, type);
}

//����������λ��
int ProcessBuySkillResearchQue(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return cdata->buyResearchQue(robj);
}

//ֹͣ�о�
int ProcessStopResearch(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return cdata->stopSkillResearch(id);
}

//���ټ����о�
int ProcessFastResearch(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return cdata->speedupResearch(id, 1, robj);
}

//��ѯ���ټ����о�����Ҫ���
int ProcessQueryFastResearch(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return cdata->speedupResearch(id, 0, robj);
}

//�������е�����
int ProcessUpgradeResearchQue(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return cdata->upgradeResearchQue(robj);
}

#if 0
//�µ�״̬��ѯ
int ProcessGetOpenStates(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array slist;
    cdata->m_newStates.getStateInfo(slist);
    json_spirit::Object info;
    cdata->m_newStates.getCostInfo(info);
    robj.push_back( Pair("info", info) );
    robj.push_back( Pair("list", slist) );
    return HC_SUCCESS;
}

//�µ�״̬ ��ѯ����������Ϣ
int ProcessGetStarInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return cdata->m_newStates.getStarInfo(robj);
}

//�µ�״̬ ������������
int ProcessLevelupStar(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return cdata->m_newStates.levelup();
}
#endif

//��ɫ��ְ��Ϣ
int ProcessGetOffical(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    Object info;
    boost::shared_ptr<baseoffical> p_bo = GeneralDataMgr::getInstance()->GetBaseOffical(cdata->m_offical);
    if (p_bo.get())
    {
        Object curPost;
        curPost.push_back( Pair("id", p_bo->m_id) );
        curPost.push_back( Pair("name", p_bo->m_name) );
        curPost.push_back( Pair("prestige", cdata->m_prestige) );
        curPost.push_back( Pair("salary", p_bo->m_salary) );
        curPost.push_back( Pair("camp", cdata->m_camp) );
        curPost.push_back( Pair("canGet", cdata->m_hasgetsalary) );
        info.push_back( Pair("curPost", curPost) );
        INFO("get baseoffical success" << endl);
    }
    p_bo = GeneralDataMgr::getInstance()->GetBaseOffical(cdata->m_offical + 1);
    if (p_bo.get())
    {
        Object nextPost;
        nextPost.push_back( Pair("id", p_bo->m_id) );
        nextPost.push_back( Pair("name", p_bo->m_name) );
        nextPost.push_back( Pair("prestige", p_bo->need_prestige) );
        info.push_back( Pair("nextPost", nextPost) );
        //Object extra;
        //int weakcamp = GeneralDataMgr::getInstance()->getWeakCamps();
        //extra.push_back( Pair("weakCamp", weakcamp) );
        //if (weakcamp != 0 && cdata->m_camp == weakcamp)
        //{
        //    int done = cdata->queryExtraData(char_data_type_daily, char_data_camp_reward);
        //    extra.push_back( Pair("canGet", done < 1) );
        //}
        //info.push_back( Pair("extra", extra) );
        INFO("get next_baseoffical success" << endl);
    }
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

//��ɫ��ְ������Ϣ
int ProcessGetOfficalTec(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array list;
    INFO("getofficaltec" << endl);

    INFO("getofficaltec success" << endl);
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//��ɫ��ְ�佫��Ϣ
int ProcessGetOfficalGenaral(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    int nums_per_page = 0;
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page == 0)
    {
        nums_per_page = 10;
    }
    int offical = cdata->m_offical;
    #if 0
    int cur_nums = 0, can_buy_nums = 0;
    std::list<boost::shared_ptr<officalgenerals> >& p_list = GeneralDataMgr::getInstance()->GetBaseOfficalGenerals();
    std::list<boost::shared_ptr<officalgenerals> >::iterator it = p_list.begin();
    while(it != p_list.end())
    {
        if ((*it).get())
        {
            //�����佫��������ʾ���ȼ���������ʾ
            if (cdata->CheckHasGeneral((*it)->m_gid) || (*it)->need_slevel > cdata->m_level)
            {
                ++it;
                continue;
            }
            ++cur_nums;
            if ((*it)->need_offical <= offical
                && (*it)->need_slevel <= cdata->m_level)
            {
                ++can_buy_nums;
            }
        }
        ++it;
    }
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    if (page > maxpage)
        page = maxpage;
    Object info;
    info.push_back( Pair("active", can_buy_nums) );
    info.push_back( Pair("total", cur_nums) );
    robj.push_back( Pair("info", info) );
    Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );

    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    cur_nums = 0;
    #endif
    int now_off = 1, cnt = 0, line = 0;
    json_spirit::Array list;
    json_spirit::Array glist;
    std::list<boost::shared_ptr<officalgenerals> >& p_list = GeneralDataMgr::getInstance()->GetBaseOfficalGenerals();
    std::list<boost::shared_ptr<officalgenerals> >::iterator it = p_list.begin();
    while(it != p_list.end())
    {
        if ((*it).get())
        {
            //�Ѿ���������һ��ְ��һ��������������佫
            if ((*it)->need_offical > now_off || cnt == 3)
            {
                //���뵱ǰ��Ϣ
                Object off;
                boost::shared_ptr<baseoffical> pbo = GeneralDataMgr::getInstance()->GetBaseOffical(now_off);
                if (pbo.get())
                {
                    off.push_back( Pair("off_name", pbo->m_name) );
                    off.push_back( Pair("off_level", pbo->m_id) );
                }
                off.push_back( Pair("glist", glist) );
                //�Ѿ�������������ٲ���
                if (line != 0 && cnt == 0)
                {
                    ;
                }
                else
                {
                    list.push_back(off);
                    ++line;
                }
                glist.clear();
                cnt = 0;
                if ((*it)->need_offical > now_off)
                {
                    line = 0;
                    now_off = (*it)->need_offical;
                }
            }
            //ֻ��ʾ����һ��ְ
            if ((*it)->need_offical != 0 && (*it)->need_offical > offical + 1)
            {
                break;
            }
            //�����佫��������ʾ���ȼ���������ʾ
            if (cdata->CheckHasGeneral((*it)->m_gid) || (*it)->need_slevel > cdata->m_currentStronghold)
            {
                ++it;
                continue;
            }
            int gid = 0;
            bool fired = cdata->CheckHasFireGeneral((*it)->m_gid, gid);
            //������ļ�佫���ڽ���б��г���
            if ((*it)->m_special && !fired)
            {
                ++it;
                continue;
            }
            //++cur_nums;
            //if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                Object general;
                general.push_back( Pair("id", (*it)->m_gid) );
                general.push_back( Pair("name", (*it)->m_name) );
                general.push_back( Pair("quality", (*it)->m_quality) );
                general.push_back( Pair("spic", (*it)->m_spic) );
                general.push_back( Pair("good_at", (*it)->m_good_at) );
                //����ٻ��佫��ʾ0����
                if (fired)
                    general.push_back( Pair("price", 0) );
                else
                    general.push_back( Pair("price", (*it)->m_price) );
                //Object soldier;
                //boost::shared_ptr<BaseSoldierData> base_sd = GeneralDataMgr::getInstance()->GetBaseSoldier((*it)->m_sid);
                //if (base_sd.get())
                //{
                //    soldier.push_back( Pair("id", base_sd->m_stype) );
                //    soldier.push_back( Pair("name", base_sd->m_name) );
                //    soldier.push_back( Pair("memo", base_sd->m_desc) );
                //    general.push_back( Pair("soldier", soldier) );
                //}
                if ((*it)->need_offical <= offical
                    && (*it)->need_slevel <= cdata->m_currentStronghold)
                {
                    general.push_back( Pair("canBuy", 1) );
                }
                else
                {
                    general.push_back( Pair("canBuy", 0) );
                    boost::shared_ptr<baseoffical> pbo = GeneralDataMgr::getInstance()->GetBaseOffical((*it)->need_offical);
                    if (pbo.get())
                    {
                        general.push_back( Pair("prestige", pbo->need_prestige) );
                        general.push_back( Pair("slevel", (*it)->need_slevel) );
                    }
                    else
                    {
                        general.push_back( Pair("prestige", -1) );
                        general.push_back( Pair("slevel", (*it)->need_slevel) );
                    }
                }
                glist.push_back(general);
                ++cnt;
            }
        }
        ++it;
    }
    //ȫ����������
    if (it == p_list.end())
    {
        //���뵱ǰ��Ϣ
        Object off;
        boost::shared_ptr<baseoffical> pbo = GeneralDataMgr::getInstance()->GetBaseOffical(now_off);
        if (pbo.get())
        {
            off.push_back( Pair("off_name", pbo->m_name) );
            off.push_back( Pair("off_level", pbo->m_id) );
        }
        off.push_back( Pair("glist", glist) );
        list.push_back(off);
        glist.clear();
        cnt = 0;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//������Ӫ
int ProcessJoinCamps(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> account = psession->user();
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata || NULL == account.get())
    {
        return ret;
    }
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    if (type != 1 && type != 2)
    {
        type = 1;
    }
    if (cdata->m_camp == 0)
    {
        INFO("######## join camps "<<type<<endl);
        //�뿪��ӪƵ��
        boost::shared_ptr<ChatChannel> ach = GeneralDataMgr::getInstance()->GetCampChannel(0);
        if (ach.get())
        {
            INFO("######## join camps , remove from camp channel 0"<<endl);
            ach->Remove(account->m_onlineCharactor);
        }
        //�����µ���ӪƵ��
        boost::shared_ptr<ChatChannel> new_ch = GeneralDataMgr::getInstance()->GetCampChannel(type);
        if (new_ch.get())
        {
            INFO("######## join camps , join camp channel "<<type<<endl);
            new_ch->Add(account->m_onlineCharactor);
        }
        cdata->m_camp = type;
        InsertSaveDb("update char_data set camp='" + LEX_CAST_STR(type)
                + "' where cid=" + LEX_CAST_STR(cdata->m_id));
        cdata->OfficalLevelUp();
        //������Ӫ�������
        cdata->updateTask(task_choose_camp, 0, 0);
        //������Ӫ����
        //int weakcamp = GeneralDataMgr::getInstance()->getWeakCamps();
        //if (type == weakcamp)
        //{
        //    cdata->addGold(20);
        //    //��һ��ͳ��
        //    add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address,20,gold_get_camps);
        //}
        //GeneralDataMgr::getInstance()->updateCampCnt(type);
        return HC_SUCCESS;
    }
    else if (cdata->m_camp == type)
    {
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

#if 0
//�Ƽ���������Ӫ
int ProcessgetWeakCamps(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    robj.push_back( Pair("type", GeneralDataMgr::getInstance()->getWeakCamps()) );
    return HC_SUCCESS;
}

//���ӵ�������Ӫ
int ProcessRevoltCamps(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> account = psession->user();
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata || NULL == account.get())
    {
        return ret;
    }
    int done = cdata->queryExtraData(char_data_type_daily, char_data_camp_revolt);
    if (done > 0)
    {
        return HC_ERROR_REVOLT_CD;
    }
    if (cdata->m_corps_member.get() && cdata->m_corps_member->offical == 1)
    {
        return HC_ERROR_REVOLT_CORPS;
    }
    //�뿪��ӪƵ��
    boost::shared_ptr<ChatChannel> ach = GeneralDataMgr::getInstance()->GetCampChannel(cdata->m_camp);
    if (ach.get())
    {
        ach->Remove(account->m_onlineCharactor);
    }
    if (GeneralDataMgr::getInstance()->RevoltCamps(cdata->m_camp) != -1)
    {
        cdata->setExtraData(char_data_type_daily, char_data_camp_revolt, done + 1);
        if (cdata->m_corps_member.get())
        {
            corpsMgr::getInstance()->quitCorps(cdata->m_id, cdata->m_corps_member->corps, robj);
        }
    }
    //�����µ���ӪƵ��
    boost::shared_ptr<ChatChannel> new_ch = GeneralDataMgr::getInstance()->GetCampChannel(cdata->m_camp);
    if (new_ch.get())
    {
        new_ch->Add(account);
    }
    return HC_SUCCESS;
}
#endif

//��ְ����
int ProcessLevelUpOffical(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    cdata->OfficalLevelUp();
    cdata->m_officalcanlevelup = cdata->OfficalLevelUpState();
    return HC_SUCCESS;
}

//��ȡ��ɫ�̵���Ʒ
int ProcessGetShopList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }

    if (cdata->m_shopOpen == 0)
    {
        return HC_ERROR;
    }

    int leftTimes = 0;
    /*����ˢ��ʱ���ж��Ƿ���Ҫˢ��*/
    time_t tnow = time(NULL);
    int time_pass = cdata->m_shop.m_refresh_time == 0 ? g_shop_refresh_secs : (tnow - cdata->m_shop.m_refresh_time);
    if (time_pass >= g_shop_refresh_secs)
    {
        cdata->m_shop.refresh(1);
        int m = time_pass % (g_shop_refresh_secs);
        cdata->m_shop.m_refresh_time = tnow - m;
        cdata->setExtraData(char_data_type_normal, char_data_shop_refresh_time, cdata->m_shop.m_refresh_time);
        leftTimes = g_shop_refresh_secs - m;
    }
    else
    {
        leftTimes = g_shop_refresh_secs - time_pass;
    }

    json_spirit::Array list;
    cdata->m_shop.getList(list);

    json_spirit::Object info;
    int refresh_times = cdata->queryExtraData(char_data_type_daily, char_data_shop_refresh) + 1;
    int need_gold = 0;
#ifdef JP_SERVER
    if (refresh_times <= 3)
    {
        need_gold = 10;
    }
    else if(refresh_times <= 10)
    {
        need_gold = 20;
    }
    else if(refresh_times <= 50)
    {
        need_gold = 50;
    }
#else
    need_gold = refresh_times * iRefreshShopGoldCost;
    if (need_gold <= 0)
    {
        need_gold = iRefreshShopGoldCost;
    }
#endif
    info.push_back( Pair("gold", need_gold) );
    info.push_back( Pair("can_refresh", refresh_times <= g_shop_refresh_more*iRefreshShopGoldVIP[cdata->m_vip]) );
    info.push_back( Pair("leftTime", leftTimes) );
    robj.push_back( Pair("info", info) );
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//��ȡ�̵깺���¼��б�
int ProcessGetShopEventList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array list;
    Singleton<shopMgr>::Instance().getList(list);
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//��ȡϴ�������Ϣ
int ProcessGetWashInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid,o,"id");
    return cdata->WashInfo(gid, robj);
}

//ϴ��
int ProcessWash(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int purpose = 1, gid = 0, type = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    READ_INT_FROM_MOBJ(gid,o,"id");
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("purpose", purpose) );
    if (purpose == 1)//ϴ������
    {
        return cdata->Wash(gid, type, robj);
    }
    else if(purpose == 2)//�滻����
    {
        return cdata->WashConfirm(gid);
    }
    return HC_ERROR;
}

//ˢ��
int ProcessRefreshXXX(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("purpose", purpose) );
    int pos = 0;
    READ_INT_FROM_MOBJ(pos,o,"pos");

    if (type != 1 && type != 2 && type != 3)
    {
        type = 1;
    }
    switch (purpose)
    {
        case 3://ˢ�±���
            {
                int ret = cdata->updateBooks(type, robj);
                return ret;
            }
            break;
#if 0
        case 5://ˢ��ұ��
            {
                int ret = SmeltMgr::getInstance()->Refresh(cdata->m_id, type);
                return ret;
            }
            break;
#endif
        case 6://ˢ��̽��
            {
                int ret = exploreMgr::getInstance()->ExploreRefresh(cdata->m_id, type == 2);
                return ret;
            }
            break;
#if 0
        case 7://ˢ��״̬
            {
                int ret = cdata->refreshStates(type, pos);
                return ret;
            }
            break;
#endif
        case 8://ˢ���̵�
            {
                int ret = cdata->refreshShopGoods(type);
                return ret;
            }
            break;
        case 9://ˢ���ճ�����
            break;
        default:
            break;
    }
    return HC_ERROR;
}

//�ջ�
int ProcessGetXXX(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    int purpose = 4;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("purpose", purpose) );
    switch (purpose)
    {
        case 1://�����ջ�
            {
                //ȡ��
            }
            break;
        case 2://��ȡٺ»
            {
                if (pc->m_hasgetsalary == 0 && pc->m_salary > 0)
                {
                    if (-1 != pc->addSilver(pc->m_salary))
                    {
                        //���һ��ͳ��
                        add_statistics_of_silver_get(pc->m_id,pc->m_ip_address,pc->m_salary,silver_get_offical, pc->m_union_id, pc->m_server_id);
                        pc->m_hasgetsalary = 1;
                        pc->saveCharDailyVar();
                        Object info;
                        info.push_back( Pair("silver", pc->m_salary) );
                        robj.push_back( Pair("info", info) );
                        //�ճ�����
                        dailyTaskMgr::getInstance()->updateDailyTask(*pc, daily_task_salary);

                        //֪ͨٺ»��ť��ʧ
                        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pc->m_name);
                        if (account.get())
                        {
                            account->Send("{\"type\":9,\"cmd\":\"removeAction\",\"s\":200}");
                        }
                        return HC_SUCCESS;
                    }
                }
                else
                {
                    return HC_ERROR;
                }
            }
            break;
        case 4:    //������
            {
                //�˺ŵ�¼״̬
                boost::shared_ptr<OnlineUser> account = psession->user();
                if (!account.get())
                {
                    return HC_ERROR_LOGIN_FIRST;
                }
                if (!account->m_onlineCharactor.get() || !account->m_onlineCharactor->m_charactor.get())
                {
                    return HC_ERROR_LOGIN_FIRST;
                }
                //int tid = 0;
                //READ_INT_FROM_MOBJ(tid,o,"id");
                //boost::shared_ptr<CharData> cdata(pc);
                return taskMgr::getInstance()->taskDone(account->m_onlineCharactor->m_charactor, id, robj);
            }
        case 5:    //��ȡ���
            {
                std::string code = "";
                if (id == 0)
                {
                    return HC_ERROR;
                    //READ_STR_FROM_MOBJ(code,o,"code");
                    //return packsMgr::getInstance()->getPacks(pc, code, robj);
                }
                else
                {
                    return packsMgr::getInstance()->getPacks(pc, id, robj);
                }
            }
            break;
        case 6:    //���ֳ��������
            {
                //�Ƿ������ֳ����
                //if (pc->m_createTime + iNewbieGoGoGoSecs - time(NULL) > 0)
                {
                    int level = 0;
                    READ_INT_FROM_MOBJ(level,o,"id");
                    if (pc->m_level >= level)
                    {
                        if (pc->m_bag.isFull())
                        {
                            return HC_ERROR_BAG_FULL;
                        }
                        int libaoid = libao_mgr::getInstance()->getLevelLibao(level);
                        if (libaoid > 0 && pc->m_newbie_reward[level] == false)
                        {
                            //�����
                            int ret = pc->rewardLibao(libaoid, robj);
                            if (ret == HC_SUCCESS)
                            {
                                pc->m_newbie_reward[level] = true;
                                InsertSaveDb("insert into char_newbie_event (cid,level) values ("
                                        + LEX_CAST_STR(pc->m_id) + "," + LEX_CAST_STR(level) + ")");
                                //�������ֳ����Ƿ���Ի��
                                pc->updateNewbieEventState();
                                //���������ť
                                pc->notifyOpeningState();
                            }
                            return ret;
                        }
                    }
                    return HC_ERROR;
                }
            }
            break;
        case 7://��ȡ����������
            {
                if (id <= 0 || id > max_map_id)
                    return HC_ERROR;
                std::map<int,int>::iterator it = pc->m_map_intro_get.find(id);
                if (it != pc->m_map_intro_get.end() && it->second == 1)
                    return HC_ERROR;
                return pc->getMapIntroReward(id,robj);
            }
            break;
        case 9://ÿ�ո���
            {
                if (pc->m_welfare == 0)
                {
                    #ifdef VN_SERVER
                        int ling_get = 12;
                        //ע��ǰ����
                        if (pc->regDays() <= 3)
                        {
                            ling_get = 20;
                        }
                    #else
                        int ling_get = 12;
                    #endif
                    //��������ֵ4��
                    if(ling_get > 0)
                        ling_get *= 4;
                    pc->addLing(ling_get);
                    pc->m_welfare = 1;
                    add_statistics_of_ling_cost(pc->m_id,pc->m_ip_address,ling_get,ling_rest_by_active, 1, pc->m_union_id, pc->m_server_id);
                    InsertSaveDb("replace into char_data_temp (cid,welfare) values (" + LEX_CAST_STR(pc->m_id)
                            + "," + LEX_CAST_STR(pc->m_welfare)
                            + ")");
                    return HC_SUCCESS;
                }
                else
                {
                    return HC_ERROR;
                }
            }
            break;
        case 10://������¼����
            {
                //ȡ��
            }
            break;
        case 11://VIP����
            {
                if (pc->m_vip >= id && pc->m_vip_present.find(id) != pc->m_vip_present.end() && pc->m_vip_present[id].state == 1 && pc->m_vip_present[id].present)
                {
                    pc->m_vip_present[id].state = 2;
                    std::string reward_msg = "";
                    std::list<Item>::iterator it = pc->m_vip_present[id].present->m_list.begin();
                    while (it != pc->m_vip_present[id].present->m_list.end())
                    {
                        if (reward_msg != "")
                        {
                            reward_msg += ",";
                        }
                        if (it->type == item_type_baoshi)
                        {
                            baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(it->id);
                            if (bbs)
                            {
                                reward_msg += bbs->Name_to_Link(it->fac);
                            }
                        }
                        else
                        {
                            reward_msg += it->toString();
                        }
                        ++it;
                    }
                    //������
                    std::list<Item> Item_list = pc->m_vip_present[id].present->m_list;
                    giveLoots(pc, Item_list, 0, pc->m_level, 0, NULL, &robj, true, give_libao_loot);
                    InsertSaveDb("INSERT INTO `char_vip_present` (`cid`, `vip_id`) VALUES ('" + LEX_CAST_STR(pc->m_id) + "', '" + LEX_CAST_STR(id)+ "')");

                    std::string msg = strGetVIPPresentMsg;
                    str_replace(msg, "$N", pc->m_name);
                    str_replace(msg, "$n", LEX_CAST_STR(id));
                    if (msg != "")
                    {
                        str_replace(msg, "$M", reward_msg);
                        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                    }

                    //���������ť״̬
                    pc->notifyVipState();
                    return HC_SUCCESS;
                }
                else
                    return HC_ERROR;
            }
            break;

        case 12://��ֵ���� ����
            {
                int charge = 100;
                READ_INT_FROM_MOBJ(charge,o,"id");
                ret = recharge_event_mgr::getInstance()->getReward(pc, 1, charge);
                if (HC_SUCCESS == ret)
                {
                    //���¿������ť״̬
                    pc->notifyOpeningState();
                }
                return ret;
            }

        case 14://��ֵ���� �ۼ�
            {
                int charge = 100;
                READ_INT_FROM_MOBJ(charge,o,"id");
                ret = recharge_event_mgr::getInstance()->getReward(pc, 2, charge);
                if (HC_SUCCESS == ret)
                {
                    //���¿������ť״̬
                    pc->notifyOpeningState();
                }
                return ret;
            }

        case 13://������Ӫ����
            {
                int done = pc->queryExtraData(char_data_type_daily, char_data_camp_reward);
                if (done < 1)
                {
                    pc->addLing(6);
                    pc->setExtraData(char_data_type_daily, char_data_camp_reward, done + 1);
                    pc->NotifyCharData();
                    return HC_SUCCESS;
                }
                else
                {
                    return HC_ERROR;
                }
            }
            break;

#ifdef QQ_PLAT
        case 15:    //��ȡ����ר���佫
        case 20:    //��ȡVIP5ר���佫 cmd��dealGetAward�� purpose��20
        {
            if (pc->m_qq_yellow_level <= 0)
            {
                //2.13��ͨ�û���ҪV5
                if (pc->m_vip < 5 || pc->m_level < 30)
                    return HC_ERROR;
            }
            else
            {
                //2.13�����û���ҪV4
                if (pc->m_vip < 4 || pc->m_level < 30)
                    return HC_ERROR;
            }
            //ר���佫�Ƿ��Ѿ���ȡ
            if (pc->queryExtraData(char_data_type_normal, char_data_qq_yellow_special))
            {
                return HC_ERROR;
            }
            baseLibao* lb = libao_mgr::getInstance()->getQQSpecialLibao();
            if (lb)
            {
                if ((pc->m_bag.size()-pc->m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                //�佫�����ж�
                if (pc->m_generals.m_generals.size() >= pc->m_general_limit)
                {
                    return HC_ERROR_TOO_MUCH_GENERALS;
                }
                //������
                std::list<Item> items = lb->m_list;
                giveLoots(pc, items, 0, pc->m_level, 0, NULL, &robj, true, give_libao_loot);
                pc->setExtraData(char_data_type_normal, char_data_qq_yellow_special, 1);
            }
            return HC_SUCCESS;
        }
        case 16:    //��ȡ�����������
        {
            if (pc->m_qq_yellow_level <= 0)
            {
                return HC_ERROR;
            }
            //�Ƿ��Ѿ���ȡ
            if (pc->queryExtraData(char_data_type_normal, char_data_qq_yellow_newbie))
            {
                return HC_ERROR;
            }
            //������
            baseLibao* lb = libao_mgr::getInstance()->getQQNewbieLibao();
            if (lb)
            {
                if ((pc->m_bag.size()-pc->m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                //������
                std::list<Item> items = lb->m_list;
                giveLoots(pc, items, 0, pc->m_level, 0, NULL, &robj, true, give_libao_loot);

                pc->setExtraData(char_data_type_normal, char_data_qq_yellow_newbie, 1);
            }
            return HC_SUCCESS;
        }
        case 17:    //��ȡ����ÿ�����
        {
            if (pc->m_qq_yellow_level <= 0)
            {
                return HC_ERROR;
            }
            //�Ƿ��Ѿ���ȡ
            if (pc->queryExtraData(char_data_type_daily, char_data_daily_qq_yellow_libao))
            {
                return HC_ERROR;
            }
            //������
            baseLibao* lb = libao_mgr::getInstance()->getQQDailyLibao(pc->m_qq_yellow_level);
            if (lb)
            {
                if ((pc->m_bag.size()-pc->m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                //������
                std::list<Item> items = lb->m_list;
                giveLoots(pc, items, 0, pc->m_level, 0, NULL, &robj, true, give_libao_loot);
                pc->setExtraData(char_data_type_daily, char_data_daily_qq_yellow_libao, 1);
            }
            return HC_SUCCESS;
        }
        case 18:    //��ȡ��ѻ���ÿ�ս���
        {
            if (pc->m_qq_yellow_year <= 0)
            {
                return HC_ERROR;
            }
            //�Ƿ��Ѿ���ȡ
            if (pc->queryExtraData(char_data_type_daily, char_data_daily_qq_year_yellow_libao))
            {
                return HC_ERROR;
            }
            baseLibao* lb = libao_mgr::getInstance()->getQQYearDailyLibao();
            if (lb)
            {
                if ((pc->m_bag.size()-pc->m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                //������
                std::list<Item> items = lb->m_list;
                giveLoots(pc, items, 0, pc->m_level, 0, NULL, &robj, true, give_libao_loot);

                pc->setExtraData(char_data_type_daily, char_data_daily_qq_year_yellow_libao, 1);
            }
            return HC_SUCCESS;
        }
        case 19:    //��ȡ����ɳ����
        {
            if (pc->m_qq_yellow_level <= 0)
            {
                return HC_ERROR;
            }
            //�Ƿ��Ѿ���ȡ
            if (pc->queryExtraData(char_data_type_normal, char_data_qq_yellow_level_libao + id))
            {
                return HC_ERROR;
            }
            baseLibao* lb = libao_mgr::getInstance()->getQQLevelLibao(id);
            if (lb)
            {
                if ((pc->m_bag.size()-pc->m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                //������
                std::list<Item> items = lb->m_list;
                giveLoots(pc, items, 0, pc->m_level, 0, NULL, &robj, true, give_libao_loot);

                pc->setExtraData(char_data_type_normal, char_data_qq_yellow_level_libao + id, 1);
            }
            return HC_SUCCESS;
        }
#else
        case 15:    //��ȡ����ר���佫
        //��ȡVIP5ר���佫 cmd��dealGetAward�� purpose��20
        case 20:
        {
            if (pc->m_vip < 5 || pc->m_level < 30)
            {
                return HC_ERROR;
            }
            //ר���佫�Ƿ��Ѿ���ȡ
            if (pc->queryExtraData(char_data_type_normal, char_data_vip_special_libao))
            {
                return HC_ERROR;
            }
            baseLibao* lb = libao_mgr::getInstance()->getVipSpecialLibao();
            if (lb)
            {
                if ((pc->m_bag.size()-pc->m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                //�佫�����ж�
                if (pc->m_generals.m_generals.size() >= pc->m_general_limit)
                {
                    return HC_ERROR_TOO_MUCH_GENERALS;
                }
                //������
                std::list<Item> items = lb->m_list;
                giveLoots(pc, items, 0, pc->m_level, 0, NULL, &robj, true, give_libao_loot);
                pc->setExtraData(char_data_type_normal, char_data_vip_special_libao, 1);
            }
            pc->notifyVipState();
            return HC_SUCCESS;
        }
        //��ȡVIPÿ����� cmd��dealGetAward�� purpose��21
        case 21:
        {
            if (pc->m_vip <= 0)
            {
                return HC_ERROR;
            }
            //�Ƿ��Ѿ���ȡ
            if (pc->queryExtraData(char_data_type_daily, char_data_daily_vip_libao))
            {
                return HC_ERROR;
            }
            //������
            baseLibao* lb = libao_mgr::getInstance()->getVipDailyLibao(pc->m_vip);
            if (lb)
            {
                if ((pc->m_bag.size()-pc->m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                //������
                std::list<Item> items = lb->m_list;
                giveLoots(pc, items, 0, pc->m_level, 0, NULL, &robj, true, give_libao_loot);
                pc->setExtraData(char_data_type_daily, char_data_daily_vip_libao, 1);
            }
            pc->notifyVipState();
            return HC_SUCCESS;
        }
        case 22:
        //��ȡVIP�ܸ��� cmd��dealGetAward�� purpose��22
        {
            if (pc->m_vip <= 0)
            {
                return HC_ERROR;
            }
            //�Ƿ��Ѿ���ȡ
            if (pc->queryExtraData(char_data_type_week, char_data_week_vip_libao))
            {
                return HC_ERROR;
            }
            //������
            baseLibao* lb = libao_mgr::getInstance()->getVipWeekLibao();
            if (lb)
            {
                if ((pc->m_bag.size()-pc->m_bag.getUsed()) < lb->need_slot_num)
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                int cost = libao_mgr::getInstance()->vipWeekGold();
                if (cost < 0)
                {
                    return HC_ERROR;
                }
                if (pc->addGold(-cost) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                //������
                std::list<Item> items = lb->m_list;
                giveLoots(pc, items, 0, pc->m_level, 0, NULL, &robj, true, give_libao_loot);
                pc->setExtraData(char_data_type_week, char_data_week_vip_libao, 1);
            }
            pc->notifyVipState();
            return HC_SUCCESS;
        }
#endif
        //��ȡ�佫��ļ����� cmd��dealGetAward�� purpose��23��id���佫����
        case 23:
        {
            char_general_event* e = Singleton<new_event_mgr>::Instance().getCharGeneralEvent(pc->m_id);
            if (e)
            {
                return e->getAwards(*pc, id, robj);
            }
            break;
        }
        //��ȡ��ʯ�ϳɻ���� cmd��dealGetAward�� purpose��24��id����ʯ�ȼ�
        case 24:
        {
            char_baoshi_event* e = Singleton<new_event_mgr>::Instance().getCharBaoshiEvent(pc->m_id);
            if (e)
            {
                return e->getAwards(*pc, id, robj);
            }
            break;
        }
        //��ȡ7��Ŀ�꽱�� cmd��dealGetAward�� purpose��25��id��1-7
        case 25:
        {
            break;
        }
        //��ȡ�ۼ�ǩ������ cmd��dealGetAward�� purpose��26��id������
        case 26:
        {
            char_sign_data* e = Singleton<new_event_mgr>::Instance().getCharSignData(pc->m_id);
            if (e)
            {
                int ret = e->getAwards(*pc, id, robj);
                pc->notifyEventState(top_level_event_sign, e->m_canGet, e->getOnlineLibaoLeftNum());
                return ret;
            }
            break;
        }
        //��ȡǩ��������� cmd��dealGetAward�� purpose��27
        case 27:
        {
            char_sign_data* e = Singleton<new_event_mgr>::Instance().getCharSignData(pc->m_id);
            if (e)
            {
                int ret = e->getOnlineAwards(*pc, robj);
                pc->notifyEventState(top_level_event_sign, e->m_canGet, e->getOnlineLibaoLeftNum());
                return ret;
            }
            break;
        }
        //��ȡ����������
        case 28:
        {
            return Singleton<cost_feedback_event>::Instance().getFeedbackAward(*pc, id, robj);
        }
#ifdef QQ_PLAT
        //��ȡÿ�����뽱�� 29
        case 29:
        {
            return Singleton<inviteMgr>::Instance().getInviteAward(*pc, id, robj);
        }
        //��ȡ������ѽ��� cmd��dealGetAward��type��30��id��
        case 30:
        {
            return Singleton<inviteMgr>::Instance().getInvitedAward(*pc, id, robj);
        }
        //��ȡ���ѽ��� cmd��dealGetAward��type��31��id��
        case 31:
        {
            return Singleton<inviteMgr>::Instance().getCloseFriendAward(*pc, id, robj);
        }
        //��ȡ�ٻغ��ѽ��� cmd��dealGetAward��type��32
        case 32:
        {
            return Singleton<inviteMgr>::Instance().getRecallAward(*pc, robj);
        }
#endif
        //��ȡ�ճ�ֵ����
        case 33:
        {
            return Singleton<new_event_mgr>::Instance().getDailyRechargeAward(*pc, robj);
        }
        //��ȡV8�佫
        case 34:
        {
            if (pc->m_vip < 8 || pc->m_level < 40)
            {
                return HC_ERROR;
            }
            //ר���佫�Ƿ��Ѿ���ȡ
            if (pc->queryExtraData(char_data_type_normal, char_data_vip8_general))
            {
                return HC_ERROR;
            }
            //�佫�����ж�
            if (pc->m_generals.m_generals.size() >= pc->m_general_limit)
            {
                return HC_ERROR_TOO_MUCH_GENERALS;
            }
            pc->m_generals.Add(998, true);
            pc->setExtraData(char_data_type_normal, char_data_vip8_general, 1);
            pc->notifyVipState();
            return HC_SUCCESS;
        }
        //��ȡ�ɳ����
        case 35:
        {
            int pos = 0;
            READ_INT_FROM_MOBJ(pos,o,"pos");
            robj.push_back( Pair("pos", pos) );
            int libaoid = libao_mgr::getInstance()->getChengzhangLibao(pos);
            if (pc->m_chengzhang_reward[pos-1] == 1)
            {
                //�����
                int slot = pc->m_bag.getChengzhangLibaoSlot(libaoid);
                int ret = pc->openLibao(slot, robj, true);
                if (ret == HC_SUCCESS)
                {
                    pc->m_chengzhang_reward[pos-1] = 2;
                    InsertSaveDb("replace into char_chengzhang_event (cid,pos,state) values ("
                            + LEX_CAST_STR(pc->m_id) + ","+ LEX_CAST_STR(pos) + "," + LEX_CAST_STR(pc->m_chengzhang_reward[pos-1]) + ")");
                    //���°�ť��ʾ
                    pc->notifyChengzhangState();
                    if (pos < pc->m_chengzhang_reward.size())
                    {
                        robj.push_back( Pair("next_pos", pos+1) );
                    }
                }
                return ret;
            }
            return HC_ERROR;
        }
        //��ȡV10�佫
        case 36:
        {
            if (pc->m_vip < 10 || pc->m_level < 70)
            {
                return HC_ERROR;
            }
            //ר���佫�Ƿ��Ѿ���ȡ
            if (pc->queryExtraData(char_data_type_normal, char_data_vip10_general))
            {
                return HC_ERROR;
            }
            //�佫�����ж�
            if (pc->m_generals.m_generals.size() >= pc->m_general_limit)
            {
                return HC_ERROR_TOO_MUCH_GENERALS;
            }
            pc->m_generals.Add(997, true);
            pc->setExtraData(char_data_type_normal, char_data_vip10_general, 1);
            pc->notifyVipState();
            return HC_SUCCESS;
        }
        default:
            break;
    }
    return HC_ERROR;
}

//����
int ProcessBuyXXX(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("purpose", purpose) );
    switch (purpose)
    {
        case 2://��ְ�佫
            {
                int gid = 0;
                if (cdata->CheckHasFireGeneral(id, gid))
                    return cdata->GetGenerals().Buyback(gid);
                else
                    return cdata->buyOfficalGeneral(id);
            }
            break;
        case 3://ѵ��λ
            {
                return cdata->buyTrainQue(id, robj);
            }
            break;
#if 0
        case 5://ұ��λ
            {
                return SmeltMgr::getInstance()->BuySmeltPos(cdata->m_id,id);
            }
            break;
#endif
        case 6://�̵���Ʒ
            {
                return cdata->buyShopGoods(id, robj);
            }
        case 7://����ϴ�辭
            {
                //return cdata->buyXisui(robj);
            }
            break;
        default:
            break;
    }
    return HC_ERROR;
}

//ֹͣ
int ProcessStopXXX(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("purpose", purpose) );
    switch (purpose)
    {
#if 0
        case 2://ұ��
            {
                return SmeltMgr::getInstance()->StopSmelt(cdata->m_id,id);
            }
            break;
#endif
        default:
            break;
    }
    return HC_ERROR;
}

//����
int ProcessSpeedXXX(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("purpose", purpose) );

    switch (purpose)
    {
        case 1://ѵ��
            {
                return pc->generalTrainSpeed(id);
            }
            break;
#if 0
        case 3://ұ��
            {
                return SmeltMgr::getInstance()->SpeedSmelt(pc->m_id, id);
            }
            break;
#endif
        case 4:    //������ȴ
            return RaceMgr::getInstance()->clearCD(pc->m_id, robj);
            break;
        case 5://��������
            //return RaceMgr::getInstance()->speedUp(pc->m_id, robj);
            break;
        case 6://���͸�
            return guardMgr::getInstance()->Finish(pc->m_id);
            break;
        case 7://��ȡ��
            return guardMgr::getInstance()->SpeedRobCD(pc->m_id);
            break;
        case 8://ɨ��
            return sweepMgr::getInstance()->SpeedUp(pc->m_id);
            break;
        //case 9://ͨ��
        //    return tradeMgr::getInstance()->SpeedUp(pc->m_id);
        //    break;
        case 10://ǿ����ȴ
            return pc->enhanceSpeed();
            break;
        case 11://�������
            return farmMgr::getInstance()->speedFarm(pc->m_id, id);
            break;
        case 12:    //��ó����ȴ
            return Singleton<newTradeMgr>::Instance().speedCool(*pc);

        case 13:    //����̽������
            return Singleton<corpsExplore>::Instance().speed(*pc, id);
        default:
            break;
    }
    return HC_ERROR;
}

//�����ܿ�����Ϣ
int ProcessGetOpenInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return cdata->getOpeninfo(robj);
}

//������������ʾ״̬
int ProcessGetConfirmInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array clist;
    for (int i = 0; i < iMaxGoldCostConfirm; ++i)
    {
        if (pc->m_gold_cost_comfirm[i])
        {
            json_spirit::Object obj;
            obj.push_back( Pair("type", i+1) );
            obj.push_back( Pair("enable", 1) );
            clist.push_back(obj);
        }
    }
    robj.push_back( Pair("list", clist) );
    return HC_SUCCESS;
}

//������������ʾ״̬
int ProcessSetConfirmInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array clist;
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    int enable = 0;
    READ_INT_FROM_MOBJ(enable,o,"enable");
    bool bEnable = (enable == 1);
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("enable", bEnable?1:0) );
    return cdata->enableNoConfirmGoldCost(type, bEnable);
}

//��ȡ��������ȴ���
int ProcessUpdateList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return cdata->getUpdateListCD(robj);
}

//��Ϣ�ӿ�
int ProcessRest(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    robj.push_back( Pair("purpose", purpose) );
    return cdata->rest(purpose, robj);
}

//��ѯ��Ϣ����
int ProcessQueryRestInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_buyLingOpen == 0)
    {
        return HC_ERROR;
    }
    return pc->queryRestInfo(robj);
}

/***************** ������ ******************/
//��ѯ��������
int ProcessGetRaceRankList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type) );
#ifdef OLD_RACE
    return RaceMgr::getInstance()->queryRaceList(cdata->m_id, type, 10, robj);
#else
    if (1 == type)
    {
        return RaceMgr::getInstance()->queryRaceList(cdata->m_id, type, 10, robj);
    }
    else
    {
        return RaceMgr::getInstance()->getTop20(robj);
    }
#endif
}

//��ѯ�Լ��ľ�����Ϣ
int ProcessGetRaceInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return RaceMgr::getInstance()->querySelfInfo(cdata->m_id, robj);
}

//������ս
int ProcessChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return RaceMgr::getInstance()->challenge(psession, cdata->m_id, id, robj);
}

//������ս
int ProcessBuyChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return RaceMgr::getInstance()->buyChallenge(psession, cdata->m_id, robj);
}

//��ѯ������������
int ProcessQueryRankRewards(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int rank1 = 0, rank2 = 0;
    READ_INT_FROM_MOBJ(rank1,o,"rank1");
    READ_INT_FROM_MOBJ(rank2,o,"rank2");
    return RaceMgr::getInstance()->QueryRankRewards(rank1, rank2, robj);
}

//����������Ʒ
int ProcessGetRaceGoodsList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return RaceMgr::getInstance()->getList(cdata->m_id, o, robj);
}

//���򾺼�������Ʒ
int ProcessBuyRaceGoods(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return RaceMgr::getInstance()->buy(cdata->m_id, o, robj);
}

/************************* ����ӿ� *******************************/
//�����б�
int ProcessTaskList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int page = 1, pageNums = 3;
    READ_INT_FROM_MOBJ(page,o,"page");
    READ_INT_FROM_MOBJ(pageNums,o,"pageNums");
    return taskMgr::getInstance()->getTaskList(*cdata, page, pageNums, robj);
}

//������ϸ
int ProcessTaskInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int tid = 0;
    READ_INT_FROM_MOBJ(tid,o,"id");
    return taskMgr::getInstance()->getTaskInfo(*cdata, tid, robj);
}

//�����������
int ProcessSetTaskDone(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //ֻ���ղ�������ſ���
    if (cdata->m_task._task.get()
        && cdata->m_task._task->type == task_add_to_favorites)
    {
        cdata->m_task.cur = cdata->m_task.need;
        cdata->m_task.done = true;
    }
    return HC_SUCCESS;
}


//���а�
//��ɫ����
int ProcessGetRanklist(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return splsRankings::getInstance()->getCharRankings(page, pc->m_id, robj);
}

//Ӣ������
int ProcessGetHeroRanklist(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return splsRankings::getInstance()->getHeroRankings(page, pc->m_id, robj);
}

//÷������������
int ProcessGetLotteryRanklist(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return splsRankings::getInstance()->getLotteryRankings(page, pc->m_id, robj);
}

//��Ӣս��������
int ProcessGetEliteRanklist(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return splsRankings::getInstance()->getEliteRankings(page, pc->m_id, robj);
}

//�����������
int ProcessGetPrestigeRanklist(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return splsRankings::getInstance()->getPrestigeRankings(page, pc->m_id, robj);
}

//ս������
int ProcessGetAttackRanklist(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return splsRankings::getInstance()->getAttackRankings(page, pc->m_id, robj);
}

//ս��̨����
int ProcessGetZSTRanklist(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0;
    READ_INT_FROM_MOBJ(page,o,"page")
    return splsRankings::getInstance()->getZSTRankings(page, pc->m_id, robj);
}

/************************* �ڲ��ӿ� *******************************/

//�ؿ�ս�����
int ProcessStrongholdCombatResult(Combat* pCombat, boost::shared_ptr<CharData>& cdata)
{
#ifdef DEBUG_PER
    std::list<uint64_t> times;
    times.push_back(splsTimeStamp());
#endif
    CharData* pc = cdata.get();
    //���� "�ٻ��һ��"�Ĵ���������
    if (cdata.get() && pCombat)
    {
        pc->m_last_stronghold = pCombat->m_strongholdId;
        pc->m_last_stronghold_level = pCombat->m_stronghold_level;
        pc->m_last_stronghold_mapid = pCombat->m_mapid;
        pc->m_last_stronghold_type = pCombat->m_stronghold_type;
        if (pc->m_vip < 6)
        {
            pc->m_reget_gold = 0;
        }
        else
        {
            pc->m_reget_gold = iRegetGold_First;
        }
        pc->m_reget_times = 0;
    }
    CharStrongholdData* pcStronghold = GeneralDataMgr::getInstance()->GetCharStrongholdData(pCombat->m_attacker->getCharId(), pCombat->m_mapid, pCombat->m_stageId, pCombat->m_strongholdPos).get();
    int attack_value = pc->getAttack(0);
    int generals_cnt = 0;
    json_spirit::Object generals_info;
    json_spirit::Array zhen_generals;
    boost::shared_ptr<ZhenData> zdata = pc->m_zhens.GetZhen(pc->m_zhens.GetDefault());
    if (zdata.get())
    {
        zdata->getList2(zhen_generals);
        generals_cnt = zdata->getGeneralCounts();
        generals_info.push_back( Pair("info", zhen_generals) );
    }

#ifdef DEBUG_PER
    times.push_back(splsTimeStamp());
#endif
    if (pCombat->m_state == attacker_win)
    {
        if (pCombat->m_defender->level() > cdata->m_level)
        {
            pCombat->m_result_obj.push_back( Pair("levelup", 1) );
        }
        //��ɫ�ؿ����ȵȼ�����
        cdata->level(pCombat->m_defender->level());

#ifdef DEBUG_PER
        times.push_back(splsTimeStamp());
#endif

        /***********�����õ��䴦��****************/
        lootMgr::getInstance()->getStrongholdLoots(pCombat->m_type_id, pCombat->m_getItems, pCombat->m_extra_chance);
        if (pCombat->m_getItems.size() == 0)
        {
            lootMgr::getInstance()->getWorldItemFall(pCombat->m_getItems);
        }

        //2.13���ӱ������
        lootMgr::getInstance()->getBoxItemFall(cdata->m_level,pCombat->m_getItems);


#ifdef DEBUG_PER
        times.push_back(splsTimeStamp());
#endif

        if (pcStronghold && pcStronghold->m_baseStronghold.get()
                && pcStronghold->m_state >= 1)
        {
            //������
            Item itm;
            itm.type = item_type_treasure;
            itm.id = treasure_type_supply;
            itm.nums = pcStronghold->m_baseStronghold->m_rob_supply;
            //QQ�����Ӷ������10%
            if (cdata->m_qq_yellow_level > 0)
            {
                itm.nums = 11 * itm.nums / 10;
            }
            pCombat->m_getItems.push_back(itm);

            //�Ӷ�֧������
            cdata->m_trunk_tasks.updateTask(task_rob_stronghold, pcStronghold->m_baseStronghold->m_id, 1);
            cdata->m_trunk_tasks.updateTask(task_attack_stronghold, pcStronghold->m_baseStronghold->m_id, 1);
        }
#ifdef DEBUG_PER
        times.push_back(splsTimeStamp());
#endif
        //�ؿ���ѫ����
        if (pcStronghold->m_baseStronghold.get() && pcStronghold->m_baseStronghold->m_gongxun > 0)
        {
            //int id = (pCombat->m_mapid - 1) * 3 + pCombat->m_stageId;
            //int gongxun = iStageGongxun[id - 1];
            Item item_gongxun;
            item_gongxun.type = item_type_treasure;
            item_gongxun.id = treasure_type_gongxun;
            item_gongxun.nums = pcStronghold->m_baseStronghold->m_gongxun;

            pCombat->m_getItems.push_back(item_gongxun);
        }
#ifdef DEBUG_PER
        times.push_back(splsTimeStamp());
#endif
        //������
        giveLoots(cdata, pCombat, true, give_stronghold_loot);
#ifdef DEBUG_PER
        times.push_back(splsTimeStamp());
#endif
        if (pcStronghold->m_state == 0)
        {
            bool needNotify = false;
            json_spirit::Array levelups;
            //����ս��������佫������������
            for (int i = 0; i < 9; ++i)
            {
                //�佫�ȼ�С�ڶԷ��ȼ�����������
#if 0
                if (pCombat->m_attacker->_army_data->m_generals[i] == NULL
                    || pCombat->m_attacker->_army_data->m_generals[i]->level() >= pCombat->m_defender->level()
                    || pCombat->m_attacker->_army_data->m_generals[i]->level() >= pc->m_level)
#else
                if (pCombat->m_attacker->_army_data->m_generals[i] == NULL || pCombat->m_attacker->_army_data->m_generals[i]->level() >= pCombat->m_defender->level())
#endif
                {
                    continue;
                }
                CharTotalGenerals& char_generals = cdata->GetGenerals();
                boost::shared_ptr<CharGeneralData> cg = char_generals.GetGenral(pCombat->m_attacker->_army_data->m_generals[i]->UniqueId());
                if (cg.get())
                {
                    if (0 == cg->Levelup())
                    {
                        needNotify = true;
                    }
                    json_spirit::Object gobj;
                    gobj.push_back( Pair("name", pCombat->m_attacker->_army_data->m_generals[i]->GetName()) );
                    gobj.push_back( Pair("spic", pCombat->m_attacker->_army_data->m_generals[i]->getSpic()) );
                    gobj.push_back( Pair("level", cg->m_level) );
                    gobj.push_back( Pair("color", cg->m_color) );
                    levelups.push_back(gobj);
                }
            }
            if (needNotify)
            {
                pc->NotifyZhenData();
            }
            if (levelups.size())
            {
                //�佫�����
                shhx_generl_upgrade_event::getInstance()->add_score(cdata->m_id, levelups.size());
            }

            pCombat->m_result_obj.push_back( Pair("upgrade", levelups) );

            //actͳ��
            act_to_tencent(cdata.get(),act_new_stronghold,pcStronghold->m_baseStronghold->m_id,1,generals_cnt,write(generals_info, json_spirit::raw_utf8));
        }
#ifdef DEBUG_PER
        times.push_back(splsTimeStamp());
#endif
        //�ٴλ�ý��
        pCombat->m_result_obj.push_back( Pair("gold", pc->m_reget_gold) );
        //�ؿ��ƽ�
        cdata->GetTempo().update(pCombat->m_defender->_army_data->m_charactor, true);

#ifdef DEBUG_PER
        times.push_back(splsTimeStamp());
#endif

    }
    else if (pcStronghold->m_state == 0)
    {
        //actͳ��
        act_to_tencent(cdata.get(),act_new_stronghold,pcStronghold->m_baseStronghold->m_id,0,generals_cnt,write(generals_info, json_spirit::raw_utf8));

        //��������
        if (pcStronghold->m_baseStronghold.get() && pcStronghold->m_baseStronghold->m_guide_fail > 0)
        {
            //��������
            pc->_checkGuide(pcStronghold->m_baseStronghold->m_guide_fail);
        }
    }
    pCombat->AppendResult(pCombat->m_result_obj);

#ifdef DEBUG_PER
    times.push_back(splsTimeStamp());
#endif

    //ˢ��״̬
    //cdata->refreshStates(0, 0);
    //ˢ����ҹؿ�״̬
    //�ؿ���״̬
    if (pcStronghold && pcStronghold->m_baseStronghold.get() && pcStronghold->m_baseStronghold->m_stateNum > 0)
    {
        pcStronghold->m_states.refresh();
        //����״̬��ս����Ӱ��
        updateCombatAttribute(pcStronghold->m_states, pcStronghold->m_combat_attribute);
    }
#ifdef DEBUG_PER
    times.push_back(splsTimeStamp());
#endif
    //ս����ʾ����
    if (pcStronghold && pcStronghold->m_baseStronghold.get()
        && defender_win == pCombat->m_state)
    {
        cdata->_checkNotifyFail(pcStronghold->m_baseStronghold->m_id);
    }
#ifdef DEBUG_PER
    times.push_back(splsTimeStamp());
#endif
    //��һ�δ�Ӯ�ؿ����빥��
    if (pcStronghold && pcStronghold->m_baseStronghold.get()
        && attacker_win == pCombat->m_state)
    {
        if (pcStronghold->m_state == 1)
        {
            pCombat->m_archive_report = pcStronghold->m_baseStronghold->m_raiders.addRecords(
                pCombat->m_attacker->Name(),
                pCombat->m_attacker->level(),
                pCombat->m_combat_id,
                attack_value,
                pCombat->m_attacker->DieHp());

            //�۳�����
            cdata->addTreasure(treasure_type_supply, -pcStronghold->m_baseStronghold->m_need_supply);
            add_statistics_of_treasure_cost(cdata->m_id,cdata->m_ip_address,treasure_type_supply,pcStronghold->m_baseStronghold->m_need_supply,treasure_stronghold,2,cdata->m_union_id,cdata->m_server_id);
            //�����л
            int score = pcStronghold->m_baseStronghold->m_map_id * 50 + pcStronghold->m_baseStronghold->m_stage_id * 10 + pcStronghold->m_baseStronghold->m_strongholdpos;
            if (score > 0)
                newRankings::getInstance()->updateEventRankings(cdata->m_id,rankings_event_stronghold,score);
        }
        //�ղ������ʼ��ʾ1ͼ2�ݵ�5�ؿ�
        if (pcStronghold->m_state == 1 && pcStronghold->m_baseStronghold->m_id == iCollectLibaoStronghold)
        {
            int get = pc->queryExtraData(char_data_type_normal, char_data_get_collect_reward);
            int create_time = time(NULL) - cdata->m_createTime;
            if (create_time < 86400 && get < 1)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("cmd", "queryCollectGift") );
                obj.push_back( Pair("left_time", create_time));
                obj.push_back( Pair("s", 200) );
                cdata->sendObj(obj);
            }
        }
        //��һ��3ͼ1�ݵ�6�ؿ���һ������ʯ
        if (pcStronghold->m_state == 1 && pcStronghold->m_baseStronghold->m_id == STRONGHOLD_ID(3,1,6))
        {
            cdata->giveBaoshi(14, 1, baoshi_gift);
        }
        //1ͼ2�ݵ�6�ؿ���һ��������������Ҫ
        if (pcStronghold->m_state == 1 && pcStronghold->m_baseStronghold->m_id == STRONGHOLD_ID(1,2,6))
        {
            cdata->addTreasure(treasure_type_levy_ling, 1);
        }
        //2ͼ3�ݵ�2�ؿ�������������Ҫ
        if (pcStronghold->m_state == 1 && pcStronghold->m_baseStronghold->m_id == STRONGHOLD_ID(2,3,2))
        {
            cdata->addGold(5);
        }
    }
#ifdef DEBUG_PER
    times.push_back(splsTimeStamp());
#endif
    //����³����ʧ�ܿ�������24
    if (pcStronghold && pcStronghold->m_baseStronghold.get()
        && defender_win == pCombat->m_state && pcStronghold->m_baseStronghold->m_id == 9)
    {
        cdata->_checkGuide(24);
    }

    if (pcStronghold && pcStronghold->m_baseStronghold.get())// && pcStronghold->m_state > 1)
    {
        //���ı���/����
        cdata->combatCost(pCombat->m_state == attacker_win, combat_stronghold);
    }
#ifdef DEBUG_PER
    times.push_back(splsTimeStamp());
#endif

#if 0
    //ļ������
    if (cdata->addSilver(-pCombat->m_attacker->_army_data->m_hp_cost) < 0)
    {
        cdata->m_hp_cost = pCombat->m_attacker->_army_data->m_hp_cost - cdata->silver();
        cdata->silver(0);
    }
    //��������ͳ��
    add_statistics_of_silver_cost(pc->m_id,pc->m_ip_address,pCombat->m_attacker->_army_data->m_hp_cost,silver_cost_for_stronghold);
#endif
    //�״�ռ�����CD
    if (pcStronghold->m_state <= 1)
    {
        int more_cd = pCombat->add_time() - TIME_MAX_A_COMBAT;
        if (more_cd < 0)
        {
            pc->m_fight_cd += more_cd;
        }
    }
    InsertSaveCombat(pCombat);
#ifdef DEBUG_PER
    times.push_back(splsTimeStamp());
    if (times.size()  > 1)
    {
        uint64_t timestart = *(times.begin());
        uint64_t timeend = *(times.rbegin());
        uint64_t totalcost = timeend - timestart;
        if (totalcost > 2000)
        {
            printf("total cost %ld us,", timeend-timestart);
            for (std::list<uint64_t>::iterator it = times.begin(); it != times.end(); ++it)
            {
                printf("%ld,", *it - timestart);
                timestart = *it;
            }
            printf("\n");
        }
    }
#endif
    return HC_SUCCESS;
}

//ս�����
int ProcessCombatResult(json_spirit::mObject& o)
{
    uint64_t point = 0;
    //INFO("ProcessCombatResult()"<<endl);
    READ_UINT64_FROM_MOBJ(point, o, "point");

    Combat* pCombat = reinterpret_cast<Combat*>(point);

    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_attacker->getCharId());
    if (!cdata.get())
    {
        ERR();
        return HC_ERROR;
    }
    switch (pCombat->m_type)
    {
        case combat_stronghold:
            //�ؿ�ս��
            return ProcessStrongholdCombatResult(pCombat, cdata);
        case combat_race:
            //������ս��
            return    RaceMgr::getInstance()->challengeResult(pCombat);
        case combat_boss:
            //bossս��
            return bossMgr::getInstance()->combatResult(pCombat);
        case combat_camp_race:
            //��Ӫս
            return campRaceMgr::getInstance()->combatResult(pCombat);
        case combat_guard:
            //����ս
            return guardMgr::getInstance()->combatResult(pCombat);
        //case combat_trade:
        //    //�����̶�ս
        //    return tradeMgr::getInstance()->combatResult(pCombat);
        case combat_group_copy:
            //���˸���ս��
            return groupCombatMgr::getInstance()->combatResult(pCombat);
        case combat_servant:
            //�Ҷ�ս��
            return servantMgr::getInstance()->combatResult(pCombat);
        case combat_elite:
            //��Ӣս��
            return eliteCombatMgr::getInstance()->combatResult(pCombat);
        case combat_maze:    //�Թ�С��ս��
        case combat_maze_boss://�Թ�bossս��
            //�Թ�ս��
            return Singleton<mazeMgr>::Instance().combatResult(pCombat);
        //����ս
        case combat_corps_fighting:
            return Singleton<corpsFightingMgr>::Instance().combatResult(pCombat);
        case combat_zst:
            return Singleton<zstMgr>::Instance().combatResult(pCombat);
    }
    ERR();
    return HC_ERROR;
}

//�����о����
int ProcessResearchDone(json_spirit::mObject& o)
{
    int cid = 0, sid = 0, times = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    READ_INT_FROM_MOBJ(sid, o, "sid");
    READ_INT_FROM_MOBJ(times, o, "times");
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (cdata.get())
    {
        cdata->skillResearchAdd(sid, times);
    }
    INFO("**************char:"<<cid<<"skill "<<sid<<" research done!"<<endl);
    return HC_SUCCESS;
}

//�����ʱ����
int ProcessFarmDone(json_spirit::mObject& o)
{
    int cid = 0, pos = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    READ_INT_FROM_MOBJ(pos, o, "pos");
    farmMgr::getInstance()->FarmDone(cid, pos);
    return HC_SUCCESS;
}

//ѵ����ʱ����
int ProcessTrainDone(json_spirit::mObject& o)
{
    int cid = 0, pos = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    READ_INT_FROM_MOBJ(pos, o, "pos");
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (cdata.get()
        && pos > 0 && pos <= (int)cdata->m_train_queue.size()
        && cdata->m_train_queue[pos - 1].pos == pos && cdata->m_train_queue[pos - 1].general.get())
    {
        if ((cdata->m_train_queue[pos - 1].end_time - time(NULL)) <= 0)
        {
            cdata->m_train_queue[pos - 1].state = 1;
            cdata->m_train_queue[pos - 1].general.reset();
            cdata->m_train_queue[pos - 1].start_time = 0;
            cdata->m_train_queue[pos - 1].end_time = 0;
            cdata->m_train_queue[pos - 1]._uuid = boost::uuids::nil_uuid();
            cdata->m_train_queue[pos - 1].save();
            return 0;
        }
    }
    return -1;
}

#if 0
//ұ����ʱ����
int ProcessSmeltDone(json_spirit::mObject& o)
{
    int cid = 0, pos = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    READ_INT_FROM_MOBJ(pos, o, "pos");

    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        return HC_ERROR;
    }
    //pos�Ƿ���ȷ
    if (pos > smelt_queue_max || pos <= 0)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<charSmeltData> sp_smelt = SmeltMgr::getInstance()->getCharSmeltData(cid);
    if (!sp_smelt.get())
    {
        return HC_ERROR;
    }
    charSmeltData& smelt = *(sp_smelt.get());
    //���Ǳ�ռ�õ�״̬
    if (smelt.SmeltList[pos - 1].state != 2)
    {
        return HC_ERROR;
    }
    cdata->addTreasure(smelt.SmeltList[pos - 1].smelt.quality, smelt.SmeltList[pos - 1].smelt.out_put);
    //������ͳ��
    add_statistics_of_treasure_cost(cdata->m_id,cdata->m_ip_address,smelt.SmeltList[pos - 1].smelt.out_put,treasure_smelt,smelt.SmeltList[pos - 1].smelt.quality);
    smelt.SmeltList[pos - 1].stop();
    smelt.SmeltList[pos - 1].save();
    return HC_SUCCESS;
}
#endif

//��������
int ProcessSaveDb(json_spirit::mObject& o)
{
    //INFO("ProcessSaveDb()"<<endl);
    int save_all = 0;
    READ_INT_FROM_MOBJ(save_all, o, "all");
    GeneralDataMgr::getInstance()->SaveDb(save_all);
    return HC_SUCCESS;
}

//����ɾ����ɫ
int ProcessRealyDeleteChar(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    DeleteChar(cid);
    return HC_SUCCESS;
}

//�������ݿ�����
int ProcessKeepDb(json_spirit::mObject& o)
{
    GetDb();
    return HC_SUCCESS;
}

//����
int ProcessOffline(json_spirit::mObject& o)
{
    std::string account = "";
    READ_STR_FROM_MOBJ(account,o,"account");
    boost::shared_ptr<OnlineUser> ou = GeneralDataMgr::getInstance()->GetAccount(account);
    if (ou.get())
    {
        GeneralDataMgr::getInstance()->Logout(ou);
    }
    return HC_SUCCESS;
}

//������ɵĴ���
int ProcessGuardDone(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    guardMgr::getInstance()->GuardDone(cid);
    return HC_SUCCESS;
}

//ɨ���Ĵ���
int ProcessSweepDone(json_spirit::mObject& o)
{
    int cid = 0, total_time = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    READ_INT_FROM_MOBJ(total_time, o, "total_time");
    sweepMgr::getInstance()->Done(cid,total_time);
    return HC_SUCCESS;
}

//�Ҷ�������ɵĴ���
int ProcessServantDone(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    servantMgr::getInstance()->Done(cid);
    return HC_SUCCESS;
}

//ս����ӹ��ڵĴ���
int ProcessFruitDone(json_spirit::mObject& o)
{
    int cid = 0, id = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    READ_INT_FROM_MOBJ(id, o, "id");
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    horseMgr::getInstance()->fruitDone(cid, id);
    return HC_SUCCESS;
}

//ȫ��ظ�����
int ProcessRecoverLing(json_spirit::mObject& o)
{
    //INFO("ProcessSaveDb()"<<endl);
    int counts = 2;
    READ_INT_FROM_MOBJ(counts, o, "add");
    GeneralDataMgr::getInstance()->addLing(counts);
    return HC_SUCCESS;
}

//���а�
int ProcessRankingsEvent(json_spirit::mObject& o)
{
    uint64_t pvoid = 0;
    READ_UINT64_FROM_MOBJ(pvoid,o,"pointer");
    if (pvoid)
    {
        rankings_event* pE = reinterpret_cast<rankings_event*>(pvoid);
        //cout<<"ProcessRankingsEvent(),type="<<pE->type<<endl;

        switch (pE->type)
        {
            case char_rankings:            //������а�
            case hero_rankings:            //Ӣ�۰�
            case lottery_rankings:        //������
                splsRankings::getInstance()->updateRankingsEvent(pE);
                giveRankingsEventReward(pE);
                break;
            case boss_rankings:            //��ħ��
                bossMgr::getInstance()->updateRankingsEvent(pE);
                giveRankingsEventReward(pE);
                break;
            case camp_race_rankings:    //��Ӫ���ư�
                campRaceMgr::getInstance()->updateRankingsEvent(pE);
                giveRankingsEventReward(pE);
                break;
        }
        delete pE;
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

//���¼������ݿ�
int ProcessReload(json_spirit::mObject& o)
{
    //INFO("ProcessSaveDb()"<<endl);
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    GeneralDataMgr::getInstance()->reload(type);
    return HC_SUCCESS;
}

int ProcessBroadCastMsg(json_spirit::mObject& o)
{
    std::string msg = "";
    READ_STR_FROM_MOBJ(msg,o,"msg");
    if (msg != "")
    {
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }
    return HC_SUCCESS;
}

//ÿ�ո���
int dailyUpdate()
{
    //���ڱ仯
    GeneralDataMgr::getInstance()->updateSeason();

    //������
    RaceMgr::getInstance()->resetAll();

    //�����Ϣ����
    //������ˢ�´���
    //�ؿ�ʣ�๥�����
    GeneralDataMgr::getInstance()->resetAll();

    //����
    farmMgr::getInstance()->resetAll();
    guardMgr::getInstance()->resetAll();

    //�Ҷ���������
    servantMgr::getInstance()->resetAll();

    //��������
    groupCopyMgr::getInstance()->reset();

    //����ν�����
    #if 0
    if (GeneralDataMgr::getInstance()->getSeason() == 2
        || GeneralDataMgr::getInstance()->getSeason() == 4)
    {
        farmMgr::getInstance()->resetRipe();
        InsertSaveDb("TRUNCATE TABLE char_year_temp");
    }
    #endif

    //�����������
    online_gift_mgr::getInstance()->resetAll();

    //ͨ��ˢ��
    //tradeMgr::getInstance()->refreshAll();
    //��Ӣս������
    eliteCombatMgr::getInstance()->reset();

#ifdef QQ_PLAT
    //������������
    Singleton<inviteMgr>::Instance().daily_reset();
#endif

    //�ճ�����
    Singleton<new_event_mgr>::Instance().dailyReset();

    Singleton<zstMgr>::Instance().resetAll();

    return HC_SUCCESS;
}

//�ⲿ��ʱ�¼�����
int ProcessScheduleEvent(json_spirit::mObject& o)
{
    uint64_t start_time = 0;
    if (g_print_debug_info > 1)
    {
        start_time = splsTimeStamp();
    }
    std::string event = "";
    READ_STR_FROM_MOBJ(event,o,"event");

    int cmd = getAdminCmds(event);
    //cout<<"ProcessScheduleEvent,event:"<<event<<endl;
    switch (cmd)
    {
        //ÿ��ˢ��
        case admin_daily_reset:
        {
            INFO(" ************ daily_reset at "<<time(NULL)<<" ****************"<<endl);
            dailyUpdate();
            break;
        }
        //��ʱ�ָ�����
        case admin_recover_ling:
        {
            INFO(" ************ recover_ling at "<<time(NULL)<<" ****************"<<endl);
            int counts = 1;
            READ_INT_FROM_MOBJ(counts,o,"param1");
            GeneralDataMgr::getInstance()->addLing(counts);
            break;
        }
        //���þ����
        case admin_reset_welfare:
        {
            GeneralDataMgr::getInstance()->resetWelfare();
            break;
        }
        //���þ����2
        case admin_reset_welfare2:
        {
            GeneralDataMgr::getInstance()->resetWelfare2();
            break;
        }
        //�����������
        case admin_save_db:
        {
            //cout<<" ************ save_db at "<<time(NULL)<<" ****************"<<endl;
            int save_all = 0;
            READ_INT_FROM_MOBJ(save_all,o,"param1");
            GeneralDataMgr::getInstance()->SaveDb(save_all);
            break;
        }
        //���½�ɫ����
        case admin_updateRank:
        {
            //cout<<" *********** updateRank *********** "<<endl;
            splsRankings::getInstance()->updateRankings(rankings_type_char);
            break;
        }
        //����Ӣ������
        case admin_updateHeroRank:
        {
            //cout<<" *********** updateHeroRank *********** "<<endl;
            splsRankings::getInstance()->updateRankings(rankings_type_hero);
            break;
        }
        //���¾�������
        case admin_updateCorpsRank:
        {
            corpsMgr::getInstance()->updateRank();
            break;
        }
        //����bossս��
        case admin_openBoss:
        {
            int boss = 0;
            READ_INT_FROM_MOBJ(boss,o,"param1");
            int last_mins = 120;
            READ_INT_FROM_MOBJ(last_mins,o,"param2");
            bossMgr::getInstance()->openBoss(boss, last_mins);
            break;
        }
        //�ر�bossս��
        case admin_closeBoss:
        {
            int boss = 0;
            READ_INT_FROM_MOBJ(boss,o,"param1");
            int corps = 0;
            READ_INT_FROM_MOBJ(corps,o,"param2");
            bossMgr::getInstance()->closeBoss(boss, corps);
            break;
        }
        //��¼��������
        case admin_saveOnlines:
        {
            GeneralDataMgr::getInstance()->getTotalOnline(true);
            break;
        }
        //ˢ���̵�
        //case admin_resetShop:
        //{
        //    GeneralDataMgr::getInstance()->resetShopGoods();
        //    break;
        //}
        //ˢ������
        case admin_resetFarm:
        {
            //farmMgr::getInstance()->resetFarmTask();
            break;
        }
        case admin_raceAwards:
        {
            int race_reward_time = GeneralDataMgr::getInstance()->GetRaceRewardTime();
            if (time(NULL) >= race_reward_time)
            {
                //ÿ�궬�ļ�����23�㷢����������
                RaceMgr::getInstance()->yearAwards();
                GeneralDataMgr::getInstance()->updateRaceRewardTime();
            }
            break;
        }
        case admin_debug_raceAwards:
        {
            //ÿ�궬�ļ�����23�㷢����������
            RaceMgr::getInstance()->yearAwards();
            break;
        }
        case admin_guardAwards:
        {
            guardMgr::getInstance()->raceAwards();
            newRankings::getInstance()->RankingsReward();
            break;
        }
        case admin_openCampRace:
        {
            int last_mins = 0;
            READ_INT_FROM_MOBJ(last_mins,o,"param1");
            campRaceMgr::getInstance()->open(last_mins);
            break;
        }
        case admin_closeCampRace:
        {
            //�ر���Ӫս
            campRaceMgr::getInstance()->close();
            break;
        }
        //ÿ��һ�賿0�c����
        // 1܊�F���ܻ�þ���
        // 2���L÷���ה��e��
        case admin_WeekReset:
        {
            corpsMgr::getInstance()->weekReset();
            GeneralDataMgr::getInstance()->weekReset();
            break;
        }
        //����ÿ�ո��±���ؕ�I
        case admin_corpsDailyReset:
        {
            corpsMgr::getInstance()->dailyReset();
            break;
        }
        //��������Ա�ӽ��
        case admin_addGold:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            int counts = 0;
            READ_INT_FROM_MOBJ(counts,o,"param2");
            if (counts <= 0)
            {
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
                if (cdata.get())
                {
                    if ((cdata->gold() + counts) < 0)
                    {
                        cdata->gold(0);
                    }
                    else
                    {
                        cdata->addGold(counts);
                    }
                    cdata->NotifyCharData();
                }
            }
            else
            {
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
                if (cdata.get())
                {
                    //��һ��ͳ��
                    add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address,counts,gold_get_gm, cdata->m_union_id, cdata->m_server_id);
#ifdef QQ_PLAT
                    gold_get_tencent(cdata.get(),counts,2);
#endif
                    cdata->addGold(counts);
                    cdata->NotifyCharData();
                }
            }
            break;
        }
        //������
        case admin_addSilver:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            int counts = 0;
            READ_INT_FROM_MOBJ(counts,o,"param2");
            if (counts <= 0)
            {
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
                if (cdata.get())
                {
                    if ((cdata->silver() + counts) < 0)
                    {
                        cdata->silver(0);
                    }
                    else
                    {
                        cdata->addSilver(counts);
                    }
                    cdata->NotifyCharData();
                }
            }
            else
            {
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
                if (cdata.get())
                {
                    cdata->addSilver(counts);
                    cdata->NotifyCharData();
                }
            }
            break;
        }
        //�Ӿ���
        case admin_addLing:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            int counts = 0;
            READ_INT_FROM_MOBJ(counts,o,"param2");
            if (counts <= 0)
            {
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
                if (cdata.get())
                {
                    if ((cdata->ling() + counts) < 0)
                    {
                        cdata->ling(0);
                    }
                    else
                    {
                        cdata->addLing(counts);
                    }
                    cdata->NotifyCharData();
                }
            }
            else
            {
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
                if (cdata.get())
                {
                    cdata->addLing(counts);
                    cdata->NotifyCharData();
                }
            }
            break;
        }
        case admin_openDebug:
        {
            start_time = splsTimeStamp();
            int level = 1;
            READ_INT_FROM_MOBJ(level,o,"param1");
            g_print_debug_info = level;
            break;
        }
        case admin_heartBeat:
        {
            //����
            GeneralDataMgr::getInstance()->HeartBeat();
            break;
        }
        //�������˸���
        case admin_openGroupCopy:
        {
            int openMins = 60;
            READ_INT_FROM_MOBJ(openMins,o,"param1");
            groupCopyMgr::getInstance()->open(openMins);
            break;
        }
        //�رն��˸���
        case admin_closeGroupCopy:
        {
            groupCopyMgr::getInstance()->close();
            break;
        }
        //�����ؿ�����
        case admin_setTempo:
        {
            int cid = 0, level = 1;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(level,o,"param2");
            GeneralDataMgr::getInstance()->updateTempo(cid, level);
            break;
        }
        //��Ӣ��
        case admin_addGeneral:
        {
            int cid = 0, gid = 1;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(gid,o,"param2");

            std::string fac_string = "";
            READ_STR_FROM_MOBJ(fac_string,o,"extra");

            bool setFac = false;
            double fac_a = 0.0;
            double fac_b = 0.0;
            if (fac_string != "")
            {
                json_spirit::mValue value;
                json_spirit::read(fac_string, value);
                if (value.type() == obj_type)
                {
                    mObject& mobj = value.get_obj();
                    setFac = true;
                    READ_REAL_FROM_MOBJ(fac_a,mobj,"a");
                    READ_REAL_FROM_MOBJ(fac_b,mobj,"b");
                }
            }
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                cdata->m_generals.Add(gid, true, 1, setFac, fac_a);
            }
            break;
        }
        //��װ��
        case admin_addEquiptment:
        {
            int cid = 0, eid = 1;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(eid,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                std::string extraMsg = "";
                cdata->addEquipt(eid, extraMsg);
            }
            break;
        }
        //�Ӽ���
        case admin_setSkill:
        {
            int cid = 0, sid = 0, level = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(sid,o,"param2");
            READ_INT_FROM_MOBJ(level,o,"param3");
            if (level > iMaxCharLevel*10)
            {
                level = iMaxCharLevel * 10;
            }
            if (level > 0)
            {
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
                if (cdata.get())
                {
                    if (sid == 0)
                    {
                        for (int skill = 1; skill <= total_skill_type; ++skill)
                        {
                            cdata->setSkillLevel(skill, level);
                        }
                    }
                    else if (sid > 0 && sid <= total_skill_type)
                    {
                        cdata->setSkillLevel(sid, level);
                    }
                }
            }
            break;
        }
        //������
        case admin_setZhen:
        {
            int cid = 0, zid = 0, level = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(zid,o,"param2");
            READ_INT_FROM_MOBJ(level,o,"param3");
            if (level > 0 && zid > 0)
            {
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
                if (cdata.get())
                {
                    while (cdata->m_zhens.Levelup(zid, level) != -1)
                    {
                        ;
                    }
                }
            }
            break;
        }
        //�ӵ���
        case admin_addTreasure:
        {
            int cid = 0, tid = 0, count = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(tid,o,"param2");
            READ_INT_FROM_MOBJ(count,o,"param3");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                if (cdata->addTreasure(tid, count) < 0)
                {
                    cdata->addTreasure(tid, -cdata->treasureCount(tid));
                }
            }
            break;
        }
        //�佫�ȼ�
        case admin_generalLevel:
        {
            int cid = 0, gid = 0, level = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(gid,o,"param2");
            READ_INT_FROM_MOBJ(level,o,"param3");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                if (0 == level)
                {
                    level = cdata->m_level;
                }
                else if (level > cdata->m_level || level < 0)
                {
                    level = cdata->m_level;
                }
                if (0 == gid)
                {
                    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = cdata->m_generals.m_generals.begin();
                    while (it != cdata->m_generals.m_generals.end())
                    {
                        if (it->second.get())
                        {
                            while (it->second->m_level < level)
                            {
                                it->second->Levelup(level);
                            }
                        }
                        ++it;
                    }
                }
                else
                {
                    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = cdata->m_generals.m_generals.begin();
                    while (it != cdata->m_generals.m_generals.end())
                    {
                        if (it->second.get() && it->second->m_id == gid)
                        {
                            while (it->second->m_level < level)
                            {
                                it->second->Levelup(level);
                            }
                            break;
                        }
                        ++it;
                    }
                }
                cdata->NotifyZhenData();
            }
            break;
        }
        //�佫�ɳ���
        case admin_generalChengzhang:
        {
            int cid = 0, gid = 0, chengzhang = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(gid,o,"param2");
            READ_INT_FROM_MOBJ(chengzhang,o,"param3");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                if (chengzhang <= 0)
                {
                    chengzhang = 0;
                }
                if (0 == gid)
                {
                    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = cdata->m_generals.m_generals.begin();
                    while (it != cdata->m_generals.m_generals.end())
                    {
                        if (it->second.get())
                        {
                            if (chengzhang > iChengZhangMax[it->second->m_color])
                            {
                                chengzhang = iChengZhangMax[it->second->m_color];
                            }
                            it->second->m_chengzhang = chengzhang;
                            //�ȼ��ӳ����¼���
                            it->second->m_add = 0.0;
                            for (int level = 2; level <= it->second->m_level; ++level)
                            {
                                double add = it->second->m_chengzhang;
                                int temp = (int)(add * 100);
                                it->second->m_add += (double)temp/100;
                            }
                            it->second->updateAttribute();
                            cdata->NotifyZhenData();
                            //������׃��
                            cdata->set_attack_change();
                            //��ս��
                            it->second->general_change = true;
                            it->second->reborn_change = true;
                        }
                        ++it;
                    }
                }
                else
                {
                    std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = cdata->m_generals.m_generals.begin();
                    while (it != cdata->m_generals.m_generals.end())
                    {
                        if (it->second.get() && it->second->m_id == gid)
                        {
                            if (chengzhang > iChengZhangMax[it->second->m_color])
                            {
                                chengzhang = iChengZhangMax[it->second->m_color];
                            }
                            it->second->m_chengzhang = chengzhang;
                            //�ȼ��ӳ����¼���
                            it->second->m_add = 0.0;
                            for (int level = 2; level <= it->second->m_level; ++level)
                            {
                                double add = it->second->m_chengzhang;
                                int temp = (int)(add * 100);
                                it->second->m_add += (double)temp/100;
                            }
                            it->second->updateAttribute();
                            cdata->NotifyZhenData();
                            //������׃��
                            cdata->set_attack_change();
                            //��ս��
                            it->second->general_change = true;
                            it->second->reborn_change = true;
                            break;
                        }
                        ++it;
                    }
                }
                cdata->NotifyZhenData();
            }
            break;
        }
        //�����ʺ�
        case admin_freeze_account:
        {
            std::string account = "";
            READ_STR_FROM_MOBJ(account,o,"extra");
            if (account != "")
            {
                boost::shared_ptr<OnlineUser> ou = GeneralDataMgr::getInstance()->GetAccount(account);
                if (ou.get())
                {
                    GeneralDataMgr::getInstance()->Logout(ou);
                }
            }
            break;
        }
        //����
        case admin_gag_char:
        {
            int cid = 0, endtime = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(endtime,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
                if (account.get())
                {
                    account->m_gag_end_time = endtime;
                }
            }
            break;
        }
        //����VIP�ȼ�
        case admin_set_vip:
        {
            int cid = 0, vip = 1;
            READ_INT_FROM_MOBJ(cid, o, "param1");
            READ_INT_FROM_MOBJ(vip, o, "param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                int old_vip = cdata->m_vip;
                cdata->m_vip = vip;

                if (old_vip != cdata->m_vip)
                {
                    //if (2 == cdata->m_vip || 4 == cdata->m_vip)
                    //{
                    //    cdata->NotifyCharState();
                    //}
                    InsertSaveDb("update char_data set vip='" + LEX_CAST_STR(cdata->m_vip)
                        + "' where cid=" + LEX_CAST_STR(cdata->m_id));

                    //����VIP��������ȡ״̬
                    std::map<int,CharVIPPresent>::iterator it = cdata->m_vip_present.begin();
                    while (it != cdata->m_vip_present.end())
                    {
                        if (it->first <= cdata->m_vip && it->second.state == 0)
                        {
                            it->second.state = 1;
                        }
                        ++it;
                    }
                    //֪ͨ�����ť״̬
                    cdata->notifyVipState();
                }
            }
            break;
        }
        //����װ��ǿ���ȼ�
        case admin_setEquiptment:
        {
            int cid = 0, eid = 0, level = 0;
            READ_INT_FROM_MOBJ(cid, o, "param1");
            READ_INT_FROM_MOBJ(eid, o, "param2");
            READ_INT_FROM_MOBJ(level, o, "param3");
            CharData* cdata = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (cdata)
            {
                cdata->setEquipmentLevel(eid, level);
            }
            break;
        }
        //���ñ����ȼ�
        case admin_setWeapon:
        {
            int cid = 0, wtype = 0, quality = 0, level = 0;
            READ_INT_FROM_MOBJ(cid, o, "param1");
            READ_INT_FROM_MOBJ(wtype, o, "param2");
            READ_INT_FROM_MOBJ(quality, o, "param3");
            READ_INT_FROM_MOBJ(level, o, "param4");
            CharData* cdata = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (cdata)
            {
                cdata->setWeaponLevel(wtype, quality, level);
            }
            break;
        }
        //��������
        case admin_setPrestige:
        {
            int cid = 0, prestige = 0;
            READ_INT_FROM_MOBJ(cid, o, "param1");
            READ_INT_FROM_MOBJ(prestige, o, "param2");
            if (prestige < 0)
            {
                prestige = 0;
            }
            CharData* cdata = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (cdata)
            {
                if (prestige < cdata->m_prestige)
                {
                    cdata->m_prestige = prestige;
                }
                else
                {
                    cdata->m_prestige = prestige;
                }
                cdata->m_officalcanlevelup = cdata->OfficalLevelUpState();
                InsertSaveDb("update char_data set prestige='" + LEX_CAST_STR(cdata->m_prestige)
                            + "',official='" + LEX_CAST_STR(cdata->m_offical)
                            + "' where cid=" + LEX_CAST_STR(cdata->m_id));
            }
            break;
        }
        //��������
        case admin_addPrestige:
        {
            int cid = 0, prestige = 0;
            READ_INT_FROM_MOBJ(cid, o, "param1");
            READ_INT_FROM_MOBJ(prestige, o, "param2");
            if (prestige < 0)
            {
                prestige = 0;
            }
            CharData* cdata = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (cdata)
            {
                cdata->m_prestige += prestige;
                if (cdata->m_prestige < 0)
                {
                    cdata->m_prestige = 0;
                }
                else
                {
                    cdata->m_officalcanlevelup = cdata->OfficalLevelUpState();
                }
                InsertSaveDb("update char_data set prestige='" + LEX_CAST_STR(cdata->m_prestige)
                        + "',official='" + LEX_CAST_STR(cdata->m_offical)
                        + "' where cid=" + LEX_CAST_STR(cdata->m_id));
            }
            break;
        }
        //���ñ���ȼ�
        case admin_addBaowu:
        {
            int cid = 0, counts = 1;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(counts,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                for (int i = 0; i < counts; ++i)
                {
                    std::string general_name = "";
                    std::string baowu_name = "";
                    cdata->m_generals.UpdateTreasure(general_name, baowu_name);
                }
            }
            break;
        }
        //÷����������佫
        case admin_lottery_get:
        {
            int cid = 0, type = 1;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(type,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                Item item;
                item.id = 20;
                item.type = item_type_general;
                item.nums = 1;

                int score_add = 0;
                switch (type)
                {
                    default:
                    case 1:
                        item.id = 20;
                        score_add = 5;
                        break;
                    case 2:
                        item.id = 30;
                        score_add = 5;
                        break;
                    case 3:
                        score_add = 5;
                        item.id = 54;
                        break;
                    case 4:
                        item.id = 53;
                        score_add = 10;
                        break;
                }
                //�ܻ���
                int score = cdata->queryExtraData(char_data_type_week, char_data_extra_lottery_score) + score_add;
                //�ܻ���
                int total_score = cdata->queryExtraData(char_data_type_normal, char_data_lottery_total_score) + score_add;

                //�����ܻ���
                cdata->setExtraData(char_data_type_week, char_data_extra_lottery_score, score);
                //�����ܻ���
                cdata->setExtraData(char_data_type_normal, char_data_lottery_total_score, total_score);

                Singleton<lotteryMgr>::instance()->addLotteryNotice("<font color=\"#00ff00\">" + cdata->m_name + "</font>", item.toString(true));

                //����佫ȫ���㲥 (�����ֳ壬���ã��˽���)
                if (item.type == item_type_general)// && (item.id == 53 || item.id == 20 || item.id == 30))
                {
                    std::string msg = strLotteryGetHeroMsg;
                    str_replace(msg, "$W", cdata->m_name);
                    boost::shared_ptr<GeneralTypeData> gr = GeneralDataMgr::getInstance()->GetBaseGeneral(item.id);
                    if (gr.get())
                    {
                        str_replace(msg, "$H", "<font color=\"#ffffff\">" + gr->m_name + "</font>");
                        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                    }
                }

                std::list<Item> getItems;
                getItems.push_back(item);
                //������
                giveLoots(cdata.get(), getItems, 0, cdata->m_level, 0, NULL, NULL, true, give_lottery);

                //������˼�¼
                Singleton<lotteryMgr>::instance()->addLotteryRecord(cdata->m_id, 1, getItems);
            }
            break;
        }
        //���¼���������
        case admin_reload_filter:
        {
            Forbid_word_replace::getInstance()->reload();
        }
        //�������ϴ���
        case admin_clear_wash:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                CharData* pc = cdata.get();
                pc->m_generals.clearWash();
            }
            break;
        }
        //�����佫ϴ���
        case admin_set_wash:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                CharData* pc = cdata.get();
                int gid = 0;
                READ_INT_FROM_MOBJ(gid,o,"param2");
                int xisui = 0;
                READ_INT_FROM_MOBJ(xisui,o,"param3");

                if (xisui > 0)
                {
                    pc->m_generals.modifyGeneral(gid, xisui, xisui, xisui);
                }
                else
                {
                    std::string wash_string = "";
                    READ_STR_FROM_MOBJ(wash_string,o,"extra");
                    if (wash_string != "")
                    {
                        //cout<<"set growth :"<<fac_string<<endl;
                        json_spirit::mValue value;
                        json_spirit::read(wash_string, value);
                        if (value.type() == obj_type)
                        {
                            mObject& mobj = value.get_obj();
                            int t = 0, z = 0, y = 0;
                            READ_INT_FROM_MOBJ(t,mobj,"t");
                            READ_INT_FROM_MOBJ(z,mobj,"z");
                            READ_INT_FROM_MOBJ(y,mobj,"y");
                            pc->m_generals.modifyGeneral(gid, t, z, y);
                        }
                    }
                }
            }
            break;
        }
        //�����佫�ɳ�
        case admin_set_growth:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                CharData* pc = cdata.get();
                int gid = 0;
                READ_INT_FROM_MOBJ(gid,o,"param2");
                std::string fac_string = "";
                READ_STR_FROM_MOBJ(fac_string,o,"extra");
                if (fac_string != "")
                {
                    json_spirit::mValue value;
                    json_spirit::read(fac_string, value);
                    if (value.type() == obj_type)
                    {
                        mObject& mobj = value.get_obj();
                        double fac_a = 0.0;
                        READ_REAL_FROM_MOBJ(fac_a,mobj,"a");
                        pc->m_generals.modifyGeneralGrowth(gid, fac_a);
                    }
                }
            }
            break;
        }
        //ɾ���佫
        case admin_del_general:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                CharData* pc = cdata.get();
                int gid = 0;
                READ_INT_FROM_MOBJ(gid,o,"param2");
                pc->m_generals.deleteGenral(gid);
            }
            break;
        }
        //�޸�ս��
        case admin_set_horse:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                CharData* pc = cdata.get();
                int horseId = 0;
                READ_INT_FROM_MOBJ(horseId,o,"param2");
                int exp = 0;
                READ_INT_FROM_MOBJ(exp,o,"param3");
                horseMgr::getInstance()->setHorse(*pc, horseId, exp);
            }
            break;
        }
        //�޸ľ��ŵȼ��;���
        case admin_set_corps:
        {
            int corps = 0;
            READ_INT_FROM_MOBJ(corps,o,"param1");
            int level = 1;
            READ_INT_FROM_MOBJ(level,o,"param2");
            if (level < 1)
            {
                level = 1;
            }
            int exp = 0;
            READ_INT_FROM_MOBJ(exp,o,"param3");
            int weekExp = 0;
            READ_INT_FROM_MOBJ(weekExp,o,"param4");
            corpsMgr::getInstance()->setCorpsExp(corps, level, exp, weekExp);
            break;
        }
        //�޸Ľ�ɫ���Ź���
        case admin_set_corps_contribute:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                int contribute = 0;
                READ_INT_FROM_MOBJ(contribute,o,"param2");
                corpsMgr::getInstance()->setCharContribution(*(cdata.get()), contribute);
            }
            break;
        }
        //�޸�÷����������
        case admin_set_lottery_score:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            int score = 0;
            READ_INT_FROM_MOBJ(score,o,"param2");
            int total_score = 0;
            READ_INT_FROM_MOBJ(total_score,o,"param3");
            Singleton<lotteryMgr>::instance()->setLotteryScore(cid, score, total_score);
            break;
        }
        //�����츳
        case admin_setGenius:
        {
            int cid = 0, gid = 0, genius_id = 0, pos = 1;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(gid,o,"param2");
            READ_INT_FROM_MOBJ(genius_id,o,"param3");
            READ_INT_FROM_MOBJ(pos,o,"param4");
            geniusMgr::getInstance()->setGenius(cid,gid,genius_id,pos);
            break;
        }
        //�����ֵ����
        case admin_clear_recharge_event_reward:
        {
            int type = 1;
            READ_INT_FROM_MOBJ(type,o,"param1");
            recharge_event_mgr::getInstance()->reset(type);
            break;
        }
        //���¼��س�ֵ�
        case admin_reload_recharge_event:
        {
            int type = 0;
            READ_INT_FROM_MOBJ(type,o,"param1");
            int clear = 0;
            READ_INT_FROM_MOBJ(clear,o,"param2");
            recharge_event_mgr::getInstance()->reload(type);
            if (clear)
            {
                recharge_event_mgr::getInstance()->reset(type);
            }
            break;
        }
        case admin_gm_reward:
        {
            GeneralDataMgr::getInstance()->GM_reward();
            break;
        }
        case admin_checkTmpVip:
        {
            GeneralDataMgr::getInstance()->checkTmpVIP();
            break;
        }
        case admin_open_wash_event:
        {
            int discount = 100;
            READ_INT_FROM_MOBJ(discount,o,"param1");
            if (discount >= 1 && discount <= 100 &&g_wash_discount != discount)
            {
                GeneralDataMgr::getInstance()->setInt("wash_discount", discount);
                g_wash_discount = discount;
                for (int i = 0; i < 5; ++i)
                {
                    g_wash_real_cost[i] = iWashConfig[i][1] * g_wash_discount / 100;
                    if (g_wash_real_cost[i] < 1)
                    {
                        g_wash_real_cost[i] = 1;
                    }
                }

                if (discount == 100)
                {
                    //�����
                    GeneralDataMgr::getInstance()->broadCastSysMsg(strWashEventClose, -1);
                }
                else
                {
                    //�����
                    GeneralDataMgr::getInstance()->broadCastSysMsg(strWashEventOpen, -1);
                }
            }
            break;
        }

        //�̵���ۻ
        case admin_open_shop_discount:
        {
            int discount = 100;
            READ_INT_FROM_MOBJ(discount,o,"param1");
            if (discount >= 1 && discount <= 100 &&g_shop_discount != discount)
            {
                GeneralDataMgr::getInstance()->setInt("shop_discount", discount);
                g_shop_discount = discount;
                Singleton<shopMgr>::Instance().updateShopPrice(discount);
            }
            break;
        }
        //�̵�ˢ�´�������
        case admin_shop_refresh_more:
        {
            int more = 1;
            READ_INT_FROM_MOBJ(more,o,"param1");
            if (more > 1)
            {
                g_shop_refresh_more = more;
                GeneralDataMgr::getInstance()->setInt("shop_refresh", more);
            }
            break;
        }
        //ǿ��װ�����ۻ
        case admin_open_enhance_equipment_event:
        {
            int discount = 88, discount2 = 100;
            READ_INT_FROM_MOBJ(discount,o,"param1");
            READ_INT_FROM_MOBJ(discount2,o,"param2");
            if (discount >= 1 && discount <= 100 && equipmentUpgrade::getInstance()->Discount() != discount)
            {
                GeneralDataMgr::getInstance()->setInt("enhance_discount", discount);
                equipmentUpgrade::getInstance()->setDiscount(discount);
                //Singleton<shopMgr>::Instance().updateShopPrice(discount);
            }
            if (discount2 >= 1 && discount2 <= 100 && g_equiptment_enhance_discount != discount2)
            {
                GeneralDataMgr::getInstance()->setInt("enhance_discount2", discount2);
                g_equiptment_enhance_discount = discount2;
                //Singleton<shopMgr>::Instance().updateShopPrice(discount);
            }
            break;
        }
        //�Ҷ�˫���
        case admin_open_servant_event:
        {
            int state = 1;
            READ_INT_FROM_MOBJ(state,o,"param1");
            if (state)
            {
                servantMgr::getInstance()->setFactor(200);
            }
            else
            {
                servantMgr::getInstance()->setFactor(100);
            }
            break;
        }
        //ս�������
        case admin_open_horse_event:
        {
            int total = 0;
            int gold_time = 0;
            READ_INT_FROM_MOBJ(total,o,"param1");
            READ_INT_FROM_MOBJ(gold_time,o,"param2");
            horseMgr::getInstance()->setHorseTrainTimes(total, gold_time);
            break;
        }
        //ս�������ۿ�
        case admin_horse_discount:
        {
            int discount = 100;
            READ_INT_FROM_MOBJ(discount,o,"param1");
            if (discount >= 1 && discount <= 100 &&iHorseTrainDiscount != discount)
            {
                GeneralDataMgr::getInstance()->setInt("horse_discount", discount);
                iHorseTrainDiscount = discount;
            }
            break;
        }
        //���VIP4�
        case admin_open_free_vip4:
        {
            int day = 1;
            READ_INT_FROM_MOBJ(day,o,"param1");
            GeneralDataMgr::getInstance()->openFreeVIP4Event(day);
            break;
        }
        //��������Ѵ���
        case admin_set_race_free_times:
        {
            int times = 0;
            READ_INT_FROM_MOBJ(times,o,"param1");
            RaceMgr::getInstance()->setRaceFreeTimes(times);
            break;
        }
        //�������ۻ
        case admin_open_reborn_discount:
        {
            int discount = 100;
            READ_INT_FROM_MOBJ(discount,o,"param1");
            if (discount >= 1 && discount <= 100 &&g_reborn_discount != discount)
            {
                GeneralDataMgr::getInstance()->setInt("reborn_discount", discount);
                g_reborn_discount = discount;
                //Singleton<shopMgr>::Instance().updateShopPrice(discount);
            }
            break;
        }
        //�޸���������
        case admin_fix_zhen:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                CharData* pc = cdata.get();
                int zid = GeneralDataMgr::getInstance()->getOpenZhen(pc->m_currentStronghold);
                for (int i = 1; i <= zid; ++i)
                {
                    boost::shared_ptr<ZhenData> z = pc->m_zhens.GetZhen(i);
                    if (z.get() == NULL || z->m_level < 5)
                    {
                        pc->m_zhens.setLevel(i, 5);
                    }
                }
            }
            break;
        }
        //�������
        case admin_add_libao:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            int libao_id = 0;
            READ_INT_FROM_MOBJ(libao_id,o,"param2");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                CharData* pc = cdata.get();
                pc->addLibao(libao_id, 1);
            }
            else
            {
                //ȫ����

            }
            break;
        }
        //�������û����
        case admin_guard_event:
        {
            int fac = 110;
            READ_INT_FROM_MOBJ(fac,o,"param1");
            int mins = 120;
            READ_INT_FROM_MOBJ(mins,o,"param2");
            if (fac < 100)
            {
                fac = 100;
            }
            guardMgr::getInstance()->openGuardEvent(fac, mins);
            break;
        }
        case admin_set_guard_times:
        {
            int guard_rob_times = 0, guard_times = 0, guard_help_times = 0;
            READ_INT_FROM_MOBJ(guard_help_times,o,"param1");
            READ_INT_FROM_MOBJ(guard_rob_times,o,"param2");
            READ_INT_FROM_MOBJ(guard_times,o,"param3");
            guardMgr::getInstance()->setGuardTimes(guard_help_times, guard_rob_times, guard_times);
            break;
        }
        case admin_set_guard_fac:
        {
            int fac = 110;
            READ_INT_FROM_MOBJ(fac,o,"param1");
            int mins = 120;
            READ_INT_FROM_MOBJ(mins,o,"param2");
            InsertSaveDb("update custom_shedule set param1=" + LEX_CAST_STR(fac) + ",param2=" + LEX_CAST_STR(mins) + " where type='openGuardEvent'");
        }
        //���ӱ�ʯ
        case admin_add_baoshi:
        {
            int cid = 0;
            int type = 1, level = 1, count = 1;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(type,o,"param2");
            READ_INT_FROM_MOBJ(level,o,"param3");
            READ_INT_FROM_MOBJ(count,o,"param4");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                int n = Singleton<newBaoshiMgr>::Instance().addBaoshi(cdata.get(), type, level, count);
                add_statistics_of_baoshi_get(cid, cdata->m_ip_address, cdata->m_union_id, cdata->m_server_id, type, level, n, baoshi_admin);
            }
            else
            {

            }
            break;
        }
        //ˢ�±�ʯ�̵�
        case admin_refresh_baoshishop:
        {
            Singleton<newBaoshiMgr>::Instance().refresh_shop();
            break;
        }
        //���¾�����������
        case admin_update_corps_limit:
        {
            corpsMgr::getInstance()->updateCorpsMemberLimit();
            break;
        }
        //������������
        case admin_open_feedback:
        {
            std::string data = "";
            READ_STR_FROM_MOBJ(data,o,"extra");
            json_spirit::mValue value;
            json_spirit::read(data, value);
            if (value.type() == json_spirit::obj_type)
            {
                json_spirit::mObject& o = value.get_obj();
                time_t m_start_time = 0, m_day = 0, m_silver_feedback_percent = 0, m_gold_feedback_percent = 0;
                std::string m_event_title = "", m_event_content = "";
                READ_INT_FROM_MOBJ(m_start_time,o,"start_time");
                READ_INT_FROM_MOBJ(m_day,o,"day");
                READ_STR_FROM_MOBJ(m_event_title,o,"title");
                READ_STR_FROM_MOBJ(m_event_content,o,"content");
                READ_INT_FROM_MOBJ(m_silver_feedback_percent,o,"silver");
                READ_INT_FROM_MOBJ(m_gold_feedback_percent,o,"gold");
                Singleton<cost_feedback_event>::Instance().openEvent(m_event_title, m_event_content, m_start_time, m_silver_feedback_percent, m_gold_feedback_percent, m_day);
            }
            break;
        }
        //�ر���������
        case admin_close_feedback:
        {
            Singleton<cost_feedback_event>::Instance().closeEvent();
            break;
        }
        //�������ѻ
        case admin_debug_feedback:
        {
            int day = 1;
            READ_INT_FROM_MOBJ(day,o,"param1");
            Singleton<cost_feedback_event>::Instance().debugSetDay(day);
            break;
        }
#ifdef QQ_PLAT
        //������������޸�
        case admin_fix_share_data:
        {
            Singleton<inviteMgr>::Instance().fixShareData();
            break;
        }
#endif
        //���ż���
        case admin_corps_jisi:
        {
            int fac = 100;
            READ_INT_FROM_MOBJ(fac,o,"param1");
            int mins = 120;
            READ_INT_FROM_MOBJ(mins,o,"param2");
            if (fac < 100)
            {
                fac = 100;
            }
            corpsMgr::getInstance()->openJisiEvent(fac, mins);
            break;
        }
        //��������
        case admin_guard_reset:
        {
            guardMgr::getInstance()->resetAll();
            break;
        }
        //���û��������ų�
        case admin_set_robot_corps_leader:
        {
            int cid = 0, id = 0;
            READ_INT_FROM_MOBJ(id,o,"param1");
            READ_INT_FROM_MOBJ(cid,o,"param2");
            corpsMgr::getInstance()->setRobotCorpsLeader(id, cid);
            break;
        }
        //�����رջ����˾��� 1������0�ر�
        case admin_open_robot_corps:
        {
            int state = 0;
            READ_INT_FROM_MOBJ(state,o,"param1");
            corpsMgr::getInstance()->openRobotCorps(state);
            break;
        }
        //����һ�������˾���
        case admin_create_robot_corps:
        {
            corpsMgr::getInstance()->createRobotCorps();
            break;
        }
        //��������ս����
        case admin_open_corps_fighting_singup:
        {
            Singleton<corpsFightingMgr>::Instance().openSignup();
            break;
        }
        //��ʼ����ս
        case admin_start_corps_fighting:
        {
            Singleton<corpsFightingMgr>::Instance().start();
            break;
        }
        //���þ���boss
        case admin_reset_jt_boss:
        {
            int corps = 0;
            READ_INT_FROM_MOBJ(corps,o,"param1");
            bossMgr::getInstance()->resetJtBoss(corps);
            break;
        }
        //��ȡ֧������
        case admin_trunk_task:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (cdata.get())
            {
                int type = 1;
                READ_INT_FROM_MOBJ(type,o,"param2");
                int param2 = 0;
                READ_INT_FROM_MOBJ(param2,o,"param3");
                if (1 == type)
                {
                    taskMgr::getInstance()->acceptTrunkTask(*(cdata.get()), param2);
                }
                else
                {
                    taskMgr::getInstance()->acceptTrunkTask2(*(cdata.get()), param2);
                }
            }
            break;
        }
        //�ݱ����߷���
        case admin_training_back:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            Singleton<trainingMgr>::Instance().SoulsDaojuBack(cid);
            break;
        }
        //�Ƴ�����״̬
        case admin_remove_chenmi:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (pc)
            {
                pc->m_check_chenmi = false;
                pc->notifyEventRemove(top_level_event_chenmi);
            }
            break;
        }
        //�����齱�
        case admin_open_lottery:
        {
            std::string data = "";
            READ_STR_FROM_MOBJ(data,o,"extra");
            json_spirit::mValue value;
            json_spirit::read(data, value);
            if (value.type() == json_spirit::obj_type)
            {
                json_spirit::mObject& o = value.get_obj();
                time_t start_time = 0, end_time = 0;
                int gold = 0, spic = 0;
                std::string m_event_title = "", m_event_content = "";
                READ_INT_FROM_MOBJ(start_time,o,"start_time");
                READ_INT_FROM_MOBJ(end_time,o,"end_time");
                READ_INT_FROM_MOBJ(gold,o,"gold");
                READ_INT_FROM_MOBJ(spic,o,"spic");
                Singleton<lottery_event>::Instance().openEvent(spic, start_time, end_time, gold);
            }
            break;
        }
        //�رճ齱�
        case admin_close_lottery:
        {
            Singleton<lottery_event>::Instance().closeEvent();
            break;
        }
        //����齱
        case admin_clear_lottery_msg:
        {
            Singleton<lottery_event>::Instance().clearMsg();
            break;
        }
        //���ó齱������Ϣ
        case admin_set_lottery_literal:
        {
            std::string data = "";
            READ_STR_FROM_MOBJ(data,o,"extra");
            json_spirit::mValue value;
            json_spirit::read(data, value);
            if (value.type() == json_spirit::obj_type)
            {
                json_spirit::mObject& o = value.get_obj();
                std::string get = "", announce = "";
                READ_STR_FROM_MOBJ(get,o,"get");
                READ_STR_FROM_MOBJ(announce,o,"announce");
                Singleton<lottery_event>::Instance().setLiteral(get, announce);
            }
            break;
        }

        //���þ�������ϵ��
        case admin_set_corpsFactor:
        //����ó������ϵ��
        case admin_set_tradeFactor:
        //���ð���������ϵ��
        case admin_set_mazeFactor:
        //������Ӫս����ϵ��
        case admin_set_campRaceFactor:
        //���þ���������ϵ��
        case admin_set_arenaFactor:
        //������������ϵ��
        case admin_set_farmFactor:
        //��������ʵ������
        case admin_set_bossFactor:
        {
            int factor = 100;
            READ_INT_FROM_MOBJ(factor,o,"param1");
            setRewardFactor(cmd, factor);
            break;
        }
        //���üҶ�����
        case admin_set_servantFactor:
        {
            int fac = 100;
            READ_INT_FROM_MOBJ(fac,o,"param1");
            servantMgr::getInstance()->setFactor(fac);
            break;
        }
        //����cdn
        case admin_update_cdn:
        {
            std::string cdn = "";
            READ_STR_FROM_MOBJ(cdn,o,"extra");
            cout<<"update cdn to "<<cdn<<endl;
            InsertSaveDb("update admin_setting set set_value='" + GetDb().safestr(cdn) + "' where set_key='flex_cdn'");
            break;
        }
        //�����佫����ȼ�
        case admin_reset_gsoul:
        {
            int cid = 0, gid = 0, level = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            READ_INT_FROM_MOBJ(gid,o,"param2");
            READ_INT_FROM_MOBJ(level,o,"param3");
            resetAllGeneralSoul(cid, gid, level);
            break;
        }
        //���û��ͼӳ�ʱ������ϵ��
        case admin_set_guardFactor:
        {
            int fac = 110;
            READ_INT_FROM_MOBJ(fac,o,"param1");
            InsertSaveDb("update custom_shedule set param1=" + LEX_CAST_STR(fac)+ " where type='openGuardEvent'");
            break;
        }
		//����ģ���ֵ�
		case admin_reprocess_recharge_event:
		{
			int cid = 0, gold = 0;
			READ_INT_FROM_MOBJ(cid,o,"param1");
			READ_INT_FROM_MOBJ(gold,o,"param2")
			//��ֵ�
			recharge_event_mgr::getInstance()->updateRechargeEvent(cid, gold, 0);
			break;
		}
		case admin_change_mall_discount:
		{
			time_t start_time = 0, end_time = 0;
	        int discount = 100;
			READ_INT_FROM_MOBJ(discount,o,"param1");
			READ_INT_FROM_MOBJ(start_time,o,"param2");
			READ_INT_FROM_MOBJ(end_time,o,"param3");
			GeneralDataMgr::getInstance()->openMallDiscountEvent(discount,start_time, end_time);
			break;
		}


#ifdef JP_SERVER
        //�շ�������ѽ��� ����1����ɫID   ����2��0���뽱����1�����뽱��
        case admin_jp_set_invite:
        {
            int cid = 0;
            READ_INT_FROM_MOBJ(cid,o,"param1");
            if (cid > 0)
            {
                boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
                if (cdata.get())
                {
                    CharData* pc = cdata.get();
                    int type = 0;
                    READ_INT_FROM_MOBJ(type,o,"param2");
                    int param = 1;
                    READ_INT_FROM_MOBJ(param,o,"param3");
                    if (type == 0)
                    {
                        //���뽱��
                        if (param == 1)
                        {
                            pc->addSilver(100000);
                            pc->NotifyCharData();
                            sendSystemMail(pc->m_name, pc->m_id, strJpInviteMail1Title, strJpInviteMail1Content);
                        }
                        else
                        {
                            pc->addTreasure(treasure_type_yushi, 18000);
                            sendSystemMail(pc->m_name, pc->m_id, strJpInviteMail2Title, strJpInviteMail2Content);
                        }
                    }
                    else
                    {
                        //�����뽱��
                        if (param == 1)
                        {
                            pc->addSilver(20000);
                            sendSystemMail(pc->m_name, pc->m_id, strJpInvitedMail1Title, strJpInvitedMail1Content);
                        }
                        else
                        {
                            pc->addSilver(100000);
                            pc->addLing(12);
                            sendSystemMail(pc->m_name, pc->m_id, strJpInvitedMail2Title, strJpInvitedMail2Content);
                        }
                        pc->NotifyCharData();
                    }
                }
            }
            break;
        }
#endif
        default:
        {
#if 0
            if (event == "debugTimer")
            {
                int times = 1000;
                READ_INT_FROM_MOBJ(times,o,"param1");
                for (int i = 0; i < times; ++i)
                {
                    json_spirit::mObject mobj;
                    mobj["cmd"] = "xxxx";
                    boost::shared_ptr<splsTimer> tmsg;
                    tmsg.reset(new splsTimer(10 + my_random(1,5), 1, mobj, 1));
                    boost::uuids::uuid _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
                    if (my_random(1,3) == 1)
                    {
                        splsTimerMgr::getInstance()->delTimer(_uuid);
                    }
                }
                return HC_SUCCESS;
            }
            if (event == "initRobot")
            {
                for (int i = 1; i <= 5000; ++i)
                {
                    std::string account = "robot" + LEX_CAST_STR(i);
                    Query q(GetDb());
                    q.get_result("SELECT `id`,lastlogin,spic,state,delete_time,level,name FROM `charactors` WHERE account='" + GetDb().safestr(account) + "' order by id limit 1");
                    if (q.fetch_row())
                    {
                        int cid = q.getval();
                        q.free_result();
                        if (cid == 0)
                        {
                            continue;
                        }
                        int level = my_random(20,66);
                        GeneralDataMgr::getInstance()->updateTempo(cid, level);
                        CharData* cdata = GeneralDataMgr::getInstance()->GetCharData(cid).get();
                        if (cdata)
                        {
                            //����1����5��
                            while (cdata->m_zhens.Levelup(1, 5) != -1)
                            {
                                ;
                            }
                            //Ӣ�۵ȼ�
                            std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = cdata->m_generals.m_generals.begin();
                            while (it != cdata->m_generals.m_generals.end())
                            {
                                if (it->second.get())
                                {
                                    while (it->second->m_level < level)
                                    {
                                        it->second->Levelup(level);
                                    }
                                }
                                ++it;
                            }
                            //Ĭ������
                            cdata->m_zhens.SetDefault(1);
                            //Ӣ������
                            it = cdata->m_generals.m_generals.begin();
                            while (it != cdata->m_generals.m_generals.end())
                            {
                                if (HC_SUCCESS != cdata->m_zhens.Up(1, it->second->m_id))
                                {
                                    break;
                                }
                                ++it;
                            }
                        }
                    }
                    else
                    {
                        q.free_result();
                    }
                }
                return HC_SUCCESS;
            }
#endif
            cout<<"!!!!!!!!!!!!!!!!!!!!! unknow shedule event "<<event<<","<<time(NULL)<<" !!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
            break;
        }
    }
    if (g_print_debug_info > 1)
    {
        uint64_t cost_us = splsTimeStamp()-start_time;
        if (cost_us > 5000)
        {
            cout<<"****** ProcessScheduleEvent : "<<event<<" cost "<<(double(cost_us)/1000)<<" ms ******"<<endl;
        }
    }
    return HC_SUCCESS;
}

//��ѯ�ʺŽ�ɫ�б���
int ProcessQueryAccountCallback(json_spirit::mObject& o)
{
    std::string account = "";
    uint64_t pvoid = 0;
    READ_UINT64_FROM_MOBJ(pvoid,o,"clist");
    CharactorInfo** pCharList = reinterpret_cast<CharactorInfo**>(pvoid);
    READ_STR_FROM_MOBJ(account, o, "account");

    boost::shared_ptr<OnlineUser> paccount = GeneralDataMgr::getInstance()->GetAccount(account);
    if (paccount.get() && paccount->m_state == 0 && paccount->m_sockethandle.get())
    {
        paccount->m_charactorlist.clear();
        int counts = 0;
        READ_INT_FROM_MOBJ(counts, o, "size");

        if (pCharList)
        {
            for (int i = 0; i < counts; ++i)
            {
                if (pCharList[i])
                {
                    paccount->m_charactorlist.push_back(*(pCharList[i]));
                    delete pCharList[i];
                }
            }
        }
    }
    else
    {
        if (pCharList)
        {
            int counts = 0;
            READ_INT_FROM_MOBJ(counts, o, "size");
            for (int i = 0; i < counts; ++i)
            {
                if (pCharList[i])
                {
                    delete pCharList[i];
                }
            }
        }
        return HC_ERROR;
    }
    json_spirit::Array char_array;
    std::list<CharactorInfo>::iterator it = paccount->m_charactorlist.begin();
    while (it != paccount->m_charactorlist.end())
    {
        if (it->m_state == 0 || it->m_deleteTime > time(NULL))
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->m_cid));
            obj.push_back( Pair("name", it->m_name));
            obj.push_back( Pair("level", it->m_level));
            obj.push_back( Pair("spic", it->m_spic));
            obj.push_back( Pair("lastlogin", it->m_lastlogin));
            obj.push_back( Pair("state", it->m_state));
            if (it->m_state == 1)
            {
                obj.push_back( Pair("leftTime", it->m_deleteTime - time(NULL)));
            }
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(it->m_cid).get();
            if (pc && pc->m_current_guide > 0)
            {
                json_spirit::Object g;
                g.push_back( Pair("id", pc->m_current_guide) );
                g.push_back( Pair("state", pc->getGuideState1(pc->m_current_guide)) );
                obj.push_back( Pair("guide", g) );
            }
            char_array.push_back(obj);
        }
        ++it;
    }
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "charlist") );
    robj.push_back( Pair("charlist", char_array));
    robj.push_back( Pair("s", 200) );
    paccount->m_state = 1;
    paccount->m_sockethandle->send(write(robj, json_spirit::raw_utf8));
    INFO("load account success,"<<splsTimeStamp()<<endl);
    return HC_SUCCESS;
}

//�����¼��Ϣ
int ProcessLogin(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    std::string username  = "";
    READ_STR_FROM_MOBJ(username,o,"user");
    std::string qid = "";
    READ_STR_FROM_MOBJ(qid,o,"qid");
    int isAdult = 0;
    READ_INT_FROM_MOBJ(isAdult,o,"isAdult");
    int union_id = 0;
    READ_INT_FROM_MOBJ(union_id,o,"union_id");
    std::string server_id = "";
    READ_STR_FROM_MOBJ(server_id,o,"server_id");

    int qq_yellow_level = 0;
    int is_qq_year_yellow = 0;
#ifdef QQ_PLAT
    std::string iopenid = "";
    std::string feedid = "0";
    std::string str1 = "";
    std::string str2 = "";
    READ_INT_FROM_MOBJ(qq_yellow_level,o,"qqYellowLevel");
    if (qq_yellow_level > 0)
    {
        READ_INT_FROM_MOBJ(is_qq_year_yellow,o,"isYearYellow");
    }
    READ_STR_FROM_MOBJ(iopenid,o,"iopenid");
    READ_STR_FROM_MOBJ(feedid,o,"feedid");
    READ_STR_FROM_MOBJ(str1,o,"login_str1");
    READ_STR_FROM_MOBJ(str2,o,"login_str2");
#else
    std::string vcode = "";
    std::string sid = "0";
    READ_STR_FROM_MOBJ(vcode,o,"vcode");
    READ_STR_FROM_MOBJ(sid,o,"sid");
#endif
    INFO(username<<" login...");
#ifdef QQ_PLAT
    return GeneralDataMgr::getInstance()->Login(qid, username, isAdult, union_id, server_id, qq_yellow_level, is_qq_year_yellow, iopenid, feedid, str1, str2, psession, robj);
#else
    return GeneralDataMgr::getInstance()->Login(qid, username, isAdult, union_id, server_id, qq_yellow_level, is_qq_year_yellow, vcode, sid, psession, robj);
#endif
}

//ս���ط�
int ProcessGetBattltRecord(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    std::string battle_msg = Singleton<combatRecordMgr>::Instance().getCombatRecord(id);
    if (battle_msg != "")
    {
        psession->send(battle_msg);
        return HC_SUCCESS_NO_RET;
    }
    else
    {
        return HC_ERROR;
    }
}

//���Գ�ֵ
int ProcessTestRecharge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    int recharge_gold = 0;
    //�����Ʋ��Գ�ֵ
    if (gTestRechargeType == 2)
    {
        recharge_gold = 300;
        if (pc->m_vip >= 10)
        {
            recharge_gold = 90000;
        }
        else if (pc->m_vip >= 0)
        {
            recharge_gold = iTEST_recharge[pc->m_vip];
        }
        pc->setExtraData(char_data_type_daily, char_data_test_recharge, 1);
    }
    else if (gTestRechargeType == 1)
    {
        //������Գ�ֵ
        //int test_recharge_time = pc->queryExtraData(char_data_type_daily, char_data_test_recharge);
        //if (0 < test_recharge_time)
        //{
        //    return HC_ERROR_IN_COOLTIME;
        //}
        //pc->setExtraData(char_data_type_daily, char_data_test_recharge, 1);
        recharge_gold = 1000;
    }
    else            //��ֹ���Գ�ֵ
    {
        return HC_ERROR;
    }

    //�ɹ�����
    InsertSaveDb("insert into char_recharge set type='test',cid=" + LEX_CAST_STR(pc->m_id)
            + ",account='',gold='" + LEX_CAST_STR(recharge_gold) + "',input=now()");
    pc->m_test_recharge_time = time(NULL) + 3600 * 6;
    ++pc->m_total_test_recharge;

    pc->addGold(recharge_gold);
    //��һ��ͳ��
    add_statistics_of_gold_get(pc->m_id,pc->m_ip_address,recharge_gold,gold_get_recharge, pc->m_union_id, pc->m_server_id);
    //if (pc->m_total_recharge == 0)
    //{
        //if (pc->queryExtraData(char_data_type_normal, char_data_first_recharge_gift) == 0)
        //{
            //�����׳����
            //pc->setExtraData(char_data_type_normal, char_data_first_recharge_gift, 1);
            //����������ť
            //pc->notifyGiftState();
        //}
    //}
    pc->m_total_recharge += recharge_gold;
    InsertSaveDb("replace into char_total_recharge (cid,total_recharge) values (" + LEX_CAST_STR(pc->m_id) + "," + LEX_CAST_STR(pc->m_total_recharge) + ")");
    pc->updateVip();
    //��ֵ�
    recharge_event_mgr::getInstance()->updateRechargeEvent(pc->m_id, recharge_gold, time(NULL));
    //pc->updateRechargeReward(recharge_gold);

    //�����л
    int score = recharge_gold;
    if (score > 0)
        newRankings::getInstance()->updateEventRankings(pc->m_id,rankings_event_recharge,score);

    pc->NotifyCharData();
    pc->notifyOpeningState();
    return HC_SUCCESS;
}

//��ѯ��ֵ
int ProcessQueryRecharge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    //�����Ʋ��Գ�ֵ
    if (gTestRechargeType == 2)
    {
        robj.push_back( Pair("left_time", 0) );
    }
    //ÿ�ղ��Գ�ֵ
    else if (1 == gTestRechargeType)
    {
        int test_recharge_time = pc->queryExtraData(char_data_type_daily, char_data_test_recharge);
        robj.push_back( Pair("left_time", test_recharge_time) );
    }
    else
    {
        robj.push_back( Pair("left_time", -1) );
    }
    robj.push_back( Pair("vip", pc->m_vip) );
    robj.push_back( Pair("recharge", pc->m_total_recharge + pc->m_vip_exp) );
    if (pc->m_vip >= 0 && pc->m_vip < 12)
    {
        robj.push_back( Pair("next", iVIP_recharge[pc->m_vip]) );
    }
    robj.push_back( Pair("first_recharge_state", pc->queryExtraData(char_data_type_normal, char_data_first_recharge_gift) != 2) );
    robj.push_back( Pair("daily_recharge_need", Singleton<new_event_mgr>::Instance().getDailyRechargeNeed(*pc)) );
    return HC_SUCCESS;
}

//��ѯ�˻�����
int ProcessQueryAccountPoints(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    dbCmd _dbcmd;
    _dbcmd._account = pc->m_account;
    _dbcmd._cmd = db_cmd_query_account_score;
    InsertDbCharCmd(_dbcmd);
    //���������Ϣ
    return HC_SUCCESS_NO_RET;
}

//��ѯ�˻������ص�
int ProcessQueryAccountPointsCallBack(json_spirit::mObject& o)
{
    std::string account = "";
    int score = 0;
    READ_STR_FROM_MOBJ(account, o, "account");
    READ_INT_FROM_MOBJ(score, o, "score");
    boost::shared_ptr<OnlineUser> paccount = GeneralDataMgr::getInstance()->GetAccount(account);
    if (paccount.get() && paccount->m_onlineCharactor.get() && paccount->m_onlineCharactor->m_charactor.get() && paccount->m_sockethandle.get())
    {
        paccount->m_onlineCharactor->m_charactor->m_temp_score = score;
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "getAccountRecharge") );
        obj.push_back( Pair("s", 200) );
        obj.push_back( Pair("points", score) );
        paccount->m_sockethandle->send(write(obj));
    }
    return HC_SUCCESS;
}

//�һ��˻�����
int ProcessExchangeAccountScore(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int nums = 0;
    READ_INT_FROM_MOBJ(nums,o,"points");

    if (pc->m_temp_score < nums)
    {
        return HC_ERROR;
    }
    Query q(GetDb());
    q.get_result("select money from accounts where account='" + GetDb().safestr(pc->m_account) + "'");
    if (!q.fetch_row())
    {
        q.free_result();
        return HC_ERROR;
    }
    int money = q.getval();
    q.free_result();
    if (nums > money || nums <= 0)
    {
        return HC_ERROR;
    }
    money -= nums;
    if (!q.execute("update accounts set money=" + LEX_CAST_STR(money) + " where account='" + GetDb().safestr(pc->m_account) + "'"))
    {
        CHECK_DB_ERR(q);
    }
    else
    {
        pc->addGold(nums);
        //��һ��ͳ��
        add_statistics_of_gold_get(pc->m_id,pc->m_ip_address,nums,gold_get_recharge, pc->m_union_id, pc->m_server_id);
        //�ɹ�����
        time_t recharge_time = time(NULL);
        InsertSaveDb("insert into char_recharge set type='convert',cid=" + LEX_CAST_STR(pc->m_id)
                + ",account='',gold='" + LEX_CAST_STR(nums) + "',input=FROM_UNIXTIME(" + LEX_CAST_STR(recharge_time) + ")");
        if (pc->queryExtraData(char_data_type_normal, char_data_first_recharge_gift) == 0)
        {
            //�����׳����
            pc->setExtraData(char_data_type_normal, char_data_first_recharge_gift, 1);
            //����������ť
            pc->notifyEventState(top_level_event_first_recharge, 1, 0);
        }

        pc->m_total_recharge += nums;
        InsertSaveDb("replace into char_total_recharge (cid,total_recharge) values (" + LEX_CAST_STR(pc->m_id) + "," + LEX_CAST_STR(pc->m_total_recharge) + ")");
        pc->updateVip();
        //��ֵ�
        recharge_event_mgr::getInstance()->updateRechargeEvent(pc->m_id, nums, recharge_time);
        //pc->updateRechargeReward(nums);

        //�����л
        int score = nums;
        if (score > 0)
            newRankings::getInstance()->updateEventRankings(pc->m_id,rankings_event_recharge,score);

        pc->NotifyCharData();
        pc->notifyOpeningState();
    }
    return HC_SUCCESS;
}

//��ѯ�����Ƿ��Ѿ����
int ProcessQueryGuideState(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("id", id) );
    if (pc->getGuideState(id) > 0)
    {
        robj.push_back( Pair("isComplete", 1) );
    }
    else
    {
        robj.push_back( Pair("isComplete", 2) );
    }
    return HC_SUCCESS;
}

//��ѯ��ǰ����
int ProcessQueryCurGuide(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int state = 0, id = pc->getGuideState1(state);
    robj.push_back( Pair("id", id) );
    if (state)
    {
        robj.push_back( Pair("state", state) );
    }
    return HC_SUCCESS;
}

//�����������
int ProcessSetGuideState(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("id", id) );

    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    //type��0��ʾǿ�������������״̬
    if (type == 0)
    {
        if (0 == pc->getGuideState2(id))
        {
            return HC_ERROR;
        }
    }
    int next_guide = 0;
    READ_INT_FROM_MOBJ(next_guide,o,"next");
    robj.push_back( Pair("next", next_guide) );
    pc->setGuideStateComplete(id, next_guide);
    return HC_SUCCESS;
}

//���ʻ������Ϣ
int ProcessGetOpeningAction(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array list;
    json_spirit::Object info;

    #if 0
    int first_view = pc->queryExtraData(char_data_type_daily, char_data_daily_view_new_event);
    if (0 == first_view)
    {
        //�����Ѿ�����
        pc->setExtraData(char_data_type_daily, char_data_daily_view_new_event, 1);
    }
    #endif
    //cout<<"ProcessGetOpeningAction,cid:"<<pc->m_id<<",";

    //���ʳ�ֵ����
    if (recharge_event_mgr::getInstance()->isOpen(1))
    {
        info.push_back( Pair("id", 1) );
        info.push_back( Pair("state", recharge_event_mgr::getInstance()->getCanget(pc, 1)) );
        //info.push_back( Pair("memo", "") );
        list.push_back(info);
        //cout<<"(id1,single recharge,state"<<recharge_event_mgr::getInstance()->getCanget(pc, 1)<<")";
    }
    //�ۼƳ�ֵ����
    if (recharge_event_mgr::getInstance()->isOpen(2))
    {
        info.clear();
        info.push_back( Pair("id", 2) );
        info.push_back( Pair("state", recharge_event_mgr::getInstance()->getCanget(pc, 2)) );
        //info.push_back( Pair("memo", "") );
        list.push_back(info);
        //cout<<"(id2,total recharge,state"<<recharge_event_mgr::getInstance()->getCanget(pc, 1)<<")";
    }

    //�ȼ���� id=3
    if (pc->m_newbie_reward_canGet != 2)
    {
        info.clear();
        info.push_back( Pair("id", 3) );
        info.push_back( Pair("state", pc->m_newbie_reward_canGet) );
        //info.push_back( Pair("memo", strNewbieEventMemo) );
        list.push_back(info);
        //cout<<"(id4,newbie action,state"<<pc->m_newbie_reward_canGet<<")";
    }

    //�����
    Singleton<new_event_mgr>::Instance().getActionList(pc, list);
    //cout<<endl;
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

#if 0
//����������Ϣ
int ProcessGetGiftAction(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array list;
    json_spirit::Object info;

    //cout<<"ProcessGetGiftAction,cid:"<<pc->m_id<<",";
    //������¼����
    time_t continue_login_gift_end = pc->queryExtraData(char_data_type_normal, char_data_get_continue_login_day);
    if (pc->m_currentStronghold >= iContinueLoginStronghold && time(NULL) < continue_login_gift_end)
    {
        bool notify = false;
        std::map<int,CharLoginPresent>::iterator it = pc->m_login_present.begin();
        while (it != pc->m_login_present.end())
        {
            CharLoginPresent clp = it->second;
            if (clp.present && clp.state == 1)
            {
                notify = true;
            }
            ++it;
        }
        info.push_back( Pair("id", 1) );
        info.push_back( Pair("state", notify ? 1 : 0) );
        //info.push_back( Pair("memo", "") );
        list.push_back(info);

        //cout<<"(id1,login present,state"<<(notify?1:0)<<")";
    }
    int firstr = pc->queryExtraData(char_data_type_normal, char_data_first_recharge_gift);
    //�׳����
    if (pc->m_currentStronghold >= iFirstRechargeStronghold && firstr != 2)
    {
        info.push_back( Pair("id", 2) );
        info.push_back( Pair("state", (firstr == 1) ? 1 : 0) );
        //info.push_back( Pair("memo", "") );
        list.push_back(info);
        //cout<<"(id2,first recharge present,state"<<(firstr==1?1:0)<<")";
    }
    //VIP����
    {
        int state = 0;
        std::map<int,CharVIPPresent>::iterator it = pc->m_vip_present.begin();
        while (it != pc->m_vip_present.end())
        {
            CharVIPPresent cvp = it->second;
            if (cvp.present && cvp.state == 1)
            {
                state = 1;
                break;
            }
            ++it;
        }
        info.clear();
        info.push_back( Pair("id", 3) );
        info.push_back( Pair("state", state) );
        //info.push_back( Pair("memo",  "") );
        list.push_back(info);
        //cout<<"(id3,vip present,state"<<state<<")";
    }
    //���ֳ��ŵȼ����
    if (pc->m_newbie_reward_canGet != 2)
    {
        info.clear();
        info.push_back( Pair("id", 4) );
        info.push_back( Pair("state", pc->m_newbie_reward_canGet) );
        //info.push_back( Pair("memo", strNewbieEventMemo) );
        list.push_back(info);
        //cout<<"(id4,newbie action,state"<<pc->m_newbie_reward_canGet<<")";
    }
    robj.push_back( Pair("list", list) );
    //cout<<endl;
    return HC_SUCCESS;
}
#endif

//�ճ��������Ϣ
int ProcessGetDailyAction(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array elist;

    //cout<<"ProcessGetDailyAction:"<<pc->m_id<<",";
    //���˸���
    if (pc->isMapPassed(1))
    {
        json_spirit::Object o;
        o.push_back( Pair("type", 1) );
        o.push_back( Pair("startTime", "0:01") );
        o.push_back( Pair("endTime", "23:59") );
        o.push_back( Pair("state", 1) );
        int total_time = 0;
        int can_time = groupCopyMgr::getInstance()->getAllCopyCanAttackTimes(pc->m_id, total_time);
        if (can_time <= 0)
        {
            o.push_back( Pair("done", 1) );
        }
        elist.push_back(o);
        //cout<<"(group copy)";
    }
    //ս���޻
    bossMgr::getInstance()->getBossList(pc, elist);

    if (pc->m_campraceOpen)
    {
        //��Ӫս
        campRaceMgr::getInstance()->getAction(pc, elist);
        //��һ��
        int first_view = pc->queryExtraData(char_data_type_normal, char_data_view_camprace);
        if (0 == first_view)
        {
            pc->setExtraData(char_data_type_normal, char_data_view_camprace, 1);
            int state = pc->getDailyState();
            if (0 == state)
            {
                pc->notifyEventState(top_level_event_daily, 0, 0);
            }
        }
    }

    robj.push_back( Pair("list", elist) );
    //cout<<endl;
    return ret;
}

//ϴ��������Ϣ
int ProcessWashAction(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (!pc->isWashEventOpen())
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("totalTimes", pc->m_daily_wash_times) );
    json_spirit::Array elist;
    for (int i = 0; i < iWashEventNum; ++i)
    {
        json_spirit::Object o;
        o.push_back( Pair("times", iWashEvent[i][0]) );
        o.push_back( Pair("reward", iWashEvent[i][1]) );
        if (pc->m_daily_wash_times >= iWashEvent[i][0] && pc->m_wash_event[i] == 0)
        {
            o.push_back( Pair("canGet", 1) );
        }
        elist.push_back(o);
    }
    robj.push_back( Pair("list", elist) );
    return HC_SUCCESS;
}

//�׳������Ϣ
int ProcessQueryFirstRechargeGift(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    //ÿ���һ��
    int first_view = pc->queryExtraData(char_data_type_daily, char_data_daily_view_first_recharge);
    if (0 == first_view)
    {
        pc->setExtraData(char_data_type_daily, char_data_daily_view_first_recharge, 1);
    }

    if (pc->queryExtraData(char_data_type_normal, char_data_first_recharge_gift) == 1)
    {
        robj.push_back( Pair("canGet", 1) );
    }
    else
    {
        robj.push_back( Pair("canGet", 0) );
        if (first_view == 0)
        {
            //֪ͨ������
            pc->notifyEventState(top_level_event_first_recharge, 0, 0);
        }
    }
    json_spirit::Array list;
    std::list<Item>& p_list = GeneralDataMgr::getInstance()->GetFirstRechargeGift();
    std::list<Item>::iterator it_l = p_list.begin();
    while (it_l != p_list.end())
    {
        boost::shared_ptr<json_spirit::Object> p_obj;
        ItemToObj(&(*it_l), p_obj);
        if (p_obj.get())
        {
            list.push_back(*(p_obj.get()));
        }
        ++it_l;
    }
    robj.push_back( Pair("gift_list", list) );
    return HC_SUCCESS;
}

//��ȡ�׳����
int ProcessGetFirstRechargeGift(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    //�佫�����ж�
    if (pc->m_generals.m_generals.size() >= pc->m_general_limit)
    {
        return HC_ERROR_TOO_MUCH_GENERALS;
    }
    if (pc->queryExtraData(char_data_type_normal, char_data_first_recharge_gift) == 1 && pc->m_level >= 11)
    {
        pc->setExtraData(char_data_type_normal, char_data_first_recharge_gift, 2);
        //������
        std::list<Item> p_list = GeneralDataMgr::getInstance()->GetFirstRechargeGift();
        giveLoots(pc, p_list, 0, pc->m_level, 0, NULL, &robj, true, give_libao_loot);
        pc->NotifyCharData();

        std::string msg = strFirstRechargeMsg;
        str_replace(msg, "$N", MakeCharNameLink(pc->m_name));
        //��һ�õ��佫
        for (std::list<Item>::iterator it = p_list.begin(); it != p_list.end(); ++it)
        {
            if (it->type == item_type_general)
            {
                int gid = pc->m_generals.GetGeneralByType(it->id);
                boost::shared_ptr<CharGeneralData> gdata = pc->m_generals.GetGenral(gid);
                if (gdata.get())
                {
                    str_replace(msg, "$n", gdata->m_color_link);
                }
                break;
            }
        }
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);

        //���»��ť
        pc->notifyEventRemove(top_level_event_first_recharge);
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

//���ֳ���
int ProcessGetNoviceAction(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    //int leftTime = pc->m_createTime + iNewbieGoGoGoSecs - time(NULL);
    //if (leftTime <= 0)
    //{
    //    return HC_SUCCESS;
    //}

#if 0
    json_spirit::Object info;
    info.push_back( Pair("times", pc->m_free_rest) );
    int restTime = pc->m_free_rest_time - time(NULL);
    info.push_back( Pair("restTime", restTime > 0 ? restTime : 0) );
    info.push_back( Pair("leftTime", leftTime) );
    robj.push_back( Pair("info", info) );
#endif
    json_spirit::Array rlist;
    libao_mgr::getInstance()->getLevelLibaoList(*pc, rlist);
    robj.push_back( Pair("list", rlist) );
    return HC_SUCCESS;
}

#if 0
//���߸���
int ProcessGetWelfareInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    Object info;
    time_t t_now = time(NULL);
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&t_now, t);
    bool next_day = false;
    std::string curTime = "", nextTime = "";
    if (t->tm_hour >= 24 || t->tm_hour < 0)
    {
        return -1;
    }
    else if (t->tm_hour >= 21 && t->tm_hour < 24)
    {
        t->tm_hour = 12;
        t->tm_min = 0;
        t->tm_sec = 0;
        next_day = true;
        curTime = "21:00";
        nextTime = "12:00";
        info.push_back( Pair("isGet", pc->m_welfare > 0) );
    }
    else if (t->tm_hour < 12)
    {
        t->tm_hour = 12;
        t->tm_min = 0;
        t->tm_sec = 0;
        curTime = "21:00";
        nextTime = "12:00";
        info.push_back( Pair("isGet", pc->m_welfare > 0) );
    }
    else if (t->tm_hour < 15)
    {
        t->tm_hour = 15;
        t->tm_min = 0;
        t->tm_sec = 0;
        curTime = "12:00";
        nextTime = "15:00";
        info.push_back( Pair("isGet", pc->m_welfare > 0) );
    }
    else if (t->tm_hour < 18)
    {
        t->tm_hour = 18;
        t->tm_min = 0;
        t->tm_sec = 0;
        curTime = "15:00";
        nextTime = "18:00";
        info.push_back( Pair("isGet", pc->m_welfare > 0) );
    }
    else
    {
        t->tm_hour = 21;
        t->tm_min = 0;
        t->tm_sec = 0;
        curTime = "18:00";
        nextTime = "21:00";
        info.push_back( Pair("isGet", pc->m_welfare > 0) );
    }
    time_t t_refresh = mktime(t) + (next_day ? 86400 : 0);
    info.push_back( Pair("leftTime", t_refresh - t_now) );
    info.push_back( Pair("curTime", curTime) );
    info.push_back( Pair("nextTime", nextTime) );
    Object getobj;
    getobj.push_back( Pair("ling", 6) );
    info.push_back( Pair("get", getobj) );
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

//������¼����
int ProcessGetLoginPresent(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    bool notify = false;
    json_spirit::Array list;
    std::map<int,CharLoginPresent>::iterator it = pc->m_login_present.begin();
    while (it != pc->m_login_present.end())
    {
        CharLoginPresent& clp = it->second;
        if (clp.present)
        {
            json_spirit::Object o;
            o.push_back( Pair("id", clp.present->id) );
            o.push_back( Pair("spic", clp.present->spic) );
            o.push_back( Pair("quality", clp.present->quality) );
            o.push_back( Pair("name", clp.present->name) );
            o.push_back( Pair("memo", clp.present->memo) );
            o.push_back( Pair("state", clp.state) );
            list.push_back(o);
            if (clp.state == 1)
                notify = true;
        }
        ++it;
    }
    if (notify)
    {
        robj.push_back( Pair("canGet", 1) );
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("days", pc->m_continue_days) );
    return HC_SUCCESS;
}
#endif

//VIP����
int ProcessGetVIPPresent(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array list;
    std::map<int,CharVIPPresent>::iterator it = pc->m_vip_present.begin();
    while (it != pc->m_vip_present.end())
    {
        CharVIPPresent cvp = it->second;
        if (cvp.present)
        {
            json_spirit::Object o;
            o.push_back( Pair("id", cvp.present->vip) );
            o.push_back( Pair("vip", cvp.present->vip) );
            o.push_back( Pair("list", cvp.present->getArray()) );
            o.push_back( Pair("state", cvp.state) );
            list.push_back(o);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

/*��ֵ������
int ProcessGetChargePresent(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array list;
    int type = 0;
    if (pc->m_createTime + iRechargeFirst - time(NULL) > 0)
    {
        type = 1;
        robj.push_back( Pair("coolTime", pc->m_createTime + iRechargeFirst - time(NULL)) );
    }
    else if (pc->m_createTime + iRechargeFirst + iRechargeSecond - time(NULL) > 0)
    {
        type = 2;
        robj.push_back( Pair("coolTime", pc->m_createTime + iRechargeFirst + iRechargeSecond - time(NULL)) );
    }
    else
        return HC_ERROR;
    if (type != 0)
    {
        for (int i = (type - 1) * 7 + 1; i <= (type - 1) * 7 + 7; ++i)
        {
            baseRechargePresent* p = GeneralDataMgr::getInstance()->getBaseRechargePresent(i);
            if (p != NULL)
            {
                json_spirit::Object o;
                o.push_back( Pair("id", p->id) );
                o.push_back( Pair("charge", p->needgold) );
                o.push_back( Pair("times", pc->m_recharge_reward[i]) );
                o.push_back( Pair("state", pc->m_recharge_reward[i] > 0 ? 1 : 0) );
                o.push_back( Pair("memo", p->memo) );
                list.push_back(o);
            }
        }
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}*/

//������Ϣ
int ProcessGetTroopsInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    if (id != 0)//����ĳ��������Ϣ
    {
        return corpsMgr::getInstance()->getCorpsDetail(pc->m_id, id, robj);
    }
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->getCorpsDetail(pc->m_id, pc->m_corps_member->corps, robj);
    }
    else
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
}

//���ų�Ա��Ϣ
int ProcessGetTroopsMemberList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->getCorpsMembers(pc->m_id, pc->m_corps_member->corps, robj);
    }
    else
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
}

//������־
int ProcessGetTroopsEventList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->getCorpsEvents(pc->m_id, pc->m_corps_member->corps, robj);
    }
    else
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
}

//���������б�
int ProcessGetTroopsApplyList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0, pageNums = 0;
    READ_INT_FROM_MOBJ(page,o,"page");
    READ_INT_FROM_MOBJ(pageNums,o,"pageNums");
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->getApplications(pc->m_id, pc->m_corps_member->corps, page, pageNums, robj);
    }
    else
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
}

//�����������
int ProcessDealTroopsApply(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        ERR();
        return ret;
    }
    int purpose = 0, id = 0;

    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    READ_INT_FROM_MOBJ(id,o,"id");
    if (pc->m_corps_member.get())
    {
        switch (purpose)
        {
            case 1:
                return corpsMgr::getInstance()->acceptApplication(pc->m_id, pc->m_corps_member->corps, id, robj);
                break;
            case 2:
                return corpsMgr::getInstance()->rejectApplication(pc->m_id, pc->m_corps_member->corps, id, robj);
                break;
            case 3:
                return corpsMgr::getInstance()->rejectAllApplication(pc->m_id, pc->m_corps_member->corps, robj);
                break;
        }
    }
    return HC_ERROR_NOT_JOIN_JT;
}

/**��ȡ���ų�ת�ö����б�**/
int ProcessGetTrMemberList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->getAssistants(pc->m_id, pc->m_corps_member->corps, robj);
    }
    return HC_ERROR_NOT_JOIN_JT;
}

/**ת�þ��ų�**/
int ProcessDealTroopsLeader(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->appointment(pc->m_id, pc->m_corps_member->corps, id, 1, robj);
    }
    return HC_ERROR_NOT_JOIN_JT;
}
/**����ı���ų�**/
int ProcessChangeTroopsLeader(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->changeLeader(pc->m_id, pc->m_corps_member->corps, robj);
    }
    return HC_ERROR_NOT_JOIN_JT;
}

/**������������**/
int ProcessRecruitTroopsMember(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->recruitMember(pc->m_id, pc->m_corps_member->corps, robj);
    }
    return HC_ERROR_NOT_JOIN_JT;
}

/**��ɢ����**/
int ProcessDimissTroops(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->dissolve(pc->m_id, pc->m_corps_member->corps, robj);
    }
    return HC_ERROR_NOT_JOIN_JT;
}

/**���þ�����Ϣ**/
int ProcessSetTroopsInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get())
    {
        std::string qq = "", memo = "", intro = "";
        READ_STR_FROM_MOBJ(memo,o,"memo");
        READ_STR_FROM_MOBJ(intro,o,"intro");
        READ_STR_FROM_MOBJ(qq,o,"qq");
        return corpsMgr::getInstance()->setCorpsInfo(pc->m_id, pc->m_corps_member->corps, memo, intro, qq, robj);
    }
    return HC_ERROR_NOT_JOIN_JT;
}

/**�˳�����**/
int ProcessQuitTroops(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->quitCorps(pc->m_id, pc->m_corps_member->corps, robj);
    }
    return HC_ERROR_NOT_JOIN_JT;
}

/**�������а�**/
int ProcessGetTroopsRankList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0, pageNums = 0;
    READ_INT_FROM_MOBJ(page,o,"page");
    READ_INT_FROM_MOBJ(pageNums,o,"pageNums");
    int apply = 0;
    READ_INT_FROM_MOBJ(apply,o,"apply");
    if (apply == 0)
    {
        return corpsMgr::getInstance()->getCorpsList(pc->m_id, page, pageNums, robj);
    }
    else
    {
        return corpsMgr::getInstance()->getApplicationCorps(pc->m_id, robj);
    }
}

/**��������**/
int ProcessCreateTroops(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    std::string name = "";
    READ_STR_FROM_MOBJ(name,o,"name");
    if (!pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->createCorps(pc->m_id, name, robj);
    }
    return HC_ERROR_ALREADY_IN_A_CORPS;
}

/**��������**/
int ProcessSearchTroops(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    std::string name = "";
    READ_STR_FROM_MOBJ(name,o,"troops");
    std::string leader = "";
    READ_STR_FROM_MOBJ(leader,o,"leader");
    return corpsMgr::getInstance()->searchCorps(pc->m_id, leader, name, robj);
}

/**����������**/
int ProcessDealjoinTroops(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    robj.push_back( Pair("purpose", purpose) );
    switch (purpose)
    {
        case 1:
            if (pc->m_corps_member.get())
            {
                return HC_ERROR_ALREADY_IN_A_CORPS;
            }
            return corpsMgr::getInstance()->submitApplication(pc->m_id, id, robj);
        case 2:
            return corpsMgr::getInstance()->cancelApplication(pc->m_id, id, robj);
            break;
    }
    return HC_ERROR;
}

/**������ų�Ա**/
int ProcessDealTroopsMember(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    if (pc->m_corps_member.get())
    {
        switch (purpose)
        {
            //��������
            case 1:
                return corpsMgr::getInstance()->appointment(pc->m_id, pc->m_corps_member->corps, id, 2, robj);
            //�������
            case 2:
                return corpsMgr::getInstance()->appointment(pc->m_id, pc->m_corps_member->corps, id, 0, robj);
            //�������
            case 3:
                return corpsMgr::getInstance()->fireMember(pc->m_id, pc->m_corps_member->corps, id, robj);
        }
    }
    return HC_ERROR_NOT_JOIN_JT;
}

/**������ž��׽��**/
int ProcessDonateGold(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_vip < iDonateGoldVIP)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    int gold = 0;
    READ_INT_FROM_MOBJ(gold,o,"nums");
    if (gold <= 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    if (pc->m_corps_member.get())
    {
        if (pc->addGold(-gold) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //�������ͳ��
        add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, gold, gold_cost_for_corps_donate, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(pc,gold,gold_cost_for_corps_donate);
#endif
        //��ɫ��������
        //pc->addPrestige(2*gold);
        pc->NotifyCharData();
        robj.push_back( Pair("exp", 20*gold) );
        return corpsMgr::getInstance()->addEvent(pc, corps_event_donate_gold, gold, 0);
    }
    return HC_ERROR_NOT_JOIN_JT;
}

/**��ѯ���ų�Ա��Ϣ**/
int ProcessGetTroopsRoleInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(id).get();
    if (pc == NULL)
    {
        return HC_ERROR;
    }

    json_spirit::Object info;

    info.push_back( Pair("name", pc->m_name) );
    info.push_back( Pair("spic", pc->m_spic) );
    info.push_back( Pair("vip", pc->m_vip) );
    info.push_back( Pair("level", pc->m_level) );
    info.push_back( Pair("officer", pc->m_offical_name) );
    info.push_back( Pair("prestige", pc->m_prestige) );

    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

/**��ѯ���Ż�����б�**/
int ProcessGetTroopsActionList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get())
    {
        return corpsMgr::getInstance()->getCorpsActionList(pc,robj);
    }
    else
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
}

/**��ѯ�ʼ��б�**/
int ProcessGetMailList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_get_list;

    InsertMailcmd(cmd);

    return HC_SUCCESS_NO_RET;
}

/**��ѯ�ʼ�����**/
int ProcessQueryMailContent(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_query_mail;

    InsertMailcmd(cmd);

    return HC_SUCCESS_NO_RET;
}

/**ɾ���ʼ�/�����ʼ�**/
int ProcessSetMail(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int purpose = 2;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    switch (purpose)
    {
        case 1:
            cmd.cmd = mail_cmd_delete_mail;
            ret = HC_SUCCESS_NO_RET;
            break;
        case 2:
            cmd.cmd = mail_cmd_archive_mail;
            ret = HC_SUCCESS_NO_RET;
            break;
        case 3:
        default:
            cmd.cmd = mail_cmd_set_unread;
            robj.push_back( Pair("purpose", purpose) );
            ret = HC_SUCCESS;
            break;
    }
    InsertMailcmd(cmd);
    return ret;
}

/**�����ʼ�**/
int ProcessSendMail(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_send_mail;
    InsertMailcmd(cmd);
    return HC_SUCCESS_NO_RET;
}

/**����ɾ���ʼ�**/
int ProcessDeleteMails(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_delete_mails;
    InsertMailcmd(cmd);
    return HC_SUCCESS_NO_RET;
}

/**��ѯδ���ʼ�����**/
int ProcessGetUnread(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_get_unread;
    InsertMailcmd(cmd);
    return HC_SUCCESS_NO_RET;
}

/**��ȡ�ʼ�����**/
int ProcessGetMailAttach(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_get_mail_attach;

    InsertMailcmd(cmd);

    return HC_SUCCESS_NO_RET;
}

//�ٴλ��
int ProcessGetStrongholdLootsAgain(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_vip < 6)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    if (id == pc->m_last_stronghold && id > 0)
    {
        if (pc->addGold(-pc->m_reget_gold) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //�������ͳ��
        add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, pc->m_reget_gold, gold_cost_for_stronghold_reget, pc->m_union_id, pc->m_server_id);
        pc->NotifyCharData();
        std::list<Item> getItems;
        lootMgr::getInstance()->getStrongholdLoots(id, getItems, 0);
        if (getItems.size() == 0)
        {
            lootMgr::getInstance()->getWorldItemFall(getItems);
        }
        //������
        if (0 == giveLoots(pc, getItems, pc->m_last_stronghold_mapid, pc->m_last_stronghold_level, pc->m_last_stronghold_type, NULL, &robj, true, give_stronghold_loot))
        {
            //ʲô������û�õ� (���ܵ���/���͵���)
            getItems.clear();
            lootMgr::getInstance()->getWorldItemFall(getItems);
            giveLoots(pc, getItems, pc->m_last_stronghold_mapid, pc->m_last_stronghold_level, pc->m_last_stronghold_type, NULL, &robj, true, give_stronghold_loot);
        }
        ++pc->m_reget_times;
        pc->m_reget_gold += iRegetGold_Add;
        robj.push_back( Pair("gold", pc->m_reget_gold) );
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

//����boss
int ProcessAttackBoss(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0, type = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    READ_INT_FROM_MOBJ(type,o,"type");
    return bossMgr::getInstance()->Attack(pc->m_id, id, type, robj);
}
#if 0
//������������ͳ�ƽڵ�
int ProcessSetUserPoint(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    InsertSaveDb("replace into admin_firststat (cid,v) values (" + LEX_CAST_STR(pc->m_id) + "," + LEX_CAST_STR(id)+ ")");
    return HC_SUCCESS_NO_RET;
}
#endif
//��Ϸ���ֻ
inline int getHelperAction(CharData* cdata, json_spirit::Array& elist)
{
    if (cdata->m_helperOpen)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_helper) );
        //�Ƿ�����콱��
        if (dailyTaskMgr::getInstance()->canGetReward(*cdata))
        {
            obj.push_back( Pair("active", 1) );
        }
        elist.push_back(obj);
        return 1;
    }
    else
    {
        0;
    }
}

//�ճ�� ���˸�����bossս����Ӫս
inline int getDailyAction(CharData* cdata, json_spirit::Array& elist)
{
    if (!cdata->m_bossOpen && !cdata->m_campraceOpen && !cdata->isMapPassed(1))
    {
        return 0;
    }
    json_spirit::Object obj;
    int state = cdata->getDailyState();
    obj.push_back( Pair("type", top_level_event_daily) );
    obj.push_back( Pair("active", state) );
    elist.push_back(obj);
    return 1;
}

//��Ӣ����
inline int getEliteAction(CharData* cdata, json_spirit::Array& elist)
{
    if (cdata->m_eliteOpen)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_elite) );
        elist.push_back(obj);
        return 1;
    }
    return 0;
}

//ٺ»
inline int getSalaryAction(CharData* cdata, json_spirit::Array& elist)
{
    if (!cdata->m_hasgetsalary)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_salary) );
        obj.push_back( Pair("active", 1) );
        elist.push_back(obj);
    }
    return 1;
}

//�����
inline int getOpeningAction(CharData* cdata, json_spirit::Array& elist)
{
    int state = cdata->getOpeningState();
    if (state != -1)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_opening) );
        obj.push_back( Pair("active", state) );
        elist.push_back(obj);
    }
    //cout<<"cid "<<cdata->m_id<<", opening state "<<state<<endl;
    return 1;
}

/*���
inline int getPresentAction(CharData* cdata, json_spirit::Array& elist)
{
    int state = cdata->getGiftState();
    json_spirit::Object obj;
    obj.push_back( Pair("type", top_level_event_present) );
    obj.push_back( Pair("active", state) );
    elist.push_back(obj);
    //cout<<"cid "<<cdata->m_id<<", present state "<<state<<endl;
    return 1;
}*/

//������
inline int getChenmiAction(CharData* cdata, json_spirit::Array& elist)
{
    if (GeneralDataMgr::getInstance()->isChenmiEnable() && cdata->m_check_chenmi)
    {
        json_spirit::Object obj;
        obj.clear();
        obj.push_back( Pair("type", top_level_event_chenmi) );
        obj.push_back( Pair("active", 0) );
        uint64_t total_time = cdata->m_chenmi_time + time(NULL) - cdata->m_chenmi_start_time;
#ifdef TEST_SERVER
        if (total_time >= 300)
        {
            obj.push_back( Pair("spic", 3) );
        }
        else if (total_time >= 180)
        {
            obj.push_back( Pair("spic", 2) );
        }
#else
        if (total_time >= 18000)
        {
            obj.push_back( Pair("spic", 3) );
        }
        else if (total_time >= 10800)
        {
            obj.push_back( Pair("spic", 2) );
        }
#endif
        else
        {
            obj.push_back( Pair("spic", 1) );
        }
        elist.push_back(obj);
    }
    return 1;
}

int getAction(CharData* cdata, json_spirit::Array& elist)
{
    //��Ϸ����
    getHelperAction(cdata, elist);
    //�ճ��
    getDailyAction(cdata, elist);
    //��Ӣ����
    getEliteAction(cdata, elist);
    //������
    RaceMgr::getInstance()->getAction(cdata, elist);
    //׳��
    servantMgr::getInstance()->getAction(cdata, elist);
    //����
    guardMgr::getInstance()->getAction(cdata, elist);
    //�������ʻ
    getOpeningAction(cdata, elist);
    //���
    //getPresentAction(cdata, elist);
    //ٺ»
    getSalaryAction(cdata, elist);
    //������
    getChenmiAction(cdata, elist);
    //������
    newRankings::getInstance()->getAction(cdata, elist);
    //Ǯׯ
    bankMgr::getInstance()->getAction(cdata, elist);
    //���Ż
    corpsMgr::getInstance()->getAction(cdata,elist);
    //������
    Singleton<mazeMgr>::Instance().getAction(cdata,elist);

    //ǩ���
    Singleton<new_event_mgr>::Instance().getSignAction(cdata->m_id,elist);

    Singleton<cost_feedback_event>::Instance().getAction(*cdata,elist);

    //����¼
    Singleton<jxl_mgr>::Instance().getAction(*cdata,elist);

    //����ť����ͬʱ���͹������
    if (cdata->m_welfare == 0)
    {
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
        if (account)
        {
            account->Send(strRestNotify);
        }
    }


    if (cdata->m_currentStronghold >= iFirstRechargeStronghold)
    {
        //�׳����
        int first_recharge = cdata->queryExtraData(char_data_type_normal, char_data_first_recharge_gift);
        if (first_recharge == 0 || 1 == first_recharge)
        {
            int active = 0;
            json_spirit::Object obj;
            obj.clear();
            obj.push_back( Pair("type", top_level_event_first_recharge) );
            //������ȡ
            if (first_recharge == 1)
            {
                active = 1;
            }
            else//δ��������
            {
                //ÿ���һ��
                if (cdata->queryExtraData(char_data_type_daily, char_data_daily_view_first_recharge) == 0)
                {
                    active = 1;
                }
            }
            obj.push_back( Pair("active", active) );
            elist.push_back(obj);
        }

        //�ճ�
        Singleton<new_event_mgr>::Instance().getDailyRechargeAction(cdata->m_id, elist);

#ifdef QQ_PLAT
        //VIP��Ȩ
        {
            int active = cdata->getVipState();
            if (active != 2)
            {
                json_spirit::Object obj;
                obj.clear();
                obj.push_back( Pair("type", top_level_event_vip_present) );

                obj.push_back( Pair("active", active) );
                elist.push_back(obj);
            }
        }
#else
        //VIP��Ȩ
        {
            int active = 0;
            json_spirit::Object obj;
            obj.clear();
            obj.push_back( Pair("type", top_level_event_vip_present) );

            obj.push_back( Pair("active", cdata->getVipState()) );
            elist.push_back(obj);
        }
#endif

    }

    //����Ŀ��
    Singleton<seven_Goals_mgr>::Instance().getAction(cdata,elist);
    //�������Ͻ���
    Singleton<char_rewards_mgr>::Instance().getAction(cdata,elist);
#ifdef QQ_PLAT
    //��������
    Singleton<inviteMgr>::Instance().getAction(cdata,elist);
#endif

    //����
    Singleton<throneMgr>::Instance().getAction(*cdata,elist);
    //ս��̨
    Singleton<zstMgr>::Instance().getAction(cdata,elist);

    //�ɳ����
    {
        cdata->notifyChengzhangState();
    }

    //�齱�
    Singleton<lottery_event>::Instance().getAction(elist);

#ifdef VN_EN_SERVER
    //facebook
    if (cdata->canGetFacebookPresent())
    {
        int active = 0;
        json_spirit::Object obj;
        obj.clear();
        obj.push_back( Pair("type", top_level_event_facebook) );
        obj.push_back( Pair("active", 1) );
        elist.push_back(obj);
    }
#endif

    return HC_SUCCESS;
}

//cmd:getActionInfo//��û������Ϣ
int ProcessGetAction(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array elist;
    getAction(cdata, elist);
    robj.push_back( Pair("list", elist) );
    return HC_SUCCESS;
}

//cmd:getActionInfo//��û����tips
int ProcessGetActionMemo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = 1;
    int id = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("id", id) );
    switch (type)
    {
        case 1:
        case 2:
            return bossMgr::getInstance()->getActionMemo(type, id, robj);
        case 3:
            {
                std::string memo = "";
                int ret = campRaceMgr::getInstance()->getActionMemo(memo);
                robj.push_back( Pair("memo", memo) );
                return ret;
            }
        case 4:
            {
                std::string memo = "";
                int ret = guardMgr::getInstance()->getActionMemo(memo);
                robj.push_back( Pair("memo", memo) );
                return ret;
            }
        case 6:
            {
                robj.push_back( Pair("memo", strGroupCopyIconTips) );
                return HC_SUCCESS;
            }
            break;
        case 8:
            if (GeneralDataMgr::getInstance()->isChenmiEnable() && cdata->m_check_chenmi)
            {
                uint64_t total_time = cdata->m_chenmi_time + time(NULL) - cdata->m_chenmi_start_time;
#ifdef TEST_SERVER
                if (total_time >= 300)
                {
                    robj.push_back( Pair("memo", strAddicted_tips3) );
                }
                else if (total_time >= 180)
                {
                    robj.push_back( Pair("memo", strAddicted_tips2) );
                }
#else
                if (total_time >= 18000)
                {
                    robj.push_back( Pair("memo", strAddicted_tips3) );
                }
                else if (total_time >= 10800)
                {
                    robj.push_back( Pair("memo", strAddicted_tips2) );
                }
#endif
                else
                {
                    robj.push_back( Pair("memo", strAddicted_tips1) );
                }
                break;
            }
            else
            {
                return HC_ERROR;
            }
    }
    return HC_SUCCESS;
}

//���뻤�ͽ���
int ProcessEnterGuardScene(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (!cdata->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    return HC_SUCCESS;
}

//��ȡ���͹�����Ϣ
int ProcessGetRobGoodsEvent(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (!cdata->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    return guardMgr::getInstance()->GetRobGoodsEvent(robj);
}

//��ȡ��ɫ�ɻ��͵ĸ�
int ProcessGetGuardGoodsList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (!cdata->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    return guardMgr::getInstance()->GetGuardGoodsList(cdata, robj);
}

//��ȡ��ɫ��ǰ������Ϣ
int ProcessGetGuardInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (!pc->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    INFO("get guardinfo is calllllllllllllllllllllll" << endl);
    boost::shared_ptr<char_goods> pcg = guardMgr::getInstance()->GetCharGoods(pc->m_id);
    if (pcg.get())
    {
        Object info;
        if (pcg->m_state == 1 && pcg->m_guard_goods.get())
            info.push_back( Pair("guardLeftTime", pcg->m_guard_goods->m_end_time - time(NULL)) );
        info.push_back( Pair("guardTimesLeft", pcg->getCanGuardtime()) );
        info.push_back( Pair("robTimesLeft", pcg->getCanRobtime()) );
        info.push_back( Pair("helpTimesLeft", pcg->getCanHelptime()) );
        int rob_left_time = pcg->m_cooltime - time(NULL);
        info.push_back( Pair("robLeftTime", rob_left_time < 0 ? 0 : rob_left_time) );
        //VIP5�����ϣ�����ɫƷ�ʿ�ʼ��
        if (pc->m_vip >= 5 && pcg->m_gid < 2)
        {
            pcg->m_gid = 2;
        }
        boost::shared_ptr<base_goods> pbg = guardMgr::getInstance()->GetBaseGoods(pcg->m_gid);
        if (pbg.get())
        {
            Object goodsVO;
            goodsVO.push_back( Pair("id", pcg->m_gid) );
            goodsVO.push_back( Pair("spic", pcg->m_gid) );
            goodsVO.push_back( Pair("quality", pcg->m_gid) );
            goodsVO.push_back( Pair("name", pbg->name) );
            goodsVO.push_back( Pair("totalTime", pbg->need_min) );
            goodsVO.push_back( Pair("silver", pbg->silver * pc->m_level) );
            goodsVO.push_back( Pair("prestige", pbg->prestige) );
            goodsVO.push_back( Pair("supply", pbg->supply) );
            info.push_back( Pair("goodsVO", goodsVO) );
        }
        robj.push_back( Pair("info", info) );
        robj.push_back( Pair("more", guardMgr::getInstance()->getGuardEvent()));

        guardMgr::getInstance()->getinsence(pc->m_id);
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

//��ʼ����
int ProcessDealGoods(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (!pc->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type) );
    if (type == 1 || type == 2)
        return guardMgr::getInstance()->RefreshGoods(pc, type, robj);
    else if(type == 3)
        return guardMgr::getInstance()->Start(pc->m_level, pc->m_id);
    else
        return HC_ERROR;
}

//�����еĸٶ���
int ProcessGetGuardTaskList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (!pc->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    guardMgr::getInstance()->GetCharGoods(pc->m_id);
    return guardMgr::getInstance()->GetGuardTaskList(pc->m_id, robj);
}

//��ȡ�����еĸ���Ϣ
int ProcessGetGuardTaskInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (!pc->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return guardMgr::getInstance()->GetGuardTaskInfo(pc->m_id, id, robj);
}

//�ٸ�
int ProcessattackGuard(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (!pc->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return guardMgr::getInstance()->Rob(pc->m_id, id, robj);
}

//�뿪���ٽ���
int ProcessQuitGuard(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    guardMgr::getInstance()->getoutsence(cdata->m_id);
    return HC_SUCCESS;
}

//���ٹ���
int ProcessInspireGuard(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return guardMgr::getInstance()->inspire(cdata->m_id,robj);
}

//��ȡ��ȡ��������
int ProcessGetGuardRobScore(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return guardMgr::getInstance()->getRobScoreList(cdata->m_id,robj);
}

//��ȡ��ȡ�����
int ProcessGetGuardRobRewardsList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return guardMgr::getInstance()->getRobRankRewardsList(robj);
}

//��ȡ��ȡ�����
int ProcessGetGuardRobRewards(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return guardMgr::getInstance()->getRobRankRewards(cdata->m_id, robj);
}

int ProcessGetGuardFriendsList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (!pc->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    return guardMgr::getInstance()->GetGuardFriendsList(pc,robj);
}

int ProcessApplyGuardHelp(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (!pc->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return guardMgr::getInstance()->ApplyGuardHelp(pc, id);
}

int ProcessApplyGuardCancel(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (!pc->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    return guardMgr::getInstance()->ApplyGuardCancel(pc->m_id);
}

int ProcessAnswerGuardHelp(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (!pc->m_guardOpen)
    {
        return HC_ERROR_GUARD_NOT_ENOUGH_LEVEL;
    }
    int id = 0, type = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    READ_INT_FROM_MOBJ(type,o,"type");
    return guardMgr::getInstance()->AnswerGuardHelp(pc->m_id,id,type);
}

//��ȡ������Ӣ��Ϣ
int ProcessGetSweepList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharTempoData& char_tempo = cdata->GetTempo();
    int mapid = 0, stageid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(stageid, o, "stageid");
    json_spirit::Array stronghold_array;
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = char_tempo.CharMapsData.find(mapid);
    if (it != char_tempo.CharMapsData.end())
    {
        boost::shared_ptr<CharMapData> md = it->second;
        CharMapData::iterator itm = (*md).find(stageid);
        if (itm != (*md).end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            for (size_t i = 0; i < (size_t)itm->second->m_baseStage->size; ++i)
            {
                if (i > 24 || !itm->second->m_stronghold[i].get())
                {
                    continue;
                }
                StrongholdData* shold = itm->second->m_stronghold[i]->m_baseStronghold.get();
                if (shold)
                {
                    Object stronghold;
                    stronghold.push_back( Pair("id", shold->m_id));
                    stronghold.push_back( Pair("name", shold->m_name));
                    stronghold.push_back( Pair("level", shold->m_level));
                    stronghold.push_back( Pair("elite", shold->m_isepic));
                    stronghold.push_back( Pair("spic", shold->m_spic));
                    stronghold.push_back( Pair("pos", shold->m_strongholdpos));
                    stronghold.push_back( Pair("state", itm->second->m_stronghold[i]->m_state));
                    //������Ϣ
                    if (shold->m_loot.get() && shold->m_Item.get())
                    {
                        if (shold->m_Item->type == item_type_skill)
                        {
                            boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(shold->m_Item->id);
                            if (bs.get())
                            {
                                json_spirit::Object getobj;
                                json_spirit::Object skill;
                                skill.push_back( Pair("id", shold->m_Item->id));
                                skill.push_back( Pair("name", bs->name));
                                skill.push_back( Pair("level", shold->m_Item->nums));
                                skill.push_back( Pair("spic", shold->m_Item->id));
                                skill.push_back( Pair("curlevel", cdata->getSkillLevel(shold->m_Item->id)));
                                getobj.push_back( Pair("skill", skill));
                                stronghold.push_back( Pair("get", getobj));
                            }
                        }
                        else if (shold->m_Item->type == item_type_zhen)
                        {
                            boost::shared_ptr<BaseZhenData> bz = GeneralDataMgr::getInstance()->GetBaseZhen(shold->m_Item->id);
                            if (bz.get())
                            {
                                json_spirit::Object getobj;
                                json_spirit::Object zhen;
                                zhen.push_back( Pair("type", shold->m_Item->id));
                                zhen.push_back( Pair("name", bz->m_name));
                                zhen.push_back( Pair("level", 5));
                                boost::shared_ptr<ZhenData> zd = cdata->m_zhens.GetZhen(shold->m_Item->id);
                                zhen.push_back( Pair("curlevel", zd.get() ? zd->m_level : 0));
                                getobj.push_back( Pair("zhen", zhen));
                                stronghold.push_back( Pair("get", getobj));
                            }
                        }
                        else
                        {
                            stronghold.push_back( Pair("get", *(shold->m_loot.get())));
                        }
                    }
                    stronghold_array.push_back(stronghold);
                }
            }
        }
    }
    robj.push_back( Pair("list", stronghold_array));
    json_spirit::Object area;
    std::string area_name = GeneralDataMgr::getInstance()->GetStageName(mapid,stageid);
    area.push_back( Pair("mapid", mapid));
    area.push_back( Pair("stageid", stageid));
    area.push_back( Pair("name", area_name));
    robj.push_back( Pair("area", area));
    int next_mapid = 0,next_stageid = 0,pre_mapid = 0,pre_stageid = 0;
    if (stageid == 1)
    {
        if (mapid < cdata->m_area || (mapid == cdata->m_area && stageid < cdata->m_cur_stage))
        {
            next_mapid = mapid;
            next_stageid = stageid + 1;
        }
        if (mapid > 1)
        {
            pre_mapid = mapid-1;
            pre_stageid = 3;
        }
    }
    else if(stageid == 2)
    {
        if (mapid < cdata->m_area || (mapid == cdata->m_area && stageid < cdata->m_cur_stage))
        {
            next_mapid = mapid;
            next_stageid = stageid + 1;
        }
        {
            pre_mapid = mapid;
            pre_stageid = stageid-1;
        }
    }
    else if(stageid == 3)
    {
        if (mapid < cdata->m_area || (mapid == cdata->m_area && stageid < cdata->m_cur_stage))
        {
            next_mapid = mapid+1;
            next_stageid = 1;
        }
        {
            pre_mapid = mapid;
            pre_stageid = stageid-1;
        }
    }
    if (next_mapid > 0 && next_stageid > 0)
    {
        json_spirit::Object next_area;
        area_name = GeneralDataMgr::getInstance()->GetStageName(next_mapid,next_stageid);
        next_area.push_back( Pair("mapid", next_mapid));
        next_area.push_back( Pair("stageid", next_stageid));
        next_area.push_back( Pair("name", area_name));
        robj.push_back( Pair("nextArea", next_area));
    }
    if (pre_mapid > 0 && pre_stageid > 0)
    {
        json_spirit::Object pre_area;
        area_name = GeneralDataMgr::getInstance()->GetStageName(pre_mapid,pre_stageid);
        pre_area.push_back( Pair("mapid", pre_mapid));
        pre_area.push_back( Pair("stageid", pre_stageid));
        pre_area.push_back( Pair("name", area_name));
        robj.push_back( Pair("preArea", pre_area));
    }
    return HC_SUCCESS;
}

//���ɨ����Ӣѡ��
int ProcessAddSweepTask(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    boost::shared_ptr<charSweep> pcs = sweepMgr::getInstance()->getCharSweepData(pc->m_id);
    if (pcs.get())
    {
        if (pcs->m_end_time != 0)
            return HC_ERROR;
        pcs->m_sweep_task.clear();
        pcs->m_left_fights = 0;
        pcs->m_need_ling = 0;
    }
    else
    {
        return HC_ERROR;
    }
    int mapid = 0, type = 0;
    READ_INT_FROM_MOBJ(mapid,o,"mapid");
    READ_INT_FROM_MOBJ(type,o,"type");
    if (mapid == 0)
        return HC_ERROR;
    mArray list;
    READ_ARRAY_FROM_MOBJ(list,o,"task");
    mArray::iterator it = list.begin();
    while (it != list.end())
    {
        if ((*it).type() != obj_type)
        {
            ++it;
            continue;
        }
        mObject& tmp_obj = (*it).get_obj();
        int id = 0, stageid = 0;
        READ_INT_FROM_MOBJ(id,tmp_obj,"id");
        READ_INT_FROM_MOBJ(stageid,tmp_obj,"stageid");
        int ret = pcs->addSweepTask(mapid, stageid, id, type);
        if (ret != HC_SUCCESS)
            return ret;
        ++it;
    }
    //sweepMgr::getInstance()->Save(pc->m_id);
    robj.push_back( Pair("mapid", mapid));
    robj.push_back( Pair("type", type));
    return HC_SUCCESS;
}

//��ȡɨ����Ϣ
int ProcessGetSweepInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    boost::shared_ptr<charSweep> pcs = sweepMgr::getInstance()->getCharSweepData(cdata->m_id);
    if (pcs.get())
    {
        Object info;
        CharTempoData& char_tempo = cdata->GetTempo();
        std::map<int, boost::shared_ptr<CharMapData> >::iterator itc = char_tempo.CharMapsData.find(pcs->m_mapid);
        if (itc != char_tempo.CharMapsData.end() && itc->second.get())
        {
            boost::shared_ptr<CharMapData> md = itc->second;
            CharMapData::iterator itm = (*md).find(1);
            info.push_back( Pair("sceneName", itm->second->m_baseStage->_baseMap->name));
        }
        if (pcs->m_start_time == 0 || pcs->m_end_time == 0)
            info.push_back( Pair("leftTime", pcs->m_left_fights * iSweepEliteTime));
        else
        {
            int lefttime = pcs->m_end_time - time(NULL);
            info.push_back( Pair("leftTime", lefttime < 0 ? 0 : lefttime));
        }
        if (pcs->m_sweep_task.size())
        {
            int rounds = cdata->ling() / pcs->m_sweep_task.size();
            if (rounds <= 0)
            {
                rounds = 1;
            }
            else if(rounds >= 5)
            {
                rounds = 5;
            }
            if (pcs->m_type == 2)//��Ӣս��ɨ��ֻ��һ��
            {
                rounds = 1;
            }
            if (cdata->ling() == 0)
            {
                rounds = 0;
            }
            info.push_back( Pair("default_round", rounds));
        }
        info.push_back( Pair("leftFights", pcs->m_left_fights));
        if (pcs->m_type == 2)
        {
            info.push_back( Pair("totalFights", pcs->m_need_ling));
        }
        else
        {
            info.push_back( Pair("totalFights", pcs->m_need_ling/4));
        }
        std::list<int>::iterator it = pcs->m_sweep_task.begin();
        Array list;
        while (it != pcs->m_sweep_task.end())
        {
            Object tmp;
            if (pcs->m_type == 2)//ս��ɨ��
            {
                boost::shared_ptr<eliteCombat> sd = eliteCombatMgr::getInstance()->getEliteCombat(pcs->m_mapid, *it);
                if (!sd.get())
                {
                    return HC_ERROR;
                }
                tmp.push_back( Pair("name", sd->_name));
                tmp.push_back( Pair("level", sd->_level));
                list.push_back(tmp);
            }
            else
            {
                boost::shared_ptr<StrongholdData> sd = GeneralDataMgr::getInstance()->GetStrongholdData(*it);
                tmp.push_back( Pair("name", sd->m_name));
                tmp.push_back( Pair("level", sd->m_level));
                list.push_back(tmp);
            }
            ++it;
        }
        robj.push_back( Pair("info", info));
        robj.push_back( Pair("list", list));
        robj.push_back( Pair("rest", iVIPRestTimes[cdata->m_vip] - cdata->m_gold_rest > 0 ? (iVIPRestTimes[cdata->m_vip] - cdata->m_gold_rest) : 0) );
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

//��ȡɨ�����
int ProcessGetSweepResult(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    boost::shared_ptr<charSweep> pcs = sweepMgr::getInstance()->getCharSweepData(cdata->m_id);
    if (pcs.get())
    {
        Array result_list;
        std::vector<boost::shared_ptr<SweepResult> >::iterator it_out = pcs->m_sweep_itemlist.begin();
        while (it_out != pcs->m_sweep_itemlist.end() && (*it_out).get())
        {
            Array list;
            std::vector<boost::shared_ptr<Item> >::iterator it = (*it_out)->itemlist.begin();
            while (it != (*it_out)->itemlist.end() && (*it).get())
            {
                Object get;
                switch ((*it)->type)
                {
                    case item_type_silver:    //����
                    {
                        get.push_back( Pair("silver", (*it)->nums));
                        break;
                    }
                    case item_type_treasure:    //����
                    {
                        json_spirit::Object gem;
                        gem.push_back( Pair("id", (*it)->id));
                        boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure((*it)->id);
                        if (tr.get())
                        {
                            gem.push_back( Pair("name", tr->name));
                            gem.push_back( Pair("spic", tr->spic));
                            gem.push_back( Pair("quality", tr->quality));
                            gem.push_back( Pair("nums", (*it)->nums));
                        }
                        else
                        {
                            ERR();
                        }
                        get.push_back( Pair("gem", gem));
                        break;
                    }
                    case item_type_equipment://װ��
                    {
                        json_spirit::Object equip;
                        equip.push_back( Pair("id", (*it)->id));
                        boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment((*it)->id);
                        if (tr.get())
                        {
                            equip.push_back( Pair("name", tr->name));
                            equip.push_back( Pair("spic", tr->baseid));
                            equip.push_back( Pair("quality", tr->quality));
                        }
                        else
                        {
                            ERR();
                        }
                        get.push_back( Pair("equipment", equip));
                        break;
                    }
                    case item_type_general://�佫
                    {
                        json_spirit::Object general;
                        general.push_back( Pair("id", (*it)->id));
                        boost::shared_ptr<GeneralTypeData> tr = GeneralDataMgr::getInstance()->GetBaseGeneral((*it)->id);
                        if (tr.get())
                        {
                            general.push_back( Pair("name", tr->m_name));
                            general.push_back( Pair("spic", tr->m_spic));
                        }
                        else
                        {
                            ERR();
                        }
                        get.push_back( Pair("general", general));
                        break;
                    }
                    case item_type_zhen:    //����
                    {
                        boost::shared_ptr<BaseZhenData> bz = GeneralDataMgr::getInstance()->GetBaseZhen((*it)->id);
                        if (bz.get())
                        {
                            json_spirit::Object zhen;
                            zhen.push_back( Pair("type", bz->m_type));
                            zhen.push_back( Pair("name", bz->m_name));
                            zhen.push_back( Pair("level", 5));
                            get.push_back( Pair("zhen", zhen));
                        }
                        break;
                    }
                    case item_type_skill:    //����
                    {
                        boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill((*it)->id);
                        if (bs.get())
                        {
                            json_spirit::Object skill;
                            skill.push_back( Pair("id", (*it)->id));
                            skill.push_back( Pair("name", bs->name));
                            skill.push_back( Pair("level", (*it)->nums));
                            skill.push_back( Pair("spic", (*it)->id));
                            get.push_back( Pair("skill", skill));
                        }
                        break;
                    }
                    case item_type_silver_map:
                    {
                        get.push_back( Pair("silver", (*it)->nums) );
                        break;
                    }
                    case item_type_gold:
                    {
                        get.push_back( Pair("gold", (*it)->nums) );
                        break;
                    }
                    case item_type_ling:
                    {
                        get.push_back( Pair("ling", (*it)->nums) );
                        break;
                    }
                    case item_type_prestige:
                    {
                        get.push_back( Pair("prestige", (*it)->nums) );
                        break;
                    }
                }
                Object info;
                info.push_back( Pair("get", get));
                list.push_back(info);
                ++it;
            }
            Object result_obj;
            result_obj.push_back( Pair("get_list", list));
            result_list.push_back(result_obj);
            ++it_out;
        }
        robj.push_back( Pair("list", result_list));
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

extern volatile int g_sweep_fast_vip;
extern const int iMazeTimesEveryday;

//��ȡɨ�����
int ProcessDealSweep(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 0, times = 0, auto_buy = 0, auto_sell = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_INT_FROM_MOBJ(times, o, "times");
    READ_INT_FROM_MOBJ(auto_buy, o, "auto_buy");
    READ_INT_FROM_MOBJ(auto_sell, o, "auto_sell");
    robj.push_back( Pair("type", type));
    boost::shared_ptr<charSweep> pcs = sweepMgr::getInstance()->getCharSweepData(cdata->m_id);
    if (pcs.get() && !pcs->m_sweep_task.empty())
    {
        robj.push_back( Pair("sweep_type", pcs->m_type));
        if (type == 0 && pcs->m_end_time == 0)//��ʼɨ��
        {
            //�����޷�ɨ��
            if (cdata->m_bag.isFull())
            {
                return HC_ERROR_BAG_FULL;
            }
            if (pcs->m_type == 2)
            {
                times = 1;
            }
            else if (times > 100)
            {
                times = 100;
            }
            pcs->m_left_fights = times * pcs->m_sweep_task.size();
            if (pcs->m_left_fights > 300)
            {
                pcs->m_left_fights = 300;
            }
            if (pcs->m_type == 2)
            {
                pcs->m_need_ling = pcs->m_left_fights;
            }
            else
            {
                pcs->m_need_ling = pcs->m_left_fights*4;
            }
            if (pcs->m_need_ling > cdata->ling())
                return HC_ERROR_NOT_ENOUGH_LING;

            if (cdata->m_vip < g_sweep_fast_vip)
            {
                pcs->m_fast_mod = 0;
                pcs->m_start_time = time(NULL);
                pcs->m_end_time = pcs->m_start_time + pcs->m_left_fights * iSweepEliteTime;
            }
            else
            {
                pcs->m_start_time = 1;
                pcs->m_end_time = 1;
                pcs->m_fast_mod = 1;
            }
            robj.push_back( Pair("totalFights", pcs->m_left_fights));
            pcs->m_auto_buy_ling = (auto_buy == 1);
            pcs->m_auto_sell = (auto_sell == 1);
            return pcs->start();
        }
        //��ʼɨ���������
        else if (type == 3 && pcs->m_end_time == 0)
        {
            //�����޷�ɨ��
            if (cdata->m_bag.isFull())
            {
                return HC_ERROR_BAG_FULL;
            }
            if (pcs->m_type == 2)
            {
                pcs->m_left_fights = cdata->ling();
                if (pcs->m_left_fights > pcs->m_sweep_task.size())
                {
                    pcs->m_left_fights = pcs->m_sweep_task.size();
                }
                pcs->m_need_ling = pcs->m_left_fights;
            }
            else
            {
                pcs->m_left_fights = cdata->ling()/4;
                if (pcs->m_left_fights > 300)
                {
                    pcs->m_left_fights = 300;
                }
                pcs->m_need_ling = pcs->m_left_fights*4;
            }
            if (pcs->m_need_ling > cdata->ling())
                return HC_ERROR_NOT_ENOUGH_LING;

            if (cdata->m_vip < g_sweep_fast_vip)
            {
                pcs->m_fast_mod = 0;
                pcs->m_start_time = time(NULL);
                pcs->m_end_time = pcs->m_start_time + pcs->m_left_fights * iSweepEliteTime;
            }
            else
            {
                pcs->m_start_time = 1;
                pcs->m_end_time = 1;
                pcs->m_fast_mod = 1;
            }
            robj.push_back( Pair("totalFights", pcs->m_left_fights));
            pcs->m_auto_buy_ling = (auto_buy == 1);
            pcs->m_auto_sell = (auto_sell == 1);
            return pcs->start();
        }
        else if (type == 1)//ȡ��ɨ��
        {
            return pcs->cancel();
        }
        //��ѯɨ��״̬
        else if (type == 2)
        {
            boost::shared_ptr<charSweep> pcs = sweepMgr::getInstance()->getCharSweepData(cdata->m_id);
            if (pcs.get())
            {
                if (pcs->m_start_time != 0 && pcs->m_end_time != 0)
                {
                    robj.push_back( Pair("state", 1));
                    if (pcs->m_type == 2)
                    {
                        robj.push_back( Pair("totalFights", pcs->m_need_ling));
                    }
                    else
                    {
                        robj.push_back( Pair("totalFights", pcs->m_need_ling/4));
                    }
                    if (cdata->isNewPlayer() > 0)
                    {
                        json_spirit::Object obj;
                        obj.push_back( Pair("cmd", "notify") );
                        obj.push_back( Pair("s", 200) );
                        obj.push_back( Pair("type", notify_msg_new_player_sweep) );
                        obj.push_back( Pair("nums", cdata->isNewPlayer()) );
                        cdata->sendObj(obj);
                    }
                    return HC_SUCCESS;
                }
            }
            robj.push_back( Pair("state", 0));
        }
        else
        {
            return HC_ERROR;
        }
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

//��ȡ������
int ProcessGetTacticInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    boost::shared_ptr<baseMap> bm = GeneralDataMgr::getInstance()->GetBaseMap(id);
    Object info;
    if (bm.get())
    {
        std::string intro = bm->intro;
        std::string toreplace = "$N";
        size_t pos = intro.find(toreplace);
        while (pos != std::string::npos)
        {
            intro.replace(pos, toreplace.length(), cdata->m_name);
            pos = intro.find(toreplace);
        }
        info.push_back( Pair("intro", intro));
        info.push_back( Pair("get", bm->get));
        std::map<int,int>::iterator it = cdata->m_map_intro_get.find(id);
        if (it != cdata->m_map_intro_get.end())
            info.push_back( Pair("isGet", it->second));
        else
            info.push_back( Pair("isGet", 0));
    }
    robj.push_back( Pair("info", info));
    return HC_SUCCESS;
}

//��ȡ��������
int ProcessGetSupplyHelpList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    json_spirit::Array list;
    //1֧������2��Ϸ����3����4�Ӷ�5��������6������7̽��
    static int typeMap[] = {1,2,3,4,5,6,7};
    for (int j = 0; j < 7; ++j)
    {
        int i = typeMap[j];
        json_spirit::Object obj;
        obj.clear();
        obj.push_back( Pair("type", i) );
        if (i == 3)
        {
            if (cdata->m_currentStronghold >= iFarmOpenStronghold[0])
            {
                int state = 0;
                int lefttime = farmMgr::getInstance()->getCoolTime(cdata->m_id, state);
                int now_seed = cdata->queryExtraData(char_data_type_daily, char_data_farm_seed);
                int total_seed = farmMgr::getInstance()->FieldNum(cdata->m_id)*2;
                if (lefttime > 0)
                {
                    obj.push_back( Pair("state", 1) );
                    obj.push_back( Pair("leftTime", lefttime) );
                }
                else if(total_seed > now_seed)
                {
                    obj.push_back( Pair("state", 2) );
                }
                else
                {
                    obj.push_back( Pair("state", 3) );
                }
            }
            else
            {
                obj.push_back( Pair("level", iFarmOpenLevel[0]) );
            }
        }
        else if(i == 5)
        {
            if (cdata->m_guardOpen)
            {
                boost::shared_ptr<char_goods> pcg = guardMgr::getInstance()->GetCharGoods(cdata->m_id);
                if (pcg.get())
                {
                    if (pcg->m_guard_goods.get())
                    {
                        int lefttime = pcg->m_guard_goods->m_end_time - time(NULL);
                        if (lefttime > 0)
                        {
                            obj.push_back( Pair("state", 1) );
                            obj.push_back( Pair("leftTime", lefttime) );
                        }
                    }
                    else if(pcg->getCanGuardtime() > 0)
                    {
                        obj.push_back( Pair("state", 2) );
                    }
                    else
                    {
                        obj.push_back( Pair("state", 3) );
                    }
                }
            }
            else
            {
                obj.push_back( Pair("level", iGuardOpenStronghold/2+1) );
            }
        }
        else if(i == 6)
        {
            if (cdata->m_currentStronghold >= iMazeOpenStronghold)
            {
                int left = 0;
                int enterTimes = cdata->queryExtraData(char_data_type_daily, char_data_daily_maze_times);
                left = iMazeTimesEveryday - enterTimes;
                if (left <= 0)
                {
                    obj.push_back( Pair("state", 3) );
                }
                else
                {
                    boost::shared_ptr<char_maze> cm = Singleton<mazeMgr>::Instance().getChar(cdata->m_id);
                    if (cm.get() && cm->m_curMaze.get())
                    {
                        int lefttime = cm->m_timeout - time(NULL);
                        if (lefttime > 0)
                        {
                            obj.push_back( Pair("state", 1) );
                            obj.push_back( Pair("leftTime", lefttime) );
                        }
                        else if(left > 0)
                        {
                            obj.push_back( Pair("state", 2) );
                        }
                    }
                }
            }
            else
            {
                obj.push_back( Pair("level", iMazeOpenStronghold/2+1) );
            }
        }
        else if(i == 7)
        {
            if (cdata->m_corps_member.get() == NULL)
            {
                //�޾���
                obj.push_back( Pair("state", 0) );
            }
            else
            {
                charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(cdata->m_id).get();
                if (c == NULL)
                {
                    obj.push_back( Pair("state", 0) );
                }
                int nums = cdata->queryExtraData(char_data_type_daily, char_data_daily_corps_explore);
                splsCorps* cp = corpsMgr::getInstance()->findCorps(cdata->m_corps_member.get()->corps);
                if (cp)
                {
                    int left = iCorpsExploreTimesOneday[cp->_level] - nums;
                    int lefttime = c->getDoneTime();
                    if (lefttime > 0)
                    {
                        obj.push_back( Pair("state", 1) );
                        obj.push_back( Pair("leftTime", lefttime) );
                    }
                    else if(left > 0)
                    {
                        obj.push_back( Pair("state", 2) );
                    }
                    else
                    {
                        obj.push_back( Pair("state", 3) );
                    }
                }
            }
        }
        list.push_back(obj);
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//��ȡ��Ϸ����
int ProcessGetHelpList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    json_spirit::Array list;
    json_spirit::Array finish_list;
    json_spirit::Object obj;
    bool finish = false;
    if (cdata->m_buyLingOpen)//�������
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_rest);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_officeOpen && cdata->m_salary > 0)//��ְٺ»
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_salary);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    //ͨ����ʱδ����
    if (cdata->m_raceOpen)//����
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<CharRaceData> rd = RaceMgr::getInstance()->getRaceData(cdata->m_id);
        if (rd.get() && rd->getChar())
        {
            boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_race);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            int cd_sec = rd->m_race_cd_time - time(NULL);
            //��ȴʱ��
            if (cd_sec > 0)
            {
                obj.push_back( Pair("coolTime", cd_sec) );
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
    }
    if (cdata->m_equiptOpen)//ǿ��
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_equipt);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_levyOpen)//����
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_levy);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_washOpen)//ϴ��
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_wash);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->isMapPassed(1))//���˸���
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_group_copy);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_horseOpen)//ս��
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_horse);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_farmOpen)//����
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_farm);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        int state = 0;
        int cd_sec = farmMgr::getInstance()->getCoolTime(cdata->m_id, state);
        //��ȴʱ��
        if (cd_sec > 0)
        {
            obj.push_back( Pair("coolTime", cd_sec) );
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_tradeOpen)//ͨ��
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_trade);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_corps_member.get())//�������
    {
        //����
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_corp_jisi);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
        //���
        obj.clear();
        finish = false;
        splsCorps* cp = corpsMgr::getInstance()->findCorps(cdata->m_corps_member.get()->corps);
        if (cp && cp->_level >= corpsMgr::getInstance()->getCorpsActionLevel(corps_action_yanhui))
        {
            pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_corp_yanhui);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
        //̽��
        obj.clear();
        finish = false;
        if (cp && cp->_level >= corpsMgr::getInstance()->getCorpsActionLevel(corps_action_tansuo))
        {
            pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_corp_explore);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
        //ԯ�����
        obj.clear();
        finish = false;
        if (cp && cp->_level >= corpsMgr::getInstance()->getCorpsActionLevel(corps_action_sheji))
        {
            pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_corp_ymsj);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
        //������
        obj.clear();
        finish = false;
        if (cp && cp->_level >= corpsMgr::getInstance()->getCorpsActionLevel(corps_action_lottery))
        {
            pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_corp_lottery);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
    }
    if (cdata->m_guardOpen)//����
    {
        //����
        obj.clear();
        finish = false;
        boost::shared_ptr<char_goods> pcg = guardMgr::getInstance()->GetCharGoods(cdata->m_id);
        if (pcg.get())
        {
            boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_guard);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            if (pcg->m_guard_goods.get())
            {
                int cd_sec = pcg->m_guard_goods->m_end_time - time(NULL);
                //��ȴʱ��
                if (cd_sec > 0)
                {
                    obj.push_back( Pair("coolTime", cd_sec) );
                }
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
        //���
        obj.clear();
        finish = false;
        if (pcg.get())
        {
            boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_guard_rob);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            int cd_sec = pcg->m_cooltime - time(NULL);
            //��ȴʱ��
            if (cd_sec > 0)
            {
                obj.push_back( Pair("coolTime", cd_sec) );
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
        #if 0
        //����
        obj.clear();
        if (pcg.get())
        {
            boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_guard_help);
            if (pbd.get())
            {
                pbd->to_obj(cdata, obj);
            }
            list.push_back(obj);
        }
        #endif
    }
    if (cdata->m_servantOpen)//�Ҷ�
    {
        //����
        obj.clear();
        finish = false;
        boost::shared_ptr<charServant> p = servantMgr::getInstance()->GetCharServant(cdata->m_id);
        if (p.get())
        {
            boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_servant);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            int cd_sec = p->m_interact_cooltime - time(NULL);
            //��ȴʱ��
            if (cd_sec > 0)
            {
                obj.push_back( Pair("coolTime", cd_sec) );
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
        //���
        obj.clear();
        finish = false;
        if (p.get())
        {
            boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_servant_rescue);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
        //ץ��
        obj.clear();
        finish = false;
        if (p.get())
        {
            boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_servant_catch);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
    }
    if (cdata->m_bossOpen)//ս����
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<spls_boss> spb = bossMgr::getInstance()->getBoss(1, 0);
        if (spb.get())
        {
            boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_attak_boss1);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            //obj.push_back( Pair("state", spb->_boss._open_state) );

            if (spb->_boss._open_state)
            {
                obj.push_back( Pair("state", 1) );
            }
            else
            {
                time_t timep;
                struct tm m_timenow;
                time(&timep);
                localtime_r(&timep, &m_timenow);

                int season = 0;
                boost::shared_ptr<DateInfo> date = GeneralDataMgr::getInstance()->GetDate();
                if (date.get())
                {
                    season = date->season;
                }
                openRule* pOpenRule = spb->_boss._open_rules.getRule(m_timenow, season);
                if (pOpenRule
                    && (pOpenRule->_open_hour > m_timenow.tm_hour
                        || (pOpenRule->_open_hour == m_timenow.tm_hour
                            && pOpenRule->_open_min > m_timenow.tm_min)
                       )
                    )
                {
                    obj.push_back( Pair("state", 0) );
                }
                else
                {
                    obj.push_back( Pair("state", 2) );
                }
            }
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
        obj.clear();
        finish = false;
        spb = bossMgr::getInstance()->getBoss(4, 0);
        if (spb.get())
        {
            boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_attak_boss4);
            if (pbd.get())
            {
                finish = pbd->to_obj(cdata, obj);
            }
            obj.push_back( Pair("state", spb->_boss._open_state) );
            if (finish)
            {
                finish_list.push_back(obj);
            }
            else
            {
                list.push_back(obj);
            }
        }
    }
    if (cdata->m_campraceOpen)//��Ӫս
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_camprace);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        obj.push_back( Pair("state", campRaceMgr::getInstance()->isOpen()) );
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_rebornOpen)//�佫����
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_general_reborn);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_trainOpen)//�佫ѵ��
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_general_train);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        int state = 0;
        int cd_sec = cdata->generalTrainCoolTime(state);
        //��ȴʱ��
        if (cd_sec > 0)
        {
            obj.push_back( Pair("coolTime", cd_sec) );
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_baoshi_count > 0)//��ʯ�һ�
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_baoshi_exchange);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_currentStronghold >= iMazeOpenStronghold)//������
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_maze);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    if (cdata->m_baoshi_count)//��ȡ��ʯ
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_get_yushi);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            int cur = cdata->queryExtraData(char_data_type_daily, char_data_get_yushi);
            if (iFreeYushi > cur)
            {
                int cd_sec = cdata->queryExtraData(char_data_type_daily, char_data_yushi_time_cd) - time(NULL);
                if (cd_sec > 0)
                {
                    obj.push_back( Pair("coolTime", cd_sec) );
                }
            }
            list.push_back(obj);
        }
    }
    if (Singleton<throneMgr>::Instance().actionState() && cdata->m_level >= 45)//�ΰ�
    {
        obj.clear();
        finish = false;
        boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(daily_task_throne_con);
        if (pbd.get())
        {
            finish = pbd->to_obj(cdata, obj);
        }
        if (finish)
        {
            finish_list.push_back(obj);
        }
        else
        {
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list));
    robj.push_back( Pair("finish_list", finish_list));
    #if 0
    json_spirit::Object info;
    info.push_back( Pair("level", cdata->m_level));
    info.push_back( Pair("ling", cdata->ling()));
    boost::shared_ptr<CharRaceData> rd = RaceMgr::getInstance()->getRaceData(cdata->m_id);
    if (rd.get() && rd->m_charactor.get())
    {
        info.push_back( Pair("rank", rd->m_rank));
    }
    info.push_back( Pair("attack", cdata->getAttack(cdata->m_zhens.m_default_zhen)));
    info.push_back( Pair("camp", cdata->m_camp));
    info.push_back( Pair("prestige", cdata->m_prestige));
    boost::shared_ptr<baseoffical> p_bo = GeneralDataMgr::getInstance()->GetBaseOffical(cdata->m_offical);
    if (p_bo.get())
    {
        info.push_back( Pair("officer", p_bo->m_name));
    }
    robj.push_back( Pair("info", info));
    #endif
    //�ճ�������Ϣ
    obj.clear();
    dailyTaskMgr::getInstance()->toObj(*cdata,obj);
    robj.push_back( Pair("dailyTask", obj));
    #if 0
    obj.clear();
    obj.push_back( Pair("supply", cdata->m_bag.getGemCount(treasure_type_supply)) );
    int need_supply = 0, max_supply = 0;
    cdata->GetMaxSupply(need_supply,max_supply);
    obj.push_back( Pair("need_supply", need_supply) );
    obj.push_back( Pair("max_supply", max_supply) );
    robj.push_back( Pair("supply_info", obj));
    #endif
    return HC_SUCCESS;
}

int ProcessFindBackHelp(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    if (!cdata->canFindBack())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<base_daily_task> pbd = dailyTaskMgr::getInstance()->getDaily_task(id);
    if (pbd.get())
    {
        json_spirit::Object obj;
        if (pbd->can_findback && pbd->to_obj(cdata, obj))
        {
            if (cdata->queryExtraData(char_data_type_daily, char_data_daily_findback_task_start + id) == 0)
            {
                return dailyTaskMgr::getInstance()->findBack(cdata,id);
            }
        }
    }
    return HC_ERROR;
}

//��ȡ����δ���
int ProcessGetLogOutList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    json_spirit::Array list;
    json_spirit::Object obj;
    if (cdata->m_buyLingOpen)//����
    {
        obj.clear();
        obj.push_back( Pair("type", daily_task_rest));
        obj.push_back( Pair("num1", cdata->ling()));
        obj.push_back( Pair("num2", iVIPRestTimes[cdata->m_vip] - cdata->m_gold_rest));
        list.push_back(obj);
    }
    if (cdata->m_officeOpen)//��ְٺ»
    {
        obj.clear();
        obj.push_back( Pair("type", daily_task_salary));
        obj.push_back( Pair("num1", cdata->m_hasgetsalary == 0 ? 1 : 0));
        list.push_back(obj);
    }
    if (cdata->m_raceOpen)//����
    {
        obj.clear();
        boost::shared_ptr<CharRaceData> rd = RaceMgr::getInstance()->getRaceData(cdata->m_id);
        if (rd.get() && rd->getChar())
        {
            obj.push_back( Pair("type", daily_task_race));
            int total_times = rd->m_race_times;
            CharData* pc = rd->getChar();
            //����Ĵ���
            int buyTime = pc->queryExtraData(char_data_type_daily, char_data_buy_race);
            int left = (iRaceFreeTimes + buyTime) - total_times;
            if (left < 0)
            {
                left = 0;
            }
            obj.push_back( Pair("num1", left));
            list.push_back(obj);
        }
    }
    if (cdata->m_levyOpen)//����
    {
        obj.clear();
        obj.push_back( Pair("type", daily_task_levy));
        obj.push_back( Pair("num1", iLevyTimes[cdata->m_vip] + iLevyFreeTime - cdata->m_levy_time));
        list.push_back(obj);
    }
    if (cdata->isMapPassed(1))//���˸���
    {
        obj.clear();
        obj.push_back( Pair("type", daily_task_group_copy));
        int total_time = 0;
        obj.push_back( Pair("num1", groupCopyMgr::getInstance()->getAllCopyCanAttackTimes(cdata->m_id, total_time)));
        list.push_back(obj);
    }
    if (cdata->m_horseOpen)//ս��
    {
        int left = iHorseTrainTime - (cdata->m_silver_train_horse + cdata->m_gold_train_horse);
        if (left < 0)
        {
            left = 0;
        }
        obj.clear();
        obj.push_back( Pair("type", daily_task_horse));
        obj.push_back( Pair("num1", left));
        list.push_back(obj);
    }
    if (cdata->m_farmOpen)//����
    {
        obj.clear();
        int left_field_num = 0;
        int left_delay_num = 0;
        boost::shared_ptr<fieldlist> fl = farmMgr::getInstance()->GetCharFieldList(cdata->m_id);
        if (fl.get())
        {
            for (size_t j = 0; j < (*fl).size(); ++j)
            {
                if ((*fl)[j]->m_state == 2 || (*fl)[j]->m_state == 3)
                {
                    ++left_field_num;
                }
                #if 0
                if ((*fl)[j]->m_state < 4 && (*fl)[j]->m_delay_times < iDelayFarmVIP[cdata->m_vip])
                {
                    left_delay_num += (iDelayFarmVIP[cdata->m_vip] - (*fl)[j]->m_delay_times);
                }
                #endif
            }
        }
        obj.push_back( Pair("type", daily_task_farm));
        obj.push_back( Pair("num1", left_field_num));
        obj.push_back( Pair("num2", left_delay_num));
        list.push_back(obj);
    }
    if (cdata->m_corps_member.get())//�������
    {
        //����
        obj.clear();
        obj.push_back( Pair("type", daily_task_corp_jisi));
        obj.push_back( Pair("num1", cdata->m_temp_jisi_times == 0 ? 1 : 0));
        list.push_back(obj);
    }
    if (cdata->m_guardOpen)//����
    {
        //����
        obj.clear();
        boost::shared_ptr<char_goods> pcg = guardMgr::getInstance()->GetCharGoods(cdata->m_id);
        if (pcg.get())
        {
            obj.push_back( Pair("type", daily_task_guard));
            obj.push_back( Pair("num1", pcg->getCanGuardtime()));
            list.push_back(obj);
        }
        //���
        obj.clear();
        if (pcg.get())
        {
            obj.push_back( Pair("type", daily_task_guard_rob));
            obj.push_back( Pair("num1", pcg->getCanRobtime()));
            list.push_back(obj);
        }
    }
    if (cdata->m_servantOpen)//�Ҷ�
    {
        //����
        obj.clear();
        boost::shared_ptr<charServant> p = servantMgr::getInstance()->GetCharServant(cdata->m_id);
        if (p.get())
        {
            obj.push_back( Pair("type", daily_task_servant));
            obj.push_back( Pair("num1", iServantInteractTime- p->m_interact_time));
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list));
    return HC_SUCCESS;
}

//��ѯ�������ۼ�ʱ��
int ProcessGetChenmiTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (!pc->m_check_chenmi)
    {
        robj.push_back( Pair("time", 0));
    }
    else
    {
        uint64_t secs = (pc->m_chenmi_time + time(NULL) - pc->m_chenmi_start_time);
        if (secs == 0)
        {
            secs = 1;
        }
        robj.push_back( Pair("time", secs));
    }
    return HC_SUCCESS;
}

//���ս���
int ProcessGetLevyInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    Object info;
    info.push_back( Pair("free", iLevyFreeTime - pc->m_levy_time));
    info.push_back( Pair("cur", pc->m_levy_time));
    info.push_back( Pair("total", iLevyTimes[pc->m_vip] + iLevyFreeTime));
    info.push_back( Pair("gold", pc->getLevyCost(pc->m_levy_time - iLevyFreeTime + 1)));
    info.push_back( Pair("silver", pc->getLevyReward()));
    int dj_count = pc->m_bag.getGemCount(treasure_type_levy_ling);
    info.push_back( Pair("levy_ling", dj_count ));
    int tmp_left_time = iLevyTimes[pc->m_vip] + iLevyFreeTime - pc->m_levy_time;
    //10�ε����ĺͻ��
    int needgold = 0, cnt = 10;
    if (tmp_left_time < cnt)
        cnt = tmp_left_time;
    int silver = pc->getLevyReward() * cnt;
    for (int i = dj_count + 1; i <= cnt; ++i)
    {
        needgold += pc->getLevyCost(pc->m_levy_time - iLevyFreeTime + i);
    }
    info.push_back( Pair("massGold1", needgold));
    info.push_back( Pair("massSilver1", silver));
    //100��
    needgold = 0, silver = 0, cnt = 100;
    if (tmp_left_time < cnt)
        cnt = tmp_left_time;
    silver = pc->getLevyReward() * cnt;
    for (int i = dj_count + 1; i <= cnt; ++i)
    {
        needgold += pc->getLevyCost(pc->m_levy_time - iLevyFreeTime + i);
    }
    info.push_back( Pair("massGold2", needgold));
    info.push_back( Pair("massSilver2", silver));
    if (pc->m_vip < 12)
    {
        info.push_back( Pair("nextTotal", iLevyTimes[pc->m_vip + 1] + iLevyFreeTime));
    }
    robj.push_back( Pair("info", info));
    return HC_SUCCESS;
}

//����
int ProcessDealLevy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type) );
    int cnt = 0, getsilver = 0;
    bool be_double = false;
    if (type == 1)
    {
        cnt = 1;
    }
    else if(type == 2)
    {
        if (pc->m_vip < iLevyMoreTimeVIP)
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        cnt = 10;
    }
    else if(type == 3)
    {
        if (pc->m_vip < iLevyMaxTimeVIP)
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        cnt = 100;
    }
    int tmp_left_time = iLevyTimes[pc->m_vip] + iLevyFreeTime - pc->m_levy_time;
    if (tmp_left_time < cnt)
        cnt = tmp_left_time;

    int total_cnt = cnt;
    if (cnt > 0 && (pc->m_levy_time + cnt - iLevyFreeTime <= iLevyTimes[pc->m_vip]))
    {
        int needgold = 0, dj_use = 0;
        int dj_count = pc->m_bag.getGemCount(treasure_type_levy_ling);
        std::vector<int> gold_cost;
        for (int i = 1; i <= cnt; ++i)
        {
            if (dj_use < dj_count)
            {
                ++dj_use;
            }
            else
            {
                int gold = pc->getLevyCost(pc->m_levy_time - iLevyFreeTime + i);
                needgold += gold;
                gold_cost.push_back(gold);
            }
        }
        if (pc->gold() < needgold)
            return HC_ERROR_NOT_ENOUGH_GOLD;
        while (pc->m_levy_time < iLevyFreeTime)
        {
            //free
            ++pc->m_levy_time;
            //�ճ�����
            //dailyTaskMgr::getInstance()->updateDailyTask(*pc);
            --cnt;
            int silver = pc->getLevyReward();
#if 1
            if (my_random(1,100) <= 20)
            {
                getsilver += silver * 3 / 2;
                be_double = true;
            }
            else
#endif
            {
                getsilver += silver;
            }
        }
        if (cnt > 0)
        {
            if (-1 == pc->addGold(-needgold))
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            if (dj_use > 0)
            {
                //�۵���
                pc->addTreasure(treasure_type_levy_ling, -dj_use);

                //��������ͳ��
                add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_levy_ling,dj_use,treasure_unknow,2,pc->m_union_id,pc->m_server_id);
            }
            for (std::vector<int>::iterator it = gold_cost.begin(); it != gold_cost.end(); ++it)
            {
                //�������ͳ��
                add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, *it, gold_cost_for_levy, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
                gold_cost_tencent(pc,*it,gold_cost_for_levy);
#endif
            }
            pc->m_levy_time += cnt;
            int silver = pc->getLevyReward();
#if 1
            for (int i = 1; i <= cnt; ++i)
            {
                if (my_random(1,100) <= 20)
                {
                    getsilver += silver * 3 / 2;
                    be_double = true;
                }
                else
                {
                    getsilver += silver;
                }
            }
#else
            //ȡ������
            getsilver = cnt * silver;
#endif
            //�ճ�����
            dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_levy,cnt);
        }
        pc->addSilver(getsilver);

        //���һ��ͳ��
        add_statistics_of_silver_get(pc->m_id,pc->m_ip_address,getsilver,silver_get_levy, pc->m_union_id, pc->m_server_id);
        std::string msg = "";
        if (be_double)
        {
            msg = strLevyCri;
            robj.push_back( Pair("baoji", 1) );
        }
        else
        {
            msg = strLevySuc;
        }
        str_replace(msg, "$R", LEX_CAST_STR(getsilver));

        //֪ͨ��������
        if (dj_use > 0)
        {
            msg += "\n" + treasure_expend_msg(treasure_type_levy_ling, dj_use);
        }
        robj.push_back( Pair("msg", msg) );

        //֧������
        pc->m_trunk_tasks.updateTask(task_levy, total_cnt);

        pc->saveCharDailyVar();
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
    return HC_ERROR;
}

//ս����ӻ��Ϣ
int ProcessGetHorseActionList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return horseMgr::getInstance()->getHorseFruitsList(*cdata, robj);
}

//ս����Ӳ���
int ProcessDealHorseFruit(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = 1, id = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("id", id) );
    return horseMgr::getInstance()->dealHorseFruit(*cdata, type, id);
}

//ս�����
int ProcessGetHorseInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }

    //ս��չʾ��Ϣ
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"cid");
    if (id > 0 && id != cdata->m_id)
    {
        CharData* cdata2 = GeneralDataMgr::getInstance()->GetCharData(id).get();
        if (cdata2)
        {
            robj.push_back( Pair("show", true) );
            return horseMgr::getInstance()->getHorseInfo(*cdata2, robj);
        }
        else
        {
            return HC_ERROR;
        }
    }
    else
    {
        return horseMgr::getInstance()->getHorseInfo(*cdata, robj);
    }
}

//ս������
int ProcessTrainHorse(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type) );
    int cri_type = 0, get_exp = 0, dj_use = 0;
    std::string msg = "";
    ret = horseMgr::getInstance()->trainHorse(*cdata, type, cri_type, get_exp, dj_use);
    if (ret == HC_SUCCESS)
    {
        switch (cri_type)
        {
            case 2:    //�󱩻�
            {
                std::string bmsg = strHorseCri;
                str_replace(bmsg, "$N", MakeCharNameLink(cdata->m_name));
                str_replace(bmsg, "$H", horseMgr::getInstance()->NameToLink(cdata->m_id,cdata->m_horse.horse->name,cdata->m_horse.horse->quality));
                GeneralDataMgr::getInstance()->broadCastSysMsg(bmsg, -1);
                msg = strHorsebigBao;
                break;
            }
            case 1:    //С����
            {
                msg = strHorseBao;
                if (type == 3)
                    msg = strHorseGold;
                str_replace(msg, "$E", LEX_CAST_STR(get_exp));
                break;
            }
            case 0:
            default:
            {
                msg = strHorseSuc;
                str_replace(msg, "$E", LEX_CAST_STR(get_exp));
                break;
            }
        }
        //�ճ�����
        dailyTaskMgr::getInstance()->updateDailyTask(*cdata,daily_task_horse);
    }
    if (dj_use > 0)
    {
        msg += "\n" + treasure_expend_msg(treasure_type_mati_tie, dj_use);
    }
    robj.push_back( Pair("msg", msg) );
    robj.push_back( Pair("cri_type", cri_type) );
    return ret;
}

//ս��ת��
int ProcessTurnHorse(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return horseMgr::getInstance()->turnHorse(*cdata, robj);
}

//��ȡ�Ҷ���Ϣ
int ProcessGetServantInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return servantMgr::getInstance()->getServantInfo(*cdata, robj);
}

//��ȡץ����־��Ϣ
int ProcessGetServantEventsList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return servantMgr::getInstance()->getServantEventsList(*cdata, robj);
}

//��ȡ�Ҷ��б���Ϣ
int ProcessGetServantList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = 0, purpose = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("purpose", purpose) );
    return servantMgr::getInstance()->getServantList(*cdata, type, purpose, robj);
}

//��ȡ�����б���Ϣ
int ProcessGetInteractionList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return servantMgr::getInstance()->getInteractionList(*cdata, robj);
}

//�Ҷ�ϵͳ����
int ProcessDealServantAction(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = 0, id = 0, aid = 0, extra_type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    READ_INT_FROM_MOBJ(extra_type,o,"extra_type");
    READ_INT_FROM_MOBJ(id,o,"id");
    READ_INT_FROM_MOBJ(aid,o,"aid");
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("extra_type", extra_type) );
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("aid", aid) );
    return servantMgr::getInstance()->DealServantAction(*cdata, type, extra_type, id, aid, robj);
}

//��ѯ��ɫ��ϸ��Ϣ
int ProcessGetRoleDetail(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    std::string cname = "";
    READ_STR_FROM_MOBJ(cname,o,"name");
    if (cname != "")
    {
        int cid = GeneralDataMgr::getInstance()->GetCharId(cname);
        if (cid)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (pc)
            {
                return pc->getRoleDetail(robj);
            }
        }
    }
    return cdata->getRoleDetail(robj);
}

//�����ر���������
int ProcessEnableStandIn(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (cdata->m_vip < 5)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    int state = 1;
    READ_INT_FROM_MOBJ(state,o,"state");
    if (state)
    {
        state = 1;
    }
    else
    {
        state = 0;
    }
    if (type == action_boss)
    {
        int boss_id = 1;
        READ_INT_FROM_MOBJ(boss_id,o,"bossId");
        boost::shared_ptr<spls_boss> spb = bossMgr::getInstance()->getBoss(boss_id, 0);
        if (spb.get())
        {
            spb->m_stand_in_mob.setStandIn(cdata->m_id, state);
        }
    }
    else if (type == action_camp_race)
    {
        campRaceMgr::getInstance()->m_stand_in_mob.setStandIn(cdata->m_id, state);
    }
    return HC_SUCCESS;
}

volatile int g_open_log_msg = 1;

struct my_msg_st {

    long int my_msg_type;
    char some_text[256];

    my_msg_st()
    {
        my_msg_type = 1;
        memset(some_text, 0, 256);
    }
};

void shhx_log_msg(const std::string& msg, int type = 1)
{
    static int inited = 0;
    static key_t key = ftok(GetExeName().c_str(), 'a');
    static int msg_id = msgget(key, IPC_CREAT|0666);

    if (inited == 0)
    {
        if (-1 == key)
        {
            cout<<"****** create key fail ******"<<endl;
        }
        else if (msg_id == -1)
        {
            cout<<"****** msgget fail ******"<<endl;
        }
        inited = 1;
    }
    if (g_open_log_msg && msg_id)
    {
        my_msg_st smsg;
        smsg.my_msg_type = type;
        strncpy(smsg.some_text, msg.c_str(), 256);
        msgsnd(msg_id, &smsg, strlen(smsg.some_text) + 1, IPC_NOWAIT);
    }
}

//��ȡ�츳�����б�
int ProcessGetGeniusUpgradeList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid,o,"id");
    robj.push_back( Pair("id", gid) );
    return geniusMgr::getInstance()->GetGeniusUpgradeList(cdata->m_id,gid,robj);
}

//�����츳
int ProcessOpenGenius(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid,o,"id");
    return geniusMgr::getInstance()->OpenGenius(cdata->m_id,gid,robj);
}

//ϴ�츳
int ProcessCleanGenius(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int gid = 0;
    READ_INT_FROM_MOBJ(gid,o,"id");
    mArray list;
    READ_ARRAY_FROM_MOBJ(list,o,"lock");
    std::vector<int> lock_list;
    mArray::iterator it = list.begin();
    while (it != list.end())
    {
        int id = (*it).get_int();
        lock_list.push_back(id);
        ++it;
    }
    return geniusMgr::getInstance()->CleanGenius(cdata->m_id,gid,lock_list,robj);
}

//��ֲ�츳
int ProcessGraftGenius(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int gid = 0, gid2 = 0;
    READ_INT_FROM_MOBJ(gid,o,"id");
    READ_INT_FROM_MOBJ(gid2,o,"id2");
    return geniusMgr::getInstance()->GraftGenius(cdata->m_id,gid,gid2,robj);
}

//��ȡ�������
int ProcessGetOnlineGift(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ���������Ϣ
int ProcessQueryOnlineGift(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//���������ʱ��
int ProcessOnlineGiftTimer(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid,o,"cid");
    online_gift_mgr::getInstance()->on_timer(cid);
    return HC_SUCCESS;
}

//��ʾ��ͼ�ؿ��б�
int ProcessGMQueryStrongholds(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    json_spirit::Array map_list;

    for (int i = 1; i <= max_map_id; ++i)
    {
        boost::shared_ptr<baseMap> pmap = GeneralDataMgr::getInstance()->GetBaseMap(i);
        if (pmap.get())
        {
            json_spirit::Object mapo;
            mapo.push_back(Pair("id", i) );
            mapo.push_back(Pair("name", pmap->name) );
            //mapo.push_back(Pair("openLevel", pmap->openLevel) );

            json_spirit::Array stage_list;
            for (int j = 0; j < 3; ++j)
            {
                if (pmap->stages[j].get())
                {
                    json_spirit::Object stageo;
                    stageo.push_back( Pair("id", pmap->stages[j]->id) );
                    stageo.push_back( Pair("name", pmap->stages[j]->name) );

                    json_spirit::Array stronghold_list;
                    for (int s = 0; s < 25; ++s)
                    {
                        if (pmap->stages[j]->_baseStrongholds[s].get())
                        {
                            json_spirit::Object strongholdo;
                            strongholdo.push_back( Pair("id", pmap->stages[j]->_baseStrongholds[s]->m_id) );
                            strongholdo.push_back( Pair("name", pmap->stages[j]->_baseStrongholds[s]->m_name) );
                            strongholdo.push_back( Pair("spic", pmap->stages[j]->_baseStrongholds[s]->m_spic) );
                            stronghold_list.push_back(strongholdo);
                        }
                    }
                    stageo.push_back( Pair("list", stronghold_list) );
                    stage_list.push_back(stageo);
                }
            }
            mapo.push_back( Pair("list", stage_list) );
            map_list.push_back(mapo);
        }
    }

    robj.push_back( Pair("list", map_list) );

    return HC_SUCCESS;
}

//��ѯ�ؿ���Ϣ
int ProcessGMQueryStrongholdInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "pos");

    //�����ؿ���Ϣ
    StrongholdData* psh = GeneralDataMgr::getInstance()->GetStrongholdData(id).get();
    if (!psh)
    {
        return HC_ERROR;
    }

    robj.push_back( Pair("name", psh->m_name) );
    robj.push_back( Pair("level", psh->m_level) );
    robj.push_back( Pair("stateNum", psh->m_stateNum) );

    json_spirit::Array glist;
    for (int i = 0; i < 9; ++i)
    {
        if (psh->m_generals[i].get())
        {
            json_spirit::Object posa;
            posa.push_back( Pair("pos", psh->m_generals[i]->m_pos) );
            posa.push_back( Pair("name", psh->m_generals[i]->m_name) );
            posa.push_back( Pair("spic", psh->m_generals[i]->m_spic) );
            posa.push_back( Pair("stype", psh->m_generals[i]->m_stype) );
            posa.push_back( Pair("level", psh->m_generals[i]->m_level) );

            posa.push_back( Pair("zhi", psh->m_generals[i]->m_int) );
            posa.push_back( Pair("yong", psh->m_generals[i]->m_str) );
            posa.push_back( Pair("gong", psh->m_generals[i]->m_attack) );
            posa.push_back( Pair("pufang", psh->m_generals[i]->m_pufang) );
            posa.push_back( Pair("cefang", psh->m_generals[i]->m_cefang) );
            posa.push_back( Pair("hp", psh->m_generals[i]->m_hp) );

            if (psh->m_generals[i]->m_speSkill.get())
            {
                posa.push_back( Pair("skill", psh->m_generals[i]->m_speSkill->id) );
            }
            else
            {
                posa.push_back( Pair("skill", 0) );
            }

            glist.push_back(posa);
        }
    }
    robj.push_back( Pair("list", glist) );
    return HC_SUCCESS;
}

//����ؿ���Ϣ
int ProcessGMSaveStrongholdInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "pos");

    //�����ؿ���Ϣ
    StrongholdData* psh = GeneralDataMgr::getInstance()->GetStrongholdData(id).get();
    if (!psh)
    {
        return HC_ERROR;
    }

    bool changed = false;
    std::string name = "";
    READ_STR_FROM_MOBJ(name,o,"name");
    if (name != "")
    {
        psh->m_name = name;
    }
    int level = -1;
    READ_INT_FROM_MOBJ(level,o,"level");
    if (level >= 0)
    {
        psh->m_level = level;
    }
    int stateNum = -1;
    READ_INT_FROM_MOBJ(stateNum,o,"stateNum");
    if (stateNum != -1)
    {
        psh->m_stateNum = stateNum;
    }

    saveDbJob job;
    if (changed)
    {
        job.sqls.push_back("update base_stronghold set level=" + LEX_CAST_STR(psh->m_level)
            + ",stateNum=" + LEX_CAST_STR(psh->m_stateNum)
            + ",name='" + psh->m_name + "' where id=" + LEX_CAST_STR(psh->m_id));
    }

    json_spirit::mArray glist;
    READ_ARRAY_FROM_MOBJ(glist,o,"list");

    if (glist.size())
    {
        for (int i = 0; i < 9; ++i)
        {
            psh->m_generals[i].reset();
        }
        job.sqls.push_back("delete from base_stronghold_generals where stronghold=" + LEX_CAST_STR(psh->m_id));
    }
    for (json_spirit::mArray::iterator it = glist.begin(); it != glist.end(); ++it)
    {
        json_spirit::mValue& value = *it;
        if (value.type() != json_spirit::obj_type)
        {
            continue;
        }
        json_spirit::mObject&posa = value.get_obj();
        int pos = 0, spic = 0, stype = 0, level = 0, zhi = 0, yong = 0, gong = 0, pufang = 0, cefang = 0;
        READ_INT_FROM_MOBJ(pos, posa, "pos");

        if (pos < 1 || pos > 9)
        {
            continue;
        }
        psh->m_generals[pos-1].reset(new StrongholdGeneralData);
        psh->m_generals[pos-1]->m_pos = pos;

        READ_STR_FROM_MOBJ(psh->m_generals[pos-1]->m_name, posa, "name");
        READ_INT_FROM_MOBJ(psh->m_generals[pos-1]->m_spic, posa, "spic");
        READ_INT_FROM_MOBJ(psh->m_generals[pos-1]->m_stype, posa, "stype");
        READ_INT_FROM_MOBJ(psh->m_generals[pos-1]->m_level, posa, "level");
        READ_INT_FROM_MOBJ(psh->m_generals[pos-1]->m_int, posa, "zhi");
        READ_INT_FROM_MOBJ(psh->m_generals[pos-1]->m_str, posa, "yong");
        READ_INT_FROM_MOBJ(psh->m_generals[pos-1]->m_attack, posa, "gong");
        READ_INT_FROM_MOBJ(psh->m_generals[pos-1]->m_pufang, posa, "pufang");
        READ_INT_FROM_MOBJ(psh->m_generals[pos-1]->m_cefang, posa, "cefang");
        READ_INT_FROM_MOBJ(psh->m_generals[pos-1]->m_hp, posa, "hp");
        int skill = 0;
        READ_INT_FROM_MOBJ(skill, posa, "skill");
        psh->m_generals[pos-1]->m_speSkill = GeneralDataMgr::getInstance()->getSpeSkill(skill);

        psh->m_generals[pos-1]->m_baseSoldier = GeneralDataMgr::getInstance()->GetBaseSoldier(psh->m_generals[pos-1]->m_stype);

        job.sqls.push_back("insert into base_stronghold_generals (stronghold,pos,name,spic,stype,hp,attack,pufang,cefang,str,wisdom,skill) values ("
                + LEX_CAST_STR(psh->m_id) + ","
                + LEX_CAST_STR(psh->m_generals[pos-1]->m_pos) + ",'"
                + LEX_CAST_STR(psh->m_generals[pos-1]->m_name) + "',"
                + LEX_CAST_STR(psh->m_generals[pos-1]->m_spic) + ","
                + LEX_CAST_STR(psh->m_generals[pos-1]->m_stype) + ","
                + LEX_CAST_STR(psh->m_generals[pos-1]->m_hp) + ","
                + LEX_CAST_STR(psh->m_generals[pos-1]->m_attack) + ","
                + LEX_CAST_STR(psh->m_generals[pos-1]->m_pufang) + ","
                + LEX_CAST_STR(psh->m_generals[pos-1]->m_cefang) + ","
                + LEX_CAST_STR(psh->m_generals[pos-1]->m_str) + ","
                + LEX_CAST_STR(psh->m_generals[pos-1]->m_int) + ","
                + LEX_CAST_STR(skill)
                + ")"
                );
    }

    if (job.sqls.size())
    {
        InsertSaveDb(job);
    }
    return HC_SUCCESS;
}

//���򱳰�λ
int ProcessBuyBagSlot(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int num = 0;
    READ_INT_FROM_MOBJ(num, o, "num");
    return pc->buyBagSlot(num, robj);
}

//��������λ
int ProcessSwapBagSlot(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int slot1 = 0, slot2 = 0;
    READ_INT_FROM_MOBJ(slot1, o, "slot1");
    READ_INT_FROM_MOBJ(slot2, o, "slot2");
    if (slot1 != slot2)
    {
        pc->m_bag.swapItem(slot1, slot2);
    }
    return HC_SUCCESS;
}

//�򿪱���λ��Ʒ
int ProcessOpenBagItem(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int slot = 0, nums = 0;
    READ_INT_FROM_MOBJ(slot, o, "slot");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    return pc->openSlotItm(slot, nums, robj);
}

//�򿪱���λ���
int ProcessOpenBagLibao(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int slot = 0;
    READ_INT_FROM_MOBJ(slot, o, "slot");
    return pc->openLibao(slot, robj);
}

//�򿪳ɳ����
int ProcessGetChengzhangLibaoInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    return pc->getChengzhangLibaoInfo(pos, robj);
}

//װ������
int ProcessMakeEquipment(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int eid = 0, type = 0;
    READ_INT_FROM_MOBJ(eid, o, "eid");
    READ_INT_FROM_MOBJ(type, o, "type");
    bool cost_gold = (type == 1);
    return pc->upgradeEquipment(eid, cost_gold, robj);
}

//��ѯװ��������Ϣ
int ProcessGetEquipmentMakeInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int eid = 0, type = 0;
    READ_INT_FROM_MOBJ(eid, o, "eid");
    READ_INT_FROM_MOBJ(type, o, "type");
    pc->getUpgradeEquipmentInfo(eid, type, robj);
    return HC_SUCCESS;
}

//��ѯ�Ƽ�װ��
int ProcessGetBestEquipment(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int level = 0, type = 0;
    READ_INT_FROM_MOBJ(level, o, "level");
    READ_INT_FROM_MOBJ(type, o, "type");
    return pc->m_bag.getBestBagEquipments(level, type, robj);
}

//������
int ProcessSortBbag(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    pc->m_bag.sortBag();
    return HC_SUCCESS;
}

//ʹ����Ʒ
int ProcessUseItem(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int use = 0;
    READ_INT_FROM_MOBJ(use,o,"use");
    robj.push_back( Pair("use", use) );
    switch (use)
    {
        //̽����
        case ITEM_USAGE_EXPLORE_LING:
        {
            if (pc->m_corps_member.get() == NULL)
            {
                return HC_ERROR_JOIN_CORPS_TO_USE;
            }
            break;
        }
        //ת�˷�
        case ITEM_USAGE_CORPS_LOTTERY:
        {
            if (pc->m_corps_member.get() == NULL)
            {
                return HC_ERROR_JOIN_CORPS2_TO_USE;
            }
            if (corpsMgr::getInstance()->getCorpsLevel(pc->m_corps_member->corps) < 2)
            {
                return HC_ERROR_JOIN_CORPS2_TO_USE;
            }
            break;
        }
        //ս��
        case ITEM_USAGE_HORSE_TRAIN:
        {
            if (pc->m_horseOpen == 0)
            {
                return HC_ERROR_HORSE_NOT_OPEN;
            }
            break;
        }
        //����
        case ITEM_USAGE_LEVY_LING:
        {
            if (pc->m_levyOpen == 0)
            {
                return HC_ERROR_LEVY_NOT_OPEN;
            }
            break;
        }
        //�ݱ�
        case ITEM_USAGE_SOUL_LING:
        case ITEM_USAGE_TRAIN:
        {
            if (pc->m_soulOpen == 0)
            {
                return HC_ERROR_SOUL_NOT_OPEN;
            }
            break;
        }
        //ó��
        case ITEM_USAGE_TRADE_LING:
        {
            if (pc->m_tradeOpen == 0)
            {
                return HC_ERROR_TRADE_NOT_OPEN;
            }
            break;
        }
        //ѵ��
        case ITEM_USAGE_TRAIN_LING:
        {
            if (pc->m_trainOpen == 0)
            {
                return HC_ERROR_TRAIN_NOT_OPEN;
            }
            break;
        }
    }
    return HC_SUCCESS;
}

//��λװ��
int ProcessLocateEquip(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    int guid = 0;
    int level = pc->m_bag.maxEquipLevel(id, guid);
    int gid = 0;
    for (std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = pc->m_generals.m_generals.begin(); it != pc->m_generals.m_generals.end(); ++it)
    {
        if (it->second.get())
        {
            int gguid = 0;
            int qlevel = it->second->m_equipments.maxEquipLevel(id, gguid);
            if (qlevel >= level)
            {
                level = qlevel;
                gid = it->first;
                guid = gguid;
            }
        }
    }
    if (guid == 0)
    {
        if (id > equip_slot_max)
        {
            //��ǰװ���Ҳ�����Ѱ���������װ����װ��
            int new_id = id-equip_slot_max;
            int new_level = pc->m_bag.maxEquipLevel(new_id, guid);
            gid = 0;
            for (std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = pc->m_generals.m_generals.begin(); it != pc->m_generals.m_generals.end(); ++it)
            {
                if (it->second.get())
                {
                    int gguid = 0;
                    int qlevel = it->second->m_equipments.maxEquipLevel(new_id, gguid);
                    if (qlevel >= new_level)
                    {
                        new_level = qlevel;
                        gid = it->first;
                        guid = gguid;
                    }
                }
            }
        }
        if (guid == 0)
        {
            gid = 0;
        }
    }

    robj.push_back( Pair("type", (id-1)%equip_slot_max + 1) );
    robj.push_back( Pair("id", guid) );
    robj.push_back( Pair("gid", gid) );
    return HC_SUCCESS;
}

int ProcessGetGMqlist(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_get_gm_question;
    InsertMailcmd(cmd);
    return HC_SUCCESS_NO_RET;
}

int ProcessSubGMquestion(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_send_gm_question;
    InsertMailcmd(cmd);
    return HC_SUCCESS_NO_RET;
}

int ProcessStartTmpVIP(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return pc->startTmpVIP();
}

int ProcessGetTmpVIPTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int timenow = time(NULL);
    if (pc->m_tmp_vip_end_time > timenow)
    {
        robj.push_back( Pair("tmpvip_lefttime", pc->m_tmp_vip_end_time-timenow));
    }
    else
    {
        robj.push_back( Pair("tmpvip_lefttime", 0));
    }
    return HC_SUCCESS;
}

int ProcessGetFriendsList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    if (type == 1)
    {
        Singleton<relationMgr>::Instance().getFriendsList(pc->m_id, robj);
    }
    else
    {
        Singleton<relationMgr>::Instance().getApplicationList(pc->m_id, robj);
    }
    //pc->getFriendsList(type,robj);
    robj.push_back( Pair("type", type));
    return HC_SUCCESS;
}

int ProcessDealFriends(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int purpose = 0, id = 0;
    std::string name = "";
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    READ_INT_FROM_MOBJ(id,o,"id");
    READ_STR_FROM_MOBJ(name,o,"name");

    robj.push_back( Pair("purpose", purpose));
    //pc->loadFriends();
    switch (purpose)
    {
        case 1://�ύ����
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            if (id == 0)
                return HC_ERROR_NAME;
            return Singleton<relationMgr>::Instance().acceptApplication(pc->m_id, id);
            //return pc->submitApplication(name);
        case 2://ͨ������
            robj.push_back( Pair("id", id));
            return Singleton<relationMgr>::Instance().acceptApplication(pc->m_id, id);
            //return pc->acceptApplication(id, true);
        case 3://ͨ��ȫ������
            robj.push_back( Pair("id", id));
            return Singleton<relationMgr>::Instance().acceptAllApplication(pc->m_id);
            //return pc->acceptAllApplication();
        case 4://�ܾ�����
            robj.push_back( Pair("id", id));
            return Singleton<relationMgr>::Instance().rejectApplication(pc->m_id, id);
            //return pc->rejectApplication(id);
        case 5://�ܾ���������
            robj.push_back( Pair("id", id));
            return Singleton<relationMgr>::Instance().rejectAllApplication(pc->m_id);
            //return pc->rejectAllApplication();
        case 6://ɾ������
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            if (id == 0)
                return HC_ERROR_NAME;
            return Singleton<relationMgr>::Instance().removeAttention(pc->m_id, id);
        case 7:
            {
                json_spirit::mArray ids;
                READ_ARRAY_FROM_MOBJ(ids,o,"ids");
                for (json_spirit::mArray::iterator it = ids.begin(); it != ids.end(); ++it)
                {
                    json_spirit::mValue& value = *it;
                    if (value.type() == int_type)
                    {
                        Singleton<relationMgr>::Instance().acceptApplication(pc->m_id, value.get_int());
                    }
                }
                return HC_SUCCESS;
            }
            //return pc->deleteFriend(name);
    }
    return HC_SUCCESS;
}

int ProcessGetNearPlayerList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_area < iOpenNearMap)
    {
        return HC_SUCCESS;
    }
    return GeneralDataMgr::getInstance()->getNearPlayerList(pc,robj);
}

//��ѯ�ղ����ʱ��
int ProcessQueryCollectGift(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int get = pc->queryExtraData(char_data_type_normal, char_data_get_collect_reward);
    int create_time = time(NULL) - pc->m_createTime;
    if (create_time < 86400 && pc->m_currentStronghold >= iCollectLibaoStronghold && get < 1)
        robj.push_back( Pair("left_time", 86400 - create_time));
    return HC_SUCCESS;
}

//��ȡ�ղ����
int ProcessGetCollectGift(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int get = pc->queryExtraData(char_data_type_normal, char_data_get_collect_reward);
    int create_time = time(NULL) - pc->m_createTime;
    if (create_time < 86400 && pc->m_currentStronghold >= iCollectLibaoStronghold && get < 1)
    {
        pc->setExtraData(char_data_type_normal, char_data_get_collect_reward, 1);
        pc->addGold(100);
        //��һ��ͳ��
        add_statistics_of_gold_get(pc->m_id,pc->m_ip_address,100,gold_get_gift, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
        gold_get_tencent(pc,100);
#endif
        pc->NotifyCharData();
        robj.push_back( Pair("gold", 100));
    }
    //actͳ��
    act_to_tencent(pc,act_new_collect_reward);
    return HC_SUCCESS;
}

//����������Ϣ
int ProcessGetNewRankingsInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return newRankings::getInstance()->getRankingsInfo(cdata,robj);
}

//����������Ϣ
int ProcessGetLastNewRankingsInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return newRankings::getInstance()->getLastRankingsInfo(cdata,robj);
}

//��ȡ���н���
int ProcessGetNewRankingsReward(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return newRankings::getInstance()->getRankRewards(cdata->m_id,robj);
}

int ProcessSetNewPlayerEnd(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    cdata->setExtraData(char_data_type_normal, char_data_new_player_end, 1);
    return HC_SUCCESS;
}

//�����ʱ��������
int ProcessQueryCharBuffs(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    json_spirit::Array list;
    cdata->m_Buffs.getBuffInfo(list);
    robj.push_back( Pair("buffs", list) );
    return HC_SUCCESS;
}

//��ʱ����״̬�ı�
int ProcessBuffChange(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid,o,"cid");
    //֪ͨ�ͻ���
    json_spirit::Object action;
    action.push_back( Pair("cmd", "buff_change") );
    action.push_back( Pair("s", 200) );
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (cdata.get())
    {
        cdata->m_Buffs.refresh();
        cdata->sendObj(action);
    }
    return HC_SUCCESS;
}

//�̳�
int ProcessQueryMallInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int page = 1, nums_per_page = 0, be_suggest = 0, type = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    READ_INT_FROM_MOBJ(be_suggest, o, "be_suggest");
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("be_suggest", be_suggest) );
    robj.push_back( Pair("type", type) );
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page == 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<baseGoods> >& p_list = GeneralDataMgr::getInstance()->GetBaseMallGoods();
    std::map<int, boost::shared_ptr<baseGoods> >::iterator it = p_list.begin();
	//�����̳��ۿ�
	float fDiscount = GeneralDataMgr::getInstance()->getMallDiscount();
    while(it != p_list.end())
    {
        boost::shared_ptr<baseGoods> p_bg = it->second;
        if (p_bg.get())
        {
            if (be_suggest)//�Ƽ��б�
            {
                if (p_bg->be_suggest == 0)
                {
                    ++it;
                    continue;
                }
            }
            else
            {
                if (p_bg->type != type)
                {
                    ++it;
                    continue;
                }
            }
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                if (p_bg->m_item.get())
                {
                    Object obj;
                    p_bg->m_item->toObj(obj);
                    obj.push_back( Pair("goods_id", p_bg->id));
                    obj.push_back( Pair("type", p_bg->type));
                    obj.push_back( Pair("be_suggest", p_bg->be_suggest));
                    obj.push_back( Pair("gold_to_buy", (int)(p_bg->gold_to_buy * fDiscount)));
                    list.push_back(obj);
                }
                else
                {
                    ERR();
                }
            }
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    if (page > maxpage)
        page = maxpage;
    Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//�̳�
int ProcessBuyMallGoods(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0, nums = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    if (nums < 1)
        nums = 1;
    std::map<int, boost::shared_ptr<baseGoods> >& p_list = GeneralDataMgr::getInstance()->GetBaseMallGoods();
    std::map<int, boost::shared_ptr<baseGoods> >::iterator it = p_list.find(id);
	//�����̳��ۿ�
	float fDiscount = GeneralDataMgr::getInstance()->getMallDiscount();
	if (it != p_list.end())
    {
        boost::shared_ptr<baseGoods> p_bg = it->second;
        if (p_bg.get() && p_bg->gold_to_buy > 0 && p_bg->m_item.get())
        {
            if (cdata->m_bag.isFull())
            {
                return HC_ERROR_BAG_FULL;
            }
			//����ܼ���������򳬴�����ʱ
			int cost_gold = (int)(p_bg->gold_to_buy * fDiscount) * nums;
            if (cost_gold < 0)
            {
                return HC_ERROR;
            }
            if (p_bg->m_item->type == item_type_baoshi)
            {
                if (cdata->addGold(-cost_gold) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                //����ʯ
                int n = cdata->m_bag.addBaoshi(p_bg->m_item->id,p_bg->m_item->fac,nums);
                if (n < nums)
                {
                    cdata->addGold((nums-n)*(int)(p_bg->gold_to_buy * fDiscount));
                    cost_gold = (int)(p_bg->gold_to_buy * fDiscount) * n;
                }
                add_statistics_of_baoshi_get(cdata->m_id, cdata->m_ip_address, cdata->m_union_id, cdata->m_server_id, p_bg->m_item->id, p_bg->m_item->fac, n, baoshi_buy);
                //��ʯ�
                Singleton<new_event_mgr>::Instance().addBaoshi(cdata->m_id, p_bg->m_item->fac);
                //�������ͳ��
                add_statistics_of_gold_cost(cdata->m_id,cdata->m_ip_address,cost_gold,gold_cost_for_baoshi+p_bg->m_item->id*100+p_bg->m_item->fac, cdata->m_union_id, cdata->m_server_id);
                #ifdef QQ_PLAT
                gold_cost_tencent(cdata,cost_gold,gold_cost_for_buy_baoshi,gold_cost_for_baoshi+p_bg->m_item->id*100+p_bg->m_item->fac,nums);
                #endif
            }
            else if(p_bg->m_item->type == item_type_treasure)
            {
                boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(p_bg->m_item->id);
                if (!bt.get())
                {
                    return HC_ERROR;
                }
                if (bt->max_size > 0)
                {
                    int left = cdata->m_bag.size() - cdata->m_bag.getUsed();
                    left *= bt->max_size;
                    if (left < nums)
                    {
                        nums = left;
                    }
                }
                cost_gold = (int)(p_bg->gold_to_buy * fDiscount) * nums;
                if (cdata->addGold(-cost_gold) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                //������
                int err_code = 0;
                cdata->m_bag.addGem(p_bg->m_item->id, nums, err_code);
                //�������ͳ��
                add_statistics_of_gold_cost(cdata->m_id,cdata->m_ip_address,cost_gold,gold_cost_for_treasure+p_bg->m_item->id, cdata->m_union_id, cdata->m_server_id);
                #ifdef QQ_PLAT
                gold_cost_tencent(cdata,cost_gold,gold_cost_for_buy_daoju,p_bg->m_item->id,nums);
                #endif
            }
            else if(p_bg->m_item->type == item_type_libao)
            {
                int left = cdata->m_bag.size() - cdata->m_bag.getUsed();
                if (left < nums)
                {
                    nums = left;
                    cost_gold = (int)(p_bg->gold_to_buy * fDiscount) * nums;
                }
                if (cdata->addGold(-cost_gold) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                cdata->addLibao(p_bg->m_item->id, nums);
                //�������ͳ��
                add_statistics_of_gold_cost(cdata->m_id,cdata->m_ip_address,cost_gold,gold_cost_for_libao+p_bg->m_item->id, cdata->m_union_id, cdata->m_server_id);
                #ifdef QQ_PLAT
                gold_cost_tencent(cdata,cost_gold,gold_cost_for_buy_libao);
                #endif
            }
            //֪ͨ
            json_spirit::Object getobj;
            Item i;
            i.type = p_bg->m_item->type;
            i.id = p_bg->m_item->id;
            i.fac = p_bg->m_item->fac;
            i.nums = nums;
            i.toObj(getobj);
            robj.push_back( Pair("get", getobj) );
            cdata->NotifyCharData();
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

#ifdef VN_EN_SERVER
//facebook���Ϣ
int ProcessGetFacebookPresentInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return cdata->getFacebookPresentInfo(robj);
}

//facebook���Ϣ
int ProcessGetFacebookPresent(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    return cdata->getFacebookPresent();
}
#endif

template <typename Job>
class worker_new
{
public:
    worker_new(const std::string& name, jobqueue<Job>& _jobqueue, std::size_t _maxthreads = 1);
    virtual ~worker_new(void);

public:
    void run();
    void stop();
    virtual bool work(Job& task) = 0;        // ��������Ҫ���ش��麯��,����ɹ���.
    int running();
protected:
    void workloop();                    // ����ѭ��.

private:
    std::string worker_name;
    std::vector<boost::shared_ptr<boost::thread> > threads_;
    boost::mutex mutex_;
    std::size_t maxthreads_;
    volatile int _runing_loop;
    jobqueue<Job>& jobqueue_;
    volatile bool exitthread;
};

template <typename Job>
worker_new<Job>::worker_new(const std::string& name,jobqueue<Job>& _jobqueue, std::size_t _maxthreads/* = 4*/) :
worker_name(name),
maxthreads_(_maxthreads),
_runing_loop(0),
jobqueue_(_jobqueue),
exitthread(false)
{
}

template <typename Job>
worker_new<Job>::~worker_new(void)
{
}

template <typename Job>
void worker_new<Job>::run()
{
    try
    {
        for (std::size_t i = 0; i < maxthreads_; ++i) {
            boost::shared_ptr<boost::thread> _thread(new boost::thread(
                boost::bind(&worker_new::workloop, this)));
            threads_.push_back(_thread);
        }

        for (std::size_t i = 0; i < maxthreads_; ++i) {
            threads_[i]->join();
        }
    }
    catch (std::exception& e)
    {
        std::cout << "ERROR INFO:" << e.what() << std::endl;
    }
}

template <typename Job>
void worker_new<Job>::stop()
{
    exitthread = true;
    jobqueue_.notify_all();
}

template <typename Job>
int worker_new<Job>::running()
{
    return _runing_loop;
}

template <typename Job>
void worker_new<Job>::workloop()               // ���й�����Щ���.
{
    ++_runing_loop;
#ifdef DEBUG_PER
    time_t last_time = 0;
    uint64_t processed_cmd = 0;
#endif
    //cout<<" ************************* action wookloop , tid="<<boost::this_thread::get_id()<<" *************************  "<<endl;
    do
    {
#ifdef DEBUG_PER

        //if (g_print_debug_info)
        {
            uint64_t processed = jobqueue_._processed_cmds-processed_cmd;
            time_t time_now = time(NULL);
            if (last_time != time_now && time_now % 10 == 0)
            {
                uint64_t inqueue = jobqueue_._total_cmds - jobqueue_._processed_cmds;
                if (processed >= 2000 || inqueue > 100)
                {
                    cout<<"======================= action work : "<<dec<<jobqueue_._processed_cmds<<"/"<<inqueue<<"("<<processed<<"/10s)"<<endl<<flush;
                }
                processed_cmd = jobqueue_._processed_cmds;
            }
            last_time = time_now;
        }
#endif
        try
        {
            //uint64_t _start_time = splsTimeStamp();
            Job task_ = jobqueue_.getjob();
            //task_._start_get = _start_time;
#ifdef DEBUG_PER
            task_._get_time = splsTimeStamp();
#endif
            if (work(task_))
            {
#ifdef DEBUG_PER
                uint64_t time_cost = splsTimeStamp() - task_._get_time;
                if (time_cost > 5000)
                {
                    cout<<"!!!!!!!!!!!! cmd "<<task_.cmd<<" cost "<<time_cost<<" us"<<endl;
                    if (task_.cmd == "scheduleEvent")
                    {
                        json_spirit::mObject& obj  = task_.getRecvObj();
                        std::string event = "";
                        READ_STR_FROM_MOBJ(event,obj,"event");
                        cout<<"event="<<event<<endl;
                    }
                    else if (task_.cmd == "combatResult")
                    {
                        json_spirit::mObject& obj  = task_.getRecvObj();
                        uint64_t combat_id = 0;
                        int type = 0;
                        READ_INT_FROM_MOBJ(type, obj, "type");
                        READ_UINT64_FROM_MOBJ(combat_id, obj, "id");
                        cout<<"battle type "<<type<<",id "<<combat_id<<endl;
                    }
                }
#endif
                continue;
            }
            else
                break;
        }
        catch (std::exception& e)
        {
            std::cerr << "wrok loop Exception: " << e.what() << "\n";
            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);
            for (int i = 0; i < nSize; i++)
            {
                cout << symbols[i] << endl;
            }
            free(symbols);
            --_runing_loop;
            return;
        }
    } while (!exitthread);
    std::cout<< "**************** "<<worker_name<<" workloop break *****************" <<endl;
    --_runing_loop;
    return;
}

class ActionWorker : public worker_new<actionmessage>
{
public:
    ActionWorker(jobqueue<actionmessage>& _jobqueue, std::size_t _maxthreads = 1) :
      worker_new<actionmessage>("mainworker",_jobqueue, _maxthreads)
    {
        //����ӿ�
        m_cmds_process_map["roleLogin"] = ProcessCharLogin;
        m_cmds_process_map["logout"] = ProcessLogout;
        m_cmds_process_map["charlist"] = ProcessCharList;
        m_cmds_process_map["qcreate"] = ProcessQcreate;
        m_cmds_process_map["create"] = ProcessCreateChar;
#ifdef CAN_DELETE_CHARACTOR
        m_cmds_process_map["deleteRole"] = ProcessDeleteChar;
#endif
        m_cmds_process_map["undeleteRole"] = ProcessUnDeleteChar;
        m_cmds_process_map["chardata"] = ProcessQueryChar;
        m_cmds_process_map["qdate"] = ProcessQueryDate;
        m_cmds_process_map["panel"] = ProcessQueryPanel;
        m_cmds_process_map["attack"] = ProcessAttackStronghold;
        m_cmds_process_map["checkName"] = ProcessCheckName;
        m_cmds_process_map["getRandomName"] = ProcessGetRandomName;

        //���ӽ������
        m_cmds_process_map["getGeneralList"] = ProcessCharGenerals;
        m_cmds_process_map["dealGeneral"] = ProcessGeneralChange;
        m_cmds_process_map["getFormationList"] = ProcessCharZhen;
        m_cmds_process_map["getRecycleGeneralList"] = ProcessCharFiredGenerals;
        m_cmds_process_map["getGeneral"] = ProcessGeneral;
        m_cmds_process_map["getFormation"] = ProcessZhenGenerals;
        m_cmds_process_map["setFormation"] = ProcessZhenGeneralChange;
        m_cmds_process_map["getChat"] = ProcessCharChat;
        m_cmds_process_map["setChat"] = ProcessCharChatChange;

        //ս����ѯ
        m_cmds_process_map["getAttackPower"] = ProcessGetAttackPower;

        //�ؿ��������
        m_cmds_process_map["getAreaStageList"] = ProcessCharMapTempo;
        m_cmds_process_map["getStageInfo"] = ProcessCharStage;
        m_cmds_process_map["getStageGateList"] = ProcessCharStageTempo;
        m_cmds_process_map["getGateInfo"] = ProcessCharStronghold;
        //m_cmds_process_map["buyElite"] = ProcessBuyStrongholdTimes;
        m_cmds_process_map["getGateDropInfo"] = ProcessGetDropInfo;
        m_cmds_process_map["getRaiders"] = ProcessStrongholdRaiders;
        m_cmds_process_map["getStageFinishLoot"] = ProcessGetStageFinishLoot;
        m_cmds_process_map["checkStageFinish"] = ProcessCheckStageFinish;

        //��ɫ��Ϣ
        m_cmds_process_map["getRoleInfo"] = ProcessQueryCharData;
        m_cmds_process_map["switchArea"] = ProcessMoveNextMap;

        //��ѯ��ɫ��ϸ��Ϣ
        m_cmds_process_map["getRoleDetail"] = ProcessGetRoleDetail;

        //��ѯ��ɫ����
        //m_cmds_process_map["getRoleExp"] = ProcessQueryCharExp;

        //��ɫ״̬
        //m_cmds_process_map["getStarList"] = ProcessGetStarInfo;
        //m_cmds_process_map["getOpenStateList"] = ProcessGetOpenStates;
        //�µ�״̬ ������������
        //m_cmds_process_map["upgradeStar"] = ProcessLevelupStar;

        //��ְ ��Ϣ
        m_cmds_process_map["getPostInfo"] = ProcessGetOffical;
        m_cmds_process_map["getPostTecList"] = ProcessGetOfficalTec;
        m_cmds_process_map["getPostHeroList"] = ProcessGetOfficalGenaral;
        m_cmds_process_map["joinCamps"] = ProcessJoinCamps;
        //m_cmds_process_map["getWeakCamps"] = ProcessgetWeakCamps;
        //m_cmds_process_map["revoltCamps"] = ProcessRevoltCamps;
        m_cmds_process_map["levelUpPost"] = ProcessLevelUpOffical;

#if 0
        //ұ�����
        m_cmds_process_map["getSmeltTaskList"] = ProcessGetSmeltTask;
        m_cmds_process_map["getSmeltQueueList"] = ProcessGetSmeltQueue;
        m_cmds_process_map["getSmeltTask"] = ProcessStartSmelt;
#endif

        //̽�����
        m_cmds_process_map["getExploreList"] = ProcessExploreInterface;
        m_cmds_process_map["explore"] = ProcessExplore;
        m_cmds_process_map["getComboTips"] = ProcessComboTips;
        m_cmds_process_map["checkExploreState"] = ProcessCheckExploreState;
        //m_cmds_process_map["buyExplore"] = ProcessBuyExplore;    //����̽������

        //�������
        m_cmds_process_map["getFarmList"] = ProcessFarmList;
        m_cmds_process_map["getFarmTask"] = ProcessStartFarm;
        m_cmds_process_map["delayFarmTask"] = ProcessDelayFarm;
        m_cmds_process_map["setFarmed"] = ProcessSetFarmed;
        m_cmds_process_map["speedUpHarvest"] = ProcessSpeedUpHarvest;
        m_cmds_process_map["upFarmField"] = ProcessUpFarmField;
        m_cmds_process_map["waterFarmField"] = ProcessWaterFarmField;
        m_cmds_process_map["waterFriendField"] = ProcessWaterFriendField;
        m_cmds_process_map["getWaterFriendList"] = ProcessGetWaterFriendList;
        m_cmds_process_map["rewardNourish"] = ProcessRewardNourish;


        //�������
        m_cmds_process_map["getWeaponList"] = ProcessListNewWeapons;
        m_cmds_process_map["upgradeWeapon"] = ProcessUpgradeWeapon;
        m_cmds_process_map["getWeaponTip"] = ProcessQueryWeaponMemo;
        m_cmds_process_map["getWeaponUpgradeInfo"] = ProcessQueryWeaponUpgradeInfo;

        //װ�����
        m_cmds_process_map["getRoleEquipList"] = ProcessGetEquipList;    //���ϵ�װ��
         m_cmds_process_map["getEquipInfo"] = ProcessGetEquipInfo;        //��������װ����Ϣ
         m_cmds_process_map["getSysEquipInfo"] = ProcessGetSysEquipInfo;    //�����װ����Ϣ
         m_cmds_process_map["getGemInfo"] = ProcessGetGemInfo;            //������������Ϣ
         m_cmds_process_map["getEnhanceInfo"] = ProcessEnhanceEquipInfo;    //װ��ǿ����Ϣ
         m_cmds_process_map["getEnhanceList"] = ProcessEnhanceEquiplist;    //װ��ǿ���б�
         m_cmds_process_map["enhanceEquip"] = ProcessEnhanceEquip;        //װ��ǿ��
         m_cmds_process_map["getLibaoInfo"] = ProcessGetLibaoInfo;            //�������������Ϣ

         m_cmds_process_map["getStoreList"] = ProcessShowBackpack;//��ʾ����
         m_cmds_process_map["upEquip"] = ProcessEquip;    //װ��
         m_cmds_process_map["onekeyUpEquip"] = ProcessOnekeyEquip;    //װ��
         m_cmds_process_map["downEquip"] = ProcessUnequip;//ж��װ��

        //ѵ�����
        m_cmds_process_map["getTrainPlaceList"] = ProcessGetTrainList;
        m_cmds_process_map["getBookList"] = ProcessGetBookList;
        m_cmds_process_map["trainGeneral"] = ProcessTrain;
        m_cmds_process_map["diamondTrain"] = ProcessUpgradeGeneralTrainQue;

        //�������
        m_cmds_process_map["buyInherit"] = ProcessBuyInherit;
        m_cmds_process_map["inheritGeneral"] = ProcessInherit;
        m_cmds_process_map["inheritInfo"] = ProcessInheritInfo;

        //�̵����
        m_cmds_process_map["getShopList"] = ProcessGetShopList;
        m_cmds_process_map["getShopEventList"] = ProcessGetShopEventList;

        //ϴ��
        m_cmds_process_map["getWashInfo"] = ProcessGetWashInfo;
        m_cmds_process_map["dealWash"] = ProcessWash;

        //������Ʒ
        m_cmds_process_map["sell"] = ProcessSellItem;
        //�ع�
        m_cmds_process_map["recycle"] = ProcessBuyback;
        //����װ��
        m_cmds_process_map["saleEquip"] = ProcessSellEquip;
        //�ع�װ��
        m_cmds_process_map["recycleEquip"] = ProcessBuybackEquip;
        //����װ���ع��б�
        m_cmds_process_map["getRecycleList"] = ProcessSelledEquiplist;

         //�������
         //��ü����б�
        m_cmds_process_map["getSkillList"] = ProcessGetSkillList;
        //��ü��������б�
        m_cmds_process_map["getSkillResearchList"] = ProcessGetSkillResearchList;
        //��õ���������Ϣ
        m_cmds_process_map["getSkillInfo"] = ProcessGetSkillInfo;
        //��ü���������Ϣ�б�
        m_cmds_process_map["getSkillResearchInfoList"] = ProcessGetSkillResearchInfo;
        //�����о�
        m_cmds_process_map["researchSkill"] = ProcessResearchSkill;
        //ˢ�¼����о���
        m_cmds_process_map["updateSkill"] = ProcessUpdateTeachers;
        //����������λ��
        m_cmds_process_map["buySkillResearchPos"] = ProcessBuySkillResearchQue;
        //ֹͣ�о�
        m_cmds_process_map["stopResearchSkill"] = ProcessStopResearch;

        //���ټ����о�
        m_cmds_process_map["fasterResearch"] = ProcessFastResearch;
        //��ѯ���ټ����о�����Ҫ���
        m_cmds_process_map["getFasterGold"] = ProcessQueryFastResearch;
        //�������е�����
        m_cmds_process_map["diamondResearch"] = ProcessUpgradeResearchQue;

         //����
         m_cmds_process_map["chat"] = ProcessChat;//����
        //��¼
         m_cmds_process_map["login"] = ProcessLogin;

        //ս���ط�
        m_cmds_process_map["getBattleList"] = ProcessGetBattltRecord;

        //��Ϣ�ӿ�
         m_cmds_process_map["getRestInfo"] = ProcessQueryRestInfo;
         m_cmds_process_map["dealRest"] = ProcessRest;

        //�ż��ӿ�
        m_cmds_process_map["getMailList"] = ProcessGetMailList;
        m_cmds_process_map["queryMail"] = ProcessQueryMailContent;
        m_cmds_process_map["setMailAction"] = ProcessSetMail;
        m_cmds_process_map["delAllMail"] = ProcessDeleteMails;
        m_cmds_process_map["sendMail"] = ProcessSendMail;
        m_cmds_process_map["getUnread"] = ProcessGetUnread;
        m_cmds_process_map["getMailAttach"] = ProcessGetMailAttach;

        //���սӿ�
        m_cmds_process_map["getLevyInfo"] = ProcessGetLevyInfo;
        m_cmds_process_map["dealLevy"] = ProcessDealLevy;

        //��Ӣս�۽ӿ�
        m_cmds_process_map["getEliteList"] = ProcessGetEliteCombatList;
        m_cmds_process_map["attackElite"] = ProcessAttackEliteCombat;
        m_cmds_process_map["resetElite"] = ProcessResetEliteCombat;
        //��ʾ��ҵ�����Ӣ�ؿ���Ϣ
        m_cmds_process_map["getEliteInfo"] = ProcessEliteStronghold;
        //��ʾ�ؿ�����
        m_cmds_process_map["getEliteRaiders"] = ProcessEliteRaiders;

        /**************ͳһ�ӿ�****************/
        m_cmds_process_map["dealRefresh"] = ProcessRefreshXXX;//ˢ��
        m_cmds_process_map["dealGetAward"] = ProcessGetXXX;//��ȡ
        m_cmds_process_map["dealBuy"] = ProcessBuyXXX;//����
        m_cmds_process_map["dealStop"] = ProcessStopXXX;//ֹͣ
        m_cmds_process_map["dealCoolTime"] = ProcessSpeedXXX;//����
        m_cmds_process_map["getOpenInfo"] = ProcessGetOpenInfo;//��ȡ���ܿ���
        m_cmds_process_map["getConfirm"] = ProcessGetConfirmInfo;//��ȡ�������ȷ��״̬
        m_cmds_process_map["enableConfirm"] = ProcessSetConfirmInfo;//���ý�������Ƿ���Ҫ��ʾ
        m_cmds_process_map["getUpdateList"] = ProcessUpdateList;//��ȡ��������ȴ���

        //��ɫ����
        m_cmds_process_map["getRankList"] = ProcessGetRanklist;
        //Ӣ������
        m_cmds_process_map["getHeroRankList"] = ProcessGetHeroRanklist;
        //÷������������
        m_cmds_process_map["getScoreRankList"] = ProcessGetLotteryRanklist;
        //boss�˺�����
        m_cmds_process_map["getBossRankList"] = ProcessGetBossRankList;
        //��Ӫս��ɱ����
        m_cmds_process_map["getCampRaceRankList"] = ProcessGetCampRaceRankList;
        //��Ӣս��������
        m_cmds_process_map["getEliteRankList"] = ProcessGetEliteRanklist;
        //�����������
        m_cmds_process_map["getPrestigeRankList"] = ProcessGetPrestigeRanklist;
        //ս������
        m_cmds_process_map["getAttackRankList"] = ProcessGetAttackRanklist;
        //ս��̨����
        m_cmds_process_map["getZSTRankList"] = ProcessGetZSTRanklist;

        //���Գ�ֵ�ӿ�
        m_cmds_process_map["testRecharge"] = ProcessTestRecharge;
        //��ѯ��ֵ�ӿ�
        m_cmds_process_map["getRecharge"] = ProcessQueryRecharge;

//#ifdef TEST_SERVER
        m_cmds_process_map["GmQueryStrongholds"] = ProcessGMQueryStrongholds;
        m_cmds_process_map["GmQueryStrongholdInfo"] = ProcessGMQueryStrongholdInfo;
        m_cmds_process_map["GmSaveStrongholdInfo"] = ProcessGMSaveStrongholdInfo;
//#endif

#ifndef ONE_CHARACTOR
        //��ѯ�ʺų�ֵ����
        m_cmds_process_map["getAccountRecharge"] = ProcessQueryAccountPoints;
        //�һ����
        m_cmds_process_map["exchangeGold"] = ProcessExchangeAccountScore;
#endif

        /*************** ������ؽӿ� *****************/
        m_cmds_process_map["getTroopsInfo"] = ProcessGetTroopsInfo;
        m_cmds_process_map["getTroopsMemberList"] = ProcessGetTroopsMemberList;
        m_cmds_process_map["getTroopsEventList"] = ProcessGetTroopsEventList;
        m_cmds_process_map["getTroopsApplyList"] = ProcessGetTroopsApplyList;
        m_cmds_process_map["dealTroopsApply"] = ProcessDealTroopsApply;
        m_cmds_process_map["getTrMemberList"] = ProcessGetTrMemberList;
        m_cmds_process_map["dealTroopsLeader"] = ProcessDealTroopsLeader;
        m_cmds_process_map["dimissTroops"] = ProcessDimissTroops;
        m_cmds_process_map["setTroopsInfo"] = ProcessSetTroopsInfo;
        m_cmds_process_map["quitTroops"] = ProcessQuitTroops;
        m_cmds_process_map["changeTroopsLeader"] = ProcessChangeTroopsLeader;
        m_cmds_process_map["recruitTroopsMember"] = ProcessRecruitTroopsMember;

        m_cmds_process_map["getTroopsRankList"] = ProcessGetTroopsRankList;
        m_cmds_process_map["createTroops"] = ProcessCreateTroops;
        m_cmds_process_map["searchTroops"] = ProcessSearchTroops;
        m_cmds_process_map["dealjoinTroops"] = ProcessDealjoinTroops;
        m_cmds_process_map["dealTroopsMember"] = ProcessDealTroopsMember;
        m_cmds_process_map["getTroopsRoleInfo"] = ProcessGetTroopsRoleInfo;
        m_cmds_process_map["getTroopsActionList"] = ProcessGetTroopsActionList;
        m_cmds_process_map["donateGold"] = ProcessDonateGold;
        m_cmds_process_map["getTroopsFeteList"] = ProcessGetFeteList;
        m_cmds_process_map["troopsFete"] = ProcessFete;
        m_cmds_process_map["getPartyList"] = ProcessQueryYanhui;
        m_cmds_process_map["joinParty"] = ProcessJoinYanhui;
        m_cmds_process_map["callMember"] = ProcessYanhuiZhaoji;
        m_cmds_process_map["inviteSomeone"] = ProcessInviteSomeone;

        /*************** �������ӿ� *******************/
        m_cmds_process_map["getRaceRankList"] = ProcessGetRaceRankList;    //��þ��������б�
        m_cmds_process_map["getRaceList"] = ProcessGetRaceInfo;    //��ѯ�Լ��ľ�����Ϣ
        m_cmds_process_map["race"] = ProcessChallenge;            //������ս
        m_cmds_process_map["buyRace"] = ProcessBuyChallenge;        //������ս����
        m_cmds_process_map["getRaceGoodsList"] = ProcessGetRaceGoodsList;        //����������Ʒ
        m_cmds_process_map["buyRaceGoods"] = ProcessBuyRaceGoods;        //���򾺼�������Ʒ

        //��ѯ������������
        m_cmds_process_map["QueryRaceRankRewards"] = ProcessQueryRankRewards;

        /*************** ����ӿ� **********************/
        m_cmds_process_map["getTaskList"] = ProcessTaskList;        //�����б�
        m_cmds_process_map["getTaskInfo"] = ProcessTaskInfo;        //��������
        m_cmds_process_map["getCurTask"] = ProcessTaskInfo;        //��ǰ����id=0
        m_cmds_process_map["getTaskAward"] = ProcessGetXXX;        //��ȡ������
        m_cmds_process_map["setTaskFinish"] = ProcessSetTaskDone;    //�ͻ��������������

        /*************** ���������ӿ� **********************/
        m_cmds_process_map["queryGuideIsComplete"] = ProcessQueryGuideState;    //��������״̬
        m_cmds_process_map["setGuideComplete"] = ProcessSetGuideState;        //������������״̬
        m_cmds_process_map["currentGuide"] = ProcessQueryCurGuide;                    //��ѯ��ǰ����id

        /*************** BOSSս�ӿ� ***********************/
        m_cmds_process_map["enterBossScene"] = ProcessEnterBossScene;    //����bossս����
        m_cmds_process_map["getBossInfo"] = ProcessGetBossInfo;        //��ȡBoss�̶���Ϣ
        m_cmds_process_map["getBossHp"] = ProcessGetBossHp;            //��ȡBoss��ǰѪ��
        m_cmds_process_map["getBossPerson"] = ProcessGetBossPerson;    //��ȡ����Boss��ǰ����
        m_cmds_process_map["upBossDamage"] = ProcessInspire;            //����
        m_cmds_process_map["getBossCoolTime"] = ProcessGetCoolTime;    //��ȡ����Boss��ȴʱ��
        m_cmds_process_map["endCoolTime"] = ProcessEndCoolTime;        //��������Boss��ȴʱ��
        m_cmds_process_map["getBossDamage"] = ProcessGetBossDamage;    //��ȡ��Boss����˺�������б�
        m_cmds_process_map["attackBoss"] = ProcessAttackBoss;            //����boss
        m_cmds_process_map["getActionMemo"] = ProcessGetActionMemo;    //��û����tips

        //���þ���boss����ʱ��
        m_cmds_process_map["setJtBossTime"] = ProcessSetJtBossTime;
        //��ѯ����boss����ʱ��
        m_cmds_process_map["queryJtBossTime"] = ProcessQueryJtBossTime;

        m_internal_cmds_process_map["checkJtBoss"] = ProcessCheckJtBossOpen;

        /*************** �����ٽӿ� ***********************/
        m_cmds_process_map["enterGuardScene"] = ProcessEnterGuardScene;    //���뻤�ͳ���
        m_cmds_process_map["getRobGoodsEvent"] = ProcessGetRobGoodsEvent;    //��ȡ���͹�����Ϣ
        m_cmds_process_map["getGuardGoodsList"] = ProcessGetGuardGoodsList;    //��ȡ��ɫ�ɻ��͵ĸ�
        m_cmds_process_map["getGuardInfo"] = ProcessGetGuardInfo;    //��ȡ��ɫ��ǰ������Ϣ
        m_cmds_process_map["dealGoods"] = ProcessDealGoods;    //��ʼ���ͺ�ˢ�¸�
        m_cmds_process_map["getGuardTaskList"] = ProcessGetGuardTaskList;    //��ȡ�����еĸٶ���
        m_cmds_process_map["getGuardTaskInfo"] = ProcessGetGuardTaskInfo;    //��ȡĳ���ٵ���Ϣ
        m_cmds_process_map["attackGuard"] = ProcessattackGuard;    //�ٸ�
        m_cmds_process_map["quitGuard"] = ProcessQuitGuard;    //�뿪���ٽ���
        m_cmds_process_map["inspireGuard"] = ProcessInspireGuard;    //���ٹ���
        m_cmds_process_map["getGuardFriends"] = ProcessGetGuardFriendsList;    //��ȡ���ͺ����б�
        m_cmds_process_map["applyGuardHelp"] = ProcessApplyGuardHelp;    //����ĳ���ѻ���
        m_cmds_process_map["applyGuardHelpCancel"] = ProcessApplyGuardCancel;    //ȡ����������
        m_cmds_process_map["answerGuardHelp"] = ProcessAnswerGuardHelp;    //��Ӧĳ���ѻ�������
        m_cmds_process_map["getGuardRobScore"] = ProcessGetGuardRobScore;    //��ȡ��ȡ���ְ�
        m_cmds_process_map["getGuardRobRewardsList"] = ProcessGetGuardRobRewardsList;    //��ȡ��ȡ�����
        m_cmds_process_map["getGuardRobRewards"] = ProcessGetGuardRobRewards;    //��ȡ�����

        /*************** ɨ����Ӣ�ӿ� ***********************/
        m_cmds_process_map["getSweepList"] = ProcessGetSweepList;    //��ȡ������Ӣ��Ϣ
        m_cmds_process_map["addSweepTask"] = ProcessAddSweepTask;    //���ɨ����Ӣѡ��
        m_cmds_process_map["getSweepInfo"] = ProcessGetSweepInfo;    //��ȡɨ����Ϣ
        m_cmds_process_map["getSweepResult"] = ProcessGetSweepResult;    //��ȡɨ�����
        m_cmds_process_map["dealSweep"] = ProcessDealSweep;    //ɨ������

        /*************** �������� ***********************/
        m_cmds_process_map["getTacticInfo"] = ProcessGetTacticInfo;    //��ȡ����������Ϣ
        m_cmds_process_map["getHelpList"] = ProcessGetHelpList;    //��ȡ��Ϸ����
        m_cmds_process_map["findBackHelp"] = ProcessFindBackHelp;    //��Ϸ�����һش���
        m_cmds_process_map["getLogOutList"] = ProcessGetLogOutList;    //��ȡ����δ�����ʾ
        m_cmds_process_map["getSupplyHelpList"] = ProcessGetSupplyHelpList;    //��ȡ��������

        /*************** ս��ӿ� ***********************/
        m_cmds_process_map["getHorseInfo"] = ProcessGetHorseInfo;    //��ȡս����Ϣ
        m_cmds_process_map["trainHorse"] = ProcessTrainHorse;    //ս������
        m_cmds_process_map["turnHorse"] = ProcessTurnHorse;    //ս��ת��

        /*************** �Ҷ��ӿ� ***********************/
        m_cmds_process_map["getServantInfo"] = ProcessGetServantInfo;    //��ȡ�Ҷ���Ϣ
        m_cmds_process_map["getServantEventsList"] = ProcessGetServantEventsList;    //��ȡץ����־��Ϣ
        m_cmds_process_map["getServantList"] = ProcessGetServantList;    //��ȡ�Ҷ��б�
        m_cmds_process_map["getInteractionList"] = ProcessGetInteractionList;    //��ȡ�Ҷ��б�
        m_cmds_process_map["dealServantAction"] = ProcessDealServantAction;    //�Ҷ�ϵͳ����

        /*************** �츳�ӿ� ***********************/
        m_cmds_process_map["getGeniusUpgradeList"] = ProcessGetGeniusUpgradeList;    //��ȡ�츳�����б�
        m_cmds_process_map["openGenius"] = ProcessOpenGenius;    //�����츳
        m_cmds_process_map["cleanGenius"] = ProcessCleanGenius;    //ϴ�츳
        m_cmds_process_map["graftGenius"] = ProcessGraftGenius;    //��ֲ�츳

        m_cmds_process_map["queryOlGift"] = ProcessQueryOnlineGift;    //��ѯ�������
        m_cmds_process_map["openOlGift"] = ProcessGetOnlineGift;        //��ȡ�������
        m_cmds_process_map["queryCollectGift"] = ProcessQueryCollectGift;    //��ѯ�ղ����
        m_cmds_process_map["getCollectGift"] = ProcessGetCollectGift;    //��ȡ�ղ����

        /*************** ��Ӫս�ӿ� *************************/

        //cmd:getGroupTime//�����Ӫս����ʱ��
        m_cmds_process_map["getGroupTime"] = ProcessGetGroupTime;
        //cmd:getGroupWins//�����Ӫս��ʤ��
        m_cmds_process_map["getGroupWins"] = ProcessGetGroupWins;
        //cmd:getGroupMark//�����Ӫս˫������
        m_cmds_process_map["getGroupMark"] = ProcessGetGroupMark;
        //cmd:getGroupInspire//��ù������ӵ�ս�����ٷ���
        m_cmds_process_map["getGroupInspire"] = ProcessGetGroupInspire;
        //cmd:getGroupList//�����Ӫս����б�
        m_cmds_process_map["getGroupList"] = ProcessGetGroupList;
        //cmd:getGroupReportList//���ս���б�
        m_cmds_process_map["getGroupReportList"] = ProcessGetGroupReportList;
        //cmd:inspireGroup//����ս����
        m_cmds_process_map["inspireGroup"] = ProcessInspireGroup;
        //������Ӫս cmd: enterGroupBattle
        m_cmds_process_map["enterGroupBattle"] = ProcessJoinCampRace;
        //cmd:startGroupBattle//��ս
        m_cmds_process_map["startGroupBattle"] = ProcessStartGroupBattle;
        //cmd:stopGroupBattle//������ս
        m_cmds_process_map["stopGroupBattle"] = ProcessStopGroupBattle;
        //cmd:getGroupBattleInfo//�����Ӫս�����Ϣ
        m_cmds_process_map["getGroupBattleInfo"] = ProcessGetGroupBattleInfo;

        //�ٴλ��
        m_cmds_process_map["getAgain"] = ProcessGetStrongholdLootsAgain;        //�ٴλ��

        //����������ص������
        //m_cmds_process_map["setUserPoint"] = ProcessSetUserPoint;

        //����ӿ�
        m_cmds_process_map["getGiftList"] = ProcessListPacks;
        m_cmds_process_map["queryUnGetGifts"] = ProcessQueryUnGetGifts;

        //����
        m_cmds_process_map["getActionInfo"] = ProcessGetAction;                //��û������Ϣ
        m_cmds_process_map["getOpeningAction"] = ProcessGetOpeningAction;        //��ÿ����������Ϣ
        //m_cmds_process_map["getGiftAction"] = ProcessGetGiftAction;            //�������������Ϣ
        m_cmds_process_map["getDailyAction"] = ProcessGetDailyAction;            //����ճ��������Ϣ
        //m_cmds_process_map["getNormalActionOpen"] = ProcessGetNormalActionOpen;    //��ѯ������ͨ��Ŀ���״̬

        m_cmds_process_map["getSCgift"] = ProcessGetFirstRechargeGift;        //��ȡ�׳����
        m_cmds_process_map["querySCgift"] = ProcessQueryFirstRechargeGift;    //��ѯ�׳����

        m_cmds_process_map["getNoviceAction"] = ProcessGetNoviceAction;        //���ֳ��Žӿ�
        //m_cmds_process_map["getWelfareInfo"] = ProcessGetWelfareInfo;            //���߸���
        //m_cmds_process_map["getLoginPresent"] = ProcessGetLoginPresent;        //������¼����
        m_cmds_process_map["getVipActionList"] = ProcessGetVIPPresent;        //VIP����
        //m_cmds_process_map["getChargeActionList"] = ProcessGetChargePresent;    //��ֵ������
        m_cmds_process_map["getChargeActionList"] = ProcessQueryRechargeEvent;    //��ֵ������

        m_cmds_process_map["getHorseActionList"] = ProcessGetHorseActionList;//��ȡս�������Ϣ
        m_cmds_process_map["dealHorseFruit"] = ProcessDealHorseFruit;            //ս����Ӳ���

        //����ϴ��
        m_cmds_process_map["queryWashEvent"] = ProcessWashAction;

        //���˸����ӿ�
        //��ѯ�����б� cmd:getMapMultiBoss //���ݵ�ǰ���������ѯ����ID���б�
        m_cmds_process_map["getMapMultiBoss"] = ProcessGetGroupCopyList;
        //��ѯ���������б� cmd:getMultiBossList //��ö��˸����б�
        m_cmds_process_map["getMultiBossList"] = ProcessGetGroupCopyTeamList;
        //��ѯ���������Ա��Ϣ cmd:getMultiBossMembers //��ö�Ա��Ϣ
        m_cmds_process_map["getMultiBossMembers"] = ProcessGetGroupCopyTeamDetail;
        //cmd:getLeftAttackTime //��ѯʣ��ɹ�������
        m_cmds_process_map["getLeftAttackTime"] = ProcessGetGroupCopyLeftTimes;
        //cmd:getMultiBossInfo//��ö��˸�����Ϣ
        m_cmds_process_map["getMultiBossInfo"] = ProcessGetGroupCopyInfo;
        //cmd:getAutoAttackBoss //��ѯ�Ƿ������Զ�ս��    !!!!!!!!!!!!!!!����Ҫ�޸�!
        m_cmds_process_map["getAutoAttackBoss"] = ProcessGetGroupCopyAutoAttack;
        //cmd:changeMultiBoss //�л�����
        m_cmds_process_map["changeMultiBoss"] = ProcessEnterGroupCopy;
        //cmd:closeMultiBoss//�رո������棬����ע������Ϣ
        m_cmds_process_map["closeMultiBoss"] = ProcessLeaveGroupCopy;
        //cmd:createMultiBoss //�������˸�������
        m_cmds_process_map["createMultiBoss"] = ProcessCreateGroupCopyTeam;
        //cmd:leaveMultiBoss //�뿪���ɢ����
        m_cmds_process_map["leaveMultiBoss"] = ProcessLeaveGroupCopyTeam;
        //cmd:joinMultiBoss //�������
        m_cmds_process_map["setAutoJoinBoss"] = ProcessJoinGroupCopyTeam;
        //cmd:attackMultiBoss    //��������
        m_cmds_process_map["attackMultiBoss"] = ProcessAttackGroupCopy;
        //cmd:setAutoAttackBoss //���������Զ�ս��
        m_cmds_process_map["setAutoAttackBoss"] = ProcessSetAutoAttack;
        //cmd:changeMultiBossIndex //�ı��Ա��ţ����µ���һλ
        m_cmds_process_map["changeMultiBossIndex"] = ProcessChangeGroupCopyTeamMemberPos;
        //cmd:fireMultiBoss //�߳���Ա
        m_cmds_process_map["fireMultiBoss"] = ProcessFireGroupCopyMember;
        //�������
        m_cmds_process_map["callMemeberMultiBoss"] = ProcessGroupCopyZhaoji;
        //�������
        m_cmds_process_map["callFriendMultiBoss"] = ProcessGroupCopyZhaojiFriend;

        /***************** �Ƕ���P�ӿ� ***********************/
        //��ʯtips
        m_cmds_process_map["queryBaoshiTips"] = ProcessQueryBaoshiInfo;
        //��ʯ�}���б�
        m_cmds_process_map["queryBaoshiList"] = ProcessQueryBaoshiList;
        //���Ƕ�䌢�б�
        m_cmds_process_map["queryBaoshiGeneralList"] = ProcessQueryBaoshiGenerals;
        //�䌢��ʯ�б�
        m_cmds_process_map["queryGeneralBaoshiList"] = ProcessQueryGeneralBaoshi;
        //ȡ���䌢��ʯ
        m_cmds_process_map["removeBaoshi"] = ProcessRemoveBaoshi;
        //�Ƕ��ʯ
        m_cmds_process_map["xiangqian"] = ProcessXiangqianBaoshi;
        //�ϲ���ʯ
        m_cmds_process_map["combineBaoshi"] = ProcessCombineBaoshi;
        //һ���ϲ���ʯ
        m_cmds_process_map["combineAllBaoshi"] = ProcessCombineAllBaoshi;
        //��ʯת��
        m_cmds_process_map["changeBaoshi"] = ProcessChangeBaoshi;
        //�������ʯ
        m_cmds_process_map["getYushi"] = ProcessGetYushi;
        //����ʯ
        m_cmds_process_map["buyBaoshi"] = ProcessBuyShopBaoshi;
        //��ʯ�̵�
        m_cmds_process_map["getBaoshiShopInfo"] = ProcessBaoshiShop;
        //ϴ����ʯ
        //m_cmds_process_map["washBaoshi"] = ProcessWashBaoshi;
        //��ԃ��ʯϴ����Ϣ
        //m_cmds_process_map["queryBaoshiWashInfo"] = ProcessQueryBaoshiWashInfo;
        //����ʯ����
        //m_cmds_process_map["openBaoshiAttribute"] = ProcessOpenBaoshiAttribute;
        //������ʯ
        //m_cmds_process_map["levelupBaoshi"] = ProcessLevelupBaoshi;
        //�һ���ʯ
        m_cmds_process_map["convertBaoshi"] = ProcessExchangeBaoshi;
        //��Ҷһ���ʯ
        m_cmds_process_map["convertYushi"] = ProcessExchangeYushi;
        //��ѯ�һ�
        m_cmds_process_map["queryBaoshiExchange"] = ProcessQueryExchangeBaoshi;
        //��ѯ��ʯ������Ϣ
        //m_cmds_process_map["queryBaoshiLevelupInfo"] = ProcessQueryBaoshiLevelupInfo;
        //Ӣ����Ƕ��ʯ������
        m_cmds_process_map["queryGeneralBaoshiAttr"] = ProcessGeneralBaoshiInfo;

        //��ѯ��������
        m_cmds_process_map["queryTreasureCount"] = ProcessQueryTreasure;
        //��ѯ���߼۸�
        m_cmds_process_map["queryTreasurePrice"] = ProcessQueryTreasurePrice;

        /*************** �����нӿ� ***********************/
        m_cmds_process_map["getNewRankingsInfo"] = ProcessGetNewRankingsInfo;    //����������Ϣ
        m_cmds_process_map["getLastNewRankingsInfo"] = ProcessGetLastNewRankingsInfo;    //����������Ϣ
        m_cmds_process_map["getNewRankingsReward"] = ProcessGetNewRankingsReward;    //��ȡ���н���

        /*************** ÷�������齱�ӿ� ***********************************/
        //÷��������ȡ��Ʒ
        m_cmds_process_map["lottery"] = ProcessLottery;
        //��ѯ÷���������˼�¼
        m_cmds_process_map["queryLottery"] = ProcessQueryLotteryRecords;
        //��ѯ÷������ȫ������
        m_cmds_process_map["queryLotteryNotice"] = ProcessQueryLotteryNotice;
        //��ѯ÷����������
        m_cmds_process_map["queryLotteryScore"] = ProcessQueryScore;

        //�¿����ĳ齱�
        m_cmds_process_map["newLottery"] = ProcessEventLottery;
        m_cmds_process_map["newLotteryNotice"] = ProcessQueryLotteryEventNotice;
        m_cmds_process_map["newLotteryList"] = ProcessQueryLotteryEventAwards;

        /*******����������********/
        //���������̹���
        m_cmds_process_map["JtqueryLotteryNotice"] = ProcessQueryCorpsLotteryNotice;
        //��ѯ����÷������������Ʒ�б�
        m_cmds_process_map["JtqueryLotteryList"] = ProcessQueryCorpsLotteryAwards;
        //÷��������ȡ��Ʒ
        m_cmds_process_map["JtLottery"] = ProcessCorpsLottery;
        //�������
        m_cmds_process_map["BuyDaoju"] = ProcessBuyDaoju;

        /******************����̽�� *******************************/

        //����̽���б�
        m_cmds_process_map["JtExploreList"] = ProcessCorpsExplreList;

        //����̽����ȡ
        m_cmds_process_map["JtExploreAccept"] = ProcessCorpsExploreAccept;

        //����̽������
        m_cmds_process_map["JtExploreAbandon"] = ProcessCorpsExploreAbandon;

        //����̽��ˢ��
        m_cmds_process_map["JtExploreRefresh"] = ProcessCorpsExploreRefresh;

        //����̽�����
        m_cmds_process_map["JtExploreDone"] = ProcessCorpsExploreDone;

        //����̽����ʱ������
        m_internal_cmds_process_map["corpsExplore"] = ProcessCorpsExploreFinish;

        //ԯ����ꪼ���
        m_cmds_process_map["JtYmsjJoin"] = ProcessCorpsYmsjJoin;
        //ԯ�����ѡ��
        m_cmds_process_map["JtYmsjChoose"] = ProcessCorpsYmsjChoose;
        //ԯ�������Ϣ
        m_cmds_process_map["JtYmsjInfo"] = ProcessCorpsYmsjInfo;

        //ԯ�������ȡ����
        m_cmds_process_map["JtYmsjAward"] = ProcessCorpsYmsjAward;

        //ԯ�����ˢ��
        m_internal_cmds_process_map["YmsjRefresh"] = ProcessCorpsYmsjRefresh;

        //ԯ����ꪳ�ʱʤ��
        m_internal_cmds_process_map["YmsjWin"] = ProcessCorpsYmsjWin;
        //ԯ����ꪳ�ʱ�Զ�ѡ��
        m_internal_cmds_process_map["YmsjChoose"] = ProcessCorpsYmsjChoose;

        //�������˾���
        m_internal_cmds_process_map["checkRobotCorps"] = ProcessRobotCorps;

        /*************** �ճ�����ӿ� ***************************************/

        //�ճ���������ȡ
        m_cmds_process_map["getDailyTaskReward"] = ProcessRewardDailyTask;

        /**��ѯ������Ϣ**/
        m_cmds_process_map["DlInfo"] = ProcessQueryDLPlace;

        /*************************** �����Խӿ� *****************************/
        //��ȡ����ʱ��
        m_cmds_process_map["cmtime"] = ProcessGetChenmiTime;

        //��ѯ�ճ�����
        m_cmds_process_map["queryNotices"] = ProcessQueryAdminNotice;

        //������������
        m_cmds_process_map["setBabyState"] = ProcessEnableStandIn;

        //���򱳰�λ��
        m_cmds_process_map["buyBagSlot"] = ProcessBuyBagSlot;
        //��������λ
        m_cmds_process_map["swapBagSlot"] = ProcessSwapBagSlot;

        //�򿪱���λ��Ʒ
        m_cmds_process_map["openBagItem"] = ProcessOpenBagItem;
        //�򿪱���λ���
        m_cmds_process_map["openBagLibao"] = ProcessOpenBagLibao;
        //�򿪳ɳ����
        m_cmds_process_map["getChengzhangLibaoInfo"] = ProcessGetChengzhangLibaoInfo;
        //����װ��
        m_cmds_process_map["makeEquip"] = ProcessMakeEquipment;
        //��ѯװ��������Ϣ
        m_cmds_process_map["getMakeInfo"] = ProcessGetEquipmentMakeInfo;
        //��ѯ�Ƽ�װ��
        m_cmds_process_map["getBestEquip"] = ProcessGetBestEquipment;
        //������
        m_cmds_process_map["sortBag"] = ProcessSortBbag;
        //ʹ����Ʒ
        m_cmds_process_map["useItem"] = ProcessUseItem;

        //��λװ��
        m_cmds_process_map["locateEquip"] = ProcessLocateEquip;

        /*************************** GM�ύ����ӿ� *****************************/
        m_cmds_process_map["getGMqlist"] = ProcessGetGMqlist;
        m_cmds_process_map["subGMquestion"] = ProcessSubGMquestion;

        //��ҿ�����ʱVIP
        //m_cmds_process_map["startTmpVIP"] = ProcessStartTmpVIP;
        //m_cmds_process_map["getTmpVIPTime"] = ProcessGetTmpVIPTime;

        /*************** ����ϵͳ ***********************/
        m_cmds_process_map["getFriendsList"] = ProcessGetFriendsList;//�����б�
        m_cmds_process_map["dealFriends"] = ProcessDealFriends;//���Ѳ���
        m_cmds_process_map["congratulation"] = ProcessCongratulation;    //ף�غ���
        m_cmds_process_map["getContratulationList"] = ProcessGetCongratulations;    //ף���б�
        m_cmds_process_map["getRecvedContratulationList"] = ProcessGetRecvedCongratulations;    //ף���б�
        //m_cmds_process_map["getFriendInfoList"] = ProcessGetFriendInfoList;    //���Ѷ�̬
        m_cmds_process_map["getEnemyList"] = ProcessGetEnemyList; //����б�
        m_cmds_process_map["getEnemyInfoList"] = ProcessGetEnemyInfoList;    //��ж�̬
        m_cmds_process_map["removeEnemy"] = ProcessRemoveEnemy; //�Ƴ����
        m_cmds_process_map["getRecommendFriends"] = ProcessGetRecommendFriends; //�Ƽ�����

        /*****************��ó�� **************************/

        //��ѯó�����    cmd��  getTradeCombos
        m_cmds_process_map["getTradeCombos"] = ProcessGetTradeCombos;
        //��ѯ��ǰ�̶����    cmd��getMyTraderList
        m_cmds_process_map["getMyTraderList"] = ProcessGetMyTraderList;
        //��������    cmd:    abandonTrader, pos:λ�ã�int��
        m_cmds_process_map["abandonTrader"] = ProcessAbandonTrader;
        //��ѯ�̶�    cmd:    getTraderList
        m_cmds_process_map["getTraderList"] = ProcessGetTraderList;
        //��ȡ���ˣ�    cmd��    selectTrader    ,id:����id��int��
        m_cmds_process_map["selectTrader"] = ProcessSelectTrader;
        //��ѯó����Ϣ��    cmd��getTradeInfo
        m_cmds_process_map["getTradeInfo"] = ProcessGetTradeInfo;
        //��ʼó�ף�    cmd��startTrade
        m_cmds_process_map["startTrade"] = ProcessStartTrade;
        //ó��wjbs    cmd��tradeWjbs
        m_cmds_process_map["tradeWjbs"] = ProcessTradeWjbs;
        //���ó�ף�������    cmd: finishTrade
        m_cmds_process_map["finishTrade"] = ProcessFinishTrade;

        /*****************Ǯׯ **************************/
        //Ͷ��
        m_cmds_process_map["buyBankCase"] = ProcessBuyBankCase;
        //����
        m_cmds_process_map["getBankFeedback"] = ProcessGetBankFeedback;
        //��ȡ��Ŀ��Ϣ
        m_cmds_process_map["getCaseInfo"] = ProcessGetCaseInfo;
        //��ȡ��Ŀ�б�
        m_cmds_process_map["getBankList"] = ProcessGetBankList;
        //Ͷ�ʿ���ȡ
        m_internal_cmds_process_map["bankCanGet"] = ProcessBankCaseCanGet;

        /******************�Թ�**************************/
        //��ѯ�Թ��б� cmd ��queryMazeList
        m_cmds_process_map["queryMazeList"] = ProcessQueryMazeList;
        //�Թ��Ѷ���Ϣ cmd��queryMazeDetail, id:�Թ�id
        m_cmds_process_map["queryMazeDetail"] = ProcessQueryMazeDetail;
        //�����Թ� cmd��enterMaze��id���Թ�id��star���Ǽ�
        m_cmds_process_map["enterMaze"] = ProcessEnterMaze;
        //��ѯ��ǰ�Թ� cmd��queryCurMaze
        m_cmds_process_map["queryCurMaze"] = ProcessQueryCurMaze;
        //��ѯ�Թ���ͼ��Ϣ cmd��queryMazeMap
        m_cmds_process_map["queryMazeMap"] = ProcessQueryMazeMap;
        //��ѯ�Թ��¼���Ϣ cmd��queryMazeEventTips,type:���
        m_cmds_process_map["queryMazeEventTips"] = ProcessQueryMazeEventTips;
        //��ѯ�Թ�״̬ cmd��queryCurMazeInfo
        m_cmds_process_map["queryCurMazeInfo"] = ProcessQueryCurMazeInfo;
        //��ѯ�Թ�������� cmd��queryMazeTeam
        m_cmds_process_map["queryMazeTeam"] = ProcessQueryMazeTeam;
        //�Թ��ƶ� cmd��mazeMove��id��Ŀ��λ��id
        m_cmds_process_map["mazeMove"] = ProcessMazeMove;
        //�Թ��ָ� cmd��mazeFull
        m_cmds_process_map["mazeFull"] = ProcessMazeFull;
        //�Թ����� cmd: mazeSkip, id:Ŀ��id
        m_cmds_process_map["mazeSkip"] = ProcessMazeSkip;
        //�Թ���� cmd��mazeChange, id:Ŀ��id
        m_cmds_process_map["mazeChange"] = ProcessMazeChange;
        //�Թ����� cmd��mazeReset
        m_cmds_process_map["mazeReset"] = ProcessMazeReset;
        //�Թ�boss�б� cmd��mazeBossList
        m_cmds_process_map["mazeBossList"] = ProcessMazeBossList;
        //�����Թ�boss cmd��mazeKillBoss
        m_cmds_process_map["mazeKillBoss"] = ProcessMazeKillBoss;
        //�Թ������� cmd��mazeGuessNumber
        m_cmds_process_map["mazeGuessNumber"] = ProcessMazeGuessNumber;
        //�Թ���ȭ cmd��mazeMora
        m_cmds_process_map["mazeMora"] = ProcessMazeMora;
        //�Թ�����˦����Ʒ cmd��mazeBuy
        m_cmds_process_map["mazeBuy"] = ProcessMazeBuy;
        //�Թ��齱 cmd��mazeLottery
        m_cmds_process_map["mazeLottery"] = ProcessMazeLottery;
        //�Թ���ѯ�齱��Ʒ cmd : mazeQueryLottery
        m_cmds_process_map["mazeQueryLottery"] = ProcessQueryMazeLottery;
        //�Թ���ѯ�����ִ���
        m_cmds_process_map["mazeQueryGuessTimes"] = ProcessQueryMazeGuessTimes;
        //�Թ���ѯ˦����Ʒ��Ϣ
        m_cmds_process_map["mazeQueryCanBuy"] = ProcessQueryMazeCanBuy;
        //�Թ�����
        m_cmds_process_map["mazeAbandon"] = ProcessMazeAbandon;
        //�Թ���ѯ��ǰ�¼����
        m_cmds_process_map["mazeCurrentResult"] = ProcessMazeQueryCurResult;
        //�Թ���ѯBOSS����
        m_cmds_process_map["mazeQueryBossLoots"] = ProcessMazeQueryBossLoots;

        //�����Թ��� cmd��mazeKill
        m_cmds_process_map["mazeKill"] = ProcessMazeKill;

        //�Թ���ʱ��
        m_internal_cmds_process_map["mazeTimeout"] = ProcessMazeTimeout;

        /*************** ����ϵͳ ***********************/
        m_cmds_process_map["querySoulList"] = ProcessQuerySoulList;//��ȡ��������
        m_cmds_process_map["upgradeSoul"] = ProcessUpgradeSoul;//������������
        m_cmds_process_map["getSoulsCostInfo"] = ProcessGetSoulsCostInfo;
        m_cmds_process_map["buySoulsDaoju"] = ProcessBuySoulsDaoju;

        /*************** ��ͼϵͳ ***********************/
        m_cmds_process_map["getNearPlayerList"] = ProcessGetNearPlayerList;//��ȡ�������

        //QQ����ϵͳ
#ifdef QQ_PLAT
        //��ѯ������棺cmd:getYellowEvent
        m_cmds_process_map["getYellowEvent"] = ProcessQueryQQYellowEvent;
        //��ѯ����������� cmd��queryQQnewbieLibao
        m_cmds_process_map["queryQQnewbieLibao"] = ProcessQueryQQNewbieLibao;
        //��ѯ����ÿ����� cmd��queryQQDailyLibao
        m_cmds_process_map["queryQQDailyLibao"] = ProcessQueryQQDailyLibao;
        //��ѯ����ɳ���� cmd��queryQQLevelLibao
        m_cmds_process_map["queryQQLevelLibao"] = ProcessQueryQQLevelLibao;
        //��Ѷ����ͳ��
        m_cmds_process_map["act_tencent"] = ProcessActTencent;
#else
        //��ѯVIPÿ����� cmd��queryVipDailyLibao
        m_cmds_process_map["queryVipDailyLibao"] = ProcessQueryVipDailyLibao;
#endif
        //��Ѷ����ͳ��
        m_cmds_process_map["act_tencent"] = ProcessActTencent;
        //��ѯVIP��Ȩ���棺cmd:getVipBenefit
        m_cmds_process_map["getVipBenefit"] = ProcessQueryVipEvent;

        //��ѯ��ļ�佫� cmd��queryGeneralEvent
        m_cmds_process_map["queryGeneralEvent"] = ProcessQueryGeneralEvent;
        //��ѯ��ʯ�ϳɻ cmd��queryBaoshiEvent
        m_cmds_process_map["queryBaoshiEvent"] = ProcessQueryBaoshiEvent;

        //��ѯĿ����Ϣ cmd ��querySevenGoals
        m_cmds_process_map["querySevenGoals"] = ProcessQuerySevenGoals;
        //��ȡĿ�꽱�� cmd ��getSevenGoals
        m_cmds_process_map["getSevenGoals"] = ProcessGetSevenGoals;

        //�������ָ���������ʾ�ѿ�cmd ��setNewPlayerEnd
        m_cmds_process_map["setNewPlayerEnd"] = ProcessSetNewPlayerEnd;
        //��ѯǩ����Ϣ
        m_cmds_process_map["querySignInfo"] = ProcessQuerySignInfo;
        //ǩ��
        m_cmds_process_map["doSign"] = ProcessSign;
        //����ǩ��
        m_cmds_process_map["debugSign"] = ProcessDebugSign;

        //��ȡ���� cmd ��getCharRewards
        m_cmds_process_map["getCharRewards"] = ProcessGetCharRewards;

        //��ѯ��������
        m_cmds_process_map["queryFeedbackEvent"] = ProcessQueryFeedbackEvent;

#ifdef QQ_PLAT
        //��ѯ����������Ϣ
        m_cmds_process_map["QueryInviteInfo"] = ProcessQueryInviteInfo;
        //��ѯ��Ϸ���� cmd��queryShareInfo������ɹ��͸�������
        m_cmds_process_map["queryShareInfo"] = ProcessQueryShareInfo;
        //��ѯ�����ٻ� cmd��queryRecallInfo
        m_cmds_process_map["queryRecallInfo"] = ProcessQueryRecallInfo;
        //��ѯ���ѽ��� cmd��queryCloseFriendInfo
        m_cmds_process_map["queryCloseFriendInfo"] = ProcessQueryCloseFriendInfo;
        //������Ϸ���� cmd��doShare��id����������
        m_cmds_process_map["doShare"] = ProcessDoShare;
        //��ѯ����齱 cmd��queryInviteLottery
        m_cmds_process_map["queryInviteLottery"] = ProcessQueryInviteLottery;
        //��������齱���� cmd��queryInviteLotteryNotice
        m_cmds_process_map["queryInviteLotteryNotice"] = ProcessQueryInviteLotteryNotice;
        //����齱 cmd: inviteLottery
        m_cmds_process_map["inviteLottery"] = ProcessInviteLottery;
        //������� cmd��doInvite��count��������Ѹ���
        m_cmds_process_map["doInvite"] = ProcessDoInvite;
        //��ѯ�������� cmd��QueryInvite
        m_cmds_process_map["QueryInvite"] = ProcessQueryInvite;
#endif
        //��ѯ�����ʱ����
        m_cmds_process_map["queryCharBuffs"] = ProcessQueryCharBuffs;
        //��ѯ�̳�
        m_cmds_process_map["queryMallInfo"] = ProcessQueryMallInfo;
        //�����̳���Ʒ
        m_cmds_process_map["buyMallGoods"] = ProcessBuyMallGoods;
        //��ѯ�ճ�ֵ��Ϣ
        m_cmds_process_map["queryDailyRecharge"] = ProcessQueryDailyRecharge;

#ifdef VN_EN_SERVER
        //��ѯ���Ϣ
        m_cmds_process_map["queryEventInfo"] = ProcessQueryEventInfo;
        //��ȡ�����
        m_cmds_process_map["getEventReward"] = ProcessGetEventReward;
        //��ѯ��¼���Ϣ
        m_cmds_process_map["queryLoginEventInfo"] = ProcessQueryLoginEventInfo;
        //��ȡ��¼�����
        m_cmds_process_map["getLoginEventReward"] = ProcessGetLoginEventReward;
#endif
        //��ѯ����¼
        m_cmds_process_map["queryJxl"] = ProcessQueryJxl;
        //���ý���¼�Ƿ�
        m_cmds_process_map["applyJxl"] = ProcessApplyJxl;
        //��ѯ����¼��ϸ
        m_cmds_process_map["queryJxlDetail"] = ProcessQueryJxlDetail;

        /***************��������ӿ� ***********************/
        m_cmds_process_map["getThroneInfo"] = ProcessGetThroneInfo;    //��ȡ������Ϣ
        m_cmds_process_map["getConInfo"] = ProcessGetConInfo;    //��ȡ�ΰݽ���
        m_cmds_process_map["throneCon"] = ProcessThroneCon;    //�ΰ�

        /***************����ս�ӿ� *************************/
        //�����μӾ���ս
        m_cmds_process_map["JtzSignup"] = ProcessSignupJtz;
        //��ѯ����ս��Ϣ
        m_cmds_process_map["queryJtzInfo"] = ProcessQueryJtzInfo;
        //��ѯ����սս��
        m_cmds_process_map["queryJtzCombat"] = ProcessQueryJtzCombat;
        //��ѯ����ս�ҵ�ս��
        m_cmds_process_map["queryMyJtz"] = ProcessQueryMyJtz;
        //�뿪����ս����
        m_cmds_process_map["leaveJtz"] = ProcessLeaveJtz;
        //��һ��ƥ��
        m_internal_cmds_process_map["JtzMatch"] = ProcessJtzMatch;
        //����ս���ű���
        m_internal_cmds_process_map["openJtzSignup"] = ProcessJtzOpenSignup;
        //��������ս
        m_internal_cmds_process_map["startJtz"] = ProcessStartJtz;

        /***************ս��̨�ӿ� ***********************/
        m_cmds_process_map["queryZstMapInfo"] = ProcessQueryZstMapInfo;//��ѯս��̨��ͼ����
        m_cmds_process_map["queryZstStageInfo"] = ProcessQueryZstStageInfo;//��ѯս��̨��������
        m_cmds_process_map["refreshZstStar"] = ProcessRefreshZstStar;//ˢ��ս��̨�Ǽ�
        m_cmds_process_map["buyZstChallenge"] = ProcessBuyZstChallenge;//����ս��̨��ս����
        m_cmds_process_map["getZstStarReward"] = ProcessGetZstStarReward;//��ȡս��̨�Ǽ�����
        m_cmds_process_map["queryZstStarReward"] = ProcessQueryZstStarReward;//��ѯս��̨�Ǽ�����
        m_cmds_process_map["getZstMapReward"] = ProcessGetZstMapReward;//��ȡս��̨��ͼ����
        m_cmds_process_map["ZstChallenge"] = ProcessZstChallenge;//��սս��̨

        /***************ս�� *****************************/
        //��ѯ������Ϣ
        m_cmds_process_map["queryGSoulInfo"] = ProcessQueryGSoulInfo;
        //��ѯ��������
        m_cmds_process_map["queryGSoulAttr"] = ProcessQueryGSoulAttr;
        //��������
        m_cmds_process_map["upgradeGSoul"] = ProcessUpgradeGSoul;
        //���ý���
        m_cmds_process_map["resetGSoul"] = ProcessResetGSoul;

#ifdef VN_EN_SERVER
        /***************facebook�ӿ� ***********************/
        m_cmds_process_map["getFacebookPresentInfo"] = ProcessGetFacebookPresentInfo;
        m_cmds_process_map["getFacebookPresent"] = ProcessGetFacebookPresent;
#endif
        //�ڲ��ӿ�
        m_internal_cmds_process_map["combatResult"] = ProcessCombatResult;
        m_internal_cmds_process_map["reload"] = ProcessReload;
        m_internal_cmds_process_map["researchDone"] = ProcessResearchDone;
        m_internal_cmds_process_map["farmDone"] = ProcessFarmDone;
        m_internal_cmds_process_map["scheduleEvent"] = ProcessScheduleEvent;
        m_internal_cmds_process_map["trainDone"] = ProcessTrainDone;
        m_internal_cmds_process_map["checkRecharge"] = ProcessCheckRecharge;
        m_internal_cmds_process_map["checkPack"] = ProcessCheckPack;
#if 0
        m_internal_cmds_process_map["smeltDone"] = ProcessSmeltDone;
#endif
        m_internal_cmds_process_map["matchCampRace"] = ProcessMatchCampRace;
        m_internal_cmds_process_map["charlist"] = ProcessQueryAccountCallback;
        m_internal_cmds_process_map["deleteChar"] = ProcessRealyDeleteChar;
        m_internal_cmds_process_map["guardDone"] = ProcessGuardDone;
        m_internal_cmds_process_map["keepDb"] = ProcessKeepDb;
        m_internal_cmds_process_map["logout"] = ProcessOffline;
        m_internal_cmds_process_map["sweepDone"] = ProcessSweepDone;
        m_internal_cmds_process_map["servantDone"] = ProcessServantDone;
        m_internal_cmds_process_map["fruitDone"] = ProcessFruitDone;
        m_internal_cmds_process_map["rankingsEvent"] = ProcessRankingsEvent;

        //ϵͳ�������
        m_internal_cmds_process_map["addAdminNotice"] = ProcessNewAdminNotice;
        m_internal_cmds_process_map["changeAdminNotice"] = ProcessChangeAdminNotice;
        m_internal_cmds_process_map["deleteAdminNotice"] = ProcessDeleteAdminNotice;
        m_internal_cmds_process_map["sendAdminNotice"] = ProcessSendAdminNotice;

        //����ϵͳ��Ϣ
        m_internal_cmds_process_map["broadCastMsg"] = ProcessBroadCastMsg;

        //��ʱ����״̬�ı�
        m_internal_cmds_process_map["buffChange"] = ProcessBuffChange;

        //�ط�����
        m_internal_cmds_process_map["shutdown"] = ProcessShutdown;

        //���������ʱ
        m_internal_cmds_process_map["online_gift"] = ProcessOnlineGiftTimer;

        //��ѯ�˻������ص�
        m_internal_cmds_process_map["queryScore"] = ProcessQueryAccountPointsCallBack;

        //����ǩ����ť״̬
        m_internal_cmds_process_map["notifySign"] = ProcessSendSignState;

        //����Ҫ��¼�Ϳ���ʹ�õĽӿ�
        //ս���ط�
        m_spe_cmds_process_map["getBattleList"] = ProcessGetBattltRecord;

        //��ʼ�������б�
        InitCreateList();
    }

    virtual bool work(actionmessage& task)       // ��Щ���ʵ������.
    {
        try
        {
            shhx_log_msg(task.cmd);
            if (task.from()== 1)
            {
                session_ptr psession;
                task.getsession(psession);

                boost::unordered_map<std::string, pFuncInternalProcessCmds>::iterator it = m_internal_cmds_process_map.find(task.cmd);
                if (it != m_internal_cmds_process_map.end())
                {
                    (*(it->second))(task.getRecvObj());
                }
                else
                {
                    INFO("work recv internal cmd "<<task.cmd<<endl);
                }
                return true;
            }
            INFO("main work recv cmd "<<task.cmd<<","<<splsTimeStamp()<<endl);
            //uint64_t recv = splsTimeStamp();
            session_ptr psession;
            task.getsession(psession);
            if (!psession.get())
            {
                INFO("action worker get no session"<<endl);
                return true;
            }
#ifdef DEBUG_PER
            uint64_t recv_time = 0;
            if (psession.get() && psession->is_debug_)
            {
                recv_time = splsTimeStamp();
            }
#endif
            using namespace std;
            using namespace boost;
            using namespace json_spirit;

            bool bFind = false;
            boost::unordered_map<std::string, pFuncProcessCmds>::iterator it;
            if (psession->state() == STATE_AUTHED)
            {
                it = m_cmds_process_map.find(task.cmd);
                bFind = (it != m_cmds_process_map.end());
            }
            else
            {
                it = m_spe_cmds_process_map.find(task.cmd);
                bFind = (it != m_spe_cmds_process_map.end());
            }
            if (bFind)
            {
                Object robj;
                int ret = (*(it->second))(psession, task.getRecvObj(), robj);
                if (HC_SUCCESS_NO_RET != ret)
                {
                    robj.push_back( Pair("cmd", task.cmd));

                    if (HC_ERROR_NO_RET != ret)
                    {
                        robj.push_back( Pair("s", ret));
                        if (ret != HC_SUCCESS)
                        {
                            std::string msg = getErrMsg(ret);
                            if ("" != msg)
                            {
                                robj.push_back( Pair("msg", msg));
                            }
                        }
                    }
                    psession->send(write(robj, json_spirit::raw_utf8));
                    CharData* pc = NULL;
                    getSessionChar(psession, pc);
                    if (pc)
                    {
                        pc->realNotifyOpenInfo(psession);
                        pc->NotifyCharData_(psession);
                    }
                }
#ifdef DEBUG_PER
                if (psession.get() && psession->is_debug_)
                {
                    cout<<"cmd("<<task.cmd<<") process cost "<<(double(splsTimeStamp()-recv_time)/1000)<<" ms|";
                    cout<<(double(recv_time-task._recv_time)/1000)<<endl;
                    //cout<<"|getjob cost "<<(double(task._get_time-task._start_get)/1000)<<" us"<<endl;
                }
#endif
            }
            else
            {
                Object robj;
                robj.push_back( Pair("cmd", task.cmd));
                robj.push_back( Pair("s", 401));
                robj.push_back( Pair("m", "unknow cmd"));
                psession->send(write(robj, json_spirit::raw_utf8));
            }
            return true;
        }
        catch (std::exception& e)
        {
            syslog(LOG_ERR, "action work , Exception: %s", e.what());
            syslog(LOG_ERR, "cmd: %s", task.cmd.c_str());
            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);
            for (int i = 0; i < nSize; i++)
            {
                syslog(LOG_ERR, symbols[i]);
            }
            free(symbols);
        }
        return true;
    }
private:
    boost::unordered_map<std::string, pFuncProcessCmds> m_cmds_process_map;
    boost::unordered_map<std::string, pFuncProcessCmds> m_spe_cmds_process_map;
    boost::unordered_map<std::string, pFuncInternalProcessCmds> m_internal_cmds_process_map;
};

