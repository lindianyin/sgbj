#ifndef _SWEEP_H_
#define _SWEEP_H_

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

//扫荡精英
struct SweepTask
{
    int stronghold_id;
    int left_time;
};

struct SweepResult
{
    int stronghold_id;
    std::vector<boost::shared_ptr<Item> > itemlist;
};

//玩家扫荡信息
struct charSweep
{
    charSweep(int cid)
    {
        m_cid = cid;
        m_type = 0;
        m_mapid = 0;
        m_left_fights = 0;
        now_sweep_id = 0;
        m_need_ling = 0;
        m_start_time = 0;
        m_end_time = 0;
        m_auto_buy_ling = false;
        m_auto_sell = false;
        m_fast_mod = 0;
    }
    std::list<int> m_sweep_task;    //玩家扫荡精英队列
    std::vector<boost::shared_ptr<SweepResult> > m_sweep_itemlist;//玩家扫荡精英掉落
    int m_type;//扫荡类型
    int m_cid;
    int m_mapid;
    int m_left_fights;
    int now_sweep_id;
    int m_need_ling;
    int m_fast_mod;        // 1立即完成模式
    time_t m_start_time;    //任务开始时间
    time_t m_end_time;        //任务结束时间
    bool m_auto_buy_ling;
    bool m_auto_sell;
    int start();
    int _start();
    int speedup();
    int done(int total_time);
    int done_all();
    int cancel();
    int stop();
    void save();
    int addSweepTask(int mapid, int stageid, int strongholdid, int type);

    boost::uuids::uuid _uuid;    //定时器唯一id
};

//战斗类
class sweepMgr
{
public:
    sweepMgr();
    ~sweepMgr();
    static sweepMgr* getInstance();
    int Save(int cid);
    int Start(int cid);
    int SpeedUp(int cid);
    int Cancel(int cid);
    int Done(int cid, int total_time);
    int addSweepTask(int cid, int mapid, int stageid, int strongholdid, int type);
    boost::shared_ptr<charSweep> getCharSweepData(int cid);
private:
    static sweepMgr* m_handle;
    std::map<int,boost::shared_ptr<charSweep> > m_sweep_task;//玩家扫荡精英信息
};

#endif

