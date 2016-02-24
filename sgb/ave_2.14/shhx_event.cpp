
#include "shhx_event.h"
#include "net.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_errcode.h"

extern Database& GetDb();
void InsertSaveDb(const std::string& sql);

class Combat;
int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

//领宝箱
int open_box(CharData* pc, int level, std::string& rewardString)
{
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    std::list<Item> items;
    
    int rand = my_random(1, 100);
    int type = (rand <= 60) ? 1 : (rand <= 90 ? 2 : 3);

    switch (level)
    {
        case 1:
            /*
            宝箱种类    奖励1（60%概率）    奖励2（30%概率）    奖励3（10%概率）
            一级宝箱    等级×600银币（1-4倍）    矿石×地图号（1-4倍）    声望10（1-4倍）
            */
            switch (type)
            {
                case 1:
                    {
                        Item item;
                        item.type = item_type_silver;
                        item.fac = my_random(1, 4);
                        item.nums = pc->m_level * 600 * item.fac;
                        items.push_back(item);
                    }
                    break;
                case 3:
                default:
                    {
                        Item item;
                        item.type = item_type_prestige;
                        item.fac = my_random(1, 4);
                        item.nums = 10 * item.fac;
                        items.push_back(item);
                    }
                    break;
            }
            break;
        case 2:
            /*
            二级宝箱    等级×900银币（1-4倍）    矿石×地图号（2-6倍）    声望15（1-4倍）
            */
            switch (type)
            {
                case 1:
                    {
                        Item item;
                        item.type = item_type_silver;
                        item.fac = my_random(1, 4);
                        item.nums = pc->m_level * 900 * item.fac;
                        items.push_back(item);
                    }
                    break;
                case 3:
                default:
                    {
                        Item item;
                        item.type = item_type_prestige;
                        item.fac = my_random(1, 4);
                        item.nums = 15 * item.fac;
                        items.push_back(item);
                    }
                    break;
            }
            break;
        case 3:
            /*
            三级宝箱    等级×1200银币（1-6倍）    20玉石×等级（1-4倍）    声望20（1-5倍）
            */
            switch (type)
            {
                case 1:
                    {
                        Item item;
                        item.type = item_type_silver;
                        item.fac = my_random(1, 6);
                        item.nums = pc->m_level * 1200 * item.fac;
                        items.push_back(item);
                    }
                    break;
                case 2:
                    {
                        Item item;
                        item.type = item_type_treasure;
                        item.id = treasure_type_yushi;
                        item.fac = my_random(1, 4);
                        item.nums = 20 * pc->m_level * item.fac;
                        items.push_back(item);
                    }
                    break;
                case 3:
                default:
                    {
                        Item item;
                        item.type = item_type_prestige;
                        item.fac = my_random(1, 5);
                        item.nums = 20 * item.fac;
                        items.push_back(item);
                    }
                    break;
            }
            break;
        case 4:
            /*
            四级宝箱    等级×1500银币（1-8倍）    30玉石×等级（1-5倍）    声望25（1-6倍）
            */
            switch (type)
            {
                case 1:
                    {
                        Item item;
                        item.type = item_type_silver;
                        item.fac = my_random(1, 8);
                        item.nums = pc->m_level * 1500 * item.fac;
                        items.push_back(item);
                    }
                    break;
                case 2:
                    {
                        Item item;
                        item.type = item_type_treasure;
                        item.id = treasure_type_yushi;
                        item.fac = my_random(1, 5);
                        item.nums = 30* pc->m_level * item.fac;
                        items.push_back(item);
                    }
                    break;
                case 3:
                default:
                    {
                        Item item;
                        item.type = item_type_prestige;
                        item.fac = my_random(1, 6);
                        item.nums = 20 * item.fac;
                        items.push_back(item);
                    }
                    break;
            }
            break;
        case 5:
        default:
            /*
            五级宝箱    等级×1800银币（1-8倍）    40玉石×等级（1-6倍）    声望30（1-6倍）
            */
            switch (type)
            {
                case 1:
                    {
                        Item item;
                        item.type = item_type_silver;
                        item.fac = my_random(1, 4);
                        item.nums = pc->m_level * 1800 * item.fac;
                        items.push_back(item);
                    }
                    break;
                case 2:
                    {
                        Item item;
                        item.type = item_type_treasure;
                        item.id = treasure_type_yushi;
                        item.fac = my_random(1, 6);
                        item.nums = 40* pc->m_level * item.fac;
                        items.push_back(item);
                    }
                    break;
                case 3:
                default:
                    {
                        Item item;
                        item.type = item_type_prestige;
                        item.fac = my_random(1, 6);
                        item.nums = 30 * item.fac;
                        items.push_back(item);
                    }
                    break;
            }
            break;
    }
    giveLoots(pc, items, pc->m_area, pc->m_level, 1, NULL, NULL, true, 0);
    rewardString = "";
    for (std::list<Item>::iterator it = items.begin(); it != items.end(); ++it)
    {
        if (rewardString != "")
        {
            rewardString += ",";
        }
        rewardString += it->toString();
    }
    return HC_SUCCESS;
}

shhx_cost_event* shhx_cost_event::m_handle = NULL;

shhx_cost_event* shhx_cost_event::getInstance()
{
    if (NULL == m_handle)
    {
        time_t time_start = time(NULL);
        cout<<"shhx_cost_event::getInstance()..."<<endl;
        m_handle = new shhx_cost_event();
        m_handle->load();
        cout<<"shhx_cost_event::dailyTaskMgr() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

void shhx_cost_event::load()        //加载活动
{
    m_enable = false;
    m_start_time = 0;
    m_end_time = 0;

    //从数据库加载
}

int shhx_cost_event::update_cost_event(int cid, int gold_cost)
{
    if (!m_enable || gold_cost < 0)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_cost_event> cc = getChar(cid);
    if (!cc.get())
    {
        return HC_ERROR;
    }
    cc->_total_cost += gold_cost;
    while (cc->_total_cost >= cc->_next)
    {
        ++cc->_can_get;
        ++cc->_can_gets[cc->_next_level-1];
        cc->_next_level = shhx_cost_event::getInstance()->get_next(cc->_next);
    }
    //存盘
    cc->save();
    return HC_SUCCESS;
}

int shhx_cost_event::openBox(int cid, std::string& rewardString)
{
    boost::shared_ptr<char_cost_event> cc = getChar(cid);
    if (!cc.get())
    {
        return HC_ERROR;
    }
    if (cc->_can_get <= 0)
    {
        return HC_ERROR;
    }
    for (int i = 0; i < 5; ++i)
    {
        if (cc->_can_gets[i])
        {
            --cc->_can_gets[i];
            //存盘
            cc->save();
            return open_box(cc->_cdata.get(), i + 1, rewardString);
        }
    }
    return HC_ERROR;
}

boost::shared_ptr<char_cost_event> shhx_cost_event::getChar(int cid)
{
    std::map<int, boost::shared_ptr<char_cost_event> >::iterator it = m_char_datas.find(cid);
    if (it != m_char_datas.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<char_cost_event> cc(new char_cost_event);
        cc->_cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cc->_cdata.get())
        {
            cc.reset();
            return cc;
        }
        cc->_cid = cid;
        Query q(GetDb());
        q.get_result("select total_cost,can1,can2,can3,can4,can5 from char_cost_event where cid=" + LEX_CAST_STR(cid));
        if (q.fetch_row())
        {
            cc->_total_cost = q.getval();
            cc->_can_gets[0] = q.getval();
            cc->_can_gets[1] = q.getval();
            cc->_can_gets[2] = q.getval();
            cc->_can_gets[3] = q.getval();
            cc->_can_gets[4] = q.getval();
            q.free_result();
        }
        else
        {
            q.free_result();
            cc->_total_cost = 0;
            cc->_can_gets[0] = 0;
            cc->_can_gets[1] = 0;
            cc->_can_gets[2] = 0;
            cc->_can_gets[3] = 0;
            cc->_can_gets[4] = 0;
        }
        cc->_can_get = 0;
        for (int i = 0; i < 5; ++i)
        {
            cc->_can_get += cc->_can_gets[i];
        }
        cc->_next = cc->_total_cost;
        cc->_next_level = shhx_cost_event::getInstance()->get_next(cc->_next);
        m_char_datas[cid] = cc;
        return cc;
    }
}

//累计消耗金币和宝箱的关系
int shhx_cost_event::get_next(int& gold)
{
    if (gold < 50)
    {
        //10,20,30,50金币可得1级宝箱
        static int ar[] = {10, 20, 30, 50};
        for (int i = 0; i < 4; ++i)
        {
            if (gold < ar[i])
            {
                gold = ar[i];                
            }
        }
        return 1;
    }
    else if (gold < 200)
    {
        //80,120,150,200金币可得2级宝箱
        static int ar[] = {80, 120, 150, 200};
        for (int i = 0; i < 4; ++i)
        {
            if (gold < ar[i])
            {
                gold = ar[i];                
            }
        }
        return 2;
    }
    else
    {
        //每多100金得宝箱，1000以下2级，2000以下3级，5000以下4级，5000以上5级
        gold = gold + 100 - gold % 100;
        if (gold < 1000)
        {
            return 2;
        }
        else if (gold < 2000)
        {
            return 3;
        }
        else if (gold < 5000)
        {
            return 4;
        }
        return 5;
    }
}

int shhx_cost_event::leftSecs()
{
    time_t timenow = time(NULL);
    if (timenow < m_end_time)
    {
        return (m_end_time - timenow);
    }
    else
    {
        return 0;
    }
}

shhx_generl_upgrade_event* shhx_generl_upgrade_event::m_handle = NULL;

shhx_generl_upgrade_event* shhx_generl_upgrade_event::getInstance()
{
    if (NULL == m_handle)
    {
        time_t time_start = time(NULL);
        cout<<"shhx_generl_upgrade_event::getInstance()..."<<endl;
        m_handle = new shhx_generl_upgrade_event();
        m_handle->load();
        cout<<"shhx_generl_upgrade_event::shhx_generl_upgrade_event() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

void shhx_generl_upgrade_event::load()        //加载活动
{
    m_enable = false;
    m_start_time = 0;
    m_end_time = 0;

    //从数据库加载
}

int shhx_generl_upgrade_event::add_score(int cid, int score)
{
    if (!m_enable)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_general_upgrade_event> cc = getChar(cid);
    if (!cc.get())
    {
        return HC_ERROR;
    }
    cc->_total_score += score;
    cc->_can_get = cc->_total_score / 10;
    //存盘
    cc->save();
    return HC_SUCCESS;
}

int shhx_generl_upgrade_event::openBox(int cid, std::string& rewardString)
{
    boost::shared_ptr<char_general_upgrade_event> cc = getChar(cid);
    if (!cc.get())
    {
        return HC_ERROR;
    }
    if (cc->_can_get <= cc->_geted)
    {
        return HC_ERROR;
    }
    ++cc->_geted;
    //存盘
    cc->save();
    return open_box(cc->_cdata.get(), 2, rewardString);
}

boost::shared_ptr<char_general_upgrade_event> shhx_generl_upgrade_event::getChar(int cid)
{
    std::map<int, boost::shared_ptr<char_general_upgrade_event> >::iterator it = m_char_datas.find(cid);
    if (it != m_char_datas.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<char_general_upgrade_event> cc(new char_general_upgrade_event);
        cc->_cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cc->_cdata.get())
        {
            cc.reset();
            return cc;
        }
        cc->_cid = cid;
        Query q(GetDb());
        q.get_result("select total_score,getted from char_general_upgrade_event where cid=" + LEX_CAST_STR(cid));
        if (q.fetch_row())
        {
            cc->_total_score = q.getval();
            cc->_geted = q.getval();
            q.free_result();
        }
        else
        {
            q.free_result();
            cc->_total_score = 0;
            cc->_geted = 0;
        }
        cc->_can_get = cc->_total_score / 10;
        m_char_datas[cid] = cc;
        return cc;
    }
}

int shhx_generl_upgrade_event::leftSecs()
{
    time_t timenow = time(NULL);
    if (timenow < m_end_time)
    {
        return (m_end_time - timenow);
    }
    else
    {
        return 0;
    }
}

void char_general_upgrade_event::save()
{
    InsertSaveDb("replace into char_general_upgrade_event (cid,total_score,getted) values ("
            + LEX_CAST_STR(_cid) + ","
            + LEX_CAST_STR(_total_score) + ","
            + LEX_CAST_STR(_geted) + ")");
}

void char_cost_event::save()
{
    InsertSaveDb("replace into char_cost_event (cid,total_cost,can1,can2,can3,can4,can5) values ("
            + LEX_CAST_STR(_cid) + ","
            + LEX_CAST_STR(_total_cost) + ","
            + LEX_CAST_STR(_can_gets[0]) + ","
            + LEX_CAST_STR(_can_gets[1]) + ","
            + LEX_CAST_STR(_can_gets[2]) + ","
            + LEX_CAST_STR(_can_gets[3]) + ","
            + LEX_CAST_STR(_can_gets[4]) + ")");
}

using namespace net;
using namespace json_spirit;

//金币消费活动界面
int ProcessGetCostEventInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int leftSecs = shhx_cost_event::getInstance()->leftSecs();
    if (leftSecs <= 0)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_cost_event> cc = shhx_cost_event::getInstance()->getChar(pc->m_id);
    if (!cc.get())
    {
        return HC_ERROR;
    }
    Object info;
    info.push_back( Pair("current", cc->_total_cost));
    info.push_back( Pair("next", cc->_next));
    info.push_back( Pair("canGet", cc->_can_get));
    info.push_back( Pair("leftSecs", leftSecs));
    robj.push_back( Pair("info", info));
    return HC_SUCCESS;
}

//金币消费活动奖励
int ProcessGetCostEventReward(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int leftSecs = shhx_cost_event::getInstance()->leftSecs();
    if (leftSecs <= 0)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_cost_event> cc = shhx_cost_event::getInstance()->getChar(pc->m_id);
    if (!cc.get())
    {
        return HC_ERROR;
    }
    std::string strGet = "";
    ret = shhx_cost_event::getInstance()->openBox(pc->m_id, strGet);
    if (ret == HC_SUCCESS)
    {
        robj.push_back( Pair("get", strGet) );        
    }
    return ret;
}

//武将升级活动界面
int ProcessGetGeneralUpgradeEventInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int leftSecs = shhx_generl_upgrade_event::getInstance()->leftSecs();
    if (leftSecs <= 0)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_general_upgrade_event> cc = shhx_generl_upgrade_event::getInstance()->getChar(pc->m_id);
    if (!cc.get())
    {
        return HC_ERROR;
    }
    Object info;
    info.push_back( Pair("current", cc->_total_score));
    info.push_back( Pair("next", cc->_total_score - cc->_total_score % 10 + 10));
    info.push_back( Pair("canGet", cc->_can_get - cc->_geted));
    info.push_back( Pair("leftSecs", leftSecs));
    robj.push_back( Pair("info", info));
    return HC_SUCCESS;
}

//武将升级活动奖励
int ProcessGetGeneralUpgradeEventReward(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int leftSecs = shhx_generl_upgrade_event::getInstance()->leftSecs();
    if (leftSecs <= 0)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_general_upgrade_event> cc = shhx_generl_upgrade_event::getInstance()->getChar(pc->m_id);
    if (!cc.get())
    {
        return HC_ERROR;
    }
    std::string strGet = "";
    ret = shhx_generl_upgrade_event::getInstance()->openBox(pc->m_id, strGet);
    if (ret == HC_SUCCESS)
    {
        robj.push_back( Pair("get", strGet) );        
    }
    return ret;
}

