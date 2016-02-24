#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "new_combat.hpp"
#include "net.h"

struct CharData;

struct base_wild_city_info
{
    int pos;
    std::string name;
    int x;
    int y;
};

struct wild_city
{
    int m_id;
    int m_owner_cid;//占领玩家
    int m_defense_hid;//城守英雄
    int m_levy_start;//税收开始时间(设置城守开始计算)
    int m_fight;//标记战斗状态
    bool m_get_notify;//可领取通知标记
    bool m_notify;//超过5k通知标记
    std::vector<int> m_viewer;//可看到的玩家
    void reset();
    void toObj(json_spirit::Object& robj);
    void save();
    void broadCastInfo();
    void addViewer(int cid);
    void removeViewer(int cid);
    int levy_get();
};

struct CharWildCitys
{
    CharData& m_chardata;
    std::vector<int> m_view_citys;

    CharWildCitys(CharData& c)
    :m_chardata(c)
    {
    };
    int load();
    int wildLevy(int id, json_spirit::Object& robj);//税收野外城池
    int wildDefense(int purpose, int id, int hid, json_spirit::Object& robj);
    void wildViewRefresh();
    bool checkView(int id);
    int getOwnCnt();
    void save();
};

class wildMgr
{
public:
    wildMgr();
    boost::shared_ptr<base_wild_city_info> getWildInfo(int pos);
    wild_city* getWildCity(int id);
    int getRandomWildCity();
    void checkWildCity();
    int combatResult(chessCombat* pCombat);    //战斗结束
private:
    int m_max_wild_city_id;
    std::vector<boost::shared_ptr<wild_city> > m_wild_citys;
    std::vector<boost::shared_ptr<base_wild_city_info> > base_wildinfo_list;//城外配置信息
};

//获取城外城池列表
int ProcessQueryWildCitys(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//城堡收税
int ProcessWildCityLevy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//野外城池设置城守
int ProcessWildCityDefense(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

