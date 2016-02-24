
#include "stdafx.h"

#include "login.h"
#include "data.h"
#include "errcode_def.h"

//在线用户
#include "utils_all.h"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include "net.h"
#include "json_spirit_writer_template.h"

#include "utils_lang.h"
#include "text_filter.h"

volatile int g_print_debug_info = 0;

volatile int g_enable_debug_cmd_line = 0;

#include<boost/tokenizer.hpp>
#include "singleton.h"
#include "pk.h"

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
extern int weekUpdate();

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
,m_cid(0)
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
    INFO("create charactor "<<m_account->m_account<<"->"<<m_cid);
    m_charactor = GeneralDataMgr::getInstance()->GetCharData(cid);
}

OnlineCharactor::~OnlineCharactor()
{
    --OnlineCharactor::_refs;
    INFO("delete charactor "<<m_account->m_account<<"->"<<m_cid);
}

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
                return gch->Chat(m_charactor->m_name, msg, 0, m_charactor->m_gender, m_charactor->m_nick.get_string());
            }
        }
        return HC_ERROR_NOT_JOIN_GUILD;
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
        boost::shared_ptr<ChatChannel> ach = GeneralDataMgr::getInstance()->GetCampChannel(m_charactor->m_race);
        if (ach.get())
        {
            return ach->Chat(m_charactor->m_name, msg, 0, m_charactor->m_gender, m_charactor->m_nick.get_string());
        }
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

    //调试开关
    if (g_enable_debug_cmd_line > 0 && !strncmp("/", msg.c_str(), 1))
    {
        cout<<"recieve debug cmd:"<<msg<<endl;
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
                    m_charactor->addGold(atoi(it->c_str()), gold_get_init, true);
                }
                else
                {
                    m_charactor->addGold(100000, gold_get_init, true);
                }
                return HC_SUCCESS;
            }
            else if ("/bindgold" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    m_charactor->addGold(atoi(it->c_str()), gold_get_init);
                }
                else
                {
                    m_charactor->addGold(100000, gold_get_init);
                }
                return HC_SUCCESS;
            }
            else if ("/silver" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    m_charactor->addSilver(atoi(it->c_str()), silver_get_init);
                }
                else
                {
                    m_charactor->addSilver(10000000, silver_get_init);
                }
                return HC_SUCCESS;
            }
            else if ("/level" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int level = atoi(it->c_str());
                    m_charactor->levelup(level);
                }
                else
                {
                }
                return HC_SUCCESS;
            }
            else if ("/vip" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int vip = atoi(it->c_str());
                    if (vip < 0)
                        vip = 0;
                    if (vip > iMaxVIP)
                        vip = iMaxVIP;
                    int old_vip = m_charactor->m_vip;
                    m_charactor->m_vip = vip;
                    if (old_vip != m_charactor->m_vip)
                    {
                        InsertSaveDb("update char_data set vip='" + LEX_CAST_STR(m_charactor->m_vip)
                            + "' where cid=" + LEX_CAST_STR(m_charactor->m_id));
                    }
                }
                else
                {
                }
                return HC_SUCCESS;
            }
            else if ("/addhero" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int hid = atoi(it->c_str());
                    m_charactor->m_heros.Add(hid);
                }
                return HC_SUCCESS;
            }
            else if ("/addheroexp" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int exp = atoi(it->c_str());
                    boost::shared_ptr<CharHeroData> p_hero = m_charactor->m_heros.GetHero(m_charactor->m_heros.m_default_hero);
                    if (p_hero.get() && p_hero->m_baseHero.get())
                    {
                        p_hero->addExp(exp);
                    }
                }
                return HC_SUCCESS;
            }
            else if ("/addequip" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int eid = atoi(it->c_str());
                    m_charactor->addEquipt(eid);
                }
                return HC_SUCCESS;
            }
            else if ("/addgem" == *it)
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
                            m_charactor->addGem(tid,num);
                            return HC_SUCCESS;
                        }
                    }
                }
            }
            else if ("/addprestige" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int race = atoi(it->c_str());
                    ++it;
                    if (it != tok.end())
                    {
                        int num = atoi(it->c_str());
                        m_charactor->addPrestige(race, num);
                    }
                }
                return HC_SUCCESS;
            }
            else if ("/addtask" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int tid = atoi(it->c_str());
                    boost::shared_ptr<baseTask> first = Singleton<taskMgr>::Instance().getTask(tid);
                    m_charactor->m_tasks.acceptTask(first);
                }
                return HC_SUCCESS;
            }
            else if ("/addlibao" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int libao_id = atoi(it->c_str());
                    m_charactor->addLibao(libao_id, 1);
                }
                return HC_SUCCESS;
            }
            else if ("/addbaoshi" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int bid = atoi(it->c_str());
                    ++it;
                    if (it != tok.end())
                    {
                        int num = atoi(it->c_str());
                        m_charactor->addBaoshi(bid, 1, num);
                    }
                }
                return HC_SUCCESS;
            }
            else if ("/tempo" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    int id = atoi(it->c_str());
                    while (m_charactor->m_cur_strongholdid < id)
                    {
                        m_charactor->m_tempo.update(m_charactor->m_cur_strongholdid+1, true);
                    }
                }
                return HC_SUCCESS;
            }
            else if (*it == "/reset")
            {
                dailyUpdate();
                return HC_SUCCESS;
            }
            else if (*it == "/weekreset")
            {
                weekUpdate();
                return HC_SUCCESS;
            }
            else if (*it == "/test_db_card")
            {
                test_DB_card();
                return HC_SUCCESS;
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
            else if ("/query" == *it)
            {
                ++it;
                if (it != tok.end())
                {
                    if (*it == "online")
                    {
                        int total_online = GeneralDataMgr::getInstance()->getTotalOnline(false);
                        std::string msg = "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_wisper)+",\"s\":200,\"f\":\"system\",\"m\":\"total online:" + LEX_CAST_STR(total_online) + "\"}";
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
                    std::string msg = "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_wisper)+",\"s\":200,\"f\":\"system\",\"m\":\"account:" + LEX_CAST_STR(account) + ", return " + LEX_CAST_STR(ret) + "\"}";
                    Send(msg);
                    return HC_SUCCESS;
                }
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
            str_replace_all(newmsg, "\\\"", "\"");
            json_spirit::read(newmsg.substr(1), value);
            if (value.type() != json_spirit::obj_type)
            {
                cout<<"recieve debug cmd:"<<newmsg<<endl;
                std::string msg = "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_wisper)+",\"s\":200,\"f\":\"system\",\"m\":\"unknow cmd!\"}";
                Send(msg);
                return HC_SUCCESS;
            }
            json_spirit::mObject obj = value.get_obj();
            net::actionmessage act_msg(obj, 0);
            act_msg.setsession(m_account->GetSocket());
            InsertActionWork(act_msg);
            Send("{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_wisper)+",\"s\":200,\"f\":\"system\",\"m\":\"OK!\"}");
            return HC_SUCCESS;
        }
    }
    boost::shared_ptr<ChatChannel> wch = GeneralDataMgr::getInstance()->GetWorldChannel();
    if (needgold)
    {
        if (m_charactor->subGem(GEM_ID_HORN, 1, gem_cost_horn) < 0)
        {
            if (m_charactor->subGold(iHornChatGoldCost, gold_cost_horn) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
        }
        wch = GeneralDataMgr::getInstance()->GetHornChannel();
    }
    if (wch.get())
    {
        return wch->Chat(m_charactor->m_name, msg, 0, m_charactor->m_gender, m_charactor->m_nick.get_string());
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
            static std::string startmsg = "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_wisper)+",\"s\":200,\"f\":\"";
            std::string sendmsg = startmsg + m_charactor->m_name + "\""
                                    + ",\"gd\":" + boost::lexical_cast<std::string>(m_charactor->m_gender)
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

int OnlineCharactor::PKRoomChat(const std::string& msg)
{
    if (m_charactor.get())
    {
        //被禁言
        if (m_gag_end_time > time(NULL))
        {
            return HC_ERROR_FORBIDEN_CHAT;
        }
        boost::shared_ptr<CharPkData> charPk = Singleton<PkMgr>::Instance().getPkData(m_charactor->m_id);
        if (charPk.get() && charPk->getCharData().get() && charPk->m_roomid > 0)
        {
            boost::shared_ptr<ChatChannel> ach = Singleton<PkMgr>::Instance().GetRoomChannel(charPk->m_roomid);
            if (ach.get())
            {
                return ach->Chat(m_charactor->m_name, msg, 0, m_charactor->m_gender, m_charactor->m_nick.get_string());
            }
        }
        return HC_ERROR_NOT_JOIN_ROOM;
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
    if (m_sockethandle.get())
    {
        time_t t_now = time(NULL);
        int check = 15 * 60;
        if (m_sockethandle->connect_time() - t_now > check
            && m_sockethandle->last_beat_time() - t_now > check)
        {
            //15分钟没心跳则踢下线
            json_spirit::mObject mobj;
            mobj["cmd"] = "logout";
            mobj["account"] = m_account->m_account;
            InsertInternalActionWork(mobj);
            return HC_SUCCESS;
        }
    }
    if (m_charactor.get())
    {
        CharData* pc = m_charactor.get();
        //一些合并到心跳里的通知
        if (pc->m_need_notify.size())
        {
            for (std::map<int, int>::iterator it = pc->m_need_notify.begin(); it != pc->m_need_notify.end(); ++it)
            {
                Send("{\"cmd\":\"notify\",\"type\":" + LEX_CAST_STR(it->first) + ",\"nums\":" + LEX_CAST_STR(it->second) + ",\"s\":200}");
            }
            pc->m_need_notify.clear();
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
            INFO(" msg to " << account->m_cid);
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

int ChatChannel::Chat(const std::string &who, const std::string &what, int types, int gender, const std::string& nick)
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
                + ",\"nick\":" + nick
                + "}";
    }
    else
    {
        msg = m_s + "\"f\":\"" + json_spirit::add_esc_chars<std::string>(who,true)
                + "\",\"m\":\"" + json_spirit::add_esc_chars<std::string>(msg,true) +"\""
                + ",\"gd\":" + boost::lexical_cast<std::string>(gender)
                + ",\"nick\":" + nick
                +"}";
    }
    m_broadmsg_que.submitjob(msg);
    //BroadMsg(msg);
    return HC_SUCCESS_NO_RET;
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

