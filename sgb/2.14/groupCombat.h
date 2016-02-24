#pragma once

#include <string>
#include <list>

class Combat;
struct army_data;

const int iMaxGroupCombat = 3;

class groupCombat
{
public:
    groupCombat(int type, int combat_id, int extra_id);
    virtual ~groupCombat();
    int start();
    Combat* createCombat(int pos, army_data* army_a, army_data* army_d);
    Combat* createCombat(int pos);
    Combat* createCombat2(int pos);
    int onCombatResult(Combat* pCombat);
    virtual void broadCastMsg(const std::string& msg);
    void setAttacker(const std::list<army_data*>& a);
    void setDefender(const std::list<army_data*>& a);
    void npcName(const std::string& name) {_npc_name = name;}
    virtual int onEnd(int) = 0;

    friend class groupCopyCombat;
private:
    int _combat_type;
    int _combat_id;
    int _extra_id;
    std::string _npc_name;

    std::list<army_data*> _attackerList;    //����������
    std::list<army_data*> _defenderList;    //���ط�����
    Combat* _combats[iMaxGroupCombat];    //���ڽ��е�ս��
    //std::list<army_data* > _attackerQue;    //�������Ⱥ����
    //std::list<army_data* > _defenderQue;    //���ط��Ⱥ����

std::list<army_data* > _attackerQues[iMaxGroupCombat];    //�������Ⱥ����
std::list<army_data* > _defenderQues[iMaxGroupCombat];    //���ط��Ⱥ����

};

class groupCopyCombat : public groupCombat
{
public:
    groupCopyCombat(int combatid, int copyId, int groupId, const std::list<army_data*>& alist, const std::list<army_data*>& dlist, int* l_bfriend);
    virtual ~groupCopyCombat() {}
    virtual void broadCastMsg(const std::string& msg);
    virtual int onEnd(int);
private:
    int _copyid;
    int _groupid;
    int _combatId;
    int _bfriend[iMaxGroupCopyMembers];    //0��ʾ�������1��ʾ����ĺ������
};

class groupCombatMgr
{
public:
    groupCombatMgr();
    ~groupCombatMgr();

    int combatResult(Combat* pCombat);    //ս������
    //�������˸���
    int AttackGroupCopy(int groupId, int copyId, int* l_bfriend);

    static groupCombatMgr* getInstance();
private:
    static groupCombatMgr* m_handle;
    volatile int m_groupCopyCombatId;
    std::map<int, boost::shared_ptr<groupCopyCombat> > m_groupCopyCombats;
};

