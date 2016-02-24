#pragma once

#include <string>
#include <map>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "utils_all.h"
#include "data.h"
#include "net.h"

struct baseBossData
{
    openRules _open_rules;

    int _open_state;

    int _id;
    int _spic;
    int _star;
    int _race;
    int _attack;
    int _defense;
    int _magic;
    int _max_hp;
    int _cur_hp;
    int _hide_hp;
    int _level;
    int _base_attack;
    int _base_defense;
    int _base_magic;
    int _base_hp;
    int _attack_per_level;
    int _defense_per_level;
    int _magic_per_level;
    int _hp_per_level;

    std::string _name;
    std::string _chat;

    time_t _last_open_time;

    combatAttribute m_combat_attribute;    //ս������

    baseBossData();
};

struct bossRankRewards
{
    int rank;
    std::list<Item> reward;//����
};

struct bossScore
{
    int cid;
    int score;
    std::string name;
};

struct OnlineCharactor;
class ChatChannel;

struct boss_log
{
    std::string m_name;
    int m_damage;
    int m_silver;
};

class Boss
{
public:
    Boss(int id);
    int canAttack(int cid, int& cool);
    void reset();
    int open(int last_mins);
    int close(int killer);
    int combatResult(chessCombat* pCombat);

    int leave(boost::shared_ptr<CharData>& cdata, boost::shared_ptr<OnlineCharactor>&);
    int enter(boost::shared_ptr<CharData>& cdata, boost::shared_ptr<OnlineCharactor>&);
    int broadBossLogEvent();
    int getBossLogEvent(json_spirit::Object& robj);

    baseBossData _boss;
    time_t _start_time;
    time_t _end_time;

    std::string _boss_hp_msg;

    std::map<int, time_t> m_attack_cd_maps;//��ҵĹ�����ȴʱ��
    std::map<int, int> m_damage_maps;//��ҵ�����˺�
    std::map<int, int> m_silver_maps;//��ҵĳ���
    std::map<int, int> m_score_maps;//��ҵĻ���
    std::map<std::pair<int, int>, int> m_prestige_maps;//��ҵ�����
    std::map<int, boost::shared_ptr<CharData> > m_char_list;//boss�����еĽ�ɫ
    std::list<boost::shared_ptr<boss_log> > m_event_log;//�¼���¼

    std::list<bossScore> _topten;
    boost::uuids::uuid _uuid;    //��ʱ��Ψһid
    boost::shared_ptr<ChatChannel> _channel;    //bossսƵ��
};

class bossMgr
{
public:
    bossMgr();
    ~bossMgr();
    int load();
    boost::shared_ptr<Boss> getBoss(int id);
    int getInspire(int cid, json_spirit::Object& robj);        //��ѯ����Ч��
    int inspire(int cid, json_spirit::Object& robj);
    int clearInspire(int cid);        //�������Ч��
    int combatResult(chessCombat* pCombat);    //bossս������
    int getBossHp(int id, json_spirit::Object& robj);
    int openBoss(int id, int last_mins);    //boss����
    int closeBoss(int id);    //boss�ر�
    int getAction(CharData* pc, json_spirit::Array& blist);
    bool isOpen();    //bossս�Ƿ���
    void setLevel(int level);    //��������boss�ȼ�
    int getAwards(int rank, std::list<Item>& loots);
    static bossMgr* getInstance();
private:
    static bossMgr* m_handle;
    std::vector<bossRankRewards> m_rewards;        //��������
    std::map<int, int> m_inspire_map;    //�����б�
    std::map<int, boost::shared_ptr<Boss> > m_boss_maps;
};

//����boss����
int ProcessBossScene(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡBoss��Ϣ
int ProcessGetBossInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡBossʣ��Ѫ��
int ProcessGetBossHp(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//����
int ProcessInspire(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ����Boss��ȴʱ��
int ProcessGetCoolTime(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��������Boss��ȴʱ��
int ProcessEndCoolTime(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ��Boss����
int ProcessGetBossRank(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ȡ������Ϣ
int ProcessGetBossLogEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

