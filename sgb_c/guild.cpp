
#include "guild.h"
#include "errcode_def.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "utils_lang.h"
#include "net.h"
#include "text_filter.h"
#include "relation.h"
#include "singleton.h"

#include "spls_timer.h"
#include "json_spirit_writer_template.h"
#include "rewards.h"
#include "arena.h"

using namespace net;

extern void InsertSaveDb(const std::string& sql);
extern Database& GetDb();

extern int getSessionChar(session_ptr& psession, CharData* &pc);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);

extern std::string strGuildMsgDonateGold;
extern std::string strGuildMsgDonateSilver;
extern std::string strGuildMsgNewLeader;
extern std::string strGuildMsgNewAss;
extern std::string strGuildMsgFire;
extern std::string strGuildMsgLeave;
extern std::string strGuildMsgJoin;

extern std::string strSystemPassGuildMoshen;
extern std::string strSystemPassGuildMoshen2;

//公会列表
int ProcessGetGuildList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0, pageNums = 0;
    READ_INT_FROM_MOBJ(page,o,"page");
    READ_INT_FROM_MOBJ(pageNums,o,"pageNums");
    return Singleton<guildMgr>::Instance().getGuildList(pc->m_id, page, pageNums, robj);
}

//申请加入
int ProcessDealJoinGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    robj.push_back( Pair("purpose", purpose) );
    switch (purpose)
    {
        case 1:
            return Singleton<guildMgr>::Instance().submitApplication(pc->m_id, id);
        case 2:
            return Singleton<guildMgr>::Instance().cancelApplication(pc->m_id, id);
    }
    return HC_ERROR;
}

//创建公会
int ProcessCreateGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    std::string name = "";
    READ_STR_FROM_MOBJ(name,o,"name");
    if (!pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().createGuild(pc->m_id, name, robj);
    }
    return HC_ERROR_ALREADY_IN_GUILD;
}

//公会信息
int ProcessGetGuildInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().getGuildInfo(pc->m_id, pc->m_guild_data->m_gid, robj);
    }
    else
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
}

//捐献
int ProcessDonate(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int type = 0, nums = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    READ_INT_FROM_MOBJ(nums,o,"nums");
    if (nums <= 0)
    {
        return HC_ERROR;
    }
    if (pc->m_guild_data.get())
    {
        int add = 0;
        if (type == 1)
        {
            if (pc->subGold(nums, gold_cost_guild_donate, true) < 0)
                return HC_ERROR_NOT_ENOUGH_GOLD;
            add = nums * 10;
        }
        else if (type == 2)
        {
            if (pc->subSilver(nums, silver_cost_guild_donate) < 0)
                return HC_ERROR_NOT_ENOUGH_SILVER;
            add = nums / 1000 * 10;
        }
        else
        {
            return HC_ERROR;
        }
        robj.push_back( Pair("exp", add) );
        return Singleton<guildMgr>::Instance().donate(pc->m_id, pc->m_guild_data->m_gid, add, type);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//解散公会
int ProcessDissolveGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().dissolve(pc->m_id, pc->m_guild_data->m_gid);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//退出公会
int ProcessQuitGuild(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().quitGuild(pc->m_id, pc->m_guild_data->m_gid);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//设置公会信息
int ProcessSetGuildInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_guild_data.get())
    {
        std::string qq = "", memo = "";
        READ_STR_FROM_MOBJ(memo,o,"memo");
        READ_STR_FROM_MOBJ(qq,o,"qq");
        return Singleton<guildMgr>::Instance().setGuildInfo(pc->m_id, pc->m_guild_data->m_gid, memo, qq);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//查询公会活动列表大厅
int ProcessGetGuildActionList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().getGuildActionList(pc,robj);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//查询公会宝箱列表
int ProcessGetGuildBoxList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    int page = 0, pageNums = 0;
    READ_INT_FROM_MOBJ(page,o,"page");
    READ_INT_FROM_MOBJ(pageNums,o,"pageNums");
    robj.push_back( Pair("purpose", purpose) );
    if (pc->m_guild_data.get())
    {
        switch (purpose)
        {
            case 1://每日宝箱
                return Singleton<guildMgr>::Instance().getGuildDailyBoxList(pc->m_id, pc->m_guild_data->m_gid, robj);
            case 2://成就宝箱
                return Singleton<guildMgr>::Instance().getGuildBoxList(pc->m_id, pc->m_guild_data->m_gid, page, pageNums, robj);
        }
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//公会宝箱领取
int ProcessGetGuildBoxReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int purpose = 0, id = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("purpose", purpose) );
    if (pc->m_guild_data.get())
    {
        switch (purpose)
        {
            case 1://每日宝箱
                return Singleton<guildMgr>::Instance().getGuildDailyBoxReward(pc->m_id, pc->m_guild_data->m_gid, id, robj);
            case 2://成就宝箱
                return Singleton<guildMgr>::Instance().getGuildBoxReward(pc->m_id, pc->m_guild_data->m_gid, id, robj);
        }
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//公会成员信息
int ProcessGetGuildMemberList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0, pageNums = 0;
    READ_INT_FROM_MOBJ(page,o,"page");
    READ_INT_FROM_MOBJ(pageNums,o,"pageNums");
    if (pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().getGuildMembers(pc->m_id, pc->m_guild_data->m_gid, page, pageNums, robj);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//会员职位操作
int ProcessDealGuildMember(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    robj.push_back( Pair("purpose", purpose) );
    if (pc->m_guild_data.get())
    {
        switch (purpose)
        {
            case 1://转让会长
                return Singleton<guildMgr>::Instance().appointment(pc->m_id, pc->m_guild_data->m_gid, id, GUILD_OFFICE_LEADER);
            case 2://解除副会
                return Singleton<guildMgr>::Instance().appointment(pc->m_id, pc->m_guild_data->m_gid, id, GUILD_OFFICE_NORMAL);
            case 3://任命副会
                return Singleton<guildMgr>::Instance().appointment(pc->m_id, pc->m_guild_data->m_gid, id, GUILD_OFFICE_ASS);
            case 4://开除团员
                return Singleton<guildMgr>::Instance().fireMember(pc->m_id, pc->m_guild_data->m_gid, id);
        }
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//日志
int ProcessGetGuildEventList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().getGuildEvents(pc->m_id, pc->m_guild_data->m_gid, robj);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//申请列表
int ProcessGetGuildApplyList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 0, pageNums = 0;
    READ_INT_FROM_MOBJ(page,o,"page");
    READ_INT_FROM_MOBJ(pageNums,o,"pageNums");
    if (pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().getApplications(pc->m_id, pc->m_guild_data->m_gid, page, pageNums, robj);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//处理申请
int ProcessDealGuildApply(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        ERR();
        return ret;
    }
    int purpose = 0, id = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("purpose", purpose) );
    if (pc->m_guild_data.get())
    {
        switch (purpose)
        {
            case 1://通过
                return Singleton<guildMgr>::Instance().acceptApplication(pc->m_id, pc->m_guild_data->m_gid, id);
            case 2://拒绝
                return Singleton<guildMgr>::Instance().rejectApplication(pc->m_id, pc->m_guild_data->m_gid, id);
            case 3://拒绝全部
                return Singleton<guildMgr>::Instance().rejectAllApplication(pc->m_id, pc->m_guild_data->m_gid);
        }
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//查询公会技能列表
int ProcessGetGuildSkillList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().getGuildSkillList(pc->m_id, pc->m_guild_data->m_gid, robj);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//升级公会技能
int ProcessUpgradeGuildSkill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    if (pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().upgradeGuildSkill(pc->m_id, pc->m_guild_data->m_gid, type);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

//查询公会魔神信息
int ProcessGetGuildMoshenInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int moshen_id = 0;
    READ_INT_FROM_MOBJ(moshen_id,o,"moshen_id");
    if (pc->m_guild_data.get())
    {
        return Singleton<guildMgr>::Instance().getGuildMoshenInfo(pc->m_id, pc->m_guild_data->m_gid, moshen_id, robj);
    }
    return HC_ERROR_NOT_JOIN_GUILD;
}

void broadCastGuildMsg(int gid, const std::string& msg)
{
    boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(gid);
    if (gch)
    {
        std::string smsg = "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_guild)+",\"s\":200,\"m\":\"" + json_spirit::add_esc_chars<std::string>(msg, true) + "\"}";
        gch->BroadMsg(smsg);
    }
}

bool compare_guild(boost::shared_ptr<Guild> a, boost::shared_ptr<Guild> b)
{
    if (!a.get())
    {
        return false;
    }
    if (!b.get())
    {
        return true;
    }
    if (a->m_level > b->m_level)
    {
        return true;
    }
    else if (a->m_level < b->m_level)
    {
        return false;
    }
    else
    {
        return (a->m_exp > b->m_exp);
    }
}

bool compare_guild_member(boost::shared_ptr<CharGuildData> a, boost::shared_ptr<CharGuildData> b)
{
    if (!a.get())
    {
        return false;
    }
    if (!b.get())
    {
        return true;
    }
    if (a->m_offical != b->m_offical)
    {
        return a->m_offical > b->m_offical;
    }
    else if (a->m_contribution != b->m_contribution)
    {
        return (a->m_contribution > b->m_contribution);
    }
    else
    {
        return (a->m_join_time < b->m_join_time);
    }
}

int CharGuildData::save()
{
    InsertSaveDb("update char_guild_data set gid=" + LEX_CAST_STR(m_gid)
            + ",offical=" + LEX_CAST_STR(m_offical)
            + ",contribution=" + LEX_CAST_STR(m_contribution)
            + ",joinTime=" + LEX_CAST_STR(m_join_time)
            + " where cid=" + LEX_CAST_STR(m_cid));
    return 0;
}

void CharGuildForeverData::getSkillAdd(int& attack, int& defense, int& magic, int& hp)
{
    boost::shared_ptr<baseGuildSkill> bgs = Singleton<guildMgr>::Instance().getGuildSkill(1, m_guild_skill_level[0]);
    if (bgs.get())
    {
        attack = bgs->add;
    }
    bgs = Singleton<guildMgr>::Instance().getGuildSkill(2, m_guild_skill_level[1]);
    if (bgs.get())
    {
        defense = bgs->add;
    }
    bgs = Singleton<guildMgr>::Instance().getGuildSkill(3, m_guild_skill_level[2]);
    if (bgs.get())
    {
        magic = bgs->add;
    }
    bgs = Singleton<guildMgr>::Instance().getGuildSkill(4, m_guild_skill_level[3]);
    if (bgs.get())
    {
        hp = bgs->add;
    }
    return;
}

void CharGuildForeverData::clearSkill()
{
    for (int i = 0; i < 4; ++i)
    {
        m_guild_skill_level[i] = 0;
    }
    return;
}

int CharGuildForeverData::save()
{
    std::vector<int> m_box_state;
    for (std::map<int,int>::iterator it = m_guild_box_state.begin(); it != m_guild_box_state.end(); ++it)
    {
        if (it->second == 1)
        {
            m_box_state.push_back(it->first);
        }
    }
    const json_spirit::Value val_state(m_box_state.begin(), m_box_state.end());
    InsertSaveDb("update char_guild_forever_data set gid=" + LEX_CAST_STR(m_gid)
            + ",box_state='" + json_spirit::write(val_state)
            + "',attack_skill_level=" + LEX_CAST_STR(m_guild_skill_level[0])
            + ",defense_skill_level=" + LEX_CAST_STR(m_guild_skill_level[1])
            + ",magic_skill_level=" + LEX_CAST_STR(m_guild_skill_level[2])
            + ",hp_skill_level=" + LEX_CAST_STR(m_guild_skill_level[3])
            + " where cid=" + LEX_CAST_STR(m_cid));
    return 0;
}

Guild::Guild(int id, int level, int exp, const std::string& name,
                const std::string& bulletin, const std::string& qqGroup)
:m_id(id)
{
    m_level = level;
    m_exp = exp;
    m_maxExp = iGuildData[level-1][0];
    m_memberNums = 0;
    m_memberLimit = iGuildData[level-1][1];
    m_name = name;
    m_strBulletin = bulletin;
    m_qqGroup = qqGroup;
    m_createTime = 0;
}

int Guild::load()
{
    //cout<<"Guild::load()"<<m_id<<endl;
    Query q(GetDb());
    q.get_result("select cid,offical,contribution,joinTime from char_guild_data where gid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<CharGuildData> cm(new CharGuildData);
        cm->m_cid = q.getval();
        //cout<<"::load:"<<cm->cid<<endl;
        cm->m_offical = q.getval();
        cm->m_contribution = q.getval();
        cm->m_join_time = q.getval();
        cm->m_gid = m_id;
        cm->cdata = GeneralDataMgr::getInstance()->GetCharData(cm->m_cid);

        if (GUILD_OFFICE_LEADER == cm->m_offical)
        {
            m_leader = cm;
        }
        else if (GUILD_OFFICE_ASS == cm->m_offical)
        {
            m_assistant.push_back(cm);
        }
        if (cm->cdata.get())
        {
            cm->cdata->m_guild_data.reset();
            cm->cdata->m_guild_data = cm;
        }
        else
        {
            continue;
        }
        m_members[cm->m_cid] = cm;
        m_members_list.push_back(cm);
    }
    q.free_result();

    //加载申请
    q.get_result("select cid,message,inputTime from char_guild_applications where gid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<GuildApplication> app(new GuildApplication);
        app->cid = q.getval();
        app->message = q.getstr();
        app->app_time = q.getval();
        app->gid = m_id;
        app->cdata = GeneralDataMgr::getInstance()->GetCharData(app->cid);
        m_applications[app->cid] = app;
    }
    q.free_result();

    time_t last_input = 0;
    //加载事件
    q.get_result("select cid,msg,input,name from char_guild_event where gid=" + LEX_CAST_STR(m_id) + " order by input desc limit 10");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<GuildEvent> e(new GuildEvent);
        e->cid = q.getval();
        e->message = q.getstr();
        e->inputTime = q.getval();
        e->name = q.getstr();
        e->gid = m_id;
        m_event_list.push_back(e);
        last_input = e->inputTime;
    }
    q.free_result();
    InsertSaveDb("delete from char_guild_event where gid=" + LEX_CAST_STR(m_id) + " and input<" + LEX_CAST_STR(last_input));

    //加载魔神
    q.get_result("select finish_list from char_guild_moshen where gid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        std::string f = q.getstr();
        read_int_vector(f, m_moshen_list);
        q.free_result();
    }
    else
    {
        q.free_result();
        InsertSaveDb("replace into char_guild_moshen (gid,finish_list) values (" + LEX_CAST_STR(m_id) + ",'')");
    }
    //排列公会
    sort();
    return 0;
}

boost::shared_ptr<CharGuildData> Guild::addMember(int cid)
{
    std::map<int, boost::shared_ptr<CharGuildData> >::iterator it = m_members.find(cid);
    if (it != m_members.end())
    {
        return it->second;
    }
    boost::shared_ptr<CharGuildData> cm(new CharGuildData);
    cm->m_cid = cid;
    cm->cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    cm->m_contribution = 0;
    cm->m_join_time = time(NULL);
    cm->m_offical = GUILD_OFFICE_NORMAL;
    cm->m_gid = m_id;

    InsertSaveDb("replace into char_guild_data (gid,cid,offical,contribution,joinTime) values ("
            + LEX_CAST_STR(cm->m_gid) + "," + LEX_CAST_STR(cid) + ","
            + LEX_CAST_STR(cm->m_offical) + ","
            + LEX_CAST_STR(cm->m_contribution) + ","
            + LEX_CAST_STR(cm->m_join_time) + ")");
    if (cm->cdata.get())
    {
        //玩家公会数据重置
        cm->cdata->m_guild_data.reset();
        cm->cdata->m_guild_data = cm;
        //玩家永久公会数据处理
        if (cm->cdata->m_guild_forever_data.get())
        {
            cm->cdata->m_guild_forever_data->m_gid = m_id;
            cm->cdata->m_guild_forever_data->save();
            //英雄加成生效
            cm->cdata->m_heros.updateAttribute();
        }
        else
        {
            boost::shared_ptr<CharGuildForeverData> cgfd(new CharGuildForeverData);
            cgfd->cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            cgfd->m_cid = cid;
            cgfd->m_gid = m_id;
            cgfd->cdata->m_guild_forever_data = cgfd;
            InsertSaveDb("replace into char_guild_forever_data (gid,cid,box_state) values ("
                    + LEX_CAST_STR(cm->m_gid) + "," + LEX_CAST_STR(cid) + ",'')");
        }
        m_members[cid] = cm;
        m_members_list.push_back(cm);
        //通知
        cm->cdata->NotifyCharData();
        //加入公会频道
        if (cm->cdata->GetGuildId() > 0)
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cm->cdata->m_name);
            if (account.get())
            {
                boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(cm->cdata->GetGuildId());
                if (gch.get())
                {
                    gch->Add(account);
                }
            }
        }
        addEvent(cid, cm->cdata->m_name, "", GUILD_EVENT_ADD_MEMBER);
        return cm;
    }
    else
    {
        ERR();
        boost::shared_ptr<CharGuildData> cm;
        cm.reset();
        return cm;
    }
}

boost::shared_ptr<CharGuildData> Guild::getMember(int cid)
{
    std::map<int, boost::shared_ptr<CharGuildData> >::iterator it = m_members.find(cid);
    if (it != m_members.end())
    {
        return it->second;
    }
    boost::shared_ptr<CharGuildData> cm;
    cm.reset();
    return cm;
}

int Guild::setOffical(int cid, int offical)
{
    boost::shared_ptr<CharGuildData> cg = getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        ERR();
        return HC_ERROR;
    }
    if (cm->m_offical == offical)
    {
        return HC_SUCCESS;
    }
    if (cm->m_offical == GUILD_OFFICE_NORMAL)
    {
        if (offical == GUILD_OFFICE_ASS)
        {
            if ((int)m_assistant.size() >= 2)
            {
                return HC_ERROR_GUILD_MAX_ASSISTANT;
            }
            cm->m_offical = GUILD_OFFICE_ASS;
            m_assistant.push_back(cg);
            cm->save();
            sort();
            if (m_leader.get() && m_leader->cdata.get())
            {
                addEvent(m_leader->m_cid, m_leader->cdata->m_name, cm->cdata->m_name, GUILD_EVENT_NEW_ASS);
            }
            return HC_SUCCESS;
        }
        else if (m_members.size() == 1 && cm->m_offical == GUILD_OFFICE_NORMAL)
        {
            cm->m_offical = GUILD_OFFICE_LEADER;
            if (m_leader.get())
            {
                m_leader->m_offical = GUILD_OFFICE_NORMAL;
                m_leader->save();
            }
            m_leader.reset();
            m_leader = cg;
            cm->save();
            sort();
            return HC_SUCCESS;
        }
        else
        {
            ERR();
            return HC_ERROR;
        }
    }
    else if (cm->m_offical == GUILD_OFFICE_ASS)
    {
        //转让会长
        if (offical == GUILD_OFFICE_LEADER)
        {
            cm->m_offical = GUILD_OFFICE_LEADER;
            std::list<boost::shared_ptr<CharGuildData> >::iterator it = m_assistant.begin();
            while (it != m_assistant.end())
            {
                if ((*it)->m_cid == cm->m_cid)
                {
                    m_assistant.erase(it);
                    break;
                }
                ++it;
            }
            if (m_leader.get())
            {
                m_leader->m_offical = GUILD_OFFICE_ASS;
                m_assistant.push_back(m_leader);
                m_leader->save();
            }
            m_leader.reset();
            m_leader = cg;
            if (cm->cdata.get())
                addEvent(cm->cdata->m_id, cm->cdata->m_name, "", GUILD_EVENT_NEW_LEADER);
            cm->save();
            sort();
            return HC_SUCCESS;
        }
        //降职
        else if (offical == GUILD_OFFICE_NORMAL)
        {
            cm->m_offical = GUILD_OFFICE_NORMAL;
            cm->save();
            std::list<boost::shared_ptr<CharGuildData> >::iterator it = m_assistant.begin();
            while (it != m_assistant.end())
            {
                if ((*it)->m_cid == cm->m_cid)
                {
                    m_assistant.erase(it);
                    break;
                }
                ++it;
            }
            sort();
            return HC_SUCCESS;
        }
    }
    ERR();
    return HC_ERROR;
}

int Guild::setNewLeader(int cid)
{
    boost::shared_ptr<CharGuildData> cg = getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        ERR();
        return HC_ERROR;
    }
    if (cm->m_offical == GUILD_OFFICE_LEADER)
    {
        return HC_SUCCESS;
    }
    //转让军团长
    cm->m_offical = GUILD_OFFICE_LEADER;
    std::list<boost::shared_ptr<CharGuildData> >::iterator it = m_assistant.begin();
    while (it != m_assistant.end())
    {
        if ((*it)->m_cid == cm->m_cid)
        {
            m_assistant.erase(it);
            break;
        }
        ++it;
    }
    if (m_leader.get())
    {
        m_leader->m_offical = GUILD_OFFICE_NORMAL;
        m_leader->save();
    }
    m_leader.reset();
    m_leader = cg;
    if (cm->cdata.get())
        addEvent(cm->cdata->m_id, cm->cdata->m_name, "", GUILD_EVENT_NEW_LEADER);
    cm->save();
    sort();
    return HC_SUCCESS;
}

int Guild::removeMember(int cid, int type, const std::string& who)
{
    boost::shared_ptr<CharGuildData> cg = getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_SUCCESS;
    }
    //会长不能退出，只能解散
    if (GUILD_OFFICE_LEADER == cm->m_offical)
    {
        return HC_ERROR;
    }

    if (GUILD_OFFICE_ASS == cm->m_offical)
    {
        std::list<boost::shared_ptr<CharGuildData> >::iterator it = m_assistant.begin();
        while (it != m_assistant.end())
        {
            if ((*it)->m_cid == cm->m_cid)
            {
                m_assistant.erase(it);
                break;
            }
            ++it;
        }
    }

    //离开公会频道
    if (cm->cdata->GetGuildId() > 0)
    {
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cm->cdata->m_name);
        if (account.get())
        {
            boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(cm->cdata->GetGuildId());
            if (gch.get())
            {
                gch->Remove(account);
            }
        }
    }

    switch (type)
    {
        case GUILD_EVENT_FIRE_MEMBER:
            addEvent(cid, cm->cdata->m_name, who, type);
            break;
        case GUILD_EVENT_LEAVE_MEMBER:
            addEvent(cid, cm->cdata->m_name, "", type);
            break;
    }
    //删除成员
    m_members.erase(cid);
    std::list<boost::shared_ptr<CharGuildData> >::iterator it = m_members_list.begin();    //成员
    while (it != m_members_list.end())
    {
        if ((*it).get() && (*it)->m_cid == cid)
        {
            m_members_list.erase(it);
            break;
        }
        ++it;
    }
    if (cm->cdata.get())
    {
        //玩家公会数据删除
        cm->cdata->m_guild_data.reset();
        //玩家永久公会数据处理
        if (cm->cdata->m_guild_forever_data.get())
        {
            cm->cdata->m_guild_forever_data->m_gid = 0;
            cm->cdata->m_guild_forever_data->clearSkill();
            cm->cdata->m_guild_forever_data->save();
            //英雄加成生效
            cm->cdata->m_heros.updateAttribute();
        }
        cm->cdata->NotifyCharData();
    }
    //删除公会成员
    InsertSaveDb("delete from char_guild_data where cid=" + LEX_CAST_STR(cm->m_cid));
    return HC_SUCCESS;
}

bool Guild::haveApplication(int cid)
{
    return (m_applications.find(cid) != m_applications.end());
}

int Guild::cancelApplication(int cid)
{
    if (!haveApplication(cid))
    {
        return HC_SUCCESS;
    }
    std::map<int, boost::shared_ptr<GuildApplication> >::iterator it = m_applications.find(cid);
    boost::shared_ptr<GuildApplication> capp = it->second;
    GuildApplication* app = capp.get();
    m_applications.erase(it);
    return HC_SUCCESS;
}

void Guild::addEvent(int cid, const std::string& name, const std::string& name2, int type, int extra)
{
    GuildEvent* ce = new GuildEvent;
    ce->cid = cid;
    ce->gid = m_id;
    ce->inputTime = time(NULL);
    ce->name = name;
    switch (type)
    {
        case GUILD_EVENT_DONATE_GOLD:
            ce->message = strGuildMsgDonateGold;
            str_replace(ce->message, "$N", LEX_CAST_STR(extra / 10));
            str_replace(ce->message, "$A", LEX_CAST_STR(extra));
            break;
        case GUILD_EVENT_DONATE_SILVER:
            ce->message = strGuildMsgDonateSilver;
            str_replace(ce->message, "$N", LEX_CAST_STR(extra * 100));
            str_replace(ce->message, "$A", LEX_CAST_STR(extra));
            break;
        case GUILD_EVENT_NEW_LEADER:
            ce->message = strGuildMsgNewLeader;
            break;
        case GUILD_EVENT_NEW_ASS:
            ce->message = strGuildMsgNewAss;
            str_replace(ce->message, "$N", name2);
            break;
        case GUILD_EVENT_FIRE_MEMBER:
            ce->message = strGuildMsgFire;
            str_replace(ce->message, "$N", name2);
            break;
        case GUILD_EVENT_LEAVE_MEMBER:
            ce->message = strGuildMsgLeave;
            break;
        case GUILD_EVENT_ADD_MEMBER:
        default:
            ce->message = strGuildMsgJoin;
            break;
    }
    InsertSaveDb("insert into char_guild_event (gid,cid,name,msg,input) values (" + LEX_CAST_STR(ce->gid)
                        + "," + LEX_CAST_STR(ce->cid) + ",'" + GetDb().safestr(ce->name)
                        + "','" + GetDb().safestr(ce->message) + "',unix_timestamp())");
    boost::shared_ptr<GuildEvent> ge(ce);
    m_event_list.insert(m_event_list.begin(), ge);
    while (m_event_list.size() >= 10)
    {
        m_event_list.pop_back();
    }
}

void Guild::toSimpleObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_id) );
    obj.push_back( Pair("rank", m_rank) );
    obj.push_back( Pair("name", m_name) );
    obj.push_back( Pair("level", m_level) );
    obj.push_back( Pair("memberNums", m_members.size()) );
    obj.push_back( Pair("memberLimit", m_memberLimit) );
    if (m_leader.get() && m_leader->cdata.get())
    {
        obj.push_back( Pair("leader", m_leader->cdata->m_name) );
    }
}

void Guild::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_id) );
    obj.push_back( Pair("rank", m_rank) );
    obj.push_back( Pair("name", m_name) );
    obj.push_back( Pair("level", m_level) );
    obj.push_back( Pair("memberNums", m_members.size()) );
    obj.push_back( Pair("memberLimit", m_memberLimit) );
    obj.push_back( Pair("qq", m_qqGroup) );
    obj.push_back( Pair("exp", m_exp) );
    obj.push_back( Pair("expLimit", m_maxExp) );
    if (m_leader.get() && m_leader->cdata.get())
    {
        obj.push_back( Pair("leader", m_leader->cdata->m_name) );
    }
    obj.push_back( Pair("memo", m_strBulletin) );
}

void Guild::save()
{
    InsertSaveDb("update char_guilds set name='" + GetDb().safestr(m_name) +
            "',level='"  + LEX_CAST_STR(m_level) +
            "',exp='" + LEX_CAST_STR(m_exp) +
            "',qqGroup='" + GetDb().safestr(m_qqGroup) +
            "',bulletin='" + GetDb().safestr(m_strBulletin) + "' where id=" + LEX_CAST_STR(m_id));
}

void Guild::saveMoshenData()
{
    std::string sql = "update char_guild_moshen set finish_list='";
    int finish_count = 0;
    for (std::vector<int>::iterator it = m_moshen_list.begin(); it != m_moshen_list.end(); ++it)
    {
        if (finish_count)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++finish_count;
    }
    sql += "' where gid=" + LEX_CAST_STR(m_id);
    InsertSaveDb(sql);
}

void Guild::sort()
{
    m_members_list.sort(compare_guild_member);
}

void baseGuildBox::loadReward()
{
    Query q(GetDb());
    if (type == 1)
    {
        q.get_result("select itemType,itemId,counts,extra from base_guild_dailybox_rewards where bid="+LEX_CAST_STR(id)+" order by id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            Item item;
            item.type = q.getval();
            item.id = q.getval();
            item.nums = q.getval();
            item.extra = q.getval();
            reward.push_back(item);
        }
        q.free_result();
    }
    else if(type == 2)
    {
        q.get_result("select itemType,itemId,counts,extra from base_guild_box_rewards where bid="+LEX_CAST_STR(id)+" order by id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            Item item;
            item.type = q.getval();
            item.id = q.getval();
            item.nums = q.getval();
            item.extra = q.getval();
            reward.push_back(item);
        }
        q.free_result();
    }
}

void baseGuildMoshen::loadReward()
{
    Query q(GetDb());
    q.get_result("select itemType,itemId,counts,extra from base_guild_moshen_loot where id=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();
        item.extra = q.getval();
        m_Item_list.push_back(item);
    }
    q.free_result();
}

void baseGuildMoshen::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_id));
    if (m_hero.get())
    {
        obj.push_back( Pair("name", m_hero->m_name));
        obj.push_back( Pair("spic", m_hero->m_spic));
    }
    json_spirit::Array get_list;
    itemlistToArray(m_Item_list, get_list);
    obj.push_back( Pair("get", get_list) );
    return;
}

guildMgr::guildMgr()
{
    cout<<"int guildMgr::init()..."<<endl;
    Query q(GetDb());
    //公会活动
    max_guild_action = 0;
    q.get_result("select id,need_level,name,memo from base_guild_action where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        baseGuildAction action;
        action.id = q.getval();
        action.needlevel = q.getval();
        action.name = q.getstr();
        action.memo = q.getstr();
        m_base_guild_action.push_back(action);
        max_guild_action = action.id;
    }
    q.free_result();
    //公会宝箱
    q.get_result("select id,need_level,need_contribution from base_guild_dailybox where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<baseGuildBox> bgb(new baseGuildBox);
        bgb->id = q.getval();
        bgb->type = 1;
        bgb->needlevel = q.getval();
        bgb->needcontribution = q.getval();
        bgb->loadReward();
        m_guild_dailybox[bgb->id] = bgb;
    }
    q.free_result();
    q.get_result("select id,need_level,need_contribution from base_guild_box where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<baseGuildBox> bgb(new baseGuildBox);
        bgb->id = q.getval();
        bgb->type = 2;
        bgb->needlevel = q.getval();
        bgb->needcontribution = q.getval();
        bgb->loadReward();
        m_guild_box[bgb->id] = bgb;
    }
    q.free_result();
    //公会技能
    q.get_result("select type,level,need_contribution,add_fac from base_guild_skill where 1 order by type,level");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<baseGuildSkill> bgs(new baseGuildSkill);
        int type = q.getval();
        int level = q.getval();
        bgs->type = type;
        bgs->level = level;
        bgs->needcontribution = q.getval();
        bgs->add = q.getval();
        m_guild_skills.insert(std::make_pair(std::make_pair(type,level),bgs));
    }
    q.free_result();
    //公会魔神
    q.get_result("select id,level,name,spic,silver,chat,race,star,attack,defense,magic,hp from base_guild_moshen where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int level = q.getval();
        boost::shared_ptr<baseGuildMoshen> c;
        c.reset(new baseGuildMoshen);
        c->m_id = id;
        c->m_name = q.getstr();
        c->m_spic = q.getval();
        c->m_silver = q.getval();
        c->m_chat = q.getstr();
        //加载守将
        boost::shared_ptr<baseStrongholdHeroData> sh;
        if (!(c->m_hero.get()))
        {
            sh.reset(new (baseStrongholdHeroData));
            c->m_hero = sh;
        }
        else
        {
            sh = c->m_hero;
        }
        sh->m_name = c->m_name;
        sh->m_spic = c->m_spic;
        sh->m_race = q.getval();
        sh->m_star = q.getval();
        sh->m_quality = sh->m_star;
        sh->m_level = level;
        sh->m_attack = q.getval();
        sh->m_defense = q.getval();
        sh->m_magic = q.getval();
        sh->m_hp = q.getval();
        c->loadReward();
        m_guild_moshen.push_back(c);
        assert(m_guild_moshen.size() == id);
    }
    q.free_result();
    //加载事件
    time_t last_input = 0;
    q.get_result("select cid,msg,input,name from char_guild_donate_event where 1 order by input desc limit 4");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<GuildEvent> e(new GuildEvent);
        e->cid = q.getval();
        e->message = q.getstr();
        e->inputTime = q.getval();
        e->name = q.getstr();
        e->gid = 0;
        m_event_list.push_back(e);
        last_input = e->inputTime;
    }
    q.free_result();
    InsertSaveDb("delete from char_guild_donate_event where input<" + LEX_CAST_STR(last_input));

    max_guild = 0;
    q.get_result("select id,name,level,exp,qqGroup,bulletin,createTime from char_guilds where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        std::string name = q.getstr();
        int level = q.getval();
        int exp = q.getval();
        std::string qqGroup = q.getstr();
        std::string bulletin = q.getstr();
        time_t createTime = q.getval();
        boost::shared_ptr<Guild> gdata(new Guild(id, level, exp, name, bulletin, qqGroup));
        m_guild_maps[id] = gdata;
        m_guild_maps2[name] = gdata;
        gdata->m_createTime = createTime;
        m_guild_list.push_back(gdata);

        max_guild = id;
    }
    q.free_result();

    std::map<int, boost::shared_ptr<Guild> >::iterator it = m_guild_maps.begin();
    while (it != m_guild_maps.end())
    {
        if (it->second.get())
        {
            it->second->load();
        }
        ++it;
    }
    //更新排名
    updateRank();

    //公会永久数据载入
    q.get_result("select cid,gid,box_state,attack_skill_level,defense_skill_level,magic_skill_level,hp_skill_level from char_guild_forever_data where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int cid = q.getval();
        boost::shared_ptr<CharGuildForeverData> cgfd(new CharGuildForeverData);
        cgfd->cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        cgfd->m_cid = cid;
        cgfd->m_gid = q.getval();
        std::string state = q.getstr();
        if (state != "")
        {
            json_spirit::Value types;
            json_spirit::read(state, types);
            if (types.type() == json_spirit::array_type)
            {
                json_spirit::Array& types_array = types.get_array();
                for (json_spirit::Array::iterator it = types_array.begin(); it != types_array.end(); ++it)
                {
                    if ((*it).type() != json_spirit::int_type)
                    {
                        break;
                    }
                    cgfd->m_guild_box_state[(*it).get_int()] = 1;
                }
            }
            else
            {
                ERR();
            }
        }
        for (int i = 0; i < 4; ++i)
        {
            cgfd->m_guild_skill_level[i] = q.getval();
        }
        if (cgfd->cdata.get())
            cgfd->cdata->m_guild_forever_data = cgfd;
    }
    q.free_result();
}

//查找公会
Guild* guildMgr::getGuild(int gid)
{
    std::map<int, boost::shared_ptr<Guild> >::iterator it = m_guild_maps.find(gid);
    if (it == m_guild_maps.end() || !it->second.get())
    {
        return NULL;
    }
    return it->second.get();
}

//创建公会
int guildMgr::createGuild(int cid, const std::string& name, json_spirit::Object& robj)
{
    std::map<const std::string, boost::shared_ptr<Guild> >::iterator it = m_guild_maps2.find(name);
    if (it != m_guild_maps2.end())
    {
        return HC_ERROR_GUILD_NAME_EXIST;
    }
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    if (name.length() > 18)
    {
        return HC_ERROR_NAME_TOO_LONG;
    }
    if (name.length() < 3 || !Forbid_word_replace::getInstance()->isLegal(name))
    {
        return HC_ERROR_NAME_ILLEGAL;
    }
    //等级限制
    if (pc->m_level < iGuildOpenLevel)
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    if (pc->subSilver(300000, silver_cost_guild_create) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_SILVER;
    }
    boost::shared_ptr<Guild> gdata(new Guild(++max_guild, 1, 0, name, "", ""));
    m_guild_maps[max_guild] = gdata;
    m_guild_maps2[name] = gdata;
    gdata->m_createTime = time(NULL);
    m_guild_list.push_back(gdata);
    gdata->m_rank = m_guild_list.size();

    InsertSaveDb("insert into char_guilds (id,name,level,exp,qqGroup,bulletin,createTime) values ("
        + LEX_CAST_STR(max_guild) + ",'" + GetDb().safestr(name) + "',1,0,'',''," + LEX_CAST_STR(gdata->m_createTime) +")");
    robj.push_back( Pair("id", max_guild) );

    boost::shared_ptr<CharGuildData> cm = gdata->addMember(cid);
    //设置会长
    gdata->setOffical(cid, GUILD_OFFICE_LEADER);
    //取消该角色其他的申请
    std::map<int, boost::shared_ptr<Guild> >::iterator itx = m_guild_maps.begin();
    while (itx != m_guild_maps.end())
    {
        if (itx->second.get())
        {
            itx->second->cancelApplication(cid);
        }
        ++itx;
    }
    pc->NotifyCharData();
    return HC_SUCCESS;
}

//查看审核信息
int guildMgr::getApplications(int cid, int gid, int page, int nums_per_page, json_spirit::Object& robj)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    //权限审查
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->m_offical != GUILD_OFFICE_LEADER && cm->m_offical != GUILD_OFFICE_ASS)
    {
        return HC_ERROR_GUILD_OFFICAL_LIMIT;
    }
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;

    json_spirit::Array app_list;
    std::map<int, boost::shared_ptr<GuildApplication> >::iterator it_app = cp->m_applications.begin();
    while (it_app != cp->m_applications.end())
    {
        if (it_app->second.get() && it_app->second->cdata.get())
        {
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                GuildApplication* app = it_app->second.get();
                CharData* pc = it_app->second->cdata.get();
                json_spirit::Object obj;
                obj.push_back( Pair("id", pc->m_id) );
                obj.push_back( Pair("name", pc->m_name) );
                obj.push_back( Pair("level", pc->m_level) );
                obj.push_back( Pair("time", time(NULL)-app->app_time) );
                app_list.push_back(obj);
            }
        }
        ++it_app;
    }
    robj.push_back( Pair("list", app_list) );

    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//通过审核
int guildMgr::acceptApplication(int cid, int gid, int tcid)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    //权限审查
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->m_offical != GUILD_OFFICE_LEADER && cm->m_offical != GUILD_OFFICE_ASS)
    {
        return HC_ERROR_GUILD_OFFICAL_LIMIT;
    }
    //人员数量是否达到上限了
    if ((int)cp->m_members_list.size() >= cp->m_memberLimit)
    {
        return HC_ERROR_GUILD_MAX_MEMBERS;
    }
    //是否有该申请
    std::map<int, boost::shared_ptr<GuildApplication> >::iterator it_app = cp->m_applications.find(tcid);
    if (it_app == cp->m_applications.end())
    {
        return HC_ERROR;
    }
    cp->m_applications.erase(it_app);
    cp->addMember(tcid);
    cp->sort();
    //取消该角色其他的申请
    std::map<int, boost::shared_ptr<Guild> >::iterator it = m_guild_maps.begin();
    while (it != m_guild_maps.end())
    {
        if (it->second.get())
        {
            it->second->cancelApplication(tcid);
        }
        ++it;
    }
    return HC_SUCCESS;
}

//拒绝审核
int guildMgr::rejectApplication(int cid, int gid, int tcid)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    //权限审查
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->m_offical != GUILD_OFFICE_LEADER && cm->m_offical != GUILD_OFFICE_ASS)
    {
        return HC_ERROR_GUILD_OFFICAL_LIMIT;
    }
    //是否有该申请
    std::map<int, boost::shared_ptr<GuildApplication> >::iterator it_app = cp->m_applications.find(tcid);
    if (it_app == cp->m_applications.end())
    {
        return HC_ERROR;
    }
    cp->m_applications.erase(it_app);
    return HC_SUCCESS;
}

//拒绝所有审核
int guildMgr::rejectAllApplication(int cid, int gid)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    //权限审查
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->m_offical != GUILD_OFFICE_LEADER && cm->m_offical != GUILD_OFFICE_ASS)
    {
        return HC_ERROR_GUILD_OFFICAL_LIMIT;
    }
    std::map<int, boost::shared_ptr<GuildApplication> >::iterator it_app = cp->m_applications.begin();
    while (it_app != cp->m_applications.end())
    {
        if (it_app->second.get() && it_app->second->cdata.get())
        {
            CharData* cdata = it_app->second->cdata.get();
            rejectApplication(cid,gid,cdata->m_id);
        }
        ++it_app;
    }
    cp->m_applications.clear();
    return HC_SUCCESS;
}

//获取公会名字
std::string guildMgr::getGuildName(int gid)
{
    std::string name = "";
    //找军团
    std::map<int, boost::shared_ptr<Guild> >::iterator it = m_guild_maps.find(gid);
    if (it != m_guild_maps.end() && it->second.get())
    {
        name = it->second->m_name;
    }
    return name;
}

//获取公会等级
int guildMgr::getGuildLevel(int gid)
{
    int level = 0;
    //找军团
    std::map<int, boost::shared_ptr<Guild> >::iterator it = m_guild_maps.find(gid);
    if (it != m_guild_maps.end() && it->second.get())
    {
        level = it->second->m_level;
    }
    return level;
}

//查询公会信息
int guildMgr::getGuildInfo(int cid, int gid, json_spirit::Object& robj)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    cp->toObj(robj);
    robj.push_back( Pair("offical", cm->m_offical) );
    robj.push_back( Pair("contribution", cm->m_contribution) );
    return HC_SUCCESS;
}

//查询公会成员
int guildMgr::getGuildMembers(int cid, int gid, int page, int nums_per_page, json_spirit::Object& robj)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;

    bool bDetail = true;
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        bDetail = false;
    }
    json_spirit::Array alist;
    std::list<boost::shared_ptr<CharGuildData> >::iterator it_l = cp->m_members_list.begin();
    while (it_l != cp->m_members_list.end())
    {
        if (it_l->get() && it_l->get()->cdata.get())
        {
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                CharGuildData* cm = it_l->get();
                CharData* cdata = cm->cdata.get();
                json_spirit::Object obj;
                obj.push_back( Pair("id", cm->m_cid) );
                obj.push_back( Pair("name", cdata->m_name) );
                obj.push_back( Pair("level", cdata->m_level) );
                obj.push_back( Pair("offical", cm->m_offical) );
                if (bDetail)
                {
                    obj.push_back( Pair("contribution", cm->m_contribution) );
                    int day_con = cdata->queryExtraData(char_data_type_daily,char_data_daily_guild_contribution);
                    obj.push_back( Pair("contributionDay", day_con) );
                    obj.push_back( Pair("login", time(NULL) - cdata->m_login_time) );
                }
                boost::shared_ptr<CharArenaData> rd = Singleton<arenaMgr>::Instance().getArenaData(cdata->m_id);
                if (rd.get() && rd->getCharData().get())
                {
                    obj.push_back( Pair("rank", rd->m_rank) );
                }
                obj.push_back( Pair("online", cdata->m_is_online) );
                alist.push_back(obj);
            }
        }
        ++it_l;
    }
    robj.push_back( Pair("list", alist) );
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//查询公会日志
int guildMgr::getGuildEvents(int cid, int gid, json_spirit::Object& robj)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    json_spirit::Array elist;
    std::list<boost::shared_ptr<GuildEvent> >::iterator it_e = cp->m_event_list.begin();
    while (it_e != cp->m_event_list.end())
    {
        if (it_e->get())
        {
            GuildEvent* pe = it_e->get();
            json_spirit::Object obj;
            obj.push_back( Pair("id", pe->cid) );
            obj.push_back( Pair("name", pe->name) );
            obj.push_back( Pair("memo", pe->message) );
            obj.push_back( Pair("time", time(NULL)-pe->inputTime) );
            elist.push_back(obj);
        }
        ++it_e;
    }
    robj.push_back( Pair("list", elist) );
    return HC_SUCCESS;
}

//退出公会
int guildMgr::quitGuild(int cid, int gid)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    return cp->removeMember(cid, GUILD_EVENT_LEAVE_MEMBER, "");
}

//提交申请
int guildMgr::submitApplication(int cid, int gid)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR;
    }
    if (cp->m_memberLimit <= (int)cp->m_members_list.size())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    CharData* pc = cdata.get();
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    if (pc->GetGuildId())
    {
        return HC_ERROR_ALREADY_IN_GUILD;
    }
    //等级限制
    if (pc->m_level < iGuildOpenLevel)
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    //是否提交过申请了
    if (cp->haveApplication(cid))
    {
        return HC_ERROR_GUILD_ALREADY_APPLY;
    }
    boost::shared_ptr<GuildApplication> capp(new GuildApplication);
    capp->cid = cid;
    capp->cdata = cdata;
    capp->gid = gid;
    capp->app_time = time(NULL);
    cp->m_applications[cid] = capp;
    //通知会长副会长
    json_spirit::Object msg_obj;
    msg_obj.push_back( Pair("cmd", "guild_sub") );
    msg_obj.push_back( Pair("s", 200) );
    cp->m_leader->cdata->sendObj(msg_obj);
    std::list<boost::shared_ptr<CharGuildData> >::iterator it = cp->m_assistant.begin();
    while (it != cp->m_assistant.end())
    {
        if ((*it).get() && (*it)->cdata.get())
        {
            (*it)->cdata->sendObj(msg_obj);
        }
        ++it;
    }
    return HC_SUCCESS;
}

//取消申请
int guildMgr::cancelApplication(int cid, int gid)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR;
    }
    cp->cancelApplication(cid);
    return HC_SUCCESS;
}

//任命
int guildMgr::appointment(int cid, int gid, int tcid, int offical)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    //权限审查
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->m_offical != GUILD_OFFICE_LEADER)
    {
        return HC_ERROR_GUILD_OFFICAL_LIMIT;
    }
    return cp->setOffical(tcid, offical);
}

//每日更新
void guildMgr::dailyUpdate()
{
    time_t t_now = time(NULL);
    std::map<int, boost::shared_ptr<Guild> >::iterator it = m_guild_maps.begin();
    while(it != m_guild_maps.end())
    {
        if (it->second.get())
        {
            Guild* cp = it->second.get();
            //会长变更检查
            if (cp->m_leader.get() && cp->m_leader->cdata.get() && (t_now - cp->m_leader->cdata->m_login_time) >= iONE_DAY_SECS*3)
            {
                std::list<boost::shared_ptr<CharGuildData> >::iterator it_char = cp->m_members_list.begin();
                while (it_char != cp->m_members_list.end())
                {
                    CharGuildData* cm = (*it_char).get();
                    if (cm->m_offical != GUILD_OFFICE_LEADER)
                    {
                        cp->setNewLeader(cm->m_cid);
                        break;
                    }
                    ++it_char;
                }
            }
            //魔神重置
            cp->m_moshen_list.clear();
        }
        ++it;
    }
    InsertSaveDb("update char_guild_moshen set finish_list='' where 1");
    return;
}

//开除成员
int guildMgr::fireMember(int cid, int gid, int tcid)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    //权限审查
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->m_offical != GUILD_OFFICE_LEADER && cm->m_offical != GUILD_OFFICE_ASS)
    {
        return HC_ERROR_GUILD_OFFICAL_LIMIT;
    }
    boost::shared_ptr<CharGuildData> target_cg = cp->getMember(tcid);
    if (NULL == target_cg.get())
    {
        return HC_ERROR;
    }
    //只能踢比自己级别低的成员
    if (cm->m_offical < target_cg->m_offical)
    {
        return HC_ERROR_GUILD_OFFICAL_LIMIT;
    }
    return cp->removeMember(tcid, GUILD_EVENT_FIRE_MEMBER, cm->cdata->m_name);
}

//解散公会
int guildMgr::dissolve(int cid, int gid)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    //权限审查
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->m_offical != GUILD_OFFICE_LEADER)
    {
        return HC_ERROR_GUILD_OFFICAL_LIMIT;
    }

    //数据库
    InsertSaveDb("delete from char_guilds where id=" + LEX_CAST_STR(cp->m_id));
    InsertSaveDb("delete from char_guild_data where gid=" + LEX_CAST_STR(cp->m_id));
    InsertSaveDb("delete from char_guild_applications where gid=" + LEX_CAST_STR(cp->m_id));
    InsertSaveDb("delete from char_guild_event where gid=" + LEX_CAST_STR(cp->m_id));

    boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(cm->cdata->GetGuildId());

    //删除角色的公会数据
    std::list<boost::shared_ptr<CharGuildData> >::iterator it_m = cp->m_members_list.begin();
    while (it_m != cp->m_members_list.end())
    {
        if (it_m->get() && it_m->get()->cdata.get())
        {
            if (gch.get())
            {
                //离开军团频道
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(it_m->get()->cdata->m_name);
                if (account.get())
                {
                    gch->Remove(account);
                }
            }
            it_m->get()->cdata->m_guild_data.reset();
            it_m->get()->cdata->NotifyCharData();
            //玩家永久公会数据处理
            if (cm->cdata->m_guild_forever_data.get())
            {
                cm->cdata->m_guild_forever_data->m_gid = 0;
                cm->cdata->m_guild_forever_data->clearSkill();
                cm->cdata->m_guild_forever_data->save();
                //英雄加成生效
                cm->cdata->m_heros.updateAttribute();
            }
        }
        ++it_m;
    }
    //删除角色的申请数据
    rejectAllApplication(cid,gid);
    //删除军团
    m_guild_maps.erase(cp->m_id);
    m_guild_maps2.erase(cp->m_name);
    std::list<boost::shared_ptr<Guild> >::iterator itx = m_guild_list.begin();
    while (itx != m_guild_list.end())
    {
        if (itx->get() && itx->get()->m_id == gid)
        {
            m_guild_list.erase(itx);
            break;
        }
        ++itx;
    }
    updateRank();
    return HC_SUCCESS;
}

//查询公会列表
int guildMgr::getGuildList(int cid, int page, int nums_per_page, json_spirit::Object& robj)
{
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;

    json_spirit::Array clist;
    std::list<boost::shared_ptr<Guild> >::iterator it = m_guild_list.begin();
    while (it != m_guild_list.end())
    {
        if (it->get())
        {
            Guild* cp = it->get();
            {
                ++cur_nums;
                if (cur_nums >= first_nums && cur_nums <= last_nums)
                {
                    json_spirit::Object obj;
                    cp->toSimpleObj(obj);
                    obj.push_back( Pair("isApply", cp->haveApplication(cid)?1:0) );
                    clist.push_back(obj);
                }
            }
        }
        ++it;
    }
    robj.push_back( Pair("list", clist) );

    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//更新排名
int guildMgr::updateRank()
{
    m_guild_list.sort(compare_guild);
    int rank = 0;
    std::list<boost::shared_ptr<Guild> >::iterator it = m_guild_list.begin();
    while (it != m_guild_list.end())
    {
        if (it->get())
        {
            it->get()->m_rank = ++rank;
        }
        ++it;
    }
    return HC_SUCCESS;
}

//捐献
int guildMgr::donate(int cid, int gid, int add, int type)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm || NULL == cm->cdata.get())
    {
        return HC_ERROR;
    }
    cm->m_contribution += add;
    int day_con = cm->cdata->queryExtraData(char_data_type_daily,char_data_daily_guild_contribution);
    cm->cdata->setExtraData(char_data_type_daily,char_data_daily_guild_contribution, day_con+add);
    int event = GUILD_EVENT_DONATE_GOLD;
    if (type == 1)
        event = GUILD_EVENT_DONATE_GOLD;
    else if(type == 2)
        event = GUILD_EVENT_DONATE_SILVER;
    cp->addEvent(cid, cm->cdata->m_name, "", event, add);
    if (type == 1)
    {
        addDonateEvent(cid, cm->cdata->m_name, add);
    }
    cp->m_exp += add;
    while (cp->m_level < iMaxGuildLevel && cp->m_exp >= cp->m_maxExp)
    {
        ++cp->m_level;
        cp->m_exp -= cp->m_maxExp;
        cp->m_maxExp = iGuildData[cp->m_level-1][0];
        cp->m_memberLimit = iGuildData[cp->m_level-1][1];
        //更新排名
        updateRank();
    }
    cm->save();
    cp->save();
    cp->sort();
    return HC_SUCCESS;
}

//设置公会信息
int guildMgr::setGuildInfo(int cid, int gid, const std::string& memo, const std::string& qq)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    //权限审查
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm)
    {
        return HC_ERROR;
    }
    if (cm->m_offical != GUILD_OFFICE_LEADER && cm->m_offical != GUILD_OFFICE_ASS)
    {
        return HC_ERROR_GUILD_OFFICAL_LIMIT;
    }
    cp->m_qqGroup = qq;
    cp->m_strBulletin = memo;
    cp->save();
    return HC_SUCCESS;
}

//查询公会每日宝箱
int guildMgr::getGuildDailyBoxList(int cid, int gid, json_spirit::Object& robj)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm || !cm->cdata.get())
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    json_spirit::Array elist;
    std::list<boost::shared_ptr<GuildEvent> >::iterator it_e = m_event_list.begin();
    while (it_e != m_event_list.end())
    {
        if (it_e->get())
        {
            GuildEvent* pe = it_e->get();
            json_spirit::Object obj;
            obj.push_back( Pair("id", pe->cid) );
            obj.push_back( Pair("name", pe->name) );
            obj.push_back( Pair("memo", pe->message) );
            obj.push_back( Pair("time", time(NULL)-pe->inputTime) );
            elist.push_back(obj);
        }
        ++it_e;
    }
    robj.push_back( Pair("elist", elist) );
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<baseGuildBox> >::iterator it = m_guild_dailybox.begin();
    while (it != m_guild_dailybox.end())
    {
        if (it->second.get() && (it->second->needlevel == cp->m_level || it->second->needlevel == cp->m_level+1))
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->first) );
            obj.push_back( Pair("needlevel", it->second->needlevel) );
            obj.push_back( Pair("needcontribution", it->second->needcontribution) );
            int state = cm->cdata->queryExtraData(char_data_type_daily,char_data_daily_guild_box);
            obj.push_back( Pair("state", state) );
            json_spirit::Array reward_list;
            for (std::list<Item>::iterator it_i = it->second->reward.begin(); it_i != it->second->reward.end(); ++it_i)
            {
                Item& item = *it_i;
                json_spirit::Object obj;
                item.toObj(obj);
                reward_list.push_back(obj);
            }
            obj.push_back( Pair("get", reward_list) );
            list.push_back(obj);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("level", cp->m_level) );
    robj.push_back( Pair("contribution", cm->m_contribution) );
    return HC_SUCCESS;
}

//领取公会每日宝箱
int guildMgr::getGuildDailyBoxReward(int cid, int gid, int boxid, json_spirit::Object& robj)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm || !cm->cdata.get())
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    int state = cm->cdata->queryExtraData(char_data_type_daily,char_data_daily_guild_box);
    if (state > 0)
        return HC_ERROR;
    std::map<int, boost::shared_ptr<baseGuildBox> >::iterator it = m_guild_dailybox.find(boxid);
    if (it != m_guild_dailybox.end())
    {
        if (it->second.get() && it->second->needlevel == cp->m_level && cm->m_contribution >= it->second->needcontribution)
        {
            std::list<Item> items;
            items = it->second->reward;
            if (!cm->cdata->m_bag.hasSlot(itemlistNeedBagSlot(items)))
            {
                return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
            }
            giveLoots(cm->cdata.get(), items, NULL, &robj, true, loot_guild_box);
            cm->cdata->setExtraData(char_data_type_daily,char_data_daily_guild_box,1);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//查询公会成就宝箱
int guildMgr::getGuildBoxList(int cid, int gid, int page, int nums_per_page, json_spirit::Object& robj)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;

    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm || !cm->cdata.get() || !cm->cdata->m_guild_forever_data.get())
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    json_spirit::Array elist;
    std::list<boost::shared_ptr<GuildEvent> >::iterator it_e = m_event_list.begin();
    while (it_e != m_event_list.end())
    {
        if (it_e->get())
        {
            GuildEvent* pe = it_e->get();
            json_spirit::Object obj;
            obj.push_back( Pair("id", pe->cid) );
            obj.push_back( Pair("name", pe->name) );
            obj.push_back( Pair("memo", pe->message) );
            obj.push_back( Pair("time", time(NULL)-pe->inputTime) );
            elist.push_back(obj);
        }
        ++it_e;
    }
    robj.push_back( Pair("elist", elist) );
    CharGuildForeverData* cgfd = cm->cdata->m_guild_forever_data.get();
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<baseGuildBox> >::iterator it = m_guild_box.begin();
    while (it != m_guild_box.end())
    {
        if (it->second.get())
        {
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", it->first) );
                obj.push_back( Pair("needlevel", it->second->needlevel) );
                obj.push_back( Pair("needcontribution", it->second->needcontribution) );
                obj.push_back( Pair("state", cgfd->m_guild_box_state[it->first]) );
                json_spirit::Array reward_list;
                for (std::list<Item>::iterator it_i = it->second->reward.begin(); it_i != it->second->reward.end(); ++it_i)
                {
                    Item& item = *it_i;
                    json_spirit::Object obj;
                    item.toObj(obj);
                    reward_list.push_back(obj);
                }
                obj.push_back( Pair("get", reward_list) );
                list.push_back(obj);
            }
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("level", cp->m_level) );
    robj.push_back( Pair("contribution", cm->m_contribution) );
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//领取公会成就宝箱
int guildMgr::getGuildBoxReward(int cid, int gid, int boxid, json_spirit::Object& robj)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm || !cm->cdata.get() || !cm->cdata->m_guild_forever_data.get())
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    CharGuildForeverData* cgfd = cm->cdata->m_guild_forever_data.get();
    if (cgfd->m_guild_box_state[boxid] > 0)
        return HC_ERROR;
    std::map<int, boost::shared_ptr<baseGuildBox> >::iterator it = m_guild_box.find(boxid);
    if (it != m_guild_box.end())
    {
        if (it->second.get() && cp->m_level >= it->second->needlevel && cm->m_contribution >= it->second->needcontribution)
        {
            std::list<Item> items;
            items = it->second->reward;
            if (!cm->cdata->m_bag.hasSlot(itemlistNeedBagSlot(items)))
            {
                return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
            }
            giveLoots(cm->cdata.get(), items, NULL, &robj, true, loot_guild_box);
            cgfd->m_guild_box_state[boxid] = 1;
            cgfd->save();
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

boost::shared_ptr<baseGuildSkill> guildMgr::getGuildSkill(int type, int level)
{
    std::map<std::pair<int,int>, boost::shared_ptr<baseGuildSkill>, compare_pair>::iterator it = m_guild_skills.find(std::make_pair(type,level));
    if (it != m_guild_skills.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<baseGuildSkill> bgs;
    return bgs;
}

//查询公会技能加成
int guildMgr::getGuildSkillList(int cid, int gid, json_spirit::Object& robj)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }

    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm || !cm->cdata.get() || !cm->cdata->m_guild_forever_data.get())
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    CharGuildForeverData* cgfd = cm->cdata->m_guild_forever_data.get();
    json_spirit::Array list;
    for (int i = 0; i < 4; ++i)
    {
        json_spirit::Object obj;
        int type = i+1;
        int level = cgfd->m_guild_skill_level[i];
        obj.push_back( Pair("type", type) );
        boost::shared_ptr<baseGuildSkill> bgs = getGuildSkill(type, level);
        if (bgs.get())
        {
            obj.push_back( Pair("add", bgs->add) );
        }
        if (level < iMaxGuildLevel * 2)
        {
            obj.push_back( Pair("next_level", level+1) );
            bgs = getGuildSkill(type, level+1);
            if (bgs.get())
            {
                obj.push_back( Pair("needcontribution", bgs->needcontribution) );
                obj.push_back( Pair("next_add", bgs->add) );
            }
        }
        list.push_back(obj);
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("contribution", cm->m_contribution) );
    return HC_SUCCESS;
}

//升级公会技能
int guildMgr::upgradeGuildSkill(int cid, int gid, int type)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    if (type < 1 || type > 4)
        return HC_ERROR;
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm || !cm->cdata.get() || !cm->cdata->m_guild_forever_data.get())
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    CharData* pc = cm->cdata.get();
    CharGuildForeverData* cgfd = cm->cdata->m_guild_forever_data.get();
    int level = cgfd->m_guild_skill_level[type-1];
    //技能上限公会等级*2
    if (level >= cp->m_level * 2)
        return HC_ERROR;
    boost::shared_ptr<baseGuildSkill> bgs = getGuildSkill(type, level+1);
    if (bgs.get())
    {
        if (cm->m_contribution < bgs->needcontribution)
            return HC_ERROR;
        cgfd->m_guild_skill_level[type-1] = level+1;
        cgfd->save();
        //英雄加成生效
        pc->m_heros.updateAttribute();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//查询公会活动所需等级
int guildMgr::getGuildActionLevel(int id)
{
    int needlevel = 0;
    for (std::vector<baseGuildAction>::iterator it = m_base_guild_action.begin(); it != m_base_guild_action.end(); ++it)
    {
        if (it->id == id)
        {
            needlevel = it->needlevel;
            break;
        }
    }
    return needlevel;
}

//查询公会活动页面
int guildMgr::getGuildActionList(CharData* pc, json_spirit::Object& robj)
{
    json_spirit::Array list;
    for (std::vector<baseGuildAction>::iterator it = m_base_guild_action.begin(); it != m_base_guild_action.end(); ++it)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("id", it->id) );
        obj.push_back( Pair("spic", it->id) );
        obj.push_back( Pair("needlevel", it->needlevel) );
        obj.push_back( Pair("name", it->name) );
        obj.push_back( Pair("memo", it->memo) );
        list.push_back(obj);
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

void guildMgr::addDonateEvent(int cid, const std::string& name, int extra)
{
    GuildEvent* ce = new GuildEvent;
    ce->cid = cid;
    ce->gid = 0;
    ce->inputTime = time(NULL);
    ce->name = name;
    ce->message = strGuildMsgDonateGold;
    str_replace(ce->message, "$N", LEX_CAST_STR(extra / 10));
    str_replace(ce->message, "$A", LEX_CAST_STR(extra));
    InsertSaveDb("insert into char_guild_donate_event (cid,name,msg,input) values (" + LEX_CAST_STR(ce->cid) + ",'" + GetDb().safestr(ce->name)
                        + "','" + GetDb().safestr(ce->message) + "',unix_timestamp())");
    boost::shared_ptr<GuildEvent> ge(ce);
    m_event_list.insert(m_event_list.begin(), ge);
    while (m_event_list.size() >= 4)
    {
        m_event_list.pop_back();
    }
}

boost::shared_ptr<baseGuildMoshen> guildMgr::getGuildMoshen(int id)
{
    if (id >= 1 && id <= m_guild_moshen.size())
    {
        return m_guild_moshen[id-1];
    }
    boost::shared_ptr<baseGuildMoshen> p;
    p.reset();
    return p;
}

int guildMgr::getGuildMoshenInfo(int cid, int gid, int moshen_id, json_spirit::Object& robj)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    boost::shared_ptr<CharGuildData> cg = cp->getMember(cid);
    CharGuildData* cm = cg.get();
    if (NULL == cm || !cm->cdata.get() || !cm->cdata->m_guild_forever_data.get())
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    if (moshen_id > m_guild_moshen.size())
    {
        return HC_ERROR;
    }
    CharData* pc = cm->cdata.get();
    int cur_moshen_id = pc->queryExtraData(char_data_type_daily, char_data_daily_guild_moshen) + 1;
    if (moshen_id <= 0)
    {
        moshen_id = cur_moshen_id;
    }
    if (moshen_id > m_guild_moshen.size())
    {
        moshen_id = m_guild_moshen.size();
    }
    int moshen_state = pc->queryExtraData(char_data_type_daily, char_data_daily_guild_moshen_start + moshen_id);
    robj.push_back( Pair("state", moshen_state) );
    robj.push_back( Pair("moshen_id", cur_moshen_id) );
    robj.push_back( Pair("max_moshen_id", m_guild_moshen.size()) );
    std::string first_name = "";
    if (moshen_id <= cp->m_moshen_list.size())
    {
        int first_cid = cp->m_moshen_list[moshen_id-1];
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(first_cid);
        if (cdata.get())
        {
            first_name = cdata->m_name;
        }
    }
    robj.push_back( Pair("first_name", first_name) );
    if (cp->m_moshen_list.size())
    {
        int best_cid = cp->m_moshen_list[cp->m_moshen_list.size()-1];
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(best_cid);
        if (cdata.get())
        {
            json_spirit::Object best_obj;
            best_obj.push_back( Pair("name", cdata->m_name) );
            best_obj.push_back( Pair("level", cdata->m_level) );
            best_obj.push_back( Pair("spic", cdata->m_spic) );
            best_obj.push_back( Pair("moshen_id", cp->m_moshen_list.size()) );
            robj.push_back( Pair("best_obj", best_obj) );
        }
    }
    if (m_guild_moshen[moshen_id-1].get())
    {
        json_spirit::Object obj;
        m_guild_moshen[moshen_id-1]->toObj(obj);
        robj.push_back( Pair("moshen_obj", obj) );
    }
    return HC_SUCCESS;
}

int guildMgr::combatResult(chessCombat* pCombat)    //战斗结束
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (COMBAT_TYPE_GUILD_MOSHEN != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_players[0].m_cid);
    if (!cdata.get())
    {
        ERR();
        return HC_ERROR;
    }
    Guild* cp = Singleton<guildMgr>::Instance().getGuild(cdata->GetGuildId());
    if (!cp)
    {
        return HC_ERROR_NOT_JOIN_GUILD;
    }
    boost::shared_ptr<baseGuildMoshen> bs = getGuildMoshen(pCombat->m_data_id);
    if (!bs.get())
    {
        ERR();
        return HC_ERROR;
    }
    cout << "guildMgr::combatResult   " << pCombat->m_combat_id << ",reuslt=" << pCombat->m_result << endl;
    cdata->m_tasks.updateTask(GOAL_DAILY_GUILD_MOSHEN, 0, 1);
    if (pCombat->m_result == COMBAT_RESULT_ATTACK_WIN)
    {
        std::string msg = "";//广播
        if (bs->m_id >= 5 && bs->m_id < 20)
        {
            msg = strSystemPassGuildMoshen;
            str_replace(msg, "$N", LEX_CAST_STR(bs->m_id));
        }
        else if (bs->m_id == 20)
        {
            msg = strSystemPassGuildMoshen2;
        }
        if (msg != "")
        {
            str_replace(msg, "$W", MakeCharNameLink(cdata->m_name,cdata->m_nick.get_string()));
            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }

        //设置玩家今日魔神状态
        int tmp = cdata->queryExtraData(char_data_type_daily, char_data_daily_guild_moshen);
        if (tmp < bs->m_id)
        {
            cdata->setExtraData(char_data_type_daily, char_data_daily_guild_moshen, bs->m_id);
        }
        cdata->setExtraData(char_data_type_daily, char_data_daily_guild_moshen_start + bs->m_id, 1);
        //更新公会魔神状态
        if (bs->m_id > cp->m_moshen_list.size())
        {
            cp->m_moshen_list.push_back(cdata->m_id);
            cp->saveMoshenData();
        }
        pCombat->m_getItems = bs->m_Item_list;
        //给东西
        giveLoots(cdata, pCombat, true, loot_guild_moshen);
    }
    return HC_SUCCESS;
}

void guildMgr::setGuildExp(int gid, int level, int exp)
{
    Guild* cp = getGuild(gid);
    if (!cp)
    {
        return;
    }
    cp->m_level = level;
    cp->m_exp = exp;
    cp->save();
}

void guildMgr::setCharContribution(CharData& cdata, int contribute)
{
    if (cdata.GetGuildId())
    {
        cdata.m_guild_data->m_contribution = contribute;
        cdata.m_guild_data->save();
    }
}

