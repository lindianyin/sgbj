#pragma once

 #include <string>
#include <map>
#include <list>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <time.h>
#include "json_spirit.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "utils_all.h"
#include "spls_const.h"
#include "data.h"

class ChatChannel;
class CharData;
struct CharGeneralData;
class Combat;
struct OnlineCharactor;
struct corpsFighting;

enum CORPS_FIGHTING_SIGNUP_STATE
{
    CORPS_FIGHTING_SIGNUP_STATE_NOT_SIGN = 0,
    CORPS_FIGHTING_SIGNUP_STATE_SIGNED = 1,
    CORPS_FIGHTING_SIGNUP_STATE_NOT_SIGNED_START = 2,
    CORPS_FIGHTING_SIGNUP_STATE_SIGNED_START = 3,
    CORPS_FIGHTING_SIGNUP_STATE_END = 4
};

struct corpsFightingRecord
{
    int combat_id;
    int attacker;
    int defender;
    
};

struct corpsFihtingMember
{
    corpsFihtingMember(corpsFighting& c);

    void setDie(int);
    void setHp(Army& amy);
    int _cid;    //角色id
    int _wins;   //胜利次数
    int _skips;  //跳过次数
    int _lose_round;   //在拿轮被击败
    corpsFighting& _corps;

    boost::shared_ptr<CharData> _cdata;

    int _zhen_type;
    boost::shared_ptr<CharGeneralData> m_generals[9];      //各位置的武将
    int m_generals_hp[9];       //各位置的武将的受伤血量 -1表示死亡
    int m_hp_percent;

    json_spirit::Array m_records;
    void addRecord(int type, const std::string& name, int wins);
};

struct corpsFightingResult
{
    int combat_id;
    boost::shared_ptr<corpsFihtingMember> attacker;
    boost::shared_ptr<corpsFihtingMember> defender;
    int result;

    std::string report;
};

struct corpsFihtingRound
{
    int round;
    std::map<int, boost::shared_ptr<corpsFightingResult> > m_results;
    int m_recv_results;
};

struct corpsFightingCorpsRound
{
    int round;
    std::vector<boost::shared_ptr<corpsFightingResult> > m_reports;
    void add(boost::shared_ptr<corpsFightingResult> );
};

struct corpsFighting
{
    corpsFighting();
    //参加的
    std::list<boost::shared_ptr<corpsFihtingMember> > members;
    //活着的
    std::vector<boost::shared_ptr<corpsFihtingMember> > alives;

    //未匹配的队列
    std::vector<boost::shared_ptr<corpsFihtingMember> > unmatch;

    std::vector<boost::shared_ptr<corpsFightingCorpsRound> > m_round_reports;
    boost::shared_ptr<corpsFightingCorpsRound> m_cur_reports;

    boost::shared_ptr<corpsFihtingMember> getMatch();

    void addUnMatch(boost::shared_ptr<corpsFihtingMember> m);
    int getUnMatchCount();

    void removeAlive(int id);

    void processSkips();

    int id;
    int rank;
    int lose_round;
    int lastround_alive;

    std::string name;
};

struct corpsFightingSignup
{
    int corps;
    std::map<int, time_t> signups;
};

struct JtzAwards
{
    int rank;
    std::list<Item> awards;
};

class corpsFightingMgr
{
public:
    corpsFightingMgr();
    void roundMatch();
    int combatResult(Combat* pCombat);    //战斗结束
    int signUp(int corps, int cid, time_t signupTime = 0);
    //取消报名
    int cancelSignUp(int corps, int cid);

    int getSignupState(int corps, int cid);
    void start();
    void openSignup();

    int getJtzInfo(boost::shared_ptr<OnlineCharactor>& ou, int corps, int cid, json_spirit::Object& robj);
    int getJtzCombat(net::session_ptr& sp, int corps, int round, int r2, json_spirit::Object& robj);
    int leaveJtz(int cid);
    int getMyJtz(int corps, int cid, json_spirit::Object& robj);
    void setCorpsFightingRank(corpsFighting& cf, int rank);

private:
    Combat* createCombat(corpsFihtingMember* r1, corpsFihtingMember* r2);
    void onRoundEnd();

//报名数据
    std::map<int, corpsFightingSignup> m_signup_data;

    std::map<int, boost::shared_ptr<corpsFighting> > m_corps_map;
    std::list<boost::shared_ptr<corpsFighting> > m_alive_corps;

    int m_state;    //状态 0未开始，1正在进行  2已经结束
    int m_round_state;  //0未结束，1结束
    int m_round;
    int m_next_rank;
    time_t m_next_time;

    bool m_round_start;

    boost::shared_ptr<corpsFihtingRound> m_cur_round;
    std::vector<boost::shared_ptr<corpsFihtingRound> > m_rounds_data;
    
    boost::shared_ptr<ChatChannel> _channel;    //军团战频道
    std::map<int,int> m_char_map;

    //军团战排名奖励
    std::vector<JtzAwards> m_awards;
};

//报名参加军团战
int ProcessSignupJtz(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//查询军团战信息
int ProcessQueryJtzInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//查询军团战战报
int ProcessQueryJtzCombat(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//查询军团战我的战报
int ProcessQueryMyJtz(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//离开军团战界面
int ProcessLeaveJtz(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//下一轮匹配
int ProcessJtzMatch(json_spirit::mObject& o);

//军团战开放报名
int ProcessJtzOpenSignup(json_spirit::mObject& o);

//开启军团战
int ProcessStartJtz(json_spirit::mObject& o);

