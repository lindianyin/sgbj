#pragma once

#include <string>
#include <time.h>
#include <map>
#include <list>
#include <vector>

#include "json_spirit.h"

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#include "const_def.h"
#include "net.h"
#include "item.h"
#include "multi_combat.hpp"
#include "data.h"

using namespace net;

const int iPKMaxRoom = 200;
const int iPKMaxSet = 8;

struct PkRankRewards
{
    int rank;
    std::list<Item> reward;//奖励
};

struct CharData;

enum enum_pk_state
{
    PK_STATE_INIT = 0,
    PK_STATE_ROOM = 1,
    PK_STATE_END,
};

struct CharPkData
{
private:
    boost::shared_ptr<CharData> m_charactor;    //对应的角色
public:
    CharPkData(int cid);
    boost::shared_ptr<CharData> getCharData();
    void toObj(json_spirit::Object& robj);
    void toSimpleObj(json_spirit::Object& robj);
    int sendInvite(int toid);
    int acceptInvite(int roomid);
    void load();
    void save();
    bool canChallenge();

    int m_cid;
    int m_total_get;//总赢取筹码
    int m_bet;//筹码底设定
    int m_state;//pk状态
    int m_roomid;
    int m_pos;
};

struct CharPkGet
{
    int m_cid;
    int m_rank;
    int m_total_get;
};

//房间座位
struct PkSet
{
    int m_pos;//编号
    int m_state;//状态(-1关闭0空闲1有玩家)
    bool m_ready;//准备状态
    boost::shared_ptr<CharPkData> m_playes;//玩家pk信息

    void reset()
    {
        m_state = 0;
        m_ready = false;
        m_playes.reset();
    }
};

//筹码场房间
struct PkRoom
{
    int m_id;//房间编号
    std::string m_name;//房间名字
    std::string m_password;//密码
    int m_cur;//当前玩家数量
    int m_max;//几人房
    int m_own_cid;//房主id
    int m_state;//状态(0空闲房间1等候玩家2开局游戏)
    int m_combat_id;//战斗id
    int m_bet;
    PkSet m_sets[iPKMaxSet];//房间里位置信息
    boost::shared_ptr<ChatChannel> _channel;//房间频道
    void toObj(json_spirit::Object& obj);
    void broadInfo();
};

class PkMgr
{
public:
    PkMgr();
    boost::shared_ptr<CharPkData> getPkData(int cid);
    boost::shared_ptr<CharPkData> getPkDataIndex(int index);
    boost::shared_ptr<CharPkData> addCharactor(int cid);
    boost::shared_ptr<CharPkGet> getPkGet(int cid);
    void updateRank();
    void removeCharactor(int cid);
    int querySelfInfo(int cid, json_spirit::Object& robj);
    int querySysList(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int queryRooms(json_spirit::mObject& o, json_spirit::Object& robj);
    int queryRoomInfo(int cid, json_spirit::Object& robj);
    //房间操作
    PkRoom* getRoom(int roomid)
    {
        if (roomid > 0 && roomid <= iPKMaxRoom)
        {
            return &m_rooms[roomid-1];
        }
        return NULL;
    }
    int createRoom(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int joinRoom(int cid, int roomid, std::string password, bool invite = false);
    int leaveRoom(int cid);
    int kickPlayer(int cid, json_spirit::mObject& o);
    int changeSet(int cid, json_spirit::mObject& o);
    int openSet(int cid, json_spirit::mObject& o);
    int closeSet(int cid, json_spirit::mObject& o);
    int setPassword(int cid, json_spirit::mObject& o);
    int setBet(int cid, json_spirit::mObject& o);
    int setReady(int cid, json_spirit::mObject& o);
    int startCombat(int cid);
    int invite(int cid);
    void broadInfo(int roomid);
    //请求排行
    int queryTopList(int cid, json_spirit::Object& robj);
    int queryTopCid();
    //排名奖励
    int queryRankRewards(int rank, json_spirit::Array& reward_list);
    int seasonAwards();
    int combatAllResult(multi_combat* pCombat);
    int combatResult(multi_combat* pCombat);
    boost::shared_ptr<ChatChannel> GetRoomChannel(int roomid);

private:
    boost::unordered_map<int, boost::shared_ptr<CharPkData> > m_pk_maps;//在线角色pk信息
    std::list<boost::shared_ptr<CharPkGet> > m_pk_get;//玩家pk获得
    std::vector<PkRankRewards> m_rewards;//排名奖励
    int m_last_rank;//最后有奖励的
    std::vector<int> m_last_top;//上次排名有称号的玩家
    PkRoom m_rooms[iPKMaxRoom];//房间
};

//获得挑战列表
int ProcessQueryPKList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询自己的挑战信息
int ProcessQueryPKInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询排行前3
int ProcessQueryPKTop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//pk操作
int ProcessDealPK(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获得房间列表
int ProcessQueryPKRooms(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询房间信息
int ProcessQueryRoomInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//创建房间
int ProcessPKCreateRoom(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//加入房间
int ProcessPKJoinRoom(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//离开房间
int ProcessPKLeaveRoom(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//踢出玩家
int ProcessPKKickPlayer(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//换位置
int ProcessPKChangeSet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//开位置
int ProcessPKOpenSet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//关位置
int ProcessPKCloseSet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//设定密码
int ProcessPKSetPassword(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//设定筹码
int ProcessPKSetBet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//设置准备
int ProcessPKSetReady(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//开始战斗
int ProcessPKStart(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//邀请
int ProcessPKInvite(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

