#pragma once

#include <string>
#include "json_spirit.h"

/*
����
1    ����
2    ����
3    װ��
4    �佫
5    ����
6    ����
7    ���
8    ��ͼ�ȼ�����
9    ����
10    ����
11    N�������ȼ�������
12    �佫����
13    N�������ȼ��ĵ���
14    ����
15    ��ʯ
16    ǿ��ʯ
17    ���
18    ����������
19    N�������ȼ�������
*/
enum item_type_enum
{
    item_type_silver = 1,
    item_type_treasure = 2,
    item_type_equipment = 3,
    item_type_general = 4,
    item_type_zhen = 5,
    item_type_skill = 6,
    item_type_gold = 7,
    item_type_silver_map = 8,
    item_type_ling = 9,
    item_type_prestige = 10,
    item_type_silver_level = 11,     // n*�����ȼ�������
    item_type_general_baowu = 12,//Ӣ�۱���-�����
    item_type_treasure_level = 13,//n*�����ȼ��ĵ���
    item_type_exp = 14,            //��������
    item_type_baoshi = 15,        //���䱦ʯ
    item_type_stone = 16,            //�ȼ���Ӧ��ǿ��ʯ����(���ƾ�����������)
    item_type_libao = 17,//���
    item_type_arena_score = 18,    //����������
    item_type_prestige_level = 19,//n*�ȼ�������
};

enum enum_treasure_type
{
    treasure_type_yushi = 9,
    treasure_type_mati_tie = 10,
    treasure_type_yinyang_fish = 11,
    treasure_type_supply = 12,
    treasure_type_silver_card = 511,
    treasure_type_gold_card = 512,
    treasure_type_jade_card = 513,
    treasure_type_prestige_card = 514,
    treasure_type_food_card = 515,
    treasure_type_honor_card = 516,
    treasure_type_box1 = 517,
    treasure_type_box2 = 518,
    treasure_type_box3 = 519,
    treasure_type_gongxun = 2001,
    treasure_type_chuanchengdan = 2002,
    treasure_type_corps_lottery = 4001,
    treasure_type_soul_type1 = 4002,//������Ƭ
    treasure_type_soul_type2 = 4003,//����
    treasure_type_soul_type3 = 4004,//����
    treasure_type_friend_lottery = 4005,

    treasure_type_bag_key = 4006,
    treasure_type_levy_ling = 4007,
    treasure_type_explore_ling = 4008,
    treasure_type_soul_ling = 4009,
    treasure_type_wjbs = 4010,
    treasure_type_book_refresh = 4011,

    treasure_type_general_soul = 4012,

    treasure_type_buff_bingli = 5001,
    treasure_type_buff_wugong = 5002,
    treasure_type_buff_wufang = 5003,
    treasure_type_buff_cegong = 5004,
    treasure_type_buff_cefang = 5005,
};

struct Item
{
    int type;   //������Ʒ���࣬1���ң�2���� 3װ�� 4Ӣ�� 5���� 6���� 7��� 8��ͼ�������������ң�9����
    int id;     //type�����е�id
    int nums;   //����
    int fac;    //����
    int spic;    //ͼƬid

    int guid;    //��Ʒ��Ψһid

    std::string toString(bool withColor = false, int char_level = 0) const;
    Item()
    {
        type = 0;
        id = 0;
        nums = 0;
        fac = 0;
        guid = 0;
        spic = 0;
    }
    Item(int type_, int id_, int nums_, int fac_)
    :type(type_)
    ,id(id_)
    ,nums(nums_)
    ,fac(fac_)
    {
        guid = 0;
        spic = id;
    }
    void toObj(json_spirit::Object& obj);

    std::string name() const;
};

