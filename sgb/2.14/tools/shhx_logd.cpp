
#include <stdio.h>
#include <iostream>
#include <syslog.h>
#include <signal.h>
#include <list>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <boost/thread.hpp>

using namespace std;

void do_sleep(int ms)
{
    boost::xtime xt;
    boost::xtime_get(&xt,boost::TIME_UTC_);
    xt.nsec += ms%1000*1000*1000;
    xt.sec += ms/1000;
    boost::thread::sleep(xt);
}

volatile int m_quit = 0;

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

#define buff_len 256

struct my_msg_st {

    long int my_msg_type;
    char some_text[buff_len + 1];
};

int main(int argc,char *argv[])
{
    //cout<<"=>"<<argv[0]<<endl;
    std::string que_name = "";
    if (argc > 1)
    {
        que_name = argv[1];
        //cout<<"que:"<<que_name<<endl;
    }

    if (que_name == "")
    {
        cout<<"error!"<<endl;
        return -1;
    }
    siginit();

    if (argc == 2)
    {
        key_t key = ftok(que_name.c_str(), 'a');
        if (-1 == key)
        {
            cout<<"create key fail."<<endl;
            return -1;
        }
        int msg_id = msgget(key, IPC_CREAT|0666);
        if (-1 == msg_id)
        {
            cout<<"msgget fail"<<endl;
            return -1;
        }
        my_msg_st msg;

        openlog(que_name.c_str(), LOG_PID|LOG_CONS, LOG_USER);

        std::list<std::string> m_logs;
        for (;!m_quit;)
        {
            ssize_t recv_size = msgrcv(msg_id, &msg, buff_len, 0, IPC_NOWAIT);
            while (recv_size > 0)
            {
                if (recv_size && recv_size <= buff_len)
                {
                    msg.some_text[recv_size] = 0;
                    switch (msg.my_msg_type)
                    {
                        case 777:
                            {
                                //cout<<"!!!!!!!!!!!!!recv restart msg!!!!!!!!!!!!!!!"<<endl;
                                time_t timep; 
                                time (&timep); 
                                syslog(LOG_DEBUG, "restart at %s", ctime(&timep));
                                int i = 1;
                                for (std::list<std::string>::iterator it = m_logs.begin(); it != m_logs.end(); ++it)
                                {
                                    syslog(LOG_DEBUG, "%d - %s", i, it->c_str());
                                    ++i;
                                }
                                m_logs.clear();
                            }
                            break;
                        case 888:
                            {
                                //cout<<"recv clear msg"<<endl;
                                //clear
                                time_t timep; 
                                time (&timep); 
                                syslog(LOG_DEBUG, "shutdown at %s", ctime(&timep));
                                m_logs.clear();
                            }
                            break;
                        case 999:
                            {
                                syslog(LOG_DEBUG, "********** print log *********");
                                int i = 1;
                                for (std::list<std::string>::iterator it = m_logs.begin(); it != m_logs.end(); ++it)
                                {
                                    syslog(LOG_DEBUG, "%d - %s", i, it->c_str());
                                    ++i;
                                }
                            }
                            break;
                        default:
                            {
                                //cout<<"recv msg:"<<msg.some_text<<endl;
                                m_logs.push_back(msg.some_text);
                                if (m_logs.size() > 500)
                                {
                                    m_logs.pop_front();
                                }
                            }
                            break;
                    }
                }
                recv_size = msgrcv(msg_id, &msg, buff_len, 0, IPC_NOWAIT);
            }
            do_sleep(1000);
        }
        //cout<<"quit..."<<endl;
        time_t timep; 
        time (&timep); 
        syslog(LOG_DEBUG, "**************** logd quit at %s *****************", ctime(&timep));
        int i = 1;
        for (std::list<std::string>::iterator it = m_logs.begin(); it != m_logs.end(); ++it)
        {
            syslog(LOG_DEBUG, "%d - %s", i, it->c_str());
            ++i;
        }
        closelog();

        if (msgctl(msg_id, IPC_RMID, 0) == -1) {

            fprintf(stderr, "msgctl(IPC_RMID) failed\n");

            exit(EXIT_FAILURE);
        }
    }
    else if (argc >= 3)
    {
        int type = 1;
        if (argc >= 4)
        {
            type = atoi(argv[3]);
            if (type < 0)
            {
                type = 1;
            }
        }
        //cout<<"type="<<type<<endl;
        key_t key = ftok(que_name.c_str(), 'a');
        if (-1 == key)
        {
            cout<<"create key fail."<<endl;
            return -1;
        }
        int msg_id = msgget(key, IPC_CREAT);
        if (-1 == msg_id)
        {
            cout<<"msgget fail"<<endl;
            return -1;
        }
        my_msg_st msg;

        if (strlen(argv[2]) >= buff_len)
        {
            cout<<"too long"<<endl;
            return -1;
        }
        msg.my_msg_type = type;
        strncpy(msg.some_text, argv[2], buff_len);

        cout<<"msg:"<<msg.some_text<<endl;
        
        if (msgsnd(msg_id, &msg, strlen(argv[2]) + 1, IPC_NOWAIT))
        {
             cout<<"msgsnd:fail"<<endl;
        }
    }
    return -1;
}

