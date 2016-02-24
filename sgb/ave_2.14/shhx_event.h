
#pragma once

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include <map>
#include <time.h>

struct CharData;

//领宝箱
int open_box(CharData* pc, int level);

//水浒豪侠充值活动
class shhx_recharge_event
{
    
};

//角色消费活动数据
struct char_cost_event
{
    int _cid;            //角色id
    int _total_cost;    //活动累计消费金币
    int _next;            //下一个可以得宝箱
    int _next_level;    //下一个宝箱等级
    int _can_get;        //可以领取的宝箱
    int _can_gets[5];    //每级可以领的宝箱

    boost::shared_ptr<CharData> _cdata;

    std::string open_box();
    void save();
};

//水浒豪侠消费活动
class shhx_cost_event
{
public:
    void load();        //加载活动
    int query_event(int cid, json_spirit::Object& robj);
    int update_cost_event(int cid, int gold_cost);
    int openBox(int cid, std::string&);
    boost::shared_ptr<char_cost_event> getChar(int cid);

    int leftSecs();

    static int get_next(int& gold);
    static shhx_cost_event* getInstance();
private:
    static shhx_cost_event* m_handle;
    bool m_enable;

    time_t m_start_time;
    time_t m_end_time;

    std::map<int, boost::shared_ptr<char_cost_event> > m_char_datas;
};

//角色武将升级活动数据
struct char_general_upgrade_event
{
    int _cid;            //角色id
    int _total_score;    //活动累计积分

    int _can_get;        //可以领取的宝箱
    int _geted;            //已经领取宝箱

    boost::shared_ptr<CharData> _cdata;

    std::string open_box();
    void save();
};

//武将升级活动
class shhx_generl_upgrade_event
{
public:
    void load();        //加载活动
    int add_score(int cid, int score);
    int openBox(int cid, std::string&);
    int query_event(int cid, json_spirit::Object& robj);
    int leftSecs();

    boost::shared_ptr<char_general_upgrade_event> getChar(int cid);

    static shhx_generl_upgrade_event* getInstance();
private:
    static shhx_generl_upgrade_event* m_handle;
    bool m_enable;

    time_t m_start_time;
    time_t m_end_time;

    std::map<int, boost::shared_ptr<char_general_upgrade_event> > m_char_datas;
    
};

