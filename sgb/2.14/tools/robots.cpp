
#include "robots.h"
#include <execinfo.h>
#include "inc.h"
#include <boost/asio/buffer.hpp>
#include "json_spirit.h"
#include "utils_all.h"
#include <syslog.h>
#include "spls_errcode.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include <pthread.h>
#include <sys/syscall.h>
#include <boost/random.hpp>
#include <boost/thread.hpp>


//#define INFO(x) cout<<x

using namespace std;
using namespace net;
using namespace json_spirit;
using namespace boost;

static std::string g_db_name = "spls";
static std::string g_exe_name = "shhx";
static int m_socket_port = 80;
static int g_mysql_option = 0;
static int g_mysql_port = 3306;

static std::string g_char_link = "<font color='#FFFF00'><A HREF=\"event:{'cmd':'showRole','name':'$N'}\"><U>$n</U></A></font>";

using namespace std;

pthread_key_t thread_random_gen_key;
pthread_key_t thread_db_key;
extern void close_thread_db(void* db);

void close_thread_random_gen(void* pgen)
{
    boost::mt19937* pg = reinterpret_cast<boost::mt19937*>(pgen);
    if (pg)
    {
        delete pg;
    }
}

void SetDbname(const std::string& dbname)
{
    g_db_name = dbname;
}

std::string GetDbName()
{
    return g_db_name;
}

void SetExeName(const std::string& ename)
{
    g_exe_name = ename;
}

std::string GetExeName()
{
    return g_exe_name;
}

int mysqlOption()
{
    return g_mysql_option;
}

int mysqlPort()
{
    return g_mysql_port;
}

void mysqlPort(int port)
{
    g_mysql_port = port;
}

void SetPort(int port)
{
    m_socket_port = port;
}

int GetPort()
{
    return m_socket_port;
}

void init_random_seed()
{
    srand((int)time(0));
}

boost::mt19937* get_random_generator()
{
    boost::mt19937* pgen = reinterpret_cast<boost::mt19937*>(pthread_getspecific (thread_random_gen_key));
    if (!pgen)
    {
        pgen = new boost::mt19937(42u);//, CLIENT_INTERACTIVE);
        pthread_setspecific (thread_random_gen_key, pgen);
        pgen->seed(time(NULL));    //初始化随机数
    }
    return pgen;
}

int my_random(int mins, int maxs)
{
    if (mins >= maxs)
    {
        return mins;
    }
    boost::mt19937* pgen = get_random_generator();
    if (pgen)
    {
        boost::uniform_int<> dist(mins, maxs);
        boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(*pgen, dist);
        return die();
    }
    else
    {
        ERR();
        return mins + rand()%(maxs- mins + 1);
    }
}

double my_random(double mins, double maxs)
{
    int mins_int = (int)(10000.0 * mins);
    int maxs_int = (int)(10000.0 * maxs);
    int ret = my_random(mins_int, maxs_int);
    return (double)(ret/10000.0);
    //return (maxs > mins) ? (mins + (double)(g_uni() * (maxs + 1 - mins))) : mins;
}

void do_sleep(int ms)
{
    boost::xtime xt;
    boost::xtime_get(&xt,boost::TIME_UTC_);
    xt.nsec += ms%1000*1000*1000;
    xt.sec += ms/1000;
    boost::thread::sleep(xt);
}

bool str_replace(std::string& msg, const std::string& toreplace, const std::string& replacemsg, bool rfind)
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


#define INFO(x)
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

#if 0
#define DUMP(x, len) for (size_t i = 0; i < len; ++i)\
{\
    char* p = static_cast<char*>(x + i);\
    cout<<"0x"<<hex<<(int)(*p)<<",";\
}\
cout<<endl
#else
#define DUMP(x,len)
#endif

#define NET_PACK_START_STRING "FEESEE!!"

#define MAX_CONNECT_EVERY_IP 200
#define MAX_SESSITONS 5000


static std::map<std::string,int> g_ip_connects;
static std::map<std::string,int> g_free_connects;
static std::map<std::string,int> g_filter_ip;

volatile int g_print_debug_info = 1;

int add_ip_connect(const std::string& ip)
{
    if (g_free_connects.find(ip) != g_free_connects.end()
        || g_ip_connects[ip] < MAX_CONNECT_EVERY_IP)
    {
        g_ip_connects[ip] = g_ip_connects[ip] + 1;
        return g_ip_connects[ip];
    }
    else
    {
        if (g_filter_ip.find(ip) != g_filter_ip.end())
        {
            ++g_filter_ip[ip];
        }
        else
        {
            g_filter_ip[ip] = 1;
            //syslog
            syslog (LOG_ERR, "ip %s too mush connects.", ip.c_str());
        }
        return -1;
    }
}

void sub_ip_connect(const std::string& ip)
{
    if (g_ip_connects[ip] > 0)
    {
        g_ip_connects[ip] = g_ip_connects[ip] - 1;
    }
}

void accountOffline(int cid)
{
    //void(cid);
}


namespace net 
{

//处理登录信息
void ProcessTestLogin(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessTestLogin message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    
    int login_type  = 0;
    READ_INT_FROM_MOBJ(login_type,o,"login");
    if (1 == login_type)
    {
        if (pUser->m_state == robot_try_connected2)
        {
            std::string msg = strLoginMessage;
            str_replace(msg, "$U", pUser->_account);
            //str_replace(msg, "$P", pUser->_password);
            psession->send(msg);
            return;
        }
        else
        {
            INFO(pUser->_account<<":HC_ERROR_LOGOUT_FIRST"<<endl);
        }
    }
    else
    {
        int ret = 0;
        READ_INT_FROM_MOBJ(ret,o,"s");
        if (ret == HC_SUCCESS)
        {
            psession->state(STATE_AUTHED);
            INFO(pUser->_account<<":query char list"<<endl);
            pUser->m_state = robot_logined1;
            psession->send(strCmdCharList);
            return;
        }
        else
        {
            INFO("account not exist or wrong password!("<<pUser->_account<<")"<<endl);
        }
        return;
    }
}

//处理角色列表
void ProcessCharListResult(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharListResult message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_try_login2)
    {
        INFO(pUser->_account<<":ProcessCharListResult message,state="<<pUser->_state<<endl);
        return;
    }
    pUser->_charList.clear();
    json_spirit::mArray clist;
    READ_ARRAY_FROM_MOBJ(clist,o,"charlist");
    pUser->m_state = robot_logined2;
    for (json_spirit::mArray::iterator it = clist.begin(); it != clist.end(); ++it)
    {
        json_spirit::mValue& value = *it;
        if (value.type() != obj_type)
        {
            INFO(pUser->_account<<":ProcessCharListResult,value not a obj type"<<endl);
        }
        else
        {
            mObject& mobj = value.get_obj();
            CharactorInfo cInfo;
            READ_INT_FROM_MOBJ(cInfo.m_cid,mobj,"id");
            READ_INT_FROM_MOBJ(cInfo.m_level,mobj,"level");
            READ_INT_FROM_MOBJ(cInfo.m_state,mobj,"state");
            READ_INT_FROM_MOBJ(cInfo.m_spic,mobj,"spic");
            READ_STR_FROM_MOBJ(cInfo.m_name,mobj,"name");
            pUser->_charList.push_back(cInfo);
            INFO(pUser->_account<<":ProcessCharListResult,cid:"<<cInfo.m_cid<<",level:"<<cInfo.m_level<<endl);
        }
    }
    INFO(pUser->_account<<":ProcessCharListResult, total charactors "<<pUser->_charList.size()<<endl);
}

//处理角色创建选择列表
void ProcessCreateListResult(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCreateListResult message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_logined2)
    {
        INFO(pUser->_account<<":ProcessCreateListResult message,state="<<pUser->_state<<endl);
        return;
    }
    int rolNum = 0;
    READ_INT_FROM_MOBJ(rolNum,o,"rolenum");
    json_spirit::mArray alist;
    READ_ARRAY_FROM_MOBJ(alist,o,"alist");
    json_spirit::mArray dlist;
    READ_ARRAY_FROM_MOBJ(dlist,o,"dlist");
    int g1 = 4, g2 = 3;
    std::string name = "shhx";
    int spic = 1;
    if (rolNum > 1)
    {
        spic = my_random(1, rolNum);
    }
    for (int i = 0; i < 9; ++i)
    {
        char c = my_random('0', '9');
        name += c;
    }
    if (alist.size() > 0)
    {
        json_spirit::mArray::iterator it = alist.begin();
        int randIdx = my_random(0, alist.size()-1);
        for (int i = 0; i < randIdx; ++i)
        {
            ++it;
        }
        if (it->type() == obj_type)
        {
            json_spirit::mObject& mobj = it->get_obj();
            READ_INT_FROM_MOBJ(g1,mobj,"id");
        }
        else
        {
            INFO("a list not obj"<<endl);
        }
    }
    if (dlist.size() > 0)
    {
        json_spirit::mArray::iterator it = dlist.begin();
        int randIdx = my_random(0, dlist.size()-1);
        for (int i = 0; i < randIdx; ++i)
        {
            ++it;
        }
        if (it->type() == obj_type)
        {
            json_spirit::mObject& mobj = it->get_obj();
            READ_INT_FROM_MOBJ(g2,mobj,"id");
        }
        else
        {
            INFO("d list not obj"<<endl);
        }
    }
    INFO(pUser->_account<<" create char:"<<name<<",g1="<<g1<<",g2="<<g2<<",spic="<<spic<<endl);
    std::string cmd = strcmdCreateChar;
    str_replace(cmd, "$N", name);
    str_replace(cmd, "$S", LEX_CAST_STR(spic));
    str_replace(cmd, "$A", LEX_CAST_STR(g1));
    str_replace(cmd, "$D", LEX_CAST_STR(g2));
    psession->send(cmd);
}

//处理角色创成功
void ProcessCreateSuccess(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCreateSuccess message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_logined2)
    {
        INFO(pUser->_account<<":ProcessCreateSuccess message,state="<<pUser->m_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<":ProcessCreateSuccess,"<<ret<<endl);
    psession->send(strCmdCharList);
}

//处理角色登录成功
void ProcessCharLoginSuccess(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharLoginSuccess message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_try_connected2)
    {
        INFO(pUser->_account<<":ProcessCharLoginSuccess message,state="<<pUser->m_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<":ProcessCharLoginSuccess,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        pUser->m_state = robot_logined2;
    }
    else
    {
        cout<<pUser->_account<<":ProcessCharLoginSuccess, fail !!!!!!!!!!!!!"<<ret<<endl;
    }
}

//处理角色信息
void ProcessCharInfo(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharInfo message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_logined2)
    {
        INFO(pUser->_account<<":ProcessCharInfo message,state="<<pUser->m_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<":ProcessCharInfo,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        json_spirit::mObject charInfoObj;
        READ_OBJ_FROM_MOBJ(charInfoObj,o,"chardata");
        if (charInfoObj.size())
        {
            int id = -1, gid = -1, vip = -1, gold = -1, silver = -1, ling = -1, level = -1;
            READ_INT_FROM_MOBJ(id,charInfoObj,"id");
            READ_INT_FROM_MOBJ(gid,charInfoObj,"gid");
            READ_INT_FROM_MOBJ(vip,charInfoObj,"vip");
            READ_INT_FROM_MOBJ(gold,charInfoObj,"gold");
            READ_INT_FROM_MOBJ(silver,charInfoObj,"silver");
            READ_INT_FROM_MOBJ(ling,charInfoObj,"ling");
            READ_INT_FROM_MOBJ(level,charInfoObj,"level");
            if (id == pUser->m_cid)
            {
                if (gid != -1)
                {
                    pUser->m_corps = gid;
                }
                if (gold != -1)
                {
                    pUser->m_gold = gold;
                }
                if (level != -1)
                {
                    pUser->m_level = level;
                }
                if (silver != -1)
                {
                    pUser->m_silver = silver;
                }
                if (vip != -1)
                {
                    pUser->m_vip = vip;
                }
                if (ling != -1)
                {
                    pUser->m_ling = ling;
                }
            }
            json_spirit::mObject areaObj;
            READ_OBJ_FROM_MOBJ(areaObj,charInfoObj,"area");
            if (areaObj.size())
            {
                int area = -1;
                READ_INT_FROM_MOBJ(area,areaObj,"area");
                if (area != -1)
                {
                    pUser->m_mapid = area;
                }
            }
        }
    }
    else
    {
        cout<<pUser->_account<<":ProcessCharInfo, fail !!!!!!!!!!!!!"<<ret<<endl;
    }
}

//处理角色地图信息
void ProcessCharMapInfo(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharMapInfo message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_logined2)
    {
        INFO(pUser->_account<<":ProcessCharMapInfo message,state="<<pUser->m_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<":ProcessCharMapInfo,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        int area = -1;
        READ_INT_FROM_MOBJ(area,o,"mapid");
        if (area != -1)
        {
            if (pUser->m_mapid < area)
            {
                pUser->m_mapid = area;
            }
        }
        json_spirit::mArray stageList;
        READ_ARRAY_FROM_MOBJ(stageList,o,"list");
        INFO(" char mapid "<<pUser->_mapid<<",stage "<<pUser->m_stage<<endl);
        INFO(" stagelist size "<<stageList.size()<<endl);
        if (stageList.size())
        {
            for (json_spirit::mArray::iterator it = stageList.begin(); it != stageList.end(); ++it)
            {
                json_spirit::mValue& value = *it;
                if (value.type() != obj_type)
                {
                    ERR();
                }
                else
                {
                    json_spirit::mObject& mobj = value.get_obj();
                    int stageId = -1, state = -1, curGates = -1, totalGates = -1;
                    READ_INT_FROM_MOBJ(stageId,mobj,"id");
                    READ_INT_FROM_MOBJ(state,mobj,"state");
                    READ_INT_FROM_MOBJ(curGates,mobj,"cur_gate");
                    READ_INT_FROM_MOBJ(totalGates,mobj,"total_gate");

                    INFO("stageId "<<stageId<<",state "<<state<<endl);
                    if (state > 0 && stageId > pUser->m_stage)
                    {
                        pUser->m_stage = stageId;
                    }
                }
            }
        }
    }
    else
    {
        cout<<pUser->_account<<":ProcessCharMapInfo, fail !!!!!!!!!!!!!"<<ret<<endl;
    }
}

//处理角色关卡详细信息
void ProcessCharStageInfo(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharStageInfo message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_logined2)
    {
        INFO(pUser->_account<<":ProcessCharStageInfo message,state="<<pUser->m_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<":ProcessCharStageInfo,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        int mapid = -1, stageid = -1;
        READ_INT_FROM_MOBJ(mapid,o,"mapid");
        READ_INT_FROM_MOBJ(stageid,o,"stageid");

        if (mapid != -1 && stageid != -1)
        {
            INFO("current map "<<mapid<<",current stage "<<stageid<<endl);
            pUser->m_cur_mapid = mapid;
            pUser->m_cur_stage = stageid;
            json_spirit::mArray strongholdList;
            READ_ARRAY_FROM_MOBJ(strongholdList,o,"list");
            for (int i = 0; i < 25; ++i)
            {
                pUser->_strongholds[i]._id = 0;
                pUser->_strongholds[i]._state = -1;
                pUser->_strongholds[i]._level = 0;
            }
            if (strongholdList.size())
            {
                for (json_spirit::mArray::iterator it = strongholdList.begin(); it != strongholdList.end(); ++it)
                {
                    json_spirit::mValue& value = *it;
                    if (value.type() != obj_type)
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
                        if (state != -99 && pos > 0 && pos <= 25 && stronghold > 0 && level > 0)
                        {
                            INFO("pos "<<pos<<": id = "<<stronghold<<" , level = "<<level<<" , state = "<<state<<endl);
                            pUser->_strongholds[pos - 1]._id = stronghold;
                            pUser->_strongholds[pos - 1]._level = level;
                            pUser->_strongholds[pos - 1]._state = state;
                        }
                    }
                }
            }
        }        
    }
    else
    {
        cout<<pUser->_account<<":ProcessCharStageInfo, fail !!!!!!!!!!!!!"<<ret<<endl;
    }
}

//处理角色官职信息
void ProcessCharOfficalInfo(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharOfficalInfo message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_logined2)
    {
        INFO(pUser->_account<<":ProcessCharOfficalInfo message,state="<<pUser->_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<":ProcessCharOfficalInfo,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        json_spirit::mObject info;
        READ_OBJ_FROM_MOBJ(info,o,"info");
        json_spirit::mObject curOffical;
        READ_OBJ_FROM_MOBJ(curOffical,info,"curPost");
        int camp = 0;
        READ_INT_FROM_MOBJ(camp,curOffical,"camp");
        pUser->m_camp = camp;
    }
    else
    {
        cout<<pUser->_account<<":ProcessCharOfficalInfo, fail !!!!!!!!!!!!!"<<ret<<endl;
    }
}

/*
{"list":[{"type":2,"spic":8001,"id":1,"active":1},
        {"type":3},{"type":7,"active":1},
        {"type":5,"active":1},
        {"type":4,"active":1}],
"cmd":"getActionInfo","s":200}
*/
//处理活动信息
void ProcessActionInfo(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessActionInfo message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_logined2)
    {
        INFO(pUser->_account<<":ProcessActionInfo message,state="<<pUser->m_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<":ProcessActionInfo,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        pUser->resetActionState();
        json_spirit::mArray aList;
        READ_ARRAY_FROM_MOBJ(aList, o, "list");
        if (aList.size())
        {
            for (json_spirit::mArray::iterator it = aList.begin(); it != aList.end(); ++it)
            {
                json_spirit::mValue& value = *it;
                if (value.type() != obj_type)
                {
                    ERR();
                }
                else
                {
                    json_spirit::mObject& mobj = value.get_obj();
                    int type = 0, active = 0;
                    READ_INT_FROM_MOBJ(type,mobj,"type");
                    READ_INT_FROM_MOBJ(active,mobj,"active");
                    switch (type)
                    {
                        case 2:    //boss
                        {
                            int bossId = 0;
                            READ_INT_FROM_MOBJ(bossId,mobj,"id");
                            if (bossId > 0 && bossId <= 10)
                            {
                                pUser->m_boss_state[bossId-1] = active;
                                pUser->m_boss_id = bossId;
                                INFO(pUser->_account<<":ProcessActionInfo,boss "<<bossId<<" is openning"<<endl);
                            }
                            break;
                        }
                        case 3:
                            pUser->m_camp_race_state = 1;
                            INFO(pUser->_account<<":ProcessActionInfo,camp race is openning"<<endl);
                            break;
                        case 5:    //竞技
                            pUser->m_race_state = 1;
                            INFO(pUser->_account<<":ProcessActionInfo,race is opened"<<endl);
                            break;
                        case 7:    //通商
                            pUser->m_trade_state = 1;
                            INFO(pUser->_account<<":ProcessActionInfo,trade is opened"<<endl);
                            break;
                    }
                }
            }

        }
    }
}

//处理攻击boss结果
void ProcessAttackBossResult(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessAttackBossResult message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_logined2)
    {
        INFO(pUser->_account<<"ProcessAttackBossResult message,state="<<pUser->m_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<"ProcessAttackBossResult,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
    }
    else if (HC_ERROR_BOSS_EVENT_END == ret)
    {
        memset(pUser->m_boss_state, 0, sizeof(int)*10);
        pUser->m_boss_id = 0;
        if (pUser->m_my_action == action_in_boss)
        {
            pUser->m_my_action = action_idle;
        }
    }
    else if (HC_ERROR_BOSS_NOT_ENTER == ret)
    {
        pUser->enterBoss(pUser->m_boss_id);
    }
}

//处理进入boss场景结果
void ProcessEnterBossResult(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessEnterBossResult message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_logined2)
    {
        INFO(pUser->_account<<"ProcessEnterBossResult message,state="<<pUser->m_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<"ProcessEnterBossResult,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        pUser->m_my_action = action_in_boss;
    }
    else
    {
        pUser->queryActionInfo();
    }
}

//处理进入阵营战结果
void ProcessEnterCampResult(session_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<robot> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessEnterCampResult message,no user handle."<<endl);
        return;
    }
    robot* pUser = tUser.get();
    if (pUser->m_state != robot_logined2)
    {
        INFO(pUser->_account<<"ProcessEnterCampResult message,state="<<pUser->_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<"ProcessEnterCampResult,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        pUser->m_my_action = action_in_camp_race;
    }
    else if (HC_ERROR_CAMP_RACE_NO_CAMP == ret)
    {
        pUser->JoinCamp(my_random(1,2));
    }
    else
    {
        pUser->queryActionInfo();
    }
}

//////////////////////////////////////////////////////////////////////////
    recvbuff::recvbuff()
    {
        clear();
    };
    void recvbuff::clear()
    {
        body_length_ = 0;
        point1 = data_;
        point2 = data_;
        state = 0;
    };

    void recvbuff::adddata(void* ad, size_t len)
    {
        #if 0
        char* pc = static_cast<char*>(ad);
        INFO("recv buff add "<<len<<endl);
        DUMP(pc, len);
        #endif
        //剩余buff不够了
        if (len > size_t(data_ + NET_HEAD_SIZE + NET_MAX_BUFF_SIZE - point2))
        {
            INFO("**************move buff*********************"<<endl);
            size_t len_now = size_t(point2 - point1);
            //内存可能重叠
            memmove(data_, point1, len_now);
            point1 = data_;
            point2 = data_ + len_now;
            memset(point2, 0, data_ + NET_HEAD_SIZE + NET_MAX_BUFF_SIZE - point2);
            //空间还是不够
            if (len > size_t(data_ + NET_HEAD_SIZE + NET_MAX_BUFF_SIZE - point2))
            {
                ERR();
                clear();
            }
        }
        memcpy(point2, ad, len);
        point2 += len;
    }
    // 0 继续 1 继续收 -1 断开
    int recvbuff::check(session& se)
    {
        switch (state)
        {
            case 0:     //还未收到固定开始
            {
                    char* ps = strstr(point1, NET_PACK_START_STRING);
                    if (NULL == ps || ps >= point2)
                    {
                        //找不到开始，把前面的全扔了，剩下8个字节

                        //如果超过8个
                        if (point2 - point1 > 8)
                        {
                            memmove(data_, point1, 8);
                            point1 = data_;
                            point2 = data_ + 8;
                            return 1;
                        }
                        else
                        {
                            //没超过8个就不用处理
                            return 1;
                        }
                    }
                    else
                    {
                        //找到了修改状态
                        state = 1;
                        point1 = ps;
                        //INFO("find start!"<<endl);
                        return 0;
                    }
                break;
            }
            case 1: //已经收到固定开始，正在收包头
            {
                //收到包头了
                size_t len = point2 - point1;
                if (len >= (NET_HEAD_SIZE + 8))
                {
                    #if 0
                    INFO("find head!");
                    DUMP(point1+8,2);
                    #endif
                    body_length_ = ntohs(*((unsigned short*)(point1+8)));
                    //INFO(",len="<<dec<<body_length_<<endl);
                    if (body_length_ > NET_MAX_BUFF_SIZE)
                    {
                        state = 0;
                        point1 = data_;
                        point2 = data_;
                        return -1;
                    }                    
                    state = 2;
                    return 0;    //继续处理
                }
                else
                {
                    //INFO("<head len"<<endl);
                    //不足长度，等着收包继续处理
                    return 1;
                }
                break;
            }
            case 2:
                //收到整个包了
                if (size_t(point2 - point1) >= (NET_HEAD_SIZE + 8 + body_length_))
                {
                    std::string s(point1 + NET_HEAD_SIZE + 8);
                    point1 += (NET_HEAD_SIZE + 8 + body_length_);
                    json_spirit::mValue value;
                    json_spirit::read(s, value);
                    state = 0;
                    INFO("recv a pack! from "<<se.remote_ip()<<endl<<s<<endl<<"pack end!"<<endl);
                    //处理消息
                    return se.process_msg(value);
                }
                else
                {
                    //INFO("< bodylen"<<body_length_<<endl);
                    return 1;
                }
                break;
        }
        return -1;
    }

//////////////////////////////////////////////////////////////////////////
    actionmessage::actionmessage() 
    {
        m_from = 0;
        cmd = "";
#ifdef DEBUG_PER
        _recv_time = splsTimeStamp();
#endif
    }

    actionmessage::actionmessage(json_spirit::mObject& robj, int from = 0) 
    :recvObj(robj)
    {
        cmd = "";
        m_from = from;
        READ_STR_FROM_MOBJ(cmd, robj, "cmd");
#ifdef DEBUG_PER
        _recv_time = splsTimeStamp();
#endif
    }
    void actionmessage::setsession(const session_ptr& _session)
    {
        session_ = _session;
    }
    void actionmessage::getsession(session_ptr& _session)
    {
        _session = session_.lock();
    }
    actionmessage& actionmessage::operator =(actionmessage &msg)
    {
        session_ = msg.session_;
        cmd = msg.cmd;
        recvObj = msg.recvObj;
        m_from = msg.m_from;
        return (*this);
    }

    //////////////////////////////////////////////////////////////////////////
    io_service_pool::io_service_pool(std::size_t pool_size)
        : next_io_service_(0)
    {
        if (pool_size == 0)
            throw std::runtime_error("io_service_pool size is 0");

        for (std::size_t i = 0; i < pool_size; ++i)
        {
            io_service_ptr io_service(new boost::asio::io_service);
            work_ptr work(new boost::asio::io_service::work(*io_service));
            io_services_.push_back(io_service);
            work_.push_back(work);
        }
    }

    void io_service_pool::run()
    {
        std::vector<boost::shared_ptr<boost::thread> > threads;
        for (std::size_t i = 0; i < io_services_.size(); ++i)
        {
            boost::shared_ptr<boost::thread> thread(new boost::thread(
                boost::bind(&boost::asio::io_service::run, io_services_[i])));
            threads.push_back(thread);
        }

        for (std::size_t i = 0; i < threads.size(); ++i)
        {
            threads[i]->join();
            cout<<" ************************* io_service_pool::run(),"<<threads[i]->get_id()<<" *************************  "<<endl;
        }
    }

    void io_service_pool::stop()
    {
        for (std::size_t i = 0; i < io_services_.size(); ++i)
            io_services_[i]->stop();
    }

    boost::asio::io_service& io_service_pool::get_io_service()
    {
        boost::asio::io_service& io_service = *io_services_[next_io_service_];
        ++next_io_service_;
        if (next_io_service_ == io_services_.size())
            next_io_service_ = 0;
        return io_service;
    }
    volatile uint64_t session::_refs;
//////////////////////////////////////////////////////////////////////////
    session::session(boost::asio::io_service& io_service)
        : socket_(io_service)
        , using_(false)
        , wait_close_(false)
        , is_sending_(false)
        , is_reading_(false)
        , io_service_(io_service)
    #ifdef USE_STRAND
        , strand_(io_service)
    #endif // USE_STRAND
    {
        ++session::_refs;
        m_port = 0;
        slen = 0;
        memcpy(send_data, NET_PACK_START_STRING, 8);
        send_data[11] = 1;
        send_data[12] = 1;
    }

    session::~session()
    {
        --session::_refs;
    }

    uint64_t session::refs()
    {
        return session::_refs;
    }

    tcp::socket& session::socket()
    {
        return socket_;
    }

    boost::asio::io_service& session::io_service()
    {
        return io_service_;
    }

    bool session::is_using() 
    {
        // boost::mutex::scoped_lock lock(using_mutex_);
        return using_;
    };

    void session::is_using(bool b)
    {
        // boost::mutex::scoped_lock lock(using_mutex_);
        if (!b)
        {
            if (_remote_ip != "")
            {
                sub_ip_connect(_remote_ip);
            }
        }
        using_ = b;
    }

    void session::start()
    {
        m_log_cmds = 0;
        m_action_cmds = 0;
        m_last_log_time = 0;
        m_last_action_time = 0;
        m_connect_time = time(NULL);

        is_using(true);
        is_sending_ = false;
        m_state = STATE_CONNECTED;
        wait_close_ = false;
        is_debug_ = false;
        recv.clear();
        slen = 0;
        _remote_ip = "0.0.0.0";
        // 必须读满一个数据头才返回.
        try
        {
            _remote_ip = socket_.remote_endpoint().address().to_string();
            if (g_print_debug_info)
            {
                cout<<"session::start(),remote ip:"<<_remote_ip<<endl;
            }
            if (add_ip_connect(_remote_ip) < 0)
            {
                using_ = false;
                boost::system::error_code ignored_ec;
                socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
                socket().close(ignored_ec);
                return;
            }
            is_reading_ = true;
            boost::asio::async_read(socket_, boost::asio::buffer(tempbuff, 8),
                boost::asio::transfer_at_least(8),
            #ifdef USE_STRAND
                strand_.wrap(
            #endif // USE_STRAND
                boost::bind(&session::handle_read_login_or_policy, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
            #ifdef USE_STRAND
                )
            #endif // USE_STRAND
            );
        }
        catch (boost::system::system_error& e)
        {
            cout<<"start session fail!"<<e.what()<<endl;
            is_reading_ = false;
            _closeconnect();
        }
    }

    void session::_send(const std::string& buff, bool pack)
    {
        try
        {
            INFO("send data to "<<_remote_ip<<endl);
            if (pack)
            {
                memcpy(send_data, NET_PACK_START_STRING, 8);
                send_data[11] = 1;
                send_data[12] = 1;
                slen = buff.length() + NET_HEAD_SIZE + 8;
                //设置内容长度
                *(unsigned short*)(send_data + 8) = htons(buff.length());
                memcpy(send_data + NET_HEAD_SIZE + 8, buff.data(), slen);
                INFO(buff<<endl);
                INFO("org.len="<<buff.length()<<".send len="<<slen<<endl);
                DUMP(send_data, slen);
            }
            else
            {
                slen = buff.length();
                memcpy(send_data, buff.data(), slen);
            }
            net::write_handler w_handler(shared_from_this());
            boost::asio::async_write(socket_, boost::asio::buffer(send_data, slen),
                    #ifdef USE_STRAND  
                        strand_.wrap(
                    #endif // USE_STRAND
                       w_handler
                    #ifdef USE_STRAND
                        )
                    #endif // USE_STRAND
                    );
        }
        catch (boost::system::system_error& e)
        {
            INFO("_send exception:"<<e.what()<<endl);
            boost::mutex::scoped_lock lock(session_mutex_);
            is_sending_ = false;
            wait_close_ = true;
            m_send_que.clear();
            boost::system::error_code ignored_ec;
            socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket().close(ignored_ec);
            if (!is_reading_)
            {
                is_using(false);
                if (m_ponlineuser != NULL)
                {
                    accountOffline(m_ponlineuser->get_cid());
                    m_ponlineuser.reset();                    
                }
            }
        }
    }

    void session::send(const std::string& buff, bool pack)
    {
        if (buff.size() >= (sizeof(send_data) - 100))
        {
            INFO("packet to send is to large!"<<endl);
            return;
        }
        io_service_.post(boost::bind(&session::do_send, shared_from_this(), buff, pack));
    }

    void session::do_send(const std::string& buff, bool pack)
    {
        INFO("session::send"<<endl);
        if (!wait_close_)
        {
            try
            {
                boost::mutex::scoped_lock lock(session_mutex_);
                m_send_que.push_back(buff);
                if (is_sending_)
                {
                    INFO("is_sending_!"<<endl);
                }
                else
                {
                    INFO("call _send()"<<endl);
                    is_sending_ = true;
                    io_service_.post(boost::bind(&session::_send,
                        shared_from_this(),
                        buff, pack));
                }
            }
            catch (boost::system::system_error& e)
            {
                INFO("send exception:"<<e.what()<<endl);
                boost::mutex::scoped_lock lock(session_mutex_);
                is_sending_ = false;
                wait_close_ = true;
                m_send_que.clear();
                boost::system::error_code ignored_ec;
                socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
                socket().close(ignored_ec);
                if (!is_reading_)
                {
                    is_using(false);
                    if (m_ponlineuser.get())
                    {
                        accountOffline(m_ponlineuser->get_cid());
                        m_ponlineuser.reset();
                    }
                }
            }
        }
        else
        {
            INFO("wait to close!not send"<<endl);
        }
    }

    void session::handle_read_login_or_policy(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
            using namespace std;
            if (!strncmp(tempbuff, "login!!!", 8))
            {
                INFO("recv login!!!"<<remote_ip()<<endl);
                using namespace json_spirit;
                Object robj;
                robj.push_back( Pair("cmd", "login"));
                robj.push_back( Pair("s", 200));
                robj.push_back( Pair("login", 1));
                do_send(write(robj));
                // 必须读满一个数据头才返回.
                try_read();
            }
            else if (!strncmp(tempbuff, "<policy-", 8))
            {
                is_reading_ = false;
                INFO("recv <policy> "<<remote_ip()<<endl);
                do_send("<?xml version=\"1.0\"?><!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"><cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"" + boost::lexical_cast<std::string>(m_port) + "\" /><allow-http-request-headers-from domain=\"*\" headers=\"*\" /></cross-domain-policy>", false);
                closeconnect();
            }
            else
            {
                #if 0
                if (!strncmp(tempbuff, "GET ", 4))
                {
                    _httpRequst._bComplete = false;
                    _httpRequst._state = sw_start;
                    _httpRequst._type = 1;
                    memcpy(_httpRequst._req_url, tempbuff, 8);
                    _httpRequst._req_url[8] = 0;
                    _httpRequst._pos = 8;
                    read_http_request();
                }
                else if (!strncmp(tempbuff, "POST ", 5))
                {
                    _httpRequst._bComplete = false;
                    _httpRequst._state = sw_start;
                    _httpRequst._type = 2;
                    memcpy(_httpRequst._req_url, tempbuff, 8);
                    _httpRequst._req_url[8] = 0;
                    _httpRequst._pos = 8;
                    read_http_request();
                }
                else
                #endif
                is_reading_ = false;
                {
                    _closeconnect();
                }
            }
        }
        else if (error != boost::asio::error::operation_aborted)
        {
            is_reading_ = false;
            boost::system::error_code ignored_ec;
            socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket().close(ignored_ec);
            is_using(false);
        }
        else
        {
            is_reading_ = false;
            m_ponlineuser.reset();
            is_using(false);
        }
    }  

    void session::try_read()
    {
        // 必须读满一个数据头才返回.
        is_reading_ = true;
        try
        {
            INFO("try read!!!"<<endl);
            boost::asio::async_read(socket_, boost::asio::buffer(tempbuff, NET_MAX_BUFF_SIZE/2),
                boost::asio::transfer_at_least(1),
            #ifdef USE_STRAND
                strand_.wrap(
            #endif // USE_STRAND
                boost::bind(&session::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
            #ifdef USE_STRAND
                )
            #endif // USE_STRAND
            );
        }
        catch (boost::system::system_error& e)
        {
            is_reading_ = false;
            INFO(e.what()<<endl);
            _closeconnect();
        }
    }
    void session::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
    {
        //有错误，或者需要关闭socket
        if (error && error != boost::asio::error::operation_aborted)
        {
            is_reading_ = false;
            _closeconnect();
            return;
        }
        //读成功
        if (!error)
        {
            recv.adddata(tempbuff, bytes_transferred);
            int ret = recv.check(*this);
            while (0 == ret)
            {
                ret = recv.check(*this);
            }
            if (1 == ret)
            {
                // 继续读
                try_read();
            }
            else
            {
                is_reading_ = false;
                _closeconnect();
            }
        }
        if (error == boost::asio::error::operation_aborted)
        {
            is_reading_ = false;
            m_ponlineuser.reset();
            is_using(false);
        }
    }

    int session::process_msg(json_spirit::mValue& value)
    {
        using namespace std;
        using namespace boost;
        using namespace json_spirit;
        Object robj;
        if (value.type() != obj_type)
        {
            if (m_retry < 3)
            {
                ++m_retry;
                INFO("*******************************recv none obj!!!!!"<<endl);
            }
            else
            {
                Object robj;
                robj.push_back( Pair("cmd", "bye"));
                robj.push_back( Pair("s", 402));
                send(write(robj));
                return -1;
            }
        }
        else
        {
            mObject& mobj = value.get_obj();
            std::string cmd = "";
            READ_STR_FROM_MOBJ(cmd, mobj, "cmd");
            if (cmd == "")
            {
                if (m_retry < 3)
                {
                    ++m_retry;
                    return 0;
                }
                else
                {
                    Object robj;
                    robj.push_back( Pair("cmd", "bye"));
                    robj.push_back( Pair("s", 402));
                    do_send(write(robj));
                    return -1;
                }
            }
            
            if (cmd == "beat")
            {
                robj.clear();
                robj.push_back( Pair("cmd", "beat"));
                robj.push_back( Pair("s", 200));
                do_send(write(robj));
            }
            else
            {
                if (time(NULL) - m_last_action_time >= 5)
                {
                    m_action_cmds = 0;
                    m_last_action_time = time(NULL);
                }
                ++m_action_cmds;
                //忽略過多命令
                if (m_action_cmds > 100)
                {
                    return 0;
                }
                actionmessage act_msg(mobj);
                act_msg.setsession(shared_from_this());
                actionwork_.submitjob(act_msg);
            }
        }
        return 0;
    }
    
    void session::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
            if (slen > bytes_transferred)
            {
                INFO("***********handle_write************"<<slen<<"!="<<bytes_transferred<<endl);
                memmove(send_data, send_data+bytes_transferred, slen - bytes_transferred);
                slen = slen - bytes_transferred;
                try
                {
                    net::write_handler w_handler(shared_from_this());
                    boost::asio::async_write(socket_, boost::asio::buffer(send_data, slen),
                    #ifdef USE_STRAND  
                        strand_.wrap(
                    #endif // USE_STRAND
                       w_handler
                    #ifdef USE_STRAND
                        )
                    #endif // USE_STRAND
                    );
                }
                catch(boost::system::system_error& e)
                {
                    boost::mutex::scoped_lock lock(session_mutex_);
                    is_sending_ = false;
                    wait_close_ = true;
                    m_send_que.clear();
                    boost::system::error_code ignored_ec;
                    socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
                    socket().close(ignored_ec);
                    if (!is_reading_)
                    {
                        is_using(false);
                        if (m_ponlineuser.get())
                        {
                            INFO(e.what()<<endl);
                            accountOffline(m_ponlineuser->get_cid());
                            m_ponlineuser.reset();
                        }
                    }
                }
            }
            //如果发完了
            else
            {
                INFO("***********handle_write************ complete "<<bytes_transferred<<endl);
                //清空发送缓冲
                slen = 0;
                boost::mutex::scoped_lock lock(session_mutex_);
                //移除发送完的包
                if (m_send_que.size() > 0)
                {
                    m_send_que.pop_front();
                } 
                //找到下一个要发送的包
                if (m_send_que.size() > 0)
                {
                    std::string buff = m_send_que.front();
                    return _send(buff);
                }
                else
                {
                    is_sending_ = false;
                    if (wait_close_)
                    {
                        boost::system::error_code ignored_ec;
                        socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
                        socket().close(ignored_ec);
                        if (!is_reading_)
                        {
                            is_using(false);
                            if (m_ponlineuser != NULL)
                            {
                                INFO(error<<endl);
                                accountOffline(m_ponlineuser->get_cid());
                                m_ponlineuser.reset();
                            }
                        }
                    }
                    return;
                }
            }            
        }
        else if ((error && error != boost::asio::error::operation_aborted))
        {
            boost::mutex::scoped_lock lock(session_mutex_);
            is_sending_ = false;
            wait_close_ = true;
            m_send_que.clear();
            boost::system::error_code ignored_ec;
            socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket().close(ignored_ec);
            if (!is_reading_)
            {
                is_using(false);
                if (m_ponlineuser != NULL)
                {
                    INFO(error<<endl);
                    accountOffline(m_ponlineuser->get_cid());
                    m_ponlineuser.reset();
                }
            }
        }
    }

    void session::closeconnect(bool kickoff)
    {
        io_service_.post(boost::bind(&session::_closeconnect, shared_from_this(), kickoff));
    }

    void session::_closeconnect(bool kickoff)
    {
        boost::mutex::scoped_lock lock(session_mutex_);
        wait_close_ = true;
        if (!is_sending_ && !is_reading_)
        {
            INFO("session::closeconnect()"<<endl);
            if (m_ponlineuser != NULL)
            {
                if (!kickoff)
                {
                    accountOffline(m_ponlineuser->get_cid());
                }
                m_ponlineuser.reset();
            }
            wait_close_ = false;
            m_send_que.clear();
            boost::system::error_code ignored_ec;
            socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket().close(ignored_ec);
            is_using(false);
        }
        return;
    }

    void session::handle_read_http_request(const boost::system::error_code& error, size_t bytes_transferred)
    {
        //有错误，或者需要关闭socket
        if (error && error != boost::asio::error::operation_aborted)
        {
            is_reading_ = false;
            _closeconnect();
            return;
        }
        //读成功
        if (!error)
        {
            // state 
            for (size_t i = 0; i < bytes_transferred; ++i)
            {
                switch (_httpRequst._state)
                {
                    case sw_start:
                        if (tempbuff[i] == 0x0d)
                        {
                            _httpRequst._state = sw_req_url_CR;
                            _httpRequst._req_url[_httpRequst._pos] = tempbuff[i];
                            ++_httpRequst._pos;
                        }
                        else
                        {
                            if (_httpRequst._pos > 253)
                            {
                                goto close_connect;
                            }
                            _httpRequst._req_url[_httpRequst._pos] = tempbuff[i];
                            ++_httpRequst._pos;
                        }
                        break;
                    case sw_req_url_CR:
                        if (tempbuff[i] == 0x0a && _httpRequst._pos >= 10 && !strncmp(_httpRequst._req_url + _httpRequst._pos - 10, " HTTP", 5))
                        {
                            _httpRequst._state = sw_req_url_LF;
                            _httpRequst._req_url[_httpRequst._pos] = tempbuff[i];
                            _httpRequst._req_url[_httpRequst._pos - 10] = 0;
                            _httpRequst._pos = _httpRequst._pos - 10;
                        }
                        else
                        {
                            goto close_connect;
                        }
                        break;
                    case sw_req_url_LF:
                        if (tempbuff[i] == 0x0d)
                        {
                            _httpRequst._state = sw_end_CR1;
                        }
                        break;
                    case sw_end_CR1:
                        if (tempbuff[i] == 0x0a)
                        {
                            _httpRequst._state = sw_end_LF1;
                        }
                        else
                        {
                            _httpRequst._state = sw_req_url_LF;
                        }
                        break;
                    case sw_end_LF1:
                        if (tempbuff[i] == 0x0d)
                        {
                            _httpRequst._state = sw_end_CR2;
                        }
                        else
                        {
                            _httpRequst._state = sw_req_url_LF;
                        }
                        break;
                    case sw_end_CR2:
                        if (tempbuff[i] == 0x0a)
                        {
                            _httpRequst._state = sw_end_LF2;
                            //成功接受到完整http request
                        }
                        else
                        {
                            _httpRequst._state = sw_req_url_LF;
                        }
                        break;
                }
                
                if (sw_end_LF2 == _httpRequst._state)
                {
                    break;
                }
            }
            if (sw_end_LF2 == _httpRequst._state)
            {
                INFO("SUCCESS!"<<endl);
                INFO("["<<endl);
                INFO(_httpRequst._req_url<<endl);
                INFO(endl<<"]"<<endl);

                bool bFindStart = false;
                int iStartPos = 0;
                for (int i = 0; i < _httpRequst._pos - 1; ++i)
                {
                    if (_httpRequst._req_url[i] == '/' && _httpRequst._req_url[i+1] == '?')
                    {
                        bFindStart = true;
                        iStartPos = i + 2;
                        break;
                    }
                }
                if (bFindStart)
                {
                    json_spirit::mObject mobj;
                    mobj["cmd"] = "recharge";
                     
                     std::string strInput(_httpRequst._req_url + iStartPos);
                    INFO("recv recharge cmd ...["<<strInput<<"]"<<endl);
                    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
                    boost::char_separator<char> sep("&");
                    boost::char_separator<char> sep2("=");

                    tokenizer tok(strInput, sep);
                    tokenizer::iterator it = tok.begin();
                    while (it != tok.end())
                    {
                        INFO("pair:"<<*it<<endl);
                        tokenizer tok2(*it, sep2);
                        tokenizer::iterator it_key = tok2.begin();
                        if (it_key == tok2.end())
                        {
                            ERR();
                            break;
                        }
                        std::string key = *it_key;
                        ++it_key;
                        if (it_key == tok2.end())
                        {
                            ERR();
                            break;
                        }
                        INFO(key<<"="<<*it_key<<endl);
                        if (key != "cmd")
                        {
                            mobj[key] = *it_key;
                        }
                        ++it;
                    }
                    actionmessage act_msg(mobj, 1);
                    act_msg.setsession(shared_from_this());
                    actionwork_.submitjob(act_msg);
                    return;
                }
            }
            else
            {
                INFO("continue read..."<<endl);
                read_http_request();
                return;
            }
            close_connect:
            is_reading_ = false;
            boost::system::error_code ignored_ec;
            socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket().close(ignored_ec);
            is_using(false);
        }
    }

    int session::read_http_request()
    {
        is_reading_ = true;
        try
        {
            boost::asio::async_read(socket_, boost::asio::buffer(tempbuff, NET_MAX_BUFF_SIZE/2),
                boost::asio::transfer_at_least(1),
            #ifdef USE_STRAND
                strand_.wrap(
            #endif // USE_STRAND
                boost::bind(&session::handle_read_http_request, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
            #ifdef USE_STRAND
                )
            #endif // USE_STRAND
            );
        }
        catch(boost::system::system_error& e)
        {
            is_reading_ = false;
            INFO(e.what()<<endl);
            _closeconnect();
        }
        return 0;
    }

    size_t session::msg_count()
    {
        return actionwork_.jobcounts();
    }
    actionmessage session::get_job()
    {
        return actionwork_.getjob();
    }

    void session::handle_connect(const boost::system::error_code& error)
    {
        if (!error)
        {
            try
            {
                _remote_ip = socket_.remote_endpoint().address().to_string();
            }
            catch (boost::system::system_error& e)
            {
                _remote_ip = "";
            }
            //cout<<" ************ connected *************"<<endl;
            //sendMsg msg;
            //msg._msg = "login!!!";
            //msg._pack = false;
            //do_write(msg);

            //if (m_char.get())
            //{
            //    m_char->_state = char_connected;
            //}
            m_state = STATE_CONNECTED;

            // 必须读满一个数据头才返回.
            try_read();
        }
        else
        {
            //do_close();
            cout<<" ********** connected fail ********** "<<endl;
        }
    }

//////////////////////////////////////////////////////////////////////////
    
    robot::robot(robotMgr& h)
    :m_cid(0)
    ,m_level(0)
    ,m_stronghold(0)
    ,m_mapid(0)
    ,m_task_state(0)
    ,m_task_id(0)
    ,m_task_need1(0)
    ,m_task_need2(0)
    ,m_camp(0)
    ,m_handle(h)
    {
    }
    int robot::queryCharData()
    {
        return 0;
    }

    int robot::queryTreasure(int tid)
    {
        return 0;
    }

    int robot::doSomething()
    {
        if (m_state == robot_not_connect)
        {
            return try_connect();
        }
        else if (m_state >= robot_try_connect2)
        {
            //处理消息
            if (_session->msg_count())
            {
                actionmessage msg = _session->get_job();
                work(msg);
            }
        }
        return 0;
    }

    void robot::handle_connect(const boost::system::error_code& error)
    {
        if (!error)
        {
            m_state = robot_connected;
            //发送登录
            if (_session.get())
            {
                _session->handle_connect(error);
                _session->send("login!!!", false);
                m_state = robot_try_connect2;
            }
        }
        else
        {
            cout<<" ********** connected fail ********** "<<endl;
        }
    }

    void robot::do_connect()
    {
        tcp::resolver resolver(_session->io_service());
        tcp::resolver::query query(m_handle.get_host(), LEX_CAST_STR(m_handle.get_port()));
        tcp::resolver::iterator iter = resolver.resolve(query);
            //cout<<"try connect to "<<endpoint<<" ..."<<endl;
#ifdef USE_STRAND
#endif
        boost::asio::async_connect(_session->socket(), iter,
#ifdef USE_STRAND
                            _session->strand().wrap(
#endif // USE_STRAND
                            boost::bind(&robot::handle_connect, this,
                            boost::asio::placeholders::error)
#ifdef USE_STRAND
                            )
#endif // USE_STRAND
                        );
    }

    int robot::try_connect()
    {
        if (!_session.get())
        {
            _session = m_handle.get_session();
            if (!_session.get())
            {
                cout<<"get session fail."<<endl;
                return -1;
            }
            m_state = robot_not_connect;
        }

        if (m_state == robot_not_connect)
        {
            m_state = robot_try_connect;        // try connect
            _session->io_service().post(boost::bind(&robot::do_connect, shared_from_this()));
        }
        return m_state;
    }

    bool robot::work(actionmessage& task)       // 在些完成实际任务.
    {
        try
        {
            if (task.from()== 1)
            {                
                boost::unordered_map<std::string, pFuncInternalProcessCmds>::iterator it = robot::m_internal_cmds_process_map.find(task.cmd);
                if (it != robot::m_internal_cmds_process_map.end())
                {
                    (*(it->second))(task.getRecvObj());
                }
                else
                {
                    INFO("work recv internal cmd "<<task.cmd<<endl);
                }
                return true;
            }
            INFO("main work recv cmd "<<task.cmd<<endl);
            session_ptr psession;
            task.getsession(psession);
            if (!psession.get())
            {
                INFO("action worker get no session"<<endl);
                return true;
            }
            using namespace std;
            using namespace boost;
            using namespace json_spirit;
            if (psession->state() == STATE_AUTHED || "login" == task.cmd)
            {
                boost::unordered_map<std::string, pFuncProcessCmds>::iterator it = robot::m_cmds_process_map.find(task.cmd);
                if (it != robot::m_cmds_process_map.end())
                {
                    (*(it->second))(psession, task.getRecvObj());
                    return true;
                }
                else
                {
                    INFO("recv unknow cmd:"<<task.cmd);
                    return true;
                }
            }
            else
            {
                INFO("unauthed!!!!!");
            }
            return true;
        }
        catch (std::exception& e)
        {
            cout<<"action work , Exception: "<<e.what()<<endl;
            cout<<"cmd: "<<task.cmd<<endl;
        }
        return true;
    }

    boost::unordered_map<std::string, pFuncProcessCmds> robot::m_cmds_process_map;
    boost::unordered_map<std::string, pFuncInternalProcessCmds> robot::m_internal_cmds_process_map;

    void robot::init_cmd_map()
    {
        //对外接口
        m_cmds_process_map["login"] = ProcessTestLogin;
        m_cmds_process_map["charlist"] = ProcessCharListResult;
        m_cmds_process_map["qcreate"] = ProcessCreateListResult;
        m_cmds_process_map["create"] = ProcessCreateSuccess;
        m_cmds_process_map["roleLogin"] = ProcessCharLoginSuccess;

        m_cmds_process_map["getRoleInfo"] = ProcessCharInfo;
        m_cmds_process_map["getAreaStageList"] = ProcessCharMapInfo;
        m_cmds_process_map["getStageGateList"] = ProcessCharStageInfo;
        m_cmds_process_map["getPostInfo"] = ProcessCharOfficalInfo;
        m_cmds_process_map["getActionInfo"] = ProcessActionInfo;
        m_cmds_process_map["attackBoss"] = ProcessAttackBossResult;
        m_cmds_process_map["enterBossScene"] = ProcessEnterBossResult;
        m_cmds_process_map["enterGroupBattle"] = ProcessEnterCampResult;
    }

    void robot::resetActionState()
    {
        memset(m_boss_state, 0, 10*sizeof(int));
        m_trade_state = 0;
        m_race_state = 0;
        m_camp_race_state = 0;
    }

    int robot::queryActionInfo()     //查询活动信息
    {
        INFO(" ****** queryActionInfo ****** "<<endl);
        std::string cmd = strCmdFormat;
        str_replace(cmd, "$C", "getActionInfo");
        _session->send(cmd);
        return 0;
    }
    
    int robot::queryTradeInfo()        //查询通商信息
    {
        INFO(" ****** queryTradeInfo ****** "<<endl);
        std::string cmd = strCmdFormat;
        str_replace(cmd, "$C", "checkTradeState");
        _session->send(cmd);
        return 0;
    }
    
    int robot::queryRaceInfo()        //查询竞技信息
    {
        INFO(" ****** queryRaceInfo ****** "<<endl);
        std::string cmd = strCmdFormat;
        str_replace(cmd, "$C", "getRaceRankList");
        _session->send(cmd);
    
        cmd = strCmdFormat;
        str_replace(cmd, "$C", "getRaceList");
        _session->send(cmd);
        return 0;
    }
    
    //enterBossScene
    int robot::enterBoss(int id)
    {
        INFO(" ****** enterBoss ****** "<<endl);
        std::string cmd = strCmdIdFormat;
        str_replace(cmd, "$C", "enterBossScene");
        str_replace(cmd, "$I", LEX_CAST_STR(id));
        _session->send(cmd);
        return 0;
    }
    
    //enterCampRace
    int robot::enterCampRace(int type)
    {
        INFO(" ****** enterCamp ****** "<<endl);
        std::string cmd = strCmdTypeFormat;
        str_replace(cmd, "$C", "enterGroupBattle");
        str_replace(cmd, "$I", LEX_CAST_STR(type));
        _session->send(cmd);
        return 0;
    }
    
    //startCampRace
    int robot::startCampRace()
    {
        INFO(" ****** startCamp ****** "<<endl);
        std::string cmd = strCmdFormat;
        str_replace(cmd, "$C", "startGroupBattle");
        _session->send(cmd);
        return 0;
    }
    
    //getCampRace
    int robot::getCampRace()
    {
        INFO(" ****** startCamp ****** "<<endl);
        std::string cmd = strCmdFormat;
        str_replace(cmd, "$C", "getGroupBattleInfo");
        _session->send(cmd);
        return 0;
    }
    
    //join Camp
    int robot::JoinCamp(int type)
    {
        INFO(" ****** joinCamp ****** "<<endl);
        std::string cmd = strCmdTypeFormat;
        str_replace(cmd, "$C", "joinCamps");
        str_replace(cmd, "$I", LEX_CAST_STR(type));
        _session->send(cmd);
        return 0;
    }
    
    int robot::queryBossCD(int id)    //查询boss攻击CD
    {
        INFO(" ****** queryBossCD ****** "<<endl);
        std::string cmd = strCmdIdFormat;
        str_replace(cmd, "$C", "getBossCoolTime");
        str_replace(cmd, "$I", LEX_CAST_STR(id));
        _session->send(cmd);
    
        cmd = strCmdIdFormat;
        str_replace(cmd, "$C", "getBossPerson");
        str_replace(cmd, "$I", LEX_CAST_STR(id));
        _session->send(cmd);
        return 0;
    }
    
    //{"id":1,"cmd":"attackBoss"}
    int robot::attackBoss(int id) //攻击boss
    {
        INFO(" ****** attackBoss ****** "<<endl);
        std::string cmd = strCmdIdFormat;
        str_replace(cmd, "$C", "attackBoss");
        str_replace(cmd, "$I", LEX_CAST_STR(id));
        _session->send(cmd);
        return 0;
    }
    
    //{"id":1,"cmd":"getBossInfo"}
    int robot::queryBossInfo(int id) //查询boss信息
    {
        INFO(" ****** queryBossInfo ****** "<<endl);
        std::string cmd = strCmdIdFormat;
        str_replace(cmd, "$C", "getBossInfo");
        str_replace(cmd, "$I", LEX_CAST_STR(id));
        _session->send(cmd);
        return 0;
    }
    
    int robot::queryOfficalInfo()    //查询官职
    {
        INFO(" ****** queryBossInfo ****** "<<endl);
        std::string cmd = strCmdFormat;
        str_replace(cmd, "$C", "getPostInfo");
        _session->send(cmd);
        return 0;
    }

    robotMgr::robotMgr(std::string& host, short port, std::size_t io_service_pool_size)
    :io_service_pool_(io_service_pool_size)
    ,m_host(host)
    ,m_port(port)
    {
        
    }

    void robotMgr::run()
    {
        io_service_pool_.run();
    }

    void robotMgr::stop()
    {
        io_service_pool_.stop();
    }

    void robotMgr::load()
    {
        //阵营战是否开启了
        m_camp_race_open = -1;
        //boss战是否开启了
        m_boss_open = -1;
        //多人副本是否开启了
        m_group_copy_open = -1;

        //当前季节
        m_seaon = -1;
        //当前年份
        m_year = -1;

        //当前时间小时，分钟
        m_hour = -1;
        m_min = -1;

        //配置
        //阵营1在线人数
        m_camp1_online = 1;
        //阵营2在线人数
        m_camp2_online = 1;
    }

    int robotMgr::checkConnect()
    {
        return 0;
    }

    session_ptr robotMgr::get_session()
    {
        session_ptr new_session;
        std::vector<session_ptr>::iterator i;
        // 尝试在session池寻找一个没有使用的session.
        for(i = session_pool_.begin(); i != session_pool_.end(); i++)
        {
            if (!(*i)->is_using())
            {
                new_session = *i;
                break;
            }
        }
        // 没找到将插入一个新的session到池中.
        if (i == session_pool_.end())
        {
        #ifdef USE_POOL
            if (session_pool_.size() < MAX_SESSITONS)
               {
                   new_session.reset(new session(io_service_pool_.get_io_service()));
                session_pool_.push_back(new_session);
            }
            else
            {
                new_session->_closeconnect();
                new_session.reset(new session(io_service_pool_.get_io_service()));
                session_pool_.push_back(new_session);
            }
        #else
            new_session.reset(new session(io_service_pool_.get_io_service()));
        #endif // USE_POOL
        }
        return new_session;
    }

    int robotMgr::workLoop()
    {
        do
        {
            try
            {
                //检查是否需要连接角色
                checkConnect();
                
                for (std::map<int, boost::shared_ptr<robot> >::iterator it = m_robots.begin(); it != m_robots.end(); ++it)
                {
                    if (it->second.get())
                    {
                        it->second->doSomething();
                    }
                }
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
                
                return -1;
            }
        }
        while (true);
        
        return 0;
    }
} // namespace net

void close_thread_db(void* db)
{
    Database* pdb = reinterpret_cast<Database*>(db);
    if (pdb)
    {
        delete pdb;
    }
}

int main(int argc, char* argv[])
{
    pthread_key_create (&thread_db_key, close_thread_db);
    int port = 80;
    std::string host = "localhost";
    if (argc >= 2)
    {
        host = argv[1];
    }
    if (argc >= 3)
    {
        port = atoi(argv[2]);
    }
    boost::shared_ptr<net::robotMgr> mgr(new net::robotMgr(host, port));
    mgr->load();
    mgr->run();

    boost::shared_ptr<boost::thread> thread(new boost::thread(
                boost::bind(&net::robotMgr::workLoop, mgr)));
    thread->join();
    return 0;
}

