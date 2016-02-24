#pragma once

#include <string>
#include "data.h"
/*
�����������
*/
enum gold_cost_type
{
    gold_cost_for_rest = 1,                        //�����Ϣ
    gold_cost_for_buy_elite_stronghold = 2,        //����Ӣabandon
    gold_cost_for_world_chat = 3,                //��������abandon
    gold_cost_for_refresh_state = 4,            //ˢ��״̬abandon
    gold_cost_for_refresh_smelt = 5,            //ˢ��ұ��abandon
    gold_cost_for_refresh_explore = 6,            //ˢ��̽��abandon
    gold_cost_for_refresh_research = 7,            //ˢ�¼����о�abandon
    gold_cost_for_buy_research_que = 8,            //���������о�abandon
    gold_cost_for_upgrade_research_que = 9,        //�����������
    gold_cost_for_accelerate_research = 10,        //��ɼ����о�abandon
    gold_cost_for_buy_smelt_que = 11,            //������ʯұ��abandon
    gold_cost_for_accelerate_smelt = 12,        //���ٿ�ʯұ��abandon
    gold_cost_for_buy_train_que = 13,            //�����佫ѵ��λ
    gold_cost_for_accelerate_train = 14,        //�����佫ѵ��
    gold_cost_for_refresh_shop = 15,            //ˢ���̵�
    gold_cost_for_inspire = 16,                    //��ҹ���abandon
    gold_cost_for_stronghold_reget = 17,        //�ٴλ�ȡabandon
    gold_cost_for_accelerate_farm = 18,            //������������abandon
    gold_cost_for_corps_donate = 19,            //���ž���
    gold_cost_for_corps_jisi = 20,                //����abandon
    gold_cost_for_buy_wash_book = 21,            //����ϴ�辭abandon
    gold_cost_for_refresh_guard_goods = 22,        //ˢ�¿ɻ�������
    gold_cost_for_call_guard_goods = 23,        //�ٻ��ƽ�����
    gold_cost_for_clear_guard_cd = 24,            //�����
    gold_cost_for_accelerate_guard = 25,        //���ٻ�������
    gold_cost_for_clear_boss_cd = 26,            //����ս����
    gold_cost_for_buy_baoshi_pos = 27,            //������ʯλabandon
    gold_cost_for_wash = 28,                    //ϴ��abandon
    gold_cost_for_refresh_train = 29,            //ˢ��ѵ����abandon
    gold_cost_for_levy = 30,                    //����
    gold_cost_for_horse_train = 31,                //ս������
    gold_cost_for_trade = 32,                    //ͨ��abandon
    gold_cost_for_sweep = 33,                    //ɨ������
    gold_cost_for_race = 34,                    //����������
    gold_cost_for_corps_create = 35,            //���Ŵ���abandon
    gold_cost_for_corps_invite = 36,            //�������·��
    gold_cost_for_wash_baoshi = 37,                //��ұ�ʯϴ��abandon
    gold_cost_for_lottery = 38,                    //÷������abandon
    gold_cost_for_servant = 39,                    //�Ҷ�ϵͳabandon
    gold_cost_for_daily_task = 40,                //�ճ�����abandon
    gold_cost_for_reborn = 41,                    //һ������
    gold_cost_for_buy_explore = 42,                //����̽������abandon
    gold_cost_for_buy_race = 43,                //���򾺼�����
    gold_cost_for_open_genius = 44,                //�����츳abandon
    gold_cost_for_clean_genius = 45,            //ϴ�츳abandon
    gold_cost_for_graft_genius = 46,            //��ֲ�츳abandon
    gold_cost_for_delay_farm = 47,                //�ӳ�����abandon
    gold_cost_for_reset_elitecombat = 48,        //���þ�Ӣս��
    gold_cost_for_upgrade_equipment = 49,        //����װ�������ȼ�
    gold_cost_for_buy_bag = 50,                    //����ֿ��
    gold_cost_for_speed_farm = 51,                //��������
    gold_cost_for_bank = 52,                    //Ͷ��Ǯׯ
    gold_cost_for_trade_speed = 53,                //ó�׳�ȡ���˼���
    gold_cost_for_trade_abandon = 54,            //ó����������
    gold_cost_for_trade_double = 55,            //ó���޼鲻��
    gold_cost_for_convert_jade = 56,            //��ҹ�����ʯ
    gold_cost_for_buy_inherit = 57,                //���򴫳е�abandon
    gold_cost_for_speed_enhance = 58,            //����ǿ����ȴ
    gold_cost_for_buy_daoju = 59,                //�������
    gold_cost_for_corps_explore_refresh = 60,    //����̽��ˢ��
    gold_cost_for_corps_explore_speed = 61,        //����̽������
    gold_cost_for_corps_recruit = 62,            //������ļ
    gold_cost_for_findback = 63,                //��Ϸ�����һ�
    gold_cost_for_baby = 64,                    //��������
    gold_cost_for_upgrade_farm = 65,            //�����������
    gold_cost_for_con_friend = 66,                //һ��ף��
    gold_cost_for_refresh_train_normal = 67,    //���ˢ��ѵ����
    gold_cost_for_refresh_train_best = 68,        //����ˢ��ѵ����
    gold_cost_for_wash_type2 = 69,                //ϴ����ͭ
    gold_cost_for_wash_type3 = 70,                //ϴ��׽�
    gold_cost_for_wash_type4 = 71,                //ϴ����ʯ
    gold_cost_for_wash_type5 = 72,                //ϴ������
    gold_cost_for_clear_boss_cd_best = 73,        //ս���޷������
    gold_cost_for_corps_jisi_type2 = 74,        //������
    gold_cost_for_corps_jisi_type3 = 75,        //����Ѫ��
    gold_cost_for_servant_exploit = 76,            //׳������
    gold_cost_for_servant_exploit2 = 77,        //׳�����
    gold_cost_for_servant_escape = 78,            //׳������
    gold_cost_for_servant_buy_random = 79,        //׳���������ץ������
    gold_cost_for_bank1 = 80,                    //Ͷ��Ǯׯ1
    gold_cost_for_bank2 = 81,                    //Ͷ��Ǯׯ2
    gold_cost_for_bank3 = 82,                    //Ͷ��Ǯׯ3
    gold_cost_for_bank4 = 83,                    //Ͷ��Ǯׯ4
    gold_cost_for_inspire_boss = 84,            //ս���޹���
    gold_cost_for_inspire_camprace = 85,        //��Ӫս����
    gold_cost_for_inspire_guard = 86,            //�������ù���
    gold_cost_for_maze_full = 87,                //�Թ���Ѫ
    gold_cost_for_maze_reset = 88,                //�Թ�����
    gold_cost_for_maze_skip = 89,                //�Թ�����
    gold_cost_for_maze_change = 90,                //�Թ�����
    gold_cost_for_maze_kill = 91,                //�Թ�����
    gold_cost_for_maze_winmora = 92,            //�Թ���ȭ��ʤ
    gold_cost_for_maze_winguess = 93,            //�Թ������ֱ�ʤ
    gold_cost_for_buy_baoshi = 94,                //����ʯ
    gold_cost_for_servant_buy = 95,                //׳������ץ������
    gold_cost_for_buy_libao = 96,                //�������
    gold_cost_for_throne_con = 97,              //�ΰݻ���
    gold_cost_for_zst_refresh = 98,             //ˢ��ս��̨�Ǽ�
    gold_cost_for_zst_buy = 99,             //����ս��̨��ս����

    gold_cost_for_baoshi = 10000,                //����ʯ+��ʯ����*100+��ʯ�ȼ�
    gold_cost_for_treasure = 20000,                //������+����id(�����ã���Ѷ��id����)
    gold_cost_for_libao = 30000,                //�������+���id
};

/*
��һ������
*/
enum gold_get_type
{
    gold_get_recharge = 1,                        //��ֵ
    gold_get_active = 2,                        //�����
    gold_get_stronghold = 3,                    //�ؿ�����
    gold_get_explore = 4,                        //̽������abandon
    gold_get_race = 5,                            //������
    gold_get_trade = 6,                            //ͨ�̻��abandon
    gold_get_task = 7,                            //������
    gold_get_packs = 8,                            //���(������)
    gold_get_camps = 9,                            //������Ӫabandon
    gold_get_shop = 10,                            //�̵�abandon
    gold_get_lottery = 11,                        //������
    gold_get_daily_task = 12,                    //�ճ�����abandon
    gold_get_online_gift = 13,                    //�������
    gold_get_bank = 14,                            //Ǯׯ
    gold_get_treasure = 15,                        //��ҿ�
    gold_get_gm = 16,                            //gm����
    gold_get_gift = 17,                            //�ղش���
    gold_get_libao = 18,                        //���(vip��������������)
    gold_get_bank1 = 19,                        //Ǯׯ1����
    gold_get_bank2 = 20,                        //Ǯׯ2����
    gold_get_bank3 = 21,                        //Ǯׯ3����
    gold_get_bank4 = 22,                        //Ǯׯ4����
    gold_get_gift_recharge = 10001,            //���ͳ�ֵ
    gold_get_plat_recharge = 10002,            //�ڱҳ�ֵ
};

/*
��������
*/
enum silver_cost_type
{
    silver_cost_for_upgrade_weapon = 1,            //��������abandon
    silver_cost_for_shop_buy = 2,                //�̵깺��
    silver_cost_for_buy_hero = 3,                //��ļ�佫
    silver_cost_for_refresh_state = 4,            //ˢ��״̬abandon
    silver_cost_for_buyback_equiptment = 5,        //�ع�װ��
    silver_cost_for_inherit_equiptment = 6,        //����װ��abandon
    silver_cost_for_inspire = 7,                //���ҹ���abandon
    silver_cost_for_refresh_research = 8,        //ˢ�¼����о���abandon
    silver_cost_for_refresh_train = 9,            //ˢ���佫ѵ����
    silver_cost_for_wash = 10,                    //����ϴ��
    silver_cost_for_refresh_smelt = 11,            //ˢ��ұ��abandon
    silver_cost_for_stronghold = 12,            //�����ؿ�abandon
    silver_cost_for_horse_train = 13,            //ս������
    silver_cost_for_wash_baoshi = 14,            //��ʯϴ��abandon
    silver_cost_for_research = 15,                //����ѵ��abandon
    silver_cost_for_upgrade_star = 16,            //������������abandon
    silver_cost_for_enhance = 17,                //ǿ��װ��
    silver_cost_for_baoshi_change = 18,            //ת����ʯ
    silver_cost_for_baoshi_buy = 19,            //����ʯ
    silver_cost_for_corps_create = 20,            //���Ŵ���
    silver_cost_for_open_box = 21,                //��������
    silver_cost_for_throne_con = 22,            //�ΰݻ���
};

/*
���һ������
*/
enum silver_get_type
{
    silver_get_task = 1,                        //������
    silver_get_stronghold = 2,                    //�ؿ�����
    silver_get_groupCombat = 3,                    //С��ս�۵���
    silver_get_explore = 4,                        //̽������abandon
    silver_get_race = 5,                        //������
    silver_get_guard = 6,                        //��������
    silver_get_boss = 7,                        //ս����
    silver_get_trade = 8,                        //ó��
    silver_get_farm = 9,                        //����abandon
    silver_get_sell_equiptment = 10,            //�佫װ������
    silver_get_corps_jisi = 11,                    //����abandon
    silver_get_boss_last = 12,                    //ս��������һ��
    silver_get_offical = 13,                    //��ְٺ»
    silver_get_campRace = 14,                    //��Ӫս
    silver_get_packs = 15,                        //���(������)
    silver_get_levy = 16,                        //����
    silver_get_by_active = 17,                    //�
    silver_get_by_lottery = 18,                    //������
    silver_get_by_daily_task = 19,                //�ճ�abandon
    silver_get_by_online_gift = 20,                //�������
    silver_get_by_friend = 21,                    //����ף��
    silver_get_by_treasure = 22,                //���ҿ�
    silver_get_by_libao = 23,                    //���(vip��������������)
    silver_get_by_con_friend = 24,                //ף�غ���
    silver_get_sell_treasure = 25,                //���۵���
    silver_get_by_ranking = 26,                    //���н���
    silver_get_by_maze = 27,                    //������
    silver_get_sell_baoshi = 28,                //���۱�ʯ
    silver_get_sell_unknow = 29,                //����δ����
};

/*
������/��������
*/
enum ling_statistics_type
{
    ling_stronghold = 1,                //�ؿ�ս��
    ling_explore = 2,                    //̽��abandon
    ling_hero_train = 3,                //�佫ѵ��
    ling_skill_train = 4,                //����ѵ��abandon
    ling_task = 5,                        //����
    ling_rest_by_gold = 6,                //�����Ϣ
    ling_rest_by_active = 7,            //���Ϣ
    ling_race = 8,                        //������abandon
    ling_trade = 9,                        //ͨ��abandon
    ling_smelt = 10,                    //ұ��abandon
    ling_shop = 11,                        //�̵�abandon
    ling_map = 12,                        //��ͼ����abandon
    ling_farm = 13,                        //����abandon
    ling_boss = 14,                        //ս����
    ling_race_final = 15,                //����������abandon
    ling_corps = 16,                    //������ά��
    ling_packs = 17,                    //���(������)
    ling_lottery = 18,                    //������
    ling_servant = 19,                    //�Ҷ�abandon
    ling_daily_task = 20,                //�ճ�abandon
    ling_online_gift = 21,                //�������
    ling_libao = 22,                    //���(vip��������������)
    ling_elite_combat = 23,                //��Ӣ�ؿ�
};

/*
����(������ѫ��ʯ)
*/
enum treasure_statistics_type
{
    treasure_stronghold = 1,            //�ؿ�
    treasure_smelt = 2,                    //ұ��abandon
    treasure_shop = 3,                    //�̵�
    treasure_task = 4,                    //����
    treasure_explore = 5,                //̽��abandon
    treasure_groupCombat = 6,            //С��ս��
    treasure_active = 7,                //�
    treasure_lottery = 8,                //������
    treasure_servant = 9,                //׳��
    treasure_daily_task = 10,            //�ճ�����
    treasure_online_gift = 11,            //���߽���
    treasure_race = 12,                    //������
    treasure_guard_rob = 13,            //��ȡ����
    treasure_guard = 14,                //��������
    treasure_farm = 15,                    //����
    treasure_farm_water = 16,            //���ｽˮ
    treasure_libao = 17,                //���(vip��������������)
    treasure_corps_explore = 18,        //����̽��
    treasure_corps_ymsj = 19,            //����ԯ�����
    treasure_weapon = 20,                //�ط�
    treasure_inspire = 21,                //����
    treasure_inspire_boss = 22,            //ս���޹���
    treasure_inspire_camprace = 23,        //��Ӫս����
    treasure_baoshi = 24,                //�һ���ʯ
    treasure_buy = 25,                    //��ҹ���
    treasure_maze = 26,            //��������
    treasure_unknow = 9999
};

/*
�������
*/
enum prestige_statistics_type
{
    prestige_groupCombat = 1,            //С��ս��
    prestige_boss = 2,                    //ս����
    prestige_boss_ranking = 3,            //ս��������
    prestige_guard = 4,                    //��������
    prestige_guard_rob = 5,                //��ȡ����
    prestige_jisi = 6,                    //����
    prestige_jisi_type1 = 7,            //����
    prestige_jisi_type2 = 8,            //��
    prestige_jisi_type3 = 9,            //Ѫ��
    prestige_camprace = 10,                //��Ӫս
    prestige_ranking = 11,                //������
    prestige_race = 12,                    //������
};

enum give_loot_type
{
    give_stronghold_loot = 1,
    give_boss_loot = 2,
    give_campRace_loot = 3,
    give_groupCombat_loot = 4,
    give_packs_loot = 5,
    give_race_loot = 6,
    give_sweep_loot = 7,
    give_lottery = 8,
    give_daily_task = 9,
    give_rankings_event = 10,    //���а�
    give_online_gift = 11,        //�������
    give_stage_loot = 12,
    give_guard = 13,    //��������
    give_elite = 14,
    give_libao_loot = 15,
    give_servant_loot = 16,
    give_mail_loot = 17,
    give_maze = 18,
    give_seven_goals = 19,
    give_boss_rank_loot = 20,
    give_boss_last_loot = 21,
    give_explore_loot = 22,
    give_yanhui_loot = 23,
    give_feedback = 24,
    give_invite = 25,
    give_daily_recharge = 26,
    give_box_loot = 27,
    give_throne_loot = 28,
    give_jtz_loot = 29,
    give_jt_boss = 30,
    give_zst = 31,
};

enum baoshi_get_cost
{
    baoshi_admin,
    baoshi_merge,    //�ϲ�
    baoshi_convert,//��ʯת��
    baoshi_buy,    //�̵깺��
    baoshi_sell,    //��������
    baoshi_gift,    //����

    baoshi_give_loot = 1000
};

//��ʯ���Ļ��ͳ��
void add_statistics_of_baoshi_cost(int cid, const std::string& strIP, int union_id, const std::string& server_id, int baoshiType, int baoshiLevel, int baoshiCount, int type);
void add_statistics_of_baoshi_get(int cid, const std::string& strIP, int union_id, const std::string& server_id, int baoshiType, int baoshiLevel, int baoshiCount, int type);

//���ӽ������ͳ��
void add_statistics_of_gold_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

//������������ͳ��
void add_statistics_of_silver_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

//���ӽ����Դͳ��
void add_statistics_of_gold_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

//����������Դͳ��
void add_statistics_of_silver_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

//����������Դͳ��
void add_statistics_of_prestige_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

//����ͳ��stype:1���2����
void add_statistics_of_ling_cost(int cid, const std::string& strIP, int counts, int type, int stype, int union_id, const std::string& server_id);

//����ͳ��stype:1���2����
void add_statistics_of_treasure_cost(int cid, const std::string& strIP, int treasure_id, int counts, int type, int stype, int union_id, const std::string& server_id);

#ifdef QQ_PLAT

//��Ѷͳ��

void cost_todo(CharData* pc, const std::string& gold_price, int type, int nums);
void cost_tobuy(CharData* pc, const std::string& gold_price, int tid, int nums);
void gold_cost_tencent(CharData* pc, int gold_price, int type, int tid = 0, int nums = 1);
void gold_cost_tencent(CharData* pc, double gold_price, int type, int tid = 0, int nums = 1);
void gold_get_tencent(CharData* pc, int gold, int type = 1);
void treasure_cost_tencent(CharData* pc, int tid, int nums);
void att_change_tencent(CharData* pc, const std::string& type, const std::string& before, const std::string& after);
void bag_to_tencent(CharData* pc, const std::string& info);
void ser_to_tencent(const std::string& type, const std::string& stype, int nums);
void guide_to_tencent(CharData* pc, int guide_id);
void login_to_tencent(CharData* pc, const std::string& str1, const std::string& str2, const std::string& friend_id, const std::string& feed_id);

#endif
#if 0
enum act_type
{
    act_stronghold = 1,                    //�ؿ��״λ�ɱ        ����1 �ؿ�id
    act_get_daily = 2,                    //��Ծ�������ȡ        ����1 ���id
    act_get_online = 3,                    //���������ȡ        ����1 ���id,�ڼ���
    act_shop_buy = 4,                    //�̵깺��
    act_farm_plant = 5,                    //������ֲ
    act_farm_water = 6,                    //�����Լ�
    act_farm_reap = 7,                    //�ջ�
    act_farm_water_friend = 8,            //�������
    act_farm_water_all = 9,                //һ���������
    act_farm_click_upgrade = 10,        //���ũ������[�ͻ���]
    act_farm_upgrade_recharge = 11,        //ũ��������Ҳ�����ת��ֵ[�ͻ���]
    act_trade_click_select = 12,        //�����ȡ����
    act_trade_click_abandon = 13,        //�����������[�ͻ���]
    act_trade_start = 14,                //��ʼó��                ���� 1�Ǽ�
    act_trade_enter = 15,                //����ó��
    act_trade_get_goods = 16,            //ó����Ʒ��������ȡ[�ͻ���]
    act_trade_get_goods_sweep = 17,        //��ȡ��Ʒ���ɨ��[�ͻ���]
    act_trade_click_wjbs = 18,            //������̲���        ����1�ڼ���
    act_trade_get_silver = 19,            //���þ���
    act_wash_use_silver = 20,            //����ϴ��
    act_enhance_equiptment = 21,        //ǿ��װ��                   ����1װ������( 1 ����  2 ���� 3������ 4������ 5������)
    act_make_equiptment = 22,            //����װ��                   ����1װ������( 1 ����  2 ���� 3������ 4������ 5������)
    act_train_general = 23,                //�佫ѵ��
    act_train_refresh = 24,                //����ˢ���佫ѵ��
    act_reborn_click = 25,                //�佫����
    act_exchange_baoshi = 26,            //��ʯ�һ���ʯ         ����1 ��ʯ����
    act_baoshi_xiangqian = 27,            //��ʯ��Ƕ                   ����1 ��ʯ�ȼ�
    act_recruit_general = 28,            //��ļ�佫                ����1 �佫����id
    act_general_click_inherit = 29,        //�������[�ͻ���]
    act_general_free_inherit = 30,        //��Ѵ���
    act_general_treasure_inherit = 31,    //���е�����
    act_general_click_buy_inherit = 32,    //������е�����[�ͻ���]
    act_mifa_levelup = 33,                //�ط�����                ����1 ( 1 ��ͨ���� 2 ��ͨ���� 3 ���Թ��� 4 ���Է��� 5 ����)
    act_formation_new = 34,                //�л�Ĭ������
    act_formation_change = 35,            //���͵���λ��
    act_train_horse = 36,                //��ͨս������
    act_ssoul_click_levelup = 37,        //��������                   ����1 1-3,����ˣ�����2 1-5
    act_ssoul_click_buy = 38,            //�����������Ƭ��[�ͻ���]
    act_corps_click_apply = 39,            //������ż�������
    act_corps_click_create = 40,        //������Ŵ���[�ͻ���]
    act_corps_create_confirm = 41,        //���Ŵ���
    act_corps_explore_speed = 42,        //����̽���������
    act_corps_explore_refresh = 43,        //����̽������ˢ��
    act_open_ling = 44,                    //�򿪾�������
    act_get_free_ling = 45,                //����ǲ����ȡ����
    act_buy_ling = 46,                    //�������
    act_bank_back = 47,                    //Ǯׯ����                ����1 �������(1-4)  ����2 ����2�����(1-4)
    act_arena_challenge = 48,            //������ս
    act_arena_exchange = 49,            //�������ֶһ�
    act_group_copy = 50,                //С��ս��                 ����1 ս�۱��
    act_boss = 51,                        //�μ�����ս               ����1 boss���
    act_camp_race = 52,                    //�μ���Ӫս
    act_supply_click = 53,                //������Ӿ���[�ͻ���]
    act_rob = 54,                        //�Ӷ�ؿ�
    act_sweep = 55,                        //��������ɨ��[�ͻ���]
    act_sweep_click_speed = 56,            //�������ɨ������
    act_congratulation = 57,            //ף�غ��Ѵ���
    act_servant_click_random = 58,        //������ץ��
    act_servant_arrest = 59,            //ץ�����°ܽ�
    act_servant_rescue = 60,            //��ȼҶ�
    act_servant_click_yushi = 61,        //�Ҷ��������һ���ʯ[�ͻ���]
    act_elite_click = 62,                //��Ӣ�ؿ��������      ���� 1��Ӣ�ؿ�id[�ͻ���]
    act_elite_attack = 63,                //��Ӣ�ؿ�����        ���� 1��Ӣ�ؿ�id
    act_view_attack_click = 64,            //�鿴��ս��[�ͻ���]
    act_view_attack_click_by_pop = 65,    //�����鿴��ս��[�ͻ���]
    act_maze_finish = 66,                //������ͨ��            ����1ʲô�Ų���2ʲô�Ѷ�
    act_maze_click_change = 67,            //�������������[�ͻ���]
    act_maze_buy = 68,                    //�����������Ʒȷ�Ϲ���
    act_guard = 69,                        //��������
    act_corps_jisi = 70,                //���ż���
    act_corps_lottery = 71,                //����������
    act_get_sign = 72,                    //ǩ�����                ����1ǩ������
    act_get_level = 73,                    //�ȼ����                ����1�ȼ�
    act_get_vip = 74,                    //vip���                    ����1vip�ȼ�
    act_yellow_general_get = 75,        //�����佫��ȡ
    act_yellow_new_libao = 76,            //�����������
    act_yellow_daily_libao = 77,        //����ÿ�����
    act_yellow_daily_year = 78,            //�������ÿ�����
    act_yellow_level_libao = 79,        //����ɳ����        ����1�ȼ�
    act_yellow_click = 80,                //�����ͨ����[�ͻ���]
    act_bag_use = 81,                    //�ֿ�ʹ��
    act_bag_free = 82,                    //�ֿ�ʣ��
    act_bag_click_buy = 83,                //����ֿ⹺��[�ͻ���]
    act_maze_click = 84,                //��������[�ͻ���]    ����1ʲô�Ų���2ʲô�Ѷ�
    act_maze_enter = 85,                //���������            ����1ʲô�Ų���2ʲô�Ѷ�

    act_click_mall = 86,                //����̳�
    act_click_mall_buy = 87,            //����̳ǹ���
    act_click_recharge_in_mall = 88,    //����̳��ڵĳ�ֵ
    act_click_recharge_in_daily_recharge = 89,    //���ÿ�ճ�ֵ�ڵĳ�ֵ��ť
    act_get_daily_recharge_award = 90,    //��ȡÿ�ճ�ֵ����
    act_click_farm_yecha = 91,            //�������Ұ���ջ�
    act_click_horse_gold_train = 92,        //����������

    act_click_invite = 93,    //���������Ѱ�ť
    act_click_invite_right_now = 94,    //�����������
    act_click_recall_right_now = 95,    //��������ٻ�
    act_click_invite_share = 96,        //������� ����1 ����id
    act_invite_lottery = 97,            //����ҡ��
    act_click_mall_baoshi = 98,        //�̳ǽ�������ʯ��ǩ
    act_click_mall_gem = 99,            //�̳ǽ��������߱�ǩ
    act_click_daily_recharge = 100,    //����ճ尴ť
};
#endif
enum act_new_type
{
    act_new_stronghold = 1, //�����ؿ�����[�佫��Ϣ]
    act_new_elite = 2,  //������Ӣ�ؿ�
    act_new_helplist_click = 3, //������ְ�ť[flex]
    act_new_helplist_reward = 4,    //��ȡ���ֽ���[��Ծ��]
    act_new_sign = 5,   //ǩ��
    act_new_sign_reward = 6,    //ǩ����������ȡ[���id]
    act_new_7_goal_click = 7,   //�������Ŀ��[flex]
    act_new_7_goal_reward = 8,  //��ȡ����Ŀ�꽱��[����id]
    act_new_collect_reward = 9, //��ȡ�ղؽ���
    act_new_online_reward = 10, //��ȡ�������߽���[����id]
    act_new_shop = 11,  //�̵깺��
    act_new_farm = 12,  //��ֲ
    act_new_water = 13, //����
    act_new_farm_get = 14,    //�ջ�
    act_new_water_friend = 15,  //�������
    act_new_water_friend_all = 16,  //һ���������
    act_new_nourish = 17,   //ũ��Ұ��
    act_new_farm_up_click = 18,   //ũ���������[flex]
    act_new_farm_up_recharge_click = 19,   //ũ�����������ֵ���[flex]
    act_new_trade_select = 20,  //��ȡ����
    act_new_trade_wjbs_recharge_click = 21,  //�޼鲻�̽����ֵ���[flex]
    act_new_trade_wjbs = 22,  //ʹ���޼鲻��
    act_new_trade_abandon = 23, //��������
    act_new_finish = 24,    //���ó��[����Ʒ��]
    act_new_wash = 25,  //ϴ�����
    act_new_wash_confirm = 26,  //ϴ��ȷ������
    act_new_wash_continue_click = 27, //����ϴ����[flex]
    act_new_wash_up = 28, //ϴ���Ǽ�����[�Ǽ�]
    act_new_equipt_up = 29, //����װ��[װ������1 ����  2 ���� 3������ 4������ 5������]
    act_new_equipt_make = 30,   //����װ��[װ������1 ����  2 ���� 3������ 4������ 5������]
    act_new_equipt_make_special = 31,   //�����ȼ�����[װ���ȼ���Ʒ��]
    act_new_train = 32, //ѵ���佫
    act_new_refresh_book_silver = 33,   //����ˢ�±���
    act_new_baoshi_by_yushi = 34,  //��ʯ����ʯ[����]
    act_new_baoshi_xiangqian = 35,    //��Ƕ��ʯ[��ʯ���ͣ���ʯ�ȼ�]
    act_new_baoshi_change = 36, //ת����ʯ[��ʯ�ȼ�]
    act_new_yushi_get = 37, //��ȡ��ʯ
    act_new_baoshi_by_gold = 38,    //������ʯ[��ʯ����]
    act_new_baoshi_conbine = 39,    //�ϳɱ�ʯ[��ʯ�ȼ�]
    act_new_reborn_up = 40, //�����Ǽ�����[�Ǽ�]
    act_new_reborn = 41,    //����[�佫id]
    act_new_buy_general = 42,   //��ļ�佫[�佫id]
    act_new_prestige_get_click = 43,    //��ļ�佫����������ȡ���Ե��[flex]
    act_new_inherit_click = 44, //�佫���洫�е��[flex]
    act_new_inherit = 45,   //�佫����[���У������У��Ƿ񴫳е�]
    act_new_inherit_buy_click = 46,   //���򴫳е����[flex]
    act_new_mifa = 47,    //�ط�����[����1 ��ͨ���� 2 ��ͨ���� 3 ���Թ��� 4 ���Է��� 5 ����]
    act_new_zhen_change = 48,   //����Ĭ������
    act_new_zhen_general_change = 49,   //���������佫
    act_new_horse_train_silver = 50,    //��������ս��
    act_new_soul_up = 51,   //��������[���ͣ�С�࣬�ȼ�]
    act_new_soul_buy_click = 52,    //���������[flex]
    act_new_corps_apply_click = 53,   //������ŵ��[flex]
    act_new_corps_create_click = 54,   //�������ŵ��[flex]
    act_new_corps_create_confirm_click = 55,   //��������ȷ�����[flex]
    act_new_corps_ymsj = 56,    //ԯ�����
    act_new_corps_explore = 57, //����̽��
    act_new_corps_lottery = 58, //������
    act_new_corps_jisi_click = 59,    //���ż������[flex]
    act_new_corps_ymsj_click = 60,   //ԯ����ꪻ���[flex]
    act_new_corps_yanhui_click = 61,   //������[flex]
    act_new_corps_explore_click = 62,   //����̽������[flex]
    act_new_corps_lottery_click = 63,   //�����̻���[flex]
    act_new_ling_click = 64,   //���Ӿ��ť���[flex]
    act_new_ling_free = 65,   //����ǲ��
    act_new_ling_buy = 66,   //�����[����]
    act_new_supply_click = 67,  //���Ӿ�����ť���[flex]
    act_new_supply_window_click = 68,   //�������Ӿ������[flex]
    act_new_char_click = 69,   //��ɫͷ����[flex]
    act_new_vip_click = 70,   // vip�ȼ����[flex]
    act_new_vip_recharge_click = 71,   // vip�����ֵ���[flex]
    act_new_attack_click = 72,   //ս���鿴���[flex]
    act_new_race = 73,   //��������ս
    act_new_race_buy = 74,   //���������ֶһ�
    act_new_guard = 75, //��������
    act_new_guard_rob = 76,    //��ȡ����
    act_new_guard_rob_rank_click = 77,    //�����������[flex]
    act_new_guard_help_click = 78,    //������ѻ���[flex]
    act_new_servant_catch_random = 79,  //���ץ��[flex]
    act_new_servant_catch_race = 80,    //������ץ��[flex]
    act_new_servant_catch_enemy = 81,   //���ץ��[flex]
    act_new_servant_rescue_corps = 82,  //���Ž��[flex]
    act_new_servant_rescue_friend = 83, //���ѽ��[flex]
    act_new_servant_interact = 84,  //����[flex]
    act_new_servant_yushi_click = 85,   //��ʯ�һ����[flex]
    act_new_camp_race_battle = 86,  //��Ӫս��ս
    act_new_camp_race = 87, //��Ӫս�μ�
    act_new_congratulation = 88,    //ף�غ���
    act_new_congratulation_recv = 89,   //����ף��
    act_new_relation_click = 90,    //����⽻��ť[flex]
    act_new_group_copy = 91,    //�μ�С��ս��
    act_new_group_copy_zhaoji_click = 92,    //���С��ս���ټ�����[flex]
    act_new_group_copy_random_click = 93,    //���С��ս���������[flex]
    act_new_boss = 94,  //��սboss[bossid]
    act_new_maze = 95,  //���������[���ͣ��Ѷ�]
    act_new_maze_abandon = 96,  //����������[���ͣ��Ѷ�]
    act_new_maze_finish = 97,  //��ɰ�����[���ͣ��Ѷ�]
    act_new_bag_buy = 98,   //�ֿ⹺��[���������������]
    act_new_bag_click = 99,   //�ֿⰴť���[flex]
    act_new_mall_click = 100,   //�̳ǰ�ť���[flex]
    act_new_mall_baoshi_click = 101,   //�̳Ǳ�ʯ��ť���[flex]
    act_new_mall_daoju_click = 102,   //�̳ǵ��߰�ť���[flex]
    act_new_mall_recharge_click = 103,   //�̳ǳ�ֵ��ť���[flex]
    act_new_rank_click = 104,    //������а�ť[flex]
    act_new_rank_type_click = 105,    //��������ڱ�ǩ[��������][flex]
    act_new_jxl_click = 106,    //�������¼[flex]
    act_new_jxl_type_click = 107,    //�������¼������[����][flex]
    act_new_jxl = 108,    //���ý���¼[����]
    act_new_vip_libao_click = 109,    //���vip���[flex]
    act_new_vip_daily_get_click = 110,    //���vipÿ�����������ȡ[flex]
    act_new_vip_daily_recharge_click = 111,    //���vipÿ����������ֵ[flex]
    act_new_vip_libao_type_click = 112,    //���vip�����ǩ[flex]
    act_new_vip_purple_click = 113,    //���vipר���Ͻ���ǩ[flex]
    act_new_vip_purple_recharge_click = 114,    //���vipר���Ͻ���ֵ[flex]
    act_new_vip_orange_click = 115,    //���vipר���Ƚ���ǩ[flex]
    act_new_vip_orange_recharge_click = 116,    //���vipר���Ƚ���ֵ[flex]
    act_new_rank_event_click = 117, //�����а�ť���[flex]
    act_new_rank_event_last_click = 118, //�������а�ť���[flex]
    act_new_bank_click = 119,   //Ǯׯ��ť���[flex]
    act_new_bank = 120, //ǮׯͶ��[����]
    act_new_daily_click = 121,  //����ճ�ֵ���[flex]
    act_new_daily_recharge_click = 122,  //����ճ�ֵ�����ֵ[flex]
    act_new_action_click = 123,  //������ʻ[flex]
    act_new_daily_action_click = 124,  //����ճ��[flex]
    act_new_corps_action_click = 125,  //������Ż[flex]
    act_new_more_attack_window = 126,    //ʵ������Ʈ��[flex]
    act_new_more_attack_click = 127,    //�������ʵ��[flex]
    act_new_more_attack_more_click = 128,    //�������ʵ��"�˽����"[flex]
    act_new_more_attack_more_type_click = 129,    //����ʵ��ϸ�ְ�ť[����][flex]
    act_new_levy_window = 130,    //����Ʈ��[flex]
    act_new_levy_window_click = 131,    //����Ʈ�����[flex]
    act_new_supply_window = 132,    //����Ʈ��[flex]
    act_new_supply_window_click_ = 133,    //����Ʈ�����[flex]
    act_new_supply_window_type_click = 134,    //��������ϸ�ְ�ť[����][flex]
};

void act_to_tencent(CharData* pc, int type, int param1 = 0, int param2 = 0, int param3 = 0, std::string param4 = "");

//��Ѷ����ͳ��
int ProcessActTencent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);



