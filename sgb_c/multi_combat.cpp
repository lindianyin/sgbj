
#include "multi_combat.hpp"
#include "errcode_def.h"
#include "data.h"
#include "singleton.h"
#include "pk.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include <algorithm>

using namespace net;
using namespace json_spirit;

#define IS_NPC(x) ((x)<0)
#define IS_NOT_NPC(x) ((x)>0)

extern int InsertInternalActionWork(json_spirit::mObject& obj);
extern void InsertMultiCombat(MultiCombatCmd& pcombatCmd);

//发起挑战
int createMultiCombat(int cid, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (cdata == NULL)
    {
        ERR();
        return HC_ERROR;
    }
    int type = 0;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("type", type));
    std::vector<int> battle_cid_list;
    json_spirit::mArray toList;
    READ_ARRAY_FROM_MOBJ(toList,o,"list");
    cout << "list size=" << toList.size() << endl;
    if (toList.size())
    {
        for (json_spirit::mArray::iterator it = toList.begin(); it != toList.end(); ++it)
        {
            json_spirit::mValue& value = *it;
            if (value.type() != json_spirit::obj_type)
            {
                ERR();
            }
            else
            {
                json_spirit::mObject& mobj = value.get_obj();
                int toid = 0;
                READ_INT_FROM_MOBJ(toid,mobj,"id");
                cout << "get " << toid << endl;
                battle_cid_list.push_back(toid);
            }
        }
    }
    if(type == COMBAT_TYPE_PK)
    {
        int roomid = 0, bet = 0;
        READ_INT_FROM_MOBJ(roomid,o,"roomid");
        READ_INT_FROM_MOBJ(bet,o,"bet");
        boost::shared_ptr<CharPkData> rd = Singleton<PkMgr>::Instance().getPkData(cdata->m_id);
        if (!rd.get() || !rd->getCharData().get())
        {
            ERR();
            return HC_ERROR;
        }
        for (int i = 0; i < battle_cid_list.size(); ++i)
        {
            int now_cid = battle_cid_list[i];
            if (now_cid > 0)
            {
                //对方是否在战斗中
                multi_combat* pCombat = Singleton<MultiCombatMgr>::Instance().getCharMultiCombat(now_cid);
                if (pCombat)
                {
                    ERR();
                    return HC_ERROR_TARGET_IN_COMBAT;
                }
                boost::shared_ptr<CharPkData> td = Singleton<PkMgr>::Instance().getPkData(now_cid);
                if (!td.get() || !td->getCharData().get())
                {
                    ERR();
                    return HC_ERROR;
                }
                CharData* tc = td->getCharData().get();
                if (cdata->silver() < bet || tc->silver() < bet)
                {
                    ERR();
                    return HC_ERROR;
                }
            }
        }
        multi_combat* pb = Singleton<MultiCombatMgr>::Instance().createPkCombat(battle_cid_list, bet);
        if (NULL == pb)
        {
            ERR();
            return HC_ERROR;
        }
        pb->m_data_id = roomid;
        PkRoom* room = Singleton<PkMgr>::Instance().getRoom(roomid);
        if (room && room->m_state == 2)
        {
            room->m_combat_id = pb->m_combat_id;
        }
        //通知参战者
        for (int i = 0; i < battle_cid_list.size(); ++i)
        {
            int now_cid = battle_cid_list[i];
            if (now_cid > 0)
            {
                CharData* tc = GeneralDataMgr::getInstance()->GetCharData(now_cid).get();
                boost::shared_ptr<OnlineCharactor> toc = GeneralDataMgr::getInstance()->GetOnlineCharactor(tc->m_name);
                if (toc.get())
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "multi_challenged") );
                    obj.push_back( Pair("type", type));
                    obj.push_back( Pair("s", 200) );
                    pb->getCombatInfo(tc->m_id, obj);
                    int wait_time = iCombatWaitSec;
                    obj.push_back( Pair("secs", wait_time) );
                    obj.push_back( Pair("bet", pb->m_base_bet) );
                    toc->Send(write(obj));
                }
            }
        }
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS_NO_RET;
}

//发起挑战
int ProcessMultiChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    multi_combat* myCombat = Singleton<MultiCombatMgr>::Instance().getCharMultiCombat(cdata->m_id);
    if (myCombat)
    {
        ERR();
        cout << "cid=" << cdata->m_id << " is in combat!!! combat_type=" << myCombat->m_type << endl;
        return HC_ERROR;
    }
    //插入战斗处理
    MultiCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_create;
    cmd._pCombat = NULL;
    InsertMultiCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//牌局行动
int ProcessMultiGameAct(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    multi_combat* myCombat = Singleton<MultiCombatMgr>::Instance().getCharMultiCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    MultiCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_player_act;
    cmd._pCombat = myCombat;
    InsertMultiCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//退出挑战
int ProcessMultiQuitChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    multi_combat* myCombat = Singleton<MultiCombatMgr>::Instance().getCharMultiCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    MultiCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_player_quit;
    cmd._pCombat = myCombat;
    InsertMultiCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//客户端准备好战斗信号
int ProcessMultiCombatSign(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    multi_combat* myCombat = Singleton<MultiCombatMgr>::Instance().getCharMultiCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    MultiCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_sign;
    cmd._pCombat = myCombat;
    InsertMultiCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//继续参加战斗
int ProcessMultiContinueChallenge(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //我是否在战斗中了
    multi_combat* myCombat = Singleton<MultiCombatMgr>::Instance().getCharMultiCombat(cdata->m_id);
    if (NULL == myCombat)
    {
        ERR();
        cout<<"not combat, cid "<<cdata->m_id<<endl;
        return HC_ERROR;
    }
    MultiCombatCmd cmd;
    cmd.mobj = o;
    cmd.cname = cdata->m_name;
    cmd.cid = cdata->m_id;
    cmd.cmd = combat_cmd_continue;
    cmd._pCombat = myCombat;
    InsertMultiCombat(cmd);
    return HC_SUCCESS_NO_RET;
}

//请求战斗信息
int ProcessMultiQueryCombatInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int combat_id = 0;
    READ_INT_FROM_MOBJ(combat_id,o,"id");
    //账号登录状态
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    multi_combat* pb = Singleton<MultiCombatMgr>::Instance().findMultiCombat(combat_id);
    if (pb && pb->m_state != COMBAT_STATE_END)
    {
        MultiCombatCmd cmd;
        cmd.mobj = o;
        cmd.cname = cdata->m_name;
        cmd.cid = cdata->m_id;
        cmd.cmd = combat_cmd_query;
        cmd._pCombat = pb;
        InsertMultiCombat(cmd);
        return HC_SUCCESS_NO_RET;
    }
    return HC_SUCCESS;
}

//牌局开始
int ProcessMultiStartGame(json_spirit::mObject& o)
{
    int combat_id = 0;
    READ_INT_FROM_MOBJ(combat_id,o,"id");
    multi_combat* pb = Singleton<MultiCombatMgr>::Instance().findMultiCombat(combat_id);
    if (pb && pb->m_state != COMBAT_STATE_END)
    {
        MultiCombatCmd cmd;
        cmd.mobj = o;
        cmd.cmd = combat_cmd_start;
        cmd._pCombat = pb;
        InsertMultiCombat(cmd);
        return HC_SUCCESS_NO_RET;
    }
    return HC_SUCCESS;
}

//牌局结束
int ProcessMultiGameEnd(json_spirit::mObject& o)
{
    int combat_id = 0;
    READ_INT_FROM_MOBJ(combat_id,o,"id");
    multi_combat* pb = Singleton<MultiCombatMgr>::Instance().findMultiCombat(combat_id);
    if (pb)
    {
        MultiCombatCmd cmd;
        cmd.mobj = o;
        cmd.cmd = combat_cmd_end;
        cmd._pCombat = pb;
        InsertMultiCombat(cmd);
        return HC_SUCCESS_NO_RET;
    }
    return HC_SUCCESS;
}

//操作超时
int ProcessMultiGameTimeout(json_spirit::mObject& o)
{
    int combat_id = 0;
    READ_INT_FROM_MOBJ(combat_id,o,"id");
    multi_combat* pb = Singleton<MultiCombatMgr>::Instance().findMultiCombat(combat_id);
    if (pb)
    {
        MultiCombatCmd cmd;
        cmd.mobj = o;
        cmd.cmd = combat_cmd_timeout;
        cmd._pCombat = pb;
        InsertMultiCombat(cmd);
        return HC_SUCCESS_NO_RET;
    }
    return HC_SUCCESS;
}

//牌局npc动作
int ProcessMultiNPCAct(json_spirit::mObject& o)
{
    int combat_id = 0, cmd = 0, seq = 0, cid = 0;
    READ_INT_FROM_MOBJ(combat_id,o,"id");
    READ_INT_FROM_MOBJ(cid,o,"cid");
    READ_INT_FROM_MOBJ(cmd,o,"act");
    READ_INT_FROM_MOBJ(seq,o,"seq");
    multi_combat* pb = Singleton<MultiCombatMgr>::Instance().findMultiCombat(combat_id);
    if (pb && pb->m_state != COMBAT_STATE_END)
    {
        MultiCombatCmd cmd;
        cmd.mobj = o;
        cmd.cname = "";
        cmd.cid = cid;
        cmd.cmd = combat_cmd_player_act;
        cmd._pCombat = pb;
        InsertMultiCombat(cmd);
        return HC_SUCCESS_NO_RET;
    }
    return HC_SUCCESS;
}

int combatPlayer::LoadCharactor(CharData& cdata, int hid)
{
    if (hid == 0)
        hid = cdata.m_heros.m_default_hero;
    boost::shared_ptr<CharHeroData> p_hero = cdata.m_heros.GetHero(hid);
    if (!p_hero.get() || !p_hero->m_baseHero.get())
    {
        ERR();
        return -1;
    }
    if (!p_hero->m_init_attr)
    {
        p_hero->updateAttribute();
    }
    m_cid = cdata.m_id;
    m_hid = p_hero->m_id;
    m_player_name = cdata.m_name;
    m_player_level = cdata.m_level;
    m_chat = cdata.m_chat;
    m_hp_org = p_hero->m_hp;
    m_hp_left = p_hero->m_hp;
    m_hp_max = p_hero->m_hp;
    m_name = p_hero->m_baseHero->m_name;
    m_level = p_hero->m_level;
    m_star = p_hero->m_star;
    m_race = p_hero->m_race;
    m_attack = p_hero->m_attack;
    m_defense = p_hero->m_defense;
    m_magic = p_hero->m_magic;
    m_spic = p_hero->m_spic;
    m_virtual_combat = 0;
    m_virtual_silver = p_hero->m_attribute * 150;
    m_silver = cdata.silver();
    m_org_silver = cdata.silver();
    m_combat_attribute = cdata.m_combat_attribute;
    //技能加成已经实时结算不需要在战斗再次加载
    //m_attack += m_combat_attribute.skill_attack_add();
    //m_defense+= m_combat_attribute.skill_defense_add();
    //m_magic += m_combat_attribute.skill_magic_add();
    m_card_rank.clear();

    m_equipt.clear();
    json_spirit::mObject o;
    int cur_nums = 0;
    p_hero->m_bag.showBagEquipments(m_equipt, o, cur_nums);
    return 0;
}

void combatPlayer::getInfo(json_spirit::Object& robj)
{
    robj.push_back( Pair("id", m_cid) );
    robj.push_back( Pair("name", IS_NPC(m_cid) ? m_name : m_player_name) );
    robj.push_back( Pair("chat", m_chat) );
    if (IS_NOT_NPC(m_cid))
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
        if (cdata.get())
        {
            robj.push_back( Pair("nick", cdata->m_nick.get_string()) );
        }
    }
    robj.push_back( Pair("hp", m_hp_left) );
    robj.push_back( Pair("max_hp", m_hp_max) );
    robj.push_back( Pair("race", m_race) );
    robj.push_back( Pair("level", m_level) );
    robj.push_back( Pair("star", m_star) );
    robj.push_back( Pair("card", 100*m_race+getLevelRank(m_level)) );
    robj.push_back( Pair("attack", m_attack) );
    robj.push_back( Pair("defense", m_defense) );
    robj.push_back( Pair("magic", m_magic) );
    robj.push_back( Pair("org_silver", m_org_silver) );
    if (m_virtual_combat)
    {
        robj.push_back( Pair("silver", m_org_silver) );
    }
    else
    {
        robj.push_back( Pair("silver", m_silver) );
    }
    robj.push_back( Pair("spic", m_spic) );
    robj.push_back( Pair("equipt", m_equipt) );
}

void combatPlayer::getExtraInfo(json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        ERR();
        return;
    }
    //声望信息
    json_spirit::Array prestigelist;
    for (int type = 1; type <= 4; ++type)
    {
        json_spirit::Object prestigeobj;
        prestigeobj.push_back( Pair("type", type) );
        prestigeobj.push_back( Pair("prestige_level", cdata->m_prestige_level[type-1]) );
        prestigeobj.push_back( Pair("prestige", cdata->prestige(type)) );
        prestigeobj.push_back( Pair("max_prestige", cdata->prestigeMax(type)) );
        prestigelist.push_back(prestigeobj);
    }
    robj.push_back( Pair("prestige", prestigelist) );
    boost::shared_ptr<CharHeroData> p_hero = cdata->m_heros.GetHero(cdata->m_heros.m_default_hero);
    if (p_hero.get() && p_hero->m_baseHero.get())
    {
        json_spirit::Object org_exp;
        org_exp.push_back( Pair("exp", p_hero->m_exp) );
        org_exp.push_back( Pair("level", p_hero->m_level) );
        if (p_hero->m_level < iMaxLevel)
            org_exp.push_back( Pair("need_exp", GeneralDataMgr::getInstance()->GetBaseExp(p_hero->m_level + 1)));
        robj.push_back( Pair("exp", org_exp) );
    }
}

void combatPlayer::set_my_first()
{
    baseCard c;
    c.suit = m_race;
    c.value = getLevelRank(m_level);
    deal_public_1(c);
}

void combatPlayer::set_his_first(combatPlayer* p)
{
    baseCard c;
    c.suit = p->m_cards.cards[0].suit;
    c.value = p->m_cards.cards[0].value;
    deal_public_3(c);
}

multi_cardgame::multi_cardgame(multi_combat* c)
{
    m_combat_id = c->m_combat_id;
    p_combat = c;
    std::vector<int>::iterator it = c->m_pos_list.begin();
    while (it != c->m_pos_list.end())
    {
        m_pos_list.push_back(*it);
        ++it;
    }
    m_player_cnt = 0;
    m_base_bet = c->m_base_bet;
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        cout << "pos " << i+1 << " cid " << now_cid << endl;
        if (now_cid > 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p && (p->m_silver > (m_base_bet*4)) && p->m_hp_left > 0)
            {
                cout << "join combat" << endl;
                ++m_player_cnt;
                p->m_org_silver = p->m_silver;
                p->m_combat_state = 0;
                p->m_deal_state = 0;
                p->m_bet_silver = 0;
                p->m_round_bet = 0;
                p->m_cards.reset();
                //设置英雄牌
                baseCard c;
                c.suit = p->m_race;
                c.value = getLevelRank(p->m_level);
                p->deal_private_1(c);
                //剔除英雄牌
                m_poker.remove(p->m_cards.cards[2].suit, p->m_cards.cards[2].value);
            }
            else if(p)
            {
                cout << "kick " << p->m_cid << endl;
                int out_cid = p->m_cid;
                //出局玩家踢出战斗管理
                p_combat->removePlayer(p);
                m_pos_list[i] = 0;
                //出局玩家离开pk房间
                if (p_combat && p_combat->m_type == COMBAT_TYPE_PK)
                {
                    Singleton<PkMgr>::Instance().leaveRoom(out_cid);
                }
            }
        }
    }
    reset();
}

int multi_cardgame::onStart()
{
    if (m_player_cnt <= 1)
    {
        //战斗无法正常
        lastAct.type = COMBAT_CMD_FOLD;
        return onEnd();
    }
    if (COMBAT_STATE_INIT == m_state)
    {
        m_dealer = 0;
        m_dealer = dealer();
        betBase();

        m_next = m_dealer;
        m_state = COMBAT_STATE_INGOING;
        m_seq = 1;
        m_round = 0;
        lastAct.more = 0;
        lastAct.cid = 0;
        lastAct.left = 0;
        lastAct.type = COMBAT_CMD_START;
        lastAct.call = 0;
        m_total_bet = m_base_bet*m_player_cnt;

        json_spirit::Object o;
        o.push_back( Pair("cmd", "multi_gameStart") );
        o.push_back( Pair("s", 200) );
        o.push_back( Pair("base", m_base_bet) );
        o.push_back( Pair("next", 0) );
        o.push_back( Pair("seq", 1) );
        broadCastMsg(write(o));
        if (p_combat)
        {
            p_combat->AppendResult(o);
        }
        dealCard();
    }
    return HC_SUCCESS;
}

combatPlayer* multi_cardgame::getPlayer(int cid)
{
    if (p_combat)
    {
        return p_combat->getPlayer(cid);
    }
    return NULL;
}

void multi_cardgame::broadCastMsg(const std::string& msg)
{
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p)
            {
                boost::shared_ptr<OnlineCharactor> c = GeneralDataMgr::getInstance()->GetOnlineCharactor(p->m_player_name);
                if (c.get())
                {
                    c->Send(msg);
                }
            }
        }
    }
}

void multi_cardgame::broadCastAct(int cid, int type, int call, int more, int total, int left, int times, int next, int seq)
{
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "multi_playAct") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("id", cid) );
    obj.push_back( Pair("type", type) );
    obj.push_back( Pair("call", call) );
    obj.push_back( Pair("bet", more) );
    obj.push_back( Pair("total", total) );
    obj.push_back( Pair("left", left) );
    obj.push_back( Pair("round", m_round+1) );
    obj.push_back( Pair("t", times) );
    obj.push_back( Pair("next", next) );
    obj.push_back( Pair("seq", seq) );
    std::string msg = json_spirit::write(obj);
    broadCastMsg(msg);
    if (p_combat)
    {
        p_combat->AppendResult(obj);
    }
}

void multi_cardgame::broadCastAct()
{
    broadCastAct(lastAct.cid, lastAct.type, lastAct.call, lastAct.more, m_total_bet, lastAct.left,
        m_rounds[m_round].m_actList.size(), m_next, m_seq);
}

void multi_cardgame::broadCastCard(bool pri, std::vector<std::pair<int, int> >& card_list, int next, int seq)
{
    //本轮发牌信息
    json_spirit::Array list;
    std::vector<std::pair<int, int> >::iterator it = card_list.begin();
    while (it != card_list.end())
    {
        json_spirit::Object c;
        c.push_back( Pair("id", (*it).first) );
        c.push_back( Pair("card", (*it).second) );
        list.push_back(c);
        ++it;
    }
    //生成发牌信息通知双方
    json_spirit::Object o;
    o.push_back( Pair("cmd", "multi_deal") );
    o.push_back( Pair("s", 200) );
    o.push_back( Pair("private", pri) );
    o.push_back( Pair("card_list", list) );
    o.push_back( Pair("round", m_round+1) );
    o.push_back( Pair("next", next) );
    o.push_back( Pair("seq", seq) );

    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p)
            {
                boost::shared_ptr<OnlineCharactor> c = GeneralDataMgr::getInstance()->GetOnlineCharactor(p->m_player_name);
                if (c.get())
                {
                    json_spirit::Object tmp;
                    tmp = o;
                    if (p->inCombat())
                    {
                        p->m_cards.evaluatorSimple();
                        tmp.push_back( Pair("type", p->m_cards.rank) );
                        p->m_cards.resetEva();
                    }
                    c->Send(write(tmp));
                }
            }
        }
    }
    if (p_combat)
    {
        p_combat->AppendResult(o);
    }
}

void multi_cardgame::doAttack()
{
    cout << "multi_cardgame doAttack" << endl;
    if (m_attacker == NULL)
    {
        cout << "no attacker" << endl;
        return;
    }
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        int now_damage = 0;
        cout << "check " << now_cid << endl;
        if (now_cid != 0 && now_cid != m_attacker->m_cid)
        {
            if (find(m_winner_list.begin(),m_winner_list.end(),now_cid) != m_winner_list.end())
            {
                //该玩家也是胜利者之一
                m_damage_list.push_back(0);
                cout << "is winner" << endl;
                continue;
            }
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->inCombat())
            {
                //（自身攻击*5+自身魔力*5）-（敌方防御/2+敌方魔力/2）
                now_damage = (m_attacker->m_attack + m_attacker->m_magic) * 5 - (p->m_attack + p->m_magic) / 2;
                //技能影响
                int add_per = 0;
                int resist_per = 0;
                if (m_result == GAME_OVER)
                {
                    add_per += m_attacker->m_combat_attribute.skill_rank_add(m_attacker->m_cards.rank);
                    resist_per += p->m_combat_attribute.skill_rank_resist(p->m_cards.rank);
                }
                add_per += m_attacker->m_combat_attribute.damage_per();
                resist_per += p->m_combat_attribute.damage_resist_per();
                now_damage = now_damage * (100 + add_per - resist_per) / 100;
                if (now_damage < 1)
                    now_damage = 1;
                p->m_hp_left -= now_damage;
                if (p->m_hp_left < 0)
                {
                    p->m_hp_left = 0;
                }
            }
        }
        m_damage_list.push_back(now_damage);
    }
    return;
}

void multi_cardgame::compare_result()
{
    cout << "multi_cardgame::compare_result" << endl;
    int tmp_rank = TOTAL_HAND_RANK;
    int tmp_score = 0;
    m_winner_list.clear();
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        cout << "check " << now_cid <<endl;
        if (now_cid != 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->inCombat() && p->m_combat_state != 2)
            {
                if (p->m_cards.rank < tmp_rank)
                {
                    m_winner_list.clear();
                    m_winner_list.push_back(p->m_cid);
                    tmp_rank = p->m_cards.rank;
                    tmp_score = p->m_cards.score;
                    m_attacker = p;
                    cout << "new winner=" << p->m_cid << endl;
                }
                else if (p->m_cards.rank == tmp_rank)
                {
                    if (p->m_cards.score > tmp_score)
                    {
                        m_winner_list.clear();
                        m_winner_list.push_back(p->m_cid);
                        tmp_rank = p->m_cards.rank;
                        tmp_score = p->m_cards.score;
                        m_attacker = p;
                        cout << "new winner=" << p->m_cid << endl;
                    }
                    else if (p->m_cards.score == tmp_score)
                    {
                        m_winner_list.push_back(p->m_cid);
                        cout << "add winner=" << p->m_cid << endl;

                        if (m_attacker == NULL || (m_attacker->m_attack + m_attacker->m_magic) < (p->m_attack + p->m_magic))
                        {
                            m_attacker = p;
                            cout << "change attack=" << p->m_cid << endl;
                        }
                    }
                }
            }
        }
    }
}

int multi_cardgame::onEnd()
{
    m_state = COMBAT_STATE_END;
    //其他人都放弃了
    if (lastAct.type == COMBAT_CMD_FOLD)
    {
        for (int i = 0; i < m_pos_list.size(); ++i)
        {
            int now_cid = m_pos_list[i];
            if (now_cid != 0)
            {
                combatPlayer* p = getPlayer(now_cid);
                if (p)
                {
                    if (p->inCombat() && p->m_combat_state != 2)
                    {
                        m_attacker = p;
                        m_winner_list.push_back(now_cid);
                    }
                }
            }
        }
        m_result = GAME_FOLD;
    }
    else
    {
        m_result = GAME_OVER;
        //比较牌大小了
        for (int i = 0; i < m_pos_list.size(); ++i)
        {
            int now_cid = m_pos_list[i];
            if (now_cid != 0)
            {
                combatPlayer* p = getPlayer(now_cid);
                if (p && p->inCombat() && p->m_combat_state != 2)
                {
                    debug_print_input(p->m_cards);
                    p->m_cards.evaluator();
                    debug_print_result(p->m_cards);
                }
            }
        }
        compare_result();
        if (m_winner_list.size() > 0)
        {
            ;
        }
        else
        {
            m_result = GAME_PINGJU;
        }
    }
    switch (m_result)
    {
        //放弃
        case GAME_FOLD:
        //正常结束
        case GAME_OVER:
            //分钱
            getSilver();
            //攻击
            doAttack();
            break;
        //平均
        case GAME_PINGJU:
        default:
            //回本
            backSilver();
            break;
    }
    //通知本局结果
    json_spirit::Object result;
    result.push_back( Pair("cmd", "multi_PlayResult") );
    result.push_back( Pair("s", 200) );
    result.push_back( Pair("result", m_result) );
    json_spirit::Array win_list;
    if (m_winner_list.size() > 0)
    {
        for (int i = 0; i < m_winner_list.size(); ++i)
        {
            int now_cid = m_winner_list[i];
            if (now_cid != 0)
            {
                json_spirit::Object w;
                w.push_back( Pair("id", now_cid) );
                win_list.push_back(w);
            }
        }
    }
    result.push_back( Pair("win_list", win_list) );
    if (m_attacker)
    {
        result.push_back( Pair("attack_cid", m_attacker->m_cid) );
    }
    json_spirit::Array list;
    for (int i = 0; i < m_pos_list.size() && i < m_damage_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid != 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p)
            {
                json_spirit::Object c;
                c.push_back( Pair("id", p->m_cid) );
                c.push_back( Pair("silver", p->m_silver) );
                c.push_back( Pair("hp", p->m_hp_left) );

                if (m_result != GAME_FOLD)
                {
                    c.push_back( Pair("type", p->m_cards.rank) );
                    json_spirit::Array cards;
                    for (int i = 0; i < 7; ++i)
                    {
                        cards.push_back(p->m_cards.cards[i].suit*100 + p->m_cards.cards[i].value);
                    }
                    c.push_back( Pair("cards", cards) );
                    json_spirit::Array hand;
                    if (p->inCombat() && p->m_combat_state != 2)
                    {
                        for (int i = 0; i < 5; ++i)
                        {
                            json_spirit::Object c;
                            c.push_back( Pair("p", p->m_cards.final_hand[i]->pos) );
                            c.push_back( Pair("c", p->m_cards.final_hand[i]->trans_suit*100+p->m_cards.final_hand[i]->trans_value) );
                            hand.push_back(c);
                        }
                    }
                    c.push_back( Pair("hand", hand) );
                }
                c.push_back( Pair("damage", m_damage_list[i]) );
                c.push_back( Pair("winBet", p->m_silver - p->m_org_silver) );
                cout << "cid " << p->m_cid << ", org_silver = " << p->m_org_silver << ", now_silver = " << p->m_silver << endl;
                list.push_back(c);
            }
        }
    }
    result.push_back( Pair("list", list) );
    std::string msg = write(result);
    broadCastMsg(msg);
    //牌局结束，战斗是否结束?
    if (p_combat)
    {
        p_combat->AppendResult(result);
        p_combat->checkEnd();
        //战斗结果在每局结尾处理
        switch (p_combat->m_type)
        {
            case COMBAT_TYPE_PK:
                Singleton<PkMgr>::Instance().combatResult(p_combat);
                break;
        }
    }
    return HC_SUCCESS_NO_RET;
}

int multi_cardgame::onCheck(int cid)
{
    //本轮第一个下注的才能check
    if (m_rounds[m_round].m_actList.size() > 0)
    {
        return HC_ERROR_COMBAT_ACT;
    }
    combatPlayer* p = getPlayer(cid);
    if (p)
    {
        //check不押注
        lastAct.cid = cid;
        lastAct.call = 0;
        lastAct.more = 0;
        lastAct.left = p->m_silver;
        lastAct.type = COMBAT_CMD_CHECK;
        m_rounds[m_round].m_actList.push_back(lastAct);
        //下一个操作玩家
        m_next = nextCid(cid);
        //后续跟拍设置
        m_call_bet = 0;
        resetCall();
        p->m_combat_state = 1;

        ++m_seq;
        broadCastAct();
        cout << p->m_cid << " check combat call=0,more=0 game_call=0,game_bet=" << m_total_bet << endl;
        cout << p->m_cid << " char_bet=" << p->m_bet_silver << endl;

        if (IS_NPC(m_next))
        {
            //npc自动
            json_spirit::mObject obj;
            obj["cmd"] = "multi_npcAct";
            obj["id"] = m_combat_id;
            obj["cid"] = m_next;
            obj["type"] = NPCActRandom(m_next);
            obj["seq"] = m_seq;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(2, 1, obj,1));
            splsTimerMgr::getInstance()->addTimer(tmsg);
        }
    }
    return HC_SUCCESS_NO_RET;
}

int multi_cardgame::onFold(int cid)
{
    combatPlayer* p = getPlayer(cid);
    if (p)
    {
        //fold不押注
        lastAct.cid = cid;
        lastAct.call = 0;
        lastAct.more = 0;
        lastAct.left = p->m_silver;
        lastAct.type = COMBAT_CMD_FOLD;
        m_rounds[m_round].m_actList.push_back(lastAct);
        m_next = 0;
        if (p->inCombat())
        {
            p->m_combat_state = 2;
        }
        ++m_fold_cnt;
        if (m_fold_cnt >= m_player_cnt - 1)//只剩一个没放弃则结束战斗
        {
            m_state = COMBAT_STATE_END;
            ++m_seq;
            broadCastAct();
            return onEnd();
        }

        m_next = nextCid(cid);
        //下一个玩家如果已经操作过则发牌
        combatPlayer* p_next = getPlayer(m_next);
        if (p_next && p_next->m_combat_state > 0)
        {
            m_next = 0;
        }
        if (m_next == 0)
        {
            ++m_round;
            return dealCard();
        }
        ++m_seq;
        broadCastAct();

        if (IS_NPC(m_next))
        {
            //npc自动
            json_spirit::mObject obj;
            obj["cmd"] = "multi_npcAct";
            obj["id"] = m_combat_id;
            obj["cid"] = m_next;
            obj["type"] = NPCActRandom(m_next);
            obj["seq"] = m_seq;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(2, 1, obj,1));
            splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        return HC_SUCCESS_NO_RET;
    }
    else
    {
        return HC_ERROR;
    }
}

int multi_cardgame::onDouble(int cid)
{
    //最多下注3个来回
    if (m_rounds[m_round].m_actList.size() >= 5)
    {
        ERR();
        return HC_ERROR_COMBAT_ACT;
    }
    combatPlayer* p = getPlayer(cid);
    if (p)
    {
        int call = (m_call_bet-p->m_round_bet);
        int more = (m_total_bet+call)*2;
        //double先跟一个call,再追加上次桌面筹码翻倍
        if (p->m_silver > call)
        {
            lastAct.call = call;
            p->m_silver -= call;

            if (p->m_silver > more)
            {
                lastAct.more = more;
                p->m_silver -= more;
            }
            else
            {
                more = p->m_silver;
                lastAct.more = more;
                p->m_silver = 0;
            }
        }
        else
        {
            call = p->m_silver;
            lastAct.call = call;
            lastAct.more = 0;
            p->m_silver = 0;
        }
        m_total_bet += (call+more);
        p->m_bet_silver += (call+more);
        p->m_round_bet += (call+more);

        lastAct.cid = cid;
        lastAct.left = p->m_silver;
        lastAct.type = COMBAT_CMD_DOUBLEBET;
        m_rounds[m_round].m_actList.push_back(lastAct);

        //加注成功后续跟拍重新设置
        if (more > 0)
        {
            m_call_bet += more;
            resetCall();
            p->m_combat_state = 1;
        }

        m_next = nextCid(cid);
        combatPlayer* p_next = getPlayer(m_next);
        if (p_next && p_next->m_combat_state > 0)
        {
            m_next = 0;
        }

        ++m_seq;
        broadCastAct();
        cout << p->m_cid << " double combat call="<<call<<",more="<<more<<" game_call="<<m_call_bet<<",game_bet=" << m_total_bet << endl;
        cout << p->m_cid << " char_bet=" << p->m_bet_silver << endl;

        if (m_next == 0)
        {
            ++m_round;
            return dealCard();
        }
        else if (IS_NPC(m_next))
        {
            //npc自动
            json_spirit::mObject obj;
            obj["cmd"] = "multi_npcAct";
            obj["id"] = m_combat_id;
            obj["cid"] = m_next;
            obj["type"] = NPCActRandom(m_next);
            obj["seq"] = m_seq;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(2, 1, obj,1));
            splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        return HC_SUCCESS_NO_RET;
    }
    else
    {
        ERR();
        return HC_ERROR;
    }
}
int multi_cardgame::onHalf(int cid)
{
    //最多下注3个来回
    if (m_rounds[m_round].m_actList.size() >= 5)
    {
        return HC_ERROR_COMBAT_ACT;
    }
    combatPlayer* p = getPlayer(cid);
    if (p)
    {
        int call = (m_call_bet-p->m_round_bet);
        int more = (m_total_bet+call)/2;
        //double先跟一个call,再追加上次桌面筹码除半
        if (p->m_silver > call)
        {
            lastAct.call = call;
            p->m_silver -= call;

            if (p->m_silver > more)
            {
                lastAct.more = more;
                p->m_silver -= more;
            }
            else
            {
                more = p->m_silver;
                lastAct.more = more;
                p->m_silver = 0;
            }
        }
        else
        {
            call = p->m_silver;
            lastAct.call = call;
            lastAct.more = 0;
            p->m_silver = 0;
        }
        m_total_bet += (call+more);
        p->m_bet_silver += (call+more);
        p->m_round_bet += (call+more);

        lastAct.cid = cid;
        lastAct.left = p->m_silver;
        lastAct.type = COMBAT_CMD_HALFBET;
        m_rounds[m_round].m_actList.push_back(lastAct);

        //加注成功后续跟拍重新设置
        if (more > 0)
        {
            m_call_bet += more;
            resetCall();
            p->m_combat_state = 1;
        }

        m_next = nextCid(cid);
        combatPlayer* p_next = getPlayer(m_next);
        if (p_next && p_next->m_combat_state > 0)
        {
            m_next = 0;
        }

        ++m_seq;
        broadCastAct();
        cout << p->m_cid << " half combat call="<<call<<",more="<<more<<" game_call="<<m_call_bet<<",game_bet=" << m_total_bet << endl;
        cout << p->m_cid << " char_bet=" << p->m_bet_silver << endl;

        if (m_next == 0)
        {
            ++m_round;
            return dealCard();
        }
        else if (IS_NPC(m_next))
        {
            //npc自动
            json_spirit::mObject obj;
            obj["cmd"] = "multi_npcAct";
            obj["id"] = m_combat_id;
            obj["cid"] = m_next;
            obj["type"] = NPCActRandom(m_next);
            obj["seq"] = m_seq;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(2, 1, obj,1));
            splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        return HC_SUCCESS_NO_RET;
    }
    else
    {
        return HC_ERROR;
    }
}

int multi_cardgame::onCall(int cid)
{
    if (m_rounds[m_round].m_actList.size() == 0)
    {
        ERR();
        return HC_ERROR_COMBAT_ACT;
    }
    combatPlayer* p = getPlayer(cid);
    if (p)
    {
        int call = (m_call_bet-p->m_round_bet);
        if (p->m_silver > call)
        {
            lastAct.call = call;
            p->m_silver -= call;
        }
        else
        {
            call = p->m_silver;
            lastAct.call = call;
            p->m_silver = 0;
        }
        lastAct.more = 0;
        m_total_bet += call;
        p->m_bet_silver += call;
        p->m_round_bet += call;

        lastAct.cid = cid;
        lastAct.left = p->m_silver;
        lastAct.type = COMBAT_CMD_CALL;
        m_rounds[m_round].m_actList.push_back(lastAct);

        p->m_combat_state = 1;
        m_next = nextCid(cid);
        combatPlayer* p_next = getPlayer(m_next);
        if (p_next && p_next->m_combat_state > 0)
        {
            m_next = 0;
        }

        ++m_seq;
        broadCastAct();
        cout << p->m_cid << " call combat call="<<call<<",more=0 game_call="<<m_call_bet<<",game_bet=" << m_total_bet << endl;
        cout << p->m_cid << " char_bet=" << p->m_bet_silver << endl;

        if (m_next == 0)
        {
            ++m_round;
            return dealCard();
        }
        else if (IS_NPC(m_next))
        {
            //npc自动
            json_spirit::mObject obj;
            obj["cmd"] = "multi_npcAct";
            obj["id"] = m_combat_id;
            obj["cid"] = m_next;
            obj["type"] = NPCActRandom(m_next);
            obj["seq"] = m_seq;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(2, 1, obj,1));
            splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        return HC_SUCCESS_NO_RET;
    }
    else
    {
        return HC_ERROR;
    }
}

int multi_cardgame::NPCActRandom(int cid)
{
    int act_default = 0;
    if (m_rounds[m_round].m_actList.size() == 0)
    {
        act_default = COMBAT_CMD_CHECK;
    }
    else
    {
        act_default = COMBAT_CMD_CALL;
    }
    //最多下注3个来回
    if (m_rounds[m_round].m_actList.size() >= 5)
    {
        return act_default;
    }
    //ai在上次加注后只会默认操作
    if (m_rounds[m_round].m_actList.size() && (lastAct.type == COMBAT_CMD_DOUBLEBET || lastAct.type == COMBAT_CMD_HALFBET))
    {
        return act_default;
    }
    combatPlayer* p = getPlayer(cid);
    if (p)
    {
        p->m_cards.evaluatorSimple();
        //cout << "NPCAct rank=" << player1->m_cards.rank << endl;
        int m_gailvs[3];
        int act[3] = {COMBAT_CMD_DOUBLEBET,COMBAT_CMD_HALFBET,act_default};
        for (int i = 0; i < 3; ++i)
        {
            //最后一次押注，降低默认一半概率
            if (m_round == 4 && i == 2)
            {
                m_gailvs[i] = iAIAct[p->m_cards.rank-1][i] / 2;
            }
            else
            {
                m_gailvs[i] = iAIAct[p->m_cards.rank-1][i];
            }
            //cout << "round " << m_round << ", i=" << i << endl;
            //cout << "act " << act[i] << " gailv=" << m_gailvs[i] << endl;
        }
        p->m_cards.resetEva();
        boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
        boost::random::discrete_distribution<> dist(m_gailvs);
        return act[dist(gen)];
    }
    return act_default;
}

int multi_cardgame::dealCard()
{
    cout << "dealCard round=" << m_round << endl;
    switch (m_round)
    {
        case 0:
            return deal1();

        case 1:
            return deal2();

        case 2:
            return deal3();

        case 3:
            return deal4();

        default:
            return onEnd();
    }
}

baseCard* multi_cardgame::deal()
{
    baseCard* c = m_poker.deal();
    return c;
}

int multi_cardgame::dealer()
{
    //初始化
    if (m_dealer == 0)
    {
        m_dealer = startCid();
        return m_dealer;
    }
    //根据明牌组合决定
    switch (m_round)
    {
        case 0:
            return m_dealer;
        case 1:
        case 2:
        {
            //比三张
            int max_rank = TOTAL_HAND_RANK + 1, max_score = 0, max_cid = 0;
            for (int i = 0; i < m_pos_list.size(); ++i)
            {
                int now_cid = m_pos_list[i];
                if (now_cid > 0)
                {
                    combatPlayer* p = getPlayer(now_cid);
                    if (p && p->m_combat_state > 0)
                    {
                        if (p->m_cards.cards[0].value == 0 || p->m_cards.cards[1].value == 0)
                        {
                            return m_dealer;
                        }
                        sevenCards card;
                        card.reset();
                        card.cards[0].set(p->m_cards.cards[0].trans_suit, p->m_cards.cards[0].trans_value);
                        card.cards[1].set(p->m_cards.cards[1].trans_suit, p->m_cards.cards[1].trans_value);
                        card.cards[2].set(p->m_cards.cards[2].trans_suit, p->m_cards.cards[2].trans_value);
                        int score = 0;
                        card.evaluatorSimple();
                        if (card.rank == THREE_OF_A_KIND)
                        {
                            score = card.cards[2].trans_value;
                        }
                        else if(card.rank == ONE_PAIR)
                        {
                            myCard *tmp1, *tmp2;
                            tmp1 = card.get_max_card(Ace);
                            tmp2 = card.get_max_card(tmp1->trans_value-1);
                            score = tmp1->trans_value * 20 + tmp2->trans_value;
                        }
                        else if (p->m_cards.rank == HIGH_CARD)
                        {
                            myCard *tmp1, *tmp2, *tmp3;
                            tmp1 = card.get_max_card(Ace);
                            tmp2 = card.get_max_card(tmp1->trans_value-1);
                            tmp3 = card.get_max_card(tmp2->trans_value-1);
                            score = tmp1->trans_value * 400 + tmp2->trans_value * 20 + tmp3->trans_value;
                        }
                        if (card.rank < max_rank)
                        {
                            max_rank = card.rank;
                            max_score = score;
                            max_cid = p->m_cid;
                        }
                        else if(card.rank == max_rank)
                        {
                            if (score > max_score)
                            {
                                max_score = score;
                                max_cid = p->m_cid;
                            }
                        }
                    }
                }
            }
            if (max_cid > 0)
                m_dealer = max_cid;
            return m_dealer;
        }
        case 3:
        default:
        {
            //比五张
            int max_rank = TOTAL_HAND_RANK + 1, max_score = 0, max_cid = 0;
            for (int i = 0; i < m_pos_list.size(); ++i)
            {
                int now_cid = m_pos_list[i];
                if (now_cid > 0)
                {
                    combatPlayer* p = getPlayer(now_cid);
                    if (p && p->m_combat_state > 0)
                    {
                        if (p->m_cards.cards[0].value == 0 || p->m_cards.cards[1].value == 0
                            || p->m_cards.cards[5].value == 0 || p->m_cards.cards[6].value == 0)
                        {
                            return m_dealer;
                        }
                        sevenCards card;
                        card.reset();
                        card.cards[0].set(p->m_cards.cards[0].trans_suit, p->m_cards.cards[0].trans_value);
                        card.cards[1].set(p->m_cards.cards[1].trans_suit, p->m_cards.cards[1].trans_value);
                        card.cards[2].set(p->m_cards.cards[2].trans_suit, p->m_cards.cards[2].trans_value);
                        card.cards[5].set(p->m_cards.cards[5].trans_suit, p->m_cards.cards[5].trans_value);
                        card.cards[6].set(p->m_cards.cards[6].trans_suit, p->m_cards.cards[6].trans_value);
                        card.evaluator();
                        if (card.rank < max_rank)
                        {
                            max_rank = card.rank;
                            max_score = card.score;
                            max_cid = p->m_cid;
                        }
                        else if(card.rank == max_rank)
                        {
                            if (card.score > max_score)
                            {
                                max_score = card.score;
                                max_cid = p->m_cid;
                            }
                        }
                    }
                }
            }
            if (max_cid > 0)
                m_dealer = max_cid;
            return m_dealer;
        }
    }
    return m_dealer;
}

int multi_cardgame::deal1()
{
    //第一次发牌是2张公共牌
    int start_cid = dealer();
    std::vector<std::pair<int, int> > card_list;
    baseCard* card1 = deal();
    card_list.push_back( std::make_pair(0, card1->suit*100+card1->value) );
    baseCard* card2 = deal();
    card_list.push_back( std::make_pair(0, card2->suit*100+card2->value) );

    public_cards[0].set(card1->suit,card1->value);
    public_cards[1].set(card2->suit,card2->value);

    //循环给所有玩家发公共牌
    int now_cid = start_cid;
    combatPlayer* p = getPlayer(now_cid);
    while (p && p->m_deal_state == 0)
    {
        cout << "deal 1 public2Card = " << p->m_cid << endl;
        p->deal_public_1(*card1);
        p->deal_public_2(*card2);
        p->m_deal_state = 1;
        now_cid = nextCid(now_cid);
        p = getPlayer(now_cid);
    }

    m_next = dealer();
    ++m_seq;
    broadCastCard(false, card_list, m_next, m_seq);
    resetDeal();

    //npc自动
    if (IS_NPC(m_next))
    {
        json_spirit::mObject obj;
        obj["cmd"] = "multi_npcAct";
        obj["id"] = m_combat_id;
        obj["cid"] = m_next;
        obj["type"] = NPCActRandom(m_next);
        obj["seq"] = m_seq;

        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(2, 1, obj,1));
        splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return HC_SUCCESS_NO_RET;
}

int multi_cardgame::deal2()
{
    cout << "deal 2 private1Card" << endl;
    //第二次发五张私有牌
    int start_cid = dealer();
    std::vector<std::pair<int, int> > card_list;

    //循环给所有玩家发
    int now_cid = start_cid;
    combatPlayer* p = getPlayer(now_cid);
    cout << now_cid << endl;
    while (p && p->m_deal_state == 0)
    {
        cout << "deal 2 private1Card = " << p->m_cid << endl;
        baseCard* card = deal();
        //private1是英雄卡，初始化即设置
        p->deal_private_2(*card);
        card_list.push_back( std::make_pair(p->m_cid, card->suit*100+card->value) );
        p->m_deal_state = 1;
        now_cid = nextCid(now_cid);
        p = getPlayer(now_cid);
    }

    m_next = dealer();
    ++m_seq;
    broadCastCard(true, card_list, m_next, m_seq);
    resetDeal();

    //npc自动
    if (IS_NPC(m_next))
    {
        json_spirit::mObject obj;
        obj["cmd"] = "multi_npcAct";
        obj["id"] = m_combat_id;
        obj["cid"] = m_next;
        obj["type"] = NPCActRandom(m_next);
        obj["seq"] = m_seq;

        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(2, 1, obj,1));
        splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return HC_SUCCESS_NO_RET;
}

int multi_cardgame::deal3()
{
    //第三次发牌是2张公共牌
    int start_cid = dealer();
    std::vector<std::pair<int, int> > card_list;
    baseCard* card1 = deal();
    card_list.push_back( std::make_pair(0, card1->suit*100+card1->value) );
    baseCard* card2 = deal();
    card_list.push_back( std::make_pair(0, card2->suit*100+card2->value) );

    public_cards[2].set(card1->suit,card1->value);
    public_cards[3].set(card2->suit,card2->value);

    //循环给所有玩家发公共牌
    int now_cid = start_cid;
    combatPlayer* p = getPlayer(now_cid);
    while (p && p->m_deal_state == 0)
    {
        cout << "deal 3 public2Card = " << p->m_cid << endl;
        p->deal_public_3(*card1);
        p->deal_public_4(*card2);
        p->m_deal_state = 1;
        now_cid = nextCid(now_cid);
        p = getPlayer(now_cid);
    }

    m_next = dealer();
    ++m_seq;
    broadCastCard(false, card_list, m_next, m_seq);
    resetDeal();

    //npc自动
    if (IS_NPC(m_next))
    {
        json_spirit::mObject obj;
        obj["cmd"] = "multi_npcAct";
        obj["id"] = m_combat_id;
        obj["cid"] = m_next;
        obj["type"] = NPCActRandom(m_next);
        obj["seq"] = m_seq;

        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(2, 1, obj,1));
        splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return HC_SUCCESS_NO_RET;

}

int multi_cardgame::deal4()
{
    //最后次发五张私有牌
    int start_cid = dealer();
    std::vector<std::pair<int, int> > card_list;

    //循环给所有玩家发
    int now_cid = start_cid;
    combatPlayer* p = getPlayer(now_cid);
    while (p && p->m_deal_state == 0)
    {
        cout << "deal 4 private1Card = " << p->m_cid << endl;
        baseCard* card = deal();
        p->deal_private_3(*card);
        card_list.push_back( std::make_pair(p->m_cid, card->suit*100+card->value) );
        p->m_deal_state = 1;
        now_cid = nextCid(now_cid);
        p = getPlayer(now_cid);
    }

    m_next = dealer();
    ++m_seq;
    broadCastCard(true, card_list, m_next, m_seq);
    resetDeal();

    //npc自动
    if (IS_NPC(m_next))
    {
        json_spirit::mObject obj;
        obj["cmd"] = "multi_npcAct";
        obj["id"] = m_combat_id;
        obj["cid"] = m_next;
        obj["type"] = NPCActRandom(m_next);
        obj["seq"] = m_seq;

        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(2, 1, obj,1));
        splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return HC_SUCCESS_NO_RET;
}

void multi_cardgame::betBase()
{
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->inCombat())
            {
                p->m_silver -= m_base_bet;
                p->m_bet_silver += m_base_bet;
            }
        }
    }
}

void multi_cardgame::getSilver()
{
    cout << "multi_cardgame getSilver total " << m_total_bet << endl;
    //获胜玩家下注情况
    int winner_bet = 0;
    //遍历玩家统计获胜玩家下注情况
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            bool w = false;
            if (find(m_winner_list.begin(),m_winner_list.end(),now_cid) != m_winner_list.end())
            {
                w = true;
            }
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->m_combat_state > 0 && w)
            {
                //统计获胜玩家信息
                winner_bet += p->m_bet_silver;
                //获胜玩家筹码不参与最后偿还
                m_total_bet -= p->m_bet_silver;
                cout << "winner " << p->m_cid << " bet " << p->m_bet_silver << endl;
            }
        }
    }
    cout << "total win_bet=" << winner_bet << endl;
    cout << "desk total=" << m_total_bet << endl;
    int cnt = 1;
    //遍历玩家处理失败玩家筹码
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            bool w = false;
            if (find(m_winner_list.begin(),m_winner_list.end(),now_cid) != m_winner_list.end())
            {
                w = true;
            }
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->m_combat_state > 0 && !w)
            {
                cout << "loser " << p->m_cid << " bet " << p->m_bet_silver << endl;
                //失败玩家下注够偿还多余部分取回
                if (p->m_bet_silver >= winner_bet)
                {
                    p->m_silver += (p->m_bet_silver - winner_bet);
                    //够还的不参与最后偿还
                    m_total_bet -= p->m_bet_silver;
                    ++cnt;
                }
            }
        }
    }
    //剩余筹码都是不够还债和逃跑的
    if (m_total_bet >= winner_bet)
    {
        //只偿还一次
        ++cnt;
    }
    //遍历玩家处理获胜玩家筹码
    cout << "winner get" << cnt << " times bet" << endl;
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            bool w = false;
            if (find(m_winner_list.begin(),m_winner_list.end(),now_cid) != m_winner_list.end())
            {
                w = true;
            }
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->m_combat_state > 0 && w)
            {
                p->m_silver += (p->m_bet_silver * cnt);
                cout << "winner " << p->m_cid << " get " << (p->m_bet_silver * cnt) << endl;
            }
        }
    }
    //筹码直接结算
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->m_combat_state > 0)
            {
                if (p->m_silver < 0)
                    p->m_silver = 0;
                CharData* pc = GeneralDataMgr::getInstance()->GetCharData(p->m_cid).get();
                if (pc)
                {
                    pc->silver(p->m_silver);
                }
            }
        }
    }
}

void multi_cardgame::backSilver()
{
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->m_combat_state > 0)
            {
                p->m_silver = p->m_org_silver;
            }
        }
    }
}

int multi_cardgame::startCid()
{
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->inCombat())
            {
                return p->m_cid;
            }
        }
    }
    return 0;
}

int multi_cardgame::nextCid(int cid)
{
    cout << "find "<<cid<<" nextCid" << endl;
    if (cid == 0)
        return 0;
    if (m_player_cnt <= 1)
        return 0;
    int pos = 1;
    //遍历位置定位到当前玩家下一个
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid == cid)
        {
            pos = i + 2;
        }
    }
    while (pos <= m_pos_list.size() && m_pos_list[pos-1] == 0)
    {
        ++pos;
    }
    cout << "that is pos=" << pos << endl;
    //当前已经是队列末尾则返回第一个
    if (pos > m_pos_list.size())
    {
        return startCid();
    }
    assert(pos > 0);
    combatPlayer* p = getPlayer(m_pos_list[pos-1]);
    if (p && p->inCombat() && p->m_combat_state != 2)
    {
        cout << "find next " << p->m_cid << endl;
        return p->m_cid;
    }
    else if (p)
    {
        return nextCid(p->m_cid);
    }
}

void multi_cardgame::resetCall()
{
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->m_combat_state > 0)
            {
                p->m_combat_state = 0;
            }
        }
    }
}

void multi_cardgame::resetDeal()
{
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p && p->m_deal_state > 0)
            {
                p->m_deal_state = 0;
                p->m_round_bet = 0;
            }
        }
    }
}

multi_combat::multi_combat()
{
    m_combat_id = GeneralDataMgr::getInstance()->newCombatId();
    m_type = COMBAT_TYPE_INIT;
    m_base_bet = 0;
    m_ignore_time = time(NULL) + iCombatIgnoreSec;
    m_client_check = 1;
    m_state = COMBAT_STATE_INIT;

    m_data_id = 0;
    m_data_type = 0;
    m_extra_data[0] = 0;
    m_extra_data[1] = 0;

    m_archive_report = 0;
    m_cur_game = NULL;
    m_winner = 0;
}

void multi_combat::onStart()
{
    if (m_state == COMBAT_STATE_END)
    {
        return;
    }
    if (m_cur_game == NULL)
    {
        m_cur_game = start_a_game();
    }
    if (m_cur_game && m_cur_game->m_state == COMBAT_STATE_INIT)
    {
        m_cur_game->onStart();
    }
    return;
}

int multi_combat::quit(int cid)
{
    int ret = HC_SUCCESS;
    if (m_state == COMBAT_STATE_END)
    {
        return HC_SUCCESS;
    }
    combatPlayer* p = getPlayer(cid);
    if (p && p->inCombat())
    {
        cout << m_combat_id << " combat has quit confirm" << endl;
        p->m_combat_state = -2;
        if (m_client_check > 0)
            --m_client_check;
    }
    if (m_cur_game && m_cur_game->m_state != COMBAT_STATE_END)
    {
        json_spirit::Object o;
        o.push_back( Pair("cmd", "multi_quit") );
        o.push_back( Pair("s", 200) );
        o.push_back( Pair("cid", cid) );
        m_cur_game->broadCastMsg(write(o));
        if (p && p->m_combat_state != -1)
        {
            //退出时如果在等候操作选择放弃
            if (cid == m_cur_game->m_next || m_cur_game->m_next == 0)
            {
                ret = m_cur_game->onFold(cid);
            }
            else
            {
                ++m_cur_game->m_fold_cnt;
                if (m_cur_game->m_fold_cnt >= m_cur_game->m_player_cnt - 1)//只剩一个没放弃则结束战斗
                {
                    m_cur_game->lastAct.type = COMBAT_CMD_FOLD;
                    m_cur_game->m_state = COMBAT_STATE_END;
                    ret = m_cur_game->onEnd();
                }
            }
        }
    }
    //离开玩家踢出战斗管理
    removePlayer(p);
    //玩家离开pk房间
    if (m_type == COMBAT_TYPE_PK)
    {
        Singleton<PkMgr>::Instance().leaveRoom(cid);
    }
    return ret;
}

int multi_combat::continueNext(int cid)
{
    if (m_state == COMBAT_STATE_END)
    {
        return HC_ERROR;
    }
    if (m_cur_game)
    {
        return HC_ERROR;
    }
    combatPlayer* p = getPlayer(cid);
    if (p)
    {
        cout << m_combat_id << " combat has next confirm" << endl;
        if (p->m_hp_left > 0 && (p->m_silver> (m_base_bet*4)))
        {
            if (m_client_check > 0)
                --m_client_check;
            return HC_SUCCESS;
        }
        else if(p->m_hp_left <= 0)
        {
            if (p->m_silver > (100000 + (m_base_bet*4)))
            {
                p->m_hp_left = p->m_hp_org;
                p->m_silver -= 100000;
                CharData* pc = GeneralDataMgr::getInstance()->GetCharData(p->m_cid).get();
                if (pc)
                {
                    pc->silver(p->m_silver);
                    statistics_of_silver_cost(pc->m_id, pc->m_ip_address, 100000, silver_cost_pk, pc->m_union_id, pc->m_server_id);
                }
                if (m_client_check > 0)
                    --m_client_check;
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

int multi_combat::checkEnd()
{
    //是否结束了，一方筹码为0，或者血量为0
    if (m_cur_game)
    {
        if (m_cur_game->m_state == COMBAT_STATE_END)
        {
            int lose_cnt = 0, quit_cnt = 0;
            int live_cid = 0;
            std::map<int, boost::shared_ptr<combatPlayer> >::iterator it = m_players.begin();
            while (it != m_players.end())
            {
                combatPlayer* p = it->second.get();
                if (p)
                {
                    if (p->m_combat_state < 0)
                    {
                        ++lose_cnt;
                        ++quit_cnt;
                    }
                    else if ((p->m_hp_left <= 0 || p->m_silver < m_cur_game->m_base_bet))
                    {
                        ++lose_cnt;
                    }
                    else
                    {
                        live_cid = p->m_cid;
                    }
                }
                ++it;
            }
            cout << "lose cnt=" << lose_cnt << ",total_cnt=" << m_players.size() << endl;
            if (lose_cnt >= m_players.size() - 1)
            {
                cout << "end combat win=" << live_cid << endl;
                m_state = COMBAT_STATE_END;
                m_winner = live_cid;
                json_spirit::mObject obj;
                obj["cmd"] = "multi_gameEnd";
                obj["id"] = m_combat_id;
                InsertInternalActionWork(obj);
                return HC_SUCCESS;
            }
            else
            {
                int wait_sec = 17;
                if (m_cur_game->m_result == GAME_FOLD)
                {
                    wait_sec = 9;
                }
                delete m_cur_game;
                m_cur_game = NULL;
                m_client_check = m_players.size() - quit_cnt;
                m_ignore_time = time(NULL) + iCombatIgnoreSec;
                m_state = COMBAT_STATE_INIT;
                cout << "next combat need " << m_client_check << " check!!!" << endl;
                return HC_SUCCESS_NO_RET;
            }
        }
    }
    return HC_ERROR;
}

multi_cardgame* multi_combat::start_a_game()
{
    multi_cardgame* new_game = new multi_cardgame(this);
    return new_game;
}

combatPlayer* multi_combat::getPlayer(int cid)
{
    std::map<int, boost::shared_ptr<combatPlayer> >::iterator it = m_players.find(cid);
    if (it != m_players.end())
    {
        return it->second.get();
    }
    return NULL;
}

void multi_combat::addPlayer(int pos, boost::shared_ptr<combatPlayer> p)
{
    multi_combat* tmpCombat = Singleton<MultiCombatMgr>::Instance().getCharMultiCombat(p->m_cid);
    if (tmpCombat)
    {
        ERR();
        return;
    }
    if (pos > m_pos_list.size() || pos <= 0 || m_pos_list[pos-1] > 0)
    {
        ERR();
        return;
    }
    Singleton<MultiCombatMgr>::Instance().addCharMultiCombat(p->m_cid, this);
    m_pos_list[pos-1] = p->m_cid;
    cout << "combat state = " << m_state << endl;
    if (m_cur_game && m_state == COMBAT_STATE_INGOING)
    {
        m_cur_game->m_pos_list[pos-1] = p->m_cid;
    }
    std::map<int, boost::shared_ptr<combatPlayer> >::iterator it = m_players.find(p->m_cid);
    if (it == m_players.end())
    {
        m_players[p->m_cid] = p;
        p->m_combat_state = -1;//状态为未参加
    }
    return;
}

void multi_combat::removePlayer(combatPlayer* p)
{
    if (p == NULL)
        return;
    //筹码直接结算
    if (p->m_combat_state > 0)
    {
        if (p->m_silver < 0)
            p->m_silver = 0;
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(p->m_cid).get();
        if (pc)
        {
            pc->silver(p->m_silver);
        }
    }
    multi_combat* tmpCombat = Singleton<MultiCombatMgr>::Instance().getCharMultiCombat(p->m_cid);
    if (tmpCombat && tmpCombat->m_combat_id == m_combat_id)
    {
        Singleton<MultiCombatMgr>::Instance().removeCharMultiCombat(p->m_cid);
    }
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        if (m_pos_list[i] == p->m_cid)
        {
            m_pos_list[i] = 0;
            if (m_cur_game && m_cur_game->m_state == COMBAT_STATE_INGOING)
            {
                m_cur_game->m_pos_list[i] = 0;
            }
            break;
        }
    }
    std::map<int, boost::shared_ptr<combatPlayer> >::iterator it = m_players.find(p->m_cid);
    if (it != m_players.end())
    {
        if (it->second.get() == p)
        {
            m_players.erase(it);
        }
    }
    return;
}

void multi_combat::toObj(json_spirit::Object& obj)
{
    cout << "toObj combat_state=" << m_state << endl;
    obj.push_back( Pair("combat_state", m_state) );
    obj.push_back( Pair("combat_type", m_type) );
    obj.push_back( Pair("bet", m_base_bet) );
    int wait_time = iCombatWaitSec;
    obj.push_back( Pair("secs", wait_time) );
    if (m_cur_game)
    {
        obj.push_back( Pair("round", m_cur_game->m_round + 1) );
        obj.push_back( Pair("silver", m_cur_game->m_silver) );
        json_spirit::Array public_list;
        for (int i = 0; i < 4; ++i)
        {
            int pos = 1;
            if (i == 0)
                pos = 1;
            else if(i == 1)
                pos = 2;
            else if(i == 2)
                pos = 6;
            else if(i == 3)
                pos = 7;
            if (m_cur_game->public_cards[i].value > 0)
            {
                json_spirit::Object c;
                c.push_back( Pair("pos", pos) );
                c.push_back( Pair("card", m_cur_game->public_cards[i].suit * 100 + m_cur_game->public_cards[i].value) );
                public_list.push_back(c);
            }
        }
        obj.push_back( Pair("public_list", public_list) );
    }
}

int multi_combat::processCombat(int cid, int cmd, int seq)
{
    cout<<"processCombat("<<cid<<","<<cmd<<","<<seq<<"), combat id "<<m_combat_id<<endl;
    if (m_cur_game == NULL || m_state != COMBAT_STATE_INGOING || m_cur_game->m_state != COMBAT_STATE_INGOING)
    {
        ERR();
        cout<<"cur game ->"<<m_cur_game<<endl;
        if (m_cur_game)
        {
            cout<<"cur game state->"<<m_cur_game->m_state<<endl;
        }
        return HC_ERROR;
    }

    if (cid != m_cur_game->m_next || seq != m_cur_game->m_seq)
    {
        cout << "cid=" << cid << ",next=" << m_cur_game->m_next << endl;
        cout << "seq=" << seq << ",m_seq=" << m_cur_game->m_seq << endl;
        ERR();
        return HC_ERROR;
    }
    switch (cmd)
    {
        case COMBAT_CMD_CHECK:
            return m_cur_game->onCheck(cid);

        case COMBAT_CMD_CALL:
            return m_cur_game->onCall(cid);

        case COMBAT_CMD_DOUBLEBET:
            return m_cur_game->onDouble(cid);

        case COMBAT_CMD_HALFBET:
            return m_cur_game->onHalf(cid);

        case COMBAT_CMD_FOLD:
            return m_cur_game->onFold(cid);

        default:
            return HC_ERROR;
    }
}

void multi_combat::getCombatInfo(int cid, json_spirit::Object& robj)
{
    //本轮发牌信息
    json_spirit::Array list;
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        json_spirit::Object tmp;
        tmp.push_back( Pair("pos", i+1) );
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            combatPlayer* p = getPlayer(now_cid);
            if (p)
            {
                json_spirit::Object info;
                p->getInfo(info);
                tmp.push_back( Pair("self", cid == now_cid) );
                tmp.push_back( Pair("combat", p->inCombat()) );
                tmp.push_back( Pair("info", info) );
            }
        }
        list.push_back(tmp);
    }
    robj.push_back( Pair("list", list) );
}

void multi_combat::AppendResult(json_spirit::Object obj)
{
    m_result_array.push_back(obj);
}

void multi_combat::broadCastCharList()
{
    for (int i = 0; i < m_pos_list.size(); ++i)
    {
        int now_cid = m_pos_list[i];
        if (now_cid > 0)
        {
            CharData* tc = GeneralDataMgr::getInstance()->GetCharData(now_cid).get();
            boost::shared_ptr<OnlineCharactor> toc = GeneralDataMgr::getInstance()->GetOnlineCharactor(tc->m_name);
            if (toc.get())
            {
                json_spirit::Object obj;
                obj.push_back( Pair("cmd", "multi_charlist") );
                obj.push_back( Pair("s", 200) );
                getCombatInfo(tc->m_id, obj);
                toc->Send(write(obj));
            }
        }
    }
}

MultiCombatMgr::MultiCombatMgr()
{
    rwlock_init(&combats_rwmutex);
}

multi_combat* MultiCombatMgr::findMultiCombat(int id)
{
    readLock lockit(&combats_rwmutex);
    std::map<int, multi_combat* >::iterator it = m_combats.find(id);
    if (it != m_combats.end())
    {
        return it->second;
    }
    return NULL;
}

multi_combat* MultiCombatMgr::getCharMultiCombat(int cid)
{
    std::map<int, multi_combat* >::iterator it = m_char_combats.find(cid);
    if (it != m_char_combats.end())
    {
        return it->second;
    }
    return NULL;
}

void MultiCombatMgr::addCharMultiCombat(int cid, multi_combat* pCombat)
{
    m_char_combats[cid] = pCombat;
}

void MultiCombatMgr::removeCharMultiCombat(int cid)
{
    m_char_combats.erase(cid);
}

multi_combat* MultiCombatMgr::createPkCombat(std::vector<int>& cid_list, int bet)
{
    multi_combat* new_combat = new multi_combat;
    new_combat->m_type = COMBAT_TYPE_PK;
    new_combat->m_base_bet = bet;
    for (int i = 0; i < cid_list.size(); ++i)
    {
        int now_cid = cid_list[i];
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(now_cid).get();
        if (pc)
        {
            boost::shared_ptr<combatPlayer> p;
            p.reset(new combatPlayer);
            p->LoadCharactor(*pc);
            p->m_auto = false;
            new_combat->m_players[now_cid] = p;
            if (p->m_silver < bet)
            {
                return NULL;
            }
        }
        new_combat->m_pos_list.push_back(now_cid);
    }
    cout << "createPKCombat size=" << new_combat->m_players.size() << endl;

    //创建战斗牌局等候客户端确认才开始
    //new_combat->m_cur_game = new_combat->start_a_game();
    new_combat->m_client_check = new_combat->m_players.size();

    writeLock lockit(&combats_rwmutex);
    m_combats[new_combat->m_combat_id] = new_combat;
    for (int i = 0; i < cid_list.size(); ++i)
    {
        int now_cid = cid_list[i];
        if (now_cid > 0)
        {
            m_char_combats[now_cid] = new_combat;
        }
    }
    cout<<"createPkCombat("<<new_combat->m_players.size()<<")"<<endl;
    return new_combat;
}

void MultiCombatMgr::autoLoop()
{
    readLock lockit(&combats_rwmutex);
    time_t t_now = time(NULL);
    std::map<int, multi_combat* >::iterator it = m_combats.begin();
    while (it != m_combats.end())
    {
        if (it->second == NULL || it->second->m_state != COMBAT_STATE_INIT)
        {
            ++it;
            continue;
        }
        multi_combat* p = it->second;
        if (p->m_client_check <= 0)
        {
            if (p->m_cur_game == NULL)
            {
                p->m_cur_game = p->start_a_game();
            }
            if (p->m_cur_game->m_player_cnt > 1)
            {
                //通知参战玩家列表
                p->broadCastCharList();
                //通知客户端全部就绪
                json_spirit::Object o;
                o.push_back( Pair("cmd", "multi_CombatReady") );
                o.push_back( Pair("s", 200) );
                p->m_cur_game->broadCastMsg(json_spirit::write(o, json_spirit::raw_utf8));

                //客户端确认则正式开始
                cout << "autoloop set combat_state ingoing" << endl;
                p->m_state = COMBAT_STATE_INGOING;
                //定时器开始
                json_spirit::mObject mobj;
                mobj["cmd"] = "multi_startGame";
                mobj["id"] = p->m_combat_id;
                boost::shared_ptr<splsTimer> tmsg;
                int wait_time = iCombatWaitSec;
                tmsg.reset(new splsTimer(wait_time, 1, mobj,1));
                p->m_cur_game->m_timer = splsTimerMgr::getInstance()->addTimer(tmsg);
            }
            else
            {
                //通知客户端人数不足
                json_spirit::Object o;
                o.push_back( Pair("cmd", "multi_CombatFail") );
                o.push_back( Pair("s", 200) );
                p->m_cur_game->broadCastMsg(json_spirit::write(o, json_spirit::raw_utf8));

                p->m_state = COMBAT_STATE_END;
                json_spirit::mObject mobj;
                mobj["cmd"] = "multi_actTimeout";
                mobj["id"] = p->m_combat_id;
                mobj["notify"] = 0;
                boost::shared_ptr<splsTimer> tmsg;
                tmsg.reset(new splsTimer(1, 1, mobj,1));
                p->m_cur_game->m_timer = splsTimerMgr::getInstance()->addTimer(tmsg);
            }
        }
        else if (p->m_ignore_time < t_now)
        {
            p->m_state = COMBAT_STATE_END;
            //超时处理
            json_spirit::mObject mobj;
            mobj["cmd"] = "multi_actTimeout";
            mobj["id"] = p->m_combat_id;
            mobj["notify"] = 1;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(1, 1, mobj,1));
            //p->m_cur_game->m_timer = splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        ++it;
    }
}

void MultiCombatMgr::onCombatEnd(int id)
{
    cout << "MultiCombatMgr::onCombatEnd id " << id << endl;
    std::map<int, multi_combat* >::iterator it = m_combats.find(id);
    if (it != m_combats.end())
    {
        multi_combat* pb = it->second;
        if (pb)
        {
            cout << "flag11111" << endl;
            switch (pb->m_type)
            {
                case COMBAT_TYPE_PK:
                    Singleton<PkMgr>::Instance().combatAllResult(pb);
                    break;
            }
            cout << "flag22222" << endl;
            json_spirit::Object o;
            o.push_back( Pair("cmd", "multi_MatchResult"));
            o.push_back( Pair("s", 200));
            json_spirit::Array list;
            for (int i = 0; i < pb->m_pos_list.size(); ++i)
            {
                json_spirit::Object tmp;
                tmp.push_back( Pair("pos", i+1) );
                int now_cid = pb->m_pos_list[i];
                if (now_cid > 0)
                {
                    tmp.push_back( Pair("win", now_cid == pb->m_winner) );
                    combatPlayer* p = pb->getPlayer(now_cid);
                    if (p)
                    {
                        json_spirit::Object info;
                        p->getInfo(info);
                        tmp.push_back( Pair("info", info) );
                    }
                }
                list.push_back(tmp);
            }
            cout << "flag33333" << endl;
            o.push_back( Pair("list", list) );
            o.push_back( Pair("type", pb->m_type));
            pb->m_result_obj.push_back( Pair("extra1", pb->m_extra_data[0]) );
            pb->m_result_obj.push_back( Pair("extra2", pb->m_extra_data[1]) );
            o.push_back( Pair("result", pb->m_result_obj));
            //结果发送
            std::string msg = write(o);
            pb->m_cur_game->broadCastMsg(msg);
            pb->AppendResult(o);
            //保存战斗
            {
                //战斗结果存入数据库
            }
            //数据清理
            cout << "flag44444" << endl;
            delete pb->m_cur_game;
            pb->m_cur_game = NULL;
            cout << "flag55555" << endl;
            for (int i = 0; i < pb->m_pos_list.size(); ++i)
            {
                int now_cid = pb->m_pos_list[i];
                if (now_cid > 0)
                {
                    multi_combat* tmpCombat = getCharMultiCombat(now_cid);
                    if (tmpCombat && tmpCombat->m_combat_id == pb->m_combat_id)
                    {
                        cout << "remove CharMultiCombat cid=" << now_cid << endl;
                        m_char_combats.erase(now_cid);
                    }
                }
            }
        }
            cout << "flag66666" << endl;
        writeLock lockit(&combats_rwmutex);
        m_combats.erase(it);
            cout << "flag77777" << endl;
        delete pb;
            cout << "flag88888" << endl;
    }
}

void MultiCombatMgr::combatTimeout(int id, int notify)
{
    cout << "MultiCombatMgr::combatTimeout " << id << endl;
    multi_combat* pb = findMultiCombat(id);
    if (pb)
    {
        switch (pb->m_type)
        {
            case COMBAT_TYPE_PK:
                Singleton<PkMgr>::Instance().combatAllResult(pb);
                break;
        }
        if (pb->m_cur_game && notify == 1)
        {
            if (notify == 1)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("cmd", "multi_combatTimeout") );
                obj.push_back( Pair("s", 200) );
                std::string msg = json_spirit::write(obj);
                pb->m_cur_game->broadCastMsg(msg);
            }
            delete pb->m_cur_game;
            pb->m_cur_game = NULL;
        }
        for (int i = 0; i < pb->m_pos_list.size(); ++i)
        {
            int now_cid = pb->m_pos_list[i];
            if (now_cid > 0)
            {
                multi_combat* tmpCombat = getCharMultiCombat(now_cid);
                if (tmpCombat && tmpCombat->m_combat_id == pb->m_combat_id)
                {
                    m_char_combats.erase(now_cid);
                }
            }
        }
        writeLock lockit(&combats_rwmutex);
        m_combats.erase(id);
        delete pb;
    }
}

bool MultiCombatProcesser::work(MultiCombatCmd &Cmd)       // 在些完成实际任务.
{
    try
    {
        int ret = HC_SUCCESS;
        std::string cmd = "";
        json_spirit::Object robj;
        if (Cmd.cmd == combat_cmd_create)
        {
            cmd = "multi_challenge";
            ret = createMultiCombat(Cmd.cid,Cmd.mobj,robj);
        }
        else
        {
            multi_combat* combat = Cmd._pCombat;
            if (combat == NULL)
            {
                std::cout<<"CombatProcess receive a empty combat...break."<<endl;
                return false;
            }
            //处理命令
            switch (Cmd.cmd)
            {
                case combat_cmd_player_act:
                    {
                        cmd = "multi_myAct";
                        int type = 0, seq = 0;
                        READ_INT_FROM_MOBJ(type,Cmd.mobj,"type");
                        READ_INT_FROM_MOBJ(seq,Cmd.mobj,"seq");
                        ret = combat->processCombat(Cmd.cid, type, seq);
                    }
                    break;
                case combat_cmd_player_quit:
                    {
                        cmd = "multi_quitChallenge";
                        combat->quit(Cmd.cid);
                    }
                    break;
                case combat_cmd_sign:
                    {
                        cmd = "multi_combatSign";
                        if (combat->m_client_check > 0)
                            --combat->m_client_check;
                        robj.push_back( Pair("wait", combat->m_client_check > 0));
                    }
                    break;
                case combat_cmd_continue:
                    {
                        cmd = "multi_continueChallenge";
                        ret = combat->continueNext(Cmd.cid);
                    }
                    break;
                case combat_cmd_start:
                    {
                        combat->onStart();
                    }
                    break;
                case combat_cmd_end:
                    {
                        Singleton<MultiCombatMgr>::Instance().onCombatEnd(combat->m_combat_id);
                    }
                    break;
                case combat_cmd_timeout:
                    {
                        int notify = 0;
                        READ_INT_FROM_MOBJ(notify,Cmd.mobj,"notify");
                        Singleton<MultiCombatMgr>::Instance().combatTimeout(combat->m_combat_id, notify);
                    }
                    break;
                case combat_cmd_query:
                    {
                        cout << "combat_cmd_query" << endl;
                        cmd = "multi_queryCombatInfo";
                        if (combat)
                        {
                            combat->toObj(robj);
                            combat->broadCastCharList();
                        }
                        ret = HC_SUCCESS;
                    }
                    break;
            }
        }
        if (HC_SUCCESS_NO_RET != ret && Cmd.cid > 0 && cmd != "")
        {
            robj.push_back( Pair("cmd", cmd) );
            if (HC_ERROR_NO_RET != ret)
            {
                robj.push_back( Pair("s", ret));
                if (ret != HC_SUCCESS)
                {
                    std::string msg = getErrMsg(ret);
                    if ("" != msg)
                    {
                        robj.push_back( Pair("msg", msg));
                    }
                }
            }
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(Cmd.cname);
            if (account.get())
            {
                account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
            }
        }
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "multi_combat work , Exception: " << e.what() << "\n";
    }
    return true;
}

