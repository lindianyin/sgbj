
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
//��ͨ��
    char_data_first_wusong = 1,        //�Ѿ�����״���ļ����
    char_data_explore_num,            //̽������-2�����ϲ�����
    char_data_welfare,                //�������ȡ���
    char_data_chenmi_time,            //����ʱ��
    char_data_continue_days,            //������¼����
    char_data_daily_task_pos,        //�ճ�����id
    char_data_lottery_total_score,    //÷�������ܻ���
    char_data_first_luzhishen,        //�Ѿ�����״���ļ³����
    char_data_first_shijin,        //�Ѿ�����״���ļʷ��
    char_data_first_yangzhi,        //�Ѿ�����״���ļ��־
    char_data_race_maxrank,        //��������������������
    char_data_first_recharge_gift,//�׳����״̬ 1 ������ȡ��2 �Ѿ���ȡ
    char_data_wash_start_time,    //ϴ�蹦�ܿ���ʱ��
    char_data_current_guide,        //��ǰ��������
    char_data_shop_refresh_time,//�̵�ˢ��ʱ��
    char_data_change_spic,        //��������
    char_data_change_spic_time,    //�������ʱ��
    char_data_get_onlinegift_day,    //��ȡ���߽�������timestamp
    char_data_get_collect_reward,    //��ȡ�ղ����
    char_data_get_continue_login_day,    //������¼�����������timestamp
    char_data_stage_award_start    = 1000,    //ͨ�ؽ�����ȡ��ǿ�ʼ��33��ͼ������
    char_data_stage_award_end     = 1200,    //ͨ�ؽ�����ȡ��ǽ���

    char_data_first_general_quality1,    //��һ���̽�
    char_data_first_general_quality2,    //��һ������
    char_data_first_general_quality3,    //��һ���Ͻ�
    char_data_first_general_quality4,    //��һ���Ƚ�
    char_data_first_general_quality5,    //��һ��x��

    char_data_first_race_success,        //�׳�����ʤ��

    char_data_qq_yellow_special,            //QQ������Ȩ�����ȡ
    char_data_qq_yellow_newbie,            //QQ�������������ȡ
    char_data_qq_yellow_level_libao,        //QQ����ȼ������ȡ - ����200��
    char_data_qq_yellow_level_libao_end = char_data_qq_yellow_level_libao + 200,

    char_data_seven_goals_small_start,    //����Ŀ�����ϸ��ɶ�
    char_data_seven_goals_small_end = char_data_seven_goals_small_start + 50,
    char_data_seven_goals_start,    //����Ŀ��������ɶ�
    char_data_seven_goals_end = char_data_seven_goals_start + 15,

    char_data_vip_special_libao,        //VIPר���佫���
    char_data_new_player_end,        //���ָ�����������
    char_data_wash_per,        //ϴ��ɹ��ʼӳ�(ȡֵ/1000)
    char_data_vip8_general,    //V8�佫
    char_data_trade_refresh_num,    //ˢ��ó�����˴�������ͳ��ǰ2��������
    char_data_first_sign_info,    //��һ�δ�ǩ������
    char_data_zhen_attack,                //ս��
    char_data_zst_map_award_start,    //ս��̨ͨ�ص�ͼ������ȡ
    char_data_zst_map_award_end = char_data_zst_map_award_start + 50,

    char_data_view_bank,    //����Ǯׯ
    char_data_view_ranking,        //����������
    char_data_view_boss,            //����ս����
    char_data_view_camprace,            //������Ӫս
    char_data_view_seven,            //��������Ŀ��
    char_data_vip10_general,    //V10�佫
    char_data_event_train_horse_cnt,//ս��ѵ�������
    char_data_fb_get,//facebook

//ÿ�յ� 10000 ��
    char_data_gold_rest = 10000,    //�����Ϣ����
    char_data_refesh_explore,    //ˢ��̽������
    char_data_get_salary,            //��ȡٺ»
    char_data_buy_xisui_time,    //����ϴ�����
    char_data_levy_time,            //���մ���
    char_data_jisi_time,            //�������
    char_data_yanhui_time,        //������
    char_data_train_horse,        //�������ս�����
    char_data_daily_task,            //�ճ�������ɴ���
    char_data_free_reborn,        //VIP�������
    char_data_camp_reward,        //������Ӫÿ�ս���
    char_data_camp_revolt,        //������Ӫ���Ӵ���
    char_data_buy_explore,        //����̽������
    char_data_buy_race,            //���򾺼�����
    char_data_trade_time,            //ͨ�̴���
    char_data_randomservant,        //�������ץ������
    char_data_buy_randomservant,    //�������ץ������
    char_data_online_gift,        //���߱���ڼ���
    char_data_online_gift_state,    //���߱���״̬1������ȡ
    char_data_daily_task_reward,    //�ճ���������ȡ����
    char_data_super_wash_times,    //����ϴ�����
    char_data_wash_event,            //ϴ��5��
    char_data_wash_event_end = char_data_wash_event + 10,    //ϴ������
    char_data_shop_refresh,        //ˢ���̵����
    char_data_book_refresh,        //ˢ�±������
    char_data_test_recharge,        //���Գ�ֵ����
    char_data_horse_gold_train,    //ÿ��ս������������
    char_data_horse_silver_train,//ÿ��ս����ͨ��������
    char_data_daily_task_start,    //�����и��ճ�����
    char_data_daily_task_end = char_data_daily_task_start + 50,    //�����и��ճ�����
    char_data_daily_task_reward_start,    //�ճ������������ȡ���
    char_data_daily_task_reward_end = char_data_daily_task_reward_start + 50,    //�ճ������������ȡ���

    char_data_daily_reserved = char_data_daily_task_reward_end + 10,    //����
    char_data_daily_congratulation_received,    //�����յ���ף����
    char_data_daily_congratulation_sended,        //���췢����ף����

    char_data_farm_seed,    //�������
    char_data_farm_water,    //���ｽˮ����
    char_data_farm_water_cd,    //���ｽˮ��ȴʱ��
    char_data_farm_friend_water,    //��������ѽ�ˮ����

    char_data_daily_trade_abandon,//ÿ��ó�׷������˴���

    char_data_daily_corps_explore,//����̽������

    char_data_daily_corps_ymsj,    //ԯ����ꪴ���

    char_data_daily_findback,//�����һر��
    char_data_daily_findback_task_start,//�����и��ճ������һؼ�¼
    char_data_daily_findback_task_end = char_data_daily_findback_task_start + 50,

    char_data_daily_maze_times,    //�Թ�����

    char_data_daily_tencent_bag,    //��Ѷͳ�Ʊ�����Ʒ
    char_data_daily_qq_yellow_libao,            //QQ����ÿ�������ȡ
    char_data_daily_qq_year_yellow_libao,    //QQ��ѻ���ÿ�������ȡ

    char_data_daily_view_first_recharge,    //�����׳����
    char_data_daily_view_vip_benefit,        //����VIP��Ȩ
    char_data_daily_view_new_event,            //�������ʻ��ť
    char_data_daily_vip_libao,                //VIPÿ�������ȡ

    char_data_get_yushi,        //�����ȡ��ʯ����
    char_data_yushi_time_cd,    //�´���ȡ��ʯʱ��

    char_data_daily_view_feedback,    //�鿴����������

    char_data_daily_view_invite,        //������������

    char_data_daily_buy_souls_daoju1,        //���������Ƭ����
    char_data_daily_buy_souls_daoju2,        //���������Ƭ����
    char_data_daily_buy_souls_daoju3,        //���������Ƭ����

    char_data_daily_con,        //�ΰݴ���

    char_data_daily_zst_challenge,        //ս��̨��ս����
    char_data_daily_zst_buy_challenge,  //ս��̨������ս����
    char_data_daily_zst_refresh,        //ս��̨�����Ǽ�����

    char_data_daily_login_event,//��¼�ÿ�ռ�����


//ÿ�ܵ� 20000 ��
    char_data_extra_lottery_score = 20000,    //÷������ÿ�ܻ���
    char_data_extra_daily_task_star,            //�ճ������Ǽ�
    char_data_week_vip_libao,                //VIP�ܸ���
    char_data_extra_prestige_get,                //ÿ���������
};

//״̬�Ƕ���ߵȼ�20
const int iMaxStateStarLevel = 20;

//�����������Ĺ���
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

//�����
enum action_enum
{
    action_active = 1,        //�
    action_boss = 2,        //bossս
    action_camp_race = 3,    //��Ӫս
    action_guard = 4,        //����
    action_race = 5,        //������
    action_group_copy = 6,//���˸���
    action_trade = 7,        //ͨ��
    action_chenmi = 8,        //������
    action_lottery = 9,    //÷������
    action_daily_task = 10//�ճ�����
};

//�������޷���
enum baby_enum
{
    baby_boss_start = 1,        //bossս
    baby_boss_end = baby_boss_start + 5,
    baby_camp_race,    //��Ӫս
};

enum top_level_event
{
    top_level_event_helper = 1,    //��Ϸ����
    top_level_event_daily = 2,    //�ճ��:1���˸�����2ս���ޣ�3��Ӫս
    top_level_event_elite = 3,    //��Ӣ����
    top_level_event_race = 4,        //������
    top_level_event_servant = 5,    //׳��
    top_level_event_guard = 6,    //����
    top_level_event_opening = 7,    //�����:1���ʳ�ֵ 2�ۼƳ�ֵ 3�ȼ���� 4�佫� 5��ʯ�
    top_level_event_present = 8,    //���:1��¼�����2�׳������3VIP���
    top_level_event_salary = 9,    //ٺ»
    top_level_event_chenmi = 10,    //������
    top_level_event_rankings = 11,    //������
    top_level_event_rankings_reward = 12,    //���н���
    top_level_event_bank = 13,    //Ǯׯ
    top_level_event_corp = 14,    //����
    top_level_event_maze = 15,    //������
    top_level_event_first_recharge = 16,    //�׳����
    top_level_event_vip_present = 17,    //VIP���
    top_level_event_seven_goals = 18,    //����Ŀ��
    top_level_event_sign = 19,            //ǩ��

    top_level_event_reward_boss = 20,    //ս���޽���
    top_level_event_reward_boss_kill = 21,    //ս���޽���
    top_level_event_reward_explore = 22,//̽������
    top_level_event_reward_race = 23,    //����������
    top_level_event_reward_yanhui = 24,        //������ά��
    top_level_event_feedback = 25,        //��������
    top_level_event_invite = 26,        //�������
    top_level_event_daily_recharge = 27,    //�ճ�
    top_level_event_jxl = 28,        //����¼
    top_level_event_throne = 29,        //����
    top_level_event_jtz_awards = 30,    //����ս����
    top_level_event_jt_boss_kill = 31,   //����boss
    top_level_event_zst = 32,        //ս��̨
    top_level_event_chengzhang = 33,        //�ɳ����
    top_level_event_lottery_event = 34, //�齱�

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
    //��ѯ������������״̬
    void getStandIn(int cid, int& enable, int& silver, int& prestige);
    //������������
    void setStandIn(int cid, int enable);

    void load();

    int m_type;
    std::map<int, stand_in_get> m_stand_ins;            //��������
};

//״̬����
struct StateData
{
    int m_state1;
    int m_state2;
    int m_state3;
};

//ϴ��ӵ��Ӧ����
struct baseWashPer
{
    int add;
    int per[3];//�ӵ��Ӧ����0����1����2���
};

//����װ������
struct baseEquipment
{
    int baseid;        //װ������id
    int type;        //���� 1 ����  2 ���� 3������ 4������ 5������
    int slot;
    int quality;    //Ʒ�� 1 ��ɫ 2����ɫ 3����ɫ 4����ɫ 5����ɫ
    int up_quality;    //ǿ��Ʒ��
    int baseValue;    //��������ֵ
    int baseValue2;//��������ֵ
    int basePrice;    //�������ۼ۸�
    int needLevel;    //װ����Ҫ�ȼ�
    std::string name;    //װ����
    std::string desc;    //װ������

    std::string link_name;

    boost::shared_ptr<lootPlaceInfo> m_place;
    boost::shared_ptr<baseEquipment> m_next;
};

//װ������
struct EquipmentData : public iItem
{
    EquipmentData(uint16_t type_, int id);
    int id;            //Ψһid
    int type;        //���� 1 ����  2 ���� 3������ 4������ 5������
    int baseid;        //����id
    //int slot;        //װ����λ��
    int quality;    //Ʒ��
    int up_quality;//ǿ��Ʒ��
    int cid;        //��ɫ
    int qLevel;        //ǿ���ȼ�
    int value;        //����ֵ
    int value2;

    int addValue;    //���ӵ����Ե�
    int addValue2;

    int price;        //�۸�

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

//��ɫ����װ��
struct CharEquipments
{
    //boost::shared_ptr<EquipmentData> m_equiped[5];    //װ�������ϵ�
    std::list<boost::shared_ptr<EquipmentData> > m_selled_equips;    //�ع���
    int m_equipment_counts;

    std::map<int, boost::shared_ptr<EquipmentData> > equipments;
    int list(int page, int nums_per_page, int max_pages);        //��ʾ��Ұ��������װ��

    //װ��ǿ��
    int levelUp(int eid);
    //��װ��
    int sellEquipment(int eid, int& price);
    //�ع�װ��
    int buybackEquipment(int id, int& price);
    //��ʾ�ع�
    int listBuyBack(int page, int nums_per_page, json_spirit::Object& o);
    int addToSelled(boost::shared_ptr<EquipmentData> eq);
    int removeFromSelled(boost::shared_ptr<EquipmentData> eq);
    int load(int cid);
    boost::shared_ptr<EquipmentData> getEquipById(int id);

    bool haveEquip(int type);
    //ĳ��װ������ߵȼ�
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
    int usage;        //��; 8��ʾװ����������
    int value;        //     ��;8ʱ��ʾ������װ��id
    int quality;

    int sellPrice;

    int gold_to_buy;    //��ҹ���۸�

    int max_size;
    int invalidTime;    //����ʧЧʱ��
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

//��ɫ����
struct CharBackpack
{
    CharData& m_chardata;
    int m_max_size;        //�ֿ��С,����ǿ��ʯͷ
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
    //��ʾ����װ���б�
    int listInheritEquipt(int type, int id, int page, int nums_per_page, int& max_page, json_spirit::Array& elists);
    //bool b_changed;
};

//������ְ
struct baseoffical
{
    int m_id;
    std::string m_name;
    int need_prestige;
    int m_salary;
    int m_sid;//�����ļ���id
    int m_fid;//����������pos
};

//������ְ����
struct baseofficalskill
{
    int m_sid;
    int m_spic;
    int m_type;//�ӳ�����
    int m_add_per_level;
    std::string m_effect;
    std::string m_name;
    std::string m_memo;
};

//��ɫ��ְ�佫
struct officalgenerals
{
    int m_gid;
    int m_price;
    int m_sid;
    int m_spic;
    int m_quality;
    int need_offical;    //��Ҫ�Ĺ�ְ
    int need_slevel;    //��Ҫ�Ĺؿ����ȵȼ�
    int m_good_at;        //�ó�
    bool m_special;    //������ļ�佫
    std::string m_name;
};

//��������
struct BaseZhenData
{
    int m_type;         //����id
    std::string m_name; //������
    int m_open_pos[5];
};

//��������
struct ZhenData
{
    int m_cid;          //��ɫid
    int m_zhen_type;    //����
    int m_level;

    int m_generals[9];  //��λ�õ��佫
    int m_org_attack;     //��ʼ��������ȥ����Ч��
    //��ս��ϵͳ
    int m_general_score;//�佫����
    int m_general_power;//�佫ս��
    int m_equip_score;//װ������
    int m_equip_power;//װ��ս��
    int m_wash_score;//ϴ������
    int m_wash_power;//ϴ��ս��
    int m_baoshi_score;//��ʯ����
    int m_baoshi_power;//��ʯս��
    int m_reborn_score;//��������
    int m_reborn_power;//����ս��
    int m_level_score;//�ȼ�����

    int m_weapon_power;//�ط�ս��

    int m_soul_power;    //����ս��

    int m_jxl_power;    //����¼ս��

    int m_gsoul_power;  //����ս��

    bool m_isnpc;       //������ŵ���StrongholdGeneralData����CharGeneralData
    bool m_changed;     //�иĶ�
    bool m_attack_change;//����׃��

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
        //����
        m_general_score = 0;
        m_equip_score = 0;
        m_wash_score = 0;
        m_baoshi_score = 0;
        m_reborn_score = 0;
        m_level_score = 0;
        //ս��
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
        //����
        m_general_score = 0;
        m_equip_score = 0;
        m_wash_score = 0;
        m_baoshi_score = 0;
        m_reborn_score = 0;
        m_level_score = 0;
        //ս��
        m_general_power = 0;
        m_equip_power = 0;
        m_wash_power = 0;
        m_baoshi_power = 0;
        m_reborn_power = 0;
        m_soul_power = 0;
    }
    int Save();     //����

    int updateAttack();    //������
    void updateNewAttack();

    int getAttack();

    void set_attack_change();

    void getList(json_spirit::Array& hlist);

    void getList2(json_spirit::Array& hlist);
    int getGeneralCounts();
};

//�������
struct CharZhens
{
    CharData& m_charData;
    std::map<int, boost::shared_ptr<ZhenData> > m_zhens;
    int m_default_zhen;
    int m_cid;      //��ɫid

    CharZhens(int cid, CharData& c)
    :m_charData(c)
    {
        m_cid = cid;
    };
    int Load();
      int Swap(int zhenid, int pos1, int pos2);   //��������λ�õ��佫
    int Up(int zhenid, int pos, int gid);        //�佫����
    int Up(int zhenid, int gid);          //�佫����,ֻҪ�п�λ����

    int Down(int zhenid, int pos);              //�佫����
    int Down(int gid);              //���������еĸ��佫����,���ڽ���佫ʱ�Ĵ���

    bool Check(int gid);              //�����������Ƿ��и��佫

    int SetDefault(int zhenid);                 //����ȱʡ����
    int GetDefault();                           //ȱʡ����
    boost::shared_ptr<ZhenData> GetZhen(int zhenid);        //��ȡ������Ϣ
    int Save(int type = 0);
    int Levelup(int type, int level);
    void setLevel(int type, int level);

    void set_attack_change();

    int m_changed;          //�иĶ�
    int m_changed_default;  //ȱʡ�����иĶ�
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

//����佫����
struct CharGeneralData: public boost::enable_shared_from_this<CharGeneralData>
{
    int m_id;       //Ψһid
    int m_cid;      //������ɫid
    int m_gid;      //�佫id
    int m_spic;     //ͷ�����
    int m_stype;    //����id
    int m_level;    //�ȼ�
    int m_str;      //����
    int m_int;      //����
    int m_tongyu;   //ͳ��
    int m_color;    //�佫��ɫ

    double m_chengzhang;        //�ɳ�ϵ��
    boost::shared_ptr<baseChengzhangStars> m_chengzhang_star;
    boost::shared_ptr<baseChengzhangStars> m_chengzhang_next_star;
    double m_add;    //�������ӵ���������ͬ��
    int m_reborn_point;    //��������
    int m_wash_str;    //ϴ�����ӵ���
    int m_wash_int;    //ϴ�����ӵ���
    int m_wash_tong;    //ϴ�����ӵ�ͳ
    int m_tmp_wash_result[3];//ϴ��֮��ûȷ�ϵ�������ʱ����
    boost::shared_ptr<baseWashStars> m_wash_star;
    boost::shared_ptr<baseWashStars> m_wash_next_star;
    int m_state;        // 0 ����  1 �ع���
    std::string m_baowu;    //��������
    int m_baowu_level;        //����ȼ�
    int m_baowu_type;        //������������1��2��3ͳ
    int m_baowu_add;        //����ӳ�������ֵ
    time_t m_delete_time; //�ع���ֹʱ��
    int m_reborn_times;        //�佫��������
    int m_wash_times;    //�佫ϴ�����
    std::string m_link_name;    //�����ӵ�����
    std::string m_color_link;    //����ɫ�����ֿ�����

    //std::vector<boost::shared_ptr<baowuBaoshi> > m_baoshi;    //���ﱦʯ

    std::vector<int> m_genius;                    //�츳�б�
    bool m_genius_lock[iGeniusMaxNum];        //�츳�������
    int m_genius_count;    //�츳����

    int b_nickname;//�º��Ƿ񼤻�0��1��

    //�ؼ�������
    combatAttribute m_combat_attr;
    std::list<combatSpeSkill> m_more_damage_skills;    //���˺�ս���ؼ�
    std::list<combatSpeSkill> m_attack_skills;            //�仯������ʽ���ؼ�

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

    //��ս��ϵͳ
    bool general_change;
    bool equip_change;
    bool wash_change;
    bool baoshi_change;
    bool reborn_change;
    int m_general_score;//�佫����
    int m_general_power;//�佫ս��
    int m_equip_score;  //װ������
    int m_equip_power;  //װ��ս��
    int m_wash_power;   //ϴ��ս��
    int m_baoshi_power; //��ʯս��
    int m_reborn_power; //����ս��
    int m_soul_power;   //����ս��
    int m_jxl_power;    //����¼ս��

    int m_gsoul_power;  //����ս��
    base_general_soul*  m_general_soul;
    //����ս����ֵ
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
    int m_changed;  //�иĶ�
    int Levelup(int level = 9999);  //����
    int SetColor(bool update_color = false);//�佫�������Գ���һ����ʼ��ʾ��ɫ
    int Save();     //����

    void toObj(json_spirit::Object& obj);
    //������
    void updateAttribute();
    void addToList(base_genius Genius, int type);
    //�����츳�ӳ�
    void updateGeniusAttribute();
    //ϴ�蹫��
    void broadWashMsg(int ret);
    //��������
    void broadRebornMsg(int ret);
    void updateEquipmentEffect();
    void updateBaoshiCount();

    //�佫���ֱ�����Ӹ�ʽ
    std::string NameToLink();

    std::string colorLink()
    {
        return m_color_link;
    }

    //�Ƴ����б�ʯ
    void removeAllBaoshi();

    //����
    int equip(int slot, int eid);
    //ж��
    int unequip(int slot);
    //�Ƴ�����װ��
    int removeAllEquipment();

    //����װ���ļ��б�
    void getList(json_spirit::Array& elist);

    //���±�ʯ���佫����
    void updateBaoshiAttr();
    //���������Ǽ�
    void updateChengzhangStar();
    //����ϴ���Ǽ�
    void updateWashStar(bool act = false);
};

//����佫����
struct CharTotalGenerals
{
    std::map<int, boost::shared_ptr<CharGeneralData> > m_generals;
    std::map<int, boost::shared_ptr<CharGeneralData> > m_fired_generals;
    CharData& m_charData;
    int m_cid;              //��ɫid

    CharTotalGenerals(int cid, CharData& cdata)
    :m_charData(cdata)
    {
        m_cid = cid;
        //Load();
    };
    //�����佫�������佫id
    int GetGeneralByType(int gtype);
    //�����佫�������Ͳ����佫
    int GetFiredGeneralByType(int gtype);

    //�ﵽһ���ȼ����佫����
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
    //ɾ���佫
    int deleteGenral(int id);
    //�޸��佫����
    int modifyGeneral(int id, int t, int z, int y);
    //�������ϴ��ӵ�
    int clearWash();
    //�޸��佫�ɳ�
    int modifyGeneralGrowth(int id, double fac1);
    EquipmentData* getEquipById(int id);

    int Save(); //����
    bool m_changed; //�иĶ�
};

//��ҹؿ���������
struct CharStrongholdData
{
    boost::shared_ptr<StrongholdData> m_baseStronghold;

    int m_state;//�ؿ�״̬-2�������ڣ�-1����δ������������ܹ�������

    int m_cid;            //��ɫid

    //״̬
    npcStrongholdStates m_states;

    //�ؿ�״̬��ս������
    combatAttribute m_combat_attribute;

    CharStrongholdData(int cid, int id, int level, int num)
    :m_cid(cid)
    ,m_states(cid, id, level, num)
    {};
    //��ȡ����״̬
    int getStates(json_spirit::Array& states);

    void save_state();
};

//��ҳ�����������
struct CharStageData
{
    int m_cid;
    int m_finished;//�Ѿ���ɵĹؿ���

    boost::shared_ptr<baseStage> m_baseStage;
    std::list<boost::shared_ptr<Item> > m_fall_list;//��������

    boost::shared_ptr<CharStrongholdData> m_stronghold[25];

    int m_cur_group;    //��ǰ�ڴ�ڼ���

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

//��ҽ�������
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
    //�����ɫ�ؿ�����
    int InitCharTempo(int mapid);
    int update(int stronghold, bool bBroad);
    int Save();
    int reset();
    int get_stage_finish_loot(int mapid, int stageid , json_spirit::Object& robj);
    bool check_stage_finish(int mapid, int stageid);
    bool check_stronghold_can_sweep(int mapid, int stageid, int pos);
};

//��½����
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

//������½����
struct CharLoginPresent
{
    int cid;
    int state;
    baseLoginPresent* present;
};

//VIP����[���������������һ��ʯ]
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
    std::list<Item> m_list;        //�����б�
};

//���VIP������ȡ���
struct CharVIPPresent
{
    int cid;
    int state;
    boost::shared_ptr<baseVIPPresent> present;
};

//���γ�ֵ������
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

//GM�Զ����Ž���
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
    int friend_state;//0�����1��ʽ
    int flag;//0��ʼ1�ı�2ɾ��
    boost::shared_ptr<CharData> f_charData;
    CharFriend(boost::shared_ptr<CharData> c, int cid)
    :f_charData(c)
    {
        friend_id = cid;
        friend_state = 0;
        flag = 0;
    }
};

//�ǳ�
enum nick_enum
{
    nick_race_start = 1,//�����������ǳ�1-5
    nick_race_end = 5,
    nick_ranking_start = 6,//�����е�һ�ǳ�
    nick_ranking_end = 20,
    nick_throne_start = 21,//�������������ǳ�
    nick_throne_end = 25,
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

//��ɫ����
struct CharData:public boost::enable_shared_from_this<CharData>
{
public:
    static volatile uint64_t _refs;
    static uint64_t refs();
    bool m_load_success;
    int m_id;   //��ɫid
    int m_state;//״̬ 0 ���� 1 ��ɾ��
    time_t m_deleteTime;    //ɾ��ʱ��
    int m_level;            //��ɫ�ȼ�
    time_t m_createTime;    //��ɫ����ʱ��
    time_t m_levelupTime;    //��ɫ����ʱ��

    int m_union_id;
    std::string m_account;    //�˻�
    std::string m_qid;    //ƽ̨id
    std::string m_server_id;
    std::string m_name;//��ɫ����
    int m_spic; //��ɫͷ��
    int m_gender;//�Ա� 1�� 0Ů
    int m_vip;  //vip�ȼ�
    int m_camp; //��Ӫ1�ٸ�2����
    int m_currentStronghold;    //�ؿ�����
    std::string m_chat;//ս����������

    int m_area;         //��ɫ���ڵ���
    std::string m_area_name;
    std::string m_area_memo;
    int m_cur_stage;         //��ɫ���ڳ���

    int m_tmp_vip;    //��ʱVIP
    int m_tmp_vip_start_time;    //��ʱVIP��ʼʱ��
    int m_tmp_vip_end_time;    //��ʱVIP����ʱ��

    int m_qq_yellow_level;    //QQ����ȼ�
    int m_qq_yellow_year;        //�Ƿ�QQ��ѻ���
#ifdef QQ_PLAT
    std::string m_iopenid;                //QQƽ̨�Ƽ����
    std::string m_feedid;                //QQƽ̨����id
    std::string m_login_str1;    //QQƽ̨��¼��Ϣ
    std::string m_login_str2;    //QQƽ̨��¼��Ϣ
#else
    std::string m_vcode;
    std::string m_sid;
#endif
    nick m_nick;
    void SaveNick();
    std::string getUserMsg();
private:
    //��Դ
    int m_gold;    //���
    int m_silver;  //����
    int m_ling;    //����
    int m_explore_ling;//̽����

public:
    //��ť����1����0�ر�
    uint8_t m_panel_zhuj;      //���°�ť 2
    uint8_t m_panel_army;      //���Ӱ�ť 3
    uint8_t m_panel_junt;      //���Ű�ť 1
    uint8_t m_panel_interior;    //������ť 4

    //���ܿ���1����0����
    uint8_t m_weaponOpen;    //������
    uint8_t m_eliteOpen;    //��Ӣս��
    uint8_t m_raceOpen;    //����
    uint8_t m_skillOpen;    //����
    uint8_t m_skillTrainOpen;//����ѵ��
    uint8_t m_baowuOpen;    //����ϵͳ����(��Ƕ)
    uint8_t m_equiptOpen;    //װ��
    uint8_t m_equiptEnhanceOpen;//װ��ǿ��
    uint8_t m_zhenOpen;    //����
    uint8_t m_trainOpen;    //�佫ѵ��
    uint8_t m_farmOpen;    //����
    uint8_t m_exploreOpen;//̽��
    uint8_t m_shopOpen;    //�̵�
    uint8_t m_tradeOpen;    //ͨ�̿���
    uint8_t m_washOpen;    //ϴ��
    uint8_t m_recruitOpen;//��ļ�佫
    uint8_t m_sweepOpen;    //ɨ����Ӣ
    uint8_t m_horseOpen;    //ս��
    uint8_t m_servantOpen;//�Ҷ�
    uint8_t m_levyOpen;    //���տ���
    uint8_t m_guardOpen;    //��������
    uint8_t m_officeOpen;    //��ְ����
    uint8_t m_corpsOpen;    //���ſ���
    uint8_t m_bossOpen;    //boss����
    uint8_t m_helperOpen;    //���ֿ���
    uint8_t m_campraceOpen;//��Ӫս����
    uint8_t m_rebornOpen;    //��������
    uint8_t m_buyLingOpen;    //��������
    uint8_t m_rankEventOpen;    //�����л����
    uint8_t m_bankOpen;    //Ǯׯ����
    uint8_t m_soulOpen;    //��������
    uint8_t m_sevenOpen;    //7��Ŀ�꿪��
    uint8_t m_jxlOpen;        //����¼����
    uint8_t m_generalSoulOpen; //���꿪��

    time_t m_wash_open_time;    //ϴ�迪��ʱ��
    int m_wash_event_state;    //ϴ��״̬
    time_t m_daily_wash_times;//ÿ������ϴ�����
    int m_wash_event[10];    //����ϴ����ȡ���

    time_t m_login_time;            //��¼ʱ��
    bool m_can_world_chat;          //�Ƿ������������
    bool m_can_chat;                //�Ƿ񱻽���

    bool m_check_chenmi;    //�Ƿ���Ҫ��������
    uint64_t m_chenmi_time; //�ۼƵĳ���ʱ������
    time_t m_notify_chenmi_time;    //֪ͨ������Ϣ��ʱ��
    time_t m_chenmi_start_time;     //�����Կ�ʼʱ��

    time_t m_fight_cd;        //ս����ȴʱ��
    time_t m_enhance_cd;    //ǿ��װ����ȴʱ��
    bool m_can_enhance;        //�Ƿ��ǿ��

    //�����Ѿ������Ϣ�Ĵ���
    int m_gold_rest;
    //���Ϣʣ�����
    int m_free_rest;
    time_t m_free_rest_time;

    //����̽�����ˢ�´���
    int m_explore_refresh_times;
    //����ǰ2��̽��
    int m_first_explore;
    int m_second_explore;

    //����+װ��
    int m_total_pugong;
    int m_total_pufang;
    int m_total_cegong;
    int m_total_cefang;
    int m_total_bingli;

    bool m_weapon_attack_change;        //�b��ͱ���׃��

    //����
    std::map<int, boost::shared_ptr<charSkill> > m_skill_list;
    int m_skill_power[5];        //���ܶ�ս����ֵ�ļӳ� �������չ����շ����߹����߷�

    //�����о�����
    std::vector<skillResearchQue> m_skill_queue;

    //�����о���
    boost::shared_ptr<skillTeacher> m_teachers[skill_teacher_nums];
    bool m_teachers_change;    //�з�仯

    //ѵ������
    std::vector<generalTrainQue> m_train_queue;
    //�����б�
    boost::shared_ptr<Book> m_book[general_book_nums];
    bool m_book_change;

    //ս������
    combatAttribute m_combat_attribute;

    //�ؿ�����
    CharTempoData m_tempo;
    //�ؿ�״̬
    StrongholdsStatesData m_strongholdStates;

    //��ɫ����
    //CharBackpack m_backpack;
    Bag m_bag;
    selledBag m_selled_bag;

    //�佫�б�
    CharTotalGenerals m_generals;

    //����
    CharZhens m_zhens;//��������

    int m_hp_cost;        //��Ƿ��ļ������

    charTask m_task;    //��ɫ����
    charTrunkTasks m_trunk_tasks;    //֧������
    void init_task_done();

    //��ְ���
    int m_prestige;    //����
    int m_offical;        //��ְ
    std::string m_offical_name;    //��ְ��
    int m_salary;//ٺ»
    int m_hasgetsalary;//�����Ƿ���ȡ��ٺ»0û��ȡ1�Ѿ���ȡ
    bool m_officalcanlevelup;//��ְ�ɷ�����

    int m_buy_xisui_time;//����ϴ�������ÿ����
    int m_farm_harvest;//�����ջ������ÿ������
    int m_levy_time;//���մ���ÿ����

    int m_gold_train_horse;//ս����������
    int m_silver_train_horse;//ս����������
    int m_welfare;//������ȡ���

    //�������ȷ��
    int m_gold_cost_comfirm[iMaxGoldCostConfirm];

    //�ۼƳ�ֵ���
    int m_total_recharge;
    //�������ӵ�VIP����
    int m_vip_exp;

    boost::shared_ptr<corps_member> m_corps_member;    //���ų�Ա
    std::list<boost::shared_ptr<corps_application> > m_corps_applications;    //��������

    //����������ȡ���
    std::map<int,int> m_map_intro_get;

    //��������������
    std::map<int,time_t> m_guide_completes;
    int m_current_guide;        //��ǰ��������

    //���ֳ��Ž������ first=�ȼ�
    std::map<int,bool> m_newbie_reward;
    int m_newbie_reward_canGet;

    //�ɳ�����������
    std::vector<int> m_chengzhang_reward;

    //���ʳ�ֵ�������
    //std::map<int,int> m_recharge_reward;

#if 0
    //������¼����
    std::map<int,CharLoginPresent> m_login_present;
#endif
    //������¼(20����ʼͳ��)
    int m_continue_days;//������¼����(��ȡ�������)
    int m_total_continue_days;//������������¼ͳ��

    //VIP������ȡ���
    std::map<int,CharVIPPresent> m_vip_present;

    int m_last_stronghold;    //��һ������Ĺؿ�id
    int m_reget_times;            //�ٴλ�ô���
    int m_reget_gold;            //�ٴλ����Ҫ�Ľ��
    int m_last_stronghold_mapid;
    int m_last_stronghold_level;
    int m_last_stronghold_type;

    int m_copy_id;        //���ڵĸ���id
    int m_copy_id_leave;    //����ʱ�ĸ���id

    time_t m_test_recharge_time;    //�´ο��Բ��Գ�ֵ��ʱ��
    int m_total_test_recharge;    //�ۼƲ��Գ�ֵ����
    //�ϴδ���ʱ��
    time_t m_save_time;

    charShop m_shop;//����̵�

    int m_temp_jisi_times;    //�������(����) - ��ʱ -ÿ�����
    int m_temp_corps_yanhui;    //���������� - ÿ�����

    CharNewWeapons m_new_weapons;
    CharHorse m_horse;

    //CharTrainings m_training;//����ϵͳ

    int m_current_map;            //��ǰ�򿪵�ͼ
    int m_current_stage;        //��ǰ�򿪳���

    //ͨ����Ϣ
    int m_trade_state;
    int m_tradeque_type;
    int m_tradeque_id;
    int m_tradeque_pos;

    std::string m_ip_address;    //ip��ַ

    int m_temp_score;    //��ʱ���ʺŵ�����

    std::map<int, boost::shared_ptr<baowuBaoshi> > m_baoshi;    //���ﱦʯ

    std::map<int, int> m_normal_extra_data;
    std::map<int, int> m_daily_extra_data;
    std::map<int, int> m_week_extra_data;

    newCharStates m_newStates;    //��״̬

    CharBuffs m_Buffs;    //��ʱ����ӳ�

    int m_general_limit;    //�佫��������

    int m_enhance_state;    //ǿ������״̬
    int m_enhance_silver;    //ǿ��װ����Ҫ������
    int m_enhance_eid;        //�ܹ�ǿ����װ��id

    int m_upgrade_weapon_state;    //�����ط�״̬
    int m_upgrade_weapon_gongxun;//�����ط���Ҫ�Ĺ�ѫ
    int m_upgrade_weapon_type;    //���������ط�������

    bool m_open_info_change;    //���б仯
    bool m_formation_change;    //�·��������ݱ仯
    bool m_char_data_change;    //��ɫ��Ϣ�����仯

    int m_change_spic;            //��������
    time_t m_change_spic_time;//�������ʱ��

    int m_is_online;            //�Ƿ�����

    int m_up_generals;        //��������

    std::map<int, boost::shared_ptr<CharData> > m_recommend_friends;
    time_t m_recommend_friend_refresh;

    int m_close_friend_to;    //˭������

    //��Ҫ֪ͨ��ҵ�����
    std::map<int, int> m_need_notify;

    char_jxl_buff m_jxl_buff;

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
    //��ɫ���ڵ������
    int GetArea();

    //�佫��Ϣ
    CharTotalGenerals& GetGenerals();

    //������Ϣ
    CharZhens& GetZhens();

    //������Ϣ
    CharTempoData& GetTempo();

    //��ȡ��ǰ�������޺�����
    void GetMaxSupply(int& need_supply, int& max_supply);

    //���빫��
    int JoinGuildId(uint64_t gid);
    //�뿪����
    int LeaveGuild();

    //��ɫ�ȼ�
    int level(int level);

    int addGold(int gold);      //���Ӽ��ٶ����ԣ�����-1��ʾ���������������ر仯�������
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

    //�׳�
    int action_first_recharge(int gold_num);

    int getCharInfo(json_spirit::Object& charobj);
    //֪ͨ��Ϣ�仯
    int getPanelInfo(json_spirit::Object& panelobj);
    int NotifyCharData();
    //֪ͨ�ͻ��˽�ɫ��Ϣ�����仯
    int NotifyCharData_(net::session_ptr& sk);

    int NotifyCharOpenInfo();
    void notifyOpeningState();
    //֪ͨVIP���ť��״̬
    void notifyVipState();
    void notifyChengzhangState();

    //��ѯVIP�״̬
    int getVipState();

    //��ѯ�����״̬
    int getOpeningState();
    //��ѯ�ճ��״̬
    int getDailyState();

    void notifyEventState(int type, int active, int leftNums);
    //֪ͨ������ť�Ƴ�
    void notifyEventRemove(int type);

    //֪ͨ�ͻ��˽�ɫ������Ϣ�����仯
    //int NotifyCharExp();
    //��ɫ״̬�仯
    int NotifyCharState();

    //type: 0��ʾԭ���ӿڣ�1,��ʾ��ͨ�ؿ� 2��ʾ��Ӣ�ؿ���id��ʾ�ؿ�id
    int getZhenGeneral(int zhenid, json_spirit::Object& robj, int type = 0, int id = 0);
    int NotifyZhenData();

    int sendObj(json_spirit::Object& obj);

    //��ʾ��������
    int showBackpack(int page, int pagenums, json_spirit::Object& o);
    //��ʾ����װ��
    int getEquiped(int gid, json_spirit::Object& robj);
    //��ʾ����װ����ϸ��Ϣ
    int getEquipmentInfo(int id, json_spirit::Object& robj);
    //��ʾ���ϵ�����ϸ��Ϣ
    int getTreasureInfo(int id, int nums, json_spirit::Object& robj);
    //���۵���
    int sellTreasure(int id, int count, json_spirit::Object& robj);
    //���ӽ�ɫװ��
    int addEquipt(int id, std::string& msg, bool notify = true);
    //���ӽ�ɫ����
    int addTreasure(int id, int counts);
    int addTreasure(int id, int counts, int& err_code);
    //��ɫ���ߔ���
    int treasureCount(int id);
    //װ������
    int equip(int gid, int slot, int eid);
    //һ��װ��
    int onekeyEquip(int gid);
    //ж�¶���
    int unequip(int gid, int slot);
    //���װ��ǿ����Ϣ
    int getEquipmentUpInfo(int eid, json_spirit::Object& obj);
    //���ǿ����ȴ
    int enhanceSpeed();
    //ǿ����Ϣ
    int enhanceEquipment(int eid, json_spirit::Object& robj);
    //����ǿ���ȼ�
    int setEquipmentLevel(int id, int level);
    //���ǿ��װ���б�
    int getEnhanceEquiptlist(int page, int pagenums, json_spirit::Object& obj);
    //��װ��
    int sellEquipment(int id, json_spirit::Object& robj);
    //�ع�װ��
    int buybackEquipment(int id, json_spirit::Object& robj);
    //��ʾ�ع�
    int listSelledEquipment(int page, int page_per, json_spirit::Object& o);

    //�����о���ʱ�ӵ�
    int skillResearchAdd(int sid, int times);
    //�����о���ʼ
    int startSkillResearch(int sid, int teacher);
    //ֹͣ�����о�
    int stopSkillResearch(int sid);
    //ֹͣȫ�������о�
    int stopSkillResearchAll();
    //���ܵ��䣬��������
    int skillLevelup(int sid, int level);
    //���ü��ܵȼ�
    int setSkillLevel(int sid, int level);

    //����о�����Ϣ
    int getTeacherList(json_spirit::Object& o);
    //��ü�����ϸ��Ϣ
    int getSkillDetail(int sid, json_spirit::Object& o);
    //��ü����б�
    int getSkillList(json_spirit::Object& o);
    //��ü����о��б�
    int getSkillResearchList(json_spirit::Object& o);
    //��ü��ܵȼ�
    int getSkillLevel(int sid);
    //��ü����о��б�
    int getSkillResearchInfo(int sid, json_spirit::Object& o);

    //ˢ���о���
    int updateTeachers(json_spirit::Object& o, int type = 1);
    //����
    int buyResearchQue(json_spirit::Object& o);
    //��������
    int upgradeResearchQue(json_spirit::Object& o);
    //�����о�
    int speedupResearch(int sid, int type, json_spirit::Object& o);

    //���ѵ���б�
    int getTrainList(json_spirit::Object& o);
    //��ñ����б�
    int getBookList(int id, json_spirit::Object& o);
    //�佫ѵ��
    int generalTrain(int gid, int bid, int pos, json_spirit::Object& o);
    //��������
    int upgradeGeneralTrainQue(int pos, json_spirit::Object& o);
    //ˢ�±���
    int updateBooks(int type, json_spirit::Object& robj);
    //����ѵ��λ
    int buyTrainQue(int pos, json_spirit::Object& robj);
    //����ѵ��λ���ٴ���
    void resetTrainQue();
    //����ѵ���б�
    int LoadTrainList();
    //ѵ��λ����CD
    int generalTrainSpeed(int pos);
    int generalTrainCoolTime(int& state);

    //�����佫ѵ��
    int openTrain();

    //�������
    int generalInheritObj(int gid1, int gid2, int type, json_spirit::Object& o);
    int generalInheritInfo(int gid1, int gid2, json_spirit::Object& o);
    int generalInherit(int gid1, int gid2, int type, json_spirit::Object& o);
    int buyInherit(int num);

    //�Ƿ����и��佫
    bool CheckHasGeneral(int base_id);
    //����б��Ƿ��и��佫
    bool CheckHasFireGeneral(int base_id, int& general_id);
    //�Ƿ��и��佫(��͵�Ҳ��)
    bool HasGeneral(int base_id);
    //�Ƿ����и�װ��
    bool CheckHasEquipt(int base_id);

    int getPugong(bool load_horse);
    int getPufang(bool load_horse);
    int getCefang(bool load_horse);
    int getCegong(bool load_horse);
    int getBingli(bool load_horse);
    //ˢ��״̬
    int refreshStates(int type, int id = 1);
    //��ȡս��
    int getHorse(json_spirit::Object& horse);
    //���¹���
    int updateAttackDefense();
    //ս�����ı���/����
    int combatCost(bool win, int type);
    //��Ϣ����
    int getRestCost(int times);
    //��Ϣ
    int rest(int type, json_spirit::Object& robj);
    //��ѯ��Ϣ��Ϣ
    int queryRestInfo(json_spirit::Object& robj);
    //��������
    int updateTask(int type, int n1 = 0, int n2 = 0);
    //�������
    int checkTask();
    //��ְ����
    int OfficalLevelUp();
    //���¹�ְ������Ϣ
    bool OfficalLevelUpState();

    //�����ְ�佫
    int buyOfficalGeneral(int gid);
    //�ܷ����ְ�佫
    bool canBuyOfficalGeneral(int gid);

    //������Ʒ
    int buyShopGoods(int pos, json_spirit::Object& o);
    int refreshShopGoods(int type);
    //��ȡ��ͼ���Խ���
    int getMapIntroReward(int mapid, json_spirit::Object& o);

    //����
    int HeartBeat();
    //����
    int Save();
    void SaveWeapons(int);
    //�����ճ�����
    int saveCharDailyVar();
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

    //��ѯ����������
    time_t getGuideState(int id);
    //�����������
    void setGuideStateComplete(int id, int next_guide);

    //����Ƿ񴥷�����
    int checkGuide(int type, int param1, int param2);
    //����Ƿ񴥷�����
    int _checkGuide(int id);
    //���ս����ʾ����
    int _checkNotifyFail(int strongholdid);

    //����vip�ȼ�
    int updateVip();
    //��ֵ���������ж�
    //int updateRechargeReward(int num, time_t t = 0);

    //��ȡ��������ȴ���
    int getUpdateListCD(json_spirit::Object& robj);
    //��ȡ���ܿ�����Ϣ
    int getOpeninfo(json_spirit::Object& robj);
    //��ȡϴ�����
    int getWashPer(int type, int add);
    //��ȡϴ�����
    int WashInfo(int gid, json_spirit::Object& robj);
    //ϴ��
    int Wash(int gid, int type, json_spirit::Object& robj);
    int WashConfirm(int gid);

    //�����������ʱ����佫��װ��
    int ClearData();
    //����Ƿ���Ҫ����Ĭ�ϱ���
    bool checkWeapon();

    //����ϵͳ�ļ��
    //bool checkSouls();

    //ĳ�ŵ�ͼ�Ƿ�ͨ����
    bool isMapPassed(int mapid);

    //��Ǩ��ͼ֪ͨ
    void notifyChangeMap();

    //���ǳ������غ�Ļ��
    int chenmiGet(int input);
    //������ս������
    int getLevyCost(int times);
    //����������ҵ��ν���
    int getLevyReward();
    //���ñ����ȼ�
    int setWeaponLevel(int type, int quality, int level);
    //��û��ť����������
    bool getActionActive();
    int sellBagSlot(uint8_t slot, json_spirit::Object& robj);

    //���¹��ܿ���
    int updateOpen();

    //����ǿ����Ҫ����������
    void updateEnhanceCost();

    //����ǿ������״̬
    void updateEnhanceCDList();

    //����ǿ���ط���Ҫ�����ٹ�ѫ
    void updateUpgradeWeaponCost();

    //�����ط�����״̬
    void updateUpgradeWeaponCDList();

    //֪ͨ������Ϣ
    void realNotifyOpenInfo(net::session_ptr& sp);

    //����
    int getChangeSpic();

    //ĳ��װ������ߵȼ�
    int maxEquipLevel(int type);

    //��ҿ�ʼ��ʱVIP
    int startTmpVIP();

    //ȫ�����VIP 4�
    int startTmpVIP2();

    //������������
    void friendslevelup(int level);
    //��ȡ��Һ����б�
    void getFriendsList(int type, json_spirit::Object& robj);
    //�������
    int submitApplication(const std::string& name);
    //ͨ������
    int addFriend_id(int id);
    int acceptApplication(int friend_id, bool);
    int acceptAllApplication();
    //�ܾ�����
    int rejectApplication(int friend_id);
    int rejectAllApplication();
    //ɾ������
    int deleteFriend(const std::string& name);
    int deleteFriend_id(int id);
    //ȷ���Ƿ����
    bool check_friend(int id);

    void loadFriends();
    void saveFriends();

public:
    int getAttack(int zid = 0);        //��������ԃ
    void set_attack_change(bool e = false);    //�O��׃��
    int get_skill_attack();    //���܌������ӳ�
    int buff_attack(int damage_type, int attack, int hp, int wufang, int cefang);    //ս����������

    //��ԃ�����Ƕ��ʯ��Ӣ���б�
    int queryBaoshiGeneralList(json_spirit::Array& glist);
    //��ԃӢ�ی�ʯ
    int queryGeneralBaoshi(int gid, json_spirit::Object& robj);

    //��ԃ��ʯ����
    int queryBaoshi(int baoshi_id, json_spirit::Object& robj);

    //��ԃӢ�ی�ʯ�܌���
    int queryGeneralBaoshiInfo(int gid, json_spirit::Object& robj);

    //�Ƴ���ʯ
    int removeBaoshi(int gid, int slot);
    //�Ƕ��ʯ
    int xiangruBaoshi(int bagSlot, int gid, int slot);

    //���ͱ�ʯ
    int giveBaoshi(int type, int level = 1, int reason = 1);

    //����״̬�����ܵ�Ӱ��
    void updateCombatAttribute();

    int getRoleDetail(json_spirit::Object& robj);

    bool updatePanelOpen();

    //���򱳰�
    int buyBagSlot(int num, json_spirit::Object& robj);

    //����װ��
    int upgradeEquipment(int eid, bool cost_gold, json_spirit::Object& robj);
    //��ȡװ��������Ϣ
    int getUpgradeEquipmentInfo(int eid, int type, json_spirit::Object& robj);
    void getScrollTips(baseTreasure* bt, json_spirit::Object& robj);
    int openSlotItm(int slot, int nums, json_spirit::Object& robj);

    int updateNewbieEventState();

    int isNewPlayer();
    //��ȡ������ɫ����Ȼ����
    int queryCreateDays();

    bool isWashEventOpen();

    int updateWashEventState();

    //��ǰ����id
    int getCurrentGuide(int& state);

    int getGuideState1(int guide);

    int getGuideState2(int guide);
    //�ؿ�״̬
    int getStrongholdState(int strongholdId);

    //���ҹؿ�
    boost::shared_ptr<CharStrongholdData> getDestStronghold();
    //�����
    int openLibao(int slot, json_spirit::Object& robj, bool real_get = false);
    //ֱ�ӽ������
    int rewardLibao(int libao_id, json_spirit::Object& robj);
    //��ȡ�����Ϣ
    int getLibaoInfo(int libao_id, json_spirit::Object& robj);
    //�ɳ������Ϣ
    int getChengzhangLibaoInfo(int pos, json_spirit::Object& robj);

    //ע������
    int regDays();

    int m_baoshi_count;
    //���±�ʯ��-true ��ʯ�ױ仯��
    bool updateBaoshiCount();
    //�����佫�ı�ʯ��
    int updateGeneralsBaoshiCount();
    //�����Ƿ�������һ�
    bool canFindBack();
};

//������������
struct BaseSoldierData
{
    int m_stype;        //����id
    int m_base_type;    //�����������(1��2��3ʿ4��5��)
    int m_attack;        //�����﹥
    int m_wufang;        //�������
    int m_cefang;        //�����߷�
    int m_damage_type;    //�������(1������2���Թ���)
    int m_attack_type;    //������Χ(���繥��һ��)
    int m_attack_type2;//�������� Զ��(2)���ս(1)
    int m_attack_type3;//��ͨ���� Զ��(2)���ս(1)
    int m_damage_type2;//�˺���� 1��������,2��ǹ�̣�3������,4������,5��ħ������

    int m_special_attack_fac;    //��������

    boost::shared_ptr<baseState> m_pec[3];        //����(���繥������)
    int m_spic;            //����ͼƬ
    std::string m_name;    //������
    std::string m_desc;    //��������

    Object m_soldier_obj;

    //ս������
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

//�����佫����
struct GeneralTypeData
{
    int m_gid;         //�佫id
    int m_spic;        //ͼƬ
    int m_stype;       //����id
    int base_str;      //��������
    int base_int;      //��������
    int base_tongyu;   //����ͳ��
    //int m_equipt;        //�佫����ӳ�(����)
    int m_tianfu;        //�����츳(ͳ1����2����3)

    //2.14 ���츳
    newTianfu m_new_tianfu;

    int m_quality;        //Ʒ��

    int m_baowu_baseval;    //�����������
    int m_baowu_addperlev;    //����ɳ�ֵ
    int m_baowu_type;        //������������1��2��3ͳ
    int m_baowu_spic;    //����ͼƬ

    int m_spe_skill_id;    //���⼼��id
    int m_good_at;            //�ó�

    double m_base_chengzhang;    //�佫�����ɳ�
    int m_inherit_cnt;//���е�����

    std::string m_name; //�佫��
    std::string m_nickname; //�佫�º�
    std::string m_desc;     //����
    std::string m_baowu;    //��������
    std::string m_jxl;        //����¼��Ϣ

    boost::shared_ptr<specialSkill> m_speSkill;

    json_spirit::Array m_tj_baoshi_list;

    void toObj(json_spirit::Object& obj);
};


//�ؿ��佫����
struct StrongholdGeneralData
{
    int m_pos;            //λ��
    std::string m_name; //����
    int m_spic;         //ͼƬ
    int m_color;        //Ʒ��
    int m_stype;        //����id
    int m_level;        //�ȼ�
    int m_hp;           //Ѫ��
    int m_attack;       //����
    int m_pufang;       //�շ�
    int m_cefang;       //�߷�
    int m_str;          //����
    int m_int;          //����
    int m_special;        //����ආ����ϴ�
    boost::shared_ptr<specialSkill> m_speSkill;
    boost::shared_ptr<BaseSoldierData> m_baseSoldier;
};

struct strongholdReport
{
    uint64_t bid;        //ս��id
    std::string aName;    //����������
    int aLevel;            //�����ߵȼ�
    int attack;            //ս��ֵ-����
    int hurt;            //ս����ʧ����
    time_t attack_time;//����ʱ��

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
    strongholdReport m_first_report;                //�״ι����ս��
    strongholdReport m_best_report;                //��ѻ�ɱ
    //����һ������
    int addRecords(const std::string& name, int level, uint64_t bid, int attack = 0, int hurt = 0);
    void load(const std::string& name, int level, uint64_t bid, int attack, int hurt, time_t time);
    void getRadiers(json_spirit::Object& robj);
};

//��ͼ-����-�ؿ�(����ɫ���ƹ��������μ��佫)
//�ؿ�����
struct StrongholdData
{
    int m_id;           //�ؿ�id
    int m_level;        //�ؿ��ȼ�
    int m_map_id;        //������ͼ
    int m_stage_id;    //��������
    int m_strongholdpos;//����λ��
    int m_isepic;        //�Ƿ�Ӣ
    int m_spic;            //�ؿ�ͼƬ
    int m_color;      //Ʒ��
    int m_stateNum;    //״̬����

    int m_group;        //�ڼ���

    int m_model;        //��ͼ����ʾģ�ͱ��

    int m_x;            //��ͼ������
    int m_y;

    //int m_limits;        //��Ӣ�ؿ�������������

    int m_need_supply;    //ռ����Ҫ������
    int m_rob_supply;    //�Ӷ�õ�������
    int m_gongxun;        //�õ��Ĺ�ѫ
    int m_open_zhen;
    int m_open_zhen_level;
    int m_general_limit;    //�佫����
    int m_guide_id;    //����ؿ��Ķ�Ӧ����
    int m_guide_fail;   //ʧ�ܶ�Ӧ����

    std::string m_name;//������
    std::string m_chat;//����
    std::string m_failMsg;//ʧ����ʾ

    boost::shared_ptr<json_spirit::Object> m_loot;//�ؿ�����
    boost::shared_ptr<Item> m_Item;

    boost::shared_ptr<StrongholdGeneralData> m_generals[9];
    boost::shared_ptr<baseStage> m_baseStage;

    //�ؿ��Ŀ���
    combatAttribute m_combat_attribute;

    strongholdRaiders m_raiders;

    StrongholdData()
    {
        m_need_supply = 10;    //ռ����Ҫ������
        m_rob_supply = 50;    //�Ӷ�õ�������
        m_open_zhen = 0;
        m_open_zhen_level = 0;
        m_general_limit = 0;
    };
    int getAttack();
};

//��������
struct StageData
{
    int m_stage_id;//����id
    int m_mapid;//������ͼid
    int m_size;//������С
    int m_strongholds[25];//�����ؿ�
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
#else
    std::string m_vcode;
    std::string m_sid;
#endif
    time_t m_logintime;           // ��¼ʱ��
    boost::shared_ptr<OnlineCharactor> m_onlineCharactor;  //��ɫ
    std::list<CharactorInfo> m_charactorlist;              //��ɫ��Ϣ�б�

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
    int Chat(const std::string &, const std::string &, int type = 0, int gender = 0, int mod = 0, const std::string& nick = "[]");
    std::string genChatMsg(const std::string &, const std::string &, int type = 0);
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

struct baseStage;

//��ͼ
struct baseMap
{
    int id;
    std::string name;
    int openLevel;
    std::string memo;
    std::string intro;
    std::string get;
    boost::shared_ptr<baseStage> stages[3];    //��ͼ�еĳ���
};

//����
struct baseStage
{
    int id;
    int mapid;
    std::string name;
    int openLevel;
    int size;
    int spic;

    int stage_finish_guide;    //����ͨ�ؽ����������

    json_spirit::Array boss_list;

    void addLoot(boost::shared_ptr<Item> i, boost::shared_ptr<json_spirit::Object> o);

    boost::shared_ptr<json_spirit::Array> loots_array;        //����

    boost::shared_ptr<baseMap> _baseMap;
    boost::shared_ptr<StrongholdData> _baseStrongholds[25];
};

struct DateInfo
{
    int year;       //���
    int season;    //���� 1 - 4�������ﶬ
    int effect_type;   //����Ч��
    std::string effect; //����Ч��
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

//�̳��ۿۻ
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
    int GetStrongholdid(int mapid, int stageid, int strongholdpos);//��ȡ�ؿ�id��
    int GetStrongholdPos(int& mapid, int& stageid, int strongholdid);//��ȡ�ؿ�λ��
    int GetMapMemo(int mapid, std::string& name, std::string& memo);//��ȡ��ͼ����
    int GetStageLimitLevel(int mapid, int stageid);//��ȡ�����ȼ�����
    std::string GetStageName(int mapid, int stageid);//��ȡ��������
    boost::shared_ptr<BaseSoldierData> GetBaseSoldier(int sid);    //��ȡ����������
    boost::shared_ptr<GeneralTypeData> GetBaseGeneral(int gid);    //��ȡ�����佫����
    //��û���������Ϣ
    boost::shared_ptr<BaseZhenData> GetBaseZhen(int zid);
    //��ȡ�ɳ����Ǽ�
    boost::shared_ptr<baseChengzhangStars> GetBaseChengzhangStarByValue(double chengzhang);
    boost::shared_ptr<baseChengzhangStars> GetBaseChengzhangStarByStar(int star);
    //��ȡϴ���Ǽ�
    boost::shared_ptr<baseWashStars> GetBaseWashStarByValue(int score);
    boost::shared_ptr<baseWashStars> GetBaseWashStarByStar(int star);

    void addCharData(boost::shared_ptr<CharData> cdata);    //�����ɫ����
    boost::shared_ptr<CharData> GetCharData(int cid);    //��ȡ��ɫ����
    boost::shared_ptr<std::list<boost::shared_ptr<EquipmentData> > > GetCharEquiptData(int cid);    //��ȡ��ɫװ���б�
    boost::shared_ptr<CharGeneralsData> GetCharGenerals(int cid);    //��ȡ��ɫȫ���佫����
    boost::shared_ptr<ZhenData> GetCharDefaultZhen(int cid);//��ý�ɫĬ������
    boost::shared_ptr<CharStageData> GetCharStageData(int cid, int stageid);    //��ȡ��ɫ�ؿ�����
    boost::shared_ptr<CharStrongholdData> GetCharStrongholdData(int cid, int mapid, int stageid, int strongholdpos);    //��ȡ��ɫ�ؿ�״̬
    boost::shared_ptr<StrongholdData> GetStrongholdData(int strongholdid);    //��ȡ�ؿ�����
    boost::shared_ptr<StrongholdData> GetStrongholdData(int mapid, int stageid, int pos);    //��ȡ�ؿ�����

    boost::shared_ptr<StrongholdGeneralData> GetStrongholdGeneralData(int gid);    //��ȡ�ؿ��佫����
    //�������߽�ɫ��Ϣ
    boost::shared_ptr<OnlineCharactor> CreateOnlineCharactor(boost::shared_ptr<OnlineUser> account, uint64_t cid);
    //���߽�ɫ��Ϣ
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

    //��ý�ɫ��¼�б�
    //int GetCharactorInfoList(const std::string& account, std::list<boost::shared_ptr<CharactorInfo> >& charactorlist);
    //�����ɫ��Ϣ
    int SaveDb(int save_all);
    //�ظ���Ҿ���
    int addLing(int counts);

    //��û���װ��
    boost::shared_ptr<baseEquipment> GetBaseEquipment(int baseid);

    //��û�������
    boost::shared_ptr<baseTreasure> GetBaseTreasure(int tid);

    //��õ�ͼ
    boost::shared_ptr<baseMap> GetBaseMap(int mid);

    boost::shared_ptr<baseoffical> GetBaseOffical(int id);

    std::list<boost::shared_ptr<officalgenerals> >& GetBaseOfficalGenerals();
    std::list<Item>& GetFirstRechargeGift();
    //ϵͳ��Ϣ�㲥
    int broadCastSysMsg(const std::string& msg, int type);
    //ϵͳ��Ϣ�㲥- level�ȼ�����
    int broadCastSysMsg(const std::string& msg, int type, int level);

    //ϵͳ��Ϣ�㲥-��ͼ�㲥
    int broadCastSysMapMsg(const std::string& msg, int type, int map);

    //�㲥��ȫ���������
    int broadCastToEveryone(const std::string & msg, int repeatnums, int interval);

    //��õ�ǰ��ݡ�����
    boost::shared_ptr<DateInfo> GetDate();

    //��õ�ǰ��ݡ�����
    int getYear();

    //��õ�ǰ��ݡ�����
    int getSeason();

    //��õ�ǰ��ݡ�����
    std::string getSeasonString();

    //��þ���������ʱ��
    int GetRaceRewardTime();

    boost::shared_ptr<color_data> getColorData(int level);

    baseLoginPresent* getBaseLoginPresent(int pid);

    boost::shared_ptr<baseVIPPresent> getBaseVIPPresent(int id);

    baseRechargePresent* getBaseRechargePresent(int id);
    //ϴ�����
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
    //ɾ����ɫʱ���Ƴ���ɫ����
    int removeCharData(int cid);
    void updateSeason();
    void updateRaceRewardTime();
    int resetWelfare();
    int resetWelfare2();
    //�����ճ���һЩ����
    int resetAll();
    //ÿ�L����
    int weekReset();
    //�����̵깺���¼
    void saveShopRecord();
    //ϵͳˢ����������̵�
    int resetShopGoods();
    //ϵͳˢ���������ұ������
    //int refreshSmeltTask();
    //��������
    int getTotalOnline(bool record);
    //�Ƿ���������
    bool isChenmiEnable() {return m_enable_chenmi;}
    //����
    int HeartBeat();

    //����ɫ������Ϣ
    int sendSysMsg(const std::string& char_name, const std::string& msg, int type);
    int sendNotifyMsg(const std::string& char_name, const std::string& msg);

    //���ý�ɫ�ؿ�����
    int updateTempo(int cid, int level);

    int getNearPlayerList(CharData* cdata, json_spirit::Object& robj);

    //��������
    int getRecommendFriends(CharData* cdata, json_spirit::Object& robj);

    //��������
    bool canRecommendFriends(CharData* cdata);

    //���������Ӫ
    int getWeakCamps();
    //int updateCampCnt(int type);
    int RevoltCamps(int& type);

    //���ϵͳ����仯
    void checkAdminNotice(int type);
    int adminNoticeDeleted(int id);
    int adminNoticeNew(int id, const std::string& message);
    int adminNoticeChanged(int id, const std::string& message);
    int getAdminNotice(json_spirit::Array& notice_list);

    void addGeneral(int gid, int cid);

    int getGeneralOwner(int gid);

    void shutdown();

    //��ý�ɫid
    int GetCharId(const std::string& cname);
    //GM
    int GM_reward();
    void GM_recharge(boost::shared_ptr<CharData>& cdata, int recharge_gold);
    //GM�ظ�
    void GM_answer();

    //��������VIP�
    void checkTmpVIP();

    //camp = 1,2,0,
    //void campRaceWinner(int camp);

    int getInt(const std::string& field, int defaultv = 0);
    void setInt(const std::string& field, int value);

    std::string getStr(const std::string& field);
    void setStr(const std::string& field, const std::string& value);

    //���ǿ����Ӫ
    //void checkWeakCamp();

    //��ѯŭ������
    boost::shared_ptr<specialSkill> getSpeSkill(int id);

    int getSpeSkillType(const std::string& type);
    int getSpeSkillTarget(const std::string& target);
    baseBuff* getBaseBuff(int id);

    //ȫ�����VIP4��Ƿ���
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
	//�����̳Ǵ��ۻ
	void openMallDiscountEvent(int discount, time_t start_time, time_t end_time);
    float getMallDiscount();

    //�������
    std::string getRandomName(int gender);

    //�ܷ����ְ�佫
    bool canBuyOfficalGeneral(int gid, int slevel, int offical);
    //��ѯ��ְ�佫
    officalgenerals* getOfficalGeneral(int gid);

private:
    rwlock guild_chanel_rwmutex;
    rwlock camp_chanel_rwmutex;
    rwlock onlineuser_rwmutex;

    rwlock onlinechar_rwmutex;
    rwlock globalchar_rwmutex;

    //Ƶ��
    boost::shared_ptr<ChatChannel> m_world_channel;     //��������
    boost::unordered_map<int, boost::shared_ptr<ChatChannel> > m_camp_channels;         //��Ӫ����
    boost::unordered_map<uint64_t, boost::shared_ptr<ChatChannel> > m_guild_channels;   //��������
    //��ͼƵ��
    boost::shared_ptr<ChatChannel> m_map_channels[max_map_id];   //��������

    boost::unordered_map<std::string, boost::shared_ptr<OnlineUser> > m_onlineuserlist;

    //int camp1_cnt;//�ٸ������
    //int camp2_cnt;//���������

    //��ͼ

    static GeneralDataMgr* m_handle;
    std::map<int, boost::shared_ptr<BaseSoldierData> > m_base_soldiers_map; //����������
    std::map<int, boost::shared_ptr<GeneralTypeData> > m_base_generals_map; //�����佫����
    std::map<int, boost::shared_ptr<baseChengzhangStars> > m_base_chengzhang_stars; //�����ɳ��Ǽ�����
    std::map<int, boost::shared_ptr<baseWashStars> > m_base_wash_stars; //����ϴ���Ǽ�����

    std::map<int, boost::shared_ptr<CharData> > m_chardata_map;             //ȫ����ɫ����
    std::map<std::string, int> m_charid_map;       //ȫ����ɫidӳ��

    std::map<int, boost::shared_ptr<StrongholdGeneralData> > m_stronghold_generals_map;//�ؿ��佫����

    std::map<int, boost::shared_ptr<StrongholdData> > m_stronghold_data_map;//�ؿ�����

    std::map<std::string, boost::shared_ptr<OnlineCharactor> > m_online_charactors; //���߽�ɫ

    std::map<int, boost::shared_ptr<BaseZhenData> > m_base_zhens;       //��������

    std::map<int, boost::shared_ptr<baseEquipment> > m_base_equipments;     //����װ������

    std::map<int, boost::shared_ptr<baseTreasure> > m_base_treasures;         //������������

    std::map<int, boost::shared_ptr<baseGoods> > m_base_mall_goods;         //�̳�����
	struct mall_discount_st m_mall_discount_st;					//�̳Ǵ��ۻ

    boost::shared_ptr<baseMap> m_base_maps[10];    //������ͼ

    std::map<int, boost::shared_ptr<specialSkill> > m_spe_skill_map;        //ŭ������
    std::map<int, boost::shared_ptr<baseBuff> > m_base_buff_map;

    std::map<const std::string, int> m_target_string_map;
    std::map<const std::string, int> m_speskill_type_string_map;

    volatile uint64_t m_combat_id;
    volatile uint32_t m_equipt_id;
    volatile uint32_t m_general_id;
    volatile uint32_t m_gem_id;
    volatile uint32_t m_charactor_id;

    bool m_enable_chenmi;        //����������

    bool m_inited;

    std::map<int, boost::shared_ptr<baseoffical> > m_base_officals;    //������ְ����

    std::list<boost::shared_ptr<officalgenerals> > m_base_offical_generals;    //������ְ�佫����
    std::map<int, boost::shared_ptr<officalgenerals> > m_base_offical_generals_map;//������ְ�佫����
    std::map<int, std::list<boost::shared_ptr<officalgenerals> > > m_offical_generals;   //����ְ���ŵĹ�ְ�佫
    std::map<int, std::list<boost::shared_ptr<officalgenerals> > > m_stronghold_offical_generals;   //���ؿ����ŵĹ�ְ�佫

    std::map<int, boost::shared_ptr<baseofficalskill> > m_base_offical_skills;//������ְ��������

    std::map<int, boost::shared_ptr<color_data> > m_level_color_data;//�佫��ɫ��Ӧ����ֵ
    std::map<int, boost::shared_ptr<baseLoginPresent> > m_base_login_present;//����������¼����
    std::map<int, boost::shared_ptr<baseVIPPresent> > m_base_vip_present;//����VIP����
    std::map<int, boost::shared_ptr<baseRechargePresent> > m_base_recharge_present;//������ֵ������

    std::map<int, int> m_zhen_open_map;    //���Ϳ��Źؿ�

    std::list<Item> m_first_recharge_gift;    //�׳����

    std::map<int, int> m_general_map;    //�䌢�ͽ�ɫ������

    //�����߳��õĵ�ǰ����
    std::list<admin_notice> m_currentAdminNotices;

    //������߳��õ�
    std::list<admin_notice> m_adminNotices;

    int m_year;        //��Ϸ���
    int m_season;    //���� 1 - 4
    int m_race_reward_time;    //��������������ʱ��

    //��ǰ��ݡ�����
    boost::shared_ptr<DateInfo> m_spls_date;

    //��Ӫս����ʤ��
    //int m_camp_race_wins[2];

    //��Ӫ˫����Ծʵ��
    int m_camp_strength[2];
    //��Ӫ��Ծ����
    int m_camp_count[2];

    time_t m_free_vip4_endtime;

    //int m_weak_camp;    //������Ӫ

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
    int m_state;    // 0 ���� 1 ��ɾ��
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
    //������Ϣ
    int Send(const std::string&);
    //��������
    int GuildChat(const std::string& msg);
    //��������
    int AreaChat(const std::string& msg);
    //��������
    int WorldChat(const std::string& msg, bool needgold = true);
    //��Ӫ����
    int CampChat(const std::string& msg);
    //�������
    int TeamChat(const std::string& msg);
    //�������
    int EnterArea(int area);
    //˽��
    int Tell(const std::string& to, const std::string& what, boost::shared_ptr<OnlineCharactor>& toChar);
    //���´����ݿ������Ϣ
    int reload();

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
int CreateChar(const std::string& account, int union_id, const std::string& server_id, const std::string& qid, int camp, int spic, const std::string& name, int g1, int g2, uint64_t& cid, int inv_id);

//ɾ����ɫ
int DeleteChar(uint64_t cid);

//�����ݿ��л�ȡ��ɫ��Ϣ
//int GetCharInfo(int cid, int& camp, int& guild, std::string& name);

//ս���ط�
std::string getCombatRecord(int id);

int ProcessCheckPack(json_spirit::mObject& o);

//����ֵ
int ProcessCheckRecharge(json_spirit::mObject& o);

//�������
void accountOffline(const std::string& account);

#endif

