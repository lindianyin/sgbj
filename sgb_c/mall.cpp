
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>

#include "mall.h"
#include "utils_all.h"
#include "singleton.h"
#include "data.h"
#include "hero.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#define MALL_ACTION "mall_action"

#define CHANGE_MALL_DISCOUNT "change_mall_discount"

Database& GetDb();

extern void InsertSaveDb(const std::string& sql);

//商城
int ProcessQueryMallInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int page = 1, nums_per_page = 8, action = 0, type = ITEM_TYPE_HERO_PACK;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    READ_INT_FROM_MOBJ(action, o, "action");
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("action", action) );
    robj.push_back( Pair("type", type) );
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 8;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<baseGoods> > p_list;
    if (action)
    {
        p_list = Singleton<mallMgr>::Instance().GetActionMallGoods();
    }
    else
    {
        p_list = Singleton<mallMgr>::Instance().GetBaseMallGoods();
    }
    std::map<int, boost::shared_ptr<baseGoods> >::iterator it = p_list.begin();
    while(it != p_list.end())
    {
        boost::shared_ptr<baseGoods> p_bg = it->second;
        if (p_bg.get())
        {
            if (action)//限时包无需关心类型和推荐
            {
                ;
            }
            else if (p_bg->type != type)
            {
                //类型不对跳过
                ++it;
                continue;
            }
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                if (p_bg->m_item.get())
                {
                    json_spirit::Object obj;
                    p_bg->toObj(obj);
                    json_spirit::Object i_obj;
                    if (p_bg->type == ITEM_TYPE_HERO_PACK)
                    {
                        boost::shared_ptr<base_hero_pack> pbhp = Singleton<HeroMgr>::Instance().GetBaseHeroPack(p_bg->m_item->id);
                        if (pbhp)
                        {
                            pbhp->toObj(i_obj);
                        }
                    }
                    else
                    {
                        p_bg->m_item->toObj(i_obj);
                    }
                    obj.push_back( Pair("good_info", i_obj));
                    list.push_back(obj);
                }
                else
                {
                    ERR();
                }
            }
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    if (page > maxpage)
        page = maxpage;
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//商城热卖
int ProcessQueryMallHotInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int page = 1, nums_per_page = 8;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 8;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    json_spirit::Array suggest_list;
    std::map<int, boost::shared_ptr<baseGoods> > p_list = Singleton<mallMgr>::Instance().GetBaseMallGoods();
    std::map<int, boost::shared_ptr<baseGoods> >::iterator it = p_list.begin();
    while(it != p_list.end())
    {
        boost::shared_ptr<baseGoods> p_bg = it->second;
        if (p_bg.get())
        {
            //推荐插入推荐列表
            if (p_bg->be_suggest && p_bg->m_item.get())
            {
                ++cur_nums;
                if (cur_nums >= first_nums && cur_nums <= last_nums)
                {
                    json_spirit::Object obj;
                    p_bg->toObj(obj);
                    json_spirit::Object i_obj;
                    if (p_bg->type == ITEM_TYPE_HERO_PACK)
                    {
                        boost::shared_ptr<base_hero_pack> pbhp = Singleton<HeroMgr>::Instance().GetBaseHeroPack(p_bg->m_item->id);
                        if (pbhp)
                        {
                            pbhp->toObj(i_obj);
                        }
                    }
                    else
                    {
                        p_bg->m_item->toObj(i_obj);
                    }
                    obj.push_back( Pair("good_info", i_obj));
                    suggest_list.push_back(obj);
                }
            }
        }
        ++it;
    }
    robj.push_back( Pair("suggest_list", suggest_list) );
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    if (page > maxpage)
        page = maxpage;
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//商城
int ProcessBuyMallGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0, nums = 0, action = 0, type = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    READ_INT_FROM_MOBJ(action, o, "action");
    READ_INT_FROM_MOBJ(type, o, "type");
    if (nums < 1)
        nums = 1;
    std::map<int, boost::shared_ptr<baseGoods> > p_list;
    if (action)
    {
        if (Singleton<mallMgr>::Instance().isActionOpen())
        {
            p_list = Singleton<mallMgr>::Instance().GetActionMallGoods();
        }
        else
        {
            return HC_ERROR;
        }
    }
    else
    {
        p_list = Singleton<mallMgr>::Instance().GetBaseMallGoods();
    }

	//计算商城折扣
	struct mall_discount_st mallDiscount_st = Singleton<mallMgr>::Instance().m_mall_discount_st;
	float fDiscount = 1;
	time_t tNow = time(NULL);

	if(mallDiscount_st.start< tNow && tNow< mallDiscount_st.end)
	{
		fDiscount = mallDiscount_st.discount / 100.0;
	}
    std::map<int, boost::shared_ptr<baseGoods> >::iterator it = p_list.find(id);
    if (it != p_list.end())
    {
        boost::shared_ptr<baseGoods> p_bg = it->second;
        if (p_bg.get() && p_bg->m_item.get())
        {
            if (p_bg->m_item->type == ITEM_TYPE_GEM && cdata->m_bag.isFull())
            {
                return HC_ERROR_BAG_FULL;
            }
            //商城购买英雄包直接开启
            if (p_bg->m_item->type == ITEM_TYPE_HERO_PACK)
            {
                json_spirit::mObject mobj;
                mobj["cmd"] = "openHeroPack";
                mobj["id"] = p_bg->m_item->id;
                mobj["type"] = type;
                return Singleton<HeroMgr>::Instance().OpenHeroPack(robj,mobj,cdata);
            }
            //总价溢出，购买超大数量时
            int cost_gold = (int)(p_bg->gold * fDiscount) * nums;
            int cost_silver = (int)(p_bg->silver * fDiscount) * nums;
            if (cost_gold <= 0 && cost_silver <= 0)
            {
                return HC_ERROR;
            }
            if (p_bg->m_item->type == ITEM_TYPE_HERO)
            {
                if (cdata->m_heros.isFull())
                    return HC_ERROR_HERO_FULL;
            }
            else if(p_bg->m_item->type == ITEM_TYPE_GEM)
            {
                if (cdata->m_bag.isFull())
                {
                    return HC_ERROR_BAG_FULL;
                }
                boost::shared_ptr<baseGem> bt = GeneralDataMgr::getInstance()->GetBaseGem(p_bg->m_item->id);
                if (!bt.get())
                {
                    return HC_ERROR;
                }
                if (bt->max_size > 0)
                {
                    int left = cdata->m_bag.getSize() - cdata->m_bag.getUsed();
                    left *= bt->max_size;
                    if (left < nums)
                    {
                        nums = left;
                    }
                }
                cost_gold = (int)(p_bg->gold * fDiscount) * nums;
                cost_silver = (int)(p_bg->silver * fDiscount) * nums;
                //商城碎片VIP折扣
                if (bt->usage == GEM_USAGE_ELITE_HERO)
                {
                    cost_gold = cost_gold * iMallGoldFac[cdata->m_vip] / 100;
                }
            }
            else if(p_bg->m_item->type == ITEM_TYPE_BAOSHI)
            {
                if (cdata->m_bag.isFull())
                {
                    return HC_ERROR_BAG_FULL;
                }
                boost::shared_ptr<baseBaoshi> bt = GeneralDataMgr::getInstance()->GetBaseBaoshi(p_bg->m_item->id);
                if (!bt.get())
                {
                    return HC_ERROR;
                }
                int left = cdata->m_bag.getSize() - cdata->m_bag.getUsed();
                left *= MAX_BAOSHI_COUNT;
                if (left < nums)
                {
                    nums = left;
                }
                cost_gold = (int)(p_bg->gold * fDiscount)  * nums;
                cost_silver = (int)(p_bg->silver * fDiscount) * nums;
            }
            //扣除资源
            if (type == 1 && cost_silver > 0)//筹码购买
            {
                if (cdata->subSilver(cost_silver, silver_cost_buy_mall_good) < 0)
                    return HC_ERROR_NOT_ENOUGH_SILVER;
            }
            else if(type == 2 && cost_gold > 0)//金币购买
            {
                if (cdata->subGold(cost_gold, gold_cost_buy_mall_good, true) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
            }
            else
            {
                return HC_ERROR;
            }
            if (p_bg->m_item->type == ITEM_TYPE_HERO)
            {
                cdata->m_heros.Add(p_bg->m_item->id);
            }
            else if(p_bg->m_item->type == ITEM_TYPE_GEM)
            {
                //给道具
                cdata->addGem(p_bg->m_item->id, nums, gem_get_buy_mall);
                //更新任务
                cdata->m_tasks.updateTask(GOAL_MALL_GEM, p_bg->m_item->id, 1);
            }
            else if(p_bg->m_item->type == ITEM_TYPE_BAOSHI)
            {
                cdata->addBaoshi(p_bg->m_item->id, p_bg->m_item->extra, nums);
            }
            //通知
            json_spirit::Object getobj;
            Item i(p_bg->m_item->type, p_bg->m_item->id, nums, p_bg->m_item->extra);
            i.toObj(getobj);
            robj.push_back( Pair("get", getobj) );
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

void baseGoods::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("goods_id", id));
    obj.push_back( Pair("type", type));
    obj.push_back( Pair("be_suggest", be_suggest));
    obj.push_back( Pair("gold", gold));
    obj.push_back( Pair("org_gold", org_gold));
    obj.push_back( Pair("silver", silver));
    obj.push_back( Pair("org_silver", org_silver));
}

mallMgr::mallMgr()
{
    Query q(GetDb());

    //商城宝物
    q.get_result("SELECT id,gold,silver,suggest,itemType,itemId,extra FROM base_mall_goods WHERE inUse=1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseGoods> p_bg;
        p_bg.reset(new baseGoods);
        m_base_mall_goods[id] = p_bg;
        p_bg->id = id;
        p_bg->org_gold = q.getval();
        p_bg->org_silver = q.getval();
        p_bg->be_suggest = q.getval();
        p_bg->gold = p_bg->org_gold;
        p_bg->silver = p_bg->org_silver;
        if (p_bg->be_suggest)
        {
            p_bg->gold = p_bg->org_gold * 80 / 100;
            p_bg->silver = p_bg->org_silver * 80 / 100;
        }
        boost::shared_ptr<Item> p_i;
        p_i.reset(new Item);
        p_i->type = q.getval();
        p_i->id = q.getval();
        p_i->extra = q.getval();
        p_i->nums = 1;
        p_bg->m_item = p_i;
        p_bg->type = p_i->type;
    }
    q.free_result();

    //折扣商品
    //refresh();
    //限时活动
    loadAction();
}

void mallMgr::refresh()
{
    //刷新折扣商品
    int mall_refresh = GeneralDataMgr::getInstance()->getInt("mall_refresh", 0);
    int mall_good1 = GeneralDataMgr::getInstance()->getInt("mall_good1", 0);
    int mall_good2 = GeneralDataMgr::getInstance()->getInt("mall_good2", 0);
    time_t t_now = time(NULL);
    if (mall_good1 == 0 || mall_good2 == 0 || mall_refresh < t_now)
    {
        //随机新折扣商品
        int tmp1 = 0, tmp2 = 0;
        if (m_base_mall_goods.size() > 1)
        {
            tmp1 = my_random(0, m_base_mall_goods.size()-1);
            do
            {
                tmp2 = my_random(0, m_base_mall_goods.size()-1);
            }while (tmp2 == tmp1);
        }
        int tmp = 0;
        std::map<int, boost::shared_ptr<baseGoods> >::iterator it = m_base_mall_goods.begin();
        while(it != m_base_mall_goods.end())
        {
            boost::shared_ptr<baseGoods> p_bg = it->second;
            if (p_bg.get() && p_bg->m_item.get())
            {
                p_bg->be_suggest = 0;
                p_bg->gold = p_bg->org_gold;
            }
            if (tmp == tmp1)
            {
                mall_good1 = it->first;
                GeneralDataMgr::getInstance()->setInt("mall_good1", mall_good1);
            }
            if (tmp == tmp2)
            {
                mall_good2 = it->first;
                GeneralDataMgr::getInstance()->setInt("mall_good2", mall_good2);
            }
            ++tmp;
            ++it;
        }
        GeneralDataMgr::getInstance()->setInt("mall_refresh", t_now + iMallRefreshCD);
    }
    std::map<int, boost::shared_ptr<baseGoods> >::iterator it = m_base_mall_goods.begin();
    while(it != m_base_mall_goods.end())
    {
        boost::shared_ptr<baseGoods> p_bg = it->second;
        if (p_bg.get() && p_bg->m_item.get() && (it->first == mall_good1 || it->first == mall_good2))
        {
            p_bg->be_suggest = 1;
            p_bg->gold = p_bg->org_gold * 80 / 100;
        }
        ++it;
    }
}

void mallMgr::loadAction()
{
    //商城折扣活动
	m_mall_discount_st.discount = 100;
	m_mall_discount_st.end = 0;
	m_mall_discount_st.start = 0;
	std::string mall_discount_str = GeneralDataMgr::getInstance()->getStr(CHANGE_MALL_DISCOUNT);
	if(mall_discount_str != "")
	{
		json_spirit::mValue mall_discount_value;
		json_spirit::read(mall_discount_str, mall_discount_value);
        if (mall_discount_value.type() == json_spirit::obj_type)
        {
            json_spirit::mObject& o = mall_discount_value.get_obj();
            READ_INT_FROM_MOBJ(m_mall_discount_st.discount,o,"discount");
            READ_INT_FROM_MOBJ(m_mall_discount_st.start,o,"start_time");
            READ_INT_FROM_MOBJ(m_mall_discount_st.end,o,"end_time");
        }
	}
    m_day = 0;
    m_end_time = 0;
    m_start_time = 0;
    m_start_day = 0;
    m_action_mall_goods.clear();

    std::string data = GeneralDataMgr::getInstance()->getStr(MALL_ACTION);
    json_spirit::mValue value;
    json_spirit::read(data, value);
    if (value.type() == json_spirit::obj_type)
    {
        json_spirit::mObject& o = value.get_obj();
        READ_INT_FROM_MOBJ(m_start_time,o,"start_time");
        READ_INT_FROM_MOBJ(m_day,o,"day");
        time_t t_c = m_start_time;
        struct tm tm;
        struct tm *t = &tm;
        localtime_r(&t_c, t);
        t->tm_hour = 0;
        t->tm_min = 0;
        t->tm_sec = 0;
        m_start_day = mktime(t);
        m_end_time = m_start_day + iONE_DAY_SECS * m_day;

        json_spirit::mArray list;
        int action_good_id = 0;
        READ_ARRAY_FROM_MOBJ(list,o,"goods_list");
        json_spirit::mArray::iterator it = list.begin();
        while (it != list.end())
        {
            if ((*it).type() != json_spirit::obj_type)
            {
                ++it;
                continue;
            }
            json_spirit::mObject& tmp_obj = (*it).get_obj();
            int itemType = 0, itemId = 0, extra = 0, cost = 0;
            READ_INT_FROM_MOBJ(itemType,tmp_obj,"type");
            READ_INT_FROM_MOBJ(itemId,tmp_obj,"id");
            READ_INT_FROM_MOBJ(extra,tmp_obj,"extra");
            READ_INT_FROM_MOBJ(cost,tmp_obj,"cost");
            ++action_good_id;
            boost::shared_ptr<baseGoods> p_bg;
            p_bg.reset(new baseGoods);
            m_action_mall_goods[action_good_id] = p_bg;
            p_bg->id = action_good_id;
            p_bg->be_suggest = 0;
            p_bg->gold = cost;
            p_bg->org_gold = cost;
            boost::shared_ptr<Item> p_i;
            p_i.reset(new Item);
            p_i->type = itemType;
            p_i->id = itemId;
            p_i->extra = extra;
            p_i->nums = 1;
            p_bg->m_item = p_i;
            p_bg->type = p_i->type;
            ++it;
        }
    }
}

bool mallMgr::isActionOpen()
{
    time_t time_now = time(NULL);
    return m_start_time <= time_now && m_end_time > time_now;
}

void mallMgr::openMallDiscountEvent(int discount, time_t start_time, time_t end_time)
{
    m_mall_discount_st.discount = discount;
    m_mall_discount_st.start = start_time;
    m_mall_discount_st.end = end_time;
    //json object to string
    json_spirit::Object discount_obj;
    discount_obj.push_back( Pair("discount", m_mall_discount_st.discount) );
    discount_obj.push_back( Pair("start_time", m_mall_discount_st.start) );
    discount_obj.push_back( Pair("end_time", m_mall_discount_st.end) );
    GeneralDataMgr::getInstance()->setStr(CHANGE_MALL_DISCOUNT, json_spirit::write(discount_obj));
}

std::map<int, boost::shared_ptr<baseGoods> >& mallMgr::GetBaseMallGoods()
{
    return m_base_mall_goods;
}

std::map<int, boost::shared_ptr<baseGoods> >& mallMgr::GetActionMallGoods()
{
    return m_action_mall_goods;
}

