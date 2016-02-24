#pragma once

#include <ctime>
#include "json_spirit.h"
#include <boost/thread.hpp>
#include "boost/smart_ptr/shared_ptr.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <map>
#include <list>

struct splsTimer
{
    static volatile uint64_t _refs;
    uint64_t _start_time;
    uint64_t _fire_time;
    int _times;
    double _period;
    boost::uuids::uuid _uuid;
    int _event_to;        //定时器触发后，发送event到哪个工作队列
    json_spirit::mObject _event;
    splsTimer(double period, int to, json_spirit::mObject& ev, int times = 0, double firstf = 0);
    ~splsTimer();
};

class splsTimerMgr
{
public:
    splsTimerMgr();
    ~splsTimerMgr();
    void workloop();
    void run();
    boost::uuids::uuid addTimer(boost::shared_ptr<splsTimer> t);
    bool delTimer(boost::uuids::uuid uuid);
    boost::uuids::uuid genUUID();
    static splsTimerMgr* getInstance();

private:
    uint64_t _next_fire_time;
    boost::mutex mutex_;

    std::list<boost::shared_ptr<splsTimer> > _timer_list;
    std::map<boost::uuids::uuid,int> _timer_uuids;

    boost::shared_ptr<boost::thread> _threadptr;
    static splsTimerMgr* m_handle;
};

