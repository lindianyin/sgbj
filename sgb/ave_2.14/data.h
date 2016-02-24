
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
#include "combat_def.h"
#include "state.h"
#include "skill.h"
#include "combat_attr.h"
#include "loot.h"
#include "farm.h"
#include "task.h"
#include "general_train.h"
//#include "smelt.h"
#include "corps.h"
#include "spls_errcode.h"

#include "new_weapon.h"
#include "horse.h"
#include "training.h"
#include "genius.h"

#include "jobqueue.hpp"
#include "worker.hpp"

#include "bag.h"
#include "shop.h"
#include "utils_all.h"
#include "buff.h"
#include "char_jxl.h"

//typedef boost::shared_mutex rwmutex;
//typedef boost::shared_lock<rwmutex> readLock;
//typedef boost::unique_lock<rwmutex> writeLock;

const int iONE_DAY_SECS = 60*60*24;

struct base_general_soul;

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

const std::string strGuidemsg = "{\"cmd\":\"currentGuide\",\"id\":$D,\"state\":$S,\"s\":200}";

struct CharBackpack;
struct CharData;
struct baseStage;
struct StrongholdData;
struct BaseSoldierData;
struct GeneralTypeData;
class ChatChannel;
class OnlineUser;

struct baowuBaoshi;
struct lootPlaceInfo;

int getSessionChar(net::session_ptr& psession, boost::shared_ptr<CharData>& cdata);
int getSessionChar(net::session_ptr& psession, CharData* &pc);

enum notify_msg_type
{
    notify_msg_new_weapon = 1,
    notify_msg_new_equipment = 2,
    notify_msg_new_general_limit = 3,
    notify_msg_new_present = 4,
    notify_msg_new_zhen = 5,
    notify_msg_more_up_zhen = 6,
    notify_msg_more_xiangqian = 7,
    notify_msg_recv_congratulation = 8,
    notify_msg_new_congratulation = 9,
    notify_msg_new_player_enhance = 10,
    notify_msg_new_player_sweep = 11,
    notify_msg_new_player_end = 12,
    notify_msg_new_get = 13,
    notify_msg_supply = 14,
    notify_msg_jxl = 15
};

enum enum_gold_cost_type
{
    gold_cost_buy_stronghold = 1,
    gold_cost_rest
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
    char_data_first_wusong = 1,        //已经免费首次招募武松
    char_data_explore_num,            //探索次数-2次以上不管了
    char_data_welfare,                //活动福利领取情况
    char_data_chenmi_time,            //沉迷时间
    char_data_continue_days,            //连续登录天数
    char_data_daily_task_pos,        //日常任务id
    char_data_lottery_total_score,    //梅花易数总积分
    char_data_first_luzhishen,        //已经免费首次招募鲁智深
    char_data_first_shijin,        //已经免费首次招募史进
    char_data_first_yangzhi,        //已经免费首次招募杨志
    char_data_race_maxrank,        //竞技曾经到达的最高排名
    char_data_first_recharge_gift,//首充礼包状态 1 可以领取，2 已经领取
    char_data_wash_start_time,    //洗髓功能开放时间
    char_data_current_guide,        //当前引导步骤
    char_data_shop_refresh_time,//商店刷新时间
    char_data_change_spic,        //变身形象
    char_data_change_spic_time,    //变身结束时间
    char_data_get_onlinegift_day,    //领取在线奖励截至timestamp
    char_data_get_collect_reward,    //领取收藏礼包
    char_data_get_continue_login_day,    //连续登录礼包奖励截至timestamp
    char_data_stage_award_start    = 1000,    //通关奖励领取标记开始，33张图的容量
    char_data_stage_award_end     = 1200,    //通关奖励领取标记结束

    char_data_first_general_quality1,    //第一个绿将
    char_data_first_general_quality2,    //第一个蓝将
    char_data_first_general_quality3,    //第一个紫将
    char_data_first_general_quality4,    //第一个橙将
    char_data_first_general_quality5,    //第一个x将

    char_data_first_race_success,        //首场竞技胜利

    char_data_qq_yellow_special,            //QQ黄钻特权礼包领取
    char_data_qq_yellow_newbie,            //QQ黄钻新手礼包领取
    char_data_qq_yellow_level_libao,        //QQ黄钻等级礼包领取 - 保留200级
    char_data_qq_yellow_level_libao_end = char_data_qq_yellow_level_libao + 200,

    char_data_seven_goals_small_start,    //七天目标各明细完成度
    char_data_seven_goals_small_end = char_data_seven_goals_small_start + 50,
    char_data_seven_goals_start,    //七天目标主线完成度
    char_data_seven_goals_end = char_data_seven_goals_start + 15,

    char_data_vip_special_libao,        //VIP专属武将礼包
    char_data_new_player_end,        //新手福利过期提醒
    char_data_wash_per,        //洗髓成功率加成(取值/1000)
    char_data_vip8_general,    //V8武将
    char_data_trade_refresh_num,    //刷新贸易商人次数，仅统计前2次引导用
    char_data_first_sign_info,    //第一次打开签到界面
    char_data_zhen_attack,                //战力
    char_data_zst_map_award_start,    //战神台通关地图奖励领取
    char_data_zst_map_award_end = char_data_zst_map_award_start + 50,

    char_data_view_bank,    //看过钱庄
    char_data_view_ranking,        //看过周排行
    char_data_view_boss,            //看过战神兽
    char_data_view_camprace,            //看过阵营战
    char_data_view_seven,            //看过七日目标
    char_data_vip10_general,    //V10武将
    char_data_event_train_horse_cnt,//战马训练活动计数
    char_data_fb_get,//facebook

//每日的 10000 起
    char_data_gold_rest = 10000,    //金币休息次数
    char_data_refesh_explore,    //刷新探索次数
    char_data_get_salary,            //领取俸禄
    char_data_buy_xisui_time,    //购买洗髓次数
    char_data_levy_time,            //征收次数
    char_data_jisi_time,            //祭祀次数
    char_data_yanhui_time,        //宴会次数
    char_data_train_horse,        //免费培养战马次数
    char_data_daily_task,            //日常任务完成次数
    char_data_free_reborn,        //VIP免费重生
    char_data_camp_reward,        //弱势阵营每日奖励
    char_data_camp_revolt,        //弱势阵营叛逃次数
    char_data_buy_explore,        //购买探索次数
    char_data_buy_race,            //购买竞技次数
    char_data_trade_time,            //通商次数
    char_data_randomservant,        //购买随机抓捕次数
    char_data_buy_randomservant,    //购买随机抓捕次数
    char_data_online_gift,        //在线宝箱第几个
    char_data_online_gift_state,    //在线宝箱状态1可以领取
    char_data_daily_task_reward,    //日常任务奖励领取次数
    char_data_super_wash_times,    //至尊洗髓次数
    char_data_wash_event,            //洗髓活动5次
    char_data_wash_event_end = char_data_wash_event + 10,    //洗髓活动结束
    char_data_shop_refresh,        //刷新商店次数
    char_data_book_refresh,        //刷新兵书次数
    char_data_test_recharge,        //测试充值次数
    char_data_horse_gold_train,    //每日战马金币培养次数
    char_data_horse_silver_train,//每日战马普通培养次数
    char_data_daily_task_start,    //助手中各日常任务
    char_data_daily_task_end = char_data_daily_task_start + 50,    //助手中各日常任务
    char_data_daily_task_reward_start,    //日常任务各奖励领取情况
    char_data_daily_task_reward_end = char_data_daily_task_reward_start + 50,    //日常任务各奖励领取情况

    char_data_daily_reserved = char_data_daily_task_reward_end + 10,    //保留
    char_data_daily_congratulation_received,    //今天收到的祝贺数
    char_data_daily_congratulation_sended,        //今天发出的祝贺数

    char_data_farm_seed,    //屯田次数
    char_data_farm_water,    //屯田浇水次数
    char_data_farm_water_cd,    //屯田浇水冷却时间
    char_data_farm_friend_water,    //屯田给好友浇水次数

    char_data_daily_trade_abandon,//每天贸易放弃商人次数

    char_data_daily_corps_explore,//军团探索次数

    char_data_daily_corps_ymsj,    //辕门射戟次数

    char_data_daily_findback,//助手找回标记
    char_data_daily_findback_task_start,//助手中各日常任务找回记录
    char_data_daily_findback_task_end = char_data_daily_findback_task_start + 50,

    char_data_daily_maze_times,    //迷宫次数

    char_data_daily_tencent_bag,    //腾讯统计背包物品
    char_data_daily_qq_yellow_libao,            //QQ黄钻每日礼包领取
    char_data_daily_qq_year_yellow_libao,    //QQ年费黄钻每日礼包领取

    char_data_daily_view_first_recharge,    //看过首冲礼包
    char_data_daily_view_vip_benefit,        //看过VIP特权
    char_data_daily_view_new_event,            //看过精彩活动按钮
    char_data_daily_vip_libao,                //VIP每日礼包领取

    char_data_get_yushi,        //免费领取玉石次数
    char_data_yushi_time_cd,    //下次领取玉石时间

    char_data_daily_view_feedback,    //查看过消费有礼

    char_data_daily_view_invite,        //看过好友邀请

    char_data_daily_buy_souls_daoju1,        //购买兵魂碎片次数
    char_data_daily_buy_souls_daoju2,        //购买兵魂碎片次数
    char_data_daily_buy_souls_daoju3,        //购买兵魂碎片次数

    char_data_daily_con,        //参拜次数

    char_data_daily_zst_challenge,        //战神台挑战次数
    char_data_daily_zst_buy_challenge,  //战神台购买挑战次数
    char_data_daily_zst_refresh,        //战神台重置星级次数

    char_data_daily_login_event,//登录活动每日检查情况


//每周的 20000 起
    char_data_extra_lottery_score = 20000,    //梅花易数每周积分
    char_data_extra_daily_task_star,            //日常任务星级
    char_data_week_vip_libao,                //VIP周福利
    char_data_extra_prestige_get,                //每周声望获得
};

//状态星斗最高等级20
const int iMaxStateStarLevel = 20;

//星星升级消耗规则
const int iStarLevelupCost[iMaxStateStarLevel] =
{
    10000,
    15000,
    20000,
    25000,
    50000,
    200000,
    250000,
    300000,
    400000,
    500000,
    750000,
    1000000,
    1500000,
    2000000,
    2500000,
    3000000,
    3500000,
    4000000,
    4500000,
    5000000
};

enum equip_slot_enum
{
    equip_ring = 1,
    equip_cloth = 2,
    equip_shield = 3,
    equip_fu = 4,
    equip_sword = 5,
    equip_necklace = 6,
    equip_slot_max = 6
};

struct baoshi_general
{
    int id;
    int level;
    int color;
    int spic;
    int baoshi_num;
    int baoshi_hole;
    std::string name;
};

//活动分类
enum action_enum
{
    action_active = 1,        //活动
    action_boss = 2,        //boss战
    action_camp_race = 3,    //阵营战
    action_guard = 4,        //护送
    action_race = 5,        //竞技场
    action_group_copy = 6,//多人副本
    action_trade = 7,        //通商
    action_chenmi = 8,        //防沉迷
    action_lottery = 9,    //梅花易数
    action_daily_task = 10//日常任务
};

//替身娃娃分类
enum baby_enum
{
    baby_boss_start = 1,        //boss战
    baby_boss_end = baby_boss_start + 5,
    baby_camp_race,    //阵营战
};

enum top_level_event
{
    top_level_event_helper = 1,    //游戏助手
    top_level_event_daily = 2,    //日常活动:1多人副本，2战神兽，3阵营战
    top_level_event_elite = 3,    //精英副本
    top_level_event_race = 4,        //竞技场
    top_level_event_servant = 5,    //壮丁
    top_level_event_guard = 6,    //护送
    top_level_event_opening = 7,    //开服活动:1单笔充值 2累计充值 3等级礼包 4武将活动 5宝石活动
    top_level_event_present = 8,    //礼包:1登录礼包、2首充礼包、3VIP礼包
    top_level_event_salary = 9,    //俸禄
    top_level_event_chenmi = 10,    //防沉迷
    top_level_event_rankings = 11,    //周排行
    top_level_event_rankings_reward = 12,    //排行奖励
    top_level_event_bank = 13,    //钱庄
    top_level_event_corp = 14,    //军团
    top_level_event_maze = 15,    //八卦阵
    top_level_event_first_recharge = 16,    //首充礼包
    top_level_event_vip_present = 17,    //VIP礼包
    top_level_event_seven_goals = 18,    //七日目标
    top_level_event_sign = 19,            //签到

    top_level_event_reward_boss = 20,    //战神兽奖励
    top_level_event_reward_boss_kill = 21,    //战神兽奖励
    top_level_event_reward_explore = 22,//探索奖励
    top_level_event_reward_race = 23,    //竞技场奖励
    top_level_event_reward_yanhui = 24,        //军团宴会奖励
    top_level_event_feedback = 25,        //消费有礼
    top_level_event_invite = 26,        //邀请好友
    top_level_event_daily_recharge = 27,    //日冲活动
    top_level_event_jxl = 28,        //将星录
    top_level_event_throne = 29,        //皇座
    top_level_event_jtz_awards = 30,    //军团战奖励
    top_level_event_jt_boss_kill = 31,   //军团boss
    top_level_event_zst = 32,        //战神台
    top_level_event_chengzhang = 33,        //成长礼包
    top_level_event_lottery_event = 34, //抽奖活动

    top_level_event_facebook = 35,
};

struct stand_in_get
{
    int cid;
    int enable;
    int payed;
    int type;
    int prestige;
    int silver;
    void save();
};

struct stand_in_mob
{
    stand_in_mob(int type)
    {
        m_type = type;
        load();
    }
    void processGold();
    void processReward(int prestige_fac, int attack_fac);
    //查询替身娃娃设置状态
    void getStandIn(int cid, int& enable, int& silver, int& prestige);
    //设置替身娃娃
    void setStandIn(int cid, int enable);

    void load();

    int m_type;
    std::map<int, stand_in_get> m_stand_ins;            //替身娃娃
};

//状态数据
struct StateData
{
    int m_state1;
    int m_state2;
    int m_state3;
};

//洗髓加点对应概率
struct baseWashPer
{
    int add;
    int per[3];//加点对应概率0超级1银币2金币
};

//基础装备数据
struct baseEquipment
{
    int baseid;        //装备基础id
    int type;        //类型 1 武器  2 盾牌 3、扇子 4、道袍 5、兵符
    int slot;
    int quality;    //品质 1 蓝色 2、绿色 3、黄色 4、红色 5、紫色
    int up_quality;    //强化品质
    int baseValue;    //基础属性值
    int baseValue2;//基础属性值
    int basePrice;    //基础出售价格
    int needLevel;    //装备需要等级
    std::string name;    //装备名
    std::string desc;    //装备描述

    std::string link_name;

    boost::shared_ptr<lootPlaceInfo> m_place;
    boost::shared_ptr<baseEquipment> m_next;
};

//装备数据
struct EquipmentData : public iItem
{
    EquipmentData(uint16_t type_, int id);
    int id;            //唯一id
    int type;        //类型 1 武器  2 盾牌 3、扇子 4、道袍 5、兵符
    int baseid;        //基础id
    //int slot;        //装备的位置
    int quality;    //品质
    int up_quality;//强化品质
    int cid;        //角色
    int qLevel;        //强化等级
    int value;        //属性值
    int value2;

    int addValue;    //增加的属性点
    int addValue2;

    int price;        //价格

    boost::shared_ptr<baseEquipment> baseEq;

    int getvalue() const {return value + addValue;}
    int getvalue2() const {return value2 + addValue2;}
    void Save();
    uint16_t getSpic() const {return baseid;}

    void toObj(json_spirit::Object& obj);
    int upgrade(bool cost_gold);

    virtual int32_t sellPrice() const;
    virtual std::string name() const;
    virtual std::string memo() const;
    virtual int getQuality() const {return quality;}

};

//角色所有装备
struct CharEquipments
{
    //boost::shared_ptr<EquipmentData> m_equiped[5];    //装备在身上的
    std::list<boost::shared_ptr<EquipmentData> > m_selled_equips;    //回购的
    int m_equipment_counts;

    std::map<int, boost::shared_ptr<EquipmentData> > equipments;
    int list(int page, int nums_per_page, int max_pages);        //显示玩家包裹里面的装备

    //装备强化
    int levelUp(int eid);
    //卖装备
    int sellEquipment(int eid, int& price);
    //回购装备
    int buybackEquipment(int id, int& price);
    //显示回购
    int listBuyBack(int page, int nums_per_page, json_spirit::Object& o);
    int addToSelled(boost::shared_ptr<EquipmentData> eq);
    int removeFromSelled(boost::shared_ptr<EquipmentData> eq);
    int load(int cid);
    boost::shared_ptr<EquipmentData> getEquipById(int id);

    bool haveEquip(int type);
    //某样装备的最高等级
    int maxEquipLevel(int type);
    int update_count();

    CharBackpack& m_backpack;

    CharEquipments(CharBackpack& p)
    :m_backpack(p)
    {
        m_equipment_counts = 0;
    };
};

struct baseTreasure
{
    bool currency;

    int id;
    int spic;
    int usage;        //用途 8表示装备制作卷轴
    int value;        //     用途8时表示制作的装备id
    int quality;

    int sellPrice;

    int gold_to_buy;    //金币购买价格

    int max_size;
    int invalidTime;    //道具失效时间
    std::string name;
    std::string memo;

    json_spirit::Array canMake;
    bool canMakeInited;

    bool b_used_for_task;

    boost::shared_ptr<lootPlaceInfo> m_place;
};

struct Treasure
{
    int id;
    int usage;
    int quality;
    int nums;
    int sellPrice;

    Treasure()
    {
        id = 0;
        usage = 0;
        quality = 0;
        nums = 0;
        sellPrice = 1000;
    }
};

//角色背包
struct CharBackpack
{
    CharData& m_chardata;
    int m_max_size;        //仓库大小,不算强化石头
    CharEquipments equipments;
    std::map<int, Treasure> treasures;
    int listBackpack(int page, int nums_per_page, int& max_pages, json_spirit::Array& elists, json_spirit::Array& tlists);
    int listWielded(json_spirit::Array& robj);
    int queryEquipment(int eid);
    Treasure* getTreasure(int id);
    int getTreasureNum(int id);

    int load();
    int Save();
    CharBackpack(CharData& charD);
    //显示传承装备列表
    int listInheritEquipt(int type, int id, int page, int nums_per_page, int& max_page, json_spirit::Array& elists);
    //bool b_changed;
};

//基础官职
struct baseoffical
{
    int m_id;
    std::string m_name;
    int need_prestige;
    int m_salary;
    int m_sid;//奖励的技能id
    int m_fid;//开启的屯田pos
};

//基础官职技能
struct baseofficalskill
{
    int m_sid;
    int m_spic;
    int m_type;//加成类型
    int m_add_per_level;
    std::string m_effect;
    std::string m_name;
    std::string m_memo;
};

//角色官职武将
struct officalgenerals
{
    int m_gid;
    int m_price;
    int m_sid;
    int m_spic;
    int m_quality;
    int need_offical;    //需要的官职
    int need_slevel;    //需要的关卡进度等级
    int m_good_at;        //擅长
    bool m_special;    //特殊招募武将
    std::string m_name;
};

//基础阵形
struct BaseZhenData
{
    int m_type;         //阵型id
    std::string m_name; //阵型名
    int m_open_pos[5];
};

//阵型数据
struct ZhenData
{
    int m_cid;          //角色id
    int m_zhen_type;    //阵型
    int m_level;

    int m_generals[9];  //各位置的武将
    int m_org_attack;     //初始戰力，除去技能效果
    //新战力系统
    int m_general_score;//武将评分
    int m_general_power;//武将战力
    int m_equip_score;//装备评分
    int m_equip_power;//装备战力
    int m_wash_score;//洗髓评分
    int m_wash_power;//洗髓战力
    int m_baoshi_score;//宝石评分
    int m_baoshi_power;//宝石战力
    int m_reborn_score;//重生评分
    int m_reborn_power;//重生战力
    int m_level_score;//等级评分

    int m_weapon_power;//秘法战力

    int m_soul_power;    //兵魂战力

    int m_jxl_power;    //将星录战力

    int m_gsoul_power;  //将魂战力

    bool m_isnpc;       //决定存放的是StrongholdGeneralData还是CharGeneralData
    bool m_changed;     //有改动
    bool m_attack_change;//戰力變化

    std::string m_name;

    CharData& m_charData;

    ZhenData(int cid, CharData& c)
    :m_charData(c)
    {
        m_changed = false;
        m_cid = cid;
        m_zhen_type = 0;
        memset(m_generals, -1, sizeof(int)*9);
        m_isnpc = cid == 0;
        m_org_attack = 0;
        m_attack_change = true;
        //评分
        m_general_score = 0;
        m_equip_score = 0;
        m_wash_score = 0;
        m_baoshi_score = 0;
        m_reborn_score = 0;
        m_level_score = 0;
        //战力
        m_general_power = 0;
        m_equip_power = 0;
        m_wash_power = 0;
        m_baoshi_power = 0;
        m_reborn_power = 0;
        m_soul_power = 0;
        m_jxl_power = 0;
        m_gsoul_power = 0;
    }
    ZhenData(const ZhenData& z)
    :m_charData(z.m_charData)
    {
        m_changed = false;
        m_cid = z.m_cid;
        m_zhen_type = z.m_zhen_type;
        memcpy(m_generals, z.m_generals, sizeof(int)*9);
        m_isnpc = z.m_isnpc;
        m_org_attack = 0;
        m_attack_change = true;
        //评分
        m_general_score = 0;
        m_equip_score = 0;
        m_wash_score = 0;
        m_baoshi_score = 0;
        m_reborn_score = 0;
        m_level_score = 0;
        //战力
        m_general_power = 0;
        m_equip_power = 0;
        m_wash_power = 0;
        m_baoshi_power = 0;
        m_reborn_power = 0;
        m_soul_power = 0;
    }
    int Save();     //保存

    int updateAttack();    //更新戰力
    void updateNewAttack();

    int getAttack();

    void set_attack_change();

    void getList(json_spirit::Array& hlist);

    void getList2(json_spirit::Array& hlist);
    int getGeneralCounts();
};

//玩家阵型
struct CharZhens
{
    CharData& m_charData;
    std::map<int, boost::shared_ptr<ZhenData> > m_zhens;
    int m_default_zhen;
    int m_cid;      //角色id

    CharZhens(int cid, CharData& c)
    :m_charData(c)
    {
        m_cid = cid;
    };
    int Load();
      int Swap(int zhenid, int pos1, int pos2);   //交换两个位置的武将
    int Up(int zhenid, int pos, int gid);        //武将上阵
    int Up(int zhenid, int gid);          //武将上阵,只要有空位就上

    int Down(int zhenid, int pos);              //武将下阵
    int Down(int gid);              //所有阵型中的该武将下阵,用于解雇武将时的处理

    bool Check(int gid);              //所有阵型中是否有该武将

    int SetDefault(int zhenid);                 //设置缺省阵型
    int GetDefault();                           //缺省阵型
    boost::shared_ptr<ZhenData> GetZhen(int zhenid);        //获取阵型信息
    int Save(int type = 0);
    int Levelup(int type, int level);
    void setLevel(int type, int level);

    void set_attack_change();

    int m_changed;          //有改动
    int m_changed_default;  //缺省阵型有改动
};

struct CharTotalGenerals;
struct base_genius;

struct baseChengzhangStars
{
    int id;
    double need_chengzhang;
    int gongji;
    int fangyu;
    int bingli;
};

struct baseWashStars
{
    int id;
    int need_score;
    int value;
};

//玩家武将数据
struct CharGeneralData: public boost::enable_shared_from_this<CharGeneralData>
{
    int m_id;       //唯一id
    int m_cid;      //所属角色id
    int m_gid;      //武将id
    int m_spic;     //头像类别
    int m_stype;    //兵种id
    int m_level;    //等级
    int m_str;      //勇武
    int m_int;      //智力
    int m_tongyu;   //统御
    int m_color;    //武将颜色

    double m_chengzhang;        //成长系数
    boost::shared_ptr<baseChengzhangStars> m_chengzhang_star;
    boost::shared_ptr<baseChengzhangStars> m_chengzhang_next_star;
    double m_add;    //升级增加的属性三者同加
    int m_reborn_point;    //重生点数
    int m_wash_str;    //洗髓增加的勇
    int m_wash_int;    //洗髓增加的智
    int m_wash_tong;    //洗髓增加的统
    int m_tmp_wash_result[3];//洗髓之后没确认的数据暂时保存
    boost::shared_ptr<baseWashStars> m_wash_star;
    boost::shared_ptr<baseWashStars> m_wash_next_star;
    int m_state;        // 0 正常  1 回购中
    std::string m_baowu;    //宝物名字
    int m_baowu_level;        //宝物等级
    int m_baowu_type;        //宝物属性类型1勇2智3统
    int m_baowu_add;        //宝物加成属性数值
    time_t m_delete_time; //回购截止时间
    int m_reborn_times;        //武将重生次数
    int m_wash_times;    //武将洗髓次数
    std::string m_link_name;    //带链接的名字
    std::string m_color_link;    //带颜色的名字可链接

    //std::vector<boost::shared_ptr<baowuBaoshi> > m_baoshi;    //宝物宝石

    std::vector<int> m_genius;                    //天赋列表
    bool m_genius_lock[iGeniusMaxNum];        //天赋锁定情况
    int m_genius_count;    //天赋数量

    int b_nickname;//绰号是否激活0否1是

    //特技的作用
    combatAttribute m_combat_attr;
    std::list<combatSpeSkill> m_more_damage_skills;    //加伤害战斗特技
    std::list<combatSpeSkill> m_attack_skills;            //变化攻击方式的特技

    CharTotalGenerals& m_belong_to;

    boost::shared_ptr<BaseSoldierData> m_baseSoldier;
    boost::shared_ptr<GeneralTypeData> m_baseGeneral;

    //boost::shared_ptr<EquipmentData> m_equiped[equip_slot_max];

    Bag m_equipments;

    Bag m_baoshis;

    int m_attack;
    int m_pufang;
    int m_cefang;
    int m_hp;

    //新战力系统
    bool general_change;
    bool equip_change;
    bool wash_change;
    bool baoshi_change;
    bool reborn_change;
    int m_general_score;//武将评分
    int m_general_power;//武将战力
    int m_equip_score;  //装备评分
    int m_equip_power;  //装备战力
    int m_wash_power;   //洗髓战力
    int m_baoshi_power; //宝石战力
    int m_reborn_power; //重生战力
    int m_soul_power;   //兵魂战力
    int m_jxl_power;    //将星录战力

    int m_gsoul_power;  //将魂战力
    base_general_soul*  m_general_soul;
    //更新战力数值
    void updateNewAttack();
    int getAttack();

    CharGeneralData(CharData& c, CharTotalGenerals& bl)
    :m_genius(iGeniusMaxNum,0)
    ,m_belong_to(bl)
    ,m_equipments(c, equip_slot_max)
    ,m_baoshis(c, 6)
    {
        m_attack = 0;
        m_pufang = 0;
        m_cefang = 0;
        m_hp = 0;
        m_tmp_wash_result[0] = 0;
        m_tmp_wash_result[1] = 0;
        m_tmp_wash_result[2] = 0;
        m_general_score = 0;
        m_general_power = 0;
        m_equip_score = 0;
        m_equip_power = 0;
        m_wash_power = 0;
        m_baoshi_power = 0;
        m_reborn_power = 0;
        m_jxl_power = 0;
        m_gsoul_power = 0;
        general_change = true;
        equip_change = true;
        wash_change = true;
        baoshi_change = true;
        reborn_change = true;
        m_general_soul = NULL;
    };
    int m_changed;  //有改动
    int Levelup(int level = 9999);  //升级
    int SetColor(bool update_color = false);//武将额外属性超过一点则开始显示颜色
    int Save();     //保存

    void toObj(json_spirit::Object& obj);
    //更新屬性
    void updateAttribute();
    void addToList(base_genius Genius, int type);
    //更新天赋加成
    void updateGeniusAttribute();
    //洗髓公告
    void broadWashMsg(int ret);
    //重生公告
    void broadRebornMsg(int ret);
    void updateEquipmentEffect();
    void updateBaoshiCount();

    //武将名字变成链接格式
    std::string NameToLink();

    std::string colorLink()
    {
        return m_color_link;
    }

    //移除所有宝石
    void removeAllBaoshi();

    //穿上
    int equip(int slot, int eid);
    //卸下
    int unequip(int slot);
    //移除所有装备
    int removeAllEquipment();

    //身上装备的简单列表
    void getList(json_spirit::Array& elist);

    //更新宝石对武将作用
    void updateBaoshiAttr();
    //更新重生星级
    void updateChengzhangStar();
    //更新洗髓星级
    void updateWashStar(bool act = false);
};

//玩家武将数据
struct CharTotalGenerals
{
    std::map<int, boost::shared_ptr<CharGeneralData> > m_generals;
    std::map<int, boost::shared_ptr<CharGeneralData> > m_fired_generals;
    CharData& m_charData;
    int m_cid;              //角色id

    CharTotalGenerals(int cid, CharData& cdata)
    :m_charData(cdata)
    {
        m_cid = cid;
        //Load();
    };
    //根据武将种类获得武将id
    int GetGeneralByType(int gtype);
    //根据武将基础类型查找武将
    int GetFiredGeneralByType(int gtype);

    //达到一定等级的武将数量
    int getGeneralCounts(int level);

    int Load();
    int Fire(int id);
    int Buyback(int id);
    int Reborn(int id, json_spirit::Object& robj, int fast = 0);
    int Recover(int id, json_spirit::Object& robj);
    int Add(int id, bool broad = true, int level = 1, bool setFac = false, double fac_aa = 0);
    bool CheckTreasureCanUp(int id);
    int UpdateTreasure(std::string& general_name, std::string& baowu_name, int id = 0);
    std::map<int, boost::shared_ptr<CharGeneralData> >& GetFiredGeneralsList();
    boost::shared_ptr<CharGeneralData> GetGenral(int id);
    int GetGenralLevel(int gtype);
    //删除武将
    int deleteGenral(int id);
    //修改武将属性
    int modifyGeneral(int id, int t, int z, int y);
    //清除所有洗髓加点
    int clearWash();
    //修改武将成长
    int modifyGeneralGrowth(int id, double fac1);
    EquipmentData* getEquipById(int id);

    int Save(); //保存
    bool m_changed; //有改动
};

//玩家关卡进度数据
struct CharStrongholdData
{
    boost::shared_ptr<StrongholdData> m_baseStronghold;

    int m_state;//关卡状态-2代表不存在，-1代表未激活，其他代表总攻击次数

    int m_cid;            //角色id

    //状态
    npcStrongholdStates m_states;

    //关卡状态的战斗属性
    combatAttribute m_combat_attribute;

    CharStrongholdData(int cid, int id, int level, int num)
    :m_cid(cid)
    ,m_states(cid, id, level, num)
    {};
    //获取现有状态
    int getStates(json_spirit::Array& states);

    void save_state();
};

//玩家场景进度数据
struct CharStageData
{
    int m_cid;
    int m_finished;//已经完成的关卡数

    boost::shared_ptr<baseStage> m_baseStage;
    std::list<boost::shared_ptr<Item> > m_fall_list;//场景掉落

    boost::shared_ptr<CharStrongholdData> m_stronghold[25];

    int m_cur_group;    //当前在打第几组

    bool isGroupPassed(int group);
    void groupPass(int group);
    void openGroup(int group);
    int curGroup();
};

struct CharGeneralData;
class OnlineUser;
struct OnlineCharactor;
struct CharactorInfo;

typedef std::map<int, boost::shared_ptr<CharGeneralData> > CharGeneralsData;
typedef std::map<int, boost::shared_ptr<CharStageData> > CharMapData;
typedef std::map<int, boost::shared_ptr<StateData> > StrongholdsStatesData;

//玩家进度数据
struct CharTempoData
{
    CharData& m_charData;
    std::map<int, boost::shared_ptr<CharMapData> > CharMapsData;
    int m_cid;
    CharTempoData(CharData& c, int cid)
    :m_charData(c)
    ,m_cid(cid)
    {
    };
    int load(int cid, int loadMap);
    //插入角色关卡进度
    int InitCharTempo(int mapid);
    int update(int stronghold, bool bBroad);
    int Save();
    int reset();
    int get_stage_finish_loot(int mapid, int stageid , json_spirit::Object& robj);
    bool check_stage_finish(int mapid, int stageid);
    bool check_stronghold_can_sweep(int mapid, int stageid, int pos);
};

//登陆宝箱
struct baseLoginPresent
{
    baseLoginPresent()
    {
        id = 0;
        spic = 0;
        quality = 0;
        ling = 0;
        silver = 0;
        treasure = 0;
        treasure_num = 0;
        name = "";
        memo = "";
    }
    int id;
    int spic;
    int quality;
    int ling;
    int silver;
    int treasure;
    int treasure_num;
    std::string name;
    std::string memo;
};

//连续登陆宝箱
struct CharLoginPresent
{
    int cid;
    int state;
    baseLoginPresent* present;
};

//VIP奖励[金币声望，两宝物一宝石]
struct baseVIPPresent
{
    baseVIPPresent()
    {
        vip = 0;
    }
    int vip;
    const json_spirit::Array& getArray() const;
    void updateObj();
    json_spirit::Array m_item_list;
    std::list<Item> m_list;        //奖励列表
};

//玩家VIP奖励领取情况
struct CharVIPPresent
{
    int cid;
    int state;
    boost::shared_ptr<baseVIPPresent> present;
};

//单次充值满奖励
struct baseRechargePresent
{
    baseRechargePresent()
    {
        id = 0;
        needgold = 0;
        prestige = 0;
        silver = 0;
        gold = 0;
        ling = 0;
        treasure = 0;
        treasure_num = 0;
        treasure1 = 0;
        treasure1_num = 0;
        treasure2 = 0;
        treasure2_num = 0;
        memo = "";
    }
    int id;
    int needgold;
    int prestige;
    int silver;
    int gold;
    int ling;
    int treasure;
    int treasure_num;
    int treasure1;
    int treasure1_num;
    int treasure2;
    int treasure2_num;
    std::string memo;
};

//GM自动发放奖励
struct CharGMPresent
{
    int gm_id;
    int gm_level;
    int gm_first;
    int gm_get_reward;
    CharGMPresent()
    {
        gm_id = 0;
        gm_level = 0;
        gm_first = 0;
        gm_get_reward = 0;
    }
};

const int iWashEventNum = 4;
const int iWashEvent[iWashEventNum][2] =
{
    {5,1},
    {15,2},
    {50,10},
    {100,25}
};

struct CharFriend
{
    int friend_id;
    int friend_state;//0待审核1正式
    int flag;//0初始1改变2删除
    boost::shared_ptr<CharData> f_charData;
    CharFriend(boost::shared_ptr<CharData> c, int cid)
    :f_charData(c)
    {
        friend_id = cid;
        friend_state = 0;
        flag = 0;
    }
};

//昵称
enum nick_enum
{
    nick_race_start = 1,//竞技场排名昵称1-5
    nick_race_end = 5,
    nick_ranking_start = 6,//周排行第一昵称
    nick_ranking_end = 20,
    nick_throne_start = 21,//至尊王座排名昵称
    nick_throne_end = 25,
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

//角色数据
struct CharData:public boost::enable_shared_from_this<CharData>
{
public:
    static volatile uint64_t _refs;
    static uint64_t refs();
    bool m_load_success;
    int m_id;   //角色id
    int m_state;//状态 0 正常 1 待删除
    time_t m_deleteTime;    //删除时间
    int m_level;            //角色等级
    time_t m_createTime;    //角色创建时间
    time_t m_levelupTime;    //角色升级时间

    int m_union_id;
    std::string m_account;    //账户
    std::string m_qid;    //平台id
    std::string m_server_id;
    std::string m_name;//角色名字
    int m_spic; //角色头像
    int m_gender;//性别 1男 0女
    int m_vip;  //vip等级
    int m_camp; //阵营1官府2绿林
    int m_currentStronghold;    //关卡进度
    std::string m_chat;//战斗喊话内容

    int m_area;         //角色所在地区
    std::string m_area_name;
    std::string m_area_memo;
    int m_cur_stage;         //角色所在场景

    int m_tmp_vip;    //临时VIP
    int m_tmp_vip_start_time;    //临时VIP开始时间
    int m_tmp_vip_end_time;    //临时VIP结束时间

    int m_qq_yellow_level;    //QQ黄钻等级
    int m_qq_yellow_year;        //是否QQ年费黄钻
#ifdef QQ_PLAT
    std::string m_iopenid;                //QQ平台推荐玩家
    std::string m_feedid;                //QQ平台传播id
    std::string m_login_str1;    //QQ平台登录信息
    std::string m_login_str2;    //QQ平台登录信息
#else
    std::string m_vcode;
    std::string m_sid;
#endif
    nick m_nick;
    void SaveNick();
    std::string getUserMsg();
private:
    //资源
    int m_gold;    //金币
    int m_silver;  //白银
    int m_ling;    //军令
    int m_explore_ling;//探索令

public:
    //按钮开放1开放0关闭
    uint8_t m_panel_zhuj;      //军事按钮 2
    uint8_t m_panel_army;      //部队按钮 3
    uint8_t m_panel_junt;      //军团按钮 1
    uint8_t m_panel_interior;    //内政按钮 4

    //功能开放1开放0限制
    uint8_t m_weaponOpen;    //兵器栏
    uint8_t m_eliteOpen;    //精英战役
    uint8_t m_raceOpen;    //竞技
    uint8_t m_skillOpen;    //技能
    uint8_t m_skillTrainOpen;//技能训练
    uint8_t m_baowuOpen;    //宝物系统开放(镶嵌)
    uint8_t m_equiptOpen;    //装备
    uint8_t m_equiptEnhanceOpen;//装备强化
    uint8_t m_zhenOpen;    //布阵
    uint8_t m_trainOpen;    //武将训练
    uint8_t m_farmOpen;    //屯田
    uint8_t m_exploreOpen;//探索
    uint8_t m_shopOpen;    //商店
    uint8_t m_tradeOpen;    //通商开放
    uint8_t m_washOpen;    //洗髓
    uint8_t m_recruitOpen;//招募武将
    uint8_t m_sweepOpen;    //扫荡精英
    uint8_t m_horseOpen;    //战马
    uint8_t m_servantOpen;//家丁
    uint8_t m_levyOpen;    //征收开放
    uint8_t m_guardOpen;    //护粮开放
    uint8_t m_officeOpen;    //官职开放
    uint8_t m_corpsOpen;    //军团开放
    uint8_t m_bossOpen;    //boss开放
    uint8_t m_helperOpen;    //助手开放
    uint8_t m_campraceOpen;//阵营战开放
    uint8_t m_rebornOpen;    //重生开放
    uint8_t m_buyLingOpen;    //购买军令开放
    uint8_t m_rankEventOpen;    //周排行活动开放
    uint8_t m_bankOpen;    //钱庄开放
    uint8_t m_soulOpen;    //练兵开放
    uint8_t m_sevenOpen;    //7日目标开放
    uint8_t m_jxlOpen;        //将星录开放
    uint8_t m_generalSoulOpen; //将魂开启

    time_t m_wash_open_time;    //洗髓开放时间
    int m_wash_event_state;    //洗髓活动状态
    time_t m_daily_wash_times;//每日至尊洗髓次数
    int m_wash_event[10];    //至尊洗髓活动领取情况

    time_t m_login_time;            //登录时间
    bool m_can_world_chat;          //是否可以世界聊天
    bool m_can_chat;                //是否被禁言

    bool m_check_chenmi;    //是否需要检查防沉迷
    uint64_t m_chenmi_time; //累计的沉迷时间秒数
    time_t m_notify_chenmi_time;    //通知沉迷信息的时间
    time_t m_chenmi_start_time;     //防沉迷开始时间

    time_t m_fight_cd;        //战斗冷却时间
    time_t m_enhance_cd;    //强化装备冷却时间
    bool m_can_enhance;        //是否可强化

    //今天已经金币休息的次数
    int m_gold_rest;
    //活动休息剩余次数
    int m_free_rest;
    time_t m_free_rest_time;

    //今天探索金币刷新次数
    int m_explore_refresh_times;
    //新手前2次探索
    int m_first_explore;
    int m_second_explore;

    //兵器+装备
    int m_total_pugong;
    int m_total_pufang;
    int m_total_cegong;
    int m_total_cefang;
    int m_total_bingli;

    bool m_weapon_attack_change;        //裝備和兵器變化

    //技能
    std::map<int, boost::shared_ptr<charSkill> > m_skill_list;
    int m_skill_power[5];        //技能对战斗数值的加成 兵力，普攻，普防，策攻，策防

    //技能研究队列
    std::vector<skillResearchQue> m_skill_queue;

    //技能研究者
    boost::shared_ptr<skillTeacher> m_teachers[skill_teacher_nums];
    bool m_teachers_change;    //有否变化

    //训练队列
    std::vector<generalTrainQue> m_train_queue;
    //兵书列表
    boost::shared_ptr<Book> m_book[general_book_nums];
    bool m_book_change;

    //战斗属性
    combatAttribute m_combat_attribute;

    //关卡进度
    CharTempoData m_tempo;
    //关卡状态
    StrongholdsStatesData m_strongholdStates;

    //角色背包
    //CharBackpack m_backpack;
    Bag m_bag;
    selledBag m_selled_bag;

    //武将列表
    CharTotalGenerals m_generals;

    //阵型
    CharZhens m_zhens;//所有阵型

    int m_hp_cost;        //拖欠的募兵消耗

    charTask m_task;    //角色任务
    charTrunkTasks m_trunk_tasks;    //支线任务
    void init_task_done();

    //官职相关
    int m_prestige;    //声望
    int m_offical;        //官职
    std::string m_offical_name;    //官职名
    int m_salary;//俸禄
    int m_hasgetsalary;//今天是否领取过俸禄0没领取1已经领取
    bool m_officalcanlevelup;//官职可否升级

    int m_buy_xisui_time;//购买洗髓次数，每日清
    int m_farm_harvest;//屯田收获次数，每四天清
    int m_levy_time;//征收次数每天清

    int m_gold_train_horse;//战马培养次数
    int m_silver_train_horse;//战马培养次数
    int m_welfare;//福利领取情况

    //金币消费确认
    int m_gold_cost_comfirm[iMaxGoldCostConfirm];

    //累计充值金币
    int m_total_recharge;
    //道具增加的VIP经验
    int m_vip_exp;

    boost::shared_ptr<corps_member> m_corps_member;    //军团成员
    std::list<boost::shared_ptr<corps_application> > m_corps_applications;    //军团申请

    //地区攻略领取情况
    std::map<int,int> m_map_intro_get;

    //新手引导完成情况
    std::map<int,time_t> m_guide_completes;
    int m_current_guide;        //当前引导步骤

    //新手冲锋号奖励情况 first=等级
    std::map<int,bool> m_newbie_reward;
    int m_newbie_reward_canGet;

    //成长礼包奖励情况
    std::vector<int> m_chengzhang_reward;

    //单笔充值奖励情况
    //std::map<int,int> m_recharge_reward;

#if 0
    //连续登录奖励
    std::map<int,CharLoginPresent> m_login_present;
#endif
    //连续登录(20级后开始统计)
    int m_continue_days;//用作登录奖励(领取完会重置)
    int m_total_continue_days;//真正的连续登录统计

    //VIP奖励领取情况
    std::map<int,CharVIPPresent> m_vip_present;

    int m_last_stronghold;    //上一个攻打的关卡id
    int m_reget_times;            //再次获得次数
    int m_reget_gold;            //再次获得需要的金币
    int m_last_stronghold_mapid;
    int m_last_stronghold_level;
    int m_last_stronghold_type;

    int m_copy_id;        //所在的副本id
    int m_copy_id_leave;    //离线时的副本id

    time_t m_test_recharge_time;    //下次可以测试充值的时间
    int m_total_test_recharge;    //累计测试充值次数
    //上次存盘时间
    time_t m_save_time;

    charShop m_shop;//玩家商店

    int m_temp_jisi_times;    //祭祀次数(军团) - 临时 -每日清空
    int m_temp_corps_yanhui;    //军团宴会次数 - 每日清空

    CharNewWeapons m_new_weapons;
    CharHorse m_horse;

    //CharTrainings m_training;//练兵系统

    int m_current_map;            //当前打开地图
    int m_current_stage;        //当前打开场景

    //通商信息
    int m_trade_state;
    int m_tradeque_type;
    int m_tradeque_id;
    int m_tradeque_pos;

    std::string m_ip_address;    //ip地址

    int m_temp_score;    //临时存帐号点数的

    std::map<int, boost::shared_ptr<baowuBaoshi> > m_baoshi;    //宝物宝石

    std::map<int, int> m_normal_extra_data;
    std::map<int, int> m_daily_extra_data;
    std::map<int, int> m_week_extra_data;

    newCharStates m_newStates;    //新状态

    CharBuffs m_Buffs;    //限时增益加成

    int m_general_limit;    //武将人数上限

    int m_enhance_state;    //强化队列状态
    int m_enhance_silver;    //强化装备需要的银币
    int m_enhance_eid;        //能够强化的装备id

    int m_upgrade_weapon_state;    //升级秘法状态
    int m_upgrade_weapon_gongxun;//升级秘法需要的功勋
    int m_upgrade_weapon_type;    //可以升级秘法的类型

    bool m_open_info_change;    //队列变化
    bool m_formation_change;    //下方阵型数据变化
    bool m_char_data_change;    //角色信息发生变化

    int m_change_spic;            //变身形象
    time_t m_change_spic_time;//变身结束时间

    int m_is_online;            //是否在线

    int m_up_generals;        //上阵人数

    std::map<int, boost::shared_ptr<CharData> > m_recommend_friends;
    time_t m_recommend_friend_refresh;

    int m_close_friend_to;    //谁的密友

    //需要通知玩家的事情
    std::map<int, int> m_need_notify;

    char_jxl_buff m_jxl_buff;

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
    //角色所在地区编号
    int GetArea();

    //武将信息
    CharTotalGenerals& GetGenerals();

    //阵型信息
    CharZhens& GetZhens();

    //进度信息
    CharTempoData& GetTempo();

    //获取当前军粮上限和需求
    void GetMaxSupply(int& need_supply, int& max_supply);

    //加入公会
    int JoinGuildId(uint64_t gid);
    //离开工会
    int LeaveGuild();

    //角色等级
    int level(int level);

    int addGold(int gold);      //增加减少都可以，返回-1表示不够减，正常返回变化后的数量
    int addLing(int ling);
    int addSilver(int silver, bool buy_back = false);
    int addPrestige(int prestige);
    int addExploreLing(int ling);

    int addGongxun(int a);
    int getGongxun();
    int addLibao(int libao_id, int counts);

    int resetExploreLing();

    int ling() {return m_ling;}
    int gold() {return m_gold;}
    int exploreLing() {return m_explore_ling;}
    int silver() {return m_silver;}
    int prestige() {return m_prestige;}

    //uint64_t exp() {return m_exp;}
    int silver(int);
    int ling(int);
    int gold(int);

    //首充活动
    int action_first_recharge(int gold_num);

    int getCharInfo(json_spirit::Object& charobj);
    //通知信息变化
    int getPanelInfo(json_spirit::Object& panelobj);
    int NotifyCharData();
    //通知客户端角色信息发生变化
    int NotifyCharData_(net::session_ptr& sk);

    int NotifyCharOpenInfo();
    void notifyOpeningState();
    //通知VIP活动按钮的状态
    void notifyVipState();
    void notifyChengzhangState();

    //查询VIP活动状态
    int getVipState();

    //查询开服活动状态
    int getOpeningState();
    //查询日常活动状态
    int getDailyState();

    void notifyEventState(int type, int active, int leftNums);
    //通知礼包活动按钮移除
    void notifyEventRemove(int type);

    //通知客户端角色经验信息发生变化
    //int NotifyCharExp();
    //角色状态变化
    int NotifyCharState();

    //type: 0表示原来接口，1,表示普通关卡 2表示精英关卡，id表示关卡id
    int getZhenGeneral(int zhenid, json_spirit::Object& robj, int type = 0, int id = 0);
    int NotifyZhenData();

    int sendObj(json_spirit::Object& obj);

    //显示背包东西
    int showBackpack(int page, int pagenums, json_spirit::Object& o);
    //显示身上装备
    int getEquiped(int gid, json_spirit::Object& robj);
    //显示身上装备详细信息
    int getEquipmentInfo(int id, json_spirit::Object& robj);
    //显示身上道具详细信息
    int getTreasureInfo(int id, int nums, json_spirit::Object& robj);
    //出售道具
    int sellTreasure(int id, int count, json_spirit::Object& robj);
    //增加角色装备
    int addEquipt(int id, std::string& msg, bool notify = true);
    //增加角色道具
    int addTreasure(int id, int counts);
    int addTreasure(int id, int counts, int& err_code);
    //角色道具數量
    int treasureCount(int id);
    //装备东西
    int equip(int gid, int slot, int eid);
    //一键装备
    int onekeyEquip(int gid);
    //卸下东西
    int unequip(int gid, int slot);
    //获得装备强化信息
    int getEquipmentUpInfo(int eid, json_spirit::Object& obj);
    //清除强化冷却
    int enhanceSpeed();
    //强化信息
    int enhanceEquipment(int eid, json_spirit::Object& robj);
    //设置强化等级
    int setEquipmentLevel(int id, int level);
    //获得强化装备列表
    int getEnhanceEquiptlist(int page, int pagenums, json_spirit::Object& obj);
    //卖装备
    int sellEquipment(int id, json_spirit::Object& robj);
    //回购装备
    int buybackEquipment(int id, json_spirit::Object& robj);
    //显示回购
    int listSelledEquipment(int page, int page_per, json_spirit::Object& o);

    //技能研究定时加点
    int skillResearchAdd(int sid, int times);
    //技能研究开始
    int startSkillResearch(int sid, int teacher);
    //停止技能研究
    int stopSkillResearch(int sid);
    //停止全部技能研究
    int stopSkillResearchAll();
    //技能掉落，技能升级
    int skillLevelup(int sid, int level);
    //设置技能等级
    int setSkillLevel(int sid, int level);

    //获得研究者信息
    int getTeacherList(json_spirit::Object& o);
    //获得技能详细信息
    int getSkillDetail(int sid, json_spirit::Object& o);
    //获得技能列表
    int getSkillList(json_spirit::Object& o);
    //获得技能研究列表
    int getSkillResearchList(json_spirit::Object& o);
    //获得技能等级
    int getSkillLevel(int sid);
    //获得技能研究列表
    int getSkillResearchInfo(int sid, json_spirit::Object& o);

    //刷新研究者
    int updateTeachers(json_spirit::Object& o, int type = 1);
    //购买
    int buyResearchQue(json_spirit::Object& o);
    //升级队列
    int upgradeResearchQue(json_spirit::Object& o);
    //加速研究
    int speedupResearch(int sid, int type, json_spirit::Object& o);

    //获得训练列表
    int getTrainList(json_spirit::Object& o);
    //获得兵书列表
    int getBookList(int id, json_spirit::Object& o);
    //武将训练
    int generalTrain(int gid, int bid, int pos, json_spirit::Object& o);
    //升级队列
    int upgradeGeneralTrainQue(int pos, json_spirit::Object& o);
    //刷新兵书
    int updateBooks(int type, json_spirit::Object& robj);
    //购买训练位
    int buyTrainQue(int pos, json_spirit::Object& robj);
    //重置训练位加速次数
    void resetTrainQue();
    //加载训练列表
    int LoadTrainList();
    //训练位重置CD
    int generalTrainSpeed(int pos);
    int generalTrainCoolTime(int& state);

    //开放武将训练
    int openTrain();

    //传承相关
    int generalInheritObj(int gid1, int gid2, int type, json_spirit::Object& o);
    int generalInheritInfo(int gid1, int gid2, json_spirit::Object& o);
    int generalInherit(int gid1, int gid2, int type, json_spirit::Object& o);
    int buyInherit(int num);

    //是否已有该武将
    bool CheckHasGeneral(int base_id);
    //解雇列表是否有该武将
    bool CheckHasFireGeneral(int base_id, int& general_id);
    //是否有该武将(解雇的也算)
    bool HasGeneral(int base_id);
    //是否已有该装备
    bool CheckHasEquipt(int base_id);

    int getPugong(bool load_horse);
    int getPufang(bool load_horse);
    int getCefang(bool load_horse);
    int getCegong(bool load_horse);
    int getBingli(bool load_horse);
    //刷新状态
    int refreshStates(int type, int id = 1);
    //获取战马
    int getHorse(json_spirit::Object& horse);
    //更新攻防
    int updateAttackDefense();
    //战斗消耗兵器/军令
    int combatCost(bool win, int type);
    //休息消耗
    int getRestCost(int times);
    //休息
    int rest(int type, json_spirit::Object& robj);
    //查询休息信息
    int queryRestInfo(json_spirit::Object& robj);
    //更新任务
    int updateTask(int type, int n1 = 0, int n2 = 0);
    //检查任务
    int checkTask();
    //官职升级
    int OfficalLevelUp();
    //更新官职升级信息
    bool OfficalLevelUpState();

    //购买官职武将
    int buyOfficalGeneral(int gid);
    //能否购买官职武将
    bool canBuyOfficalGeneral(int gid);

    //购买商品
    int buyShopGoods(int pos, json_spirit::Object& o);
    int refreshShopGoods(int type);
    //领取地图攻略奖励
    int getMapIntroReward(int mapid, json_spirit::Object& o);

    //心跳
    int HeartBeat();
    //保存
    int Save();
    void SaveWeapons(int);
    //保存日常数据
    int saveCharDailyVar();
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

    //查询引导完成情况
    time_t getGuideState(int id);
    //设置引导完成
    void setGuideStateComplete(int id, int next_guide);

    //检查是否触发引导
    int checkGuide(int type, int param1, int param2);
    //检查是否触发引导
    int _checkGuide(int id);
    //检查战败提示引导
    int _checkNotifyFail(int strongholdid);

    //更新vip等级
    int updateVip();
    //充值满奖励的判断
    //int updateRechargeReward(int num, time_t t = 0);

    //获取各队列冷却情况
    int getUpdateListCD(json_spirit::Object& robj);
    //获取功能开放信息
    int getOpeninfo(json_spirit::Object& robj);
    //获取洗髓概率
    int getWashPer(int type, int add);
    //获取洗髓界面
    int WashInfo(int gid, json_spirit::Object& robj);
    //洗髓
    int Wash(int gid, int type, json_spirit::Object& robj);
    int WashConfirm(int gid);

    //清除超过回收时间的武将和装备
    int ClearData();
    //检查是否需要插入默认兵器
    bool checkWeapon();

    //练兵系统的检查
    //bool checkSouls();

    //某张地图是否通关了
    bool isMapPassed(int mapid);

    //搬迁地图通知
    void notifyChangeMap();

    //考虑沉迷因素后的获得
    int chenmiGet(int input);
    //获得征收金币消耗
    int getLevyCost(int times);
    //获得征收银币单次奖励
    int getLevyReward();
    //设置兵器等级
    int setWeaponLevel(int type, int quality, int level);
    //获得活动按钮的闪亮条件
    bool getActionActive();
    int sellBagSlot(uint8_t slot, json_spirit::Object& robj);

    //更新功能开放
    int updateOpen();

    //更新强化需要的最少银币
    void updateEnhanceCost();

    //更新强化队列状态
    void updateEnhanceCDList();

    //更新强化秘法需要的最少功勋
    void updateUpgradeWeaponCost();

    //更新秘法队列状态
    void updateUpgradeWeaponCDList();

    //通知开放信息
    void realNotifyOpenInfo(net::session_ptr& sp);

    //变身卡
    int getChangeSpic();

    //某样装备的最高等级
    int maxEquipLevel(int type);

    //玩家开始临时VIP
    int startTmpVIP();

    //全服免费VIP 4活动
    int startTmpVIP2();

    //好友升级奖励
    void friendslevelup(int level);
    //获取玩家好友列表
    void getFriendsList(int type, json_spirit::Object& robj);
    //申请好友
    int submitApplication(const std::string& name);
    //通过申请
    int addFriend_id(int id);
    int acceptApplication(int friend_id, bool);
    int acceptAllApplication();
    //拒绝申请
    int rejectApplication(int friend_id);
    int rejectAllApplication();
    //删除好友
    int deleteFriend(const std::string& name);
    int deleteFriend_id(int id);
    //确认是否好友
    bool check_friend(int id);

    void loadFriends();
    void saveFriends();

public:
    int getAttack(int zid = 0);        //攻擊力查詢
    void set_attack_change(bool e = false);    //設置變化
    int get_skill_attack();    //技能對戰力加成
    int buff_attack(int damage_type, int attack, int hp, int wufang, int cefang);    //战力算上增益

    //查詢可以鑲嵌寶石的英雄列表
    int queryBaoshiGeneralList(json_spirit::Array& glist);
    //查詢英雄寶石
    int queryGeneralBaoshi(int gid, json_spirit::Object& robj);

    //查詢寶石屬性
    int queryBaoshi(int baoshi_id, json_spirit::Object& robj);

    //查詢英雄寶石总屬性
    int queryGeneralBaoshiInfo(int gid, json_spirit::Object& robj);

    //移除寶石
    int removeBaoshi(int gid, int slot);
    //鑲嵌寶石
    int xiangruBaoshi(int bagSlot, int gid, int slot);

    //赠送宝石
    int giveBaoshi(int type, int level = 1, int reason = 1);

    //更新状态，技能的影响
    void updateCombatAttribute();

    int getRoleDetail(json_spirit::Object& robj);

    bool updatePanelOpen();

    //购买背包
    int buyBagSlot(int num, json_spirit::Object& robj);

    //升级装备
    int upgradeEquipment(int eid, bool cost_gold, json_spirit::Object& robj);
    //获取装备升级信息
    int getUpgradeEquipmentInfo(int eid, int type, json_spirit::Object& robj);
    void getScrollTips(baseTreasure* bt, json_spirit::Object& robj);
    int openSlotItm(int slot, int nums, json_spirit::Object& robj);

    int updateNewbieEventState();

    int isNewPlayer();
    //获取创建角色的自然天数
    int queryCreateDays();

    bool isWashEventOpen();

    int updateWashEventState();

    //当前引导id
    int getCurrentGuide(int& state);

    int getGuideState1(int guide);

    int getGuideState2(int guide);
    //关卡状态
    int getStrongholdState(int strongholdId);

    //查找关卡
    boost::shared_ptr<CharStrongholdData> getDestStronghold();
    //打开礼包
    int openLibao(int slot, json_spirit::Object& robj, bool real_get = false);
    //直接奖励礼包
    int rewardLibao(int libao_id, json_spirit::Object& robj);
    //获取礼包信息
    int getLibaoInfo(int libao_id, json_spirit::Object& robj);
    //成长礼包信息
    int getChengzhangLibaoInfo(int pos, json_spirit::Object& robj);

    //注册天数
    int regDays();

    int m_baoshi_count;
    //更新宝石孔-true 宝石孔变化了
    bool updateBaoshiCount();
    //更新武将的宝石孔
    int updateGeneralsBaoshiCount();
    //今天是否可享受找回
    bool canFindBack();
};

//基础兵种数据
struct BaseSoldierData
{
    int m_stype;        //兵种id
    int m_base_type;    //所属基本类别(1步2弓3士4马5车)
    int m_attack;        //基础物攻
    int m_wufang;        //基础物防
    int m_cefang;        //基础策防
    int m_damage_type;    //攻击类别(1物理攻击2策略攻击)
    int m_attack_type;    //攻击范围(例如攻击一列)
    int m_attack_type2;//绝技攻击 远程(2)或近战(1)
    int m_attack_type3;//普通攻击 远程(2)或近战(1)
    int m_damage_type2;//伤害类别 1被火器打,2被枪刺，3被刀砍,4被箭射,5被魔法忽悠

    int m_special_attack_fac;    //绝技攻击

    boost::shared_ptr<baseState> m_pec[3];        //特性(例如攻击混乱)
    int m_spic;            //兵种图片
    std::string m_name;    //兵种名
    std::string m_desc;    //兵种描述

    Object m_soldier_obj;

    //战斗属性
    combatAttribute m_combat_attribute;

    void addObj(const std::string& name, json_spirit::Object& obj);
};

struct newTianfu
{
    std::string m_name;
    std::string m_memo;
    int m_more_tong;
    int m_more_int;
    int m_more_str;

    int m_more_hp;

    combatAttribute m_combatAttr;
};

//基础武将数据
struct GeneralTypeData
{
    int m_gid;         //武将id
    int m_spic;        //图片
    int m_stype;       //兵种id
    int base_str;      //基础勇武
    int base_int;      //基础智力
    int base_tongyu;   //基础统御
    //int m_equipt;        //武将特殊加成(武器)
    int m_tianfu;        //属性天赋(统1，智2，勇3)

    //2.14 新天赋
    newTianfu m_new_tianfu;

    int m_quality;        //品质

    int m_baowu_baseval;    //宝物基础属性
    int m_baowu_addperlev;    //宝物成长值
    int m_baowu_type;        //宝物属性类型1勇2智3统
    int m_baowu_spic;    //宝物图片

    int m_spe_skill_id;    //特殊技能id
    int m_good_at;            //擅长

    double m_base_chengzhang;    //武将基础成长
    int m_inherit_cnt;//传承丹数量

    std::string m_name; //武将名
    std::string m_nickname; //武将绰号
    std::string m_desc;     //介绍
    std::string m_baowu;    //宝物名字
    std::string m_jxl;        //将星录信息

    boost::shared_ptr<specialSkill> m_speSkill;

    json_spirit::Array m_tj_baoshi_list;

    void toObj(json_spirit::Object& obj);
};


//关卡武将数据
struct StrongholdGeneralData
{
    int m_pos;            //位置
    std::string m_name; //姓名
    int m_spic;         //图片
    int m_color;        //品质
    int m_stype;        //兵种id
    int m_level;        //等级
    int m_hp;           //血量
    int m_attack;       //攻击
    int m_pufang;       //普防
    int m_cefang;       //策防
    int m_str;          //勇武
    int m_int;          //智力
    int m_special;        //区分喽啰和老大
    boost::shared_ptr<specialSkill> m_speSkill;
    boost::shared_ptr<BaseSoldierData> m_baseSoldier;
};

struct strongholdReport
{
    uint64_t bid;        //战报id
    std::string aName;    //攻击者名字
    int aLevel;            //攻击者等级
    int attack;            //战力值-新增
    int hurt;            //战斗损失兵力
    time_t attack_time;//攻击时间

    strongholdReport()
    {
        bid = 0;
        aName = "";
        aLevel = 0;
        attack = 0;
        hurt = 0;
        attack_time = 0;
    }
};

struct strongholdRaiders
{
    strongholdReport m_first_report;                //首次攻打的战报
    strongholdReport m_best_report;                //最佳击杀
    //增加一条攻略
    int addRecords(const std::string& name, int level, uint64_t bid, int attack = 0, int hurt = 0);
    void load(const std::string& name, int level, uint64_t bid, int attack, int hurt, time_t time);
    void getRadiers(json_spirit::Object& robj);
};

//地图-场景-关卡(跟角色类似管理着阵形及武将)
//关卡数据
struct StrongholdData
{
    int m_id;           //关卡id
    int m_level;        //关卡等级
    int m_map_id;        //所属地图
    int m_stage_id;    //所属场景
    int m_strongholdpos;//所在位置
    int m_isepic;        //是否精英
    int m_spic;            //关卡图片
    int m_color;      //品质
    int m_stateNum;    //状态个数

    int m_group;        //第几组

    int m_model;        //地图上显示模型编号

    int m_x;            //地图上坐标
    int m_y;

    //int m_limits;        //精英关卡攻击次数限制

    int m_need_supply;    //占领需要的粮草
    int m_rob_supply;    //掠夺得到的粮草
    int m_gongxun;        //得到的功勋
    int m_open_zhen;
    int m_open_zhen_level;
    int m_general_limit;    //武将上限
    int m_guide_id;    //打完关卡的对应引导
    int m_guide_fail;   //失败对应引导

    std::string m_name;//主将名
    std::string m_chat;//喊话
    std::string m_failMsg;//失败提示

    boost::shared_ptr<json_spirit::Object> m_loot;//关卡掉落
    boost::shared_ptr<Item> m_Item;

    boost::shared_ptr<StrongholdGeneralData> m_generals[9];
    boost::shared_ptr<baseStage> m_baseStage;

    //关卡的抗性
    combatAttribute m_combat_attribute;

    strongholdRaiders m_raiders;

    StrongholdData()
    {
        m_need_supply = 10;    //占领需要的粮草
        m_rob_supply = 50;    //掠夺得到的粮草
        m_open_zhen = 0;
        m_open_zhen_level = 0;
        m_general_limit = 0;
    };
    int getAttack();
};

//场景数据
struct StageData
{
    int m_stage_id;//场景id
    int m_mapid;//所属地图id
    int m_size;//场景大小
    int m_strongholds[25];//场景关卡
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
#else
    std::string m_vcode;
    std::string m_sid;
#endif
    time_t m_logintime;           // 登录时间
    boost::shared_ptr<OnlineCharactor> m_onlineCharactor;  //角色
    std::list<CharactorInfo> m_charactorlist;              //角色信息列表

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
    int Chat(const std::string &, const std::string &, int type = 0, int gender = 0, int mod = 0, const std::string& nick = "[]");
    std::string genChatMsg(const std::string &, const std::string &, int type = 0);
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

struct baseStage;

//地图
struct baseMap
{
    int id;
    std::string name;
    int openLevel;
    std::string memo;
    std::string intro;
    std::string get;
    boost::shared_ptr<baseStage> stages[3];    //地图中的场景
};

//场景
struct baseStage
{
    int id;
    int mapid;
    std::string name;
    int openLevel;
    int size;
    int spic;

    int stage_finish_guide;    //领完通关奖励后的引导

    json_spirit::Array boss_list;

    void addLoot(boost::shared_ptr<Item> i, boost::shared_ptr<json_spirit::Object> o);

    boost::shared_ptr<json_spirit::Array> loots_array;        //掉落

    boost::shared_ptr<baseMap> _baseMap;
    boost::shared_ptr<StrongholdData> _baseStrongholds[25];
};

struct DateInfo
{
    int year;       //年份
    int season;    //季节 1 - 4，春夏秋冬
    int effect_type;   //特殊效果
    std::string effect; //特殊效果
};

struct color_data
{
    int white;
    int blue;
    int green;
    int yellow;
    int red;
    int purple;
};

struct admin_notice
{
    int _id;
    int _state;
    std::string _message;
};

struct baseGoods
{
    int id;
    int type;
    int be_suggest;
    int gold_to_buy;
    boost::shared_ptr<Item> m_item;
};

//商城折扣活动
struct mall_discount_st
{
	int discount;
	time_t start;
	time_t end;
};

class GeneralDataMgr
{
public:
    GeneralDataMgr();
    ~GeneralDataMgr();
    int reload(int flag);
    int reloadBaseSoldier();
    int reloadBaseEquipments();

    int loadLang();

    int reloadOtherBaseData();

    int reloadMap();

    int reloadStage();

    int reloadBaseStrongholdGenerals();
    static GeneralDataMgr* getInstance();
    static void release();
    int GetStrongholdid(int mapid, int stageid, int strongholdpos);//获取关卡id号
    int GetStrongholdPos(int& mapid, int& stageid, int strongholdid);//获取关卡位置
    int GetMapMemo(int mapid, std::string& name, std::string& memo);//获取地图描述
    int GetStageLimitLevel(int mapid, int stageid);//获取场景等级限制
    std::string GetStageName(int mapid, int stageid);//获取场景名字
    boost::shared_ptr<BaseSoldierData> GetBaseSoldier(int sid);    //获取基础兵数据
    boost::shared_ptr<GeneralTypeData> GetBaseGeneral(int gid);    //获取基础武将数据
    //获得基础阵型信息
    boost::shared_ptr<BaseZhenData> GetBaseZhen(int zid);
    //获取成长率星级
    boost::shared_ptr<baseChengzhangStars> GetBaseChengzhangStarByValue(double chengzhang);
    boost::shared_ptr<baseChengzhangStars> GetBaseChengzhangStarByStar(int star);
    //获取洗髓星级
    boost::shared_ptr<baseWashStars> GetBaseWashStarByValue(int score);
    boost::shared_ptr<baseWashStars> GetBaseWashStarByStar(int star);

    void addCharData(boost::shared_ptr<CharData> cdata);    //加入角色数据
    boost::shared_ptr<CharData> GetCharData(int cid);    //获取角色数据
    boost::shared_ptr<std::list<boost::shared_ptr<EquipmentData> > > GetCharEquiptData(int cid);    //获取角色装备列表
    boost::shared_ptr<CharGeneralsData> GetCharGenerals(int cid);    //获取角色全部武将数据
    boost::shared_ptr<ZhenData> GetCharDefaultZhen(int cid);//获得角色默认阵型
    boost::shared_ptr<CharStageData> GetCharStageData(int cid, int stageid);    //获取角色关卡进度
    boost::shared_ptr<CharStrongholdData> GetCharStrongholdData(int cid, int mapid, int stageid, int strongholdpos);    //获取角色关卡状态
    boost::shared_ptr<StrongholdData> GetStrongholdData(int strongholdid);    //获取关卡属性
    boost::shared_ptr<StrongholdData> GetStrongholdData(int mapid, int stageid, int pos);    //获取关卡属性

    boost::shared_ptr<StrongholdGeneralData> GetStrongholdGeneralData(int gid);    //获取关卡武将数据
    //创建在线角色信息
    boost::shared_ptr<OnlineCharactor> CreateOnlineCharactor(boost::shared_ptr<OnlineUser> account, uint64_t cid);
    //在线角色信息
    boost::shared_ptr<OnlineCharactor> GetOnlineCharactor(const std::string& char_name);

    int Logout(boost::shared_ptr<OnlineUser>& p);
#ifdef QQ_PLAT
    int Login(const std::string& qid, const std::string& id, int isAdult, int union_id, const std::string& server_id, int qqyellow, int isyear, const std::string& iopenid, const std::string& feedid, const std::string& str1, const std::string& str2, net::session_ptr csocket, Object& robj);
#else
    int Login(const std::string& qid, const std::string& account, int isAdult, int union_id, const std::string& server_id, int qq_yellow, int is_year, const std::string& vcode, const std::string& sid, net::session_ptr csocket, Object& robj);
#endif
    boost::shared_ptr<OnlineUser> GetAccount(const std::string& account);
    boost::shared_ptr<ChatChannel> GetWorldChannel();
    boost::shared_ptr<ChatChannel> GetGuildChannel(uint64_t guild_id);
    boost::shared_ptr<ChatChannel> GetCampChannel(int camp);
    boost::shared_ptr<ChatChannel> GetMapChannel(int mapid);

    int CharactorLogin(boost::shared_ptr<OnlineCharactor> oc);
    int CharactorLogout(boost::shared_ptr<OnlineCharactor> oc);

    //获得角色登录列表
    //int GetCharactorInfoList(const std::string& account, std::list<boost::shared_ptr<CharactorInfo> >& charactorlist);
    //保存角色信息
    int SaveDb(int save_all);
    //回复玩家军令
    int addLing(int counts);

    //获得基础装备
    boost::shared_ptr<baseEquipment> GetBaseEquipment(int baseid);

    //获得基础宝物
    boost::shared_ptr<baseTreasure> GetBaseTreasure(int tid);

    //获得地图
    boost::shared_ptr<baseMap> GetBaseMap(int mid);

    boost::shared_ptr<baseoffical> GetBaseOffical(int id);

    std::list<boost::shared_ptr<officalgenerals> >& GetBaseOfficalGenerals();
    std::list<Item>& GetFirstRechargeGift();
    //系统消息广播
    int broadCastSysMsg(const std::string& msg, int type);
    //系统消息广播- level等级以上
    int broadCastSysMsg(const std::string& msg, int type, int level);

    //系统消息广播-地图广播
    int broadCastSysMapMsg(const std::string& msg, int type, int map);

    //广播到全服在线玩家
    int broadCastToEveryone(const std::string & msg, int repeatnums, int interval);

    //获得当前年份、季节
    boost::shared_ptr<DateInfo> GetDate();

    //获得当前年份、季节
    int getYear();

    //获得当前年份、季节
    int getSeason();

    //获得当前年份、季节
    std::string getSeasonString();

    //获得竞技场奖励时间
    int GetRaceRewardTime();

    boost::shared_ptr<color_data> getColorData(int level);

    baseLoginPresent* getBaseLoginPresent(int pid);

    boost::shared_ptr<baseVIPPresent> getBaseVIPPresent(int id);

    baseRechargePresent* getBaseRechargePresent(int id);
    //洗髓概率
    std::list<boost::shared_ptr<baseWashPer> >& GetBaseWashPer();
    int checkRechargePresent(int num, int type);

    uint64_t newCombatId()
    {
        return ++m_combat_id;
    }
    uint32_t newEquiptId()
    {
        return ++m_equipt_id;
    }
    uint32_t newGeneralId()
    {
        return ++m_general_id;
    }
    uint32_t newGemId()
    {
        return ++m_gem_id;
    }
    uint32_t newCharId()
    {
        return ++m_charactor_id;
    }
    //删除角色时，移除角色数据
    int removeCharData(int cid);
    void updateSeason();
    void updateRaceRewardTime();
    int resetWelfare();
    int resetWelfare2();
    //重置日常的一些数据
    int resetAll();
    //每週更新
    int weekReset();
    //保存商店购买记录
    void saveShopRecord();
    //系统刷新所有玩家商店
    int resetShopGoods();
    //系统刷新所有玩家冶炼任务
    //int refreshSmeltTask();
    //在线人数
    int getTotalOnline(bool record);
    //是否开启防沉迷
    bool isChenmiEnable() {return m_enable_chenmi;}
    //心跳
    int HeartBeat();

    //给角色发送消息
    int sendSysMsg(const std::string& char_name, const std::string& msg, int type);
    int sendNotifyMsg(const std::string& char_name, const std::string& msg);

    //设置角色关卡进度
    int updateTempo(int cid, int level);

    int getNearPlayerList(CharData* cdata, json_spirit::Object& robj);

    //好友推送
    int getRecommendFriends(CharData* cdata, json_spirit::Object& robj);

    //好友推送
    bool canRecommendFriends(CharData* cdata);

    //获得弱势阵营
    int getWeakCamps();
    //int updateCampCnt(int type);
    int RevoltCamps(int& type);

    //检查系统公告变化
    void checkAdminNotice(int type);
    int adminNoticeDeleted(int id);
    int adminNoticeNew(int id, const std::string& message);
    int adminNoticeChanged(int id, const std::string& message);
    int getAdminNotice(json_spirit::Array& notice_list);

    void addGeneral(int gid, int cid);

    int getGeneralOwner(int gid);

    void shutdown();

    //获得角色id
    int GetCharId(const std::string& cname);
    //GM
    int GM_reward();
    void GM_recharge(boost::shared_ptr<CharData>& cdata, int recharge_gold);
    //GM回复
    void GM_answer();

    //更新试用VIP活动
    void checkTmpVIP();

    //camp = 1,2,0,
    //void campRaceWinner(int camp);

    int getInt(const std::string& field, int defaultv = 0);
    void setInt(const std::string& field, int value);

    std::string getStr(const std::string& field);
    void setStr(const std::string& field, const std::string& value);

    //检查强弱阵营
    //void checkWeakCamp();

    //查询怒气技能
    boost::shared_ptr<specialSkill> getSpeSkill(int id);

    int getSpeSkillType(const std::string& type);
    int getSpeSkillTarget(const std::string& target);
    baseBuff* getBaseBuff(int id);

    //全服免费VIP4活动是否开启
    bool isFreeVIPEventOpen();
    time_t freeVIPEndtime() {return m_free_vip4_endtime; }
    void openFreeVIP4Event(int day);

    int getOpenZhen(int stronghold);

#ifdef QQ_PLAT
    void to_Tencent();
#endif
    void getRandomServantList(std::vector<int>& list);
    int GetBaseMallGoodId(int type, int id);
    std::map<int, boost::shared_ptr<baseGoods> >& GetBaseMallGoods();
	//开启商城打折活动
	void openMallDiscountEvent(int discount, time_t start_time, time_t end_time);
    float getMallDiscount();

    //随机名字
    std::string getRandomName(int gender);

    //能否购买官职武将
    bool canBuyOfficalGeneral(int gid, int slevel, int offical);
    //查询官职武将
    officalgenerals* getOfficalGeneral(int gid);

private:
    rwlock guild_chanel_rwmutex;
    rwlock camp_chanel_rwmutex;
    rwlock onlineuser_rwmutex;

    rwlock onlinechar_rwmutex;
    rwlock globalchar_rwmutex;

    //频道
    boost::shared_ptr<ChatChannel> m_world_channel;     //世界聊天
    boost::unordered_map<int, boost::shared_ptr<ChatChannel> > m_camp_channels;         //阵营聊天
    boost::unordered_map<uint64_t, boost::shared_ptr<ChatChannel> > m_guild_channels;   //公会聊天
    //地图频道
    boost::shared_ptr<ChatChannel> m_map_channels[max_map_id];   //公会聊天

    boost::unordered_map<std::string, boost::shared_ptr<OnlineUser> > m_onlineuserlist;

    //int camp1_cnt;//官府玩家数
    //int camp2_cnt;//绿林玩家数

    //地图

    static GeneralDataMgr* m_handle;
    std::map<int, boost::shared_ptr<BaseSoldierData> > m_base_soldiers_map; //基础兵数据
    std::map<int, boost::shared_ptr<GeneralTypeData> > m_base_generals_map; //基础武将数据
    std::map<int, boost::shared_ptr<baseChengzhangStars> > m_base_chengzhang_stars; //基础成长星级数据
    std::map<int, boost::shared_ptr<baseWashStars> > m_base_wash_stars; //基础洗髓星级数据

    std::map<int, boost::shared_ptr<CharData> > m_chardata_map;             //全服角色数据
    std::map<std::string, int> m_charid_map;       //全服角色id映射

    std::map<int, boost::shared_ptr<StrongholdGeneralData> > m_stronghold_generals_map;//关卡武将数据

    std::map<int, boost::shared_ptr<StrongholdData> > m_stronghold_data_map;//关卡数据

    std::map<std::string, boost::shared_ptr<OnlineCharactor> > m_online_charactors; //在线角色

    std::map<int, boost::shared_ptr<BaseZhenData> > m_base_zhens;       //基础阵型

    std::map<int, boost::shared_ptr<baseEquipment> > m_base_equipments;     //基础装备数据

    std::map<int, boost::shared_ptr<baseTreasure> > m_base_treasures;         //基础宝物数据

    std::map<int, boost::shared_ptr<baseGoods> > m_base_mall_goods;         //商城数据
	struct mall_discount_st m_mall_discount_st;					//商城打折活动

    boost::shared_ptr<baseMap> m_base_maps[10];    //基础地图

    std::map<int, boost::shared_ptr<specialSkill> > m_spe_skill_map;        //怒气技能
    std::map<int, boost::shared_ptr<baseBuff> > m_base_buff_map;

    std::map<const std::string, int> m_target_string_map;
    std::map<const std::string, int> m_speskill_type_string_map;

    volatile uint64_t m_combat_id;
    volatile uint32_t m_equipt_id;
    volatile uint32_t m_general_id;
    volatile uint32_t m_gem_id;
    volatile uint32_t m_charactor_id;

    bool m_enable_chenmi;        //开启防沉迷

    bool m_inited;

    std::map<int, boost::shared_ptr<baseoffical> > m_base_officals;    //基础官职数据

    std::list<boost::shared_ptr<officalgenerals> > m_base_offical_generals;    //基础官职武将数据
    std::map<int, boost::shared_ptr<officalgenerals> > m_base_offical_generals_map;//基础官职武将数据
    std::map<int, std::list<boost::shared_ptr<officalgenerals> > > m_offical_generals;   //各官职开放的官职武将
    std::map<int, std::list<boost::shared_ptr<officalgenerals> > > m_stronghold_offical_generals;   //各关卡开放的官职武将

    std::map<int, boost::shared_ptr<baseofficalskill> > m_base_offical_skills;//基础官职技能数据

    std::map<int, boost::shared_ptr<color_data> > m_level_color_data;//武将颜色对应属性值
    std::map<int, boost::shared_ptr<baseLoginPresent> > m_base_login_present;//基础连续登录奖励
    std::map<int, boost::shared_ptr<baseVIPPresent> > m_base_vip_present;//基础VIP奖励
    std::map<int, boost::shared_ptr<baseRechargePresent> > m_base_recharge_present;//基础充值满奖励

    std::map<int, int> m_zhen_open_map;    //阵型开放关卡

    std::list<Item> m_first_recharge_gift;    //首充礼包

    std::map<int, int> m_general_map;    //武將和角色對應表

    //正常线程用的当前公告
    std::list<admin_notice> m_currentAdminNotices;

    //检测变更线程用的
    std::list<admin_notice> m_adminNotices;

    int m_year;        //游戏年份
    int m_season;    //季节 1 - 4
    int m_race_reward_time;    //竞技场奖励发放时间

    //当前年份、季节
    boost::shared_ptr<DateInfo> m_spls_date;

    //阵营战连续胜利
    //int m_camp_race_wins[2];

    //阵营双方活跃实力
    int m_camp_strength[2];
    //阵营活跃人数
    int m_camp_count[2];

    time_t m_free_vip4_endtime;

    //int m_weak_camp;    //弱势阵营

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
    int m_state;    // 0 正常 1 待删除
    time_t m_deleteTime;
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
    //公会聊天
    int GuildChat(const std::string& msg);
    //地区聊天
    int AreaChat(const std::string& msg);
    //世界聊天
    int WorldChat(const std::string& msg, bool needgold = true);
    //阵营聊天
    int CampChat(const std::string& msg);
    //组队聊天
    int TeamChat(const std::string& msg);
    //进入地区
    int EnterArea(int area);
    //私聊
    int Tell(const std::string& to, const std::string& what, boost::shared_ptr<OnlineCharactor>& toChar);
    //重新从数据库加载信息
    int reload();

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
int CreateChar(const std::string& account, int union_id, const std::string& server_id, const std::string& qid, int camp, int spic, const std::string& name, int g1, int g2, uint64_t& cid, int inv_id);

//删除角色
int DeleteChar(uint64_t cid);

//从数据库中获取角色信息
//int GetCharInfo(int cid, int& camp, int& guild, std::string& name);

//战斗回放
std::string getCombatRecord(int id);

int ProcessCheckPack(json_spirit::mObject& o);

//检查充值
int ProcessCheckRecharge(json_spirit::mObject& o);

//玩家离线
void accountOffline(const std::string& account);

#endif

