#pragma once

#include <vector>
#include <string>
#include <list>
#include <map>
#include <boost/cstdint.hpp>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "loot.h"
#include "json_spirit.h"
#include "data.h"
#include "singleton.h"

//玩家扫荡信息
struct charSweep
{
    charSweep(int cid)
    {
        m_cid = cid;
        //m_type = 0;
        m_sweep_id = 0;
        m_start_time = 0;
        m_end_time = 0;
        m_fast_mod = 0;
    }
    //int m_type;//扫荡类型
    int m_cid;
    int m_sweep_id;
    int m_fast_mod;        // 1立即完成模式
    time_t m_start_time;    //任务开始时间
    time_t m_end_time;        //任务结束时间
    std::list<Item> itemlist;//玩家扫荡掉落
    int start();
    int speedup();
    int done();
    int stop();
    void save();
    boost::uuids::uuid _uuid;    //定时器唯一id
};

class sweepMgr
{
public:
    sweepMgr();
    int speedUp(int cid);
    int done(int cid);
    boost::shared_ptr<charSweep> getCharSweepData(int cid);
private:
    std::map<int,boost::shared_ptr<charSweep> > m_sweep_task;//玩家扫荡精英信息
};

//扫荡的处理
int ProcessSweepDone(json_spirit::mObject& o);
//开始扫荡
int ProcessSweep(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

