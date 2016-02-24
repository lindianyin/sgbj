
#ifndef _GENERAL_H_
#define _GENERAL_H_

#include "combat_def.h"

#include <string>
#include "igeneral.h"

#include "combat_attr.h"

//战斗初始士气
#define DEFAULT_SHIQI 50

class Army;
//玩家武将数据
struct CharGeneralData;
//关卡武将属性
struct StrongholdGeneralData;
//玩家关卡
struct CharStrongholdData;

struct bossData;

struct char_maze_general;

struct char_zst_general;

//武将类
class General:public iGeneral
{
public:
    General(Army* army, int pos, const CharGeneralData& gdata, int hurted);
    General(Army* army, int pos, const char_maze_general& m_gdata);
    General(Army* army, int pos, const char_zst_general& m_gdata);
    General(Army* army, int pos, const CharStrongholdData& cstronghold, const StrongholdGeneralData& gdata);
    General(Army* army, int pos, const bossData& bd);
    General(Army* army,
        int id,
        int cid,
        int pos,
        const std::string& name,
        int level,
        int tong,
        int str,
        int wisdom,
        int stype,
        int base_type,
        int damage_type,
        int attack_type,
        int gong,
        int pufang,
        int cefang,
        int hp,
        int failure = 0,
        int duoshan = 0,
        int gedang = 0,
        int shipo = 0,
        int baoji = 0,
        int xixue = 0,
        int hunluan = 0,
        int weihe = 0,
        int podan = 0
    );
    virtual ~General() {};
    //攻击地方单个武将
    virtual int Attack(iGeneral& target);
    //被攻击
    virtual int RecieveDamage(int damage, iGeneral& attacker, int attack_type, int injure_type, int attacker_base_type, bool test);

    //行动
    virtual int Action();
    //获得部队
    virtual Army& GetArmy() const;
    virtual void setArmy(Army*);

    //保存
    virtual int Save();
    //返回id
    virtual int Id();

    virtual int getSpic() {return m_spic;}

    virtual int UniqueId() {return m_unique_id;};

    virtual int level() const {return m_level;};

    virtual int baseType() {return m_base_stype;};

    //取属性
    virtual int Gong(int totype);
    virtual int Wufang(int fromtype);
    virtual int CeFang(int fromtype);
    virtual int Str() {return m_str;};
    virtual int Int() {return m_int;};
    virtual int Tongyu() {return m_tongyu;};
    virtual int Hp() {return m_hp_now;};
    virtual int MaxHp() {return m_hp_org;};
    virtual const std::string& GetName() {return m_name;};
    virtual int Shiqi() {return m_shiqi;};
    virtual int Pos() {return m_pos;};
    virtual int GetBaoji() {return m_baoji_flag;};

    //特殊攻击是否触发
    virtual int Weihe(const iGeneral& target);
    virtual int Chaos(const iGeneral& target);
    //是否格挡
    virtual int CheckGedang(const iGeneral& att);
    //是否躲闪
    virtual int CheckDodge(const iGeneral& att);
    //是否识破
    virtual int CheckShipo(const iGeneral& att);
    //是否兵种克制
    virtual bool CheckStypeDamage(int stype);
    //还活着
    virtual bool isLive();
    //加血
    virtual int Addhp(int hp);
    //加士气
    virtual int AddShiqi(int shiqi, bool type = false);

    //无攻击
    virtual bool NoAttack();

    virtual json_spirit::Object& GetObj();

    virtual json_spirit::Object& UpdateObj();

    json_spirit::Object& GetOrgObj();
    void GenOrgObj();
    void GenAttackMsg(int result);

    virtual int resist(int type) const;

    virtual int resist_level(int type) const;

    //受普通伤害和策略伤害的减免
    virtual int subDamage(int attack_type);
    //受来自特定兵种的伤害减免
    virtual int subDamageFrom(int base_stype);

    virtual void addBuff(baseBuff* bb, iGeneral* from, int value, int last);    //增加一个buff

    virtual void removeBuff(boost::shared_ptr<Buff> b);
    
    virtual void addGeneratedBuff(boost::shared_ptr<Buff> b);    //增加一个产生的buff

    virtual void updateGenerateBuff();

    virtual int getBuffValue(int type) const; //获取一类buff的效果

    virtual void updateBuff();    //没回合更新buff

    virtual void clearBuff();

    int HealAll();
    
    int HealOne();

    virtual void sendBuff();
    virtual void getBuffChange(json_spirit::Array& blist);
    virtual void setBuffChange();
    //攻击目标造成的伤害
    virtual int Damages(iGeneral& target);

    //对目标逃兵的伤害
    virtual int taobingDamage(iGeneral& target);

    virtual int total_inspired();
protected:
    //受到伤害
    int RecieveDamage(int damage, int type, int attacker_base_type, bool test = false);
    //对特定兵种加强伤害
    int moreDamageto(int base_type);
    //攻击前判断自身状态
    int CheckState();
    //点攻击
    int AttackArmy_Single();
    //点攻击-优先后排
    int AttackArmy_SingleB();
    //列攻击
    int AttackArmy_Row();
    //面攻击
    int AttackArmy_Side();
    //溅射攻击(邻近攻击)
    int AttackArmy_Round();
    //连环3攻击
    int AttackArmy_ByStep();
    //全体攻击
    int AttackArmy_All();
    //暴击
    int Set_BaojiFlag(const iGeneral& target);
    //攻击是否命中
    int Attack_Result(iGeneral& target);
    //尝试计算攻击的伤害
    int tryAttack(iGeneral& target);
    //设定伤害的攻击
    int Attack(iGeneral& target, int damage, bool special_attack);
    //攻击灵魂锁链
    int AttackSoulLink(Army& amy, const std::list<iGeneral*>& glist, int pos);
    //英雄死亡
    int Die(iGeneral& killer);
    void GenDamageInfo(int damage, int attack_type, int damage_type, bool luan = false, bool weihe = false, bool podan = false, bool baoji = false);
    void GenAttackResultMsg(int result);

    void updateSpecial();

    //治疗和增加怒气的时候是否暴击
    int checkBaoji();

    Army* m_army;       //所在部队

    //是否躲闪
    int m_unique_id;    //武将唯一id
    int m_id;           //武将id
    int m_spic;         //头像id
    int m_charactor;    //所属角色id

    int m_level;        //等级
    int m_color;

    int m_stype;        //兵种
    int m_base_stype;   //基础兵种
    std::string m_sname;//兵种名
    int m_soldier_spic; //兵种图片
    
    int m_damage_type;  // 攻击类别   1、物理攻击 2、策略攻击 3、治疗  4、士气
    int m_attack_type;  // 攻击方式   1、单个目标 2、一列 3、一排 4、目标及周围 5、固定三个目标6、全体
    int m_attack_type2; // 攻击方式   1、近战 2、远程    
    int m_attack_type3; // 攻击方式   1、近战 2、远程    

    int m_real_attack_type;    //实际的攻击方式
    int m_special_attack;        //放绝招中
    int m_special_attack_fac;    //绝技攻击系数
    std::string m_attack_skill;    //转发攻击方式的触发技能

    int m_the_damage_total;      //本次攻击总伤害

    int m_damage_type2;//伤害类别 1被火器打,2被枪刺，3被刀砍,4被箭射,5被魔法忽悠

    int m_nuqi_add;        //普通攻击增加的怒气
    int m_nuqi_add_baoji;    //暴击增加的怒气
    //勇武、智力、统御
    int m_str;
    int m_int;
    int m_tongyu;

    //攻防
    int m_attack;
    int m_wu_fang;
    int m_ce_fang;

    combatAttribute m_combat_attribute;    //战斗属性

    int m_special_attacks[special_attack_max];
    int m_special_resists[special_attack_max];

    boost::shared_ptr<specialSkill> m_speSkill;

    std::list<boost::shared_ptr<Buff> > m_buff_list;

    //由我产生的buff
    std::list<boost::shared_ptr<Buff> > m_generate_buff_list;

    bool m_buff_changed;

    //std::list<combatSpeSkill> m_more_damage_skills;    //加伤害战斗特技

    //std::list<combatSpeSkill> m_attack_skills;            //变化攻击方式的特技
    /*
    int m_duoshan;  //躲避概率
    int m_gedang;   //格挡概率
    int m_shipo;    //识破概率

    int m_baoji;    //暴击
    int m_xixue;    //吸血概率
    int m_hunluan;  //混乱
    int m_weihe;    //威吓
    int m_podan;    //破胆*/

    int m_failure;  //失败概率
    //血量
    int m_hp_org;   //原始血量
    int m_hp_now;   //当前血量

    //士气
    int m_shiqi;    //受伤加的士气
    int m_shiqi_old;//原来的士气

    int m_pos;      //9宫图中位置1-9
    std::string m_name; //武将名
    std::string m_nickname; //绰号名

    //特殊状态
    int m_chaos_flag;   //混乱标记
    int m_weihe_flag;   //威吓标记
    int m_baoji_flag;   //暴击标记

    //增加伤害
    int m_more_damage_fac;        //特技增加的伤害翻倍系数
    std::string m_more_damage_skill;//伤害翻番的触发技能

    int m_type;         // 1 攻击方  2 防守方
    bool b_init_success;    //初始化成功

    json_spirit::Object m_org_obj;  //初始obj
    json_spirit::Object m_cur_obj;  //当前obj

    json_spirit::Object m_action_obj;
    json_spirit::Array m_result_list;
};

#endif

