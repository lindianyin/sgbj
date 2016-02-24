
#include "relation.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "singleton.h"
#include "net.h"
#include "statistics.h"
#include "guard.h"
#include "eliteCombat.h"

using namespace net;

Database& GetDb();
extern int getSessionChar(session_ptr& psession, CharData* &pc);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);

//祝贺成功获得银币
extern std::string strCongratulationSuccessMsg;
//一键祝贺成功
extern std::string strCongratulationAllSuccessMsg;
//今日祝贺收益次数满
extern std::string strCongratulationMaxMsg;
//仇敌通知
extern std::string strAddEnemy;
//仇敌通知
extern std::string strAddEnemyMailContent;

void InsertSaveDb(const std::string& sql);

//好友申请提示
const std::string strFriendApply = "{\"time\":$T,\"cmd\":\"friendApply\",\"s\":200}";
const std::string strNotifyNearPlayer = "{\"cmd\":\"updateNearList\",\"s\":200}";

//祝贺保持时间16小时，超过16小时，无法祝贺
const int iCongratulationKeepTime = 16 * 3600;

//仇敌最低仇恨
static int iEnemyHate = 3;

static const int max_friend = 200;
static const int max_enemy = 100;

static void db_attention(int cid1, int cid2)
{
    InsertSaveDb("replace into char_friends (cid,friend_id,state) values ("
        + LEX_CAST_STR(cid1) + "," + LEX_CAST_STR(cid2) + ",1)");
}

static void db_remove_attention(int cid1, int cid2)
{
    InsertSaveDb("delete from char_friends where cid="    + LEX_CAST_STR(cid1)
        + " and friend_id=" + LEX_CAST_STR(cid2));
}

static void db_applicition(int cid1, int cid2)
{
    InsertSaveDb("replace into char_friends (cid,friend_id,state) values ("
        + LEX_CAST_STR(cid1) + "," + LEX_CAST_STR(cid2) + ",0)");
}

static void db_remove_all_app(int cid)
{
    InsertSaveDb("delete from char_friends where cid="    + LEX_CAST_STR(cid)
        + " and state=0");
}

static void db_remove_con(int id)
{
    InsertSaveDb("delete from char_congratulations where id="    + LEX_CAST_STR(id));
}

static void db_set_congralutioned(int id)
{
    InsertSaveDb("update char_congratulations set type='2' where id="    + LEX_CAST_STR(id));
}

static int add_congratulation_silver(CharData& cdata, int type, int level)
{
    int silver = level * 20;
    cdata.addSilver(silver);
    //银币统计,在外面统计了
    return silver;
}

static int add_be_congratulationed_silver(CharData& cdata, int type, int level)
{
    int silver = level * 20;
    cdata.addSilver(silver);
    cdata.NotifyCharData();
    //银币统计
    add_statistics_of_silver_get(cdata.m_id, cdata.m_ip_address, silver, silver_get_by_friend, cdata.m_union_id, cdata.m_server_id);
    return silver;
}

static void db_enemy(int cid, int eid, int hate)
{
    InsertSaveDb("replace into char_enemys (cid,eid,hate) values ("
        + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(eid) + "," + LEX_CAST_STR(hate)+ ")");
}

inline int maxCongratulations(int vip)
{
    int max_times[12+1] = {50,80,120,160,200,250,300,300,300,300,300,300,300};
    if (vip > 12)
    {
        vip = 12;
    }
    else if (vip < 0)
    {
        vip = 0;
    }
    return max_times[vip];
}

boost::shared_ptr<CharData> my_enemy::getChar()
{
    if (!b_getChar)
    {
        m_charData = GeneralDataMgr::getInstance()->GetCharData(enemy_id);        
    }
    return m_charData;
}

bool my_enemy::is_real_enemy()
{
    return hate >= iEnemyHate;
}

void char_congratulation::gen_msg()
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(from_cid).get();
    if (!pc)
    {
        return;
    }

    from_name = pc->m_name;

    //未祝贺和已经祝贺
    if (type == 0 || type == 2)
    {
        Singleton<relationMgr>::Instance().getCongratulationMsg(ctype, msg);
        str_replace(msg, "$W", MakeCharNameLink(pc->m_name));
        switch (ctype)
        {
            case CONGRATULATION_MAKE_EQUIP:    
            {
                boost::shared_ptr<baseEquipment> be = GeneralDataMgr::getInstance()->GetBaseEquipment(param[0]);
                if (be.get())
                {
                    str_replace(msg, "$E", be->link_name);
                }
                break;
            }
            case CONGRATULATION_REBORN_HORSE1:// = 2,
            case CONGRATULATION_REBORN_HORSE2:// = 3,
            case CONGRATULATION_REBORN_HORSE3:// = 4,
            case CONGRATULATION_REBORN_HORSE4:// = 5,
            case CONGRATULATION_REBORN_HORSE5:// = 6,
            {
                baseHorse* horse = horseMgr::getInstance()->getHorse(param[0]);
                if (horse)
                {
                    std::string horsename = horseMgr::getInstance()->NameToLink(from_cid, horse->name, horse->quality);
                    str_replace(msg, "$H", horsename);
                }
                break;
            }
            case CONGRATULATION_LEVEL:// = 7,
            {
                str_replace(msg, "$L", LEX_CAST_STR(param[0]));
                break;
            }
            case CONGRATULATION_FRIST_QUALITY1_GENERAL:// = 8,
            case CONGRATULATION_FRIST_QUALITY2_GENERAL:// = 9,
            case CONGRATULATION_FRIST_QUALITY3_GENERAL:// = 10,
            case CONGRATULATION_FRIST_QUALITY4_GENERAL:// = 11,
            {
                boost::shared_ptr<CharGeneralData> g = pc->GetGenerals().GetGenral(param[0]);
                if (g.get())
                {
                    str_replace(msg, "$G", g->colorLink());
                }
                break;
            }
            case CONGRATULATION_VIP_LEVEL_1:// = 12,
            case CONGRATULATION_VIP_LEVEL_2:// = 13,
            case CONGRATULATION_VIP_LEVEL_3:// = 14,
            {
                str_replace(msg, "$V", LEX_CAST_STR(param[0]));
                break;
            }
            case CONGRATULATION_ENHANCE_EQUIPMENT:// = 15,
            {
                boost::shared_ptr<baseEquipment> be = GeneralDataMgr::getInstance()->GetBaseEquipment(param[0]);
                if (be.get())
                {
                    str_replace(msg, "$E", be->link_name);
                }
                str_replace(msg, "$L", LEX_CAST_STR(param[1]));
                break;
            }
            case CONGRATULATION_MIFA:// = 16,
            {
                baseNewWeapon* be = newWeaponMgr::getInstance()->getWeapon(param[0]);
                if (be)
                {
                    str_replace(msg, "$M", be->_name);
                }
                str_replace(msg, "$L", LEX_CAST_STR(param[1]));
                break;
            }
            case CONGRATULATION_FRIST_RACE_WIN:// = 17,
            case CONGRATULATION_TOP_RACER:// = 18,
                break;
            case CONGRATULATION_PASS_STAGE:// = 19,
            {
                str_replace(msg, "$S", GeneralDataMgr::getInstance()->GetStageName(param[0], param[1]));
                break;
            }
            case CONGRATULATION_WASH_1:// = 20,
            case CONGRATULATION_WASH_2:// = 21,
            case CONGRATULATION_WASH_3:// = 22,
            {
                boost::shared_ptr<CharGeneralData> g = pc->GetGenerals().GetGenral(param[0]);
                if (g.get())
                {
                    str_replace(msg, "$G", g->colorLink());
                }
                str_replace(msg, "$X", LEX_CAST_STR(param[1]));
                break;
            }
            case CONGRATULATION_FIRST_BAOSHI:// = 23,
            {
                str_replace(msg, "$L", LEX_CAST_STR(param[1]));
                baseNewBaoshi* bs = Singleton<newBaoshiMgr>::Instance().getBaoshi(param[0]);
                if (bs)
                {
                    str_replace(msg, "$S", bs->name);
                }
                break;
            }
            case CONGRATULATION_REBORN:// = 24,
            {
                boost::shared_ptr<CharGeneralData> g = pc->GetGenerals().GetGenral(param[0]);
                if (g.get())
                {
                    str_replace(msg, "$G", g->colorLink());
                    double cz = param[1];
                    cz = cz / 100;
                    char chengzhang[32];
                    memset(chengzhang, 0, 32);
                    snprintf(chengzhang, 31, "%.2f", cz);
                    str_replace(msg, "$C", chengzhang);
                }
                break;
            }
            case CONGRATULATION_ELITE:// = 25,
            {
                boost::shared_ptr<eliteCombat> ec = eliteCombatMgr::getInstance()->getEliteCombatById(param[0]);
                if (ec.get())
                {
                    str_replace(msg, "$E", ec->_name);
                }
                break;
            }
        }
    }
    //别人发来的祝贺
    else if (1 == type)
    {
        Singleton<relationMgr>::Instance().getCongratulationMsg2(ctype, msg);
        str_replace(msg, "$W", MakeCharNameLink(pc->m_name));
        str_replace(msg, "$S", LEX_CAST_STR(silver));
    }
    else//盟友动态
    {
        Singleton<relationMgr>::Instance().getFriendMsg(ctype, msg);
        str_replace(msg, "$W", MakeCharNameLink(from_name));

        if (FRIEND_NEWS_JOIN_CORPS == ctype)
        {
            str_replace(msg, "$C", extra);
        }
    }
}

void char_enemy_news::gen_msg()
{
    Singleton<relationMgr>::Instance().getEnemyMsg(type, msg);
    str_replace(msg, "$W", MakeCharNameLink(ename));
}

char_ralation::char_ralation(int id)
{
    b_getChar = false;
    m_char_id = id;
    b_friend_news_loaded = false;
    b_enemy_news_loaded = false;
    b_congratulations_loaded = false;
    b_recved_congratulations_loaded = false;
    b_enemy_loaded = false;
}

boost::shared_ptr<CharData> char_ralation::getChar()
{
    if (!b_getChar)
    {
        m_charData = GeneralDataMgr::getInstance()->GetCharData(m_char_id);        
    }
    return m_charData;
}

//加关注
bool char_ralation::add_attention(boost::shared_ptr<char_ralation> r)
{
     if (r.get() && !is_attention(r->m_char_id))
     {
        m_my_attentions[r->m_char_id] = r;
        return true;
     }
    else
    {
        return false;
    }
}

//加听众
void char_ralation::add_listener(boost::shared_ptr<char_ralation> r)
{
    if (r.get() && !is_listener(r->m_char_id))
     {
        m_my_listeners[r->m_char_id] = r;
     }
}

//加互相关注
void char_ralation::add_friend(boost::shared_ptr<char_ralation> r)
{
    if (r.get() && !is_friend(r->m_char_id))
     {
        m_my_friends[r->m_char_id] = r;
     }
}

//加申请
bool char_ralation::add_application(boost::shared_ptr<char_ralation> r)
{
    if (r.get() && !is_application(r->m_char_id))
     {
        m_my_pending_review[r->m_char_id] = r;
        return true;
     }
    return false;
}

bool char_ralation::is_attention(int id)
{
    return (m_my_attentions.find(id) != m_my_attentions.end());
}

bool char_ralation::is_listener(int id)
{
    return (m_my_listeners.find(id) != m_my_listeners.end());
}

bool char_ralation::is_friend(int id)
{
    return (m_my_friends.find(id) != m_my_friends.end());
}

bool char_ralation::is_application(int id)
{
    return (m_my_pending_review.find(id) != m_my_pending_review.end());
}

bool char_ralation::is_enemy(int id)
{
    std::map<int, my_enemy>::iterator it = m_my_enemys.find(id);
    if (it != m_my_enemys.end() && it->second.is_real_enemy())
    {
        return true;
    }
    return false;
}

//取消关注
boost::shared_ptr<char_ralation> char_ralation::remove_attention(int id)
{
    boost::shared_ptr<char_ralation> rl;
    std::map<int, boost::shared_ptr<char_ralation> >::iterator it = m_my_attentions.find(id);
    if (it != m_my_attentions.end())
    {
        rl = it->second;
        m_my_attentions.erase(it);
    }
    return rl;
}

//移除听众
boost::shared_ptr<char_ralation> char_ralation::remove_listener(int id)
{
    boost::shared_ptr<char_ralation> rl;
    std::map<int, boost::shared_ptr<char_ralation> >::iterator it = m_my_listeners.find(id);
    if (it != m_my_listeners.end())
    {
        rl = it->second;
        m_my_listeners.erase(it);
    }
    return rl;
}

//移除申请
boost::shared_ptr<char_ralation> char_ralation::remove_application(int id)
{
    boost::shared_ptr<char_ralation> rl;
    std::map<int, boost::shared_ptr<char_ralation> >::iterator it = m_my_pending_review.find(id);
    if (it != m_my_pending_review.end())
    {
        rl = it->second;
        m_my_pending_review.erase(it);
    }
    return rl;
}
    
//移除互相关注
boost::shared_ptr<char_ralation> char_ralation::remove_friend(int id)
{
    boost::shared_ptr<char_ralation> rl;
    std::map<int, boost::shared_ptr<char_ralation> >::iterator it = m_my_friends.find(id);
    if (it != m_my_friends.end())
    {
        rl = it->second;
        m_my_friends.erase(it);
    }
    return rl;
}

//移除仇敌
bool char_ralation::remove_enemy(int eid)
{
    std::map<int, my_enemy>::iterator it = m_my_enemys.find(eid);
    if (it != m_my_enemys.end())
    {
        m_my_enemys.erase(it);
        InsertSaveDb("delete from char_enemys where cid=" + LEX_CAST_STR(m_char_id) + " and eid=" + LEX_CAST_STR(eid));    
        return true;
    }
    return false;
}

//移除谁仇恨我
bool char_ralation::remove_hate_me(int id)
{
    std::list<int>::iterator it = m_who_hateme.begin();
    while (it != m_who_hateme.end())
    {
        if (*it == id)
        {
            m_who_hateme.erase(it);
            return true;
        }
        ++it;
    }
    return false;
}

void char_ralation::add_hate_me(int id)
{
    std::list<int>::iterator it = m_who_hateme.begin();
    while (it != m_who_hateme.end())
    {
        if (*it == id)
        {
            return;
        }
        ++it;
    }
    m_who_hateme.push_back(id);
}

void char_ralation::_add_congratulation(char_congratulation& con)
{
    try_load_congratulations();
    m_my_congratulations.push_back(con);
    InsertSaveDb("insert into char_congratulations (id,cid,fid,type,ctype,param1,param2,extra,input) values ("
        + LEX_CAST_STR(con.id) + ","
        + LEX_CAST_STR(con.cid) + ","
        + LEX_CAST_STR(con.from_cid) + ","
        + LEX_CAST_STR(con.type) + ","
        + LEX_CAST_STR(con.ctype) + ","
        + LEX_CAST_STR(con.param[0]) + ","
        + LEX_CAST_STR(con.param[1]) + ",'"
        + GetDb().safestr(con.extra) + "',from_unixtime(" + LEX_CAST_STR(con.time_stamp) + "))");

    while (m_my_congratulations.size() > 100)
    {
        char_congratulation c = m_my_congratulations.front();
        InsertSaveDb("delete from char_congratulations where id=" + LEX_CAST_STR(c.id));
        m_my_congratulations.pop_front();
    }
}

//收到祝贺
void char_ralation::recv_congratulation(int id, int cid, int s, int type, int param)
{
    char_congratulation con;
    con.id = id;
    con.cid = m_char_id;
    con.type = 1;
    con.ctype = type;
    con.from_cid = cid;
    con.silver = s;
    con.time_stamp = time(NULL);
    con.param[0] = param;
    con.param[1] = s;

    con.gen_msg();

    try_load_recved_congratulations();
    m_my_recved_congratulations.push_back(con);
    InsertSaveDb("insert into char_recved_congratulations (id,cid,fid,ctype,param1,param2,input) values ("
        + LEX_CAST_STR(con.id) + ","
        + LEX_CAST_STR(con.cid) + ","
        + LEX_CAST_STR(con.from_cid) + ","
        + LEX_CAST_STR(con.ctype) + ","
        + LEX_CAST_STR(con.param[0]) + ","
        + LEX_CAST_STR(con.param[1]) + ",from_unixtime(" + LEX_CAST_STR(con.time_stamp) + "))");

    while (m_my_recved_congratulations.size() > 100)
    {
        char_congratulation c = m_my_recved_congratulations.front();
        InsertSaveDb("delete from char_recved_congratulations where id=" + LEX_CAST_STR(c.id));
        m_my_recved_congratulations.pop_front();
    }
}

//收到等待祝贺
void char_ralation::add_congratulation(int id, int cid, int type, int param1, int param2)
{
    char_congratulation con;
    con.id = id;
    con.cid = m_char_id;
    con.type = 0;
    con.ctype = type;
    con.from_cid = cid;

    con.time_stamp = time(NULL);
    con.param[0] = param1;
    con.param[1] = param2;

    con.gen_msg();
    
    _add_congratulation(con);

    ++m_unread_congratulations;
    CharData* pc = getChar().get();
    if (pc)
    {
        pc->m_need_notify[notify_msg_new_congratulation] = m_unread_congratulations;
    }
}

//增加好友动态
void char_ralation::add_friend_news(int id, int cid, const std::string& name, int type, const std::string& param)
{
    char_congratulation fnews;
    fnews.id = id;
    fnews.cid = m_char_id;
    fnews.from_name = name;
    fnews.from_cid = cid;
    fnews.time_stamp = time(NULL);
    fnews.type = 4;
    fnews.ctype = type;
    fnews.extra = param;

    fnews.gen_msg();
    _add_congratulation(fnews);

    //CharData* pc = getChar().get();
    //if (pc)
    //{
    //    ++(pc->m_need_notify[notify_msg_new_congratulation]);
    //}
}

//增加仇敌动态
void char_ralation::add_enemy_news(int id, int cid, const std::string& name, int type)
{
    try_load_enemy_news();
    char_enemy_news enews;
    enews.id = id;
    enews.cid = m_char_id;
    enews.ename = name;
    enews.eid = cid;
    enews.time_stamp = time(NULL);
    enews.type = type;

    enews.gen_msg();
    m_enemy_news.push_back(enews);

    //保存数据库
    InsertSaveDb("insert into char_enemy_infos (id,cid,eid,type,ename,input) values ("
        + LEX_CAST_STR(enews.id) + ","
        + LEX_CAST_STR(enews.cid) + ","
        + LEX_CAST_STR(enews.eid) + ","
        + LEX_CAST_STR(enews.type) + ",'"
        + GetDb().safestr(enews.ename) + "',"
        + "from_unixtime(" + LEX_CAST_STR(enews.time_stamp) + "))");

    while (m_enemy_news.size() > 100)
    {
        char_enemy_news f = m_enemy_news.front();
        InsertSaveDb("delete from char_enemy_infos where id=" + LEX_CAST_STR(f.id));
        m_enemy_news.pop_front();
    }
}

void char_ralation::try_load_congratulations()
{
    if (!b_congratulations_loaded)
    {
        m_unread_congratulations = 0;
        Query q(GetDb());
        q.get_result("select id,fid,type,ctype,param1,param2,extra,unix_timestamp(input) from char_congratulations where cid="
                + LEX_CAST_STR(m_char_id) + " and (unix_timestamp(input)+2592000)>=unix_timestamp() order by id limit 100");
        int total = q.num_rows();
        int first_id = 0;
        while (q.fetch_row())
        {
            char_congratulation con;
            con.id = q.getval();
            if (first_id == 0)
            {
                first_id = con.id;
            }
            con.from_cid = q.getval();
            con.cid = m_char_id;
            con.type = q.getval();
            con.ctype = q.getval();
            if (con.type == 0)
            {
                ++m_unread_congratulations;
            }

            con.param[0] = q.getval();
            con.param[1] = q.getval();
            con.extra = q.getstr();
            con.time_stamp = q.getval();

            if (con.type == 1)
            {
                con.silver = con.param[1];
            }

            con.gen_msg();

            m_my_congratulations.push_back(con);
        }
        q.free_result();

        if (total == 100)
        {
            InsertSaveDb("delete from char_congratulations where cid=" + LEX_CAST_STR(m_char_id) + " and id<" + LEX_CAST_STR(first_id));
        }
        if (m_unread_congratulations)
        {
            CharData* pc = getChar().get();
            if (pc)
            {
                pc->m_need_notify[notify_msg_new_congratulation] = m_unread_congratulations;
            }
        }
        b_congratulations_loaded = true;
    }
}

void char_ralation::try_load_recved_congratulations()
{
    if (!b_recved_congratulations_loaded)
    {
        m_unread_recved_congratulations = 0;
        Query q(GetDb());
        q.get_result("select id,fid,ctype,param1,param2,unix_timestamp(input) from char_recved_congratulations where cid="
                + LEX_CAST_STR(m_char_id) + " order by id limit 100");
        while (q.fetch_row())
        {
            char_congratulation con;
            con.id = q.getval();
            con.from_cid = q.getval();
            con.cid = m_char_id;
            con.type = 1;
            con.ctype = q.getval();

            con.param[0] = q.getval();
            con.param[1] = q.getval();
            con.extra = "";
            con.time_stamp = q.getval();

            if (con.type == 1)
            {
                con.silver = con.param[1];
            }

            con.gen_msg();

            m_my_recved_congratulations.push_back(con);
        }
        q.free_result();

        if (m_my_recved_congratulations.size())
        {
            m_unread_recved_congratulations = m_my_recved_congratulations.size();
            CharData* pc = getChar().get();
            if (pc)
            {
                pc->m_need_notify[notify_msg_recv_congratulation] = m_unread_recved_congratulations;
            }
        }
        b_recved_congratulations_loaded = true;
    }
}


void char_ralation::try_load_enemy_news()
{
    if (!b_enemy_news_loaded)
    {
        Query q(GetDb());
        q.get_result("select id,eid,type,ename,unix_timestamp(input) from char_enemy_infos where cid="
                + LEX_CAST_STR(m_char_id) + " order by id limit 100");
        while (q.fetch_row())
        {
            char_enemy_news con;
            con.id = q.getval();
            con.eid = q.getval();
            con.cid = m_char_id;
            con.type = q.getval();
            con.ename = q.getstr();

            con.time_stamp = q.getval();

            con.gen_msg();

            m_enemy_news.push_back(con);
        }
        q.free_result();
        b_enemy_news_loaded = true;
    }
}


void char_ralation::try_load_enemys()
{
    if (!b_enemy_loaded)
    {
        Query q(GetDb());
        q.get_result("select eid,hate from char_enemys where cid="
                + LEX_CAST_STR(m_char_id));
        while (q.fetch_row())
        {
            int eid = q.getval();
            int hate = q.getval();
            my_enemy e(eid, hate);
            m_my_enemys[eid] = e;
        }
        q.free_result();

        q.get_result("select cid from char_enemys where eid="
                + LEX_CAST_STR(m_char_id) + " and hate>=" + LEX_CAST_STR(iEnemyHate));
        while (q.fetch_row())
        {
            int id = q.getval();
            m_who_hateme.push_back(id);
        }
        q.free_result();
        b_enemy_loaded = true;
    }
}

//增加对谁的仇恨
bool char_ralation::addHate(int eid, int hate)
{
    std::map<int, my_enemy>::iterator it = m_my_enemys.find(eid);
    if (it != m_my_enemys.end())
    {
        my_enemy& e = it->second;
        int oldhate = e.getHate();
        hate = e.addHate(hate);
        db_enemy(m_char_id, eid, hate);
        if (oldhate < iEnemyHate && hate >= iEnemyHate)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        my_enemy e(eid, hate);
        m_my_enemys[eid] = e;
        db_enemy(m_char_id, eid, hate);
    }
    return false;
}

relationMgr::relationMgr()
{
    reload();
}

void relationMgr::reload()
{
    boost::shared_ptr<char_ralation> r;
    int cid = 0, precid = 0;
    Query q(GetDb());
    //好友关系
    q.get_result("select cid,friend_id,state from char_friends where cid > 0 and friend_id > 0 order by cid,friend_id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        cid = q.getval();
        int friend_id = q.getval();
        int state = q.getval();

        if (cid != precid)
        {
            r = getRelation(cid);
        }
        precid = cid;
        boost::shared_ptr<char_ralation> fr = getRelation(friend_id);
        if (state)
        {
            r->add_attention(fr);
            if (fr->is_attention(cid))
            {
                r->add_friend(fr);
                fr->add_friend(r);
            }
        }
        else
        {
            r->add_application(fr);
        }
    }
    q.free_result();

    //祝贺消息模板
    q.get_result("select type,info1,info2 from base_congratulations where 1 order by type");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        assert(m_base_congratulations.size() + 1 == q.getval());
        m_base_congratulations.push_back(q.getstr());
        m_base_congratulations2.push_back(q.getstr());
    }
    q.free_result();

    //好友动态消息模板
    q.get_result("select type,info from base_friend_news where 1 order by type");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        assert(m_base_friend_news.size() + 1 == q.getval());
        m_base_friend_news.push_back(q.getstr());
    }
    q.free_result();

    //仇敌动态消息模板
    q.get_result("select type,info from base_enemy_news where 1 order by type");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        assert(m_base_enemy_news.size() + 1 == q.getval());
        m_base_enemy_news.push_back(q.getstr());
    }
    q.free_result();
    
    m_con_id = 0;    // 祝贺自增id
    m_recv_con_id = 0;    //收到的祝贺自增id
    m_enemy_news_id = 0;        //仇敌动态自增id

    q.get_result("select max(id) from char_congratulations where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_con_id = q.getval();
    }
    q.free_result();

    q.get_result("select max(id) from char_recved_congratulations where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_recv_con_id = q.getval();
    }
    q.free_result();

    q.get_result("select max(id) from char_enemy_infos where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_enemy_news_id = q.getval();
    }
    q.free_result();
}

boost::shared_ptr<char_ralation> relationMgr::getRelation(int cid)
{
    std::map<int, boost::shared_ptr<char_ralation> >::iterator it = m_relations.find(cid);
    if (it != m_relations.end())
    {
        return it->second;
    }
    boost::shared_ptr<char_ralation> r(new char_ralation(cid));
    m_relations[cid] = r;
    return r;
}

/*申请好友
int relationMgr::submitApplication(int cid, const std::string& name)
{
    int friend_id = GeneralDataMgr::getInstance()->GetCharId(name);
    if (cid == friend_id)
    {
        return HC_ERROR;
    }
    return acceptApplication(cid, friend_id);
}*/

//接受好友申请
int relationMgr::acceptApplication(int cid, int friend_id)
{
    if (cid == friend_id)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);

    if (my_rl->m_my_attentions.size() >= max_friend)
    {
        return HC_SUCCESS;
    }
    if (my_rl->is_attention(friend_id))
    {
        return HC_ERROR_FRIEND_YET;
    }

    //支线任务
    CharData* pc = my_rl->getChar().get();
    if (pc)
    {
        pc->m_trunk_tasks.updateTask(task_add_friends, 1);
    }
    
    boost::shared_ptr<char_ralation> rl = getRelation(friend_id);
    
    my_rl->remove_application(friend_id);
    //关注对方
    if (my_rl->add_attention(rl))
    {        
        db_attention(cid, friend_id);
        CharData* pc = rl->getChar().get();
        if (pc)
        {
            postFriendInfos(cid, pc->m_name, FRIEND_NEWS_I_ADD, friend_id, "");
        }
    }    
    //互相关注
    if (rl->is_attention(cid))
    {
        my_rl->add_friend(rl);
        rl->add_friend(my_rl);
    }
    else
    {
        if (rl->add_application(my_rl))
        {
            db_applicition(friend_id, cid);
            if (rl->getChar())
            {
                //通知被申请人
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(rl->getChar()->m_name);
                if (account)
                {
                    std::string msg = strFriendApply;
                    str_replace(msg, "$T", LEX_CAST_STR(time(NULL)));
                    account->Send(msg);
                }
            }
        }
    }
    if (my_rl->getChar().get() && rl->getChar().get()
        && my_rl->getChar()->m_area >= iOpenNearMap
        && my_rl->getChar()->m_area == rl->getChar()->m_area)
    {
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(my_rl->getChar()->m_name);
        if (account)
        {
            account->Send(strNotifyNearPlayer);
        }
    }
    return HC_SUCCESS;
}

//接受全部好友申请
int relationMgr::acceptAllApplication(int cid)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);

    //支线任务
    CharData* pc = my_rl->getChar().get();
    
    std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_pending_review.begin();
    while (my_rl->m_my_pending_review.size() > 0)
    {
        if (it->second.get())
        {
            if (my_rl->m_my_attentions.size() >= max_friend)
            {
                return HC_SUCCESS;
            }

            if (pc)
            {
                pc->m_trunk_tasks.updateTask(task_add_friends, 1);
            }
            //关注对方
            my_rl->add_attention(it->second);
            db_attention(cid, it->first);
            //互相关注
            if (it->second->is_attention(cid))
            {
                my_rl->add_friend(it->second);
                it->second->add_friend(my_rl);
            }
            else
            {
                if (it->second->add_application(my_rl))
                {
                    if (it->second->getChar())
                    {
                        //通知被申请人
                        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(it->second->getChar()->m_name);
                        if (account)
                        {
                            std::string msg = strFriendApply;
                            str_replace(msg, "$T", LEX_CAST_STR(time(NULL)));
                            account->Send(msg);
                        }
                    }
                    db_applicition(it->first, cid);
                }
            }
        }
        my_rl->m_my_pending_review.erase(it);
        it = my_rl->m_my_pending_review.begin();
    }
    db_remove_all_app(cid);
    if (my_rl->getChar().get())
    {
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(my_rl->getChar()->m_name);
        if (account)
        {
            account->Send(strNotifyNearPlayer);
        }
    }
    return HC_SUCCESS;
}

//拒绝好友申请
int relationMgr::rejectApplication(int cid, int aid)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);

    if (my_rl->remove_application(aid).get())
    {
        db_remove_attention(cid, aid);
    }
    return HC_SUCCESS;
}

//拒绝全部好友申请
int relationMgr::rejectAllApplication(int cid)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);

    my_rl->m_my_pending_review.clear();
    db_remove_all_app(cid);
    return HC_SUCCESS;
}

//移除关注
int relationMgr::removeAttention(int cid, int friend_id)
{
    if (cid == friend_id)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    boost::shared_ptr<char_ralation> rl = getRelation(friend_id);

    bool changed = false, changed2 = false;
    if (my_rl->remove_friend(friend_id).get())
    {
        changed = true;
    }
    if (my_rl->remove_attention(friend_id).get())
    {
        changed = true;
    }
    db_remove_attention(cid, friend_id);

    if (changed)
    {
        if (my_rl->getChar().get() && rl->getChar().get()
            && my_rl->getChar()->m_area >= iOpenNearMap
            && my_rl->getChar()->m_area == rl->getChar()->m_area)
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(my_rl->getChar()->m_name);
            if (account)
            {
                account->Send(strNotifyNearPlayer);
            }
        }
    }
    
    if (rl->remove_friend(cid).get())
    {
        changed2 = true;
    }
    if (rl->remove_listener(cid).get())
    {
        changed2 = true;
    }
    if (changed2)
    {
        if (my_rl->getChar().get() && rl->getChar().get()
            && my_rl->getChar()->m_area >= iOpenNearMap
            && my_rl->getChar()->m_area == rl->getChar()->m_area)
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(rl->getChar()->m_name);
            if (account)
            {
                account->Send(strNotifyNearPlayer);
            }
        }
    }
    return HC_SUCCESS;
}

//移除仇敌
int relationMgr::removeEnemy(int cid, int eid)
{
    char_ralation* my_rl = getRelation(cid).get();
    assert(my_rl);
    my_rl->try_load_enemys();
    if (my_rl->remove_enemy(eid))
    {
        char_ralation* his_rl = getRelation(eid).get();
        assert(his_rl);
        his_rl->remove_hate_me(cid);
    }
    return HC_SUCCESS;
}

//好友数量
int relationMgr::getFriendsCount(int cid)
{
    int cnt = 0;
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_attentions.begin();
            it != my_rl->m_my_attentions.end();
                ++it)
    {
        CharData* pc = it->second->getChar().get();
        if (pc && my_rl->is_friend(pc->m_id))
        {
            ++cnt;
        }
    }
    return cnt;
}

//好友列表
void relationMgr::getFriendsList(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_attentions.begin();
            it != my_rl->m_my_attentions.end();
                ++it)
    {
        CharData* pc = it->second->getChar().get();
        if (pc)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", pc->m_id) );
            obj.push_back( Pair("name", pc->m_name) );
            obj.push_back( Pair("gender", pc->m_gender) );
            std::string area_name = GeneralDataMgr::getInstance()->GetStageName(pc->m_area, pc->m_cur_stage);
            obj.push_back( Pair("area", area_name) );
            obj.push_back( Pair("level", pc->m_level) );
#ifdef QQ_PLAT
            obj.push_back( Pair("vip_is_year_yellow", pc->m_qq_yellow_year) );
            obj.push_back( Pair("vip_yellow_level", pc->m_qq_yellow_level) );
#endif
            obj.push_back( Pair("online", pc->m_is_online) );
            obj.push_back( Pair("both", my_rl->is_friend(pc->m_id)) );
            list.push_back(obj);

            if (list.size() >= max_friend)
            {
                break;
            }
        }
    }
    robj.push_back( Pair("list", list) );
    
}

//仇敌列表
void relationMgr::getEnemyList(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    json_spirit::Array list;
    my_rl->try_load_enemys();
    for (std::map<int, my_enemy>::iterator it = my_rl->m_my_enemys.begin();
            it != my_rl->m_my_enemys.end();
                ++it)
    {
        if (it->second.is_real_enemy())
        {
            CharData* pc = it->second.getChar().get();
            if (pc)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", pc->m_id) );
                obj.push_back( Pair("name", pc->m_name) );
                obj.push_back( Pair("gender", pc->m_gender) );
                std::string area_name = GeneralDataMgr::getInstance()->GetStageName(pc->m_area, pc->m_cur_stage);
                obj.push_back( Pair("area", area_name) );
                obj.push_back( Pair("online", pc->m_is_online) );
                boost::shared_ptr<char_goods> cg = guardMgr::getInstance()->GetCharGoods(pc->m_id);
                if (cg.get() && cg->m_state && cg->m_guard_goods.get() && cg->m_guard_goods->m_rob_time)
                {
                    obj.push_back( Pair("hs", 1) );
                }
                list.push_back(obj);
                if (list.size() >= max_enemy)
                {
                    break;
                }
            }
        }
    }
    json_spirit::Array hlist;
    std::list<int>::iterator it = my_rl->m_who_hateme.begin();
    while (it != my_rl->m_who_hateme.end())
    {
        hlist.push_back(*it);
        ++it;
    }
    robj.push_back( Pair("hlist", hlist) );
    robj.push_back( Pair("list", list) );
}

//申请列表
void relationMgr::getApplicationList(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_pending_review.begin();
            it != my_rl->m_my_pending_review.end();
                ++it)
    {
        CharData* pc = it->second->getChar().get();
        if (pc)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", pc->m_id) );
            obj.push_back( Pair("name", pc->m_name) );
            obj.push_back( Pair("gender", pc->m_gender) );
            std::string area_name = GeneralDataMgr::getInstance()->GetStageName(pc->m_area, pc->m_cur_stage);
            obj.push_back( Pair("area", area_name) );
            obj.push_back( Pair("level", pc->m_level) );
            obj.push_back( Pair("online", pc->m_is_online) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
}

//祝贺列表
void relationMgr::getCongratulationList(int cid, int page, int perPage, json_spirit::Object& robj)
{
    if (perPage < 0)
    {
        perPage = 10;
    }
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    my_rl->try_load_congratulations();
    
    int maxPage = my_rl->m_my_congratulations.size() / perPage + ((my_rl->m_my_congratulations.size() % perPage) ? 1 : 0);
    if (page > maxPage || page < 1)
    {
        page = 1;
    }

    int cur_nums = 0;
    int first_nums = perPage * (page - 1) + 1;
    int last_nums = perPage * page;

    int total_can = 0;

    time_t time_now = time(NULL);
    json_spirit::Array list;
    for (std::list<char_congratulation>::reverse_iterator it = my_rl->m_my_congratulations.rbegin();
            it != my_rl->m_my_congratulations.rend();
                ++it)
    {
        ++cur_nums;
        if (cur_nums >= first_nums && cur_nums <= last_nums)
        {
            char_congratulation& con = *it;
            json_spirit::Object obj;
            obj.push_back( Pair("id", con.id) );
            obj.push_back( Pair("msg", con.msg) );
            obj.push_back( Pair("name", con.from_name) );
            obj.push_back( Pair("cid", con.from_cid) );
            obj.push_back( Pair("type", con.type) );
            obj.push_back( Pair("ctype", con.ctype) );
            if (con.type == 0 && (con.time_stamp + iCongratulationKeepTime) >= time_now)
            {
                obj.push_back( Pair("con", 1) );
                ++total_can;
            }
            obj.push_back( Pair("param1", con.param[0]) );
            obj.push_back( Pair("param2", con.param[1]) );
            obj.push_back( Pair("time", con.time_stamp) );
            list.push_back(obj);
        }
        else if ((*it).type == 0 && ((*it).time_stamp + iCongratulationKeepTime) >= time_now)
        {
            ++total_can;
        }
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxPage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", perPage) );
    if (maxPage > 0)
        robj.push_back( Pair("page", pageobj) );

    CharData* pc = my_rl->getChar().get();
    if (pc)
    {
        json_spirit::Object info;
        info.push_back( Pair("received", pc->queryExtraData(char_data_type_daily, char_data_daily_congratulation_received)) );
        info.push_back( Pair("canReceive", maxCongratulations(pc->m_vip)) );
        info.push_back( Pair("sended", pc->queryExtraData(char_data_type_daily, char_data_daily_congratulation_sended)) );
        info.push_back( Pair("canSend", maxCongratulations(pc->m_vip)) );

        if (total_can > maxCongratulations(pc->m_vip))
        {
            total_can = maxCongratulations(pc->m_vip);
        }
        info.push_back( Pair("can", total_can) );

        robj.push_back( Pair("info", info) );

        //提醒收到新祝贺删除
        pc->m_need_notify.erase(notify_msg_new_congratulation);
        my_rl->m_unread_congratulations = 0;
    }
    
    robj.push_back( Pair("list", list) );
}

//接受祝贺列表
void relationMgr::getRecvedCongratulationList(int cid, int page, int perPage, json_spirit::Object& robj)
{
    (void)page;
    (void)perPage;
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    my_rl->try_load_recved_congratulations();

    int total = 0;
    json_spirit::Array list;
    for (std::list<char_congratulation>::reverse_iterator it = my_rl->m_my_recved_congratulations.rbegin();
            it != my_rl->m_my_recved_congratulations.rend();
                ++it)
    {
        char_congratulation& con = *it;
        json_spirit::Object obj;
        obj.push_back( Pair("id", con.id) );
        obj.push_back( Pair("msg", con.msg) );
        obj.push_back( Pair("name", con.from_name) );
        obj.push_back( Pair("cid", con.from_cid) );
        obj.push_back( Pair("type", con.type) );
        obj.push_back( Pair("ctype", con.ctype) );
        if (con.type == 0)
        {
            obj.push_back( Pair("con", 1) );
        }
        obj.push_back( Pair("param1", con.param[0]) );
        obj.push_back( Pair("silver", con.param[1]) );
        obj.push_back( Pair("time", con.time_stamp) );
        total += con.param[1];
        list.push_back(obj);
    }

    //删除所有接受到的祝贺
    InsertSaveDb("delete from char_recved_congratulations where cid=" + LEX_CAST_STR(cid));
    my_rl->m_my_recved_congratulations.clear();

    CharData* pc = my_rl->getChar().get();
    if (pc)
    {
        json_spirit::Object info;
        info.push_back( Pair("received", pc->queryExtraData(char_data_type_daily, char_data_daily_congratulation_received)) );
        info.push_back( Pair("canReceive", maxCongratulations(pc->m_vip)) );
        info.push_back( Pair("sended", pc->queryExtraData(char_data_type_daily, char_data_daily_congratulation_sended)) );
        info.push_back( Pair("canSend", maxCongratulations(pc->m_vip)) );

        robj.push_back( Pair("total", total) );

        robj.push_back( Pair("info", info) );

        //提醒收到新祝贺删除
        pc->m_need_notify.erase(notify_msg_recv_congratulation);
        my_rl->m_unread_recved_congratulations = 0;
    }
    
    robj.push_back( Pair("list", list) );
}

//仇敌动态
void relationMgr::getEnemyNewsList(int cid, int page, int perPage, json_spirit::Object& robj)
{
    if (perPage < 0)
    {
        perPage = 10;
    }
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    my_rl->try_load_enemy_news();
    
    int maxPage = my_rl->m_enemy_news.size() / perPage + ((my_rl->m_enemy_news.size() % perPage) ? 1 : 0);
    if (page > maxPage || page < 1)
    {
        page = 1;
    }

    int cur_nums = 0;
    int first_nums = perPage * (page - 1) + 1;
    int last_nums = perPage * page;
    
    json_spirit::Array list;
    for (std::list<char_enemy_news>::reverse_iterator it = my_rl->m_enemy_news.rbegin();
            it != my_rl->m_enemy_news.rend();
                ++it)
    {
        ++cur_nums;
        if (cur_nums >= first_nums && cur_nums <= last_nums)
        {
            char_enemy_news& con = *it;
            json_spirit::Object obj;
            obj.push_back( Pair("msg", con.msg) );
            obj.push_back( Pair("type", con.type) );
            obj.push_back( Pair("time", con.time_stamp) );
            list.push_back(obj);
        }
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxPage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", perPage) );
    if (maxPage > 0)
        robj.push_back( Pair("page", pageobj) );

    robj.push_back( Pair("list", list) );
}

//是否好友
bool relationMgr::is_my_friend(int cid1, int cid2)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid1);
    return my_rl->is_friend(cid2);
}

//是否我的关注
bool relationMgr::is_my_attention(int cid1, int cid2)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid1);
    return my_rl->is_attention(cid2);
}

//是否我的听众
bool relationMgr::is_my_listener(int cid1, int cid2)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid1);
    return my_rl->is_listener(cid2);
}

//是否我的仇敌
bool relationMgr::is_my_enemy(int cid1, int cid2)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid1);
    return my_rl->is_enemy(cid2);
}

//祝贺好友
int relationMgr::congratulation(int cid, int con_id, json_spirit::Object& robj)
{
    bool max_con = false;
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);

    CharData* pc = my_rl->getChar().get();
    if (!pc)
    {
        return HC_ERROR;
    }
    //还能祝贺吗
    int sended = pc->queryExtraData(char_data_type_daily, char_data_daily_congratulation_sended);
    int max_send = maxCongratulations(pc->m_vip);

    my_rl->try_load_congratulations();

    int silver = 0;

    if (0 == con_id && pc->gold() < 2)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    time_t time_now = time(NULL);
    std::list<char_congratulation>::iterator it = my_rl->m_my_congratulations.begin();
    while (it != my_rl->m_my_congratulations.end())
    {
        char_congratulation& con = *it;
        //找到这条等待祝贺的消息
        if (0 == con.type && (con.time_stamp + iCongratulationKeepTime) >= time_now && (0 == con_id || con.id == con_id))
        {
            boost::shared_ptr<char_ralation> his_rl = getRelation(con.from_cid);
            
            CharData* his_pc = his_rl->getChar().get();
            if (his_pc)
            {
                //他还能接受祝贺吗
                int received = his_pc->queryExtraData(char_data_type_daily, char_data_daily_congratulation_received);
                if (maxCongratulations(his_pc->m_vip) <= received)
                {
                    //对方不获得银币
                    his_rl->recv_congratulation(new_recv_con_id(),
                                                cid,
                                                0,
                                                con.ctype,
                                                con.param[0]); 
                }
                else
                {
                    his_pc->setExtraData(char_data_type_daily, char_data_daily_congratulation_received, received+1);
                    //对方获得银币
                    his_rl->recv_congratulation(new_recv_con_id(),
                                                cid,
                                                add_be_congratulationed_silver(*his_pc, con.ctype, pc->m_level),
                                                con.ctype,
                                                con.param[0]);
                }
                //提醒收到新祝贺
                his_pc->m_need_notify[notify_msg_recv_congratulation] = his_rl->m_my_recved_congratulations.size();
                //act统计
                act_to_tencent(his_pc,act_new_congratulation_recv);
            }
            ++sended;
            if (sended > max_send)
            {
                
            }
            else
            {
                //自己获得银币
                silver += add_congratulation_silver(*pc, con.ctype, pc->m_level);
            }
            
            //删除这条祝贺
            //db_remove_con((*it).id);
            //it = my_rl->m_my_congratulations.erase(it);

            //修改为已经祝贺
            con.type = 2;
            db_set_congralutioned(con.id);

            ++it;

            if (con_id > 0)
            {
                if (0 == silver)
                {
                    robj.push_back( Pair("msg", strCongratulationMaxMsg) );
                }
                else
                {
                    std::string msg = strCongratulationSuccessMsg;
                    str_replace(msg, "$S", LEX_CAST_STR(silver));
                    robj.push_back( Pair("msg", msg) );

                    pc->setExtraData(char_data_type_daily, char_data_daily_congratulation_sended, sended);
                }                
                return HC_SUCCESS;
            }            
        }
        else
        {
            ++it;
        }
    }
    if (silver)
    {
        std::string msg = (con_id > 0) ? strCongratulationSuccessMsg : strCongratulationAllSuccessMsg;
        str_replace(msg, "$S", LEX_CAST_STR(silver));
        robj.push_back( Pair("msg", msg) );

        //银币统计
        add_statistics_of_silver_get(pc->m_id, pc->m_ip_address, silver, silver_get_by_con_friend, pc->m_union_id, pc->m_server_id);

        if (0 == con_id)
        {
            pc->addGold(-2);
            //金币消耗统计
            add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 2, gold_cost_for_con_friend, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
            gold_cost_tencent(pc,2,gold_cost_for_con_friend);
#endif
        }
        pc->NotifyCharData();
        if (sended > max_send)
        {
            pc->setExtraData(char_data_type_daily, char_data_daily_congratulation_sended, max_send);
        }
        else
        {
            pc->setExtraData(char_data_type_daily, char_data_daily_congratulation_sended, sended);
        }
    }
    else
    {
        robj.push_back( Pair("msg", strCongratulationMaxMsg) );
    }
    return HC_SUCCESS;
}

//广播祝贺请求
void relationMgr::postCongradulation(int cid, int type, int param1, int param2)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_friends.begin();
            it != my_rl->m_my_friends.end();
                ++it)
    {
        it->second->add_congratulation(new_con_id(), cid, type, param1, param2);
    }
}

//广播好友动态
void relationMgr::postFriendInfos(int cid, const std::string& name, int type, int id, const std::string& param)
{
    /*
        type :    1加你好友
                2你加好友
                3加入军团
    */
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    //加入军团，好友广播
    switch (type)
    {
        case FRIEND_NEWS_JOIN_CORPS:
        {
            for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_friends.begin();
                it != my_rl->m_my_friends.end();
                    ++it)
            {
                it->second->add_friend_news(new_con_id(), cid, name, FRIEND_NEWS_JOIN_CORPS, param);
            }
            break;
        }
        case FRIEND_NEWS_I_ADD:
        {
            //加好友，发给两个人
            my_rl->add_friend_news(new_con_id(), id, name, FRIEND_NEWS_I_ADD, "");
            CharData* pc = my_rl->getChar().get();
            if (pc)
            {
                boost::shared_ptr<char_ralation> his_rl = getRelation(id);
                his_rl->add_friend_news(new_con_id(), cid, pc->m_name, FRIEND_NEWS_ADD_ME, "");
            }
            break;
        }
        case FRIEND_NEWS_INVITE_GUARD_SUCCESS:        //邀请好友护送成功
        //case FRIEND_NEWS_BE_INVITE_GUARD_SUCCESS:    //好友邀请我护送成功
        {
            //发给两个人
            my_rl->add_friend_news(new_con_id(), id, name, FRIEND_NEWS_INVITE_GUARD_SUCCESS, "");
            CharData* pc = my_rl->getChar().get();
            if (pc)
            {
                boost::shared_ptr<char_ralation> his_rl = getRelation(id);
                his_rl->add_friend_news(new_con_id(), cid, pc->m_name, FRIEND_NEWS_BE_INVITE_GUARD_SUCCESS, "");
            }
            break;
        }            
    }
}

//广播仇敌动态
void relationMgr::postEnemyInfos(int cid, const std::string& myname, int eid, const std::string& ename, int type)
{
    //cout<<"post enemy infos "<<cid<<", type "<<type<<endl;
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    my_rl->try_load_enemys();
    if (ENEMY_NEWS_ENEMY_GUARD == type)
    {
        //发送给所有把我当仇敌的人
        for (std::list<int>::iterator it = my_rl->m_who_hateme.begin();
              it != my_rl->m_who_hateme.end();
             ++it)
        {
            //cout<<"send enemy info to "<<(*it)<<endl;
            boost::shared_ptr<char_ralation> his_rl = getRelation(*it);
            his_rl->add_enemy_news(new_enemy_news_id(), cid, myname, type);
        }
    }
    else
    {
        //case ENEMY_NEWS_ADD_ENEMY:
        //case ENEMY_NEWS_ROB_ENEMY_FAIL:
        //case ENEMY_NEWS_SERVANT_ENEMY_FAIL:
        //case ENEMY_NEWS_SERVANT_ENEMY_SUCCESS:
        //case ENEMY_NEWS_ROB_ENEMY_SUCCESS:
        //添加到自己的仇敌动态
        if (my_rl->getChar().get())
        {
            my_rl->add_enemy_news(new_enemy_news_id(), eid, ename, type);
        }
    }
}

void relationMgr::getCongratulationMsg(int type, std::string& msg)
{
    if (type > 0 && type <= m_base_congratulations.size())
    {
        msg = m_base_congratulations[type-1];
    }
    else
    {
        msg = "";
    }
}

void relationMgr::getCongratulationMsg2(int type, std::string& msg)
{
    if (type > 0 && type <= m_base_congratulations2.size())
    {
        msg = m_base_congratulations2[type-1];
    }
    else
    {
        msg = "";
    }
}

void relationMgr::getFriendMsg(int type, std::string& msg)
{
    if (type > 0 && type <= m_base_friend_news.size())
    {
        msg = m_base_friend_news[type-1];
    }
    else
    {
        msg = "";
    }
}

void relationMgr::getEnemyMsg(int type, std::string& msg)
{
    if (type > 0 && type <= m_base_enemy_news.size())
    {
        msg = m_base_enemy_news[type-1];
    }
    else
    {
        msg = "";
    }
}

//增加仇恨
void relationMgr::addHate(int cid, int to, int hate)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    my_rl->try_load_enemys();
    if (my_rl->addHate(to, hate))
    {
        boost::shared_ptr<char_ralation> his_rl = getRelation(to);
        his_rl->add_hate_me(cid);

        if (his_rl->getChar().get() && my_rl->getChar().get())
        {
            //添加仇敌->仇敌动态
            Singleton<relationMgr>::Instance().postEnemyInfos(cid,
                                            "", to, his_rl->getChar().get()->m_name, ENEMY_NEWS_ADD_ENEMY);
            //添加仇敌通知
            sendSystemMail(my_rl->getChar().get()->m_name, cid, strAddEnemy, strAddEnemyMailContent, "", 0, 3, to);
        }
    }
}

//祝贺好友
int ProcessCongratulation(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    //act统计
    act_to_tencent(cdata.get(),act_new_congratulation);
    return Singleton<relationMgr>::Instance().congratulation(cdata->m_id, id, robj);
}

//祝贺列表
int ProcessGetCongratulations(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int page = 1, pageNums = 10;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(pageNums, o, "pageNums");
    Singleton<relationMgr>::Instance().getCongratulationList(cdata->m_id, page, pageNums, robj);
    return HC_SUCCESS;
}

//接收到的祝贺列表
int ProcessGetRecvedCongratulations(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int page = 1, pageNums = 10;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(pageNums, o, "pageNums");
    Singleton<relationMgr>::Instance().getRecvedCongratulationList(cdata->m_id, page, pageNums, robj);
    return HC_SUCCESS;
}

//仇敌列表
int ProcessGetEnemyList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    Singleton<relationMgr>::Instance().getEnemyList(cdata->m_id, robj);    
    return HC_SUCCESS;
}

//仇敌动态列表
int ProcessGetEnemyInfoList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int page = 1, pageNums = 10;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(pageNums, o, "pageNums");
    Singleton<relationMgr>::Instance().getEnemyNewsList(cdata->m_id, page, pageNums, robj);
    return HC_SUCCESS;
}

//移除仇敌
int ProcessRemoveEnemy(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int eid = 0;
    READ_INT_FROM_MOBJ(eid,o,"id");
    Singleton<relationMgr>::Instance().removeEnemy(cdata->m_id, eid);
    return HC_SUCCESS;
}

//推荐好友列表
int ProcessGetRecommendFriends(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    GeneralDataMgr::getInstance()->getRecommendFriends(cdata.get(), robj);
    return HC_SUCCESS;
}

