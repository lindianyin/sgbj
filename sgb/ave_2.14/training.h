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
    int _id;        // 唯一id
    int _type;        // 1,2,3 类别
    int _level;        // 1--30 等级
    int _stype;            //兵种类别 1，2，3，4，5, 0表示中心魂效果，全体都加
    int _moreDamage;    //兵种相克伤害百分比
    int _bingli;        //兵种兵力
    int _attack;        //兵种攻击
    int _wufang;        //兵种物防
    int _cefang;        //兵种策防

    int _baseCost;        // 1,2,3 升级碎片类别
    int _costCount;    //升级消耗数量
    baseSoul* _next;    //下一级兵魂
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
    int _cid;    //角色id

    charSoldierSoul m_soldier_souls[iTrainingNum];

    //加起来的战斗属性加成
    combatAttribute _combatAttr;

    int _score;
    int getNewScore();

    void load(CharData& c);
    void checkSouls(CharData& c);
    void SaveSouls(int type);
    void getList(json_spirit::Array& slist);

    //升级兵魂
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
    //根据id返回兵魂
    baseSoul* getSoul(int id);
    //根据类型查找初始兵魂
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

