
#include "corpsLottery.h"

#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>

#include "loot.h"
#include "utils_all.h"
#include "data.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include <list>
#include "statistics.h"
#include "combat.h"
#include "daily_task.h"
#include "singleton.h"
#include "libao.h"
#include "spls_timer.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

int giveLoots(CharData* cdata, Item& getItem, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

extern std::string strCorpsLotteryGet;
extern std::string strCorpsLotteryMsg;

corpsLottery::corpsLottery(int id)
{
    total_count = 0;
    total_score = 0;
    need_save = false;
    corps_id = id;

    Query q(GetDb());

    q.get_result("select count,score,notices from char_corps_lottery_data where id=" + LEX_CAST_STR(id));
    if (q.fetch_row())
    {
        total_count = q.getval();
        total_score = q.getval();
        std::string lottery_notices = q.getstr();
        q.free_result();
        json_spirit::read(lottery_notices, m_notices_value);
    }
    else
    {
        std::string lottery_notices = "[]";
        q.free_result();
        json_spirit::read(lottery_notices, m_notices_value);
    }
}

//查询抽奖公告
int corpsLottery::queryLotteryNotice(json_spirit::Object& robj)
{
    robj.push_back( Pair("list", m_notices_value.get_array()) );
    robj.push_back( Pair("score", total_score) );
    return HC_SUCCESS;
}

//增加记录
void corpsLottery::addLotteryNotice(const std::string& name, Item& item)
{
    std::string what = strCorpsLotteryGet;
    str_replace(what, "$W", item.toString(true));

    json_spirit::Array& notice_array = m_notices_value.get_array();
    json_spirit::Object obj;
    obj.push_back( Pair("name", name) );
    obj.push_back( Pair("get", what) );
    notice_array.push_back(obj);
    while ((int)notice_array.size() > 20)
    {
        notice_array.erase(notice_array.begin());
    }

    need_save = true;    
}

//通告获得
void corpsLottery::broadLotteryNotice(const std::string& name, Item& item)
{
    std::string msg = strCorpsLotteryMsg;
    str_replace(msg, "$N", MakeCharNameLink(name));
    std::string reward_msg = "";
    if (item.type == item_type_baoshi)
    {
        baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(item.id);
        if (bbs)
        {
            reward_msg = bbs->Name_to_Link(item.fac);
        }
    }
    else if (item.type == item_type_silver || item.type == item_type_gold)
    {
        reward_msg = item.toString(true);
    }
    else
    {
        return;
    }
    if (msg != "")
    {
        str_replace(msg, "$M", reward_msg);
        //GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        json_spirit::mObject mobj;
        mobj["cmd"] = "broadCastMsg";
        mobj["msg"] = msg;

        boost::shared_ptr<splsTimer> tmsg;
           tmsg.reset(new splsTimer(4, 1, mobj,1));
           splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return;
}

int corpsLottery::addCount()
{
    ++total_count;
    need_save = true;
}

int corpsLottery::addScore(int a)
{
    total_score += a;
    need_save = true;
}

void corpsLottery::Save()
{
    if (need_save)
    {
        //保存
        InsertSaveDb("replace into char_corps_lottery_data (id,count,score,notices) values ("
            + LEX_CAST_STR(corps_id) + ","
            + LEX_CAST_STR(total_count) + ","
            + LEX_CAST_STR(total_score) + ",'"
            + GetDb().safestr(json_spirit::write(m_notices_value)) + "')");
        need_save = false;
    }
}

//重置
void corpsLottery::Reset()
{
}

corpsLotteryMgr::corpsLotteryMgr()
{
    reload();
}

void corpsLotteryMgr::reload()
{
    Query q(GetDb());
    q.get_result("select pos,itemType,itemId,count,fac,gailv,notice from base_corps_lottery_awards where 1 order by pos");
    while (q.fetch_row())
    {
        int pos = q.getval();
        assert(pos == m_awards.size() + 1);
        Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();
        item.fac = q.getval();
        item.spic = item.id;
        //礼包图片特殊处理
        if (item.type == item_type_libao)
        {
            baseLibao* p = libao_mgr::getInstance()->getBaselibao(item.id);
            if (p)
            {
                item.spic = p->m_spic;
            }
        }

        m_gailvs.push_back(q.getval());
        m_awards.push_back(item);
        m_need_notice.push_back(q.getval());
    }
    q.free_result();
}

//可能获得奖励列表
void corpsLotteryMgr::getAwards(json_spirit::Array& list)
{
    for (std::vector<Item>::iterator it = m_awards.begin(); it != m_awards.end(); ++it)
    {
        Item& item = *it;
        json_spirit::Object obj;
        item.toObj(obj);
        list.push_back(obj);
    }
}

//随机物品
Item corpsLotteryMgr::random_award(int& add_notice, int& pos)
{
    add_notice = 0;
    pos = 1;
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    boost::random::discrete_distribution<> dist(m_gailvs);

    int idx = dist(gen);
    if (idx >= 0 && idx < m_awards.size())
    {
        add_notice = m_need_notice[idx];
        pos = idx + 1;
        return m_awards[idx];
    }
    //不可能出现的异常
    ERR();
    add_notice = m_need_notice[0];
    return m_awards[0];
}

//查询军团梅花易数公告
int ProcessQueryCorpsLotteryNotice(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    
    splsCorps* corps = corpsMgr::getInstance()->findCorps(pc->m_corps_member->corps);
    if (NULL == corps)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }

    if (NULL == corps->m_corpsLottery.get())
    {
        corps->m_corpsLottery.reset(new corpsLottery(corps->_id));
    }

    return corps->m_corpsLottery->queryLotteryNotice(robj);
}

//查询军团梅花易数奖励物品列表
int ProcessQueryCorpsLotteryAwards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    json_spirit::Array list;
    Singleton<corpsLotteryMgr>::Instance().getAwards(list);
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//梅花易数抽取奖品
int ProcessCorpsLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    
    splsCorps* corps = corpsMgr::getInstance()->findCorps(pc->m_corps_member->corps);
    if (NULL == corps)
    {
        return HC_ERROR_NOT_JOIN_JT;
    }

    if (corps->_level < 2)
    {
        return HC_ERROR;
    }

    if (pc->m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }

    int tr_counts = pc->treasureCount(treasure_type_corps_lottery);
    if (tr_counts < 1)
    {
        return HC_ERROR;
    }

    int err = 0;
    pc->addTreasure(treasure_type_corps_lottery, -1, err);
#ifdef QQ_PLAT
    treasure_cost_tencent(pc,treasure_type_corps_lottery,1);
#endif
    if (NULL == corps->m_corpsLottery.get())
    {
        corps->m_corpsLottery.reset(new corpsLottery(corps->_id));
    }

    int need_notice = 0, pos = 1;
    Item item = Singleton<corpsLotteryMgr>::Instance().random_award(need_notice, pos);
    if (need_notice)
    {
        corps->m_corpsLottery->addLotteryNotice(pc->m_name, item);
        corps->m_corpsLottery->broadLotteryNotice(pc->m_name, item);
    }
    corps->m_corpsLottery->addScore(1);
    corps->m_corpsLottery->Save();

    giveLoots(pc, item, 0, pc->m_level, 0, NULL, NULL, false, give_lottery);

    robj.push_back( Pair("pos", pos) );
    robj.push_back( Pair("daoju", pc->treasureCount(treasure_type_corps_lottery)) );
    //日常任务
    dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_corp_lottery);
    //act统计
    act_to_tencent(pc,act_new_corps_lottery);
    return HC_SUCCESS;
}

//购买道具
int ProcessBuyDaoju(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int tid = 0;
    READ_INT_FROM_MOBJ(tid,o,"id");
    robj.push_back( Pair("id", tid) );
    
    boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(tid);
    if (!bt.get())
    {
        //cout<<"error id"<<endl;
        return HC_ERROR;
    }
    if (bt->gold_to_buy <= 0)
    {
        //cout<<"can not buy "<<tid<<endl;
        return HC_ERROR;
    }

    int count = 1;
    READ_INT_FROM_MOBJ(count,o,"nums");
    if (count < 1)
    {
        count = 1;
    }

    if (pc->m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }

    if (bt->max_size > 0)
    {
        int left = pc->m_bag.size() - pc->m_bag.getUsed();
        left *= bt->max_size;
        if (left < count)
        {
            count = left;
        }
    }
    else if (count > 10000)
    {
        count = 10000;
    }

    int gold = count * bt->gold_to_buy;
    if (pc->addGold(-gold) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }

    //金币消耗统计
    add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, gold, gold_cost_for_treasure+tid, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
    gold_cost_tencent(pc,bt->gold_to_buy,gold_cost_for_buy_daoju,tid,count);
#endif

    //给道具
    int err_code = 0;
    pc->m_bag.addGem(tid, count,err_code);

    pc->NotifyCharData();

    return HC_SUCCESS;
}

