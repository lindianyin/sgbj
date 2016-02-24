#pragma once

#include <time.h>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <map>
#include <list>
#include "json_spirit.h"
#include "const_def.h"
#include <boost/uuid/uuid.hpp>
#include "net.h"
#include "item.h"
#include "utils_all.h"
#include "map.h"

using boost::uint64_t;
using namespace std;
using namespace json_spirit;
using namespace boost;

//公会等级数据(经验上限，人数限制)
const int iMaxGuildLevel = 20;
const int iGuildData[iMaxGuildLevel][2] =
{
    {20000, 40},
    {45000, 45},
    {100000, 50},
    {220000, 55},
    {360000, 60},
    {520000, 65},
    {700000, 70},
    {900000, 75},
    {1120000, 80},
    {1360000, 85},
    {1620000, 90},
    {1900000, 95},
    {2200000, 100},
    {2520000, 105},
    {2860000, 110},
    {3220000, 115},
    {3600000, 120},
    {4000000, 125},
    {4420000, 130},
    {4860000, 135}
};
const int iGuildOpenLevel = 16;

enum GUILD_EVENT
{
    GUILD_EVENT_DONATE_GOLD = 1,
    GUILD_EVENT_DONATE_SILVER,
    GUILD_EVENT_NEW_LEADER,
    GUILD_EVENT_NEW_ASS,
    GUILD_EVENT_ADD_MEMBER,
    GUILD_EVENT_LEAVE_MEMBER,
    GUILD_EVENT_FIRE_MEMBER,
};

enum GUILD_ACTION
{
    GUILD_ACTION_SKILL = 1,
    GUILD_ACTION_MOSHEN,
};

enum GUILD_OFFICE
{
    GUILD_OFFICE_NORMAL = 0,
    GUILD_OFFICE_ASS,
    GUILD_OFFICE_LEADER,
};

class CharData;

//玩家公会相关数据(退会就清除)
struct CharGuildData
{
    boost::shared_ptr<CharData> cdata;
    int m_cid;//角色id
    int m_gid;//军团id
    int m_offical;//职位 1 会长2 副会长 0 普通成员
    int m_contribution;//累计贡献
    time_t m_join_time;//加入时间

    CharGuildData()
    {
        m_cid = 0;
        m_gid = 0;
        m_offical = 0;
        m_contribution = 0;
        m_join_time = 0;
    }
    int save();
};

//玩家公会相关数据(退会保留)
struct CharGuildForeverData
{
    boost::shared_ptr<CharData> cdata;
    int m_cid;//角色id
    int m_gid;//军团id
    std::map<int,int> m_guild_box_state;//公会成就宝箱领取情况
    int m_guild_skill_level[4];

    CharGuildForeverData()
    {
        m_cid = 0;
        m_gid = 0;
        for (int i = 0; i < 4; ++i)
        {
            m_guild_skill_level[i] = 0;
        }
    }
    void getSkillAdd(int& attack, int& defense, int& magic, int& hp);
    void clearSkill();
    int save();
};

struct GuildApplication
{
    int cid;        //角色id
    int gid;        //申请军团id
    time_t app_time;        //申请时间
    std::string message;    //申请信息
    boost::shared_ptr<CharData> cdata;
};

struct GuildEvent
{
    time_t inputTime;
    int gid;
    int cid;
    std::string name;
    std::string message;
};

struct Guild
{
public:
    Guild(int id, int level, int exp, const std::string& name,
                const std::string& bulletin, const std::string& qqGroup);
    int load();
    boost::shared_ptr<CharGuildData> addMember(int cid);
    boost::shared_ptr<CharGuildData> getMember(int cid);
    int setOffical(int cid, int offical);
    int setNewLeader(int cid);
    int removeMember(int cid, int type = 0, const std::string& who = "");
    bool haveApplication(int cid);
    int cancelApplication(int cid);
    void addEvent(int cid, const std::string& name, const std::string& name2, int type, int extra = 0);
    void toSimpleObj(json_spirit::Object& obj);
    void toObj(json_spirit::Object& robj);
    void save();
    void saveMoshenData();
    void sort();

    int m_id;//唯一id
    int m_level;//军团等级
    int m_rank;//排行
    int m_exp;//当前经验
    int m_maxExp;//经验上限
    int m_memberNums;//成员数
    int m_memberLimit;//成员上限
    std::string m_qqGroup;//qq群号
    std::string m_name;//军团名
    std::string m_strBulletin;        //公告

    time_t m_createTime; //创建时间

    boost::shared_ptr<CharGuildData> m_leader;    //会长
    std::list<boost::shared_ptr<CharGuildData> > m_assistant;    //副会长

    std::map<int, boost::shared_ptr<CharGuildData> > m_members;    //成员
    std::list<boost::shared_ptr<CharGuildData> > m_members_list;    //成员(根据自定义规则排序用)
    std::map<int, boost::shared_ptr<GuildApplication> > m_applications;        //申请列表

    std::list<boost::shared_ptr<GuildEvent> > m_event_list;    //事件列表

    std::vector<int> m_moshen_list;//魔神通关玩家
};

struct baseGuildAction
{
    int id;                    //活动id
    int needlevel;                //活动需要军团等级
    std::string name;            //活动名称
    std::string memo;            //活动描述
};

struct baseGuildBox
{
    int id;//宝箱id
    int type;//1每日2成就
    int needlevel;//宝箱需要公会等级
    int needcontribution;//宝箱需要公会贡献
    std::list<Item> reward;//奖励
    void loadReward();
};

struct baseGuildSkill
{
    int type;//1攻击2防御3魔力4血量
    int level;//等级
    int needcontribution;//升级需要公会贡献
    int add;//加成
};

//公会魔神数据
struct baseGuildMoshen
{
    int m_id;       //魔神id
    int m_spic;     //图片
    int m_silver;   //初始筹码
    std::string m_name;
    std::string m_chat;
    //掉落
    std::list<Item> m_Item_list;
    boost::shared_ptr<baseStrongholdHeroData> m_hero;//英雄
    void loadReward();
    void toObj(json_spirit::Object& obj);
};

class guildMgr
{
public:
    guildMgr();
    //查找公会
    Guild* getGuild(int gid);
    //创建公会
    int createGuild(int cid, const std::string& name, json_spirit::Object& robj);
    //查看审核信息
    int getApplications(int cid, int gid, int page, int nums_per_page, json_spirit::Object& robj);
    //通过审核
    int acceptApplication(int cid, int gid, int tcid);
    //拒绝审核
    int rejectApplication(int cid, int gid, int tcid);
    //拒绝所有审核
    int rejectAllApplication(int cid, int gid);
    //获取公会名字
    std::string getGuildName(int gid);
    //获取公会等级
    int getGuildLevel(int gid);
    //查询公会信息
    int getGuildInfo(int cid, int gid, json_spirit::Object& robj);
    //查询公会成员
    int getGuildMembers(int cid, int gid, int page, int nums_per_page, json_spirit::Object& robj);
    //查询公会日志
    int getGuildEvents(int cid, int gid, json_spirit::Object& robj);
    //退出公会
    int quitGuild(int cid, int gid);
    //提交申请
    int submitApplication(int cid, int gid);
    //取消申请
    int cancelApplication(int cid, int gid);
    //任命
    int appointment(int cid, int gid, int tcid, int offical);
    //每日更新
    void dailyUpdate();
    //开除成员
    int fireMember(int cid, int gid, int tcid);
    //解散公会
    int dissolve(int cid, int gid);
    //查询公会列表
    int getGuildList(int cid, int page, int nums_per_page, json_spirit::Object& robj);
    //更新排名
    int updateRank();
    //捐献
    int donate(int cid, int gid, int add, int type);
    //设置公会信息
    int setGuildInfo(int cid, int gid, const std::string& memo, const std::string& qq);
    //获取每日宝箱信息
    int getGuildDailyBoxList(int cid, int gid, json_spirit::Object& robj);
    //领取公会每日宝箱
    int getGuildDailyBoxReward(int cid, int gid, int boxid, json_spirit::Object& robj);
    //查询公会成就宝箱
    int getGuildBoxList(int cid, int gid, int page, int nums_per_page, json_spirit::Object& robj);
    //领取公会成就宝箱
    int getGuildBoxReward(int cid, int gid, int boxid, json_spirit::Object& robj);
    //获取基础公会技能
    boost::shared_ptr<baseGuildSkill> getGuildSkill(int type, int level);
    int getGuildSkillList(int cid, int gid, json_spirit::Object& robj);
    int upgradeGuildSkill(int cid, int gid, int type);

    int getGuildActionLevel(int id);
    int getGuildActionList(CharData* pc, json_spirit::Object& robj);
    void addDonateEvent(int cid, const std::string& name, int extra);

    boost::shared_ptr<baseGuildMoshen> getGuildMoshen(int id);
    int getGuildMoshenInfo(int cid, int gid, int moshen_id, json_spirit::Object& robj);
    int combatResult(chessCombat* pCombat);

    //设置军团等级/经验
    void setGuildExp(int gid, int level, int exp);
    //设置玩家贡献
    void setCharContribution(CharData& cdata, int contribute);
private:
    std::map<int, boost::shared_ptr<Guild> > m_guild_maps;
    std::map<const std::string, boost::shared_ptr<Guild> > m_guild_maps2;
    std::list<boost::shared_ptr<Guild> > m_guild_list;
    int max_guild;

    std::list<boost::shared_ptr<GuildEvent> > m_event_list;    //全服捐献事件列表

    std::vector<baseGuildAction> m_base_guild_action;
    int max_guild_action;

    std::map<int, boost::shared_ptr<baseGuildBox> > m_guild_dailybox;
    std::map<int, boost::shared_ptr<baseGuildBox> > m_guild_box;

    std::map<std::pair<int,int>, boost::shared_ptr<baseGuildSkill>, compare_pair> m_guild_skills;

    std::vector<boost::shared_ptr<baseGuildMoshen> > m_guild_moshen;    //魔神列表
};


//公会列表
int ProcessGetGuildList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//申请加入
int ProcessDealJoinGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//创建公会
int ProcessCreateGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//公会信息
int ProcessGetGuildInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//捐献
int ProcessDonate(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//解散公会
int ProcessDissolveGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//退出公会
int ProcessQuitGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//设置公会信息
int ProcessSetGuildInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询公会活动列表大厅
int ProcessGetGuildActionList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询公会宝箱列表
int ProcessGetGuildBoxList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//公会宝箱领取
int ProcessGetGuildBoxReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//公会成员信息
int ProcessGetGuildMemberList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//会员职位操作
int ProcessDealGuildMember(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//日志
int ProcessGetGuildEventList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//申请列表
int ProcessGetGuildApplyList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//处理申请
int ProcessDealGuildApply(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询公会技能列表
int ProcessGetGuildSkillList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//升级公会技能
int ProcessUpgradeGuildSkill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询公会魔神信息
int ProcessGetGuildMoshenInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

