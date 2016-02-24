
#pragma once

#include <time.h>
#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"

#ifdef JP_SERVER
const int supply_goods_max = 18;
const int supply_goods_data[supply_goods_max][2] =
{
    {1000, 25000},
    {1200, 25000},
    {1500, 25000},
    {1800, 25000},
    {2000, 30000},
    {2200, 30000},
    {2500, 30000},
    {2800, 30000},
    {3000, 40000},
    {3200, 40000},
    {3500, 40000},
    {3800, 40000},
    {4000, 50000},
    {4200, 50000},
    {4500, 50000},
    {4800, 50000},
    {5000, 60000},
    {10000, 60000}
};
const int supply_goods_gailv[supply_goods_max] =
{
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,2,2
};
#endif

struct CharData;
//基础商店物品
struct baseShopGoods
{
    baseShopGoods()
    {
        type = 0;
        id = 0;
        num = 0;
        spic = 0;
        org_price = 0;
        price = 0;
        quality = 0;
        up_quality = 0;
        use = 0;
        need_notify = false;
        name = "";
        memo = "";
    }
    int type;// 1 资源(金币、军令)2道具3英雄宝物4技能,6宝石 7装备
    int id;
    int num;
    int spic;
    int price;
    int org_price;
    int quality;
    int up_quality;
    int use;
    bool need_notify;
    std::string name;
    std::string memo;
};

struct charShopGoods
{
    bool canbuy;
    boost::shared_ptr<baseShopGoods> baseGoods;
};

struct charShop
{
    CharData& m_charData;
    time_t m_refresh_time;            //商品刷新时间
    charShopGoods m_goods[6];        //商品列表

    charShop(CharData& c)
    :m_charData(c)
    {
    }
    int buy(int pos);                //购买商品
    int refresh(int type);            //刷新商品
    //查询商品列表
    int getList(json_spirit::Array& glist);
    //加载商品
    void Load();
    
    void Save(int pos);
};

class shopMgr
{
public:
    shopMgr();
    //查询购买记录
    int getList(json_spirit::Array& glist);
    //刷新商店
    int refresh(charShop& shop);
    //初始化商品
    int initGoogs(int type, int id, bool cb, charShopGoods& goods);
    //添加购买记录
    int addShopRecord(const std::string& who, int type, const std::string& what, int count, int quality);

    void updateShopPrice(int discount);
    void getRandomList(int level, std::vector<boost::shared_ptr<baseShopGoods> >& list, int min_q=0, int max_q=0);

private:
    std::map<int,int> m_gem_type_map;
    int m_mat_start_id;
    int m_card_start_id;
    int m_equipt_start_id;
    std::vector<boost::shared_ptr<baseShopGoods> > m_mat_list;        //全部材料列表
    std::vector<boost::shared_ptr<baseShopGoods> > m_card_list;    //变身卡列表
    std::vector<boost::shared_ptr<baseShopGoods> > m_equip_list;    //装备列表
    std::vector<boost::shared_ptr<baseShopGoods> > m_baoshi_list;    //宝石列表
    boost::shared_ptr<baseShopGoods> m_supply_goods;    //军粮
    boost::shared_ptr<baseShopGoods> m_matitie_goods;    //马蹄铁
    #ifdef JP_SERVER
    std::vector<boost::shared_ptr<baseShopGoods> > m_supply_list;    //军粮列表
    #endif

    json_spirit::Value m_notices_value;
};

