
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
//普通的
    char_data_normal_tmp = 1,
    char_data_normal_smelt_hero_refresh = 2,
    char_data_normal_arena_maxrank = 3,
    char_data_normal_arena_maxwin = 4,
    char_data_normal_sign_score = 5,
    char_data_normal_stronghold = 6,
    char_data_normal_copy = 7,
    char_data_normal_shenling = 8,
    char_data_normal_current_guide = 9,
    char_data_normal_first_recharge_event1 = 10,//首充活动1
    char_data_normal_first_recharge_event2 = 11,//首充活动2
    char_data_normal_help_times = 12,//新手帮助筹码次数
    char_data_normal_first_refresh_daily_score = 13,//新手第一次刷新每日必做
    char_data_normal_first_refresh_treasure = 14,//新手刷红色宝箱
    char_data_normal_lottery_score = 15,
    char_data_normal_lottery_get_recharge_score = 16,
    char_data_normal_lottery_total_recharge_score = 17,
    char_data_normal_tmp_vip = 18,

    char_data_normal_copy_reward_start = 800,//副本地图通关奖励领取标记开始
    char_data_normal_copy_reward_end = 899,//副本地图通关奖励领取标记结束
    char_data_normal_copy_perfect_reward_start = 900,//副本地图完美通关奖励领取标记开始
    char_data_normal_copy_perfect_reward_end = 999,//副本地图完美通关奖励领取标记结束
    char_data_normal_stage_reward_start = 1000,//通关奖励领取标记开始
    char_data_normal_stage_reward_end = 2000,//通关奖励领取标记结束
    char_data_normal_qq_yellow_special = 2001,//QQ黄钻特权礼包领取
    char_data_normal_qq_yellow_newbie = 2002,//QQ黄钻新手礼包领取
    char_data_normal_qq_yellow_level_libao = 2003,//QQ黄钻等级礼包领取 - 保留500级
    char_data_normal_qq_yellow_level_libao_end = 2499,
    char_data_normal_goal_level_reward_start = 2500,//目标系统等级礼包
    char_data_normal_goal_level_reward_end = 3000,

//每日的 10000 起
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
    char_data_daily_arena_reward_start = 11008,//竞技场积分商城领取情况
    char_data_daily_arena_reward_end = 11200,//竞技场积分商城领取情况
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
    char_data_daily_congratulation_received = 11218,    //今天收到的祝贺数
    char_data_daily_congratulation_sended = 11219,        //今天发出的祝贺数

    char_data_daily_findback = 14001,//找回标记
    char_data_daily_findback_start = 14002,//找回记录
    char_data_daily_findback_end = char_data_daily_findback_start + 50,

    char_data_daily_guild_moshen = 14100,
    char_data_daily_guild_moshen_start = 14101,
    char_data_daily_guild_moshen_end = char_data_daily_guild_moshen_start + 99,

    char_data_daily_qq_yellow_libao = 15001,//QQ黄钻每日礼包领取
    char_data_daily_qq_year_yellow_libao = 15002,//QQ年费黄钻每日礼包领取

//每周的 20000 起
    char_data_week_tmp = 20000,
};

//界面顶栏按钮分类
enum top_button
{
#ifdef QQ_PLAT
    top_button_yellow = 0,//黄钻按钮
#endif
    top_button_daily = 1,//每日必做
    top_button_timeLimitAction = 2,//限时活动
    top_button_sign = 3,//签到
    top_button_treasure = 4,//藏宝图
    top_button_rechargeAction = 5,//充值活动
    top_button_daily_task = 6,//日常任务
    top_button_online = 7,//在线礼包
    top_button_first_recharge = 8,//首充活动
    top_button_goal = 9,//目标系统
    top_button_dailyAction = 10,//日常活动
    top_button_bank = 11,//银行
    top_button_lotteryAction = 12,//转盘活动
    top_button_pack = 13,//兑换码礼包入口
    top_button_weekRanking = 14,//周排行

    top_button_city_levy = 1001,//城堡征收
    top_button_city_recruit = 1002,//城堡
    top_button_smelt_refresh = 1003,//熔炼英雄刷新
    top_button_prestige_task = 1004,//声望任务
    top_button_find_back = 1005,//找回奖励

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
    guide_id_get_silver = 115,//引导新手筹码为0
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

//种族
struct baseRaceData
{
    int m_type;//1弓箭手梅花，2魔法师红桃，3炼金术士方块，4骑士黑桃
    std::string m_name;
    int m_attak_add[iMaxLevel];//攻击加成
    int m_defense_add[iMaxLevel];//防御加成
    int m_magic_add[iMaxLevel];//魔力加成
    int m_resident_add;//城堡居民加成
    int m_silver_add;//筹码产出加成
    int m_hp_add;//出战英雄生命加成
    int m_mag_add;//魔力值加成
    int m_compound_add;//合成成功率加成
    int getLevelAdd(int level, int& attack_add, int& defense_add, int& magic_add);
};

//等级数据
struct baseLevelData
{
    int m_level;//等级
    int m_need_char_exp;//升级所需玩家经验(历练)
    double m_reward_add;//战斗奖励加成
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

//声望数据
struct basePrestigeData
{
    int m_level;//等级
    int m_need_exp;//升级所需声望
};

//声望奖励数据
struct CharPrestigeAward
{
    int m_cid;
    int m_race;
    std::vector<int> m_state;//各项奖励领取情况
    void save();
};

//昵称
enum nick_enum
{
    nick_arena_start = 1,//竞技场排名昵称1-3
    nick_arena_end = 3,
    nick_pk_start = 4,//pk场排名昵称4-6
    nick_pk_end = 6,
    nick_weekRank_start = 7,
    nick_weekRank_end = 20,
};

struct nick
{
    //昵称列表
    std::list<int> m_nick_list;

    void add_nick(int n);

    void remove_nick(int n);

    bool check_nick(int n);

    std::string get_string();

    void init(const std::string& data);
};

struct baseUpgradeEquiptData
{
    int m_level;//等级
    int m_fail_per;//失败百分比
    int m_need_stone;//需要强化石
    int m_bless_max;//祝福值上限
    int m_bless_value;//祝福值生效值
    int m_bless_per;//祝福值生效百分比
};

//角色数据
struct CharData:public boost::enable_shared_from_this<CharData>
{
public:
    CharData(int cid, bool b_create = false);
    ~CharData();
    int Create();
    int Load();

    //用户角色名称
    const std::string& GetCharName();
    //用户角色id
    uint64_t GetCharId();
    //角色公会id
    int GetGuildId();
    //角色等级
    int levelup(int level);
    //各种增减
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
    int addEquipt(int id, int level = 1, int quality = 1);//增加角色装备
    int subEquipt(int id);
    int addGem(int id, int counts, int statistics_type = 0);//增加角色道具
    int subGem(int id, int counts, int statistics_type = 0);//消耗角色道具
    int addCurrency(int id, int counts);//增加角色资源道具
    int subCurrency(int id, int counts);//消耗角色资源道具
    int addLibao(int libao_id, int counts);
    int addBaoshi(int base_id, int level, int counts);
    int updateVip();

    int getRoleInfo(json_spirit::Object& charobj);
    int getRoleDetail(json_spirit::Object& robj);
    int getTopButtonList(json_spirit::Array& list);
    //通知客户端角色信息发生变化
    int NotifyCharData();
    int NotifyCharData_(net::session_ptr& sk);
    int NotifyCharTopButtonList();

    int sendObj(json_spirit::Object& obj);

    //心跳
    int HeartBeat();
    //获取今日在线时长(秒)
    int getTodayOnlineTime();
    //保存
    int Save();
    //设置金币消费有提示
    int enableNoConfirmGoldCost(int type, bool enable);
    bool getNoConfirmGoldCost(int type);

    //加载特殊字段
    void loadExtraData();
    //取角色字段值
    int queryExtraData(int type, int field);
    //设置角色字段值
    void setExtraData(int type, int field, int value);
    //清除角色字段值
    void clearExtraData(int type);

    //掉落限制管理
    void loadLootTimes();
    int getLootTimes(int loot_type, int loot_id);
    void setLootTimes(int loot_type, int loot_id, int value);
    void clearLootTimes();

    //获取创建角色的自然天数
    int queryCreateDays();
    //获取创建角色自然天后时间戳
    time_t queryCreateXDays(int day);

    //检查是否触发引导
    int checkGuide(int type, int param1, int param2);
    //检查是否触发引导
    int checkGuide(int id);
    time_t getGuideState(int id);
    void setGuideStateComplete(int id, int next_guide);
    //顶栏按钮
    void updateTopButton(int type, int active, int leftNums = 0, int extra1 = 0, int extra2 = 0, int extra3 = 0);
    void addTopButton(int type, int active, int leftNums = 0, int extra1 = 0, int extra2 = 0, int extra3 = 0);
    void removeTopButton(int type);

    //今天是否可享受找回
    bool canFindBack();

    //所有装备
    int ShowEquipts(json_spirit::Object& obj, json_spirit::mObject& o);
    int equipt(int hid, int slot, int eid);
    int unequipt(int hid, int slot);
    int useGem(int hid, int tid, int nums);
    //装备合成
    int CompoundEquiptInfo(json_spirit::Object& obj, json_spirit::mObject& o);
    int CompoundEquiptOneKey(json_spirit::Object& obj, json_spirit::mObject& o);
    int CompoundEquipt(json_spirit::Object& obj, json_spirit::mObject& o);
    //升级装备
    int UpgradeEquiptInfo(json_spirit::Object& obj, json_spirit::mObject& o);
    int UpgradeEquipt(json_spirit::Object& obj, json_spirit::mObject& o);

    //所有宝石
    int ShowBaoshis(json_spirit::Object& obj, json_spirit::mObject& o);
    //镶嵌宝石
    int inlayBaoshi(int bagSlot, int eid, int slot);
    //移除宝石
    int removeBaoshi(int eid, int slot);
    //合并宝石
    int CombineBaoshi(int base_id, int level, int nums, json_spirit::Object& robj);
    int CombineAllBaoshi(int tolevel, json_spirit::Object& robj);

public:
    //功能开启
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
    int m_id;   //角色id
    int m_level;//角色等级
    int m_spic; //角色头像
    int m_gender;//性别 1男 0女
    int m_vip;  //vip等级
    int m_real_vip;//真实vip
    int m_tmp_vip;//临时vip
    int m_race; //种族
    boost::shared_ptr<baseRaceData> m_race_data;//种族数据加成
    boost::shared_ptr<baseLevelData> m_level_data;//等级数据加成
    int m_prestige[4];//各族声望
    int m_prestige_level[4];//各族声望等级
    CharPrestigeAward m_prestige_award[4];
    int m_cur_mapid;//角色所在地区
    int m_cur_stageid;//角色所在地区
    int m_cur_strongholdid;//角色进度关卡
    std::string m_name;//角色名字
    std::string m_chat;//战斗喊话内容
    int m_union_id;
    std::string m_account;//账户
    std::string m_qid;//平台id
    std::string m_server_id;
    std::string m_ip_address;//ip地址
    time_t m_createTime;//角色创建时间
    time_t m_levelupTime;//角色升级时间
    time_t m_login_time;//角色登录时间

    int m_is_online;//是否在线

    //需要通知玩家的事情
    std::map<int, int> m_need_notify;

    //属性
    int m_total_attack;
    int m_total_defense;
    int m_total_hp;

    //按钮开放1开放0关闭
    uint8_t m_panel_tmp;//按钮

    bool m_can_world_chat;          //是否可以世界聊天
    bool m_can_chat;                //是否被禁言
    bool m_char_data_change;    //角色信息发生变化

    //金币消费确认
    int m_gold_cost_comfirm[iMaxGoldCostConfirm];

    //新手引导完成情况
    std::map<int,time_t> m_guide_completes;
    int m_current_guide;        //当前引导步骤

    //累计充值金币
    int m_total_recharge;
    //道具增加的VIP经验
    int m_vip_exp;

    //历练值
    int m_char_exp;
    //上次存盘时间
    time_t m_save_time;

    std::map<int, int> m_normal_extra_data;
    std::map<int, int> m_daily_extra_data;
    std::map<int, int> m_week_extra_data;
    //掉落次数
    std::map<std::pair<int, int>, int> m_loot_cnt;
#ifdef QQ_PLAT
    int m_qq_yellow_level;      //QQ黄钻等级
    int m_qq_yellow_year;       //是否QQ年费黄钻
    std::string m_iopenid;      //QQ平台推荐玩家
    std::string m_feedid;       //QQ平台传播id
    std::string m_login_str1;   //QQ平台登录信息
    std::string m_login_str2;   //QQ平台登录信息
#endif
    nick m_nick;
    void SaveNick();

    int m_silver_get_combat;//战斗中获得筹码(翻倍用)
    int m_double_times;//翻倍次数
private:
    int m_gold;     //金币
    int m_bind_gold;//绑定金币
    int m_silver;   //银币，筹码
    int m_bank_silver;//银行筹码
public:
    std::map<int, Currency> m_currencys;//货币类道具
    boost::shared_ptr<CharGuildData> m_guild_data;//公会数据
    boost::shared_ptr<CharGuildForeverData> m_guild_forever_data;//公会永久数据
    //战斗属性
    combatAttribute m_combat_attribute;
    //英雄列表
    CharTotalHeros m_heros;
    //技能
    CharTotalSkills m_skills;
    CharTotalMagics m_magics;
    //关卡进度
    CharTempoData m_tempo;
    //背包
    CharBag m_bag;
    //城外城池
    CharWildCitys m_wild_citys;
    //角色任务
    CharAllTasks m_tasks;
    //每日必做任务
    CharDailyScore m_score_tasks;
    //角色声望任务
    CharAllPrestigeTasks m_prestige_tasks;
    //限时增益加成
    CharBuffs m_Buffs;
};

//在线用户
class OnlineUser: public boost::enable_shared_from_this<OnlineUser>
{
public:
    OnlineUser();
    OnlineUser(const std::string& qid, const std::string& account, int union_id, const std::string& server_id, net::session_ptr h);
    ~OnlineUser();
    //用户注册id
    const std::string& GetAccount();
    //关闭连接
    int CloseConnect();

    //获得ip地址
    const std::string& GetRemoteAddress();
    net::session_ptr GetSocket()
    {
        return m_sockethandle;
    };

    //登录角色
    int Login(uint64_t cid);

    int Kickout(const std::string& reason);
    boost::shared_ptr<OnlineUser> getAccount();

    friend struct OnlineCharactor;
    friend class ActionWorker;

    std::string m_qid;            //平台qid
    std::string m_account;      //账号
    std::string m_server_id;    //平台server_id
    int m_union_id;    //合作商union_id
    std::string m_ipaddress;    //IP
    net::session_ptr m_sockethandle;
    int m_state;                // 0 请求帐号信息中 1 完成
    int m_isAdult;                /*    0    用户未填写实名制信息
                                1    用户填写过实名制信息，且大于18岁
                                2    用户填写过实名制信息，但是小于18岁)*/

    int m_qq_yellow_level;    //QQ黄钻等级
    int m_qq_yellow_year;        //是否QQ年费黄钻
#ifdef QQ_PLAT
    std::string m_iopenid;
    std::string m_feedid;
    std::string m_login_str1;    //登录信息
    std::string m_login_str2;    //登录信息
#endif
    time_t m_logintime;           // 登录时间
    boost::shared_ptr<OnlineCharactor> m_onlineCharactor;  //角色
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
    bool work(std::string &cmd);       // 在些完成实际任务.
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

//聊天频道
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
    //广播给指定等级以上玩家
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

//声望奖励
struct basePrestigeAward
{
    int id;
    int race;
    int need_prestige_level;
    Item m_item;
};

typedef std::map<int, boost::shared_ptr<basePrestigeAward> > PrestigeAwards;

//声望商店物品
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
    void addCharData(boost::shared_ptr<CharData> cdata);    //加入角色数据
    boost::shared_ptr<CharData> GetCharData(int cid);    //获取角色数据
    boost::shared_ptr<baseRaceData> GetBaseRace(int type);
    boost::shared_ptr<baseLevelData> GetLevelData(int level);
    boost::shared_ptr<baseGem> GetBaseGem(int tid);
    boost::shared_ptr<baseEquipment> GetBaseEquipment(int baseid);
    boost::shared_ptr<baseUpgradeEquiptData> GetUpgradeEquiptData(int level);
    boost::shared_ptr<baseBaoshi> GetBaseBaoshi(int baseid);
    //创建在线角色信息
    boost::shared_ptr<OnlineCharactor> CreateOnlineCharactor(boost::shared_ptr<OnlineUser> account, uint64_t cid);
    //在线角色信息
    boost::shared_ptr<OnlineCharactor> GetOnlineCharactor(const std::string& char_name);
    //获得在线挑战列表
    int GetOnlineChallengeList(const std::string& char_name, int level, json_spirit::Array& list);
    //获得道具列表
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
    //保存角色信息
    int SaveDb(int save_all);

    //系统消息广播
    int broadCastSysMsg(const std::string& msg, int type);
    //系统消息广播- level等级以上
    int broadCastSysMsg(const std::string& msg, int type, int level);

    //广播到全服在线玩家
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
    //删除角色时，移除角色数据
    int removeCharData(int cid);
    //每周更新
    int weekReset();
    //在线人数
    int getTotalOnline(bool record);
    //心跳
    int HeartBeat();

    //检查系统公告变化
    void checkAdminNotice(int type);
    int adminNoticeDeleted(int id);
    int adminNoticeNew(int id, const std::string& message);
    int adminNoticeChanged(int id, const std::string& message);
    int getAdminNotice(json_spirit::Array& notice_list);

    void shutdown();

    //获得角色id
    int GetCharId(const std::string& cname);

    void setHeroOwner(int hid, int cid);
    int getHeroOwner(int hid);
    void removeHeroOwner(int hid);

    int getInt(const std::string& field, int defaultv = 0);
    void setInt(const std::string& field, int value);

    std::string getStr(const std::string& field);
    void setStr(const std::string& field, const std::string& value);
    //随机名字
    std::string getRandomName(int gender);
    //每日重置
    int dailyUpdate();
    //每周重置
    int weekUpdate();
    //通知玩家信息
    int dailyOnlineChar();
    //获取声望基础信息
    boost::shared_ptr<basePrestigeData> GetPrestigeData(int level);
    //获取某声望奖励列表
    boost::shared_ptr<PrestigeAwards> GetBasePrestigeAward(int race);
    //获取某声望商店列表
    boost::shared_ptr<PrestigeGoods> GetBasePrestigeGoods(int race);

    //好友推送
    bool canRecommendFriends(CharData* cdata);
    void getRecommendFriends(CharData* cdata, std::map<int, boost::shared_ptr<CharData> >& char_list);
private:
    static GeneralDataMgr* m_handle;
    time_t server_open_time;//开服时间

    rwlock guild_chanel_rwmutex;
    rwlock camp_chanel_rwmutex;
    rwlock onlineuser_rwmutex;

    rwlock onlinechar_rwmutex;
    rwlock globalchar_rwmutex;

    //频道
    boost::shared_ptr<ChatChannel> m_world_channel;     //世界聊天
    boost::shared_ptr<ChatChannel> m_horn_channel;     //喇叭聊天
    boost::unordered_map<int, boost::shared_ptr<ChatChannel> > m_camp_channels;         //阵营聊天
    boost::unordered_map<uint64_t, boost::shared_ptr<ChatChannel> > m_guild_channels;   //公会聊天

    boost::unordered_map<std::string, boost::shared_ptr<OnlineUser> > m_onlineuserlist;

    std::map<int, boost::shared_ptr<CharData> > m_chardata_map;             //全服角色数据
    std::map<std::string, int> m_charid_map;       //全服角色id映射
    std::map<int, int> m_hero_map;    //英雄和角色对应表

    std::map<std::string, boost::shared_ptr<OnlineCharactor> > m_online_charactors; //在线角色

    volatile uint32_t m_charactor_id;
    volatile uint32_t m_hero_id;
    volatile uint32_t m_gem_id;
    volatile uint32_t m_equipt_id;
    volatile uint32_t m_baoshi_id;
    volatile uint64_t m_combat_id;

    bool m_inited;

    //基础数据
    std::map<int, int> m_base_exps; //基础经验数据
    std::map<int, boost::shared_ptr<baseRaceData> > m_base_races;    //基础种族数据
    std::map<int, boost::shared_ptr<baseLevelData> > m_base_levels;    //等级数据
    std::map<int, boost::shared_ptr<baseGem> > m_base_gems;         //基础道具数据
    std::map<int, boost::shared_ptr<baseEquipment> > m_base_equipments;     //基础装备数据
    std::map<int, boost::shared_ptr<baseBaoshi> > m_base_baoshis;     //基础宝石数据

    std::map<int, boost::shared_ptr<basePrestigeData> > m_base_prestiges;//声望数据
    std::map<int, boost::shared_ptr<PrestigeAwards> > m_base_prestige_awards;//声望奖励商品
    std::map<int, boost::shared_ptr<PrestigeGoods> > m_base_prestige_goods;//声望商店商品

    std::map<int, boost::shared_ptr<baseUpgradeEquiptData> > m_base_upgrade_equipt_data;

    //正常线程用的当前公告
    std::list<admin_notice> m_currentAdminNotices;

    //检测变更线程用的
    std::list<admin_notice> m_adminNotices;

    std::vector<std::string> m_first_name_list_male;
    std::vector<std::string> m_second_name_list_male;

    std::vector<std::string> m_first_name_list_female;
    std::vector<std::string> m_second_name_list_female;
};

//角色登录信息
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
    //发送信息
    int Send(const std::string&);
    //世界聊天
    int WorldChat(const std::string& msg, bool needgold);
    //阵营聊天
    int CampChat(const std::string& msg);
    //公会聊天
    int GuildChat(const std::string& msg);
    //私聊
    int Tell(const std::string& to, const std::string& what, boost::shared_ptr<OnlineCharactor>& toChar);
    //pk筹码房间聊天
    int PKRoomChat(const std::string& msg);

    //在线心跳
    int onHeartBeat();

    boost::shared_ptr<CharData> m_charactor;

    boost::shared_ptr<OnlineUser> m_account;                      //所属账号

    net::session_ptr m_sockethandle;

    int m_cid;              //角色id
    time_t m_gag_end_time;    //禁言结束时间

    static volatile uint64_t _refs;

};

//创建角色
int CreateChar(const std::string& account, int union_id, const std::string& server_id, const std::string& qid, int race, int spic, const std::string& name, uint64_t& cid);

//删除角色
int DeleteChar(uint64_t cid);

//玩家离线
void accountOffline(const std::string& account);

int giveLoots(CharData* cdata, std::list<Item>& getItems, chessCombat* pCombat, json_spirit::Object* robj, bool isAttacker, int loot_type);
int giveLoots(boost::shared_ptr<CharData>& cdata, chessCombat* pCombat, bool isAttacker, int loot_type);

//检查充值
int ProcessCheckRecharge(json_spirit::mObject& o);
int ProcessCheckPack(json_spirit::mObject& o);


//接口
//充值信息
int ProcessQueryRecharge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//新手引导
int ProcessSetGuideState(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//改变玩家喊话内容
int ProcessCharChatChange(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//顶栏按钮信息
int ProcessTopButtonList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//声望奖励
int ProcessQueryPrestigeAward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//领取声望奖励
int ProcessGetPrestigeAward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//声望商店
int ProcessQueryPrestigeShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//购买声望商品
int ProcessBuyPrestigeShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//合成概率信息
int ProcessShowEquipts(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//穿装备
int ProcessEquipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//卸下装备
int ProcessUnequipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//合成概率信息
int ProcessCompoundEquiptInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//合成装备一键装满
int ProcessCompoundEquiptOneKey(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//合成装备
int ProcessCompoundEquipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//强化概率信息
int ProcessUpgradeEquiptInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//强化装备
int ProcessUpgradeEquipt(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//对英雄使用道具
int ProcessHeroUseGem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//装备开孔
int ProcessAddEquiptBaoshiSlot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//宝石信息
int ProcessQueryBaoshiInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//请求宝石列表
int ProcessQueryBaoshiList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//镶嵌宝石
int ProcessInlayBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//移除宝石
int ProcessRemoveBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//合成宝石
int ProcessCombineBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessCombineAllBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

#endif

