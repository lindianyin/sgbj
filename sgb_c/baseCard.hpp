
#pragma once

class heroCard
{
public:
    heroCard();
private:
    int m_cardType;
    std::string m_cardName;
    int m_magic;     //»ù´¡Ä§Á¦
    int m_hp;       //»ù´¡ÑªÁ¿
    int m_attack;   //»ù´¡¹¥
    int m_defense;  //»ù´¡·À
    int m_rarity;   //Ï¡ÓÐ¶È
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

