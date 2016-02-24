#pragma once

#include <string>
#include <map>
#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "combat_def.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "spls_const.h"

//兵书
struct Book
{
    int id;
    int quality;
    int hours;
    int uplevel;
    std::string name;
    std::string memo;
};

//武将信息
struct charGeneral
{
    int cid;
    int gid;
    int pre_level;
    int pre_color;
    int cur_level;
    int cur_color;
};

//武将训练队列
struct generalTrainQue
{
    int pos;    //位置
    int cid;    //角色id
    int type;    //队列列别 0 普通 1 金钻
    int speed_time;    //加速次数

    boost::shared_ptr<charGeneral> general;    //训练的武将

    time_t start_time;    //开始时间
    time_t end_time;    //结束时间

    int state;    //状态0未开放1空闲2已被占用
    int start();

    boost::uuids::uuid _uuid;    //定时器唯一id

    int save();
    int getSpeedGold();
    int resetSpeedtime();
};

class TrainMgr
{
public:
    int load();
    //获得兵书
    boost::shared_ptr<Book> GetBook(int id);
    static TrainMgr* getInstance();
    //刷新训练兵书
    int updateBook(int mapid, boost::shared_ptr<Book>* books);

private:
    static TrainMgr* m_handle;
    std::map<int, boost::shared_ptr<Book> > m_base_books;        //兵书列表
    int prop_silver[24];
    int prop_gold[24];
    int prop_best[24];
    int prop_sys[24];
};

