
#pragma once

#include "net.h"

const int iMaxGeneralSoulLevel = 100;

//���ý���������
const int iResetGeneralSoulGold = 500;

struct base_general_soul_attribute
{
    int attack;
    int wufang;
    int cefang;
    int hp;

    base_general_soul_attribute()
    {
        attack = 0;
        wufang = 0;
        cefang = 0;
        hp = 0;
    }

    base_general_soul_attribute& operator+=(const base_general_soul_attribute& a)
    {
        attack += a.attack;
        wufang += a.wufang;
        cefang += a.cefang;
        hp += a.hp;
        return *this;
    }
};

struct base_general_soul
{
    int level;
    int cost;
    int total_cost;

    base_general_soul_attribute attr[6];            //������������
    base_general_soul_attribute total_attr[6];     //������

    base_general_soul* next;

    int getAttack(int quality)
    {
        if (quality >= 0 && quality <= 5)
        {
            return total_attr[quality].attack;
        }
        return 0;
    }
    int getWufang(int quality)
    {
        if (quality >= 0 && quality <= 5)
        {
            return total_attr[quality].wufang;
        }
        return 0;
    }
    int getCefang(int quality)
    {
        if (quality >= 0 && quality <= 5)
        {
            return total_attr[quality].cefang;
        }
        return 0;
    }
    int getBingli(int quality)
    {
        if (quality >= 0 && quality <= 5)
        {
            return total_attr[quality].hp;
        }
        return 0;
    }

    int getAttack2(int quality)
    {
        if (quality >= 0 && quality <= 5)
        {
            return attr[quality].attack;
        }
        return 0;
    }
    int getWufang2(int quality)
    {
        if (quality >= 0 && quality <= 5)
        {
            return attr[quality].wufang;
        }
        return 0;
    }
    int getCefang2(int quality)
    {
        if (quality >= 0 && quality <= 5)
        {
            return attr[quality].cefang;
        }
        return 0;
    }
    int getBingli2(int quality)
    {
        if (quality >= 0 && quality <= 5)
        {
            return attr[quality].hp;
        }
        return 0;
    }    
};

class charGeneralSoulMgr
{
public:
    charGeneralSoulMgr();
    void reload();
    base_general_soul* getSoul(int level);
    
private:
    base_general_soul m_general_souls[iMaxGeneralSoulLevel];
};

//��ѯ������Ϣ
int ProcessQueryGSoulInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ��������
int ProcessQueryGSoulAttr(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��������
int ProcessUpgradeGSoul(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//���ý���
int ProcessResetGSoul(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);


