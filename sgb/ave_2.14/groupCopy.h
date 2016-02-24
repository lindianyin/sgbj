#pragma once

#include "data.h"
#include "net.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace net;

struct groupCopy;
struct army_data;

//副本部队
struct groupCopyArmy
{
    int m_copyId;        //副本id
    int m_armyId;         //关卡id
    int m_stateNum;    //状态个数
    int m_level;        //等级
    int m_spic;            //头像id

    std::string m_name;//主将名

    //关卡的抗性
    combatAttribute m_combat_attribute;

    boost::shared_ptr<StrongholdGeneralData> m_generals[9];

    int load();
};

//多人副本玩家队伍
struct groupCopyTeam
{
    int _id;            //队伍id
    int _maxMember;    //最大人数
    int _bAutoAttack;    //人满自动战斗
    boost::shared_ptr<CharData> _memberList[iMaxGroupCopyMembers];    //成员列表
    int _bfriend[iMaxGroupCopyMembers];    //0表示正常玩家1表示虚拟的好友玩家

    CharData* leader();                //队长
    int getMemberPos(int cid);    //成员位置 1，2，3
    int leave(int cid, net::session_ptr& psession);                //成员离开
    void getMembersDetail(json_spirit::Array& members_list);
    int members();
    int add(boost::shared_ptr<CharData>, int b_friend = 0);
    groupCopy& _copyHandle;

    groupCopyTeam(groupCopy&);
};

//多人副本中的角色
struct groupCopyChar
{
    int _cid;            //角色id
    int _team_id;        //队伍id
    net::session_ptr _psession;    //会话句柄
};

//多人副本
struct groupCopy
{
    int _id;    //副本id
    int _mapid;//地图id
    int _level;//推荐等级
    int _spic;    //图片
    int _attackTimes;    //可攻击次数
    int _maxMember;    //最大人数
    int _maxTeamId;    //最大队伍id
    int _prestige_reward;//聲望獎勵
    int _gongxun_reward;        //功勋奖励
    
    std::string _name;    //名字
    std::string _memo;    //描述

    std::map<int, boost::shared_ptr<groupCopyTeam> > _teams;        //创建的队伍
    std::map<int, boost::shared_ptr<groupCopyChar> > _copyChars;    //进入的角色

    std::map<int, int> _attackTimesMaps;

    boost::shared_ptr<groupCopyArmy> _armys[iMaxGroupCopyArmys];    //副本npc

    int createTeam(boost::shared_ptr<CharData> cdata, groupCopyChar*);    //创建队伍
    int joinTeam(boost::shared_ptr<CharData> cdata, int team_id, net::session_ptr& psession, int b_friend = 0);    //加入队伍
    int leaveTeam(int cid, int team_id, net::session_ptr& psession);    //离开队伍
    int attackCopy(int team_id, net::session_ptr& psession);    //开始攻击
    int enterCopy(int cid);    //进入副本
    int leaveCopy(int cid);    //离开副本

    int getLeftTimes(int cid);    //剩余攻击次数

    int loadTeamArmy(int team_id, std::list<army_data*>& alist);    //加载玩家队伍的部队
    int loadNpcArmy(int team_id, std::list<army_data*>& dlist);    //加载npc部队

    int _setTeam(int cid, int team_id, net::session_ptr& psession);

    //查询副本内角色
    groupCopyChar* _getCopyChar(int cid, net::session_ptr&, bool);

    //查询副本队伍
    groupCopyTeam* _getTeam(int team_id);

    //广播，队伍移除
    int _broadTeamRemove(int team_id);
    //广播，新建队伍
    int _broadTeamAdd(int team_id, const std::string& name);
    //广播，队伍人数变化
    int _broadTeamChange(int team_id, int nums1, int nums2);
    //广播，队伍成员
    int _broadTeamMembers(int team_id, const std::string& msg);
    //广播消息
    int _broadMsg(const std::string& msg);

    //队伍详情
    int _getTeamDetail(int& team_id, int& autoAttack, int cid, json_spirit::Array& member_list, net::session_ptr& psession);
    int load();

    //每日凌成5点重置每个人攻击次数
    int reset();
    int reset(int cid);

    //关闭
    int close();
};

//多人副本管理
class groupCopyMgr
{
public:
    groupCopyMgr();
    ~groupCopyMgr();

    int leaveCopy(int cid, net::session_ptr&);    //离开副本

    int getCopyList(session_ptr& psession, CharData* pc, int mapid);        //查询副本列表

    groupCopy* getCopy(int copyId);

    int enterCopy(int cid, int copyid, net::session_ptr&);

    int load();
    //每日凌成5点重置每个人攻击次数
    int reset();
    int reset(int cid);

    void getAction(CharData* pc, json_spirit::Array& blist);

    int close();

    int open(int);

    int getAllCopyCanAttackTimes(int cid, int& total_attack_times);

    bool isOpen() {return m_state;}

    //玩家所在副本id，team id
    int getCopyIn(int cid, int& team_id);

    static groupCopyMgr* getInstance();
private:
    static groupCopyMgr* m_handle;

    int m_state;    // 1 活动开放中 0 关闭
    boost::uuids::uuid _uuid;    //定时器唯一id

    std::vector<boost::shared_ptr<groupCopy> > _groupCopys;    //所有副本列表

    //玩家所在副本
    std::map<int, int> _char_in_copys;
    //std::string m_copy_list_msg;
};

//查询副本列表 cmd:getMapMultiBoss //根据当前所在区域查询副本ID跟列表
int ProcessGetGroupCopyList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询副本队伍列表 cmd:getMultiBossList //获得多人副本列表
int ProcessGetGroupCopyTeamList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询副本队伍成员信息 cmd:getMultiBossMembers //获得队员信息
int ProcessGetGroupCopyTeamDetail(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getLeftAttackTime //查询剩余可攻击次数
int ProcessGetGroupCopyLeftTimes(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getMultiBossInfo//获得多人副本信息
int ProcessGetGroupCopyInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:getAutoAttackBoss //查询是否人满自动战斗  !!!!!!!!!!!!!!!还需要修改!
int ProcessGetGroupCopyAutoAttack(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:changeMultiBoss //切换副本
int ProcessEnterGroupCopy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:closeMultiBoss//关闭副本界面，不关注副本信息
int ProcessLeaveGroupCopy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:createMultiBoss //创建多人副本队伍
int ProcessCreateGroupCopyTeam(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:leaveMultiBoss //离开或解散队列
int ProcessLeaveGroupCopyTeam(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:joinMultiBoss //加入队伍
int ProcessJoinGroupCopyTeam(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:attackMultiBoss    //攻击副本
int ProcessAttackGroupCopy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:setAutoAttackBoss //设置人满自动战斗
int ProcessSetAutoAttack(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:changeMultiBossIndex //改变成员序号，向下调整一位参数:    id//副本id    roleId//成员ID返回:无
int ProcessChangeGroupCopyTeamMemberPos(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//cmd:fireMultiBoss //踢除成员
int ProcessFireGroupCopyMember(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessGroupCopyZhaoji(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessGroupCopyZhaojiFriend(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

