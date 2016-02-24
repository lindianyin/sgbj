#pragma once

#include <stdint.h>
#include <map>
#include "net.h"

struct CharData;
struct jxl_combo;
struct jxl_buff_attr;
struct combatAttribute;

//����¼buff���� 1���� 2���� 3���� 4��Ч
const int iJxlBuffTotalType = 4;

struct jxl_buff_attr
{
    int hp;
    int attack;
    int fang;

    int hp_percent;
    int attack_percent;
    int fang_percent;

    int hit;
    int crit;
    int shipo;
    int parry;
    int resist_hit;
    int resist_crit;
    int resist_shipo;
    int resist_parry;

    //�ٷֱ����ӵ�ս��
    int get_attack(int attack, int hp, int wufang, int cefang);

    //�ٷֱ����ӵ�ս��-����Ϊÿ�������佫�Ĺ�����Ѫ�����շ����߷�
    void get_add(int attack_, int hp_,
                 int wufang_, int cefang_,
                 int& attack_add, int& hp_add,
                 int& wufang_add, int& cefang_add);

    //������Ч
    void add_special(combatAttribute& cb);

    void clear()
    {
        hp = 0;
        attack = 0;
        fang = 0;
        hp_percent = 0;
        attack_percent = 0;
        fang_percent = 0;

        hit = 0;
        crit = 0;
        shipo = 0;
        parry = 0;
        resist_hit = 0;
        resist_crit = 0;
        resist_shipo = 0;
        resist_parry = 0;
    }
    jxl_buff_attr()
    {
        clear();
    }

    void dump();
};

struct jxl_combo
{
    int id;
    int type;
    std::string name;
    std::string memo;
    std::list<int> glist;
    json_spirit::Object obj;
    jxl_buff_attr attr;
};

struct char_jxl_buff
{
    CharData& cdata;
    boost::shared_ptr<jxl_combo> used_buff[iJxlBuffTotalType];
    jxl_buff_attr total_buff_attr;
    void update();
    void save();
    void load();
    char_jxl_buff(CharData& c);
    void getInfo(json_spirit::Array& list);
};

class jxl_mgr
{
public:    
    jxl_mgr();
    boost::shared_ptr<jxl_combo> getCombo(int id);
    int queryJxl(CharData& cdata, json_spirit::Object& robj);
    int queryJxlDetail(int id, CharData& cdata, json_spirit::Object& robj);
    void getAction(CharData& cdata, json_spirit::Array& elist);
    int checkActivation(CharData& cdata, int gid);
    bool needNotifyOffical(int offical);
    bool needNotifyStronghold(int stronghold);
private:
    std::vector<boost::shared_ptr<jxl_combo> > m_jxl_combo;

    std::map<int,int> m_jxl_officals;   //Ӱ�콫��¼�Ĺ�ְ
    std::map<int,int> m_jxl_stronghols; //Ӱ�콫��¼�Ĺؿ�
};

//��ѯ����¼
int ProcessQueryJxl(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//���ý���¼�Ƿ�
int ProcessApplyJxl(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//��ѯ����¼��ϸ
int ProcessQueryJxlDetail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

