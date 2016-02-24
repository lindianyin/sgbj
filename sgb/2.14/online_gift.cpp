
#include "online_gift.h"
#include "data.h"
#include "statistics.h"
#include "spls_errcode.h"
#include "spls_timer.h"

using namespace net;

class Combat;
int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

online_gift_mgr* online_gift_mgr::m_handle = NULL;

online_gift_mgr* online_gift_mgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new online_gift_mgr();
        m_handle->load();
    }
    return m_handle;
}

#ifdef TEST_SERVER
    static int online_gift_mins[] = {0,2,3,4,5};
#else
    #ifdef VN_SERVER
    static int online_gift_mins[] = {2,8,10,30,60};
    #else
    static int online_gift_mins[] = {0,5,10,30,60};
    #endif
#endif

void online_gift_mgr::load()
{
    m_online_gift_count = sizeof(online_gift_mins)/sizeof(int);
#ifdef VN_SERVER
/*
1st Day    2 phút 50 vàng (2分钟 50金币)        　
　    　    8 phút 30.000 b?c (8分钟 30.000银币)        　
　    　    10 phút 80.000 b?c (10分钟 80.000银币)        　
　    　    30 phút 10 Quan l?nh (30分钟  10军令)        　
　    　    60 phút 35 Quan l?nh (60分钟  35军令)        　
*/
    boost::shared_ptr<online_gift> gift1(new online_gift(1,online_gift_mins[0]));
    Item item1;
    item1.type = item_type_gold;
    item1.nums = 50;
    gift1->_rewards.push_back(item1);
    m_online_gifts.push_back(gift1);

    boost::shared_ptr<online_gift> gift2(new online_gift(2,online_gift_mins[1]));
    Item item2;
    item2.type = item_type_silver;
    item2.nums = 30000;
    gift2->_rewards.push_back(item2);
    m_online_gifts.push_back(gift2);

    boost::shared_ptr<online_gift> gift3(new online_gift(3,online_gift_mins[2]));
    Item item3;
    item3.type = item_type_silver;
    item3.nums = 80000;
    gift3->_rewards.push_back(item3);
    m_online_gifts.push_back(gift3);

    boost::shared_ptr<online_gift> gift4(new online_gift(4,online_gift_mins[3]));
    Item item4;
    item4.type = item_type_ling;
    item4.nums = 10 * 4;
    gift4->_rewards.push_back(item4);
    m_online_gifts.push_back(gift4);

    boost::shared_ptr<online_gift> gift5(new online_gift(5,online_gift_mins[4]));
    Item item5;
    item5.type = item_type_ling;
    item5.nums = 35 * 4;
    gift5->_rewards.push_back(item5);
    m_online_gifts.push_back(gift5);
/*
2nd Day    2 phút 200 vàng (2分钟 200金币)        　
　    　    8 phút 100.000 b?c (8分钟 100.000银币)        　
　    　    10 phút 200.000 b?c (10分钟 200.000银币)        　
　    　    30 phút 50 Quan l?nh (30分钟  55军令)        　
　    　    60 phút 100 Quan l?nh (60分钟  110军令)        　
*/
    {
        boost::shared_ptr<online_gift> gift(new online_gift(1,online_gift_mins[0]));
        Item item;
        item.type = item_type_gold;
        item.nums = 200;
        gift->_rewards.push_back(item);
        m_online_gifts.push_back(gift);
    }
    {
        boost::shared_ptr<online_gift> gift(new online_gift(2,online_gift_mins[1]));
        Item item;
        item.type = item_type_silver;
        item.nums = 100000;
        gift->_rewards.push_back(item);
        m_online_gifts.push_back(gift);
    }
    {
        boost::shared_ptr<online_gift> gift(new online_gift(3,online_gift_mins[2]));
        Item item;
        item.type = item_type_silver;
        item.nums = 200000;
        gift->_rewards.push_back(item);
        m_online_gifts.push_back(gift);
    }
    {
        boost::shared_ptr<online_gift> gift(new online_gift(4,online_gift_mins[3]));
        Item item;
        item.type = item_type_ling;
        item.nums = 55 * 4;
        gift->_rewards.push_back(item);
        m_online_gifts.push_back(gift);
    }
    {
        boost::shared_ptr<online_gift> gift(new online_gift(5,online_gift_mins[4]));
        Item item;
        item.type = item_type_ling;
        item.nums = 110 * 4;
        gift->_rewards.push_back(item);
        m_online_gifts.push_back(gift);
    }
/*
3rd Day    2 phút 250 vàng (2分钟 250金币)        　
　    　    8 phút 200.000 b?c (8分钟 200.000银币)        　
　    　    10 phút 300.000 b?c  (10分钟 300.000银币)        　
　    　    30 phút 50 Quan l?nh (30分钟  55军令)        　
　    　    60 phút 150 Quan l?nh (60分钟  160军令)
*/
    {
        boost::shared_ptr<online_gift> gift(new online_gift(1,online_gift_mins[0]));
        Item item;
        item.type = item_type_gold;
        item.nums = 250;
        gift->_rewards.push_back(item);
        m_online_gifts.push_back(gift);
    }
    {
        boost::shared_ptr<online_gift> gift(new online_gift(2,online_gift_mins[1]));
        Item item;
        item.type = item_type_silver;
        item.nums = 200000;
        gift->_rewards.push_back(item);
        m_online_gifts.push_back(gift);
    }
    {
        boost::shared_ptr<online_gift> gift(new online_gift(3,online_gift_mins[2]));
        Item item;
        item.type = item_type_silver;
        item.nums = 300000;
        gift->_rewards.push_back(item);
        m_online_gifts.push_back(gift);
    }
    {
        boost::shared_ptr<online_gift> gift(new online_gift(4,online_gift_mins[3]));
        Item item;
        item.type = item_type_ling;
        item.nums = 55 * 4;
        gift->_rewards.push_back(item);
        m_online_gifts.push_back(gift);
    }
    {
        boost::shared_ptr<online_gift> gift(new online_gift(5,online_gift_mins[4]));
        Item item;
        item.type = item_type_ling;
        item.nums = 160 * 4;
        gift->_rewards.push_back(item);
        m_online_gifts.push_back(gift);
    }
#else
    /*
    连续在线时间    奖励物品
    0分钟    30金币
    8分钟10000银币
    10分钟20000银币
    30分钟5军令
    1小时    10军令
    */

    boost::shared_ptr<online_gift> gift1(new online_gift(1,online_gift_mins[0]));
    Item item1;
    item1.type = item_type_gold;
    item1.nums = 30;
    gift1->_rewards.push_back(item1);
    m_online_gifts.push_back(gift1);

    boost::shared_ptr<online_gift> gift2(new online_gift(2,online_gift_mins[1]));
    Item item2;
    item2.type = item_type_silver;
    item2.nums = 10000;
    gift2->_rewards.push_back(item2);
    m_online_gifts.push_back(gift2);

    boost::shared_ptr<online_gift> gift3(new online_gift(3,online_gift_mins[2]));
    Item item3;
    item3.type = item_type_silver;
    item3.nums = 20000;
    gift3->_rewards.push_back(item3);
    m_online_gifts.push_back(gift3);

    boost::shared_ptr<online_gift> gift4(new online_gift(4,online_gift_mins[3]));
    Item item4;
    item4.type = item_type_ling;
    item4.nums = 5 * 4;
    gift4->_rewards.push_back(item4);
    m_online_gifts.push_back(gift4);

    boost::shared_ptr<online_gift> gift5(new online_gift(5,online_gift_mins[4]));
    Item item5;
    item5.type = item_type_ling;
    item5.nums = 10 * 4;
    gift5->_rewards.push_back(item5);
    m_online_gifts.push_back(gift5);
#endif
}

boost::shared_ptr<char_online_gift> online_gift_mgr::getChar(int cid)
{
    std::map<int, boost::shared_ptr<char_online_gift> >::iterator it = m_char_datas.find(cid);
    if (it != m_char_datas.end())
    {
        return it->second;
    }
    boost::shared_ptr<char_online_gift> cog(new char_online_gift(cid));
    cog->load();
    m_char_datas[cid] = cog;
    return cog;
}

boost::shared_ptr<online_gift> online_gift_mgr::getGift(int day, int id)
{
#ifdef VN_SERVER
    if (id > m_online_gift_count)
    {
        boost::shared_ptr<online_gift> tmp;
        return tmp;
    }
    id = 5 * (day - 1)    + id;
#endif
    if (id > 0 && id <= (int)m_online_gifts.size())
    {
        return m_online_gifts[id-1];
    }
    else
    {
        boost::shared_ptr<online_gift> tmp;
        return tmp;
    }
}

void online_gift_mgr::charLogout(int cid)
{
    boost::shared_ptr<char_online_gift> cog = getChar(cid);
    if (cog.get())
    {
        cog->del_timer();
    }
}

void online_gift_mgr::on_timer(int cid)
{
    boost::shared_ptr<char_online_gift> cog = getChar(cid);
    if (cog.get())
    {
        cog->_uuid = boost::uuids::nil_uuid();
        cog->_canGet = 1;
        //save
        cog->_cdata->setExtraData(char_data_type_daily, char_data_online_gift_state, 1);

        if (cog->_cdata.get())
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cog->_cdata->m_name);
            if (account.get())
            {
                json_spirit::Object o;
                o.push_back( Pair("cmd", "queryOlGift") );
                o.push_back( Pair("s", 200) );
                o.push_back( Pair("leftSecs", 0) );
                if (cog->_online_gift.get())
                {
                    o.push_back( Pair("id", cog->_online_gift->_id) );
                }
                account->Send(json_spirit::write(o));
            }
        }
    }
}

void online_gift_mgr::resetAll()
{
    //删除定时器
    for (std::map<int, boost::shared_ptr<char_online_gift> >::iterator it = m_char_datas.begin(); it != m_char_datas.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->del_timer();
        }
    }
    m_char_datas.clear();
}

bool char_online_gift::del_timer()
{
    bool delete_success = true;
    if (!_uuid.is_nil())
    {
        delete_success = splsTimerMgr::getInstance()->delTimer(_uuid);
        _uuid = boost::uuids::nil_uuid();
    }
    return delete_success;
}

void char_online_gift::reset()
{
    _start_time = time(NULL);

    if (del_timer() && _online_gift.get())
    {
        json_spirit::mObject mobj;
        mobj["cmd"] = "online_gift";
        mobj["cid"] = _cid;
        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(_online_gift->_need_secs, 1, mobj, 1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
}

void char_online_gift::load()
{
    _cdata = GeneralDataMgr::getInstance()->GetCharData(_cid);
    if (!_cdata.get())
    {
        return;
    }
    CharData* pc = _cdata.get();
    int gift = pc->queryExtraData(char_data_type_daily, char_data_online_gift);
    if (0 == gift)
    {
        gift = 1;
    }
    int gift_state = pc->queryExtraData(char_data_type_daily, char_data_online_gift_state);
    //创建号少于三天，而且领取天数少于三天
    time_t olgift_end = pc->queryExtraData(char_data_type_normal, char_data_get_onlinegift_day);
    time_t time_now = time(NULL);
    if (pc->m_currentStronghold >= iOnlineGiftStronghold && time_now < olgift_end)
    {
        int day = 3 - (olgift_end - time_now) / iONE_DAY_SECS;
        _online_gift = online_gift_mgr::getInstance()->getGift(day, gift);
    }

    _canGet = gift_state > 0;
    _start_time = time(NULL);
}

void char_online_gift::add2obj(const std::string& f, json_spirit::Object& robj)
{
    json_spirit::Array a;
    if (_cdata.get() && _online_gift.get())
    {
        for (std::list<Item>::iterator it = _online_gift->_rewards.begin(); it != _online_gift->_rewards.end(); ++it)
        {
            json_spirit::Object o;
            Item item = *it;
            
            switch (it->type)
            {
                case item_type_stone:
                    item.type = item_type_treasure;
                    if (_cdata->m_level <= 39)
                    {
                        item.id = 1;
                    }
                    else if (_cdata->m_level <= 59)
                    {
                        item.id = 2;
                    }
                    else if (_cdata->m_level <= 79)
                    {
                        item.id = 3;
                    }
                    else if (_cdata->m_level <= 99)
                    {
                        item.id = 4;
                    }
                    else
                    {
                        item.id = 5;
                    }
                    break;
                case item_type_silver_level:
                    item.type = item_type_silver;
                    item.nums = item.nums * _cdata->m_level;
                    item.fac = 4;
                    break;
            }

            item.toObj(o);
            a.push_back(o);
        }

        robj.push_back( Pair(f, a) );
    }
}

//查询在线礼包信息 /{"cmd":"queryOlGift"}
int ProcessQueryOnlineGift(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //创建号少于三天，而且领取天数少于三天
    time_t olgift_end = cdata->queryExtraData(char_data_type_normal, char_data_get_onlinegift_day);
    if (cdata->m_currentStronghold >= iOnlineGiftStronghold && time(NULL) < olgift_end)
    {
        ;
    }
    else
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_online_gift> cog = online_gift_mgr::getInstance()->getChar(cdata->m_id);
    if (cog.get() && cog->_online_gift.get())
    {
        if (cog->_canGet)
        {
            robj.push_back( Pair("leftSecs", 0) );
        }
        else
        {
            int leftSecs = (cog->_online_gift->_need_secs  - time(NULL) + cog->_start_time);
            if (leftSecs <= 0)
            {
                leftSecs = 1;
                //cog->_canGet = 1;

                //save
                //cog->_cdata->setExtraData(char_data_type_daily, char_data_online_gift_state, 1);
            }
            robj.push_back( Pair("leftSecs", leftSecs) );
        }
        
        robj.push_back( Pair("id", cog->_online_gift->_id) );
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

//领取在线礼包 /{"cmd":"openOlGift"}
int ProcessGetOnlineGift(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }

    time_t time_now = time(NULL);
    //创建号少于三天，而且领取天数少于三天
    time_t olgift_end = cdata->queryExtraData(char_data_type_normal, char_data_get_onlinegift_day);
    if (cdata->m_currentStronghold >= iOnlineGiftStronghold && time_now < olgift_end)
    {
        ;
    }
    else
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_online_gift> cog = online_gift_mgr::getInstance()->getChar(cdata->m_id);
    if (cog.get() && cog->_online_gift.get() && cog->_cdata.get())
    {
        if (!cog->_canGet)
        {
            return HC_ERROR;
        }

        int old_recharge = cog->_cdata->m_total_recharge + cog->_cdata->m_vip_exp;
        //给东西
        cog->add2obj("list", robj);
        std::list<Item> items = cog->_online_gift->_rewards;
        giveLoots(cog->_cdata.get(), items, 0, cog->_cdata->m_level, 0, NULL, NULL, true, give_online_gift);
        int cur_recharge = cog->_cdata->m_total_recharge + cog->_cdata->m_vip_exp;

        int day = 3 - (olgift_end - time_now) / iONE_DAY_SECS;

        //通知客户端，充值条变化
        if (old_recharge != cur_recharge)
        {            
            robj.push_back( Pair("day", day) );
            robj.push_back( Pair("rechargeFrom", old_recharge) );
            robj.push_back( Pair("rechargeAdd", cur_recharge - old_recharge) );
        }
        //act统计
        act_to_tencent(cdata,act_new_online_reward,cog->_online_gift->_id);
        //通知信息变化
        cdata->NotifyCharData();
        //下一个礼包
        cog->_canGet = 0;
        //cog->_start_time = time(NULL);

        cog->_cdata->setExtraData(char_data_type_daily, char_data_online_gift, cog->_online_gift->_id + 1);
        cog->_cdata->setExtraData(char_data_type_daily, char_data_online_gift_state, 0);

        cog->_online_gift = online_gift_mgr::getInstance()->getGift(day, cog->_online_gift->_id + 1);

        //开始定时
        cog->reset();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

