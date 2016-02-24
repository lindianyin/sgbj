#pragma once

#include "general.h"
#include <string>
#include <map>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "combat.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "utils_all.h"
#include "data.h"

const int iBossDamageRankingsPage = 3;        //排行榜3页
const int iBossDamageRankingsPerPage = 8;    //每页8个
const int iBossDamageRankingsCount = iBossDamageRankingsPage*iBossDamageRankingsPerPage;

const int iCorpsBossId = 100;

//军团boss开启允许时间
const int iCorpsBossOpenTime[][2] =
{
    {8,0},
    {9,0},
    {13,0},
    {14,0},
    {19,0},
    {20,0}
};

const int iCorpsBossOpenTimeSize = sizeof(iCorpsBossOpenTime)/(2*sizeof(int));

struct bossData
{
    openRules _open_rules;
    openRule* _last_rule;

    int _open_state;
    int _spic;
    int _soldier_spic;
    int _season;
    int _hour;
    int _minute;
    int _last_mins;

    int _pufang;
    int _cefang;
    int _str;
    int _wisdom;
    int _base_hp;
    int _base_damage1;
    int _base_damage2;
    int _max_hp;
    int _cur_hp;
    int _hide_hp;
    int _damage1;
    int _damage2;
    int _level;
    int _damage_per_level;
    int _hp_per_level;
    int _state;
    int _type;
    int _id;
    int _stype;

    int _max_silver;
    int _max_prestige;
    
    char _name[50];
    char _chat[200];

    time_t _last_open_time;

    combatAttribute m_combat_attribute;    //战斗属性

    boost::shared_ptr<specialSkill> m_speSkill;

    bossData();
};

class combatBoss : public General
{
public:
    combatBoss(Army* army, int pos, const bossData& bd);
    //攻击地方单个武将
    //virtual int Attack(iGeneral& target);
    //行动
    //virtual int Action();
    //士气(boss 无士气)
    //virtual int Shiqi() {return 0;}
    //被攻击
    virtual int RecieveDamage(int damage, iGeneral& attacker, int attack_type, int injure_type, int attacker_base_type, bool test);
    //增加士气
    //virtual int AddShiqi(int , bool ) {return 0;}
    //攻击目标造成的伤害
    virtual int Damages(iGeneral& target);

    //加士气
    virtual int AddShiqi(int shiqi, bool type = false);
private:
    bossData _data;
};

struct bossDamage
{
    int cid;
    int damage;
    double percent;
    std::string name;
};

struct boss_attacker
{
    int cid;
    int type;
};

struct OnlineCharactor;
class ChatChannel;
struct corpsBoss;

class spls_boss
{
public:
    spls_boss(int id);

    void _tryAttack(int cid, int type);                 //玩家攻击boss
    int tryAttack(int cid, int type, json_spirit::Object& robj);                    //玩家攻击boss
    
    int canAttack(int cid, int& cool);
    int reset();

    int levelup();
    int open(int);    //boss开启
    int close(int type, int killer, int silver_get);//boss关闭
    int combatResult(Combat* pCombat);    //boss战斗结束

    int leave(boost::shared_ptr<CharData>& cdata, boost::shared_ptr<OnlineCharactor>&);//角色进入副本
    int enter(boost::shared_ptr<CharData>& cdata, boost::shared_ptr<OnlineCharactor>&);//角色离开副本
    int total();

    int update(corpsBoss& base, int level);

    friend class bossMgr;
    friend class Army;
//private:
    int _corps;

    bossData _boss;
    Combat* _pCombat;
    time_t _start_time;
    time_t _end_time;

    std::string _boss_hp_msg;
    std::string _boss_person_msg;
    std::string _boss_will_open;

    std::string _boss_open_time;

    std::string _report;    //boss戰鬥的統計報告

    int _attacker;    //当前攻击者id
    int _type;
    
    std::list<boss_attacker> m_attack_list;     //攻击队列

    std::map<int, time_t> m_attack_cd_maps;     //玩家的攻击冷却时间
    std::map<int, int> m_attack_end_cd_times;  //玩家的涅槃次数
    std::map<int, time_t> m_attack_min_cd_maps;//玩家的攻击冷却时间
    std::map<int, int> m_damage_maps;           //玩家的输出伤害
    std::map<int, int> m_max_damage_maps;       //玩家最高傷害
    std::map<int, time_t> m_char_enter;         //副本中的角色进入时间

    std::map<int, boost::shared_ptr<CharData> > m_char_list;            //副本中的角色
    
    std::map<int, int> m_silver_gets;          //累计获得银币
    std::map<int, int> m_prestige_gets;        //累计获得声望

    std::list<bossDamage> _topten_damages;

    boost::uuids::uuid _uuid;    //定时器唯一id

    boost::shared_ptr<ChatChannel> _channel;    //boss战频道

    int _corps_hour;
    int _corps_min;
    int _corps_state;

    stand_in_mob m_stand_in_mob;
};

struct bossDamageRanking
{
    int cid;
    int damage;
};

struct rankings_event;

struct bossDamageRankings
{
    int size;
    int min_damage;
    std::map<int, int> damageMap;
    std::list<bossDamageRanking> damageList;

    int rankings[iBossDamageRankingsCount];        //cid

    std::string strRangkingsPages[iBossDamageRankingsPage];
    json_spirit::Array aRankingsPages[iBossDamageRankingsPage];

    void updateRankings(std::map<int, int>&, const std::string&);

    //更新排行榜活动中的cid字段
    void updateRankingsEvent(rankings_event* pE);

    std::string getRankings(int page = 1);
};

struct corpsBoss
{
    int id;
    int spic;
    int soldier_spic;
    int base_hp;
    int base_damage1;
    int base_damage2;
    int hp_every_level;
    int damage_every_level;
    int pufang;
    int cefang;
    int str;
    int wisdom;
    std::string name;
    std::string chat;
};


class bossMgr
{
public:
    bossMgr();
    ~bossMgr();
    int load();
    boost::shared_ptr<spls_boss> getBoss(int id, int cid);
    int getInspire(int cid, int level, json_spirit::Object& robj);        //查询鼓舞效果
    int inspire(int cid, int type, json_spirit::Object& robj);    //鼓舞 type=1银币鼓舞，type=2 金币鼓舞
    int clearInspire(int cid);        //清除鼓舞效果
    int clearCD(int cid, int bossId);                //金币秒 CD
    int getToptenDamage(int cid, int bossId);    //前10输出
    int combatResult(Combat* pCombat);    //boss战斗结束
    int Attack(int cid, int id, int type, json_spirit::Object& robj);
    //打开副本界面
    int openCopy(int cid, int id, json_spirit::Object& robj);
    int getToptenDamage(int cid, int id, json_spirit::Object& robj);
    int getBossHp(int cid, int id, json_spirit::Object& robj);
    int openBoss(int id, int last_mins);    //boss开启
    int closeBoss(int id, int corps = 0);    //boss关闭
    int getBossList(CharData* pc, json_spirit::Array& blist);
    int getActionMemo(int type, int id, json_spirit::Object& robj);

    bool isOpen();    //boss战是否开启

    void setLevel(int level);    //设置所有boss等级

    void updateRankings(std::map<int, int>&, const std::string&);
    //更新排行榜活动中的cid字段
    void updateRankingsEvent(rankings_event* pE);

    std::string getRankings(int page = 1);

    //删除角色
    int deleteChar(int cid);

    static bossMgr* getInstance();

    //检查军团boss开放
    void checkCorpsBossOpen(int,int);
    //设置军团boss开启时间
    int setBossOpenTime(int corps, int sel);
    //增加军团boss(升级到3级)
    int addCorpsBoss(int corps, int time_idx);
    //查询军团boss开启时间
    int queryCorpsBossOpen(int corps, json_spirit::Object& robj);
    //军团boss开启状态
    int getJtBossState(int corps);

    void resetJtBoss(int corps);
    //stand_in_mob m_stand_in_mob;

private:
    static bossMgr* m_handle;
    
    std::map<int, int> m_inspire_map;    //鼓舞列表

    std::map<int, boost::shared_ptr<spls_boss> > m_boss_maps;

    corpsBoss m_base_corps_boss;
    
    //军团boss
    std::map<int, boost::shared_ptr<spls_boss> > m_corps_boss;

    //傷害排名
    bossDamageRankings m_damage_rankings;
};

