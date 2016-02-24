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
    //����������ӵ����ԣ�����0���ǲ���������,type=װ������ 1-5
    int getUpgradeValue(int quality, int type, int level, int& value2);
    //�����������
    int getUpgradeNeeds(int quality, int type, int level, int& org);
    //Ʒ��������ĵȼ�ת��
    int convertUpgradeLevel(int quality, int type, int level);
    //����ǿ���ȼ�
    int getMaxLevel(int quality);
    static equipmentUpgrade* getInstance();

    void updateDiscountTime();

    int Discount() { return m_discount; }

    void setDiscount(int d) { m_discount = d; }

private:
    bool inDiscountTime();

    static equipmentUpgrade* m_handle;
    int upgradeCost[iTotalEquipmentType][iMaxEquipmentLevel];    //����
    int upgradeAdd[iTotalEquipmentType][iUpEquipmentQuality];

    time_t m_discount_time[4];
    int m_discount;
};

