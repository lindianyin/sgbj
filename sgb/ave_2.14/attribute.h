#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include <map>

enum combat_attr_enum
{
    combat_attr_none = 0,

    combat_attr_pugong,    //普通攻击 1
    combat_attr_pufang,    //普通防御 2
    combat_attr_cegong,    //策略攻击 3
    combat_attr_cefang,    //策略防御 4
    combat_attr_bingli,    //兵力 5

    combat_attr_baoji,    //暴击 6
    combat_attr_dodge,    //躲闪 7
    combat_attr_parry,    //格挡 8 
    combat_attr_shipo,    //识破 9
    combat_attr_xixue,    //吸血 10
    combat_attr_chaos,    //混乱 11
    combat_attr_podan,    //破胆 12
    combat_attr_weihe,    //威慑 13

    combat_attr_resist_baoji,//抗暴击 14
    combat_attr_resist_dodge,//抗躲闪 15
    combat_attr_resist_parry,//抗格挡 16
    combat_attr_resist_shipo,//抗识破 17
    combat_attr_resist_xixue,//抗吸血 18
    combat_attr_resist_chaos,//抗混乱 19
    combat_attr_resist_podan,//抗破胆 20
    combat_attr_resist_weihe,//抗威慑 21

    combat_attr_sub_damage_wuli,        //物理伤害减少 22
    combat_attr_sub_damage_celue,    //策略伤害减少 23

    combat_attr_ke_bubing,    //对步兵克制 24
    combat_attr_ke_gbing,        //对弓兵克制 25
    combat_attr_ke_moushi,    //对策士克制 26
    combat_attr_ke_qibing,    //对骑士克制 27
    combat_attr_ke_qixie,        //对器械克制 28

    combat_attr_fang_bubing,    //对步兵防御 29
    combat_attr_fang_gbing,    //对弓兵防御 30
    combat_attr_fang_moushi,    //对策士防御 31
    combat_attr_fang_qibing,    //对骑士防御 32
    combat_attr_fang_qixie,    //对器械防御 33

    combat_attr_death_fight,    //死战 34

    combat_attr_inspire,        //鼓舞 35

    combat_attr_shiqi,        //初始士气 36

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

