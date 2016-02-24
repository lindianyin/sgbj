
#pragma once

#include <vector>
#include <list>
#include <map>
#include <boost/cstdint.hpp>

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "data.h"
#include "new_combat.hpp"

enum TREASURE_TYPE
{
    TREASURE_INIT = 0,
    TREASURE_WHITE,
    TREASURE_GREEN,
    TREASURE_BLUE,
    TREASURE_PURPLE,
    TREASURE_ORANGE,
    TREASURE_RED,
    TREASURE_MAX = TREASURE_RED,
};

struct base_treasures
{
    int silver;
    int need_min;
    std::string name;
    std::string color_name;
    //掉落
    std::list<Item> m_Item_list;
};

struct treasure
{
    treasure(int cid, int tid)
    {
        m_cid = cid;
        m_tid = tid;
        m_rob_time = 2;
        m_silver = 0;
        m_needmin = 0;
        m_start_time = time(NULL);
        m_end_time = 0;
    }
    int m_cid;
    int m_tid;
    int m_needmin;
    int m_rob_time;//可被劫取次数
    int m_silver;
    time_t m_start_time;//开始时间
    time_t m_end_time;//结束时间
    int start();
    int finish();

    boost::uuids::uuid _uuid;    //定时器唯一id
};

struct char_treasure
{
    int m_cid;
    int m_tid;
    int m_state;//0初始1寻宝
    int m_x;
    int m_y;
    int m_refresh_times;//刷新次数
    boost::shared_ptr<treasure> m_treasure;//进行中的藏宝图
    int getCanRobTimes();
    int getCanStartTimes();
    void save();
    void reset();
};

struct event_log
{
    int m_atk_id;
    int m_def_id;
    int m_tid;
    int m_silver;
};

class treasureMgr
{
public:
    treasureMgr();
    int reload();
    void getButton(CharData* pc, json_spirit::Array& list);
    int getinsence(int cid);
    int getoutsence(int cid);
    int start(int cid);
    int finish(int cid);
    int combatResult(chessCombat* pCombat);
    int robReward(int atk_id, int def_id, int& silver);
    int jobDone(int cid);
    int getList(int cid, json_spirit::Object& robj);
    int getInfo(int cid, int id, json_spirit::Object& robj);
    int broadRobEvent();
    int getRobEvent(json_spirit::Object& robj);
    int getTreasureList(CharData* pc, json_spirit::Object& robj);
    int getTreasureInfo(int tid, json_spirit::Object& robj);
    int getCharTreasureInfo(CharData* pc, json_spirit::Object& robj);
    int refresh(CharData* pc, int type, json_spirit::Object& robj);
    boost::shared_ptr<char_treasure> getCharTreasure(int cid);
    boost::shared_ptr<base_treasures> getBaseTreasure(int tid);
private:
    std::map<int, boost::shared_ptr<char_treasure> > m_char_treasures;//玩家藏宝图
    std::map<int, boost::shared_ptr<base_treasures> > m_base_treasures;//基础藏宝图列表
    std::list<boost::shared_ptr<event_log> > m_event_log;//事件记录
    std::map<uint64_t,int> m_uid_list;//储存当前打开着护送界面的玩家列表
};

//藏宝图完成的处理
int ProcessTreasureDone(json_spirit::mObject& o);
//获取公告信息
int ProcessGetRobEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取藏宝图列表
int ProcessGetBaseTreasureInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取藏宝图列表
int ProcessGetAllTreasureList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取角色当前藏宝图信息
int ProcessGetCharTreasure(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessDealTreasure(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//进行中的藏宝图列表
int ProcessGetTreasureList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//进行中的藏宝图信息
int ProcessGetTreasureInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//离开藏宝图界面
int ProcessQuitTreasure(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

