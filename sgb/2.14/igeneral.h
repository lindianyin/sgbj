
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
    //�����ط������佫
    virtual int Attack(iGeneral& target) = 0;
    //������
    virtual int RecieveDamage(int damage, iGeneral& attacker, int attack_type, int injure_type, int attacker_base_type, bool test) = 0;

    //�ж�
    virtual int Action() = 0;
    //��ò���
    virtual Army& GetArmy() const = 0;
    virtual void setArmy(Army*) = 0;

    //����
    virtual int Save() = 0;
    //����id
    virtual int Id() = 0;

    virtual int UniqueId() = 0;

    virtual int level() const = 0;

    virtual const std::string& GetName() = 0;

    virtual int baseType() = 0;

    //����ͨ�˺��Ͳ����˺��ļ���
    virtual int subDamage(int attack_type) = 0;
    //�������ض����ֵ��˺�����
    virtual int subDamageFrom(int base_stype) = 0;

    //ȡ����
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

    //���⹥���Ƿ񴥷�
    virtual int Weihe(const iGeneral& target) = 0;
    virtual int Chaos(const iGeneral& target) = 0;

    //����
    virtual int resist(int type) const = 0;

    virtual int resist_level(int type) const = 0;
    
    //�Ƿ��
    virtual int CheckGedang(const iGeneral& att) = 0;
    //�Ƿ����
    virtual int CheckDodge(const iGeneral& att) = 0;
    //�Ƿ�ʶ��
    virtual int CheckShipo(const iGeneral& att) = 0;
    //�Ƿ���ֿ���
    virtual bool CheckStypeDamage(int stype) = 0;
    //������
    virtual bool isLive() = 0;
    //��Ѫ
    virtual int Addhp(int hp) = 0;
    //��ʿ��
    virtual int AddShiqi(int shiqi, bool type = false) = 0;
    //�޹���
    virtual bool NoAttack() = 0;

    virtual json_spirit::Object& GetObj() = 0;

    virtual json_spirit::Object& UpdateObj() = 0;

    virtual void GenOrgObj() = 0;

    virtual json_spirit::Object& GetOrgObj() = 0;

    virtual void GenAttackResultMsg(int result) = 0;

    virtual void addBuff(baseBuff* bb, iGeneral* from, int value, int last) = 0;    //����һ��buff

    virtual void addGeneratedBuff(boost::shared_ptr<Buff> b) = 0;    //����һ��������buff

    virtual void removeBuff(boost::shared_ptr<Buff> b) = 0;

    virtual void updateGenerateBuff() = 0;

    virtual int getBuffValue(int type) const = 0;    //��ȡһ��buff��Ч��

    virtual void updateBuff() = 0;    //û�غϸ���buff

    virtual void sendBuff() = 0;

    virtual void getBuffChange(json_spirit::Array& blist) = 0;
    virtual void setBuffChange() = 0;
    virtual void clearBuff() = 0;

    //����Ŀ����ɵ��˺�
    virtual int Damages(iGeneral& target) = 0;

    //��Ŀ���ӱ����˺�
    virtual int taobingDamage(iGeneral& target) = 0;

    virtual int total_inspired() = 0;
};

#endif

