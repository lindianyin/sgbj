
#include "general_train.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "utils_all.h"
#include "json_spirit.h"
#include "spls_timer.h"
#include "spls_errcode.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

//重生打折活动
extern volatile int g_reborn_discount;// = 100;

int generalTrainQue::start()
{
    //cout<<"******generalTrainQue start pos="<<pos<<",state="<<state<<","<<cid<<endl;
    //重启后的情况
    int leftsecond = end_time - time(NULL);

    json_spirit::mObject mobj;
    mobj["cmd"] = "trainDone";
    mobj["cid"] = cid;
    mobj["pos"] = pos;
    boost::shared_ptr<splsTimer> tmsg;
    if (leftsecond <= 0)
    {
        //直接完成了
        tmsg.reset(new splsTimer(1, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    else
    {
        tmsg.reset(new splsTimer(leftsecond, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    save();
    return 0;
}

int generalTrainQue::save()
{
    int cur_level = 0, pre_level = 0, cur_color = 0, pre_color = 0, gid = 0;
    if (general.get())
    {
        gid = general->gid;
        cur_level = general->cur_level;
        pre_level = general->pre_level;
        cur_color = general->cur_color;
        pre_color = general->pre_color;
    }
    InsertSaveDb("update char_train_place set state=" + LEX_CAST_STR(state) +
        ",gid=" + LEX_CAST_STR(gid) +
        ",type=" + LEX_CAST_STR(type) +
        ",speed_time=" + LEX_CAST_STR(speed_time) +
        ",cur_level=" + LEX_CAST_STR(cur_level) +
        ",pre_level=" + LEX_CAST_STR(pre_level) +
        ",cur_color=" + LEX_CAST_STR(cur_color) +
        ",pre_color=" + LEX_CAST_STR(pre_color) +
        ",starttime=" + LEX_CAST_STR(start_time) +
        ",endtime=" + LEX_CAST_STR(end_time) +
        " where cid=" + LEX_CAST_STR(cid) +
        " and pos=" + LEX_CAST_STR(pos));
    return 0;
}

#ifdef JP_SERVER
int generalTrainQue::getSpeedGold()
{
    int needgold = 0;
    if (speed_time < 0)
        speed_time = 0;
    int now_time = speed_time + 1;
    if (now_time < 6)
    {
        needgold = 2;
        for (int i = 1; i < now_time; ++i)
        {
            needgold *= 2;
        }
    }
    else if(now_time < 11)
    {
        needgold = 60;
    }
    else if(now_time < 51)
    {
        needgold = 100;
    }
    else if(now_time < 201)
    {
        needgold = 200;
    }
    else
    {
        needgold = 400;
    }
    if (needgold && g_reborn_discount != 100)
    {
        needgold = needgold * g_reborn_discount / 100;
        if (needgold < 1)
        {
            needgold = 1;
        }
    }
    return needgold;
}
#else
int generalTrainQue::getSpeedGold()
{
    int needgold = 0;
    if (speed_time < 0)
        speed_time = 0;
    int now_time = speed_time + 1;
    //前三次每次增长
    if (now_time <= 3)
    {
        needgold = 2;
        for (int i = 1; i < now_time; ++i)
        {
            needgold *= 2;
        }
    }
    else
    {
        needgold = 8;
        int tmp = (now_time - 4)/3;
        for (int i = 1; i <= tmp; ++i)
        {
            needgold *= 2;
        }
        if (needgold > 128)
            needgold = 128;
    }
    if (needgold && g_reborn_discount != 100)
    {
        needgold = needgold * g_reborn_discount / 100;
        if (needgold < 1)
        {
            needgold = 1;
        }
    }
    return needgold;
}
#endif

int generalTrainQue::resetSpeedtime()
{
    speed_time = 0;
    InsertSaveDb("update char_train_place set speed_time=" + LEX_CAST_STR(speed_time) +
        " where cid=" + LEX_CAST_STR(cid) +
        " and pos=" + LEX_CAST_STR(pos));
    return 0;
}

TrainMgr* TrainMgr::m_handle = NULL;

TrainMgr* TrainMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new TrainMgr();
        m_handle->load();
    }
    return m_handle;
}

int TrainMgr::load()
{
    int init[24] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    memcpy(prop_silver, init, 24*sizeof(int));
    memcpy(prop_gold, init, 24*sizeof(int));
    memcpy(prop_best, init, 24*sizeof(int));
    memcpy(prop_sys, init, 24*sizeof(int));

    Query q(GetDb());
    q.get_result("select id,quality,hours,uplevel,name,memo,silver_prop, gold_prop,best_prop,sys_prop from base_books where 1");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<Book> p_b;
        p_b.reset(new Book);
        p_b->id = id;
        p_b->quality = q.getval();
        p_b->hours = q.getval();
        p_b->uplevel = q.getval();
        p_b->name = q.getstr();
        p_b->memo = q.getstr();
        if (id >= 1 && id <= 24)
        {
            prop_silver[id-1] = (int)(q.getnum() * 100);
            prop_gold[id-1] = (int)(q.getnum() * 100);
            prop_best[id-1] = (int)(q.getnum() * 100);
            prop_sys[id-1] = (int)(q.getnum() * 100);
        }
        m_base_books[id] = p_b;
    }
    q.free_result();
    
    return 0;
}

boost::shared_ptr<Book> TrainMgr::GetBook(int id)
{
    std::map<int, boost::shared_ptr<Book> >::iterator it = m_base_books.find(id);
    if (it != m_base_books.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<Book> p;
        p.reset();
        return p;
    }
}

int TrainMgr::updateBook(int type, boost::shared_ptr<Book>* books)
{
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();

    //cout<<"update book ...type:"<<type<<endl;
    int prop[24];
    switch (type)
    {
        case 1:
            memcpy(prop, prop_silver, 24*sizeof(int));
            break;
        case 2:
            memcpy(prop, prop_gold, 24*sizeof(int));
            break;
        case 3:
            memcpy(prop, prop_best, 24*sizeof(int));
            break;
        default:
            memcpy(prop, prop_sys, 24*sizeof(int));
            break;
    }
    for (int i = 0; i < general_book_nums; ++i)
    {
        //cout<<"update book ..."<<(i+1)<<endl;
        boost::random::discrete_distribution<> distg(prop);
        int idx = distg(gen);
        prop[idx] = 0;
        //成功获取则加入到结果并重置颜色
        if (m_base_books[idx + 1].get())
        {
            books[i].reset();
            books[i] = m_base_books[idx + 1];
        }
    }
    return HC_SUCCESS;
}

