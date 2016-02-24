
#include "spls_timer.h"
#include <boost/interprocess/ipc/message_queue.hpp>
#include "net.h"
#include "utils_all.h"
#include <boost/progress.hpp>
#include <iostream>
#include <climits>

extern volatile int m_quit;
extern volatile int g_print_debug_info;

using namespace net;
using namespace std;
using namespace boost::interprocess;

#define INFO(x) //cout<<x

extern int InsertInternalActionWork(json_spirit::mObject& obj);

extern boost::mt19937* get_random_generator();

volatile uint64_t splsTimer::_refs = 0;

splsTimer::    splsTimer(double f, int to, json_spirit::mObject& ev, int times, double firstf)
{
    ++splsTimer::_refs;
    _event = ev;
    _start_time = splsTimeStamp();
    if (firstf > 0)
    {
        _fire_time = (uint64_t)(firstf*1000000) + _start_time;
    }
    else
    {
        _fire_time = (uint64_t)(f*1000000) + _start_time;
    }
    _times = times;
    _period = f;
    _event_to = to;
}

splsTimer::~splsTimer()
{
    --splsTimer::_refs;
}

splsTimerMgr* splsTimerMgr::m_handle = NULL;

splsTimerMgr* splsTimerMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new splsTimerMgr();
        m_handle->run();
    }
    return m_handle;
}

splsTimerMgr::splsTimerMgr()
{
    _next_fire_time = 0;
    _threadptr.reset(new boost::thread(boost::bind(&splsTimerMgr::workloop, this)));
}

splsTimerMgr::~splsTimerMgr()
{
    if (_threadptr)
    {
        _threadptr->join();
    }
}

void splsTimerMgr::workloop()
{
    cout<<"splsTimerMgr::workloop()"<<endl;

#ifdef DEBUG_PER
    time_t last_time = 0;
#endif

    while (!m_quit)
    {
        boost::mutex::scoped_lock lock(mutex_);
        uint64_t clock_now = splsTimeStamp();
        //cout<<"clock now "<<clock_now<<endl;
        //触发了时间
        if (_next_fire_time <= clock_now)
        {
#ifdef DEBUG_PER
            if (g_print_debug_info)
            {
                time_t time_now = time(NULL);
                if (last_time != time_now && time_now % 10 == 0)
                {                
                    cout<<"======================= splsTimerMgr::workloop : total timers "<<dec<<splsTimer::_refs<<endl<<flush;
                }
                last_time = time_now;
            }
#endif

            std::list<boost::shared_ptr<splsTimer> >::iterator it = _timer_list.begin();
            while (it != _timer_list.end())
            {
                if (clock_now >= (*it)->_fire_time)
                {
                    //time_t t = time(NULL);
                    //cout<<"**************"<<clock_now<<">="<<(*it)->_fire_time<<endl;
                    std::string cmd = "";
                    READ_STR_FROM_MOBJ(cmd, (*it)->_event, "cmd");
                    INFO(cout<<"**************"<<ctime(&t)<<",timer fire,cmd:"<<cmd);
                    /*if ((*it)->_times == 1)
                    {
                        (*it)->_event["done"] = 1;
                    }*/
                    if (0 != InsertInternalActionWork((*it)->_event))
                    {
                        ERR();
                    }
                    --(*it)->_times;
                    if ((*it)->_times == 0)
                    {
                        it = _timer_list.erase(it);
                        INFO(cout<<"************** timer erase!"<<endl);
                    }
                    else
                    {
                        INFO(cout<<"************** timer left "<<(*it)->_times<<endl);
                        boost::shared_ptr<splsTimer> timermsg = *it;
                        _timer_list.erase(it);
                        //重新加入定时器
                        timermsg->_fire_time = (uint64_t)(timermsg->_period*1000000) + splsTimeStamp();
                        //cout<<"************** reset at "<<timermsg->_fire_time<<endl;
                        it = _timer_list.begin();
                        while (it != _timer_list.end())
                        {
                            if (timermsg->_fire_time < (*it)->_fire_time)
                            {
                                break;
                            }
                            ++it;
                        }
                        if (it == _timer_list.begin())
                        {
                            _next_fire_time = timermsg->_fire_time;
                            //cout<<"************** reset set next fire at "<<_next_fire_time<<endl;
                        }
                        _timer_list.insert(it, timermsg);
                        it = _timer_list.begin();
                    }
                }
                else
                {
                    _next_fire_time = (*it)->_fire_time;
                    //cout<<"************** 2 set next fire at "<<_next_fire_time<<endl;
                    break;
                }
            }
        }
        lock.unlock();
        do_sleep(100);
    }
    return;
}

void splsTimerMgr::run()
{
    if (_threadptr)
    {
        _threadptr->detach();
    }
}

boost::uuids::uuid splsTimerMgr::genUUID()
{
    boost::uuids::basic_random_generator<boost::mt19937> gen(get_random_generator());
    while (true)
    {
        //cout<<"splsTimerMgr::genUUID()"<<endl;
        boost::uuids::uuid uuid = gen();
        std::map<boost::uuids::uuid,int>::iterator it = _timer_uuids.find(uuid);
        if (it == _timer_uuids.end())
        {
            //cout<<"splsTimerMgr::genUUID() return "<<hex;
            for (boost::uuids::uuid::iterator it = uuid.begin(); it != uuid.end(); ++it)
            {
                //cout<<*it<<"-";
            }
            //cout<<dec<<endl;
            return uuid;
        }
    }
    boost::uuids::uuid uuid = gen();
    return uuid;
}

boost::uuids::uuid splsTimerMgr::addTimer(boost::shared_ptr<splsTimer> timermsg)
{
    if (!timermsg.get())
    {
        //boost::uuids::nil_generator gen;
        return boost::uuids::nil_uuid();
    }
    boost::mutex::scoped_lock lock(mutex_);
    std::list<boost::shared_ptr<splsTimer> >::iterator it = _timer_list.begin();
    while (it != _timer_list.end())
    {
        if (timermsg->_fire_time < (*it)->_fire_time)
        {
            break;
        }
        ++it;
    }
    if (it == _timer_list.begin())
    {
        _next_fire_time = timermsg->_fire_time;
    }
    boost::uuids::uuid uuid = genUUID();
    timermsg->_uuid = uuid;
    _timer_list.insert(it, timermsg);
    _timer_uuids[uuid] = 1;
    //cout << "addTimer to event " << timermsg->_event_to << endl;
    return uuid;
}

bool splsTimerMgr::delTimer(boost::uuids::uuid uuid)
{
    boost::mutex::scoped_lock lock(mutex_);
    std::list<boost::shared_ptr<splsTimer> >::iterator it = _timer_list.begin();
    while (it != _timer_list.end())
    {
        //cout << "delTimer " << (*it)->_event_to << endl;
        if ((*it)->_uuid == uuid)
        {
            //cout << "find succ" << endl;
            bool del_first = (it == _timer_list.begin());
            it = _timer_list.erase(it);
            _timer_uuids.erase(uuid);
            if (del_first)
            {
                if (_timer_list.size() == 0)
                {
                    _next_fire_time = std::clock();
                }
                else
                {
                    _next_fire_time = (*it)->_fire_time;
                }
            }
            return true;
        }
        else
        {
            ++it;
        }
    }
    return false;
}

