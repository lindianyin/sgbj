
#ifdef QQ_PLAT

#include "qq_invite.h"

#include "singleton.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "statistics.h"
#include <boost/random/discrete_distribution.hpp>
#include "ThreadLocalSingleton.h"

#include "combat.h"
#include "SaveDb.h"

int getSessionChar(net::session_ptr& psession, boost::shared_ptr<CharData>& cdata);
Database& GetDb();
int giveLoots(CharData* cdata, Item& getItem, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

extern std::string strInviteLotteryGet;

//查询好友邀请 cmd：QueryInvite
int ProcessQueryInvite(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(pc->m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();

    robj.push_back( Pair("invite", c->getInviteState()) );
    robj.push_back( Pair("cfriend", c->getCloseFriendState()) );
    robj.push_back( Pair("share", c->getShareState()) );
    robj.push_back( Pair("recall", c->get_recall()) );

    int view = pc->queryExtraData(char_data_type_daily, char_data_daily_view_invite);
    if (view == 0)
    {
        cdata->setExtraData(char_data_type_daily, char_data_daily_view_invite, 1);
        int state = c->getCanGet();
        if (0 == state)
        {
            cdata->notifyEventState(top_level_event_invite, 0, 0);
        }
    }
    return HC_SUCCESS;
}

//查询好友邀请信息 cmd：QueryInviteInfo
int ProcessQueryInviteInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(pc->m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();

    json_spirit::Array invite_list;
    json_spirit::Array invited_list;
    Singleton<inviteMgr>::Instance().get_invite_list(*c, invite_list, invited_list);

    json_spirit::Object invite;
    invite.push_back( Pair("count", c->get_invite()) );
    invite.push_back( Pair("list", invite_list) );
    robj.push_back( Pair("invite", invite) );

    json_spirit::Object invited;
    invited.push_back( Pair("count", c->get_invited()) );
    invited.push_back( Pair("list", invited_list) );
    robj.push_back( Pair("invited", invited) );

    return HC_SUCCESS;
}

//查询游戏分享 cmd：queryShareInfo（分享成功就给奖励）
int ProcessQueryShareInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(pc->m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();

    json_spirit::Array list;
    Singleton<inviteMgr>::Instance().get_share_list(*c, list);
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//查询好友召回 cmd：queryRecallInfo
int ProcessQueryRecallInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(pc->m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();

    robj.push_back( Pair("get", c->get_recall()) );
    return HC_SUCCESS;
}

//查询密友奖励 cmd：queryCloseFriendInfo
int ProcessQueryCloseFriendInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(pc->m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();
    robj.push_back( Pair("count", c->get_close_friend_count()) );
    json_spirit::Array list;
    Singleton<inviteMgr>::Instance().get_close_friend_list(*c, list);
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

//分享游戏内容 cmd：doShare，id：分享类型
int ProcessDoShare(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(pc->m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();

    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    if (c->share_get(id) == 0)
    {
        c->set_share_get(id, 1);
        c->save_share(id);
        //分享成功给奖励
        Singleton<inviteMgr>::Instance().getShareAward(*pc, id, robj);

        int state = c->getCanGet();
        pc->notifyEventState(top_level_event_invite, state, 0);

        //act_to_tencent(pc, act_click_invite_share, id, 0);
        return HC_SUCCESS;
    }
    return HC_ERROR;    
}

//查询邀请抽奖 cmd：queryInviteLottery
int ProcessQueryInviteLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(pc->m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();

    robj.push_back( Pair("count", pc->treasureCount(treasure_type_friend_lottery)) );

    json_spirit::Array list;
    Singleton<inviteMgr>::Instance().getAwards(list);
    robj.push_back( Pair("list", list) );

    return HC_SUCCESS;
}

//好友邀请抽奖公告 cmd：queryInviteLotteryNotice
int ProcessQueryInviteLotteryNotice(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    Singleton<inviteMgr>::Instance().queryLotteryNotice(robj);
    return HC_SUCCESS;
}

//邀请抽奖 cmd: inviteLottery
int ProcessInviteLottery(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (pc->m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }

    int tr_counts = pc->treasureCount(treasure_type_friend_lottery);
    if (tr_counts < 1)
    {
        return HC_ERROR;
    }

    int err = 0;
    pc->addTreasure(treasure_type_friend_lottery, -1, err);
#ifdef QQ_PLAT
    treasure_cost_tencent(pc,treasure_type_friend_lottery,1);
#endif
    
    int need_notice = 0, pos = 1;
    Item item = Singleton<inviteMgr>::Instance().random_award(need_notice, pos);
    if (need_notice)
    {
        Singleton<inviteMgr>::Instance().addLotteryNotice(pc->m_name, item);
        //corps->m_corpsLottery->broadLotteryNotice(pc->m_name, item);
    }

    giveLoots(pc, item, 0, pc->m_level, 0, NULL, NULL, false, give_invite);

    robj.push_back( Pair("pos", pos) );
    robj.push_back( Pair("daoju", pc->treasureCount(treasure_type_corps_lottery)) );

    //act_to_tencent(pc, act_invite_lottery, 0, 0);
    return HC_SUCCESS;
}

//邀请好友 cmd：doInvite，count：邀请好友个数
int ProcessDoInvite(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(pc->m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();

    int count = 1;
    READ_INT_FROM_MOBJ(count,o,"count");

    c->add_invite(count);

    return HC_SUCCESS;
}

char_invite::char_invite(inviteMgr& h)
:m_handle(h)
{
    m_cid = 0;
    m_recall = 0;
    m_invite = 0;
    m_invited = 0;
}

void char_invite::init(int c, int r, int inv, int inved)
{
    m_cid = c;
    m_recall = r;
    m_invite = inv;
    m_invited = inved;
}

void char_invite::add_invite(int count)
{
    if (count > 0)
    {
        bool need_save = false;
        for (int i = 1; i <= count; ++i)
        {
            ++m_invite;
            int id = m_handle.in_base_invite(m_invite);
            if (id > 0 && invite_get(id) == 0)
            {
                set_invite_get(id, 1);
                need_save = true;
            }
        }
        if (need_save)
        {
            save_invite();
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_cid).get();
            if (pc)
            {
                pc->notifyEventState(top_level_event_invite, 1, 0);
            }
        }
    }
}

void char_invite::save_share(int id)
{
    (void)id;
    std::string sql = "replace into char_share_event (cid,data) values (" + LEX_CAST_STR(m_cid) + ","
        + "'";
    for (size_t i = 0; i < m_share_geted.size(); ++i)
    {
        sql += LEX_CAST_STR(m_share_geted[i]);
        if ((i + 1) != m_share_geted.size())
        {
            sql += ",";
        }
    }
    sql += "')";
    InsertSaveDb(sql);
}

void char_invite::save_close_friend()
{
    std::string sql = "replace into char_close_friend (cid,getted) values (" + LEX_CAST_STR(m_cid) + ","
        + "'";
    for (size_t i = 0; i < m_close_friend_geted.size(); ++i)
    {
        sql += LEX_CAST_STR(m_close_friend_geted[i]);
        if ((i + 1) != m_close_friend_geted.size())
        {
            sql += ",";
        }
    }    
    sql += "')";
    InsertSaveDb(sql);
}

void char_invite::save_invite()
{
    std::string sql = "update char_invite_data set invite=" + LEX_CAST_STR(m_invite) + " where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

void char_invite::save_invite_get()
{
    std::string sql = "update char_invite_data set invite=" + LEX_CAST_STR(m_invite) + ",invite_get='";

    for (size_t i = 0; i < m_invite_geted.size(); ++i)
    {
        sql += LEX_CAST_STR(m_invite_geted[i]);
        if ((i + 1) != m_invite_geted.size())
        {
            sql += ",";
        }
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

void char_invite::save_invited()
{
    std::string sql = "update char_invite_data set invited=" + LEX_CAST_STR(m_invited) + " where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

void char_invite::save_invited_get()
{
    std::string sql = "update char_invite_data set invited=" + LEX_CAST_STR(m_invited) + ",invited_get='";

    for (size_t i = 0; i < m_invited_geted.size(); ++i)
    {
        sql += LEX_CAST_STR(m_invited_geted[i]);
        if ((i + 1) != m_invited_geted.size())
        {
            sql += ",";
        }
    }
    sql += "' where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

void char_invite::save_recall()
{
    std::string sql = "update char_invite_data set recall=" + LEX_CAST_STR(m_recall) + " where cid=" + LEX_CAST_STR(m_cid);
    InsertSaveDb(sql);
}

void char_invite::daily_reset()
{
    m_invite = 0;
    m_invited = 0;
    m_recall = 0;
    m_invited_geted.clear();
    m_invite_geted.clear();
}

int char_invite::close_friend_get(int id)
{
    if (id > 0 && id <= m_close_friend_geted.size())
    {
        return m_close_friend_geted[id-1];
    }
    return 0;
}

//-1未达成，0可以分享
int char_invite::share_get(int id)
{
    if (id > 0 && id <= m_share_geted.size())
    {
        return m_share_geted[id-1];
    }
    return -1;
}

int char_invite::invite_get(int id)
{
    if (id > 0 && id <= m_invite_geted.size())
    {
        return m_invite_geted[id-1];
    }
    return 0;
}

int char_invite::invited_get(int id)
{
    if (id > 0 && id <= m_invited_geted.size())
    {
        return m_invited_geted[id-1];
    }
    return 0;
}

void char_invite::add_close_friend(int id, int level)
{
    if (level > 0)
        m_close_friends[id] = level;
}

int char_invite::add_invited(int count)
{
    m_invited += count;
    return m_invited;
}

void char_invite::set_recall(int s)
{
    cout<<"set_recall:"<<m_recall<<"->"<<s<<endl;
    if ((m_recall == 0 && s == 1)
        || (m_recall == 1 && s == 2))
    {
        m_recall = s;
        save_recall();

        if (m_recall == 1)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_cid).get();
            if (pc)
            {
                pc->notifyEventState(top_level_event_invite, 1, 0);
            }
        }
    }
}

void char_invite::set_invite_get(int id, int s)
{
    while (id > m_invite_geted.size())
    {
        m_invite_geted.push_back(0);
    }
    m_invite_geted[id-1] = s;
    save_invite();
}

void char_invite::set_invited_get(int id, int s)
{
    while (id > m_invited_geted.size())
    {
        m_invited_geted.push_back(0);
    }
    m_invited_geted[id-1] = s;
    save_invited();
}

void char_invite::set_share_get(int id, int s)
{
    //cout<<"set share get,id:"<<id<<",size()"<<m_share_geted.size()<<endl;
    while (id > m_share_geted.size())
    {
        m_share_geted.push_back(-1);
    }
    m_share_geted[id-1] = s;
    save_share(id);
}

void char_invite::set_close_friend_get(int id, int s)
{
    while (id > m_close_friend_geted.size())
    {
        m_close_friend_geted.push_back(0);
    }
    cout<<"set_close_friend_get,"<<id<<","<<s<<endl;
    m_close_friend_geted[id-1] = s;
    save_close_friend();
}

void char_invite::update_close_friend_get(const base_close_friend_reward& r)
{
    cout<<"------------>update_close_friend_get("<<r.id<<","<<r.level<<","<<r.count<<")"<<endl;
    //未达成才需要更新
    if (close_friend_get(r.id) == 0)
    {
        int total = 0;
        for (std::map<int,int>::iterator it = m_close_friends.begin(); it != m_close_friends.end(); ++it)
        {
            cout<<it->first<<"->"<<it->second<<endl;
            if (it->second >= r.level)
            {
                ++total;
            }
        }
        cout<<"--->total "<<total<<endl;
        if (total >= r.count)
        {
            set_close_friend_get(r.id, 1);
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_cid).get();
            if (pc)
            {
                pc->notifyEventState(top_level_event_invite, 1, 0);
            }
        }
    }
}

void char_invite::load_invite_get(const std::string& s)
{
    read_int_vector(s, m_invite_geted);
}

void char_invite::load_invited_get(const std::string& s)
{
    read_int_vector(s, m_invited_geted);
}

void char_invite::load_share_get(const std::string& s)
{
    read_int_vector(s, m_share_geted);
}

void char_invite::load_close_friend_get(const std::string& s)
{
    read_int_vector(s, m_close_friend_geted);
}

int char_invite::getCanGet()
{
    for (size_t i = 0; i < m_invite_geted.size(); ++i)
    {
        if (m_invite_geted[i] == 1)
        {
            return 1;
        }
    }
    for (size_t i = 0; i < m_invited_geted.size(); ++i)
    {
        if (m_invited_geted[i] == 1)
        {
            return 1;
        }
    }
    for (size_t i = 0; i < m_close_friend_geted.size(); ++i)
    {
        if (m_close_friend_geted[i] == 1)
        {
            return 1;
        }
    }
    for (size_t i = 0; i < m_share_geted.size(); ++i)
    {
        if (m_share_geted[i] == 0)
        {
            return 1;
        }
    }
    return 0;
}

int char_invite::getCloseFriendState()
{
    int state = 2;
    for (size_t i = 0; i < m_close_friend_geted.size(); ++i)
    {
        if (m_close_friend_geted[i] == 1)
        {
            return 1;
        }
        if (m_close_friend_geted[i] == 0)
        {
            state = m_close_friend_geted[i];
        }
    }
    if (state == 2 && m_close_friend_geted.size() == m_handle.close_friend_size())
    {
        return 2;
    }
    else
    {
        return 0;
    }
}

int char_invite::getShareState()
{
    int state = 1;
    for (size_t i = 0; i < m_share_geted.size(); ++i)
    {
        if (m_share_geted[i] == 0)
        {
            return 1;
        }
        if (-1 == m_share_geted[i])
        {
            state = 0;
        }
    }
    if (state == 1 && m_share_geted.size() == m_handle.share_size())
    {
        return 2;
    }
    else
    {
        return 0;
    }
}

int char_invite::getInviteState()
{
    for (size_t i = 0; i < m_invite_geted.size(); ++i)
    {
        if (m_invite_geted[i] == 1)
        {
            return 1;
        }
    }
    for (size_t i = 0; i < m_invited_geted.size(); ++i)
    {
        if (m_invited_geted[i] == 1)
        {
            return 1;
        }
    }
    return 0;
}

inviteMgr::inviteMgr()
{
    load();
}

void inviteMgr::load()
{
    Query q(GetDb());

    //密友
    q.get_result("select id,count,level from base_close_friend where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        base_close_friend_reward r;
        r.id = q.getval();
        r.count = q.getval();
        r.level = q.getval();
        r.reward.reset(new baseLibao);
        m_base_close_friend.push_back(r);
        assert(r.id == m_base_close_friend.size());        
    }
    q.free_result();

    //密友奖励
    q.get_result("select bid,itemType,itemId,count from base_close_friend_awards where 1 order by bid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int bid = q.getval();
        if (0 < bid && bid <= m_base_close_friend.size())
        {
            base_close_friend_reward& r = m_base_close_friend[bid-1];
            Item itm;
            itm.type = q.getval();
            itm.id = q.getval();
            itm.nums = q.getval();

            if (r.reward.get())
            {
                r.reward->m_list.push_back(itm);
            }
            else
            {
                cout<<"bid->"<<bid<<endl;
                ERR();
            }
        }
        else
        {
            cout<<bid<<endl;
            ERR();
        }
    }
    q.free_result();

    //分享
    q.get_result("select id,title,content,type,param from base_share_event where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        base_share_event e;
        e.id = q.getval();
        e.title = q.getstr();
        e.content = q.getstr();
        e.type = q.getval();
        e.param = q.getval();
        e.reward.reset(new baseLibao);
        m_base_share_event.push_back(e);
    }
    q.free_result();

    //分享奖励
    /*q.get_result("select bid,itemType,itemId,count from base_share_event_awards where 1 order by bid,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        if (id > 0 && id <= m_base_share_event.size())
        {
            base_share_event& r = m_base_share_event[id-1];
            Item itm;
            itm.type = q.getval();
            itm.id = q.getval();
            itm.nums = q.getval();
            if (r.reward.get())
            {
                r.reward->m_list.push_back(itm);
            }
            else
            {
                cout<<"bid->"<<id<<endl;
                ERR();
            }
        }
        else
        {
            cout<<id<<endl;
            ERR();
        }
    }
    q.free_result();*/
    
    //好友邀请
    q.get_result("select id,count from base_invite where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        base_invite_reward e;
        e.id = q.getval();
        e.count = q.getval();
        e.reward.reset(new baseLibao);
        m_base_invite.push_back(e);        
        assert(e.id == m_base_invite.size());
    }
    q.free_result();

    //好友邀请成功
    q.get_result("select id,count from base_invite_success where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        base_invited_reward e;
        e.id = q.getval();
        e.count = q.getval();
        e.reward.reset(new baseLibao);
        m_base_invited.push_back(e);
        assert(e.id == m_base_invited.size());
    }
    q.free_result();

    //邀请奖励
    q.get_result("select bid,itemType,itemId,count from base_invite_awards where 1 order by bid,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        if (id > 0 && id <= m_base_invite.size())
        {
            base_invite_reward& r = m_base_invite[id-1];
            Item itm;
            itm.type = q.getval();
            itm.id = q.getval();
            itm.nums = q.getval();
            if (r.reward.get())
            {
                r.reward->m_list.push_back(itm);
            }
            else
            {
                cout<<"bid->"<<id<<endl;
                ERR();
            }
        }
        else
        {
            cout<<id<<endl;
            ERR();
        }
    }
    q.free_result();

    //邀请成功奖励
    q.get_result("select bid,itemType,itemId,count from base_invite_success_awards where 1 order by bid,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        if (id > 0 && id <= m_base_invited.size())
        {
            base_invited_reward& r = m_base_invited[id-1];
            Item itm;
            itm.type = q.getval();
            itm.id = q.getval();
            itm.nums = q.getval();
            if (r.reward.get())
            {
                r.reward->m_list.push_back(itm);
            }
            else
            {
                cout<<"bid->"<<id<<endl;
                ERR();
            }
        }
        else
        {
            cout<<id<<endl;
            ERR();
        }
    }
    q.free_result();

    q.get_result("select pos,itemType,itemId,count,fac,gailv,notice from base_invite_lottery_awards where 1 order by pos");
    CHECK_DB_ERR(q);
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

    q.get_result("select value from custom_settings where code='invite_lottery'");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
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

    q.get_result("select cid,fid from char_close_friend_id where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int cid = q.getval();
        int fid = q.getval();
        m_close_friend_to_map[fid] = cid;
    }
    q.free_result();

    for (size_t i = 0; i < m_base_invite.size(); ++i)
    {
        base_invite_reward& r = m_base_invite[i];
        r.reward->updateObj();
    }
    for (size_t i = 0; i < m_base_invited.size(); ++i)
    {
        base_invited_reward& r = m_base_invited[i];
        r.reward->updateObj();
    }
    for (size_t i = 0; i < m_base_share_event.size(); ++i)
    {
        base_share_event& r = m_base_share_event[i];
        r.reward->updateObj();
    }
    for (size_t i = 0; i < m_base_close_friend.size(); ++i)
    {
        base_close_friend_reward& r = m_base_close_friend[i];
        r.reward->updateObj();
    }

    m_recall_rewards.reset(new baseLibao);
    Item itm;
    itm.type = 2;
    itm.id = treasure_type_friend_lottery;
    itm.nums = 1;
    m_recall_rewards->m_list.push_back(itm);
    m_recall_rewards->updateObj();
}

boost::shared_ptr<char_invite> inviteMgr::getChar(int cid)
{
    if (cid <= 0)
    {
        boost::shared_ptr<char_invite> tmp;
        return tmp;
    }
    std::map<int, boost::shared_ptr<char_invite> >::iterator it = m_chars.find(cid);
    if (it != m_chars.end())
    {
        return it->second;
    }
    char_invite* c = new char_invite(*this);
    Query q(GetDb());
    q.get_result("select recall,invite,invited,invite_get,invited_get from char_invite_data where cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        int r = q.getval();
        int inv = q.getval();
        int inved = q.getval();
        c->init(cid, r, inv, inved);

        std::string invite_get = q.getstr();
        std::string invited_get = q.getstr();

        c->load_invite_get(invite_get);
        c->load_invited_get(invited_get);
    }
    else
    {
        c->init(cid, 0, 0, 0);
        InsertSaveDb("insert into char_invite_data (cid,recall,invite,invited,invite_get,invited_get) values ("
            + LEX_CAST_STR(cid) + ",0,0,0,'','')");
    }
    q.free_result();

    q.get_result("select data from char_share_event where cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        std::string data = q.getstr();
        q.free_result();
        c->load_share_get(data);        
    }
    else
    {
        q.free_result();
        InsertSaveDb("insert into char_share_event (cid,data) values (" + LEX_CAST_STR(cid) + ",'')");
    }

    q.get_result("select getted from char_close_friend where cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        std::string getted = q.getstr();
        c->load_close_friend_get(getted);
        //c->load_close_friend(friends);
    }
    else
    {
        InsertSaveDb("insert into char_close_friend (cid,getted,friends) values (" + LEX_CAST_STR(cid) + ",'','')");
    }
    q.free_result();

    q.get_result("select f.fid,c.level from char_close_friend_id as f left join charactors as c on f.fid=c.id where f.cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int fid = q.getval();
        int level = q.getval();
        c->add_close_friend(fid, level);
    }
    q.free_result();

    m_chars[cid].reset(c);
    return m_chars[cid];
}

void inviteMgr::getAction(CharData* pc, json_spirit::Array& list)
{
    if (pc)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_invite) );
        int state = 0;
        int view = pc->queryExtraData(char_data_type_daily, char_data_daily_view_invite);
        if (view == 0)
        {
            state = 1;
        }
        else
        {
            boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(pc->m_id);
            if (cc.get() == NULL)
            {
                char_invite* c = cc.get();
                state = c->getCanGet();
            }
        }
        obj.push_back( Pair("active", state) );
        list.push_back(obj);
    }
}

int inviteMgr::getInviteAward(CharData& cdata, int id, json_spirit::Object& robj)
{
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(cdata.m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();
    if (c->invite_get(id) == 1)
    {
        c->set_invite_get(id, 2);
        c->save_invite_get();

        if (id > 0 && id <= m_base_invite.size())
        {
            base_invite_reward& r = m_base_invite[id-1];
            std::list<Item> items = r.reward->m_list;
            giveLoots(&cdata, items, cdata.m_area, cdata.m_level, 0, NULL, &robj, true, give_invite);
        }
        int state = c->getCanGet();
        cdata.notifyEventState(top_level_event_invite, state, 0);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int inviteMgr::getInvitedAward(CharData& cdata, int id, json_spirit::Object& robj)
{
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(cdata.m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();
    if (c->invited_get(id) == 1)
    {
        c->set_invited_get(id, 2);
        c->save_invited_get();

        if (id > 0 && id <= m_base_invited.size())
        {
            base_invited_reward& r = m_base_invited[id-1];
            std::list<Item> items = r.reward->m_list;
            giveLoots(&cdata, items, cdata.m_area, cdata.m_level, 0, NULL, &robj, true, give_invite);
        }
        int state = c->getCanGet();
        cdata.notifyEventState(top_level_event_invite, state, 0);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int inviteMgr::getCloseFriendAward(CharData& cdata, int id, json_spirit::Object& robj)
{
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(cdata.m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();
    if (c->close_friend_get(id) == 1)
    {
        c->set_close_friend_get(id, 2);
        c->save_close_friend();

        if (id > 0 && id <= m_base_close_friend.size())
        {
            base_close_friend_reward& r = m_base_close_friend[id-1];
            std::list<Item> items = r.reward->m_list;
            giveLoots(&cdata, items, cdata.m_area, cdata.m_level, 0, NULL, &robj, true, give_invite);
        }
        int state = c->getCanGet();
        cdata.notifyEventState(top_level_event_invite, state, 0);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int inviteMgr::getShareAward(CharData& cdata, int id, json_spirit::Object& robj)
{
    std::list<Item> items = m_recall_rewards->m_list;
    giveLoots(&cdata, items, cdata.m_area, cdata.m_level, 0, NULL, &robj, true, give_invite);
    return HC_SUCCESS;
}

int inviteMgr::getRecallAward(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_invite> cc = Singleton<inviteMgr>::Instance().getChar(cdata.m_id);
    if (cc.get() == NULL)
    {
        return HC_ERROR;
    }
    char_invite* c = cc.get();
    if (c->get_recall() == 1)
    {
        c->set_recall(2);

        if (m_recall_rewards.get())
        {
            std::list<Item> items = m_recall_rewards->m_list;
            giveLoots(&cdata, items, cdata.m_area, cdata.m_level, 0, NULL, &robj, true, give_invite);
        }        
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void inviteMgr::get_invite_list(char_invite& c, json_spirit::Array& list1, json_spirit::Array& list2)
{
    for (std::vector<base_invite_reward>::iterator it = m_base_invite.begin(); it != m_base_invite.end(); ++it)
    {        
        if (it->reward.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->id) );
            obj.push_back( Pair("nums", it->count) );
            obj.push_back( Pair("get", c.invite_get(it->id)) );

            const json_spirit::Array& rlist = it->reward->getArray();
            obj.push_back( Pair("list", rlist) );

            list1.push_back(obj);
        }
    }

    for (std::vector<base_invited_reward>::iterator it = m_base_invited.begin(); it != m_base_invited.end(); ++it)
    {        
        if (it->reward.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->id) );
            obj.push_back( Pair("nums", it->count) );
            obj.push_back( Pair("get", c.invited_get(it->id)) );

            const json_spirit::Array& rlist = it->reward->getArray();
            obj.push_back( Pair("list", rlist) );

            list2.push_back(obj);
        }
    }
}

void inviteMgr::get_share_list(char_invite& c, json_spirit::Array& list)
{
    for (std::vector<base_share_event>::iterator it = m_base_share_event.begin(); it != m_base_share_event.end(); ++it)
    {
        json_spirit::Object obj;    
        if (c.share_get(it->id) == -1)
        {
            obj.push_back( Pair("state", 0) );
        }
        else if (0 == c.share_get(it->id))
        {
            obj.push_back( Pair("state", 1) );
        }
        else
        {
            continue;
        }
        obj.push_back( Pair("id", it->id) );
        obj.push_back( Pair("title", it->title) );
        obj.push_back( Pair("msg", it->content) );

        const json_spirit::Array& rlist = m_recall_rewards->getArray();
        obj.push_back( Pair("list", rlist) );

        list.push_back(obj);
    }
}

void inviteMgr::get_close_friend_list(char_invite& c, json_spirit::Array& list)
{
    for (std::vector<base_close_friend_reward>::iterator it = m_base_close_friend.begin(); it != m_base_close_friend.end(); ++it)
    {
        int get = c.close_friend_get(it->id);
        if (get != 2)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->id) );
            obj.push_back( Pair("level", it->level) );
            obj.push_back( Pair("count", it->count) );
            obj.push_back( Pair("get", get) );

            const json_spirit::Array& rlist = it->reward->getArray();
            obj.push_back( Pair("list", rlist) );

            list.push_back(obj);
        }
    }
}

//可能获得奖励列表
void inviteMgr::getAwards(json_spirit::Array& list)
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
Item inviteMgr::random_award(int& add_notice, int& pos)
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

//查询抽奖公告
int inviteMgr::queryLotteryNotice(json_spirit::Object& robj)
{
    robj.push_back( Pair("list", m_notices_value.get_array()) );
    return HC_SUCCESS;
}

//增加记录
void inviteMgr::addLotteryNotice(const std::string& name, Item& item)
{
    std::string what = strInviteLotteryGet;
    str_replace(what, "$W", item.toString(true));

    json_spirit::Array& notice_array = m_notices_value.get_array();
    json_spirit::Object obj;
    obj.push_back( Pair("name", name) );
    obj.push_back( Pair("get", what) );
    notice_array.push_back(obj);
    while ((int)notice_array.size() > 5)
    {
        notice_array.erase(notice_array.begin());
    }
    InsertSaveDb("replace into custom_settings (code,value) values ('invite_lottery','" +GetDb().safestr(json_spirit::write(m_notices_value)) + "')");
}

//更新分享事件
void inviteMgr::update_event(int cid, int type, int praram)
{
    for (std::vector<base_share_event>::iterator it = m_base_share_event.begin(); it != m_base_share_event.end(); ++it)
    {
        if (it->type == type && it->param == praram)
        {
            boost::shared_ptr<char_invite> cc = getChar(cid);
            if (cc.get() == NULL)
            {
                return;
            }
            char_invite* c = cc.get();
            if (c->share_get(it->id) == -1)
            {
                c->set_share_get(it->id, 0);
            }
            break;
        }
    }
}

//成功邀请
void inviteMgr::add_invited(int cid, int icid)
{
    (void)icid;
    boost::shared_ptr<char_invite> cc = getChar(cid);
    if (cc.get() == NULL)
    {
        return;
    }
    char_invite* c = cc.get();
    int inv = c->add_invited(1);
    int id = in_base_invited(inv);
    if (id > 0 && c->invited_get(id) == 0)
    {
        c->set_invited_get(id, 1);
        c->save_invited_get();

        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc)
        {
            pc->notifyEventState(top_level_event_invite, 1, 0);
        }
    }
    else
    {
        c->save_invited();
    }    

    c->add_close_friend(icid, 1);
    m_close_friend_to_map[icid] = cid;
    InsertSaveDb("insert into char_close_friend_id (cid,fid) values (" + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(icid) + ")");
}

//成功召回
void inviteMgr::add_recall(int cid, int rcid)
{
    boost::shared_ptr<char_invite> cc = getChar(cid);
    if (cc.get() == NULL)
    {
        return;
    }
    char_invite* c = cc.get();
    c->set_recall(1);
}

void inviteMgr::daily_reset()
{
    for (std::map<int, boost::shared_ptr<char_invite> >::iterator it = m_chars.begin(); it != m_chars.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->daily_reset();            
        }
    }
    InsertSaveDb("update char_invite_data set recall=0,invite=0,invited=0,invite_get='',invited_get='' where 1");
}

int inviteMgr::in_base_invite(int i)
{
    for (std::vector<base_invite_reward>::iterator it = m_base_invite.begin(); it != m_base_invite.end(); ++it)
    {
        if (it->count == i)
        {
            return it->id;
        }
        else if (it->count > i)
        {
            return 0;
        }
    }
    return 0;
}

int inviteMgr::in_base_invited(int i)
{
    for (std::vector<base_invited_reward>::iterator it = m_base_invited.begin(); it != m_base_invited.end(); ++it)
    {
        if (it->count == i)
        {
            return it->id;
        }
        else if (it->count > i)
        {
            return 0;
        }
    }
    return 0;
}

void inviteMgr::fixShareData()
{
    std::vector<int> cids;
    Query q(GetDb());
    q.get_result("select cid from char_corps_members where 1");
    while (q.fetch_row())
    {
        cids.push_back(q.getval());
    }
    q.free_result();

    for (size_t i = 0; i < cids.size(); ++i)
    {
        update_event(cids[i], SHARE_EVENT_JOIN_CORPS, 0);
    }
}

int inviteMgr::getCloseFriendTo(int cid)
{
    std::map<int,int>::iterator it = m_close_friend_to_map.find(cid);
    if (it != m_close_friend_to_map.end())
    {
        return it->second;
    }
    return 0;
}

void inviteMgr::close_friend_levelup(int cid, int fid, int level)
{
    cout<<"close_friend_levelup("<<cid<<","<<fid<<","<<level<<")"<<endl;
    boost::shared_ptr<char_invite> cc = getChar(cid);
    if (cc.get() == NULL)
    {
        return;
    }
    char_invite* c = cc.get();
    c->add_close_friend(fid, level);
    for (size_t i = 0; i < m_base_close_friend.size(); ++i)
    {
        base_close_friend_reward& r = m_base_close_friend[i];
        if (r.level == level)
        {
            c->update_close_friend_get(r);
        }
    }    
}

#endif

