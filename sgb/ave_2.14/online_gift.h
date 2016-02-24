
#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include <list>
#include <map>
#include <time.h>
#include "json_spirit.h"
#include "base_item.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

struct CharData;

//在线礼包
struct online_gift
{
    online_gift(int id, int mins)
    {
        _id = id;
        _need_mins = mins;
        _need_secs = 60 * mins;
    }
    int _id;
    int _need_mins;
    int _need_secs;

    std::list<Item> _rewards;
};

//角色在线礼包
struct char_online_gift
{
    char_online_gift(int cid)
    {
        _uuid = boost::uuids::nil_uuid();
        _cid = cid;
    }
    int _cid;
    boost::shared_ptr<CharData> _cdata;
    boost::shared_ptr<online_gift> _online_gift;
    bool _canGet;            //能否领取了
    time_t _start_time;    //开始时间

    boost::uuids::uuid _uuid;    //定时器唯一id

    void reset();        //重新开始计时

    bool del_timer();

    void load();        //加载数据

    void open();        //开启礼包
    

    void add2obj(const std::string&, json_spirit::Object&);
};

//在线礼包管理
class online_gift_mgr
{
public:
    void load();
    boost::shared_ptr<char_online_gift> getChar(int cid);
    boost::shared_ptr<online_gift> getGift(int day, int id);
    void charLogout(int cid);
    void on_timer(int cid);
    void resetAll();

    static online_gift_mgr* getInstance();
    
private:
    static online_gift_mgr* m_handle;
    std::map<int, boost::shared_ptr<char_online_gift> > m_char_datas;

    std::vector<boost::shared_ptr<online_gift> > m_online_gifts;

    int m_online_gift_count;

    std::string m_timer_msg;
};

