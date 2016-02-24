
#ifndef _IGENERAL_H_
#define _IGENERAL_H_

#include <string>
#include "json_spirit.h"

class Army;
struct baseBuff;
struct Buff;

class iGeneral
{
public:
    iGeneral() {};
    virtual ~iGeneral() {};
    //攻击地方单个武将
    virtual int Attack(iGeneral& target) = 0;
    //被攻击
    virtual int RecieveDamage(int damage, iGeneral& attacker, int attack_type, int injure_type, int attacker_base_type, bool test) = 0;

    //行动
    virtual int Action() = 0;
    //获得部队
    virtual Army& GetArmy() const = 0;
    virtual void setArmy(Army*) = 0;

    //保存
    virtual int Save() = 0;
    //返回id
    virtual int Id() = 0;

    virtual int UniqueId() = 0;

    virtual int level() const = 0;

    virtual const std::string& GetName() = 0;

    virtual int baseType() = 0;

    //受普通伤害和策略伤害的减免
    virtual int subDamage(int attack_type) = 0;
    //受来自特定兵种的伤害减免
    virtual int subDamageFrom(int base_stype) = 0;

    //取属性
    virtual int Gong(int to_type) = 0;
    virtual int Wufang(int from_type) = 0;
    virtual int CeFang(int from_type) = 0;
    virtual int Str() = 0;
    virtual int Int() = 0;
    virtual int Tongyu() = 0;
    virtual int Hp() = 0;
    virtual int MaxHp() = 0;
    virtual int Shiqi() = 0;
    virtual int Pos() = 0;
    virtual int GetBaoji() = 0;
    virtual int getSpic() = 0;

    //特殊攻击是否触发
    virtual int Weihe(const iGeneral& target) = 0;
    virtual int Chaos(const iGeneral& target) = 0;

    //抗性
    virtual int resist(int type) const = 0;

    virtual int resist_level(int type) const = 0;
    
    //是否格挡
    virtual int CheckGedang(const iGeneral& att) = 0;
    //是否躲闪
    virtual int CheckDodge(const iGeneral& att) = 0;
    //是否识破
    virtual int CheckShipo(const iGeneral& att) = 0;
    //是否兵种克制
    virtual bool CheckStypeDamage(int stype) = 0;
    //还活着
    virtual bool isLive() = 0;
    //加血
    virtual int Addhp(int hp) = 0;
    //加士气
    virtual int AddShiqi(int shiqi, bool type = false) = 0;
    //无攻击
    virtual bool NoAttack() = 0;

    virtual json_spirit::Object& GetObj() = 0;

    virtual json_spirit::Object& UpdateObj() = 0;

    virtual void GenOrgObj() = 0;

    virtual json_spirit::Object& GetOrgObj() = 0;

    virtual void GenAttackResultMsg(int result) = 0;

    virtual void addBuff(baseBuff* bb, iGeneral* from, int value, int last) = 0;    //增加一个buff

    virtual void addGeneratedBuff(boost::shared_ptr<Buff> b) = 0;    //增加一个产生的buff

    virtual void removeBuff(boost::shared_ptr<Buff> b) = 0;

    virtual void updateGenerateBuff() = 0;

    virtual int getBuffValue(int type) const = 0;    //获取一类buff的效果

    virtual void updateBuff() = 0;    //没回合更新buff

    virtual void sendBuff() = 0;

    virtual void getBuffChange(json_spirit::Array& blist) = 0;
    virtual void setBuffChange() = 0;
    virtual void clearBuff() = 0;

    //攻击目标造成的伤害
    virtual int Damages(iGeneral& target) = 0;

    //对目标逃兵的伤害
    virtual int taobingDamage(iGeneral& target) = 0;

    virtual int total_inspired() = 0;
};

#endif

