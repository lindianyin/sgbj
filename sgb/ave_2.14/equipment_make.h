
#pragma once

#include <map>
#include <list>
#include <utility>
#include "json_spirit.h"
#include "boost/smart_ptr/shared_ptr.hpp"

struct baseEquipment;
struct baseTreasure;

struct equipment_scroll
{
    int m_scroll_id;        //卷轴id
    int m_gem_id;            //对应的道具id
    int m_src_equipment;    //需要的装备
    int m_make_equipment;    //生成的装备

    boost::shared_ptr<baseEquipment> m_equipment_src;
    boost::shared_ptr<baseEquipment> m_equipment;
    
    std::list<std::pair<boost::shared_ptr<baseTreasure>,int> > m_mlist;        //材料列表

    json_spirit::Object m_eqobj;

    int m_silver_cost;        //消耗的银币
};

class equipment_scroll_mgr
{
public:
    equipment_scroll_mgr();
    equipment_scroll* getScroll(int sid);
    equipment_scroll* getScrollBySrcId(int srcid);
private:
    void load();
    std::map<int, boost::shared_ptr<equipment_scroll> > m_scrolls;
    std::map<int, boost::shared_ptr<equipment_scroll> > m_scrolls2;
};

