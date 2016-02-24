#pragma once

#include <time.h>

const int iMaxEquipmentLevel = 120;
const int iTotalEquipmentType = 6;
const int iTotalEquipmentQuality = 5;
const int iUpEquipmentQuality = 51;

class equipmentUpgrade
{
public:
    equipmentUpgrade();
    ~equipmentUpgrade();
    //获得升级增加的属性，返回0就是不能升级了,type=装备类型 1-5
    int getUpgradeValue(int quality, int type, int level, int& value2);
    //获得升级消耗
    int getUpgradeNeeds(int quality, int type, int level, int& org);
    //品质提升后的等级转换
    int convertUpgradeLevel(int quality, int type, int level);
    //最大可强化等级
    int getMaxLevel(int quality);
    static equipmentUpgrade* getInstance();

    void updateDiscountTime();

    int Discount() { return m_discount; }

    void setDiscount(int d) { m_discount = d; }

private:
    bool inDiscountTime();

    static equipmentUpgrade* m_handle;
    int upgradeCost[iTotalEquipmentType][iMaxEquipmentLevel];    //升级
    int upgradeAdd[iTotalEquipmentType][iUpEquipmentQuality];

    time_t m_discount_time[4];
    int m_discount;
};

