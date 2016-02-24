
#include "iostream"
#include "robot.h"
#include "utils_all.h"
#include "spls_errcode.h"
#include <execinfo.h>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/random.hpp>

using namespace std;

volatile uint64_t g_time_now = 0;

volatile int g_first_robot = 1;

volatile int g_connect = 0;

static boost::mt19937 g_generator(42u);

boost::mutex _global_lock;

boost::mt19937* getGenerator()
{
    return &g_generator;
}

void init_random_seed()
{
    g_generator.seed(time(NULL));    //初始化随机数
}

int my_random(int mins, int maxs)
{
    boost::uniform_real<> dist(0, 1);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<> > _rand_(g_generator, dist);
    //return (max > min) ? (min + rand()%(max + 1 - min)) : min;
    return (maxs > mins) ? (mins + (int)(_rand_() * (maxs + 1 - mins))) : mins;
}

volatile int g_debug = 0;

using namespace std;
using namespace net;
using namespace json_spirit;
using namespace boost;

#define INFO(x)

#define ERR() std::cout<<"!!!!!!!!!!!!!!!!!error!!!!!!!!!!!!!!!!:"<<__FILE__<<" at line "<<__LINE__<<std::endl<<std::flush
#define NET_PACK_START_STRING "FEESEE!!"

#if 1
#define DUMP(x,len)
#else
#define DUMP(x, len) for (size_t i = 0; i < len; ++i)\
{\
    char* p = static_cast<char*>(x + i);\
    cout<<"0x"<<hex<<(int)(*p)<<"("<<(*p)<<")"<<",";\
}\
cout<<endl
#endif

typedef void (*pFuncProcessCmds)(tsession_ptr& psession, json_spirit::mObject& o);
typedef void (*pFuncInternalProcessCmds)(json_spirit::mObject& o);

int m_quit_test = 0;

void do_sleep(int ms)
{
    boost::xtime xt;
    boost::xtime_get(&xt,boost::TIME_UTC_);
    xt.nsec += ms%1000*1000*1000;
    xt.sec += ms/1000;
    boost::thread::sleep(xt);
}

uint64_t splsTimeStamp()
{
    struct timeval _tstart;
    gettimeofday(&_tstart, NULL);
    return 1000*1000*_tstart.tv_sec + _tstart.tv_usec;
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

inline uint64_t rand_delay(int secs, int secs2)
{
    return g_time_now + secs*1000000 + (secs2-secs) * 1000 * my_random(0, 1000);
}

//处理登录信息
void ProcessTestLogin(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessTestLogin message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    
    int login_type  = 0;
    READ_INT_FROM_MOBJ(login_type,o,"login");
    if (1 == login_type)
    {
        if (pUser->_state == char_connected)
        {
            std::string msg = strLoginMessage;
            str_replace(msg, "$U", pUser->_account);
            str_replace(msg, "$P", pUser->_password);
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
            pUser->_state = char_login1;
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
void ProcessCharListResult(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharListResult message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_login1)
    {
        INFO(pUser->_account<<":ProcessCharListResult message,state="<<pUser->_state<<endl);
        return;
    }
    pUser->_charList.clear();
    json_spirit::mArray clist;
    READ_ARRAY_FROM_MOBJ(clist,o,"charlist");
    pUser->_state = char_list_return;
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
void ProcessCreateListResult(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCreateListResult message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_list_return)
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
void ProcessCreateSuccess(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCreateSuccess message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_list_return)
    {
        INFO(pUser->_account<<":ProcessCreateSuccess message,state="<<pUser->_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<":ProcessCreateSuccess,"<<ret<<endl);
    psession->send(strCmdCharList);
}

//处理角色登录成功
void ProcessCharLoginSuccess(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharLoginSuccess message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_list_return)
    {
        INFO(pUser->_account<<":ProcessCharLoginSuccess message,state="<<pUser->_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<":ProcessCharLoginSuccess,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        pUser->_state = char_login2;
    }
    else
    {
        cout<<pUser->_account<<":ProcessCharLoginSuccess, fail !!!!!!!!!!!!!"<<ret<<endl;
    }
}

//处理角色信息
void ProcessCharInfo(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharInfo message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_login2)
    {
        INFO(pUser->_account<<":ProcessCharInfo message,state="<<pUser->_state<<endl);
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
            if (id == pUser->_cid)
            {
                if (gid != -1)
                {
                    pUser->_gid = gid;
                }
                if (gold != -1)
                {
                    pUser->_gold = gold;
                }
                if (level != -1)
                {
                    pUser->_level = level;
                }
                if (silver != -1)
                {
                    pUser->_silver = silver;
                }
                if (vip != -1)
                {
                    pUser->_vip = vip;
                }
                if (ling != -1)
                {
                    pUser->_ling = ling;
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
                    pUser->_mapid = area;
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
void ProcessCharMapInfo(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharMapInfo message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_login2)
    {
        INFO(pUser->_account<<":ProcessCharMapInfo message,state="<<pUser->_state<<endl);
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
            if (pUser->_mapid < area)
            {
                pUser->_mapid = area;
            }
        }
        json_spirit::mArray stageList;
        READ_ARRAY_FROM_MOBJ(stageList,o,"list");
        INFO(" char mapid "<<pUser->_mapid<<",stage "<<pUser->_stage<<endl);
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
                    if (state > 0 && stageId > pUser->_stage)
                    {
                        pUser->_stage = stageId;
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
void ProcessCharStageInfo(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharStageInfo message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_login2)
    {
        INFO(pUser->_account<<":ProcessCharStageInfo message,state="<<pUser->_state<<endl);
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
            pUser->_curMapid = mapid;
            pUser->_curStage = stageid;
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
void ProcessCharOfficalInfo(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessCharOfficalInfo message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_login2)
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
        pUser->_camp = camp;
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
void ProcessActionInfo(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessActionInfo message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_login2)
    {
        INFO(pUser->_account<<":ProcessActionInfo message,state="<<pUser->_state<<endl);
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
                                pUser->_boss_state[bossId-1] = active;
                                pUser->_boss_id = bossId;
                                INFO(pUser->_account<<":ProcessActionInfo,boss "<<bossId<<" is openning"<<endl);
                            }
                            break;
                        }
                        case 3:
                            pUser->_camp_race_state = 1;
                            INFO(pUser->_account<<":ProcessActionInfo,camp race is openning"<<endl);
                            break;
                        case 5:    //竞技
                            pUser->_race_state = 1;
                            INFO(pUser->_account<<":ProcessActionInfo,race is opened"<<endl);
                            break;
                        case 7:    //通商
                            pUser->_trade_state = 1;
                            INFO(pUser->_account<<":ProcessActionInfo,trade is opened"<<endl);
                            break;
                    }
                }
            }

        }
    }
}

//处理攻击boss结果
void ProcessAttackBossResult(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessAttackBossResult message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_login2)
    {
        INFO(pUser->_account<<"ProcessAttackBossResult message,state="<<pUser->_state<<endl);
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
        memset(pUser->_boss_state, 0, sizeof(int)*10);
        pUser->_boss_id = 0;
        if (pUser->_my_action == action_in_boss)
        {
            pUser->_my_action = action_idle;
        }
    }
    else if (HC_ERROR_BOSS_NOT_ENTER == ret)
    {
        pUser->enterBoss(pUser->_boss_id);
    }
}

//处理进入boss场景结果
void ProcessEnterBossResult(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessEnterBossResult message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_login2)
    {
        INFO(pUser->_account<<"ProcessEnterBossResult message,state="<<pUser->_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<"ProcessEnterBossResult,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        pUser->_my_action = action_in_boss;
    }
    else
    {
        pUser->queryActionInfo();
    }
}

//处理进入阵营战结果
void ProcessEnterCampResult(tsession_ptr& psession, json_spirit::mObject& o)
{
    boost::shared_ptr<testChar> tUser = psession->user();
    if (!tUser.get())
    {
        INFO("ProcessEnterCampResult message,no user handle."<<endl);
        return;
    }
    testChar* pUser = tUser.get();
    if (pUser->_state != char_login2)
    {
        INFO(pUser->_account<<"ProcessEnterCampResult message,state="<<pUser->_state<<endl);
        return;
    }
    int ret = 0;
    READ_INT_FROM_MOBJ(ret,o,"s");
    INFO(pUser->_account<<"ProcessEnterCampResult,"<<ret<<endl);
    if (HC_SUCCESS == ret)
    {
        pUser->_my_action = action_in_camp_race;
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

namespace net 
{
class write_handler
    {
    public:
        write_handler(boost::shared_ptr<tsession> psn)
        {
            pthis = psn;
        }
        boost::shared_ptr<tsession> pthis;
        void operator()(const boost::system::error_code& error, size_t bytes_transferred)
        {
            pthis->handle_write(error, bytes_transferred);
        }
    };
}
trecvbuff::trecvbuff()
{
    clear();
};
void trecvbuff::clear()
{
    body_length_ = 0;
    point1 = data_;
    point2 = data_;
    state = 0;
};

void trecvbuff::adddata(void* ad, size_t len)
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
int trecvbuff::check(tsession& se)
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
                    INFO("find start!"<<endl);
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
                INFO("find head!");
                //DUMP(point1+8,2);
                body_length_ = ntohs(*((unsigned short*)(point1+8)));
                INFO(",len="<<dec<<body_length_<<endl);
                if (body_length_ > NET_MAX_BUFF_SIZE)
                {
                    state = 0;
                    point1 = data_;
                    point2 = data_;
                    return -1;
                }                     
                state = 2;
                return 0;     //继续处理
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
                INFO("recv a pack! from "<<se.remote_ip()<<endl<<s<<endl<<"pack end!"<<endl);
                point1 += (NET_HEAD_SIZE + 8 + body_length_);
                json_spirit::mValue value;
                json_spirit::read(s, value);
                INFO("read from pack end..."<<endl);
                state = 0;
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

testMessage::testMessage() 
{
    m_from = 0;
    cmd = "";
}

testMessage::testMessage(json_spirit::mObject& robj, int from = 0) 
:recvObj(robj)
{
    cmd = "";
    m_from = from;
    READ_STR_FROM_MOBJ(cmd, robj, "cmd");
}
void testMessage::setsession(const tsession_ptr& _session)
{
    session_ = _session;
}
void testMessage::getsession(tsession_ptr& _session)
{
    _session = session_.lock();
}
testMessage& testMessage::operator =(testMessage &msg)
{
    session_ = msg.session_;
    cmd = msg.cmd;
    recvObj = msg.recvObj;
    m_from = msg.m_from;
    return (*this);
}

int testChar::_totals = 0;

testChar::testChar(boost::asio::io_service& io_service,jobqueue<testMessage>& actionwork, 
      tcp::resolver::iterator endpoint_iterator, const std::string& account, const std::string& password, int cid)
{
    ++testChar::_totals;
    _session.reset(new tsession(io_service, actionwork, endpoint_iterator));

    strcpy(_account, account.c_str());
    strcpy(_password, password.c_str());
    _cid = cid;
    _gid = 0;        //军团id
    _level = 0;        //等级
    _silver = 0;    //银币数量
    _gold = 0;        //金币数量
    _ling = 0;        //军令数量    
    _mapid = 0;        //地图进度
    _stage = 0;        //场景进度
    _curMapid = 0;    //当前所在地图
    _curStage = 0;    //当前场景

    _my_action = action_idle;

    _boss_id = 0;
    memset(_boss_state, 0, 10*sizeof(int));    //boss副本状态
    _camp_race_state = 0;    //阵营战状态
    _trade_state = 0;        //通商是否开启
    _race_state = 0;        //竞技是否开启
    _my_trade_state = 0;    //我的通商状态
    _my_camp_race_state = 0;//我的阵营战状态

    _state = char_inited;
    _action_time = g_time_now + 2000000 + testChar::_totals * 100000 + my_random(1000,100000);
    for (int i = 0; i < 25; ++i)
    {
        _strongholds[i]._id = 0;
        _strongholds[i]._state = -1;
        _strongholds[i]._level = 0;
    }
}

testChar::~testChar()
{
    --testChar::_totals;
}

void testChar::start()
{
    _state = char_try_connect;
    _session->user(shared_from_this());
    _session->start();
    cout<<"testChar::start()..."<<endl;
}

int testChar::queryCharInfo()
{
    INFO(" ****** getRoleInfo ****** "<<endl);
    std::string cmd = strCmdFormat;
    str_replace(cmd, "$C", "getRoleInfo");
    _session->send(cmd);
    return 0;
}

int testChar::queryMapInfo()
{
    INFO(" ****** getMapInfo ****** "<<endl);
    std::string cmd = strcmdGetMapInfo;
    str_replace(cmd, "$M", LEX_CAST_STR(_mapid));
    _session->send(cmd);
    return 0;
}

int testChar::queryStageInfo()
{
    INFO(" ****** getStageInfo ****** "<<endl);
    std::string cmd = strcmdGetStageInfo;
    str_replace(cmd, "$M", LEX_CAST_STR(_mapid));
    str_replace(cmd, "$S", LEX_CAST_STR(_stage));
    _session->send(cmd);
    return 0;
}

void testChar::randAction()
{
    const int iRand_of_chat = 1;
    const int iRand_of_attackStronghold = 30;
    const int iRand_of_queryStageInfo = 30;

    INFO(" ****** randAction ******"<<endl);
    int iRand = my_random(1,100);

    iRand -= iRand_of_chat;
    if (iRand <= 0)
    {
        INFO(" ****** rand chat ****** "<<endl);
        randChat();
        return;
    }

    if (_level == 0 || _mapid == 0)
    {
        queryCharInfo();
        queryActionInfo();
        return;
    }
    if (_stage == 0)
    {
        queryMapInfo();
        return;
    }
    if (_curStage == 0)
    {
        queryStageInfo();
        return;
    }

    if (_my_action == action_in_boss && _boss_id)
    {
        queryBossCD(_boss_id);
        queryBossInfo(_boss_id);
        attackBoss(_boss_id);
        _action_time = rand_delay(2, 3);
        return;
    }
    if (_boss_id && _boss_state[_boss_id-1] && _my_action == action_idle)
    {
        switch (_my_action)
        {
            
        }
        enterBoss(_boss_id);
        _action_time = rand_delay(2, 3);
        return;
    }
    queryOfficalInfo();
    if (_camp != 1 && _camp != 2)
    {
        JoinCamp(my_random(1,2));
        queryOfficalInfo();
    }
    //阵营战开启并且机器人处于战斗界面自动准备
    if (_camp_race_state && _my_action == action_in_camp_race)
    {
        startCampRace();
        getCampRace();//模拟战斗结束重新刷回阵营战界面
        _action_time = rand_delay(2, 3);
        return;
    }
    //阵营战开启如果空闲则参战
    if (_camp_race_state && _my_action == action_idle)
    {
        enterCampRace(1);
        _action_time = rand_delay(2, 3);
        return;
    }
    _my_action = action_idle;
    iRand -= iRand_of_attackStronghold;
    if (iRand <= 0)
    {
        _my_action = action_attack_stronghold;
        attackStronghold();
        return;
    }

    iRand -= iRand_of_queryStageInfo;
    if (iRand <= 0)
    {
        queryStageInfo();
        return;
    }
    queryCharInfo();
    queryActionInfo();
    return;
}

int testChar::attackStronghold()    //攻击关卡
{
    INFO(" ****** try attackStronghold "<<_ling<<"|"<<_curMapid<<"|"<<_curStage<<" ****** "<<endl);    
    if (_ling > 0 && _curMapid > 0 && _curStage)
    {
        int attack_id = 1;
        int attack_level = 1;
        for (int i = 0; i < 25; ++i)
        {
            if (_strongholds[i]._state >= 0 && _strongholds[i]._level > attack_level)
            {
                attack_id = _strongholds[i]._id;
                attack_level = _strongholds[i]._level;
            }
        }
        INFO(" ****** attack stronghold "<<attack_id<<" ****** "<<endl);
        std::string cmd = strcmdAttackStronghold;
        str_replace(cmd, "$S", LEX_CAST_STR(attack_id));
        _session->send(cmd);
        return attack_id;
    }
    return 0;
}

void testChar::resetActionState()
{
    memset(_boss_state, 0, 10*sizeof(int));
    _trade_state = 0;
    _race_state = 0;
    _camp_race_state = 0;
}

void testChar::randChat()
{
    if (!_session.get())
    {
        return;
    }
    std::string cmd = strcmdChatFormat;
    switch (my_random(1, 10))
    {
        case 1:
            str_replace(cmd, "$M", "hi, you ren ma?");
            break;
        case 2:
            str_replace(cmd, "$M", "da jia hao !");
            break;
        case 3:
            str_replace(cmd, "$M", "you xi hao wan ma ?");
            break;
        case 4:
            str_replace(cmd, "$M", "xin lai de !");
            break;
        case 5:
            str_replace(cmd, "$M", "hao wan !");
            break;
        case 6:
            str_replace(cmd, "$M", "zhen hao wan !");
            break;
        case 7:
            str_replace(cmd, "$M", "wo xi huan !");
            break;
        case 8:
            str_replace(cmd, "$M", "bu cuo o !");
            break;
        case 9:
            str_replace(cmd, "$M", "en...");
            break;
        case 10:
            str_replace(cmd, "$M", "#%^&*@!...");
            break;
    }
    INFO("rand chat ..."<<endl);
    _session->send(cmd);
}

int testChar::doSomething()
{
    if (_action_time <= g_time_now)
    {
        if (_session.get() == NULL)
        {
            return -1;
        }
        INFO("doSomething..."<<endl);
        INFO(_account<<" _state="<<_state<<endl);
        _action_time = rand_delay(8,10);
        switch (_state)
        {
            case char_inited:
                if (!g_connect)
                {
                    cout<<"try connect to server:"<<_account<<",session:"<<_session.get()<<endl;
                    _session->user(shared_from_this());
                    _session->start();
                }
                break;
            case char_try_connect:
            case char_connected:
                break;
            case char_list_return:
                if (_charList.size() == 0)
                {
                    std::string cmd = strCmdFormat;
                    str_replace(cmd, "$C", "qcreate");
                    _session->send(cmd);
                }
                else
                {
                    INFO(" ****** char login ****** "<<endl);
                    int cindex = my_random(0, _charList.size() -1);
                    std::list<CharactorInfo>::iterator it = _charList.begin();
                    for (int i = 0; i < cindex; ++i)
                    {
                        ++it;
                    }
                    _cid = it->m_cid;;    
                    std::string cmd = strcmdCharLogin;
                    str_replace(cmd, "$C", LEX_CAST_STR(_cid));
                    _session->send(cmd);
                }
                break;
            case char_login2:
                randAction();
                break;
        }
    }
    return 0;
}

int testChar::queryActionInfo()        //查询活动信息
{
    INFO(" ****** queryActionInfo ****** "<<endl);
    std::string cmd = strCmdFormat;
    str_replace(cmd, "$C", "getActionInfo");
    _session->send(cmd);
    return 0;
}

int testChar::queryTradeInfo()        //查询通商信息
{
    INFO(" ****** queryTradeInfo ****** "<<endl);
    std::string cmd = strCmdFormat;
    str_replace(cmd, "$C", "checkTradeState");
    _session->send(cmd);
    return 0;
}

int testChar::queryRaceInfo()        //查询竞技信息
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
int testChar::enterBoss(int id)
{
    INFO(" ****** enterBoss ****** "<<endl);
    std::string cmd = strCmdIdFormat;
    str_replace(cmd, "$C", "enterBossScene");
    str_replace(cmd, "$I", LEX_CAST_STR(id));
    _session->send(cmd);
    return 0;
}

//enterCampRace
int testChar::enterCampRace(int type)
{
    INFO(" ****** enterCamp ****** "<<endl);
    std::string cmd = strCmdTypeFormat;
    str_replace(cmd, "$C", "enterGroupBattle");
    str_replace(cmd, "$I", LEX_CAST_STR(type));
    _session->send(cmd);
    return 0;
}

//startCampRace
int testChar::startCampRace()
{
    INFO(" ****** startCamp ****** "<<endl);
    std::string cmd = strCmdFormat;
    str_replace(cmd, "$C", "startGroupBattle");
    _session->send(cmd);
    return 0;
}

//getCampRace
int testChar::getCampRace()
{
    INFO(" ****** startCamp ****** "<<endl);
    std::string cmd = strCmdFormat;
    str_replace(cmd, "$C", "getGroupBattleInfo");
    _session->send(cmd);
    return 0;
}

//join Camp
int testChar::JoinCamp(int type)
{
    INFO(" ****** joinCamp ****** "<<endl);
    std::string cmd = strCmdTypeFormat;
    str_replace(cmd, "$C", "joinCamps");
    str_replace(cmd, "$I", LEX_CAST_STR(type));
    _session->send(cmd);
    return 0;
}

int testChar::queryBossCD(int id)    //查询boss攻击CD
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
int testChar::attackBoss(int id) //攻击boss
{
    INFO(" ****** attackBoss ****** "<<endl);
    std::string cmd = strCmdIdFormat;
    str_replace(cmd, "$C", "attackBoss");
    str_replace(cmd, "$I", LEX_CAST_STR(id));
    _session->send(cmd);
    return 0;
}

//{"id":1,"cmd":"getBossInfo"}
int testChar::queryBossInfo(int id)    //查询boss信息
{
    INFO(" ****** queryBossInfo ****** "<<endl);
    std::string cmd = strCmdIdFormat;
    str_replace(cmd, "$C", "getBossInfo");
    str_replace(cmd, "$I", LEX_CAST_STR(id));
    _session->send(cmd);
    return 0;
}

int testChar::queryOfficalInfo()    //查询官职
{
    INFO(" ****** queryBossInfo ****** "<<endl);
    std::string cmd = strCmdFormat;
    str_replace(cmd, "$C", "getPostInfo");
    _session->send(cmd);
    return 0;
}

tsession::tsession(boost::asio::io_service& io_service, jobqueue<testMessage>& actionwork, tcp::resolver::iterator endpoint_iterator)
    : socket_(io_service)
    , actionwork_(actionwork)
    , endpoint_it(endpoint_iterator)
    , using_(false)
    , wait_close_(false)
    , is_sending_(false)
    , is_reading_(false)
    , io_service_(io_service)
#ifdef USE_STRAND
    , strand_(io_service)
#endif // USE_STRAND
{
    m_state = -2;
    //m_char = c;
    m_port = 0;
    slen = 0;
    memcpy(send_data, NET_PACK_START_STRING, 8);
    send_data[11] = 1;
    send_data[12] = 1;
}

tsession::~tsession()
{
}

tcp::socket& tsession::socket()
{
    return socket_;
}

bool tsession::is_using() 
{
    return using_;
};

void tsession::is_using(bool b)
{
    using_ = b;
}

void tsession::start()
{
    g_connect = 1;
    if (m_state == STATE_DISCONNECT)
    {
        return;
    }
    io_service_.post(
#ifdef USE_STRAND
                    strand_.wrap(
#endif // USE_STRAND

        boost::bind(&tsession::do_start, this)
#ifdef USE_STRAND
)
#endif // USE_STRAND

);
}

void tsession::do_start()
{
    boost::mutex::scoped_lock lock(_global_lock);
    if (m_state == STATE_DISCONNECT)
    {
        return;
    }
    if (m_char.get())
    {
        m_char->_state = char_try_connect;
    }
    is_using(true);
    is_sending_ = false;
    is_reading_ = false;
    m_state = STATE_TRY_CONNECT;
    wait_close_ = false;
    recv.clear();
    slen = 0;
    tcp::endpoint endpoint = *endpoint_it; 
    //cout<<"try connect to "<<endpoint<<" ..."<<endl;
#ifdef USE_STRAND
    INFO("use strand..."<<endl);
#endif
    boost::asio::async_connect(socket_, endpoint_it,
#ifdef USE_STRAND
                strand_.wrap(
#endif // USE_STRAND
                boost::bind(&tsession::handle_connect, this,
                boost::asio::placeholders::error)
#ifdef USE_STRAND
                )
#endif // USE_STRAND
            );
}

void tsession::write(const sendMsg& msg)
{
    if (m_state == STATE_DISCONNECT)
    {
        return;
    }
    io_service_.post(
#ifdef USE_STRAND
                    strand_.wrap(
#endif // USE_STRAND
        boost::bind(&tsession::do_write, this, msg)
#ifdef USE_STRAND
)
#endif // USE_STRAND

        );
}

void tsession::send(const std::string& msg)
{
    if (m_state == STATE_DISCONNECT)
    {
        return;
    }
    sendMsg smsg;
    strcpy(smsg._msg, msg.c_str());
    smsg._pack = true;
    io_service_.post(
#ifdef USE_STRAND
            strand_.wrap(
#endif // USE_STRAND
        boost::bind(&tsession::do_write, this, smsg)
#ifdef USE_STRAND
        )
#endif // USE_STRAND
        );
}

void tsession::do_close()
{
    boost::mutex::scoped_lock lock(_global_lock);
    if (m_state == STATE_DISCONNECT)
    {
        return;
    }
    INFO("tsession::do_close()");
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket_.close(ignored_ec);
    if (m_char != NULL)
    {
        m_char->_state = char_dis;
        m_char.reset();
        m_state = STATE_DISCONNECT;
    }
    if (!is_reading_&& !is_sending_)
    {
        is_using(false);
    }
}

void tsession::do_write(const sendMsg msg)
{
    boost::mutex::scoped_lock lock(_global_lock);
    if (m_state == STATE_DISCONNECT)
    {
        return;
    }
    bool write_in_progress = !m_send_queue.empty();
    m_send_queue.push_back(msg);
    if (!write_in_progress)
    {
        INFO("send data to "<<remote_ip()<<endl);
        if (msg._pack)
        {
            memcpy(send_data, NET_PACK_START_STRING, 8);
            send_data[11] = 1;
            send_data[12] = 1;
            slen = strlen(msg._msg) + NET_HEAD_SIZE + 8;
            //设置内容长度
            *(unsigned short*)(send_data + 8) = htons(strlen(msg._msg));
            memcpy(send_data + NET_HEAD_SIZE + 8, msg._msg, slen);
            //INFO("org.len="<<buff.length()<<".send len="<<slen<<endl);
            //DUMP(send_data, slen);
        }
        else
        {
            slen = strlen(msg._msg);
            memcpy(send_data, msg._msg, slen);
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
}

void tsession::handle_read_login_or_policy(const boost::system::error_code& error, size_t bytes_transferred)
{
    if (!error)
    {
        if (!strncmp(tempbuff, "login!!!", 8))
        {
            INFO("recv login!!!"<<remote_ip()<<endl);

            Object robj;
            robj.push_back( Pair("cmd", "login"));
            robj.push_back( Pair("s", 200));
            robj.push_back( Pair("login", 1));

            sendMsg smsg;
            std::string msg = json_spirit::write(robj);
            strcpy(smsg._msg, msg.c_str());
            smsg._pack = true;
            do_write(smsg);

            // 必须读满一个数据头才返回.
            try_read();
        }
        else if (!strncmp(tempbuff, "<policy-", 8))
        {
            INFO("recv <policy> "<<remote_ip()<<endl);
            sendMsg smsg;
            std::string msg = "<?xml version=\"1.0\"?><!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"><cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"" + boost::lexical_cast<std::string>(m_port) + "\" /><allow-http-request-headers-from domain=\"*\" headers=\"*\" /></cross-domain-policy>";
            smsg._pack = false;
            wait_close_ = true;
            strcpy(smsg._msg, msg.c_str());
            do_write(smsg);
        }
        else
        {
            is_reading_ = false;
            do_close();
        }
    }
    else
    {
        is_reading_ = false;
        do_close();
    }
}  

void tsession::try_read()
{
    // 必须读满一个数据头才返回.
    is_reading_ = true;
    INFO("try read!!!"<<endl);
    boost::asio::async_read(socket_, boost::asio::buffer(tempbuff, NET_MAX_BUFF_SIZE/2),
        boost::asio::transfer_at_least(1),
    #ifdef USE_STRAND
        strand_.wrap(
    #endif // USE_STRAND
        boost::bind(&tsession::handle_read, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)
    #ifdef USE_STRAND
        )
    #endif // USE_STRAND
    );
}
void tsession::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
    //读成功
    if (!error)
    {
        INFO("read "<<bytes_transferred<<endl);
        recv.adddata(tempbuff, bytes_transferred);
        INFO("call recv.check()"<<endl);
        int ret = recv.check(*this);
        while (0 == ret)
        {
            ret = recv.check(*this);
        }
        if (wait_close_)
        {
            is_reading_ = false;
            do_close();
            return;
        }
        INFO("recv.check() return "<<ret<<endl);
        if (1 == ret)
        {
            // 继续读
            try_read();
        }
        else
        {
            is_reading_ = false;
            do_close();
        }
    }
    else
    {
        is_reading_ = false;
        do_close();
    }
}

int tsession::process_msg(json_spirit::mValue& value)
{
    INFO("process_msg()"<<endl);
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
            send(json_spirit::write(robj));
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
                send(json_spirit::write(robj));
                return -1;
            }
        }
        
        if (cmd == "beat")
        {
            robj.clear();
            robj.push_back( Pair("cmd", "beat"));
            robj.push_back( Pair("s", 200));
            send(json_spirit::write(robj));
        }        
        else
        {
            testMessage act_msg(mobj);
            act_msg.setsession(shared_from_this());
            actionwork_.submitjob(act_msg);
        }
    }
    return 0;
}

void tsession::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
{
    if (!error)
    {
        if(slen > bytes_transferred)
        {
            INFO("***********handle_write************"<<slen<<"!="<<bytes_transferred<<endl);
            memmove(send_data, send_data+bytes_transferred, slen - bytes_transferred);
            slen = slen - bytes_transferred;
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
        //如果发完了
        else
        {
            //清空发送缓冲
            slen = 0;
            //移除发送完的包
            m_send_queue.pop_front();
            //找到下一个要发送的包
            if (!m_send_queue.empty())
            {
                sendMsg msg = m_send_queue.front();
                if (msg._pack)
                {
                    memcpy(send_data, NET_PACK_START_STRING, 8);
                    send_data[11] = 1;
                    send_data[12] = 1;
                    slen = strlen(msg._msg) + NET_HEAD_SIZE + 8;
                    //设置内容长度
                    *(unsigned short*)(send_data + 8) = htons(strlen(msg._msg));
                    memcpy(send_data + NET_HEAD_SIZE + 8, msg._msg, slen);
                    //INFO("org.len="<<buff.length()<<".send len="<<slen<<endl);
                    //DUMP(send_data, slen);
                }
                else
                {
                    slen = strlen(msg._msg);
                    memcpy(send_data, msg._msg, slen);
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
            else
            {
                is_sending_ = false;
                if (wait_close_)
                {
                    do_close();
                }
                return;
            }
        }             
    }
    else
    {
        is_sending_ = false;
        do_close();
    }
}

void tsession::close()
{
    if (m_state == STATE_DISCONNECT)
    {
        return;
    }
    io_service_.post(boost::bind(&tsession::do_close, shared_from_this()));
}

void tsession::handle_connect(const boost::system::error_code& error)
{
    g_connect= 0;
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
        sendMsg msg;
        strcpy(msg._msg , "login!!!");
        msg._pack = false;
        do_write(msg);

        if (m_char.get())
        {
            m_char->_state = char_connected;
        }
        m_state = STATE_CONNECTED;

        // 必须读满一个数据头才返回.
        try_read();
    }
    else
    {
        cout<<" ********** connected fail ********** "<<endl;
        do_close();
    }
}

class tActionWorker : public worker<testMessage>
{
public:    
    
    tActionWorker(jobqueue<testMessage>& _jobqueue, std::size_t _maxthreads = 1) :
      worker<testMessage>("mainworker",_jobqueue, _maxthreads)
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
        //内部接口
        //m_internal_cmds_process_map["reload"] = ProcessReload;
        //m_internal_cmds_process_map["doSomething"] = ProcessReload;
    }

    virtual bool work(testMessage& task)       // 在些完成实际任务.
    {
        try
        {
            if (task.from()== 1)
            {                
                boost::unordered_map<std::string, pFuncInternalProcessCmds>::iterator it = m_internal_cmds_process_map.find(task.cmd);
                if (it != m_internal_cmds_process_map.end())
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
            tsession_ptr psession;
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
                boost::unordered_map<std::string, pFuncProcessCmds>::iterator it = m_cmds_process_map.find(task.cmd);
                if (it != m_cmds_process_map.end())
                {
                    (*(it->second))(psession, task.getRecvObj());
                    int ret = 0;
                    READ_INT_FROM_MOBJ(ret,task.getRecvObj(),"s");
                    if (404 == ret)
                    {
                        boost::shared_ptr<testChar> tUser = psession->user();
                        if (tUser.get())
                        {
                            testChar* pUser = tUser.get();
                            pUser->_state = char_dis;
                        }                        
                    }
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
private:
    boost::unordered_map<std::string, pFuncProcessCmds> m_cmds_process_map;
    boost::unordered_map<std::string, pFuncInternalProcessCmds> m_internal_cmds_process_map;
};

net::jobqueue<testMessage> g_testJobqueue;
tActionWorker g_testWorker(g_testJobqueue);

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        cout<<"xxx host port counts"<<endl;
        return -1;
    }
    int totals = atoi(argv[3]);

    if (totals < 5)
    {
        g_debug = 1;
    }
    init_random_seed();
     boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(argv[1], argv[2]);
    tcp::resolver::iterator iter = resolver.resolve(query);
    //tcp::resolver::iterator end; // End marker.

    //testChar test(io_service, g_testJobqueue, iter, "wlj", "111111", 0);
    g_time_now = splsTimeStamp();

    int from = 1;
    if (argc >= 5)
    {
        from = atoi(argv[4]);
    }

    g_first_robot = from;
    cout<<"from "<<from<<" to "<<(from+totals)<<endl;
    std::list<boost::shared_ptr<testChar> > test_char_list;
    for (int i = from; i <= from + totals; ++i)
    {
        std::string account = "robot" + LEX_CAST_STR(i);
        boost::shared_ptr<testChar> spChar(new testChar(io_service, g_testJobqueue, iter, account, "111111", 0));
        if (i == from)
        {
            spChar->start();
        }
        test_char_list.push_back(spChar);
    }

    boost::thread testThread(boost::bind(&tActionWorker::run, &g_testWorker));
    testThread.detach();
    cout<<"action worker run"<<endl;

    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
    t.detach();

    cout<<"io_server run"<<endl;

    /*while (iter != end)
    {
        cout<<"****************"<<endl;
        tcp::endpoint endpoint = *iter++;
        std::cout << endpoint << std::endl;
        cout<<"****************"<<endl;
    }*/

    //time_t time_now = time(NULL);
    while (!m_quit_test)
    {
        g_time_now = splsTimeStamp();
        do_sleep(10);
        //if (time_now + 5 < time(NULL))
        //{
        //    time_now = time(NULL);
        //    cout<<"spls time stamp:"<<g_time_now<<endl;
        //}
        //test.doSomething();
        for (std::list<boost::shared_ptr<testChar> >::iterator it = test_char_list.begin(); it != test_char_list.end(); ++it)
        {
            if (it->get())
            {
                it->get()->doSomething();
            }
        }
    }
    return 0;
}

