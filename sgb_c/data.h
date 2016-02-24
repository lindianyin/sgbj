
#ifndef _DATA_H_
#define _DATA_H_

#include <string>
#include <map>
#include <list>
#include <boost/cstdint.hpp>

#include "boost/smart_ptr/shared_ptr.hpp"
#include <boost/enable_shared_from_this.hpp>

#include <vector>
#include "json_spirit.h"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include "net.h"
#include "errcode_def.h"
#include "const_def.h"

#include "jobqueue.hpp"
#include "worker.hpp"

#include "utils_all.h"

#include "hero.h"
#include "skill.h"
#include "magic.h"
#include "item.h"
#include "map.h"
#include "loot.h"
#include "wild.h"
#include "task.h"
#include "goal.h"
#include "shenling.h"
#include "dailyScore.h"
#include "guild.h"
#include "statistics.h"
#include "buff.h"
#include "prestige_task.h"

//typedef boost::shared_mutex rwmutex;
//typedef boost::shared_lock<rwmutex> readLock;
//typedef boost::unique_lock<rwmutex> writeLock;

struct readLock
{
    readLock(rwlock* l)
    {
        lock = l;
        rwlock_rlock(l);
    }
    ~readLock()
    {
        unlock();
        lock = NULL;
    }
    void unlock()
    {
        if (lock)
        {
            rwlock_runlock(lock);
            lock = NULL;
        }
    }
    rwlock* lock;
};

struct writeLock
{
    writeLock(rwlock* l)
    {
        lock = l;
        rwlock_wlock(l);
    }
    ~writeLock()
    {
        unlock();
    }
    void unlock()
    {
        if (lock)
        {
            rwlock_wunlock(lock);
            lock = NULL;
        }
    }
    rwlock* lock;
};

using boost::uint64_t;
using namespace std;
using namespace json_spirit;
using namespace boost;

struct CharData;
class ChatChannel;
class OnlineUser;
struct OnlineCharactor;
struct CharactorInfo;
struct CharTempoData;


int getSessionChar(net::session_ptr& psession, boost::shared_ptr<CharData>& cdata);
int getSessionChar(net::session_ptr& psession, CharData* &pc);

enum notify_msg_type
{
    notify_msg_new_congratulation = 1,
    notify_msg_recv_congratulation = 2,
};

enum enum_char_data_type
{
    char_data_type_normal = 0,
    char_data_type_daily = 1,
    char_data_type_week = 2
};

enum enum_char_data_extra
{
//��ͨ��
    char_data_normal_tmp = 1,
    char_data_normal_smelt_hero_refresh = 2,
    char_data_normal_arena_maxrank = 3,
    char_data_normal_arena_maxwin = 4,
    char_data_normal_sign_score = 5,
    char_data_normal_stronghold = 6,
    char_data_normal_copy = 7,
    char_data_normal_shenling = 8,
    char_data_normal_current_guide = 9,
    char_data_normal_first_recharge_event1 = 10,//�׳�1
    char_data_normal_first_recharge_event2 = 11,//�׳�2
    char_data_normal_help_times = 12,//���ְ����������
    char_data_normal_first_refresh_daily_score = 13,//���ֵ�һ��ˢ��ÿ�ձ���
    char_data_normal_first_refresh_treasure = 14,//����ˢ��ɫ����
    char_data_normal_lottery_score = 15,
    char_data_normal_lottery_get_recharge_score = 16,
    char_data_normal_lottery_total_recharge_score = 17,
    char_data_normal_tmp_vip = 18,

    char_data_normal_copy_reward_start = 800,//������ͼͨ�ؽ�����ȡ��ǿ�ʼ
    char_data_normal_copy_reward_end = 899,//������ͼͨ�ؽ�����ȡ��ǽ���
    char_data_normal_copy_perfect_reward_start = 900,//������ͼ����ͨ�ؽ�����ȡ��ǿ�ʼ
    char_data_normal_copy_perfect_reward_end = 999,//������ͼ����ͨ�ؽ�����ȡ��ǽ���
    char_data_normal_stage_reward_start = 1000,//ͨ�ؽ�����ȡ��ǿ�ʼ
    char_data_normal_stage_reward_end = 2000,//ͨ�ؽ�����ȡ��ǽ���
    char_data_normal_qq_yellow_special = 2001,//QQ������Ȩ�����ȡ
    char_data_normal_qq_yellow_newbie = 2002,//QQ�������������ȡ
    char_data_normal_qq_yellow_level_libao = 2003,//QQ����ȼ������ȡ - ����500��
    char_data_normal_qq_yellow_level_libao_end = 2499,
    char_data_normal_goal_level_reward_start = 2500,//Ŀ��ϵͳ�ȼ����
    char_data_normal_goal_level_reward_end = 3000,

//ÿ�յ� 10000 ��
    char_data_daily_tmp = 10000,
    char_data_daily_explore_begin = 10001,
    char_data_daily_explore_end = 11000,
    char_data_daily_buy_copy = 11001,
    char_data_daily_reset_copy = 11002,
    char_data_daily_attack_copy = 11003,
    char_data_daily_buy_arena = 11004,
    char_data_daily_arena = 11005,
    char_data_daily_arena_score = 11006,
    char_data_daily_arena_cd = 11007,
    char_data_daily_arena_reward_start = 11008,//�����������̳���ȡ���
    char_data_daily_arena_reward_end = 11200,//�����������̳���ȡ���
    char_data_daily_seven_action_reward = 11201,
    char_data_daily_hero_pack_action = 11202,
    char_data_daily_shenling = 11203,
    char_data_daily_treasure = 11204,
    char_data_daily_treasure_rob = 11205,
    char_data_daily_arena_shop_open = 11206,
    char_data_daily_attack_wild = 11207,
    char_data_daily_gold_levy = 11208,
    char_data_daily_daily_score_refresh = 11209,
    char_data_daily_guild_contribution = 11210,
    char_data_daily_guild_box = 11211,
    char_data_daily_task_refresh = 11212,
    char_data_daily_online_time = 11213,
    char_data_daily_free_levy = 11214,
    char_data_daily_view_first_recharge = 11215,
    char_data_daily_view_hero_pack_action = 11216,
    char_data_daily_get_lottery_score = 11217,
    char_data_daily_congratulation_received = 11218,    //�����յ���ף����
    char_data_daily_congratulation_sended = 11219,        //���췢����ף����

    char_data_daily_findback = 14001,//�һر��
    char_data_daily_findback_start = 14002,//�һؼ�¼
    char_data_daily_findback_end = char_data_daily_findback_start + 50,

    char_data_daily_guild_moshen = 14100,
    char_data_daily_guild_moshen_start = 14101,
    char_data_daily_guild_moshen_end = char_data_daily_guild_moshen_start + 99,

    char_data_daily_qq_yellow_libao = 15001,//QQ����ÿ�������ȡ
    char_data_daily_qq_year_yellow_libao = 15002,//QQ��ѻ���ÿ�������ȡ

//ÿ�ܵ� 20000 ��
    char_data_week_tmp = 20000,
};

//���涥����ť����
enum top_button
{
#ifdef QQ_PLAT
    top_button_yellow = 0,//���갴ť
#endif
    top_button_daily = 1,//ÿ�ձ���
    top_button_timeLimitAction = 2,//��ʱ�
    top_button_sign = 3,//ǩ��
    top_button_treasure = 4,//�ر�ͼ
    top_button_rechargeAction = 5,//��ֵ�
    top_button_daily_task = 6,//�ճ�����
    top_button_online = 7,//�������
    top_button_first_recharge = 8,//�׳�
    top_button_goal = 9,//Ŀ��ϵͳ
    top_button_dailyAction = 10,//�ճ��
    top_button_bank = 11,//����
    top_button_lotteryAction = 12,//ת�̻
    top_button_pack = 13,//�һ���������
    top_button_weekRanking = 14,//������

    top_button_city_levy = 1001,//�Ǳ�����
    top_button_city_recruit = 1002,//�Ǳ�
    top_button_smelt_refresh = 1003,//����Ӣ��ˢ��
    top_button_prestige_task = 1004,//��������
    top_button_find_back = 1005,//�һؽ���

    top_button_reward_start = 2000,
    top_button_arena_rank_reward = 2001,
    top_button_pk_rank_reward = 2002,
    top_button_boss_reward = 2003,
    top_button_boss_kill_reward = 2004,
    top_button_weekRanking_reward = 2005,
};

enum log_out_list_type
{
    log_out_levy = 1,
    log_out_treasure = 2,
    log_out_daily_socre = 3,
    log_out_daily_task = 4,
    log_out_sign = 5,
    log_out_online = 6,
    log_out_wild = 7,
    log_out_lottery = 8,
    log_out_arena = 9,
    log_out_shenling = 10,
    log_out_copy = 11,
};

enum guide_id_enum
{
    guide_id_get_silver = 115,//�������ֳ���Ϊ0
};

enum daily_action
{
    action_boss = 1,
};

enum channel_type
{
    channel_race = 1,
    channel_world = 2,
    channel_horn = 3,
    channel_wisper = 4,
    channel_broad = 5,
    channel_guild = 6,
    channel_room = 7,
};

//����
struct baseRaceData
{
    int m_type;//1������÷����2ħ��ʦ���ң�3������ʿ���飬4��ʿ����
    std::string m_name;
    int m_attak_add[iMaxLevel];//�����ӳ�
    int m_defense_add[iMaxLevel];//�����ӳ�
    int m_magic_add[iMaxLevel];//ħ���ӳ�
    int m_resident_add;//�Ǳ�����ӳ�
    int m_silver_add;//��������ӳ�
    int m_hp_add;//��սӢ�������ӳ�
    int m_mag_add;//ħ��ֵ�ӳ�
    int m_compound_add;//�ϳɳɹ��ʼӳ�
    int getLevelAdd(int level, int& attack_add, int& defense_add, int& magic_add);
};

//�ȼ�����
struct baseLevelData
{
    int m_level;//�ȼ�
    int m_need_char_exp;//����������Ҿ���(����)
    double m_reward_add;//ս�������ӳ�
    int m_guide_id;
};

struct Currency
{
    uint16_t type;
    uint32_t id;
    int32_t count;
    Currency()
    {
        id = 0;
        type = 0;
        count = 0;
    }
};

//��������
struct basePrestigeData
{
    int m_level;//�ȼ�
    int m_need_exp;//������������
};

//������������
struct CharPrestigeAward
{
    int m_cid;
    int m_race;
    std::vector<int> m_state;//�������ȡ���
    void save();
};

//�ǳ�
enum nick_enum
{
    nick_arena_start = 1,//�����������ǳ�1-3
    nick_arena_end = 3,
    nick_pk_start = 4,//pk�������ǳ�4-6
    nick_pk_end = 6,
    nick_weekRank_start = 7,
    nick_weekRank_end = 20,
};

struct nick
{
    //�ǳ��б�
    std::list<int> m_nick_list;

    void add_nick(int n);

    void remove_nick(int n);

    bool check_nick(int n);

    std::string get_string();

    void init(const std::string& data);
};

struct baseUpgradeEquiptData
{
    int m_level;//�ȼ�
    int m_fail_per;//ʧ�ܰٷֱ�
    int m_need_stone;//��Ҫǿ��ʯ
    int m_bless_max;//ף��ֵ����
    int m_bless_value;//ף��ֵ��Чֵ
    int m_bless_per;//ף��ֵ��Ч�ٷֱ�
};

//��ɫ����
struct CharData:public boost::enable_shared_from_this<CharData>
{
public:
    CharData(int cid, bool b_create = false);
    ~CharData();
    int Create();
    int Load();

    //�û���ɫ����
    const std::string& GetCharName();
    //�û���ɫid
    uint64_t GetCharId();
    //��ɫ����id
    int GetGuildId();
    //��ɫ�ȼ�
    int levelup(int level);
    //��������
    int addCharExp(int exp, int statistics_type = 0);
    int addGold(int gold, int statistics_type = 0, bool only_real = false);
    int subGold(int gold, int statistics_type = 0, bool only_real = false);
    int gold(bool only_real = false) {return only_real ? m_gold : (m_gold+m_bind_gold);}
    int gold(int);
    int addSilver(int silver, int statistics_type = 0);
    int subSilver(int silver, int statistics_type= 0);
    int silver() {return m_silver;}
    int silver(int);
    int addBankSilver(int silver);
    int subBankSilver(int silver);
    int BankSilver() {return m_bank_silver;}
    int combatSilverMax();
    int addPrestige(int race, int prestige, int statistics_type = 0);
    int prestige(int race);
    int prestigeLevel(int race);
    int prestigeMax(int race);
    int addEquipt(int id, int level = 1, int quality = 1);//���ӽ�ɫװ��
    int subEquipt(int id);
    int addGem(int id, int counts, int statistics_type = 0);//���ӽ�ɫ����
    int subGem(int id, int counts, int statistics_type = 0);//���Ľ�ɫ����
    int addCurrency(int id, int counts);//���ӽ�ɫ��Դ����
    int subCurrency(int id, int counts);//���Ľ�ɫ��Դ����
    int addLibao(int libao_id, int counts);
    int addBaoshi(int base_id, int level, int counts);
    int updateVip();

    int getRoleInfo(json_spirit::Object& charobj);
    int getRoleDetail(json_spirit::Object& robj);
    int getTopButtonList(json_spirit::Array& list);
    //֪ͨ�ͻ��˽�ɫ��Ϣ�����仯
    int NotifyCharData();
    int NotifyCharData_(net::session_ptr& sk);
    int NotifyCharTopButtonList();

    int sendObj(json_spirit::Object& obj);

    //����
    int HeartBeat();
    //��ȡ��������ʱ��(��)
    int getTodayOnlineTime();
    //����
    int Save();
    //���ý����������ʾ
    int enableNoConfirmGoldCost(int type, bool enable);
    bool getNoConfirmGoldCost(int type);

    //���������ֶ�
    void loadExtraData();
    //ȡ��ɫ�ֶ�ֵ
    int queryExtraData(int type, int field);
    //���ý�ɫ�ֶ�ֵ
    void setExtraData(int type, int field, int value);
    //�����ɫ�ֶ�ֵ
    void clearExtraData(int type);

    //�������ƹ���
    void loadLootTimes();
    int getLootTimes(int loot_type, int loot_id);
    void setLootTimes(int loot_type, int loot_id, int value);
    void clearLootTimes();

    //��ȡ������ɫ����Ȼ����
    int queryCreateDays();
    //��ȡ������ɫ��Ȼ���ʱ���
    time_t queryCreateXDays(int day);

    //����Ƿ񴥷�����
    int checkGuide(int type, int param1, int param2);
    //����Ƿ񴥷�����
    int checkGuide(int id);
    time_t getGuideState(int id);
    void setGuideStateComplete(int id, int next_guide);
    //������ť
    void updateTopButton(int type, int active, int leftNums = 0, int extra1 = 0, int extra2 = 0, int extra3 = 0);
    void addTopButton(int type, int active, int leftNums = 0, int extra1 = 0, int extra2 = 0, int extra3 = 0);
    void removeTopButton(int type);

    //�����Ƿ�������һ�
    bool canFindBack();

    //����װ��
    int ShowEquipts(json_spirit::Object& obj, json_spirit::mObject& o);
    int equipt(int hid, int slot, int eid);
    int unequipt(int hid, int slot);
    int useGem(int hid, int tid, int nums);
    //װ���ϳ�
    int CompoundEquiptInfo(json_spirit::Object& obj, json_spirit::mObject& o);
    int CompoundEquiptOneKey(json_spirit::Object& obj, json_spirit::mObject& o);
    int CompoundEquipt(json_spirit::Object& obj, json_spirit::mObject& o);
    //����װ��
    int UpgradeEquiptInfo(json_spirit::Object& obj, json_spirit::mObject& o);
    int UpgradeEquipt(json_spirit::Object& obj, json_spirit::mObject& o);

    //���б�ʯ
    int ShowBaoshis(json_spirit::Object& obj, json_spirit::mObject& o);
    //��Ƕ��ʯ
    int inlayBaoshi(int bagSlot, int eid, int slot);
    //�Ƴ���ʯ
    int removeBaoshi(int eid, int slot);
    //�ϲ���ʯ
    int CombineBaoshi(int base_id, int level, int nums, json_spirit::Object& robj);
    int CombineAllBaoshi(int tolevel, json_spirit::Object& robj);

public:
    //���ܿ���
    inline bool isSignOpen(){return m_level >= iSignOpenLevel;}
    inline bool isOnlineLibaoOpen(){return m_level >= iOnlineLibaoOpenLevel;}
    inline bool isFirstRechargeOpen(){return m_level >= iFirstRechargeOpenLevel;}
    inline bool isTimelimitActionOpen(){return m_level >= iTimelimitActionOpenLevel;}
    inline bool isRechargeActionOpen(){return m_level >= iRechargeActionOpenLevel;}
    inline bool isTreasureOpen(){return m_level >= iTreasureOpenLevel;}
    inline bool isDailyScoreOpen(){return m_level >= iDailyScoreOpenLevel;}
    inline bool isDailyTaskOpen(){return m_level >= iDailyTaskOpenLevel;}
    inline bool isLotteryActionOpen(){return m_level >= iLotteryActionOpenLevel;}
    inline bool isBossOpen(){return m_level >= iBossOpenLevel;}
    inline bool isBankOpen(){return m_level >= iBankOpenLevel;}
    inline bool isArenaOpen(){return m_level >= 14;}
    inline bool isCopyOpen(){return m_level >= 15;}
    inline bool isShenlingOpen(){return m_level >= 16;}
    inline bool isWeekRankingActionOpen(){return m_level >= 8;}
public:
    static volatile uint64_t _refs;
    static uint64_t refs();
    bool m_load_success;
    int m_id;   //��ɫid
    int m_level;//��ɫ�ȼ�
    int m_spic; //��ɫͷ��
    int m_gender;//�Ա� 1�� 0Ů
    int m_vip;  //vip�ȼ�
    int m_real_vip;//��ʵvip
    int m_tmp_vip;//��ʱvip
    int m_race; //����
    boost::shared_ptr<baseRaceData> m_race_data;//�������ݼӳ�
    boost::shared_ptr<baseLevelData> m_level_data;//�ȼ����ݼӳ�
    int m_prestige[4];//��������
    int m_prestige_level[4];//���������ȼ�
    CharPrestigeAward m_prestige_award[4];
    int m_cur_mapid;//��ɫ���ڵ���
    int m_cur_stageid;//��ɫ���ڵ���
    int m_cur_strongholdid;//��ɫ���ȹؿ�
    std::string m_name;//��ɫ����
    std::string m_chat;//ս����������
    int m_union_id;
    std::string m_account;//�˻�
    std::string m_qid;//ƽ̨id
    std::string m_server_id;
    std::string m_ip_address;//ip��ַ
    time_t m_createTime;//��ɫ����ʱ��
    time_t m_levelupTime;//��ɫ����ʱ��
    time_t m_login_time;//��ɫ��¼ʱ��

    int m_is_online;//�Ƿ�����

    //��Ҫ֪ͨ��ҵ�����
    std::map<int, int> m_need_notify;

    //����
    int m_total_attack;
    int m_total_defense;
    int m_total_hp;

    //��ť����1����0�ر�
    uint8_t m_panel_tmp;//��ť

    bool m_can_world_chat;          //�Ƿ������������
    bool m_can_chat;                //�Ƿ񱻽���
    bool m_char_data_change;    //��ɫ��Ϣ�����仯

    //�������ȷ��
    int m_gold_cost_comfirm[iMaxGoldCostConfirm];

    //��������������
    std::map<int,time_t> m_guide_completes;
    int m_current_guide;        //��ǰ��������

    //�ۼƳ�ֵ���
    int m_total_recharge;
    //�������ӵ�VIP����
    int m_vip_exp;

    //����ֵ
    int m_char_exp;
    //�ϴδ���ʱ��
    time_t m_save_time;

    std::map<int, int> m_normal_extra_data;
    std::map<int, int> m_daily_extra_data;
    std::map<int, int> m_week_extra_data;
    //�������
    std::map<std::pair<int, int>, int> m_loot_cnt;
#ifdef QQ_PLAT
    int m_qq_yellow_level;      //QQ����ȼ�
    int m_qq_yellow_year;       //�Ƿ�QQ��ѻ���
    std::string m_iopenid;      //QQƽ̨�Ƽ����
    std::string m_feedid;       //QQƽ̨����id
    std::string m_login_str1;   //QQƽ̨��¼��Ϣ
    std::string m_login_str2;   //QQƽ̨��¼��Ϣ
#endif
    nick m_nick;
    void SaveNick();

    int m_silver_get_combat;//ս���л�ó���(������)
    int m_double_times;//��������
private:
    int m_gold;     //���
    int m_bind_gold;//�󶨽��
    int m_silver;   //���ң�����
    int m_bank_silver;//���г���
public:
    std::map<int, Currency> m_currencys;//���������
    boost::shared_ptr<CharGuildData> m_guild_data;//��������
    boost::shared_ptr<CharGuildForeverData> m_guild_forever_data;//������������
    //ս������
    combatAttribute m_combat_attribute;
    //Ӣ���б�
    CharTotalHeros m_heros;
    //����
    CharTotalSkills m_skills;
    CharTotalMagics m_magics;
    //�ؿ�����
    CharTempoData m_tempo;
    //����
    CharBag m_bag;
    //����ǳ�
    CharWildCitys m_wild_citys;
    //��ɫ����
    CharAllTasks m_tasks;
    //ÿ�ձ�������
    CharDailyScore m_score_tasks;
    //��ɫ��������
    CharAllPrestigeTasks m_prestige_tasks;
    //��ʱ����ӳ�
    CharBuffs m_Buffs;
};

//�����û�
class OnlineUser: public boost::enable_shared_from_this<OnlineUser>
{
public:
    OnlineUser();
    OnlineUser(const std::string& qid, const std::string& account, int union_id, const std::string& server_id, net::session_ptr h);
    ~OnlineUser();
    //�û�ע��id
    const std::string& GetAccount();
    //�ر�����
    int CloseConnect();

    //���ip��ַ
    const std::string& GetRemoteAddress();
    net::session_ptr GetSocket()
    {
        return m_sockethandle;
    };

    //��¼��ɫ
    int Login(uint64_t cid);

    int Kickout(const std::string& reason);
    boost::shared_ptr<OnlineUser> getAccount();

    friend struct OnlineCharactor;
    friend class ActionWorker;

    std::string m_qid;            //ƽ̨qid
    std::string m_account;      //�˺�
    std::string m_server_id;    //ƽ̨server_id
    int m_union_id;    //������union_id
    std::string m_ipaddress;    //IP
    net::session_ptr m_sockethandle;
    int m_state;                // 0 �����ʺ���Ϣ�� 1 ���
    int m_isAdult;                /*    0    �û�δ��дʵ������Ϣ
                                1    �û���д��ʵ������Ϣ���Ҵ���18��
                                2    �û���д��ʵ������Ϣ������С��18��)*/

    int m_qq_yellow_level;    //QQ����ȼ�
    int m_qq_yellow_year;        //�Ƿ�QQ��ѻ���
#ifdef QQ_PLAT
    std::string m_iopenid;
    std::string m_feedid;
    std::string m_login_str1;    //��¼��Ϣ
    std::string m_login_str2;    //��¼��Ϣ
#endif
    time_t m_logintime;           // ��¼ʱ��
    boost::shared_ptr<OnlineCharactor> m_onlineCharactor;  //��ɫ
    uint64_t m_cid;

    static volatile uint64_t _refs;
};

class channelProcesser
{
public:
    channelProcesser(const std::string& name, net::jobqueue<std::string>& _jobqueue, std::size_t _maxthreads = 1) :
    worker_name(name),
    maxthreads_(_maxthreads),
    _runing_loop(0),
    jobqueue_(_jobqueue),
    exitthread(false)
    {
    }
    ~channelProcesser(void)
    {
    }
    void run();
    void stop();
    bool work(std::string &cmd);       // ��Щ���ʵ������.
    void workloop();
    int running();
    void setChannel(boost::shared_ptr<ChatChannel>);
private:
    boost::shared_ptr<ChatChannel> _channel;
    std::string worker_name;
    std::vector<boost::shared_ptr<boost::thread> > threads_;
    boost::mutex mutex_;
    std::size_t maxthreads_;
    volatile int _runing_loop;
    net::jobqueue<std::string>& jobqueue_;
    volatile bool exitthread;
};

#define CHANNEL_LOCK 1

//����Ƶ��
class ChatChannel: public boost::enable_shared_from_this<ChatChannel>
{
public:
    ChatChannel();
    ChatChannel(const std::string &, uint64_t id, const std::string&);
    ~ChatChannel();
    int Add(boost::shared_ptr<OnlineCharactor> p);
    int Remove(boost::shared_ptr<OnlineCharactor> p);
    int Remove(int cid);
    uint64_t GetChannelId();
    bool IsEmpty();
    int Chat(const std::string &, const std::string &, int type = 0, int gender = 0, const std::string& nick = "[]");
    int BroadMsg(const std::string &);
    //�㲥��ָ���ȼ��������
    int BroadMsg(const std::string & msg, int level);

    int Clear();
    void start();
    void stop();

    friend class channelProcesser;

private:
    int _BroadMsg(const std::string &);
    std::list<boost::shared_ptr<OnlineCharactor> > m_useridlist;
    std::string m_channelname;
    std::string m_prompt;
    std::string m_s;
    net::jobqueue<std::string> m_broadmsg_que;
    channelProcesser m_worker;

    uint64_t m_channelid;
#ifdef CHANNEL_LOCK
    boost::mutex channel_mutex_;
#endif
};

struct admin_notice
{
    int _id;
    int _state;
    std::string _message;
};

//��������
struct basePrestigeAward
{
    int id;
    int race;
    int need_prestige_level;
    Item m_item;
};

typedef std::map<int, boost::shared_ptr<basePrestigeAward> > PrestigeAwards;

//�����̵���Ʒ
struct basePrestigeGoods
{
    int id;
    int race;
    int need_prestige_level;
    int silver;
    Item m_item;
};

typedef std::map<int, boost::shared_ptr<basePrestigeGoods> > PrestigeGoods;

class GeneralDataMgr
{
public:
    GeneralDataMgr();
    ~GeneralDataMgr();
    int reload(int flag);

    int loadLang();

    int reloadOtherBaseData();
    static GeneralDataMgr* getInstance();
    static void release();
    time_t getServerOpenTime() {return server_open_time;}
    int GetBaseExp(int level);
    void addCharData(boost::shared_ptr<CharData> cdata);    //�����ɫ����
    boost::shared_ptr<CharData> GetCharData(int cid);    //��ȡ��ɫ����
    boost::shared_ptr<baseRaceData> GetBaseRace(int type);
    boost::shared_ptr<baseLevelData> GetLevelData(int level);
    boost::shared_ptr<baseGem> GetBaseGem(int tid);
    boost::shared_ptr<baseEquipment> GetBaseEquipment(int baseid);
    boost::shared_ptr<baseUpgradeEquiptData> GetUpgradeEquiptData(int level);
    boost::shared_ptr<baseBaoshi> GetBaseBaoshi(int baseid);
    //�������߽�ɫ��Ϣ
    boost::shared_ptr<OnlineCharactor> CreateOnlineCharactor(boost::shared_ptr<OnlineUser> account, uint64_t cid);
    //���߽�ɫ��Ϣ
    boost::shared_ptr<OnlineCharactor> GetOnlineCharactor(const std::string& char_name);
    //���������ս�б�
    int GetOnlineChallengeList(const std::string& char_name, int level, json_spirit::Array& list);
    //��õ����б�
    void GetGemList(json_spirit::mObject& o, json_spirit::Object& robj);

    int Logout(boost::shared_ptr<OnlineUser>& p);
#ifdef QQ_PLAT
    int Login(const std::string& qid, const std::string& id, int isAdult, int union_id, const std::string& server_id, int qqyellow, int isyear, const std::string& iopenid, const std::string& feedid, const std::string& str1, const std::string& str2, net::session_ptr csocket, Object& robj);
#else
    int Login(const std::string& qid, const std::string& id, int isAdult, int union_id, const std::string& server_id, net::session_ptr csocket, Object& robj);
#endif
    boost::shared_ptr<OnlineUser> GetAccount(const std::string& account);
    boost::shared_ptr<ChatChannel> GetWorldChannel();
    boost::shared_ptr<ChatChannel> GetHornChannel();
    boost::shared_ptr<ChatChannel> GetCampChannel(int camp);
    boost::shared_ptr<ChatChannel> GetGuildChannel(uint64_t guild_id);

    int CharactorLogin(boost::shared_ptr<OnlineCharactor> oc);
    int CharactorLogout(boost::shared_ptr<OnlineCharactor> oc);
    //�����ɫ��Ϣ
    int SaveDb(int save_all);

    //ϵͳ��Ϣ�㲥
    int broadCastSysMsg(const std::string& msg, int type);
    //ϵͳ��Ϣ�㲥- level�ȼ�����
    int broadCastSysMsg(const std::string& msg, int type, int level);

    //�㲥��ȫ���������
    int broadCastToEveryone(const std::string & msg, int repeatnums, int interval);
    uint32_t newCharId()
    {
        return ++m_charactor_id;
    }
    uint32_t newHeroId()
    {
        return ++m_hero_id;
    }
    uint32_t newGemId()
    {
        return ++m_gem_id;
    }
    uint32_t newEquiptId()
    {
        return ++m_equipt_id;
    }
    uint32_t newBaoshiId()
    {
        return ++m_baoshi_id;
    }
    uint64_t newCombatId()
    {
        return ++m_combat_id;
    }
    //ɾ����ɫʱ���Ƴ���ɫ����
    int removeCharData(int cid);
    //ÿ�ܸ���
    int weekReset();
    //��������
    int getTotalOnline(bool record);
    //����
    int HeartBeat();

    //���ϵͳ����仯
    void checkAdminNotice(int type);
    int adminNoticeDeleted(int id);
    int adminNoticeNew(int id, const std::string& message);
    int adminNoticeChanged(int id, const std::string& message);
    int getAdminNotice(json_spirit::Array& notice_list);

    void shutdown();

    //��ý�ɫid
    int GetCharId(const std::string& cname);

    void setHeroOwner(int hid, int cid);
    int getHeroOwner(int hid);
    void removeHeroOwner(int hid);

    int getInt(const std::string& field, int defaultv = 0);
    void setInt(const std::string& field, int value);

    std::string getStr(const std::string& field);
    void setStr(const std::string& field, const std::string& value);
    //�������
    std::string getRandomName(int gender);
    //ÿ������
    int dailyUpdate();
    //ÿ������
    int weekUpdate();
    //֪ͨ�����Ϣ
    int dailyOnlineChar();
    //��ȡ����������Ϣ
    boost::shared_ptr<basePrestigeData> GetPrestigeData(int level);
    //��ȡĳ���������б�
    boost::shared_ptr<PrestigeAwards> GetBasePrestigeAward(int race);
    //��ȡĳ�����̵��б�
    boost::shared_ptr<PrestigeGoods> GetBasePrestigeGoods(int race);

    //��������
    bool canRecommendFriends(CharData* cdata);
    void getRecommendFriends(CharData* cdata, std::map<int, boost::shared_ptr<CharData> >& char_list);
private:
    static GeneralDataMgr* m_handle;
    time_t server_open_time;//����ʱ��

    rwlock guild_chanel_rwmutex;
    rwlock camp_chanel_rwmutex;
    rwlock onlineuser_rwmutex;

    rwlock onlinechar_rwmutex;
    rwlock globalchar_rwmutex;

    //Ƶ��
    boost::shared_ptr<ChatChannel> m_world_channel;     //��������
    boost::shared_ptr<ChatChannel> m_horn_channel;     //��������
    boost::unordered_map<int, boost::shared_ptr<ChatChannel> > m_camp_channels;         //��Ӫ����
    boost::unordered_map<uint64_t, boost::shared_ptr<ChatChannel> > m_guild_channels;   //��������

    boost::unordered_map<std::string, boost::shared_ptr<OnlineUser> > m_onlineuserlist;

    std::map<int, boost::shared_ptr<CharData> > m_chardata_map;             //ȫ����ɫ����
    std::map<std::string, int> m_charid_map;       //ȫ����ɫidӳ��
    std::map<int, int> m_hero_map;    //Ӣ�ۺͽ�ɫ��Ӧ��

    std::map<std::string, boost::shared_ptr<OnlineCharactor> > m_online_charactors; //���߽�ɫ

    volatile uint32_t m_charactor_id;
    volatile uint32_t m_hero_id;
    volatile uint32_t m_gem_id;
    volatile uint32_t m_equipt_id;
    volatile uint32_t m_baoshi_id;
    volatile uint64_t m_combat_id;

    bool m_inited;

    //��������
    std::map<int, int> m_base_exps; //������������
    std::map<int, boost::shared_ptr<baseRaceData> > m_base_races;    //������������
    std::map<int, boost::shared_ptr<baseLevelData> > m_base_levels;    //�ȼ�����
    std::map<int, boost::shared_ptr<baseGem> > m_base_gems;         //������������
    std::map<int, boost::shared_ptr<baseEquipment> > m_base_equipments;     //����װ������
    std::map<int, boost::shared_ptr<baseBaoshi> > m_base_baoshis;     //������ʯ����

    std::map<int, boost::shared_ptr<basePrestigeData> > m_base_prestiges;//��������
    std::map<int, boost::shared_ptr<PrestigeAwards> > m_base_prestige_awards;//����������Ʒ
    std::map<int, boost::shared_ptr<PrestigeGoods> > m_base_prestige_goods;//�����̵���Ʒ

    std::map<int, boost::shared_ptr<baseUpgradeEquiptData> > m_base_upgrade_equipt_data;

    //�����߳��õĵ�ǰ����
    std::list<admin_notice> m_currentAdminNotices;

    //������߳��õ�
    std::list<admin_notice> m_adminNotices;

    std::vector<std::string> m_first_name_list_male;
    std::vector<std::string> m_second_name_list_male;

    std::vector<std::string> m_first_name_list_female;
    std::vector<std::string> m_second_name_list_female;
};

//��ɫ��¼��Ϣ
struct CharactorInfo
{
    int m_cid;
    int m_level;
    int m_spic;
    time_t m_lastlogin;
    std::string m_name;
    static volatile uint64_t _refs;
    CharactorInfo()
    {
        ++CharactorInfo::_refs;
    }
    CharactorInfo(const CharactorInfo& c);

    ~CharactorInfo()
    {
        --CharactorInfo::_refs;
    }
};

struct OnlineCharactor
{
    OnlineCharactor(boost::shared_ptr<OnlineUser> account, int id);
    ~OnlineCharactor();
    //������Ϣ
    int Send(const std::string&);
    //��������
    int WorldChat(const std::string& msg, bool needgold);
    //��Ӫ����
    int CampChat(const std::string& msg);
    //��������
    int GuildChat(const std::string& msg);
    //˽��
    int Tell(const std::string& to, const std::string& what, boost::shared_ptr<OnlineCharactor>& toChar);
    //pk���뷿������
    int PKRoomChat(const std::string& msg);

    //��������
    int onHeartBeat();

    boost::shared_ptr<CharData> m_charactor;

    boost::shared_ptr<OnlineUser> m_account;                      //�����˺�

    net::session_ptr m_sockethandle;

    int m_cid;              //��ɫid
    time_t m_gag_end_time;    //���Խ���ʱ��

    static volatile uint64_t _refs;

};

//������ɫ
int CreateChar(const std::string& account, int union_id, const std::string& server_id, const std::string& qid, int race, int spic, const std::string& name, uint64_t& cid);

//ɾ����ɫ
int DeleteChar(uint64_t cid);

//�������
void accountOffline(const std::string& account);

int giveLoots(CharData* cdata, std::list<Item>& getItems, chessCombat* pCombat, json_spirit::Object* robj, bool isAttacker, int loot_type);
int giveLoots(boost::shared_ptr<CharData>& cdata, chessCombat* pCombat, bool isAttacker, int loot_type);

//����ֵ
int ProcessCheckRecharge(json_spirit::mObject& o);
int ProcessCheckPack(json_spirit::mObject& o);


//�ӿ�
//��ֵ��Ϣ
int ProcessQueryRecharge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������
int ProcessSetGuideState(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ı���Һ�������
int ProcessCharChatChange(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//������ť��Ϣ
int ProcessTopButtonList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������
int ProcessQueryPrestigeAward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ��������
int ProcessGetPrestigeAward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����̵�
int ProcessQueryPrestigeShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����������Ʒ
int ProcessBuyPrestigeShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//�ϳɸ�����Ϣ
int ProcessShowEquipts(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��װ��
int ProcessEquipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ж��װ��
int ProcessUnequipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ϳɸ�����Ϣ
int ProcessCompoundEquiptInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ϳ�װ��һ��װ��
int ProcessCompoundEquiptOneKey(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ϳ�װ��
int ProcessCompoundEquipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ǿ��������Ϣ
int ProcessUpgradeEquiptInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//ǿ��װ��
int ProcessUpgradeEquipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��Ӣ��ʹ�õ���
int ProcessHeroUseGem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//װ������
int ProcessAddEquiptBaoshiSlot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʯ��Ϣ
int ProcessQueryBaoshiInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����ʯ�б�
int ProcessQueryBaoshiList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��Ƕ��ʯ
int ProcessInlayBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�Ƴ���ʯ
int ProcessRemoveBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�ϳɱ�ʯ
int ProcessCombineBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessCombineAllBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

#endif

