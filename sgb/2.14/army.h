
#ifndef _ARMY_H_
#define _ARMY_H_

#include <list>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"

class iGeneral;
class Combat;
struct CharData;
struct CharStrongholdData;
struct charCampRace;
class Army;
struct groupCopyArmy;
struct eliteCombat;
struct mazeMonster;
struct corpsFihtingMember;
struct spls_boss;
struct base_ZST_Stronghold;

#define TIME_EVERY_COMBAT 3
#define TIME_PER_ACTION 1

//��������
struct army_data
{
    json_spirit::Array m_weapons;    //����
    json_spirit::Array m_state; //״̬
    json_spirit::Object m_horse; //ս��
    json_spirit::Array m_buff; //��ʱ����
    json_spirit::Array m_jxl;    //����¼�ӳ�

    int m_charactor;       //��ɫid��or ϵͳid
    int m_type;             //��ҽ�ɫ or ϵͳ��
    int m_camp;             //��Ӫ
    std::string m_name;    //��ɫ����
    std::string m_shoutMsg;//��������
    int m_level;
    int m_spic; 

    int m_hp_cost;            //ļ������
    int m_hp_total;
    int m_hp_die;
    int m_hp_max;

    int m_attack_value;    //ս��ֵ(��Ҳ���)
    Army* m_army;

    iGeneral* m_generals[9];//�佫�б�

    army_data();
    ~army_data();
    void setArmy(Army* pArmy);
    //���ؽ�ɫ����    
    int LoadCharactor(int cid);
    //���عؿ�����
    int LoadStronghold(CharStrongholdData& cstronghold, int strongholdId);
    //����boss
    int LoadBoss(spls_boss& boss);
    //������Ӫսһ������
    int LoadCampRace(charCampRace* ccr);
    //���ؾ���սսһ������
    int LoadCorpsFighting(corpsFihtingMember* ccr);
    //���ض��˸���npc��
    int loadGroupCopy(int copyId, int pos, groupCopyArmy* pArmy);
    //���ؾ�Ӣս��
    int loadEliteCombat(int id, eliteCombat* peliteCombat);

    //�����Թ��н�ɫ
    int LoadMazeCharactor(int cid);
    //�����Թ�����
    int loadMazeCombat(int id, mazeMonster* pm);

    int LoadZSTCharactor(int cid);
    int loadZSTCombat(int id, base_ZST_Stronghold* pzst);

    void _update();

    static uint64_t refs();

    static volatile uint64_t _refs;
} ;


class Army
{
public:
    Army(army_data*);
    
    ~Army();
    //ָ��λ�õ��佫
    iGeneral* GetGeneral(int pos);
    //ָ��λ�õ��佫
    iGeneral* GetGeneral2(int pos);
    //���ȱ��ţ�����û�д����һ�ſ�ʼ�����ȴ�ǰ����
    iGeneral* GetRowGeneral(int row);
    //һ�ŵĵ�һ��Ŀ�꣬���ȴ�ǰ����
    iGeneral* GetRowGeneral2(int row);

    //���ȱ��ţ�����û�д����һ�ſ�ʼ�����ȴӺ�ǰ
    iGeneral* GetRowGeneralr(int row);
    //��ȡָ���佫������佫�����û�з����Լ�
    iGeneral* GetBackGeneralr(int pos);
    //һ����佫
    int GetSideGenerals(int pos, std::list<iGeneral*>& glist);
    //һ�ŵ��佫
    int GetRowGenerals(int pos, std::list<iGeneral*>& glist);
    //�ڽ����佫
    int GetAroundGenerals(int pos, std::list<iGeneral*>& glist);
    //pos����̶������ĵ��佫
    int GetSomeGenerals(int pos, int counts, std::list<iGeneral*>& glist);
    //ȫ���佫
    int GetAllGenerals(int pos, std::list<iGeneral*>& glist);
    //�Ƿ����
    bool IsLive();
    //�ֵ��ж��������ж���pos
    int Action();
    //���ûغ�
    int Reset();
    //û�н�������
    bool NoAttack();
    //Ѫ�����ٵ��佫
    iGeneral* GetMinhpGeneral();
    //Ѫ�������佫
    iGeneral* GetMaxhpGeneral();
    //����
    Army* GetEnermy();
    //���ս��
    Combat& GetCombat();
    Combat* GetCombatHandle();

    void setEnermy(Army* e)
    {
        m_enermy = e;
    }
    void setCombat(Combat* c);

    int DieHp();
    int TotalHp();
    int TotalMaxHp();
    std::string Name() {return _army_data->m_name;}
    int getCharId() {return _army_data->m_charactor;}
    int level() const {
        if (_army_data) {
            return _army_data->m_level;
        } 
        else {
            return 1;
        }
    }
    int attack_value() {return _army_data->m_attack_value;}
    int type() {return _army_data->m_type;}
    int GetObj(json_spirit::Object& army_obj, bool brief = false);
    bool isAttacker() {return m_is_attacker;}
    void setAttacker() {m_is_attacker = true;}
    int sendBuff();
    void updateBuff();
    void clearBuff();
    //�������������
    int in_soul_link();

    //���ŵ��佫����
    int getLiveGeneralCount();
    
    int getBuffChange(json_spirit::Array& blist);

    void Calc();
    static uint64_t refs();

    friend class Combat;
    friend class RaceMgr;
    friend class spls_boss;
    friend class bossMgr;
    friend class campRaceMgr;
    friend class guardMgr;
    friend class SaveCombatWorker;
    friend class groupCombat;
    friend class groupCopyCombat;
    friend int ProcessCombatResult(json_spirit::mObject& o);
    friend int ProcessStrongholdCombatResult(Combat* pCombat, boost::shared_ptr<CharData>& cdata);
    friend int createBossCombat(int cid, Combat* pCombat);
private:
    int CheckPush(int pos, std::list<iGeneral*>& glist);

    bool b_init_success;  //��ʼ���ɹ�
    Combat* m_combat;         //ս��
    Army* m_enermy;         //����
    bool m_is_attacker;   //���������Ƿ�����
    int m_action_pos;        //��ǰ�ĸ��佫�ж�

    army_data* _army_data;

    static volatile uint64_t _refs;
};

#endif

