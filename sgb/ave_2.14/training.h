#pragma once

#include <string>
#include "json_spirit.h"
#include "spls_const.h"
#include "combat.h"

const int iSoulNum = 5;
const int iTrainingNum = 3;

const int iTrainingOpenLevel[iTrainingNum] = {34,50,70};
const int iTrainingOpenStrongholdid[iTrainingNum] = {66,98,138};

struct baseSoul
{
    int _id;        // Ψһid
    int _type;        // 1,2,3 ���
    int _level;        // 1--30 �ȼ�
    int _stype;            //������� 1��2��3��4��5, 0��ʾ���Ļ�Ч����ȫ�嶼��
    int _moreDamage;    //��������˺��ٷֱ�
    int _bingli;        //���ֱ���
    int _attack;        //���ֹ���
    int _wufang;        //�������
    int _cefang;        //���ֲ߷�

    int _baseCost;        // 1,2,3 ������Ƭ���
    int _costCount;    //������������
    baseSoul* _next;    //��һ������
};

struct charSoldierSoul
{
    int type;    // 1,2,3
    baseSoul* _soul[iSoulNum];
    baseSoul* _centerSoul;
    charSoldierSoul()
    {
        type = 0;
        for (int i = 0; i < iSoulNum; ++i)
        {
            _soul[i] = NULL;
        }
        _centerSoul = NULL;
    }
    ~charSoldierSoul()
    {
    }
};

struct CharTrainings
{
    int _cid;    //��ɫid

    charSoldierSoul m_soldier_souls[iTrainingNum];

    //��������ս�����Լӳ�
    combatAttribute _combatAttr;

    int _score;
    int getNewScore();

    void load(CharData& c);
    void checkSouls(CharData& c);
    void SaveSouls(int type);
    void getList(json_spirit::Array& slist);

    //��������
    int levelUp(int type, int stype, json_spirit::Object& robj);
    int minlevel(int type);

    void updateAttribute(CharData& c);

    void DaojuBack();

    CharTrainings(int cid)
    :_cid(cid)
    {
    }
};


class trainingMgr
{
public:
    trainingMgr();
    boost::shared_ptr<CharTrainings> getChar(CharData& c);
    //����id���ر���
    baseSoul* getSoul(int id);
    //�������Ͳ��ҳ�ʼ����
    baseSoul* getDefaultSoul(int type, int stype);
    int openLevel(int type);
    int openLevel();

    int openStronghold(int type);
    int openStronghold();

    int getSoulsDaojuCost(int times);

    void SoulsDaojuBack(int cid);
private:
    int reload();
    int m_max_soul_id;
    baseSoul _base_souls[10000];
    std::map<int, boost::shared_ptr<CharTrainings> > m_char_datas;
};

