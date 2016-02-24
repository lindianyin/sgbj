
#include <boost/random.hpp>
#include "testChar.h"
#include "robotMgr.h"
#include "md5.h"
#include <pthread.h>

#include <iostream>

#define NET_PACK_START "WUDONG!!"
#define NET_PACK_START_LEN 8
#define HC_SUCCESS 200

#define INFO(x)// std::cout<<x

using json_spirit::Pair;
using json_spirit::mObject;

bool str_replace(std::string& msg, const std::string& toreplace, const std::string& replacemsg, bool rfind = false)
{
    size_t pos = rfind ? msg.rfind(toreplace):msg.find(toreplace);
    if (pos != std::string::npos)
    {
        msg.replace(pos, toreplace.length(), replacemsg);
        return true;
    }
    else
    {
        std::cout<<"str replace fail:"<<msg<<","<<toreplace<<","<<replacemsg<<std::endl;
        return false;
    }
}

const std::string strLoginMessage = "{\"cmd\":\"login\",\"user\":\"$U\",\"password\":\"$P\"}";
const std::string strCmdCharList = "{\"cmd\":\"charlist\"}";
const std::string strCmdFormat = "{\"cmd\":\"$C\"}";
const std::string strCmdIdFormat = "{\"cmd\":\"$C\",\"id\":$I}";
const std::string strCmdTypeFormat = "{\"cmd\":\"$C\",\"type\":$I}";

const std::string strcmdCreateChar = "{\"cmd\":\"create\",\"name\":\"$N\",\"spic\":$S,\"g1\":$A,\"g2\":$D}";

const std::string strcmdCharLogin = "{\"cmd\":\"roleLogin\",\"id\":$C}";

const std::string strcmdChatFormat = "{\"cmd\":\"chat\",\"ctype\":1,\"m\":\"$M\"}";
const std::string strcmdAttackStronghold = "{\"cmd\":\"attack\",\"id\":$S}";

const std::string strcmdGetMapInfo = "{\"cmd\":\"getAreaStageList\",\"id\":$M}";

const std::string strcmdGetStageInfo = "{\"cmd\":\"getStageGateList\",\"mapid\":$M,\"stageid\":$S}";

//查询关卡详情
const std::string strcmdQueryStronghold = "{\"pos\":$P,\"stageid\":$S,\"cmd\":\"getGateInfo\",\"mapid\":$M}";

//[INFO] Output------------------------- {"type":0,"page":1,"pageNums":15,"cmd":"getGeneralList"}
//显示武将列表
const std::string strcmdShowGenerals = "{\"type\":0,\"page\":1,\"pageNums\":15,\"cmd\":\"getGeneralList\"}";
//[INFO] Output------------------------- {"type":3,"page":1,"pageNums":35,"cmd":"getStoreList"}
//显示包中的装备
const std::string strcmdShowEquiptments = "{\"type\":3,\"page\":1,\"pageNums\":35,\"cmd\":\"getStoreList\"}";

//显示武将身上装备
const std::string strcmdShowGeneralEquiptments = "{\"cmd\":\"getRoleEquipList\",\"gid\":$G}";

//装备物品
const std::string strcmdEquipItem = "{\"gid\":$G,\"cmd\":\"upEquip\",\"id\":$E}";

//查询任务信息
const std::string strcmdQueryTask = "{\"cmd\":\"getCurTask\"}";

//查询可招募武将
const std::string strcmdCanBuyGeneralList = "{\"cmd\":\"getPostHeroList\"}";

//招募武将
const std::string strcmdBuyGeneral = "{\"purpose\":2,\"cmd\":\"dealBuy\",\"id\":$I}";

//查询阵上英雄
const std::string strcmdGetZhenGenerals = "{\"id\":1,\"cmd\":\"getFormation\"}";

const std::string strcmdSetFormation = "{\"pos1\":$1,\"pos2\":$2,\"cmd\":\"setFormation\",\"gid\":$G,\"id\":1,\"purpose\":$P}";

//查询左侧队列
const std::string strcmdGetUpdateList = "{\"cmd\":\"getUpdateList\"}";

//强化装备
const std::string strcmdEnhanceEquiptment = "{\"id\":$E,\"cmd\":\"enhanceEquip\"}";

//升级秘法
const std::string strcmdLevelupSkill = "{\"type\":$T,\"cmd\":\"upgradeWeapon\"}";

#define FIGHT_COOL_TIME 8

int convertCmd(const std::string& cmd)
{
    static std::map<std::string, int> cmd_map;
    static bool inited = false;
    if (!inited)
    {
        cmd_map["login"] = CMD_LOGIN;
        cmd_map["charlist"] = CMD_CHARLIST;
        cmd_map["roleLogin"] = CMD_ROLE_LOGIN;
        cmd_map["create"] = CMD_CREATE_CHAR;
        cmd_map["getRoleInfo"] = CMD_ROLE_INFO;

        cmd_map["getCurTask"] = CMD_TASK_INFO;
        cmd_map["getGeneralList"] = CMD_GENERAL_LIST;
        cmd_map["getStoreList"] = CMD_STORE_LIST;
        cmd_map["getRoleEquipList"] = CMD_GENERAL_EQUIPTS;
        cmd_map["getStageGateList"] = CMD_STAGE_INFO;
        cmd_map["upEquip"] = CMD_EQUIP;
        cmd_map["getGateInfo"] = CMD_STRONGHOLD_INFO;
        cmd_map["attack"] = CMD_ATTACK;
        cmd_map["checkStageFinish"] = CMD_GET_STAGE_FINISH;
        cmd_map["dealGetAward"] = CMD_GET_AWARD;
        cmd_map["getPostHeroList"] = CMD_GET_POST_GENERAL_LIST;
        cmd_map["dealBuy"] = CMD_DEAL_BUY;
        cmd_map["getFormation"] = CMD_GET_FORMATION;
        cmd_map["setFormation"] = CMD_SET_FORMAIION;
        cmd_map["getUpdateList"] = CMD_GET_UPDATE_LIST;
        cmd_map["getBookList"] = CMD_GET_BOOK_LIST;
        cmd_map["trainGeneral"] = CMD_TRAIN_GENERAL;
        
    }
    /*
        CMD_LOGIN = 1001,
        CMD_CHARLIST,
        CMD_ROLE_LOGIN,
        CMD_CREATE_CHAR,
        CMD_ROLE_INFO,
    */
    std::map<std::string, int>::iterator it = cmd_map.find(cmd);
    if (it != cmd_map.end())
    {
        return it->second;
    }
    return 0;
}

int my_random(int mins, int maxs)
{
    if (mins >= maxs)
    {
        return mins;
    }
    static boost::mt19937 gen(42u);
    static rwlock lock;
    static bool inited = false;
    if (!inited)
    {
        rwlock_init(&lock);
        gen.seed(time(NULL));
        inited = true;
    }

    rwlock_wlock(&lock);
    boost::uniform_int<> dist(mins, maxs);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(gen, dist);
    int ret = die();
    rwlock_wunlock(&lock);
    return ret;
}

double my_random(double mins, double maxs)
{
    int mins_int = (int)(10000.0 * mins);
    int maxs_int = (int)(10000.0 * maxs);
    int ret = my_random(mins_int, maxs_int);
    return (double)(ret/10000.0);
    //return (maxs > mins) ? (mins + (double)(g_uni() * (maxs + 1 - mins))) : mins;
}

testRobot::testRobot(robotMgr& h, const std::string& account)
:m_account(account)
,m_mgr(h)
,io_service_(h.get_io_service())
,strand_(io_service_)
,socket_(io_service_)
{
    state_ = CHAR_STATE_UNCONNECT;
    memset(&cdata_, 0, sizeof(cdata_));
    memset(send_buffer_, 0, 2048);
    memcpy(send_buffer_, NET_PACK_START, NET_PACK_START_LEN);
    pending_write_ = 0;
    pending_read_ = 0;
    m_heart_beat = 0;
    m_last_attack_time = 0;
    m_idle = 0;
    cdata_.stage_reward.mapid = 1;
    cdata_.stage_reward.stageid = 1;
    encryption_ = 0;
}

void testRobot::postStart()
{
    if (state_ != 0)
    {
        return;
    }
    state_ = CHAR_STATE_TRY_CONNECT;
    io_service_.post(strand_.wrap(boost::bind(&testRobot::handleStart, shared_from_this())));
}

void testRobot::handleStart()
{
#if 1
    tcp::resolver::iterator itr = m_mgr.getServerEndpoint();
    INFO(m_account<<" -> try connect..."<<endl);
    boost::asio::async_connect(socket_, itr,
                strand_.wrap(
                boost::bind(&testRobot::handleConnect, this,boost::asio::placeholders::error)
                )
            );
#else
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string(m_mgr.gethost()), m_mgr.getport());
    INFO(m_account<<" -> try connect..."<<endl);
    socket_.async_connect(endpoint, strand_.wrap(
                boost::bind(&testRobot::handleConnect, this, boost::asio::placeholders::error)
                ));
#endif
}

void testRobot::handleConnect(const boost::system::error_code& error)
{
    if (error)
    {
        state_ = CHAR_STATE_CONNECT_FAIL;
    }
    else
    {
        INFO(m_account<<" -> handleConnect()"<<endl);
        state_ = CHAR_STATE_CONNECTED;
        tryReadHead();
    }
}

std::string hexstr(unsigned char *buf, int len)
{
    const char *set = "0123456789abcdef";
    char str[65], *tmp;
    unsigned char *end; 
    if (len > 32)
        len = 32; 

    end = buf + len;
    tmp = &str[0]; 
    while (buf < end)
    {
        *tmp++ = set[ (*buf) >> 4 ];
        *tmp++ = set[ (*buf) & 0xF ];
        buf ++;
    }
    *tmp = 0; 
    return std::string(str);
}

void testRobot::tryAuth()
{
    json_spirit::Object obj;
    int union_id = m_mgr.get_union_id();
    std::string auth_code = m_mgr.get_auth_code();
    time_t timenow = time(NULL);

    obj.push_back( Pair("cmd", "login") );
    obj.push_back( Pair("qid", m_account) );
    obj.push_back( Pair("qname", m_account) );
    obj.push_back( Pair("server_id", "s1") );
    obj.push_back( Pair("extra1", "") );
    obj.push_back( Pair("extra2", "") );
    obj.push_back( Pair("isAdult", 1) );
    obj.push_back( Pair("union_id", union_id) );
    obj.push_back( Pair("time", timenow) );

    std::string strUpdate = "qid=" + m_account + "&qname=" + m_account + "&union_id=" + LEX_CAST_STR(union_id)
                    + "&time=" + LEX_CAST_STR(timenow) + "&server_id=s1&extra1=&extra2=" + auth_code;

    CMD5 md5;
    unsigned char res[16];
    md5.MessageDigest((const unsigned char *)strUpdate.c_str(), strUpdate.length(), res);

    std::string finalSign = hexstr(res,16);
    obj.push_back( Pair("sign", finalSign) );
    
    doWrite(json_spirit::write(obj));
}

void testRobot::doWrite(const std::string& msg)
{
    m_idle = 0;
    if (pending_write_ == 0)
    {
        int len = msg.length();

        INFO(m_account<<" -> send msg ["<<msg<<"], length "<<len<<endl);
        if (encryption_)
        {
            *(unsigned short*)(send_buffer_ + 8) = htons(len * encryption_);
        }
        else
        {
            *(unsigned short*)(send_buffer_ + 8) = htons(len);
        }
        char* buffer = send_buffer_;
        len += 24;
        if (len > 2048)
        {
            boost::system::error_code err;
            handleWrite(err, 0);
            return;
        }
        memcpy(send_buffer_ + 24, msg.c_str(), msg.length());

        boost::asio::async_write(socket_, boost::asio::buffer(buffer, len),
                            strand_.wrap(
                                            boost::bind(&testRobot::handleWrite,
                                                        shared_from_this(),
                                                        boost::asio::placeholders::error,
                                                        boost::asio::placeholders::bytes_transferred)
                                        )
                        );
        ++pending_write_;
    }
    else
    {
        INFO(m_account<<" -> push to send queue - "<<msg<<endl);
        send_queue_.push_back(msg);
    }
}

void testRobot::postHeartbeat(time_t t, int cmd)
{
    m_heart_beat = t;
    //if (CHAR_STATE_CHAR_LOGINED == state_)
    {
        io_service_.post(boost::bind(&testRobot::handleHeartbeat, shared_from_this(), cmd));
    }
}

void testRobot::handleHeartbeat(int cmd)
{
    INFO(m_account<<" -> handleHeartbeat("<<cmd<<")"<<endl);

    switch (state_)
    {
        case CHAR_STATE_CHAR_LOGINED:
            break;
        case CHAR_STATE_CONNECT_CLOSED:
            //try connect again
            ++m_idle;
            if (m_idle > 3)
            {
                m_idle = 0;
                handleStart();
            }
            return;
        default:
            ++m_idle;
            if (m_idle > 10)
            {
                m_idle = 0;
                pending_read_ = 0;
                pending_write_ = 0;
                closeSocket();
            }
            return;
    }
    if (cmd > 0)
    {
        switch (cmd)
        {
            
        }
    }
    else
    {
        if (0 == cdata_.mapid)
        {
            doWrite("{\"cmd\":\"getRoleInfo\"}");
            return;
        }
        //关卡信息没有就查询
        if (cdata_.strongholds.mapid != cdata_.mapid)
        {
            queryStageInfo();
            return;
        }
        //查询通关奖励
        if (cdata_.stage_reward.get == 0)
        {
            std::string cmd = "{\"stageid\":$S,\"cmd\":\"checkStageFinish\",\"mapid\":$M}";
            str_replace(cmd, "$M", LEX_CAST_STR(cdata_.stage_reward.mapid));
            str_replace(cmd, "$S", LEX_CAST_STR(cdata_.stage_reward.stageid));
            doWrite(cmd);
            return;
        }
        //查任务
        if (cdata_.tasks_get == 0)
        {
            doWrite(strcmdQueryTask);
            return;
        }
        //是否能领取奖励
        for (int i = 0; i < 3; ++i)
        {
            if (cdata_.tasks[i].done == 2)
            {
                std::string msg = "{\"cmd\":\"dealGetAward\",\"purpose\":4,\"id\":$I}";
                str_replace(msg, "$I", LEX_CAST_STR(cdata_.tasks[i].id));
                doWrite(msg);
                cdata_.tasks[i].done = 1;
                return;
            }
        }
        //查背包中的装备
        if (cdata_.equipts_get == 0)
        {
            doWrite(strcmdShowEquiptments);
            return;
        }
        //查武将列表
        if (cdata_.generals_get == 0)
        {
            INFO(m_account<<" -> query generals"<<endl);
            doWrite(strcmdShowGenerals);
            return;
        }

        //查武将装备列表
        for (int i = 0; i < 10; ++i)
        {
            if (cdata_.generals[i].gid > 0 && cdata_.generals[i].equips_get == 0)
            {
                std::string cmd = strcmdShowGeneralEquiptments;
                str_replace(cmd, "$G", LEX_CAST_STR(cdata_.generals[i].gid));
                doWrite(cmd);
                return;
            }
        }

        //是否可以装备物品
        if (checkUpEquipment())
        {
            return;
        }

        //是否可以招募武将
        if (checkCanBuyGeneral())
        {
            return;
        }

        //武将是否可以上阵
        if (checkCanUpZhen())
        {
            return;
        }

        //武将是否可以训练升级
        if (checkCanTrain())
        {
            return;
        }
        
        //检查是否可以强化装备
        if (checkEnhanceEquipment())
        {
            return;
        }
        
        //看是否要攻打关卡
        if (cdata_.sinfo.get == 0)
        {
            std::string cmd = strcmdQueryStronghold;
            str_replace(cmd, "$M", LEX_CAST_STR(cdata_.mapid));
            str_replace(cmd, "$S", LEX_CAST_STR(cdata_.stageid));
            str_replace(cmd, "$P", LEX_CAST_STR(cdata_.stronghold % 8));
            doWrite(cmd);
            return;
        }
        else
        {
            if (m_last_attack_time < (time(NULL) - FIGHT_COOL_TIME)
                && cdata_.supply >= cdata_.sinfo.need_supply)
            {
                std::string cmd = strcmdAttackStronghold;
                str_replace(cmd, "$S", LEX_CAST_STR(cdata_.sinfo.id));
                doWrite(cmd);
                return;
            }
        }

        ++m_idle;
        if (m_idle % 10 == 0)
        {
            cdata_.mapid = 0;
            cdata_.tasks_get = 0;
            cdata_.equipts_get = 0;
            cdata_.generals_get = 0;
        }
        if (m_idle % 15 == 0)
        {
            cdata_.updateList.get = 0;
        }
        INFO(m_account<<" -> nothing to do.(ling:"<<cdata_.ling<<"), idle "<<m_idle<<endl);
        
    }
}

void testRobot::tryReadHead()
{
    // 必须读满一个数据头才返回.
    try
    {
        boost::asio::async_read(socket_,
                                boost::asio::buffer(&recv_msg_, 24),
                                boost::asio::transfer_at_least(24),
                                strand_.wrap(
                                    boost::bind(&testRobot::handleReadHead,
                                                shared_from_this(),
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred)
                                )
        );
        ++pending_read_;
    }
    catch (boost::system::system_error& e)
    {
        std::cout<<m_account<<" -> tryReadHead, exception : "<<e.what()<<endl;
        closeSocket();
    }
}

void testRobot::tryReadData()
{
    --pending_read_;
    // 必须读满一个数据头才返回.
    try
    {
        int recv_len = 0;
        if (recv_msg_._left_recv > iMaxRecvBufferLen)
        {
            recv_len = iMaxRecvBufferLen;            
        }
        else
        {
            recv_len = recv_msg_._left_recv;
        }
        boost::asio::async_read(socket_,
                                boost::asio::buffer(recv_msg_._data, recv_len),
                                boost::asio::transfer_at_least(recv_len),
                                strand_.wrap(
                                    boost::bind(&testRobot::handleReadData,
                                                shared_from_this(),
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred)
                                )
        );
        ++pending_read_;
    }
    catch (boost::system::system_error& e)
    {
        std::cout<<m_account<<" -> tryReadData, exception : "<<e.what()<<endl;
        closeSocket();
    }
}

void testRobot::handleReadHead(const boost::system::error_code& error, size_t bytes_transferred)
{
    --pending_read_;
    if (error || bytes_transferred != 24)
    {
        std::cout<<m_account<<" -> handleReadHead, error "<<error<<",bytes "<<bytes_transferred<<std::endl;
        closeSocket();
        return;
    }
    recv_msg_.len = ntohs(recv_msg_.len);
    if (0 != strncmp(NET_PACK_START, recv_msg_._head, NET_PACK_START_LEN))
    {
        std::cout<<m_account<<" -> handleReadHead, head error"<<std::endl;
        closeSocket();
        return;
    }

    if (recv_msg_.len <= 0)
    {
        std::cout<<m_account<<" -> handleReadHead, len = "<<recv_msg_.len<<std::endl;
        closeSocket();
        return;
    }

    INFO(m_account<<" -> read head, total len = "<<recv_msg_.len<<endl);
    if (recv_msg_.len > (iMaxRecvBufferLen))
    {
        recv_msg_._drop = true;
    }
    else
    {
        recv_msg_._drop = false;
    }
    recv_msg_._left_recv = recv_msg_.len;
    tryReadData();
}

void testRobot::handleReadData(const boost::system::error_code& error, size_t bytes_transferred)
{
    --pending_read_;
    if (error)
    {
        std::cout<<m_account<<" -> handleReadData, error "<<error<<",bytes "<<bytes_transferred<<std::endl;
        closeSocket();
        return;
    }
    if (recv_msg_._left_recv < bytes_transferred)
    {
        std::cout<<m_account<<" -> handleReadHead, want recv "<<recv_msg_._left_recv<<",bytes "<<bytes_transferred<<std::endl;
        closeSocket();
        return;
    }
    recv_msg_._left_recv -= bytes_transferred;
    if (recv_msg_._left_recv == 0)
    {
        if (!recv_msg_._drop)
        {
            //处理消息
            recv_msg_._data[recv_msg_.len] = 0;
            std::string s(recv_msg_._data);
            INFO(m_account<<" -> recv ["<<s<<"]"<<endl);
            json_spirit::mValue value;
            json_spirit::read(s, value);
            using namespace std;
            using namespace boost;
            using namespace json_spirit;
            Object robj;
            if (value.type() != obj_type)
            {
                std::cout<<m_account<<" -> handleReadHead, not a obj , ["<<s<<"]"<<std::endl;
                closeSocket();
                return;
            }
            else
            {
                mObject& mobj = value.get_obj();
                std::string cmd = "";
                READ_STR_FROM_MOBJ(cmd, mobj, "cmd");

                int iCmd = convertCmd(cmd);

                int ret = 200;
                READ_INT_FROM_MOBJ(ret, mobj, "s");
                
                INFO("recv cmd "<<cmd<<endl);

                if (ret != 200 && iCmd == CMD_LOGIN)
                {
                    std::cout<<m_account<<" -> recv login cmd, s = "<<ret<<endl;
                    closeSocket();
                    return;
                }
                switch (state_)
                {
                    case CHAR_STATE_CONNECTED:
                        if (CMD_LOGIN == iCmd)
                        {
                            int login_type = 0;
                            READ_INT_FROM_MOBJ(login_type,mobj,"login");
                            if (1 == login_type)
                            {
                                READ_INT_FROM_MOBJ(encryption_, mobj,"ver");
                                encryption_ = encryption_ % 3;
                                if (encryption_)
                                {
                                    ++encryption_;
                                }
                                INFO(m_account<<" -> recv login"<<endl);
                                //收到连接信息，尝试登录验证
                                state_ = CHAR_STATE_TRY_LOGIN;
                                tryAuth();
                            }
                            else
                            {
                                std::cout<<m_account<<" -> connected state : recv unknow login type "<<login_type<<std::endl;
                                closeSocket();
                                return;
                            }
                        }
                        break;
                    case CHAR_STATE_TRY_LOGIN:
                        if (CMD_LOGIN == iCmd)
                        {
                            int login_type = 0;
                            READ_INT_FROM_MOBJ(login_type,mobj,"login");
                            if (2 == login_type)
                            {
                                INFO(m_account<<" -> login success."<<endl);
                                //请求角色列表
                                state_ = CHAR_STATE_LOGINED;
                                doWrite("{\"cmd\":\"charlist\"}");
                            }
                            else
                            {
                                std::cout<<m_account<<" -> try login state : recv unknow login type "<<login_type<<std::endl;
                                closeSocket();
                                return;
                            }
                        }
                        break;
                    case CHAR_STATE_LOGINED:
                        if (CMD_CHARLIST == iCmd)
                        {
                            //处理角色列表，找出角色id
                            processCharList(mobj);
                            if (cdata_.cid > 0)
                            {
                                INFO(m_account<<" -> recv char list."<<endl);
                                //登入角色
                                state_ = CHAR_STATE_TRY_LOGIN_CHAR;
                                doWrite("{\"id\":" + LEX_CAST_STR(cdata_.cid) + ",\"cmd\":\"roleLogin\"}");
                            }
                            else
                            {
                                INFO(m_account<<" -> try create char"<<endl);
                                //创建角色
                                state_ = CHAR_STATE_TRY_CREATE_CHAR;
                                //doWrite("{\"cmd\":\"qcreate\"}");
                                int spic = m_mgr.random_spic();
                                std::string name = m_mgr.random_name(spic);
                                doWrite("{\"spic\":" + LEX_CAST_STR(spic) + ",\"g1\":4,\"name\":\"" + name + "\",\"cmd\":\"create\",\"g2\":3}");
                            }
                        }                        
                        break;
                    case CHAR_STATE_TRY_CREATE_CHAR:
                        if (CMD_CREATE_CHAR == iCmd)
                        {
                            //[INFO] Input-------------------------
                            //{"id":714,"cmd":"create","s":200}
                            READ_INT_FROM_MOBJ(cdata_.cid, mobj, "id");
                            if (cdata_.cid > 0)
                            {
                                INFO(m_account<<" -> create char success"<<endl);
                                //登入角色
                                state_ = CHAR_STATE_TRY_LOGIN_CHAR;
                                doWrite("{\"id\":" + LEX_CAST_STR(cdata_.cid) + ",\"cmd\":\"roleLogin\"}");
                            }
                            else
                            {
                                INFO(m_account<<" -> create char fail."<<endl);
                                int spic = m_mgr.random_spic();
                                std::string name = m_mgr.random_name(spic);
                                doWrite("{\"spic\":" + LEX_CAST_STR(spic) + ",\"g1\":4,\"name\":\"" + name + "\",\"cmd\":\"create\",\"g2\":3}");
                            }
                        }
                        break;
                    case CHAR_STATE_TRY_LOGIN_CHAR:
                        {
                            if (CMD_ROLE_LOGIN == iCmd)
                            {
                                if (200 != ret)
                                {
                                    std::cout<<m_account<<" -> try login char state : login return "<<ret<<std::endl;
                                    closeSocket();
                                    return;
                                }
                                INFO(m_account<<" -> char login success!"<<endl);
                                state_ = CHAR_STATE_CHAR_LOGINED;
                                doWrite(m_mgr.random_hello_msg());
                                doWrite("{\"cmd\":\"qdate\"}");
                                doWrite("{\"cmd\":\"panel\"}");
                                doWrite("{\"cmd\":\"getRoleInfo\"}");
                                doWrite(strcmdQueryTask);
                                doWrite("{\"cmd\":\"getActionInfo\"}");
                                doWrite("{\"cmd\":\"getUnread\"}");

                                doWrite("{\"cmd\":\"queryNotices\"}");
                                doWrite("{\"cmd\":\"queryUnGetGifts\"}");
                                doWrite("{\"cmd\":\"getUpdateList\"}");
                                doWrite("{\"cmd\":\"getOpenInfo\"}");
                                doWrite("{\"cmd\":\"getNearPlayerList\"}");
                            }
                        }
                        break;
                        
                    case CHAR_STATE_CHAR_LOGINED:    // 角色登录成功了，正常处理游戏消息
                        if (200 == ret)
                        {
                            INFO(m_account<<" recv cmd "<<cmd<<endl);
                            switch (iCmd)
                            {
                                /*
                                    [INFO] Input-------------------------
                                        {"chardata":{"id":714,"spic":2,"gid":0,"vip":0,
                                        "name":"邹琦","gold":0,"silver":10000,"ling":10,
                                        "maxLing":45,"prestige":0,"supply":160,"need_supply":10,
                                        "max_supply":1100,"cur_mapid":1,"cur_stageid":1,
                                        "cur_strongholdid":1,"finish_all_stronghold":false,
                                        "officer":"议郎","canLevelUp":false,
                                        "area":{"area":1,"name":"怒破黄巾",
                                                "memo":"桃园结义，英雄立功","stage_name":""},
                                        "camp":0,"level":1,"openNext":false,"attack":1},
                                        "panel":{"zhuj":0,"army":0,"junt":0,"interior":0},
                                        "cmd":"getRoleInfo","s":200}
                                */
                                case CMD_ROLE_INFO:
                                    {
                                        json_spirit::mObject charInfoObj;
                                        READ_OBJ_FROM_MOBJ(charInfoObj,mobj,"chardata");
                                        READ_INT_FROM_MOBJ(cdata_.gold,charInfoObj,"gold");
                                        READ_INT_FROM_MOBJ(cdata_.silver,charInfoObj,"silver");
                                        int old_level = cdata_.level;
                                        READ_INT_FROM_MOBJ(cdata_.level,charInfoObj,"level");
                                        if (old_level != cdata_.level)
                                        {
                                            if (10 == cdata_.level)
                                            {
                                                std::string msg = m_mgr.random_hello_msg();
                                                doWrite(msg);
                                            }
                                        }
                                        int old_supply = cdata_.supply;
                                        READ_INT_FROM_MOBJ(cdata_.supply,charInfoObj,"supply");
                                        if (cdata_.sinfo.get && old_supply < cdata_.sinfo.need_supply && cdata_.supply >= cdata_.sinfo.need_supply)
                                        {
                                            cdata_.sinfo.get = 0;
                                        }
                                        READ_INT_FROM_MOBJ(cdata_.prestige,charInfoObj,"prestige");

                                        int old_ling = cdata_.ling;
                                        READ_INT_FROM_MOBJ(cdata_.ling,charInfoObj,"ling");
                                        if (old_ling == 0 && cdata_.ling > 0)
                                        {
                                            cdata_.sinfo.get = 0;
                                        }
                                        {
                                            std::string name = "";
                                            READ_STR_FROM_MOBJ(name, charInfoObj, "name");
                                            if (name != "")
                                            {
                                                if (name.length() < 32)
                                                {
                                                    memcpy(cdata_.name, name.c_str(), name.length());
                                                }
                                                else
                                                {
                                                    memcpy(cdata_.name, name.c_str(), 31);
                                                }
                                            }
                                        }
                                        
                                        READ_UINT64_FROM_MOBJ(cdata_.mapid,charInfoObj,"cur_mapid");
                                        READ_UINT64_FROM_MOBJ(cdata_.stageid,charInfoObj,"cur_stageid");
                                        int old_stronghold = cdata_.stronghold;                                        
                                        READ_UINT64_FROM_MOBJ(cdata_.stronghold,charInfoObj,"cur_strongholdid");
                                        if (old_stronghold != cdata_.stronghold)
                                        {
                                            cdata_.sinfo.get = 0;
                                            if ((cdata_.stronghold-1) % 8 == 0)
                                            {
                                                cdata_.stage_reward.get = 0;
                                            }
                                        }
                                        READ_UINT64_FROM_MOBJ(cdata_.finish_all_stronghold,mobj,"finish_all_stronghold");
                                    }
                                    break;
                                
                                case CMD_TASK_INFO:
                                    ProcessTaskList(mobj);
                                    break;
                                case CMD_GENERAL_LIST:
                                    ProcessGeneralList(mobj);
                                    break;
                                case CMD_GENERAL_EQUIPTS:
                                    ProcessGeneralEquiptments(mobj);
                                    break;
                                case CMD_STORE_LIST:
                                    {
                                        int type = 0;
                                        READ_INT_FROM_MOBJ(type,mobj,"type");
                                        if (3 == type)
                                        {
                                            ProcessEquipmentlist(mobj);
                                        }
                                    }
                                    break;
                                case CMD_STAGE_INFO:
                                    ProcessCharStageInfo(mobj);
                                    break;
                                case CMD_EQUIP:
                                    {
                                        int gid = 0;
                                        READ_INT_FROM_MOBJ(gid,mobj,"gid");
                                        if (gid == 0)
                                        {
                                            for (int i = 0; i < 10; ++i)
                                            {
                                                cdata_.generals[i].equips_get = 0;
                                            }
                                        }
                                        else
                                        {
                                            for (int i = 0; i < 10; ++i)
                                            {
                                                if (cdata_.generals[i].gid == gid)
                                                {
                                                    cdata_.generals[i].equips_get = 0;
                                                    break;
                                                }
                                            }
                                        }
                                        cdata_.equipts_get = 0;
                                    }
                                    break;
                                case CMD_STRONGHOLD_INFO:
                                    ProcessStrongholdInfo(mobj);
                                    break;
                                case CMD_ATTACK:
                                    cdata_.mapid = 0;
                                    m_last_attack_time = time(NULL);
                                    break;
                                case CMD_GET_STAGE_FINISH:
                                    ProcessStageFinish(mobj);
                                    break;
                                case CMD_GET_AWARD:
                                    {
                                        int purpose = 0;
                                        READ_INT_FROM_MOBJ(purpose,mobj,"purpose");
                                        if (4 == purpose)
                                        {
                                            //领取任务奖励成功,刷新下任务
                                            cdata_.tasks_get = 0;
                                        }
                                    }
                                    break;
                                case CMD_GET_POST_GENERAL_LIST:
                                    ProcessCanBuyGeneralList(mobj);
                                    break;
                                case CMD_DEAL_BUY:
                                    {
                                        int purpose = 0;
                                        READ_INT_FROM_MOBJ(purpose,mobj,"purpose");
                                        if (purpose == 2)
                                        {
                                            cdata_.generals_get = 0;
                                            cdata_.can_buy_general.get = 0;
                                        }
                                    }
                                    break;
                                case CMD_GET_FORMATION:
                                    ProcessFormation(mobj);
                                    break;
                                case CMD_SET_FORMAIION:
                                    {
                                        int purpose = 0;
                                        READ_INT_FROM_MOBJ(purpose,mobj,"purpose");
                                        if (purpose == 2)
                                        {
                                            cdata_.generals_get = 0;
                                            cdata_.zhen_generals.get = 0;
                                        }
                                    }
                                    break;
                                case CMD_GET_UPDATE_LIST:
                                    ProcessUpdateList(mobj);
                                    break;
                                case CMD_GET_BOOK_LIST:
                                    ProcessGetBookList(mobj);
                                    break;
                                case CMD_TRAIN_GENERAL:
                                    cdata_.generals_get = 0;
                                    cdata_.updateList.get = 0;
                                    break;
                                default:
                                    INFO(m_account<<" -> !!!!!! unknow cmd "<<cmd<<" !!!!!!"<<endl);
                                    break;
                            }
                        }
                        else
                        {
                            INFO(m_account<<" -> s = "<<ret<<endl);
                            cdata_.mapid = 0;
                            switch (iCmd)
                            {
                                case CMD_ATTACK:
                                    cdata_.sinfo.get = 0;
                                    break;
                            }
                        }
                        break;                    
                }
            }
        }
        tryReadHead();
    }
    else
    {
        tryReadData();
    }
}

void testRobot::handleWrite(const boost::system::error_code& error, size_t bytes_transferred)
{
    //INFO(m_account<<" -> handleWrite() "<<bytes_transferred<<endl);
    --pending_write_;
    assert(pending_write_ == 0);

    if (error)
    {
        std::cout<<m_account<<" -> handleWrite, error "<<error<<",bytes "<<bytes_transferred<<std::endl;
        closeSocket();
    }
    else if (send_queue_.size())
    {
        std::string msg = send_queue_.front();
        send_queue_.pop_front();
        doWrite(msg);
    }
}

void testRobot::processCharList(json_spirit::mObject& o)
{
    json_spirit::mArray clist;
    READ_ARRAY_FROM_MOBJ(clist,o,"charlist");

    for (json_spirit::mArray::iterator it = clist.begin(); it != clist.end(); ++it)
    {
        json_spirit::mValue& value = *it;
        if (value.type() != json_spirit::obj_type)
        {
            INFO(m_account<<":processCharList,value not a obj type"<<endl);
        }
        else
        {
            json_spirit::mObject& mobj = value.get_obj();
            READ_INT_FROM_MOBJ(cdata_.cid, mobj, "id");
            READ_INT_FROM_MOBJ(cdata_.level, mobj, "level");
            std::string name = "";
            READ_STR_FROM_MOBJ(name, mobj, "name");
            if (name != "")
            {
                if (name.length() < 32)
                {
                    memcpy(cdata_.name, name.c_str(), name.length());
                }
                else
                {
                    memcpy(cdata_.name, name.c_str(), 31);
                }
            }
            INFO(m_account<<":processCharList,cid:"<<cdata_.cid<<",level:"<<cdata_.level<<endl);
            return;
        }
    }
}

//处理角色关卡详细信息
void testRobot::ProcessCharStageInfo(json_spirit::mObject& o)
{
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(m_account<<" -> ProcessCharStageInfo,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        int mapid = -1, stageid = -1;
        READ_INT_FROM_MOBJ(mapid,o,"mapid");
        READ_INT_FROM_MOBJ(stageid,o,"stageid");

        if (mapid != -1 && stageid != -1)
        {
            cdata_.strongholds.mapid = mapid;
            cdata_.strongholds.stageid = stageid;
            json_spirit::mArray strongholdList;
            READ_ARRAY_FROM_MOBJ(strongholdList,o,"list");
            
            if (strongholdList.size())
            {
                for (json_spirit::mArray::iterator it = strongholdList.begin(); it != strongholdList.end(); ++it)
                {
                    json_spirit::mValue& value = *it;
                    if (value.type() != json_spirit::obj_type)
                    {
                        ERR();
                    }
                    else
                    {
                        json_spirit::mObject& mobj = value.get_obj();
                        int pos = -1, stronghold = -1, state = -99, level = -1;
                        READ_INT_FROM_MOBJ(stronghold,mobj,"id");
                        READ_INT_FROM_MOBJ(state,mobj,"state");
                        READ_INT_FROM_MOBJ(pos,mobj,"pos");
                        READ_INT_FROM_MOBJ(level,mobj,"level");
                        if (state != -99 && pos > 0 && pos <= 9 && stronghold > 0 && level > 0)
                        {
                            cdata_.strongholds.strongholds[pos - 1].id = stronghold;
                            cdata_.strongholds.strongholds[pos - 1].state = state;
                            cdata_.strongholds.strongholds[pos - 1].level = level;
                        }
                    }
                }
            }
        }        
    }
    else
    {
        INFO(m_account<<" -> ProcessCharStageInfo, fail !!!!!!!!!!!!!"<<ret<<endl);
    }
}

//处理角色关卡详细信息
/*
{"info":{"pos":4,"id":4,"name":"虎妖","level":3,"elite":1,"spic":2004,"state":0,
         "gate_formation":[{"position":-2},{"position":-2},{"position":-2},{"position":-2},{"spic":2004,"name":"虎妖",
               "level":3,"soldier":{"type":1},"position":5},{"position":-2},{"position":-2},{"position":-2},{"position":-2}],
         "needSupply":10,"state_list":[],"default_formation":1},
 "cmd":"getGateInfo","s":200}
*/
void testRobot::ProcessStrongholdInfo(json_spirit::mObject& o)
{
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(m_account<<" -> ProcessStrongholdInfo,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        cdata_.sinfo.clear();
        cdata_.sinfo.get = 1;
        mObject info;
        READ_OBJ_FROM_MOBJ(info,o,"info");
        
        READ_INT_FROM_MOBJ(cdata_.sinfo.mapid,info,"mapid");
        READ_INT_FROM_MOBJ(cdata_.sinfo.stageid,info,"stageid");
        READ_INT_FROM_MOBJ(cdata_.sinfo.pos,info,"pos");
        READ_INT_FROM_MOBJ(cdata_.sinfo.id,info,"id");

        READ_INT_FROM_MOBJ(cdata_.sinfo.need_supply,info,"needSupply");
        READ_INT_FROM_MOBJ(cdata_.sinfo.state,info,"state");
        if (cdata_.sinfo.state == 0
            && m_last_attack_time < (time(NULL) - FIGHT_COOL_TIME)
            && cdata_.supply >= cdata_.sinfo.need_supply
            )
        {
            std::string cmd = strcmdAttackStronghold;
            str_replace(cmd, "$S", LEX_CAST_STR(cdata_.sinfo.id));
            doWrite(cmd);
        }
    }
    else
    {
        INFO(m_account<<" -> ProcessCharStageInfo, fail !!!!!!!!!!!!!"<<ret<<endl);
    }
}

/*{"list":
[{"id":1,"primary":1,"type":1,"name":"击败白蛇","goal":1,"current":0,"isDone":1,
"award":"军令×1","mapid":1,"stageid":1,"pos":1,"state":0}],"cmd":"getCurTask","s":200}
*/
void testRobot::ProcessTaskList(json_spirit::mObject& o)
{
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(m_account<<" -> ProcessTaskList,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        cdata_.tasks_get = 1;
        cdata_.tasks[0].clear();
        cdata_.tasks[1].clear();
        cdata_.tasks[2].clear();
        json_spirit::mArray taskList;
        READ_ARRAY_FROM_MOBJ(taskList,o,"list");
        if (taskList.size() > 0)
        {
            int idx = 0;
            for (json_spirit::mArray::iterator it = taskList.begin(); it != taskList.end(); ++it)
            {
                json_spirit::mValue& value = *it;
                if (value.type() != json_spirit::obj_type)
                {
                    ERR();
                }
                else
                {
                    json_spirit::mObject& mobj = value.get_obj();
                    READ_INT_FROM_MOBJ(cdata_.tasks[idx].id,mobj,"id");
                    READ_INT_FROM_MOBJ(cdata_.tasks[idx].type,mobj,"type");
                    READ_INT_FROM_MOBJ(cdata_.tasks[idx].primary,mobj,"primary");
                    READ_INT_FROM_MOBJ(cdata_.tasks[idx].goal,mobj,"goal");
                    READ_INT_FROM_MOBJ(cdata_.tasks[idx].done,mobj,"isDone");
                    READ_INT_FROM_MOBJ(cdata_.tasks[idx].current,mobj,"current");
                    READ_INT_FROM_MOBJ(cdata_.tasks[idx].pos,mobj,"pos");
                    READ_INT_FROM_MOBJ(cdata_.tasks[idx].mapid,mobj,"mapid");
                    READ_INT_FROM_MOBJ(cdata_.tasks[idx].stageid,mobj,"stageid");
                    READ_INT_FROM_MOBJ(cdata_.tasks[idx].state,mobj,"state");
                    ++idx;
                    //最多记录三个任务
                    if (idx > 2)
                    {
                        break;
                    }
                }
            }
        }
    }
}

//[INFO] Input-------------------------
//{"list":[{"id":1585,"level":3,"color":0,"name":"许褚","spic":22}],
//"type":0,"page":{"maxPage":1,"page":1,"pageNums":15},"maxNums":3,"nums":1,"cmd":"getGeneralList","s":200}
void testRobot::ProcessGeneralList(mObject& o)
{
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(m_account<<" -> ProcessGeneralList,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        cdata_.generals_get = 1;
        cdata_.generals_count = 0;
        for (int i = 0; i < 10; ++i)
        {
            cdata_.generals[i].clear();
        }
        json_spirit::mArray xList;
        READ_ARRAY_FROM_MOBJ(xList,o,"list");
        if (xList.size() > 0)
        {
            int idx = 0;
            for (json_spirit::mArray::iterator it = xList.begin(); it != xList.end(); ++it)
            {
                json_spirit::mValue& value = *it;
                if (value.type() != json_spirit::obj_type)
                {
                    ERR();
                }
                else
                {
                    json_spirit::mObject& mobj = value.get_obj();
                    READ_INT_FROM_MOBJ(cdata_.generals[idx].gid,mobj,"id");
                    READ_INT_FROM_MOBJ(cdata_.generals[idx].level,mobj,"level");
                    READ_INT_FROM_MOBJ(cdata_.generals[idx].type,mobj,"spic");
                    ++idx;
                    ++cdata_.generals_count;
                    //最多记录10个武将
                    if (idx > 9)
                    {
                        break;
                    }
                }
            }
        }
    }
}

//{"equipList":[{"itype":3,"id":2762,"quality":1,"name":"铁指环","type":1,"spic":1,"type":1,"level":0,"slot":1}],
//"type":3,"cmd":"getStoreList","s":200}
//显示包中的装备
void testRobot::ProcessEquipmentlist(mObject& o)
{
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(m_account<<" -> ProcessEquipmentlist,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        cdata_.equipts_get = 1;
        for (int i = 0; i < 10; ++i)
        {
            cdata_.equipts[i].clear();
        }
        json_spirit::mArray xList;
        READ_ARRAY_FROM_MOBJ(xList,o,"equipList");
        if (xList.size() > 0)
        {
            int idx = 0;
            for (json_spirit::mArray::iterator it = xList.begin(); it != xList.end(); ++it)
            {
                json_spirit::mValue& value = *it;
                if (value.type() != json_spirit::obj_type)
                {
                    ERR();
                }
                else
                {
                    json_spirit::mObject& mobj = value.get_obj();
                    int type = 0;
                    READ_INT_FROM_MOBJ(type, mobj, "itype");
                    if (type == 3)
                    {
                        READ_INT_FROM_MOBJ(cdata_.equipts[idx].id,mobj,"id");
                        READ_INT_FROM_MOBJ(cdata_.equipts[idx].level,mobj,"level");
                        READ_INT_FROM_MOBJ(cdata_.equipts[idx].type,mobj,"type");
                        READ_INT_FROM_MOBJ(cdata_.equipts[idx].slot,mobj,"slot");
                        ++idx;
                        //最多记录10个
                        if (idx > 9)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }    
}

/*
{"roleEquips":[{"id":2762,"type":1,"quality":1,"level":0,"addNums":40,"slot":1,"name":"铁指环","spic":1},
{"slot":2},{"slot":3},{"slot":4},{"slot":5},{"slot":6}],"cmd":"getRoleEquipList","s":200}
*/
void testRobot::ProcessGeneralEquiptments(mObject& o)
{
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(m_account<<" -> ProcessGeneralEquiptments,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        int gid = 0;
        READ_INT_FROM_MOBJ(gid,o,"gid");
        if (gid == 0)
        {
            return;
        }
        int gidx = -1;
        for (int i = 0; i < 10; ++i)
        {
            if (cdata_.generals[i].gid == gid)
            {
                gidx = i;
                break;
            }
        }
        if (gidx == -1)
        {
            return;
        }

        cdata_.generals[gidx].equips_get = 1;
        for (int i = 0; i < 6; ++i)
        {
            cdata_.generals[gidx].equips[i].clear();
        }
        json_spirit::mArray xList;
        READ_ARRAY_FROM_MOBJ(xList,o,"roleEquips");
        if (xList.size() > 0)
        {
            for (json_spirit::mArray::iterator it = xList.begin(); it != xList.end(); ++it)
            {
                json_spirit::mValue& value = *it;
                if (value.type() != json_spirit::obj_type)
                {
                    ERR();
                }
                else
                {
                    json_spirit::mObject& mobj = value.get_obj();
                    int slot = 0;
                    READ_INT_FROM_MOBJ(slot, mobj, "slot");
                    if (slot >=1 && slot <= 6)
                    {
                        READ_INT_FROM_MOBJ(cdata_.generals[gidx].equips[slot-1].id,mobj,"id");
                        READ_INT_FROM_MOBJ(cdata_.generals[gidx].equips[slot-1].level,mobj,"level");
                        READ_INT_FROM_MOBJ(cdata_.generals[gidx].equips[slot-1].type,mobj,"type");
                        cdata_.generals[gidx].equips[slot-1].slot = slot;
                    }
                }
            }
        }
        
    }    
}

//{"mapid":1,"stageid":1,"can_get":true,"cmd":"checkStageFinish","s":200}
void testRobot::ProcessStageFinish(mObject& o)
{
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(m_account<<" -> ProcessStageFinish,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        int mapid = 0, stageid = 0;
        bool can_get = false;
        READ_INT_FROM_MOBJ(mapid,o,"mapid");
        READ_INT_FROM_MOBJ(stageid,o,"stageid");
        READ_BOOL_FROM_MOBJ(can_get,o,"can_get");

        if (can_get)
        {
            std::string cmd = "{\"stageid\":$S,\"cmd\":\"getStageFinishLoot\",\"mapid\":$M}";
            str_replace(cmd, "$M", LEX_CAST_STR(mapid));
            str_replace(cmd, "$S", LEX_CAST_STR(stageid));
            //领取
            doWrite(cmd);
        }

        if ((mapid * 3 + stageid) < (cdata_.mapid * 3 + cdata_.stageid))
        {
            cdata_.stage_reward.stageid++;
            if (cdata_.stage_reward.stageid == 4)
            {
                cdata_.stage_reward.stageid = 1;
                cdata_.stage_reward.mapid++;    
            }
        }
        else
        {
            cdata_.stage_reward.get = 1;
        }
    }
}

/*{"list":
   [
       {"off_name":"议郎","off_level":1,"glist":[{"id":2,"name":"典韦","quality":0,"spic":7,"price":0,"canBuy":1}]},
       {"off_name":"偏将军","off_level":2,"glist":
             [
                {"id":4,"name":"贾诩","quality":0,"spic":35,"price":0,"canBuy":0,"prestige":10,"slevel":0}
            ]
        }
   ],
   "cmd":"getPostHeroList","s":200}
*/
void testRobot::ProcessCanBuyGeneralList(mObject& o)
{
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(m_account<<" -> ProcessCanBuyGeneralList,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        cdata_.can_buy_general.clear();
        cdata_.can_buy_general.get = 1;
        json_spirit::mArray xList;
        READ_ARRAY_FROM_MOBJ(xList,o,"list");
        if (xList.size() > 0)
        {
            for (json_spirit::mArray::iterator it = xList.begin(); it != xList.end(); ++it)
            {
                json_spirit::mValue& value = *it;
                if (value.type() != json_spirit::obj_type)
                {
                    ERR();
                }
                else
                {
                    json_spirit::mObject& mobj = value.get_obj();
                    json_spirit::mArray gList;
                    READ_ARRAY_FROM_MOBJ(gList,mobj,"glist");
                    if (gList.size() > 0)
                    {
                        for (json_spirit::mArray::iterator it = gList.begin(); it != gList.end(); ++it)
                        {
                            json_spirit::mValue& value = *it;
                            if (value.type() != json_spirit::obj_type)
                            {
                                ERR();
                            }
                            else
                            {
                                json_spirit::mObject& mobj = value.get_obj();
                                int canBuy = 0;
                                READ_INT_FROM_MOBJ(canBuy,mobj,"canBuy");
                                if (canBuy == 1)
                                {
                                    int id = 0, price = 0;
                                    READ_INT_FROM_MOBJ(id,mobj,"id");
                                    READ_INT_FROM_MOBJ(price,mobj,"price");
                                    cdata_.can_buy_general.get = 1;
                                    cdata_.can_buy_general.type = id;
                                    cdata_.can_buy_general.price = price;
                                    return;
                                }
                            }
                        }
                    }                    
                }
            }            
        }
        
    }
}

/* 
{"list":[{"position":-2},
        {"id":1588,"level":11,"color":0,"isOriginal":true,"name":"典韦","spic":7,"soldier":{"type":1},"position":1588},
        {"position":-2},
        {"position":-2},
        {"id":1585,"level":11,"color":0,"isOriginal":true,"name":"许褚","spic":22,"soldier":{"type":1},"position":1585},
        {"position":0},
        {"position":-1},
        {"position":-2},
        {"position":-1}],"attack":236,"cmd":"getFormation","s":200}
*/
void testRobot::ProcessFormation(mObject& o)
{
    cdata_.zhen_generals.clear();
    json_spirit::mArray xList;
    READ_ARRAY_FROM_MOBJ(xList,o,"list");
    if (xList.size() > 0)
    {
        cdata_.zhen_generals.get = 1;
        int idx = 0;
        for (json_spirit::mArray::iterator it = xList.begin(); it != xList.end(); ++it)
        {
            json_spirit::mValue& value = *it;
            if (value.type() != json_spirit::obj_type)
            {
                ERR();
            }
            else
            {
                json_spirit::mObject& mobj = value.get_obj();
                int gid = 0;
                READ_INT_FROM_MOBJ(gid,mobj,"position");
                cdata_.zhen_generals.gid[idx] = gid;
                if (gid == 0)
                {
                    ++(cdata_.zhen_generals.left);
                }
                else if (gid > 0)
                {
                    ++(cdata_.zhen_generals.total);
                }
                ++idx;
                if (idx >= 9)
                {
                    return;
                }
            }
        }
    }
}

//处理左侧队列信息
void testRobot::ProcessUpdateList(mObject& o)
{
    cdata_.updateList.clear();
    cdata_.updateList.get = 1;
    json_spirit::mArray xList;
    READ_ARRAY_FROM_MOBJ(xList,o,"list");
    if (xList.size())
    {
        for (json_spirit::mArray::iterator it = xList.begin(); it != xList.end(); ++it)
        {
            json_spirit::mValue& value = *it;
            if (value.type() != json_spirit::obj_type)
            {
                ERR();
            }
            else
            {
                json_spirit::mObject& mobj = value.get_obj();
                int type = 0;
                READ_INT_FROM_MOBJ(type,mobj,"type");
                if (type >= 1 && type <= 10)
                {
                    cdata_.updateList.cd[type-1][0] = 1;
                    if (type == 8 || 9 == type)
                    {
                        READ_INT_FROM_MOBJ(cdata_.updateList.cd[type-1][1],mobj ,"id");
                    }
                    else
                    {
                        READ_INT_FROM_MOBJ(cdata_.updateList.cd[type-1][1],mobj ,"state");
                    }
                }
            }
        }
    }
}

/*
{"ginfo":{"id":1589,"rebornOpen":0,
        "level":1,"quality":0,"growRate":4.0000000000000000,
        "growRate_max":8.0000000000000000,"rebornLv":35},
 "info":{"silver":3000},
 "list":[{"id":2,"name":"战国策","quality":0,"memo":"","addLevel":5,"trainTime":5},
         {"id":3,"name":"典论","quality":0,"memo":"","addLevel":7,"trainTime":7},
         {"id":7,"name":"太平清领道","quality":1,"memo":"","addLevel":8,"trainTime":3}],
 "cmd":"getBookList","s":200}

*/
void testRobot::ProcessGetBookList(mObject& o)
{
    cdata_.updateList.cd[2][0] = 0;
    mObject ginfo;
    READ_OBJ_FROM_MOBJ(ginfo,o,"ginfo");
    int level = 0;
    READ_INT_FROM_MOBJ(level,ginfo,"level");
    if (level >= cdata_.level)
    {
        return;
    }
    mObject info;
    READ_OBJ_FROM_MOBJ(info,o,"info");
    int silver = 0;
    READ_INT_FROM_MOBJ(silver,info,"silver");
    if (silver > cdata_.silver)
    {
        return;
    }
    json_spirit::mArray xList;
    READ_ARRAY_FROM_MOBJ(xList,o,"list");
    int id = 0, quality = -1;
    if (xList.size() > 0)
    {
        for (json_spirit::mArray::iterator it = xList.begin(); it != xList.end(); ++it)
        {
            json_spirit::mValue& value = *it;
            if (value.type() != json_spirit::obj_type)
            {
                ERR();
            }
            else
            {
                json_spirit::mObject& mobj = value.get_obj();
                int book_id = 0, book_quality = 0;
                READ_INT_FROM_MOBJ(book_id,mobj,"id");
                READ_INT_FROM_MOBJ(book_quality,mobj,"quality");
                if (book_quality > quality)
                {
                    id = book_id;
                    quality = book_quality;
                }
            }
        }
    }

    int gid = 0;
    READ_INT_FROM_MOBJ(gid,ginfo,"id");

    std::string cmd = "{\"bid\":$I,\"cmd\":\"trainGeneral\",\"gid\":$G}";
    str_replace(cmd, "$G", LEX_CAST_STR(gid));
    str_replace(cmd, "$I", LEX_CAST_STR(id));
    doWrite(cmd);    
}

//查询关卡信息
int testRobot::queryStageInfo()
{
    INFO(" ****** getStageInfo ****** "<<endl);
    std::string cmd = strcmdGetStageInfo;
    str_replace(cmd, "$M", LEX_CAST_STR(cdata_.mapid));
    str_replace(cmd, "$S", LEX_CAST_STR(cdata_.stageid));
    doWrite(cmd);
    return 0;
}


int testRobot::attackStronghold()    //攻击关卡
{
    if (cdata_.ling > 0 && cdata_.mapid > 0 && cdata_.stageid > 0)
    {
        int attack_id = 1;
        int attack_level = 1;
        for (int i = 0; i < 9; ++i)
        {
            if (cdata_.strongholds.strongholds[i].state >= 0 && cdata_.strongholds.strongholds[i].level > attack_level)
            {
                attack_id = cdata_.strongholds.strongholds[i].id;
                attack_level = cdata_.strongholds.strongholds[i].level;
            }
        }
        INFO(m_account<<" -> attack stronghold "<<attack_id<<" ****** "<<endl);
        std::string cmd = strcmdAttackStronghold;
        str_replace(cmd, "$S", LEX_CAST_STR(attack_id));
        doWrite(cmd);
        return attack_id;
    }
    else
    {
        INFO(m_account<<" -> can not attack stronghold, ling "<<cdata_.ling<<",map:"<<cdata_.mapid<<",stage:"<<cdata_.stageid<<endl);
    }
    return 0;
}

int testRobot::checkUpEquipment()
{
    for (int i = 0; i < 10; ++i)
    {
        if (cdata_.equipts[i].id > 0)
        {
            for (int g = 0; g < 10; ++g)
            {
                if (cdata_.generals[g].gid > 0 && cdata_.generals[g].equips_get > 0)
                {
                    for (int s = 0; s < 6; ++s)
                    {
                        if (cdata_.equipts[i].type == (s+1)
                            && (cdata_.generals[g].equips[s].id == 0
                                || (cdata_.generals[g].equips[s].id < cdata_.equipts[i].id)
                                || (cdata_.generals[g].equips[s].id == cdata_.equipts[i].id
                                    && (cdata_.generals[g].equips[s].level < cdata_.equipts[i].level)
                                   )
                                )
                            )
                        {
                            //可以装备
                            std::string msg = strcmdEquipItem;
                            str_replace(msg, "$G", LEX_CAST_STR(cdata_.generals[g].gid));
                            str_replace(msg, "$E", LEX_CAST_STR(cdata_.equipts[i].id));
                            doWrite(msg);
                            return 1;
                        }
                    }
                    
                }
            }
        }
    }    
    return 0;
}

//检查是否可以强化装备
int testRobot::checkEnhanceEquipment()
{
    if (cdata_.updateList.get == 0)
    {
        doWrite(strcmdGetUpdateList);
        return 1;
    }
    else if (cdata_.updateList.cd[7][0] == 1)
    {
        //强化装备
        std::string cmd = strcmdEnhanceEquiptment;
        str_replace(cmd, "$E", LEX_CAST_STR(cdata_.updateList.cd[7][1]));
        doWrite(cmd);
        cdata_.updateList.get = 0;
        return 1;
    }
    else if (cdata_.updateList.cd[8][0] == 1)
    {
        //升级秘法
        std::string cmd = strcmdLevelupSkill;
        str_replace(cmd, "$T", LEX_CAST_STR(cdata_.updateList.cd[8][1]));
        doWrite(cmd);
        cdata_.updateList.get = 0;
        return 1;
        
    }
    return 0;
}

//是否可以升级秘法
int testRobot::checkCanLevelupSkill()
{
    return 0;
}

//是否可以完成扫荡任务
int testRobot::checkCanSweep()
{
    return 0;
}

//是否可以招募武将
int testRobot::checkCanBuyGeneral()
{
    //请求可招募武将
    if (cdata_.can_buy_general.get == 0)
    {
        doWrite(strcmdCanBuyGeneralList);
        return 1;
    }
    else if (cdata_.can_buy_general.type > 0 && cdata_.can_buy_general.price <= cdata_.silver)
    {
        //购买武将
        std::string cmd = strcmdBuyGeneral;
        str_replace(cmd, "$I", LEX_CAST_STR(cdata_.can_buy_general.type));
        doWrite(cmd);
        return 1;
    }
    return 0;
}

//武将是否可以上阵
int testRobot::checkCanUpZhen()
{
    if (cdata_.zhen_generals.get == 0)
    {
        doWrite(strcmdGetZhenGenerals);
        return 1;
    }
    else
    {
        if (cdata_.zhen_generals.left > 0 && cdata_.generals_count > cdata_.zhen_generals.total)
        {
            for (int i = 0; i < 10; ++i)
            {
                if (cdata_.generals[i].gid > 0)
                {
                    bool alreadyUp = false;
                    int left_pos = 0;
                    for (int p = 0; p < 9; ++p)
                    {
                        if (cdata_.zhen_generals.gid[p] == cdata_.generals[i].gid)
                        {
                            alreadyUp = true;
                            break;
                        }
                        else if (cdata_.zhen_generals.gid[p] == 0)
                        {
                            left_pos = p + 1;
                        }
                    }
                    if (!alreadyUp && left_pos > 0)
                    {
                        std::string cmd = strcmdSetFormation;
                        str_replace(cmd, "$P", LEX_CAST_STR(2));
                        str_replace(cmd, "$1", LEX_CAST_STR(left_pos));
                        str_replace(cmd, "$2", "0");
                        str_replace(cmd, "$G", LEX_CAST_STR(cdata_.generals[i].gid));
                        doWrite(cmd);
                        return 1;
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
    return 0;
}

//武将是否可以训练升级
int testRobot::checkCanTrain()
{
    if (cdata_.updateList.cd[2][0] == 1 && cdata_.updateList.cd[2][1] == 1)
    {
        for (int i = 0; i < 10; ++i)
        {
            if (cdata_.generals[i].gid > 0 && cdata_.generals[i].level < cdata_.level)
            {
                std::string cmd = "{\"cmd\":\"getBookList\",\"gid\":$G}";
                str_replace(cmd, "$G", LEX_CAST_STR(cdata_.generals[i].gid));
                doWrite(cmd);                
                return 1;
            }
        }
    }
    return 0;
}

void testRobot::closeSocket()
{    
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket_.close(ignored_ec);

    if (pending_read_ == 0 && pending_write_ == 0)
    {
        send_queue_.clear();
        encryption_ = 0;
        state_ = CHAR_STATE_CONNECT_CLOSED;
    }
}

/*

[INFO] Input-------------------------
{"cmd":"login","s":200,"login":1}

[INFO] Output-------------------------
{"extra1":"","cmd":"login","extra2":"","time":1352883884,"qid":"wlj","sign":"69c371f58c7f8d955958a0fd92c70993","union_id":10000,"server_id":"s1","isAdult":1,"qname":"wlj"}

[INFO] Input-------------------------
{"login":2,"cmd":"login","s":200}

[INFO] Output-------------------------
{"cmd":"charlist"}

[INFO] Input-------------------------
{"cmd":"charlist","charlist":[{"id":13,"name":"闵夜柳","level":120,"spic":3,"lastlogin":1352883875,"state":0}],"s":200}

[INFO] Output-------------------------
{"id":13,"cmd":"roleLogin"}

[INFO] Input-------------------------
{"cmd":"roleLogin","s":200}

[WARNING] 角色登录成功................
[INFO] Output-------------------------
{"cmd":"qdate"}
[INFO] Output-------------------------
{"cmd":"panel"}
[INFO] Output-------------------------
{"cmd":"getRoleInfo"}
[INFO] Output-------------------------
{"cmd":"getCurTask"}
[INFO] Output-------------------------
{"cmd":"getActionInfo"}
[INFO] Output-------------------------
{"cmd":"getUnread"}
[INFO] Output-------------------------
{"cmd":"queryNotices"}
[INFO] Output-------------------------
{"cmd":"queryUnGetGifts"}
[INFO] Output-------------------------
{"cmd":"getUpdateList"}
[INFO] Output-------------------------
{"cmd":"getOpenInfo"}

*/

