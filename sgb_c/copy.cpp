
#include "copy.h"

#include "errcode_def.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_lang.h"
#include "net.h"
#include "spls_timer.h"
#include "singleton.h"
#include "sweep.h"
#include "relation.h"
#include "weekRanking.h"

#define INFO(x)// cout<<x

extern void InsertSaveDb(const std::string& sql);
extern Database& GetDb();

int ProcessGetCopyMapList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<copyMgr>::Instance().getCharCopyMapList(pc->m_id, robj);
}

int ProcessGetCopyList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0;
    READ_INT_FROM_MOBJ(mapid,o,"mapid");
    return Singleton<copyMgr>::Instance().getCharCopyList(pc->m_id, mapid, robj);
}

int ProcessGetCopyInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0, copyid = 0;
    json_spirit::Object info;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(copyid, o, "copyid");
    robj.push_back( Pair("mapid", mapid) );
    robj.push_back( Pair("copyid", copyid) );
    json_spirit::Object copyObj;
    boost::shared_ptr<CharCopyData> p_cd = Singleton<copyMgr>::Instance().getCharCopy(pc->m_id, mapid, copyid);
    if (p_cd.get() && p_cd->m_baseCopy.get())
    {
        baseCopy* copy = p_cd->m_baseCopy.get();
        copyObj.push_back( Pair("id", copy->m_id) );
        copyObj.push_back( Pair("name", copy->m_name) );
        copyObj.push_back( Pair("spic", copy->m_spic) );
        copyObj.push_back( Pair("can_attack", p_cd->m_can_attack) );
        copyObj.push_back( Pair("can_attack_max", p_cd->m_can_attack_max) );
        copyObj.push_back( Pair("result", p_cd->m_result) );
        copyObj.push_back( Pair("need_level", copy->m_openLevel) );
        if (copy->m_hero.get())
        {
            json_spirit::Object tmpObj;
            copy->m_hero->toObj(tmpObj);
            copyObj.push_back( Pair("hero_info", tmpObj) );
        }
        //掉落信息
        if (copy->m_Item_list.size() > 0)
        {
            json_spirit::Array list;
            std::list<Item>::iterator it_l = copy->m_Item_list.begin();
            while (it_l != copy->m_Item_list.end())
            {
                boost::shared_ptr<json_spirit::Object> p_obj;
                (*it_l).toPointObj(p_obj);
                if (p_obj.get())
                {
                    list.push_back(*(p_obj.get()));
                }
                ++it_l;
            }
            copyObj.push_back( Pair("get_list", list) );
        }
        int has_buy = pc->queryExtraData(char_data_type_daily,char_data_daily_buy_copy);
        copyObj.push_back( Pair("can_add_time", iCopyCanBuy[pc->m_vip] - has_buy) );
        copyObj.push_back( Pair("add_cost", iCopyBuyCost) );
        copyObj.push_back( Pair("reset_cost", iCopyResetCost) );
        robj.push_back( Pair("copyObj", copyObj) );
        int has_attack = pc->queryExtraData(char_data_type_daily,char_data_daily_attack_copy);
        robj.push_back( Pair("attack_time", iCopyTotal - has_attack) );
        robj.push_back( Pair("attack_time_max", iCopyTotal) );
    }
    return HC_SUCCESS;
}

int ProcessResetCopy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0, copyid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(copyid, o, "copyid");
    return Singleton<copyMgr>::Instance().ResetCopy(pc->m_id, mapid, copyid);
}

int ProcessAddCopyTimes(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0, copyid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(copyid, o, "copyid");
    return Singleton<copyMgr>::Instance().AddCopyTimes(pc->m_id, mapid, copyid);
}

int ProcessQueryCopyShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    robj.push_back( Pair("coin", pc->m_bag.getGemCount(GEM_ID_COPY_COIN)) );
    return Singleton<copyMgr>::Instance().getCopyShop(pc->m_id, o, robj);
}

int ProcessBuyCopyShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    boost::shared_ptr<baseCopyGoods> bs = Singleton<copyMgr>::Instance().GetBaseCopyGoods(id);
    if (bs.get())
    {
        if (bs->need_copy_map > 1)
        {
            if (!Singleton<copyMgr>::Instance().isCharMapCopyPassed(cdata->m_id, bs->need_copy_map - 1))
            {
                return HC_ERROR;
            }
        }
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
        if (cdata->subGem(GEM_ID_COPY_COIN, cost, gem_cost_copy_add) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GEM;
        }
        giveLoots(cdata.get(),items,NULL,&robj,true, loot_copy_shop);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int ProcessGetCopyFinishReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0, type = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("mapid", mapid) );
    robj.push_back( Pair("type", type) );
    return Singleton<copyMgr>::Instance().GetCopyFinishReward(pc->m_id, mapid, type, robj);
}

void baseCopyMap::load()
{
    Query q(GetDb());
    //加载通关副本地图宝箱
    q.get_result("select itemType,itemId,counts,extra from base_copy_map_finish_loots where type = 1 and copy_mapid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();
        item.extra = q.getval();
        m_finish_reward.push_back(item);
    }
    q.free_result();

    //加载完美通关副本地图宝箱
    q.get_result("select itemType,itemId,counts,extra from base_copy_map_finish_loots where type = 2 and copy_mapid=" + LEX_CAST_STR(m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();
        item.extra = q.getval();
        m_perfect_reward.push_back(item);
    }
    q.free_result();

}

void CharCopyData::reset()
{
    if (m_baseCopy.get())
    {
        m_can_attack = iCopyEvery;
        m_can_attack_max = iCopyEvery;
        save();
    }
}

void CharCopyData::save()
{
    if (m_baseCopy.get())
    {
        InsertSaveDb("replace into char_copy_tempo (cid,mapid,copyid,can_attack,can_attack_max,result) values ("
                                + LEX_CAST_STR(m_cid)
                                + "," + LEX_CAST_STR(m_baseCopy->m_mapid)
                                + "," + LEX_CAST_STR(m_copyid)
                                + "," + LEX_CAST_STR(m_can_attack)
                                + "," + LEX_CAST_STR(m_can_attack_max)
                                + "," + LEX_CAST_STR(m_result)
                                + ")");
    }
}

int CharMapCopyData::load()
{
    int mapcopy_tempo = 0;
    Query q(GetDb());
    q.get_result("select copyid,can_attack,can_attack_max,result from char_copy_tempo where mapid=" + LEX_CAST_STR(m_mapid) + " and cid=" + LEX_CAST_STR(m_cid)+ " order by copyid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int copyid = q.getval();
        boost::shared_ptr<CharCopyData> pcd = Singleton<copyMgr>::Instance().getCharCopy(m_cid,m_mapid,copyid);
        if (!pcd.get())
        {
            pcd.reset(new (CharCopyData));
            m_char_copy_list.push_back(pcd);
        }
        pcd->m_can_attack = q.getval();
        pcd->m_can_attack_max = q.getval();
        pcd->m_result = q.getval();
        if (pcd->m_result > 0)
            mapcopy_tempo = copyid;
        pcd->m_cid = m_cid;
        pcd->m_copyid = copyid;
        pcd->m_baseCopy = Singleton<copyMgr>::Instance().getCopyById(copyid);
    }
    q.free_result();

    //副本进度
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (cdata.get())
    {
        int tmp = cdata->queryExtraData(char_data_type_normal, char_data_normal_copy);
        if (tmp < mapcopy_tempo)
        {
            cdata->setExtraData(char_data_type_normal, char_data_normal_copy, mapcopy_tempo);
        }
    }

    if (m_char_copy_list.size() == 0)
    {
        Singleton<copyMgr>::Instance().initCharMapCopy(m_cid,m_mapid);
    }
    return 0;
}

void CharMapCopyData::checkFinish()
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
        return;
    bool perfect = true;
    std::list<boost::shared_ptr<CharCopyData> >::iterator it_ecd = m_char_copy_list.begin();
    while(it_ecd != m_char_copy_list.end())
    {
        if (it_ecd->get() && (*it_ecd)->m_baseCopy.get())
        {
            //本图有锁定关卡
            if ((*it_ecd)->m_result == 0)
            {
                return;
            }
            if ((*it_ecd)->m_result != 3)
            {
                perfect = false;
            }
        }
        ++it_ecd;
    }
    int idx = char_data_normal_copy_reward_start + m_mapid;
    int state = cdata->queryExtraData(char_data_type_normal, idx);
    if (state == 0)
    {
        cdata->setExtraData(char_data_type_normal, idx, 1);
    }

    idx = char_data_normal_copy_perfect_reward_start + m_mapid;
    state = cdata->queryExtraData(char_data_type_normal, idx);
    if (state == 0 && perfect)
    {
        cdata->setExtraData(char_data_type_normal, idx, 1);
    }
}

copyMgr::copyMgr()
{
    Query q(GetDb());
    q.get_result("select mapid,name from base_copy_maps where 1 order by mapid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        boost::shared_ptr<baseCopyMap> c;
        c.reset(new baseCopyMap);
        c->m_id = mapid;
        c->m_name = q.getstr();
        c->load();
        m_copy_maps.push_back(c);
        assert(m_copy_maps.size() == mapid);

        m_max_mapid = mapid;
    }
    q.free_result();
    q.get_result("select mapid,copyid,openlevel,level,name,spic,silver,chat,race,star,attack,defense,magic,hp,skill1,skill2,skill3 from base_copys where 1 order by copyid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        int copyid = q.getval();
        boost::shared_ptr<baseCopy> c;
        c.reset(new baseCopy);
        c->m_mapid = mapid;
        c->m_id = copyid;
        c->m_openLevel = q.getval();
        c->m_level = q.getval();
        c->m_name = q.getstr();
        c->m_spic = q.getval();
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
        sh->m_name = c->m_name;
        sh->m_spic = c->m_spic;
        sh->m_race = q.getval();
        sh->m_star = q.getval();
        sh->m_quality = sh->m_star;
        sh->m_level = c->m_level;
        sh->m_attack = q.getval();
        sh->m_defense = q.getval();
        sh->m_magic = q.getval();
        sh->m_hp = q.getval();
        int skill1 = q.getval();
        int skill2 = q.getval();
        int skill3 = q.getval();
        //sh->m_speSkill = getSpeSkill(q.getval());
        //掉落加载
        Singleton<lootMgr>::Instance().getCopyLootInfo(c->m_id, c->m_Item_list);

        m_copys.push_back(c);
        assert(m_copys.size() == copyid);
    }
    q.free_result();
    //基础副本商品
    q.get_result("select id,cost,need_copy_map,itemType,itemId,counts,extra from base_copy_shop where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<baseCopyGoods> bpa = m_base_copy_goods[id];
        if (!bpa.get())
        {
            bpa.reset(new baseCopyGoods);
            m_base_copy_goods[id] = bpa;
        }
        bpa->id = id;
        bpa->cost = q.getval();
        bpa->need_copy_map = q.getval();
        bpa->m_item.type = q.getval();
        bpa->m_item.id = q.getval();
        bpa->m_item.nums = q.getval();
        bpa->m_item.extra = q.getval();
    }
    q.free_result();
}

void copyMgr::dailyUpdate()
{
    std::map<int, boost::shared_ptr<m_char_map_copy> >::iterator it =  m_char_copys.begin();
    while (it != m_char_copys.end())
    {
        if (it->second.get())
        {
            m_char_map_copy::iterator it_cm =  it->second->begin();
            while (it_cm != it->second->end() && it_cm->second.get())
            {
                boost::shared_ptr<CharMapCopyData> pcmcd = it_cm->second;
                if (pcmcd.get())
                {
                    std::list<boost::shared_ptr<CharCopyData> >::iterator it_cd = pcmcd->m_char_copy_list.begin();
                    while (it_cd != pcmcd->m_char_copy_list.end())
                    {
                        if (it_cd->get() && (*it_cd)->m_can_attack > -1)
                        {
                            (*it_cd)->m_can_attack = iCopyEvery;
                            (*it_cd)->m_can_attack_max = iCopyEvery;
                        }
                        ++it_cd;
                    }
                }
                ++it_cm;
            }
        }
        ++it;
    }
    InsertSaveDb("update char_copy_tempo set can_attack=" + LEX_CAST_STR(iCopyEvery)
                            + ",can_attack_max=" + LEX_CAST_STR(iCopyEvery)
                            + " where can_attack>-1");
}

boost::shared_ptr<baseCopy> copyMgr::getCopyById(int copyid)
{
    if (copyid >= 1 && copyid <= m_copys.size())
    {
        return m_copys[copyid-1];
    }
    boost::shared_ptr<baseCopy> p;
    p.reset();
    return p;
}

boost::shared_ptr<CharCopyData> copyMgr::getCharCopy(int cid, int mapid, int copyid)
{
    boost::shared_ptr<CharMapCopyData> pcmcd = getCharCopys(cid, mapid);
    if (pcmcd.get())
    {
        std::list<boost::shared_ptr<CharCopyData> >::iterator it = pcmcd->m_char_copy_list.begin();
        while (it != pcmcd->m_char_copy_list.end())
        {
            if (it->get() && (*it)->m_copyid == copyid)
            {
                return (*it);
            }
            ++it;
        }
    }
    boost::shared_ptr<CharCopyData> p;
    p.reset();
    return p;
}

boost::shared_ptr<CharMapCopyData> copyMgr::getCharCopys(int cid, int mapid)
{
    //cout << "copyMgr::getCharCopys cid=" << cid << " mapid=" << mapid << endl;
    std::map<int, boost::shared_ptr<m_char_map_copy> >::iterator it =  m_char_copys.find(cid);
    if (it != m_char_copys.end() && it->second.get())
    {
        //cout << "get cid=" << cid << endl;
        m_char_map_copy::iterator it_cm =  it->second->find(mapid);
        if (it_cm != it->second->end() && it_cm->second.get())
        {
            //cout << "get mapid=" << mapid << endl;
            return it_cm->second;
        }
        else
        {
            //cout << "11 copyMgr::getCharCopys cid=" << cid << " mapid=" << mapid << endl;
            //cout << "not get mapdata!!!init!!!" << endl;
            boost::shared_ptr<CharMapCopyData> pcmcd;
            pcmcd.reset(new CharMapCopyData);
            (*it->second)[mapid] = (pcmcd);
            pcmcd->m_mapid = mapid;
            pcmcd->m_cid = cid;
            initCharMapCopy(cid,mapid);
            InsertSaveDb("replace into char_copy_map_tempo (cid,mapid) values ("
                                    + LEX_CAST_STR(cid)
                                    + "," + LEX_CAST_STR(mapid)
                                    + ")");
            return pcmcd;
        }
    }
    else
    {
        //没有玩家数据则先从数据库中加载
        Query q(GetDb());
        q.get_result("select cid,mapid from char_copy_map_tempo where 1");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int cid = q.getval();
            int mapid = q.getval();
            boost::shared_ptr<m_char_map_copy> pcmc;
            std::map<int, boost::shared_ptr<m_char_map_copy> >::iterator it =  m_char_copys.find(cid);
            if (it != m_char_copys.end() && it->second.get())
            {
                pcmc = it->second;
            }
            else
            {
                pcmc.reset(new m_char_map_copy);
                m_char_copys[cid] = pcmc;
            }
            boost::shared_ptr<CharMapCopyData> pcmcd;
            m_char_map_copy::iterator it_cm =  pcmc->find(mapid);
            if (it_cm != pcmc->end() && it_cm->second.get())
            {
                pcmcd = it_cm->second;
            }
            else
            {
                pcmcd.reset(new CharMapCopyData);
                (*pcmc)[mapid] = pcmcd;
            }
            pcmcd->m_mapid = mapid;
            pcmcd->m_cid = cid;
            //加载地图所有精英信息
            pcmcd->load();
        }
        q.free_result();
        //上述初始化后再获取
        it =  m_char_copys.find(cid);
        if (it != m_char_copys.end() && it->second.get())
        {
            return getCharCopys(cid, mapid);
        }
        else
        {
            //cout << "22 copyMgr::getCharCopys cid=" << cid << " mapid=" << mapid << endl;
            //cout << "not get chardata!!!init!!!" << endl;
            //玩家没有精英数据则初始化
            boost::shared_ptr<m_char_map_copy> pcm;
            pcm.reset(new m_char_map_copy);
            m_char_copys[cid] = pcm;
            boost::shared_ptr<CharMapCopyData> pcmcd;
            m_char_map_copy::iterator it_cm =  pcm->find(mapid);
            if (it_cm != pcm->end() && it_cm->second.get())
            {
                pcmcd = it_cm->second;
            }
            else
            {
                pcmcd.reset(new CharMapCopyData);
                (*pcm)[mapid] = (pcmcd);
            }
            pcmcd->m_mapid = mapid;
            pcmcd->m_cid = cid;
            initCharMapCopy(cid,mapid);
            InsertSaveDb("replace into char_copy_map_tempo (cid,mapid) values ("
                                    + LEX_CAST_STR(cid)
                                    + "," + LEX_CAST_STR(mapid)
                                    + ")");
            return pcmcd;
        }
    }
}

int copyMgr::getCharCopyMapList(int cid, json_spirit::Object& robj)
{
    cout << "getCharCopyMapList" << " cid=" << cid << endl;
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    int cur_mapid = getCharCurMap(cid);
    json_spirit::Array mapList;
    for (int i = 0; i < m_copy_maps.size(); ++i)
    {
        json_spirit::Object copyMapObj;
        if (m_copy_maps[i].get())
        {
            copyMapObj.push_back( Pair("id", m_copy_maps[i]->m_id) );
            copyMapObj.push_back( Pair("name", m_copy_maps[i]->m_name) );
            copyMapObj.push_back( Pair("passed", m_copy_maps[i]->m_id < cur_mapid ? 1 : 0) );
            mapList.push_back(copyMapObj);
        }
    }
    robj.push_back( Pair("mapList", mapList) );
    boost::shared_ptr<charSweep> pcs = Singleton<sweepMgr>::Instance().getCharSweepData(cdata->m_id);
    if (pcs.get())
    {
        if (pcs->m_sweep_id > 0 && pcs->m_end_time > 0)
        {
            boost::shared_ptr<baseCopy> bc = Singleton<copyMgr>::Instance().getCopyById(pcs->m_sweep_id);
            if (bc.get())
            {
                json_spirit::Object info;
                info.push_back( Pair("mapid", bc->m_mapid));
                info.push_back( Pair("copyid", bc->m_id));
                int lefttime = pcs->m_end_time - time(NULL);
                info.push_back( Pair("leftTime", lefttime < 0 ? 0 : lefttime));
                robj.push_back( Pair("sweep_info", info));
            }
        }
    }
    return HC_SUCCESS;
}

int copyMgr::getCharCopyList(int cid, int mapid, json_spirit::Object& robj)
{
    //cout << "getCharCopyList" << " cid=" << cid << ",mapid=" << mapid << endl;
    if (mapid < 1 || mapid > m_max_mapid)
    {
        mapid = m_max_mapid;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    bool next_map = true;
    boost::shared_ptr<CharMapCopyData> pcmcd = getCharCopys(cid, mapid);
    if (pcmcd.get())
    {
        int cnt = 1;
        json_spirit::Array copyList;
        std::list<boost::shared_ptr<CharCopyData> >::iterator it_ecd = pcmcd->m_char_copy_list.begin();
        //cout << "travel m_char_copy_list******************" << endl;
        while(it_ecd != pcmcd->m_char_copy_list.end())
        {
            if (it_ecd->get() && (*it_ecd)->m_baseCopy.get())
            {
                //本图第一关还锁定则显示前面地图精英战役
                if (cnt == 1 && (*it_ecd)->m_can_attack == -1)
                {
                    if (mapid == 1)
                    {
                        (*it_ecd)->m_can_attack = iCopyEvery;
                        (*it_ecd)->m_can_attack_max = iCopyEvery;
                    }
                    else
                    {
                        if (isCharMapCopyPassed(cid, mapid-1))
                        {
                            (*it_ecd)->m_can_attack = iCopyEvery;
                            (*it_ecd)->m_can_attack_max = iCopyEvery;
                        }
                        else
                        {
                            --mapid;
                            return getCharCopyList(cid,mapid,robj);
                        }
                    }
                }
                //本图有锁定关卡不需要开放下图按钮
                if ((*it_ecd)->m_result == 0)
                {
                    next_map = false;
                }
                json_spirit::Object copyObj;
                baseCopy* copy = (*it_ecd)->m_baseCopy.get();
                //cout << "push_back copy_id=" << copy->m_id << endl;
                copyObj.push_back( Pair("id", copy->m_id) );
                copyObj.push_back( Pair("name", copy->m_name) );
                copyObj.push_back( Pair("spic", copy->m_spic) );
                copyObj.push_back( Pair("result", (*it_ecd)->m_result) );
                copyObj.push_back( Pair("need_level", copy->m_openLevel) );
                copyList.push_back(copyObj);
                //cout << "travel ++" << endl;
            }
            ++cnt;
            ++it_ecd;
        }
        robj.push_back( Pair("mapid", pcmcd->m_mapid) );
        robj.push_back( Pair("copyList", copyList) );
        //通关奖励
        if (pcmcd->m_mapid < m_max_mapid)
        {
            int idx = char_data_normal_copy_reward_start + pcmcd->m_mapid;
            int state = cdata->queryExtraData(char_data_type_normal, idx);
            json_spirit::Object copyFinishObj;
            if (m_copy_maps[pcmcd->m_mapid-1].get())
            {
                json_spirit::Array finish_get;
                itemlistToArray(m_copy_maps[pcmcd->m_mapid-1]->m_finish_reward,finish_get);
                copyFinishObj.push_back( Pair("get", finish_get) );
            }
            copyFinishObj.push_back( Pair("state", state) );
            robj.push_back( Pair("copyFinishObj", copyFinishObj) );

            idx = char_data_normal_copy_perfect_reward_start + pcmcd->m_mapid;
            state = cdata->queryExtraData(char_data_type_normal, idx);
            json_spirit::Object copyFinishPerfectObj;
            if (m_copy_maps[pcmcd->m_mapid-1].get())
            {
                json_spirit::Array finish_perfect_get;
                itemlistToArray(m_copy_maps[pcmcd->m_mapid-1]->m_perfect_reward,finish_perfect_get);
                copyFinishPerfectObj.push_back( Pair("get", finish_perfect_get) );
            }
            copyFinishPerfectObj.push_back( Pair("state", state) );
            robj.push_back( Pair("copyFinishPerfectObj", copyFinishPerfectObj) );
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//精英关卡是否全通
bool copyMgr::isCharMapCopyPassed(int cid, int mapid)
{
    if (mapid < 1 || mapid > m_max_mapid)
    {
        mapid = m_max_mapid;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return false;
    bool next_map = true;
    boost::shared_ptr<CharMapCopyData> pcmcd = getCharCopys(cid, mapid);
    if (pcmcd.get())
    {
        std::list<boost::shared_ptr<CharCopyData> >::iterator it_ecd = pcmcd->m_char_copy_list.begin();
        //cout << "travel m_char_copy_list******************" << endl;
        while(it_ecd != pcmcd->m_char_copy_list.end())
        {
            if (it_ecd->get() && (*it_ecd)->m_baseCopy.get())
            {
                //本图有锁定关卡
                if ((*it_ecd)->m_result == 0)
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

int copyMgr::getCharCurMap(int cid)
{
    int cur_mapid = 1;
    for (int mapid = 1; mapid <= m_max_mapid; ++mapid)
    {
        if (isCharMapCopyPassed(cid,mapid))
        {
            ++cur_mapid;
        }
        else
        {
            break;
        }
    }
    return cur_mapid;
}

int copyMgr::initCharMapCopy(int cid, int mapid)
{
    if (mapid < 1 || mapid > m_max_mapid)
    {
        mapid = m_max_mapid;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return false;
    bool next_map = true;
    boost::shared_ptr<CharMapCopyData> pcmcd = getCharCopys(cid, mapid);
    if (pcmcd.get())
    {
        for (int i = 0; i < m_copys.size(); ++i)
        {
            if (m_copys[i].get() && m_copys[i]->m_mapid == mapid)
            {
                boost::shared_ptr<CharCopyData> pcd = getCharCopy(cid,mapid,m_copys[i]->m_id);
                if (pcd.get())
                {
                    continue;
                }
                pcd.reset(new (CharCopyData));
                pcmcd->m_char_copy_list.push_back(pcd);
                pcd->m_can_attack = -1;
                pcd->m_can_attack_max = -1;
                if (m_copys[i]->m_id == 1)
                {
                    pcd->m_can_attack = iCopyEvery;
                    pcd->m_can_attack_max = iCopyEvery;
                }
                pcd->m_result = 0;
                pcd->m_cid = cid;
                pcd->m_copyid = m_copys[i]->m_id;
                pcd->m_baseCopy = getCopyById(pcd->m_copyid);
                pcd->save();
            }
        }
    }
}

int copyMgr::ResetCopy(int cid, int mapid, int copyid)
{
    if (mapid < 1 || mapid > m_max_mapid)
    {
        mapid = m_max_mapid;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<CharCopyData> pcd = getCharCopy(cid,mapid,copyid);
    if (pcd.get())
    {
        //可攻击
        if (pcd->m_can_attack > -1 && pcd->m_can_attack_max > -1)
        {
            #if 0
            int has_reset = cdata->queryExtraData(char_data_type_daily,char_data_daily_reset_copy);
            if (has_reset>= iCopyCanReset[cdata->m_vip])
            {
                return HC_ERROR_NEED_MORE_VIP;
            }
            #endif
            if (cdata->subGold(iCopyResetCost, gold_cost_copy_reset) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            pcd->reset();
            //cdata->setExtraData(char_data_type_daily,char_data_daily_reset_copy, ++has_reset);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int copyMgr::AddCopyTimes(int cid, int mapid, int copyid)
{
    if (mapid < 1 || mapid > m_max_mapid)
    {
        mapid = m_max_mapid;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<CharCopyData> pcd = getCharCopy(cid,mapid,copyid);
    if (pcd.get())
    {
        //可攻击
        if (pcd->m_can_attack > -1 && pcd->m_can_attack_max > -1)
        {
            int has_buy = cdata->queryExtraData(char_data_type_daily,char_data_daily_buy_copy);
            if (has_buy >= iCopyCanBuy[cdata->m_vip])
            {
                return HC_ERROR_NEED_MORE_VIP;
            }
            if (cdata->subGold(iCopyBuyCost, gold_cost_copy_add) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            pcd->m_can_attack += 1;
            pcd->m_can_attack_max += 1;
            pcd->save();
            cdata->setExtraData(char_data_type_daily,char_data_daily_buy_copy, ++has_buy);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int copyMgr::combatResult(chessCombat* pCombat)    //战斗结束
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (COMBAT_TYPE_COPY != pCombat->m_type)
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
    boost::shared_ptr<baseCopy> bc = getCopyById(pCombat->m_data_id);
    if (!bc.get())
    {
        ERR();
        return HC_ERROR;
    }
    CharCopyData* pcd = getCharCopy(pCombat->m_players[0].m_cid, bc->m_mapid, bc->m_id).get();
    if (!pcd)
    {
        ERR();
        return HC_ERROR;
    }
    cout << "copyMgr::combatResult   " << pCombat->m_combat_id << ",reuslt=" << pCombat->m_result << endl;
    int has_attack = cdata->queryExtraData(char_data_type_daily,char_data_daily_attack_copy);
    cdata->setExtraData(char_data_type_daily,char_data_daily_attack_copy, ++has_attack);
    --pcd->m_can_attack;
    cdata->m_score_tasks.updateTask(DAILY_SCORE_COPY_ATTACK);
    if (pCombat->m_result == COMBAT_RESULT_ATTACK_WIN)
    {
        if (pcd->m_result == 0)
        {
            //广播好友祝贺
            Singleton<relationMgr>::Instance().postCongradulation(cdata->m_id, CONGRATULATION_COPY, bc->m_id, 0);
            //周排行活动
            int score = bc->m_id;
            if (score > 0)
                Singleton<weekRankings>::Instance().updateEventRankings(cdata->m_id,week_ranking_copy,score);
            //第一次通关开放后续
            boost::shared_ptr<CharCopyData> pced_next = getCharCopy(pCombat->m_players[0].m_cid,bc->m_mapid,bc->m_id + 1);
            if (pced_next.get())
            {
                //本图下一关卡存在
                if (pced_next->m_can_attack == -1)
                {
                    pced_next->m_can_attack = iCopyEvery;
                    pced_next->m_can_attack_max = iCopyEvery;
                    pced_next->save();
                }
            }
            else
            {
                //本图结束查看下一图
                pced_next = getCharCopy(pCombat->m_players[0].m_cid,bc->m_mapid+1,bc->m_id + 1);
                std::string msg = strSystemPassCopyMap;//广播
                if (pced_next.get())
                {
                    if (pced_next->m_can_attack == -1)
                    {
                        pced_next->m_can_attack = iCopyEvery;
                        pced_next->m_can_attack_max = iCopyEvery;
                        pced_next->save();
                    }
                    Singleton<goalMgr>::Instance().updateTask(cdata->m_id, GOAL_TYPE_COPY_MAP, bc->m_mapid);
                    msg = strSystemPassCopyMap2;
                    str_replace(msg, "$N", m_copy_maps[bc->m_mapid]->m_name);
                }
                str_replace(msg, "$W", MakeCharNameLink(cdata->m_name,cdata->m_nick.get_string()));
                str_replace(msg, "$T", m_copy_maps[bc->m_mapid-1]->m_name);
                GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
            }
        }
        if (pcd->m_result < pCombat->m_result_type)
        {
            pcd->m_result = pCombat->m_result_type;
            if (pcd->m_result == 3)
            {
                //广播好友祝贺
                Singleton<relationMgr>::Instance().postCongradulation(cdata->m_id, CONGRATULATION_COPY_PERFECT, bc->m_id, 0);
            }
        }
        //根据结果决定掉落数量(翻牌展现)
        /***********随机获得掉落处理****************/
        Singleton<lootMgr>::Instance().getCopyLoots(pCombat->m_data_id, pCombat->m_getItems, pCombat->m_result_type);
        //给东西
        giveLoots(cdata, pCombat, true, loot_copy);
        //翻牌展现发送所有掉落给客户端
        json_spirit::Array get_list;
        itemlistToArray(bc->m_Item_list, get_list);
        pCombat->m_result_obj.push_back( Pair("get_total", get_list) );
        cdata->m_tasks.updateTask(GOAL_COPY_ATTACK, pCombat->m_data_id, 1);
        cdata->m_tasks.updateTask(GOAL_DAILY_COPY, pCombat->m_data_id, 1);
        cdata->m_tasks.updateTask(GOAL_DAILY_COPY_ATTACK, 0, 1);
        //副本进度
        int tmp = cdata->queryExtraData(char_data_type_normal, char_data_normal_copy);
        if (tmp < bc->m_id)
        {
            cdata->setExtraData(char_data_type_normal, char_data_normal_copy, bc->m_id);
        }
        boost::shared_ptr<CharMapCopyData> pcmcd = getCharCopys(pCombat->m_players[0].m_cid,bc->m_mapid);
        if (pcmcd.get())
        {
            pcmcd->checkFinish();
        }
    }
    pcd->save();
    return HC_SUCCESS;
}

boost::shared_ptr<baseCopyGoods> copyMgr::GetBaseCopyGoods(int id)
{
    if (m_base_copy_goods.find(id) != m_base_copy_goods.end())
    {
        return m_base_copy_goods[id];
    }
    boost::shared_ptr<baseCopyGoods> tmp;
    tmp.reset();
    return tmp;
}

int copyMgr::getCopyShop(int cid, json_spirit::mObject& o, json_spirit::Object& robj)
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
    int can_buy_mapid = getCharCurMap(cid);
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<baseCopyGoods> >::iterator it = m_base_copy_goods.begin();
    while(it != m_base_copy_goods.end())
    {
        if (it->second.get())
        {
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", it->second->id));
                obj.push_back( Pair("cost", it->second->cost));
                obj.push_back( Pair("need_copy_map", it->second->need_copy_map));
                obj.push_back( Pair("can_buy_map", can_buy_mapid));
                obj.push_back( Pair("can_buy_map_name", m_copy_maps[it->second->need_copy_map-1]->m_name));
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

int copyMgr::GetCopyFinishReward(int cid, int mapid, int type, json_spirit::Object& robj)
{
    if (mapid < 1 || mapid > m_max_mapid)
    {
        mapid = m_max_mapid;
    }
    if (!m_copy_maps[mapid-1].get())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    int idx = 0;
    if (type == 1)
    {
        idx = char_data_normal_copy_reward_start + mapid;
        int state = cdata->queryExtraData(char_data_type_normal, idx);
        if (state != 1)
        {
            return HC_ERROR;
        }
    }
    else if(type == 2)
    {
        idx = char_data_normal_copy_perfect_reward_start + mapid;
        int state = cdata->queryExtraData(char_data_type_normal, idx);
        if (state != 1)
        {
            return HC_ERROR;
        }
    }
    else
    {
        return HC_ERROR;
    }
    if (idx == 0)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharMapCopyData> pcmcd = getCharCopys(cid, mapid);
    if (pcmcd.get())
    {
        std::list<Item> items;
        if (type == 1)
        {
            items = m_copy_maps[pcmcd->m_mapid-1]->m_finish_reward;
        }
        else if(type == 2)
        {
            items = m_copy_maps[pcmcd->m_mapid-1]->m_perfect_reward;
            /*********** 广播 ************/
            std::string msg = strCopyPerfectFinish;
            str_replace(msg, "$W", MakeCharNameLink(cdata->m_name,cdata->m_nick.get_string()));
            str_replace(msg, "$T", m_copy_maps[mapid-1]->m_name);
            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }
        else
        {
            return HC_ERROR;
        }
        if (!cdata->m_bag.hasSlot(itemlistNeedBagSlot(items)))
        {
            return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
        }
        giveLoots(cdata.get(),items,NULL,&robj,true, loot_copy_finish);
        cdata->setExtraData(char_data_type_normal, idx, 2);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

