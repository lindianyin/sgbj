#pragma once

#include "base_item.h"

#include "net.h"

#include "singleton.h"

#include <boost/uuid/uuid.hpp>

#include <vector>


//军团探索

struct CharData;

struct baseCorpsExploreData
{
    int quality;
    float fac;
    int gailv;
    int mins;
};

struct corpsExploreData
{
    int pos;
    int state;    // 0未领取  1已经领取
    int type;
    std::string name;

    int quality;

    int need_secs;

    time_t done_time;

    boost::uuids::uuid m_done_timer;

    float fac;

    void Save();

    void refresh();

    bool isDone();

    bool Done();

    void start(int cid);

    corpsExploreData();
};

struct charCorpsExplore
{
    int cid;
    int special;
    std::string special_name;

    time_t refresh_time;

    time_t refresh_special;

    corpsExploreData que[3];

    void refresh(int pos);

    void Save();

    charCorpsExplore(int id);
    int getDoneTime();
    bool isDone();
    void checkRefresh();
};

class corpsExplore
{
public:
    corpsExplore();
    boost::shared_ptr<charCorpsExplore> getChar(int cid);
    baseCorpsExploreData* getQuality(int quality);
    std::string getName(int type);

    int refresh(std::string& name, int& quality, int& secs, float& fac);

    int speed(CharData& cdata, int pos);
private:
    std::map<int, boost::shared_ptr<charCorpsExplore> > m_datas;
    std::vector<baseCorpsExploreData> m_baseDatas;
    std::vector<int> m_gailvs;
    std::vector<std::string> m_names;
};

//军团探索列表
int ProcessCorpsExplreList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//军团探索领取
int ProcessCorpsExploreAccept(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//军团探索放弃
int ProcessCorpsExploreAbandon(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//军团探索刷新
int ProcessCorpsExploreRefresh(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

//军团探索完成
int ProcessCorpsExploreDone(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

int ProcessCorpsExploreFinish(json_spirit::mObject& o);


