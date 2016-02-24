#pragma once

#include <string>
#include "data.h"

/*
��������
*/
enum statistics_loot_type
{
    loot_init = 0,
    loot_sign_action = 1,               //ǩ������
    loot_limit_action = 2,              //��ʱ�
    loot_arena = 3,                     //������ս��
    loot_arena_shop = 4,                //�������̵�
    loot_copy = 5,                      //����ս��
    loot_copy_shop = 6,                 //�����̵�
    loot_daily_score = 7,               //ÿ�ձ���
    loot_prestige_shop = 8,             //�����̵�
    loot_explore = 9,                   //̽����Ѩ
    loot_guild_box = 10,                //���ᱦ��
    loot_libao = 11,                    //���
    loot_treasure = 12,                 //�ر�ͼ
    loot_stronghold = 13,               //�ؿ�ս��
    loot_stage = 14,                    //ͨ�ؽ���
    loot_recharge_event = 15,           //��ֵ�
    loot_shenling = 16,                 //������ս��
    loot_shenling_shop = 17,            //�������̵�
    loot_task = 18,                     //����
    loot_arena_rank = 19,               //����������
    loot_pk_rank = 20,                  //pk����
    loot_qq_yellow = 21,                //����
    loot_find_back = 22,                //�һؽ���
    loot_present = 23,                  //���������
    loot_copy_finish = 24,              //����ͨ�ؽ���
    loot_online_action = 25,            //���߽���
    loot_prestige_task = 26,            //��������
    loot_wild = 27,                     //����ս��
    loot_guild_moshen = 28,             //����ħ��
    loot_goal = 29,                     //Ŀ��ϵͳ
    loot_goal_shop = 30,                //Ŀ���̵�
    loot_daily_task = 31,               //�ճ�����
    loot_boss = 32,                     //boss
    loot_lottery = 33,                  //ת��
    loot_auction = 34,                  //������
    loot_weekRanking = 35,              //������
};

/*
���������
*/
enum statistics_gold_get_type
{
    gold_get_init = 0,
    gold_get_limit_action = 1,          //��ʱ�
    gold_get_arena_rank = 2,            //����������
    gold_get_daily_score = 3,           //ÿ�ձ���
    gold_get_qq_yellow = 4,             //����
    gold_get_pk_rank = 5,               //��������
    gold_get_sign_action = 6,           //ǩ������
    gold_get_bank = 7,                  //Ͷ��
    gold_get_prestige_shop = 8,         //�����̵�
    gold_get_lottery = 9,               //ת��
    gold_get_auction = 10,              //������
};

/*
����������
*/
enum statistics_silver_get_type
{
    silver_get_init = 0,
    silver_get_limit_action = 1,        //��ʱ�
    silver_get_sign_action = 2,         //ǩ������
    silver_get_treasure = 3,            //�ر�ͼ
    silver_get_recharge_event = 4,      //��ֵ�
    silver_get_libao = 5,               //���
    silver_get_online_action = 6,       //���߽���
    silver_get_levy = 7,                //����
    silver_get_wild_levy = 8,           //��������
    silver_get_shenling = 9,            //������ս��pve
    silver_get_explore = 10,            //̽����Ѩ
    silver_get_arena_rank = 11,         //����������
    silver_get_copy = 12,               //����ս��pve
    silver_get_stronghold = 13,         //�ؿ�ս��pve
    silver_get_task = 14,               //����
    silver_get_prestige_shop = 15,      //�����̵�
    silver_get_pk = 16,                 //���볡ս��pvp
    silver_get_qq_yellow = 17,          //����
    silver_get_daily_score = 18,        //ÿ�ձ���
    silver_get_guild_box = 19,          //���ᱦ��
    silver_get_stage = 20,              //ͨ�ؽ���
    silver_get_help = 21,               //���ֱ���
    silver_get_sell = 22,               //����
    silver_get_gem = 23,                //���߻��
    silver_get_wild = 24,               //����ս��
    silver_get_find_back = 25,          //�һؽ���
    silver_get_copy_finish = 26,        //����ͨ��
    silver_get_guild_moshen = 27,       //����ħ��ս��pve
    silver_get_daily_task = 28,         //�ճ�����
    silver_get_boss = 29,               //boss
    silver_get_goal = 30,               //Ŀ��ϵͳ
    silver_get_double = 31,             //�������
    silver_get_by_friend = 32,          //����ף��
    silver_get_auction = 33,            //������
    silver_get_weekRanking = 34,        //������
};

/*
���߻������
*/
enum statistics_gem_get_type
{
    gem_get_hero_decompose = 1,         //Ӣ�۷ֽ�
    gem_get_buy = 2,                    //�������
    gem_get_buy_mall = 3,               //�����̳ǵ���
    gem_get_shenling_add = 4,           //���������Ӵ���
    gem_get_sign_action = 5,            //ǩ������
    gem_get_limit_action = 6,           //��ʱ�
    gem_get_arena = 7,                  //������ս��
    gem_get_arena_shop = 8,             //�������̵�
    gem_get_copy = 9,                   //����ս��
    gem_get_copy_shop = 10,             //�����̵�
    gem_get_daily_score = 11,           //ÿ�ձ���
    gem_get_prestige_shop = 12,         //�����̵�
    gem_get_guild_box = 13,             //���ᱦ��
    gem_get_libao = 14,                 //���
    gem_get_treasure = 15,              //�ر�ͼ
    gem_get_stronghold = 16,            //�ؿ�ս��
    gem_get_recharge_event = 17,        //��ֵ�
    gem_get_shenling = 18,              //������ս��
    gem_get_shenling_shop = 19,         //�������̵�
    gem_get_task = 20,                  //����
    gem_get_pk_rank = 21,               //pk����
    gem_get_qq_yellow = 22,             //����
    gem_get_online_action = 23,         //���߽���
    gem_get_copy_finish = 24,           //����ͨ��
    gem_get_daily_task = 25,            //�ճ�����
    gem_get_guild_moshen = 26,          //����ħ��ս��pve
    gem_get_goal = 27,                  //Ŀ��ϵͳ
    gem_get_goal_shop = 28,             //Ŀ���̵�
    gem_get_boss = 29,                  //boss
    gem_get_lottery = 30,               //ת��
    gem_get_present = 31,               //���������
    gem_get_auction = 32,               //������
    gem_get_weekRanking = 33,           //������
};

/*
�����������
*/
enum statistics_char_exp_get_type
{
    char_exp_get_task = 1,              //����
    char_exp_get_stage = 2,             //ͨ�ؽ���
    char_exp_get_find_back = 3,         //�һؽ���
    char_exp_get_daily_task = 4,        //�ճ�����
};

/*
�����������
*/
enum statistics_prestige_get_type
{
    prestige_get_stronghold = 1,        //�ؿ�ս��
    prestige_get_prestige_task = 2,     //��������
    prestige_get_gem = 3,               //���߻��
    prestige_get_boss = 4,              //bossս���
    prestige_get_weekRanking = 5,       //������
};

/*
Ӣ�۾���������
*/
enum statistics_hero_exp_get_type
{
    hero_exp_get_stronghold = 1,        //�ؿ�ս��
    hero_exp_get_gem = 2,               //���߻��
    hero_exp_get_explore = 3,           //̽����Ѩ
};

enum statistics_hero_get_type
{
    hero_get_stronghold = 1,        //�ؿ�ս��
    hero_get_auction = 2,
};

/*
������������
*/
enum statistics_silver_cost_type
{
    silver_cost_init = 0,
    silver_cost_equipt_compound = 1,    //װ���ϳ�
    silver_cost_equipt_upgrade = 2,     //װ������
    silver_cost_guild_donate = 3,       //�������
    silver_cost_hero_compound = 4,      //Ӣ�ۺϳ�
    silver_cost_hero_decompose = 5,     //Ӣ�۷ֽ�
    silver_cost_hero_pack = 6,          //Ӣ�ۿ���
    silver_cost_buy_mall_good = 7,      //�����̳���Ʒ
    silver_cost_treasure_refresh = 8,   //�ر�ͼˢ��
    silver_cost_castle_levelup = 9,     //�Ǳ�����
    silver_cost_metallurgy_levelup = 10,//��������
    silver_cost_smithy_levelup = 11,    //����������
    silver_cost_barracks_levelup = 12,  //��Ӫ����
    silver_cost_wild_attack = 13,       //����ս��
    silver_cost_buy_prestige = 14,      //���������̵�
    silver_cost_guild_create = 15,      //��������
    silver_cost_hero_golden = 16,       //Ӣ�۵��
    silver_cost_skill_upgrade = 17,     //��������
    silver_cost_quit_combat = 18,       //���ܳͷ�
    silver_cost_stronghold = 19,        //�ؿ�ս��pve
    silver_cost_copy = 20,              //����ս��pve
    silver_cost_shenling = 21,          //������ս��pve
    silver_cost_guild_moshen = 22,      //����ħ��ս��pve
    silver_cost_boss_inspire = 23,      //boss����
    silver_cost_boss = 24,              //bossս��pve
    silver_cost_bank = 25,              //����������
    silver_cost_auction = 26,           //����
    silver_cost_pk = 27,                //pk���
};

/*
�����������
*/
enum statistics_gold_cost_type
{
    gold_cost_init = 0,
    gold_cost_sign = 1,                 //��ǩ
    gold_cost_arena_add = 2,            //�������������
    gold_cost_arena_cd = 3,             //��������ȴ
    gold_cost_levy = 4,                 //ǿ������
    gold_cost_copy_reset = 5,           //��������
    gold_cost_copy_add = 6,             //�����������
    gold_cost_daily_score_refresh = 7,  //ÿ�ձ���ˢ��
    gold_cost_daily_score_done = 8,     //ÿ�ձ����������
    gold_cost_equipt_compound = 9,      //װ���ϳ�
    gold_cost_equipt_upgrade = 10,      //װ������
    gold_cost_guild_donate = 11,        //�������
    gold_cost_buy_hero_size = 12,       //����Ӣ��λ
    gold_cost_hero_compound = 13,       //Ӣ�ۺϳ�
    gold_cost_hero_pack = 14,           //Ӣ�ۿ���
    gold_cost_buy_gem = 15,             //�������
    gold_cost_buy_bag = 16,             //���򱳰�λ
    gold_cost_horn = 17,                //��������
    gold_cost_buy_mall_good = 18,       //�����̳���Ʒ
    gold_cost_shenling_refresh_skill = 19,//������ˢ�¼���
    gold_cost_shenling_reset = 20,      //����������
    gold_cost_shenling_add = 21,        //���������Ӵ���
    gold_cost_sweep_cd = 22,            //ɨ����ȴ
    gold_cost_task_done = 23,           //����������
    gold_cost_treasure_finish = 24,     //�ر�ͼ�������
    gold_cost_treasure_refresh = 25,    //�ر�ͼˢ��
    gold_cost_treasure_call = 26,       //�ر�ͼһ���ٻ�
    gold_cost_hero_smelt_refresh = 27,  //Ӣ������ˢ��
    gold_cost_attack_stronghold = 28,   //��ȯͨ��
    gold_cost_find_back = 29,           //�һ�
    gold_cost_buy_goal = 30,            //����Ŀ����Ʒ
    gold_cost_boss_cd = 31,             //��boss��ȴ
    gold_cost_boss_inspire = 32,        //����boss
    gold_cost_for_bank = 33,            //Ͷ��
    gold_cost_double = 34,              //����
    gold_cost_for_con_friend = 35,      //ף�غ���
    gold_cost_auction = 36,             //����
};

/*
������������
*/
enum statistics_gem_cost_type
{
    gem_cost_use = 1,                   //ʹ��
    gem_cost_copy_add = 2,              //�����������
    gem_cost_equipt_upgrade = 3,        //װ������
    gem_cost_hero_smelt = 4,            //Ӣ������
    gem_cost_hero_golden = 5,           //Ӣ�۵��
    gem_cost_skill_upgrade = 6,         //��������
    gem_cost_hero_epic = 7,             //��
    gem_cost_shenling_shop = 8,         //�������̵�
    gem_cost_shenling = 9,              //������ս��
    gem_cost_horn = 10,                 //��������
    gem_cost_auction = 11,              //����
    gem_cost_equipt_add_slot = 12,      //װ������
};

void statistics_of_gold_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);
void statistics_of_gold_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

void statistics_of_silver_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);
void statistics_of_silver_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

void statistics_of_prestige_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);
void statistics_of_char_exp_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);
void statistics_of_hero_exp_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id);

void statistics_of_gem_get(int cid, const std::string& strIP, int gem_id, int counts, int type, int union_id, const std::string& server_id);
void statistics_of_gem_cost(int cid, const std::string& strIP, int gem_id, int counts, int type, int union_id, const std::string& server_id);

void statistics_of_hero_get(int cid, const std::string& strIP, int hero_id, int counts, int type, int union_id, const std::string& server_id);

