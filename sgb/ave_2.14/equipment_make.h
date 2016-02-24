
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
    int m_scroll_id;        //����id
    int m_gem_id;            //��Ӧ�ĵ���id
    int m_src_equipment;    //��Ҫ��װ��
    int m_make_equipment;    //���ɵ�װ��

    boost::shared_ptr<baseEquipment> m_equipment_src;
    boost::shared_ptr<baseEquipment> m_equipment;
    
    std::list<std::pair<boost::shared_ptr<baseTreasure>,int> > m_mlist;        //�����б�

    json_spirit::Object m_eqobj;

    int m_silver_cost;        //���ĵ�����
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

