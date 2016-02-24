
#define INFO(x) cout<<x

#include "utils_all.h"
#include "errcode_def.h"
#include <syslog.h>
#include "data.h"

#include "loginworker.h"
#include "SaveDb.h"
#include <signal.h>

#include "spls_timer.h"

#include <boost/progress.hpp>
#include <iostream>
#include <climits>

#include "actionworker.h"
#include <pthread.h>

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
extern void checkCustomSchedule(struct tm& tm_now);
extern int checkRecharge();
extern int checkPack();
void KeepDbLoginWork();
extern void SetExeName(const std::string & ename);

net::jobqueue<saveDbJob> g_SavedbJobqueue;
SavedbProcess g_saveDbProcess(g_SavedbJobqueue, 1);

net::jobqueue<mailCmd> g_mailJobqueue;
mailProcesser g_mailProcess(g_mailJobqueue);

net::jobqueue<dbCmd> g_charDbJobqueue;
charDbProcesser g_charDbProcess(g_charDbJobqueue);

net::jobqueue<chessCombatCmd> g_combatJobqueue;
chessCombatProcesser g_combatProcess(g_combatJobqueue);

net::jobqueue<MultiCombatCmd> g_multiCombatJobqueue;
MultiCombatProcesser g_MultiCombatProcess(g_multiCombatJobqueue);

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

void InsertMailcmd(mailCmd& pmailCmd)
{
    g_mailJobqueue.submitjob(pmailCmd);
}

void InsertDbCharCmd(dbCmd& _dbCmd)
{
    g_charDbJobqueue.submitjob(_dbCmd);
}

void InsertCombat(chessCombatCmd& pcombatCmd)
{
    g_combatJobqueue.submitjob(pcombatCmd);
}

void InsertMultiCombat(MultiCombatCmd& pcombatCmd)
{
    g_multiCombatJobqueue.submitjob(pcombatCmd);
}

//定时的处理
int Update()
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
        Singleton<chessCombatMgr>::Instance().autoLoop();
        Singleton<MultiCombatMgr>::Instance().autoLoop();
        //更新当前秒
        m_tm_secs = m_timenow.tm_sec;
        if (m_tm_secs % 5 == 0)
        {
            //检查定时处理 5秒一次
            checkScheduleEvent();
            checkRecharge();
            checkPack();
            Singleton<AuctionHouseMgr>::Instance().Update();

            //5秒钟保持数据库连接
            // action thread
            json_spirit::mObject obj;
            obj["cmd"] = "keepDb";
            if (0 != InsertInternalActionWork(obj))
            {
                ERR();
            }

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
                cout<<"            ################### CharData("<<CharData::refs()<<"),net::session("<<net::session::refs()<<"),Online("<<GeneralDataMgr::getInstance()->getTotalOnline(false)<<"),OnlineUser("<<OnlineUser::_refs<<"),OnlineCharactor("<<OnlineCharactor::_refs<<"),CharactorInfo("<<CharactorInfo::_refs<<")"<<endl;
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
            if (0 == m_tm_min % 5)
            {
                //在线统计5分钟一次
                GeneralDataMgr::getInstance()->getTotalOnline(true);
            }
            //每小时
            if (m_tm_hour != m_timenow.tm_hour)
            {
                bool change_hour = false;
                if (m_tm_hour != -1)
                {
                    //-1时属于服务器重启
                    change_hour = true;
                }

                //更新当前小时
                m_tm_hour = m_timenow.tm_hour;

                //以下一些功能逻辑仅在真实时间定时触发
                if (change_hour)
                {
                    //城堡居民流失
                    Singleton<cityMgr>::Instance().residentAway();
                    //重置税收
                    if (m_tm_hour == 8 || m_tm_hour == 20)
                    {
                        Singleton<cityMgr>::Instance().resetLevy();
                    }
                }

                //每日
                if (m_tm_mday != m_timenow.tm_mday)
                {
                    m_tm_mday = m_timenow.tm_mday;
                }
            }
            //检查自定义活动
            checkCustomSchedule(m_timenow);
            //检查系统公告变化
            GeneralDataMgr::getInstance()->checkAdminNotice(1);
            //排名
            Singleton<rankingMgr>::Instance().updateRankings();
            Singleton<wildMgr>::Instance().checkWildCity();
        }
        //检查系统公告变化
        GeneralDataMgr::getInstance()->checkAdminNotice(2);
    }
    return 0;
}

int main(int argc,char *argv[])
{
    SetExeName(argv[0]);
    log_msg("restart", 777);

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
    splsTimerMgr::getInstance();
    Singleton<lootMgr>::Instance();
    Singleton<taskMgr>::Instance();
    Singleton<mapMgr>::Instance();
    Singleton<cityMgr>::Instance();
    Singleton<HeroMgr>::Instance();
    Singleton<mallMgr>::Instance();
    Singleton<copyMgr>::Instance();
    Singleton<wildMgr>::Instance();
    Singleton<arenaMgr>::Instance();
    Singleton<actionMgr>::Instance();
    Singleton<exploreMgr>::Instance();
    Singleton<shenlingMgr>::Instance();
    Singleton<skillMgr>::Instance();
    Singleton<guildMgr>::Instance();
    Singleton<PrestigeTaskMgr>::Instance();
    Singleton<AuctionHouseMgr>::Instance();

    boost::thread SaveDbThread(boost::bind(&SavedbProcess::run, &g_saveDbProcess));
    SaveDbThread.detach();

    boost::thread mailThread(boost::bind(&mailProcesser::run, &g_mailProcess));
    mailThread.detach();

    boost::thread charDbhread(boost::bind(&charDbProcesser::run, &g_charDbProcess));
    charDbhread.detach();

    boost::thread combatThread(boost::bind(&chessCombatProcesser::run, &g_combatProcess));
    combatThread.detach();

    boost::thread multiCombatThread(boost::bind(&MultiCombatProcesser::run, &g_MultiCombatProcess));
    multiCombatThread.detach();

    cout<<"************* start success ***************"<<endl;


    // 退出命令处理...
    std::string in;
    while (!m_quit)
    {
        do_sleep(1000);
        //db.KeepAlive();
        //定时的处理
        Update();
    }
    runServer1.Stop();

    json_spirit::mObject obj;

    //关闭服务器
    obj.clear();
    obj["cmd"] = "shutdown";
    if (0 != InsertInternalActionWork(obj))
    {
        ERR();
    }
    while (g_charDbJobqueue.jobcounts()
            || g_SavedbJobqueue.jobcounts()
            || g_mailJobqueue.jobcounts()
            || g_combatJobqueue.jobcounts()
            || g_multiCombatJobqueue.jobcounts()
            || g_runServer->getActionwork().jobcounts())
    {
        do_sleep(5000);
        cout<<"wait close chardb:"<<g_charDbJobqueue.jobcounts()<<",mailjob:"<<g_mailJobqueue.jobcounts()<<",savedb:"<<g_SavedbJobqueue.jobcounts();
        cout<<",combat:"<<g_combatJobqueue.jobcounts()<<",multi_combat:"<<g_multiCombatJobqueue.jobcounts()<<",action:"<<g_runServer->getActionwork().jobcounts()<<endl;
    }
    runServer1.StopAction();
    g_charDbProcess.stop();
    g_mailProcess.stop();
    g_combatProcess.stop();
    g_MultiCombatProcess.stop();
    g_saveDbProcess.stop();   // 停止.

    while (g_saveDbProcess.running())
    {
        cout<<"*********wait saveing db***********"<<endl<<flush;
        do_sleep(1000);
    }

    int wait_times = 0;
    while (runServer1._loginworkerptr->running()
            || runServer1._actionworkerptr->running()
            || g_mailProcess.running()
            || g_charDbProcess.running()
            || g_combatProcess.running()
            || g_MultiCombatProcess.running())
    {
        ++wait_times;
        cout<<"*********wait close***********"<<endl<<flush;
        if (wait_times % 10 == 0)
        {
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
            if (g_mailProcess.running())
            {
                cout<<"can not stop mail processer"<<endl;
            }
            if (g_charDbProcess.running())
            {
                cout<<"can not stop car db processer"<<endl;
            }
            if (g_combatProcess.running())
            {
                cout<<"can not stop combat processer"<<endl;
            }
            if (g_MultiCombatProcess.running())
            {
                cout<<"can not stop multi_combat processer"<<endl;
            }
        }
        if (wait_times > 600)
        {
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
            if (g_mailProcess.running())
            {
                syslog(LOG_ERR, "can not stop mail processer");
            }
            if (g_charDbProcess.running())
            {
                syslog(LOG_ERR, "can not stop car db processer");
            }
            if (g_combatProcess.running())
            {
                syslog(LOG_ERR, "can not stop combat processer");
            }
            if (g_MultiCombatProcess.running())
            {
                syslog(LOG_ERR, "can not stop multi_combat processer");
            }
            break;
        }
        do_sleep(1000);
    }
    cout<<"game down ... "<<endl<<flush;

    log_msg("game quit", 888);
    return 0;
}

