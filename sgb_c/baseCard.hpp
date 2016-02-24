
#pragma once

class heroCard
{
public:
    heroCard();
private:
    int m_cardType;
    std::string m_cardName;
    int m_magic;     //基础魔力
    int m_hp;       //基础血量
    int m_attack;   //基础攻
    int m_defense;  //基础防
    int m_rarity;   //稀有度
};

class charCard
{
public:
    

private:
    heroCard* base;
    int m_level;
    int m_exp;

    int m_hp;
    int m_magic;
    int m_attack;
    int m_defense;
};

