
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

//部队属性
struct army_data
{
    json_spirit::Array m_weapons;    //武器
    json_spirit::Array m_state; //状态
    json_spirit::Object m_horse; //战马
    json_spirit::Array m_buff; //限时增益
    json_spirit::Array m_jxl;    //将星录加成

    int m_charactor;       //角色id，or 系统id
    int m_type;             //玩家角色 or 系统方
    int m_camp;             //阵营
    std::string m_name;    //角色名字
    std::string m_shoutMsg;//喊话内容
    int m_level;
    int m_spic; 

    int m_hp_cost;            //募兵消耗
    int m_hp_total;
    int m_hp_die;
    int m_hp_max;

    int m_attack_value;    //战力值(玩家才有)
    Army* m_army;

    iGeneral* m_generals[9];//武将列表

    army_data();
    ~army_data();
    void setArmy(Army* pArmy);
    //加载角色部队    
    int LoadCharactor(int cid);
    //加载关卡部队
    int LoadStronghold(CharStrongholdData& cstronghold, int strongholdId);
    //加载boss
    int LoadBoss(spls_boss& boss);
    //加载阵营战一方数据
    int LoadCampRace(charCampRace* ccr);
    //加载军团战战一方数据
    int LoadCorpsFighting(corpsFihtingMember* ccr);
    //加载多人副本npc方
    int loadGroupCopy(int copyId, int pos, groupCopyArmy* pArmy);
    //加载精英战役
    int loadEliteCombat(int id, eliteCombat* peliteCombat);

    //加载迷宫中角色
    int LoadMazeCharactor(int cid);
    //加载迷宫怪物
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
    //指定位置的武将
    iGeneral* GetGeneral(int pos);
    //指定位置的武将
    iGeneral* GetGeneral2(int pos);
    //优先本排，本排没有从最近一排开始，优先从前到后
    iGeneral* GetRowGeneral(int row);
    //一排的第一个目标，优先从前到后
    iGeneral* GetRowGeneral2(int row);

    //优先本排，本排没有从最近一排开始，优先从后到前
    iGeneral* GetRowGeneralr(int row);
    //获取指定武将后面的武将，如果没有返回自己
    iGeneral* GetBackGeneralr(int pos);
    //一面的武将
    int GetSideGenerals(int pos, std::list<iGeneral*>& glist);
    //一排的武将
    int GetRowGenerals(int pos, std::list<iGeneral*>& glist);
    //邻近的武将
    int GetAroundGenerals(int pos, std::list<iGeneral*>& glist);
    //pos后面固定个数的的武将
    int GetSomeGenerals(int pos, int counts, std::list<iGeneral*>& glist);
    //全体武将
    int GetAllGenerals(int pos, std::list<iGeneral*>& glist);
    //是否活着
    bool IsLive();
    //轮到行动，返回行动者pos
    int Action();
    //重置回合
    int Reset();
    //没有进攻能力
    bool NoAttack();
    //血量最少的武将
    iGeneral* GetMinhpGeneral();
    //血量最多的武将
    iGeneral* GetMaxhpGeneral();
    //敌人
    Army* GetEnermy();
    //获得战斗
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
    //灵魂锁链保护中
    int in_soul_link();

    //活着的武将数量
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

    bool b_init_success;  //初始化成功
    Combat* m_combat;         //战斗
    Army* m_enermy;         //敌人
    bool m_is_attacker;   //攻击方还是防御方
    int m_action_pos;        //当前哪个武将行动

    army_data* _army_data;

    static volatile uint64_t _refs;
};

#endif

