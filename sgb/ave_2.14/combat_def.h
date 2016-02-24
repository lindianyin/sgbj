
#ifndef _COMBAT_DEF_H_
#define _COMBAT_DEF_H_

//物理攻击的伤害系数
#define WU_LI_FAC 1
//策略攻击的伤害系数
#define CE_LUE_FAC 1

#define POS_TO_ROW(pos) ((pos-1)%3)
#define POS_TO_SIDE(pos) ((pos-1)/3)

#define CHECK_VALID_ROW(row, ret) if (row < 0 || row > 2) return ret
#define CHECK_VALID_POS(pos, ret) if (pos < 1 || pos > 9) return ret
#define IS_END_POS(pos) (7 <= pos && pos >= 9)

enum Attack_result
{
    ATTACK_RESULT_SUCCESS = 1,
    ATTACK_RESULT_FAIL = 2,
    ATTACK_RESULT_DUOSHAN = 3,
    ATTACK_RESULT_GEDANG = 4,
    ATTACK_RESULT_SHIPO = 5,
    ATTACK_RESULT_BAOJI = 6,
    ATTACK_RESULT_HEAL = 7,
    ATTACK_RESULT_GEDANG_DUOSHAN = 8,
    ATTACK_RESULT_SHIPO_DUOSHAN = 9,
    ATTACK_RESULT_ADD_NUQI = 10,
    ATTACK_RESULT_SUB_NUQI = 11,
};

enum damage_type
{
    min_damage = 0,
    wuli_damage = 1,    //物理伤害
    celue_damage = 2,   //策略伤害
    weihe_damage = 10,  //威吓
    gedang_damage = 11, //格挡伤害(物理)
    shipo_damage = 12,  //识破伤害(策略)
    max_damage
};

//特殊攻击类型
enum special_attack_type
{
    special_attack_baoji = 0,
    special_attack_dodge,
    special_attack_parry,
    special_attack_shipo,
    special_attack_xixue,
    special_attack_chaos,
    special_attack_podan,
    special_attack_weihe,
    special_attack_max
};

enum action_type
{
    act_wuli_attack = 1,//物理攻击
    act_celue_attack,   //策略攻击
    act_heal,           //治疗
    act_add_shiqi       //加士气
};

//攻击范围
enum target_range
{
    range_none = 0,
    range_single = 1,   //单体
    range_chuantou,     //穿透
    range_fenlie,       //分裂
    range_around,       //眦邻
    range_single_back,  //单体 (后排)
    range_three,        //引导
    range_all,          //全体 (对方)
    range_self_single,  //单体 (己方)
    range_self_all      //全体 (己方)
};

enum combat_result
{
    combating = 0,
    attacker_win = 1,
    defender_win = 2,
    combat_draw = 3
};

enum combat_type
{
    combat_stronghold = 1,
    combat_race = 2,
    combat_boss = 3,
    combat_camp_race = 4,
    combat_guard = 5,
    combat_group_copy = 6,
    combat_trade = 7,
    combat_servant = 8,
    combat_elite = 9,
    combat_maze = 10,
    combat_maze_boss = 11,
    combat_corps_fighting = 12,
    combat_zst = 13
};

enum base_soldier_enum
{
    base_soldier_bubing = 1,
    base_soldier_gongbing = 2,
    base_soldier_moushi = 3,
    base_soldier_qibing = 4,
    base_soldier_qixie = 5
};

//特殊状态
enum special_state
{
    baoji_state = 0,    //暴击 0
    dodge_state,        //躲闪    1
    parry_state,        //格挡     2
    shipo_state,        //识破    3
    xixue_state,        //吸血    4
    chaos_state,        //混乱    5
    podan_state,        //破胆    6
    weihe_state,        //威吓    7

    baowei_state,        //包围-提高包围技能，提高对步兵伤害    8
    zhanma_state,        //斩马-提高斩马技能，提高对骑兵伤害    9
    shanji_state,        //闪击-提高闪击技能，提高对弓兵伤害 10
    qixi_state,        //奇袭-提高奇袭技能，提高对策士伤害    11
    dongxi_state,        //洞悉-提高洞悉技能，提高对器械伤害    12
    fenge_state,        //分割-提高分割技能，提高对步兵防御    13
    zujie_state,        //阻截-提高阻截技能，提高对骑兵防御    14
    linjia_state,        //鳞甲-提高鳞甲技能，提高对弓箭防御    15
    guidao_state,        //诡道-提高诡道技能，提高对策士防御    16
    moshou_state,        //墨守-提高墨守技能，提高对器械防御    17
    baqi_state,        //霸气-提高霸气技能，提高攻击的伤害 18
    wuwei_state,        //无畏-提高无畏技能，提高普通的防御    19
    guzhu_state,        //孤注一掷-提高孤注一掷技能，提高普通伤害 20
    shanzhan_state,    //能征善战-提高能征善战技能，提高普通防御 21
    yunchou_state,        //运筹帷幄-提高运筹帷幄技能，提高策略伤害 22
    mingjing_state,    //明镜止水-提高明镜止水技能，提高策略防御 23
    
    pufang_state,        //普通防御，减少所受到普通攻击的伤害的50% 24
    cefang_state,        //策略防御，减少所受到策略攻击的伤害的50% 25
    bubingfang_state,    //步兵防御，减少所受到步兵攻击的伤害的50% 26
    bubingke_state,    //步兵克制，增加对步兵的伤害的50% 27
    gongbingfang_state,    //弓兵防御，减少所受到弓兵攻击的伤害的50% 28
    gongbingke_state,    //弓兵克制，增加对弓兵的伤害的50%     29
    qibingfang_state,    //骑兵防御，减少所受到骑兵攻击的伤害的50%    30
    qibingke_state,    //骑兵克制，增加对骑兵的伤害的50%    31
    moushifang_state,    //谋士防御，减少所受到谋士攻击的伤害的50%    32
    moushike_state,    //谋士克制，增加对谋士的伤害的50%    33
    qixiefang_state,    //器械防御，减少所受到器械攻击的伤害的50%    34
    qixieke_state,        //器械克制，增加对器械的伤害的50%    35

    shizhan_state,        //死战，士兵数量每减少1%,所造成的伤害增加1%.    36
    weak_state,        //虚弱状态，伤害降低
    special_state_max
};

//特长效果类别
enum genius_type
{
    genius_type_bubing_fangyu = 1,//1.增加步兵防御
    genius_type_gongbing_fangyu,//2.增加弓兵防御
    genius_type_qibing_fangyu,//3.增加骑兵防御
    genius_type_moushi_fangyu,//4.增加谋士防御
    genius_type_jixie_fangyu,//5.增加机械防御
    genius_type_putong_fangyu,//6.增加普通防御
    genius_type_celue_fangyu,//7.增加策略防御

    genius_type_bubing_shanghai,//8.增加步兵伤害
    genius_type_gongbing_shanghai,//9.增加弓兵伤害
    genius_type_qibing_shanghai,//10.增加骑兵伤害
    genius_type_moushi_shanghai,//11.增加谋士伤害
    genius_type_jixie_shanghai,//12.增加机械伤害
    genius_type_BGQ_shanghai,//13.增加步兵弓兵骑兵伤害
    genius_type_MJ_shanghai,//14.增加谋士机械伤害

    genius_type_baoji,//15.增加暴击概率
    genius_type_dodge,//16.增加闪避概率
    genius_type_parry,//17.增加格挡概率
    genius_type_shipo,//18.增加识破概率
    genius_type_chaos,//19.增加混乱概率
    genius_type_weihe,//20.增加逃兵概率
    genius_type_BAndD,//21.增加暴击&闪避概率
    genius_type_PAndS,//22.增加格挡&识破概率
    genius_type_CAndW,//23.增加混乱&逃兵概率

    genius_type_baoji_d,//24.增加抗暴击概率
    genius_type_dodge_d,//25.增加抗闪避概率
    genius_type_parry_d,//26.增加抗格挡概率
    genius_type_shipo_d,//27.增加抗识破概率
    genius_type_chaos_d,//28.增加抗混乱概率
    genius_type_weihe_d,//29.增加抗逃兵概率
    genius_type_BAndD_d,//30.增加抗暴击&抗闪避概率
    genius_type_PAndS_d,//31.增加抗格挡&抗识破概率
    genius_type_CAndW_d,//32.增加抗混乱&抗逃兵概率

    genius_type_attack_times,//33.单体攻击翻倍
    genius_type_attack_type,//34.单体攻击转换类别
};


//主将技能效果
enum skill_effect
{
    skill_add_hp = 0,
    skill_add_pugong,  // 1
    skill_add_pufang,
    skill_add_cegong,  // 3
    skill_add_cefang,
    skill_add_gong_to_bubing,    // 5
    skill_add_fang_to_bubing,
    skill_add_gong_to_gongbing,
    skill_add_fang_to_gongbing,
    skill_add_gong_to_moushi,
    skill_add_fang_to_moshi,
    skill_add_gong_to_qibing,
    skill_add_fang_to_qibing,
    skill_add_gong_to_qixie,    // 13
    skill_add_fang_to_qixie,
    skill_add_max
};

//主将技能

//领袖
//霸气
//无畏
//突进
//固守
//孤注一掷
//能征善战
//机智
//谨慎
//运筹帷幄
//明镜止水
//包围
//斩马
//闪击
//奇袭
//洞悉
//分割
//阻截
//鳞甲
//诡道
//墨守

#endif

