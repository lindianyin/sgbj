
#pragma once

#include <time.h>
#include <vector>
#include <map>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "item.h"

//�̳���Ʒ
struct baseGoods
{
    int id;
    int type;
    int be_suggest;
    int org_gold;
    int gold;
    int org_silver;
    int silver;
    boost::shared_ptr<Item> m_item;
    void toObj(json_spirit::Object& obj);
};

//�̳��ۿۻ
struct mall_discount_st
{
	int discount;
	time_t start;
	time_t end;
};



class mallMgr
{
public:
    mallMgr();
    void refresh();
    void loadAction();
    bool isActionOpen();
    void openMallDiscountEvent(int discount, time_t start_time, time_t end_time);
    std::map<int, boost::shared_ptr<baseGoods> >& GetBaseMallGoods();
    std::map<int, boost::shared_ptr<baseGoods> >& GetActionMallGoods();
	struct mall_discount_st m_mall_discount_st;
private:
    std::map<int, boost::shared_ptr<baseGoods> > m_base_mall_goods;         //�̳���Ʒ
    //�̳ǻ
    time_t m_start_time;
    time_t m_end_time;
    time_t m_start_day;
    int m_day;
    std::map<int, boost::shared_ptr<baseGoods> > m_action_mall_goods;         //��ʱ���Ʒ
};

//��ѯ�̳�
int ProcessQueryMallInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//��ѯ�̳�����
int ProcessQueryMallHotInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//�����̳�
int ProcessBuyMallGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

