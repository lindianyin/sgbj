
#include "shenling.h"

#include "errcode_def.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_lang.h"
#include "net.h"
#include "spls_timer.h"
#include "singleton.h"
#include "relation.h"
#include "weekRanking.h"

#define INFO(x)// cout<<x

extern void InsertSaveDb(const std::string& sql);
extern Database& GetDb();

int ProcessGetShenlingList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<shenlingMgr>::Instance().getShenlingList(pc->m_id, robj);
}

int ProcessGetShenlingInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<shenlingMgr>::Instance().getCharShenlingInfo(pc->m_id, robj);
}

int ProcessRefreshSkill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<shenlingMgr>::Instance().refreshSkill(pc->m_id);
}

int ProcessResetShenling(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<shenlingMgr>::Instance().reset(pc->m_id, true);
}

int ProcessBuyShenlingTimes(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<shenlingMgr>::Instance().buyTimes(pc->m_id);
}

int ProcessQueryShenlingShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    robj.push_back( Pair("coin", pc->m_bag.getGemCount(GEM_ID_SHENLING_COIN)) );
    return Singleton<shenlingMgr>::Instance().getShenlingShop(o, robj);
}

int ProcessBuyShenlingShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0, nums = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    if (nums < 1)
        nums = 1;
    boost::shared_ptr<baseShenlingGoods> bs = Singleton<shenlingMgr>::Instance().GetBaseShenlingGoods(id);
    if (bs.get())
    {
        int cost = bs->cost * nums;
        if (cost < 0)
        {
            return HC_ERROR;
        }
        std::list<Item> items;
        Item tmp(bs->m_item.type, bs->m_item.id, bs->m_item.nums*nums, bs->m_item.extra);
        items.push_back(tmp);
        if (!cdata->m_bag.hasSlot(itemlistNeedBagSlot(items)))
        {
            return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
        }
        if (cdata->subGem(GEM_ID_SHENLING_COIN, cost, gem_cost_shenling_shop) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GEM;
        }
        giveLoots(cdata.get(),items,NULL,&robj,true,loot_shenling_shop);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void baseShenling::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_id));
    obj.push_back( Pair("level", m_level));
    if (m_hero.get())
    {
        obj.push_back( Pair("name", m_hero->m_name));
        obj.push_back( Pair("spic", m_hero->m_spic));
    }
    return;
}

void CharShenling::load()
{
    m_sid = 0;
    Query q(GetDb());
    q.get_result("select sid,skill1,skill2,skill3 from char_shenling where cid=" + LEX_CAST_STR(m_cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        int sid = q.getval();
        boost::shared_ptr<baseShenling> p = Singleton<shenlingMgr>::Instance().getShenlingById(sid);
        if (p.get())
        {
            m_sid = sid;
            m_magics[0] = q.getval();
            m_magics[1] = q.getval();
            m_magics[2] = q.getval();
        }
    }
    q.free_result();

    //神灵塔进度
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (cdata.get())
    {
        int tmp = cdata->queryExtraData(char_data_type_normal, char_data_normal_shenling);
        if (tmp < m_sid)
        {
            cdata->setExtraData(char_data_type_normal, char_data_normal_shenling, m_sid);
        }
    }

    if (m_sid == 0)
    {
        reset();
    }
    return;
}

void CharShenling::refreshSkill()
{
    m_magics[0] = Singleton<skillMgr>::Instance().RandomSkill();
    m_magics[1] = Singleton<skillMgr>::Instance().RandomSkill();
    while (m_magics[1] == m_magics[0])
    {
        m_magics[1] = Singleton<skillMgr>::Instance().RandomSkill();
    }
    m_magics[2] = Singleton<skillMgr>::Instance().RandomSkill();
    while (m_magics[2] == m_magics[0] || m_magics[2] == m_magics[1])
    {
        m_magics[2] = Singleton<skillMgr>::Instance().RandomSkill();
    }
    save();
}

void CharShenling::reset()
{
    m_sid = 1;
    refreshSkill();
}

void CharShenling::save()
{
    InsertSaveDb("replace into char_shenling (cid,sid,skill1,skill2,skill3) values ("
                            + LEX_CAST_STR(m_cid)
                            + "," + LEX_CAST_STR(m_sid)
                            + "," + LEX_CAST_STR(m_magics[0])
                            + "," + LEX_CAST_STR(m_magics[1])
                            + "," + LEX_CAST_STR(m_magics[2])
                            + ")");
}

shenlingMgr::shenlingMgr()
{
    Query q(GetDb());
    q.get_result("select sid,silver,chat,level,name,spic,race,star,attack,defense,magic,hp from base_shenling where 1 order by sid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int sid = q.getval();
        boost::shared_ptr<baseShenling> c;
        c.reset(new baseShenling);
        c->m_id = sid;
        c->m_level = sid;
        c->m_silver = q.getval();
        c->m_chat = q.getstr();
        //加载武将
        boost::shared_ptr<baseStrongholdHeroData> sh;
        if (!(c->m_hero.get()))
        {
            sh.reset(new (baseStrongholdHeroData));
            c->m_hero = sh;
        }
        else
        {
            sh = c->m_hero;
        }
        sh->m_level = q.getval();
        sh->m_name = q.getstr();
        sh->m_spic = q.getval();
        sh->m_race = q.getval();
        sh->m_star = q.getval();
        sh->m_quality = sh->m_star;
        sh->m_attack = q.getval();
        sh->m_defense = q.getval();
        sh->m_magic = q.getval();
        sh->m_hp = q.getval();
        //掉落加载
        Singleton<lootMgr>::Instance().getShenlingLootInfo(c->m_id, c->m_Item_list);

        m_shenling.push_back(c);
        assert(m_shenling.size() == sid);
    }
    q.free_result();
    //基础神灵塔商品
    q.get_result("select id,cost,itemType,itemId,counts,extra from base_shenling_shop where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseShenlingGoods> bpa = m_base_shenling_goods[id];
        if (!bpa.get())
        {
            bpa.reset(new baseShenlingGoods);
            m_base_shenling_goods[id] = bpa;
        }
        bpa->id = id;
        bpa->cost = q.getval();
        bpa->m_item.type = q.getval();
        bpa->m_item.id = q.getval();
        bpa->m_item.nums = q.getval();
        bpa->m_item.extra = q.getval();
    }
    q.free_result();
}

void shenlingMgr::weekUpdate()
{
    std::map<int, boost::shared_ptr<CharShenling> >::iterator it =  m_char_shenling.begin();
    while (it != m_char_shenling.end())
    {
        if (it->second.get())
        {
            it->second->reset();
        }
        ++it;
    }
}

boost::shared_ptr<baseShenling> shenlingMgr::getShenlingById(int sid)
{
    if (sid >= 1 && sid <= m_shenling.size())
    {
        return m_shenling[sid-1];
    }
    boost::shared_ptr<baseShenling> p;
    p.reset();
    return p;
}

boost::shared_ptr<CharShenling> shenlingMgr::getCharShenling(int cid)
{
    std::map<int, boost::shared_ptr<CharShenling> >::iterator it =  m_char_shenling.find(cid);
    if (it != m_char_shenling.end() && it->second.get())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<CharShenling> p;
        p.reset(new CharShenling);
        p->m_cid = cid;
        p->load();
        m_char_shenling[cid] = p;
        return p;
    }
    boost::shared_ptr<CharShenling> p;
    p.reset();
    return p;
}

int shenlingMgr::getCharShenlingInfo(int cid, json_spirit::Object& robj)
{
    cout << "getCharShenlingInfo" << " cid=" << cid << endl;
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<CharShenling> cs = getCharShenling(cid);
    if (cs.get())
    {
        robj.push_back( Pair("id", cs->m_sid) );
        robj.push_back( Pair("level", cs->m_sid) );
        //magic
        json_spirit::Array magic_list;
        for(int i = 0; i < 3; ++i)
        {
            json_spirit::Object o;
            boost::shared_ptr<baseMagic> bs = Singleton<MagicMgr>::Instance().getBaseMagic(cs->m_magics[i]);
            if (bs.get())
            {
                bs->toObj(o);
            }
            magic_list.push_back(o);
        }
        robj.push_back( Pair("magic_list", magic_list) );

        //loot
        if (cs->m_sid < m_shenling.size() && m_shenling[cs->m_sid-1].get())
        {
            json_spirit::Array get_list;
            itemlistToArray(m_shenling[cs->m_sid-1]->m_Item_list, get_list);
            robj.push_back( Pair("get_list", get_list) );
        }
    }
    boost::shared_ptr<baseGem> b = GeneralDataMgr::getInstance()->GetBaseGem(GEM_ID_SHENLING_KEY);
    if (b.get())
    {
        robj.push_back( Pair("buy_cost", b->gold_to_buy) );
    }
    robj.push_back( Pair("refresh_cost", iShenlingRefreshSkillGold) );
    robj.push_back( Pair("reset_cost", iShenlingResetGold) );
    robj.push_back( Pair("free_times", iShenlingFreeTime - cdata->queryExtraData(char_data_type_daily, char_data_daily_shenling)) );
    robj.push_back( Pair("times", cdata->m_bag.getGemCount(GEM_ID_SHENLING_KEY)) );
    robj.push_back( Pair("coin", cdata->m_bag.getGemCount(GEM_ID_SHENLING_COIN)) );
    return HC_SUCCESS;
}

int shenlingMgr::getShenlingList(int cid, json_spirit::Object& robj)
{
    //cout << "getShenlingList" << " cid=" << cid << endl;
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    int now_level = 1;
    boost::shared_ptr<CharShenling> cs = getCharShenling(cid);
    if (cs.get())
    {
        now_level = cs->m_sid;
    }
    json_spirit::Array list;
    for (int i = 0; i < m_shenling.size(); ++i)
    {
        json_spirit::Object o;
        m_shenling[i]->toObj(o);
        list.push_back(o);
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int shenlingMgr::refreshSkill(int cid)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<CharShenling> cs = getCharShenling(cid);
    if (cs.get())
    {
        if (cdata->subGold(iShenlingRefreshSkillGold, gold_cost_shenling_refresh_skill) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        cs->refreshSkill();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int shenlingMgr::reset(int cid, bool needgold)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<CharShenling> cs = getCharShenling(cid);
    if (cs.get())
    {
        if (needgold)
        {
            if (cdata->subGold(iShenlingResetGold, gold_cost_shenling_reset) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
        }
        cs->reset();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int shenlingMgr::buyTimes(int cid)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<baseGem> b = GeneralDataMgr::getInstance()->GetBaseGem(GEM_ID_SHENLING_KEY);
    if (b.get())
    {
        if (cdata->subGold(b->gold_to_buy, gold_cost_shenling_add) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        cdata->addGem(GEM_ID_SHENLING_KEY, 1, gem_get_shenling_add);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int shenlingMgr::combatResult(chessCombat* pCombat)    //战斗结束
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (COMBAT_TYPE_SHENLING != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_players[0].m_cid);
    if (!cdata.get())
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<baseShenling> bs = getShenlingById(pCombat->m_data_id);
    if (!bs.get())
    {
        ERR();
        return HC_ERROR;
    }
    CharShenling* pcd = getCharShenling(pCombat->m_players[0].m_cid).get();
    if (!pcd)
    {
        ERR();
        return HC_ERROR;
    }
    cout << "shenlingMgr::combatResult   " << pCombat->m_combat_id << ",reuslt=" << pCombat->m_result << endl;
    cdata->m_score_tasks.updateTask(DAILY_SCORE_SHENLING_TOWER);
    cdata->m_tasks.updateTask(GOAL_DAILY_SHENLING_ATTACK, 0, 1);
    if (pCombat->m_result == COMBAT_RESULT_ATTACK_WIN)
    {
        cdata->m_tasks.updateTask(GOAL_SHENLING_TOWER, 0, pcd->m_sid);
        Singleton<goalMgr>::Instance().updateTask(cdata->m_id, GOAL_TYPE_SHENLING_TOWER, pcd->m_sid);
        //周排行活动
        int score = pcd->m_sid;
        if (score > 0)
            Singleton<weekRankings>::Instance().updateEventRankings(cdata->m_id,week_ranking_shenling,score);
        //神灵塔进度
        int tmp = cdata->queryExtraData(char_data_type_normal, char_data_normal_shenling);
        if (tmp < pcd->m_sid)
        {
            cdata->setExtraData(char_data_type_normal, char_data_normal_shenling, pcd->m_sid);
        }
        /*********** 广播 ************/
        std::string msg = "";
        switch(pcd->m_sid)
        {
            case 10:
                msg = strShenling10;
                break;
            case 20:
                msg = strShenling20;
                break;
            case 30:
                msg = strShenling30;
                break;
            case 40:
                msg = strShenling40;
                break;
            case 50:
                msg = strShenling50;
                break;
            default:
                break;
        }
        if (msg != "")
        {
            str_replace(msg, "$W", MakeCharNameLink(cdata->m_name));
            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }

        //广播好友祝贺
        if (10 == pcd->m_sid || 20 == pcd->m_sid || 30 == pcd->m_sid || 40 == pcd->m_sid || 50 == pcd->m_sid)
        {
            Singleton<relationMgr>::Instance().postCongradulation(cdata->m_id, CONGRATULATION_SHENLING, pcd->m_sid, 0);
        }

        ++pcd->m_sid;
        pcd->refreshSkill();
        //根据结果决定掉落数量(翻牌展现)
        /***********随机获得掉落处理****************/
        Singleton<lootMgr>::Instance().getShenlingLoots(pCombat->m_data_id, pCombat->m_getItems, 1);
        //给东西
        giveLoots(cdata, pCombat, true, loot_shenling);
        //翻牌展现另外随机19个掉落给客户端
        json_spirit::Array get_list;
        std::list<Item> all_loot;
        Singleton<lootMgr>::Instance().getShenlingLoots(bs->m_id, all_loot, 19);
        itemlistToArray(all_loot, get_list);
        itemlistToArray(pCombat->m_getItems, get_list);
        pCombat->m_result_obj.push_back( Pair("get_total", get_list) );
    }
    else if (pCombat->m_result == COMBAT_RESULT_ATTACK_LOSE)
    {
        int has_attack = cdata->queryExtraData(char_data_type_daily,char_data_daily_shenling);
        if (has_attack == 0)
        {
            cdata->setExtraData(char_data_type_daily,char_data_daily_shenling, ++has_attack);
        }
        else
        {
            cdata->subGem(GEM_ID_SHENLING_KEY,1,gem_cost_shenling);
        }
    }
    return HC_SUCCESS;
}

boost::shared_ptr<baseShenlingGoods> shenlingMgr::GetBaseShenlingGoods(int id)
{
    if (m_base_shenling_goods.find(id) != m_base_shenling_goods.end())
    {
        return m_base_shenling_goods[id];
    }
    boost::shared_ptr<baseShenlingGoods> tmp;
    tmp.reset();
    return tmp;
}

int shenlingMgr::getShenlingShop(json_spirit::mObject& o, json_spirit::Object& robj)
{
    //页面信息
    int page = 1, nums_per_page = 8;
    READ_INT_FROM_MOBJ(page, o, "page");
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
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<baseShenlingGoods> >::iterator it = m_base_shenling_goods.begin();
    while(it != m_base_shenling_goods.end())
    {
        if (it->second.get())
        {
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", it->second->id));
                obj.push_back( Pair("cost", it->second->cost));
                json_spirit::Object i_obj;
                it->second->m_item.toObj(i_obj);
                obj.push_back( Pair("item_info", i_obj));
                list.push_back(obj);
            }
        }
        ++it;
    }
    robj.push_back( Pair("list", list));
    json_spirit::Object pageobj;
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

