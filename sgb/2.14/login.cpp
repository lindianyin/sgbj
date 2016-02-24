
#include "stdafx.h"

#include "login.h"
#include "data.h"
#include "spls_errcode.h"

//在线用户
#include "utils_all.h"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include "net.h"
#include "json_spirit_writer_template.h"

#include "utils_lang.h"
#include "text_filter.h"
#include "statistics.h"

volatile int g_print_debug_info = 0;

volatile int g_enable_debug_cmd_line = 0;

#include<boost/tokenizer.hpp>
#include "campRace.h"
#include "boss.h"
#include "spls_race.h"
#include "corps.h"
#include "guard.h"
#include "groupCombat.h"
#include "singleton.h"
#include "new_ranking.h"
#include "cost_feedback.h"
#include "corpsFighting.hpp"
#include "buff.h"

//平台用户验证
extern int platformAuthAccount(const std::string& qid,
                          const std::string& qname,
                          int union_id,
                          const std::string& server_id,
                          time_t _time,
                          int iAdult,
                          const std::string& extra1,
                          const std::string& extra2,
                          const std::string& sign,
                          std::string& account);
void InsertSaveDb(const std::string& sql);

using namespace std;
using namespace boost;
using namespace net;

extern int InsertActionWork(actionmessage& msg);
extern int InsertInternalActionWork(json_spirit::mObject& obj);

extern int dailyUpdate();

extern volatile int g_print_debug_info;

#define INFO(x) if (g_print_debug_info) cout<<__FILE__<<","<<__LINE__<<"-------->"<<x<<endl

/**************** 账号 ****************/

volatile uint64_t CharactorInfo::_refs = 0;

CharactorInfo::CharactorInfo(const CharactorInfo& c)
{
    ++CharactorInfo::_refs;
    m_cid = c.m_cid;
    m_level = c.m_level;
    m_spic = c.m_spic;
    m_state = c.m_state;    // 0 正常 1 待删除
    m_deleteTime = c.m_deleteTime;
    m_lastlogin = c.m_lastlogin;
    m_name = c.m_name;
}

volatile uint64_t OnlineUser::_refs = 0;

OnlineUser::OnlineUser(const std::string& qid, const std::string& account, int union_id, const std::string& server_id, net::session_ptr h)
:m_qid(qid)
,m_account(account)
,m_server_id(server_id)
,m_union_id(union_id)
,m_ipaddress(h->remote_ip())
,m_sockethandle(h)
,m_logintime(time(NULL))
{
    ++OnlineUser::_refs;
    m_onlineCharactor.reset();
    m_state = 0;
    INFO("create online user:"<<account<<endl);
}

OnlineUser::~OnlineUser()
{
    --OnlineUser::_refs;
    m_sockethandle.reset();
    INFO("delete online user:"<<m_account<<endl);
}

const std::string& OnlineUser::GetRemoteAddress()
{
    return m_ipaddress;
}

//选择角色登录
int OnlineUser::Login(uint64_t cid)
{
    INFO("OnlineUser::Login("<<cid<<")");
    //原来有登陆
    if (m_onlineCharactor.get())
    {
        GeneralDataMgr::getInstance()->CharactorLogout(m_onlineCharactor);
    }
    m_onlineCharactor.reset();

    //角色信息还未返回
    if (0 == m_state)
    {
        ERR();
        cout<<"!!! OnlineUser::Login(),charlist not return,account:"<<m_account<<",ip:"<<m_ipaddress<<",cid:"<<cid<<endl;
        return HC_ERROR;
    }
    m_onlineCharactor = GeneralDataMgr::getInstance()->CreateOnlineCharactor(shared_from_this(), cid);
    if (m_onlineCharactor.get())
    {
        GeneralDataMgr::getInstance()->CharactorLogin(m_onlineCharactor);
        return HC_SUCCESS;
    }
    else
    {
        ERR();
        cout<<"cid"<<cid<<endl;
        return HC_ERROR;
    }
}

boost::shared_ptr<OnlineUser> OnlineUser::getAccount()
{
    return shared_from_this();
}

volatile uint64_t OnlineCharactor::_refs = 0;

/****************** 角色 **********************/
OnlineCharactor::OnlineCharactor(boost::shared_ptr<OnlineUser> account, int cid)
:m_cid(cid)
{
    m_account = account;
    if (account.get())
    {
        m_sockethandle = account->m_sockethandle;
    }
    ++OnlineCharactor::_refs;
    m_gag_end_time = 0;
    //Reset();
    INFO("create charactor "<<m_account->m_account<<"->"<<m_cid);
    m_charactor = GeneralDataMgr::getInstance()->GetCharData(cid);
    //reload();
}

OnlineCharactor::~OnlineCharactor()
{
    --OnlineCharactor::_refs;
    INFO("delete charactor "<<m_account->m_account<<"->"<<m_cid);
}

//int OnlineCharactor::reload()
//{
//    return GetCharInfo(m_cid, m_camp, m_guildId, m_name);
//}

int OnlineCharactor::GuildChat(const std::string& msg)
{
    if (m_charactor.get())
    {
        //被禁言
        if (m_gag_end_time > time(NULL))
        {
            return HC_ERROR_FORBIDEN_CHAT;
        }
        int guild_id = m_charactor->GetGuildId();
        if (guild_id > 0)
        {
            boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(guild_id);
            if (gch.get())
            {
                return gch->Chat(m_charactor->m_name, msg, 0, m_charactor->m_gender, m_charactor->getChangeSpic(), m_charactor->m_nick.get_string());
            }
        }
        return HC_ERROR_NOT_JOIN_JT;        
    }
    else
    {
        return HC_ERROR_LOGIN_FIRST;
    }
}

int OnlineCharactor::CampChat(const std::string& msg)
{
    if (m_charactor.get())
    {
        //被禁言
        if (m_gag_end_time > time(NULL))
        {
            return HC_ERROR_FORBIDEN_CHAT;
        }
        boost::shared_ptr<ChatChannel> ach = GeneralDataMgr::getInstance()->GetCampChannel(m_charactor->m_camp);
        if (ach.get())
        {
            std::string _msg = ach->genChatMsg(m_charactor->m_name, msg);
            ach->BroadMsg(_msg);
            if (m_charactor->m_camp)
            {
                boost::shared_ptr<ChatChannel> newbieChannel = GeneralDataMgr::getInstance()->GetCampChannel(0);
                if (newbieChannel.get())
                {
                    newbieChannel->BroadMsg(_msg);
                }
            }
            else
            {
                boost::shared_ptr<ChatChannel> channel1 = GeneralDataMgr::getInstance()->GetCampChannel(1);
                if (channel1.get())
                {
                    channel1->BroadMsg(_msg);
                }
                boost::shared_ptr<ChatChannel> channel2 = GeneralDataMgr::getInstance()->GetCampChannel(2);
                if (channel2.get())
                {
                    channel2->BroadMsg(_msg);
                }
            }
            //return ach->Chat(m_charactor->m_name, msg);
        }
        return HC_SUCCESS;
    }
    return HC_ERROR_LOGIN_FIRST;
}

int OnlineCharactor::WorldChat(const std::string& msg, bool needgold)
{
    if (!m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    //被禁言
    if (m_gag_end_time > time(NULL))
    {
        return HC_ERROR_FORBIDEN_CHAT;
    }

/*   <xxx>表示要自己填入参数
    /heroup           所有英雄升到最高等级
    /hero <id>         加英雄
    /ling             加令
    /gold             加金币
    /silver         加银币
    /skill <id> <level> 加技能或者删除技能（level 0）
    /skill all <level> 全体技能设置到指定等级
    /stone             加强化石
    /zb             加5件顶级装备，
    /zb <id>         加指定装备
    /vip             升到vip10， 
    /vip <level>        设定指定vip等级
    /state <pos> <state> <level>    设置第几个位置的状态，等级
    /season <1-4>    //设置季节
    /farm            屯田任务立即完成
    /openboss <id>    开启boss战
    /closeboss <id>    关闭boss战斗
    /memdump
    /md5test "898989uiuij"
    /partytest
    /task xxx        设置任务进度
    /level xxx        设置角色等级
    /updatea 1-10 1,0
    /adda 1-10 1,0
    /removea 1-10
    
*/    
    //调试开关
    if (g_enable_debug_cmd_line > 0 && !strncmp("/", msg.c_str(), 1))
    {
        bool validcmd = true;
        using namespace boost;
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        boost::char_separator<char> sep(" ");
        tokenizer tok(msg, sep);
        tokenizer::iterator it = tok.begin();
        if (it != tok.end())
        {
            if ("/reload" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    if (*it == "bs")
                    {
                        json_spirit::mObject obj;
                        obj["cmd"] = "reload";
                        obj["type"] = 1;
                        InsertInternalActionWork(obj);
                        return HC_SUCCESS;
                    }
                    else if ("gs" == *it)
                    {
                        json_spirit::mObject obj;
                        obj["cmd"] = "reload";
                        obj["type"] = 2;
                        InsertInternalActionWork(obj);
                        return HC_SUCCESS;
                    }
                }
                else
                {
                    json_spirit::mObject obj;
                    obj["cmd"] = "reload";
                    InsertInternalActionWork(obj);
                    return HC_SUCCESS;
                }
            }
            else if ("/gold" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    m_charactor->addGold(atoi(it->c_str()));
                }
                else
                {
                    m_charactor->addGold(100000);
                }
                m_charactor->NotifyCharData();
                return HC_SUCCESS;
            }
            else if ("/ling" == *it)
            {
                m_charactor->ling(10000);
                m_charactor->NotifyCharData();
                return HC_SUCCESS;
            }
            else if ("/silver" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    m_charactor->addSilver(atoi(it->c_str()));
                }
                else
                {
                    m_charactor->addSilver(10000000);
                }
                m_charactor->NotifyCharData();
                return HC_SUCCESS;
            }
            else if ("/daoju" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int tid = atoi(it->c_str());
                    if (tid > 0)
                    {
                        ++it;
                        if (it != tok.end())
                        {
                            int num = atoi(it->c_str());
                            m_charactor->addTreasure(tid,num);
                            return HC_SUCCESS;
                        }
                    }
                }
            }
            else if ("/zb" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    cout<<" gm cmd /zb "<<*it<<endl;
                    std::string extraMsg = "";
                    m_charactor->addEquipt(atoi(it->c_str()), extraMsg);
                    return HC_SUCCESS;
                }
                else
                {
                    std::string extraMsg = "";
                    m_charactor->addEquipt(5, extraMsg);
                    m_charactor->addEquipt(10, extraMsg);
                    m_charactor->addEquipt(15, extraMsg);
                    m_charactor->addEquipt(20, extraMsg);
                    m_charactor->addEquipt(25, extraMsg);
                    return HC_SUCCESS;
                }
            }
            else if ("/vip" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int vip = atoi(it->c_str());
                    if (vip < 0)
                    {
                        vip = 1;
                    }
                    else if (vip > 10)
                    {
                        vip = 10;
                    }
                    cout<<" gm cmd /vip "<<*it<<endl;
                    m_charactor->m_vip = vip;
                    return HC_SUCCESS;
                }
                else
                {
                    m_charactor->m_vip = 10;
                    return HC_SUCCESS;
                }
            }
            else if (*it == "/farm")
            {
                farmMgr::getInstance()->updateAll(m_charactor->m_id);
                return HC_SUCCESS;
            }
            else if (*it == "/reset")
            {
                dailyUpdate();
                return HC_SUCCESS;
            }
            else if (*it == "/weekreset")
            {
                guardMgr::getInstance()->raceAwards();
                newRankings::getInstance()->RankingsReward();
                return HC_SUCCESS;
            }
            else if (*it == "/racedebug")
            {
                RaceMgr::getInstance()->updateAll();
                return HC_SUCCESS;
            }
            else if ("/hero" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    m_charactor->m_generals.Add(atoi(it->c_str()));
                    return HC_SUCCESS;
                }
            }
            else if ("/heroup" == *it)
            {
                //全部英雄升级到顶级
                std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = m_charactor->m_generals.m_generals.begin();
                while (it != m_charactor->m_generals.m_generals.end())
                {
                    if (it->second.get())
                    {
                        while (it->second->m_level < m_charactor->m_level)
                        {
                            it->second->Levelup(m_charactor->m_level);
                        }                        
                    }
                    ++it;
                }
                return HC_SUCCESS;
            }
            else if ("/levelup" == *it)
            {
                int level = iMaxCharLevel;
                ++it;
                if (it != tok.end())
                {
                    level = atoi(it->c_str());
                }
                GeneralDataMgr::getInstance()->updateTempo(m_charactor->m_id, level);
                return HC_SUCCESS;
            }
            else if ("/skill" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    if (*it == "all")
                    {
                        ++it;
                        if (it != tok.end())
                        {
                            int level = atoi(it->c_str());
                            for (int skill = 1; skill <= 21; ++skill)
                            {
                                m_charactor->setSkillLevel(skill, level);
                            }
                            return HC_SUCCESS;
                        }
                    }
                    else
                    {
                        int skill = atoi(it->c_str());
                        ++it;
                        if (it != tok.end() && skill >= 1 && skill <= 21)
                        {
                            int level = atoi(it->c_str());
                            m_charactor->setSkillLevel(skill, level);
                            return HC_SUCCESS;
                        }
                    }
                }
            }
            else if ("/debug" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    g_print_debug_info = atoi(it->c_str());
                    return HC_SUCCESS;
                }
                g_print_debug_info = 1;
                return HC_SUCCESS;
            }
            else if ("/season" == *it)
            {
                //季节变化
                GeneralDataMgr::getInstance()->updateSeason();
                return HC_SUCCESS;
            }
            else if ("/reset tired" == *it)
            {
                farmMgr::getInstance()->resetAll();
                return HC_SUCCESS;
            }
            else if ("/prestige" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int prestige = atoi(it->c_str());
                    m_charactor->addPrestige(prestige);
                }
                else
                {
                    m_charactor->addPrestige(1000);
                }
                return HC_SUCCESS;
            }
            else if ("/openboss" == *it)
            {
                int boss = 1;
                int mins = 120;
                ++it;
                if (it != tok.end())
                {
                    boss = atoi(it->c_str());
                    ++it;
                    if (it != tok.end())
                    {
                        mins = atoi(it->c_str());
                    }
                }
                bossMgr::getInstance()->openBoss(boss, mins);
                return HC_SUCCESS;
            }
            else if ("/closeboss" == *it)
            {
                int boss = 1;
                ++it;
                if (it != tok.end())
                {
                    boss = atoi(it->c_str());
                }
                bossMgr::getInstance()->closeBoss(boss);
                return HC_SUCCESS;
            }
            else if ("/attackboss" == *it)
            {
                int boss = 1;
                ++it;
                if (it != tok.end())
                {
                    boss = atoi(it->c_str());                    
                }
                json_spirit::Object robj;
                bossMgr::getInstance()->Attack(m_charactor->m_id, boss, 1, robj);
                return HC_SUCCESS;
            }
            else if ("/query" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    if (*it == "online")
                    {
                        int total_online = GeneralDataMgr::getInstance()->getTotalOnline(false);
                        std::string msg = "{\"cmd\":\"chat\",\"ctype\":4,\"s\":200,\"f\":\"system\",\"m\":\"total online:" + LEX_CAST_STR(total_online) + "\"}";
                        Send(msg);
                        return HC_SUCCESS;
                    }
                }
            }
            else if ("/md5test" == *it)
            {                
                ++it;
                if (it != tok.end())
                {
                    std::string account = "";
                    int ret = platformAuthAccount("qid", "qname", 0, "server_id", time(NULL), 0, "extra1", "extra2", *it, account);
                    std::string msg = "{\"cmd\":\"chat\",\"ctype\":4,\"s\":200,\"f\":\"system\",\"m\":\"account:" + LEX_CAST_STR(account) + ", return " + LEX_CAST_STR(ret) + "\"}";
                    Send(msg);
                    return HC_SUCCESS;
                }
            }
            else if ("/opencamprace" == *it)
            {
                ++it;
                int last_mins = 30;
                if (it != tok.end())
                {
                    last_mins = atoi(it->c_str());
                }
                campRaceMgr::getInstance()->open(last_mins);
                return HC_SUCCESS;
            }
            else if ("/camprace" == *it)
            {
                campRaceMgr::getInstance()->Race();
                return HC_SUCCESS;
            }
            else if ("/partytest" == *it)
            {                
                corpsMgr::getInstance()->inviteSomeone(m_charactor.get(), 1);
                return HC_SUCCESS;
            }
            else if ("/guardtest" == *it)
            {                
                guardMgr::getInstance()->guardtest(m_charactor->m_id);
                return HC_SUCCESS;
            }
            else if ("/setdebug" == *it)
            {
                boost::shared_ptr<OnlineCharactor> oc = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_charactor->m_name);
                if (oc.get() && oc->m_sockethandle.get())
                {
                    oc->m_sockethandle->is_debug_ = true;
                }
                return HC_SUCCESS;
            }
            else if ("/task" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    m_charactor->m_task.tid = atoi(it->c_str());
                    m_charactor->m_task._task = taskMgr::getInstance()->getTask(m_charactor->m_task.tid);
                }
                else
                {
                }
                return HC_SUCCESS;
            }
            else if ("/level" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int level = atoi(it->c_str());
                    GeneralDataMgr::getInstance()->updateTempo(m_charactor->m_id, level);
                }
                else
                {
                }
                return HC_SUCCESS;
            }
            else if ("/groupcopy" == *it)
            {
                int copyId = 1;
                ++it;
                if (it != tok.end())
                {
                    copyId = atoi(it->c_str());
                }
                int _bfriend[iMaxGroupCopyMembers]={0};
                json_spirit::Object robj;
                groupCombatMgr::getInstance()->AttackGroupCopy(1, copyId, &_bfriend[0]);
                return HC_SUCCESS;
            }
            else if ("/notifyWallow" == *it)
            {
                int state = 1;
                ++it;
                if (it != tok.end())
                {
                    state = atoi(it->c_str());
                }
                switch (state)
                {
                    default:
                    case 0:
                        Send("{\"cmd\":\"notifyWallow\",\"state\":0,\"time\":0,\"s\":200}");
                        break;
                    case 1:
                        Send("{\"cmd\":\"notifyWallow\",\"state\":1,\"time\":1,\"s\":200}");
                        break;
                    case 2:
                        Send("{\"cmd\":\"notifyWallow\",\"state\":1,\"time\":2,\"s\":200}");
                        break;
                    case 3:
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":3,\"s\":200}");
                        break;
                    case 4:
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":3.5,\"s\":200}");
                        break;
                    case 5:
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":4,\"s\":200}");
                        break;
                    case 6:
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":4.5,\"s\":200}");
                        break;
                    case 7:
                        Send("{\"cmd\":\"notifyWallow\",\"state\":3,\"time\":5,\"s\":200}");
                        break;
                }
                return HC_SUCCESS;
            }
            else if ("/updatea" == *it)
            {
                int type = 1;
                int state = 1;
                ++it;
                if (it != tok.end())
                {
                    type = atoi(it->c_str());
                    ++it;
                    if (it != tok.end())
                    {
                        state = atoi(it->c_str());
                    }
                }
                Send("{\"type\":" + LEX_CAST_STR(type) + ",\"active\":" + LEX_CAST_STR(state) + ",\"cmd\":\"updateAction\",\"s\":200}");
                return HC_SUCCESS;
            }
            else if ("/adda" == *it)
            {
                int type = 1;
                int state = 1;
                ++it;
                if (it != tok.end())
                {
                    type = atoi(it->c_str());
                    ++it;
                    if (it != tok.end())
                    {
                        state = atoi(it->c_str());
                    }
                }
                Send("{\"type\":" + LEX_CAST_STR(type) + ",\"active\":" + LEX_CAST_STR(state) + ",\"cmd\":\"addAction\",\"s\":200}");
                return HC_SUCCESS;
            }
            else if ("/removea" == *it)
            {
                int type = 1;
                ++it;
                if (it != tok.end())
                {
                    type = atoi(it->c_str());
                }
                Send("{\"type\":" + LEX_CAST_STR(type) + ",\"cmd\":\"removeAction\",\"s\":200}");
                return HC_SUCCESS;
            }
            else if ("/baoshi" == *it)
            {
                for (int i = 1; i <= 5; ++i)
                {
                    m_charactor->giveBaoshi(i, 1, baoshi_admin);
                }
                if (m_account.get())
                {
                    json_spirit::mObject obj;
                    obj["cmd"] = "getStoreList";
                    
                    net::actionmessage act_msg(obj, 0);
                    act_msg.setsession(m_account->GetSocket());
                    InsertActionWork(act_msg);
                    return HC_SUCCESS;
                }
            }
            else if ("/sortbag" == *it)
            {
                m_charactor->m_bag.sortBag();
                if (m_account.get())
                {
                    json_spirit::mObject obj;
                    obj["cmd"] = "getStoreList";
                    
                    net::actionmessage act_msg(obj, 0);
                    act_msg.setsession(m_account->GetSocket());
                    InsertActionWork(act_msg);
                    return HC_SUCCESS;
                }
            }
            else if ("/cmd" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    if (m_account.get())
                    {
                        json_spirit::mObject obj;
                        obj["cmd"] = *it;
                        
                        net::actionmessage act_msg(obj, 0);
                        act_msg.setsession(m_account->GetSocket());
                        InsertActionWork(act_msg);
                    }
                }
            }
            else if ("/ymsj" == *it)
            {
                if (m_charactor->m_corps_member.get())
                {
                    splsCorps* cp = corpsMgr::getInstance()->findCorps(m_charactor->m_corps_member->corps);
                    if (cp)
                    {
                        if (cp->m_ymsj.next_time > time(NULL))
                        {
                            cp->m_ymsj.next_time = 0;
                        }
                    }
                }
            }
            else if ("/additem" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int type = atoi(it->c_str());
                    int type2 = 1;
                    ++it;
                    if (it != tok.end())
                    {
                        type2 = atoi(it->c_str());
                    }
                    switch (type)
                    {
                        case iItem_type_gem:
                            {
                                boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(type2);
                                if (!bt.get() || bt->currency)
                                {
                                    break;
                                }
                                //增加道具
                                boost::shared_ptr<iItem> t;
                                uint32_t id = GeneralDataMgr::getInstance()->newGemId();
                                t.reset(dynamic_cast<iItem*>(new Gem((uint16_t)iItem_type_gem, type2, id, 10)));
                                uint8_t slot = m_charactor->m_bag.addItem(t);

                                if (slot > 0)
                                {
                                    //保存数据库
                                    InsertSaveDb("insert into char_treasures (id,cid,tid,nums,slot) value ("
                                        + LEX_CAST_STR(id) + ","
                                        + LEX_CAST_STR(m_charactor->m_id) + ","
                                        + LEX_CAST_STR(type2) + ","
                                        + LEX_CAST_STR(t->getCount()) + ","
                                        + LEX_CAST_STR((int)(slot)) + ")");
                                    t->unsetChanged();
                                }
                            }
                            break;
                        default:
                        case iItem_type_baoshi:
                            {
                                if (m_charactor->m_bag.isFull())
                                {
                                    return HC_ERROR_BAG_FULL;
                                }
                                boost::shared_ptr<iItem> bs = Singleton<newBaoshiMgr>::Instance().cloneBaoshi(type2, 1, 1);
                                if (!bs.get())
                                {
                                    return HC_ERROR;
                                }
                                m_charactor->m_bag.addItem(bs);
                                
                                newBaoshi* pb = dynamic_cast<newBaoshi*>(bs.get());
                                pb->Save();
                            }
                            break;
                    }
                    if (m_account.get())
                    {
                        json_spirit::mObject obj;
                        obj["cmd"] = "getStoreList";
                        
                        net::actionmessage act_msg(obj, 0);
                        act_msg.setsession(m_account->GetSocket());
                        InsertActionWork(act_msg);
                    }
                    return HC_SUCCESS;
                }
            }
            else if ("/debugFeedback" == *it)
            {
                ++it;
                int day = 1;
                if (it != tok.end())
                {
                    day = atoi(it->c_str());
                    Singleton<cost_feedback_event>::Instance().debugSetDay(day);
                    return HC_SUCCESS;
                }
            }
            else if ("/jtzStart" == *it)
            {
                //Singleton<corpsFightingMgr>::Instance().openSignup();
                Singleton<corpsFightingMgr>::Instance().start();
                return HC_SUCCESS;
            }
            else if ("/jtzOpen" == *it)
            {
                Singleton<corpsFightingMgr>::Instance().openSignup();
                return HC_SUCCESS;
            }
            else if ("/jtBoss" == *it)
            {
                if (m_charactor->m_corps_member.get())
                {
                    bossMgr::getInstance()->resetJtBoss(m_charactor->m_corps_member->corps);
                }
                return HC_SUCCESS;
            }
            else if ("/clearBuff" == *it)
            {
                m_charactor->m_Buffs.clearBuff();
                return HC_SUCCESS;
            }
            else
            {
                if (0 == strncmp(it->c_str(), "/{", 2))
                {
                    validcmd = true;
                }
                else
                {
                    validcmd = false;
                }
            }
        }
        else
        {
            validcmd = false;
        }
        if (validcmd && m_account.get())
        {
            json_spirit::mValue value;
            std::string newmsg = msg;
            str_replace_all(newmsg, strLeft_p, "{");
            str_replace_all(newmsg, strRight_p, "}");
            str_replace_all(newmsg, "&quot;", "\"");
            json_spirit::read(newmsg.substr(1), value);
            if (value.type() != json_spirit::obj_type)
            {
                cout<<"recieve debug cmd:"<<newmsg<<endl;
                std::string msg = "{\"cmd\":\"chat\",\"ctype\":4,\"s\":200,\"f\":\"system\",\"m\":\"unknow cmd!\"}";
                Send(msg);
                return HC_SUCCESS;
            }
            json_spirit::mObject obj = value.get_obj();
            net::actionmessage act_msg(obj, 0);
            act_msg.setsession(m_account->GetSocket());
            InsertActionWork(act_msg);
            Send("{\"cmd\":\"chat\",\"ctype\":4,\"s\":200,\"f\":\"system\",\"m\":\"OK!\"}");
            return HC_SUCCESS;
        }
    }

    //主将等级要求
    if (needgold && m_charactor->m_level < iWorldChatLevel)
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    //世界聊天的vip等级要求及金币消耗
    if (m_charactor->m_vip < iWorldChatVipLevel && needgold)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    if (iWorldChatGoldCost && needgold)
    {
        if (m_charactor->addGold(-iWorldChatGoldCost) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        m_charactor->NotifyCharData();
        //金币消耗统计
        add_statistics_of_gold_cost(m_charactor->m_id, m_charactor->m_ip_address, iWorldChatGoldCost, gold_cost_for_world_chat, m_charactor->m_union_id, m_charactor->m_server_id);
    }
    boost::shared_ptr<ChatChannel> wch = GeneralDataMgr::getInstance()->GetWorldChannel();
    if (wch.get())
    {
        return wch->Chat(m_charactor->m_name, msg, 0, m_charactor->m_gender, m_charactor->getChangeSpic(), m_charactor->m_nick.get_string());
    }
    return HC_ERROR_LOGIN_FIRST;
}

int OnlineCharactor::Tell(const std::string& who, const std::string& msg, boost::shared_ptr<OnlineCharactor>& toChar)
{
    if (m_charactor.get())
    {
        //被禁言
        if (m_gag_end_time > time(NULL))
        {
            return HC_ERROR_FORBIDEN_CHAT;
        }
        toChar = GeneralDataMgr::getInstance()->GetOnlineCharactor(who);
        if (toChar.get())
        {
            static std::string startmsg = "{\"cmd\":\"chat\",\"ctype\":4,\"s\":200,\"f\":\"";
            std::string sendmsg = startmsg + m_charactor->m_name + "\""
                                    + ",\"gd\":" + boost::lexical_cast<std::string>(m_charactor->m_gender)
                                    + ",\"spic\":" + boost::lexical_cast<std::string>(m_charactor->getChangeSpic())
                                    +",\"m\":\"" + json_spirit::add_esc_chars<std::string>(msg, true) + "\"}";
            toChar->Send(sendmsg);
            return HC_SUCCESS;
        }
        else
        {
            return HC_ERROR_CHAR_NOT_ONLINE;
        }
    }
    else
    {
        return HC_ERROR_LOGIN_FIRST;
    }
}

int OnlineCharactor::Send(const std::string &what)
{
    //INFO("send to charactor "<<m_cid<<":"<<what);
    if (m_sockethandle.get())
    {
        m_sockethandle->send(what);
    }
    else
    {
        INFO("have no socket handle!");
    }
    return HC_SUCCESS;
}

//在线心跳
int OnlineCharactor::onHeartBeat()
{
    if (m_charactor.get())
    {
        CharData* pc = m_charactor.get();
        //通知祝贺列表，被祝贺列表
        if (pc->m_need_notify.size())
        {
            if (pc->m_need_notify[notify_msg_recv_congratulation])
            {
                Send("{\"cmd\":\"notify\",\"type\":8,\"nums\":" + LEX_CAST_STR(pc->m_need_notify[notify_msg_recv_congratulation]) + ",\"s\":200}");
            }
            if (pc->m_need_notify[notify_msg_new_congratulation])
            {
                Send("{\"cmd\":\"notify\",\"type\":9,\"nums\":" + LEX_CAST_STR(pc->m_need_notify[notify_msg_new_congratulation]) + ",\"s\":200}");
            }
            pc->m_need_notify.clear();
        }

        //检查沉迷
        if (pc->m_check_chenmi)
        {
            time_t total_secs = pc->m_chenmi_time + time(NULL) - pc->m_chenmi_start_time;
            uint64_t total_min = (total_secs)/60;

            //if (m_sockethandle.get() && m_sockethandle->is_debug_)
            //{
            //    cout<<"OnlineCharactor::onHeartBeat(),cid="<<pc->m_id<<",total min ="<<total_min<<",notify time="<<pc->m_notify_chenmi_time<<endl;
            //}

            if (total_min >= (uint64_t)pc->m_notify_chenmi_time)
            {
                /* 通知防沉迷信息
                ·使用者累计在线时间在3小时以内的，游戏收益正常。每累计在线时间满1小时，应提醒一次："您累计在线时间已满1小时。"
                ·累计在线时间满3小时时，应提醒："您累计在线时间已满3小时，请您下线休息，做适当身体活动。" 
                ·如果累计在线时间超过3小时进入第4－5小时，在开始进入时就应做出警示：
                    "您已经进入疲劳游戏时间，您的游戏收益将降为正常值的50％，请您尽快下线休息，做适当身体活动。"
                    此后，应每30分钟警示一次。 
                ·如果累计在线时间超过5小时进入第6小时，在开始进入时就应做出警示：
                    "您已进入不健康游戏时间，请您立即下线休息。如不下线，您的身体健康将受到损害，您的收益已降为零。"
                    此后，应每15分钟警示一次。                */
#ifndef TEST_SERVER
                if (total_min >= 300)
                {
                    pc->m_notify_chenmi_time = ((int)(total_min/15))*15 + 15;
                    Send("{\"cmd\":\"notifyWallow\",\"state\":3,\"time\":5,\"s\":200}");
                    if (total_min == 300)
                    {
                        Send("{\"cmd\":\"updateAction\",\"type\":10,\"spic\":3,\"s\":200,\"active\":1}");
                    }
                }
                else if (total_min >= 180)
                {
                    pc->m_notify_chenmi_time = ((int)(total_min/30))*30 + 30;
                    if (total_min >= 270)
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":4.5,\"s\":200}");
                    }
                    else if (total_min >= 240)
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":4,\"s\":200}");
                    }
                    else if (total_min >= 210)
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":3.5,\"s\":200}");
                    }
                    else
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":3,\"s\":200}");
                    }
                    if (total_min == 180)
                    {
                        Send("{\"cmd\":\"updateAction\",\"type\":10,\"spic\":2,\"s\":200,\"active\":1}");
                    }
                }
                else if (total_min >= 60)
                {
                    pc->m_notify_chenmi_time = ((int)(total_min/60))*60 + 60;
                    if (total_min >= 120)
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":1,\"time\":2,\"s\":200}");
                    }
                    else
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":1,\"time\":1,\"s\":200}");
                    }
                }
                else
                {
                    Send("{\"cmd\":\"notifyWallow\",\"state\":0,\"time\":0,\"s\":200}");
                    pc->m_notify_chenmi_time = 60;
                }
#else
                if (total_min >= 8)
                {
                    pc->m_notify_chenmi_time = 1;
                    Send("{\"cmd\":\"notifyWallow\",\"state\":3,\"time\":5,\"s\":200}");
                    if (total_min == 8)
                    {
                        Send("{\"cmd\":\"updateAction\",\"type\":10,\"spic\":3,\"s\":200,\"active\":1}");
                    }
                }
                else if (total_min >= 4)
                {
                    pc->m_notify_chenmi_time = 1;
                    if (total_min >= 7)
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":4.5,\"s\":200}");
                    }
                    else if (total_min >= 6)
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":4,\"s\":200}");
                    }
                    else if (total_min >= 5)
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":3.5,\"s\":200}");
                    }
                    else
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":2,\"time\":3,\"s\":200}");
                    }
                    if (total_min == 4)
                    {
                        Send("{\"cmd\":\"updateAction\",\"type\":10,\"spic\":2,\"s\":200,\"active\":1}");
                    }
                }
                else if (total_min >= 2)
                {
                    pc->m_notify_chenmi_time = 1;
                    if (total_min >= 3)
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":1,\"time\":2,\"s\":200}");
                    }
                    else
                    {
                        Send("{\"cmd\":\"notifyWallow\",\"state\":1,\"time\":1,\"s\":200}");
                    }
                }
                else
                {
                    Send("{\"cmd\":\"notifyWallow\",\"state\":0,\"time\":0,\"s\":200}");
                    pc->m_notify_chenmi_time = 1;
                }
#endif
                //if (m_sockethandle.get() && m_sockethandle->is_debug_)
                //{
                //    cout<<"next notify after "<<pc->m_notify_chenmi_time<<" min's"<<endl;
                //}
            }
        }
    }
    return HC_SUCCESS;
}

/******************* 聊天频道 *************************/

ChatChannel::ChatChannel()
:m_worker("unknow", m_broadmsg_que, 4)
{
    m_channelname = "unknow";
    m_prompt = "";
    m_channelid = 0;
    m_s = "unknow";
    //start();
}

ChatChannel::ChatChannel(const std::string & name, uint64_t id, const std::string& s)
:m_worker(name, m_broadmsg_que, 1)
{
    m_channelname = name;
    m_prompt = "";
    m_channelid = id;
    m_s = s;
    //start();
}

ChatChannel::~ChatChannel()
{
    //stop();
}

int ChatChannel::Add(boost::shared_ptr<OnlineCharactor> u)
{
    if (u.get())
    {
        INFO(m_channelname<<"@"<<m_channelid<<" "<<u->m_cid);
    }
    else
    {
        return -1;
    }
#ifdef CHANNEL_LOCK        
    boost::mutex::scoped_lock lock(channel_mutex_);
#endif
    for (std::list<boost::shared_ptr<OnlineCharactor> >::iterator pos = m_useridlist.begin(); pos != m_useridlist.end(); ++pos)
    {
        if ((*pos).get() == u.get())
        {
            return 0;
        }
    }
    m_useridlist.push_back(u);
    //u->m_world_channel = shared_from_this();
    return 0;
}

int ChatChannel::Remove(boost::shared_ptr<OnlineCharactor> u)
{
    if (u.get())
    {
        INFO(m_channelname<<"@"<<m_channelid<<" remove "<<u->m_cid);
    }
    else
    {
        return -1;
    }
#ifdef CHANNEL_LOCK    
    boost::mutex::scoped_lock lock(channel_mutex_);
#endif
    std::list<boost::shared_ptr<OnlineCharactor> >::iterator pos;
    for (pos = m_useridlist.begin(); pos != m_useridlist.end(); ++pos)
    {
        if ((*pos).get() == u.get())
        {
            //(*pos)->m_charactor.reset();
            (*pos).reset();
            m_useridlist.erase(pos);
            return 0;
        }
    }
    return -1;
}

int ChatChannel::Remove(int cid)
{
#ifdef CHANNEL_LOCK    
    boost::mutex::scoped_lock lock(channel_mutex_);
#endif
    std::list<boost::shared_ptr<OnlineCharactor> >::iterator pos;
    for (pos = m_useridlist.begin(); pos != m_useridlist.end(); ++pos)
    {
        if ((*pos).get() && (*pos)->m_cid == cid)
        {
            (*pos).reset();
            m_useridlist.erase(pos);
            return 0;
        }
    }
    return -1;
}

int ChatChannel::Clear()
{
    m_useridlist.clear();
    return HC_SUCCESS;
}

uint64_t ChatChannel::GetChannelId()
{
    return m_channelid;
}

int ChatChannel::BroadMsg(const std::string & msg)
{
    m_broadmsg_que.submitjob(msg);
    return HC_SUCCESS;
}

int ChatChannel::_BroadMsg(const std::string & msg)
{
    std::list<boost::shared_ptr<OnlineCharactor> >::iterator pos;
#ifdef CHANNEL_LOCK    
    boost::mutex::scoped_lock lock(channel_mutex_);
#endif
    INFO("####################### ChatChannel::BroadMsg to "<<m_useridlist.size());
    for (pos = m_useridlist.begin(); pos != m_useridlist.end(); ++pos)
    {
        boost::shared_ptr<OnlineCharactor>& account = *pos;
        if (account.get())
        {
            account->Send(msg);
        }
    }
    return HC_SUCCESS;
}

int ChatChannel::BroadMsg(const std::string & msg, int level)
{
    std::list<boost::shared_ptr<OnlineCharactor> >::iterator pos;
#ifdef CHANNEL_LOCK    
    boost::mutex::scoped_lock lock(channel_mutex_);
#endif
    INFO("####################### ChatChannel::BroadMsg to "<<m_useridlist.size());
    for (pos = m_useridlist.begin(); pos != m_useridlist.end(); ++pos)
    {
        boost::shared_ptr<OnlineCharactor>& account = *pos;
        if (account.get() && account->m_charactor.get() && account->m_charactor->m_level >= level)
        {
            account->Send(msg);
        }
    }
    return HC_SUCCESS;
}

int ChatChannel::Chat(const std::string &who, const std::string &what, int types, int gender, int mod, const std::string& nick)
{
    INFO(m_channelname<<"@"<<m_channelid<<":"<<who<<","<<what);
    std::string msg = what;
    //关键字过滤
    Forbid_word_replace::getInstance()->Filter(msg);
    if (types != 0)
    {
        msg = m_s + "\"f\":\"" + json_spirit::add_esc_chars<std::string>(who,true)
                + "\",\"m\":\"" + json_spirit::add_esc_chars<std::string>(msg, true)
                + "\",\"type\":" + boost::lexical_cast<std::string>(types)
                + ",\"gd\":" + boost::lexical_cast<std::string>(gender)
                + ",\"spic\":" + boost::lexical_cast<std::string>(mod)
                + ",\"nick\":" + nick
                + "}";
    }
    else
    {
        msg = m_s + "\"f\":\"" + json_spirit::add_esc_chars<std::string>(who,true)
                + "\",\"m\":\"" + json_spirit::add_esc_chars<std::string>(msg,true) +"\""
                + ",\"gd\":" + boost::lexical_cast<std::string>(gender)
                + ",\"spic\":" + boost::lexical_cast<std::string>(mod)
                + ",\"nick\":" + nick
                +"}";
    }
    m_broadmsg_que.submitjob(msg);
    //BroadMsg(msg);
    return HC_SUCCESS;
}

std::string ChatChannel::genChatMsg(const std::string& who, const std::string& what, int type)
{
    std::string msg = what;
    //关键字过滤
    Forbid_word_replace::getInstance()->Filter(msg);
    if (type != 0)
    {
        msg = m_s + "\"f\":\"" + json_spirit::add_esc_chars<std::string>(who,true)
                + "\",\"m\":\"" + json_spirit::add_esc_chars<std::string>(msg, true)
                + "\",\"type\":" + boost::lexical_cast<std::string>(type) + "}";
    }
    else
    {
        msg = m_s + "\"f\":\"" + json_spirit::add_esc_chars<std::string>(who,true)
                + "\",\"m\":\"" + json_spirit::add_esc_chars<std::string>(msg,true) +"\"}";
    }
    return msg;
}

bool ChatChannel::IsEmpty()
{
#ifdef CHANNEL_LOCK    
    boost::mutex::scoped_lock lock(channel_mutex_);
#endif
    return m_useridlist.empty();
}

void ChatChannel::start()
{
    m_worker.setChannel(shared_from_this());
    boost::thread thread(boost::bind(&channelProcesser::run, &m_worker));
    thread.detach();    
}

void ChatChannel::stop()
{
    m_worker.stop();
}

//广播频道信息
bool channelProcesser::work(std::string &msg)       // 在些完成实际任务.
{
    try
    {
        if (_channel.get())
            _channel->_BroadMsg(msg);
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "channel Processer work , Exception: " << e.what() << "\n";
    }
    return true;
}

void channelProcesser::workloop()
{
    ++_runing_loop;
#ifdef DEBUG_PER        
    time_t last_time = 0;
    uint64_t processed_cmd = 0;
#endif
    do
    {
        try
        {
#ifdef DEBUG_PER
            {            
                uint64_t processed = jobqueue_._processed_cmds-processed_cmd;
                time_t time_now = time(NULL);
                if (last_time != time_now && time_now % 10 == 0)
                {
                    uint64_t inqueue = jobqueue_._total_cmds - jobqueue_._processed_cmds;
                    if (processed >= 2000 || inqueue > 100)
                    {
                        cout<<"======================= channel "<<worker_name<<" : "<<dec<<jobqueue_._processed_cmds<<"/"<<inqueue<<"("<<processed<<"/10s)"<<endl<<flush;
                    }
                    processed_cmd = jobqueue_._processed_cmds;
                }
                last_time = time_now;
            }
#endif
            //uint64_t _start_time = splsTimeStamp();
            std::string task_ = jobqueue_.getjob();
            //task_._start_get = _start_time;
            if (work(task_))
            {
                do_sleep(10);
                continue;
            }
            else
                break;
        }
        catch (std::exception& e)
        {
            std::cerr << "wrok loop Exception: " << e.what() << "\n";
            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);
            for (int i = 0; i < nSize; i++)
            {
                cout << symbols[i] << endl;
            }
            free(symbols);
            --_runing_loop;
            return;
        }
    } while (!exitthread);
    std::cout<< "**************** "<<worker_name<<" workloop break *****************" <<endl;
    --_runing_loop;
    return;
}

void channelProcesser::setChannel(boost::shared_ptr<ChatChannel> ch)
{
    _channel = ch;
}

void channelProcesser::run()
{
    try
    {
        for (std::size_t i = 0; i < maxthreads_; ++i) {
            boost::shared_ptr<boost::thread> _thread(new boost::thread(
                boost::bind(&channelProcesser::workloop, this)));
            threads_.push_back(_thread);
        }

        for (std::size_t i = 0; i < maxthreads_; ++i) {
            threads_[i]->join();
        }
    }
    catch (std::exception& e)
    {
        std::cout << "ERROR INFO:" << e.what() << std::endl;
    }
}

void channelProcesser::stop()
{
    exitthread = true;
    jobqueue_.notify_all();
}

int channelProcesser::running()
{
    return _runing_loop;
}

