
#include "corpsExplore.h"

#include "corps.h"

#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>

#include "utils_all.h"
#include "data.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <list>
#include "statistics.h"
#include "singleton.h"
#include "daily_task.h"
#include "rewards.h"
#include "spls_timer.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

//军团实际收益
extern void corpsRealReward(int& get);

//加速每分钟金币
static const int iCorpsExploreSpeedGoldOneMin = 1;

//刷新金币
static const int iCorpsExploreRefreshGold = 10;

corpsExploreData::corpsExploreData()
{
    pos = 1;
    state = 0;
    type = 1;
    name = "";
    quality = 1;
    need_secs = 60;
    done_time = 0;
    fac = 0;
}

void corpsExploreData::start(int cid)
{
    //开始任务计时器
    json_spirit::mObject mobj;
    mobj["cmd"] = "corpsExplore";
    mobj["cid"] = cid;
    mobj["pos"] = pos;
    int left_time = done_time-time(NULL);
    if (left_time <= 0)
        left_time = 0;
    boost::shared_ptr<splsTimer> tmsg;
    tmsg.reset(new splsTimer(left_time, 1, mobj,1));
    m_done_timer = splsTimerMgr::getInstance()->addTimer(tmsg);
}

void corpsExploreData::refresh()
{
    if (state == 0)
    {
        //cout<<"refresh pos "<<pos<<endl;
        type = Singleton<corpsExplore>::Instance().refresh(name, quality, need_secs, fac);
    }
}

bool corpsExploreData::isDone()
{
    return state == 1 && time(NULL) >= done_time;
}

bool corpsExploreData::Done()
{
    if (isDone())
    {
        state = 0;
        refresh();
        return true;
    }
    else
    {
        return false;
    }
}

charCorpsExplore::charCorpsExplore(int id)
{
    cid = id;
    special = 1;
    refresh_special = 0;
    refresh_time = 0;

    Query q(GetDb());

    q.get_result("select data from char_corps_explore_data where cid=" + LEX_CAST_STR(id));
    if (q.fetch_row())
    {
        std::string data = q.getstr();
        q.free_result();
        json_spirit::mValue value;
        json_spirit::read(data, value);
        if (value.type() == json_spirit::obj_type)
        {
            mObject& mobj = value.get_obj();
            READ_INT_FROM_MOBJ(special,mobj,"special");
            if (special == 0)
            {
                special = my_random(1,6);
            }

            special_name = Singleton<corpsExplore>::Instance().getName(special);

            READ_INT_FROM_MOBJ(refresh_time,mobj,"refresh");

            READ_INT_FROM_MOBJ(refresh_special,mobj,"refresh_spe");

            json_spirit::mArray list;
            READ_ARRAY_FROM_MOBJ(list,mobj,"list");

            for (json_spirit::mArray::iterator it = list.begin(); it != list.end(); ++it)
            {
                json_spirit::mValue& v = *it;
                if (v.type() != json_spirit::obj_type)
                {
                    refresh(0);
                    break;
                }
                else
                {
                    json_spirit::mObject& o = v.get_obj();
                    int pos = 1;
                    READ_INT_FROM_MOBJ(pos,o,"pos");
                    if (pos >= 1 && pos <= 3)
                    {
                        que[pos-1].pos = pos;
                        READ_INT_FROM_MOBJ(que[pos-1].quality, o, "quality");
                        READ_INT_FROM_MOBJ(que[pos-1].state, o, "state");
                        READ_INT_FROM_MOBJ(que[pos-1].type, o, "type");
                        READ_INT_FROM_MOBJ(que[pos-1].done_time, o, "done_time");
                        baseCorpsExploreData* b = Singleton<corpsExplore>::Instance().getQuality(que[pos-1].quality);
                        if (b)
                        {
                            que[pos-1].fac = b->fac;
                            que[pos-1].need_secs = 60 * b->mins;
                        }
                        else
                        {
                            que[pos-1].state = 0;
                            refresh(pos);
                            continue;
                        }
                        que[pos-1].name = Singleton<corpsExplore>::Instance().getName(que[pos-1].type);
                        if (que[pos-1].state == 1)
                        {
                            que[pos-1].start(id);
                        }
                    }
                    else
                    {
                        refresh(0);
                        break;
                    }
                }
            }
        }
        else
        {
            refresh(0);
        }
    }
    else
    {
        q.free_result();
        refresh(0);
    }
}

void charCorpsExplore::refresh(int pos)
{
    if (pos == 0)
    {
        for (int i = 0; i < 3; ++i)
        {
            que[i].refresh();
        }
        refresh_time = time(NULL) + 3600;
        Save();
    }
    else if (pos >= 1 && pos <= 3)
    {
        que[pos-1].refresh();
        Save();
    }
}

void charCorpsExplore::checkRefresh()
{
    time_t timex = time(NULL);
    if (timex >= refresh_time)
    {
        refresh(0);
    }
    if (timex >= refresh_special)
    {
        special = my_random(1, 6);
        special_name = Singleton<corpsExplore>::Instance().getName(special);

        struct tm tm;
        struct tm *t = &tm;
        localtime_r(&timex, t);
        t->tm_hour = 0;
        t->tm_min = 0;
        t->tm_sec = 0;
        refresh_special = mktime(t) + iONE_DAY_SECS;
        Save();
    }
}

int charCorpsExplore::getDoneTime()
{
    time_t timex = time(NULL);
    int min_cd = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (que[i].state == 0)
            return 0;
        else if(timex > que[i].done_time)
            return 0;
        else if(min_cd == 0 || (que[i].done_time - timex) < min_cd)
            min_cd = (que[i].done_time - timex);
    }
    return min_cd;
}

bool charCorpsExplore::isDone()
{
    for (int i = 0; i < 3; ++i)
    {
        if (que[i].isDone())
            return true;
    }
    return false;
}

void charCorpsExplore::Save()
{
    json_spirit::Object robj;

    robj.push_back( Pair("special", special) );
    robj.push_back( Pair("refresh", refresh_time) );
    robj.push_back( Pair("refresh_spe", refresh_special) );
    json_spirit::Array list;
    for (int i = 0; i < 3; ++i)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("pos", i+1) );
        obj.push_back( Pair("type", que[i].type) );
        obj.push_back( Pair("quality", que[i].quality) );
        obj.push_back( Pair("state", que[i].state) );
        obj.push_back( Pair("done_time", que[i].done_time) );
        list.push_back(obj);
    }
    robj.push_back( Pair("list", list) );

    InsertSaveDb("replace into char_corps_explore_data (cid,data) values (" + LEX_CAST_STR(cid) + ",'"
            + json_spirit::write(robj) + "')");
}

corpsExplore::corpsExplore()
{
    Query q(GetDb());
    q.get_result("select quality,mins,fac,gailv from base_corps_explore_data where 1 order by quality");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        baseCorpsExploreData bd;
        bd.quality = q.getval();
        bd.mins = q.getval();
        bd.fac = q.getnum();
        bd.gailv = q.getval();

        m_baseDatas.push_back(bd);
        m_gailvs.push_back(bd.gailv);
        assert(bd.quality == m_baseDatas.size());
    }
    q.free_result();

    assert(m_baseDatas.size());

    q.get_result("select type,name from base_corps_explore where 1 order by type");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int type = q.getval();
        m_names.push_back(q.getstr());
        assert(type == m_names.size());
    }
    q.free_result();

    assert(m_names.size());
}

boost::shared_ptr<charCorpsExplore> corpsExplore::getChar(int cid)
{
    std::map<int, boost::shared_ptr<charCorpsExplore> >::iterator it = m_datas.find(cid);
    if (it != m_datas.end())
    {
        if (it->second.get())
        {
            it->second->checkRefresh();
        }
        return it->second;
    }

    boost::shared_ptr<charCorpsExplore> c(new charCorpsExplore(cid));
    m_datas[cid] = c;
    c->checkRefresh();
    return c;
}

baseCorpsExploreData* corpsExplore::getQuality(int quality)
{
    if (quality >= 1 && quality <= m_baseDatas.size())
    {
        return &(m_baseDatas[quality-1]);
    }
    return &(m_baseDatas[0]);
}

std::string corpsExplore::getName(int type)
{
    if (type >= 1 && type <= m_names.size())
    {
        return m_names[type-1];
    }
    return m_names[0];
}

int corpsExplore::refresh(std::string& name, int& quality, int& secs, float& fac)
{
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    boost::random::discrete_distribution<> dist(m_gailvs);
    quality = dist(gen) + 1;
    int type = my_random(1, m_names.size());

    //cout<<"corpsExplore::refresh - type "<<type<<", quality "<<quality<<endl;
    name = m_names[type-1];

    secs = 600;
    fac = 1;
    baseCorpsExploreData* q = getQuality(quality);
    if (q)
    {
        secs = q->mins * 60;
        fac = q->fac;
    }
    return type;
}

int corpsExplore::speed(CharData& cdata, int pos)
{
    charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(cdata.m_id).get();
    if (c == NULL)
    {
        return HC_ERROR;
    }

    if (pos < 1 || pos > 3)
    {
        return HC_ERROR;
    }

    if (c->que[pos-1].state == 0)
    {
        return HC_ERROR;
    }

    int left = c->que[pos-1].done_time - time(NULL);
    if (left <= 0)
    {
        return HC_SUCCESS;
    }

    int gold = left / 60;
    if (gold % 60)
    {
        ++gold;
    }
    gold *= iCorpsExploreSpeedGoldOneMin;
    if (cdata.addGold(-gold) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }

    //金币消耗统计
    add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, gold, gold_cost_for_corps_explore_speed, cdata.m_union_id, cdata.m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(&cdata,gold,gold_cost_for_corps_explore_speed);
#endif

    cdata.NotifyCharData();
    c->que[pos-1].done_time = 0;
    if (!c->que[pos-1].m_done_timer.is_nil())
    {
        splsTimerMgr::getInstance()->delTimer(c->que[pos-1].m_done_timer);
        c->que[pos-1].m_done_timer = boost::uuids::nil_uuid();
    }
    int nums = cdata.queryExtraData(char_data_type_daily, char_data_daily_corps_explore);
    int corps_explore_times = 0;
    splsCorps* cp = corpsMgr::getInstance()->findCorps(cdata.m_corps_member.get()->corps);
    if (cp)
    {
        corps_explore_times = iCorpsExploreTimesOneday[cp->_level];
    }
    if (corps_explore_times > nums)
    {
        //军团活动按钮闪动
        json_spirit::Object action;
        action.push_back( Pair("cmd", "updateAction") );
        action.push_back( Pair("type", top_level_event_corp) );
        action.push_back( Pair("active", 1) );
        action.push_back( Pair("s", 200) );
        cdata.sendObj(action);
    }
    c->Save();
    return HC_SUCCESS;
}

//军团探索列表
int ProcessCorpsExplreList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(pc->m_id).get();
    if (c == NULL)
    {
        return HC_ERROR;
    }

    robj.push_back( Pair("refresh", c->refresh_time - time(NULL)) );
    robj.push_back( Pair("gold", iCorpsExploreRefreshGold) );
    robj.push_back( Pair("explore_ling", pc->m_bag.getGemCount(treasure_type_explore_ling)) );

    int nums = pc->queryExtraData(char_data_type_daily, char_data_daily_corps_explore);
    int corps_explore_times = 0;
    splsCorps* cp = corpsMgr::getInstance()->findCorps(pc->m_corps_member.get()->corps);
    if (cp)
    {
        corps_explore_times = iCorpsExploreTimesOneday[cp->_level];
    }
    int left = corps_explore_times - nums;
    if (left < 0)
    {
        left = 0;
    }
    robj.push_back( Pair("left", left) );
    json_spirit::Object special;
    special.push_back( Pair("type", c->special) );
    special.push_back( Pair("name", c->special_name) );
    robj.push_back( Pair("special", special) );

    json_spirit::Array list;
    for (int i = 0; i < 3; ++i)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("pos", i+1) );
        obj.push_back( Pair("type", c->que[i].type) );
        obj.push_back( Pair("quality", c->que[i].quality) );
        obj.push_back( Pair("name", c->que[i].name) );

        if (c->que[i].state == 1)
        {
            int left_secs = c->que[i].done_time - time(NULL);
            if (left_secs <= 0)
            {
                obj.push_back( Pair("state", 2) );
            }
            else
            {
                obj.push_back( Pair("state", 1) );
                obj.push_back( Pair("time", left_secs) );
                int gold = left_secs / 60;
                if (gold % 60 > 0)
                {
                    ++gold;
                }
                obj.push_back( Pair("gold", gold) );
            }
        }
        else
        {
            obj.push_back( Pair("state", c->que[i].state) );
            obj.push_back( Pair("time", c->que[i].need_secs) );
        }
        //军粮，军团经验
        int supply = pc->m_level * pc->m_level * c->que[i].fac / 80;
        if (supply == 0)
        {
            supply = 1;
        }
#ifdef SF_SERVER
        if (supply > 0)
            supply *= 20;
#else
        //军粮数值放大10倍
        if (supply > 0)
            supply *= 10;
#endif

        int exp = 30 * c->que[i].fac;

        //军团探索收益系数
        corpsRealReward(supply);
        corpsRealReward(exp);

        if (c->special == c->que[i].type)
        {
            obj.push_back( Pair("double", 1) );
        }
        obj.push_back( Pair("supply", supply) );
        obj.push_back( Pair("exp", exp) );

        list.push_back(obj);
    }

    robj.push_back( Pair("list", list) );

    return HC_SUCCESS;
}

//军团探索领取
int ProcessCorpsExploreAccept(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(pc->m_id).get();
    if (c == NULL)
    {
        return HC_ERROR;
    }

    int pos = 1;
    READ_INT_FROM_MOBJ(pos,o,"pos");

    if (pos < 1 || pos > 3)
    {
        return HC_ERROR;
    }

    if (c->que[pos-1].state != 0)
    {
        return HC_ERROR;
    }

    c->que[pos-1].state = 1;
    c->que[pos-1].done_time = time(NULL) + c->que[pos-1].need_secs;
    c->que[pos-1].start(pc->m_id);
    c->Save();

    return HC_SUCCESS;
}

//军团探索放弃
int ProcessCorpsExploreAbandon(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(pc->m_id).get();
    if (c == NULL)
    {
        return HC_ERROR;
    }

    int pos = 1;
    READ_INT_FROM_MOBJ(pos,o,"pos");

    if (pos < 1 || pos > 3)
    {
        return HC_ERROR;
    }

    if (c->que[pos-1].state == 0)
    {
        return HC_ERROR;
    }

    if (c->que[pos-1].isDone())
    {
        return HC_ERROR;
    }

    c->que[pos-1].state = 0;
    c->Save();

    return HC_SUCCESS;
}

//军团探索刷新
int ProcessCorpsExploreRefresh(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(pc->m_id).get();
    if (c == NULL)
    {
        return HC_ERROR;
    }

    bool canrefresh = false;
    for (int i = 0; i < 3; ++i)
    {
        if (c->que[i].state == 0)
        {
            canrefresh = true;
            break;
        }
    }
    if (!canrefresh)
    {
        return HC_SUCCESS;
    }

    //优先扣道具
    if (pc->addTreasure(treasure_type_explore_ling, -1) >= 0)
    {
        //通知道具消耗
        std::string msg = treasure_expend_msg(treasure_type_explore_ling, 1);
        if (msg != "")
        {
            robj.push_back( Pair("msg", msg) );
        }
        //统计道具消耗
        add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_explore_ling,1,treasure_unknow,2,pc->m_union_id,pc->m_server_id);

    }
    else if (pc->addGold(-iCorpsExploreRefreshGold) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }

    //金币消耗统计
    add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, iCorpsExploreRefreshGold, gold_cost_for_corps_explore_refresh, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(pc,iCorpsExploreRefreshGold,gold_cost_for_corps_explore_refresh);
#endif

    c->refresh(0);

    pc->NotifyCharData();

    return HC_SUCCESS;
}

//军团探索完成
int ProcessCorpsExploreDone(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_corps_member.get() == NULL)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
    int nums = pc->queryExtraData(char_data_type_daily, char_data_daily_corps_explore);
    int corps_explore_times = 0;
    splsCorps* cp = corpsMgr::getInstance()->findCorps(pc->m_corps_member.get()->corps);
    if (cp)
    {
        corps_explore_times = iCorpsExploreTimesOneday[cp->_level];
    }
    int left = corps_explore_times - nums;
    if (left <= 0)
    {
        return HC_ERROR_CORPS_EXPLORE_NO_TIMES;
    }

    charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(pc->m_id).get();
    if (c == NULL)
    {
        return HC_ERROR;
    }

    int pos = 1;
    READ_INT_FROM_MOBJ(pos,o,"pos");

    if (pos < 1 || pos > 3)
    {
        return HC_ERROR;
    }

    if (c->que[pos-1].state == 0)
    {
        return HC_ERROR;
    }

    if (!c->que[pos-1].isDone())
    {
        return HC_ERROR;
    }

    c->que[pos-1].state = 0;
    //军粮，军团经验
    int supply = pc->m_level * pc->m_level * c->que[pos-1].fac / 80;
    if (supply <= 0)
    {
        supply = 1;
    }

    int exp = 30 * c->que[pos-1].fac;
    if (c->special == c->que[pos-1].type)
    {
        supply *= 2;
        exp *= 2;
    }

#ifdef SF_SERVER
            if (supply > 0)
                supply *= 20;
#else
            //军粮数值放大10倍
            if (supply > 0)
                supply *= 10;
#endif


    //军团实际收益
    corpsRealReward(supply);
    corpsRealReward(exp);

    robj.push_back( Pair("supply", supply) );
    robj.push_back( Pair("exp", exp) );

    pc->addTreasure(treasure_type_supply, supply);
    add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_supply,supply,treasure_corps_explore,1,pc->m_union_id,pc->m_server_id);

    pc->NotifyCharData();

    corpsMgr::getInstance()->addEvent(pc, corps_event_explore, c->que[pos-1].type + c->que[pos-1].quality * 10, exp);

    c->refresh(pos);

    ++nums;
    pc->setExtraData(char_data_type_daily, char_data_daily_corps_explore, nums);
    //act统计
    act_to_tencent(pc,act_new_corps_explore);

    if (nums == 10)
    {
        //第十次
        splsCorps* corps = corpsMgr::getInstance()->findCorps(pc->m_corps_member->corps);
        if (corps && corps->_level >= 2)
        {
            # if 0
            int err_code = 0;
            pc->m_bag.addGem(treasure_type_corps_lottery, 1, err_code, true);
            robj.push_back( Pair("getGem", 1) );
            #endif
            //奖励暂存
            std::list<Item> tmp_list;
            {
                Item item_p;
                item_p.type = item_type_treasure;
                item_p.id = treasure_type_corps_lottery;
                item_p.nums = 1;
                //军团实际收益
                corpsRealReward(item_p.nums);
                tmp_list.push_back(item_p);
            }
            Singleton<char_rewards_mgr>::Instance().updateCharRewards(pc->m_id,rewards_type_explore,0,tmp_list);
        }
    }
    //日常任务
    dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_corp_explore);

    //支线任务
    pc->m_trunk_tasks.updateTask(task_corps_explore, 1);

    //军团活动按钮
    json_spirit::Object action;
    action.push_back( Pair("cmd", "updateAction") );
    action.push_back( Pair("type", top_level_event_corp) );
    int state = 0;
    if (pc->m_corps_member->ymsj_can_get)
    {
        state = 1;
    }
    if (c->isDone() && corps_explore_times > nums)
    {
        state = 1;
    }
    action.push_back( Pair("active", state) );
    action.push_back( Pair("s", 200) );
    pc->sendObj(action);
    return HC_SUCCESS;
}

//军团探索完成
int ProcessCorpsExploreFinish(json_spirit::mObject& o)
{
    int cid = 0, pos = 0;
    READ_INT_FROM_MOBJ(cid,o,"cid");
    READ_INT_FROM_MOBJ(pos,o,"pos");
    charCorpsExplore* c = Singleton<corpsExplore>::Instance().getChar(cid).get();
    if (c && pos >= 1 && pos <= 3 && c->que[pos-1].state)
    {
        //军团活动按钮闪动
        json_spirit::Object action;
        action.push_back( Pair("cmd", "updateAction") );
        action.push_back( Pair("type", top_level_event_corp) );
        action.push_back( Pair("active", 1) );
        action.push_back( Pair("s", 200) );
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (cdata.get() && cdata->m_corps_member.get())
        {
            int nums = cdata->queryExtraData(char_data_type_daily, char_data_daily_corps_explore);
            int corps_explore_times = 0;
            splsCorps* cp = corpsMgr::getInstance()->findCorps(cdata->m_corps_member.get()->corps);
            if (cp)
            {
                corps_explore_times = iCorpsExploreTimesOneday[cp->_level];
            }
            if (corps_explore_times > nums)
            {
                cdata->sendObj(action);
            }
        }
    }
    return HC_SUCCESS;
}

