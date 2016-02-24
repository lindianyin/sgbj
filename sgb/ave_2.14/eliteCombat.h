#pragma once

#include "data.h"
#include "net.h"
#include "json_spirit.h"
#include "combat.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace net;

//��VIP�����ô���
const int iEliteRestTimes[]={0,0,0,1,1,1,1,1,1,1,1,1,1};
//���û���
#ifdef JP_SERVER
const int iEliteRestGold = 100;
#else
const int iEliteRestGold = 200;
#endif

struct eliteCombat;

enum eliteCombatState
{
    elite_lock = 1,
    elite_active = 2,
    elite_win_little = 3,
    elite_win_normal = 4,
    elite_win_perfect = 5
};

//��Ҿ�Ӣս������
struct CharEliteCombatData
{
    boost::shared_ptr<eliteCombat> m_baseEliteCombat;

    int m_state;//״̬eliteCombatState
    int m_cid;            //��ɫid
    int m_eliteid;
    int m_result;//��������
    int save();
};

//��Ҿ�Ӣս������
struct CharMapEliteCombatData
{
    int m_mapid;        //��ͼid
    int m_cid;            //��ɫid
    int m_reset_time;    //���ô���
    std::list<boost::shared_ptr<CharEliteCombatData> > m_char_eliteCombat_list;//��Ӣս�۶���
    int init();
    int load();
};

struct strongholdRaiders;

//��Ӣս��
struct eliteCombat
{
    int _id;    //ս��id
    int _mapid;//��ͼid
    int _level;        //�ȼ�
    int _spic;    //ͷ��
    int _color;     //��ɫ
    int _open_stronghold;//�����ؿ�id
    std::string _name;    //����
    std::list<Item> m_Item_list;
    int supply;
    int gongxun;

    //�ؿ��Ŀ���
    combatAttribute m_combat_attribute;

    boost::shared_ptr<StrongholdGeneralData> m_generals[9];//�����佫

    strongholdRaiders m_raiders;
    int load();
    int getAttack();
};

typedef std::list<boost::shared_ptr<eliteCombat> > m_eliteCombat_list;//��Ӣս�۶���
typedef std::map<int, boost::shared_ptr<CharMapEliteCombatData> > m_char_map_eliteCombat;//��Ҹ���ͼ��Ӣս������

//��Ӣս�۹���
class eliteCombatMgr
{
public:
    boost::shared_ptr<eliteCombat> getEliteCombat(int mapid, int eliteid);
    boost::shared_ptr<eliteCombat> getEliteCombatById(int eliteid);
    boost::shared_ptr<m_eliteCombat_list> getEliteCombats(int mapid);
    boost::shared_ptr<CharEliteCombatData> getCharEliteCombat(int cid, int mapid, int eliteid);
    boost::shared_ptr<CharMapEliteCombatData> getCharEliteCombats(int cid, int mapid);
    int getCharEliteList(int cid, int mapid, json_spirit::Object& robj);
    int ResetEliteCombat(int cid, int mapid, bool auto_reset);
    int AttackElite(session_ptr & psession,int cid,int mapid,int eliteid);
    //��Ӣ�ؿ��Ƿ�ȫͨ
    bool isCharElitePassed(int cid, int mapid);
    
    int combatResult(Combat* pCombat);
    bool check_stronghold_can_sweep(int cid, int mapid, int eliteid);
    bool check_stronghold_can_attack(int cid, int mapid, int eliteid);
    int load();
    //ÿ�����5������ÿ���˹�������
    void reset();
    static eliteCombatMgr* getInstance();
private:
    static eliteCombatMgr* m_handle;
    std::map<int, boost::shared_ptr<m_eliteCombat_list> > m_eliteCombats;    //ȫ����ͼ��Ӣ����

    std::vector<boost::shared_ptr<eliteCombat> > m_eliteCombats2;

    std::map<int, boost::shared_ptr<m_char_map_eliteCombat> > m_char_eliteCombats;    //ȫ����ҵ�ͼ��Ӣ����
};

//���ݵ�ǰ���������ѯ��Ӣս���б�
int ProcessGetEliteCombatList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����
int ProcessAttackEliteCombat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//���þ�Ӣս��
int ProcessResetEliteCombat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʾ��ҵ�����Ӣ�ؿ���Ϣ
int ProcessEliteStronghold(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʾ��Ӣ�ؿ�����
int ProcessEliteRaiders(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//ѡ��ɨ��
//int ProcessAddSweepElite(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ʼɨ��
//int ProcessSweepElite(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡɨ����Ϣ
//��ȡɨ�����

