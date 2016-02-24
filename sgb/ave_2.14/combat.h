#ifndef _COMBAT_H_
#define _COMBAT_H_

#include <string>
#include <list>
#include "army.h"

#include "loot.h"

struct charCampRace;

//ս����
class Combat
{
public:
    Combat(Army* attacker, Army* defender);
    ~Combat();

    int CalcResult();
    void AppendResult(json_spirit::Object obj);
    void AppendRoundInfo();

    int type() {return m_type;}
    void restart() { m_fight_secs = 0; }
    uint64_t combat_id() { return m_combat_id; }
    static uint64_t refs();
    json_spirit::Object& getCombatInfo();
    void setCombatInfo();
    void add_time(int t) { m_fight_secs += t; }
    int add_time() { return m_fight_secs; }
    const std::string& getCombatInfoText() {return m_combat_info_text;}
    void set_extra_viewer(const std::string& w) {m_extra_viewer = w; }
    int state() { return m_state; }
    friend int ProcessStrongholdCombatResult(Combat* pCombat, boost::shared_ptr<CharData>& cdata);
    friend int ProcessCombatResult(json_spirit::mObject& o);
    friend int giveLoots(boost::shared_ptr<CharData>& cdata, Combat* pCombat, bool isAttacker, int statistics_type);
    friend int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
    friend class RaceMgr;
    friend class spls_boss;
    friend class bossMgr;
    friend class campRaceMgr;
    friend class guardMgr;
    friend class SaveCombatWorker;
    friend class groupCombat;
    friend class groupCopyCombat;
    friend class groupCombatMgr;
    friend class servantMgr;
    friend class eliteCombatMgr;
    friend class mazeMgr;
    friend class char_maze;
    friend class corpsFightingMgr;
    friend class zstMgr;
    friend class char_zst;
    friend int createBossCombat(int cid, Combat* pCombat);
    friend Combat* createStrongholdCombat(int cid, int strongholdId, int& ret);
    friend Combat* createRaceCombat(int cid, int tid, int& ret);
    friend Combat* createBossCombat(int cid, int tid, int& ret);
    friend Combat* createCampRaceCombat(charCampRace* c1, charCampRace* c2, int& ret);
    friend Combat* createGuardCombat(int cid, int tid, int true_guard_cid, int& ret);
    friend Combat* createTradeCombat(int cid, int tid, int& ret);
    friend Combat* createServantCombat(int cid, int tid, int extra_data, int extra_data2, int& ret);
    friend Combat* createEliteCombat(int cid, int mapid, int eliteid, int& ret);
    friend Combat* createMazeCombat(int cid, int maze_army_id, int extra_data, int& ret, bool isBoss);
    friend Combat* createZSTCombat(int cid, int star, int army_id, int& ret, bool isBoss);
private:
    int SaveDb();
    bool CheckEnd();

    //std::string m_date;    //ս����������
    uint64_t m_combat_id;     //ս��id
    Army* m_attacker;        //������
    Army* m_defender;        //���ط�
    int m_state;            //ս��״̬
    int m_round;            //�غ�
    int m_type;             //ս������  1 �ؿ�ս��
    int m_type_id;             //ս�����Ͷ�Ӧ��id
    int m_result_type;        //����ս�������Ľ������ʤ�Ұܡ���

    //�ؿ�ս�����е��ֶ�
    int m_mapid;        //�ؿ����ڵ�ͼ
    int m_stageId;        //����id
    int m_strongholdId;    //�ؿ�id
    int m_strongholdPos;    //�ؿ�λ��
    int m_stronghold_type;    //�ؿ����� ��ͨ/��Ӣ/boss
    int m_stronghold_level;    //�ؿ��ȼ�
    int m_attack_times;        //�Ѿ�����Ĵ���
    int m_extra_chance;        //���������

    int m_extra_data[2];    //ս����һЩ������Ϣ
    std::string m_extra_viewer;

    int m_mail_to;
    std::string m_mail_to_name;
    std::string m_mail_title;
    std::string m_mail_content;

    int m_position;        //����ս�������ֶΣ�ս��λ�� 1-3

    json_spirit::Object m_combat_info;  //ս��˫������
    std::string m_combat_info_text;

    std::string m_result_text;
    
    std::vector<int> m_levelup_generals;//�������佫
    std::list<Item> m_getItems;          //��õĶ���(������)
    std::list<Item> m_getItems2;         //��õĶ���(���ط�)

    json_spirit::Object m_result_obj;       //ս�����
    json_spirit::Array m_result_array;   //ս�����̽��
    int m_archive_report;

    std::string m_final_result;

    std::string m_failMsg;

    int m_fight_secs;

    static volatile uint64_t _refs;
};

#endif

