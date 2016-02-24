#pragma once

enum admin_cmds_enum
{
    admin_empty_cmd = 0,
    //ÿ�ո���
    admin_daily_reset = 1,
    //ÿ�ܸ���
    admin_week_reset,
    //����
    admin_heartBeat,
    //�����������
    admin_save_db,
    //��ҽ���
    admin_gag_char,
    //�����ʺ�
    admin_freeze_account,
    //���δ���ʼ�
    admin_check_mail,
    //ÿ�վ���������
    admin_arenaAwards,
    //���ý�ɫ�ȼ�
    admin_setLevel,
    //���ý�ɫVIP
    admin_setVip,
    //�ӵ�ȯ
    admin_addGold,
    //�����
    admin_addBindgold,
    //�ӳ���
    admin_addSilver,
    //������
    admin_addPrestige,
    //�ӵ���
    admin_addGem,
    //��Ӣ��
    admin_addHero,
    //����Ӣ�۵ȼ�
    admin_heroLevel,
    //��װ��
    admin_addEquiptment,
    //����װ���ȼ�
    admin_setEquiptment,
    //�ӱ�ʯ
    admin_addBaoshi,
    //�������
    admin_add_libao,
    //���¼��س�ֵ�
    admin_reload_recharge_event,
    //ÿ�ճ��뾺������
    admin_pkAwards,
    //����bossս��
    admin_openBoss,
    //�ر�bossս��
    admin_closeBoss,
    //�����齱�
    admin_open_lottery,
    //�رճ齱�
    admin_close_lottery,

    //�޸�Ӣ��״̬
    admin_fix_hero_state,

	//�ı��̳ǵ��ۿ�(�ۿۻ)
	admin_change_mall_discount,


};

#include <string>

void initAdminCmds();
int getAdminCmds(const std::string& cmds);

