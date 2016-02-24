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

const int iBossDamageRankingsPage = 3;        //���а�3ҳ
const int iBossDamageRankingsPerPage = 8;    //ÿҳ8��
const int iBossDamageRankingsCount = iBossDamageRankingsPage*iBossDamageRankingsPerPage;

const int iCorpsBossId = 100;

//����boss��������ʱ��
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

    combatAttribute m_combat_attribute;    //ս������

    boost::shared_ptr<specialSkill> m_speSkill;

    bossData();
};

class combatBoss : public General
{
public:
    combatBoss(Army* army, int pos, const bossData& bd);
    //�����ط������佫
    //virtual int Attack(iGeneral& target);
    //�ж�
    //virtual int Action();
    //ʿ��(boss ��ʿ��)
    //virtual int Shiqi() {return 0;}
    //������
    virtual int RecieveDamage(int damage, iGeneral& attacker, int attack_type, int injure_type, int attacker_base_type, bool test);
    //����ʿ��
    //virtual int AddShiqi(int , bool ) {return 0;}
    //����Ŀ����ɵ��˺�
    virtual int Damages(iGeneral& target);

    //��ʿ��
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

    void _tryAttack(int cid, int type);                 //��ҹ���boss
    int tryAttack(int cid, int type, json_spirit::Object& robj);                    //��ҹ���boss
    
    int canAttack(int cid, int& cool);
    int reset();

    int levelup();
    int open(int);    //boss����
    int close(int type, int killer, int silver_get);//boss�ر�
    int combatResult(Combat* pCombat);    //bossս������

    int leave(boost::shared_ptr<CharData>& cdata, boost::shared_ptr<OnlineCharactor>&);//��ɫ���븱��
    int enter(boost::shared_ptr<CharData>& cdata, boost::shared_ptr<OnlineCharactor>&);//��ɫ�뿪����
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

    std::string _report;    //boss���Y�ĽyӋ���

    int _attacker;    //��ǰ������id
    int _type;
    
    std::list<boss_attacker> m_attack_list;     //��������

    std::map<int, time_t> m_attack_cd_maps;     //��ҵĹ�����ȴʱ��
    std::map<int, int> m_attack_end_cd_times;  //��ҵ���������
    std::map<int, time_t> m_attack_min_cd_maps;//��ҵĹ�����ȴʱ��
    std::map<int, int> m_damage_maps;           //��ҵ�����˺�
    std::map<int, int> m_max_damage_maps;       //�����߂���
    std::map<int, time_t> m_char_enter;         //�����еĽ�ɫ����ʱ��

    std::map<int, boost::shared_ptr<CharData> > m_char_list;            //�����еĽ�ɫ
    
    std::map<int, int> m_silver_gets;          //�ۼƻ������
    std::map<int, int> m_prestige_gets;        //�ۼƻ������

    std::list<bossDamage> _topten_damages;

    boost::uuids::uuid _uuid;    //��ʱ��Ψһid

    boost::shared_ptr<ChatChannel> _channel;    //bossսƵ��

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

    //�������а��е�cid�ֶ�
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
    int getInspire(int cid, int level, json_spirit::Object& robj);        //��ѯ����Ч��
    int inspire(int cid, int type, json_spirit::Object& robj);    //���� type=1���ҹ��裬type=2 ��ҹ���
    int clearInspire(int cid);        //�������Ч��
    int clearCD(int cid, int bossId);                //����� CD
    int getToptenDamage(int cid, int bossId);    //ǰ10���
    int combatResult(Combat* pCombat);    //bossս������
    int Attack(int cid, int id, int type, json_spirit::Object& robj);
    //�򿪸�������
    int openCopy(int cid, int id, json_spirit::Object& robj);
    int getToptenDamage(int cid, int id, json_spirit::Object& robj);
    int getBossHp(int cid, int id, json_spirit::Object& robj);
    int openBoss(int id, int last_mins);    //boss����
    int closeBoss(int id, int corps = 0);    //boss�ر�
    int getBossList(CharData* pc, json_spirit::Array& blist);
    int getActionMemo(int type, int id, json_spirit::Object& robj);

    bool isOpen();    //bossս�Ƿ���

    void setLevel(int level);    //��������boss�ȼ�

    void updateRankings(std::map<int, int>&, const std::string&);
    //�������а��е�cid�ֶ�
    void updateRankingsEvent(rankings_event* pE);

    std::string getRankings(int page = 1);

    //ɾ����ɫ
    int deleteChar(int cid);

    static bossMgr* getInstance();

    //������boss����
    void checkCorpsBossOpen(int,int);
    //���þ���boss����ʱ��
    int setBossOpenTime(int corps, int sel);
    //���Ӿ���boss(������3��)
    int addCorpsBoss(int corps, int time_idx);
    //��ѯ����boss����ʱ��
    int queryCorpsBossOpen(int corps, json_spirit::Object& robj);
    //����boss����״̬
    int getJtBossState(int corps);

    void resetJtBoss(int corps);
    //stand_in_mob m_stand_in_mob;

private:
    static bossMgr* m_handle;
    
    std::map<int, int> m_inspire_map;    //�����б�

    std::map<int, boost::shared_ptr<spls_boss> > m_boss_maps;

    corpsBoss m_base_corps_boss;
    
    //����boss
    std::map<int, boost::shared_ptr<spls_boss> > m_corps_boss;

    //��������
    bossDamageRankings m_damage_rankings;
};

