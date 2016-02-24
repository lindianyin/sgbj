#pragma once

#include <string>
#include "json_spirit.h"
#include "spls_const.h"

const int iNewWeaponUpgrade[max_map_id][20] =
{
    {50, 100, 110, 120, 140, 160, 190, 220, 260, 300, 340, 380, 420, 460, 510, 560, 610, 660, 710, 760},
    {800, 850, 900, 950, 1000, 1060, 1120, 1180, 1240, 1300, 1370, 1440, 1530, 1610, 1680, 1750, 1830, 1910, 1990, 2070},
    {2150, 2230, 2320, 2420, 2530, 2650, 2780, 2920, 3070, 3230, 3400, 3580, 3770, 3970, 4180, 4400, 4630, 4870, 5120, 5370},
    {5620, 5880, 6150, 6430, 6720, 7020, 7330, 7650, 7980, 8320, 8670, 9030, 9400, 9780, 10170, 10570, 10980, 11400, 11830, 12270},
    {12720, 13180, 13650, 14130, 14620, 15120, 15630, 16150, 16680, 17220, 17770, 18330, 18900, 19480, 20070, 20670, 21280, 21900, 22530, 23170},
    {23820, 24520, 25220, 25920, 26620, 27320, 28020, 28720, 29420, 30120, 30820, 31520, 32220, 32920, 33620, 34320, 35020, 35720, 36420, 37120},
    {23820, 24520, 25220, 25920, 26620, 27320, 28020, 28720, 29420, 30120, 30820, 31520, 32220, 32920, 33620, 34320, 35020, 35720, 36420, 37120},
    {23820, 24520, 25220, 25920, 26620, 27320, 28020, 28720, 29420, 30120, 30820, 31520, 32220, 32920, 33620, 34320, 35020, 35720, 36420, 37120}
};

struct baseNewWeapon
{
    int _id;            //����Ψһid
    int _mapid;            //���ŵ�ͼ
    int _openLevel;    //���ŵȼ�
    int _openStronghold;    //���Źؿ�
    int _type;            //�������� 1 ��ͨ���� 2 ��ͨ���� 3 ���Թ��� 4 ���Է��� 5 ����
    int _baseCost;        //����������������
    int _baseEffect;    //1�����ӵ���ֵ
    int _effectPerLevel;//ÿ�����ӵ���ֵ
    int _maxLevel;        //��ߵȼ�
    int _quality;        //Ʒ�� 1 - 5 ,��/��/��/��/��
    std::string _name;    //����
    std::string _memo;    //����

    int effect(int level);    //���ݵȼ����ؼӳ���ֵ
    baseNewWeapon* nextLevel(int level, int& next_level);    //��һ��
    int levelCost(int level);
};

struct newWeapon
{
    int _level;
    int _effect;
    int _cost;
    int _type;
    baseNewWeapon* _baseWeapon;

    newWeapon()
    {
        _level = 0;
        _effect = 0;
        _type = 0;
        _cost = 0;
        _baseWeapon = NULL;
    }
    ~newWeapon()
    {
    }
};

struct CharNewWeapons
{
    int _cid;    //��ɫid
    newWeapon _weapons[5];
    int _score;
    int _power_pu;
    int _power_ce;

    void getList(json_spirit::Array& wlist);
    void updateNewAttack();
    int getNewScore(){return _score;}
    int getNewPower_pu(){return _power_pu;}
    int getNewPower_ce(){return _power_ce;}

    CharNewWeapons(int cid)
    :_cid(cid)
    {
        _weapons[0]._type = 1;
        _weapons[1]._type = 2;
        _weapons[2]._type = 3;
        _weapons[3]._type = 4;
        _weapons[4]._type = 5;
        _score = 0;
        _power_pu = 0;
        _power_ce = 0;
    }
};


class newWeaponMgr
{
public:
    baseNewWeapon* getWeapon(int id);
    baseNewWeapon* getDefaultWeapon(int type);
    static newWeaponMgr* getInstance();
    int openLevel(int type);
    int openLevel();

    int openStronghold(int type);
    int openStronghold();

private:
    static newWeaponMgr* m_handle;
    int reload();

    int m_max_new_weapon;
    baseNewWeapon _base_new_weapons[100];        //5��ͼ��ÿ��ͼ5������
};

