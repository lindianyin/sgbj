#pragma once

#include "data.h"
#include "net.h"
#include "json_spirit.h"
#include "combat.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace net;

//各VIP可重置次数
const int iEliteRestTimes[]={0,0,0,1,1,1,1,1,1,1,1,1,1};
//重置花费
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

//玩家精英战役数据
struct CharEliteCombatData
{
    boost::shared_ptr<eliteCombat> m_baseEliteCombat;

    int m_state;//状态eliteCombatState
    int m_cid;            //角色id
    int m_eliteid;
    int m_result;//闯关评价
    int save();
};

//玩家精英战役数据
struct CharMapEliteCombatData
{
    int m_mapid;        //地图id
    int m_cid;            //角色id
    int m_reset_time;    //重置次数
    std::list<boost::shared_ptr<CharEliteCombatData> > m_char_eliteCombat_list;//精英战役队列
    int init();
    int load();
};

struct strongholdRaiders;

//精英战役
struct eliteCombat
{
    int _id;    //战役id
    int _mapid;//地图id
    int _level;        //等级
    int _spic;    //头像
    int _color;     //颜色
    int _open_stronghold;//关联关卡id
    std::string _name;    //名字
    std::list<Item> m_Item_list;
    int supply;
    int gongxun;

    //关卡的抗性
    combatAttribute m_combat_attribute;

    boost::shared_ptr<StrongholdGeneralData> m_generals[9];//防守武将

    strongholdRaiders m_raiders;
    int load();
    int getAttack();
};

typedef std::list<boost::shared_ptr<eliteCombat> > m_eliteCombat_list;//精英战役队列
typedef std::map<int, boost::shared_ptr<CharMapEliteCombatData> > m_char_map_eliteCombat;//玩家各地图精英战役数据

//精英战役管理
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
    //精英关卡是否全通
    bool isCharElitePassed(int cid, int mapid);
    
    int combatResult(Combat* pCombat);
    bool check_stronghold_can_sweep(int cid, int mapid, int eliteid);
    bool check_stronghold_can_attack(int cid, int mapid, int eliteid);
    int load();
    //每日凌成5点重置每个人攻击次数
    void reset();
    static eliteCombatMgr* getInstance();
private:
    static eliteCombatMgr* m_handle;
    std::map<int, boost::shared_ptr<m_eliteCombat_list> > m_eliteCombats;    //全部地图精英队列

    std::vector<boost::shared_ptr<eliteCombat> > m_eliteCombats2;

    std::map<int, boost::shared_ptr<m_char_map_eliteCombat> > m_char_eliteCombats;    //全部玩家地图精英队列
};

//根据当前所在区域查询精英战役列表
int ProcessGetEliteCombatList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//攻击
int ProcessAttackEliteCombat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//重置精英战役
int ProcessResetEliteCombat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//显示玩家单个精英关卡信息
int ProcessEliteStronghold(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//显示精英关卡攻略
int ProcessEliteRaiders(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//选择扫荡
//int ProcessAddSweepElite(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//开始扫荡
//int ProcessSweepElite(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取扫荡信息
//获取扫荡结果

