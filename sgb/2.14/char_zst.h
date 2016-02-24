#pragma once

#include "data.h"
#include "net.h"
#include "json_spirit.h"
#include "combat.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace net;


struct base_ZST_Stronghold;
struct base_ZST_Stage;
struct base_ZST_Map;

//战神台据点
struct base_ZST_Stronghold
{
    int _id;    //id
    int _star;
    int _stageid;//场景id
    int _mapid;//地图id
    int _pos;
    int _level;        //等级
    int _spic;    //头像
    std::string _name;    //名字
    int needAttack;

    //关卡的抗性
    combatAttribute m_combat_attribute;

    boost::shared_ptr<StrongholdGeneralData> m_generals[9];//防守武将

    int load();
};

//战神台场景
struct base_ZST_Stage
{
    int id;
    int mapid;
    std::string name;
    int spic;
    int needAttack;
    boost::shared_ptr<base_ZST_Map> _baseMap;
    boost::shared_ptr<base_ZST_Stronghold> _baseStrongholds[25];
};

//战神台地图
struct base_ZST_Map
{
    int id;
    std::string name;
    boost::shared_ptr<base_ZST_Stage> stages[10];    //地图中的场景
};

struct CharZSTStageData
{
    int m_cid;
    int m_mapid;
    int m_stageid;
    int m_state;//场景状态0不可攻击1可攻击2已经击败
    int m_result_star;
    boost::shared_ptr<base_ZST_Stage> m_baseStage;
    std::vector<int> m_stronghold_state;        //各关卡状态0可攻击1已经击败
    std::vector<int> m_stronghold_star;    //各关卡星级
    void load();
    void Save();
    int getStar();
};

typedef std::map<int, boost::shared_ptr<CharZSTStageData> > CharZSTMapData;

//战神台中的武将数据
struct char_zst_general
{
    int pos;
    int id;
    int gid;
    int cid;
    int spic;
    int level;
    int color;
    int b_nickname;
    std::string name;

    //勇武、智力、统御
    int m_str;
    int m_int;
    int m_tongyu;

    int m_org_attack;
    int m_org_wu_fang;
    int m_org_ce_fang;

    //攻防
    int m_attack;
    int m_wu_fang;
    int m_ce_fang;

    //原始
    int m_org_hp_max;    //原始满血量
    int m_org_hp_hurt;//原始受伤血量

    combatAttribute m_combat_attribute; //战斗属性

    int m_inspired;
    
    //当前血量
    int m_hp_org;    //原始血量
    int m_hp_hurt;    //受伤血量

    void Save();
};

struct char_zst
{
    std::map<int, boost::shared_ptr<CharZSTMapData> > CharZSTMapsData;//战神台关卡数据
    int m_cid;                //角色id
    int m_total_star;        //总星星
    int m_star_update_time; //星级升级时间
    int m_cur_star_reward;  //当前已领星级奖励id
    int m_cur_map;        //当前挑战地图
    int m_cur_stage;        //当前挑战场景
    std::list<char_zst_general> m_generals;    //队伍里面的武将

    class zstMgr& m_handle;

    char_zst(int cid, zstMgr& z)
    :m_cid(cid)
    ,m_handle(z)
    {
        m_total_star = 0;
        m_star_update_time = 0;
        m_cur_map = 0;
        m_cur_stage = 0;
        m_cur_star_reward = 0;
        m_generals.clear();
    };

    //重置武将
    void reset_generals(int cid);
    int reset_stage(int mapid, int stageid);
    //刷新星级
    int refreshStar(CharData& cdata, int mapid, int stageid, int type, json_spirit::Object& robj);
    int updateTotalStar();
    bool checkFinish(int mapid);
    int challenge(CharData& cdata, int mapid, int stageid, int pos, json_spirit::Object& robj);
    void combatEnd(Combat* pCombat);
    void Clear();
    void Save();
    void SaveGenerals();
    void load();
};

//战神台星级奖励
struct total_star_rewards
{
    int _needstar;
    std::list<Item> _rewards;
    std::string toString(int level);
};

class zstMgr
{
public:
    zstMgr();
    boost::shared_ptr<char_zst> getChar(int cid);
    int queryZstMapInfo(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj);
    int queryZstStageInfo(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj);
    int refreshZstStar(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj);
    int buyZstChallenge(CharData& cdata, json_spirit::Object& robj);
    int getZstStarReward(CharData& cdata, json_spirit::Object& robj);
    int queryZstStarReward(CharData& cdata, json_spirit::Object& robj);
    int getZstMapReward(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj);
    int ZstChallenge(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj);

    int combatResult(Combat* pCombat);

    void getAction(CharData* pc, json_spirit::Array& blist);
    void resetAll();
    int GetStageMemo(int mapid, int stageid, std::string& name);
    int GetMapMemo(int mapid, std::string& name);
    boost::shared_ptr<base_ZST_Stronghold> getBaseZSTStronghold(int id);
    boost::shared_ptr<base_ZST_Stage> getBaseZSTStage(int mapid, int stageid);

private:
    int max_map;
    std::map<int, boost::shared_ptr<char_zst> > m_char_datas;
    std::map<int, boost::shared_ptr<base_ZST_Stronghold> > m_stronghold_data_map;//战神台关卡数据
    boost::shared_ptr<base_ZST_Map> m_base_maps[10];    //基础地图
    std::map<int, boost::shared_ptr<total_star_rewards> > m_total_star_rewards;//星级奖励
};

//查询战神台地图界面 cmd ：queryZstMapInfo
int ProcessQueryZstMapInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询战神台场景界面 cmd ：queryZstStageInfo
int ProcessQueryZstStageInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//刷新战神台星级cmd：refreshZstStar
int ProcessRefreshZstStar(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//购买战神台挑战次数 cmd：buyZstChallenge
int ProcessBuyZstChallenge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//领取战神台星级奖励 cmd：getZstStarReward
int ProcessGetZstStarReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询战神台星级奖励 cmd：queryZstStarReward
int ProcessQueryZstStarReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//领取战神台地图奖励 cmd：getZstMapReward
int ProcessGetZstMapReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//挑战战神台 cmd：ZstChallenge
int ProcessZstChallenge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

