
#include "newbaoshi.h"
#include "net.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_all.h"
#include "spls_errcode.h"
#include "data.h"
#include "singleton.h"
#include "new_ranking.h"
#include "statistics.h"
#include "new_event.h"
#include "daily_task.h"

using namespace net;

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

const std::string strBaoshiLink = "<A HREF=\"event:{'type':$T,'cmd':'showBaoshi','level':$L}\"><U>$N</U></A>";

void baseNewBaoshi::toObj(int level, json_spirit::Object& obj)
{
    obj.clear();
    if (level >= 1 && level <= MAX_BAOSHI_LEVEL)
    {
        obj.push_back( Pair("name", baseAttr.name) );
        obj.push_back( Pair("value", values[level-1]) );
    }
}

std::string baseNewBaoshi::Name_to_Link(int level)
{
    if (level < 1 || level > MAX_BAOSHI_LEVEL)
    {
        return "";
    }
    std::string m_link_name = strBaoshiLink;
    str_replace(m_link_name, "$T", LEX_CAST_STR(type));
    str_replace(m_link_name, "$L", LEX_CAST_STR(level));
    str_replace(m_link_name, "$N", (name + "Lv" + LEX_CAST_STR(level)));
    addColor(m_link_name, qualitys[level-1]);
    return m_link_name;
}

newBaoshi::newBaoshi(int id, baseNewBaoshi& base, int level, int count)
:iItem(iItem_type_baoshi, base.type, id, count)
,m_base(base)
,m_level(level)
{
    if (m_level >= 1 && m_level <= MAX_BAOSHI_LEVEL)
    {
        m_quality = m_base.qualitys[m_level-1];
        m_value = m_base.values[m_level-1];
        //if (m_exp == 0)
        //{
        //    m_exp = m_base.exps[m_level - 1];
        //}
    }
    else
    {
        m_quality = 0;
        m_value = 0;
        m_level = 1;
    }
    updateObj();
}

//宝石当前经验
//int newBaoshi::exp() const
//{
//    return m_exp;
//}

//int newBaoshi::addExp(int a)
//{
//    bool levelup = false;
//    m_exp += a;
//    while (canLevelup() && m_exp >= levelupExp())
//    {
//        levelup = true;
//        ++m_level;
//    }
//    if (levelup)
//    {
//        m_value = m_base.values[m_level-1];
//        updateObj();
//    }
//    return m_exp;
//}

//升级需要的经验
//int newBaoshi::levelupExp()
//{
//    if (m_level >= 1 && m_level < MAX_BAOSHI_LEVEL)
//    {
//        return m_base.exps[m_level];
//    }
//    return 0;
//}

int newBaoshi::level() const
{
    return m_level;
}

int newBaoshi::levelup()
{
    ++m_level;
    m_value = m_base.values[m_level-1];
    m_quality = m_base.qualitys[m_level-1];
    updateObj();
    m_changed = true;
    return m_level;
}

int newBaoshi::value() const
{
    return m_value;
}

void newBaoshi::setLevel(int l)
{
    if (l >= 1 && l <= MAX_BAOSHI_LEVEL)
    {
        m_level = l;
        //m_exp = m_base.exps[l-1];
        m_value = m_base.values[m_level-1];
        m_quality = m_base.qualitys[m_level-1];
        updateObj();
        m_changed = true;
    }
}

bool newBaoshi::canLevelup() const
{
    return m_level < MAX_BAOSHI_LEVEL;
}

std::string newBaoshi::memo() const
{
    return "";
}

std::string newBaoshi::name() const
{
    return m_base.name;
}

const json_spirit::Object& newBaoshi::getObj() const
{
    return m_obj;
}

int32_t newBaoshi::sellPrice() const
{
    return m_level * 1000;
}

void newBaoshi::updateObj()
{
    m_obj.clear();
    m_obj.push_back( Pair("name", m_base.baseAttr.name) );
    //if (m_base.baseAttr.isChance)
    //{
    //    m_obj.push_back( Pair("value", int2percent(m_value, 1000)) );
    //}
    //else
    {
        m_obj.push_back( Pair("value", m_value) );
    }
}

void newBaoshi::Save()
{
    if (m_changed)
    {
        int32_t c = getCount();    
        if (c > 0)
        {
            CharData* pc = getChar();
            InsertSaveDb("update char_baoshi set type=" + LEX_CAST_STR(m_base.type)
                                    + ",cid=" + LEX_CAST_STR(pc!=NULL ? pc->m_id : 0)
                                    + ",slot=" + LEX_CAST_STR((int)(getSlot()))
                                    + ",gid="+ LEX_CAST_STR(getGeneral().get() ? getGeneral().get()->m_id : 0)
                                    + ",level=" + LEX_CAST_STR(level())
                                    + ",nums=" + LEX_CAST_STR(c)
                                    + ",state=" + LEX_CAST_STR((int)m_state)
                                    + ",deleteTime=" + LEX_CAST_STR(m_deleteTime)
                                    + " where id=" + LEX_CAST_STR(getId()));
        }
        else
        {
            InsertSaveDb("delete from char_baoshi where id=" + LEX_CAST_STR(getId()));
        }
        m_changed = false;
    }
}

newBaoshiMgr::newBaoshiMgr()
{
    //int level_exp[MAX_BAOSHI_LEVEL] = {30,60,120,240,480,960,1920,3840,7680,15360,30720,61140,122880};
    int qualitys[MAX_BAOSHI_LEVEL] = {1,1,1,2,2,3,3,3,4,4,4,5,5};
    //从数据库中加载
    Query q(GetDb());
    q.get_result("select id,type,name,memo,lv1,lv2,lv3,lv4,lv5,lv6,lv7,lv8,lv9,lv10,lv11,lv12,lv13 from base_new_baoshi where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        if (id != m_baseBaoshis.size() + 1)
        {
            ERR();
            exit(0);
        }
        int type = q.getval();
        boost::shared_ptr<baseAttribute> att = attributeMgr::getInstance()->getAttribute(type);
        if (!att.get())
        {
            ERR();
            exit(0);
        }
        baseNewBaoshi *bs = new baseNewBaoshi(*att.get());
        bs->type = id;
        bs->name = q.getstr();
        bs->memo = q.getstr();
        bs->cost = 1800;
        bs->goldCost = 0;
        bs->silverCost = 10000;
        bs->atype = type;

        for (int i = 0; i < MAX_BAOSHI_LEVEL; ++i)
        {
            bs->values[i] = q.getval();
        }
        //memcpy(bs->exps, level_exp, sizeof(int)*MAX_BAOSHI_LEVEL);
        memcpy(bs->qualitys, qualitys, sizeof(int)*MAX_BAOSHI_LEVEL);

        m_baseBaoshis.push_back(bs);
    }
    q.free_result();

    m_baoshi_id = 0;
    q.get_result("select max(id) from char_baoshi where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_baoshi_id = q.getval();
    }
    q.free_result();
    load_shop();
}

newBaoshiMgr::~newBaoshiMgr()
{
    std::vector<baseNewBaoshi*>::iterator it = m_baseBaoshis.begin();
    while (it != m_baseBaoshis.end())
    {
        delete *it;
        ++it;
    }
    m_baseBaoshis.clear();
}

baseNewBaoshi* newBaoshiMgr::getBaoshi(int type)
{
    if (type >= 1 && type <= m_baseBaoshis.size())
    {
        return m_baseBaoshis[type-1];
    }
    return NULL;
}

//购买宝石
int newBaoshiMgr::buyBaoshi(CharData* pc, int& type, json_spirit::Object& robj, int buyType, int auto_buy)
{
    if (!pc)
    {
        return HC_ERROR;
    }

    if (pc->m_bag.isFull())
    {
        if (auto_buy)
        {
            int ret = pc->buyBagSlot(1, robj);
            if (ret != HC_SUCCESS)
            {
                return ret;
            }
        }
        else
        {
            return HC_ERROR_BAG_FULL;
        }
    }

    //宝石种类随机
    type = my_random(1, 14);
    baseNewBaoshi* bbs = getBaoshi(type);
    if (!bbs)
    {
        cout<<"unknow baoshi type "<<type<<endl;
        return HC_ERROR;
    }
    newBaoshi* p = pc->m_bag.getBaoshiCanMerge(type, 1, 1);

    switch (buyType)
    {
        //玉石购买
        case 1:
            if (bbs->cost <= 0 || bbs->silverCost <= 0)
            {
                return HC_ERROR;
            }
            if (pc->silver() < bbs->silverCost)
            {
                return HC_ERROR_NOT_ENOUGH_SILVER;
            }
            if (pc->addTreasure(treasure_type_yushi, -bbs->cost) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_YUSHI;
            }
            add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_yushi,bbs->cost,treasure_baoshi,2,pc->m_union_id,pc->m_server_id);
#ifdef QQ_PLAT
            treasure_cost_tencent(pc,treasure_type_yushi,bbs->cost);
#endif
            pc->addSilver(-bbs->silverCost);
            //银币消耗统计
            add_statistics_of_silver_cost(pc->m_id, pc->m_ip_address, bbs->silverCost, silver_cost_for_baoshi_buy, pc->m_union_id, pc->m_server_id);
            break;
#if 0    //金币购买
        case 2:
            if (bbs->goldCost <= 0)
            {
                return HC_ERROR;
            }
            if (pc->addGold(-bbs->goldCost) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            break;
#endif
        default:
            return HC_ERROR;
    }

    if (p)
    {
        p->addCount(1);
        p->Save();
    }
    else
    {
        boost::shared_ptr<iItem> bs = cloneBaoshi(type, 1, 1);
        if (!bs.get())
        {
            return HC_ERROR;
        }
        newBaoshi* pb = dynamic_cast<newBaoshi*>(bs.get());
        pc->m_bag.addItem(bs);
        pb->Save();
    }
    //周排行活动
    int score = 10;//兑换10分
    if (score > 0)
        newRankings::getInstance()->updateEventRankings(pc->m_id,rankings_event_baoshi,score);
    return HC_SUCCESS;
}

//克隆宝石
boost::shared_ptr<iItem> newBaoshiMgr::cloneBaoshi(int type, int level, int count)
{
    boost::shared_ptr<iItem> bs;
    baseNewBaoshi* bbs = getBaoshi(type);
    if (!bbs)
    {
         return bs;
    }
    int id = newBaoshiId();
    if (level < 1 || level > MAX_BAOSHI_LEVEL)
    {
        level = 1;
    }
    newBaoshi* pb = new newBaoshi(id, *bbs, level, count);
    bs.reset(pb);
    m_baoshiMap[id] = bs;

    InsertSaveDb("insert into char_baoshi set type=" + LEX_CAST_STR(pb->m_base.type)
                                //+ ",exp=" + LEX_CAST_STR(pb->exp())
                                + ",gid=0,slot=" + LEX_CAST_STR((int)(bs->getSlot()))
                                + ",level=" + LEX_CAST_STR(pb->level())
                                + ",cid=0,nums=1,id=" + LEX_CAST_STR((int)(bs->getId())));

    return bs;
}

//销毁宝石
void newBaoshiMgr::destroyBaoshi(int id)
{
    m_baoshiMap.erase(id);    
}

//注册宝石
void newBaoshiMgr::registerBaoshi(int id, boost::shared_ptr<iItem> bs)
{
    m_baoshiMap[id] = bs;
}

//查询宝石
boost::shared_ptr<iItem> newBaoshiMgr::queryBaoshi(int id)
{
    std::map<int, boost::shared_ptr<iItem> >::iterator it = m_baoshiMap.find(id);
    if (it != m_baoshiMap.end())
    {
        return it->second;
    }
    boost::shared_ptr<iItem> tmp;
    return tmp;
}

//增加宝石
int newBaoshiMgr::addBaoshi(CharData* pc, int type, int level, int count, int reason)
{
    if (!pc)
    {
        return HC_ERROR;
    }

    newBaoshi* p = pc->m_bag.getBaoshiCanMerge(type, level, count);
    if (p)
    {
        p->addCount(count);
        p->Save();
        //宝石统计
        add_statistics_of_baoshi_get(pc->m_id, pc->m_ip_address, pc->m_union_id, pc->m_server_id, type, level, count, reason);
    }
    else
    {
        boost::shared_ptr<iItem> bs = cloneBaoshi(type, level, count);
        if (!bs.get())
        {
            return HC_ERROR;
        }
        //宝石统计
        add_statistics_of_baoshi_get(pc->m_id, pc->m_ip_address, pc->m_union_id, pc->m_server_id, type, level, count, reason);
        if (pc->m_bag.isFull())
        {
            bs->setDeleteTime();
            pc->m_selled_bag.add(bs);
            //宝石卖出统计
            add_statistics_of_baoshi_cost(pc->m_id, pc->m_ip_address, pc->m_union_id, pc->m_server_id, type, level, count, baoshi_sell);
        }
        else
        {
            pc->m_bag.addItem(bs);
        }

        newBaoshi* pb = dynamic_cast<newBaoshi*>(bs.get());
        pb->Save();
    }
    
    //宝石活动
    Singleton<new_event_mgr>::Instance().addBaoshi(pc->m_id, level);
    
    json_spirit::Object obj;
    json_spirit::Object item;
    item.push_back( Pair("type", item_type_baoshi) );
    item.push_back( Pair("id", type) );
    item.push_back( Pair("fac", level) );
    item.push_back( Pair("level", level) );
    obj.push_back( Pair("item", item) );
    obj.push_back( Pair("cmd", "notify") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("type", notify_msg_new_get) );
    pc->sendObj(obj);
    return HC_SUCCESS;
}

void newBaoshiMgr::load_shop()
{
    Query q(GetDb());
    int cnt = 0;
    q.get_result("SELECT `pos`,`baoshi_type`,`baoshi_level`,`baoshi_counts` FROM baoshi_shop WHERE 1 order by pos");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        int pos = q.getval();
        if (pos > 0 && pos <= 6)
        {
            m_goods[pos-1].type = q.getval();
            m_goods[pos-1].level = q.getval();
            m_goods[pos-1].counts = q.getval();
            #ifdef JP_SERVER
            m_goods[pos-1].goldCostMax = 512;
            m_goods[pos-1].goldCost = 480;
            #else
            m_goods[pos-1].goldCostMax = 500;
            m_goods[pos-1].goldCost = m_goods[pos-1].goldCostMax * 8 / 10;
            #endif
            ++cnt;
        }
    }
    q.free_result();
    if (cnt < 6)
        refresh_shop();
}

//刷新限购宝石
void newBaoshiMgr::refresh_shop()
{
    std::vector<int> type_list;
    for (int type = 1; type <= 14; ++type)
        type_list.push_back(type);
    for (int pos = 0; pos < 6; ++pos)
    {
        int index = my_random(0, type_list.size()-1);
        m_goods[pos].type = type_list[index];
        #ifdef JP_SERVER
        m_goods[pos].level = 8;
        m_goods[pos].goldCostMax = 512;
        m_goods[pos].goldCost = 480;
        #else
        m_goods[pos].level = 5;
        m_goods[pos].goldCostMax = 500;
        m_goods[pos].goldCost = m_goods[pos].goldCostMax * 8 / 10;
        #endif
        m_goods[pos].counts = 5;
        type_list.erase(type_list.begin()+index);
    }
    save_shop(0);
}

void newBaoshiMgr::save_shop(int change_pos)
{
    if (change_pos == 0)
    {
        InsertSaveDb("TRUNCATE TABLE baoshi_shop");
        for (int pos = 0; pos < 6; ++pos)
        {
            InsertSaveDb("INSERT INTO baoshi_shop (`pos`,`baoshi_type`,`baoshi_level`,`baoshi_counts`) VALUES ('"
                + LEX_CAST_STR(pos+1) + "','"
                + LEX_CAST_STR(m_goods[pos].type) + "','"
                + LEX_CAST_STR(m_goods[pos].level) + "','"
                + LEX_CAST_STR(m_goods[pos].counts) + "')");
        }
    }
    else
    {
        InsertSaveDb("UPDATE baoshi_shop set `baoshi_counts`="+LEX_CAST_STR(m_goods[change_pos-1].counts)+" where `pos`="+LEX_CAST_STR(change_pos));
    }
}

int newBaoshiMgr::shop_baoshi(CharData* pc, int pos, json_spirit::Object& robj)
{
    if (pc && pos >= 1 && pos <= 6)
    {
        if (m_goods[pos-1].counts <= 0)
        {
            return HC_ERROR;
        }
        if (pc->m_bag.isFull())
        {
            return HC_ERROR_BAG_FULL;
        }
        if (pc->addGold(-m_goods[pos-1].goldCost) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(pc->m_id,pc->m_ip_address,m_goods[pos-1].goldCost,gold_cost_for_baoshi+m_goods[pos-1].type*100+m_goods[pos-1].level, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(pc,m_goods[pos-1].goldCost,gold_cost_for_buy_baoshi,gold_cost_for_baoshi+m_goods[pos-1].type*100+m_goods[pos-1].level,1);
#endif
        int n = pc->m_bag.addBaoshi(m_goods[pos-1].type,m_goods[pos-1].level,1);
        add_statistics_of_baoshi_get(pc->m_id, pc->m_ip_address, pc->m_union_id, pc->m_server_id, m_goods[pos-1].type,m_goods[pos-1].level, n, baoshi_buy);

        //通知
        json_spirit::Array getlist;
        json_spirit::Object getobj;
        Item baoshi;
        baoshi.type = item_type_baoshi;
        baoshi.id = m_goods[pos-1].type;
        baoshi.fac = m_goods[pos-1].level;
        baoshi.nums = 1;
        baoshi.toObj(getobj);
        getlist.push_back(getobj);
        robj.push_back( Pair("get", getlist) );
        //宝石活动
        Singleton<new_event_mgr>::Instance().addBaoshi(pc->m_id, m_goods[pos-1].level);
        --m_goods[pos-1].counts;
        save_shop(pos);
        pc->NotifyCharData();
        //act统计
        act_to_tencent(pc,act_new_baoshi_by_gold, m_goods[pos-1].type);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int newBaoshiMgr::baoshishopInfo(json_spirit::Object& robj)
{
    time_t t_c = time(NULL);
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&t_c, t);
    bool next_day = false;
    if (t->tm_hour >= 24 || t->tm_hour < 0)
    {
        ;
    }
    else if(t->tm_hour < 10)
    {
        t->tm_hour = 10;
        t->tm_min = 0;
        t->tm_sec = 0;
    }
    else if(t->tm_hour < 15)
    {
        t->tm_hour = 15;
        t->tm_min = 0;
        t->tm_sec = 0;
    }
    else if(t->tm_hour < 20)
    {
        t->tm_hour = 20;
        t->tm_min = 0;
        t->tm_sec = 0;
    }
    else if(t->tm_hour < 24)
    {
        t->tm_hour = 10;
        t->tm_min = 0;
        t->tm_sec = 0;
        next_day = true;
    }
    time_t tmp_time = (int)(mktime(t));
    if (next_day)
        tmp_time += 86400;
    robj.push_back( Pair("shop_refresh_time", tmp_time-t_c) );
    json_spirit::Array list;
    for (int pos = 0; pos < 6; ++pos)
    {
        json_spirit::Object o;
        o.push_back( Pair("pos", pos+1) );
        o.push_back( Pair("type", m_goods[pos].type) );
        o.push_back( Pair("level", m_goods[pos].level) );
        baseNewBaoshi* bbs = getBaoshi(m_goods[pos].type);
        if (bbs)
        {
            o.push_back( Pair("name", bbs->name) );
            o.push_back( Pair("quality", bbs->qualitys[m_goods[pos].level-1]) );
        }
        o.push_back( Pair("counts", m_goods[pos].counts) );
        o.push_back( Pair("goldCost", m_goods[pos].goldCost) );
        o.push_back( Pair("goldCostMax", m_goods[pos].goldCostMax) );
        list.push_back(o);
    }
    robj.push_back( Pair("baoshi_list", list) );
}

//查询宝石仓库
int ProcessQueryBaoshiList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return pc->m_bag.showBagBaoshis(robj);
}

//取下武將寶石
int ProcessRemoveBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int gid = 0, slot = 0;
    READ_INT_FROM_MOBJ(gid, o, "gid");
    READ_INT_FROM_MOBJ(slot, o, "slot");
    return pc->removeBaoshi(gid, slot);
}

//鑲嵌寶石
int ProcessXiangqianBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int bagSlot = 0;
    READ_INT_FROM_MOBJ(bagSlot, o, "bagSlot");
    int gid = 0, slot = 0;
    READ_INT_FROM_MOBJ(gid, o, "gid");
    READ_INT_FROM_MOBJ(slot, o, "slot");
    return pc->xiangruBaoshi(bagSlot, gid, slot);
}

int Baoshilevel_to_score(int level)
{
    int score = 0;
    switch(level)
    {
        case 2:
            score = 20;
            break;
        case 3:
            score = 30;
            break;
        case 4:
            score = 50;
            break;
        case 5:
            score = 80;
            break;
        case 6:
            score = 120;
            break;
        case 7:
            score = 170;
            break;
        case 8:
            score = 240;
            break;
        case 9:
            score = 320;
            break;
        case 10:
            score = 400;
            break;
        case 11:
            score = 500;
            break;
        case 12:
            score = 700;
            break;
        case 13:
            score = 1000;
            break;
        default:
            break;
    }
    return score;
}

//合并寶石
int ProcessCombineBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int type = 0, level = 0, nums = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_INT_FROM_MOBJ(level, o, "level");
    READ_INT_FROM_MOBJ(nums, o, "nums");

    if (nums < 0)
    {
        return HC_ERROR;
    }

    if (level >= MAX_BAOSHI_LEVEL)
    {
        return HC_ERROR;
    }

    //if (nums > 44)
    //{
    //    nums = 44;
    //}
    std::list<boost::shared_ptr<iItem> > list1;
    std::list<boost::shared_ptr<iItem> > list2;

    int level1_count = 0, level2_count = 0;
    for (int i = 1; i <= pc->m_bag.m_size; ++i)
    {            
        //只算在背包里面的
        if (pc->m_bag.m_bagslot[i-1].get())
        {
            iItem* pp = pc->m_bag.m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_baoshi && pp->getSubtype() == type)
            {
                newBaoshi* p = dynamic_cast<newBaoshi*>(pp);
                if (p->level() == level)
                {
                    level1_count += pp->getCount();
                    list1.push_back(pc->m_bag.m_bagslot[i-1]);
                }
                else if (p->level() == (level + 1))
                {
                    level2_count += pp->getCount();
                    list2.push_back(pc->m_bag.m_bagslot[i-1]);
                }
            }
        }
    }

    if (2*nums > level1_count)
    {
        nums = level1_count / 2;
    }

    //cout<<"combine baoshi type:"<<type<<",nums:"<<nums<<","<<level1_count<<"|"<<level2_count<<endl;
    if (nums == 0)
    {
        return HC_ERROR;
    }
    int sub1 = -2*nums;
    int sub = pc->m_bag.addBaoshi(type, level, sub1);
    if (sub != sub1)
    {
        //cout<<"sub:"<<sub<<",sub1:"<<sub1<<endl;
        pc->m_bag.addBaoshi(type, level, -sub);
        return HC_ERROR;
    }
    
    int add = pc->m_bag.addBaoshi(type, level + 1, nums);
    if (add < nums)
    {
        pc->m_bag.addBaoshi(type, level, 2*(nums-add));
        return HC_ERROR_BAG_FULL;
    }

    add_statistics_of_baoshi_cost(pc->m_id, pc->m_ip_address, pc->m_union_id, pc->m_server_id, type, level, 2*add, baoshi_merge);
    add_statistics_of_baoshi_get(pc->m_id, pc->m_ip_address, pc->m_union_id, pc->m_server_id, type, level + 1, add, baoshi_merge);    
    
    json_spirit::Array list;
    baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(type);
    if (bbs)
    {
        json_spirit::Object o;
        o.push_back( Pair("name", bbs->name) );
        o.push_back( Pair("level", level + 1) );
        o.push_back( Pair("count", add) );
        list.push_back(o);
    }
    robj.push_back( Pair("get_list", list) );
    
    //act统计
    act_to_tencent(pc,act_new_baoshi_conbine, level+1);
    //周排行活动
    int score = Baoshilevel_to_score(level + 1) * add;//合成分数
    if (score > 0)
        newRankings::getInstance()->updateEventRankings(pc->m_id,rankings_event_baoshi,score);
    //支线任务
    pc->m_trunk_tasks.updateTask(task_baoshi_combine, add);
    pc->m_trunk_tasks.updateTask(task_baoshi_combine_level, level+1);
    return HC_SUCCESS;
}

//寶石tips
int ProcessQueryBaoshiInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int bid = 0;
    READ_INT_FROM_MOBJ(bid, o, "id");

    if (bid > 0)
    {
        boost::shared_ptr<iItem> bs = Singleton<newBaoshiMgr>::Instance().queryBaoshi(bid);
        if (!bs.get())
        {
            cout<<"can not find this baoshi"<<bid<<endl;
            return HC_ERROR;
        }
        newBaoshi* p = dynamic_cast<newBaoshi*>(bs.get());
        robj.push_back( Pair("name", bs->name()));
        robj.push_back( Pair("quality", p->getQuality()));
        robj.push_back( Pair("level", p->level()));
        robj.push_back( Pair("price", bs->sellPrice()));
        robj.push_back( Pair("spic", bs->getSubtype()));
        json_spirit::Array alist;
        alist.push_back(p->getObj());
        robj.push_back( Pair("list", alist));
    }
    else
    {
        int type = 1, level = 1;
        READ_INT_FROM_MOBJ(type,o,"type");
        READ_INT_FROM_MOBJ(level,o,"level");
        baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(type);
        if (bbs)
        {
            robj.push_back( Pair("name", bbs->name) );
            robj.push_back( Pair("quality", bbs->qualitys[level-1]));
            robj.push_back( Pair("type", type));
            robj.push_back( Pair("level", level));
            robj.push_back( Pair("price", level * 1000));
            robj.push_back( Pair("spic", bbs->type));
            robj.push_back( Pair("memo", bbs->memo));
            json_spirit::Object obj;
            bbs->toObj(level, obj);
            json_spirit::Array alist;
            alist.push_back(obj);
            robj.push_back( Pair("list", alist));
        }
        else
        {
            return HC_ERROR;
        }
    }
    return HC_SUCCESS;
}

//一键合并寶石
int ProcessCombineAllBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_vip < 4)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    int tolevel = 0;
    READ_INT_FROM_MOBJ(tolevel, o, "level");

    if (tolevel > MAX_BAOSHI_LEVEL)
    {
        tolevel = MAX_BAOSHI_LEVEL;
    }

    if (tolevel <= 1)
    {
        return HC_SUCCESS;
    }
    std::map<int,int> list_pre;
    std::map<int,int> list_now;
    //合成前计数
    for (int i = 1; i <= pc->m_bag.m_size; ++i)
    {
        //只算在背包里面的
        if (pc->m_bag.m_bagslot[i-1].get())
        {
            iItem* pp = pc->m_bag.m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_baoshi)
            {
                int type = pp->getSubtype();
                newBaoshi* p = dynamic_cast<newBaoshi*>(pp);
                int level = p->level();
                int tmp_index = (type-1)*14 + level;
                list_pre[tmp_index] += pp->getCount();
            }
        }
    }
    //批量合成
    for (int type = 1; type <= 14; ++type)
    {
        for (int level = 1; level < tolevel; ++level)
        {
            int level1_count = 0, level2_count = 0;
            for (int i = 1; i <= pc->m_bag.m_size; ++i)
            {            
                //只算在背包里面的
                if (pc->m_bag.m_bagslot[i-1].get())
                {
                    iItem* pp = pc->m_bag.m_bagslot[i-1].get();
                    if (pp->getType() == iItem_type_baoshi && pp->getSubtype() == type)
                    {
                        newBaoshi* p = dynamic_cast<newBaoshi*>(pp);
                        if (p->level() == level)
                        {
                            level1_count += pp->getCount();
                        }
                        else if (p->level() == (level + 1))
                        {
                            level2_count += pp->getCount();
                        }
                    }
                }
            }
            int nums = level1_count / 2;
            int sub1 = -2*nums;
            int sub = pc->m_bag.addBaoshi(type, level, sub1);
            if (sub != sub1)
            {
                //cout<<"sub:"<<sub<<",sub1:"<<sub1<<endl;
                pc->m_bag.addBaoshi(type, level, -sub);
                return HC_ERROR;
            }
            int add = pc->m_bag.addBaoshi(type, level + 1, nums);
            //周排行活动
            int score = Baoshilevel_to_score(level + 1) * add;//合成分数
            if (score > 0)
                newRankings::getInstance()->updateEventRankings(pc->m_id,rankings_event_baoshi,score);

            add_statistics_of_baoshi_cost(pc->m_id, pc->m_ip_address, pc->m_union_id, pc->m_server_id, type, level, 2*add, baoshi_merge);
            add_statistics_of_baoshi_get(pc->m_id, pc->m_ip_address, pc->m_union_id, pc->m_server_id, type, level + 1, add, baoshi_merge);    
            if (add < nums)
            {
                pc->m_bag.addBaoshi(type, level, 2*(nums-add));
                return HC_ERROR_BAG_FULL;
            }

            //支线任务
            pc->m_trunk_tasks.updateTask(task_baoshi_combine, add);
            pc->m_trunk_tasks.updateTask(task_baoshi_combine_level, level+1);
        }
    }
    //合成后计数
    for (int i = 1; i <= pc->m_bag.m_size; ++i)
    {
        //只算在背包里面的
        if (pc->m_bag.m_bagslot[i-1].get())
        {
            iItem* pp = pc->m_bag.m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_baoshi)
            {
                int type = pp->getSubtype();
                newBaoshi* p = dynamic_cast<newBaoshi*>(pp);
                int level = p->level();
                int tmp_index = (type-1)*14 + level;
                list_now[tmp_index] += pp->getCount();
            }
        }
    }
    //统计通知
    json_spirit::Array list;
    for (int type = 1; type <= 14; ++type)
    {
        for (int level = 1; level < tolevel; ++level)
        {
            int tmp_index = (type-1)*14 + level;
            if (list_now[tmp_index] > list_pre[tmp_index])
            {
                baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(type);
                if (bbs)
                {
                    json_spirit::Object o;
                    o.push_back( Pair("name", bbs->name) );
                    o.push_back( Pair("level", level) );
                    o.push_back( Pair("count", list_now[tmp_index] - list_pre[tmp_index]) );
                    list.push_back(o);
                }
            }
        }
    }
    robj.push_back( Pair("get_list", list) );
    return HC_SUCCESS;
}

//宝石转化
int ProcessChangeBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    if (pc->m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    int type = 0, level = 0, nums = 0, to_type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_INT_FROM_MOBJ(level, o, "level");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    READ_INT_FROM_MOBJ(to_type, o, "to_type");

    if (level <= 1)
    {
        return HC_ERROR;
    }
    baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(to_type);
    if (!bbs)
    {
        return HC_ERROR;
    }
    int level1_count = 0;
    for (int i = 1; i <= pc->m_bag.m_size; ++i)
    {
        //只算在背包里面的
        if (pc->m_bag.m_bagslot[i-1].get())
        {
            iItem* pp = pc->m_bag.m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_baoshi && pp->getSubtype() == type)
            {
                newBaoshi* p = dynamic_cast<newBaoshi*>(pp);
                if (p->level() == level)
                {
                    level1_count += pp->getCount();
                }
            }
        }
    }

    if (nums > level1_count)
    {
        nums = level1_count;
    }
    if (nums > MAX_BAOSHI_COUNT)
    {
        nums = MAX_BAOSHI_COUNT;
    }
    if (nums <= 0)
    {
        return HC_ERROR;
    }
    int silver_cost = (level - 1) * nums * 10000;
    if (pc->addSilver(-silver_cost) < 0)
        return HC_ERROR_NOT_ENOUGH_SILVER;
    //银币消耗统计
    add_statistics_of_silver_cost(pc->m_id, pc->m_ip_address, silver_cost, silver_cost_for_baoshi_change, pc->m_union_id, pc->m_server_id);
    int sub1 = -nums;
    int sub = pc->m_bag.addBaoshi(type, level, sub1);
    if (sub != sub1)
    {
        pc->m_bag.addBaoshi(type, level, -sub);
        return HC_ERROR;
    }
    int add = pc->m_bag.addBaoshi(to_type, level - 1, nums);
    add_statistics_of_baoshi_cost(pc->m_id, pc->m_ip_address, pc->m_union_id, pc->m_server_id, type, level, add, baoshi_convert);
    add_statistics_of_baoshi_get(pc->m_id, pc->m_ip_address, pc->m_union_id, pc->m_server_id, to_type, level, add, baoshi_convert);    
    json_spirit::Array list;
    //baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(to_type);
    if (bbs)
    {
        json_spirit::Object o;
        o.push_back( Pair("name", bbs->name) );
        o.push_back( Pair("level", level - 1) );
        o.push_back( Pair("count", add) );
        list.push_back(o);
    }
    robj.push_back( Pair("get_list", list) );
    //act统计
    act_to_tencent(pc,act_new_baoshi_change, level);
    //周排行活动
    int score = 30 * add;//转换30分
    if (score > 0)
        newRankings::getInstance()->updateEventRankings(pc->m_id,rankings_event_baoshi,score);
    if (add < nums)
    {
        pc->m_bag.addBaoshi(type, level, (nums-add));
        pc->addSilver((level - 1) * (nums-add) * 10000);
        return HC_ERROR_BAG_FULL;
    }

    //支线任务
    pc->m_trunk_tasks.updateTask(task_baoshi_convert, add);
    return HC_SUCCESS;
}

//免费领玉石
int ProcessGetYushi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int cnt = pc->queryExtraData(char_data_type_daily, char_data_get_yushi);
    int cd_time = pc->queryExtraData(char_data_type_daily, char_data_yushi_time_cd);
    time_t t_now = time(NULL);
    if (iFreeYushi > cnt && t_now > cd_time)
    {
        int get_cnt = 400+pc->m_level*20;
        pc->addTreasure(treasure_type_yushi, get_cnt);
        pc->setExtraData(char_data_type_daily, char_data_get_yushi, ++cnt);
        pc->setExtraData(char_data_type_daily, char_data_yushi_time_cd, t_now+1800);
        //通知
        json_spirit::Array getlist;
        json_spirit::Object getobj;
        Item yushi;
        yushi.type = item_type_treasure;
        yushi.id = treasure_type_yushi;
        yushi.nums = get_cnt;
        yushi.toObj(getobj);
        getlist.push_back(getobj);
        robj.push_back( Pair("get", getlist) );
        //日常任务
        dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_get_yushi);
        //act统计
        act_to_tencent(pc,act_new_yushi_get);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//购买宝石
int ProcessBuyShopBaoshi(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    return Singleton<newBaoshiMgr>::Instance().shop_baoshi(pc, pos, robj);
}

//宝石商店
int ProcessBaoshiShop(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (iFreeYushi > pc->queryExtraData(char_data_type_daily, char_data_get_yushi))
    {
        robj.push_back( Pair("get_cnt", iFreeYushi - pc->queryExtraData(char_data_type_daily, char_data_get_yushi)) );
        time_t time_now = time(NULL);
        if (pc->queryExtraData(char_data_type_daily, char_data_yushi_time_cd) > time_now)
        {
            robj.push_back( Pair("leftTime", pc->queryExtraData(char_data_type_daily, char_data_yushi_time_cd) - time_now) );
        }
    }
    Singleton<newBaoshiMgr>::Instance().baoshishopInfo(robj);
    return HC_SUCCESS;
}

