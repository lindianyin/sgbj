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

    combatAttribute m_combat_attribute;    //战斗属性

    baseBossData();
};

struct bossRankRewards
{
    int rank;
    std::list<Item> reward;//奖励
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

    std::map<int, time_t> m_attack_cd_maps;//玩家的攻击冷却时间
    std::map<int, int> m_damage_maps;//玩家的输出伤害
    std::map<int, int> m_silver_maps;//玩家的筹码
    std::map<int, int> m_score_maps;//玩家的积分
    std::map<std::pair<int, int>, int> m_prestige_maps;//玩家的声望
    std::map<int, boost::shared_ptr<CharData> > m_char_list;//boss场景中的角色
    std::list<boost::shared_ptr<boss_log> > m_event_log;//事件记录

    std::list<bossScore> _topten;
    boost::uuids::uuid _uuid;    //定时器唯一id
    boost::shared_ptr<ChatChannel> _channel;    //boss战频道
};

class bossMgr
{
public:
    bossMgr();
    ~bossMgr();
    int load();
    boost::shared_ptr<Boss> getBoss(int id);
    int getInspire(int cid, json_spirit::Object& robj);        //查询鼓舞效果
    int inspire(int cid, json_spirit::Object& robj);
    int clearInspire(int cid);        //清除鼓舞效果
    int combatResult(chessCombat* pCombat);    //boss战斗结束
    int getBossHp(int id, json_spirit::Object& robj);
    int openBoss(int id, int last_mins);    //boss开启
    int closeBoss(int id);    //boss关闭
    int getAction(CharData* pc, json_spirit::Array& blist);
    bool isOpen();    //boss战是否开启
    void setLevel(int level);    //设置所有boss等级
    int getAwards(int rank, std::list<Item>& loots);
    static bossMgr* getInstance();
private:
    static bossMgr* m_handle;
    std::vector<bossRankRewards> m_rewards;        //排名奖励
    std::map<int, int> m_inspire_map;    //鼓舞列表
    std::map<int, boost::shared_ptr<Boss> > m_boss_maps;
};

//进出boss场景
int ProcessBossScene(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取Boss信息
int ProcessGetBossInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取Boss剩余血量
int ProcessGetBossHp(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//鼓舞
int ProcessInspire(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取攻打Boss冷却时间
int ProcessGetCoolTime(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//结束攻打Boss冷却时间
int ProcessEndCoolTime(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取对Boss排行
int ProcessGetBossRank(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取公告信息
int ProcessGetBossLogEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

