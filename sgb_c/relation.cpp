
#include "relation.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "singleton.h"
#include "copy.h"

using namespace net;

Database& GetDb();
extern int getSessionChar(session_ptr& psession, CharData* &pc);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);

void InsertSaveDb(const std::string& sql);

//祝贺成功获得银币
extern std::string strCongratulationSuccessMsg;
//一键祝贺成功
extern std::string strCongratulationAllSuccessMsg;
//今日祝贺收益次数满
extern std::string strCongratulationMaxMsg;

//祝贺保持时间16小时，超过16小时，无法祝贺
const int iCongratulationKeepTime = 16 * 3600;
const int iCongratulationOneKeyCost = 5;

static const int max_friend = 100;

//好友申请提示
const std::string strFriendApply = "{\"time\":$T,\"cmd\":\"friendApply\",\"s\":200}";

static void db_attention(int cid1, int cid2)
{
    InsertSaveDb("replace into char_relation_friends (cid,friend_id,state) values ("
        + LEX_CAST_STR(cid1) + "," + LEX_CAST_STR(cid2) + ",1)");
}

static void db_remove_attention(int cid1, int cid2)
{
    InsertSaveDb("delete from char_relation_friends where cid=" + LEX_CAST_STR(cid1)
        + " and friend_id=" + LEX_CAST_STR(cid2));
}

static void db_applicition(int cid1, int cid2)
{
    InsertSaveDb("replace into char_relation_friends (cid,friend_id,state) values ("
        + LEX_CAST_STR(cid1) + "," + LEX_CAST_STR(cid2) + ",0)");
}

static void db_blacklist(int cid1, int cid2)
{
    InsertSaveDb("replace into char_relation_blacklist (cid,friend_id) values ("
        + LEX_CAST_STR(cid1) + "," + LEX_CAST_STR(cid2) + ")");
}

static void db_remove_blacklist(int cid1, int cid2)
{
    InsertSaveDb("delete from char_relation_blacklist where cid=" + LEX_CAST_STR(cid1)
        + " and friend_id=" + LEX_CAST_STR(cid2));
}

static void db_remove_all_app(int cid)
{
    InsertSaveDb("delete from char_relation_friends where cid="    + LEX_CAST_STR(cid)
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
    cdata.addSilver(silver, silver_get_by_friend);
    return silver;
}

static int add_be_congratulationed_silver(CharData& cdata, int type, int level)
{
    int silver = level * 20;
    cdata.addSilver(silver, silver_get_by_friend);
    return silver;
}

inline int maxCongratulations(int vip)
{
    int max_times[iMaxVIP+1] = {50,80,120,160,200,250,300,300,300,300,300};
    if (vip > iMaxVIP)
    {
        vip = iMaxVIP;
    }
    else if (vip < 0)
    {
        vip = 0;
    }
    return max_times[vip];
}

int ProcessGetFriendsList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int purpose = 0;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    robj.push_back( Pair("purpose", purpose));
    switch (purpose)
    {
        case 1://好友列表
        {
            Singleton<relationMgr>::Instance().getFriendsList(pc->m_id, robj);
            break;
        }
        case 2://申请列表
        {
            Singleton<relationMgr>::Instance().getApplicationList(pc->m_id, robj);
            break;
        }
        case 3://黑名单列表
        {
            Singleton<relationMgr>::Instance().getBlackList(pc->m_id, robj);
            break;
        }
        case 4://最近聊天列表
        {
            Singleton<relationMgr>::Instance().getRecentlyList(pc->m_id, robj);
            break;
        }
        case 5://推荐好友列表
        {
            Singleton<relationMgr>::Instance().getRecommendList(pc, robj);
            break;
        }
    }
    return HC_SUCCESS;
}

int ProcessDealFriends(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int purpose = 0, id = 0;
    std::string name = "";
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    READ_INT_FROM_MOBJ(id,o,"id");
    READ_STR_FROM_MOBJ(name,o,"name");
    
    robj.push_back( Pair("purpose", purpose));
    switch (purpose)
    {
        case 1://提交申请
        {
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            if (id == 0)
                return HC_ERROR_NAME;
            return Singleton<relationMgr>::Instance().acceptApplication(pc->m_id, id);
        }
        case 2://通过申请
        {
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            return Singleton<relationMgr>::Instance().acceptApplication(pc->m_id, id);
        }
        case 3://通过全部申请
        {
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            return Singleton<relationMgr>::Instance().acceptAllApplication(pc->m_id);
        }
        case 4://拒绝申请
        {
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            return Singleton<relationMgr>::Instance().rejectApplication(pc->m_id, id);
        }
        case 5://拒绝所有申请
        {
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            return Singleton<relationMgr>::Instance().rejectAllApplication(pc->m_id);
        }
        case 6://删除好友
        {
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            if (id == 0)
                return HC_ERROR_NAME;
            return Singleton<relationMgr>::Instance().removeAttention(pc->m_id, id);
        }
        case 7://添加黑名单
        {
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            if (id == 0)
                return HC_ERROR_NAME;
            return Singleton<relationMgr>::Instance().addBlacklist(pc->m_id, id);
        }
        case 8://移出黑名单
        {
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            if (id == 0)
                return HC_ERROR_NAME;
            return Singleton<relationMgr>::Instance().removeBlacklist(pc->m_id, id);
        }
        case 9://提交全部推荐好友申请
        {
            id = GeneralDataMgr::getInstance()->GetCharId(name);
            robj.push_back( Pair("id", id));
            return Singleton<relationMgr>::Instance().acceptAllRecommendApplication(pc);
        }
    }
    return HC_SUCCESS;
}

//祝贺好友
int ProcessCongratulation(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    return Singleton<relationMgr>::Instance().congratulation(pc->m_id, id, robj);
}

//祝贺列表
int ProcessGetCongratulations(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int page = 1, pageNums = 10;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(pageNums, o, "pageNums");
    Singleton<relationMgr>::Instance().getCongratulationList(pc->m_id, page, pageNums, robj);
    return HC_SUCCESS;
}

//接收到的祝贺列表
int ProcessGetRecvedCongratulations(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    Singleton<relationMgr>::Instance().getRecvedCongratulationList(pc->m_id, robj);
    return HC_SUCCESS;
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
            case CONGRATULATION_LEVEL:// = 1,
            {
                str_replace(msg, "$L", LEX_CAST_STR(param[0]));
                break;
            }
            case CONGRATULATION_SHENLING:// = 2,
            {
                str_replace(msg, "$L", LEX_CAST_STR(param[0]));
                break;
            }
            case CONGRATULATION_COPY:// = 3,
            case CONGRATULATION_COPY_PERFECT:// = 4,
            {
                boost::shared_ptr<baseCopy> bc = Singleton<copyMgr>::Instance().getCopyById(param[0]);
                if (bc.get())
                {
                    str_replace(msg, "$C", bc->m_name);
                }
                break;
            }
            case CONGRATULATION_PASS_MAP:// = 5,
            {
                boost::shared_ptr<baseMap> bm = Singleton<mapMgr>::Instance().GetBaseMap(param[0]);
                if (bm.get())
                {
                    str_replace(msg, "$M", bm->m_name);
                }
                break;
            }
            case CONGRATULATION_UPGRADE_EQUIPMENT:// = 6,
            {
                str_replace(msg, "$L", LEX_CAST_STR(param[0]));
                break;
            }
            case CONGRATULATION_EPIC_HERO:// = 16,
            {
                boost::shared_ptr<baseHeroData> bh = Singleton<HeroMgr>::Instance().GetBaseHero(param[0]);
                if (bh.get())
                {
                    std::string hero_link = bh->m_name;
                    addColor(hero_link,bh->m_quality);
                    str_replace(msg, "$H", hero_link);
                }
                break;
            }
            case CONGRATULATION_TOP_ARENA:// = 17,
            {
                str_replace(msg, "$R", LEX_CAST_STR(param[0]));
                break;
            }
            case CONGRATULATION_VIP:// = 18,
            {
                str_replace(msg, "$V", LEX_CAST_STR(param[0]));
                break;
            }
            case CONGRATULATION_SKILL:// = 19,
            {
                str_replace(msg, "$L", LEX_CAST_STR(param[0]));
                break;
            }
            case CONGRATULATION_PRESTIGE1:// = 20,
            case CONGRATULATION_PRESTIGE2:// = 21,
            case CONGRATULATION_PRESTIGE3:// = 22,
            case CONGRATULATION_PRESTIGE4:// = 23,
            {
                str_replace(msg, "$L", LEX_CAST_STR(param[0]));
                break;
            }
            default:
                break;
        }
    }
    //别人发来的祝贺
    else if (1 == type)
    {
        Singleton<relationMgr>::Instance().getCongratulationMsg2(ctype, msg);
        str_replace(msg, "$W", MakeCharNameLink(pc->m_name));
        str_replace(msg, "$S", LEX_CAST_STR(silver));
    }
}

char_ralation::char_ralation(int id)
{
    b_getChar = false;
    m_char_id = id;
    m_recommend_refresh = 0;
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

//加黑名单
bool char_ralation::add_blacklist(boost::shared_ptr<char_ralation> r)
{
    if (r.get() && !is_blacklist(r->m_char_id))
    {
        m_my_blacklist[r->m_char_id] = r;
        return true;
    }
    return false;
}

//加最近聊天
void char_ralation::add_recently(boost::shared_ptr<char_ralation> r)
{
    if (r.get() && (m_my_recently.find(r->m_char_id) == m_my_recently.end()))
    {
        m_my_recently[r->m_char_id] = r;
    }
}

bool char_ralation::is_attention(int id)
{
    return (m_my_attentions.find(id) != m_my_attentions.end());
}

bool char_ralation::is_blacklist(int id)
{
    return (m_my_blacklist.find(id) != m_my_blacklist.end());
}

bool char_ralation::is_friend(int id)
{
    return (m_my_friends.find(id) != m_my_friends.end());
}

bool char_ralation::is_application(int id)
{
    return (m_my_pending_review.find(id) != m_my_pending_review.end());
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

//移除黑名单
boost::shared_ptr<char_ralation> char_ralation::remove_blacklist(int id)
{
    boost::shared_ptr<char_ralation> rl;
    std::map<int, boost::shared_ptr<char_ralation> >::iterator it = m_my_blacklist.find(id);
    if (it != m_my_blacklist.end())
    {
        rl = it->second;
        m_my_blacklist.erase(it);
    }
    return rl;
}

void char_ralation::get_recommend(CharData* cdata)
{
    if (time(NULL) > (m_recommend_refresh + 300))
    {
        GeneralDataMgr::getInstance()->getRecommendFriends(cdata, m_my_recommend);
        m_recommend_refresh = time(NULL);
    }
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
    q.get_result("select cid,friend_id,state from char_relation_friends where cid > 0 and friend_id > 0 order by cid,friend_id");
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
            cout << cid << " add attention " << friend_id << endl;
            if (fr->is_attention(cid))
            {
                cout << cid << " link friend " << friend_id << endl;
                r->add_friend(fr);
                fr->add_friend(r);
            }
        }
        else
        {
            r->add_application(fr);
            cout << cid << " add application " << friend_id << endl;
        }
    }
    q.free_result();
    
    //黑名单
    q.get_result("select cid,friend_id from char_relation_blacklist where cid > 0 and friend_id > 0 order by cid,friend_id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        cid = q.getval();
        int friend_id = q.getval();

        if (cid != precid)
        {
            r = getRelation(cid);
        }
        precid = cid;
        boost::shared_ptr<char_ralation> fr = getRelation(friend_id);
        r->add_blacklist(fr);
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
    
    m_con_id = 0;    // 祝贺自增id
    m_recv_con_id = 0;    //收到的祝贺自增id

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

//接受/提交好友申请
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
        return HC_ERROR;
    }
    
    boost::shared_ptr<char_ralation> rl = getRelation(friend_id);
    
    my_rl->remove_application(friend_id);
    //关注对方
    if (my_rl->add_attention(rl))
    {
        db_attention(cid, friend_id);
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
    return HC_SUCCESS;
}

//接受全部好友申请
int relationMgr::acceptAllApplication(int cid)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_pending_review.begin();
    while (my_rl->m_my_pending_review.size() > 0)
    {
        if (it->second.get())
        {
            if (my_rl->m_my_attentions.size() >= max_friend)
            {
                return HC_SUCCESS;
            }
            acceptApplication(cid,it->first);
        }
        it = my_rl->m_my_pending_review.begin();
    }
    db_remove_all_app(cid);
    return HC_SUCCESS;
}

//提交全部推荐好友申请
int relationMgr::acceptAllRecommendApplication(CharData* cdata)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cdata->m_id);
    my_rl->get_recommend(cdata);
    std::map<int, boost::shared_ptr<CharData> >::iterator it = my_rl->m_my_recommend.begin();
    while (it != my_rl->m_my_recommend.end())
    {
        if (it->second.get())
        {
            if (my_rl->m_my_attentions.size() >= max_friend)
            {
                return HC_SUCCESS;
            }
            acceptApplication(cdata->m_id,it->first);
        }
        ++it;
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
    
    if (rl->remove_friend(cid).get())
    {
        changed2 = true;
    }
    if (rl->remove_attention(cid).get())
    {
        changed2 = true;
    }
    return HC_SUCCESS;
}

//加入黑名单
int relationMgr::addBlacklist(int cid, int eid)
{
    char_ralation* my_rl = getRelation(cid).get();
    assert(my_rl);
    boost::shared_ptr<char_ralation> rl = getRelation(eid);
    if (my_rl->add_blacklist(rl))
    {
        db_blacklist(cid,eid);
        removeAttention(cid,eid);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}


//移除黑名单
int relationMgr::removeBlacklist(int cid, int eid)
{
    char_ralation* my_rl = getRelation(cid).get();
    assert(my_rl);
    if (my_rl->remove_blacklist(eid).get())
    {
        db_remove_blacklist(cid,eid);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}


//加入最近聊天
int relationMgr::addRecently(int cid, int rid)
{
    char_ralation* my_rl = getRelation(cid).get();
    assert(my_rl);
    boost::shared_ptr<char_ralation> rl = getRelation(rid);
    my_rl->add_recently(rl);
    return HC_SUCCESS;
}

//好友数量
int relationMgr::getFriendsCount(int cid)
{
    int cnt = 0;
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_friends.begin();
            it != my_rl->m_my_friends.end();
                ++it)
    {
        CharData* pc = it->second->getChar().get();
        if (pc)
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
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_friends.begin();
            it != my_rl->m_my_friends.end();
                ++it)
    {
        CharData* pc = it->second->getChar().get();
        if (pc && !my_rl->is_blacklist(pc->m_id))
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", pc->m_id) );
            obj.push_back( Pair("name", pc->m_name) );
            obj.push_back( Pair("gender", pc->m_gender) );
            obj.push_back( Pair("level", pc->m_level) );
            obj.push_back( Pair("online", pc->m_is_online) );
#ifdef QQ_PLAT
            obj.push_back( Pair("vip_is_year_yellow", pc->m_qq_yellow_year) );
            obj.push_back( Pair("vip_yellow_level", pc->m_qq_yellow_level) );
#endif
            list.push_back(obj);
            if (list.size() >= max_friend)
            {
                break;
            }
        }
    }
    robj.push_back( Pair("list", list) );
    
}

//黑名单列表
void relationMgr::getBlackList(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_blacklist.begin();
            it != my_rl->m_my_blacklist.end();
                ++it)
    {
        CharData* pc = it->second->getChar().get();
        if (pc)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", pc->m_id) );
            obj.push_back( Pair("name", pc->m_name) );
            obj.push_back( Pair("gender", pc->m_gender) );
            obj.push_back( Pair("level", pc->m_level) );
            obj.push_back( Pair("online", pc->m_is_online) );
            list.push_back(obj);
        }
    }
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
            obj.push_back( Pair("level", pc->m_level) );
            obj.push_back( Pair("online", pc->m_is_online) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
}

//最近聊天列表
void relationMgr::getRecentlyList(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid);
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_recently.begin();
            it != my_rl->m_my_recently.end();
                ++it)
    {
        CharData* pc = it->second->getChar().get();
        if (pc)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", pc->m_id) );
            obj.push_back( Pair("name", pc->m_name) );
            obj.push_back( Pair("gender", pc->m_gender) );
            obj.push_back( Pair("level", pc->m_level) );
            obj.push_back( Pair("online", pc->m_is_online) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
}

//好友推送
void relationMgr::getRecommendList(CharData* cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cdata->m_id);
    my_rl->get_recommend(cdata);
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<CharData> >::iterator it = my_rl->m_my_recommend.begin();
            it != my_rl->m_my_recommend.end();
                ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", it->second->m_id) );
            obj.push_back( Pair("name", it->second->m_name) );
            obj.push_back( Pair("gender", it->second->m_gender) );
            obj.push_back( Pair("level", it->second->m_level) );
            obj.push_back( Pair("online", it->second->m_is_online) );
            list.push_back(obj);
        }
    }
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

//是否我的黑名单
bool relationMgr::is_my_blacklist(int cid1, int cid2)
{
    boost::shared_ptr<char_ralation> my_rl = getRelation(cid1);
    return my_rl->is_blacklist(cid2);
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
        info.push_back( Pair("cost", iCongratulationOneKeyCost) );

        robj.push_back( Pair("info", info) );

        //提醒收到新祝贺删除
        pc->m_need_notify.erase(notify_msg_new_congratulation);
        my_rl->m_unread_congratulations = 0;
    }
    
    robj.push_back( Pair("list", list) );
}

//接受祝贺列表
void relationMgr::getRecvedCongratulationList(int cid, json_spirit::Object& robj)
{
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

    if (0 == con_id && pc->gold() < iCongratulationOneKeyCost)
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

            //修改为已经祝贺
            con.type = 2;
            db_set_congralutioned(con.id);

            ++it;

            if (con_id > 0)//单条祝贺
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
    if (silver)//一键祝贺成功
    {
        std::string msg = strCongratulationAllSuccessMsg;
        str_replace(msg, "$S", LEX_CAST_STR(silver));
        robj.push_back( Pair("msg", msg) );
        pc->subGold(iCongratulationOneKeyCost, gold_cost_for_con_friend);
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
        if (it->second.get())
        {
            it->second->add_congratulation(new_con_id(), cid, type, param1, param2);
        }
        else
        {
            cout << it->first << " char_ralation is NULL" << endl;
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

