#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include <map>

enum combat_attr_enum
{
    combat_attr_none = 0,

    combat_attr_pugong,    //��ͨ���� 1
    combat_attr_pufang,    //��ͨ���� 2
    combat_attr_cegong,    //���Թ��� 3
    combat_attr_cefang,    //���Է��� 4
    combat_attr_bingli,    //���� 5

    combat_attr_baoji,    //���� 6
    combat_attr_dodge,    //���� 7
    combat_attr_parry,    //�� 8 
    combat_attr_shipo,    //ʶ�� 9
    combat_attr_xixue,    //��Ѫ 10
    combat_attr_chaos,    //���� 11
    combat_attr_podan,    //�Ƶ� 12
    combat_attr_weihe,    //���� 13

    combat_attr_resist_baoji,//������ 14
    combat_attr_resist_dodge,//������ 15
    combat_attr_resist_parry,//���� 16
    combat_attr_resist_shipo,//��ʶ�� 17
    combat_attr_resist_xixue,//����Ѫ 18
    combat_attr_resist_chaos,//������ 19
    combat_attr_resist_podan,//���Ƶ� 20
    combat_attr_resist_weihe,//������ 21

    combat_attr_sub_damage_wuli,        //�����˺����� 22
    combat_attr_sub_damage_celue,    //�����˺����� 23

    combat_attr_ke_bubing,    //�Բ������� 24
    combat_attr_ke_gbing,        //�Թ������� 25
    combat_attr_ke_moushi,    //�Բ�ʿ���� 26
    combat_attr_ke_qibing,    //����ʿ���� 27
    combat_attr_ke_qixie,        //����е���� 28

    combat_attr_fang_bubing,    //�Բ������� 29
    combat_attr_fang_gbing,    //�Թ������� 30
    combat_attr_fang_moushi,    //�Բ�ʿ���� 31
    combat_attr_fang_qibing,    //����ʿ���� 32
    combat_attr_fang_qixie,    //����е���� 33

    combat_attr_death_fight,    //��ս 34

    combat_attr_inspire,        //���� 35

    combat_attr_shiqi,        //��ʼʿ�� 36

    combat_attr_max
};

struct baseAttribute
{
    int type;
    bool isChance;
    std::string name;
};

class attributeMgr
{
public:
    boost::shared_ptr<baseAttribute> getAttribute(int type);
    static attributeMgr* getInstance();

private:
    static attributeMgr* m_handle;

    void reload();
    std::map<int, boost::shared_ptr<baseAttribute> > m_attributes;
};

