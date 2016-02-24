
#include "eliteCombat.h"

#include "spls_errcode.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_lang.h"
#include "net.h"
#include "army.h"
#include "spls_timer.h"
#include "statistics.h"
#include "singleton.h"
#include "relation.h"

using namespace net;

#define INFO(x)// cout<<x


extern void InsertSaveDb(const std::string& sql);
extern Database& GetDb();
extern Combat* createEliteCombat(int cid, int mapid, int eliteid, int& ret);
extern void InsertCombat(Combat* pcombat);
extern void InsertSaveCombat(Combat* pcombat);

extern std::string strEliteMsg;

//根据攻击方式和对方的阵型情况返回攻击范围
json_spirit::Array getAttackRange(int atype, int mypos, int pos[]);

eliteCombatMgr* eliteCombatMgr::m_handle = NULL;
eliteCombatMgr* eliteCombatMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new eliteCombatMgr();
        m_handle->load();
    }
    return m_handle;
}

void ItemToObj(Item* sitem, boost::shared_ptr<json_spirit::Object>& sgetobj);

int eliteCombatMgr::load()
{
    Query q(GetDb());
    q.get_result("select mapid,eliteid,stronghold,level,name,color,hit,crit,shipo,parry,resist_hit,resist_crit,resist_shipo,resist_parry,supply,gongxun from base_elite_armys where 1 order by eliteid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        int eliteid = q.getval();
        boost::shared_ptr<m_eliteCombat_list> pec = getEliteCombats(mapid);
        if (!pec.get())
        {
            pec.reset(new m_eliteCombat_list);
            m_eliteCombats[mapid] = pec;
            //cout << "new elitecombats mapid=" << mapid << endl;
        }
        boost::shared_ptr<eliteCombat> ec;
        ec.reset(new eliteCombat);
        ec->_mapid = mapid;
        ec->_id = eliteid;
        ec->_open_stronghold = q.getval();
        ec->_level = q.getval();
        ec->_name = q.getstr();
        ec->_color = q.getval();
        //特性hit,crit,shipo,parry,resist_hit,resist_crit,resist_shipo,resist_parry
        ec->m_combat_attribute.special_resist(special_attack_dodge, 10 * q.getval());
        ec->m_combat_attribute.special_attack(special_attack_baoji, 10 * q.getval());
        ec->m_combat_attribute.special_attack(special_attack_shipo, 10 * q.getval());
        ec->m_combat_attribute.special_attack(special_attack_parry, 10 * q.getval());
        ec->m_combat_attribute.special_attack(special_attack_dodge, 10 * q.getval());
        ec->m_combat_attribute.special_resist(special_attack_baoji, 10 * q.getval());
        ec->m_combat_attribute.special_resist(special_attack_shipo, 10 * q.getval());
        ec->m_combat_attribute.special_resist(special_attack_parry, 10 * q.getval());
        ec->m_combat_attribute.enable();
        //加载武将
        ec->load();
        //掉落加载
        lootMgr::getInstance()->getEliteCombatsLootInfo(ec->_id, ec->m_Item_list);
        //粮草
        ec->supply = q.getval();
        Item itm;
        itm.type = item_type_treasure;
        itm.id = treasure_type_supply;
        itm.nums = ec->supply;
        if (itm.nums > 0)
        {
            ec->m_Item_list.push_back(itm);
        }
        //功勋
        ec->gongxun = q.getval();
        Item item_gongxun;
        item_gongxun.type = item_type_treasure;
        item_gongxun.id = treasure_type_gongxun;
        item_gongxun.nums = ec->gongxun;
        if (item_gongxun.nums > 0)
        {
            ec->m_Item_list.push_back(item_gongxun);
        }
        pec->push_back(ec);

        m_eliteCombats2.push_back(ec);
        assert(m_eliteCombats2.size() == eliteid);

        //cout << "push elitecombats mapid=" << mapid << ",eliteid=" << eliteid << endl;
    }
    q.free_result();

    for (std::vector<boost::shared_ptr<eliteCombat> >::iterator it = m_eliteCombats2.begin(); it != m_eliteCombats2.end(); ++it)
    {
        if (*it != NULL)
        {
            eliteCombat* ps = (*it).get();
            //读入攻略
            q.get_result("select id,input,hurt,attackerName,alevel,aAttack from battle_records where type=9 and archive=1 and defender="
                    + LEX_CAST_STR(ps->_id) + " order by input,hurt");
            CHECK_DB_ERR(q);
            while (q.fetch_row())
            {
                int id = q.getval();
                time_t input = q.getval();
                int hurt = q.getval();
                std::string name = q.getstr();
                int level = q.getval();
                int attack = q.getval();

                ps->m_raiders.load(name, level, id, attack, hurt, input);
            }
            q.free_result();
        }
    }

    q.get_result("select cid,mapid,reset_time from char_elite_map_tempo where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int cid = q.getval();
        int mapid = q.getval();
        boost::shared_ptr<m_char_map_eliteCombat> pec;
        std::map<int, boost::shared_ptr<m_char_map_eliteCombat> >::iterator it =  m_char_eliteCombats.find(cid);
        if (it != m_char_eliteCombats.end() && it->second.get())
        {
            pec = it->second;
        }
        else
        {
            pec.reset(new m_char_map_eliteCombat);
            m_char_eliteCombats[cid] = pec;
        }
        boost::shared_ptr<CharMapEliteCombatData> pcmec;
        m_char_map_eliteCombat::iterator it_cm =  pec->find(mapid);
        if (it_cm != pec->end() && it_cm->second.get())
        {
            pcmec = it_cm->second;
        }
        else
        {
            pcmec.reset(new CharMapEliteCombatData);
            (*pec)[mapid] = pcmec;
        }
        pcmec->m_mapid = mapid;
        pcmec->m_cid = cid;
        pcmec->m_reset_time = q.getval();
        //加载地图所有精英信息
        pcmec->load();
    }
    q.free_result();
    return 0;
}

void eliteCombatMgr::reset()
{
    std::map<int, boost::shared_ptr<m_char_map_eliteCombat> >::iterator it =  m_char_eliteCombats.begin();
    while (it != m_char_eliteCombats.end())
    {
        if (it->second.get())
        {
            m_char_map_eliteCombat::iterator it_cm =  it->second->begin();
            while (it_cm != it->second->end() && it_cm->second.get())
            {
                ResetEliteCombat(it_cm->second->m_cid,it_cm->second->m_mapid,true);
                ++it_cm;
            }
        }
        ++it;
    }
}

boost::shared_ptr<eliteCombat> eliteCombatMgr::getEliteCombat(int mapid, int eliteid)
{
#if 0
    boost::shared_ptr<m_eliteCombat_list> pec = getEliteCombats(mapid);
    if (pec.get())
    {
        m_eliteCombat_list::iterator it = pec->begin();
        while (it != pec->end())
        {
            if (it->get() && (*it)->_id == eliteid)
            {
                return (*it);
            }
            ++it;
        }
    }
    boost::shared_ptr<eliteCombat> p;
    p.reset();
    return p;
#else
    (void)mapid;
    return getEliteCombatById(eliteid);
#endif   
}

boost::shared_ptr<m_eliteCombat_list> eliteCombatMgr::getEliteCombats(int mapid)
{
    std::map<int, boost::shared_ptr<m_eliteCombat_list> >::iterator it =  m_eliteCombats.find(mapid);
    if (it != m_eliteCombats.end())
    {
        return it->second;
    }
    boost::shared_ptr<m_eliteCombat_list> p;
    p.reset();
    return p;
}

boost::shared_ptr<eliteCombat> eliteCombatMgr::getEliteCombatById(int eliteid)
{
#if 0
    std::map<int, boost::shared_ptr<m_eliteCombat_list> >::iterator it =  m_eliteCombats.begin();
    while (it != m_eliteCombats.end())
    {
        if (it->second.get())
        {
            m_eliteCombat_list::iterator it_in = it->second->begin();
            while (it_in != it->second->end())
            {
                if (it_in->get() && (*it_in)->_id == eliteid)
                {
                    return (*it_in);
                }
                ++it_in;
            }
        }
        ++it;
    }
    boost::shared_ptr<eliteCombat> p;
    p.reset();
    return p;
#else
    if (eliteid >= 1 && eliteid <= m_eliteCombats2.size())
    {
        return m_eliteCombats2[eliteid-1];
    }
    boost::shared_ptr<eliteCombat> p;
    p.reset();
    return p;
#endif
}

boost::shared_ptr<CharEliteCombatData> eliteCombatMgr::getCharEliteCombat(int cid, int mapid, int eliteid)
{
    boost::shared_ptr<CharMapEliteCombatData> pcmec = getCharEliteCombats(cid, mapid);
    if (pcmec.get())
    {
        std::list<boost::shared_ptr<CharEliteCombatData> >::iterator it = pcmec->m_char_eliteCombat_list.begin();
        while (it != pcmec->m_char_eliteCombat_list.end())
        {
            if (it->get() && (*it)->m_eliteid == eliteid)
            {
                return (*it);
            }
            ++it;
        }
    }
    boost::shared_ptr<CharEliteCombatData> p;
    p.reset();
    return p;
}

boost::shared_ptr<CharMapEliteCombatData> eliteCombatMgr::getCharEliteCombats(int cid, int mapid)
{
    //cout << "eliteCombatMgr::getCharEliteCombats cid=" << cid << " mapid=" << mapid << endl;
    std::map<int, boost::shared_ptr<m_char_map_eliteCombat> >::iterator it =  m_char_eliteCombats.find(cid);
    if (it != m_char_eliteCombats.end() && it->second.get())
    {
        //cout << "get cid=" << cid << endl;
        m_char_map_eliteCombat::iterator it_cm =  it->second->find(mapid);
        if (it_cm != it->second->end() && it_cm->second.get())
        {
            //cout << "get mapid=" << mapid << endl;
            return it_cm->second;
        }
        else
        {
            //cout << "11 eliteCombatMgr::getCharEliteCombats cid=" << cid << " mapid=" << mapid << endl;
            //cout << "not get mapdata!!!init!!!" << endl;
            boost::shared_ptr<CharMapEliteCombatData> pcmec;
            pcmec.reset(new CharMapEliteCombatData);
            (*it->second)[mapid] = (pcmec);
            pcmec->m_mapid = mapid;
            pcmec->m_cid = cid;
            pcmec->m_reset_time = 0;
            pcmec->init();
            InsertSaveDb("replace into char_elite_map_tempo (cid,mapid,reset_time) values ("
                                    + LEX_CAST_STR(cid)
                                    + "," + LEX_CAST_STR(mapid)
                                    + ",0"
                                    + ")");
            return pcmec;
        }
    }
    else
    {
        //cout << "eliteCombatMgr::getCharEliteCombats cid=" << cid << " mapid=" << mapid << endl;
        //cout << "not get chardata!!!init!!!" << endl;
        //玩家没有精英数据则初始化
        boost::shared_ptr<m_char_map_eliteCombat> pec;
        pec.reset(new m_char_map_eliteCombat);
        m_char_eliteCombats[cid] = pec;
        boost::shared_ptr<CharMapEliteCombatData> pcmec;
        m_char_map_eliteCombat::iterator it_cm =  pec->find(mapid);
        if (it_cm != pec->end() && it_cm->second.get())
        {
            pcmec = it_cm->second;
        }
        else
        {
            pcmec.reset(new CharMapEliteCombatData);
            (*pec)[mapid] = (pcmec);
        }
        pcmec->m_mapid = mapid;
        pcmec->m_cid = cid;
        pcmec->m_reset_time = 0;
        pcmec->init();
        InsertSaveDb("replace into char_elite_map_tempo (cid,mapid,reset_time) values ("
                                + LEX_CAST_STR(cid)
                                + "," + LEX_CAST_STR(mapid)
                                + ",0"
                                + ")");
        return pcmec;
    }
}

//精英关卡是否全通
bool eliteCombatMgr::isCharElitePassed(int cid, int mapid)
{
    //cout << "getCharEliteList" << " cid=" << cid << ",mapid=" << mapid << endl;
    if (mapid < 3 || mapid > max_map_id)
    {
        mapid = max_map_id;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return false;
    bool next_map = true;
    boost::shared_ptr<CharMapEliteCombatData> pcmec = getCharEliteCombats(cid, mapid);
    if (pcmec.get())
    {
        int cnt = 1;
        json_spirit::Array eliteCombatList;
        std::list<boost::shared_ptr<CharEliteCombatData> >::iterator it_ecd = pcmec->m_char_eliteCombat_list.begin();
        //cout << "travel m_char_eliteCombat_list******************" << endl;
        while(it_ecd != pcmec->m_char_eliteCombat_list.end())
        {
            if (it_ecd->get() && (*it_ecd)->m_baseEliteCombat.get())
            {
                //本图有锁定关卡
                if ((*it_ecd)->m_state == elite_lock
                    || (*it_ecd)->m_result <= elite_active)
                {
                    return false;
                }
            }
            ++it_ecd;
        }
        return true;
    }
    return false;
}

int eliteCombatMgr::getCharEliteList(int cid, int mapid, json_spirit::Object& robj)
{
    //cout << "getCharEliteList" << " cid=" << cid << ",mapid=" << mapid << endl;
    if (mapid < 3)
    {
        mapid = max_map_id;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    bool next_map = true;
    boost::shared_ptr<CharMapEliteCombatData> pcmec = getCharEliteCombats(cid, mapid);
    if (pcmec.get())
    {
        int cnt = 1;
        json_spirit::Array eliteCombatList;
        std::list<boost::shared_ptr<CharEliteCombatData> >::iterator it_ecd = pcmec->m_char_eliteCombat_list.begin();
        //cout << "travel m_char_eliteCombat_list******************" << endl;
        while(it_ecd != pcmec->m_char_eliteCombat_list.end())
        {
            if (it_ecd->get() && (*it_ecd)->m_baseEliteCombat.get())
            {
                //本图第一关还锁定则显示前面地图精英战役
                if (cnt == 1 && (*it_ecd)->m_state == elite_lock)
                {
                    if (mapid == 3)
                    {
                        (*it_ecd)->m_state = elite_active;
                    }
                    else
                    {
                        if (isCharElitePassed(cid, mapid-1))
                        {
                            (*it_ecd)->m_state = elite_active;
                        }
                        else
                        {
                            --mapid;
                            return getCharEliteList(cid,mapid,robj);
                        }
                    }
                }
                //本图有锁定关卡不需要开放下图按钮
                if ((*it_ecd)->m_state == elite_lock
                    || (*it_ecd)->m_result <= elite_active)
                {
                    next_map = false;
                }
                json_spirit::Object eliteCombatObj;
                eliteCombat* elite = (*it_ecd)->m_baseEliteCombat.get();
                //cout << "push_back elite_id=" << elite->_id << endl;
                eliteCombatObj.push_back( Pair("id", elite->_id) );
                eliteCombatObj.push_back( Pair("state", (*it_ecd)->m_state) );
                eliteCombatObj.push_back( Pair("result", (*it_ecd)->m_result) );
                eliteCombatObj.push_back( Pair("can_attack", cdata->m_currentStronghold >= elite->_open_stronghold) );
                boost::shared_ptr<StrongholdData> sh = GeneralDataMgr::getInstance()->GetStrongholdData((*it_ecd)->m_baseEliteCombat->_open_stronghold);
                if (sh.get())
                {
                    eliteCombatObj.push_back( Pair("open_stronghold_level", sh->m_level) );
                    eliteCombatObj.push_back( Pair("open_stronghold_name", sh->m_name) );
                }
                json_spirit::Array InfoList;
                for (int i = 0; i < 9; ++i)
                {
                    if (elite->m_generals[i].get() && elite->m_generals[i]->m_special)//区分喽啰和武将
                    {
                        json_spirit::Object tmpObj;
                        tmpObj.push_back( Pair("spic", elite->m_generals[i]->m_spic) );
                        tmpObj.push_back( Pair("name", elite->m_generals[i]->m_name) );
                        tmpObj.push_back( Pair("color", elite->_color) );
                        InfoList.push_back(tmpObj);
                        break;
                    }
                }
                eliteCombatObj.push_back( Pair("info_list", InfoList) );
                //cout << "push_back infolist" << endl;
                //掉落信息
                if (elite->m_Item_list.size() > 0)
                {
                    json_spirit::Array list;
                    std::list<Item>::iterator it_l = elite->m_Item_list.begin();
                    while (it_l != elite->m_Item_list.end())
                    {
                        boost::shared_ptr<json_spirit::Object> p_obj;
                        ItemToObj(&(*it_l), p_obj);
                        if (p_obj.get())
                        {
                            list.push_back(*(p_obj.get()));
                        }
                        ++it_l;
                    }
                    eliteCombatObj.push_back( Pair("loot_list", list) );
                    //cout << "push_back m_loot" << endl;
                }
                eliteCombatList.push_back(eliteCombatObj);
                //cout << "travel ++" << endl;
            }
            ++cnt;
            ++it_ecd;
        }
        robj.push_back( Pair("mapid", pcmec->m_mapid) );
        robj.push_back( Pair("reset", pcmec->m_reset_time) );
        robj.push_back( Pair("eliteCombatList", eliteCombatList) );
        json_spirit::Object area;
        std::string name = "";
        std::string memo = "";
        int ret = GeneralDataMgr::getInstance()->GetMapMemo(mapid,name,memo);
        if (ret == 0)
        {
            area.push_back( Pair("area", mapid));
            area.push_back( Pair("name", name));
            area.push_back( Pair("memo", memo));
            robj.push_back( Pair("area", area));
        }
        if (mapid < cdata->m_area)
        {
            json_spirit::Object next_area;
            ret = GeneralDataMgr::getInstance()->GetMapMemo(mapid + 1,name,memo);
            if (ret == 0)
            {
                next_area.push_back( Pair("area", mapid + 1));
                next_area.push_back( Pair("name", name));
                next_area.push_back( Pair("memo", memo));
                if (!next_map)
                {
                    next_area.push_back( Pair("lock", 1));
                }
            }
            robj.push_back( Pair("nextArea", next_area));
        }
        if (mapid > 3)
        {
            json_spirit::Object pre_area;
            ret = GeneralDataMgr::getInstance()->GetMapMemo(mapid - 1,name,memo);
            if (ret == 0)
            {
                pre_area.push_back( Pair("area", mapid - 1));
                pre_area.push_back( Pair("name", name));
                pre_area.push_back( Pair("memo", memo));
            }
            robj.push_back( Pair("preArea", pre_area));
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int eliteCombatMgr::ResetEliteCombat(int cid, int mapid, bool auto_reset)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<CharMapEliteCombatData> pcmec = getCharEliteCombats(cid, mapid);
    if (pcmec.get())
    {
        if (!auto_reset)
        {
            if (pcmec->m_reset_time >= iEliteRestTimes[cdata->m_vip])
            {
                return HC_ERROR_MORE_VIP_LEVEL;
            }
            if (cdata->addGold(-iEliteRestGold) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            //金币消耗统计
            add_statistics_of_gold_cost(cdata->m_id, cdata->m_ip_address, iEliteRestGold, gold_cost_for_reset_elitecombat, cdata->m_union_id, cdata->m_server_id);
#ifdef QQ_PLAT
            gold_cost_tencent(cdata.get(),iEliteRestGold,gold_cost_for_reset_elitecombat);
#endif
            cdata->NotifyCharData();
            ++pcmec->m_reset_time;
        }
        else
        {
            pcmec->m_reset_time = 0;
        }
        std::list<boost::shared_ptr<CharEliteCombatData> >::iterator it = pcmec->m_char_eliteCombat_list.begin();
        while (it != pcmec->m_char_eliteCombat_list.end())
        {
            if (it->get() && (*it)->m_state != elite_lock)
            {
                (*it)->m_state = elite_active;
                (*it)->save();
            }
            ++it;
        }
        InsertSaveDb("replace into char_elite_map_tempo (cid,mapid,reset_time) values ("
                                + LEX_CAST_STR(cid)
                                + "," + LEX_CAST_STR(mapid)
                                + "," + LEX_CAST_STR(pcmec->m_reset_time)
                                + ")");
    }
    return HC_SUCCESS;
}

int eliteCombatMgr::AttackElite(session_ptr& psession, int cid, int mapid, int eliteid)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<CharEliteCombatData> pced = getCharEliteCombat(cid,mapid,eliteid);
    if (!pced.get())
    {
        return HC_ERROR;
    }
    //判断可攻击状态
    if (!pced->m_baseEliteCombat.get() || cdata->m_currentStronghold < pced->m_baseEliteCombat->_open_stronghold)
    {
        return HC_ERROR;
    }
    if (pced->m_state != elite_active)
    {
        return HC_ERROR;
    }
    int ret = HC_SUCCESS;
    Combat* pCombat = createEliteCombat(cid, mapid, eliteid, ret);
    if (HC_SUCCESS == ret && pCombat)
    {
        //防止玩家多次发送该接口，多次攻打精英关卡
        pced->m_state = 0;
        //立即返回战斗双方的信息
        pCombat->setCombatInfo();
        InsertCombat(pCombat);
        std::string sendMsg = "{\"cmd\":\"attack\",\"s\":200,\"getBattleList\":" + pCombat->getCombatInfoText() + "}";
        psession->send(sendMsg);
        return HC_SUCCESS_NO_RET;
    }
    return ret;
}

int eliteCombatMgr::combatResult(Combat* pCombat)    //战斗结束
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (combat_elite != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_attacker->getCharId());
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<CharEliteCombatData> pced = getCharEliteCombat(cdata->m_id,pCombat->m_mapid,pCombat->m_type_id);
    if (!pced.get() || !pced->m_baseEliteCombat.get())
    {
        return HC_ERROR;
    }
    int generals_cnt = 0;
    json_spirit::Object generals_info;
    json_spirit::Array zhen_generals;
    boost::shared_ptr<ZhenData> zdata = cdata->m_zhens.GetZhen(cdata->m_zhens.GetDefault());
    if (zdata.get())
    {
        zdata->getList2(zhen_generals);
        generals_cnt = zdata->getGeneralCounts();
        generals_info.push_back( Pair("info", zhen_generals) );
    }
    pced->m_state = elite_active;
    if (pCombat->m_state == attacker_win)
    {
        //首次击败
        bool first_kill = false;
        if (pced->m_result == elite_active)
        {
            first_kill = true;
            InsertSaveDb("update char_data set elite_id='" + LEX_CAST_STR(pced->m_eliteid) + "' where cid=" + LEX_CAST_STR(pced->m_cid));
            pCombat->m_archive_report = pced->m_baseEliteCombat->m_raiders.addRecords(
                pCombat->m_attacker->Name(),
                pCombat->m_attacker->level(),
                pCombat->m_combat_id,
                cdata->getAttack(0),
                pCombat->m_attacker->DieHp());
            //act统计
            act_to_tencent(cdata.get(),act_new_elite,pCombat->m_type_id,1,generals_cnt,json_spirit::write(generals_info, json_spirit::raw_utf8));
        }
        //修改可攻击状态
        switch (pCombat->m_result_type)
        {
            case 1:
            case 2:
                pced->m_state = elite_win_perfect;
                break;
            case 3:
                pced->m_state = elite_win_normal;
                break;
            case 4:
            case 5:
                pced->m_state = elite_win_little;
                break;
            default:
                pced->m_state = elite_win_normal;
                break;
        }
        if (pced->m_state > pced->m_result)
            pced->m_result = pced->m_state;
        pced->save();
        boost::shared_ptr<CharEliteCombatData> pced_next = getCharEliteCombat(cdata->m_id,pCombat->m_mapid,pCombat->m_type_id + 1);
        if (pced_next.get() && pced_next->m_state == elite_lock)
        {
            pced_next->m_state = elite_active;
            pced_next->save();
        }
        else
        {
            pced_next = getCharEliteCombat(cdata->m_id,pCombat->m_mapid + 1,pCombat->m_type_id + 1);
            if (pced_next.get() && pced_next->m_state == elite_lock)
            {
                pced_next->m_state = elite_active;
                pced_next->save();
            }
        }
        /***********随机获得掉落处理****************/
        lootMgr::getInstance()->getEliteCombatsLoots(pCombat->m_type_id, pCombat->m_getItems, pCombat->m_extra_chance);
        if (pCombat->m_getItems.size() == 0)
        {
            lootMgr::getInstance()->getWorldItemFall(pCombat->m_getItems);
        }
        //军粮
        Item item_sp(item_type_treasure, treasure_type_supply, pced->m_baseEliteCombat->supply, 1);
        pCombat->m_getItems.push_back(item_sp);
        //功勋
        Item item_g(item_type_treasure, treasure_type_gongxun, pced->m_baseEliteCombat->gongxun, 1);
        pCombat->m_getItems.push_back(item_g);
        //给东西
        giveLoots(cdata, pCombat, true, give_elite);

        //精英支线任务
        cdata->m_trunk_tasks.updateTask(task_elite_combat, pced->m_eliteid, 1);

        if (first_kill)
        {
            //系统公告
            std::string msg = strEliteMsg;
            str_replace(msg, "$N", MakeCharNameLink(cdata->m_name));
            str_replace(msg, "$E", pced->m_baseEliteCombat->_name);
            std::list<Item>::iterator it = pCombat->m_getItems.begin();
            while (it != pCombat->m_getItems.end())
            {
                if (it->type == item_type_treasure && it->id != treasure_type_supply && it->id != treasure_type_gongxun)
                {
                    boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(it->id);
                    if (tr.get())
                    {
                        std::string name = MakeTreasureLink(tr->name, it->id);
                        addColor(name, tr->quality);
                        str_replace(msg, "$R", name);
                    }
                    break;
                }
                ++it;
            }
            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
            //好友祝贺
            Singleton<relationMgr>::Instance().postCongradulation(cdata->m_id, CONGRATULATION_ELITE, pced->m_eliteid, 0);
        }
    }
    else if (pCombat->m_state == defender_win)
    {
        cdata->addLing(1);
        if (pced->m_result == elite_active)
        {
            //act统计
            act_to_tencent(cdata.get(),act_new_elite,pCombat->m_type_id,0,generals_cnt,json_spirit::write(generals_info, json_spirit::raw_utf8));
        }
    }
    pCombat->AppendResult(pCombat->m_result_obj);

    InsertSaveCombat(pCombat);
    return HC_SUCCESS;
}

bool eliteCombatMgr::check_stronghold_can_sweep(int cid, int mapid, int eliteid)
{
    boost::shared_ptr<CharEliteCombatData> pced = getCharEliteCombat(cid,mapid,eliteid);
    if (!pced.get())
    {
        return false;
    }
    if (pced->m_result > elite_active)
    {
        return true;
    }
    return false;
}

bool eliteCombatMgr::check_stronghold_can_attack(int cid, int mapid, int eliteid)
{
    boost::shared_ptr<CharEliteCombatData> pced = getCharEliteCombat(cid,mapid,eliteid);
    if (!pced.get())
    {
        return false;
    }
    if (pced->m_state >= elite_active)
    {
        return true;
    }
    return false;
}

int CharEliteCombatData::save()
{
    if (m_baseEliteCombat.get())
    {
        InsertSaveDb("replace into char_elite_tempo (cid,mapid,eliteid,state,result) values ("
                                + LEX_CAST_STR(m_cid)
                                + "," + LEX_CAST_STR(m_baseEliteCombat->_mapid)
                                + "," + LEX_CAST_STR(m_eliteid)
                                + "," + LEX_CAST_STR(m_state)
                                + "," + LEX_CAST_STR(m_result)
                                + ")");
    }
}

int CharMapEliteCombatData::init()
{
    boost::shared_ptr<m_eliteCombat_list> pec = eliteCombatMgr::getInstance()->getEliteCombats(m_mapid);
    if (!pec.get())
    {
        return 0;
    }
    m_eliteCombat_list::iterator it = (*pec).begin();
    while (it != (*pec).end())
    {
        boost::shared_ptr<CharEliteCombatData> pcecd = eliteCombatMgr::getInstance()->getCharEliteCombat(m_cid,m_mapid,(*it)->_id);
        if (pcecd.get())
        {
            ++it;
            continue;
        }
        pcecd.reset(new (CharEliteCombatData));
        m_char_eliteCombat_list.push_back(pcecd);
        pcecd->m_state = elite_lock;
        if ((*it)->_id == 1)
        {
            pcecd->m_state = elite_active;
        }
        pcecd->m_result = elite_active;
        pcecd->m_cid = m_cid;
        pcecd->m_eliteid = (*it)->_id;
        pcecd->m_baseEliteCombat = eliteCombatMgr::getInstance()->getEliteCombat(m_mapid,pcecd->m_eliteid);
        InsertSaveDb("replace into char_elite_tempo (cid,mapid,eliteid,state,result) values ("
                                + LEX_CAST_STR(m_cid)
                                + "," + LEX_CAST_STR(m_mapid)
                                + "," + LEX_CAST_STR(pcecd->m_eliteid)
                                + "," + LEX_CAST_STR(pcecd->m_state)
                                + "," + LEX_CAST_STR(pcecd->m_result)
                                + ")");
        ++it;
    }
    return 0;
}

int CharMapEliteCombatData::load()
{
    Query q(GetDb());
    q.get_result("select eliteid,state,result from char_elite_tempo where mapid=" + LEX_CAST_STR(m_mapid) + " and cid=" + LEX_CAST_STR(m_cid)+ " order by eliteid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int eliteid = q.getval();
        boost::shared_ptr<CharEliteCombatData> pcecd = eliteCombatMgr::getInstance()->getCharEliteCombat(m_cid,m_mapid,eliteid);
        if (!pcecd.get())
        {
            pcecd.reset(new (CharEliteCombatData));
            m_char_eliteCombat_list.push_back(pcecd);
        }
        pcecd->m_state = q.getval();
        pcecd->m_result = q.getval();
        pcecd->m_cid = m_cid;
        pcecd->m_eliteid = eliteid;
        pcecd->m_baseEliteCombat = eliteCombatMgr::getInstance()->getEliteCombat(m_mapid,eliteid);
    }
    q.free_result();

    if (m_char_eliteCombat_list.size() == 0)
    {
        init();
    }
    return 0;
}

int eliteCombat::load()
{
    Query q(GetDb());
    q.get_result("select pos,name,spic,color,stype,hp,attack,pufang,cefang,str,wisdom,special,skill from base_elite_armys_generals where mapid=" + LEX_CAST_STR(_mapid) + " and eliteid=" + LEX_CAST_STR(_id)+ " order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int pos = q.getval();
        if (pos >= 9)
        {
            pos = 9;
        }
        else if (pos < 1)
        {
            pos = 1;
        }
        boost::shared_ptr<StrongholdGeneralData> sg;
        if (!(m_generals[pos-1].get()))
        {
            sg.reset(new (StrongholdGeneralData));
            m_generals[pos-1] = sg;
        }
        else
        {
            sg = m_generals[pos-1];
        }
        sg->m_pos = pos;
        sg->m_name = q.getstr();
        sg->m_spic = q.getval();
        sg->m_color = q.getval();
        sg->m_stype = q.getval();
        sg->m_hp = q.getval();
        sg->m_attack = q.getval();
        sg->m_pufang = q.getval();
        sg->m_cefang = q.getval();
        sg->m_str = q.getval();
        sg->m_int = q.getval();
        sg->m_special = q.getval();
        sg->m_speSkill = GeneralDataMgr::getInstance()->getSpeSkill(q.getval());
        sg->m_level = _level;

        sg->m_baseSoldier = GeneralDataMgr::getInstance()->GetBaseSoldier(sg->m_stype);

        if (sg->m_special)
        {
            _spic = sg->m_spic;
        }
    }
    q.free_result();
    return 0;
}

int eliteCombat::getAttack()
{
    int hp = 0, attack = 0, pufang = 0, cefang = 0;
    for (size_t i = 0; i < 9; ++i)
    {
        json_spirit::Object generalobj;
        if (m_generals[i].get())
        {
            hp += m_generals[i]->m_hp;
            attack += m_generals[i]->m_attack;
            pufang += m_generals[i]->m_pufang;
            cefang += m_generals[i]->m_cefang;
        }
    }
    return (hp + pufang + cefang + attack * 2) * 10 / 100;
}

int ProcessGetEliteCombatList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int mapid = 0;
    READ_INT_FROM_MOBJ(mapid,o,"mapid");
    robj.push_back( Pair("can_reset", iEliteRestTimes[cdata->m_vip]) );
    return eliteCombatMgr::getInstance()->getCharEliteList(cdata->m_id, mapid, robj);
}

int ProcessAttackEliteCombat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (cdata->addLing(-1) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_LING;
    }
    //军令统计
    add_statistics_of_ling_cost(cdata->m_id,cdata->m_ip_address,1,ling_elite_combat,2, cdata->m_union_id, cdata->m_server_id);
    int mapid = 0, eliteid = 0;
    READ_UINT64_FROM_MOBJ(mapid, o, "mapid");
    READ_UINT64_FROM_MOBJ(eliteid, o, "eliteid");
    return eliteCombatMgr::getInstance()->AttackElite(psession, cdata->m_id, mapid, eliteid);
}

int ProcessResetEliteCombat(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int mapid = 0;
    READ_UINT64_FROM_MOBJ(mapid, o, "mapid");
    return eliteCombatMgr::getInstance()->ResetEliteCombat(cdata->m_id, mapid, false);
}

//显示玩家单个精英关卡信息
int ProcessEliteStronghold(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0, id = 0;
    json_spirit::Object info;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(id, o, "id");
    boost::shared_ptr<CharEliteCombatData> elite = eliteCombatMgr::getInstance()->getCharEliteCombat(pc->m_id, mapid, id);
    if (!elite.get())
    {
        return HC_ERROR;
    }
    //判断可攻击状态
    if (!elite->m_baseEliteCombat.get() || pc->m_currentStronghold < elite->m_baseEliteCombat->_open_stronghold)
    {
        return HC_ERROR;
    }
    if (elite->m_state != elite_active)
    {
        return HC_ERROR;
    }

    //关卡的信息
    info.push_back( Pair("mapid", elite->m_baseEliteCombat->_mapid));
    info.push_back( Pair("id", elite->m_baseEliteCombat->_id));
    info.push_back( Pair("name", elite->m_baseEliteCombat->_name));
    info.push_back( Pair("level", elite->m_baseEliteCombat->_level));
    info.push_back( Pair("spic", elite->m_baseEliteCombat->_spic));
    info.push_back( Pair("color", elite->m_baseEliteCombat->_color));
    info.push_back( Pair("state", elite->m_state));
    info.push_back( Pair("result", elite->m_result));

    //掉落信息
    if (elite->m_baseEliteCombat->m_Item_list.size() > 0)
    {
        json_spirit::Array list;
        std::list<Item>::iterator it_l = elite->m_baseEliteCombat->m_Item_list.begin();
        while (it_l != elite->m_baseEliteCombat->m_Item_list.end())
        {
            boost::shared_ptr<json_spirit::Object> p_obj;
            ItemToObj(&(*it_l), p_obj);
            if (p_obj.get())
            {
                list.push_back(*(p_obj.get()));
            }
            ++it_l;
        }
        info.push_back( Pair("loot_list", list) );
    }

    //关卡武将信息
    json_spirit::Array generallist;
    for (size_t i = 0; i < 9; ++i)
    {
        json_spirit::Object generalobj;
        if (elite->m_baseEliteCombat->m_generals[i].get())
        {
            generalobj.push_back( Pair("spic", elite->m_baseEliteCombat->m_generals[i]->m_spic));
            generalobj.push_back( Pair("color", elite->m_baseEliteCombat->m_generals[i]->m_color));
            generalobj.push_back( Pair("name", elite->m_baseEliteCombat->m_generals[i]->m_name));
            generalobj.push_back( Pair("level", elite->m_baseEliteCombat->_level));

            if (elite->m_baseEliteCombat->m_generals[i]->m_speSkill.get())
            {
                generalobj.push_back( Pair("spe_name",  elite->m_baseEliteCombat->m_generals[i]->m_speSkill->name));
                generalobj.push_back( Pair("spe_memo",  elite->m_baseEliteCombat->m_generals[i]->m_speSkill->memo));
            }
            json_spirit::Object soldierobj;
            soldierobj.push_back( Pair("type", elite->m_baseEliteCombat->m_generals[i]->m_baseSoldier->m_base_type));
            soldierobj.push_back( Pair("attack_type", elite->m_baseEliteCombat->m_generals[i]->m_baseSoldier->m_damage_type));
            soldierobj.push_back( Pair("name", elite->m_baseEliteCombat->m_generals[i]->m_baseSoldier->m_name));
            soldierobj.push_back( Pair("memo", elite->m_baseEliteCombat->m_generals[i]->m_baseSoldier->m_desc));
            generalobj.push_back( Pair("soldier", soldierobj));

            //擅长
            generalobj.push_back( Pair("good_at", 0) );

            //攻击范围...
            if (elite->m_baseEliteCombat->m_generals[i]->m_speSkill.get() && elite->m_baseEliteCombat->m_generals[i]->m_speSkill->action_type == 1)
            {
                int atype = elite->m_baseEliteCombat->m_generals[i]->m_speSkill->attack_type;
                int pos[9];
                memset(pos, 0, sizeof(int)*9);
                boost::shared_ptr<ZhenData> zdata = pc->m_zhens.GetZhen(pc->m_zhens.m_default_zhen);
                if (zdata.get())
                {
                    for (int i = 0; i < 9; ++i)
                    {
                        if (zdata->m_generals[i] > 0)
                        {
                            pos[i] = 1;
                        }
                    }
                }
                json_spirit::Array v = getAttackRange(atype, i+1, pos);
                generalobj.push_back( Pair("range", v) );
            }
            
            generalobj.push_back( Pair("position", elite->m_baseEliteCombat->m_generals[i]->m_pos));
        }
        else
        {
            generalobj.push_back( Pair("position", -2));
        }
        generallist.push_back(generalobj);
    }
    info.push_back( Pair("gate_formation", generallist));

    info.push_back( Pair("attack", elite->m_baseEliteCombat->getAttack()));

    robj.push_back( Pair("info", info));

    return HC_SUCCESS;
}

//显示关卡攻略
int ProcessEliteRaiders(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    boost::shared_ptr<eliteCombat> elite = eliteCombatMgr::getInstance()->getEliteCombatById(id);
    if (elite.get())
    {
        elite->m_raiders.getRadiers(robj);        
    }
    return HC_SUCCESS;
}

