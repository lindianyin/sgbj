#include "admin_cmds.h"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

static boost::unordered_map<std::string, int>    m_admin_cmds;
static bool g_admin_cmds_inited = false;

void initAdminCmds()
{
    if (g_admin_cmds_inited)
    {
        return;
    }
    //ÿ�ո���
    m_admin_cmds["daily_reset"] = admin_daily_reset;
    //ÿ�ܸ���
    m_admin_cmds["week_reset"] = admin_week_reset;
    //��������
    m_admin_cmds["heartBeat"] = admin_heartBeat;
    //�����������
    m_admin_cmds["save_db"] = admin_save_db;
    //����
    m_admin_cmds["gagChar"] = admin_gag_char;
    //�����ʺ�
    m_admin_cmds["freezeAccount"] = admin_freeze_account;
    //���δ���ʼ�
    m_admin_cmds["checkMail"] = admin_check_mail;
    //ÿ�վ���������
    m_admin_cmds["arenaAwards"] = admin_arenaAwards;
    //ÿ�ճ��뾺������
    m_admin_cmds["pkAwards"] = admin_pkAwards;
    //���ý�ɫ�ȼ�
    m_admin_cmds["setLevel"] = admin_setLevel;
    //���ý�ɫVIP
    m_admin_cmds["setVip"] = admin_setVip;
    //�ӵ�ȯ
    m_admin_cmds["addGold"] = admin_addGold;
    //�����
    m_admin_cmds["addBindgold"] = admin_addBindgold;
    //�ӳ���
    m_admin_cmds["addSilver"] = admin_addSilver;
    //������
    m_admin_cmds["addPrestige"] = admin_addPrestige;
    //�ӵ���
    m_admin_cmds["addGem"] = admin_addGem;
    //��Ӣ��
    m_admin_cmds["addHero"] = admin_addHero;
    //����Ӣ�۵ȼ�
    m_admin_cmds["heroLevel"] = admin_heroLevel;
    //��װ��
    m_admin_cmds["addEquiptment"] = admin_addEquiptment;
    //����װ���ȼ�
    m_admin_cmds["setEquiptment"] = admin_setEquiptment;
    //�ӱ�ʯ
    m_admin_cmds["addBaoshi"] = admin_addBaoshi;
    //�������
    m_admin_cmds["addLibao"] = admin_add_libao;
    //���¼��س�ֵ�
    m_admin_cmds["reloadRechargeEvent"] = admin_reload_recharge_event;
    //����bossս��
    m_admin_cmds["openBoss"] = admin_openBoss;
    //�ر�bossս��
    m_admin_cmds["closeBoss"] = admin_closeBoss;
    //�����齱�
    m_admin_cmds["openLottery"] = admin_open_lottery;
    //�رճ齱�
    m_admin_cmds["closeLottery"] = admin_close_lottery;

    //�޸�����Ӣ���쳣
    m_admin_cmds["fixHeroState"] = admin_fix_hero_state;

	m_admin_cmds["changeMallDiscount"] = admin_change_mall_discount;

    g_admin_cmds_inited = true;
}

int getAdminCmds(const std::string& cmds)
{
    if (!g_admin_cmds_inited)
    {
        initAdminCmds();
    }
    boost::unordered_map<std::string, int>::iterator it = m_admin_cmds.find(cmds);
    if (it != m_admin_cmds.end())
    {
        return it->second;
    }
    else
    {
        return admin_empty_cmd;
    }
}

