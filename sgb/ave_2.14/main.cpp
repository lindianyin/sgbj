
#define INFO(x)// cout<<x

#include "data.h"
#include "combat.h"
#include "utils_all.h"
#include "spls_errcode.h"
#include <syslog.h>

#include "loginworker.h"
#include "combat_process.h"
#include "SaveDb.h"
#include <signal.h>

#include "spls_timer.h"

#include <boost/progress.hpp>
#include <iostream>
#include <climits>

#include "actionworker.h"
#include <pthread.h>
#include "new_trade.h"
#include "kingnet_analyzer.h"

#include "equipt_upgrade.h"

using boost::timer;
using boost::progress_timer;
using boost::progress_display;
using std::cout;
using std::endl;

volatile int m_quit = 0;
class RunServer;
RunServer* g_runServer = NULL;
extern volatile int g_print_debug_info;

pthread_key_t thread_db_key;
extern void close_thread_db(void* db);

//排名奖倒计时通告
extern std::string strArenaNotifyMsg1;
extern std::string strArenaNotifyMsg2;

#ifndef _WINDOWS
void sigint(int s) /* save+quit */
{
    m_quit++;
}

void sighup(int s) /* quit */
{
    m_quit++;
}

void sigusr1(int s) /* save */
{
}

void sigusr2(int s) /* reset all */
{
    
}

void sigpipe(int s)
{
    
}

void sigdump(int s)
{
    void *array[10];

    size_t size;
    char **strings;
    size_t i;

    size = backtrace (array, 10);
    strings = backtrace_symbols (array, size);
    syslog (LOG_ERR, "Obtained %zd stack frames.", size);
    for (i = 0; i < size; i++)
        syslog(LOG_ERR, strings[i]);
    free (strings);
    exit(0);
}

void siginit(void)
{
    signal(SIGINT, (__sighandler_t)sigint);
    signal(SIGTERM,(__sighandler_t)sigint);
    signal(SIGHUP, (__sighandler_t)sighup);
    signal(SIGUSR1, (__sighandler_t)sigusr1);
    signal(SIGUSR2, (__sighandler_t)sigusr2);
    signal(SIGPIPE, (__sighandler_t)sigpipe);
    //signal(SIGSEGV, (__sighandler_t)sigdump);
}

#endif

extern int checkScheduleEvent();
extern int checkRecharge();
extern int checkPack();
extern void checkCustomSchedule(struct tm& tm_now);
void KeepDbLoginWork();
extern void SetExeName(const std::string & ename);

//清理死号
int clearDeadCharactors();

net::jobqueue<combatCmd> g_combatJobqueue;
CombatProces g_combatProcess(g_combatJobqueue);

net::jobqueue<combatCmd> g_savecombatJobqueue;
SaveCombatWorker g_saveCombatWorker(g_savecombatJobqueue);

net::jobqueue<saveDbJob> g_SavedbJobqueue;
SavedbProcess g_saveDbProcess(g_SavedbJobqueue, 1);

net::jobqueue<mailCmd> g_mailJobqueue;
mailProcesser g_mailProcess(g_mailJobqueue);

net::jobqueue<dbCmd> g_charDbJobqueue;
charDbProcesser g_charDbProcess(g_charDbJobqueue);

class RunServer
{
public:
    RunServer(int port, int port2 = 0)
        : _actionqueueptr(new net::jobqueue<net::actionmessage>)
        , _loginqueueptr(new net::jobqueue<net::loginmessage>)
        //, _policyServerptr(new net::policyServer)
        , _actionworkerptr(new ActionWorker((*_actionqueueptr.get())))
        , _loginworkerptr(new LoginWorker((*_loginqueueptr.get())))
        , _serverptr(new net::server(port, (*_actionqueueptr.get()), (*_loginqueueptr.get()), 1, port2))
    {
        //_policy_listenthreadptr.reset(new boost::thread(boost::bind(&policyServer::run, _policyServerptr.get())));

        //_policyServerptr->start_accept();

        _listenthreadptr.reset(new boost::thread(boost::bind(&RunServer::RunNetListen, this)));
        _actionthreadptr.reset(new boost::thread(boost::bind(&RunServer::RunActionWorks, this)));
        _loginthreadptr.reset(new boost::thread(boost::bind(&RunServer::RunLoginWorks, this)));
    }

    ~RunServer() 
    {
        _actionworkerptr->stop();   // 停止.
        //_actionqueueptr->notify_all();  // 通知.
        //_loginqueueptr->notify_all();   // 通知.
        //_listenthreadptr->join();
        //_actionthreadptr->join();
        //_loginthreadptr->join();
    }

    void Stop()
    {
        _serverptr->stop();         // 停止.
        _loginworkerptr->stop();    // 停止.
        //_policyServerptr->stop();
    }
    void StopAction()
    {
        _actionworkerptr->stop();
    }

    void RunNetListen()
    {
        cout<<" ************************* thread "<<syscall(SYS_gettid)<<" _serverptr->run()*************************  "<<endl;
        if (_serverptr)
            _serverptr->run();
    }

    void RunActionWorks()
    {
        cout<<" ************************* thread "<<syscall(SYS_gettid)<<" _actionworkerptr->run()*************************  "<<endl;
        if (_actionworkerptr)
            _actionworkerptr->run();
    }

    void RunLoginWorks()
    {
        cout<<" ************************* thread "<<syscall(SYS_gettid)<<" _loginworkerptr->run()*************************  "<<endl;
        if (_loginworkerptr)
            _loginworkerptr->run();
    }
    jobqueue<actionmessage>& getActionwork()
    {
        return *_actionqueueptr.get();
    }

    jobqueue<loginmessage>& getLoginwork()
    {
        return *_loginqueueptr.get();
    }

private:
    boost::shared_ptr<boost::thread> _listenthreadptr;
    boost::shared_ptr<boost::thread> _actionthreadptr;
    boost::shared_ptr<boost::thread> _loginthreadptr;

    boost::shared_ptr<jobqueue<actionmessage> > _actionqueueptr;
    boost::shared_ptr<jobqueue<loginmessage> > _loginqueueptr;

    //boost::shared_ptr<net::policyServer> _policyServerptr;    //flex策略文件
    //flex策略文件端口
    boost::shared_ptr<boost::thread> _policy_listenthreadptr;

public:
    boost::shared_ptr<ActionWorker> _actionworkerptr;
    boost::shared_ptr<LoginWorker> _loginworkerptr;
    boost::shared_ptr<server> _serverptr;
};

int InsertInternalActionWork(json_spirit::mObject& obj)
{
    if (g_runServer != NULL)
    {
        net::actionmessage act_msg(obj, 1);
        g_runServer->getActionwork().submitjob(act_msg);
        return 0;
    }
    else
    {
        return -1;
    }
}

int InsertActionWork(net::actionmessage& msg)
{
    if (g_runServer != NULL)
    {
        g_runServer->getActionwork().submitjob(msg);
        return 0;
    }
    else
    {
        return -1;
    }
}

void KeepDbLoginWork()
{
    if (g_runServer != NULL)
    {
        loginmessage msg;
        msg.username = "keepDb";
        msg.password = "";
        g_runServer->getLoginwork().submitjob(msg);
    }    
}

void InsertSaveDb(const std::string& sql, int param1, int param2, const std::string& sp1, const std::string& sp2)
{
    saveDbJob job(sql, param1, param2, sp1, sp2);
    g_SavedbJobqueue.submitjob(job);
}

void InsertSaveDb(const std::string& sql)
{
    saveDbJob job(sql, 0, 0, "", "");
    g_SavedbJobqueue.submitjob(job);
}

void InsertSaveDb(const saveDbJob& job)
{
    g_SavedbJobqueue.submitjob(job);
}

void InsertCombat(Combat* pcombat)
{
    combatCmd cmd;
    cmd._pCombat = pcombat;
    g_combatJobqueue.submitjob(cmd);
}

void InsertSaveCombat(Combat* pcombat)
{
    combatCmd cmd;
    cmd._pCombat = pcombat;
    g_savecombatJobqueue.submitjob(cmd);
}

void InsertMailcmd(mailCmd& pmailCmd)
{
    g_mailJobqueue.submitjob(pmailCmd);
}

void InsertDbCharCmd(dbCmd& _dbCmd)
{
    g_charDbJobqueue.submitjob(_dbCmd);
}

//定时的处理
int splsUpdate()
{
    time_t timep;
    struct tm m_timenow;
    time(&timep);
    localtime_r(&timep, &m_timenow);

    static int m_tm_min = -1; 
    static int m_tm_hour = -1;
    static int m_tm_secs = -1;
    static int m_tm_mday = -1;

    //每秒
    if (m_tm_secs != m_timenow.tm_sec)
    {
        //更新当前秒
        m_tm_secs = m_timenow.tm_sec;
        if (m_tm_secs % 5 == 0)
        {
            //检查定时处理 5秒一次
            checkScheduleEvent();
            checkRecharge();
            checkPack();

            //5秒钟保持数据库连接
            // action thread
            json_spirit::mObject obj;
            obj["cmd"] = "keepDb";
            if (0 != InsertInternalActionWork(obj))
            {
                ERR();
            }
            // mail thread
            mailCmd cmd;
            cmd.cmd = mail_cmd_keep_db;
            InsertMailcmd(cmd);

            // save db thread
            InsertSaveDb("");

            // login thread
            KeepDbLoginWork();

            //db thread
            dbCmd dbc;
            dbc._cmd = db_cmd_keep_db;
            g_charDbJobqueue.submitjob(dbc);

            if (g_print_debug_info)
            {
                cout<<"            ################### Combat("<<dec<<Combat::refs()<<"),Army("<<Army::refs()<<"),army_data("<<army_data::refs()<<"),CharData("<<CharData::refs()<<"),net::session("<<net::session::refs()<<"),Online("<<GeneralDataMgr::getInstance()->getTotalOnline(false)<<"),OnlineUser("<<OnlineUser::_refs<<"),OnlineCharactor("<<OnlineCharactor::_refs<<"),CharactorInfo("<<CharactorInfo::_refs<<")"<<endl;
            }
        }
        if (m_tm_secs % 10 == 0)
        {
            //存盘
            json_spirit::mObject obj;
            obj["cmd"] = "scheduleEvent";
            obj["event"] = "save_db";
            if (0 != InsertInternalActionWork(obj))
            {
                ERR();
            }
        }
        if (0 == m_tm_secs % 15)
        {
            //heartBeat
            json_spirit::mObject obj;
            obj["cmd"] = "scheduleEvent";
            obj["event"] = "heartBeat";
            if (0 != InsertInternalActionWork(obj))
            {
                ERR();
            }
        }
        //每分钟
        if (m_tm_min != m_timenow.tm_min)
        {
            //更新当前分
            m_tm_min = m_timenow.tm_min;
#ifdef QQ_PLAT
            if (0 == m_tm_min % 5)
            {
                //腾讯系统统计
                GeneralDataMgr::getInstance()->to_Tencent();
            }
#endif

            //竞技场倒计时公告
            int race_reward_time = GeneralDataMgr::getInstance()->GetRaceRewardTime();
            int jjcleft = race_reward_time - timep;
            int jjc_left_min = (jjcleft + 59) / 60;
            {
                if (jjc_left_min > 0 && jjc_left_min < 60 && jjc_left_min % 10 == 0)
                {
                    std::string msg = strArenaNotifyMsg2;
                    str_replace(msg, "$M", LEX_CAST_STR(jjc_left_min));
                    json_spirit::mObject mobj;
                    mobj["cmd"] = "broadCastMsg";
                    mobj["msg"] = msg;
                    if (0 != InsertInternalActionWork(mobj))
                    {
                        ERR();
                    }
                    jjc_left_min = 0;
                }
            }

            //检查军团boss开启
            {
                json_spirit::mObject obj;
                obj["cmd"] = "checkJtBoss";
                obj["hour"] = m_tm_hour;
                obj["min"] = m_tm_min;
                if (0 != InsertInternalActionWork(obj))
                {
                    ERR();
                }
            }
            
#if 1   //暂时
            //检查机器人军团
            {                
                json_spirit::mObject obj;
                obj["cmd"] = "checkRobotCorps";
                if (0 != InsertInternalActionWork(obj))
                {
                    ERR();
                }
            }
#endif

            //每小时
            if (m_tm_hour != m_timenow.tm_hour)
            {
                //更新当前时间
                m_tm_hour = m_timenow.tm_hour;

                //竞技场倒计时公告
                {
                    int jjc_left_hour = (jjc_left_min + 59) / 60;
                    if (jjc_left_hour >= 1 && jjc_left_hour <= 24)
                    {
                        std::string msg = strArenaNotifyMsg1;
                        str_replace(msg, "$H", LEX_CAST_STR(jjc_left_hour));
                        json_spirit::mObject mobj;
                        mobj["cmd"] = "broadCastMsg";
                        mobj["msg"] = msg;
                        if (0 != InsertInternalActionWork(mobj))
                        {
                            ERR();
                        }
                    }
                }
                //6点12点18点0点刷新商店
                //if (m_tm_hour % 6 == 0)
                //{
                //    json_spirit::mObject obj;
                //    obj["cmd"] = "scheduleEvent";
                //    obj["event"] = "resetShop";
                //    if (0 != InsertInternalActionWork(obj))
                //    {
                //        ERR();
                //    }
                //}
                //每三小时刷新屯田任务
                if (m_tm_hour % 3 == 0)
                {
                    json_spirit::mObject obj;
                    obj["cmd"] = "scheduleEvent";
                    obj["event"] = "resetFarm";
                    if (0 != InsertInternalActionWork(obj))
                    {
                        ERR();
                    }
                }
                //每小时GM奖励发放
                {
                    json_spirit::mObject obj;
                    obj["cmd"] = "scheduleEvent";
                    obj["event"] = "gmReward";
                    if (0 != InsertInternalActionWork(obj))
                    {
                        ERR();
                    }
                    //GeneralDataMgr::getInstance()->GM_reward();
                }
                //每日
                if (m_tm_mday != m_timenow.tm_mday)
                {
                    m_tm_mday = m_timenow.tm_mday;
                    equipmentUpgrade::getInstance()->updateDiscountTime();
                }
            }
            //检查自定义活动
            checkCustomSchedule(m_timenow);    
            //检查系统公告变化
            GeneralDataMgr::getInstance()->checkAdminNotice(1);
            //排名
            splsRankings::getInstance()->updateRankings(rankings_type_char);
            splsRankings::getInstance()->updateRankings(rankings_type_hero);
            splsRankings::getInstance()->updateRankings(rankings_type_lottery);
            splsRankings::getInstance()->updateRankings(rankings_type_elite);
            splsRankings::getInstance()->updateRankings(rankings_type_prestige);
            splsRankings::getInstance()->updateRankings(rankings_type_attack);
            splsRankings::getInstance()->updateRankings(rankings_type_zst);
            {
                json_spirit::mObject obj;
                obj["cmd"] = "scheduleEvent";
                obj["event"] = "updateCorpsRank";
                if (0 != InsertInternalActionWork(obj))
                {
                    ERR();
                }
            }
            //检查排行榜活动
            checkRankingsEvent();
            //检查临时VIP变动
            {
                json_spirit::mObject obj;
                obj["cmd"] = "scheduleEvent";
                obj["event"] = "checkTmpVip";
                if (0 != InsertInternalActionWork(obj))
                {
                    ERR();
                }
                //GeneralDataMgr::getInstance()->checkTmpVIP();
            }
        }
        //检查系统公告变化
        GeneralDataMgr::getInstance()->checkAdminNotice(2);

#if 0
        //測試排名對內存的影響
        splsRankings::getInstance()->updateCharRankings();
        splsRankings::getInstance()->updateHeroRankings();
        {
            json_spirit::mObject obj;
            obj["cmd"] = "scheduleEvent";
            obj["event"] = "updateCorpsRank";
            if (0 != InsertInternalActionWork(obj))
            {
                ERR();
            }
        }
#endif
    }
    return 0;
}

int main(int argc,char *argv[])
{
    SetExeName(argv[0]);
    shhx_log_msg("restart", 777);
    
    syslog(LOG_ERR, "start at %ld", time(NULL));
    cout<<endl<<endl<<endl;
    cout<<"**************************************************"<<endl<<endl;
    cout<<"version build "<<__DATE__<<" "<<__TIME__<<endl;
    int port = 88, port2 = 0;
    if (argc > 1)
    {
        SetDbname(argv[1]);
        cout<<"set db "<<argv[1]<<endl;
    }
    if (argc > 2)
    {
        port = atoi(argv[2]);
        SetPort(port);
    }
    if (argc > 3)
    {
        port2 = atoi(argv[3]);
    }
    cout<<"port "<<port<<","<<port2<<endl;
    init_random_seed();    //初始化随机数

    //Database& db = GetDb();

    pthread_key_create (&thread_db_key, close_thread_db);

    #ifndef _WIN32
    siginit();
    #endif

    //清理死号
    clearDeadCharactors();

    time_t timex = time(NULL);

    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&timex, t);
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    time_t tend = mktime(t) + 3*iONE_DAY_SECS;
    localtime_r(&timex, t);
    printf("start time -> %s\n", asctime(t));
    localtime_r(&tend, t);
    printf("end time -> %s\n", asctime(t));

    RunServer runServer1(port, port2);                // 开启一个server.
    g_runServer = &runServer1;
    cout<<"RunServer runServer1(port, port2) finish, cost "<<(time(NULL)-timex)<<" seconds"<<endl;
    //这个初始化放在最前面
    GeneralDataMgr::getInstance();
    lootMgr::getInstance()->updaeLootPlace();
    taskMgr::getInstance();
    bossMgr::getInstance();
    Singleton<newBaoshiMgr>::Instance();
    horseMgr::getInstance();
    RaceMgr::getInstance();
    Singleton<newTradeMgr>::Instance();
    splsTimerMgr::getInstance();
    corpsMgr::getInstance();
    splsRankings::getInstance()->_updateEliteRankings();
    campRaceMgr::getInstance();
    //多人副本全天开放
    groupCopyMgr::getInstance()->open(0);
    groupCombatMgr::getInstance();
    farmMgr::getInstance();
    newRankings::getInstance();

    boost::thread combatThread(boost::bind(&CombatProces::run, &g_combatProcess));
    combatThread.detach();

    boost::thread savecombatThread(boost::bind(&SaveCombatWorker::run, &g_saveCombatWorker));
    savecombatThread.detach();

    boost::thread SaveDbThread(boost::bind(&SavedbProcess::run, &g_saveDbProcess));
    SaveDbThread.detach();

    boost::thread mailThread(boost::bind(&mailProcesser::run, &g_mailProcess));
    mailThread.detach();

    boost::thread charDbhread(boost::bind(&charDbProcesser::run, &g_charDbProcess));
    charDbhread.detach();

    cout<<"************* start success ***************"<<endl;
    
    _analyzer.Init();

    // 退出命令处理...
    std::string in;
    while (!m_quit)
    {        
        do_sleep(1000);
        //db.KeepAlive();
        //定时的处理
        splsUpdate();
    }
    runServer1.Stop();

    //存盘
    //RaceMgr::getInstance()->saveAll();

    json_spirit::mObject obj;

    //存盘
    //obj["cmd"] = "scheduleEvent";
    //obj["event"] = "save_db";
    //obj["param1"] = 1;
    //if (0 != InsertInternalActionWork(obj))
    //{
    //    ERR();
    //}

    //关闭服务器
    obj.clear();
    obj["cmd"] = "shutdown";
    if (0 != InsertInternalActionWork(obj))
    {
        ERR();
    }
    while (g_charDbJobqueue.jobcounts()
            || g_mailJobqueue.jobcounts()
            || g_SavedbJobqueue.jobcounts()
            || g_savecombatJobqueue.jobcounts()
            || g_combatJobqueue.jobcounts()
            || g_runServer->getActionwork().jobcounts())
    {
        do_sleep(5000);
        cout<<"wait close chardb:"<<g_charDbJobqueue.jobcounts()<<",mailjob:"<<g_mailJobqueue.jobcounts()<<",savedb:"<<g_SavedbJobqueue.jobcounts();
        cout<<",combat:"<<g_combatJobqueue.jobcounts()<<",action:"<<g_runServer->getActionwork().jobcounts()<<endl;
    }
    runServer1.StopAction();
    g_charDbProcess.stop();
    g_mailProcess.stop();
    g_combatProcess.stop();
    g_saveCombatWorker.stop();
    g_saveDbProcess.stop();   // 停止.

    while (g_saveDbProcess.running())
    {
        cout<<"*********wait saveing db***********"<<endl<<flush;
        do_sleep(1000);
    }

    int wait_times = 0;
    while (g_combatProcess.running()
            || runServer1._loginworkerptr->running()
            || runServer1._actionworkerptr->running()
            //|| g_saveDbProcess.running()
            || g_charDbProcess.running()
            || g_mailProcess.running()
            || g_saveCombatWorker.running())
    {
        ++wait_times;
        cout<<"*********wait close***********"<<endl<<flush;
        if (wait_times % 10 == 0)
        {
            if (g_combatProcess.running())
            {
                cout<<"can not stop combat processer"<<endl;
            }
            if (runServer1._loginworkerptr->running())
            {
                cout<<"can not stop login worker processer"<<endl;
            }
            if (runServer1._actionworkerptr->running())
            {
                cout<<"can not stop action worker processer"<<endl;
            }
            if (g_saveDbProcess.running())
            {
                cout<<"can not stop saveDb processer"<<endl;
            }
            if (g_charDbProcess.running())
            {
                cout<<"can not stop car db processer"<<endl;
            }
            if (g_mailProcess.running())
            {
                cout<<"can not stop mail processer"<<endl;
            }
            if (g_saveCombatWorker.running())
            {
                cout<<"can not stop save combat processer"<<endl;
            }            
        }
        if (wait_times > 600)
        {
            if (g_combatProcess.running())
            {
                syslog(LOG_ERR, "can not stop combat processer");
            }
            if (runServer1._loginworkerptr->running())
            {
                syslog(LOG_ERR, "can not stop login worker processer");
            }
            if (runServer1._actionworkerptr->running())
            {
                syslog(LOG_ERR, "can not stop action worker processer");
            }
            if (g_saveDbProcess.running())
            {
                syslog(LOG_ERR, "can not stop saveDb processer");
            }
            if (g_charDbProcess.running())
            {
                syslog(LOG_ERR, "can not stop car db processer");
            }
            if (g_mailProcess.running())
            {
                syslog(LOG_ERR, "can not stop mail processer");
            }
            if (g_saveCombatWorker.running())
            {
                syslog(LOG_ERR, "can not stop save combat processer");
            }
            break;
        }
        do_sleep(1000);
    }
    cout<<"spls down ... "<<endl<<flush;

    shhx_log_msg("shhx quit", 888);
    return 0;
}

