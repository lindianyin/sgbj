
#include "equipt_upgrade.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"

Database& GetDb();

#include "utils_all.h"
using namespace std;

//升级装备的打折活动
volatile int g_equiptment_enhance_discount = 100;

equipmentUpgrade* equipmentUpgrade::m_handle = NULL;

equipmentUpgrade* equipmentUpgrade::getInstance()
{
    if (NULL == m_handle)
    {
        m_handle = new equipmentUpgrade();
    }
    return m_handle;
}

equipmentUpgrade::equipmentUpgrade()
{
    Query q(GetDb());
    //基础状态
    q.get_result("SELECT level,ring,cloth,shield,fu,sword,necklace FROM base_equipment_upgrade_cost WHERE 1 order by level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        if (level > iMaxEquipmentLevel)
        {
            ERR();
            continue;
        }
        for (int i = 0; i < iTotalEquipmentType; ++i)
        {
            //升级消耗银币
            upgradeCost[i][level-1] = q.getval();
        }
    }
    q.free_result();

    for (int t = 0; t < iTotalEquipmentType; ++t)
    {
        for (int l = 0; l < iMaxEquipmentLevel; ++l)
        {
            if (upgradeCost[t][l] == 0)
            {
                if (l == 0)
                {
                    upgradeCost[t][l] = 100;
                }
                else
                {
                    upgradeCost[t][l] = upgradeCost[t][l-1] * 105/100;
                }
            }
        }
    }

    memset(upgradeAdd, 0, sizeof(int)*iTotalEquipmentType*iUpEquipmentQuality);

    q.get_result("select quality,ring,cloth,shield,fu,sword,necklace from base_equipment_upgrade_add where 1 order by quality");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int quality = q.getval();
        if (quality >= 0 && quality < iUpEquipmentQuality)
        {
            for (int i = 0; i < iTotalEquipmentType; ++i)
            {
                upgradeAdd[i][quality] = q.getval();
            }
        }
    }
    q.free_result();

    //强化打折活动
    g_equiptment_enhance_discount = GeneralDataMgr::getInstance()->getInt("enhance_discount2", 100);
    //打折时间的折扣是8.8折
    m_discount = GeneralDataMgr::getInstance()->getInt("enhance_discount", 88);

    //更新打折时间段
    updateDiscountTime();
}

equipmentUpgrade::~equipmentUpgrade()
{
}

void equipmentUpgrade::updateDiscountTime()
{
    time_t timex = time(NULL);
    struct tm tm1;
    localtime_r(&timex, &tm1);
    tm1.tm_hour = 12;
    tm1.tm_min = 0;
    tm1.tm_sec = 0;

    struct tm tm2 = tm1;
    tm2.tm_hour = 12;
    tm2.tm_min = 30;
    tm2.tm_sec = 0;

    struct tm tm3 = tm1;
    tm3.tm_hour = 19;
    tm3.tm_min = 0;
    tm3.tm_sec = 0;

    struct tm tm4 = tm1;
    tm4.tm_hour = 19;
    tm4.tm_min = 30;
    tm4.tm_sec = 0;

    m_discount_time[0] = mktime(&tm1);
    m_discount_time[1] = mktime(&tm2);
    m_discount_time[2] = mktime(&tm3);
    m_discount_time[3] = mktime(&tm4);
}

bool equipmentUpgrade::inDiscountTime()
{
    time_t t = time(NULL);
    return (t >= m_discount_time[0] && t <= m_discount_time[1]) || (t >= m_discount_time[2] && t <= m_discount_time[3]);
}

//获得升级增加的属性，返回0就是不能升级了,type=装备类型 1-5
int equipmentUpgrade::getUpgradeValue(int quality, int type, int level, int& value2)
{
    if (level >= 1 && level <= iMaxEquipmentLevel
        && quality >= 0 && quality < iUpEquipmentQuality
        && type >= 1 && type <= iTotalEquipmentType)
    {
        if (type == equip_necklace)
        {
            value2 = upgradeAdd[type-1][quality];
            return value2;
        }
        else
        {
            value2 = 0;
            return upgradeAdd[type-1][quality];
        }
    }
    value2 = 0;
    return 0;
}

int equipmentUpgrade::convertUpgradeLevel(int quality, int type, int level)
{
    (void)quality;
    if (level >= 1 && level <= iMaxEquipmentLevel
        && type >= 1 && type <= iTotalEquipmentType)
    {
        if (level > 5)
        {
            return level - 5;
        }
        else
        {
            return 1;
        }
    }
    return 0;
}

//获得升级增加的属性，返回0就是不能升级了
int equipmentUpgrade::getUpgradeNeeds(int quality, int type, int level, int& org_silver)
{
    (void)quality;
    if (level >= 1 && level <= iMaxEquipmentLevel
        && type >= 1 && type <= iTotalEquipmentType)
    {
        org_silver = upgradeCost[type-1][level-1];
        int discount = g_equiptment_enhance_discount;
        if (inDiscountTime())
        {
            discount = m_discount;
            if (g_equiptment_enhance_discount < m_discount)
            {
                discount = g_equiptment_enhance_discount;
            }
        }
        if (discount != 100)
        {
            int cost = org_silver * discount / 100;
            if (cost <= 0)
            {
                cost = 1;
            }
            return cost;
        }
        else
        {
            //cout<<"getUpgradeNeeds return "<<upgradedata[(quality-1)*quality*10 + level - 1][5]<<":"<<quality<<","<<level<<endl;
            return org_silver;
        }
    }
    else
    {
        //cout<<"getUpgradeNeeds return 0:"<<quality<<","<<level<<endl;
        return 0;
    }
}

//最大可强化等级
int equipmentUpgrade::getMaxLevel(int quality)
{
    (void)quality;
    return iMaxEquipmentLevel;        // 最大强化等级
}

