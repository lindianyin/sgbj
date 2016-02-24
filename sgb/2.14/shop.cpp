
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>

#include "shop.h"
#include "utils_all.h"
#include "singleton.h"
#include "data.h"
#include "statistics.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

Database& GetDb();
extern std::string strCounts;
extern std::string strBuy;

extern void InsertSaveDb(const std::string& sql);

//商店打折活动
extern volatile int g_shop_discount;

//商店刷新时间 6小时
volatile int g_shop_refresh_secs = 6*3600;

//商店刷新次数增加 x ?
volatile int g_shop_refresh_more = 1;

void charShop::Save(int pos)
{
    InsertSaveDb("update char_shop_goods set type=" + LEX_CAST_STR(m_goods[pos-1].baseGoods->type)
        + ",id=" + LEX_CAST_STR(m_goods[pos-1].baseGoods->id)
        + ",can_buy=" + (m_goods[pos-1].canbuy ? "1" : "0")
        + " where cid=" + LEX_CAST_STR(m_charData.m_id)
        + " and pos=" + LEX_CAST_STR(pos)
        );
}

int charShop::buy(int pos)                //购买商品
{
    if (m_charData.m_shopOpen == 0)
    {
        return HC_ERROR;
    }
    if (pos >= 1 && pos <= 6)
    {
        if (!m_goods[pos-1].canbuy)
        {
            return HC_ERROR;
        }
        if (m_goods[pos-1].baseGoods->type != 2 ||
            (m_goods[pos-1].baseGoods->type == 2
                && m_goods[pos-1].baseGoods->id != treasure_type_supply
                && treasure_type_mati_tie != m_goods[pos-1].baseGoods->id))
        {
            if (m_charData.m_bag.isFull())
            {
                return HC_ERROR_BAG_FULL;
            }
        }
        if (m_charData.addSilver(-m_goods[pos-1].baseGoods->price) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_SILVER;
        }
        //银币消耗统计
        add_statistics_of_silver_cost(m_charData.m_id,m_charData.m_ip_address,m_goods[pos-1].baseGoods->price,silver_cost_for_shop_buy, m_charData.m_union_id, m_charData.m_server_id);
        switch (m_goods[pos-1].baseGoods->type)
        {
            //道具
            case 2:
                //宝物获得统计
                add_statistics_of_treasure_cost(m_charData.m_id,m_charData.m_ip_address,m_goods[pos-1].baseGoods->id,m_goods[pos-1].baseGoods->num,treasure_shop,1,m_charData.m_union_id, m_charData.m_server_id);
                m_charData.addTreasure(m_goods[pos-1].baseGoods->id, m_goods[pos-1].baseGoods->num);

                //材料
                if (m_goods[pos-1].baseGoods->use == 9)
                {
                    //支线任务
                    m_charData.m_trunk_tasks.updateTask(task_shop_buy_mat, 1);
                }
                break;
            //宝石
            case 6:
                m_charData.giveBaoshi(m_goods[pos-1].baseGoods->id, 1, baoshi_buy);
                //支线任务
                m_charData.m_trunk_tasks.updateTask(task_shop_buy_baoshi, 1);
                break;
            //装备
            case 7:
                {
                    std::string tmp;
                    m_charData.addEquipt(m_goods[pos-1].baseGoods->id, tmp);
                }
                break;
        }
        m_goods[pos-1].canbuy = false;
        Save(pos);

        if (m_goods[pos-1].baseGoods->need_notify)
        {
            Singleton<shopMgr>::Instance().addShopRecord(m_charData.m_name, m_goods[pos-1].baseGoods->type, m_goods[pos-1].baseGoods->name, m_goods[pos-1].baseGoods->num, m_goods[pos-1].baseGoods->quality);
        }
        //act统计
        act_to_tencent(&m_charData,act_new_shop);
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

int charShop::refresh(int type)         //刷新商品
{
    if (!m_charData.m_shopOpen)
    {
        return HC_ERROR;
    }
    if (type == 2)//gold
    {
        int refresh_times = m_charData.queryExtraData(char_data_type_daily, char_data_shop_refresh) + 1;
        if (refresh_times > iRefreshShopGoldVIP[m_charData.m_vip]*g_shop_refresh_more)
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        int need_gold = 0;
#ifdef JP_SERVER
        if (refresh_times <= 3)
        {
            need_gold = 10;
        }
        else if(refresh_times <= 10)
        {
            need_gold = 20;
        }
        else if(refresh_times <= 50)
        {
            need_gold = 50;
        }
#else
        need_gold = refresh_times * iRefreshShopGoldCost;
        if (need_gold <= 0)
        {
            need_gold = iRefreshShopGoldCost;
        }
#endif
        if (-1 == m_charData.addGold(-need_gold))
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(m_charData.m_id, m_charData.m_ip_address, need_gold, gold_cost_for_refresh_shop, m_charData.m_union_id, m_charData.m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(&m_charData,need_gold,gold_cost_for_refresh_shop);
#endif
        m_charData.NotifyCharData();

        m_charData.setExtraData(char_data_type_daily, char_data_shop_refresh, refresh_times);

        //修改刷新时间
        m_refresh_time = time(NULL);
        m_charData.setExtraData(char_data_type_normal, char_data_shop_refresh_time, m_refresh_time);
    }
    return Singleton<shopMgr>::Instance().refresh(*this);
}

//查询商品列表
int charShop::getList(json_spirit::Array& glist)
{
    for (int pos = 1; pos <= 6; ++pos)
    {
        if (!m_goods[pos-1].baseGoods.get())
        {
            return HC_ERROR;
        }
        json_spirit::Object shop;
        shop.push_back( Pair("pos", pos) );
        shop.push_back( Pair("type", m_goods[pos-1].baseGoods->type) );
        //商店宝石默认1级
        if (m_goods[pos-1].baseGoods->type == 6)
        {
            shop.push_back( Pair("level", 1) );
        }
        shop.push_back( Pair("id", m_goods[pos-1].baseGoods->id) );
        shop.push_back( Pair("name", m_goods[pos-1].baseGoods->name) );
        shop.push_back( Pair("memo", m_goods[pos-1].baseGoods->memo) );
        shop.push_back( Pair("nums", m_goods[pos-1].baseGoods->num) );
        shop.push_back( Pair("quality", m_goods[pos-1].baseGoods->quality) );
        shop.push_back( Pair("spic", m_goods[pos-1].baseGoods->spic) );
        shop.push_back( Pair("price", m_goods[pos-1].baseGoods->price) );
        shop.push_back( Pair("org_price", m_goods[pos-1].baseGoods->org_price) );
        shop.push_back( Pair("canBuy", m_goods[pos-1].canbuy) );
        glist.push_back(shop);
     }
    return HC_SUCCESS;
}

//加载商品
void charShop::Load()
{
    m_refresh_time = 0;
    if (m_charData.m_shopOpen)
    {
        m_refresh_time = m_charData.queryExtraData(char_data_type_normal, char_data_shop_refresh_time);
        Query q(GetDb());
        //角色商店商品信息加载
        q.get_result("select pos,type,id,can_buy from char_shop_goods where cid=" + LEX_CAST_STR(m_charData.m_id) + " order by pos");
        CHECK_DB_ERR(q);
        if (q.num_rows() < 6)
        {
            q.free_result();
            InsertSaveDb("delete from char_shop_goods where cid=" + LEX_CAST_STR(m_charData.m_id));
            for(int pos = 1; pos <= 6; ++pos)
            {
                InsertSaveDb("insert into char_shop_goods set cid=" + LEX_CAST_STR(m_charData.m_id)
                    + ",pos=" + LEX_CAST_STR(pos));
            }
            refresh(0);
            return;
        }

        while(q.fetch_row())
        {
            int pos = q.getval();
            if (pos < 1 || pos > 6)
            {
                continue;
            }
            int type = q.getval();
            int id = q.getval();
            uint8_t cb = q.getval();

            //初始化商品
            Singleton<shopMgr>::Instance().initGoogs(type, id, cb, m_goods[pos-1]);
        }
        q.free_result();
    }
}

shopMgr::shopMgr()
{
    Query q(GetDb());

    //变身卡
    int tid = 0;
    q.get_result("select id,spic,name,memo,quality from base_treasures where type=10 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        if (tid != 0 && tid + 1 != id)
        {
            //变身卡必须连续
            ERR();
            exit(0);
        }
        if (tid == 0)
        {
            m_card_start_id = id;
        }
        tid = id;
        baseShopGoods* pg = new baseShopGoods();
        pg->type = 2;
        pg->need_notify = false;
        pg->id = id;
        pg->spic = q.getval();
        pg->name = q.getstr();
        pg->memo = q.getstr();
        pg->quality = q.getval();
        pg->price = 100;
        pg->org_price = pg->price;
        pg->num = 1;
        pg->use = 10;
        boost::shared_ptr<baseShopGoods> bg(pg);
        m_card_list.push_back(bg);

        m_gem_type_map[id] = 10;
    }
    q.free_result();

    //cout<<"card start from "<<m_card_start_id<<endl;
    //材料
    tid = 0;
    q.get_result("select id,spic,name,memo,quality,up_quality from base_treasures where type=9 order by id");
    while (q.fetch_row())
    {
        int id = q.getval();
        if (tid != 0 && tid + 1 != id)
        {
            //必须连续
            ERR();
            exit(0);
        }
        if (tid == 0)
        {
            m_mat_start_id = id;
        }
        tid = id;
        baseShopGoods* pg = new baseShopGoods();
        pg->type = 2;
        pg->need_notify = false;
        pg->id = id;
        pg->spic = q.getval();
        pg->name = q.getstr();
        pg->memo = q.getstr();
        pg->quality = q.getval();
        pg->up_quality = q.getval();
        pg->price = 12000;
        pg->org_price = pg->price;
        pg->num = 50;
        pg->use = 9;
        boost::shared_ptr<baseShopGoods> bg(pg);
        m_mat_list.push_back(bg);
        m_gem_type_map[id] = 9;
    }
    q.free_result();

    //cout<<"mat start from "<<m_mat_start_id<<endl;

    tid = 0;
    q.get_result("select id,name,memo,quality from base_equipment where quality=1 order by id");
    while (q.fetch_row())
    {
        int id = q.getval();
        if (tid != 0 && tid + 1 != id)
        {
            //必须连续
            ERR();
            exit(0);
        }
        tid = id;
        baseShopGoods* pg = new baseShopGoods();
        pg->type = 7;
        pg->need_notify = true;
        pg->id = id;
        pg->spic = id;
        pg->name = q.getstr();
        pg->memo = q.getstr();
        pg->quality = q.getval();
        pg->price = 3000;
        pg->org_price = pg->price;
        pg->num = 1;
        boost::shared_ptr<baseShopGoods> bg(pg);
        m_equip_list.push_back(bg);
    }
    q.free_result();

    //宝石
    for (int i = 1; i <= 14; ++i)
    {
        baseShopGoods* pg = new baseShopGoods();
        baseNewBaoshi* pb = Singleton<newBaoshiMgr>::Instance().getBaoshi(i);
        if (!pb)
        {
            ERR();
            exit(0);
        }
        pg->type = 6;
        pg->need_notify = true;
        pg->id = i;
        pg->spic = i;
        pg->name = pb->name;
        pg->memo = pb->name;
        pg->quality = 1;
        pg->price = 25000;
        pg->org_price = pg->price;
        pg->num = 1;
        boost::shared_ptr<baseShopGoods> bg(pg);
        m_baoshi_list.push_back(bg);
    }

    //军粮
    boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_supply);
    if (bt.get())
    {
        baseShopGoods* pg = new baseShopGoods();
        pg->type = 2;
        pg->need_notify = true;
        pg->id = treasure_type_supply;
        pg->spic = bt->spic;
        pg->name = bt->name;
        pg->memo = bt->memo;
        pg->quality = bt->quality;
        pg->price = 25000;
        pg->org_price = pg->price;
        pg->num = 10000;
        m_supply_goods.reset(pg);


        #ifdef JP_SERVER
        for (int i = 0; i < supply_goods_max; ++i)
        {
            baseShopGoods* pg = new baseShopGoods();
            pg->type = 2;
            pg->need_notify = true;
            pg->id = treasure_type_supply;
            pg->spic = bt->spic;
            pg->name = bt->name;
            pg->memo = bt->memo;
            pg->quality = bt->quality;
            pg->price = supply_goods_data[i][1];
            pg->org_price = pg->price;
            pg->num = supply_goods_data[i][0];
            boost::shared_ptr<baseShopGoods> bg(pg);
            m_supply_list.push_back(bg);
        }
        #endif
    }
    else
    {
        ERR();
        exit(0);
    }

    //马蹄铁
    bt = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_mati_tie);
    if (bt.get())
    {
        baseShopGoods* pg = new baseShopGoods();
        pg->type = 2;
        pg->need_notify = true;
        pg->id = bt->id;
        pg->spic = bt->spic;
        pg->name = bt->name;
        pg->memo = bt->memo;
        pg->quality = bt->quality;
        pg->price = 25000;
        pg->org_price = pg->price;
        pg->num = 1;
        pg->use = 5;
        m_matitie_goods.reset(pg);
    }
    else
    {
        ERR();
        exit(0);
    }

    g_shop_discount = GeneralDataMgr::getInstance()->getInt("shop_discount", 100);
    if (g_shop_discount < 100)
    {
        updateShopPrice(g_shop_discount);
    }

    g_shop_refresh_more = GeneralDataMgr::getInstance()->getInt("shop_refresh", 1);
    if (g_shop_refresh_more < 1)
    {
        g_shop_refresh_more = 1;
    }

    //购买记录
    q.get_result("select value from custom_settings where code='shop_notices'");
    if (q.fetch_row())
    {
        std::string notices = q.getstr();
        q.free_result();
        json_spirit::read(notices, m_notices_value);
    }
    else
    {
        std::string notices = "[]";
        q.free_result();
        json_spirit::read(notices, m_notices_value);
    }
}

void shopMgr::updateShopPrice(int discount)
{
    for (std::vector<boost::shared_ptr<baseShopGoods> >::iterator it = m_mat_list.begin(); it != m_mat_list.end(); ++it)        //全部材料列表
    {
        if ((*it).get())
        {
            (*it)->price = (*it)->org_price * discount / 100;
        }
    }
    for (std::vector<boost::shared_ptr<baseShopGoods> >::iterator it = m_card_list.begin(); it != m_card_list.end(); ++it)//变身卡列表
    {
        if ((*it).get())
        {
            (*it)->price = (*it)->org_price * discount / 100;
        }
    }
    for (std::vector<boost::shared_ptr<baseShopGoods> >::iterator it = m_equip_list.begin(); it != m_equip_list.end(); ++it)    //装备列表
    {
        if ((*it).get())
        {
            (*it)->price = (*it)->org_price * discount / 100;
        }
    }
    for (std::vector<boost::shared_ptr<baseShopGoods> >::iterator it = m_baoshi_list.begin(); it != m_baoshi_list.end(); ++it)    //宝石列表
    {
        if ((*it).get())
        {
            (*it)->price = (*it)->org_price * discount / 100;
        }
    }
    if (m_supply_goods.get())
    {
        m_supply_goods->price = m_supply_goods->org_price * discount / 100;    //军粮
    }
    if (m_matitie_goods.get())    //马蹄铁
    {
        m_matitie_goods->price = m_matitie_goods->org_price * discount / 100;
    }
}

//查询购买记录
int shopMgr::getList(json_spirit::Array& glist)
{
    glist = m_notices_value.get_array();
    return HC_SUCCESS;
}

//根据材料对应的装备品质设定随机范围
void shopMgr::getRandomList(int level, std::vector<boost::shared_ptr<baseShopGoods> >& list, int min_q, int max_q)
{
    int min = 2, max = 7;
    if (min_q != 0 && max_q != 0)
    {
        min = min_q;
        max = max_q;
    }
    else if(level < 60)
    {
        min = 2;
        max = 4;
    }
    else if(level < 70)
    {
        min = 3;
        max = 5;
    }
    else if(level < 80)
    {
        min = 4;
        max = 6;
    }
    else
    {
        min = 5;
        max = 7;
    }
    for (std::vector<boost::shared_ptr<baseShopGoods> >::iterator it = m_mat_list.begin(); it != m_mat_list.end(); ++it)
    {
        if ((*it).get() && (*it)->up_quality >= min && (*it)->up_quality <= max)
        {
            list.push_back(*it);
        }
    }
}

//刷新商店
int shopMgr::refresh(charShop& shop)
{
    assert(m_card_list.size() > 0);
    assert(m_mat_list.size() > 0);
    assert(m_equip_list.size() > 0);
    assert(m_baoshi_list.size() > 0);

    //cout<<"refresh card..."<<endl;
    //第一个变身卡
    shop.m_goods[0].canbuy = true;
    shop.m_goods[0].baseGoods = m_card_list[my_random(0, m_card_list.size()-1)];

    #ifdef JP_SERVER
    //第二个军粮
    shop.m_goods[1].canbuy = true;
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    boost::random::discrete_distribution<> dist(supply_goods_gailv);
    int idx = dist(gen);
    shop.m_goods[1].baseGoods = m_supply_list[idx];
    #else
    //第二个变身卡
    shop.m_goods[1].canbuy = true;
    shop.m_goods[1].baseGoods = m_card_list[my_random(0, m_card_list.size()-1)];
    while (shop.m_goods[0].baseGoods == shop.m_goods[1].baseGoods)
    {
        shop.m_goods[1].baseGoods = m_card_list[my_random(0, m_card_list.size()-1)];
    }
    #endif

    //cout<<"refresh equip..."<<endl;
    //第3格    绿色装备一件    随机
    shop.m_goods[2].canbuy = true;
    shop.m_goods[2].baseGoods = m_equip_list[my_random(0, m_equip_list.size()-1)];
    //
    //第4格    材料    随机
    //第5格    材料    随机
    //材料根据等级出
    shop.m_goods[3].canbuy = true;
    shop.m_goods[4].canbuy = true;
    std::vector<boost::shared_ptr<baseShopGoods> > m_tmp_list;        //材料列表
    if (shop.m_charData.m_level <= 50)
    {
        m_tmp_list.clear();
        //50级特殊处理，必出一蓝色
        getRandomList(shop.m_charData.m_level,m_tmp_list,2,2);
        shop.m_goods[3].baseGoods = m_tmp_list[my_random(0, m_tmp_list.size()-1)];
        m_tmp_list.clear();
        getRandomList(shop.m_charData.m_level,m_tmp_list);
        shop.m_goods[4].baseGoods = m_tmp_list[my_random(0, m_tmp_list.size()-1)];
        while (shop.m_goods[4].baseGoods == shop.m_goods[3].baseGoods)
        {
            shop.m_goods[4].baseGoods = m_tmp_list[my_random(0, m_tmp_list.size()-1)];
        }
    }
    else
    {
        m_tmp_list.clear();
        getRandomList(shop.m_charData.m_level,m_tmp_list);
        if (m_tmp_list.size() > 2)
        {
            shop.m_goods[3].baseGoods = m_tmp_list[my_random(0, m_tmp_list.size()-1)];
            shop.m_goods[4].baseGoods = m_tmp_list[my_random(0, m_tmp_list.size()-1)];
            while (shop.m_goods[4].baseGoods == shop.m_goods[3].baseGoods)
            {
                shop.m_goods[4].baseGoods = m_tmp_list[my_random(0, m_tmp_list.size()-1)];
            }
        }
    }

    //cout<<"refresh last..."<<endl;
    //第六
    shop.m_goods[5].canbuy = true;
    switch (my_random(1,15))
    {
        #ifndef JP_SERVER
        case 1:
            shop.m_goods[5].baseGoods = m_supply_goods;
            break;
        #endif
        default:
            shop.m_goods[5].baseGoods = m_baoshi_list[my_random(0, m_baoshi_list.size()-1)];
            break;
    }
    for (int pos = 1; pos <= 6; ++pos)
    {
        shop.Save(pos);
    }
    //shop.m_refresh_time = time(NULL);
    //shop.m_charData.setExtraData(char_data_type_normal, char_data_shop_refresh_time, shop.m_refresh_time);
    return HC_SUCCESS;
}

int shopMgr::addShopRecord(const std::string& who, int type, const std::string& what, int count, int quality)
{
    json_spirit::Object rcd;
    std::string msg = strBuy;
    str_replace(msg, "$N", who);
    std::string get_name = what;

    if (count > 1)
    {
        get_name += (strCounts + LEX_CAST_STR(count));
    }

    addColor(get_name, quality);
    str_replace(msg, "$G", get_name);
    rcd.push_back( Pair("msg", msg) );

    json_spirit::Array& notice_array = m_notices_value.get_array();

    notice_array.push_back(rcd);
    while ((int)notice_array.size() > 15)
    {
        notice_array.erase(notice_array.begin());
    }
    //保存
    InsertSaveDb("replace into custom_settings (code,value) values ('shop_notices','" +GetDb().safestr(json_spirit::write(m_notices_value)) + "')");

    return 0;
}

//初始化商品
int shopMgr::initGoogs(int type, int id, bool cb, charShopGoods& goods)
{
    //cout<<"init goods "<<type<<","<<id<<endl;
    goods.canbuy = cb;
    // 1 资源(金币、军令)2道具3英雄宝物4技能,6宝石 7装备
    switch (type)
    {
        //道具
        case 2:
            {
                switch (id)
                {
                    case treasure_type_supply:
                        goods.baseGoods = m_supply_goods;
                        break;
                    case treasure_type_mati_tie:
                        goods.baseGoods = m_matitie_goods;
                        break;
                    default:
                        switch (m_gem_type_map[id])
                        {
                            //材料
                            case 9:
                                {
                                    int idx = id-m_mat_start_id;
                                    if (idx >= 0 && idx < m_mat_list.size())
                                    {
                                        goods.baseGoods = m_mat_list[id-m_mat_start_id];
                                    }
                                    else
                                    {
                                        ERR();
                                        cout<<"error mat type "<<id<<endl;
                                        goods.baseGoods = m_mat_list[0];
                                    }
                                }
                                break;
                            //变身卡
                            case 10:
                                {
                                    int idx = id-m_card_start_id;
                                    if (idx >= 0 && idx < m_card_list.size())
                                    {
                                        goods.baseGoods = m_card_list[id-m_card_start_id];
                                    }
                                    else
                                    {
                                        ERR();
                                        cout<<"error card type "<<id<<endl;
                                        goods.baseGoods = m_card_list[0];
                                    }
                                }
                                break;
                            default:
                                goods.baseGoods = m_card_list[0];
                                break;
                        }
                        break;
                }
            }
            break;
        //宝石
        case 6:
            if (id >= 1 && id <= 14)
            {
                goods.baseGoods = m_baoshi_list[id-1];
            }
            break;
        //装备
        case 7:
            if (id >= 1 && id <= m_equip_list.size())
            {
                goods.baseGoods = m_equip_list[id-1];
            }
            break;
    }
    return 0;
}

