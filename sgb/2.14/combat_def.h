
#ifndef _COMBAT_DEF_H_
#define _COMBAT_DEF_H_

//���������˺�ϵ��
#define WU_LI_FAC 1
//���Թ������˺�ϵ��
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
    wuli_damage = 1,    //�����˺�
    celue_damage = 2,   //�����˺�
    weihe_damage = 10,  //����
    gedang_damage = 11, //���˺�(����)
    shipo_damage = 12,  //ʶ���˺�(����)
    max_damage
};

//���⹥������
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
    act_wuli_attack = 1,//������
    act_celue_attack,   //���Թ���
    act_heal,           //����
    act_add_shiqi       //��ʿ��
};

//������Χ
enum target_range
{
    range_none = 0,
    range_single = 1,   //����
    range_chuantou,     //��͸
    range_fenlie,       //����
    range_around,       //����
    range_single_back,  //���� (����)
    range_three,        //����
    range_all,          //ȫ�� (�Է�)
    range_self_single,  //���� (����)
    range_self_all      //ȫ�� (����)
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

//����״̬
enum special_state
{
    baoji_state = 0,    //���� 0
    dodge_state,        //����    1
    parry_state,        //��     2
    shipo_state,        //ʶ��    3
    xixue_state,        //��Ѫ    4
    chaos_state,        //����    5
    podan_state,        //�Ƶ�    6
    weihe_state,        //����    7

    baowei_state,        //��Χ-��߰�Χ���ܣ���߶Բ����˺�    8
    zhanma_state,        //ն��-���ն���ܣ���߶�����˺�    9
    shanji_state,        //����-����������ܣ���߶Թ����˺� 10
    qixi_state,        //��Ϯ-�����Ϯ���ܣ���߶Բ�ʿ�˺�    11
    dongxi_state,        //��Ϥ-��߶�Ϥ���ܣ���߶���е�˺�    12
    fenge_state,        //�ָ�-��߷ָ�ܣ���߶Բ�������    13
    zujie_state,        //���-�����ؼ��ܣ���߶��������    14
    linjia_state,        //�ۼ�-����ۼ׼��ܣ���߶Թ�������    15
    guidao_state,        //���-��߹�����ܣ���߶Բ�ʿ����    16
    moshou_state,        //ī��-���ī�ؼ��ܣ���߶���е����    17
    baqi_state,        //����-��߰������ܣ���߹������˺� 18
    wuwei_state,        //��η-�����η���ܣ������ͨ�ķ���    19
    guzhu_state,        //��עһ��-��߹�עһ�����ܣ������ͨ�˺� 20
    shanzhan_state,    //������ս-���������ս���ܣ������ͨ���� 21
    yunchou_state,        //�˳���-����˳��ᢼ��ܣ���߲����˺� 22
    mingjing_state,    //����ֹˮ-�������ֹˮ���ܣ���߲��Է��� 23
    
    pufang_state,        //��ͨ�������������ܵ���ͨ�������˺���50% 24
    cefang_state,        //���Է������������ܵ����Թ������˺���50% 25
    bubingfang_state,    //�����������������ܵ������������˺���50% 26
    bubingke_state,    //�������ƣ����ӶԲ������˺���50% 27
    gongbingfang_state,    //�����������������ܵ������������˺���50% 28
    gongbingke_state,    //�������ƣ����ӶԹ������˺���50%     29
    qibingfang_state,    //����������������ܵ�����������˺���50%    30
    qibingke_state,    //������ƣ����Ӷ�������˺���50%    31
    moushifang_state,    //ıʿ�������������ܵ�ıʿ�������˺���50%    32
    moushike_state,    //ıʿ���ƣ����Ӷ�ıʿ���˺���50%    33
    qixiefang_state,    //��е�������������ܵ���е�������˺���50%    34
    qixieke_state,        //��е���ƣ����Ӷ���е���˺���50%    35

    shizhan_state,        //��ս��ʿ������ÿ����1%,����ɵ��˺�����1%.    36
    weak_state,        //����״̬���˺�����
    special_state_max
};

//�س�Ч�����
enum genius_type
{
    genius_type_bubing_fangyu = 1,//1.���Ӳ�������
    genius_type_gongbing_fangyu,//2.���ӹ�������
    genius_type_qibing_fangyu,//3.�����������
    genius_type_moushi_fangyu,//4.����ıʿ����
    genius_type_jixie_fangyu,//5.���ӻ�е����
    genius_type_putong_fangyu,//6.������ͨ����
    genius_type_celue_fangyu,//7.���Ӳ��Է���

    genius_type_bubing_shanghai,//8.���Ӳ����˺�
    genius_type_gongbing_shanghai,//9.���ӹ����˺�
    genius_type_qibing_shanghai,//10.��������˺�
    genius_type_moushi_shanghai,//11.����ıʿ�˺�
    genius_type_jixie_shanghai,//12.���ӻ�е�˺�
    genius_type_BGQ_shanghai,//13.���Ӳ�����������˺�
    genius_type_MJ_shanghai,//14.����ıʿ��е�˺�

    genius_type_baoji,//15.���ӱ�������
    genius_type_dodge,//16.�������ܸ���
    genius_type_parry,//17.���Ӹ񵲸���
    genius_type_shipo,//18.����ʶ�Ƹ���
    genius_type_chaos,//19.���ӻ��Ҹ���
    genius_type_weihe,//20.�����ӱ�����
    genius_type_BAndD,//21.���ӱ���&���ܸ���
    genius_type_PAndS,//22.���Ӹ�&ʶ�Ƹ���
    genius_type_CAndW,//23.���ӻ���&�ӱ�����

    genius_type_baoji_d,//24.���ӿ���������
    genius_type_dodge_d,//25.���ӿ����ܸ���
    genius_type_parry_d,//26.���ӿ��񵲸���
    genius_type_shipo_d,//27.���ӿ�ʶ�Ƹ���
    genius_type_chaos_d,//28.���ӿ����Ҹ���
    genius_type_weihe_d,//29.���ӿ��ӱ�����
    genius_type_BAndD_d,//30.���ӿ�����&�����ܸ���
    genius_type_PAndS_d,//31.���ӿ���&��ʶ�Ƹ���
    genius_type_CAndW_d,//32.���ӿ�����&���ӱ�����

    genius_type_attack_times,//33.���幥������
    genius_type_attack_type,//34.���幥��ת�����
};


//��������Ч��
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

//��������

//����
//����
//��η
//ͻ��
//����
//��עһ��
//������ս
//����
//����
//�˳���
//����ֹˮ
//��Χ
//ն��
//����
//��Ϯ
//��Ϥ
//�ָ�
//���
//�ۼ�
//���
//ī��

#endif

