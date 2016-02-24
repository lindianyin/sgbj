
#include "pk.h"
#include "utils_all.h"
#include "errcode_def.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_lang.h"
#include <syslog.h>
#include "singleton.h"
#include "rewards.h"
#include "relation.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
extern void InsertMultiCombat(MultiCombatCmd& pcombatCmd);

const int iPKWaitTime = 20;
const int iPKOpenLevel = 7;

//获得挑战列表
int ProcessQueryPKList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().querySysList(pc->m_id, o, robj);
}

//查询自己的挑战信息
int ProcessQueryPKInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().querySelfInfo(pc->m_id, robj);
}

//查询排行前3
int ProcessQueryPKTop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().queryTopList(pc->m_id, robj);
}

//pk邀请操作
int ProcessDealPK(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharPkData> charPk = Singleton<PkMgr>::Instance().getPkData(pc->m_id);
    if (!charPk.get() || !charPk->getCharData().get())
    {
        return HC_ERROR;
    }
    int purpose = 1, toid = 0, roomid = 0;
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    READ_INT_FROM_MOBJ(toid, o, "toid");
    READ_INT_FROM_MOBJ(roomid, o, "roomid");
    robj.push_back( Pair("purpose", purpose) );
    robj.push_back( Pair("toid", toid) );
    robj.push_back( Pair("roomid", roomid) );
    if (purpose == 1)//发送邀请
    {
        return charPk->sendInvite(toid);
    }
    else if(purpose == 2)//普通邀请只需邀请人在房间
    {
        boost::shared_ptr<CharPkData> inviter = Singleton<PkMgr>::Instance().getPkData(toid);
        if (!inviter.get() || !inviter->getCharData().get())
        {
            return HC_ERROR;
        }
        if (inviter->m_roomid != roomid)
        {
            return HC_ERROR_NO_PK_ROOM;
        }
        return charPk->acceptInvite(roomid);
    }
    else if(purpose == 3)//聊天邀请需要邀请人是房主
    {
        boost::shared_ptr<CharPkData> inviter = Singleton<PkMgr>::Instance().getPkData(toid);
        if (!inviter.get() || !inviter->getCharData().get())
        {
            return HC_ERROR;
        }
        PkRoom* room = Singleton<PkMgr>::Instance().getRoom(roomid);
        if (room == NULL || room->m_own_cid != toid)
        {
            return HC_ERROR_NO_PK_ROOM;
        }
        return charPk->acceptInvite(roomid);
    }
    return HC_ERROR;
}

//获得房间列表
int ProcessQueryPKRooms(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().queryRooms(o, robj);
}

//查询房间信息
int ProcessQueryRoomInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().queryRoomInfo(pc->m_id, robj);
}

int ProcessPKCreateRoom(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().createRoom(pc->m_id, o, robj);
}

int ProcessPKJoinRoom(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    int roomid = 0, type = 0;
    std::string password = "";
    READ_INT_FROM_MOBJ(roomid, o, "roomid");
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_STR_FROM_MOBJ(password, o, "password");
    robj.push_back( Pair("roomid", roomid) );
    robj.push_back( Pair("type", type) );
    return Singleton<PkMgr>::Instance().joinRoom(pc->m_id, roomid, password);
}

int ProcessPKLeaveRoom(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().leaveRoom(pc->m_id);
}

int ProcessPKKickPlayer(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().kickPlayer(pc->m_id, o);
}

int ProcessPKChangeSet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().changeSet(pc->m_id, o);
}

int ProcessPKOpenSet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().openSet(pc->m_id, o);
}

int ProcessPKCloseSet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().closeSet(pc->m_id, o);
}

int ProcessPKSetPassword(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().setPassword(pc->m_id, o);
}

int ProcessPKSetBet(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().setBet(pc->m_id, o);
}

int ProcessPKSetReady(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().setReady(pc->m_id, o);
}

int ProcessPKStart(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().startCombat(pc->m_id);
}

int ProcessPKInvite(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    if (pc->m_level < iPKOpenLevel)
    {
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().invite(pc->m_id);
}

bool compare_pk_get(boost::shared_ptr<CharPkGet> a, boost::shared_ptr<CharPkGet> b)
{
    if (!a.get())
    {
        return false;
    }
    if (!b.get())
    {
        return true;
    }
    if (a->m_total_get > b->m_total_get)
    {
        return true;
    }
    else if (a->m_total_get < b->m_total_get)
    {
        return false;
    }
    else
    {
        return true;
    }
}

CharPkData::CharPkData(int cid)
:m_cid(cid)
{
    m_total_get = 0;
    m_bet = 0;
    m_state = PK_STATE_INIT;
    m_roomid = 0;
    m_pos = 0;
}

boost::shared_ptr<CharData> CharPkData::getCharData()
{
    if (m_charactor.get())
    {
        return m_charactor;
    }
    else
    {
        m_charactor = GeneralDataMgr::getInstance()->GetCharData(m_cid);
        return m_charactor;
    }
}

void CharPkData::toObj(json_spirit::Object& robj)
{
    robj.push_back( Pair("id", m_charactor->m_id) );
    robj.push_back( Pair("name", m_charactor->m_name) );
    robj.push_back( Pair("spic", m_charactor->m_spic) );
    robj.push_back( Pair("silver", m_charactor->silver()) );
    robj.push_back( Pair("level", m_charactor->m_level) );
    robj.push_back( Pair("state", m_state) );
    boost::shared_ptr<CharHeroData> p_hero = m_charactor->m_heros.GetHero(m_charactor->m_heros.m_default_hero);
    if (p_hero.get())
    {
        json_spirit::Object hero;
        p_hero->toObj(hero);
        robj.push_back( Pair("heroInfo", hero) );
    }
}

void CharPkData::toSimpleObj(json_spirit::Object& robj)
{
    robj.push_back( Pair("id", m_charactor->m_id) );
    robj.push_back( Pair("name", m_charactor->m_name) );
    robj.push_back( Pair("spic", m_charactor->m_spic) );
    robj.push_back( Pair("silver", m_charactor->silver()) );
    robj.push_back( Pair("level", m_charactor->m_level) );
    robj.push_back( Pair("state", m_state) );
}

int CharPkData::sendInvite(int toid)
{
    //自己状态判断
    if (m_state != PK_STATE_ROOM)
    {
        ERR();
        return HC_ERROR;
    }
    //对方状态判断
    boost::shared_ptr<CharPkData> toData = Singleton<PkMgr>::Instance().getPkData(toid);
    if (!toData.get() || !toData->getCharData().get())
    {
        ERR();
        return HC_ERROR;
    }
    if (toData->m_state != PK_STATE_INIT)
    {
        return HC_ERROR_TARGET_IN_PK;
    }
    if (!toData->canChallenge())
    {
        ERR();
        return HC_ERROR;
    }
    //通知双方客户端
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "pkInvite") );
    obj.push_back( Pair("bet", m_bet) );
    obj.push_back( Pair("silver", getCharData()->silver()) );
    obj.push_back( Pair("roomid", m_roomid) );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("end_time", iPKWaitTime) );
    json_spirit::Object o;
    toObj(o);
    obj.push_back( Pair("info", o) );
    toData->getCharData()->sendObj(obj);
    return HC_SUCCESS;
}

int CharPkData::acceptInvite(int roomid)
{
    //自己状态判断
    if (!getCharData().get())
    {
        ERR();
        return HC_ERROR;
    }
    if (m_state != PK_STATE_INIT)
    {
        ERR();
        return HC_ERROR;
    }
    return Singleton<PkMgr>::Instance().joinRoom(m_charactor->m_id, roomid, "", true);
}

void CharPkData::load()
{
    Query q(GetDb());
    q.get_result("select totalGet,bet from char_pk where cid="+LEX_CAST_STR(m_cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        m_total_get = q.getval();
        m_bet = q.getval();
    }
    q.free_result();
    return;
}

void CharPkData::save()
{
    InsertSaveDb("replace into char_pk (cid,totalGet,bet,updateTime) values ("
                    + LEX_CAST_STR(m_cid) + "," + LEX_CAST_STR(m_total_get)
                    + "," + LEX_CAST_STR(m_bet)
                    + ",unix_timestamp()"
                    + ")");
    return;
}

bool CharPkData::canChallenge()
{
    multi_combat* myCombat = Singleton<MultiCombatMgr>::Instance().getCharMultiCombat(m_cid);
    if (myCombat != NULL && myCombat->m_type != COMBAT_TYPE_PK)
    {
        return false;
    }
    getCharData();
    return (m_charactor->m_level >= iPKOpenLevel && m_charactor->silver() >= 1000);
}

void PkRoom::toObj(json_spirit::Object& robj)
{
    robj.push_back( Pair("id", m_id));
    robj.push_back( Pair("name", m_name));
    robj.push_back( Pair("bet", m_bet));
    robj.push_back( Pair("locked", m_password != ""));
    json_spirit::Array list;
    for (int j = 0; j < iPKMaxSet; ++j)
    {
        int pos = j+1;
        json_spirit::Object o;
        o.push_back( Pair("pos", pos));
        o.push_back( Pair("ready", m_sets[j].m_ready));
        o.push_back( Pair("state", m_sets[j].m_state));
        if (m_sets[j].m_playes.get() && m_sets[j].m_playes->getCharData().get())
        {
            json_spirit::Object charInfo;
            m_sets[j].m_playes->toObj(charInfo);
            o.push_back( Pair("charInfo", charInfo));
            o.push_back( Pair("owner", m_sets[j].m_playes->m_cid == m_own_cid));
        }
        list.push_back(o);
    }
    robj.push_back( Pair("list", list));
}

void PkRoom::broadInfo()
{
    json_spirit::Object obj;
    toObj(obj);
    obj.push_back( Pair("cmd", "queryRoomInfo"));
    obj.push_back( Pair("s",200) );
    //_channel->BroadMsg(write(obj, json_spirit::raw_utf8));
    for (int j = 0; j < iPKMaxSet; ++j)
    {
        if (m_sets[j].m_playes.get() && m_sets[j].m_playes->getCharData().get())
        {
            m_sets[j].m_playes->getCharData()->sendObj(obj);
        }
    }
}

PkMgr::PkMgr()
{
    Query q(GetDb());
    int rank_rewards_max = 0;
    q.get_result("SELECT count(distinct(rank)) FROM base_pk_rankings_rewards where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        rank_rewards_max = q.getval();
    }
    for (int i = 1; i <= rank_rewards_max; ++i)
    {
        PkRankRewards rrr;
        rrr.rank = i;
        rrr.reward.clear();
        q.get_result("select itemType,itemId,counts,extra from base_pk_rankings_rewards where rank="+LEX_CAST_STR(i)+" order by id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            Item item;
            item.type = q.getval();
            item.id = q.getval();
            item.nums = q.getval();
            item.extra = q.getval();
            rrr.reward.push_back(item);
        }
        q.free_result();
        if (rrr.reward.size())
        {
            m_rewards.push_back(rrr);
            m_last_rank = rrr.rank;
        }
    }
    //加载玩家获得
    int rank = 1;
    q.get_result("select cid,totalGet from char_pk where 1 order by totalGet desc");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int cid = q.getval();
        int total_get = q.getval();
        boost::shared_ptr<CharPkGet> rd(new CharPkGet);
        rd->m_cid = cid;
        rd->m_total_get = total_get;
        rd->m_rank = rank;
        m_pk_get.push_back(rd);
        ++rank;
    }
    q.free_result();
    //读取上次结算称号
    std::string top = GeneralDataMgr::getInstance()->getStr("last_pk_top");
    if (top != "")
    {
        json_spirit::Value v;
        json_spirit::read(top, v);
        if (v.type() == json_spirit::array_type)
        {
            json_spirit::Array& a = v.get_array();
            for (json_spirit::Array::iterator it = a.begin(); it != a.end(); ++it)
            {
                json_spirit::Value v2 = *it;
                if (v2.type() == json_spirit::int_type)
                {
                    m_last_top.push_back(v2.get_int());
                }
                else
                {
                    m_last_top.clear();
                    break;
                }
            }
        }
    }
    if (m_last_top.size())
    {
        for (int i = 0; i < m_last_top.size(); ++i)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_last_top[i]).get();
            if (pc)
            {
                pc->m_nick.add_nick(nick_pk_start + i);
                pc->SaveNick();
            }
        }
    }
    for (int i = 0; i < iPKMaxRoom; ++i)
    {
        m_rooms[i].m_id = i+1;
        m_rooms[i].m_name = "";
        m_rooms[i].m_password = "";
        m_rooms[i].m_cur = 0;
        m_rooms[i].m_max = 0;
        m_rooms[i].m_own_cid = 0;
        m_rooms[i].m_state = 0;
        m_rooms[i].m_combat_id = 0;
        m_rooms[i].m_bet = 0;
        m_rooms[i]._channel.reset(new ChatChannel("room", m_rooms[i].m_id, "{\"cmd\":\"chat\",\"ctype\":"+LEX_CAST_STR(channel_room)+",\"s\":200,"));
        m_rooms[i]._channel->start();
        for (int j = 0; j < iPKMaxSet; ++j)
        {
            m_rooms[i].m_sets[j].m_pos = j+1;
            m_rooms[i].m_sets[j].reset();
        }
    }
}

boost::shared_ptr<CharPkData> PkMgr::getPkData(int cid)
{
    boost::unordered_map<int, boost::shared_ptr<CharPkData> >::iterator it = m_pk_maps.find(cid);
    if (it != m_pk_maps.end() && it->second.get())
    {
        it->second->getCharData();
        return it->second;
    }
    else
    {
        boost::shared_ptr<CharPkData> rd;
        return rd;
    }
}

boost::shared_ptr<CharPkData> PkMgr::getPkDataIndex(int index)
{
    if (index >= 0 && (index + 1) <= m_pk_maps.size())
    {
        boost::unordered_map<int, boost::shared_ptr<CharPkData> >::iterator it = m_pk_maps.begin();
        while (it != m_pk_maps.end())
        {
            if (!it->second.get())
            {
                ++it;
                continue;
            }
            if (index == 0)
            {
                it->second->getCharData();
                return it->second;
            }
            --index;
            ++it;
        }
    }
    boost::shared_ptr<CharPkData> rd;
    return rd;
}

boost::shared_ptr<CharPkData> PkMgr::addCharactor(int cid)
{
    boost::unordered_map<int, boost::shared_ptr<CharPkData> >::iterator it = m_pk_maps.find(cid);
    if (it != m_pk_maps.end() && it->second.get() != NULL)
    {
        ERR();
        return it->second;
    }
    else
    {
        boost::shared_ptr<CharPkData> rd(new CharPkData(cid));
        m_pk_maps[cid] = rd;
        rd->load();
        return rd;
    }
}

boost::shared_ptr<CharPkGet> PkMgr::getPkGet(int cid)
{
    std::list<boost::shared_ptr<CharPkGet> >::iterator it = m_pk_get.begin();
    while (it != m_pk_get.end())
    {
        if ((*it).get() && (*it)->m_cid == cid)
        {
            return (*it);
        }
        ++it;
    }
    boost::shared_ptr<CharPkGet> rd;
    return rd;
}

//更新排名
void PkMgr::updateRank()
{
    m_pk_get.sort(compare_pk_get);
    int rank = 0;
    std::list<boost::shared_ptr<CharPkGet> >::iterator it = m_pk_get.begin();
    while (it != m_pk_get.end())
    {
        if (it->get())
        {
            it->get()->m_rank = ++rank;
        }
        ++it;
    }
    return;
}

void PkMgr::removeCharactor(int cid)
{
    boost::unordered_map<int, boost::shared_ptr<CharPkData> >::iterator it = m_pk_maps.find(cid);
    if (it != m_pk_maps.end() && it->second.get() != NULL)
    {
        m_pk_maps.erase(cid);
    }
}

int PkMgr::querySelfInfo(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get())
    {
        return HC_ERROR;
    }

    CharData* pc = rd->getCharData().get();
    if (!pc)
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("bet", rd->m_bet) );
    robj.push_back( Pair("state", rd->m_state) );
    robj.push_back( Pair("total_get", rd->m_total_get) );
    return HC_SUCCESS;
}

//查询可对战玩家列表
int PkMgr::querySysList(int cid, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int type = 1;
    READ_INT_FROM_MOBJ(type, o, "type");
    //页面信息
    int page = 1, nums_per_page = 8;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    std::string search_name = "";
    READ_STR_FROM_MOBJ(search_name, o, "search");
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
    if (type == 1)//系统推荐
    {
        boost::unordered_map<int, boost::shared_ptr<CharPkData> >::iterator it = m_pk_maps.begin();
        while (it != m_pk_maps.end())
        {
            if (!it->second.get())
            {
                ++it;
                continue;
            }
            if (!it->second->canChallenge())
            {
                ++it;
                continue;
            }
            if (it->second->m_state == PK_STATE_ROOM)
            {
                ++it;
                continue;
            }
            it->second->getCharData();
            if (it->second->getCharData()->m_is_online == 0)
            {
                ++it;
                continue;
            }
            if (it->second->m_cid == cid)
            {
                ++it;
                continue;
            }
            if (search_name != "" && it->second->getCharData()->m_name.find(search_name) == std::string::npos)
            {
                ++it;
                continue;
            }
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                json_spirit::Object o;
                it->second->toSimpleObj(o);
                list.push_back(o);
            }
            ++it;
        }
    }
    else if(type == 2)//好友列表
    {
        boost::shared_ptr<char_ralation> my_rl = Singleton<relationMgr>::Instance().getRelation(cid);
        for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_friends.begin();
                it != my_rl->m_my_friends.end();
                    ++it)
        {
            CharData* pc = it->second->getChar().get();
            if (pc && pc->m_is_online)
            {
                boost::shared_ptr<CharPkData> tmp = getPkData(pc->m_id);
                if (!tmp.get())
                {
                    continue;
                }
                if (!tmp->canChallenge())
                {
                    continue;
                }
                if (tmp->m_state == PK_STATE_ROOM)
                {
                    continue;
                }
                tmp->getCharData();
                if (search_name != "" && pc->m_name.find(search_name) == std::string::npos)
                {
                    continue;
                }
                ++cur_nums;
                if (cur_nums >= first_nums && cur_nums <= last_nums)
                {
                    json_spirit::Object o;
                    tmp->toSimpleObj(o);
                    list.push_back(o);
                }
            }
        }
    }
    else if(type == 3)//公会列表
    {
        boost::shared_ptr<CharPkData> rd = getPkData(cid);
        if (!rd.get() || !rd->getCharData().get())
            return HC_ERROR;
        Guild* cp = Singleton<guildMgr>::Instance().getGuild(rd->getCharData()->GetGuildId());
        if (!cp)
        {
            return HC_ERROR_NOT_JOIN_GUILD;
        }
        std::list<boost::shared_ptr<CharGuildData> >::iterator it_l = cp->m_members_list.begin();
        while (it_l != cp->m_members_list.end())
        {
            if (it_l->get() && it_l->get()->cdata.get())
            {
                CharData* pc = it_l->get()->cdata.get();
                if (pc && pc->m_is_online)
                {
                    boost::shared_ptr<CharPkData> tmp = getPkData(pc->m_id);
                    if (!tmp.get())
                    {
                        ++it_l;
                        continue;
                    }
                    if (!tmp->canChallenge())
                    {
                        ++it_l;
                        continue;
                    }
                    if (tmp->m_state == PK_STATE_ROOM)
                    {
                        ++it_l;
                        continue;
                    }
                    tmp->getCharData();
                    if (search_name != "" && pc->m_name.find(search_name) == std::string::npos)
                    {
                        ++it_l;
                        continue;
                    }
                    ++cur_nums;
                    if (cur_nums >= first_nums && cur_nums <= last_nums)
                    {
                        json_spirit::Object o;
                        tmp->toSimpleObj(o);
                        list.push_back(o);
                    }
                }
            }
            ++it_l;
        }
    }
    robj.push_back( Pair("list", list));
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//查询房间列表
int PkMgr::queryRooms(json_spirit::mObject& o, json_spirit::Object& robj)
{
    //页面信息
    int page = 1, nums_per_page = 8;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    int search_id = 0;
    READ_INT_FROM_MOBJ(search_id, o, "search");
    int search_state = 0;
    READ_INT_FROM_MOBJ(search_state, o, "search_state");
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
    for (int i = 0; i < iPKMaxRoom; ++i)
    {
        if (m_rooms[i].m_state == 0)
        {
            continue;
        }
        if (search_state != 0 && m_rooms[i].m_state != search_state)
        {
            continue;
        }
        if (search_id != 0 && m_rooms[i].m_id != search_id)
        {
            continue;
        }
        ++cur_nums;
        if (cur_nums >= first_nums && cur_nums <= last_nums)
        {
            json_spirit::Object o;
            o.push_back( Pair("id", m_rooms[i].m_id));
            o.push_back( Pair("name", m_rooms[i].m_name));
            o.push_back( Pair("cur", m_rooms[i].m_cur));
            o.push_back( Pair("max", m_rooms[i].m_max));
            o.push_back( Pair("state", m_rooms[i].m_state));
            o.push_back( Pair("locked", m_rooms[i].m_password != ""));
            list.push_back(o);
        }
    }
    robj.push_back( Pair("list", list));
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

//查询房间信息
int PkMgr::queryRoomInfo(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_roomid == 0)
        return HC_ERROR;
    int roomid = rd->m_roomid;
    if (roomid > 0 && roomid < iPKMaxRoom)
    {
        int i = roomid - 1;
        if (m_rooms[i].m_state > 0)
        {
            m_rooms[i].toObj(robj);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//创建房间
int PkMgr::createRoom(int cid, json_spirit::mObject& o, json_spirit::Object& robj)
{
    std::string name = "", password = "";
    READ_STR_FROM_MOBJ(name, o, "name");
    READ_STR_FROM_MOBJ(password, o, "password");
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_state == PK_STATE_ROOM)
        return HC_ERROR;
    for (int i = 0; i < iPKMaxRoom; ++i)
    {
        if (m_rooms[i].m_state == 0)
        {
            m_rooms[i].m_name = name;
            m_rooms[i].m_password = password;
            m_rooms[i].m_cur = 1;
            m_rooms[i].m_max = 2;
            m_rooms[i].m_own_cid = cid;
            m_rooms[i].m_state = 1;
            m_rooms[i].m_bet = rd->m_bet;
            if (m_rooms[i]._channel.get())
            {
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(rd->getCharData()->m_name);
                if (account.get())
                {
                    m_rooms[i]._channel->Add(account);
                }
            }
            if (rd->m_state == PK_STATE_INIT)
            {
                rd->m_state = PK_STATE_ROOM;
                rd->m_roomid = m_rooms[i].m_id;
                rd->m_pos = 1;
            }
            for (int j = 0; j < iPKMaxSet; ++j)
            {
                if (m_rooms[i].m_sets[j].m_pos == 1)//第一个默认房主
                {
                    m_rooms[i].m_sets[j].m_state = 1;
                    m_rooms[i].m_sets[j].m_playes = rd;//房主
                    m_rooms[i].m_sets[j].m_ready = true;
                }
                else if(m_rooms[i].m_sets[j].m_pos > m_rooms[i].m_max)//超过房间大小的关闭
                {
                    m_rooms[i].m_sets[j].m_state = -1;
                }
            }
            robj.push_back( Pair("roomid", m_rooms[i].m_id));
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//加入房间
int PkMgr::joinRoom(int cid, int roomid, std::string password, bool invite)
{
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_state == PK_STATE_ROOM)
        return HC_ERROR;
    if (roomid == 0)
    {
        //自动加入
        for (int i = 0; i < iPKMaxRoom; ++i)
        {
            if (m_rooms[i].m_state != 0 && m_rooms[i].m_password == "" && m_rooms[i].m_cur < m_rooms[i].m_max)
            {
                if (m_rooms[i].m_state == 2 && rd->getCharData()->silver() < m_rooms[i].m_bet)
                {
                    return HC_ERROR_NOT_ENOUGH_SILVER;
                }
                for (int j = 0; j < iPKMaxSet; ++j)
                {
                    if (m_rooms[i].m_sets[j].m_state == 0)
                    {
                        m_rooms[i].m_sets[j].m_state = 1;
                        m_rooms[i].m_sets[j].m_playes = rd;
                        m_rooms[i].m_sets[j].m_ready = false;
                        if (m_rooms[i]._channel.get())
                        {
                            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(rd->getCharData()->m_name);
                            if (account.get())
                            {
                                m_rooms[i]._channel->Add(account);
                            }
                        }
                        if (rd->m_state == PK_STATE_INIT)
                        {
                            rd->m_state = PK_STATE_ROOM;
                            rd->m_roomid = m_rooms[i].m_id;
                            rd->m_pos = m_rooms[i].m_sets[j].m_pos;
                        }
                        ++m_rooms[i].m_cur;
                        if (m_rooms[i].m_state == 1)
                        {
                            //通知玩家
                            m_rooms[i].broadInfo();
                        }
                        else if(m_rooms[i].m_state == 2 && m_rooms[i].m_combat_id > 0)
                        {
                            //需要加载进战斗场景
                            multi_combat* pCombat = Singleton<MultiCombatMgr>::Instance().findMultiCombat(m_rooms[i].m_combat_id);
                            if (pCombat && pCombat->m_state != COMBAT_STATE_END)
                            {
                                //加入战斗管理
                                boost::shared_ptr<combatPlayer> p;
                                p.reset(new combatPlayer);
                                p->LoadCharactor(*(rd->getCharData().get()));
                                p->m_auto = false;
                                pCombat->addPlayer(m_rooms[i].m_sets[j].m_pos, p);

                                //房间战斗通知
                                json_spirit::Object obj;
                                obj.push_back( Pair("cmd", "PKCombatNotify"));
                                obj.push_back( Pair("id",pCombat->m_combat_id) );
                                obj.push_back( Pair("s",200) );
                                rd->getCharData()->sendObj(obj);
                            }
                        }
                        return HC_SUCCESS;
                    }
                }
            }
        }
    }
    else if (roomid > 0 && roomid < iPKMaxRoom)
    {
        int i = roomid - 1;
        if (m_rooms[i].m_state == 0)
        {
            return HC_ERROR_NO_PK_ROOM;
        }
        if (m_rooms[i].m_cur >= m_rooms[i].m_max)
        {
            return HC_ERROR_PK_ROOM_FULL;
        }
        if (m_rooms[i].m_cur < m_rooms[i].m_max)
        {
            if (!invite && m_rooms[i].m_password != password)
            {
                if (password == "")
                {
                    return HC_ERROR_NEED_PASSWORD;
                }
                return HC_ERROR_WRONG_PASSWORD;
            }
            for (int j = 0; j < iPKMaxSet; ++j)
            {
                if (m_rooms[i].m_sets[j].m_state == 0)
                {
                    m_rooms[i].m_sets[j].m_state = 1;
                    m_rooms[i].m_sets[j].m_playes = rd;
                    m_rooms[i].m_sets[j].m_ready = false;
                    if (m_rooms[i]._channel.get())
                    {
                        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(rd->getCharData()->m_name);
                        if (account.get())
                        {
                            m_rooms[i]._channel->Add(account);
                        }
                    }
                    if (rd->m_state == PK_STATE_INIT)
                    {
                        rd->m_state = PK_STATE_ROOM;
                        rd->m_roomid = m_rooms[i].m_id;
                        rd->m_pos = m_rooms[i].m_sets[j].m_pos;
                    }
                    ++m_rooms[i].m_cur;
                    if (m_rooms[i].m_state == 1)
                    {
                        //通知玩家
                        m_rooms[i].broadInfo();
                    }
                    else if(m_rooms[i].m_state == 2 && m_rooms[i].m_combat_id > 0)
                    {
                        //需要加载进战斗场景
                        multi_combat* pCombat = Singleton<MultiCombatMgr>::Instance().findMultiCombat(m_rooms[i].m_combat_id);
                        if (pCombat && pCombat->m_state != COMBAT_STATE_END)
                        {
                            //加入战斗管理
                            boost::shared_ptr<combatPlayer> p;
                            p.reset(new combatPlayer);
                            p->LoadCharactor(*(rd->getCharData().get()));
                            p->m_auto = false;
                            pCombat->addPlayer(m_rooms[i].m_sets[j].m_pos, p);

                            //房间战斗通知
                            json_spirit::Object obj;
                            obj.push_back( Pair("cmd", "PKCombatNotify"));
                            obj.push_back( Pair("id",pCombat->m_combat_id) );
                            obj.push_back( Pair("s",200) );
                            rd->getCharData()->sendObj(obj);
                        }
                    }
                    return HC_SUCCESS;
                }
            }
        }
    }
    return HC_ERROR_NO_PK_ROOM;
}

//离开房间
int PkMgr::leaveRoom(int cid)
{
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_state == PK_STATE_INIT || rd->m_roomid == 0)
    {
        return HC_ERROR;
    }
    bool notify = false;
    bool changeOwner = false;
    int room_index = rd->m_roomid - 1;
    int set_index = rd->m_pos - 1;
    if (rd->m_roomid > 0 && rd->m_roomid <= iPKMaxRoom)
    {
        if (m_rooms[room_index].m_state != 0)
        {
            if (rd->m_pos > 0 && rd->m_pos <= iPKMaxSet)
            {
                if (m_rooms[room_index].m_sets[set_index].m_playes.get() && m_rooms[room_index].m_sets[set_index].m_playes == rd)
                {
                    if (m_rooms[room_index]._channel.get())
                    {
                        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(rd->getCharData()->m_name);
                        if (account.get())
                        {
                            m_rooms[room_index]._channel->Remove(account);
                        }
                    }
                    m_rooms[room_index].m_sets[set_index].reset();
                    if (rd->m_state == PK_STATE_ROOM)
                    {
                        rd->m_state = PK_STATE_INIT;
                        rd->m_roomid = 0;
                        rd->m_pos = 0;
                    }
                    --m_rooms[room_index].m_cur;
                    notify = true;
                    //玩家是房主则转让房主
                    if (m_rooms[room_index].m_own_cid == cid)
                    {
                        changeOwner = true;
                    }
                }
            }
            if (!notify)
            {
                return HC_ERROR;
            }
            if (changeOwner)
            {
                if (m_rooms[room_index].m_cur <= 0)//清空
                {
                    m_rooms[room_index].m_name = "";
                    m_rooms[room_index].m_password = "";
                    m_rooms[room_index].m_cur = 0;
                    m_rooms[room_index].m_max = 0;
                    m_rooms[room_index].m_own_cid = 0;
                    m_rooms[room_index].m_state = 0;
                    m_rooms[room_index].m_bet = 0;
                }
                else//转让
                {
                    for (int j = 0; j < iPKMaxSet; ++j)
                    {
                        if (m_rooms[room_index].m_sets[j].m_state == 1
                            && m_rooms[room_index].m_sets[j].m_playes.get() && m_rooms[room_index].m_sets[j].m_playes->getCharData().get()
                            && m_rooms[room_index].m_sets[j].m_playes->m_cid != m_rooms[room_index].m_own_cid)
                        {
                            //新房主
                            m_rooms[room_index].m_own_cid = m_rooms[room_index].m_sets[j].m_playes->m_cid;
                            m_rooms[room_index].m_sets[j].m_ready = true;
                        }
                    }
                }
            }
            //通知玩家
            m_rooms[room_index].broadInfo();
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//踢人
int PkMgr::kickPlayer(int cid, json_spirit::mObject& o)
{
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_roomid == 0)
        return HC_ERROR;
    int room_index = rd->m_roomid - 1;
    int set_index = pos - 1;
    if (rd->m_roomid > 0 && rd->m_roomid <= iPKMaxRoom)
    {
        if (m_rooms[room_index].m_state == 1 && set_index >= 0 && set_index < iPKMaxSet && pos != rd->m_pos)
        {
            if (m_rooms[room_index].m_own_cid == cid)
            {
                if (m_rooms[room_index].m_sets[set_index].m_state == 1
                    && m_rooms[room_index].m_sets[set_index].m_playes.get()
                    && m_rooms[room_index].m_sets[set_index].m_playes->getCharData().get())
                {
                    if (m_rooms[room_index]._channel.get())
                    {
                        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_rooms[room_index].m_sets[set_index].m_playes->getCharData()->m_name);
                        if (account.get())
                        {
                            m_rooms[room_index]._channel->Remove(account);
                        }
                    }
                    if (m_rooms[room_index].m_sets[set_index].m_playes->m_state == PK_STATE_ROOM)
                    {
                        m_rooms[room_index].m_sets[set_index].m_playes->m_state = PK_STATE_INIT;
                        m_rooms[room_index].m_sets[set_index].m_playes->m_roomid = 0;
                        m_rooms[room_index].m_sets[set_index].m_playes->m_pos = 0;
                    }
                    //通知被踢玩家
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "PKKicked"));
                    obj.push_back( Pair("s",200) );
                    m_rooms[room_index].m_sets[set_index].m_playes->getCharData()->sendObj(obj);
                    //重置位置
                    m_rooms[room_index].m_sets[set_index].reset();
                    --m_rooms[room_index].m_cur;
                    //通知房间其他玩家
                    m_rooms[room_index].broadInfo();
                    return HC_SUCCESS;
                }
            }
        }
    }
    return HC_ERROR;
}

//换位置
int PkMgr::changeSet(int cid, json_spirit::mObject& o)
{
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_roomid == 0)
        return HC_ERROR;
    int room_index = rd->m_roomid - 1;
    int set_index = rd->m_pos - 1;
    int new_index = pos - 1;
    if (rd->m_roomid > 0 && rd->m_roomid <= iPKMaxRoom)
    {
        if (m_rooms[room_index].m_state == 1 && set_index >= 0 && set_index < iPKMaxSet)
        {
            //判断新位置情况
            if (new_index >= 0 && new_index < iPKMaxSet && new_index != set_index)
            {
                if (m_rooms[room_index].m_sets[new_index].m_state != 0)
                {
                    return HC_ERROR;
                }
                m_rooms[room_index].m_sets[new_index].m_state = 1;
                m_rooms[room_index].m_sets[new_index].m_playes = rd;
                m_rooms[room_index].m_sets[new_index].m_ready = m_rooms[room_index].m_sets[set_index].m_ready;
                rd->m_pos = pos;
            }
            if (m_rooms[room_index].m_sets[set_index].m_state == 1
                && m_rooms[room_index].m_sets[set_index].m_playes.get()
                && m_rooms[room_index].m_sets[set_index].m_playes->getCharData().get())
            {
                m_rooms[room_index].m_sets[set_index].reset();
            }
            //通知玩家
            m_rooms[room_index].broadInfo();
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//打开位置
int PkMgr::openSet(int cid, json_spirit::mObject& o)
{
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_roomid == 0)
        return HC_ERROR;
    int room_index = rd->m_roomid - 1;
    int new_index = pos - 1;
    if (rd->m_roomid > 0 && rd->m_roomid <= iPKMaxRoom)
    {
        if (m_rooms[room_index].m_own_cid != cid)
        {
            return HC_ERROR;
        }
        if (m_rooms[room_index].m_state == 1)
        {
            //判断新位置情况
            if (new_index >= 0 && new_index < iPKMaxSet)
            {
                if (m_rooms[room_index].m_sets[new_index].m_state == -1)
                {
                    m_rooms[room_index].m_sets[new_index].m_state = 0;
                    m_rooms[room_index].m_sets[new_index].m_ready = false;
                    ++m_rooms[room_index].m_max;
                    //通知玩家
                    m_rooms[room_index].broadInfo();
                    return HC_SUCCESS;
                }
            }
        }
    }
    return HC_ERROR;
}

//关闭位置
int PkMgr::closeSet(int cid, json_spirit::mObject& o)
{
    int pos = 0;
    READ_INT_FROM_MOBJ(pos, o, "pos");
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_roomid == 0)
        return HC_ERROR;
    int room_index = rd->m_roomid - 1;
    int new_index = pos - 1;
    if (rd->m_roomid > 0 && rd->m_roomid <= iPKMaxRoom)
    {
        if (m_rooms[room_index].m_own_cid != cid)
        {
            return HC_ERROR;
        }
        if (m_rooms[room_index].m_state == 1)
        {
            //判断新位置情况
            if (new_index >= 0 && new_index < iPKMaxSet)
            {
                if (m_rooms[room_index].m_sets[new_index].m_state == 0)
                {
                    m_rooms[room_index].m_sets[new_index].m_state = -1;
                    m_rooms[room_index].m_sets[new_index].m_ready = false;
                    --m_rooms[room_index].m_max;
                    //通知玩家
                    m_rooms[room_index].broadInfo();
                    return HC_SUCCESS;
                }
            }
        }
    }
    return HC_ERROR;
}

//设定密码
int PkMgr::setPassword(int cid, json_spirit::mObject& o)
{
    std::string password = "";
    READ_STR_FROM_MOBJ(password, o, "password");
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_roomid == 0)
        return HC_ERROR;
    int room_index = rd->m_roomid - 1;
    if (rd->m_roomid > 0 && rd->m_roomid < iPKMaxRoom)
    {
        if (m_rooms[room_index].m_state == 1)
        {
            if (m_rooms[room_index].m_own_cid == cid)
            {
                m_rooms[room_index].m_password = password;
                //通知玩家
                m_rooms[room_index].broadInfo();
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

//设定筹码底注
int PkMgr::setBet(int cid, json_spirit::mObject& o)
{
    int bet = 0;
    READ_INT_FROM_MOBJ(bet, o, "bet");
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_roomid == 0)
        return HC_ERROR;
    int room_index = rd->m_roomid - 1;
    if (rd->m_roomid > 0 && rd->m_roomid < iPKMaxRoom)
    {
        if (m_rooms[room_index].m_state == 1)
        {
            if (m_rooms[room_index].m_own_cid == cid)
            {
                rd->m_bet = bet;
                m_rooms[room_index].m_bet = bet;
                //需要其他玩家再次确认
                for (int j = 0; j < iPKMaxSet; ++j)
                {
                    if (m_rooms[room_index].m_sets[j].m_state == 1
                        && m_rooms[room_index].m_sets[j].m_playes.get()
                        && m_rooms[room_index].m_sets[j].m_playes->m_cid != m_rooms[room_index].m_own_cid)
                    {
                        m_rooms[room_index].m_sets[j].m_ready = false;
                    }
                }
                //通知玩家
                m_rooms[room_index].broadInfo();
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

int PkMgr::setReady(int cid, json_spirit::mObject& o)
{
    int purpose = 1;
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_roomid == 0)
        return HC_ERROR;
    int room_index = rd->m_roomid - 1;
    int set_index = rd->m_pos - 1;
    if (rd->m_roomid > 0 && rd->m_roomid < iPKMaxRoom)
    {
        if (m_rooms[room_index].m_state == 1 && set_index >= 0 && set_index < iPKMaxSet)
        {
            if (m_rooms[room_index].m_own_cid == cid)
            {
                return HC_ERROR;
            }
            if (m_rooms[room_index].m_sets[set_index].m_state == 1
                && m_rooms[room_index].m_sets[set_index].m_playes.get()
                && m_rooms[room_index].m_sets[set_index].m_playes->m_cid == cid)
            {
                if (purpose == 1)
                {
                    m_rooms[room_index].m_sets[set_index].m_ready = true;
                }
                else if (purpose == 2)
                {
                    m_rooms[room_index].m_sets[set_index].m_ready = false;
                }
                else
                {
                    return HC_ERROR;
                }
                //通知玩家
                m_rooms[room_index].broadInfo();
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

//开始战斗
int PkMgr::startCombat(int cid)
{
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_roomid == 0)
        return HC_ERROR;
    int room_index = rd->m_roomid - 1;
    if (rd->m_roomid > 0 && rd->m_roomid < iPKMaxRoom)
    {
        if (m_rooms[room_index].m_state == 1)
        {
            if (m_rooms[room_index].m_own_cid != cid)
            {
                return HC_ERROR;
            }
            if (m_rooms[room_index].m_cur < 2)
            {
                return HC_ERROR;
            }
            std::vector<int> cid_list;
            for (int j = 0; j < iPKMaxSet; ++j)
            {
                if (m_rooms[room_index].m_sets[j].m_state == 1
                    && m_rooms[room_index].m_sets[j].m_playes.get())
                {
                    if (!m_rooms[room_index].m_sets[j].m_ready)
                    {
                        return HC_ERROR_PK_START_NOT_READY;
                    }
                    cid_list.push_back(m_rooms[room_index].m_sets[j].m_playes->m_cid);
                }
                else
                {
                    cid_list.push_back(0);
                }
            }
            m_rooms[room_index].m_state = 2;
            #if 0
            //当前只有1v1
            if (toid > 0)
            {
                //开始战斗
                json_spirit::mObject o;
                o["cmd"] = "challenge";
                o["to"] = toid;
                o["type"] = COMBAT_TYPE_PK;
                o["bet"] = m_rooms[room_index].m_bet;
                o["roomid"] = m_rooms[room_index].m_id;

                combatCmd cmd;
                cmd.mobj = o;
                cmd.cname = rd->getCharData()->m_name;
                cmd.cid = rd->getCharData()->m_id;
                cmd.cmd = combat_cmd_create;
                cmd._pCombat = NULL;
                InsertCombat(cmd);
            }
            #endif
            if (cid_list.size() > 1)
            {
                //开始战斗
                json_spirit::mObject o;
                o["cmd"] = "multi_challenge";
                json_spirit::mArray list;
                for (int i = 0; i < cid_list.size(); ++i)
                {
                    json_spirit::mObject tmp;
                    tmp["id"] = cid_list[i];
                    list.push_back(tmp);
                }
                o["list"] = list;
                o["type"] = COMBAT_TYPE_PK;
                o["bet"] = m_rooms[room_index].m_bet;
                o["roomid"] = m_rooms[room_index].m_id;

                MultiCombatCmd cmd;
                cmd.mobj = o;
                cmd.cname = rd->getCharData()->m_name;
                cmd.cid = rd->getCharData()->m_id;
                cmd.cmd = combat_cmd_create;
                cmd._pCombat = NULL;
                InsertMultiCombat(cmd);
            }
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//开始战斗
int PkMgr::invite(int cid)
{
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData().get() || rd->m_roomid == 0)
        return HC_ERROR;
    int room_index = rd->m_roomid - 1;
    if (rd->m_roomid > 0 && rd->m_roomid < iPKMaxRoom)
    {
        if (m_rooms[room_index].m_state == 1)
        {
            if (m_rooms[room_index].m_own_cid != cid)
            {
                return HC_ERROR;
            }
            if (m_rooms[room_index].m_cur >=  m_rooms[room_index].m_max)
            {
                return HC_ERROR;
            }
            std::string inviteMsg = strPkInviteMsg;
            str_replace(inviteMsg, "$N", MakeCharNameLink(rd->getCharData()->m_name,rd->getCharData()->m_nick.get_string()));
            str_replace(inviteMsg, "$B", LEX_CAST_STR(m_rooms[room_index].m_bet));
            str_replace(inviteMsg, "$C", LEX_CAST_STR(cid));
            str_replace(inviteMsg, "$R", LEX_CAST_STR(rd->m_roomid));
            GeneralDataMgr::getInstance()->broadCastSysMsg(inviteMsg, -1);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//其他模块同步房间内信息
void PkMgr::broadInfo(int roomid)
{
    int room_index = roomid - 1;
    if (roomid > 0 && roomid < iPKMaxRoom)
    {
        if (m_rooms[room_index].m_state == 1)
        {
            m_rooms[room_index].broadInfo();
        }
    }
    return;
}

int PkMgr::queryTopList(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharPkData> rd = getPkData(cid);
    if (!rd.get() || !rd->getCharData())
    {
        return HC_ERROR;
    }
    updateRank();
    int get_rank = 0;
    json_spirit::Array list;
    std::list<boost::shared_ptr<CharPkGet> >::iterator it = m_pk_get.begin();
    while (it != m_pk_get.end())
    {
        if ((*it).get())
        {
            if ((*it)->m_rank > 3 || (*it)->m_total_get <= 0)
            {
                break;
            }
            get_rank = (*it)->m_rank;
            boost::shared_ptr<CharData> cd = GeneralDataMgr::getInstance()->GetCharData((*it)->m_cid);
            if (!cd.get())
            {
                ERR();
                cout<<"cid:"<<(*it)->m_cid<<endl;
                ++it;
                continue;
            }
            json_spirit::Object o;
            o.push_back( Pair("id", cd->m_id) );
            o.push_back( Pair("name", cd->m_name) );
            o.push_back( Pair("spic", cd->m_spic) );
            o.push_back( Pair("win_silver", (*it)->m_total_get) );
            o.push_back( Pair("rank", (*it)->m_rank) );

            json_spirit::Array get_list;
            queryRankRewards((*it)->m_rank, get_list);
            o.push_back( Pair("get", get_list) );
            list.push_back(o);
        }
        ++it;
    }
    if (get_rank < 3)
    {
        for (int i = get_rank; i < 3; ++i)
        {
            json_spirit::Object o;
            o.push_back( Pair("rank", i+1) );
            json_spirit::Array get_list;
            queryRankRewards(i+1, get_list);
            o.push_back( Pair("get", get_list) );
            list.push_back(o);
        }
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("state", Singleton<rewardsMgr>::Instance().canGetReward(rd->getCharData()->m_id, REWARDS_TYPE_PK)) );
    return HC_SUCCESS;
}

int PkMgr::queryTopCid()
{
    int topcid = 0;
    std::list<boost::shared_ptr<CharPkGet> >::iterator it = m_pk_get.begin();
    if (it != m_pk_get.end())
    {
        if ((*it).get())
        {
            if ((*it)->m_total_get > 0)
            {
                topcid = (*it)->m_cid;
            }
        }
    }
    return topcid;
}

//查询排行奖励
int PkMgr::queryRankRewards(int rank, json_spirit::Array& reward_list)
{
    if (rank >= 1 && rank <= m_last_rank)
    {
        for (std::vector<PkRankRewards>::iterator it = m_rewards.begin(); it != m_rewards.end(); ++it)
        {
            if (rank <= it->rank)
            {
                for (std::list<Item>::iterator it_i = it->reward.begin(); it_i != it->reward.end(); ++it_i)
                {
                    Item& item = *it_i;
                    json_spirit::Object obj;
                    item.toObj(obj);
                    reward_list.push_back(obj);
                }
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

//每天17点
int PkMgr::seasonAwards()
{
    //移除上次的竞技场称号
    if (m_last_top.size())
    {
        for (int i = 0; i < m_last_top.size(); ++i)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData(m_last_top[i]).get();
            if (pc)
            {
                pc->m_nick.remove_nick(nick_pk_start + i);
                pc->SaveNick();
            }
        }
        m_last_top.clear();
    }
    updateRank();
    json_spirit::Array list;
    std::list<boost::shared_ptr<CharPkGet> >::iterator it = m_pk_get.begin();
    while (it != m_pk_get.end())
    {
        if ((*it).get())
        {
            if ((*it)->m_rank > m_last_rank || (*it)->m_total_get <= 0)
            {
                break;
            }
            boost::shared_ptr<CharData> cd = GeneralDataMgr::getInstance()->GetCharData((*it)->m_cid);
            if (!cd.get())
            {
                ERR();
                cout<<"cid:"<<(*it)->m_cid<<endl;
                ++it;
                continue;
            }
            for (std::vector<PkRankRewards>::iterator it_r = m_rewards.begin(); it_r != m_rewards.end(); ++it_r)
            {
                if ((*it)->m_rank <= it_r->rank)
                {
                    //奖励暂存
                    std::list<Item> tmp_list = it_r->reward;
                    Singleton<rewardsMgr>::Instance().updateCharRewards(cd->m_id,REWARDS_TYPE_PK,(*it)->m_rank,tmp_list);
                    //给称号
                    if ((*it)->m_rank >= 1 && (*it)->m_rank <= 3)
                    {
                        cd->m_nick.add_nick(nick_pk_start + (*it)->m_rank - 1);
                        cd->SaveNick();
                        m_last_top.push_back(cd->m_id);
                    }
            		std::string msg = "";
            		switch ((*it)->m_rank)
            		{
            			case 1:
            				msg = strPkTop1Msg;
            				str_replace(msg, "$W", MakeCharNameLink(cd->m_name,cd->m_nick.get_string()));
            				str_replace(msg, "$G", LEX_CAST_STR((*it)->m_total_get));
                            str_replace(msg, "$R", itemlistToString(tmp_list));
            				break;
            			case 2:
            				msg = strPkTop2Msg;
            				str_replace(msg, "$W", MakeCharNameLink(cd->m_name,cd->m_nick.get_string()));
            				str_replace(msg, "$G", LEX_CAST_STR((*it)->m_total_get));
                            str_replace(msg, "$R", itemlistToString(tmp_list));
            				break;
            			case 3:
            				msg = strPkTop3Msg;
            				str_replace(msg, "$W", MakeCharNameLink(cd->m_name,cd->m_nick.get_string()));
            				str_replace(msg, "$G", LEX_CAST_STR((*it)->m_total_get));
                            str_replace(msg, "$R", itemlistToString(tmp_list));
            				break;
            		}
            		if (msg != "")
            		{
            			GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
            		}
                    break;
                }
            }
        }
        ++it;
    }
    it = m_pk_get.begin();
    while (it != m_pk_get.end())
    {
        if ((*it).get())
        {
            (*it)->m_rank = 0;
            (*it)->m_total_get = 0;
        }
        ++it;
    }
    boost::unordered_map<int, boost::shared_ptr<CharPkData> >::iterator it_m = m_pk_maps.begin();
    while (it_m != m_pk_maps.end())
    {
        if (!it_m->second.get())
        {
            ++it;
            continue;
        }
        it_m->second->m_total_get = 0;
        ++it_m;
    }
    InsertSaveDb("update char_pk set totalGet='0'");
    //保存上次排名
    json_spirit::Value v(m_last_top.begin(), m_last_top.end());
    GeneralDataMgr::getInstance()->setStr("last_pk_top", json_spirit::write(v));
    return HC_SUCCESS;
}

int PkMgr::combatAllResult(multi_combat* pCombat)
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (COMBAT_TYPE_PK != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    //房间处理
    if (pCombat->m_data_id > 0 && pCombat->m_data_id < iPKMaxRoom)
    {
        int i = pCombat->m_data_id - 1;
        if (m_rooms[i].m_state == 2)
        {
            m_rooms[i].m_state = 1;
            m_rooms[i].m_combat_id = 0;
            for (int j = 0; j < iPKMaxSet; ++j)
            {
                if (m_rooms[i].m_sets[j].m_state == 1 && m_rooms[i].m_sets[j].m_playes.get() && m_rooms[i].m_sets[j].m_playes->m_cid != m_rooms[i].m_own_cid)
                {
                    m_rooms[i].m_sets[j].m_ready = false;
                }
            }
        }
    }
    return HC_SUCCESS;
}

int PkMgr::combatResult(multi_combat* pCombat)
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (COMBAT_TYPE_PK != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<combatPlayer> >::iterator it = pCombat->m_players.begin();
    while (it != pCombat->m_players.end())
    {
        combatPlayer* p = it->second.get();
        if (p && p->inCombat())
        {
            //参战玩家筹码处理
            boost::shared_ptr<CharPkData> pd = getPkData(p->m_cid);
            if (pd.get() && pd->getCharData().get())
            {
                CharData* pc = pd->getCharData().get();
                pc->m_tasks.updateTask(GOAL_DAILY_PK, 0, 1);

                if (p->m_silver > p->m_org_silver)
                {
                    int win_silver = p->m_silver  - p->m_org_silver;
                    if (win_silver > 50000)
                    {
                        win_silver = win_silver * 0.98;
                        p->m_silver = p->m_org_silver + win_silver;
                    }
                    pd->m_total_get += win_silver;
                    statistics_of_silver_get(pc->m_id, pc->m_ip_address, win_silver, silver_get_pk, pc->m_union_id, pc->m_server_id);
                    //改成30w提示
                    if (win_silver > 300000)
                    {
                		std::string msg = strPkGet500KMsg;
            			str_replace(msg, "$W", MakeCharNameLink(pc->m_name,pc->m_nick.get_string()));
            			str_replace(msg, "$G", LEX_CAST_STR(win_silver));
                		if (msg != "")
                		{
                			GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                		}
                    }
                }
                else
                {
                    pd->m_total_get -= (p->m_org_silver - p->m_silver);
                }
                if (pd->m_total_get < 0)
                    pd->m_total_get = 0;
                //更新筹码排行榜
                boost::shared_ptr<CharPkGet> pk_get = getPkGet(pd->m_cid);
                if (pk_get.get())
                {
                    pk_get->m_total_get = pd->m_total_get;
                }
                else
                {
                    boost::shared_ptr<CharPkGet> tmp(new CharPkGet);
                    tmp->m_cid = pd->m_cid;
                    tmp->m_total_get = pd->m_total_get;
                    tmp->m_rank = 0;
                    m_pk_get.push_back(tmp);
                }
                pd->save();
            }
        }
        ++it;
    }
    int oldTopCid = queryTopCid();
    updateRank();
    int newTopCid = queryTopCid();
    //新冠军产生公告
    if (newTopCid != oldTopCid)
    {
        boost::shared_ptr<CharData> pc = GeneralDataMgr::getInstance()->GetCharData(newTopCid);
        boost::shared_ptr<CharPkGet> pk_get = getPkGet(newTopCid);
        if (pc.get() && pk_get.get())
        {
            std::string msg = strPkTopChangeMsg;
            str_replace(msg, "$W", MakeCharNameLink(pc->m_name,pc->m_nick.get_string()));
            str_replace(msg, "$G", LEX_CAST_STR(pk_get->m_total_get));
            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }
    }
    return HC_SUCCESS;
}

boost::shared_ptr<ChatChannel> PkMgr::GetRoomChannel(int roomid)
{
    int room_index = roomid - 1;
    if (roomid > 0 && roomid <= iPKMaxRoom)
    {
        if (m_rooms[room_index].m_state != 0)
        {
            if (m_rooms[room_index]._channel.get())
            {
                return m_rooms[room_index]._channel;
            }
        }
    }
    boost::shared_ptr<ChatChannel> tmp;
    return tmp;
}

