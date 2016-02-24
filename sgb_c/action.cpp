
#include "action.h"
#include "singleton.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "errcode_def.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "new_combat.hpp"
#include "loot.h"
#include "SaveDb.h"
#include "task.h"
#include "goal.h"
#include "spls_timer.h"

int getSessionChar(net::session_ptr& psession, boost::shared_ptr<CharData>& cdata);
Database& GetDb();
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

//查询签到信息
int ProcessQuerySignInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_sign_data* e = Singleton<actionMgr>::Instance().getCharSignData(cdata->m_id);
    if (e)
    {
        Singleton<actionMgr>::Instance().getSignInfo(*e, robj);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//签到
int ProcessSign(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_sign_data* e = Singleton<actionMgr>::Instance().getCharSignData(cdata->m_id);
    if (e)
    {
        int day = 1;
        READ_INT_FROM_MOBJ(day,o,"day");
        return e->doSign(day, robj);
    }
}

//请求签到商店
int ProcessQuerySignShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    int nums_per_page = 8;
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

    std::vector<baseSignShopGoods> m_goods = Singleton<actionMgr>::Instance().getSignShopGoods();
    json_spirit::Array list;
    for (int pos = 1; pos <= m_goods.size(); ++pos)
    {
        ++cur_nums;
        if (cur_nums >= first_nums && cur_nums <= last_nums)
        {
            json_spirit::Object shop;
            shop.push_back( Pair("pos", pos) );
            shop.push_back( Pair("need_score", m_goods[pos-1].needscore) );
            json_spirit::Object get;
            m_goods[pos-1].reward.toObj(get);
            shop.push_back( Pair("get", get) );
            list.push_back(shop);
        }
    }
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    robj.push_back( Pair("list", list));
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    int score = cdata->queryExtraData(char_data_type_normal, char_data_normal_sign_score);
    robj.push_back( Pair("score", score) );
    return HC_SUCCESS;
}

//购买签到商店商品
int ProcessBuySignShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int pos = 0, nums = 0;
    READ_INT_FROM_MOBJ(pos,o,"pos");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    if (nums < 1)
        nums = 1;
    std::vector<baseSignShopGoods> m_goods = Singleton<actionMgr>::Instance().getSignShopGoods();
    if (pos < 1 || pos > m_goods.size())
    {
        return HC_ERROR;
    }
    int need = m_goods[pos-1].needscore * nums;
    if (need < 0)
    {
        return HC_ERROR;
    }
    int score = cdata->queryExtraData(char_data_type_normal, char_data_normal_sign_score);
    if (score < need)
    {
        return HC_ERROR_NOT_ENOUGH_SCORE;
    }
    std::list<Item> items;
    Item tmp(m_goods[pos-1].reward.type, m_goods[pos-1].reward.id, m_goods[pos-1].reward.nums * nums, m_goods[pos-1].reward.extra);
    items.push_back(tmp);
    if (!cdata->m_bag.hasSlot(itemlistNeedBagSlot(items)))
    {
        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
    }
    cdata->setExtraData(char_data_type_normal, char_data_normal_sign_score, (score-need));
    giveLoots(cdata.get(),items,NULL,&robj,true,loot_sign_action);
    return HC_SUCCESS;
}

//查询在线礼包信息
int ProcessQueryOnlineLibaoInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_online_libao_data* e = Singleton<actionMgr>::Instance().getCharOnlineLibaoData(cdata->m_id);
    if (e)
    {
        Singleton<actionMgr>::Instance().getOnlineLibaoInfo(*e, robj);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//在线礼包状态更新
int ProcessOnlineLibaoUpdate(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    Singleton<actionMgr>::Instance().updateOnlineLibaoAction(cid);
    return HC_SUCCESS;
}

//查询限时活动列表
int ProcessQueryTimeLimitActionList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    Singleton<actionMgr>::Instance().getTimeLimitActionList(cdata.get(), robj);
    return HC_SUCCESS;
}

//查询七日活动信息
int ProcessQuerySevenInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_seven_data* e = Singleton<actionMgr>::Instance().getCharSevenData(cdata->m_id);
    if (e)
    {
        Singleton<actionMgr>::Instance().getSevenAction(cdata.get(), *e, robj);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//查询强化活动信息
int ProcessQueryEquiptLevelInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_equipt_level_data* e = Singleton<actionMgr>::Instance().getCharEquiptLevelData(cdata->m_id);
    if (e)
    {
        Singleton<actionMgr>::Instance().getEquiptLevelAction(cdata.get(), *e, robj);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//查询升星活动信息
int ProcessQueryHeroStarInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_hero_star_data* e = Singleton<actionMgr>::Instance().getCharHeroStarData(cdata->m_id);
    if (e)
    {
        Singleton<actionMgr>::Instance().getHeroStarAction(cdata.get(), *e, robj);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//查询撕包活动信息
int ProcessQueryHeroPackInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    //每天第一闪
    int first_view = cdata->queryExtraData(char_data_type_daily, char_data_daily_view_hero_pack_action);
    if (0 == first_view)
    {
        cdata->setExtraData(char_data_type_daily, char_data_daily_view_hero_pack_action, 1);
        Singleton<actionMgr>::Instance().notifyTimeLimitActionState(cdata.get());
    }
    Singleton<actionMgr>::Instance().getHeroPackAction(cdata.get(),robj);
    return HC_SUCCESS;
}

//查询英雄活动信息
int ProcessQueryHeroActionInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_hero_data* e = Singleton<actionMgr>::Instance().getCharHeroData(cdata->m_id);
    if (e)
    {
        Singleton<actionMgr>::Instance().getHeroAction(cdata.get(), *e, robj);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//查询冲关活动信息
int ProcessQueryStrongholdActionInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    char_stronghold_data* e = Singleton<actionMgr>::Instance().getCharStrongholdData(cdata->m_id);
    if (e)
    {
        Singleton<actionMgr>::Instance().getStrongholdAction(cdata.get(), *e, robj);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

bool sign_action::in_list(int days)
{
    return m_rewards.find(days) != m_rewards.end();
}

char_sign_data::char_sign_data(sign_action& h, int cid)
:m_handle(h)
,m_cid(cid)
{
    m_sign_time = 0;
    m_canGet = 0;
}

void char_sign_data::save()
{
    std::string sql = "update char_signs set ctime=" + LEX_CAST_STR(m_generate_time) + ",sign_data='";
    int scount = 0;
    for (std::map<int, int>::iterator it = m_sign_data.begin(); it != m_sign_data.end(); ++it)
    {
        if (it->second == 1)
        {
            if (scount)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++scount;
        }
    }
    sql += "',sign_time=" + LEX_CAST_STR(m_sign_time)
        + ",reward_getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_getted.begin(); it != m_getted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

int char_sign_data::getAwards(CharData& cdata, int days, json_spirit::Object& robj)
{
    if (m_getted[days] == 1)
    {
        if (m_handle.m_rewards.find(days) != m_handle.m_rewards.end())
        {
            baseLibao* lb = m_handle.m_rewards[days].get();
            if (lb)
            {
                //给东西
                if (!cdata.m_bag.hasSlot(lb->need_slot_num))
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, NULL, &robj, true, loot_sign_action);
            }
        }

        m_getted[days] = 2;
        m_canGet = getSignAction();
        //通知图标
        Singleton<actionMgr>::Instance().notifySignState(&cdata);
        save();
    }
    return HC_SUCCESS;
}

int char_sign_data::doSign(int day, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        ERR();
        return 0;
    }
    if (day > m_tm_now.tm_mday)
    {
        return HC_ERROR;
    }
    if (day < 0 || day > 31)
    {
        return HC_ERROR;
    }
    if (m_sign_data[day])
    {
        return HC_SUCCESS;
    }
    if (day == m_tm_now.tm_mday)
    {
        //今天签到了?
        if (m_sign_time > 0)
        {
            return HC_SUCCESS;
        }
        m_sign_time = m_time_now;
        m_sign_data[m_tm_now.tm_mday] = 1;
        int score = cdata->queryExtraData(char_data_type_normal, char_data_normal_sign_score);
        int tmp = score + iActionSignScore;
        if (tmp > iActionSignScoreMax)
        {
            tmp = iActionSignScoreMax;
        }
        cdata->setExtraData(char_data_type_normal, char_data_normal_sign_score, tmp);
    }
    else
    {
        //开服时间之前的日期无法签到
        time_t open = GeneralDataMgr::getInstance()->getServerOpenTime();
        struct tm open_tm;
        localtime_r(&open, &open_tm);
        //当前月正好是开服的月份
        if (m_tm_now.tm_mon == open_tm.tm_mon
            && m_tm_now.tm_year == open_tm.tm_year)
        {
            if (day < open_tm.tm_mday)
            {
                return HC_ERROR;
            }
        }
        if (cdata->subGold(iActionSignGold, gold_cost_sign) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        m_sign_data[day] = 1;
    }
    ++m_total_sign;

    //新的可以领取
    if (m_handle.in_list(m_total_sign))
    {
        m_getted[m_total_sign] = 1;
        m_canGet = 1;
    }
    else
    {
        m_canGet = getSignAction();
    }
    //通知图标
    Singleton<actionMgr>::Instance().notifySignState(cdata.get());
    save();
    //自动发签到单天奖励
    if (m_handle.m_single_rewards.find(day) != m_handle.m_single_rewards.end())
    {
        baseLibao* lb = m_handle.m_single_rewards[day].get();
        if (lb)
        {
            //给东西
            std::list<Item> items = lb->m_list;
            giveLoots(cdata.get(), items, NULL, &robj, true, loot_sign_action);
        }
    }
    Singleton<goalMgr>::Instance().updateTask(cdata->m_id, GOAL_TYPE_SIGN, 1);
    return HC_SUCCESS;
}

void char_sign_data::checkReset()
{
    m_time_now = time(NULL);
    localtime_r(&m_time_now, &m_tm_now);
    //月份变化
    if (m_tm_now.tm_mon != m_tm_generate.tm_mon
        || m_tm_now.tm_year != m_tm_generate.tm_year)
    {
        //全部重置
        m_getted.clear();
        m_sign_data.clear();
        m_tm_generate = m_tm_now;
        m_generate_time = m_time_now;

        m_sign_time = 0;
        m_total_sign = 0;

        m_canGet = 1;

        save();
    }
    else if (m_tm_now.tm_mday != m_tm_generate.tm_mday)
    {
        //在线礼包重置
        m_tm_generate = m_tm_now;
        m_generate_time = m_time_now;
        m_sign_time = 0;
        m_canGet = 1;
        save();
    }
    else
    {
        ;
    }
}

int char_sign_data::getSignAction()
{
    //未签到
    if (m_sign_time == 0)
    {
        return 1;
    }
    for (std::map<int, int>::iterator it = m_getted.begin(); it != m_getted.end(); ++it)
    {
        if (it->second == 1)
        {
            return 1;
        }
    }
    return 0;
}

void char_online_libao::save()
{
    InsertSaveDb("update char_online_libao set state="+ LEX_CAST_STR(state)
        + " where cid="+ LEX_CAST_STR(cid) + " and libaoid="+ LEX_CAST_STR(id));
}

void char_online_libao_data::save_get(int id)
{
    InsertSaveDb("replace into char_online_libao_get (cid,libaoid,itemType,itemId,counts,extra) values ("
        + LEX_CAST_STR(m_cid) + ","
        + LEX_CAST_STR(id) + ","
        + LEX_CAST_STR(m_get_item[id-1].type) + ","
        + LEX_CAST_STR(m_get_item[id-1].id) + ","
        + LEX_CAST_STR(m_get_item[id-1].nums) + ","
        + LEX_CAST_STR(m_get_item[id-1].extra) + ")");
}

bool char_online_libao_data::del_timer()
{
    bool delete_success = true;
    if (!_uuid.is_nil())
    {
        delete_success = splsTimerMgr::getInstance()->delTimer(_uuid);
        _uuid = boost::uuids::nil_uuid();
    }
    return delete_success;
}

int char_online_libao_data::getAwards(CharData& cdata, json_spirit::Object& robj)
{
    std::vector<char_online_libao>::iterator it = m_online_libaos.begin();
    while(it != m_online_libaos.end())
    {
        if (it->state == 1)
        {
            //发奖
            json_spirit::Object obj;
            std::list<Item> items;
            Singleton<lootMgr>::Instance().getOnlineLoots(it->id, items);
            //给东西
            giveLoots(&cdata, items, NULL, &obj, true, loot_online_action);
            robj.push_back( Pair("online"+LEX_CAST_STR(it->id), obj) );
            it->state = 2;
            std::list<Item>::iterator it_i = items.begin();
            if (it_i != items.end())
            {
                Item tmp;
                tmp.id = it_i->id;
                tmp.nums = it_i->nums;
                tmp.type = it_i->type;
                tmp.extra = it_i->extra;
                m_get_item.push_back(tmp);
                save_get(it->id);
            }
            it->save();
        }
        ++it;
    }
    //通知图标
    cdata.updateTopButton(top_button_online, getOnlineLibaoState());
    return HC_SUCCESS;
}

//更新在线礼包状态
void char_online_libao_data::updateState(int type)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        ERR();
        return;
    }
    int t_online = cdata->getTodayOnlineTime();
    std::vector<char_online_libao>::iterator it = m_online_libaos.begin();
    while(it != m_online_libaos.end())
    {
        if (it->state == 0)
        {
            if (t_online >= it->total_online_time)
            {
                it->state = 1;
                it->save();
                //通知图标
                cdata->updateTopButton(top_button_online, 1);
            }
            else//未完成
            {
                int leftsecond = it->total_online_time - t_online;
                if (type == 1)//初始化需要设置定时器
                {
                    json_spirit::mObject mobj;
                    mobj["cmd"] = "onlineLibaoUpdate";
                    mobj["cid"] = m_cid;
                    boost::shared_ptr<splsTimer> tmsg;
                    tmsg.reset(new splsTimer(leftsecond, 1,mobj,1));
                    _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
                }
                else if(type == 2)//玩家中途请求不修改定时器
                {
                    ;
                }
                else if(type == 3)//下线删除定时器
                {
                    del_timer();
                }
                break;
            }
        }
        ++it;
    }
}

int char_online_libao_data::getOnlineLibaoState()
{
    std::vector<char_online_libao>::iterator it = m_online_libaos.begin();
    while(it != m_online_libaos.end())
    {
        if (it->state == 1)
            return 1;
        ++it;
    }
    return 0;
}

int char_online_libao_data::getOnlineLibaoCnt()
{
    int cnt = 0;
    std::vector<char_online_libao>::iterator it = m_online_libaos.begin();
    while(it != m_online_libaos.end())
    {
        if (it->state != 2)
            ++cnt;
        ++it;
    }
    return cnt;
}

char_seven_data::char_seven_data(seven_action& h, int cid)
:m_handle(h)
{
    m_cid = cid;
    m_cur_day = 1;
    m_canGet = 0;
}

int char_seven_data::getAwards(CharData& cdata, int days, json_spirit::Object& robj)
{
    if (m_geted[days] == 1)
    {
        if (m_handle.m_rewards.find(days) != m_handle.m_rewards.end())
        {
            baseLibao* lb = m_handle.m_rewards[days].get();
            if (lb)
            {
                //给东西
                if (!cdata.m_bag.hasSlot(lb->need_slot_num))
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, NULL, &robj, true, loot_limit_action);
            }
        }
        m_geted[days] = 2;
        m_canGet = 0;
        std::map<int, int>::iterator it = m_geted.begin();
        while(it != m_geted.end())
        {
            if (it->second == 1)
            {
                m_canGet = 1;
                break;
            }
            ++it;
        }
        cdata.setExtraData(char_data_type_daily, char_data_daily_seven_action_reward, 1);
        Singleton<actionMgr>::Instance().notifyTimeLimitActionState(&cdata);
        save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void char_seven_data::check(CharData* pc)
{
    int today_set = pc->queryExtraData(char_data_type_daily, char_data_daily_seven_action_reward);
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator itr = m_handle.m_rewards.begin(); itr != m_handle.m_rewards.end(); ++itr)
    {
        if (itr->second.get() && m_cur_day == itr->first)
        {
            if (m_geted[itr->first] == 0 && today_set == 0)
            {
                m_geted[itr->first] = 1;
                m_cur_day += 1;
                m_finish_list.push_back(itr->first);
                m_canGet = 1;
                today_set = 1;
                pc->setExtraData(char_data_type_daily, char_data_daily_seven_action_reward, 1);
                pc->updateTopButton(top_button_timeLimitAction, m_canGet);
                save();
            }
        }
    }
}

void char_seven_data::save()
{
    std::string sql = "update char_action_seven set reward_getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "',finish_list='";
    int finish_count = 0;
    for (std::vector<int>::iterator it = m_finish_list.begin(); it != m_finish_list.end(); ++it)
    {
        if (finish_count)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++finish_count;
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

char_equipt_level_data::char_equipt_level_data(equipt_level_action& h, int cid)
:m_handle(h)
{
    m_cid = cid;
    m_canGet = 0;
}

int char_equipt_level_data::getAwards(CharData& cdata, int level, json_spirit::Object& robj)
{
    if (m_geted[level] == 1)
    {
        if (m_handle.m_rewards.find(level) != m_handle.m_rewards.end())
        {
            baseLibao* lb = m_handle.m_rewards[level].get();
            if (lb)
            {
                //给东西
                if (!cdata.m_bag.hasSlot(lb->need_slot_num))
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, NULL, &robj, true, loot_limit_action);
            }
        }

        m_geted[level] = 2;
        m_canGet = 0;
        for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
        {
            if (it->second == 1)
            {
                m_canGet = 1;
                break;
            }
        }
        Singleton<actionMgr>::Instance().notifyTimeLimitActionState(&cdata);
        save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void char_equipt_level_data::check(CharHeroData* ch)
{
    m_canGet = 0;
    if (ch == NULL)
    {
        return;
    }
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_handle.m_rewards.begin(); it != m_handle.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            if (m_geted[it->first] == 0)
            {
                if (ch->checkEquiptLevel(it->first))
                {
                    m_geted[it->first] = 1;
                    m_finish_list.push_back(it->first);
                    m_canGet = 1;
                    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
                    if (cdata.get())
                    {
                        cdata->updateTopButton(top_button_timeLimitAction, m_canGet);
                    }
                }
            }
        }
    }
}

void char_equipt_level_data::save()
{
    std::string sql = "update char_action_equipt_level set reward_getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "',finish_list='";
    int finish_count = 0;
    for (std::vector<int>::iterator it = m_finish_list.begin(); it != m_finish_list.end(); ++it)
    {
        if (finish_count)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++finish_count;
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

bool hero_star_action::in_list(int star)
{
    if (m_rewards.find(star) != m_rewards.end())
    {
        return m_rewards[star].get() != NULL;
    }
    else
    {
        return false;
    }
}

char_hero_star_data::char_hero_star_data(hero_star_action& h, int cid)
:m_handle(h)
{
    m_cid = cid;
    m_canGet = 0;
}

int char_hero_star_data::getAwards(CharData& cdata, int star, json_spirit::Object& robj)
{
    if (m_geted[star] == 1)
    {
        if (m_handle.m_rewards.find(star) != m_handle.m_rewards.end())
        {
            baseLibao* lb = m_handle.m_rewards[star].get();
            if (lb)
            {
                //给东西
                if (!cdata.m_bag.hasSlot(lb->need_slot_num))
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, NULL, &robj, true, loot_limit_action);
            }
        }
        m_geted[star] = 2;
        m_canGet = 0;
        for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
        {
            if (it->second == 1)
            {
                m_canGet = 1;
                break;
            }
        }
        Singleton<actionMgr>::Instance().notifyTimeLimitActionState(&cdata);
        save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void char_hero_star_data::heroStar(int star)
{
    if (m_handle.in_list(star))
    {
        if (m_geted[star] == 0)
        {
            m_geted[star] = 1;
            m_finish_list.push_back(star);
            save();
            m_canGet = 1;
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
            if (cdata.get())
            {
                cdata->updateTopButton(top_button_timeLimitAction, m_canGet);
            }
        }
    }
}

void char_hero_star_data::save()
{
    std::string sql = "update char_action_hero_star set reward_getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "',finish_list='";
    int finish_count = 0;
    for (std::vector<int>::iterator it = m_finish_list.begin(); it != m_finish_list.end(); ++it)
    {
        if (finish_count)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++finish_count;
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

bool hero_action::in_list(int id)
{
    if (m_rewards.find(id) != m_rewards.end())
    {
        return m_rewards[id].get() != NULL;
    }
    else
    {
        return false;
    }
}

char_hero_data::char_hero_data(hero_action& h, int cid)
:m_handle(h)
{
    m_cid = cid;
    m_canGet = 0;
}

int char_hero_data::getAwards(CharData& cdata, int id, json_spirit::Object& robj)
{
    if (m_geted[id] == 1)
    {
        if (m_handle.m_rewards.find(id) != m_handle.m_rewards.end())
        {
            baseLibao* lb = m_handle.m_rewards[id].get();
            if (lb)
            {
                //给东西
                if (!cdata.m_bag.hasSlot(lb->need_slot_num))
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, NULL, &robj, true, loot_limit_action);
            }
        }
        m_geted[id] = 2;
        m_canGet = 0;
        for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
        {
            if (it->second == 1)
            {
                m_canGet = 1;
                break;
            }
        }
        Singleton<actionMgr>::Instance().notifyTimeLimitActionState(&cdata);
        save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void char_hero_data::heroGet(int id)
{
    if (m_handle.in_list(id))
    {
        if (m_geted[id] == 0)
        {
            m_geted[id] = 1;
            m_finish_list.push_back(id);
            save();
            m_canGet = 1;
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
            if (cdata.get())
            {
                cdata->updateTopButton(top_button_timeLimitAction, m_canGet);
            }
        }
    }
}

void char_hero_data::save()
{
    std::string sql = "update char_action_hero set reward_getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "',finish_list='";
    int finish_count = 0;
    for (std::vector<int>::iterator it = m_finish_list.begin(); it != m_finish_list.end(); ++it)
    {
        if (finish_count)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++finish_count;
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

bool stronghold_action::in_list(int id)
{
    if (m_rewards.find(id) != m_rewards.end())
    {
        return m_rewards[id].get() != NULL;
    }
    else
    {
        return false;
    }
}

char_stronghold_data::char_stronghold_data(stronghold_action& h, int cid)
:m_handle(h)
{
    m_cid = cid;
    m_canGet = 0;
}

int char_stronghold_data::getAwards(CharData& cdata, int id, json_spirit::Object& robj)
{
    if (m_geted[id] == 1)
    {
        if (m_handle.m_rewards.find(id) != m_handle.m_rewards.end())
        {
            baseLibao* lb = m_handle.m_rewards[id].get();
            if (lb)
            {
                //给东西
                if (!cdata.m_bag.hasSlot(lb->need_slot_num))
                {
                    return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
                }
                std::list<Item> items = lb->m_list;
                giveLoots(&cdata, items, NULL, &robj, true, loot_limit_action);
            }
        }
        m_geted[id] = 2;
        m_canGet = 0;
        for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
        {
            if (it->second == 1)
            {
                m_canGet = 1;
                break;
            }
        }
        Singleton<actionMgr>::Instance().notifyTimeLimitActionState(&cdata);
        save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void char_stronghold_data::stronghold(int id)
{
    if (m_handle.in_list(id))
    {
        if (m_geted[id] == 0)
        {
            m_geted[id] = 1;
            m_finish_list.push_back(id);
            save();
            m_canGet = 1;
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
            if (cdata.get())
            {
                cdata->updateTopButton(top_button_timeLimitAction, m_canGet);
            }
        }
    }
}

void char_stronghold_data::save()
{
    std::string sql = "update char_action_stronghold set reward_getted='";
    int get_count = 0;
    for (std::map<int,int>::iterator it = m_geted.begin(); it != m_geted.end(); ++it)
    {
        if (it->second == 2)
        {
            if (get_count)
            {
                sql += ("," + LEX_CAST_STR(it->first));
            }
            else
            {
                sql += LEX_CAST_STR(it->first);
            }
            ++get_count;
        }
    }
    sql += "',finish_list='";
    int finish_count = 0;
    for (std::vector<int>::iterator it = m_finish_list.begin(); it != m_finish_list.end(); ++it)
    {
        if (finish_count)
        {
            sql += ("," + LEX_CAST_STR(*it));
        }
        else
        {
            sql += LEX_CAST_STR(*it);
        }
        ++finish_count;
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

actionMgr::actionMgr()
{
    Query q(GetDb());
    //签到活动
    q.get_result("select days,itemType,itemId,counts,extra from base_sign_libao where 1 order by days,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int days = q.getval();
        Item itm;
        itm.type = q.getval();
        itm.id = q.getval();
        itm.nums = q.getval();
        itm.extra = q.getval();

        if (m_sign_action.m_rewards[days].get() == NULL)
        {
            baseLibao* pb = new baseLibao;
            pb->m_need_extra = days;
            m_sign_action.m_rewards[days].reset(pb);
            pb->m_list.push_back(itm);
        }
        else
        {
            m_sign_action.m_rewards[days]->m_list.push_back(itm);
        }
    }
    q.free_result();

    q.get_result("select day,itemType,itemId,counts,extra from base_sign_single_libao where 1 order by day,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int day = q.getval();
        Item itm;
        itm.type = q.getval();
        itm.id = q.getval();
        itm.nums = q.getval();
        itm.extra = q.getval();

        if (m_sign_action.m_single_rewards[day].get() == NULL)
        {
            baseLibao* pb = new baseLibao;
            pb->m_need_extra = day;
            m_sign_action.m_single_rewards[day].reset(pb);
            pb->m_list.push_back(itm);
        }
        else
        {
            m_sign_action.m_single_rewards[day]->m_list.push_back(itm);
        }
    }
    q.free_result();

    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_sign_action.m_rewards.begin(); it != m_sign_action.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->updateObj();
        }
    }
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_sign_action.m_single_rewards.begin(); it != m_sign_action.m_single_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->updateObj();
        }
    }
    //签到商店
    q.get_result("SELECT id,needscore,itemType,itemId,counts,extra FROM base_sign_shop_rewards WHERE 1 order by id asc");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        int id = q.getval();
        baseSignShopGoods tmp;
        tmp.needscore = q.getval();
        tmp.reward.type = q.getval();
        tmp.reward.id = q.getval();
        tmp.reward.nums = q.getval();
        tmp.reward.extra = q.getval();
        m_sign_shop_goods.push_back(tmp);
    }
    q.free_result();

    //七日活动
    {
        q.get_result("select days,itemType,itemId,counts,extra from base_action_seven where 1 order by days,id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int days = q.getval();
            Item itm;
            itm.type = q.getval();
            itm.id = q.getval();
            itm.nums = q.getval();
            itm.extra = q.getval();

            if (m_seven_action.m_rewards[days].get() == NULL)
            {
                baseLibao* pb = new baseLibao;
                pb->m_need_extra = days;
                m_seven_action.m_rewards[days].reset(pb);
                pb->m_list.push_back(itm);
            }
            else
            {
                m_seven_action.m_rewards[days]->m_list.push_back(itm);
            }
        }
        q.free_result();
        for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_seven_action.m_rewards.begin(); it != m_seven_action.m_rewards.end(); ++it)
        {
            if (it->second.get())
            {
                it->second->updateObj();
            }
        }
    }

    //强化活动
    {
        q.get_result("select level,itemType,itemId,counts,extra from base_action_equipt_level where 1 order by level,id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int level = q.getval();
            Item itm;
            itm.type = q.getval();
            itm.id = q.getval();
            itm.nums = q.getval();
            itm.extra = q.getval();

            if (m_equipt_level_action.m_rewards[level].get() == NULL)
            {
                baseLibao* pb = new baseLibao;
                pb->m_need_extra = level;
                m_equipt_level_action.m_rewards[level].reset(pb);
                pb->m_list.push_back(itm);
            }
            else
            {
                m_equipt_level_action.m_rewards[level]->m_list.push_back(itm);
            }
        }
        q.free_result();
        for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_equipt_level_action.m_rewards.begin(); it != m_equipt_level_action.m_rewards.end(); ++it)
        {
            if (it->second.get())
            {
                it->second->updateObj();
            }
        }
    }

    //升星活动
    {
        q.get_result("select star,itemType,itemId,counts,extra from base_action_hero_star where 1 order by star,id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int star = q.getval();
            Item itm;
            itm.type = q.getval();
            itm.id = q.getval();
            itm.nums = q.getval();
            itm.extra = q.getval();

            if (m_hero_star_action.m_rewards[star].get() == NULL)
            {
                baseLibao* pb = new baseLibao;
                pb->m_need_extra = star;
                m_hero_star_action.m_rewards[star].reset(pb);
                pb->m_list.push_back(itm);
            }
            else
            {
                m_hero_star_action.m_rewards[star]->m_list.push_back(itm);
            }
        }
        q.free_result();
        for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_hero_star_action.m_rewards.begin(); it != m_hero_star_action.m_rewards.end(); ++it)
        {
            if (it->second.get())
            {
                it->second->updateObj();
            }
        }
    }

    //英雄活动
    {
        q.get_result("select hid,itemType,itemId,counts,extra from base_action_hero where 1 order by hid,id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int hid = q.getval();
            Item itm;
            itm.type = q.getval();
            itm.id = q.getval();
            itm.nums = q.getval();
            itm.extra = q.getval();

            if (m_hero_action.m_rewards[hid].get() == NULL)
            {
                baseLibao* pb = new baseLibao;
                pb->m_need_extra = hid;
                m_hero_action.m_rewards[hid].reset(pb);
                pb->m_list.push_back(itm);
            }
            else
            {
                m_hero_action.m_rewards[hid]->m_list.push_back(itm);
            }
        }
        q.free_result();
        for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_hero_action.m_rewards.begin(); it != m_hero_action.m_rewards.end(); ++it)
        {
            if (it->second.get())
            {
                it->second->updateObj();
            }
        }
    }

    //冲关活动
    {
        q.get_result("select stronghold,itemType,itemId,counts,extra from base_action_stronghold where 1 order by stronghold,id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int stronghold = q.getval();
            Item itm;
            itm.type = q.getval();
            itm.id = q.getval();
            itm.nums = q.getval();
            itm.extra = q.getval();

            if (m_stronghold_action.m_rewards[stronghold].get() == NULL)
            {
                baseLibao* pb = new baseLibao;
                pb->m_need_extra = stronghold;
                m_stronghold_action.m_rewards[stronghold].reset(pb);
                pb->m_list.push_back(itm);
            }
            else
            {
                m_stronghold_action.m_rewards[stronghold]->m_list.push_back(itm);
            }
        }
        q.free_result();
        for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_stronghold_action.m_rewards.begin(); it != m_stronghold_action.m_rewards.end(); ++it)
        {
            if (it->second.get())
            {
                it->second->updateObj();
            }
        }
    }
}

//查询限时活动状态
int actionMgr::getTimeLimitActionState(CharData* pc)
{
    //0没可领取1可领取2结束
    int active = 2;
    if (active != 1 && pc->queryCreateDays() <= WEEK)
    {
        char_seven_data* e = getCharSevenData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
    }
    if (active != 1 && pc->queryCreateDays() <= WEEK)
    {
        char_equipt_level_data* e = getCharEquiptLevelData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
    }
    if (active != 1 && pc->queryCreateDays() <= WEEK)
    {
        char_hero_star_data* e = getCharHeroStarData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
    }
    if (active != 1 && pc->queryCreateDays() <= WEEK)
    {
        char_hero_data* e = getCharHeroData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
    }
    if (active != 1 && pc->queryCreateDays() <= WEEK)
    {
        //每天第一闪
        int first_view = pc->queryExtraData(char_data_type_daily, char_data_daily_view_hero_pack_action);
        if (0 == first_view)
        {
            active = 1;
        }
    }
    if (active != 1 && pc->queryCreateDays() <= WEEK)
    {
        char_stronghold_data* e = getCharStrongholdData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
    }
    return active;
}

void actionMgr::notifyTimeLimitActionState(CharData* pc)
{
    int active = getTimeLimitActionState(pc);
    if (active == 2)
    {
        pc->removeTopButton(top_button_timeLimitAction);
    }
    else
    {
        pc->updateTopButton(top_button_timeLimitAction, active);
    }
}

void actionMgr::getTimeLimitActionList(CharData* pc, json_spirit::Object& robj)
{
    json_spirit::Array list;
    if (pc->queryCreateDays() <= WEEK)
    {
        int active = 0;
        char_seven_data* e = getCharSevenData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
        json_spirit::Object obj;
        obj.push_back( Pair("type", action_seven) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    if (pc->queryCreateDays() <= WEEK)
    {
        int active = 0;
        char_equipt_level_data* e = getCharEquiptLevelData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
        json_spirit::Object obj;
        obj.push_back( Pair("type", action_equipt_level) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    if (pc->queryCreateDays() <= WEEK)
    {
        int active = 0;
        char_hero_star_data* e = getCharHeroStarData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
        json_spirit::Object obj;
        obj.push_back( Pair("type", action_hero_star) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    if (pc->queryCreateDays() <= WEEK)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", action_hero_pack) );
        //每天第一闪
        int first_view = pc->queryExtraData(char_data_type_daily, char_data_daily_view_hero_pack_action);
        obj.push_back( Pair("active", first_view == 0 ? 1 : 0) );
        list.push_back(obj);
    }
    if (pc->queryCreateDays() <= WEEK)
    {
        int active = 0;
        char_hero_data* e = getCharHeroData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
        json_spirit::Object obj;
        obj.push_back( Pair("type", action_hero) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    if (pc->queryCreateDays() <= WEEK)
    {
        int active = 0;
        char_stronghold_data* e = getCharStrongholdData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
        json_spirit::Object obj;
        obj.push_back( Pair("type", action_stronghold) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    robj.push_back( Pair("list", list) );
    return;
}

void actionMgr::notifySignState(CharData* pc)
{
    if (pc->isSignOpen())
    {
        int active = 0;
        char_sign_data* e = getCharSignData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
        pc->updateTopButton(top_button_sign, active);
    }
}

void actionMgr::getButton(CharData* pc, json_spirit::Array& list)
{
    if (pc->isTimelimitActionOpen()
         && pc->queryCreateDays() <= WEEK)
    {
        int active = getTimeLimitActionState(pc);
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_timeLimitAction) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    if (pc->isSignOpen())
    {
        int active = 0;
        char_sign_data* e = getCharSignData(pc->m_id);
        if (e)
        {
            active = e->m_canGet;
        }
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_sign) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    if (pc->isOnlineLibaoOpen())
    {
        int active = 0;
        char_online_libao_data* online = getCharOnlineLibaoData(pc->m_id);
        if (online)
        {
            active = online->getOnlineLibaoState();
        }
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_button_online) );
        obj.push_back( Pair("active", active) );
        list.push_back(obj);
    }
    return;
}

void actionMgr::getSignInfo(char_sign_data& e, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(e.m_cid);
    if (!cdata.get())
    {
        ERR();
        return;
    }
    robj.push_back( Pair("total", e.m_total_sign) );
    robj.push_back( Pair("curTime", e.m_time_now) );
    robj.push_back( Pair("canSign", e.m_sign_time == 0 ? 1 : 0) );
    std::vector<int> sign_date;
    for (std::map<int, int>::iterator it = e.m_sign_data.begin(); it != e.m_sign_data.end(); ++it)
    {
        if (it->second == 1)
        {
            sign_date.push_back(it->first);
        }
    }
    json_spirit::Array signlist(sign_date.begin(), sign_date.end());
    robj.push_back( Pair("signList", signlist) );
    //开服时间之前的日期无法签到
    time_t open = GeneralDataMgr::getInstance()->getServerOpenTime();
    struct tm open_tm;
    localtime_r(&open, &open_tm);
    //当前月正好是开服的月份
    if (e.m_tm_now.tm_mon == open_tm.tm_mon
        && e.m_tm_now.tm_year == open_tm.tm_year)
    {
        robj.push_back( Pair("openDay", open_tm.tm_mday) );
    }
    //累积奖励
    json_spirit::Array tlist;
    int cur_t = 1, cur_state = 2;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_sign_action.m_rewards.begin(); it != m_sign_action.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            //前面有未领取的
            if (cur_state == 1)
            {
                ;
            }
            //前面全部领取了
            else if (cur_state == 2)
            {
                cur_t = it->first;
                cur_state = e.m_getted[it->first];
            }
            //前面全部不能领取
            else if (cur_state == 0)
            {
                if (e.m_getted[it->first] == 1)
                {
                    cur_t = it->first;
                    cur_state = 1;
                }
            }
            json_spirit::Object obj;
            obj.push_back( Pair("times", it->first) );
            obj.push_back( Pair("get", e.m_getted[it->first]) );

            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );

            tlist.push_back(obj);
        }
    }
    robj.push_back( Pair("tlist", tlist) );
    //每日奖励
    json_spirit::Array slist;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_sign_action.m_single_rewards.begin(); it != m_sign_action.m_single_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("day", it->first) );
            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );
			slist.push_back(obj);
        }
    }
    robj.push_back( Pair("slist", slist) );
    robj.push_back( Pair("show_t", cur_t) );
    int score = cdata->queryExtraData(char_data_type_normal, char_data_normal_sign_score);
    robj.push_back( Pair("score", score) );
    robj.push_back( Pair("sign_cost", iActionSignGold) );
    e.m_canGet = e.getSignAction();
}

char_sign_data* actionMgr::getCharSignData(int cid)
{
    if (m_char_sign_actions.find(cid) != m_char_sign_actions.end())
    {
        m_char_sign_actions[cid]->checkReset();
        return m_char_sign_actions[cid].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select ctime,sign_data,sign_time,reward_getted from char_signs where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            char_sign_data* ce = new char_sign_data(m_sign_action, cid);
            ce->m_time_now = time(NULL);
            localtime_r(&ce->m_time_now, &ce->m_tm_now);

            ce->m_generate_time = q.getval();
            localtime_r(&ce->m_generate_time, &ce->m_tm_generate);

            std::string data = q.getstr();
            std::vector<int> sign_list;
            read_int_vector(data, sign_list);
            for (std::vector<int>::iterator it = sign_list.begin(); it != sign_list.end(); ++it)
            {
                ce->m_sign_data[*it] = 1;
            }

            ce->m_sign_time = q.getval();
            std::string getted_data = q.getstr();
            q.free_result();

            ce->m_total_sign = sign_list.size();

            if (sign_list.size() > 0 && ce->m_tm_now.tm_mday == *sign_list.rbegin())
            {
                if (ce->m_sign_time == 0)
                {
                    ce->m_sign_time = ce->m_time_now;
                }
            }
            else
            {
                ce->m_sign_time = 0;
            }

            if (ce->m_sign_time == 0)
            {
                ce->m_canGet = 1;
            }

            std::vector<int> get_list;
            read_int_vector(getted_data, get_list);
            for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
            {
                ce->m_getted[*it] = 2;
            }

            for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_sign_action.m_rewards.begin(); it != m_sign_action.m_rewards.end(); ++it)
            {
                if (ce->m_getted[it->first] == 0 && it->first <= ce->m_total_sign)
                {
                    ce->m_getted[it->first] = 1;
                    ce->m_canGet = 1;
                }
            }
            m_char_sign_actions[cid].reset(ce);
            ce->checkReset();
        }
        else
        {
            q.free_result();

            //是否要插入数据?
            char_sign_data* ce = new char_sign_data(m_sign_action, cid);
            ce->m_time_now = time(NULL);
            localtime_r(&ce->m_time_now, &ce->m_tm_now);

            //全部重置
            ce->m_getted.clear();
            ce->m_sign_data.clear();
            ce->m_tm_generate = ce->m_tm_now;
            ce->m_generate_time = ce->m_time_now;

            ce->m_sign_time = 0;
            ce->m_total_sign = 0;

            InsertSaveDb("replace into char_signs (cid,ctime,sign_data,sign_time,reward_getted) values ("
                + LEX_CAST_STR(ce->m_cid) + ","
                + LEX_CAST_STR(ce->m_generate_time) + ",'',0,'')");

            m_char_sign_actions[cid].reset(ce);
        }
        return m_char_sign_actions[cid].get();
    }
}

void actionMgr::getOnlineLibaoInfo(char_online_libao_data& e, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(e.m_cid);
    if (!cdata.get())
    {
        ERR();
        return;
    }
    int t_online = cdata->getTodayOnlineTime();
    e.updateState(2);
    json_spirit::Array list;
    for (int i = 0; i < iActionOnlineLibaoMax; ++i)
    {
        if (i < e.m_online_libaos.size())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", e.m_online_libaos[i].id) );
            obj.push_back( Pair("state", e.m_online_libaos[i].state) );
            obj.push_back( Pair("total_time", e.m_online_libaos[i].total_time) );
            int leftsecond = 0;
            if (e.m_online_libaos[i].state == 0)
            {
                leftsecond = e.m_online_libaos[i].total_online_time - t_online;
            }
            obj.push_back( Pair("left_time", leftsecond) );
            if (i < e.m_get_item.size())
            {
                json_spirit::Object get;
                e.m_get_item[i].toObj(get);
                obj.push_back( Pair("get", get) );
            }
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("online_time", t_online) );
    robj.push_back( Pair("list", list) );
}

char_online_libao_data* actionMgr::getCharOnlineLibaoData(int cid)
{
    if (m_char_online_actions.find(cid) != m_char_online_actions.end())
    {
        return m_char_online_actions[cid].get();
    }
    else
    {
        Query q(GetDb());
        char_online_libao_data* ce = new char_online_libao_data(cid);
        //加载已经领取
        q.get_result("select itemType,itemId,counts,extra from char_online_libao_get where cid=" + LEX_CAST_STR(cid) + " order by libaoid");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            Item lc;
            lc.type = q.getval();
            lc.id = q.getval();
            lc.nums = q.getval();
            lc.extra = q.getval();
            ce->m_get_item.push_back(lc);
        }
        q.free_result();
        int total_online_need = 0;
        for (int i = 0; i < iActionOnlineLibaoMax; ++i)
        {
            char_online_libao online_libao;
            online_libao.cid = cid;
            online_libao.id = i+1;
            online_libao.total_time = iActionOnlineLibaoSec[i];
            total_online_need += online_libao.total_time;
            online_libao.total_online_time = total_online_need;
            //加载礼包状态
            q.get_result("select state from char_online_libao where cid=" + LEX_CAST_STR(cid) + " and libaoid=" + LEX_CAST_STR(online_libao.id));
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                online_libao.state = q.getval();
                q.free_result();
            }
            else
            {
                q.free_result();
                online_libao.state = 0;

                InsertSaveDb("replace into char_online_libao (cid,libaoid,state) values ("
                    + LEX_CAST_STR(ce->m_cid) + ","
                    + LEX_CAST_STR(online_libao.id) + ","
                    + LEX_CAST_STR(online_libao.state) + ")");
            }
            ce->m_online_libaos.push_back(online_libao);
        }
        m_char_online_actions[cid].reset(ce);
        return m_char_online_actions[cid].get();
    }
}

void actionMgr::updateOnlineLibaoAction(int cid)
{
    char_online_libao_data* co = getCharOnlineLibaoData(cid);
    if (co)
    {
        co->updateState(1);
    }
}

void actionMgr::resetOnlineLibaoAll()
{
    //删除定时器
    std::map<int, boost::shared_ptr<char_online_libao_data> >::iterator it = m_char_online_actions.begin();
    while(it != m_char_online_actions.end())
    {
        if (it->second.get())
        {
            it->second->del_timer();
        }
        ++it;
    }
    m_char_online_actions.clear();
    Query q(GetDb());
    if (!q.execute("TRUNCATE TABLE char_online_libao"))
    {
        CHECK_DB_ERR(q);
    }
    if (!q.execute("TRUNCATE TABLE char_online_libao_get"))
    {
        CHECK_DB_ERR(q);
    }
}

bool actionMgr::isSevenActionOpen(CharData* pc)
{
    return pc->queryCreateDays() <= WEEK;
}

void actionMgr::getSevenAction(CharData* pc, char_seven_data& ce, json_spirit::Object& robj)
{
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_seven_action.m_rewards.begin(); it != m_seven_action.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("day", it->first) );
            obj.push_back( Pair("get", ce.m_geted[it->first]) );
            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("cur_day", ce.m_cur_day) );
    robj.push_back( Pair("start_time", pc->m_createTime) );
    robj.push_back( Pair("end_time", pc->queryCreateXDays(7)));
    return;
}

char_seven_data* actionMgr::getCharSevenData(int cid)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL || pc->queryCreateDays() > WEEK)
    {
        return NULL;
    }
    if (m_char_seven_actions.find(cid) != m_char_seven_actions.end())
    {
        return m_char_seven_actions[cid].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select reward_getted,finish_list from char_action_seven where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            char_seven_data* ce = new char_seven_data(m_seven_action, cid);
            std::string data = q.getstr();
            std::vector<int> get_list;
            read_int_vector(data, get_list);
            for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
            {
                ce->m_geted[*it] = 2;
                ce->m_cur_day = *it + 1;
            }
            std::string f = q.getstr();
            read_int_vector(f, ce->m_finish_list);
            for (std::vector<int>::iterator it = ce->m_finish_list.begin(); it != ce->m_finish_list.end(); ++it)
            {
                if (ce->m_geted[*it] == 0)
                {
                    ce->m_geted[*it] = 1;
                    ce->m_cur_day = *it + 1;
                    ce->m_canGet = 1;
                }
            }
            m_char_seven_actions[cid].reset(ce);
            q.free_result();
        }
        else
        {
            q.free_result();
            char_seven_data* ce = new char_seven_data(m_seven_action, cid);
            m_char_seven_actions[cid].reset(ce);
            InsertSaveDb("replace into char_action_seven (cid,reward_getted,finish_list) values (" + LEX_CAST_STR(pc->m_id) + ",'','')");
        }
        return m_char_seven_actions[cid].get();
    }
}

void actionMgr::updateSevenAction(CharData* pc)
{
    if (pc == NULL)
        return;
    char_seven_data* e = getCharSevenData(pc->m_id);
    if (e)
    {
        e->check(pc);
    }
    return;
}

bool actionMgr::isEquiptLevelActionOpen(CharData* pc)
{
    return pc->queryCreateDays() <= WEEK;
}

void actionMgr::getEquiptLevelAction(CharData* pc, char_equipt_level_data& ce, json_spirit::Object& robj)
{
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_equipt_level_action.m_rewards.begin(); it != m_equipt_level_action.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("level", it->first) );
            obj.push_back( Pair("get", ce.m_geted[it->first]) );
            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("start_time", pc->m_createTime) );
    robj.push_back( Pair("end_time", pc->queryCreateXDays(7)));
    return;
}

char_equipt_level_data* actionMgr::getCharEquiptLevelData(int cid)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL || pc->queryCreateDays() > WEEK)
    {
        return NULL;
    }
    if (m_char_equipt_level_actions.find(cid) != m_char_equipt_level_actions.end())
    {
        return m_char_equipt_level_actions[cid].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select reward_getted,finish_list from char_action_equipt_level where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            char_equipt_level_data* ce = new char_equipt_level_data(m_equipt_level_action, cid);
            std::string data = q.getstr();
            std::vector<int> get_list;
            read_int_vector(data, get_list);
            for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
            {
                ce->m_geted[*it] = 2;
            }
            std::string f = q.getstr();
            read_int_vector(f, ce->m_finish_list);
            for (std::vector<int>::iterator it = ce->m_finish_list.begin(); it != ce->m_finish_list.end(); ++it)
            {
                if (ce->m_geted[*it] == 0)
                {
                    ce->m_geted[*it] = 1;
                }
            }
            m_char_equipt_level_actions[cid].reset(ce);
            q.free_result();
        }
        else
        {
            q.free_result();
            char_equipt_level_data* ce = new char_equipt_level_data(m_equipt_level_action, cid);
            m_char_equipt_level_actions[cid].reset(ce);
            InsertSaveDb("replace into char_action_equipt_level (cid,reward_getted,finish_list) values (" + LEX_CAST_STR(pc->m_id) + ",'','')");
        }
        return m_char_equipt_level_actions[cid].get();
    }
}

void actionMgr::updateEquiptLevelAction(CharHeroData* ch)
{
    if (ch == NULL)
        return;
    char_equipt_level_data* e = getCharEquiptLevelData(ch->m_cid);
    if (e)
    {
        e->check(ch);
    }
    return;
}

bool actionMgr::isHeroStarActionOpen(CharData* pc)
{
    return pc->queryCreateDays() <= WEEK;
}

void actionMgr::getHeroStarAction(CharData* pc, char_hero_star_data& ce, json_spirit::Object& robj)
{
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_hero_star_action.m_rewards.begin(); it != m_hero_star_action.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("star", it->first) );
            obj.push_back( Pair("get", ce.m_geted[it->first]) );
            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("start_time", pc->m_createTime) );
    robj.push_back( Pair("end_time", pc->queryCreateXDays(7)));
    return;
}

char_hero_star_data* actionMgr::getCharHeroStarData(int cid)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL || pc->queryCreateDays() > WEEK)
    {
        return NULL;
    }
    if (m_char_hero_star_actions.find(cid) != m_char_hero_star_actions.end())
    {
        return m_char_hero_star_actions[cid].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select reward_getted,finish_list from char_action_hero_star where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            char_hero_star_data* ce = new char_hero_star_data(m_hero_star_action, cid);
            std::string data = q.getstr();
            std::vector<int> get_list;
            read_int_vector(data, get_list);
            for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
            {
                ce->m_geted[*it] = 2;
            }
            std::string f = q.getstr();
            read_int_vector(f, ce->m_finish_list);
            for (std::vector<int>::iterator it = ce->m_finish_list.begin(); it != ce->m_finish_list.end(); ++it)
            {
                if (ce->m_geted[*it] == 0)
                {
                    ce->m_geted[*it] = 1;
                }
            }
            m_char_hero_star_actions[cid].reset(ce);
            q.free_result();
        }
        else
        {
            q.free_result();
            char_hero_star_data* ce = new char_hero_star_data(m_hero_star_action, cid);
            m_char_hero_star_actions[cid].reset(ce);
            InsertSaveDb("replace into char_action_hero_star (cid,reward_getted,finish_list) values (" + LEX_CAST_STR(pc->m_id) + ",'','')");
        }
        return m_char_hero_star_actions[cid].get();
    }
}

void actionMgr::updateHeroStarAction(int cid, int star)
{
    char_hero_star_data* e = getCharHeroStarData(cid);
    if (e)
    {
        e->heroStar(star);
    }
    return;
}

bool actionMgr::isHeroPackActionOpen(CharData* pc)
{
    return pc->queryCreateDays() <= WEEK;
}

void actionMgr::getHeroPackAction(CharData* pc, json_spirit::Object& robj)
{
    robj.push_back( Pair("start_time", pc->m_createTime) );
    robj.push_back( Pair("end_time", pc->queryCreateXDays(7)));
    return;
}

bool actionMgr::isHeroActionOpen(CharData* pc)
{
    return pc->queryCreateDays() <= WEEK;
}

void actionMgr::getHeroAction(CharData* pc, char_hero_data& ce, json_spirit::Object& robj)
{
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_hero_action.m_rewards.begin(); it != m_hero_action.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("hid", it->first) );
            boost::shared_ptr<baseHeroData> bs = Singleton<HeroMgr>::Instance().GetBaseHero(it->first);
            if (!bs.get())
            {
                continue;
            }
            json_spirit::Object hero_obj;
            hero_obj.push_back( Pair("id", it->first));
            hero_obj.push_back( Pair("star", bs->m_quality));
            hero_obj.push_back( Pair("level", 1));
            hero_obj.push_back( Pair("exp", 0));
            hero_obj.push_back( Pair("quality", bs->m_quality));
            bs->toObj(hero_obj);
            obj.push_back( Pair("hero_info", hero_obj) );
            obj.push_back( Pair("get", ce.m_geted[it->first]) );
            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("start_time", pc->m_createTime) );
    robj.push_back( Pair("end_time", pc->queryCreateXDays(7)));
    return;
}

char_hero_data* actionMgr::getCharHeroData(int cid)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL || pc->queryCreateDays() > WEEK)
    {
        return NULL;
    }
    if (m_char_hero_actions.find(cid) != m_char_hero_actions.end())
    {
        return m_char_hero_actions[cid].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select reward_getted,finish_list from char_action_hero where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            char_hero_data* ce = new char_hero_data(m_hero_action, cid);
            std::string data = q.getstr();
            std::vector<int> get_list;
            read_int_vector(data, get_list);
            for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
            {
                ce->m_geted[*it] = 2;
            }
            std::string f = q.getstr();
            read_int_vector(f, ce->m_finish_list);
            for (std::vector<int>::iterator it = ce->m_finish_list.begin(); it != ce->m_finish_list.end(); ++it)
            {
                if (ce->m_geted[*it] == 0)
                {
                    ce->m_geted[*it] = 1;
                }
            }
            m_char_hero_actions[cid].reset(ce);
            q.free_result();
        }
        else
        {
            q.free_result();
            char_hero_data* ce = new char_hero_data(m_hero_action, cid);
            m_char_hero_actions[cid].reset(ce);
            InsertSaveDb("replace into char_action_hero (cid,reward_getted,finish_list) values (" + LEX_CAST_STR(pc->m_id) + ",'','')");
        }
        return m_char_hero_actions[cid].get();
    }
}

void actionMgr::updateHeroAction(int cid, int id)
{
    char_hero_data* e = getCharHeroData(cid);
    if (e)
    {
        e->heroGet(id);
    }
    return;
}

bool actionMgr::isStrongholdActionOpen(CharData* pc)
{
    return pc->queryCreateDays() <= WEEK;
}

void actionMgr::getStrongholdAction(CharData* pc, char_stronghold_data& ce, json_spirit::Object& robj)
{
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_stronghold_action.m_rewards.begin(); it != m_stronghold_action.m_rewards.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("strongholdid", it->first) );
            boost::shared_ptr<baseStronghold> bs = Singleton<mapMgr>::Instance().GetBaseStrongholdData(it->first);
            if (!bs.get())
            {
                continue;
            }
            obj.push_back( Pair("name", bs->m_name) );
            obj.push_back( Pair("get", ce.m_geted[it->first]) );
            const json_spirit::Array& rlist = it->second->getArray();
            obj.push_back( Pair("list", rlist) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("start_time", pc->m_createTime) );
    robj.push_back( Pair("end_time", pc->queryCreateXDays(7)));
    return;
}

char_stronghold_data* actionMgr::getCharStrongholdData(int cid)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc == NULL || pc->queryCreateDays() > WEEK)
    {
        return NULL;
    }
    if (m_char_stronghold_actions.find(cid) != m_char_stronghold_actions.end())
    {
        return m_char_stronghold_actions[cid].get();
    }
    else
    {
        Query q(GetDb());
        q.get_result("select reward_getted,finish_list from char_action_stronghold where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            char_stronghold_data* ce = new char_stronghold_data(m_stronghold_action, cid);
            std::string data = q.getstr();
            std::vector<int> get_list;
            read_int_vector(data, get_list);
            for (std::vector<int>::iterator it = get_list.begin(); it != get_list.end(); ++it)
            {
                ce->m_geted[*it] = 2;
            }
            std::string f = q.getstr();
            read_int_vector(f, ce->m_finish_list);
            for (std::vector<int>::iterator it = ce->m_finish_list.begin(); it != ce->m_finish_list.end(); ++it)
            {
                if (ce->m_geted[*it] == 0)
                {
                    ce->m_geted[*it] = 1;
                }
            }
            m_char_stronghold_actions[cid].reset(ce);
            q.free_result();
        }
        else
        {
            q.free_result();
            char_stronghold_data* ce = new char_stronghold_data(m_stronghold_action, cid);
            m_char_stronghold_actions[cid].reset(ce);
            InsertSaveDb("replace into char_action_stronghold (cid,reward_getted,finish_list) values (" + LEX_CAST_STR(pc->m_id) + ",'','')");
        }
        return m_char_stronghold_actions[cid].get();
    }
}

void actionMgr::updateStrongholdAction(int cid, int id)
{
    char_stronghold_data* e = getCharStrongholdData(cid);
    if (e)
    {
        e->stronghold(id);
    }
    return;
}

