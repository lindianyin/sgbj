
#pragma once

class heroCard
{
public:
    heroCard();
private:
    int m_cardType;
    std::string m_cardName;
    int m_magic;     //����ħ��
    int m_hp;       //����Ѫ��
    int m_attack;   //������
    int m_defense;  //������
    int m_rarity;   //ϡ�ж�
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

