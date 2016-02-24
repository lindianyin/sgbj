
#pragma once

#include "net.h"

const int iMaxGeneralSoulLevel = 100;

//重置将魂金币消耗
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

    base_general_soul_attribute attr[6];            //单级增加属性
    base_general_soul_attribute total_attr[6];     //总属性

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

//查询将魂信息
int ProcessQueryGSoulInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//查询将魂属性
int ProcessQueryGSoulAttr(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//升级将魂
int ProcessUpgradeGSoul(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//重置将魂
int ProcessResetGSoul(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);


